// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: NFA-based multi-cycle SVA assertion evaluation
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2005-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// V3AssertNfa's Transformations:
//
//  - Convert multi-cycle SVA sequences/properties into NFA graphs.
//  - Attach inherited assertion clocks before moving sampled-value functions.
//  - Emit module-level state registers driven by AstAlways blocks.
//  - Replace converted assertions with combinational match/reject checks
//    so V3AssertPre sees no multi-cycle SExpr (unsupported ones fall through).
//
//  Members marked OWNED hold an AST tree this pass allocated and must delete;
//  they are not linked into the netlist.
//
//*************************************************************************

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3AssertNfa.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Assert.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Task.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// NFA Graph Data Structures (V3Graph-derived per upstream convention)

ਨਾਮਾਕਾਸ਼ {

ਵਰਗ Sva_State_ਸ਼ੀਰਸ਼;

// Per-vertex algorithm data, stored via V3GraphVertex::userp() during lowering
ਬਣਤਰ Sva_ਸ਼ੀਰਸ਼_ਅੰਕੜੇ final {
    AstVar* stateVarp = ਖਾਲੀ_ਸੂਚਕ;  // NBA state register for this vertex
    AstVar* delayRingVarp = ਖਾਲੀ_ਸੂਚਕ;  // Packed occupancy bits or per-slot match counts
    AstVar* delay_Ring_ਸਿਰ_Varp
        = ਖਾਲੀ_ਸੂਚਕ;  // Cached outgoing slot; isolates readers from in-place writes
    AstVar* delayRingIdxVarp = ਖਾਲੀ_ਸੂਚਕ;  // Next slot written in delayRingVarp
    AstVar* delay_Ring_ਜੀਵਤ_ਗਿਣਤੀ_Varp = ਖਾਲੀ_ਸੂਚਕ;  // Number of threads in delayRingVarp
    AstVar* delayRingWrappedVarp = ਖਾਲੀ_ਸੂਚਕ;  // All slots written since the last clear
    AstVar* ਪੂਰਾ_LVarp = ਖਾਲੀ_ਸੂਚਕ;  // SAnd LHS done-latch
    AstVar* ਪੂਰਾ_RVarp = ਖਾਲੀ_ਸੂਚਕ;  // SAnd RHS done-latch
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* stateSigp = ਖਾਲੀ_ਸੂਚਕ;  // Combinational state signal; OWNED during lowering
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* state_ਗਿਣਤੀ_Sigp = ਖਾਲੀ_ਸੂਚਕ;  // Cover-sequence match count; OWNED during lowering
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* state_ਗਿਣਤੀ_ਉਛਾਲ_Sigp = ਖਾਲੀ_ਸੂਚਕ;  // Count overflow; OWNED during lowering
    ਤਾਰਕਿਕ delayRingCounts = ਝੂਠ;  // Ring slots hold cover-sequence match counts
    ਤਾਰਕਿਕ needsReg = ਝੂਠ;  // True if vertex has incoming clocked edge
};

// NFA state vertex -- one per NFA position in the sequence evaluation
ਵਰਗ Sva_State_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Sva_State_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
ਜਨਤਕ:
    // True if this is the sequence-match terminal vertex
    ਤਾਰਕਿਕ m_is_ਮੇਲ = ਝੂਠ;
    // OWNED throughout-guard condition clones; IEEE 1800-2023 16.9.9
    std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> m_throughoutConds;
    // Nonzero for a ring-buffer vertex for ## delays.
    ਤਾਰਕਿਕ m_isFixedDelayRing = ਝੂਠ;
    ਬਿਨਾਨਿਸ਼ਾਨੀ m_delay_Ring_ਮਾਪ = 0;  // Number of ring slots. Range: max-min+1.
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_delayRingClearCondp = ਖਾਲੀ_ਸੂਚਕ;  // local RHS for pure-boolean range
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_delayRingAdvanceCondp = ਖਾਲੀ_ਸੂਚਕ;  // Advance only when this condition holds
    ਤਾਰਕਿਕ m_replayAbortReject = ਝੂਠ;  // Compressed repetition needs per-thread abort replay
    // OWNED; enclosing-abort fire condition clearing state or suppressing guard rejection
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_abortClearp = ਖਾਲੀ_ਸੂਚਕ;
    // OWNED; reject-abort fire condition rejecting all represented live threads
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_abortRejectp = ਖਾਲੀ_ਸੂਚਕ;
    // Liveness terminal (IEEE weak semantics): reject must not fire from this source
    ਤਾਰਕਿਕ m_isUnbounded = ਝੂਠ;
    // Temporal sequence AND combiner; IEEE 1800-2023 16.9.5
    ਤਾਰਕਿਕ m_isAndCombiner = ਝੂਠ;
    Sva_State_ਸ਼ੀਰਸ਼* m_andLhsTermp = ਖਾਲੀ_ਸੂਚਕ;  // LHS sub-NFA terminal vertex
    Sva_State_ਸ਼ੀਰਸ਼* m_andRhsTermp = ਖਾਲੀ_ਸੂਚਕ;  // RHS sub-NFA terminal vertex
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_andLhsCondp = ਖਾਲੀ_ਸੂਚਕ;  // OWNED; LHS final condition (may be nullptr)
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_andRhsCondp = ਖਾਲੀ_ਸੂਚਕ;  // OWNED; RHS final condition (may be nullptr)
    // Reject sink for SAnd rejectOnFail wiring; not a state-signal source
    ਤਾਰਕਿਕ m_isRejectSink = ਝੂਠ;
    // In-window vertex of a strong s_always[m:n]: if its state is still set at
    // end-of-simulation the universal-quantifier window never completed, which is
    // a liveness failure (IEEE 1800-2023 16.12.11 strong semantics).
    ਤਾਰਕਿਕ m_strongPending = ਝੂਠ;
    // Ring introduced for the checked portion of a bounded strong s_always window.
    ਤਾਰਕਿਕ m_strongAlwaysRing = ਝੂਠ;

    // CONSTRUCTORS
    ਸਪਸ਼ਟ Sva_State_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp} {}
    ~Sva_State_ਸ਼ੀਰਸ਼() override {
        ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* cp : m_throughoutConds) VL_DO_DANGLING(cp->delete_ਰੁੱਖ(), cp);
        ਜੇ (m_delayRingClearCondp)
            VL_DO_DANGLING(m_delayRingClearCondp->delete_ਰੁੱਖ(), m_delayRingClearCondp);
        ਜੇ (m_delayRingAdvanceCondp)
            VL_DO_DANGLING(m_delayRingAdvanceCondp->delete_ਰੁੱਖ(), m_delayRingAdvanceCondp);
        ਜੇ (m_abortClearp) VL_DO_DANGLING(m_abortClearp->delete_ਰੁੱਖ(), m_abortClearp);
        ਜੇ (m_abortRejectp) VL_DO_DANGLING(m_abortRejectp->delete_ਰੁੱਖ(), m_abortRejectp);
        ਜੇ (m_andLhsCondp) VL_DO_DANGLING(m_andLhsCondp->delete_ਰੁੱਖ(), m_andLhsCondp);
        ਜੇ (m_andRhsCondp) VL_DO_DANGLING(m_andRhsCondp->delete_ਰੁੱਖ(), m_andRhsCondp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string name() ਅਚਲ override {
        string name = "s" + cvt_ਮੰਜ਼ਿਲ_Str(color());
        ਜੇ (m_delay_Ring_ਮਾਪ) {
            name += "\\n";
            name += m_isFixedDelayRing ? "fixed chain " : "range chain ";
            name += cvt_ਮੰਜ਼ਿਲ_Str(m_delay_Ring_ਮਾਪ) + " slots";
        }
        ਵਾਪਸ name;
    }
    string dotColor() ਅਚਲ override {
        ਜੇ (m_is_ਮੇਲ) ਵਾਪਸ "red";
        ਜੇ (m_delay_Ring_ਮਾਪ) ਵਾਪਸ "blue";
        ਜੇ (m_isAndCombiner) ਵਾਪਸ "purple";
        ਵਾਪਸ "black";
    }
    // LCOV_EXCL_STOP
    // Access per-vertex algorithm data (valid only during lowering phase)
    Sva_ਸ਼ੀਰਸ਼_ਅੰਕੜੇ* datap() ਅਚਲ { ਵਾਪਸ static_cast<Sva_ਸ਼ੀਰਸ਼_ਅੰਕੜੇ*>(userp()); }
};

// NFA transition edge -- clocked (##1) or combinational link (##0)
ਵਰਗ Sva_Trans_ਕਿਨਾਰਾ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ {
    VL_RTTI_IMPL(Sva_Trans_ਕਿਨਾਰਾ, V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ)
ਜਨਤਕ:
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_condp;  // Transition condition; nullptr = unconditional; OWNED
    ਤਾਰਕਿਕ m_consumes_ਚੱਕਰ;  // true = clocked edge (##1), false = link (##0/boolean)
    // Reject when source is active and condp is false; set only on
    // outermost required-step Link
    ਤਾਰਕਿਕ m_rejectOnFail = ਝੂਠ;
    // Optional dynamic condition vertex for m_rejectOnFail. Used when the
    // success condition is another NFA state rather than a static expression.
    Sva_State_ਸ਼ੀਰਸ਼* m_condVtxp = ਖਾਲੀ_ਸੂਚਕ;

    // CONSTRUCTORS
    Sva_Trans_ਕਿਨਾਰਾ(V3_ਗ੍ਰਾਫ* graphp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp,
                 ਤਾਰਕਿਕ consumes_ਚੱਕਰ)
        : V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{graphp, fromp, ਸਿਖਰ, /*weight=*/1}
        , m_condp{condp}
        , m_consumes_ਚੱਕਰ{consumes_ਚੱਕਰ} {}
    ~Sva_Trans_ਕਿਨਾਰਾ() override {
        ਜੇ (m_condp) VL_DO_DANGLING(m_condp->delete_ਰੁੱਖ(), m_condp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string dotLabel() ਅਚਲ override { ਵਾਪਸ m_consumes_ਚੱਕਰ ? "##1" : "link"; }
    string dotStyle() ਅਚਲ override { ਵਾਪਸ m_consumes_ਚੱਕਰ ? "" : "dashed"; }
    // LCOV_EXCL_STOP
    // Typed accessors for NFA vertices
    Sva_State_ਸ਼ੀਰਸ਼* fromVtxp() ਅਚਲ { ਵਾਪਸ static_cast<Sva_State_ਸ਼ੀਰਸ਼*>(fromp()); }
    Sva_State_ਸ਼ੀਰਸ਼* ਮੰਜ਼ਿਲ_Vtxp() ਅਚਲ { ਵਾਪਸ static_cast<Sva_State_ਸ਼ੀਰਸ਼*>(ਸਿਖਰ()); }
};

// NFA graph container
ਵਰਗ Sva_ਗ੍ਰਾਫ final {
ਜਨਤਕ:
    V3_ਗ੍ਰਾਫ m_ਗ੍ਰਾਫ;  // Owns all vertices and edges
    Sva_State_ਸ਼ੀਰਸ਼* m_ਸ਼ੁਰੂ_Vertexp = ਖਾਲੀ_ਸੂਚਕ;  // Trigger/start vertex
    Sva_State_ਸ਼ੀਰਸ਼* m_ਮੇਲ_Vertexp = ਖਾਲੀ_ਸੂਚਕ;  // Sequence-match terminal vertex

    // Create a new state vertex
    Sva_State_ਸ਼ੀਰਸ਼* create_State_ਸ਼ੀਰਸ਼() { ਵਾਪਸ ਨਵਾਂ Sva_State_ਸ਼ੀਰਸ਼{&m_ਗ੍ਰਾਫ}; }
    // Create the match terminal vertex
    Sva_State_ਸ਼ੀਰਸ਼* create_ਮੇਲ_ਸ਼ੀਰਸ਼() {
        Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = create_State_ਸ਼ੀਰਸ਼();
        vtxp->m_is_ਮੇਲ = ਸੱਚ;
        m_ਮੇਲ_Vertexp = vtxp;
        ਵਾਪਸ vtxp;
    }
    // Add a clocked transition edge (##1)
    Sva_Trans_ਕਿਨਾਰਾ* ਜੋੜੋ_Clocked_ਕਿਨਾਰਾ(Sva_State_ਸ਼ੀਰਸ਼* fromp, Sva_State_ਸ਼ੀਰਸ਼* ਸਿਖਰ,
                                 Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp = ਖਾਲੀ_ਸੂਚਕ) {
        ਵਾਪਸ ਨਵਾਂ Sva_Trans_ਕਿਨਾਰਾ{&m_ਗ੍ਰਾਫ, fromp, ਸਿਖਰ, condp, /*consumesCycle=*/ਸੱਚ};
    }
    // Add a combinational link (##0 / boolean condition)
    Sva_Trans_ਕਿਨਾਰਾ* ਜੋੜੋ_Link(Sva_State_ਸ਼ੀਰਸ਼* fromp, Sva_State_ਸ਼ੀਰਸ਼* ਸਿਖਰ,
                          Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp = ਖਾਲੀ_ਸੂਚਕ) {
        ਵਾਪਸ ਨਵਾਂ Sva_Trans_ਕਿਨਾਰਾ{&m_ਗ੍ਰਾਫ, fromp, ਸਿਖਰ, condp, /*consumesCycle=*/ਝੂਠ};
    }
    // Collect all edges into a flat vector for iteration.
    // Used by the lowering phase which needs global edge scans.
    std::ਸਦਿਸ਼<ਅਚਲ Sva_Trans_ਕਿਨਾਰਾ*> all_ਕਿਨਾਰੇ() ਅਚਲ {
        std::ਸਦਿਸ਼<ਅਚਲ Sva_Trans_ਕਿਨਾਰਾ*> ਨਤੀਜਾ;
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtxr : m_ਗ੍ਰਾਫ.vertices()) {
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& edger : vtxr.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                ਨਤੀਜਾ.ਧੱਕੋ_ਪਿੱਠ(static_cast<ਅਚਲ Sva_Trans_ਕਿਨਾਰਾ*>(&edger));
            }
        }
        ਵਾਪਸ ਨਤੀਜਾ;
    }
};

//######################################################################
// Builder result: terminal vertex + optional final condition (match Link condition).
ਬਣਤਰ Build_ਨਤੀਜਾ final {
    Sva_State_ਸ਼ੀਰਸ਼* ਪਦ_Vertexp;  // Primary terminal; contributes to both match and reject
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* finalCondp;  // nullptr = unconditional
    // Mid-window sources for range delays (pure boolean RHS): match-only (isUnbounded)
    std::ਸਦਿਸ਼<Sva_State_ਸ਼ੀਰਸ਼*> midSources;
    ਤਾਰਕਿਕ errorEmitted = ਝੂਠ;  // Builder already emitted specific error; skip generic
    // For cover_sequence: when true, midSources already enumerate every
    // end-of-match, so wireMatchAndMidSources must NOT add the main
    // termVtxp -> matchVertex Link (would double-count via the merge vertex).
    ਤਾਰਕਿਕ ਪਦ_Is_Mid_ਮਿਲਾਓ = ਝੂਠ;
    ਤਾਰਕਿਕ valid() ਅਚਲ { ਵਾਪਸ ਪਦ_Vertexp != ਖਾਲੀ_ਸੂਚਕ; }
    ਸਥਿਰ Build_ਨਤੀਜਾ fail(ਤਾਰਕਿਕ errored = ਝੂਠ) { ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ, {}, errored}; }
    ਸਥਿਰ Build_ਨਤੀਜਾ failWithError() { ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ, {}, ਸੱਚ}; }
};

// Parser-marked SAnd of overlapped implications: a property if/else/case.
ਸਥਿਰ ਤਾਰਕਿਕ hasPropertyControlConjunction(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
    ਵਾਪਸ nodep->exists([](ਅਚਲ AstSAnd* andp) { ਵਾਪਸ andp->propertyControl(); });
}

ਸਥਿਰ AstConst* newTypedConstp(File_ਸਤਰ* ਅਚਲ flp, ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep,
                                ਅਚਲ uint32_t ਮੁੱਲ) {
    AstConst* ਅਚਲ constp = ਨਵਾਂ AstConst{flp, AstConst::DTyped{}, dtypep};
    constp->ਅੰਕ().ਸੈੱਟ_Long(ਮੁੱਲ);
    ਵਾਪਸ constp;
}

ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* sampled(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
    ਵਾਪਸ ਨਵਾਂ AstSampled{exprp->fileline(), exprp, exprp->dtypep(), ਸੱਚ};
}

ਸਥਿਰ ਅਚਲ ਅੱਖਰ* assertPassOnQuery(ਤਾਰਕਿਕ vacuous) {
    ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* queries[2]
        = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
    ਵਾਪਸ queries[vacuous];
}

ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* assertOnCond(File_ਸਤਰ* flp, VAssertType type,
                                 VAssertDirectiveType directiveType) {
    ਜੇ (!v3Global.opt.assertOn()) { ਵਾਪਸ ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}; }
    ਵਾਪਸ ਨਵਾਂ AstCExpr{flp, AstCExpr::ਸ਼ੁੱਧ{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
}

ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* assertKillGet(File_ਸਤਰ* flp, VAssertType type,
                                  VAssertDirectiveType directiveType) {
    ਵਾਪਸ ਨਵਾਂ AstCExpr{flp, AstCExpr::ਸ਼ੁੱਧ{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_KILL", type, directiveType),
                        32};
}

ਸਥਿਰ string assert_Action_Control_ਅਗੇਤਰ(VAssertDirectiveType directiveType) {
    ਅਚਲ ਪੂਰਨ controlled = !!(static_cast<ਪੂਰਨ>(directiveType)
                              & (static_cast<ਪੂਰਨ>(VAssertDirectiveType::ASSERT)
                                 | static_cast<ਪੂਰਨ>(VAssertDirectiveType::COVER)
                                 | static_cast<ਪੂਰਨ>(VAssertDirectiveType::ASSUME)));
    ਅਚਲ ਪੂਰਨ checkRuntime = controlled & static_cast<ਪੂਰਨ>(v3Global.opt.assertOn());
    ਵਾਪਸ "("s + std::ਮੰਜ਼ਿਲ_string(controlled ^ 1) + " || ("s + std::ਮੰਜ਼ਿਲ_string(checkRuntime)
           + " && "s;
}

ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* assertPassOnCond(File_ਸਤਰ* flp, VAssertType type,
                                     VAssertDirectiveType directiveType, ਤਾਰਕਿਕ vacuous) {
    ਵਾਪਸ ਨਵਾਂ AstCExpr{
        flp, AstCExpr::ਸ਼ੁੱਧ{},
        assert_Action_Control_ਅਗੇਤਰ(directiveType)
            + V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType)
            + "))"s,
        1};
}

ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* assertFailOnCond(File_ਸਤਰ* flp, VAssertType type,
                                     VAssertDirectiveType directiveType) {
    ਵਾਪਸ ਨਵਾਂ AstCExpr{
        flp, AstCExpr::ਸ਼ੁੱਧ{},
        assert_Action_Control_ਅਗੇਤਰ(directiveType)
            + V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType) + "))"s,
        1};
}

ਸਥਿਰ AstIf* newPassOnIf(File_ਸਤਰ* flp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* firep, Ast_ਗੰਢ* bodyp, VAssertType type,
                          VAssertDirectiveType directiveType, ਤਾਰਕਿਕ vacuous) {
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp
        = ਨਵਾਂ AstLogAnd{flp, firep, assertPassOnCond(flp, type, directiveType, vacuous)};
    AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(ਸੱਚ);
    ifp->user1(ਸੱਚ);
    ਵਾਪਸ ifp;
}

ਸਥਿਰ Ast_ਗੰਢ_Stmt* newIfAssertFailOn(Ast_ਗੰਢ* bodyp, VAssertDirectiveType directiveType,
                                      VAssertType type) {
    File_ਸਤਰ* ਅਚਲ flp = bodyp->fileline();
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = assertFailOnCond(flp, type, directiveType);
    AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(ਸੱਚ);
    ifp->user1(ਸੱਚ);
    ਵਾਪਸ ifp;
}

//######################################################################
// NFA Builder

