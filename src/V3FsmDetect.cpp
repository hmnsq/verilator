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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"

#ಅನ್ತರ್ಭಾವ "V3FsmDetect.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Control.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3SharedTmps.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <cctype>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

ನಾಮಾಕಾಶ {

// Width-preserving FSM state identity. FSM detection needs a stable key for
// graph vertices and lookup tables, but lowering still needs the original
// folded Verilog value so emitted comparisons keep the correct width and bits.
ವರ್ಗ Fsm_State_ಮೌಲ್ಯ final {
    // Hash/equality key only. It deliberately ignores signedness because
    // signed and unsigned constants with the same width and bits denote the
    // same encoded FSM state.
    string m_ಕೀಲಿ;  // Canonical "width:value" identity, independent of signedness

    // Semantic value. This is what diagnostics and lowering use when printing
    // values or rebuilding AstConst nodes for instrumentation.
    V3_ಸಂಖ್ಯೆ m_ಅಂಕೆ;  // Original folded value, preserving width for lowered comparisons

    ಸ್ಥಿರ string make_ಕೀಲಿ(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ) {
        V3_ಸಂಖ್ಯೆ ಕೀಲಿ_ಅಂಕೆ = ಅಂಕೆ;
        // Signedness does not change FSM state identity: same width and bits
        // should address the same graph vertex.
        ಕೀಲಿ_ಅಂಕೆ.isSigned(ಅಸತ್ಯ);
        ಪ್ರತ್ಯಾಗಮ cvtToStr(ಕೀಲಿ_ಅಂಕೆ.width()) + ":" + ಕೀಲಿ_ಅಂಕೆ.ascii(ಸತ್ಯ, ಸತ್ಯ);
    }

ಸಾರ್ವಜನಿಕ:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    Fsm_State_ಮೌಲ್ಯ()
        : m_ಕೀಲಿ{"1:1'h0"}
        , m_ಅಂಕೆ{static_cast<Ast_ಗಂಟು*>(nullptr), 1, 0} {}
    explicit Fsm_State_ಮೌಲ್ಯ(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ)
        : m_ಕೀಲಿ{make_ಕೀಲಿ(ಅಂಕೆ)}
        , m_ಅಂಕೆ{ಅಂಕೆ} {}

    ಧ್ರುವ string& ಕೀಲಿ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಕೀಲಿ; }
    ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಅಂಕೆ; }
    string ascii() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಅಂಕೆ.ascii(ಸತ್ಯ, ಸತ್ಯ); }
    string warnText() ಧ್ರುವ {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        ಯದಿ (m_ಅಂಕೆ.width() <= 32) ಪ್ರತ್ಯಾಗಮ cvtToStr(m_ಅಂಕೆ.toUInt());
        ಪ್ರತ್ಯಾಗಮ ascii();
    }

    ತಾರ್ಕಿಕ operator==(ಧ್ರುವ Fsm_State_ಮೌಲ್ಯ& rhs) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಕೀಲಿ == rhs.m_ಕೀಲಿ; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
ಸಂರಚನಾ Fsm_State_ಮೌಲ್ಯ_Hash final {
    size_t operator()(ಧ್ರುವ Fsm_State_ಮೌಲ್ಯ& ಮೌಲ್ಯ) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ std::hash<string>{}(ಮೌಲ್ಯ.ಕೀಲಿ());
    }
};

// Captures one sensitivity-list entry so the lowering phase can later rebuild
// an active block with the same triggering event control.
ಸಂರಚನಾ FsmSenDesc final {
    // Encoded edge kind copied from AstSenItem::edgeType() so lowering can
    // rebuild the same trigger semantics on the synthesized coverage block.
    VEdgeType::en edgeType = static_cast<VEdgeType::en>(0);
    // Triggering signal in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
};

// Captures the simple reset predicate shape that survives to this pass after
// earlier normalization so reset arcs can be reconstructed during lowering.
ಸಂರಚನಾ FsmResetCondDesc final {
    // Reset signal used by the FSM in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
    ತಾರ್ಕಿಕ active_ಕೆಳಗಿನ = ಅಸತ್ಯ;
};

ವರ್ಗ FsmResetArcDesc final {
    Fsm_State_ಮೌಲ್ಯ m_to_ಮೌಲ್ಯ;  // Encoded reset target state.
    Ast_ಗಂಟು* m_nodep = nullptr;  // Source node for warnings and emitted metadata.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_valuep = nullptr;  // Expression that provided the reset value.

ಸಾರ್ವಜನಿಕ:
    FsmResetArcDesc() = ಸ್ವಭಾವ;
    FsmResetArcDesc(Fsm_State_ಮೌಲ್ಯ to_ಮೌಲ್ಯ, Ast_ಗಂಟು_Assign* nodep)
        : m_to_ಮೌಲ್ಯ{to_ಮೌಲ್ಯ}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(Fsm_State_ಮೌಲ್ಯ to_ಮೌಲ್ಯ, Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : m_to_ಮೌಲ್ಯ{to_ಮೌಲ್ಯ}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    Fsm_State_ಮೌಲ್ಯ to_ಮೌಲ್ಯ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_to_ಮೌಲ್ಯ; }
    Ast_ಗಂಟು* nodep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_nodep; }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_valuep; }
};

ಸಂರಚನಾ FsmWrapperRoles final {
    string dPort;
    string qPort;
    string clkPort;
    string rstPort;
    string rstValParam;
    ತಾರ್ಕಿಕ has_Rst_Active_ಕೆಳಗಿನ = ಅಸತ್ಯ;
    ತಾರ್ಕಿಕ rst_Active_ಕೆಳಗಿನ = ಅಸತ್ಯ;
};

ಸ್ಥಿರ ತಾರ್ಕಿಕ fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, ಧ್ರುವ string& portName,
                                                  ತಾರ್ಕಿಕ& active_ಕೆಳಗಿನ) {
    ತಾರ್ಕಿಕ matched = ಅಸತ್ಯ;
    cellp->modp()->foreach([&](AstSenItem* itemp) {
        Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ vrefp = itemp->varrefp();
        ಯದಿ (!vrefp) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (vrefp->varp()->name() != portName) ಪ್ರತ್ಯಾಗಮ;
        active_ಕೆಳಗಿನ = itemp->edgeType() == VEdgeType::ET_NEGEDGE;
        matched = ಸತ್ಯ;
    });
    ಪ್ರತ್ಯಾಗಮ matched;
}

ಸ್ಥಿರ ಧ್ರುವ V3Control::FsmRegisterWrapper* fsmRegisterWrapperDesc(AstCell* cellp) {
    Ast_ಗಂಟು_Module* ಧ್ರುವ modp = cellp->modp();
    ಧ್ರುವ string origName = modp->origName();
    ಯದಿ (ಧ್ರುವ V3Control::FsmRegisterWrapper* ಧ್ರುವ descp
        = V3Control::getFsmRegisterWrapper(origName)) {
        ಪ್ರತ್ಯಾಗಮ descp;
    }
    ಪ್ರತ್ಯಾಗಮ V3Control::getFsmRegisterWrapper(modp->prettyDehashOrigOrName());
}

ಸ್ಥಿರ FsmWrapperRoles rolesFromDesc(ಧ್ರುವ V3Control::FsmRegisterWrapper& desc) {
    FsmWrapperRoles roles;
    roles.dPort = desc.d;
    roles.qPort = desc.q;
    roles.clkPort = desc.clock;
    roles.rstPort = desc.reset;
    roles.rstValParam = desc.reset_ಮೌಲ್ಯ;
    ಪ್ರತ್ಯಾಗಮ roles;
}

ವರ್ಗ FsmRegisterCandidate final {
    AstScope* m_scopep = nullptr;  // Owning scope for the paired FSM.
    AstAlways* m_alwaysp = nullptr;  // Register process that commits the state.
    AstVarScope* m_stateVscp = nullptr;  // Registered FSM state variable.
    AstVarScope* m_sampleVscp = nullptr;  // Variable sampled by lowered coverage logic.
    AstVarScope* m_nextVscp = nullptr;  // Next-state variable or same state var for 1-block FSMs.
    std::vector<FsmSenDesc> m_senses;  // Event controls for recreated coverage blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate, if any.
    std::vector<FsmResetArcDesc> m_resetArcs;  // Reset target arcs recovered during detect.
    ತಾರ್ಕಿಕ m_hasResetCond = ಅಸತ್ಯ;  // Whether the FSM had a modeled reset predicate.
    ತಾರ್ಕಿಕ m_inclCond = ಅಸತ್ಯ;  // Whether conditional/default arcs are kept explicitly.
    File_ಸಾಲು* m_flp = nullptr;  // Representative source location.

ಸಾರ್ವಜನಿಕ:
    AstScope* scopep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_scopep; }
    ಶೂನ್ಯ scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* alwaysp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_alwaysp; }
    ಶೂನ್ಯ alwaysp(AstAlways* alwaysp) { m_alwaysp = alwaysp; }
    AstVarScope* stateVscp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stateVscp; }
    ಶೂನ್ಯ stateVscp(AstVarScope* vscp) { m_stateVscp = vscp; }
    AstVarScope* sampleVscp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_sampleVscp ? m_sampleVscp : m_stateVscp; }
    ಶೂನ್ಯ sampleVscp(AstVarScope* vscp) { m_sampleVscp = vscp; }
    AstVarScope* nextVscp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_nextVscp; }
    ಶೂನ್ಯ nextVscp(AstVarScope* vscp) { m_nextVscp = vscp; }
    ಧ್ರುವ std::vector<FsmSenDesc>& senses() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_senses; }
    std::vector<FsmSenDesc>& senses() { ಪ್ರತ್ಯಾಗಮ m_senses; }
    ಧ್ರುವ FsmResetCondDesc& resetCond() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_resetCond; }
    FsmResetCondDesc& resetCond() { ಪ್ರತ್ಯಾಗಮ m_resetCond; }
    ಧ್ರುವ std::vector<FsmResetArcDesc>& resetArcs() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_resetArcs; }
    std::vector<FsmResetArcDesc>& resetArcs() { ಪ್ರತ್ಯಾಗಮ m_resetArcs; }
    ತಾರ್ಕಿಕ hasResetCond() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hasResetCond; }
    ಶೂನ್ಯ hasResetCond(ತಾರ್ಕಿಕ flag) { m_hasResetCond = flag; }
    ತಾರ್ಕಿಕ inclCond() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_inclCond; }
    ಶೂನ್ಯ inclCond(ತಾರ್ಕಿಕ flag) { m_inclCond = flag; }
    File_ಸಾಲು* fileline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_flp; }
    ಶೂನ್ಯ fileline(File_ಸಾಲು* flp) { m_flp = flp; }
};

ವರ್ಗ FsmComboAlways final {
    AstScope* ಧ್ರುವ m_scopep = nullptr;  // Owning scope for the combinational process.
    AstAlways* ಧ್ರುವ m_alwaysp = nullptr;  // Candidate transition process.

ಸಾರ್ವಜನಿಕ:
    FsmComboAlways() = ಸ್ವಭಾವ;
    FsmComboAlways(AstScope* scopep, AstAlways* alwaysp)
        : m_scopep{scopep}
        , m_alwaysp{alwaysp} {}

    AstScope* scopep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_scopep; }
    AstAlways* alwaysp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_alwaysp; }
};

ವರ್ಗ FsmGraph;

ವರ್ಗ FsmVertex VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(FsmVertex, V3GraphVertex)

ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ ವರ್ಗ Kind : uint8_t { STATE, RESET_ANY, DEFAULT_ANY };

ಗುಪ್ತ:
    Kind m_kind;  // State vs synthetic ANY/default vertex role.
    string m_label;  // User-facing state or pseudo-state label.
    Fsm_State_ಮೌಲ್ಯ m_ಮೌಲ್ಯ;  // Encoded state value for real state vertices.

ರಕ್ಷಿತ:
    FsmVertex(V3Graph* graphp, Kind kind, string label, Fsm_State_ಮೌಲ್ಯ ಮೌಲ್ಯ) VL_MT_DISABLED
        : V3GraphVertex{graphp},
          m_kind{kind},
          m_label{label},
          m_ಮೌಲ್ಯ{ಮೌಲ್ಯ} {}
    ~FsmVertex() override = ಸ್ವಭಾವ;

ಸಾರ್ವಜನಿಕ:
    Kind kind() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_kind; }
    ತಾರ್ಕಿಕ isState() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_kind == Kind::STATE; }
    ತಾರ್ಕಿಕ isResetAny() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_kind == Kind::RESET_ANY; }
    ತಾರ್ಕಿಕ isDefaultAny() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_kind == Kind::DEFAULT_ANY; }
    ಧ್ರುವ string& label() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_label; }
    ಧ್ರುವ Fsm_State_ಮೌಲ್ಯ& ಮೌಲ್ಯ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಮೌಲ್ಯ; }

    string name() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_label + "=" + m_ಮೌಲ್ಯ.ascii(); }
};

ವರ್ಗ FsmStateVertex final : ಸಾರ್ವಜನಿಕ FsmVertex {
    VL_RTTI_IMPL(FsmStateVertex, FsmVertex)

ಸಾರ್ವಜನಿಕ:
    FsmStateVertex(V3Graph* graphp, string label, Fsm_State_ಮೌಲ್ಯ ಮೌಲ್ಯ) VL_MT_DISABLED
        : FsmVertex{graphp, Kind::STATE, label, ಮೌಲ್ಯ} {}
    ~FsmStateVertex() override = ಸ್ವಭಾವ;

    string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "lightblue"; }
    string dotShape() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "ellipse"; }
};

ವರ್ಗ FsmPseudoVertex final : ಸಾರ್ವಜನಿಕ FsmVertex {
    VL_RTTI_IMPL(FsmPseudoVertex, FsmVertex)

ಸಾರ್ವಜನಿಕ:
    FsmPseudoVertex(V3Graph* graphp, Kind kind, string label) VL_MT_DISABLED
        : FsmVertex{graphp, kind, label, Fsm_State_ಮೌಲ್ಯ{}} {}
    ~FsmPseudoVertex() override = ಸ್ವಭಾವ;

    string name() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ label(); }
    string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ isResetAny() ? "darkgreen" : "orange"; }
    string dotShape() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "diamond"; }
};

ವರ್ಗ FsmArcEdge final : ಸಾರ್ವಜನಿಕ V3GraphEdge {
    VL_RTTI_IMPL(FsmArcEdge, V3GraphEdge)
    ತಾರ್ಕಿಕ m_isReset = ಅಸತ್ಯ;  // Arc originates from the synthetic reset source.
    ತಾರ್ಕಿಕ m_isCond = ಅಸತ್ಯ;  // Arc came from a conditional next-state split.
    ತಾರ್ಕಿಕ m_isDefault = ಅಸತ್ಯ;  // Arc represents a case default source.
    File_ಸಾಲು* m_flp = nullptr;  // Source location for emitted coverage metadata.

ಸಾರ್ವಜನಿಕ:
    FsmArcEdge(V3Graph* graphp, FsmVertex* fromp, FsmStateVertex* ತುದಿ, ತಾರ್ಕಿಕ isReset, ತಾರ್ಕಿಕ isCond,
               ತಾರ್ಕಿಕ isDefault, File_ಸಾಲು* flp) VL_MT_DISABLED : V3GraphEdge{graphp, fromp, ತುದಿ, 1},
                                                               m_isReset{isReset},
                                                               m_isCond{isCond},
                                                               m_isDefault{isDefault},
                                                               m_flp{flp} {}
    ~FsmArcEdge() override = ಸ್ವಭಾವ;

    ತಾರ್ಕಿಕ isReset() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isReset; }
    ತಾರ್ಕಿಕ isCond() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isCond; }
    ತಾರ್ಕಿಕ isDefault() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isDefault; }
    File_ಸಾಲು* fileline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_flp; }

    string dotLabel() ಧ್ರುವ override {
        ಯದಿ (m_isReset) ಪ್ರತ್ಯಾಗಮ "reset";
        ಯದಿ (m_isDefault) ಪ್ರತ್ಯಾಗಮ "default";
        ಯದಿ (m_isCond) ಪ್ರತ್ಯಾಗಮ "cond";
        ಪ್ರತ್ಯಾಗಮ "";
    }
    string dotColor() ಧ್ರುವ override {
        ಯದಿ (m_isReset) ಪ್ರತ್ಯಾಗಮ "darkgreen";
        ಯದಿ (m_isDefault) ಪ್ರತ್ಯಾಗಮ "orange";
        ಯದಿ (m_isCond) ಪ್ರತ್ಯಾಗಮ "blue";
        ಪ್ರತ್ಯಾಗಮ "black";
    }
};

