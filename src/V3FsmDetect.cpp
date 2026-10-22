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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3FsmDetect.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Control.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3SharedTmps.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <cctype>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

ਨਾਮਾਕਾਸ਼ {

// Width-preserving FSM state identity. FSM detection needs a stable key for
// graph vertices and lookup tables, but lowering still needs the original
// folded Verilog value so emitted comparisons keep the correct width and bits.
ਵਰਗ Fsm_State_ਮੁੱਲ final {
    // Hash/equality key only. It deliberately ignores signedness because
    // signed and unsigned constants with the same width and bits denote the
    // same encoded FSM state.
    string m_ਕੁੰਜੀ;  // Canonical "width:value" identity, independent of signedness

    // Semantic value. This is what diagnostics and lowering use when printing
    // values or rebuilding AstConst nodes for instrumentation.
    V3_ਸੰਖਿਆ m_ਅੰਕ;  // Original folded value, preserving width for lowered comparisons

    ਸਥਿਰ string make_ਕੁੰਜੀ(ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ) {
        V3_ਸੰਖਿਆ ਕੁੰਜੀ_ਅੰਕ = ਅੰਕ;
        // Signedness does not change FSM state identity: same width and bits
        // should address the same graph vertex.
        ਕੁੰਜੀ_ਅੰਕ.isSigned(ਝੂਠ);
        ਵਾਪਸ cvt_ਮੰਜ਼ਿਲ_Str(ਕੁੰਜੀ_ਅੰਕ.ਚੌੜਾਈ()) + ":" + ਕੁੰਜੀ_ਅੰਕ.ascii(ਸੱਚ, ਸੱਚ);
    }

ਜਨਤਕ:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    Fsm_State_ਮੁੱਲ()
        : m_ਕੁੰਜੀ{"1:1'h0"}
        , m_ਅੰਕ{static_cast<Ast_ਗੰਢ*>(ਖਾਲੀ_ਸੂਚਕ), 1, 0} {}
    ਸਪਸ਼ਟ Fsm_State_ਮੁੱਲ(ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ)
        : m_ਕੁੰਜੀ{make_ਕੁੰਜੀ(ਅੰਕ)}
        , m_ਅੰਕ{ਅੰਕ} {}

    ਅਚਲ string& ਕੁੰਜੀ() ਅਚਲ { ਵਾਪਸ m_ਕੁੰਜੀ; }
    ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ() ਅਚਲ { ਵਾਪਸ m_ਅੰਕ; }
    string ascii() ਅਚਲ { ਵਾਪਸ m_ਅੰਕ.ascii(ਸੱਚ, ਸੱਚ); }
    string warn_ਪਾਠ() ਅਚਲ {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        ਜੇ (m_ਅੰਕ.ਚੌੜਾਈ() <= 32) ਵਾਪਸ cvt_ਮੰਜ਼ਿਲ_Str(m_ਅੰਕ.ਮੰਜ਼ਿਲ_UInt());
        ਵਾਪਸ ascii();
    }

    ਤਾਰਕਿਕ ਸੰਚਾਲਕ==(ਅਚਲ Fsm_State_ਮੁੱਲ& rhs) ਅਚਲ { ਵਾਪਸ m_ਕੁੰਜੀ == rhs.m_ਕੁੰਜੀ; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
ਬਣਤਰ Fsm_State_ਮੁੱਲ_Hash final {
    ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ Fsm_State_ਮੁੱਲ& ਮੁੱਲ) ਅਚਲ {
        ਵਾਪਸ std::hash<string>{}(ਮੁੱਲ.ਕੁੰਜੀ());
    }
};

// Captures one sensitivity-list entry so the lowering phase can later rebuild
// an active block with the same triggering event control.
ਬਣਤਰ FsmSenDesc final {
    // Encoded edge kind copied from AstSenItem::edgeType() so lowering can
    // rebuild the same trigger semantics on the synthesized coverage block.
    VEdgeType::en ਕਿਨਾਰਾ_Type = static_cast<VEdgeType::en>(0);
    // Triggering signal in the saved scoped AST.
    AstVarScope* varScopep = ਖਾਲੀ_ਸੂਚਕ;
};

// Captures the simple reset predicate shape that survives to this pass after
// earlier normalization so reset arcs can be reconstructed during lowering.
ਬਣਤਰ FsmResetCondDesc final {
    // Reset signal used by the FSM in the saved scoped AST.
    AstVarScope* varScopep = ਖਾਲੀ_ਸੂਚਕ;
    ਤਾਰਕਿਕ ਸਰਗਰਮ_ਨੀਵਾਂ = ਝੂਠ;
};

ਵਰਗ FsmResetArcDesc final {
    Fsm_State_ਮੁੱਲ m_ਮੰਜ਼ਿਲ_ਮੁੱਲ;  // Encoded reset target state.
    Ast_ਗੰਢ* m_nodep = ਖਾਲੀ_ਸੂਚਕ;  // Source node for warnings and emitted metadata.
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_valuep = ਖਾਲੀ_ਸੂਚਕ;  // Expression that provided the reset value.

ਜਨਤਕ:
    FsmResetArcDesc() = ਮੂਲ;
    FsmResetArcDesc(Fsm_State_ਮੁੱਲ ਮੰਜ਼ਿਲ_ਮੁੱਲ, Ast_ਗੰਢ_ਸੌਂਪੋ* nodep)
        : m_ਮੰਜ਼ਿਲ_ਮੁੱਲ{ਮੰਜ਼ਿਲ_ਮੁੱਲ}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(Fsm_State_ਮੁੱਲ ਮੰਜ਼ਿਲ_ਮੁੱਲ, Ast_ਗੰਢ* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep)
        : m_ਮੰਜ਼ਿਲ_ਮੁੱਲ{ਮੰਜ਼ਿਲ_ਮੁੱਲ}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    Fsm_State_ਮੁੱਲ ਮੰਜ਼ਿਲ_ਮੁੱਲ() ਅਚਲ { ਵਾਪਸ m_ਮੰਜ਼ਿਲ_ਮੁੱਲ; }
    Ast_ਗੰਢ* nodep() ਅਚਲ { ਵਾਪਸ m_nodep; }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep() ਅਚਲ { ਵਾਪਸ m_valuep; }
};

ਬਣਤਰ FsmWrapperRoles final {
    string d_ਪੋਰਟ;
    string q_ਪੋਰਟ;
    string clk_ਪੋਰਟ;
    string rst_ਪੋਰਟ;
    string rstValParam;
    ਤਾਰਕਿਕ has_Rst_ਸਰਗਰਮ_ਨੀਵਾਂ = ਝੂਠ;
    ਤਾਰਕਿਕ rst_ਸਰਗਰਮ_ਨੀਵਾਂ = ਝੂਠ;
};

ਸਥਿਰ ਤਾਰਕਿਕ fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, ਅਚਲ string& ਪੋਰਟ_Name,
                                                  ਤਾਰਕਿਕ& ਸਰਗਰਮ_ਨੀਵਾਂ) {
    ਤਾਰਕਿਕ matched = ਝੂਠ;
    cellp->modp()->foreach([&](Ast_Sen_ਵਸਤੂ* itemp) {
        Ast_ਗੰਢ_Var_Ref* ਅਚਲ vrefp = itemp->varrefp();
        ਜੇ (!vrefp) ਵਾਪਸ;
        ਜੇ (vrefp->varp()->name() != ਪੋਰਟ_Name) ਵਾਪਸ;
        ਸਰਗਰਮ_ਨੀਵਾਂ = itemp->ਕਿਨਾਰਾ_Type() == VEdgeType::ET_NEGEDGE;
        matched = ਸੱਚ;
    });
    ਵਾਪਸ matched;
}

ਸਥਿਰ ਅਚਲ V3Control::Fsm_ਰਜਿਸਟਰ_Wrapper* fsm_ਰਜਿਸਟਰ_Wrapper_Desc(AstCell* cellp) {
    Ast_ਗੰਢ_Module* ਅਚਲ modp = cellp->modp();
    ਅਚਲ string origName = modp->origName();
    ਜੇ (ਅਚਲ V3Control::Fsm_ਰਜਿਸਟਰ_Wrapper* ਅਚਲ descp
        = V3Control::get_Fsm_ਰਜਿਸਟਰ_Wrapper(origName)) {
        ਵਾਪਸ descp;
    }
    ਵਾਪਸ V3Control::get_Fsm_ਰਜਿਸਟਰ_Wrapper(modp->prettyDehashOrigOrName());
}

ਸਥਿਰ FsmWrapperRoles rolesFromDesc(ਅਚਲ V3Control::Fsm_ਰਜਿਸਟਰ_Wrapper& desc) {
    FsmWrapperRoles roles;
    roles.d_ਪੋਰਟ = desc.d;
    roles.q_ਪੋਰਟ = desc.q;
    roles.clk_ਪੋਰਟ = desc.clock;
    roles.rst_ਪੋਰਟ = desc.reset;
    roles.rstValParam = desc.reset_ਮੁੱਲ;
    ਵਾਪਸ roles;
}

ਵਰਗ Fsm_ਰਜਿਸਟਰ_Candidate final {
    AstScope* m_scopep = ਖਾਲੀ_ਸੂਚਕ;  // Owning scope for the paired FSM.
    AstAlways* m_alwaysp = ਖਾਲੀ_ਸੂਚਕ;  // Register process that commits the state.
    AstVarScope* m_stateVscp = ਖਾਲੀ_ਸੂਚਕ;  // Registered FSM state variable.
    AstVarScope* m_sampleVscp = ਖਾਲੀ_ਸੂਚਕ;  // Variable sampled by lowered coverage logic.
    AstVarScope* m_ਅਗਲਾ_Vscp = ਖਾਲੀ_ਸੂਚਕ;  // Next-state variable or same state var for 1-block FSMs.
    std::ਸਦਿਸ਼<FsmSenDesc> m_senses;  // Event controls for recreated coverage blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate, if any.
    std::ਸਦਿਸ਼<FsmResetArcDesc> m_resetArcs;  // Reset target arcs recovered during detect.
    ਤਾਰਕਿਕ m_hasResetCond = ਝੂਠ;  // Whether the FSM had a modeled reset predicate.
    ਤਾਰਕਿਕ m_inclCond = ਝੂਠ;  // Whether conditional/default arcs are kept explicitly.
    File_ਸਤਰ* m_flp = ਖਾਲੀ_ਸੂਚਕ;  // Representative source location.

ਜਨਤਕ:
    AstScope* scopep() ਅਚਲ { ਵਾਪਸ m_scopep; }
    ਖਾਲੀ scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* alwaysp() ਅਚਲ { ਵਾਪਸ m_alwaysp; }
    ਖਾਲੀ alwaysp(AstAlways* alwaysp) { m_alwaysp = alwaysp; }
    AstVarScope* stateVscp() ਅਚਲ { ਵਾਪਸ m_stateVscp; }
    ਖਾਲੀ stateVscp(AstVarScope* vscp) { m_stateVscp = vscp; }
    AstVarScope* sampleVscp() ਅਚਲ { ਵਾਪਸ m_sampleVscp ? m_sampleVscp : m_stateVscp; }
    ਖਾਲੀ sampleVscp(AstVarScope* vscp) { m_sampleVscp = vscp; }
    AstVarScope* ਅਗਲਾ_Vscp() ਅਚਲ { ਵਾਪਸ m_ਅਗਲਾ_Vscp; }
    ਖਾਲੀ ਅਗਲਾ_Vscp(AstVarScope* vscp) { m_ਅਗਲਾ_Vscp = vscp; }
    ਅਚਲ std::ਸਦਿਸ਼<FsmSenDesc>& senses() ਅਚਲ { ਵਾਪਸ m_senses; }
    std::ਸਦਿਸ਼<FsmSenDesc>& senses() { ਵਾਪਸ m_senses; }
    ਅਚਲ FsmResetCondDesc& resetCond() ਅਚਲ { ਵਾਪਸ m_resetCond; }
    FsmResetCondDesc& resetCond() { ਵਾਪਸ m_resetCond; }
    ਅਚਲ std::ਸਦਿਸ਼<FsmResetArcDesc>& resetArcs() ਅਚਲ { ਵਾਪਸ m_resetArcs; }
    std::ਸਦਿਸ਼<FsmResetArcDesc>& resetArcs() { ਵਾਪਸ m_resetArcs; }
    ਤਾਰਕਿਕ hasResetCond() ਅਚਲ { ਵਾਪਸ m_hasResetCond; }
    ਖਾਲੀ hasResetCond(ਤਾਰਕਿਕ flag) { m_hasResetCond = flag; }
    ਤਾਰਕਿਕ inclCond() ਅਚਲ { ਵਾਪਸ m_inclCond; }
    ਖਾਲੀ inclCond(ਤਾਰਕਿਕ flag) { m_inclCond = flag; }
    File_ਸਤਰ* fileline() ਅਚਲ { ਵਾਪਸ m_flp; }
    ਖਾਲੀ fileline(File_ਸਤਰ* flp) { m_flp = flp; }
};

ਵਰਗ FsmComboAlways final {
    AstScope* ਅਚਲ m_scopep = ਖਾਲੀ_ਸੂਚਕ;  // Owning scope for the combinational process.
    AstAlways* ਅਚਲ m_alwaysp = ਖਾਲੀ_ਸੂਚਕ;  // Candidate transition process.

ਜਨਤਕ:
    FsmComboAlways() = ਮੂਲ;
    FsmComboAlways(AstScope* scopep, AstAlways* alwaysp)
        : m_scopep{scopep}
        , m_alwaysp{alwaysp} {}

    AstScope* scopep() ਅਚਲ { ਵਾਪਸ m_scopep; }
    AstAlways* alwaysp() ਅਚਲ { ਵਾਪਸ m_alwaysp; }
};

ਵਰਗ Fsm_ਗ੍ਰਾਫ;

ਵਰਗ Fsm_ਸ਼ੀਰਸ਼ VL_NOT_FINAL : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Fsm_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)

ਜਨਤਕ:
    ਗਿਣਤੀ ਵਰਗ ਕਿਸਮ_2 : uint8_t { STATE, RESET_ANY, DEFAULT_ANY };

ਨਿਜੀ:
    ਕਿਸਮ_2 m_ਕਿਸਮ;  // State vs synthetic ANY/default vertex role.
    string m_label;  // User-facing state or pseudo-state label.
    Fsm_State_ਮੁੱਲ m_ਮੁੱਲ;  // Encoded state value for real state vertices.

ਸੁਰੱਖਿਅਤ:
    Fsm_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, ਕਿਸਮ_2 ਕਿਸਮ, string label, Fsm_State_ਮੁੱਲ ਮੁੱਲ) VL_MT_DISABLED
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp},
          m_ਕਿਸਮ{ਕਿਸਮ},
          m_label{label},
          m_ਮੁੱਲ{ਮੁੱਲ} {}
    ~Fsm_ਸ਼ੀਰਸ਼() override = ਮੂਲ;

ਜਨਤਕ:
    ਕਿਸਮ_2 ਕਿਸਮ() ਅਚਲ { ਵਾਪਸ m_ਕਿਸਮ; }
    ਤਾਰਕਿਕ isState() ਅਚਲ { ਵਾਪਸ m_ਕਿਸਮ == ਕਿਸਮ_2::STATE; }
    ਤਾਰਕਿਕ isResetAny() ਅਚਲ { ਵਾਪਸ m_ਕਿਸਮ == ਕਿਸਮ_2::RESET_ANY; }
    ਤਾਰਕਿਕ isDefaultAny() ਅਚਲ { ਵਾਪਸ m_ਕਿਸਮ == ਕਿਸਮ_2::DEFAULT_ANY; }
    ਅਚਲ string& label() ਅਚਲ { ਵਾਪਸ m_label; }
    ਅਚਲ Fsm_State_ਮੁੱਲ& ਮੁੱਲ() ਅਚਲ { ਵਾਪਸ m_ਮੁੱਲ; }

    string name() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_label + "=" + m_ਮੁੱਲ.ascii(); }
};

ਵਰਗ Fsm_State_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ Fsm_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Fsm_State_ਸ਼ੀਰਸ਼, Fsm_ਸ਼ੀਰਸ਼)

ਜਨਤਕ:
    Fsm_State_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, string label, Fsm_State_ਮੁੱਲ ਮੁੱਲ) VL_MT_DISABLED
        : Fsm_ਸ਼ੀਰਸ਼{graphp, ਕਿਸਮ_2::STATE, label, ਮੁੱਲ} {}
    ~Fsm_State_ਸ਼ੀਰਸ਼() override = ਮੂਲ;

    string dotColor() ਅਚਲ override { ਵਾਪਸ "lightblue"; }
    string dotShape() ਅਚਲ override { ਵਾਪਸ "ellipse"; }
};

ਵਰਗ Fsm_Pseudo_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ Fsm_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Fsm_Pseudo_ਸ਼ੀਰਸ਼, Fsm_ਸ਼ੀਰਸ਼)

ਜਨਤਕ:
    Fsm_Pseudo_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, ਕਿਸਮ_2 ਕਿਸਮ, string label) VL_MT_DISABLED
        : Fsm_ਸ਼ੀਰਸ਼{graphp, ਕਿਸਮ, label, Fsm_State_ਮੁੱਲ{}} {}
    ~Fsm_Pseudo_ਸ਼ੀਰਸ਼() override = ਮੂਲ;

    string name() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ label(); }
    string dotColor() ਅਚਲ override { ਵਾਪਸ isResetAny() ? "darkgreen" : "orange"; }
    string dotShape() ਅਚਲ override { ਵਾਪਸ "diamond"; }
};

ਵਰਗ Fsm_Arc_ਕਿਨਾਰਾ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ {
    VL_RTTI_IMPL(Fsm_Arc_ਕਿਨਾਰਾ, V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ)
    ਤਾਰਕਿਕ m_isReset = ਝੂਠ;  // Arc originates from the synthetic reset source.
    ਤਾਰਕਿਕ m_isCond = ਝੂਠ;  // Arc came from a conditional next-state split.
    ਤਾਰਕਿਕ m_isDefault = ਝੂਠ;  // Arc represents a case default source.
    File_ਸਤਰ* m_flp = ਖਾਲੀ_ਸੂਚਕ;  // Source location for emitted coverage metadata.

ਜਨਤਕ:
    Fsm_Arc_ਕਿਨਾਰਾ(V3_ਗ੍ਰਾਫ* graphp, Fsm_ਸ਼ੀਰਸ਼* fromp, Fsm_State_ਸ਼ੀਰਸ਼* ਸਿਖਰ, ਤਾਰਕਿਕ isReset, ਤਾਰਕਿਕ isCond,
               ਤਾਰਕਿਕ isDefault, File_ਸਤਰ* flp) VL_MT_DISABLED : V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{graphp, fromp, ਸਿਖਰ, 1},
                                                               m_isReset{isReset},
                                                               m_isCond{isCond},
                                                               m_isDefault{isDefault},
                                                               m_flp{flp} {}
    ~Fsm_Arc_ਕਿਨਾਰਾ() override = ਮੂਲ;

    ਤਾਰਕਿਕ isReset() ਅਚਲ { ਵਾਪਸ m_isReset; }
    ਤਾਰਕਿਕ isCond() ਅਚਲ { ਵਾਪਸ m_isCond; }
    ਤਾਰਕਿਕ isDefault() ਅਚਲ { ਵਾਪਸ m_isDefault; }
    File_ਸਤਰ* fileline() ਅਚਲ { ਵਾਪਸ m_flp; }

    string dotLabel() ਅਚਲ override {
        ਜੇ (m_isReset) ਵਾਪਸ "reset";
        ਜੇ (m_isDefault) ਵਾਪਸ "default";
        ਜੇ (m_isCond) ਵਾਪਸ "cond";
        ਵਾਪਸ "";
    }
    string dotColor() ਅਚਲ override {
        ਜੇ (m_isReset) ਵਾਪਸ "darkgreen";
        ਜੇ (m_isDefault) ਵਾਪਸ "orange";
        ਜੇ (m_isCond) ਵਾਪਸ "blue";
        ਵਾਪਸ "black";
    }
};