ਵਰਗ SvaNfaBuilder final {
    Sva_ਗ੍ਰਾਫ& m_ਗ੍ਰਾਫ;  // NFA graph being built
    Ast_ਗੰਢ_Module* ਅਚਲ m_modp;  // Module to receive hoisted sampled-prop temps
    V3_Unique_ਨਾਂ& m_prop_ਆਰਜ਼ੀ_ਨਾਂ;  // Module-shared temp-var name source
    std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> m_temporal_Guard_ਢੇਰ;  // Guards active across nested temporal states
    // Outer abort conditions, AND-ed as !cond into inner abort edges
    // (IEEE 1800-2023 16.12.14 outer-wraps-inner).
    std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> m_outer_Abort_ਢੇਰ;
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Unbounded_Scope = ਝੂਠ;  // Sticky: nodes created after inherit liveness
    ਤਾਰਕਿਕ m_markStrongPending = ਝੂਠ;  // Mark new vertices as strong s_always in-window
    ਤਾਰਕਿਕ m_isCover = ਝੂਠ;  // Cover directives do not fail at end-of-simulation
    // IEEE 1800-2023 16.14.3 cover sequence: each end-of-match fires the action,
    // not just the first. Builder builds parallel-branch (no first-match-wins)
    // topology when true. Default false preserves cover_property semantics.
    ਤਾਰਕਿਕ m_isCoverSeq = ਝੂਠ;
    // Unsupported endpoint topology must reject, not ignore, or the wait hangs
    ਤਾਰਕਿਕ m_isSeqEvent = ਝੂਠ;
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Sequence_ਅਗੇਤਰ = ਝੂਠ;  // Prefix composition does not preserve every endpoint
    ਤਾਰਕਿਕ m_want_Bool_ਪੱਤੀ = ਝੂਠ;  // Building a |-> / |=> or top-level body (see buildOrMerge)

    ਬਣਤਰ RangeDelayRejectInfo final {
        Sva_State_ਸ਼ੀਰਸ਼* startp = ਖਾਲੀ_ਸੂਚਕ;
        ਬਿਨਾਨਿਸ਼ਾਨੀ range = 0;
        ਪੂਰਨ rhsLen = 0;
    };

    ਖਾਲੀ warnEndpointUnsupported(File_ਸਤਰ* flp, ਅਚਲ string& what) ਅਚਲ {
        ਜੇ (m_isSeqEvent) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: sequence used as an event control with " << what);
        } ਨਹੀਂਤਾਂ {
            flp->v3warn(COVERIGN, "Ignoring unsupported: cover sequence with " << what);
        }
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* throughoutCond(Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਧਾਰ_Condp, File_ਸਤਰ* flp) {
        ਜੇ (m_temporal_Guard_ਢੇਰ.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਅਧਾਰ_Condp;
        // AND all active temporal guards (supports nesting)
        // Each must use $sampled values.
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* guardp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp : m_temporal_Guard_ਢੇਰ) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ clonep = sampled(condp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
            ਜੇ (!guardp) {
                guardp = clonep;
            } ਨਹੀਂਤਾਂ {
                guardp = ਨਵਾਂ AstLogAnd{flp, guardp, clonep};
            }
        }
        ਜੇ (ਅਧਾਰ_Condp) { guardp = ਨਵਾਂ AstLogAnd{flp, ਅਧਾਰ_Condp, guardp}; }
        ਵਾਪਸ guardp;
    }

    ਸਥਿਰ ਬਿਨਾਨਿਸ਼ਾਨੀ getConstUInt(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ constp = V3Const::constifyEdit(exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
        ਅਚਲ AstConst* ਅਚਲ cp = VN_CAST(constp, Const);
        ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ val = cp ? cp->ਮੰਜ਼ਿਲ_UInt() : 0;
        VL_DO_DANGLING(constp->delete_ਰੁੱਖ(), constp);
        ਵਾਪਸ val;
    }

    // Static fixed-length analysis: clock ticks from entry to terminal, or -1.
    // Used by SIntersect to verify IEEE 1800-2023 16.9.6 equal-length precondition.
    //
    // Supported:
    //  - Boolean leaf (default)              -> 0
    //  - AstSExpr with fixed cycle delay     -> pre + N + body
    //  - AstSExpr with range delay M==N      -> pre + N + body
    //  - AstSThroughout                      -> length of rhsp (the seq)
    // Unsupported (returns -1):
    //  - Range delays with M != N            -> variable
    //  - Unbounded waits ##[M:$]             -> infinite/variable
    //  - ConsRep / SGotoRep / SAnd / SOr     -> defer (rare in intersect)
    //  - SIntersect nested in SIntersect     -> defer
    ਸਥਿਰ ਪੂਰਨ fixed_ਲੰਮਾਈ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਜੇ (AstSExpr* ਅਚਲ sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ਅਚਲ delayp = VN_CAST(sexprp->delayp(), Delay);
            ਜੇ (!delayp || !delayp->is_ਚੱਕਰ_Delay()) ਵਾਪਸ -1;
            ਬਿਨਾਨਿਸ਼ਾਨੀ delay_ਚੱਕਰਾਂ;
            ਜੇ (delayp->isRangeDelay()) {
                ਜੇ (delayp->isUnbounded()) ਵਾਪਸ -1;  // LCOV_EXCL_LINE
                ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ minD = getConstUInt(delayp->lhsp());
                ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ maxD = getConstUInt(delayp->rhsp());
                ਜੇ (minD != maxD) ਵਾਪਸ -1;
                delay_ਚੱਕਰਾਂ = minD;
            } ਨਹੀਂਤਾਂ {
                delay_ਚੱਕਰਾਂ = getConstUInt(delayp->lhsp());
            }
            ਪੂਰਨ preLen = 0;
            ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ prep = sexprp->preExprp()) {
                preLen = fixed_ਲੰਮਾਈ(prep);
                ਜੇ (preLen < 0) ਵਾਪਸ -1;  // LCOV_EXCL_LINE
            }
            ਅਚਲ ਪੂਰਨ bodyLen = fixed_ਲੰਮਾਈ(sexprp->exprp());
            ਜੇ (bodyLen < 0) ਵਾਪਸ -1;  // LCOV_EXCL_LINE
            ਵਾਪਸ preLen + delay_ਚੱਕਰਾਂ + bodyLen;
        }
        ਜੇ (AstSThroughout* ਅਚਲ throughp = VN_CAST(nodep, SThroughout)) {
            ਵਾਪਸ fixed_ਲੰਮਾਈ(throughp->rhsp());
        }
        ਜੇ (AstSOr* ਅਚਲ orp = VN_CAST(nodep, SOr)) {
            // Alternatives must share one end cycle; buildSWithin relies on
            // this to pair the OR with an SIntersect.
            ਅਚਲ ਪੂਰਨ lhsLen = fixed_ਲੰਮਾਈ(orp->lhsp());
            ਅਚਲ ਪੂਰਨ rhsLen = fixed_ਲੰਮਾਈ(orp->rhsp());
            ਜੇ (lhsLen < 0 || rhsLen < 0 || lhsLen != rhsLen) ਵਾਪਸ -1;
            ਵਾਪਸ lhsLen;
        }
        ਜੇ (AstSWithin* ਅਚਲ withinp = VN_CAST(nodep, SWithin)) {
            // `seq1 within seq2` ends at seq2's end cycle (IEEE 16.9.10).
            ਅਚਲ ਪੂਰਨ lhsLen = fixed_ਲੰਮਾਈ(withinp->lhsp());
            ਅਚਲ ਪੂਰਨ rhsLen = fixed_ਲੰਮਾਈ(withinp->rhsp());
            ਜੇ (lhsLen < 0 || rhsLen < 0 || lhsLen > rhsLen) ਵਾਪਸ -1;
            ਵਾਪਸ rhsLen;
        }
        // LCOV_EXCL_START -- defensive: V3AssertPre rejects composite SVA ops
        // nested in an intersect arm before fixedLength runs (clock-context
        // resolution fails). Kept as a guard in case future parser relaxations
        // permit it.
        ਜੇ (nodep->is_Multi_ਚੱਕਰ_Sva()) ਵਾਪਸ -1;
        // LCOV_EXCL_STOP
        // Plain boolean expression (no SVA constructs) -- 0 cycles.
        ਵਾਪਸ 0;
    }

    // Contiguous match-length range [lo,hi] for an operand whose length varies
    // from AT MOST ONE ranged cycle delay; {-1,-1} otherwise. Drives the
    // variable-length `intersect` lowering (IEEE 1800-2023 16.9.6): more than
    // one ranged delay would make the per-length realization ambiguous, so it
    // is reported unsupported rather than mis-paired.
    ਸਥਿਰ std::pair<ਪੂਰਨ, ਪੂਰਨ> ਲੰਮਾਈ_Range(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਜੇ (AstSExpr* ਅਚਲ sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ਅਚਲ delayp = VN_CAST(sexprp->delayp(), Delay);
            ਜੇ (!delayp || !delayp->is_ਚੱਕਰ_Delay()) ਵਾਪਸ {-1, -1};
            std::pair<ਪੂਰਨ, ਪੂਰਨ> delayRange;
            ਜੇ (delayp->isRangeDelay()) {
                ਜੇ (delayp->isUnbounded()) ਵਾਪਸ {-1, -1};
                ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ minD = getConstUInt(delayp->lhsp());
                ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ maxD = getConstUInt(delayp->rhsp());
                delayRange = {minD, maxD};
            } ਨਹੀਂਤਾਂ {
                ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ d = getConstUInt(delayp->lhsp());
                delayRange = {d, d};
            }
            std::pair<ਪੂਰਨ, ਪੂਰਨ> preRange{0, 0};
            ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ prep = sexprp->preExprp()) {
                preRange = ਲੰਮਾਈ_Range(prep);
                ਜੇ (preRange.ਪਹਿਲਾ < 0) ਵਾਪਸ {-1, -1};
            }
            ਅਚਲ std::pair<ਪੂਰਨ, ਪੂਰਨ> bodyRange = ਲੰਮਾਈ_Range(sexprp->exprp());
            ਜੇ (bodyRange.ਪਹਿਲਾ < 0) ਵਾਪਸ {-1, -1};
            ਅਚਲ ਪੂਰਨ variableParts = (preRange.ਪਹਿਲਾ != preRange.ਦੂਜਾ)
                                      + (delayRange.ਪਹਿਲਾ != delayRange.ਦੂਜਾ)
                                      + (bodyRange.ਪਹਿਲਾ != bodyRange.ਦੂਜਾ);
            ਜੇ (variableParts > 1) ਵਾਪਸ {-1, -1};
            ਵਾਪਸ {preRange.ਪਹਿਲਾ + delayRange.ਪਹਿਲਾ + bodyRange.ਪਹਿਲਾ,
                    preRange.ਦੂਜਾ + delayRange.ਦੂਜਾ + bodyRange.ਦੂਜਾ};
        }
        ਜੇ (AstSThroughout* ਅਚਲ throughp = VN_CAST(nodep, SThroughout)) {
            ਵਾਪਸ ਲੰਮਾਈ_Range(throughp->rhsp());
        }
        ਜੇ (nodep->is_Multi_ਚੱਕਰ_Sva()) ਵਾਪਸ {-1, -1};
        ਵਾਪਸ {0, 0};  // plain boolean -- 0 cycles
    }

    // Clone `operand` with its sole variable ranged cycle delay pinned so the
    // total match length is exactly `len`. `lo` is the operand's minimum length
    // (lengthRange().first). A fixed operand has no such delay and is returned
    // as a plain clone (callers only request its single achievable length).
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* realize_At_ਲੰਮਾਈ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਸੰਕਾਰਜ, ਪੂਰਨ len, ਪੂਰਨ lo) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ clonep = ਸੰਕਾਰਜ->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        AstDelay* rangeDelayp = ਖਾਲੀ_ਸੂਚਕ;
        clonep->foreach([&](AstDelay* dp) {
            ਜੇ (!rangeDelayp && dp->isRangeDelay() && !dp->isUnbounded()
                && getConstUInt(dp->lhsp()) != getConstUInt(dp->rhsp())) {
                rangeDelayp = dp;
            }
        });
        ਜੇ (rangeDelayp) {
            File_ਸਤਰ* ਅਚਲ flp = rangeDelayp->fileline();
            ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ pinned = getConstUInt(rangeDelayp->lhsp()) + (len - lo);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ oldMinp = rangeDelayp->lhsp();
            oldMinp->replaceWith(ਨਵਾਂ AstConst{flp, pinned});
            VL_DO_DANGLING(oldMinp->delete_ਰੁੱਖ(), oldMinp);
            // Drop the max bound so it lowers as a fixed `##d`, not `##[d:d]`.
            Ast_ਗੰਢ* ਅਚਲ oldMaxp = rangeDelayp->rhsp()->unlink_Fr_ਪਿੱਠ();
            VL_DO_DANGLING(oldMaxp->delete_ਰੁੱਖ(), oldMaxp);
        }
        ਵਾਪਸ clonep;
    }

    // Cuts AST size from O(N * sizeof(exprp)) to O(N) + O(sizeof(exprp)) by
    // sharing a single `VarRef` across N check edges. Hoist also matches the
    // IEEE 1800-2023 16.9.9 "single preponed-region snapshot" semantic for
    // any exprp -- even an impure one would now evaluate exactly once per
    // clock instead of N times. Orphan temps from failed builds are unused
    // MODULETEMPs and are removed by V3Dead.
    AstVar* tryHoistSampled(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, File_ਸਤਰ* flp, ਬਿਨਾਨਿਸ਼ਾਨੀ clone_ਗਿਣਤੀ) {
        constexpr ਬਿਨਾਨਿਸ਼ਾਨੀ k_Hoist_ਹੱਦ = 2;
        ਜੇ (clone_ਗਿਣਤੀ < k_Hoist_ਹੱਦ) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        AstVar* ਅਚਲ ਆਰਜ਼ੀ_Varp
            = ਨਵਾਂ AstVar{flp, VVarType::MODULETEMP, m_prop_ਆਰਜ਼ੀ_ਨਾਂ.get(exprp), exprp->dtypep()};
        m_modp->ਜੋੜੋ_Stmtsp(ਆਰਜ਼ੀ_Varp);
        Ast_ਸੌਂਪੋ* ਅਚਲ assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, ਆਰਜ਼ੀ_Varp, VAccess::WRITE},
                                                 sampled(exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ))};
        m_modp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, ਖਾਲੀ_ਸੂਚਕ, assignp});
        ਵਾਪਸ ਆਰਜ਼ੀ_Varp;
    }

    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* sampledRefOrClone(AstVar* hoistVarp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, File_ਸਤਰ* flp) {
        ਜੇ (hoistVarp) ਵਾਪਸ ਨਵਾਂ AstVarRef{flp, hoistVarp, VAccess::READ};
        ਵਾਪਸ sampled(exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
    }

    // Create vertex and inherit temporal guards from the current scope.
    Sva_State_ਸ਼ੀਰਸ਼* scoped_Create_ਸ਼ੀਰਸ਼() {
        Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = m_ਗ੍ਰਾਫ.create_State_ਸ਼ੀਰਸ਼();
        ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ cp : m_temporal_Guard_ਢੇਰ) {
            vtxp->m_throughoutConds.ਧੱਕੋ_ਪਿੱਠ(cp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
        }
        ਜੇ (m_ਵਿੱਚ_Unbounded_Scope) vtxp->m_isUnbounded = ਸੱਚ;
        ਜੇ (m_markStrongPending) vtxp->m_strongPending = ਸੱਚ;
        ਵਾਪਸ vtxp;
    }

    // AND current temporal guards into every edge/link.
    Sva_Trans_ਕਿਨਾਰਾ* guardedLink(Sva_State_ਸ਼ੀਰਸ਼* fromp, Sva_State_ਸ਼ੀਰਸ਼* ਸਿਖਰ, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp,
                              File_ਸਤਰ* flp) {
        ਵਾਪਸ m_ਗ੍ਰਾਫ.ਜੋੜੋ_Link(fromp, ਸਿਖਰ, throughoutCond(condp, flp));
    }
    Sva_Trans_ਕਿਨਾਰਾ* guardedLink(Sva_State_ਸ਼ੀਰਸ਼* fromp, Sva_State_ਸ਼ੀਰਸ਼* ਸਿਖਰ, File_ਸਤਰ* flp) {
        ਵਾਪਸ m_ਗ੍ਰਾਫ.ਜੋੜੋ_Link(fromp, ਸਿਖਰ, throughoutCond(ਖਾਲੀ_ਸੂਚਕ, flp));
    }
    Sva_Trans_ਕਿਨਾਰਾ* guarded_ਕਿਨਾਰਾ(Sva_State_ਸ਼ੀਰਸ਼* fromp, Sva_State_ਸ਼ੀਰਸ਼* ਸਿਖਰ, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp,
                              File_ਸਤਰ* flp) {
        ਵਾਪਸ m_ਗ੍ਰਾਫ.ਜੋੜੋ_Clocked_ਕਿਨਾਰਾ(fromp, ਸਿਖਰ, throughoutCond(condp, flp));
    }
    Sva_Trans_ਕਿਨਾਰਾ* guarded_ਕਿਨਾਰਾ(Sva_State_ਸ਼ੀਰਸ਼* fromp, Sva_State_ਸ਼ੀਰਸ਼* ਸਿਖਰ, File_ਸਤਰ* flp) {
        ਵਾਪਸ m_ਗ੍ਰਾਫ.ਜੋੜੋ_Clocked_ਕਿਨਾਰਾ(fromp, ਸਿਖਰ, throughoutCond(ਖਾਲੀ_ਸੂਚਕ, flp));
    }

    Sva_State_ਸ਼ੀਰਸ਼* ਜੋੜੋ_Delay_Chain(Sva_State_ਸ਼ੀਰਸ਼* startp, ਬਿਨਾਨਿਸ਼ਾਨੀ ਮਾਪ, File_ਸਤਰ* flp,
                                  ਤਾਰਕਿਕ isFixed = ਸੱਚ, Ast_ਗੰਢ_ਵਿਅੰਜਕ* clearCondp = ਖਾਲੀ_ਸੂਚਕ,
                                  Ast_ਗੰਢ_ਵਿਅੰਜਕ* advanceCondp = ਖਾਲੀ_ਸੂਚਕ) {
        ਜੇ (isFixed && ਮਾਪ == 0) ਵਾਪਸ startp;
        UASSERT_OBJ(ਮਾਪ > 0, startp, "Delay chain needs at least one slot");
        ਜੇ (isFixed && ਮਾਪ == 1 && !advanceCondp) {
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ nextp = scoped_Create_ਸ਼ੀਰਸ਼();
            guarded_ਕਿਨਾਰਾ(startp, nextp, flp);
            ਵਾਪਸ nextp;
        }
        Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ ringVtxp = scoped_Create_ਸ਼ੀਰਸ਼();
        ringVtxp->m_isFixedDelayRing = isFixed;
        ringVtxp->m_delay_Ring_ਮਾਪ = ਮਾਪ;
        ਜੇ (clearCondp) {
            UASSERT_OBJ(!isFixed, startp, "Fixed delay cannot have a clear condition");
            ringVtxp->m_delayRingClearCondp = clearCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        }
        ringVtxp->m_delayRingAdvanceCondp = advanceCondp;
        ਜੇ (isFixed) {
            guarded_ਕਿਨਾਰਾ(startp, ringVtxp, flp);
        } ਨਹੀਂਤਾਂ {
            guardedLink(startp, ringVtxp, flp);
        }
        ਵਾਪਸ ringVtxp;
    }

    // Build NFA for an SExpr. finalCond = RHS (not yet added as a vertex).
    // isTopLevelStep: marks outermost required boolean check as rejectOnFail.
    // Apply a range delay `##[M:N]` to currentp. Returns true on success. On
    // failure, emits a diagnostic and returns false.
    ਤਾਰਕਿਕ applyRangeDelay(AstSExpr* sexprp, Sva_State_ਸ਼ੀਰਸ਼*& currentp,
                         std::ਸਦਿਸ਼<Sva_State_ਸ਼ੀਰਸ਼*>& midSources,
                         RangeDelayRejectInfo* rangeRejectInfop = ਖਾਲੀ_ਸੂਚਕ) {
        File_ਸਤਰ* ਅਚਲ flp = sexprp->fileline();
        AstDelay* ਅਚਲ delayp = VN_AS(sexprp->delayp(), Delay);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsExprp = sexprp->exprp();
        ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ minDelay = getConstUInt(delayp->lhsp());
        ਜੇ (delayp->isUnbounded()) {
            // `##[M:$]`: wait M cycles, then retain every matured attempt in a
            // one-slot ring while also exposing the first eligible tick.
            currentp = ਜੋੜੋ_Delay_Chain(currentp, minDelay, flp);
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ waitRingp = ਜੋੜੋ_Delay_Chain(currentp, 1, flp, ਝੂਠ);
            // The ring's live count is updated in NBA, so this link retains the previous
            // tick's live attempts alongside new arrivals for the next tick.
            guardedLink(waitRingp, waitRingp, flp);
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ ਮਿਲਾਓ_Vtxp = scoped_Create_ਸ਼ੀਰਸ਼();
            guardedLink(currentp, ਮਿਲਾਓ_Vtxp, flp);
            guardedLink(waitRingp, ਮਿਲਾਓ_Vtxp, flp);
            currentp = ਮਿਲਾਓ_Vtxp;
            currentp->m_isUnbounded = ਸੱਚ;  // Liveness, so no reject.
            m_ਵਿੱਚ_Unbounded_Scope = ਸੱਚ;
            ਵਾਪਸ ਸੱਚ;
        }
        ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ maxDelay = getConstUInt(delayp->rhsp());
        ਜੇ (minDelay == maxDelay) {
            currentp = ਜੋੜੋ_Delay_Chain(currentp, minDelay, flp);
            ਵਾਪਸ ਸੱਚ;
        }
        ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ range = maxDelay - minDelay;
        currentp = ਜੋੜੋ_Delay_Chain(currentp, minDelay, flp);
        ਅਚਲ ਤਾਰਕਿਕ multi_ਚੱਕਰ_Rhs = rhsExprp->is_Multi_ਚੱਕਰ_Sva();
        ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ preExprp = sexprp->preExprp();
        // Prefix and nested-sequence composition can lose endpoints or their multiplicity
        // required by cover sequence (IEEE 1800-2023 16.14.3), regardless of the range width.
        ਜੇ (m_isCoverSeq
            && (multi_ਚੱਕਰ_Rhs || m_ਵਿੱਚ_Sequence_ਅਗੇਤਰ
                || (preExprp && preExprp->is_Multi_ਚੱਕਰ_Sva()))) {
            warnEndpointUnsupported(flp, "this ranged cycle delay");
            ਵਾਪਸ ਝੂਠ;
        }
        ਜੇ (m_ਵਿੱਚ_Sequence_ਅਗੇਤਰ) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: Bounded ranged cycle delay in a sequence prefix");
            ਵਾਪਸ ਝੂਠ;
        }
        ਜੇ (multi_ਚੱਕਰ_Rhs) {
            // Launch a candidate at every eligible tick. Candidate-local misses are not
            // assertion rejects while a later position can still match.
            ਜੇ (rangeRejectInfop) {
                ਅਚਲ ਪੂਰਨ rhsLen = fixed_ਲੰਮਾਈ(rhsExprp);
                ਜੇ (rhsLen >= 0) *rangeRejectInfop = {currentp, range, rhsLen};
            }
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ tailp = ਜੋੜੋ_Delay_Chain(currentp, range + 1U, flp, ਝੂਠ);
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ ਮਿਲਾਓ_Vtxp = scoped_Create_ਸ਼ੀਰਸ਼();
            ਮਿਲਾਓ_Vtxp->m_isUnbounded = ਸੱਚ;
            guardedLink(currentp, ਮਿਲਾਓ_Vtxp, flp);
            guardedLink(tailp, ਮਿਲਾਓ_Vtxp, flp);
            currentp = ਮਿਲਾਓ_Vtxp;
            m_ਵਿੱਚ_Unbounded_Scope = ਸੱਚ;
        } ਨਹੀਂਤਾਂ {
            // The first eligible tick is explicit. The ring retains attempts for the
            // remaining ticks and exposes the final tick for rejection. Only properties
            // clear on success, since cover sequence counts every endpoint.
            midSources.ਧੱਕੋ_ਪਿੱਠ(currentp);
            currentp = ਜੋੜੋ_Delay_Chain(currentp, range + 1U, flp, ਝੂਠ,
                                     m_isCoverSeq ? ਖਾਲੀ_ਸੂਚਕ : rhsExprp);
        }
        ਵਾਪਸ ਸੱਚ;
    }

    ਖਾਲੀ ਜੋੜੋ_Finite_Range_Reject(ਅਚਲ RangeDelayRejectInfo& info, ਅਚਲ Build_ਨਤੀਜਾ& ਨਤੀਜਾ,
                              File_ਸਤਰ* flp) {
        ਜੇ (!info.startp) ਵਾਪਸ;

        Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ expiryVtxp
            = ਜੋੜੋ_Delay_Chain(info.startp, info.range + info.rhsLen, flp);
        Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ expiryMatchp = scoped_Create_ਸ਼ੀਰਸ਼();
        std::ਸਦਿਸ਼<Sva_State_ਸ਼ੀਰਸ਼*> sources = ਨਤੀਜਾ.midSources;
        sources.ਧੱਕੋ_ਪਿੱਠ(ਨਤੀਜਾ.ਪਦ_Vertexp);
        ਲਈ (Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ srcp : sources) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp
                = ਨਤੀਜਾ.finalCondp ? sampled(ਨਤੀਜਾ.finalCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)) : ਖਾਲੀ_ਸੂਚਕ;
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ successNowp = scoped_Create_ਸ਼ੀਰਸ਼();
            guardedLink(srcp, successNowp, condp, flp);
            // Retain a success through the latest deadline it can satisfy.
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ historyp
                = ਜੋੜੋ_Delay_Chain(successNowp, info.range + 1U, flp, ਝੂਠ);
            guardedLink(successNowp, expiryMatchp, flp);
            guardedLink(historyp, expiryMatchp, flp);
        }

        Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ sinkVtxp = m_ਗ੍ਰਾਫ.create_State_ਸ਼ੀਰਸ਼();
        sinkVtxp->m_isRejectSink = ਸੱਚ;
        Sva_Trans_ਕਿਨਾਰਾ* ਅਚਲ rejectp = m_ਗ੍ਰਾਫ.ਜੋੜੋ_Link(expiryVtxp, sinkVtxp);
        rejectp->m_rejectOnFail = ਸੱਚ;
        rejectp->m_condVtxp = expiryMatchp;
    }

    Build_ਨਤੀਜਾ buildSExpr(AstSExpr* sexprp, Sva_State_ਸ਼ੀਰਸ਼* ਇੰਦਰਾਜ_Vtxp,
                           ਤਾਰਕਿਕ is_ਸਿਖਰ_Level_ਕਦਮ = ਝੂਠ) {
        AstDelay* ਅਚਲ delayp = VN_CAST(sexprp->delayp(), Delay);
        ਜੇ (!delayp || !delayp->is_ਚੱਕਰ_Delay()) ਵਾਪਸ Build_ਨਤੀਜਾ::fail();

        File_ਸਤਰ* ਅਚਲ flp = sexprp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = sexprp->exprp();

        // Handle LHS (preExpr)
        Sva_State_ਸ਼ੀਰਸ਼* currentp = ਇੰਦਰਾਜ_Vtxp;
        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ preExprp = sexprp->preExprp()) {
            VL_RESTORER(m_ਵਿੱਚ_Sequence_ਅਗੇਤਰ);
            m_ਵਿੱਚ_Sequence_ਅਗੇਤਰ = ਸੱਚ;
            ਅਚਲ Build_ਨਤੀਜਾ pre = build_ਵਿਅੰਜਕ(preExprp, currentp, is_ਸਿਖਰ_Level_ਕਦਮ);
            ਜੇ (!pre.valid()) ਵਾਪਸ Build_ਨਤੀਜਾ::fail(pre.errorEmitted);  // LCOV_EXCL_LINE
            ਜੇ (pre.finalCondp) {
                Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ condVtxp = scoped_Create_ਸ਼ੀਰਸ਼();
                Sva_Trans_ਕਿਨਾਰਾ* ਅਚਲ edgep = guardedLink(
                    pre.ਪਦ_Vertexp, condVtxp, sampled(pre.finalCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)), flp);
                ਜੇ (is_ਸਿਖਰ_Level_ਕਦਮ && !pre.ਪਦ_Vertexp->m_isUnbounded && !m_ਵਿੱਚ_Unbounded_Scope) {
                    // Do not mark liveness sources: first boolean check is deferred.
                    edgep->m_rejectOnFail = ਸੱਚ;
                }
                freeUnlinkedCondp(pre.finalCondp);
                currentp = condVtxp;
            } ਨਹੀਂਤਾਂ {
                currentp = pre.ਪਦ_Vertexp;
            }
        }

        // Handle delay
        std::ਸਦਿਸ਼<Sva_State_ਸ਼ੀਰਸ਼*> rangeMidSources;
        RangeDelayRejectInfo rangeRejectInfo;
        ਅਚਲ ਤਾਰਕਿਕ ਜੋੜੋ_Range_Reject = is_ਸਿਖਰ_Level_ਕਦਮ && !m_ਵਿੱਚ_Unbounded_Scope;
        ਜੇ (delayp->isRangeDelay()) {
            ਜੇ (!applyRangeDelay(sexprp, currentp, rangeMidSources,
                                 ਜੋੜੋ_Range_Reject ? &rangeRejectInfo : ਖਾਲੀ_ਸੂਚਕ)) {
                ਵਾਪਸ Build_ਨਤੀਜਾ::failWithError();
            }
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ delay_ਚੱਕਰਾਂ = getConstUInt(delayp->lhsp());
            currentp = ਜੋੜੋ_Delay_Chain(currentp, delay_ਚੱਕਰਾਂ, flp);
        }

        // Multi-cycle RHS: recurse (only plain boolean is returned as finalCondp).
        ਜੇ (exprp->is_Multi_ਚੱਕਰ_Sva()) {
            ਅਚਲ Build_ਨਤੀਜਾ ਨਤੀਜਾ = build_ਵਿਅੰਜਕ(exprp, currentp, is_ਸਿਖਰ_Level_ਕਦਮ);
            ਜੇ (ਨਤੀਜਾ.valid()) ਜੋੜੋ_Finite_Range_Reject(rangeRejectInfo, ਨਤੀਜਾ, flp);
            ਵਾਪਸ ਨਤੀਜਾ;
        }
        ਵਾਪਸ {currentp, exprp, std::move(rangeMidSources)};
    }

    Build_ਨਤੀਜਾ buildConsRep(AstSConsRep* repp, Sva_State_ਸ਼ੀਰਸ਼* ਇੰਦਰਾਜ_Vtxp,
                             ਤਾਰਕਿਕ is_ਸਿਖਰ_Level_ਕਦਮ = ਝੂਠ) {
        File_ਸਤਰ* ਅਚਲ flp = repp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = repp->exprp();
        // Multi-cycle expr in ConsRep not yet supported; bail to avoid invalid AST.
        ਜੇ (exprp->is_Multi_ਚੱਕਰ_Sva()) {
            repp->v3warn(E_UNSUPPORTED, "Unsupported: multi-cycle sequence expression inside"
                                        " consecutive repetition (IEEE 1800-2023 16.9.2)");
            ਵਾਪਸ Build_ਨਤੀਜਾ::failWithError();
        }
        ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ minN = getConstUInt(repp->countp());

        // Sum sites across prefix + unbounded/range tail so one hoist covers
        // every check edge of this repetition.
        ਬਿਨਾਨਿਸ਼ਾਨੀ ਕੁੱਲ_Sites = minN;
        ਜੇ (repp->unbounded()) {
            ਕੁੱਲ_Sites += 1;
        } ਨਹੀਂਤਾਂ ਜੇ (repp->maxCountp()) {
            ਕੁੱਲ_Sites += getConstUInt(repp->maxCountp()) - minN;
        }
        AstVar* ਅਚਲ hoistVarp = tryHoistSampled(exprp, flp, ਕੁੱਲ_Sites);

        // Cover-sequence (IEEE 1800-2023 16.14.3): collect each end-of-match
        // position so they all fire the action, not just the merged terminal.
        std::ਸਦਿਸ਼<Sva_State_ਸ਼ੀਰਸ਼*> consMidSources;

        Sva_State_ਸ਼ੀਰਸ਼* currentp = ਇੰਦਰਾਜ_Vtxp;
        ਲਈ (ਬਿਨਾਨਿਸ਼ਾਨੀ i = 0; i < minN; ++i) {
            // Keep the first repetition explicit, collapse all remaining checks into the ring.
            ਜੇ (i == 1) {
                currentp = ਜੋੜੋ_Delay_Chain(currentp, minN - 1, flp);
                currentp->m_delayRingClearCondp
                    = ਨਵਾਂ AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                currentp->m_replayAbortReject = ਸੱਚ;
                ਜੇ (is_ਸਿਖਰ_Level_ਕਦਮ) {
                    currentp->m_throughoutConds.ਧੱਕੋ_ਪਿੱਠ(
                        sampledRefOrClone(hoistVarp, exprp, flp));
                }
                i = minN - 1;
            }
            // Every repetition in the minimum prefix is required.
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ condVtxp = scoped_Create_ਸ਼ੀਰਸ਼();
            Sva_Trans_ਕਿਨਾਰਾ* ਅਚਲ linkp
                = guardedLink(currentp, condVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            // Only an outermost required step rejects the property. Its first check is explicit,
            // later required checks reject through the ring's throughout guard.
            ਜੇ (is_ਸਿਖਰ_Level_ਕਦਮ && i == 0) linkp->m_rejectOnFail = ਸੱਚ;
            currentp = condVtxp;
        }
        // After minN: currentp is the first valid end-of-match position for [*m:n].
        ਜੇ (m_isCoverSeq && (repp->unbounded() || repp->maxCountp())) {
            consMidSources.ਧੱਕੋ_ਪਿੱਠ(currentp);
        }

        ਜੇ (repp->unbounded()) {
            ਜੇ (minN == 0) {
                Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ waitVtxp = scoped_Create_ਸ਼ੀਰਸ਼();
                guarded_ਕਿਨਾਰਾ(currentp, waitVtxp, flp);
                Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ checkVtxp = scoped_Create_ਸ਼ੀਰਸ਼();
                guardedLink(waitVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guarded_ਕਿਨਾਰਾ(checkVtxp, waitVtxp, flp);
                guardedLink(currentp, checkVtxp, flp);
                currentp = checkVtxp;
            } ਨਹੀਂਤਾਂ {
                Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ loop_ਪਿੱਠ_Vtxp = scoped_Create_ਸ਼ੀਰਸ਼();
                guarded_ਕਿਨਾਰਾ(currentp, loop_ਪਿੱਠ_Vtxp, flp);
                Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ reCheckVtxp = scoped_Create_ਸ਼ੀਰਸ਼();
                guardedLink(loop_ਪਿੱਠ_Vtxp, reCheckVtxp, sampledRefOrClone(hoistVarp, exprp, flp),
                            flp);
                guarded_ਕਿਨਾਰਾ(reCheckVtxp, loop_ਪਿੱਠ_Vtxp, flp);
                guardedLink(reCheckVtxp, currentp, flp);
            }
            currentp->m_isUnbounded = ਸੱਚ;
            m_ਵਿੱਚ_Unbounded_Scope = ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (repp->maxCountp()) {
            ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ maxN = getConstUInt(repp->maxCountp());
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ ਮਿਲਾਓ_Vtxp = scoped_Create_ਸ਼ੀਰਸ਼();
            guardedLink(currentp, ਮਿਲਾਓ_Vtxp, flp);
            ਬਿਨਾਨਿਸ਼ਾਨੀ ਪੂਛ_Min_N = minN;
            Sva_State_ਸ਼ੀਰਸ਼* ਪੂਛ_Startp = currentp;
            ਜੇ (minN == 0) {
                // Build the first optional iteration explicitly. Feeding the empty endpoint
                // directly into a range ring would incorrectly keep that match alive.
                Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ ਅਗਲਾ_Vtxp = scoped_Create_ਸ਼ੀਰਸ਼();
                guarded_ਕਿਨਾਰਾ(currentp, ਅਗਲਾ_Vtxp, flp);
                Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ checkVtxp = scoped_Create_ਸ਼ੀਰਸ਼();
                guardedLink(ਅਗਲਾ_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, ਮਿਲਾਓ_Vtxp, flp);
                ਜੇ (m_isCoverSeq) consMidSources.ਧੱਕੋ_ਪਿੱਠ(checkVtxp);
                ਪੂਛ_Startp = checkVtxp;
                ਪੂਛ_Min_N = 1;
            }
            ਜੇ (maxN > ਪੂਛ_Min_N) {
                // Add tail-ring only if the tail is non-empty.
                Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ ਅਗਲਾ_Vtxp
                    = ਜੋੜੋ_Delay_Chain(ਪੂਛ_Startp, maxN - ਪੂਛ_Min_N + 1, flp, ਝੂਠ);
                ਅਗਲਾ_Vtxp->m_delayRingClearCondp
                    = ਨਵਾਂ AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                ਅਗਲਾ_Vtxp->m_replayAbortReject = ਸੱਚ;
                Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ checkVtxp = scoped_Create_ਸ਼ੀਰਸ਼();
                guardedLink(ਅਗਲਾ_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, ਮਿਲਾਓ_Vtxp, flp);
                ਜੇ (m_isCoverSeq) consMidSources.ਧੱਕੋ_ਪਿੱਠ(checkVtxp);
            }
            currentp = ਮਿਲਾਓ_Vtxp;
        }
        // finalCond = nullptr (already checked via Links)
        Build_ਨਤੀਜਾ res;
        res.ਪਦ_Vertexp = currentp;
        res.finalCondp = ਖਾਲੀ_ਸੂਚਕ;
        res.midSources = std::move(consMidSources);
        // mergeVtxp is the OR of all the end-positions we already pushed to
        // midSources, so the main termVtxp -> matchVertex Link would duplicate.
        res.ਪਦ_Is_Mid_ਮਿਲਾਓ = m_isCoverSeq && !res.midSources.ਖਾਲੀ_ਥਾਵਾਂ();
        ਵਾਪਸ res;
    }

    // always[lo:hi] / s_always[lo:hi] (IEEE 1800-2023 16.12.11).
    Build_ਨਤੀਜਾ buildPropAlways(AstPropAlways* nodep, Sva_State_ਸ਼ੀਰਸ਼* ਇੰਦਰਾਜ_Vtxp,
                                ਤਾਰਕਿਕ is_ਸਿਖਰ_Level_ਕਦਮ = ਝੂਠ) {
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ propp = nodep->propp();
        ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ lo = getConstUInt(nodep->loBoundp());
        ਜੇ (VN_IS(nodep->hiBoundp(), Unbounded)) {
            // Weak always [lo:$]: unbounded upper bound (IEEE 1800-2023 16.12.11).
            // p must hold at every clock tick at least lo cycles after the attempt
            // start; those ticks are not required to exist, so there is no
            // end-of-trace obligation (weak). The self-loop keeps the attempt live
            // every cycle; each observed cycle is a safety obligation, so a false p
            // rejects immediately.
            UASSERT_OBJ(!nodep->isStrong(), nodep, "Unbounded always must be weak (V3Width)");
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ livep = ਜੋੜੋ_Delay_Chain(ਇੰਦਰਾਜ_Vtxp, lo, flp);
            livep->m_isUnbounded = ਸੱਚ;
            guarded_ਕਿਨਾਰਾ(livep, livep, flp);  // stay active every subsequent cycle
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ sinkp = m_ਗ੍ਰਾਫ.create_State_ਸ਼ੀਰਸ਼();
            sinkp->m_isRejectSink = ਸੱਚ;
            Sva_Trans_ਕਿਨਾਰਾ* ਅਚਲ rejEdgep
                = guardedLink(livep, sinkp, sampled(propp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)), flp);
            ਜੇ (is_ਸਿਖਰ_Level_ਕਦਮ) rejEdgep->m_rejectOnFail = ਸੱਚ;
            ਵਾਪਸ {livep, ਖਾਲੀ_ਸੂਚਕ, {}};
        }
        ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ hi = getConstUInt(nodep->hiBoundp());
        // Strong s_always[m:n]: mark every in-window registered vertex so an
        // attempt still mid-window at end-of-simulation is reported as a liveness
        // failure (IEEE strong: the n+1 ticks must exist). An attempt that has
        // completed earlier in the trace has already cleared its state, so it is
        // not flagged; an attempt whose final tick coincides with $finish is still
        // flagged, matching the strong reference. Weak always[m:n] is not marked.
        VL_RESTORER(m_markStrongPending);
        m_markStrongPending = nodep->isStrong();
        // Check the first in-window tick, then reuse a guarded fixed-delay ring
        // for the remaining ticks instead of creating one state per cycle.
        Sva_State_ਸ਼ੀਰਸ਼* currentp = ਜੋੜੋ_Delay_Chain(ਇੰਦਰਾਜ_Vtxp, lo, flp);
        Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ checkp = scoped_Create_ਸ਼ੀਰਸ਼();
        Sva_Trans_ਕਿਨਾਰਾ* ਅਚਲ linkp
            = guardedLink(currentp, checkp, sampled(propp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)), flp);
        ਜੇ (is_ਸਿਖਰ_Level_ਕਦਮ) linkp->m_rejectOnFail = ਸੱਚ;
        currentp = checkp;
        m_temporal_Guard_ਢੇਰ.ਧੱਕੋ_ਪਿੱਠ(propp);
        currentp = ਜੋੜੋ_Delay_Chain(currentp, hi - lo, flp);
        ਜੇ (nodep->isStrong() && currentp->m_delay_Ring_ਮਾਪ) currentp->m_strongAlwaysRing = ਸੱਚ;
        m_temporal_Guard_ਢੇਰ.ਕੱਢੋ_ਪਿੱਠ();
        ਵਾਪਸ {currentp, propp, {}};
    }

    Build_ਨਤੀਜਾ buildGotoRep(AstSGotoRep* repp, Sva_State_ਸ਼ੀਰਸ਼* ਇੰਦਰਾਜ_Vtxp) {
        File_ਸਤਰ* ਅਚਲ flp = repp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = repp->exprp();
        ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ minN = getConstUInt(repp->countp());
        ਜੇ (minN == 0) ਵਾਪਸ Build_ਨਤੀਜਾ::fail();
        ਅਚਲ ਤਾਰਕਿਕ hasMax = repp->maxCountp() != ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ maxN = hasMax ? getConstUInt(repp->maxCountp()) : minN;
        ਜੇ (m_isCoverSeq) {
            // Several attempts may coalesce in the one-bit wait vertex before reaching the ring,
            // so a cover sequence action block could run too few times.
            warnEndpointUnsupported(flp, "a goto repetition");
            ਵਾਪਸ Build_ਨਤੀਜਾ::failWithError();
        }

        AstVar* ਅਚਲ hoistVarp = tryHoistSampled(exprp, flp, 2);

        // The first guardedEdge is the ##1 before waiting for a match. In the wait state, false
        // takes the clocked self-loop, while true takes the zero-delay guardedLink on that tick.
        Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ waitVtxp = scoped_Create_ਸ਼ੀਰਸ਼();
        guarded_ਕਿਨਾਰਾ(ਇੰਦਰਾਜ_Vtxp, waitVtxp, flp);
        guarded_ਕਿਨਾਰਾ(waitVtxp, waitVtxp,
                    ਨਵਾਂ AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)}, flp);
        Sva_State_ਸ਼ੀਰਸ਼* currentp = scoped_Create_ਸ਼ੀਰਸ਼();
        guardedLink(waitVtxp, currentp, sampledRefOrClone(hoistVarp, exprp, flp), flp);

        ਜੇ (minN > 1) {
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ ringVtxp = ਜੋੜੋ_Delay_Chain(
                currentp, minN - 1, flp, ਸੱਚ, ਖਾਲੀ_ਸੂਚਕ, sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = ਸੱਚ;
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ checkVtxp = scoped_Create_ਸ਼ੀਰਸ਼();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            currentp = checkVtxp;
        }
        ਜੇ (!hasMax) {
            currentp->m_isUnbounded = ਸੱਚ;  // [->N] waits unboundedly
            m_ਵਿੱਚ_Unbounded_Scope = ਸੱਚ;
            ਵਾਪਸ {currentp, ਖਾਲੀ_ਸੂਚਕ, {}};
        }

        // [->M:N]: the range ring holds matches from M through N and advances
        // only on expr, preserving arbitrarily long gaps between occurrences.
        Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ ਮਿਲਾਓ_Vtxp = scoped_Create_ਸ਼ੀਰਸ਼();
        guardedLink(currentp, ਮਿਲਾਓ_Vtxp, flp);  // accept at match_M
        ਜੇ (maxN > minN) {
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ ringVtxp
                = ਜੋੜੋ_Delay_Chain(currentp, maxN - minN + 1, flp, ਝੂਠ, ਖਾਲੀ_ਸੂਚਕ,
                                sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = ਸੱਚ;
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ checkVtxp = scoped_Create_ਸ਼ੀਰਸ਼();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            guardedLink(checkVtxp, ਮਿਲਾਓ_Vtxp, flp);
        }
        ਮਿਲਾਓ_Vtxp->m_isUnbounded = ਸੱਚ;  // [->M:N] still has unbounded waits between matches
        m_ਵਿੱਚ_Unbounded_Scope = ਸੱਚ;
        ਵਾਪਸ {ਮਿਲਾਓ_Vtxp, ਖਾਲੀ_ਸੂਚਕ, {}};
    }

    // True if the sequence has a zero-minimum consecutive repetition, which may match empty
    ਸਥਿਰ ਤਾਰਕਿਕ hasZeroMinRep(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* seqp) {
        ਵਾਪਸ seqp->exists(
            [](ਅਚਲ AstSConsRep* repp) { ਵਾਪਸ getConstUInt(repp->countp()) == 0; });
    }

    // True if a leaf's condition has no implication or multi-cycle sequence in it
    // (a leaf can be e.g. 'not (x |-> y)')
    ਸਥਿਰ ਤਾਰਕਿਕ isPlainBoolean(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp) {
        ਵਾਪਸ !condp->exists(
            [](ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* np) { ਵਾਪਸ np->is_Multi_ਚੱਕਰ_Sva() || VN_IS(np, Implication); });
    }

    // Build merge vertex for SOr / LogOr: both branches feed into one vertex.
    Build_ਨਤੀਜਾ build_Or_ਮਿਲਾਓ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, Sva_State_ਸ਼ੀਰਸ਼* ਇੰਦਰਾਜ_Vtxp,
                             File_ਸਤਰ* flp, ਤਾਰਕਿਕ want_Bool_ਪੱਤੀ) {
        ਅਚਲ Build_ਨਤੀਜਾ lhs = build_ਵਿਅੰਜਕ(lhsp, ਇੰਦਰਾਜ_Vtxp);
        ਅਚਲ Build_ਨਤੀਜਾ rhs = build_ਵਿਅੰਜਕ(rhsp, ਇੰਦਰਾਜ_Vtxp);
        ਜੇ (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ਵਾਪਸ Build_ਨਤੀਜਾ::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP
        // IEEE 1800-2023 16.14.3: a cover sequence counts every end-of-match. A
        // sequence operand of 'or' can end more than once, but only its final
        // end reaches the merge vertex below, so reject sequence operands rather
        // than under-count. Plain boolean disjunction has one end per cycle and
        // is handled by the OR-fold.
        ਅਚਲ ਤਾਰਕਿਕ booleanOnly = lhs.ਪਦ_Vertexp == ਇੰਦਰਾਜ_Vtxp && lhs.finalCondp
                                 && rhs.ਪਦ_Vertexp == ਇੰਦਰਾਜ_Vtxp && rhs.finalCondp;
        ਜੇ (m_isCoverSeq && !booleanOnly) {
            warnEndpointUnsupported(flp, "a sequence operand of 'or'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ਵਾਪਸ Build_ਨਤੀਜਾ::failWithError();
        }
        ਜੇ (booleanOnly && want_Bool_ਪੱਤੀ) {
            ਜੇ (isPlainBoolean(lhs.finalCondp) && isPlainBoolean(rhs.finalCondp)) {
                // As a |-> / |=> or top-level body, a disjunction of two plain booleans must
                // be able to reject: return it as a boolean finalCondp. The merge vertex
                // below drops the attempt silently when both operands are false.
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ orCondp
                    = ਨਵਾਂ AstLogOr{flp, lhs.finalCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                                   rhs.finalCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                freeUnlinkedCondp(lhs.finalCondp);
                freeUnlinkedCondp(rhs.finalCondp);
                ਵਾਪਸ {ਇੰਦਰਾਜ_Vtxp, orCondp, {}};
            }
        }
        Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ ਮਿਲਾਓ_Vtxp = scoped_Create_ਸ਼ੀਰਸ਼();
        ਜੇ (booleanOnly) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsp,
                        "Single-cycle SOr operands must have finalCondp");
            guardedLink(ਇੰਦਰਾਜ_Vtxp, ਮਿਲਾਓ_Vtxp,
                        ਨਵਾਂ AstLogOr{flp, sampled(lhs.finalCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)),
                                     sampled(rhs.finalCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ))},
                        flp);
        } ਨਹੀਂਤਾਂ {
            ਜੇ (lhs.finalCondp) {
                guardedLink(lhs.ਪਦ_Vertexp, ਮਿਲਾਓ_Vtxp,
                            sampled(lhs.finalCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)), flp);
            } ਨਹੀਂਤਾਂ {
                guardedLink(lhs.ਪਦ_Vertexp, ਮਿਲਾਓ_Vtxp, flp);
            }
            ਜੇ (rhs.finalCondp) {
                guardedLink(rhs.ਪਦ_Vertexp, ਮਿਲਾਓ_Vtxp,
                            sampled(rhs.finalCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)), flp);
            } ਨਹੀਂਤਾਂ {
                guardedLink(rhs.ਪਦ_Vertexp, ਮਿਲਾਓ_Vtxp, flp);
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        ਵਾਪਸ {ਮਿਲਾਓ_Vtxp, ਖਾਲੀ_ਸੂਚਕ, {}};
    }

    // Build done-latch combiner for SAnd/SIntersect (IEEE 1800-2023 16.9.5).
    Build_ਨਤੀਜਾ buildAndCombiner(Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsExprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsExprp,
                                 Sva_State_ਸ਼ੀਰਸ਼* ਇੰਦਰਾਜ_Vtxp, File_ਸਤਰ* flp) {
        // Snapshot-restore scope so LHS liveness does not leak into RHS.
        ਅਚਲ ਤਾਰਕਿਕ savedScope = m_ਵਿੱਚ_Unbounded_Scope;
        ਅਚਲ Build_ਨਤੀਜਾ lhs = build_ਵਿਅੰਜਕ(lhsExprp, ਇੰਦਰਾਜ_Vtxp);
        ਅਚਲ ਤਾਰਕਿਕ lhsScope = m_ਵਿੱਚ_Unbounded_Scope;
        m_ਵਿੱਚ_Unbounded_Scope = savedScope;
        ਅਚਲ Build_ਨਤੀਜਾ rhs = build_ਵਿਅੰਜਕ(rhsExprp, ਇੰਦਰਾਜ_Vtxp);
        ਅਚਲ ਤਾਰਕਿਕ rhsScope = m_ਵਿੱਚ_Unbounded_Scope;
        m_ਵਿੱਚ_Unbounded_Scope = savedScope || lhsScope || rhsScope;
        ਜੇ (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ਵਾਪਸ Build_ਨਤੀਜਾ::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP

        // Single-cycle operands: use boolean AND (done-latch would fire across cycles).
        // If both operands stayed at entry, they must be boolean leaves which
        // buildExpr returns with finalCondp=nodep (non-null).
        ਜੇ (lhs.ਪਦ_Vertexp == ਇੰਦਰਾਜ_Vtxp && rhs.ਪਦ_Vertexp == ਇੰਦਰਾਜ_Vtxp) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsExprp,
                        "Single-cycle SAnd operands must have finalCondp");
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = ਨਵਾਂ AstLogAnd{flp, lhs.finalCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                                                     rhs.finalCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ਵਾਪਸ {ਇੰਦਰਾਜ_Vtxp, condp, {}};
        }
        // Range-delay mid-window sources in either sub-branch would need
        // to be folded into the latch's match-now signal, which the
        // current combiner does not support.
        ਜੇ (!lhs.midSources.ਖਾਲੀ_ਥਾਵਾਂ() || !rhs.midSources.ਖਾਲੀ_ਥਾਵਾਂ()) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: ranged cycle delay in an operand of property 'and'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ਵਾਪਸ Build_ਨਤੀਜਾ::failWithError();
        }
        Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ combVtxp = scoped_Create_ਸ਼ੀਰਸ਼();
        combVtxp->m_isAndCombiner = ਸੱਚ;
        combVtxp->m_andLhsTermp = lhs.ਪਦ_Vertexp;
        combVtxp->m_andRhsTermp = rhs.ਪਦ_Vertexp;
        ਜੇ (lhs.finalCondp) combVtxp->m_andLhsCondp = lhs.finalCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        ਜੇ (rhs.finalCondp) combVtxp->m_andRhsCondp = rhs.finalCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        ਜੇ (lhs.ਪਦ_Vertexp->m_isUnbounded || rhs.ਪਦ_Vertexp->m_isUnbounded) {
            combVtxp->m_isUnbounded = ਸੱਚ;
        }
        // Wire terminal-boolean rejects to a dedicated sink so each side can fail
        // the AND independently. Skip for liveness or single-cycle operands
        // (single-cycle termVertexp == entryVtxp would fire every cycle).
        ਜੇ (!combVtxp->m_isUnbounded) {
            ਤਾਰਕਿਕ needSink = ਝੂਠ;
            ਅਚਲ ਤਾਰਕਿਕ lhs_Multi_ਚੱਕਰ = (lhs.ਪਦ_Vertexp != ਇੰਦਰਾਜ_Vtxp);
            ਅਚਲ ਤਾਰਕਿਕ rhs_Multi_ਚੱਕਰ = (rhs.ਪਦ_Vertexp != ਇੰਦਰਾਜ_Vtxp);
            ਜੇ (lhs.finalCondp && lhs_Multi_ਚੱਕਰ && !lhs.ਪਦ_Vertexp->m_isUnbounded) {
                needSink = ਸੱਚ;
            }
            ਜੇ (rhs.finalCondp && rhs_Multi_ਚੱਕਰ && !rhs.ਪਦ_Vertexp->m_isUnbounded) {
                needSink = ਸੱਚ;
            }
            ਜੇ (needSink) {
                Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ sinkVtxp = m_ਗ੍ਰਾਫ.create_State_ਸ਼ੀਰਸ਼();
                sinkVtxp->m_isRejectSink = ਸੱਚ;
                ਜੇ (lhs.finalCondp && lhs_Multi_ਚੱਕਰ && !lhs.ਪਦ_Vertexp->m_isUnbounded) {
                    Sva_Trans_ਕਿਨਾਰਾ* ਅਚਲ ep = m_ਗ੍ਰਾਫ.ਜੋੜੋ_Link(
                        lhs.ਪਦ_Vertexp, sinkVtxp, sampled(lhs.finalCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)));
                    ep->m_rejectOnFail = ਸੱਚ;
                }
                ਜੇ (rhs.finalCondp && rhs_Multi_ਚੱਕਰ && !rhs.ਪਦ_Vertexp->m_isUnbounded) {
                    Sva_Trans_ਕਿਨਾਰਾ* ਅਚਲ ep = m_ਗ੍ਰਾਫ.ਜੋੜੋ_Link(
                        rhs.ਪਦ_Vertexp, sinkVtxp, sampled(rhs.finalCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)));
                    ep->m_rejectOnFail = ਸੱਚ;
                }
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        ਵਾਪਸ {combVtxp, ਖਾਲੀ_ਸੂਚਕ, {}};
    }

    // Lower `seq1 within seq2` (IEEE 1800-2023 16.9.10) as:
    //   (OR_{i in 0..slack} 1 ##i seq1 ##(slack-i) 1) intersect seq2
    // Both operands must have fixed length (no ranged cycle delays).
    // The OR lives inside a single SIntersect so one AndCombiner done-latch
    // serves all offsets; lifting it out would double-count matches where
    // multiple offsets accept on the same seq2 end cycle.
    Build_ਨਤੀਜਾ buildSWithin(AstSWithin* nodep, Sva_State_ਸ਼ੀਰਸ਼* ਇੰਦਰਾਜ_Vtxp,
                             ਤਾਰਕਿਕ is_ਸਿਖਰ_Level_ਕਦਮ = ਝੂਠ) {
        ਅਚਲ ਪੂਰਨ innerLen = fixed_ਲੰਮਾਈ(nodep->lhsp());
        ਅਚਲ ਪੂਰਨ outerLen = fixed_ਲੰਮਾਈ(nodep->rhsp());
        ਜੇ (innerLen < 0 || outerLen < 0) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: within with ranged cycle-delay operand");
            ਵਾਪਸ Build_ਨਤੀਜਾ::failWithError();
        }
        ਜੇ (innerLen > outerLen) {
            ਵਾਪਸ build_Never_ਮੇਲ_Intersect(
                nodep, ਇੰਦਰਾਜ_Vtxp, is_ਸਿਖਰ_Level_ਕਦਮ,
                "the inner sequence is longer than the outer sequence");
        }
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        ਅਚਲ ਪੂਰਨ slack = outerLen - innerLen;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* innerOrp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਪੂਰਨ i = 0; i <= slack; ++i) {
            ਅਚਲ ਪੂਰਨ postPad = slack - i;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* branchp = nodep->lhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            ਜੇ (i > 0) {
                AstConst* ਅਚਲ prePadp = ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}};
                AstDelay* ਅਚਲ delayp
                    = ਨਵਾਂ AstDelay{flp, ਨਵਾਂ AstConst{flp, static_cast<uint32_t>(i)}, ਸੱਚ};
                AstSExpr* ਅਚਲ wrapped = ਨਵਾਂ AstSExpr{flp, prePadp, delayp, branchp};
                wrapped->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
                branchp = wrapped;
            }
            ਜੇ (postPad > 0) {
                AstConst* ਅਚਲ postTruep = ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}};
                AstDelay* ਅਚਲ delayp
                    = ਨਵਾਂ AstDelay{flp, ਨਵਾਂ AstConst{flp, static_cast<uint32_t>(postPad)}, ਸੱਚ};
                AstSExpr* ਅਚਲ wrapped = ਨਵਾਂ AstSExpr{flp, branchp, delayp, postTruep};
                wrapped->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
                branchp = wrapped;
            }
            innerOrp = innerOrp ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstSOr{flp, innerOrp, branchp})
                                : branchp;
        }
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ outerClonep = nodep->rhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ combinedp = ਨਵਾਂ AstSIntersect{flp, innerOrp, outerClonep};
        Build_ਨਤੀਜਾ ਨਤੀਜਾ = build_ਵਿਅੰਜਕ(combinedp, ਇੰਦਰਾਜ_Vtxp, is_ਸਿਖਰ_Level_ਕਦਮ);
        VL_DO_DANGLING(combinedp->delete_ਰੁੱਖ(), combinedp);
        // When both operands are plain booleans, buildAndCombiner returns a
        // freshly-allocated AstAnd as finalCondp with no parent. Callers up
        // the chain clone-and-discard finalCondp, so leave it parent-less and
        // it leaks; anchor it in the graph now via an edge.
        ਜੇ (ਨਤੀਜਾ.valid() && ਨਤੀਜਾ.finalCondp && !ਨਤੀਜਾ.finalCondp->backp()) {
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ wrapVtxp = scoped_Create_ਸ਼ੀਰਸ਼();
            guardedLink(ਨਤੀਜਾ.ਪਦ_Vertexp, wrapVtxp, sampled(ਨਤੀਜਾ.finalCondp), flp);
            ਨਤੀਜਾ = {wrapVtxp, ਖਾਲੀ_ਸੂਚਕ, ਨਤੀਜਾ.midSources};
        }
        ਵਾਪਸ ਨਤੀਜਾ;
    }

    // Collect the boolean leaf checks of a fixed-length sequence keyed by their
    // clock offset from the start. Returns false for anything other than nested
    // AstSExpr with fixed cycle delays over boolean leaves (e.g. throughout).
    ਸਥਿਰ ਤਾਰਕਿਕ flattenFixedSeq(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, ਪੂਰਨ ਅਧਾਰ_ਵਿੱਥ,
                                std::map<ਪੂਰਨ, std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>>& ਬਾਹਰਲਾ) {
        ਜੇ (AstSExpr* ਅਚਲ sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ਅਚਲ delayp = VN_CAST(sexprp->delayp(), Delay);
            ਜੇ (!delayp || !delayp->is_ਚੱਕਰ_Delay() || delayp->isUnbounded()) ਵਾਪਸ ਝੂਠ;
            ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ delay_ਚੱਕਰਾਂ = getConstUInt(delayp->lhsp());
            ਜੇ (delayp->isRangeDelay() && getConstUInt(delayp->rhsp()) != delay_ਚੱਕਰਾਂ)
                ਵਾਪਸ ਝੂਠ;
            ਪੂਰਨ preLen = 0;
            ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ prep = sexprp->preExprp()) {
                ਜੇ (!flattenFixedSeq(prep, ਅਧਾਰ_ਵਿੱਥ, ਬਾਹਰਲਾ)) ਵਾਪਸ ਝੂਠ;
                preLen = fixed_ਲੰਮਾਈ(prep);
                ਜੇ (preLen < 0) ਵਾਪਸ ਝੂਠ;
            }
            ਵਾਪਸ flattenFixedSeq(sexprp->exprp(), ਅਧਾਰ_ਵਿੱਥ + preLen + delay_ਚੱਕਰਾਂ, ਬਾਹਰਲਾ);
        }
        ਜੇ (nodep->is_Multi_ਚੱਕਰ_Sva()) ਵਾਪਸ ਝੂਠ;
        ਬਾਹਰਲਾ[ਅਧਾਰ_ਵਿੱਥ].ਧੱਕੋ_ਪਿੱਠ(nodep);
        ਵਾਪਸ ਸੱਚ;
    }

    // Conjoin two equal-length fixed sequences into one: at each clock offset
    // AND the boolean checks of both operands (IEEE 1800-2023 16.9.6 -- both
    // operands match the same window). Returns null if either operand is not a
    // plain fixed sequence of boolean leaves.
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* conjoinFixedSeqs(Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, File_ਸਤਰ* flp) {
        std::map<ਪੂਰਨ, std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>> checks;
        ਜੇ (!flattenFixedSeq(lhsp, 0, checks) || !flattenFixedSeq(rhsp, 0, checks)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (checks.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* resultp = ਖਾਲੀ_ਸੂਚਕ;
        ਪੂਰਨ prev_ਵਿੱਥ = 0;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਵਿੱਥ_Checks : checks) {
            ਅਚਲ ਪੂਰਨ ਵਿੱਥ = ਵਿੱਥ_Checks.ਪਹਿਲਾ;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ leafp : ਵਿੱਥ_Checks.ਦੂਜਾ) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ clonep = leafp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                ਜੇ (!condp) {
                    condp = clonep;
                } ਨਹੀਂਤਾਂ {
                    condp = ਨਵਾਂ AstLogAnd{flp, condp, clonep};
                    condp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
                }
            }
            ਜੇ (!resultp) {
                ਜੇ (ਵਿੱਥ > 0) {
                    AstDelay* ਅਚਲ delayp = ਨਵਾਂ AstDelay{
                        flp, ਨਵਾਂ AstConst{flp, static_cast<uint32_t>(ਵਿੱਥ)}, /*isCycle=*/ਸੱਚ};
                    resultp
                        = ਨਵਾਂ AstSExpr{flp, ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}, delayp, condp};
                    resultp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
                } ਨਹੀਂਤਾਂ {
                    resultp = condp;
                }
            } ਨਹੀਂਤਾਂ {
                AstDelay* ਅਚਲ delayp = ਨਵਾਂ AstDelay{
                    flp, ਨਵਾਂ AstConst{flp, static_cast<uint32_t>(ਵਿੱਥ - prev_ਵਿੱਥ)},
                    /*isCycle=*/ਸੱਚ};
                resultp = ਨਵਾਂ AstSExpr{flp, resultp, delayp, condp};
                resultp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
            }
            prev_ਵਿੱਥ = ਵਿੱਥ;
        }
        ਵਾਪਸ resultp;
    }

    // `seq` is a simple ranged sequence `start ##[m:n] end` (start/end boolean,
    // start may be absent). Used to collapse a both-variable intersect to one
    // ranged delay.
    ਬਣਤਰ SimpleRanged final {
        ਤਾਰਕਿਕ ok = ਝੂਠ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* startp = ਖਾਲੀ_ਸੂਚਕ;  // may be null (absent start)
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* endp = ਖਾਲੀ_ਸੂਚਕ;
    };
    ਸਥਿਰ SimpleRanged asSimpleRanged(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        AstSExpr* ਅਚਲ sexprp = VN_CAST(nodep, SExpr);
        ਜੇ (!sexprp) ਵਾਪਸ {};
        AstDelay* ਅਚਲ delayp = VN_CAST(sexprp->delayp(), Delay);
        ਜੇ (!delayp || !delayp->is_ਚੱਕਰ_Delay() || !delayp->isRangeDelay() || delayp->isUnbounded())
            ਵਾਪਸ {};
        ਜੇ (getConstUInt(delayp->lhsp()) == getConstUInt(delayp->rhsp())) ਵਾਪਸ {};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ prep = sexprp->preExprp();
        ਜੇ (prep && fixed_ਲੰਮਾਈ(prep) != 0) ਵਾਪਸ {};
        ਜੇ (fixed_ਲੰਮਾਈ(sexprp->exprp()) != 0) ਵਾਪਸ {};
        ਵਾਪਸ {ਸੱਚ, prep, sexprp->exprp()};
    }

    // Build the NFA for a synthesized intersect lowering tree, then free it.
    // buildExpr returns the terminal condition (finalCondp) by reference into the
    // tree; detach a clone so the tree can be freed here. The graph already holds
    // clones/hoists of every edge condition, so nothing else dangles.
    Build_ਨਤੀਜਾ build_From_Lowering_ਰੁੱਖ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* treep, Sva_State_ਸ਼ੀਰਸ਼* ਇੰਦਰਾਜ_Vtxp,
                                      ਤਾਰਕਿਕ is_ਸਿਖਰ_Level_ਕਦਮ) {
        Build_ਨਤੀਜਾ ਨਤੀਜਾ = build_ਵਿਅੰਜਕ(treep, ਇੰਦਰਾਜ_Vtxp, is_ਸਿਖਰ_Level_ਕਦਮ);
        ਜੇ (ਨਤੀਜਾ.valid() && ਨਤੀਜਾ.finalCondp) {
            ਨਤੀਜਾ.finalCondp = ਨਤੀਜਾ.finalCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        }
        VL_DO_DANGLING(treep->delete_ਰੁੱਖ(), treep);
        ਵਾਪਸ ਨਤੀਜਾ;
    }

    // Empty common-length intersection -- unequal fixed lengths, or disjoint
    // ranged lengths. IEEE 1800-2023 16.9.6 requires both operands to match
    // over a window of the same length, so with no common length the intersect
    // simply never matches. This is legal (matching nothing), not an error, so
    // lower to a constant false rather than rejecting legal code.
    Build_ਨਤੀਜਾ build_Never_ਮੇਲ_Intersect(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, Sva_State_ਸ਼ੀਰਸ਼* ਇੰਦਰਾਜ_Vtxp,
                                         ਤਾਰਕਿਕ is_ਸਿਖਰ_Level_ਕਦਮ, ਅਚਲ ਅੱਖਰ* reason) {
        nodep->v3warn(NEVERMATCH, "Sequence can never match because " << reason << ".");
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ falsep = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
        ਵਾਪਸ build_From_Lowering_ਰੁੱਖ(falsep, ਇੰਦਰਾਜ_Vtxp, is_ਸਿਖਰ_Level_ਕਦਮ);
    }

    // Lower `seq1 intersect seq2` when an operand's match length varies
    // (IEEE 1800-2023 16.9.6: both match over one window, equal start and end).
    // The common length range is [lo,hi] = intersection of the two operands'
    // achievable lengths. The equal-length combiner is avoided -- it mis-handles
    // operands with an internal boolean check -- by lowering to plain sequences:
    //  - lo == hi (one shared length, e.g. one fixed + one ranged operand):
    //    pin each operand to that length and conjoin them cycle-by-cycle into a
    //    single fixed sequence.
    //  - lo < hi with simple `bool ##[m:n] bool` operands: collapse to one
    //    ranged delay `(start1 & start2) ##[lo:hi] (end1 & end2)`. (An OR of
    //    per-length branches cannot reject correctly -- a single missed length
    //    would fail the whole intersect, cf. Lesson 48.)
    //  - otherwise unsupported (clean error, not the legacy fall-through crash).
    Build_ਨਤੀਜਾ buildVarLenIntersect(AstSIntersect* nodep, Sva_State_ਸ਼ੀਰਸ਼* ਇੰਦਰਾਜ_Vtxp,
                                     ਤਾਰਕਿਕ is_ਸਿਖਰ_Level_ਕਦਮ) {
        ਅਚਲ std::pair<ਪੂਰਨ, ਪੂਰਨ> lhsRange = ਲੰਮਾਈ_Range(nodep->lhsp());
        ਅਚਲ std::pair<ਪੂਰਨ, ਪੂਰਨ> rhsRange = ਲੰਮਾਈ_Range(nodep->rhsp());
        ਜੇ (lhsRange.ਪਹਿਲਾ < 0 || rhsRange.ਪਹਿਲਾ < 0) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect with this variable-length operand");
            ਵਾਪਸ Build_ਨਤੀਜਾ::failWithError();
        }
        ਅਚਲ ਪੂਰਨ lo = std::max(lhsRange.ਪਹਿਲਾ, rhsRange.ਪਹਿਲਾ);
        ਅਚਲ ਪੂਰਨ hi = std::min(lhsRange.ਦੂਜਾ, rhsRange.ਦੂਜਾ);
        ਜੇ (lo > hi) {
            // Disjoint length ranges share no common length -> never matches.
            ਵਾਪਸ build_Never_ਮੇਲ_Intersect(nodep, ਇੰਦਰਾਜ_Vtxp, is_ਸਿਖਰ_Level_ਕਦਮ,
                                            "intersect operands have no common length");
        }
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        ਜੇ (lo == hi) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lp = realize_At_ਲੰਮਾਈ(nodep->lhsp(), lo, lhsRange.ਪਹਿਲਾ);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rp = realize_At_ਲੰਮਾਈ(nodep->rhsp(), lo, rhsRange.ਪਹਿਲਾ);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ conjp = conjoinFixedSeqs(lp, rp, flp);
            VL_DO_DANGLING(lp->delete_ਰੁੱਖ(), lp);
            VL_DO_DANGLING(rp->delete_ਰੁੱਖ(), rp);
            ਜੇ (!conjp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: intersect operand is not a plain boolean sequence");
                ਵਾਪਸ Build_ਨਤੀਜਾ::failWithError();
            }
            ਵਾਪਸ build_From_Lowering_ਰੁੱਖ(conjp, ਇੰਦਰਾਜ_Vtxp, is_ਸਿਖਰ_Level_ਕਦਮ);
        }
        ਅਚਲ SimpleRanged sl = asSimpleRanged(nodep->lhsp());
        ਅਚਲ SimpleRanged sr = asSimpleRanged(nodep->rhsp());
        ਜੇ (!sl.ok || !sr.ok) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect of two sequences that each vary in length over a"
                          " range with internal structure");
            ਵਾਪਸ Build_ਨਤੀਜਾ::failWithError();
        }
        ਅਚਲ ਸਵੈਚਾਲਿਤ andBool = [&](Ast_ਗੰਢ_ਵਿਅੰਜਕ* ap, Ast_ਗੰਢ_ਵਿਅੰਜਕ* bp) -> Ast_ਗੰਢ_ਵਿਅੰਜਕ* {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ aClonep
                = ap ? ap->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ) : ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ bClonep
                = bp ? bp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ) : ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}};
            AstLogAnd* ਅਚਲ andp = ਨਵਾਂ AstLogAnd{flp, aClonep, bClonep};
            andp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
            ਵਾਪਸ andp;
        };
        AstDelay* ਅਚਲ delayp = ਨਵਾਂ AstDelay{flp, ਨਵਾਂ AstConst{flp, static_cast<uint32_t>(lo)},
                                              /*isCycle=*/ਸੱਚ};
        delayp->rhsp(ਨਵਾਂ AstConst{flp, static_cast<uint32_t>(hi)});
        AstSExpr* ਅਚਲ reducedp
            = ਨਵਾਂ AstSExpr{flp, andBool(sl.startp, sr.startp), delayp, andBool(sl.endp, sr.endp)};
        reducedp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        ਵਾਪਸ build_From_Lowering_ਰੁੱਖ(reducedp, ਇੰਦਰਾਜ_Vtxp, is_ਸਿਖਰ_Level_ਕਦਮ);
    }

    Build_ਨਤੀਜਾ buildThroughout(AstSThroughout* nodep, Sva_State_ਸ਼ੀਰਸ਼* ਇੰਦਰਾਜ_Vtxp,
                                ਤਾਰਕਿਕ is_ਸਿਖਰ_Level_ਕਦਮ = ਝੂਠ) {
        // Mark entryVtxp so "cond false at tick 0" is detected as throughout-drop.
        ਇੰਦਰਾਜ_Vtxp->m_throughoutConds.ਧੱਕੋ_ਪਿੱਠ(nodep->lhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
        m_temporal_Guard_ਢੇਰ.ਧੱਕੋ_ਪਿੱਠ(nodep->lhsp());
        ਅਚਲ Build_ਨਤੀਜਾ ਨਤੀਜਾ = build_ਵਿਅੰਜਕ(nodep->rhsp(), ਇੰਦਰਾਜ_Vtxp, is_ਸਿਖਰ_Level_ਕਦਮ);
        m_temporal_Guard_ਢੇਰ.ਕੱਢੋ_ਪਿੱਠ();
        ਵਾਪਸ ਨਤੀਜਾ;
    }

    // until / until_with per IEEE 1800-2023 16.12.12.
    // Topology: combinational wait vertex with self-feeding state register.
    //   entry  --link[T]--> waitC
    //   waitR  --link[T]--> waitC                          (back-loop)
    //   waitC  --edge[##1, sampled(p) && !sampled(q)]--> waitR  (continue)
    //   waitC  --link[REQUIRE, rejectOnFail]--> sink            (per-cycle fail)
    //   waitC  --link[T]--> match  (added by wireMatchAndMidSources;
    //                               accept condition rides via finalCondp)
    // waitC is m_isUnbounded so the terminal-match link contributes only to
    // terminalActive, not to rejectBase (which would otherwise spuriously fire
    // every cycle q is false). Per-cycle reject comes from the explicit
    // rejectOnFail link to the sink vertex.
    //
    // Non-overlapping (p until q):
    //   REQUIRE = sampled(p) || sampled(q)   accept = sampled(q)
    // Overlapping (p until_with q):
    //   REQUIRE = sampled(p)                 accept = sampled(p) && sampled(q)
    // Strong forms use the same checks and mark the registered wait state as
    // an end-of-simulation liveness obligation.
    Build_ਨਤੀਜਾ buildUntil(AstUntil* nodep, Sva_State_ਸ਼ੀਰਸ਼* ਇੰਦਰਾਜ_Vtxp, ਤਾਰਕਿਕ is_ਸਿਖਰ_Level_ਕਦਮ) {
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        ਜੇ (!is_ਸਿਖਰ_Level_ਕਦਮ) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            ਵਾਪਸ Build_ਨਤੀਜਾ::failWithError();
        }
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp();
        ਅਚਲ ਸਵੈਚਾਲਿਤ hasSeq = [](ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ep) {
            ਵਾਪਸ ep->exists([](ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* np) { ਵਾਪਸ np->is_Multi_ਚੱਕਰ_Sva(); });
        };
        ਜੇ (hasSeq(lhsp) || hasSeq(rhsp)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            ਵਾਪਸ Build_ਨਤੀਜਾ::failWithError();
        }

        ਅਚਲ ਤਾਰਕਿਕ ov = nodep->isOverlapping();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsBitp = nodep->lhsp();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsBitp = nodep->rhsp();
        // p hoist count: continue, require (ov: 1 use; nov: 1 use). At least 2 uses.
        AstVar* ਅਚਲ pHoistp = tryHoistSampled(lhsBitp, flp, 2);
        // q hoist count: continue (1) + require nov (1) = 2; ov: continue only (1).
        AstVar* ਅਚਲ qHoistp = ov ? ਖਾਲੀ_ਸੂਚਕ : tryHoistSampled(rhsBitp, flp, 2);

        Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ waitCp = scoped_Create_ਸ਼ੀਰਸ਼();
        Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ waitRp = scoped_Create_ਸ਼ੀਰਸ਼();
        waitCp->m_isUnbounded = ਸੱਚ;
        waitRp->m_strongPending = nodep->isStrong() && !m_isCover;

        // Entry and back-loop Links carry no condition; throughout-folding still applies.
        guardedLink(ਇੰਦਰਾਜ_Vtxp, waitCp, flp);
        guardedLink(waitRp, waitCp, flp);

        // Continue clocked edge: p && !q advances to next-cycle wait.
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ contCondp
            = ਨਵਾਂ AstLogAnd{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                            ਨਵਾਂ AstLogNot{flp, sampledRefOrClone(qHoistp, rhsBitp, flp)}};
        guarded_ਕਿਨਾਰਾ(waitCp, waitRp, contCondp, flp);

        // Reject sink: fires when require-condition is false.
        Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ sinkVtxp = m_ਗ੍ਰਾਫ.create_State_ਸ਼ੀਰਸ਼();
        sinkVtxp->m_isRejectSink = ਸੱਚ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* requireCondp;
        ਜੇ (ov) {
            requireCondp = sampledRefOrClone(pHoistp, lhsBitp, flp);
        } ਨਹੀਂਤਾਂ {
            requireCondp = ਨਵਾਂ AstLogOr{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                                        sampledRefOrClone(qHoistp, rhsBitp, flp)};
        }
        Sva_Trans_ਕਿਨਾਰਾ* ਅਚਲ rejEdgep = m_ਗ੍ਰਾਫ.ਜੋੜੋ_Link(waitCp, sinkVtxp, requireCondp);
        rejEdgep->m_rejectOnFail = ਸੱਚ;

        // Accept condition rides via finalCondp; assembleResult $sampled-wraps it.
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* acceptCondp;
        ਜੇ (ov) {
            acceptCondp
                = ਨਵਾਂ AstLogAnd{flp, lhsBitp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), rhsBitp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
        } ਨਹੀਂਤਾਂ {
            acceptCondp = rhsBitp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        }
        ਵਾਪਸ {waitCp, acceptCondp, {}};
    }

    // IEEE 1800-2023 16.12.14 property abort operators. Sync and async share
    // the same NFA encoding: AstSampled already gives matured values at every
    // maturing clocking event, and async firing "between clocks" is not
    // observable in a cycle-based model. VAbortKind selects accept vs reject
    // verdict (sync vs async only changes the user-visible spelling).

    // Build `condp && !outer_1 && !outer_2 ...` (unsampled).
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* abort_Fire_ਵਿਅੰਜਕ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp, File_ਸਤਰ* flp) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* resultp = condp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ op : m_outer_Abort_ਢੇਰ)
            resultp = ਨਵਾਂ AstLogAnd{flp, resultp, ਨਵਾਂ AstLogNot{flp, op->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)}};
        ਵਾਪਸ resultp;
    }

    // True when unguarded ring-wide rejection or a same-tick Link chain already accounts the
    // attempt: a required-step Link covers both outcomes; followed-by pairs both edges.
    ਸਥਿਰ ਤਾਰਕਿਕ chain_Accounts_ਸਰੋਤ(ਅਚਲ Sva_State_ਸ਼ੀਰਸ਼* srcp,
                                    ਅਚਲ std::unordered_ਸੈੱਟ<ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ*>& pre_ਕਿਨਾਰੇ) {
        ਜੇ (srcp->m_delay_Ring_ਮਾਪ && srcp->m_throughoutConds.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਸੱਚ;
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& edger : srcp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
            ਜੇ (pre_ਕਿਨਾਰੇ.ਗਿਣਤੀ_2(&edger)) ਜਾਰੀ;
            ਅਚਲ Sva_Trans_ਕਿਨਾਰਾ& tedger = static_cast<ਅਚਲ Sva_Trans_ਕਿਨਾਰਾ&>(edger);
            ਜੇ (tedger.m_consumes_ਚੱਕਰ) ਜਾਰੀ;
            ਅਚਲ ਸਵੈਚਾਲਿਤ* ਅਚਲ fromp = static_cast<ਅਚਲ Sva_State_ਸ਼ੀਰਸ਼*>(tedger.fromVtxp());
            ਜੇ (fromp->m_abortRejectp) ਵਾਪਸ ਸੱਚ;
        }
        ਤਾਰਕਿਕ plainNonSink = ਝੂਠ;
        ਤਾਰਕਿਕ markedSink = ਝੂਠ;
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& edger : srcp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            ਜੇ (pre_ਕਿਨਾਰੇ.ਗਿਣਤੀ_2(&edger)) ਜਾਰੀ;
            ਅਚਲ Sva_Trans_ਕਿਨਾਰਾ& tedger = static_cast<ਅਚਲ Sva_Trans_ਕਿਨਾਰਾ&>(edger);
            ਜੇ (tedger.m_consumes_ਚੱਕਰ) ਜਾਰੀ;
            ਅਚਲ ਤਾਰਕਿਕ sink = static_cast<ਅਚਲ Sva_State_ਸ਼ੀਰਸ਼*>(tedger.ਮੰਜ਼ਿਲ_Vtxp())->m_isRejectSink;
            ਜੇ (tedger.m_rejectOnFail) {
                ਜੇ (!sink) ਵਾਪਸ ਸੱਚ;
                markedSink = ਸੱਚ;
            } ਨਹੀਂਤਾਂ ਜੇ (!sink) {
                ਜੇ (srcp->m_abortRejectp) ਵਾਪਸ ਸੱਚ;
                plainNonSink = ਸੱਚ;
            }
        }
        ਵਾਪਸ plainNonSink && markedSink;
    }

    // Reject edge: fires when the source is live and the abort samples true.
    ਖਾਲੀ ਜੋੜੋ_Abort_Reject_ਕਿਨਾਰਾ(Sva_State_ਸ਼ੀਰਸ਼* srcp, Sva_State_ਸ਼ੀਰਸ਼* sinkp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp,
                            File_ਸਤਰ* flp) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ notFirep = ਨਵਾਂ AstLogNot{flp, sampled(abort_Fire_ਵਿਅੰਜਕ(condp, flp))};
        m_ਗ੍ਰਾਫ.ਜੋੜੋ_Link(srcp, sinkp, notFirep)->m_rejectOnFail = ਸੱਚ;
        ਵਾਪਸ;
    }

    // On the fire tick: kill body threads; accept kinds also forgive step misses.
    ਖਾਲੀ gate_Body_ਕਿਨਾਰੇ_On_Abort(ਅਚਲ std::unordered_ਸੈੱਟ<ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ*>& pre_ਕਿਨਾਰੇ,
                              Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp, VAbort_ਕਿਸਮ ਕਿਸਮ, File_ਸਤਰ* flp) {
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtxr : m_ਗ੍ਰਾਫ.m_ਗ੍ਰਾਫ.vertices()) {
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& edger : vtxr.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                ਜੇ (pre_ਕਿਨਾਰੇ.ਗਿਣਤੀ_2(&edger)) ਜਾਰੀ;
                Sva_Trans_ਕਿਨਾਰਾ* ਅਚਲ tedgep = static_cast<Sva_Trans_ਕਿਨਾਰਾ*>(&edger);
                ਜੇ (tedgep->m_rejectOnFail) {
                    ਜੇ (!ਕਿਸਮ.isAccept()) ਜਾਰੀ;
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ firep = sampled(abort_Fire_ਵਿਅੰਜਕ(condp, flp));
                    tedgep->m_condp
                        = tedgep->m_condp ? ਨਵਾਂ AstLogOr{flp, tedgep->m_condp, firep} : firep;
                } ਨਹੀਂਤਾਂ ਜੇ (tedgep->m_consumes_ਚੱਕਰ) {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ notFirep
                        = ਨਵਾਂ AstLogNot{flp, sampled(abort_Fire_ਵਿਅੰਜਕ(condp, flp))};
                    tedgep->m_condp = tedgep->m_condp
                                          ? ਨਵਾਂ AstLogAnd{flp, tedgep->m_condp, notFirep}
                                          : notFirep;
                }
            }
        }
    }

    Build_ਨਤੀਜਾ buildAbortOn(Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* bodyp, Sva_State_ਸ਼ੀਰਸ਼* ਇੰਦਰਾਜ_Vtxp,
                             VAbort_ਕਿਸਮ ਕਿਸਮ, File_ਸਤਰ* flp, ਤਾਰਕਿਕ is_ਸਿਖਰ_Level_ਕਦਮ) {
        // Snapshot pre-body vertices/edges so post-build diff yields the body's sub-NFA.
        std::unordered_ਸੈੱਟ<ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*> preExisting;
        std::unordered_ਸੈੱਟ<ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ*> pre_ਕਿਨਾਰੇ;
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtxr : m_ਗ੍ਰਾਫ.m_ਗ੍ਰਾਫ.vertices()) {
            preExisting.ਪਾਓ(&vtxr);
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& edger : vtxr.ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) pre_ਕਿਨਾਰੇ.ਪਾਓ(&edger);
        }

        m_outer_Abort_ਢੇਰ.ਧੱਕੋ_ਪਿੱਠ(condp);
        ਅਚਲ Build_ਨਤੀਜਾ body_ਨਤੀਜਾ = build_ਵਿਅੰਜਕ(bodyp, ਇੰਦਰਾਜ_Vtxp, is_ਸਿਖਰ_Level_ਕਦਮ);
        m_outer_Abort_ਢੇਰ.ਕੱਢੋ_ਪਿੱਠ();
        ਜੇ (!body_ਨਤੀਜਾ.valid()) ਵਾਪਸ body_ਨਤੀਜਾ;

        gate_Body_ਕਿਨਾਰੇ_On_Abort(pre_ਕਿਨਾਰੇ, condp, ਕਿਸਮ, flp);

        // Live-thread sources for the abort edge: entry + new body vertices,
        // minus reject sinks (they carry reject fuel, not live-thread fuel).
        std::ਸਦਿਸ਼<Sva_State_ਸ਼ੀਰਸ਼*> abortSources;
        abortSources.ਧੱਕੋ_ਪਿੱਠ(ਇੰਦਰਾਜ_Vtxp);
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtxr : m_ਗ੍ਰਾਫ.m_ਗ੍ਰਾਫ.vertices()) {
            ਜੇ (preExisting.ਗਿਣਤੀ_2(&vtxr)) ਜਾਰੀ;
            ਸਵੈਚਾਲਿਤ* ਅਚਲ sp = static_cast<Sva_State_ਸ਼ੀਰਸ਼*>(&vtxr);
            ਜੇ (sp->m_delay_Ring_ਮਾਪ || !sp->m_throughoutConds.ਖਾਲੀ_ਥਾਵਾਂ()) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ firep = abort_Fire_ਵਿਅੰਜਕ(condp, flp);
                sp->m_abortClearp
                    = sp->m_abortClearp ? ਨਵਾਂ AstLogOr{flp, sp->m_abortClearp, firep} : firep;
            }
            ਜੇ (!ਕਿਸਮ.isAccept()
                && ((sp->m_delay_Ring_ਮਾਪ && sp->m_throughoutConds.ਖਾਲੀ_ਥਾਵਾਂ())
                    || sp->m_replayAbortReject)) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ firep = abort_Fire_ਵਿਅੰਜਕ(condp, flp);
                sp->m_abortRejectp
                    = sp->m_abortRejectp ? ਨਵਾਂ AstLogOr{flp, sp->m_abortRejectp, firep} : firep;
            }
            ਜੇ (sp->m_isRejectSink) ਜਾਰੀ;
            abortSources.ਧੱਕੋ_ਪਿੱਠ(sp);
        }

        ਸਵੈਚਾਲਿਤ sampledAbortFire = [&]() -> Ast_ਗੰਢ_ਵਿਅੰਜਕ* {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਵਿਅੰਜਕ = abort_Fire_ਵਿਅੰਜਕ(condp, flp);
            ਵਾਪਸ sampled(ਵਿਅੰਜਕ);
        };

        ਜੇ (ਕਿਸਮ.isAccept()) {
            // Match-only sink fed by $sampled(abort-fire) from every live source;
            // registered as midSource so it never contributes a reject.
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ acceptSinkp = scoped_Create_ਸ਼ੀਰਸ਼();
            ਲਈ (Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ srcp : abortSources)
                guardedLink(srcp, acceptSinkp, sampledAbortFire(), flp);
            std::ਸਦਿਸ਼<Sva_State_ਸ਼ੀਰਸ਼*> midSources = body_ਨਤੀਜਾ.midSources;
            midSources.ਧੱਕੋ_ਪਿੱਠ(acceptSinkp);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* finalCondp = body_ਨਤੀਜਾ.finalCondp;
            ਜੇ (finalCondp) {
                ਜੇ (finalCondp->backp()) finalCondp = finalCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                finalCondp = ਨਵਾਂ AstLogOr{flp, finalCondp, abort_Fire_ਵਿਅੰਜਕ(condp, flp)};
            }
            ਵਾਪਸ {body_ਨਤੀਜਾ.ਪਦ_Vertexp, finalCondp, std::move(midSources)};
        }

        // rejectOnFail treats m_condp as the success condition and fires on
        // !condp, so the edge carries !sampledAbortFire().
        Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ rejectSinkp = m_ਗ੍ਰਾਫ.create_State_ਸ਼ੀਰਸ਼();
        rejectSinkp->m_isRejectSink = ਸੱਚ;
        ਲਈ (Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ srcp : abortSources)
            ਜੇ (!chain_Accounts_ਸਰੋਤ(srcp, pre_ਕਿਨਾਰੇ))
                ਜੋੜੋ_Abort_Reject_ਕਿਨਾਰਾ(srcp, rejectSinkp, condp, flp);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* finalCondp = body_ਨਤੀਜਾ.finalCondp;
        ਜੇ (finalCondp) {
            ਜੇ (finalCondp->backp()) finalCondp = finalCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            finalCondp
                = ਨਵਾਂ AstLogAnd{flp, finalCondp, ਨਵਾਂ AstLogNot{flp, abort_Fire_ਵਿਅੰਜਕ(condp, flp)}};
        }
        ਵਾਪਸ {body_ਨਤੀਜਾ.ਪਦ_Vertexp, finalCondp, body_ਨਤੀਜਾ.midSources};
    }