// One graph per detected FSM. Graph-level metadata captures the non-graph
// context needed to lower states/arcs back into the AST after detection.
ವರ್ಗ FsmGraph final : ಸಾರ್ವಜನಿಕ V3Graph {
    AstScope* m_scopep = nullptr;  // Owning scoped block for the detected FSM.
    AstAlways* m_stateAlwaysp = nullptr;  // Register always block being instrumented.
    string m_stateVarName;  // Pretty state variable name for user-visible output.
    string m_stateVarInternalName;  // Internal state symbol name for dump tags.
    AstVarScope* m_stateVarScopep = nullptr;  // Scoped state variable being tracked.
    AstVarScope* m_sampleVarScopep = nullptr;  // Scoped variable sampled by coverage logic.
    std::vector<FsmSenDesc> m_senses;  // Saved event controls for recreated active blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate shape, if one exists.
    ತಾರ್ಕಿಕ m_hasResetCond = ಅಸತ್ಯ;  // Whether the detected FSM had a reset branch.
    ತಾರ್ಕಿಕ m_inclCond = ಅಸತ್ಯ;  // Whether conditional arcs should be kept explicitly.
    File_ಸಾಲು* m_flp = nullptr;  // Representative source location for declarations/arcs.
    std::unordered_map<Fsm_State_ಮೌಲ್ಯ, FsmStateVertex*, Fsm_State_ಮೌಲ್ಯ_Hash>
        m_stateVertices;  // Value to state map.
    FsmPseudoVertex* m_resetVertexp = nullptr;  // Synthetic ANY source for reset arcs.
    FsmPseudoVertex* m_defaultVertexp = nullptr;  // Synthetic default source for case defaults.

ಸಾರ್ವಜನಿಕ:
    FsmGraph() VL_MT_DISABLED
        : m_resetVertexp{ನೂತನ FsmPseudoVertex{ಏತತ್, FsmVertex::Kind::RESET_ANY, "ANY"}},
          m_defaultVertexp{ನೂತನ FsmPseudoVertex{ಏತತ್, FsmVertex::Kind::DEFAULT_ANY, "default"}} {}

    AstScope* scopep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_scopep; }
    ಶೂನ್ಯ scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* stateAlwaysp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stateAlwaysp; }
    ಶೂನ್ಯ stateAlwaysp(AstAlways* alwaysp) { m_stateAlwaysp = alwaysp; }
    ಧ್ರುವ string& stateVarName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stateVarName; }
    ಶೂನ್ಯ stateVarName(ಧ್ರುವ string& name) { m_stateVarName = name; }
    ಧ್ರುವ string& stateVarInternalName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stateVarInternalName; }
    ಶೂನ್ಯ stateVarInternalName(ಧ್ರುವ string& name) { m_stateVarInternalName = name; }
    AstVarScope* stateVarScopep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stateVarScopep; }
    ಶೂನ್ಯ stateVarScopep(AstVarScope* vscp) { m_stateVarScopep = vscp; }
    AstVarScope* sampleVarScopep() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ m_sampleVarScopep ? m_sampleVarScopep : m_stateVarScopep;
    }
    ಶೂನ್ಯ sampleVarScopep(AstVarScope* vscp) { m_sampleVarScopep = vscp; }
    ಧ್ರುವ std::vector<FsmSenDesc>& senses() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_senses; }
    std::vector<FsmSenDesc>& senses() { ಪ್ರತ್ಯಾಗಮ m_senses; }
    ಧ್ರುವ FsmResetCondDesc& resetCond() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_resetCond; }
    FsmResetCondDesc& resetCond() { ಪ್ರತ್ಯಾಗಮ m_resetCond; }
    ತಾರ್ಕಿಕ hasResetCond() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hasResetCond; }
    ಶೂನ್ಯ hasResetCond(ತಾರ್ಕಿಕ flag) { m_hasResetCond = flag; }
    ತಾರ್ಕಿಕ inclCond() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_inclCond; }
    ಶೂನ್ಯ inclCond(ತಾರ್ಕಿಕ flag) { m_inclCond = flag; }
    File_ಸಾಲು* fileline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_flp; }
    ಶೂನ್ಯ fileline(File_ಸಾಲು* flp) { m_flp = flp; }

    FsmStateVertex* addStateVertex(string label, Fsm_State_ಮೌಲ್ಯ ಮೌಲ್ಯ) VL_MT_DISABLED {
        FsmStateVertex* ಧ್ರುವ vertexp = ನೂತನ FsmStateVertex{ಏತತ್, label, ಮೌಲ್ಯ};
        m_stateVertices.emplace(ಮೌಲ್ಯ, vertexp);
        ಪ್ರತ್ಯಾಗಮ vertexp;
    }
    FsmPseudoVertex* resetAnyVertex() VL_MT_DISABLED { ಪ್ರತ್ಯಾಗಮ m_resetVertexp; }
    FsmPseudoVertex* defaultAnyVertex() VL_MT_DISABLED { ಪ್ರತ್ಯಾಗಮ m_defaultVertexp; }
    FsmArcEdge* addArc(Fsm_State_ಮೌಲ್ಯ from_ಮೌಲ್ಯ, Fsm_State_ಮೌಲ್ಯ to_ಮೌಲ್ಯ, ತಾರ್ಕಿಕ isReset, ತಾರ್ಕಿಕ isCond,
                       ತಾರ್ಕಿಕ isDefault, File_ಸಾಲು* flp) VL_MT_DISABLED {
        FsmStateVertex* ಧ್ರುವ ತುದಿ = m_stateVertices.at(to_ಮೌಲ್ಯ);
        FsmVertex* fromp = nullptr;
        ಯದಿ (isReset) {
            fromp = resetAnyVertex();
        } ಅನ್ಯಥಾ ಯದಿ (isDefault) {
            fromp = defaultAnyVertex();
        } ಅನ್ಯಥಾ {
            fromp = m_stateVertices.at(from_ಮೌಲ್ಯ);
        }
        ಪ್ರತ್ಯಾಗಮ ನೂತನ FsmArcEdge{ಏತತ್, fromp, ತುದಿ, isReset, isCond, isDefault, flp};
    }

    string name() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ "FSM "
               + (m_stateVarName.empty() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dumpTag(size_t index) ಧ್ರುವ {
        string tag = stateVarInternalName();
        ಪ್ರತಿ (ವರ್ಣ& ch : tag) {
            ಯದಿ (!std::isalnum(static_cast<unsigned ವರ್ಣ>(ch))) ch = '_';
        }
        ಪ್ರತ್ಯಾಗಮ "fsm_" + cvtToStr(index) + "_" + tag;
    }
};

ಸಂರಚನಾ DetectedFsm final {
    std::unique_ptr<FsmGraph> graphp;  // Extracted graph for one detected FSM candidate.
};

