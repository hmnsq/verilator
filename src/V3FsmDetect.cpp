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

#समावेश "V3PchAstNoMT.h"

#समावेश "V3FsmDetect.h"

#समावेश "V3Ast.h"
#समावेश "V3Control.h"
#समावेश "V3Graph.h"
#समावेश "V3SharedTmps.h"

#समावेश <algorithm>
#समावेश <cctype>
#समावेश <map>
#समावेश <memory>
#समावेश <unordered_map>
#समावेश <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

namespace {

// Width-preserving FSM state identity. FSM detection needs a stable key for
// graph vertices and lookup tables, but lowering still needs the original
// folded Verilog value so emitted comparisons keep the correct width and bits.
श्रेणी Fsm_State_मूल्य final {
    // Hash/equality key only. It deliberately ignores signedness because
    // signed and unsigned constants with the same width and bits denote the
    // same encoded FSM state.
    string m_कुंजी;  // Canonical "width:value" identity, independent of signedness

    // Semantic value. This is what diagnostics and lowering use when printing
    // values or rebuilding AstConst nodes for instrumentation.
    V3_संख्या m_अंक;  // Original folded value, preserving width for lowered comparisons

    अटल string make_कुंजी(स्थिर V3_संख्या& अंक) {
        V3_संख्या कुंजी_अंक = अंक;
        // Signedness does not change FSM state identity: same width and bits
        // should address the same graph vertex.
        कुंजी_अंक.isSigned(false);
        वापस cvtToStr(कुंजी_अंक.width()) + ":" + कुंजी_अंक.ascii(true, true);
    }

खुला:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    Fsm_State_मूल्य()
        : m_कुंजी{"1:1'h0"}
        , m_अंक{static_cast<Ast_गाँठ*>(nullptr), 1, 0} {}
    explicit Fsm_State_मूल्य(स्थिर V3_संख्या& अंक)
        : m_कुंजी{make_कुंजी(अंक)}
        , m_अंक{अंक} {}

    स्थिर string& कुंजी() स्थिर { वापस m_कुंजी; }
    स्थिर V3_संख्या& अंक() स्थिर { वापस m_अंक; }
    string ascii() स्थिर { वापस m_अंक.ascii(true, true); }
    string warnText() स्थिर {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        अगर (m_अंक.width() <= 32) वापस cvtToStr(m_अंक.toUInt());
        वापस ascii();
    }

    bool चालक==(स्थिर Fsm_State_मूल्य& rhs) स्थिर { वापस m_कुंजी == rhs.m_कुंजी; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
काष्ठा Fsm_State_मूल्य_Hash final {
    माप_प्रकार चालक()(स्थिर Fsm_State_मूल्य& मूल्य) स्थिर {
        वापस std::hash<string>{}(मूल्य.कुंजी());
    }
};

// Captures one sensitivity-list entry so the lowering phase can later rebuild
// an active block with the same triggering event control.
काष्ठा FsmSenDesc final {
    // Encoded edge kind copied from AstSenItem::edgeType() so lowering can
    // rebuild the same trigger semantics on the synthesized coverage block.
    VEdgeType::en edgeType = static_cast<VEdgeType::en>(0);
    // Triggering signal in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
};

// Captures the simple reset predicate shape that survives to this pass after
// earlier normalization so reset arcs can be reconstructed during lowering.
काष्ठा FsmResetCondDesc final {
    // Reset signal used by the FSM in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
    bool active_निचला = false;
};

श्रेणी FsmResetArcDesc final {
    Fsm_State_मूल्य m_to_मूल्य;  // Encoded reset target state.
    Ast_गाँठ* m_nodep = nullptr;  // Source node for warnings and emitted metadata.
    Ast_गाँठ_व्यंजक* m_valuep = nullptr;  // Expression that provided the reset value.

खुला:
    FsmResetArcDesc() = शेष;
    FsmResetArcDesc(Fsm_State_मूल्य to_मूल्य, Ast_गाँठ_Assign* nodep)
        : m_to_मूल्य{to_मूल्य}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(Fsm_State_मूल्य to_मूल्य, Ast_गाँठ* nodep, Ast_गाँठ_व्यंजक* valuep)
        : m_to_मूल्य{to_मूल्य}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    Fsm_State_मूल्य to_मूल्य() स्थिर { वापस m_to_मूल्य; }
    Ast_गाँठ* nodep() स्थिर { वापस m_nodep; }
    Ast_गाँठ_व्यंजक* valuep() स्थिर { वापस m_valuep; }
};

काष्ठा FsmWrapperRoles final {
    string dPort;
    string qPort;
    string clkPort;
    string rstPort;
    string rstValParam;
    bool has_Rst_Active_निचला = false;
    bool rst_Active_निचला = false;
};

अटल bool fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, स्थिर string& portName,
                                                  bool& active_निचला) {
    bool matched = false;
    cellp->modp()->foreach([&](AstSenItem* itemp) {
        Ast_गाँठ_Var_Ref* स्थिर vrefp = itemp->varrefp();
        अगर (!vrefp) वापस;
        अगर (vrefp->varp()->name() != portName) वापस;
        active_निचला = itemp->edgeType() == VEdgeType::ET_NEGEDGE;
        matched = true;
    });
    वापस matched;
}

अटल स्थिर V3Control::FsmRegisterWrapper* fsmRegisterWrapperDesc(AstCell* cellp) {
    Ast_गाँठ_Module* स्थिर modp = cellp->modp();
    स्थिर string origName = modp->origName();
    अगर (स्थिर V3Control::FsmRegisterWrapper* स्थिर descp
        = V3Control::getFsmRegisterWrapper(origName)) {
        वापस descp;
    }
    वापस V3Control::getFsmRegisterWrapper(modp->prettyDehashOrigOrName());
}

अटल FsmWrapperRoles rolesFromDesc(स्थिर V3Control::FsmRegisterWrapper& desc) {
    FsmWrapperRoles roles;
    roles.dPort = desc.d;
    roles.qPort = desc.q;
    roles.clkPort = desc.घड़ी;
    roles.rstPort = desc.reset;
    roles.rstValParam = desc.reset_मूल्य;
    वापस roles;
}

श्रेणी FsmRegisterCandidate final {
    AstScope* m_scopep = nullptr;  // Owning scope for the paired FSM.
    AstAlways* m_alwaysp = nullptr;  // Register process that commits the state.
    AstVarScope* m_stateVscp = nullptr;  // Registered FSM state variable.
    AstVarScope* m_sampleVscp = nullptr;  // Variable sampled by lowered coverage logic.
    AstVarScope* m_अग्र_Vscp = nullptr;  // Next-state variable or same state var for 1-block FSMs.
    std::vector<FsmSenDesc> m_senses;  // Event controls for recreated coverage blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate, if any.
    std::vector<FsmResetArcDesc> m_resetArcs;  // Reset target arcs recovered during detect.
    bool m_hasResetCond = false;  // Whether the FSM had a modeled reset predicate.
    bool m_inclCond = false;  // Whether conditional/default arcs are kept explicitly.
    File_पंक्ति* m_flp = nullptr;  // Representative source location.

खुला:
    AstScope* scopep() स्थिर { वापस m_scopep; }
    व्योम scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* alwaysp() स्थिर { वापस m_alwaysp; }
    व्योम alwaysp(AstAlways* alwaysp) { m_alwaysp = alwaysp; }
    AstVarScope* stateVscp() स्थिर { वापस m_stateVscp; }
    व्योम stateVscp(AstVarScope* vscp) { m_stateVscp = vscp; }
    AstVarScope* sampleVscp() स्थिर { वापस m_sampleVscp ? m_sampleVscp : m_stateVscp; }
    व्योम sampleVscp(AstVarScope* vscp) { m_sampleVscp = vscp; }
    AstVarScope* अग्र_Vscp() स्थिर { वापस m_अग्र_Vscp; }
    व्योम अग्र_Vscp(AstVarScope* vscp) { m_अग्र_Vscp = vscp; }
    स्थिर std::vector<FsmSenDesc>& senses() स्थिर { वापस m_senses; }
    std::vector<FsmSenDesc>& senses() { वापस m_senses; }
    स्थिर FsmResetCondDesc& resetCond() स्थिर { वापस m_resetCond; }
    FsmResetCondDesc& resetCond() { वापस m_resetCond; }
    स्थिर std::vector<FsmResetArcDesc>& resetArcs() स्थिर { वापस m_resetArcs; }
    std::vector<FsmResetArcDesc>& resetArcs() { वापस m_resetArcs; }
    bool hasResetCond() स्थिर { वापस m_hasResetCond; }
    व्योम hasResetCond(bool flag) { m_hasResetCond = flag; }
    bool inclCond() स्थिर { वापस m_inclCond; }
    व्योम inclCond(bool flag) { m_inclCond = flag; }
    File_पंक्ति* fileline() स्थिर { वापस m_flp; }
    व्योम fileline(File_पंक्ति* flp) { m_flp = flp; }
};

श्रेणी FsmComboAlways final {
    AstScope* स्थिर m_scopep = nullptr;  // Owning scope for the combinational process.
    AstAlways* स्थिर m_alwaysp = nullptr;  // Candidate transition process.

खुला:
    FsmComboAlways() = शेष;
    FsmComboAlways(AstScope* scopep, AstAlways* alwaysp)
        : m_scopep{scopep}
        , m_alwaysp{alwaysp} {}

    AstScope* scopep() स्थिर { वापस m_scopep; }
    AstAlways* alwaysp() स्थिर { वापस m_alwaysp; }
};

श्रेणी FsmGraph;

श्रेणी FsmVertex VL_NOT_FINAL : खुला V3GraphVertex {
    VL_RTTI_IMPL(FsmVertex, V3GraphVertex)

खुला:
    क्रमागत श्रेणी Kind : uint8_t { STATE, RESET_ANY, DEFAULT_ANY };

निजी:
    Kind m_kind;  // State vs synthetic ANY/default vertex role.
    string m_label;  // User-facing state or pseudo-state label.
    Fsm_State_मूल्य m_मूल्य;  // Encoded state value for real state vertices.

रक्षित:
    FsmVertex(V3Graph* graphp, Kind kind, string label, Fsm_State_मूल्य मूल्य) VL_MT_DISABLED
        : V3GraphVertex{graphp},
          m_kind{kind},
          m_label{label},
          m_मूल्य{मूल्य} {}
    ~FsmVertex() override = शेष;

खुला:
    Kind kind() स्थिर { वापस m_kind; }
    bool isState() स्थिर { वापस m_kind == Kind::STATE; }
    bool isResetAny() स्थिर { वापस m_kind == Kind::RESET_ANY; }
    bool isDefaultAny() स्थिर { वापस m_kind == Kind::DEFAULT_ANY; }
    स्थिर string& label() स्थिर { वापस m_label; }
    स्थिर Fsm_State_मूल्य& मूल्य() स्थिर { वापस m_मूल्य; }

    string name() स्थिर override VL_MT_SAFE { वापस m_label + "=" + m_मूल्य.ascii(); }
};

श्रेणी FsmStateVertex final : खुला FsmVertex {
    VL_RTTI_IMPL(FsmStateVertex, FsmVertex)

खुला:
    FsmStateVertex(V3Graph* graphp, string label, Fsm_State_मूल्य मूल्य) VL_MT_DISABLED
        : FsmVertex{graphp, Kind::STATE, label, मूल्य} {}
    ~FsmStateVertex() override = शेष;

    string dotColor() स्थिर override { वापस "lightblue"; }
    string dotShape() स्थिर override { वापस "ellipse"; }
};

श्रेणी FsmPseudoVertex final : खुला FsmVertex {
    VL_RTTI_IMPL(FsmPseudoVertex, FsmVertex)

खुला:
    FsmPseudoVertex(V3Graph* graphp, Kind kind, string label) VL_MT_DISABLED
        : FsmVertex{graphp, kind, label, Fsm_State_मूल्य{}} {}
    ~FsmPseudoVertex() override = शेष;

    string name() स्थिर override VL_MT_SAFE { वापस label(); }
    string dotColor() स्थिर override { वापस isResetAny() ? "darkgreen" : "orange"; }
    string dotShape() स्थिर override { वापस "diamond"; }
};

श्रेणी FsmArcEdge final : खुला V3GraphEdge {
    VL_RTTI_IMPL(FsmArcEdge, V3GraphEdge)
    bool m_isReset = false;  // Arc originates from the synthetic reset source.
    bool m_isCond = false;  // Arc came from a conditional next-state split.
    bool m_isDefault = false;  // Arc represents a case default source.
    File_पंक्ति* m_flp = nullptr;  // Source location for emitted coverage metadata.

खुला:
    FsmArcEdge(V3Graph* graphp, FsmVertex* fromp, FsmStateVertex* शीर्ष, bool isReset, bool isCond,
               bool isDefault, File_पंक्ति* flp) VL_MT_DISABLED : V3GraphEdge{graphp, fromp, शीर्ष, 1},
                                                               m_isReset{isReset},
                                                               m_isCond{isCond},
                                                               m_isDefault{isDefault},
                                                               m_flp{flp} {}
    ~FsmArcEdge() override = शेष;

    bool isReset() स्थिर { वापस m_isReset; }
    bool isCond() स्थिर { वापस m_isCond; }
    bool isDefault() स्थिर { वापस m_isDefault; }
    File_पंक्ति* fileline() स्थिर { वापस m_flp; }

    string dotLabel() स्थिर override {
        अगर (m_isReset) वापस "reset";
        अगर (m_isDefault) वापस "default";
        अगर (m_isCond) वापस "cond";
        वापस "";
    }
    string dotColor() स्थिर override {
        अगर (m_isReset) वापस "darkgreen";
        अगर (m_isDefault) वापस "orange";
        अगर (m_isCond) वापस "blue";
        वापस "black";
    }
};

// One graph per detected FSM. Graph-level metadata captures the non-graph
// context needed to lower states/arcs back into the AST after detection.
श्रेणी FsmGraph final : खुला V3Graph {
    AstScope* m_scopep = nullptr;  // Owning scoped block for the detected FSM.
    AstAlways* m_stateAlwaysp = nullptr;  // Register always block being instrumented.
    string m_stateVarName;  // Pretty state variable name for user-visible output.
    string m_stateVarInternalName;  // Internal state symbol name for dump tags.
    AstVarScope* m_stateVarScopep = nullptr;  // Scoped state variable being tracked.
    AstVarScope* m_sampleVarScopep = nullptr;  // Scoped variable sampled by coverage logic.
    std::vector<FsmSenDesc> m_senses;  // Saved event controls for recreated active blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate shape, if one exists.
    bool m_hasResetCond = false;  // Whether the detected FSM had a reset branch.
    bool m_inclCond = false;  // Whether conditional arcs should be kept explicitly.
    File_पंक्ति* m_flp = nullptr;  // Representative source location for declarations/arcs.
    std::unordered_map<Fsm_State_मूल्य, FsmStateVertex*, Fsm_State_मूल्य_Hash>
        m_stateVertices;  // Value to state map.
    FsmPseudoVertex* m_resetVertexp = nullptr;  // Synthetic ANY source for reset arcs.
    FsmPseudoVertex* m_defaultVertexp = nullptr;  // Synthetic default source for case defaults.

खुला:
    FsmGraph() VL_MT_DISABLED
        : m_resetVertexp{नया FsmPseudoVertex{यह, FsmVertex::Kind::RESET_ANY, "ANY"}},
          m_defaultVertexp{नया FsmPseudoVertex{यह, FsmVertex::Kind::DEFAULT_ANY, "default"}} {}

    AstScope* scopep() स्थिर { वापस m_scopep; }
    व्योम scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* stateAlwaysp() स्थिर { वापस m_stateAlwaysp; }
    व्योम stateAlwaysp(AstAlways* alwaysp) { m_stateAlwaysp = alwaysp; }
    स्थिर string& stateVarName() स्थिर { वापस m_stateVarName; }
    व्योम stateVarName(स्थिर string& name) { m_stateVarName = name; }
    स्थिर string& stateVarInternalName() स्थिर { वापस m_stateVarInternalName; }
    व्योम stateVarInternalName(स्थिर string& name) { m_stateVarInternalName = name; }
    AstVarScope* stateVarScopep() स्थिर { वापस m_stateVarScopep; }
    व्योम stateVarScopep(AstVarScope* vscp) { m_stateVarScopep = vscp; }
    AstVarScope* sampleVarScopep() स्थिर {
        वापस m_sampleVarScopep ? m_sampleVarScopep : m_stateVarScopep;
    }
    व्योम sampleVarScopep(AstVarScope* vscp) { m_sampleVarScopep = vscp; }
    स्थिर std::vector<FsmSenDesc>& senses() स्थिर { वापस m_senses; }
    std::vector<FsmSenDesc>& senses() { वापस m_senses; }
    स्थिर FsmResetCondDesc& resetCond() स्थिर { वापस m_resetCond; }
    FsmResetCondDesc& resetCond() { वापस m_resetCond; }
    bool hasResetCond() स्थिर { वापस m_hasResetCond; }
    व्योम hasResetCond(bool flag) { m_hasResetCond = flag; }
    bool inclCond() स्थिर { वापस m_inclCond; }
    व्योम inclCond(bool flag) { m_inclCond = flag; }
    File_पंक्ति* fileline() स्थिर { वापस m_flp; }
    व्योम fileline(File_पंक्ति* flp) { m_flp = flp; }

    FsmStateVertex* addStateVertex(string label, Fsm_State_मूल्य मूल्य) VL_MT_DISABLED {
        FsmStateVertex* स्थिर vertexp = नया FsmStateVertex{यह, label, मूल्य};
        m_stateVertices.emplace(मूल्य, vertexp);
        वापस vertexp;
    }
    FsmPseudoVertex* resetAnyVertex() VL_MT_DISABLED { वापस m_resetVertexp; }
    FsmPseudoVertex* defaultAnyVertex() VL_MT_DISABLED { वापस m_defaultVertexp; }
    FsmArcEdge* addArc(Fsm_State_मूल्य from_मूल्य, Fsm_State_मूल्य to_मूल्य, bool isReset, bool isCond,
                       bool isDefault, File_पंक्ति* flp) VL_MT_DISABLED {
        FsmStateVertex* स्थिर शीर्ष = m_stateVertices.at(to_मूल्य);
        FsmVertex* fromp = nullptr;
        अगर (isReset) {
            fromp = resetAnyVertex();
        } अन्यथा अगर (isDefault) {
            fromp = defaultAnyVertex();
        } अन्यथा {
            fromp = m_stateVertices.at(from_मूल्य);
        }
        वापस नया FsmArcEdge{यह, fromp, शीर्ष, isReset, isCond, isDefault, flp};
    }

    string name() स्थिर VL_MT_SAFE {
        वापस "FSM "
               + (m_stateVarName.empty() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dumpTag(माप_प्रकार सूचकांक_2) स्थिर {
        string tag = stateVarInternalName();
        क्रम (अक्षर& ch : tag) {
            अगर (!std::है_अक्षर_अंक(static_cast<अचिन्हित अक्षर>(ch))) ch = '_';
        }
        वापस "fsm_" + cvtToStr(सूचकांक_2) + "_" + tag;
    }
};

काष्ठा DetectedFsm final {
    std::unique_ptr<FsmGraph> graphp;  // Extracted graph for one detected FSM candidate.
};

काष्ठा FsmCaseCandidate final {
    Ast_गाँठ* warnNodep = nullptr;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = nullptr;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
काष्ठा FsmStateComparison final {
    AstVarScope* stateVscp = nullptr;  // Compared state variable
    Ast_गाँठ_व्यंजक* valuep = nullptr;  // Compared constant value expression
    Fsm_State_मूल्य मूल्य;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
काष्ठा FsmIfBranch final {
    AstIf* ifp = nullptr;  // Source if/else-if node
    Ast_गाँठ* stmtsp = nullptr;  // Branch body
    Ast_गाँठ_व्यंजक* valuep = nullptr;  // Source state value expression
    Fsm_State_मूल्य from_मूल्य;  // Encoded source state value
    bool has_शीर्ष_Guard = false;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
काष्ठा FsmIfChainCandidate final {
    AstIf* ifp = nullptr;  // Top-level if-chain node
    AstVarScope* compareVscp = nullptr;  // Variable used by every state comparison
    std::vector<FsmIfBranch> branches;  // Recognized state-dispatch branches
    Ast_गाँठ* defaultStmtsp = nullptr;  // Optional final else body
};

// Aliases are accepted only when they are equivalent to spelling the state
// comparison inline; this avoids inferring FSM semantics from arbitrary logic.
using FsmAliasMap = std::unordered_map<स्थिर AstVarScope*, FsmStateComparison>;
using FsmCellPortMap = std::unordered_map<string, AstVarScope*>;
using FsmCellPortAliasMap = std::unordered_map<स्थिर AstCell*, FsmCellPortMap>;

काष्ठा StateConstLabel final {
    string text;
    bool fromParam = false;
    माप_प्रकार state_सूचकांक = 0;
};

काष्ठा FsmStateSpace final {
    std::vector<std::pair<string, Fsm_State_मूल्य>> states;  // User label and encoded value
    std::unordered_map<Fsm_State_मूल्य, StateConstLabel, Fsm_State_मूल्य_Hash>
        labels;  // Encoded value to label
    AstVar* stateVarp = nullptr;  // Tracked FSM state variable
    bool enumBacked = false;  // Whether states came from an enum declaration
};

// Local shared state between the two adjacent FSM coverage phases. Detection
// fills this with recovered FSM graphs; lowering consumes the completed graphs
// immediately afterward without needing any AST serialization bridge.
श्रेणी FsmState final {
    // All detected FSMs in discovery order. This is the only bridge between
    // the adjacent detect and lower phases, so the second phase never needs to
    // rediscover or serialize the extracted machine.
    std::vector<DetectedFsm> m_fsms;
    std::map<स्थिर AstVarScope*, माप_प्रकार> m_fsm_सूचकांक;

खुला:
    DetectedFsm& fsmFor(AstVarScope* stateVscp) {
        स्थिर std::map<स्थिर AstVarScope*, माप_प्रकार>::const_iterator it = m_fsm_सूचकांक.find(stateVscp);
        अगर (it != m_fsm_सूचकांक.end()) वापस m_fsms.at(it->second);
        स्थिर माप_प्रकार सूचकांक_2 = m_fsms.आकार();
        m_fsm_सूचकांक.emplace(stateVscp, सूचकांक_2);
        m_fsms.emplace_back();
        वापस m_fsms.back();
    }
    स्थिर std::vector<DetectedFsm>& fsms() स्थिर { वापस m_fsms; }
};

// Detection runs while the original clocked/case structure is still intact and
// populates graph-backed FSM models without mutating the tree mid-traversal.
// This pass is intentionally conservative: for this PR we only lock down the
// small set of transition/selector forms that are already stable in the
// normalized AST we see here. The remaining reject branches are therefore
// mostly future-feature boundaries, not accidental dead code.
श्रेणी FsmDetectVisitor final : खुला VNVisitor {
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
    std::unordered_map<स्थिर AstVarScope*, FsmCaseCandidate> m_comboPaired;
    // Continuous aliases are order-independent, while procedural aliases must
    // remain source-order scoped to avoid using assignments not yet executed.
    FsmAliasMap m_stateAliases;
    std::unordered_set<स्थिर AstVarScope*> m_ambiguousStateAliases;
    // A surviving wrapper's semantic d/q relationship is split across the
    // parent scope and the child module scope. This table is the narrow bridge
    // between those scopes: only transparent port aliases are recorded, so the
    // detector does not become a general cross-module dataflow engine.
    FsmCellPortAliasMap m_cellPortAliases;
    FsmCellPortAliasMap m_cellPortChildAliases;

    // METHODS
    // Enum-backed FSMs may be wrapped in refs/typedefs; normalize to the
    // underlying enum type before deciding whether a case is a candidate.
    अटल Ast_गाँठ_DType* unwrapEnumCandidate(Ast_गाँठ_DType* dtypep) {
        वापस dtypep->skipRefToEnump();
    }

    अटल string candidateConflictContext(Ast_गाँठ* laterNodep,
                                           स्थिर FsmCaseCandidate& firstCand) {
        वापस '\n' + laterNodep->warnContextPrimary() + firstCand.warnNodep->warnOther()
               + "... Location of first supported candidate for "
               + firstCand.stateVscp->prettyNameQ() + '\n'
               + firstCand.warnNodep->warnContextSecondary();
    }

    अटल bool rejectFsmWrapperCell(AstCell* cellp, स्थिर string& reason) {
        cellp->v3warn(COVERIGN, "Ignoring unsupported: " + reason);
        वापस false;
    }

    अटल bool simple_Param_State_मूल्य(AstCell* cellp, स्थिर string& name, Fsm_State_मूल्य& मूल्य,
                                      Ast_गाँठ_व्यंजक*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = nullptr;
        क्रम (Ast_गाँठ* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* स्थिर varp = VN_CAST(stmtp, Var);
            अगर (!varp || !varp->isParam() || varp->name() != name) जारी;
            valuepr = VN_AS(varp->valuep(), गाँठ_व्यंजक);
            वापस const_मूल्य_Status(valuepr, मूल्य) == Const_मूल्य_Status::OK;
        }
        वापस false;
    }

    अटल bool childPortInScope(AstVarScope* vscp, AstScope* parentScopep, AstCell*& cellpr) {
        अगर (!vscp->varp()->isIO()) वापस false;
        AstScope* स्थिर scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        अगर (scopep->aboveScopep() != parentScopep) वापस false;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        वापस true;
    }

    अटल AstVarScope* simpleAssignVarScope(Ast_गाँठ_व्यंजक* exprp) {
        AstVarRef* स्थिर vrefp = VN_CAST(exprp, VarRef);
        वापस vrefp ? vrefp->varScopep() : nullptr;
    }

    व्योम addWrapperCell(AstScope* scopep, AstCell* cellp) {
        m_cellPortAliases.emplace(cellp, FsmCellPortMap{});
        m_cellPortChildAliases.emplace(cellp, FsmCellPortMap{});
        स्थिर std::pair<AstScope*, AstCell*> item{scopep, cellp};
        अगर (std::find(m_wrapperCells.cbegin(), m_wrapperCells.cend(), item)
            != m_wrapperCells.cend()) {
            वापस;
        }
        m_wrapperCells.emplace_back(item);
    }

    व्योम collectCellPortAlias(AstAssignW* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* स्थिर lhsVscp = simpleAssignVarScope(nodep->lhsp());
        AstVarScope* स्थिर rhsVscp = simpleAssignVarScope(nodep->rhsp());
        अगर (!lhsVscp || !rhsVscp) वापस;
        AstCell* cellp = nullptr;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        अगर (childPortInScope(lhsVscp, m_scopep, cellp)) {
            अगर (!fsmRegisterWrapperDesc(cellp)) वापस;
            UASSERT_OBJ(lhsVscp->varp()->is_निवेश(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cellPortAliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cellPortChildAliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            addWrapperCell(m_scopep, cellp);
        } अन्यथा अगर (childPortInScope(rhsVscp, m_scopep, cellp)) {
            अगर (!fsmRegisterWrapperDesc(cellp)) वापस;
            UASSERT_OBJ(rhsVscp->varp()->isWritable(), nodep,
                        "Child-side port alias rhs should be writable");
            m_cellPortAliases[cellp][rhsVscp->varp()->name()] = lhsVscp;
            m_cellPortChildAliases[cellp][rhsVscp->varp()->name()] = rhsVscp;
            addWrapperCell(m_scopep, cellp);
        }
    }

    AstVarScope* roleVarScope(AstCell* cellp, स्थिर string& portName) स्थिर {
        // At this point explicit AstPin expressions have been lowered away, so
        // role resolution crosses the wrapper boundary only through the
        // transparent alias table above. This keeps wrapper support aligned with
        // direct-register detection instead of growing into interprocedural FSM
        // inference.
        स्थिर FsmCellPortMap& ports = m_cellPortAliases.at(cellp);
        स्थिर FsmCellPortMap::const_iterator portIt = ports.find(portName);
        वापस portIt == ports.end() ? nullptr : portIt->second;
    }

    AstVarScope* childRoleVarScope(AstCell* cellp, स्थिर string& portName) स्थिर {
        स्थिर FsmCellPortMap& ports = m_cellPortChildAliases.at(cellp);
        स्थिर FsmCellPortMap::const_iterator portIt = ports.find(portName);
        वापस portIt == ports.end() ? nullptr : portIt->second;
    }

    श्रेणी RegisterAlwaysAnalyzer final {
        AstScope* स्थिर m_scopep;  // Scope containing the always block(s) being analyzed

    खुला:
        explicit RegisterAlwaysAnalyzer(AstScope* scopep)
            : m_scopep{scopep} {}

        std::vector<std::pair<AstCase*, Ast_गाँठ_व्यंजक*>>
        oneBlockCandidates(AstAlways* alwaysp) स्थिर {
            std::vector<std::pair<AstCase*, Ast_गाँठ_व्यंजक*>> candidates;
            Ast_गाँठ* स्थिर stmtsp = alwaysp->stmtsp();
            अगर (AstIf* स्थिर firstIfp = VN_CAST(stmtsp, If)) {
                अगर (AstCase* स्थिर casep = VN_CAST(firstIfp->elsesp(), Case)) {
                    candidates.emplace_back(casep,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            क्रम (Ast_गाँठ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                अगर (AstCase* स्थिर casep = VN_CAST(nodep, Case))
                    candidates.emplace_back(casep, nullptr);
            }
            वापस candidates;
        }

        std::vector<std::pair<AstIf*, Ast_गाँठ_व्यंजक*>>
        oneBlockIfCandidates(AstAlways* alwaysp) स्थिर {
            std::vector<std::pair<AstIf*, Ast_गाँठ_व्यंजक*>> candidates;
            Ast_गाँठ* स्थिर stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            अगर (AstIf* स्थिर firstIfp = VN_CAST(stmtsp, If)) {
                अगर (AstIf* स्थिर chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(firstIfp->elsesp()), If)) {
                    candidates.emplace_back(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            क्रम (Ast_गाँठ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                अगर (AstIf* स्थिर ifp = VN_CAST(nodep, If)) candidates.emplace_back(ifp, nullptr);
            }
            वापस candidates;
        }

        bool matchRegisterCandidate(AstAlways* alwaysp, FsmRegisterCandidate& cand) स्थिर {
            वापस FsmDetectVisitor::matchRegisterAlways(alwaysp, m_scopep, cand);
        }

        व्योम buildOneBlockCandidate(AstAlways* alwaysp, AstVarScope* vscp, Ast_गाँठ_व्यंजक* resetCondp,
                                    FsmRegisterCandidate& reg) स्थिर {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.अग्र_Vscp(vscp);
            reg.senses() = FsmDetectVisitor::describe_Sen_वृक्ष(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != nullptr);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* स्थिर firstIfp = VN_CAST(alwaysp->stmtsp(), If);
            अगर (firstIfp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = nullptr;
                स्थिर ResetAssignStatus resetStatus = FsmDetectVisitor::collectConstStateAssigns(
                    firstIfp->thensp(), resetStateVscp, reg.resetArcs());
                अगर (resetStatus == ResetAssignStatus::EMPTY) {
                    reg.resetArcs().सफा();
                } अन्यथा अगर (resetStatus == ResetAssignStatus::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().सफा();
                    Fsm_State_मूल्य reset_मूल्य;
                    Ast_गाँठ* स्थिर thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(firstIfp->thensp());
                    UASSERT_OBJ(thenNodep, firstIfp,
                                "one-block reset fallback requires a non-empty reset branch");
                    Ast_गाँठ_Assign* स्थिर resetAssp = FsmDetectVisitor::direct_Const_State_Assign_गाँठ(
                        thenNodep, resetStateVscp, reset_मूल्य);
                    अगर (resetAssp && resetStateVscp == vscp) {
                        reg.resetArcs().emplace_back(reset_मूल्य, resetAssp);
                    }
                } अन्यथा अगर (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                    reg.resetArcs().सफा();
                }
            }
        }
    };

    bool matchFsmWrapperCell(AstScope* scopep, AstCell* cellp, FsmRegisterCandidate& cand) स्थिर {
        FsmWrapperRoles roles = rolesFromDesc(*fsmRegisterWrapperDesc(cellp));

        AstVarScope* स्थिर अग्र_Vscp = roleVarScope(cellp, roles.dPort);
        AstVarScope* स्थिर stateVscp = roleVarScope(cellp, roles.qPort);
        अगर (!अग्र_Vscp || !stateVscp) {
            वापस rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* स्थिर clkVscp = roleVarScope(cellp, roles.clkPort);
        अगर (!clkVscp) {
            वापस rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper instance requires a simple clock connection");
        }

        FsmSenDesc clkSense;
        clkSense.edgeType = VEdgeType::ET_POSEDGE;
        clkSense.varScopep = clkVscp;
        cand.senses().धकेलो_back(clkSense);

        AstVarScope* resetVscp = nullptr;
        अगर (!roles.rstPort.empty()) resetVscp = roleVarScope(cellp, roles.rstPort);
        अगर (resetVscp) {
            // The descriptor identifies the reset port but not its polarity. Use
            // the wrapper's own event control AST as the contract for sampling
            // the connected parent signal.
            bool inferred_Active_निचला = false;
            अगर (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rstPort, inferred_Active_निचला)) {
                roles.has_Rst_Active_निचला = true;
                roles.rst_Active_निचला = inferred_Active_निचला;
            }
        }

        Ast_गाँठ_व्यंजक* resetValuep = nullptr;
        Fsm_State_मूल्य reset_मूल्य;
        स्थिर bool has_Reset_मूल्य
            = !roles.rstValParam.empty()
              && simple_Param_State_मूल्य(cellp, roles.rstValParam, reset_मूल्य, resetValuep);
        अगर (resetVscp && roles.has_Rst_Active_निचला && has_Reset_मूल्य) {
            FsmSenDesc rstSense;
            rstSense.edgeType = roles.rst_Active_निचला ? VEdgeType::ET_NEGEDGE : VEdgeType::ET_POSEDGE;
            rstSense.varScopep = resetVscp;
            cand.senses().धकेलो_back(rstSense);
            cand.resetCond().varScopep = resetVscp;
            cand.resetCond().active_निचला = roles.rst_Active_निचला;
            cand.hasResetCond(true);
            cand.resetArcs().emplace_back(reset_मूल्य, cellp, resetValuep);
        } अन्यथा अगर (!roles.rstPort.empty() || !roles.rstValParam.empty()) {
            string reason;
            अगर (roles.rstPort.empty()) {
                reason = "reset port is not configured";
            } अन्यथा अगर (!resetVscp) {
                reason = "reset connection is missing or not a simple variable";
            } अन्यथा अगर (!roles.has_Rst_Active_निचला) {
                reason = "reset polarity could not be inferred from the wrapper";
            } अन्यथा अगर (roles.rstValParam.empty()) {
                reason = "reset_value parameter is not configured";
            } अन्यथा {
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
        cand.अग्र_Vscp(अग्र_Vscp);
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(cellp->fileline());
        वापस true;
    }

    श्रेणी ComboAlwaysAnalyzer final {
    खुला:
        काष्ठा ComboMatch final {
            स्थिर FsmRegisterCandidate* matchedp = nullptr;
            Ast_गाँठ* warnNodep = nullptr;
        };

    निजी:
        स्थिर std::vector<FsmRegisterCandidate>& m_registerCandidates;

    खुला:
        explicit ComboAlwaysAnalyzer(स्थिर std::vector<FsmRegisterCandidate>& registerCandidates)
            : m_registerCandidates{registerCandidates} {}

        ComboMatch matchCase(Ast_गाँठ* stmtsp, AstCase* casep) स्थिर {
            ComboMatch match;
            AstVarRef* स्थिर selp = VN_CAST(casep->exprp(), VarRef);
            अगर (!selp) वापस match;
            क्रम (स्थिर FsmRegisterCandidate& reg : m_registerCandidates) {
                अगर (selp->varScopep() == reg.अग्र_Vscp()) {
                    अगर (!FsmDetectVisitor::has_Canonical_अग्र_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.अग्र_Vscp())) {
                        जारी;
                    }
                } अन्यथा अगर (selp->varScopep() != reg.stateVscp()) {
                    जारी;
                }
                Ast_गाँठ* स्थिर warnNodep = FsmDetectVisitor::case_Supported_Transition_गाँठ(
                    casep, reg.अग्र_Vscp(), reg.inclCond());
                अगर (!warnNodep) जारी;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            वापस match;
        }

        ComboMatch matchIfChain(Ast_गाँठ* stmtsp, स्थिर FsmIfChainCandidate& chain) स्थिर {
            ComboMatch match;
            क्रम (स्थिर FsmRegisterCandidate& reg : m_registerCandidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                अगर (chain.compareVscp == reg.अग्र_Vscp()) {
                    अगर (!FsmDetectVisitor::has_Canonical_अग्र_State_Default_Before_Case(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.अग्र_Vscp())) {
                        जारी;
                    }
                } अन्यथा अगर (chain.compareVscp != reg.stateVscp()) {
                    जारी;
                }
                Ast_गाँठ* स्थिर warnNodep
                    = FsmDetectVisitor::if_Chain_Supported_Transition_गाँठ(chain, reg.अग्र_Vscp());
                अगर (!warnNodep) जारी;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            वापस match;
        }

        bool shouldWarnUnsupported(Ast_गाँठ* stmtsp, AstCase* casep) स्थिर {
            स्थिर AstVarRef* स्थिर selp = VN_CAST(casep->exprp(), VarRef);
            अगर (!selp) वापस false;

            वापस std::any_of(
                m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                [&](स्थिर FsmRegisterCandidate& reg) -> bool {
                    स्थिर bool matches_अग्र = selp->varScopep() == reg.अग्र_Vscp();
                    स्थिर bool matchesState = selp->varScopep() == reg.stateVscp();

                    अगर (!matches_अग्र && !matchesState) वापस false;
                    अगर (matches_अग्र
                        && !FsmDetectVisitor::has_Canonical_अग्र_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.अग्र_Vscp())) {
                        वापस false;
                    }
                    वापस FsmDetectVisitor::case_Supported_Transition_गाँठ(casep, reg.अग्र_Vscp(),
                                                                         reg.inclCond());
                });
        }
    };

    // Reset arcs are only modeled for the simple signal form that survives to
    // this pass after earlier normalization.
    अटल bool isSimpleResetCond(Ast_गाँठ_व्यंजक* condp) { वापस VN_IS(condp, VarRef); }

    // Normalize the reset condition into a compact description so the lowering
    // phase can regenerate the same predicate after detection. By the time
    // this pass runs, active-low source forms such as "!rst_n" have already
    // been canonicalized to a positive-condition if/else shape, so only a
    // plain VarRef survives here.
    अटल FsmResetCondDesc describeResetCond(Ast_गाँठ_व्यंजक* condp) {
        FsmResetCondDesc desc;
        अगर (AstVarRef* स्थिर vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        वापस desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    अटल std::vector<FsmSenDesc> describe_Sen_वृक्ष(Ast_Sen_वृक्ष* sentreep) {
        std::vector<FsmSenDesc> senses;
        क्रम (AstSenItem* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), SenItem)) {
            Ast_गाँठ_Var_Ref* स्थिर vrefp = itemp->varrefp();
            अगर (!vrefp) जारी;
            FsmSenDesc desc;
            desc.edgeType = itemp->edgeType().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.धकेलो_back(desc);
        }
        वापस senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    अटल bool isIgnorableStmt(Ast_गाँठ* nodep) { वापस VN_IS(nodep, CoverInc); }

    अटल Ast_गाँठ* skipLeadingIgnorableStmt(Ast_गाँठ* nodep) {
        जबतक (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        वापस nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    अटल Ast_गाँठ* singleMeaningfulStmt(Ast_गाँठ* stmtp) {
        Ast_गाँठ* resultp = nullptr;
        क्रम (Ast_गाँठ* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            अगर (isIgnorableStmt(nodep)) जारी;
            अगर (resultp) वापस nullptr;
            resultp = nodep;
        }
        वापस resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    अटल Ast_गाँठ* singleMeaningfulBranch(Ast_गाँठ* branchp) {
        अगर (!branchp) वापस nullptr;
        वापस branchp;
    }

    // By fsm-detect time, non-clocked always @* blocks are already admitted through
    // a missing sentree. This helper therefore only needs to recognize
    // explicit changed-sensitivity lists such as always @(a or b); clocked and
    // event-driven forms remain out of scope.
    अटल bool isPlainComboSentree(स्थिर Ast_Sen_वृक्ष* sentreep) {
        UASSERT(sentreep, "plain combo sensitivity check requires a sensitivity tree");
        क्रम (स्थिर AstSenItem* senp = sentreep->sensesp(); senp;
             senp = VN_AS(senp->nextp(), SenItem)) {
            अगर (senp->edgeType() == VEdgeType::ET_CHANGED) जारी;
            वापस false;
        }
        वापस true;
    }

    व्योम warnUnsupportedComboAlways(स्थिर FsmComboAlways& combo) {
        स्थिर ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_गाँठ* स्थिर stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        bool warned = false;
        क्रम (Ast_गाँठ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* स्थिर casep = VN_CAST(nodep, Case);
            अगर (!casep) जारी;
            अगर (analyzer.shouldWarnUnsupported(stmtsp, casep)) {
                casep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-clocked always "
                                        "blocks requires a combinational sensitivity list or "
                                        "always_comb");
                warned = true;
            }
            अगर (warned) अवरोध;
        }
    }

    // Case-item bodies are single subtrees like if/else arms, not statement
    // lists, so unwrap only local begin/end wrappers here rather than walking
    // sibling case items via nextp().
    अटल Ast_गाँठ_Assign* directStateAssign(Ast_गाँठ* stmtp, AstVarScope* stateVscp) {
        Ast_गाँठ* स्थिर nodep = singleMeaningfulBranch(stmtp);
        अगर (!nodep) वापस nullptr;
        Ast_गाँठ_Assign* स्थिर assp = VN_CAST(nodep, गाँठ_Assign);
        अगर (!assp) वापस nullptr;
        AstVarRef* स्थिर vrefp = VN_CAST(assp->lhsp(), VarRef);
        अगर (!vrefp || vrefp->varScopep() != stateVscp) वापस nullptr;
        वापस assp;
    }

    अटल Ast_गाँठ_Assign* गाँठ_State_Var_Assign(Ast_गाँठ* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        Ast_गाँठ_Assign* स्थिर assp = VN_CAST(nodep, गाँठ_Assign);
        अगर (!assp) वापस nullptr;
        AstVarRef* स्थिर lhsp = VN_CAST(assp->lhsp()->baseFromp(true), VarRef);
        AstVarRef* स्थिर rhsp = VN_CAST(assp->rhsp(), VarRef);
        अगर (!rhsp || !lhsp) वापस nullptr;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        वापस assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    अटल Ast_गाँठ_Assign* directCondStateVarAssign(Ast_गाँठ* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, Ast_गाँठ_व्यंजक*& condp,
                                                   Fsm_State_मूल्य& reset_मूल्य) {
        Ast_गाँठ_Assign* स्थिर assp = VN_CAST(nodep, गाँठ_Assign);
        अगर (!assp) वापस nullptr;
        AstVarRef* स्थिर lhsp = VN_CAST(assp->lhsp()->baseFromp(true), VarRef);
        AstCond* स्थिर rhsp = VN_CAST(assp->rhsp(), Cond);
        अगर (!rhsp || !lhsp) वापस nullptr;
        AstVarRef* स्थिर elsep = VN_CAST(rhsp->elsep(), VarRef);
        अगर (!elsep) वापस nullptr;
        अगर (const_मूल्य_Status(rhsp->thenp(), reset_मूल्य) != Const_मूल्य_Status::OK) वापस nullptr;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        वापस assp;
    }

    अटल Ast_गाँठ_Assign* direct_Const_State_Assign_गाँठ(Ast_गाँठ* nodep, AstVarScope*& stateVscp,
                                                     Fsm_State_मूल्य& मूल्य) {
        Ast_गाँठ_Assign* स्थिर assp = VN_CAST(nodep, गाँठ_Assign);
        अगर (!assp) वापस nullptr;
        AstVarRef* स्थिर lhsp = VN_CAST(assp->lhsp()->baseFromp(true), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        अगर (const_मूल्य_Status(assp->rhsp(), मूल्य) != Const_मूल्य_Status::OK) वापस nullptr;
        stateVscp = lhsp->varScopep();
        वापस assp;
    }

    क्रमागत श्रेणी ResetAssignStatus : uint8_t {
        EMPTY,  // Reset branch had no non-coverage statements.
        NONE,  // Reset branch was not the supported direct-constant shape.
        SINGLE,  // Exactly one supported reset assignment was collected.
        MULTI_SAME_STATE  // Multiple assignments to the same FSM state var; warn and ignore.
    };

    // Reset arcs are only extracted from the single direct-constant form. If
    // user RTL assigns the same state register multiple times in the reset
    // branch, warn and skip reset-arc modeling rather than inventing multiple
    // reset transitions for an odd but legal coding style.
    अटल ResetAssignStatus collectConstStateAssigns(Ast_गाँठ* stmtp, AstVarScope*& stateVscp,
                                                      std::vector<FsmResetArcDesc>& resetArcs) {
        Ast_गाँठ* nodep = skipLeadingIgnorableStmt(stmtp);
        अगर (!nodep) वापस ResetAssignStatus::EMPTY;
        क्रम (;; nodep = nodep->nextp()) {
            AstVarScope* assignStateVscp = nullptr;
            Fsm_State_मूल्य मूल्य;
            Ast_गाँठ_Assign* स्थिर assp = direct_Const_State_Assign_गाँठ(nodep, assignStateVscp, मूल्य);
            अगर (!assp) वापस ResetAssignStatus::NONE;
            अगर (!stateVscp) stateVscp = assignStateVscp;
            अगर (assignStateVscp != stateVscp) वापस ResetAssignStatus::NONE;
            अगर (!resetArcs.empty()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.सफा();
                वापस ResetAssignStatus::MULTI_SAME_STATE;
            }
            resetArcs.emplace_back(मूल्य, assp);
            अगर (!nodep->nextp()) वापस ResetAssignStatus::SINGLE;
        }
    }

    अटल bool has_Canonical_अग्र_State_Default_Before_Case(Ast_गाँठ* stmtsp, Ast_गाँठ* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* अग्र_Vscp) {
        Ast_गाँठ* स्थिर bodyp = skipLeadingIgnorableStmt(stmtsp);
        bool sawCanonicalDefault = false;
        क्रम (Ast_गाँठ* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            अगर (nodep == targetp) वापस sawCanonicalDefault;
            अगर (Ast_गाँठ_Assign* स्थिर assp = VN_CAST(nodep, गाँठ_Assign)) {
                AstVarRef* स्थिर lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* स्थिर rhsp = VN_CAST(assp->rhsp(), VarRef);
                अगर (!lhsp || lhsp->varScopep() != अग्र_Vscp) जारी;
                अगर (sawCanonicalDefault) {
                    स्थिर string अग्र_Name = अग्र_Vscp->varp()->prettyNameQ();
                    स्थिर string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + अग्र_Name
                                     + ") when the canonical " + अग्र_Name + " = " + stateName
                                     + " default is overwritten before the case statement");
                    वापस false;
                }
                अगर (!rhsp || rhsp->varScopep() != stateVscp) वापस false;
                sawCanonicalDefault = true;
            }
        }
    }

    अटल bool ifStateConstAssign(Ast_गाँठ* stmtp, AstVarScope* stateVscp,
                                   Fsm_State_मूल्य& then_मूल्य, Fsm_State_मूल्य& else_मूल्य) {
        AstIf* स्थिर ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        अगर (!ifp || !ifp->elsesp()) वापस false;
        AstVarScope* thenVscp = nullptr;
        AstVarScope* elseVscp = nullptr;
        Ast_गाँठ* स्थिर thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        Ast_गाँठ* स्थिर elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        अगर (!thenNodep || !elseNodep) वापस false;
        अगर (!direct_Const_State_Assign_गाँठ(thenNodep, thenVscp, then_मूल्य)) वापस false;
        अगर (!direct_Const_State_Assign_गाँठ(elseNodep, elseVscp, else_मूल्य)) वापस false;
        अगर (thenVscp == stateVscp && elseVscp == stateVscp) वापस true;
        अगर (thenVscp != elseVscp) वापस false;
        Ast_गाँठ* स्थिर followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = nullptr;
        AstVarScope* finalFromVscp = nullptr;
        Ast_गाँठ* स्थिर finalNodep = singleMeaningfulBranch(followp);
        अगर (!finalNodep) वापस false;
        अगर (!गाँठ_State_Var_Assign(finalNodep, finalStateVscp, finalFromVscp)) वापस false;
        अगर (finalStateVscp != stateVscp) वापस false;
        अगर (finalFromVscp != thenVscp) वापस false;
        वापस true;
    }

    अटल bool directStateCondConstAssign(Ast_गाँठ* stmtp, AstVarScope* stateVscp,
                                           Fsm_State_मूल्य& then_मूल्य, Fsm_State_मूल्य& else_मूल्य) {
        Ast_गाँठ_Assign* स्थिर assp = directStateAssign(stmtp, stateVscp);
        अगर (!assp) वापस false;
        AstCond* स्थिर condp = VN_CAST(assp->rhsp(), Cond);
        अगर (!condp) वापस false;
        वापस const_मूल्य_Status(condp->thenp(), then_मूल्य) == Const_मूल्य_Status::OK
               && const_मूल्य_Status(condp->elsep(), else_मूल्य) == Const_मूल्य_Status::OK;
    }

    अटल Ast_गाँठ* case_Item_Supported_Arc_गाँठ(AstCaseItem* itemp, AstVarScope* stateVscp,
                                             bool inclCond) {
        अगर (itemp->isDefault()) {
            अगर (!inclCond) वापस nullptr;
        }
        Ast_गाँठ_Assign* स्थिर assp = directStateAssign(itemp->stmtsp(), stateVscp);
        अगर (assp) {
            Fsm_State_मूल्य to_मूल्य;
            अगर (const_मूल्य_Status(assp->rhsp(), to_मूल्य) == Const_मूल्य_Status::OK) वापस assp;
        }
        Fsm_State_मूल्य then_मूल्य;
        Fsm_State_मूल्य else_मूल्य;
        अगर (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_मूल्य, else_मूल्य)) {
            वापस assp;
        }
        अगर (ifStateConstAssign(itemp->stmtsp(), stateVscp, then_मूल्य, else_मूल्य)) {
            वापस singleMeaningfulBranch(itemp->stmtsp());
        }
        वापस nullptr;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    अटल Ast_गाँठ* case_Supported_Transition_गाँठ(AstCase* casep, AstVarScope* stateVscp,
                                                bool inclCond) {
        क्रम (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            अगर (Ast_गाँठ* स्थिर nodep = case_Item_Supported_Arc_गाँठ(itemp, stateVscp, inclCond))
                वापस nodep;
        }
        वापस nullptr;
    }

    अटल Ast_गाँठ* case_Item_Supported_Arc_गाँठ_Like(Ast_गाँठ* stmtsp, AstVarScope* stateVscp) {
        अगर (Ast_गाँठ_Assign* स्थिर assp = directStateAssign(stmtsp, stateVscp)) {
            Fsm_State_मूल्य to_मूल्य;
            अगर (const_मूल्य_Status(assp->rhsp(), to_मूल्य) == Const_मूल्य_Status::OK) वापस assp;
            Fsm_State_मूल्य then_मूल्य;
            Fsm_State_मूल्य else_मूल्य;
            अगर (directStateCondConstAssign(stmtsp, stateVscp, then_मूल्य, else_मूल्य)) वापस assp;
        }
        वापस nullptr;
    }

    अटल Ast_गाँठ* if_Chain_Supported_Transition_गाँठ(स्थिर FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        क्रम (माप_प्रकार i = 0; i < chain.branches.आकार(); ++i) {
            स्थिर FsmIfBranch& branch = chain.branches[i];
            Ast_गाँठ* स्थिर nodep = case_Item_Supported_Arc_गाँठ_Like(branch.stmtsp, stateVscp);
            अगर (!nodep) वापस nullptr;
        }
        वापस chain.branches.front().ifp;
    }

    // Prefer user labels in reports. Forced non-enum FSMs prepopulate synthetic
    // labels, so all emitted arcs should already have a known label here.
    अटल string label_For_मूल्य(
        स्थिर std::unordered_map<Fsm_State_मूल्य, StateConstLabel, Fsm_State_मूल्य_Hash>& labels,
        स्थिर Fsm_State_मूल्य& मूल्य) {
        वापस labels.at(मूल्य).text;
    }

    // The extractor only models constant-valued state transitions, and by the
    // time detect runs those values have already been constant-folded.
    क्रमागत श्रेणी Const_मूल्य_Status : uint8_t { OK, NOT_CONST, XZ };

    अटल Const_मूल्य_Status const_मूल्य_Status(Ast_गाँठ_व्यंजक* exprp, Fsm_State_मूल्य& मूल्य) {
        स्थिर AstConst* स्थिर constp = VN_CAST(exprp, Const);
        अगर (!constp) वापस Const_मूल्य_Status::NOT_CONST;
        स्थिर V3_संख्या& अंक = constp->अंक();
        अगर (अंक.isAnyXZ()) वापस Const_मूल्य_Status::XZ;
        मूल्य = Fsm_State_मूल्य{अंक};
        वापस Const_मूल्य_Status::OK;
    }

    अटल bool pureStateComparisonNoAlias(Ast_गाँठ_व्यंजक* exprp, FsmStateComparison& cmp) {
        AstEq* स्थिर eqp = VN_CAST(exprp, Eq);
        अगर (!eqp) वापस false;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        Ast_गाँठ_व्यंजक* valuep = eqp->rhsp();
        अगर (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->baseFromp(true), VarRef);
            अगर (!vrefp) { वापस false; }
            valuep = eqp->lhsp();
        }

        Fsm_State_मूल्य मूल्य;
        अगर (const_मूल्य_Status(valuep, मूल्य) != Const_मूल्य_Status::OK) वापस false;
        cmp.stateVscp = vrefp->varScopep();
        cmp.valuep = valuep;
        cmp.मूल्य = मूल्य;
        वापस true;
    }

    अटल bool pureStateComparison(Ast_गाँठ_व्यंजक* exprp, स्थिर FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        अगर (pureStateComparisonNoAlias(exprp, cmp)) वापस true;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        अगर (AstVarRef* स्थिर vrefp = VN_CAST(exprp, VarRef)) {
            स्थिर FsmAliasMap::const_iterator it = aliases.find(vrefp->varScopep());
            अगर (it == aliases.end()) वापस false;
            cmp = it->second;
            वापस true;
        }
        वापस false;
    }

    अटल bool supported_शीर्ष_Level_Guard(Ast_गाँठ_व्यंजक* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        अगर (VN_IS(exprp, Or)) वापस false;
        अगर (VN_IS(exprp, RedAnd)) वापस false;
        अगर (VN_IS(exprp, RedOr)) वापस false;
        अगर (VN_IS(exprp, RedXor)) वापस false;
        वापस true;
    }

    अटल bool resolveIfPredicate(Ast_गाँठ_व्यंजक* exprp, स्थिर FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, bool& hasGuard) {
        std::vector<Ast_गाँठ_व्यंजक*> terms;
        std::vector<Ast_गाँठ_व्यंजक*> pending;
        pending.धकेलो_back(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        जबतक (!pending.empty()) {
            Ast_गाँठ_व्यंजक* स्थिर nodep = pending.back();
            pending.निकालो_back();
            अगर (AstAnd* स्थिर andp = VN_CAST(nodep, And)) {
                pending.धकेलो_back(andp->rhsp());
                pending.धकेलो_back(andp->lhsp());
            } अन्यथा {
                terms.धकेलो_back(nodep);
            }
        }

        bool sawComparison = false;
        क्रम (माप_प्रकार i = 0; i < terms.आकार(); ++i) {
            Ast_गाँठ_व्यंजक* स्थिर termp = terms[i];
            FsmStateComparison termCmp;
            अगर (pureStateComparison(termp, aliases, termCmp /*ref*/)) {
                अगर (sawComparison) वापस false;
                cmp = termCmp;
                sawComparison = true;
                जारी;
            }
            अगर (!supported_शीर्ष_Level_Guard(termp)) वापस false;
            hasGuard = true;
        }
        वापस sawComparison;
    }

    अटल व्योम addAlias(FsmAliasMap& aliases, std::unordered_set<स्थिर AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, स्थिर FsmStateComparison& cmp) {
        अगर (ambiguous.find(aliasVscp) != ambiguous.end()) वापस;
        स्थिर FsmAliasMap::iterator it = aliases.find(aliasVscp);
        अगर (it == aliases.end()) {
            aliases.emplace(aliasVscp, cmp);
            वापस;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        अगर (it->second.stateVscp == cmp.stateVscp && it->second.मूल्य == cmp.मूल्य) वापस;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        वापस;
    }

    अटल व्योम collectAliasFromAssign(Ast_गाँठ_Assign* assp, FsmAliasMap& aliases,
                                       std::unordered_set<स्थिर AstVarScope*>& ambiguous) {
        AstVarRef* स्थिर lhsp = VN_CAST(assp->lhsp(), VarRef);
        अगर (!lhsp) वापस;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        अगर (!pureStateComparisonNoAlias(assp->rhsp(), cmp /*ref*/)) वापस;
        addAlias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(Ast_गाँठ* stmtsp, Ast_गाँठ* limitp) स्थिर {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_set<स्थिर AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        क्रम (Ast_गाँठ* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            अगर (Ast_गाँठ_Assign* स्थिर assp = VN_CAST(nodep, गाँठ_Assign)) {
                collectAliasFromAssign(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        क्रम (स्थिर AstVarScope* स्थिर vscp : ambiguous) aliases.erase(vscp);
        वापस aliases;
    }

    अटल bool collectIfChain(AstIf* ifp, स्थिर FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_set<string> seen_मूल्यावली;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        जबतक (true) {
            FsmStateComparison cmp;
            bool hasGuard = false;
            अगर (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) वापस false;
            अगर (chain.compareVscp && chain.compareVscp != cmp.stateVscp) वापस false;
            अगर (!seen_मूल्यावली.जोड़ो(cmp.मूल्य.कुंजी()).second) वापस false;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.धकेलो_back(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.मूल्य, hasGuard});

            Ast_गाँठ* स्थिर elseNodep
                = singleMeaningfulBranch(skipLeadingIgnorableStmt(curp->elsesp()));
            अगर (!elseNodep) अवरोध;
            अगर (AstIf* स्थिर elseIfp = VN_CAST(elseNodep, If)) {
                curp = elseIfp;
                जारी;
            }
            chain.defaultStmtsp = elseNodep;
            अवरोध;
        }
        वापस chain.branches.आकार() >= 2;
    }

    // Enum-backed FSMs should only use values that were interned as known states.
    // If a constant transition references some other encoding, warn and skip FSM
    // instrumentation for that edge rather than silently dropping it or turning
    // optional coverage into a hard compile failure.
    अटल bool validate_Known_State_मूल्य(Ast_गाँठ* nodep, स्थिर FsmStateSpace& stateSpace,
                                        स्थिर Fsm_State_मूल्य& मूल्य, स्थिर string& role) {
        अगर (stateSpace.labels.find(मूल्य) != stateSpace.labels.end()) वापस true;
        अगर (stateSpace.enumBacked) {
            स्थिर string enumRole = role == "source" ? "case item value" : "assigned value";
            nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on enum state variable "
                                        + stateSpace.stateVarp->prettyNameQ() + ": " + enumRole
                                        + " " + मूल्य.warnText()
                                        + " is not present in the declared enum");
            वापस false;
        }
        nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum state variable "
                                    + stateSpace.stateVarp->prettyNameQ() + ": " + role + " value "
                                    + मूल्य.warnText()
                                    + " is not present in the inferred state space");
        वापस false;
    }

    अटल StateConstLabel stateLabelForConst(AstConst* constp) {
        स्थिर string name = constp->origParamName();
        अगर (!name.empty()) वापस StateConstLabel{Ast_गाँठ::prettyName(name), true, 0};
        वापस StateConstLabel{constp->name(), false, 0};
    }

    अटल व्योम updateStateLabel(FsmStateSpace& stateSpace, स्थिर Fsm_State_मूल्य& मूल्य,
                                 स्थिर StateConstLabel& label) {
        stateSpace.states.at(stateSpace.labels.at(मूल्य).state_सूचकांक).first = label.text;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    अटल bool matchRegisterAlways(AstAlways* alwaysp, AstScope* scopep,
                                    FsmRegisterCandidate& cand) {
        अगर (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) वापस false;

        Ast_गाँठ* स्थिर stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        Ast_गाँठ* स्थिर nodep = singleMeaningfulStmt(stmtsp);
        अगर (!nodep) वापस false;

        AstVarScope* stateVscp = nullptr;
        AstVarScope* अग्र_Vscp = nullptr;
        अगर (AstIf* स्थिर ifp = VN_CAST(nodep, If)) {
            अगर (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) वापस false;
            AstVarScope* resetStateVscp = nullptr;
            स्थिर ResetAssignStatus resetStatus
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            स्थिर bool emptyResetBranch = resetStatus == ResetAssignStatus::EMPTY;
            अगर (emptyResetBranch) {
                cand.resetArcs().सफा();
            } अन्यथा अगर (resetStatus == ResetAssignStatus::NONE) {
                cand.resetArcs().सफा();
                Fsm_State_मूल्य reset_मूल्य;
                Ast_गाँठ* स्थिर thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                Ast_गाँठ_Assign* स्थिर resetAssp
                    = direct_Const_State_Assign_गाँठ(thenNodep, resetStateVscp, reset_मूल्य);
                अगर (!resetAssp) वापस false;
                cand.resetArcs().emplace_back(reset_मूल्य, resetAssp);
            } अन्यथा अगर (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                cand.resetArcs().सफा();
            }
            Ast_गाँठ* स्थिर elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            अगर (!गाँठ_State_Var_Assign(elseNodep, stateVscp, अग्र_Vscp)) वापस false;
            अगर (!emptyResetBranch && resetStateVscp != stateVscp) वापस false;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != nullptr);
        } अन्यथा {
            Ast_गाँठ_व्यंजक* resetCondp = nullptr;
            Fsm_State_मूल्य reset_मूल्य;
            // Note the active low 'reset ? next : CONST' form is not accepted
            अगर (Ast_गाँठ_Assign* स्थिर assp
                = directCondStateVarAssign(nodep, stateVscp, अग्र_Vscp, resetCondp, reset_मूल्य)) {
                cand.resetArcs().emplace_back(reset_मूल्य, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != nullptr);
            } अन्यथा अगर (!गाँठ_State_Var_Assign(nodep, stateVscp, अग्र_Vscp)) {
                वापस false;
            }
        }
        cand.scopep(scopep);
        cand.alwaysp(alwaysp);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(stateVscp);
        cand.अग्र_Vscp(अग्र_Vscp);
        cand.senses() = describe_Sen_वृक्ष(alwaysp->sentreep());
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(alwaysp->fileline());
        वापस true;
    }

    अटल bool add_मूल्य_To_State_Space(Ast_गाँठ* nodep, FsmStateSpace& stateSpace,
                                     स्थिर Fsm_State_मूल्य& मूल्य, StateConstLabel label) {
        स्थिर स्वतः labelIt = stateSpace.labels.find(मूल्य);
        अगर (labelIt != stateSpace.labels.end()) {
            StateConstLabel& existingLabel = labelIt->second;
            अगर (existingLabel.text != label.text && existingLabel.fromParam && label.fromParam) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                        "state variable "
                                            + stateSpace.stateVarp->prettyNameQ()
                                            + " with multiple labels for the same value "
                                            + मूल्य.warnText() + ": " + existingLabel.text
                                            + " and " + label.text);
                वापस false;
            }
            अगर (!existingLabel.fromParam && label.fromParam) {
                existingLabel.text = label.text;
                existingLabel.fromParam = label.fromParam;
                updateStateLabel(stateSpace, मूल्य, label);
            }
        } अन्यथा {
            StateConstLabel storedLabel = label;
            storedLabel.state_सूचकांक = stateSpace.states.आकार();
            stateSpace.states.emplace_back(label.text, मूल्य);
            stateSpace.labels.emplace(मूल्य, storedLabel);
        }
        वापस true;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    अटल bool add_व्यंजक_To_State_Space(Ast_गाँठ_व्यंजक* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_मूल्य मूल्य;
        स्थिर Const_मूल्य_Status status = const_मूल्य_Status(valuep, मूल्य);
        अगर (status != Const_मूल्य_Status::OK) {
            अगर (status == Const_मूल्य_Status::XZ) {
                valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                         "state variable "
                                             + stateSpace.stateVarp->prettyNameQ()
                                             + " with X/Z state encoding values");
            }
            वापस false;
        }
        AstConst* स्थिर constp = VN_AS(valuep, Const);
        वापस add_मूल्य_To_State_Space(valuep, stateSpace, मूल्य, stateLabelForConst(constp));
    }

    अटल bool add_Optional_Target_व्यंजक_To_State_Space(Ast_गाँठ_व्यंजक* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_मूल्य मूल्य;
        स्थिर Const_मूल्य_Status status = const_मूल्य_Status(valuep, मूल्य);
        अगर (status != Const_मूल्य_Status::OK) {
            valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                     "state variable "
                                         + stateSpace.stateVarp->prettyNameQ()
                                         + " with non-constant target state values");
            वापस false;
        }
        AstConst* स्थिर constp = VN_AS(valuep, Const);
        वापस add_मूल्य_To_State_Space(valuep, stateSpace, मूल्य, stateLabelForConst(constp));
    }

    अटल व्योम addResetTargetsToStateSpace(स्थिर std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        क्रम (स्थिर FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.to_मूल्य().ascii(), false, 0};
            अगर (AstConst* स्थिर constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                add_मूल्य_To_State_Space(resetArc.nodep(), stateSpace, resetArc.to_मूल्य(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    अटल bool collectDeclaredStateSpace(Ast_गाँठ* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, bool& needs_Source_मूल्यावली) {
        AstVar* स्थिर stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        अगर (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        स्थिर bool forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        अगर (enump) {
            stateSpace.enumBacked = true;
            क्रम (AstEnumItem* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                स्थिर AstConst* स्थिर constp = VN_AS(itemp->valuep(), Const);
                स्थिर Fsm_State_मूल्य मूल्य{constp->अंक()};
                स्थिर माप_प्रकार state_सूचकांक = stateSpace.states.आकार();
                stateSpace.states.emplace_back(itemp->name(), मूल्य);
                stateSpace.labels.emplace(मूल्य,
                                          StateConstLabel{itemp->name(), false, state_सूचकांक});
            }
            वापस stateSpace.states.आकार() >= 2;
        }

        अगर (forced) {
            needs_Source_मूल्यावली = true;
            वापस true;
        }

        needs_Source_मूल्यावली = true;
        वापस true;
    }

    ढाँचा <typename T_ValuepVisitor>
    अटल bool collect_State_Space_From_मूल्यावली(Ast_गाँठ* warnNodep, AstVarScope* stateVscp,
                                            स्थिर std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            स्थिर T_ValuepVisitor& visitValueps) {
        bool needs_Source_मूल्यावली = false;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        अगर (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needs_Source_मूल्यावली)) {
            वापस false;
        }
        अगर (!needs_Source_मूल्यावली) वापस true;
        addResetTargetsToStateSpace(resetArcs, stateSpace);
        अगर (!visitValueps(
                [&](Ast_गाँठ_व्यंजक* valuep) { वापस add_व्यंजक_To_State_Space(valuep, stateSpace); })) {
            वापस false;
        }
        वापस stateSpace.states.आकार() >= 2;
    }

    अटल bool collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* assignVscp,
                                  स्थिर std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        वापस collect_State_Space_From_मूल्यावली(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, assignVscp, &stateSpace](स्थिर स्वतः& visitValuep) {
                क्रम (AstCaseItem* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), CaseItem)) {
                    अगर (!itemp->isDefault()) {
                        क्रम (Ast_गाँठ_व्यंजक* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), गाँठ_व्यंजक)) {
                            अगर (!visitValuep(condp)) वापस false;
                        }
                    }
                    अगर (Ast_गाँठ_Assign* स्थिर assp
                        = directStateAssign(itemp->stmtsp(), assignVscp)) {
                        Fsm_State_मूल्य then_मूल्य;
                        Fsm_State_मूल्य else_मूल्य;
                        AstCond* स्थिर condp = VN_CAST(assp->rhsp(), Cond);
                        अगर (condp
                            && directStateCondConstAssign(itemp->stmtsp(), assignVscp, then_मूल्य,
                                                          else_मूल्य)) {
                            अगर (!visitValuep(condp->thenp())) वापस false;
                            अगर (!visitValuep(condp->elsep())) वापस false;
                        } अन्यथा अगर (!add_Optional_Target_व्यंजक_To_State_Space(assp->rhsp(), stateSpace)) {
                            वापस false;
                        }
                    }
                }
                वापस true;
            });
    }

    अटल bool collectStateSpace(स्थिर FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* assignVscp,
                                  स्थिर std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        वापस collect_State_Space_From_मूल्यावली(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, assignVscp, &stateSpace](स्थिर स्वतः& visitValuep) {
                क्रम (स्थिर FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(visitValuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    Ast_गाँठ_Assign* स्थिर assp = directStateAssign(branch.stmtsp, assignVscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    Fsm_State_मूल्य then_मूल्य;
                    Fsm_State_मूल्य else_मूल्य;
                    AstCond* स्थिर condp = VN_CAST(assp->rhsp(), Cond);
                    अगर (condp) {
                        UASSERT_OBJ(directStateCondConstAssign(branch.stmtsp, assignVscp,
                                                               then_मूल्य, else_मूल्य),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        अगर (!visitValuep(condp->thenp())) वापस false;
                        अगर (!visitValuep(condp->elsep())) वापस false;
                    } अन्यथा अगर (!add_Optional_Target_व्यंजक_To_State_Space(assp->rhsp(), stateSpace)) {
                        वापस false;
                    }
                }
                वापस true;
            });
    }

    // Extract supported case-item transitions in one place so the conservative
    // policy for direct and ternary forms stays consistent. The false exits in
    // this helper are deliberate subset boundaries: they document shapes we do
    // not yet model in this PR and that future FSM-detection work may widen.
    अटल bool emitCaseItemArcs(FsmGraph& graph, AstCaseItem* itemp, AstVarScope* stateVscp,
                                 स्थिर FsmStateSpace& stateSpace, bool inclCond) {
        std::vector<std::pair<string, Fsm_State_मूल्य>> froms;
        अगर (itemp->isDefault()) {
            अगर (!inclCond) वापस false;
            froms.emplace_back("default", Fsm_State_मूल्य{});
        } अन्यथा {
            क्रम (Ast_गाँठ_व्यंजक* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), गाँठ_व्यंजक)) {
                Fsm_State_मूल्य मूल्य;
                अगर (const_मूल्य_Status(condp, मूल्य) != Const_मूल्य_Status::OK) जारी;
                अगर (!validate_Known_State_मूल्य(condp, stateSpace, मूल्य, "source")) वापस true;
                froms.emplace_back(label_For_मूल्य(stateSpace.labels, मूल्य), मूल्य);
            }
            अगर (froms.empty()) वापस false;
        }

        अगर (Ast_गाँठ_Assign* स्थिर assp = directStateAssign(itemp->stmtsp(), stateVscp)) {
            Fsm_State_मूल्य to_मूल्य;
            स्थिर Const_मूल्य_Status status = const_मूल्य_Status(assp->rhsp(), to_मूल्य);
            अगर (status == Const_मूल्य_Status::OK) {
                अगर (!validate_Known_State_मूल्य(assp, stateSpace, to_मूल्य, "target")) वापस true;
                क्रम (स्थिर std::pair<string, Fsm_State_मूल्य>& from : froms) {
                    graph.addArc(from.second, to_मूल्य, false, false, itemp->isDefault(),
                                 assp->fileline());
                }
                वापस true;
            }
        }

        Fsm_State_मूल्य then_मूल्य;
        Fsm_State_मूल्य else_मूल्य;
        अगर (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_मूल्य, else_मूल्य)
            || ifStateConstAssign(itemp->stmtsp(), stateVscp, then_मूल्य, else_मूल्य)) {
            अगर (!validate_Known_State_मूल्य(itemp->stmtsp(), stateSpace, then_मूल्य, "target"))
                वापस true;
            अगर (!validate_Known_State_मूल्य(itemp->stmtsp(), stateSpace, else_मूल्य, "target"))
                वापस true;
            क्रम (स्थिर Fsm_State_मूल्य& branch_मूल्य : {then_मूल्य, else_मूल्य}) {
                क्रम (स्थिर std::pair<string, Fsm_State_मूल्य>& from : froms) {
                    graph.addArc(from.second, branch_मूल्य, false, true, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            वापस true;
        }

        वापस false;
    }

    अटल व्योम emitStmtArcsFrom(FsmGraph& graph, Ast_गाँठ* stmtsp, AstVarScope* stateVscp,
                                 स्थिर FsmStateSpace& stateSpace, Fsm_State_मूल्य from_मूल्य,
                                 bool isDefault, bool forceCond) {
        Ast_गाँठ_Assign* स्थिर assp = directStateAssign(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        Fsm_State_मूल्य to_मूल्य;
        स्थिर Const_मूल्य_Status status = const_मूल्य_Status(assp->rhsp(), to_मूल्य);
        अगर (status == Const_मूल्य_Status::OK) {
            अगर (!validate_Known_State_मूल्य(assp, stateSpace, to_मूल्य, "target")) वापस;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            graph.addArc(from_मूल्य, to_मूल्य, false, forceCond, isDefault, assp->fileline());
            वापस;
        }

        Fsm_State_मूल्य then_मूल्य;
        Fsm_State_मूल्य else_मूल्य;
        स्थिर bool condAssign
            = directStateCondConstAssign(stmtsp, stateVscp, then_मूल्य, else_मूल्य);
        UASSERT_OBJ(condAssign, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        अगर (!validate_Known_State_मूल्य(stmtsp, stateSpace, then_मूल्य, "target")) वापस;
        अगर (!validate_Known_State_मूल्य(stmtsp, stateSpace, else_मूल्य, "target")) वापस;
        क्रम (स्थिर Fsm_State_मूल्य& branch_मूल्य : {then_मूल्य, else_मूल्य}) {
            graph.addArc(from_मूल्य, branch_मूल्य, false, true, isDefault, stmtsp->fileline());
        }
    }

    अटल व्योम emitIfChainArcs(FsmGraph& graph, स्थिर FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, स्थिर FsmStateSpace& stateSpace) {
        क्रम (माप_प्रकार i = 0; i < chain.branches.आकार(); ++i) {
            स्थिर FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            अगर (!validate_Known_State_मूल्य(branch.ifp, stateSpace, branch.from_मूल्य, "source"))
                वापस;
            emitStmtArcsFrom(graph, branch.stmtsp, stateVscp, stateSpace, branch.from_मूल्य, false,
                             branch.has_शीर्ष_Guard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    अटल व्योम addResetArcs(FsmGraph& graph, स्थिर std::vector<FsmResetArcDesc>& resetArcs,
                             स्थिर FsmStateSpace& stateSpace) {
        क्रम (स्थिर FsmResetArcDesc& resetArc : resetArcs) {
            अगर (!validate_Known_State_मूल्य(resetArc.nodep(), stateSpace, resetArc.to_मूल्य(),
                                         "target"))
                जारी;
            graph.addArc(Fsm_State_मूल्य{}, resetArc.to_मूल्य(), true, false, false,
                         resetArc.nodep()->fileline());
        }
    }

    // Turn one candidate case statement into the graph representation that the
    // later lowering phase will consume directly, while reviewers can still
    // inspect the extracted machine via DOT dumps.
    व्योम processCase(AstCase* casep, AstVarScope* assignVscp, स्थिर FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, casep, "FSM case processing requires a non-null assignment var");
        AstVarScope* स्थिर stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        अगर (!collectStateSpace(casep, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) वापस;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        अगर (!entry.graphp) {
            entry.graphp.reset(नया FsmGraph{});
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
            क्रम (स्थिर std::pair<string, Fsm_State_मूल्य>& state : stateSpace.states) {
                entry.graphp->addStateVertex(state.first, state.second);
            }
            addResetArcs(*entry.graphp, reg.resetArcs(), stateSpace);
        }
        क्रम (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            emitCaseItemArcs(*entry.graphp, itemp, assignVscp, stateSpace,
                             entry.graphp->inclCond());
        }
    }

    व्योम processIfChain(स्थिर FsmIfChainCandidate& chain, AstVarScope* assignVscp,
                        स्थिर FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, chain.ifp,
                    "FSM if-chain processing requires a non-null assignment var");
        AstVarScope* स्थिर stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        अगर (!collectStateSpace(chain, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) वापस;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        // Case candidates keep ownership of existing graphs; reaching this path
        // means the if-chain is the only supported dispatch for this FSM.
        UASSERT_OBJ(!entry.graphp, chain.ifp, "FSM if-chain graph should not already exist");
        entry.graphp.reset(नया FsmGraph{});
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
        क्रम (स्थिर std::pair<string, Fsm_State_मूल्य>& state : stateSpace.states) {
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
    व्योम processOneBlockAlways(स्थिर FsmComboAlways& oneBlock) {
        स्थिर RegisterAlwaysAnalyzer analyzer{oneBlock.scopep()};
        AstAlways* स्थिर alwaysp = oneBlock.alwaysp();
        अगर (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) वापस;
        स्थिर std::vector<std::pair<AstCase*, Ast_गाँठ_व्यंजक*>> candidates
            = analyzer.oneBlockCandidates(alwaysp);

        FsmCaseCandidate firstCand;
        क्रम (स्थिर std::pair<AstCase*, Ast_गाँठ_व्यंजक*>& cand : candidates) {
            AstVarRef* स्थिर selp = VN_CAST(cand.first->exprp(), VarRef);
            AstVarScope* स्थिर vscp = selp ? selp->varScopep() : nullptr;
            अगर (!vscp) जारी;
            अगर (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processCase(cand.first, vscp, reg);
            } अन्यथा अगर (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed case statements found in "
                                   "the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } अन्यथा {
                cand.first->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported case "
                                   "statements found in the same always block. Only the first "
                                   "candidate will be instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            }
        }

        अगर (firstCand.stateVscp) वापस;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        स्थिर std::vector<std::pair<AstIf*, Ast_गाँठ_व्यंजक*>> ifCandidates
            = analyzer.oneBlockIfCandidates(alwaysp);
        क्रम (स्थिर std::pair<AstIf*, Ast_गाँठ_व्यंजक*>& cand : ifCandidates) {
            स्थिर FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.first);
            FsmIfChainCandidate chain;
            अगर (!collectIfChain(cand.first, aliases, chain)) जारी;
            AstVarScope* स्थिर vscp = chain.compareVscp;
            अगर (!if_Chain_Supported_Transition_गाँठ(chain, vscp)) जारी;
            अगर (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processIfChain(chain, vscp, reg);
            } अन्यथा अगर (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed transition candidates found "
                                   "in the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } अन्यथा {
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
    अटल व्योम warnComboSameAlways(Ast_गाँठ* warnNodep, स्थिर FsmCaseCandidate& firstCand) {
        warnNodep->v3warn(FSMMULTI,
                          "FSM coverage: multiple supported transition candidates found in "
                          "the same combinational always block. Only the first candidate "
                          "will be instrumented."
                              << candidateConflictContext(warnNodep, firstCand));
    }

    व्योम processComboAlways(स्थिर FsmComboAlways& combo) {
        स्थिर ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_गाँठ* स्थिर stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate firstCand;
        क्रम (Ast_गाँठ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* स्थिर casep = VN_CAST(nodep, Case);
            अगर (!casep) जारी;
            स्थिर ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchCase(stmtsp, casep);
            स्थिर FsmRegisterCandidate* स्थिर matchedp = match.matchedp;
            Ast_गाँठ* स्थिर matchedWarnNodep = match.warnNodep;
            अगर (!matchedp) जारी;
            अगर (!firstCand.stateVscp) {
                स्थिर स्वतः जोड़ो_Pair = m_comboPaired.emplace(
                    matchedp->stateVscp(),
                    FsmCaseCandidate{matchedWarnNodep,
                                     const_cast<AstVarScope*>(matchedp->stateVscp())});
                अगर (!जोड़ो_Pair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  जोड़ो_Pair.first->second));
                    जारी;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processCase(casep, matchedp->अग्र_Vscp(), *matchedp);
                जारी;
            }
            अगर (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                जारी;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported case statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, firstCand));
        }
        अगर (firstCand.stateVscp) वापस;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        क्रम (Ast_गाँठ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* स्थिर ifp = VN_CAST(nodep, If);
            अगर (!ifp) जारी;
            FsmIfChainCandidate chain;
            स्थिर FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            अगर (!collectIfChain(ifp, aliases, chain)) जारी;
            स्थिर ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchIfChain(stmtsp, chain);
            स्थिर FsmRegisterCandidate* स्थिर matchedp = match.matchedp;
            Ast_गाँठ* स्थिर matchedWarnNodep = match.warnNodep;
            अगर (!matchedp) जारी;
            अगर (!firstCand.stateVscp) {
                स्थिर std::pair<std::unordered_map<स्थिर AstVarScope*, FsmCaseCandidate>::iterator,
                                bool>
                    जोड़ो_Pair = m_comboPaired.emplace(
                        matchedp->stateVscp(),
                        FsmCaseCandidate{matchedWarnNodep,
                                         const_cast<AstVarScope*>(matchedp->stateVscp())});
                अगर (!जोड़ो_Pair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  जोड़ो_Pair.first->second));
                    जारी;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processIfChain(chain, matchedp->अग्र_Vscp(), *matchedp);
                जारी;
            }
            अगर (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                जारी;
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
    व्योम visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    // Collect processes first, then analyze FSM candidates once all alias and
    // register information is available.
    व्योम visit(AstAlways* nodep) override {
        अगर (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            वापस;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_oneBlockAlwayss.emplace_back(m_scopep, nodep);
        स्थिर RegisterAlwaysAnalyzer analyzer{m_scopep};
        FsmRegisterCandidate reg;
        अगर (analyzer.matchRegisterCandidate(nodep, reg)) {
            AstVarScope* स्थिर stateVscp = reg.stateVscp();
            स्थिर bool found
                = std::any_of(m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                              [stateVscp](स्थिर FsmRegisterCandidate& existing) {
                                  वापस existing.stateVscp() == stateVscp;
                              });
            अगर (!found) { m_registerCandidates.emplace_back(reg); }
        }
        अगर (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            m_comboAlwayss.emplace_back(m_scopep, nodep);
        } अन्यथा अगर (nodep->keyword() == VAlwaysKwd::ALWAYS) {
            अगर (!nodep->sentreep() || isPlainComboSentree(nodep->sentreep())) {
                m_comboAlwayss.emplace_back(m_scopep, nodep);
            } अन्यथा {
                m_nonComboAlwayss.emplace_back(m_scopep, nodep);
            }
        }
    }

    व्योम visit(AstAssignW* nodep) override {
        // Continuous aliases are unordered hardware connections, so source
        // order should not affect whether an if-chain FSM is recognized.
        collectAliasFromAssign(nodep, m_stateAliases, m_ambiguousStateAliases);
        collectCellPortAlias(nodep);
        iterateChildren(nodep);
    }

    व्योम visit(AstCell* nodep) override {
        // Cells are matched after the full traversal because linkdot lowers
        // uninlined port connections into sibling continuous assignments.
        अगर (m_scopep && fsmRegisterWrapperDesc(nodep)) addWrapperCell(m_scopep, nodep);
        iterateChildren(nodep);
    }

    // Continue the walk through the rest of the design hierarchy.
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        क्रम (स्थिर std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            FsmRegisterCandidate reg;
            अगर (matchFsmWrapperCell(wrapperCell.first, wrapperCell.second, reg)) {
                m_registerCandidates.emplace_back(reg);
            }
        }
        क्रम (स्थिर FsmComboAlways& oneBlock : m_oneBlockAlwayss) processOneBlockAlways(oneBlock);
        क्रम (स्थिर FsmComboAlways& combo : m_comboAlwayss) processComboAlways(combo);
        क्रम (स्थिर FsmComboAlways& combo : m_nonComboAlwayss) warnUnsupportedComboAlways(combo);
    }
};

// Builder of statements that will handle incrementation of proper AstNodeCoverDecls - which will
// be also created here
श्रेणी FsmCoverIncBuilder final {
    स्थिर FsmGraph& m_graph;  // Fsm graph
    std::vector<स्थिर FsmStateVertex*> m_states;  // States vertexes inside fsm graph
    std::unordered_set<uint64_t>
        m_undefinedStates;  // States that have no enum value but are possible to represent with a
                            // variable of given width.
                            // This variable keeps uint64_t instead of V3Numebr because it is safe
                            // to assume that nobody has over 16M TB of RAM
    स्थिर FsmVertex* m_defaultVertexp = nullptr;  // Default vertex
    स्थिर FsmVertex* m_resetVertexp = nullptr;  // Reset vertex
    माप_प्रकार m_edges_गिनती = 0;  // Total count of edges
    AstVarScope* स्थिर m_पूर्व_Vscp;  // Previous state of fsm
    File_पंक्ति* स्थिर m_flp;  // Fileline pointer to use across the builder
    स्थिर std::string m_modPrettyName;  // Pretty fsm mode name
    Ast_गाँठ_Stmt* m_coverIncp = nullptr;  // Increments proper CoverDecl after each change
    Ast_गाँठ_Stmt* m_initialCoverIncp = nullptr;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = nullptr;  // List of CoverDecls
    bool m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(स्थिर uint64_t मूल्य) {
        V3_संख्या अंक{static_cast<Ast_गाँठ*>(nullptr), m_पूर्व_Vscp->width()};
        अंक.setQuad(मूल्य);
        अंक.isSigned(m_पूर्व_Vscp->dtypep()->isSigned());
        वापस नया AstConst{m_flp, अंक};
    }

    std::string quadToBitStirngLabel(स्थिर uint64_t val) {
        पूर्णांक width = m_पूर्व_Vscp->width();
        UASSERT(width < 64, "We should not expand values that wide");
        std::string परिणाम = std::to_string(width) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        जबतक (--width >= 0) परिणाम.धकेलो_back("01"[(val >> width) & 1]);
        वापस परिणाम;
    }

    व्योम initExistingStates() {
        क्रम (स्थिर V3GraphVertex& vtx : m_graph.vertices()) {
            स्थिर FsmVertex* स्थिर vertexp = vtx.as<FsmVertex>();
            m_edges_गिनती += vertexp->inEdges().आकार();
            अगर (स्थिर FsmStateVertex* स्थिर statep = vertexp->cast<FsmStateVertex>()) {
                m_states.धकेलो_back(statep);
            } अन्यथा अगर (vertexp->isDefaultAny()) {
                UASSERT_OBJ(!std::exchange(m_defaultVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two defaults?");
            } अन्यथा /* if (vertexp->isResetAny()) */ {
                UASSERT_OBJ(!std::exchange(m_resetVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two resets?");
            }
        }
        स्थिर पूर्णांक width = m_graph.sampleVarScopep()->width();
        m_isComplete = width < 64 && m_states.आकार() == (static_cast<माप_प्रकार>(1) << width);
    }

    व्योम initUndefinedStates() {
        UASSERT_OBJ(m_graph.sampleVarScopep()->width() < 64, m_graph.sampleVarScopep(),
                    "This FSM is too wide to expand - this should be caught earlier");
        स्थिर uint64_t combinations_गिनती = static_cast<माप_प्रकार>(1)
                                           << m_graph.sampleVarScopep()->width();
        // Rolling trick allows to cut peak memory usage
        bool undefRolled = false;
        स्वतः rollUndefs = [यह, combinations_गिनती] {
            क्रम (uint64_t i = 0; i < combinations_गिनती; ++i) {
                स्थिर स्वतः जोड़ो = m_undefinedStates.जोड़ो(i);
                अगर (!जोड़ो.second) m_undefinedStates.erase(जोड़ो.first);
            }
        };
        क्रम (स्थिर FsmStateVertex* स्थिर statep : m_states) {
            स्थिर V3_संख्या& अंक = statep->मूल्य().अंक();
            स्थिर uint64_t मूल्य = अंक.toUQuad();
            अगर (!m_isComplete) {
                अगर (undefRolled) {
                    UASSERT_OBJ(m_undefinedStates.erase(मूल्य) == 1, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                } अन्यथा {
                    UASSERT_OBJ(m_undefinedStates.जोड़ो(मूल्य).second, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                    अगर (m_undefinedStates.आकार() * 2 >= combinations_गिनती) {
                        undefRolled = true;
                        rollUndefs();
                    }
                }
            }
        }
        अगर (!(m_isComplete || undefRolled)) rollUndefs();
    }

    Ast_गाँठ_व्यंजक* buildResetCond() स्थिर {
        Ast_गाँठ_व्यंजक* स्थिर refp
            = नया AstVarRef{m_flp, m_graph.resetCond().varScopep, VAccess::READ};
        वापस m_graph.resetCond().active_निचला
                   ? static_cast<Ast_गाँठ_व्यंजक*>(नया AstLogNot{m_flp, refp})
                   : refp;
    }

    माप_प्रकार get_Arc_गिनती_With_Expansion(स्थिर VFsmExpandType expand) स्थिर {
        माप_प्रकार arc_गिनती_With_Expansion = ~0ULL;
        चयन (expand) {
        हाल VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_graph.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            अवरोध;
        }
        हाल VFsmExpandType::AUTO: {
            arc_गिनती_With_Expansion = m_edges_गिनती;
            अगर (m_defaultVertexp) {
                UASSERT_OBJ(m_states.आकार() >= m_defaultVertexp->outEdges().आकार(),
                            m_graph.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arc_गिनती_With_Expansion -= m_defaultVertexp->outEdges().आकार();
                क्रम (स्थिर FsmStateVertex* स्थिर statep : m_states) {
                    arc_गिनती_With_Expansion += static_cast<माप_प्रकार>(statep->outEdges().empty())
                                             * m_defaultVertexp->outEdges().आकार();
                }
            }
            अगर (m_resetVertexp) {
                UASSERT_OBJ(m_states.आकार() >= m_resetVertexp->outEdges().आकार(),
                            m_graph.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arc_गिनती_With_Expansion += m_states.आकार() - m_resetVertexp->outEdges().आकार();
            }
            अवरोध;
        }
        हाल VFsmExpandType::AUTO_EXPAND: {
            arc_गिनती_With_Expansion = m_states.आकार() * m_states.आकार() - m_states.आकार();
            अवरोध;
        }
        हाल VFsmExpandType::FULL: {
            स्थिर पूर्णांक width = m_graph.sampleVarScopep()->width();
            // 32 is derived from:
            // tranitionsCount <= std::numeric_limits<uint64_t>::max()
            // (2^width)^2 - (2^width) <= 2^64 - 1
            // (2^width)^2 - (2^width) < 2^64 since we operate on integral values we can trade 1
            // log_2((2^width)^2 - (2^width)) < 64
            // log_2((2^width)^2) < 64 - `(2^width)` is negigible so, it may be dropped
            // 2width < 64
            अगर (width < 32) {
                स्थिर uint64_t combinations_गिनती = static_cast<माप_प्रकार>(1) << width;
                arc_गिनती_With_Expansion = combinations_गिनती * combinations_गिनती - combinations_गिनती;
            }
            अवरोध;
        }
        }
        वापस arc_गिनती_With_Expansion;
    }

    AstCoverOtherDecl* buildStateCoverDecl(स्थिर std::string& label,
                                           स्थिर std::string& fsmTag = "") {
        AstCoverOtherDecl* स्थिर declp
            = नया AstCoverOtherDecl{m_flp,
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
        m_declsp = Ast_गाँठ::add_अग्र(m_declsp, declp);
        वापस declp;
    }

    Ast_गाँठ_Stmt* buildCoverIncStmtp(स्थिर std::string& fromStateLabel,
                                    स्थिर std::string& toStateLabel,
                                    स्थिर std::string& baseLabel = "") {
        // Whole builder is designed in such a way that this function won't be called twice with
        // the same arguments - but if it will ever change in future, `AstCoverOtherDecl` shall be
        // cached for particular args set
        AstCoverOtherDecl* स्थिर declp = नया AstCoverOtherDecl{
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
        m_declsp = Ast_गाँठ::add_अग्र(m_declsp, declp);
        वापस नया AstCoverInc{m_flp, declp};
    }
    Ast_गाँठ_Stmt* buildExpandedTransitionHandler(स्थिर std::string& toStateLabel,
                                                स्थिर bool considerUndefs,
                                                स्थिर bool expandOnlyWithTerminalVerticies,
                                                स्थिर FsmStateVertex* स्थिर toStatep = nullptr) {
        AstCase* स्थिर arcCasep = नया AstCase{
            m_flp, VCaseType::CT_CASE, नया AstVarRef{m_flp, m_पूर्व_Vscp, VAccess::READ}, nullptr};
        std::unordered_set<स्थिर FsmStateVertex*> handledSoureces;
        अगर (toStatep) {
            क्रम (स्थिर V3GraphEdge& edge : toStatep->inEdges()) {
                अगर (स्थिर FsmStateVertex* स्थिर fromp = edge.fromp()->cast<FsmStateVertex>()) {
                    अगर (!handledSoureces.emplace(fromp).second) जारी;
                    arcCasep->addItemsp(नया AstCaseItem{
                        m_flp, makeConst(fromp->मूल्य().अंक().toUQuad()),
                        (fromp != toStatep) ? buildCoverIncStmtp(fromp->label(), toStateLabel)
                                            : nullptr});
                }
            }
        }
        क्रम (स्थिर FsmStateVertex* स्थिर fromp : m_states) {
            अगर (handledSoureces.find(fromp) != handledSoureces.end()) जारी;
            arcCasep->addItemsp(नया AstCaseItem{
                m_flp, makeConst(fromp->मूल्य().अंक().toUQuad()),
                (fromp != toStatep
                 && (!expandOnlyWithTerminalVerticies || fromp->outEdges().empty()))
                    ? buildCoverIncStmtp(fromp->label(), toStateLabel, "artificial")
                    : nullptr});
        }
        अगर (!m_isComplete) {
            अगर (considerUndefs) {
                क्रम (स्थिर uint64_t from : m_undefinedStates) {
                    स्थिर std::string& label = quadToBitStirngLabel(from);
                    arcCasep->addItemsp(
                        नया AstCaseItem{m_flp, makeConst(from),
                                        (label != toStateLabel)
                                            ? buildCoverIncStmtp(label, toStateLabel, "artificial")
                                            : nullptr});
                }
            } अन्यथा {
                arcCasep->addItemsp(नया AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        वापस arcCasep;
    }
    Ast_गाँठ_Stmt* buildUnxpandedTransitionHandler(स्थिर FsmStateVertex* स्थिर toStatep) {
        AstCase* स्थिर arcCasep = नया AstCase{
            m_flp, VCaseType::CT_CASE, नया AstVarRef{m_flp, m_पूर्व_Vscp, VAccess::READ}, nullptr};
        क्रम (स्थिर V3GraphEdge& edge : toStatep->inEdges()) {
            स्थिर FsmArcEdge* स्थिर arcp = edge.as<FsmArcEdge>();
            UASSERT_OBJ(!arcp->isReset(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            स्थिर FsmStateVertex* स्थिर fromp = arcp->fromp()->as<FsmStateVertex>();
            अगर (fromp != toStatep) {
                arcCasep->addItemsp(
                    नया AstCaseItem{m_flp, makeConst(fromp->मूल्य().अंक().toUQuad()),
                                    buildCoverIncStmtp(fromp->label(), toStatep->label())});
            }
        }
        arcCasep->addItemsp(नया AstCaseItem{m_flp, nullptr, nullptr});
        वापस arcCasep;
    }

खुला:
    FsmCoverIncBuilder(स्थिर FsmGraph& graph, AstVarScope* स्थिर पूर्व_Vscp)
        : m_graph{graph}
        , m_पूर्व_Vscp{पूर्व_Vscp}
        , m_flp{graph.fileline()}
        , m_modPrettyName{graph.scopep()->modp()->prettyName()} {
        initExistingStates();
        AstVarScope* स्थिर sampleVscp = graph.sampleVarScopep();
        स्थिर VFsmExpandType expand
            = graph.stateVarScopep()->varp()->attrFsmStateExpand() != VFsmExpandType::DEFAULT
                  ? graph.stateVarScopep()->varp()->attrFsmStateExpand()
                  : v3Global.opt.coverageFsmExpand();

        {
            स्थिर माप_प्रकार arc_गिनती_With_Expansion = get_Arc_गिनती_With_Expansion(expand);
            अगर (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arc_गिनती_With_Expansion == 0ULL
                            || arc_गिनती_With_Expansion
                                   > static_cast<माप_प्रकार>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arc_गिनती_With_Expansion);
                वापस;
            }
        }

        AstCase* स्थिर arcCasep
            = नया AstCase{m_flp, VCaseType::CT_CASE,
                          नया AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};
        AstCase* स्थिर initialStateCasep
            = नया AstCase{m_flp, VCaseType::CT_CASE,
                          नया AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};

        स्थिर bool expandAll = expand != VFsmExpandType::AUTO;
        std::unordered_set<स्थिर FsmStateVertex*>
            defaultTargets;  // States that have edge incoming from default vertex
        std::unordered_set<स्थिर FsmStateVertex*>
            resetTargets;  // States that have edge incoming from reset vertex
        अगर (!expandAll) {
            अगर (m_defaultVertexp) {
                क्रम (स्थिर V3GraphEdge& edge : m_defaultVertexp->outEdges()) {
                    defaultTargets.emplace(edge.शीर्ष()->as<FsmStateVertex>());
                }
            }
            अगर (m_resetVertexp) {
                क्रम (स्थिर V3GraphEdge& edge : m_resetVertexp->outEdges()) {
                    resetTargets.emplace(edge.शीर्ष()->as<FsmStateVertex>());
                }
            }
        }
        स्थिर bool isFullExpansion = expand == VFsmExpandType::FULL;
        अगर (isFullExpansion) initUndefinedStates();
        क्रम (स्थिर FsmStateVertex* स्थिर statep : m_states) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* स्थिर declp = buildStateCoverDecl(statep->label());
            initialStateCasep->addItemsp(
                नया AstCaseItem{m_flp, makeConst(statep->मूल्य().अंक().toUQuad()),
                                नया AstCoverInc{m_flp, declp, true}});
            स्थिर bool expandAllOrReset
                = expandAll || (resetTargets.find(statep) != resetTargets.end());
            स्थिर bool defaultExpantion
                = !expandAllOrReset && (defaultTargets.find(statep) != defaultTargets.end());
            arcCasep->addItemsp(नया AstCaseItem{
                m_flp, makeConst(statep->मूल्य().अंक().toUQuad()),
                Ast_गाँठ::add_अग्र<Ast_गाँठ_Stmt>(
                    नया AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), isFullExpansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        अगर (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            अगर (isFullExpansion) {
                क्रम (स्थिर uint64_t मूल्य : m_undefinedStates) {
                    स्थिर std::string& label = quadToBitStirngLabel(मूल्य);
                    AstCoverOtherDecl* स्थिर declp = buildStateCoverDecl(label);
                    initialStateCasep->addItemsp(नया AstCaseItem{
                        m_flp, makeConst(मूल्य), नया AstCoverInc{m_flp, declp, true}});
                    arcCasep->addItemsp(
                        नया AstCaseItem{m_flp, makeConst(मूल्य),
                                        Ast_गाँठ::add_अग्र<Ast_गाँठ_Stmt>(
                                            नया AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, true, false))});
                }
            } अन्यथा {
                // No expansion
                arcCasep->addItemsp(नया AstCaseItem{m_flp, nullptr, nullptr});
                initialStateCasep->addItemsp(नया AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        m_coverIncp = arcCasep;
        m_initialCoverIncp = initialStateCasep;
    }
    VL_UNCOPYABLE(FsmCoverIncBuilder);
    ~FsmCoverIncBuilder() = शेष;
    Ast_गाँठ_Cover_Decl* getCoverDeclsp() स्थिर { वापस m_declsp; }
    Ast_गाँठ_Stmt* getCoverIncp() स्थिर { वापस m_coverIncp; }
    Ast_गाँठ_Stmt* getInitialCoverIncp() स्थिर { वापस m_initialCoverIncp; }
};

// Lower the completed FSM graphs into the concrete coverage declarations,
// previous-state tracking, and pre/post-triggered instrumentation that the
// runtime uses to record state and transition coverage.
श्रेणी FsmLowerVisitor final {
    // STATE - across all visitors
    स्थिर FsmState& m_state;
    V3SharedTmps m_पूर्व_Tmps{"__VfsmCovPrev", VVarType::MODULETEMP};  // Previous states

    // METHODS
    // Rebuild the original event control from the saved sense description so
    // post-state coverage sampling runs on the same triggering edges.
    अटल Ast_Sen_वृक्ष* build_Sen_वृक्ष(File_पंक्ति* flp, स्थिर std::vector<FsmSenDesc>& senses) {
        Ast_Sen_वृक्ष* स्थिर sentreep = नया Ast_Sen_वृक्ष{flp, nullptr};
        क्रम (स्थिर FsmSenDesc& sense : senses) {
            AstSenItem* स्थिर senItemp
                = नया AstSenItem{flp, VEdgeType{sense.edgeType},
                                 नया AstVarRef{flp, sense.varScopep, VAccess::READ}};
            sentreep->addSensesp(senItemp);
        }
        वापस sentreep;
    }

    // Lower one fully detected FSM graph into the concrete coverage machinery
    // used by generated models: declarations, previous-state tracking, and the
    // pre/post-triggered increment logic for states and arcs.
    व्योम buildOne(स्थिर FsmGraph& graph) {
        UINFO(1, "buildOne lowering FSM " << graph.stateVarName()
                                          << " vertices=" << graph.vertices().आकार() << प_पूर्ण);
        AstAlways* स्थिर alwaysp = graph.stateAlwaysp();
        AstScope* स्थिर scopep = graph.scopep();
        AstVarScope* स्थिर stateVscp = graph.stateVarScopep();
        AstVarScope* स्थिर sampleVscp = graph.sampleVarScopep();
        File_पंक्ति* स्थिर flp = graph.fileline();
        Ast_गाँठ_Module* स्थिर modp = scopep->modp();
        AstVarScope* स्थिर पूर्व_Vscp
            = m_पूर्व_Tmps.make(flp, scopep, sampleVscp->dtypep(), stateVscp->varp()->shortName());
        // The saved previous-state temp crosses the scheduler's pre/post split
        // in the same way as Verilator's built-in NBA shadow variables, so keep
        // both vars marked as post-life participants for stable MT ordering.
        stateVscp->optimizeLifePost(true);
        sampleVscp->optimizeLifePost(true);
        पूर्व_Vscp->optimizeLifePost(true);

        AstActive* स्थिर initActivep
            = नया AstActive{flp, "fsm-coverage-init",
                            नया Ast_Sen_वृक्ष{flp, नया AstSenItem{flp, AstSenItem::Initial{}}}};
        initActivep->sen_वृक्ष_Storep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->addStmtsp(नया AstInitialStatic{
            flp, नया AstAssign{flp, नया AstVarRef{flp, पूर्व_Vscp, VAccess::WRITE},
                               नया AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->addBlocksp(initActivep);

        AstAlwaysPost* स्थिर covPostp = नया AstAlwaysPost{flp};
        bool update_पूर्व_After_Post = false;
        अगर (alwaysp) {
            // Save the previous state as plain sequential logic at the front of
            // the original always_ff body, then evaluate coverage in post logic
            // after the delayed state update commits. This avoids a scheduler
            // race between a separate AstAlwaysPre task and the real state
            // commit for direct parent-level registers.
            alwaysp->prependStmtsp(नया AstAssign{flp, नया AstVarRef{flp, पूर्व_Vscp, VAccess::WRITE},
                                                 नया AstVarRef{flp, sampleVscp, VAccess::READ}});
        } अन्यथा {
            // Wrapper-derived register candidates do not have a parent
            // always_ff body to splice into. Sample coverage first, then save
            // the current state for the next clock tick; this survives cell
            // boundary scheduling where the real flop update lives elsewhere.
            update_पूर्व_After_Post = true;
            पूर्व_Vscp->varp()->setIgnorePostRead();
        }

        {
            // Create and increment proper AstNodeCoverDecls
            स्थिर FsmCoverIncBuilder coverIncBuilder{graph, पूर्व_Vscp};
            modp->addStmtsp(coverIncBuilder.getCoverDeclsp());
            initActivep->addStmtsp(coverIncBuilder.getInitialCoverIncp());
            covPostp->addStmtsp(
                नया AstIf{flp,
                          नया AstNeq{flp, नया AstVarRef{flp, पूर्व_Vscp, VAccess::READ},
                                     नया AstVarRef{flp, sampleVscp, VAccess::READ}},
                          coverIncBuilder.getCoverIncp()});
        }

        अगर (update_पूर्व_After_Post) {
            covPostp->addStmtsp(नया AstAssign{flp, नया AstVarRef{flp, पूर्व_Vscp, VAccess::WRITE},
                                              नया AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        Ast_Sen_वृक्ष* स्थिर sentreep = build_Sen_वृक्ष(flp, graph.senses());
        AstActive* स्थिर activep = नया AstActive{flp, "fsm-coverage", sentreep};
        activep->sen_वृक्ष_Storep(sentreep);
        scopep->addBlocksp(activep);
        activep->addStmtsp(covPostp);
    }

खुला:
    // CONSTRUCTORS
    // Lower every detected FSM graph from the shared local state into
    // concrete coverage instrumentation while the saved scoped pointers are
    // still valid in the same pass.
    explicit FsmLowerVisitor(स्थिर FsmState& state)
        : m_state{state} {
        क्रम (स्थिर DetectedFsm& fsm : m_state.fsms()) { buildOne(*fsm.graphp); }
    }
};

}  // namespace

व्योम V3FsmDetect::detect(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    FsmState state;
    // Phase 1: recover each supported FSM into a complete graph while the
    // original clocked/case structure is still easy to recognize.
    FsmDetectVisitor detect{state, rootp};
    अगर (dumpGraphLevel() >= 6) {
        माप_प्रकार सूचकांक_2 = 0;
        क्रम (स्थिर DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dumpTag(सूचकांक_2++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { FsmLowerVisitor lower{state}; }
    V3Global::dump_Check_Global_वृक्ष("fsm-detect", 0, dump_वृक्ष_Either_Level() >= 3);
}