ਜਨਤਕ:
    // Free a dropped sub-result condition that is not linked into the AST
    // (abort folds synthesize unparented finalCondp trees).
    ਸਥਿਰ ਖਾਲੀ freeUnlinkedCondp(Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp) {
        ਜੇ (condp && !condp->backp()) VL_DO_DANGLING(condp->delete_ਰੁੱਖ(), condp);
    }

    SvaNfaBuilder(Sva_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ, Ast_ਗੰਢ_Module* modp, V3_Unique_ਨਾਂ& prop_ਆਰਜ਼ੀ_ਨਾਂ,
                  ਤਾਰਕਿਕ isCoverSeq = ਝੂਠ, ਤਾਰਕਿਕ isSeqEvent = ਝੂਠ, ਤਾਰਕਿਕ isCover = ਝੂਠ)
        : m_ਗ੍ਰਾਫ{ਗ੍ਰਾਫ}
        , m_modp{modp}
        , m_prop_ਆਰਜ਼ੀ_ਨਾਂ{prop_ਆਰਜ਼ੀ_ਨਾਂ}
        , m_isCover{isCover}
        , m_isCoverSeq{isCoverSeq}
        , m_isSeqEvent{isSeqEvent} {}

    // Reset scope between antecedent and consequent: liveness must not leak.
    // m_outerAbortStack survives: an abort wrapping the implication covers the
    // consequent too (IEEE 1800-2023 16.12.14).
    ਖਾਲੀ resetScope() {
        m_ਵਿੱਚ_Unbounded_Scope = ਝੂਠ;
        m_temporal_Guard_ਢੇਰ.clear();
    }

    Build_ਨਤੀਜਾ build_ਵਿਅੰਜਕ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, Sva_State_ਸ਼ੀਰਸ਼* ਇੰਦਰਾਜ_Vtxp,
                          ਤਾਰਕਿਕ is_ਸਿਖਰ_Level_ਕਦਮ = ਝੂਠ) {
        VL_RESTORER(m_want_Bool_ਪੱਤੀ);
        ਅਚਲ ਤਾਰਕਿਕ want_Bool_ਪੱਤੀ = m_want_Bool_ਪੱਤੀ;
        m_want_Bool_ਪੱਤੀ = ਝੂਠ;
        ਜੇ (AstSExpr* ਅਚਲ sexprp = VN_CAST(nodep, SExpr)) {
            ਵਾਪਸ buildSExpr(sexprp, ਇੰਦਰਾਜ_Vtxp, is_ਸਿਖਰ_Level_ਕਦਮ);
        }
        ਜੇ (AstSConsRep* ਅਚਲ repp = VN_CAST(nodep, SConsRep)) {
            ਵਾਪਸ buildConsRep(repp, ਇੰਦਰਾਜ_Vtxp, is_ਸਿਖਰ_Level_ਕਦਮ);
        }
        ਜੇ (AstPropAlways* ਅਚਲ alwaysp = VN_CAST(nodep, PropAlways)) {
            ਵਾਪਸ buildPropAlways(alwaysp, ਇੰਦਰਾਜ_Vtxp, is_ਸਿਖਰ_Level_ਕਦਮ);
        }
        ਜੇ (AstSGotoRep* ਅਚਲ repp = VN_CAST(nodep, SGotoRep)) {
            ਵਾਪਸ buildGotoRep(repp, ਇੰਦਰਾਜ_Vtxp);
        }
        ਜੇ (AstSThroughout* ਅਚਲ throughoutp = VN_CAST(nodep, SThroughout)) {
            ਵਾਪਸ buildThroughout(throughoutp, ਇੰਦਰਾਜ_Vtxp, is_ਸਿਖਰ_Level_ਕਦਮ);
        }
        ਜੇ (AstSOr* ਅਚਲ orp = VN_CAST(nodep, SOr)) {
            ਵਾਪਸ build_Or_ਮਿਲਾਓ(orp->lhsp(), orp->rhsp(), ਇੰਦਰਾਜ_Vtxp, orp->fileline(),
                                want_Bool_ਪੱਤੀ);
        }
        ਜੇ (AstLogOr* ਅਚਲ orp = VN_CAST(nodep, LogOr)) {
            ਵਾਪਸ build_Or_ਮਿਲਾਓ(orp->lhsp(), orp->rhsp(), ਇੰਦਰਾਜ_Vtxp, orp->fileline(),
                                want_Bool_ਪੱਤੀ);
        }
        ਜੇ (AstSAnd* ਅਚਲ andp = VN_CAST(nodep, SAnd)) {
            ਵਾਪਸ buildAndCombiner(andp->lhsp(), andp->rhsp(), ਇੰਦਰਾਜ_Vtxp, andp->fileline());
        }
        ਜੇ (AstSIntersect* ਅਚਲ intp = VN_CAST(nodep, SIntersect)) {
            // IEEE 1800-2023 16.9.6: both operands match over one window with
            // equal start and end (equal length). Lower to a single sequence
            // that conjoins both operands' per-cycle checks -- correct under
            // concurrent attempts, where the done-latch combiner conflates the
            // two operands' start times and over-accepts. The combiner remains
            // only as a fallback for operands that do not flatten.
            ਅਚਲ ਪੂਰਨ lhsLen = fixed_ਲੰਮਾਈ(intp->lhsp());
            ਅਚਲ ਪੂਰਨ rhsLen = fixed_ਲੰਮਾਈ(intp->rhsp());
            ਜੇ (lhsLen >= 0 && rhsLen >= 0) {
                ਜੇ (lhsLen != rhsLen) {
                    // Unequal fixed lengths share no common length -> never matches.
                    ਵਾਪਸ build_Never_ਮੇਲ_Intersect(intp, ਇੰਦਰਾਜ_Vtxp, is_ਸਿਖਰ_Level_ਕਦਮ,
                                                    "intersect operands have no common length");
                }
                ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ conjp
                    = conjoinFixedSeqs(intp->lhsp(), intp->rhsp(), intp->fileline())) {
                    ਵਾਪਸ build_From_Lowering_ਰੁੱਖ(conjp, ਇੰਦਰਾਜ_Vtxp, is_ਸਿਖਰ_Level_ਕਦਮ);
                }
                ਵਾਪਸ buildAndCombiner(intp->lhsp(), intp->rhsp(), ਇੰਦਰਾਜ_Vtxp, intp->fileline());
            }
            ਵਾਪਸ buildVarLenIntersect(intp, ਇੰਦਰਾਜ_Vtxp, is_ਸਿਖਰ_Level_ਕਦਮ);
        }
        ਜੇ (AstSWithin* ਅਚਲ withinp = VN_CAST(nodep, SWithin)) {
            ਵਾਪਸ buildSWithin(withinp, ਇੰਦਰਾਜ_Vtxp, is_ਸਿਖਰ_Level_ਕਦਮ);
        }
        ਜੇ (AstAbortOn* ਅਚਲ ap = VN_CAST(nodep, AbortOn)) {
            ਵਾਪਸ buildAbortOn(ap->condp(), ap->propp(), ਇੰਦਰਾਜ_Vtxp, ap->ਕਿਸਮ(), ap->fileline(),
                                is_ਸਿਖਰ_Level_ਕਦਮ);
        }
        ਜੇ (VN_IS(nodep, SNonConsRep)) ਵਾਪਸ Build_ਨਤੀਜਾ::fail();
        ਜੇ (AstImplication* ਅਚਲ implp = VN_CAST(nodep, Implication)) {
            ਵਾਪਸ build_Implication_ਕਿਨਾਰੇ(implp->lhsp(), implp->rhsp(), ਇੰਦਰਾਜ_Vtxp,
                                         implp->isOverlapped(), implp->isFollowedBy(),
                                         implp->lhsp(), implp->fileline());
        }
        ਜੇ (AstUntil* ਅਚਲ untilp = VN_CAST(nodep, Until)) {
            ਵਾਪਸ buildUntil(untilp, ਇੰਦਰਾਜ_Vtxp, is_ਸਿਖਰ_Level_ਕਦਮ);
        }
        // Boolean leaf (including LogAnd): return as finalCond
        ਵਾਪਸ {ਇੰਦਰਾਜ_Vtxp, nodep, {}};
    }

    // Wire an implication / followed-by from `entryVtxp`: builds the antecedent,
    // emits the match-link (and for followed-by the reject-sink edge), inserts a
    // delay vertex for non-overlapped forms, and builds the body. Used both for
    // nested AstImplication in pexpr position and for the top-level assertion
    // antecedent -- `errorNodep` anchors the "unsupported sequence antecedent"
    // error, which differs between the two call sites.
    Build_ਨਤੀਜਾ build_Implication_ਕਿਨਾਰੇ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* antExprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* bodyExprp,
                                      Sva_State_ਸ਼ੀਰਸ਼* ਇੰਦਰਾਜ_Vtxp, ਤਾਰਕਿਕ isOverlapped,
                                      ਤਾਰਕਿਕ isFollowedBy, Ast_ਗੰਢ* errorNodep, File_ਸਤਰ* flp) {
        ਅਚਲ Build_ਨਤੀਜਾ ant_ਨਤੀਜਾ = build_ਵਿਅੰਜਕ(antExprp, ਇੰਦਰਾਜ_Vtxp);
        ਜੇ (!ant_ਨਤੀਜਾ.valid()) ਵਾਪਸ ant_ਨਤੀਜਾ;

        // Followed-by requires pure-boolean antecedent for non-vacuous-fail at
        // the attempt-start cycle. IEEE 1800-2023 16.12.9 permits a multi-cycle
        // sequence LHS, so this is an implementation gap rather than illegal SV.
        ਜੇ (isFollowedBy && ant_ਨਤੀਜਾ.ਪਦ_Vertexp != ਇੰਦਰਾਜ_Vtxp) {
            errorNodep->v3warn(E_UNSUPPORTED,
                               "Unsupported: sequence expression as antecedent of followed-by"
                               " (#-# / #=#) (IEEE 1800-2023 16.12.9)");
            ਵਾਪਸ Build_ਨਤੀਜਾ::failWithError();
        }
        UASSERT_OBJ(!isFollowedBy || ant_ਨਤੀਜਾ.finalCondp, errorNodep,
                    "followed-by antecedent terminal at entry must carry finalCondp");

        // Use raw createStateVertex() so trigVtxp starts without liveness --
        // reaching the antecedent terminal is a definitive event.
        Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ trigVtxp = m_ਗ੍ਰਾਫ.create_State_ਸ਼ੀਰਸ਼();
        ਜੇ (ant_ਨਤੀਜਾ.finalCondp) {
            m_ਗ੍ਰਾਫ.ਜੋੜੋ_Link(ant_ਨਤੀਜਾ.ਪਦ_Vertexp, trigVtxp,
                            sampled(ant_ਨਤੀਜਾ.finalCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)));
            // Followed-by non-vacuous fail: rejectOnFail fires when the attempt
            // is live (termVtx reachable) and sampled(antecedent) is false.
            ਜੇ (isFollowedBy) {
                Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ sinkVtxp = m_ਗ੍ਰਾਫ.create_State_ਸ਼ੀਰਸ਼();
                sinkVtxp->m_isRejectSink = ਸੱਚ;
                Sva_Trans_ਕਿਨਾਰਾ* ਅਚਲ ep
                    = m_ਗ੍ਰਾਫ.ਜੋੜੋ_Link(ant_ਨਤੀਜਾ.ਪਦ_Vertexp, sinkVtxp,
                                      sampled(ant_ਨਤੀਜਾ.finalCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)));
                ep->m_rejectOnFail = ਸੱਚ;
            }
            // finalCondp is cloned into the Sampled nodes; if the original is
            // not parented anywhere in the AST anymore it must be freed here
            // or ASan flags it as a leak (e.g. t_sequence_bool_ops).
            ਜੇ (!ant_ਨਤੀਜਾ.finalCondp->backp()) {
                VL_DO_DANGLING(ant_ਨਤੀਜਾ.finalCondp->delete_ਰੁੱਖ(), ant_ਨਤੀਜਾ.finalCondp);
            }
        } ਨਹੀਂਤਾਂ {
            m_ਗ੍ਰਾਫ.ਜੋੜੋ_Link(ant_ਨਤੀਜਾ.ਪਦ_Vertexp, trigVtxp);
        }
        resetScope();

        Sva_State_ਸ਼ੀਰਸ਼* bodyEntryp = trigVtxp;
        ਜੇ (!isOverlapped) {
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ delayVtxp = m_ਗ੍ਰਾਫ.create_State_ਸ਼ੀਰਸ਼();
            m_ਗ੍ਰਾਫ.ਜੋੜੋ_Clocked_ਕਿਨਾਰਾ(trigVtxp, delayVtxp);
            bodyEntryp = delayVtxp;
        }
        // An empty antecedent match must not start the consequent, but a zero-minimum
        // repetition's empty match is not handled here; keep the merge vertex for those
        VL_RESTORER(m_want_Bool_ਪੱਤੀ);
        m_want_Bool_ਪੱਤੀ = !isFollowedBy && !hasZeroMinRep(antExprp);
        ਵਾਪਸ build_ਵਿਅੰਜਕ(bodyExprp, bodyEntryp, /*isTopLevelStep=*/ਸੱਚ);
    }

    Build_ਨਤੀਜਾ build(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        m_ਗ੍ਰਾਫ.m_ਸ਼ੁਰੂ_Vertexp = scoped_Create_ਸ਼ੀਰਸ਼();
        VL_RESTORER(m_want_Bool_ਪੱਤੀ);
        m_want_Bool_ਪੱਤੀ = ਸੱਚ;
        ਵਾਪਸ build_ਵਿਅੰਜਕ(exprp, m_ਗ੍ਰਾਫ.m_ਸ਼ੁਰੂ_Vertexp, /*isTopLevelStep=*/ਸੱਚ);
    }
};

