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

#അന്തര്ഭാവ "V3PchAstNoMT.h"

#അന്തര്ഭാവ "V3FsmDetect.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Control.h"
#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3SharedTmps.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <cctype>
#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <memory>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

നാമാകാശ {

// Width-preserving FSM state identity. FSM detection needs a stable key for
// graph vertices and lookup tables, but lowering still needs the original
// folded Verilog value so emitted comparisons keep the correct width and bits.
വർഗ്ഗം Fsm_State_മൂല്യം final {
    // Hash/equality key only. It deliberately ignores signedness because
    // signed and unsigned constants with the same width and bits denote the
    // same encoded FSM state.
    string m_താക്കോൽ;  // Canonical "width:value" identity, independent of signedness

    // Semantic value. This is what diagnostics and lowering use when printing
    // values or rebuilding AstConst nodes for instrumentation.
    V3_സംഖ്യ m_അക്കം;  // Original folded value, preserving width for lowered comparisons

    സ്ഥിതം string make_താക്കോൽ(സ്ഥിരം V3_സംഖ്യ& അക്കം) {
        V3_സംഖ്യ താക്കോൽ_അക്കം = അക്കം;
        // Signedness does not change FSM state identity: same width and bits
        // should address the same graph vertex.
        താക്കോൽ_അക്കം.isSigned(അസത്യം);
        തിരികെ cvtToStr(താക്കോൽ_അക്കം.width()) + ":" + താക്കോൽ_അക്കം.ascii(സത്യം, സത്യം);
    }

പൊതു:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    Fsm_State_മൂല്യം()
        : m_താക്കോൽ{"1:1'h0"}
        , m_അക്കം{static_cast<Ast_കണ്ണി*>(nullptr), 1, 0} {}
    explicit Fsm_State_മൂല്യം(സ്ഥിരം V3_സംഖ്യ& അക്കം)
        : m_താക്കോൽ{make_താക്കോൽ(അക്കം)}
        , m_അക്കം{അക്കം} {}

    സ്ഥിരം string& താക്കോൽ() സ്ഥിരം { തിരികെ m_താക്കോൽ; }
    സ്ഥിരം V3_സംഖ്യ& അക്കം() സ്ഥിരം { തിരികെ m_അക്കം; }
    string ascii() സ്ഥിരം { തിരികെ m_അക്കം.ascii(സത്യം, സത്യം); }
    string warnText() സ്ഥിരം {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        എങ്കിൽ (m_അക്കം.width() <= 32) തിരികെ cvtToStr(m_അക്കം.toUInt());
        തിരികെ ascii();
    }

    തർക്കം operator==(സ്ഥിരം Fsm_State_മൂല്യം& rhs) സ്ഥിരം { തിരികെ m_താക്കോൽ == rhs.m_താക്കോൽ; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
ഘടന Fsm_State_മൂല്യം_Hash final {
    size_t operator()(സ്ഥിരം Fsm_State_മൂല്യം& മൂല്യം) സ്ഥിരം {
        തിരികെ std::hash<string>{}(മൂല്യം.താക്കോൽ());
    }
};

// Captures one sensitivity-list entry so the lowering phase can later rebuild
// an active block with the same triggering event control.
ഘടന FsmSenDesc final {
    // Encoded edge kind copied from AstSenItem::edgeType() so lowering can
    // rebuild the same trigger semantics on the synthesized coverage block.
    VEdgeType::en edgeType = static_cast<VEdgeType::en>(0);
    // Triggering signal in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
};

// Captures the simple reset predicate shape that survives to this pass after
// earlier normalization so reset arcs can be reconstructed during lowering.
ഘടന FsmResetCondDesc final {
    // Reset signal used by the FSM in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
    തർക്കം active_താഴെയുള്ള = അസത്യം;
};

വർഗ്ഗം FsmResetArcDesc final {
    Fsm_State_മൂല്യം m_to_മൂല്യം;  // Encoded reset target state.
    Ast_കണ്ണി* m_nodep = nullptr;  // Source node for warnings and emitted metadata.
    Ast_കണ്ണി_വ്യഞ്ജകം* m_valuep = nullptr;  // Expression that provided the reset value.

പൊതു:
    FsmResetArcDesc() = സ്ഥിരസ്ഥിതി;
    FsmResetArcDesc(Fsm_State_മൂല്യം to_മൂല്യം, Ast_കണ്ണി_Assign* nodep)
        : m_to_മൂല്യം{to_മൂല്യം}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(Fsm_State_മൂല്യം to_മൂല്യം, Ast_കണ്ണി* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* valuep)
        : m_to_മൂല്യം{to_മൂല്യം}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    Fsm_State_മൂല്യം to_മൂല്യം() സ്ഥിരം { തിരികെ m_to_മൂല്യം; }
    Ast_കണ്ണി* nodep() സ്ഥിരം { തിരികെ m_nodep; }
    Ast_കണ്ണി_വ്യഞ്ജകം* valuep() സ്ഥിരം { തിരികെ m_valuep; }
};

ഘടന FsmWrapperRoles final {
    string dPort;
    string qPort;
    string clkPort;
    string rstPort;
    string rstValParam;
    തർക്കം has_Rst_Active_താഴെയുള്ള = അസത്യം;
    തർക്കം rst_Active_താഴെയുള്ള = അസത്യം;
};

സ്ഥിതം തർക്കം fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, സ്ഥിരം string& portName,
                                                  തർക്കം& active_താഴെയുള്ള) {
    തർക്കം matched = അസത്യം;
    cellp->modp()->foreach([&](AstSenItem* itemp) {
        Ast_കണ്ണി_Var_Ref* സ്ഥിരം vrefp = itemp->varrefp();
        എങ്കിൽ (!vrefp) തിരികെ;
        എങ്കിൽ (vrefp->varp()->name() != portName) തിരികെ;
        active_താഴെയുള്ള = itemp->edgeType() == VEdgeType::ET_NEGEDGE;
        matched = സത്യം;
    });
    തിരികെ matched;
}

സ്ഥിതം സ്ഥിരം V3Control::FsmRegisterWrapper* fsmRegisterWrapperDesc(AstCell* cellp) {
    Ast_കണ്ണി_Module* സ്ഥിരം modp = cellp->modp();
    സ്ഥിരം string origName = modp->origName();
    എങ്കിൽ (സ്ഥിരം V3Control::FsmRegisterWrapper* സ്ഥിരം descp
        = V3Control::getFsmRegisterWrapper(origName)) {
        തിരികെ descp;
    }
    തിരികെ V3Control::getFsmRegisterWrapper(modp->prettyDehashOrigOrName());
}

സ്ഥിതം FsmWrapperRoles rolesFromDesc(സ്ഥിരം V3Control::FsmRegisterWrapper& desc) {
    FsmWrapperRoles roles;
    roles.dPort = desc.d;
    roles.qPort = desc.q;
    roles.clkPort = desc.clock;
    roles.rstPort = desc.reset;
    roles.rstValParam = desc.reset_മൂല്യം;
    തിരികെ roles;
}

വർഗ്ഗം FsmRegisterCandidate final {
    AstScope* m_scopep = nullptr;  // Owning scope for the paired FSM.
    AstAlways* m_alwaysp = nullptr;  // Register process that commits the state.
    AstVarScope* m_stateVscp = nullptr;  // Registered FSM state variable.
    AstVarScope* m_sampleVscp = nullptr;  // Variable sampled by lowered coverage logic.
    AstVarScope* m_nextVscp = nullptr;  // Next-state variable or same state var for 1-block FSMs.
    std::vector<FsmSenDesc> m_senses;  // Event controls for recreated coverage blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate, if any.
    std::vector<FsmResetArcDesc> m_resetArcs;  // Reset target arcs recovered during detect.
    തർക്കം m_hasResetCond = അസത്യം;  // Whether the FSM had a modeled reset predicate.
    തർക്കം m_inclCond = അസത്യം;  // Whether conditional/default arcs are kept explicitly.
    File_വരി* m_flp = nullptr;  // Representative source location.

പൊതു:
    AstScope* scopep() സ്ഥിരം { തിരികെ m_scopep; }
    ശൂന്യം scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* alwaysp() സ്ഥിരം { തിരികെ m_alwaysp; }
    ശൂന്യം alwaysp(AstAlways* alwaysp) { m_alwaysp = alwaysp; }
    AstVarScope* stateVscp() സ്ഥിരം { തിരികെ m_stateVscp; }
    ശൂന്യം stateVscp(AstVarScope* vscp) { m_stateVscp = vscp; }
    AstVarScope* sampleVscp() സ്ഥിരം { തിരികെ m_sampleVscp ? m_sampleVscp : m_stateVscp; }
    ശൂന്യം sampleVscp(AstVarScope* vscp) { m_sampleVscp = vscp; }
    AstVarScope* nextVscp() സ്ഥിരം { തിരികെ m_nextVscp; }
    ശൂന്യം nextVscp(AstVarScope* vscp) { m_nextVscp = vscp; }
    സ്ഥിരം std::vector<FsmSenDesc>& senses() സ്ഥിരം { തിരികെ m_senses; }
    std::vector<FsmSenDesc>& senses() { തിരികെ m_senses; }
    സ്ഥിരം FsmResetCondDesc& resetCond() സ്ഥിരം { തിരികെ m_resetCond; }
    FsmResetCondDesc& resetCond() { തിരികെ m_resetCond; }
    സ്ഥിരം std::vector<FsmResetArcDesc>& resetArcs() സ്ഥിരം { തിരികെ m_resetArcs; }
    std::vector<FsmResetArcDesc>& resetArcs() { തിരികെ m_resetArcs; }
    തർക്കം hasResetCond() സ്ഥിരം { തിരികെ m_hasResetCond; }
    ശൂന്യം hasResetCond(തർക്കം flag) { m_hasResetCond = flag; }
    തർക്കം inclCond() സ്ഥിരം { തിരികെ m_inclCond; }
    ശൂന്യം inclCond(തർക്കം flag) { m_inclCond = flag; }
    File_വരി* fileline() സ്ഥിരം { തിരികെ m_flp; }
    ശൂന്യം fileline(File_വരി* flp) { m_flp = flp; }
};

വർഗ്ഗം FsmComboAlways final {
    AstScope* സ്ഥിരം m_scopep = nullptr;  // Owning scope for the combinational process.
    AstAlways* സ്ഥിരം m_alwaysp = nullptr;  // Candidate transition process.

പൊതു:
    FsmComboAlways() = സ്ഥിരസ്ഥിതി;
    FsmComboAlways(AstScope* scopep, AstAlways* alwaysp)
        : m_scopep{scopep}
        , m_alwaysp{alwaysp} {}

    AstScope* scopep() സ്ഥിരം { തിരികെ m_scopep; }
    AstAlways* alwaysp() സ്ഥിരം { തിരികെ m_alwaysp; }
};

വർഗ്ഗം FsmGraph;

വർഗ്ഗം FsmVertex VL_NOT_FINAL : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(FsmVertex, V3GraphVertex)

പൊതു:
    എണ്ണൽ വർഗ്ഗം Kind : uint8_t { STATE, RESET_ANY, DEFAULT_ANY };

സ്വകാര്യ:
    Kind m_kind;  // State vs synthetic ANY/default vertex role.
    string m_label;  // User-facing state or pseudo-state label.
    Fsm_State_മൂല്യം m_മൂല്യം;  // Encoded state value for real state vertices.

സംരക്ഷിത:
    FsmVertex(V3Graph* graphp, Kind kind, string label, Fsm_State_മൂല്യം മൂല്യം) VL_MT_DISABLED
        : V3GraphVertex{graphp},
          m_kind{kind},
          m_label{label},
          m_മൂല്യം{മൂല്യം} {}
    ~FsmVertex() override = സ്ഥിരസ്ഥിതി;

പൊതു:
    Kind kind() സ്ഥിരം { തിരികെ m_kind; }
    തർക്കം isState() സ്ഥിരം { തിരികെ m_kind == Kind::STATE; }
    തർക്കം isResetAny() സ്ഥിരം { തിരികെ m_kind == Kind::RESET_ANY; }
    തർക്കം isDefaultAny() സ്ഥിരം { തിരികെ m_kind == Kind::DEFAULT_ANY; }
    സ്ഥിരം string& label() സ്ഥിരം { തിരികെ m_label; }
    സ്ഥിരം Fsm_State_മൂല്യം& മൂല്യം() സ്ഥിരം { തിരികെ m_മൂല്യം; }

    string name() സ്ഥിരം override VL_MT_SAFE { തിരികെ m_label + "=" + m_മൂല്യം.ascii(); }
};

വർഗ്ഗം FsmStateVertex final : പൊതു FsmVertex {
    VL_RTTI_IMPL(FsmStateVertex, FsmVertex)

പൊതു:
    FsmStateVertex(V3Graph* graphp, string label, Fsm_State_മൂല്യം മൂല്യം) VL_MT_DISABLED
        : FsmVertex{graphp, Kind::STATE, label, മൂല്യം} {}
    ~FsmStateVertex() override = സ്ഥിരസ്ഥിതി;

    string dotColor() സ്ഥിരം override { തിരികെ "lightblue"; }
    string dotShape() സ്ഥിരം override { തിരികെ "ellipse"; }
};

വർഗ്ഗം FsmPseudoVertex final : പൊതു FsmVertex {
    VL_RTTI_IMPL(FsmPseudoVertex, FsmVertex)

പൊതു:
    FsmPseudoVertex(V3Graph* graphp, Kind kind, string label) VL_MT_DISABLED
        : FsmVertex{graphp, kind, label, Fsm_State_മൂല്യം{}} {}
    ~FsmPseudoVertex() override = സ്ഥിരസ്ഥിതി;

    string name() സ്ഥിരം override VL_MT_SAFE { തിരികെ label(); }
    string dotColor() സ്ഥിരം override { തിരികെ isResetAny() ? "darkgreen" : "orange"; }
    string dotShape() സ്ഥിരം override { തിരികെ "diamond"; }
};

വർഗ്ഗം FsmArcEdge final : പൊതു V3GraphEdge {
    VL_RTTI_IMPL(FsmArcEdge, V3GraphEdge)
    തർക്കം m_isReset = അസത്യം;  // Arc originates from the synthetic reset source.
    തർക്കം m_isCond = അസത്യം;  // Arc came from a conditional next-state split.
    തർക്കം m_isDefault = അസത്യം;  // Arc represents a case default source.
    File_വരി* m_flp = nullptr;  // Source location for emitted coverage metadata.

പൊതു:
    FsmArcEdge(V3Graph* graphp, FsmVertex* fromp, FsmStateVertex* മുകൾ, തർക്കം isReset, തർക്കം isCond,
               തർക്കം isDefault, File_വരി* flp) VL_MT_DISABLED : V3GraphEdge{graphp, fromp, മുകൾ, 1},
                                                               m_isReset{isReset},
                                                               m_isCond{isCond},
                                                               m_isDefault{isDefault},
                                                               m_flp{flp} {}
    ~FsmArcEdge() override = സ്ഥിരസ്ഥിതി;

    തർക്കം isReset() സ്ഥിരം { തിരികെ m_isReset; }
    തർക്കം isCond() സ്ഥിരം { തിരികെ m_isCond; }
    തർക്കം isDefault() സ്ഥിരം { തിരികെ m_isDefault; }
    File_വരി* fileline() സ്ഥിരം { തിരികെ m_flp; }

    string dotLabel() സ്ഥിരം override {
        എങ്കിൽ (m_isReset) തിരികെ "reset";
        എങ്കിൽ (m_isDefault) തിരികെ "default";
        എങ്കിൽ (m_isCond) തിരികെ "cond";
        തിരികെ "";
    }
    string dotColor() സ്ഥിരം override {
        എങ്കിൽ (m_isReset) തിരികെ "darkgreen";
        എങ്കിൽ (m_isDefault) തിരികെ "orange";
        എങ്കിൽ (m_isCond) തിരികെ "blue";
        തിരികെ "black";
    }
};

// One graph per detected FSM. Graph-level metadata captures the non-graph
// context needed to lower states/arcs back into the AST after detection.
വർഗ്ഗം FsmGraph final : പൊതു V3Graph {
    AstScope* m_scopep = nullptr;  // Owning scoped block for the detected FSM.
    AstAlways* m_stateAlwaysp = nullptr;  // Register always block being instrumented.
    string m_stateVarName;  // Pretty state variable name for user-visible output.
    string m_stateVarInternalName;  // Internal state symbol name for dump tags.
    AstVarScope* m_stateVarScopep = nullptr;  // Scoped state variable being tracked.
    AstVarScope* m_sampleVarScopep = nullptr;  // Scoped variable sampled by coverage logic.
    std::vector<FsmSenDesc> m_senses;  // Saved event controls for recreated active blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate shape, if one exists.
    തർക്കം m_hasResetCond = അസത്യം;  // Whether the detected FSM had a reset branch.
    തർക്കം m_inclCond = അസത്യം;  // Whether conditional arcs should be kept explicitly.
    File_വരി* m_flp = nullptr;  // Representative source location for declarations/arcs.
    std::unordered_map<Fsm_State_മൂല്യം, FsmStateVertex*, Fsm_State_മൂല്യം_Hash>
        m_stateVertices;  // Value to state map.
    FsmPseudoVertex* m_resetVertexp = nullptr;  // Synthetic ANY source for reset arcs.
    FsmPseudoVertex* m_defaultVertexp = nullptr;  // Synthetic default source for case defaults.

പൊതു:
    FsmGraph() VL_MT_DISABLED
        : m_resetVertexp{പുതിയ FsmPseudoVertex{ഇത്, FsmVertex::Kind::RESET_ANY, "ANY"}},
          m_defaultVertexp{പുതിയ FsmPseudoVertex{ഇത്, FsmVertex::Kind::DEFAULT_ANY, "default"}} {}

    AstScope* scopep() സ്ഥിരം { തിരികെ m_scopep; }
    ശൂന്യം scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* stateAlwaysp() സ്ഥിരം { തിരികെ m_stateAlwaysp; }
    ശൂന്യം stateAlwaysp(AstAlways* alwaysp) { m_stateAlwaysp = alwaysp; }
    സ്ഥിരം string& stateVarName() സ്ഥിരം { തിരികെ m_stateVarName; }
    ശൂന്യം stateVarName(സ്ഥിരം string& name) { m_stateVarName = name; }
    സ്ഥിരം string& stateVarInternalName() സ്ഥിരം { തിരികെ m_stateVarInternalName; }
    ശൂന്യം stateVarInternalName(സ്ഥിരം string& name) { m_stateVarInternalName = name; }
    AstVarScope* stateVarScopep() സ്ഥിരം { തിരികെ m_stateVarScopep; }
    ശൂന്യം stateVarScopep(AstVarScope* vscp) { m_stateVarScopep = vscp; }
    AstVarScope* sampleVarScopep() സ്ഥിരം {
        തിരികെ m_sampleVarScopep ? m_sampleVarScopep : m_stateVarScopep;
    }
    ശൂന്യം sampleVarScopep(AstVarScope* vscp) { m_sampleVarScopep = vscp; }
    സ്ഥിരം std::vector<FsmSenDesc>& senses() സ്ഥിരം { തിരികെ m_senses; }
    std::vector<FsmSenDesc>& senses() { തിരികെ m_senses; }
    സ്ഥിരം FsmResetCondDesc& resetCond() സ്ഥിരം { തിരികെ m_resetCond; }
    FsmResetCondDesc& resetCond() { തിരികെ m_resetCond; }
    തർക്കം hasResetCond() സ്ഥിരം { തിരികെ m_hasResetCond; }
    ശൂന്യം hasResetCond(തർക്കം flag) { m_hasResetCond = flag; }
    തർക്കം inclCond() സ്ഥിരം { തിരികെ m_inclCond; }
    ശൂന്യം inclCond(തർക്കം flag) { m_inclCond = flag; }
    File_വരി* fileline() സ്ഥിരം { തിരികെ m_flp; }
    ശൂന്യം fileline(File_വരി* flp) { m_flp = flp; }

    FsmStateVertex* addStateVertex(string label, Fsm_State_മൂല്യം മൂല്യം) VL_MT_DISABLED {
        FsmStateVertex* സ്ഥിരം vertexp = പുതിയ FsmStateVertex{ഇത്, label, മൂല്യം};
        m_stateVertices.emplace(മൂല്യം, vertexp);
        തിരികെ vertexp;
    }
    FsmPseudoVertex* resetAnyVertex() VL_MT_DISABLED { തിരികെ m_resetVertexp; }
    FsmPseudoVertex* defaultAnyVertex() VL_MT_DISABLED { തിരികെ m_defaultVertexp; }
    FsmArcEdge* addArc(Fsm_State_മൂല്യം from_മൂല്യം, Fsm_State_മൂല്യം to_മൂല്യം, തർക്കം isReset, തർക്കം isCond,
                       തർക്കം isDefault, File_വരി* flp) VL_MT_DISABLED {
        FsmStateVertex* സ്ഥിരം മുകൾ = m_stateVertices.at(to_മൂല്യം);
        FsmVertex* fromp = nullptr;
        എങ്കിൽ (isReset) {
            fromp = resetAnyVertex();
        } അല്ലെങ്കിൽ എങ്കിൽ (isDefault) {
            fromp = defaultAnyVertex();
        } അല്ലെങ്കിൽ {
            fromp = m_stateVertices.at(from_മൂല്യം);
        }
        തിരികെ പുതിയ FsmArcEdge{ഇത്, fromp, മുകൾ, isReset, isCond, isDefault, flp};
    }

    string name() സ്ഥിരം VL_MT_SAFE {
        തിരികെ "FSM "
               + (m_stateVarName.empty() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dumpTag(size_t index) സ്ഥിരം {
        string tag = stateVarInternalName();
        വേണ്ടി (അക്ഷരം& ch : tag) {
            എങ്കിൽ (!std::isalnum(static_cast<unsigned അക്ഷരം>(ch))) ch = '_';
        }
        തിരികെ "fsm_" + cvtToStr(index) + "_" + tag;
    }
};

ഘടന DetectedFsm final {
    std::unique_ptr<FsmGraph> graphp;  // Extracted graph for one detected FSM candidate.
};

ഘടന FsmCaseCandidate final {
    Ast_കണ്ണി* warnNodep = nullptr;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = nullptr;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
ഘടന FsmStateComparison final {
    AstVarScope* stateVscp = nullptr;  // Compared state variable
    Ast_കണ്ണി_വ്യഞ്ജകം* valuep = nullptr;  // Compared constant value expression
    Fsm_State_മൂല്യം മൂല്യം;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
ഘടന FsmIfBranch final {
    AstIf* ifp = nullptr;  // Source if/else-if node
    Ast_കണ്ണി* stmtsp = nullptr;  // Branch body
    Ast_കണ്ണി_വ്യഞ്ജകം* valuep = nullptr;  // Source state value expression
    Fsm_State_മൂല്യം from_മൂല്യം;  // Encoded source state value
    തർക്കം has_മുകൾ_Guard = അസത്യം;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
ഘടന FsmIfChainCandidate final {
    AstIf* ifp = nullptr;  // Top-level if-chain node
    AstVarScope* compareVscp = nullptr;  // Variable used by every state comparison
    std::vector<FsmIfBranch> branches;  // Recognized state-dispatch branches
    Ast_കണ്ണി* defaultStmtsp = nullptr;  // Optional final else body
};

// Aliases are accepted only when they are equivalent to spelling the state
// comparison inline; this avoids inferring FSM semantics from arbitrary logic.
using FsmAliasMap = std::unordered_map<സ്ഥിരം AstVarScope*, FsmStateComparison>;
using FsmCellPortMap = std::unordered_map<string, AstVarScope*>;
using FsmCellPortAliasMap = std::unordered_map<സ്ഥിരം AstCell*, FsmCellPortMap>;

ഘടന StateConstLabel final {
    string text;
    തർക്കം fromParam = അസത്യം;
    size_t stateIndex = 0;
};

ഘടന FsmStateSpace final {
    std::vector<std::pair<string, Fsm_State_മൂല്യം>> states;  // User label and encoded value
    std::unordered_map<Fsm_State_മൂല്യം, StateConstLabel, Fsm_State_മൂല്യം_Hash>
        labels;  // Encoded value to label
    AstVar* stateVarp = nullptr;  // Tracked FSM state variable
    തർക്കം enumBacked = അസത്യം;  // Whether states came from an enum declaration
};

// Local shared state between the two adjacent FSM coverage phases. Detection
// fills this with recovered FSM graphs; lowering consumes the completed graphs
// immediately afterward without needing any AST serialization bridge.
വർഗ്ഗം FsmState final {
    // All detected FSMs in discovery order. This is the only bridge between
    // the adjacent detect and lower phases, so the second phase never needs to
    // rediscover or serialize the extracted machine.
    std::vector<DetectedFsm> m_fsms;
    std::map<സ്ഥിരം AstVarScope*, size_t> m_fsmIndex;

പൊതു:
    DetectedFsm& fsmFor(AstVarScope* stateVscp) {
        സ്ഥിരം std::map<സ്ഥിരം AstVarScope*, size_t>::const_iterator it = m_fsmIndex.find(stateVscp);
        എങ്കിൽ (it != m_fsmIndex.end()) തിരികെ m_fsms.at(it->second);
        സ്ഥിരം size_t index = m_fsms.size();
        m_fsmIndex.emplace(stateVscp, index);
        m_fsms.emplace_back();
        തിരികെ m_fsms.back();
    }
    സ്ഥിരം std::vector<DetectedFsm>& fsms() സ്ഥിരം { തിരികെ m_fsms; }
};

// Detection runs while the original clocked/case structure is still intact and
// populates graph-backed FSM models without mutating the tree mid-traversal.
// This pass is intentionally conservative: for this PR we only lock down the
// small set of transition/selector forms that are already stable in the
// normalized AST we see here. The remaining reject branches are therefore
// mostly future-feature boundaries, not accidental dead code.
വർഗ്ഗം FsmDetectVisitor final : പൊതു VNVisitor {
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
    std::unordered_map<സ്ഥിരം AstVarScope*, FsmCaseCandidate> m_comboPaired;
    // Continuous aliases are order-independent, while procedural aliases must
    // remain source-order scoped to avoid using assignments not yet executed.
    FsmAliasMap m_stateAliases;
    std::unordered_set<സ്ഥിരം AstVarScope*> m_ambiguousStateAliases;
    // A surviving wrapper's semantic d/q relationship is split across the
    // parent scope and the child module scope. This table is the narrow bridge
    // between those scopes: only transparent port aliases are recorded, so the
    // detector does not become a general cross-module dataflow engine.
    FsmCellPortAliasMap m_cellPortAliases;
    FsmCellPortAliasMap m_cellPortChildAliases;

    // METHODS
    // Enum-backed FSMs may be wrapped in refs/typedefs; normalize to the
    // underlying enum type before deciding whether a case is a candidate.
    സ്ഥിതം Ast_കണ്ണി_DType* unwrapEnumCandidate(Ast_കണ്ണി_DType* dtypep) {
        തിരികെ dtypep->skipRefToEnump();
    }

    സ്ഥിതം string candidateConflictContext(Ast_കണ്ണി* laterNodep,
                                           സ്ഥിരം FsmCaseCandidate& firstCand) {
        തിരികെ '\n' + laterNodep->warnContextPrimary() + firstCand.warnNodep->warnOther()
               + "... Location of first supported candidate for "
               + firstCand.stateVscp->prettyNameQ() + '\n'
               + firstCand.warnNodep->warnContextSecondary();
    }

    സ്ഥിതം തർക്കം rejectFsmWrapperCell(AstCell* cellp, സ്ഥിരം string& reason) {
        cellp->v3warn(COVERIGN, "Ignoring unsupported: " + reason);
        തിരികെ അസത്യം;
    }

    സ്ഥിതം തർക്കം simple_Param_State_മൂല്യം(AstCell* cellp, സ്ഥിരം string& name, Fsm_State_മൂല്യം& മൂല്യം,
                                      Ast_കണ്ണി_വ്യഞ്ജകം*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = nullptr;
        വേണ്ടി (Ast_കണ്ണി* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* സ്ഥിരം varp = VN_CAST(stmtp, Var);
            എങ്കിൽ (!varp || !varp->isParam() || varp->name() != name) തുടരുക;
            valuepr = VN_AS(varp->valuep(), കണ്ണി_വ്യഞ്ജകം);
            തിരികെ const_മൂല്യം_Status(valuepr, മൂല്യം) == Const_മൂല്യം_Status::OK;
        }
        തിരികെ അസത്യം;
    }

    സ്ഥിതം തർക്കം childPortInScope(AstVarScope* vscp, AstScope* parentScopep, AstCell*& cellpr) {
        എങ്കിൽ (!vscp->varp()->isIO()) തിരികെ അസത്യം;
        AstScope* സ്ഥിരം scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        എങ്കിൽ (scopep->aboveScopep() != parentScopep) തിരികെ അസത്യം;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        തിരികെ സത്യം;
    }

    സ്ഥിതം AstVarScope* simpleAssignVarScope(Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        AstVarRef* സ്ഥിരം vrefp = VN_CAST(exprp, VarRef);
        തിരികെ vrefp ? vrefp->varScopep() : nullptr;
    }

    ശൂന്യം addWrapperCell(AstScope* scopep, AstCell* cellp) {
        m_cellPortAliases.emplace(cellp, FsmCellPortMap{});
        m_cellPortChildAliases.emplace(cellp, FsmCellPortMap{});
        സ്ഥിരം std::pair<AstScope*, AstCell*> item{scopep, cellp};
        എങ്കിൽ (std::find(m_wrapperCells.cbegin(), m_wrapperCells.cend(), item)
            != m_wrapperCells.cend()) {
            തിരികെ;
        }
        m_wrapperCells.emplace_back(item);
    }

    ശൂന്യം collectCellPortAlias(AstAssignW* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* സ്ഥിരം lhsVscp = simpleAssignVarScope(nodep->lhsp());
        AstVarScope* സ്ഥിരം rhsVscp = simpleAssignVarScope(nodep->rhsp());
        എങ്കിൽ (!lhsVscp || !rhsVscp) തിരികെ;
        AstCell* cellp = nullptr;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        എങ്കിൽ (childPortInScope(lhsVscp, m_scopep, cellp)) {
            എങ്കിൽ (!fsmRegisterWrapperDesc(cellp)) തിരികെ;
            UASSERT_OBJ(lhsVscp->varp()->is_നിവേശം(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cellPortAliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cellPortChildAliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            addWrapperCell(m_scopep, cellp);
        } അല്ലെങ്കിൽ എങ്കിൽ (childPortInScope(rhsVscp, m_scopep, cellp)) {
            എങ്കിൽ (!fsmRegisterWrapperDesc(cellp)) തിരികെ;
            UASSERT_OBJ(rhsVscp->varp()->isWritable(), nodep,
                        "Child-side port alias rhs should be writable");
            m_cellPortAliases[cellp][rhsVscp->varp()->name()] = lhsVscp;
            m_cellPortChildAliases[cellp][rhsVscp->varp()->name()] = rhsVscp;
            addWrapperCell(m_scopep, cellp);
        }
    }

    AstVarScope* roleVarScope(AstCell* cellp, സ്ഥിരം string& portName) സ്ഥിരം {
        // At this point explicit AstPin expressions have been lowered away, so
        // role resolution crosses the wrapper boundary only through the
        // transparent alias table above. This keeps wrapper support aligned with
        // direct-register detection instead of growing into interprocedural FSM
        // inference.
        സ്ഥിരം FsmCellPortMap& ports = m_cellPortAliases.at(cellp);
        സ്ഥിരം FsmCellPortMap::const_iterator portIt = ports.find(portName);
        തിരികെ portIt == ports.end() ? nullptr : portIt->second;
    }

    AstVarScope* childRoleVarScope(AstCell* cellp, സ്ഥിരം string& portName) സ്ഥിരം {
        സ്ഥിരം FsmCellPortMap& ports = m_cellPortChildAliases.at(cellp);
        സ്ഥിരം FsmCellPortMap::const_iterator portIt = ports.find(portName);
        തിരികെ portIt == ports.end() ? nullptr : portIt->second;
    }

    വർഗ്ഗം RegisterAlwaysAnalyzer final {
        AstScope* സ്ഥിരം m_scopep;  // Scope containing the always block(s) being analyzed

    പൊതു:
        explicit RegisterAlwaysAnalyzer(AstScope* scopep)
            : m_scopep{scopep} {}

        std::vector<std::pair<AstCase*, Ast_കണ്ണി_വ്യഞ്ജകം*>>
        oneBlockCandidates(AstAlways* alwaysp) സ്ഥിരം {
            std::vector<std::pair<AstCase*, Ast_കണ്ണി_വ്യഞ്ജകം*>> candidates;
            Ast_കണ്ണി* സ്ഥിരം stmtsp = alwaysp->stmtsp();
            എങ്കിൽ (AstIf* സ്ഥിരം firstIfp = VN_CAST(stmtsp, If)) {
                എങ്കിൽ (AstCase* സ്ഥിരം casep = VN_CAST(firstIfp->elsesp(), Case)) {
                    candidates.emplace_back(casep,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            വേണ്ടി (Ast_കണ്ണി* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                എങ്കിൽ (AstCase* സ്ഥിരം casep = VN_CAST(nodep, Case))
                    candidates.emplace_back(casep, nullptr);
            }
            തിരികെ candidates;
        }

        std::vector<std::pair<AstIf*, Ast_കണ്ണി_വ്യഞ്ജകം*>>
        oneBlockIfCandidates(AstAlways* alwaysp) സ്ഥിരം {
            std::vector<std::pair<AstIf*, Ast_കണ്ണി_വ്യഞ്ജകം*>> candidates;
            Ast_കണ്ണി* സ്ഥിരം stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            എങ്കിൽ (AstIf* സ്ഥിരം firstIfp = VN_CAST(stmtsp, If)) {
                എങ്കിൽ (AstIf* സ്ഥിരം chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(firstIfp->elsesp()), If)) {
                    candidates.emplace_back(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            വേണ്ടി (Ast_കണ്ണി* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                എങ്കിൽ (AstIf* സ്ഥിരം ifp = VN_CAST(nodep, If)) candidates.emplace_back(ifp, nullptr);
            }
            തിരികെ candidates;
        }

        തർക്കം matchRegisterCandidate(AstAlways* alwaysp, FsmRegisterCandidate& cand) സ്ഥിരം {
            തിരികെ FsmDetectVisitor::matchRegisterAlways(alwaysp, m_scopep, cand);
        }

        ശൂന്യം buildOneBlockCandidate(AstAlways* alwaysp, AstVarScope* vscp, Ast_കണ്ണി_വ്യഞ്ജകം* resetCondp,
                                    FsmRegisterCandidate& reg) സ്ഥിരം {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.nextVscp(vscp);
            reg.senses() = FsmDetectVisitor::describe_Sen_വൃക്ഷം(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != nullptr);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* സ്ഥിരം firstIfp = VN_CAST(alwaysp->stmtsp(), If);
            എങ്കിൽ (firstIfp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = nullptr;
                സ്ഥിരം ResetAssignStatus resetStatus = FsmDetectVisitor::collectConstStateAssigns(
                    firstIfp->thensp(), resetStateVscp, reg.resetArcs());
                എങ്കിൽ (resetStatus == ResetAssignStatus::EMPTY) {
                    reg.resetArcs().clear();
                } അല്ലെങ്കിൽ എങ്കിൽ (resetStatus == ResetAssignStatus::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().clear();
                    Fsm_State_മൂല്യം reset_മൂല്യം;
                    Ast_കണ്ണി* സ്ഥിരം thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(firstIfp->thensp());
                    UASSERT_OBJ(thenNodep, firstIfp,
                                "one-block reset fallback requires a non-empty reset branch");
                    Ast_കണ്ണി_Assign* സ്ഥിരം resetAssp = FsmDetectVisitor::direct_Const_State_Assign_കണ്ണി(
                        thenNodep, resetStateVscp, reset_മൂല്യം);
                    എങ്കിൽ (resetAssp && resetStateVscp == vscp) {
                        reg.resetArcs().emplace_back(reset_മൂല്യം, resetAssp);
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                    reg.resetArcs().clear();
                }
            }
        }
    };

    തർക്കം matchFsmWrapperCell(AstScope* scopep, AstCell* cellp, FsmRegisterCandidate& cand) സ്ഥിരം {
        FsmWrapperRoles roles = rolesFromDesc(*fsmRegisterWrapperDesc(cellp));

        AstVarScope* സ്ഥിരം nextVscp = roleVarScope(cellp, roles.dPort);
        AstVarScope* സ്ഥിരം stateVscp = roleVarScope(cellp, roles.qPort);
        എങ്കിൽ (!nextVscp || !stateVscp) {
            തിരികെ rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* സ്ഥിരം clkVscp = roleVarScope(cellp, roles.clkPort);
        എങ്കിൽ (!clkVscp) {
            തിരികെ rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper instance requires a simple clock connection");
        }

        FsmSenDesc clkSense;
        clkSense.edgeType = VEdgeType::ET_POSEDGE;
        clkSense.varScopep = clkVscp;
        cand.senses().തള്ളുക_back(clkSense);

        AstVarScope* resetVscp = nullptr;
        എങ്കിൽ (!roles.rstPort.empty()) resetVscp = roleVarScope(cellp, roles.rstPort);
        എങ്കിൽ (resetVscp) {
            // The descriptor identifies the reset port but not its polarity. Use
            // the wrapper's own event control AST as the contract for sampling
            // the connected parent signal.
            തർക്കം inferred_Active_താഴെയുള്ള = അസത്യം;
            എങ്കിൽ (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rstPort, inferred_Active_താഴെയുള്ള)) {
                roles.has_Rst_Active_താഴെയുള്ള = സത്യം;
                roles.rst_Active_താഴെയുള്ള = inferred_Active_താഴെയുള്ള;
            }
        }

        Ast_കണ്ണി_വ്യഞ്ജകം* resetValuep = nullptr;
        Fsm_State_മൂല്യം reset_മൂല്യം;
        സ്ഥിരം തർക്കം has_Reset_മൂല്യം
            = !roles.rstValParam.empty()
              && simple_Param_State_മൂല്യം(cellp, roles.rstValParam, reset_മൂല്യം, resetValuep);
        എങ്കിൽ (resetVscp && roles.has_Rst_Active_താഴെയുള്ള && has_Reset_മൂല്യം) {
            FsmSenDesc rstSense;
            rstSense.edgeType = roles.rst_Active_താഴെയുള്ള ? VEdgeType::ET_NEGEDGE : VEdgeType::ET_POSEDGE;
            rstSense.varScopep = resetVscp;
            cand.senses().തള്ളുക_back(rstSense);
            cand.resetCond().varScopep = resetVscp;
            cand.resetCond().active_താഴെയുള്ള = roles.rst_Active_താഴെയുള്ള;
            cand.hasResetCond(സത്യം);
            cand.resetArcs().emplace_back(reset_മൂല്യം, cellp, resetValuep);
        } അല്ലെങ്കിൽ എങ്കിൽ (!roles.rstPort.empty() || !roles.rstValParam.empty()) {
            string reason;
            എങ്കിൽ (roles.rstPort.empty()) {
                reason = "reset port is not configured";
            } അല്ലെങ്കിൽ എങ്കിൽ (!resetVscp) {
                reason = "reset connection is missing or not a simple variable";
            } അല്ലെങ്കിൽ എങ്കിൽ (!roles.has_Rst_Active_താഴെയുള്ള) {
                reason = "reset polarity could not be inferred from the wrapper";
            } അല്ലെങ്കിൽ എങ്കിൽ (roles.rstValParam.empty()) {
                reason = "reset_value parameter is not configured";
            } അല്ലെങ്കിൽ {
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
        തിരികെ സത്യം;
    }

    വർഗ്ഗം ComboAlwaysAnalyzer final {
    പൊതു:
        ഘടന ComboMatch final {
            സ്ഥിരം FsmRegisterCandidate* matchedp = nullptr;
            Ast_കണ്ണി* warnNodep = nullptr;
        };

    സ്വകാര്യ:
        സ്ഥിരം std::vector<FsmRegisterCandidate>& m_registerCandidates;

    പൊതു:
        explicit ComboAlwaysAnalyzer(സ്ഥിരം std::vector<FsmRegisterCandidate>& registerCandidates)
            : m_registerCandidates{registerCandidates} {}

        ComboMatch matchCase(Ast_കണ്ണി* stmtsp, AstCase* casep) സ്ഥിരം {
            ComboMatch match;
            AstVarRef* സ്ഥിരം selp = VN_CAST(casep->exprp(), VarRef);
            എങ്കിൽ (!selp) തിരികെ match;
            വേണ്ടി (സ്ഥിരം FsmRegisterCandidate& reg : m_registerCandidates) {
                എങ്കിൽ (selp->varScopep() == reg.nextVscp()) {
                    എങ്കിൽ (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        തുടരുക;
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (selp->varScopep() != reg.stateVscp()) {
                    തുടരുക;
                }
                Ast_കണ്ണി* സ്ഥിരം warnNodep = FsmDetectVisitor::case_Supported_Transition_കണ്ണി(
                    casep, reg.nextVscp(), reg.inclCond());
                എങ്കിൽ (!warnNodep) തുടരുക;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            തിരികെ match;
        }

        ComboMatch matchIfChain(Ast_കണ്ണി* stmtsp, സ്ഥിരം FsmIfChainCandidate& chain) സ്ഥിരം {
            ComboMatch match;
            വേണ്ടി (സ്ഥിരം FsmRegisterCandidate& reg : m_registerCandidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                എങ്കിൽ (chain.compareVscp == reg.nextVscp()) {
                    എങ്കിൽ (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.nextVscp())) {
                        തുടരുക;
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (chain.compareVscp != reg.stateVscp()) {
                    തുടരുക;
                }
                Ast_കണ്ണി* സ്ഥിരം warnNodep
                    = FsmDetectVisitor::if_Chain_Supported_Transition_കണ്ണി(chain, reg.nextVscp());
                എങ്കിൽ (!warnNodep) തുടരുക;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            തിരികെ match;
        }

        തർക്കം shouldWarnUnsupported(Ast_കണ്ണി* stmtsp, AstCase* casep) സ്ഥിരം {
            സ്ഥിരം AstVarRef* സ്ഥിരം selp = VN_CAST(casep->exprp(), VarRef);
            എങ്കിൽ (!selp) തിരികെ അസത്യം;

            തിരികെ std::any_of(
                m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                [&](സ്ഥിരം FsmRegisterCandidate& reg) -> തർക്കം {
                    സ്ഥിരം തർക്കം matchesNext = selp->varScopep() == reg.nextVscp();
                    സ്ഥിരം തർക്കം matchesState = selp->varScopep() == reg.stateVscp();

                    എങ്കിൽ (!matchesNext && !matchesState) തിരികെ അസത്യം;
                    എങ്കിൽ (matchesNext
                        && !FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        തിരികെ അസത്യം;
                    }
                    തിരികെ FsmDetectVisitor::case_Supported_Transition_കണ്ണി(casep, reg.nextVscp(),
                                                                         reg.inclCond());
                });
        }
    };

    // Reset arcs are only modeled for the simple signal form that survives to
    // this pass after earlier normalization.
    സ്ഥിതം തർക്കം isSimpleResetCond(Ast_കണ്ണി_വ്യഞ്ജകം* condp) { തിരികെ VN_IS(condp, VarRef); }

    // Normalize the reset condition into a compact description so the lowering
    // phase can regenerate the same predicate after detection. By the time
    // this pass runs, active-low source forms such as "!rst_n" have already
    // been canonicalized to a positive-condition if/else shape, so only a
    // plain VarRef survives here.
    സ്ഥിതം FsmResetCondDesc describeResetCond(Ast_കണ്ണി_വ്യഞ്ജകം* condp) {
        FsmResetCondDesc desc;
        എങ്കിൽ (AstVarRef* സ്ഥിരം vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        തിരികെ desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    സ്ഥിതം std::vector<FsmSenDesc> describe_Sen_വൃക്ഷം(Ast_Sen_വൃക്ഷം* sentreep) {
        std::vector<FsmSenDesc> senses;
        വേണ്ടി (AstSenItem* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), SenItem)) {
            Ast_കണ്ണി_Var_Ref* സ്ഥിരം vrefp = itemp->varrefp();
            എങ്കിൽ (!vrefp) തുടരുക;
            FsmSenDesc desc;
            desc.edgeType = itemp->edgeType().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.തള്ളുക_back(desc);
        }
        തിരികെ senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    സ്ഥിതം തർക്കം isIgnorableStmt(Ast_കണ്ണി* nodep) { തിരികെ VN_IS(nodep, CoverInc); }

    സ്ഥിതം Ast_കണ്ണി* skipLeadingIgnorableStmt(Ast_കണ്ണി* nodep) {
        ആകുമ്പോൾ (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        തിരികെ nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    സ്ഥിതം Ast_കണ്ണി* singleMeaningfulStmt(Ast_കണ്ണി* stmtp) {
        Ast_കണ്ണി* resultp = nullptr;
        വേണ്ടി (Ast_കണ്ണി* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            എങ്കിൽ (isIgnorableStmt(nodep)) തുടരുക;
            എങ്കിൽ (resultp) തിരികെ nullptr;
            resultp = nodep;
        }
        തിരികെ resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    സ്ഥിതം Ast_കണ്ണി* singleMeaningfulBranch(Ast_കണ്ണി* branchp) {
        എങ്കിൽ (!branchp) തിരികെ nullptr;
        തിരികെ branchp;
    }

    // By fsm-detect time, non-clocked always @* blocks are already admitted through
    // a missing sentree. This helper therefore only needs to recognize
    // explicit changed-sensitivity lists such as always @(a or b); clocked and
    // event-driven forms remain out of scope.
    സ്ഥിതം തർക്കം isPlainComboSentree(സ്ഥിരം Ast_Sen_വൃക്ഷം* sentreep) {
        UASSERT(sentreep, "plain combo sensitivity check requires a sensitivity tree");
        വേണ്ടി (സ്ഥിരം AstSenItem* senp = sentreep->sensesp(); senp;
             senp = VN_AS(senp->nextp(), SenItem)) {
            എങ്കിൽ (senp->edgeType() == VEdgeType::ET_CHANGED) തുടരുക;
            തിരികെ അസത്യം;
        }
        തിരികെ സത്യം;
    }

    ശൂന്യം warnUnsupportedComboAlways(സ്ഥിരം FsmComboAlways& combo) {
        സ്ഥിരം ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_കണ്ണി* സ്ഥിരം stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        തർക്കം warned = അസത്യം;
        വേണ്ടി (Ast_കണ്ണി* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* സ്ഥിരം casep = VN_CAST(nodep, Case);
            എങ്കിൽ (!casep) തുടരുക;
            എങ്കിൽ (analyzer.shouldWarnUnsupported(stmtsp, casep)) {
                casep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-clocked always "
                                        "blocks requires a combinational sensitivity list or "
                                        "always_comb");
                warned = സത്യം;
            }
            എങ്കിൽ (warned) നിർത്തുക;
        }
    }

    // Case-item bodies are single subtrees like if/else arms, not statement
    // lists, so unwrap only local begin/end wrappers here rather than walking
    // sibling case items via nextp().
    സ്ഥിതം Ast_കണ്ണി_Assign* directStateAssign(Ast_കണ്ണി* stmtp, AstVarScope* stateVscp) {
        Ast_കണ്ണി* സ്ഥിരം nodep = singleMeaningfulBranch(stmtp);
        എങ്കിൽ (!nodep) തിരികെ nullptr;
        Ast_കണ്ണി_Assign* സ്ഥിരം assp = VN_CAST(nodep, കണ്ണി_Assign);
        എങ്കിൽ (!assp) തിരികെ nullptr;
        AstVarRef* സ്ഥിരം vrefp = VN_CAST(assp->lhsp(), VarRef);
        എങ്കിൽ (!vrefp || vrefp->varScopep() != stateVscp) തിരികെ nullptr;
        തിരികെ assp;
    }

    സ്ഥിതം Ast_കണ്ണി_Assign* കണ്ണി_State_Var_Assign(Ast_കണ്ണി* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        Ast_കണ്ണി_Assign* സ്ഥിരം assp = VN_CAST(nodep, കണ്ണി_Assign);
        എങ്കിൽ (!assp) തിരികെ nullptr;
        AstVarRef* സ്ഥിരം lhsp = VN_CAST(assp->lhsp()->baseFromp(സത്യം), VarRef);
        AstVarRef* സ്ഥിരം rhsp = VN_CAST(assp->rhsp(), VarRef);
        എങ്കിൽ (!rhsp || !lhsp) തിരികെ nullptr;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        തിരികെ assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    സ്ഥിതം Ast_കണ്ണി_Assign* directCondStateVarAssign(Ast_കണ്ണി* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, Ast_കണ്ണി_വ്യഞ്ജകം*& condp,
                                                   Fsm_State_മൂല്യം& reset_മൂല്യം) {
        Ast_കണ്ണി_Assign* സ്ഥിരം assp = VN_CAST(nodep, കണ്ണി_Assign);
        എങ്കിൽ (!assp) തിരികെ nullptr;
        AstVarRef* സ്ഥിരം lhsp = VN_CAST(assp->lhsp()->baseFromp(സത്യം), VarRef);
        AstCond* സ്ഥിരം rhsp = VN_CAST(assp->rhsp(), Cond);
        എങ്കിൽ (!rhsp || !lhsp) തിരികെ nullptr;
        AstVarRef* സ്ഥിരം elsep = VN_CAST(rhsp->elsep(), VarRef);
        എങ്കിൽ (!elsep) തിരികെ nullptr;
        എങ്കിൽ (const_മൂല്യം_Status(rhsp->thenp(), reset_മൂല്യം) != Const_മൂല്യം_Status::OK) തിരികെ nullptr;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        തിരികെ assp;
    }

    സ്ഥിതം Ast_കണ്ണി_Assign* direct_Const_State_Assign_കണ്ണി(Ast_കണ്ണി* nodep, AstVarScope*& stateVscp,
                                                     Fsm_State_മൂല്യം& മൂല്യം) {
        Ast_കണ്ണി_Assign* സ്ഥിരം assp = VN_CAST(nodep, കണ്ണി_Assign);
        എങ്കിൽ (!assp) തിരികെ nullptr;
        AstVarRef* സ്ഥിരം lhsp = VN_CAST(assp->lhsp()->baseFromp(സത്യം), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        എങ്കിൽ (const_മൂല്യം_Status(assp->rhsp(), മൂല്യം) != Const_മൂല്യം_Status::OK) തിരികെ nullptr;
        stateVscp = lhsp->varScopep();
        തിരികെ assp;
    }

    എണ്ണൽ വർഗ്ഗം ResetAssignStatus : uint8_t {
        EMPTY,  // Reset branch had no non-coverage statements.
        NONE,  // Reset branch was not the supported direct-constant shape.
        SINGLE,  // Exactly one supported reset assignment was collected.
        MULTI_SAME_STATE  // Multiple assignments to the same FSM state var; warn and ignore.
    };

    // Reset arcs are only extracted from the single direct-constant form. If
    // user RTL assigns the same state register multiple times in the reset
    // branch, warn and skip reset-arc modeling rather than inventing multiple
    // reset transitions for an odd but legal coding style.
    സ്ഥിതം ResetAssignStatus collectConstStateAssigns(Ast_കണ്ണി* stmtp, AstVarScope*& stateVscp,
                                                      std::vector<FsmResetArcDesc>& resetArcs) {
        Ast_കണ്ണി* nodep = skipLeadingIgnorableStmt(stmtp);
        എങ്കിൽ (!nodep) തിരികെ ResetAssignStatus::EMPTY;
        വേണ്ടി (;; nodep = nodep->nextp()) {
            AstVarScope* assignStateVscp = nullptr;
            Fsm_State_മൂല്യം മൂല്യം;
            Ast_കണ്ണി_Assign* സ്ഥിരം assp = direct_Const_State_Assign_കണ്ണി(nodep, assignStateVscp, മൂല്യം);
            എങ്കിൽ (!assp) തിരികെ ResetAssignStatus::NONE;
            എങ്കിൽ (!stateVscp) stateVscp = assignStateVscp;
            എങ്കിൽ (assignStateVscp != stateVscp) തിരികെ ResetAssignStatus::NONE;
            എങ്കിൽ (!resetArcs.empty()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.clear();
                തിരികെ ResetAssignStatus::MULTI_SAME_STATE;
            }
            resetArcs.emplace_back(മൂല്യം, assp);
            എങ്കിൽ (!nodep->nextp()) തിരികെ ResetAssignStatus::SINGLE;
        }
    }

    സ്ഥിതം തർക്കം hasCanonicalNextStateDefaultBeforeCase(Ast_കണ്ണി* stmtsp, Ast_കണ്ണി* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* nextVscp) {
        Ast_കണ്ണി* സ്ഥിരം bodyp = skipLeadingIgnorableStmt(stmtsp);
        തർക്കം sawCanonicalDefault = അസത്യം;
        വേണ്ടി (Ast_കണ്ണി* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            എങ്കിൽ (nodep == targetp) തിരികെ sawCanonicalDefault;
            എങ്കിൽ (Ast_കണ്ണി_Assign* സ്ഥിരം assp = VN_CAST(nodep, കണ്ണി_Assign)) {
                AstVarRef* സ്ഥിരം lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* സ്ഥിരം rhsp = VN_CAST(assp->rhsp(), VarRef);
                എങ്കിൽ (!lhsp || lhsp->varScopep() != nextVscp) തുടരുക;
                എങ്കിൽ (sawCanonicalDefault) {
                    സ്ഥിരം string nextName = nextVscp->varp()->prettyNameQ();
                    സ്ഥിരം string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + nextName
                                     + ") when the canonical " + nextName + " = " + stateName
                                     + " default is overwritten before the case statement");
                    തിരികെ അസത്യം;
                }
                എങ്കിൽ (!rhsp || rhsp->varScopep() != stateVscp) തിരികെ അസത്യം;
                sawCanonicalDefault = സത്യം;
            }
        }
    }

    സ്ഥിതം തർക്കം ifStateConstAssign(Ast_കണ്ണി* stmtp, AstVarScope* stateVscp,
                                   Fsm_State_മൂല്യം& then_മൂല്യം, Fsm_State_മൂല്യം& else_മൂല്യം) {
        AstIf* സ്ഥിരം ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        എങ്കിൽ (!ifp || !ifp->elsesp()) തിരികെ അസത്യം;
        AstVarScope* thenVscp = nullptr;
        AstVarScope* elseVscp = nullptr;
        Ast_കണ്ണി* സ്ഥിരം thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        Ast_കണ്ണി* സ്ഥിരം elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        എങ്കിൽ (!thenNodep || !elseNodep) തിരികെ അസത്യം;
        എങ്കിൽ (!direct_Const_State_Assign_കണ്ണി(thenNodep, thenVscp, then_മൂല്യം)) തിരികെ അസത്യം;
        എങ്കിൽ (!direct_Const_State_Assign_കണ്ണി(elseNodep, elseVscp, else_മൂല്യം)) തിരികെ അസത്യം;
        എങ്കിൽ (thenVscp == stateVscp && elseVscp == stateVscp) തിരികെ സത്യം;
        എങ്കിൽ (thenVscp != elseVscp) തിരികെ അസത്യം;
        Ast_കണ്ണി* സ്ഥിരം followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = nullptr;
        AstVarScope* finalFromVscp = nullptr;
        Ast_കണ്ണി* സ്ഥിരം finalNodep = singleMeaningfulBranch(followp);
        എങ്കിൽ (!finalNodep) തിരികെ അസത്യം;
        എങ്കിൽ (!കണ്ണി_State_Var_Assign(finalNodep, finalStateVscp, finalFromVscp)) തിരികെ അസത്യം;
        എങ്കിൽ (finalStateVscp != stateVscp) തിരികെ അസത്യം;
        എങ്കിൽ (finalFromVscp != thenVscp) തിരികെ അസത്യം;
        തിരികെ സത്യം;
    }

    സ്ഥിതം തർക്കം directStateCondConstAssign(Ast_കണ്ണി* stmtp, AstVarScope* stateVscp,
                                           Fsm_State_മൂല്യം& then_മൂല്യം, Fsm_State_മൂല്യം& else_മൂല്യം) {
        Ast_കണ്ണി_Assign* സ്ഥിരം assp = directStateAssign(stmtp, stateVscp);
        എങ്കിൽ (!assp) തിരികെ അസത്യം;
        AstCond* സ്ഥിരം condp = VN_CAST(assp->rhsp(), Cond);
        എങ്കിൽ (!condp) തിരികെ അസത്യം;
        തിരികെ const_മൂല്യം_Status(condp->thenp(), then_മൂല്യം) == Const_മൂല്യം_Status::OK
               && const_മൂല്യം_Status(condp->elsep(), else_മൂല്യം) == Const_മൂല്യം_Status::OK;
    }

    സ്ഥിതം Ast_കണ്ണി* case_Item_Supported_Arc_കണ്ണി(AstCaseItem* itemp, AstVarScope* stateVscp,
                                             തർക്കം inclCond) {
        എങ്കിൽ (itemp->isDefault()) {
            എങ്കിൽ (!inclCond) തിരികെ nullptr;
        }
        Ast_കണ്ണി_Assign* സ്ഥിരം assp = directStateAssign(itemp->stmtsp(), stateVscp);
        എങ്കിൽ (assp) {
            Fsm_State_മൂല്യം to_മൂല്യം;
            എങ്കിൽ (const_മൂല്യം_Status(assp->rhsp(), to_മൂല്യം) == Const_മൂല്യം_Status::OK) തിരികെ assp;
        }
        Fsm_State_മൂല്യം then_മൂല്യം;
        Fsm_State_മൂല്യം else_മൂല്യം;
        എങ്കിൽ (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_മൂല്യം, else_മൂല്യം)) {
            തിരികെ assp;
        }
        എങ്കിൽ (ifStateConstAssign(itemp->stmtsp(), stateVscp, then_മൂല്യം, else_മൂല്യം)) {
            തിരികെ singleMeaningfulBranch(itemp->stmtsp());
        }
        തിരികെ nullptr;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    സ്ഥിതം Ast_കണ്ണി* case_Supported_Transition_കണ്ണി(AstCase* casep, AstVarScope* stateVscp,
                                                തർക്കം inclCond) {
        വേണ്ടി (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം nodep = case_Item_Supported_Arc_കണ്ണി(itemp, stateVscp, inclCond))
                തിരികെ nodep;
        }
        തിരികെ nullptr;
    }

    സ്ഥിതം Ast_കണ്ണി* case_Item_Supported_Arc_കണ്ണി_Like(Ast_കണ്ണി* stmtsp, AstVarScope* stateVscp) {
        എങ്കിൽ (Ast_കണ്ണി_Assign* സ്ഥിരം assp = directStateAssign(stmtsp, stateVscp)) {
            Fsm_State_മൂല്യം to_മൂല്യം;
            എങ്കിൽ (const_മൂല്യം_Status(assp->rhsp(), to_മൂല്യം) == Const_മൂല്യം_Status::OK) തിരികെ assp;
            Fsm_State_മൂല്യം then_മൂല്യം;
            Fsm_State_മൂല്യം else_മൂല്യം;
            എങ്കിൽ (directStateCondConstAssign(stmtsp, stateVscp, then_മൂല്യം, else_മൂല്യം)) തിരികെ assp;
        }
        തിരികെ nullptr;
    }

    സ്ഥിതം Ast_കണ്ണി* if_Chain_Supported_Transition_കണ്ണി(സ്ഥിരം FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        വേണ്ടി (size_t i = 0; i < chain.branches.size(); ++i) {
            സ്ഥിരം FsmIfBranch& branch = chain.branches[i];
            Ast_കണ്ണി* സ്ഥിരം nodep = case_Item_Supported_Arc_കണ്ണി_Like(branch.stmtsp, stateVscp);
            എങ്കിൽ (!nodep) തിരികെ nullptr;
        }
        തിരികെ chain.branches.front().ifp;
    }

    // Prefer user labels in reports. Forced non-enum FSMs prepopulate synthetic
    // labels, so all emitted arcs should already have a known label here.
    സ്ഥിതം string label_For_മൂല്യം(
        സ്ഥിരം std::unordered_map<Fsm_State_മൂല്യം, StateConstLabel, Fsm_State_മൂല്യം_Hash>& labels,
        സ്ഥിരം Fsm_State_മൂല്യം& മൂല്യം) {
        തിരികെ labels.at(മൂല്യം).text;
    }

    // The extractor only models constant-valued state transitions, and by the
    // time detect runs those values have already been constant-folded.
    എണ്ണൽ വർഗ്ഗം Const_മൂല്യം_Status : uint8_t { OK, NOT_CONST, XZ };

    സ്ഥിതം Const_മൂല്യം_Status const_മൂല്യം_Status(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Fsm_State_മൂല്യം& മൂല്യം) {
        സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(exprp, Const);
        എങ്കിൽ (!constp) തിരികെ Const_മൂല്യം_Status::NOT_CONST;
        സ്ഥിരം V3_സംഖ്യ& അക്കം = constp->അക്കം();
        എങ്കിൽ (അക്കം.isAnyXZ()) തിരികെ Const_മൂല്യം_Status::XZ;
        മൂല്യം = Fsm_State_മൂല്യം{അക്കം};
        തിരികെ Const_മൂല്യം_Status::OK;
    }

    സ്ഥിതം തർക്കം pureStateComparisonNoAlias(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, FsmStateComparison& cmp) {
        AstEq* സ്ഥിരം eqp = VN_CAST(exprp, Eq);
        എങ്കിൽ (!eqp) തിരികെ അസത്യം;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        Ast_കണ്ണി_വ്യഞ്ജകം* valuep = eqp->rhsp();
        എങ്കിൽ (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->baseFromp(സത്യം), VarRef);
            എങ്കിൽ (!vrefp) { തിരികെ അസത്യം; }
            valuep = eqp->lhsp();
        }

        Fsm_State_മൂല്യം മൂല്യം;
        എങ്കിൽ (const_മൂല്യം_Status(valuep, മൂല്യം) != Const_മൂല്യം_Status::OK) തിരികെ അസത്യം;
        cmp.stateVscp = vrefp->varScopep();
        cmp.valuep = valuep;
        cmp.മൂല്യം = മൂല്യം;
        തിരികെ സത്യം;
    }

    സ്ഥിതം തർക്കം pureStateComparison(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, സ്ഥിരം FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        എങ്കിൽ (pureStateComparisonNoAlias(exprp, cmp)) തിരികെ സത്യം;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        എങ്കിൽ (AstVarRef* സ്ഥിരം vrefp = VN_CAST(exprp, VarRef)) {
            സ്ഥിരം FsmAliasMap::const_iterator it = aliases.find(vrefp->varScopep());
            എങ്കിൽ (it == aliases.end()) തിരികെ അസത്യം;
            cmp = it->second;
            തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }

    സ്ഥിതം തർക്കം supported_മുകൾ_Level_Guard(Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        എങ്കിൽ (VN_IS(exprp, Or)) തിരികെ അസത്യം;
        എങ്കിൽ (VN_IS(exprp, RedAnd)) തിരികെ അസത്യം;
        എങ്കിൽ (VN_IS(exprp, RedOr)) തിരികെ അസത്യം;
        എങ്കിൽ (VN_IS(exprp, RedXor)) തിരികെ അസത്യം;
        തിരികെ സത്യം;
    }

    സ്ഥിതം തർക്കം resolveIfPredicate(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, സ്ഥിരം FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, തർക്കം& hasGuard) {
        std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> terms;
        std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> pending;
        pending.തള്ളുക_back(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        ആകുമ്പോൾ (!pending.empty()) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം nodep = pending.back();
            pending.എടുക്കുക_back();
            എങ്കിൽ (AstAnd* സ്ഥിരം andp = VN_CAST(nodep, And)) {
                pending.തള്ളുക_back(andp->rhsp());
                pending.തള്ളുക_back(andp->lhsp());
            } അല്ലെങ്കിൽ {
                terms.തള്ളുക_back(nodep);
            }
        }

        തർക്കം sawComparison = അസത്യം;
        വേണ്ടി (size_t i = 0; i < terms.size(); ++i) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം termp = terms[i];
            FsmStateComparison termCmp;
            എങ്കിൽ (pureStateComparison(termp, aliases, termCmp /*ref*/)) {
                എങ്കിൽ (sawComparison) തിരികെ അസത്യം;
                cmp = termCmp;
                sawComparison = സത്യം;
                തുടരുക;
            }
            എങ്കിൽ (!supported_മുകൾ_Level_Guard(termp)) തിരികെ അസത്യം;
            hasGuard = സത്യം;
        }
        തിരികെ sawComparison;
    }

    സ്ഥിതം ശൂന്യം addAlias(FsmAliasMap& aliases, std::unordered_set<സ്ഥിരം AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, സ്ഥിരം FsmStateComparison& cmp) {
        എങ്കിൽ (ambiguous.find(aliasVscp) != ambiguous.end()) തിരികെ;
        സ്ഥിരം FsmAliasMap::iterator it = aliases.find(aliasVscp);
        എങ്കിൽ (it == aliases.end()) {
            aliases.emplace(aliasVscp, cmp);
            തിരികെ;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        എങ്കിൽ (it->second.stateVscp == cmp.stateVscp && it->second.മൂല്യം == cmp.മൂല്യം) തിരികെ;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        തിരികെ;
    }

    സ്ഥിതം ശൂന്യം collectAliasFromAssign(Ast_കണ്ണി_Assign* assp, FsmAliasMap& aliases,
                                       std::unordered_set<സ്ഥിരം AstVarScope*>& ambiguous) {
        AstVarRef* സ്ഥിരം lhsp = VN_CAST(assp->lhsp(), VarRef);
        എങ്കിൽ (!lhsp) തിരികെ;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        എങ്കിൽ (!pureStateComparisonNoAlias(assp->rhsp(), cmp /*ref*/)) തിരികെ;
        addAlias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(Ast_കണ്ണി* stmtsp, Ast_കണ്ണി* limitp) സ്ഥിരം {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_set<സ്ഥിരം AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        വേണ്ടി (Ast_കണ്ണി* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            എങ്കിൽ (Ast_കണ്ണി_Assign* സ്ഥിരം assp = VN_CAST(nodep, കണ്ണി_Assign)) {
                collectAliasFromAssign(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        വേണ്ടി (സ്ഥിരം AstVarScope* സ്ഥിരം vscp : ambiguous) aliases.erase(vscp);
        തിരികെ aliases;
    }

    സ്ഥിതം തർക്കം collectIfChain(AstIf* ifp, സ്ഥിരം FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_set<string> seen_മൂല്യങ്ങൾ;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        ആകുമ്പോൾ (സത്യം) {
            FsmStateComparison cmp;
            തർക്കം hasGuard = അസത്യം;
            എങ്കിൽ (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) തിരികെ അസത്യം;
            എങ്കിൽ (chain.compareVscp && chain.compareVscp != cmp.stateVscp) തിരികെ അസത്യം;
            എങ്കിൽ (!seen_മൂല്യങ്ങൾ.തിരുകുക(cmp.മൂല്യം.താക്കോൽ()).second) തിരികെ അസത്യം;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.തള്ളുക_back(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.മൂല്യം, hasGuard});

            Ast_കണ്ണി* സ്ഥിരം elseNodep
                = singleMeaningfulBranch(skipLeadingIgnorableStmt(curp->elsesp()));
            എങ്കിൽ (!elseNodep) നിർത്തുക;
            എങ്കിൽ (AstIf* സ്ഥിരം elseIfp = VN_CAST(elseNodep, If)) {
                curp = elseIfp;
                തുടരുക;
            }
            chain.defaultStmtsp = elseNodep;
            നിർത്തുക;
        }
        തിരികെ chain.branches.size() >= 2;
    }

    // Enum-backed FSMs should only use values that were interned as known states.
    // If a constant transition references some other encoding, warn and skip FSM
    // instrumentation for that edge rather than silently dropping it or turning
    // optional coverage into a hard compile failure.
    സ്ഥിതം തർക്കം validate_Known_State_മൂല്യം(Ast_കണ്ണി* nodep, സ്ഥിരം FsmStateSpace& stateSpace,
                                        സ്ഥിരം Fsm_State_മൂല്യം& മൂല്യം, സ്ഥിരം string& role) {
        എങ്കിൽ (stateSpace.labels.find(മൂല്യം) != stateSpace.labels.end()) തിരികെ സത്യം;
        എങ്കിൽ (stateSpace.enumBacked) {
            സ്ഥിരം string enumRole = role == "source" ? "case item value" : "assigned value";
            nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on enum state variable "
                                        + stateSpace.stateVarp->prettyNameQ() + ": " + enumRole
                                        + " " + മൂല്യം.warnText()
                                        + " is not present in the declared enum");
            തിരികെ അസത്യം;
        }
        nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum state variable "
                                    + stateSpace.stateVarp->prettyNameQ() + ": " + role + " value "
                                    + മൂല്യം.warnText()
                                    + " is not present in the inferred state space");
        തിരികെ അസത്യം;
    }

    സ്ഥിതം StateConstLabel stateLabelForConst(AstConst* constp) {
        സ്ഥിരം string name = constp->origParamName();
        എങ്കിൽ (!name.empty()) തിരികെ StateConstLabel{Ast_കണ്ണി::prettyName(name), സത്യം, 0};
        തിരികെ StateConstLabel{constp->name(), അസത്യം, 0};
    }

    സ്ഥിതം ശൂന്യം updateStateLabel(FsmStateSpace& stateSpace, സ്ഥിരം Fsm_State_മൂല്യം& മൂല്യം,
                                 സ്ഥിരം StateConstLabel& label) {
        stateSpace.states.at(stateSpace.labels.at(മൂല്യം).stateIndex).first = label.text;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    സ്ഥിതം തർക്കം matchRegisterAlways(AstAlways* alwaysp, AstScope* scopep,
                                    FsmRegisterCandidate& cand) {
        എങ്കിൽ (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) തിരികെ അസത്യം;

        Ast_കണ്ണി* സ്ഥിരം stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        Ast_കണ്ണി* സ്ഥിരം nodep = singleMeaningfulStmt(stmtsp);
        എങ്കിൽ (!nodep) തിരികെ അസത്യം;

        AstVarScope* stateVscp = nullptr;
        AstVarScope* nextVscp = nullptr;
        എങ്കിൽ (AstIf* സ്ഥിരം ifp = VN_CAST(nodep, If)) {
            എങ്കിൽ (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) തിരികെ അസത്യം;
            AstVarScope* resetStateVscp = nullptr;
            സ്ഥിരം ResetAssignStatus resetStatus
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            സ്ഥിരം തർക്കം emptyResetBranch = resetStatus == ResetAssignStatus::EMPTY;
            എങ്കിൽ (emptyResetBranch) {
                cand.resetArcs().clear();
            } അല്ലെങ്കിൽ എങ്കിൽ (resetStatus == ResetAssignStatus::NONE) {
                cand.resetArcs().clear();
                Fsm_State_മൂല്യം reset_മൂല്യം;
                Ast_കണ്ണി* സ്ഥിരം thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                Ast_കണ്ണി_Assign* സ്ഥിരം resetAssp
                    = direct_Const_State_Assign_കണ്ണി(thenNodep, resetStateVscp, reset_മൂല്യം);
                എങ്കിൽ (!resetAssp) തിരികെ അസത്യം;
                cand.resetArcs().emplace_back(reset_മൂല്യം, resetAssp);
            } അല്ലെങ്കിൽ എങ്കിൽ (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                cand.resetArcs().clear();
            }
            Ast_കണ്ണി* സ്ഥിരം elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            എങ്കിൽ (!കണ്ണി_State_Var_Assign(elseNodep, stateVscp, nextVscp)) തിരികെ അസത്യം;
            എങ്കിൽ (!emptyResetBranch && resetStateVscp != stateVscp) തിരികെ അസത്യം;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != nullptr);
        } അല്ലെങ്കിൽ {
            Ast_കണ്ണി_വ്യഞ്ജകം* resetCondp = nullptr;
            Fsm_State_മൂല്യം reset_മൂല്യം;
            // Note the active low 'reset ? next : CONST' form is not accepted
            എങ്കിൽ (Ast_കണ്ണി_Assign* സ്ഥിരം assp
                = directCondStateVarAssign(nodep, stateVscp, nextVscp, resetCondp, reset_മൂല്യം)) {
                cand.resetArcs().emplace_back(reset_മൂല്യം, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != nullptr);
            } അല്ലെങ്കിൽ എങ്കിൽ (!കണ്ണി_State_Var_Assign(nodep, stateVscp, nextVscp)) {
                തിരികെ അസത്യം;
            }
        }
        cand.scopep(scopep);
        cand.alwaysp(alwaysp);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(stateVscp);
        cand.nextVscp(nextVscp);
        cand.senses() = describe_Sen_വൃക്ഷം(alwaysp->sentreep());
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(alwaysp->fileline());
        തിരികെ സത്യം;
    }

    സ്ഥിതം തർക്കം add_മൂല്യം_To_State_Space(Ast_കണ്ണി* nodep, FsmStateSpace& stateSpace,
                                     സ്ഥിരം Fsm_State_മൂല്യം& മൂല്യം, StateConstLabel label) {
        സ്ഥിരം auto labelIt = stateSpace.labels.find(മൂല്യം);
        എങ്കിൽ (labelIt != stateSpace.labels.end()) {
            StateConstLabel& existingLabel = labelIt->second;
            എങ്കിൽ (existingLabel.text != label.text && existingLabel.fromParam && label.fromParam) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                        "state variable "
                                            + stateSpace.stateVarp->prettyNameQ()
                                            + " with multiple labels for the same value "
                                            + മൂല്യം.warnText() + ": " + existingLabel.text
                                            + " and " + label.text);
                തിരികെ അസത്യം;
            }
            എങ്കിൽ (!existingLabel.fromParam && label.fromParam) {
                existingLabel.text = label.text;
                existingLabel.fromParam = label.fromParam;
                updateStateLabel(stateSpace, മൂല്യം, label);
            }
        } അല്ലെങ്കിൽ {
            StateConstLabel storedLabel = label;
            storedLabel.stateIndex = stateSpace.states.size();
            stateSpace.states.emplace_back(label.text, മൂല്യം);
            stateSpace.labels.emplace(മൂല്യം, storedLabel);
        }
        തിരികെ സത്യം;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    സ്ഥിതം തർക്കം add_വ്യഞ്ജകം_To_State_Space(Ast_കണ്ണി_വ്യഞ്ജകം* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_മൂല്യം മൂല്യം;
        സ്ഥിരം Const_മൂല്യം_Status status = const_മൂല്യം_Status(valuep, മൂല്യം);
        എങ്കിൽ (status != Const_മൂല്യം_Status::OK) {
            എങ്കിൽ (status == Const_മൂല്യം_Status::XZ) {
                valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                         "state variable "
                                             + stateSpace.stateVarp->prettyNameQ()
                                             + " with X/Z state encoding values");
            }
            തിരികെ അസത്യം;
        }
        AstConst* സ്ഥിരം constp = VN_AS(valuep, Const);
        തിരികെ add_മൂല്യം_To_State_Space(valuep, stateSpace, മൂല്യം, stateLabelForConst(constp));
    }

    സ്ഥിതം തർക്കം add_Optional_Target_വ്യഞ്ജകം_To_State_Space(Ast_കണ്ണി_വ്യഞ്ജകം* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_മൂല്യം മൂല്യം;
        സ്ഥിരം Const_മൂല്യം_Status status = const_മൂല്യം_Status(valuep, മൂല്യം);
        എങ്കിൽ (status != Const_മൂല്യം_Status::OK) {
            valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                     "state variable "
                                         + stateSpace.stateVarp->prettyNameQ()
                                         + " with non-constant target state values");
            തിരികെ അസത്യം;
        }
        AstConst* സ്ഥിരം constp = VN_AS(valuep, Const);
        തിരികെ add_മൂല്യം_To_State_Space(valuep, stateSpace, മൂല്യം, stateLabelForConst(constp));
    }

    സ്ഥിതം ശൂന്യം addResetTargetsToStateSpace(സ്ഥിരം std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        വേണ്ടി (സ്ഥിരം FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.to_മൂല്യം().ascii(), അസത്യം, 0};
            എങ്കിൽ (AstConst* സ്ഥിരം constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                add_മൂല്യം_To_State_Space(resetArc.nodep(), stateSpace, resetArc.to_മൂല്യം(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    സ്ഥിതം തർക്കം collectDeclaredStateSpace(Ast_കണ്ണി* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, തർക്കം& needs_Source_മൂല്യങ്ങൾ) {
        AstVar* സ്ഥിരം stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        എങ്കിൽ (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        സ്ഥിരം തർക്കം forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        എങ്കിൽ (enump) {
            stateSpace.enumBacked = സത്യം;
            വേണ്ടി (AstEnumItem* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                സ്ഥിരം AstConst* സ്ഥിരം constp = VN_AS(itemp->valuep(), Const);
                സ്ഥിരം Fsm_State_മൂല്യം മൂല്യം{constp->അക്കം()};
                സ്ഥിരം size_t stateIndex = stateSpace.states.size();
                stateSpace.states.emplace_back(itemp->name(), മൂല്യം);
                stateSpace.labels.emplace(മൂല്യം,
                                          StateConstLabel{itemp->name(), അസത്യം, stateIndex});
            }
            തിരികെ stateSpace.states.size() >= 2;
        }

        എങ്കിൽ (forced) {
            needs_Source_മൂല്യങ്ങൾ = സത്യം;
            തിരികെ സത്യം;
        }

        needs_Source_മൂല്യങ്ങൾ = സത്യം;
        തിരികെ സത്യം;
    }

    template <typename T_ValuepVisitor>
    സ്ഥിതം തർക്കം collect_State_Space_From_മൂല്യങ്ങൾ(Ast_കണ്ണി* warnNodep, AstVarScope* stateVscp,
                                            സ്ഥിരം std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            സ്ഥിരം T_ValuepVisitor& visitValueps) {
        തർക്കം needs_Source_മൂല്യങ്ങൾ = അസത്യം;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        എങ്കിൽ (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needs_Source_മൂല്യങ്ങൾ)) {
            തിരികെ അസത്യം;
        }
        എങ്കിൽ (!needs_Source_മൂല്യങ്ങൾ) തിരികെ സത്യം;
        addResetTargetsToStateSpace(resetArcs, stateSpace);
        എങ്കിൽ (!visitValueps(
                [&](Ast_കണ്ണി_വ്യഞ്ജകം* valuep) { തിരികെ add_വ്യഞ്ജകം_To_State_Space(valuep, stateSpace); })) {
            തിരികെ അസത്യം;
        }
        തിരികെ stateSpace.states.size() >= 2;
    }

    സ്ഥിതം തർക്കം collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* assignVscp,
                                  സ്ഥിരം std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        തിരികെ collect_State_Space_From_മൂല്യങ്ങൾ(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, assignVscp, &stateSpace](സ്ഥിരം auto& visitValuep) {
                വേണ്ടി (AstCaseItem* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), CaseItem)) {
                    എങ്കിൽ (!itemp->isDefault()) {
                        വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), കണ്ണി_വ്യഞ്ജകം)) {
                            എങ്കിൽ (!visitValuep(condp)) തിരികെ അസത്യം;
                        }
                    }
                    എങ്കിൽ (Ast_കണ്ണി_Assign* സ്ഥിരം assp
                        = directStateAssign(itemp->stmtsp(), assignVscp)) {
                        Fsm_State_മൂല്യം then_മൂല്യം;
                        Fsm_State_മൂല്യം else_മൂല്യം;
                        AstCond* സ്ഥിരം condp = VN_CAST(assp->rhsp(), Cond);
                        എങ്കിൽ (condp
                            && directStateCondConstAssign(itemp->stmtsp(), assignVscp, then_മൂല്യം,
                                                          else_മൂല്യം)) {
                            എങ്കിൽ (!visitValuep(condp->thenp())) തിരികെ അസത്യം;
                            എങ്കിൽ (!visitValuep(condp->elsep())) തിരികെ അസത്യം;
                        } അല്ലെങ്കിൽ എങ്കിൽ (!add_Optional_Target_വ്യഞ്ജകം_To_State_Space(assp->rhsp(), stateSpace)) {
                            തിരികെ അസത്യം;
                        }
                    }
                }
                തിരികെ സത്യം;
            });
    }

    സ്ഥിതം തർക്കം collectStateSpace(സ്ഥിരം FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* assignVscp,
                                  സ്ഥിരം std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        തിരികെ collect_State_Space_From_മൂല്യങ്ങൾ(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, assignVscp, &stateSpace](സ്ഥിരം auto& visitValuep) {
                വേണ്ടി (സ്ഥിരം FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(visitValuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    Ast_കണ്ണി_Assign* സ്ഥിരം assp = directStateAssign(branch.stmtsp, assignVscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    Fsm_State_മൂല്യം then_മൂല്യം;
                    Fsm_State_മൂല്യം else_മൂല്യം;
                    AstCond* സ്ഥിരം condp = VN_CAST(assp->rhsp(), Cond);
                    എങ്കിൽ (condp) {
                        UASSERT_OBJ(directStateCondConstAssign(branch.stmtsp, assignVscp,
                                                               then_മൂല്യം, else_മൂല്യം),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        എങ്കിൽ (!visitValuep(condp->thenp())) തിരികെ അസത്യം;
                        എങ്കിൽ (!visitValuep(condp->elsep())) തിരികെ അസത്യം;
                    } അല്ലെങ്കിൽ എങ്കിൽ (!add_Optional_Target_വ്യഞ്ജകം_To_State_Space(assp->rhsp(), stateSpace)) {
                        തിരികെ അസത്യം;
                    }
                }
                തിരികെ സത്യം;
            });
    }

    // Extract supported case-item transitions in one place so the conservative
    // policy for direct and ternary forms stays consistent. The false exits in
    // this helper are deliberate subset boundaries: they document shapes we do
    // not yet model in this PR and that future FSM-detection work may widen.
    സ്ഥിതം തർക്കം emitCaseItemArcs(FsmGraph& graph, AstCaseItem* itemp, AstVarScope* stateVscp,
                                 സ്ഥിരം FsmStateSpace& stateSpace, തർക്കം inclCond) {
        std::vector<std::pair<string, Fsm_State_മൂല്യം>> froms;
        എങ്കിൽ (itemp->isDefault()) {
            എങ്കിൽ (!inclCond) തിരികെ അസത്യം;
            froms.emplace_back("default", Fsm_State_മൂല്യം{});
        } അല്ലെങ്കിൽ {
            വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), കണ്ണി_വ്യഞ്ജകം)) {
                Fsm_State_മൂല്യം മൂല്യം;
                എങ്കിൽ (const_മൂല്യം_Status(condp, മൂല്യം) != Const_മൂല്യം_Status::OK) തുടരുക;
                എങ്കിൽ (!validate_Known_State_മൂല്യം(condp, stateSpace, മൂല്യം, "source")) തിരികെ സത്യം;
                froms.emplace_back(label_For_മൂല്യം(stateSpace.labels, മൂല്യം), മൂല്യം);
            }
            എങ്കിൽ (froms.empty()) തിരികെ അസത്യം;
        }

        എങ്കിൽ (Ast_കണ്ണി_Assign* സ്ഥിരം assp = directStateAssign(itemp->stmtsp(), stateVscp)) {
            Fsm_State_മൂല്യം to_മൂല്യം;
            സ്ഥിരം Const_മൂല്യം_Status status = const_മൂല്യം_Status(assp->rhsp(), to_മൂല്യം);
            എങ്കിൽ (status == Const_മൂല്യം_Status::OK) {
                എങ്കിൽ (!validate_Known_State_മൂല്യം(assp, stateSpace, to_മൂല്യം, "target")) തിരികെ സത്യം;
                വേണ്ടി (സ്ഥിരം std::pair<string, Fsm_State_മൂല്യം>& from : froms) {
                    graph.addArc(from.second, to_മൂല്യം, അസത്യം, അസത്യം, itemp->isDefault(),
                                 assp->fileline());
                }
                തിരികെ സത്യം;
            }
        }

        Fsm_State_മൂല്യം then_മൂല്യം;
        Fsm_State_മൂല്യം else_മൂല്യം;
        എങ്കിൽ (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_മൂല്യം, else_മൂല്യം)
            || ifStateConstAssign(itemp->stmtsp(), stateVscp, then_മൂല്യം, else_മൂല്യം)) {
            എങ്കിൽ (!validate_Known_State_മൂല്യം(itemp->stmtsp(), stateSpace, then_മൂല്യം, "target"))
                തിരികെ സത്യം;
            എങ്കിൽ (!validate_Known_State_മൂല്യം(itemp->stmtsp(), stateSpace, else_മൂല്യം, "target"))
                തിരികെ സത്യം;
            വേണ്ടി (സ്ഥിരം Fsm_State_മൂല്യം& branch_മൂല്യം : {then_മൂല്യം, else_മൂല്യം}) {
                വേണ്ടി (സ്ഥിരം std::pair<string, Fsm_State_മൂല്യം>& from : froms) {
                    graph.addArc(from.second, branch_മൂല്യം, അസത്യം, സത്യം, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            തിരികെ സത്യം;
        }

        തിരികെ അസത്യം;
    }

    സ്ഥിതം ശൂന്യം emitStmtArcsFrom(FsmGraph& graph, Ast_കണ്ണി* stmtsp, AstVarScope* stateVscp,
                                 സ്ഥിരം FsmStateSpace& stateSpace, Fsm_State_മൂല്യം from_മൂല്യം,
                                 തർക്കം isDefault, തർക്കം forceCond) {
        Ast_കണ്ണി_Assign* സ്ഥിരം assp = directStateAssign(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        Fsm_State_മൂല്യം to_മൂല്യം;
        സ്ഥിരം Const_മൂല്യം_Status status = const_മൂല്യം_Status(assp->rhsp(), to_മൂല്യം);
        എങ്കിൽ (status == Const_മൂല്യം_Status::OK) {
            എങ്കിൽ (!validate_Known_State_മൂല്യം(assp, stateSpace, to_മൂല്യം, "target")) തിരികെ;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            graph.addArc(from_മൂല്യം, to_മൂല്യം, അസത്യം, forceCond, isDefault, assp->fileline());
            തിരികെ;
        }

        Fsm_State_മൂല്യം then_മൂല്യം;
        Fsm_State_മൂല്യം else_മൂല്യം;
        സ്ഥിരം തർക്കം condAssign
            = directStateCondConstAssign(stmtsp, stateVscp, then_മൂല്യം, else_മൂല്യം);
        UASSERT_OBJ(condAssign, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        എങ്കിൽ (!validate_Known_State_മൂല്യം(stmtsp, stateSpace, then_മൂല്യം, "target")) തിരികെ;
        എങ്കിൽ (!validate_Known_State_മൂല്യം(stmtsp, stateSpace, else_മൂല്യം, "target")) തിരികെ;
        വേണ്ടി (സ്ഥിരം Fsm_State_മൂല്യം& branch_മൂല്യം : {then_മൂല്യം, else_മൂല്യം}) {
            graph.addArc(from_മൂല്യം, branch_മൂല്യം, അസത്യം, സത്യം, isDefault, stmtsp->fileline());
        }
    }

    സ്ഥിതം ശൂന്യം emitIfChainArcs(FsmGraph& graph, സ്ഥിരം FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, സ്ഥിരം FsmStateSpace& stateSpace) {
        വേണ്ടി (size_t i = 0; i < chain.branches.size(); ++i) {
            സ്ഥിരം FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            എങ്കിൽ (!validate_Known_State_മൂല്യം(branch.ifp, stateSpace, branch.from_മൂല്യം, "source"))
                തിരികെ;
            emitStmtArcsFrom(graph, branch.stmtsp, stateVscp, stateSpace, branch.from_മൂല്യം, അസത്യം,
                             branch.has_മുകൾ_Guard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    സ്ഥിതം ശൂന്യം addResetArcs(FsmGraph& graph, സ്ഥിരം std::vector<FsmResetArcDesc>& resetArcs,
                             സ്ഥിരം FsmStateSpace& stateSpace) {
        വേണ്ടി (സ്ഥിരം FsmResetArcDesc& resetArc : resetArcs) {
            എങ്കിൽ (!validate_Known_State_മൂല്യം(resetArc.nodep(), stateSpace, resetArc.to_മൂല്യം(),
                                         "target"))
                തുടരുക;
            graph.addArc(Fsm_State_മൂല്യം{}, resetArc.to_മൂല്യം(), സത്യം, അസത്യം, അസത്യം,
                         resetArc.nodep()->fileline());
        }
    }

    // Turn one candidate case statement into the graph representation that the
    // later lowering phase will consume directly, while reviewers can still
    // inspect the extracted machine via DOT dumps.
    ശൂന്യം processCase(AstCase* casep, AstVarScope* assignVscp, സ്ഥിരം FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, casep, "FSM case processing requires a non-null assignment var");
        AstVarScope* സ്ഥിരം stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        എങ്കിൽ (!collectStateSpace(casep, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) തിരികെ;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        എങ്കിൽ (!entry.graphp) {
            entry.graphp.reset(പുതിയ FsmGraph{});
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
            വേണ്ടി (സ്ഥിരം std::pair<string, Fsm_State_മൂല്യം>& state : stateSpace.states) {
                entry.graphp->addStateVertex(state.first, state.second);
            }
            addResetArcs(*entry.graphp, reg.resetArcs(), stateSpace);
        }
        വേണ്ടി (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            emitCaseItemArcs(*entry.graphp, itemp, assignVscp, stateSpace,
                             entry.graphp->inclCond());
        }
    }

    ശൂന്യം processIfChain(സ്ഥിരം FsmIfChainCandidate& chain, AstVarScope* assignVscp,
                        സ്ഥിരം FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, chain.ifp,
                    "FSM if-chain processing requires a non-null assignment var");
        AstVarScope* സ്ഥിരം stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        എങ്കിൽ (!collectStateSpace(chain, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) തിരികെ;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        // Case candidates keep ownership of existing graphs; reaching this path
        // means the if-chain is the only supported dispatch for this FSM.
        UASSERT_OBJ(!entry.graphp, chain.ifp, "FSM if-chain graph should not already exist");
        entry.graphp.reset(പുതിയ FsmGraph{});
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
        വേണ്ടി (സ്ഥിരം std::pair<string, Fsm_State_മൂല്യം>& state : stateSpace.states) {
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
    ശൂന്യം processOneBlockAlways(സ്ഥിരം FsmComboAlways& oneBlock) {
        സ്ഥിരം RegisterAlwaysAnalyzer analyzer{oneBlock.scopep()};
        AstAlways* സ്ഥിരം alwaysp = oneBlock.alwaysp();
        എങ്കിൽ (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) തിരികെ;
        സ്ഥിരം std::vector<std::pair<AstCase*, Ast_കണ്ണി_വ്യഞ്ജകം*>> candidates
            = analyzer.oneBlockCandidates(alwaysp);

        FsmCaseCandidate firstCand;
        വേണ്ടി (സ്ഥിരം std::pair<AstCase*, Ast_കണ്ണി_വ്യഞ്ജകം*>& cand : candidates) {
            AstVarRef* സ്ഥിരം selp = VN_CAST(cand.first->exprp(), VarRef);
            AstVarScope* സ്ഥിരം vscp = selp ? selp->varScopep() : nullptr;
            എങ്കിൽ (!vscp) തുടരുക;
            എങ്കിൽ (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processCase(cand.first, vscp, reg);
            } അല്ലെങ്കിൽ എങ്കിൽ (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed case statements found in "
                                   "the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } അല്ലെങ്കിൽ {
                cand.first->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported case "
                                   "statements found in the same always block. Only the first "
                                   "candidate will be instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            }
        }

        എങ്കിൽ (firstCand.stateVscp) തിരികെ;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        സ്ഥിരം std::vector<std::pair<AstIf*, Ast_കണ്ണി_വ്യഞ്ജകം*>> ifCandidates
            = analyzer.oneBlockIfCandidates(alwaysp);
        വേണ്ടി (സ്ഥിരം std::pair<AstIf*, Ast_കണ്ണി_വ്യഞ്ജകം*>& cand : ifCandidates) {
            സ്ഥിരം FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.first);
            FsmIfChainCandidate chain;
            എങ്കിൽ (!collectIfChain(cand.first, aliases, chain)) തുടരുക;
            AstVarScope* സ്ഥിരം vscp = chain.compareVscp;
            എങ്കിൽ (!if_Chain_Supported_Transition_കണ്ണി(chain, vscp)) തുടരുക;
            എങ്കിൽ (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processIfChain(chain, vscp, reg);
            } അല്ലെങ്കിൽ എങ്കിൽ (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed transition candidates found "
                                   "in the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } അല്ലെങ്കിൽ {
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
    സ്ഥിതം ശൂന്യം warnComboSameAlways(Ast_കണ്ണി* warnNodep, സ്ഥിരം FsmCaseCandidate& firstCand) {
        warnNodep->v3warn(FSMMULTI,
                          "FSM coverage: multiple supported transition candidates found in "
                          "the same combinational always block. Only the first candidate "
                          "will be instrumented."
                              << candidateConflictContext(warnNodep, firstCand));
    }

    ശൂന്യം processComboAlways(സ്ഥിരം FsmComboAlways& combo) {
        സ്ഥിരം ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_കണ്ണി* സ്ഥിരം stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate firstCand;
        വേണ്ടി (Ast_കണ്ണി* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* സ്ഥിരം casep = VN_CAST(nodep, Case);
            എങ്കിൽ (!casep) തുടരുക;
            സ്ഥിരം ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchCase(stmtsp, casep);
            സ്ഥിരം FsmRegisterCandidate* സ്ഥിരം matchedp = match.matchedp;
            Ast_കണ്ണി* സ്ഥിരം matchedWarnNodep = match.warnNodep;
            എങ്കിൽ (!matchedp) തുടരുക;
            എങ്കിൽ (!firstCand.stateVscp) {
                സ്ഥിരം auto തിരുകുക_Pair = m_comboPaired.emplace(
                    matchedp->stateVscp(),
                    FsmCaseCandidate{matchedWarnNodep,
                                     const_cast<AstVarScope*>(matchedp->stateVscp())});
                എങ്കിൽ (!തിരുകുക_Pair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  തിരുകുക_Pair.first->second));
                    തുടരുക;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processCase(casep, matchedp->nextVscp(), *matchedp);
                തുടരുക;
            }
            എങ്കിൽ (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                തുടരുക;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported case statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, firstCand));
        }
        എങ്കിൽ (firstCand.stateVscp) തിരികെ;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        വേണ്ടി (Ast_കണ്ണി* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* സ്ഥിരം ifp = VN_CAST(nodep, If);
            എങ്കിൽ (!ifp) തുടരുക;
            FsmIfChainCandidate chain;
            സ്ഥിരം FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            എങ്കിൽ (!collectIfChain(ifp, aliases, chain)) തുടരുക;
            സ്ഥിരം ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchIfChain(stmtsp, chain);
            സ്ഥിരം FsmRegisterCandidate* സ്ഥിരം matchedp = match.matchedp;
            Ast_കണ്ണി* സ്ഥിരം matchedWarnNodep = match.warnNodep;
            എങ്കിൽ (!matchedp) തുടരുക;
            എങ്കിൽ (!firstCand.stateVscp) {
                സ്ഥിരം std::pair<std::unordered_map<സ്ഥിരം AstVarScope*, FsmCaseCandidate>::iterator,
                                തർക്കം>
                    തിരുകുക_Pair = m_comboPaired.emplace(
                        matchedp->stateVscp(),
                        FsmCaseCandidate{matchedWarnNodep,
                                         const_cast<AstVarScope*>(matchedp->stateVscp())});
                എങ്കിൽ (!തിരുകുക_Pair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  തിരുകുക_Pair.first->second));
                    തുടരുക;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processIfChain(chain, matchedp->nextVscp(), *matchedp);
                തുടരുക;
            }
            എങ്കിൽ (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                തുടരുക;
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
    ശൂന്യം visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    // Collect processes first, then analyze FSM candidates once all alias and
    // register information is available.
    ശൂന്യം visit(AstAlways* nodep) override {
        എങ്കിൽ (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            തിരികെ;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_oneBlockAlwayss.emplace_back(m_scopep, nodep);
        സ്ഥിരം RegisterAlwaysAnalyzer analyzer{m_scopep};
        FsmRegisterCandidate reg;
        എങ്കിൽ (analyzer.matchRegisterCandidate(nodep, reg)) {
            AstVarScope* സ്ഥിരം stateVscp = reg.stateVscp();
            സ്ഥിരം തർക്കം found
                = std::any_of(m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                              [stateVscp](സ്ഥിരം FsmRegisterCandidate& existing) {
                                  തിരികെ existing.stateVscp() == stateVscp;
                              });
            എങ്കിൽ (!found) { m_registerCandidates.emplace_back(reg); }
        }
        എങ്കിൽ (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            m_comboAlwayss.emplace_back(m_scopep, nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->keyword() == VAlwaysKwd::ALWAYS) {
            എങ്കിൽ (!nodep->sentreep() || isPlainComboSentree(nodep->sentreep())) {
                m_comboAlwayss.emplace_back(m_scopep, nodep);
            } അല്ലെങ്കിൽ {
                m_nonComboAlwayss.emplace_back(m_scopep, nodep);
            }
        }
    }

    ശൂന്യം visit(AstAssignW* nodep) override {
        // Continuous aliases are unordered hardware connections, so source
        // order should not affect whether an if-chain FSM is recognized.
        collectAliasFromAssign(nodep, m_stateAliases, m_ambiguousStateAliases);
        collectCellPortAlias(nodep);
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstCell* nodep) override {
        // Cells are matched after the full traversal because linkdot lowers
        // uninlined port connections into sibling continuous assignments.
        എങ്കിൽ (m_scopep && fsmRegisterWrapperDesc(nodep)) addWrapperCell(m_scopep, nodep);
        iterateChildren(nodep);
    }

    // Continue the walk through the rest of the design hierarchy.
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        വേണ്ടി (സ്ഥിരം std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            FsmRegisterCandidate reg;
            എങ്കിൽ (matchFsmWrapperCell(wrapperCell.first, wrapperCell.second, reg)) {
                m_registerCandidates.emplace_back(reg);
            }
        }
        വേണ്ടി (സ്ഥിരം FsmComboAlways& oneBlock : m_oneBlockAlwayss) processOneBlockAlways(oneBlock);
        വേണ്ടി (സ്ഥിരം FsmComboAlways& combo : m_comboAlwayss) processComboAlways(combo);
        വേണ്ടി (സ്ഥിരം FsmComboAlways& combo : m_nonComboAlwayss) warnUnsupportedComboAlways(combo);
    }
};

// Builder of statements that will handle incrementation of proper AstNodeCoverDecls - which will
// be also created here
വർഗ്ഗം FsmCoverIncBuilder final {
    സ്ഥിരം FsmGraph& m_graph;  // Fsm graph
    std::vector<സ്ഥിരം FsmStateVertex*> m_states;  // States vertexes inside fsm graph
    std::unordered_set<uint64_t>
        m_undefinedStates;  // States that have no enum value but are possible to represent with a
                            // variable of given width.
                            // This variable keeps uint64_t instead of V3Numebr because it is safe
                            // to assume that nobody has over 16M TB of RAM
    സ്ഥിരം FsmVertex* m_defaultVertexp = nullptr;  // Default vertex
    സ്ഥിരം FsmVertex* m_resetVertexp = nullptr;  // Reset vertex
    size_t m_edges_എണ്ണം = 0;  // Total count of edges
    AstVarScope* സ്ഥിരം m_prevVscp;  // Previous state of fsm
    File_വരി* സ്ഥിരം m_flp;  // Fileline pointer to use across the builder
    സ്ഥിരം std::string m_modPrettyName;  // Pretty fsm mode name
    Ast_കണ്ണി_Stmt* m_coverIncp = nullptr;  // Increments proper CoverDecl after each change
    Ast_കണ്ണി_Stmt* m_initialCoverIncp = nullptr;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = nullptr;  // List of CoverDecls
    തർക്കം m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(സ്ഥിരം uint64_t മൂല്യം) {
        V3_സംഖ്യ അക്കം{static_cast<Ast_കണ്ണി*>(nullptr), m_prevVscp->width()};
        അക്കം.setQuad(മൂല്യം);
        അക്കം.isSigned(m_prevVscp->dtypep()->isSigned());
        തിരികെ പുതിയ AstConst{m_flp, അക്കം};
    }

    std::string quadToBitStirngLabel(സ്ഥിരം uint64_t val) {
        പൂർണ്ണസംഖ്യ width = m_prevVscp->width();
        UASSERT(width < 64, "We should not expand values that wide");
        std::string result = std::to_string(width) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        ആകുമ്പോൾ (--width >= 0) result.തള്ളുക_back("01"[(val >> width) & 1]);
        തിരികെ result;
    }

    ശൂന്യം initExistingStates() {
        വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : m_graph.vertices()) {
            സ്ഥിരം FsmVertex* സ്ഥിരം vertexp = vtx.as<FsmVertex>();
            m_edges_എണ്ണം += vertexp->inEdges().size();
            എങ്കിൽ (സ്ഥിരം FsmStateVertex* സ്ഥിരം statep = vertexp->cast<FsmStateVertex>()) {
                m_states.തള്ളുക_back(statep);
            } അല്ലെങ്കിൽ എങ്കിൽ (vertexp->isDefaultAny()) {
                UASSERT_OBJ(!std::exchange(m_defaultVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two defaults?");
            } അല്ലെങ്കിൽ /* if (vertexp->isResetAny()) */ {
                UASSERT_OBJ(!std::exchange(m_resetVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two resets?");
            }
        }
        സ്ഥിരം പൂർണ്ണസംഖ്യ width = m_graph.sampleVarScopep()->width();
        m_isComplete = width < 64 && m_states.size() == (static_cast<size_t>(1) << width);
    }

    ശൂന്യം initUndefinedStates() {
        UASSERT_OBJ(m_graph.sampleVarScopep()->width() < 64, m_graph.sampleVarScopep(),
                    "This FSM is too wide to expand - this should be caught earlier");
        സ്ഥിരം uint64_t combinations_എണ്ണം = static_cast<size_t>(1)
                                           << m_graph.sampleVarScopep()->width();
        // Rolling trick allows to cut peak memory usage
        തർക്കം undefRolled = അസത്യം;
        auto rollUndefs = [ഇത്, combinations_എണ്ണം] {
            വേണ്ടി (uint64_t i = 0; i < combinations_എണ്ണം; ++i) {
                സ്ഥിരം auto തിരുകുക = m_undefinedStates.തിരുകുക(i);
                എങ്കിൽ (!തിരുകുക.second) m_undefinedStates.erase(തിരുകുക.first);
            }
        };
        വേണ്ടി (സ്ഥിരം FsmStateVertex* സ്ഥിരം statep : m_states) {
            സ്ഥിരം V3_സംഖ്യ& അക്കം = statep->മൂല്യം().അക്കം();
            സ്ഥിരം uint64_t മൂല്യം = അക്കം.toUQuad();
            എങ്കിൽ (!m_isComplete) {
                എങ്കിൽ (undefRolled) {
                    UASSERT_OBJ(m_undefinedStates.erase(മൂല്യം) == 1, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                } അല്ലെങ്കിൽ {
                    UASSERT_OBJ(m_undefinedStates.തിരുകുക(മൂല്യം).second, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                    എങ്കിൽ (m_undefinedStates.size() * 2 >= combinations_എണ്ണം) {
                        undefRolled = സത്യം;
                        rollUndefs();
                    }
                }
            }
        }
        എങ്കിൽ (!(m_isComplete || undefRolled)) rollUndefs();
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* buildResetCond() സ്ഥിരം {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം refp
            = പുതിയ AstVarRef{m_flp, m_graph.resetCond().varScopep, VAccess::READ};
        തിരികെ m_graph.resetCond().active_താഴെയുള്ള
                   ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstLogNot{m_flp, refp})
                   : refp;
    }

    size_t get_Arc_എണ്ണം_With_Expansion(സ്ഥിരം VFsmExpandType expand) സ്ഥിരം {
        size_t arc_എണ്ണം_With_Expansion = ~0ULL;
        തിരഞ്ഞെടുപ്പ് (expand) {
        അവസ്ഥ VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_graph.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            നിർത്തുക;
        }
        അവസ്ഥ VFsmExpandType::AUTO: {
            arc_എണ്ണം_With_Expansion = m_edges_എണ്ണം;
            എങ്കിൽ (m_defaultVertexp) {
                UASSERT_OBJ(m_states.size() >= m_defaultVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arc_എണ്ണം_With_Expansion -= m_defaultVertexp->outEdges().size();
                വേണ്ടി (സ്ഥിരം FsmStateVertex* സ്ഥിരം statep : m_states) {
                    arc_എണ്ണം_With_Expansion += static_cast<size_t>(statep->outEdges().empty())
                                             * m_defaultVertexp->outEdges().size();
                }
            }
            എങ്കിൽ (m_resetVertexp) {
                UASSERT_OBJ(m_states.size() >= m_resetVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arc_എണ്ണം_With_Expansion += m_states.size() - m_resetVertexp->outEdges().size();
            }
            നിർത്തുക;
        }
        അവസ്ഥ VFsmExpandType::AUTO_EXPAND: {
            arc_എണ്ണം_With_Expansion = m_states.size() * m_states.size() - m_states.size();
            നിർത്തുക;
        }
        അവസ്ഥ VFsmExpandType::FULL: {
            സ്ഥിരം പൂർണ്ണസംഖ്യ width = m_graph.sampleVarScopep()->width();
            // 32 is derived from:
            // tranitionsCount <= std::numeric_limits<uint64_t>::max()
            // (2^width)^2 - (2^width) <= 2^64 - 1
            // (2^width)^2 - (2^width) < 2^64 since we operate on integral values we can trade 1
            // log_2((2^width)^2 - (2^width)) < 64
            // log_2((2^width)^2) < 64 - `(2^width)` is negigible so, it may be dropped
            // 2width < 64
            എങ്കിൽ (width < 32) {
                സ്ഥിരം uint64_t combinations_എണ്ണം = static_cast<size_t>(1) << width;
                arc_എണ്ണം_With_Expansion = combinations_എണ്ണം * combinations_എണ്ണം - combinations_എണ്ണം;
            }
            നിർത്തുക;
        }
        }
        തിരികെ arc_എണ്ണം_With_Expansion;
    }

    AstCoverOtherDecl* buildStateCoverDecl(സ്ഥിരം std::string& label,
                                           സ്ഥിരം std::string& fsmTag = "") {
        AstCoverOtherDecl* സ്ഥിരം declp
            = പുതിയ AstCoverOtherDecl{m_flp,
                                    "v_fsm_state/" + m_modPrettyName,
                                    m_graph.stateVarName() + "::" + label,
                                    "",
                                    0,
                                    m_graph.stateVarName(),
                                    "",
                                    label,
                                    fsmTag};
        // We run after scoping, so this is declared for one instance only
        declp->perInstance(സത്യം);
        m_declsp = Ast_കണ്ണി::addNext(m_declsp, declp);
        തിരികെ declp;
    }

    Ast_കണ്ണി_Stmt* buildCoverIncStmtp(സ്ഥിരം std::string& fromStateLabel,
                                    സ്ഥിരം std::string& toStateLabel,
                                    സ്ഥിരം std::string& baseLabel = "") {
        // Whole builder is designed in such a way that this function won't be called twice with
        // the same arguments - but if it will ever change in future, `AstCoverOtherDecl` shall be
        // cached for particular args set
        AstCoverOtherDecl* സ്ഥിരം declp = പുതിയ AstCoverOtherDecl{
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
        declp->perInstance(സത്യം);
        m_declsp = Ast_കണ്ണി::addNext(m_declsp, declp);
        തിരികെ പുതിയ AstCoverInc{m_flp, declp};
    }
    Ast_കണ്ണി_Stmt* buildExpandedTransitionHandler(സ്ഥിരം std::string& toStateLabel,
                                                സ്ഥിരം തർക്കം considerUndefs,
                                                സ്ഥിരം തർക്കം expandOnlyWithTerminalVerticies,
                                                സ്ഥിരം FsmStateVertex* സ്ഥിരം toStatep = nullptr) {
        AstCase* സ്ഥിരം arcCasep = പുതിയ AstCase{
            m_flp, VCaseType::CT_CASE, പുതിയ AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        std::unordered_set<സ്ഥിരം FsmStateVertex*> handledSoureces;
        എങ്കിൽ (toStatep) {
            വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : toStatep->inEdges()) {
                എങ്കിൽ (സ്ഥിരം FsmStateVertex* സ്ഥിരം fromp = edge.fromp()->cast<FsmStateVertex>()) {
                    എങ്കിൽ (!handledSoureces.emplace(fromp).second) തുടരുക;
                    arcCasep->addItemsp(പുതിയ AstCaseItem{
                        m_flp, makeConst(fromp->മൂല്യം().അക്കം().toUQuad()),
                        (fromp != toStatep) ? buildCoverIncStmtp(fromp->label(), toStateLabel)
                                            : nullptr});
                }
            }
        }
        വേണ്ടി (സ്ഥിരം FsmStateVertex* സ്ഥിരം fromp : m_states) {
            എങ്കിൽ (handledSoureces.find(fromp) != handledSoureces.end()) തുടരുക;
            arcCasep->addItemsp(പുതിയ AstCaseItem{
                m_flp, makeConst(fromp->മൂല്യം().അക്കം().toUQuad()),
                (fromp != toStatep
                 && (!expandOnlyWithTerminalVerticies || fromp->outEdges().empty()))
                    ? buildCoverIncStmtp(fromp->label(), toStateLabel, "artificial")
                    : nullptr});
        }
        എങ്കിൽ (!m_isComplete) {
            എങ്കിൽ (considerUndefs) {
                വേണ്ടി (സ്ഥിരം uint64_t from : m_undefinedStates) {
                    സ്ഥിരം std::string& label = quadToBitStirngLabel(from);
                    arcCasep->addItemsp(
                        പുതിയ AstCaseItem{m_flp, makeConst(from),
                                        (label != toStateLabel)
                                            ? buildCoverIncStmtp(label, toStateLabel, "artificial")
                                            : nullptr});
                }
            } അല്ലെങ്കിൽ {
                arcCasep->addItemsp(പുതിയ AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        തിരികെ arcCasep;
    }
    Ast_കണ്ണി_Stmt* buildUnxpandedTransitionHandler(സ്ഥിരം FsmStateVertex* സ്ഥിരം toStatep) {
        AstCase* സ്ഥിരം arcCasep = പുതിയ AstCase{
            m_flp, VCaseType::CT_CASE, പുതിയ AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : toStatep->inEdges()) {
            സ്ഥിരം FsmArcEdge* സ്ഥിരം arcp = edge.as<FsmArcEdge>();
            UASSERT_OBJ(!arcp->isReset(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            സ്ഥിരം FsmStateVertex* സ്ഥിരം fromp = arcp->fromp()->as<FsmStateVertex>();
            എങ്കിൽ (fromp != toStatep) {
                arcCasep->addItemsp(
                    പുതിയ AstCaseItem{m_flp, makeConst(fromp->മൂല്യം().അക്കം().toUQuad()),
                                    buildCoverIncStmtp(fromp->label(), toStatep->label())});
            }
        }
        arcCasep->addItemsp(പുതിയ AstCaseItem{m_flp, nullptr, nullptr});
        തിരികെ arcCasep;
    }

പൊതു:
    FsmCoverIncBuilder(സ്ഥിരം FsmGraph& graph, AstVarScope* സ്ഥിരം prevVscp)
        : m_graph{graph}
        , m_prevVscp{prevVscp}
        , m_flp{graph.fileline()}
        , m_modPrettyName{graph.scopep()->modp()->prettyName()} {
        initExistingStates();
        AstVarScope* സ്ഥിരം sampleVscp = graph.sampleVarScopep();
        സ്ഥിരം VFsmExpandType expand
            = graph.stateVarScopep()->varp()->attrFsmStateExpand() != VFsmExpandType::DEFAULT
                  ? graph.stateVarScopep()->varp()->attrFsmStateExpand()
                  : v3Global.opt.coverageFsmExpand();

        {
            സ്ഥിരം size_t arc_എണ്ണം_With_Expansion = get_Arc_എണ്ണം_With_Expansion(expand);
            എങ്കിൽ (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arc_എണ്ണം_With_Expansion == 0ULL
                            || arc_എണ്ണം_With_Expansion
                                   > static_cast<size_t>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arc_എണ്ണം_With_Expansion);
                തിരികെ;
            }
        }

        AstCase* സ്ഥിരം arcCasep
            = പുതിയ AstCase{m_flp, VCaseType::CT_CASE,
                          പുതിയ AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};
        AstCase* സ്ഥിരം initialStateCasep
            = പുതിയ AstCase{m_flp, VCaseType::CT_CASE,
                          പുതിയ AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};

        സ്ഥിരം തർക്കം expandAll = expand != VFsmExpandType::AUTO;
        std::unordered_set<സ്ഥിരം FsmStateVertex*>
            defaultTargets;  // States that have edge incoming from default vertex
        std::unordered_set<സ്ഥിരം FsmStateVertex*>
            resetTargets;  // States that have edge incoming from reset vertex
        എങ്കിൽ (!expandAll) {
            എങ്കിൽ (m_defaultVertexp) {
                വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : m_defaultVertexp->outEdges()) {
                    defaultTargets.emplace(edge.മുകൾ()->as<FsmStateVertex>());
                }
            }
            എങ്കിൽ (m_resetVertexp) {
                വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : m_resetVertexp->outEdges()) {
                    resetTargets.emplace(edge.മുകൾ()->as<FsmStateVertex>());
                }
            }
        }
        സ്ഥിരം തർക്കം isFullExpansion = expand == VFsmExpandType::FULL;
        എങ്കിൽ (isFullExpansion) initUndefinedStates();
        വേണ്ടി (സ്ഥിരം FsmStateVertex* സ്ഥിരം statep : m_states) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* സ്ഥിരം declp = buildStateCoverDecl(statep->label());
            initialStateCasep->addItemsp(
                പുതിയ AstCaseItem{m_flp, makeConst(statep->മൂല്യം().അക്കം().toUQuad()),
                                പുതിയ AstCoverInc{m_flp, declp, സത്യം}});
            സ്ഥിരം തർക്കം expandAllOrReset
                = expandAll || (resetTargets.find(statep) != resetTargets.end());
            സ്ഥിരം തർക്കം defaultExpantion
                = !expandAllOrReset && (defaultTargets.find(statep) != defaultTargets.end());
            arcCasep->addItemsp(പുതിയ AstCaseItem{
                m_flp, makeConst(statep->മൂല്യം().അക്കം().toUQuad()),
                Ast_കണ്ണി::addNext<Ast_കണ്ണി_Stmt>(
                    പുതിയ AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), isFullExpansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        എങ്കിൽ (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            എങ്കിൽ (isFullExpansion) {
                വേണ്ടി (സ്ഥിരം uint64_t മൂല്യം : m_undefinedStates) {
                    സ്ഥിരം std::string& label = quadToBitStirngLabel(മൂല്യം);
                    AstCoverOtherDecl* സ്ഥിരം declp = buildStateCoverDecl(label);
                    initialStateCasep->addItemsp(പുതിയ AstCaseItem{
                        m_flp, makeConst(മൂല്യം), പുതിയ AstCoverInc{m_flp, declp, സത്യം}});
                    arcCasep->addItemsp(
                        പുതിയ AstCaseItem{m_flp, makeConst(മൂല്യം),
                                        Ast_കണ്ണി::addNext<Ast_കണ്ണി_Stmt>(
                                            പുതിയ AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, സത്യം, അസത്യം))});
                }
            } അല്ലെങ്കിൽ {
                // No expansion
                arcCasep->addItemsp(പുതിയ AstCaseItem{m_flp, nullptr, nullptr});
                initialStateCasep->addItemsp(പുതിയ AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        m_coverIncp = arcCasep;
        m_initialCoverIncp = initialStateCasep;
    }
    VL_UNCOPYABLE(FsmCoverIncBuilder);
    ~FsmCoverIncBuilder() = സ്ഥിരസ്ഥിതി;
    Ast_കണ്ണി_Cover_Decl* getCoverDeclsp() സ്ഥിരം { തിരികെ m_declsp; }
    Ast_കണ്ണി_Stmt* getCoverIncp() സ്ഥിരം { തിരികെ m_coverIncp; }
    Ast_കണ്ണി_Stmt* getInitialCoverIncp() സ്ഥിരം { തിരികെ m_initialCoverIncp; }
};

// Lower the completed FSM graphs into the concrete coverage declarations,
// previous-state tracking, and pre/post-triggered instrumentation that the
// runtime uses to record state and transition coverage.
വർഗ്ഗം FsmLowerVisitor final {
    // STATE - across all visitors
    സ്ഥിരം FsmState& m_state;
    V3SharedTmps m_prevTmps{"__VfsmCovPrev", VVarType::MODULETEMP};  // Previous states

    // METHODS
    // Rebuild the original event control from the saved sense description so
    // post-state coverage sampling runs on the same triggering edges.
    സ്ഥിതം Ast_Sen_വൃക്ഷം* build_Sen_വൃക്ഷം(File_വരി* flp, സ്ഥിരം std::vector<FsmSenDesc>& senses) {
        Ast_Sen_വൃക്ഷം* സ്ഥിരം sentreep = പുതിയ Ast_Sen_വൃക്ഷം{flp, nullptr};
        വേണ്ടി (സ്ഥിരം FsmSenDesc& sense : senses) {
            AstSenItem* സ്ഥിരം senItemp
                = പുതിയ AstSenItem{flp, VEdgeType{sense.edgeType},
                                 പുതിയ AstVarRef{flp, sense.varScopep, VAccess::READ}};
            sentreep->addSensesp(senItemp);
        }
        തിരികെ sentreep;
    }

    // Lower one fully detected FSM graph into the concrete coverage machinery
    // used by generated models: declarations, previous-state tracking, and the
    // pre/post-triggered increment logic for states and arcs.
    ശൂന്യം buildOne(സ്ഥിരം FsmGraph& graph) {
        UINFO(1, "buildOne lowering FSM " << graph.stateVarName()
                                          << " vertices=" << graph.vertices().size() << endl);
        AstAlways* സ്ഥിരം alwaysp = graph.stateAlwaysp();
        AstScope* സ്ഥിരം scopep = graph.scopep();
        AstVarScope* സ്ഥിരം stateVscp = graph.stateVarScopep();
        AstVarScope* സ്ഥിരം sampleVscp = graph.sampleVarScopep();
        File_വരി* സ്ഥിരം flp = graph.fileline();
        Ast_കണ്ണി_Module* സ്ഥിരം modp = scopep->modp();
        AstVarScope* സ്ഥിരം prevVscp
            = m_prevTmps.make(flp, scopep, sampleVscp->dtypep(), stateVscp->varp()->shortName());
        // The saved previous-state temp crosses the scheduler's pre/post split
        // in the same way as Verilator's built-in NBA shadow variables, so keep
        // both vars marked as post-life participants for stable MT ordering.
        stateVscp->optimizeLifePost(സത്യം);
        sampleVscp->optimizeLifePost(സത്യം);
        prevVscp->optimizeLifePost(സത്യം);

        AstActive* സ്ഥിരം initActivep
            = പുതിയ AstActive{flp, "fsm-coverage-init",
                            പുതിയ Ast_Sen_വൃക്ഷം{flp, പുതിയ AstSenItem{flp, AstSenItem::Initial{}}}};
        initActivep->sen_വൃക്ഷം_Storep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->addStmtsp(പുതിയ AstInitialStatic{
            flp, പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, prevVscp, VAccess::WRITE},
                               പുതിയ AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->addBlocksp(initActivep);

        AstAlwaysPost* സ്ഥിരം covPostp = പുതിയ AstAlwaysPost{flp};
        തർക്കം updatePrevAfterPost = അസത്യം;
        എങ്കിൽ (alwaysp) {
            // Save the previous state as plain sequential logic at the front of
            // the original always_ff body, then evaluate coverage in post logic
            // after the delayed state update commits. This avoids a scheduler
            // race between a separate AstAlwaysPre task and the real state
            // commit for direct parent-level registers.
            alwaysp->prependStmtsp(പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, prevVscp, VAccess::WRITE},
                                                 പുതിയ AstVarRef{flp, sampleVscp, VAccess::READ}});
        } അല്ലെങ്കിൽ {
            // Wrapper-derived register candidates do not have a parent
            // always_ff body to splice into. Sample coverage first, then save
            // the current state for the next clock tick; this survives cell
            // boundary scheduling where the real flop update lives elsewhere.
            updatePrevAfterPost = സത്യം;
            prevVscp->varp()->setIgnorePostRead();
        }

        {
            // Create and increment proper AstNodeCoverDecls
            സ്ഥിരം FsmCoverIncBuilder coverIncBuilder{graph, prevVscp};
            modp->addStmtsp(coverIncBuilder.getCoverDeclsp());
            initActivep->addStmtsp(coverIncBuilder.getInitialCoverIncp());
            covPostp->addStmtsp(
                പുതിയ AstIf{flp,
                          പുതിയ AstNeq{flp, പുതിയ AstVarRef{flp, prevVscp, VAccess::READ},
                                     പുതിയ AstVarRef{flp, sampleVscp, VAccess::READ}},
                          coverIncBuilder.getCoverIncp()});
        }

        എങ്കിൽ (updatePrevAfterPost) {
            covPostp->addStmtsp(പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, prevVscp, VAccess::WRITE},
                                              പുതിയ AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        Ast_Sen_വൃക്ഷം* സ്ഥിരം sentreep = build_Sen_വൃക്ഷം(flp, graph.senses());
        AstActive* സ്ഥിരം activep = പുതിയ AstActive{flp, "fsm-coverage", sentreep};
        activep->sen_വൃക്ഷം_Storep(sentreep);
        scopep->addBlocksp(activep);
        activep->addStmtsp(covPostp);
    }

പൊതു:
    // CONSTRUCTORS
    // Lower every detected FSM graph from the shared local state into
    // concrete coverage instrumentation while the saved scoped pointers are
    // still valid in the same pass.
    explicit FsmLowerVisitor(സ്ഥിരം FsmState& state)
        : m_state{state} {
        വേണ്ടി (സ്ഥിരം DetectedFsm& fsm : m_state.fsms()) { buildOne(*fsm.graphp); }
    }
};

}  // namespace

ശൂന്യം V3FsmDetect::detect(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    FsmState state;
    // Phase 1: recover each supported FSM into a complete graph while the
    // original clocked/case structure is still easy to recognize.
    FsmDetectVisitor detect{state, rootp};
    എങ്കിൽ (dumpGraphLevel() >= 6) {
        size_t index = 0;
        വേണ്ടി (സ്ഥിരം DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dumpTag(index++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { FsmLowerVisitor lower{state}; }
    V3Global::dump_Check_Global_വൃക്ഷം("fsm-detect", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