ಸಂರಚನಾ FsmCaseCandidate final {
    Ast_ಗಂಟು* warnNodep = nullptr;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = nullptr;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
ಸಂರಚನಾ FsmStateComparison final {
    AstVarScope* stateVscp = nullptr;  // Compared state variable
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = nullptr;  // Compared constant value expression
    Fsm_State_ಮೌಲ್ಯ ಮೌಲ್ಯ;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
ಸಂರಚನಾ FsmIfBranch final {
    AstIf* ifp = nullptr;  // Source if/else-if node
    Ast_ಗಂಟು* stmtsp = nullptr;  // Branch body
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = nullptr;  // Source state value expression
    Fsm_State_ಮೌಲ್ಯ from_ಮೌಲ್ಯ;  // Encoded source state value
    ತಾರ್ಕಿಕ has_ತುದಿ_Guard = ಅಸತ್ಯ;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
ಸಂರಚನಾ FsmIfChainCandidate final {
    AstIf* ifp = nullptr;  // Top-level if-chain node
    AstVarScope* compareVscp = nullptr;  // Variable used by every state comparison
    std::vector<FsmIfBranch> branches;  // Recognized state-dispatch branches
    Ast_ಗಂಟು* defaultStmtsp = nullptr;  // Optional final else body
};

// Aliases are accepted only when they are equivalent to spelling the state
// comparison inline; this avoids inferring FSM semantics from arbitrary logic.
using FsmAliasMap = std::unordered_map<ಧ್ರುವ AstVarScope*, FsmStateComparison>;
using FsmCellPortMap = std::unordered_map<string, AstVarScope*>;
using FsmCellPortAliasMap = std::unordered_map<ಧ್ರುವ AstCell*, FsmCellPortMap>;

ಸಂರಚನಾ StateConstLabel final {
    string text;
    ತಾರ್ಕಿಕ fromParam = ಅಸತ್ಯ;
    size_t stateIndex = 0;
};

ಸಂರಚನಾ FsmStateSpace final {
    std::vector<std::pair<string, Fsm_State_ಮೌಲ್ಯ>> states;  // User label and encoded value
    std::unordered_map<Fsm_State_ಮೌಲ್ಯ, StateConstLabel, Fsm_State_ಮೌಲ್ಯ_Hash>
        labels;  // Encoded value to label
    AstVar* stateVarp = nullptr;  // Tracked FSM state variable
    ತಾರ್ಕಿಕ enumBacked = ಅಸತ್ಯ;  // Whether states came from an enum declaration
};

// Local shared state between the two adjacent FSM coverage phases. Detection
// fills this with recovered FSM graphs; lowering consumes the completed graphs
// immediately afterward without needing any AST serialization bridge.
ವರ್ಗ FsmState final {
    // All detected FSMs in discovery order. This is the only bridge between
    // the adjacent detect and lower phases, so the second phase never needs to
    // rediscover or serialize the extracted machine.
    std::vector<DetectedFsm> m_fsms;
    std::map<ಧ್ರುವ AstVarScope*, size_t> m_fsmIndex;

ಸಾರ್ವಜನಿಕ:
    DetectedFsm& fsmFor(AstVarScope* stateVscp) {
        ಧ್ರುವ std::map<ಧ್ರುವ AstVarScope*, size_t>::const_iterator it = m_fsmIndex.find(stateVscp);
        ಯದಿ (it != m_fsmIndex.end()) ಪ್ರತ್ಯಾಗಮ m_fsms.at(it->second);
        ಧ್ರುವ size_t index = m_fsms.size();
        m_fsmIndex.emplace(stateVscp, index);
        m_fsms.emplace_back();
        ಪ್ರತ್ಯಾಗಮ m_fsms.back();
    }
    ಧ್ರುವ std::vector<DetectedFsm>& fsms() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fsms; }
};

// Detection runs while the original clocked/case structure is still intact and
// populates graph-backed FSM models without mutating the tree mid-traversal.
// This pass is intentionally conservative: for this PR we only lock down the
// small set of transition/selector forms that are already stable in the
// normalized AST we see here. The remaining reject branches are therefore
// mostly future-feature boundaries, not accidental dead code.
ವರ್ಗ FsmDetectVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
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
    std::unordered_map<ಧ್ರುವ AstVarScope*, FsmCaseCandidate> m_comboPaired;
    // Continuous aliases are order-independent, while procedural aliases must
    // remain source-order scoped to avoid using assignments not yet executed.
    FsmAliasMap m_stateAliases;
    std::unordered_set<ಧ್ರುವ AstVarScope*> m_ambiguousStateAliases;
    // A surviving wrapper's semantic d/q relationship is split across the
    // parent scope and the child module scope. This table is the narrow bridge
    // between those scopes: only transparent port aliases are recorded, so the
    // detector does not become a general cross-module dataflow engine.
    FsmCellPortAliasMap m_cellPortAliases;
    FsmCellPortAliasMap m_cellPortChildAliases;

    // METHODS
    // Enum-backed FSMs may be wrapped in refs/typedefs; normalize to the
    // underlying enum type before deciding whether a case is a candidate.
    ಸ್ಥಿರ Ast_ಗಂಟು_DType* unwrapEnumCandidate(Ast_ಗಂಟು_DType* dtypep) {
        ಪ್ರತ್ಯಾಗಮ dtypep->skipRefToEnump();
    }

    ಸ್ಥಿರ string candidateConflictContext(Ast_ಗಂಟು* laterNodep,
                                           ಧ್ರುವ FsmCaseCandidate& firstCand) {
        ಪ್ರತ್ಯಾಗಮ '\n' + laterNodep->warnContextPrimary() + firstCand.warnNodep->warnOther()
               + "... Location of first supported candidate for "
               + firstCand.stateVscp->prettyNameQ() + '\n'
               + firstCand.warnNodep->warnContextSecondary();
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ rejectFsmWrapperCell(AstCell* cellp, ಧ್ರುವ string& reason) {
        cellp->v3warn(COVERIGN, "Ignoring unsupported: " + reason);
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ simple_Param_State_ಮೌಲ್ಯ(AstCell* cellp, ಧ್ರುವ string& name, Fsm_State_ಮೌಲ್ಯ& ಮೌಲ್ಯ,
                                      Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = nullptr;
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ಧ್ರುವ varp = VN_CAST(stmtp, Var);
            ಯದಿ (!varp || !varp->isParam() || varp->name() != name) ಅನುವರ್ತ;
            valuepr = VN_AS(varp->valuep(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            ಪ್ರತ್ಯಾಗಮ const_ಮೌಲ್ಯ_Status(valuepr, ಮೌಲ್ಯ) == Const_ಮೌಲ್ಯ_Status::OK;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ childPortInScope(AstVarScope* vscp, AstScope* parentScopep, AstCell*& cellpr) {
        ಯದಿ (!vscp->varp()->isIO()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        AstScope* ಧ್ರುವ scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        ಯದಿ (scopep->aboveScopep() != parentScopep) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಸ್ಥಿರ AstVarScope* simpleAssignVarScope(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        AstVarRef* ಧ್ರುವ vrefp = VN_CAST(exprp, VarRef);
        ಪ್ರತ್ಯಾಗಮ vrefp ? vrefp->varScopep() : nullptr;
    }

    ಶೂನ್ಯ addWrapperCell(AstScope* scopep, AstCell* cellp) {
        m_cellPortAliases.emplace(cellp, FsmCellPortMap{});
        m_cellPortChildAliases.emplace(cellp, FsmCellPortMap{});
        ಧ್ರುವ std::pair<AstScope*, AstCell*> item{scopep, cellp};
        ಯದಿ (std::find(m_wrapperCells.cbegin(), m_wrapperCells.cend(), item)
            != m_wrapperCells.cend()) {
            ಪ್ರತ್ಯಾಗಮ;
        }
        m_wrapperCells.emplace_back(item);
    }

    ಶೂನ್ಯ collectCellPortAlias(AstAssignW* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* ಧ್ರುವ lhsVscp = simpleAssignVarScope(nodep->lhsp());
        AstVarScope* ಧ್ರುವ rhsVscp = simpleAssignVarScope(nodep->rhsp());
        ಯದಿ (!lhsVscp || !rhsVscp) ಪ್ರತ್ಯಾಗಮ;
        AstCell* cellp = nullptr;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        ಯದಿ (childPortInScope(lhsVscp, m_scopep, cellp)) {
            ಯದಿ (!fsmRegisterWrapperDesc(cellp)) ಪ್ರತ್ಯಾಗಮ;
            UASSERT_OBJ(lhsVscp->varp()->is_ಒಳಹರಿವು(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cellPortAliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cellPortChildAliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            addWrapperCell(m_scopep, cellp);
        } ಅನ್ಯಥಾ ಯದಿ (childPortInScope(rhsVscp, m_scopep, cellp)) {
            ಯದಿ (!fsmRegisterWrapperDesc(cellp)) ಪ್ರತ್ಯಾಗಮ;
            UASSERT_OBJ(rhsVscp->varp()->isWritable(), nodep,
                        "Child-side port alias rhs should be writable");
            m_cellPortAliases[cellp][rhsVscp->varp()->name()] = lhsVscp;
            m_cellPortChildAliases[cellp][rhsVscp->varp()->name()] = rhsVscp;
            addWrapperCell(m_scopep, cellp);
        }
    }

    AstVarScope* roleVarScope(AstCell* cellp, ಧ್ರುವ string& portName) ಧ್ರುವ {
        // At this point explicit AstPin expressions have been lowered away, so
        // role resolution crosses the wrapper boundary only through the
        // transparent alias table above. This keeps wrapper support aligned with
        // direct-register detection instead of growing into interprocedural FSM
        // inference.
        ಧ್ರುವ FsmCellPortMap& ports = m_cellPortAliases.at(cellp);
        ಧ್ರುವ FsmCellPortMap::const_iterator portIt = ports.find(portName);
        ಪ್ರತ್ಯಾಗಮ portIt == ports.end() ? nullptr : portIt->second;
    }

    AstVarScope* childRoleVarScope(AstCell* cellp, ಧ್ರುವ string& portName) ಧ್ರುವ {
        ಧ್ರುವ FsmCellPortMap& ports = m_cellPortChildAliases.at(cellp);
        ಧ್ರುವ FsmCellPortMap::const_iterator portIt = ports.find(portName);
        ಪ್ರತ್ಯಾಗಮ portIt == ports.end() ? nullptr : portIt->second;
    }

    ವರ್ಗ RegisterAlwaysAnalyzer final {
        AstScope* ಧ್ರುವ m_scopep;  // Scope containing the always block(s) being analyzed

    ಸಾರ್ವಜನಿಕ:
        explicit RegisterAlwaysAnalyzer(AstScope* scopep)
            : m_scopep{scopep} {}

        std::vector<std::pair<AstCase*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>>
        oneBlockCandidates(AstAlways* alwaysp) ಧ್ರುವ {
            std::vector<std::pair<AstCase*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>> candidates;
            Ast_ಗಂಟು* ಧ್ರುವ stmtsp = alwaysp->stmtsp();
            ಯದಿ (AstIf* ಧ್ರುವ firstIfp = VN_CAST(stmtsp, If)) {
                ಯದಿ (AstCase* ಧ್ರುವ casep = VN_CAST(firstIfp->elsesp(), Case)) {
                    candidates.emplace_back(casep,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            ಪ್ರತಿ (Ast_ಗಂಟು* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                ಯದಿ (AstCase* ಧ್ರುವ casep = VN_CAST(nodep, Case))
                    candidates.emplace_back(casep, nullptr);
            }
            ಪ್ರತ್ಯಾಗಮ candidates;
        }

        std::vector<std::pair<AstIf*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>>
        oneBlockIfCandidates(AstAlways* alwaysp) ಧ್ರುವ {
            std::vector<std::pair<AstIf*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>> candidates;
            Ast_ಗಂಟು* ಧ್ರುವ stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            ಯದಿ (AstIf* ಧ್ರುವ firstIfp = VN_CAST(stmtsp, If)) {
                ಯದಿ (AstIf* ಧ್ರುವ chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(firstIfp->elsesp()), If)) {
                    candidates.emplace_back(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            ಪ್ರತಿ (Ast_ಗಂಟು* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                ಯದಿ (AstIf* ಧ್ರುವ ifp = VN_CAST(nodep, If)) candidates.emplace_back(ifp, nullptr);
            }
            ಪ್ರತ್ಯಾಗಮ candidates;
        }

        ತಾರ್ಕಿಕ matchRegisterCandidate(AstAlways* alwaysp, FsmRegisterCandidate& cand) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ FsmDetectVisitor::matchRegisterAlways(alwaysp, m_scopep, cand);
        }

        ಶೂನ್ಯ buildOneBlockCandidate(AstAlways* alwaysp, AstVarScope* vscp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resetCondp,
                                    FsmRegisterCandidate& reg) ಧ್ರುವ {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.nextVscp(vscp);
            reg.senses() = FsmDetectVisitor::describe_Sen_ವೃಕ್ಷ(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != nullptr);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* ಧ್ರುವ firstIfp = VN_CAST(alwaysp->stmtsp(), If);
            ಯದಿ (firstIfp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = nullptr;
                ಧ್ರುವ ResetAssignStatus resetStatus = FsmDetectVisitor::collectConstStateAssigns(
                    firstIfp->thensp(), resetStateVscp, reg.resetArcs());
                ಯದಿ (resetStatus == ResetAssignStatus::EMPTY) {
                    reg.resetArcs().clear();
                } ಅನ್ಯಥಾ ಯದಿ (resetStatus == ResetAssignStatus::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().clear();
                    Fsm_State_ಮೌಲ್ಯ reset_ಮೌಲ್ಯ;
                    Ast_ಗಂಟು* ಧ್ರುವ thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(firstIfp->thensp());
                    UASSERT_OBJ(thenNodep, firstIfp,
                                "one-block reset fallback requires a non-empty reset branch");
                    Ast_ಗಂಟು_Assign* ಧ್ರುವ resetAssp = FsmDetectVisitor::direct_Const_State_Assign_ಗಂಟು(
                        thenNodep, resetStateVscp, reset_ಮೌಲ್ಯ);
                    ಯದಿ (resetAssp && resetStateVscp == vscp) {
                        reg.resetArcs().emplace_back(reset_ಮೌಲ್ಯ, resetAssp);
                    }
                } ಅನ್ಯಥಾ ಯದಿ (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                    reg.resetArcs().clear();
                }
            }
        }
    };

    ತಾರ್ಕಿಕ matchFsmWrapperCell(AstScope* scopep, AstCell* cellp, FsmRegisterCandidate& cand) ಧ್ರುವ {
        FsmWrapperRoles roles = rolesFromDesc(*fsmRegisterWrapperDesc(cellp));

        AstVarScope* ಧ್ರುವ nextVscp = roleVarScope(cellp, roles.dPort);
        AstVarScope* ಧ್ರುವ stateVscp = roleVarScope(cellp, roles.qPort);
        ಯದಿ (!nextVscp || !stateVscp) {
            ಪ್ರತ್ಯಾಗಮ rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* ಧ್ರುವ clkVscp = roleVarScope(cellp, roles.clkPort);
        ಯದಿ (!clkVscp) {
            ಪ್ರತ್ಯಾಗಮ rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper instance requires a simple clock connection");
        }

        FsmSenDesc clkSense;
        clkSense.edgeType = VEdgeType::ET_POSEDGE;
        clkSense.varScopep = clkVscp;
        cand.senses().ತಳ್ಳು_back(clkSense);

        AstVarScope* resetVscp = nullptr;
        ಯದಿ (!roles.rstPort.empty()) resetVscp = roleVarScope(cellp, roles.rstPort);
        ಯದಿ (resetVscp) {
            // The descriptor identifies the reset port but not its polarity. Use
            // the wrapper's own event control AST as the contract for sampling
            // the connected parent signal.
            ತಾರ್ಕಿಕ inferred_Active_ಕೆಳಗಿನ = ಅಸತ್ಯ;
            ಯದಿ (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rstPort, inferred_Active_ಕೆಳಗಿನ)) {
                roles.has_Rst_Active_ಕೆಳಗಿನ = ಸತ್ಯ;
                roles.rst_Active_ಕೆಳಗಿನ = inferred_Active_ಕೆಳಗಿನ;
            }
        }

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resetValuep = nullptr;
        Fsm_State_ಮೌಲ್ಯ reset_ಮೌಲ್ಯ;
        ಧ್ರುವ ತಾರ್ಕಿಕ has_Reset_ಮೌಲ್ಯ
            = !roles.rstValParam.empty()
              && simple_Param_State_ಮೌಲ್ಯ(cellp, roles.rstValParam, reset_ಮೌಲ್ಯ, resetValuep);
        ಯದಿ (resetVscp && roles.has_Rst_Active_ಕೆಳಗಿನ && has_Reset_ಮೌಲ್ಯ) {
            FsmSenDesc rstSense;
            rstSense.edgeType = roles.rst_Active_ಕೆಳಗಿನ ? VEdgeType::ET_NEGEDGE : VEdgeType::ET_POSEDGE;
            rstSense.varScopep = resetVscp;
            cand.senses().ತಳ್ಳು_back(rstSense);
            cand.resetCond().varScopep = resetVscp;
            cand.resetCond().active_ಕೆಳಗಿನ = roles.rst_Active_ಕೆಳಗಿನ;
            cand.hasResetCond(ಸತ್ಯ);
            cand.resetArcs().emplace_back(reset_ಮೌಲ್ಯ, cellp, resetValuep);
        } ಅನ್ಯಥಾ ಯದಿ (!roles.rstPort.empty() || !roles.rstValParam.empty()) {
            string reason;
            ಯದಿ (roles.rstPort.empty()) {
                reason = "reset port is not configured";
            } ಅನ್ಯಥಾ ಯದಿ (!resetVscp) {
                reason = "reset connection is missing or not a simple variable";
            } ಅನ್ಯಥಾ ಯದಿ (!roles.has_Rst_Active_ಕೆಳಗಿನ) {
                reason = "reset polarity could not be inferred from the wrapper";
            } ಅನ್ಯಥಾ ಯದಿ (roles.rstValParam.empty()) {
                reason = "reset_value parameter is not configured";
            } ಅನ್ಯಥಾ {
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
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ವರ್ಗ ComboAlwaysAnalyzer final {
    ಸಾರ್ವಜನಿಕ:
        ಸಂರಚನಾ ComboMatch final {
            ಧ್ರುವ FsmRegisterCandidate* matchedp = nullptr;
            Ast_ಗಂಟು* warnNodep = nullptr;
        };

    ಗುಪ್ತ:
        ಧ್ರುವ std::vector<FsmRegisterCandidate>& m_registerCandidates;

    ಸಾರ್ವಜನಿಕ:
        explicit ComboAlwaysAnalyzer(ಧ್ರುವ std::vector<FsmRegisterCandidate>& registerCandidates)
            : m_registerCandidates{registerCandidates} {}

        ComboMatch matchCase(Ast_ಗಂಟು* stmtsp, AstCase* casep) ಧ್ರುವ {
            ComboMatch match;
            AstVarRef* ಧ್ರುವ selp = VN_CAST(casep->exprp(), VarRef);
            ಯದಿ (!selp) ಪ್ರತ್ಯಾಗಮ match;
            ಪ್ರತಿ (ಧ್ರುವ FsmRegisterCandidate& reg : m_registerCandidates) {
                ಯದಿ (selp->varScopep() == reg.nextVscp()) {
                    ಯದಿ (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        ಅನುವರ್ತ;
                    }
                } ಅನ್ಯಥಾ ಯದಿ (selp->varScopep() != reg.stateVscp()) {
                    ಅನುವರ್ತ;
                }
                Ast_ಗಂಟು* ಧ್ರುವ warnNodep = FsmDetectVisitor::case_Supported_Transition_ಗಂಟು(
                    casep, reg.nextVscp(), reg.inclCond());
                ಯದಿ (!warnNodep) ಅನುವರ್ತ;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            ಪ್ರತ್ಯಾಗಮ match;
        }

        ComboMatch matchIfChain(Ast_ಗಂಟು* stmtsp, ಧ್ರುವ FsmIfChainCandidate& chain) ಧ್ರುವ {
            ComboMatch match;
            ಪ್ರತಿ (ಧ್ರುವ FsmRegisterCandidate& reg : m_registerCandidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                ಯದಿ (chain.compareVscp == reg.nextVscp()) {
                    ಯದಿ (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.nextVscp())) {
                        ಅನುವರ್ತ;
                    }
                } ಅನ್ಯಥಾ ಯದಿ (chain.compareVscp != reg.stateVscp()) {
                    ಅನುವರ್ತ;
                }
                Ast_ಗಂಟು* ಧ್ರುವ warnNodep
                    = FsmDetectVisitor::if_Chain_Supported_Transition_ಗಂಟು(chain, reg.nextVscp());
                ಯದಿ (!warnNodep) ಅನುವರ್ತ;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            ಪ್ರತ್ಯಾಗಮ match;
        }

        ತಾರ್ಕಿಕ shouldWarnUnsupported(Ast_ಗಂಟು* stmtsp, AstCase* casep) ಧ್ರುವ {
            ಧ್ರುವ AstVarRef* ಧ್ರುವ selp = VN_CAST(casep->exprp(), VarRef);
            ಯದಿ (!selp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

            ಪ್ರತ್ಯಾಗಮ std::any_of(
                m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                [&](ಧ್ರುವ FsmRegisterCandidate& reg) -> ತಾರ್ಕಿಕ {
                    ಧ್ರುವ ತಾರ್ಕಿಕ matchesNext = selp->varScopep() == reg.nextVscp();
                    ಧ್ರುವ ತಾರ್ಕಿಕ matchesState = selp->varScopep() == reg.stateVscp();

                    ಯದಿ (!matchesNext && !matchesState) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                    ಯದಿ (matchesNext
                        && !FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                    }
                    ಪ್ರತ್ಯಾಗಮ FsmDetectVisitor::case_Supported_Transition_ಗಂಟು(casep, reg.nextVscp(),
                                                                         reg.inclCond());
                });
        }
    };

    // Reset arcs are only modeled for the simple signal form that survives to
    // this pass after earlier normalization.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isSimpleResetCond(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) { ಪ್ರತ್ಯಾಗಮ VN_IS(condp, VarRef); }

    // Normalize the reset condition into a compact description so the lowering
    // phase can regenerate the same predicate after detection. By the time
    // this pass runs, active-low source forms such as "!rst_n" have already
    // been canonicalized to a positive-condition if/else shape, so only a
    // plain VarRef survives here.
    ಸ್ಥಿರ FsmResetCondDesc describeResetCond(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        FsmResetCondDesc desc;
        ಯದಿ (AstVarRef* ಧ್ರುವ vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        ಪ್ರತ್ಯಾಗಮ desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    ಸ್ಥಿರ std::vector<FsmSenDesc> describe_Sen_ವೃಕ್ಷ(Ast_Sen_ವೃಕ್ಷ* sentreep) {
        std::vector<FsmSenDesc> senses;
        ಪ್ರತಿ (AstSenItem* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), SenItem)) {
            Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ vrefp = itemp->varrefp();
            ಯದಿ (!vrefp) ಅನುವರ್ತ;
            FsmSenDesc desc;
            desc.edgeType = itemp->edgeType().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.ತಳ್ಳು_back(desc);
        }
        ಪ್ರತ್ಯಾಗಮ senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isIgnorableStmt(Ast_ಗಂಟು* nodep) { ಪ್ರತ್ಯಾಗಮ VN_IS(nodep, CoverInc); }

    ಸ್ಥಿರ Ast_ಗಂಟು* skipLeadingIgnorableStmt(Ast_ಗಂಟು* nodep) {
        ಯಾವತ್ (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        ಪ್ರತ್ಯಾಗಮ nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    ಸ್ಥಿರ Ast_ಗಂಟು* singleMeaningfulStmt(Ast_ಗಂಟು* stmtp) {
        Ast_ಗಂಟು* resultp = nullptr;
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            ಯದಿ (isIgnorableStmt(nodep)) ಅನುವರ್ತ;
            ಯದಿ (resultp) ಪ್ರತ್ಯಾಗಮ nullptr;
            resultp = nodep;
        }
        ಪ್ರತ್ಯಾಗಮ resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    ಸ್ಥಿರ Ast_ಗಂಟು* singleMeaningfulBranch(Ast_ಗಂಟು* branchp) {
        ಯದಿ (!branchp) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪ್ರತ್ಯಾಗಮ branchp;
    }

    // By fsm-detect time, non-clocked always @* blocks are already admitted through
    // a missing sentree. This helper therefore only needs to recognize
    // explicit changed-sensitivity lists such as always @(a or b); clocked and
    // event-driven forms remain out of scope.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isPlainComboSentree(ಧ್ರುವ Ast_Sen_ವೃಕ್ಷ* sentreep) {
        UASSERT(sentreep, "plain combo sensitivity check requires a sensitivity tree");
        ಪ್ರತಿ (ಧ್ರುವ AstSenItem* senp = sentreep->sensesp(); senp;
             senp = VN_AS(senp->nextp(), SenItem)) {
            ಯದಿ (senp->edgeType() == VEdgeType::ET_CHANGED) ಅನುವರ್ತ;
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಶೂನ್ಯ warnUnsupportedComboAlways(ಧ್ರುವ FsmComboAlways& combo) {
        ಧ್ರುವ ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_ಗಂಟು* ಧ್ರುವ stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        ತಾರ್ಕಿಕ warned = ಅಸತ್ಯ;
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ಧ್ರುವ casep = VN_CAST(nodep, Case);
            ಯದಿ (!casep) ಅನುವರ್ತ;
            ಯದಿ (analyzer.shouldWarnUnsupported(stmtsp, casep)) {
                casep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-clocked always "
                                        "blocks requires a combinational sensitivity list or "
                                        "always_comb");
                warned = ಸತ್ಯ;
            }
            ಯದಿ (warned) ಭಙ್ಗ;
        }
    }

    // Case-item bodies are single subtrees like if/else arms, not statement
    // lists, so unwrap only local begin/end wrappers here rather than walking
    // sibling case items via nextp().
    ಸ್ಥಿರ Ast_ಗಂಟು_Assign* directStateAssign(Ast_ಗಂಟು* stmtp, AstVarScope* stateVscp) {
        Ast_ಗಂಟು* ಧ್ರುವ nodep = singleMeaningfulBranch(stmtp);
        ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ nullptr;
        Ast_ಗಂಟು_Assign* ಧ್ರುವ assp = VN_CAST(nodep, ಗಂಟು_Assign);
        ಯದಿ (!assp) ಪ್ರತ್ಯಾಗಮ nullptr;
        AstVarRef* ಧ್ರುವ vrefp = VN_CAST(assp->lhsp(), VarRef);
        ಯದಿ (!vrefp || vrefp->varScopep() != stateVscp) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪ್ರತ್ಯಾಗಮ assp;
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_Assign* ಗಂಟು_State_Var_Assign(Ast_ಗಂಟು* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        Ast_ಗಂಟು_Assign* ಧ್ರುವ assp = VN_CAST(nodep, ಗಂಟು_Assign);
        ಯದಿ (!assp) ಪ್ರತ್ಯಾಗಮ nullptr;
        AstVarRef* ಧ್ರುವ lhsp = VN_CAST(assp->lhsp()->baseFromp(ಸತ್ಯ), VarRef);
        AstVarRef* ಧ್ರುವ rhsp = VN_CAST(assp->rhsp(), VarRef);
        ಯದಿ (!rhsp || !lhsp) ಪ್ರತ್ಯಾಗಮ nullptr;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        ಪ್ರತ್ಯಾಗಮ assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    ಸ್ಥಿರ Ast_ಗಂಟು_Assign* directCondStateVarAssign(Ast_ಗಂಟು* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*& condp,
                                                   Fsm_State_ಮೌಲ್ಯ& reset_ಮೌಲ್ಯ) {
        Ast_ಗಂಟು_Assign* ಧ್ರುವ assp = VN_CAST(nodep, ಗಂಟು_Assign);
        ಯದಿ (!assp) ಪ್ರತ್ಯಾಗಮ nullptr;
        AstVarRef* ಧ್ರುವ lhsp = VN_CAST(assp->lhsp()->baseFromp(ಸತ್ಯ), VarRef);
        AstCond* ಧ್ರುವ rhsp = VN_CAST(assp->rhsp(), Cond);
        ಯದಿ (!rhsp || !lhsp) ಪ್ರತ್ಯಾಗಮ nullptr;
        AstVarRef* ಧ್ರುವ elsep = VN_CAST(rhsp->elsep(), VarRef);
        ಯದಿ (!elsep) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಯದಿ (const_ಮೌಲ್ಯ_Status(rhsp->thenp(), reset_ಮೌಲ್ಯ) != Const_ಮೌಲ್ಯ_Status::OK) ಪ್ರತ್ಯಾಗಮ nullptr;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        ಪ್ರತ್ಯಾಗಮ assp;
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_Assign* direct_Const_State_Assign_ಗಂಟು(Ast_ಗಂಟು* nodep, AstVarScope*& stateVscp,
                                                     Fsm_State_ಮೌಲ್ಯ& ಮೌಲ್ಯ) {
        Ast_ಗಂಟು_Assign* ಧ್ರುವ assp = VN_CAST(nodep, ಗಂಟು_Assign);
        ಯದಿ (!assp) ಪ್ರತ್ಯಾಗಮ nullptr;
        AstVarRef* ಧ್ರುವ lhsp = VN_CAST(assp->lhsp()->baseFromp(ಸತ್ಯ), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        ಯದಿ (const_ಮೌಲ್ಯ_Status(assp->rhsp(), ಮೌಲ್ಯ) != Const_ಮೌಲ್ಯ_Status::OK) ಪ್ರತ್ಯಾಗಮ nullptr;
        stateVscp = lhsp->varScopep();
        ಪ್ರತ್ಯಾಗಮ assp;
    }

    ಗಣನಾ ವರ್ಗ ResetAssignStatus : uint8_t {
        EMPTY,  // Reset branch had no non-coverage statements.
        NONE,  // Reset branch was not the supported direct-constant shape.
        SINGLE,  // Exactly one supported reset assignment was collected.
        MULTI_SAME_STATE  // Multiple assignments to the same FSM state var; warn and ignore.
    };

    // Reset arcs are only extracted from the single direct-constant form. If
    // user RTL assigns the same state register multiple times in the reset
    // branch, warn and skip reset-arc modeling rather than inventing multiple
    // reset transitions for an odd but legal coding style.
    ಸ್ಥಿರ ResetAssignStatus collectConstStateAssigns(Ast_ಗಂಟು* stmtp, AstVarScope*& stateVscp,
                                                      std::vector<FsmResetArcDesc>& resetArcs) {
        Ast_ಗಂಟು* nodep = skipLeadingIgnorableStmt(stmtp);
        ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ ResetAssignStatus::EMPTY;
        ಪ್ರತಿ (;; nodep = nodep->nextp()) {
            AstVarScope* assignStateVscp = nullptr;
            Fsm_State_ಮೌಲ್ಯ ಮೌಲ್ಯ;
            Ast_ಗಂಟು_Assign* ಧ್ರುವ assp = direct_Const_State_Assign_ಗಂಟು(nodep, assignStateVscp, ಮೌಲ್ಯ);
            ಯದಿ (!assp) ಪ್ರತ್ಯಾಗಮ ResetAssignStatus::NONE;
            ಯದಿ (!stateVscp) stateVscp = assignStateVscp;
            ಯದಿ (assignStateVscp != stateVscp) ಪ್ರತ್ಯಾಗಮ ResetAssignStatus::NONE;
            ಯದಿ (!resetArcs.empty()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.clear();
                ಪ್ರತ್ಯಾಗಮ ResetAssignStatus::MULTI_SAME_STATE;
            }
            resetArcs.emplace_back(ಮೌಲ್ಯ, assp);
            ಯದಿ (!nodep->nextp()) ಪ್ರತ್ಯಾಗಮ ResetAssignStatus::SINGLE;
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ hasCanonicalNextStateDefaultBeforeCase(Ast_ಗಂಟು* stmtsp, Ast_ಗಂಟು* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* nextVscp) {
        Ast_ಗಂಟು* ಧ್ರುವ bodyp = skipLeadingIgnorableStmt(stmtsp);
        ತಾರ್ಕಿಕ sawCanonicalDefault = ಅಸತ್ಯ;
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            ಯದಿ (nodep == targetp) ಪ್ರತ್ಯಾಗಮ sawCanonicalDefault;
            ಯದಿ (Ast_ಗಂಟು_Assign* ಧ್ರುವ assp = VN_CAST(nodep, ಗಂಟು_Assign)) {
                AstVarRef* ಧ್ರುವ lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* ಧ್ರುವ rhsp = VN_CAST(assp->rhsp(), VarRef);
                ಯದಿ (!lhsp || lhsp->varScopep() != nextVscp) ಅನುವರ್ತ;
                ಯದಿ (sawCanonicalDefault) {
                    ಧ್ರುವ string nextName = nextVscp->varp()->prettyNameQ();
                    ಧ್ರುವ string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + nextName
                                     + ") when the canonical " + nextName + " = " + stateName
                                     + " default is overwritten before the case statement");
                    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                }
                ಯದಿ (!rhsp || rhsp->varScopep() != stateVscp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                sawCanonicalDefault = ಸತ್ಯ;
            }
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ ifStateConstAssign(Ast_ಗಂಟು* stmtp, AstVarScope* stateVscp,
                                   Fsm_State_ಮೌಲ್ಯ& then_ಮೌಲ್ಯ, Fsm_State_ಮೌಲ್ಯ& else_ಮೌಲ್ಯ) {
        AstIf* ಧ್ರುವ ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        ಯದಿ (!ifp || !ifp->elsesp()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        AstVarScope* thenVscp = nullptr;
        AstVarScope* elseVscp = nullptr;
        Ast_ಗಂಟು* ಧ್ರುವ thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        Ast_ಗಂಟು* ಧ್ರುವ elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        ಯದಿ (!thenNodep || !elseNodep) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!direct_Const_State_Assign_ಗಂಟು(thenNodep, thenVscp, then_ಮೌಲ್ಯ)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!direct_Const_State_Assign_ಗಂಟು(elseNodep, elseVscp, else_ಮೌಲ್ಯ)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (thenVscp == stateVscp && elseVscp == stateVscp) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (thenVscp != elseVscp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        Ast_ಗಂಟು* ಧ್ರುವ followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = nullptr;
        AstVarScope* finalFromVscp = nullptr;
        Ast_ಗಂಟು* ಧ್ರುವ finalNodep = singleMeaningfulBranch(followp);
        ಯದಿ (!finalNodep) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!ಗಂಟು_State_Var_Assign(finalNodep, finalStateVscp, finalFromVscp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (finalStateVscp != stateVscp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (finalFromVscp != thenVscp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ directStateCondConstAssign(Ast_ಗಂಟು* stmtp, AstVarScope* stateVscp,
                                           Fsm_State_ಮೌಲ್ಯ& then_ಮೌಲ್ಯ, Fsm_State_ಮೌಲ್ಯ& else_ಮೌಲ್ಯ) {
        Ast_ಗಂಟು_Assign* ಧ್ರುವ assp = directStateAssign(stmtp, stateVscp);
        ಯದಿ (!assp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        AstCond* ಧ್ರುವ condp = VN_CAST(assp->rhsp(), Cond);
        ಯದಿ (!condp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ const_ಮೌಲ್ಯ_Status(condp->thenp(), then_ಮೌಲ್ಯ) == Const_ಮೌಲ್ಯ_Status::OK
               && const_ಮೌಲ್ಯ_Status(condp->elsep(), else_ಮೌಲ್ಯ) == Const_ಮೌಲ್ಯ_Status::OK;
    }

    ಸ್ಥಿರ Ast_ಗಂಟು* case_Item_Supported_Arc_ಗಂಟು(AstCaseItem* itemp, AstVarScope* stateVscp,
                                             ತಾರ್ಕಿಕ inclCond) {
        ಯದಿ (itemp->isDefault()) {
            ಯದಿ (!inclCond) ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        Ast_ಗಂಟು_Assign* ಧ್ರುವ assp = directStateAssign(itemp->stmtsp(), stateVscp);
        ಯದಿ (assp) {
            Fsm_State_ಮೌಲ್ಯ to_ಮೌಲ್ಯ;
            ಯದಿ (const_ಮೌಲ್ಯ_Status(assp->rhsp(), to_ಮೌಲ್ಯ) == Const_ಮೌಲ್ಯ_Status::OK) ಪ್ರತ್ಯಾಗಮ assp;
        }
        Fsm_State_ಮೌಲ್ಯ then_ಮೌಲ್ಯ;
        Fsm_State_ಮೌಲ್ಯ else_ಮೌಲ್ಯ;
        ಯದಿ (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_ಮೌಲ್ಯ, else_ಮೌಲ್ಯ)) {
            ಪ್ರತ್ಯಾಗಮ assp;
        }
        ಯದಿ (ifStateConstAssign(itemp->stmtsp(), stateVscp, then_ಮೌಲ್ಯ, else_ಮೌಲ್ಯ)) {
            ಪ್ರತ್ಯಾಗಮ singleMeaningfulBranch(itemp->stmtsp());
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    ಸ್ಥಿರ Ast_ಗಂಟು* case_Supported_Transition_ಗಂಟು(AstCase* casep, AstVarScope* stateVscp,
                                                ತಾರ್ಕಿಕ inclCond) {
        ಪ್ರತಿ (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ nodep = case_Item_Supported_Arc_ಗಂಟು(itemp, stateVscp, inclCond))
                ಪ್ರತ್ಯಾಗಮ nodep;
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    ಸ್ಥಿರ Ast_ಗಂಟು* case_Item_Supported_Arc_ಗಂಟು_Like(Ast_ಗಂಟು* stmtsp, AstVarScope* stateVscp) {
        ಯದಿ (Ast_ಗಂಟು_Assign* ಧ್ರುವ assp = directStateAssign(stmtsp, stateVscp)) {
            Fsm_State_ಮೌಲ್ಯ to_ಮೌಲ್ಯ;
            ಯದಿ (const_ಮೌಲ್ಯ_Status(assp->rhsp(), to_ಮೌಲ್ಯ) == Const_ಮೌಲ್ಯ_Status::OK) ಪ್ರತ್ಯಾಗಮ assp;
            Fsm_State_ಮೌಲ್ಯ then_ಮೌಲ್ಯ;
            Fsm_State_ಮೌಲ್ಯ else_ಮೌಲ್ಯ;
            ಯದಿ (directStateCondConstAssign(stmtsp, stateVscp, then_ಮೌಲ್ಯ, else_ಮೌಲ್ಯ)) ಪ್ರತ್ಯಾಗಮ assp;
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    ಸ್ಥಿರ Ast_ಗಂಟು* if_Chain_Supported_Transition_ಗಂಟು(ಧ್ರುವ FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        ಪ್ರತಿ (size_t i = 0; i < chain.branches.size(); ++i) {
            ಧ್ರುವ FsmIfBranch& branch = chain.branches[i];
            Ast_ಗಂಟು* ಧ್ರುವ nodep = case_Item_Supported_Arc_ಗಂಟು_Like(branch.stmtsp, stateVscp);
            ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        ಪ್ರತ್ಯಾಗಮ chain.branches.front().ifp;
    }

    // Prefer user labels in reports. Forced non-enum FSMs prepopulate synthetic
    // labels, so all emitted arcs should already have a known label here.
    ಸ್ಥಿರ string label_For_ಮೌಲ್ಯ(
        ಧ್ರುವ std::unordered_map<Fsm_State_ಮೌಲ್ಯ, StateConstLabel, Fsm_State_ಮೌಲ್ಯ_Hash>& labels,
        ಧ್ರುವ Fsm_State_ಮೌಲ್ಯ& ಮೌಲ್ಯ) {
        ಪ್ರತ್ಯಾಗಮ labels.at(ಮೌಲ್ಯ).text;
    }

    // The extractor only models constant-valued state transitions, and by the
    // time detect runs those values have already been constant-folded.
    ಗಣನಾ ವರ್ಗ Const_ಮೌಲ್ಯ_Status : uint8_t { OK, NOT_CONST, XZ };

    ಸ್ಥಿರ Const_ಮೌಲ್ಯ_Status const_ಮೌಲ್ಯ_Status(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Fsm_State_ಮೌಲ್ಯ& ಮೌಲ್ಯ) {
        ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(exprp, Const);
        ಯದಿ (!constp) ಪ್ರತ್ಯಾಗಮ Const_ಮೌಲ್ಯ_Status::NOT_CONST;
        ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = constp->ಅಂಕೆ();
        ಯದಿ (ಅಂಕೆ.isAnyXZ()) ಪ್ರತ್ಯಾಗಮ Const_ಮೌಲ್ಯ_Status::XZ;
        ಮೌಲ್ಯ = Fsm_State_ಮೌಲ್ಯ{ಅಂಕೆ};
        ಪ್ರತ್ಯಾಗಮ Const_ಮೌಲ್ಯ_Status::OK;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ pureStateComparisonNoAlias(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, FsmStateComparison& cmp) {
        AstEq* ಧ್ರುವ eqp = VN_CAST(exprp, Eq);
        ಯದಿ (!eqp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = eqp->rhsp();
        ಯದಿ (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->baseFromp(ಸತ್ಯ), VarRef);
            ಯದಿ (!vrefp) { ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ; }
            valuep = eqp->lhsp();
        }

        Fsm_State_ಮೌಲ್ಯ ಮೌಲ್ಯ;
        ಯದಿ (const_ಮೌಲ್ಯ_Status(valuep, ಮೌಲ್ಯ) != Const_ಮೌಲ್ಯ_Status::OK) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        cmp.stateVscp = vrefp->varScopep();
        cmp.valuep = valuep;
        cmp.ಮೌಲ್ಯ = ಮೌಲ್ಯ;
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ pureStateComparison(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಧ್ರುವ FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        ಯದಿ (pureStateComparisonNoAlias(exprp, cmp)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        ಯದಿ (AstVarRef* ಧ್ರುವ vrefp = VN_CAST(exprp, VarRef)) {
            ಧ್ರುವ FsmAliasMap::const_iterator it = aliases.find(vrefp->varScopep());
            ಯದಿ (it == aliases.end()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            cmp = it->second;
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ supported_ತುದಿ_Level_Guard(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        ಯದಿ (VN_IS(exprp, Or)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (VN_IS(exprp, RedAnd)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (VN_IS(exprp, RedOr)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (VN_IS(exprp, RedXor)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ resolveIfPredicate(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಧ್ರುವ FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, ತಾರ್ಕಿಕ& hasGuard) {
        std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> terms;
        std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> pending;
        pending.ತಳ್ಳು_back(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        ಯಾವತ್ (!pending.empty()) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ nodep = pending.back();
            pending.ತೆಗೆ_back();
            ಯದಿ (AstAnd* ಧ್ರುವ andp = VN_CAST(nodep, And)) {
                pending.ತಳ್ಳು_back(andp->rhsp());
                pending.ತಳ್ಳು_back(andp->lhsp());
            } ಅನ್ಯಥಾ {
                terms.ತಳ್ಳು_back(nodep);
            }
        }

        ತಾರ್ಕಿಕ sawComparison = ಅಸತ್ಯ;
        ಪ್ರತಿ (size_t i = 0; i < terms.size(); ++i) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ termp = terms[i];
            FsmStateComparison termCmp;
            ಯದಿ (pureStateComparison(termp, aliases, termCmp /*ref*/)) {
                ಯದಿ (sawComparison) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                cmp = termCmp;
                sawComparison = ಸತ್ಯ;
                ಅನುವರ್ತ;
            }
            ಯದಿ (!supported_ತುದಿ_Level_Guard(termp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            hasGuard = ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ sawComparison;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ addAlias(FsmAliasMap& aliases, std::unordered_set<ಧ್ರುವ AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, ಧ್ರುವ FsmStateComparison& cmp) {
        ಯದಿ (ambiguous.find(aliasVscp) != ambiguous.end()) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ FsmAliasMap::iterator it = aliases.find(aliasVscp);
        ಯದಿ (it == aliases.end()) {
            aliases.emplace(aliasVscp, cmp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        ಯದಿ (it->second.stateVscp == cmp.stateVscp && it->second.ಮೌಲ್ಯ == cmp.ಮೌಲ್ಯ) ಪ್ರತ್ಯಾಗಮ;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ collectAliasFromAssign(Ast_ಗಂಟು_Assign* assp, FsmAliasMap& aliases,
                                       std::unordered_set<ಧ್ರುವ AstVarScope*>& ambiguous) {
        AstVarRef* ಧ್ರುವ lhsp = VN_CAST(assp->lhsp(), VarRef);
        ಯದಿ (!lhsp) ಪ್ರತ್ಯಾಗಮ;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        ಯದಿ (!pureStateComparisonNoAlias(assp->rhsp(), cmp /*ref*/)) ಪ್ರತ್ಯಾಗಮ;
        addAlias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(Ast_ಗಂಟು* stmtsp, Ast_ಗಂಟು* limitp) ಧ್ರುವ {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_set<ಧ್ರುವ AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            ಯದಿ (Ast_ಗಂಟು_Assign* ಧ್ರುವ assp = VN_CAST(nodep, ಗಂಟು_Assign)) {
                collectAliasFromAssign(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        ಪ್ರತಿ (ಧ್ರುವ AstVarScope* ಧ್ರುವ vscp : ambiguous) aliases.erase(vscp);
        ಪ್ರತ್ಯಾಗಮ aliases;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ collectIfChain(AstIf* ifp, ಧ್ರುವ FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_set<string> seen_ಮೌಲ್ಯಗಳು;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        ಯಾವತ್ (ಸತ್ಯ) {
            FsmStateComparison cmp;
            ತಾರ್ಕಿಕ hasGuard = ಅಸತ್ಯ;
            ಯದಿ (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಯದಿ (chain.compareVscp && chain.compareVscp != cmp.stateVscp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಯದಿ (!seen_ಮೌಲ್ಯಗಳು.ಸೇರಿಸು(cmp.ಮೌಲ್ಯ.ಕೀಲಿ()).second) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.ತಳ್ಳು_back(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.ಮೌಲ್ಯ, hasGuard});

            Ast_ಗಂಟು* ಧ್ರುವ elseNodep
                = singleMeaningfulBranch(skipLeadingIgnorableStmt(curp->elsesp()));
            ಯದಿ (!elseNodep) ಭಙ್ಗ;
            ಯದಿ (AstIf* ಧ್ರುವ elseIfp = VN_CAST(elseNodep, If)) {
                curp = elseIfp;
                ಅನುವರ್ತ;
            }
            chain.defaultStmtsp = elseNodep;
            ಭಙ್ಗ;
        }
        ಪ್ರತ್ಯಾಗಮ chain.branches.size() >= 2;
    }

    // Enum-backed FSMs should only use values that were interned as known states.
    // If a constant transition references some other encoding, warn and skip FSM
    // instrumentation for that edge rather than silently dropping it or turning
    // optional coverage into a hard compile failure.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ validate_Known_State_ಮೌಲ್ಯ(Ast_ಗಂಟು* nodep, ಧ್ರುವ FsmStateSpace& stateSpace,
                                        ಧ್ರುವ Fsm_State_ಮೌಲ್ಯ& ಮೌಲ್ಯ, ಧ್ರುವ string& role) {
        ಯದಿ (stateSpace.labels.find(ಮೌಲ್ಯ) != stateSpace.labels.end()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (stateSpace.enumBacked) {
            ಧ್ರುವ string enumRole = role == "source" ? "case item value" : "assigned value";
            nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on enum state variable "
                                        + stateSpace.stateVarp->prettyNameQ() + ": " + enumRole
                                        + " " + ಮೌಲ್ಯ.warnText()
                                        + " is not present in the declared enum");
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum state variable "
                                    + stateSpace.stateVarp->prettyNameQ() + ": " + role + " value "
                                    + ಮೌಲ್ಯ.warnText()
                                    + " is not present in the inferred state space");
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ಸ್ಥಿರ StateConstLabel stateLabelForConst(AstConst* constp) {
        ಧ್ರುವ string name = constp->origParamName();
        ಯದಿ (!name.empty()) ಪ್ರತ್ಯಾಗಮ StateConstLabel{Ast_ಗಂಟು::prettyName(name), ಸತ್ಯ, 0};
        ಪ್ರತ್ಯಾಗಮ StateConstLabel{constp->name(), ಅಸತ್ಯ, 0};
    }

    ಸ್ಥಿರ ಶೂನ್ಯ updateStateLabel(FsmStateSpace& stateSpace, ಧ್ರುವ Fsm_State_ಮೌಲ್ಯ& ಮೌಲ್ಯ,
                                 ಧ್ರುವ StateConstLabel& label) {
        stateSpace.states.at(stateSpace.labels.at(ಮೌಲ್ಯ).stateIndex).first = label.text;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ matchRegisterAlways(AstAlways* alwaysp, AstScope* scopep,
                                    FsmRegisterCandidate& cand) {
        ಯದಿ (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        Ast_ಗಂಟು* ಧ್ರುವ stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        Ast_ಗಂಟು* ಧ್ರುವ nodep = singleMeaningfulStmt(stmtsp);
        ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        AstVarScope* stateVscp = nullptr;
        AstVarScope* nextVscp = nullptr;
        ಯದಿ (AstIf* ಧ್ರುವ ifp = VN_CAST(nodep, If)) {
            ಯದಿ (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            AstVarScope* resetStateVscp = nullptr;
            ಧ್ರುವ ResetAssignStatus resetStatus
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            ಧ್ರುವ ತಾರ್ಕಿಕ emptyResetBranch = resetStatus == ResetAssignStatus::EMPTY;
            ಯದಿ (emptyResetBranch) {
                cand.resetArcs().clear();
            } ಅನ್ಯಥಾ ಯದಿ (resetStatus == ResetAssignStatus::NONE) {
                cand.resetArcs().clear();
                Fsm_State_ಮೌಲ್ಯ reset_ಮೌಲ್ಯ;
                Ast_ಗಂಟು* ಧ್ರುವ thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                Ast_ಗಂಟು_Assign* ಧ್ರುವ resetAssp
                    = direct_Const_State_Assign_ಗಂಟು(thenNodep, resetStateVscp, reset_ಮೌಲ್ಯ);
                ಯದಿ (!resetAssp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                cand.resetArcs().emplace_back(reset_ಮೌಲ್ಯ, resetAssp);
            } ಅನ್ಯಥಾ ಯದಿ (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                cand.resetArcs().clear();
            }
            Ast_ಗಂಟು* ಧ್ರುವ elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            ಯದಿ (!ಗಂಟು_State_Var_Assign(elseNodep, stateVscp, nextVscp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಯದಿ (!emptyResetBranch && resetStateVscp != stateVscp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != nullptr);
        } ಅನ್ಯಥಾ {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resetCondp = nullptr;
            Fsm_State_ಮೌಲ್ಯ reset_ಮೌಲ್ಯ;
            // Note the active low 'reset ? next : CONST' form is not accepted
            ಯದಿ (Ast_ಗಂಟು_Assign* ಧ್ರುವ assp
                = directCondStateVarAssign(nodep, stateVscp, nextVscp, resetCondp, reset_ಮೌಲ್ಯ)) {
                cand.resetArcs().emplace_back(reset_ಮೌಲ್ಯ, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != nullptr);
            } ಅನ್ಯಥಾ ಯದಿ (!ಗಂಟು_State_Var_Assign(nodep, stateVscp, nextVscp)) {
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
        }
        cand.scopep(scopep);
        cand.alwaysp(alwaysp);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(stateVscp);
        cand.nextVscp(nextVscp);
        cand.senses() = describe_Sen_ವೃಕ್ಷ(alwaysp->sentreep());
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(alwaysp->fileline());
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ add_ಮೌಲ್ಯ_To_State_Space(Ast_ಗಂಟು* nodep, FsmStateSpace& stateSpace,
                                     ಧ್ರುವ Fsm_State_ಮೌಲ್ಯ& ಮೌಲ್ಯ, StateConstLabel label) {
        ಧ್ರುವ auto labelIt = stateSpace.labels.find(ಮೌಲ್ಯ);
        ಯದಿ (labelIt != stateSpace.labels.end()) {
            StateConstLabel& existingLabel = labelIt->second;
            ಯದಿ (existingLabel.text != label.text && existingLabel.fromParam && label.fromParam) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                        "state variable "
                                            + stateSpace.stateVarp->prettyNameQ()
                                            + " with multiple labels for the same value "
                                            + ಮೌಲ್ಯ.warnText() + ": " + existingLabel.text
                                            + " and " + label.text);
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            ಯದಿ (!existingLabel.fromParam && label.fromParam) {
                existingLabel.text = label.text;
                existingLabel.fromParam = label.fromParam;
                updateStateLabel(stateSpace, ಮೌಲ್ಯ, label);
            }
        } ಅನ್ಯಥಾ {
            StateConstLabel storedLabel = label;
            storedLabel.stateIndex = stateSpace.states.size();
            stateSpace.states.emplace_back(label.text, ಮೌಲ್ಯ);
            stateSpace.labels.emplace(ಮೌಲ್ಯ, storedLabel);
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    ಸ್ಥಿರ ತಾರ್ಕಿಕ add_ಅಭಿವ್ಯಕ್ತಿ_To_State_Space(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_ಮೌಲ್ಯ ಮೌಲ್ಯ;
        ಧ್ರುವ Const_ಮೌಲ್ಯ_Status status = const_ಮೌಲ್ಯ_Status(valuep, ಮೌಲ್ಯ);
        ಯದಿ (status != Const_ಮೌಲ್ಯ_Status::OK) {
            ಯದಿ (status == Const_ಮೌಲ್ಯ_Status::XZ) {
                valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                         "state variable "
                                             + stateSpace.stateVarp->prettyNameQ()
                                             + " with X/Z state encoding values");
            }
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        AstConst* ಧ್ರುವ constp = VN_AS(valuep, Const);
        ಪ್ರತ್ಯಾಗಮ add_ಮೌಲ್ಯ_To_State_Space(valuep, stateSpace, ಮೌಲ್ಯ, stateLabelForConst(constp));
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ add_Optional_Target_ಅಭಿವ್ಯಕ್ತಿ_To_State_Space(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_ಮೌಲ್ಯ ಮೌಲ್ಯ;
        ಧ್ರುವ Const_ಮೌಲ್ಯ_Status status = const_ಮೌಲ್ಯ_Status(valuep, ಮೌಲ್ಯ);
        ಯದಿ (status != Const_ಮೌಲ್ಯ_Status::OK) {
            valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                     "state variable "
                                         + stateSpace.stateVarp->prettyNameQ()
                                         + " with non-constant target state values");
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        AstConst* ಧ್ರುವ constp = VN_AS(valuep, Const);
        ಪ್ರತ್ಯಾಗಮ add_ಮೌಲ್ಯ_To_State_Space(valuep, stateSpace, ಮೌಲ್ಯ, stateLabelForConst(constp));
    }

    ಸ್ಥಿರ ಶೂನ್ಯ addResetTargetsToStateSpace(ಧ್ರುವ std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        ಪ್ರತಿ (ಧ್ರುವ FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.to_ಮೌಲ್ಯ().ascii(), ಅಸತ್ಯ, 0};
            ಯದಿ (AstConst* ಧ್ರುವ constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                add_ಮೌಲ್ಯ_To_State_Space(resetArc.nodep(), stateSpace, resetArc.to_ಮೌಲ್ಯ(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ collectDeclaredStateSpace(Ast_ಗಂಟು* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, ತಾರ್ಕಿಕ& needs_Source_ಮೌಲ್ಯಗಳು) {
        AstVar* ಧ್ರುವ stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        ಯದಿ (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        ಧ್ರುವ ತಾರ್ಕಿಕ forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        ಯದಿ (enump) {
            stateSpace.enumBacked = ಸತ್ಯ;
            ಪ್ರತಿ (AstEnumItem* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_AS(itemp->valuep(), Const);
                ಧ್ರುವ Fsm_State_ಮೌಲ್ಯ ಮೌಲ್ಯ{constp->ಅಂಕೆ()};
                ಧ್ರುವ size_t stateIndex = stateSpace.states.size();
                stateSpace.states.emplace_back(itemp->name(), ಮೌಲ್ಯ);
                stateSpace.labels.emplace(ಮೌಲ್ಯ,
                                          StateConstLabel{itemp->name(), ಅಸತ್ಯ, stateIndex});
            }
            ಪ್ರತ್ಯಾಗಮ stateSpace.states.size() >= 2;
        }

        ಯದಿ (forced) {
            needs_Source_ಮೌಲ್ಯಗಳು = ಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }

        needs_Source_ಮೌಲ್ಯಗಳು = ಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    template <typename T_ValuepVisitor>
    ಸ್ಥಿರ ತಾರ್ಕಿಕ collect_State_Space_From_ಮೌಲ್ಯಗಳು(Ast_ಗಂಟು* warnNodep, AstVarScope* stateVscp,
                                            ಧ್ರುವ std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            ಧ್ರುವ T_ValuepVisitor& visitValueps) {
        ತಾರ್ಕಿಕ needs_Source_ಮೌಲ್ಯಗಳು = ಅಸತ್ಯ;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        ಯದಿ (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needs_Source_ಮೌಲ್ಯಗಳು)) {
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಯದಿ (!needs_Source_ಮೌಲ್ಯಗಳು) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        addResetTargetsToStateSpace(resetArcs, stateSpace);
        ಯದಿ (!visitValueps(
                [&](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) { ಪ್ರತ್ಯಾಗಮ add_ಅಭಿವ್ಯಕ್ತಿ_To_State_Space(valuep, stateSpace); })) {
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ stateSpace.states.size() >= 2;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* assignVscp,
                                  ಧ್ರುವ std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        ಪ್ರತ್ಯಾಗಮ collect_State_Space_From_ಮೌಲ್ಯಗಳು(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, assignVscp, &stateSpace](ಧ್ರುವ auto& visitValuep) {
                ಪ್ರತಿ (AstCaseItem* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), CaseItem)) {
                    ಯದಿ (!itemp->isDefault()) {
                        ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                            ಯದಿ (!visitValuep(condp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                        }
                    }
                    ಯದಿ (Ast_ಗಂಟು_Assign* ಧ್ರುವ assp
                        = directStateAssign(itemp->stmtsp(), assignVscp)) {
                        Fsm_State_ಮೌಲ್ಯ then_ಮೌಲ್ಯ;
                        Fsm_State_ಮೌಲ್ಯ else_ಮೌಲ್ಯ;
                        AstCond* ಧ್ರುವ condp = VN_CAST(assp->rhsp(), Cond);
                        ಯದಿ (condp
                            && directStateCondConstAssign(itemp->stmtsp(), assignVscp, then_ಮೌಲ್ಯ,
                                                          else_ಮೌಲ್ಯ)) {
                            ಯದಿ (!visitValuep(condp->thenp())) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                            ಯದಿ (!visitValuep(condp->elsep())) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                        } ಅನ್ಯಥಾ ಯದಿ (!add_Optional_Target_ಅಭಿವ್ಯಕ್ತಿ_To_State_Space(assp->rhsp(), stateSpace)) {
                            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                        }
                    }
                }
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            });
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ collectStateSpace(ಧ್ರುವ FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* assignVscp,
                                  ಧ್ರುವ std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        ಪ್ರತ್ಯಾಗಮ collect_State_Space_From_ಮೌಲ್ಯಗಳು(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, assignVscp, &stateSpace](ಧ್ರುವ auto& visitValuep) {
                ಪ್ರತಿ (ಧ್ರುವ FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(visitValuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    Ast_ಗಂಟು_Assign* ಧ್ರುವ assp = directStateAssign(branch.stmtsp, assignVscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    Fsm_State_ಮೌಲ್ಯ then_ಮೌಲ್ಯ;
                    Fsm_State_ಮೌಲ್ಯ else_ಮೌಲ್ಯ;
                    AstCond* ಧ್ರುವ condp = VN_CAST(assp->rhsp(), Cond);
                    ಯದಿ (condp) {
                        UASSERT_OBJ(directStateCondConstAssign(branch.stmtsp, assignVscp,
                                                               then_ಮೌಲ್ಯ, else_ಮೌಲ್ಯ),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        ಯದಿ (!visitValuep(condp->thenp())) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                        ಯದಿ (!visitValuep(condp->elsep())) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                    } ಅನ್ಯಥಾ ಯದಿ (!add_Optional_Target_ಅಭಿವ್ಯಕ್ತಿ_To_State_Space(assp->rhsp(), stateSpace)) {
                        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                    }
                }
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            });
    }

    // Extract supported case-item transitions in one place so the conservative
    // policy for direct and ternary forms stays consistent. The false exits in
    // this helper are deliberate subset boundaries: they document shapes we do
    // not yet model in this PR and that future FSM-detection work may widen.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ emitCaseItemArcs(FsmGraph& graph, AstCaseItem* itemp, AstVarScope* stateVscp,
                                 ಧ್ರುವ FsmStateSpace& stateSpace, ತಾರ್ಕಿಕ inclCond) {
        std::vector<std::pair<string, Fsm_State_ಮೌಲ್ಯ>> froms;
        ಯದಿ (itemp->isDefault()) {
            ಯದಿ (!inclCond) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            froms.emplace_back("default", Fsm_State_ಮೌಲ್ಯ{});
        } ಅನ್ಯಥಾ {
            ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                Fsm_State_ಮೌಲ್ಯ ಮೌಲ್ಯ;
                ಯದಿ (const_ಮೌಲ್ಯ_Status(condp, ಮೌಲ್ಯ) != Const_ಮೌಲ್ಯ_Status::OK) ಅನುವರ್ತ;
                ಯದಿ (!validate_Known_State_ಮೌಲ್ಯ(condp, stateSpace, ಮೌಲ್ಯ, "source")) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                froms.emplace_back(label_For_ಮೌಲ್ಯ(stateSpace.labels, ಮೌಲ್ಯ), ಮೌಲ್ಯ);
            }
            ಯದಿ (froms.empty()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }

        ಯದಿ (Ast_ಗಂಟು_Assign* ಧ್ರುವ assp = directStateAssign(itemp->stmtsp(), stateVscp)) {
            Fsm_State_ಮೌಲ್ಯ to_ಮೌಲ್ಯ;
            ಧ್ರುವ Const_ಮೌಲ್ಯ_Status status = const_ಮೌಲ್ಯ_Status(assp->rhsp(), to_ಮೌಲ್ಯ);
            ಯದಿ (status == Const_ಮೌಲ್ಯ_Status::OK) {
                ಯದಿ (!validate_Known_State_ಮೌಲ್ಯ(assp, stateSpace, to_ಮೌಲ್ಯ, "target")) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                ಪ್ರತಿ (ಧ್ರುವ std::pair<string, Fsm_State_ಮೌಲ್ಯ>& from : froms) {
                    graph.addArc(from.second, to_ಮೌಲ್ಯ, ಅಸತ್ಯ, ಅಸತ್ಯ, itemp->isDefault(),
                                 assp->fileline());
                }
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }

        Fsm_State_ಮೌಲ್ಯ then_ಮೌಲ್ಯ;
        Fsm_State_ಮೌಲ್ಯ else_ಮೌಲ್ಯ;
        ಯದಿ (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_ಮೌಲ್ಯ, else_ಮೌಲ್ಯ)
            || ifStateConstAssign(itemp->stmtsp(), stateVscp, then_ಮೌಲ್ಯ, else_ಮೌಲ್ಯ)) {
            ಯದಿ (!validate_Known_State_ಮೌಲ್ಯ(itemp->stmtsp(), stateSpace, then_ಮೌಲ್ಯ, "target"))
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            ಯದಿ (!validate_Known_State_ಮೌಲ್ಯ(itemp->stmtsp(), stateSpace, else_ಮೌಲ್ಯ, "target"))
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            ಪ್ರತಿ (ಧ್ರುವ Fsm_State_ಮೌಲ್ಯ& branch_ಮೌಲ್ಯ : {then_ಮೌಲ್ಯ, else_ಮೌಲ್ಯ}) {
                ಪ್ರತಿ (ಧ್ರುವ std::pair<string, Fsm_State_ಮೌಲ್ಯ>& from : froms) {
                    graph.addArc(from.second, branch_ಮೌಲ್ಯ, ಅಸತ್ಯ, ಸತ್ಯ, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }

        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ emitStmtArcsFrom(FsmGraph& graph, Ast_ಗಂಟು* stmtsp, AstVarScope* stateVscp,
                                 ಧ್ರುವ FsmStateSpace& stateSpace, Fsm_State_ಮೌಲ್ಯ from_ಮೌಲ್ಯ,
                                 ತಾರ್ಕಿಕ isDefault, ತಾರ್ಕಿಕ forceCond) {
        Ast_ಗಂಟು_Assign* ಧ್ರುವ assp = directStateAssign(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        Fsm_State_ಮೌಲ್ಯ to_ಮೌಲ್ಯ;
        ಧ್ರುವ Const_ಮೌಲ್ಯ_Status status = const_ಮೌಲ್ಯ_Status(assp->rhsp(), to_ಮೌಲ್ಯ);
        ಯದಿ (status == Const_ಮೌಲ್ಯ_Status::OK) {
            ಯದಿ (!validate_Known_State_ಮೌಲ್ಯ(assp, stateSpace, to_ಮೌಲ್ಯ, "target")) ಪ್ರತ್ಯಾಗಮ;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            graph.addArc(from_ಮೌಲ್ಯ, to_ಮೌಲ್ಯ, ಅಸತ್ಯ, forceCond, isDefault, assp->fileline());
            ಪ್ರತ್ಯಾಗಮ;
        }

        Fsm_State_ಮೌಲ್ಯ then_ಮೌಲ್ಯ;
        Fsm_State_ಮೌಲ್ಯ else_ಮೌಲ್ಯ;
        ಧ್ರುವ ತಾರ್ಕಿಕ condAssign
            = directStateCondConstAssign(stmtsp, stateVscp, then_ಮೌಲ್ಯ, else_ಮೌಲ್ಯ);
        UASSERT_OBJ(condAssign, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        ಯದಿ (!validate_Known_State_ಮೌಲ್ಯ(stmtsp, stateSpace, then_ಮೌಲ್ಯ, "target")) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!validate_Known_State_ಮೌಲ್ಯ(stmtsp, stateSpace, else_ಮೌಲ್ಯ, "target")) ಪ್ರತ್ಯಾಗಮ;
        ಪ್ರತಿ (ಧ್ರುವ Fsm_State_ಮೌಲ್ಯ& branch_ಮೌಲ್ಯ : {then_ಮೌಲ್ಯ, else_ಮೌಲ್ಯ}) {
            graph.addArc(from_ಮೌಲ್ಯ, branch_ಮೌಲ್ಯ, ಅಸತ್ಯ, ಸತ್ಯ, isDefault, stmtsp->fileline());
        }
    }

    ಸ್ಥಿರ ಶೂನ್ಯ emitIfChainArcs(FsmGraph& graph, ಧ್ರುವ FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, ಧ್ರುವ FsmStateSpace& stateSpace) {
        ಪ್ರತಿ (size_t i = 0; i < chain.branches.size(); ++i) {
            ಧ್ರುವ FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            ಯದಿ (!validate_Known_State_ಮೌಲ್ಯ(branch.ifp, stateSpace, branch.from_ಮೌಲ್ಯ, "source"))
                ಪ್ರತ್ಯಾಗಮ;
            emitStmtArcsFrom(graph, branch.stmtsp, stateVscp, stateSpace, branch.from_ಮೌಲ್ಯ, ಅಸತ್ಯ,
                             branch.has_ತುದಿ_Guard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    ಸ್ಥಿರ ಶೂನ್ಯ addResetArcs(FsmGraph& graph, ಧ್ರುವ std::vector<FsmResetArcDesc>& resetArcs,
                             ಧ್ರುವ FsmStateSpace& stateSpace) {
        ಪ್ರತಿ (ಧ್ರುವ FsmResetArcDesc& resetArc : resetArcs) {
            ಯದಿ (!validate_Known_State_ಮೌಲ್ಯ(resetArc.nodep(), stateSpace, resetArc.to_ಮೌಲ್ಯ(),
                                         "target"))
                ಅನುವರ್ತ;
            graph.addArc(Fsm_State_ಮೌಲ್ಯ{}, resetArc.to_ಮೌಲ್ಯ(), ಸತ್ಯ, ಅಸತ್ಯ, ಅಸತ್ಯ,
                         resetArc.nodep()->fileline());
        }
    }

    // Turn one candidate case statement into the graph representation that the
    // later lowering phase will consume directly, while reviewers can still
    // inspect the extracted machine via DOT dumps.
    ಶೂನ್ಯ processCase(AstCase* casep, AstVarScope* assignVscp, ಧ್ರುವ FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, casep, "FSM case processing requires a non-null assignment var");
        AstVarScope* ಧ್ರುವ stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        ಯದಿ (!collectStateSpace(casep, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) ಪ್ರತ್ಯಾಗಮ;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        ಯದಿ (!entry.graphp) {
            entry.graphp.reset(ನೂತನ FsmGraph{});
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
            ಪ್ರತಿ (ಧ್ರುವ std::pair<string, Fsm_State_ಮೌಲ್ಯ>& state : stateSpace.states) {
                entry.graphp->addStateVertex(state.first, state.second);
            }
            addResetArcs(*entry.graphp, reg.resetArcs(), stateSpace);
        }
        ಪ್ರತಿ (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            emitCaseItemArcs(*entry.graphp, itemp, assignVscp, stateSpace,
                             entry.graphp->inclCond());
        }
    }

    ಶೂನ್ಯ processIfChain(ಧ್ರುವ FsmIfChainCandidate& chain, AstVarScope* assignVscp,
                        ಧ್ರುವ FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, chain.ifp,
                    "FSM if-chain processing requires a non-null assignment var");
        AstVarScope* ಧ್ರುವ stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        ಯದಿ (!collectStateSpace(chain, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) ಪ್ರತ್ಯಾಗಮ;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        // Case candidates keep ownership of existing graphs; reaching this path
        // means the if-chain is the only supported dispatch for this FSM.
        UASSERT_OBJ(!entry.graphp, chain.ifp, "FSM if-chain graph should not already exist");
        entry.graphp.reset(ನೂತನ FsmGraph{});
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
        ಪ್ರತಿ (ಧ್ರುವ std::pair<string, Fsm_State_ಮೌಲ್ಯ>& state : stateSpace.states) {
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
    ಶೂನ್ಯ processOneBlockAlways(ಧ್ರುವ FsmComboAlways& oneBlock) {
        ಧ್ರುವ RegisterAlwaysAnalyzer analyzer{oneBlock.scopep()};
        AstAlways* ಧ್ರುವ alwaysp = oneBlock.alwaysp();
        ಯದಿ (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ std::vector<std::pair<AstCase*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>> candidates
            = analyzer.oneBlockCandidates(alwaysp);

        FsmCaseCandidate firstCand;
        ಪ್ರತಿ (ಧ್ರುವ std::pair<AstCase*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& cand : candidates) {
            AstVarRef* ಧ್ರುವ selp = VN_CAST(cand.first->exprp(), VarRef);
            AstVarScope* ಧ್ರುವ vscp = selp ? selp->varScopep() : nullptr;
            ಯದಿ (!vscp) ಅನುವರ್ತ;
            ಯದಿ (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processCase(cand.first, vscp, reg);
            } ಅನ್ಯಥಾ ಯದಿ (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed case statements found in "
                                   "the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } ಅನ್ಯಥಾ {
                cand.first->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported case "
                                   "statements found in the same always block. Only the first "
                                   "candidate will be instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            }
        }

        ಯದಿ (firstCand.stateVscp) ಪ್ರತ್ಯಾಗಮ;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        ಧ್ರುವ std::vector<std::pair<AstIf*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>> ifCandidates
            = analyzer.oneBlockIfCandidates(alwaysp);
        ಪ್ರತಿ (ಧ್ರುವ std::pair<AstIf*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& cand : ifCandidates) {
            ಧ್ರುವ FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.first);
            FsmIfChainCandidate chain;
            ಯದಿ (!collectIfChain(cand.first, aliases, chain)) ಅನುವರ್ತ;
            AstVarScope* ಧ್ರುವ vscp = chain.compareVscp;
            ಯದಿ (!if_Chain_Supported_Transition_ಗಂಟು(chain, vscp)) ಅನುವರ್ತ;
            ಯದಿ (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processIfChain(chain, vscp, reg);
            } ಅನ್ಯಥಾ ಯದಿ (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed transition candidates found "
                                   "in the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } ಅನ್ಯಥಾ {
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
    ಸ್ಥಿರ ಶೂನ್ಯ warnComboSameAlways(Ast_ಗಂಟು* warnNodep, ಧ್ರುವ FsmCaseCandidate& firstCand) {
        warnNodep->v3warn(FSMMULTI,
                          "FSM coverage: multiple supported transition candidates found in "
                          "the same combinational always block. Only the first candidate "
                          "will be instrumented."
                              << candidateConflictContext(warnNodep, firstCand));
    }

    ಶೂನ್ಯ processComboAlways(ಧ್ರುವ FsmComboAlways& combo) {
        ಧ್ರುವ ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_ಗಂಟು* ಧ್ರುವ stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate firstCand;
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ಧ್ರುವ casep = VN_CAST(nodep, Case);
            ಯದಿ (!casep) ಅನುವರ್ತ;
            ಧ್ರುವ ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchCase(stmtsp, casep);
            ಧ್ರುವ FsmRegisterCandidate* ಧ್ರುವ matchedp = match.matchedp;
            Ast_ಗಂಟು* ಧ್ರುವ matchedWarnNodep = match.warnNodep;
            ಯದಿ (!matchedp) ಅನುವರ್ತ;
            ಯದಿ (!firstCand.stateVscp) {
                ಧ್ರುವ auto ಸೇರಿಸು_Pair = m_comboPaired.emplace(
                    matchedp->stateVscp(),
                    FsmCaseCandidate{matchedWarnNodep,
                                     const_cast<AstVarScope*>(matchedp->stateVscp())});
                ಯದಿ (!ಸೇರಿಸು_Pair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  ಸೇರಿಸು_Pair.first->second));
                    ಅನುವರ್ತ;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processCase(casep, matchedp->nextVscp(), *matchedp);
                ಅನುವರ್ತ;
            }
            ಯದಿ (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                ಅನುವರ್ತ;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported case statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, firstCand));
        }
        ಯದಿ (firstCand.stateVscp) ಪ್ರತ್ಯಾಗಮ;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* ಧ್ರುವ ifp = VN_CAST(nodep, If);
            ಯದಿ (!ifp) ಅನುವರ್ತ;
            FsmIfChainCandidate chain;
            ಧ್ರುವ FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            ಯದಿ (!collectIfChain(ifp, aliases, chain)) ಅನುವರ್ತ;
            ಧ್ರುವ ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchIfChain(stmtsp, chain);
            ಧ್ರುವ FsmRegisterCandidate* ಧ್ರುವ matchedp = match.matchedp;
            Ast_ಗಂಟು* ಧ್ರುವ matchedWarnNodep = match.warnNodep;
            ಯದಿ (!matchedp) ಅನುವರ್ತ;
            ಯದಿ (!firstCand.stateVscp) {
                ಧ್ರುವ std::pair<std::unordered_map<ಧ್ರುವ AstVarScope*, FsmCaseCandidate>::iterator,
                                ತಾರ್ಕಿಕ>
                    ಸೇರಿಸು_Pair = m_comboPaired.emplace(
                        matchedp->stateVscp(),
                        FsmCaseCandidate{matchedWarnNodep,
                                         const_cast<AstVarScope*>(matchedp->stateVscp())});
                ಯದಿ (!ಸೇರಿಸು_Pair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  ಸೇರಿಸು_Pair.first->second));
                    ಅನುವರ್ತ;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processIfChain(chain, matchedp->nextVscp(), *matchedp);
                ಅನುವರ್ತ;
            }
            ಯದಿ (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                ಅನುವರ್ತ;
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
    ಶೂನ್ಯ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    // Collect processes first, then analyze FSM candidates once all alias and
    // register information is available.
    ಶೂನ್ಯ visit(AstAlways* nodep) override {
        ಯದಿ (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_oneBlockAlwayss.emplace_back(m_scopep, nodep);
        ಧ್ರುವ RegisterAlwaysAnalyzer analyzer{m_scopep};
        FsmRegisterCandidate reg;
        ಯದಿ (analyzer.matchRegisterCandidate(nodep, reg)) {
            AstVarScope* ಧ್ರುವ stateVscp = reg.stateVscp();
            ಧ್ರುವ ತಾರ್ಕಿಕ found
                = std::any_of(m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                              [stateVscp](ಧ್ರುವ FsmRegisterCandidate& existing) {
                                  ಪ್ರತ್ಯಾಗಮ existing.stateVscp() == stateVscp;
                              });
            ಯದಿ (!found) { m_registerCandidates.emplace_back(reg); }
        }
        ಯದಿ (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            m_comboAlwayss.emplace_back(m_scopep, nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->keyword() == VAlwaysKwd::ALWAYS) {
            ಯದಿ (!nodep->sentreep() || isPlainComboSentree(nodep->sentreep())) {
                m_comboAlwayss.emplace_back(m_scopep, nodep);
            } ಅನ್ಯಥಾ {
                m_nonComboAlwayss.emplace_back(m_scopep, nodep);
            }
        }
    }

    ಶೂನ್ಯ visit(AstAssignW* nodep) override {
        // Continuous aliases are unordered hardware connections, so source
        // order should not affect whether an if-chain FSM is recognized.
        collectAliasFromAssign(nodep, m_stateAliases, m_ambiguousStateAliases);
        collectCellPortAlias(nodep);
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstCell* nodep) override {
        // Cells are matched after the full traversal because linkdot lowers
        // uninlined port connections into sibling continuous assignments.
        ಯದಿ (m_scopep && fsmRegisterWrapperDesc(nodep)) addWrapperCell(m_scopep, nodep);
        iterateChildren(nodep);
    }

    // Continue the walk through the rest of the design hierarchy.
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        ಪ್ರತಿ (ಧ್ರುವ std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            FsmRegisterCandidate reg;
            ಯದಿ (matchFsmWrapperCell(wrapperCell.first, wrapperCell.second, reg)) {
                m_registerCandidates.emplace_back(reg);
            }
        }
        ಪ್ರತಿ (ಧ್ರುವ FsmComboAlways& oneBlock : m_oneBlockAlwayss) processOneBlockAlways(oneBlock);
        ಪ್ರತಿ (ಧ್ರುವ FsmComboAlways& combo : m_comboAlwayss) processComboAlways(combo);
        ಪ್ರತಿ (ಧ್ರುವ FsmComboAlways& combo : m_nonComboAlwayss) warnUnsupportedComboAlways(combo);
    }
};

// Builder of statements that will handle incrementation of proper AstNodeCoverDecls - which will
// be also created here
ವರ್ಗ FsmCoverIncBuilder final {
    ಧ್ರುವ FsmGraph& m_graph;  // Fsm graph
    std::vector<ಧ್ರುವ FsmStateVertex*> m_states;  // States vertexes inside fsm graph
    std::unordered_set<uint64_t>
        m_undefinedStates;  // States that have no enum value but are possible to represent with a
                            // variable of given width.
                            // This variable keeps uint64_t instead of V3Numebr because it is safe
                            // to assume that nobody has over 16M TB of RAM
    ಧ್ರುವ FsmVertex* m_defaultVertexp = nullptr;  // Default vertex
    ಧ್ರುವ FsmVertex* m_resetVertexp = nullptr;  // Reset vertex
    size_t m_edges_ಎಣಿಕೆ = 0;  // Total count of edges
    AstVarScope* ಧ್ರುವ m_prevVscp;  // Previous state of fsm
    File_ಸಾಲು* ಧ್ರುವ m_flp;  // Fileline pointer to use across the builder
    ಧ್ರುವ std::string m_modPrettyName;  // Pretty fsm mode name
    Ast_ಗಂಟು_Stmt* m_coverIncp = nullptr;  // Increments proper CoverDecl after each change
    Ast_ಗಂಟು_Stmt* m_initialCoverIncp = nullptr;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = nullptr;  // List of CoverDecls
    ತಾರ್ಕಿಕ m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(ಧ್ರುವ uint64_t ಮೌಲ್ಯ) {
        V3_ಸಂಖ್ಯೆ ಅಂಕೆ{static_cast<Ast_ಗಂಟು*>(nullptr), m_prevVscp->width()};
        ಅಂಕೆ.setQuad(ಮೌಲ್ಯ);
        ಅಂಕೆ.isSigned(m_prevVscp->dtypep()->isSigned());
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{m_flp, ಅಂಕೆ};
    }

    std::string quadToBitStirngLabel(ಧ್ರುವ uint64_t val) {
        ಪೂರ್ಣಾಙ್ಕ width = m_prevVscp->width();
        UASSERT(width < 64, "We should not expand values that wide");
        std::string result = std::to_string(width) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        ಯಾವತ್ (--width >= 0) result.ತಳ್ಳು_back("01"[(val >> width) & 1]);
        ಪ್ರತ್ಯಾಗಮ result;
    }

    ಶೂನ್ಯ initExistingStates() {
        ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : m_graph.vertices()) {
            ಧ್ರುವ FsmVertex* ಧ್ರುವ vertexp = vtx.as<FsmVertex>();
            m_edges_ಎಣಿಕೆ += vertexp->inEdges().size();
            ಯದಿ (ಧ್ರುವ FsmStateVertex* ಧ್ರುವ statep = vertexp->cast<FsmStateVertex>()) {
                m_states.ತಳ್ಳು_back(statep);
            } ಅನ್ಯಥಾ ಯದಿ (vertexp->isDefaultAny()) {
                UASSERT_OBJ(!std::exchange(m_defaultVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two defaults?");
            } ಅನ್ಯಥಾ /* if (vertexp->isResetAny()) */ {
                UASSERT_OBJ(!std::exchange(m_resetVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two resets?");
            }
        }
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = m_graph.sampleVarScopep()->width();
        m_isComplete = width < 64 && m_states.size() == (static_cast<size_t>(1) << width);
    }

    ಶೂನ್ಯ initUndefinedStates() {
        UASSERT_OBJ(m_graph.sampleVarScopep()->width() < 64, m_graph.sampleVarScopep(),
                    "This FSM is too wide to expand - this should be caught earlier");
        ಧ್ರುವ uint64_t combinations_ಎಣಿಕೆ = static_cast<size_t>(1)
                                           << m_graph.sampleVarScopep()->width();
        // Rolling trick allows to cut peak memory usage
        ತಾರ್ಕಿಕ undefRolled = ಅಸತ್ಯ;
        auto rollUndefs = [ಏತತ್, combinations_ಎಣಿಕೆ] {
            ಪ್ರತಿ (uint64_t i = 0; i < combinations_ಎಣಿಕೆ; ++i) {
                ಧ್ರುವ auto ಸೇರಿಸು = m_undefinedStates.ಸೇರಿಸು(i);
                ಯದಿ (!ಸೇರಿಸು.second) m_undefinedStates.erase(ಸೇರಿಸು.first);
            }
        };
        ಪ್ರತಿ (ಧ್ರುವ FsmStateVertex* ಧ್ರುವ statep : m_states) {
            ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = statep->ಮೌಲ್ಯ().ಅಂಕೆ();
            ಧ್ರುವ uint64_t ಮೌಲ್ಯ = ಅಂಕೆ.toUQuad();
            ಯದಿ (!m_isComplete) {
                ಯದಿ (undefRolled) {
                    UASSERT_OBJ(m_undefinedStates.erase(ಮೌಲ್ಯ) == 1, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                } ಅನ್ಯಥಾ {
                    UASSERT_OBJ(m_undefinedStates.ಸೇರಿಸು(ಮೌಲ್ಯ).second, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                    ಯದಿ (m_undefinedStates.size() * 2 >= combinations_ಎಣಿಕೆ) {
                        undefRolled = ಸತ್ಯ;
                        rollUndefs();
                    }
                }
            }
        }
        ಯದಿ (!(m_isComplete || undefRolled)) rollUndefs();
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* buildResetCond() ಧ್ರುವ {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ refp
            = ನೂತನ AstVarRef{m_flp, m_graph.resetCond().varScopep, VAccess::READ};
        ಪ್ರತ್ಯಾಗಮ m_graph.resetCond().active_ಕೆಳಗಿನ
                   ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstLogNot{m_flp, refp})
                   : refp;
    }

    size_t get_Arc_ಎಣಿಕೆ_With_Expansion(ಧ್ರುವ VFsmExpandType expand) ಧ್ರುವ {
        size_t arc_ಎಣಿಕೆ_With_Expansion = ~0ULL;
        ವಿಕಲ್ಪ (expand) {
        ಪಕ್ಷ VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_graph.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ VFsmExpandType::AUTO: {
            arc_ಎಣಿಕೆ_With_Expansion = m_edges_ಎಣಿಕೆ;
            ಯದಿ (m_defaultVertexp) {
                UASSERT_OBJ(m_states.size() >= m_defaultVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arc_ಎಣಿಕೆ_With_Expansion -= m_defaultVertexp->outEdges().size();
                ಪ್ರತಿ (ಧ್ರುವ FsmStateVertex* ಧ್ರುವ statep : m_states) {
                    arc_ಎಣಿಕೆ_With_Expansion += static_cast<size_t>(statep->outEdges().empty())
                                             * m_defaultVertexp->outEdges().size();
                }
            }
            ಯದಿ (m_resetVertexp) {
                UASSERT_OBJ(m_states.size() >= m_resetVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arc_ಎಣಿಕೆ_With_Expansion += m_states.size() - m_resetVertexp->outEdges().size();
            }
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ VFsmExpandType::AUTO_EXPAND: {
            arc_ಎಣಿಕೆ_With_Expansion = m_states.size() * m_states.size() - m_states.size();
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ VFsmExpandType::FULL: {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = m_graph.sampleVarScopep()->width();
            // 32 is derived from:
            // tranitionsCount <= std::numeric_limits<uint64_t>::max()
            // (2^width)^2 - (2^width) <= 2^64 - 1
            // (2^width)^2 - (2^width) < 2^64 since we operate on integral values we can trade 1
            // log_2((2^width)^2 - (2^width)) < 64
            // log_2((2^width)^2) < 64 - `(2^width)` is negigible so, it may be dropped
            // 2width < 64
            ಯದಿ (width < 32) {
                ಧ್ರುವ uint64_t combinations_ಎಣಿಕೆ = static_cast<size_t>(1) << width;
                arc_ಎಣಿಕೆ_With_Expansion = combinations_ಎಣಿಕೆ * combinations_ಎಣಿಕೆ - combinations_ಎಣಿಕೆ;
            }
            ಭಙ್ಗ;
        }
        }
        ಪ್ರತ್ಯಾಗಮ arc_ಎಣಿಕೆ_With_Expansion;
    }

    AstCoverOtherDecl* buildStateCoverDecl(ಧ್ರುವ std::string& label,
                                           ಧ್ರುವ std::string& fsmTag = "") {
        AstCoverOtherDecl* ಧ್ರುವ declp
            = ನೂತನ AstCoverOtherDecl{m_flp,
                                    "v_fsm_state/" + m_modPrettyName,
                                    m_graph.stateVarName() + "::" + label,
                                    "",
                                    0,
                                    m_graph.stateVarName(),
                                    "",
                                    label,
                                    fsmTag};
        // We run after scoping, so this is declared for one instance only
        declp->perInstance(ಸತ್ಯ);
        m_declsp = Ast_ಗಂಟು::addNext(m_declsp, declp);
        ಪ್ರತ್ಯಾಗಮ declp;
    }

    Ast_ಗಂಟು_Stmt* buildCoverIncStmtp(ಧ್ರುವ std::string& fromStateLabel,
                                    ಧ್ರುವ std::string& toStateLabel,
                                    ಧ್ರುವ std::string& baseLabel = "") {
        // Whole builder is designed in such a way that this function won't be called twice with
        // the same arguments - but if it will ever change in future, `AstCoverOtherDecl` shall be
        // cached for particular args set
        AstCoverOtherDecl* ಧ್ರುವ declp = ನೂತನ AstCoverOtherDecl{
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
        declp->perInstance(ಸತ್ಯ);
        m_declsp = Ast_ಗಂಟು::addNext(m_declsp, declp);
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCoverInc{m_flp, declp};
    }
    Ast_ಗಂಟು_Stmt* buildExpandedTransitionHandler(ಧ್ರುವ std::string& toStateLabel,
                                                ಧ್ರುವ ತಾರ್ಕಿಕ considerUndefs,
                                                ಧ್ರುವ ತಾರ್ಕಿಕ expandOnlyWithTerminalVerticies,
                                                ಧ್ರುವ FsmStateVertex* ಧ್ರುವ toStatep = nullptr) {
        AstCase* ಧ್ರುವ arcCasep = ನೂತನ AstCase{
            m_flp, VCaseType::CT_CASE, ನೂತನ AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        std::unordered_set<ಧ್ರುವ FsmStateVertex*> handledSoureces;
        ಯದಿ (toStatep) {
            ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : toStatep->inEdges()) {
                ಯದಿ (ಧ್ರುವ FsmStateVertex* ಧ್ರುವ fromp = edge.fromp()->cast<FsmStateVertex>()) {
                    ಯದಿ (!handledSoureces.emplace(fromp).second) ಅನುವರ್ತ;
                    arcCasep->addItemsp(ನೂತನ AstCaseItem{
                        m_flp, makeConst(fromp->ಮೌಲ್ಯ().ಅಂಕೆ().toUQuad()),
                        (fromp != toStatep) ? buildCoverIncStmtp(fromp->label(), toStateLabel)
                                            : nullptr});
                }
            }
        }
        ಪ್ರತಿ (ಧ್ರುವ FsmStateVertex* ಧ್ರುವ fromp : m_states) {
            ಯದಿ (handledSoureces.find(fromp) != handledSoureces.end()) ಅನುವರ್ತ;
            arcCasep->addItemsp(ನೂತನ AstCaseItem{
                m_flp, makeConst(fromp->ಮೌಲ್ಯ().ಅಂಕೆ().toUQuad()),
                (fromp != toStatep
                 && (!expandOnlyWithTerminalVerticies || fromp->outEdges().empty()))
                    ? buildCoverIncStmtp(fromp->label(), toStateLabel, "artificial")
                    : nullptr});
        }
        ಯದಿ (!m_isComplete) {
            ಯದಿ (considerUndefs) {
                ಪ್ರತಿ (ಧ್ರುವ uint64_t from : m_undefinedStates) {
                    ಧ್ರುವ std::string& label = quadToBitStirngLabel(from);
                    arcCasep->addItemsp(
                        ನೂತನ AstCaseItem{m_flp, makeConst(from),
                                        (label != toStateLabel)
                                            ? buildCoverIncStmtp(label, toStateLabel, "artificial")
                                            : nullptr});
                }
            } ಅನ್ಯಥಾ {
                arcCasep->addItemsp(ನೂತನ AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        ಪ್ರತ್ಯಾಗಮ arcCasep;
    }
    Ast_ಗಂಟು_Stmt* buildUnxpandedTransitionHandler(ಧ್ರುವ FsmStateVertex* ಧ್ರುವ toStatep) {
        AstCase* ಧ್ರುವ arcCasep = ನೂತನ AstCase{
            m_flp, VCaseType::CT_CASE, ನೂತನ AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : toStatep->inEdges()) {
            ಧ್ರುವ FsmArcEdge* ಧ್ರುವ arcp = edge.as<FsmArcEdge>();
            UASSERT_OBJ(!arcp->isReset(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            ಧ್ರುವ FsmStateVertex* ಧ್ರುವ fromp = arcp->fromp()->as<FsmStateVertex>();
            ಯದಿ (fromp != toStatep) {
                arcCasep->addItemsp(
                    ನೂತನ AstCaseItem{m_flp, makeConst(fromp->ಮೌಲ್ಯ().ಅಂಕೆ().toUQuad()),
                                    buildCoverIncStmtp(fromp->label(), toStatep->label())});
            }
        }
        arcCasep->addItemsp(ನೂತನ AstCaseItem{m_flp, nullptr, nullptr});
        ಪ್ರತ್ಯಾಗಮ arcCasep;
    }

ಸಾರ್ವಜನಿಕ:
    FsmCoverIncBuilder(ಧ್ರುವ FsmGraph& graph, AstVarScope* ಧ್ರುವ prevVscp)
        : m_graph{graph}
        , m_prevVscp{prevVscp}
        , m_flp{graph.fileline()}
        , m_modPrettyName{graph.scopep()->modp()->prettyName()} {
        initExistingStates();
        AstVarScope* ಧ್ರುವ sampleVscp = graph.sampleVarScopep();
        ಧ್ರುವ VFsmExpandType expand
            = graph.stateVarScopep()->varp()->attrFsmStateExpand() != VFsmExpandType::DEFAULT
                  ? graph.stateVarScopep()->varp()->attrFsmStateExpand()
                  : v3Global.opt.coverageFsmExpand();

        {
            ಧ್ರುವ size_t arc_ಎಣಿಕೆ_With_Expansion = get_Arc_ಎಣಿಕೆ_With_Expansion(expand);
            ಯದಿ (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arc_ಎಣಿಕೆ_With_Expansion == 0ULL
                            || arc_ಎಣಿಕೆ_With_Expansion
                                   > static_cast<size_t>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arc_ಎಣಿಕೆ_With_Expansion);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        AstCase* ಧ್ರುವ arcCasep
            = ನೂತನ AstCase{m_flp, VCaseType::CT_CASE,
                          ನೂತನ AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};
        AstCase* ಧ್ರುವ initialStateCasep
            = ನೂತನ AstCase{m_flp, VCaseType::CT_CASE,
                          ನೂತನ AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};

        ಧ್ರುವ ತಾರ್ಕಿಕ expandAll = expand != VFsmExpandType::AUTO;
        std::unordered_set<ಧ್ರುವ FsmStateVertex*>
            defaultTargets;  // States that have edge incoming from default vertex
        std::unordered_set<ಧ್ರುವ FsmStateVertex*>
            resetTargets;  // States that have edge incoming from reset vertex
        ಯದಿ (!expandAll) {
            ಯದಿ (m_defaultVertexp) {
                ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : m_defaultVertexp->outEdges()) {
                    defaultTargets.emplace(edge.ತುದಿ()->as<FsmStateVertex>());
                }
            }
            ಯದಿ (m_resetVertexp) {
                ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : m_resetVertexp->outEdges()) {
                    resetTargets.emplace(edge.ತುದಿ()->as<FsmStateVertex>());
                }
            }
        }
        ಧ್ರುವ ತಾರ್ಕಿಕ isFullExpansion = expand == VFsmExpandType::FULL;
        ಯದಿ (isFullExpansion) initUndefinedStates();
        ಪ್ರತಿ (ಧ್ರುವ FsmStateVertex* ಧ್ರುವ statep : m_states) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* ಧ್ರುವ declp = buildStateCoverDecl(statep->label());
            initialStateCasep->addItemsp(
                ನೂತನ AstCaseItem{m_flp, makeConst(statep->ಮೌಲ್ಯ().ಅಂಕೆ().toUQuad()),
                                ನೂತನ AstCoverInc{m_flp, declp, ಸತ್ಯ}});
            ಧ್ರುವ ತಾರ್ಕಿಕ expandAllOrReset
                = expandAll || (resetTargets.find(statep) != resetTargets.end());
            ಧ್ರುವ ತಾರ್ಕಿಕ defaultExpantion
                = !expandAllOrReset && (defaultTargets.find(statep) != defaultTargets.end());
            arcCasep->addItemsp(ನೂತನ AstCaseItem{
                m_flp, makeConst(statep->ಮೌಲ್ಯ().ಅಂಕೆ().toUQuad()),
                Ast_ಗಂಟು::addNext<Ast_ಗಂಟು_Stmt>(
                    ನೂತನ AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), isFullExpansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        ಯದಿ (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            ಯದಿ (isFullExpansion) {
                ಪ್ರತಿ (ಧ್ರುವ uint64_t ಮೌಲ್ಯ : m_undefinedStates) {
                    ಧ್ರುವ std::string& label = quadToBitStirngLabel(ಮೌಲ್ಯ);
                    AstCoverOtherDecl* ಧ್ರುವ declp = buildStateCoverDecl(label);
                    initialStateCasep->addItemsp(ನೂತನ AstCaseItem{
                        m_flp, makeConst(ಮೌಲ್ಯ), ನೂತನ AstCoverInc{m_flp, declp, ಸತ್ಯ}});
                    arcCasep->addItemsp(
                        ನೂತನ AstCaseItem{m_flp, makeConst(ಮೌಲ್ಯ),
                                        Ast_ಗಂಟು::addNext<Ast_ಗಂಟು_Stmt>(
                                            ನೂತನ AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, ಸತ್ಯ, ಅಸತ್ಯ))});
                }
            } ಅನ್ಯಥಾ {
                // No expansion
                arcCasep->addItemsp(ನೂತನ AstCaseItem{m_flp, nullptr, nullptr});
                initialStateCasep->addItemsp(ನೂತನ AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        m_coverIncp = arcCasep;
        m_initialCoverIncp = initialStateCasep;
    }
    VL_UNCOPYABLE(FsmCoverIncBuilder);
    ~FsmCoverIncBuilder() = ಸ್ವಭಾವ;
    Ast_ಗಂಟು_Cover_Decl* getCoverDeclsp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_declsp; }
    Ast_ಗಂಟು_Stmt* getCoverIncp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_coverIncp; }
    Ast_ಗಂಟು_Stmt* getInitialCoverIncp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_initialCoverIncp; }
};

// Lower the completed FSM graphs into the concrete coverage declarations,
// previous-state tracking, and pre/post-triggered instrumentation that the
// runtime uses to record state and transition coverage.
ವರ್ಗ FsmLowerVisitor final {
    // STATE - across all visitors
    ಧ್ರುವ FsmState& m_state;
    V3SharedTmps m_prevTmps{"__VfsmCovPrev", VVarType::MODULETEMP};  // Previous states

    // METHODS
    // Rebuild the original event control from the saved sense description so
    // post-state coverage sampling runs on the same triggering edges.
    ಸ್ಥಿರ Ast_Sen_ವೃಕ್ಷ* build_Sen_ವೃಕ್ಷ(File_ಸಾಲು* flp, ಧ್ರುವ std::vector<FsmSenDesc>& senses) {
        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ sentreep = ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, nullptr};
        ಪ್ರತಿ (ಧ್ರುವ FsmSenDesc& sense : senses) {
            AstSenItem* ಧ್ರುವ senItemp
                = ನೂತನ AstSenItem{flp, VEdgeType{sense.edgeType},
                                 ನೂತನ AstVarRef{flp, sense.varScopep, VAccess::READ}};
            sentreep->addSensesp(senItemp);
        }
        ಪ್ರತ್ಯಾಗಮ sentreep;
    }

    // Lower one fully detected FSM graph into the concrete coverage machinery
    // used by generated models: declarations, previous-state tracking, and the
    // pre/post-triggered increment logic for states and arcs.
    ಶೂನ್ಯ buildOne(ಧ್ರುವ FsmGraph& graph) {
        UINFO(1, "buildOne lowering FSM " << graph.stateVarName()
                                          << " vertices=" << graph.vertices().size() << endl);
        AstAlways* ಧ್ರುವ alwaysp = graph.stateAlwaysp();
        AstScope* ಧ್ರುವ scopep = graph.scopep();
        AstVarScope* ಧ್ರುವ stateVscp = graph.stateVarScopep();
        AstVarScope* ಧ್ರುವ sampleVscp = graph.sampleVarScopep();
        File_ಸಾಲು* ಧ್ರುವ flp = graph.fileline();
        Ast_ಗಂಟು_Module* ಧ್ರುವ modp = scopep->modp();
        AstVarScope* ಧ್ರುವ prevVscp
            = m_prevTmps.make(flp, scopep, sampleVscp->dtypep(), stateVscp->varp()->shortName());
        // The saved previous-state temp crosses the scheduler's pre/post split
        // in the same way as Verilator's built-in NBA shadow variables, so keep
        // both vars marked as post-life participants for stable MT ordering.
        stateVscp->optimizeLifePost(ಸತ್ಯ);
        sampleVscp->optimizeLifePost(ಸತ್ಯ);
        prevVscp->optimizeLifePost(ಸತ್ಯ);

        AstActive* ಧ್ರುವ initActivep
            = ನೂತನ AstActive{flp, "fsm-coverage-init",
                            ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, ನೂತನ AstSenItem{flp, AstSenItem::Initial{}}}};
        initActivep->sen_ವೃಕ್ಷ_Storep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->addStmtsp(ನೂತನ AstInitialStatic{
            flp, ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, prevVscp, VAccess::WRITE},
                               ನೂತನ AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->addBlocksp(initActivep);

        AstAlwaysPost* ಧ್ರುವ covPostp = ನೂತನ AstAlwaysPost{flp};
        ತಾರ್ಕಿಕ updatePrevAfterPost = ಅಸತ್ಯ;
        ಯದಿ (alwaysp) {
            // Save the previous state as plain sequential logic at the front of
            // the original always_ff body, then evaluate coverage in post logic
            // after the delayed state update commits. This avoids a scheduler
            // race between a separate AstAlwaysPre task and the real state
            // commit for direct parent-level registers.
            alwaysp->prependStmtsp(ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, prevVscp, VAccess::WRITE},
                                                 ನೂತನ AstVarRef{flp, sampleVscp, VAccess::READ}});
        } ಅನ್ಯಥಾ {
            // Wrapper-derived register candidates do not have a parent
            // always_ff body to splice into. Sample coverage first, then save
            // the current state for the next clock tick; this survives cell
            // boundary scheduling where the real flop update lives elsewhere.
            updatePrevAfterPost = ಸತ್ಯ;
            prevVscp->varp()->setIgnorePostRead();
        }

        {
            // Create and increment proper AstNodeCoverDecls
            ಧ್ರುವ FsmCoverIncBuilder coverIncBuilder{graph, prevVscp};
            modp->addStmtsp(coverIncBuilder.getCoverDeclsp());
            initActivep->addStmtsp(coverIncBuilder.getInitialCoverIncp());
            covPostp->addStmtsp(
                ನೂತನ AstIf{flp,
                          ನೂತನ AstNeq{flp, ನೂತನ AstVarRef{flp, prevVscp, VAccess::READ},
                                     ನೂತನ AstVarRef{flp, sampleVscp, VAccess::READ}},
                          coverIncBuilder.getCoverIncp()});
        }

        ಯದಿ (updatePrevAfterPost) {
            covPostp->addStmtsp(ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, prevVscp, VAccess::WRITE},
                                              ನೂತನ AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ sentreep = build_Sen_ವೃಕ್ಷ(flp, graph.senses());
        AstActive* ಧ್ರುವ activep = ನೂತನ AstActive{flp, "fsm-coverage", sentreep};
        activep->sen_ವೃಕ್ಷ_Storep(sentreep);
        scopep->addBlocksp(activep);
        activep->addStmtsp(covPostp);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    // Lower every detected FSM graph from the shared local state into
    // concrete coverage instrumentation while the saved scoped pointers are
    // still valid in the same pass.
    explicit FsmLowerVisitor(ಧ್ರುವ FsmState& state)
        : m_state{state} {
        ಪ್ರತಿ (ಧ್ರುವ DetectedFsm& fsm : m_state.fsms()) { buildOne(*fsm.graphp); }
    }
};

}  // namespace

ಶೂನ್ಯ V3FsmDetect::detect(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    FsmState state;
    // Phase 1: recover each supported FSM into a complete graph while the
    // original clocked/case structure is still easy to recognize.
    FsmDetectVisitor detect{state, rootp};
    ಯದಿ (dumpGraphLevel() >= 6) {
        size_t index = 0;
        ಪ್ರತಿ (ಧ್ರುವ DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dumpTag(index++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { FsmLowerVisitor lower{state}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("fsm-detect", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
