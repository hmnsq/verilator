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
صنف Fsm_State_قيمة final {
    // Hash/equality key only. It deliberately ignores signedness because
    // signed and unsigned constants with the same width and bits denote the
    // same encoded FSM state.
    string m_مفتاح;  // Canonical "width:value" identity, independent of signedness

    // Semantic value. This is what diagnostics and lowering use when printing
    // values or rebuilding AstConst nodes for instrumentation.
    V3_رقم m_رقم;  // Original folded value, preserving width for lowered comparisons

    ساكن string make_مفتاح(ثابت V3_رقم& رقم) {
        V3_رقم مفتاح_رقم = رقم;
        // Signedness does not change FSM state identity: same width and bits
        // should address the same graph vertex.
        مفتاح_رقم.isSigned(خطأ);
        أرجع cvtToStr(مفتاح_رقم.width()) + ":" + مفتاح_رقم.ascii(true, true);
    }

عام:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    Fsm_State_قيمة()
        : m_مفتاح{"1:1'h0"}
        , m_رقم{static_cast<Ast_عقدة*>(nullptr), 1, 0} {}
    explicit Fsm_State_قيمة(ثابت V3_رقم& رقم)
        : m_مفتاح{make_مفتاح(رقم)}
        , m_رقم{رقم} {}

    ثابت string& مفتاح() ثابت { أرجع m_مفتاح; }
    ثابت V3_رقم& رقم() ثابت { أرجع m_رقم; }
    string ascii() ثابت { أرجع m_رقم.ascii(true, true); }
    string warnText() ثابت {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        إذا (m_رقم.width() <= 32) أرجع cvtToStr(m_رقم.toUInt());
        أرجع ascii();
    }

    منطقي operator==(ثابت Fsm_State_قيمة& rhs) ثابت { أرجع m_مفتاح == rhs.m_مفتاح; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
بنية Fsm_State_قيمة_Hash final {
    size_t operator()(ثابت Fsm_State_قيمة& قيمة) ثابت {
        أرجع std::hash<string>{}(قيمة.مفتاح());
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
    منطقي active_أدنى = خطأ;
};

صنف FsmResetArcDesc final {
    Fsm_State_قيمة m_to_قيمة;  // Encoded reset target state.
    Ast_عقدة* m_nodep = nullptr;  // Source node for warnings and emitted metadata.
    Ast_عقدة_تعبير* m_valuep = nullptr;  // Expression that provided the reset value.

عام:
    FsmResetArcDesc() = افتراضي;
    FsmResetArcDesc(Fsm_State_قيمة to_قيمة, Ast_عقدة_Assign* nodep)
        : m_to_قيمة{to_قيمة}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(Fsm_State_قيمة to_قيمة, Ast_عقدة* nodep, Ast_عقدة_تعبير* valuep)
        : m_to_قيمة{to_قيمة}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    Fsm_State_قيمة to_قيمة() ثابت { أرجع m_to_قيمة; }
    Ast_عقدة* nodep() ثابت { أرجع m_nodep; }
    Ast_عقدة_تعبير* valuep() ثابت { أرجع m_valuep; }
};

بنية FsmWrapperRoles final {
    string dPort;
    string qPort;
    string clkPort;
    string rstPort;
    string rstValParam;
    منطقي has_Rst_Active_أدنى = خطأ;
    منطقي rst_Active_أدنى = خطأ;
};

ساكن منطقي fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, ثابت string& portName,
                                                  منطقي& active_أدنى) {
    منطقي matched = خطأ;
    cellp->modp()->foreach([&](AstSenItem* itemp) {
        Ast_عقدة_Var_Ref* ثابت vrefp = itemp->varrefp();
        إذا (!vrefp) أرجع;
        إذا (vrefp->varp()->name() != portName) أرجع;
        active_أدنى = itemp->edgeType() == VEdgeType::ET_NEGEDGE;
        matched = true;
    });
    أرجع matched;
}

ساكن ثابت V3Control::FsmRegisterWrapper* fsmRegisterWrapperDesc(AstCell* cellp) {
    Ast_عقدة_Module* ثابت modp = cellp->modp();
    ثابت string origName = modp->origName();
    إذا (ثابت V3Control::FsmRegisterWrapper* ثابت descp
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
    roles.rstValParam = desc.reset_قيمة;
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
    File_سطر* m_flp = nullptr;  // Representative source location.

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
    File_سطر* fileline() ثابت { أرجع m_flp; }
    فراغ fileline(File_سطر* flp) { m_flp = flp; }
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
    Fsm_State_قيمة m_قيمة;  // Encoded state value for real state vertices.

محمي:
    FsmVertex(V3Graph* graphp, Kind kind, string label, Fsm_State_قيمة قيمة) VL_MT_DISABLED
        : V3GraphVertex{graphp},
          m_kind{kind},
          m_label{label},
          m_قيمة{قيمة} {}
    ~FsmVertex() override = افتراضي;

عام:
    Kind kind() ثابت { أرجع m_kind; }
    منطقي isState() ثابت { أرجع m_kind == Kind::STATE; }
    منطقي isResetAny() ثابت { أرجع m_kind == Kind::RESET_ANY; }
    منطقي isDefaultAny() ثابت { أرجع m_kind == Kind::DEFAULT_ANY; }
    ثابت string& label() ثابت { أرجع m_label; }
    ثابت Fsm_State_قيمة& قيمة() ثابت { أرجع m_قيمة; }

    string name() ثابت override VL_MT_SAFE { أرجع m_label + "=" + m_قيمة.ascii(); }
};

صنف FsmStateVertex final : عام FsmVertex {
    VL_RTTI_IMPL(FsmStateVertex, FsmVertex)

عام:
    FsmStateVertex(V3Graph* graphp, string label, Fsm_State_قيمة قيمة) VL_MT_DISABLED
        : FsmVertex{graphp, Kind::STATE, label, قيمة} {}
    ~FsmStateVertex() override = افتراضي;

    string dotColor() ثابت override { أرجع "lightblue"; }
    string dotShape() ثابت override { أرجع "ellipse"; }
};

صنف FsmPseudoVertex final : عام FsmVertex {
    VL_RTTI_IMPL(FsmPseudoVertex, FsmVertex)

عام:
    FsmPseudoVertex(V3Graph* graphp, Kind kind, string label) VL_MT_DISABLED
        : FsmVertex{graphp, kind, label, Fsm_State_قيمة{}} {}
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
    File_سطر* m_flp = nullptr;  // Source location for emitted coverage metadata.

عام:
    FsmArcEdge(V3Graph* graphp, FsmVertex* fromp, FsmStateVertex* قمة, منطقي isReset, منطقي isCond,
               منطقي isDefault, File_سطر* flp) VL_MT_DISABLED : V3GraphEdge{graphp, fromp, قمة, 1},
                                                               m_isReset{isReset},
                                                               m_isCond{isCond},
                                                               m_isDefault{isDefault},
                                                               m_flp{flp} {}
    ~FsmArcEdge() override = افتراضي;

    منطقي isReset() ثابت { أرجع m_isReset; }
    منطقي isCond() ثابت { أرجع m_isCond; }
    منطقي isDefault() ثابت { أرجع m_isDefault; }
    File_سطر* fileline() ثابت { أرجع m_flp; }

    string dotLabel() ثابت override {
        إذا (m_isReset) أرجع "reset";
        إذا (m_isDefault) أرجع "default";
        إذا (m_isCond) أرجع "cond";
        أرجع "";
    }
    string dotColor() ثابت override {
        إذا (m_isReset) أرجع "darkgreen";
        إذا (m_isDefault) أرجع "orange";
        إذا (m_isCond) أرجع "blue";
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
    File_سطر* m_flp = nullptr;  // Representative source location for declarations/arcs.
    std::unordered_map<Fsm_State_قيمة, FsmStateVertex*, Fsm_State_قيمة_Hash>
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
    File_سطر* fileline() ثابت { أرجع m_flp; }
    فراغ fileline(File_سطر* flp) { m_flp = flp; }

    FsmStateVertex* addStateVertex(string label, Fsm_State_قيمة قيمة) VL_MT_DISABLED {
        FsmStateVertex* ثابت vertexp = جديد FsmStateVertex{هذا, label, قيمة};
        m_stateVertices.emplace(قيمة, vertexp);
        أرجع vertexp;
    }
    FsmPseudoVertex* resetAnyVertex() VL_MT_DISABLED { أرجع m_resetVertexp; }
    FsmPseudoVertex* defaultAnyVertex() VL_MT_DISABLED { أرجع m_defaultVertexp; }
    FsmArcEdge* addArc(Fsm_State_قيمة from_قيمة, Fsm_State_قيمة to_قيمة, منطقي isReset, منطقي isCond,
                       منطقي isDefault, File_سطر* flp) VL_MT_DISABLED {
        FsmStateVertex* ثابت قمة = m_stateVertices.at(to_قيمة);
        FsmVertex* fromp = nullptr;
        إذا (isReset) {
            fromp = resetAnyVertex();
        } وإلا إذا (isDefault) {
            fromp = defaultAnyVertex();
        } وإلا {
            fromp = m_stateVertices.at(from_قيمة);
        }
        أرجع جديد FsmArcEdge{هذا, fromp, قمة, isReset, isCond, isDefault, flp};
    }

    string name() ثابت VL_MT_SAFE {
        أرجع "FSM "
               + (m_stateVarName.empty() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dumpTag(size_t index) ثابت {
        string tag = stateVarInternalName();
        لكل (حرف& ch : tag) {
            إذا (!std::isalnum(static_cast<unsigned حرف>(ch))) ch = '_';
        }
        أرجع "fsm_" + cvtToStr(index) + "_" + tag;
    }
};

بنية DetectedFsm final {
    std::unique_ptr<FsmGraph> graphp;  // Extracted graph for one detected FSM candidate.
};

بنية FsmCaseCandidate final {
    Ast_عقدة* warnNodep = nullptr;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = nullptr;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
بنية FsmStateComparison final {
    AstVarScope* stateVscp = nullptr;  // Compared state variable
    Ast_عقدة_تعبير* valuep = nullptr;  // Compared constant value expression
    Fsm_State_قيمة قيمة;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
بنية FsmIfBranch final {
    AstIf* ifp = nullptr;  // Source if/else-if node
    Ast_عقدة* stmtsp = nullptr;  // Branch body
    Ast_عقدة_تعبير* valuep = nullptr;  // Source state value expression
    Fsm_State_قيمة from_قيمة;  // Encoded source state value
    منطقي has_قمة_Guard = خطأ;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
بنية FsmIfChainCandidate final {
    AstIf* ifp = nullptr;  // Top-level if-chain node
    AstVarScope* compareVscp = nullptr;  // Variable used by every state comparison
    std::vector<FsmIfBranch> branches;  // Recognized state-dispatch branches
    Ast_عقدة* defaultStmtsp = nullptr;  // Optional final else body
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
    std::vector<std::pair<string, Fsm_State_قيمة>> states;  // User label and encoded value
    std::unordered_map<Fsm_State_قيمة, StateConstLabel, Fsm_State_قيمة_Hash>
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
        إذا (it != m_fsmIndex.end()) أرجع m_fsms.at(it->second);
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
    ساكن Ast_عقدة_DType* unwrapEnumCandidate(Ast_عقدة_DType* dtypep) {
        أرجع dtypep->skipRefToEnump();
    }

    ساكن string candidateConflictContext(Ast_عقدة* laterNodep,
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

    ساكن منطقي simple_Param_State_قيمة(AstCell* cellp, ثابت string& name, Fsm_State_قيمة& قيمة,
                                      Ast_عقدة_تعبير*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = nullptr;
        لكل (Ast_عقدة* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ثابت varp = VN_CAST(stmtp, Var);
            إذا (!varp || !varp->isParam() || varp->name() != name) استمر;
            valuepr = VN_AS(varp->valuep(), عقدة_تعبير);
            أرجع const_قيمة_Status(valuepr, قيمة) == Const_قيمة_Status::OK;
        }
        أرجع خطأ;
    }

    ساكن منطقي childPortInScope(AstVarScope* vscp, AstScope* parentScopep, AstCell*& cellpr) {
        إذا (!vscp->varp()->isIO()) أرجع خطأ;
        AstScope* ثابت scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        إذا (scopep->aboveScopep() != parentScopep) أرجع خطأ;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        أرجع true;
    }

    ساكن AstVarScope* simpleAssignVarScope(Ast_عقدة_تعبير* exprp) {
        AstVarRef* ثابت vrefp = VN_CAST(exprp, VarRef);
        أرجع vrefp ? vrefp->varScopep() : nullptr;
    }

    فراغ addWrapperCell(AstScope* scopep, AstCell* cellp) {
        m_cellPortAliases.emplace(cellp, FsmCellPortMap{});
        m_cellPortChildAliases.emplace(cellp, FsmCellPortMap{});
        ثابت std::pair<AstScope*, AstCell*> item{scopep, cellp};
        إذا (std::find(m_wrapperCells.cbegin(), m_wrapperCells.cend(), item)
            != m_wrapperCells.cend()) {
            أرجع;
        }
        m_wrapperCells.emplace_back(item);
    }

    فراغ collectCellPortAlias(AstAssignW* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* ثابت lhsVscp = simpleAssignVarScope(nodep->lhsp());
        AstVarScope* ثابت rhsVscp = simpleAssignVarScope(nodep->rhsp());
        إذا (!lhsVscp || !rhsVscp) أرجع;
        AstCell* cellp = nullptr;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        إذا (childPortInScope(lhsVscp, m_scopep, cellp)) {
            إذا (!fsmRegisterWrapperDesc(cellp)) أرجع;
            UASSERT_OBJ(lhsVscp->varp()->is_مدخل(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cellPortAliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cellPortChildAliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            addWrapperCell(m_scopep, cellp);
        } وإلا إذا (childPortInScope(rhsVscp, m_scopep, cellp)) {
            إذا (!fsmRegisterWrapperDesc(cellp)) أرجع;
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

        std::vector<std::pair<AstCase*, Ast_عقدة_تعبير*>>
        oneBlockCandidates(AstAlways* alwaysp) ثابت {
            std::vector<std::pair<AstCase*, Ast_عقدة_تعبير*>> candidates;
            Ast_عقدة* ثابت stmtsp = alwaysp->stmtsp();
            إذا (AstIf* ثابت firstIfp = VN_CAST(stmtsp, If)) {
                إذا (AstCase* ثابت casep = VN_CAST(firstIfp->elsesp(), Case)) {
                    candidates.emplace_back(casep,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            لكل (Ast_عقدة* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                إذا (AstCase* ثابت casep = VN_CAST(nodep, Case))
                    candidates.emplace_back(casep, nullptr);
            }
            أرجع candidates;
        }

        std::vector<std::pair<AstIf*, Ast_عقدة_تعبير*>>
        oneBlockIfCandidates(AstAlways* alwaysp) ثابت {
            std::vector<std::pair<AstIf*, Ast_عقدة_تعبير*>> candidates;
            Ast_عقدة* ثابت stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            إذا (AstIf* ثابت firstIfp = VN_CAST(stmtsp, If)) {
                إذا (AstIf* ثابت chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(firstIfp->elsesp()), If)) {
                    candidates.emplace_back(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            لكل (Ast_عقدة* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                إذا (AstIf* ثابت ifp = VN_CAST(nodep, If)) candidates.emplace_back(ifp, nullptr);
            }
            أرجع candidates;
        }

        منطقي matchRegisterCandidate(AstAlways* alwaysp, FsmRegisterCandidate& cand) ثابت {
            أرجع FsmDetectVisitor::matchRegisterAlways(alwaysp, m_scopep, cand);
        }

        فراغ buildOneBlockCandidate(AstAlways* alwaysp, AstVarScope* vscp, Ast_عقدة_تعبير* resetCondp,
                                    FsmRegisterCandidate& reg) ثابت {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.nextVscp(vscp);
            reg.senses() = FsmDetectVisitor::describe_Sen_شجرة(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != nullptr);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* ثابت firstIfp = VN_CAST(alwaysp->stmtsp(), If);
            إذا (firstIfp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = nullptr;
                ثابت ResetAssignStatus resetStatus = FsmDetectVisitor::collectConstStateAssigns(
                    firstIfp->thensp(), resetStateVscp, reg.resetArcs());
                إذا (resetStatus == ResetAssignStatus::EMPTY) {
                    reg.resetArcs().clear();
                } وإلا إذا (resetStatus == ResetAssignStatus::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().clear();
                    Fsm_State_قيمة reset_قيمة;
                    Ast_عقدة* ثابت thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(firstIfp->thensp());
                    UASSERT_OBJ(thenNodep, firstIfp,
                                "one-block reset fallback requires a non-empty reset branch");
                    Ast_عقدة_Assign* ثابت resetAssp = FsmDetectVisitor::direct_Const_State_Assign_عقدة(
                        thenNodep, resetStateVscp, reset_قيمة);
                    إذا (resetAssp && resetStateVscp == vscp) {
                        reg.resetArcs().emplace_back(reset_قيمة, resetAssp);
                    }
                } وإلا إذا (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                    reg.resetArcs().clear();
                }
            }
        }
    };

    منطقي matchFsmWrapperCell(AstScope* scopep, AstCell* cellp, FsmRegisterCandidate& cand) ثابت {
        FsmWrapperRoles roles = rolesFromDesc(*fsmRegisterWrapperDesc(cellp));

        AstVarScope* ثابت nextVscp = roleVarScope(cellp, roles.dPort);
        AstVarScope* ثابت stateVscp = roleVarScope(cellp, roles.qPort);
        إذا (!nextVscp || !stateVscp) {
            أرجع rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* ثابت clkVscp = roleVarScope(cellp, roles.clkPort);
        إذا (!clkVscp) {
            أرجع rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper instance requires a simple clock connection");
        }

        FsmSenDesc clkSense;
        clkSense.edgeType = VEdgeType::ET_POSEDGE;
        clkSense.varScopep = clkVscp;
        cand.senses().ادفع_back(clkSense);

        AstVarScope* resetVscp = nullptr;
        إذا (!roles.rstPort.empty()) resetVscp = roleVarScope(cellp, roles.rstPort);
        إذا (resetVscp) {
            // The descriptor identifies the reset port but not its polarity. Use
            // the wrapper's own event control AST as the contract for sampling
            // the connected parent signal.
            منطقي inferred_Active_أدنى = خطأ;
            إذا (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rstPort, inferred_Active_أدنى)) {
                roles.has_Rst_Active_أدنى = true;
                roles.rst_Active_أدنى = inferred_Active_أدنى;
            }
        }

        Ast_عقدة_تعبير* resetValuep = nullptr;
        Fsm_State_قيمة reset_قيمة;
        ثابت منطقي has_Reset_قيمة
            = !roles.rstValParam.empty()
              && simple_Param_State_قيمة(cellp, roles.rstValParam, reset_قيمة, resetValuep);
        إذا (resetVscp && roles.has_Rst_Active_أدنى && has_Reset_قيمة) {
            FsmSenDesc rstSense;
            rstSense.edgeType = roles.rst_Active_أدنى ? VEdgeType::ET_NEGEDGE : VEdgeType::ET_POSEDGE;
            rstSense.varScopep = resetVscp;
            cand.senses().ادفع_back(rstSense);
            cand.resetCond().varScopep = resetVscp;
            cand.resetCond().active_أدنى = roles.rst_Active_أدنى;
            cand.hasResetCond(true);
            cand.resetArcs().emplace_back(reset_قيمة, cellp, resetValuep);
        } وإلا إذا (!roles.rstPort.empty() || !roles.rstValParam.empty()) {
            string reason;
            إذا (roles.rstPort.empty()) {
                reason = "reset port is not configured";
            } وإلا إذا (!resetVscp) {
                reason = "reset connection is missing or not a simple variable";
            } وإلا إذا (!roles.has_Rst_Active_أدنى) {
                reason = "reset polarity could not be inferred from the wrapper";
            } وإلا إذا (roles.rstValParam.empty()) {
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
        أرجع true;
    }

    صنف ComboAlwaysAnalyzer final {
    عام:
        بنية ComboMatch final {
            ثابت FsmRegisterCandidate* matchedp = nullptr;
            Ast_عقدة* warnNodep = nullptr;
        };

    خاص:
        ثابت std::vector<FsmRegisterCandidate>& m_registerCandidates;

    عام:
        explicit ComboAlwaysAnalyzer(ثابت std::vector<FsmRegisterCandidate>& registerCandidates)
            : m_registerCandidates{registerCandidates} {}

        ComboMatch matchCase(Ast_عقدة* stmtsp, AstCase* casep) ثابت {
            ComboMatch match;
            AstVarRef* ثابت selp = VN_CAST(casep->exprp(), VarRef);
            إذا (!selp) أرجع match;
            لكل (ثابت FsmRegisterCandidate& reg : m_registerCandidates) {
                إذا (selp->varScopep() == reg.nextVscp()) {
                    إذا (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        استمر;
                    }
                } وإلا إذا (selp->varScopep() != reg.stateVscp()) {
                    استمر;
                }
                Ast_عقدة* ثابت warnNodep = FsmDetectVisitor::case_Supported_Transition_عقدة(
                    casep, reg.nextVscp(), reg.inclCond());
                إذا (!warnNodep) استمر;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            أرجع match;
        }

        ComboMatch matchIfChain(Ast_عقدة* stmtsp, ثابت FsmIfChainCandidate& chain) ثابت {
            ComboMatch match;
            لكل (ثابت FsmRegisterCandidate& reg : m_registerCandidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                إذا (chain.compareVscp == reg.nextVscp()) {
                    إذا (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.nextVscp())) {
                        استمر;
                    }
                } وإلا إذا (chain.compareVscp != reg.stateVscp()) {
                    استمر;
                }
                Ast_عقدة* ثابت warnNodep
                    = FsmDetectVisitor::if_Chain_Supported_Transition_عقدة(chain, reg.nextVscp());
                إذا (!warnNodep) استمر;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            أرجع match;
        }

        منطقي shouldWarnUnsupported(Ast_عقدة* stmtsp, AstCase* casep) ثابت {
            ثابت AstVarRef* ثابت selp = VN_CAST(casep->exprp(), VarRef);
            إذا (!selp) أرجع خطأ;

            أرجع std::any_of(
                m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                [&](ثابت FsmRegisterCandidate& reg) -> منطقي {
                    ثابت منطقي matchesNext = selp->varScopep() == reg.nextVscp();
                    ثابت منطقي matchesState = selp->varScopep() == reg.stateVscp();

                    إذا (!matchesNext && !matchesState) أرجع خطأ;
                    إذا (matchesNext
                        && !FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        أرجع خطأ;
                    }
                    أرجع FsmDetectVisitor::case_Supported_Transition_عقدة(casep, reg.nextVscp(),
                                                                         reg.inclCond());
                });
        }
    };

    // Reset arcs are only modeled for the simple signal form that survives to
    // this pass after earlier normalization.
    ساكن منطقي isSimpleResetCond(Ast_عقدة_تعبير* condp) { أرجع VN_IS(condp, VarRef); }

    // Normalize the reset condition into a compact description so the lowering
    // phase can regenerate the same predicate after detection. By the time
    // this pass runs, active-low source forms such as "!rst_n" have already
    // been canonicalized to a positive-condition if/else shape, so only a
    // plain VarRef survives here.
    ساكن FsmResetCondDesc describeResetCond(Ast_عقدة_تعبير* condp) {
        FsmResetCondDesc desc;
        إذا (AstVarRef* ثابت vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        أرجع desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    ساكن std::vector<FsmSenDesc> describe_Sen_شجرة(Ast_Sen_شجرة* sentreep) {
        std::vector<FsmSenDesc> senses;
        لكل (AstSenItem* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), SenItem)) {
            Ast_عقدة_Var_Ref* ثابت vrefp = itemp->varrefp();
            إذا (!vrefp) استمر;
            FsmSenDesc desc;
            desc.edgeType = itemp->edgeType().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.ادفع_back(desc);
        }
        أرجع senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    ساكن منطقي isIgnorableStmt(Ast_عقدة* nodep) { أرجع VN_IS(nodep, CoverInc); }

    ساكن Ast_عقدة* skipLeadingIgnorableStmt(Ast_عقدة* nodep) {
        طالما (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        أرجع nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    ساكن Ast_عقدة* singleMeaningfulStmt(Ast_عقدة* stmtp) {
        Ast_عقدة* resultp = nullptr;
        لكل (Ast_عقدة* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            إذا (isIgnorableStmt(nodep)) استمر;
            إذا (resultp) أرجع nullptr;
            resultp = nodep;
        }
        أرجع resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    ساكن Ast_عقدة* singleMeaningfulBranch(Ast_عقدة* branchp) {
        إذا (!branchp) أرجع nullptr;
        أرجع branchp;
    }

    // By fsm-detect time, non-clocked always @* blocks are already admitted through
    // a missing sentree. This helper therefore only needs to recognize
    // explicit changed-sensitivity lists such as always @(a or b); clocked and
    // event-driven forms remain out of scope.
    ساكن منطقي isPlainComboSentree(ثابت Ast_Sen_شجرة* sentreep) {
        UASSERT(sentreep, "plain combo sensitivity check requires a sensitivity tree");
        لكل (ثابت AstSenItem* senp = sentreep->sensesp(); senp;
             senp = VN_AS(senp->nextp(), SenItem)) {
            إذا (senp->edgeType() == VEdgeType::ET_CHANGED) استمر;
            أرجع خطأ;
        }
        أرجع true;
    }

    فراغ warnUnsupportedComboAlways(ثابت FsmComboAlways& combo) {
        ثابت ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_عقدة* ثابت stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        منطقي warned = خطأ;
        لكل (Ast_عقدة* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ثابت casep = VN_CAST(nodep, Case);
            إذا (!casep) استمر;
            إذا (analyzer.shouldWarnUnsupported(stmtsp, casep)) {
                casep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-clocked always "
                                        "blocks requires a combinational sensitivity list or "
                                        "always_comb");
                warned = true;
            }
            إذا (warned) اكسر;
        }
    }

    // Case-item bodies are single subtrees like if/else arms, not statement
    // lists, so unwrap only local begin/end wrappers here rather than walking
    // sibling case items via nextp().
    ساكن Ast_عقدة_Assign* directStateAssign(Ast_عقدة* stmtp, AstVarScope* stateVscp) {
        Ast_عقدة* ثابت nodep = singleMeaningfulBranch(stmtp);
        إذا (!nodep) أرجع nullptr;
        Ast_عقدة_Assign* ثابت assp = VN_CAST(nodep, عقدة_Assign);
        إذا (!assp) أرجع nullptr;
        AstVarRef* ثابت vrefp = VN_CAST(assp->lhsp(), VarRef);
        إذا (!vrefp || vrefp->varScopep() != stateVscp) أرجع nullptr;
        أرجع assp;
    }

    ساكن Ast_عقدة_Assign* عقدة_State_Var_Assign(Ast_عقدة* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        Ast_عقدة_Assign* ثابت assp = VN_CAST(nodep, عقدة_Assign);
        إذا (!assp) أرجع nullptr;
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp()->baseFromp(true), VarRef);
        AstVarRef* ثابت rhsp = VN_CAST(assp->rhsp(), VarRef);
        إذا (!rhsp || !lhsp) أرجع nullptr;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        أرجع assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    ساكن Ast_عقدة_Assign* directCondStateVarAssign(Ast_عقدة* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, Ast_عقدة_تعبير*& condp,
                                                   Fsm_State_قيمة& reset_قيمة) {
        Ast_عقدة_Assign* ثابت assp = VN_CAST(nodep, عقدة_Assign);
        إذا (!assp) أرجع nullptr;
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp()->baseFromp(true), VarRef);
        AstCond* ثابت rhsp = VN_CAST(assp->rhsp(), Cond);
        إذا (!rhsp || !lhsp) أرجع nullptr;
        AstVarRef* ثابت elsep = VN_CAST(rhsp->elsep(), VarRef);
        إذا (!elsep) أرجع nullptr;
        إذا (const_قيمة_Status(rhsp->thenp(), reset_قيمة) != Const_قيمة_Status::OK) أرجع nullptr;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        أرجع assp;
    }

    ساكن Ast_عقدة_Assign* direct_Const_State_Assign_عقدة(Ast_عقدة* nodep, AstVarScope*& stateVscp,
                                                     Fsm_State_قيمة& قيمة) {
        Ast_عقدة_Assign* ثابت assp = VN_CAST(nodep, عقدة_Assign);
        إذا (!assp) أرجع nullptr;
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp()->baseFromp(true), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        إذا (const_قيمة_Status(assp->rhsp(), قيمة) != Const_قيمة_Status::OK) أرجع nullptr;
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
    ساكن ResetAssignStatus collectConstStateAssigns(Ast_عقدة* stmtp, AstVarScope*& stateVscp,
                                                      std::vector<FsmResetArcDesc>& resetArcs) {
        Ast_عقدة* nodep = skipLeadingIgnorableStmt(stmtp);
        إذا (!nodep) أرجع ResetAssignStatus::EMPTY;
        لكل (;; nodep = nodep->nextp()) {
            AstVarScope* assignStateVscp = nullptr;
            Fsm_State_قيمة قيمة;
            Ast_عقدة_Assign* ثابت assp = direct_Const_State_Assign_عقدة(nodep, assignStateVscp, قيمة);
            إذا (!assp) أرجع ResetAssignStatus::NONE;
            إذا (!stateVscp) stateVscp = assignStateVscp;
            إذا (assignStateVscp != stateVscp) أرجع ResetAssignStatus::NONE;
            إذا (!resetArcs.empty()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.clear();
                أرجع ResetAssignStatus::MULTI_SAME_STATE;
            }
            resetArcs.emplace_back(قيمة, assp);
            إذا (!nodep->nextp()) أرجع ResetAssignStatus::SINGLE;
        }
    }

    ساكن منطقي hasCanonicalNextStateDefaultBeforeCase(Ast_عقدة* stmtsp, Ast_عقدة* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* nextVscp) {
        Ast_عقدة* ثابت bodyp = skipLeadingIgnorableStmt(stmtsp);
        منطقي sawCanonicalDefault = خطأ;
        لكل (Ast_عقدة* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            إذا (nodep == targetp) أرجع sawCanonicalDefault;
            إذا (Ast_عقدة_Assign* ثابت assp = VN_CAST(nodep, عقدة_Assign)) {
                AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* ثابت rhsp = VN_CAST(assp->rhsp(), VarRef);
                إذا (!lhsp || lhsp->varScopep() != nextVscp) استمر;
                إذا (sawCanonicalDefault) {
                    ثابت string nextName = nextVscp->varp()->prettyNameQ();
                    ثابت string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + nextName
                                     + ") when the canonical " + nextName + " = " + stateName
                                     + " default is overwritten before the case statement");
                    أرجع خطأ;
                }
                إذا (!rhsp || rhsp->varScopep() != stateVscp) أرجع خطأ;
                sawCanonicalDefault = true;
            }
        }
    }

    ساكن منطقي ifStateConstAssign(Ast_عقدة* stmtp, AstVarScope* stateVscp,
                                   Fsm_State_قيمة& then_قيمة, Fsm_State_قيمة& else_قيمة) {
        AstIf* ثابت ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        إذا (!ifp || !ifp->elsesp()) أرجع خطأ;
        AstVarScope* thenVscp = nullptr;
        AstVarScope* elseVscp = nullptr;
        Ast_عقدة* ثابت thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        Ast_عقدة* ثابت elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        إذا (!thenNodep || !elseNodep) أرجع خطأ;
        إذا (!direct_Const_State_Assign_عقدة(thenNodep, thenVscp, then_قيمة)) أرجع خطأ;
        إذا (!direct_Const_State_Assign_عقدة(elseNodep, elseVscp, else_قيمة)) أرجع خطأ;
        إذا (thenVscp == stateVscp && elseVscp == stateVscp) أرجع true;
        إذا (thenVscp != elseVscp) أرجع خطأ;
        Ast_عقدة* ثابت followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = nullptr;
        AstVarScope* finalFromVscp = nullptr;
        Ast_عقدة* ثابت finalNodep = singleMeaningfulBranch(followp);
        إذا (!finalNodep) أرجع خطأ;
        إذا (!عقدة_State_Var_Assign(finalNodep, finalStateVscp, finalFromVscp)) أرجع خطأ;
        إذا (finalStateVscp != stateVscp) أرجع خطأ;
        إذا (finalFromVscp != thenVscp) أرجع خطأ;
        أرجع true;
    }

    ساكن منطقي directStateCondConstAssign(Ast_عقدة* stmtp, AstVarScope* stateVscp,
                                           Fsm_State_قيمة& then_قيمة, Fsm_State_قيمة& else_قيمة) {
        Ast_عقدة_Assign* ثابت assp = directStateAssign(stmtp, stateVscp);
        إذا (!assp) أرجع خطأ;
        AstCond* ثابت condp = VN_CAST(assp->rhsp(), Cond);
        إذا (!condp) أرجع خطأ;
        أرجع const_قيمة_Status(condp->thenp(), then_قيمة) == Const_قيمة_Status::OK
               && const_قيمة_Status(condp->elsep(), else_قيمة) == Const_قيمة_Status::OK;
    }

    ساكن Ast_عقدة* case_Item_Supported_Arc_عقدة(AstCaseItem* itemp, AstVarScope* stateVscp,
                                             منطقي inclCond) {
        إذا (itemp->isDefault()) {
            إذا (!inclCond) أرجع nullptr;
        }
        Ast_عقدة_Assign* ثابت assp = directStateAssign(itemp->stmtsp(), stateVscp);
        إذا (assp) {
            Fsm_State_قيمة to_قيمة;
            إذا (const_قيمة_Status(assp->rhsp(), to_قيمة) == Const_قيمة_Status::OK) أرجع assp;
        }
        Fsm_State_قيمة then_قيمة;
        Fsm_State_قيمة else_قيمة;
        إذا (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_قيمة, else_قيمة)) {
            أرجع assp;
        }
        إذا (ifStateConstAssign(itemp->stmtsp(), stateVscp, then_قيمة, else_قيمة)) {
            أرجع singleMeaningfulBranch(itemp->stmtsp());
        }
        أرجع nullptr;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    ساكن Ast_عقدة* case_Supported_Transition_عقدة(AstCase* casep, AstVarScope* stateVscp,
                                                منطقي inclCond) {
        لكل (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            إذا (Ast_عقدة* ثابت nodep = case_Item_Supported_Arc_عقدة(itemp, stateVscp, inclCond))
                أرجع nodep;
        }
        أرجع nullptr;
    }

    ساكن Ast_عقدة* case_Item_Supported_Arc_عقدة_Like(Ast_عقدة* stmtsp, AstVarScope* stateVscp) {
        إذا (Ast_عقدة_Assign* ثابت assp = directStateAssign(stmtsp, stateVscp)) {
            Fsm_State_قيمة to_قيمة;
            إذا (const_قيمة_Status(assp->rhsp(), to_قيمة) == Const_قيمة_Status::OK) أرجع assp;
            Fsm_State_قيمة then_قيمة;
            Fsm_State_قيمة else_قيمة;
            إذا (directStateCondConstAssign(stmtsp, stateVscp, then_قيمة, else_قيمة)) أرجع assp;
        }
        أرجع nullptr;
    }

    ساكن Ast_عقدة* if_Chain_Supported_Transition_عقدة(ثابت FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        لكل (size_t i = 0; i < chain.branches.size(); ++i) {
            ثابت FsmIfBranch& branch = chain.branches[i];
            Ast_عقدة* ثابت nodep = case_Item_Supported_Arc_عقدة_Like(branch.stmtsp, stateVscp);
            إذا (!nodep) أرجع nullptr;
        }
        أرجع chain.branches.front().ifp;
    }

    // Prefer user labels in reports. Forced non-enum FSMs prepopulate synthetic
    // labels, so all emitted arcs should already have a known label here.
    ساكن string label_For_قيمة(
        ثابت std::unordered_map<Fsm_State_قيمة, StateConstLabel, Fsm_State_قيمة_Hash>& labels,
        ثابت Fsm_State_قيمة& قيمة) {
        أرجع labels.at(قيمة).text;
    }

    // The extractor only models constant-valued state transitions, and by the
    // time detect runs those values have already been constant-folded.
    تعداد صنف Const_قيمة_Status : uint8_t { OK, NOT_CONST, XZ };

    ساكن Const_قيمة_Status const_قيمة_Status(Ast_عقدة_تعبير* exprp, Fsm_State_قيمة& قيمة) {
        ثابت AstConst* ثابت constp = VN_CAST(exprp, Const);
        إذا (!constp) أرجع Const_قيمة_Status::NOT_CONST;
        ثابت V3_رقم& رقم = constp->رقم();
        إذا (رقم.isAnyXZ()) أرجع Const_قيمة_Status::XZ;
        قيمة = Fsm_State_قيمة{رقم};
        أرجع Const_قيمة_Status::OK;
    }

    ساكن منطقي pureStateComparisonNoAlias(Ast_عقدة_تعبير* exprp, FsmStateComparison& cmp) {
        AstEq* ثابت eqp = VN_CAST(exprp, Eq);
        إذا (!eqp) أرجع خطأ;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        Ast_عقدة_تعبير* valuep = eqp->rhsp();
        إذا (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->baseFromp(true), VarRef);
            إذا (!vrefp) { أرجع خطأ; }
            valuep = eqp->lhsp();
        }

        Fsm_State_قيمة قيمة;
        إذا (const_قيمة_Status(valuep, قيمة) != Const_قيمة_Status::OK) أرجع خطأ;
        cmp.stateVscp = vrefp->varScopep();
        cmp.valuep = valuep;
        cmp.قيمة = قيمة;
        أرجع true;
    }

    ساكن منطقي pureStateComparison(Ast_عقدة_تعبير* exprp, ثابت FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        إذا (pureStateComparisonNoAlias(exprp, cmp)) أرجع true;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        إذا (AstVarRef* ثابت vrefp = VN_CAST(exprp, VarRef)) {
            ثابت FsmAliasMap::const_iterator it = aliases.find(vrefp->varScopep());
            إذا (it == aliases.end()) أرجع خطأ;
            cmp = it->second;
            أرجع true;
        }
        أرجع خطأ;
    }

    ساكن منطقي supported_قمة_Level_Guard(Ast_عقدة_تعبير* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        إذا (VN_IS(exprp, Or)) أرجع خطأ;
        إذا (VN_IS(exprp, RedAnd)) أرجع خطأ;
        إذا (VN_IS(exprp, RedOr)) أرجع خطأ;
        إذا (VN_IS(exprp, RedXor)) أرجع خطأ;
        أرجع true;
    }

    ساكن منطقي resolveIfPredicate(Ast_عقدة_تعبير* exprp, ثابت FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, منطقي& hasGuard) {
        std::vector<Ast_عقدة_تعبير*> terms;
        std::vector<Ast_عقدة_تعبير*> pending;
        pending.ادفع_back(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        طالما (!pending.empty()) {
            Ast_عقدة_تعبير* ثابت nodep = pending.back();
            pending.اسحب_back();
            إذا (AstAnd* ثابت andp = VN_CAST(nodep, And)) {
                pending.ادفع_back(andp->rhsp());
                pending.ادفع_back(andp->lhsp());
            } وإلا {
                terms.ادفع_back(nodep);
            }
        }

        منطقي sawComparison = خطأ;
        لكل (size_t i = 0; i < terms.size(); ++i) {
            Ast_عقدة_تعبير* ثابت termp = terms[i];
            FsmStateComparison termCmp;
            إذا (pureStateComparison(termp, aliases, termCmp /*ref*/)) {
                إذا (sawComparison) أرجع خطأ;
                cmp = termCmp;
                sawComparison = true;
                استمر;
            }
            إذا (!supported_قمة_Level_Guard(termp)) أرجع خطأ;
            hasGuard = true;
        }
        أرجع sawComparison;
    }

    ساكن فراغ addAlias(FsmAliasMap& aliases, std::unordered_set<ثابت AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, ثابت FsmStateComparison& cmp) {
        إذا (ambiguous.find(aliasVscp) != ambiguous.end()) أرجع;
        ثابت FsmAliasMap::iterator it = aliases.find(aliasVscp);
        إذا (it == aliases.end()) {
            aliases.emplace(aliasVscp, cmp);
            أرجع;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        إذا (it->second.stateVscp == cmp.stateVscp && it->second.قيمة == cmp.قيمة) أرجع;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        أرجع;
    }

    ساكن فراغ collectAliasFromAssign(Ast_عقدة_Assign* assp, FsmAliasMap& aliases,
                                       std::unordered_set<ثابت AstVarScope*>& ambiguous) {
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp(), VarRef);
        إذا (!lhsp) أرجع;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        إذا (!pureStateComparisonNoAlias(assp->rhsp(), cmp /*ref*/)) أرجع;
        addAlias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(Ast_عقدة* stmtsp, Ast_عقدة* limitp) ثابت {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_set<ثابت AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        لكل (Ast_عقدة* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            إذا (Ast_عقدة_Assign* ثابت assp = VN_CAST(nodep, عقدة_Assign)) {
                collectAliasFromAssign(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        لكل (ثابت AstVarScope* ثابت vscp : ambiguous) aliases.erase(vscp);
        أرجع aliases;
    }

    ساكن منطقي collectIfChain(AstIf* ifp, ثابت FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_set<string> seen_قيم;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        طالما (true) {
            FsmStateComparison cmp;
            منطقي hasGuard = خطأ;
            إذا (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) أرجع خطأ;
            إذا (chain.compareVscp && chain.compareVscp != cmp.stateVscp) أرجع خطأ;
            إذا (!seen_قيم.أدرج(cmp.قيمة.مفتاح()).second) أرجع خطأ;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.ادفع_back(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.قيمة, hasGuard});

            Ast_عقدة* ثابت elseNodep
                = singleMeaningfulBranch(skipLeadingIgnorableStmt(curp->elsesp()));
            إذا (!elseNodep) اكسر;
            إذا (AstIf* ثابت elseIfp = VN_CAST(elseNodep, If)) {
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
    ساكن منطقي validate_Known_State_قيمة(Ast_عقدة* nodep, ثابت FsmStateSpace& stateSpace,
                                        ثابت Fsm_State_قيمة& قيمة, ثابت string& role) {
        إذا (stateSpace.labels.find(قيمة) != stateSpace.labels.end()) أرجع true;
        إذا (stateSpace.enumBacked) {
            ثابت string enumRole = role == "source" ? "case item value" : "assigned value";
            nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on enum state variable "
                                        + stateSpace.stateVarp->prettyNameQ() + ": " + enumRole
                                        + " " + قيمة.warnText()
                                        + " is not present in the declared enum");
            أرجع خطأ;
        }
        nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum state variable "
                                    + stateSpace.stateVarp->prettyNameQ() + ": " + role + " value "
                                    + قيمة.warnText()
                                    + " is not present in the inferred state space");
        أرجع خطأ;
    }

    ساكن StateConstLabel stateLabelForConst(AstConst* constp) {
        ثابت string name = constp->origParamName();
        إذا (!name.empty()) أرجع StateConstLabel{Ast_عقدة::prettyName(name), true, 0};
        أرجع StateConstLabel{constp->name(), خطأ, 0};
    }

    ساكن فراغ updateStateLabel(FsmStateSpace& stateSpace, ثابت Fsm_State_قيمة& قيمة,
                                 ثابت StateConstLabel& label) {
        stateSpace.states.at(stateSpace.labels.at(قيمة).stateIndex).first = label.text;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    ساكن منطقي matchRegisterAlways(AstAlways* alwaysp, AstScope* scopep,
                                    FsmRegisterCandidate& cand) {
        إذا (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) أرجع خطأ;

        Ast_عقدة* ثابت stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        Ast_عقدة* ثابت nodep = singleMeaningfulStmt(stmtsp);
        إذا (!nodep) أرجع خطأ;

        AstVarScope* stateVscp = nullptr;
        AstVarScope* nextVscp = nullptr;
        إذا (AstIf* ثابت ifp = VN_CAST(nodep, If)) {
            إذا (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) أرجع خطأ;
            AstVarScope* resetStateVscp = nullptr;
            ثابت ResetAssignStatus resetStatus
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            ثابت منطقي emptyResetBranch = resetStatus == ResetAssignStatus::EMPTY;
            إذا (emptyResetBranch) {
                cand.resetArcs().clear();
            } وإلا إذا (resetStatus == ResetAssignStatus::NONE) {
                cand.resetArcs().clear();
                Fsm_State_قيمة reset_قيمة;
                Ast_عقدة* ثابت thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                Ast_عقدة_Assign* ثابت resetAssp
                    = direct_Const_State_Assign_عقدة(thenNodep, resetStateVscp, reset_قيمة);
                إذا (!resetAssp) أرجع خطأ;
                cand.resetArcs().emplace_back(reset_قيمة, resetAssp);
            } وإلا إذا (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                cand.resetArcs().clear();
            }
            Ast_عقدة* ثابت elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            إذا (!عقدة_State_Var_Assign(elseNodep, stateVscp, nextVscp)) أرجع خطأ;
            إذا (!emptyResetBranch && resetStateVscp != stateVscp) أرجع خطأ;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != nullptr);
        } وإلا {
            Ast_عقدة_تعبير* resetCondp = nullptr;
            Fsm_State_قيمة reset_قيمة;
            // Note the active low 'reset ? next : CONST' form is not accepted
            إذا (Ast_عقدة_Assign* ثابت assp
                = directCondStateVarAssign(nodep, stateVscp, nextVscp, resetCondp, reset_قيمة)) {
                cand.resetArcs().emplace_back(reset_قيمة, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != nullptr);
            } وإلا إذا (!عقدة_State_Var_Assign(nodep, stateVscp, nextVscp)) {
                أرجع خطأ;
            }
        }
        cand.scopep(scopep);
        cand.alwaysp(alwaysp);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(stateVscp);
        cand.nextVscp(nextVscp);
        cand.senses() = describe_Sen_شجرة(alwaysp->sentreep());
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(alwaysp->fileline());
        أرجع true;
    }

    ساكن منطقي add_قيمة_To_State_Space(Ast_عقدة* nodep, FsmStateSpace& stateSpace,
                                     ثابت Fsm_State_قيمة& قيمة, StateConstLabel label) {
        ثابت auto labelIt = stateSpace.labels.find(قيمة);
        إذا (labelIt != stateSpace.labels.end()) {
            StateConstLabel& existingLabel = labelIt->second;
            إذا (existingLabel.text != label.text && existingLabel.fromParam && label.fromParam) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                        "state variable "
                                            + stateSpace.stateVarp->prettyNameQ()
                                            + " with multiple labels for the same value "
                                            + قيمة.warnText() + ": " + existingLabel.text
                                            + " and " + label.text);
                أرجع خطأ;
            }
            إذا (!existingLabel.fromParam && label.fromParam) {
                existingLabel.text = label.text;
                existingLabel.fromParam = label.fromParam;
                updateStateLabel(stateSpace, قيمة, label);
            }
        } وإلا {
            StateConstLabel storedLabel = label;
            storedLabel.stateIndex = stateSpace.states.size();
            stateSpace.states.emplace_back(label.text, قيمة);
            stateSpace.labels.emplace(قيمة, storedLabel);
        }
        أرجع true;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    ساكن منطقي add_تعبير_To_State_Space(Ast_عقدة_تعبير* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_قيمة قيمة;
        ثابت Const_قيمة_Status status = const_قيمة_Status(valuep, قيمة);
        إذا (status != Const_قيمة_Status::OK) {
            إذا (status == Const_قيمة_Status::XZ) {
                valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                         "state variable "
                                             + stateSpace.stateVarp->prettyNameQ()
                                             + " with X/Z state encoding values");
            }
            أرجع خطأ;
        }
        AstConst* ثابت constp = VN_AS(valuep, Const);
        أرجع add_قيمة_To_State_Space(valuep, stateSpace, قيمة, stateLabelForConst(constp));
    }

    ساكن منطقي add_Optional_Target_تعبير_To_State_Space(Ast_عقدة_تعبير* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_قيمة قيمة;
        ثابت Const_قيمة_Status status = const_قيمة_Status(valuep, قيمة);
        إذا (status != Const_قيمة_Status::OK) {
            valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                     "state variable "
                                         + stateSpace.stateVarp->prettyNameQ()
                                         + " with non-constant target state values");
            أرجع خطأ;
        }
        AstConst* ثابت constp = VN_AS(valuep, Const);
        أرجع add_قيمة_To_State_Space(valuep, stateSpace, قيمة, stateLabelForConst(constp));
    }

    ساكن فراغ addResetTargetsToStateSpace(ثابت std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        لكل (ثابت FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.to_قيمة().ascii(), خطأ, 0};
            إذا (AstConst* ثابت constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                add_قيمة_To_State_Space(resetArc.nodep(), stateSpace, resetArc.to_قيمة(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    ساكن منطقي collectDeclaredStateSpace(Ast_عقدة* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, منطقي& needs_Source_قيم) {
        AstVar* ثابت stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        إذا (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        ثابت منطقي forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        إذا (enump) {
            stateSpace.enumBacked = true;
            لكل (AstEnumItem* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
                ثابت Fsm_State_قيمة قيمة{constp->رقم()};
                ثابت size_t stateIndex = stateSpace.states.size();
                stateSpace.states.emplace_back(itemp->name(), قيمة);
                stateSpace.labels.emplace(قيمة,
                                          StateConstLabel{itemp->name(), خطأ, stateIndex});
            }
            أرجع stateSpace.states.size() >= 2;
        }

        إذا (forced) {
            needs_Source_قيم = true;
            أرجع true;
        }

        needs_Source_قيم = true;
        أرجع true;
    }

    template <typename T_ValuepVisitor>
    ساكن منطقي collect_State_Space_From_قيم(Ast_عقدة* warnNodep, AstVarScope* stateVscp,
                                            ثابت std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            ثابت T_ValuepVisitor& visitValueps) {
        منطقي needs_Source_قيم = خطأ;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        إذا (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needs_Source_قيم)) {
            أرجع خطأ;
        }
        إذا (!needs_Source_قيم) أرجع true;
        addResetTargetsToStateSpace(resetArcs, stateSpace);
        إذا (!visitValueps(
                [&](Ast_عقدة_تعبير* valuep) { أرجع add_تعبير_To_State_Space(valuep, stateSpace); })) {
            أرجع خطأ;
        }
        أرجع stateSpace.states.size() >= 2;
    }

    ساكن منطقي collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* assignVscp,
                                  ثابت std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        أرجع collect_State_Space_From_قيم(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, assignVscp, &stateSpace](ثابت auto& visitValuep) {
                لكل (AstCaseItem* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), CaseItem)) {
                    إذا (!itemp->isDefault()) {
                        لكل (Ast_عقدة_تعبير* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), عقدة_تعبير)) {
                            إذا (!visitValuep(condp)) أرجع خطأ;
                        }
                    }
                    إذا (Ast_عقدة_Assign* ثابت assp
                        = directStateAssign(itemp->stmtsp(), assignVscp)) {
                        Fsm_State_قيمة then_قيمة;
                        Fsm_State_قيمة else_قيمة;
                        AstCond* ثابت condp = VN_CAST(assp->rhsp(), Cond);
                        إذا (condp
                            && directStateCondConstAssign(itemp->stmtsp(), assignVscp, then_قيمة,
                                                          else_قيمة)) {
                            إذا (!visitValuep(condp->thenp())) أرجع خطأ;
                            إذا (!visitValuep(condp->elsep())) أرجع خطأ;
                        } وإلا إذا (!add_Optional_Target_تعبير_To_State_Space(assp->rhsp(), stateSpace)) {
                            أرجع خطأ;
                        }
                    }
                }
                أرجع true;
            });
    }

    ساكن منطقي collectStateSpace(ثابت FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* assignVscp,
                                  ثابت std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        أرجع collect_State_Space_From_قيم(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, assignVscp, &stateSpace](ثابت auto& visitValuep) {
                لكل (ثابت FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(visitValuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    Ast_عقدة_Assign* ثابت assp = directStateAssign(branch.stmtsp, assignVscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    Fsm_State_قيمة then_قيمة;
                    Fsm_State_قيمة else_قيمة;
                    AstCond* ثابت condp = VN_CAST(assp->rhsp(), Cond);
                    إذا (condp) {
                        UASSERT_OBJ(directStateCondConstAssign(branch.stmtsp, assignVscp,
                                                               then_قيمة, else_قيمة),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        إذا (!visitValuep(condp->thenp())) أرجع خطأ;
                        إذا (!visitValuep(condp->elsep())) أرجع خطأ;
                    } وإلا إذا (!add_Optional_Target_تعبير_To_State_Space(assp->rhsp(), stateSpace)) {
                        أرجع خطأ;
                    }
                }
                أرجع true;
            });
    }

    // Extract supported case-item transitions in one place so the conservative
    // policy for direct and ternary forms stays consistent. The false exits in
    // this helper are deliberate subset boundaries: they document shapes we do
    // not yet model in this PR and that future FSM-detection work may widen.
    ساكن منطقي emitCaseItemArcs(FsmGraph& graph, AstCaseItem* itemp, AstVarScope* stateVscp,
                                 ثابت FsmStateSpace& stateSpace, منطقي inclCond) {
        std::vector<std::pair<string, Fsm_State_قيمة>> froms;
        إذا (itemp->isDefault()) {
            إذا (!inclCond) أرجع خطأ;
            froms.emplace_back("default", Fsm_State_قيمة{});
        } وإلا {
            لكل (Ast_عقدة_تعبير* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), عقدة_تعبير)) {
                Fsm_State_قيمة قيمة;
                إذا (const_قيمة_Status(condp, قيمة) != Const_قيمة_Status::OK) استمر;
                إذا (!validate_Known_State_قيمة(condp, stateSpace, قيمة, "source")) أرجع true;
                froms.emplace_back(label_For_قيمة(stateSpace.labels, قيمة), قيمة);
            }
            إذا (froms.empty()) أرجع خطأ;
        }

        إذا (Ast_عقدة_Assign* ثابت assp = directStateAssign(itemp->stmtsp(), stateVscp)) {
            Fsm_State_قيمة to_قيمة;
            ثابت Const_قيمة_Status status = const_قيمة_Status(assp->rhsp(), to_قيمة);
            إذا (status == Const_قيمة_Status::OK) {
                إذا (!validate_Known_State_قيمة(assp, stateSpace, to_قيمة, "target")) أرجع true;
                لكل (ثابت std::pair<string, Fsm_State_قيمة>& from : froms) {
                    graph.addArc(from.second, to_قيمة, خطأ, خطأ, itemp->isDefault(),
                                 assp->fileline());
                }
                أرجع true;
            }
        }

        Fsm_State_قيمة then_قيمة;
        Fsm_State_قيمة else_قيمة;
        إذا (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_قيمة, else_قيمة)
            || ifStateConstAssign(itemp->stmtsp(), stateVscp, then_قيمة, else_قيمة)) {
            إذا (!validate_Known_State_قيمة(itemp->stmtsp(), stateSpace, then_قيمة, "target"))
                أرجع true;
            إذا (!validate_Known_State_قيمة(itemp->stmtsp(), stateSpace, else_قيمة, "target"))
                أرجع true;
            لكل (ثابت Fsm_State_قيمة& branch_قيمة : {then_قيمة, else_قيمة}) {
                لكل (ثابت std::pair<string, Fsm_State_قيمة>& from : froms) {
                    graph.addArc(from.second, branch_قيمة, خطأ, true, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            أرجع true;
        }

        أرجع خطأ;
    }

    ساكن فراغ emitStmtArcsFrom(FsmGraph& graph, Ast_عقدة* stmtsp, AstVarScope* stateVscp,
                                 ثابت FsmStateSpace& stateSpace, Fsm_State_قيمة from_قيمة,
                                 منطقي isDefault, منطقي forceCond) {
        Ast_عقدة_Assign* ثابت assp = directStateAssign(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        Fsm_State_قيمة to_قيمة;
        ثابت Const_قيمة_Status status = const_قيمة_Status(assp->rhsp(), to_قيمة);
        إذا (status == Const_قيمة_Status::OK) {
            إذا (!validate_Known_State_قيمة(assp, stateSpace, to_قيمة, "target")) أرجع;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            graph.addArc(from_قيمة, to_قيمة, خطأ, forceCond, isDefault, assp->fileline());
            أرجع;
        }

        Fsm_State_قيمة then_قيمة;
        Fsm_State_قيمة else_قيمة;
        ثابت منطقي condAssign
            = directStateCondConstAssign(stmtsp, stateVscp, then_قيمة, else_قيمة);
        UASSERT_OBJ(condAssign, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        إذا (!validate_Known_State_قيمة(stmtsp, stateSpace, then_قيمة, "target")) أرجع;
        إذا (!validate_Known_State_قيمة(stmtsp, stateSpace, else_قيمة, "target")) أرجع;
        لكل (ثابت Fsm_State_قيمة& branch_قيمة : {then_قيمة, else_قيمة}) {
            graph.addArc(from_قيمة, branch_قيمة, خطأ, true, isDefault, stmtsp->fileline());
        }
    }

    ساكن فراغ emitIfChainArcs(FsmGraph& graph, ثابت FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, ثابت FsmStateSpace& stateSpace) {
        لكل (size_t i = 0; i < chain.branches.size(); ++i) {
            ثابت FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            إذا (!validate_Known_State_قيمة(branch.ifp, stateSpace, branch.from_قيمة, "source"))
                أرجع;
            emitStmtArcsFrom(graph, branch.stmtsp, stateVscp, stateSpace, branch.from_قيمة, خطأ,
                             branch.has_قمة_Guard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    ساكن فراغ addResetArcs(FsmGraph& graph, ثابت std::vector<FsmResetArcDesc>& resetArcs,
                             ثابت FsmStateSpace& stateSpace) {
        لكل (ثابت FsmResetArcDesc& resetArc : resetArcs) {
            إذا (!validate_Known_State_قيمة(resetArc.nodep(), stateSpace, resetArc.to_قيمة(),
                                         "target"))
                استمر;
            graph.addArc(Fsm_State_قيمة{}, resetArc.to_قيمة(), true, خطأ, خطأ,
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
        إذا (!collectStateSpace(casep, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) أرجع;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        إذا (!entry.graphp) {
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
            لكل (ثابت std::pair<string, Fsm_State_قيمة>& state : stateSpace.states) {
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
        إذا (!collectStateSpace(chain, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) أرجع;
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
        لكل (ثابت std::pair<string, Fsm_State_قيمة>& state : stateSpace.states) {
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
        إذا (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) أرجع;
        ثابت std::vector<std::pair<AstCase*, Ast_عقدة_تعبير*>> candidates
            = analyzer.oneBlockCandidates(alwaysp);

        FsmCaseCandidate firstCand;
        لكل (ثابت std::pair<AstCase*, Ast_عقدة_تعبير*>& cand : candidates) {
            AstVarRef* ثابت selp = VN_CAST(cand.first->exprp(), VarRef);
            AstVarScope* ثابت vscp = selp ? selp->varScopep() : nullptr;
            إذا (!vscp) استمر;
            إذا (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processCase(cand.first, vscp, reg);
            } وإلا إذا (vscp != firstCand.stateVscp) {
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

        إذا (firstCand.stateVscp) أرجع;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        ثابت std::vector<std::pair<AstIf*, Ast_عقدة_تعبير*>> ifCandidates
            = analyzer.oneBlockIfCandidates(alwaysp);
        لكل (ثابت std::pair<AstIf*, Ast_عقدة_تعبير*>& cand : ifCandidates) {
            ثابت FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.first);
            FsmIfChainCandidate chain;
            إذا (!collectIfChain(cand.first, aliases, chain)) استمر;
            AstVarScope* ثابت vscp = chain.compareVscp;
            إذا (!if_Chain_Supported_Transition_عقدة(chain, vscp)) استمر;
            إذا (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processIfChain(chain, vscp, reg);
            } وإلا إذا (vscp != firstCand.stateVscp) {
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
    ساكن فراغ warnComboSameAlways(Ast_عقدة* warnNodep, ثابت FsmCaseCandidate& firstCand) {
        warnNodep->v3warn(FSMMULTI,
                          "FSM coverage: multiple supported transition candidates found in "
                          "the same combinational always block. Only the first candidate "
                          "will be instrumented."
                              << candidateConflictContext(warnNodep, firstCand));
    }

    فراغ processComboAlways(ثابت FsmComboAlways& combo) {
        ثابت ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_عقدة* ثابت stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate firstCand;
        لكل (Ast_عقدة* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ثابت casep = VN_CAST(nodep, Case);
            إذا (!casep) استمر;
            ثابت ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchCase(stmtsp, casep);
            ثابت FsmRegisterCandidate* ثابت matchedp = match.matchedp;
            Ast_عقدة* ثابت matchedWarnNodep = match.warnNodep;
            إذا (!matchedp) استمر;
            إذا (!firstCand.stateVscp) {
                ثابت auto أدرج_Pair = m_comboPaired.emplace(
                    matchedp->stateVscp(),
                    FsmCaseCandidate{matchedWarnNodep,
                                     const_cast<AstVarScope*>(matchedp->stateVscp())});
                إذا (!أدرج_Pair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  أدرج_Pair.first->second));
                    استمر;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processCase(casep, matchedp->nextVscp(), *matchedp);
                استمر;
            }
            إذا (matchedp->stateVscp() != firstCand.stateVscp) {
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
        إذا (firstCand.stateVscp) أرجع;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        لكل (Ast_عقدة* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* ثابت ifp = VN_CAST(nodep, If);
            إذا (!ifp) استمر;
            FsmIfChainCandidate chain;
            ثابت FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            إذا (!collectIfChain(ifp, aliases, chain)) استمر;
            ثابت ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchIfChain(stmtsp, chain);
            ثابت FsmRegisterCandidate* ثابت matchedp = match.matchedp;
            Ast_عقدة* ثابت matchedWarnNodep = match.warnNodep;
            إذا (!matchedp) استمر;
            إذا (!firstCand.stateVscp) {
                ثابت std::pair<std::unordered_map<ثابت AstVarScope*, FsmCaseCandidate>::iterator,
                                منطقي>
                    أدرج_Pair = m_comboPaired.emplace(
                        matchedp->stateVscp(),
                        FsmCaseCandidate{matchedWarnNodep,
                                         const_cast<AstVarScope*>(matchedp->stateVscp())});
                إذا (!أدرج_Pair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  أدرج_Pair.first->second));
                    استمر;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processIfChain(chain, matchedp->nextVscp(), *matchedp);
                استمر;
            }
            إذا (matchedp->stateVscp() != firstCand.stateVscp) {
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
        إذا (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            أرجع;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_oneBlockAlwayss.emplace_back(m_scopep, nodep);
        ثابت RegisterAlwaysAnalyzer analyzer{m_scopep};
        FsmRegisterCandidate reg;
        إذا (analyzer.matchRegisterCandidate(nodep, reg)) {
            AstVarScope* ثابت stateVscp = reg.stateVscp();
            ثابت منطقي found
                = std::any_of(m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                              [stateVscp](ثابت FsmRegisterCandidate& existing) {
                                  أرجع existing.stateVscp() == stateVscp;
                              });
            إذا (!found) { m_registerCandidates.emplace_back(reg); }
        }
        إذا (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            m_comboAlwayss.emplace_back(m_scopep, nodep);
        } وإلا إذا (nodep->keyword() == VAlwaysKwd::ALWAYS) {
            إذا (!nodep->sentreep() || isPlainComboSentree(nodep->sentreep())) {
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
        إذا (m_scopep && fsmRegisterWrapperDesc(nodep)) addWrapperCell(m_scopep, nodep);
        iterateChildren(nodep);
    }

    // Continue the walk through the rest of the design hierarchy.
    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        لكل (ثابت std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            FsmRegisterCandidate reg;
            إذا (matchFsmWrapperCell(wrapperCell.first, wrapperCell.second, reg)) {
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
    size_t m_edges_عدد = 0;  // Total count of edges
    AstVarScope* ثابت m_prevVscp;  // Previous state of fsm
    File_سطر* ثابت m_flp;  // Fileline pointer to use across the builder
    ثابت std::string m_modPrettyName;  // Pretty fsm mode name
    Ast_عقدة_Stmt* m_coverIncp = nullptr;  // Increments proper CoverDecl after each change
    Ast_عقدة_Stmt* m_initialCoverIncp = nullptr;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = nullptr;  // List of CoverDecls
    منطقي m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(ثابت uint64_t قيمة) {
        V3_رقم رقم{static_cast<Ast_عقدة*>(nullptr), m_prevVscp->width()};
        رقم.setQuad(قيمة);
        رقم.isSigned(m_prevVscp->dtypep()->isSigned());
        أرجع جديد AstConst{m_flp, رقم};
    }

    std::string quadToBitStirngLabel(ثابت uint64_t val) {
        صحيح width = m_prevVscp->width();
        UASSERT(width < 64, "We should not expand values that wide");
        std::string result = std::to_string(width) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        طالما (--width >= 0) result.ادفع_back("01"[(val >> width) & 1]);
        أرجع result;
    }

    فراغ initExistingStates() {
        لكل (ثابت V3GraphVertex& vtx : m_graph.vertices()) {
            ثابت FsmVertex* ثابت vertexp = vtx.as<FsmVertex>();
            m_edges_عدد += vertexp->inEdges().size();
            إذا (ثابت FsmStateVertex* ثابت statep = vertexp->cast<FsmStateVertex>()) {
                m_states.ادفع_back(statep);
            } وإلا إذا (vertexp->isDefaultAny()) {
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
        ثابت uint64_t combinations_عدد = static_cast<size_t>(1)
                                           << m_graph.sampleVarScopep()->width();
        // Rolling trick allows to cut peak memory usage
        منطقي undefRolled = خطأ;
        auto rollUndefs = [هذا, combinations_عدد] {
            لكل (uint64_t i = 0; i < combinations_عدد; ++i) {
                ثابت auto أدرج = m_undefinedStates.أدرج(i);
                إذا (!أدرج.second) m_undefinedStates.erase(أدرج.first);
            }
        };
        لكل (ثابت FsmStateVertex* ثابت statep : m_states) {
            ثابت V3_رقم& رقم = statep->قيمة().رقم();
            ثابت uint64_t قيمة = رقم.toUQuad();
            إذا (!m_isComplete) {
                إذا (undefRolled) {
                    UASSERT_OBJ(m_undefinedStates.erase(قيمة) == 1, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                } وإلا {
                    UASSERT_OBJ(m_undefinedStates.أدرج(قيمة).second, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                    إذا (m_undefinedStates.size() * 2 >= combinations_عدد) {
                        undefRolled = true;
                        rollUndefs();
                    }
                }
            }
        }
        إذا (!(m_isComplete || undefRolled)) rollUndefs();
    }

    Ast_عقدة_تعبير* buildResetCond() ثابت {
        Ast_عقدة_تعبير* ثابت refp
            = جديد AstVarRef{m_flp, m_graph.resetCond().varScopep, VAccess::READ};
        أرجع m_graph.resetCond().active_أدنى
                   ? static_cast<Ast_عقدة_تعبير*>(جديد AstLogNot{m_flp, refp})
                   : refp;
    }

    size_t get_Arc_عدد_With_Expansion(ثابت VFsmExpandType expand) ثابت {
        size_t arc_عدد_With_Expansion = ~0ULL;
        بدل (expand) {
        حالة VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_graph.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            اكسر;
        }
        حالة VFsmExpandType::AUTO: {
            arc_عدد_With_Expansion = m_edges_عدد;
            إذا (m_defaultVertexp) {
                UASSERT_OBJ(m_states.size() >= m_defaultVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arc_عدد_With_Expansion -= m_defaultVertexp->outEdges().size();
                لكل (ثابت FsmStateVertex* ثابت statep : m_states) {
                    arc_عدد_With_Expansion += static_cast<size_t>(statep->outEdges().empty())
                                             * m_defaultVertexp->outEdges().size();
                }
            }
            إذا (m_resetVertexp) {
                UASSERT_OBJ(m_states.size() >= m_resetVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arc_عدد_With_Expansion += m_states.size() - m_resetVertexp->outEdges().size();
            }
            اكسر;
        }
        حالة VFsmExpandType::AUTO_EXPAND: {
            arc_عدد_With_Expansion = m_states.size() * m_states.size() - m_states.size();
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
            إذا (width < 32) {
                ثابت uint64_t combinations_عدد = static_cast<size_t>(1) << width;
                arc_عدد_With_Expansion = combinations_عدد * combinations_عدد - combinations_عدد;
            }
            اكسر;
        }
        }
        أرجع arc_عدد_With_Expansion;
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
        declp->perInstance(true);
        m_declsp = Ast_عقدة::addNext(m_declsp, declp);
        أرجع declp;
    }

    Ast_عقدة_Stmt* buildCoverIncStmtp(ثابت std::string& fromStateLabel,
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
        declp->perInstance(true);
        m_declsp = Ast_عقدة::addNext(m_declsp, declp);
        أرجع جديد AstCoverInc{m_flp, declp};
    }
    Ast_عقدة_Stmt* buildExpandedTransitionHandler(ثابت std::string& toStateLabel,
                                                ثابت منطقي considerUndefs,
                                                ثابت منطقي expandOnlyWithTerminalVerticies,
                                                ثابت FsmStateVertex* ثابت toStatep = nullptr) {
        AstCase* ثابت arcCasep = جديد AstCase{
            m_flp, VCaseType::CT_CASE, جديد AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        std::unordered_set<ثابت FsmStateVertex*> handledSoureces;
        إذا (toStatep) {
            لكل (ثابت V3GraphEdge& edge : toStatep->inEdges()) {
                إذا (ثابت FsmStateVertex* ثابت fromp = edge.fromp()->cast<FsmStateVertex>()) {
                    إذا (!handledSoureces.emplace(fromp).second) استمر;
                    arcCasep->addItemsp(جديد AstCaseItem{
                        m_flp, makeConst(fromp->قيمة().رقم().toUQuad()),
                        (fromp != toStatep) ? buildCoverIncStmtp(fromp->label(), toStateLabel)
                                            : nullptr});
                }
            }
        }
        لكل (ثابت FsmStateVertex* ثابت fromp : m_states) {
            إذا (handledSoureces.find(fromp) != handledSoureces.end()) استمر;
            arcCasep->addItemsp(جديد AstCaseItem{
                m_flp, makeConst(fromp->قيمة().رقم().toUQuad()),
                (fromp != toStatep
                 && (!expandOnlyWithTerminalVerticies || fromp->outEdges().empty()))
                    ? buildCoverIncStmtp(fromp->label(), toStateLabel, "artificial")
                    : nullptr});
        }
        إذا (!m_isComplete) {
            إذا (considerUndefs) {
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
    Ast_عقدة_Stmt* buildUnxpandedTransitionHandler(ثابت FsmStateVertex* ثابت toStatep) {
        AstCase* ثابت arcCasep = جديد AstCase{
            m_flp, VCaseType::CT_CASE, جديد AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        لكل (ثابت V3GraphEdge& edge : toStatep->inEdges()) {
            ثابت FsmArcEdge* ثابت arcp = edge.as<FsmArcEdge>();
            UASSERT_OBJ(!arcp->isReset(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            ثابت FsmStateVertex* ثابت fromp = arcp->fromp()->as<FsmStateVertex>();
            إذا (fromp != toStatep) {
                arcCasep->addItemsp(
                    جديد AstCaseItem{m_flp, makeConst(fromp->قيمة().رقم().toUQuad()),
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
            ثابت size_t arc_عدد_With_Expansion = get_Arc_عدد_With_Expansion(expand);
            إذا (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arc_عدد_With_Expansion == 0ULL
                            || arc_عدد_With_Expansion
                                   > static_cast<size_t>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arc_عدد_With_Expansion);
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
        إذا (!expandAll) {
            إذا (m_defaultVertexp) {
                لكل (ثابت V3GraphEdge& edge : m_defaultVertexp->outEdges()) {
                    defaultTargets.emplace(edge.قمة()->as<FsmStateVertex>());
                }
            }
            إذا (m_resetVertexp) {
                لكل (ثابت V3GraphEdge& edge : m_resetVertexp->outEdges()) {
                    resetTargets.emplace(edge.قمة()->as<FsmStateVertex>());
                }
            }
        }
        ثابت منطقي isFullExpansion = expand == VFsmExpandType::FULL;
        إذا (isFullExpansion) initUndefinedStates();
        لكل (ثابت FsmStateVertex* ثابت statep : m_states) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* ثابت declp = buildStateCoverDecl(statep->label());
            initialStateCasep->addItemsp(
                جديد AstCaseItem{m_flp, makeConst(statep->قيمة().رقم().toUQuad()),
                                جديد AstCoverInc{m_flp, declp, true}});
            ثابت منطقي expandAllOrReset
                = expandAll || (resetTargets.find(statep) != resetTargets.end());
            ثابت منطقي defaultExpantion
                = !expandAllOrReset && (defaultTargets.find(statep) != defaultTargets.end());
            arcCasep->addItemsp(جديد AstCaseItem{
                m_flp, makeConst(statep->قيمة().رقم().toUQuad()),
                Ast_عقدة::addNext<Ast_عقدة_Stmt>(
                    جديد AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), isFullExpansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        إذا (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            إذا (isFullExpansion) {
                لكل (ثابت uint64_t قيمة : m_undefinedStates) {
                    ثابت std::string& label = quadToBitStirngLabel(قيمة);
                    AstCoverOtherDecl* ثابت declp = buildStateCoverDecl(label);
                    initialStateCasep->addItemsp(جديد AstCaseItem{
                        m_flp, makeConst(قيمة), جديد AstCoverInc{m_flp, declp, true}});
                    arcCasep->addItemsp(
                        جديد AstCaseItem{m_flp, makeConst(قيمة),
                                        Ast_عقدة::addNext<Ast_عقدة_Stmt>(
                                            جديد AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, true, خطأ))});
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
    Ast_عقدة_Cover_Decl* getCoverDeclsp() ثابت { أرجع m_declsp; }
    Ast_عقدة_Stmt* getCoverIncp() ثابت { أرجع m_coverIncp; }
    Ast_عقدة_Stmt* getInitialCoverIncp() ثابت { أرجع m_initialCoverIncp; }
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
    ساكن Ast_Sen_شجرة* build_Sen_شجرة(File_سطر* flp, ثابت std::vector<FsmSenDesc>& senses) {
        Ast_Sen_شجرة* ثابت sentreep = جديد Ast_Sen_شجرة{flp, nullptr};
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
        File_سطر* ثابت flp = graph.fileline();
        Ast_عقدة_Module* ثابت modp = scopep->modp();
        AstVarScope* ثابت prevVscp
            = m_prevTmps.make(flp, scopep, sampleVscp->dtypep(), stateVscp->varp()->shortName());
        // The saved previous-state temp crosses the scheduler's pre/post split
        // in the same way as Verilator's built-in NBA shadow variables, so keep
        // both vars marked as post-life participants for stable MT ordering.
        stateVscp->optimizeLifePost(true);
        sampleVscp->optimizeLifePost(true);
        prevVscp->optimizeLifePost(true);

        AstActive* ثابت initActivep
            = جديد AstActive{flp, "fsm-coverage-init",
                            جديد Ast_Sen_شجرة{flp, جديد AstSenItem{flp, AstSenItem::Initial{}}}};
        initActivep->sen_شجرة_Storep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->addStmtsp(جديد AstInitialStatic{
            flp, جديد AstAssign{flp, جديد AstVarRef{flp, prevVscp, VAccess::WRITE},
                               جديد AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->addBlocksp(initActivep);

        AstAlwaysPost* ثابت covPostp = جديد AstAlwaysPost{flp};
        منطقي updatePrevAfterPost = خطأ;
        إذا (alwaysp) {
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
            updatePrevAfterPost = true;
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

        إذا (updatePrevAfterPost) {
            covPostp->addStmtsp(جديد AstAssign{flp, جديد AstVarRef{flp, prevVscp, VAccess::WRITE},
                                              جديد AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        Ast_Sen_شجرة* ثابت sentreep = build_Sen_شجرة(flp, graph.senses());
        AstActive* ثابت activep = جديد AstActive{flp, "fsm-coverage", sentreep};
        activep->sen_شجرة_Storep(sentreep);
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
    إذا (dumpGraphLevel() >= 6) {
        size_t index = 0;
        لكل (ثابت DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dumpTag(index++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { FsmLowerVisitor lower{state}; }
    V3Global::dump_Check_Global_شجرة("fsm-detect", 0, dump_شجرة_Either_Level() >= 3);
}