//######################################################################
// NFA Lowering (converts NFA graph to synthesizable AstAlways blocks)

ਵਰਗ SvaNfaLowering final {
    Ast_ਗੰਢ_Module* ਅਚਲ m_modp;  // Module to add state vars and always blocks to
    Ast_ਗੰਢ_DType* ਅਚਲ m_u32DTypep;  // Shared unsigned counter dtype
    V3_Unique_ਨਾਂ m_ਨਾਂ{"__Vnfa"};
    ਮਾਪ_t m_stat_Delay_Ring_ਕਿਨਾਰਾ_Visits = 0;  // Delay-ring incoming edges visited

    // Per-lowering shared context (passed to phase sub-functions)
    // Per-vertex lowering state is stored in SvaVertexData and accessed via
    // V3GraphVertex::userp() (see SvaStateVertex::datap()).
    ਬਣਤਰ ਹੇਠਲਾ_Ctx final {
        File_ਸਤਰ* flp;  // Source location for generated AST
        Ast_Sen_ਰੁੱਖ* senTreep;  // Clock sensitivity tree
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* disableExprp;  // disable iff expression (may be nullptr)
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਮੇਲ_Condp;  // Final boolean match condition (may be nullptr)
        VAssertType assertType;  // Assertion type for control tasks
        VAssertDirectiveType directiveType;  // Directive type for control tasks
        AstVar* killVarp;  // Last observed kill generation
        Sva_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ;  // NFA graph
    };

    // Build a match-now expression: stateSig[i] && $sampled(condp)
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* build_ਮੇਲ_Now(File_ਸਤਰ* flp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* stateExprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ statep = stateExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        ਜੇ (!condp) ਵਾਪਸ statep;
        ਵਾਪਸ ਨਵਾਂ AstLogAnd{flp, statep, sampled(condp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ))};
    }
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* andCond(File_ਸਤਰ* flp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp) {
        ਜੇ (!condp) ਵਾਪਸ exprp;
        ਵਾਪਸ ਨਵਾਂ AstLogAnd{flp, exprp, condp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
    }
    // bp is always non-null; only ap can be null (serving as accumulator).
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* orExprs(File_ਸਤਰ* flp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ap, Ast_ਗੰਢ_ਵਿਅੰਜਕ* bp) {
        ਜੇ (!ap) ਵਾਪਸ bp;
        ਵਾਪਸ ਨਵਾਂ AstLogOr{flp, ap, bp};
    }
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਜੋੜੋ_Countp(File_ਸਤਰ* ਅਚਲ flp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਕੁੱਲ_Countp,
                                  Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ contributionp,
                                  Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enablep = ਖਾਲੀ_ਸੂਚਕ) {
        // contribution = enable ? contribution : 0;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enabledContributionp
            = enablep ? ਨਵਾਂ AstCond{flp, enablep, contributionp,
                                    newTypedConstp(flp, contributionp->dtypep(), 0)}
                      : contributionp;
        ਜੇ (!ਕੁੱਲ_Countp) ਵਾਪਸ enabledContributionp;
        ਵਾਪਸ ਨਵਾਂ Ast_ਜੋੜੋ{flp, ਕੁੱਲ_Countp, enabledContributionp};
    }
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* gateCheckedCountp(File_ਸਤਰ* ਅਚਲ flp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ countp,
                                          Ast_ਗੰਢ_ਵਿਅੰਜਕ*& overflowp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enablep) {
        ਜੇ (overflowp) {
            overflowp = ਨਵਾਂ AstLogAnd{flp, enablep->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), overflowp};
        }
        ਵਾਪਸ ਜੋੜੋ_Countp(flp, ਖਾਲੀ_ਸੂਚਕ, countp, enablep);
    }
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਜੋੜੋ_Checked_Countp(File_ਸਤਰ* ਅਚਲ flp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਕੁੱਲ_Countp,
                                         Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ contributionp, Ast_ਗੰਢ_ਵਿਅੰਜਕ*& overflowp,
                                         Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ contributionOverflowp = ਖਾਲੀ_ਸੂਚਕ) {
        ਜੇ (contributionOverflowp) { overflowp = orExprs(flp, overflowp, contributionOverflowp); }
        ਜੇ (!ਕੁੱਲ_Countp) ਵਾਪਸ contributionp;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ oldTotalp = ਕੁੱਲ_Countp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ resultp = ਨਵਾਂ Ast_ਜੋੜੋ{flp, ਕੁੱਲ_Countp, contributionp};
        overflowp
            = orExprs(flp, overflowp, ਨਵਾਂ AstLt{flp, resultp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), oldTotalp});
        ਵਾਪਸ resultp;
    }
    AstIf* new_ਗਿਣਤੀ_ਉਛਾਲ_Warning(File_ਸਤਰ* ਅਚਲ flp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp) ਅਚਲ {
        AstDisplay* ਅਚਲ dispp = ਨਵਾਂ AstDisplay{
            flp, VDisplayType::DT_WARNING,
            "Cover sequence match count overflowed 32-bit storage; results may be incorrect.",
            ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, condp, dispp};
        ifp->branchPred(VBranchPred::BP_UNLIKELY);
        ਵਾਪਸ ifp;
    }
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* kill_ਸਰਗਰਮ(ਹੇਠਲਾ_Ctx& c) {
        ਵਾਪਸ ਨਵਾਂ AstNeq{c.flp, ਨਵਾਂ AstVarRef{c.flp, c.killVarp, VAccess::READ},
                          assertKillGet(c.flp, c.assertType, c.directiveType)};
    }
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* not_Kill_ਸਰਗਰਮ(ਹੇਠਲਾ_Ctx& c) { ਵਾਪਸ ਨਵਾਂ AstLogNot{c.flp, kill_ਸਰਗਰਮ(c)}; }
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* gateNotKill(ਹੇਠਲਾ_Ctx& c, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        ਜੇ (!exprp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਵਾਪਸ ਨਵਾਂ AstLogAnd{c.flp, exprp, not_Kill_ਸਰਗਰਮ(c)};
    }
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਗਲਾ_Ring_ਸੂਚਕਾਂਕ(File_ਸਤਰ* flp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* idxExprp, uint32_t ਮਾਪ) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ u32Const = [flp](uint32_t ਮੁੱਲ) {
            ਵਾਪਸ ਨਵਾਂ AstConst{flp, AstConst::Widthed_ਮੁੱਲ{}, 32, ਮੁੱਲ};
        };
        UASSERT_OBJ(ਮਾਪ > 0, idxExprp, "Ring size must be positive");
        ਜੇ (ਮਾਪ == 1) {
            VL_DO_DANGLING(idxExprp->delete_ਰੁੱਖ(), idxExprp);
            ਵਾਪਸ u32Const(0);
        }
        // idx == size - 1 ? 0 : idx + 1
        Ast_ਜੋੜੋ* ਅਚਲ addp = ਨਵਾਂ Ast_ਜੋੜੋ{flp, idxExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), u32Const(1)};
        addp->dtypeFrom(idxExprp);
        AstCond* ਅਚਲ condp
            = ਨਵਾਂ AstCond{flp, ਨਵਾਂ AstEq{flp, idxExprp, u32Const(ਮਾਪ - 1)}, u32Const(0), addp};
        condp->dtypeFrom(idxExprp);
        ਵਾਪਸ condp;
    }
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* delay_Ring_ਥਾਂ(File_ਸਤਰ* flp, Sva_State_ਸ਼ੀਰਸ਼* vtxp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* idxExprp,
                                      VAccess access = VAccess::READ) {
        // ring[idx]
        AstVar* ਅਚਲ ringp = vtxp->datap()->delayRingVarp;
        AstVarRef* ਅਚਲ refp = ਨਵਾਂ AstVarRef{flp, ringp, access};
        ਜੇ (vtxp->datap()->delayRingCounts) ਵਾਪਸ ਨਵਾਂ AstArraySel{flp, refp, idxExprp};
        ਵਾਪਸ ਨਵਾਂ AstSel{flp, refp, idxExprp, 1};
    }
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* delay_Ring_ਸਿਰ_ਗਿਣਤੀ(File_ਸਤਰ* flp, Sva_State_ਸ਼ੀਰਸ਼* vtxp) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ headp
            = ਨਵਾਂ AstVarRef{flp, vtxp->datap()->delay_Ring_ਸਿਰ_Varp, VAccess::READ};
        ਜੇ (vtxp->datap()->delayRingCounts) ਵਾਪਸ headp;
        ਵਾਪਸ ਨਵਾਂ AstExtend{flp, headp, vtxp->datap()->delay_Ring_ਜੀਵਤ_ਗਿਣਤੀ_Varp->dtypep()->ਚੌੜਾਈ()};
    }
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* delay_Ring_At_ਆਖਰੀ_ਸੂਚਕਾਂਕ(File_ਸਤਰ* ਅਚਲ flp, AstVar* ਅਚਲ idxp,
                                             ਅਚਲ uint32_t ਮਾਪ) {
        ਵਾਪਸ ਨਵਾਂ AstEq{flp, ਨਵਾਂ AstVarRef{flp, idxp, VAccess::READ},
                         ਨਵਾਂ AstConst{flp, AstConst::Widthed_ਮੁੱਲ{}, 32, ਮਾਪ - 1}};
    }
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* delay_Ring_Output_ਗਿਣਤੀ(File_ਸਤਰ* ਅਚਲ flp, Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp) {
        AstVar* ਅਚਲ idxp = vtxp->datap()->delayRingIdxVarp;
        ਅਚਲ uint32_t ਮਾਪ = vtxp->m_delay_Ring_ਮਾਪ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* outgoingValidp
            = ਨਵਾਂ AstVarRef{flp, vtxp->datap()->delayRingWrappedVarp, VAccess::READ};
        ਜੇ (!vtxp->m_isFixedDelayRing) {
            outgoingValidp
                = ਨਵਾਂ AstLogOr{flp, outgoingValidp, delay_Ring_At_ਆਖਰੀ_ਸੂਚਕਾਂਕ(flp, idxp, ਮਾਪ)};
        }
        AstVar* ਅਚਲ ਜੀਵਤ_ਗਿਣਤੀ_Varp = vtxp->datap()->delay_Ring_ਜੀਵਤ_ਗਿਣਤੀ_Varp;
        ਵਾਪਸ ਨਵਾਂ AstCond{flp, outgoingValidp, delay_Ring_ਸਿਰ_ਗਿਣਤੀ(flp, vtxp),
                           newTypedConstp(flp, ਜੀਵਤ_ਗਿਣਤੀ_Varp->dtypep(), 0)};
    }
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* delay_Ring_Has_ਜੀਵਤ_Threadsp(File_ਸਤਰ* ਅਚਲ flp,
                                                 AstVar* ਅਚਲ ਜੀਵਤ_ਗਿਣਤੀ_Varp) {
        // active = live_count != 0;
        ਵਾਪਸ ਨਵਾਂ AstNeq{flp, ਨਵਾਂ AstVarRef{flp, ਜੀਵਤ_ਗਿਣਤੀ_Varp, VAccess::READ},
                          newTypedConstp(flp, ਜੀਵਤ_ਗਿਣਤੀ_Varp->dtypep(), 0)};
    }

    // Phase 3 output signals
    ਬਣਤਰ Signal_ਸੈੱਟ final {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* terminalActivep = ਖਾਲੀ_ਸੂਚਕ;  // OR of all successful terminal matches
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਮੇਲ_Countp = ਖਾਲੀ_ਸੂਚਕ;  // NFA paths completing the sequence this tick
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਮੇਲ_ਗਿਣਤੀ_Overflowp = ਖਾਲੀ_ਸੂਚਕ;  // Match count overflow this tick
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* rejectBasep = ਖਾਲੀ_ਸੂਚਕ;  // Reject when a terminal match fails
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* required_ਕਦਮ_Rejectp = ਖਾਲੀ_ਸੂਚਕ;  // Per-source reject from rejectOnFail Links
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* throughoutRejectp = ਖਾਲੀ_ਸੂਚਕ;  // Reject when a throughout guard drops
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* threadFailCountp = ਖਾਲੀ_ਸੂਚਕ;  // Number of threads rejected on this tick
    };

    // Disable condition also aborts attempts between clock events. A constant disable
    // never rises, and $sampled is unsupported in a sensitivity list.
    ਸਥਿਰ ਤਾਰਕਿਕ isAsyncDisable(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ disableExprp) {
        ਵਾਪਸ disableExprp && !VN_IS(disableExprp, Const)
               && !disableExprp->exists([](ਅਚਲ AstSampled*) { ਵਾਪਸ ਸੱਚ; });
    }

    // Emit the clocked update block of registers that hold pending attempts. The clearp
    // statements reset the same registers: a disable rising between clock events also
    // aborts pending attempts (IEEE 1800-2023 16.12), so clearp runs on that rising edge
    // in a separate block. It must clear unconditionally, as the disable may already be
    // false again when it runs, and the update block must not run on that edge, as it
    // would advance the attempts as if the clock had ticked. Blocks that hold no attempt
    // state, such as the $assertkill acknowledge in emitKillAckNba, do not go through here.
    // Callers pass a null clearp when the disable cannot rise between clock events.
    ਖਾਲੀ ਜੋੜੋ_State_Always(ਹੇਠਲਾ_Ctx& c, Ast_ਗੰਢ* bodyp, Ast_ਗੰਢ* clearp) {
        m_modp->ਜੋੜੋ_Stmtsp(
            ਨਵਾਂ AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_ਰੁੱਖ(ਝੂਠ), bodyp});
        ਜੇ (!clearp) ਵਾਪਸ;
        m_modp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstAlways{
            c.flp, VAlwaysKwd::ALWAYS,
            ਨਵਾਂ Ast_Sen_ਰੁੱਖ{c.flp, ਨਵਾਂ Ast_Sen_ਵਸਤੂ{c.flp, VEdgeType::ET_POSEDGE,
                                                 c.disableExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)}},
            clearp});
    }

    // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
    // Phase 2: State register NBA always block. Each clocked-edge target
    // latches the OR of its incoming contributions.
    ਖਾਲੀ emit_State_ਰਜਿਸਟਰ_Nba(ਹੇਠਲਾ_Ctx& c) {
        ਅਚਲ ਤਾਰਕਿਕ asyncDisable = isAsyncDisable(c.disableExprp);
        Ast_ਗੰਢ* bodyp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ* clearp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtxr : c.ਗ੍ਰਾਫ.m_ਗ੍ਰਾਫ.vertices()) {
            ਅਚਲ Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = static_cast<ਅਚਲ Sva_State_ਸ਼ੀਰਸ਼*>(&vtxr);
            ਜੇ (!vtxp->datap()->stateVarp) ਜਾਰੀ;

            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਗਲਾ_Statep = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& edger : vtxp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
                ਅਚਲ Sva_Trans_ਕਿਨਾਰਾ& tedger = static_cast<ਅਚਲ Sva_Trans_ਕਿਨਾਰਾ&>(edger);
                ਜੇ (!tedger.m_consumes_ਚੱਕਰ) ਜਾਰੀ;
                ਅਚਲ Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Clocked-edge source missing stateSig");

                Ast_ਗੰਢ_ਵਿਅੰਜਕ* srcSigp = fromVtxp->datap()->stateSigp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                srcSigp = andCond(c.flp, srcSigp, tedger.m_condp);

                ਜੇ (c.disableExprp) {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ notDisp
                        = ਨਵਾਂ AstLogNot{c.flp, c.disableExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                    srcSigp = ਨਵਾਂ AstLogAnd{c.flp, srcSigp, notDisp};
                }
                ਅਗਲਾ_Statep = orExprs(c.flp, ਅਗਲਾ_Statep, srcSigp);
            }

            UASSERT_OBJ(ਅਗਲਾ_Statep, vtxp,
                        "Registered vertex has no clocked incoming contribution");
            ਅਗਲਾ_Statep = gateNotKill(c, ਅਗਲਾ_Statep);

            Ast_ਸੌਂਪੋ_Dly* ਅਚਲ assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{
                c.flp, ਨਵਾਂ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE}, ਅਗਲਾ_Statep};
            bodyp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(bodyp, assignp);
            ਜੇ (asyncDisable) {
                clearp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(
                    clearp,
                    ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{
                        c.flp, ਨਵਾਂ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE},
                        ਨਵਾਂ AstConst{c.flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}});
            }
        }
        ਜੇ (bodyp) ਜੋੜੋ_State_Always(c, bodyp, clearp);
    }

    // Phase 2b: In-place ring storage and registered ring metadata.
    ਖਾਲੀ emitDelayRingUpdates(ਹੇਠਲਾ_Ctx& c, ਅਚਲ ਤਾਰਕਿਕ ਗਿਣਤੀ_Matches) {
        ਅਚਲ ਤਾਰਕਿਕ asyncDisable = isAsyncDisable(c.disableExprp);
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtxr : c.ਗ੍ਰਾਫ.m_ਗ੍ਰਾਫ.vertices()) {
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = static_cast<Sva_State_ਸ਼ੀਰਸ਼*>(&vtxr);
            ਜੇ (!vtxp->datap()->delayRingVarp) ਜਾਰੀ;
            AstVar* ਅਚਲ idxp = vtxp->datap()->delayRingIdxVarp;
            AstVar* ਅਚਲ ਜੀਵਤ_ਗਿਣਤੀ_Varp = vtxp->datap()->delay_Ring_ਜੀਵਤ_ਗਿਣਤੀ_Varp;
            AstVar* ਅਚਲ wrappedp = vtxp->datap()->delayRingWrappedVarp;
            ਅਚਲ uint32_t ਮਾਪ = static_cast<uint32_t>(vtxp->m_delay_Ring_ਮਾਪ);

            Ast_ਗੰਢ_ਵਿਅੰਜਕ* incomingp = ਖਾਲੀ_ਸੂਚਕ;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* incomingOverflowp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& edger : vtxp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
                ++m_stat_Delay_Ring_ਕਿਨਾਰਾ_Visits;
                ਅਚਲ Sva_Trans_ਕਿਨਾਰਾ& tedger = static_cast<ਅਚਲ Sva_Trans_ਕਿਨਾਰਾ&>(edger);
                UASSERT_OBJ(tedger.m_consumes_ਚੱਕਰ == vtxp->m_isFixedDelayRing, vtxp,
                            "Delay-ring incoming edge kind mismatch");
                ਅਚਲ Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Delay-ring incoming source missing stateSig");
                UASSERT_OBJ(!ਗਿਣਤੀ_Matches || fromVtxp->datap()->state_ਗਿਣਤੀ_Sigp, fromVtxp,
                            "Delay-ring incoming source missing stateCountSig");
                // IEEE 1800-2023 16.14.1/16.14.3: assertion alternatives belong to one
                // evaluation; only cover sequence counts each end-of-match.
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* contribp
                    = ਗਿਣਤੀ_Matches
                          ? fromVtxp->datap()->state_ਗਿਣਤੀ_Sigp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)
                          : ਨਵਾਂ AstExtend{c.flp,
                                          fromVtxp->datap()->stateSigp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                                          m_u32DTypep->ਚੌੜਾਈ()};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* contribOverflowp
                    = ਗਿਣਤੀ_Matches && fromVtxp->datap()->state_ਗਿਣਤੀ_ਉਛਾਲ_Sigp
                          ? fromVtxp->datap()->state_ਗਿਣਤੀ_ਉਛਾਲ_Sigp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)
                          : ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (tedger.m_condp) {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enablep = tedger.m_condp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                    contribp = ਗਿਣਤੀ_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, enablep)
                                   : ਜੋੜੋ_Countp(c.flp, ਖਾਲੀ_ਸੂਚਕ, contribp, enablep);
                }
                ਜੇ (c.disableExprp) {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ notDisp
                        = ਨਵਾਂ AstLogNot{c.flp, c.disableExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                    contribp = ਗਿਣਤੀ_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, notDisp)
                                   : ਜੋੜੋ_Countp(c.flp, ਖਾਲੀ_ਸੂਚਕ, contribp, notDisp);
                }
                incomingp = ਗਿਣਤੀ_Matches ? ਜੋੜੋ_Checked_Countp(c.flp, incomingp, contribp,
                                                            incomingOverflowp, contribOverflowp)
                                         : ਜੋੜੋ_Countp(c.flp, incomingp, contribp);
            }
            UASSERT_OBJ(incomingp, vtxp, "Delay ring has no incoming edge");
            ਅਚਲ ਤਾਰਕਿਕ storesCounts = vtxp->datap()->delayRingCounts;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ incomingCountp
                = storesCounts
                      ? incomingp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)
                      : ਨਵਾਂ AstExtend{c.flp,
                                      ਨਵਾਂ AstNeq{c.flp, incomingp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                                                 newTypedConstp(c.flp, incomingp->dtypep(), 0)},
                                      m_u32DTypep->ਚੌੜਾਈ()};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ storedIncomingp
                = storesCounts ? incomingp
                               : ਨਵਾਂ AstNeq{c.flp, incomingp,
                                            newTypedConstp(c.flp, incomingp->dtypep(), 0)};
            // Only this block accesses the storage. Other processes read the cached head, so
            // the ring can be updated in place without an NBA shadow copy.
            Ast_ਸੌਂਪੋ* ਅਚਲ writeIncomingp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                c.flp,
                delay_Ring_ਥਾਂ(c.flp, vtxp, ਨਵਾਂ AstVarRef{c.flp, idxp, VAccess::READ},
                              VAccess::WRITE),
                storedIncomingp};
            Ast_ਗੰਢ* updateBodyp = writeIncomingp;
            // live_count <= live_count - outgoing_count + incoming_count;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ outgoingp = delay_Ring_Output_ਗਿਣਤੀ(c.flp, vtxp);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ afterOutgoingp
                = ਨਵਾਂ AstSub{c.flp, ਨਵਾਂ AstVarRef{c.flp, ਜੀਵਤ_ਗਿਣਤੀ_Varp, VAccess::READ}, outgoingp};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ oldAfterOutgoingp = afterOutgoingp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            Ast_ਜੋੜੋ* ਅਚਲ ਅਗਲਾ_ਜੀਵਤ_Countp = ਨਵਾਂ Ast_ਜੋੜੋ{c.flp, afterOutgoingp, incomingCountp};
            ਜੇ (storesCounts) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਜੀਵਤ_Overflowp
                    = ਨਵਾਂ AstLt{c.flp, ਅਗਲਾ_ਜੀਵਤ_Countp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), oldAfterOutgoingp};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ overflowp = orExprs(c.flp, incomingOverflowp, ਜੀਵਤ_Overflowp);
                updateBodyp = new_ਗਿਣਤੀ_ਉਛਾਲ_Warning(c.flp, overflowp);
                updateBodyp->ਜੋੜੋ_ਅਗਲਾ(writeIncomingp);
            } ਨਹੀਂਤਾਂ {
                VL_DO_DANGLING(oldAfterOutgoingp->delete_ਰੁੱਖ(), oldAfterOutgoingp);
            }
            // Cache the outgoing slot for the next index after the write. Reading after
            // writing also handles one-slot rings and two-slot range rings without alias cases.
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਗਲਾ_ਸਿਰ_Idxp
                = ਅਗਲਾ_Ring_ਸੂਚਕਾਂਕ(c.flp, ਨਵਾਂ AstVarRef{c.flp, idxp, VAccess::READ}, ਮਾਪ);
            ਜੇ (!vtxp->m_isFixedDelayRing) {
                ਅਗਲਾ_ਸਿਰ_Idxp = ਅਗਲਾ_Ring_ਸੂਚਕਾਂਕ(c.flp, ਅਗਲਾ_ਸਿਰ_Idxp, ਮਾਪ);
            }
            updateBodyp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{
                c.flp, ਨਵਾਂ AstVarRef{c.flp, vtxp->datap()->delay_Ring_ਸਿਰ_Varp, VAccess::WRITE},
                delay_Ring_ਥਾਂ(c.flp, vtxp, ਅਗਲਾ_ਸਿਰ_Idxp)});
            updateBodyp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{
                c.flp, ਨਵਾਂ AstVarRef{c.flp, ਜੀਵਤ_ਗਿਣਤੀ_Varp, VAccess::WRITE}, ਅਗਲਾ_ਜੀਵਤ_Countp});
            // wrapped <= wrapped || idx == size - 1;
            updateBodyp->ਜੋੜੋ_ਅਗਲਾ(
                ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{c.flp, ਨਵਾਂ AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                 ਨਵਾਂ AstLogOr{c.flp, ਨਵਾਂ AstVarRef{c.flp, wrappedp, VAccess::READ},
                                              delay_Ring_At_ਆਖਰੀ_ਸੂਚਕਾਂਕ(c.flp, idxp, ਮਾਪ)}});
            // idx <= next_idx;
            updateBodyp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{
                c.flp, ਨਵਾਂ AstVarRef{c.flp, idxp, VAccess::WRITE},
                ਅਗਲਾ_Ring_ਸੂਚਕਾਂਕ(c.flp, ਨਵਾਂ AstVarRef{c.flp, idxp, VAccess::READ}, ਮਾਪ)});
            ਜੇ (vtxp->m_delayRingAdvanceCondp) {
                updateBodyp = ਨਵਾਂ AstIf{
                    c.flp, sampled(vtxp->m_delayRingAdvanceCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)),
                    updateBodyp};
            }

            Ast_ਗੰਢ_ਵਿਅੰਜਕ* clearCondp = kill_ਸਰਗਰਮ(c);
            ਜੇ (vtxp->m_delayRingClearCondp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_delayRingClearCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)));
            }
            ਜੇ (vtxp->m_abortClearp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_abortClearp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)));
            }
            ਜੇ (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
            }
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* guardp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ cp : vtxp->m_throughoutConds) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ sampledp = sampled(cp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
                guardp = guardp ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstLogAnd{c.flp, guardp, sampledp})
                                : sampledp;
            }
            ਜੇ (guardp) clearCondp = orExprs(c.flp, clearCondp, ਨਵਾਂ AstLogNot{c.flp, guardp});
            // Logically clear the ring without touching its wide storage.
            Ast_ਸੌਂਪੋ_Dly* ਅਚਲ clearCountp
                = ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{c.flp, ਨਵਾਂ AstVarRef{c.flp, ਜੀਵਤ_ਗਿਣਤੀ_Varp, VAccess::WRITE},
                                   newTypedConstp(c.flp, ਜੀਵਤ_ਗਿਣਤੀ_Varp->dtypep(), 0)};
            clearCountp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{c.flp,
                                                  ਨਵਾਂ AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                                  ਨਵਾਂ AstConst{c.flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}});
            clearCountp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{c.flp,
                                                  ਨਵਾਂ AstVarRef{c.flp, idxp, VAccess::WRITE},
                                                  newTypedConstp(c.flp, idxp->dtypep(), 0)});
            updateBodyp = ਨਵਾਂ AstIf{c.flp, clearCondp, clearCountp, updateBodyp};

            ਜੋੜੋ_State_Always(c, updateBodyp, asyncDisable ? clearCountp->clone_ਰੁੱਖ(ਸੱਚ) : ਖਾਲੀ_ਸੂਚਕ);
        }
    }

    // Phase 2c: SAnd combiner done-latch always block.
    // NBA semantics ensure doneL/doneR read pre-update values (IEEE 16.9.5).
    ਖਾਲੀ emit_And_Combiner_ਪੂਰਾ_Latch_Nba(ਹੇਠਲਾ_Ctx& c) {
        ਅਚਲ ਤਾਰਕਿਕ asyncDisable = isAsyncDisable(c.disableExprp);
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtxr : c.ਗ੍ਰਾਫ.m_ਗ੍ਰਾਫ.vertices()) {
            ਅਚਲ Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ avp = static_cast<ਅਚਲ Sva_State_ਸ਼ੀਰਸ਼*>(&vtxr);
            ਜੇ (!avp->datap()->ਪੂਰਾ_LVarp) ਜਾਰੀ;
            // doneLVars is non-null only for AndCombiner vertices, which always
            // have both m_andLhsTermp and m_andRhsTermp set at build time.
            UASSERT_OBJ(avp->m_andLhsTermp && avp->m_andRhsTermp, avp,
                        "AndCombiner vertex missing LHS/RHS terminal");
            ਅਚਲ Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ lhsTermp = avp->m_andLhsTermp;
            ਅਚਲ Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ rhsTermp = avp->m_andRhsTermp;
            // resolveLinks' 2*N+2 fixed-point pass is guaranteed to populate
            // stateSigp on every AndCombiner and its LHS/RHS terminals.
            UASSERT_OBJ(lhsTermp->datap()->stateSigp && rhsTermp->datap()->stateSigp
                            && avp->datap()->stateSigp,
                        avp, "AndCombiner stateSigp chain unresolved after resolveLinks");

            Ast_ਸੌਂਪੋ_Dly* ਅਚਲ clearLp = ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{
                c.flp, ਨਵਾਂ AstVarRef{c.flp, avp->datap()->ਪੂਰਾ_LVarp, VAccess::WRITE},
                ਨਵਾਂ AstConst{c.flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}};
            Ast_ਸੌਂਪੋ_Dly* ਅਚਲ clearRp = ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{
                c.flp, ਨਵਾਂ AstVarRef{c.flp, avp->datap()->ਪੂਰਾ_RVarp, VAccess::WRITE},
                ਨਵਾਂ AstConst{c.flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}};
            clearLp->ਜੋੜੋ_ਅਗਲਾ(clearRp);

            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਮੇਲ_LNowp
                = build_ਮੇਲ_Now(c.flp, lhsTermp->datap()->stateSigp, avp->m_andLhsCondp);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਮੇਲ_RNowp
                = build_ਮੇਲ_Now(c.flp, rhsTermp->datap()->stateSigp, avp->m_andRhsCondp);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* gateLp = ਮੇਲ_LNowp;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* gateRp = ਮੇਲ_RNowp;
            ਜੇ (c.disableExprp) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ notDisLp
                    = ਨਵਾਂ AstLogNot{c.flp, c.disableExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                gateLp = ਨਵਾਂ AstLogAnd{c.flp, gateLp, notDisLp};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ notDisRp
                    = ਨਵਾਂ AstLogNot{c.flp, c.disableExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                gateRp = ਨਵਾਂ AstLogAnd{c.flp, gateRp, notDisRp};
            }
            Ast_ਸੌਂਪੋ_Dly* ਅਚਲ ਸੈੱਟ_Lp = ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{
                c.flp, ਨਵਾਂ AstVarRef{c.flp, avp->datap()->ਪੂਰਾ_LVarp, VAccess::WRITE},
                ਨਵਾਂ AstConst{c.flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}};
            AstIf* ਅਚਲ ਸੈੱਟ_LIfp = ਨਵਾਂ AstIf{c.flp, gateLp, ਸੈੱਟ_Lp, ਖਾਲੀ_ਸੂਚਕ};
            Ast_ਸੌਂਪੋ_Dly* ਅਚਲ ਸੈੱਟ_Rp = ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{
                c.flp, ਨਵਾਂ AstVarRef{c.flp, avp->datap()->ਪੂਰਾ_RVarp, VAccess::WRITE},
                ਨਵਾਂ AstConst{c.flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}};
            AstIf* ਅਚਲ ਸੈੱਟ_RIfp = ਨਵਾਂ AstIf{c.flp, gateRp, ਸੈੱਟ_Rp, ਖਾਲੀ_ਸੂਚਕ};
            ਸੈੱਟ_LIfp->ਜੋੜੋ_ਅਗਲਾ(ਸੈੱਟ_RIfp);

            Ast_ਗੰਢ_ਵਿਅੰਜਕ* clearCondp = ਨਵਾਂ AstLogOr{c.flp, kill_ਸਰਗਰਮ(c),
                                                   avp->datap()->stateSigp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
            ਜੇ (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
            }
            AstIf* ਅਚਲ topp = ਨਵਾਂ AstIf{c.flp, clearCondp, clearLp, ਸੈੱਟ_LIfp};
            ਜੋੜੋ_State_Always(c, topp, asyncDisable ? clearLp->clone_ਰੁੱਖ(ਸੱਚ) : ਖਾਲੀ_ਸੂਚਕ);
        }
    }

    ਖਾਲੀ emitKillAckNba(ਹੇਠਲਾ_Ctx& c) {
        Ast_ਸੌਂਪੋ_Dly* ਅਚਲ ackp
            = ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{c.flp, ਨਵਾਂ AstVarRef{c.flp, c.killVarp, VAccess::WRITE},
                               assertKillGet(c.flp, c.assertType, c.directiveType)};
        m_modp->ਜੋੜੋ_Stmtsp(
            ਨਵਾਂ AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_ਰੁੱਖ(ਝੂਠ), ackp});
    }

    // Phase 3/3a/3b: Compute terminal match/reject signals, required-step reject,
    // throughout-drop reject; clean up intermediate state signals.
    // Phase 3: terminalActive and rejectBase from Links to matchVertex.
    // Builder only adds Links (non-clocked) to matchVertex via addLink in
    // wireMatchAndMidSources. For cover sequence, also count each end-of-match
    // so the action can be replayed without unrolling endpoints.
    ਖਾਲੀ compute_Terminal_ਮੇਲ_And_Reject(ਹੇਠਲਾ_Ctx& c, Signal_ਸੈੱਟ& sigs, ਅਚਲ ਤਾਰਕਿਕ need_ਮੇਲ_ਗਿਣਤੀ) {
        ਲਈ (ਅਚਲ Sva_Trans_ਕਿਨਾਰਾ* ਅਚਲ tedgep : c.ਗ੍ਰਾਫ.all_ਕਿਨਾਰੇ()) {
            ਜੇ (tedgep->ਮੰਜ਼ਿਲ_Vtxp() != c.ਗ੍ਰਾਫ.m_ਮੇਲ_Vertexp) ਜਾਰੀ;
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                        "Terminal-link source missing stateSig");

            Ast_ਗੰਢ_ਵਿਅੰਜਕ* srcSigp = fromVtxp->datap()->stateSigp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            srcSigp = andCond(c.flp, srcSigp, tedgep->m_condp);
            ਜੇ (need_ਮੇਲ_ਗਿਣਤੀ) {
                UASSERT_OBJ(fromVtxp->datap()->state_ਗਿਣਤੀ_Sigp, fromVtxp,
                            "Terminal-link source missing stateCountSig");
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* contributionp
                    = fromVtxp->datap()->state_ਗਿਣਤੀ_Sigp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* contributionOverflowp
                    = fromVtxp->datap()->state_ਗਿਣਤੀ_ਉਛਾਲ_Sigp
                          ? fromVtxp->datap()->state_ਗਿਣਤੀ_ਉਛਾਲ_Sigp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)
                          : ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (tedgep->m_condp) {
                    contributionp = gateCheckedCountp(c.flp, contributionp, contributionOverflowp,
                                                      tedgep->m_condp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
                }
                sigs.ਮੇਲ_Countp
                    = ਜੋੜੋ_Checked_Countp(c.flp, sigs.ਮੇਲ_Countp, contributionp,
                                       sigs.ਮੇਲ_ਗਿਣਤੀ_Overflowp, contributionOverflowp);
            }

            ਜੇ (fromVtxp->m_delay_Ring_ਮਾਪ && !fromVtxp->m_isFixedDelayRing) {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
                // reject |= (wrapped || idx == size - 1) && head && final_condition;
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* expireContribp = delay_Ring_Output_ਗਿਣਤੀ(c.flp, fromVtxp);
                expireContribp = andCond(c.flp, expireContribp, tedgep->m_condp);
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, expireContribp);
                VL_DO_DANGLING(srcSigp->delete_ਰੁੱਖ(), srcSigp);
            } ਨਹੀਂਤਾਂ ਜੇ (fromVtxp->m_isUnbounded || fromVtxp->m_isAndCombiner) {
                sigs.terminalActivep = orExprs(c.flp, sigs.terminalActivep, srcSigp);
            } ਨਹੀਂਤਾਂ {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, srcSigp);
            }
        }
        // wireMatchAndMidSources always adds a Link from result.termVertexp
        // to m_matchVertexp, so the loop above always sets terminalActivep.
        UASSERT_OBJ(sigs.terminalActivep, c.ਗ੍ਰਾਫ.m_ਮੇਲ_Vertexp,
                    "No terminal edge to match vertex");
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newThroughoutThreadFailCountp(ਹੇਠਲਾ_Ctx& c, AstVar* ਅਚਲ delay_Ring_ਜੀਵਤ_ਗਿਣਤੀ_Varp,
                                               Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ stateExprp,
                                               Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enablep) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਸਰਗਰਮ_Thread_Countp
            = delay_Ring_ਜੀਵਤ_ਗਿਣਤੀ_Varp
                  ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(
                        ਨਵਾਂ AstVarRef{c.flp, delay_Ring_ਜੀਵਤ_ਗਿਣਤੀ_Varp, VAccess::READ})
                  : ਨਵਾਂ AstExtend{c.flp, stateExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), m_u32DTypep->ਚੌੜਾਈ()};
        ਵਾਪਸ ਜੋੜੋ_Countp(c.flp, ਖਾਲੀ_ਸੂਚਕ, ਸਰਗਰਮ_Thread_Countp, enablep);
    }

    // Phase 3b: Throughout-drop and ring-wide abort rejection.
    ਖਾਲੀ computeThroughoutReject(ਹੇਠਲਾ_Ctx& c, Signal_ਸੈੱਟ& sigs, ਅਚਲ ਤਾਰਕਿਕ need_Thread_Fail_ਗਿਣਤੀ) {
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtxr : c.ਗ੍ਰਾਫ.m_ਗ੍ਰਾਫ.vertices()) {
            ਅਚਲ Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = static_cast<ਅਚਲ Sva_State_ਸ਼ੀਰਸ਼*>(&vtxr);
            ਅਚਲ ਸਵੈਚਾਲਿਤ& conds = vtxp->m_throughoutConds;
            ਜੇ (conds.ਖਾਲੀ_ਥਾਵਾਂ() && !vtxp->m_abortRejectp) ਜਾਰੀ;
            ਜੇ (vtxp->m_isAndCombiner) ਜਾਰੀ;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* stateExprp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (vtxp->datap()->stateVarp) {
                stateExprp = ਨਵਾਂ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
            } ਨਹੀਂਤਾਂ ਜੇ (vtxp->datap()->delayRingVarp && vtxp->m_isFixedDelayRing) {
                stateExprp
                    = delay_Ring_Has_ਜੀਵਤ_Threadsp(c.flp, vtxp->datap()->delay_Ring_ਜੀਵਤ_ਗਿਣਤੀ_Varp);
            } ਨਹੀਂਤਾਂ {
                UASSERT_OBJ(vtxp->datap()->stateSigp, vtxp,
                            "Throughout-conds vertex missing state representation");
                stateExprp = vtxp->datap()->stateSigp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            }
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* guardp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ cp : conds) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ sp = sampled(cp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
                guardp = guardp ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstLogAnd{c.flp, guardp, sp}) : sp;
            }
            ਜੇ (vtxp->m_abortRejectp) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ notAbortp
                    = ਨਵਾਂ AstLogNot{c.flp, sampled(vtxp->m_abortRejectp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ))};
                guardp = guardp
                             ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstLogAnd{c.flp, guardp, notAbortp})
                             : notAbortp;
            }
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* rejectCondp = ਨਵਾਂ AstLogNot{c.flp, guardp};
            ਜੇ (vtxp->m_abortClearp) {
                // Any abort clears the ring. Accept aborts suppress a simultaneous guard failure;
                // reject aborts are restored below so they still force rejection.
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ notAbortClearp
                    = ਨਵਾਂ AstLogNot{c.flp, sampled(vtxp->m_abortClearp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ))};
                rejectCondp = ਨਵਾਂ AstLogAnd{c.flp, rejectCondp, notAbortClearp};
                ਜੇ (vtxp->m_abortRejectp) {
                    rejectCondp = ਨਵਾਂ AstLogOr{
                        c.flp, rejectCondp, sampled(vtxp->m_abortRejectp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ))};
                }
            }
            ਜੇ (need_Thread_Fail_ਗਿਣਤੀ) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ contributionp
                    = newThroughoutThreadFailCountp(c, vtxp->datap()->delay_Ring_ਜੀਵਤ_ਗਿਣਤੀ_Varp,
                                                    stateExprp, rejectCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
                sigs.threadFailCountp = ਜੋੜੋ_Countp(c.flp, sigs.threadFailCountp, contributionp);
                ਜੇ (vtxp->m_abortRejectp && vtxp->m_delayRingAdvanceCondp) {
                    // An advancing ring thread also occupies its same-tick match vertex in the
                    // unrolled NFA, so abort rejection must replay both fail actions.
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ abortAndAdvancep = ਨਵਾਂ AstLogAnd{
                        c.flp, sampled(vtxp->m_abortRejectp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)),
                        sampled(vtxp->m_delayRingAdvanceCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ))};
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਮੇਲ_Contributionp = newThroughoutThreadFailCountp(
                        c, vtxp->datap()->delay_Ring_ਜੀਵਤ_ਗਿਣਤੀ_Varp, stateExprp, abortAndAdvancep);
                    sigs.threadFailCountp
                        = ਜੋੜੋ_Countp(c.flp, sigs.threadFailCountp, ਮੇਲ_Contributionp);
                }
            }
            sigs.throughoutRejectp = orExprs(c.flp, sigs.throughoutRejectp,
                                             ਨਵਾਂ AstLogAnd{c.flp, stateExprp, rejectCondp});
        }
    }

    Signal_ਸੈੱਟ compute_ਸੰਕੇਤ(ਹੇਠਲਾ_Ctx& c, ਅਚਲ ਤਾਰਕਿਕ need_Thread_Fail_ਗਿਣਤੀ,
                             ਅਚਲ ਤਾਰਕਿਕ need_Throughout_Thread_Fail_ਗਿਣਤੀ, ਅਚਲ ਤਾਰਕਿਕ need_ਮੇਲ_ਗਿਣਤੀ) {
        Signal_ਸੈੱਟ sigs;

        compute_Terminal_ਮੇਲ_And_Reject(c, sigs, need_ਮੇਲ_ਗਿਣਤੀ);

        // Phase 3a: required-step rejection.
        // Builder only sets m_rejectOnFail on non-clocked Links with m_condp
        // or m_condVtxp, and the source always has a resolved stateSig.
        ਲਈ (ਅਚਲ Sva_Trans_ਕਿਨਾਰਾ* ਅਚਲ tedgep : c.ਗ੍ਰਾਫ.all_ਕਿਨਾਰੇ()) {
            ਜੇ (!tedgep->m_rejectOnFail) ਜਾਰੀ;
            ਅਚਲ Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp && (tedgep->m_condp || tedgep->m_condVtxp),
                        fromVtxp,
                        "rejectOnFail Link must have condp/condVtxp and source stateSig");
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ srcSigp = fromVtxp->datap()->stateSigp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (tedgep->m_condVtxp) {
                UASSERT_OBJ(tedgep->m_condVtxp->datap()->stateSigp, tedgep->m_condVtxp,
                            "rejectOnFail condVtxp missing stateSig");
                condp = tedgep->m_condVtxp->datap()->stateSigp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                ਜੇ (tedgep->m_condp) {
                    condp = ਨਵਾਂ AstLogOr{c.flp, condp, tedgep->m_condp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                }
            } ਨਹੀਂਤਾਂ {
                condp = tedgep->m_condp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            }
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ notCondp = ਨਵਾਂ AstLogNot{c.flp, condp};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rawFailp = ਨਵਾਂ AstLogAnd{c.flp, srcSigp, notCondp};
            ਜੇ (need_Thread_Fail_ਗਿਣਤੀ) {
                // thread_fail_count += fail;
                sigs.threadFailCountp = ਜੋੜੋ_Countp(
                    c.flp, sigs.threadFailCountp,
                    ਨਵਾਂ AstExtend{c.flp, rawFailp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), m_u32DTypep->ਚੌੜਾਈ()});
            }
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ failp = gateNotKill(c, rawFailp);
            sigs.required_ਕਦਮ_Rejectp = orExprs(c.flp, sigs.required_ਕਦਮ_Rejectp, failp);
        }

        computeThroughoutReject(c, sigs, need_Throughout_Thread_Fail_ਗਿਣਤੀ);
        ਜੇ (sigs.threadFailCountp) {
            sigs.threadFailCountp
                = ਜੋੜੋ_Countp(c.flp, ਖਾਲੀ_ਸੂਚਕ, sigs.threadFailCountp, not_Kill_ਸਰਗਰਮ(c));
        }
        ਜੇ (sigs.ਮੇਲ_Countp) {
            ਜੇ (c.ਮੇਲ_Condp) {
                sigs.ਮੇਲ_Countp
                    = gateCheckedCountp(c.flp, sigs.ਮੇਲ_Countp, sigs.ਮੇਲ_ਗਿਣਤੀ_Overflowp,
                                        sampled(c.ਮੇਲ_Condp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)));
            }
            sigs.ਮੇਲ_Countp = gateCheckedCountp(c.flp, sigs.ਮੇਲ_Countp, sigs.ਮੇਲ_ਗਿਣਤੀ_Overflowp,
                                                 not_Kill_ਸਰਗਰਮ(c));
        }
        sigs.terminalActivep = gateNotKill(c, sigs.terminalActivep);
        sigs.rejectBasep = gateNotKill(c, sigs.rejectBasep);
        sigs.throughoutRejectp = gateNotKill(c, sigs.throughoutRejectp);

        // Clean up intermediate state signals. These are orphan subtrees
        // (never linked into the enclosing AST); deleteTree() is immediate
        // which is what we want since stateSigp lifetime ends with this scope.
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtxr : c.ਗ੍ਰਾਫ.m_ਗ੍ਰਾਫ.vertices()) {
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = static_cast<Sva_State_ਸ਼ੀਰਸ਼*>(&vtxr);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ*& sigp = vtxp->datap()->stateSigp;
            ਜੇ (sigp) VL_DO_DANGLING(sigp->delete_ਰੁੱਖ(), sigp);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ*& countp = vtxp->datap()->state_ਗਿਣਤੀ_Sigp;
            ਜੇ (countp) VL_DO_DANGLING(countp->delete_ਰੁੱਖ(), countp);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ*& overflowp = vtxp->datap()->state_ਗਿਣਤੀ_ਉਛਾਲ_Sigp;
            ਜੇ (overflowp) VL_DO_DANGLING(overflowp->delete_ਰੁੱਖ(), overflowp);
        }
        // Disable iff gating (IEEE 1800-2023 16.12) on the current level value.
        ਜੇ (c.disableExprp) {
            // terminalActivep is always set, so gate it unconditionally.
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ notTermp
                = ਨਵਾਂ AstLogNot{c.flp, c.disableExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
            sigs.terminalActivep = ਨਵਾਂ AstLogAnd{c.flp, sigs.terminalActivep, notTermp};
            ਜੇ (sigs.rejectBasep) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ notDisp
                    = ਨਵਾਂ AstLogNot{c.flp, c.disableExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                sigs.rejectBasep = ਨਵਾਂ AstLogAnd{c.flp, sigs.rejectBasep, notDisp};
            }
            ਜੇ (sigs.throughoutRejectp) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ notDisp
                    = ਨਵਾਂ AstLogNot{c.flp, c.disableExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                sigs.throughoutRejectp = ਨਵਾਂ AstLogAnd{c.flp, sigs.throughoutRejectp, notDisp};
            }
            ਜੇ (sigs.required_ਕਦਮ_Rejectp) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ notDisp
                    = ਨਵਾਂ AstLogNot{c.flp, c.disableExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                sigs.required_ਕਦਮ_Rejectp = ਨਵਾਂ AstLogAnd{c.flp, sigs.required_ਕਦਮ_Rejectp, notDisp};
            }
            ਜੇ (sigs.ਮੇਲ_Countp) {
                sigs.ਮੇਲ_Countp = gateCheckedCountp(
                    c.flp, sigs.ਮੇਲ_Countp, sigs.ਮੇਲ_ਗਿਣਤੀ_Overflowp,
                    ਨਵਾਂ AstLogNot{c.flp, c.disableExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)});
            }
        }

        ਜੇ (sigs.ਮੇਲ_ਗਿਣਤੀ_Overflowp) {
            m_modp->ਜੋੜੋ_Stmtsp(
                ਨਵਾਂ AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_ਰੁੱਖ(ਝੂਠ),
                              new_ਗਿਣਤੀ_ਉਛਾਲ_Warning(c.flp, sigs.ਮੇਲ_ਗਿਣਤੀ_Overflowp)});
            sigs.ਮੇਲ_ਗਿਣਤੀ_Overflowp = ਖਾਲੀ_ਸੂਚਕ;
        }

        ਵਾਪਸ sigs;
    }

    // Phase 1: Resolve combinational Links via fixed-point propagation.
    ਖਾਲੀ resolveLinks(ਹੇਠਲਾ_Ctx& c, Ast_ਗੰਢ_ਵਿਅੰਜਕ* triggerExprp, ਅਚਲ ਤਾਰਕਿਕ ਗਿਣਤੀ_Matches) {
        // datap() was freshly allocated in lower() -- all state signals start null.
        c.ਗ੍ਰਾਫ.m_ਸ਼ੁਰੂ_Vertexp->datap()->stateSigp = triggerExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        c.ਗ੍ਰਾਫ.m_ਸ਼ੁਰੂ_Vertexp->datap()->state_ਗਿਣਤੀ_Sigp
            = ਨਵਾਂ AstExtend{c.flp, triggerExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), m_u32DTypep->ਚੌੜਾਈ()};
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtxr : c.ਗ੍ਰਾਫ.m_ਗ੍ਰਾਫ.vertices()) {
            Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = static_cast<Sva_State_ਸ਼ੀਰਸ਼*>(&vtxr);
            ਜੇ (vtxp->datap()->stateVarp) {
                vtxp->datap()->stateSigp
                    = ਨਵਾਂ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
                vtxp->datap()->state_ਗਿਣਤੀ_Sigp = ਨਵਾਂ AstExtend{
                    c.flp, ਨਵਾਂ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ},
                    m_u32DTypep->ਚੌੜਾਈ()};
            } ਨਹੀਂਤਾਂ ਜੇ (vtxp->datap()->delayRingVarp) {
                ਜੇ (vtxp->m_isFixedDelayRing) {
                    // state_count = wrapped ? head : 0; state = state_count != 0;
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ countp = delay_Ring_Output_ਗਿਣਤੀ(c.flp, vtxp);
                    vtxp->datap()->stateSigp = ਨਵਾਂ AstNeq{c.flp, countp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                                                          newTypedConstp(c.flp, m_u32DTypep, 0)};
                    vtxp->datap()->state_ਗਿਣਤੀ_Sigp = countp;
                } ਨਹੀਂਤਾਂ {
                    vtxp->datap()->stateSigp
                        = delay_Ring_Has_ਜੀਵਤ_Threadsp(c.flp, vtxp->datap()->delay_Ring_ਜੀਵਤ_ਗਿਣਤੀ_Varp);
                    vtxp->datap()->state_ਗਿਣਤੀ_Sigp = ਨਵਾਂ AstVarRef{
                        c.flp, vtxp->datap()->delay_Ring_ਜੀਵਤ_ਗਿਣਤੀ_Varp, VAccess::READ};
                }
            }
        }
        // Fixed-point propagation along zero-delay (Link) edges. Rebuild each
        // derived signal from its incoming edges on every pass; appending the same
        // contributions repeatedly makes merge expressions grow exponentially.
        ਅਚਲ ਮਾਪ_t passes = 2 * c.ਗ੍ਰਾਫ.m_ਗ੍ਰਾਫ.vertices().ਮਾਪ() + 2;
        ਲਈ (ਮਾਪ_t pass = 0; pass < passes; ++pass) {
            // Rebuild SAnd combiners once both sub-NFA terminals are available.
            ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtxr : c.ਗ੍ਰਾਫ.m_ਗ੍ਰਾਫ.vertices()) {
                Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = static_cast<Sva_State_ਸ਼ੀਰਸ਼*>(&vtxr);
                ਜੇ (!vtxp->m_isAndCombiner) ਜਾਰੀ;
                // AndCombiner vertices always have both terminal pointers set.
                UASSERT_OBJ(vtxp->m_andLhsTermp && vtxp->m_andRhsTermp, vtxp,
                            "AndCombiner vertex missing LHS/RHS terminal");
                ਅਚਲ Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ lhsTermp = vtxp->m_andLhsTermp;
                ਅਚਲ Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ rhsTermp = vtxp->m_andRhsTermp;
                ਜੇ (!lhsTermp->datap()->stateSigp || !rhsTermp->datap()->stateSigp) ਜਾਰੀ;
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਮੇਲ_Lp
                    = build_ਮੇਲ_Now(c.flp, lhsTermp->datap()->stateSigp, vtxp->m_andLhsCondp);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਮੇਲ_Rp
                    = build_ਮੇਲ_Now(c.flp, rhsTermp->datap()->stateSigp, vtxp->m_andRhsCondp);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਪੂਰਾ_LOrp = ਨਵਾਂ AstLogOr{
                    c.flp, ਨਵਾਂ AstVarRef{c.flp, vtxp->datap()->ਪੂਰਾ_LVarp, VAccess::READ}, ਮੇਲ_Lp};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਪੂਰਾ_ROrp = ਨਵਾਂ AstLogOr{
                    c.flp, ਨਵਾਂ AstVarRef{c.flp, vtxp->datap()->ਪੂਰਾ_RVarp, VAccess::READ}, ਮੇਲ_Rp};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ bothp = ਨਵਾਂ AstLogAnd{c.flp, ਪੂਰਾ_LOrp, ਪੂਰਾ_ROrp};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ oneNowp = ਨਵਾਂ AstLogOr{c.flp, ਮੇਲ_Lp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                                                          ਮੇਲ_Rp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                ਜੇ (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_ਰੁੱਖ(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = ਨਵਾਂ AstLogAnd{c.flp, bothp, oneNowp};
                ਜੇ (vtxp->datap()->state_ਗਿਣਤੀ_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_ਗਿਣਤੀ_Sigp->delete_ਰੁੱਖ(),
                                   vtxp->datap()->state_ਗਿਣਤੀ_Sigp);
                }
                vtxp->datap()->state_ਗਿਣਤੀ_Sigp = ਨਵਾਂ AstExtend{
                    c.flp, vtxp->datap()->stateSigp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), m_u32DTypep->ਚੌੜਾਈ()};
            }

            ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtxr : c.ਗ੍ਰਾਫ.m_ਗ੍ਰਾਫ.vertices()) {
                Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = static_cast<Sva_State_ਸ਼ੀਰਸ਼*>(&vtxr);
                ਜੇ (vtxp == c.ਗ੍ਰਾਫ.m_ਸ਼ੁਰੂ_Vertexp || vtxp->datap()->stateVarp
                    || vtxp->datap()->delayRingVarp || vtxp->m_isAndCombiner || vtxp->m_is_ਮੇਲ
                    || vtxp->m_isRejectSink) {
                    ਜਾਰੀ;
                }
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਗਲਾ_Statep = ਖਾਲੀ_ਸੂਚਕ;
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਗਲਾ_State_Countp = ਖਾਲੀ_ਸੂਚਕ;
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਗਲਾ_State_ਗਿਣਤੀ_Overflowp = ਖਾਲੀ_ਸੂਚਕ;
                ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& er : vtxp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
                    ਅਚਲ Sva_Trans_ਕਿਨਾਰਾ& te = static_cast<ਅਚਲ Sva_Trans_ਕਿਨਾਰਾ&>(er);
                    ਅਚਲ Sva_State_ਸ਼ੀਰਸ਼* ਅਚਲ fromVtxp = te.fromVtxp();
                    ਜੇ (!fromVtxp->datap()->stateSigp) ਜਾਰੀ;
                    UASSERT_OBJ(fromVtxp->datap()->state_ਗਿਣਤੀ_Sigp, fromVtxp,
                                "Link source missing stateCountSig");
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ contributionp = andCond(
                        c.flp, fromVtxp->datap()->stateSigp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), te.m_condp);
                    ਅਗਲਾ_Statep = orExprs(c.flp, ਅਗਲਾ_Statep, contributionp);
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਗਿਣਤੀ_Contributionp
                        = fromVtxp->datap()->state_ਗਿਣਤੀ_Sigp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਗਿਣਤੀ_Contribution_Overflowp
                        = ਗਿਣਤੀ_Matches && fromVtxp->datap()->state_ਗਿਣਤੀ_ਉਛਾਲ_Sigp
                              ? fromVtxp->datap()->state_ਗਿਣਤੀ_ਉਛਾਲ_Sigp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)
                              : ਖਾਲੀ_ਸੂਚਕ;
                    ਜੇ (te.m_condp) {
                        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ enablep = te.m_condp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                        ਗਿਣਤੀ_Contributionp
                            = ਗਿਣਤੀ_Matches
                                  ? gateCheckedCountp(c.flp, ਗਿਣਤੀ_Contributionp,
                                                      ਗਿਣਤੀ_Contribution_Overflowp, enablep)
                                  : ਜੋੜੋ_Countp(c.flp, ਖਾਲੀ_ਸੂਚਕ, ਗਿਣਤੀ_Contributionp, enablep);
                    }
                    ਅਗਲਾ_State_Countp
                        = ਗਿਣਤੀ_Matches
                              ? ਜੋੜੋ_Checked_Countp(c.flp, ਅਗਲਾ_State_Countp, ਗਿਣਤੀ_Contributionp,
                                                 ਅਗਲਾ_State_ਗਿਣਤੀ_Overflowp,
                                                 ਗਿਣਤੀ_Contribution_Overflowp)
                              : ਜੋੜੋ_Countp(c.flp, ਅਗਲਾ_State_Countp, ਗਿਣਤੀ_Contributionp);
                }
                ਜੇ (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_ਰੁੱਖ(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = ਅਗਲਾ_Statep;
                ਜੇ (vtxp->datap()->state_ਗਿਣਤੀ_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_ਗਿਣਤੀ_Sigp->delete_ਰੁੱਖ(),
                                   vtxp->datap()->state_ਗਿਣਤੀ_Sigp);
                }
                vtxp->datap()->state_ਗਿਣਤੀ_Sigp = ਅਗਲਾ_State_Countp;
                ਜੇ (vtxp->datap()->state_ਗਿਣਤੀ_ਉਛਾਲ_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_ਗਿਣਤੀ_ਉਛਾਲ_Sigp->delete_ਰੁੱਖ(),
                                   vtxp->datap()->state_ਗਿਣਤੀ_ਉਛਾਲ_Sigp);
                }
                vtxp->datap()->state_ਗਿਣਤੀ_ਉਛਾਲ_Sigp = ਅਗਲਾ_State_ਗਿਣਤੀ_Overflowp;
            }
        }
    }

