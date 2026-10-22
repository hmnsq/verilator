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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"

#ଅନ୍ତର୍ଭାଵ "V3FsmDetect.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Control.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3SharedTmps.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <cctype>
#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <memory>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

ନାମାକାଶ {

// Width-preserving FSM state identity. FSM detection needs a stable key for
// graph vertices and lookup tables, but lowering still needs the original
// folded Verilog value so emitted comparisons keep the correct width and bits.
ଶ୍ରେଣୀ Fsm_State_ମୂଲ୍ୟ final {
    // Hash/equality key only. It deliberately ignores signedness because
    // signed and unsigned constants with the same width and bits denote the
    // same encoded FSM state.
    string m_ଚାବି;  // Canonical "width:value" identity, independent of signedness

    // Semantic value. This is what diagnostics and lowering use when printing
    // values or rebuilding AstConst nodes for instrumentation.
    V3_ସଂଖ୍ୟା m_ଅଙ୍କ;  // Original folded value, preserving width for lowered comparisons

    ସ୍ଥିର string make_ଚାବି(ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ) {
        V3_ସଂଖ୍ୟା ଚାବି_ଅଙ୍କ = ଅଙ୍କ;
        // Signedness does not change FSM state identity: same width and bits
        // should address the same graph vertex.
        ଚାବି_ଅଙ୍କ.isSigned(ମିଥ୍ୟା);
        ଫେରନ୍ତୁ cvtToStr(ଚାବି_ଅଙ୍କ.width()) + ":" + ଚାବି_ଅଙ୍କ.ascii(ସତ୍ୟ, ସତ୍ୟ);
    }

ସର୍ବସାଧାରଣ:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    Fsm_State_ମୂଲ୍ୟ()
        : m_ଚାବି{"1:1'h0"}
        , m_ଅଙ୍କ{static_cast<Ast_ଗଣ୍ଠି*>(nullptr), 1, 0} {}
    explicit Fsm_State_ମୂଲ୍ୟ(ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ)
        : m_ଚାବି{make_ଚାବି(ଅଙ୍କ)}
        , m_ଅଙ୍କ{ଅଙ୍କ} {}

    ଅଚଳ string& ଚାବି() ଅଚଳ { ଫେରନ୍ତୁ m_ଚାବି; }
    ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ() ଅଚଳ { ଫେରନ୍ତୁ m_ଅଙ୍କ; }
    string ascii() ଅଚଳ { ଫେରନ୍ତୁ m_ଅଙ୍କ.ascii(ସତ୍ୟ, ସତ୍ୟ); }
    string warnText() ଅଚଳ {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        ଯଦି (m_ଅଙ୍କ.width() <= 32) ଫେରନ୍ତୁ cvtToStr(m_ଅଙ୍କ.toUInt());
        ଫେରନ୍ତୁ ascii();
    }

    ତାର୍କିକ operator==(ଅଚଳ Fsm_State_ମୂଲ୍ୟ& rhs) ଅଚଳ { ଫେରନ୍ତୁ m_ଚାବି == rhs.m_ଚାବି; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
ସଂରଚନା Fsm_State_ମୂଲ୍ୟ_Hash final {
    size_t operator()(ଅଚଳ Fsm_State_ମୂଲ୍ୟ& ମୂଲ୍ୟ) ଅଚଳ {
        ଫେରନ୍ତୁ std::hash<string>{}(ମୂଲ୍ୟ.ଚାବି());
    }
};

// Captures one sensitivity-list entry so the lowering phase can later rebuild
// an active block with the same triggering event control.
ସଂରଚନା FsmSenDesc final {
    // Encoded edge kind copied from AstSenItem::edgeType() so lowering can
    // rebuild the same trigger semantics on the synthesized coverage block.
    VEdgeType::en edgeType = static_cast<VEdgeType::en>(0);
    // Triggering signal in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
};

// Captures the simple reset predicate shape that survives to this pass after
// earlier normalization so reset arcs can be reconstructed during lowering.
ସଂରଚନା FsmResetCondDesc final {
    // Reset signal used by the FSM in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
    ତାର୍କିକ active_ତଳ = ମିଥ୍ୟା;
};

ଶ୍ରେଣୀ FsmResetArcDesc final {
    Fsm_State_ମୂଲ୍ୟ m_to_ମୂଲ୍ୟ;  // Encoded reset target state.
    Ast_ଗଣ୍ଠି* m_nodep = nullptr;  // Source node for warnings and emitted metadata.
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_valuep = nullptr;  // Expression that provided the reset value.

ସର୍ବସାଧାରଣ:
    FsmResetArcDesc() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    FsmResetArcDesc(Fsm_State_ମୂଲ୍ୟ to_ମୂଲ୍ୟ, Ast_ଗଣ୍ଠି_Assign* nodep)
        : m_to_ମୂଲ୍ୟ{to_ମୂଲ୍ୟ}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(Fsm_State_ମୂଲ୍ୟ to_ମୂଲ୍ୟ, Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep)
        : m_to_ମୂଲ୍ୟ{to_ମୂଲ୍ୟ}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    Fsm_State_ମୂଲ୍ୟ to_ମୂଲ୍ୟ() ଅଚଳ { ଫେରନ୍ତୁ m_to_ମୂଲ୍ୟ; }
    Ast_ଗଣ୍ଠି* nodep() ଅଚଳ { ଫେରନ୍ତୁ m_nodep; }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep() ଅଚଳ { ଫେରନ୍ତୁ m_valuep; }
};

ସଂରଚନା FsmWrapperRoles final {
    string dPort;
    string qPort;
    string clkPort;
    string rstPort;
    string rstValParam;
    ତାର୍କିକ has_Rst_Active_ତଳ = ମିଥ୍ୟା;
    ତାର୍କିକ rst_Active_ତଳ = ମିଥ୍ୟା;
};

ସ୍ଥିର ତାର୍କିକ fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, ଅଚଳ string& portName,
                                                  ତାର୍କିକ& active_ତଳ) {
    ତାର୍କିକ matched = ମିଥ୍ୟା;
    cellp->modp()->foreach([&](AstSenItem* itemp) {
        Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ vrefp = itemp->varrefp();
        ଯଦି (!vrefp) ଫେରନ୍ତୁ;
        ଯଦି (vrefp->varp()->name() != portName) ଫେରନ୍ତୁ;
        active_ତଳ = itemp->edgeType() == VEdgeType::ET_NEGEDGE;
        matched = ସତ୍ୟ;
    });
    ଫେରନ୍ତୁ matched;
}

ସ୍ଥିର ଅଚଳ V3Control::FsmRegisterWrapper* fsmRegisterWrapperDesc(AstCell* cellp) {
    Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = cellp->modp();
    ଅଚଳ string origName = modp->origName();
    ଯଦି (ଅଚଳ V3Control::FsmRegisterWrapper* ଅଚଳ descp
        = V3Control::getFsmRegisterWrapper(origName)) {
        ଫେରନ୍ତୁ descp;
    }
    ଫେରନ୍ତୁ V3Control::getFsmRegisterWrapper(modp->prettyDehashOrigOrName());
}

ସ୍ଥିର FsmWrapperRoles rolesFromDesc(ଅଚଳ V3Control::FsmRegisterWrapper& desc) {
    FsmWrapperRoles roles;
    roles.dPort = desc.d;
    roles.qPort = desc.q;
    roles.clkPort = desc.clock;
    roles.rstPort = desc.reset;
    roles.rstValParam = desc.reset_ମୂଲ୍ୟ;
    ଫେରନ୍ତୁ roles;
}

ଶ୍ରେଣୀ FsmRegisterCandidate final {
    AstScope* m_scopep = nullptr;  // Owning scope for the paired FSM.
    AstAlways* m_alwaysp = nullptr;  // Register process that commits the state.
    AstVarScope* m_stateVscp = nullptr;  // Registered FSM state variable.
    AstVarScope* m_sampleVscp = nullptr;  // Variable sampled by lowered coverage logic.
    AstVarScope* m_nextVscp = nullptr;  // Next-state variable or same state var for 1-block FSMs.
    std::vector<FsmSenDesc> m_senses;  // Event controls for recreated coverage blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate, if any.
    std::vector<FsmResetArcDesc> m_resetArcs;  // Reset target arcs recovered during detect.
    ତାର୍କିକ m_hasResetCond = ମିଥ୍ୟା;  // Whether the FSM had a modeled reset predicate.
    ତାର୍କିକ m_inclCond = ମିଥ୍ୟା;  // Whether conditional/default arcs are kept explicitly.
    File_ଧାଡ଼ି* m_flp = nullptr;  // Representative source location.

ସର୍ବସାଧାରଣ:
    AstScope* scopep() ଅଚଳ { ଫେରନ୍ତୁ m_scopep; }
    ଶୂନ୍ୟ scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* alwaysp() ଅଚଳ { ଫେରନ୍ତୁ m_alwaysp; }
    ଶୂନ୍ୟ alwaysp(AstAlways* alwaysp) { m_alwaysp = alwaysp; }
    AstVarScope* stateVscp() ଅଚଳ { ଫେରନ୍ତୁ m_stateVscp; }
    ଶୂନ୍ୟ stateVscp(AstVarScope* vscp) { m_stateVscp = vscp; }
    AstVarScope* sampleVscp() ଅଚଳ { ଫେରନ୍ତୁ m_sampleVscp ? m_sampleVscp : m_stateVscp; }
    ଶୂନ୍ୟ sampleVscp(AstVarScope* vscp) { m_sampleVscp = vscp; }
    AstVarScope* nextVscp() ଅଚଳ { ଫେରନ୍ତୁ m_nextVscp; }
    ଶୂନ୍ୟ nextVscp(AstVarScope* vscp) { m_nextVscp = vscp; }
    ଅଚଳ std::vector<FsmSenDesc>& senses() ଅଚଳ { ଫେରନ୍ତୁ m_senses; }
    std::vector<FsmSenDesc>& senses() { ଫେରନ୍ତୁ m_senses; }
    ଅଚଳ FsmResetCondDesc& resetCond() ଅଚଳ { ଫେରନ୍ତୁ m_resetCond; }
    FsmResetCondDesc& resetCond() { ଫେରନ୍ତୁ m_resetCond; }
    ଅଚଳ std::vector<FsmResetArcDesc>& resetArcs() ଅଚଳ { ଫେରନ୍ତୁ m_resetArcs; }
    std::vector<FsmResetArcDesc>& resetArcs() { ଫେରନ୍ତୁ m_resetArcs; }
    ତାର୍କିକ hasResetCond() ଅଚଳ { ଫେରନ୍ତୁ m_hasResetCond; }
    ଶୂନ୍ୟ hasResetCond(ତାର୍କିକ flag) { m_hasResetCond = flag; }
    ତାର୍କିକ inclCond() ଅଚଳ { ଫେରନ୍ତୁ m_inclCond; }
    ଶୂନ୍ୟ inclCond(ତାର୍କିକ flag) { m_inclCond = flag; }
    File_ଧାଡ଼ି* fileline() ଅଚଳ { ଫେରନ୍ତୁ m_flp; }
    ଶୂନ୍ୟ fileline(File_ଧାଡ଼ି* flp) { m_flp = flp; }
};

ଶ୍ରେଣୀ FsmComboAlways final {
    AstScope* ଅଚଳ m_scopep = nullptr;  // Owning scope for the combinational process.
    AstAlways* ଅଚଳ m_alwaysp = nullptr;  // Candidate transition process.

ସର୍ବସାଧାରଣ:
    FsmComboAlways() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    FsmComboAlways(AstScope* scopep, AstAlways* alwaysp)
        : m_scopep{scopep}
        , m_alwaysp{alwaysp} {}

    AstScope* scopep() ଅଚଳ { ଫେରନ୍ତୁ m_scopep; }
    AstAlways* alwaysp() ଅଚଳ { ଫେରନ୍ତୁ m_alwaysp; }
};

ଶ୍ରେଣୀ FsmGraph;

ଶ୍ରେଣୀ FsmVertex VL_NOT_FINAL : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(FsmVertex, V3GraphVertex)

ସର୍ବସାଧାରଣ:
    ଗଣନା ଶ୍ରେଣୀ Kind : uint8_t { STATE, RESET_ANY, DEFAULT_ANY };

ବ୍ୟକ୍ତିଗତ:
    Kind m_kind;  // State vs synthetic ANY/default vertex role.
    string m_label;  // User-facing state or pseudo-state label.
    Fsm_State_ମୂଲ୍ୟ m_ମୂଲ୍ୟ;  // Encoded state value for real state vertices.

ସୁରକ୍ଷିତ:
    FsmVertex(V3Graph* graphp, Kind kind, string label, Fsm_State_ମୂଲ୍ୟ ମୂଲ୍ୟ) VL_MT_DISABLED
        : V3GraphVertex{graphp},
          m_kind{kind},
          m_label{label},
          m_ମୂଲ୍ୟ{ମୂଲ୍ୟ} {}
    ~FsmVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

ସର୍ବସାଧାରଣ:
    Kind kind() ଅଚଳ { ଫେରନ୍ତୁ m_kind; }
    ତାର୍କିକ isState() ଅଚଳ { ଫେରନ୍ତୁ m_kind == Kind::STATE; }
    ତାର୍କିକ isResetAny() ଅଚଳ { ଫେରନ୍ତୁ m_kind == Kind::RESET_ANY; }
    ତାର୍କିକ isDefaultAny() ଅଚଳ { ଫେରନ୍ତୁ m_kind == Kind::DEFAULT_ANY; }
    ଅଚଳ string& label() ଅଚଳ { ଫେରନ୍ତୁ m_label; }
    ଅଚଳ Fsm_State_ମୂଲ୍ୟ& ମୂଲ୍ୟ() ଅଚଳ { ଫେରନ୍ତୁ m_ମୂଲ୍ୟ; }

    string name() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ m_label + "=" + m_ମୂଲ୍ୟ.ascii(); }
};

ଶ୍ରେଣୀ FsmStateVertex final : ସର୍ବସାଧାରଣ FsmVertex {
    VL_RTTI_IMPL(FsmStateVertex, FsmVertex)

ସର୍ବସାଧାରଣ:
    FsmStateVertex(V3Graph* graphp, string label, Fsm_State_ମୂଲ୍ୟ ମୂଲ୍ୟ) VL_MT_DISABLED
        : FsmVertex{graphp, Kind::STATE, label, ମୂଲ୍ୟ} {}
    ~FsmStateVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "lightblue"; }
    string dotShape() ଅଚଳ override { ଫେରନ୍ତୁ "ellipse"; }
};

ଶ୍ରେଣୀ FsmPseudoVertex final : ସର୍ବସାଧାରଣ FsmVertex {
    VL_RTTI_IMPL(FsmPseudoVertex, FsmVertex)

ସର୍ବସାଧାରଣ:
    FsmPseudoVertex(V3Graph* graphp, Kind kind, string label) VL_MT_DISABLED
        : FsmVertex{graphp, kind, label, Fsm_State_ମୂଲ୍ୟ{}} {}
    ~FsmPseudoVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    string name() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ label(); }
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ isResetAny() ? "darkgreen" : "orange"; }
    string dotShape() ଅଚଳ override { ଫେରନ୍ତୁ "diamond"; }
};