// One graph per detected FSM. Graph-level metadata captures the non-graph
// context needed to lower states/arcs back into the AST after detection.
ਵਰਗ Fsm_ਗ੍ਰਾਫ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ {
    AstScope* m_scopep = ਖਾਲੀ_ਸੂਚਕ;  // Owning scoped block for the detected FSM.
    AstAlways* m_stateAlwaysp = ਖਾਲੀ_ਸੂਚਕ;  // Register always block being instrumented.
    string m_stateVarName;  // Pretty state variable name for user-visible output.
    string m_state_Var_ਅੰਦਰੂਨੀ_Name;  // Internal state symbol name for dump tags.
    AstVarScope* m_stateVarScopep = ਖਾਲੀ_ਸੂਚਕ;  // Scoped state variable being tracked.
    AstVarScope* m_sampleVarScopep = ਖਾਲੀ_ਸੂਚਕ;  // Scoped variable sampled by coverage logic.
    std::ਸਦਿਸ਼<FsmSenDesc> m_senses;  // Saved event controls for recreated active blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate shape, if one exists.
    ਤਾਰਕਿਕ m_hasResetCond = ਝੂਠ;  // Whether the detected FSM had a reset branch.
    ਤਾਰਕਿਕ m_inclCond = ਝੂਠ;  // Whether conditional arcs should be kept explicitly.
    File_ਸਤਰ* m_flp = ਖਾਲੀ_ਸੂਚਕ;  // Representative source location for declarations/arcs.
    std::unordered_map<Fsm_State_ਮੁੱਲ, Fsm_State_ਸ਼ੀਰਸ਼*, Fsm_State_ਮੁੱਲ_Hash>
        m_stateVertices;  // Value to state map.
    Fsm_Pseudo_ਸ਼ੀਰਸ਼* m_resetVertexp = ਖਾਲੀ_ਸੂਚਕ;  // Synthetic ANY source for reset arcs.
    Fsm_Pseudo_ਸ਼ੀਰਸ਼* m_defaultVertexp = ਖਾਲੀ_ਸੂਚਕ;  // Synthetic default source for case defaults.

ਜਨਤਕ:
    Fsm_ਗ੍ਰਾਫ() VL_MT_DISABLED
        : m_resetVertexp{ਨਵਾਂ Fsm_Pseudo_ਸ਼ੀਰਸ਼{ਇਹ, Fsm_ਸ਼ੀਰਸ਼::ਕਿਸਮ_2::RESET_ANY, "ANY"}},
          m_defaultVertexp{ਨਵਾਂ Fsm_Pseudo_ਸ਼ੀਰਸ਼{ਇਹ, Fsm_ਸ਼ੀਰਸ਼::ਕਿਸਮ_2::DEFAULT_ANY, "default"}} {}

    AstScope* scopep() ਅਚਲ { ਵਾਪਸ m_scopep; }
    ਖਾਲੀ scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* stateAlwaysp() ਅਚਲ { ਵਾਪਸ m_stateAlwaysp; }
    ਖਾਲੀ stateAlwaysp(AstAlways* alwaysp) { m_stateAlwaysp = alwaysp; }
    ਅਚਲ string& stateVarName() ਅਚਲ { ਵਾਪਸ m_stateVarName; }
    ਖਾਲੀ stateVarName(ਅਚਲ string& name) { m_stateVarName = name; }
    ਅਚਲ string& state_Var_ਅੰਦਰੂਨੀ_Name() ਅਚਲ { ਵਾਪਸ m_state_Var_ਅੰਦਰੂਨੀ_Name; }
    ਖਾਲੀ state_Var_ਅੰਦਰੂਨੀ_Name(ਅਚਲ string& name) { m_state_Var_ਅੰਦਰੂਨੀ_Name = name; }
    AstVarScope* stateVarScopep() ਅਚਲ { ਵਾਪਸ m_stateVarScopep; }
    ਖਾਲੀ stateVarScopep(AstVarScope* vscp) { m_stateVarScopep = vscp; }
    AstVarScope* sampleVarScopep() ਅਚਲ {
        ਵਾਪਸ m_sampleVarScopep ? m_sampleVarScopep : m_stateVarScopep;
    }
    ਖਾਲੀ sampleVarScopep(AstVarScope* vscp) { m_sampleVarScopep = vscp; }
    ਅਚਲ std::ਸਦਿਸ਼<FsmSenDesc>& senses() ਅਚਲ { ਵਾਪਸ m_senses; }
    std::ਸਦਿਸ਼<FsmSenDesc>& senses() { ਵਾਪਸ m_senses; }
    ਅਚਲ FsmResetCondDesc& resetCond() ਅਚਲ { ਵਾਪਸ m_resetCond; }
    FsmResetCondDesc& resetCond() { ਵਾਪਸ m_resetCond; }
    ਤਾਰਕਿਕ hasResetCond() ਅਚਲ { ਵਾਪਸ m_hasResetCond; }
    ਖਾਲੀ hasResetCond(ਤਾਰਕਿਕ flag) { m_hasResetCond = flag; }
    ਤਾਰਕਿਕ inclCond() ਅਚਲ { ਵਾਪਸ m_inclCond; }
    ਖਾਲੀ inclCond(ਤਾਰਕਿਕ flag) { m_inclCond = flag; }
    File_ਸਤਰ* fileline() ਅਚਲ { ਵਾਪਸ m_flp; }
    ਖਾਲੀ fileline(File_ਸਤਰ* flp) { m_flp = flp; }

    Fsm_State_ਸ਼ੀਰਸ਼* ਜੋੜੋ_State_ਸ਼ੀਰਸ਼(string label, Fsm_State_ਮੁੱਲ ਮੁੱਲ) VL_MT_DISABLED {
        Fsm_State_ਸ਼ੀਰਸ਼* ਅਚਲ vertexp = ਨਵਾਂ Fsm_State_ਸ਼ੀਰਸ਼{ਇਹ, label, ਮੁੱਲ};
        m_stateVertices.emplace(ਮੁੱਲ, vertexp);
        ਵਾਪਸ vertexp;
    }
    Fsm_Pseudo_ਸ਼ੀਰਸ਼* reset_Any_ਸ਼ੀਰਸ਼() VL_MT_DISABLED { ਵਾਪਸ m_resetVertexp; }
    Fsm_Pseudo_ਸ਼ੀਰਸ਼* default_Any_ਸ਼ੀਰਸ਼() VL_MT_DISABLED { ਵਾਪਸ m_defaultVertexp; }
    Fsm_Arc_ਕਿਨਾਰਾ* ਜੋੜੋ_Arc(Fsm_State_ਮੁੱਲ from_ਮੁੱਲ, Fsm_State_ਮੁੱਲ ਮੰਜ਼ਿਲ_ਮੁੱਲ, ਤਾਰਕਿਕ isReset, ਤਾਰਕਿਕ isCond,
                       ਤਾਰਕਿਕ isDefault, File_ਸਤਰ* flp) VL_MT_DISABLED {
        Fsm_State_ਸ਼ੀਰਸ਼* ਅਚਲ ਸਿਖਰ = m_stateVertices.at(ਮੰਜ਼ਿਲ_ਮੁੱਲ);
        Fsm_ਸ਼ੀਰਸ਼* fromp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (isReset) {
            fromp = reset_Any_ਸ਼ੀਰਸ਼();
        } ਨਹੀਂਤਾਂ ਜੇ (isDefault) {
            fromp = default_Any_ਸ਼ੀਰਸ਼();
        } ਨਹੀਂਤਾਂ {
            fromp = m_stateVertices.at(from_ਮੁੱਲ);
        }
        ਵਾਪਸ ਨਵਾਂ Fsm_Arc_ਕਿਨਾਰਾ{ਇਹ, fromp, ਸਿਖਰ, isReset, isCond, isDefault, flp};
    }

    string name() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ "FSM "
               + (m_stateVarName.ਖਾਲੀ_ਥਾਵਾਂ() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dump_ਟੈਗ(ਮਾਪ_t ਸੂਚਕਾਂਕ) ਅਚਲ {
        string ਟੈਗ = state_Var_ਅੰਦਰੂਨੀ_Name();
        ਲਈ (ਅੱਖਰ& ch : ਟੈਗ) {
            ਜੇ (!std::ਅੱਖਰ_ਜਾਂ_ਅੰਕ(static_cast<ਬਿਨਾਨਿਸ਼ਾਨੀ ਅੱਖਰ>(ch))) ch = '_';
        }
        ਵਾਪਸ "fsm_" + cvt_ਮੰਜ਼ਿਲ_Str(ਸੂਚਕਾਂਕ) + "_" + ਟੈਗ;
    }
};

ਬਣਤਰ DetectedFsm final {
    std::unique_ptr<Fsm_ਗ੍ਰਾਫ> graphp;  // Extracted graph for one detected FSM candidate.
};

ਬਣਤਰ FsmCaseCandidate final {
    Ast_ਗੰਢ* warnNodep = ਖਾਲੀ_ਸੂਚਕ;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = ਖਾਲੀ_ਸੂਚਕ;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
ਬਣਤਰ FsmStateComparison final {
    AstVarScope* stateVscp = ਖਾਲੀ_ਸੂਚਕ;  // Compared state variable
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep = ਖਾਲੀ_ਸੂਚਕ;  // Compared constant value expression
    Fsm_State_ਮੁੱਲ ਮੁੱਲ;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
ਬਣਤਰ FsmIfBranch final {
    AstIf* ifp = ਖਾਲੀ_ਸੂਚਕ;  // Source if/else-if node
    Ast_ਗੰਢ* stmtsp = ਖਾਲੀ_ਸੂਚਕ;  // Branch body
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep = ਖਾਲੀ_ਸੂਚਕ;  // Source state value expression
    Fsm_State_ਮੁੱਲ from_ਮੁੱਲ;  // Encoded source state value
    ਤਾਰਕਿਕ has_ਸਿਖਰ_Guard = ਝੂਠ;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
ਬਣਤਰ FsmIfChainCandidate final {
    AstIf* ifp = ਖਾਲੀ_ਸੂਚਕ;  // Top-level if-chain node
    AstVarScope* compareVscp = ਖਾਲੀ_ਸੂਚਕ;  // Variable used by every state comparison
    std::ਸਦਿਸ਼<FsmIfBranch> branches;  // Recognized state-dispatch branches
    Ast_ਗੰਢ* defaultStmtsp = ਖਾਲੀ_ਸੂਚਕ;  // Optional final else body
};

// Aliases are accepted only when they are equivalent to spelling the state
// comparison inline; this avoids inferring FSM semantics from arbitrary logic.
ਵਰਤੋਂ FsmAliasMap = std::unordered_map<ਅਚਲ AstVarScope*, FsmStateComparison>;
ਵਰਤੋਂ Fsm_Cell_ਪੋਰਟ_Map = std::unordered_map<string, AstVarScope*>;
ਵਰਤੋਂ Fsm_Cell_ਪੋਰਟ_Alias_Map = std::unordered_map<ਅਚਲ AstCell*, Fsm_Cell_ਪੋਰਟ_Map>;

ਬਣਤਰ StateConstLabel final {
    string ਪਾਠ;
    ਤਾਰਕਿਕ fromParam = ਝੂਠ;
    ਮਾਪ_t state_ਸੂਚਕਾਂਕ = 0;
};

ਬਣਤਰ FsmStateSpace final {
    std::ਸਦਿਸ਼<std::pair<string, Fsm_State_ਮੁੱਲ>> ਹਾਲਤਾਂ;  // User label and encoded value
    std::unordered_map<Fsm_State_ਮੁੱਲ, StateConstLabel, Fsm_State_ਮੁੱਲ_Hash>
        labels;  // Encoded value to label
    AstVar* stateVarp = ਖਾਲੀ_ਸੂਚਕ;  // Tracked FSM state variable
    ਤਾਰਕਿਕ enumBacked = ਝੂਠ;  // Whether states came from an enum declaration
};

// Local shared state between the two adjacent FSM coverage phases. Detection
// fills this with recovered FSM graphs; lowering consumes the completed graphs
// immediately afterward without needing any AST serialization bridge.
ਵਰਗ FsmState final {
    // All detected FSMs in discovery order. This is the only bridge between
    // the adjacent detect and lower phases, so the second phase never needs to
    // rediscover or serialize the extracted machine.
    std::ਸਦਿਸ਼<DetectedFsm> m_fsms;
    std::map<ਅਚਲ AstVarScope*, ਮਾਪ_t> m_fsm_ਸੂਚਕਾਂਕ;

ਜਨਤਕ:
    DetectedFsm& fsmFor(AstVarScope* stateVscp) {
        ਅਚਲ std::map<ਅਚਲ AstVarScope*, ਮਾਪ_t>::const_iterator it = m_fsm_ਸੂਚਕਾਂਕ.ਲੱਭੋ(stateVscp);
        ਜੇ (it != m_fsm_ਸੂਚਕਾਂਕ.ਸਿਰਾ()) ਵਾਪਸ m_fsms.at(it->ਦੂਜਾ);
        ਅਚਲ ਮਾਪ_t ਸੂਚਕਾਂਕ = m_fsms.ਮਾਪ();
        m_fsm_ਸੂਚਕਾਂਕ.emplace(stateVscp, ਸੂਚਕਾਂਕ);
        m_fsms.emplace_ਪਿੱਠ();
        ਵਾਪਸ m_fsms.ਪਿੱਠ();
    }
    ਅਚਲ std::ਸਦਿਸ਼<DetectedFsm>& fsms() ਅਚਲ { ਵਾਪਸ m_fsms; }
};

// Detection runs while the original clocked/case structure is still intact and
// populates graph-backed FSM models without mutating the tree mid-traversal.
// This pass is intentionally conservative: for this PR we only lock down the
// small set of transition/selector forms that are already stable in the
// normalized AST we see here. The remaining reject branches are therefore
// mostly future-feature boundaries, not accidental dead code.
ਵਰਗ FsmDetectVisitor final : ਜਨਤਕ VNVisitor {
    // STATE - for current visit position (use VL_RESTORER)
    FsmState& m_state;
    AstScope* m_scopep = ਖਾਲੀ_ਸੂਚਕ;
    std::ਸਦਿਸ਼<Fsm_ਰਜਿਸਟਰ_Candidate> m_ਰਜਿਸਟਰ_Candidates;
    // Deferring one-block detection avoids making continuous alias support
    // depend on whether the assign appears before or after the always block.
    std::ਸਦਿਸ਼<FsmComboAlways> m_one_ਖੰਡ_Alwayss;
    std::ਸਦਿਸ਼<FsmComboAlways> m_comboAlwayss;
    std::ਸਦਿਸ਼<FsmComboAlways> m_nonComboAlwayss;
    // Wrapper FSM detection has a second path for designs compiled without
    // inlining. In that shape the state register stays behind an AstCell, so we
    // remember candidate cells and resolve them only after the surrounding
    // transition logic and post-link port wiring have both been seen.
    std::ਸਦਿਸ਼<std::pair<AstScope*, AstCell*>> m_wrapperCells;
    std::unordered_map<ਅਚਲ AstVarScope*, FsmCaseCandidate> m_comboPaired;
    // Continuous aliases are order-independent, while procedural aliases must
    // remain source-order scoped to avoid using assignments not yet executed.
    FsmAliasMap m_stateAliases;
    std::unordered_ਸੈੱਟ<ਅਚਲ AstVarScope*> m_ambiguousStateAliases;
    // A surviving wrapper's semantic d/q relationship is split across the
    // parent scope and the child module scope. This table is the narrow bridge
    // between those scopes: only transparent port aliases are recorded, so the
    // detector does not become a general cross-module dataflow engine.
    Fsm_Cell_ਪੋਰਟ_Alias_Map m_cell_ਪੋਰਟ_Aliases;
    Fsm_Cell_ਪੋਰਟ_Alias_Map m_cell_ਪੋਰਟ_ਬੱਚਾ_Aliases;

    // METHODS
    // Enum-backed FSMs may be wrapped in refs/typedefs; normalize to the
    // underlying enum type before deciding whether a case is a candidate.
    ਸਥਿਰ Ast_ਗੰਢ_DType* unwrapEnumCandidate(Ast_ਗੰਢ_DType* dtypep) {
        ਵਾਪਸ dtypep->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
    }

    ਸਥਿਰ string candidateConflictContext(Ast_ਗੰਢ* laterNodep,
                                           ਅਚਲ FsmCaseCandidate& ਪਹਿਲਾ_Cand) {
        ਵਾਪਸ '\n' + laterNodep->warnContextPrimary() + ਪਹਿਲਾ_Cand.warnNodep->warnOther()
               + "... Location of first supported candidate for "
               + ਪਹਿਲਾ_Cand.stateVscp->prettyNameQ() + '\n'
               + ਪਹਿਲਾ_Cand.warnNodep->warnContextSecondary();
    }

    ਸਥਿਰ ਤਾਰਕਿਕ rejectFsmWrapperCell(AstCell* cellp, ਅਚਲ string& reason) {
        cellp->v3warn(COVERIGN, "Ignoring unsupported: " + reason);
        ਵਾਪਸ ਝੂਠ;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ simple_Param_State_ਮੁੱਲ(AstCell* cellp, ਅਚਲ string& name, Fsm_State_ਮੁੱਲ& ਮੁੱਲ,
                                      Ast_ਗੰਢ_ਵਿਅੰਜਕ*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (Ast_ਗੰਢ* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ਅਚਲ varp = VN_CAST(stmtp, Var);
            ਜੇ (!varp || !varp->isParam() || varp->name() != name) ਜਾਰੀ;
            valuepr = VN_AS(varp->valuep(), ਗੰਢ_ਵਿਅੰਜਕ);
            ਵਾਪਸ const_ਮੁੱਲ_ਸਥਿਤੀ(valuepr, ਮੁੱਲ) == Const_ਮੁੱਲ_ਸਥਿਤੀ::OK;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ ਬੱਚਾ_ਪੋਰਟ_ਵਿੱਚ_Scope(AstVarScope* vscp, AstScope* ਮਾਪਾ_Scopep, AstCell*& cellpr) {
        ਜੇ (!vscp->varp()->isIO()) ਵਾਪਸ ਝੂਠ;
        AstScope* ਅਚਲ scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        ਜੇ (scopep->aboveScopep() != ਮਾਪਾ_Scopep) ਵਾਪਸ ਝੂਠ;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        ਵਾਪਸ ਸੱਚ;
    }

    ਸਥਿਰ AstVarScope* simple_ਸੌਂਪੋ_Var_Scope(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        AstVarRef* ਅਚਲ vrefp = VN_CAST(exprp, VarRef);
        ਵਾਪਸ vrefp ? vrefp->varScopep() : ਖਾਲੀ_ਸੂਚਕ;
    }

    ਖਾਲੀ ਜੋੜੋ_Wrapper_Cell(AstScope* scopep, AstCell* cellp) {
        m_cell_ਪੋਰਟ_Aliases.emplace(cellp, Fsm_Cell_ਪੋਰਟ_Map{});
        m_cell_ਪੋਰਟ_ਬੱਚਾ_Aliases.emplace(cellp, Fsm_Cell_ਪੋਰਟ_Map{});
        ਅਚਲ std::pair<AstScope*, AstCell*> ਵਸਤੂ_3{scopep, cellp};
        ਜੇ (std::ਲੱਭੋ(m_wrapperCells.cbegin(), m_wrapperCells.cend(), ਵਸਤੂ_3)
            != m_wrapperCells.cend()) {
            ਵਾਪਸ;
        }
        m_wrapperCells.emplace_ਪਿੱਠ(ਵਸਤੂ_3);
    }

    ਖਾਲੀ collect_Cell_ਪੋਰਟ_Alias(Ast_ਸੌਂਪੋ_W* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* ਅਚਲ lhsVscp = simple_ਸੌਂਪੋ_Var_Scope(nodep->lhsp());
        AstVarScope* ਅਚਲ rhsVscp = simple_ਸੌਂਪੋ_Var_Scope(nodep->rhsp());
        ਜੇ (!lhsVscp || !rhsVscp) ਵਾਪਸ;
        AstCell* cellp = ਖਾਲੀ_ਸੂਚਕ;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        ਜੇ (ਬੱਚਾ_ਪੋਰਟ_ਵਿੱਚ_Scope(lhsVscp, m_scopep, cellp)) {
            ਜੇ (!fsm_ਰਜਿਸਟਰ_Wrapper_Desc(cellp)) ਵਾਪਸ;
            UASSERT_OBJ(lhsVscp->varp()->is_ਨਿਵੇਸ਼(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cell_ਪੋਰਟ_Aliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cell_ਪੋਰਟ_ਬੱਚਾ_Aliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            ਜੋੜੋ_Wrapper_Cell(m_scopep, cellp);
        } ਨਹੀਂਤਾਂ ਜੇ (ਬੱਚਾ_ਪੋਰਟ_ਵਿੱਚ_Scope(rhsVscp, m_scopep, cellp)) {
            ਜੇ (!fsm_ਰਜਿਸਟਰ_Wrapper_Desc(cellp)) ਵਾਪਸ;
            UASSERT_OBJ(rhsVscp->varp()->isWritable(), nodep,
                        "Child-side port alias rhs should be writable");
            m_cell_ਪੋਰਟ_Aliases[cellp][rhsVscp->varp()->name()] = lhsVscp;
            m_cell_ਪੋਰਟ_ਬੱਚਾ_Aliases[cellp][rhsVscp->varp()->name()] = rhsVscp;
            ਜੋੜੋ_Wrapper_Cell(m_scopep, cellp);
        }
    }

    AstVarScope* roleVarScope(AstCell* cellp, ਅਚਲ string& ਪੋਰਟ_Name) ਅਚਲ {
        // At this point explicit AstPin expressions have been lowered away, so
        // role resolution crosses the wrapper boundary only through the
        // transparent alias table above. This keeps wrapper support aligned with
        // direct-register detection instead of growing into interprocedural FSM
        // inference.
        ਅਚਲ Fsm_Cell_ਪੋਰਟ_Map& ports = m_cell_ਪੋਰਟ_Aliases.at(cellp);
        ਅਚਲ Fsm_Cell_ਪੋਰਟ_Map::const_iterator ਪੋਰਟ_It = ports.ਲੱਭੋ(ਪੋਰਟ_Name);
        ਵਾਪਸ ਪੋਰਟ_It == ports.ਸਿਰਾ() ? ਖਾਲੀ_ਸੂਚਕ : ਪੋਰਟ_It->ਦੂਜਾ;
    }

    AstVarScope* ਬੱਚਾ_Role_Var_Scope(AstCell* cellp, ਅਚਲ string& ਪੋਰਟ_Name) ਅਚਲ {
        ਅਚਲ Fsm_Cell_ਪੋਰਟ_Map& ports = m_cell_ਪੋਰਟ_ਬੱਚਾ_Aliases.at(cellp);
        ਅਚਲ Fsm_Cell_ਪੋਰਟ_Map::const_iterator ਪੋਰਟ_It = ports.ਲੱਭੋ(ਪੋਰਟ_Name);
        ਵਾਪਸ ਪੋਰਟ_It == ports.ਸਿਰਾ() ? ਖਾਲੀ_ਸੂਚਕ : ਪੋਰਟ_It->ਦੂਜਾ;
    }

    ਵਰਗ ਰਜਿਸਟਰ_Always_Analyzer final {
        AstScope* ਅਚਲ m_scopep;  // Scope containing the always block(s) being analyzed

    ਜਨਤਕ:
        ਸਪਸ਼ਟ ਰਜਿਸਟਰ_Always_Analyzer(AstScope* scopep)
            : m_scopep{scopep} {}

        std::ਸਦਿਸ਼<std::pair<AstCase*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*>>
        one_ਖੰਡ_Candidates(AstAlways* alwaysp) ਅਚਲ {
            std::ਸਦਿਸ਼<std::pair<AstCase*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*>> candidates;
            Ast_ਗੰਢ* ਅਚਲ stmtsp = alwaysp->stmtsp();
            ਜੇ (AstIf* ਅਚਲ ਪਹਿਲਾ_Ifp = VN_CAST(stmtsp, If)) {
                ਜੇ (AstCase* ਅਚਲ casep = VN_CAST(ਪਹਿਲਾ_Ifp->elsesp(), Case)) {
                    candidates.emplace_ਪਿੱਠ(casep,
                                            FsmDetectVisitor::isSimpleResetCond(ਪਹਿਲਾ_Ifp->condp())
                                                ? ਪਹਿਲਾ_Ifp->condp()
                                                : ਖਾਲੀ_ਸੂਚਕ);
                }
            }
            ਲਈ (Ast_ਗੰਢ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                ਜੇ (AstCase* ਅਚਲ casep = VN_CAST(nodep, Case))
                    candidates.emplace_ਪਿੱਠ(casep, ਖਾਲੀ_ਸੂਚਕ);
            }
            ਵਾਪਸ candidates;
        }

        std::ਸਦਿਸ਼<std::pair<AstIf*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*>>
        one_ਖੰਡ_If_Candidates(AstAlways* alwaysp) ਅਚਲ {
            std::ਸਦਿਸ਼<std::pair<AstIf*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*>> candidates;
            Ast_ਗੰਢ* ਅਚਲ stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            ਜੇ (AstIf* ਅਚਲ ਪਹਿਲਾ_Ifp = VN_CAST(stmtsp, If)) {
                ਜੇ (AstIf* ਅਚਲ chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(ਪਹਿਲਾ_Ifp->elsesp()), If)) {
                    candidates.emplace_ਪਿੱਠ(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(ਪਹਿਲਾ_Ifp->condp())
                                                ? ਪਹਿਲਾ_Ifp->condp()
                                                : ਖਾਲੀ_ਸੂਚਕ);
                }
            }
            ਲਈ (Ast_ਗੰਢ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                ਜੇ (AstIf* ਅਚਲ ifp = VN_CAST(nodep, If)) candidates.emplace_ਪਿੱਠ(ifp, ਖਾਲੀ_ਸੂਚਕ);
            }
            ਵਾਪਸ candidates;
        }

        ਤਾਰਕਿਕ ਮੇਲ_ਰਜਿਸਟਰ_Candidate(AstAlways* alwaysp, Fsm_ਰਜਿਸਟਰ_Candidate& cand) ਅਚਲ {
            ਵਾਪਸ FsmDetectVisitor::ਮੇਲ_ਰਜਿਸਟਰ_Always(alwaysp, m_scopep, cand);
        }

        ਖਾਲੀ build_One_ਖੰਡ_Candidate(AstAlways* alwaysp, AstVarScope* vscp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* resetCondp,
                                    Fsm_ਰਜਿਸਟਰ_Candidate& reg) ਅਚਲ {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.ਅਗਲਾ_Vscp(vscp);
            reg.senses() = FsmDetectVisitor::describe_Sen_ਰੁੱਖ(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != ਖਾਲੀ_ਸੂਚਕ);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* ਅਚਲ ਪਹਿਲਾ_Ifp = VN_CAST(alwaysp->stmtsp(), If);
            ਜੇ (ਪਹਿਲਾ_Ifp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = ਖਾਲੀ_ਸੂਚਕ;
                ਅਚਲ Reset_ਸੌਂਪੋ_ਸਥਿਤੀ reset_ਸਥਿਤੀ = FsmDetectVisitor::collectConstStateAssigns(
                    ਪਹਿਲਾ_Ifp->thensp(), resetStateVscp, reg.resetArcs());
                ਜੇ (reset_ਸਥਿਤੀ == Reset_ਸੌਂਪੋ_ਸਥਿਤੀ::ਖਾਲੀ_ਥਾਵਾਂ_3) {
                    reg.resetArcs().clear();
                } ਨਹੀਂਤਾਂ ਜੇ (reset_ਸਥਿਤੀ == Reset_ਸੌਂਪੋ_ਸਥਿਤੀ::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().clear();
                    Fsm_State_ਮੁੱਲ reset_ਮੁੱਲ;
                    Ast_ਗੰਢ* ਅਚਲ thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(ਪਹਿਲਾ_Ifp->thensp());
                    UASSERT_OBJ(thenNodep, ਪਹਿਲਾ_Ifp,
                                "one-block reset fallback requires a non-empty reset branch");
                    Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ resetAssp = FsmDetectVisitor::ਸਿੱਧਾ_Const_State_ਸੌਂਪੋ_ਗੰਢ(
                        thenNodep, resetStateVscp, reset_ਮੁੱਲ);
                    ਜੇ (resetAssp && resetStateVscp == vscp) {
                        reg.resetArcs().emplace_ਪਿੱਠ(reset_ਮੁੱਲ, resetAssp);
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (reset_ਸਥਿਤੀ == Reset_ਸੌਂਪੋ_ਸਥਿਤੀ::MULTI_SAME_STATE) {
                    reg.resetArcs().clear();
                }
            }
        }
    };

    ਤਾਰਕਿਕ ਮੇਲ_Fsm_Wrapper_Cell(AstScope* scopep, AstCell* cellp, Fsm_ਰਜਿਸਟਰ_Candidate& cand) ਅਚਲ {
        FsmWrapperRoles roles = rolesFromDesc(*fsm_ਰਜਿਸਟਰ_Wrapper_Desc(cellp));

        AstVarScope* ਅਚਲ ਅਗਲਾ_Vscp = roleVarScope(cellp, roles.d_ਪੋਰਟ);
        AstVarScope* ਅਚਲ stateVscp = roleVarScope(cellp, roles.q_ਪੋਰਟ);
        ਜੇ (!ਅਗਲਾ_Vscp || !stateVscp) {
            ਵਾਪਸ rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* ਅਚਲ clkVscp = roleVarScope(cellp, roles.clk_ਪੋਰਟ);
        ਜੇ (!clkVscp) {
            ਵਾਪਸ rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper instance requires a simple clock connection");
        }

        FsmSenDesc clkSense;
        clkSense.ਕਿਨਾਰਾ_Type = VEdgeType::ET_POSEDGE;
        clkSense.varScopep = clkVscp;
        cand.senses().ਧੱਕੋ_ਪਿੱਠ(clkSense);

        AstVarScope* resetVscp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!roles.rst_ਪੋਰਟ.ਖਾਲੀ_ਥਾਵਾਂ()) resetVscp = roleVarScope(cellp, roles.rst_ਪੋਰਟ);
        ਜੇ (resetVscp) {
            // The descriptor identifies the reset port but not its polarity. Use
            // the wrapper's own event control AST as the contract for sampling
            // the connected parent signal.
            ਤਾਰਕਿਕ inferred_ਸਰਗਰਮ_ਨੀਵਾਂ = ਝੂਠ;
            ਜੇ (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rst_ਪੋਰਟ, inferred_ਸਰਗਰਮ_ਨੀਵਾਂ)) {
                roles.has_Rst_ਸਰਗਰਮ_ਨੀਵਾਂ = ਸੱਚ;
                roles.rst_ਸਰਗਰਮ_ਨੀਵਾਂ = inferred_ਸਰਗਰਮ_ਨੀਵਾਂ;
            }
        }

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* resetValuep = ਖਾਲੀ_ਸੂਚਕ;
        Fsm_State_ਮੁੱਲ reset_ਮੁੱਲ;
        ਅਚਲ ਤਾਰਕਿਕ has_Reset_ਮੁੱਲ
            = !roles.rstValParam.ਖਾਲੀ_ਥਾਵਾਂ()
              && simple_Param_State_ਮੁੱਲ(cellp, roles.rstValParam, reset_ਮੁੱਲ, resetValuep);
        ਜੇ (resetVscp && roles.has_Rst_ਸਰਗਰਮ_ਨੀਵਾਂ && has_Reset_ਮੁੱਲ) {
            FsmSenDesc rstSense;
            rstSense.ਕਿਨਾਰਾ_Type = roles.rst_ਸਰਗਰਮ_ਨੀਵਾਂ ? VEdgeType::ET_NEGEDGE : VEdgeType::ET_POSEDGE;
            rstSense.varScopep = resetVscp;
            cand.senses().ਧੱਕੋ_ਪਿੱਠ(rstSense);
            cand.resetCond().varScopep = resetVscp;
            cand.resetCond().ਸਰਗਰਮ_ਨੀਵਾਂ = roles.rst_ਸਰਗਰਮ_ਨੀਵਾਂ;
            cand.hasResetCond(ਸੱਚ);
            cand.resetArcs().emplace_ਪਿੱਠ(reset_ਮੁੱਲ, cellp, resetValuep);
        } ਨਹੀਂਤਾਂ ਜੇ (!roles.rst_ਪੋਰਟ.ਖਾਲੀ_ਥਾਵਾਂ() || !roles.rstValParam.ਖਾਲੀ_ਥਾਵਾਂ()) {
            string reason;
            ਜੇ (roles.rst_ਪੋਰਟ.ਖਾਲੀ_ਥਾਵਾਂ()) {
                reason = "reset port is not configured";
            } ਨਹੀਂਤਾਂ ਜੇ (!resetVscp) {
                reason = "reset connection is missing or not a simple variable";
            } ਨਹੀਂਤਾਂ ਜੇ (!roles.has_Rst_ਸਰਗਰਮ_ਨੀਵਾਂ) {
                reason = "reset polarity could not be inferred from the wrapper";
            } ਨਹੀਂਤਾਂ ਜੇ (roles.rstValParam.ਖਾਲੀ_ਥਾਵਾਂ()) {
                reason = "reset_value parameter is not configured";
            } ਨਹੀਂਤਾਂ {
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
        cand.alwaysp(ਖਾਲੀ_ਸੂਚਕ);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(ਬੱਚਾ_Role_Var_Scope(cellp, roles.q_ਪੋਰਟ));
        cand.ਅਗਲਾ_Vscp(ਅਗਲਾ_Vscp);
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(cellp->fileline());
        ਵਾਪਸ ਸੱਚ;
    }

    ਵਰਗ ComboAlwaysAnalyzer final {
    ਜਨਤਕ:
        ਬਣਤਰ Combo_ਮੇਲ final {
            ਅਚਲ Fsm_ਰਜਿਸਟਰ_Candidate* matchedp = ਖਾਲੀ_ਸੂਚਕ;
            Ast_ਗੰਢ* warnNodep = ਖਾਲੀ_ਸੂਚਕ;
        };

    ਨਿਜੀ:
        ਅਚਲ std::ਸਦਿਸ਼<Fsm_ਰਜਿਸਟਰ_Candidate>& m_ਰਜਿਸਟਰ_Candidates;

    ਜਨਤਕ:
        ਸਪਸ਼ਟ ComboAlwaysAnalyzer(ਅਚਲ std::ਸਦਿਸ਼<Fsm_ਰਜਿਸਟਰ_Candidate>& ਰਜਿਸਟਰ_Candidates)
            : m_ਰਜਿਸਟਰ_Candidates{ਰਜਿਸਟਰ_Candidates} {}

        Combo_ਮੇਲ ਮੇਲ_Case(Ast_ਗੰਢ* stmtsp, AstCase* casep) ਅਚਲ {
            Combo_ਮੇਲ ਮੇਲ;
            AstVarRef* ਅਚਲ selp = VN_CAST(casep->exprp(), VarRef);
            ਜੇ (!selp) ਵਾਪਸ ਮੇਲ;
            ਲਈ (ਅਚਲ Fsm_ਰਜਿਸਟਰ_Candidate& reg : m_ਰਜਿਸਟਰ_Candidates) {
                ਜੇ (selp->varScopep() == reg.ਅਗਲਾ_Vscp()) {
                    ਜੇ (!FsmDetectVisitor::has_Canonical_ਅਗਲਾ_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.ਅਗਲਾ_Vscp())) {
                        ਜਾਰੀ;
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (selp->varScopep() != reg.stateVscp()) {
                    ਜਾਰੀ;
                }
                Ast_ਗੰਢ* ਅਚਲ warnNodep = FsmDetectVisitor::case_Supported_Transition_ਗੰਢ(
                    casep, reg.ਅਗਲਾ_Vscp(), reg.inclCond());
                ਜੇ (!warnNodep) ਜਾਰੀ;
                ਮੇਲ.matchedp = &reg;
                ਮੇਲ.warnNodep = warnNodep;
            }
            ਵਾਪਸ ਮੇਲ;
        }

        Combo_ਮੇਲ ਮੇਲ_If_Chain(Ast_ਗੰਢ* stmtsp, ਅਚਲ FsmIfChainCandidate& chain) ਅਚਲ {
            Combo_ਮੇਲ ਮੇਲ;
            ਲਈ (ਅਚਲ Fsm_ਰਜਿਸਟਰ_Candidate& reg : m_ਰਜਿਸਟਰ_Candidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                ਜੇ (chain.compareVscp == reg.ਅਗਲਾ_Vscp()) {
                    ਜੇ (!FsmDetectVisitor::has_Canonical_ਅਗਲਾ_State_Default_Before_Case(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.ਅਗਲਾ_Vscp())) {
                        ਜਾਰੀ;
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (chain.compareVscp != reg.stateVscp()) {
                    ਜਾਰੀ;
                }
                Ast_ਗੰਢ* ਅਚਲ warnNodep
                    = FsmDetectVisitor::if_Chain_Supported_Transition_ਗੰਢ(chain, reg.ਅਗਲਾ_Vscp());
                ਜੇ (!warnNodep) ਜਾਰੀ;
                ਮੇਲ.matchedp = &reg;
                ਮੇਲ.warnNodep = warnNodep;
            }
            ਵਾਪਸ ਮੇਲ;
        }

        ਤਾਰਕਿਕ shouldWarnUnsupported(Ast_ਗੰਢ* stmtsp, AstCase* casep) ਅਚਲ {
            ਅਚਲ AstVarRef* ਅਚਲ selp = VN_CAST(casep->exprp(), VarRef);
            ਜੇ (!selp) ਵਾਪਸ ਝੂਠ;

            ਵਾਪਸ std::any_ਦਾ(
                m_ਰਜਿਸਟਰ_Candidates.cbegin(), m_ਰਜਿਸਟਰ_Candidates.cend(),
                [&](ਅਚਲ Fsm_ਰਜਿਸਟਰ_Candidate& reg) -> ਤਾਰਕਿਕ {
                    ਅਚਲ ਤਾਰਕਿਕ matches_ਅਗਲਾ = selp->varScopep() == reg.ਅਗਲਾ_Vscp();
                    ਅਚਲ ਤਾਰਕਿਕ matchesState = selp->varScopep() == reg.stateVscp();

                    ਜੇ (!matches_ਅਗਲਾ && !matchesState) ਵਾਪਸ ਝੂਠ;
                    ਜੇ (matches_ਅਗਲਾ
                        && !FsmDetectVisitor::has_Canonical_ਅਗਲਾ_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.ਅਗਲਾ_Vscp())) {
                        ਵਾਪਸ ਝੂਠ;
                    }
                    ਵਾਪਸ FsmDetectVisitor::case_Supported_Transition_ਗੰਢ(casep, reg.ਅਗਲਾ_Vscp(),
                                                                         reg.inclCond());
                });
        }
    };

    // Reset arcs are only modeled for the simple signal form that survives to
    // this pass after earlier normalization.
    ਸਥਿਰ ਤਾਰਕਿਕ isSimpleResetCond(Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp) { ਵਾਪਸ VN_IS(condp, VarRef); }

    // Normalize the reset condition into a compact description so the lowering
    // phase can regenerate the same predicate after detection. By the time
    // this pass runs, active-low source forms such as "!rst_n" have already
    // been canonicalized to a positive-condition if/else shape, so only a
    // plain VarRef survives here.
    ਸਥਿਰ FsmResetCondDesc describeResetCond(Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp) {
        FsmResetCondDesc desc;
        ਜੇ (AstVarRef* ਅਚਲ vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        ਵਾਪਸ desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    ਸਥਿਰ std::ਸਦਿਸ਼<FsmSenDesc> describe_Sen_ਰੁੱਖ(Ast_Sen_ਰੁੱਖ* sentreep) {
        std::ਸਦਿਸ਼<FsmSenDesc> senses;
        ਲਈ (Ast_Sen_ਵਸਤੂ* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), Sen_ਵਸਤੂ)) {
            Ast_ਗੰਢ_Var_Ref* ਅਚਲ vrefp = itemp->varrefp();
            ਜੇ (!vrefp) ਜਾਰੀ;
            FsmSenDesc desc;
            desc.ਕਿਨਾਰਾ_Type = itemp->ਕਿਨਾਰਾ_Type().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.ਧੱਕੋ_ਪਿੱਠ(desc);
        }
        ਵਾਪਸ senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    ਸਥਿਰ ਤਾਰਕਿਕ isIgnorableStmt(Ast_ਗੰਢ* nodep) { ਵਾਪਸ VN_IS(nodep, CoverInc); }

    ਸਥਿਰ Ast_ਗੰਢ* skipLeadingIgnorableStmt(Ast_ਗੰਢ* nodep) {
        ਜਦੋਂ (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        ਵਾਪਸ nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    ਸਥਿਰ Ast_ਗੰਢ* singleMeaningfulStmt(Ast_ਗੰਢ* stmtp) {
        Ast_ਗੰਢ* resultp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (Ast_ਗੰਢ* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            ਜੇ (isIgnorableStmt(nodep)) ਜਾਰੀ;
            ਜੇ (resultp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            resultp = nodep;
        }
        ਵਾਪਸ resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    ਸਥਿਰ Ast_ਗੰਢ* singleMeaningfulBranch(Ast_ਗੰਢ* branchp) {
        ਜੇ (!branchp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਵਾਪਸ branchp;
    }

    // By fsm-detect time, non-clocked always @* blocks are already admitted through
    // a missing sentree. This helper therefore only needs to recognize
    // explicit changed-sensitivity lists such as always @(a or b); clocked and
    // event-driven forms remain out of scope.
    ਸਥਿਰ ਤਾਰਕਿਕ isPlainComboSentree(ਅਚਲ Ast_Sen_ਰੁੱਖ* sentreep) {
        UASSERT(sentreep, "plain combo sensitivity check requires a sensitivity tree");
        ਲਈ (ਅਚਲ Ast_Sen_ਵਸਤੂ* senp = sentreep->sensesp(); senp;
             senp = VN_AS(senp->nextp(), Sen_ਵਸਤੂ)) {
            ਜੇ (senp->ਕਿਨਾਰਾ_Type() == VEdgeType::ET_ਬਦਲਿਆ) ਜਾਰੀ;
            ਵਾਪਸ ਝੂਠ;
        }
        ਵਾਪਸ ਸੱਚ;
    }

    ਖਾਲੀ warnUnsupportedComboAlways(ਅਚਲ FsmComboAlways& combo) {
        ਅਚਲ ComboAlwaysAnalyzer analyzer{m_ਰਜਿਸਟਰ_Candidates};
        Ast_ਗੰਢ* ਅਚਲ stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        ਤਾਰਕਿਕ warned = ਝੂਠ;
        ਲਈ (Ast_ਗੰਢ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ਅਚਲ casep = VN_CAST(nodep, Case);
            ਜੇ (!casep) ਜਾਰੀ;
            ਜੇ (analyzer.shouldWarnUnsupported(stmtsp, casep)) {
                casep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-clocked always "
                                        "blocks requires a combinational sensitivity list or "
                                        "always_comb");
                warned = ਸੱਚ;
            }
            ਜੇ (warned) ਰੋਕੋ;
        }
    }

    // Case-item bodies are single subtrees like if/else arms, not statement
    // lists, so unwrap only local begin/end wrappers here rather than walking
    // sibling case items via nextp().
    ਸਥਿਰ Ast_ਗੰਢ_ਸੌਂਪੋ* ਸਿੱਧਾ_State_ਸੌਂਪੋ(Ast_ਗੰਢ* stmtp, AstVarScope* stateVscp) {
        Ast_ਗੰਢ* ਅਚਲ nodep = singleMeaningfulBranch(stmtp);
        ਜੇ (!nodep) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assp = VN_CAST(nodep, ਗੰਢ_ਸੌਂਪੋ);
        ਜੇ (!assp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        AstVarRef* ਅਚਲ vrefp = VN_CAST(assp->lhsp(), VarRef);
        ਜੇ (!vrefp || vrefp->varScopep() != stateVscp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਵਾਪਸ assp;
    }

    ਸਥਿਰ Ast_ਗੰਢ_ਸੌਂਪੋ* ਗੰਢ_State_Var_ਸੌਂਪੋ(Ast_ਗੰਢ* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assp = VN_CAST(nodep, ਗੰਢ_ਸੌਂਪੋ);
        ਜੇ (!assp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        AstVarRef* ਅਚਲ lhsp = VN_CAST(assp->lhsp()->ਅਧਾਰ_Fromp(ਸੱਚ), VarRef);
        AstVarRef* ਅਚਲ rhsp = VN_CAST(assp->rhsp(), VarRef);
        ਜੇ (!rhsp || !lhsp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        ਵਾਪਸ assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    ਸਥਿਰ Ast_ਗੰਢ_ਸੌਂਪੋ* ਸਿੱਧਾ_Cond_State_Var_ਸੌਂਪੋ(Ast_ਗੰਢ* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, Ast_ਗੰਢ_ਵਿਅੰਜਕ*& condp,
                                                   Fsm_State_ਮੁੱਲ& reset_ਮੁੱਲ) {
        Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assp = VN_CAST(nodep, ਗੰਢ_ਸੌਂਪੋ);
        ਜੇ (!assp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        AstVarRef* ਅਚਲ lhsp = VN_CAST(assp->lhsp()->ਅਧਾਰ_Fromp(ਸੱਚ), VarRef);
        AstCond* ਅਚਲ rhsp = VN_CAST(assp->rhsp(), Cond);
        ਜੇ (!rhsp || !lhsp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        AstVarRef* ਅਚਲ elsep = VN_CAST(rhsp->elsep(), VarRef);
        ਜੇ (!elsep) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (const_ਮੁੱਲ_ਸਥਿਤੀ(rhsp->thenp(), reset_ਮੁੱਲ) != Const_ਮੁੱਲ_ਸਥਿਤੀ::OK) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        ਵਾਪਸ assp;
    }

    ਸਥਿਰ Ast_ਗੰਢ_ਸੌਂਪੋ* ਸਿੱਧਾ_Const_State_ਸੌਂਪੋ_ਗੰਢ(Ast_ਗੰਢ* nodep, AstVarScope*& stateVscp,
                                                     Fsm_State_ਮੁੱਲ& ਮੁੱਲ) {
        Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assp = VN_CAST(nodep, ਗੰਢ_ਸੌਂਪੋ);
        ਜੇ (!assp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        AstVarRef* ਅਚਲ lhsp = VN_CAST(assp->lhsp()->ਅਧਾਰ_Fromp(ਸੱਚ), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        ਜੇ (const_ਮੁੱਲ_ਸਥਿਤੀ(assp->rhsp(), ਮੁੱਲ) != Const_ਮੁੱਲ_ਸਥਿਤੀ::OK) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        stateVscp = lhsp->varScopep();
        ਵਾਪਸ assp;
    }

    ਗਿਣਤੀ ਵਰਗ Reset_ਸੌਂਪੋ_ਸਥਿਤੀ : uint8_t {
        ਖਾਲੀ_ਥਾਵਾਂ_3,  // Reset branch had no non-coverage statements.
        NONE,  // Reset branch was not the supported direct-constant shape.
        SINGLE,  // Exactly one supported reset assignment was collected.
        MULTI_SAME_STATE  // Multiple assignments to the same FSM state var; warn and ignore.
    };

    // Reset arcs are only extracted from the single direct-constant form. If
    // user RTL assigns the same state register multiple times in the reset
    // branch, warn and skip reset-arc modeling rather than inventing multiple
    // reset transitions for an odd but legal coding style.
    ਸਥਿਰ Reset_ਸੌਂਪੋ_ਸਥਿਤੀ collectConstStateAssigns(Ast_ਗੰਢ* stmtp, AstVarScope*& stateVscp,
                                                      std::ਸਦਿਸ਼<FsmResetArcDesc>& resetArcs) {
        Ast_ਗੰਢ* nodep = skipLeadingIgnorableStmt(stmtp);
        ਜੇ (!nodep) ਵਾਪਸ Reset_ਸੌਂਪੋ_ਸਥਿਤੀ::ਖਾਲੀ_ਥਾਵਾਂ_3;
        ਲਈ (;; nodep = nodep->nextp()) {
            AstVarScope* ਸੌਂਪੋ_State_Vscp = ਖਾਲੀ_ਸੂਚਕ;
            Fsm_State_ਮੁੱਲ ਮੁੱਲ;
            Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assp = ਸਿੱਧਾ_Const_State_ਸੌਂਪੋ_ਗੰਢ(nodep, ਸੌਂਪੋ_State_Vscp, ਮੁੱਲ);
            ਜੇ (!assp) ਵਾਪਸ Reset_ਸੌਂਪੋ_ਸਥਿਤੀ::NONE;
            ਜੇ (!stateVscp) stateVscp = ਸੌਂਪੋ_State_Vscp;
            ਜੇ (ਸੌਂਪੋ_State_Vscp != stateVscp) ਵਾਪਸ Reset_ਸੌਂਪੋ_ਸਥਿਤੀ::NONE;
            ਜੇ (!resetArcs.ਖਾਲੀ_ਥਾਵਾਂ()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.clear();
                ਵਾਪਸ Reset_ਸੌਂਪੋ_ਸਥਿਤੀ::MULTI_SAME_STATE;
            }
            resetArcs.emplace_ਪਿੱਠ(ਮੁੱਲ, assp);
            ਜੇ (!nodep->nextp()) ਵਾਪਸ Reset_ਸੌਂਪੋ_ਸਥਿਤੀ::SINGLE;
        }
    }

    ਸਥਿਰ ਤਾਰਕਿਕ has_Canonical_ਅਗਲਾ_State_Default_Before_Case(Ast_ਗੰਢ* stmtsp, Ast_ਗੰਢ* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* ਅਗਲਾ_Vscp) {
        Ast_ਗੰਢ* ਅਚਲ bodyp = skipLeadingIgnorableStmt(stmtsp);
        ਤਾਰਕਿਕ sawCanonicalDefault = ਝੂਠ;
        ਲਈ (Ast_ਗੰਢ* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            ਜੇ (nodep == targetp) ਵਾਪਸ sawCanonicalDefault;
            ਜੇ (Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assp = VN_CAST(nodep, ਗੰਢ_ਸੌਂਪੋ)) {
                AstVarRef* ਅਚਲ lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* ਅਚਲ rhsp = VN_CAST(assp->rhsp(), VarRef);
                ਜੇ (!lhsp || lhsp->varScopep() != ਅਗਲਾ_Vscp) ਜਾਰੀ;
                ਜੇ (sawCanonicalDefault) {
                    ਅਚਲ string ਅਗਲਾ_Name = ਅਗਲਾ_Vscp->varp()->prettyNameQ();
                    ਅਚਲ string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + ਅਗਲਾ_Name
                                     + ") when the canonical " + ਅਗਲਾ_Name + " = " + stateName
                                     + " default is overwritten before the case statement");
                    ਵਾਪਸ ਝੂਠ;
                }
                ਜੇ (!rhsp || rhsp->varScopep() != stateVscp) ਵਾਪਸ ਝੂਠ;
                sawCanonicalDefault = ਸੱਚ;
            }
        }
    }

    ਸਥਿਰ ਤਾਰਕਿਕ if_State_Const_ਸੌਂਪੋ(Ast_ਗੰਢ* stmtp, AstVarScope* stateVscp,
                                   Fsm_State_ਮੁੱਲ& then_ਮੁੱਲ, Fsm_State_ਮੁੱਲ& else_ਮੁੱਲ) {
        AstIf* ਅਚਲ ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        ਜੇ (!ifp || !ifp->elsesp()) ਵਾਪਸ ਝੂਠ;
        AstVarScope* thenVscp = ਖਾਲੀ_ਸੂਚਕ;
        AstVarScope* elseVscp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ* ਅਚਲ thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        Ast_ਗੰਢ* ਅਚਲ elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        ਜੇ (!thenNodep || !elseNodep) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!ਸਿੱਧਾ_Const_State_ਸੌਂਪੋ_ਗੰਢ(thenNodep, thenVscp, then_ਮੁੱਲ)) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!ਸਿੱਧਾ_Const_State_ਸੌਂਪੋ_ਗੰਢ(elseNodep, elseVscp, else_ਮੁੱਲ)) ਵਾਪਸ ਝੂਠ;
        ਜੇ (thenVscp == stateVscp && elseVscp == stateVscp) ਵਾਪਸ ਸੱਚ;
        ਜੇ (thenVscp != elseVscp) ਵਾਪਸ ਝੂਠ;
        Ast_ਗੰਢ* ਅਚਲ followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = ਖਾਲੀ_ਸੂਚਕ;
        AstVarScope* finalFromVscp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ* ਅਚਲ finalNodep = singleMeaningfulBranch(followp);
        ਜੇ (!finalNodep) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!ਗੰਢ_State_Var_ਸੌਂਪੋ(finalNodep, finalStateVscp, finalFromVscp)) ਵਾਪਸ ਝੂਠ;
        ਜੇ (finalStateVscp != stateVscp) ਵਾਪਸ ਝੂਠ;
        ਜੇ (finalFromVscp != thenVscp) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ ਸੱਚ;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ ਸਿੱਧਾ_State_Cond_Const_ਸੌਂਪੋ(Ast_ਗੰਢ* stmtp, AstVarScope* stateVscp,
                                           Fsm_State_ਮੁੱਲ& then_ਮੁੱਲ, Fsm_State_ਮੁੱਲ& else_ਮੁੱਲ) {
        Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assp = ਸਿੱਧਾ_State_ਸੌਂਪੋ(stmtp, stateVscp);
        ਜੇ (!assp) ਵਾਪਸ ਝੂਠ;
        AstCond* ਅਚਲ condp = VN_CAST(assp->rhsp(), Cond);
        ਜੇ (!condp) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ const_ਮੁੱਲ_ਸਥਿਤੀ(condp->thenp(), then_ਮੁੱਲ) == Const_ਮੁੱਲ_ਸਥਿਤੀ::OK
               && const_ਮੁੱਲ_ਸਥਿਤੀ(condp->elsep(), else_ਮੁੱਲ) == Const_ਮੁੱਲ_ਸਥਿਤੀ::OK;
    }

    ਸਥਿਰ Ast_ਗੰਢ* case_ਵਸਤੂ_Supported_Arc_ਗੰਢ(Ast_Case_ਵਸਤੂ* itemp, AstVarScope* stateVscp,
                                             ਤਾਰਕਿਕ inclCond) {
        ਜੇ (itemp->isDefault()) {
            ਜੇ (!inclCond) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assp = ਸਿੱਧਾ_State_ਸੌਂਪੋ(itemp->stmtsp(), stateVscp);
        ਜੇ (assp) {
            Fsm_State_ਮੁੱਲ ਮੰਜ਼ਿਲ_ਮੁੱਲ;
            ਜੇ (const_ਮੁੱਲ_ਸਥਿਤੀ(assp->rhsp(), ਮੰਜ਼ਿਲ_ਮੁੱਲ) == Const_ਮੁੱਲ_ਸਥਿਤੀ::OK) ਵਾਪਸ assp;
        }
        Fsm_State_ਮੁੱਲ then_ਮੁੱਲ;
        Fsm_State_ਮੁੱਲ else_ਮੁੱਲ;
        ਜੇ (ਸਿੱਧਾ_State_Cond_Const_ਸੌਂਪੋ(itemp->stmtsp(), stateVscp, then_ਮੁੱਲ, else_ਮੁੱਲ)) {
            ਵਾਪਸ assp;
        }
        ਜੇ (if_State_Const_ਸੌਂਪੋ(itemp->stmtsp(), stateVscp, then_ਮੁੱਲ, else_ਮੁੱਲ)) {
            ਵਾਪਸ singleMeaningfulBranch(itemp->stmtsp());
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    ਸਥਿਰ Ast_ਗੰਢ* case_Supported_Transition_ਗੰਢ(AstCase* casep, AstVarScope* stateVscp,
                                                ਤਾਰਕਿਕ inclCond) {
        ਲਈ (Ast_Case_ਵਸਤੂ* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_ਵਸਤੂ)) {
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ nodep = case_ਵਸਤੂ_Supported_Arc_ਗੰਢ(itemp, stateVscp, inclCond))
                ਵਾਪਸ nodep;
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }

    ਸਥਿਰ Ast_ਗੰਢ* case_ਵਸਤੂ_Supported_Arc_ਗੰਢ_Like(Ast_ਗੰਢ* stmtsp, AstVarScope* stateVscp) {
        ਜੇ (Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assp = ਸਿੱਧਾ_State_ਸੌਂਪੋ(stmtsp, stateVscp)) {
            Fsm_State_ਮੁੱਲ ਮੰਜ਼ਿਲ_ਮੁੱਲ;
            ਜੇ (const_ਮੁੱਲ_ਸਥਿਤੀ(assp->rhsp(), ਮੰਜ਼ਿਲ_ਮੁੱਲ) == Const_ਮੁੱਲ_ਸਥਿਤੀ::OK) ਵਾਪਸ assp;
            Fsm_State_ਮੁੱਲ then_ਮੁੱਲ;
            Fsm_State_ਮੁੱਲ else_ਮੁੱਲ;
            ਜੇ (ਸਿੱਧਾ_State_Cond_Const_ਸੌਂਪੋ(stmtsp, stateVscp, then_ਮੁੱਲ, else_ਮੁੱਲ)) ਵਾਪਸ assp;
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }

    ਸਥਿਰ Ast_ਗੰਢ* if_Chain_Supported_Transition_ਗੰਢ(ਅਚਲ FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        ਲਈ (ਮਾਪ_t i = 0; i < chain.branches.ਮਾਪ(); ++i) {
            ਅਚਲ FsmIfBranch& branch = chain.branches[i];
            Ast_ਗੰਢ* ਅਚਲ nodep = case_ਵਸਤੂ_Supported_Arc_ਗੰਢ_Like(branch.stmtsp, stateVscp);
            ਜੇ (!nodep) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        ਵਾਪਸ chain.branches.front().ifp;
    }

    // Prefer user labels in reports. Forced non-enum FSMs prepopulate synthetic
    // labels, so all emitted arcs should already have a known label here.
    ਸਥਿਰ string label_For_ਮੁੱਲ(
        ਅਚਲ std::unordered_map<Fsm_State_ਮੁੱਲ, StateConstLabel, Fsm_State_ਮੁੱਲ_Hash>& labels,
        ਅਚਲ Fsm_State_ਮੁੱਲ& ਮੁੱਲ) {
        ਵਾਪਸ labels.at(ਮੁੱਲ).ਪਾਠ;
    }

    // The extractor only models constant-valued state transitions, and by the
    // time detect runs those values have already been constant-folded.
    ਗਿਣਤੀ ਵਰਗ Const_ਮੁੱਲ_ਸਥਿਤੀ : uint8_t { OK, NOT_CONST, XZ };

    ਸਥਿਰ Const_ਮੁੱਲ_ਸਥਿਤੀ const_ਮੁੱਲ_ਸਥਿਤੀ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Fsm_State_ਮੁੱਲ& ਮੁੱਲ) {
        ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(exprp, Const);
        ਜੇ (!constp) ਵਾਪਸ Const_ਮੁੱਲ_ਸਥਿਤੀ::NOT_CONST;
        ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ = constp->ਅੰਕ();
        ਜੇ (ਅੰਕ.isAnyXZ()) ਵਾਪਸ Const_ਮੁੱਲ_ਸਥਿਤੀ::XZ;
        ਮੁੱਲ = Fsm_State_ਮੁੱਲ{ਅੰਕ};
        ਵਾਪਸ Const_ਮੁੱਲ_ਸਥਿਤੀ::OK;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ ਸ਼ੁੱਧ_State_Comparison_No_Alias(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, FsmStateComparison& cmp) {
        AstEq* ਅਚਲ eqp = VN_CAST(exprp, Eq);
        ਜੇ (!eqp) ਵਾਪਸ ਝੂਠ;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep = eqp->rhsp();
        ਜੇ (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->ਅਧਾਰ_Fromp(ਸੱਚ), VarRef);
            ਜੇ (!vrefp) { ਵਾਪਸ ਝੂਠ; }
            valuep = eqp->lhsp();
        }

        Fsm_State_ਮੁੱਲ ਮੁੱਲ;
        ਜੇ (const_ਮੁੱਲ_ਸਥਿਤੀ(valuep, ਮੁੱਲ) != Const_ਮੁੱਲ_ਸਥਿਤੀ::OK) ਵਾਪਸ ਝੂਠ;
        cmp.stateVscp = vrefp->varScopep();
        cmp.valuep = valuep;
        cmp.ਮੁੱਲ = ਮੁੱਲ;
        ਵਾਪਸ ਸੱਚ;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ ਸ਼ੁੱਧ_State_Comparison(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਅਚਲ FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        ਜੇ (ਸ਼ੁੱਧ_State_Comparison_No_Alias(exprp, cmp)) ਵਾਪਸ ਸੱਚ;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        ਜੇ (AstVarRef* ਅਚਲ vrefp = VN_CAST(exprp, VarRef)) {
            ਅਚਲ FsmAliasMap::const_iterator it = aliases.ਲੱਭੋ(vrefp->varScopep());
            ਜੇ (it == aliases.ਸਿਰਾ()) ਵਾਪਸ ਝੂਠ;
            cmp = it->ਦੂਜਾ;
            ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ supported_ਸਿਖਰ_Level_Guard(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        ਜੇ (VN_IS(exprp, Or)) ਵਾਪਸ ਝੂਠ;
        ਜੇ (VN_IS(exprp, RedAnd)) ਵਾਪਸ ਝੂਠ;
        ਜੇ (VN_IS(exprp, RedOr)) ਵਾਪਸ ਝੂਠ;
        ਜੇ (VN_IS(exprp, RedXor)) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ ਸੱਚ;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ resolveIfPredicate(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਅਚਲ FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, ਤਾਰਕਿਕ& hasGuard) {
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> terms;
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> pending;
        pending.ਧੱਕੋ_ਪਿੱਠ(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        ਜਦੋਂ (!pending.ਖਾਲੀ_ਥਾਵਾਂ()) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ nodep = pending.ਪਿੱਠ();
            pending.ਕੱਢੋ_ਪਿੱਠ();
            ਜੇ (AstAnd* ਅਚਲ andp = VN_CAST(nodep, And)) {
                pending.ਧੱਕੋ_ਪਿੱਠ(andp->rhsp());
                pending.ਧੱਕੋ_ਪਿੱਠ(andp->lhsp());
            } ਨਹੀਂਤਾਂ {
                terms.ਧੱਕੋ_ਪਿੱਠ(nodep);
            }
        }

        ਤਾਰਕਿਕ sawComparison = ਝੂਠ;
        ਲਈ (ਮਾਪ_t i = 0; i < terms.ਮਾਪ(); ++i) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ termp = terms[i];
            FsmStateComparison ਪਦ_Cmp;
            ਜੇ (ਸ਼ੁੱਧ_State_Comparison(termp, aliases, ਪਦ_Cmp /*ref*/)) {
                ਜੇ (sawComparison) ਵਾਪਸ ਝੂਠ;
                cmp = ਪਦ_Cmp;
                sawComparison = ਸੱਚ;
                ਜਾਰੀ;
            }
            ਜੇ (!supported_ਸਿਖਰ_Level_Guard(termp)) ਵਾਪਸ ਝੂਠ;
            hasGuard = ਸੱਚ;
        }
        ਵਾਪਸ sawComparison;
    }

    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Alias(FsmAliasMap& aliases, std::unordered_ਸੈੱਟ<ਅਚਲ AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, ਅਚਲ FsmStateComparison& cmp) {
        ਜੇ (ambiguous.ਲੱਭੋ(aliasVscp) != ambiguous.ਸਿਰਾ()) ਵਾਪਸ;
        ਅਚਲ FsmAliasMap::iterator it = aliases.ਲੱਭੋ(aliasVscp);
        ਜੇ (it == aliases.ਸਿਰਾ()) {
            aliases.emplace(aliasVscp, cmp);
            ਵਾਪਸ;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        ਜੇ (it->ਦੂਜਾ.stateVscp == cmp.stateVscp && it->ਦੂਜਾ.ਮੁੱਲ == cmp.ਮੁੱਲ) ਵਾਪਸ;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        ਵਾਪਸ;
    }

    ਸਥਿਰ ਖਾਲੀ collect_Alias_From_ਸੌਂਪੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* assp, FsmAliasMap& aliases,
                                       std::unordered_ਸੈੱਟ<ਅਚਲ AstVarScope*>& ambiguous) {
        AstVarRef* ਅਚਲ lhsp = VN_CAST(assp->lhsp(), VarRef);
        ਜੇ (!lhsp) ਵਾਪਸ;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        ਜੇ (!ਸ਼ੁੱਧ_State_Comparison_No_Alias(assp->rhsp(), cmp /*ref*/)) ਵਾਪਸ;
        ਜੋੜੋ_Alias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(Ast_ਗੰਢ* stmtsp, Ast_ਗੰਢ* limitp) ਅਚਲ {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_ਸੈੱਟ<ਅਚਲ AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        ਲਈ (Ast_ਗੰਢ* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            ਜੇ (Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assp = VN_CAST(nodep, ਗੰਢ_ਸੌਂਪੋ)) {
                collect_Alias_From_ਸੌਂਪੋ(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        ਲਈ (ਅਚਲ AstVarScope* ਅਚਲ vscp : ambiguous) aliases.erase(vscp);
        ਵਾਪਸ aliases;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ collectIfChain(AstIf* ifp, ਅਚਲ FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_ਸੈੱਟ<string> ਵੇਖਿਆ_ਮੁੱਲਾਂ;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        ਜਦੋਂ (ਸੱਚ) {
            FsmStateComparison cmp;
            ਤਾਰਕਿਕ hasGuard = ਝੂਠ;
            ਜੇ (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) ਵਾਪਸ ਝੂਠ;
            ਜੇ (chain.compareVscp && chain.compareVscp != cmp.stateVscp) ਵਾਪਸ ਝੂਠ;
            ਜੇ (!ਵੇਖਿਆ_ਮੁੱਲਾਂ.ਪਾਓ(cmp.ਮੁੱਲ.ਕੁੰਜੀ()).ਦੂਜਾ) ਵਾਪਸ ਝੂਠ;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.ਧੱਕੋ_ਪਿੱਠ(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.ਮੁੱਲ, hasGuard});

            Ast_ਗੰਢ* ਅਚਲ elseNodep
                = singleMeaningfulBranch(skipLeadingIgnorableStmt(curp->elsesp()));
            ਜੇ (!elseNodep) ਰੋਕੋ;
            ਜੇ (AstIf* ਅਚਲ elseIfp = VN_CAST(elseNodep, If)) {
                curp = elseIfp;
                ਜਾਰੀ;
            }
            chain.defaultStmtsp = elseNodep;
            ਰੋਕੋ;
        }
        ਵਾਪਸ chain.branches.ਮਾਪ() >= 2;
    }

    // Enum-backed FSMs should only use values that were interned as known states.
    // If a constant transition references some other encoding, warn and skip FSM
    // instrumentation for that edge rather than silently dropping it or turning
    // optional coverage into a hard compile failure.
    ਸਥਿਰ ਤਾਰਕਿਕ validate_ਗਿਆਤ_State_ਮੁੱਲ(Ast_ਗੰਢ* nodep, ਅਚਲ FsmStateSpace& stateSpace,
                                        ਅਚਲ Fsm_State_ਮੁੱਲ& ਮੁੱਲ, ਅਚਲ string& role) {
        ਜੇ (stateSpace.labels.ਲੱਭੋ(ਮੁੱਲ) != stateSpace.labels.ਸਿਰਾ()) ਵਾਪਸ ਸੱਚ;
        ਜੇ (stateSpace.enumBacked) {
            ਅਚਲ string enumRole = role == "source" ? "case item value" : "assigned value";
            nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on enum state variable "
                                        + stateSpace.stateVarp->prettyNameQ() + ": " + enumRole
                                        + " " + ਮੁੱਲ.warn_ਪਾਠ()
                                        + " is not present in the declared enum");
            ਵਾਪਸ ਝੂਠ;
        }
        nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum state variable "
                                    + stateSpace.stateVarp->prettyNameQ() + ": " + role + " value "
                                    + ਮੁੱਲ.warn_ਪਾਠ()
                                    + " is not present in the inferred state space");
        ਵਾਪਸ ਝੂਠ;
    }

    ਸਥਿਰ StateConstLabel stateLabelForConst(AstConst* constp) {
        ਅਚਲ string name = constp->origParamName();
        ਜੇ (!name.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ StateConstLabel{Ast_ਗੰਢ::prettyName(name), ਸੱਚ, 0};
        ਵਾਪਸ StateConstLabel{constp->name(), ਝੂਠ, 0};
    }

    ਸਥਿਰ ਖਾਲੀ updateStateLabel(FsmStateSpace& stateSpace, ਅਚਲ Fsm_State_ਮੁੱਲ& ਮੁੱਲ,
                                 ਅਚਲ StateConstLabel& label) {
        stateSpace.ਹਾਲਤਾਂ.at(stateSpace.labels.at(ਮੁੱਲ).state_ਸੂਚਕਾਂਕ).ਪਹਿਲਾ = label.ਪਾਠ;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    ਸਥਿਰ ਤਾਰਕਿਕ ਮੇਲ_ਰਜਿਸਟਰ_Always(AstAlways* alwaysp, AstScope* scopep,
                                    Fsm_ਰਜਿਸਟਰ_Candidate& cand) {
        ਜੇ (!alwaysp->sentreep() || !alwaysp->sentreep()->has_ਕਿਨਾਰਾ()) ਵਾਪਸ ਝੂਠ;

        Ast_ਗੰਢ* ਅਚਲ stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        Ast_ਗੰਢ* ਅਚਲ nodep = singleMeaningfulStmt(stmtsp);
        ਜੇ (!nodep) ਵਾਪਸ ਝੂਠ;

        AstVarScope* stateVscp = ਖਾਲੀ_ਸੂਚਕ;
        AstVarScope* ਅਗਲਾ_Vscp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (AstIf* ਅਚਲ ifp = VN_CAST(nodep, If)) {
            ਜੇ (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) ਵਾਪਸ ਝੂਠ;
            AstVarScope* resetStateVscp = ਖਾਲੀ_ਸੂਚਕ;
            ਅਚਲ Reset_ਸੌਂਪੋ_ਸਥਿਤੀ reset_ਸਥਿਤੀ
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            ਅਚਲ ਤਾਰਕਿਕ ਖਾਲੀ_ਥਾਵਾਂ_Reset_Branch = reset_ਸਥਿਤੀ == Reset_ਸੌਂਪੋ_ਸਥਿਤੀ::ਖਾਲੀ_ਥਾਵਾਂ_3;
            ਜੇ (ਖਾਲੀ_ਥਾਵਾਂ_Reset_Branch) {
                cand.resetArcs().clear();
            } ਨਹੀਂਤਾਂ ਜੇ (reset_ਸਥਿਤੀ == Reset_ਸੌਂਪੋ_ਸਥਿਤੀ::NONE) {
                cand.resetArcs().clear();
                Fsm_State_ਮੁੱਲ reset_ਮੁੱਲ;
                Ast_ਗੰਢ* ਅਚਲ thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ resetAssp
                    = ਸਿੱਧਾ_Const_State_ਸੌਂਪੋ_ਗੰਢ(thenNodep, resetStateVscp, reset_ਮੁੱਲ);
                ਜੇ (!resetAssp) ਵਾਪਸ ਝੂਠ;
                cand.resetArcs().emplace_ਪਿੱਠ(reset_ਮੁੱਲ, resetAssp);
            } ਨਹੀਂਤਾਂ ਜੇ (reset_ਸਥਿਤੀ == Reset_ਸੌਂਪੋ_ਸਥਿਤੀ::MULTI_SAME_STATE) {
                cand.resetArcs().clear();
            }
            Ast_ਗੰਢ* ਅਚਲ elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            ਜੇ (!ਗੰਢ_State_Var_ਸੌਂਪੋ(elseNodep, stateVscp, ਅਗਲਾ_Vscp)) ਵਾਪਸ ਝੂਠ;
            ਜੇ (!ਖਾਲੀ_ਥਾਵਾਂ_Reset_Branch && resetStateVscp != stateVscp) ਵਾਪਸ ਝੂਠ;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != ਖਾਲੀ_ਸੂਚਕ);
        } ਨਹੀਂਤਾਂ {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* resetCondp = ਖਾਲੀ_ਸੂਚਕ;
            Fsm_State_ਮੁੱਲ reset_ਮੁੱਲ;
            // Note the active low 'reset ? next : CONST' form is not accepted
            ਜੇ (Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assp
                = ਸਿੱਧਾ_Cond_State_Var_ਸੌਂਪੋ(nodep, stateVscp, ਅਗਲਾ_Vscp, resetCondp, reset_ਮੁੱਲ)) {
                cand.resetArcs().emplace_ਪਿੱਠ(reset_ਮੁੱਲ, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != ਖਾਲੀ_ਸੂਚਕ);
            } ਨਹੀਂਤਾਂ ਜੇ (!ਗੰਢ_State_Var_ਸੌਂਪੋ(nodep, stateVscp, ਅਗਲਾ_Vscp)) {
                ਵਾਪਸ ਝੂਠ;
            }
        }
        cand.scopep(scopep);
        cand.alwaysp(alwaysp);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(stateVscp);
        cand.ਅਗਲਾ_Vscp(ਅਗਲਾ_Vscp);
        cand.senses() = describe_Sen_ਰੁੱਖ(alwaysp->sentreep());
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(alwaysp->fileline());
        ਵਾਪਸ ਸੱਚ;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ ਜੋੜੋ_ਮੁੱਲ_ਮੰਜ਼ਿਲ_State_Space(Ast_ਗੰਢ* nodep, FsmStateSpace& stateSpace,
                                     ਅਚਲ Fsm_State_ਮੁੱਲ& ਮੁੱਲ, StateConstLabel label) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ labelIt = stateSpace.labels.ਲੱਭੋ(ਮੁੱਲ);
        ਜੇ (labelIt != stateSpace.labels.ਸਿਰਾ()) {
            StateConstLabel& existingLabel = labelIt->ਦੂਜਾ;
            ਜੇ (existingLabel.ਪਾਠ != label.ਪਾਠ && existingLabel.fromParam && label.fromParam) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                        "state variable "
                                            + stateSpace.stateVarp->prettyNameQ()
                                            + " with multiple labels for the same value "
                                            + ਮੁੱਲ.warn_ਪਾਠ() + ": " + existingLabel.ਪਾਠ
                                            + " and " + label.ਪਾਠ);
                ਵਾਪਸ ਝੂਠ;
            }
            ਜੇ (!existingLabel.fromParam && label.fromParam) {
                existingLabel.ਪਾਠ = label.ਪਾਠ;
                existingLabel.fromParam = label.fromParam;
                updateStateLabel(stateSpace, ਮੁੱਲ, label);
            }
        } ਨਹੀਂਤਾਂ {
            StateConstLabel storedLabel = label;
            storedLabel.state_ਸੂਚਕਾਂਕ = stateSpace.ਹਾਲਤਾਂ.ਮਾਪ();
            stateSpace.ਹਾਲਤਾਂ.emplace_ਪਿੱਠ(label.ਪਾਠ, ਮੁੱਲ);
            stateSpace.labels.emplace(ਮੁੱਲ, storedLabel);
        }
        ਵਾਪਸ ਸੱਚ;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    ਸਥਿਰ ਤਾਰਕਿਕ ਜੋੜੋ_ਵਿਅੰਜਕ_ਮੰਜ਼ਿਲ_State_Space(Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_ਮੁੱਲ ਮੁੱਲ;
        ਅਚਲ Const_ਮੁੱਲ_ਸਥਿਤੀ ਸਥਿਤੀ = const_ਮੁੱਲ_ਸਥਿਤੀ(valuep, ਮੁੱਲ);
        ਜੇ (ਸਥਿਤੀ != Const_ਮੁੱਲ_ਸਥਿਤੀ::OK) {
            ਜੇ (ਸਥਿਤੀ == Const_ਮੁੱਲ_ਸਥਿਤੀ::XZ) {
                valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                         "state variable "
                                             + stateSpace.stateVarp->prettyNameQ()
                                             + " with X/Z state encoding values");
            }
            ਵਾਪਸ ਝੂਠ;
        }
        AstConst* ਅਚਲ constp = VN_AS(valuep, Const);
        ਵਾਪਸ ਜੋੜੋ_ਮੁੱਲ_ਮੰਜ਼ਿਲ_State_Space(valuep, stateSpace, ਮੁੱਲ, stateLabelForConst(constp));
    }

    ਸਥਿਰ ਤਾਰਕਿਕ ਜੋੜੋ_Optional_ਨਿਸ਼ਾਨਾ_ਵਿਅੰਜਕ_ਮੰਜ਼ਿਲ_State_Space(Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_ਮੁੱਲ ਮੁੱਲ;
        ਅਚਲ Const_ਮੁੱਲ_ਸਥਿਤੀ ਸਥਿਤੀ = const_ਮੁੱਲ_ਸਥਿਤੀ(valuep, ਮੁੱਲ);
        ਜੇ (ਸਥਿਤੀ != Const_ਮੁੱਲ_ਸਥਿਤੀ::OK) {
            valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                     "state variable "
                                         + stateSpace.stateVarp->prettyNameQ()
                                         + " with non-constant target state values");
            ਵਾਪਸ ਝੂਠ;
        }
        AstConst* ਅਚਲ constp = VN_AS(valuep, Const);
        ਵਾਪਸ ਜੋੜੋ_ਮੁੱਲ_ਮੰਜ਼ਿਲ_State_Space(valuep, stateSpace, ਮੁੱਲ, stateLabelForConst(constp));
    }

    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Reset_Targets_ਮੰਜ਼ਿਲ_State_Space(ਅਚਲ std::ਸਦਿਸ਼<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        ਲਈ (ਅਚਲ FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.ਮੰਜ਼ਿਲ_ਮੁੱਲ().ascii(), ਝੂਠ, 0};
            ਜੇ (AstConst* ਅਚਲ constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                ਜੋੜੋ_ਮੁੱਲ_ਮੰਜ਼ਿਲ_State_Space(resetArc.nodep(), stateSpace, resetArc.ਮੰਜ਼ਿਲ_ਮੁੱਲ(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    ਸਥਿਰ ਤਾਰਕਿਕ collectDeclaredStateSpace(Ast_ਗੰਢ* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, ਤਾਰਕਿਕ& needs_ਸਰੋਤ_ਮੁੱਲਾਂ) {
        AstVar* ਅਚਲ stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        ਜੇ (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        ਅਚਲ ਤਾਰਕਿਕ forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        ਜੇ (enump) {
            stateSpace.enumBacked = ਸੱਚ;
            ਲਈ (Ast_Enum_ਵਸਤੂ* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_ਵਸਤੂ)) {
                ਅਚਲ AstConst* ਅਚਲ constp = VN_AS(itemp->valuep(), Const);
                ਅਚਲ Fsm_State_ਮੁੱਲ ਮੁੱਲ{constp->ਅੰਕ()};
                ਅਚਲ ਮਾਪ_t state_ਸੂਚਕਾਂਕ = stateSpace.ਹਾਲਤਾਂ.ਮਾਪ();
                stateSpace.ਹਾਲਤਾਂ.emplace_ਪਿੱਠ(itemp->name(), ਮੁੱਲ);
                stateSpace.labels.emplace(ਮੁੱਲ,
                                          StateConstLabel{itemp->name(), ਝੂਠ, state_ਸੂਚਕਾਂਕ});
            }
            ਵਾਪਸ stateSpace.ਹਾਲਤਾਂ.ਮਾਪ() >= 2;
        }

        ਜੇ (forced) {
            needs_ਸਰੋਤ_ਮੁੱਲਾਂ = ਸੱਚ;
            ਵਾਪਸ ਸੱਚ;
        }

        needs_ਸਰੋਤ_ਮੁੱਲਾਂ = ਸੱਚ;
        ਵਾਪਸ ਸੱਚ;
    }

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_ValuepVisitor>
    ਸਥਿਰ ਤਾਰਕਿਕ collect_State_Space_From_ਮੁੱਲਾਂ(Ast_ਗੰਢ* warnNodep, AstVarScope* stateVscp,
                                            ਅਚਲ std::ਸਦਿਸ਼<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            ਅਚਲ T_ValuepVisitor& ਦੇਖੋ_Valueps) {
        ਤਾਰਕਿਕ needs_ਸਰੋਤ_ਮੁੱਲਾਂ = ਝੂਠ;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        ਜੇ (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needs_ਸਰੋਤ_ਮੁੱਲਾਂ)) {
            ਵਾਪਸ ਝੂਠ;
        }
        ਜੇ (!needs_ਸਰੋਤ_ਮੁੱਲਾਂ) ਵਾਪਸ ਸੱਚ;
        ਜੋੜੋ_Reset_Targets_ਮੰਜ਼ਿਲ_State_Space(resetArcs, stateSpace);
        ਜੇ (!ਦੇਖੋ_Valueps(
                [&](Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep) { ਵਾਪਸ ਜੋੜੋ_ਵਿਅੰਜਕ_ਮੰਜ਼ਿਲ_State_Space(valuep, stateSpace); })) {
            ਵਾਪਸ ਝੂਠ;
        }
        ਵਾਪਸ stateSpace.ਹਾਲਤਾਂ.ਮਾਪ() >= 2;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* ਸੌਂਪੋ_Vscp,
                                  ਅਚਲ std::ਸਦਿਸ਼<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        ਵਾਪਸ collect_State_Space_From_ਮੁੱਲਾਂ(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, ਸੌਂਪੋ_Vscp, &stateSpace](ਅਚਲ ਸਵੈਚਾਲਿਤ& ਦੇਖੋ_Valuep) {
                ਲਈ (Ast_Case_ਵਸਤੂ* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Case_ਵਸਤੂ)) {
                    ਜੇ (!itemp->isDefault()) {
                        ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), ਗੰਢ_ਵਿਅੰਜਕ)) {
                            ਜੇ (!ਦੇਖੋ_Valuep(condp)) ਵਾਪਸ ਝੂਠ;
                        }
                    }
                    ਜੇ (Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assp
                        = ਸਿੱਧਾ_State_ਸੌਂਪੋ(itemp->stmtsp(), ਸੌਂਪੋ_Vscp)) {
                        Fsm_State_ਮੁੱਲ then_ਮੁੱਲ;
                        Fsm_State_ਮੁੱਲ else_ਮੁੱਲ;
                        AstCond* ਅਚਲ condp = VN_CAST(assp->rhsp(), Cond);
                        ਜੇ (condp
                            && ਸਿੱਧਾ_State_Cond_Const_ਸੌਂਪੋ(itemp->stmtsp(), ਸੌਂਪੋ_Vscp, then_ਮੁੱਲ,
                                                          else_ਮੁੱਲ)) {
                            ਜੇ (!ਦੇਖੋ_Valuep(condp->thenp())) ਵਾਪਸ ਝੂਠ;
                            ਜੇ (!ਦੇਖੋ_Valuep(condp->elsep())) ਵਾਪਸ ਝੂਠ;
                        } ਨਹੀਂਤਾਂ ਜੇ (!ਜੋੜੋ_Optional_ਨਿਸ਼ਾਨਾ_ਵਿਅੰਜਕ_ਮੰਜ਼ਿਲ_State_Space(assp->rhsp(), stateSpace)) {
                            ਵਾਪਸ ਝੂਠ;
                        }
                    }
                }
                ਵਾਪਸ ਸੱਚ;
            });
    }

    ਸਥਿਰ ਤਾਰਕਿਕ collectStateSpace(ਅਚਲ FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* ਸੌਂਪੋ_Vscp,
                                  ਅਚਲ std::ਸਦਿਸ਼<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        ਵਾਪਸ collect_State_Space_From_ਮੁੱਲਾਂ(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, ਸੌਂਪੋ_Vscp, &stateSpace](ਅਚਲ ਸਵੈਚਾਲਿਤ& ਦੇਖੋ_Valuep) {
                ਲਈ (ਅਚਲ FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(ਦੇਖੋ_Valuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assp = ਸਿੱਧਾ_State_ਸੌਂਪੋ(branch.stmtsp, ਸੌਂਪੋ_Vscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    Fsm_State_ਮੁੱਲ then_ਮੁੱਲ;
                    Fsm_State_ਮੁੱਲ else_ਮੁੱਲ;
                    AstCond* ਅਚਲ condp = VN_CAST(assp->rhsp(), Cond);
                    ਜੇ (condp) {
                        UASSERT_OBJ(ਸਿੱਧਾ_State_Cond_Const_ਸੌਂਪੋ(branch.stmtsp, ਸੌਂਪੋ_Vscp,
                                                               then_ਮੁੱਲ, else_ਮੁੱਲ),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        ਜੇ (!ਦੇਖੋ_Valuep(condp->thenp())) ਵਾਪਸ ਝੂਠ;
                        ਜੇ (!ਦੇਖੋ_Valuep(condp->elsep())) ਵਾਪਸ ਝੂਠ;
                    } ਨਹੀਂਤਾਂ ਜੇ (!ਜੋੜੋ_Optional_ਨਿਸ਼ਾਨਾ_ਵਿਅੰਜਕ_ਮੰਜ਼ਿਲ_State_Space(assp->rhsp(), stateSpace)) {
                        ਵਾਪਸ ਝੂਠ;
                    }
                }
                ਵਾਪਸ ਸੱਚ;
            });
    }

    // Extract supported case-item transitions in one place so the conservative
    // policy for direct and ternary forms stays consistent. The false exits in
    // this helper are deliberate subset boundaries: they document shapes we do
    // not yet model in this PR and that future FSM-detection work may widen.
    ਸਥਿਰ ਤਾਰਕਿਕ emit_Case_ਵਸਤੂ_Arcs(Fsm_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ, Ast_Case_ਵਸਤੂ* itemp, AstVarScope* stateVscp,
                                 ਅਚਲ FsmStateSpace& stateSpace, ਤਾਰਕਿਕ inclCond) {
        std::ਸਦਿਸ਼<std::pair<string, Fsm_State_ਮੁੱਲ>> froms;
        ਜੇ (itemp->isDefault()) {
            ਜੇ (!inclCond) ਵਾਪਸ ਝੂਠ;
            froms.emplace_ਪਿੱਠ("default", Fsm_State_ਮੁੱਲ{});
        } ਨਹੀਂਤਾਂ {
            ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), ਗੰਢ_ਵਿਅੰਜਕ)) {
                Fsm_State_ਮੁੱਲ ਮੁੱਲ;
                ਜੇ (const_ਮੁੱਲ_ਸਥਿਤੀ(condp, ਮੁੱਲ) != Const_ਮੁੱਲ_ਸਥਿਤੀ::OK) ਜਾਰੀ;
                ਜੇ (!validate_ਗਿਆਤ_State_ਮੁੱਲ(condp, stateSpace, ਮੁੱਲ, "source")) ਵਾਪਸ ਸੱਚ;
                froms.emplace_ਪਿੱਠ(label_For_ਮੁੱਲ(stateSpace.labels, ਮੁੱਲ), ਮੁੱਲ);
            }
            ਜੇ (froms.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਝੂਠ;
        }

        ਜੇ (Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assp = ਸਿੱਧਾ_State_ਸੌਂਪੋ(itemp->stmtsp(), stateVscp)) {
            Fsm_State_ਮੁੱਲ ਮੰਜ਼ਿਲ_ਮੁੱਲ;
            ਅਚਲ Const_ਮੁੱਲ_ਸਥਿਤੀ ਸਥਿਤੀ = const_ਮੁੱਲ_ਸਥਿਤੀ(assp->rhsp(), ਮੰਜ਼ਿਲ_ਮੁੱਲ);
            ਜੇ (ਸਥਿਤੀ == Const_ਮੁੱਲ_ਸਥਿਤੀ::OK) {
                ਜੇ (!validate_ਗਿਆਤ_State_ਮੁੱਲ(assp, stateSpace, ਮੰਜ਼ਿਲ_ਮੁੱਲ, "target")) ਵਾਪਸ ਸੱਚ;
                ਲਈ (ਅਚਲ std::pair<string, Fsm_State_ਮੁੱਲ>& from : froms) {
                    ਗ੍ਰਾਫ.ਜੋੜੋ_Arc(from.ਦੂਜਾ, ਮੰਜ਼ਿਲ_ਮੁੱਲ, ਝੂਠ, ਝੂਠ, itemp->isDefault(),
                                 assp->fileline());
                }
                ਵਾਪਸ ਸੱਚ;
            }
        }

        Fsm_State_ਮੁੱਲ then_ਮੁੱਲ;
        Fsm_State_ਮੁੱਲ else_ਮੁੱਲ;
        ਜੇ (ਸਿੱਧਾ_State_Cond_Const_ਸੌਂਪੋ(itemp->stmtsp(), stateVscp, then_ਮੁੱਲ, else_ਮੁੱਲ)
            || if_State_Const_ਸੌਂਪੋ(itemp->stmtsp(), stateVscp, then_ਮੁੱਲ, else_ਮੁੱਲ)) {
            ਜੇ (!validate_ਗਿਆਤ_State_ਮੁੱਲ(itemp->stmtsp(), stateSpace, then_ਮੁੱਲ, "target"))
                ਵਾਪਸ ਸੱਚ;
            ਜੇ (!validate_ਗਿਆਤ_State_ਮੁੱਲ(itemp->stmtsp(), stateSpace, else_ਮੁੱਲ, "target"))
                ਵਾਪਸ ਸੱਚ;
            ਲਈ (ਅਚਲ Fsm_State_ਮੁੱਲ& branch_ਮੁੱਲ : {then_ਮੁੱਲ, else_ਮੁੱਲ}) {
                ਲਈ (ਅਚਲ std::pair<string, Fsm_State_ਮੁੱਲ>& from : froms) {
                    ਗ੍ਰਾਫ.ਜੋੜੋ_Arc(from.ਦੂਜਾ, branch_ਮੁੱਲ, ਝੂਠ, ਸੱਚ, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            ਵਾਪਸ ਸੱਚ;
        }

        ਵਾਪਸ ਝੂਠ;
    }

    ਸਥਿਰ ਖਾਲੀ emitStmtArcsFrom(Fsm_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ, Ast_ਗੰਢ* stmtsp, AstVarScope* stateVscp,
                                 ਅਚਲ FsmStateSpace& stateSpace, Fsm_State_ਮੁੱਲ from_ਮੁੱਲ,
                                 ਤਾਰਕਿਕ isDefault, ਤਾਰਕਿਕ forceCond) {
        Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assp = ਸਿੱਧਾ_State_ਸੌਂਪੋ(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        Fsm_State_ਮੁੱਲ ਮੰਜ਼ਿਲ_ਮੁੱਲ;
        ਅਚਲ Const_ਮੁੱਲ_ਸਥਿਤੀ ਸਥਿਤੀ = const_ਮੁੱਲ_ਸਥਿਤੀ(assp->rhsp(), ਮੰਜ਼ਿਲ_ਮੁੱਲ);
        ਜੇ (ਸਥਿਤੀ == Const_ਮੁੱਲ_ਸਥਿਤੀ::OK) {
            ਜੇ (!validate_ਗਿਆਤ_State_ਮੁੱਲ(assp, stateSpace, ਮੰਜ਼ਿਲ_ਮੁੱਲ, "target")) ਵਾਪਸ;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            ਗ੍ਰਾਫ.ਜੋੜੋ_Arc(from_ਮੁੱਲ, ਮੰਜ਼ਿਲ_ਮੁੱਲ, ਝੂਠ, forceCond, isDefault, assp->fileline());
            ਵਾਪਸ;
        }

        Fsm_State_ਮੁੱਲ then_ਮੁੱਲ;
        Fsm_State_ਮੁੱਲ else_ਮੁੱਲ;
        ਅਚਲ ਤਾਰਕਿਕ cond_ਸੌਂਪੋ
            = ਸਿੱਧਾ_State_Cond_Const_ਸੌਂਪੋ(stmtsp, stateVscp, then_ਮੁੱਲ, else_ਮੁੱਲ);
        UASSERT_OBJ(cond_ਸੌਂਪੋ, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        ਜੇ (!validate_ਗਿਆਤ_State_ਮੁੱਲ(stmtsp, stateSpace, then_ਮੁੱਲ, "target")) ਵਾਪਸ;
        ਜੇ (!validate_ਗਿਆਤ_State_ਮੁੱਲ(stmtsp, stateSpace, else_ਮੁੱਲ, "target")) ਵਾਪਸ;
        ਲਈ (ਅਚਲ Fsm_State_ਮੁੱਲ& branch_ਮੁੱਲ : {then_ਮੁੱਲ, else_ਮੁੱਲ}) {
            ਗ੍ਰਾਫ.ਜੋੜੋ_Arc(from_ਮੁੱਲ, branch_ਮੁੱਲ, ਝੂਠ, ਸੱਚ, isDefault, stmtsp->fileline());
        }
    }

    ਸਥਿਰ ਖਾਲੀ emitIfChainArcs(Fsm_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ, ਅਚਲ FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, ਅਚਲ FsmStateSpace& stateSpace) {
        ਲਈ (ਮਾਪ_t i = 0; i < chain.branches.ਮਾਪ(); ++i) {
            ਅਚਲ FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            ਜੇ (!validate_ਗਿਆਤ_State_ਮੁੱਲ(branch.ifp, stateSpace, branch.from_ਮੁੱਲ, "source"))
                ਵਾਪਸ;
            emitStmtArcsFrom(ਗ੍ਰਾਫ, branch.stmtsp, stateVscp, stateSpace, branch.from_ਮੁੱਲ, ਝੂਠ,
                             branch.has_ਸਿਖਰ_Guard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Reset_Arcs(Fsm_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ, ਅਚਲ std::ਸਦਿਸ਼<FsmResetArcDesc>& resetArcs,
                             ਅਚਲ FsmStateSpace& stateSpace) {
        ਲਈ (ਅਚਲ FsmResetArcDesc& resetArc : resetArcs) {
            ਜੇ (!validate_ਗਿਆਤ_State_ਮੁੱਲ(resetArc.nodep(), stateSpace, resetArc.ਮੰਜ਼ਿਲ_ਮੁੱਲ(),
                                         "target"))
                ਜਾਰੀ;
            ਗ੍ਰਾਫ.ਜੋੜੋ_Arc(Fsm_State_ਮੁੱਲ{}, resetArc.ਮੰਜ਼ਿਲ_ਮੁੱਲ(), ਸੱਚ, ਝੂਠ, ਝੂਠ,
                         resetArc.nodep()->fileline());
        }
    }

    // Turn one candidate case statement into the graph representation that the
    // later lowering phase will consume directly, while reviewers can still
    // inspect the extracted machine via DOT dumps.
    ਖਾਲੀ processCase(AstCase* casep, AstVarScope* ਸੌਂਪੋ_Vscp, ਅਚਲ Fsm_ਰਜਿਸਟਰ_Candidate& reg) {
        UASSERT_OBJ(ਸੌਂਪੋ_Vscp, casep, "FSM case processing requires a non-null assignment var");
        AstVarScope* ਅਚਲ stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        ਜੇ (!collectStateSpace(casep, stateVscp, ਸੌਂਪੋ_Vscp, reg.resetArcs(), stateSpace)) ਵਾਪਸ;
        DetectedFsm& ਇੰਦਰਾਜ = m_state.fsmFor(stateVscp);
        ਜੇ (!ਇੰਦਰਾਜ.graphp) {
            ਇੰਦਰਾਜ.graphp.reset(ਨਵਾਂ Fsm_ਗ੍ਰਾਫ{});
            ਇੰਦਰਾਜ.graphp->scopep(reg.scopep());
            ਇੰਦਰਾਜ.graphp->stateAlwaysp(reg.alwaysp());
            ਇੰਦਰਾਜ.graphp->stateVarName(stateVscp->prettyName());
            ਇੰਦਰਾਜ.graphp->state_Var_ਅੰਦਰੂਨੀ_Name(stateVscp->varp()->name());
            ਇੰਦਰਾਜ.graphp->stateVarScopep(stateVscp);
            ਇੰਦਰਾਜ.graphp->sampleVarScopep(reg.sampleVscp());
            ਇੰਦਰਾਜ.graphp->senses() = reg.senses();
            ਇੰਦਰਾਜ.graphp->resetCond() = reg.resetCond();
            ਇੰਦਰਾਜ.graphp->hasResetCond(reg.hasResetCond());
            ਇੰਦਰਾਜ.graphp->inclCond(reg.inclCond());
            ਇੰਦਰਾਜ.graphp->fileline(casep->fileline());
            ਲਈ (ਅਚਲ std::pair<string, Fsm_State_ਮੁੱਲ>& state : stateSpace.ਹਾਲਤਾਂ) {
                ਇੰਦਰਾਜ.graphp->ਜੋੜੋ_State_ਸ਼ੀਰਸ਼(state.ਪਹਿਲਾ, state.ਦੂਜਾ);
            }
            ਜੋੜੋ_Reset_Arcs(*ਇੰਦਰਾਜ.graphp, reg.resetArcs(), stateSpace);
        }
        ਲਈ (Ast_Case_ਵਸਤੂ* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_ਵਸਤੂ)) {
            emit_Case_ਵਸਤੂ_Arcs(*ਇੰਦਰਾਜ.graphp, itemp, ਸੌਂਪੋ_Vscp, stateSpace,
                             ਇੰਦਰਾਜ.graphp->inclCond());
        }
    }

    ਖਾਲੀ processIfChain(ਅਚਲ FsmIfChainCandidate& chain, AstVarScope* ਸੌਂਪੋ_Vscp,
                        ਅਚਲ Fsm_ਰਜਿਸਟਰ_Candidate& reg) {
        UASSERT_OBJ(ਸੌਂਪੋ_Vscp, chain.ifp,
                    "FSM if-chain processing requires a non-null assignment var");
        AstVarScope* ਅਚਲ stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        ਜੇ (!collectStateSpace(chain, stateVscp, ਸੌਂਪੋ_Vscp, reg.resetArcs(), stateSpace)) ਵਾਪਸ;
        DetectedFsm& ਇੰਦਰਾਜ = m_state.fsmFor(stateVscp);
        // Case candidates keep ownership of existing graphs; reaching this path
        // means the if-chain is the only supported dispatch for this FSM.
        UASSERT_OBJ(!ਇੰਦਰਾਜ.graphp, chain.ifp, "FSM if-chain graph should not already exist");
        ਇੰਦਰਾਜ.graphp.reset(ਨਵਾਂ Fsm_ਗ੍ਰਾਫ{});
        ਇੰਦਰਾਜ.graphp->scopep(reg.scopep());
        ਇੰਦਰਾਜ.graphp->stateAlwaysp(reg.alwaysp());
        ਇੰਦਰਾਜ.graphp->stateVarName(stateVscp->prettyName());
        ਇੰਦਰਾਜ.graphp->state_Var_ਅੰਦਰੂਨੀ_Name(stateVscp->varp()->name());
        ਇੰਦਰਾਜ.graphp->stateVarScopep(stateVscp);
        ਇੰਦਰਾਜ.graphp->sampleVarScopep(reg.sampleVscp());
        ਇੰਦਰਾਜ.graphp->senses() = reg.senses();
        ਇੰਦਰਾਜ.graphp->resetCond() = reg.resetCond();
        ਇੰਦਰਾਜ.graphp->hasResetCond(reg.hasResetCond());
        ਇੰਦਰਾਜ.graphp->inclCond(reg.inclCond());
        ਇੰਦਰਾਜ.graphp->fileline(chain.ifp->fileline());
        ਲਈ (ਅਚਲ std::pair<string, Fsm_State_ਮੁੱਲ>& state : stateSpace.ਹਾਲਤਾਂ) {
            ਇੰਦਰਾਜ.graphp->ਜੋੜੋ_State_ਸ਼ੀਰਸ਼(state.ਪਹਿਲਾ, state.ਦੂਜਾ);
        }
        ਜੋੜੋ_Reset_Arcs(*ਇੰਦਰਾਜ.graphp, reg.resetArcs(), stateSpace);
        emitIfChainArcs(*ਇੰਦਰਾਜ.graphp, chain, ਸੌਂਪੋ_Vscp, stateSpace);
    }

    // Find the first supported FSM candidate in a clocked always block, warn on
    // additional candidates, and attach reset arcs when present. Candidate
    // filtering stays narrow on purpose: we prefer to skip ambiguous shapes now
    // and expand detection in a later PR rather than over-infer coverage from
    // forms we do not yet model confidently.
    ਖਾਲੀ process_One_ਖੰਡ_Always(ਅਚਲ FsmComboAlways& one_ਖੰਡ) {
        ਅਚਲ ਰਜਿਸਟਰ_Always_Analyzer analyzer{one_ਖੰਡ.scopep()};
        AstAlways* ਅਚਲ alwaysp = one_ਖੰਡ.alwaysp();
        ਜੇ (!alwaysp->sentreep() || !alwaysp->sentreep()->has_ਕਿਨਾਰਾ()) ਵਾਪਸ;
        ਅਚਲ std::ਸਦਿਸ਼<std::pair<AstCase*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*>> candidates
            = analyzer.one_ਖੰਡ_Candidates(alwaysp);

        FsmCaseCandidate ਪਹਿਲਾ_Cand;
        ਲਈ (ਅਚਲ std::pair<AstCase*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*>& cand : candidates) {
            AstVarRef* ਅਚਲ selp = VN_CAST(cand.ਪਹਿਲਾ->exprp(), VarRef);
            AstVarScope* ਅਚਲ vscp = selp ? selp->varScopep() : ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (!vscp) ਜਾਰੀ;
            ਜੇ (!ਪਹਿਲਾ_Cand.stateVscp) {
                ਪਹਿਲਾ_Cand.warnNodep = cand.ਪਹਿਲਾ;
                ਪਹਿਲਾ_Cand.stateVscp = vscp;
                Fsm_ਰਜਿਸਟਰ_Candidate reg;
                analyzer.build_One_ਖੰਡ_Candidate(alwaysp, vscp, cand.ਦੂਜਾ, reg);
                processCase(cand.ਪਹਿਲਾ, vscp, reg);
            } ਨਹੀਂਤਾਂ ਜੇ (vscp != ਪਹਿਲਾ_Cand.stateVscp) {
                cand.ਪਹਿਲਾ->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed case statements found in "
                                   "the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.ਪਹਿਲਾ, ਪਹਿਲਾ_Cand));
            } ਨਹੀਂਤਾਂ {
                cand.ਪਹਿਲਾ->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported case "
                                   "statements found in the same always block. Only the first "
                                   "candidate will be instrumented."
                                       << candidateConflictContext(cand.ਪਹਿਲਾ, ਪਹਿਲਾ_Cand));
            }
        }

        ਜੇ (ਪਹਿਲਾ_Cand.stateVscp) ਵਾਪਸ;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        ਅਚਲ std::ਸਦਿਸ਼<std::pair<AstIf*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*>> ifCandidates
            = analyzer.one_ਖੰਡ_If_Candidates(alwaysp);
        ਲਈ (ਅਚਲ std::pair<AstIf*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*>& cand : ifCandidates) {
            ਅਚਲ FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.ਪਹਿਲਾ);
            FsmIfChainCandidate chain;
            ਜੇ (!collectIfChain(cand.ਪਹਿਲਾ, aliases, chain)) ਜਾਰੀ;
            AstVarScope* ਅਚਲ vscp = chain.compareVscp;
            ਜੇ (!if_Chain_Supported_Transition_ਗੰਢ(chain, vscp)) ਜਾਰੀ;
            ਜੇ (!ਪਹਿਲਾ_Cand.stateVscp) {
                ਪਹਿਲਾ_Cand.warnNodep = cand.ਪਹਿਲਾ;
                ਪਹਿਲਾ_Cand.stateVscp = vscp;
                Fsm_ਰਜਿਸਟਰ_Candidate reg;
                analyzer.build_One_ਖੰਡ_Candidate(alwaysp, vscp, cand.ਦੂਜਾ, reg);
                processIfChain(chain, vscp, reg);
            } ਨਹੀਂਤਾਂ ਜੇ (vscp != ਪਹਿਲਾ_Cand.stateVscp) {
                cand.ਪਹਿਲਾ->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed transition candidates found "
                                   "in the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.ਪਹਿਲਾ, ਪਹਿਲਾ_Cand));
            } ਨਹੀਂਤਾਂ {
                cand.ਪਹਿਲਾ->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported "
                                   "transition candidates found in the same always block. Only "
                                   "the first candidate will be instrumented."
                                       << candidateConflictContext(cand.ਪਹਿਲਾ, ਪਹਿਲਾ_Cand));
            }
        }
    }

    // Phase 1 two-process pairing scans combinational always blocks only after
    // all strict register candidates have been collected, so source order does
    // not matter.
    ਸਥਿਰ ਖਾਲੀ warnComboSameAlways(Ast_ਗੰਢ* warnNodep, ਅਚਲ FsmCaseCandidate& ਪਹਿਲਾ_Cand) {
        warnNodep->v3warn(FSMMULTI,
                          "FSM coverage: multiple supported transition candidates found in "
                          "the same combinational always block. Only the first candidate "
                          "will be instrumented."
                              << candidateConflictContext(warnNodep, ਪਹਿਲਾ_Cand));
    }

    ਖਾਲੀ processComboAlways(ਅਚਲ FsmComboAlways& combo) {
        ਅਚਲ ComboAlwaysAnalyzer analyzer{m_ਰਜਿਸਟਰ_Candidates};
        Ast_ਗੰਢ* ਅਚਲ stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate ਪਹਿਲਾ_Cand;
        ਲਈ (Ast_ਗੰਢ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ਅਚਲ casep = VN_CAST(nodep, Case);
            ਜੇ (!casep) ਜਾਰੀ;
            ਅਚਲ ComboAlwaysAnalyzer::Combo_ਮੇਲ ਮੇਲ = analyzer.ਮੇਲ_Case(stmtsp, casep);
            ਅਚਲ Fsm_ਰਜਿਸਟਰ_Candidate* ਅਚਲ matchedp = ਮੇਲ.matchedp;
            Ast_ਗੰਢ* ਅਚਲ matchedWarnNodep = ਮੇਲ.warnNodep;
            ਜੇ (!matchedp) ਜਾਰੀ;
            ਜੇ (!ਪਹਿਲਾ_Cand.stateVscp) {
                ਅਚਲ ਸਵੈਚਾਲਿਤ ਪਾਓ_Pair = m_comboPaired.emplace(
                    matchedp->stateVscp(),
                    FsmCaseCandidate{matchedWarnNodep,
                                     const_cast<AstVarScope*>(matchedp->stateVscp())});
                ਜੇ (!ਪਾਓ_Pair.ਦੂਜਾ) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  ਪਾਓ_Pair.ਪਹਿਲਾ->ਦੂਜਾ));
                    ਜਾਰੀ;
                }
                ਪਹਿਲਾ_Cand.warnNodep = matchedWarnNodep;
                ਪਹਿਲਾ_Cand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processCase(casep, matchedp->ਅਗਲਾ_Vscp(), *matchedp);
                ਜਾਰੀ;
            }
            ਜੇ (matchedp->stateVscp() != ਪਹਿਲਾ_Cand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, ਪਹਿਲਾ_Cand);
                ਜਾਰੀ;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported case statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, ਪਹਿਲਾ_Cand));
        }
        ਜੇ (ਪਹਿਲਾ_Cand.stateVscp) ਵਾਪਸ;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        ਲਈ (Ast_ਗੰਢ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* ਅਚਲ ifp = VN_CAST(nodep, If);
            ਜੇ (!ifp) ਜਾਰੀ;
            FsmIfChainCandidate chain;
            ਅਚਲ FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            ਜੇ (!collectIfChain(ifp, aliases, chain)) ਜਾਰੀ;
            ਅਚਲ ComboAlwaysAnalyzer::Combo_ਮੇਲ ਮੇਲ = analyzer.ਮੇਲ_If_Chain(stmtsp, chain);
            ਅਚਲ Fsm_ਰਜਿਸਟਰ_Candidate* ਅਚਲ matchedp = ਮੇਲ.matchedp;
            Ast_ਗੰਢ* ਅਚਲ matchedWarnNodep = ਮੇਲ.warnNodep;
            ਜੇ (!matchedp) ਜਾਰੀ;
            ਜੇ (!ਪਹਿਲਾ_Cand.stateVscp) {
                ਅਚਲ std::pair<std::unordered_map<ਅਚਲ AstVarScope*, FsmCaseCandidate>::iterator,
                                ਤਾਰਕਿਕ>
                    ਪਾਓ_Pair = m_comboPaired.emplace(
                        matchedp->stateVscp(),
                        FsmCaseCandidate{matchedWarnNodep,
                                         const_cast<AstVarScope*>(matchedp->stateVscp())});
                ਜੇ (!ਪਾਓ_Pair.ਦੂਜਾ) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  ਪਾਓ_Pair.ਪਹਿਲਾ->ਦੂਜਾ));
                    ਜਾਰੀ;
                }
                ਪਹਿਲਾ_Cand.warnNodep = matchedWarnNodep;
                ਪਹਿਲਾ_Cand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processIfChain(chain, matchedp->ਅਗਲਾ_Vscp(), *matchedp);
                ਜਾਰੀ;
            }
            ਜੇ (matchedp->stateVscp() != ਪਹਿਲਾ_Cand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, ਪਹਿਲਾ_Cand);
                ਜਾਰੀ;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported if-chain statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, ਪਹਿਲਾ_Cand));
        }
    }

    // Track the current scope so each detected FSM records the module/scope
    // where instrumentation must later be inserted.
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    // Collect processes first, then analyze FSM candidates once all alias and
    // register information is available.
    ਖਾਲੀ ਦੇਖੋ(AstAlways* nodep) override {
        ਜੇ (nodep->keyword() == VAlwaysKwd::CONT_ਸੌਂਪੋ) {
            iterateChildren(nodep);
            ਵਾਪਸ;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_one_ਖੰਡ_Alwayss.emplace_ਪਿੱਠ(m_scopep, nodep);
        ਅਚਲ ਰਜਿਸਟਰ_Always_Analyzer analyzer{m_scopep};
        Fsm_ਰਜਿਸਟਰ_Candidate reg;
        ਜੇ (analyzer.ਮੇਲ_ਰਜਿਸਟਰ_Candidate(nodep, reg)) {
            AstVarScope* ਅਚਲ stateVscp = reg.stateVscp();
            ਅਚਲ ਤਾਰਕਿਕ ਮਿਲਿਆ
                = std::any_ਦਾ(m_ਰਜਿਸਟਰ_Candidates.cbegin(), m_ਰਜਿਸਟਰ_Candidates.cend(),
                              [stateVscp](ਅਚਲ Fsm_ਰਜਿਸਟਰ_Candidate& existing) {
                                  ਵਾਪਸ existing.stateVscp() == stateVscp;
                              });
            ਜੇ (!ਮਿਲਿਆ) { m_ਰਜਿਸਟਰ_Candidates.emplace_ਪਿੱਠ(reg); }
        }
        ਜੇ (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            m_comboAlwayss.emplace_ਪਿੱਠ(m_scopep, nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->keyword() == VAlwaysKwd::ALWAYS) {
            ਜੇ (!nodep->sentreep() || isPlainComboSentree(nodep->sentreep())) {
                m_comboAlwayss.emplace_ਪਿੱਠ(m_scopep, nodep);
            } ਨਹੀਂਤਾਂ {
                m_nonComboAlwayss.emplace_ਪਿੱਠ(m_scopep, nodep);
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_W* nodep) override {
        // Continuous aliases are unordered hardware connections, so source
        // order should not affect whether an if-chain FSM is recognized.
        collect_Alias_From_ਸੌਂਪੋ(nodep, m_stateAliases, m_ambiguousStateAliases);
        collect_Cell_ਪੋਰਟ_Alias(nodep);
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstCell* nodep) override {
        // Cells are matched after the full traversal because linkdot lowers
        // uninlined port connections into sibling continuous assignments.
        ਜੇ (m_scopep && fsm_ਰਜਿਸਟਰ_Wrapper_Desc(nodep)) ਜੋੜੋ_Wrapper_Cell(m_scopep, nodep);
        iterateChildren(nodep);
    }

    // Continue the walk through the rest of the design hierarchy.
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        ਲਈ (ਅਚਲ std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            Fsm_ਰਜਿਸਟਰ_Candidate reg;
            ਜੇ (ਮੇਲ_Fsm_Wrapper_Cell(wrapperCell.ਪਹਿਲਾ, wrapperCell.ਦੂਜਾ, reg)) {
                m_ਰਜਿਸਟਰ_Candidates.emplace_ਪਿੱਠ(reg);
            }
        }
        ਲਈ (ਅਚਲ FsmComboAlways& one_ਖੰਡ : m_one_ਖੰਡ_Alwayss) process_One_ਖੰਡ_Always(one_ਖੰਡ);
        ਲਈ (ਅਚਲ FsmComboAlways& combo : m_comboAlwayss) processComboAlways(combo);
        ਲਈ (ਅਚਲ FsmComboAlways& combo : m_nonComboAlwayss) warnUnsupportedComboAlways(combo);
    }
};

// Builder of statements that will handle incrementation of proper AstNodeCoverDecls - which will
// be also created here
ਵਰਗ FsmCoverIncBuilder final {
    ਅਚਲ Fsm_ਗ੍ਰਾਫ& m_ਗ੍ਰਾਫ;  // Fsm graph
    std::ਸਦਿਸ਼<ਅਚਲ Fsm_State_ਸ਼ੀਰਸ਼*> m_ਹਾਲਤਾਂ;  // States vertexes inside fsm graph
    std::unordered_ਸੈੱਟ<uint64_t>
        m_undefined_ਹਾਲਤਾਂ;  // States that have no enum value but are possible to represent with a
                            // variable of given width.
                            // This variable keeps uint64_t instead of V3Numebr because it is safe
                            // to assume that nobody has over 16M TB of RAM
    ਅਚਲ Fsm_ਸ਼ੀਰਸ਼* m_defaultVertexp = ਖਾਲੀ_ਸੂਚਕ;  // Default vertex
    ਅਚਲ Fsm_ਸ਼ੀਰਸ਼* m_resetVertexp = ਖਾਲੀ_ਸੂਚਕ;  // Reset vertex
    ਮਾਪ_t m_ਕਿਨਾਰੇ_ਗਿਣਤੀ = 0;  // Total count of edges
    AstVarScope* ਅਚਲ m_prevVscp;  // Previous state of fsm
    File_ਸਤਰ* ਅਚਲ m_flp;  // Fileline pointer to use across the builder
    ਅਚਲ std::string m_modPrettyName;  // Pretty fsm mode name
    Ast_ਗੰਢ_Stmt* m_coverIncp = ਖਾਲੀ_ਸੂਚਕ;  // Increments proper CoverDecl after each change
    Ast_ਗੰਢ_Stmt* m_initialCoverIncp = ਖਾਲੀ_ਸੂਚਕ;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = ਖਾਲੀ_ਸੂਚਕ;  // List of CoverDecls
    ਤਾਰਕਿਕ m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(ਅਚਲ uint64_t ਮੁੱਲ) {
        V3_ਸੰਖਿਆ ਅੰਕ{static_cast<Ast_ਗੰਢ*>(ਖਾਲੀ_ਸੂਚਕ), m_prevVscp->ਚੌੜਾਈ()};
        ਅੰਕ.ਸੈੱਟ_Quad(ਮੁੱਲ);
        ਅੰਕ.isSigned(m_prevVscp->dtypep()->isSigned());
        ਵਾਪਸ ਨਵਾਂ AstConst{m_flp, ਅੰਕ};
    }

    std::string quad_ਮੰਜ਼ਿਲ_ਦੋਆਧਾਰੀ_ਅੰਕ_Stirng_Label(ਅਚਲ uint64_t val) {
        ਪੂਰਨ ਚੌੜਾਈ = m_prevVscp->ਚੌੜਾਈ();
        UASSERT(ਚੌੜਾਈ < 64, "We should not expand values that wide");
        std::string ਨਤੀਜਾ = std::ਮੰਜ਼ਿਲ_string(ਚੌੜਾਈ) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        ਜਦੋਂ (--ਚੌੜਾਈ >= 0) ਨਤੀਜਾ.ਧੱਕੋ_ਪਿੱਠ("01"[(val >> ਚੌੜਾਈ) & 1]);
        ਵਾਪਸ ਨਤੀਜਾ;
    }

    ਖਾਲੀ init_Existing_ਹਾਲਤਾਂ() {
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_ਗ੍ਰਾਫ.vertices()) {
            ਅਚਲ Fsm_ਸ਼ੀਰਸ਼* ਅਚਲ vertexp = vtx.as<Fsm_ਸ਼ੀਰਸ਼>();
            m_ਕਿਨਾਰੇ_ਗਿਣਤੀ += vertexp->ਵਿੱਚ_ਕਿਨਾਰੇ().ਮਾਪ();
            ਜੇ (ਅਚਲ Fsm_State_ਸ਼ੀਰਸ਼* ਅਚਲ statep = vertexp->cast<Fsm_State_ਸ਼ੀਰਸ਼>()) {
                m_ਹਾਲਤਾਂ.ਧੱਕੋ_ਪਿੱਠ(statep);
            } ਨਹੀਂਤਾਂ ਜੇ (vertexp->isDefaultAny()) {
                UASSERT_OBJ(!std::exchange(m_defaultVertexp, vertexp), m_ਗ੍ਰਾਫ.sampleVarScopep(),
                            "Two defaults?");
            } ਨਹੀਂਤਾਂ /* if (vertexp->isResetAny()) */ {
                UASSERT_OBJ(!std::exchange(m_resetVertexp, vertexp), m_ਗ੍ਰਾਫ.sampleVarScopep(),
                            "Two resets?");
            }
        }
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = m_ਗ੍ਰਾਫ.sampleVarScopep()->ਚੌੜਾਈ();
        m_isComplete = ਚੌੜਾਈ < 64 && m_ਹਾਲਤਾਂ.ਮਾਪ() == (static_cast<ਮਾਪ_t>(1) << ਚੌੜਾਈ);
    }

    ਖਾਲੀ init_Undefined_ਹਾਲਤਾਂ() {
        UASSERT_OBJ(m_ਗ੍ਰਾਫ.sampleVarScopep()->ਚੌੜਾਈ() < 64, m_ਗ੍ਰਾਫ.sampleVarScopep(),
                    "This FSM is too wide to expand - this should be caught earlier");
        ਅਚਲ uint64_t combinations_ਗਿਣਤੀ = static_cast<ਮਾਪ_t>(1)
                                           << m_ਗ੍ਰਾਫ.sampleVarScopep()->ਚੌੜਾਈ();
        // Rolling trick allows to cut peak memory usage
        ਤਾਰਕਿਕ undefRolled = ਝੂਠ;
        ਸਵੈਚਾਲਿਤ rollUndefs = [ਇਹ, combinations_ਗਿਣਤੀ] {
            ਲਈ (uint64_t i = 0; i < combinations_ਗਿਣਤੀ; ++i) {
                ਅਚਲ ਸਵੈਚਾਲਿਤ ਪਾਓ = m_undefined_ਹਾਲਤਾਂ.ਪਾਓ(i);
                ਜੇ (!ਪਾਓ.ਦੂਜਾ) m_undefined_ਹਾਲਤਾਂ.erase(ਪਾਓ.ਪਹਿਲਾ);
            }
        };
        ਲਈ (ਅਚਲ Fsm_State_ਸ਼ੀਰਸ਼* ਅਚਲ statep : m_ਹਾਲਤਾਂ) {
            ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ = statep->ਮੁੱਲ().ਅੰਕ();
            ਅਚਲ uint64_t ਮੁੱਲ = ਅੰਕ.ਮੰਜ਼ਿਲ_UQuad();
            ਜੇ (!m_isComplete) {
                ਜੇ (undefRolled) {
                    UASSERT_OBJ(m_undefined_ਹਾਲਤਾਂ.erase(ਮੁੱਲ) == 1, m_ਗ੍ਰਾਫ.sampleVarScopep(),
                                "Two states has the same value?");
                } ਨਹੀਂਤਾਂ {
                    UASSERT_OBJ(m_undefined_ਹਾਲਤਾਂ.ਪਾਓ(ਮੁੱਲ).ਦੂਜਾ, m_ਗ੍ਰਾਫ.sampleVarScopep(),
                                "Two states has the same value?");
                    ਜੇ (m_undefined_ਹਾਲਤਾਂ.ਮਾਪ() * 2 >= combinations_ਗਿਣਤੀ) {
                        undefRolled = ਸੱਚ;
                        rollUndefs();
                    }
                }
            }
        }
        ਜੇ (!(m_isComplete || undefRolled)) rollUndefs();
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* buildResetCond() ਅਚਲ {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ refp
            = ਨਵਾਂ AstVarRef{m_flp, m_ਗ੍ਰਾਫ.resetCond().varScopep, VAccess::READ};
        ਵਾਪਸ m_ਗ੍ਰਾਫ.resetCond().ਸਰਗਰਮ_ਨੀਵਾਂ
                   ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstLogNot{m_flp, refp})
                   : refp;
    }

    ਮਾਪ_t get_Arc_ਗਿਣਤੀ_With_Expansion(ਅਚਲ VFsmExpandType expand) ਅਚਲ {
        ਮਾਪ_t arc_ਗਿਣਤੀ_With_Expansion = ~0ULL;
        ਚੋਣ (expand) {
        ਹਾਲਤ VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_ਗ੍ਰਾਫ.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            ਰੋਕੋ;
        }
        ਹਾਲਤ VFsmExpandType::AUTO: {
            arc_ਗਿਣਤੀ_With_Expansion = m_ਕਿਨਾਰੇ_ਗਿਣਤੀ;
            ਜੇ (m_defaultVertexp) {
                UASSERT_OBJ(m_ਹਾਲਤਾਂ.ਮਾਪ() >= m_defaultVertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().ਮਾਪ(),
                            m_ਗ੍ਰਾਫ.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arc_ਗਿਣਤੀ_With_Expansion -= m_defaultVertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().ਮਾਪ();
                ਲਈ (ਅਚਲ Fsm_State_ਸ਼ੀਰਸ਼* ਅਚਲ statep : m_ਹਾਲਤਾਂ) {
                    arc_ਗਿਣਤੀ_With_Expansion += static_cast<ਮਾਪ_t>(statep->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().ਖਾਲੀ_ਥਾਵਾਂ())
                                             * m_defaultVertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().ਮਾਪ();
                }
            }
            ਜੇ (m_resetVertexp) {
                UASSERT_OBJ(m_ਹਾਲਤਾਂ.ਮਾਪ() >= m_resetVertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().ਮਾਪ(),
                            m_ਗ੍ਰਾਫ.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arc_ਗਿਣਤੀ_With_Expansion += m_ਹਾਲਤਾਂ.ਮਾਪ() - m_resetVertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().ਮਾਪ();
            }
            ਰੋਕੋ;
        }
        ਹਾਲਤ VFsmExpandType::AUTO_EXPAND: {
            arc_ਗਿਣਤੀ_With_Expansion = m_ਹਾਲਤਾਂ.ਮਾਪ() * m_ਹਾਲਤਾਂ.ਮਾਪ() - m_ਹਾਲਤਾਂ.ਮਾਪ();
            ਰੋਕੋ;
        }
        ਹਾਲਤ VFsmExpandType::ਭਰਿਆ: {
            ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = m_ਗ੍ਰਾਫ.sampleVarScopep()->ਚੌੜਾਈ();
            // 32 is derived from:
            // tranitionsCount <= std::numeric_limits<uint64_t>::max()
            // (2^width)^2 - (2^width) <= 2^64 - 1
            // (2^width)^2 - (2^width) < 2^64 since we operate on integral values we can trade 1
            // log_2((2^width)^2 - (2^width)) < 64
            // log_2((2^width)^2) < 64 - `(2^width)` is negigible so, it may be dropped
            // 2width < 64
            ਜੇ (ਚੌੜਾਈ < 32) {
                ਅਚਲ uint64_t combinations_ਗਿਣਤੀ = static_cast<ਮਾਪ_t>(1) << ਚੌੜਾਈ;
                arc_ਗਿਣਤੀ_With_Expansion = combinations_ਗਿਣਤੀ * combinations_ਗਿਣਤੀ - combinations_ਗਿਣਤੀ;
            }
            ਰੋਕੋ;
        }
        }
        ਵਾਪਸ arc_ਗਿਣਤੀ_With_Expansion;
    }

    AstCoverOtherDecl* buildStateCoverDecl(ਅਚਲ std::string& label,
                                           ਅਚਲ std::string& fsm_ਟੈਗ = "") {
        AstCoverOtherDecl* ਅਚਲ declp
            = ਨਵਾਂ AstCoverOtherDecl{m_flp,
                                    "v_fsm_state/" + m_modPrettyName,
                                    m_ਗ੍ਰਾਫ.stateVarName() + "::" + label,
                                    "",
                                    0,
                                    m_ਗ੍ਰਾਫ.stateVarName(),
                                    "",
                                    label,
                                    fsm_ਟੈਗ};
        // We run after scoping, so this is declared for one instance only
        declp->ਪ੍ਰਤੀ_Instance(ਸੱਚ);
        m_declsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(m_declsp, declp);
        ਵਾਪਸ declp;
    }

    Ast_ਗੰਢ_Stmt* buildCoverIncStmtp(ਅਚਲ std::string& fromStateLabel,
                                    ਅਚਲ std::string& ਮੰਜ਼ਿਲ_State_Label,
                                    ਅਚਲ std::string& ਅਧਾਰ_Label = "") {
        // Whole builder is designed in such a way that this function won't be called twice with
        // the same arguments - but if it will ever change in future, `AstCoverOtherDecl` shall be
        // cached for particular args set
        AstCoverOtherDecl* ਅਚਲ declp = ਨਵਾਂ AstCoverOtherDecl{
            m_flp,
            "v_fsm_arc/" + m_modPrettyName,
            m_ਗ੍ਰਾਫ.stateVarName() + "::" + fromStateLabel + "->" + ਮੰਜ਼ਿਲ_State_Label,
            "",
            0,
            m_ਗ੍ਰਾਫ.stateVarName(),
            fromStateLabel,
            ਮੰਜ਼ਿਲ_State_Label,
            ਅਧਾਰ_Label};
        // We run after scoping, so this is declared for one instance only
        declp->ਪ੍ਰਤੀ_Instance(ਸੱਚ);
        m_declsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(m_declsp, declp);
        ਵਾਪਸ ਨਵਾਂ AstCoverInc{m_flp, declp};
    }
    Ast_ਗੰਢ_Stmt* buildExpandedTransitionHandler(ਅਚਲ std::string& ਮੰਜ਼ਿਲ_State_Label,
                                                ਅਚਲ ਤਾਰਕਿਕ considerUndefs,
                                                ਅਚਲ ਤਾਰਕਿਕ expandOnlyWithTerminalVerticies,
                                                ਅਚਲ Fsm_State_ਸ਼ੀਰਸ਼* ਅਚਲ ਮੰਜ਼ਿਲ_Statep = ਖਾਲੀ_ਸੂਚਕ) {
        AstCase* ਅਚਲ arcCasep = ਨਵਾਂ AstCase{
            m_flp, VCaseType::CT_CASE, ਨਵਾਂ AstVarRef{m_flp, m_prevVscp, VAccess::READ}, ਖਾਲੀ_ਸੂਚਕ};
        std::unordered_ਸੈੱਟ<ਅਚਲ Fsm_State_ਸ਼ੀਰਸ਼*> handledSoureces;
        ਜੇ (ਮੰਜ਼ਿਲ_Statep) {
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : ਮੰਜ਼ਿਲ_Statep->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
                ਜੇ (ਅਚਲ Fsm_State_ਸ਼ੀਰਸ਼* ਅਚਲ fromp = ਕਿਨਾਰਾ.fromp()->cast<Fsm_State_ਸ਼ੀਰਸ਼>()) {
                    ਜੇ (!handledSoureces.emplace(fromp).ਦੂਜਾ) ਜਾਰੀ;
                    arcCasep->ਜੋੜੋ_Itemsp(ਨਵਾਂ Ast_Case_ਵਸਤੂ{
                        m_flp, makeConst(fromp->ਮੁੱਲ().ਅੰਕ().ਮੰਜ਼ਿਲ_UQuad()),
                        (fromp != ਮੰਜ਼ਿਲ_Statep) ? buildCoverIncStmtp(fromp->label(), ਮੰਜ਼ਿਲ_State_Label)
                                            : ਖਾਲੀ_ਸੂਚਕ});
                }
            }
        }
        ਲਈ (ਅਚਲ Fsm_State_ਸ਼ੀਰਸ਼* ਅਚਲ fromp : m_ਹਾਲਤਾਂ) {
            ਜੇ (handledSoureces.ਲੱਭੋ(fromp) != handledSoureces.ਸਿਰਾ()) ਜਾਰੀ;
            arcCasep->ਜੋੜੋ_Itemsp(ਨਵਾਂ Ast_Case_ਵਸਤੂ{
                m_flp, makeConst(fromp->ਮੁੱਲ().ਅੰਕ().ਮੰਜ਼ਿਲ_UQuad()),
                (fromp != ਮੰਜ਼ਿਲ_Statep
                 && (!expandOnlyWithTerminalVerticies || fromp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().ਖਾਲੀ_ਥਾਵਾਂ()))
                    ? buildCoverIncStmtp(fromp->label(), ਮੰਜ਼ਿਲ_State_Label, "artificial")
                    : ਖਾਲੀ_ਸੂਚਕ});
        }
        ਜੇ (!m_isComplete) {
            ਜੇ (considerUndefs) {
                ਲਈ (ਅਚਲ uint64_t from : m_undefined_ਹਾਲਤਾਂ) {
                    ਅਚਲ std::string& label = quad_ਮੰਜ਼ਿਲ_ਦੋਆਧਾਰੀ_ਅੰਕ_Stirng_Label(from);
                    arcCasep->ਜੋੜੋ_Itemsp(
                        ਨਵਾਂ Ast_Case_ਵਸਤੂ{m_flp, makeConst(from),
                                        (label != ਮੰਜ਼ਿਲ_State_Label)
                                            ? buildCoverIncStmtp(label, ਮੰਜ਼ਿਲ_State_Label, "artificial")
                                            : ਖਾਲੀ_ਸੂਚਕ});
                }
            } ਨਹੀਂਤਾਂ {
                arcCasep->ਜੋੜੋ_Itemsp(ਨਵਾਂ Ast_Case_ਵਸਤੂ{m_flp, ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ});
            }
        }
        ਵਾਪਸ arcCasep;
    }
    Ast_ਗੰਢ_Stmt* buildUnxpandedTransitionHandler(ਅਚਲ Fsm_State_ਸ਼ੀਰਸ਼* ਅਚਲ ਮੰਜ਼ਿਲ_Statep) {
        AstCase* ਅਚਲ arcCasep = ਨਵਾਂ AstCase{
            m_flp, VCaseType::CT_CASE, ਨਵਾਂ AstVarRef{m_flp, m_prevVscp, VAccess::READ}, ਖਾਲੀ_ਸੂਚਕ};
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : ਮੰਜ਼ਿਲ_Statep->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
            ਅਚਲ Fsm_Arc_ਕਿਨਾਰਾ* ਅਚਲ arcp = ਕਿਨਾਰਾ.as<Fsm_Arc_ਕਿਨਾਰਾ>();
            UASSERT_OBJ(!arcp->isReset(), m_ਗ੍ਰਾਫ.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_ਗ੍ਰਾਫ.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            ਅਚਲ Fsm_State_ਸ਼ੀਰਸ਼* ਅਚਲ fromp = arcp->fromp()->as<Fsm_State_ਸ਼ੀਰਸ਼>();
            ਜੇ (fromp != ਮੰਜ਼ਿਲ_Statep) {
                arcCasep->ਜੋੜੋ_Itemsp(
                    ਨਵਾਂ Ast_Case_ਵਸਤੂ{m_flp, makeConst(fromp->ਮੁੱਲ().ਅੰਕ().ਮੰਜ਼ਿਲ_UQuad()),
                                    buildCoverIncStmtp(fromp->label(), ਮੰਜ਼ਿਲ_Statep->label())});
            }
        }
        arcCasep->ਜੋੜੋ_Itemsp(ਨਵਾਂ Ast_Case_ਵਸਤੂ{m_flp, ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ});
        ਵਾਪਸ arcCasep;
    }

ਜਨਤਕ:
    FsmCoverIncBuilder(ਅਚਲ Fsm_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ, AstVarScope* ਅਚਲ prevVscp)
        : m_ਗ੍ਰਾਫ{ਗ੍ਰਾਫ}
        , m_prevVscp{prevVscp}
        , m_flp{ਗ੍ਰਾਫ.fileline()}
        , m_modPrettyName{ਗ੍ਰਾਫ.scopep()->modp()->prettyName()} {
        init_Existing_ਹਾਲਤਾਂ();
        AstVarScope* ਅਚਲ sampleVscp = ਗ੍ਰਾਫ.sampleVarScopep();
        ਅਚਲ VFsmExpandType expand
            = ਗ੍ਰਾਫ.stateVarScopep()->varp()->attrFsmStateExpand() != VFsmExpandType::DEFAULT
                  ? ਗ੍ਰਾਫ.stateVarScopep()->varp()->attrFsmStateExpand()
                  : v3Global.opt.coverageFsmExpand();

        {
            ਅਚਲ ਮਾਪ_t arc_ਗਿਣਤੀ_With_Expansion = get_Arc_ਗਿਣਤੀ_With_Expansion(expand);
            ਜੇ (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arc_ਗਿਣਤੀ_With_Expansion == 0ULL
                            || arc_ਗਿਣਤੀ_With_Expansion
                                   > static_cast<ਮਾਪ_t>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arc_ਗਿਣਤੀ_With_Expansion);
                ਵਾਪਸ;
            }
        }

        AstCase* ਅਚਲ arcCasep
            = ਨਵਾਂ AstCase{m_flp, VCaseType::CT_CASE,
                          ਨਵਾਂ AstVarRef{m_flp, ਗ੍ਰਾਫ.sampleVarScopep(), VAccess::READ}, ਖਾਲੀ_ਸੂਚਕ};
        AstCase* ਅਚਲ initialStateCasep
            = ਨਵਾਂ AstCase{m_flp, VCaseType::CT_CASE,
                          ਨਵਾਂ AstVarRef{m_flp, ਗ੍ਰਾਫ.sampleVarScopep(), VAccess::READ}, ਖਾਲੀ_ਸੂਚਕ};

        ਅਚਲ ਤਾਰਕਿਕ expandAll = expand != VFsmExpandType::AUTO;
        std::unordered_ਸੈੱਟ<ਅਚਲ Fsm_State_ਸ਼ੀਰਸ਼*>
            defaultTargets;  // States that have edge incoming from default vertex
        std::unordered_ਸੈੱਟ<ਅਚਲ Fsm_State_ਸ਼ੀਰਸ਼*>
            resetTargets;  // States that have edge incoming from reset vertex
        ਜੇ (!expandAll) {
            ਜੇ (m_defaultVertexp) {
                ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : m_defaultVertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                    defaultTargets.emplace(ਕਿਨਾਰਾ.ਸਿਖਰ()->as<Fsm_State_ਸ਼ੀਰਸ਼>());
                }
            }
            ਜੇ (m_resetVertexp) {
                ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : m_resetVertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                    resetTargets.emplace(ਕਿਨਾਰਾ.ਸਿਖਰ()->as<Fsm_State_ਸ਼ੀਰਸ਼>());
                }
            }
        }
        ਅਚਲ ਤਾਰਕਿਕ is_ਭਰਿਆ_Expansion = expand == VFsmExpandType::ਭਰਿਆ;
        ਜੇ (is_ਭਰਿਆ_Expansion) init_Undefined_ਹਾਲਤਾਂ();
        ਲਈ (ਅਚਲ Fsm_State_ਸ਼ੀਰਸ਼* ਅਚਲ statep : m_ਹਾਲਤਾਂ) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* ਅਚਲ declp = buildStateCoverDecl(statep->label());
            initialStateCasep->ਜੋੜੋ_Itemsp(
                ਨਵਾਂ Ast_Case_ਵਸਤੂ{m_flp, makeConst(statep->ਮੁੱਲ().ਅੰਕ().ਮੰਜ਼ਿਲ_UQuad()),
                                ਨਵਾਂ AstCoverInc{m_flp, declp, ਸੱਚ}});
            ਅਚਲ ਤਾਰਕਿਕ expandAllOrReset
                = expandAll || (resetTargets.ਲੱਭੋ(statep) != resetTargets.ਸਿਰਾ());
            ਅਚਲ ਤਾਰਕਿਕ defaultExpantion
                = !expandAllOrReset && (defaultTargets.ਲੱਭੋ(statep) != defaultTargets.ਸਿਰਾ());
            arcCasep->ਜੋੜੋ_Itemsp(ਨਵਾਂ Ast_Case_ਵਸਤੂ{
                m_flp, makeConst(statep->ਮੁੱਲ().ਅੰਕ().ਮੰਜ਼ਿਲ_UQuad()),
                Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ<Ast_ਗੰਢ_Stmt>(
                    ਨਵਾਂ AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), is_ਭਰਿਆ_Expansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        ਜੇ (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            ਜੇ (is_ਭਰਿਆ_Expansion) {
                ਲਈ (ਅਚਲ uint64_t ਮੁੱਲ : m_undefined_ਹਾਲਤਾਂ) {
                    ਅਚਲ std::string& label = quad_ਮੰਜ਼ਿਲ_ਦੋਆਧਾਰੀ_ਅੰਕ_Stirng_Label(ਮੁੱਲ);
                    AstCoverOtherDecl* ਅਚਲ declp = buildStateCoverDecl(label);
                    initialStateCasep->ਜੋੜੋ_Itemsp(ਨਵਾਂ Ast_Case_ਵਸਤੂ{
                        m_flp, makeConst(ਮੁੱਲ), ਨਵਾਂ AstCoverInc{m_flp, declp, ਸੱਚ}});
                    arcCasep->ਜੋੜੋ_Itemsp(
                        ਨਵਾਂ Ast_Case_ਵਸਤੂ{m_flp, makeConst(ਮੁੱਲ),
                                        Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ<Ast_ਗੰਢ_Stmt>(
                                            ਨਵਾਂ AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, ਸੱਚ, ਝੂਠ))});
                }
            } ਨਹੀਂਤਾਂ {
                // No expansion
                arcCasep->ਜੋੜੋ_Itemsp(ਨਵਾਂ Ast_Case_ਵਸਤੂ{m_flp, ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ});
                initialStateCasep->ਜੋੜੋ_Itemsp(ਨਵਾਂ Ast_Case_ਵਸਤੂ{m_flp, ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ});
            }
        }
        m_coverIncp = arcCasep;
        m_initialCoverIncp = initialStateCasep;
    }
    VL_UNCOPYABLE(FsmCoverIncBuilder);
    ~FsmCoverIncBuilder() = ਮੂਲ;
    Ast_ਗੰਢ_Cover_Decl* getCoverDeclsp() ਅਚਲ { ਵਾਪਸ m_declsp; }
    Ast_ਗੰਢ_Stmt* getCoverIncp() ਅਚਲ { ਵਾਪਸ m_coverIncp; }
    Ast_ਗੰਢ_Stmt* getInitialCoverIncp() ਅਚਲ { ਵਾਪਸ m_initialCoverIncp; }
};

// Lower the completed FSM graphs into the concrete coverage declarations,
// previous-state tracking, and pre/post-triggered instrumentation that the
// runtime uses to record state and transition coverage.
ਵਰਗ Fsm_ਹੇਠਲਾ_Visitor final {
    // STATE - across all visitors
    ਅਚਲ FsmState& m_state;
    V3SharedTmps m_prevTmps{"__VfsmCovPrev", VVarType::MODULETEMP};  // Previous states

    // METHODS
    // Rebuild the original event control from the saved sense description so
    // post-state coverage sampling runs on the same triggering edges.
    ਸਥਿਰ Ast_Sen_ਰੁੱਖ* build_Sen_ਰੁੱਖ(File_ਸਤਰ* flp, ਅਚਲ std::ਸਦਿਸ਼<FsmSenDesc>& senses) {
        Ast_Sen_ਰੁੱਖ* ਅਚਲ sentreep = ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, ਖਾਲੀ_ਸੂਚਕ};
        ਲਈ (ਅਚਲ FsmSenDesc& sense : senses) {
            Ast_Sen_ਵਸਤੂ* ਅਚਲ senItemp
                = ਨਵਾਂ Ast_Sen_ਵਸਤੂ{flp, VEdgeType{sense.ਕਿਨਾਰਾ_Type},
                                 ਨਵਾਂ AstVarRef{flp, sense.varScopep, VAccess::READ}};
            sentreep->ਜੋੜੋ_Sensesp(senItemp);
        }
        ਵਾਪਸ sentreep;
    }

    // Lower one fully detected FSM graph into the concrete coverage machinery
    // used by generated models: declarations, previous-state tracking, and the
    // pre/post-triggered increment logic for states and arcs.
    ਖਾਲੀ buildOne(ਅਚਲ Fsm_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ) {
        UINFO(1, "buildOne lowering FSM " << ਗ੍ਰਾਫ.stateVarName()
                                          << " vertices=" << ਗ੍ਰਾਫ.vertices().ਮਾਪ() << ਸਤਰ_ਅੰਤ);
        AstAlways* ਅਚਲ alwaysp = ਗ੍ਰਾਫ.stateAlwaysp();
        AstScope* ਅਚਲ scopep = ਗ੍ਰਾਫ.scopep();
        AstVarScope* ਅਚਲ stateVscp = ਗ੍ਰਾਫ.stateVarScopep();
        AstVarScope* ਅਚਲ sampleVscp = ਗ੍ਰਾਫ.sampleVarScopep();
        File_ਸਤਰ* ਅਚਲ flp = ਗ੍ਰਾਫ.fileline();
        Ast_ਗੰਢ_Module* ਅਚਲ modp = scopep->modp();
        AstVarScope* ਅਚਲ prevVscp
            = m_prevTmps.make(flp, scopep, sampleVscp->dtypep(), stateVscp->varp()->shortName());
        // The saved previous-state temp crosses the scheduler's pre/post split
        // in the same way as Verilator's built-in NBA shadow variables, so keep
        // both vars marked as post-life participants for stable MT ordering.
        stateVscp->optimizeLifePost(ਸੱਚ);
        sampleVscp->optimizeLifePost(ਸੱਚ);
        prevVscp->optimizeLifePost(ਸੱਚ);

        Ast_ਸਰਗਰਮ* ਅਚਲ initActivep
            = ਨਵਾਂ Ast_ਸਰਗਰਮ{flp, "fsm-coverage-init",
                            ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, ਨਵਾਂ Ast_Sen_ਵਸਤੂ{flp, Ast_Sen_ਵਸਤੂ::Initial{}}}};
        initActivep->sen_ਰੁੱਖ_Storep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstInitialStatic{
            flp, ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, prevVscp, VAccess::WRITE},
                               ਨਵਾਂ AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->ਜੋੜੋ_Blocksp(initActivep);

        AstAlwaysPost* ਅਚਲ covPostp = ਨਵਾਂ AstAlwaysPost{flp};
        ਤਾਰਕਿਕ updatePrevAfterPost = ਝੂਠ;
        ਜੇ (alwaysp) {
            // Save the previous state as plain sequential logic at the front of
            // the original always_ff body, then evaluate coverage in post logic
            // after the delayed state update commits. This avoids a scheduler
            // race between a separate AstAlwaysPre task and the real state
            // commit for direct parent-level registers.
            alwaysp->prependStmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, prevVscp, VAccess::WRITE},
                                                 ਨਵਾਂ AstVarRef{flp, sampleVscp, VAccess::READ}});
        } ਨਹੀਂਤਾਂ {
            // Wrapper-derived register candidates do not have a parent
            // always_ff body to splice into. Sample coverage first, then save
            // the current state for the next clock tick; this survives cell
            // boundary scheduling where the real flop update lives elsewhere.
            updatePrevAfterPost = ਸੱਚ;
            prevVscp->varp()->ਸੈੱਟ_Ignore_Post_Read();
        }

        {
            // Create and increment proper AstNodeCoverDecls
            ਅਚਲ FsmCoverIncBuilder coverIncBuilder{ਗ੍ਰਾਫ, prevVscp};
            modp->ਜੋੜੋ_Stmtsp(coverIncBuilder.getCoverDeclsp());
            initActivep->ਜੋੜੋ_Stmtsp(coverIncBuilder.getInitialCoverIncp());
            covPostp->ਜੋੜੋ_Stmtsp(
                ਨਵਾਂ AstIf{flp,
                          ਨਵਾਂ AstNeq{flp, ਨਵਾਂ AstVarRef{flp, prevVscp, VAccess::READ},
                                     ਨਵਾਂ AstVarRef{flp, sampleVscp, VAccess::READ}},
                          coverIncBuilder.getCoverIncp()});
        }

        ਜੇ (updatePrevAfterPost) {
            covPostp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, prevVscp, VAccess::WRITE},
                                              ਨਵਾਂ AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        Ast_Sen_ਰੁੱਖ* ਅਚਲ sentreep = build_Sen_ਰੁੱਖ(flp, ਗ੍ਰਾਫ.senses());
        Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਨਵਾਂ Ast_ਸਰਗਰਮ{flp, "fsm-coverage", sentreep};
        activep->sen_ਰੁੱਖ_Storep(sentreep);
        scopep->ਜੋੜੋ_Blocksp(activep);
        activep->ਜੋੜੋ_Stmtsp(covPostp);
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    // Lower every detected FSM graph from the shared local state into
    // concrete coverage instrumentation while the saved scoped pointers are
    // still valid in the same pass.
    ਸਪਸ਼ਟ Fsm_ਹੇਠਲਾ_Visitor(ਅਚਲ FsmState& state)
        : m_state{state} {
        ਲਈ (ਅਚਲ DetectedFsm& fsm : m_state.fsms()) { buildOne(*fsm.graphp); }
    }
};

}  // namespace

ਖਾਲੀ V3FsmDetect::detect(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    FsmState state;
    // Phase 1: recover each supported FSM into a complete graph while the
    // original clocked/case structure is still easy to recognize.
    FsmDetectVisitor detect{state, rootp};
    ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) {
        ਮਾਪ_t ਸੂਚਕਾਂਕ = 0;
        ਲਈ (ਅਚਲ DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dump_ਟੈਗ(ਸੂਚਕਾਂਕ++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { Fsm_ਹੇਠਲਾ_Visitor ਹੇਠਲਾ{state}; }
    V3Global::dump_Check_Global_ਰੁੱਖ("fsm-detect", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
