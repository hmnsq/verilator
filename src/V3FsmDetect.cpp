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

    ಸ್ಥಿರ string make_ಕೀಲಿ(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ) {
        V3_ಸಂಖ್ಯೆ ಕೀಲಿ_ಅಂಕೆ = ಅಂಕೆ;
        // Signedness does not change FSM state identity: same width and bits
        // should address the same graph vertex.
        ಕೀಲಿ_ಅಂಕೆ.isSigned(ಸುಳ್ಳು);
        ಹಿಂತಿರುಗಿ cvt_ಗಮ್ಯ_Str(ಕೀಲಿ_ಅಂಕೆ.ಅಗಲ()) + ":" + ಕೀಲಿ_ಅಂಕೆ.ascii(ನಿಜ, ನಿಜ);
    }

ಸಾರ್ವಜನಿಕ:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    Fsm_State_ಮೌಲ್ಯ()
        : m_ಕೀಲಿ{"1:1'h0"}
        , m_ಅಂಕೆ{static_cast<Ast_ಗಂಟು*>(ಶೂನ್ಯಸೂಚಕ), 1, 0} {}
    ಸ್ಪಷ್ಟ Fsm_State_ಮೌಲ್ಯ(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ)
        : m_ಕೀಲಿ{make_ಕೀಲಿ(ಅಂಕೆ)}
        , m_ಅಂಕೆ{ಅಂಕೆ} {}

    ಸ್ಥಿರಾಂಕ string& ಕೀಲಿ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಕೀಲಿ; }
    ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಅಂಕೆ; }
    string ascii() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಅಂಕೆ.ascii(ನಿಜ, ನಿಜ); }
    string warn_ಪಠ್ಯ() ಸ್ಥಿರಾಂಕ {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        ಆದರೆ (m_ಅಂಕೆ.ಅಗಲ() <= 32) ಹಿಂತಿರುಗಿ cvt_ಗಮ್ಯ_Str(m_ಅಂಕೆ.ಗಮ್ಯ_UInt());
        ಹಿಂತಿರುಗಿ ascii();
    }

    ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ Fsm_State_ಮೌಲ್ಯ& rhs) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಕೀಲಿ == rhs.m_ಕೀಲಿ; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
ರಚನೆ Fsm_State_ಮೌಲ್ಯ_Hash final {
    ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Fsm_State_ಮೌಲ್ಯ& ಮೌಲ್ಯ) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ std::hash<string>{}(ಮೌಲ್ಯ.ಕೀಲಿ());
    }
};

// Captures one sensitivity-list entry so the lowering phase can later rebuild
// an active block with the same triggering event control.
ರಚನೆ FsmSenDesc final {
    // Encoded edge kind copied from AstSenItem::edgeType() so lowering can
    // rebuild the same trigger semantics on the synthesized coverage block.
    VEdgeType::en ಅಂಚು_Type = static_cast<VEdgeType::en>(0);
    // Triggering signal in the saved scoped AST.
    AstVarScope* varScopep = ಶೂನ್ಯಸೂಚಕ;
};

// Captures the simple reset predicate shape that survives to this pass after
// earlier normalization so reset arcs can be reconstructed during lowering.
ರಚನೆ FsmResetCondDesc final {
    // Reset signal used by the FSM in the saved scoped AST.
    AstVarScope* varScopep = ಶೂನ್ಯಸೂಚಕ;
    ತಾರ್ಕಿಕ ಸಕ್ರಿಯ_ಕೆಳಗಿನ = ಸುಳ್ಳು;
};

ವರ್ಗ FsmResetArcDesc final {
    Fsm_State_ಮೌಲ್ಯ m_ಗಮ್ಯ_ಮೌಲ್ಯ;  // Encoded reset target state.
    Ast_ಗಂಟು* m_nodep = ಶೂನ್ಯಸೂಚಕ;  // Source node for warnings and emitted metadata.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_valuep = ಶೂನ್ಯಸೂಚಕ;  // Expression that provided the reset value.

ಸಾರ್ವಜನಿಕ:
    FsmResetArcDesc() = ಪೂರ್ವನಿಯೋಜಿತ;
    FsmResetArcDesc(Fsm_State_ಮೌಲ್ಯ ಗಮ್ಯ_ಮೌಲ್ಯ, Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep)
        : m_ಗಮ್ಯ_ಮೌಲ್ಯ{ಗಮ್ಯ_ಮೌಲ್ಯ}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(Fsm_State_ಮೌಲ್ಯ ಗಮ್ಯ_ಮೌಲ್ಯ, Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : m_ಗಮ್ಯ_ಮೌಲ್ಯ{ಗಮ್ಯ_ಮೌಲ್ಯ}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    Fsm_State_ಮೌಲ್ಯ ಗಮ್ಯ_ಮೌಲ್ಯ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಗಮ್ಯ_ಮೌಲ್ಯ; }
    Ast_ಗಂಟು* nodep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_nodep; }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_valuep; }
};

ರಚನೆ FsmWrapperRoles final {
    string d_ಪೋರ್ಟ್;
    string q_ಪೋರ್ಟ್;
    string clk_ಪೋರ್ಟ್;
    string rst_ಪೋರ್ಟ್;
    string rstValParam;
    ತಾರ್ಕಿಕ has_Rst_ಸಕ್ರಿಯ_ಕೆಳಗಿನ = ಸುಳ್ಳು;
    ತಾರ್ಕಿಕ rst_ಸಕ್ರಿಯ_ಕೆಳಗಿನ = ಸುಳ್ಳು;
};

ಸ್ಥಿರ ತಾರ್ಕಿಕ fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, ಸ್ಥಿರಾಂಕ string& ಪೋರ್ಟ್_Name,
                                                  ತಾರ್ಕಿಕ& ಸಕ್ರಿಯ_ಕೆಳಗಿನ) {
    ತಾರ್ಕಿಕ matched = ಸುಳ್ಳು;
    cellp->modp()->foreach([&](Ast_Sen_ಅಂಶ* itemp) {
        Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ vrefp = itemp->varrefp();
        ಆದರೆ (!vrefp) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (vrefp->varp()->name() != ಪೋರ್ಟ್_Name) ಹಿಂತಿರುಗಿ;
        ಸಕ್ರಿಯ_ಕೆಳಗಿನ = itemp->ಅಂಚು_Type() == VEdgeType::ET_NEGEDGE;
        matched = ನಿಜ;
    });
    ಹಿಂತಿರುಗಿ matched;
}

ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ V3Control::Fsm_ರಿಜಿಸ್ಟರ್_Wrapper* fsm_ರಿಜಿಸ್ಟರ್_Wrapper_Desc(AstCell* cellp) {
    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = cellp->modp();
    ಸ್ಥಿರಾಂಕ string origName = modp->origName();
    ಆದರೆ (ಸ್ಥಿರಾಂಕ V3Control::Fsm_ರಿಜಿಸ್ಟರ್_Wrapper* ಸ್ಥಿರಾಂಕ descp
        = V3Control::get_Fsm_ರಿಜಿಸ್ಟರ್_Wrapper(origName)) {
        ಹಿಂತಿರುಗಿ descp;
    }
    ಹಿಂತಿರುಗಿ V3Control::get_Fsm_ರಿಜಿಸ್ಟರ್_Wrapper(modp->prettyDehashOrigOrName());
}

ಸ್ಥಿರ FsmWrapperRoles rolesFromDesc(ಸ್ಥಿರಾಂಕ V3Control::Fsm_ರಿಜಿಸ್ಟರ್_Wrapper& desc) {
    FsmWrapperRoles roles;
    roles.d_ಪೋರ್ಟ್ = desc.d;
    roles.q_ಪೋರ್ಟ್ = desc.q;
    roles.clk_ಪೋರ್ಟ್ = desc.clock;
    roles.rst_ಪೋರ್ಟ್ = desc.reset;
    roles.rstValParam = desc.reset_ಮೌಲ್ಯ;
    ಹಿಂತಿರುಗಿ roles;
}

ವರ್ಗ Fsm_ರಿಜಿಸ್ಟರ್_Candidate final {
    AstScope* m_scopep = ಶೂನ್ಯಸೂಚಕ;  // Owning scope for the paired FSM.
    AstAlways* m_alwaysp = ಶೂನ್ಯಸೂಚಕ;  // Register process that commits the state.
    AstVarScope* m_stateVscp = ಶೂನ್ಯಸೂಚಕ;  // Registered FSM state variable.
    AstVarScope* m_sampleVscp = ಶೂನ್ಯಸೂಚಕ;  // Variable sampled by lowered coverage logic.
    AstVarScope* m_ಮುಂದಿನ_Vscp = ಶೂನ್ಯಸೂಚಕ;  // Next-state variable or same state var for 1-block FSMs.
    std::ಸದಿಶ<FsmSenDesc> m_senses;  // Event controls for recreated coverage blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate, if any.
    std::ಸದಿಶ<FsmResetArcDesc> m_resetArcs;  // Reset target arcs recovered during detect.
    ತಾರ್ಕಿಕ m_hasResetCond = ಸುಳ್ಳು;  // Whether the FSM had a modeled reset predicate.
    ತಾರ್ಕಿಕ m_inclCond = ಸುಳ್ಳು;  // Whether conditional/default arcs are kept explicitly.
    File_ಸಾಲು* m_flp = ಶೂನ್ಯಸೂಚಕ;  // Representative source location.

ಸಾರ್ವಜನಿಕ:
    AstScope* scopep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_scopep; }
    ಶೂನ್ಯ scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* alwaysp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_alwaysp; }
    ಶೂನ್ಯ alwaysp(AstAlways* alwaysp) { m_alwaysp = alwaysp; }
    AstVarScope* stateVscp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stateVscp; }
    ಶೂನ್ಯ stateVscp(AstVarScope* vscp) { m_stateVscp = vscp; }
    AstVarScope* sampleVscp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_sampleVscp ? m_sampleVscp : m_stateVscp; }
    ಶೂನ್ಯ sampleVscp(AstVarScope* vscp) { m_sampleVscp = vscp; }
    AstVarScope* ಮುಂದಿನ_Vscp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಮುಂದಿನ_Vscp; }
    ಶೂನ್ಯ ಮುಂದಿನ_Vscp(AstVarScope* vscp) { m_ಮುಂದಿನ_Vscp = vscp; }
    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<FsmSenDesc>& senses() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_senses; }
    std::ಸದಿಶ<FsmSenDesc>& senses() { ಹಿಂತಿರುಗಿ m_senses; }
    ಸ್ಥಿರಾಂಕ FsmResetCondDesc& resetCond() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_resetCond; }
    FsmResetCondDesc& resetCond() { ಹಿಂತಿರುಗಿ m_resetCond; }
    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<FsmResetArcDesc>& resetArcs() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_resetArcs; }
    std::ಸದಿಶ<FsmResetArcDesc>& resetArcs() { ಹಿಂತಿರುಗಿ m_resetArcs; }
    ತಾರ್ಕಿಕ hasResetCond() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hasResetCond; }
    ಶೂನ್ಯ hasResetCond(ತಾರ್ಕಿಕ flag) { m_hasResetCond = flag; }
    ತಾರ್ಕಿಕ inclCond() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_inclCond; }
    ಶೂನ್ಯ inclCond(ತಾರ್ಕಿಕ flag) { m_inclCond = flag; }
    File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_flp; }
    ಶೂನ್ಯ fileline(File_ಸಾಲು* flp) { m_flp = flp; }
};

ವರ್ಗ FsmComboAlways final {
    AstScope* ಸ್ಥಿರಾಂಕ m_scopep = ಶೂನ್ಯಸೂಚಕ;  // Owning scope for the combinational process.
    AstAlways* ಸ್ಥಿರಾಂಕ m_alwaysp = ಶೂನ್ಯಸೂಚಕ;  // Candidate transition process.

ಸಾರ್ವಜನಿಕ:
    FsmComboAlways() = ಪೂರ್ವನಿಯೋಜಿತ;
    FsmComboAlways(AstScope* scopep, AstAlways* alwaysp)
        : m_scopep{scopep}
        , m_alwaysp{alwaysp} {}

    AstScope* scopep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_scopep; }
    AstAlways* alwaysp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_alwaysp; }
};

ವರ್ಗ Fsm_ನಕ್ಷೆ;

ವರ್ಗ Fsm_ಶೃಂಗ VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Fsm_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)

ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ ವರ್ಗ ಬಗೆ_2 : uint8_t { STATE, RESET_ANY, DEFAULT_ANY };

ಖಾಸಗಿ:
    ಬಗೆ_2 m_ಬಗೆ;  // State vs synthetic ANY/default vertex role.
    string m_label;  // User-facing state or pseudo-state label.
    Fsm_State_ಮೌಲ್ಯ m_ಮೌಲ್ಯ;  // Encoded state value for real state vertices.

ಸಂರಕ್ಷಿತ:
    Fsm_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, ಬಗೆ_2 ಬಗೆ, string label, Fsm_State_ಮೌಲ್ಯ ಮೌಲ್ಯ) VL_MT_DISABLED
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp},
          m_ಬಗೆ{ಬಗೆ},
          m_label{label},
          m_ಮೌಲ್ಯ{ಮೌಲ್ಯ} {}
    ~Fsm_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;

ಸಾರ್ವಜನಿಕ:
    ಬಗೆ_2 ಬಗೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಗೆ; }
    ತಾರ್ಕಿಕ isState() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಗೆ == ಬಗೆ_2::STATE; }
    ತಾರ್ಕಿಕ isResetAny() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಗೆ == ಬಗೆ_2::RESET_ANY; }
    ತಾರ್ಕಿಕ isDefaultAny() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಗೆ == ಬಗೆ_2::DEFAULT_ANY; }
    ಸ್ಥಿರಾಂಕ string& label() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_label; }
    ಸ್ಥಿರಾಂಕ Fsm_State_ಮೌಲ್ಯ& ಮೌಲ್ಯ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಮೌಲ್ಯ; }

    string name() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_label + "=" + m_ಮೌಲ್ಯ.ascii(); }
};

ವರ್ಗ Fsm_State_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ Fsm_ಶೃಂಗ {
    VL_RTTI_IMPL(Fsm_State_ಶೃಂಗ, Fsm_ಶೃಂಗ)

ಸಾರ್ವಜನಿಕ:
    Fsm_State_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, string label, Fsm_State_ಮೌಲ್ಯ ಮೌಲ್ಯ) VL_MT_DISABLED
        : Fsm_ಶೃಂಗ{graphp, ಬಗೆ_2::STATE, label, ಮೌಲ್ಯ} {}
    ~Fsm_State_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;

    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "lightblue"; }
    string dotShape() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "ellipse"; }
};

ವರ್ಗ Fsm_Pseudo_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ Fsm_ಶೃಂಗ {
    VL_RTTI_IMPL(Fsm_Pseudo_ಶೃಂಗ, Fsm_ಶೃಂಗ)

ಸಾರ್ವಜನಿಕ:
    Fsm_Pseudo_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, ಬಗೆ_2 ಬಗೆ, string label) VL_MT_DISABLED
        : Fsm_ಶೃಂಗ{graphp, ಬಗೆ, label, Fsm_State_ಮೌಲ್ಯ{}} {}
    ~Fsm_Pseudo_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;

    string name() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ label(); }
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ isResetAny() ? "darkgreen" : "orange"; }
    string dotShape() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "diamond"; }
};

ವರ್ಗ Fsm_Arc_ಅಂಚು final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಅಂಚು {
    VL_RTTI_IMPL(Fsm_Arc_ಅಂಚು, V3_ನಕ್ಷೆ_ಅಂಚು)
    ತಾರ್ಕಿಕ m_isReset = ಸುಳ್ಳು;  // Arc originates from the synthetic reset source.
    ತಾರ್ಕಿಕ m_isCond = ಸುಳ್ಳು;  // Arc came from a conditional next-state split.
    ತಾರ್ಕಿಕ m_isDefault = ಸುಳ್ಳು;  // Arc represents a case default source.
    File_ಸಾಲು* m_flp = ಶೂನ್ಯಸೂಚಕ;  // Source location for emitted coverage metadata.

ಸಾರ್ವಜನಿಕ:
    Fsm_Arc_ಅಂಚು(V3_ನಕ್ಷೆ* graphp, Fsm_ಶೃಂಗ* fromp, Fsm_State_ಶೃಂಗ* ತುದಿ, ತಾರ್ಕಿಕ isReset, ತಾರ್ಕಿಕ isCond,
               ತಾರ್ಕಿಕ isDefault, File_ಸಾಲು* flp) VL_MT_DISABLED : V3_ನಕ್ಷೆ_ಅಂಚು{graphp, fromp, ತುದಿ, 1},
                                                               m_isReset{isReset},
                                                               m_isCond{isCond},
                                                               m_isDefault{isDefault},
                                                               m_flp{flp} {}
    ~Fsm_Arc_ಅಂಚು() override = ಪೂರ್ವನಿಯೋಜಿತ;

    ತಾರ್ಕಿಕ isReset() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isReset; }
    ತಾರ್ಕಿಕ isCond() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isCond; }
    ತಾರ್ಕಿಕ isDefault() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isDefault; }
    File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_flp; }

    string dotLabel() ಸ್ಥಿರಾಂಕ override {
        ಆದರೆ (m_isReset) ಹಿಂತಿರುಗಿ "reset";
        ಆದರೆ (m_isDefault) ಹಿಂತಿರುಗಿ "default";
        ಆದರೆ (m_isCond) ಹಿಂತಿರುಗಿ "cond";
        ಹಿಂತಿರುಗಿ "";
    }
    string dotColor() ಸ್ಥಿರಾಂಕ override {
        ಆದರೆ (m_isReset) ಹಿಂತಿರುಗಿ "darkgreen";
        ಆದರೆ (m_isDefault) ಹಿಂತಿರುಗಿ "orange";
        ಆದರೆ (m_isCond) ಹಿಂತಿರುಗಿ "blue";
        ಹಿಂತಿರುಗಿ "black";
    }
};