ଶ୍ରେଣୀ FsmArcEdge final : ସର୍ବସାଧାରଣ V3GraphEdge {
    VL_RTTI_IMPL(FsmArcEdge, V3GraphEdge)
    ତାର୍କିକ m_isReset = ମିଥ୍ୟା;  // Arc originates from the synthetic reset source.
    ତାର୍କିକ m_isCond = ମିଥ୍ୟା;  // Arc came from a conditional next-state split.
    ତାର୍କିକ m_isDefault = ମିଥ୍ୟା;  // Arc represents a case default source.
    File_ଧାଡ଼ି* m_flp = nullptr;  // Source location for emitted coverage metadata.

ସର୍ବସାଧାରଣ:
    FsmArcEdge(V3Graph* graphp, FsmVertex* fromp, FsmStateVertex* ଶୀର୍ଷ, ତାର୍କିକ isReset, ତାର୍କିକ isCond,
               ତାର୍କିକ isDefault, File_ଧାଡ଼ି* flp) VL_MT_DISABLED : V3GraphEdge{graphp, fromp, ଶୀର୍ଷ, 1},
                                                               m_isReset{isReset},
                                                               m_isCond{isCond},
                                                               m_isDefault{isDefault},
                                                               m_flp{flp} {}
    ~FsmArcEdge() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    ତାର୍କିକ isReset() ଅଚଳ { ଫେରନ୍ତୁ m_isReset; }
    ତାର୍କିକ isCond() ଅଚଳ { ଫେରନ୍ତୁ m_isCond; }
    ତାର୍କିକ isDefault() ଅଚଳ { ଫେରନ୍ତୁ m_isDefault; }
    File_ଧାଡ଼ି* fileline() ଅଚଳ { ଫେରନ୍ତୁ m_flp; }

    string dotLabel() ଅଚଳ override {
        ଯଦି (m_isReset) ଫେରନ୍ତୁ "reset";
        ଯଦି (m_isDefault) ଫେରନ୍ତୁ "default";
        ଯଦି (m_isCond) ଫେରନ୍ତୁ "cond";
        ଫେରନ୍ତୁ "";
    }
    string dotColor() ଅଚଳ override {
        ଯଦି (m_isReset) ଫେରନ୍ତୁ "darkgreen";
        ଯଦି (m_isDefault) ଫେରନ୍ତୁ "orange";
        ଯଦି (m_isCond) ଫେରନ୍ତୁ "blue";
        ଫେରନ୍ତୁ "black";
    }
};

