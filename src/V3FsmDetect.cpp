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

#અન્તર્ભાવ "V3PchAstNoMT.h"

#અન્તર્ભાવ "V3FsmDetect.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Control.h"
#અન્તર્ભાવ "V3Graph.h"
#અન્તર્ભાવ "V3SharedTmps.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <cctype>
#અન્તર્ભાવ <map>
#અન્તર્ભાવ <memory>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

નામાકાશ {

// Width-preserving FSM state identity. FSM detection needs a stable key for
// graph vertices and lookup tables, but lowering still needs the original
// folded Verilog value so emitted comparisons keep the correct width and bits.
વર્ગ Fsm_State_કિંમત final {
    // Hash/equality key only. It deliberately ignores signedness because
    // signed and unsigned constants with the same width and bits denote the
    // same encoded FSM state.
    string m_ચાવી;  // Canonical "width:value" identity, independent of signedness

    // Semantic value. This is what diagnostics and lowering use when printing
    // values or rebuilding AstConst nodes for instrumentation.
    V3_સંખ્યા m_અંક;  // Original folded value, preserving width for lowered comparisons

    સ્થિર string make_ચાવી(અચલ V3_સંખ્યા& અંક) {
        V3_સંખ્યા ચાવી_અંક = અંક;
        // Signedness does not change FSM state identity: same width and bits
        // should address the same graph vertex.
        ચાવી_અંક.isSigned(ખોટું);
        પાછા cvtToStr(ચાવી_અંક.width()) + ":" + ચાવી_અંક.ascii(સાચું, સાચું);
    }

જાહેર:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    Fsm_State_કિંમત()
        : m_ચાવી{"1:1'h0"}
        , m_અંક{static_cast<Ast_ગાંઠ*>(nullptr), 1, 0} {}
    explicit Fsm_State_કિંમત(અચલ V3_સંખ્યા& અંક)
        : m_ચાવી{make_ચાવી(અંક)}
        , m_અંક{અંક} {}

    અચલ string& ચાવી() અચલ { પાછા m_ચાવી; }
    અચલ V3_સંખ્યા& અંક() અચલ { પાછા m_અંક; }
    string ascii() અચલ { પાછા m_અંક.ascii(સાચું, સાચું); }
    string warnText() અચલ {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        જો (m_અંક.width() <= 32) પાછા cvtToStr(m_અંક.toUInt());
        પાછા ascii();
    }

    તાર્કિક operator==(અચલ Fsm_State_કિંમત& rhs) અચલ { પાછા m_ચાવી == rhs.m_ચાવી; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
સંરચના Fsm_State_કિંમત_Hash final {
    size_t operator()(અચલ Fsm_State_કિંમત& કિંમત) અચલ {
        પાછા std::hash<string>{}(કિંમત.ચાવી());
    }
};

// Captures one sensitivity-list entry so the lowering phase can later rebuild
// an active block with the same triggering event control.
સંરચના FsmSenDesc final {
    // Encoded edge kind copied from AstSenItem::edgeType() so lowering can
    // rebuild the same trigger semantics on the synthesized coverage block.
    VEdgeType::en edgeType = static_cast<VEdgeType::en>(0);
    // Triggering signal in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
};

// Captures the simple reset predicate shape that survives to this pass after
// earlier normalization so reset arcs can be reconstructed during lowering.
સંરચના FsmResetCondDesc final {
    // Reset signal used by the FSM in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
    તાર્કિક active_નીચલું = ખોટું;
};

વર્ગ FsmResetArcDesc final {
    Fsm_State_કિંમત m_to_કિંમત;  // Encoded reset target state.
    Ast_ગાંઠ* m_nodep = nullptr;  // Source node for warnings and emitted metadata.
    Ast_ગાંઠ_પદાવલિ* m_valuep = nullptr;  // Expression that provided the reset value.

જાહેર:
    FsmResetArcDesc() = મૂળભૂત;
    FsmResetArcDesc(Fsm_State_કિંમત to_કિંમત, Ast_ગાંઠ_Assign* nodep)
        : m_to_કિંમત{to_કિંમત}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(Fsm_State_કિંમત to_કિંમત, Ast_ગાંઠ* nodep, Ast_ગાંઠ_પદાવલિ* valuep)
        : m_to_કિંમત{to_કિંમત}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    Fsm_State_કિંમત to_કિંમત() અચલ { પાછા m_to_કિંમત; }
    Ast_ગાંઠ* nodep() અચલ { પાછા m_nodep; }
    Ast_ગાંઠ_પદાવલિ* valuep() અચલ { પાછા m_valuep; }
};

સંરચના FsmWrapperRoles final {
    string dPort;
    string qPort;
    string clkPort;
    string rstPort;
    string rstValParam;
    તાર્કિક has_Rst_Active_નીચલું = ખોટું;
    તાર્કિક rst_Active_નીચલું = ખોટું;
};

સ્થિર તાર્કિક fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, અચલ string& portName,
                                                  તાર્કિક& active_નીચલું) {
    તાર્કિક matched = ખોટું;
    cellp->modp()->foreach([&](AstSenItem* itemp) {
        Ast_ગાંઠ_Var_Ref* અચલ vrefp = itemp->varrefp();
        જો (!vrefp) પાછા;
        જો (vrefp->varp()->name() != portName) પાછા;
        active_નીચલું = itemp->edgeType() == VEdgeType::ET_NEGEDGE;
        matched = સાચું;
    });
    પાછા matched;
}

સ્થિર અચલ V3Control::FsmRegisterWrapper* fsmRegisterWrapperDesc(AstCell* cellp) {
    Ast_ગાંઠ_Module* અચલ modp = cellp->modp();
    અચલ string origName = modp->origName();
    જો (અચલ V3Control::FsmRegisterWrapper* અચલ descp
        = V3Control::getFsmRegisterWrapper(origName)) {
        પાછા descp;
    }
    પાછા V3Control::getFsmRegisterWrapper(modp->prettyDehashOrigOrName());
}

સ્થિર FsmWrapperRoles rolesFromDesc(અચલ V3Control::FsmRegisterWrapper& desc) {
    FsmWrapperRoles roles;
    roles.dPort = desc.d;
    roles.qPort = desc.q;
    roles.clkPort = desc.clock;
    roles.rstPort = desc.reset;
    roles.rstValParam = desc.reset_કિંમત;
    પાછા roles;
}

વર્ગ FsmRegisterCandidate final {
    AstScope* m_scopep = nullptr;  // Owning scope for the paired FSM.
    AstAlways* m_alwaysp = nullptr;  // Register process that commits the state.
    AstVarScope* m_stateVscp = nullptr;  // Registered FSM state variable.
    AstVarScope* m_sampleVscp = nullptr;  // Variable sampled by lowered coverage logic.
    AstVarScope* m_nextVscp = nullptr;  // Next-state variable or same state var for 1-block FSMs.
    std::vector<FsmSenDesc> m_senses;  // Event controls for recreated coverage blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate, if any.
    std::vector<FsmResetArcDesc> m_resetArcs;  // Reset target arcs recovered during detect.
    તાર્કિક m_hasResetCond = ખોટું;  // Whether the FSM had a modeled reset predicate.
    તાર્કિક m_inclCond = ખોટું;  // Whether conditional/default arcs are kept explicitly.
    File_લીટી* m_flp = nullptr;  // Representative source location.

જાહેર:
    AstScope* scopep() અચલ { પાછા m_scopep; }
    શૂન્ય scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* alwaysp() અચલ { પાછા m_alwaysp; }
    શૂન્ય alwaysp(AstAlways* alwaysp) { m_alwaysp = alwaysp; }
    AstVarScope* stateVscp() અચલ { પાછા m_stateVscp; }
    શૂન્ય stateVscp(AstVarScope* vscp) { m_stateVscp = vscp; }
    AstVarScope* sampleVscp() અચલ { પાછા m_sampleVscp ? m_sampleVscp : m_stateVscp; }
    શૂન્ય sampleVscp(AstVarScope* vscp) { m_sampleVscp = vscp; }
    AstVarScope* nextVscp() અચલ { પાછા m_nextVscp; }
    શૂન્ય nextVscp(AstVarScope* vscp) { m_nextVscp = vscp; }
    અચલ std::vector<FsmSenDesc>& senses() અચલ { પાછા m_senses; }
    std::vector<FsmSenDesc>& senses() { પાછા m_senses; }
    અચલ FsmResetCondDesc& resetCond() અચલ { પાછા m_resetCond; }
    FsmResetCondDesc& resetCond() { પાછા m_resetCond; }
    અચલ std::vector<FsmResetArcDesc>& resetArcs() અચલ { પાછા m_resetArcs; }
    std::vector<FsmResetArcDesc>& resetArcs() { પાછા m_resetArcs; }
    તાર્કિક hasResetCond() અચલ { પાછા m_hasResetCond; }
    શૂન્ય hasResetCond(તાર્કિક flag) { m_hasResetCond = flag; }
    તાર્કિક inclCond() અચલ { પાછા m_inclCond; }
    શૂન્ય inclCond(તાર્કિક flag) { m_inclCond = flag; }
    File_લીટી* fileline() અચલ { પાછા m_flp; }
    શૂન્ય fileline(File_લીટી* flp) { m_flp = flp; }
};

વર્ગ FsmComboAlways final {
    AstScope* અચલ m_scopep = nullptr;  // Owning scope for the combinational process.
    AstAlways* અચલ m_alwaysp = nullptr;  // Candidate transition process.

જાહેર:
    FsmComboAlways() = મૂળભૂત;
    FsmComboAlways(AstScope* scopep, AstAlways* alwaysp)
        : m_scopep{scopep}
        , m_alwaysp{alwaysp} {}

    AstScope* scopep() અચલ { પાછા m_scopep; }
    AstAlways* alwaysp() અચલ { પાછા m_alwaysp; }
};

વર્ગ FsmGraph;

વર્ગ FsmVertex VL_NOT_FINAL : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(FsmVertex, V3GraphVertex)

જાહેર:
    ગણના વર્ગ Kind : uint8_t { STATE, RESET_ANY, DEFAULT_ANY };

ખાનગી:
    Kind m_kind;  // State vs synthetic ANY/default vertex role.
    string m_label;  // User-facing state or pseudo-state label.
    Fsm_State_કિંમત m_કિંમત;  // Encoded state value for real state vertices.

સુરક્ષિત:
    FsmVertex(V3Graph* graphp, Kind kind, string label, Fsm_State_કિંમત કિંમત) VL_MT_DISABLED
        : V3GraphVertex{graphp},
          m_kind{kind},
          m_label{label},
          m_કિંમત{કિંમત} {}
    ~FsmVertex() override = મૂળભૂત;

જાહેર:
    Kind kind() અચલ { પાછા m_kind; }
    તાર્કિક isState() અચલ { પાછા m_kind == Kind::STATE; }
    તાર્કિક isResetAny() અચલ { પાછા m_kind == Kind::RESET_ANY; }
    તાર્કિક isDefaultAny() અચલ { પાછા m_kind == Kind::DEFAULT_ANY; }
    અચલ string& label() અચલ { પાછા m_label; }
    અચલ Fsm_State_કિંમત& કિંમત() અચલ { પાછા m_કિંમત; }

    string name() અચલ override VL_MT_SAFE { પાછા m_label + "=" + m_કિંમત.ascii(); }
};

વર્ગ FsmStateVertex final : જાહેર FsmVertex {
    VL_RTTI_IMPL(FsmStateVertex, FsmVertex)

જાહેર:
    FsmStateVertex(V3Graph* graphp, string label, Fsm_State_કિંમત કિંમત) VL_MT_DISABLED
        : FsmVertex{graphp, Kind::STATE, label, કિંમત} {}
    ~FsmStateVertex() override = મૂળભૂત;

    string dotColor() અચલ override { પાછા "lightblue"; }
    string dotShape() અચલ override { પાછા "ellipse"; }
};

વર્ગ FsmPseudoVertex final : જાહેર FsmVertex {
    VL_RTTI_IMPL(FsmPseudoVertex, FsmVertex)

જાહેર:
    FsmPseudoVertex(V3Graph* graphp, Kind kind, string label) VL_MT_DISABLED
        : FsmVertex{graphp, kind, label, Fsm_State_કિંમત{}} {}
    ~FsmPseudoVertex() override = મૂળભૂત;

    string name() અચલ override VL_MT_SAFE { પાછા label(); }
    string dotColor() અચલ override { પાછા isResetAny() ? "darkgreen" : "orange"; }
    string dotShape() અચલ override { પાછા "diamond"; }
};

વર્ગ FsmArcEdge final : જાહેર V3GraphEdge {
    VL_RTTI_IMPL(FsmArcEdge, V3GraphEdge)
    તાર્કિક m_isReset = ખોટું;  // Arc originates from the synthetic reset source.
    તાર્કિક m_isCond = ખોટું;  // Arc came from a conditional next-state split.
    તાર્કિક m_isDefault = ખોટું;  // Arc represents a case default source.
    File_લીટી* m_flp = nullptr;  // Source location for emitted coverage metadata.

જાહેર:
    FsmArcEdge(V3Graph* graphp, FsmVertex* fromp, FsmStateVertex* ટોચ, તાર્કિક isReset, તાર્કિક isCond,
               તાર્કિક isDefault, File_લીટી* flp) VL_MT_DISABLED : V3GraphEdge{graphp, fromp, ટોચ, 1},
                                                               m_isReset{isReset},
                                                               m_isCond{isCond},
                                                               m_isDefault{isDefault},
                                                               m_flp{flp} {}
    ~FsmArcEdge() override = મૂળભૂત;

    તાર્કિક isReset() અચલ { પાછા m_isReset; }
    તાર્કિક isCond() અચલ { પાછા m_isCond; }
    તાર્કિક isDefault() અચલ { પાછા m_isDefault; }
    File_લીટી* fileline() અચલ { પાછા m_flp; }

    string dotLabel() અચલ override {
        જો (m_isReset) પાછા "reset";
        જો (m_isDefault) પાછા "default";
        જો (m_isCond) પાછા "cond";
        પાછા "";
    }
    string dotColor() અચલ override {
        જો (m_isReset) પાછા "darkgreen";
        જો (m_isDefault) પાછા "orange";
        જો (m_isCond) પાછા "blue";
        પાછા "black";
    }
};

// One graph per detected FSM. Graph-level metadata captures the non-graph
// context needed to lower states/arcs back into the AST after detection.
વર્ગ FsmGraph final : જાહેર V3Graph {
    AstScope* m_scopep = nullptr;  // Owning scoped block for the detected FSM.
    AstAlways* m_stateAlwaysp = nullptr;  // Register always block being instrumented.
    string m_stateVarName;  // Pretty state variable name for user-visible output.
    string m_stateVarInternalName;  // Internal state symbol name for dump tags.
    AstVarScope* m_stateVarScopep = nullptr;  // Scoped state variable being tracked.
    AstVarScope* m_sampleVarScopep = nullptr;  // Scoped variable sampled by coverage logic.
    std::vector<FsmSenDesc> m_senses;  // Saved event controls for recreated active blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate shape, if one exists.
    તાર્કિક m_hasResetCond = ખોટું;  // Whether the detected FSM had a reset branch.
    તાર્કિક m_inclCond = ખોટું;  // Whether conditional arcs should be kept explicitly.
    File_લીટી* m_flp = nullptr;  // Representative source location for declarations/arcs.
    std::unordered_map<Fsm_State_કિંમત, FsmStateVertex*, Fsm_State_કિંમત_Hash>
        m_stateVertices;  // Value to state map.
    FsmPseudoVertex* m_resetVertexp = nullptr;  // Synthetic ANY source for reset arcs.
    FsmPseudoVertex* m_defaultVertexp = nullptr;  // Synthetic default source for case defaults.

જાહેર:
    FsmGraph() VL_MT_DISABLED
        : m_resetVertexp{નવું FsmPseudoVertex{આ, FsmVertex::Kind::RESET_ANY, "ANY"}},
          m_defaultVertexp{નવું FsmPseudoVertex{આ, FsmVertex::Kind::DEFAULT_ANY, "default"}} {}

    AstScope* scopep() અચલ { પાછા m_scopep; }
    શૂન્ય scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* stateAlwaysp() અચલ { પાછા m_stateAlwaysp; }
    શૂન્ય stateAlwaysp(AstAlways* alwaysp) { m_stateAlwaysp = alwaysp; }
    અચલ string& stateVarName() અચલ { પાછા m_stateVarName; }
    શૂન્ય stateVarName(અચલ string& name) { m_stateVarName = name; }
    અચલ string& stateVarInternalName() અચલ { પાછા m_stateVarInternalName; }
    શૂન્ય stateVarInternalName(અચલ string& name) { m_stateVarInternalName = name; }
    AstVarScope* stateVarScopep() અચલ { પાછા m_stateVarScopep; }
    શૂન્ય stateVarScopep(AstVarScope* vscp) { m_stateVarScopep = vscp; }
    AstVarScope* sampleVarScopep() અચલ {
        પાછા m_sampleVarScopep ? m_sampleVarScopep : m_stateVarScopep;
    }
    શૂન્ય sampleVarScopep(AstVarScope* vscp) { m_sampleVarScopep = vscp; }
    અચલ std::vector<FsmSenDesc>& senses() અચલ { પાછા m_senses; }
    std::vector<FsmSenDesc>& senses() { પાછા m_senses; }
    અચલ FsmResetCondDesc& resetCond() અચલ { પાછા m_resetCond; }
    FsmResetCondDesc& resetCond() { પાછા m_resetCond; }
    તાર્કિક hasResetCond() અચલ { પાછા m_hasResetCond; }
    શૂન્ય hasResetCond(તાર્કિક flag) { m_hasResetCond = flag; }
    તાર્કિક inclCond() અચલ { પાછા m_inclCond; }
    શૂન્ય inclCond(તાર્કિક flag) { m_inclCond = flag; }
    File_લીટી* fileline() અચલ { પાછા m_flp; }
    શૂન્ય fileline(File_લીટી* flp) { m_flp = flp; }

    FsmStateVertex* addStateVertex(string label, Fsm_State_કિંમત કિંમત) VL_MT_DISABLED {
        FsmStateVertex* અચલ vertexp = નવું FsmStateVertex{આ, label, કિંમત};
        m_stateVertices.emplace(કિંમત, vertexp);
        પાછા vertexp;
    }
    FsmPseudoVertex* resetAnyVertex() VL_MT_DISABLED { પાછા m_resetVertexp; }
    FsmPseudoVertex* defaultAnyVertex() VL_MT_DISABLED { પાછા m_defaultVertexp; }
    FsmArcEdge* addArc(Fsm_State_કિંમત from_કિંમત, Fsm_State_કિંમત to_કિંમત, તાર્કિક isReset, તાર્કિક isCond,
                       તાર્કિક isDefault, File_લીટી* flp) VL_MT_DISABLED {
        FsmStateVertex* અચલ ટોચ = m_stateVertices.at(to_કિંમત);
        FsmVertex* fromp = nullptr;
        જો (isReset) {
            fromp = resetAnyVertex();
        } નહીંતર જો (isDefault) {
            fromp = defaultAnyVertex();
        } નહીંતર {
            fromp = m_stateVertices.at(from_કિંમત);
        }
        પાછા નવું FsmArcEdge{આ, fromp, ટોચ, isReset, isCond, isDefault, flp};
    }

    string name() અચલ VL_MT_SAFE {
        પાછા "FSM "
               + (m_stateVarName.empty() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dumpTag(size_t index) અચલ {
        string tag = stateVarInternalName();
        માટે (અક્ષર& ch : tag) {
            જો (!std::isalnum(static_cast<unsigned અક્ષર>(ch))) ch = '_';
        }
        પાછા "fsm_" + cvtToStr(index) + "_" + tag;
    }
};

સંરચના DetectedFsm final {
    std::unique_ptr<FsmGraph> graphp;  // Extracted graph for one detected FSM candidate.
};

સંરચના FsmCaseCandidate final {
    Ast_ગાંઠ* warnNodep = nullptr;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = nullptr;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
સંરચના FsmStateComparison final {
    AstVarScope* stateVscp = nullptr;  // Compared state variable
    Ast_ગાંઠ_પદાવલિ* valuep = nullptr;  // Compared constant value expression
    Fsm_State_કિંમત કિંમત;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
સંરચના FsmIfBranch final {
    AstIf* ifp = nullptr;  // Source if/else-if node
    Ast_ગાંઠ* stmtsp = nullptr;  // Branch body
    Ast_ગાંઠ_પદાવલિ* valuep = nullptr;  // Source state value expression
    Fsm_State_કિંમત from_કિંમત;  // Encoded source state value
    તાર્કિક has_ટોચ_Guard = ખોટું;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
સંરચના FsmIfChainCandidate final {
    AstIf* ifp = nullptr;  // Top-level if-chain node
    AstVarScope* compareVscp = nullptr;  // Variable used by every state comparison
    std::vector<FsmIfBranch> branches;  // Recognized state-dispatch branches
    Ast_ગાંઠ* defaultStmtsp = nullptr;  // Optional final else body
};

// Aliases are accepted only when they are equivalent to spelling the state
// comparison inline; this avoids inferring FSM semantics from arbitrary logic.
using FsmAliasMap = std::unordered_map<અચલ AstVarScope*, FsmStateComparison>;
using FsmCellPortMap = std::unordered_map<string, AstVarScope*>;
using FsmCellPortAliasMap = std::unordered_map<અચલ AstCell*, FsmCellPortMap>;

સંરચના StateConstLabel final {
    string text;
    તાર્કિક fromParam = ખોટું;
    size_t stateIndex = 0;
};

સંરચના FsmStateSpace final {
    std::vector<std::pair<string, Fsm_State_કિંમત>> states;  // User label and encoded value
    std::unordered_map<Fsm_State_કિંમત, StateConstLabel, Fsm_State_કિંમત_Hash>
        labels;  // Encoded value to label
    AstVar* stateVarp = nullptr;  // Tracked FSM state variable
    તાર્કિક enumBacked = ખોટું;  // Whether states came from an enum declaration
};

// Local shared state between the two adjacent FSM coverage phases. Detection
// fills this with recovered FSM graphs; lowering consumes the completed graphs
// immediately afterward without needing any AST serialization bridge.
વર્ગ FsmState final {
    // All detected FSMs in discovery order. This is the only bridge between
    // the adjacent detect and lower phases, so the second phase never needs to
    // rediscover or serialize the extracted machine.
    std::vector<DetectedFsm> m_fsms;
    std::map<અચલ AstVarScope*, size_t> m_fsmIndex;

જાહેર:
    DetectedFsm& fsmFor(AstVarScope* stateVscp) {
        અચલ std::map<અચલ AstVarScope*, size_t>::const_iterator it = m_fsmIndex.find(stateVscp);
        જો (it != m_fsmIndex.end()) પાછા m_fsms.at(it->second);
        અચલ size_t index = m_fsms.size();
        m_fsmIndex.emplace(stateVscp, index);
        m_fsms.emplace_back();
        પાછા m_fsms.back();
    }
    અચલ std::vector<DetectedFsm>& fsms() અચલ { પાછા m_fsms; }
};

// Detection runs while the original clocked/case structure is still intact and
// populates graph-backed FSM models without mutating the tree mid-traversal.
// This pass is intentionally conservative: for this PR we only lock down the
// small set of transition/selector forms that are already stable in the
// normalized AST we see here. The remaining reject branches are therefore
// mostly future-feature boundaries, not accidental dead code.
વર્ગ FsmDetectVisitor final : જાહેર VNVisitor {
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
    std::unordered_map<અચલ AstVarScope*, FsmCaseCandidate> m_comboPaired;
    // Continuous aliases are order-independent, while procedural aliases must
    // remain source-order scoped to avoid using assignments not yet executed.
    FsmAliasMap m_stateAliases;
    std::unordered_set<અચલ AstVarScope*> m_ambiguousStateAliases;
    // A surviving wrapper's semantic d/q relationship is split across the
    // parent scope and the child module scope. This table is the narrow bridge
    // between those scopes: only transparent port aliases are recorded, so the
    // detector does not become a general cross-module dataflow engine.
    FsmCellPortAliasMap m_cellPortAliases;
    FsmCellPortAliasMap m_cellPortChildAliases;

    // METHODS
    // Enum-backed FSMs may be wrapped in refs/typedefs; normalize to the
    // underlying enum type before deciding whether a case is a candidate.
    સ્થિર Ast_ગાંઠ_DType* unwrapEnumCandidate(Ast_ગાંઠ_DType* dtypep) {
        પાછા dtypep->skipRefToEnump();
    }

    સ્થિર string candidateConflictContext(Ast_ગાંઠ* laterNodep,
                                           અચલ FsmCaseCandidate& firstCand) {
        પાછા '\n' + laterNodep->warnContextPrimary() + firstCand.warnNodep->warnOther()
               + "... Location of first supported candidate for "
               + firstCand.stateVscp->prettyNameQ() + '\n'
               + firstCand.warnNodep->warnContextSecondary();
    }

    સ્થિર તાર્કિક rejectFsmWrapperCell(AstCell* cellp, અચલ string& reason) {
        cellp->v3warn(COVERIGN, "Ignoring unsupported: " + reason);
        પાછા ખોટું;
    }

    સ્થિર તાર્કિક simple_Param_State_કિંમત(AstCell* cellp, અચલ string& name, Fsm_State_કિંમત& કિંમત,
                                      Ast_ગાંઠ_પદાવલિ*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = nullptr;
        માટે (Ast_ગાંઠ* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* અચલ varp = VN_CAST(stmtp, Var);
            જો (!varp || !varp->isParam() || varp->name() != name) ચાલુ;
            valuepr = VN_AS(varp->valuep(), ગાંઠ_પદાવલિ);
            પાછા const_કિંમત_Status(valuepr, કિંમત) == Const_કિંમત_Status::OK;
        }
        પાછા ખોટું;
    }

    સ્થિર તાર્કિક childPortInScope(AstVarScope* vscp, AstScope* parentScopep, AstCell*& cellpr) {
        જો (!vscp->varp()->isIO()) પાછા ખોટું;
        AstScope* અચલ scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        જો (scopep->aboveScopep() != parentScopep) પાછા ખોટું;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        પાછા સાચું;
    }

    સ્થિર AstVarScope* simpleAssignVarScope(Ast_ગાંઠ_પદાવલિ* exprp) {
        AstVarRef* અચલ vrefp = VN_CAST(exprp, VarRef);
        પાછા vrefp ? vrefp->varScopep() : nullptr;
    }

    શૂન્ય addWrapperCell(AstScope* scopep, AstCell* cellp) {
        m_cellPortAliases.emplace(cellp, FsmCellPortMap{});
        m_cellPortChildAliases.emplace(cellp, FsmCellPortMap{});
        અચલ std::pair<AstScope*, AstCell*> item{scopep, cellp};
        જો (std::find(m_wrapperCells.cbegin(), m_wrapperCells.cend(), item)
            != m_wrapperCells.cend()) {
            પાછા;
        }
        m_wrapperCells.emplace_back(item);
    }

    શૂન્ય collectCellPortAlias(AstAssignW* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* અચલ lhsVscp = simpleAssignVarScope(nodep->lhsp());
        AstVarScope* અચલ rhsVscp = simpleAssignVarScope(nodep->rhsp());
        જો (!lhsVscp || !rhsVscp) પાછા;
        AstCell* cellp = nullptr;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        જો (childPortInScope(lhsVscp, m_scopep, cellp)) {
            જો (!fsmRegisterWrapperDesc(cellp)) પાછા;
            UASSERT_OBJ(lhsVscp->varp()->is_આગત(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cellPortAliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cellPortChildAliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            addWrapperCell(m_scopep, cellp);
        } નહીંતર જો (childPortInScope(rhsVscp, m_scopep, cellp)) {
            જો (!fsmRegisterWrapperDesc(cellp)) પાછા;
            UASSERT_OBJ(rhsVscp->varp()->isWritable(), nodep,
                        "Child-side port alias rhs should be writable");
            m_cellPortAliases[cellp][rhsVscp->varp()->name()] = lhsVscp;
            m_cellPortChildAliases[cellp][rhsVscp->varp()->name()] = rhsVscp;
            addWrapperCell(m_scopep, cellp);
        }
    }

    AstVarScope* roleVarScope(AstCell* cellp, અચલ string& portName) અચલ {
        // At this point explicit AstPin expressions have been lowered away, so
        // role resolution crosses the wrapper boundary only through the
        // transparent alias table above. This keeps wrapper support aligned with
        // direct-register detection instead of growing into interprocedural FSM
        // inference.
        અચલ FsmCellPortMap& ports = m_cellPortAliases.at(cellp);
        અચલ FsmCellPortMap::const_iterator portIt = ports.find(portName);
        પાછા portIt == ports.end() ? nullptr : portIt->second;
    }

    AstVarScope* childRoleVarScope(AstCell* cellp, અચલ string& portName) અચલ {
        અચલ FsmCellPortMap& ports = m_cellPortChildAliases.at(cellp);
        અચલ FsmCellPortMap::const_iterator portIt = ports.find(portName);
        પાછા portIt == ports.end() ? nullptr : portIt->second;
    }

    વર્ગ RegisterAlwaysAnalyzer final {
        AstScope* અચલ m_scopep;  // Scope containing the always block(s) being analyzed

    જાહેર:
        explicit RegisterAlwaysAnalyzer(AstScope* scopep)
            : m_scopep{scopep} {}

        std::vector<std::pair<AstCase*, Ast_ગાંઠ_પદાવલિ*>>
        oneBlockCandidates(AstAlways* alwaysp) અચલ {
            std::vector<std::pair<AstCase*, Ast_ગાંઠ_પદાવલિ*>> candidates;
            Ast_ગાંઠ* અચલ stmtsp = alwaysp->stmtsp();
            જો (AstIf* અચલ firstIfp = VN_CAST(stmtsp, If)) {
                જો (AstCase* અચલ casep = VN_CAST(firstIfp->elsesp(), Case)) {
                    candidates.emplace_back(casep,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            માટે (Ast_ગાંઠ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                જો (AstCase* અચલ casep = VN_CAST(nodep, Case))
                    candidates.emplace_back(casep, nullptr);
            }
            પાછા candidates;
        }

        std::vector<std::pair<AstIf*, Ast_ગાંઠ_પદાવલિ*>>
        oneBlockIfCandidates(AstAlways* alwaysp) અચલ {
            std::vector<std::pair<AstIf*, Ast_ગાંઠ_પદાવલિ*>> candidates;
            Ast_ગાંઠ* અચલ stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            જો (AstIf* અચલ firstIfp = VN_CAST(stmtsp, If)) {
                જો (AstIf* અચલ chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(firstIfp->elsesp()), If)) {
                    candidates.emplace_back(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            માટે (Ast_ગાંઠ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                જો (AstIf* અચલ ifp = VN_CAST(nodep, If)) candidates.emplace_back(ifp, nullptr);
            }
            પાછા candidates;
        }

        તાર્કિક matchRegisterCandidate(AstAlways* alwaysp, FsmRegisterCandidate& cand) અચલ {
            પાછા FsmDetectVisitor::matchRegisterAlways(alwaysp, m_scopep, cand);
        }

        શૂન્ય buildOneBlockCandidate(AstAlways* alwaysp, AstVarScope* vscp, Ast_ગાંઠ_પદાવલિ* resetCondp,
                                    FsmRegisterCandidate& reg) અચલ {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.nextVscp(vscp);
            reg.senses() = FsmDetectVisitor::describe_Sen_વૃક્ષ(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != nullptr);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* અચલ firstIfp = VN_CAST(alwaysp->stmtsp(), If);
            જો (firstIfp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = nullptr;
                અચલ ResetAssignStatus resetStatus = FsmDetectVisitor::collectConstStateAssigns(
                    firstIfp->thensp(), resetStateVscp, reg.resetArcs());
                જો (resetStatus == ResetAssignStatus::EMPTY) {
                    reg.resetArcs().clear();
                } નહીંતર જો (resetStatus == ResetAssignStatus::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().clear();
                    Fsm_State_કિંમત reset_કિંમત;
                    Ast_ગાંઠ* અચલ thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(firstIfp->thensp());
                    UASSERT_OBJ(thenNodep, firstIfp,
                                "one-block reset fallback requires a non-empty reset branch");
                    Ast_ગાંઠ_Assign* અચલ resetAssp = FsmDetectVisitor::direct_Const_State_Assign_ગાંઠ(
                        thenNodep, resetStateVscp, reset_કિંમત);
                    જો (resetAssp && resetStateVscp == vscp) {
                        reg.resetArcs().emplace_back(reset_કિંમત, resetAssp);
                    }
                } નહીંતર જો (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                    reg.resetArcs().clear();
                }
            }
        }
    };

    તાર્કિક matchFsmWrapperCell(AstScope* scopep, AstCell* cellp, FsmRegisterCandidate& cand) અચલ {
        FsmWrapperRoles roles = rolesFromDesc(*fsmRegisterWrapperDesc(cellp));

        AstVarScope* અચલ nextVscp = roleVarScope(cellp, roles.dPort);
        AstVarScope* અચલ stateVscp = roleVarScope(cellp, roles.qPort);
        જો (!nextVscp || !stateVscp) {
            પાછા rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* અચલ clkVscp = roleVarScope(cellp, roles.clkPort);
        જો (!clkVscp) {
            પાછા rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper instance requires a simple clock connection");
        }

        FsmSenDesc clkSense;
        clkSense.edgeType = VEdgeType::ET_POSEDGE;
        clkSense.varScopep = clkVscp;
        cand.senses().ધકેલો_back(clkSense);

        AstVarScope* resetVscp = nullptr;
        જો (!roles.rstPort.empty()) resetVscp = roleVarScope(cellp, roles.rstPort);
        જો (resetVscp) {
            // The descriptor identifies the reset port but not its polarity. Use
            // the wrapper's own event control AST as the contract for sampling
            // the connected parent signal.
            તાર્કિક inferred_Active_નીચલું = ખોટું;
            જો (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rstPort, inferred_Active_નીચલું)) {
                roles.has_Rst_Active_નીચલું = સાચું;
                roles.rst_Active_નીચલું = inferred_Active_નીચલું;
            }
        }

        Ast_ગાંઠ_પદાવલિ* resetValuep = nullptr;
        Fsm_State_કિંમત reset_કિંમત;
        અચલ તાર્કિક has_Reset_કિંમત
            = !roles.rstValParam.empty()
              && simple_Param_State_કિંમત(cellp, roles.rstValParam, reset_કિંમત, resetValuep);
        જો (resetVscp && roles.has_Rst_Active_નીચલું && has_Reset_કિંમત) {
            FsmSenDesc rstSense;
            rstSense.edgeType = roles.rst_Active_નીચલું ? VEdgeType::ET_NEGEDGE : VEdgeType::ET_POSEDGE;
            rstSense.varScopep = resetVscp;
            cand.senses().ધકેલો_back(rstSense);
            cand.resetCond().varScopep = resetVscp;
            cand.resetCond().active_નીચલું = roles.rst_Active_નીચલું;
            cand.hasResetCond(સાચું);
            cand.resetArcs().emplace_back(reset_કિંમત, cellp, resetValuep);
        } નહીંતર જો (!roles.rstPort.empty() || !roles.rstValParam.empty()) {
            string reason;
            જો (roles.rstPort.empty()) {
                reason = "reset port is not configured";
            } નહીંતર જો (!resetVscp) {
                reason = "reset connection is missing or not a simple variable";
            } નહીંતર જો (!roles.has_Rst_Active_નીચલું) {
                reason = "reset polarity could not be inferred from the wrapper";
            } નહીંતર જો (roles.rstValParam.empty()) {
                reason = "reset_value parameter is not configured";
            } નહીંતર {
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
        પાછા સાચું;
    }

    વર્ગ ComboAlwaysAnalyzer final {
    જાહેર:
        સંરચના ComboMatch final {
            અચલ FsmRegisterCandidate* matchedp = nullptr;
            Ast_ગાંઠ* warnNodep = nullptr;
        };

    ખાનગી:
        અચલ std::vector<FsmRegisterCandidate>& m_registerCandidates;

    જાહેર:
        explicit ComboAlwaysAnalyzer(અચલ std::vector<FsmRegisterCandidate>& registerCandidates)
            : m_registerCandidates{registerCandidates} {}

        ComboMatch matchCase(Ast_ગાંઠ* stmtsp, AstCase* casep) અચલ {
            ComboMatch match;
            AstVarRef* અચલ selp = VN_CAST(casep->exprp(), VarRef);
            જો (!selp) પાછા match;
            માટે (અચલ FsmRegisterCandidate& reg : m_registerCandidates) {
                જો (selp->varScopep() == reg.nextVscp()) {
                    જો (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        ચાલુ;
                    }
                } નહીંતર જો (selp->varScopep() != reg.stateVscp()) {
                    ચાલુ;
                }
                Ast_ગાંઠ* અચલ warnNodep = FsmDetectVisitor::case_Supported_Transition_ગાંઠ(
                    casep, reg.nextVscp(), reg.inclCond());
                જો (!warnNodep) ચાલુ;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            પાછા match;
        }

        ComboMatch matchIfChain(Ast_ગાંઠ* stmtsp, અચલ FsmIfChainCandidate& chain) અચલ {
            ComboMatch match;
            માટે (અચલ FsmRegisterCandidate& reg : m_registerCandidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                જો (chain.compareVscp == reg.nextVscp()) {
                    જો (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.nextVscp())) {
                        ચાલુ;
                    }
                } નહીંતર જો (chain.compareVscp != reg.stateVscp()) {
                    ચાલુ;
                }
                Ast_ગાંઠ* અચલ warnNodep
                    = FsmDetectVisitor::if_Chain_Supported_Transition_ગાંઠ(chain, reg.nextVscp());
                જો (!warnNodep) ચાલુ;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            પાછા match;
        }

        તાર્કિક shouldWarnUnsupported(Ast_ગાંઠ* stmtsp, AstCase* casep) અચલ {
            અચલ AstVarRef* અચલ selp = VN_CAST(casep->exprp(), VarRef);
            જો (!selp) પાછા ખોટું;

            પાછા std::any_of(
                m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                [&](અચલ FsmRegisterCandidate& reg) -> તાર્કિક {
                    અચલ તાર્કિક matchesNext = selp->varScopep() == reg.nextVscp();
                    અચલ તાર્કિક matchesState = selp->varScopep() == reg.stateVscp();

                    જો (!matchesNext && !matchesState) પાછા ખોટું;
                    જો (matchesNext
                        && !FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        પાછા ખોટું;
                    }
                    પાછા FsmDetectVisitor::case_Supported_Transition_ગાંઠ(casep, reg.nextVscp(),
                                                                         reg.inclCond());
                });
        }
    };

    // Reset arcs are only modeled for the simple signal form that survives to
    // this pass after earlier normalization.
    સ્થિર તાર્કિક isSimpleResetCond(Ast_ગાંઠ_પદાવલિ* condp) { પાછા VN_IS(condp, VarRef); }

    // Normalize the reset condition into a compact description so the lowering
    // phase can regenerate the same predicate after detection. By the time
    // this pass runs, active-low source forms such as "!rst_n" have already
    // been canonicalized to a positive-condition if/else shape, so only a
    // plain VarRef survives here.
    સ્થિર FsmResetCondDesc describeResetCond(Ast_ગાંઠ_પદાવલિ* condp) {
        FsmResetCondDesc desc;
        જો (AstVarRef* અચલ vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        પાછા desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    સ્થિર std::vector<FsmSenDesc> describe_Sen_વૃક્ષ(Ast_Sen_વૃક્ષ* sentreep) {
        std::vector<FsmSenDesc> senses;
        માટે (AstSenItem* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), SenItem)) {
            Ast_ગાંઠ_Var_Ref* અચલ vrefp = itemp->varrefp();
            જો (!vrefp) ચાલુ;
            FsmSenDesc desc;
            desc.edgeType = itemp->edgeType().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.ધકેલો_back(desc);
        }
        પાછા senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    સ્થિર તાર્કિક isIgnorableStmt(Ast_ગાંઠ* nodep) { પાછા VN_IS(nodep, CoverInc); }

    સ્થિર Ast_ગાંઠ* skipLeadingIgnorableStmt(Ast_ગાંઠ* nodep) {
        જ્યારે (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        પાછા nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    સ્થિર Ast_ગાંઠ* singleMeaningfulStmt(Ast_ગાંઠ* stmtp) {
        Ast_ગાંઠ* resultp = nullptr;
        માટે (Ast_ગાંઠ* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            જો (isIgnorableStmt(nodep)) ચાલુ;
            જો (resultp) પાછા nullptr;
            resultp = nodep;
        }
        પાછા resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    સ્થિર Ast_ગાંઠ* singleMeaningfulBranch(Ast_ગાંઠ* branchp) {
        જો (!branchp) પાછા nullptr;
        પાછા branchp;
    }

    // By fsm-detect time, non-clocked always @* blocks are already admitted through
    // a missing sentree. This helper therefore only needs to recognize
    // explicit changed-sensitivity lists such as always @(a or b); clocked and
    // event-driven forms remain out of scope.
    સ્થિર તાર્કિક isPlainComboSentree(અચલ Ast_Sen_વૃક્ષ* sentreep) {
        UASSERT(sentreep, "plain combo sensitivity check requires a sensitivity tree");
        માટે (અચલ AstSenItem* senp = sentreep->sensesp(); senp;
             senp = VN_AS(senp->nextp(), SenItem)) {
            જો (senp->edgeType() == VEdgeType::ET_CHANGED) ચાલુ;
            પાછા ખોટું;
        }
        પાછા સાચું;
    }

    શૂન્ય warnUnsupportedComboAlways(અચલ FsmComboAlways& combo) {
        અચલ ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_ગાંઠ* અચલ stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        તાર્કિક warned = ખોટું;
        માટે (Ast_ગાંઠ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* અચલ casep = VN_CAST(nodep, Case);
            જો (!casep) ચાલુ;
            જો (analyzer.shouldWarnUnsupported(stmtsp, casep)) {
                casep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-clocked always "
                                        "blocks requires a combinational sensitivity list or "
                                        "always_comb");
                warned = સાચું;
            }
            જો (warned) તોડો;
        }
    }

    // Case-item bodies are single subtrees like if/else arms, not statement
    // lists, so unwrap only local begin/end wrappers here rather than walking
    // sibling case items via nextp().
    સ્થિર Ast_ગાંઠ_Assign* directStateAssign(Ast_ગાંઠ* stmtp, AstVarScope* stateVscp) {
        Ast_ગાંઠ* અચલ nodep = singleMeaningfulBranch(stmtp);
        જો (!nodep) પાછા nullptr;
        Ast_ગાંઠ_Assign* અચલ assp = VN_CAST(nodep, ગાંઠ_Assign);
        જો (!assp) પાછા nullptr;
        AstVarRef* અચલ vrefp = VN_CAST(assp->lhsp(), VarRef);
        જો (!vrefp || vrefp->varScopep() != stateVscp) પાછા nullptr;
        પાછા assp;
    }

    સ્થિર Ast_ગાંઠ_Assign* ગાંઠ_State_Var_Assign(Ast_ગાંઠ* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        Ast_ગાંઠ_Assign* અચલ assp = VN_CAST(nodep, ગાંઠ_Assign);
        જો (!assp) પાછા nullptr;
        AstVarRef* અચલ lhsp = VN_CAST(assp->lhsp()->baseFromp(સાચું), VarRef);
        AstVarRef* અચલ rhsp = VN_CAST(assp->rhsp(), VarRef);
        જો (!rhsp || !lhsp) પાછા nullptr;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        પાછા assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    સ્થિર Ast_ગાંઠ_Assign* directCondStateVarAssign(Ast_ગાંઠ* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, Ast_ગાંઠ_પદાવલિ*& condp,
                                                   Fsm_State_કિંમત& reset_કિંમત) {
        Ast_ગાંઠ_Assign* અચલ assp = VN_CAST(nodep, ગાંઠ_Assign);
        જો (!assp) પાછા nullptr;
        AstVarRef* અચલ lhsp = VN_CAST(assp->lhsp()->baseFromp(સાચું), VarRef);
        AstCond* અચલ rhsp = VN_CAST(assp->rhsp(), Cond);
        જો (!rhsp || !lhsp) પાછા nullptr;
        AstVarRef* અચલ elsep = VN_CAST(rhsp->elsep(), VarRef);
        જો (!elsep) પાછા nullptr;
        જો (const_કિંમત_Status(rhsp->thenp(), reset_કિંમત) != Const_કિંમત_Status::OK) પાછા nullptr;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        પાછા assp;
    }

    સ્થિર Ast_ગાંઠ_Assign* direct_Const_State_Assign_ગાંઠ(Ast_ગાંઠ* nodep, AstVarScope*& stateVscp,
                                                     Fsm_State_કિંમત& કિંમત) {
        Ast_ગાંઠ_Assign* અચલ assp = VN_CAST(nodep, ગાંઠ_Assign);
        જો (!assp) પાછા nullptr;
        AstVarRef* અચલ lhsp = VN_CAST(assp->lhsp()->baseFromp(સાચું), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        જો (const_કિંમત_Status(assp->rhsp(), કિંમત) != Const_કિંમત_Status::OK) પાછા nullptr;
        stateVscp = lhsp->varScopep();
        પાછા assp;
    }

    ગણના વર્ગ ResetAssignStatus : uint8_t {
        EMPTY,  // Reset branch had no non-coverage statements.
        NONE,  // Reset branch was not the supported direct-constant shape.
        SINGLE,  // Exactly one supported reset assignment was collected.
        MULTI_SAME_STATE  // Multiple assignments to the same FSM state var; warn and ignore.
    };

    // Reset arcs are only extracted from the single direct-constant form. If
    // user RTL assigns the same state register multiple times in the reset
    // branch, warn and skip reset-arc modeling rather than inventing multiple
    // reset transitions for an odd but legal coding style.
    સ્થિર ResetAssignStatus collectConstStateAssigns(Ast_ગાંઠ* stmtp, AstVarScope*& stateVscp,
                                                      std::vector<FsmResetArcDesc>& resetArcs) {
        Ast_ગાંઠ* nodep = skipLeadingIgnorableStmt(stmtp);
        જો (!nodep) પાછા ResetAssignStatus::EMPTY;
        માટે (;; nodep = nodep->nextp()) {
            AstVarScope* assignStateVscp = nullptr;
            Fsm_State_કિંમત કિંમત;
            Ast_ગાંઠ_Assign* અચલ assp = direct_Const_State_Assign_ગાંઠ(nodep, assignStateVscp, કિંમત);
            જો (!assp) પાછા ResetAssignStatus::NONE;
            જો (!stateVscp) stateVscp = assignStateVscp;
            જો (assignStateVscp != stateVscp) પાછા ResetAssignStatus::NONE;
            જો (!resetArcs.empty()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.clear();
                પાછા ResetAssignStatus::MULTI_SAME_STATE;
            }
            resetArcs.emplace_back(કિંમત, assp);
            જો (!nodep->nextp()) પાછા ResetAssignStatus::SINGLE;
        }
    }

    સ્થિર તાર્કિક hasCanonicalNextStateDefaultBeforeCase(Ast_ગાંઠ* stmtsp, Ast_ગાંઠ* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* nextVscp) {
        Ast_ગાંઠ* અચલ bodyp = skipLeadingIgnorableStmt(stmtsp);
        તાર્કિક sawCanonicalDefault = ખોટું;
        માટે (Ast_ગાંઠ* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            જો (nodep == targetp) પાછા sawCanonicalDefault;
            જો (Ast_ગાંઠ_Assign* અચલ assp = VN_CAST(nodep, ગાંઠ_Assign)) {
                AstVarRef* અચલ lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* અચલ rhsp = VN_CAST(assp->rhsp(), VarRef);
                જો (!lhsp || lhsp->varScopep() != nextVscp) ચાલુ;
                જો (sawCanonicalDefault) {
                    અચલ string nextName = nextVscp->varp()->prettyNameQ();
                    અચલ string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + nextName
                                     + ") when the canonical " + nextName + " = " + stateName
                                     + " default is overwritten before the case statement");
                    પાછા ખોટું;
                }
                જો (!rhsp || rhsp->varScopep() != stateVscp) પાછા ખોટું;
                sawCanonicalDefault = સાચું;
            }
        }
    }

    સ્થિર તાર્કિક ifStateConstAssign(Ast_ગાંઠ* stmtp, AstVarScope* stateVscp,
                                   Fsm_State_કિંમત& then_કિંમત, Fsm_State_કિંમત& else_કિંમત) {
        AstIf* અચલ ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        જો (!ifp || !ifp->elsesp()) પાછા ખોટું;
        AstVarScope* thenVscp = nullptr;
        AstVarScope* elseVscp = nullptr;
        Ast_ગાંઠ* અચલ thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        Ast_ગાંઠ* અચલ elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        જો (!thenNodep || !elseNodep) પાછા ખોટું;
        જો (!direct_Const_State_Assign_ગાંઠ(thenNodep, thenVscp, then_કિંમત)) પાછા ખોટું;
        જો (!direct_Const_State_Assign_ગાંઠ(elseNodep, elseVscp, else_કિંમત)) પાછા ખોટું;
        જો (thenVscp == stateVscp && elseVscp == stateVscp) પાછા સાચું;
        જો (thenVscp != elseVscp) પાછા ખોટું;
        Ast_ગાંઠ* અચલ followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = nullptr;
        AstVarScope* finalFromVscp = nullptr;
        Ast_ગાંઠ* અચલ finalNodep = singleMeaningfulBranch(followp);
        જો (!finalNodep) પાછા ખોટું;
        જો (!ગાંઠ_State_Var_Assign(finalNodep, finalStateVscp, finalFromVscp)) પાછા ખોટું;
        જો (finalStateVscp != stateVscp) પાછા ખોટું;
        જો (finalFromVscp != thenVscp) પાછા ખોટું;
        પાછા સાચું;
    }

    સ્થિર તાર્કિક directStateCondConstAssign(Ast_ગાંઠ* stmtp, AstVarScope* stateVscp,
                                           Fsm_State_કિંમત& then_કિંમત, Fsm_State_કિંમત& else_કિંમત) {
        Ast_ગાંઠ_Assign* અચલ assp = directStateAssign(stmtp, stateVscp);
        જો (!assp) પાછા ખોટું;
        AstCond* અચલ condp = VN_CAST(assp->rhsp(), Cond);
        જો (!condp) પાછા ખોટું;
        પાછા const_કિંમત_Status(condp->thenp(), then_કિંમત) == Const_કિંમત_Status::OK
               && const_કિંમત_Status(condp->elsep(), else_કિંમત) == Const_કિંમત_Status::OK;
    }

    સ્થિર Ast_ગાંઠ* case_Item_Supported_Arc_ગાંઠ(AstCaseItem* itemp, AstVarScope* stateVscp,
                                             તાર્કિક inclCond) {
        જો (itemp->isDefault()) {
            જો (!inclCond) પાછા nullptr;
        }
        Ast_ગાંઠ_Assign* અચલ assp = directStateAssign(itemp->stmtsp(), stateVscp);
        જો (assp) {
            Fsm_State_કિંમત to_કિંમત;
            જો (const_કિંમત_Status(assp->rhsp(), to_કિંમત) == Const_કિંમત_Status::OK) પાછા assp;
        }
        Fsm_State_કિંમત then_કિંમત;
        Fsm_State_કિંમત else_કિંમત;
        જો (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_કિંમત, else_કિંમત)) {
            પાછા assp;
        }
        જો (ifStateConstAssign(itemp->stmtsp(), stateVscp, then_કિંમત, else_કિંમત)) {
            પાછા singleMeaningfulBranch(itemp->stmtsp());
        }
        પાછા nullptr;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    સ્થિર Ast_ગાંઠ* case_Supported_Transition_ગાંઠ(AstCase* casep, AstVarScope* stateVscp,
                                                તાર્કિક inclCond) {
        માટે (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            જો (Ast_ગાંઠ* અચલ nodep = case_Item_Supported_Arc_ગાંઠ(itemp, stateVscp, inclCond))
                પાછા nodep;
        }
        પાછા nullptr;
    }

    સ્થિર Ast_ગાંઠ* case_Item_Supported_Arc_ગાંઠ_Like(Ast_ગાંઠ* stmtsp, AstVarScope* stateVscp) {
        જો (Ast_ગાંઠ_Assign* અચલ assp = directStateAssign(stmtsp, stateVscp)) {
            Fsm_State_કિંમત to_કિંમત;
            જો (const_કિંમત_Status(assp->rhsp(), to_કિંમત) == Const_કિંમત_Status::OK) પાછા assp;
            Fsm_State_કિંમત then_કિંમત;
            Fsm_State_કિંમત else_કિંમત;
            જો (directStateCondConstAssign(stmtsp, stateVscp, then_કિંમત, else_કિંમત)) પાછા assp;
        }
        પાછા nullptr;
    }

    સ્થિર Ast_ગાંઠ* if_Chain_Supported_Transition_ગાંઠ(અચલ FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        માટે (size_t i = 0; i < chain.branches.size(); ++i) {
            અચલ FsmIfBranch& branch = chain.branches[i];
            Ast_ગાંઠ* અચલ nodep = case_Item_Supported_Arc_ગાંઠ_Like(branch.stmtsp, stateVscp);
            જો (!nodep) પાછા nullptr;
        }
        પાછા chain.branches.front().ifp;
    }

    // Prefer user labels in reports. Forced non-enum FSMs prepopulate synthetic
    // labels, so all emitted arcs should already have a known label here.
    સ્થિર string label_For_કિંમત(
        અચલ std::unordered_map<Fsm_State_કિંમત, StateConstLabel, Fsm_State_કિંમત_Hash>& labels,
        અચલ Fsm_State_કિંમત& કિંમત) {
        પાછા labels.at(કિંમત).text;
    }

    // The extractor only models constant-valued state transitions, and by the
    // time detect runs those values have already been constant-folded.
    ગણના વર્ગ Const_કિંમત_Status : uint8_t { OK, NOT_CONST, XZ };

    સ્થિર Const_કિંમત_Status const_કિંમત_Status(Ast_ગાંઠ_પદાવલિ* exprp, Fsm_State_કિંમત& કિંમત) {
        અચલ AstConst* અચલ constp = VN_CAST(exprp, Const);
        જો (!constp) પાછા Const_કિંમત_Status::NOT_CONST;
        અચલ V3_સંખ્યા& અંક = constp->અંક();
        જો (અંક.isAnyXZ()) પાછા Const_કિંમત_Status::XZ;
        કિંમત = Fsm_State_કિંમત{અંક};
        પાછા Const_કિંમત_Status::OK;
    }

    સ્થિર તાર્કિક pureStateComparisonNoAlias(Ast_ગાંઠ_પદાવલિ* exprp, FsmStateComparison& cmp) {
        AstEq* અચલ eqp = VN_CAST(exprp, Eq);
        જો (!eqp) પાછા ખોટું;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        Ast_ગાંઠ_પદાવલિ* valuep = eqp->rhsp();
        જો (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->baseFromp(સાચું), VarRef);
            જો (!vrefp) { પાછા ખોટું; }
            valuep = eqp->lhsp();
        }

        Fsm_State_કિંમત કિંમત;
        જો (const_કિંમત_Status(valuep, કિંમત) != Const_કિંમત_Status::OK) પાછા ખોટું;
        cmp.stateVscp = vrefp->varScopep();
        cmp.valuep = valuep;
        cmp.કિંમત = કિંમત;
        પાછા સાચું;
    }

    સ્થિર તાર્કિક pureStateComparison(Ast_ગાંઠ_પદાવલિ* exprp, અચલ FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        જો (pureStateComparisonNoAlias(exprp, cmp)) પાછા સાચું;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        જો (AstVarRef* અચલ vrefp = VN_CAST(exprp, VarRef)) {
            અચલ FsmAliasMap::const_iterator it = aliases.find(vrefp->varScopep());
            જો (it == aliases.end()) પાછા ખોટું;
            cmp = it->second;
            પાછા સાચું;
        }
        પાછા ખોટું;
    }

    સ્થિર તાર્કિક supported_ટોચ_Level_Guard(Ast_ગાંઠ_પદાવલિ* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        જો (VN_IS(exprp, Or)) પાછા ખોટું;
        જો (VN_IS(exprp, RedAnd)) પાછા ખોટું;
        જો (VN_IS(exprp, RedOr)) પાછા ખોટું;
        જો (VN_IS(exprp, RedXor)) પાછા ખોટું;
        પાછા સાચું;
    }

    સ્થિર તાર્કિક resolveIfPredicate(Ast_ગાંઠ_પદાવલિ* exprp, અચલ FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, તાર્કિક& hasGuard) {
        std::vector<Ast_ગાંઠ_પદાવલિ*> terms;
        std::vector<Ast_ગાંઠ_પદાવલિ*> pending;
        pending.ધકેલો_back(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        જ્યારે (!pending.empty()) {
            Ast_ગાંઠ_પદાવલિ* અચલ nodep = pending.back();
            pending.કાઢો_back();
            જો (AstAnd* અચલ andp = VN_CAST(nodep, And)) {
                pending.ધકેલો_back(andp->rhsp());
                pending.ધકેલો_back(andp->lhsp());
            } નહીંતર {
                terms.ધકેલો_back(nodep);
            }
        }

        તાર્કિક sawComparison = ખોટું;
        માટે (size_t i = 0; i < terms.size(); ++i) {
            Ast_ગાંઠ_પદાવલિ* અચલ termp = terms[i];
            FsmStateComparison termCmp;
            જો (pureStateComparison(termp, aliases, termCmp /*ref*/)) {
                જો (sawComparison) પાછા ખોટું;
                cmp = termCmp;
                sawComparison = સાચું;
                ચાલુ;
            }
            જો (!supported_ટોચ_Level_Guard(termp)) પાછા ખોટું;
            hasGuard = સાચું;
        }
        પાછા sawComparison;
    }

    સ્થિર શૂન્ય addAlias(FsmAliasMap& aliases, std::unordered_set<અચલ AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, અચલ FsmStateComparison& cmp) {
        જો (ambiguous.find(aliasVscp) != ambiguous.end()) પાછા;
        અચલ FsmAliasMap::iterator it = aliases.find(aliasVscp);
        જો (it == aliases.end()) {
            aliases.emplace(aliasVscp, cmp);
            પાછા;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        જો (it->second.stateVscp == cmp.stateVscp && it->second.કિંમત == cmp.કિંમત) પાછા;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        પાછા;
    }

    સ્થિર શૂન્ય collectAliasFromAssign(Ast_ગાંઠ_Assign* assp, FsmAliasMap& aliases,
                                       std::unordered_set<અચલ AstVarScope*>& ambiguous) {
        AstVarRef* અચલ lhsp = VN_CAST(assp->lhsp(), VarRef);
        જો (!lhsp) પાછા;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        જો (!pureStateComparisonNoAlias(assp->rhsp(), cmp /*ref*/)) પાછા;
        addAlias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(Ast_ગાંઠ* stmtsp, Ast_ગાંઠ* limitp) અચલ {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_set<અચલ AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        માટે (Ast_ગાંઠ* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            જો (Ast_ગાંઠ_Assign* અચલ assp = VN_CAST(nodep, ગાંઠ_Assign)) {
                collectAliasFromAssign(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        માટે (અચલ AstVarScope* અચલ vscp : ambiguous) aliases.erase(vscp);
        પાછા aliases;
    }

    સ્થિર તાર્કિક collectIfChain(AstIf* ifp, અચલ FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_set<string> seen_કિંમતો;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        જ્યારે (સાચું) {
            FsmStateComparison cmp;
            તાર્કિક hasGuard = ખોટું;
            જો (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) પાછા ખોટું;
            જો (chain.compareVscp && chain.compareVscp != cmp.stateVscp) પાછા ખોટું;
            જો (!seen_કિંમતો.ઉમેરો(cmp.કિંમત.ચાવી()).second) પાછા ખોટું;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.ધકેલો_back(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.કિંમત, hasGuard});

            Ast_ગાંઠ* અચલ elseNodep
                = singleMeaningfulBranch(skipLeadingIgnorableStmt(curp->elsesp()));
            જો (!elseNodep) તોડો;
            જો (AstIf* અચલ elseIfp = VN_CAST(elseNodep, If)) {
                curp = elseIfp;
                ચાલુ;
            }
            chain.defaultStmtsp = elseNodep;
            તોડો;
        }
        પાછા chain.branches.size() >= 2;
    }

    // Enum-backed FSMs should only use values that were interned as known states.
    // If a constant transition references some other encoding, warn and skip FSM
    // instrumentation for that edge rather than silently dropping it or turning
    // optional coverage into a hard compile failure.
    સ્થિર તાર્કિક validate_Known_State_કિંમત(Ast_ગાંઠ* nodep, અચલ FsmStateSpace& stateSpace,
                                        અચલ Fsm_State_કિંમત& કિંમત, અચલ string& role) {
        જો (stateSpace.labels.find(કિંમત) != stateSpace.labels.end()) પાછા સાચું;
        જો (stateSpace.enumBacked) {
            અચલ string enumRole = role == "source" ? "case item value" : "assigned value";
            nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on enum state variable "
                                        + stateSpace.stateVarp->prettyNameQ() + ": " + enumRole
                                        + " " + કિંમત.warnText()
                                        + " is not present in the declared enum");
            પાછા ખોટું;
        }
        nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum state variable "
                                    + stateSpace.stateVarp->prettyNameQ() + ": " + role + " value "
                                    + કિંમત.warnText()
                                    + " is not present in the inferred state space");
        પાછા ખોટું;
    }

    સ્થિર StateConstLabel stateLabelForConst(AstConst* constp) {
        અચલ string name = constp->origParamName();
        જો (!name.empty()) પાછા StateConstLabel{Ast_ગાંઠ::prettyName(name), સાચું, 0};
        પાછા StateConstLabel{constp->name(), ખોટું, 0};
    }

    સ્થિર શૂન્ય updateStateLabel(FsmStateSpace& stateSpace, અચલ Fsm_State_કિંમત& કિંમત,
                                 અચલ StateConstLabel& label) {
        stateSpace.states.at(stateSpace.labels.at(કિંમત).stateIndex).first = label.text;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    સ્થિર તાર્કિક matchRegisterAlways(AstAlways* alwaysp, AstScope* scopep,
                                    FsmRegisterCandidate& cand) {
        જો (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) પાછા ખોટું;

        Ast_ગાંઠ* અચલ stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        Ast_ગાંઠ* અચલ nodep = singleMeaningfulStmt(stmtsp);
        જો (!nodep) પાછા ખોટું;

        AstVarScope* stateVscp = nullptr;
        AstVarScope* nextVscp = nullptr;
        જો (AstIf* અચલ ifp = VN_CAST(nodep, If)) {
            જો (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) પાછા ખોટું;
            AstVarScope* resetStateVscp = nullptr;
            અચલ ResetAssignStatus resetStatus
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            અચલ તાર્કિક emptyResetBranch = resetStatus == ResetAssignStatus::EMPTY;
            જો (emptyResetBranch) {
                cand.resetArcs().clear();
            } નહીંતર જો (resetStatus == ResetAssignStatus::NONE) {
                cand.resetArcs().clear();
                Fsm_State_કિંમત reset_કિંમત;
                Ast_ગાંઠ* અચલ thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                Ast_ગાંઠ_Assign* અચલ resetAssp
                    = direct_Const_State_Assign_ગાંઠ(thenNodep, resetStateVscp, reset_કિંમત);
                જો (!resetAssp) પાછા ખોટું;
                cand.resetArcs().emplace_back(reset_કિંમત, resetAssp);
            } નહીંતર જો (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                cand.resetArcs().clear();
            }
            Ast_ગાંઠ* અચલ elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            જો (!ગાંઠ_State_Var_Assign(elseNodep, stateVscp, nextVscp)) પાછા ખોટું;
            જો (!emptyResetBranch && resetStateVscp != stateVscp) પાછા ખોટું;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != nullptr);
        } નહીંતર {
            Ast_ગાંઠ_પદાવલિ* resetCondp = nullptr;
            Fsm_State_કિંમત reset_કિંમત;
            // Note the active low 'reset ? next : CONST' form is not accepted
            જો (Ast_ગાંઠ_Assign* અચલ assp
                = directCondStateVarAssign(nodep, stateVscp, nextVscp, resetCondp, reset_કિંમત)) {
                cand.resetArcs().emplace_back(reset_કિંમત, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != nullptr);
            } નહીંતર જો (!ગાંઠ_State_Var_Assign(nodep, stateVscp, nextVscp)) {
                પાછા ખોટું;
            }
        }
        cand.scopep(scopep);
        cand.alwaysp(alwaysp);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(stateVscp);
        cand.nextVscp(nextVscp);
        cand.senses() = describe_Sen_વૃક્ષ(alwaysp->sentreep());
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(alwaysp->fileline());
        પાછા સાચું;
    }

    સ્થિર તાર્કિક add_કિંમત_To_State_Space(Ast_ગાંઠ* nodep, FsmStateSpace& stateSpace,
                                     અચલ Fsm_State_કિંમત& કિંમત, StateConstLabel label) {
        અચલ auto labelIt = stateSpace.labels.find(કિંમત);
        જો (labelIt != stateSpace.labels.end()) {
            StateConstLabel& existingLabel = labelIt->second;
            જો (existingLabel.text != label.text && existingLabel.fromParam && label.fromParam) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                        "state variable "
                                            + stateSpace.stateVarp->prettyNameQ()
                                            + " with multiple labels for the same value "
                                            + કિંમત.warnText() + ": " + existingLabel.text
                                            + " and " + label.text);
                પાછા ખોટું;
            }
            જો (!existingLabel.fromParam && label.fromParam) {
                existingLabel.text = label.text;
                existingLabel.fromParam = label.fromParam;
                updateStateLabel(stateSpace, કિંમત, label);
            }
        } નહીંતર {
            StateConstLabel storedLabel = label;
            storedLabel.stateIndex = stateSpace.states.size();
            stateSpace.states.emplace_back(label.text, કિંમત);
            stateSpace.labels.emplace(કિંમત, storedLabel);
        }
        પાછા સાચું;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    સ્થિર તાર્કિક add_પદાવલિ_To_State_Space(Ast_ગાંઠ_પદાવલિ* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_કિંમત કિંમત;
        અચલ Const_કિંમત_Status status = const_કિંમત_Status(valuep, કિંમત);
        જો (status != Const_કિંમત_Status::OK) {
            જો (status == Const_કિંમત_Status::XZ) {
                valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                         "state variable "
                                             + stateSpace.stateVarp->prettyNameQ()
                                             + " with X/Z state encoding values");
            }
            પાછા ખોટું;
        }
        AstConst* અચલ constp = VN_AS(valuep, Const);
        પાછા add_કિંમત_To_State_Space(valuep, stateSpace, કિંમત, stateLabelForConst(constp));
    }

    સ્થિર તાર્કિક add_Optional_Target_પદાવલિ_To_State_Space(Ast_ગાંઠ_પદાવલિ* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_કિંમત કિંમત;
        અચલ Const_કિંમત_Status status = const_કિંમત_Status(valuep, કિંમત);
        જો (status != Const_કિંમત_Status::OK) {
            valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                     "state variable "
                                         + stateSpace.stateVarp->prettyNameQ()
                                         + " with non-constant target state values");
            પાછા ખોટું;
        }
        AstConst* અચલ constp = VN_AS(valuep, Const);
        પાછા add_કિંમત_To_State_Space(valuep, stateSpace, કિંમત, stateLabelForConst(constp));
    }

    સ્થિર શૂન્ય addResetTargetsToStateSpace(અચલ std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        માટે (અચલ FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.to_કિંમત().ascii(), ખોટું, 0};
            જો (AstConst* અચલ constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                add_કિંમત_To_State_Space(resetArc.nodep(), stateSpace, resetArc.to_કિંમત(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    સ્થિર તાર્કિક collectDeclaredStateSpace(Ast_ગાંઠ* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, તાર્કિક& needs_Source_કિંમતો) {
        AstVar* અચલ stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        જો (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        અચલ તાર્કિક forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        જો (enump) {
            stateSpace.enumBacked = સાચું;
            માટે (AstEnumItem* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                અચલ AstConst* અચલ constp = VN_AS(itemp->valuep(), Const);
                અચલ Fsm_State_કિંમત કિંમત{constp->અંક()};
                અચલ size_t stateIndex = stateSpace.states.size();
                stateSpace.states.emplace_back(itemp->name(), કિંમત);
                stateSpace.labels.emplace(કિંમત,
                                          StateConstLabel{itemp->name(), ખોટું, stateIndex});
            }
            પાછા stateSpace.states.size() >= 2;
        }

        જો (forced) {
            needs_Source_કિંમતો = સાચું;
            પાછા સાચું;
        }

        needs_Source_કિંમતો = સાચું;
        પાછા સાચું;
    }

    template <typename T_ValuepVisitor>
    સ્થિર તાર્કિક collect_State_Space_From_કિંમતો(Ast_ગાંઠ* warnNodep, AstVarScope* stateVscp,
                                            અચલ std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            અચલ T_ValuepVisitor& visitValueps) {
        તાર્કિક needs_Source_કિંમતો = ખોટું;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        જો (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needs_Source_કિંમતો)) {
            પાછા ખોટું;
        }
        જો (!needs_Source_કિંમતો) પાછા સાચું;
        addResetTargetsToStateSpace(resetArcs, stateSpace);
        જો (!visitValueps(
                [&](Ast_ગાંઠ_પદાવલિ* valuep) { પાછા add_પદાવલિ_To_State_Space(valuep, stateSpace); })) {
            પાછા ખોટું;
        }
        પાછા stateSpace.states.size() >= 2;
    }

    સ્થિર તાર્કિક collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* assignVscp,
                                  અચલ std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        પાછા collect_State_Space_From_કિંમતો(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, assignVscp, &stateSpace](અચલ auto& visitValuep) {
                માટે (AstCaseItem* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), CaseItem)) {
                    જો (!itemp->isDefault()) {
                        માટે (Ast_ગાંઠ_પદાવલિ* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), ગાંઠ_પદાવલિ)) {
                            જો (!visitValuep(condp)) પાછા ખોટું;
                        }
                    }
                    જો (Ast_ગાંઠ_Assign* અચલ assp
                        = directStateAssign(itemp->stmtsp(), assignVscp)) {
                        Fsm_State_કિંમત then_કિંમત;
                        Fsm_State_કિંમત else_કિંમત;
                        AstCond* અચલ condp = VN_CAST(assp->rhsp(), Cond);
                        જો (condp
                            && directStateCondConstAssign(itemp->stmtsp(), assignVscp, then_કિંમત,
                                                          else_કિંમત)) {
                            જો (!visitValuep(condp->thenp())) પાછા ખોટું;
                            જો (!visitValuep(condp->elsep())) પાછા ખોટું;
                        } નહીંતર જો (!add_Optional_Target_પદાવલિ_To_State_Space(assp->rhsp(), stateSpace)) {
                            પાછા ખોટું;
                        }
                    }
                }
                પાછા સાચું;
            });
    }

    સ્થિર તાર્કિક collectStateSpace(અચલ FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* assignVscp,
                                  અચલ std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        પાછા collect_State_Space_From_કિંમતો(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, assignVscp, &stateSpace](અચલ auto& visitValuep) {
                માટે (અચલ FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(visitValuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    Ast_ગાંઠ_Assign* અચલ assp = directStateAssign(branch.stmtsp, assignVscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    Fsm_State_કિંમત then_કિંમત;
                    Fsm_State_કિંમત else_કિંમત;
                    AstCond* અચલ condp = VN_CAST(assp->rhsp(), Cond);
                    જો (condp) {
                        UASSERT_OBJ(directStateCondConstAssign(branch.stmtsp, assignVscp,
                                                               then_કિંમત, else_કિંમત),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        જો (!visitValuep(condp->thenp())) પાછા ખોટું;
                        જો (!visitValuep(condp->elsep())) પાછા ખોટું;
                    } નહીંતર જો (!add_Optional_Target_પદાવલિ_To_State_Space(assp->rhsp(), stateSpace)) {
                        પાછા ખોટું;
                    }
                }
                પાછા સાચું;
            });
    }

    // Extract supported case-item transitions in one place so the conservative
    // policy for direct and ternary forms stays consistent. The false exits in
    // this helper are deliberate subset boundaries: they document shapes we do
    // not yet model in this PR and that future FSM-detection work may widen.
    સ્થિર તાર્કિક emitCaseItemArcs(FsmGraph& graph, AstCaseItem* itemp, AstVarScope* stateVscp,
                                 અચલ FsmStateSpace& stateSpace, તાર્કિક inclCond) {
        std::vector<std::pair<string, Fsm_State_કિંમત>> froms;
        જો (itemp->isDefault()) {
            જો (!inclCond) પાછા ખોટું;
            froms.emplace_back("default", Fsm_State_કિંમત{});
        } નહીંતર {
            માટે (Ast_ગાંઠ_પદાવલિ* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), ગાંઠ_પદાવલિ)) {
                Fsm_State_કિંમત કિંમત;
                જો (const_કિંમત_Status(condp, કિંમત) != Const_કિંમત_Status::OK) ચાલુ;
                જો (!validate_Known_State_કિંમત(condp, stateSpace, કિંમત, "source")) પાછા સાચું;
                froms.emplace_back(label_For_કિંમત(stateSpace.labels, કિંમત), કિંમત);
            }
            જો (froms.empty()) પાછા ખોટું;
        }

        જો (Ast_ગાંઠ_Assign* અચલ assp = directStateAssign(itemp->stmtsp(), stateVscp)) {
            Fsm_State_કિંમત to_કિંમત;
            અચલ Const_કિંમત_Status status = const_કિંમત_Status(assp->rhsp(), to_કિંમત);
            જો (status == Const_કિંમત_Status::OK) {
                જો (!validate_Known_State_કિંમત(assp, stateSpace, to_કિંમત, "target")) પાછા સાચું;
                માટે (અચલ std::pair<string, Fsm_State_કિંમત>& from : froms) {
                    graph.addArc(from.second, to_કિંમત, ખોટું, ખોટું, itemp->isDefault(),
                                 assp->fileline());
                }
                પાછા સાચું;
            }
        }

        Fsm_State_કિંમત then_કિંમત;
        Fsm_State_કિંમત else_કિંમત;
        જો (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_કિંમત, else_કિંમત)
            || ifStateConstAssign(itemp->stmtsp(), stateVscp, then_કિંમત, else_કિંમત)) {
            જો (!validate_Known_State_કિંમત(itemp->stmtsp(), stateSpace, then_કિંમત, "target"))
                પાછા સાચું;
            જો (!validate_Known_State_કિંમત(itemp->stmtsp(), stateSpace, else_કિંમત, "target"))
                પાછા સાચું;
            માટે (અચલ Fsm_State_કિંમત& branch_કિંમત : {then_કિંમત, else_કિંમત}) {
                માટે (અચલ std::pair<string, Fsm_State_કિંમત>& from : froms) {
                    graph.addArc(from.second, branch_કિંમત, ખોટું, સાચું, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            પાછા સાચું;
        }

        પાછા ખોટું;
    }

    સ્થિર શૂન્ય emitStmtArcsFrom(FsmGraph& graph, Ast_ગાંઠ* stmtsp, AstVarScope* stateVscp,
                                 અચલ FsmStateSpace& stateSpace, Fsm_State_કિંમત from_કિંમત,
                                 તાર્કિક isDefault, તાર્કિક forceCond) {
        Ast_ગાંઠ_Assign* અચલ assp = directStateAssign(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        Fsm_State_કિંમત to_કિંમત;
        અચલ Const_કિંમત_Status status = const_કિંમત_Status(assp->rhsp(), to_કિંમત);
        જો (status == Const_કિંમત_Status::OK) {
            જો (!validate_Known_State_કિંમત(assp, stateSpace, to_કિંમત, "target")) પાછા;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            graph.addArc(from_કિંમત, to_કિંમત, ખોટું, forceCond, isDefault, assp->fileline());
            પાછા;
        }

        Fsm_State_કિંમત then_કિંમત;
        Fsm_State_કિંમત else_કિંમત;
        અચલ તાર્કિક condAssign
            = directStateCondConstAssign(stmtsp, stateVscp, then_કિંમત, else_કિંમત);
        UASSERT_OBJ(condAssign, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        જો (!validate_Known_State_કિંમત(stmtsp, stateSpace, then_કિંમત, "target")) પાછા;
        જો (!validate_Known_State_કિંમત(stmtsp, stateSpace, else_કિંમત, "target")) પાછા;
        માટે (અચલ Fsm_State_કિંમત& branch_કિંમત : {then_કિંમત, else_કિંમત}) {
            graph.addArc(from_કિંમત, branch_કિંમત, ખોટું, સાચું, isDefault, stmtsp->fileline());
        }
    }

    સ્થિર શૂન્ય emitIfChainArcs(FsmGraph& graph, અચલ FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, અચલ FsmStateSpace& stateSpace) {
        માટે (size_t i = 0; i < chain.branches.size(); ++i) {
            અચલ FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            જો (!validate_Known_State_કિંમત(branch.ifp, stateSpace, branch.from_કિંમત, "source"))
                પાછા;
            emitStmtArcsFrom(graph, branch.stmtsp, stateVscp, stateSpace, branch.from_કિંમત, ખોટું,
                             branch.has_ટોચ_Guard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    સ્થિર શૂન્ય addResetArcs(FsmGraph& graph, અચલ std::vector<FsmResetArcDesc>& resetArcs,
                             અચલ FsmStateSpace& stateSpace) {
        માટે (અચલ FsmResetArcDesc& resetArc : resetArcs) {
            જો (!validate_Known_State_કિંમત(resetArc.nodep(), stateSpace, resetArc.to_કિંમત(),
                                         "target"))
                ચાલુ;
            graph.addArc(Fsm_State_કિંમત{}, resetArc.to_કિંમત(), સાચું, ખોટું, ખોટું,
                         resetArc.nodep()->fileline());
        }
    }

    // Turn one candidate case statement into the graph representation that the
    // later lowering phase will consume directly, while reviewers can still
    // inspect the extracted machine via DOT dumps.
    શૂન્ય processCase(AstCase* casep, AstVarScope* assignVscp, અચલ FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, casep, "FSM case processing requires a non-null assignment var");
        AstVarScope* અચલ stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        જો (!collectStateSpace(casep, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) પાછા;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        જો (!entry.graphp) {
            entry.graphp.reset(નવું FsmGraph{});
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
            માટે (અચલ std::pair<string, Fsm_State_કિંમત>& state : stateSpace.states) {
                entry.graphp->addStateVertex(state.first, state.second);
            }
            addResetArcs(*entry.graphp, reg.resetArcs(), stateSpace);
        }
        માટે (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            emitCaseItemArcs(*entry.graphp, itemp, assignVscp, stateSpace,
                             entry.graphp->inclCond());
        }
    }

    શૂન્ય processIfChain(અચલ FsmIfChainCandidate& chain, AstVarScope* assignVscp,
                        અચલ FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, chain.ifp,
                    "FSM if-chain processing requires a non-null assignment var");
        AstVarScope* અચલ stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        જો (!collectStateSpace(chain, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) પાછા;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        // Case candidates keep ownership of existing graphs; reaching this path
        // means the if-chain is the only supported dispatch for this FSM.
        UASSERT_OBJ(!entry.graphp, chain.ifp, "FSM if-chain graph should not already exist");
        entry.graphp.reset(નવું FsmGraph{});
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
        માટે (અચલ std::pair<string, Fsm_State_કિંમત>& state : stateSpace.states) {
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
    શૂન્ય processOneBlockAlways(અચલ FsmComboAlways& oneBlock) {
        અચલ RegisterAlwaysAnalyzer analyzer{oneBlock.scopep()};
        AstAlways* અચલ alwaysp = oneBlock.alwaysp();
        જો (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) પાછા;
        અચલ std::vector<std::pair<AstCase*, Ast_ગાંઠ_પદાવલિ*>> candidates
            = analyzer.oneBlockCandidates(alwaysp);

        FsmCaseCandidate firstCand;
        માટે (અચલ std::pair<AstCase*, Ast_ગાંઠ_પદાવલિ*>& cand : candidates) {
            AstVarRef* અચલ selp = VN_CAST(cand.first->exprp(), VarRef);
            AstVarScope* અચલ vscp = selp ? selp->varScopep() : nullptr;
            જો (!vscp) ચાલુ;
            જો (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processCase(cand.first, vscp, reg);
            } નહીંતર જો (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed case statements found in "
                                   "the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } નહીંતર {
                cand.first->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported case "
                                   "statements found in the same always block. Only the first "
                                   "candidate will be instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            }
        }

        જો (firstCand.stateVscp) પાછા;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        અચલ std::vector<std::pair<AstIf*, Ast_ગાંઠ_પદાવલિ*>> ifCandidates
            = analyzer.oneBlockIfCandidates(alwaysp);
        માટે (અચલ std::pair<AstIf*, Ast_ગાંઠ_પદાવલિ*>& cand : ifCandidates) {
            અચલ FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.first);
            FsmIfChainCandidate chain;
            જો (!collectIfChain(cand.first, aliases, chain)) ચાલુ;
            AstVarScope* અચલ vscp = chain.compareVscp;
            જો (!if_Chain_Supported_Transition_ગાંઠ(chain, vscp)) ચાલુ;
            જો (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processIfChain(chain, vscp, reg);
            } નહીંતર જો (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed transition candidates found "
                                   "in the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } નહીંતર {
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
    સ્થિર શૂન્ય warnComboSameAlways(Ast_ગાંઠ* warnNodep, અચલ FsmCaseCandidate& firstCand) {
        warnNodep->v3warn(FSMMULTI,
                          "FSM coverage: multiple supported transition candidates found in "
                          "the same combinational always block. Only the first candidate "
                          "will be instrumented."
                              << candidateConflictContext(warnNodep, firstCand));
    }

    શૂન્ય processComboAlways(અચલ FsmComboAlways& combo) {
        અચલ ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_ગાંઠ* અચલ stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate firstCand;
        માટે (Ast_ગાંઠ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* અચલ casep = VN_CAST(nodep, Case);
            જો (!casep) ચાલુ;
            અચલ ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchCase(stmtsp, casep);
            અચલ FsmRegisterCandidate* અચલ matchedp = match.matchedp;
            Ast_ગાંઠ* અચલ matchedWarnNodep = match.warnNodep;
            જો (!matchedp) ચાલુ;
            જો (!firstCand.stateVscp) {
                અચલ auto ઉમેરો_Pair = m_comboPaired.emplace(
                    matchedp->stateVscp(),
                    FsmCaseCandidate{matchedWarnNodep,
                                     const_cast<AstVarScope*>(matchedp->stateVscp())});
                જો (!ઉમેરો_Pair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  ઉમેરો_Pair.first->second));
                    ચાલુ;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processCase(casep, matchedp->nextVscp(), *matchedp);
                ચાલુ;
            }
            જો (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                ચાલુ;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported case statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, firstCand));
        }
        જો (firstCand.stateVscp) પાછા;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        માટે (Ast_ગાંઠ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* અચલ ifp = VN_CAST(nodep, If);
            જો (!ifp) ચાલુ;
            FsmIfChainCandidate chain;
            અચલ FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            જો (!collectIfChain(ifp, aliases, chain)) ચાલુ;
            અચલ ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchIfChain(stmtsp, chain);
            અચલ FsmRegisterCandidate* અચલ matchedp = match.matchedp;
            Ast_ગાંઠ* અચલ matchedWarnNodep = match.warnNodep;
            જો (!matchedp) ચાલુ;
            જો (!firstCand.stateVscp) {
                અચલ std::pair<std::unordered_map<અચલ AstVarScope*, FsmCaseCandidate>::iterator,
                                તાર્કિક>
                    ઉમેરો_Pair = m_comboPaired.emplace(
                        matchedp->stateVscp(),
                        FsmCaseCandidate{matchedWarnNodep,
                                         const_cast<AstVarScope*>(matchedp->stateVscp())});
                જો (!ઉમેરો_Pair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  ઉમેરો_Pair.first->second));
                    ચાલુ;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processIfChain(chain, matchedp->nextVscp(), *matchedp);
                ચાલુ;
            }
            જો (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                ચાલુ;
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
    શૂન્ય visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    // Collect processes first, then analyze FSM candidates once all alias and
    // register information is available.
    શૂન્ય visit(AstAlways* nodep) override {
        જો (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            પાછા;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_oneBlockAlwayss.emplace_back(m_scopep, nodep);
        અચલ RegisterAlwaysAnalyzer analyzer{m_scopep};
        FsmRegisterCandidate reg;
        જો (analyzer.matchRegisterCandidate(nodep, reg)) {
            AstVarScope* અચલ stateVscp = reg.stateVscp();
            અચલ તાર્કિક found
                = std::any_of(m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                              [stateVscp](અચલ FsmRegisterCandidate& existing) {
                                  પાછા existing.stateVscp() == stateVscp;
                              });
            જો (!found) { m_registerCandidates.emplace_back(reg); }
        }
        જો (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            m_comboAlwayss.emplace_back(m_scopep, nodep);
        } નહીંતર જો (nodep->keyword() == VAlwaysKwd::ALWAYS) {
            જો (!nodep->sentreep() || isPlainComboSentree(nodep->sentreep())) {
                m_comboAlwayss.emplace_back(m_scopep, nodep);
            } નહીંતર {
                m_nonComboAlwayss.emplace_back(m_scopep, nodep);
            }
        }
    }

    શૂન્ય visit(AstAssignW* nodep) override {
        // Continuous aliases are unordered hardware connections, so source
        // order should not affect whether an if-chain FSM is recognized.
        collectAliasFromAssign(nodep, m_stateAliases, m_ambiguousStateAliases);
        collectCellPortAlias(nodep);
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstCell* nodep) override {
        // Cells are matched after the full traversal because linkdot lowers
        // uninlined port connections into sibling continuous assignments.
        જો (m_scopep && fsmRegisterWrapperDesc(nodep)) addWrapperCell(m_scopep, nodep);
        iterateChildren(nodep);
    }

    // Continue the walk through the rest of the design hierarchy.
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        માટે (અચલ std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            FsmRegisterCandidate reg;
            જો (matchFsmWrapperCell(wrapperCell.first, wrapperCell.second, reg)) {
                m_registerCandidates.emplace_back(reg);
            }
        }
        માટે (અચલ FsmComboAlways& oneBlock : m_oneBlockAlwayss) processOneBlockAlways(oneBlock);
        માટે (અચલ FsmComboAlways& combo : m_comboAlwayss) processComboAlways(combo);
        માટે (અચલ FsmComboAlways& combo : m_nonComboAlwayss) warnUnsupportedComboAlways(combo);
    }
};

// Builder of statements that will handle incrementation of proper AstNodeCoverDecls - which will
// be also created here
વર્ગ FsmCoverIncBuilder final {
    અચલ FsmGraph& m_graph;  // Fsm graph
    std::vector<અચલ FsmStateVertex*> m_states;  // States vertexes inside fsm graph
    std::unordered_set<uint64_t>
        m_undefinedStates;  // States that have no enum value but are possible to represent with a
                            // variable of given width.
                            // This variable keeps uint64_t instead of V3Numebr because it is safe
                            // to assume that nobody has over 16M TB of RAM
    અચલ FsmVertex* m_defaultVertexp = nullptr;  // Default vertex
    અચલ FsmVertex* m_resetVertexp = nullptr;  // Reset vertex
    size_t m_edges_ગણતરી = 0;  // Total count of edges
    AstVarScope* અચલ m_prevVscp;  // Previous state of fsm
    File_લીટી* અચલ m_flp;  // Fileline pointer to use across the builder
    અચલ std::string m_modPrettyName;  // Pretty fsm mode name
    Ast_ગાંઠ_Stmt* m_coverIncp = nullptr;  // Increments proper CoverDecl after each change
    Ast_ગાંઠ_Stmt* m_initialCoverIncp = nullptr;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = nullptr;  // List of CoverDecls
    તાર્કિક m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(અચલ uint64_t કિંમત) {
        V3_સંખ્યા અંક{static_cast<Ast_ગાંઠ*>(nullptr), m_prevVscp->width()};
        અંક.setQuad(કિંમત);
        અંક.isSigned(m_prevVscp->dtypep()->isSigned());
        પાછા નવું AstConst{m_flp, અંક};
    }

    std::string quadToBitStirngLabel(અચલ uint64_t val) {
        પૂર્ણાંક width = m_prevVscp->width();
        UASSERT(width < 64, "We should not expand values that wide");
        std::string result = std::to_string(width) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        જ્યારે (--width >= 0) result.ધકેલો_back("01"[(val >> width) & 1]);
        પાછા result;
    }

    શૂન્ય initExistingStates() {
        માટે (અચલ V3GraphVertex& vtx : m_graph.vertices()) {
            અચલ FsmVertex* અચલ vertexp = vtx.as<FsmVertex>();
            m_edges_ગણતરી += vertexp->inEdges().size();
            જો (અચલ FsmStateVertex* અચલ statep = vertexp->cast<FsmStateVertex>()) {
                m_states.ધકેલો_back(statep);
            } નહીંતર જો (vertexp->isDefaultAny()) {
                UASSERT_OBJ(!std::exchange(m_defaultVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two defaults?");
            } નહીંતર /* if (vertexp->isResetAny()) */ {
                UASSERT_OBJ(!std::exchange(m_resetVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two resets?");
            }
        }
        અચલ પૂર્ણાંક width = m_graph.sampleVarScopep()->width();
        m_isComplete = width < 64 && m_states.size() == (static_cast<size_t>(1) << width);
    }

    શૂન્ય initUndefinedStates() {
        UASSERT_OBJ(m_graph.sampleVarScopep()->width() < 64, m_graph.sampleVarScopep(),
                    "This FSM is too wide to expand - this should be caught earlier");
        અચલ uint64_t combinations_ગણતરી = static_cast<size_t>(1)
                                           << m_graph.sampleVarScopep()->width();
        // Rolling trick allows to cut peak memory usage
        તાર્કિક undefRolled = ખોટું;
        auto rollUndefs = [આ, combinations_ગણતરી] {
            માટે (uint64_t i = 0; i < combinations_ગણતરી; ++i) {
                અચલ auto ઉમેરો = m_undefinedStates.ઉમેરો(i);
                જો (!ઉમેરો.second) m_undefinedStates.erase(ઉમેરો.first);
            }
        };
        માટે (અચલ FsmStateVertex* અચલ statep : m_states) {
            અચલ V3_સંખ્યા& અંક = statep->કિંમત().અંક();
            અચલ uint64_t કિંમત = અંક.toUQuad();
            જો (!m_isComplete) {
                જો (undefRolled) {
                    UASSERT_OBJ(m_undefinedStates.erase(કિંમત) == 1, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                } નહીંતર {
                    UASSERT_OBJ(m_undefinedStates.ઉમેરો(કિંમત).second, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                    જો (m_undefinedStates.size() * 2 >= combinations_ગણતરી) {
                        undefRolled = સાચું;
                        rollUndefs();
                    }
                }
            }
        }
        જો (!(m_isComplete || undefRolled)) rollUndefs();
    }

    Ast_ગાંઠ_પદાવલિ* buildResetCond() અચલ {
        Ast_ગાંઠ_પદાવલિ* અચલ refp
            = નવું AstVarRef{m_flp, m_graph.resetCond().varScopep, VAccess::READ};
        પાછા m_graph.resetCond().active_નીચલું
                   ? static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstLogNot{m_flp, refp})
                   : refp;
    }

    size_t get_Arc_ગણતરી_With_Expansion(અચલ VFsmExpandType expand) અચલ {
        size_t arc_ગણતરી_With_Expansion = ~0ULL;
        પસંદગી (expand) {
        સ્થિતિ VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_graph.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            તોડો;
        }
        સ્થિતિ VFsmExpandType::AUTO: {
            arc_ગણતરી_With_Expansion = m_edges_ગણતરી;
            જો (m_defaultVertexp) {
                UASSERT_OBJ(m_states.size() >= m_defaultVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arc_ગણતરી_With_Expansion -= m_defaultVertexp->outEdges().size();
                માટે (અચલ FsmStateVertex* અચલ statep : m_states) {
                    arc_ગણતરી_With_Expansion += static_cast<size_t>(statep->outEdges().empty())
                                             * m_defaultVertexp->outEdges().size();
                }
            }
            જો (m_resetVertexp) {
                UASSERT_OBJ(m_states.size() >= m_resetVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arc_ગણતરી_With_Expansion += m_states.size() - m_resetVertexp->outEdges().size();
            }
            તોડો;
        }
        સ્થિતિ VFsmExpandType::AUTO_EXPAND: {
            arc_ગણતરી_With_Expansion = m_states.size() * m_states.size() - m_states.size();
            તોડો;
        }
        સ્થિતિ VFsmExpandType::FULL: {
            અચલ પૂર્ણાંક width = m_graph.sampleVarScopep()->width();
            // 32 is derived from:
            // tranitionsCount <= std::numeric_limits<uint64_t>::max()
            // (2^width)^2 - (2^width) <= 2^64 - 1
            // (2^width)^2 - (2^width) < 2^64 since we operate on integral values we can trade 1
            // log_2((2^width)^2 - (2^width)) < 64
            // log_2((2^width)^2) < 64 - `(2^width)` is negigible so, it may be dropped
            // 2width < 64
            જો (width < 32) {
                અચલ uint64_t combinations_ગણતરી = static_cast<size_t>(1) << width;
                arc_ગણતરી_With_Expansion = combinations_ગણતરી * combinations_ગણતરી - combinations_ગણતરી;
            }
            તોડો;
        }
        }
        પાછા arc_ગણતરી_With_Expansion;
    }

    AstCoverOtherDecl* buildStateCoverDecl(અચલ std::string& label,
                                           અચલ std::string& fsmTag = "") {
        AstCoverOtherDecl* અચલ declp
            = નવું AstCoverOtherDecl{m_flp,
                                    "v_fsm_state/" + m_modPrettyName,
                                    m_graph.stateVarName() + "::" + label,
                                    "",
                                    0,
                                    m_graph.stateVarName(),
                                    "",
                                    label,
                                    fsmTag};
        // We run after scoping, so this is declared for one instance only
        declp->perInstance(સાચું);
        m_declsp = Ast_ગાંઠ::addNext(m_declsp, declp);
        પાછા declp;
    }

    Ast_ગાંઠ_Stmt* buildCoverIncStmtp(અચલ std::string& fromStateLabel,
                                    અચલ std::string& toStateLabel,
                                    અચલ std::string& baseLabel = "") {
        // Whole builder is designed in such a way that this function won't be called twice with
        // the same arguments - but if it will ever change in future, `AstCoverOtherDecl` shall be
        // cached for particular args set
        AstCoverOtherDecl* અચલ declp = નવું AstCoverOtherDecl{
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
        declp->perInstance(સાચું);
        m_declsp = Ast_ગાંઠ::addNext(m_declsp, declp);
        પાછા નવું AstCoverInc{m_flp, declp};
    }
    Ast_ગાંઠ_Stmt* buildExpandedTransitionHandler(અચલ std::string& toStateLabel,
                                                અચલ તાર્કિક considerUndefs,
                                                અચલ તાર્કિક expandOnlyWithTerminalVerticies,
                                                અચલ FsmStateVertex* અચલ toStatep = nullptr) {
        AstCase* અચલ arcCasep = નવું AstCase{
            m_flp, VCaseType::CT_CASE, નવું AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        std::unordered_set<અચલ FsmStateVertex*> handledSoureces;
        જો (toStatep) {
            માટે (અચલ V3GraphEdge& edge : toStatep->inEdges()) {
                જો (અચલ FsmStateVertex* અચલ fromp = edge.fromp()->cast<FsmStateVertex>()) {
                    જો (!handledSoureces.emplace(fromp).second) ચાલુ;
                    arcCasep->addItemsp(નવું AstCaseItem{
                        m_flp, makeConst(fromp->કિંમત().અંક().toUQuad()),
                        (fromp != toStatep) ? buildCoverIncStmtp(fromp->label(), toStateLabel)
                                            : nullptr});
                }
            }
        }
        માટે (અચલ FsmStateVertex* અચલ fromp : m_states) {
            જો (handledSoureces.find(fromp) != handledSoureces.end()) ચાલુ;
            arcCasep->addItemsp(નવું AstCaseItem{
                m_flp, makeConst(fromp->કિંમત().અંક().toUQuad()),
                (fromp != toStatep
                 && (!expandOnlyWithTerminalVerticies || fromp->outEdges().empty()))
                    ? buildCoverIncStmtp(fromp->label(), toStateLabel, "artificial")
                    : nullptr});
        }
        જો (!m_isComplete) {
            જો (considerUndefs) {
                માટે (અચલ uint64_t from : m_undefinedStates) {
                    અચલ std::string& label = quadToBitStirngLabel(from);
                    arcCasep->addItemsp(
                        નવું AstCaseItem{m_flp, makeConst(from),
                                        (label != toStateLabel)
                                            ? buildCoverIncStmtp(label, toStateLabel, "artificial")
                                            : nullptr});
                }
            } નહીંતર {
                arcCasep->addItemsp(નવું AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        પાછા arcCasep;
    }
    Ast_ગાંઠ_Stmt* buildUnxpandedTransitionHandler(અચલ FsmStateVertex* અચલ toStatep) {
        AstCase* અચલ arcCasep = નવું AstCase{
            m_flp, VCaseType::CT_CASE, નવું AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        માટે (અચલ V3GraphEdge& edge : toStatep->inEdges()) {
            અચલ FsmArcEdge* અચલ arcp = edge.as<FsmArcEdge>();
            UASSERT_OBJ(!arcp->isReset(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            અચલ FsmStateVertex* અચલ fromp = arcp->fromp()->as<FsmStateVertex>();
            જો (fromp != toStatep) {
                arcCasep->addItemsp(
                    નવું AstCaseItem{m_flp, makeConst(fromp->કિંમત().અંક().toUQuad()),
                                    buildCoverIncStmtp(fromp->label(), toStatep->label())});
            }
        }
        arcCasep->addItemsp(નવું AstCaseItem{m_flp, nullptr, nullptr});
        પાછા arcCasep;
    }

જાહેર:
    FsmCoverIncBuilder(અચલ FsmGraph& graph, AstVarScope* અચલ prevVscp)
        : m_graph{graph}
        , m_prevVscp{prevVscp}
        , m_flp{graph.fileline()}
        , m_modPrettyName{graph.scopep()->modp()->prettyName()} {
        initExistingStates();
        AstVarScope* અચલ sampleVscp = graph.sampleVarScopep();
        અચલ VFsmExpandType expand
            = graph.stateVarScopep()->varp()->attrFsmStateExpand() != VFsmExpandType::DEFAULT
                  ? graph.stateVarScopep()->varp()->attrFsmStateExpand()
                  : v3Global.opt.coverageFsmExpand();

        {
            અચલ size_t arc_ગણતરી_With_Expansion = get_Arc_ગણતરી_With_Expansion(expand);
            જો (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arc_ગણતરી_With_Expansion == 0ULL
                            || arc_ગણતરી_With_Expansion
                                   > static_cast<size_t>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arc_ગણતરી_With_Expansion);
                પાછા;
            }
        }

        AstCase* અચલ arcCasep
            = નવું AstCase{m_flp, VCaseType::CT_CASE,
                          નવું AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};
        AstCase* અચલ initialStateCasep
            = નવું AstCase{m_flp, VCaseType::CT_CASE,
                          નવું AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};

        અચલ તાર્કિક expandAll = expand != VFsmExpandType::AUTO;
        std::unordered_set<અચલ FsmStateVertex*>
            defaultTargets;  // States that have edge incoming from default vertex
        std::unordered_set<અચલ FsmStateVertex*>
            resetTargets;  // States that have edge incoming from reset vertex
        જો (!expandAll) {
            જો (m_defaultVertexp) {
                માટે (અચલ V3GraphEdge& edge : m_defaultVertexp->outEdges()) {
                    defaultTargets.emplace(edge.ટોચ()->as<FsmStateVertex>());
                }
            }
            જો (m_resetVertexp) {
                માટે (અચલ V3GraphEdge& edge : m_resetVertexp->outEdges()) {
                    resetTargets.emplace(edge.ટોચ()->as<FsmStateVertex>());
                }
            }
        }
        અચલ તાર્કિક isFullExpansion = expand == VFsmExpandType::FULL;
        જો (isFullExpansion) initUndefinedStates();
        માટે (અચલ FsmStateVertex* અચલ statep : m_states) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* અચલ declp = buildStateCoverDecl(statep->label());
            initialStateCasep->addItemsp(
                નવું AstCaseItem{m_flp, makeConst(statep->કિંમત().અંક().toUQuad()),
                                નવું AstCoverInc{m_flp, declp, સાચું}});
            અચલ તાર્કિક expandAllOrReset
                = expandAll || (resetTargets.find(statep) != resetTargets.end());
            અચલ તાર્કિક defaultExpantion
                = !expandAllOrReset && (defaultTargets.find(statep) != defaultTargets.end());
            arcCasep->addItemsp(નવું AstCaseItem{
                m_flp, makeConst(statep->કિંમત().અંક().toUQuad()),
                Ast_ગાંઠ::addNext<Ast_ગાંઠ_Stmt>(
                    નવું AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), isFullExpansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        જો (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            જો (isFullExpansion) {
                માટે (અચલ uint64_t કિંમત : m_undefinedStates) {
                    અચલ std::string& label = quadToBitStirngLabel(કિંમત);
                    AstCoverOtherDecl* અચલ declp = buildStateCoverDecl(label);
                    initialStateCasep->addItemsp(નવું AstCaseItem{
                        m_flp, makeConst(કિંમત), નવું AstCoverInc{m_flp, declp, સાચું}});
                    arcCasep->addItemsp(
                        નવું AstCaseItem{m_flp, makeConst(કિંમત),
                                        Ast_ગાંઠ::addNext<Ast_ગાંઠ_Stmt>(
                                            નવું AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, સાચું, ખોટું))});
                }
            } નહીંતર {
                // No expansion
                arcCasep->addItemsp(નવું AstCaseItem{m_flp, nullptr, nullptr});
                initialStateCasep->addItemsp(નવું AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        m_coverIncp = arcCasep;
        m_initialCoverIncp = initialStateCasep;
    }
    VL_UNCOPYABLE(FsmCoverIncBuilder);
    ~FsmCoverIncBuilder() = મૂળભૂત;
    Ast_ગાંઠ_Cover_Decl* getCoverDeclsp() અચલ { પાછા m_declsp; }
    Ast_ગાંઠ_Stmt* getCoverIncp() અચલ { પાછા m_coverIncp; }
    Ast_ગાંઠ_Stmt* getInitialCoverIncp() અચલ { પાછા m_initialCoverIncp; }
};

// Lower the completed FSM graphs into the concrete coverage declarations,
// previous-state tracking, and pre/post-triggered instrumentation that the
// runtime uses to record state and transition coverage.
વર્ગ FsmLowerVisitor final {
    // STATE - across all visitors
    અચલ FsmState& m_state;
    V3SharedTmps m_prevTmps{"__VfsmCovPrev", VVarType::MODULETEMP};  // Previous states

    // METHODS
    // Rebuild the original event control from the saved sense description so
    // post-state coverage sampling runs on the same triggering edges.
    સ્થિર Ast_Sen_વૃક્ષ* build_Sen_વૃક્ષ(File_લીટી* flp, અચલ std::vector<FsmSenDesc>& senses) {
        Ast_Sen_વૃક્ષ* અચલ sentreep = નવું Ast_Sen_વૃક્ષ{flp, nullptr};
        માટે (અચલ FsmSenDesc& sense : senses) {
            AstSenItem* અચલ senItemp
                = નવું AstSenItem{flp, VEdgeType{sense.edgeType},
                                 નવું AstVarRef{flp, sense.varScopep, VAccess::READ}};
            sentreep->addSensesp(senItemp);
        }
        પાછા sentreep;
    }

    // Lower one fully detected FSM graph into the concrete coverage machinery
    // used by generated models: declarations, previous-state tracking, and the
    // pre/post-triggered increment logic for states and arcs.
    શૂન્ય buildOne(અચલ FsmGraph& graph) {
        UINFO(1, "buildOne lowering FSM " << graph.stateVarName()
                                          << " vertices=" << graph.vertices().size() << endl);
        AstAlways* અચલ alwaysp = graph.stateAlwaysp();
        AstScope* અચલ scopep = graph.scopep();
        AstVarScope* અચલ stateVscp = graph.stateVarScopep();
        AstVarScope* અચલ sampleVscp = graph.sampleVarScopep();
        File_લીટી* અચલ flp = graph.fileline();
        Ast_ગાંઠ_Module* અચલ modp = scopep->modp();
        AstVarScope* અચલ prevVscp
            = m_prevTmps.make(flp, scopep, sampleVscp->dtypep(), stateVscp->varp()->shortName());
        // The saved previous-state temp crosses the scheduler's pre/post split
        // in the same way as Verilator's built-in NBA shadow variables, so keep
        // both vars marked as post-life participants for stable MT ordering.
        stateVscp->optimizeLifePost(સાચું);
        sampleVscp->optimizeLifePost(સાચું);
        prevVscp->optimizeLifePost(સાચું);

        AstActive* અચલ initActivep
            = નવું AstActive{flp, "fsm-coverage-init",
                            નવું Ast_Sen_વૃક્ષ{flp, નવું AstSenItem{flp, AstSenItem::Initial{}}}};
        initActivep->sen_વૃક્ષ_Storep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->addStmtsp(નવું AstInitialStatic{
            flp, નવું AstAssign{flp, નવું AstVarRef{flp, prevVscp, VAccess::WRITE},
                               નવું AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->addBlocksp(initActivep);

        AstAlwaysPost* અચલ covPostp = નવું AstAlwaysPost{flp};
        તાર્કિક updatePrevAfterPost = ખોટું;
        જો (alwaysp) {
            // Save the previous state as plain sequential logic at the front of
            // the original always_ff body, then evaluate coverage in post logic
            // after the delayed state update commits. This avoids a scheduler
            // race between a separate AstAlwaysPre task and the real state
            // commit for direct parent-level registers.
            alwaysp->prependStmtsp(નવું AstAssign{flp, નવું AstVarRef{flp, prevVscp, VAccess::WRITE},
                                                 નવું AstVarRef{flp, sampleVscp, VAccess::READ}});
        } નહીંતર {
            // Wrapper-derived register candidates do not have a parent
            // always_ff body to splice into. Sample coverage first, then save
            // the current state for the next clock tick; this survives cell
            // boundary scheduling where the real flop update lives elsewhere.
            updatePrevAfterPost = સાચું;
            prevVscp->varp()->setIgnorePostRead();
        }

        {
            // Create and increment proper AstNodeCoverDecls
            અચલ FsmCoverIncBuilder coverIncBuilder{graph, prevVscp};
            modp->addStmtsp(coverIncBuilder.getCoverDeclsp());
            initActivep->addStmtsp(coverIncBuilder.getInitialCoverIncp());
            covPostp->addStmtsp(
                નવું AstIf{flp,
                          નવું AstNeq{flp, નવું AstVarRef{flp, prevVscp, VAccess::READ},
                                     નવું AstVarRef{flp, sampleVscp, VAccess::READ}},
                          coverIncBuilder.getCoverIncp()});
        }

        જો (updatePrevAfterPost) {
            covPostp->addStmtsp(નવું AstAssign{flp, નવું AstVarRef{flp, prevVscp, VAccess::WRITE},
                                              નવું AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        Ast_Sen_વૃક્ષ* અચલ sentreep = build_Sen_વૃક્ષ(flp, graph.senses());
        AstActive* અચલ activep = નવું AstActive{flp, "fsm-coverage", sentreep};
        activep->sen_વૃક્ષ_Storep(sentreep);
        scopep->addBlocksp(activep);
        activep->addStmtsp(covPostp);
    }

જાહેર:
    // CONSTRUCTORS
    // Lower every detected FSM graph from the shared local state into
    // concrete coverage instrumentation while the saved scoped pointers are
    // still valid in the same pass.
    explicit FsmLowerVisitor(અચલ FsmState& state)
        : m_state{state} {
        માટે (અચલ DetectedFsm& fsm : m_state.fsms()) { buildOne(*fsm.graphp); }
    }
};

}  // namespace

શૂન્ય V3FsmDetect::detect(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    FsmState state;
    // Phase 1: recover each supported FSM into a complete graph while the
    // original clocked/case structure is still easy to recognize.
    FsmDetectVisitor detect{state, rootp};
    જો (dumpGraphLevel() >= 6) {
        size_t index = 0;
        માટે (અચલ DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dumpTag(index++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { FsmLowerVisitor lower{state}; }
    V3Global::dump_Check_Global_વૃક્ષ("fsm-detect", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