// One graph per detected FSM. Graph-level metadata captures the non-graph
// context needed to lower states/arcs back into the AST after detection.
ವರ್ಗ Fsm_ನಕ್ಷೆ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ {
    AstScope* m_scopep = ಶೂನ್ಯಸೂಚಕ;  // Owning scoped block for the detected FSM.
    AstAlways* m_stateAlwaysp = ಶೂನ್ಯಸೂಚಕ;  // Register always block being instrumented.
    string m_stateVarName;  // Pretty state variable name for user-visible output.
    string m_state_Var_ಆಂತರಿಕ_Name;  // Internal state symbol name for dump tags.
    AstVarScope* m_stateVarScopep = ಶೂನ್ಯಸೂಚಕ;  // Scoped state variable being tracked.
    AstVarScope* m_sampleVarScopep = ಶೂನ್ಯಸೂಚಕ;  // Scoped variable sampled by coverage logic.
    std::ಸದಿಶ<FsmSenDesc> m_senses;  // Saved event controls for recreated active blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate shape, if one exists.
    ತಾರ್ಕಿಕ m_hasResetCond = ಸುಳ್ಳು;  // Whether the detected FSM had a reset branch.
    ತಾರ್ಕಿಕ m_inclCond = ಸುಳ್ಳು;  // Whether conditional arcs should be kept explicitly.
    File_ಸಾಲು* m_flp = ಶೂನ್ಯಸೂಚಕ;  // Representative source location for declarations/arcs.
    std::unordered_map<Fsm_State_ಮೌಲ್ಯ, Fsm_State_ಶೃಂಗ*, Fsm_State_ಮೌಲ್ಯ_Hash>
        m_stateVertices;  // Value to state map.
    Fsm_Pseudo_ಶೃಂಗ* m_resetVertexp = ಶೂನ್ಯಸೂಚಕ;  // Synthetic ANY source for reset arcs.
    Fsm_Pseudo_ಶೃಂಗ* m_defaultVertexp = ಶೂನ್ಯಸೂಚಕ;  // Synthetic default source for case defaults.

ಸಾರ್ವಜನಿಕ:
    Fsm_ನಕ್ಷೆ() VL_MT_DISABLED
        : m_resetVertexp{ಹೊಸ Fsm_Pseudo_ಶೃಂಗ{ಇದು, Fsm_ಶೃಂಗ::ಬಗೆ_2::RESET_ANY, "ANY"}},
          m_defaultVertexp{ಹೊಸ Fsm_Pseudo_ಶೃಂಗ{ಇದು, Fsm_ಶೃಂಗ::ಬಗೆ_2::DEFAULT_ANY, "default"}} {}

    AstScope* scopep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_scopep; }
    ಶೂನ್ಯ scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* stateAlwaysp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stateAlwaysp; }
    ಶೂನ್ಯ stateAlwaysp(AstAlways* alwaysp) { m_stateAlwaysp = alwaysp; }
    ಸ್ಥಿರಾಂಕ string& stateVarName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stateVarName; }
    ಶೂನ್ಯ stateVarName(ಸ್ಥಿರಾಂಕ string& name) { m_stateVarName = name; }
    ಸ್ಥಿರಾಂಕ string& state_Var_ಆಂತರಿಕ_Name() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_state_Var_ಆಂತರಿಕ_Name; }
    ಶೂನ್ಯ state_Var_ಆಂತರಿಕ_Name(ಸ್ಥಿರಾಂಕ string& name) { m_state_Var_ಆಂತರಿಕ_Name = name; }
    AstVarScope* stateVarScopep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stateVarScopep; }
    ಶೂನ್ಯ stateVarScopep(AstVarScope* vscp) { m_stateVarScopep = vscp; }
    AstVarScope* sampleVarScopep() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_sampleVarScopep ? m_sampleVarScopep : m_stateVarScopep;
    }
    ಶೂನ್ಯ sampleVarScopep(AstVarScope* vscp) { m_sampleVarScopep = vscp; }
    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<FsmSenDesc>& senses() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_senses; }
    std::ಸದಿಶ<FsmSenDesc>& senses() { ಹಿಂತಿರುಗಿ m_senses; }
    ಸ್ಥಿರಾಂಕ FsmResetCondDesc& resetCond() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_resetCond; }
    FsmResetCondDesc& resetCond() { ಹಿಂತಿರುಗಿ m_resetCond; }
    ತಾರ್ಕಿಕ hasResetCond() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hasResetCond; }
    ಶೂನ್ಯ hasResetCond(ತಾರ್ಕಿಕ flag) { m_hasResetCond = flag; }
    ತಾರ್ಕಿಕ inclCond() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_inclCond; }
    ಶೂನ್ಯ inclCond(ತಾರ್ಕಿಕ flag) { m_inclCond = flag; }
    File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_flp; }
    ಶೂನ್ಯ fileline(File_ಸಾಲು* flp) { m_flp = flp; }

    Fsm_State_ಶೃಂಗ* ಕೂಡಿಸು_State_ಶೃಂಗ(string label, Fsm_State_ಮೌಲ್ಯ ಮೌಲ್ಯ) VL_MT_DISABLED {
        Fsm_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vertexp = ಹೊಸ Fsm_State_ಶೃಂಗ{ಇದು, label, ಮೌಲ್ಯ};
        m_stateVertices.emplace(ಮೌಲ್ಯ, vertexp);
        ಹಿಂತಿರುಗಿ vertexp;
    }
    Fsm_Pseudo_ಶೃಂಗ* reset_Any_ಶೃಂಗ() VL_MT_DISABLED { ಹಿಂತಿರುಗಿ m_resetVertexp; }
    Fsm_Pseudo_ಶೃಂಗ* default_Any_ಶೃಂಗ() VL_MT_DISABLED { ಹಿಂತಿರುಗಿ m_defaultVertexp; }
    Fsm_Arc_ಅಂಚು* ಕೂಡಿಸು_Arc(Fsm_State_ಮೌಲ್ಯ from_ಮೌಲ್ಯ, Fsm_State_ಮೌಲ್ಯ ಗಮ್ಯ_ಮೌಲ್ಯ, ತಾರ್ಕಿಕ isReset, ತಾರ್ಕಿಕ isCond,
                       ತಾರ್ಕಿಕ isDefault, File_ಸಾಲು* flp) VL_MT_DISABLED {
        Fsm_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ತುದಿ = m_stateVertices.at(ಗಮ್ಯ_ಮೌಲ್ಯ);
        Fsm_ಶೃಂಗ* fromp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (isReset) {
            fromp = reset_Any_ಶೃಂಗ();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isDefault) {
            fromp = default_Any_ಶೃಂಗ();
        } ಇಲ್ಲದಿದ್ದರೆ {
            fromp = m_stateVertices.at(from_ಮೌಲ್ಯ);
        }
        ಹಿಂತಿರುಗಿ ಹೊಸ Fsm_Arc_ಅಂಚು{ಇದು, fromp, ತುದಿ, isReset, isCond, isDefault, flp};
    }

    string name() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ "FSM "
               + (m_stateVarName.ಖಾಲಿಗಳು() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dump_ಟ್ಯಾಗ್(ಗಾತ್ರ_t ಸೂಚ್ಯಂಕ) ಸ್ಥಿರಾಂಕ {
        string ಟ್ಯಾಗ್ = state_Var_ಆಂತರಿಕ_Name();
        ಗಾಗಿ (ಅಕ್ಷರ& ch : ಟ್ಯಾಗ್) {
            ಆದರೆ (!std::ಅಕ್ಷರವೇ_ಅಂಕಿಯೇ(static_cast<ಚಿಹ್ನೆರಹಿತ ಅಕ್ಷರ>(ch))) ch = '_';
        }
        ಹಿಂತಿರುಗಿ "fsm_" + cvt_ಗಮ್ಯ_Str(ಸೂಚ್ಯಂಕ) + "_" + ಟ್ಯಾಗ್;
    }
};

ರಚನೆ DetectedFsm final {
    std::unique_ptr<Fsm_ನಕ್ಷೆ> graphp;  // Extracted graph for one detected FSM candidate.
};