// One graph per detected FSM. Graph-level metadata captures the non-graph
// context needed to lower states/arcs back into the AST after detection.
ଶ୍ରେଣୀ FsmGraph final : ସର୍ବସାଧାରଣ V3Graph {
    AstScope* m_scopep = nullptr;  // Owning scoped block for the detected FSM.
    AstAlways* m_stateAlwaysp = nullptr;  // Register always block being instrumented.
    string m_stateVarName;  // Pretty state variable name for user-visible output.
    string m_stateVarInternalName;  // Internal state symbol name for dump tags.
    AstVarScope* m_stateVarScopep = nullptr;  // Scoped state variable being tracked.
    AstVarScope* m_sampleVarScopep = nullptr;  // Scoped variable sampled by coverage logic.
    std::vector<FsmSenDesc> m_senses;  // Saved event controls for recreated active blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate shape, if one exists.
    ତାର୍କିକ m_hasResetCond = ମିଥ୍ୟା;  // Whether the detected FSM had a reset branch.
    ତାର୍କିକ m_inclCond = ମିଥ୍ୟା;  // Whether conditional arcs should be kept explicitly.
    File_ଧାଡ଼ି* m_flp = nullptr;  // Representative source location for declarations/arcs.
    std::unordered_map<Fsm_State_ମୂଲ୍ୟ, FsmStateVertex*, Fsm_State_ମୂଲ୍ୟ_Hash>
        m_stateVertices;  // Value to state map.
    FsmPseudoVertex* m_resetVertexp = nullptr;  // Synthetic ANY source for reset arcs.
    FsmPseudoVertex* m_defaultVertexp = nullptr;  // Synthetic default source for case defaults.

ସର୍ବସାଧାରଣ:
    FsmGraph() VL_MT_DISABLED
        : m_resetVertexp{ନୂତନ FsmPseudoVertex{ଏହି, FsmVertex::Kind::RESET_ANY, "ANY"}},
          m_defaultVertexp{ନୂତନ FsmPseudoVertex{ଏହି, FsmVertex::Kind::DEFAULT_ANY, "default"}} {}

    AstScope* scopep() ଅଚଳ { ଫେରନ୍ତୁ m_scopep; }
    ଶୂନ୍ୟ scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* stateAlwaysp() ଅଚଳ { ଫେରନ୍ତୁ m_stateAlwaysp; }
    ଶୂନ୍ୟ stateAlwaysp(AstAlways* alwaysp) { m_stateAlwaysp = alwaysp; }
    ଅଚଳ string& stateVarName() ଅଚଳ { ଫେରନ୍ତୁ m_stateVarName; }
    ଶୂନ୍ୟ stateVarName(ଅଚଳ string& name) { m_stateVarName = name; }
    ଅଚଳ string& stateVarInternalName() ଅଚଳ { ଫେରନ୍ତୁ m_stateVarInternalName; }
    ଶୂନ୍ୟ stateVarInternalName(ଅଚଳ string& name) { m_stateVarInternalName = name; }
    AstVarScope* stateVarScopep() ଅଚଳ { ଫେରନ୍ତୁ m_stateVarScopep; }
    ଶୂନ୍ୟ stateVarScopep(AstVarScope* vscp) { m_stateVarScopep = vscp; }
    AstVarScope* sampleVarScopep() ଅଚଳ {
        ଫେରନ୍ତୁ m_sampleVarScopep ? m_sampleVarScopep : m_stateVarScopep;
    }
    ଶୂନ୍ୟ sampleVarScopep(AstVarScope* vscp) { m_sampleVarScopep = vscp; }
    ଅଚଳ std::vector<FsmSenDesc>& senses() ଅଚଳ { ଫେରନ୍ତୁ m_senses; }
    std::vector<FsmSenDesc>& senses() { ଫେରନ୍ତୁ m_senses; }
    ଅଚଳ FsmResetCondDesc& resetCond() ଅଚଳ { ଫେରନ୍ତୁ m_resetCond; }
    FsmResetCondDesc& resetCond() { ଫେରନ୍ତୁ m_resetCond; }
    ତାର୍କିକ hasResetCond() ଅଚଳ { ଫେରନ୍ତୁ m_hasResetCond; }
    ଶୂନ୍ୟ hasResetCond(ତାର୍କିକ flag) { m_hasResetCond = flag; }
    ତାର୍କିକ inclCond() ଅଚଳ { ଫେରନ୍ତୁ m_inclCond; }
    ଶୂନ୍ୟ inclCond(ତାର୍କିକ flag) { m_inclCond = flag; }
    File_ଧାଡ଼ି* fileline() ଅଚଳ { ଫେରନ୍ତୁ m_flp; }
    ଶୂନ୍ୟ fileline(File_ଧାଡ଼ି* flp) { m_flp = flp; }

    FsmStateVertex* addStateVertex(string label, Fsm_State_ମୂଲ୍ୟ ମୂଲ୍ୟ) VL_MT_DISABLED {
        FsmStateVertex* ଅଚଳ vertexp = ନୂତନ FsmStateVertex{ଏହି, label, ମୂଲ୍ୟ};
        m_stateVertices.emplace(ମୂଲ୍ୟ, vertexp);
        ଫେରନ୍ତୁ vertexp;
    }
    FsmPseudoVertex* resetAnyVertex() VL_MT_DISABLED { ଫେରନ୍ତୁ m_resetVertexp; }
    FsmPseudoVertex* defaultAnyVertex() VL_MT_DISABLED { ଫେରନ୍ତୁ m_defaultVertexp; }
    FsmArcEdge* addArc(Fsm_State_ମୂଲ୍ୟ from_ମୂଲ୍ୟ, Fsm_State_ମୂଲ୍ୟ to_ମୂଲ୍ୟ, ତାର୍କିକ isReset, ତାର୍କିକ isCond,
                       ତାର୍କିକ isDefault, File_ଧାଡ଼ି* flp) VL_MT_DISABLED {
        FsmStateVertex* ଅଚଳ ଶୀର୍ଷ = m_stateVertices.at(to_ମୂଲ୍ୟ);
        FsmVertex* fromp = nullptr;
        ଯଦି (isReset) {
            fromp = resetAnyVertex();
        } ଅନ୍ୟଥା ଯଦି (isDefault) {
            fromp = defaultAnyVertex();
        } ଅନ୍ୟଥା {
            fromp = m_stateVertices.at(from_ମୂଲ୍ୟ);
        }
        ଫେରନ୍ତୁ ନୂତନ FsmArcEdge{ଏହି, fromp, ଶୀର୍ଷ, isReset, isCond, isDefault, flp};
    }

    string name() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ "FSM "
               + (m_stateVarName.empty() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dumpTag(size_t index) ଅଚଳ {
        string tag = stateVarInternalName();
        ପାଇଁ (ଅକ୍ଷର& ch : tag) {
            ଯଦି (!std::isalnum(static_cast<unsigned ଅକ୍ଷର>(ch))) ch = '_';
        }
        ଫେରନ୍ତୁ "fsm_" + cvtToStr(index) + "_" + tag;
    }
};

ସଂରଚନା DetectedFsm final {
    std::unique_ptr<FsmGraph> graphp;  // Extracted graph for one detected FSM candidate.
};

ସଂରଚନା FsmCaseCandidate final {
    Ast_ଗଣ୍ଠି* warnNodep = nullptr;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = nullptr;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
ସଂରଚନା FsmStateComparison final {
    AstVarScope* stateVscp = nullptr;  // Compared state variable
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep = nullptr;  // Compared constant value expression
    Fsm_State_ମୂଲ୍ୟ ମୂଲ୍ୟ;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
ସଂରଚନା FsmIfBranch final {
    AstIf* ifp = nullptr;  // Source if/else-if node
    Ast_ଗଣ୍ଠି* stmtsp = nullptr;  // Branch body
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep = nullptr;  // Source state value expression
    Fsm_State_ମୂଲ୍ୟ from_ମୂଲ୍ୟ;  // Encoded source state value
    ତାର୍କିକ has_ଶୀର୍ଷ_Guard = ମିଥ୍ୟା;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
ସଂରଚନା FsmIfChainCandidate final {
    AstIf* ifp = nullptr;  // Top-level if-chain node
    AstVarScope* compareVscp = nullptr;  // Variable used by every state comparison
    std::vector<FsmIfBranch> branches;  // Recognized state-dispatch branches
    Ast_ଗଣ୍ଠି* defaultStmtsp = nullptr;  // Optional final else body
};

// Aliases are accepted only when they are equivalent to spelling the state
// comparison inline; this avoids inferring FSM semantics from arbitrary logic.
using FsmAliasMap = std::unordered_map<ଅଚଳ AstVarScope*, FsmStateComparison>;
using FsmCellPortMap = std::unordered_map<string, AstVarScope*>;
using FsmCellPortAliasMap = std::unordered_map<ଅଚଳ AstCell*, FsmCellPortMap>;

ସଂରଚନା StateConstLabel final {
    string text;
    ତାର୍କିକ fromParam = ମିଥ୍ୟା;
    size_t stateIndex = 0;
};

ସଂରଚନା FsmStateSpace final {
    std::vector<std::pair<string, Fsm_State_ମୂଲ୍ୟ>> states;  // User label and encoded value
    std::unordered_map<Fsm_State_ମୂଲ୍ୟ, StateConstLabel, Fsm_State_ମୂଲ୍ୟ_Hash>
        labels;  // Encoded value to label
    AstVar* stateVarp = nullptr;  // Tracked FSM state variable
    ତାର୍କିକ enumBacked = ମିଥ୍ୟା;  // Whether states came from an enum declaration
};

// Local shared state between the two adjacent FSM coverage phases. Detection
// fills this with recovered FSM graphs; lowering consumes the completed graphs
// immediately afterward without needing any AST serialization bridge.
ଶ୍ରେଣୀ FsmState final {
    // All detected FSMs in discovery order. This is the only bridge between
    // the adjacent detect and lower phases, so the second phase never needs to
    // rediscover or serialize the extracted machine.
    std::vector<DetectedFsm> m_fsms;
    std::map<ଅଚଳ AstVarScope*, size_t> m_fsmIndex;

ସର୍ବସାଧାରଣ:
    DetectedFsm& fsmFor(AstVarScope* stateVscp) {
        ଅଚଳ std::map<ଅଚଳ AstVarScope*, size_t>::const_iterator it = m_fsmIndex.find(stateVscp);
        ଯଦି (it != m_fsmIndex.end()) ଫେରନ୍ତୁ m_fsms.at(it->second);
        ଅଚଳ size_t index = m_fsms.size();
        m_fsmIndex.emplace(stateVscp, index);
        m_fsms.emplace_back();
        ଫେରନ୍ତୁ m_fsms.back();
    }
    ଅଚଳ std::vector<DetectedFsm>& fsms() ଅଚଳ { ଫେରନ୍ତୁ m_fsms; }
};

// Detection runs while the original clocked/case structure is still intact and
// populates graph-backed FSM models without mutating the tree mid-traversal.
// This pass is intentionally conservative: for this PR we only lock down the
// small set of transition/selector forms that are already stable in the
// normalized AST we see here. The remaining reject branches are therefore
// mostly future-feature boundaries, not accidental dead code.
ଶ୍ରେଣୀ FsmDetectVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
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
    std::unordered_map<ଅଚଳ AstVarScope*, FsmCaseCandidate> m_comboPaired;
    // Continuous aliases are order-independent, while procedural aliases must
    // remain source-order scoped to avoid using assignments not yet executed.
    FsmAliasMap m_stateAliases;
    std::unordered_set<ଅଚଳ AstVarScope*> m_ambiguousStateAliases;
    // A surviving wrapper's semantic d/q relationship is split across the
    // parent scope and the child module scope. This table is the narrow bridge
    // between those scopes: only transparent port aliases are recorded, so the
    // detector does not become a general cross-module dataflow engine.
    FsmCellPortAliasMap m_cellPortAliases;
    FsmCellPortAliasMap m_cellPortChildAliases;

    // METHODS
    // Enum-backed FSMs may be wrapped in refs/typedefs; normalize to the
    // underlying enum type before deciding whether a case is a candidate.
    ସ୍ଥିର Ast_ଗଣ୍ଠି_DType* unwrapEnumCandidate(Ast_ଗଣ୍ଠି_DType* dtypep) {
        ଫେରନ୍ତୁ dtypep->skipRefToEnump();
    }

    ସ୍ଥିର string candidateConflictContext(Ast_ଗଣ୍ଠି* laterNodep,
                                           ଅଚଳ FsmCaseCandidate& firstCand) {
        ଫେରନ୍ତୁ '\n' + laterNodep->warnContextPrimary() + firstCand.warnNodep->warnOther()
               + "... Location of first supported candidate for "
               + firstCand.stateVscp->prettyNameQ() + '\n'
               + firstCand.warnNodep->warnContextSecondary();
    }

    ସ୍ଥିର ତାର୍କିକ rejectFsmWrapperCell(AstCell* cellp, ଅଚଳ string& reason) {
        cellp->v3warn(COVERIGN, "Ignoring unsupported: " + reason);
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ସ୍ଥିର ତାର୍କିକ simple_Param_State_ମୂଲ୍ୟ(AstCell* cellp, ଅଚଳ string& name, Fsm_State_ମୂଲ୍ୟ& ମୂଲ୍ୟ,
                                      Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = nullptr;
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ଅଚଳ varp = VN_CAST(stmtp, Var);
            ଯଦି (!varp || !varp->isParam() || varp->name() != name) ଜାରି;
            valuepr = VN_AS(varp->valuep(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
            ଫେରନ୍ତୁ const_ମୂଲ୍ୟ_Status(valuepr, ମୂଲ୍ୟ) == Const_ମୂଲ୍ୟ_Status::OK;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ସ୍ଥିର ତାର୍କିକ childPortInScope(AstVarScope* vscp, AstScope* parentScopep, AstCell*& cellpr) {
        ଯଦି (!vscp->varp()->isIO()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        AstScope* ଅଚଳ scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        ଯଦି (scopep->aboveScopep() != parentScopep) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ସ୍ଥିର AstVarScope* simpleAssignVarScope(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        AstVarRef* ଅଚଳ vrefp = VN_CAST(exprp, VarRef);
        ଫେରନ୍ତୁ vrefp ? vrefp->varScopep() : nullptr;
    }

    ଶୂନ୍ୟ addWrapperCell(AstScope* scopep, AstCell* cellp) {
        m_cellPortAliases.emplace(cellp, FsmCellPortMap{});
        m_cellPortChildAliases.emplace(cellp, FsmCellPortMap{});
        ଅଚଳ std::pair<AstScope*, AstCell*> item{scopep, cellp};
        ଯଦି (std::find(m_wrapperCells.cbegin(), m_wrapperCells.cend(), item)
            != m_wrapperCells.cend()) {
            ଫେରନ୍ତୁ;
        }
        m_wrapperCells.emplace_back(item);
    }

    ଶୂନ୍ୟ collectCellPortAlias(AstAssignW* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* ଅଚଳ lhsVscp = simpleAssignVarScope(nodep->lhsp());
        AstVarScope* ଅଚଳ rhsVscp = simpleAssignVarScope(nodep->rhsp());
        ଯଦି (!lhsVscp || !rhsVscp) ଫେରନ୍ତୁ;
        AstCell* cellp = nullptr;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        ଯଦି (childPortInScope(lhsVscp, m_scopep, cellp)) {
            ଯଦି (!fsmRegisterWrapperDesc(cellp)) ଫେରନ୍ତୁ;
            UASSERT_OBJ(lhsVscp->varp()->is_ନିବେଶ(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cellPortAliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cellPortChildAliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            addWrapperCell(m_scopep, cellp);
        } ଅନ୍ୟଥା ଯଦି (childPortInScope(rhsVscp, m_scopep, cellp)) {
            ଯଦି (!fsmRegisterWrapperDesc(cellp)) ଫେରନ୍ତୁ;
            UASSERT_OBJ(rhsVscp->varp()->isWritable(), nodep,
                        "Child-side port alias rhs should be writable");
            m_cellPortAliases[cellp][rhsVscp->varp()->name()] = lhsVscp;
            m_cellPortChildAliases[cellp][rhsVscp->varp()->name()] = rhsVscp;
            addWrapperCell(m_scopep, cellp);
        }
    }

    AstVarScope* roleVarScope(AstCell* cellp, ଅଚଳ string& portName) ଅଚଳ {
        // At this point explicit AstPin expressions have been lowered away, so
        // role resolution crosses the wrapper boundary only through the
        // transparent alias table above. This keeps wrapper support aligned with
        // direct-register detection instead of growing into interprocedural FSM
        // inference.
        ଅଚଳ FsmCellPortMap& ports = m_cellPortAliases.at(cellp);
        ଅଚଳ FsmCellPortMap::const_iterator portIt = ports.find(portName);
        ଫେରନ୍ତୁ portIt == ports.end() ? nullptr : portIt->second;
    }

    AstVarScope* childRoleVarScope(AstCell* cellp, ଅଚଳ string& portName) ଅଚଳ {
        ଅଚଳ FsmCellPortMap& ports = m_cellPortChildAliases.at(cellp);
        ଅଚଳ FsmCellPortMap::const_iterator portIt = ports.find(portName);
        ଫେରନ୍ତୁ portIt == ports.end() ? nullptr : portIt->second;
    }

    ଶ୍ରେଣୀ RegisterAlwaysAnalyzer final {
        AstScope* ଅଚଳ m_scopep;  // Scope containing the always block(s) being analyzed

    ସର୍ବସାଧାରଣ:
        explicit RegisterAlwaysAnalyzer(AstScope* scopep)
            : m_scopep{scopep} {}

        std::vector<std::pair<AstCase*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>>
        oneBlockCandidates(AstAlways* alwaysp) ଅଚଳ {
            std::vector<std::pair<AstCase*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>> candidates;
            Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = alwaysp->stmtsp();
            ଯଦି (AstIf* ଅଚଳ firstIfp = VN_CAST(stmtsp, If)) {
                ଯଦି (AstCase* ଅଚଳ casep = VN_CAST(firstIfp->elsesp(), Case)) {
                    candidates.emplace_back(casep,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                ଯଦି (AstCase* ଅଚଳ casep = VN_CAST(nodep, Case))
                    candidates.emplace_back(casep, nullptr);
            }
            ଫେରନ୍ତୁ candidates;
        }

        std::vector<std::pair<AstIf*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>>
        oneBlockIfCandidates(AstAlways* alwaysp) ଅଚଳ {
            std::vector<std::pair<AstIf*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>> candidates;
            Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            ଯଦି (AstIf* ଅଚଳ firstIfp = VN_CAST(stmtsp, If)) {
                ଯଦି (AstIf* ଅଚଳ chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(firstIfp->elsesp()), If)) {
                    candidates.emplace_back(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                ଯଦି (AstIf* ଅଚଳ ifp = VN_CAST(nodep, If)) candidates.emplace_back(ifp, nullptr);
            }
            ଫେରନ୍ତୁ candidates;
        }

        ତାର୍କିକ matchRegisterCandidate(AstAlways* alwaysp, FsmRegisterCandidate& cand) ଅଚଳ {
            ଫେରନ୍ତୁ FsmDetectVisitor::matchRegisterAlways(alwaysp, m_scopep, cand);
        }

        ଶୂନ୍ୟ buildOneBlockCandidate(AstAlways* alwaysp, AstVarScope* vscp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* resetCondp,
                                    FsmRegisterCandidate& reg) ଅଚଳ {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.nextVscp(vscp);
            reg.senses() = FsmDetectVisitor::describe_Sen_ବୃକ୍ଷ(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != nullptr);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* ଅଚଳ firstIfp = VN_CAST(alwaysp->stmtsp(), If);
            ଯଦି (firstIfp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = nullptr;
                ଅଚଳ ResetAssignStatus resetStatus = FsmDetectVisitor::collectConstStateAssigns(
                    firstIfp->thensp(), resetStateVscp, reg.resetArcs());
                ଯଦି (resetStatus == ResetAssignStatus::EMPTY) {
                    reg.resetArcs().clear();
                } ଅନ୍ୟଥା ଯଦି (resetStatus == ResetAssignStatus::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().clear();
                    Fsm_State_ମୂଲ୍ୟ reset_ମୂଲ୍ୟ;
                    Ast_ଗଣ୍ଠି* ଅଚଳ thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(firstIfp->thensp());
                    UASSERT_OBJ(thenNodep, firstIfp,
                                "one-block reset fallback requires a non-empty reset branch");
                    Ast_ଗଣ୍ଠି_Assign* ଅଚଳ resetAssp = FsmDetectVisitor::direct_Const_State_Assign_ଗଣ୍ଠି(
                        thenNodep, resetStateVscp, reset_ମୂଲ୍ୟ);
                    ଯଦି (resetAssp && resetStateVscp == vscp) {
                        reg.resetArcs().emplace_back(reset_ମୂଲ୍ୟ, resetAssp);
                    }
                } ଅନ୍ୟଥା ଯଦି (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                    reg.resetArcs().clear();
                }
            }
        }
    };

    ତାର୍କିକ matchFsmWrapperCell(AstScope* scopep, AstCell* cellp, FsmRegisterCandidate& cand) ଅଚଳ {
        FsmWrapperRoles roles = rolesFromDesc(*fsmRegisterWrapperDesc(cellp));

        AstVarScope* ଅଚଳ nextVscp = roleVarScope(cellp, roles.dPort);
        AstVarScope* ଅଚଳ stateVscp = roleVarScope(cellp, roles.qPort);
        ଯଦି (!nextVscp || !stateVscp) {
            ଫେରନ୍ତୁ rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* ଅଚଳ clkVscp = roleVarScope(cellp, roles.clkPort);
        ଯଦି (!clkVscp) {
            ଫେରନ୍ତୁ rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper instance requires a simple clock connection");
        }

        FsmSenDesc clkSense;
        clkSense.edgeType = VEdgeType::ET_POSEDGE;
        clkSense.varScopep = clkVscp;
        cand.senses().ଠେଲ_back(clkSense);

        AstVarScope* resetVscp = nullptr;
        ଯଦି (!roles.rstPort.empty()) resetVscp = roleVarScope(cellp, roles.rstPort);
        ଯଦି (resetVscp) {
            // The descriptor identifies the reset port but not its polarity. Use
            // the wrapper's own event control AST as the contract for sampling
            // the connected parent signal.
            ତାର୍କିକ inferred_Active_ତଳ = ମିଥ୍ୟା;
            ଯଦି (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rstPort, inferred_Active_ତଳ)) {
                roles.has_Rst_Active_ତଳ = ସତ୍ୟ;
                roles.rst_Active_ତଳ = inferred_Active_ତଳ;
            }
        }

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* resetValuep = nullptr;
        Fsm_State_ମୂଲ୍ୟ reset_ମୂଲ୍ୟ;
        ଅଚଳ ତାର୍କିକ has_Reset_ମୂଲ୍ୟ
            = !roles.rstValParam.empty()
              && simple_Param_State_ମୂଲ୍ୟ(cellp, roles.rstValParam, reset_ମୂଲ୍ୟ, resetValuep);
        ଯଦି (resetVscp && roles.has_Rst_Active_ତଳ && has_Reset_ମୂଲ୍ୟ) {
            FsmSenDesc rstSense;
            rstSense.edgeType = roles.rst_Active_ତଳ ? VEdgeType::ET_NEGEDGE : VEdgeType::ET_POSEDGE;
            rstSense.varScopep = resetVscp;
            cand.senses().ଠେଲ_back(rstSense);
            cand.resetCond().varScopep = resetVscp;
            cand.resetCond().active_ତଳ = roles.rst_Active_ତଳ;
            cand.hasResetCond(ସତ୍ୟ);
            cand.resetArcs().emplace_back(reset_ମୂଲ୍ୟ, cellp, resetValuep);
        } ଅନ୍ୟଥା ଯଦି (!roles.rstPort.empty() || !roles.rstValParam.empty()) {
            string reason;
            ଯଦି (roles.rstPort.empty()) {
                reason = "reset port is not configured";
            } ଅନ୍ୟଥା ଯଦି (!resetVscp) {
                reason = "reset connection is missing or not a simple variable";
            } ଅନ୍ୟଥା ଯଦି (!roles.has_Rst_Active_ତଳ) {
                reason = "reset polarity could not be inferred from the wrapper";
            } ଅନ୍ୟଥା ଯଦି (roles.rstValParam.empty()) {
                reason = "reset_value parameter is not configured";
            } ଅନ୍ୟଥା {
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
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ଶ୍ରେଣୀ ComboAlwaysAnalyzer final {
    ସର୍ବସାଧାରଣ:
        ସଂରଚନା ComboMatch final {
            ଅଚଳ FsmRegisterCandidate* matchedp = nullptr;
            Ast_ଗଣ୍ଠି* warnNodep = nullptr;
        };

    ବ୍ୟକ୍ତିଗତ:
        ଅଚଳ std::vector<FsmRegisterCandidate>& m_registerCandidates;

    ସର୍ବସାଧାରଣ:
        explicit ComboAlwaysAnalyzer(ଅଚଳ std::vector<FsmRegisterCandidate>& registerCandidates)
            : m_registerCandidates{registerCandidates} {}

        ComboMatch matchCase(Ast_ଗଣ୍ଠି* stmtsp, AstCase* casep) ଅଚଳ {
            ComboMatch match;
            AstVarRef* ଅଚଳ selp = VN_CAST(casep->exprp(), VarRef);
            ଯଦି (!selp) ଫେରନ୍ତୁ match;
            ପାଇଁ (ଅଚଳ FsmRegisterCandidate& reg : m_registerCandidates) {
                ଯଦି (selp->varScopep() == reg.nextVscp()) {
                    ଯଦି (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        ଜାରି;
                    }
                } ଅନ୍ୟଥା ଯଦି (selp->varScopep() != reg.stateVscp()) {
                    ଜାରି;
                }
                Ast_ଗଣ୍ଠି* ଅଚଳ warnNodep = FsmDetectVisitor::case_Supported_Transition_ଗଣ୍ଠି(
                    casep, reg.nextVscp(), reg.inclCond());
                ଯଦି (!warnNodep) ଜାରି;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            ଫେରନ୍ତୁ match;
        }

        ComboMatch matchIfChain(Ast_ଗଣ୍ଠି* stmtsp, ଅଚଳ FsmIfChainCandidate& chain) ଅଚଳ {
            ComboMatch match;
            ପାଇଁ (ଅଚଳ FsmRegisterCandidate& reg : m_registerCandidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                ଯଦି (chain.compareVscp == reg.nextVscp()) {
                    ଯଦି (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.nextVscp())) {
                        ଜାରି;
                    }
                } ଅନ୍ୟଥା ଯଦି (chain.compareVscp != reg.stateVscp()) {
                    ଜାରି;
                }
                Ast_ଗଣ୍ଠି* ଅଚଳ warnNodep
                    = FsmDetectVisitor::if_Chain_Supported_Transition_ଗଣ୍ଠି(chain, reg.nextVscp());
                ଯଦି (!warnNodep) ଜାରି;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            ଫେରନ୍ତୁ match;
        }

        ତାର୍କିକ shouldWarnUnsupported(Ast_ଗଣ୍ଠି* stmtsp, AstCase* casep) ଅଚଳ {
            ଅଚଳ AstVarRef* ଅଚଳ selp = VN_CAST(casep->exprp(), VarRef);
            ଯଦି (!selp) ଫେରନ୍ତୁ ମିଥ୍ୟା;

            ଫେରନ୍ତୁ std::any_of(
                m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                [&](ଅଚଳ FsmRegisterCandidate& reg) -> ତାର୍କିକ {
                    ଅଚଳ ତାର୍କିକ matchesNext = selp->varScopep() == reg.nextVscp();
                    ଅଚଳ ତାର୍କିକ matchesState = selp->varScopep() == reg.stateVscp();

                    ଯଦି (!matchesNext && !matchesState) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                    ଯଦି (matchesNext
                        && !FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        ଫେରନ୍ତୁ ମିଥ୍ୟା;
                    }
                    ଫେରନ୍ତୁ FsmDetectVisitor::case_Supported_Transition_ଗଣ୍ଠି(casep, reg.nextVscp(),
                                                                         reg.inclCond());
                });
        }
    };

    // Reset arcs are only modeled for the simple signal form that survives to
    // this pass after earlier normalization.
    ସ୍ଥିର ତାର୍କିକ isSimpleResetCond(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp) { ଫେରନ୍ତୁ VN_IS(condp, VarRef); }

    // Normalize the reset condition into a compact description so the lowering
    // phase can regenerate the same predicate after detection. By the time
    // this pass runs, active-low source forms such as "!rst_n" have already
    // been canonicalized to a positive-condition if/else shape, so only a
    // plain VarRef survives here.
    ସ୍ଥିର FsmResetCondDesc describeResetCond(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp) {
        FsmResetCondDesc desc;
        ଯଦି (AstVarRef* ଅଚଳ vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        ଫେରନ୍ତୁ desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    ସ୍ଥିର std::vector<FsmSenDesc> describe_Sen_ବୃକ୍ଷ(Ast_Sen_ବୃକ୍ଷ* sentreep) {
        std::vector<FsmSenDesc> senses;
        ପାଇଁ (AstSenItem* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), SenItem)) {
            Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ vrefp = itemp->varrefp();
            ଯଦି (!vrefp) ଜାରି;
            FsmSenDesc desc;
            desc.edgeType = itemp->edgeType().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.ଠେଲ_back(desc);
        }
        ଫେରନ୍ତୁ senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    ସ୍ଥିର ତାର୍କିକ isIgnorableStmt(Ast_ଗଣ୍ଠି* nodep) { ଫେରନ୍ତୁ VN_IS(nodep, CoverInc); }

    ସ୍ଥିର Ast_ଗଣ୍ଠି* skipLeadingIgnorableStmt(Ast_ଗଣ୍ଠି* nodep) {
        ଯେତେବେଳେ (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        ଫେରନ୍ତୁ nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    ସ୍ଥିର Ast_ଗଣ୍ଠି* singleMeaningfulStmt(Ast_ଗଣ୍ଠି* stmtp) {
        Ast_ଗଣ୍ଠି* resultp = nullptr;
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            ଯଦି (isIgnorableStmt(nodep)) ଜାରି;
            ଯଦି (resultp) ଫେରନ୍ତୁ nullptr;
            resultp = nodep;
        }
        ଫେରନ୍ତୁ resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    ସ୍ଥିର Ast_ଗଣ୍ଠି* singleMeaningfulBranch(Ast_ଗଣ୍ଠି* branchp) {
        ଯଦି (!branchp) ଫେରନ୍ତୁ nullptr;
        ଫେରନ୍ତୁ branchp;
    }

    // By fsm-detect time, non-clocked always @* blocks are already admitted through
    // a missing sentree. This helper therefore only needs to recognize
    // explicit changed-sensitivity lists such as always @(a or b); clocked and
    // event-driven forms remain out of scope.
    ସ୍ଥିର ତାର୍କିକ isPlainComboSentree(ଅଚଳ Ast_Sen_ବୃକ୍ଷ* sentreep) {
        UASSERT(sentreep, "plain combo sensitivity check requires a sensitivity tree");
        ପାଇଁ (ଅଚଳ AstSenItem* senp = sentreep->sensesp(); senp;
             senp = VN_AS(senp->nextp(), SenItem)) {
            ଯଦି (senp->edgeType() == VEdgeType::ET_CHANGED) ଜାରି;
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ଶୂନ୍ୟ warnUnsupportedComboAlways(ଅଚଳ FsmComboAlways& combo) {
        ଅଚଳ ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        ତାର୍କିକ warned = ମିଥ୍ୟା;
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ଅଚଳ casep = VN_CAST(nodep, Case);
            ଯଦି (!casep) ଜାରି;
            ଯଦି (analyzer.shouldWarnUnsupported(stmtsp, casep)) {
                casep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-clocked always "
                                        "blocks requires a combinational sensitivity list or "
                                        "always_comb");
                warned = ସତ୍ୟ;
            }
            ଯଦି (warned) ଭାଙ୍ଗ;
        }
    }

    // Case-item bodies are single subtrees like if/else arms, not statement
    // lists, so unwrap only local begin/end wrappers here rather than walking
    // sibling case items via nextp().
    ସ୍ଥିର Ast_ଗଣ୍ଠି_Assign* directStateAssign(Ast_ଗଣ୍ଠି* stmtp, AstVarScope* stateVscp) {
        Ast_ଗଣ୍ଠି* ଅଚଳ nodep = singleMeaningfulBranch(stmtp);
        ଯଦି (!nodep) ଫେରନ୍ତୁ nullptr;
        Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assp = VN_CAST(nodep, ଗଣ୍ଠି_Assign);
        ଯଦି (!assp) ଫେରନ୍ତୁ nullptr;
        AstVarRef* ଅଚଳ vrefp = VN_CAST(assp->lhsp(), VarRef);
        ଯଦି (!vrefp || vrefp->varScopep() != stateVscp) ଫେରନ୍ତୁ nullptr;
        ଫେରନ୍ତୁ assp;
    }

    ସ୍ଥିର Ast_ଗଣ୍ଠି_Assign* ଗଣ୍ଠି_State_Var_Assign(Ast_ଗଣ୍ଠି* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assp = VN_CAST(nodep, ଗଣ୍ଠି_Assign);
        ଯଦି (!assp) ଫେରନ୍ତୁ nullptr;
        AstVarRef* ଅଚଳ lhsp = VN_CAST(assp->lhsp()->baseFromp(ସତ୍ୟ), VarRef);
        AstVarRef* ଅଚଳ rhsp = VN_CAST(assp->rhsp(), VarRef);
        ଯଦି (!rhsp || !lhsp) ଫେରନ୍ତୁ nullptr;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        ଫେରନ୍ତୁ assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    ସ୍ଥିର Ast_ଗଣ୍ଠି_Assign* directCondStateVarAssign(Ast_ଗଣ୍ଠି* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*& condp,
                                                   Fsm_State_ମୂଲ୍ୟ& reset_ମୂଲ୍ୟ) {
        Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assp = VN_CAST(nodep, ଗଣ୍ଠି_Assign);
        ଯଦି (!assp) ଫେରନ୍ତୁ nullptr;
        AstVarRef* ଅଚଳ lhsp = VN_CAST(assp->lhsp()->baseFromp(ସତ୍ୟ), VarRef);
        AstCond* ଅଚଳ rhsp = VN_CAST(assp->rhsp(), Cond);
        ଯଦି (!rhsp || !lhsp) ଫେରନ୍ତୁ nullptr;
        AstVarRef* ଅଚଳ elsep = VN_CAST(rhsp->elsep(), VarRef);
        ଯଦି (!elsep) ଫେରନ୍ତୁ nullptr;
        ଯଦି (const_ମୂଲ୍ୟ_Status(rhsp->thenp(), reset_ମୂଲ୍ୟ) != Const_ମୂଲ୍ୟ_Status::OK) ଫେରନ୍ତୁ nullptr;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        ଫେରନ୍ତୁ assp;
    }

    ସ୍ଥିର Ast_ଗଣ୍ଠି_Assign* direct_Const_State_Assign_ଗଣ୍ଠି(Ast_ଗଣ୍ଠି* nodep, AstVarScope*& stateVscp,
                                                     Fsm_State_ମୂଲ୍ୟ& ମୂଲ୍ୟ) {
        Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assp = VN_CAST(nodep, ଗଣ୍ଠି_Assign);
        ଯଦି (!assp) ଫେରନ୍ତୁ nullptr;
        AstVarRef* ଅଚଳ lhsp = VN_CAST(assp->lhsp()->baseFromp(ସତ୍ୟ), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        ଯଦି (const_ମୂଲ୍ୟ_Status(assp->rhsp(), ମୂଲ୍ୟ) != Const_ମୂଲ୍ୟ_Status::OK) ଫେରନ୍ତୁ nullptr;
        stateVscp = lhsp->varScopep();
        ଫେରନ୍ତୁ assp;
    }

    ଗଣନା ଶ୍ରେଣୀ ResetAssignStatus : uint8_t {
        EMPTY,  // Reset branch had no non-coverage statements.
        NONE,  // Reset branch was not the supported direct-constant shape.
        SINGLE,  // Exactly one supported reset assignment was collected.
        MULTI_SAME_STATE  // Multiple assignments to the same FSM state var; warn and ignore.
    };

    // Reset arcs are only extracted from the single direct-constant form. If
    // user RTL assigns the same state register multiple times in the reset
    // branch, warn and skip reset-arc modeling rather than inventing multiple
    // reset transitions for an odd but legal coding style.
    ସ୍ଥିର ResetAssignStatus collectConstStateAssigns(Ast_ଗଣ୍ଠି* stmtp, AstVarScope*& stateVscp,
                                                      std::vector<FsmResetArcDesc>& resetArcs) {
        Ast_ଗଣ୍ଠି* nodep = skipLeadingIgnorableStmt(stmtp);
        ଯଦି (!nodep) ଫେରନ୍ତୁ ResetAssignStatus::EMPTY;
        ପାଇଁ (;; nodep = nodep->nextp()) {
            AstVarScope* assignStateVscp = nullptr;
            Fsm_State_ମୂଲ୍ୟ ମୂଲ୍ୟ;
            Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assp = direct_Const_State_Assign_ଗଣ୍ଠି(nodep, assignStateVscp, ମୂଲ୍ୟ);
            ଯଦି (!assp) ଫେରନ୍ତୁ ResetAssignStatus::NONE;
            ଯଦି (!stateVscp) stateVscp = assignStateVscp;
            ଯଦି (assignStateVscp != stateVscp) ଫେରନ୍ତୁ ResetAssignStatus::NONE;
            ଯଦି (!resetArcs.empty()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.clear();
                ଫେରନ୍ତୁ ResetAssignStatus::MULTI_SAME_STATE;
            }
            resetArcs.emplace_back(ମୂଲ୍ୟ, assp);
            ଯଦି (!nodep->nextp()) ଫେରନ୍ତୁ ResetAssignStatus::SINGLE;
        }
    }

    ସ୍ଥିର ତାର୍କିକ hasCanonicalNextStateDefaultBeforeCase(Ast_ଗଣ୍ଠି* stmtsp, Ast_ଗଣ୍ଠି* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* nextVscp) {
        Ast_ଗଣ୍ଠି* ଅଚଳ bodyp = skipLeadingIgnorableStmt(stmtsp);
        ତାର୍କିକ sawCanonicalDefault = ମିଥ୍ୟା;
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            ଯଦି (nodep == targetp) ଫେରନ୍ତୁ sawCanonicalDefault;
            ଯଦି (Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assp = VN_CAST(nodep, ଗଣ୍ଠି_Assign)) {
                AstVarRef* ଅଚଳ lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* ଅଚଳ rhsp = VN_CAST(assp->rhsp(), VarRef);
                ଯଦି (!lhsp || lhsp->varScopep() != nextVscp) ଜାରି;
                ଯଦି (sawCanonicalDefault) {
                    ଅଚଳ string nextName = nextVscp->varp()->prettyNameQ();
                    ଅଚଳ string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + nextName
                                     + ") when the canonical " + nextName + " = " + stateName
                                     + " default is overwritten before the case statement");
                    ଫେରନ୍ତୁ ମିଥ୍ୟା;
                }
                ଯଦି (!rhsp || rhsp->varScopep() != stateVscp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                sawCanonicalDefault = ସତ୍ୟ;
            }
        }
    }

    ସ୍ଥିର ତାର୍କିକ ifStateConstAssign(Ast_ଗଣ୍ଠି* stmtp, AstVarScope* stateVscp,
                                   Fsm_State_ମୂଲ୍ୟ& then_ମୂଲ୍ୟ, Fsm_State_ମୂଲ୍ୟ& else_ମୂଲ୍ୟ) {
        AstIf* ଅଚଳ ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        ଯଦି (!ifp || !ifp->elsesp()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        AstVarScope* thenVscp = nullptr;
        AstVarScope* elseVscp = nullptr;
        Ast_ଗଣ୍ଠି* ଅଚଳ thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        Ast_ଗଣ୍ଠି* ଅଚଳ elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        ଯଦି (!thenNodep || !elseNodep) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!direct_Const_State_Assign_ଗଣ୍ଠି(thenNodep, thenVscp, then_ମୂଲ୍ୟ)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!direct_Const_State_Assign_ଗଣ୍ଠି(elseNodep, elseVscp, else_ମୂଲ୍ୟ)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (thenVscp == stateVscp && elseVscp == stateVscp) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (thenVscp != elseVscp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        Ast_ଗଣ୍ଠି* ଅଚଳ followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = nullptr;
        AstVarScope* finalFromVscp = nullptr;
        Ast_ଗଣ୍ଠି* ଅଚଳ finalNodep = singleMeaningfulBranch(followp);
        ଯଦି (!finalNodep) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!ଗଣ୍ଠି_State_Var_Assign(finalNodep, finalStateVscp, finalFromVscp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (finalStateVscp != stateVscp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (finalFromVscp != thenVscp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ସ୍ଥିର ତାର୍କିକ directStateCondConstAssign(Ast_ଗଣ୍ଠି* stmtp, AstVarScope* stateVscp,
                                           Fsm_State_ମୂଲ୍ୟ& then_ମୂଲ୍ୟ, Fsm_State_ମୂଲ୍ୟ& else_ମୂଲ୍ୟ) {
        Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assp = directStateAssign(stmtp, stateVscp);
        ଯଦି (!assp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        AstCond* ଅଚଳ condp = VN_CAST(assp->rhsp(), Cond);
        ଯଦି (!condp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ const_ମୂଲ୍ୟ_Status(condp->thenp(), then_ମୂଲ୍ୟ) == Const_ମୂଲ୍ୟ_Status::OK
               && const_ମୂଲ୍ୟ_Status(condp->elsep(), else_ମୂଲ୍ୟ) == Const_ମୂଲ୍ୟ_Status::OK;
    }

    ସ୍ଥିର Ast_ଗଣ୍ଠି* case_Item_Supported_Arc_ଗଣ୍ଠି(AstCaseItem* itemp, AstVarScope* stateVscp,
                                             ତାର୍କିକ inclCond) {
        ଯଦି (itemp->isDefault()) {
            ଯଦି (!inclCond) ଫେରନ୍ତୁ nullptr;
        }
        Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assp = directStateAssign(itemp->stmtsp(), stateVscp);
        ଯଦି (assp) {
            Fsm_State_ମୂଲ୍ୟ to_ମୂଲ୍ୟ;
            ଯଦି (const_ମୂଲ୍ୟ_Status(assp->rhsp(), to_ମୂଲ୍ୟ) == Const_ମୂଲ୍ୟ_Status::OK) ଫେରନ୍ତୁ assp;
        }
        Fsm_State_ମୂଲ୍ୟ then_ମୂଲ୍ୟ;
        Fsm_State_ମୂଲ୍ୟ else_ମୂଲ୍ୟ;
        ଯଦି (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_ମୂଲ୍ୟ, else_ମୂଲ୍ୟ)) {
            ଫେରନ୍ତୁ assp;
        }
        ଯଦି (ifStateConstAssign(itemp->stmtsp(), stateVscp, then_ମୂଲ୍ୟ, else_ମୂଲ୍ୟ)) {
            ଫେରନ୍ତୁ singleMeaningfulBranch(itemp->stmtsp());
        }
        ଫେରନ୍ତୁ nullptr;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    ସ୍ଥିର Ast_ଗଣ୍ଠି* case_Supported_Transition_ଗଣ୍ଠି(AstCase* casep, AstVarScope* stateVscp,
                                                ତାର୍କିକ inclCond) {
        ପାଇଁ (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ nodep = case_Item_Supported_Arc_ଗଣ୍ଠି(itemp, stateVscp, inclCond))
                ଫେରନ୍ତୁ nodep;
        }
        ଫେରନ୍ତୁ nullptr;
    }

    ସ୍ଥିର Ast_ଗଣ୍ଠି* case_Item_Supported_Arc_ଗଣ୍ଠି_Like(Ast_ଗଣ୍ଠି* stmtsp, AstVarScope* stateVscp) {
        ଯଦି (Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assp = directStateAssign(stmtsp, stateVscp)) {
            Fsm_State_ମୂଲ୍ୟ to_ମୂଲ୍ୟ;
            ଯଦି (const_ମୂଲ୍ୟ_Status(assp->rhsp(), to_ମୂଲ୍ୟ) == Const_ମୂଲ୍ୟ_Status::OK) ଫେରନ୍ତୁ assp;
            Fsm_State_ମୂଲ୍ୟ then_ମୂଲ୍ୟ;
            Fsm_State_ମୂଲ୍ୟ else_ମୂଲ୍ୟ;
            ଯଦି (directStateCondConstAssign(stmtsp, stateVscp, then_ମୂଲ୍ୟ, else_ମୂଲ୍ୟ)) ଫେରନ୍ତୁ assp;
        }
        ଫେରନ୍ତୁ nullptr;
    }

    ସ୍ଥିର Ast_ଗଣ୍ଠି* if_Chain_Supported_Transition_ଗଣ୍ଠି(ଅଚଳ FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        ପାଇଁ (size_t i = 0; i < chain.branches.size(); ++i) {
            ଅଚଳ FsmIfBranch& branch = chain.branches[i];
            Ast_ଗଣ୍ଠି* ଅଚଳ nodep = case_Item_Supported_Arc_ଗଣ୍ଠି_Like(branch.stmtsp, stateVscp);
            ଯଦି (!nodep) ଫେରନ୍ତୁ nullptr;
        }
        ଫେରନ୍ତୁ chain.branches.front().ifp;
    }

    // Prefer user labels in reports. Forced non-enum FSMs prepopulate synthetic
    // labels, so all emitted arcs should already have a known label here.
    ସ୍ଥିର string label_For_ମୂଲ୍ୟ(
        ଅଚଳ std::unordered_map<Fsm_State_ମୂଲ୍ୟ, StateConstLabel, Fsm_State_ମୂଲ୍ୟ_Hash>& labels,
        ଅଚଳ Fsm_State_ମୂଲ୍ୟ& ମୂଲ୍ୟ) {
        ଫେରନ୍ତୁ labels.at(ମୂଲ୍ୟ).text;
    }

    // The extractor only models constant-valued state transitions, and by the
    // time detect runs those values have already been constant-folded.
    ଗଣନା ଶ୍ରେଣୀ Const_ମୂଲ୍ୟ_Status : uint8_t { OK, NOT_CONST, XZ };

    ସ୍ଥିର Const_ମୂଲ୍ୟ_Status const_ମୂଲ୍ୟ_Status(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Fsm_State_ମୂଲ୍ୟ& ମୂଲ୍ୟ) {
        ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(exprp, Const);
        ଯଦି (!constp) ଫେରନ୍ତୁ Const_ମୂଲ୍ୟ_Status::NOT_CONST;
        ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ = constp->ଅଙ୍କ();
        ଯଦି (ଅଙ୍କ.isAnyXZ()) ଫେରନ୍ତୁ Const_ମୂଲ୍ୟ_Status::XZ;
        ମୂଲ୍ୟ = Fsm_State_ମୂଲ୍ୟ{ଅଙ୍କ};
        ଫେରନ୍ତୁ Const_ମୂଲ୍ୟ_Status::OK;
    }

    ସ୍ଥିର ତାର୍କିକ pureStateComparisonNoAlias(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, FsmStateComparison& cmp) {
        AstEq* ଅଚଳ eqp = VN_CAST(exprp, Eq);
        ଯଦି (!eqp) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep = eqp->rhsp();
        ଯଦି (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->baseFromp(ସତ୍ୟ), VarRef);
            ଯଦି (!vrefp) { ଫେରନ୍ତୁ ମିଥ୍ୟା; }
            valuep = eqp->lhsp();
        }

        Fsm_State_ମୂଲ୍ୟ ମୂଲ୍ୟ;
        ଯଦି (const_ମୂଲ୍ୟ_Status(valuep, ମୂଲ୍ୟ) != Const_ମୂଲ୍ୟ_Status::OK) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        cmp.stateVscp = vrefp->varScopep();
        cmp.valuep = valuep;
        cmp.ମୂଲ୍ୟ = ମୂଲ୍ୟ;
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ସ୍ଥିର ତାର୍କିକ pureStateComparison(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ଅଚଳ FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        ଯଦି (pureStateComparisonNoAlias(exprp, cmp)) ଫେରନ୍ତୁ ସତ୍ୟ;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        ଯଦି (AstVarRef* ଅଚଳ vrefp = VN_CAST(exprp, VarRef)) {
            ଅଚଳ FsmAliasMap::const_iterator it = aliases.find(vrefp->varScopep());
            ଯଦି (it == aliases.end()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            cmp = it->second;
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ସ୍ଥିର ତାର୍କିକ supported_ଶୀର୍ଷ_Level_Guard(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        ଯଦି (VN_IS(exprp, Or)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (VN_IS(exprp, RedAnd)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (VN_IS(exprp, RedOr)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (VN_IS(exprp, RedXor)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ସ୍ଥିର ତାର୍କିକ resolveIfPredicate(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ଅଚଳ FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, ତାର୍କିକ& hasGuard) {
        std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> terms;
        std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> pending;
        pending.ଠେଲ_back(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        ଯେତେବେଳେ (!pending.empty()) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ nodep = pending.back();
            pending.କାଢ଼_back();
            ଯଦି (AstAnd* ଅଚଳ andp = VN_CAST(nodep, And)) {
                pending.ଠେଲ_back(andp->rhsp());
                pending.ଠେଲ_back(andp->lhsp());
            } ଅନ୍ୟଥା {
                terms.ଠେଲ_back(nodep);
            }
        }

        ତାର୍କିକ sawComparison = ମିଥ୍ୟା;
        ପାଇଁ (size_t i = 0; i < terms.size(); ++i) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ termp = terms[i];
            FsmStateComparison termCmp;
            ଯଦି (pureStateComparison(termp, aliases, termCmp /*ref*/)) {
                ଯଦି (sawComparison) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                cmp = termCmp;
                sawComparison = ସତ୍ୟ;
                ଜାରି;
            }
            ଯଦି (!supported_ଶୀର୍ଷ_Level_Guard(termp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            hasGuard = ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ sawComparison;
    }

    ସ୍ଥିର ଶୂନ୍ୟ addAlias(FsmAliasMap& aliases, std::unordered_set<ଅଚଳ AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, ଅଚଳ FsmStateComparison& cmp) {
        ଯଦି (ambiguous.find(aliasVscp) != ambiguous.end()) ଫେରନ୍ତୁ;
        ଅଚଳ FsmAliasMap::iterator it = aliases.find(aliasVscp);
        ଯଦି (it == aliases.end()) {
            aliases.emplace(aliasVscp, cmp);
            ଫେରନ୍ତୁ;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        ଯଦି (it->second.stateVscp == cmp.stateVscp && it->second.ମୂଲ୍ୟ == cmp.ମୂଲ୍ୟ) ଫେରନ୍ତୁ;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        ଫେରନ୍ତୁ;
    }

    ସ୍ଥିର ଶୂନ୍ୟ collectAliasFromAssign(Ast_ଗଣ୍ଠି_Assign* assp, FsmAliasMap& aliases,
                                       std::unordered_set<ଅଚଳ AstVarScope*>& ambiguous) {
        AstVarRef* ଅଚଳ lhsp = VN_CAST(assp->lhsp(), VarRef);
        ଯଦି (!lhsp) ଫେରନ୍ତୁ;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        ଯଦି (!pureStateComparisonNoAlias(assp->rhsp(), cmp /*ref*/)) ଫେରନ୍ତୁ;
        addAlias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(Ast_ଗଣ୍ଠି* stmtsp, Ast_ଗଣ୍ଠି* limitp) ଅଚଳ {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_set<ଅଚଳ AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            ଯଦି (Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assp = VN_CAST(nodep, ଗଣ୍ଠି_Assign)) {
                collectAliasFromAssign(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        ପାଇଁ (ଅଚଳ AstVarScope* ଅଚଳ vscp : ambiguous) aliases.erase(vscp);
        ଫେରନ୍ତୁ aliases;
    }

    ସ୍ଥିର ତାର୍କିକ collectIfChain(AstIf* ifp, ଅଚଳ FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_set<string> seen_ମୂଲ୍ୟଗୁଡ଼ିକ;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        ଯେତେବେଳେ (ସତ୍ୟ) {
            FsmStateComparison cmp;
            ତାର୍କିକ hasGuard = ମିଥ୍ୟା;
            ଯଦି (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଯଦି (chain.compareVscp && chain.compareVscp != cmp.stateVscp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଯଦି (!seen_ମୂଲ୍ୟଗୁଡ଼ିକ.ଭର୍ତ୍ତି(cmp.ମୂଲ୍ୟ.ଚାବି()).second) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.ଠେଲ_back(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.ମୂଲ୍ୟ, hasGuard});

            Ast_ଗଣ୍ଠି* ଅଚଳ elseNodep
                = singleMeaningfulBranch(skipLeadingIgnorableStmt(curp->elsesp()));
            ଯଦି (!elseNodep) ଭାଙ୍ଗ;
            ଯଦି (AstIf* ଅଚଳ elseIfp = VN_CAST(elseNodep, If)) {
                curp = elseIfp;
                ଜାରି;
            }
            chain.defaultStmtsp = elseNodep;
            ଭାଙ୍ଗ;
        }
        ଫେରନ୍ତୁ chain.branches.size() >= 2;
    }

    // Enum-backed FSMs should only use values that were interned as known states.
    // If a constant transition references some other encoding, warn and skip FSM
    // instrumentation for that edge rather than silently dropping it or turning
    // optional coverage into a hard compile failure.
    ସ୍ଥିର ତାର୍କିକ validate_Known_State_ମୂଲ୍ୟ(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ FsmStateSpace& stateSpace,
                                        ଅଚଳ Fsm_State_ମୂଲ୍ୟ& ମୂଲ୍ୟ, ଅଚଳ string& role) {
        ଯଦି (stateSpace.labels.find(ମୂଲ୍ୟ) != stateSpace.labels.end()) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (stateSpace.enumBacked) {
            ଅଚଳ string enumRole = role == "source" ? "case item value" : "assigned value";
            nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on enum state variable "
                                        + stateSpace.stateVarp->prettyNameQ() + ": " + enumRole
                                        + " " + ମୂଲ୍ୟ.warnText()
                                        + " is not present in the declared enum");
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum state variable "
                                    + stateSpace.stateVarp->prettyNameQ() + ": " + role + " value "
                                    + ମୂଲ୍ୟ.warnText()
                                    + " is not present in the inferred state space");
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ସ୍ଥିର StateConstLabel stateLabelForConst(AstConst* constp) {
        ଅଚଳ string name = constp->origParamName();
        ଯଦି (!name.empty()) ଫେରନ୍ତୁ StateConstLabel{Ast_ଗଣ୍ଠି::prettyName(name), ସତ୍ୟ, 0};
        ଫେରନ୍ତୁ StateConstLabel{constp->name(), ମିଥ୍ୟା, 0};
    }

    ସ୍ଥିର ଶୂନ୍ୟ updateStateLabel(FsmStateSpace& stateSpace, ଅଚଳ Fsm_State_ମୂଲ୍ୟ& ମୂଲ୍ୟ,
                                 ଅଚଳ StateConstLabel& label) {
        stateSpace.states.at(stateSpace.labels.at(ମୂଲ୍ୟ).stateIndex).first = label.text;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    ସ୍ଥିର ତାର୍କିକ matchRegisterAlways(AstAlways* alwaysp, AstScope* scopep,
                                    FsmRegisterCandidate& cand) {
        ଯଦି (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        Ast_ଗଣ୍ଠି* ଅଚଳ nodep = singleMeaningfulStmt(stmtsp);
        ଯଦି (!nodep) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        AstVarScope* stateVscp = nullptr;
        AstVarScope* nextVscp = nullptr;
        ଯଦି (AstIf* ଅଚଳ ifp = VN_CAST(nodep, If)) {
            ଯଦି (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            AstVarScope* resetStateVscp = nullptr;
            ଅଚଳ ResetAssignStatus resetStatus
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            ଅଚଳ ତାର୍କିକ emptyResetBranch = resetStatus == ResetAssignStatus::EMPTY;
            ଯଦି (emptyResetBranch) {
                cand.resetArcs().clear();
            } ଅନ୍ୟଥା ଯଦି (resetStatus == ResetAssignStatus::NONE) {
                cand.resetArcs().clear();
                Fsm_State_ମୂଲ୍ୟ reset_ମୂଲ୍ୟ;
                Ast_ଗଣ୍ଠି* ଅଚଳ thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                Ast_ଗଣ୍ଠି_Assign* ଅଚଳ resetAssp
                    = direct_Const_State_Assign_ଗଣ୍ଠି(thenNodep, resetStateVscp, reset_ମୂଲ୍ୟ);
                ଯଦି (!resetAssp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                cand.resetArcs().emplace_back(reset_ମୂଲ୍ୟ, resetAssp);
            } ଅନ୍ୟଥା ଯଦି (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                cand.resetArcs().clear();
            }
            Ast_ଗଣ୍ଠି* ଅଚଳ elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            ଯଦି (!ଗଣ୍ଠି_State_Var_Assign(elseNodep, stateVscp, nextVscp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଯଦି (!emptyResetBranch && resetStateVscp != stateVscp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != nullptr);
        } ଅନ୍ୟଥା {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* resetCondp = nullptr;
            Fsm_State_ମୂଲ୍ୟ reset_ମୂଲ୍ୟ;
            // Note the active low 'reset ? next : CONST' form is not accepted
            ଯଦି (Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assp
                = directCondStateVarAssign(nodep, stateVscp, nextVscp, resetCondp, reset_ମୂଲ୍ୟ)) {
                cand.resetArcs().emplace_back(reset_ମୂଲ୍ୟ, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != nullptr);
            } ଅନ୍ୟଥା ଯଦି (!ଗଣ୍ଠି_State_Var_Assign(nodep, stateVscp, nextVscp)) {
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
        }
        cand.scopep(scopep);
        cand.alwaysp(alwaysp);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(stateVscp);
        cand.nextVscp(nextVscp);
        cand.senses() = describe_Sen_ବୃକ୍ଷ(alwaysp->sentreep());
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(alwaysp->fileline());
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ସ୍ଥିର ତାର୍କିକ add_ମୂଲ୍ୟ_To_State_Space(Ast_ଗଣ୍ଠି* nodep, FsmStateSpace& stateSpace,
                                     ଅଚଳ Fsm_State_ମୂଲ୍ୟ& ମୂଲ୍ୟ, StateConstLabel label) {
        ଅଚଳ auto labelIt = stateSpace.labels.find(ମୂଲ୍ୟ);
        ଯଦି (labelIt != stateSpace.labels.end()) {
            StateConstLabel& existingLabel = labelIt->second;
            ଯଦି (existingLabel.text != label.text && existingLabel.fromParam && label.fromParam) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                        "state variable "
                                            + stateSpace.stateVarp->prettyNameQ()
                                            + " with multiple labels for the same value "
                                            + ମୂଲ୍ୟ.warnText() + ": " + existingLabel.text
                                            + " and " + label.text);
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            ଯଦି (!existingLabel.fromParam && label.fromParam) {
                existingLabel.text = label.text;
                existingLabel.fromParam = label.fromParam;
                updateStateLabel(stateSpace, ମୂଲ୍ୟ, label);
            }
        } ଅନ୍ୟଥା {
            StateConstLabel storedLabel = label;
            storedLabel.stateIndex = stateSpace.states.size();
            stateSpace.states.emplace_back(label.text, ମୂଲ୍ୟ);
            stateSpace.labels.emplace(ମୂଲ୍ୟ, storedLabel);
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    ସ୍ଥିର ତାର୍କିକ add_ପଦସମଷ୍ଟି_To_State_Space(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_ମୂଲ୍ୟ ମୂଲ୍ୟ;
        ଅଚଳ Const_ମୂଲ୍ୟ_Status status = const_ମୂଲ୍ୟ_Status(valuep, ମୂଲ୍ୟ);
        ଯଦି (status != Const_ମୂଲ୍ୟ_Status::OK) {
            ଯଦି (status == Const_ମୂଲ୍ୟ_Status::XZ) {
                valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                         "state variable "
                                             + stateSpace.stateVarp->prettyNameQ()
                                             + " with X/Z state encoding values");
            }
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        AstConst* ଅଚଳ constp = VN_AS(valuep, Const);
        ଫେରନ୍ତୁ add_ମୂଲ୍ୟ_To_State_Space(valuep, stateSpace, ମୂଲ୍ୟ, stateLabelForConst(constp));
    }

    ସ୍ଥିର ତାର୍କିକ add_Optional_Target_ପଦସମଷ୍ଟି_To_State_Space(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_ମୂଲ୍ୟ ମୂଲ୍ୟ;
        ଅଚଳ Const_ମୂଲ୍ୟ_Status status = const_ମୂଲ୍ୟ_Status(valuep, ମୂଲ୍ୟ);
        ଯଦି (status != Const_ମୂଲ୍ୟ_Status::OK) {
            valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                     "state variable "
                                         + stateSpace.stateVarp->prettyNameQ()
                                         + " with non-constant target state values");
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        AstConst* ଅଚଳ constp = VN_AS(valuep, Const);
        ଫେରନ୍ତୁ add_ମୂଲ୍ୟ_To_State_Space(valuep, stateSpace, ମୂଲ୍ୟ, stateLabelForConst(constp));
    }

    ସ୍ଥିର ଶୂନ୍ୟ addResetTargetsToStateSpace(ଅଚଳ std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        ପାଇଁ (ଅଚଳ FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.to_ମୂଲ୍ୟ().ascii(), ମିଥ୍ୟା, 0};
            ଯଦି (AstConst* ଅଚଳ constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                add_ମୂଲ୍ୟ_To_State_Space(resetArc.nodep(), stateSpace, resetArc.to_ମୂଲ୍ୟ(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    ସ୍ଥିର ତାର୍କିକ collectDeclaredStateSpace(Ast_ଗଣ୍ଠି* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, ତାର୍କିକ& needs_Source_ମୂଲ୍ୟଗୁଡ଼ିକ) {
        AstVar* ଅଚଳ stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        ଯଦି (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        ଅଚଳ ତାର୍କିକ forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        ଯଦି (enump) {
            stateSpace.enumBacked = ସତ୍ୟ;
            ପାଇଁ (AstEnumItem* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ଅଚଳ AstConst* ଅଚଳ constp = VN_AS(itemp->valuep(), Const);
                ଅଚଳ Fsm_State_ମୂଲ୍ୟ ମୂଲ୍ୟ{constp->ଅଙ୍କ()};
                ଅଚଳ size_t stateIndex = stateSpace.states.size();
                stateSpace.states.emplace_back(itemp->name(), ମୂଲ୍ୟ);
                stateSpace.labels.emplace(ମୂଲ୍ୟ,
                                          StateConstLabel{itemp->name(), ମିଥ୍ୟା, stateIndex});
            }
            ଫେରନ୍ତୁ stateSpace.states.size() >= 2;
        }

        ଯଦି (forced) {
            needs_Source_ମୂଲ୍ୟଗୁଡ଼ିକ = ସତ୍ୟ;
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }

        needs_Source_ମୂଲ୍ୟଗୁଡ଼ିକ = ସତ୍ୟ;
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    template <typename T_ValuepVisitor>
    ସ୍ଥିର ତାର୍କିକ collect_State_Space_From_ମୂଲ୍ୟଗୁଡ଼ିକ(Ast_ଗଣ୍ଠି* warnNodep, AstVarScope* stateVscp,
                                            ଅଚଳ std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            ଅଚଳ T_ValuepVisitor& visitValueps) {
        ତାର୍କିକ needs_Source_ମୂଲ୍ୟଗୁଡ଼ିକ = ମିଥ୍ୟା;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        ଯଦି (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needs_Source_ମୂଲ୍ୟଗୁଡ଼ିକ)) {
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଯଦି (!needs_Source_ମୂଲ୍ୟଗୁଡ଼ିକ) ଫେରନ୍ତୁ ସତ୍ୟ;
        addResetTargetsToStateSpace(resetArcs, stateSpace);
        ଯଦି (!visitValueps(
                [&](Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep) { ଫେରନ୍ତୁ add_ପଦସମଷ୍ଟି_To_State_Space(valuep, stateSpace); })) {
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଫେରନ୍ତୁ stateSpace.states.size() >= 2;
    }

    ସ୍ଥିର ତାର୍କିକ collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* assignVscp,
                                  ଅଚଳ std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        ଫେରନ୍ତୁ collect_State_Space_From_ମୂଲ୍ୟଗୁଡ଼ିକ(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, assignVscp, &stateSpace](ଅଚଳ auto& visitValuep) {
                ପାଇଁ (AstCaseItem* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), CaseItem)) {
                    ଯଦି (!itemp->isDefault()) {
                        ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) {
                            ଯଦି (!visitValuep(condp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                        }
                    }
                    ଯଦି (Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assp
                        = directStateAssign(itemp->stmtsp(), assignVscp)) {
                        Fsm_State_ମୂଲ୍ୟ then_ମୂଲ୍ୟ;
                        Fsm_State_ମୂଲ୍ୟ else_ମୂଲ୍ୟ;
                        AstCond* ଅଚଳ condp = VN_CAST(assp->rhsp(), Cond);
                        ଯଦି (condp
                            && directStateCondConstAssign(itemp->stmtsp(), assignVscp, then_ମୂଲ୍ୟ,
                                                          else_ମୂଲ୍ୟ)) {
                            ଯଦି (!visitValuep(condp->thenp())) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                            ଯଦି (!visitValuep(condp->elsep())) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                        } ଅନ୍ୟଥା ଯଦି (!add_Optional_Target_ପଦସମଷ୍ଟି_To_State_Space(assp->rhsp(), stateSpace)) {
                            ଫେରନ୍ତୁ ମିଥ୍ୟା;
                        }
                    }
                }
                ଫେରନ୍ତୁ ସତ୍ୟ;
            });
    }

    ସ୍ଥିର ତାର୍କିକ collectStateSpace(ଅଚଳ FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* assignVscp,
                                  ଅଚଳ std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        ଫେରନ୍ତୁ collect_State_Space_From_ମୂଲ୍ୟଗୁଡ଼ିକ(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, assignVscp, &stateSpace](ଅଚଳ auto& visitValuep) {
                ପାଇଁ (ଅଚଳ FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(visitValuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assp = directStateAssign(branch.stmtsp, assignVscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    Fsm_State_ମୂଲ୍ୟ then_ମୂଲ୍ୟ;
                    Fsm_State_ମୂଲ୍ୟ else_ମୂଲ୍ୟ;
                    AstCond* ଅଚଳ condp = VN_CAST(assp->rhsp(), Cond);
                    ଯଦି (condp) {
                        UASSERT_OBJ(directStateCondConstAssign(branch.stmtsp, assignVscp,
                                                               then_ମୂଲ୍ୟ, else_ମୂଲ୍ୟ),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        ଯଦି (!visitValuep(condp->thenp())) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                        ଯଦି (!visitValuep(condp->elsep())) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                    } ଅନ୍ୟଥା ଯଦି (!add_Optional_Target_ପଦସମଷ୍ଟି_To_State_Space(assp->rhsp(), stateSpace)) {
                        ଫେରନ୍ତୁ ମିଥ୍ୟା;
                    }
                }
                ଫେରନ୍ତୁ ସତ୍ୟ;
            });
    }

    // Extract supported case-item transitions in one place so the conservative
    // policy for direct and ternary forms stays consistent. The false exits in
    // this helper are deliberate subset boundaries: they document shapes we do
    // not yet model in this PR and that future FSM-detection work may widen.
    ସ୍ଥିର ତାର୍କିକ emitCaseItemArcs(FsmGraph& graph, AstCaseItem* itemp, AstVarScope* stateVscp,
                                 ଅଚଳ FsmStateSpace& stateSpace, ତାର୍କିକ inclCond) {
        std::vector<std::pair<string, Fsm_State_ମୂଲ୍ୟ>> froms;
        ଯଦି (itemp->isDefault()) {
            ଯଦି (!inclCond) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            froms.emplace_back("default", Fsm_State_ମୂଲ୍ୟ{});
        } ଅନ୍ୟଥା {
            ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) {
                Fsm_State_ମୂଲ୍ୟ ମୂଲ୍ୟ;
                ଯଦି (const_ମୂଲ୍ୟ_Status(condp, ମୂଲ୍ୟ) != Const_ମୂଲ୍ୟ_Status::OK) ଜାରି;
                ଯଦି (!validate_Known_State_ମୂଲ୍ୟ(condp, stateSpace, ମୂଲ୍ୟ, "source")) ଫେରନ୍ତୁ ସତ୍ୟ;
                froms.emplace_back(label_For_ମୂଲ୍ୟ(stateSpace.labels, ମୂଲ୍ୟ), ମୂଲ୍ୟ);
            }
            ଯଦି (froms.empty()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }

        ଯଦି (Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assp = directStateAssign(itemp->stmtsp(), stateVscp)) {
            Fsm_State_ମୂଲ୍ୟ to_ମୂଲ୍ୟ;
            ଅଚଳ Const_ମୂଲ୍ୟ_Status status = const_ମୂଲ୍ୟ_Status(assp->rhsp(), to_ମୂଲ୍ୟ);
            ଯଦି (status == Const_ମୂଲ୍ୟ_Status::OK) {
                ଯଦି (!validate_Known_State_ମୂଲ୍ୟ(assp, stateSpace, to_ମୂଲ୍ୟ, "target")) ଫେରନ୍ତୁ ସତ୍ୟ;
                ପାଇଁ (ଅଚଳ std::pair<string, Fsm_State_ମୂଲ୍ୟ>& from : froms) {
                    graph.addArc(from.second, to_ମୂଲ୍ୟ, ମିଥ୍ୟା, ମିଥ୍ୟା, itemp->isDefault(),
                                 assp->fileline());
                }
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }

        Fsm_State_ମୂଲ୍ୟ then_ମୂଲ୍ୟ;
        Fsm_State_ମୂଲ୍ୟ else_ମୂଲ୍ୟ;
        ଯଦି (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_ମୂଲ୍ୟ, else_ମୂଲ୍ୟ)
            || ifStateConstAssign(itemp->stmtsp(), stateVscp, then_ମୂଲ୍ୟ, else_ମୂଲ୍ୟ)) {
            ଯଦି (!validate_Known_State_ମୂଲ୍ୟ(itemp->stmtsp(), stateSpace, then_ମୂଲ୍ୟ, "target"))
                ଫେରନ୍ତୁ ସତ୍ୟ;
            ଯଦି (!validate_Known_State_ମୂଲ୍ୟ(itemp->stmtsp(), stateSpace, else_ମୂଲ୍ୟ, "target"))
                ଫେରନ୍ତୁ ସତ୍ୟ;
            ପାଇଁ (ଅଚଳ Fsm_State_ମୂଲ୍ୟ& branch_ମୂଲ୍ୟ : {then_ମୂଲ୍ୟ, else_ମୂଲ୍ୟ}) {
                ପାଇଁ (ଅଚଳ std::pair<string, Fsm_State_ମୂଲ୍ୟ>& from : froms) {
                    graph.addArc(from.second, branch_ମୂଲ୍ୟ, ମିଥ୍ୟା, ସତ୍ୟ, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }

        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ସ୍ଥିର ଶୂନ୍ୟ emitStmtArcsFrom(FsmGraph& graph, Ast_ଗଣ୍ଠି* stmtsp, AstVarScope* stateVscp,
                                 ଅଚଳ FsmStateSpace& stateSpace, Fsm_State_ମୂଲ୍ୟ from_ମୂଲ୍ୟ,
                                 ତାର୍କିକ isDefault, ତାର୍କିକ forceCond) {
        Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assp = directStateAssign(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        Fsm_State_ମୂଲ୍ୟ to_ମୂଲ୍ୟ;
        ଅଚଳ Const_ମୂଲ୍ୟ_Status status = const_ମୂଲ୍ୟ_Status(assp->rhsp(), to_ମୂଲ୍ୟ);
        ଯଦି (status == Const_ମୂଲ୍ୟ_Status::OK) {
            ଯଦି (!validate_Known_State_ମୂଲ୍ୟ(assp, stateSpace, to_ମୂଲ୍ୟ, "target")) ଫେରନ୍ତୁ;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            graph.addArc(from_ମୂଲ୍ୟ, to_ମୂଲ୍ୟ, ମିଥ୍ୟା, forceCond, isDefault, assp->fileline());
            ଫେରନ୍ତୁ;
        }

        Fsm_State_ମୂଲ୍ୟ then_ମୂଲ୍ୟ;
        Fsm_State_ମୂଲ୍ୟ else_ମୂଲ୍ୟ;
        ଅଚଳ ତାର୍କିକ condAssign
            = directStateCondConstAssign(stmtsp, stateVscp, then_ମୂଲ୍ୟ, else_ମୂଲ୍ୟ);
        UASSERT_OBJ(condAssign, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        ଯଦି (!validate_Known_State_ମୂଲ୍ୟ(stmtsp, stateSpace, then_ମୂଲ୍ୟ, "target")) ଫେରନ୍ତୁ;
        ଯଦି (!validate_Known_State_ମୂଲ୍ୟ(stmtsp, stateSpace, else_ମୂଲ୍ୟ, "target")) ଫେରନ୍ତୁ;
        ପାଇଁ (ଅଚଳ Fsm_State_ମୂଲ୍ୟ& branch_ମୂଲ୍ୟ : {then_ମୂଲ୍ୟ, else_ମୂଲ୍ୟ}) {
            graph.addArc(from_ମୂଲ୍ୟ, branch_ମୂଲ୍ୟ, ମିଥ୍ୟା, ସତ୍ୟ, isDefault, stmtsp->fileline());
        }
    }

    ସ୍ଥିର ଶୂନ୍ୟ emitIfChainArcs(FsmGraph& graph, ଅଚଳ FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, ଅଚଳ FsmStateSpace& stateSpace) {
        ପାଇଁ (size_t i = 0; i < chain.branches.size(); ++i) {
            ଅଚଳ FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            ଯଦି (!validate_Known_State_ମୂଲ୍ୟ(branch.ifp, stateSpace, branch.from_ମୂଲ୍ୟ, "source"))
                ଫେରନ୍ତୁ;
            emitStmtArcsFrom(graph, branch.stmtsp, stateVscp, stateSpace, branch.from_ମୂଲ୍ୟ, ମିଥ୍ୟା,
                             branch.has_ଶୀର୍ଷ_Guard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    ସ୍ଥିର ଶୂନ୍ୟ addResetArcs(FsmGraph& graph, ଅଚଳ std::vector<FsmResetArcDesc>& resetArcs,
                             ଅଚଳ FsmStateSpace& stateSpace) {
        ପାଇଁ (ଅଚଳ FsmResetArcDesc& resetArc : resetArcs) {
            ଯଦି (!validate_Known_State_ମୂଲ୍ୟ(resetArc.nodep(), stateSpace, resetArc.to_ମୂଲ୍ୟ(),
                                         "target"))
                ଜାରି;
            graph.addArc(Fsm_State_ମୂଲ୍ୟ{}, resetArc.to_ମୂଲ୍ୟ(), ସତ୍ୟ, ମିଥ୍ୟା, ମିଥ୍ୟା,
                         resetArc.nodep()->fileline());
        }
    }

    // Turn one candidate case statement into the graph representation that the
    // later lowering phase will consume directly, while reviewers can still
    // inspect the extracted machine via DOT dumps.
    ଶୂନ୍ୟ processCase(AstCase* casep, AstVarScope* assignVscp, ଅଚଳ FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, casep, "FSM case processing requires a non-null assignment var");
        AstVarScope* ଅଚଳ stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        ଯଦି (!collectStateSpace(casep, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) ଫେରନ୍ତୁ;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        ଯଦି (!entry.graphp) {
            entry.graphp.reset(ନୂତନ FsmGraph{});
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
            ପାଇଁ (ଅଚଳ std::pair<string, Fsm_State_ମୂଲ୍ୟ>& state : stateSpace.states) {
                entry.graphp->addStateVertex(state.first, state.second);
            }
            addResetArcs(*entry.graphp, reg.resetArcs(), stateSpace);
        }
        ପାଇଁ (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            emitCaseItemArcs(*entry.graphp, itemp, assignVscp, stateSpace,
                             entry.graphp->inclCond());
        }
    }

    ଶୂନ୍ୟ processIfChain(ଅଚଳ FsmIfChainCandidate& chain, AstVarScope* assignVscp,
                        ଅଚଳ FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, chain.ifp,
                    "FSM if-chain processing requires a non-null assignment var");
        AstVarScope* ଅଚଳ stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        ଯଦି (!collectStateSpace(chain, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) ଫେରନ୍ତୁ;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        // Case candidates keep ownership of existing graphs; reaching this path
        // means the if-chain is the only supported dispatch for this FSM.
        UASSERT_OBJ(!entry.graphp, chain.ifp, "FSM if-chain graph should not already exist");
        entry.graphp.reset(ନୂତନ FsmGraph{});
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
        ପାଇଁ (ଅଚଳ std::pair<string, Fsm_State_ମୂଲ୍ୟ>& state : stateSpace.states) {
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
    ଶୂନ୍ୟ processOneBlockAlways(ଅଚଳ FsmComboAlways& oneBlock) {
        ଅଚଳ RegisterAlwaysAnalyzer analyzer{oneBlock.scopep()};
        AstAlways* ଅଚଳ alwaysp = oneBlock.alwaysp();
        ଯଦି (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) ଫେରନ୍ତୁ;
        ଅଚଳ std::vector<std::pair<AstCase*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>> candidates
            = analyzer.oneBlockCandidates(alwaysp);

        FsmCaseCandidate firstCand;
        ପାଇଁ (ଅଚଳ std::pair<AstCase*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>& cand : candidates) {
            AstVarRef* ଅଚଳ selp = VN_CAST(cand.first->exprp(), VarRef);
            AstVarScope* ଅଚଳ vscp = selp ? selp->varScopep() : nullptr;
            ଯଦି (!vscp) ଜାରି;
            ଯଦି (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processCase(cand.first, vscp, reg);
            } ଅନ୍ୟଥା ଯଦି (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed case statements found in "
                                   "the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } ଅନ୍ୟଥା {
                cand.first->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported case "
                                   "statements found in the same always block. Only the first "
                                   "candidate will be instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            }
        }

        ଯଦି (firstCand.stateVscp) ଫେରନ୍ତୁ;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        ଅଚଳ std::vector<std::pair<AstIf*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>> ifCandidates
            = analyzer.oneBlockIfCandidates(alwaysp);
        ପାଇଁ (ଅଚଳ std::pair<AstIf*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>& cand : ifCandidates) {
            ଅଚଳ FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.first);
            FsmIfChainCandidate chain;
            ଯଦି (!collectIfChain(cand.first, aliases, chain)) ଜାରି;
            AstVarScope* ଅଚଳ vscp = chain.compareVscp;
            ଯଦି (!if_Chain_Supported_Transition_ଗଣ୍ଠି(chain, vscp)) ଜାରି;
            ଯଦି (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processIfChain(chain, vscp, reg);
            } ଅନ୍ୟଥା ଯଦି (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed transition candidates found "
                                   "in the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } ଅନ୍ୟଥା {
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
    ସ୍ଥିର ଶୂନ୍ୟ warnComboSameAlways(Ast_ଗଣ୍ଠି* warnNodep, ଅଚଳ FsmCaseCandidate& firstCand) {
        warnNodep->v3warn(FSMMULTI,
                          "FSM coverage: multiple supported transition candidates found in "
                          "the same combinational always block. Only the first candidate "
                          "will be instrumented."
                              << candidateConflictContext(warnNodep, firstCand));
    }

    ଶୂନ୍ୟ processComboAlways(ଅଚଳ FsmComboAlways& combo) {
        ଅଚଳ ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate firstCand;
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ଅଚଳ casep = VN_CAST(nodep, Case);
            ଯଦି (!casep) ଜାରି;
            ଅଚଳ ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchCase(stmtsp, casep);
            ଅଚଳ FsmRegisterCandidate* ଅଚଳ matchedp = match.matchedp;
            Ast_ଗଣ୍ଠି* ଅଚଳ matchedWarnNodep = match.warnNodep;
            ଯଦି (!matchedp) ଜାରି;
            ଯଦି (!firstCand.stateVscp) {
                ଅଚଳ auto ଭର୍ତ୍ତି_Pair = m_comboPaired.emplace(
                    matchedp->stateVscp(),
                    FsmCaseCandidate{matchedWarnNodep,
                                     const_cast<AstVarScope*>(matchedp->stateVscp())});
                ଯଦି (!ଭର୍ତ୍ତି_Pair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  ଭର୍ତ୍ତି_Pair.first->second));
                    ଜାରି;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processCase(casep, matchedp->nextVscp(), *matchedp);
                ଜାରି;
            }
            ଯଦି (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                ଜାରି;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported case statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, firstCand));
        }
        ଯଦି (firstCand.stateVscp) ଫେରନ୍ତୁ;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* ଅଚଳ ifp = VN_CAST(nodep, If);
            ଯଦି (!ifp) ଜାରି;
            FsmIfChainCandidate chain;
            ଅଚଳ FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            ଯଦି (!collectIfChain(ifp, aliases, chain)) ଜାରି;
            ଅଚଳ ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchIfChain(stmtsp, chain);
            ଅଚଳ FsmRegisterCandidate* ଅଚଳ matchedp = match.matchedp;
            Ast_ଗଣ୍ଠି* ଅଚଳ matchedWarnNodep = match.warnNodep;
            ଯଦି (!matchedp) ଜାରି;
            ଯଦି (!firstCand.stateVscp) {
                ଅଚଳ std::pair<std::unordered_map<ଅଚଳ AstVarScope*, FsmCaseCandidate>::iterator,
                                ତାର୍କିକ>
                    ଭର୍ତ୍ତି_Pair = m_comboPaired.emplace(
                        matchedp->stateVscp(),
                        FsmCaseCandidate{matchedWarnNodep,
                                         const_cast<AstVarScope*>(matchedp->stateVscp())});
                ଯଦି (!ଭର୍ତ୍ତି_Pair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  ଭର୍ତ୍ତି_Pair.first->second));
                    ଜାରି;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processIfChain(chain, matchedp->nextVscp(), *matchedp);
                ଜାରି;
            }
            ଯଦି (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                ଜାରି;
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
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    // Collect processes first, then analyze FSM candidates once all alias and
    // register information is available.
    ଶୂନ୍ୟ visit(AstAlways* nodep) override {
        ଯଦି (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            ଫେରନ୍ତୁ;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_oneBlockAlwayss.emplace_back(m_scopep, nodep);
        ଅଚଳ RegisterAlwaysAnalyzer analyzer{m_scopep};
        FsmRegisterCandidate reg;
        ଯଦି (analyzer.matchRegisterCandidate(nodep, reg)) {
            AstVarScope* ଅଚଳ stateVscp = reg.stateVscp();
            ଅଚଳ ତାର୍କିକ found
                = std::any_of(m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                              [stateVscp](ଅଚଳ FsmRegisterCandidate& existing) {
                                  ଫେରନ୍ତୁ existing.stateVscp() == stateVscp;
                              });
            ଯଦି (!found) { m_registerCandidates.emplace_back(reg); }
        }
        ଯଦି (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            m_comboAlwayss.emplace_back(m_scopep, nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->keyword() == VAlwaysKwd::ALWAYS) {
            ଯଦି (!nodep->sentreep() || isPlainComboSentree(nodep->sentreep())) {
                m_comboAlwayss.emplace_back(m_scopep, nodep);
            } ଅନ୍ୟଥା {
                m_nonComboAlwayss.emplace_back(m_scopep, nodep);
            }
        }
    }

    ଶୂନ୍ୟ visit(AstAssignW* nodep) override {
        // Continuous aliases are unordered hardware connections, so source
        // order should not affect whether an if-chain FSM is recognized.
        collectAliasFromAssign(nodep, m_stateAliases, m_ambiguousStateAliases);
        collectCellPortAlias(nodep);
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstCell* nodep) override {
        // Cells are matched after the full traversal because linkdot lowers
        // uninlined port connections into sibling continuous assignments.
        ଯଦି (m_scopep && fsmRegisterWrapperDesc(nodep)) addWrapperCell(m_scopep, nodep);
        iterateChildren(nodep);
    }

    // Continue the walk through the rest of the design hierarchy.
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        ପାଇଁ (ଅଚଳ std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            FsmRegisterCandidate reg;
            ଯଦି (matchFsmWrapperCell(wrapperCell.first, wrapperCell.second, reg)) {
                m_registerCandidates.emplace_back(reg);
            }
        }
        ପାଇଁ (ଅଚଳ FsmComboAlways& oneBlock : m_oneBlockAlwayss) processOneBlockAlways(oneBlock);
        ପାଇଁ (ଅଚଳ FsmComboAlways& combo : m_comboAlwayss) processComboAlways(combo);
        ପାଇଁ (ଅଚଳ FsmComboAlways& combo : m_nonComboAlwayss) warnUnsupportedComboAlways(combo);
    }
};

// Builder of statements that will handle incrementation of proper AstNodeCoverDecls - which will
// be also created here
ଶ୍ରେଣୀ FsmCoverIncBuilder final {
    ଅଚଳ FsmGraph& m_graph;  // Fsm graph
    std::vector<ଅଚଳ FsmStateVertex*> m_states;  // States vertexes inside fsm graph
    std::unordered_set<uint64_t>
        m_undefinedStates;  // States that have no enum value but are possible to represent with a
                            // variable of given width.
                            // This variable keeps uint64_t instead of V3Numebr because it is safe
                            // to assume that nobody has over 16M TB of RAM
    ଅଚଳ FsmVertex* m_defaultVertexp = nullptr;  // Default vertex
    ଅଚଳ FsmVertex* m_resetVertexp = nullptr;  // Reset vertex
    size_t m_edges_ଗଣନା = 0;  // Total count of edges
    AstVarScope* ଅଚଳ m_prevVscp;  // Previous state of fsm
    File_ଧାଡ଼ି* ଅଚଳ m_flp;  // Fileline pointer to use across the builder
    ଅଚଳ std::string m_modPrettyName;  // Pretty fsm mode name
    Ast_ଗଣ୍ଠି_Stmt* m_coverIncp = nullptr;  // Increments proper CoverDecl after each change
    Ast_ଗଣ୍ଠି_Stmt* m_initialCoverIncp = nullptr;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = nullptr;  // List of CoverDecls
    ତାର୍କିକ m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(ଅଚଳ uint64_t ମୂଲ୍ୟ) {
        V3_ସଂଖ୍ୟା ଅଙ୍କ{static_cast<Ast_ଗଣ୍ଠି*>(nullptr), m_prevVscp->width()};
        ଅଙ୍କ.setQuad(ମୂଲ୍ୟ);
        ଅଙ୍କ.isSigned(m_prevVscp->dtypep()->isSigned());
        ଫେରନ୍ତୁ ନୂତନ AstConst{m_flp, ଅଙ୍କ};
    }

    std::string quadToBitStirngLabel(ଅଚଳ uint64_t val) {
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = m_prevVscp->width();
        UASSERT(width < 64, "We should not expand values that wide");
        std::string result = std::to_string(width) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        ଯେତେବେଳେ (--width >= 0) result.ଠେଲ_back("01"[(val >> width) & 1]);
        ଫେରନ୍ତୁ result;
    }

    ଶୂନ୍ୟ initExistingStates() {
        ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : m_graph.vertices()) {
            ଅଚଳ FsmVertex* ଅଚଳ vertexp = vtx.as<FsmVertex>();
            m_edges_ଗଣନା += vertexp->inEdges().size();
            ଯଦି (ଅଚଳ FsmStateVertex* ଅଚଳ statep = vertexp->cast<FsmStateVertex>()) {
                m_states.ଠେଲ_back(statep);
            } ଅନ୍ୟଥା ଯଦି (vertexp->isDefaultAny()) {
                UASSERT_OBJ(!std::exchange(m_defaultVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two defaults?");
            } ଅନ୍ୟଥା /* if (vertexp->isResetAny()) */ {
                UASSERT_OBJ(!std::exchange(m_resetVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two resets?");
            }
        }
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = m_graph.sampleVarScopep()->width();
        m_isComplete = width < 64 && m_states.size() == (static_cast<size_t>(1) << width);
    }

    ଶୂନ୍ୟ initUndefinedStates() {
        UASSERT_OBJ(m_graph.sampleVarScopep()->width() < 64, m_graph.sampleVarScopep(),
                    "This FSM is too wide to expand - this should be caught earlier");
        ଅଚଳ uint64_t combinations_ଗଣନା = static_cast<size_t>(1)
                                           << m_graph.sampleVarScopep()->width();
        // Rolling trick allows to cut peak memory usage
        ତାର୍କିକ undefRolled = ମିଥ୍ୟା;
        auto rollUndefs = [ଏହି, combinations_ଗଣନା] {
            ପାଇଁ (uint64_t i = 0; i < combinations_ଗଣନା; ++i) {
                ଅଚଳ auto ଭର୍ତ୍ତି = m_undefinedStates.ଭର୍ତ୍ତି(i);
                ଯଦି (!ଭର୍ତ୍ତି.second) m_undefinedStates.erase(ଭର୍ତ୍ତି.first);
            }
        };
        ପାଇଁ (ଅଚଳ FsmStateVertex* ଅଚଳ statep : m_states) {
            ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ = statep->ମୂଲ୍ୟ().ଅଙ୍କ();
            ଅଚଳ uint64_t ମୂଲ୍ୟ = ଅଙ୍କ.toUQuad();
            ଯଦି (!m_isComplete) {
                ଯଦି (undefRolled) {
                    UASSERT_OBJ(m_undefinedStates.erase(ମୂଲ୍ୟ) == 1, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                } ଅନ୍ୟଥା {
                    UASSERT_OBJ(m_undefinedStates.ଭର୍ତ୍ତି(ମୂଲ୍ୟ).second, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                    ଯଦି (m_undefinedStates.size() * 2 >= combinations_ଗଣନା) {
                        undefRolled = ସତ୍ୟ;
                        rollUndefs();
                    }
                }
            }
        }
        ଯଦି (!(m_isComplete || undefRolled)) rollUndefs();
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* buildResetCond() ଅଚଳ {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ refp
            = ନୂତନ AstVarRef{m_flp, m_graph.resetCond().varScopep, VAccess::READ};
        ଫେରନ୍ତୁ m_graph.resetCond().active_ତଳ
                   ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstLogNot{m_flp, refp})
                   : refp;
    }

    size_t get_Arc_ଗଣନା_With_Expansion(ଅଚଳ VFsmExpandType expand) ଅଚଳ {
        size_t arc_ଗଣନା_With_Expansion = ~0ULL;
        ବାଛ (expand) {
        ସ୍ଥିତି VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_graph.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି VFsmExpandType::AUTO: {
            arc_ଗଣନା_With_Expansion = m_edges_ଗଣନା;
            ଯଦି (m_defaultVertexp) {
                UASSERT_OBJ(m_states.size() >= m_defaultVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arc_ଗଣନା_With_Expansion -= m_defaultVertexp->outEdges().size();
                ପାଇଁ (ଅଚଳ FsmStateVertex* ଅଚଳ statep : m_states) {
                    arc_ଗଣନା_With_Expansion += static_cast<size_t>(statep->outEdges().empty())
                                             * m_defaultVertexp->outEdges().size();
                }
            }
            ଯଦି (m_resetVertexp) {
                UASSERT_OBJ(m_states.size() >= m_resetVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arc_ଗଣନା_With_Expansion += m_states.size() - m_resetVertexp->outEdges().size();
            }
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି VFsmExpandType::AUTO_EXPAND: {
            arc_ଗଣନା_With_Expansion = m_states.size() * m_states.size() - m_states.size();
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି VFsmExpandType::FULL: {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = m_graph.sampleVarScopep()->width();
            // 32 is derived from:
            // tranitionsCount <= std::numeric_limits<uint64_t>::max()
            // (2^width)^2 - (2^width) <= 2^64 - 1
            // (2^width)^2 - (2^width) < 2^64 since we operate on integral values we can trade 1
            // log_2((2^width)^2 - (2^width)) < 64
            // log_2((2^width)^2) < 64 - `(2^width)` is negigible so, it may be dropped
            // 2width < 64
            ଯଦି (width < 32) {
                ଅଚଳ uint64_t combinations_ଗଣନା = static_cast<size_t>(1) << width;
                arc_ଗଣନା_With_Expansion = combinations_ଗଣନା * combinations_ଗଣନା - combinations_ଗଣନା;
            }
            ଭାଙ୍ଗ;
        }
        }
        ଫେରନ୍ତୁ arc_ଗଣନା_With_Expansion;
    }

    AstCoverOtherDecl* buildStateCoverDecl(ଅଚଳ std::string& label,
                                           ଅଚଳ std::string& fsmTag = "") {
        AstCoverOtherDecl* ଅଚଳ declp
            = ନୂତନ AstCoverOtherDecl{m_flp,
                                    "v_fsm_state/" + m_modPrettyName,
                                    m_graph.stateVarName() + "::" + label,
                                    "",
                                    0,
                                    m_graph.stateVarName(),
                                    "",
                                    label,
                                    fsmTag};
        // We run after scoping, so this is declared for one instance only
        declp->perInstance(ସତ୍ୟ);
        m_declsp = Ast_ଗଣ୍ଠି::addNext(m_declsp, declp);
        ଫେରନ୍ତୁ declp;
    }

    Ast_ଗଣ୍ଠି_Stmt* buildCoverIncStmtp(ଅଚଳ std::string& fromStateLabel,
                                    ଅଚଳ std::string& toStateLabel,
                                    ଅଚଳ std::string& baseLabel = "") {
        // Whole builder is designed in such a way that this function won't be called twice with
        // the same arguments - but if it will ever change in future, `AstCoverOtherDecl` shall be
        // cached for particular args set
        AstCoverOtherDecl* ଅଚଳ declp = ନୂତନ AstCoverOtherDecl{
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
        declp->perInstance(ସତ୍ୟ);
        m_declsp = Ast_ଗଣ୍ଠି::addNext(m_declsp, declp);
        ଫେରନ୍ତୁ ନୂତନ AstCoverInc{m_flp, declp};
    }
    Ast_ଗଣ୍ଠି_Stmt* buildExpandedTransitionHandler(ଅଚଳ std::string& toStateLabel,
                                                ଅଚଳ ତାର୍କିକ considerUndefs,
                                                ଅଚଳ ତାର୍କିକ expandOnlyWithTerminalVerticies,
                                                ଅଚଳ FsmStateVertex* ଅଚଳ toStatep = nullptr) {
        AstCase* ଅଚଳ arcCasep = ନୂତନ AstCase{
            m_flp, VCaseType::CT_CASE, ନୂତନ AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        std::unordered_set<ଅଚଳ FsmStateVertex*> handledSoureces;
        ଯଦି (toStatep) {
            ପାଇଁ (ଅଚଳ V3GraphEdge& edge : toStatep->inEdges()) {
                ଯଦି (ଅଚଳ FsmStateVertex* ଅଚଳ fromp = edge.fromp()->cast<FsmStateVertex>()) {
                    ଯଦି (!handledSoureces.emplace(fromp).second) ଜାରି;
                    arcCasep->addItemsp(ନୂତନ AstCaseItem{
                        m_flp, makeConst(fromp->ମୂଲ୍ୟ().ଅଙ୍କ().toUQuad()),
                        (fromp != toStatep) ? buildCoverIncStmtp(fromp->label(), toStateLabel)
                                            : nullptr});
                }
            }
        }
        ପାଇଁ (ଅଚଳ FsmStateVertex* ଅଚଳ fromp : m_states) {
            ଯଦି (handledSoureces.find(fromp) != handledSoureces.end()) ଜାରି;
            arcCasep->addItemsp(ନୂତନ AstCaseItem{
                m_flp, makeConst(fromp->ମୂଲ୍ୟ().ଅଙ୍କ().toUQuad()),
                (fromp != toStatep
                 && (!expandOnlyWithTerminalVerticies || fromp->outEdges().empty()))
                    ? buildCoverIncStmtp(fromp->label(), toStateLabel, "artificial")
                    : nullptr});
        }
        ଯଦି (!m_isComplete) {
            ଯଦି (considerUndefs) {
                ପାଇଁ (ଅଚଳ uint64_t from : m_undefinedStates) {
                    ଅଚଳ std::string& label = quadToBitStirngLabel(from);
                    arcCasep->addItemsp(
                        ନୂତନ AstCaseItem{m_flp, makeConst(from),
                                        (label != toStateLabel)
                                            ? buildCoverIncStmtp(label, toStateLabel, "artificial")
                                            : nullptr});
                }
            } ଅନ୍ୟଥା {
                arcCasep->addItemsp(ନୂତନ AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        ଫେରନ୍ତୁ arcCasep;
    }
    Ast_ଗଣ୍ଠି_Stmt* buildUnxpandedTransitionHandler(ଅଚଳ FsmStateVertex* ଅଚଳ toStatep) {
        AstCase* ଅଚଳ arcCasep = ନୂତନ AstCase{
            m_flp, VCaseType::CT_CASE, ନୂତନ AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        ପାଇଁ (ଅଚଳ V3GraphEdge& edge : toStatep->inEdges()) {
            ଅଚଳ FsmArcEdge* ଅଚଳ arcp = edge.as<FsmArcEdge>();
            UASSERT_OBJ(!arcp->isReset(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            ଅଚଳ FsmStateVertex* ଅଚଳ fromp = arcp->fromp()->as<FsmStateVertex>();
            ଯଦି (fromp != toStatep) {
                arcCasep->addItemsp(
                    ନୂତନ AstCaseItem{m_flp, makeConst(fromp->ମୂଲ୍ୟ().ଅଙ୍କ().toUQuad()),
                                    buildCoverIncStmtp(fromp->label(), toStatep->label())});
            }
        }
        arcCasep->addItemsp(ନୂତନ AstCaseItem{m_flp, nullptr, nullptr});
        ଫେରନ୍ତୁ arcCasep;
    }

ସର୍ବସାଧାରଣ:
    FsmCoverIncBuilder(ଅଚଳ FsmGraph& graph, AstVarScope* ଅଚଳ prevVscp)
        : m_graph{graph}
        , m_prevVscp{prevVscp}
        , m_flp{graph.fileline()}
        , m_modPrettyName{graph.scopep()->modp()->prettyName()} {
        initExistingStates();
        AstVarScope* ଅଚଳ sampleVscp = graph.sampleVarScopep();
        ଅଚଳ VFsmExpandType expand
            = graph.stateVarScopep()->varp()->attrFsmStateExpand() != VFsmExpandType::DEFAULT
                  ? graph.stateVarScopep()->varp()->attrFsmStateExpand()
                  : v3Global.opt.coverageFsmExpand();

        {
            ଅଚଳ size_t arc_ଗଣନା_With_Expansion = get_Arc_ଗଣନା_With_Expansion(expand);
            ଯଦି (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arc_ଗଣନା_With_Expansion == 0ULL
                            || arc_ଗଣନା_With_Expansion
                                   > static_cast<size_t>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arc_ଗଣନା_With_Expansion);
                ଫେରନ୍ତୁ;
            }
        }

        AstCase* ଅଚଳ arcCasep
            = ନୂତନ AstCase{m_flp, VCaseType::CT_CASE,
                          ନୂତନ AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};
        AstCase* ଅଚଳ initialStateCasep
            = ନୂତନ AstCase{m_flp, VCaseType::CT_CASE,
                          ନୂତନ AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};

        ଅଚଳ ତାର୍କିକ expandAll = expand != VFsmExpandType::AUTO;
        std::unordered_set<ଅଚଳ FsmStateVertex*>
            defaultTargets;  // States that have edge incoming from default vertex
        std::unordered_set<ଅଚଳ FsmStateVertex*>
            resetTargets;  // States that have edge incoming from reset vertex
        ଯଦି (!expandAll) {
            ଯଦି (m_defaultVertexp) {
                ପାଇଁ (ଅଚଳ V3GraphEdge& edge : m_defaultVertexp->outEdges()) {
                    defaultTargets.emplace(edge.ଶୀର୍ଷ()->as<FsmStateVertex>());
                }
            }
            ଯଦି (m_resetVertexp) {
                ପାଇଁ (ଅଚଳ V3GraphEdge& edge : m_resetVertexp->outEdges()) {
                    resetTargets.emplace(edge.ଶୀର୍ଷ()->as<FsmStateVertex>());
                }
            }
        }
        ଅଚଳ ତାର୍କିକ isFullExpansion = expand == VFsmExpandType::FULL;
        ଯଦି (isFullExpansion) initUndefinedStates();
        ପାଇଁ (ଅଚଳ FsmStateVertex* ଅଚଳ statep : m_states) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* ଅଚଳ declp = buildStateCoverDecl(statep->label());
            initialStateCasep->addItemsp(
                ନୂତନ AstCaseItem{m_flp, makeConst(statep->ମୂଲ୍ୟ().ଅଙ୍କ().toUQuad()),
                                ନୂତନ AstCoverInc{m_flp, declp, ସତ୍ୟ}});
            ଅଚଳ ତାର୍କିକ expandAllOrReset
                = expandAll || (resetTargets.find(statep) != resetTargets.end());
            ଅଚଳ ତାର୍କିକ defaultExpantion
                = !expandAllOrReset && (defaultTargets.find(statep) != defaultTargets.end());
            arcCasep->addItemsp(ନୂତନ AstCaseItem{
                m_flp, makeConst(statep->ମୂଲ୍ୟ().ଅଙ୍କ().toUQuad()),
                Ast_ଗଣ୍ଠି::addNext<Ast_ଗଣ୍ଠି_Stmt>(
                    ନୂତନ AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), isFullExpansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        ଯଦି (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            ଯଦି (isFullExpansion) {
                ପାଇଁ (ଅଚଳ uint64_t ମୂଲ୍ୟ : m_undefinedStates) {
                    ଅଚଳ std::string& label = quadToBitStirngLabel(ମୂଲ୍ୟ);
                    AstCoverOtherDecl* ଅଚଳ declp = buildStateCoverDecl(label);
                    initialStateCasep->addItemsp(ନୂତନ AstCaseItem{
                        m_flp, makeConst(ମୂଲ୍ୟ), ନୂତନ AstCoverInc{m_flp, declp, ସତ୍ୟ}});
                    arcCasep->addItemsp(
                        ନୂତନ AstCaseItem{m_flp, makeConst(ମୂଲ୍ୟ),
                                        Ast_ଗଣ୍ଠି::addNext<Ast_ଗଣ୍ଠି_Stmt>(
                                            ନୂତନ AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, ସତ୍ୟ, ମିଥ୍ୟା))});
                }
            } ଅନ୍ୟଥା {
                // No expansion
                arcCasep->addItemsp(ନୂତନ AstCaseItem{m_flp, nullptr, nullptr});
                initialStateCasep->addItemsp(ନୂତନ AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        m_coverIncp = arcCasep;
        m_initialCoverIncp = initialStateCasep;
    }
    VL_UNCOPYABLE(FsmCoverIncBuilder);
    ~FsmCoverIncBuilder() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    Ast_ଗଣ୍ଠି_Cover_Decl* getCoverDeclsp() ଅଚଳ { ଫେରନ୍ତୁ m_declsp; }
    Ast_ଗଣ୍ଠି_Stmt* getCoverIncp() ଅଚଳ { ଫେରନ୍ତୁ m_coverIncp; }
    Ast_ଗଣ୍ଠି_Stmt* getInitialCoverIncp() ଅଚଳ { ଫେରନ୍ତୁ m_initialCoverIncp; }
};

// Lower the completed FSM graphs into the concrete coverage declarations,
// previous-state tracking, and pre/post-triggered instrumentation that the
// runtime uses to record state and transition coverage.
ଶ୍ରେଣୀ FsmLowerVisitor final {
    // STATE - across all visitors
    ଅଚଳ FsmState& m_state;
    V3SharedTmps m_prevTmps{"__VfsmCovPrev", VVarType::MODULETEMP};  // Previous states

    // METHODS
    // Rebuild the original event control from the saved sense description so
    // post-state coverage sampling runs on the same triggering edges.
    ସ୍ଥିର Ast_Sen_ବୃକ୍ଷ* build_Sen_ବୃକ୍ଷ(File_ଧାଡ଼ି* flp, ଅଚଳ std::vector<FsmSenDesc>& senses) {
        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ sentreep = ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, nullptr};
        ପାଇଁ (ଅଚଳ FsmSenDesc& sense : senses) {
            AstSenItem* ଅଚଳ senItemp
                = ନୂତନ AstSenItem{flp, VEdgeType{sense.edgeType},
                                 ନୂତନ AstVarRef{flp, sense.varScopep, VAccess::READ}};
            sentreep->addSensesp(senItemp);
        }
        ଫେରନ୍ତୁ sentreep;
    }

    // Lower one fully detected FSM graph into the concrete coverage machinery
    // used by generated models: declarations, previous-state tracking, and the
    // pre/post-triggered increment logic for states and arcs.
    ଶୂନ୍ୟ buildOne(ଅଚଳ FsmGraph& graph) {
        UINFO(1, "buildOne lowering FSM " << graph.stateVarName()
                                          << " vertices=" << graph.vertices().size() << endl);
        AstAlways* ଅଚଳ alwaysp = graph.stateAlwaysp();
        AstScope* ଅଚଳ scopep = graph.scopep();
        AstVarScope* ଅଚଳ stateVscp = graph.stateVarScopep();
        AstVarScope* ଅଚଳ sampleVscp = graph.sampleVarScopep();
        File_ଧାଡ଼ି* ଅଚଳ flp = graph.fileline();
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = scopep->modp();
        AstVarScope* ଅଚଳ prevVscp
            = m_prevTmps.make(flp, scopep, sampleVscp->dtypep(), stateVscp->varp()->shortName());
        // The saved previous-state temp crosses the scheduler's pre/post split
        // in the same way as Verilator's built-in NBA shadow variables, so keep
        // both vars marked as post-life participants for stable MT ordering.
        stateVscp->optimizeLifePost(ସତ୍ୟ);
        sampleVscp->optimizeLifePost(ସତ୍ୟ);
        prevVscp->optimizeLifePost(ସତ୍ୟ);

        AstActive* ଅଚଳ initActivep
            = ନୂତନ AstActive{flp, "fsm-coverage-init",
                            ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, ନୂତନ AstSenItem{flp, AstSenItem::Initial{}}}};
        initActivep->sen_ବୃକ୍ଷ_Storep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->addStmtsp(ନୂତନ AstInitialStatic{
            flp, ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, prevVscp, VAccess::WRITE},
                               ନୂତନ AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->addBlocksp(initActivep);

        AstAlwaysPost* ଅଚଳ covPostp = ନୂତନ AstAlwaysPost{flp};
        ତାର୍କିକ updatePrevAfterPost = ମିଥ୍ୟା;
        ଯଦି (alwaysp) {
            // Save the previous state as plain sequential logic at the front of
            // the original always_ff body, then evaluate coverage in post logic
            // after the delayed state update commits. This avoids a scheduler
            // race between a separate AstAlwaysPre task and the real state
            // commit for direct parent-level registers.
            alwaysp->prependStmtsp(ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, prevVscp, VAccess::WRITE},
                                                 ନୂତନ AstVarRef{flp, sampleVscp, VAccess::READ}});
        } ଅନ୍ୟଥା {
            // Wrapper-derived register candidates do not have a parent
            // always_ff body to splice into. Sample coverage first, then save
            // the current state for the next clock tick; this survives cell
            // boundary scheduling where the real flop update lives elsewhere.
            updatePrevAfterPost = ସତ୍ୟ;
            prevVscp->varp()->setIgnorePostRead();
        }

        {
            // Create and increment proper AstNodeCoverDecls
            ଅଚଳ FsmCoverIncBuilder coverIncBuilder{graph, prevVscp};
            modp->addStmtsp(coverIncBuilder.getCoverDeclsp());
            initActivep->addStmtsp(coverIncBuilder.getInitialCoverIncp());
            covPostp->addStmtsp(
                ନୂତନ AstIf{flp,
                          ନୂତନ AstNeq{flp, ନୂତନ AstVarRef{flp, prevVscp, VAccess::READ},
                                     ନୂତନ AstVarRef{flp, sampleVscp, VAccess::READ}},
                          coverIncBuilder.getCoverIncp()});
        }

        ଯଦି (updatePrevAfterPost) {
            covPostp->addStmtsp(ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, prevVscp, VAccess::WRITE},
                                              ନୂତନ AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ sentreep = build_Sen_ବୃକ୍ଷ(flp, graph.senses());
        AstActive* ଅଚଳ activep = ନୂତନ AstActive{flp, "fsm-coverage", sentreep};
        activep->sen_ବୃକ୍ଷ_Storep(sentreep);
        scopep->addBlocksp(activep);
        activep->addStmtsp(covPostp);
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    // Lower every detected FSM graph from the shared local state into
    // concrete coverage instrumentation while the saved scoped pointers are
    // still valid in the same pass.
    explicit FsmLowerVisitor(ଅଚଳ FsmState& state)
        : m_state{state} {
        ପାଇଁ (ଅଚଳ DetectedFsm& fsm : m_state.fsms()) { buildOne(*fsm.graphp); }
    }
};

}  // namespace

ଶୂନ୍ୟ V3FsmDetect::detect(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    FsmState state;
    // Phase 1: recover each supported FSM into a complete graph while the
    // original clocked/case structure is still easy to recognize.
    FsmDetectVisitor detect{state, rootp};
    ଯଦି (dumpGraphLevel() >= 6) {
        size_t index = 0;
        ପାଇଁ (ଅଚଳ DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dumpTag(index++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { FsmLowerVisitor lower{state}; }
    V3Global::dump_Check_Global_ବୃକ୍ଷ("fsm-detect", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