ਜਨਤਕ:
    // Combine terminal/reject signals into final output expression.
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* assemble_ਨਤੀਜਾ(Ast_ਗੰਢ_Cover_Or_Assert* ਅਚਲ assertp, ਅਚਲ ਤਾਰਕਿਕ negated,
                                       Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਮੇਲ_Condp, ਅਚਲ Signal_ਸੈੱਟ& sigs,
                                       Ast_ਗੰਢ_ਵਿਅੰਜਕ** ਅਚਲ ਬਾਹਰਲਾ_Matchpp) {
        File_ਸਤਰ* ਅਚਲ flp = assertp->fileline();
        ਅਚਲ ਤਾਰਕਿਕ isCover = VN_IS(assertp, Cover);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ terminalActivep = sigs.terminalActivep;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rejectBasep = sigs.rejectBasep;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ throughoutRejectp = sigs.throughoutRejectp;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ required_ਕਦਮ_Rejectp = sigs.required_ਕਦਮ_Rejectp;
        // Property negation (IEEE 1800-2023 16.12.1 `not`): invert match/reject.
        ਜੇ (negated) {
            ਜੇ (isCover) {
                ਜੇ (terminalActivep)
                    VL_DO_DANGLING(terminalActivep->delete_ਰੁੱਖ(), terminalActivep);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* negRejectp = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (ਮੇਲ_Condp && rejectBasep) {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ sampledCondp = sampled(ਮੇਲ_Condp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ notCondp = ਨਵਾਂ AstLogNot{flp, sampledCondp};
                    negRejectp = ਨਵਾਂ AstLogAnd{flp, rejectBasep, notCondp};
                } ਨਹੀਂਤਾਂ ਜੇ (rejectBasep) {
                    VL_DO_DANGLING(rejectBasep->delete_ਰੁੱਖ(), rejectBasep);
                }
                ਜੇ (throughoutRejectp) negRejectp = orExprs(flp, negRejectp, throughoutRejectp);
                ਜੇ (required_ਕਦਮ_Rejectp)
                    negRejectp = orExprs(flp, negRejectp, required_ਕਦਮ_Rejectp);
                ਵਾਪਸ negRejectp ? negRejectp : ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
            }
            // Negated assert/assume: output = !match.
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* matchp = terminalActivep;
            ਜੇ (ਮੇਲ_Condp) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ sampledCondp = sampled(ਮੇਲ_Condp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
                matchp = ਨਵਾਂ AstLogAnd{flp, matchp, sampledCondp};
            }
            ਜੇ (ਬਾਹਰਲਾ_Matchpp) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* notPMatchp = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (ਮੇਲ_Condp && rejectBasep) {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ sampledCondp = sampled(ਮੇਲ_Condp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
                    notPMatchp = ਨਵਾਂ AstLogAnd{flp, rejectBasep->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                                               ਨਵਾਂ AstLogNot{flp, sampledCondp}};
                } ਨਹੀਂਤਾਂ ਜੇ (rejectBasep) {
                    notPMatchp = rejectBasep->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                }
                ਜੇ (throughoutRejectp)
                    notPMatchp = orExprs(flp, notPMatchp, throughoutRejectp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
                ਜੇ (required_ਕਦਮ_Rejectp)
                    notPMatchp
                        = orExprs(flp, notPMatchp, required_ਕਦਮ_Rejectp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
                *ਬਾਹਰਲਾ_Matchpp = notPMatchp;
            }
            ਜੇ (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_ਰੁੱਖ(), throughoutRejectp);
            ਜੇ (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_ਰੁੱਖ(), rejectBasep);
            ਜੇ (required_ਕਦਮ_Rejectp)
                VL_DO_DANGLING(required_ਕਦਮ_Rejectp->delete_ਰੁੱਖ(), required_ਕਦਮ_Rejectp);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਨਤੀਜਾ_Exprp = ਨਵਾਂ AstLogNot{flp, matchp};
            ਵਾਪਸ ਨਤੀਜਾ_Exprp;
        }
        ਜੇ (isCover) {
            ਜੇ (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_ਰੁੱਖ(), throughoutRejectp);
            ਜੇ (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_ਰੁੱਖ(), rejectBasep);
            ਜੇ (required_ਕਦਮ_Rejectp)
                VL_DO_DANGLING(required_ਕਦਮ_Rejectp->delete_ਰੁੱਖ(), required_ਕਦਮ_Rejectp);
            ਜੇ (ਮੇਲ_Condp) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ sampledCondp = sampled(ਮੇਲ_Condp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
                ਵਾਪਸ ਨਵਾਂ AstLogAnd{flp, terminalActivep, sampledCondp};
            }
            ਵਾਪਸ terminalActivep;
        }
        // Assert/assume: output = !reject
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* rejectp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (ਮੇਲ_Condp && rejectBasep) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ sampledCondp = sampled(ਮੇਲ_Condp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
            rejectp = ਨਵਾਂ AstLogAnd{flp, rejectBasep, ਨਵਾਂ AstLogNot{flp, sampledCondp}};
        } ਨਹੀਂਤਾਂ ਜੇ (rejectBasep) {
            VL_DO_DANGLING(rejectBasep->delete_ਰੁੱਖ(), rejectBasep);
        }
        ਜੇ (ਬਾਹਰਲਾ_Matchpp) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਮੇਲ_Exprp = terminalActivep->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            ਜੇ (ਮੇਲ_Condp) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ sp = sampled(ਮੇਲ_Condp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
                ਮੇਲ_Exprp = ਨਵਾਂ AstLogAnd{flp, ਮੇਲ_Exprp, sp};
            }
            *ਬਾਹਰਲਾ_Matchpp = ਮੇਲ_Exprp;
        }
        ਜੇ (terminalActivep) VL_DO_DANGLING(terminalActivep->delete_ਰੁੱਖ(), terminalActivep);
        ਜੇ (throughoutRejectp) rejectp = orExprs(flp, rejectp, throughoutRejectp);
        ਜੇ (required_ਕਦਮ_Rejectp) rejectp = orExprs(flp, rejectp, required_ਕਦਮ_Rejectp);
        ਜੇ (!rejectp) ਵਾਪਸ ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਨਤੀਜਾ_Exprp = ਨਵਾਂ AstLogNot{flp, rejectp};
        ਵਾਪਸ ਨਤੀਜਾ_Exprp;
    }

    ਸਪਸ਼ਟ SvaNfaLowering(Ast_ਗੰਢ_Module* modp)
        : m_modp{modp}
        , m_u32DTypep{modp->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::UINT32)} {}
    ~SvaNfaLowering() {
        V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("Assertions, NFA delay ring edge visits", m_stat_Delay_Ring_ਕਿਨਾਰਾ_Visits);
    }

    // Lower NFA graph to synthesizable AstAlways blocks and raw result signals.
    // Links are combinational; Edges are registered (NBA).
    Signal_ਸੈੱਟ ਹੇਠਲਾ(Ast_ਗੰਢ_Cover_Or_Assert* ਅਚਲ assertp, Sva_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ,
                    Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਮੇਲ_Condp,
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ disableExprp, ਅਚਲ ਤਾਰਕਿਕ need_Thread_Fail_ਗਿਣਤੀ,
                    ਅਚਲ ਤਾਰਕਿਕ need_Throughout_Thread_Fail_ਗਿਣਤੀ) {
        File_ਸਤਰ* ਅਚਲ flp = assertp->fileline();
        AstCover* ਅਚਲ coverp = VN_CAST(assertp, Cover);
        ਅਚਲ ਤਾਰਕਿਕ isSeqEvent = coverp && coverp->isSeqEvent();
        ਅਚਲ VAssertType assertType
            = isSeqEvent ? VAssertType{VAssertType::ਅੰਦਰੂਨੀ} : assertp->userType();
        ਅਚਲ VAssertDirectiveType directiveType
            = isSeqEvent ? VAssertDirectiveType{VAssertDirectiveType::ਅੰਦਰੂਨੀ}
                         : assertp->directive();
        ਅਚਲ std::string ਅਧਾਰ_Name = m_ਨਾਂ.get("");

        // Number vertices with sequential colors for array indexing.
        ਪੂਰਨ N = 0;
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtxr : ਗ੍ਰਾਫ.m_ਗ੍ਰਾਫ.vertices()) { vtxr.color(N++); }
        std::ਸਦਿਸ਼<Sva_State_ਸ਼ੀਰਸ਼*> vtx(N, ਖਾਲੀ_ਸੂਚਕ);
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtxr : ਗ੍ਰਾਫ.m_ਗ੍ਰਾਫ.vertices()) {
            vtx[vtxr.color()] = static_cast<Sva_State_ਸ਼ੀਰਸ਼*>(&vtxr);
        }

        // Allocate per-vertex lowering data (stored via V3GraphVertex::userp()).
        std::ਸਦਿਸ਼<Sva_ਸ਼ੀਰਸ਼_ਅੰਕੜੇ> ਸ਼ੀਰਸ਼_ਅੰਕੜੇ(N);
        ਲਈ (ਪੂਰਨ i = 0; i < N; ++i) vtx[i]->userp(&ਸ਼ੀਰਸ਼_ਅੰਕੜੇ[i]);

        // Identify registered vertices (targets of clocked edges).
        ਲਈ (ਪੂਰਨ i = 0; i < N; ++i) {
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& edger : vtx[i]->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                ਅਚਲ Sva_Trans_ਕਿਨਾਰਾ& tedger = static_cast<ਅਚਲ Sva_Trans_ਕਿਨਾਰਾ&>(edger);
                ਅਚਲ ਪੂਰਨ ਮੰਜ਼ਿਲ_Idx = tedger.ਮੰਜ਼ਿਲ_Vtxp()->color();
                ਜੇ (tedger.m_consumes_ਚੱਕਰ && tedger.ਮੰਜ਼ਿਲ_Vtxp() != ਗ੍ਰਾਫ.m_ਮੇਲ_Vertexp
                    && !tedger.ਮੰਜ਼ਿਲ_Vtxp()->m_isRejectSink) {
                    vtx[ਮੰਜ਼ਿਲ_Idx]->datap()->needsReg = ਸੱਚ;
                }
            }
        }

        // A packed range ring can emit several live endpoints without needing per-slot counts.
        // Widen only a ring whose input can already carry multiplicity.
        std::ਸਦਿਸ਼<ਤਾਰਕਿਕ> stateMayHaveMultiplicity(N);
        std::ਸਦਿਸ਼<ਤਾਰਕਿਕ> ringNeedsMultiplicity(N);
        std::ਸਦਿਸ਼<ਪੂਰਨ> multiplicityWork;
        ਅਚਲ ਤਾਰਕਿਕ ਗਿਣਤੀ_Matches = coverp && coverp->isCoverSeq() && !isSeqEvent;
        ਜੇ (ਗਿਣਤੀ_Matches) {
            ਲਈ (ਪੂਰਨ i = 0; i < N; ++i) {
                ਅਚਲ ਤਾਰਕਿਕ ਗੁਣਜ_Inputs = vtx[i]->ਵਿੱਚ_ਕਿਨਾਰੇ().ਮਾਪ() > 1;
                ਅਚਲ ਤਾਰਕਿਕ isRing = vtx[i]->m_delay_Ring_ਮਾਪ;
                ringNeedsMultiplicity[i] = isRing && ਗੁਣਜ_Inputs;
                ਅਚਲ ਤਾਰਕਿਕ startsMultiplicity = isRing
                                                    ? !vtx[i]->m_isFixedDelayRing
                                                    : ਗੁਣਜ_Inputs && !vtx[i]->datap()->needsReg;
                ਜੇ (startsMultiplicity) {
                    stateMayHaveMultiplicity[i] = ਸੱਚ;
                    multiplicityWork.ਧੱਕੋ_ਪਿੱਠ(i);
                }
            }
            ਲਈ (ਮਾਪ_t wi = 0; wi < multiplicityWork.ਮਾਪ(); ++wi) {
                ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& edger : vtx[multiplicityWork[wi]]->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                    ਅਚਲ Sva_Trans_ਕਿਨਾਰਾ& tedger = static_cast<ਅਚਲ Sva_Trans_ਕਿਨਾਰਾ&>(edger);
                    ਅਚਲ ਪੂਰਨ ti = tedger.ਮੰਜ਼ਿਲ_Vtxp()->color();
                    ਅਚਲ ਤਾਰਕਿਕ isRing = vtx[ti]->m_delay_Ring_ਮਾਪ;
                    ਜੇ (isRing) ringNeedsMultiplicity[ti] = ਸੱਚ;
                    // Match and reject sinks have no outgoing edges, so they stop naturally.
                    ਅਚਲ ਤਾਰਕਿਕ stopsMultiplicity = !isRing && vtx[ti]->datap()->needsReg;
                    ਜੇ (stateMayHaveMultiplicity[ti] || stopsMultiplicity) { ਜਾਰੀ; }
                    stateMayHaveMultiplicity[ti] = ਸੱਚ;
                    multiplicityWork.ਧੱਕੋ_ਪਿੱਠ(ti);
                }
            }
        }

        ਅਚਲ ਤਾਰਕਿਕ asyncDisable = isAsyncDisable(disableExprp);
        // Registers cleared on a rising disable are also written by their clocked block.
        File_ਸਤਰ* ਅਚਲ regFlp = asyncDisable ? ਨਵਾਂ File_ਸਤਰ{flp} : flp;
        ਜੇ (asyncDisable) regFlp->warnOff(V3_Error_ਕੋਡ::MULTIDRIVEN, ਸੱਚ);

        AstVar* ਅਚਲ killVarp
            = ਨਵਾਂ AstVar{flp, VVarType::MODULETEMP, ਅਧਾਰ_Name + "__kill", m_u32DTypep};
        killVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->ਜੋੜੋ_Stmtsp(killVarp);
        ਲਈ (ਪੂਰਨ i = 0; i < N; ++i) {
            ਜੇ (vtx[i]->m_isAndCombiner) {
                ਅਚਲ std::string ਅਧਾਰ = ਅਧਾਰ_Name + "__a" + std::ਮੰਜ਼ਿਲ_string(i);
                AstVar* ਅਚਲ lp = ਨਵਾਂ AstVar{regFlp, VVarType::MODULETEMP, ਅਧਾਰ + "_doneL",
                                              m_modp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType()};
                lp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->ਜੋੜੋ_Stmtsp(lp);
                vtx[i]->datap()->ਪੂਰਾ_LVarp = lp;
                AstVar* ਅਚਲ rp = ਨਵਾਂ AstVar{regFlp, VVarType::MODULETEMP, ਅਧਾਰ + "_doneR",
                                              m_modp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType()};
                rp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->ਜੋੜੋ_Stmtsp(rp);
                vtx[i]->datap()->ਪੂਰਾ_RVarp = rp;
                ਜਾਰੀ;
            }
            ਜੇ (vtx[i]->m_delay_Ring_ਮਾਪ) {
                ਅਚਲ std::string ਅਧਾਰ = ਅਧਾਰ_Name + "__d" + std::ਮੰਜ਼ਿਲ_string(i);
                ਅਚਲ ਤਾਰਕਿਕ storesCounts = ਗਿਣਤੀ_Matches && ringNeedsMultiplicity[i];
                Ast_ਗੰਢ_DType* ringDTypep = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (storesCounts) {
                    // int unsigned ring [size];
                    ringDTypep = ਨਵਾਂ AstUnpackArrayDType{
                        flp, m_u32DTypep,
                        ਨਵਾਂ AstRange{flp, static_cast<ਪੂਰਨ>(vtx[i]->m_delay_Ring_ਮਾਪ - 1), 0}};
                    v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(ringDTypep);
                } ਨਹੀਂਤਾਂ {
                    // bit [size-1:0] ring;
                    ringDTypep = m_modp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(vtx[i]->m_delay_Ring_ਮਾਪ,
                                                      vtx[i]->m_delay_Ring_ਮਾਪ, VSigning::UNSIGNED);
                }
                AstVar* ਅਚਲ ringp
                    = ਨਵਾਂ AstVar{flp, VVarType::MODULETEMP, ਅਧਾਰ + "_ring", ringDTypep};
                ringp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->ਜੋੜੋ_Stmtsp(ringp);
                vtx[i]->datap()->delayRingVarp = ringp;
                vtx[i]->datap()->delayRingCounts = storesCounts;
                AstVar* ਅਚਲ headp
                    = ਨਵਾਂ AstVar{flp, VVarType::MODULETEMP, ਅਧਾਰ + "_head",
                                 storesCounts ? m_u32DTypep : m_modp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType()};
                headp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->ਜੋੜੋ_Stmtsp(headp);
                vtx[i]->datap()->delay_Ring_ਸਿਰ_Varp = headp;
                // int unsigned idx;
                AstVar* ਅਚਲ idxp
                    = ਨਵਾਂ AstVar{regFlp, VVarType::MODULETEMP, ਅਧਾਰ + "_idx", m_u32DTypep};
                idxp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->ਜੋੜੋ_Stmtsp(idxp);
                vtx[i]->datap()->delayRingIdxVarp = idxp;
                // int unsigned live_count;
                AstVar* ਅਚਲ ਜੀਵਤ_ਗਿਣਤੀ_Varp
                    = ਨਵਾਂ AstVar{regFlp, VVarType::MODULETEMP, ਅਧਾਰ + "_liveCount", m_u32DTypep};
                ਜੀਵਤ_ਗਿਣਤੀ_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->ਜੋੜੋ_Stmtsp(ਜੀਵਤ_ਗਿਣਤੀ_Varp);
                vtx[i]->datap()->delay_Ring_ਜੀਵਤ_ਗਿਣਤੀ_Varp = ਜੀਵਤ_ਗਿਣਤੀ_Varp;
                AstVar* ਅਚਲ wrappedp = ਨਵਾਂ AstVar{regFlp, VVarType::MODULETEMP,
                                                    ਅਧਾਰ + "_wrapped", m_modp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType()};
                wrappedp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->ਜੋੜੋ_Stmtsp(wrappedp);
                vtx[i]->datap()->delayRingWrappedVarp = wrappedp;
                ਜਾਰੀ;
            }
            ਜੇ (!vtx[i]->datap()->needsReg) ਜਾਰੀ;
            ਜੇ (vtx[i] == ਗ੍ਰਾਫ.m_ਸ਼ੁਰੂ_Vertexp || vtx[i]->m_is_ਮੇਲ) ਜਾਰੀ;
            ਅਚਲ std::string varName = ਅਧਾਰ_Name + "__s" + std::ਮੰਜ਼ਿਲ_string(i);
            AstVar* ਅਚਲ varp
                = ਨਵਾਂ AstVar{regFlp, VVarType::MODULETEMP, varName, m_modp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->ਜੋੜੋ_Stmtsp(varp);
            vtx[i]->datap()->stateVarp = varp;
        }

        // Build lowering context for phase sub-functions.
        ਹੇਠਲਾ_Ctx c{flp,        senTreep,      disableExprp, ਮੇਲ_Condp,
                   assertType, directiveType, killVarp,     ਗ੍ਰਾਫ};

        // Phase 1: Resolve combinational Links via fixed-point propagation.
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ triggerExprp
            = isSeqEvent ? ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}
                         : assertOnCond(flp, assertp->userType(), assertp->directive());
        resolveLinks(c, triggerExprp, ਗਿਣਤੀ_Matches);
        VL_DO_DANGLING(triggerExprp->delete_ਰੁੱਖ(), triggerExprp);

        // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
        emit_State_ਰਜਿਸਟਰ_Nba(c);
        emitDelayRingUpdates(c, ਗਿਣਤੀ_Matches);
        emit_And_Combiner_ਪੂਰਾ_Latch_Nba(c);
        emitKillAckNba(c);

        // Phase 3/3a/3b: Compute terminal match/reject signals (cleans up stateSig).
        ਅਚਲ Signal_ਸੈੱਟ sigs
            = compute_ਸੰਕੇਤ(c, need_Thread_Fail_ਗਿਣਤੀ, need_Throughout_Thread_Fail_ਗਿਣਤੀ, ਗਿਣਤੀ_Matches);

        // Strong s_always[m:n] end-of-simulation liveness: if any in-window state
        // is still set at $finish, the universal-quantifier window never completed
        // (IEEE 1800-2023 16.12.11 strong semantics). Fire the assertion failure
        // from a final block; V3Assert turns the DT_ERROR display into the standard
        // "Assertion failed in %m" message.
        // pending = |{strong state registers, strong delay live counts != 0};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* pendingp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਪੂਰਨ i = 0; i < N; ++i) {
            ਜੇ (!vtx[i]->m_strongPending) ਜਾਰੀ;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* pendingExprp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (vtx[i]->datap()->stateVarp) {
                pendingExprp = ਨਵਾਂ AstVarRef{flp, vtx[i]->datap()->stateVarp, VAccess::READ};
            } ਨਹੀਂਤਾਂ ਜੇ (vtx[i]->m_strongAlwaysRing) {
                pendingExprp
                    = delay_Ring_Has_ਜੀਵਤ_Threadsp(flp, vtx[i]->datap()->delay_Ring_ਜੀਵਤ_ਗਿਣਤੀ_Varp);
            } ਨਹੀਂਤਾਂ {
                ਜਾਰੀ;
            }
            ਜੇ (!pendingp) {
                pendingp = pendingExprp;
            } ਨਹੀਂਤਾਂ {
                pendingp = ਨਵਾਂ AstLogOr{flp, pendingp, pendingExprp};
            }
        }
        ਜੇ (pendingp) {
            AstCExpr* ਅਚਲ assertOnp
                = ਨਵਾਂ AstCExpr{flp, AstCExpr::ਸ਼ੁੱਧ{}, "vlSymsp->_vm_contextp__->assertOn()", 1};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = ਨਵਾਂ AstLogAnd{flp, assertOnp, pendingp};
            AstDisplay* ਅਚਲ dispp
                = ਨਵਾਂ AstDisplay{flp, VDisplayType::DT_ERROR, "", ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
            dispp->fmtp()->timeunit(m_modp->timeunit());
            Ast_ਗੰਢ_Stmt* ਅਚਲ firep = dispp;
            ਜੇ (v3Global.opt.stopFail()) firep->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ AstStop{flp, ਝੂਠ});
            m_modp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstFinal{flp, ਨਵਾਂ AstIf{flp, condp, firep}});
        }

        // Clear userp on every vertex before vertexData is destroyed.
        ਲਈ (ਪੂਰਨ i = 0; i < N; ++i) vtx[i]->userp(ਖਾਲੀ_ਸੂਚਕ);
        ਵਾਪਸ sigs;
    }
};

}  // namespace