ರಚನೆ FsmCaseCandidate final {
    Ast_ಗಂಟು* warnNodep = ಶೂನ್ಯಸೂಚಕ;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = ಶೂನ್ಯಸೂಚಕ;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
ರಚನೆ FsmStateComparison final {
    AstVarScope* stateVscp = ಶೂನ್ಯಸೂಚಕ;  // Compared state variable
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = ಶೂನ್ಯಸೂಚಕ;  // Compared constant value expression
    Fsm_State_ಮೌಲ್ಯ ಮೌಲ್ಯ;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
ರಚನೆ FsmIfBranch final {
    AstIf* ifp = ಶೂನ್ಯಸೂಚಕ;  // Source if/else-if node
    Ast_ಗಂಟು* stmtsp = ಶೂನ್ಯಸೂಚಕ;  // Branch body
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = ಶೂನ್ಯಸೂಚಕ;  // Source state value expression
    Fsm_State_ಮೌಲ್ಯ from_ಮೌಲ್ಯ;  // Encoded source state value
    ತಾರ್ಕಿಕ has_ತುದಿ_Guard = ಸುಳ್ಳು;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
ರಚನೆ FsmIfChainCandidate final {
    AstIf* ifp = ಶೂನ್ಯಸೂಚಕ;  // Top-level if-chain node
    AstVarScope* compareVscp = ಶೂನ್ಯಸೂಚಕ;  // Variable used by every state comparison
    std::ಸದಿಶ<FsmIfBranch> branches;  // Recognized state-dispatch branches
    Ast_ಗಂಟು* defaultStmtsp = ಶೂನ್ಯಸೂಚಕ;  // Optional final else body
};

// Aliases are accepted only when they are equivalent to spelling the state
// comparison inline; this avoids inferring FSM semantics from arbitrary logic.
ಬಳಸಿ FsmAliasMap = std::unordered_map<ಸ್ಥಿರಾಂಕ AstVarScope*, FsmStateComparison>;
ಬಳಸಿ Fsm_Cell_ಪೋರ್ಟ್_Map = std::unordered_map<string, AstVarScope*>;
ಬಳಸಿ Fsm_Cell_ಪೋರ್ಟ್_Alias_Map = std::unordered_map<ಸ್ಥಿರಾಂಕ AstCell*, Fsm_Cell_ಪೋರ್ಟ್_Map>;

ರಚನೆ StateConstLabel final {
    string ಪಠ್ಯ;
    ತಾರ್ಕಿಕ fromParam = ಸುಳ್ಳು;
    ಗಾತ್ರ_t state_ಸೂಚ್ಯಂಕ = 0;
};

ರಚನೆ FsmStateSpace final {
    std::ಸದಿಶ<std::pair<string, Fsm_State_ಮೌಲ್ಯ>> ಸ್ಥಿತಿಗಳು;  // User label and encoded value
    std::unordered_map<Fsm_State_ಮೌಲ್ಯ, StateConstLabel, Fsm_State_ಮೌಲ್ಯ_Hash>
        labels;  // Encoded value to label
    AstVar* stateVarp = ಶೂನ್ಯಸೂಚಕ;  // Tracked FSM state variable
    ತಾರ್ಕಿಕ enumBacked = ಸುಳ್ಳು;  // Whether states came from an enum declaration
};

// Local shared state between the two adjacent FSM coverage phases. Detection
// fills this with recovered FSM graphs; lowering consumes the completed graphs
// immediately afterward without needing any AST serialization bridge.
ವರ್ಗ FsmState final {
    // All detected FSMs in discovery order. This is the only bridge between
    // the adjacent detect and lower phases, so the second phase never needs to
    // rediscover or serialize the extracted machine.
    std::ಸದಿಶ<DetectedFsm> m_fsms;
    std::map<ಸ್ಥಿರಾಂಕ AstVarScope*, ಗಾತ್ರ_t> m_fsm_ಸೂಚ್ಯಂಕ;

ಸಾರ್ವಜನಿಕ:
    DetectedFsm& fsmFor(AstVarScope* stateVscp) {
        ಸ್ಥಿರಾಂಕ std::map<ಸ್ಥಿರಾಂಕ AstVarScope*, ಗಾತ್ರ_t>::const_iterator it = m_fsm_ಸೂಚ್ಯಂಕ.ಕಂಡುಹಿಡಿ(stateVscp);
        ಆದರೆ (it != m_fsm_ಸೂಚ್ಯಂಕ.ಕೊನೆ()) ಹಿಂತಿರುಗಿ m_fsms.at(it->ಎರಡನೇ);
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಸೂಚ್ಯಂಕ = m_fsms.ಗಾತ್ರ_2();
        m_fsm_ಸೂಚ್ಯಂಕ.emplace(stateVscp, ಸೂಚ್ಯಂಕ);
        m_fsms.emplace_ಹಿಂಭಾಗ();
        ಹಿಂತಿರುಗಿ m_fsms.ಹಿಂಭಾಗ();
    }
    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<DetectedFsm>& fsms() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fsms; }
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
    AstScope* m_scopep = ಶೂನ್ಯಸೂಚಕ;
    std::ಸದಿಶ<Fsm_ರಿಜಿಸ್ಟರ್_Candidate> m_ರಿಜಿಸ್ಟರ್_Candidates;
    // Deferring one-block detection avoids making continuous alias support
    // depend on whether the assign appears before or after the always block.
    std::ಸದಿಶ<FsmComboAlways> m_one_ಖಂಡ_Alwayss;
    std::ಸದಿಶ<FsmComboAlways> m_comboAlwayss;
    std::ಸದಿಶ<FsmComboAlways> m_nonComboAlwayss;
    // Wrapper FSM detection has a second path for designs compiled without
    // inlining. In that shape the state register stays behind an AstCell, so we
    // remember candidate cells and resolve them only after the surrounding
    // transition logic and post-link port wiring have both been seen.
    std::ಸದಿಶ<std::pair<AstScope*, AstCell*>> m_wrapperCells;
    std::unordered_map<ಸ್ಥಿರಾಂಕ AstVarScope*, FsmCaseCandidate> m_comboPaired;
    // Continuous aliases are order-independent, while procedural aliases must
    // remain source-order scoped to avoid using assignments not yet executed.
    FsmAliasMap m_stateAliases;
    std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ AstVarScope*> m_ambiguousStateAliases;
    // A surviving wrapper's semantic d/q relationship is split across the
    // parent scope and the child module scope. This table is the narrow bridge
    // between those scopes: only transparent port aliases are recorded, so the
    // detector does not become a general cross-module dataflow engine.
    Fsm_Cell_ಪೋರ್ಟ್_Alias_Map m_cell_ಪೋರ್ಟ್_Aliases;
    Fsm_Cell_ಪೋರ್ಟ್_Alias_Map m_cell_ಪೋರ್ಟ್_ಮಗು_Aliases;

    // METHODS
    // Enum-backed FSMs may be wrapped in refs/typedefs; normalize to the
    // underlying enum type before deciding whether a case is a candidate.
    ಸ್ಥಿರ Ast_ಗಂಟು_DType* unwrapEnumCandidate(Ast_ಗಂಟು_DType* dtypep) {
        ಹಿಂತಿರುಗಿ dtypep->skip_Ref_ಗಮ್ಯ_Enump();
    }

    ಸ್ಥಿರ string candidateConflictContext(Ast_ಗಂಟು* laterNodep,
                                           ಸ್ಥಿರಾಂಕ FsmCaseCandidate& ಮೊದಲ_Cand) {
        ಹಿಂತಿರುಗಿ '\n' + laterNodep->warnContextPrimary() + ಮೊದಲ_Cand.warnNodep->warnOther()
               + "... Location of first supported candidate for "
               + ಮೊದಲ_Cand.stateVscp->prettyNameQ() + '\n'
               + ಮೊದಲ_Cand.warnNodep->warnContextSecondary();
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ rejectFsmWrapperCell(AstCell* cellp, ಸ್ಥಿರಾಂಕ string& reason) {
        cellp->v3warn(COVERIGN, "Ignoring unsupported: " + reason);
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ simple_Param_State_ಮೌಲ್ಯ(AstCell* cellp, ಸ್ಥಿರಾಂಕ string& name, Fsm_State_ಮೌಲ್ಯ& ಮೌಲ್ಯ,
                                      Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtp, Var);
            ಆದರೆ (!varp || !varp->isParam() || varp->name() != name) ಮುಂದುವರಿ;
            valuepr = VN_AS(varp->valuep(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            ಹಿಂತಿರುಗಿ const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ(valuepr, ಮೌಲ್ಯ) == Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ::OK;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ ಮಗು_ಪೋರ್ಟ್_ಒಳ_Scope(AstVarScope* vscp, AstScope* ಪೋಷಕ_Scopep, AstCell*& cellpr) {
        ಆದರೆ (!vscp->varp()->isIO()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        AstScope* ಸ್ಥಿರಾಂಕ scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        ಆದರೆ (scopep->aboveScopep() != ಪೋಷಕ_Scopep) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ಸ್ಥಿರ AstVarScope* simple_ನಿಯೋಜಿಸು_Var_Scope(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        AstVarRef* ಸ್ಥಿರಾಂಕ vrefp = VN_CAST(exprp, VarRef);
        ಹಿಂತಿರುಗಿ vrefp ? vrefp->varScopep() : ಶೂನ್ಯಸೂಚಕ;
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_Wrapper_Cell(AstScope* scopep, AstCell* cellp) {
        m_cell_ಪೋರ್ಟ್_Aliases.emplace(cellp, Fsm_Cell_ಪೋರ್ಟ್_Map{});
        m_cell_ಪೋರ್ಟ್_ಮಗು_Aliases.emplace(cellp, Fsm_Cell_ಪೋರ್ಟ್_Map{});
        ಸ್ಥಿರಾಂಕ std::pair<AstScope*, AstCell*> ಅಂಶ_2{scopep, cellp};
        ಆದರೆ (std::ಕಂಡುಹಿಡಿ(m_wrapperCells.cbegin(), m_wrapperCells.cend(), ಅಂಶ_2)
            != m_wrapperCells.cend()) {
            ಹಿಂತಿರುಗಿ;
        }
        m_wrapperCells.emplace_ಹಿಂಭಾಗ(ಅಂಶ_2);
    }

    ಶೂನ್ಯ collect_Cell_ಪೋರ್ಟ್_Alias(Ast_ನಿಯೋಜಿಸು_W* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* ಸ್ಥಿರಾಂಕ lhsVscp = simple_ನಿಯೋಜಿಸು_Var_Scope(nodep->lhsp());
        AstVarScope* ಸ್ಥಿರಾಂಕ rhsVscp = simple_ನಿಯೋಜಿಸು_Var_Scope(nodep->rhsp());
        ಆದರೆ (!lhsVscp || !rhsVscp) ಹಿಂತಿರುಗಿ;
        AstCell* cellp = ಶೂನ್ಯಸೂಚಕ;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        ಆದರೆ (ಮಗು_ಪೋರ್ಟ್_ಒಳ_Scope(lhsVscp, m_scopep, cellp)) {
            ಆದರೆ (!fsm_ರಿಜಿಸ್ಟರ್_Wrapper_Desc(cellp)) ಹಿಂತಿರುಗಿ;
            UASSERT_OBJ(lhsVscp->varp()->is_ಒಳಹರಿವು(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cell_ಪೋರ್ಟ್_Aliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cell_ಪೋರ್ಟ್_ಮಗು_Aliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            ಕೂಡಿಸು_Wrapper_Cell(m_scopep, cellp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಮಗು_ಪೋರ್ಟ್_ಒಳ_Scope(rhsVscp, m_scopep, cellp)) {
            ಆದರೆ (!fsm_ರಿಜಿಸ್ಟರ್_Wrapper_Desc(cellp)) ಹಿಂತಿರುಗಿ;
            UASSERT_OBJ(rhsVscp->varp()->isWritable(), nodep,
                        "Child-side port alias rhs should be writable");
            m_cell_ಪೋರ್ಟ್_Aliases[cellp][rhsVscp->varp()->name()] = lhsVscp;
            m_cell_ಪೋರ್ಟ್_ಮಗು_Aliases[cellp][rhsVscp->varp()->name()] = rhsVscp;
            ಕೂಡಿಸು_Wrapper_Cell(m_scopep, cellp);
        }
    }

    AstVarScope* roleVarScope(AstCell* cellp, ಸ್ಥಿರಾಂಕ string& ಪೋರ್ಟ್_Name) ಸ್ಥಿರಾಂಕ {
        // At this point explicit AstPin expressions have been lowered away, so
        // role resolution crosses the wrapper boundary only through the
        // transparent alias table above. This keeps wrapper support aligned with
        // direct-register detection instead of growing into interprocedural FSM
        // inference.
        ಸ್ಥಿರಾಂಕ Fsm_Cell_ಪೋರ್ಟ್_Map& ports = m_cell_ಪೋರ್ಟ್_Aliases.at(cellp);
        ಸ್ಥಿರಾಂಕ Fsm_Cell_ಪೋರ್ಟ್_Map::const_iterator ಪೋರ್ಟ್_It = ports.ಕಂಡುಹಿಡಿ(ಪೋರ್ಟ್_Name);
        ಹಿಂತಿರುಗಿ ಪೋರ್ಟ್_It == ports.ಕೊನೆ() ? ಶೂನ್ಯಸೂಚಕ : ಪೋರ್ಟ್_It->ಎರಡನೇ;
    }

    AstVarScope* ಮಗು_Role_Var_Scope(AstCell* cellp, ಸ್ಥಿರಾಂಕ string& ಪೋರ್ಟ್_Name) ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ Fsm_Cell_ಪೋರ್ಟ್_Map& ports = m_cell_ಪೋರ್ಟ್_ಮಗು_Aliases.at(cellp);
        ಸ್ಥಿರಾಂಕ Fsm_Cell_ಪೋರ್ಟ್_Map::const_iterator ಪೋರ್ಟ್_It = ports.ಕಂಡುಹಿಡಿ(ಪೋರ್ಟ್_Name);
        ಹಿಂತಿರುಗಿ ಪೋರ್ಟ್_It == ports.ಕೊನೆ() ? ಶೂನ್ಯಸೂಚಕ : ಪೋರ್ಟ್_It->ಎರಡನೇ;
    }

    ವರ್ಗ ರಿಜಿಸ್ಟರ್_Always_Analyzer final {
        AstScope* ಸ್ಥಿರಾಂಕ m_scopep;  // Scope containing the always block(s) being analyzed

    ಸಾರ್ವಜನಿಕ:
        ಸ್ಪಷ್ಟ ರಿಜಿಸ್ಟರ್_Always_Analyzer(AstScope* scopep)
            : m_scopep{scopep} {}

        std::ಸದಿಶ<std::pair<AstCase*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>>
        one_ಖಂಡ_Candidates(AstAlways* alwaysp) ಸ್ಥಿರಾಂಕ {
            std::ಸದಿಶ<std::pair<AstCase*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>> candidates;
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = alwaysp->stmtsp();
            ಆದರೆ (AstIf* ಸ್ಥಿರಾಂಕ ಮೊದಲ_Ifp = VN_CAST(stmtsp, If)) {
                ಆದರೆ (AstCase* ಸ್ಥಿರಾಂಕ casep = VN_CAST(ಮೊದಲ_Ifp->elsesp(), Case)) {
                    candidates.emplace_ಹಿಂಭಾಗ(casep,
                                            FsmDetectVisitor::isSimpleResetCond(ಮೊದಲ_Ifp->condp())
                                                ? ಮೊದಲ_Ifp->condp()
                                                : ಶೂನ್ಯಸೂಚಕ);
                }
            }
            ಗಾಗಿ (Ast_ಗಂಟು* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                ಆದರೆ (AstCase* ಸ್ಥಿರಾಂಕ casep = VN_CAST(nodep, Case))
                    candidates.emplace_ಹಿಂಭಾಗ(casep, ಶೂನ್ಯಸೂಚಕ);
            }
            ಹಿಂತಿರುಗಿ candidates;
        }

        std::ಸದಿಶ<std::pair<AstIf*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>>
        one_ಖಂಡ_If_Candidates(AstAlways* alwaysp) ಸ್ಥಿರಾಂಕ {
            std::ಸದಿಶ<std::pair<AstIf*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>> candidates;
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            ಆದರೆ (AstIf* ಸ್ಥಿರಾಂಕ ಮೊದಲ_Ifp = VN_CAST(stmtsp, If)) {
                ಆದರೆ (AstIf* ಸ್ಥಿರಾಂಕ chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(ಮೊದಲ_Ifp->elsesp()), If)) {
                    candidates.emplace_ಹಿಂಭಾಗ(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(ಮೊದಲ_Ifp->condp())
                                                ? ಮೊದಲ_Ifp->condp()
                                                : ಶೂನ್ಯಸೂಚಕ);
                }
            }
            ಗಾಗಿ (Ast_ಗಂಟು* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                ಆದರೆ (AstIf* ಸ್ಥಿರಾಂಕ ifp = VN_CAST(nodep, If)) candidates.emplace_ಹಿಂಭಾಗ(ifp, ಶೂನ್ಯಸೂಚಕ);
            }
            ಹಿಂತಿರುಗಿ candidates;
        }

        ತಾರ್ಕಿಕ ಹೊಂದಿಕೆ_ರಿಜಿಸ್ಟರ್_Candidate(AstAlways* alwaysp, Fsm_ರಿಜಿಸ್ಟರ್_Candidate& cand) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ FsmDetectVisitor::ಹೊಂದಿಕೆ_ರಿಜಿಸ್ಟರ್_Always(alwaysp, m_scopep, cand);
        }

        ಶೂನ್ಯ build_One_ಖಂಡ_Candidate(AstAlways* alwaysp, AstVarScope* vscp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resetCondp,
                                    Fsm_ರಿಜಿಸ್ಟರ್_Candidate& reg) ಸ್ಥಿರಾಂಕ {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.ಮುಂದಿನ_Vscp(vscp);
            reg.senses() = FsmDetectVisitor::describe_Sen_ವೃಕ್ಷ(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != ಶೂನ್ಯಸೂಚಕ);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* ಸ್ಥಿರಾಂಕ ಮೊದಲ_Ifp = VN_CAST(alwaysp->stmtsp(), If);
            ಆದರೆ (ಮೊದಲ_Ifp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = ಶೂನ್ಯಸೂಚಕ;
                ಸ್ಥಿರಾಂಕ Reset_ನಿಯೋಜಿಸು_ಸ್ಥಿತಿ reset_ಸ್ಥಿತಿ = FsmDetectVisitor::collectConstStateAssigns(
                    ಮೊದಲ_Ifp->thensp(), resetStateVscp, reg.resetArcs());
                ಆದರೆ (reset_ಸ್ಥಿತಿ == Reset_ನಿಯೋಜಿಸು_ಸ್ಥಿತಿ::ಖಾಲಿಗಳು_3) {
                    reg.resetArcs().clear();
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (reset_ಸ್ಥಿತಿ == Reset_ನಿಯೋಜಿಸು_ಸ್ಥಿತಿ::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().clear();
                    Fsm_State_ಮೌಲ್ಯ reset_ಮೌಲ್ಯ;
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(ಮೊದಲ_Ifp->thensp());
                    UASSERT_OBJ(thenNodep, ಮೊದಲ_Ifp,
                                "one-block reset fallback requires a non-empty reset branch");
                    Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ resetAssp = FsmDetectVisitor::ನೇರ_Const_State_ನಿಯೋಜಿಸು_ಗಂಟು(
                        thenNodep, resetStateVscp, reset_ಮೌಲ್ಯ);
                    ಆದರೆ (resetAssp && resetStateVscp == vscp) {
                        reg.resetArcs().emplace_ಹಿಂಭಾಗ(reset_ಮೌಲ್ಯ, resetAssp);
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (reset_ಸ್ಥಿತಿ == Reset_ನಿಯೋಜಿಸು_ಸ್ಥಿತಿ::MULTI_SAME_STATE) {
                    reg.resetArcs().clear();
                }
            }
        }
    };

    ತಾರ್ಕಿಕ ಹೊಂದಿಕೆ_Fsm_Wrapper_Cell(AstScope* scopep, AstCell* cellp, Fsm_ರಿಜಿಸ್ಟರ್_Candidate& cand) ಸ್ಥಿರಾಂಕ {
        FsmWrapperRoles roles = rolesFromDesc(*fsm_ರಿಜಿಸ್ಟರ್_Wrapper_Desc(cellp));

        AstVarScope* ಸ್ಥಿರಾಂಕ ಮುಂದಿನ_Vscp = roleVarScope(cellp, roles.d_ಪೋರ್ಟ್);
        AstVarScope* ಸ್ಥಿರಾಂಕ stateVscp = roleVarScope(cellp, roles.q_ಪೋರ್ಟ್);
        ಆದರೆ (!ಮುಂದಿನ_Vscp || !stateVscp) {
            ಹಿಂತಿರುಗಿ rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* ಸ್ಥಿರಾಂಕ clkVscp = roleVarScope(cellp, roles.clk_ಪೋರ್ಟ್);
        ಆದರೆ (!clkVscp) {
            ಹಿಂತಿರುಗಿ rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper instance requires a simple clock connection");
        }

        FsmSenDesc clkSense;
        clkSense.ಅಂಚು_Type = VEdgeType::ET_POSEDGE;
        clkSense.varScopep = clkVscp;
        cand.senses().ತಳ್ಳು_ಹಿಂಭಾಗ(clkSense);

        AstVarScope* resetVscp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!roles.rst_ಪೋರ್ಟ್.ಖಾಲಿಗಳು()) resetVscp = roleVarScope(cellp, roles.rst_ಪೋರ್ಟ್);
        ಆದರೆ (resetVscp) {
            // The descriptor identifies the reset port but not its polarity. Use
            // the wrapper's own event control AST as the contract for sampling
            // the connected parent signal.
            ತಾರ್ಕಿಕ inferred_ಸಕ್ರಿಯ_ಕೆಳಗಿನ = ಸುಳ್ಳು;
            ಆದರೆ (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rst_ಪೋರ್ಟ್, inferred_ಸಕ್ರಿಯ_ಕೆಳಗಿನ)) {
                roles.has_Rst_ಸಕ್ರಿಯ_ಕೆಳಗಿನ = ನಿಜ;
                roles.rst_ಸಕ್ರಿಯ_ಕೆಳಗಿನ = inferred_ಸಕ್ರಿಯ_ಕೆಳಗಿನ;
            }
        }

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resetValuep = ಶೂನ್ಯಸೂಚಕ;
        Fsm_State_ಮೌಲ್ಯ reset_ಮೌಲ್ಯ;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ has_Reset_ಮೌಲ್ಯ
            = !roles.rstValParam.ಖಾಲಿಗಳು()
              && simple_Param_State_ಮೌಲ್ಯ(cellp, roles.rstValParam, reset_ಮೌಲ್ಯ, resetValuep);
        ಆದರೆ (resetVscp && roles.has_Rst_ಸಕ್ರಿಯ_ಕೆಳಗಿನ && has_Reset_ಮೌಲ್ಯ) {
            FsmSenDesc rstSense;
            rstSense.ಅಂಚು_Type = roles.rst_ಸಕ್ರಿಯ_ಕೆಳಗಿನ ? VEdgeType::ET_NEGEDGE : VEdgeType::ET_POSEDGE;
            rstSense.varScopep = resetVscp;
            cand.senses().ತಳ್ಳು_ಹಿಂಭಾಗ(rstSense);
            cand.resetCond().varScopep = resetVscp;
            cand.resetCond().ಸಕ್ರಿಯ_ಕೆಳಗಿನ = roles.rst_ಸಕ್ರಿಯ_ಕೆಳಗಿನ;
            cand.hasResetCond(ನಿಜ);
            cand.resetArcs().emplace_ಹಿಂಭಾಗ(reset_ಮೌಲ್ಯ, cellp, resetValuep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!roles.rst_ಪೋರ್ಟ್.ಖಾಲಿಗಳು() || !roles.rstValParam.ಖಾಲಿಗಳು()) {
            string reason;
            ಆದರೆ (roles.rst_ಪೋರ್ಟ್.ಖಾಲಿಗಳು()) {
                reason = "reset port is not configured";
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!resetVscp) {
                reason = "reset connection is missing or not a simple variable";
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!roles.has_Rst_ಸಕ್ರಿಯ_ಕೆಳಗಿನ) {
                reason = "reset polarity could not be inferred from the wrapper";
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (roles.rstValParam.ಖಾಲಿಗಳು()) {
                reason = "reset_value parameter is not configured";
            } ಇಲ್ಲದಿದ್ದರೆ {
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
        cand.alwaysp(ಶೂನ್ಯಸೂಚಕ);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(ಮಗು_Role_Var_Scope(cellp, roles.q_ಪೋರ್ಟ್));
        cand.ಮುಂದಿನ_Vscp(ಮುಂದಿನ_Vscp);
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(cellp->fileline());
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ವರ್ಗ ComboAlwaysAnalyzer final {
    ಸಾರ್ವಜನಿಕ:
        ರಚನೆ Combo_ಹೊಂದಿಕೆ final {
            ಸ್ಥಿರಾಂಕ Fsm_ರಿಜಿಸ್ಟರ್_Candidate* matchedp = ಶೂನ್ಯಸೂಚಕ;
            Ast_ಗಂಟು* warnNodep = ಶೂನ್ಯಸೂಚಕ;
        };

    ಖಾಸಗಿ:
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Fsm_ರಿಜಿಸ್ಟರ್_Candidate>& m_ರಿಜಿಸ್ಟರ್_Candidates;

    ಸಾರ್ವಜನಿಕ:
        ಸ್ಪಷ್ಟ ComboAlwaysAnalyzer(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Fsm_ರಿಜಿಸ್ಟರ್_Candidate>& ರಿಜಿಸ್ಟರ್_Candidates)
            : m_ರಿಜಿಸ್ಟರ್_Candidates{ರಿಜಿಸ್ಟರ್_Candidates} {}

        Combo_ಹೊಂದಿಕೆ ಹೊಂದಿಕೆ_Case(Ast_ಗಂಟು* stmtsp, AstCase* casep) ಸ್ಥಿರಾಂಕ {
            Combo_ಹೊಂದಿಕೆ ಹೊಂದಿಕೆ;
            AstVarRef* ಸ್ಥಿರಾಂಕ selp = VN_CAST(casep->exprp(), VarRef);
            ಆದರೆ (!selp) ಹಿಂತಿರುಗಿ ಹೊಂದಿಕೆ;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Fsm_ರಿಜಿಸ್ಟರ್_Candidate& reg : m_ರಿಜಿಸ್ಟರ್_Candidates) {
                ಆದರೆ (selp->varScopep() == reg.ಮುಂದಿನ_Vscp()) {
                    ಆದರೆ (!FsmDetectVisitor::has_Canonical_ಮುಂದಿನ_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.ಮುಂದಿನ_Vscp())) {
                        ಮುಂದುವರಿ;
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (selp->varScopep() != reg.stateVscp()) {
                    ಮುಂದುವರಿ;
                }
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ warnNodep = FsmDetectVisitor::case_Supported_Transition_ಗಂಟು(
                    casep, reg.ಮುಂದಿನ_Vscp(), reg.inclCond());
                ಆದರೆ (!warnNodep) ಮುಂದುವರಿ;
                ಹೊಂದಿಕೆ.matchedp = &reg;
                ಹೊಂದಿಕೆ.warnNodep = warnNodep;
            }
            ಹಿಂತಿರುಗಿ ಹೊಂದಿಕೆ;
        }

        Combo_ಹೊಂದಿಕೆ ಹೊಂದಿಕೆ_If_Chain(Ast_ಗಂಟು* stmtsp, ಸ್ಥಿರಾಂಕ FsmIfChainCandidate& chain) ಸ್ಥಿರಾಂಕ {
            Combo_ಹೊಂದಿಕೆ ಹೊಂದಿಕೆ;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Fsm_ರಿಜಿಸ್ಟರ್_Candidate& reg : m_ರಿಜಿಸ್ಟರ್_Candidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                ಆದರೆ (chain.compareVscp == reg.ಮುಂದಿನ_Vscp()) {
                    ಆದರೆ (!FsmDetectVisitor::has_Canonical_ಮುಂದಿನ_State_Default_Before_Case(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.ಮುಂದಿನ_Vscp())) {
                        ಮುಂದುವರಿ;
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (chain.compareVscp != reg.stateVscp()) {
                    ಮುಂದುವರಿ;
                }
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ warnNodep
                    = FsmDetectVisitor::if_Chain_Supported_Transition_ಗಂಟು(chain, reg.ಮುಂದಿನ_Vscp());
                ಆದರೆ (!warnNodep) ಮುಂದುವರಿ;
                ಹೊಂದಿಕೆ.matchedp = &reg;
                ಹೊಂದಿಕೆ.warnNodep = warnNodep;
            }
            ಹಿಂತಿರುಗಿ ಹೊಂದಿಕೆ;
        }

        ತಾರ್ಕಿಕ shouldWarnUnsupported(Ast_ಗಂಟು* stmtsp, AstCase* casep) ಸ್ಥಿರಾಂಕ {
            ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ selp = VN_CAST(casep->exprp(), VarRef);
            ಆದರೆ (!selp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

            ಹಿಂತಿರುಗಿ std::any_ದ(
                m_ರಿಜಿಸ್ಟರ್_Candidates.cbegin(), m_ರಿಜಿಸ್ಟರ್_Candidates.cend(),
                [&](ಸ್ಥಿರಾಂಕ Fsm_ರಿಜಿಸ್ಟರ್_Candidate& reg) -> ತಾರ್ಕಿಕ {
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ matches_ಮುಂದಿನ = selp->varScopep() == reg.ಮುಂದಿನ_Vscp();
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ matchesState = selp->varScopep() == reg.stateVscp();

                    ಆದರೆ (!matches_ಮುಂದಿನ && !matchesState) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                    ಆದರೆ (matches_ಮುಂದಿನ
                        && !FsmDetectVisitor::has_Canonical_ಮುಂದಿನ_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.ಮುಂದಿನ_Vscp())) {
                        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                    }
                    ಹಿಂತಿರುಗಿ FsmDetectVisitor::case_Supported_Transition_ಗಂಟು(casep, reg.ಮುಂದಿನ_Vscp(),
                                                                         reg.inclCond());
                });
        }
    };

    // Reset arcs are only modeled for the simple signal form that survives to
    // this pass after earlier normalization.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isSimpleResetCond(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) { ಹಿಂತಿರುಗಿ VN_IS(condp, VarRef); }

    // Normalize the reset condition into a compact description so the lowering
    // phase can regenerate the same predicate after detection. By the time
    // this pass runs, active-low source forms such as "!rst_n" have already
    // been canonicalized to a positive-condition if/else shape, so only a
    // plain VarRef survives here.
    ಸ್ಥಿರ FsmResetCondDesc describeResetCond(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        FsmResetCondDesc desc;
        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        ಹಿಂತಿರುಗಿ desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    ಸ್ಥಿರ std::ಸದಿಶ<FsmSenDesc> describe_Sen_ವೃಕ್ಷ(Ast_Sen_ವೃಕ್ಷ* sentreep) {
        std::ಸದಿಶ<FsmSenDesc> senses;
        ಗಾಗಿ (Ast_Sen_ಅಂಶ* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), Sen_ಅಂಶ)) {
            Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ vrefp = itemp->varrefp();
            ಆದರೆ (!vrefp) ಮುಂದುವರಿ;
            FsmSenDesc desc;
            desc.ಅಂಚು_Type = itemp->ಅಂಚು_Type().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.ತಳ್ಳು_ಹಿಂಭಾಗ(desc);
        }
        ಹಿಂತಿರುಗಿ senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isIgnorableStmt(Ast_ಗಂಟು* nodep) { ಹಿಂತಿರುಗಿ VN_IS(nodep, CoverInc); }

    ಸ್ಥಿರ Ast_ಗಂಟು* skipLeadingIgnorableStmt(Ast_ಗಂಟು* nodep) {
        ಆಗ (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        ಹಿಂತಿರುಗಿ nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    ಸ್ಥಿರ Ast_ಗಂಟು* singleMeaningfulStmt(Ast_ಗಂಟು* stmtp) {
        Ast_ಗಂಟು* resultp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            ಆದರೆ (isIgnorableStmt(nodep)) ಮುಂದುವರಿ;
            ಆದರೆ (resultp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            resultp = nodep;
        }
        ಹಿಂತಿರುಗಿ resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    ಸ್ಥಿರ Ast_ಗಂಟು* singleMeaningfulBranch(Ast_ಗಂಟು* branchp) {
        ಆದರೆ (!branchp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಹಿಂತಿರುಗಿ branchp;
    }

    // By fsm-detect time, non-clocked always @* blocks are already admitted through
    // a missing sentree. This helper therefore only needs to recognize
    // explicit changed-sensitivity lists such as always @(a or b); clocked and
    // event-driven forms remain out of scope.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isPlainComboSentree(ಸ್ಥಿರಾಂಕ Ast_Sen_ವೃಕ್ಷ* sentreep) {
        UASSERT(sentreep, "plain combo sensitivity check requires a sensitivity tree");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ* senp = sentreep->sensesp(); senp;
             senp = VN_AS(senp->nextp(), Sen_ಅಂಶ)) {
            ಆದರೆ (senp->ಅಂಚು_Type() == VEdgeType::ET_ಬದಲಾಯಿತು) ಮುಂದುವರಿ;
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ಶೂನ್ಯ warnUnsupportedComboAlways(ಸ್ಥಿರಾಂಕ FsmComboAlways& combo) {
        ಸ್ಥಿರಾಂಕ ComboAlwaysAnalyzer analyzer{m_ರಿಜಿಸ್ಟರ್_Candidates};
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        ತಾರ್ಕಿಕ warned = ಸುಳ್ಳು;
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ಸ್ಥಿರಾಂಕ casep = VN_CAST(nodep, Case);
            ಆದರೆ (!casep) ಮುಂದುವರಿ;
            ಆದರೆ (analyzer.shouldWarnUnsupported(stmtsp, casep)) {
                casep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-clocked always "
                                        "blocks requires a combinational sensitivity list or "
                                        "always_comb");
                warned = ನಿಜ;
            }
            ಆದರೆ (warned) ನಿಲ್ಲಿಸು;
        }
    }

    // Case-item bodies are single subtrees like if/else arms, not statement
    // lists, so unwrap only local begin/end wrappers here rather than walking
    // sibling case items via nextp().
    ಸ್ಥಿರ Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ನೇರ_State_ನಿಯೋಜಿಸು(Ast_ಗಂಟು* stmtp, AstVarScope* stateVscp) {
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep = singleMeaningfulBranch(stmtp);
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = VN_CAST(nodep, ಗಂಟು_ನಿಯೋಜಿಸು);
        ಆದರೆ (!assp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        AstVarRef* ಸ್ಥಿರಾಂಕ vrefp = VN_CAST(assp->lhsp(), VarRef);
        ಆದರೆ (!vrefp || vrefp->varScopep() != stateVscp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಹಿಂತಿರುಗಿ assp;
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಗಂಟು_State_Var_ನಿಯೋಜಿಸು(Ast_ಗಂಟು* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = VN_CAST(nodep, ಗಂಟು_ನಿಯೋಜಿಸು);
        ಆದರೆ (!assp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        AstVarRef* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(assp->lhsp()->ತಳ_Fromp(ನಿಜ), VarRef);
        AstVarRef* ಸ್ಥಿರಾಂಕ rhsp = VN_CAST(assp->rhsp(), VarRef);
        ಆದರೆ (!rhsp || !lhsp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        ಹಿಂತಿರುಗಿ assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    ಸ್ಥಿರ Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ನೇರ_Cond_State_Var_ನಿಯೋಜಿಸು(Ast_ಗಂಟು* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*& condp,
                                                   Fsm_State_ಮೌಲ್ಯ& reset_ಮೌಲ್ಯ) {
        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = VN_CAST(nodep, ಗಂಟು_ನಿಯೋಜಿಸು);
        ಆದರೆ (!assp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        AstVarRef* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(assp->lhsp()->ತಳ_Fromp(ನಿಜ), VarRef);
        AstCond* ಸ್ಥಿರಾಂಕ rhsp = VN_CAST(assp->rhsp(), Cond);
        ಆದರೆ (!rhsp || !lhsp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        AstVarRef* ಸ್ಥಿರಾಂಕ elsep = VN_CAST(rhsp->elsep(), VarRef);
        ಆದರೆ (!elsep) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ(rhsp->thenp(), reset_ಮೌಲ್ಯ) != Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ::OK) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        ಹಿಂತಿರುಗಿ assp;
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ನೇರ_Const_State_ನಿಯೋಜಿಸು_ಗಂಟು(Ast_ಗಂಟು* nodep, AstVarScope*& stateVscp,
                                                     Fsm_State_ಮೌಲ್ಯ& ಮೌಲ್ಯ) {
        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = VN_CAST(nodep, ಗಂಟು_ನಿಯೋಜಿಸು);
        ಆದರೆ (!assp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        AstVarRef* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(assp->lhsp()->ತಳ_Fromp(ನಿಜ), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        ಆದರೆ (const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ(assp->rhsp(), ಮೌಲ್ಯ) != Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ::OK) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        stateVscp = lhsp->varScopep();
        ಹಿಂತಿರುಗಿ assp;
    }

    ಎಣಿಕೆ ವರ್ಗ Reset_ನಿಯೋಜಿಸು_ಸ್ಥಿತಿ : uint8_t {
        ಖಾಲಿಗಳು_3,  // Reset branch had no non-coverage statements.
        NONE,  // Reset branch was not the supported direct-constant shape.
        SINGLE,  // Exactly one supported reset assignment was collected.
        MULTI_SAME_STATE  // Multiple assignments to the same FSM state var; warn and ignore.
    };

    // Reset arcs are only extracted from the single direct-constant form. If
    // user RTL assigns the same state register multiple times in the reset
    // branch, warn and skip reset-arc modeling rather than inventing multiple
    // reset transitions for an odd but legal coding style.
    ಸ್ಥಿರ Reset_ನಿಯೋಜಿಸು_ಸ್ಥಿತಿ collectConstStateAssigns(Ast_ಗಂಟು* stmtp, AstVarScope*& stateVscp,
                                                      std::ಸದಿಶ<FsmResetArcDesc>& resetArcs) {
        Ast_ಗಂಟು* nodep = skipLeadingIgnorableStmt(stmtp);
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ Reset_ನಿಯೋಜಿಸು_ಸ್ಥಿತಿ::ಖಾಲಿಗಳು_3;
        ಗಾಗಿ (;; nodep = nodep->nextp()) {
            AstVarScope* ನಿಯೋಜಿಸು_State_Vscp = ಶೂನ್ಯಸೂಚಕ;
            Fsm_State_ಮೌಲ್ಯ ಮೌಲ್ಯ;
            Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = ನೇರ_Const_State_ನಿಯೋಜಿಸು_ಗಂಟು(nodep, ನಿಯೋಜಿಸು_State_Vscp, ಮೌಲ್ಯ);
            ಆದರೆ (!assp) ಹಿಂತಿರುಗಿ Reset_ನಿಯೋಜಿಸು_ಸ್ಥಿತಿ::NONE;
            ಆದರೆ (!stateVscp) stateVscp = ನಿಯೋಜಿಸು_State_Vscp;
            ಆದರೆ (ನಿಯೋಜಿಸು_State_Vscp != stateVscp) ಹಿಂತಿರುಗಿ Reset_ನಿಯೋಜಿಸು_ಸ್ಥಿತಿ::NONE;
            ಆದರೆ (!resetArcs.ಖಾಲಿಗಳು()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.clear();
                ಹಿಂತಿರುಗಿ Reset_ನಿಯೋಜಿಸು_ಸ್ಥಿತಿ::MULTI_SAME_STATE;
            }
            resetArcs.emplace_ಹಿಂಭಾಗ(ಮೌಲ್ಯ, assp);
            ಆದರೆ (!nodep->nextp()) ಹಿಂತಿರುಗಿ Reset_ನಿಯೋಜಿಸು_ಸ್ಥಿತಿ::SINGLE;
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ has_Canonical_ಮುಂದಿನ_State_Default_Before_Case(Ast_ಗಂಟು* stmtsp, Ast_ಗಂಟು* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* ಮುಂದಿನ_Vscp) {
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ bodyp = skipLeadingIgnorableStmt(stmtsp);
        ತಾರ್ಕಿಕ sawCanonicalDefault = ಸುಳ್ಳು;
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            ಆದರೆ (nodep == targetp) ಹಿಂತಿರುಗಿ sawCanonicalDefault;
            ಆದರೆ (Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = VN_CAST(nodep, ಗಂಟು_ನಿಯೋಜಿಸು)) {
                AstVarRef* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* ಸ್ಥಿರಾಂಕ rhsp = VN_CAST(assp->rhsp(), VarRef);
                ಆದರೆ (!lhsp || lhsp->varScopep() != ಮುಂದಿನ_Vscp) ಮುಂದುವರಿ;
                ಆದರೆ (sawCanonicalDefault) {
                    ಸ್ಥಿರಾಂಕ string ಮುಂದಿನ_Name = ಮುಂದಿನ_Vscp->varp()->prettyNameQ();
                    ಸ್ಥಿರಾಂಕ string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + ಮುಂದಿನ_Name
                                     + ") when the canonical " + ಮುಂದಿನ_Name + " = " + stateName
                                     + " default is overwritten before the case statement");
                    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                }
                ಆದರೆ (!rhsp || rhsp->varScopep() != stateVscp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                sawCanonicalDefault = ನಿಜ;
            }
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ if_State_Const_ನಿಯೋಜಿಸು(Ast_ಗಂಟು* stmtp, AstVarScope* stateVscp,
                                   Fsm_State_ಮೌಲ್ಯ& then_ಮೌಲ್ಯ, Fsm_State_ಮೌಲ್ಯ& else_ಮೌಲ್ಯ) {
        AstIf* ಸ್ಥಿರಾಂಕ ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        ಆದರೆ (!ifp || !ifp->elsesp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        AstVarScope* thenVscp = ಶೂನ್ಯಸೂಚಕ;
        AstVarScope* elseVscp = ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        ಆದರೆ (!thenNodep || !elseNodep) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!ನೇರ_Const_State_ನಿಯೋಜಿಸು_ಗಂಟು(thenNodep, thenVscp, then_ಮೌಲ್ಯ)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!ನೇರ_Const_State_ನಿಯೋಜಿಸು_ಗಂಟು(elseNodep, elseVscp, else_ಮೌಲ್ಯ)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (thenVscp == stateVscp && elseVscp == stateVscp) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (thenVscp != elseVscp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = ಶೂನ್ಯಸೂಚಕ;
        AstVarScope* finalFromVscp = ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ finalNodep = singleMeaningfulBranch(followp);
        ಆದರೆ (!finalNodep) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!ಗಂಟು_State_Var_ನಿಯೋಜಿಸು(finalNodep, finalStateVscp, finalFromVscp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (finalStateVscp != stateVscp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (finalFromVscp != thenVscp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ ನೇರ_State_Cond_Const_ನಿಯೋಜಿಸು(Ast_ಗಂಟು* stmtp, AstVarScope* stateVscp,
                                           Fsm_State_ಮೌಲ್ಯ& then_ಮೌಲ್ಯ, Fsm_State_ಮೌಲ್ಯ& else_ಮೌಲ್ಯ) {
        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = ನೇರ_State_ನಿಯೋಜಿಸು(stmtp, stateVscp);
        ಆದರೆ (!assp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        AstCond* ಸ್ಥಿರಾಂಕ condp = VN_CAST(assp->rhsp(), Cond);
        ಆದರೆ (!condp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ(condp->thenp(), then_ಮೌಲ್ಯ) == Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ::OK
               && const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ(condp->elsep(), else_ಮೌಲ್ಯ) == Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ::OK;
    }

    ಸ್ಥಿರ Ast_ಗಂಟು* case_ಅಂಶ_Supported_Arc_ಗಂಟು(Ast_Case_ಅಂಶ* itemp, AstVarScope* stateVscp,
                                             ತಾರ್ಕಿಕ inclCond) {
        ಆದರೆ (itemp->isDefault()) {
            ಆದರೆ (!inclCond) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = ನೇರ_State_ನಿಯೋಜಿಸು(itemp->stmtsp(), stateVscp);
        ಆದರೆ (assp) {
            Fsm_State_ಮೌಲ್ಯ ಗಮ್ಯ_ಮೌಲ್ಯ;
            ಆದರೆ (const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ(assp->rhsp(), ಗಮ್ಯ_ಮೌಲ್ಯ) == Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ::OK) ಹಿಂತಿರುಗಿ assp;
        }
        Fsm_State_ಮೌಲ್ಯ then_ಮೌಲ್ಯ;
        Fsm_State_ಮೌಲ್ಯ else_ಮೌಲ್ಯ;
        ಆದರೆ (ನೇರ_State_Cond_Const_ನಿಯೋಜಿಸು(itemp->stmtsp(), stateVscp, then_ಮೌಲ್ಯ, else_ಮೌಲ್ಯ)) {
            ಹಿಂತಿರುಗಿ assp;
        }
        ಆದರೆ (if_State_Const_ನಿಯೋಜಿಸು(itemp->stmtsp(), stateVscp, then_ಮೌಲ್ಯ, else_ಮೌಲ್ಯ)) {
            ಹಿಂತಿರುಗಿ singleMeaningfulBranch(itemp->stmtsp());
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    ಸ್ಥಿರ Ast_ಗಂಟು* case_Supported_Transition_ಗಂಟು(AstCase* casep, AstVarScope* stateVscp,
                                                ತಾರ್ಕಿಕ inclCond) {
        ಗಾಗಿ (Ast_Case_ಅಂಶ* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_ಅಂಶ)) {
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep = case_ಅಂಶ_Supported_Arc_ಗಂಟು(itemp, stateVscp, inclCond))
                ಹಿಂತಿರುಗಿ nodep;
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }

    ಸ್ಥಿರ Ast_ಗಂಟು* case_ಅಂಶ_Supported_Arc_ಗಂಟು_Like(Ast_ಗಂಟು* stmtsp, AstVarScope* stateVscp) {
        ಆದರೆ (Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = ನೇರ_State_ನಿಯೋಜಿಸು(stmtsp, stateVscp)) {
            Fsm_State_ಮೌಲ್ಯ ಗಮ್ಯ_ಮೌಲ್ಯ;
            ಆದರೆ (const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ(assp->rhsp(), ಗಮ್ಯ_ಮೌಲ್ಯ) == Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ::OK) ಹಿಂತಿರುಗಿ assp;
            Fsm_State_ಮೌಲ್ಯ then_ಮೌಲ್ಯ;
            Fsm_State_ಮೌಲ್ಯ else_ಮೌಲ್ಯ;
            ಆದರೆ (ನೇರ_State_Cond_Const_ನಿಯೋಜಿಸು(stmtsp, stateVscp, then_ಮೌಲ್ಯ, else_ಮೌಲ್ಯ)) ಹಿಂತಿರುಗಿ assp;
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }

    ಸ್ಥಿರ Ast_ಗಂಟು* if_Chain_Supported_Transition_ಗಂಟು(ಸ್ಥಿರಾಂಕ FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < chain.branches.ಗಾತ್ರ_2(); ++i) {
            ಸ್ಥಿರಾಂಕ FsmIfBranch& branch = chain.branches[i];
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep = case_ಅಂಶ_Supported_Arc_ಗಂಟು_Like(branch.stmtsp, stateVscp);
            ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        ಹಿಂತಿರುಗಿ chain.branches.front().ifp;
    }

    // Prefer user labels in reports. Forced non-enum FSMs prepopulate synthetic
    // labels, so all emitted arcs should already have a known label here.
    ಸ್ಥಿರ string label_For_ಮೌಲ್ಯ(
        ಸ್ಥಿರಾಂಕ std::unordered_map<Fsm_State_ಮೌಲ್ಯ, StateConstLabel, Fsm_State_ಮೌಲ್ಯ_Hash>& labels,
        ಸ್ಥಿರಾಂಕ Fsm_State_ಮೌಲ್ಯ& ಮೌಲ್ಯ) {
        ಹಿಂತಿರುಗಿ labels.at(ಮೌಲ್ಯ).ಪಠ್ಯ;
    }

    // The extractor only models constant-valued state transitions, and by the
    // time detect runs those values have already been constant-folded.
    ಎಣಿಕೆ ವರ್ಗ Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ : uint8_t { OK, NOT_CONST, XZ };

    ಸ್ಥಿರ Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Fsm_State_ಮೌಲ್ಯ& ಮೌಲ್ಯ) {
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(exprp, Const);
        ಆದರೆ (!constp) ಹಿಂತಿರುಗಿ Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ::NOT_CONST;
        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = constp->ಅಂಕೆ();
        ಆದರೆ (ಅಂಕೆ.isAnyXZ()) ಹಿಂತಿರುಗಿ Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ::XZ;
        ಮೌಲ್ಯ = Fsm_State_ಮೌಲ್ಯ{ಅಂಕೆ};
        ಹಿಂತಿರುಗಿ Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ::OK;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ ಶುದ್ಧ_State_Comparison_No_Alias(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, FsmStateComparison& cmp) {
        AstEq* ಸ್ಥಿರಾಂಕ eqp = VN_CAST(exprp, Eq);
        ಆದರೆ (!eqp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = eqp->rhsp();
        ಆದರೆ (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->ತಳ_Fromp(ನಿಜ), VarRef);
            ಆದರೆ (!vrefp) { ಹಿಂತಿರುಗಿ ಸುಳ್ಳು; }
            valuep = eqp->lhsp();
        }

        Fsm_State_ಮೌಲ್ಯ ಮೌಲ್ಯ;
        ಆದರೆ (const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ(valuep, ಮೌಲ್ಯ) != Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ::OK) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        cmp.stateVscp = vrefp->varScopep();
        cmp.valuep = valuep;
        cmp.ಮೌಲ್ಯ = ಮೌಲ್ಯ;
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ ಶುದ್ಧ_State_Comparison(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಸ್ಥಿರಾಂಕ FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        ಆದರೆ (ಶುದ್ಧ_State_Comparison_No_Alias(exprp, cmp)) ಹಿಂತಿರುಗಿ ನಿಜ;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ vrefp = VN_CAST(exprp, VarRef)) {
            ಸ್ಥಿರಾಂಕ FsmAliasMap::const_iterator it = aliases.ಕಂಡುಹಿಡಿ(vrefp->varScopep());
            ಆದರೆ (it == aliases.ಕೊನೆ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            cmp = it->ಎರಡನೇ;
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ supported_ತುದಿ_Level_Guard(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        ಆದರೆ (VN_IS(exprp, Or)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (VN_IS(exprp, RedAnd)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (VN_IS(exprp, RedOr)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (VN_IS(exprp, RedXor)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ resolveIfPredicate(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಸ್ಥಿರಾಂಕ FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, ತಾರ್ಕಿಕ& hasGuard) {
        std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> terms;
        std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> pending;
        pending.ತಳ್ಳು_ಹಿಂಭಾಗ(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        ಆಗ (!pending.ಖಾಲಿಗಳು()) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ nodep = pending.ಹಿಂಭಾಗ();
            pending.ತೆಗೆ_ಹಿಂಭಾಗ();
            ಆದರೆ (AstAnd* ಸ್ಥಿರಾಂಕ andp = VN_CAST(nodep, And)) {
                pending.ತಳ್ಳು_ಹಿಂಭಾಗ(andp->rhsp());
                pending.ತಳ್ಳು_ಹಿಂಭಾಗ(andp->lhsp());
            } ಇಲ್ಲದಿದ್ದರೆ {
                terms.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
            }
        }

        ತಾರ್ಕಿಕ sawComparison = ಸುಳ್ಳು;
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < terms.ಗಾತ್ರ_2(); ++i) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ termp = terms[i];
            FsmStateComparison ಪದ_Cmp;
            ಆದರೆ (ಶುದ್ಧ_State_Comparison(termp, aliases, ಪದ_Cmp /*ref*/)) {
                ಆದರೆ (sawComparison) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                cmp = ಪದ_Cmp;
                sawComparison = ನಿಜ;
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (!supported_ತುದಿ_Level_Guard(termp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            hasGuard = ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ sawComparison;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Alias(FsmAliasMap& aliases, std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, ಸ್ಥಿರಾಂಕ FsmStateComparison& cmp) {
        ಆದರೆ (ambiguous.ಕಂಡುಹಿಡಿ(aliasVscp) != ambiguous.ಕೊನೆ()) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ FsmAliasMap::iterator it = aliases.ಕಂಡುಹಿಡಿ(aliasVscp);
        ಆದರೆ (it == aliases.ಕೊನೆ()) {
            aliases.emplace(aliasVscp, cmp);
            ಹಿಂತಿರುಗಿ;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        ಆದರೆ (it->ಎರಡನೇ.stateVscp == cmp.stateVscp && it->ಎರಡನೇ.ಮೌಲ್ಯ == cmp.ಮೌಲ್ಯ) ಹಿಂತಿರುಗಿ;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        ಹಿಂತಿರುಗಿ;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ collect_Alias_From_ನಿಯೋಜಿಸು(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* assp, FsmAliasMap& aliases,
                                       std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ AstVarScope*>& ambiguous) {
        AstVarRef* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(assp->lhsp(), VarRef);
        ಆದರೆ (!lhsp) ಹಿಂತಿರುಗಿ;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        ಆದರೆ (!ಶುದ್ಧ_State_Comparison_No_Alias(assp->rhsp(), cmp /*ref*/)) ಹಿಂತಿರುಗಿ;
        ಕೂಡಿಸು_Alias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(Ast_ಗಂಟು* stmtsp, Ast_ಗಂಟು* limitp) ಸ್ಥಿರಾಂಕ {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            ಆದರೆ (Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = VN_CAST(nodep, ಗಂಟು_ನಿಯೋಜಿಸು)) {
                collect_Alias_From_ನಿಯೋಜಿಸು(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstVarScope* ಸ್ಥಿರಾಂಕ vscp : ambiguous) aliases.erase(vscp);
        ಹಿಂತಿರುಗಿ aliases;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ collectIfChain(AstIf* ifp, ಸ್ಥಿರಾಂಕ FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_ಗಣ<string> ನೋಡಿದ_ಮೌಲ್ಯಗಳು;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        ಆಗ (ನಿಜ) {
            FsmStateComparison cmp;
            ತಾರ್ಕಿಕ hasGuard = ಸುಳ್ಳು;
            ಆದರೆ (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಆದರೆ (chain.compareVscp && chain.compareVscp != cmp.stateVscp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಆದರೆ (!ನೋಡಿದ_ಮೌಲ್ಯಗಳು.ಸೇರಿಸು(cmp.ಮೌಲ್ಯ.ಕೀಲಿ()).ಎರಡನೇ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.ತಳ್ಳು_ಹಿಂಭಾಗ(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.ಮೌಲ್ಯ, hasGuard});

            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ elseNodep
                = singleMeaningfulBranch(skipLeadingIgnorableStmt(curp->elsesp()));
            ಆದರೆ (!elseNodep) ನಿಲ್ಲಿಸು;
            ಆದರೆ (AstIf* ಸ್ಥಿರಾಂಕ elseIfp = VN_CAST(elseNodep, If)) {
                curp = elseIfp;
                ಮುಂದುವರಿ;
            }
            chain.defaultStmtsp = elseNodep;
            ನಿಲ್ಲಿಸು;
        }
        ಹಿಂತಿರುಗಿ chain.branches.ಗಾತ್ರ_2() >= 2;
    }

    // Enum-backed FSMs should only use values that were interned as known states.
    // If a constant transition references some other encoding, warn and skip FSM
    // instrumentation for that edge rather than silently dropping it or turning
    // optional coverage into a hard compile failure.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ validate_ತಿಳಿದ_State_ಮೌಲ್ಯ(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ FsmStateSpace& stateSpace,
                                        ಸ್ಥಿರಾಂಕ Fsm_State_ಮೌಲ್ಯ& ಮೌಲ್ಯ, ಸ್ಥಿರಾಂಕ string& role) {
        ಆದರೆ (stateSpace.labels.ಕಂಡುಹಿಡಿ(ಮೌಲ್ಯ) != stateSpace.labels.ಕೊನೆ()) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (stateSpace.enumBacked) {
            ಸ್ಥಿರಾಂಕ string enumRole = role == "source" ? "case item value" : "assigned value";
            nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on enum state variable "
                                        + stateSpace.stateVarp->prettyNameQ() + ": " + enumRole
                                        + " " + ಮೌಲ್ಯ.warn_ಪಠ್ಯ()
                                        + " is not present in the declared enum");
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum state variable "
                                    + stateSpace.stateVarp->prettyNameQ() + ": " + role + " value "
                                    + ಮೌಲ್ಯ.warn_ಪಠ್ಯ()
                                    + " is not present in the inferred state space");
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಸ್ಥಿರ StateConstLabel stateLabelForConst(AstConst* constp) {
        ಸ್ಥಿರಾಂಕ string name = constp->origParamName();
        ಆದರೆ (!name.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ StateConstLabel{Ast_ಗಂಟು::prettyName(name), ನಿಜ, 0};
        ಹಿಂತಿರುಗಿ StateConstLabel{constp->name(), ಸುಳ್ಳು, 0};
    }

    ಸ್ಥಿರ ಶೂನ್ಯ updateStateLabel(FsmStateSpace& stateSpace, ಸ್ಥಿರಾಂಕ Fsm_State_ಮೌಲ್ಯ& ಮೌಲ್ಯ,
                                 ಸ್ಥಿರಾಂಕ StateConstLabel& label) {
        stateSpace.ಸ್ಥಿತಿಗಳು.at(stateSpace.labels.at(ಮೌಲ್ಯ).state_ಸೂಚ್ಯಂಕ).ಮೊದಲ = label.ಪಠ್ಯ;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ ಹೊಂದಿಕೆ_ರಿಜಿಸ್ಟರ್_Always(AstAlways* alwaysp, AstScope* scopep,
                                    Fsm_ರಿಜಿಸ್ಟರ್_Candidate& cand) {
        ಆದರೆ (!alwaysp->sentreep() || !alwaysp->sentreep()->has_ಅಂಚು()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep = singleMeaningfulStmt(stmtsp);
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        AstVarScope* stateVscp = ಶೂನ್ಯಸೂಚಕ;
        AstVarScope* ಮುಂದಿನ_Vscp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (AstIf* ಸ್ಥಿರಾಂಕ ifp = VN_CAST(nodep, If)) {
            ಆದರೆ (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            AstVarScope* resetStateVscp = ಶೂನ್ಯಸೂಚಕ;
            ಸ್ಥಿರಾಂಕ Reset_ನಿಯೋಜಿಸು_ಸ್ಥಿತಿ reset_ಸ್ಥಿತಿ
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಖಾಲಿಗಳು_Reset_Branch = reset_ಸ್ಥಿತಿ == Reset_ನಿಯೋಜಿಸು_ಸ್ಥಿತಿ::ಖಾಲಿಗಳು_3;
            ಆದರೆ (ಖಾಲಿಗಳು_Reset_Branch) {
                cand.resetArcs().clear();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (reset_ಸ್ಥಿತಿ == Reset_ನಿಯೋಜಿಸು_ಸ್ಥಿತಿ::NONE) {
                cand.resetArcs().clear();
                Fsm_State_ಮೌಲ್ಯ reset_ಮೌಲ್ಯ;
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ resetAssp
                    = ನೇರ_Const_State_ನಿಯೋಜಿಸು_ಗಂಟು(thenNodep, resetStateVscp, reset_ಮೌಲ್ಯ);
                ಆದರೆ (!resetAssp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                cand.resetArcs().emplace_ಹಿಂಭಾಗ(reset_ಮೌಲ್ಯ, resetAssp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (reset_ಸ್ಥಿತಿ == Reset_ನಿಯೋಜಿಸು_ಸ್ಥಿತಿ::MULTI_SAME_STATE) {
                cand.resetArcs().clear();
            }
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            ಆದರೆ (!ಗಂಟು_State_Var_ನಿಯೋಜಿಸು(elseNodep, stateVscp, ಮುಂದಿನ_Vscp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಆದರೆ (!ಖಾಲಿಗಳು_Reset_Branch && resetStateVscp != stateVscp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != ಶೂನ್ಯಸೂಚಕ);
        } ಇಲ್ಲದಿದ್ದರೆ {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resetCondp = ಶೂನ್ಯಸೂಚಕ;
            Fsm_State_ಮೌಲ್ಯ reset_ಮೌಲ್ಯ;
            // Note the active low 'reset ? next : CONST' form is not accepted
            ಆದರೆ (Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp
                = ನೇರ_Cond_State_Var_ನಿಯೋಜಿಸು(nodep, stateVscp, ಮುಂದಿನ_Vscp, resetCondp, reset_ಮೌಲ್ಯ)) {
                cand.resetArcs().emplace_ಹಿಂಭಾಗ(reset_ಮೌಲ್ಯ, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != ಶೂನ್ಯಸೂಚಕ);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!ಗಂಟು_State_Var_ನಿಯೋಜಿಸು(nodep, stateVscp, ಮುಂದಿನ_Vscp)) {
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
        }
        cand.scopep(scopep);
        cand.alwaysp(alwaysp);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(stateVscp);
        cand.ಮುಂದಿನ_Vscp(ಮುಂದಿನ_Vscp);
        cand.senses() = describe_Sen_ವೃಕ್ಷ(alwaysp->sentreep());
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(alwaysp->fileline());
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ ಕೂಡಿಸು_ಮೌಲ್ಯ_ಗಮ್ಯ_State_Space(Ast_ಗಂಟು* nodep, FsmStateSpace& stateSpace,
                                     ಸ್ಥಿರಾಂಕ Fsm_State_ಮೌಲ್ಯ& ಮೌಲ್ಯ, StateConstLabel label) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ labelIt = stateSpace.labels.ಕಂಡುಹಿಡಿ(ಮೌಲ್ಯ);
        ಆದರೆ (labelIt != stateSpace.labels.ಕೊನೆ()) {
            StateConstLabel& existingLabel = labelIt->ಎರಡನೇ;
            ಆದರೆ (existingLabel.ಪಠ್ಯ != label.ಪಠ್ಯ && existingLabel.fromParam && label.fromParam) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                        "state variable "
                                            + stateSpace.stateVarp->prettyNameQ()
                                            + " with multiple labels for the same value "
                                            + ಮೌಲ್ಯ.warn_ಪಠ್ಯ() + ": " + existingLabel.ಪಠ್ಯ
                                            + " and " + label.ಪಠ್ಯ);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            ಆದರೆ (!existingLabel.fromParam && label.fromParam) {
                existingLabel.ಪಠ್ಯ = label.ಪಠ್ಯ;
                existingLabel.fromParam = label.fromParam;
                updateStateLabel(stateSpace, ಮೌಲ್ಯ, label);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            StateConstLabel storedLabel = label;
            storedLabel.state_ಸೂಚ್ಯಂಕ = stateSpace.ಸ್ಥಿತಿಗಳು.ಗಾತ್ರ_2();
            stateSpace.ಸ್ಥಿತಿಗಳು.emplace_ಹಿಂಭಾಗ(label.ಪಠ್ಯ, ಮೌಲ್ಯ);
            stateSpace.labels.emplace(ಮೌಲ್ಯ, storedLabel);
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    ಸ್ಥಿರ ತಾರ್ಕಿಕ ಕೂಡಿಸು_ಅಭಿವ್ಯಕ್ತಿ_ಗಮ್ಯ_State_Space(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_ಮೌಲ್ಯ ಮೌಲ್ಯ;
        ಸ್ಥಿರಾಂಕ Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ ಸ್ಥಿತಿ_2 = const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ(valuep, ಮೌಲ್ಯ);
        ಆದರೆ (ಸ್ಥಿತಿ_2 != Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ::OK) {
            ಆದರೆ (ಸ್ಥಿತಿ_2 == Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ::XZ) {
                valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                         "state variable "
                                             + stateSpace.stateVarp->prettyNameQ()
                                             + " with X/Z state encoding values");
            }
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        AstConst* ಸ್ಥಿರಾಂಕ constp = VN_AS(valuep, Const);
        ಹಿಂತಿರುಗಿ ಕೂಡಿಸು_ಮೌಲ್ಯ_ಗಮ್ಯ_State_Space(valuep, stateSpace, ಮೌಲ್ಯ, stateLabelForConst(constp));
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ ಕೂಡಿಸು_Optional_ಗುರಿ_ಅಭಿವ್ಯಕ್ತಿ_ಗಮ್ಯ_State_Space(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_ಮೌಲ್ಯ ಮೌಲ್ಯ;
        ಸ್ಥಿರಾಂಕ Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ ಸ್ಥಿತಿ_2 = const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ(valuep, ಮೌಲ್ಯ);
        ಆದರೆ (ಸ್ಥಿತಿ_2 != Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ::OK) {
            valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                     "state variable "
                                         + stateSpace.stateVarp->prettyNameQ()
                                         + " with non-constant target state values");
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        AstConst* ಸ್ಥಿರಾಂಕ constp = VN_AS(valuep, Const);
        ಹಿಂತಿರುಗಿ ಕೂಡಿಸು_ಮೌಲ್ಯ_ಗಮ್ಯ_State_Space(valuep, stateSpace, ಮೌಲ್ಯ, stateLabelForConst(constp));
    }

    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Reset_Targets_ಗಮ್ಯ_State_Space(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.ಗಮ್ಯ_ಮೌಲ್ಯ().ascii(), ಸುಳ್ಳು, 0};
            ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                ಕೂಡಿಸು_ಮೌಲ್ಯ_ಗಮ್ಯ_State_Space(resetArc.nodep(), stateSpace, resetArc.ಗಮ್ಯ_ಮೌಲ್ಯ(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ collectDeclaredStateSpace(Ast_ಗಂಟು* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, ತಾರ್ಕಿಕ& needs_ಮೂಲ_ಮೌಲ್ಯಗಳು) {
        AstVar* ಸ್ಥಿರಾಂಕ stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        ಆದರೆ (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        ಆದರೆ (enump) {
            stateSpace.enumBacked = ನಿಜ;
            ಗಾಗಿ (Ast_Enum_ಅಂಶ* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_ಅಂಶ)) {
                ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_AS(itemp->valuep(), Const);
                ಸ್ಥಿರಾಂಕ Fsm_State_ಮೌಲ್ಯ ಮೌಲ್ಯ{constp->ಅಂಕೆ()};
                ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t state_ಸೂಚ್ಯಂಕ = stateSpace.ಸ್ಥಿತಿಗಳು.ಗಾತ್ರ_2();
                stateSpace.ಸ್ಥಿತಿಗಳು.emplace_ಹಿಂಭಾಗ(itemp->name(), ಮೌಲ್ಯ);
                stateSpace.labels.emplace(ಮೌಲ್ಯ,
                                          StateConstLabel{itemp->name(), ಸುಳ್ಳು, state_ಸೂಚ್ಯಂಕ});
            }
            ಹಿಂತಿರುಗಿ stateSpace.ಸ್ಥಿತಿಗಳು.ಗಾತ್ರ_2() >= 2;
        }

        ಆದರೆ (forced) {
            needs_ಮೂಲ_ಮೌಲ್ಯಗಳು = ನಿಜ;
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }

        needs_ಮೂಲ_ಮೌಲ್ಯಗಳು = ನಿಜ;
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_ValuepVisitor>
    ಸ್ಥಿರ ತಾರ್ಕಿಕ collect_State_Space_From_ಮೌಲ್ಯಗಳು(Ast_ಗಂಟು* warnNodep, AstVarScope* stateVscp,
                                            ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            ಸ್ಥಿರಾಂಕ T_ValuepVisitor& ಭೇಟಿ_Valueps) {
        ತಾರ್ಕಿಕ needs_ಮೂಲ_ಮೌಲ್ಯಗಳು = ಸುಳ್ಳು;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        ಆದರೆ (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needs_ಮೂಲ_ಮೌಲ್ಯಗಳು)) {
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಆದರೆ (!needs_ಮೂಲ_ಮೌಲ್ಯಗಳು) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಕೂಡಿಸು_Reset_Targets_ಗಮ್ಯ_State_Space(resetArcs, stateSpace);
        ಆದರೆ (!ಭೇಟಿ_Valueps(
                [&](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) { ಹಿಂತಿರುಗಿ ಕೂಡಿಸು_ಅಭಿವ್ಯಕ್ತಿ_ಗಮ್ಯ_State_Space(valuep, stateSpace); })) {
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಹಿಂತಿರುಗಿ stateSpace.ಸ್ಥಿತಿಗಳು.ಗಾತ್ರ_2() >= 2;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* ನಿಯೋಜಿಸು_Vscp,
                                  ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        ಹಿಂತಿರುಗಿ collect_State_Space_From_ಮೌಲ್ಯಗಳು(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, ನಿಯೋಜಿಸು_Vscp, &stateSpace](ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಭೇಟಿ_Valuep) {
                ಗಾಗಿ (Ast_Case_ಅಂಶ* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Case_ಅಂಶ)) {
                    ಆದರೆ (!itemp->isDefault()) {
                        ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                            ಆದರೆ (!ಭೇಟಿ_Valuep(condp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                        }
                    }
                    ಆದರೆ (Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp
                        = ನೇರ_State_ನಿಯೋಜಿಸು(itemp->stmtsp(), ನಿಯೋಜಿಸು_Vscp)) {
                        Fsm_State_ಮೌಲ್ಯ then_ಮೌಲ್ಯ;
                        Fsm_State_ಮೌಲ್ಯ else_ಮೌಲ್ಯ;
                        AstCond* ಸ್ಥಿರಾಂಕ condp = VN_CAST(assp->rhsp(), Cond);
                        ಆದರೆ (condp
                            && ನೇರ_State_Cond_Const_ನಿಯೋಜಿಸು(itemp->stmtsp(), ನಿಯೋಜಿಸು_Vscp, then_ಮೌಲ್ಯ,
                                                          else_ಮೌಲ್ಯ)) {
                            ಆದರೆ (!ಭೇಟಿ_Valuep(condp->thenp())) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                            ಆದರೆ (!ಭೇಟಿ_Valuep(condp->elsep())) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!ಕೂಡಿಸು_Optional_ಗುರಿ_ಅಭಿವ್ಯಕ್ತಿ_ಗಮ್ಯ_State_Space(assp->rhsp(), stateSpace)) {
                            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                        }
                    }
                }
                ಹಿಂತಿರುಗಿ ನಿಜ;
            });
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ collectStateSpace(ಸ್ಥಿರಾಂಕ FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* ನಿಯೋಜಿಸು_Vscp,
                                  ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        ಹಿಂತಿರುಗಿ collect_State_Space_From_ಮೌಲ್ಯಗಳು(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, ನಿಯೋಜಿಸು_Vscp, &stateSpace](ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಭೇಟಿ_Valuep) {
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(ಭೇಟಿ_Valuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = ನೇರ_State_ನಿಯೋಜಿಸು(branch.stmtsp, ನಿಯೋಜಿಸು_Vscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    Fsm_State_ಮೌಲ್ಯ then_ಮೌಲ್ಯ;
                    Fsm_State_ಮೌಲ್ಯ else_ಮೌಲ್ಯ;
                    AstCond* ಸ್ಥಿರಾಂಕ condp = VN_CAST(assp->rhsp(), Cond);
                    ಆದರೆ (condp) {
                        UASSERT_OBJ(ನೇರ_State_Cond_Const_ನಿಯೋಜಿಸು(branch.stmtsp, ನಿಯೋಜಿಸು_Vscp,
                                                               then_ಮೌಲ್ಯ, else_ಮೌಲ್ಯ),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        ಆದರೆ (!ಭೇಟಿ_Valuep(condp->thenp())) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                        ಆದರೆ (!ಭೇಟಿ_Valuep(condp->elsep())) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!ಕೂಡಿಸು_Optional_ಗುರಿ_ಅಭಿವ್ಯಕ್ತಿ_ಗಮ್ಯ_State_Space(assp->rhsp(), stateSpace)) {
                        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                    }
                }
                ಹಿಂತಿರುಗಿ ನಿಜ;
            });
    }

    // Extract supported case-item transitions in one place so the conservative
    // policy for direct and ternary forms stays consistent. The false exits in
    // this helper are deliberate subset boundaries: they document shapes we do
    // not yet model in this PR and that future FSM-detection work may widen.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ emit_Case_ಅಂಶ_Arcs(Fsm_ನಕ್ಷೆ& ನಕ್ಷೆ, Ast_Case_ಅಂಶ* itemp, AstVarScope* stateVscp,
                                 ಸ್ಥಿರಾಂಕ FsmStateSpace& stateSpace, ತಾರ್ಕಿಕ inclCond) {
        std::ಸದಿಶ<std::pair<string, Fsm_State_ಮೌಲ್ಯ>> froms;
        ಆದರೆ (itemp->isDefault()) {
            ಆದರೆ (!inclCond) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            froms.emplace_ಹಿಂಭಾಗ("default", Fsm_State_ಮೌಲ್ಯ{});
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                Fsm_State_ಮೌಲ್ಯ ಮೌಲ್ಯ;
                ಆದರೆ (const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ(condp, ಮೌಲ್ಯ) != Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ::OK) ಮುಂದುವರಿ;
                ಆದರೆ (!validate_ತಿಳಿದ_State_ಮೌಲ್ಯ(condp, stateSpace, ಮೌಲ್ಯ, "source")) ಹಿಂತಿರುಗಿ ನಿಜ;
                froms.emplace_ಹಿಂಭಾಗ(label_For_ಮೌಲ್ಯ(stateSpace.labels, ಮೌಲ್ಯ), ಮೌಲ್ಯ);
            }
            ಆದರೆ (froms.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }

        ಆದರೆ (Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = ನೇರ_State_ನಿಯೋಜಿಸು(itemp->stmtsp(), stateVscp)) {
            Fsm_State_ಮೌಲ್ಯ ಗಮ್ಯ_ಮೌಲ್ಯ;
            ಸ್ಥಿರಾಂಕ Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ ಸ್ಥಿತಿ_2 = const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ(assp->rhsp(), ಗಮ್ಯ_ಮೌಲ್ಯ);
            ಆದರೆ (ಸ್ಥಿತಿ_2 == Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ::OK) {
                ಆದರೆ (!validate_ತಿಳಿದ_State_ಮೌಲ್ಯ(assp, stateSpace, ಗಮ್ಯ_ಮೌಲ್ಯ, "target")) ಹಿಂತಿರುಗಿ ನಿಜ;
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<string, Fsm_State_ಮೌಲ್ಯ>& from : froms) {
                    ನಕ್ಷೆ.ಕೂಡಿಸು_Arc(from.ಎರಡನೇ, ಗಮ್ಯ_ಮೌಲ್ಯ, ಸುಳ್ಳು, ಸುಳ್ಳು, itemp->isDefault(),
                                 assp->fileline());
                }
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        }

        Fsm_State_ಮೌಲ್ಯ then_ಮೌಲ್ಯ;
        Fsm_State_ಮೌಲ್ಯ else_ಮೌಲ್ಯ;
        ಆದರೆ (ನೇರ_State_Cond_Const_ನಿಯೋಜಿಸು(itemp->stmtsp(), stateVscp, then_ಮೌಲ್ಯ, else_ಮೌಲ್ಯ)
            || if_State_Const_ನಿಯೋಜಿಸು(itemp->stmtsp(), stateVscp, then_ಮೌಲ್ಯ, else_ಮೌಲ್ಯ)) {
            ಆದರೆ (!validate_ತಿಳಿದ_State_ಮೌಲ್ಯ(itemp->stmtsp(), stateSpace, then_ಮೌಲ್ಯ, "target"))
                ಹಿಂತಿರುಗಿ ನಿಜ;
            ಆದರೆ (!validate_ತಿಳಿದ_State_ಮೌಲ್ಯ(itemp->stmtsp(), stateSpace, else_ಮೌಲ್ಯ, "target"))
                ಹಿಂತಿರುಗಿ ನಿಜ;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Fsm_State_ಮೌಲ್ಯ& branch_ಮೌಲ್ಯ : {then_ಮೌಲ್ಯ, else_ಮೌಲ್ಯ}) {
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<string, Fsm_State_ಮೌಲ್ಯ>& from : froms) {
                    ನಕ್ಷೆ.ಕೂಡಿಸು_Arc(from.ಎರಡನೇ, branch_ಮೌಲ್ಯ, ಸುಳ್ಳು, ನಿಜ, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }

        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ emitStmtArcsFrom(Fsm_ನಕ್ಷೆ& ನಕ್ಷೆ, Ast_ಗಂಟು* stmtsp, AstVarScope* stateVscp,
                                 ಸ್ಥಿರಾಂಕ FsmStateSpace& stateSpace, Fsm_State_ಮೌಲ್ಯ from_ಮೌಲ್ಯ,
                                 ತಾರ್ಕಿಕ isDefault, ತಾರ್ಕಿಕ forceCond) {
        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = ನೇರ_State_ನಿಯೋಜಿಸು(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        Fsm_State_ಮೌಲ್ಯ ಗಮ್ಯ_ಮೌಲ್ಯ;
        ಸ್ಥಿರಾಂಕ Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ ಸ್ಥಿತಿ_2 = const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ(assp->rhsp(), ಗಮ್ಯ_ಮೌಲ್ಯ);
        ಆದರೆ (ಸ್ಥಿತಿ_2 == Const_ಮೌಲ್ಯ_ಸ್ಥಿತಿ::OK) {
            ಆದರೆ (!validate_ತಿಳಿದ_State_ಮೌಲ್ಯ(assp, stateSpace, ಗಮ್ಯ_ಮೌಲ್ಯ, "target")) ಹಿಂತಿರುಗಿ;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            ನಕ್ಷೆ.ಕೂಡಿಸು_Arc(from_ಮೌಲ್ಯ, ಗಮ್ಯ_ಮೌಲ್ಯ, ಸುಳ್ಳು, forceCond, isDefault, assp->fileline());
            ಹಿಂತಿರುಗಿ;
        }

        Fsm_State_ಮೌಲ್ಯ then_ಮೌಲ್ಯ;
        Fsm_State_ಮೌಲ್ಯ else_ಮೌಲ್ಯ;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ cond_ನಿಯೋಜಿಸು
            = ನೇರ_State_Cond_Const_ನಿಯೋಜಿಸು(stmtsp, stateVscp, then_ಮೌಲ್ಯ, else_ಮೌಲ್ಯ);
        UASSERT_OBJ(cond_ನಿಯೋಜಿಸು, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        ಆದರೆ (!validate_ತಿಳಿದ_State_ಮೌಲ್ಯ(stmtsp, stateSpace, then_ಮೌಲ್ಯ, "target")) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!validate_ತಿಳಿದ_State_ಮೌಲ್ಯ(stmtsp, stateSpace, else_ಮೌಲ್ಯ, "target")) ಹಿಂತಿರುಗಿ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Fsm_State_ಮೌಲ್ಯ& branch_ಮೌಲ್ಯ : {then_ಮೌಲ್ಯ, else_ಮೌಲ್ಯ}) {
            ನಕ್ಷೆ.ಕೂಡಿಸು_Arc(from_ಮೌಲ್ಯ, branch_ಮೌಲ್ಯ, ಸುಳ್ಳು, ನಿಜ, isDefault, stmtsp->fileline());
        }
    }

    ಸ್ಥಿರ ಶೂನ್ಯ emitIfChainArcs(Fsm_ನಕ್ಷೆ& ನಕ್ಷೆ, ಸ್ಥಿರಾಂಕ FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, ಸ್ಥಿರಾಂಕ FsmStateSpace& stateSpace) {
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < chain.branches.ಗಾತ್ರ_2(); ++i) {
            ಸ್ಥಿರಾಂಕ FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            ಆದರೆ (!validate_ತಿಳಿದ_State_ಮೌಲ್ಯ(branch.ifp, stateSpace, branch.from_ಮೌಲ್ಯ, "source"))
                ಹಿಂತಿರುಗಿ;
            emitStmtArcsFrom(ನಕ್ಷೆ, branch.stmtsp, stateVscp, stateSpace, branch.from_ಮೌಲ್ಯ, ಸುಳ್ಳು,
                             branch.has_ತುದಿ_Guard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Reset_Arcs(Fsm_ನಕ್ಷೆ& ನಕ್ಷೆ, ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<FsmResetArcDesc>& resetArcs,
                             ಸ್ಥಿರಾಂಕ FsmStateSpace& stateSpace) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ FsmResetArcDesc& resetArc : resetArcs) {
            ಆದರೆ (!validate_ತಿಳಿದ_State_ಮೌಲ್ಯ(resetArc.nodep(), stateSpace, resetArc.ಗಮ್ಯ_ಮೌಲ್ಯ(),
                                         "target"))
                ಮುಂದುವರಿ;
            ನಕ್ಷೆ.ಕೂಡಿಸು_Arc(Fsm_State_ಮೌಲ್ಯ{}, resetArc.ಗಮ್ಯ_ಮೌಲ್ಯ(), ನಿಜ, ಸುಳ್ಳು, ಸುಳ್ಳು,
                         resetArc.nodep()->fileline());
        }
    }

    // Turn one candidate case statement into the graph representation that the
    // later lowering phase will consume directly, while reviewers can still
    // inspect the extracted machine via DOT dumps.
    ಶೂನ್ಯ processCase(AstCase* casep, AstVarScope* ನಿಯೋಜಿಸು_Vscp, ಸ್ಥಿರಾಂಕ Fsm_ರಿಜಿಸ್ಟರ್_Candidate& reg) {
        UASSERT_OBJ(ನಿಯೋಜಿಸು_Vscp, casep, "FSM case processing requires a non-null assignment var");
        AstVarScope* ಸ್ಥಿರಾಂಕ stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        ಆದರೆ (!collectStateSpace(casep, stateVscp, ನಿಯೋಜಿಸು_Vscp, reg.resetArcs(), stateSpace)) ಹಿಂತಿರುಗಿ;
        DetectedFsm& ನಮೂದು = m_state.fsmFor(stateVscp);
        ಆದರೆ (!ನಮೂದು.graphp) {
            ನಮೂದು.graphp.reset(ಹೊಸ Fsm_ನಕ್ಷೆ{});
            ನಮೂದು.graphp->scopep(reg.scopep());
            ನಮೂದು.graphp->stateAlwaysp(reg.alwaysp());
            ನಮೂದು.graphp->stateVarName(stateVscp->prettyName());
            ನಮೂದು.graphp->state_Var_ಆಂತರಿಕ_Name(stateVscp->varp()->name());
            ನಮೂದು.graphp->stateVarScopep(stateVscp);
            ನಮೂದು.graphp->sampleVarScopep(reg.sampleVscp());
            ನಮೂದು.graphp->senses() = reg.senses();
            ನಮೂದು.graphp->resetCond() = reg.resetCond();
            ನಮೂದು.graphp->hasResetCond(reg.hasResetCond());
            ನಮೂದು.graphp->inclCond(reg.inclCond());
            ನಮೂದು.graphp->fileline(casep->fileline());
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<string, Fsm_State_ಮೌಲ್ಯ>& state : stateSpace.ಸ್ಥಿತಿಗಳು) {
                ನಮೂದು.graphp->ಕೂಡಿಸು_State_ಶೃಂಗ(state.ಮೊದಲ, state.ಎರಡನೇ);
            }
            ಕೂಡಿಸು_Reset_Arcs(*ನಮೂದು.graphp, reg.resetArcs(), stateSpace);
        }
        ಗಾಗಿ (Ast_Case_ಅಂಶ* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_ಅಂಶ)) {
            emit_Case_ಅಂಶ_Arcs(*ನಮೂದು.graphp, itemp, ನಿಯೋಜಿಸು_Vscp, stateSpace,
                             ನಮೂದು.graphp->inclCond());
        }
    }

    ಶೂನ್ಯ processIfChain(ಸ್ಥಿರಾಂಕ FsmIfChainCandidate& chain, AstVarScope* ನಿಯೋಜಿಸು_Vscp,
                        ಸ್ಥಿರಾಂಕ Fsm_ರಿಜಿಸ್ಟರ್_Candidate& reg) {
        UASSERT_OBJ(ನಿಯೋಜಿಸು_Vscp, chain.ifp,
                    "FSM if-chain processing requires a non-null assignment var");
        AstVarScope* ಸ್ಥಿರಾಂಕ stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        ಆದರೆ (!collectStateSpace(chain, stateVscp, ನಿಯೋಜಿಸು_Vscp, reg.resetArcs(), stateSpace)) ಹಿಂತಿರುಗಿ;
        DetectedFsm& ನಮೂದು = m_state.fsmFor(stateVscp);
        // Case candidates keep ownership of existing graphs; reaching this path
        // means the if-chain is the only supported dispatch for this FSM.
        UASSERT_OBJ(!ನಮೂದು.graphp, chain.ifp, "FSM if-chain graph should not already exist");
        ನಮೂದು.graphp.reset(ಹೊಸ Fsm_ನಕ್ಷೆ{});
        ನಮೂದು.graphp->scopep(reg.scopep());
        ನಮೂದು.graphp->stateAlwaysp(reg.alwaysp());
        ನಮೂದು.graphp->stateVarName(stateVscp->prettyName());
        ನಮೂದು.graphp->state_Var_ಆಂತರಿಕ_Name(stateVscp->varp()->name());
        ನಮೂದು.graphp->stateVarScopep(stateVscp);
        ನಮೂದು.graphp->sampleVarScopep(reg.sampleVscp());
        ನಮೂದು.graphp->senses() = reg.senses();
        ನಮೂದು.graphp->resetCond() = reg.resetCond();
        ನಮೂದು.graphp->hasResetCond(reg.hasResetCond());
        ನಮೂದು.graphp->inclCond(reg.inclCond());
        ನಮೂದು.graphp->fileline(chain.ifp->fileline());
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<string, Fsm_State_ಮೌಲ್ಯ>& state : stateSpace.ಸ್ಥಿತಿಗಳು) {
            ನಮೂದು.graphp->ಕೂಡಿಸು_State_ಶೃಂಗ(state.ಮೊದಲ, state.ಎರಡನೇ);
        }
        ಕೂಡಿಸು_Reset_Arcs(*ನಮೂದು.graphp, reg.resetArcs(), stateSpace);
        emitIfChainArcs(*ನಮೂದು.graphp, chain, ನಿಯೋಜಿಸು_Vscp, stateSpace);
    }

    // Find the first supported FSM candidate in a clocked always block, warn on
    // additional candidates, and attach reset arcs when present. Candidate
    // filtering stays narrow on purpose: we prefer to skip ambiguous shapes now
    // and expand detection in a later PR rather than over-infer coverage from
    // forms we do not yet model confidently.
    ಶೂನ್ಯ process_One_ಖಂಡ_Always(ಸ್ಥಿರಾಂಕ FsmComboAlways& one_ಖಂಡ) {
        ಸ್ಥಿರಾಂಕ ರಿಜಿಸ್ಟರ್_Always_Analyzer analyzer{one_ಖಂಡ.scopep()};
        AstAlways* ಸ್ಥಿರಾಂಕ alwaysp = one_ಖಂಡ.alwaysp();
        ಆದರೆ (!alwaysp->sentreep() || !alwaysp->sentreep()->has_ಅಂಚು()) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<std::pair<AstCase*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>> candidates
            = analyzer.one_ಖಂಡ_Candidates(alwaysp);

        FsmCaseCandidate ಮೊದಲ_Cand;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<AstCase*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& cand : candidates) {
            AstVarRef* ಸ್ಥಿರಾಂಕ selp = VN_CAST(cand.ಮೊದಲ->exprp(), VarRef);
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = selp ? selp->varScopep() : ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (!vscp) ಮುಂದುವರಿ;
            ಆದರೆ (!ಮೊದಲ_Cand.stateVscp) {
                ಮೊದಲ_Cand.warnNodep = cand.ಮೊದಲ;
                ಮೊದಲ_Cand.stateVscp = vscp;
                Fsm_ರಿಜಿಸ್ಟರ್_Candidate reg;
                analyzer.build_One_ಖಂಡ_Candidate(alwaysp, vscp, cand.ಎರಡನೇ, reg);
                processCase(cand.ಮೊದಲ, vscp, reg);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (vscp != ಮೊದಲ_Cand.stateVscp) {
                cand.ಮೊದಲ->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed case statements found in "
                                   "the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.ಮೊದಲ, ಮೊದಲ_Cand));
            } ಇಲ್ಲದಿದ್ದರೆ {
                cand.ಮೊದಲ->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported case "
                                   "statements found in the same always block. Only the first "
                                   "candidate will be instrumented."
                                       << candidateConflictContext(cand.ಮೊದಲ, ಮೊದಲ_Cand));
            }
        }

        ಆದರೆ (ಮೊದಲ_Cand.stateVscp) ಹಿಂತಿರುಗಿ;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<std::pair<AstIf*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>> ifCandidates
            = analyzer.one_ಖಂಡ_If_Candidates(alwaysp);
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<AstIf*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& cand : ifCandidates) {
            ಸ್ಥಿರಾಂಕ FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.ಮೊದಲ);
            FsmIfChainCandidate chain;
            ಆದರೆ (!collectIfChain(cand.ಮೊದಲ, aliases, chain)) ಮುಂದುವರಿ;
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = chain.compareVscp;
            ಆದರೆ (!if_Chain_Supported_Transition_ಗಂಟು(chain, vscp)) ಮುಂದುವರಿ;
            ಆದರೆ (!ಮೊದಲ_Cand.stateVscp) {
                ಮೊದಲ_Cand.warnNodep = cand.ಮೊದಲ;
                ಮೊದಲ_Cand.stateVscp = vscp;
                Fsm_ರಿಜಿಸ್ಟರ್_Candidate reg;
                analyzer.build_One_ಖಂಡ_Candidate(alwaysp, vscp, cand.ಎರಡನೇ, reg);
                processIfChain(chain, vscp, reg);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (vscp != ಮೊದಲ_Cand.stateVscp) {
                cand.ಮೊದಲ->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed transition candidates found "
                                   "in the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.ಮೊದಲ, ಮೊದಲ_Cand));
            } ಇಲ್ಲದಿದ್ದರೆ {
                cand.ಮೊದಲ->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported "
                                   "transition candidates found in the same always block. Only "
                                   "the first candidate will be instrumented."
                                       << candidateConflictContext(cand.ಮೊದಲ, ಮೊದಲ_Cand));
            }
        }
    }

    // Phase 1 two-process pairing scans combinational always blocks only after
    // all strict register candidates have been collected, so source order does
    // not matter.
    ಸ್ಥಿರ ಶೂನ್ಯ warnComboSameAlways(Ast_ಗಂಟು* warnNodep, ಸ್ಥಿರಾಂಕ FsmCaseCandidate& ಮೊದಲ_Cand) {
        warnNodep->v3warn(FSMMULTI,
                          "FSM coverage: multiple supported transition candidates found in "
                          "the same combinational always block. Only the first candidate "
                          "will be instrumented."
                              << candidateConflictContext(warnNodep, ಮೊದಲ_Cand));
    }

    ಶೂನ್ಯ processComboAlways(ಸ್ಥಿರಾಂಕ FsmComboAlways& combo) {
        ಸ್ಥಿರಾಂಕ ComboAlwaysAnalyzer analyzer{m_ರಿಜಿಸ್ಟರ್_Candidates};
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate ಮೊದಲ_Cand;
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ಸ್ಥಿರಾಂಕ casep = VN_CAST(nodep, Case);
            ಆದರೆ (!casep) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ ComboAlwaysAnalyzer::Combo_ಹೊಂದಿಕೆ ಹೊಂದಿಕೆ = analyzer.ಹೊಂದಿಕೆ_Case(stmtsp, casep);
            ಸ್ಥಿರಾಂಕ Fsm_ರಿಜಿಸ್ಟರ್_Candidate* ಸ್ಥಿರಾಂಕ matchedp = ಹೊಂದಿಕೆ.matchedp;
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ matchedWarnNodep = ಹೊಂದಿಕೆ.warnNodep;
            ಆದರೆ (!matchedp) ಮುಂದುವರಿ;
            ಆದರೆ (!ಮೊದಲ_Cand.stateVscp) {
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಸೇರಿಸು_Pair = m_comboPaired.emplace(
                    matchedp->stateVscp(),
                    FsmCaseCandidate{matchedWarnNodep,
                                     const_cast<AstVarScope*>(matchedp->stateVscp())});
                ಆದರೆ (!ಸೇರಿಸು_Pair.ಎರಡನೇ) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  ಸೇರಿಸು_Pair.ಮೊದಲ->ಎರಡನೇ));
                    ಮುಂದುವರಿ;
                }
                ಮೊದಲ_Cand.warnNodep = matchedWarnNodep;
                ಮೊದಲ_Cand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processCase(casep, matchedp->ಮುಂದಿನ_Vscp(), *matchedp);
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (matchedp->stateVscp() != ಮೊದಲ_Cand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, ಮೊದಲ_Cand);
                ಮುಂದುವರಿ;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported case statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, ಮೊದಲ_Cand));
        }
        ಆದರೆ (ಮೊದಲ_Cand.stateVscp) ಹಿಂತಿರುಗಿ;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* ಸ್ಥಿರಾಂಕ ifp = VN_CAST(nodep, If);
            ಆದರೆ (!ifp) ಮುಂದುವರಿ;
            FsmIfChainCandidate chain;
            ಸ್ಥಿರಾಂಕ FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            ಆದರೆ (!collectIfChain(ifp, aliases, chain)) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ ComboAlwaysAnalyzer::Combo_ಹೊಂದಿಕೆ ಹೊಂದಿಕೆ = analyzer.ಹೊಂದಿಕೆ_If_Chain(stmtsp, chain);
            ಸ್ಥಿರಾಂಕ Fsm_ರಿಜಿಸ್ಟರ್_Candidate* ಸ್ಥಿರಾಂಕ matchedp = ಹೊಂದಿಕೆ.matchedp;
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ matchedWarnNodep = ಹೊಂದಿಕೆ.warnNodep;
            ಆದರೆ (!matchedp) ಮುಂದುವರಿ;
            ಆದರೆ (!ಮೊದಲ_Cand.stateVscp) {
                ಸ್ಥಿರಾಂಕ std::pair<std::unordered_map<ಸ್ಥಿರಾಂಕ AstVarScope*, FsmCaseCandidate>::iterator,
                                ತಾರ್ಕಿಕ>
                    ಸೇರಿಸು_Pair = m_comboPaired.emplace(
                        matchedp->stateVscp(),
                        FsmCaseCandidate{matchedWarnNodep,
                                         const_cast<AstVarScope*>(matchedp->stateVscp())});
                ಆದರೆ (!ಸೇರಿಸು_Pair.ಎರಡನೇ) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  ಸೇರಿಸು_Pair.ಮೊದಲ->ಎರಡನೇ));
                    ಮುಂದುವರಿ;
                }
                ಮೊದಲ_Cand.warnNodep = matchedWarnNodep;
                ಮೊದಲ_Cand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processIfChain(chain, matchedp->ಮುಂದಿನ_Vscp(), *matchedp);
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (matchedp->stateVscp() != ಮೊದಲ_Cand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, ಮೊದಲ_Cand);
                ಮುಂದುವರಿ;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported if-chain statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, ಮೊದಲ_Cand));
        }
    }

    // Track the current scope so each detected FSM records the module/scope
    // where instrumentation must later be inserted.
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    // Collect processes first, then analyze FSM candidates once all alias and
    // register information is available.
    ಶೂನ್ಯ ಭೇಟಿ(AstAlways* nodep) override {
        ಆದರೆ (nodep->keyword() == VAlwaysKwd::CONT_ನಿಯೋಜಿಸು) {
            iterateChildren(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_one_ಖಂಡ_Alwayss.emplace_ಹಿಂಭಾಗ(m_scopep, nodep);
        ಸ್ಥಿರಾಂಕ ರಿಜಿಸ್ಟರ್_Always_Analyzer analyzer{m_scopep};
        Fsm_ರಿಜಿಸ್ಟರ್_Candidate reg;
        ಆದರೆ (analyzer.ಹೊಂದಿಕೆ_ರಿಜಿಸ್ಟರ್_Candidate(nodep, reg)) {
            AstVarScope* ಸ್ಥಿರಾಂಕ stateVscp = reg.stateVscp();
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಸಿಕ್ಕಿತು
                = std::any_ದ(m_ರಿಜಿಸ್ಟರ್_Candidates.cbegin(), m_ರಿಜಿಸ್ಟರ್_Candidates.cend(),
                              [stateVscp](ಸ್ಥಿರಾಂಕ Fsm_ರಿಜಿಸ್ಟರ್_Candidate& existing) {
                                  ಹಿಂತಿರುಗಿ existing.stateVscp() == stateVscp;
                              });
            ಆದರೆ (!ಸಿಕ್ಕಿತು) { m_ರಿಜಿಸ್ಟರ್_Candidates.emplace_ಹಿಂಭಾಗ(reg); }
        }
        ಆದರೆ (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            m_comboAlwayss.emplace_ಹಿಂಭಾಗ(m_scopep, nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->keyword() == VAlwaysKwd::ALWAYS) {
            ಆದರೆ (!nodep->sentreep() || isPlainComboSentree(nodep->sentreep())) {
                m_comboAlwayss.emplace_ಹಿಂಭಾಗ(m_scopep, nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_nonComboAlwayss.emplace_ಹಿಂಭಾಗ(m_scopep, nodep);
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_W* nodep) override {
        // Continuous aliases are unordered hardware connections, so source
        // order should not affect whether an if-chain FSM is recognized.
        collect_Alias_From_ನಿಯೋಜಿಸು(nodep, m_stateAliases, m_ambiguousStateAliases);
        collect_Cell_ಪೋರ್ಟ್_Alias(nodep);
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override {
        // Cells are matched after the full traversal because linkdot lowers
        // uninlined port connections into sibling continuous assignments.
        ಆದರೆ (m_scopep && fsm_ರಿಜಿಸ್ಟರ್_Wrapper_Desc(nodep)) ಕೂಡಿಸು_Wrapper_Cell(m_scopep, nodep);
        iterateChildren(nodep);
    }

    // Continue the walk through the rest of the design hierarchy.
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            Fsm_ರಿಜಿಸ್ಟರ್_Candidate reg;
            ಆದರೆ (ಹೊಂದಿಕೆ_Fsm_Wrapper_Cell(wrapperCell.ಮೊದಲ, wrapperCell.ಎರಡನೇ, reg)) {
                m_ರಿಜಿಸ್ಟರ್_Candidates.emplace_ಹಿಂಭಾಗ(reg);
            }
        }
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ FsmComboAlways& one_ಖಂಡ : m_one_ಖಂಡ_Alwayss) process_One_ಖಂಡ_Always(one_ಖಂಡ);
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ FsmComboAlways& combo : m_comboAlwayss) processComboAlways(combo);
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ FsmComboAlways& combo : m_nonComboAlwayss) warnUnsupportedComboAlways(combo);
    }
};

// Builder of statements that will handle incrementation of proper AstNodeCoverDecls - which will
// be also created here
ವರ್ಗ FsmCoverIncBuilder final {
    ಸ್ಥಿರಾಂಕ Fsm_ನಕ್ಷೆ& m_ನಕ್ಷೆ;  // Fsm graph
    std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Fsm_State_ಶೃಂಗ*> m_ಸ್ಥಿತಿಗಳು;  // States vertexes inside fsm graph
    std::unordered_ಗಣ<uint64_t>
        m_undefined_ಸ್ಥಿತಿಗಳು;  // States that have no enum value but are possible to represent with a
                            // variable of given width.
                            // This variable keeps uint64_t instead of V3Numebr because it is safe
                            // to assume that nobody has over 16M TB of RAM
    ಸ್ಥಿರಾಂಕ Fsm_ಶೃಂಗ* m_defaultVertexp = ಶೂನ್ಯಸೂಚಕ;  // Default vertex
    ಸ್ಥಿರಾಂಕ Fsm_ಶೃಂಗ* m_resetVertexp = ಶೂನ್ಯಸೂಚಕ;  // Reset vertex
    ಗಾತ್ರ_t m_ಅಂಚುಗಳು_ಎಣಿಕೆ = 0;  // Total count of edges
    AstVarScope* ಸ್ಥಿರಾಂಕ m_prevVscp;  // Previous state of fsm
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ m_flp;  // Fileline pointer to use across the builder
    ಸ್ಥಿರಾಂಕ std::string m_modPrettyName;  // Pretty fsm mode name
    Ast_ಗಂಟು_Stmt* m_coverIncp = ಶೂನ್ಯಸೂಚಕ;  // Increments proper CoverDecl after each change
    Ast_ಗಂಟು_Stmt* m_initialCoverIncp = ಶೂನ್ಯಸೂಚಕ;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = ಶೂನ್ಯಸೂಚಕ;  // List of CoverDecls
    ತಾರ್ಕಿಕ m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(ಸ್ಥಿರಾಂಕ uint64_t ಮೌಲ್ಯ) {
        V3_ಸಂಖ್ಯೆ ಅಂಕೆ{static_cast<Ast_ಗಂಟು*>(ಶೂನ್ಯಸೂಚಕ), m_prevVscp->ಅಗಲ()};
        ಅಂಕೆ.ಗಣ_Quad(ಮೌಲ್ಯ);
        ಅಂಕೆ.isSigned(m_prevVscp->dtypep()->isSigned());
        ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{m_flp, ಅಂಕೆ};
    }

    std::string quad_ಗಮ್ಯ_ಬಿಟ್_Stirng_Label(ಸ್ಥಿರಾಂಕ uint64_t val) {
        ಪೂರ್ಣಾಂಕ ಅಗಲ = m_prevVscp->ಅಗಲ();
        UASSERT(ಅಗಲ < 64, "We should not expand values that wide");
        std::string ಫಲಿತಾಂಶ = std::ಗಮ್ಯ_string(ಅಗಲ) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        ಆಗ (--ಅಗಲ >= 0) ಫಲಿತಾಂಶ.ತಳ್ಳು_ಹಿಂಭಾಗ("01"[(val >> ಅಗಲ) & 1]);
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }

    ಶೂನ್ಯ init_Existing_ಸ್ಥಿತಿಗಳು() {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_ನಕ್ಷೆ.vertices()) {
            ಸ್ಥಿರಾಂಕ Fsm_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vertexp = vtx.as<Fsm_ಶೃಂಗ>();
            m_ಅಂಚುಗಳು_ಎಣಿಕೆ += vertexp->ಒಳ_ಅಂಚುಗಳು().ಗಾತ್ರ_2();
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Fsm_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ statep = vertexp->cast<Fsm_State_ಶೃಂಗ>()) {
                m_ಸ್ಥಿತಿಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(statep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (vertexp->isDefaultAny()) {
                UASSERT_OBJ(!std::exchange(m_defaultVertexp, vertexp), m_ನಕ್ಷೆ.sampleVarScopep(),
                            "Two defaults?");
            } ಇಲ್ಲದಿದ್ದರೆ /* if (vertexp->isResetAny()) */ {
                UASSERT_OBJ(!std::exchange(m_resetVertexp, vertexp), m_ನಕ್ಷೆ.sampleVarScopep(),
                            "Two resets?");
            }
        }
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = m_ನಕ್ಷೆ.sampleVarScopep()->ಅಗಲ();
        m_isComplete = ಅಗಲ < 64 && m_ಸ್ಥಿತಿಗಳು.ಗಾತ್ರ_2() == (static_cast<ಗಾತ್ರ_t>(1) << ಅಗಲ);
    }

    ಶೂನ್ಯ init_Undefined_ಸ್ಥಿತಿಗಳು() {
        UASSERT_OBJ(m_ನಕ್ಷೆ.sampleVarScopep()->ಅಗಲ() < 64, m_ನಕ್ಷೆ.sampleVarScopep(),
                    "This FSM is too wide to expand - this should be caught earlier");
        ಸ್ಥಿರಾಂಕ uint64_t combinations_ಎಣಿಕೆ = static_cast<ಗಾತ್ರ_t>(1)
                                           << m_ನಕ್ಷೆ.sampleVarScopep()->ಅಗಲ();
        // Rolling trick allows to cut peak memory usage
        ತಾರ್ಕಿಕ undefRolled = ಸುಳ್ಳು;
        ಸ್ವಯಂಚಾಲಿತ rollUndefs = [ಇದು, combinations_ಎಣಿಕೆ] {
            ಗಾಗಿ (uint64_t i = 0; i < combinations_ಎಣಿಕೆ; ++i) {
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಸೇರಿಸು = m_undefined_ಸ್ಥಿತಿಗಳು.ಸೇರಿಸು(i);
                ಆದರೆ (!ಸೇರಿಸು.ಎರಡನೇ) m_undefined_ಸ್ಥಿತಿಗಳು.erase(ಸೇರಿಸು.ಮೊದಲ);
            }
        };
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Fsm_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ statep : m_ಸ್ಥಿತಿಗಳು) {
            ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ = statep->ಮೌಲ್ಯ().ಅಂಕೆ();
            ಸ್ಥಿರಾಂಕ uint64_t ಮೌಲ್ಯ = ಅಂಕೆ.ಗಮ್ಯ_UQuad();
            ಆದರೆ (!m_isComplete) {
                ಆದರೆ (undefRolled) {
                    UASSERT_OBJ(m_undefined_ಸ್ಥಿತಿಗಳು.erase(ಮೌಲ್ಯ) == 1, m_ನಕ್ಷೆ.sampleVarScopep(),
                                "Two states has the same value?");
                } ಇಲ್ಲದಿದ್ದರೆ {
                    UASSERT_OBJ(m_undefined_ಸ್ಥಿತಿಗಳು.ಸೇರಿಸು(ಮೌಲ್ಯ).ಎರಡನೇ, m_ನಕ್ಷೆ.sampleVarScopep(),
                                "Two states has the same value?");
                    ಆದರೆ (m_undefined_ಸ್ಥಿತಿಗಳು.ಗಾತ್ರ_2() * 2 >= combinations_ಎಣಿಕೆ) {
                        undefRolled = ನಿಜ;
                        rollUndefs();
                    }
                }
            }
        }
        ಆದರೆ (!(m_isComplete || undefRolled)) rollUndefs();
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* buildResetCond() ಸ್ಥಿರಾಂಕ {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ refp
            = ಹೊಸ AstVarRef{m_flp, m_ನಕ್ಷೆ.resetCond().varScopep, VAccess::READ};
        ಹಿಂತಿರುಗಿ m_ನಕ್ಷೆ.resetCond().ಸಕ್ರಿಯ_ಕೆಳಗಿನ
                   ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstLogNot{m_flp, refp})
                   : refp;
    }

    ಗಾತ್ರ_t get_Arc_ಎಣಿಕೆ_With_Expansion(ಸ್ಥಿರಾಂಕ VFsmExpandType expand) ಸ್ಥಿರಾಂಕ {
        ಗಾತ್ರ_t arc_ಎಣಿಕೆ_With_Expansion = ~0ULL;
        ಆಯ್ಕೆ (expand) {
        ಸ್ಥಿತಿ VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_ನಕ್ಷೆ.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ VFsmExpandType::AUTO: {
            arc_ಎಣಿಕೆ_With_Expansion = m_ಅಂಚುಗಳು_ಎಣಿಕೆ;
            ಆದರೆ (m_defaultVertexp) {
                UASSERT_OBJ(m_ಸ್ಥಿತಿಗಳು.ಗಾತ್ರ_2() >= m_defaultVertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().ಗಾತ್ರ_2(),
                            m_ನಕ್ಷೆ.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arc_ಎಣಿಕೆ_With_Expansion -= m_defaultVertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().ಗಾತ್ರ_2();
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Fsm_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ statep : m_ಸ್ಥಿತಿಗಳು) {
                    arc_ಎಣಿಕೆ_With_Expansion += static_cast<ಗಾತ್ರ_t>(statep->ಹೊರಹೋಗು_ಅಂಚುಗಳು().ಖಾಲಿಗಳು())
                                             * m_defaultVertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().ಗಾತ್ರ_2();
                }
            }
            ಆದರೆ (m_resetVertexp) {
                UASSERT_OBJ(m_ಸ್ಥಿತಿಗಳು.ಗಾತ್ರ_2() >= m_resetVertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().ಗಾತ್ರ_2(),
                            m_ನಕ್ಷೆ.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arc_ಎಣಿಕೆ_With_Expansion += m_ಸ್ಥಿತಿಗಳು.ಗಾತ್ರ_2() - m_resetVertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().ಗಾತ್ರ_2();
            }
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ VFsmExpandType::AUTO_EXPAND: {
            arc_ಎಣಿಕೆ_With_Expansion = m_ಸ್ಥಿತಿಗಳು.ಗಾತ್ರ_2() * m_ಸ್ಥಿತಿಗಳು.ಗಾತ್ರ_2() - m_ಸ್ಥಿತಿಗಳು.ಗಾತ್ರ_2();
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ VFsmExpandType::ತುಂಬಿದ: {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = m_ನಕ್ಷೆ.sampleVarScopep()->ಅಗಲ();
            // 32 is derived from:
            // tranitionsCount <= std::numeric_limits<uint64_t>::max()
            // (2^width)^2 - (2^width) <= 2^64 - 1
            // (2^width)^2 - (2^width) < 2^64 since we operate on integral values we can trade 1
            // log_2((2^width)^2 - (2^width)) < 64
            // log_2((2^width)^2) < 64 - `(2^width)` is negigible so, it may be dropped
            // 2width < 64
            ಆದರೆ (ಅಗಲ < 32) {
                ಸ್ಥಿರಾಂಕ uint64_t combinations_ಎಣಿಕೆ = static_cast<ಗಾತ್ರ_t>(1) << ಅಗಲ;
                arc_ಎಣಿಕೆ_With_Expansion = combinations_ಎಣಿಕೆ * combinations_ಎಣಿಕೆ - combinations_ಎಣಿಕೆ;
            }
            ನಿಲ್ಲಿಸು;
        }
        }
        ಹಿಂತಿರುಗಿ arc_ಎಣಿಕೆ_With_Expansion;
    }

    AstCoverOtherDecl* buildStateCoverDecl(ಸ್ಥಿರಾಂಕ std::string& label,
                                           ಸ್ಥಿರಾಂಕ std::string& fsm_ಟ್ಯಾಗ್ = "") {
        AstCoverOtherDecl* ಸ್ಥಿರಾಂಕ declp
            = ಹೊಸ AstCoverOtherDecl{m_flp,
                                    "v_fsm_state/" + m_modPrettyName,
                                    m_ನಕ್ಷೆ.stateVarName() + "::" + label,
                                    "",
                                    0,
                                    m_ನಕ್ಷೆ.stateVarName(),
                                    "",
                                    label,
                                    fsm_ಟ್ಯಾಗ್};
        // We run after scoping, so this is declared for one instance only
        declp->ಪ್ರತಿ_Instance(ನಿಜ);
        m_declsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(m_declsp, declp);
        ಹಿಂತಿರುಗಿ declp;
    }

    Ast_ಗಂಟು_Stmt* buildCoverIncStmtp(ಸ್ಥಿರಾಂಕ std::string& fromStateLabel,
                                    ಸ್ಥಿರಾಂಕ std::string& ಗಮ್ಯ_State_Label,
                                    ಸ್ಥಿರಾಂಕ std::string& ತಳ_Label = "") {
        // Whole builder is designed in such a way that this function won't be called twice with
        // the same arguments - but if it will ever change in future, `AstCoverOtherDecl` shall be
        // cached for particular args set
        AstCoverOtherDecl* ಸ್ಥಿರಾಂಕ declp = ಹೊಸ AstCoverOtherDecl{
            m_flp,
            "v_fsm_arc/" + m_modPrettyName,
            m_ನಕ್ಷೆ.stateVarName() + "::" + fromStateLabel + "->" + ಗಮ್ಯ_State_Label,
            "",
            0,
            m_ನಕ್ಷೆ.stateVarName(),
            fromStateLabel,
            ಗಮ್ಯ_State_Label,
            ತಳ_Label};
        // We run after scoping, so this is declared for one instance only
        declp->ಪ್ರತಿ_Instance(ನಿಜ);
        m_declsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(m_declsp, declp);
        ಹಿಂತಿರುಗಿ ಹೊಸ AstCoverInc{m_flp, declp};
    }
    Ast_ಗಂಟು_Stmt* buildExpandedTransitionHandler(ಸ್ಥಿರಾಂಕ std::string& ಗಮ್ಯ_State_Label,
                                                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ considerUndefs,
                                                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ expandOnlyWithTerminalVerticies,
                                                ಸ್ಥಿರಾಂಕ Fsm_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ಗಮ್ಯ_Statep = ಶೂನ್ಯಸೂಚಕ) {
        AstCase* ಸ್ಥಿರಾಂಕ arcCasep = ಹೊಸ AstCase{
            m_flp, VCaseType::CT_CASE, ಹೊಸ AstVarRef{m_flp, m_prevVscp, VAccess::READ}, ಶೂನ್ಯಸೂಚಕ};
        std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ Fsm_State_ಶೃಂಗ*> handledSoureces;
        ಆದರೆ (ಗಮ್ಯ_Statep) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : ಗಮ್ಯ_Statep->ಒಳ_ಅಂಚುಗಳು()) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ Fsm_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fromp = ಅಂಚು.fromp()->cast<Fsm_State_ಶೃಂಗ>()) {
                    ಆದರೆ (!handledSoureces.emplace(fromp).ಎರಡನೇ) ಮುಂದುವರಿ;
                    arcCasep->ಕೂಡಿಸು_Itemsp(ಹೊಸ Ast_Case_ಅಂಶ{
                        m_flp, makeConst(fromp->ಮೌಲ್ಯ().ಅಂಕೆ().ಗಮ್ಯ_UQuad()),
                        (fromp != ಗಮ್ಯ_Statep) ? buildCoverIncStmtp(fromp->label(), ಗಮ್ಯ_State_Label)
                                            : ಶೂನ್ಯಸೂಚಕ});
                }
            }
        }
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Fsm_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fromp : m_ಸ್ಥಿತಿಗಳು) {
            ಆದರೆ (handledSoureces.ಕಂಡುಹಿಡಿ(fromp) != handledSoureces.ಕೊನೆ()) ಮುಂದುವರಿ;
            arcCasep->ಕೂಡಿಸು_Itemsp(ಹೊಸ Ast_Case_ಅಂಶ{
                m_flp, makeConst(fromp->ಮೌಲ್ಯ().ಅಂಕೆ().ಗಮ್ಯ_UQuad()),
                (fromp != ಗಮ್ಯ_Statep
                 && (!expandOnlyWithTerminalVerticies || fromp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().ಖಾಲಿಗಳು()))
                    ? buildCoverIncStmtp(fromp->label(), ಗಮ್ಯ_State_Label, "artificial")
                    : ಶೂನ್ಯಸೂಚಕ});
        }
        ಆದರೆ (!m_isComplete) {
            ಆದರೆ (considerUndefs) {
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ uint64_t from : m_undefined_ಸ್ಥಿತಿಗಳು) {
                    ಸ್ಥಿರಾಂಕ std::string& label = quad_ಗಮ್ಯ_ಬಿಟ್_Stirng_Label(from);
                    arcCasep->ಕೂಡಿಸು_Itemsp(
                        ಹೊಸ Ast_Case_ಅಂಶ{m_flp, makeConst(from),
                                        (label != ಗಮ್ಯ_State_Label)
                                            ? buildCoverIncStmtp(label, ಗಮ್ಯ_State_Label, "artificial")
                                            : ಶೂನ್ಯಸೂಚಕ});
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                arcCasep->ಕೂಡಿಸು_Itemsp(ಹೊಸ Ast_Case_ಅಂಶ{m_flp, ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ});
            }
        }
        ಹಿಂತಿರುಗಿ arcCasep;
    }
    Ast_ಗಂಟು_Stmt* buildUnxpandedTransitionHandler(ಸ್ಥಿರಾಂಕ Fsm_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ಗಮ್ಯ_Statep) {
        AstCase* ಸ್ಥಿರಾಂಕ arcCasep = ಹೊಸ AstCase{
            m_flp, VCaseType::CT_CASE, ಹೊಸ AstVarRef{m_flp, m_prevVscp, VAccess::READ}, ಶೂನ್ಯಸೂಚಕ};
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : ಗಮ್ಯ_Statep->ಒಳ_ಅಂಚುಗಳು()) {
            ಸ್ಥಿರಾಂಕ Fsm_Arc_ಅಂಚು* ಸ್ಥಿರಾಂಕ arcp = ಅಂಚು.as<Fsm_Arc_ಅಂಚು>();
            UASSERT_OBJ(!arcp->isReset(), m_ನಕ್ಷೆ.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_ನಕ್ಷೆ.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            ಸ್ಥಿರಾಂಕ Fsm_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fromp = arcp->fromp()->as<Fsm_State_ಶೃಂಗ>();
            ಆದರೆ (fromp != ಗಮ್ಯ_Statep) {
                arcCasep->ಕೂಡಿಸು_Itemsp(
                    ಹೊಸ Ast_Case_ಅಂಶ{m_flp, makeConst(fromp->ಮೌಲ್ಯ().ಅಂಕೆ().ಗಮ್ಯ_UQuad()),
                                    buildCoverIncStmtp(fromp->label(), ಗಮ್ಯ_Statep->label())});
            }
        }
        arcCasep->ಕೂಡಿಸು_Itemsp(ಹೊಸ Ast_Case_ಅಂಶ{m_flp, ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ});
        ಹಿಂತಿರುಗಿ arcCasep;
    }

ಸಾರ್ವಜನಿಕ:
    FsmCoverIncBuilder(ಸ್ಥಿರಾಂಕ Fsm_ನಕ್ಷೆ& ನಕ್ಷೆ, AstVarScope* ಸ್ಥಿರಾಂಕ prevVscp)
        : m_ನಕ್ಷೆ{ನಕ್ಷೆ}
        , m_prevVscp{prevVscp}
        , m_flp{ನಕ್ಷೆ.fileline()}
        , m_modPrettyName{ನಕ್ಷೆ.scopep()->modp()->prettyName()} {
        init_Existing_ಸ್ಥಿತಿಗಳು();
        AstVarScope* ಸ್ಥಿರಾಂಕ sampleVscp = ನಕ್ಷೆ.sampleVarScopep();
        ಸ್ಥಿರಾಂಕ VFsmExpandType expand
            = ನಕ್ಷೆ.stateVarScopep()->varp()->attrFsmStateExpand() != VFsmExpandType::DEFAULT
                  ? ನಕ್ಷೆ.stateVarScopep()->varp()->attrFsmStateExpand()
                  : v3Global.opt.coverageFsmExpand();

        {
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t arc_ಎಣಿಕೆ_With_Expansion = get_Arc_ಎಣಿಕೆ_With_Expansion(expand);
            ಆದರೆ (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arc_ಎಣಿಕೆ_With_Expansion == 0ULL
                            || arc_ಎಣಿಕೆ_With_Expansion
                                   > static_cast<ಗಾತ್ರ_t>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arc_ಎಣಿಕೆ_With_Expansion);
                ಹಿಂತಿರುಗಿ;
            }
        }

        AstCase* ಸ್ಥಿರಾಂಕ arcCasep
            = ಹೊಸ AstCase{m_flp, VCaseType::CT_CASE,
                          ಹೊಸ AstVarRef{m_flp, ನಕ್ಷೆ.sampleVarScopep(), VAccess::READ}, ಶೂನ್ಯಸೂಚಕ};
        AstCase* ಸ್ಥಿರಾಂಕ initialStateCasep
            = ಹೊಸ AstCase{m_flp, VCaseType::CT_CASE,
                          ಹೊಸ AstVarRef{m_flp, ನಕ್ಷೆ.sampleVarScopep(), VAccess::READ}, ಶೂನ್ಯಸೂಚಕ};

        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ expandAll = expand != VFsmExpandType::AUTO;
        std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ Fsm_State_ಶೃಂಗ*>
            defaultTargets;  // States that have edge incoming from default vertex
        std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ Fsm_State_ಶೃಂಗ*>
            resetTargets;  // States that have edge incoming from reset vertex
        ಆದರೆ (!expandAll) {
            ಆದರೆ (m_defaultVertexp) {
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : m_defaultVertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                    defaultTargets.emplace(ಅಂಚು.ತುದಿ()->as<Fsm_State_ಶೃಂಗ>());
                }
            }
            ಆದರೆ (m_resetVertexp) {
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : m_resetVertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                    resetTargets.emplace(ಅಂಚು.ತುದಿ()->as<Fsm_State_ಶೃಂಗ>());
                }
            }
        }
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ is_ತುಂಬಿದ_Expansion = expand == VFsmExpandType::ತುಂಬಿದ;
        ಆದರೆ (is_ತುಂಬಿದ_Expansion) init_Undefined_ಸ್ಥಿತಿಗಳು();
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Fsm_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ statep : m_ಸ್ಥಿತಿಗಳು) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* ಸ್ಥಿರಾಂಕ declp = buildStateCoverDecl(statep->label());
            initialStateCasep->ಕೂಡಿಸು_Itemsp(
                ಹೊಸ Ast_Case_ಅಂಶ{m_flp, makeConst(statep->ಮೌಲ್ಯ().ಅಂಕೆ().ಗಮ್ಯ_UQuad()),
                                ಹೊಸ AstCoverInc{m_flp, declp, ನಿಜ}});
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ expandAllOrReset
                = expandAll || (resetTargets.ಕಂಡುಹಿಡಿ(statep) != resetTargets.ಕೊನೆ());
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ defaultExpantion
                = !expandAllOrReset && (defaultTargets.ಕಂಡುಹಿಡಿ(statep) != defaultTargets.ಕೊನೆ());
            arcCasep->ಕೂಡಿಸು_Itemsp(ಹೊಸ Ast_Case_ಅಂಶ{
                m_flp, makeConst(statep->ಮೌಲ್ಯ().ಅಂಕೆ().ಗಮ್ಯ_UQuad()),
                Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ<Ast_ಗಂಟು_Stmt>(
                    ಹೊಸ AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), is_ತುಂಬಿದ_Expansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        ಆದರೆ (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            ಆದರೆ (is_ತುಂಬಿದ_Expansion) {
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ uint64_t ಮೌಲ್ಯ : m_undefined_ಸ್ಥಿತಿಗಳು) {
                    ಸ್ಥಿರಾಂಕ std::string& label = quad_ಗಮ್ಯ_ಬಿಟ್_Stirng_Label(ಮೌಲ್ಯ);
                    AstCoverOtherDecl* ಸ್ಥಿರಾಂಕ declp = buildStateCoverDecl(label);
                    initialStateCasep->ಕೂಡಿಸು_Itemsp(ಹೊಸ Ast_Case_ಅಂಶ{
                        m_flp, makeConst(ಮೌಲ್ಯ), ಹೊಸ AstCoverInc{m_flp, declp, ನಿಜ}});
                    arcCasep->ಕೂಡಿಸು_Itemsp(
                        ಹೊಸ Ast_Case_ಅಂಶ{m_flp, makeConst(ಮೌಲ್ಯ),
                                        Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ<Ast_ಗಂಟು_Stmt>(
                                            ಹೊಸ AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, ನಿಜ, ಸುಳ್ಳು))});
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                // No expansion
                arcCasep->ಕೂಡಿಸು_Itemsp(ಹೊಸ Ast_Case_ಅಂಶ{m_flp, ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ});
                initialStateCasep->ಕೂಡಿಸು_Itemsp(ಹೊಸ Ast_Case_ಅಂಶ{m_flp, ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ});
            }
        }
        m_coverIncp = arcCasep;
        m_initialCoverIncp = initialStateCasep;
    }
    VL_UNCOPYABLE(FsmCoverIncBuilder);
    ~FsmCoverIncBuilder() = ಪೂರ್ವನಿಯೋಜಿತ;
    Ast_ಗಂಟು_Cover_Decl* getCoverDeclsp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_declsp; }
    Ast_ಗಂಟು_Stmt* getCoverIncp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_coverIncp; }
    Ast_ಗಂಟು_Stmt* getInitialCoverIncp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_initialCoverIncp; }
};

// Lower the completed FSM graphs into the concrete coverage declarations,
// previous-state tracking, and pre/post-triggered instrumentation that the
// runtime uses to record state and transition coverage.
ವರ್ಗ Fsm_ಅಧೋ_Visitor final {
    // STATE - across all visitors
    ಸ್ಥಿರಾಂಕ FsmState& m_state;
    V3SharedTmps m_prevTmps{"__VfsmCovPrev", VVarType::MODULETEMP};  // Previous states

    // METHODS
    // Rebuild the original event control from the saved sense description so
    // post-state coverage sampling runs on the same triggering edges.
    ಸ್ಥಿರ Ast_Sen_ವೃಕ್ಷ* build_Sen_ವೃಕ್ಷ(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<FsmSenDesc>& senses) {
        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ sentreep = ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, ಶೂನ್ಯಸೂಚಕ};
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ FsmSenDesc& sense : senses) {
            Ast_Sen_ಅಂಶ* ಸ್ಥಿರಾಂಕ senItemp
                = ಹೊಸ Ast_Sen_ಅಂಶ{flp, VEdgeType{sense.ಅಂಚು_Type},
                                 ಹೊಸ AstVarRef{flp, sense.varScopep, VAccess::READ}};
            sentreep->ಕೂಡಿಸು_Sensesp(senItemp);
        }
        ಹಿಂತಿರುಗಿ sentreep;
    }

    // Lower one fully detected FSM graph into the concrete coverage machinery
    // used by generated models: declarations, previous-state tracking, and the
    // pre/post-triggered increment logic for states and arcs.
    ಶೂನ್ಯ buildOne(ಸ್ಥಿರಾಂಕ Fsm_ನಕ್ಷೆ& ನಕ್ಷೆ) {
        UINFO(1, "buildOne lowering FSM " << ನಕ್ಷೆ.stateVarName()
                                          << " vertices=" << ನಕ್ಷೆ.vertices().ಗಾತ್ರ_2() << ಸಾಲು_ಅಂತ್ಯ);
        AstAlways* ಸ್ಥಿರಾಂಕ alwaysp = ನಕ್ಷೆ.stateAlwaysp();
        AstScope* ಸ್ಥಿರಾಂಕ scopep = ನಕ್ಷೆ.scopep();
        AstVarScope* ಸ್ಥಿರಾಂಕ stateVscp = ನಕ್ಷೆ.stateVarScopep();
        AstVarScope* ಸ್ಥಿರಾಂಕ sampleVscp = ನಕ್ಷೆ.sampleVarScopep();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = ನಕ್ಷೆ.fileline();
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = scopep->modp();
        AstVarScope* ಸ್ಥಿರಾಂಕ prevVscp
            = m_prevTmps.make(flp, scopep, sampleVscp->dtypep(), stateVscp->varp()->shortName());
        // The saved previous-state temp crosses the scheduler's pre/post split
        // in the same way as Verilator's built-in NBA shadow variables, so keep
        // both vars marked as post-life participants for stable MT ordering.
        stateVscp->optimizeLifePost(ನಿಜ);
        sampleVscp->optimizeLifePost(ನಿಜ);
        prevVscp->optimizeLifePost(ನಿಜ);

        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ initActivep
            = ಹೊಸ Ast_ಸಕ್ರಿಯ{flp, "fsm-coverage-init",
                            ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, ಹೊಸ Ast_Sen_ಅಂಶ{flp, Ast_Sen_ಅಂಶ::Initial{}}}};
        initActivep->sen_ವೃಕ್ಷ_Storep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstInitialStatic{
            flp, ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, prevVscp, VAccess::WRITE},
                               ಹೊಸ AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->ಕೂಡಿಸು_Blocksp(initActivep);

        AstAlwaysPost* ಸ್ಥಿರಾಂಕ covPostp = ಹೊಸ AstAlwaysPost{flp};
        ತಾರ್ಕಿಕ updatePrevAfterPost = ಸುಳ್ಳು;
        ಆದರೆ (alwaysp) {
            // Save the previous state as plain sequential logic at the front of
            // the original always_ff body, then evaluate coverage in post logic
            // after the delayed state update commits. This avoids a scheduler
            // race between a separate AstAlwaysPre task and the real state
            // commit for direct parent-level registers.
            alwaysp->prependStmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, prevVscp, VAccess::WRITE},
                                                 ಹೊಸ AstVarRef{flp, sampleVscp, VAccess::READ}});
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Wrapper-derived register candidates do not have a parent
            // always_ff body to splice into. Sample coverage first, then save
            // the current state for the next clock tick; this survives cell
            // boundary scheduling where the real flop update lives elsewhere.
            updatePrevAfterPost = ನಿಜ;
            prevVscp->varp()->ಗಣ_Ignore_Post_Read();
        }

        {
            // Create and increment proper AstNodeCoverDecls
            ಸ್ಥಿರಾಂಕ FsmCoverIncBuilder coverIncBuilder{ನಕ್ಷೆ, prevVscp};
            modp->ಕೂಡಿಸು_Stmtsp(coverIncBuilder.getCoverDeclsp());
            initActivep->ಕೂಡಿಸು_Stmtsp(coverIncBuilder.getInitialCoverIncp());
            covPostp->ಕೂಡಿಸು_Stmtsp(
                ಹೊಸ AstIf{flp,
                          ಹೊಸ AstNeq{flp, ಹೊಸ AstVarRef{flp, prevVscp, VAccess::READ},
                                     ಹೊಸ AstVarRef{flp, sampleVscp, VAccess::READ}},
                          coverIncBuilder.getCoverIncp()});
        }

        ಆದರೆ (updatePrevAfterPost) {
            covPostp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, prevVscp, VAccess::WRITE},
                                              ಹೊಸ AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ sentreep = build_Sen_ವೃಕ್ಷ(flp, ನಕ್ಷೆ.senses());
        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ಹೊಸ Ast_ಸಕ್ರಿಯ{flp, "fsm-coverage", sentreep};
        activep->sen_ವೃಕ್ಷ_Storep(sentreep);
        scopep->ಕೂಡಿಸು_Blocksp(activep);
        activep->ಕೂಡಿಸು_Stmtsp(covPostp);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    // Lower every detected FSM graph from the shared local state into
    // concrete coverage instrumentation while the saved scoped pointers are
    // still valid in the same pass.
    ಸ್ಪಷ್ಟ Fsm_ಅಧೋ_Visitor(ಸ್ಥಿರಾಂಕ FsmState& state)
        : m_state{state} {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ DetectedFsm& fsm : m_state.fsms()) { buildOne(*fsm.graphp); }
    }
};

}  // namespace

ಶೂನ್ಯ V3FsmDetect::detect(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    FsmState state;
    // Phase 1: recover each supported FSM into a complete graph while the
    // original clocked/case structure is still easy to recognize.
    FsmDetectVisitor detect{state, rootp};
    ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) {
        ಗಾತ್ರ_t ಸೂಚ್ಯಂಕ = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dump_ಟ್ಯಾಗ್(ಸೂಚ್ಯಂಕ++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { Fsm_ಅಧೋ_Visitor ಅಧೋ{state}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("fsm-detect", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
