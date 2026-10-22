// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: FSM coverage detect pass
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// FSM COVERAGE DETECT:
//      Walk clocked always blocks while the original FSM structure is still
//      present, build a per-FSM V3Graph representation of the extracted
//      states/transitions, then immediately lower that completed graph state
//      into the final coverage declarations, previous-state tracking, and
//      active blocks needed to implement FSM state and arc coverage in the
//      generated model.
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"

#ضمن "V3FsmDetect.h"

#ضمن "V3Ast.h"
#ضمن "V3Control.h"
#ضمن "V3Graph.h"
#ضمن "V3SharedTmps.h"

#ضمن <algorithm>
#ضمن <cctype>
#ضمن <map>
#ضمن <memory>
#ضمن <unordered_map>
#ضمن <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

نطاق {

// Width-preserving FSM state identity. FSM detection needs a stable key for
// graph vertices and lookup tables, but lowering still needs the original
// folded Verilog value so emitted comparisons keep the correct width and bits.
صنف FsmStateValue final {
    // Hash/equality key only. It deliberately ignores signedness because
    // signed and unsigned constants with the same width and bits denote the
    // same encoded FSM state.
    string m_key;  // Canonical "width:value" identity, independent of signedness

    // Semantic value. This is what diagnostics and lowering use when printing
    // values or rebuilding AstConst nodes for instrumentation.
    V3Number m_num;  // Original folded value, preserving width for lowered comparisons

    ساكن string makeKey(ثابت V3Number& num) {
        V3Number keyNum = num;
        // Signedness does not change FSM state identity: same width and bits
        // should address the same graph vertex.
        keyNum.isSigned(خطأ);
        أرجع cvtToStr(keyNum.width()) + ":" + keyNum.ascii(𐡒𐡔𐡈, 𐡒𐡔𐡈);
    }

عام:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    FsmStateValue()
        : m_key{"1:1'h0"}
        , m_num{static_cast<AstNode*>(nullptr), 1, 0} {}
    explicit FsmStateValue(ثابت V3Number& num)
        : m_key{makeKey(num)}
        , m_num{num} {}

    ثابت string& key() ثابت { أرجع m_key; }
    ثابت V3Number& num() ثابت { أرجع m_num; }
    string ascii() ثابت { أرجع m_num.ascii(𐡒𐡔𐡈, 𐡒𐡔𐡈); }
    string warnText() ثابت {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        𐡄𐡍 (m_num.width() <= 32) أرجع cvtToStr(m_num.toUInt());
        أرجع ascii();
    }

    منطقي operator==(ثابت FsmStateValue& rhs) ثابت { أرجع m_key == rhs.m_key; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
بنية FsmStateValueHash final {
    size_t operator()(ثابت FsmStateValue& value) ثابت {
        أرجع std::hash<string>{}(value.key());
    }
};

// Captures one sensitivity-list entry so the lowering phase can later rebuild
// an active block with the same triggering event control.
بنية FsmSenDesc final {
    // Encoded edge kind copied from AstSenItem::edgeType() so lowering can
    // rebuild the same trigger semantics on the synthesized coverage block.
    VEdgeType::en edgeType = static_cast<VEdgeType::en>(0);
    // Triggering signal in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
};

// Captures the simple reset predicate shape that survives to this pass after
// earlier normalization so reset arcs can be reconstructed during lowering.
بنية FsmResetCondDesc final {
    // Reset signal used by the FSM in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
    منطقي activeLow = خطأ;
};

صنف FsmResetArcDesc final {
    FsmStateValue m_toValue;  // Encoded reset target state.
    AstNode* m_nodep = nullptr;  // Source node for warnings and emitted metadata.
    AstNodeExpr* m_valuep = nullptr;  // Expression that provided the reset value.

عام:
    FsmResetArcDesc() = افتراضي;
    FsmResetArcDesc(FsmStateValue toValue, AstNodeAssign* nodep)
        : m_toValue{toValue}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(FsmStateValue toValue, AstNode* nodep, AstNodeExpr* valuep)
        : m_toValue{toValue}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    FsmStateValue toValue() ثابت { أرجع m_toValue; }
    AstNode* nodep() ثابت { أرجع m_nodep; }
    AstNodeExpr* valuep() ثابت { أرجع m_valuep; }
};

بنية FsmWrapperRoles final {
    string dPort;
    string qPort;
    string clkPort;
    string rstPort;
    string rstValParam;
    منطقي hasRstActiveLow = خطأ;
    منطقي rstActiveLow = خطأ;
};

ساكن منطقي fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, ثابت string& portName,
                                                  منطقي& activeLow) {
    منطقي matched = خطأ;
    cellp->modp()->foreach([&](AstSenItem* itemp) {
        AstNodeVarRef* ثابت vrefp = itemp->varrefp();
        𐡄𐡍 (!vrefp) أرجع;
        𐡄𐡍 (vrefp->varp()->name() != portName) أرجع;
        activeLow = itemp->edgeType() == VEdgeType::ET_NEGEDGE;
        matched = 𐡒𐡔𐡈;
    });
    أرجع matched;
}

ساكن ثابت V3Control::FsmRegisterWrapper* fsmRegisterWrapperDesc(AstCell* cellp) {
    AstNodeModule* ثابت modp = cellp->modp();
    ثابت string origName = modp->origName();
    𐡄𐡍 (ثابت V3Control::FsmRegisterWrapper* ثابت descp
        = V3Control::getFsmRegisterWrapper(origName)) {
        أرجع descp;
    }
    أرجع V3Control::getFsmRegisterWrapper(modp->prettyDehashOrigOrName());
}

ساكن FsmWrapperRoles rolesFromDesc(ثابت V3Control::FsmRegisterWrapper& desc) {
    FsmWrapperRoles roles;
    roles.dPort = desc.d;
    roles.qPort = desc.q;
    roles.clkPort = desc.clock;
    roles.rstPort = desc.reset;
    roles.rstValParam = desc.resetValue;
    أرجع roles;
}

صنف FsmRegisterCandidate final {
    AstScope* m_scopep = nullptr;  // Owning scope for the paired FSM.
    AstAlways* m_alwaysp = nullptr;  // Register process that commits the state.
    AstVarScope* m_stateVscp = nullptr;  // Registered FSM state variable.
    AstVarScope* m_sampleVscp = nullptr;  // Variable sampled by lowered coverage logic.
    AstVarScope* m_nextVscp = nullptr;  // Next-state variable or same state var for 1-block FSMs.
    std::vector<FsmSenDesc> m_senses;  // Event controls for recreated coverage blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate, if any.
    std::vector<FsmResetArcDesc> m_resetArcs;  // Reset target arcs recovered during detect.
    منطقي m_hasResetCond = خطأ;  // Whether the FSM had a modeled reset predicate.
    منطقي m_inclCond = خطأ;  // Whether conditional/default arcs are kept explicitly.
    FileLine* m_flp = nullptr;  // Representative source location.

عام:
    AstScope* scopep() ثابت { أرجع m_scopep; }
    فراغ scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* alwaysp() ثابت { أرجع m_alwaysp; }
    فراغ alwaysp(AstAlways* alwaysp) { m_alwaysp = alwaysp; }
    AstVarScope* stateVscp() ثابت { أرجع m_stateVscp; }
    فراغ stateVscp(AstVarScope* vscp) { m_stateVscp = vscp; }
    AstVarScope* sampleVscp() ثابت { أرجع m_sampleVscp ? m_sampleVscp : m_stateVscp; }
    فراغ sampleVscp(AstVarScope* vscp) { m_sampleVscp = vscp; }
    AstVarScope* nextVscp() ثابت { أرجع m_nextVscp; }
    فراغ nextVscp(AstVarScope* vscp) { m_nextVscp = vscp; }
    ثابت std::vector<FsmSenDesc>& senses() ثابت { أرجع m_senses; }
    std::vector<FsmSenDesc>& senses() { أرجع m_senses; }
    ثابت FsmResetCondDesc& resetCond() ثابت { أرجع m_resetCond; }
    FsmResetCondDesc& resetCond() { أرجع m_resetCond; }
    ثابت std::vector<FsmResetArcDesc>& resetArcs() ثابت { أرجع m_resetArcs; }
    std::vector<FsmResetArcDesc>& resetArcs() { أرجع m_resetArcs; }
    منطقي hasResetCond() ثابت { أرجع m_hasResetCond; }
    فراغ hasResetCond(منطقي flag) { m_hasResetCond = flag; }
    منطقي inclCond() ثابت { أرجع m_inclCond; }
    فراغ inclCond(منطقي flag) { m_inclCond = flag; }
    FileLine* fileline() ثابت { أرجع m_flp; }
    فراغ fileline(FileLine* flp) { m_flp = flp; }
};

صنف FsmComboAlways final {
    AstScope* ثابت m_scopep = nullptr;  // Owning scope for the combinational process.
    AstAlways* ثابت m_alwaysp = nullptr;  // Candidate transition process.

عام:
    FsmComboAlways() = افتراضي;
    FsmComboAlways(AstScope* scopep, AstAlways* alwaysp)
        : m_scopep{scopep}
        , m_alwaysp{alwaysp} {}

    AstScope* scopep() ثابت { أرجع m_scopep; }
    AstAlways* alwaysp() ثابت { أرجع m_alwaysp; }
};

صنف FsmGraph;

صنف FsmVertex VL_NOT_FINAL : عام V3GraphVertex {
    VL_RTTI_IMPL(FsmVertex, V3GraphVertex)

عام:
    تعداد صنف Kind : uint8_t { STATE, RESET_ANY, DEFAULT_ANY };

خاص:
    Kind m_kind;  // State vs synthetic ANY/default vertex role.
    string m_label;  // User-facing state or pseudo-state label.
    FsmStateValue m_value;  // Encoded state value for real state vertices.

محمي:
    FsmVertex(V3Graph* graphp, Kind kind, string label, FsmStateValue value) VL_MT_DISABLED
        : V3GraphVertex{graphp},
          m_kind{kind},
          m_label{label},
          m_value{value} {}
    ~FsmVertex() override = افتراضي;

عام:
    Kind kind() ثابت { أرجع m_kind; }
    منطقي isState() ثابت { أرجع m_kind == Kind::STATE; }
    منطقي isResetAny() ثابت { أرجع m_kind == Kind::RESET_ANY; }
    منطقي isDefaultAny() ثابت { أرجع m_kind == Kind::DEFAULT_ANY; }
    ثابت string& label() ثابت { أرجع m_label; }
    ثابت FsmStateValue& value() ثابت { أرجع m_value; }

    string name() ثابت override VL_MT_SAFE { أرجع m_label + "=" + m_value.ascii(); }
};

صنف FsmStateVertex final : عام FsmVertex {
    VL_RTTI_IMPL(FsmStateVertex, FsmVertex)

عام:
    FsmStateVertex(V3Graph* graphp, string label, FsmStateValue value) VL_MT_DISABLED
        : FsmVertex{graphp, Kind::STATE, label, value} {}
    ~FsmStateVertex() override = افتراضي;

    string dotColor() ثابت override { أرجع "lightblue"; }
    string dotShape() ثابت override { أرجع "ellipse"; }
};

صنف FsmPseudoVertex final : عام FsmVertex {
    VL_RTTI_IMPL(FsmPseudoVertex, FsmVertex)

عام:
    FsmPseudoVertex(V3Graph* graphp, Kind kind, string label) VL_MT_DISABLED
        : FsmVertex{graphp, kind, label, FsmStateValue{}} {}
    ~FsmPseudoVertex() override = افتراضي;

    string name() ثابت override VL_MT_SAFE { أرجع label(); }
    string dotColor() ثابت override { أرجع isResetAny() ? "darkgreen" : "orange"; }
    string dotShape() ثابت override { أرجع "diamond"; }
};

صنف FsmArcEdge final : عام V3GraphEdge {
    VL_RTTI_IMPL(FsmArcEdge, V3GraphEdge)
    منطقي m_isReset = خطأ;  // Arc originates from the synthetic reset source.
    منطقي m_isCond = خطأ;  // Arc came from a conditional next-state split.
    منطقي m_isDefault = خطأ;  // Arc represents a case default source.
    FileLine* m_flp = nullptr;  // Source location for emitted coverage metadata.

عام:
    FsmArcEdge(V3Graph* graphp, FsmVertex* fromp, FsmStateVertex* top, منطقي isReset, منطقي isCond,
               منطقي isDefault, FileLine* flp) VL_MT_DISABLED : V3GraphEdge{graphp, fromp, top, 1},
                                                               m_isReset{isReset},
                                                               m_isCond{isCond},
                                                               m_isDefault{isDefault},
                                                               m_flp{flp} {}
    ~FsmArcEdge() override = افتراضي;

    منطقي isReset() ثابت { أرجع m_isReset; }
    منطقي isCond() ثابت { أرجع m_isCond; }
    منطقي isDefault() ثابت { أرجع m_isDefault; }
    FileLine* fileline() ثابت { أرجع m_flp; }

    string dotLabel() ثابت override {
        𐡄𐡍 (m_isReset) أرجع "reset";
        𐡄𐡍 (m_isDefault) أرجع "default";
        𐡄𐡍 (m_isCond) أرجع "cond";
        أرجع "";
    }
    string dotColor() ثابت override {
        𐡄𐡍 (m_isReset) أرجع "darkgreen";
        𐡄𐡍 (m_isDefault) أرجع "orange";
        𐡄𐡍 (m_isCond) أرجع "blue";
        أرجع "black";
    }
};

// One graph per detected FSM. Graph-level metadata captures the non-graph
// context needed to lower states/arcs back into the AST after detection.
صنف FsmGraph final : عام V3Graph {
    AstScope* m_scopep = nullptr;  // Owning scoped block for the detected FSM.
    AstAlways* m_stateAlwaysp = nullptr;  // Register always block being instrumented.
    string m_stateVarName;  // Pretty state variable name for user-visible output.
    string m_stateVarInternalName;  // Internal state symbol name for dump tags.
    AstVarScope* m_stateVarScopep = nullptr;  // Scoped state variable being tracked.
    AstVarScope* m_sampleVarScopep = nullptr;  // Scoped variable sampled by coverage logic.
    std::vector<FsmSenDesc> m_senses;  // Saved event controls for recreated active blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate shape, if one exists.
    منطقي m_hasResetCond = خطأ;  // Whether the detected FSM had a reset branch.
    منطقي m_inclCond = خطأ;  // Whether conditional arcs should be kept explicitly.
    FileLine* m_flp = nullptr;  // Representative source location for declarations/arcs.
    std::unordered_map<FsmStateValue, FsmStateVertex*, FsmStateValueHash>
        m_stateVertices;  // Value to state map.
    FsmPseudoVertex* m_resetVertexp = nullptr;  // Synthetic ANY source for reset arcs.
    FsmPseudoVertex* m_defaultVertexp = nullptr;  // Synthetic default source for case defaults.

عام:
    FsmGraph() VL_MT_DISABLED
        : m_resetVertexp{جديد FsmPseudoVertex{هذا, FsmVertex::Kind::RESET_ANY, "ANY"}},
          m_defaultVertexp{جديد FsmPseudoVertex{هذا, FsmVertex::Kind::DEFAULT_ANY, "default"}} {}

    AstScope* scopep() ثابت { أرجع m_scopep; }
    فراغ scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* stateAlwaysp() ثابت { أرجع m_stateAlwaysp; }
    فراغ stateAlwaysp(AstAlways* alwaysp) { m_stateAlwaysp = alwaysp; }
    ثابت string& stateVarName() ثابت { أرجع m_stateVarName; }
    فراغ stateVarName(ثابت string& name) { m_stateVarName = name; }
    ثابت string& stateVarInternalName() ثابت { أرجع m_stateVarInternalName; }
    فراغ stateVarInternalName(ثابت string& name) { m_stateVarInternalName = name; }
    AstVarScope* stateVarScopep() ثابت { أرجع m_stateVarScopep; }
    فراغ stateVarScopep(AstVarScope* vscp) { m_stateVarScopep = vscp; }
    AstVarScope* sampleVarScopep() ثابت {
        أرجع m_sampleVarScopep ? m_sampleVarScopep : m_stateVarScopep;
    }
    فراغ sampleVarScopep(AstVarScope* vscp) { m_sampleVarScopep = vscp; }
    ثابت std::vector<FsmSenDesc>& senses() ثابت { أرجع m_senses; }
    std::vector<FsmSenDesc>& senses() { أرجع m_senses; }
    ثابت FsmResetCondDesc& resetCond() ثابت { أرجع m_resetCond; }
    FsmResetCondDesc& resetCond() { أرجع m_resetCond; }
    منطقي hasResetCond() ثابت { أرجع m_hasResetCond; }
    فراغ hasResetCond(منطقي flag) { m_hasResetCond = flag; }
    منطقي inclCond() ثابت { أرجع m_inclCond; }
    فراغ inclCond(منطقي flag) { m_inclCond = flag; }
    FileLine* fileline() ثابت { أرجع m_flp; }
    فراغ fileline(FileLine* flp) { m_flp = flp; }

    FsmStateVertex* addStateVertex(string label, FsmStateValue value) VL_MT_DISABLED {
        FsmStateVertex* ثابت vertexp = جديد FsmStateVertex{هذا, label, value};
        m_stateVertices.emplace(value, vertexp);
        أرجع vertexp;
    }
    FsmPseudoVertex* resetAnyVertex() VL_MT_DISABLED { أرجع m_resetVertexp; }
    FsmPseudoVertex* defaultAnyVertex() VL_MT_DISABLED { أرجع m_defaultVertexp; }
    FsmArcEdge* addArc(FsmStateValue fromValue, FsmStateValue toValue, منطقي isReset, منطقي isCond,
                       منطقي isDefault, FileLine* flp) VL_MT_DISABLED {
        FsmStateVertex* ثابت top = m_stateVertices.at(toValue);
        FsmVertex* fromp = nullptr;
        𐡄𐡍 (isReset) {
            fromp = resetAnyVertex();
        } وإلا 𐡄𐡍 (isDefault) {
            fromp = defaultAnyVertex();
        } وإلا {
            fromp = m_stateVertices.at(fromValue);
        }
        أرجع جديد FsmArcEdge{هذا, fromp, top, isReset, isCond, isDefault, flp};
    }

    string name() ثابت VL_MT_SAFE {
        أرجع "FSM "
               + (m_stateVarName.empty() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dumpTag(size_t index) ثابت {
        string tag = stateVarInternalName();
        لكل (حرف& ch : tag) {
            𐡄𐡍 (!std::isalnum(static_cast<unsigned حرف>(ch))) ch = '_';
        }
        أرجع "fsm_" + cvtToStr(index) + "_" + tag;
    }
};

بنية DetectedFsm final {
    std::unique_ptr<FsmGraph> graphp;  // Extracted graph for one detected FSM candidate.
};

بنية FsmCaseCandidate final {
    AstNode* warnNodep = nullptr;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = nullptr;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
بنية FsmStateComparison final {
    AstVarScope* stateVscp = nullptr;  // Compared state variable
    AstNodeExpr* valuep = nullptr;  // Compared constant value expression
    FsmStateValue value;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
بنية FsmIfBranch final {
    AstIf* ifp = nullptr;  // Source if/else-if node
    AstNode* stmtsp = nullptr;  // Branch body
    AstNodeExpr* valuep = nullptr;  // Source state value expression
    FsmStateValue fromValue;  // Encoded source state value
    منطقي hasTopGuard = خطأ;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
بنية FsmIfChainCandidate final {
    AstIf* ifp = nullptr;  // Top-level if-chain node
    AstVarScope* compareVscp = nullptr;  // Variable used by every state comparison
    std::vector<FsmIfBranch> branches;  // Recognized state-dispatch branches
    AstNode* defaultStmtsp = nullptr;  // Optional final else body
};

// Aliases are accepted only when they are equivalent to spelling the state
// comparison inline; this avoids inferring FSM semantics from arbitrary logic.
using FsmAliasMap = std::unordered_map<ثابت AstVarScope*, FsmStateComparison>;
using FsmCellPortMap = std::unordered_map<string, AstVarScope*>;
using FsmCellPortAliasMap = std::unordered_map<ثابت AstCell*, FsmCellPortMap>;

بنية StateConstLabel final {
    string text;
    منطقي fromParam = خطأ;
    size_t stateIndex = 0;
};

بنية FsmStateSpace final {
    std::vector<std::pair<string, FsmStateValue>> states;  // User label and encoded value
    std::unordered_map<FsmStateValue, StateConstLabel, FsmStateValueHash>
        labels;  // Encoded value to label
    AstVar* stateVarp = nullptr;  // Tracked FSM state variable
    منطقي enumBacked = خطأ;  // Whether states came from an enum declaration
};

// Local shared state between the two adjacent FSM coverage phases. Detection
// fills this with recovered FSM graphs; lowering consumes the completed graphs
// immediately afterward without needing any AST serialization bridge.
صنف FsmState final {
    // All detected FSMs in discovery order. This is the only bridge between
    // the adjacent detect and lower phases, so the second phase never needs to
    // rediscover or serialize the extracted machine.
    std::vector<DetectedFsm> m_fsms;
    std::map<ثابت AstVarScope*, size_t> m_fsmIndex;

عام:
    DetectedFsm& fsmFor(AstVarScope* stateVscp) {
        ثابت std::map<ثابت AstVarScope*, size_t>::const_iterator it = m_fsmIndex.find(stateVscp);
        𐡄𐡍 (it != m_fsmIndex.end()) أرجع m_fsms.at(it->second);
        ثابت size_t index = m_fsms.size();
        m_fsmIndex.emplace(stateVscp, index);
        m_fsms.emplace_back();
        أرجع m_fsms.back();
    }
    ثابت std::vector<DetectedFsm>& fsms() ثابت { أرجع m_fsms; }
};

// Detection runs while the original clocked/case structure is still intact and
// populates graph-backed FSM models without mutating the tree mid-traversal.
// This pass is intentionally conservative: for this PR we only lock down the
// small set of transition/selector forms that are already stable in the
// normalized AST we see here. The remaining reject branches are therefore
// mostly future-feature boundaries, not accidental dead code.
صنف FsmDetectVisitor final : عام VNVisitor {
    // STATE - for current visit position (use VL_RESTORER)
    FsmState& m_state;
    AstScope* m_scopep = nullptr;
    std::vector<FsmRegisterCandidate> m_registerCandidates;
    // Deferring one-block detection avoids making continuous alias support
    // depend on whether the assign appears before or after the always block.
    std::vector<FsmComboAlways> m_oneBlockAlwayss;
    std::vector<FsmComboAlways> m_comboAlwayss;
    std::vector<FsmComboAlways> m_nonComboAlwayss;
    // Wrapper FSM detection has a second path for designs compiled without
    // inlining. In that shape the state register stays behind an AstCell, so we
    // remember candidate cells and resolve them only after the surrounding
    // transition logic and post-link port wiring have both been seen.
    std::vector<std::pair<AstScope*, AstCell*>> m_wrapperCells;
    std::unordered_map<ثابت AstVarScope*, FsmCaseCandidate> m_comboPaired;
    // Continuous aliases are order-independent, while procedural aliases must
    // remain source-order scoped to avoid using assignments not yet executed.
    FsmAliasMap m_stateAliases;
    std::unordered_set<ثابت AstVarScope*> m_ambiguousStateAliases;
    // A surviving wrapper's semantic d/q relationship is split across the
    // parent scope and the child module scope. This table is the narrow bridge
    // between those scopes: only transparent port aliases are recorded, so the
    // detector does not become a general cross-module dataflow engine.
    FsmCellPortAliasMap m_cellPortAliases;
    FsmCellPortAliasMap m_cellPortChildAliases;

    // METHODS
    // Enum-backed FSMs may be wrapped in refs/typedefs; normalize to the
    // underlying enum type before deciding whether a case is a candidate.
    ساكن AstNodeDType* unwrapEnumCandidate(AstNodeDType* dtypep) {
        أرجع dtypep->skipRefToEnump();
    }

    ساكن string candidateConflictContext(AstNode* laterNodep,
                                           ثابت FsmCaseCandidate& firstCand) {
        أرجع '\n' + laterNodep->warnContextPrimary() + firstCand.warnNodep->warnOther()
               + "... Location of first supported candidate for "
               + firstCand.stateVscp->prettyNameQ() + '\n'
               + firstCand.warnNodep->warnContextSecondary();
    }

    ساكن منطقي rejectFsmWrapperCell(AstCell* cellp, ثابت string& reason) {
        cellp->v3warn(COVERIGN, "Ignoring unsupported: " + reason);
        أرجع خطأ;
    }

    ساكن منطقي simpleParamStateValue(AstCell* cellp, ثابت string& name, FsmStateValue& value,
                                      AstNodeExpr*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = nullptr;
        لكل (AstNode* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ثابت varp = VN_CAST(stmtp, Var);
            𐡄𐡍 (!varp || !varp->isParam() || varp->name() != name) استمر;
            valuepr = VN_AS(varp->valuep(), NodeExpr);
            أرجع constValueStatus(valuepr, value) == ConstValueStatus::OK;
        }
        أرجع خطأ;
    }

    ساكن منطقي childPortInScope(AstVarScope* vscp, AstScope* parentScopep, AstCell*& cellpr) {
        𐡄𐡍 (!vscp->varp()->isIO()) أرجع خطأ;
        AstScope* ثابت scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        𐡄𐡍 (scopep->aboveScopep() != parentScopep) أرجع خطأ;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        أرجع 𐡒𐡔𐡈;
    }

    ساكن AstVarScope* simpleAssignVarScope(AstNodeExpr* exprp) {
        AstVarRef* ثابت vrefp = VN_CAST(exprp, VarRef);
        أرجع vrefp ? vrefp->varScopep() : nullptr;
    }

    فراغ addWrapperCell(AstScope* scopep, AstCell* cellp) {
        m_cellPortAliases.emplace(cellp, FsmCellPortMap{});
        m_cellPortChildAliases.emplace(cellp, FsmCellPortMap{});
        ثابت std::pair<AstScope*, AstCell*> item{scopep, cellp};
        𐡄𐡍 (std::find(m_wrapperCells.cbegin(), m_wrapperCells.cend(), item)
            != m_wrapperCells.cend()) {
            أرجع;
        }
        m_wrapperCells.emplace_back(item);
    }

    فراغ collectCellPortAlias(AstAssignW* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* ثابت lhsVscp = simpleAssignVarScope(nodep->lhsp());
        AstVarScope* ثابت rhsVscp = simpleAssignVarScope(nodep->rhsp());
        𐡄𐡍 (!lhsVscp || !rhsVscp) أرجع;
        AstCell* cellp = nullptr;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        𐡄𐡍 (childPortInScope(lhsVscp, m_scopep, cellp)) {
            𐡄𐡍 (!fsmRegisterWrapperDesc(cellp)) أرجع;
            UASSERT_OBJ(lhsVscp->varp()->isInput(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cellPortAliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cellPortChildAliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            addWrapperCell(m_scopep, cellp);
        } وإلا 𐡄𐡍 (childPortInScope(rhsVscp, m_scopep, cellp)) {
            𐡄𐡍 (!fsmRegisterWrapperDesc(cellp)) أرجع;
            UASSERT_OBJ(rhsVscp->varp()->isWritable(), nodep,
                        "Child-side port alias rhs should be writable");
            m_cellPortAliases[cellp][rhsVscp->varp()->name()] = lhsVscp;
            m_cellPortChildAliases[cellp][rhsVscp->varp()->name()] = rhsVscp;
            addWrapperCell(m_scopep, cellp);
        }
    }

    AstVarScope* roleVarScope(AstCell* cellp, ثابت string& portName) ثابت {
        // At this point explicit AstPin expressions have been lowered away, so
        // role resolution crosses the wrapper boundary only through the
        // transparent alias table above. This keeps wrapper support aligned with
        // direct-register detection instead of growing into interprocedural FSM
        // inference.
        ثابت FsmCellPortMap& ports = m_cellPortAliases.at(cellp);
        ثابت FsmCellPortMap::const_iterator portIt = ports.find(portName);
        أرجع portIt == ports.end() ? nullptr : portIt->second;
    }

    AstVarScope* childRoleVarScope(AstCell* cellp, ثابت string& portName) ثابت {
        ثابت FsmCellPortMap& ports = m_cellPortChildAliases.at(cellp);
        ثابت FsmCellPortMap::const_iterator portIt = ports.find(portName);
        أرجع portIt == ports.end() ? nullptr : portIt->second;
    }

    صنف RegisterAlwaysAnalyzer final {
        AstScope* ثابت m_scopep;  // Scope containing the always block(s) being analyzed

    عام:
        explicit RegisterAlwaysAnalyzer(AstScope* scopep)
            : m_scopep{scopep} {}

        std::vector<std::pair<AstCase*, AstNodeExpr*>>
        oneBlockCandidates(AstAlways* alwaysp) ثابت {
            std::vector<std::pair<AstCase*, AstNodeExpr*>> candidates;
            AstNode* ثابت stmtsp = alwaysp->stmtsp();
            𐡄𐡍 (AstIf* ثابت firstIfp = VN_CAST(stmtsp, If)) {
                𐡄𐡍 (AstCase* ثابت casep = VN_CAST(firstIfp->elsesp(), Case)) {
                    candidates.emplace_back(casep,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            لكل (AstNode* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                𐡄𐡍 (AstCase* ثابت casep = VN_CAST(nodep, Case))
                    candidates.emplace_back(casep, nullptr);
            }
            أرجع candidates;
        }

        std::vector<std::pair<AstIf*, AstNodeExpr*>>
        oneBlockIfCandidates(AstAlways* alwaysp) ثابت {
            std::vector<std::pair<AstIf*, AstNodeExpr*>> candidates;
            AstNode* ثابت stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            𐡄𐡍 (AstIf* ثابت firstIfp = VN_CAST(stmtsp, If)) {
                𐡄𐡍 (AstIf* ثابت chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(firstIfp->elsesp()), If)) {
                    candidates.emplace_back(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            لكل (AstNode* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                𐡄𐡍 (AstIf* ثابت ifp = VN_CAST(nodep, If)) candidates.emplace_back(ifp, nullptr);
            }
            أرجع candidates;
        }

        منطقي matchRegisterCandidate(AstAlways* alwaysp, FsmRegisterCandidate& cand) ثابت {
            أرجع FsmDetectVisitor::matchRegisterAlways(alwaysp, m_scopep, cand);
        }

        فراغ buildOneBlockCandidate(AstAlways* alwaysp, AstVarScope* vscp, AstNodeExpr* resetCondp,
                                    FsmRegisterCandidate& reg) ثابت {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.nextVscp(vscp);
            reg.senses() = FsmDetectVisitor::describeSenTree(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != nullptr);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* ثابت firstIfp = VN_CAST(alwaysp->stmtsp(), If);
            𐡄𐡍 (firstIfp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = nullptr;
                ثابت ResetAssignStatus resetStatus = FsmDetectVisitor::collectConstStateAssigns(
                    firstIfp->thensp(), resetStateVscp, reg.resetArcs());
                𐡄𐡍 (resetStatus == ResetAssignStatus::EMPTY) {
                    reg.resetArcs().clear();
                } وإلا 𐡄𐡍 (resetStatus == ResetAssignStatus::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().clear();
                    FsmStateValue resetValue;
                    AstNode* ثابت thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(firstIfp->thensp());
                    UASSERT_OBJ(thenNodep, firstIfp,
                                "one-block reset fallback requires a non-empty reset branch");
                    AstNodeAssign* ثابت resetAssp = FsmDetectVisitor::directConstStateAssignNode(
                        thenNodep, resetStateVscp, resetValue);
                    𐡄𐡍 (resetAssp && resetStateVscp == vscp) {
                        reg.resetArcs().emplace_back(resetValue, resetAssp);
                    }
                } وإلا 𐡄𐡍 (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                    reg.resetArcs().clear();
                }
            }
        }
    };

    منطقي matchFsmWrapperCell(AstScope* scopep, AstCell* cellp, FsmRegisterCandidate& cand) ثابت {
        FsmWrapperRoles roles = rolesFromDesc(*fsmRegisterWrapperDesc(cellp));

        AstVarScope* ثابت nextVscp = roleVarScope(cellp, roles.dPort);
        AstVarScope* ثابت stateVscp = roleVarScope(cellp, roles.qPort);
        𐡄𐡍 (!nextVscp || !stateVscp) {
            أرجع rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* ثابت clkVscp = roleVarScope(cellp, roles.clkPort);
        𐡄𐡍 (!clkVscp) {
            أرجع rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper instance requires a simple clock connection");
        }

        FsmSenDesc clkSense;
        clkSense.edgeType = VEdgeType::ET_POSEDGE;
        clkSense.varScopep = clkVscp;
        cand.senses().push_back(clkSense);

        AstVarScope* resetVscp = nullptr;
        𐡄𐡍 (!roles.rstPort.empty()) resetVscp = roleVarScope(cellp, roles.rstPort);
        𐡄𐡍 (resetVscp) {
            // The descriptor identifies the reset port but not its polarity. Use
            // the wrapper's own event control AST as the contract for sampling
            // the connected parent signal.
            منطقي inferredActiveLow = خطأ;
            𐡄𐡍 (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rstPort, inferredActiveLow)) {
                roles.hasRstActiveLow = 𐡒𐡔𐡈;
                roles.rstActiveLow = inferredActiveLow;
            }
        }

        AstNodeExpr* resetValuep = nullptr;
        FsmStateValue resetValue;
        ثابت منطقي hasResetValue
            = !roles.rstValParam.empty()
              && simpleParamStateValue(cellp, roles.rstValParam, resetValue, resetValuep);
        𐡄𐡍 (resetVscp && roles.hasRstActiveLow && hasResetValue) {
            FsmSenDesc rstSense;
            rstSense.edgeType = roles.rstActiveLow ? VEdgeType::ET_NEGEDGE : VEdgeType::ET_POSEDGE;
            rstSense.varScopep = resetVscp;
            cand.senses().push_back(rstSense);
            cand.resetCond().varScopep = resetVscp;
            cand.resetCond().activeLow = roles.rstActiveLow;
            cand.hasResetCond(𐡒𐡔𐡈);
            cand.resetArcs().emplace_back(resetValue, cellp, resetValuep);
        } وإلا 𐡄𐡍 (!roles.rstPort.empty() || !roles.rstValParam.empty()) {
            string reason;
            𐡄𐡍 (roles.rstPort.empty()) {
                reason = "reset port is not configured";
            } وإلا 𐡄𐡍 (!resetVscp) {
                reason = "reset connection is missing or not a simple variable";
            } وإلا 𐡄𐡍 (!roles.hasRstActiveLow) {
                reason = "reset polarity could not be inferred from the wrapper";
            } وإلا 𐡄𐡍 (roles.rstValParam.empty()) {
                reason = "reset_value parameter is not configured";
            } وإلا {
                reason = "reset_value parameter is missing or not static";
            }
            cellp->v3warn(COVERIGN,
                          "Ignoring unsupported: fsm_register_wrapper reset arcs require both "
                          "reset polarity and static reset value; "
                              + reason);
        }

        // This candidate represents a register proven through an instance
        // boundary, so there is no parent always_ff body to annotate. Lowering
        // treats null alwaysp as the explicit cell-path contract and builds its
        // sampling block from the recovered clock/reset interface instead.
        cand.scopep(scopep);
        cand.alwaysp(nullptr);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(childRoleVarScope(cellp, roles.qPort));
        cand.nextVscp(nextVscp);
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(cellp->fileline());
        أرجع 𐡒𐡔𐡈;
    }

    صنف ComboAlwaysAnalyzer final {
    عام:
        بنية ComboMatch final {
            ثابت FsmRegisterCandidate* matchedp = nullptr;
            AstNode* warnNodep = nullptr;
        };

    خاص:
        ثابت std::vector<FsmRegisterCandidate>& m_registerCandidates;

    عام:
        explicit ComboAlwaysAnalyzer(ثابت std::vector<FsmRegisterCandidate>& registerCandidates)
            : m_registerCandidates{registerCandidates} {}

        ComboMatch matchCase(AstNode* stmtsp, AstCase* casep) ثابت {
            ComboMatch match;
            AstVarRef* ثابت selp = VN_CAST(casep->exprp(), VarRef);
            𐡄𐡍 (!selp) أرجع match;
            لكل (ثابت FsmRegisterCandidate& reg : m_registerCandidates) {
                𐡄𐡍 (selp->varScopep() == reg.nextVscp()) {
                    𐡄𐡍 (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        استمر;
                    }
                } وإلا 𐡄𐡍 (selp->varScopep() != reg.stateVscp()) {
                    استمر;
                }
                AstNode* ثابت warnNodep = FsmDetectVisitor::caseSupportedTransitionNode(
                    casep, reg.nextVscp(), reg.inclCond());
                𐡄𐡍 (!warnNodep) استمر;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            أرجع match;
        }

        ComboMatch matchIfChain(AstNode* stmtsp, ثابت FsmIfChainCandidate& chain) ثابت {
            ComboMatch match;
            لكل (ثابت FsmRegisterCandidate& reg : m_registerCandidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                𐡄𐡍 (chain.compareVscp == reg.nextVscp()) {
                    𐡄𐡍 (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.nextVscp())) {
                        استمر;
                    }
                } وإلا 𐡄𐡍 (chain.compareVscp != reg.stateVscp()) {
                    استمر;
                }
                AstNode* ثابت warnNodep
                    = FsmDetectVisitor::ifChainSupportedTransitionNode(chain, reg.nextVscp());
                𐡄𐡍 (!warnNodep) استمر;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            أرجع match;
        }

        منطقي shouldWarnUnsupported(AstNode* stmtsp, AstCase* casep) ثابت {
            ثابت AstVarRef* ثابت selp = VN_CAST(casep->exprp(), VarRef);
            𐡄𐡍 (!selp) أرجع خطأ;

            أرجع std::any_of(
                m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                [&](ثابت FsmRegisterCandidate& reg) -> منطقي {
                    ثابت منطقي matchesNext = selp->varScopep() == reg.nextVscp();
                    ثابت منطقي matchesState = selp->varScopep() == reg.stateVscp();

                    𐡄𐡍 (!matchesNext && !matchesState) أرجع خطأ;
                    𐡄𐡍 (matchesNext
                        && !FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        أرجع خطأ;
                    }
                    أرجع FsmDetectVisitor::caseSupportedTransitionNode(casep, reg.nextVscp(),
                                                                         reg.inclCond());
                });
        }
    };

    // Reset arcs are only modeled for the simple signal form that survives to
    // this pass after earlier normalization.
    ساكن منطقي isSimpleResetCond(AstNodeExpr* condp) { أرجع VN_IS(condp, VarRef); }

    // Normalize the reset condition into a compact description so the lowering
    // phase can regenerate the same predicate after detection. By the time
    // this pass runs, active-low source forms such as "!rst_n" have already
    // been canonicalized to a positive-condition if/else shape, so only a
    // plain VarRef survives here.
    ساكن FsmResetCondDesc describeResetCond(AstNodeExpr* condp) {
        FsmResetCondDesc desc;
        𐡄𐡍 (AstVarRef* ثابت vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        أرجع desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    ساكن std::vector<FsmSenDesc> describeSenTree(AstSenTree* sentreep) {
        std::vector<FsmSenDesc> senses;
        لكل (AstSenItem* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), SenItem)) {
            AstNodeVarRef* ثابت vrefp = itemp->varrefp();
            𐡄𐡍 (!vrefp) استمر;
            FsmSenDesc desc;
            desc.edgeType = itemp->edgeType().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.push_back(desc);
        }
        أرجع senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    ساكن منطقي isIgnorableStmt(AstNode* nodep) { أرجع VN_IS(nodep, CoverInc); }

    ساكن AstNode* skipLeadingIgnorableStmt(AstNode* nodep) {
        طالما (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        أرجع nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    ساكن AstNode* singleMeaningfulStmt(AstNode* stmtp) {
        AstNode* resultp = nullptr;
        لكل (AstNode* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            𐡄𐡍 (isIgnorableStmt(nodep)) استمر;
            𐡄𐡍 (resultp) أرجع nullptr;
            resultp = nodep;
        }
        أرجع resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    ساكن AstNode* singleMeaningfulBranch(AstNode* branchp) {
        𐡄𐡍 (!branchp) أرجع nullptr;
        أرجع branchp;
    }

    // By fsm-detect time, non-clocked always @* blocks are already admitted through
    // a missing sentree. This helper therefore only needs to recognize
    // explicit changed-sensitivity lists such as always @(a or b); clocked and
    // event-driven forms remain out of scope.
    ساكن منطقي isPlainComboSentree(ثابت AstSenTree* sentreep) {
        UASSERT(sentreep, "plain combo sensitivity check requires a sensitivity tree");
        لكل (ثابت AstSenItem* senp = sentreep->sensesp(); senp;
             senp = VN_AS(senp->nextp(), SenItem)) {
            𐡄𐡍 (senp->edgeType() == VEdgeType::ET_CHANGED) استمر;
            أرجع خطأ;
        }
        أرجع 𐡒𐡔𐡈;
    }

    فراغ warnUnsupportedComboAlways(ثابت FsmComboAlways& combo) {
        ثابت ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        AstNode* ثابت stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        منطقي warned = خطأ;
        لكل (AstNode* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ثابت casep = VN_CAST(nodep, Case);
            𐡄𐡍 (!casep) استمر;
            𐡄𐡍 (analyzer.shouldWarnUnsupported(stmtsp, casep)) {
                casep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-clocked always "
                                        "blocks requires a combinational sensitivity list or "
                                        "always_comb");
                warned = 𐡒𐡔𐡈;
            }
            𐡄𐡍 (warned) اكسر;
        }
    }

    // Case-item bodies are single subtrees like if/else arms, not statement
    // lists, so unwrap only local begin/end wrappers here rather than walking
    // sibling case items via nextp().
    ساكن AstNodeAssign* directStateAssign(AstNode* stmtp, AstVarScope* stateVscp) {
        AstNode* ثابت nodep = singleMeaningfulBranch(stmtp);
        𐡄𐡍 (!nodep) أرجع nullptr;
        AstNodeAssign* ثابت assp = VN_CAST(nodep, NodeAssign);
        𐡄𐡍 (!assp) أرجع nullptr;
        AstVarRef* ثابت vrefp = VN_CAST(assp->lhsp(), VarRef);
        𐡄𐡍 (!vrefp || vrefp->varScopep() != stateVscp) أرجع nullptr;
        أرجع assp;
    }

    ساكن AstNodeAssign* nodeStateVarAssign(AstNode* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        AstNodeAssign* ثابت assp = VN_CAST(nodep, NodeAssign);
        𐡄𐡍 (!assp) أرجع nullptr;
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp()->baseFromp(𐡒𐡔𐡈), VarRef);
        AstVarRef* ثابت rhsp = VN_CAST(assp->rhsp(), VarRef);
        𐡄𐡍 (!rhsp || !lhsp) أرجع nullptr;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        أرجع assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    ساكن AstNodeAssign* directCondStateVarAssign(AstNode* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, AstNodeExpr*& condp,
                                                   FsmStateValue& resetValue) {
        AstNodeAssign* ثابت assp = VN_CAST(nodep, NodeAssign);
        𐡄𐡍 (!assp) أرجع nullptr;
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp()->baseFromp(𐡒𐡔𐡈), VarRef);
        AstCond* ثابت rhsp = VN_CAST(assp->rhsp(), Cond);
        𐡄𐡍 (!rhsp || !lhsp) أرجع nullptr;
        AstVarRef* ثابت elsep = VN_CAST(rhsp->elsep(), VarRef);
        𐡄𐡍 (!elsep) أرجع nullptr;
        𐡄𐡍 (constValueStatus(rhsp->thenp(), resetValue) != ConstValueStatus::OK) أرجع nullptr;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        أرجع assp;
    }

    ساكن AstNodeAssign* directConstStateAssignNode(AstNode* nodep, AstVarScope*& stateVscp,
                                                     FsmStateValue& value) {
        AstNodeAssign* ثابت assp = VN_CAST(nodep, NodeAssign);
        𐡄𐡍 (!assp) أرجع nullptr;
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp()->baseFromp(𐡒𐡔𐡈), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        𐡄𐡍 (constValueStatus(assp->rhsp(), value) != ConstValueStatus::OK) أرجع nullptr;
        stateVscp = lhsp->varScopep();
        أرجع assp;
    }

    تعداد صنف ResetAssignStatus : uint8_t {
        EMPTY,  // Reset branch had no non-coverage statements.
        NONE,  // Reset branch was not the supported direct-constant shape.
        SINGLE,  // Exactly one supported reset assignment was collected.
        MULTI_SAME_STATE  // Multiple assignments to the same FSM state var; warn and ignore.
    };

    // Reset arcs are only extracted from the single direct-constant form. If
    // user RTL assigns the same state register multiple times in the reset
    // branch, warn and skip reset-arc modeling rather than inventing multiple
    // reset transitions for an odd but legal coding style.
    ساكن ResetAssignStatus collectConstStateAssigns(AstNode* stmtp, AstVarScope*& stateVscp,
                                                      std::vector<FsmResetArcDesc>& resetArcs) {
        AstNode* nodep = skipLeadingIgnorableStmt(stmtp);
        𐡄𐡍 (!nodep) أرجع ResetAssignStatus::EMPTY;
        لكل (;; nodep = nodep->nextp()) {
            AstVarScope* assignStateVscp = nullptr;
            FsmStateValue value;
            AstNodeAssign* ثابت assp = directConstStateAssignNode(nodep, assignStateVscp, value);
            𐡄𐡍 (!assp) أرجع ResetAssignStatus::NONE;
            𐡄𐡍 (!stateVscp) stateVscp = assignStateVscp;
            𐡄𐡍 (assignStateVscp != stateVscp) أرجع ResetAssignStatus::NONE;
            𐡄𐡍 (!resetArcs.empty()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.clear();
                أرجع ResetAssignStatus::MULTI_SAME_STATE;
            }
            resetArcs.emplace_back(value, assp);
            𐡄𐡍 (!nodep->nextp()) أرجع ResetAssignStatus::SINGLE;
        }
    }

    ساكن منطقي hasCanonicalNextStateDefaultBeforeCase(AstNode* stmtsp, AstNode* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* nextVscp) {
        AstNode* ثابت bodyp = skipLeadingIgnorableStmt(stmtsp);
        منطقي sawCanonicalDefault = خطأ;
        لكل (AstNode* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            𐡄𐡍 (nodep == targetp) أرجع sawCanonicalDefault;
            𐡄𐡍 (AstNodeAssign* ثابت assp = VN_CAST(nodep, NodeAssign)) {
                AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* ثابت rhsp = VN_CAST(assp->rhsp(), VarRef);
                𐡄𐡍 (!lhsp || lhsp->varScopep() != nextVscp) استمر;
                𐡄𐡍 (sawCanonicalDefault) {
                    ثابت string nextName = nextVscp->varp()->prettyNameQ();
                    ثابت string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + nextName
                                     + ") when the canonical " + nextName + " = " + stateName
                                     + " default is overwritten before the case statement");
                    أرجع خطأ;
                }
                𐡄𐡍 (!rhsp || rhsp->varScopep() != stateVscp) أرجع خطأ;
                sawCanonicalDefault = 𐡒𐡔𐡈;
            }
        }
    }

    ساكن منطقي ifStateConstAssign(AstNode* stmtp, AstVarScope* stateVscp,
                                   FsmStateValue& thenValue, FsmStateValue& elseValue) {
        AstIf* ثابت ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        𐡄𐡍 (!ifp || !ifp->elsesp()) أرجع خطأ;
        AstVarScope* thenVscp = nullptr;
        AstVarScope* elseVscp = nullptr;
        AstNode* ثابت thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        AstNode* ثابت elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        𐡄𐡍 (!thenNodep || !elseNodep) أرجع خطأ;
        𐡄𐡍 (!directConstStateAssignNode(thenNodep, thenVscp, thenValue)) أرجع خطأ;
        𐡄𐡍 (!directConstStateAssignNode(elseNodep, elseVscp, elseValue)) أرجع خطأ;
        𐡄𐡍 (thenVscp == stateVscp && elseVscp == stateVscp) أرجع 𐡒𐡔𐡈;
        𐡄𐡍 (thenVscp != elseVscp) أرجع خطأ;
        AstNode* ثابت followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = nullptr;
        AstVarScope* finalFromVscp = nullptr;
        AstNode* ثابت finalNodep = singleMeaningfulBranch(followp);
        𐡄𐡍 (!finalNodep) أرجع خطأ;
        𐡄𐡍 (!nodeStateVarAssign(finalNodep, finalStateVscp, finalFromVscp)) أرجع خطأ;
        𐡄𐡍 (finalStateVscp != stateVscp) أرجع خطأ;
        𐡄𐡍 (finalFromVscp != thenVscp) أرجع خطأ;
        أرجع 𐡒𐡔𐡈;
    }

    ساكن منطقي directStateCondConstAssign(AstNode* stmtp, AstVarScope* stateVscp,
                                           FsmStateValue& thenValue, FsmStateValue& elseValue) {
        AstNodeAssign* ثابت assp = directStateAssign(stmtp, stateVscp);
        𐡄𐡍 (!assp) أرجع خطأ;
        AstCond* ثابت condp = VN_CAST(assp->rhsp(), Cond);
        𐡄𐡍 (!condp) أرجع خطأ;
        أرجع constValueStatus(condp->thenp(), thenValue) == ConstValueStatus::OK
               && constValueStatus(condp->elsep(), elseValue) == ConstValueStatus::OK;
    }

    ساكن AstNode* caseItemSupportedArcNode(AstCaseItem* itemp, AstVarScope* stateVscp,
                                             منطقي inclCond) {
        𐡄𐡍 (itemp->isDefault()) {
            𐡄𐡍 (!inclCond) أرجع nullptr;
        }
        AstNodeAssign* ثابت assp = directStateAssign(itemp->stmtsp(), stateVscp);
        𐡄𐡍 (assp) {
            FsmStateValue toValue;
            𐡄𐡍 (constValueStatus(assp->rhsp(), toValue) == ConstValueStatus::OK) أرجع assp;
        }
        FsmStateValue thenValue;
        FsmStateValue elseValue;
        𐡄𐡍 (directStateCondConstAssign(itemp->stmtsp(), stateVscp, thenValue, elseValue)) {
            أرجع assp;
        }
        𐡄𐡍 (ifStateConstAssign(itemp->stmtsp(), stateVscp, thenValue, elseValue)) {
            أرجع singleMeaningfulBranch(itemp->stmtsp());
        }
        أرجع nullptr;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    ساكن AstNode* caseSupportedTransitionNode(AstCase* casep, AstVarScope* stateVscp,
                                                منطقي inclCond) {
        لكل (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            𐡄𐡍 (AstNode* ثابت nodep = caseItemSupportedArcNode(itemp, stateVscp, inclCond))
                أرجع nodep;
        }
        أرجع nullptr;
    }

    ساكن AstNode* caseItemSupportedArcNodeLike(AstNode* stmtsp, AstVarScope* stateVscp) {
        𐡄𐡍 (AstNodeAssign* ثابت assp = directStateAssign(stmtsp, stateVscp)) {
            FsmStateValue toValue;
            𐡄𐡍 (constValueStatus(assp->rhsp(), toValue) == ConstValueStatus::OK) أرجع assp;
            FsmStateValue thenValue;
            FsmStateValue elseValue;
            𐡄𐡍 (directStateCondConstAssign(stmtsp, stateVscp, thenValue, elseValue)) أرجع assp;
        }
        أرجع nullptr;
    }

    ساكن AstNode* ifChainSupportedTransitionNode(ثابت FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        لكل (size_t i = 0; i < chain.branches.size(); ++i) {
            ثابت FsmIfBranch& branch = chain.branches[i];
            AstNode* ثابت nodep = caseItemSupportedArcNodeLike(branch.stmtsp, stateVscp);
            𐡄𐡍 (!nodep) أرجع nullptr;
        }
        أرجع chain.branches.front().ifp;
    }

    // Prefer user labels in reports. Forced non-enum FSMs prepopulate synthetic
    // labels, so all emitted arcs should already have a known label here.
    ساكن string labelForValue(
        ثابت std::unordered_map<FsmStateValue, StateConstLabel, FsmStateValueHash>& labels,
        ثابت FsmStateValue& value) {
        أرجع labels.at(value).text;
    }

    // The extractor only models constant-valued state transitions, and by the
    // time detect runs those values have already been constant-folded.
    تعداد صنف ConstValueStatus : uint8_t { OK, NOT_CONST, XZ };

    ساكن ConstValueStatus constValueStatus(AstNodeExpr* exprp, FsmStateValue& value) {
        ثابت AstConst* ثابت constp = VN_CAST(exprp, Const);
        𐡄𐡍 (!constp) أرجع ConstValueStatus::NOT_CONST;
        ثابت V3Number& num = constp->num();
        𐡄𐡍 (num.isAnyXZ()) أرجع ConstValueStatus::XZ;
        value = FsmStateValue{num};
        أرجع ConstValueStatus::OK;
    }

    ساكن منطقي pureStateComparisonNoAlias(AstNodeExpr* exprp, FsmStateComparison& cmp) {
        AstEq* ثابت eqp = VN_CAST(exprp, Eq);
        𐡄𐡍 (!eqp) أرجع خطأ;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        AstNodeExpr* valuep = eqp->rhsp();
        𐡄𐡍 (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->baseFromp(𐡒𐡔𐡈), VarRef);
            𐡄𐡍 (!vrefp) { أرجع خطأ; }
            valuep = eqp->lhsp();
        }

        FsmStateValue value;
        𐡄𐡍 (constValueStatus(valuep, value) != ConstValueStatus::OK) أرجع خطأ;
        cmp.stateVscp = vrefp->varScopep();
        cmp.valuep = valuep;
        cmp.value = value;
        أرجع 𐡒𐡔𐡈;
    }

    ساكن منطقي pureStateComparison(AstNodeExpr* exprp, ثابت FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        𐡄𐡍 (pureStateComparisonNoAlias(exprp, cmp)) أرجع 𐡒𐡔𐡈;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        𐡄𐡍 (AstVarRef* ثابت vrefp = VN_CAST(exprp, VarRef)) {
            ثابت FsmAliasMap::const_iterator it = aliases.find(vrefp->varScopep());
            𐡄𐡍 (it == aliases.end()) أرجع خطأ;
            cmp = it->second;
            أرجع 𐡒𐡔𐡈;
        }
        أرجع خطأ;
    }

    ساكن منطقي supportedTopLevelGuard(AstNodeExpr* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        𐡄𐡍 (VN_IS(exprp, Or)) أرجع خطأ;
        𐡄𐡍 (VN_IS(exprp, RedAnd)) أرجع خطأ;
        𐡄𐡍 (VN_IS(exprp, RedOr)) أرجع خطأ;
        𐡄𐡍 (VN_IS(exprp, RedXor)) أرجع خطأ;
        أرجع 𐡒𐡔𐡈;
    }

    ساكن منطقي resolveIfPredicate(AstNodeExpr* exprp, ثابت FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, منطقي& hasGuard) {
        std::vector<AstNodeExpr*> terms;
        std::vector<AstNodeExpr*> pending;
        pending.push_back(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        طالما (!pending.empty()) {
            AstNodeExpr* ثابت nodep = pending.back();
            pending.pop_back();
            𐡄𐡍 (AstAnd* ثابت andp = VN_CAST(nodep, And)) {
                pending.push_back(andp->rhsp());
                pending.push_back(andp->lhsp());
            } وإلا {
                terms.push_back(nodep);
            }
        }

        منطقي sawComparison = خطأ;
        لكل (size_t i = 0; i < terms.size(); ++i) {
            AstNodeExpr* ثابت termp = terms[i];
            FsmStateComparison termCmp;
            𐡄𐡍 (pureStateComparison(termp, aliases, termCmp /*ref*/)) {
                𐡄𐡍 (sawComparison) أرجع خطأ;
                cmp = termCmp;
                sawComparison = 𐡒𐡔𐡈;
                استمر;
            }
            𐡄𐡍 (!supportedTopLevelGuard(termp)) أرجع خطأ;
            hasGuard = 𐡒𐡔𐡈;
        }
        أرجع sawComparison;
    }

    ساكن فراغ addAlias(FsmAliasMap& aliases, std::unordered_set<ثابت AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, ثابت FsmStateComparison& cmp) {
        𐡄𐡍 (ambiguous.find(aliasVscp) != ambiguous.end()) أرجع;
        ثابت FsmAliasMap::iterator it = aliases.find(aliasVscp);
        𐡄𐡍 (it == aliases.end()) {
            aliases.emplace(aliasVscp, cmp);
            أرجع;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        𐡄𐡍 (it->second.stateVscp == cmp.stateVscp && it->second.value == cmp.value) أرجع;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        أرجع;
    }

    ساكن فراغ collectAliasFromAssign(AstNodeAssign* assp, FsmAliasMap& aliases,
                                       std::unordered_set<ثابت AstVarScope*>& ambiguous) {
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp(), VarRef);
        𐡄𐡍 (!lhsp) أرجع;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        𐡄𐡍 (!pureStateComparisonNoAlias(assp->rhsp(), cmp /*ref*/)) أرجع;
        addAlias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(AstNode* stmtsp, AstNode* limitp) ثابت {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_set<ثابت AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        لكل (AstNode* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            𐡄𐡍 (AstNodeAssign* ثابت assp = VN_CAST(nodep, NodeAssign)) {
                collectAliasFromAssign(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        لكل (ثابت AstVarScope* ثابت vscp : ambiguous) aliases.erase(vscp);
        أرجع aliases;
    }

    ساكن منطقي collectIfChain(AstIf* ifp, ثابت FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_set<string> seenValues;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        طالما (𐡒𐡔𐡈) {
            FsmStateComparison cmp;
            منطقي hasGuard = خطأ;
            𐡄𐡍 (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) أرجع خطأ;
            𐡄𐡍 (chain.compareVscp && chain.compareVscp != cmp.stateVscp) أرجع خطأ;
            𐡄𐡍 (!seenValues.insert(cmp.value.key()).second) أرجع خطأ;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.push_back(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.value, hasGuard});

            AstNode* ثابت elseNodep
                = singleMeaningfulBranch(skipLeadingIgnorableStmt(curp->elsesp()));
            𐡄𐡍 (!elseNodep) اكسر;
            𐡄𐡍 (AstIf* ثابت elseIfp = VN_CAST(elseNodep, If)) {
                curp = elseIfp;
                استمر;
            }
            chain.defaultStmtsp = elseNodep;
            اكسر;
        }
        أرجع chain.branches.size() >= 2;
    }

    // Enum-backed FSMs should only use values that were interned as known states.
    // If a constant transition references some other encoding, warn and skip FSM
    // instrumentation for that edge rather than silently dropping it or turning
    // optional coverage into a hard compile failure.
    ساكن منطقي validateKnownStateValue(AstNode* nodep, ثابت FsmStateSpace& stateSpace,
                                        ثابت FsmStateValue& value, ثابت string& role) {
        𐡄𐡍 (stateSpace.labels.find(value) != stateSpace.labels.end()) أرجع 𐡒𐡔𐡈;
        𐡄𐡍 (stateSpace.enumBacked) {
            ثابت string enumRole = role == "source" ? "case item value" : "assigned value";
            nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on enum state variable "
                                        + stateSpace.stateVarp->prettyNameQ() + ": " + enumRole
                                        + " " + value.warnText()
                                        + " is not present in the declared enum");
            أرجع خطأ;
        }
        nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum state variable "
                                    + stateSpace.stateVarp->prettyNameQ() + ": " + role + " value "
                                    + value.warnText()
                                    + " is not present in the inferred state space");
        أرجع خطأ;
    }

    ساكن StateConstLabel stateLabelForConst(AstConst* constp) {
        ثابت string name = constp->origParamName();
        𐡄𐡍 (!name.empty()) أرجع StateConstLabel{AstNode::prettyName(name), 𐡒𐡔𐡈, 0};
        أرجع StateConstLabel{constp->name(), خطأ, 0};
    }

    ساكن فراغ updateStateLabel(FsmStateSpace& stateSpace, ثابت FsmStateValue& value,
                                 ثابت StateConstLabel& label) {
        stateSpace.states.at(stateSpace.labels.at(value).stateIndex).first = label.text;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    ساكن منطقي matchRegisterAlways(AstAlways* alwaysp, AstScope* scopep,
                                    FsmRegisterCandidate& cand) {
        𐡄𐡍 (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) أرجع خطأ;

        AstNode* ثابت stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        AstNode* ثابت nodep = singleMeaningfulStmt(stmtsp);
        𐡄𐡍 (!nodep) أرجع خطأ;

        AstVarScope* stateVscp = nullptr;
        AstVarScope* nextVscp = nullptr;
        𐡄𐡍 (AstIf* ثابت ifp = VN_CAST(nodep, If)) {
            𐡄𐡍 (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) أرجع خطأ;
            AstVarScope* resetStateVscp = nullptr;
            ثابت ResetAssignStatus resetStatus
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            ثابت منطقي emptyResetBranch = resetStatus == ResetAssignStatus::EMPTY;
            𐡄𐡍 (emptyResetBranch) {
                cand.resetArcs().clear();
            } وإلا 𐡄𐡍 (resetStatus == ResetAssignStatus::NONE) {
                cand.resetArcs().clear();
                FsmStateValue resetValue;
                AstNode* ثابت thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                AstNodeAssign* ثابت resetAssp
                    = directConstStateAssignNode(thenNodep, resetStateVscp, resetValue);
                𐡄𐡍 (!resetAssp) أرجع خطأ;
                cand.resetArcs().emplace_back(resetValue, resetAssp);
            } وإلا 𐡄𐡍 (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                cand.resetArcs().clear();
            }
            AstNode* ثابت elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            𐡄𐡍 (!nodeStateVarAssign(elseNodep, stateVscp, nextVscp)) أرجع خطأ;
            𐡄𐡍 (!emptyResetBranch && resetStateVscp != stateVscp) أرجع خطأ;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != nullptr);
        } وإلا {
            AstNodeExpr* resetCondp = nullptr;
            FsmStateValue resetValue;
            // Note the active low 'reset ? next : CONST' form is not accepted
            𐡄𐡍 (AstNodeAssign* ثابت assp
                = directCondStateVarAssign(nodep, stateVscp, nextVscp, resetCondp, resetValue)) {
                cand.resetArcs().emplace_back(resetValue, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != nullptr);
            } وإلا 𐡄𐡍 (!nodeStateVarAssign(nodep, stateVscp, nextVscp)) {
                أرجع خطأ;
            }
        }
        cand.scopep(scopep);
        cand.alwaysp(alwaysp);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(stateVscp);
        cand.nextVscp(nextVscp);
        cand.senses() = describeSenTree(alwaysp->sentreep());
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(alwaysp->fileline());
        أرجع 𐡒𐡔𐡈;
    }

    ساكن منطقي addValueToStateSpace(AstNode* nodep, FsmStateSpace& stateSpace,
                                     ثابت FsmStateValue& value, StateConstLabel label) {
        ثابت auto labelIt = stateSpace.labels.find(value);
        𐡄𐡍 (labelIt != stateSpace.labels.end()) {
            StateConstLabel& existingLabel = labelIt->second;
            𐡄𐡍 (existingLabel.text != label.text && existingLabel.fromParam && label.fromParam) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                        "state variable "
                                            + stateSpace.stateVarp->prettyNameQ()
                                            + " with multiple labels for the same value "
                                            + value.warnText() + ": " + existingLabel.text
                                            + " and " + label.text);
                أرجع خطأ;
            }
            𐡄𐡍 (!existingLabel.fromParam && label.fromParam) {
                existingLabel.text = label.text;
                existingLabel.fromParam = label.fromParam;
                updateStateLabel(stateSpace, value, label);
            }
        } وإلا {
            StateConstLabel storedLabel = label;
            storedLabel.stateIndex = stateSpace.states.size();
            stateSpace.states.emplace_back(label.text, value);
            stateSpace.labels.emplace(value, storedLabel);
        }
        أرجع 𐡒𐡔𐡈;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    ساكن منطقي addExprToStateSpace(AstNodeExpr* valuep, FsmStateSpace& stateSpace) {
        FsmStateValue value;
        ثابت ConstValueStatus status = constValueStatus(valuep, value);
        𐡄𐡍 (status != ConstValueStatus::OK) {
            𐡄𐡍 (status == ConstValueStatus::XZ) {
                valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                         "state variable "
                                             + stateSpace.stateVarp->prettyNameQ()
                                             + " with X/Z state encoding values");
            }
            أرجع خطأ;
        }
        AstConst* ثابت constp = VN_AS(valuep, Const);
        أرجع addValueToStateSpace(valuep, stateSpace, value, stateLabelForConst(constp));
    }

    ساكن منطقي addOptionalTargetExprToStateSpace(AstNodeExpr* valuep, FsmStateSpace& stateSpace) {
        FsmStateValue value;
        ثابت ConstValueStatus status = constValueStatus(valuep, value);
        𐡄𐡍 (status != ConstValueStatus::OK) {
            valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                     "state variable "
                                         + stateSpace.stateVarp->prettyNameQ()
                                         + " with non-constant target state values");
            أرجع خطأ;
        }
        AstConst* ثابت constp = VN_AS(valuep, Const);
        أرجع addValueToStateSpace(valuep, stateSpace, value, stateLabelForConst(constp));
    }

    ساكن فراغ addResetTargetsToStateSpace(ثابت std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        لكل (ثابت FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.toValue().ascii(), خطأ, 0};
            𐡄𐡍 (AstConst* ثابت constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                addValueToStateSpace(resetArc.nodep(), stateSpace, resetArc.toValue(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    ساكن منطقي collectDeclaredStateSpace(AstNode* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, منطقي& needsSourceValues) {
        AstVar* ثابت stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        𐡄𐡍 (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        ثابت منطقي forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        𐡄𐡍 (enump) {
            stateSpace.enumBacked = 𐡒𐡔𐡈;
            لكل (AstEnumItem* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
                ثابت FsmStateValue value{constp->num()};
                ثابت size_t stateIndex = stateSpace.states.size();
                stateSpace.states.emplace_back(itemp->name(), value);
                stateSpace.labels.emplace(value,
                                          StateConstLabel{itemp->name(), خطأ, stateIndex});
            }
            أرجع stateSpace.states.size() >= 2;
        }

        𐡄𐡍 (forced) {
            needsSourceValues = 𐡒𐡔𐡈;
            أرجع 𐡒𐡔𐡈;
        }

        needsSourceValues = 𐡒𐡔𐡈;
        أرجع 𐡒𐡔𐡈;
    }

    template <typename T_ValuepVisitor>
    ساكن منطقي collectStateSpaceFromValues(AstNode* warnNodep, AstVarScope* stateVscp,
                                            ثابت std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            ثابت T_ValuepVisitor& visitValueps) {
        منطقي needsSourceValues = خطأ;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        𐡄𐡍 (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needsSourceValues)) {
            أرجع خطأ;
        }
        𐡄𐡍 (!needsSourceValues) أرجع 𐡒𐡔𐡈;
        addResetTargetsToStateSpace(resetArcs, stateSpace);
        𐡄𐡍 (!visitValueps(
                [&](AstNodeExpr* valuep) { أرجع addExprToStateSpace(valuep, stateSpace); })) {
            أرجع خطأ;
        }
        أرجع stateSpace.states.size() >= 2;
    }

    ساكن منطقي collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* assignVscp,
                                  ثابت std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        أرجع collectStateSpaceFromValues(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, assignVscp, &stateSpace](ثابت auto& visitValuep) {
                لكل (AstCaseItem* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), CaseItem)) {
                    𐡄𐡍 (!itemp->isDefault()) {
                        لكل (AstNodeExpr* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), NodeExpr)) {
                            𐡄𐡍 (!visitValuep(condp)) أرجع خطأ;
                        }
                    }
                    𐡄𐡍 (AstNodeAssign* ثابت assp
                        = directStateAssign(itemp->stmtsp(), assignVscp)) {
                        FsmStateValue thenValue;
                        FsmStateValue elseValue;
                        AstCond* ثابت condp = VN_CAST(assp->rhsp(), Cond);
                        𐡄𐡍 (condp
                            && directStateCondConstAssign(itemp->stmtsp(), assignVscp, thenValue,
                                                          elseValue)) {
                            𐡄𐡍 (!visitValuep(condp->thenp())) أرجع خطأ;
                            𐡄𐡍 (!visitValuep(condp->elsep())) أرجع خطأ;
                        } وإلا 𐡄𐡍 (!addOptionalTargetExprToStateSpace(assp->rhsp(), stateSpace)) {
                            أرجع خطأ;
                        }
                    }
                }
                أرجع 𐡒𐡔𐡈;
            });
    }

    ساكن منطقي collectStateSpace(ثابت FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* assignVscp,
                                  ثابت std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        أرجع collectStateSpaceFromValues(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, assignVscp, &stateSpace](ثابت auto& visitValuep) {
                لكل (ثابت FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(visitValuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    AstNodeAssign* ثابت assp = directStateAssign(branch.stmtsp, assignVscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    FsmStateValue thenValue;
                    FsmStateValue elseValue;
                    AstCond* ثابت condp = VN_CAST(assp->rhsp(), Cond);
                    𐡄𐡍 (condp) {
                        UASSERT_OBJ(directStateCondConstAssign(branch.stmtsp, assignVscp,
                                                               thenValue, elseValue),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        𐡄𐡍 (!visitValuep(condp->thenp())) أرجع خطأ;
                        𐡄𐡍 (!visitValuep(condp->elsep())) أرجع خطأ;
                    } وإلا 𐡄𐡍 (!addOptionalTargetExprToStateSpace(assp->rhsp(), stateSpace)) {
                        أرجع خطأ;
                    }
                }
                أرجع 𐡒𐡔𐡈;
            });
    }

    // Extract supported case-item transitions in one place so the conservative
    // policy for direct and ternary forms stays consistent. The false exits in
    // this helper are deliberate subset boundaries: they document shapes we do
    // not yet model in this PR and that future FSM-detection work may widen.
    ساكن منطقي emitCaseItemArcs(FsmGraph& graph, AstCaseItem* itemp, AstVarScope* stateVscp,
                                 ثابت FsmStateSpace& stateSpace, منطقي inclCond) {
        std::vector<std::pair<string, FsmStateValue>> froms;
        𐡄𐡍 (itemp->isDefault()) {
            𐡄𐡍 (!inclCond) أرجع خطأ;
            froms.emplace_back("default", FsmStateValue{});
        } وإلا {
            لكل (AstNodeExpr* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), NodeExpr)) {
                FsmStateValue value;
                𐡄𐡍 (constValueStatus(condp, value) != ConstValueStatus::OK) استمر;
                𐡄𐡍 (!validateKnownStateValue(condp, stateSpace, value, "source")) أرجع 𐡒𐡔𐡈;
                froms.emplace_back(labelForValue(stateSpace.labels, value), value);
            }
            𐡄𐡍 (froms.empty()) أرجع خطأ;
        }

        𐡄𐡍 (AstNodeAssign* ثابت assp = directStateAssign(itemp->stmtsp(), stateVscp)) {
            FsmStateValue toValue;
            ثابت ConstValueStatus status = constValueStatus(assp->rhsp(), toValue);
            𐡄𐡍 (status == ConstValueStatus::OK) {
                𐡄𐡍 (!validateKnownStateValue(assp, stateSpace, toValue, "target")) أرجع 𐡒𐡔𐡈;
                لكل (ثابت std::pair<string, FsmStateValue>& from : froms) {
                    graph.addArc(from.second, toValue, خطأ, خطأ, itemp->isDefault(),
                                 assp->fileline());
                }
                أرجع 𐡒𐡔𐡈;
            }
        }

        FsmStateValue thenValue;
        FsmStateValue elseValue;
        𐡄𐡍 (directStateCondConstAssign(itemp->stmtsp(), stateVscp, thenValue, elseValue)
            || ifStateConstAssign(itemp->stmtsp(), stateVscp, thenValue, elseValue)) {
            𐡄𐡍 (!validateKnownStateValue(itemp->stmtsp(), stateSpace, thenValue, "target"))
                أرجع 𐡒𐡔𐡈;
            𐡄𐡍 (!validateKnownStateValue(itemp->stmtsp(), stateSpace, elseValue, "target"))
                أرجع 𐡒𐡔𐡈;
            لكل (ثابت FsmStateValue& branchValue : {thenValue, elseValue}) {
                لكل (ثابت std::pair<string, FsmStateValue>& from : froms) {
                    graph.addArc(from.second, branchValue, خطأ, 𐡒𐡔𐡈, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            أرجع 𐡒𐡔𐡈;
        }

        أرجع خطأ;
    }

    ساكن فراغ emitStmtArcsFrom(FsmGraph& graph, AstNode* stmtsp, AstVarScope* stateVscp,
                                 ثابت FsmStateSpace& stateSpace, FsmStateValue fromValue,
                                 منطقي isDefault, منطقي forceCond) {
        AstNodeAssign* ثابت assp = directStateAssign(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        FsmStateValue toValue;
        ثابت ConstValueStatus status = constValueStatus(assp->rhsp(), toValue);
        𐡄𐡍 (status == ConstValueStatus::OK) {
            𐡄𐡍 (!validateKnownStateValue(assp, stateSpace, toValue, "target")) أرجع;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            graph.addArc(fromValue, toValue, خطأ, forceCond, isDefault, assp->fileline());
            أرجع;
        }

        FsmStateValue thenValue;
        FsmStateValue elseValue;
        ثابت منطقي condAssign
            = directStateCondConstAssign(stmtsp, stateVscp, thenValue, elseValue);
        UASSERT_OBJ(condAssign, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        𐡄𐡍 (!validateKnownStateValue(stmtsp, stateSpace, thenValue, "target")) أرجع;
        𐡄𐡍 (!validateKnownStateValue(stmtsp, stateSpace, elseValue, "target")) أرجع;
        لكل (ثابت FsmStateValue& branchValue : {thenValue, elseValue}) {
            graph.addArc(fromValue, branchValue, خطأ, 𐡒𐡔𐡈, isDefault, stmtsp->fileline());
        }
    }

    ساكن فراغ emitIfChainArcs(FsmGraph& graph, ثابت FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, ثابت FsmStateSpace& stateSpace) {
        لكل (size_t i = 0; i < chain.branches.size(); ++i) {
            ثابت FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            𐡄𐡍 (!validateKnownStateValue(branch.ifp, stateSpace, branch.fromValue, "source"))
                أرجع;
            emitStmtArcsFrom(graph, branch.stmtsp, stateVscp, stateSpace, branch.fromValue, خطأ,
                             branch.hasTopGuard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    ساكن فراغ addResetArcs(FsmGraph& graph, ثابت std::vector<FsmResetArcDesc>& resetArcs,
                             ثابت FsmStateSpace& stateSpace) {
        لكل (ثابت FsmResetArcDesc& resetArc : resetArcs) {
            𐡄𐡍 (!validateKnownStateValue(resetArc.nodep(), stateSpace, resetArc.toValue(),
                                         "target"))
                استمر;
            graph.addArc(FsmStateValue{}, resetArc.toValue(), 𐡒𐡔𐡈, خطأ, خطأ,
                         resetArc.nodep()->fileline());
        }
    }

    // Turn one candidate case statement into the graph representation that the
    // later lowering phase will consume directly, while reviewers can still
    // inspect the extracted machine via DOT dumps.
    فراغ processCase(AstCase* casep, AstVarScope* assignVscp, ثابت FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, casep, "FSM case processing requires a non-null assignment var");
        AstVarScope* ثابت stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        𐡄𐡍 (!collectStateSpace(casep, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) أرجع;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        𐡄𐡍 (!entry.graphp) {
            entry.graphp.reset(جديد FsmGraph{});
            entry.graphp->scopep(reg.scopep());
            entry.graphp->stateAlwaysp(reg.alwaysp());
            entry.graphp->stateVarName(stateVscp->prettyName());
            entry.graphp->stateVarInternalName(stateVscp->varp()->name());
            entry.graphp->stateVarScopep(stateVscp);
            entry.graphp->sampleVarScopep(reg.sampleVscp());
            entry.graphp->senses() = reg.senses();
            entry.graphp->resetCond() = reg.resetCond();
            entry.graphp->hasResetCond(reg.hasResetCond());
            entry.graphp->inclCond(reg.inclCond());
            entry.graphp->fileline(casep->fileline());
            لكل (ثابت std::pair<string, FsmStateValue>& state : stateSpace.states) {
                entry.graphp->addStateVertex(state.first, state.second);
            }
            addResetArcs(*entry.graphp, reg.resetArcs(), stateSpace);
        }
        لكل (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            emitCaseItemArcs(*entry.graphp, itemp, assignVscp, stateSpace,
                             entry.graphp->inclCond());
        }
    }

    فراغ processIfChain(ثابت FsmIfChainCandidate& chain, AstVarScope* assignVscp,
                        ثابت FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, chain.ifp,
                    "FSM if-chain processing requires a non-null assignment var");
        AstVarScope* ثابت stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        𐡄𐡍 (!collectStateSpace(chain, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) أرجع;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        // Case candidates keep ownership of existing graphs; reaching this path
        // means the if-chain is the only supported dispatch for this FSM.
        UASSERT_OBJ(!entry.graphp, chain.ifp, "FSM if-chain graph should not already exist");
        entry.graphp.reset(جديد FsmGraph{});
        entry.graphp->scopep(reg.scopep());
        entry.graphp->stateAlwaysp(reg.alwaysp());
        entry.graphp->stateVarName(stateVscp->prettyName());
        entry.graphp->stateVarInternalName(stateVscp->varp()->name());
        entry.graphp->stateVarScopep(stateVscp);
        entry.graphp->sampleVarScopep(reg.sampleVscp());
        entry.graphp->senses() = reg.senses();
        entry.graphp->resetCond() = reg.resetCond();
        entry.graphp->hasResetCond(reg.hasResetCond());
        entry.graphp->inclCond(reg.inclCond());
        entry.graphp->fileline(chain.ifp->fileline());
        لكل (ثابت std::pair<string, FsmStateValue>& state : stateSpace.states) {
            entry.graphp->addStateVertex(state.first, state.second);
        }
        addResetArcs(*entry.graphp, reg.resetArcs(), stateSpace);
        emitIfChainArcs(*entry.graphp, chain, assignVscp, stateSpace);
    }

    // Find the first supported FSM candidate in a clocked always block, warn on
    // additional candidates, and attach reset arcs when present. Candidate
    // filtering stays narrow on purpose: we prefer to skip ambiguous shapes now
    // and expand detection in a later PR rather than over-infer coverage from
    // forms we do not yet model confidently.
    فراغ processOneBlockAlways(ثابت FsmComboAlways& oneBlock) {
        ثابت RegisterAlwaysAnalyzer analyzer{oneBlock.scopep()};
        AstAlways* ثابت alwaysp = oneBlock.alwaysp();
        𐡄𐡍 (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) أرجع;
        ثابت std::vector<std::pair<AstCase*, AstNodeExpr*>> candidates
            = analyzer.oneBlockCandidates(alwaysp);

        FsmCaseCandidate firstCand;
        لكل (ثابت std::pair<AstCase*, AstNodeExpr*>& cand : candidates) {
            AstVarRef* ثابت selp = VN_CAST(cand.first->exprp(), VarRef);
            AstVarScope* ثابت vscp = selp ? selp->varScopep() : nullptr;
            𐡄𐡍 (!vscp) استمر;
            𐡄𐡍 (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processCase(cand.first, vscp, reg);
            } وإلا 𐡄𐡍 (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed case statements found in "
                                   "the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } وإلا {
                cand.first->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported case "
                                   "statements found in the same always block. Only the first "
                                   "candidate will be instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            }
        }

        𐡄𐡍 (firstCand.stateVscp) أرجع;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        ثابت std::vector<std::pair<AstIf*, AstNodeExpr*>> ifCandidates
            = analyzer.oneBlockIfCandidates(alwaysp);
        لكل (ثابت std::pair<AstIf*, AstNodeExpr*>& cand : ifCandidates) {
            ثابت FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.first);
            FsmIfChainCandidate chain;
            𐡄𐡍 (!collectIfChain(cand.first, aliases, chain)) استمر;
            AstVarScope* ثابت vscp = chain.compareVscp;
            𐡄𐡍 (!ifChainSupportedTransitionNode(chain, vscp)) استمر;
            𐡄𐡍 (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processIfChain(chain, vscp, reg);
            } وإلا 𐡄𐡍 (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed transition candidates found "
                                   "in the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } وإلا {
                cand.first->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported "
                                   "transition candidates found in the same always block. Only "
                                   "the first candidate will be instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            }
        }
    }

    // Phase 1 two-process pairing scans combinational always blocks only after
    // all strict register candidates have been collected, so source order does
    // not matter.
    ساكن فراغ warnComboSameAlways(AstNode* warnNodep, ثابت FsmCaseCandidate& firstCand) {
        warnNodep->v3warn(FSMMULTI,
                          "FSM coverage: multiple supported transition candidates found in "
                          "the same combinational always block. Only the first candidate "
                          "will be instrumented."
                              << candidateConflictContext(warnNodep, firstCand));
    }

    فراغ processComboAlways(ثابت FsmComboAlways& combo) {
        ثابت ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        AstNode* ثابت stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate firstCand;
        لكل (AstNode* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ثابت casep = VN_CAST(nodep, Case);
            𐡄𐡍 (!casep) استمر;
            ثابت ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchCase(stmtsp, casep);
            ثابت FsmRegisterCandidate* ثابت matchedp = match.matchedp;
            AstNode* ثابت matchedWarnNodep = match.warnNodep;
            𐡄𐡍 (!matchedp) استمر;
            𐡄𐡍 (!firstCand.stateVscp) {
                ثابت auto insertPair = m_comboPaired.emplace(
                    matchedp->stateVscp(),
                    FsmCaseCandidate{matchedWarnNodep,
                                     const_cast<AstVarScope*>(matchedp->stateVscp())});
                𐡄𐡍 (!insertPair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  insertPair.first->second));
                    استمر;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processCase(casep, matchedp->nextVscp(), *matchedp);
                استمر;
            }
            𐡄𐡍 (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                استمر;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported case statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, firstCand));
        }
        𐡄𐡍 (firstCand.stateVscp) أرجع;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        لكل (AstNode* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* ثابت ifp = VN_CAST(nodep, If);
            𐡄𐡍 (!ifp) استمر;
            FsmIfChainCandidate chain;
            ثابت FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            𐡄𐡍 (!collectIfChain(ifp, aliases, chain)) استمر;
            ثابت ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchIfChain(stmtsp, chain);
            ثابت FsmRegisterCandidate* ثابت matchedp = match.matchedp;
            AstNode* ثابت matchedWarnNodep = match.warnNodep;
            𐡄𐡍 (!matchedp) استمر;
            𐡄𐡍 (!firstCand.stateVscp) {
                ثابت std::pair<std::unordered_map<ثابت AstVarScope*, FsmCaseCandidate>::iterator,
                                منطقي>
                    insertPair = m_comboPaired.emplace(
                        matchedp->stateVscp(),
                        FsmCaseCandidate{matchedWarnNodep,
                                         const_cast<AstVarScope*>(matchedp->stateVscp())});
                𐡄𐡍 (!insertPair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  insertPair.first->second));
                    استمر;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processIfChain(chain, matchedp->nextVscp(), *matchedp);
                استمر;
            }
            𐡄𐡍 (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                استمر;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported if-chain statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, firstCand));
        }
    }

    // Track the current scope so each detected FSM records the module/scope
    // where instrumentation must later be inserted.
    فراغ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    // Collect processes first, then analyze FSM candidates once all alias and
    // register information is available.
    فراغ visit(AstAlways* nodep) override {
        𐡄𐡍 (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            أرجع;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_oneBlockAlwayss.emplace_back(m_scopep, nodep);
        ثابت RegisterAlwaysAnalyzer analyzer{m_scopep};
        FsmRegisterCandidate reg;
        𐡄𐡍 (analyzer.matchRegisterCandidate(nodep, reg)) {
            AstVarScope* ثابت stateVscp = reg.stateVscp();
            ثابت منطقي found
                = std::any_of(m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                              [stateVscp](ثابت FsmRegisterCandidate& existing) {
                                  أرجع existing.stateVscp() == stateVscp;
                              });
            𐡄𐡍 (!found) { m_registerCandidates.emplace_back(reg); }
        }
        𐡄𐡍 (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            m_comboAlwayss.emplace_back(m_scopep, nodep);
        } وإلا 𐡄𐡍 (nodep->keyword() == VAlwaysKwd::ALWAYS) {
            𐡄𐡍 (!nodep->sentreep() || isPlainComboSentree(nodep->sentreep())) {
                m_comboAlwayss.emplace_back(m_scopep, nodep);
            } وإلا {
                m_nonComboAlwayss.emplace_back(m_scopep, nodep);
            }
        }
    }

    فراغ visit(AstAssignW* nodep) override {
        // Continuous aliases are unordered hardware connections, so source
        // order should not affect whether an if-chain FSM is recognized.
        collectAliasFromAssign(nodep, m_stateAliases, m_ambiguousStateAliases);
        collectCellPortAlias(nodep);
        iterateChildren(nodep);
    }

    فراغ visit(AstCell* nodep) override {
        // Cells are matched after the full traversal because linkdot lowers
        // uninlined port connections into sibling continuous assignments.
        𐡄𐡍 (m_scopep && fsmRegisterWrapperDesc(nodep)) addWrapperCell(m_scopep, nodep);
        iterateChildren(nodep);
    }

    // Continue the walk through the rest of the design hierarchy.
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        لكل (ثابت std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            FsmRegisterCandidate reg;
            𐡄𐡍 (matchFsmWrapperCell(wrapperCell.first, wrapperCell.second, reg)) {
                m_registerCandidates.emplace_back(reg);
            }
        }
        لكل (ثابت FsmComboAlways& oneBlock : m_oneBlockAlwayss) processOneBlockAlways(oneBlock);
        لكل (ثابت FsmComboAlways& combo : m_comboAlwayss) processComboAlways(combo);
        لكل (ثابت FsmComboAlways& combo : m_nonComboAlwayss) warnUnsupportedComboAlways(combo);
    }
};

// Builder of statements that will handle incrementation of proper AstNodeCoverDecls - which will
// be also created here
صنف FsmCoverIncBuilder final {
    ثابت FsmGraph& m_graph;  // Fsm graph
    std::vector<ثابت FsmStateVertex*> m_states;  // States vertexes inside fsm graph
    std::unordered_set<uint64_t>
        m_undefinedStates;  // States that have no enum value but are possible to represent with a
                            // variable of given width.
                            // This variable keeps uint64_t instead of V3Numebr because it is safe
                            // to assume that nobody has over 16M TB of RAM
    ثابت FsmVertex* m_defaultVertexp = nullptr;  // Default vertex
    ثابت FsmVertex* m_resetVertexp = nullptr;  // Reset vertex
    size_t m_edgesCount = 0;  // Total count of edges
    AstVarScope* ثابت m_prevVscp;  // Previous state of fsm
    FileLine* ثابت m_flp;  // Fileline pointer to use across the builder
    ثابت std::string m_modPrettyName;  // Pretty fsm mode name
    AstNodeStmt* m_coverIncp = nullptr;  // Increments proper CoverDecl after each change
    AstNodeStmt* m_initialCoverIncp = nullptr;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = nullptr;  // List of CoverDecls
    منطقي m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(ثابت uint64_t value) {
        V3Number num{static_cast<AstNode*>(nullptr), m_prevVscp->width()};
        num.setQuad(value);
        num.isSigned(m_prevVscp->dtypep()->isSigned());
        أرجع جديد AstConst{m_flp, num};
    }

    std::string quadToBitStirngLabel(ثابت uint64_t val) {
        صحيح width = m_prevVscp->width();
        UASSERT(width < 64, "We should not expand values that wide");
        std::string result = std::to_string(width) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        طالما (--width >= 0) result.push_back("01"[(val >> width) & 1]);
        أرجع result;
    }

    فراغ initExistingStates() {
        لكل (ثابت V3GraphVertex& vtx : m_graph.vertices()) {
            ثابت FsmVertex* ثابت vertexp = vtx.as<FsmVertex>();
            m_edgesCount += vertexp->inEdges().size();
            𐡄𐡍 (ثابت FsmStateVertex* ثابت statep = vertexp->cast<FsmStateVertex>()) {
                m_states.push_back(statep);
            } وإلا 𐡄𐡍 (vertexp->isDefaultAny()) {
                UASSERT_OBJ(!std::exchange(m_defaultVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two defaults?");
            } وإلا /* if (vertexp->isResetAny()) */ {
                UASSERT_OBJ(!std::exchange(m_resetVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two resets?");
            }
        }
        ثابت صحيح width = m_graph.sampleVarScopep()->width();
        m_isComplete = width < 64 && m_states.size() == (static_cast<size_t>(1) << width);
    }

    فراغ initUndefinedStates() {
        UASSERT_OBJ(m_graph.sampleVarScopep()->width() < 64, m_graph.sampleVarScopep(),
                    "This FSM is too wide to expand - this should be caught earlier");
        ثابت uint64_t combinationsCount = static_cast<size_t>(1)
                                           << m_graph.sampleVarScopep()->width();
        // Rolling trick allows to cut peak memory usage
        منطقي undefRolled = خطأ;
        auto rollUndefs = [هذا, combinationsCount] {
            لكل (uint64_t i = 0; i < combinationsCount; ++i) {
                ثابت auto insert = m_undefinedStates.insert(i);
                𐡄𐡍 (!insert.second) m_undefinedStates.erase(insert.first);
            }
        };
        لكل (ثابت FsmStateVertex* ثابت statep : m_states) {
            ثابت V3Number& num = statep->value().num();
            ثابت uint64_t value = num.toUQuad();
            𐡄𐡍 (!m_isComplete) {
                𐡄𐡍 (undefRolled) {
                    UASSERT_OBJ(m_undefinedStates.erase(value) == 1, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                } وإلا {
                    UASSERT_OBJ(m_undefinedStates.insert(value).second, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                    𐡄𐡍 (m_undefinedStates.size() * 2 >= combinationsCount) {
                        undefRolled = 𐡒𐡔𐡈;
                        rollUndefs();
                    }
                }
            }
        }
        𐡄𐡍 (!(m_isComplete || undefRolled)) rollUndefs();
    }

    AstNodeExpr* buildResetCond() ثابت {
        AstNodeExpr* ثابت refp
            = جديد AstVarRef{m_flp, m_graph.resetCond().varScopep, VAccess::READ};
        أرجع m_graph.resetCond().activeLow
                   ? static_cast<AstNodeExpr*>(جديد AstLogNot{m_flp, refp})
                   : refp;
    }

    size_t getArcCountWithExpansion(ثابت VFsmExpandType expand) ثابت {
        size_t arcCountWithExpansion = ~0ULL;
        بدل (expand) {
        حالة VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_graph.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            اكسر;
        }
        حالة VFsmExpandType::AUTO: {
            arcCountWithExpansion = m_edgesCount;
            𐡄𐡍 (m_defaultVertexp) {
                UASSERT_OBJ(m_states.size() >= m_defaultVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arcCountWithExpansion -= m_defaultVertexp->outEdges().size();
                لكل (ثابت FsmStateVertex* ثابت statep : m_states) {
                    arcCountWithExpansion += static_cast<size_t>(statep->outEdges().empty())
                                             * m_defaultVertexp->outEdges().size();
                }
            }
            𐡄𐡍 (m_resetVertexp) {
                UASSERT_OBJ(m_states.size() >= m_resetVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arcCountWithExpansion += m_states.size() - m_resetVertexp->outEdges().size();
            }
            اكسر;
        }
        حالة VFsmExpandType::AUTO_EXPAND: {
            arcCountWithExpansion = m_states.size() * m_states.size() - m_states.size();
            اكسر;
        }
        حالة VFsmExpandType::FULL: {
            ثابت صحيح width = m_graph.sampleVarScopep()->width();
            // 32 is derived from:
            // tranitionsCount <= std::numeric_limits<uint64_t>::max()
            // (2^width)^2 - (2^width) <= 2^64 - 1
            // (2^width)^2 - (2^width) < 2^64 since we operate on integral values we can trade 1
            // log_2((2^width)^2 - (2^width)) < 64
            // log_2((2^width)^2) < 64 - `(2^width)` is negigible so, it may be dropped
            // 2width < 64
            𐡄𐡍 (width < 32) {
                ثابت uint64_t combinationsCount = static_cast<size_t>(1) << width;
                arcCountWithExpansion = combinationsCount * combinationsCount - combinationsCount;
            }
            اكسر;
        }
        }
        أرجع arcCountWithExpansion;
    }

    AstCoverOtherDecl* buildStateCoverDecl(ثابت std::string& label,
                                           ثابت std::string& fsmTag = "") {
        AstCoverOtherDecl* ثابت declp
            = جديد AstCoverOtherDecl{m_flp,
                                    "v_fsm_state/" + m_modPrettyName,
                                    m_graph.stateVarName() + "::" + label,
                                    "",
                                    0,
                                    m_graph.stateVarName(),
                                    "",
                                    label,
                                    fsmTag};
        // We run after scoping, so this is declared for one instance only
        declp->perInstance(𐡒𐡔𐡈);
        m_declsp = AstNode::addNext(m_declsp, declp);
        أرجع declp;
    }

    AstNodeStmt* buildCoverIncStmtp(ثابت std::string& fromStateLabel,
                                    ثابت std::string& toStateLabel,
                                    ثابت std::string& baseLabel = "") {
        // Whole builder is designed in such a way that this function won't be called twice with
        // the same arguments - but if it will ever change in future, `AstCoverOtherDecl` shall be
        // cached for particular args set
        AstCoverOtherDecl* ثابت declp = جديد AstCoverOtherDecl{
            m_flp,
            "v_fsm_arc/" + m_modPrettyName,
            m_graph.stateVarName() + "::" + fromStateLabel + "->" + toStateLabel,
            "",
            0,
            m_graph.stateVarName(),
            fromStateLabel,
            toStateLabel,
            baseLabel};
        // We run after scoping, so this is declared for one instance only
        declp->perInstance(𐡒𐡔𐡈);
        m_declsp = AstNode::addNext(m_declsp, declp);
        أرجع جديد AstCoverInc{m_flp, declp};
    }
    AstNodeStmt* buildExpandedTransitionHandler(ثابت std::string& toStateLabel,
                                                ثابت منطقي considerUndefs,
                                                ثابت منطقي expandOnlyWithTerminalVerticies,
                                                ثابت FsmStateVertex* ثابت toStatep = nullptr) {
        AstCase* ثابت arcCasep = جديد AstCase{
            m_flp, VCaseType::CT_CASE, جديد AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        std::unordered_set<ثابت FsmStateVertex*> handledSoureces;
        𐡄𐡍 (toStatep) {
            لكل (ثابت V3GraphEdge& edge : toStatep->inEdges()) {
                𐡄𐡍 (ثابت FsmStateVertex* ثابت fromp = edge.fromp()->cast<FsmStateVertex>()) {
                    𐡄𐡍 (!handledSoureces.emplace(fromp).second) استمر;
                    arcCasep->addItemsp(جديد AstCaseItem{
                        m_flp, makeConst(fromp->value().num().toUQuad()),
                        (fromp != toStatep) ? buildCoverIncStmtp(fromp->label(), toStateLabel)
                                            : nullptr});
                }
            }
        }
        لكل (ثابت FsmStateVertex* ثابت fromp : m_states) {
            𐡄𐡍 (handledSoureces.find(fromp) != handledSoureces.end()) استمر;
            arcCasep->addItemsp(جديد AstCaseItem{
                m_flp, makeConst(fromp->value().num().toUQuad()),
                (fromp != toStatep
                 && (!expandOnlyWithTerminalVerticies || fromp->outEdges().empty()))
                    ? buildCoverIncStmtp(fromp->label(), toStateLabel, "artificial")
                    : nullptr});
        }
        𐡄𐡍 (!m_isComplete) {
            𐡄𐡍 (considerUndefs) {
                لكل (ثابت uint64_t from : m_undefinedStates) {
                    ثابت std::string& label = quadToBitStirngLabel(from);
                    arcCasep->addItemsp(
                        جديد AstCaseItem{m_flp, makeConst(from),
                                        (label != toStateLabel)
                                            ? buildCoverIncStmtp(label, toStateLabel, "artificial")
                                            : nullptr});
                }
            } وإلا {
                arcCasep->addItemsp(جديد AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        أرجع arcCasep;
    }
    AstNodeStmt* buildUnxpandedTransitionHandler(ثابت FsmStateVertex* ثابت toStatep) {
        AstCase* ثابت arcCasep = جديد AstCase{
            m_flp, VCaseType::CT_CASE, جديد AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        لكل (ثابت V3GraphEdge& edge : toStatep->inEdges()) {
            ثابت FsmArcEdge* ثابت arcp = edge.as<FsmArcEdge>();
            UASSERT_OBJ(!arcp->isReset(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            ثابت FsmStateVertex* ثابت fromp = arcp->fromp()->as<FsmStateVertex>();
            𐡄𐡍 (fromp != toStatep) {
                arcCasep->addItemsp(
                    جديد AstCaseItem{m_flp, makeConst(fromp->value().num().toUQuad()),
                                    buildCoverIncStmtp(fromp->label(), toStatep->label())});
            }
        }
        arcCasep->addItemsp(جديد AstCaseItem{m_flp, nullptr, nullptr});
        أرجع arcCasep;
    }

عام:
    FsmCoverIncBuilder(ثابت FsmGraph& graph, AstVarScope* ثابت prevVscp)
        : m_graph{graph}
        , m_prevVscp{prevVscp}
        , m_flp{graph.fileline()}
        , m_modPrettyName{graph.scopep()->modp()->prettyName()} {
        initExistingStates();
        AstVarScope* ثابت sampleVscp = graph.sampleVarScopep();
        ثابت VFsmExpandType expand
            = graph.stateVarScopep()->varp()->attrFsmStateExpand() != VFsmExpandType::DEFAULT
                  ? graph.stateVarScopep()->varp()->attrFsmStateExpand()
                  : v3Global.opt.coverageFsmExpand();

        {
            ثابت size_t arcCountWithExpansion = getArcCountWithExpansion(expand);
            𐡄𐡍 (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arcCountWithExpansion == 0ULL
                            || arcCountWithExpansion
                                   > static_cast<size_t>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arcCountWithExpansion);
                أرجع;
            }
        }

        AstCase* ثابت arcCasep
            = جديد AstCase{m_flp, VCaseType::CT_CASE,
                          جديد AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};
        AstCase* ثابت initialStateCasep
            = جديد AstCase{m_flp, VCaseType::CT_CASE,
                          جديد AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};

        ثابت منطقي expandAll = expand != VFsmExpandType::AUTO;
        std::unordered_set<ثابت FsmStateVertex*>
            defaultTargets;  // States that have edge incoming from default vertex
        std::unordered_set<ثابت FsmStateVertex*>
            resetTargets;  // States that have edge incoming from reset vertex
        𐡄𐡍 (!expandAll) {
            𐡄𐡍 (m_defaultVertexp) {
                لكل (ثابت V3GraphEdge& edge : m_defaultVertexp->outEdges()) {
                    defaultTargets.emplace(edge.top()->as<FsmStateVertex>());
                }
            }
            𐡄𐡍 (m_resetVertexp) {
                لكل (ثابت V3GraphEdge& edge : m_resetVertexp->outEdges()) {
                    resetTargets.emplace(edge.top()->as<FsmStateVertex>());
                }
            }
        }
        ثابت منطقي isFullExpansion = expand == VFsmExpandType::FULL;
        𐡄𐡍 (isFullExpansion) initUndefinedStates();
        لكل (ثابت FsmStateVertex* ثابت statep : m_states) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* ثابت declp = buildStateCoverDecl(statep->label());
            initialStateCasep->addItemsp(
                جديد AstCaseItem{m_flp, makeConst(statep->value().num().toUQuad()),
                                جديد AstCoverInc{m_flp, declp, 𐡒𐡔𐡈}});
            ثابت منطقي expandAllOrReset
                = expandAll || (resetTargets.find(statep) != resetTargets.end());
            ثابت منطقي defaultExpantion
                = !expandAllOrReset && (defaultTargets.find(statep) != defaultTargets.end());
            arcCasep->addItemsp(جديد AstCaseItem{
                m_flp, makeConst(statep->value().num().toUQuad()),
                AstNode::addNext<AstNodeStmt>(
                    جديد AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), isFullExpansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        𐡄𐡍 (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            𐡄𐡍 (isFullExpansion) {
                لكل (ثابت uint64_t value : m_undefinedStates) {
                    ثابت std::string& label = quadToBitStirngLabel(value);
                    AstCoverOtherDecl* ثابت declp = buildStateCoverDecl(label);
                    initialStateCasep->addItemsp(جديد AstCaseItem{
                        m_flp, makeConst(value), جديد AstCoverInc{m_flp, declp, 𐡒𐡔𐡈}});
                    arcCasep->addItemsp(
                        جديد AstCaseItem{m_flp, makeConst(value),
                                        AstNode::addNext<AstNodeStmt>(
                                            جديد AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, 𐡒𐡔𐡈, خطأ))});
                }
            } وإلا {
                // No expansion
                arcCasep->addItemsp(جديد AstCaseItem{m_flp, nullptr, nullptr});
                initialStateCasep->addItemsp(جديد AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        m_coverIncp = arcCasep;
        m_initialCoverIncp = initialStateCasep;
    }
    VL_UNCOPYABLE(FsmCoverIncBuilder);
    ~FsmCoverIncBuilder() = افتراضي;
    AstNodeCoverDecl* getCoverDeclsp() ثابت { أرجع m_declsp; }
    AstNodeStmt* getCoverIncp() ثابت { أرجع m_coverIncp; }
    AstNodeStmt* getInitialCoverIncp() ثابت { أرجع m_initialCoverIncp; }
};

// Lower the completed FSM graphs into the concrete coverage declarations,
// previous-state tracking, and pre/post-triggered instrumentation that the
// runtime uses to record state and transition coverage.
صنف FsmLowerVisitor final {
    // STATE - across all visitors
    ثابت FsmState& m_state;
    V3SharedTmps m_prevTmps{"__VfsmCovPrev", VVarType::MODULETEMP};  // Previous states

    // METHODS
    // Rebuild the original event control from the saved sense description so
    // post-state coverage sampling runs on the same triggering edges.
    ساكن AstSenTree* buildSenTree(FileLine* flp, ثابت std::vector<FsmSenDesc>& senses) {
        AstSenTree* ثابت sentreep = جديد AstSenTree{flp, nullptr};
        لكل (ثابت FsmSenDesc& sense : senses) {
            AstSenItem* ثابت senItemp
                = جديد AstSenItem{flp, VEdgeType{sense.edgeType},
                                 جديد AstVarRef{flp, sense.varScopep, VAccess::READ}};
            sentreep->addSensesp(senItemp);
        }
        أرجع sentreep;
    }

    // Lower one fully detected FSM graph into the concrete coverage machinery
    // used by generated models: declarations, previous-state tracking, and the
    // pre/post-triggered increment logic for states and arcs.
    فراغ buildOne(ثابت FsmGraph& graph) {
        UINFO(1, "buildOne lowering FSM " << graph.stateVarName()
                                          << " vertices=" << graph.vertices().size() << endl);
        AstAlways* ثابت alwaysp = graph.stateAlwaysp();
        AstScope* ثابت scopep = graph.scopep();
        AstVarScope* ثابت stateVscp = graph.stateVarScopep();
        AstVarScope* ثابت sampleVscp = graph.sampleVarScopep();
        FileLine* ثابت flp = graph.fileline();
        AstNodeModule* ثابت modp = scopep->modp();
        AstVarScope* ثابت prevVscp
            = m_prevTmps.make(flp, scopep, sampleVscp->dtypep(), stateVscp->varp()->shortName());
        // The saved previous-state temp crosses the scheduler's pre/post split
        // in the same way as Verilator's built-in NBA shadow variables, so keep
        // both vars marked as post-life participants for stable MT ordering.
        stateVscp->optimizeLifePost(𐡒𐡔𐡈);
        sampleVscp->optimizeLifePost(𐡒𐡔𐡈);
        prevVscp->optimizeLifePost(𐡒𐡔𐡈);

        AstActive* ثابت initActivep
            = جديد AstActive{flp, "fsm-coverage-init",
                            جديد AstSenTree{flp, جديد AstSenItem{flp, AstSenItem::Initial{}}}};
        initActivep->senTreeStorep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->addStmtsp(جديد AstInitialStatic{
            flp, جديد AstAssign{flp, جديد AstVarRef{flp, prevVscp, VAccess::WRITE},
                               جديد AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->addBlocksp(initActivep);

        AstAlwaysPost* ثابت covPostp = جديد AstAlwaysPost{flp};
        منطقي updatePrevAfterPost = خطأ;
        𐡄𐡍 (alwaysp) {
            // Save the previous state as plain sequential logic at the front of
            // the original always_ff body, then evaluate coverage in post logic
            // after the delayed state update commits. This avoids a scheduler
            // race between a separate AstAlwaysPre task and the real state
            // commit for direct parent-level registers.
            alwaysp->prependStmtsp(جديد AstAssign{flp, جديد AstVarRef{flp, prevVscp, VAccess::WRITE},
                                                 جديد AstVarRef{flp, sampleVscp, VAccess::READ}});
        } وإلا {
            // Wrapper-derived register candidates do not have a parent
            // always_ff body to splice into. Sample coverage first, then save
            // the current state for the next clock tick; this survives cell
            // boundary scheduling where the real flop update lives elsewhere.
            updatePrevAfterPost = 𐡒𐡔𐡈;
            prevVscp->varp()->setIgnorePostRead();
        }

        {
            // Create and increment proper AstNodeCoverDecls
            ثابت FsmCoverIncBuilder coverIncBuilder{graph, prevVscp};
            modp->addStmtsp(coverIncBuilder.getCoverDeclsp());
            initActivep->addStmtsp(coverIncBuilder.getInitialCoverIncp());
            covPostp->addStmtsp(
                جديد AstIf{flp,
                          جديد AstNeq{flp, جديد AstVarRef{flp, prevVscp, VAccess::READ},
                                     جديد AstVarRef{flp, sampleVscp, VAccess::READ}},
                          coverIncBuilder.getCoverIncp()});
        }

        𐡄𐡍 (updatePrevAfterPost) {
            covPostp->addStmtsp(جديد AstAssign{flp, جديد AstVarRef{flp, prevVscp, VAccess::WRITE},
                                              جديد AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        AstSenTree* ثابت sentreep = buildSenTree(flp, graph.senses());
        AstActive* ثابت activep = جديد AstActive{flp, "fsm-coverage", sentreep};
        activep->senTreeStorep(sentreep);
        scopep->addBlocksp(activep);
        activep->addStmtsp(covPostp);
    }

عام:
    // CONSTRUCTORS
    // Lower every detected FSM graph from the shared local state into
    // concrete coverage instrumentation while the saved scoped pointers are
    // still valid in the same pass.
    explicit FsmLowerVisitor(ثابت FsmState& state)
        : m_state{state} {
        لكل (ثابت DetectedFsm& fsm : m_state.fsms()) { buildOne(*fsm.graphp); }
    }
};

}  // namespace

فراغ V3FsmDetect::detect(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    FsmState state;
    // Phase 1: recover each supported FSM into a complete graph while the
    // original clocked/case structure is still easy to recognize.
    FsmDetectVisitor detect{state, rootp};
    𐡄𐡍 (dumpGraphLevel() >= 6) {
        size_t index = 0;
        لكل (ثابت DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dumpTag(index++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { FsmLowerVisitor lower{state}; }
    V3Global::dumpCheckGlobalTree("fsm-detect", 0, dumpTreeEitherLevel() >= 3);
}