//######################################################################
// Top-level visitor

ਵਰਗ AssertNfaVisitor final : ਜਨਤਕ VNVisitor {
    // STATE
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module being processed
    AstClocking* m_defaultClockingp = ਖਾਲੀ_ਸੂਚਕ;  // Default clocking
    AstDefaultDisable* m_defaultDisablep = ਖਾਲੀ_ਸੂਚਕ;  // Default disable iff
    SvaNfaLowering* m_loweringp = ਖਾਲੀ_ਸੂਚਕ;  // NFA-to-hardware lowering engine
    Ast_Sen_ਰੁੱਖ* m_sampled_ਮੁੱਲ_Clockp = ਖਾਲੀ_ਸੂਚਕ;  // Inherited clock during scoped attachment
    V3_Unique_ਨਾਂ m_prop_Var_ਨਾਂ{"__Vpropvar"};  // Property-local variable names
    V3_Unique_ਨਾਂ m_prop_ਆਰਜ਼ੀ_ਨਾਂ{"__VnfaSampled"};  // Hoisted $sampled(propp) temps
    V3_Unique_ਨਾਂ m_fail_ਗਿਣਤੀ_ਨਾਂ{"__VnfaRemainingFailCount"};  // Fail replay counter names
    std::ਸੈੱਟ<ਅਚਲ AstProperty*> m_inliningProps;  // Recursion guard for inlineNamedProperty

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_ਗੰਢ>
    ਖਾਲੀ ਦੇਖੋ_Sampled_ਮੁੱਲ(T_ਗੰਢ* ਅਚਲ nodep) {
        ਜੇ (m_sampled_ਮੁੱਲ_Clockp && !nodep->sentreep()) {
            nodep->sentreep(m_sampled_ਮੁੱਲ_Clockp->clone_ਰੁੱਖ(ਸੱਚ));
        }
        iterateChildren(nodep);
    }

    // Wire match vertex and mid-window sources for a successful NFA build.
    ਸਥਿਰ ਖਾਲੀ wire_ਮੇਲ_And_Mid_Sources(Sva_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ, ਅਚਲ Build_ਨਤੀਜਾ& ਨਤੀਜਾ, File_ਸਤਰ* flp) {
        ਗ੍ਰਾਫ.create_ਮੇਲ_ਸ਼ੀਰਸ਼();
        // Skip the main term Link when midSources already cover every
        // end-of-match (cover_sequence path); otherwise the per-mid extraction
        // double-counts via the merge vertex.
        ਜੇ (!ਨਤੀਜਾ.ਪਦ_Is_Mid_ਮਿਲਾਓ) { ਗ੍ਰਾਫ.ਜੋੜੋ_Link(ਨਤੀਜਾ.ਪਦ_Vertexp, ਗ੍ਰਾਫ.m_ਮੇਲ_Vertexp); }
        ਲਈ (Sva_State_ਸ਼ੀਰਸ਼* srcVtxp : ਨਤੀਜਾ.midSources) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ tc : srcVtxp->m_throughoutConds) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ tcClone = tc->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                condp = condp ? ਨਵਾਂ AstLogAnd{flp, condp, tcClone} : tcClone;
            }
            ਗ੍ਰਾਫ.ਜੋੜੋ_Link(srcVtxp, ਗ੍ਰਾਫ.m_ਮੇਲ_Vertexp, condp);
            srcVtxp->m_isUnbounded = ਸੱਚ;
        }
    }

    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* getSequenceBodyExprp(ਅਚਲ AstSequence* seqp) {
        Ast_ਗੰਢ* bodyp = seqp->stmtsp();
        ਜਦੋਂ (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        ਵਾਪਸ VN_CAST(bodyp, ਗੰਢ_ਵਿਅੰਜਕ);
    }

    ਸਥਿਰ AstPropSpec* getPropertySpecp(ਅਚਲ AstProperty* propp) {
        Ast_ਗੰਢ* stmtp = propp->stmtsp();
        // V3LinkParse emits InitialStaticStmt for property-local variable
        // initialisers; the InitialAutomaticStmt variant only appears for
        // task/function-scope automatic lifetime, not properties.
        ਜਦੋਂ (stmtp
               && (VN_IS(stmtp, Var) || VN_IS(stmtp, InitialStaticStmt)
                   || VN_IS(stmtp, InitialAutomaticStmt))) {  // LCOV_EXCL_LINE
            stmtp = stmtp->nextp();
        }
        ਵਾਪਸ VN_CAST(stmtp, PropSpec);
    }

    ਖਾਲੀ inlineNamedProperty(AstPropSpec* outerSpecp, AstFuncRef* funcrefp,
                             ਅਚਲ AstProperty* propyp) {
        // Recursion guard: IEEE 1800-2023 16.12.1 forbids recursive properties.
        // V3Width emits "Recursive property call" for direct recursion before this
        // pass runs; this catches any nested-inlining cycle that slips past.
        ਜੇ (!m_inliningProps.emplace(propyp).ਦੂਜਾ) {
            funcrefp->v3error("Illegal recursive property reference");  // LCOV_EXCL_LINE
            ਵਾਪਸ;  // LCOV_EXCL_LINE
        }
        ਬਣਤਰ Guard final {
            std::ਸੈੱਟ<ਅਚਲ AstProperty*>& setr;
            ਅਚਲ AstProperty* keyp;
            ~Guard() { setr.erase(keyp); }
        } guard{m_inliningProps, propyp};
        AstPropSpec* propSpecp = getPropertySpecp(propyp);
        UASSERT_OBJ(propSpecp, funcrefp, "Property has no body PropSpec");
        propSpecp = propSpecp->clone_ਰੁੱਖ(ਝੂਠ);

        ਅਚਲ V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propyp->stmtsp());
        std::unordered_map<ਅਚਲ AstVar*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*> ਪੋਰਟ_Map;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& tconnect : tconnects) {
            ਪੋਰਟ_Map[tconnect.ਪਹਿਲਾ] = tconnect.ਦੂਜਾ->exprp();
        }

        // Promote property-local variables to module-level temps (IEEE 16.10).
        std::unordered_map<ਅਚਲ AstVar*, AstVar*> localVarMap;
        ਲਈ (Ast_ਗੰਢ* stmtp = propyp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(stmtp, Var)) {
                ਜੇ (!varp->isIO()) {
                    ਅਚਲ string newName = m_prop_Var_ਨਾਂ.get(varp);
                    AstVar* ਅਚਲ newVarp = ਨਵਾਂ AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->ਜੋੜੋ_Stmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        propSpecp->foreach([&](AstVarRef* refp) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ ਪੋਰਟ_It = ਪੋਰਟ_Map.ਲੱਭੋ(refp->varp());
            ਜੇ (ਪੋਰਟ_It != ਪੋਰਟ_Map.ਸਿਰਾ()) {
                refp->replaceWith(ਪੋਰਟ_It->ਦੂਜਾ->clone_ਰੁੱਖ(ਝੂਠ));
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(refp), refp);
                ਵਾਪਸ;
            }
            ਅਚਲ ਸਵੈਚਾਲਿਤ localIt = localVarMap.ਲੱਭੋ(refp->varp());
            ਜੇ (localIt != localVarMap.ਸਿਰਾ()) refp->varp(localIt->ਦੂਜਾ);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& tconnect : tconnects) {
            ਧੱਕੋ_Deletep(tconnect.ਦੂਜਾ->exprp()->unlink_Fr_ਪਿੱਠ());
        }
        V3AssertCommon::checkSvaInstanceDelays(propSpecp, funcrefp);

        // Merge disable iff (IEEE 1800-2023 16.12.1)
        ਜੇ (outerSpecp->disablep() && propSpecp->disablep()) {
            outerSpecp->v3error("disable iff expression before property call "
                                "and in its body is not legal");
            ਧੱਕੋ_Deletep(propSpecp->disablep()->unlink_Fr_ਪਿੱਠ());
        }
        ਜੇ (outerSpecp->disablep()) {
            propSpecp->disablep(outerSpecp->disablep()->unlink_Fr_ਪਿੱਠ());
        }

        ਜੇ (outerSpecp->sensesp() && propSpecp->sensesp()) {
            outerSpecp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Clock event before property call and in its body");
            ਧੱਕੋ_Deletep(propSpecp->sensesp()->unlink_Fr_ਪਿੱਠ());
        }
        ਜੇ (outerSpecp->sensesp()) {
            Ast_Sen_ਵਸਤੂ* ਅਚਲ sensesp = outerSpecp->sensesp();
            sensesp->unlink_Fr_ਪਿੱਠ();
            propSpecp->sensesp(sensesp);
        }

        outerSpecp->replaceWith(propSpecp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(outerSpecp), outerSpecp);
    }

    ਖਾਲੀ inlineSequenceRef(AstFuncRef* funcrefp, AstSequence* seqp) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ clonedp = bodyExprp->clone_ਰੁੱਖ(ਝੂਠ);

        ਅਚਲ V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ਅਚਲ AstVar*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*> ਪੋਰਟ_Map;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& tconnect : tconnects) {
            ਪੋਰਟ_Map[tconnect.ਪਹਿਲਾ] = tconnect.ਦੂਜਾ->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = ਪੋਰਟ_Map.ਲੱਭੋ(refp->varp());
            ਜੇ (it != ਪੋਰਟ_Map.ਸਿਰਾ()) {
                refp->replaceWith(it->ਦੂਜਾ->clone_ਰੁੱਖ(ਝੂਠ));
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(refp), refp);
            }
        });
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& tconnect : tconnects) {
            ਧੱਕੋ_Deletep(tconnect.ਦੂਜਾ->exprp()->unlink_Fr_ਪਿੱਠ());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(funcrefp), funcrefp);
        // Clear referenced flag so V3AssertPre cleanup does not emit
        // spurious UNSUPPORTED for sequences that were already inlined here.
        seqp->isReferenced(ਝੂਠ);
    }

    // Must run before hasMultiCycleExpr() so NFA sees sequence bodies.
    ਖਾਲੀ inlineAllSequenceRefs(Ast_ਗੰਢ* rootp) {
        ਤਾਰਕਿਕ ਬਦਲਿਆ = ਸੱਚ;
        ਜਦੋਂ (ਬਦਲਿਆ) {
            ਬਦਲਿਆ = ਝੂਠ;
            rootp->foreach([&](AstFuncRef* funcrefp) {
                ਜੇ (ਬਦਲਿਆ) ਵਾਪਸ;
                ਜੇ (AstSequence* ਅਚਲ seqp = VN_CAST(funcrefp->taskp(), Sequence)) {
                    inlineSequenceRef(funcrefp, seqp);
                    ਬਦਲਿਆ = ਸੱਚ;
                }
            });
        }
    }

    ਸਥਿਰ ਤਾਰਕਿਕ has_Multi_ਚੱਕਰ_ਵਿਅੰਜਕ(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਵਾਪਸ nodep->exists([](ਅਚਲ Ast_ਗੰਢ* np) {
            ਜੇ (ਅਚਲ ਸਵੈਚਾਲਿਤ* ਅਚਲ ep = VN_CAST(np, ਗੰਢ_ਵਿਅੰਜਕ)) ਵਾਪਸ ep->is_Multi_ਚੱਕਰ_Sva();
            ਵਾਪਸ ਝੂਠ;
        });
    }

    ਸਥਿਰ VPropStrength effectiveAssertPropStrength(ਅਚਲ AstPropSpec* ਅਚਲ propSpecp) {
        ਜੇ (propSpecp->propStrength() != VPropStrength::DEFAULT) ਵਾਪਸ propSpecp->propStrength();
        ਵਾਪਸ propSpecp->fileline()->language() <= V3_Lang_ਕੋਡ::L1800_2005 ? VPropStrength::STRONG
                                                                           : VPropStrength::WEAK;
    }

    // Bare `assert property (p until q)` with boolean operands stays on
    // V3AssertPre's AstLoop lowering, which preserves per-attempt action-block
    // firings that this NFA's single-bit aggregated state cannot. Strong bare
    // forms are also lowered there. NFA still owns sequence operands and any
    // embedding inside a multi-cycle context (implication consequent, or/and
    // operands, etc.).
    ਸਥਿਰ ਤਾਰਕਿਕ is_Bare_ਸਿਖਰ_Level_Until(Ast_ਗੰਢ* propp) {
        Ast_ਗੰਢ* p = propp;
        ਜੇ (AstPropSpec* ਅਚਲ specp = VN_CAST(p, PropSpec)) p = specp->propp();
        ਜਦੋਂ (AstLogNot* ਅਚਲ notp = VN_CAST(p, LogNot)) p = notp->lhsp();
        AstUntil* ਅਚਲ untilp = VN_CAST(p, Until);
        ਜੇ (!untilp) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ ਸਵੈਚਾਲਿਤ hasSeq = [](ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ep) {
            ਵਾਪਸ ep->exists([](ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* np) { ਵਾਪਸ np->is_Multi_ਚੱਕਰ_Sva(); });
        };
        ਜੇ (hasSeq(untilp->lhsp()) || hasSeq(untilp->rhsp())) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ ਸੱਚ;
    }

    ਬਣਤਰ PropertyParts final {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* triggerExprp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* seqExprp = ਖਾਲੀ_ਸੂਚਕ;
        ਤਾਰਕਿਕ isOverlapped = ਸੱਚ;
        ਤਾਰਕਿਕ hasImplication = ਝੂਠ;
        ਤਾਰਕਿਕ isFollowedBy = ਝੂਠ;  // True for #-# / #=# (non-vacuous-fail on antecedent miss)
    };

    ਸਥਿਰ PropertyParts decomposeProperty(Ast_ਗੰਢ* propp) {
        PropertyParts parts;
        ਜੇ (AstPropSpec* ਅਚਲ specp = VN_CAST(propp, PropSpec)) { propp = specp->propp(); }
        ਜੇ (AstImplication* ਅਚਲ implp = VN_CAST(propp, Implication)) {
            parts.hasImplication = ਸੱਚ;
            parts.isOverlapped = implp->isOverlapped();
            parts.isFollowedBy = implp->isFollowedBy();
            parts.triggerExprp = implp->lhsp();
            parts.seqExprp = implp->rhsp();
        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = VN_CAST(propp, ਗੰਢ_ਵਿਅੰਜਕ)) {
            parts.triggerExprp = ਖਾਲੀ_ਸੂਚਕ;
            parts.seqExprp = exprp;
        }
        ਵਾਪਸ parts;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ can_ਵੰਡੋ_Implication_Pass_Actions(ਅਚਲ PropertyParts& parts) {
        UASSERT(parts.hasImplication,
                "Implication pass action split requested without implication");
        UASSERT(parts.triggerExprp, "Implication pass action split requested without trigger");
        // Direct vacuous/nonvacuous classification uses the antecedent value in the current
        // assertion attempt. Leave delayed antecedents on the existing NFA pass path.
        ਵਾਪਸ !has_Multi_ਚੱਕਰ_ਵਿਅੰਜਕ(parts.triggerExprp);
    }

    ਸਥਿਰ ਖਾਲੀ ਵੰਡੋ_Implication_Pass_Actions(AstAssert* assertp, ਅਚਲ PropertyParts& parts,
                                            Ast_ਗੰਢ_ਵਿਅੰਜਕ* nonvacuousMatchp) {
        File_ਸਤਰ* ਅਚਲ flp = assertp->fileline();

        Ast_ਗੰਢ* ਅਚਲ passsp = assertp->passsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
        Ast_ਗੰਢ* splitsp = ਖਾਲੀ_ਸੂਚਕ;

        ਜੇ (!parts.isFollowedBy) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ vacuousp
                = ਨਵਾਂ AstLogNot{flp, sampled(parts.triggerExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ))};
            Ast_ਗੰਢ* ਅਚਲ vacuousBodyp = passsp->clone_ਰੁੱਖ(ਝੂਠ);
            splitsp = newPassOnIf(flp, vacuousp, vacuousBodyp, assertp->userType(),
                                  assertp->directive(), /*vacuous=*/ਸੱਚ);
        }

        AstIf* ਅਚਲ nonvacuousp
            = newPassOnIf(flp, nonvacuousMatchp, passsp, assertp->userType(), assertp->directive(),
                          /*vacuous=*/ਝੂਠ);
        splitsp = splitsp ? Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ<Ast_ਗੰਢ, Ast_ਗੰਢ>(splitsp, nonvacuousp)
                          : static_cast<Ast_ਗੰਢ*>(nonvacuousp);
        assertp->ਜੋੜੋ_Passsp(splitsp);
    }

    // On a PropSpec-wrapped assertion whose NFA build failed with a semantic
    // error (errorEmitted), replace the body with a BitFalse const so later
    // passes see a well-formed AST. Returns true if replaced.
    ਖਾਲੀ replaceBodyOnBuildError(File_ਸਤਰ* flp, AstPropSpec* propSpecp, ਤਾਰਕਿਕ errorEmitted) {
        ਜੇ (!errorEmitted) ਵਾਪਸ;
        Ast_ਗੰਢ* ਅਚਲ innerPropp = propSpecp->propp();
        innerPropp->replaceWith(ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(innerPropp), innerPropp);
    }

    // Hoist a leading clocking event (IEEE 1800-2023 16.7):
    ਤਾਰਕਿਕ hoistClockedSeq(AstPropSpec* specp) {
        ਜਦੋਂ (AstSClocked* ਅਚਲ clockedp = VN_CAST(specp->propp(), SClocked)) {
            ਜੇ (specp->sensesp()) {
                clockedp->v3warn(E_UNSUPPORTED, "Unsupported: multiclocked sequence or property");
                replaceBodyOnBuildError(specp->fileline(), specp, ਸੱਚ);
                ਵਾਪਸ ਸੱਚ;
            }
            ਲਈ (ਅਚਲ Ast_Sen_ਵਸਤੂ* sp = clockedp->sensesp(); sp;
                 sp = VN_CAST(sp->nextp(), Sen_ਵਸਤੂ)) {
                ਜੇ (!sp->ਕਿਨਾਰਾ_Type().an_ਕਿਨਾਰਾ()) {
                    clockedp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: non-edge clocking event on a sequence; "
                                     "use an edge such as @(posedge clk)");
                    replaceBodyOnBuildError(specp->fileline(), specp, ਸੱਚ);
                    ਵਾਪਸ ਸੱਚ;
                }
            }
            specp->sensesp(clockedp->sensesp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ bodyp = clockedp->exprp()->unlink_Fr_ਪਿੱਠ();
            clockedp->replaceWith(bodyp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(clockedp), clockedp);
        }
        // A clocking event anywhere else in the sequence is not supported.
        ਅਚਲ AstSClocked* nestedp = ਖਾਲੀ_ਸੂਚਕ;
        specp->propp()->foreach([&](ਅਚਲ AstSClocked* p) {
            ਜੇ (!nestedp) nestedp = p;
        });
        ਜੇ (nestedp) {
            nestedp->v3warn(E_UNSUPPORTED,
                            "Unsupported: clocking event inside sequence expression");
            replaceBodyOnBuildError(specp->fileline(), specp, ਸੱਚ);
            ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // Build the NFA graph for a property body, handling both the antecedent
    // |-> consequent and simple sequence cases. Returns the consequent/body
    // BuildResult (invalid on parse/build failure).
    Build_ਨਤੀਜਾ build_Assertion_ਗ੍ਰਾਫ(SvaNfaBuilder& builder, Sva_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ, Ast_ਗੰਢ_ਵਿਅੰਜਕ* seqBodyp,
                                    ਅਚਲ PropertyParts& parts, File_ਸਤਰ* flp) {
        ਜੇ (!parts.hasImplication) ਵਾਪਸ builder.build(seqBodyp);

        ਗ੍ਰਾਫ.m_ਸ਼ੁਰੂ_Vertexp = ਗ੍ਰਾਫ.create_State_ਸ਼ੀਰਸ਼();
        ਵਾਪਸ builder.build_Implication_ਕਿਨਾਰੇ(parts.triggerExprp, seqBodyp, ਗ੍ਰਾਫ.m_ਸ਼ੁਰੂ_Vertexp,
                                             parts.isOverlapped, parts.isFollowedBy,
                                             parts.triggerExprp, flp);
    }

    // Install pass-action gating and replay simultaneous per-thread failures.
    ਖਾਲੀ attachActionHandlers(AstAssert* ਅਚਲ assertp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਮੇਲ_Exprp,
                              Ast_Sen_ਰੁੱਖ* ਅਚਲ threadFailReplaySenTreep,
                              Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ threadFailCountp) {
        // Gate pass handler on match to prevent vacuous-pass firings.
        ਜੇ (ਮੇਲ_Exprp) {
            // needMatch implies passsp() was non-null when evaluated above;
            // lowering does not mutate the assert's pass-action between the
            // two reads, so passsp() is still non-null here.
            Ast_ਗੰਢ* passsp = assertp->passsp();
            UASSERT_OBJ(passsp, assertp, "needMatch set but passsp is null");
            passsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            File_ਸਤਰ* ਅਚਲ flp = assertp->fileline();
            AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, ਮੇਲ_Exprp, passsp, ਖਾਲੀ_ਸੂਚਕ};
            assertp->ਜੋੜੋ_Passsp(ifp);
            // Fail-handler prefix for overlapping instances (IEEE 16.12):
            // fires when reject=1 && match=1 in the same cycle.
            ਜੇ (assertp->failsp()) assertp->prependFailsp(ifp->clone_ਰੁੱਖ(ਝੂਠ));
        }

        ਜੇ (threadFailCountp) {
            UASSERT_OBJ(threadFailReplaySenTreep, assertp,
                        "Thread fail count missing sensitivity tree");
            Ast_ਗੰਢ* ਅਚਲ failsp = assertp->failsp();
            File_ਸਤਰ* ਅਚਲ flp = assertp->fileline();
            // IEEE 1800-2023 16.12 requires one action-block evaluation per failed
            // thread. AstAssert handles the first, so replay the rest here.
            AstVar* ਅਚਲ ਬਚਿਆ_Fail_ਗਿਣਤੀ_Varp
                = ਨਵਾਂ AstVar{flp, VVarType::MODULETEMP, m_fail_ਗਿਣਤੀ_ਨਾਂ.get(""),
                             m_modp->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::UINT32)};
            ਬਚਿਆ_Fail_ਗਿਣਤੀ_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->ਜੋੜੋ_Stmtsp(ਬਚਿਆ_Fail_ਗਿਣਤੀ_Varp);
            Ast_ਗੰਢ* ਅਚਲ replayStmtsp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                flp, ਨਵਾਂ AstVarRef{flp, ਬਚਿਆ_Fail_ਗਿਣਤੀ_Varp, VAccess::WRITE}, threadFailCountp};
            AstLoop* ਅਚਲ replayLoopp = ਨਵਾਂ AstLoop{flp};
            replayLoopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{
                flp, replayLoopp,
                ਨਵਾਂ AstGt{flp, ਨਵਾਂ AstVarRef{flp, ਬਚਿਆ_Fail_ਗਿਣਤੀ_Varp, VAccess::READ},
                          newTypedConstp(flp, ਬਚਿਆ_Fail_ਗਿਣਤੀ_Varp->dtypep(), 1)}});
            replayLoopp->ਜੋੜੋ_Stmtsp(newIfAssertFailOn(failsp->clone_ਰੁੱਖ(ਸੱਚ), assertp->directive(),
                                                     assertp->userType()));
            AstSub* ਅਚਲ decrementedFailCountp
                = ਨਵਾਂ AstSub{flp, ਨਵਾਂ AstVarRef{flp, ਬਚਿਆ_Fail_ਗਿਣਤੀ_Varp, VAccess::READ},
                             newTypedConstp(flp, ਬਚਿਆ_Fail_ਗਿਣਤੀ_Varp->dtypep(), 1)};
            replayLoopp->ਜੋੜੋ_Stmtsp(
                ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, ਬਚਿਆ_Fail_ਗਿਣਤੀ_Varp, VAccess::WRITE},
                              decrementedFailCountp});
            replayStmtsp->ਜੋੜੋ_ਅਗਲਾ(replayLoopp);
            m_modp->ਜੋੜੋ_Stmtsp(
                ਨਵਾਂ AstAlways{flp, VAlwaysKwd::ALWAYS, threadFailReplaySenTreep, replayStmtsp});
        }
    }

    // Replace one VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). No-op if refp is not in matchMap.
    ਖਾਲੀ substitute_ਮੇਲ_ਵਸਤੂ_Ref(AstVarRef* refp, ਬਿਨਾਨਿਸ਼ਾਨੀ K,
                                ਅਚਲ std::unordered_map<ਅਚਲ AstVar*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*>& ਮੇਲ_Map) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = ਮੇਲ_Map.ਲੱਭੋ(refp->varp());
        ਜੇ (it == ਮੇਲ_Map.ਸਿਰਾ()) ਵਾਪਸ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = it->ਦੂਜਾ->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        ਜੇ (K > 0) {
            AstConst* ਅਚਲ ticksp = ਨਵਾਂ AstConst{refp->fileline(), AstConst::Widthed_ਮੁੱਲ{}, 32,
                                                  static_cast<uint32_t>(K)};
            AstPast* ਅਚਲ pastp = ਨਵਾਂ AstPast{refp->fileline(), newp, ticksp, ਖਾਲੀ_ਸੂਚਕ,
                                               /* propertyTiming */ ਸੱਚ};
            pastp->dtypeFrom(newp);
            newp = pastp;
        }
        refp->replaceWith(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(refp), refp);
        ਵਾਪਸ;
    }

    // Recursively walk a consequent. Returns cycle length consumed and
    // substitutes each VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). Reports E_UNSUPPORTED on non-constant
    // delays or composite sequence operators and returns -1 on failure.
    ਪੂਰਨ
    walk_Substitute_ਮੇਲ_ਚੀਜ਼ਾਂ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, ਬਿਨਾਨਿਸ਼ਾਨੀ K,
                             ਅਚਲ std::unordered_map<ਅਚਲ AstVar*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*>& ਮੇਲ_ਚੀਜ਼ਾਂ) {
        ਜੇ (AstSExpr* ਅਚਲ sexprp = VN_CAST(nodep, SExpr)) {
            // IEEE 1800-2023 16.9.2: cycle_delay's lhsp is a constant_expression
            // and the delay form in a sequence is always `##N`, folded by
            // V3Const + V3Param before V3AssertNfa. Range form `##[m:n]` is the
            // only user-visible reject here.
            AstDelay* ਅਚਲ delayp = VN_AS(sexprp->delayp(), Delay);
            UASSERT_OBJ(delayp->is_ਚੱਕਰ_Delay() && VN_IS(delayp->lhsp(), Const), sexprp,
                        "SVA cycle delay must have a constant lhsp");
            ਜੇ (delayp->isRangeDelay()) {
                sexprp->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                              "non-constant cycle delay in consequent"
                                              " (IEEE 1800-2023 16.10)");
                ਵਾਪਸ -1;
            }
            ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ delay_ਚੱਕਰਾਂ = VN_AS(delayp->lhsp(), Const)->ਮੰਜ਼ਿਲ_UInt();
            ਪੂਰਨ preLen = 0;
            ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ prep = sexprp->preExprp()) {
                preLen = walk_Substitute_ਮੇਲ_ਚੀਜ਼ਾਂ(prep, K, ਮੇਲ_ਚੀਜ਼ਾਂ);
                ਜੇ (preLen < 0) ਵਾਪਸ -1;
            }
            ਅਚਲ ਪੂਰਨ bodyLen
                = walk_Substitute_ਮੇਲ_ਚੀਜ਼ਾਂ(sexprp->exprp(), K + preLen + delay_ਚੱਕਰਾਂ, ਮੇਲ_ਚੀਜ਼ਾਂ);
            ਜੇ (bodyLen < 0) ਵਾਪਸ -1;
            ਵਾਪਸ preLen + delay_ਚੱਕਰਾਂ + bodyLen;
        }
        ਜੇ (nodep->is_Multi_ਚੱਕਰ_Sva()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                         "composite sequence operator in consequent"
                                         " (IEEE 1800-2023 16.10)");
            ਵਾਪਸ -1;
        }
        std::ਸਦਿਸ਼<AstVarRef*> refs;
        nodep->foreach([&refs](AstVarRef* p) { refs.ਧੱਕੋ_ਪਿੱਠ(p); });
        ਲਈ (AstVarRef* ਅਚਲ refp : refs) substitute_ਮੇਲ_ਵਸਤੂ_Ref(refp, K, ਮੇਲ_ਚੀਜ਼ਾਂ);
        ਵਾਪਸ 0;
    }

    // Lower property-local match-item assignments before NFA construction.
    // Without this, the antecedent's AstExprStmt(<x = rhs_expr>, antBool)
    // survives into every NFA edge as a continuous-alias side-effect, so the
    // local-var temp tracks the current cycle's rhs_expr rather than the
    // antecedent-match cycle's value -- wrong for `|-> ##N` and `|=> ##N`
    // with N > 0 (issue #7587). Each consequent reference to the local var
    // is replaced with `$past(rhs_expr, K)` where K = (overlapped ? 0 : 1)
    // plus any accumulated `##N` delay. Returns true if E_UNSUPPORTED was
    // emitted; caller must replace the body with BitFalse and bail.
    ਤਾਰਕਿਕ lift_ਮੇਲ_ਵਸਤੂ_Substitutions(PropertyParts& parts, Ast_ਗੰਢ_ਵਿਅੰਜਕ* seqBodyp) {
        ਜੇ (!parts.hasImplication) ਵਾਪਸ ਝੂਠ;
        Ast_ਵਿਅੰਜਕ_Stmt* ਅਚਲ ਵਿਅੰਜਕ_Stmtp = VN_CAST(parts.triggerExprp, ਵਿਅੰਜਕ_Stmt);
        ਜੇ (!ਵਿਅੰਜਕ_Stmtp) ਵਾਪਸ ਝੂਠ;
        // IEEE 1800-2023 16.10 BNF requires `(expr, match_item {, match_item})`
        // with at least one match item; V3LinkParse only emits ExprStmt for
        // this form and only emits AstAssign with VarRef LHS for each item.
        std::unordered_map<ਅਚਲ AstVar*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*> ਮੇਲ_ਚੀਜ਼ਾਂ;
        ਲਈ (Ast_ਗੰਢ* stmtp = ਵਿਅੰਜਕ_Stmtp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_ਸੌਂਪੋ* ਅਚਲ assignp = VN_AS(stmtp, ਸੌਂਪੋ);
            AstVarRef* ਅਚਲ lhsRefp = VN_AS(assignp->lhsp(), VarRef);
            ਮੇਲ_ਚੀਜ਼ਾਂ[lhsRefp->varp()] = assignp->rhsp();
        }
        ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ ਸ਼ੁਰੂ_K = parts.isOverlapped ? 0 : 1;
        ਅਚਲ ਪੂਰਨ ਲੰਮਾਈ = walk_Substitute_ਮੇਲ_ਚੀਜ਼ਾਂ(seqBodyp, ਸ਼ੁਰੂ_K, ਮੇਲ_ਚੀਜ਼ਾਂ);
        // Match-item substitution / strip mutates ancestor purity. Release
        // builds don't auto-clear caches on edits, so refresh here.
        VIsCached::clear_Cache_ਰੁੱਖ();
        ਜੇ (ਲੰਮਾਈ < 0) ਵਾਪਸ ਸੱਚ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ antBoolp = ਵਿਅੰਜਕ_Stmtp->resultp()->unlink_Fr_ਪਿੱਠ();
        ਵਿਅੰਜਕ_Stmtp->replaceWith(antBoolp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(ਵਿਅੰਜਕ_Stmtp), ਵਿਅੰਜਕ_Stmtp);
        parts.triggerExprp = antBoolp;
        ਵਾਪਸ ਝੂਠ;
    }

    // Outcome counts for a property if/case are wrong in an outcome-multiplying
    // context. Returns that context, or nullptr when the shape is supported.
    ਸਥਿਰ ਅਚਲ ਅੱਖਰ* unsupportedPropertyControl(ਅਚਲ Ast_ਗੰਢ_Cover_Or_Assert* assertp,
                                                  ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* seqBodyp, ਤਾਰਕਿਕ negated) {
        ਜੇ (!hasPropertyControlConjunction(seqBodyp)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (negated) ਵਾਪਸ "negation";
        ਜੇ (VN_IS(assertp, Cover)) ਵਾਪਸ "cover";
        ਜੇ (VN_AS(assertp, Assert)->passsp()) ਵਾਪਸ "a pass action";
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }

    // Inline property/sequence refs and reject unsupported shapes.
    // Returns the PropSpec to lower, or nullptr when fully handled here.
    AstPropSpec* prepareAssertionProp(Ast_ਗੰਢ_Cover_Or_Assert* assertp) {
        ਜੇ (AstPropSpec* ਅਚਲ specp = VN_CAST(assertp->propp(), PropSpec)) {
            ਜੇ (AstFuncRef* ਅਚਲ funcrefp = VN_CAST(specp->propp(), FuncRef)) {
                ਜੇ (ਅਚਲ AstProperty* ਅਚਲ propyp = VN_CAST(funcrefp->taskp(), Property)) {
                    inlineNamedProperty(specp, funcrefp, propyp);
                }
            }
        }

        inlineAllSequenceRefs(assertp->propp());

        ਜੇ (AstPropSpec* ਅਚਲ specp = VN_CAST(assertp->propp(), PropSpec)) {
            ਜੇ (hoistClockedSeq(specp)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }

        AstPropSpec* ਅਚਲ propp = VN_AS(assertp->propp(), PropSpec);
        ਜੇ (!VN_IS(assertp, Cover)
            && effectiveAssertPropStrength(propp) == VPropStrength::STRONG) {
            propp->v3warn(E_UNSUPPORTED,
                          "Unsupported: strong property in " + assertp->verilogKwd() + ".");
            replaceBodyOnBuildError(assertp->fileline(), propp, /*errorEmitted=*/ਸੱਚ);
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }

        ਜੇ (!has_Multi_ਚੱਕਰ_ਵਿਅੰਜਕ(propp)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        // A nested property instance keeps its body behind the call; lowering would drop it.
        ਜੇ (propp->exists([](ਅਚਲ AstFuncRef* refp) { ਵਾਪਸ VN_IS(refp->taskp(), Property); })) {
            assertp->v3warn(E_UNSUPPORTED,
                            "Unsupported: property instance inside a multi-cycle property "
                            "expression");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(assertp->unlink_Fr_ਪਿੱਠ()), assertp);
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        ਜੇ (is_Bare_ਸਿਖਰ_Level_Until(propp)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਵਾਪਸ propp;
    }

    ਖਾਲੀ processAssertion(Ast_ਗੰਢ_Cover_Or_Assert* assertp) {
        ਜੇ (assertp->ਤੁਰੰਤ()) ਵਾਪਸ;

        AstPropSpec* ਅਚਲ propp = prepareAssertionProp(assertp);
        ਜੇ (!propp) ਵਾਪਸ;

        PropertyParts parts = decomposeProperty(propp);
        UASSERT_OBJ(parts.seqExprp, propp, "Property body must be an expression");

        // Unwrap `not` (IEEE 1800-2023 16.12.1); odd count -> negated semantics.
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* seqBodyp = parts.seqExprp;
        ਤਾਰਕਿਕ negated = ਝੂਠ;
        ਜਦੋਂ (AstLogNot* ਅਚਲ notp = VN_CAST(seqBodyp, LogNot)) {
            negated = !negated;
            seqBodyp = notp->lhsp();
        }

        ਅਚਲ ਅੱਖਰ* ਅਚਲ propertyControlp
            = unsupportedPropertyControl(assertp, seqBodyp, negated);

        // Substitute property-local match-item refs in consequent with
        // $past(rhs, K) before NFA build (IEEE 1800-2023 16.10).
        ਜੇ (lift_ਮੇਲ_ਵਸਤੂ_Substitutions(parts, seqBodyp)) {
            AstPropSpec* ਅਚਲ psp = VN_CAST(assertp->propp(), PropSpec);
            UASSERT_OBJ(psp, assertp, "Concurrent assertion must have PropSpec");
            replaceBodyOnBuildError(assertp->fileline(), psp, /*errorEmitted=*/ਸੱਚ);
            ਵਾਪਸ;
        }

        AstCover* ਅਚਲ coverp = VN_CAST(assertp, Cover);
        ਅਚਲ ਤਾਰਕਿਕ isCoverSeq = coverp && coverp->isCoverSeq();
        // A sequence event control is not an assertion directive; no default
        // disable iff, no assertion control
        ਅਚਲ ਤਾਰਕਿਕ isSeqEvent = coverp && coverp->isSeqEvent();
        ਅਚਲ ਤਾਰਕਿਕ ਗਿਣਤੀ_Matches = isCoverSeq && !isSeqEvent;
        // Inherit module defaults (IEEE 14.12, 16.15) when assertion has none.
        ਜੇ (!propp->sensesp() && m_defaultClockingp) {
            propp->sensesp(m_defaultClockingp->sentreep()->sensesp()->clone_ਰੁੱਖ(ਸੱਚ));
        }
        ਜੇ (!propp->disablep() && m_defaultDisablep && !isSeqEvent) {
            propp->disablep(m_defaultDisablep->condp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ));
        }
        ਜੇ (!propp->sensesp()) ਵਾਪਸ;
        Ast_Sen_ਰੁੱਖ* senTreep
            = ਨਵਾਂ Ast_Sen_ਰੁੱਖ{propp->fileline(), propp->sensesp()->clone_ਰੁੱਖ(ਸੱਚ)};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ disableExprp = propp->disablep();

        // NFA lowering clones repeated operands and may hoist them into an
        // always_comb block. Resolve implicit sampled-value clocks first, while
        // the enclosing assertion clock is still available.
        {
            VL_RESTORER(m_sampled_ਮੁੱਲ_Clockp);
            m_sampled_ਮੁੱਲ_Clockp = senTreep;
            iterate(propp->propp());
        }

        File_ਸਤਰ* ਅਚਲ flp = assertp->fileline();

        Sva_ਗ੍ਰਾਫ ਗ੍ਰਾਫ;
        SvaNfaBuilder builder{ਗ੍ਰਾਫ,      m_modp,     m_prop_ਆਰਜ਼ੀ_ਨਾਂ,
                              isCoverSeq, isSeqEvent, coverp != ਖਾਲੀ_ਸੂਚਕ};

        ਅਚਲ Build_ਨਤੀਜਾ ਨਤੀਜਾ = build_Assertion_ਗ੍ਰਾਫ(builder, ਗ੍ਰਾਫ, seqBodyp, parts, flp);
        ਜੇ (ਨਤੀਜਾ.valid()) wire_ਮੇਲ_And_Mid_Sources(ਗ੍ਰਾਫ, ਨਤੀਜਾ, flp);
        ਜੇ (!ਨਤੀਜਾ.valid()) {
            // Fall through to V3AssertPre for unsupported constructs; only
            // replace the body on real semantic errors. Any hoisted temps
            // from this attempt become orphan MODULETEMPs; V3Dead removes
            // them along with the dead always_comb driver.
            replaceBodyOnBuildError(flp, propp, ਨਤੀਜਾ.errorEmitted);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(senTreep), senTreep);
            ਵਾਪਸ;
        }
        // After the build, so a construct the builder rejects reports itself.
        ਜੇ (propertyControlp) {
            seqBodyp->v3warn(E_UNSUPPORTED,
                             "Unsupported: temporal property if/case with " << propertyControlp);
            replaceBodyOnBuildError(flp, propp, /*errorEmitted=*/ਸੱਚ);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(senTreep), senTreep);
            ਵਾਪਸ;
        }
        ਜੇ (ਗਿਣਤੀ_Matches && !v3Global.opt.coverageUser()) {
            SvaNfaBuilder::freeUnlinkedCondp(ਨਤੀਜਾ.finalCondp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(senTreep), senTreep);
            Ast_ਗੰਢ* ਅਚਲ innerPropp = propp->propp();
            innerPropp->replaceWith(ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(innerPropp), innerPropp);
            ਵਾਪਸ;
        }

        AstAssert* ਅਚਲ assertAssertp = VN_CAST(assertp, Assert);
        ਅਚਲ ਤਾਰਕਿਕ ਵੰਡੋ_Implication_Passsp = assertAssertp && assertAssertp->passsp()
                                            && parts.hasImplication
                                            && can_ਵੰਡੋ_Implication_Pass_Actions(parts);
        ਅਚਲ ਤਾਰਕਿਕ need_ਮੇਲ = assertAssertp && assertAssertp->passsp()
                               && (!parts.hasImplication || ਵੰਡੋ_Implication_Passsp);

        ਅਚਲ ਤਾਰਕਿਕ needsThreadFailReplay
            = assertAssertp && assertAssertp->failsp() && !parts.hasImplication;
        ਅਚਲ ਸਵੈਚਾਲਿਤ ਸੰਕੇਤ_2
            = m_loweringp->ਹੇਠਲਾ(assertp, ਗ੍ਰਾਫ, senTreep, ਨਤੀਜਾ.finalCondp, disableExprp,
                                 needsThreadFailReplay, needsThreadFailReplay && !negated);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਮੇਲ_Exprp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* outputExprp = m_loweringp->assemble_ਨਤੀਜਾ(
            assertp, negated, ਨਤੀਜਾ.finalCondp, ਸੰਕੇਤ_2, need_ਮੇਲ ? &ਮੇਲ_Exprp : ਖਾਲੀ_ਸੂਚਕ);
        ਜੇ (ਗਿਣਤੀ_Matches) {
            UASSERT_OBJ(ਸੰਕੇਤ_2.ਮੇਲ_Countp, coverp, "Cover sequence missing match count");
            VL_DO_DANGLING(outputExprp->delete_ਰੁੱਖ(), outputExprp);
            propp->ਮੇਲ_Countp(ਸੰਕੇਤ_2.ਮੇਲ_Countp);
            outputExprp = ਨਵਾਂ AstNeq{flp, ਸੰਕੇਤ_2.ਮੇਲ_Countp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                                     newTypedConstp(flp, ਸੰਕੇਤ_2.ਮੇਲ_Countp->dtypep(), 0)};
        }

        Ast_Sen_ਰੁੱਖ* ਅਚਲ threadFailReplaySenTreep
            = ਸੰਕੇਤ_2.threadFailCountp ? senTreep->clone_ਰੁੱਖ(ਝੂਠ) : ਖਾਲੀ_ਸੂਚਕ;

        VL_DO_DANGLING(ਧੱਕੋ_Deletep(senTreep), senTreep);
        ਜੇ (ਨਤੀਜਾ.finalCondp && !ਨਤੀਜਾ.finalCondp->backp()) ਧੱਕੋ_Deletep(ਨਤੀਜਾ.finalCondp);

        ਜੇ (ਵੰਡੋ_Implication_Passsp) {
            ਵੰਡੋ_Implication_Pass_Actions(assertAssertp, parts, ਮੇਲ_Exprp);
        } ਨਹੀਂਤਾਂ {
            attachActionHandlers(assertAssertp, ਮੇਲ_Exprp, threadFailReplaySenTreep,
                                 ਸੰਕੇਤ_2.threadFailCountp);
        }

        Ast_ਗੰਢ* ਅਚਲ innerPropp = propp->propp();
        innerPropp->replaceWith(outputExprp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(innerPropp), innerPropp);

        UINFO(4, "NFA converted assertion at " << flp << ਸਤਰ_ਅੰਤ);

        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) ਗ੍ਰਾਫ.m_ਗ੍ਰਾਫ.dumpDotFilePrefixed("assert-nfa");
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_loweringp);
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultDisablep);
        m_modp = nodep;
        // A class nested in a module is in the scope of the module's default clocking
        ਜੇ (!VN_IS(nodep, Class)) m_defaultClockingp = ਖਾਲੀ_ਸੂਚਕ;
        m_defaultDisablep = nodep->defaultDisablep();
        SvaNfaLowering lowering{nodep};
        m_loweringp = &lowering;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstClocking* nodep) override {
        ਜੇ (nodep->isDefault() && !m_defaultClockingp) m_defaultClockingp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Gen_ਖੰਡ* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstDefaultDisable* nodep) override {}
    ਖਾਲੀ ਦੇਖੋ(AstFell* nodep) override { ਦੇਖੋ_Sampled_ਮੁੱਲ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstPast* nodep) override { ਦੇਖੋ_Sampled_ਮੁੱਲ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstRose* nodep) override { ਦੇਖੋ_Sampled_ਮੁੱਲ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstStable* nodep) override { ਦੇਖੋ_Sampled_ਮੁੱਲ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstAssert* nodep) override { processAssertion(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstCover* nodep) override { processAssertion(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstRestrict* nodep) override {
        // Restrict property is ignored by simulators (IEEE 1800-2023 16.12.2).
        // Remove here so temporal SExpr don't leak to V3AssertPre.
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstAssertIntrinsic* nodep) override {}
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    ਸਪਸ਼ਟ AssertNfaVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Top entry point

ਖਾਲੀ V3AssertNfa::assertNfaAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":" << ਸਤਰ_ਅੰਤ);
    { AssertNfaVisitor{nodep}; }
    V3Global::dump_Check_Global_ਰੁੱਖ("assertnfa", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
