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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3AssertNfa.h"

#ಅನ್ತರ್ಭಾವ "V3Assert.h"
#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3Task.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <unordered_set>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// NFA Graph Data Structures (V3Graph-derived per upstream convention)

ನಾಮಾಕಾಶ {

ವರ್ಗ Sva_State_ಶೃಂಗ;

// Per-vertex algorithm data, stored via V3GraphVertex::userp() during lowering
ರಚನೆ Sva_ಶೃಂಗ_ದತ್ತಾಂಶ final {
    AstVar* stateVarp = ಶೂನ್ಯಸೂಚಕ;  // NBA state register for this vertex
    AstVar* delayRingVarp = ಶೂನ್ಯಸೂಚಕ;  // Packed occupancy bits or per-slot match counts
    AstVar* delay_Ring_ತಲೆ_Varp
        = ಶೂನ್ಯಸೂಚಕ;  // Cached outgoing slot; isolates readers from in-place writes
    AstVar* delayRingIdxVarp = ಶೂನ್ಯಸೂಚಕ;  // Next slot written in delayRingVarp
    AstVar* delay_Ring_ಜೀವಂತ_ಎಣಿಕೆ_Varp = ಶೂನ್ಯಸೂಚಕ;  // Number of threads in delayRingVarp
    AstVar* delayRingWrappedVarp = ಶೂನ್ಯಸೂಚಕ;  // All slots written since the last clear
    AstVar* ಮುಗಿದ_LVarp = ಶೂನ್ಯಸೂಚಕ;  // SAnd LHS done-latch
    AstVar* ಮುಗಿದ_RVarp = ಶೂನ್ಯಸೂಚಕ;  // SAnd RHS done-latch
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* stateSigp = ಶೂನ್ಯಸೂಚಕ;  // Combinational state signal; OWNED during lowering
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* state_ಎಣಿಕೆ_Sigp = ಶೂನ್ಯಸೂಚಕ;  // Cover-sequence match count; OWNED during lowering
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* state_ಎಣಿಕೆ_ಅತಿಪ್ರವಾಹ_Sigp = ಶೂನ್ಯಸೂಚಕ;  // Count overflow; OWNED during lowering
    ತಾರ್ಕಿಕ delayRingCounts = ಸುಳ್ಳು;  // Ring slots hold cover-sequence match counts
    ತಾರ್ಕಿಕ needsReg = ಸುಳ್ಳು;  // True if vertex has incoming clocked edge
};

// NFA state vertex -- one per NFA position in the sequence evaluation
ವರ್ಗ Sva_State_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Sva_State_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
ಸಾರ್ವಜನಿಕ:
    // True if this is the sequence-match terminal vertex
    ತಾರ್ಕಿಕ m_is_ಹೊಂದಿಕೆ = ಸುಳ್ಳು;
    // OWNED throughout-guard condition clones; IEEE 1800-2023 16.9.9
    std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> m_throughoutConds;
    // Nonzero for a ring-buffer vertex for ## delays.
    ತಾರ್ಕಿಕ m_isFixedDelayRing = ಸುಳ್ಳು;
    ಚಿಹ್ನೆರಹಿತ m_delay_Ring_ಗಾತ್ರ = 0;  // Number of ring slots. Range: max-min+1.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_delayRingClearCondp = ಶೂನ್ಯಸೂಚಕ;  // local RHS for pure-boolean range
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_delayRingAdvanceCondp = ಶೂನ್ಯಸೂಚಕ;  // Advance only when this condition holds
    ತಾರ್ಕಿಕ m_replayAbortReject = ಸುಳ್ಳು;  // Compressed repetition needs per-thread abort replay
    // OWNED; enclosing-abort fire condition clearing state or suppressing guard rejection
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_abortClearp = ಶೂನ್ಯಸೂಚಕ;
    // OWNED; reject-abort fire condition rejecting all represented live threads
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_abortRejectp = ಶೂನ್ಯಸೂಚಕ;
    // Liveness terminal (IEEE weak semantics): reject must not fire from this source
    ತಾರ್ಕಿಕ m_isUnbounded = ಸುಳ್ಳು;
    // Temporal sequence AND combiner; IEEE 1800-2023 16.9.5
    ತಾರ್ಕಿಕ m_isAndCombiner = ಸುಳ್ಳು;
    Sva_State_ಶೃಂಗ* m_andLhsTermp = ಶೂನ್ಯಸೂಚಕ;  // LHS sub-NFA terminal vertex
    Sva_State_ಶೃಂಗ* m_andRhsTermp = ಶೂನ್ಯಸೂಚಕ;  // RHS sub-NFA terminal vertex
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_andLhsCondp = ಶೂನ್ಯಸೂಚಕ;  // OWNED; LHS final condition (may be nullptr)
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_andRhsCondp = ಶೂನ್ಯಸೂಚಕ;  // OWNED; RHS final condition (may be nullptr)
    // Reject sink for SAnd rejectOnFail wiring; not a state-signal source
    ತಾರ್ಕಿಕ m_isRejectSink = ಸುಳ್ಳು;
    // In-window vertex of a strong s_always[m:n]: if its state is still set at
    // end-of-simulation the universal-quantifier window never completed, which is
    // a liveness failure (IEEE 1800-2023 16.12.11 strong semantics).
    ತಾರ್ಕಿಕ m_strongPending = ಸುಳ್ಳು;
    // Ring introduced for the checked portion of a bounded strong s_always window.
    ತಾರ್ಕಿಕ m_strongAlwaysRing = ಸುಳ್ಳು;

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ Sva_State_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp} {}
    ~Sva_State_ಶೃಂಗ() override {
        ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* cp : m_throughoutConds) VL_DO_DANGLING(cp->delete_ವೃಕ್ಷ(), cp);
        ಆದರೆ (m_delayRingClearCondp)
            VL_DO_DANGLING(m_delayRingClearCondp->delete_ವೃಕ್ಷ(), m_delayRingClearCondp);
        ಆದರೆ (m_delayRingAdvanceCondp)
            VL_DO_DANGLING(m_delayRingAdvanceCondp->delete_ವೃಕ್ಷ(), m_delayRingAdvanceCondp);
        ಆದರೆ (m_abortClearp) VL_DO_DANGLING(m_abortClearp->delete_ವೃಕ್ಷ(), m_abortClearp);
        ಆದರೆ (m_abortRejectp) VL_DO_DANGLING(m_abortRejectp->delete_ವೃಕ್ಷ(), m_abortRejectp);
        ಆದರೆ (m_andLhsCondp) VL_DO_DANGLING(m_andLhsCondp->delete_ವೃಕ್ಷ(), m_andLhsCondp);
        ಆದರೆ (m_andRhsCondp) VL_DO_DANGLING(m_andRhsCondp->delete_ವೃಕ್ಷ(), m_andRhsCondp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string name() ಸ್ಥಿರಾಂಕ override {
        string name = "s" + cvt_ಗಮ್ಯ_Str(color());
        ಆದರೆ (m_delay_Ring_ಗಾತ್ರ) {
            name += "\\n";
            name += m_isFixedDelayRing ? "fixed chain " : "range chain ";
            name += cvt_ಗಮ್ಯ_Str(m_delay_Ring_ಗಾತ್ರ) + " slots";
        }
        ಹಿಂತಿರುಗಿ name;
    }
    string dotColor() ಸ್ಥಿರಾಂಕ override {
        ಆದರೆ (m_is_ಹೊಂದಿಕೆ) ಹಿಂತಿರುಗಿ "red";
        ಆದರೆ (m_delay_Ring_ಗಾತ್ರ) ಹಿಂತಿರುಗಿ "blue";
        ಆದರೆ (m_isAndCombiner) ಹಿಂತಿರುಗಿ "purple";
        ಹಿಂತಿರುಗಿ "black";
    }
    // LCOV_EXCL_STOP
    // Access per-vertex algorithm data (valid only during lowering phase)
    Sva_ಶೃಂಗ_ದತ್ತಾಂಶ* datap() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ static_cast<Sva_ಶೃಂಗ_ದತ್ತಾಂಶ*>(userp()); }
};

// NFA transition edge -- clocked (##1) or combinational link (##0)
ವರ್ಗ Sva_Trans_ಅಂಚು final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಅಂಚು {
    VL_RTTI_IMPL(Sva_Trans_ಅಂಚು, V3_ನಕ್ಷೆ_ಅಂಚು)
ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_condp;  // Transition condition; nullptr = unconditional; OWNED
    ತಾರ್ಕಿಕ m_consumes_ಚಕ್ರ;  // true = clocked edge (##1), false = link (##0/boolean)
    // Reject when source is active and condp is false; set only on
    // outermost required-step Link
    ತಾರ್ಕಿಕ m_rejectOnFail = ಸುಳ್ಳು;
    // Optional dynamic condition vertex for m_rejectOnFail. Used when the
    // success condition is another NFA state rather than a static expression.
    Sva_State_ಶೃಂಗ* m_condVtxp = ಶೂನ್ಯಸೂಚಕ;

    // CONSTRUCTORS
    Sva_Trans_ಅಂಚು(V3_ನಕ್ಷೆ* graphp, V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp, V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp,
                 ತಾರ್ಕಿಕ consumes_ಚಕ್ರ)
        : V3_ನಕ್ಷೆ_ಅಂಚು{graphp, fromp, ತುದಿ, /*weight=*/1}
        , m_condp{condp}
        , m_consumes_ಚಕ್ರ{consumes_ಚಕ್ರ} {}
    ~Sva_Trans_ಅಂಚು() override {
        ಆದರೆ (m_condp) VL_DO_DANGLING(m_condp->delete_ವೃಕ್ಷ(), m_condp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string dotLabel() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_consumes_ಚಕ್ರ ? "##1" : "link"; }
    string dotStyle() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_consumes_ಚಕ್ರ ? "" : "dashed"; }
    // LCOV_EXCL_STOP
    // Typed accessors for NFA vertices
    Sva_State_ಶೃಂಗ* fromVtxp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ static_cast<Sva_State_ಶೃಂಗ*>(fromp()); }
    Sva_State_ಶೃಂಗ* ಗಮ್ಯ_Vtxp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ static_cast<Sva_State_ಶೃಂಗ*>(ತುದಿ()); }
};

// NFA graph container
ವರ್ಗ Sva_ನಕ್ಷೆ final {
ಸಾರ್ವಜನಿಕ:
    V3_ನಕ್ಷೆ m_ನಕ್ಷೆ;  // Owns all vertices and edges
    Sva_State_ಶೃಂಗ* m_ಆರಂಭ_Vertexp = ಶೂನ್ಯಸೂಚಕ;  // Trigger/start vertex
    Sva_State_ಶೃಂಗ* m_ಹೊಂದಿಕೆ_Vertexp = ಶೂನ್ಯಸೂಚಕ;  // Sequence-match terminal vertex

    // Create a new state vertex
    Sva_State_ಶೃಂಗ* create_State_ಶೃಂಗ() { ಹಿಂತಿರುಗಿ ಹೊಸ Sva_State_ಶೃಂಗ{&m_ನಕ್ಷೆ}; }
    // Create the match terminal vertex
    Sva_State_ಶೃಂಗ* create_ಹೊಂದಿಕೆ_ಶೃಂಗ() {
        Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = create_State_ಶೃಂಗ();
        vtxp->m_is_ಹೊಂದಿಕೆ = ನಿಜ;
        m_ಹೊಂದಿಕೆ_Vertexp = vtxp;
        ಹಿಂತಿರುಗಿ vtxp;
    }
    // Add a clocked transition edge (##1)
    Sva_Trans_ಅಂಚು* ಕೂಡಿಸು_Clocked_ಅಂಚು(Sva_State_ಶೃಂಗ* fromp, Sva_State_ಶೃಂಗ* ತುದಿ,
                                 Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = ಶೂನ್ಯಸೂಚಕ) {
        ಹಿಂತಿರುಗಿ ಹೊಸ Sva_Trans_ಅಂಚು{&m_ನಕ್ಷೆ, fromp, ತುದಿ, condp, /*consumesCycle=*/ನಿಜ};
    }
    // Add a combinational link (##0 / boolean condition)
    Sva_Trans_ಅಂಚು* ಕೂಡಿಸು_Link(Sva_State_ಶೃಂಗ* fromp, Sva_State_ಶೃಂಗ* ತುದಿ,
                          Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = ಶೂನ್ಯಸೂಚಕ) {
        ಹಿಂತಿರುಗಿ ಹೊಸ Sva_Trans_ಅಂಚು{&m_ನಕ್ಷೆ, fromp, ತುದಿ, condp, /*consumesCycle=*/ಸುಳ್ಳು};
    }
    // Collect all edges into a flat vector for iteration.
    // Used by the lowering phase which needs global edge scans.
    std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Sva_Trans_ಅಂಚು*> all_ಅಂಚುಗಳು() ಸ್ಥಿರಾಂಕ {
        std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Sva_Trans_ಅಂಚು*> ಫಲಿತಾಂಶ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtxr : m_ನಕ್ಷೆ.vertices()) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& edger : vtxr.ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                ಫಲಿತಾಂಶ.ತಳ್ಳು_ಹಿಂಭಾಗ(static_cast<ಸ್ಥಿರಾಂಕ Sva_Trans_ಅಂಚು*>(&edger));
            }
        }
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }
};

//######################################################################
// Builder result: terminal vertex + optional final condition (match Link condition).
ರಚನೆ Build_ಫಲಿತಾಂಶ final {
    Sva_State_ಶೃಂಗ* ಪದ_Vertexp;  // Primary terminal; contributes to both match and reject
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* finalCondp;  // nullptr = unconditional
    // Mid-window sources for range delays (pure boolean RHS): match-only (isUnbounded)
    std::ಸದಿಶ<Sva_State_ಶೃಂಗ*> midSources;
    ತಾರ್ಕಿಕ errorEmitted = ಸುಳ್ಳು;  // Builder already emitted specific error; skip generic
    // For cover_sequence: when true, midSources already enumerate every
    // end-of-match, so wireMatchAndMidSources must NOT add the main
    // termVtxp -> matchVertex Link (would double-count via the merge vertex).
    ತಾರ್ಕಿಕ ಪದ_Is_Mid_ವಿಲೀನ = ಸುಳ್ಳು;
    ತಾರ್ಕಿಕ valid() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಪದ_Vertexp != ಶೂನ್ಯಸೂಚಕ; }
    ಸ್ಥಿರ Build_ಫಲಿತಾಂಶ fail(ತಾರ್ಕಿಕ errored = ಸುಳ್ಳು) { ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ, {}, errored}; }
    ಸ್ಥಿರ Build_ಫಲಿತಾಂಶ failWithError() { ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ, {}, ನಿಜ}; }
};

// Parser-marked SAnd of overlapped implications: a property if/else/case.
ಸ್ಥಿರ ತಾರ್ಕಿಕ hasPropertyControlConjunction(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
    ಹಿಂತಿರುಗಿ nodep->exists([](ಸ್ಥಿರಾಂಕ AstSAnd* andp) { ಹಿಂತಿರುಗಿ andp->propertyControl(); });
}

ಸ್ಥಿರ AstConst* newTypedConstp(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep,
                                ಸ್ಥಿರಾಂಕ uint32_t ಮೌಲ್ಯ) {
    AstConst* ಸ್ಥಿರಾಂಕ constp = ಹೊಸ AstConst{flp, AstConst::DTyped{}, dtypep};
    constp->ಅಂಕೆ().ಗಣ_Long(ಮೌಲ್ಯ);
    ಹಿಂತಿರುಗಿ constp;
}

ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sampled(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
    ಹಿಂತಿರುಗಿ ಹೊಸ AstSampled{exprp->fileline(), exprp, exprp->dtypep(), ನಿಜ};
}

ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* assertPassOnQuery(ತಾರ್ಕಿಕ vacuous) {
    ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* queries[2]
        = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
    ಹಿಂತಿರುಗಿ queries[vacuous];
}

ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* assertOnCond(File_ಸಾಲು* flp, VAssertType type,
                                 VAssertDirectiveType directiveType) {
    ಆದರೆ (!v3Global.opt.assertOn()) { ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}}; }
    ಹಿಂತಿರುಗಿ ಹೊಸ AstCExpr{flp, AstCExpr::ಶುದ್ಧ{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
}

ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* assertKillGet(File_ಸಾಲು* flp, VAssertType type,
                                  VAssertDirectiveType directiveType) {
    ಹಿಂತಿರುಗಿ ಹೊಸ AstCExpr{flp, AstCExpr::ಶುದ್ಧ{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_KILL", type, directiveType),
                        32};
}

ಸ್ಥಿರ string assert_Action_Control_ಪೂರ್ವಪ್ರತ್ಯಯ(VAssertDirectiveType directiveType) {
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ controlled = !!(static_cast<ಪೂರ್ಣಾಂಕ>(directiveType)
                              & (static_cast<ಪೂರ್ಣಾಂಕ>(VAssertDirectiveType::ASSERT)
                                 | static_cast<ಪೂರ್ಣಾಂಕ>(VAssertDirectiveType::COVER)
                                 | static_cast<ಪೂರ್ಣಾಂಕ>(VAssertDirectiveType::ASSUME)));
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ checkRuntime = controlled & static_cast<ಪೂರ್ಣಾಂಕ>(v3Global.opt.assertOn());
    ಹಿಂತಿರುಗಿ "("s + std::ಗಮ್ಯ_string(controlled ^ 1) + " || ("s + std::ಗಮ್ಯ_string(checkRuntime)
           + " && "s;
}

ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* assertPassOnCond(File_ಸಾಲು* flp, VAssertType type,
                                     VAssertDirectiveType directiveType, ತಾರ್ಕಿಕ vacuous) {
    ಹಿಂತಿರುಗಿ ಹೊಸ AstCExpr{
        flp, AstCExpr::ಶುದ್ಧ{},
        assert_Action_Control_ಪೂರ್ವಪ್ರತ್ಯಯ(directiveType)
            + V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType)
            + "))"s,
        1};
}

ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* assertFailOnCond(File_ಸಾಲು* flp, VAssertType type,
                                     VAssertDirectiveType directiveType) {
    ಹಿಂತಿರುಗಿ ಹೊಸ AstCExpr{
        flp, AstCExpr::ಶುದ್ಧ{},
        assert_Action_Control_ಪೂರ್ವಪ್ರತ್ಯಯ(directiveType)
            + V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType) + "))"s,
        1};
}

ಸ್ಥಿರ AstIf* newPassOnIf(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* firep, Ast_ಗಂಟು* bodyp, VAssertType type,
                          VAssertDirectiveType directiveType, ತಾರ್ಕಿಕ vacuous) {
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp
        = ಹೊಸ AstLogAnd{flp, firep, assertPassOnCond(flp, type, directiveType, vacuous)};
    AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(ನಿಜ);
    ifp->user1(ನಿಜ);
    ಹಿಂತಿರುಗಿ ifp;
}

ಸ್ಥಿರ Ast_ಗಂಟು_Stmt* newIfAssertFailOn(Ast_ಗಂಟು* bodyp, VAssertDirectiveType directiveType,
                                      VAssertType type) {
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = bodyp->fileline();
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = assertFailOnCond(flp, type, directiveType);
    AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(ನಿಜ);
    ifp->user1(ನಿಜ);
    ಹಿಂತಿರುಗಿ ifp;
}

//######################################################################
// NFA Builder

ವರ್ಗ SvaNfaBuilder final {
    Sva_ನಕ್ಷೆ& m_ನಕ್ಷೆ;  // NFA graph being built
    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ m_modp;  // Module to receive hoisted sampled-prop temps
    V3_Unique_ಹೆಸರುಗಳು& m_prop_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು;  // Module-shared temp-var name source
    std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> m_temporal_Guard_ರಾಶಿ;  // Guards active across nested temporal states
    // Outer abort conditions, AND-ed as !cond into inner abort edges
    // (IEEE 1800-2023 16.12.14 outer-wraps-inner).
    std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> m_outer_Abort_ರಾಶಿ;
    ತಾರ್ಕಿಕ m_ಒಳ_Unbounded_Scope = ಸುಳ್ಳು;  // Sticky: nodes created after inherit liveness
    ತಾರ್ಕಿಕ m_markStrongPending = ಸುಳ್ಳು;  // Mark new vertices as strong s_always in-window
    ತಾರ್ಕಿಕ m_isCover = ಸುಳ್ಳು;  // Cover directives do not fail at end-of-simulation
    // IEEE 1800-2023 16.14.3 cover sequence: each end-of-match fires the action,
    // not just the first. Builder builds parallel-branch (no first-match-wins)
    // topology when true. Default false preserves cover_property semantics.
    ತಾರ್ಕಿಕ m_isCoverSeq = ಸುಳ್ಳು;
    // Unsupported endpoint topology must reject, not ignore, or the wait hangs
    ತಾರ್ಕಿಕ m_isSeqEvent = ಸುಳ್ಳು;
    ತಾರ್ಕಿಕ m_ಒಳ_Sequence_ಪೂರ್ವಪ್ರತ್ಯಯ = ಸುಳ್ಳು;  // Prefix composition does not preserve every endpoint
    ತಾರ್ಕಿಕ m_want_Bool_ಎಲೆ = ಸುಳ್ಳು;  // Building a |-> / |=> or top-level body (see buildOrMerge)

    ರಚನೆ RangeDelayRejectInfo final {
        Sva_State_ಶೃಂಗ* startp = ಶೂನ್ಯಸೂಚಕ;
        ಚಿಹ್ನೆರಹಿತ range = 0;
        ಪೂರ್ಣಾಂಕ rhsLen = 0;
    };

    ಶೂನ್ಯ warnEndpointUnsupported(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ string& what) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (m_isSeqEvent) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: sequence used as an event control with " << what);
        } ಇಲ್ಲದಿದ್ದರೆ {
            flp->v3warn(COVERIGN, "Ignoring unsupported: cover sequence with " << what);
        }
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* throughoutCond(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ತಳ_Condp, File_ಸಾಲು* flp) {
        ಆದರೆ (m_temporal_Guard_ರಾಶಿ.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ತಳ_Condp;
        // AND all active temporal guards (supports nesting)
        // Each must use $sampled values.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* guardp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp : m_temporal_Guard_ರಾಶಿ) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ clonep = sampled(condp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
            ಆದರೆ (!guardp) {
                guardp = clonep;
            } ಇಲ್ಲದಿದ್ದರೆ {
                guardp = ಹೊಸ AstLogAnd{flp, guardp, clonep};
            }
        }
        ಆದರೆ (ತಳ_Condp) { guardp = ಹೊಸ AstLogAnd{flp, ತಳ_Condp, guardp}; }
        ಹಿಂತಿರುಗಿ guardp;
    }

    ಸ್ಥಿರ ಚಿಹ್ನೆರಹಿತ getConstUInt(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ constp = V3Const::constifyEdit(exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ cp = VN_CAST(constp, Const);
        ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ val = cp ? cp->ಗಮ್ಯ_UInt() : 0;
        VL_DO_DANGLING(constp->delete_ವೃಕ್ಷ(), constp);
        ಹಿಂತಿರುಗಿ val;
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
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ fixed_ಉದ್ದಳತೆ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಆದರೆ (AstSExpr* ಸ್ಥಿರಾಂಕ sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ಸ್ಥಿರಾಂಕ delayp = VN_CAST(sexprp->delayp(), Delay);
            ಆದರೆ (!delayp || !delayp->is_ಚಕ್ರ_Delay()) ಹಿಂತಿರುಗಿ -1;
            ಚಿಹ್ನೆರಹಿತ delay_ಚಕ್ರಗಳು;
            ಆದರೆ (delayp->isRangeDelay()) {
                ಆದರೆ (delayp->isUnbounded()) ಹಿಂತಿರುಗಿ -1;  // LCOV_EXCL_LINE
                ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ minD = getConstUInt(delayp->lhsp());
                ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ maxD = getConstUInt(delayp->rhsp());
                ಆದರೆ (minD != maxD) ಹಿಂತಿರುಗಿ -1;
                delay_ಚಕ್ರಗಳು = minD;
            } ಇಲ್ಲದಿದ್ದರೆ {
                delay_ಚಕ್ರಗಳು = getConstUInt(delayp->lhsp());
            }
            ಪೂರ್ಣಾಂಕ preLen = 0;
            ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ prep = sexprp->preExprp()) {
                preLen = fixed_ಉದ್ದಳತೆ(prep);
                ಆದರೆ (preLen < 0) ಹಿಂತಿರುಗಿ -1;  // LCOV_EXCL_LINE
            }
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ bodyLen = fixed_ಉದ್ದಳತೆ(sexprp->exprp());
            ಆದರೆ (bodyLen < 0) ಹಿಂತಿರುಗಿ -1;  // LCOV_EXCL_LINE
            ಹಿಂತಿರುಗಿ preLen + delay_ಚಕ್ರಗಳು + bodyLen;
        }
        ಆದರೆ (AstSThroughout* ಸ್ಥಿರಾಂಕ throughp = VN_CAST(nodep, SThroughout)) {
            ಹಿಂತಿರುಗಿ fixed_ಉದ್ದಳತೆ(throughp->rhsp());
        }
        ಆದರೆ (AstSOr* ಸ್ಥಿರಾಂಕ orp = VN_CAST(nodep, SOr)) {
            // Alternatives must share one end cycle; buildSWithin relies on
            // this to pair the OR with an SIntersect.
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lhsLen = fixed_ಉದ್ದಳತೆ(orp->lhsp());
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ rhsLen = fixed_ಉದ್ದಳತೆ(orp->rhsp());
            ಆದರೆ (lhsLen < 0 || rhsLen < 0 || lhsLen != rhsLen) ಹಿಂತಿರುಗಿ -1;
            ಹಿಂತಿರುಗಿ lhsLen;
        }
        ಆದರೆ (AstSWithin* ಸ್ಥಿರಾಂಕ withinp = VN_CAST(nodep, SWithin)) {
            // `seq1 within seq2` ends at seq2's end cycle (IEEE 16.9.10).
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lhsLen = fixed_ಉದ್ದಳತೆ(withinp->lhsp());
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ rhsLen = fixed_ಉದ್ದಳತೆ(withinp->rhsp());
            ಆದರೆ (lhsLen < 0 || rhsLen < 0 || lhsLen > rhsLen) ಹಿಂತಿರುಗಿ -1;
            ಹಿಂತಿರುಗಿ rhsLen;
        }
        // LCOV_EXCL_START -- defensive: V3AssertPre rejects composite SVA ops
        // nested in an intersect arm before fixedLength runs (clock-context
        // resolution fails). Kept as a guard in case future parser relaxations
        // permit it.
        ಆದರೆ (nodep->is_Multi_ಚಕ್ರ_Sva()) ಹಿಂತಿರುಗಿ -1;
        // LCOV_EXCL_STOP
        // Plain boolean expression (no SVA constructs) -- 0 cycles.
        ಹಿಂತಿರುಗಿ 0;
    }

    // Contiguous match-length range [lo,hi] for an operand whose length varies
    // from AT MOST ONE ranged cycle delay; {-1,-1} otherwise. Drives the
    // variable-length `intersect` lowering (IEEE 1800-2023 16.9.6): more than
    // one ranged delay would make the per-length realization ambiguous, so it
    // is reported unsupported rather than mis-paired.
    ಸ್ಥಿರ std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ> ಉದ್ದಳತೆ_Range(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಆದರೆ (AstSExpr* ಸ್ಥಿರಾಂಕ sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ಸ್ಥಿರಾಂಕ delayp = VN_CAST(sexprp->delayp(), Delay);
            ಆದರೆ (!delayp || !delayp->is_ಚಕ್ರ_Delay()) ಹಿಂತಿರುಗಿ {-1, -1};
            std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ> delayRange;
            ಆದರೆ (delayp->isRangeDelay()) {
                ಆದರೆ (delayp->isUnbounded()) ಹಿಂತಿರುಗಿ {-1, -1};
                ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ minD = getConstUInt(delayp->lhsp());
                ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ maxD = getConstUInt(delayp->rhsp());
                delayRange = {minD, maxD};
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ d = getConstUInt(delayp->lhsp());
                delayRange = {d, d};
            }
            std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ> preRange{0, 0};
            ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ prep = sexprp->preExprp()) {
                preRange = ಉದ್ದಳತೆ_Range(prep);
                ಆದರೆ (preRange.ಮೊದಲ < 0) ಹಿಂತಿರುಗಿ {-1, -1};
            }
            ಸ್ಥಿರಾಂಕ std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ> bodyRange = ಉದ್ದಳತೆ_Range(sexprp->exprp());
            ಆದರೆ (bodyRange.ಮೊದಲ < 0) ಹಿಂತಿರುಗಿ {-1, -1};
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ variableParts = (preRange.ಮೊದಲ != preRange.ಎರಡನೇ)
                                      + (delayRange.ಮೊದಲ != delayRange.ಎರಡನೇ)
                                      + (bodyRange.ಮೊದಲ != bodyRange.ಎರಡನೇ);
            ಆದರೆ (variableParts > 1) ಹಿಂತಿರುಗಿ {-1, -1};
            ಹಿಂತಿರುಗಿ {preRange.ಮೊದಲ + delayRange.ಮೊದಲ + bodyRange.ಮೊದಲ,
                    preRange.ಎರಡನೇ + delayRange.ಎರಡನೇ + bodyRange.ಎರಡನೇ};
        }
        ಆದರೆ (AstSThroughout* ಸ್ಥಿರಾಂಕ throughp = VN_CAST(nodep, SThroughout)) {
            ಹಿಂತಿರುಗಿ ಉದ್ದಳತೆ_Range(throughp->rhsp());
        }
        ಆದರೆ (nodep->is_Multi_ಚಕ್ರ_Sva()) ಹಿಂತಿರುಗಿ {-1, -1};
        ಹಿಂತಿರುಗಿ {0, 0};  // plain boolean -- 0 cycles
    }

    // Clone `operand` with its sole variable ranged cycle delay pinned so the
    // total match length is exactly `len`. `lo` is the operand's minimum length
    // (lengthRange().first). A fixed operand has no such delay and is returned
    // as a plain clone (callers only request its single achievable length).
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* realize_At_ಉದ್ದಳತೆ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಕಾರ್ಯಾಂಶ, ಪೂರ್ಣಾಂಕ len, ಪೂರ್ಣಾಂಕ lo) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ clonep = ಕಾರ್ಯಾಂಶ->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        AstDelay* rangeDelayp = ಶೂನ್ಯಸೂಚಕ;
        clonep->foreach([&](AstDelay* dp) {
            ಆದರೆ (!rangeDelayp && dp->isRangeDelay() && !dp->isUnbounded()
                && getConstUInt(dp->lhsp()) != getConstUInt(dp->rhsp())) {
                rangeDelayp = dp;
            }
        });
        ಆದರೆ (rangeDelayp) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = rangeDelayp->fileline();
            ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ pinned = getConstUInt(rangeDelayp->lhsp()) + (len - lo);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ oldMinp = rangeDelayp->lhsp();
            oldMinp->replaceWith(ಹೊಸ AstConst{flp, pinned});
            VL_DO_DANGLING(oldMinp->delete_ವೃಕ್ಷ(), oldMinp);
            // Drop the max bound so it lowers as a fixed `##d`, not `##[d:d]`.
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ oldMaxp = rangeDelayp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            VL_DO_DANGLING(oldMaxp->delete_ವೃಕ್ಷ(), oldMaxp);
        }
        ಹಿಂತಿರುಗಿ clonep;
    }

    // Cuts AST size from O(N * sizeof(exprp)) to O(N) + O(sizeof(exprp)) by
    // sharing a single `VarRef` across N check edges. Hoist also matches the
    // IEEE 1800-2023 16.9.9 "single preponed-region snapshot" semantic for
    // any exprp -- even an impure one would now evaluate exactly once per
    // clock instead of N times. Orphan temps from failed builds are unused
    // MODULETEMPs and are removed by V3Dead.
    AstVar* tryHoistSampled(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, File_ಸಾಲು* flp, ಚಿಹ್ನೆರಹಿತ clone_ಎಣಿಕೆ) {
        constexpr ಚಿಹ್ನೆರಹಿತ k_Hoist_ಮಿತಿ = 2;
        ಆದರೆ (clone_ಎಣಿಕೆ < k_Hoist_ಮಿತಿ) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        AstVar* ಸ್ಥಿರಾಂಕ ತಾತ್ಕಾಲಿಕ_Varp
            = ಹೊಸ AstVar{flp, VVarType::MODULETEMP, m_prop_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು.get(exprp), exprp->dtypep()};
        m_modp->ಕೂಡಿಸು_Stmtsp(ತಾತ್ಕಾಲಿಕ_Varp);
        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, ತಾತ್ಕಾಲಿಕ_Varp, VAccess::WRITE},
                                                 sampled(exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು))};
        m_modp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, ಶೂನ್ಯಸೂಚಕ, assignp});
        ಹಿಂತಿರುಗಿ ತಾತ್ಕಾಲಿಕ_Varp;
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sampledRefOrClone(AstVar* hoistVarp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, File_ಸಾಲು* flp) {
        ಆದರೆ (hoistVarp) ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{flp, hoistVarp, VAccess::READ};
        ಹಿಂತಿರುಗಿ sampled(exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
    }

    // Create vertex and inherit temporal guards from the current scope.
    Sva_State_ಶೃಂಗ* scoped_Create_ಶೃಂಗ() {
        Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = m_ನಕ್ಷೆ.create_State_ಶೃಂಗ();
        ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ cp : m_temporal_Guard_ರಾಶಿ) {
            vtxp->m_throughoutConds.ತಳ್ಳು_ಹಿಂಭಾಗ(cp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
        }
        ಆದರೆ (m_ಒಳ_Unbounded_Scope) vtxp->m_isUnbounded = ನಿಜ;
        ಆದರೆ (m_markStrongPending) vtxp->m_strongPending = ನಿಜ;
        ಹಿಂತಿರುಗಿ vtxp;
    }

    // AND current temporal guards into every edge/link.
    Sva_Trans_ಅಂಚು* guardedLink(Sva_State_ಶೃಂಗ* fromp, Sva_State_ಶೃಂಗ* ತುದಿ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp,
                              File_ಸಾಲು* flp) {
        ಹಿಂತಿರುಗಿ m_ನಕ್ಷೆ.ಕೂಡಿಸು_Link(fromp, ತುದಿ, throughoutCond(condp, flp));
    }
    Sva_Trans_ಅಂಚು* guardedLink(Sva_State_ಶೃಂಗ* fromp, Sva_State_ಶೃಂಗ* ತುದಿ, File_ಸಾಲು* flp) {
        ಹಿಂತಿರುಗಿ m_ನಕ್ಷೆ.ಕೂಡಿಸು_Link(fromp, ತುದಿ, throughoutCond(ಶೂನ್ಯಸೂಚಕ, flp));
    }
    Sva_Trans_ಅಂಚು* guarded_ಅಂಚು(Sva_State_ಶೃಂಗ* fromp, Sva_State_ಶೃಂಗ* ತುದಿ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp,
                              File_ಸಾಲು* flp) {
        ಹಿಂತಿರುಗಿ m_ನಕ್ಷೆ.ಕೂಡಿಸು_Clocked_ಅಂಚು(fromp, ತುದಿ, throughoutCond(condp, flp));
    }
    Sva_Trans_ಅಂಚು* guarded_ಅಂಚು(Sva_State_ಶೃಂಗ* fromp, Sva_State_ಶೃಂಗ* ತುದಿ, File_ಸಾಲು* flp) {
        ಹಿಂತಿರುಗಿ m_ನಕ್ಷೆ.ಕೂಡಿಸು_Clocked_ಅಂಚು(fromp, ತುದಿ, throughoutCond(ಶೂನ್ಯಸೂಚಕ, flp));
    }

    Sva_State_ಶೃಂಗ* ಕೂಡಿಸು_Delay_Chain(Sva_State_ಶೃಂಗ* startp, ಚಿಹ್ನೆರಹಿತ ಗಾತ್ರ_2, File_ಸಾಲು* flp,
                                  ತಾರ್ಕಿಕ isFixed = ನಿಜ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* clearCondp = ಶೂನ್ಯಸೂಚಕ,
                                  Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* advanceCondp = ಶೂನ್ಯಸೂಚಕ) {
        ಆದರೆ (isFixed && ಗಾತ್ರ_2 == 0) ಹಿಂತಿರುಗಿ startp;
        UASSERT_OBJ(ಗಾತ್ರ_2 > 0, startp, "Delay chain needs at least one slot");
        ಆದರೆ (isFixed && ಗಾತ್ರ_2 == 1 && !advanceCondp) {
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ nextp = scoped_Create_ಶೃಂಗ();
            guarded_ಅಂಚು(startp, nextp, flp);
            ಹಿಂತಿರುಗಿ nextp;
        }
        Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ringVtxp = scoped_Create_ಶೃಂಗ();
        ringVtxp->m_isFixedDelayRing = isFixed;
        ringVtxp->m_delay_Ring_ಗಾತ್ರ = ಗಾತ್ರ_2;
        ಆದರೆ (clearCondp) {
            UASSERT_OBJ(!isFixed, startp, "Fixed delay cannot have a clear condition");
            ringVtxp->m_delayRingClearCondp = clearCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        }
        ringVtxp->m_delayRingAdvanceCondp = advanceCondp;
        ಆದರೆ (isFixed) {
            guarded_ಅಂಚು(startp, ringVtxp, flp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            guardedLink(startp, ringVtxp, flp);
        }
        ಹಿಂತಿರುಗಿ ringVtxp;
    }

    // Build NFA for an SExpr. finalCond = RHS (not yet added as a vertex).
    // isTopLevelStep: marks outermost required boolean check as rejectOnFail.
    // Apply a range delay `##[M:N]` to currentp. Returns true on success. On
    // failure, emits a diagnostic and returns false.
    ತಾರ್ಕಿಕ applyRangeDelay(AstSExpr* sexprp, Sva_State_ಶೃಂಗ*& currentp,
                         std::ಸದಿಶ<Sva_State_ಶೃಂಗ*>& midSources,
                         RangeDelayRejectInfo* rangeRejectInfop = ಶೂನ್ಯಸೂಚಕ) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = sexprp->fileline();
        AstDelay* ಸ್ಥಿರಾಂಕ delayp = VN_AS(sexprp->delayp(), Delay);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsExprp = sexprp->exprp();
        ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ minDelay = getConstUInt(delayp->lhsp());
        ಆದರೆ (delayp->isUnbounded()) {
            // `##[M:$]`: wait M cycles, then retain every matured attempt in a
            // one-slot ring while also exposing the first eligible tick.
            currentp = ಕೂಡಿಸು_Delay_Chain(currentp, minDelay, flp);
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ waitRingp = ಕೂಡಿಸು_Delay_Chain(currentp, 1, flp, ಸುಳ್ಳು);
            // The ring's live count is updated in NBA, so this link retains the previous
            // tick's live attempts alongside new arrivals for the next tick.
            guardedLink(waitRingp, waitRingp, flp);
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ವಿಲೀನ_Vtxp = scoped_Create_ಶೃಂಗ();
            guardedLink(currentp, ವಿಲೀನ_Vtxp, flp);
            guardedLink(waitRingp, ವಿಲೀನ_Vtxp, flp);
            currentp = ವಿಲೀನ_Vtxp;
            currentp->m_isUnbounded = ನಿಜ;  // Liveness, so no reject.
            m_ಒಳ_Unbounded_Scope = ನಿಜ;
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ maxDelay = getConstUInt(delayp->rhsp());
        ಆದರೆ (minDelay == maxDelay) {
            currentp = ಕೂಡಿಸು_Delay_Chain(currentp, minDelay, flp);
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ range = maxDelay - minDelay;
        currentp = ಕೂಡಿಸು_Delay_Chain(currentp, minDelay, flp);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ multi_ಚಕ್ರ_Rhs = rhsExprp->is_Multi_ಚಕ್ರ_Sva();
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ preExprp = sexprp->preExprp();
        // Prefix and nested-sequence composition can lose endpoints or their multiplicity
        // required by cover sequence (IEEE 1800-2023 16.14.3), regardless of the range width.
        ಆದರೆ (m_isCoverSeq
            && (multi_ಚಕ್ರ_Rhs || m_ಒಳ_Sequence_ಪೂರ್ವಪ್ರತ್ಯಯ
                || (preExprp && preExprp->is_Multi_ಚಕ್ರ_Sva()))) {
            warnEndpointUnsupported(flp, "this ranged cycle delay");
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಆದರೆ (m_ಒಳ_Sequence_ಪೂರ್ವಪ್ರತ್ಯಯ) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: Bounded ranged cycle delay in a sequence prefix");
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಆದರೆ (multi_ಚಕ್ರ_Rhs) {
            // Launch a candidate at every eligible tick. Candidate-local misses are not
            // assertion rejects while a later position can still match.
            ಆದರೆ (rangeRejectInfop) {
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ rhsLen = fixed_ಉದ್ದಳತೆ(rhsExprp);
                ಆದರೆ (rhsLen >= 0) *rangeRejectInfop = {currentp, range, rhsLen};
            }
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ tailp = ಕೂಡಿಸು_Delay_Chain(currentp, range + 1U, flp, ಸುಳ್ಳು);
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ವಿಲೀನ_Vtxp = scoped_Create_ಶೃಂಗ();
            ವಿಲೀನ_Vtxp->m_isUnbounded = ನಿಜ;
            guardedLink(currentp, ವಿಲೀನ_Vtxp, flp);
            guardedLink(tailp, ವಿಲೀನ_Vtxp, flp);
            currentp = ವಿಲೀನ_Vtxp;
            m_ಒಳ_Unbounded_Scope = ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            // The first eligible tick is explicit. The ring retains attempts for the
            // remaining ticks and exposes the final tick for rejection. Only properties
            // clear on success, since cover sequence counts every endpoint.
            midSources.ತಳ್ಳು_ಹಿಂಭಾಗ(currentp);
            currentp = ಕೂಡಿಸು_Delay_Chain(currentp, range + 1U, flp, ಸುಳ್ಳು,
                                     m_isCoverSeq ? ಶೂನ್ಯಸೂಚಕ : rhsExprp);
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_Finite_Range_Reject(ಸ್ಥಿರಾಂಕ RangeDelayRejectInfo& info, ಸ್ಥಿರಾಂಕ Build_ಫಲಿತಾಂಶ& ಫಲಿತಾಂಶ,
                              File_ಸಾಲು* flp) {
        ಆದರೆ (!info.startp) ಹಿಂತಿರುಗಿ;

        Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ expiryVtxp
            = ಕೂಡಿಸು_Delay_Chain(info.startp, info.range + info.rhsLen, flp);
        Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ expiryMatchp = scoped_Create_ಶೃಂಗ();
        std::ಸದಿಶ<Sva_State_ಶೃಂಗ*> sources = ಫಲಿತಾಂಶ.midSources;
        sources.ತಳ್ಳು_ಹಿಂಭಾಗ(ಫಲಿತಾಂಶ.ಪದ_Vertexp);
        ಗಾಗಿ (Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp : sources) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp
                = ಫಲಿತಾಂಶ.finalCondp ? sampled(ಫಲಿತಾಂಶ.finalCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)) : ಶೂನ್ಯಸೂಚಕ;
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ successNowp = scoped_Create_ಶೃಂಗ();
            guardedLink(srcp, successNowp, condp, flp);
            // Retain a success through the latest deadline it can satisfy.
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ historyp
                = ಕೂಡಿಸು_Delay_Chain(successNowp, info.range + 1U, flp, ಸುಳ್ಳು);
            guardedLink(successNowp, expiryMatchp, flp);
            guardedLink(historyp, expiryMatchp, flp);
        }

        Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ sinkVtxp = m_ನಕ್ಷೆ.create_State_ಶೃಂಗ();
        sinkVtxp->m_isRejectSink = ನಿಜ;
        Sva_Trans_ಅಂಚು* ಸ್ಥಿರಾಂಕ rejectp = m_ನಕ್ಷೆ.ಕೂಡಿಸು_Link(expiryVtxp, sinkVtxp);
        rejectp->m_rejectOnFail = ನಿಜ;
        rejectp->m_condVtxp = expiryMatchp;
    }

    Build_ಫಲಿತಾಂಶ buildSExpr(AstSExpr* sexprp, Sva_State_ಶೃಂಗ* ನಮೂದು_Vtxp,
                           ತಾರ್ಕಿಕ is_ತುದಿ_Level_ಹೆಜ್ಜೆ = ಸುಳ್ಳು) {
        AstDelay* ಸ್ಥಿರಾಂಕ delayp = VN_CAST(sexprp->delayp(), Delay);
        ಆದರೆ (!delayp || !delayp->is_ಚಕ್ರ_Delay()) ಹಿಂತಿರುಗಿ Build_ಫಲಿತಾಂಶ::fail();

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = sexprp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = sexprp->exprp();

        // Handle LHS (preExpr)
        Sva_State_ಶೃಂಗ* currentp = ನಮೂದು_Vtxp;
        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ preExprp = sexprp->preExprp()) {
            VL_RESTORER(m_ಒಳ_Sequence_ಪೂರ್ವಪ್ರತ್ಯಯ);
            m_ಒಳ_Sequence_ಪೂರ್ವಪ್ರತ್ಯಯ = ನಿಜ;
            ಸ್ಥಿರಾಂಕ Build_ಫಲಿತಾಂಶ pre = build_ಅಭಿವ್ಯಕ್ತಿ(preExprp, currentp, is_ತುದಿ_Level_ಹೆಜ್ಜೆ);
            ಆದರೆ (!pre.valid()) ಹಿಂತಿರುಗಿ Build_ಫಲಿತಾಂಶ::fail(pre.errorEmitted);  // LCOV_EXCL_LINE
            ಆದರೆ (pre.finalCondp) {
                Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ condVtxp = scoped_Create_ಶೃಂಗ();
                Sva_Trans_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep = guardedLink(
                    pre.ಪದ_Vertexp, condVtxp, sampled(pre.finalCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)), flp);
                ಆದರೆ (is_ತುದಿ_Level_ಹೆಜ್ಜೆ && !pre.ಪದ_Vertexp->m_isUnbounded && !m_ಒಳ_Unbounded_Scope) {
                    // Do not mark liveness sources: first boolean check is deferred.
                    edgep->m_rejectOnFail = ನಿಜ;
                }
                freeUnlinkedCondp(pre.finalCondp);
                currentp = condVtxp;
            } ಇಲ್ಲದಿದ್ದರೆ {
                currentp = pre.ಪದ_Vertexp;
            }
        }

        // Handle delay
        std::ಸದಿಶ<Sva_State_ಶೃಂಗ*> rangeMidSources;
        RangeDelayRejectInfo rangeRejectInfo;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಕೂಡಿಸು_Range_Reject = is_ತುದಿ_Level_ಹೆಜ್ಜೆ && !m_ಒಳ_Unbounded_Scope;
        ಆದರೆ (delayp->isRangeDelay()) {
            ಆದರೆ (!applyRangeDelay(sexprp, currentp, rangeMidSources,
                                 ಕೂಡಿಸು_Range_Reject ? &rangeRejectInfo : ಶೂನ್ಯಸೂಚಕ)) {
                ಹಿಂತಿರುಗಿ Build_ಫಲಿತಾಂಶ::failWithError();
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ delay_ಚಕ್ರಗಳು = getConstUInt(delayp->lhsp());
            currentp = ಕೂಡಿಸು_Delay_Chain(currentp, delay_ಚಕ್ರಗಳು, flp);
        }

        // Multi-cycle RHS: recurse (only plain boolean is returned as finalCondp).
        ಆದರೆ (exprp->is_Multi_ಚಕ್ರ_Sva()) {
            ಸ್ಥಿರಾಂಕ Build_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = build_ಅಭಿವ್ಯಕ್ತಿ(exprp, currentp, is_ತುದಿ_Level_ಹೆಜ್ಜೆ);
            ಆದರೆ (ಫಲಿತಾಂಶ.valid()) ಕೂಡಿಸು_Finite_Range_Reject(rangeRejectInfo, ಫಲಿತಾಂಶ, flp);
            ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
        }
        ಹಿಂತಿರುಗಿ {currentp, exprp, std::move(rangeMidSources)};
    }

    Build_ಫಲಿತಾಂಶ buildConsRep(AstSConsRep* repp, Sva_State_ಶೃಂಗ* ನಮೂದು_Vtxp,
                             ತಾರ್ಕಿಕ is_ತುದಿ_Level_ಹೆಜ್ಜೆ = ಸುಳ್ಳು) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = repp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = repp->exprp();
        // Multi-cycle expr in ConsRep not yet supported; bail to avoid invalid AST.
        ಆದರೆ (exprp->is_Multi_ಚಕ್ರ_Sva()) {
            repp->v3warn(E_UNSUPPORTED, "Unsupported: multi-cycle sequence expression inside"
                                        " consecutive repetition (IEEE 1800-2023 16.9.2)");
            ಹಿಂತಿರುಗಿ Build_ಫಲಿತಾಂಶ::failWithError();
        }
        ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ minN = getConstUInt(repp->countp());

        // Sum sites across prefix + unbounded/range tail so one hoist covers
        // every check edge of this repetition.
        ಚಿಹ್ನೆರಹಿತ ಒಟ್ಟು_Sites = minN;
        ಆದರೆ (repp->unbounded()) {
            ಒಟ್ಟು_Sites += 1;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (repp->maxCountp()) {
            ಒಟ್ಟು_Sites += getConstUInt(repp->maxCountp()) - minN;
        }
        AstVar* ಸ್ಥಿರಾಂಕ hoistVarp = tryHoistSampled(exprp, flp, ಒಟ್ಟು_Sites);

        // Cover-sequence (IEEE 1800-2023 16.14.3): collect each end-of-match
        // position so they all fire the action, not just the merged terminal.
        std::ಸದಿಶ<Sva_State_ಶೃಂಗ*> consMidSources;

        Sva_State_ಶೃಂಗ* currentp = ನಮೂದು_Vtxp;
        ಗಾಗಿ (ಚಿಹ್ನೆರಹಿತ i = 0; i < minN; ++i) {
            // Keep the first repetition explicit, collapse all remaining checks into the ring.
            ಆದರೆ (i == 1) {
                currentp = ಕೂಡಿಸು_Delay_Chain(currentp, minN - 1, flp);
                currentp->m_delayRingClearCondp
                    = ಹೊಸ AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                currentp->m_replayAbortReject = ನಿಜ;
                ಆದರೆ (is_ತುದಿ_Level_ಹೆಜ್ಜೆ) {
                    currentp->m_throughoutConds.ತಳ್ಳು_ಹಿಂಭಾಗ(
                        sampledRefOrClone(hoistVarp, exprp, flp));
                }
                i = minN - 1;
            }
            // Every repetition in the minimum prefix is required.
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ condVtxp = scoped_Create_ಶೃಂಗ();
            Sva_Trans_ಅಂಚು* ಸ್ಥಿರಾಂಕ linkp
                = guardedLink(currentp, condVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            // Only an outermost required step rejects the property. Its first check is explicit,
            // later required checks reject through the ring's throughout guard.
            ಆದರೆ (is_ತುದಿ_Level_ಹೆಜ್ಜೆ && i == 0) linkp->m_rejectOnFail = ನಿಜ;
            currentp = condVtxp;
        }
        // After minN: currentp is the first valid end-of-match position for [*m:n].
        ಆದರೆ (m_isCoverSeq && (repp->unbounded() || repp->maxCountp())) {
            consMidSources.ತಳ್ಳು_ಹಿಂಭಾಗ(currentp);
        }

        ಆದರೆ (repp->unbounded()) {
            ಆದರೆ (minN == 0) {
                Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ waitVtxp = scoped_Create_ಶೃಂಗ();
                guarded_ಅಂಚು(currentp, waitVtxp, flp);
                Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ checkVtxp = scoped_Create_ಶೃಂಗ();
                guardedLink(waitVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guarded_ಅಂಚು(checkVtxp, waitVtxp, flp);
                guardedLink(currentp, checkVtxp, flp);
                currentp = checkVtxp;
            } ಇಲ್ಲದಿದ್ದರೆ {
                Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ loop_ಹಿಂಭಾಗ_Vtxp = scoped_Create_ಶೃಂಗ();
                guarded_ಅಂಚು(currentp, loop_ಹಿಂಭಾಗ_Vtxp, flp);
                Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ reCheckVtxp = scoped_Create_ಶೃಂಗ();
                guardedLink(loop_ಹಿಂಭಾಗ_Vtxp, reCheckVtxp, sampledRefOrClone(hoistVarp, exprp, flp),
                            flp);
                guarded_ಅಂಚು(reCheckVtxp, loop_ಹಿಂಭಾಗ_Vtxp, flp);
                guardedLink(reCheckVtxp, currentp, flp);
            }
            currentp->m_isUnbounded = ನಿಜ;
            m_ಒಳ_Unbounded_Scope = ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (repp->maxCountp()) {
            ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ maxN = getConstUInt(repp->maxCountp());
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ವಿಲೀನ_Vtxp = scoped_Create_ಶೃಂಗ();
            guardedLink(currentp, ವಿಲೀನ_Vtxp, flp);
            ಚಿಹ್ನೆರಹಿತ ಬಾಲ_Min_N = minN;
            Sva_State_ಶೃಂಗ* ಬಾಲ_Startp = currentp;
            ಆದರೆ (minN == 0) {
                // Build the first optional iteration explicitly. Feeding the empty endpoint
                // directly into a range ring would incorrectly keep that match alive.
                Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ಮುಂದಿನ_Vtxp = scoped_Create_ಶೃಂಗ();
                guarded_ಅಂಚು(currentp, ಮುಂದಿನ_Vtxp, flp);
                Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ checkVtxp = scoped_Create_ಶೃಂಗ();
                guardedLink(ಮುಂದಿನ_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, ವಿಲೀನ_Vtxp, flp);
                ಆದರೆ (m_isCoverSeq) consMidSources.ತಳ್ಳು_ಹಿಂಭಾಗ(checkVtxp);
                ಬಾಲ_Startp = checkVtxp;
                ಬಾಲ_Min_N = 1;
            }
            ಆದರೆ (maxN > ಬಾಲ_Min_N) {
                // Add tail-ring only if the tail is non-empty.
                Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ಮುಂದಿನ_Vtxp
                    = ಕೂಡಿಸು_Delay_Chain(ಬಾಲ_Startp, maxN - ಬಾಲ_Min_N + 1, flp, ಸುಳ್ಳು);
                ಮುಂದಿನ_Vtxp->m_delayRingClearCondp
                    = ಹೊಸ AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                ಮುಂದಿನ_Vtxp->m_replayAbortReject = ನಿಜ;
                Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ checkVtxp = scoped_Create_ಶೃಂಗ();
                guardedLink(ಮುಂದಿನ_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, ವಿಲೀನ_Vtxp, flp);
                ಆದರೆ (m_isCoverSeq) consMidSources.ತಳ್ಳು_ಹಿಂಭಾಗ(checkVtxp);
            }
            currentp = ವಿಲೀನ_Vtxp;
        }
        // finalCond = nullptr (already checked via Links)
        Build_ಫಲಿತಾಂಶ res;
        res.ಪದ_Vertexp = currentp;
        res.finalCondp = ಶೂನ್ಯಸೂಚಕ;
        res.midSources = std::move(consMidSources);
        // mergeVtxp is the OR of all the end-positions we already pushed to
        // midSources, so the main termVtxp -> matchVertex Link would duplicate.
        res.ಪದ_Is_Mid_ವಿಲೀನ = m_isCoverSeq && !res.midSources.ಖಾಲಿಗಳು();
        ಹಿಂತಿರುಗಿ res;
    }

    // always[lo:hi] / s_always[lo:hi] (IEEE 1800-2023 16.12.11).
    Build_ಫಲಿತಾಂಶ buildPropAlways(AstPropAlways* nodep, Sva_State_ಶೃಂಗ* ನಮೂದು_Vtxp,
                                ತಾರ್ಕಿಕ is_ತುದಿ_Level_ಹೆಜ್ಜೆ = ಸುಳ್ಳು) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ propp = nodep->propp();
        ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ lo = getConstUInt(nodep->loBoundp());
        ಆದರೆ (VN_IS(nodep->hiBoundp(), Unbounded)) {
            // Weak always [lo:$]: unbounded upper bound (IEEE 1800-2023 16.12.11).
            // p must hold at every clock tick at least lo cycles after the attempt
            // start; those ticks are not required to exist, so there is no
            // end-of-trace obligation (weak). The self-loop keeps the attempt live
            // every cycle; each observed cycle is a safety obligation, so a false p
            // rejects immediately.
            UASSERT_OBJ(!nodep->isStrong(), nodep, "Unbounded always must be weak (V3Width)");
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ livep = ಕೂಡಿಸು_Delay_Chain(ನಮೂದು_Vtxp, lo, flp);
            livep->m_isUnbounded = ನಿಜ;
            guarded_ಅಂಚು(livep, livep, flp);  // stay active every subsequent cycle
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ sinkp = m_ನಕ್ಷೆ.create_State_ಶೃಂಗ();
            sinkp->m_isRejectSink = ನಿಜ;
            Sva_Trans_ಅಂಚು* ಸ್ಥಿರಾಂಕ rejEdgep
                = guardedLink(livep, sinkp, sampled(propp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)), flp);
            ಆದರೆ (is_ತುದಿ_Level_ಹೆಜ್ಜೆ) rejEdgep->m_rejectOnFail = ನಿಜ;
            ಹಿಂತಿರುಗಿ {livep, ಶೂನ್ಯಸೂಚಕ, {}};
        }
        ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ hi = getConstUInt(nodep->hiBoundp());
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
        Sva_State_ಶೃಂಗ* currentp = ಕೂಡಿಸು_Delay_Chain(ನಮೂದು_Vtxp, lo, flp);
        Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ checkp = scoped_Create_ಶೃಂಗ();
        Sva_Trans_ಅಂಚು* ಸ್ಥಿರಾಂಕ linkp
            = guardedLink(currentp, checkp, sampled(propp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)), flp);
        ಆದರೆ (is_ತುದಿ_Level_ಹೆಜ್ಜೆ) linkp->m_rejectOnFail = ನಿಜ;
        currentp = checkp;
        m_temporal_Guard_ರಾಶಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(propp);
        currentp = ಕೂಡಿಸು_Delay_Chain(currentp, hi - lo, flp);
        ಆದರೆ (nodep->isStrong() && currentp->m_delay_Ring_ಗಾತ್ರ) currentp->m_strongAlwaysRing = ನಿಜ;
        m_temporal_Guard_ರಾಶಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
        ಹಿಂತಿರುಗಿ {currentp, propp, {}};
    }

    Build_ಫಲಿತಾಂಶ buildGotoRep(AstSGotoRep* repp, Sva_State_ಶೃಂಗ* ನಮೂದು_Vtxp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = repp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = repp->exprp();
        ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ minN = getConstUInt(repp->countp());
        ಆದರೆ (minN == 0) ಹಿಂತಿರುಗಿ Build_ಫಲಿತಾಂಶ::fail();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ hasMax = repp->maxCountp() != ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ maxN = hasMax ? getConstUInt(repp->maxCountp()) : minN;
        ಆದರೆ (m_isCoverSeq) {
            // Several attempts may coalesce in the one-bit wait vertex before reaching the ring,
            // so a cover sequence action block could run too few times.
            warnEndpointUnsupported(flp, "a goto repetition");
            ಹಿಂತಿರುಗಿ Build_ಫಲಿತಾಂಶ::failWithError();
        }

        AstVar* ಸ್ಥಿರಾಂಕ hoistVarp = tryHoistSampled(exprp, flp, 2);

        // The first guardedEdge is the ##1 before waiting for a match. In the wait state, false
        // takes the clocked self-loop, while true takes the zero-delay guardedLink on that tick.
        Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ waitVtxp = scoped_Create_ಶೃಂಗ();
        guarded_ಅಂಚು(ನಮೂದು_Vtxp, waitVtxp, flp);
        guarded_ಅಂಚು(waitVtxp, waitVtxp,
                    ಹೊಸ AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)}, flp);
        Sva_State_ಶೃಂಗ* currentp = scoped_Create_ಶೃಂಗ();
        guardedLink(waitVtxp, currentp, sampledRefOrClone(hoistVarp, exprp, flp), flp);

        ಆದರೆ (minN > 1) {
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ringVtxp = ಕೂಡಿಸು_Delay_Chain(
                currentp, minN - 1, flp, ನಿಜ, ಶೂನ್ಯಸೂಚಕ, sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = ನಿಜ;
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ checkVtxp = scoped_Create_ಶೃಂಗ();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            currentp = checkVtxp;
        }
        ಆದರೆ (!hasMax) {
            currentp->m_isUnbounded = ನಿಜ;  // [->N] waits unboundedly
            m_ಒಳ_Unbounded_Scope = ನಿಜ;
            ಹಿಂತಿರುಗಿ {currentp, ಶೂನ್ಯಸೂಚಕ, {}};
        }

        // [->M:N]: the range ring holds matches from M through N and advances
        // only on expr, preserving arbitrarily long gaps between occurrences.
        Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ವಿಲೀನ_Vtxp = scoped_Create_ಶೃಂಗ();
        guardedLink(currentp, ವಿಲೀನ_Vtxp, flp);  // accept at match_M
        ಆದರೆ (maxN > minN) {
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ringVtxp
                = ಕೂಡಿಸು_Delay_Chain(currentp, maxN - minN + 1, flp, ಸುಳ್ಳು, ಶೂನ್ಯಸೂಚಕ,
                                sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = ನಿಜ;
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ checkVtxp = scoped_Create_ಶೃಂಗ();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            guardedLink(checkVtxp, ವಿಲೀನ_Vtxp, flp);
        }
        ವಿಲೀನ_Vtxp->m_isUnbounded = ನಿಜ;  // [->M:N] still has unbounded waits between matches
        m_ಒಳ_Unbounded_Scope = ನಿಜ;
        ಹಿಂತಿರುಗಿ {ವಿಲೀನ_Vtxp, ಶೂನ್ಯಸೂಚಕ, {}};
    }

    // True if the sequence has a zero-minimum consecutive repetition, which may match empty
    ಸ್ಥಿರ ತಾರ್ಕಿಕ hasZeroMinRep(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* seqp) {
        ಹಿಂತಿರುಗಿ seqp->exists(
            [](ಸ್ಥಿರಾಂಕ AstSConsRep* repp) { ಹಿಂತಿರುಗಿ getConstUInt(repp->countp()) == 0; });
    }

    // True if a leaf's condition has no implication or multi-cycle sequence in it
    // (a leaf can be e.g. 'not (x |-> y)')
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isPlainBoolean(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        ಹಿಂತಿರುಗಿ !condp->exists(
            [](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* np) { ಹಿಂತಿರುಗಿ np->is_Multi_ಚಕ್ರ_Sva() || VN_IS(np, Implication); });
    }

    // Build merge vertex for SOr / LogOr: both branches feed into one vertex.
    Build_ಫಲಿತಾಂಶ build_Or_ವಿಲೀನ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, Sva_State_ಶೃಂಗ* ನಮೂದು_Vtxp,
                             File_ಸಾಲು* flp, ತಾರ್ಕಿಕ want_Bool_ಎಲೆ) {
        ಸ್ಥಿರಾಂಕ Build_ಫಲಿತಾಂಶ lhs = build_ಅಭಿವ್ಯಕ್ತಿ(lhsp, ನಮೂದು_Vtxp);
        ಸ್ಥಿರಾಂಕ Build_ಫಲಿತಾಂಶ rhs = build_ಅಭಿವ್ಯಕ್ತಿ(rhsp, ನಮೂದು_Vtxp);
        ಆದರೆ (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ಹಿಂತಿರುಗಿ Build_ಫಲಿತಾಂಶ::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP
        // IEEE 1800-2023 16.14.3: a cover sequence counts every end-of-match. A
        // sequence operand of 'or' can end more than once, but only its final
        // end reaches the merge vertex below, so reject sequence operands rather
        // than under-count. Plain boolean disjunction has one end per cycle and
        // is handled by the OR-fold.
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ booleanOnly = lhs.ಪದ_Vertexp == ನಮೂದು_Vtxp && lhs.finalCondp
                                 && rhs.ಪದ_Vertexp == ನಮೂದು_Vtxp && rhs.finalCondp;
        ಆದರೆ (m_isCoverSeq && !booleanOnly) {
            warnEndpointUnsupported(flp, "a sequence operand of 'or'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ಹಿಂತಿರುಗಿ Build_ಫಲಿತಾಂಶ::failWithError();
        }
        ಆದರೆ (booleanOnly && want_Bool_ಎಲೆ) {
            ಆದರೆ (isPlainBoolean(lhs.finalCondp) && isPlainBoolean(rhs.finalCondp)) {
                // As a |-> / |=> or top-level body, a disjunction of two plain booleans must
                // be able to reject: return it as a boolean finalCondp. The merge vertex
                // below drops the attempt silently when both operands are false.
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ orCondp
                    = ಹೊಸ AstLogOr{flp, lhs.finalCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                                   rhs.finalCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                freeUnlinkedCondp(lhs.finalCondp);
                freeUnlinkedCondp(rhs.finalCondp);
                ಹಿಂತಿರುಗಿ {ನಮೂದು_Vtxp, orCondp, {}};
            }
        }
        Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ವಿಲೀನ_Vtxp = scoped_Create_ಶೃಂಗ();
        ಆದರೆ (booleanOnly) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsp,
                        "Single-cycle SOr operands must have finalCondp");
            guardedLink(ನಮೂದು_Vtxp, ವಿಲೀನ_Vtxp,
                        ಹೊಸ AstLogOr{flp, sampled(lhs.finalCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)),
                                     sampled(rhs.finalCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು))},
                        flp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (lhs.finalCondp) {
                guardedLink(lhs.ಪದ_Vertexp, ವಿಲೀನ_Vtxp,
                            sampled(lhs.finalCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)), flp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                guardedLink(lhs.ಪದ_Vertexp, ವಿಲೀನ_Vtxp, flp);
            }
            ಆದರೆ (rhs.finalCondp) {
                guardedLink(rhs.ಪದ_Vertexp, ವಿಲೀನ_Vtxp,
                            sampled(rhs.finalCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)), flp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                guardedLink(rhs.ಪದ_Vertexp, ವಿಲೀನ_Vtxp, flp);
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        ಹಿಂತಿರುಗಿ {ವಿಲೀನ_Vtxp, ಶೂನ್ಯಸೂಚಕ, {}};
    }

    // Build done-latch combiner for SAnd/SIntersect (IEEE 1800-2023 16.9.5).
    Build_ಫಲಿತಾಂಶ buildAndCombiner(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsExprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsExprp,
                                 Sva_State_ಶೃಂಗ* ನಮೂದು_Vtxp, File_ಸಾಲು* flp) {
        // Snapshot-restore scope so LHS liveness does not leak into RHS.
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ savedScope = m_ಒಳ_Unbounded_Scope;
        ಸ್ಥಿರಾಂಕ Build_ಫಲಿತಾಂಶ lhs = build_ಅಭಿವ್ಯಕ್ತಿ(lhsExprp, ನಮೂದು_Vtxp);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ lhsScope = m_ಒಳ_Unbounded_Scope;
        m_ಒಳ_Unbounded_Scope = savedScope;
        ಸ್ಥಿರಾಂಕ Build_ಫಲಿತಾಂಶ rhs = build_ಅಭಿವ್ಯಕ್ತಿ(rhsExprp, ನಮೂದು_Vtxp);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ rhsScope = m_ಒಳ_Unbounded_Scope;
        m_ಒಳ_Unbounded_Scope = savedScope || lhsScope || rhsScope;
        ಆದರೆ (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ಹಿಂತಿರುಗಿ Build_ಫಲಿತಾಂಶ::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP

        // Single-cycle operands: use boolean AND (done-latch would fire across cycles).
        // If both operands stayed at entry, they must be boolean leaves which
        // buildExpr returns with finalCondp=nodep (non-null).
        ಆದರೆ (lhs.ಪದ_Vertexp == ನಮೂದು_Vtxp && rhs.ಪದ_Vertexp == ನಮೂದು_Vtxp) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsExprp,
                        "Single-cycle SAnd operands must have finalCondp");
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = ಹೊಸ AstLogAnd{flp, lhs.finalCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                                                     rhs.finalCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ಹಿಂತಿರುಗಿ {ನಮೂದು_Vtxp, condp, {}};
        }
        // Range-delay mid-window sources in either sub-branch would need
        // to be folded into the latch's match-now signal, which the
        // current combiner does not support.
        ಆದರೆ (!lhs.midSources.ಖಾಲಿಗಳು() || !rhs.midSources.ಖಾಲಿಗಳು()) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: ranged cycle delay in an operand of property 'and'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ಹಿಂತಿರುಗಿ Build_ಫಲಿತಾಂಶ::failWithError();
        }
        Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ combVtxp = scoped_Create_ಶೃಂಗ();
        combVtxp->m_isAndCombiner = ನಿಜ;
        combVtxp->m_andLhsTermp = lhs.ಪದ_Vertexp;
        combVtxp->m_andRhsTermp = rhs.ಪದ_Vertexp;
        ಆದರೆ (lhs.finalCondp) combVtxp->m_andLhsCondp = lhs.finalCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        ಆದರೆ (rhs.finalCondp) combVtxp->m_andRhsCondp = rhs.finalCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        ಆದರೆ (lhs.ಪದ_Vertexp->m_isUnbounded || rhs.ಪದ_Vertexp->m_isUnbounded) {
            combVtxp->m_isUnbounded = ನಿಜ;
        }
        // Wire terminal-boolean rejects to a dedicated sink so each side can fail
        // the AND independently. Skip for liveness or single-cycle operands
        // (single-cycle termVertexp == entryVtxp would fire every cycle).
        ಆದರೆ (!combVtxp->m_isUnbounded) {
            ತಾರ್ಕಿಕ needSink = ಸುಳ್ಳು;
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ lhs_Multi_ಚಕ್ರ = (lhs.ಪದ_Vertexp != ನಮೂದು_Vtxp);
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ rhs_Multi_ಚಕ್ರ = (rhs.ಪದ_Vertexp != ನಮೂದು_Vtxp);
            ಆದರೆ (lhs.finalCondp && lhs_Multi_ಚಕ್ರ && !lhs.ಪದ_Vertexp->m_isUnbounded) {
                needSink = ನಿಜ;
            }
            ಆದರೆ (rhs.finalCondp && rhs_Multi_ಚಕ್ರ && !rhs.ಪದ_Vertexp->m_isUnbounded) {
                needSink = ನಿಜ;
            }
            ಆದರೆ (needSink) {
                Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ sinkVtxp = m_ನಕ್ಷೆ.create_State_ಶೃಂಗ();
                sinkVtxp->m_isRejectSink = ನಿಜ;
                ಆದರೆ (lhs.finalCondp && lhs_Multi_ಚಕ್ರ && !lhs.ಪದ_Vertexp->m_isUnbounded) {
                    Sva_Trans_ಅಂಚು* ಸ್ಥಿರಾಂಕ ep = m_ನಕ್ಷೆ.ಕೂಡಿಸು_Link(
                        lhs.ಪದ_Vertexp, sinkVtxp, sampled(lhs.finalCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)));
                    ep->m_rejectOnFail = ನಿಜ;
                }
                ಆದರೆ (rhs.finalCondp && rhs_Multi_ಚಕ್ರ && !rhs.ಪದ_Vertexp->m_isUnbounded) {
                    Sva_Trans_ಅಂಚು* ಸ್ಥಿರಾಂಕ ep = m_ನಕ್ಷೆ.ಕೂಡಿಸು_Link(
                        rhs.ಪದ_Vertexp, sinkVtxp, sampled(rhs.finalCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)));
                    ep->m_rejectOnFail = ನಿಜ;
                }
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        ಹಿಂತಿರುಗಿ {combVtxp, ಶೂನ್ಯಸೂಚಕ, {}};
    }

    // Lower `seq1 within seq2` (IEEE 1800-2023 16.9.10) as:
    //   (OR_{i in 0..slack} 1 ##i seq1 ##(slack-i) 1) intersect seq2
    // Both operands must have fixed length (no ranged cycle delays).
    // The OR lives inside a single SIntersect so one AndCombiner done-latch
    // serves all offsets; lifting it out would double-count matches where
    // multiple offsets accept on the same seq2 end cycle.
    Build_ಫಲಿತಾಂಶ buildSWithin(AstSWithin* nodep, Sva_State_ಶೃಂಗ* ನಮೂದು_Vtxp,
                             ತಾರ್ಕಿಕ is_ತುದಿ_Level_ಹೆಜ್ಜೆ = ಸುಳ್ಳು) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ innerLen = fixed_ಉದ್ದಳತೆ(nodep->lhsp());
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ outerLen = fixed_ಉದ್ದಳತೆ(nodep->rhsp());
        ಆದರೆ (innerLen < 0 || outerLen < 0) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: within with ranged cycle-delay operand");
            ಹಿಂತಿರುಗಿ Build_ಫಲಿತಾಂಶ::failWithError();
        }
        ಆದರೆ (innerLen > outerLen) {
            ಹಿಂತಿರುಗಿ build_Never_ಹೊಂದಿಕೆ_Intersect(
                nodep, ನಮೂದು_Vtxp, is_ತುದಿ_Level_ಹೆಜ್ಜೆ,
                "the inner sequence is longer than the outer sequence");
        }
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ slack = outerLen - innerLen;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* innerOrp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i <= slack; ++i) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ postPad = slack - i;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* branchp = nodep->lhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            ಆದರೆ (i > 0) {
                AstConst* ಸ್ಥಿರಾಂಕ prePadp = ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}};
                AstDelay* ಸ್ಥಿರಾಂಕ delayp
                    = ಹೊಸ AstDelay{flp, ಹೊಸ AstConst{flp, static_cast<uint32_t>(i)}, ನಿಜ};
                AstSExpr* ಸ್ಥಿರಾಂಕ wrapped = ಹೊಸ AstSExpr{flp, prePadp, delayp, branchp};
                wrapped->dtype_ಗಣ_ಬಿಟ್();
                branchp = wrapped;
            }
            ಆದರೆ (postPad > 0) {
                AstConst* ಸ್ಥಿರಾಂಕ postTruep = ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}};
                AstDelay* ಸ್ಥಿರಾಂಕ delayp
                    = ಹೊಸ AstDelay{flp, ಹೊಸ AstConst{flp, static_cast<uint32_t>(postPad)}, ನಿಜ};
                AstSExpr* ಸ್ಥಿರಾಂಕ wrapped = ಹೊಸ AstSExpr{flp, branchp, delayp, postTruep};
                wrapped->dtype_ಗಣ_ಬಿಟ್();
                branchp = wrapped;
            }
            innerOrp = innerOrp ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstSOr{flp, innerOrp, branchp})
                                : branchp;
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ outerClonep = nodep->rhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ combinedp = ಹೊಸ AstSIntersect{flp, innerOrp, outerClonep};
        Build_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = build_ಅಭಿವ್ಯಕ್ತಿ(combinedp, ನಮೂದು_Vtxp, is_ತುದಿ_Level_ಹೆಜ್ಜೆ);
        VL_DO_DANGLING(combinedp->delete_ವೃಕ್ಷ(), combinedp);
        // When both operands are plain booleans, buildAndCombiner returns a
        // freshly-allocated AstAnd as finalCondp with no parent. Callers up
        // the chain clone-and-discard finalCondp, so leave it parent-less and
        // it leaks; anchor it in the graph now via an edge.
        ಆದರೆ (ಫಲಿತಾಂಶ.valid() && ಫಲಿತಾಂಶ.finalCondp && !ಫಲಿತಾಂಶ.finalCondp->backp()) {
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ wrapVtxp = scoped_Create_ಶೃಂಗ();
            guardedLink(ಫಲಿತಾಂಶ.ಪದ_Vertexp, wrapVtxp, sampled(ಫಲಿತಾಂಶ.finalCondp), flp);
            ಫಲಿತಾಂಶ = {wrapVtxp, ಶೂನ್ಯಸೂಚಕ, ಫಲಿತಾಂಶ.midSources};
        }
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }

    // Collect the boolean leaf checks of a fixed-length sequence keyed by their
    // clock offset from the start. Returns false for anything other than nested
    // AstSExpr with fixed cycle delays over boolean leaves (e.g. throughout).
    ಸ್ಥಿರ ತಾರ್ಕಿಕ flattenFixedSeq(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಪೂರ್ಣಾಂಕ ತಳ_ಸ್ಥಾನಾಂತರ,
                                std::map<ಪೂರ್ಣಾಂಕ, std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>>& ಹೊರಹೋಗು) {
        ಆದರೆ (AstSExpr* ಸ್ಥಿರಾಂಕ sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ಸ್ಥಿರಾಂಕ delayp = VN_CAST(sexprp->delayp(), Delay);
            ಆದರೆ (!delayp || !delayp->is_ಚಕ್ರ_Delay() || delayp->isUnbounded()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ delay_ಚಕ್ರಗಳು = getConstUInt(delayp->lhsp());
            ಆದರೆ (delayp->isRangeDelay() && getConstUInt(delayp->rhsp()) != delay_ಚಕ್ರಗಳು)
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಪೂರ್ಣಾಂಕ preLen = 0;
            ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ prep = sexprp->preExprp()) {
                ಆದರೆ (!flattenFixedSeq(prep, ತಳ_ಸ್ಥಾನಾಂತರ, ಹೊರಹೋಗು)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                preLen = fixed_ಉದ್ದಳತೆ(prep);
                ಆದರೆ (preLen < 0) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            ಹಿಂತಿರುಗಿ flattenFixedSeq(sexprp->exprp(), ತಳ_ಸ್ಥಾನಾಂತರ + preLen + delay_ಚಕ್ರಗಳು, ಹೊರಹೋಗು);
        }
        ಆದರೆ (nodep->is_Multi_ಚಕ್ರ_Sva()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹೊರಹೋಗು[ತಳ_ಸ್ಥಾನಾಂತರ].ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Conjoin two equal-length fixed sequences into one: at each clock offset
    // AND the boolean checks of both operands (IEEE 1800-2023 16.9.6 -- both
    // operands match the same window). Returns null if either operand is not a
    // plain fixed sequence of boolean leaves.
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* conjoinFixedSeqs(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, File_ಸಾಲು* flp) {
        std::map<ಪೂರ್ಣಾಂಕ, std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>> checks;
        ಆದರೆ (!flattenFixedSeq(lhsp, 0, checks) || !flattenFixedSeq(rhsp, 0, checks)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (checks.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = ಶೂನ್ಯಸೂಚಕ;
        ಪೂರ್ಣಾಂಕ prev_ಸ್ಥಾನಾಂತರ = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಸ್ಥಾನಾಂತರ_Checks : checks) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಸ್ಥಾನಾಂತರ = ಸ್ಥಾನಾಂತರ_Checks.ಮೊದಲ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ leafp : ಸ್ಥಾನಾಂತರ_Checks.ಎರಡನೇ) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ clonep = leafp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                ಆದರೆ (!condp) {
                    condp = clonep;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    condp = ಹೊಸ AstLogAnd{flp, condp, clonep};
                    condp->dtype_ಗಣ_ಬಿಟ್();
                }
            }
            ಆದರೆ (!resultp) {
                ಆದರೆ (ಸ್ಥಾನಾಂತರ > 0) {
                    AstDelay* ಸ್ಥಿರಾಂಕ delayp = ಹೊಸ AstDelay{
                        flp, ಹೊಸ AstConst{flp, static_cast<uint32_t>(ಸ್ಥಾನಾಂತರ)}, /*isCycle=*/ನಿಜ};
                    resultp
                        = ಹೊಸ AstSExpr{flp, ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}}, delayp, condp};
                    resultp->dtype_ಗಣ_ಬಿಟ್();
                } ಇಲ್ಲದಿದ್ದರೆ {
                    resultp = condp;
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                AstDelay* ಸ್ಥಿರಾಂಕ delayp = ಹೊಸ AstDelay{
                    flp, ಹೊಸ AstConst{flp, static_cast<uint32_t>(ಸ್ಥಾನಾಂತರ - prev_ಸ್ಥಾನಾಂತರ)},
                    /*isCycle=*/ನಿಜ};
                resultp = ಹೊಸ AstSExpr{flp, resultp, delayp, condp};
                resultp->dtype_ಗಣ_ಬಿಟ್();
            }
            prev_ಸ್ಥಾನಾಂತರ = ಸ್ಥಾನಾಂತರ;
        }
        ಹಿಂತಿರುಗಿ resultp;
    }

    // `seq` is a simple ranged sequence `start ##[m:n] end` (start/end boolean,
    // start may be absent). Used to collapse a both-variable intersect to one
    // ranged delay.
    ರಚನೆ SimpleRanged final {
        ತಾರ್ಕಿಕ ok = ಸುಳ್ಳು;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* startp = ಶೂನ್ಯಸೂಚಕ;  // may be null (absent start)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* endp = ಶೂನ್ಯಸೂಚಕ;
    };
    ಸ್ಥಿರ SimpleRanged asSimpleRanged(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        AstSExpr* ಸ್ಥಿರಾಂಕ sexprp = VN_CAST(nodep, SExpr);
        ಆದರೆ (!sexprp) ಹಿಂತಿರುಗಿ {};
        AstDelay* ಸ್ಥಿರಾಂಕ delayp = VN_CAST(sexprp->delayp(), Delay);
        ಆದರೆ (!delayp || !delayp->is_ಚಕ್ರ_Delay() || !delayp->isRangeDelay() || delayp->isUnbounded())
            ಹಿಂತಿರುಗಿ {};
        ಆದರೆ (getConstUInt(delayp->lhsp()) == getConstUInt(delayp->rhsp())) ಹಿಂತಿರುಗಿ {};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ prep = sexprp->preExprp();
        ಆದರೆ (prep && fixed_ಉದ್ದಳತೆ(prep) != 0) ಹಿಂತಿರುಗಿ {};
        ಆದರೆ (fixed_ಉದ್ದಳತೆ(sexprp->exprp()) != 0) ಹಿಂತಿರುಗಿ {};
        ಹಿಂತಿರುಗಿ {ನಿಜ, prep, sexprp->exprp()};
    }

    // Build the NFA for a synthesized intersect lowering tree, then free it.
    // buildExpr returns the terminal condition (finalCondp) by reference into the
    // tree; detach a clone so the tree can be freed here. The graph already holds
    // clones/hoists of every edge condition, so nothing else dangles.
    Build_ಫಲಿತಾಂಶ build_From_Lowering_ವೃಕ್ಷ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* treep, Sva_State_ಶೃಂಗ* ನಮೂದು_Vtxp,
                                      ತಾರ್ಕಿಕ is_ತುದಿ_Level_ಹೆಜ್ಜೆ) {
        Build_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = build_ಅಭಿವ್ಯಕ್ತಿ(treep, ನಮೂದು_Vtxp, is_ತುದಿ_Level_ಹೆಜ್ಜೆ);
        ಆದರೆ (ಫಲಿತಾಂಶ.valid() && ಫಲಿತಾಂಶ.finalCondp) {
            ಫಲಿತಾಂಶ.finalCondp = ಫಲಿತಾಂಶ.finalCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        }
        VL_DO_DANGLING(treep->delete_ವೃಕ್ಷ(), treep);
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }

    // Empty common-length intersection -- unequal fixed lengths, or disjoint
    // ranged lengths. IEEE 1800-2023 16.9.6 requires both operands to match
    // over a window of the same length, so with no common length the intersect
    // simply never matches. This is legal (matching nothing), not an error, so
    // lower to a constant false rather than rejecting legal code.
    Build_ಫಲಿತಾಂಶ build_Never_ಹೊಂದಿಕೆ_Intersect(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, Sva_State_ಶೃಂಗ* ನಮೂದು_Vtxp,
                                         ತಾರ್ಕಿಕ is_ತುದಿ_Level_ಹೆಜ್ಜೆ, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* reason) {
        nodep->v3warn(NEVERMATCH, "Sequence can never match because " << reason << ".");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ falsep = ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False{}};
        ಹಿಂತಿರುಗಿ build_From_Lowering_ವೃಕ್ಷ(falsep, ನಮೂದು_Vtxp, is_ತುದಿ_Level_ಹೆಜ್ಜೆ);
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
    Build_ಫಲಿತಾಂಶ buildVarLenIntersect(AstSIntersect* nodep, Sva_State_ಶೃಂಗ* ನಮೂದು_Vtxp,
                                     ತಾರ್ಕಿಕ is_ತುದಿ_Level_ಹೆಜ್ಜೆ) {
        ಸ್ಥಿರಾಂಕ std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ> lhsRange = ಉದ್ದಳತೆ_Range(nodep->lhsp());
        ಸ್ಥಿರಾಂಕ std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ> rhsRange = ಉದ್ದಳತೆ_Range(nodep->rhsp());
        ಆದರೆ (lhsRange.ಮೊದಲ < 0 || rhsRange.ಮೊದಲ < 0) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect with this variable-length operand");
            ಹಿಂತಿರುಗಿ Build_ಫಲಿತಾಂಶ::failWithError();
        }
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lo = std::max(lhsRange.ಮೊದಲ, rhsRange.ಮೊದಲ);
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ hi = std::min(lhsRange.ಎರಡನೇ, rhsRange.ಎರಡನೇ);
        ಆದರೆ (lo > hi) {
            // Disjoint length ranges share no common length -> never matches.
            ಹಿಂತಿರುಗಿ build_Never_ಹೊಂದಿಕೆ_Intersect(nodep, ನಮೂದು_Vtxp, is_ತುದಿ_Level_ಹೆಜ್ಜೆ,
                                            "intersect operands have no common length");
        }
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        ಆದರೆ (lo == hi) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lp = realize_At_ಉದ್ದಳತೆ(nodep->lhsp(), lo, lhsRange.ಮೊದಲ);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rp = realize_At_ಉದ್ದಳತೆ(nodep->rhsp(), lo, rhsRange.ಮೊದಲ);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ conjp = conjoinFixedSeqs(lp, rp, flp);
            VL_DO_DANGLING(lp->delete_ವೃಕ್ಷ(), lp);
            VL_DO_DANGLING(rp->delete_ವೃಕ್ಷ(), rp);
            ಆದರೆ (!conjp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: intersect operand is not a plain boolean sequence");
                ಹಿಂತಿರುಗಿ Build_ಫಲಿತಾಂಶ::failWithError();
            }
            ಹಿಂತಿರುಗಿ build_From_Lowering_ವೃಕ್ಷ(conjp, ನಮೂದು_Vtxp, is_ತುದಿ_Level_ಹೆಜ್ಜೆ);
        }
        ಸ್ಥಿರಾಂಕ SimpleRanged sl = asSimpleRanged(nodep->lhsp());
        ಸ್ಥಿರಾಂಕ SimpleRanged sr = asSimpleRanged(nodep->rhsp());
        ಆದರೆ (!sl.ok || !sr.ok) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect of two sequences that each vary in length over a"
                          " range with internal structure");
            ಹಿಂತಿರುಗಿ Build_ಫಲಿತಾಂಶ::failWithError();
        }
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ andBool = [&](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ap, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bp) -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ aClonep
                = ap ? ap->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು) : ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ bClonep
                = bp ? bp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು) : ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}};
            AstLogAnd* ಸ್ಥಿರಾಂಕ andp = ಹೊಸ AstLogAnd{flp, aClonep, bClonep};
            andp->dtype_ಗಣ_ಬಿಟ್();
            ಹಿಂತಿರುಗಿ andp;
        };
        AstDelay* ಸ್ಥಿರಾಂಕ delayp = ಹೊಸ AstDelay{flp, ಹೊಸ AstConst{flp, static_cast<uint32_t>(lo)},
                                              /*isCycle=*/ನಿಜ};
        delayp->rhsp(ಹೊಸ AstConst{flp, static_cast<uint32_t>(hi)});
        AstSExpr* ಸ್ಥಿರಾಂಕ reducedp
            = ಹೊಸ AstSExpr{flp, andBool(sl.startp, sr.startp), delayp, andBool(sl.endp, sr.endp)};
        reducedp->dtype_ಗಣ_ಬಿಟ್();
        ಹಿಂತಿರುಗಿ build_From_Lowering_ವೃಕ್ಷ(reducedp, ನಮೂದು_Vtxp, is_ತುದಿ_Level_ಹೆಜ್ಜೆ);
    }

    Build_ಫಲಿತಾಂಶ buildThroughout(AstSThroughout* nodep, Sva_State_ಶೃಂಗ* ನಮೂದು_Vtxp,
                                ತಾರ್ಕಿಕ is_ತುದಿ_Level_ಹೆಜ್ಜೆ = ಸುಳ್ಳು) {
        // Mark entryVtxp so "cond false at tick 0" is detected as throughout-drop.
        ನಮೂದು_Vtxp->m_throughoutConds.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep->lhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
        m_temporal_Guard_ರಾಶಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep->lhsp());
        ಸ್ಥಿರಾಂಕ Build_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = build_ಅಭಿವ್ಯಕ್ತಿ(nodep->rhsp(), ನಮೂದು_Vtxp, is_ತುದಿ_Level_ಹೆಜ್ಜೆ);
        m_temporal_Guard_ರಾಶಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
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
    Build_ಫಲಿತಾಂಶ buildUntil(AstUntil* nodep, Sva_State_ಶೃಂಗ* ನಮೂದು_Vtxp, ತಾರ್ಕಿಕ is_ತುದಿ_Level_ಹೆಜ್ಜೆ) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        ಆದರೆ (!is_ತುದಿ_Level_ಹೆಜ್ಜೆ) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            ಹಿಂತಿರುಗಿ Build_ಫಲಿತಾಂಶ::failWithError();
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp();
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ hasSeq = [](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ep) {
            ಹಿಂತಿರುಗಿ ep->exists([](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* np) { ಹಿಂತಿರುಗಿ np->is_Multi_ಚಕ್ರ_Sva(); });
        };
        ಆದರೆ (hasSeq(lhsp) || hasSeq(rhsp)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            ಹಿಂತಿರುಗಿ Build_ಫಲಿತಾಂಶ::failWithError();
        }

        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ov = nodep->isOverlapping();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsBitp = nodep->lhsp();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsBitp = nodep->rhsp();
        // p hoist count: continue, require (ov: 1 use; nov: 1 use). At least 2 uses.
        AstVar* ಸ್ಥಿರಾಂಕ pHoistp = tryHoistSampled(lhsBitp, flp, 2);
        // q hoist count: continue (1) + require nov (1) = 2; ov: continue only (1).
        AstVar* ಸ್ಥಿರಾಂಕ qHoistp = ov ? ಶೂನ್ಯಸೂಚಕ : tryHoistSampled(rhsBitp, flp, 2);

        Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ waitCp = scoped_Create_ಶೃಂಗ();
        Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ waitRp = scoped_Create_ಶೃಂಗ();
        waitCp->m_isUnbounded = ನಿಜ;
        waitRp->m_strongPending = nodep->isStrong() && !m_isCover;

        // Entry and back-loop Links carry no condition; throughout-folding still applies.
        guardedLink(ನಮೂದು_Vtxp, waitCp, flp);
        guardedLink(waitRp, waitCp, flp);

        // Continue clocked edge: p && !q advances to next-cycle wait.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ contCondp
            = ಹೊಸ AstLogAnd{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                            ಹೊಸ AstLogNot{flp, sampledRefOrClone(qHoistp, rhsBitp, flp)}};
        guarded_ಅಂಚು(waitCp, waitRp, contCondp, flp);

        // Reject sink: fires when require-condition is false.
        Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ sinkVtxp = m_ನಕ್ಷೆ.create_State_ಶೃಂಗ();
        sinkVtxp->m_isRejectSink = ನಿಜ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* requireCondp;
        ಆದರೆ (ov) {
            requireCondp = sampledRefOrClone(pHoistp, lhsBitp, flp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            requireCondp = ಹೊಸ AstLogOr{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                                        sampledRefOrClone(qHoistp, rhsBitp, flp)};
        }
        Sva_Trans_ಅಂಚು* ಸ್ಥಿರಾಂಕ rejEdgep = m_ನಕ್ಷೆ.ಕೂಡಿಸು_Link(waitCp, sinkVtxp, requireCondp);
        rejEdgep->m_rejectOnFail = ನಿಜ;

        // Accept condition rides via finalCondp; assembleResult $sampled-wraps it.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* acceptCondp;
        ಆದರೆ (ov) {
            acceptCondp
                = ಹೊಸ AstLogAnd{flp, lhsBitp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), rhsBitp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
        } ಇಲ್ಲದಿದ್ದರೆ {
            acceptCondp = rhsBitp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        }
        ಹಿಂತಿರುಗಿ {waitCp, acceptCondp, {}};
    }

    // IEEE 1800-2023 16.12.14 property abort operators. Sync and async share
    // the same NFA encoding: AstSampled already gives matured values at every
    // maturing clocking event, and async firing "between clocks" is not
    // observable in a cycle-based model. VAbortKind selects accept vs reject
    // verdict (sync vs async only changes the user-visible spelling).

    // Build `condp && !outer_1 && !outer_2 ...` (unsampled).
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* abort_Fire_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, File_ಸಾಲು* flp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = condp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ op : m_outer_Abort_ರಾಶಿ)
            resultp = ಹೊಸ AstLogAnd{flp, resultp, ಹೊಸ AstLogNot{flp, op->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)}};
        ಹಿಂತಿರುಗಿ resultp;
    }

    // True when unguarded ring-wide rejection or a same-tick Link chain already accounts the
    // attempt: a required-step Link covers both outcomes; followed-by pairs both edges.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ chain_Accounts_ಮೂಲ(ಸ್ಥಿರಾಂಕ Sva_State_ಶೃಂಗ* srcp,
                                    ಸ್ಥಿರಾಂಕ std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು*>& pre_ಅಂಚುಗಳು) {
        ಆದರೆ (srcp->m_delay_Ring_ಗಾತ್ರ && srcp->m_throughoutConds.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& edger : srcp->ಒಳ_ಅಂಚುಗಳು()) {
            ಆದರೆ (pre_ಅಂಚುಗಳು.ಎಣಿಕೆ_2(&edger)) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ Sva_Trans_ಅಂಚು& tedger = static_cast<ಸ್ಥಿರಾಂಕ Sva_Trans_ಅಂಚು&>(edger);
            ಆದರೆ (tedger.m_consumes_ಚಕ್ರ) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ fromp = static_cast<ಸ್ಥಿರಾಂಕ Sva_State_ಶೃಂಗ*>(tedger.fromVtxp());
            ಆದರೆ (fromp->m_abortRejectp) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ತಾರ್ಕಿಕ plainNonSink = ಸುಳ್ಳು;
        ತಾರ್ಕಿಕ markedSink = ಸುಳ್ಳು;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& edger : srcp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            ಆದರೆ (pre_ಅಂಚುಗಳು.ಎಣಿಕೆ_2(&edger)) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ Sva_Trans_ಅಂಚು& tedger = static_cast<ಸ್ಥಿರಾಂಕ Sva_Trans_ಅಂಚು&>(edger);
            ಆದರೆ (tedger.m_consumes_ಚಕ್ರ) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ sink = static_cast<ಸ್ಥಿರಾಂಕ Sva_State_ಶೃಂಗ*>(tedger.ಗಮ್ಯ_Vtxp())->m_isRejectSink;
            ಆದರೆ (tedger.m_rejectOnFail) {
                ಆದರೆ (!sink) ಹಿಂತಿರುಗಿ ನಿಜ;
                markedSink = ನಿಜ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!sink) {
                ಆದರೆ (srcp->m_abortRejectp) ಹಿಂತಿರುಗಿ ನಿಜ;
                plainNonSink = ನಿಜ;
            }
        }
        ಹಿಂತಿರುಗಿ plainNonSink && markedSink;
    }

    // Reject edge: fires when the source is live and the abort samples true.
    ಶೂನ್ಯ ಕೂಡಿಸು_Abort_Reject_ಅಂಚು(Sva_State_ಶೃಂಗ* srcp, Sva_State_ಶೃಂಗ* sinkp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp,
                            File_ಸಾಲು* flp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ notFirep = ಹೊಸ AstLogNot{flp, sampled(abort_Fire_ಅಭಿವ್ಯಕ್ತಿ(condp, flp))};
        m_ನಕ್ಷೆ.ಕೂಡಿಸು_Link(srcp, sinkp, notFirep)->m_rejectOnFail = ನಿಜ;
        ಹಿಂತಿರುಗಿ;
    }

    // On the fire tick: kill body threads; accept kinds also forgive step misses.
    ಶೂನ್ಯ gate_Body_ಅಂಚುಗಳು_On_Abort(ಸ್ಥಿರಾಂಕ std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು*>& pre_ಅಂಚುಗಳು,
                              Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, VAbort_ಬಗೆ ಬಗೆ, File_ಸಾಲು* flp) {
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtxr : m_ನಕ್ಷೆ.m_ನಕ್ಷೆ.vertices()) {
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& edger : vtxr.ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                ಆದರೆ (pre_ಅಂಚುಗಳು.ಎಣಿಕೆ_2(&edger)) ಮುಂದುವರಿ;
                Sva_Trans_ಅಂಚು* ಸ್ಥಿರಾಂಕ tedgep = static_cast<Sva_Trans_ಅಂಚು*>(&edger);
                ಆದರೆ (tedgep->m_rejectOnFail) {
                    ಆದರೆ (!ಬಗೆ.isAccept()) ಮುಂದುವರಿ;
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ firep = sampled(abort_Fire_ಅಭಿವ್ಯಕ್ತಿ(condp, flp));
                    tedgep->m_condp
                        = tedgep->m_condp ? ಹೊಸ AstLogOr{flp, tedgep->m_condp, firep} : firep;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tedgep->m_consumes_ಚಕ್ರ) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ notFirep
                        = ಹೊಸ AstLogNot{flp, sampled(abort_Fire_ಅಭಿವ್ಯಕ್ತಿ(condp, flp))};
                    tedgep->m_condp = tedgep->m_condp
                                          ? ಹೊಸ AstLogAnd{flp, tedgep->m_condp, notFirep}
                                          : notFirep;
                }
            }
        }
    }

    Build_ಫಲಿತಾಂಶ buildAbortOn(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bodyp, Sva_State_ಶೃಂಗ* ನಮೂದು_Vtxp,
                             VAbort_ಬಗೆ ಬಗೆ, File_ಸಾಲು* flp, ತಾರ್ಕಿಕ is_ತುದಿ_Level_ಹೆಜ್ಜೆ) {
        // Snapshot pre-body vertices/edges so post-build diff yields the body's sub-NFA.
        std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ*> preExisting;
        std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು*> pre_ಅಂಚುಗಳು;
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtxr : m_ನಕ್ಷೆ.m_ನಕ್ಷೆ.vertices()) {
            preExisting.ಸೇರಿಸು(&vtxr);
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& edger : vtxr.ಹೊರಹೋಗು_ಅಂಚುಗಳು()) pre_ಅಂಚುಗಳು.ಸೇರಿಸು(&edger);
        }

        m_outer_Abort_ರಾಶಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(condp);
        ಸ್ಥಿರಾಂಕ Build_ಫಲಿತಾಂಶ body_ಫಲಿತಾಂಶ = build_ಅಭಿವ್ಯಕ್ತಿ(bodyp, ನಮೂದು_Vtxp, is_ತುದಿ_Level_ಹೆಜ್ಜೆ);
        m_outer_Abort_ರಾಶಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
        ಆದರೆ (!body_ಫಲಿತಾಂಶ.valid()) ಹಿಂತಿರುಗಿ body_ಫಲಿತಾಂಶ;

        gate_Body_ಅಂಚುಗಳು_On_Abort(pre_ಅಂಚುಗಳು, condp, ಬಗೆ, flp);

        // Live-thread sources for the abort edge: entry + new body vertices,
        // minus reject sinks (they carry reject fuel, not live-thread fuel).
        std::ಸದಿಶ<Sva_State_ಶೃಂಗ*> abortSources;
        abortSources.ತಳ್ಳು_ಹಿಂಭಾಗ(ನಮೂದು_Vtxp);
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtxr : m_ನಕ್ಷೆ.m_ನಕ್ಷೆ.vertices()) {
            ಆದರೆ (preExisting.ಎಣಿಕೆ_2(&vtxr)) ಮುಂದುವರಿ;
            ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ sp = static_cast<Sva_State_ಶೃಂಗ*>(&vtxr);
            ಆದರೆ (sp->m_delay_Ring_ಗಾತ್ರ || !sp->m_throughoutConds.ಖಾಲಿಗಳು()) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ firep = abort_Fire_ಅಭಿವ್ಯಕ್ತಿ(condp, flp);
                sp->m_abortClearp
                    = sp->m_abortClearp ? ಹೊಸ AstLogOr{flp, sp->m_abortClearp, firep} : firep;
            }
            ಆದರೆ (!ಬಗೆ.isAccept()
                && ((sp->m_delay_Ring_ಗಾತ್ರ && sp->m_throughoutConds.ಖಾಲಿಗಳು())
                    || sp->m_replayAbortReject)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ firep = abort_Fire_ಅಭಿವ್ಯಕ್ತಿ(condp, flp);
                sp->m_abortRejectp
                    = sp->m_abortRejectp ? ಹೊಸ AstLogOr{flp, sp->m_abortRejectp, firep} : firep;
            }
            ಆದರೆ (sp->m_isRejectSink) ಮುಂದುವರಿ;
            abortSources.ತಳ್ಳು_ಹಿಂಭಾಗ(sp);
        }

        ಸ್ವಯಂಚಾಲಿತ sampledAbortFire = [&]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಅಭಿವ್ಯಕ್ತಿ = abort_Fire_ಅಭಿವ್ಯಕ್ತಿ(condp, flp);
            ಹಿಂತಿರುಗಿ sampled(ಅಭಿವ್ಯಕ್ತಿ);
        };

        ಆದರೆ (ಬಗೆ.isAccept()) {
            // Match-only sink fed by $sampled(abort-fire) from every live source;
            // registered as midSource so it never contributes a reject.
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ acceptSinkp = scoped_Create_ಶೃಂಗ();
            ಗಾಗಿ (Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp : abortSources)
                guardedLink(srcp, acceptSinkp, sampledAbortFire(), flp);
            std::ಸದಿಶ<Sva_State_ಶೃಂಗ*> midSources = body_ಫಲಿತಾಂಶ.midSources;
            midSources.ತಳ್ಳು_ಹಿಂಭಾಗ(acceptSinkp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* finalCondp = body_ಫಲಿತಾಂಶ.finalCondp;
            ಆದರೆ (finalCondp) {
                ಆದರೆ (finalCondp->backp()) finalCondp = finalCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                finalCondp = ಹೊಸ AstLogOr{flp, finalCondp, abort_Fire_ಅಭಿವ್ಯಕ್ತಿ(condp, flp)};
            }
            ಹಿಂತಿರುಗಿ {body_ಫಲಿತಾಂಶ.ಪದ_Vertexp, finalCondp, std::move(midSources)};
        }

        // rejectOnFail treats m_condp as the success condition and fires on
        // !condp, so the edge carries !sampledAbortFire().
        Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rejectSinkp = m_ನಕ್ಷೆ.create_State_ಶೃಂಗ();
        rejectSinkp->m_isRejectSink = ನಿಜ;
        ಗಾಗಿ (Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp : abortSources)
            ಆದರೆ (!chain_Accounts_ಮೂಲ(srcp, pre_ಅಂಚುಗಳು))
                ಕೂಡಿಸು_Abort_Reject_ಅಂಚು(srcp, rejectSinkp, condp, flp);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* finalCondp = body_ಫಲಿತಾಂಶ.finalCondp;
        ಆದರೆ (finalCondp) {
            ಆದರೆ (finalCondp->backp()) finalCondp = finalCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            finalCondp
                = ಹೊಸ AstLogAnd{flp, finalCondp, ಹೊಸ AstLogNot{flp, abort_Fire_ಅಭಿವ್ಯಕ್ತಿ(condp, flp)}};
        }
        ಹಿಂತಿರುಗಿ {body_ಫಲಿತಾಂಶ.ಪದ_Vertexp, finalCondp, body_ಫಲಿತಾಂಶ.midSources};
    }

ಸಾರ್ವಜನಿಕ:
    // Free a dropped sub-result condition that is not linked into the AST
    // (abort folds synthesize unparented finalCondp trees).
    ಸ್ಥಿರ ಶೂನ್ಯ freeUnlinkedCondp(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        ಆದರೆ (condp && !condp->backp()) VL_DO_DANGLING(condp->delete_ವೃಕ್ಷ(), condp);
    }

    SvaNfaBuilder(Sva_ನಕ್ಷೆ& ನಕ್ಷೆ, Ast_ಗಂಟು_Module* modp, V3_Unique_ಹೆಸರುಗಳು& prop_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು,
                  ತಾರ್ಕಿಕ isCoverSeq = ಸುಳ್ಳು, ತಾರ್ಕಿಕ isSeqEvent = ಸುಳ್ಳು, ತಾರ್ಕಿಕ isCover = ಸುಳ್ಳು)
        : m_ನಕ್ಷೆ{ನಕ್ಷೆ}
        , m_modp{modp}
        , m_prop_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು{prop_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು}
        , m_isCover{isCover}
        , m_isCoverSeq{isCoverSeq}
        , m_isSeqEvent{isSeqEvent} {}

    // Reset scope between antecedent and consequent: liveness must not leak.
    // m_outerAbortStack survives: an abort wrapping the implication covers the
    // consequent too (IEEE 1800-2023 16.12.14).
    ಶೂನ್ಯ resetScope() {
        m_ಒಳ_Unbounded_Scope = ಸುಳ್ಳು;
        m_temporal_Guard_ರಾಶಿ.clear();
    }

    Build_ಫಲಿತಾಂಶ build_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, Sva_State_ಶೃಂಗ* ನಮೂದು_Vtxp,
                          ತಾರ್ಕಿಕ is_ತುದಿ_Level_ಹೆಜ್ಜೆ = ಸುಳ್ಳು) {
        VL_RESTORER(m_want_Bool_ಎಲೆ);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ want_Bool_ಎಲೆ = m_want_Bool_ಎಲೆ;
        m_want_Bool_ಎಲೆ = ಸುಳ್ಳು;
        ಆದರೆ (AstSExpr* ಸ್ಥಿರಾಂಕ sexprp = VN_CAST(nodep, SExpr)) {
            ಹಿಂತಿರುಗಿ buildSExpr(sexprp, ನಮೂದು_Vtxp, is_ತುದಿ_Level_ಹೆಜ್ಜೆ);
        }
        ಆದರೆ (AstSConsRep* ಸ್ಥಿರಾಂಕ repp = VN_CAST(nodep, SConsRep)) {
            ಹಿಂತಿರುಗಿ buildConsRep(repp, ನಮೂದು_Vtxp, is_ತುದಿ_Level_ಹೆಜ್ಜೆ);
        }
        ಆದರೆ (AstPropAlways* ಸ್ಥಿರಾಂಕ alwaysp = VN_CAST(nodep, PropAlways)) {
            ಹಿಂತಿರುಗಿ buildPropAlways(alwaysp, ನಮೂದು_Vtxp, is_ತುದಿ_Level_ಹೆಜ್ಜೆ);
        }
        ಆದರೆ (AstSGotoRep* ಸ್ಥಿರಾಂಕ repp = VN_CAST(nodep, SGotoRep)) {
            ಹಿಂತಿರುಗಿ buildGotoRep(repp, ನಮೂದು_Vtxp);
        }
        ಆದರೆ (AstSThroughout* ಸ್ಥಿರಾಂಕ throughoutp = VN_CAST(nodep, SThroughout)) {
            ಹಿಂತಿರುಗಿ buildThroughout(throughoutp, ನಮೂದು_Vtxp, is_ತುದಿ_Level_ಹೆಜ್ಜೆ);
        }
        ಆದರೆ (AstSOr* ಸ್ಥಿರಾಂಕ orp = VN_CAST(nodep, SOr)) {
            ಹಿಂತಿರುಗಿ build_Or_ವಿಲೀನ(orp->lhsp(), orp->rhsp(), ನಮೂದು_Vtxp, orp->fileline(),
                                want_Bool_ಎಲೆ);
        }
        ಆದರೆ (AstLogOr* ಸ್ಥಿರಾಂಕ orp = VN_CAST(nodep, LogOr)) {
            ಹಿಂತಿರುಗಿ build_Or_ವಿಲೀನ(orp->lhsp(), orp->rhsp(), ನಮೂದು_Vtxp, orp->fileline(),
                                want_Bool_ಎಲೆ);
        }
        ಆದರೆ (AstSAnd* ಸ್ಥಿರಾಂಕ andp = VN_CAST(nodep, SAnd)) {
            ಹಿಂತಿರುಗಿ buildAndCombiner(andp->lhsp(), andp->rhsp(), ನಮೂದು_Vtxp, andp->fileline());
        }
        ಆದರೆ (AstSIntersect* ಸ್ಥಿರಾಂಕ intp = VN_CAST(nodep, SIntersect)) {
            // IEEE 1800-2023 16.9.6: both operands match over one window with
            // equal start and end (equal length). Lower to a single sequence
            // that conjoins both operands' per-cycle checks -- correct under
            // concurrent attempts, where the done-latch combiner conflates the
            // two operands' start times and over-accepts. The combiner remains
            // only as a fallback for operands that do not flatten.
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lhsLen = fixed_ಉದ್ದಳತೆ(intp->lhsp());
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ rhsLen = fixed_ಉದ್ದಳತೆ(intp->rhsp());
            ಆದರೆ (lhsLen >= 0 && rhsLen >= 0) {
                ಆದರೆ (lhsLen != rhsLen) {
                    // Unequal fixed lengths share no common length -> never matches.
                    ಹಿಂತಿರುಗಿ build_Never_ಹೊಂದಿಕೆ_Intersect(intp, ನಮೂದು_Vtxp, is_ತುದಿ_Level_ಹೆಜ್ಜೆ,
                                                    "intersect operands have no common length");
                }
                ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ conjp
                    = conjoinFixedSeqs(intp->lhsp(), intp->rhsp(), intp->fileline())) {
                    ಹಿಂತಿರುಗಿ build_From_Lowering_ವೃಕ್ಷ(conjp, ನಮೂದು_Vtxp, is_ತುದಿ_Level_ಹೆಜ್ಜೆ);
                }
                ಹಿಂತಿರುಗಿ buildAndCombiner(intp->lhsp(), intp->rhsp(), ನಮೂದು_Vtxp, intp->fileline());
            }
            ಹಿಂತಿರುಗಿ buildVarLenIntersect(intp, ನಮೂದು_Vtxp, is_ತುದಿ_Level_ಹೆಜ್ಜೆ);
        }
        ಆದರೆ (AstSWithin* ಸ್ಥಿರಾಂಕ withinp = VN_CAST(nodep, SWithin)) {
            ಹಿಂತಿರುಗಿ buildSWithin(withinp, ನಮೂದು_Vtxp, is_ತುದಿ_Level_ಹೆಜ್ಜೆ);
        }
        ಆದರೆ (AstAbortOn* ಸ್ಥಿರಾಂಕ ap = VN_CAST(nodep, AbortOn)) {
            ಹಿಂತಿರುಗಿ buildAbortOn(ap->condp(), ap->propp(), ನಮೂದು_Vtxp, ap->ಬಗೆ(), ap->fileline(),
                                is_ತುದಿ_Level_ಹೆಜ್ಜೆ);
        }
        ಆದರೆ (VN_IS(nodep, SNonConsRep)) ಹಿಂತಿರುಗಿ Build_ಫಲಿತಾಂಶ::fail();
        ಆದರೆ (AstImplication* ಸ್ಥಿರಾಂಕ implp = VN_CAST(nodep, Implication)) {
            ಹಿಂತಿರುಗಿ build_Implication_ಅಂಚುಗಳು(implp->lhsp(), implp->rhsp(), ನಮೂದು_Vtxp,
                                         implp->isOverlapped(), implp->isFollowedBy(),
                                         implp->lhsp(), implp->fileline());
        }
        ಆದರೆ (AstUntil* ಸ್ಥಿರಾಂಕ untilp = VN_CAST(nodep, Until)) {
            ಹಿಂತಿರುಗಿ buildUntil(untilp, ನಮೂದು_Vtxp, is_ತುದಿ_Level_ಹೆಜ್ಜೆ);
        }
        // Boolean leaf (including LogAnd): return as finalCond
        ಹಿಂತಿರುಗಿ {ನಮೂದು_Vtxp, nodep, {}};
    }

    // Wire an implication / followed-by from `entryVtxp`: builds the antecedent,
    // emits the match-link (and for followed-by the reject-sink edge), inserts a
    // delay vertex for non-overlapped forms, and builds the body. Used both for
    // nested AstImplication in pexpr position and for the top-level assertion
    // antecedent -- `errorNodep` anchors the "unsupported sequence antecedent"
    // error, which differs between the two call sites.
    Build_ಫಲಿತಾಂಶ build_Implication_ಅಂಚುಗಳು(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* antExprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bodyExprp,
                                      Sva_State_ಶೃಂಗ* ನಮೂದು_Vtxp, ತಾರ್ಕಿಕ isOverlapped,
                                      ತಾರ್ಕಿಕ isFollowedBy, Ast_ಗಂಟು* errorNodep, File_ಸಾಲು* flp) {
        ಸ್ಥಿರಾಂಕ Build_ಫಲಿತಾಂಶ ant_ಫಲಿತಾಂಶ = build_ಅಭಿವ್ಯಕ್ತಿ(antExprp, ನಮೂದು_Vtxp);
        ಆದರೆ (!ant_ಫಲಿತಾಂಶ.valid()) ಹಿಂತಿರುಗಿ ant_ಫಲಿತಾಂಶ;

        // Followed-by requires pure-boolean antecedent for non-vacuous-fail at
        // the attempt-start cycle. IEEE 1800-2023 16.12.9 permits a multi-cycle
        // sequence LHS, so this is an implementation gap rather than illegal SV.
        ಆದರೆ (isFollowedBy && ant_ಫಲಿತಾಂಶ.ಪದ_Vertexp != ನಮೂದು_Vtxp) {
            errorNodep->v3warn(E_UNSUPPORTED,
                               "Unsupported: sequence expression as antecedent of followed-by"
                               " (#-# / #=#) (IEEE 1800-2023 16.12.9)");
            ಹಿಂತಿರುಗಿ Build_ಫಲಿತಾಂಶ::failWithError();
        }
        UASSERT_OBJ(!isFollowedBy || ant_ಫಲಿತಾಂಶ.finalCondp, errorNodep,
                    "followed-by antecedent terminal at entry must carry finalCondp");

        // Use raw createStateVertex() so trigVtxp starts without liveness --
        // reaching the antecedent terminal is a definitive event.
        Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ trigVtxp = m_ನಕ್ಷೆ.create_State_ಶೃಂಗ();
        ಆದರೆ (ant_ಫಲಿತಾಂಶ.finalCondp) {
            m_ನಕ್ಷೆ.ಕೂಡಿಸು_Link(ant_ಫಲಿತಾಂಶ.ಪದ_Vertexp, trigVtxp,
                            sampled(ant_ಫಲಿತಾಂಶ.finalCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)));
            // Followed-by non-vacuous fail: rejectOnFail fires when the attempt
            // is live (termVtx reachable) and sampled(antecedent) is false.
            ಆದರೆ (isFollowedBy) {
                Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ sinkVtxp = m_ನಕ್ಷೆ.create_State_ಶೃಂಗ();
                sinkVtxp->m_isRejectSink = ನಿಜ;
                Sva_Trans_ಅಂಚು* ಸ್ಥಿರಾಂಕ ep
                    = m_ನಕ್ಷೆ.ಕೂಡಿಸು_Link(ant_ಫಲಿತಾಂಶ.ಪದ_Vertexp, sinkVtxp,
                                      sampled(ant_ಫಲಿತಾಂಶ.finalCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)));
                ep->m_rejectOnFail = ನಿಜ;
            }
            // finalCondp is cloned into the Sampled nodes; if the original is
            // not parented anywhere in the AST anymore it must be freed here
            // or ASan flags it as a leak (e.g. t_sequence_bool_ops).
            ಆದರೆ (!ant_ಫಲಿತಾಂಶ.finalCondp->backp()) {
                VL_DO_DANGLING(ant_ಫಲಿತಾಂಶ.finalCondp->delete_ವೃಕ್ಷ(), ant_ಫಲಿತಾಂಶ.finalCondp);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_ನಕ್ಷೆ.ಕೂಡಿಸು_Link(ant_ಫಲಿತಾಂಶ.ಪದ_Vertexp, trigVtxp);
        }
        resetScope();

        Sva_State_ಶೃಂಗ* bodyEntryp = trigVtxp;
        ಆದರೆ (!isOverlapped) {
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ delayVtxp = m_ನಕ್ಷೆ.create_State_ಶೃಂಗ();
            m_ನಕ್ಷೆ.ಕೂಡಿಸು_Clocked_ಅಂಚು(trigVtxp, delayVtxp);
            bodyEntryp = delayVtxp;
        }
        // An empty antecedent match must not start the consequent, but a zero-minimum
        // repetition's empty match is not handled here; keep the merge vertex for those
        VL_RESTORER(m_want_Bool_ಎಲೆ);
        m_want_Bool_ಎಲೆ = !isFollowedBy && !hasZeroMinRep(antExprp);
        ಹಿಂತಿರುಗಿ build_ಅಭಿವ್ಯಕ್ತಿ(bodyExprp, bodyEntryp, /*isTopLevelStep=*/ನಿಜ);
    }

    Build_ಫಲಿತಾಂಶ build(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        m_ನಕ್ಷೆ.m_ಆರಂಭ_Vertexp = scoped_Create_ಶೃಂಗ();
        VL_RESTORER(m_want_Bool_ಎಲೆ);
        m_want_Bool_ಎಲೆ = ನಿಜ;
        ಹಿಂತಿರುಗಿ build_ಅಭಿವ್ಯಕ್ತಿ(exprp, m_ನಕ್ಷೆ.m_ಆರಂಭ_Vertexp, /*isTopLevelStep=*/ನಿಜ);
    }
};

//######################################################################
// NFA Lowering (converts NFA graph to synthesizable AstAlways blocks)

ವರ್ಗ SvaNfaLowering final {
    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ m_modp;  // Module to add state vars and always blocks to
    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ m_u32DTypep;  // Shared unsigned counter dtype
    V3_Unique_ಹೆಸರುಗಳು m_ಹೆಸರುಗಳು{"__Vnfa"};
    ಗಾತ್ರ_t m_stat_Delay_Ring_ಅಂಚು_Visits = 0;  // Delay-ring incoming edges visited

    // Per-lowering shared context (passed to phase sub-functions)
    // Per-vertex lowering state is stored in SvaVertexData and accessed via
    // V3GraphVertex::userp() (see SvaStateVertex::datap()).
    ರಚನೆ ಅಧೋ_Ctx final {
        File_ಸಾಲು* flp;  // Source location for generated AST
        Ast_Sen_ವೃಕ್ಷ* senTreep;  // Clock sensitivity tree
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* disableExprp;  // disable iff expression (may be nullptr)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಹೊಂದಿಕೆ_Condp;  // Final boolean match condition (may be nullptr)
        VAssertType assertType;  // Assertion type for control tasks
        VAssertDirectiveType directiveType;  // Directive type for control tasks
        AstVar* killVarp;  // Last observed kill generation
        Sva_ನಕ್ಷೆ& ನಕ್ಷೆ;  // NFA graph
    };

    // Build a match-now expression: stateSig[i] && $sampled(condp)
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* build_ಹೊಂದಿಕೆ_Now(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* stateExprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ statep = stateExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        ಆದರೆ (!condp) ಹಿಂತಿರುಗಿ statep;
        ಹಿಂತಿರುಗಿ ಹೊಸ AstLogAnd{flp, statep, sampled(condp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು))};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* andCond(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        ಆದರೆ (!condp) ಹಿಂತಿರುಗಿ exprp;
        ಹಿಂತಿರುಗಿ ಹೊಸ AstLogAnd{flp, exprp, condp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
    }
    // bp is always non-null; only ap can be null (serving as accumulator).
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* orExprs(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ap, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bp) {
        ಆದರೆ (!ap) ಹಿಂತಿರುಗಿ bp;
        ಹಿಂತಿರುಗಿ ಹೊಸ AstLogOr{flp, ap, bp};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಕೂಡಿಸು_Countp(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಒಟ್ಟು_Countp,
                                  Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ contributionp,
                                  Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enablep = ಶೂನ್ಯಸೂಚಕ) {
        // contribution = enable ? contribution : 0;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enabledContributionp
            = enablep ? ಹೊಸ AstCond{flp, enablep, contributionp,
                                    newTypedConstp(flp, contributionp->dtypep(), 0)}
                      : contributionp;
        ಆದರೆ (!ಒಟ್ಟು_Countp) ಹಿಂತಿರುಗಿ enabledContributionp;
        ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ಕೂಡಿಸು{flp, ಒಟ್ಟು_Countp, enabledContributionp};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* gateCheckedCountp(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ countp,
                                          Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*& overflowp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enablep) {
        ಆದರೆ (overflowp) {
            overflowp = ಹೊಸ AstLogAnd{flp, enablep->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), overflowp};
        }
        ಹಿಂತಿರುಗಿ ಕೂಡಿಸು_Countp(flp, ಶೂನ್ಯಸೂಚಕ, countp, enablep);
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಕೂಡಿಸು_Checked_Countp(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಒಟ್ಟು_Countp,
                                         Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ contributionp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*& overflowp,
                                         Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ contributionOverflowp = ಶೂನ್ಯಸೂಚಕ) {
        ಆದರೆ (contributionOverflowp) { overflowp = orExprs(flp, overflowp, contributionOverflowp); }
        ಆದರೆ (!ಒಟ್ಟು_Countp) ಹಿಂತಿರುಗಿ contributionp;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ oldTotalp = ಒಟ್ಟು_Countp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ resultp = ಹೊಸ Ast_ಕೂಡಿಸು{flp, ಒಟ್ಟು_Countp, contributionp};
        overflowp
            = orExprs(flp, overflowp, ಹೊಸ AstLt{flp, resultp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), oldTotalp});
        ಹಿಂತಿರುಗಿ resultp;
    }
    AstIf* new_ಎಣಿಕೆ_ಅತಿಪ್ರವಾಹ_Warning(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp) ಸ್ಥಿರಾಂಕ {
        AstDisplay* ಸ್ಥಿರಾಂಕ dispp = ಹೊಸ AstDisplay{
            flp, VDisplayType::DT_WARNING,
            "Cover sequence match count overflowed 32-bit storage; results may be incorrect.",
            ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, condp, dispp};
        ifp->branchPred(VBranchPred::BP_UNLIKELY);
        ಹಿಂತಿರುಗಿ ifp;
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* kill_ಸಕ್ರಿಯ(ಅಧೋ_Ctx& c) {
        ಹಿಂತಿರುಗಿ ಹೊಸ AstNeq{c.flp, ಹೊಸ AstVarRef{c.flp, c.killVarp, VAccess::READ},
                          assertKillGet(c.flp, c.assertType, c.directiveType)};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* not_Kill_ಸಕ್ರಿಯ(ಅಧೋ_Ctx& c) { ಹಿಂತಿರುಗಿ ಹೊಸ AstLogNot{c.flp, kill_ಸಕ್ರಿಯ(c)}; }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* gateNotKill(ಅಧೋ_Ctx& c, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಆದರೆ (!exprp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಹಿಂತಿರುಗಿ ಹೊಸ AstLogAnd{c.flp, exprp, not_Kill_ಸಕ್ರಿಯ(c)};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಮುಂದಿನ_Ring_ಸೂಚ್ಯಂಕ(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* idxExprp, uint32_t ಗಾತ್ರ_2) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ u32Const = [flp](uint32_t ಮೌಲ್ಯ) {
            ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, ಮೌಲ್ಯ};
        };
        UASSERT_OBJ(ಗಾತ್ರ_2 > 0, idxExprp, "Ring size must be positive");
        ಆದರೆ (ಗಾತ್ರ_2 == 1) {
            VL_DO_DANGLING(idxExprp->delete_ವೃಕ್ಷ(), idxExprp);
            ಹಿಂತಿರುಗಿ u32Const(0);
        }
        // idx == size - 1 ? 0 : idx + 1
        Ast_ಕೂಡಿಸು* ಸ್ಥಿರಾಂಕ addp = ಹೊಸ Ast_ಕೂಡಿಸು{flp, idxExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), u32Const(1)};
        addp->dtypeFrom(idxExprp);
        AstCond* ಸ್ಥಿರಾಂಕ condp
            = ಹೊಸ AstCond{flp, ಹೊಸ AstEq{flp, idxExprp, u32Const(ಗಾತ್ರ_2 - 1)}, u32Const(0), addp};
        condp->dtypeFrom(idxExprp);
        ಹಿಂತಿರುಗಿ condp;
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* delay_Ring_ಸ್ಥಾನ(File_ಸಾಲು* flp, Sva_State_ಶೃಂಗ* vtxp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* idxExprp,
                                      VAccess access = VAccess::READ) {
        // ring[idx]
        AstVar* ಸ್ಥಿರಾಂಕ ringp = vtxp->datap()->delayRingVarp;
        AstVarRef* ಸ್ಥಿರಾಂಕ refp = ಹೊಸ AstVarRef{flp, ringp, access};
        ಆದರೆ (vtxp->datap()->delayRingCounts) ಹಿಂತಿರುಗಿ ಹೊಸ AstArraySel{flp, refp, idxExprp};
        ಹಿಂತಿರುಗಿ ಹೊಸ AstSel{flp, refp, idxExprp, 1};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* delay_Ring_ತಲೆ_ಎಣಿಕೆ(File_ಸಾಲು* flp, Sva_State_ಶೃಂಗ* vtxp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ headp
            = ಹೊಸ AstVarRef{flp, vtxp->datap()->delay_Ring_ತಲೆ_Varp, VAccess::READ};
        ಆದರೆ (vtxp->datap()->delayRingCounts) ಹಿಂತಿರುಗಿ headp;
        ಹಿಂತಿರುಗಿ ಹೊಸ AstExtend{flp, headp, vtxp->datap()->delay_Ring_ಜೀವಂತ_ಎಣಿಕೆ_Varp->dtypep()->ಅಗಲ()};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* delay_Ring_At_ಕೊನೆಯ_ಸೂಚ್ಯಂಕ(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp, AstVar* ಸ್ಥಿರಾಂಕ idxp,
                                             ಸ್ಥಿರಾಂಕ uint32_t ಗಾತ್ರ_2) {
        ಹಿಂತಿರುಗಿ ಹೊಸ AstEq{flp, ಹೊಸ AstVarRef{flp, idxp, VAccess::READ},
                         ಹೊಸ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, ಗಾತ್ರ_2 - 1}};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* delay_Ring_Output_ಎಣಿಕೆ(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp, Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp) {
        AstVar* ಸ್ಥಿರಾಂಕ idxp = vtxp->datap()->delayRingIdxVarp;
        ಸ್ಥಿರಾಂಕ uint32_t ಗಾತ್ರ_2 = vtxp->m_delay_Ring_ಗಾತ್ರ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* outgoingValidp
            = ಹೊಸ AstVarRef{flp, vtxp->datap()->delayRingWrappedVarp, VAccess::READ};
        ಆದರೆ (!vtxp->m_isFixedDelayRing) {
            outgoingValidp
                = ಹೊಸ AstLogOr{flp, outgoingValidp, delay_Ring_At_ಕೊನೆಯ_ಸೂಚ್ಯಂಕ(flp, idxp, ಗಾತ್ರ_2)};
        }
        AstVar* ಸ್ಥಿರಾಂಕ ಜೀವಂತ_ಎಣಿಕೆ_Varp = vtxp->datap()->delay_Ring_ಜೀವಂತ_ಎಣಿಕೆ_Varp;
        ಹಿಂತಿರುಗಿ ಹೊಸ AstCond{flp, outgoingValidp, delay_Ring_ತಲೆ_ಎಣಿಕೆ(flp, vtxp),
                           newTypedConstp(flp, ಜೀವಂತ_ಎಣಿಕೆ_Varp->dtypep(), 0)};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* delay_Ring_Has_ಜೀವಂತ_Threadsp(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp,
                                                 AstVar* ಸ್ಥಿರಾಂಕ ಜೀವಂತ_ಎಣಿಕೆ_Varp) {
        // active = live_count != 0;
        ಹಿಂತಿರುಗಿ ಹೊಸ AstNeq{flp, ಹೊಸ AstVarRef{flp, ಜೀವಂತ_ಎಣಿಕೆ_Varp, VAccess::READ},
                          newTypedConstp(flp, ಜೀವಂತ_ಎಣಿಕೆ_Varp->dtypep(), 0)};
    }

    // Phase 3 output signals
    ರಚನೆ Signal_ಗಣ final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* terminalActivep = ಶೂನ್ಯಸೂಚಕ;  // OR of all successful terminal matches
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಹೊಂದಿಕೆ_Countp = ಶೂನ್ಯಸೂಚಕ;  // NFA paths completing the sequence this tick
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಹೊಂದಿಕೆ_ಎಣಿಕೆ_Overflowp = ಶೂನ್ಯಸೂಚಕ;  // Match count overflow this tick
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rejectBasep = ಶೂನ್ಯಸೂಚಕ;  // Reject when a terminal match fails
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* required_ಹೆಜ್ಜೆ_Rejectp = ಶೂನ್ಯಸೂಚಕ;  // Per-source reject from rejectOnFail Links
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* throughoutRejectp = ಶೂನ್ಯಸೂಚಕ;  // Reject when a throughout guard drops
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* threadFailCountp = ಶೂನ್ಯಸೂಚಕ;  // Number of threads rejected on this tick
    };

    // Disable condition also aborts attempts between clock events. A constant disable
    // never rises, and $sampled is unsupported in a sensitivity list.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isAsyncDisable(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ disableExprp) {
        ಹಿಂತಿರುಗಿ disableExprp && !VN_IS(disableExprp, Const)
               && !disableExprp->exists([](ಸ್ಥಿರಾಂಕ AstSampled*) { ಹಿಂತಿರುಗಿ ನಿಜ; });
    }

    // Emit the clocked update block of registers that hold pending attempts. The clearp
    // statements reset the same registers: a disable rising between clock events also
    // aborts pending attempts (IEEE 1800-2023 16.12), so clearp runs on that rising edge
    // in a separate block. It must clear unconditionally, as the disable may already be
    // false again when it runs, and the update block must not run on that edge, as it
    // would advance the attempts as if the clock had ticked. Blocks that hold no attempt
    // state, such as the $assertkill acknowledge in emitKillAckNba, do not go through here.
    // Callers pass a null clearp when the disable cannot rise between clock events.
    ಶೂನ್ಯ ಕೂಡಿಸು_State_Always(ಅಧೋ_Ctx& c, Ast_ಗಂಟು* bodyp, Ast_ಗಂಟು* clearp) {
        m_modp->ಕೂಡಿಸು_Stmtsp(
            ಹೊಸ AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), bodyp});
        ಆದರೆ (!clearp) ಹಿಂತಿರುಗಿ;
        m_modp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstAlways{
            c.flp, VAlwaysKwd::ALWAYS,
            ಹೊಸ Ast_Sen_ವೃಕ್ಷ{c.flp, ಹೊಸ Ast_Sen_ಅಂಶ{c.flp, VEdgeType::ET_POSEDGE,
                                                 c.disableExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)}},
            clearp});
    }

    // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
    // Phase 2: State register NBA always block. Each clocked-edge target
    // latches the OR of its incoming contributions.
    ಶೂನ್ಯ emit_State_ರಿಜಿಸ್ಟರ್_Nba(ಅಧೋ_Ctx& c) {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ asyncDisable = isAsyncDisable(c.disableExprp);
        Ast_ಗಂಟು* bodyp = ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು* clearp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtxr : c.ನಕ್ಷೆ.m_ನಕ್ಷೆ.vertices()) {
            ಸ್ಥಿರಾಂಕ Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = static_cast<ಸ್ಥಿರಾಂಕ Sva_State_ಶೃಂಗ*>(&vtxr);
            ಆದರೆ (!vtxp->datap()->stateVarp) ಮುಂದುವರಿ;

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಮುಂದಿನ_Statep = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& edger : vtxp->ಒಳ_ಅಂಚುಗಳು()) {
                ಸ್ಥಿರಾಂಕ Sva_Trans_ಅಂಚು& tedger = static_cast<ಸ್ಥಿರಾಂಕ Sva_Trans_ಅಂಚು&>(edger);
                ಆದರೆ (!tedger.m_consumes_ಚಕ್ರ) ಮುಂದುವರಿ;
                ಸ್ಥಿರಾಂಕ Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Clocked-edge source missing stateSig");

                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcSigp = fromVtxp->datap()->stateSigp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                srcSigp = andCond(c.flp, srcSigp, tedger.m_condp);

                ಆದರೆ (c.disableExprp) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ notDisp
                        = ಹೊಸ AstLogNot{c.flp, c.disableExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                    srcSigp = ಹೊಸ AstLogAnd{c.flp, srcSigp, notDisp};
                }
                ಮುಂದಿನ_Statep = orExprs(c.flp, ಮುಂದಿನ_Statep, srcSigp);
            }

            UASSERT_OBJ(ಮುಂದಿನ_Statep, vtxp,
                        "Registered vertex has no clocked incoming contribution");
            ಮುಂದಿನ_Statep = gateNotKill(c, ಮುಂದಿನ_Statep);

            Ast_ನಿಯೋಜಿಸು_Dly* ಸ್ಥಿರಾಂಕ assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{
                c.flp, ಹೊಸ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE}, ಮುಂದಿನ_Statep};
            bodyp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(bodyp, assignp);
            ಆದರೆ (asyncDisable) {
                clearp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(
                    clearp,
                    ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{
                        c.flp, ಹೊಸ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE},
                        ಹೊಸ AstConst{c.flp, AstConst::ಬಿಟ್_False{}}});
            }
        }
        ಆದರೆ (bodyp) ಕೂಡಿಸು_State_Always(c, bodyp, clearp);
    }

    // Phase 2b: In-place ring storage and registered ring metadata.
    ಶೂನ್ಯ emitDelayRingUpdates(ಅಧೋ_Ctx& c, ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಎಣಿಕೆ_Matches) {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ asyncDisable = isAsyncDisable(c.disableExprp);
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtxr : c.ನಕ್ಷೆ.m_ನಕ್ಷೆ.vertices()) {
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = static_cast<Sva_State_ಶೃಂಗ*>(&vtxr);
            ಆದರೆ (!vtxp->datap()->delayRingVarp) ಮುಂದುವರಿ;
            AstVar* ಸ್ಥಿರಾಂಕ idxp = vtxp->datap()->delayRingIdxVarp;
            AstVar* ಸ್ಥಿರಾಂಕ ಜೀವಂತ_ಎಣಿಕೆ_Varp = vtxp->datap()->delay_Ring_ಜೀವಂತ_ಎಣಿಕೆ_Varp;
            AstVar* ಸ್ಥಿರಾಂಕ wrappedp = vtxp->datap()->delayRingWrappedVarp;
            ಸ್ಥಿರಾಂಕ uint32_t ಗಾತ್ರ_2 = static_cast<uint32_t>(vtxp->m_delay_Ring_ಗಾತ್ರ);

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* incomingp = ಶೂನ್ಯಸೂಚಕ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* incomingOverflowp = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& edger : vtxp->ಒಳ_ಅಂಚುಗಳು()) {
                ++m_stat_Delay_Ring_ಅಂಚು_Visits;
                ಸ್ಥಿರಾಂಕ Sva_Trans_ಅಂಚು& tedger = static_cast<ಸ್ಥಿರಾಂಕ Sva_Trans_ಅಂಚು&>(edger);
                UASSERT_OBJ(tedger.m_consumes_ಚಕ್ರ == vtxp->m_isFixedDelayRing, vtxp,
                            "Delay-ring incoming edge kind mismatch");
                ಸ್ಥಿರಾಂಕ Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Delay-ring incoming source missing stateSig");
                UASSERT_OBJ(!ಎಣಿಕೆ_Matches || fromVtxp->datap()->state_ಎಣಿಕೆ_Sigp, fromVtxp,
                            "Delay-ring incoming source missing stateCountSig");
                // IEEE 1800-2023 16.14.1/16.14.3: assertion alternatives belong to one
                // evaluation; only cover sequence counts each end-of-match.
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* contribp
                    = ಎಣಿಕೆ_Matches
                          ? fromVtxp->datap()->state_ಎಣಿಕೆ_Sigp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)
                          : ಹೊಸ AstExtend{c.flp,
                                          fromVtxp->datap()->stateSigp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                                          m_u32DTypep->ಅಗಲ()};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* contribOverflowp
                    = ಎಣಿಕೆ_Matches && fromVtxp->datap()->state_ಎಣಿಕೆ_ಅತಿಪ್ರವಾಹ_Sigp
                          ? fromVtxp->datap()->state_ಎಣಿಕೆ_ಅತಿಪ್ರವಾಹ_Sigp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)
                          : ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (tedger.m_condp) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enablep = tedger.m_condp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                    contribp = ಎಣಿಕೆ_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, enablep)
                                   : ಕೂಡಿಸು_Countp(c.flp, ಶೂನ್ಯಸೂಚಕ, contribp, enablep);
                }
                ಆದರೆ (c.disableExprp) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ notDisp
                        = ಹೊಸ AstLogNot{c.flp, c.disableExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                    contribp = ಎಣಿಕೆ_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, notDisp)
                                   : ಕೂಡಿಸು_Countp(c.flp, ಶೂನ್ಯಸೂಚಕ, contribp, notDisp);
                }
                incomingp = ಎಣಿಕೆ_Matches ? ಕೂಡಿಸು_Checked_Countp(c.flp, incomingp, contribp,
                                                            incomingOverflowp, contribOverflowp)
                                         : ಕೂಡಿಸು_Countp(c.flp, incomingp, contribp);
            }
            UASSERT_OBJ(incomingp, vtxp, "Delay ring has no incoming edge");
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ storesCounts = vtxp->datap()->delayRingCounts;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ incomingCountp
                = storesCounts
                      ? incomingp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)
                      : ಹೊಸ AstExtend{c.flp,
                                      ಹೊಸ AstNeq{c.flp, incomingp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                                                 newTypedConstp(c.flp, incomingp->dtypep(), 0)},
                                      m_u32DTypep->ಅಗಲ()};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ storedIncomingp
                = storesCounts ? incomingp
                               : ಹೊಸ AstNeq{c.flp, incomingp,
                                            newTypedConstp(c.flp, incomingp->dtypep(), 0)};
            // Only this block accesses the storage. Other processes read the cached head, so
            // the ring can be updated in place without an NBA shadow copy.
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ writeIncomingp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                c.flp,
                delay_Ring_ಸ್ಥಾನ(c.flp, vtxp, ಹೊಸ AstVarRef{c.flp, idxp, VAccess::READ},
                              VAccess::WRITE),
                storedIncomingp};
            Ast_ಗಂಟು* updateBodyp = writeIncomingp;
            // live_count <= live_count - outgoing_count + incoming_count;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ outgoingp = delay_Ring_Output_ಎಣಿಕೆ(c.flp, vtxp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ afterOutgoingp
                = ಹೊಸ AstSub{c.flp, ಹೊಸ AstVarRef{c.flp, ಜೀವಂತ_ಎಣಿಕೆ_Varp, VAccess::READ}, outgoingp};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ oldAfterOutgoingp = afterOutgoingp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            Ast_ಕೂಡಿಸು* ಸ್ಥಿರಾಂಕ ಮುಂದಿನ_ಜೀವಂತ_Countp = ಹೊಸ Ast_ಕೂಡಿಸು{c.flp, afterOutgoingp, incomingCountp};
            ಆದರೆ (storesCounts) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಜೀವಂತ_Overflowp
                    = ಹೊಸ AstLt{c.flp, ಮುಂದಿನ_ಜೀವಂತ_Countp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), oldAfterOutgoingp};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ overflowp = orExprs(c.flp, incomingOverflowp, ಜೀವಂತ_Overflowp);
                updateBodyp = new_ಎಣಿಕೆ_ಅತಿಪ್ರವಾಹ_Warning(c.flp, overflowp);
                updateBodyp->ಕೂಡಿಸು_ಮುಂದಿನ(writeIncomingp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                VL_DO_DANGLING(oldAfterOutgoingp->delete_ವೃಕ್ಷ(), oldAfterOutgoingp);
            }
            // Cache the outgoing slot for the next index after the write. Reading after
            // writing also handles one-slot rings and two-slot range rings without alias cases.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಮುಂದಿನ_ತಲೆ_Idxp
                = ಮುಂದಿನ_Ring_ಸೂಚ್ಯಂಕ(c.flp, ಹೊಸ AstVarRef{c.flp, idxp, VAccess::READ}, ಗಾತ್ರ_2);
            ಆದರೆ (!vtxp->m_isFixedDelayRing) {
                ಮುಂದಿನ_ತಲೆ_Idxp = ಮುಂದಿನ_Ring_ಸೂಚ್ಯಂಕ(c.flp, ಮುಂದಿನ_ತಲೆ_Idxp, ಗಾತ್ರ_2);
            }
            updateBodyp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{
                c.flp, ಹೊಸ AstVarRef{c.flp, vtxp->datap()->delay_Ring_ತಲೆ_Varp, VAccess::WRITE},
                delay_Ring_ಸ್ಥಾನ(c.flp, vtxp, ಮುಂದಿನ_ತಲೆ_Idxp)});
            updateBodyp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{
                c.flp, ಹೊಸ AstVarRef{c.flp, ಜೀವಂತ_ಎಣಿಕೆ_Varp, VAccess::WRITE}, ಮುಂದಿನ_ಜೀವಂತ_Countp});
            // wrapped <= wrapped || idx == size - 1;
            updateBodyp->ಕೂಡಿಸು_ಮುಂದಿನ(
                ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{c.flp, ಹೊಸ AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                 ಹೊಸ AstLogOr{c.flp, ಹೊಸ AstVarRef{c.flp, wrappedp, VAccess::READ},
                                              delay_Ring_At_ಕೊನೆಯ_ಸೂಚ್ಯಂಕ(c.flp, idxp, ಗಾತ್ರ_2)}});
            // idx <= next_idx;
            updateBodyp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{
                c.flp, ಹೊಸ AstVarRef{c.flp, idxp, VAccess::WRITE},
                ಮುಂದಿನ_Ring_ಸೂಚ್ಯಂಕ(c.flp, ಹೊಸ AstVarRef{c.flp, idxp, VAccess::READ}, ಗಾತ್ರ_2)});
            ಆದರೆ (vtxp->m_delayRingAdvanceCondp) {
                updateBodyp = ಹೊಸ AstIf{
                    c.flp, sampled(vtxp->m_delayRingAdvanceCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)),
                    updateBodyp};
            }

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* clearCondp = kill_ಸಕ್ರಿಯ(c);
            ಆದರೆ (vtxp->m_delayRingClearCondp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_delayRingClearCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)));
            }
            ಆದರೆ (vtxp->m_abortClearp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_abortClearp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)));
            }
            ಆದರೆ (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* guardp = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ cp : vtxp->m_throughoutConds) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ sampledp = sampled(cp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
                guardp = guardp ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstLogAnd{c.flp, guardp, sampledp})
                                : sampledp;
            }
            ಆದರೆ (guardp) clearCondp = orExprs(c.flp, clearCondp, ಹೊಸ AstLogNot{c.flp, guardp});
            // Logically clear the ring without touching its wide storage.
            Ast_ನಿಯೋಜಿಸು_Dly* ಸ್ಥಿರಾಂಕ clearCountp
                = ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{c.flp, ಹೊಸ AstVarRef{c.flp, ಜೀವಂತ_ಎಣಿಕೆ_Varp, VAccess::WRITE},
                                   newTypedConstp(c.flp, ಜೀವಂತ_ಎಣಿಕೆ_Varp->dtypep(), 0)};
            clearCountp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{c.flp,
                                                  ಹೊಸ AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                                  ಹೊಸ AstConst{c.flp, AstConst::ಬಿಟ್_False{}}});
            clearCountp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{c.flp,
                                                  ಹೊಸ AstVarRef{c.flp, idxp, VAccess::WRITE},
                                                  newTypedConstp(c.flp, idxp->dtypep(), 0)});
            updateBodyp = ಹೊಸ AstIf{c.flp, clearCondp, clearCountp, updateBodyp};

            ಕೂಡಿಸು_State_Always(c, updateBodyp, asyncDisable ? clearCountp->clone_ವೃಕ್ಷ(ನಿಜ) : ಶೂನ್ಯಸೂಚಕ);
        }
    }

    // Phase 2c: SAnd combiner done-latch always block.
    // NBA semantics ensure doneL/doneR read pre-update values (IEEE 16.9.5).
    ಶೂನ್ಯ emit_And_Combiner_ಮುಗಿದ_Latch_Nba(ಅಧೋ_Ctx& c) {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ asyncDisable = isAsyncDisable(c.disableExprp);
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtxr : c.ನಕ್ಷೆ.m_ನಕ್ಷೆ.vertices()) {
            ಸ್ಥಿರಾಂಕ Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ avp = static_cast<ಸ್ಥಿರಾಂಕ Sva_State_ಶೃಂಗ*>(&vtxr);
            ಆದರೆ (!avp->datap()->ಮುಗಿದ_LVarp) ಮುಂದುವರಿ;
            // doneLVars is non-null only for AndCombiner vertices, which always
            // have both m_andLhsTermp and m_andRhsTermp set at build time.
            UASSERT_OBJ(avp->m_andLhsTermp && avp->m_andRhsTermp, avp,
                        "AndCombiner vertex missing LHS/RHS terminal");
            ಸ್ಥಿರಾಂಕ Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsTermp = avp->m_andLhsTermp;
            ಸ್ಥಿರಾಂಕ Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsTermp = avp->m_andRhsTermp;
            // resolveLinks' 2*N+2 fixed-point pass is guaranteed to populate
            // stateSigp on every AndCombiner and its LHS/RHS terminals.
            UASSERT_OBJ(lhsTermp->datap()->stateSigp && rhsTermp->datap()->stateSigp
                            && avp->datap()->stateSigp,
                        avp, "AndCombiner stateSigp chain unresolved after resolveLinks");

            Ast_ನಿಯೋಜಿಸು_Dly* ಸ್ಥಿರಾಂಕ clearLp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{
                c.flp, ಹೊಸ AstVarRef{c.flp, avp->datap()->ಮುಗಿದ_LVarp, VAccess::WRITE},
                ಹೊಸ AstConst{c.flp, AstConst::ಬಿಟ್_False{}}};
            Ast_ನಿಯೋಜಿಸು_Dly* ಸ್ಥಿರಾಂಕ clearRp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{
                c.flp, ಹೊಸ AstVarRef{c.flp, avp->datap()->ಮುಗಿದ_RVarp, VAccess::WRITE},
                ಹೊಸ AstConst{c.flp, AstConst::ಬಿಟ್_False{}}};
            clearLp->ಕೂಡಿಸು_ಮುಂದಿನ(clearRp);

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಹೊಂದಿಕೆ_LNowp
                = build_ಹೊಂದಿಕೆ_Now(c.flp, lhsTermp->datap()->stateSigp, avp->m_andLhsCondp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಹೊಂದಿಕೆ_RNowp
                = build_ಹೊಂದಿಕೆ_Now(c.flp, rhsTermp->datap()->stateSigp, avp->m_andRhsCondp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* gateLp = ಹೊಂದಿಕೆ_LNowp;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* gateRp = ಹೊಂದಿಕೆ_RNowp;
            ಆದರೆ (c.disableExprp) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ notDisLp
                    = ಹೊಸ AstLogNot{c.flp, c.disableExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                gateLp = ಹೊಸ AstLogAnd{c.flp, gateLp, notDisLp};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ notDisRp
                    = ಹೊಸ AstLogNot{c.flp, c.disableExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                gateRp = ಹೊಸ AstLogAnd{c.flp, gateRp, notDisRp};
            }
            Ast_ನಿಯೋಜಿಸು_Dly* ಸ್ಥಿರಾಂಕ ಗಣ_Lp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{
                c.flp, ಹೊಸ AstVarRef{c.flp, avp->datap()->ಮುಗಿದ_LVarp, VAccess::WRITE},
                ಹೊಸ AstConst{c.flp, AstConst::ಬಿಟ್_True{}}};
            AstIf* ಸ್ಥಿರಾಂಕ ಗಣ_LIfp = ಹೊಸ AstIf{c.flp, gateLp, ಗಣ_Lp, ಶೂನ್ಯಸೂಚಕ};
            Ast_ನಿಯೋಜಿಸು_Dly* ಸ್ಥಿರಾಂಕ ಗಣ_Rp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{
                c.flp, ಹೊಸ AstVarRef{c.flp, avp->datap()->ಮುಗಿದ_RVarp, VAccess::WRITE},
                ಹೊಸ AstConst{c.flp, AstConst::ಬಿಟ್_True{}}};
            AstIf* ಸ್ಥಿರಾಂಕ ಗಣ_RIfp = ಹೊಸ AstIf{c.flp, gateRp, ಗಣ_Rp, ಶೂನ್ಯಸೂಚಕ};
            ಗಣ_LIfp->ಕೂಡಿಸು_ಮುಂದಿನ(ಗಣ_RIfp);

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* clearCondp = ಹೊಸ AstLogOr{c.flp, kill_ಸಕ್ರಿಯ(c),
                                                   avp->datap()->stateSigp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
            ಆದರೆ (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
            }
            AstIf* ಸ್ಥಿರಾಂಕ topp = ಹೊಸ AstIf{c.flp, clearCondp, clearLp, ಗಣ_LIfp};
            ಕೂಡಿಸು_State_Always(c, topp, asyncDisable ? clearLp->clone_ವೃಕ್ಷ(ನಿಜ) : ಶೂನ್ಯಸೂಚಕ);
        }
    }

    ಶೂನ್ಯ emitKillAckNba(ಅಧೋ_Ctx& c) {
        Ast_ನಿಯೋಜಿಸು_Dly* ಸ್ಥಿರಾಂಕ ackp
            = ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{c.flp, ಹೊಸ AstVarRef{c.flp, c.killVarp, VAccess::WRITE},
                               assertKillGet(c.flp, c.assertType, c.directiveType)};
        m_modp->ಕೂಡಿಸು_Stmtsp(
            ಹೊಸ AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), ackp});
    }

    // Phase 3/3a/3b: Compute terminal match/reject signals, required-step reject,
    // throughout-drop reject; clean up intermediate state signals.
    // Phase 3: terminalActive and rejectBase from Links to matchVertex.
    // Builder only adds Links (non-clocked) to matchVertex via addLink in
    // wireMatchAndMidSources. For cover sequence, also count each end-of-match
    // so the action can be replayed without unrolling endpoints.
    ಶೂನ್ಯ compute_Terminal_ಹೊಂದಿಕೆ_And_Reject(ಅಧೋ_Ctx& c, Signal_ಗಣ& sigs, ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ need_ಹೊಂದಿಕೆ_ಎಣಿಕೆ) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Sva_Trans_ಅಂಚು* ಸ್ಥಿರಾಂಕ tedgep : c.ನಕ್ಷೆ.all_ಅಂಚುಗಳು()) {
            ಆದರೆ (tedgep->ಗಮ್ಯ_Vtxp() != c.ನಕ್ಷೆ.m_ಹೊಂದಿಕೆ_Vertexp) ಮುಂದುವರಿ;
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                        "Terminal-link source missing stateSig");

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcSigp = fromVtxp->datap()->stateSigp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            srcSigp = andCond(c.flp, srcSigp, tedgep->m_condp);
            ಆದರೆ (need_ಹೊಂದಿಕೆ_ಎಣಿಕೆ) {
                UASSERT_OBJ(fromVtxp->datap()->state_ಎಣಿಕೆ_Sigp, fromVtxp,
                            "Terminal-link source missing stateCountSig");
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* contributionp
                    = fromVtxp->datap()->state_ಎಣಿಕೆ_Sigp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* contributionOverflowp
                    = fromVtxp->datap()->state_ಎಣಿಕೆ_ಅತಿಪ್ರವಾಹ_Sigp
                          ? fromVtxp->datap()->state_ಎಣಿಕೆ_ಅತಿಪ್ರವಾಹ_Sigp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)
                          : ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (tedgep->m_condp) {
                    contributionp = gateCheckedCountp(c.flp, contributionp, contributionOverflowp,
                                                      tedgep->m_condp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
                }
                sigs.ಹೊಂದಿಕೆ_Countp
                    = ಕೂಡಿಸು_Checked_Countp(c.flp, sigs.ಹೊಂದಿಕೆ_Countp, contributionp,
                                       sigs.ಹೊಂದಿಕೆ_ಎಣಿಕೆ_Overflowp, contributionOverflowp);
            }

            ಆದರೆ (fromVtxp->m_delay_Ring_ಗಾತ್ರ && !fromVtxp->m_isFixedDelayRing) {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
                // reject |= (wrapped || idx == size - 1) && head && final_condition;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* expireContribp = delay_Ring_Output_ಎಣಿಕೆ(c.flp, fromVtxp);
                expireContribp = andCond(c.flp, expireContribp, tedgep->m_condp);
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, expireContribp);
                VL_DO_DANGLING(srcSigp->delete_ವೃಕ್ಷ(), srcSigp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (fromVtxp->m_isUnbounded || fromVtxp->m_isAndCombiner) {
                sigs.terminalActivep = orExprs(c.flp, sigs.terminalActivep, srcSigp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, srcSigp);
            }
        }
        // wireMatchAndMidSources always adds a Link from result.termVertexp
        // to m_matchVertexp, so the loop above always sets terminalActivep.
        UASSERT_OBJ(sigs.terminalActivep, c.ನಕ್ಷೆ.m_ಹೊಂದಿಕೆ_Vertexp,
                    "No terminal edge to match vertex");
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newThroughoutThreadFailCountp(ಅಧೋ_Ctx& c, AstVar* ಸ್ಥಿರಾಂಕ delay_Ring_ಜೀವಂತ_ಎಣಿಕೆ_Varp,
                                               Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ stateExprp,
                                               Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enablep) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಸಕ್ರಿಯ_Thread_Countp
            = delay_Ring_ಜೀವಂತ_ಎಣಿಕೆ_Varp
                  ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                        ಹೊಸ AstVarRef{c.flp, delay_Ring_ಜೀವಂತ_ಎಣಿಕೆ_Varp, VAccess::READ})
                  : ಹೊಸ AstExtend{c.flp, stateExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), m_u32DTypep->ಅಗಲ()};
        ಹಿಂತಿರುಗಿ ಕೂಡಿಸು_Countp(c.flp, ಶೂನ್ಯಸೂಚಕ, ಸಕ್ರಿಯ_Thread_Countp, enablep);
    }

    // Phase 3b: Throughout-drop and ring-wide abort rejection.
    ಶೂನ್ಯ computeThroughoutReject(ಅಧೋ_Ctx& c, Signal_ಗಣ& sigs, ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ need_Thread_Fail_ಎಣಿಕೆ) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtxr : c.ನಕ್ಷೆ.m_ನಕ್ಷೆ.vertices()) {
            ಸ್ಥಿರಾಂಕ Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = static_cast<ಸ್ಥಿರಾಂಕ Sva_State_ಶೃಂಗ*>(&vtxr);
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& conds = vtxp->m_throughoutConds;
            ಆದರೆ (conds.ಖಾಲಿಗಳು() && !vtxp->m_abortRejectp) ಮುಂದುವರಿ;
            ಆದರೆ (vtxp->m_isAndCombiner) ಮುಂದುವರಿ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* stateExprp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (vtxp->datap()->stateVarp) {
                stateExprp = ಹೊಸ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (vtxp->datap()->delayRingVarp && vtxp->m_isFixedDelayRing) {
                stateExprp
                    = delay_Ring_Has_ಜೀವಂತ_Threadsp(c.flp, vtxp->datap()->delay_Ring_ಜೀವಂತ_ಎಣಿಕೆ_Varp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                UASSERT_OBJ(vtxp->datap()->stateSigp, vtxp,
                            "Throughout-conds vertex missing state representation");
                stateExprp = vtxp->datap()->stateSigp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* guardp = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ cp : conds) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ sp = sampled(cp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
                guardp = guardp ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstLogAnd{c.flp, guardp, sp}) : sp;
            }
            ಆದರೆ (vtxp->m_abortRejectp) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ notAbortp
                    = ಹೊಸ AstLogNot{c.flp, sampled(vtxp->m_abortRejectp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು))};
                guardp = guardp
                             ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstLogAnd{c.flp, guardp, notAbortp})
                             : notAbortp;
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rejectCondp = ಹೊಸ AstLogNot{c.flp, guardp};
            ಆದರೆ (vtxp->m_abortClearp) {
                // Any abort clears the ring. Accept aborts suppress a simultaneous guard failure;
                // reject aborts are restored below so they still force rejection.
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ notAbortClearp
                    = ಹೊಸ AstLogNot{c.flp, sampled(vtxp->m_abortClearp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು))};
                rejectCondp = ಹೊಸ AstLogAnd{c.flp, rejectCondp, notAbortClearp};
                ಆದರೆ (vtxp->m_abortRejectp) {
                    rejectCondp = ಹೊಸ AstLogOr{
                        c.flp, rejectCondp, sampled(vtxp->m_abortRejectp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು))};
                }
            }
            ಆದರೆ (need_Thread_Fail_ಎಣಿಕೆ) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ contributionp
                    = newThroughoutThreadFailCountp(c, vtxp->datap()->delay_Ring_ಜೀವಂತ_ಎಣಿಕೆ_Varp,
                                                    stateExprp, rejectCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
                sigs.threadFailCountp = ಕೂಡಿಸು_Countp(c.flp, sigs.threadFailCountp, contributionp);
                ಆದರೆ (vtxp->m_abortRejectp && vtxp->m_delayRingAdvanceCondp) {
                    // An advancing ring thread also occupies its same-tick match vertex in the
                    // unrolled NFA, so abort rejection must replay both fail actions.
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ abortAndAdvancep = ಹೊಸ AstLogAnd{
                        c.flp, sampled(vtxp->m_abortRejectp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)),
                        sampled(vtxp->m_delayRingAdvanceCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು))};
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಹೊಂದಿಕೆ_Contributionp = newThroughoutThreadFailCountp(
                        c, vtxp->datap()->delay_Ring_ಜೀವಂತ_ಎಣಿಕೆ_Varp, stateExprp, abortAndAdvancep);
                    sigs.threadFailCountp
                        = ಕೂಡಿಸು_Countp(c.flp, sigs.threadFailCountp, ಹೊಂದಿಕೆ_Contributionp);
                }
            }
            sigs.throughoutRejectp = orExprs(c.flp, sigs.throughoutRejectp,
                                             ಹೊಸ AstLogAnd{c.flp, stateExprp, rejectCondp});
        }
    }

    Signal_ಗಣ compute_ಸಂಕೇತಗಳು(ಅಧೋ_Ctx& c, ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ need_Thread_Fail_ಎಣಿಕೆ,
                             ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ need_Throughout_Thread_Fail_ಎಣಿಕೆ, ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ need_ಹೊಂದಿಕೆ_ಎಣಿಕೆ) {
        Signal_ಗಣ sigs;

        compute_Terminal_ಹೊಂದಿಕೆ_And_Reject(c, sigs, need_ಹೊಂದಿಕೆ_ಎಣಿಕೆ);

        // Phase 3a: required-step rejection.
        // Builder only sets m_rejectOnFail on non-clocked Links with m_condp
        // or m_condVtxp, and the source always has a resolved stateSig.
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Sva_Trans_ಅಂಚು* ಸ್ಥಿರಾಂಕ tedgep : c.ನಕ್ಷೆ.all_ಅಂಚುಗಳು()) {
            ಆದರೆ (!tedgep->m_rejectOnFail) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp && (tedgep->m_condp || tedgep->m_condVtxp),
                        fromVtxp,
                        "rejectOnFail Link must have condp/condVtxp and source stateSig");
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ srcSigp = fromVtxp->datap()->stateSigp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (tedgep->m_condVtxp) {
                UASSERT_OBJ(tedgep->m_condVtxp->datap()->stateSigp, tedgep->m_condVtxp,
                            "rejectOnFail condVtxp missing stateSig");
                condp = tedgep->m_condVtxp->datap()->stateSigp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                ಆದರೆ (tedgep->m_condp) {
                    condp = ಹೊಸ AstLogOr{c.flp, condp, tedgep->m_condp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                condp = tedgep->m_condp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ notCondp = ಹೊಸ AstLogNot{c.flp, condp};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rawFailp = ಹೊಸ AstLogAnd{c.flp, srcSigp, notCondp};
            ಆದರೆ (need_Thread_Fail_ಎಣಿಕೆ) {
                // thread_fail_count += fail;
                sigs.threadFailCountp = ಕೂಡಿಸು_Countp(
                    c.flp, sigs.threadFailCountp,
                    ಹೊಸ AstExtend{c.flp, rawFailp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), m_u32DTypep->ಅಗಲ()});
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ failp = gateNotKill(c, rawFailp);
            sigs.required_ಹೆಜ್ಜೆ_Rejectp = orExprs(c.flp, sigs.required_ಹೆಜ್ಜೆ_Rejectp, failp);
        }

        computeThroughoutReject(c, sigs, need_Throughout_Thread_Fail_ಎಣಿಕೆ);
        ಆದರೆ (sigs.threadFailCountp) {
            sigs.threadFailCountp
                = ಕೂಡಿಸು_Countp(c.flp, ಶೂನ್ಯಸೂಚಕ, sigs.threadFailCountp, not_Kill_ಸಕ್ರಿಯ(c));
        }
        ಆದರೆ (sigs.ಹೊಂದಿಕೆ_Countp) {
            ಆದರೆ (c.ಹೊಂದಿಕೆ_Condp) {
                sigs.ಹೊಂದಿಕೆ_Countp
                    = gateCheckedCountp(c.flp, sigs.ಹೊಂದಿಕೆ_Countp, sigs.ಹೊಂದಿಕೆ_ಎಣಿಕೆ_Overflowp,
                                        sampled(c.ಹೊಂದಿಕೆ_Condp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)));
            }
            sigs.ಹೊಂದಿಕೆ_Countp = gateCheckedCountp(c.flp, sigs.ಹೊಂದಿಕೆ_Countp, sigs.ಹೊಂದಿಕೆ_ಎಣಿಕೆ_Overflowp,
                                                 not_Kill_ಸಕ್ರಿಯ(c));
        }
        sigs.terminalActivep = gateNotKill(c, sigs.terminalActivep);
        sigs.rejectBasep = gateNotKill(c, sigs.rejectBasep);
        sigs.throughoutRejectp = gateNotKill(c, sigs.throughoutRejectp);

        // Clean up intermediate state signals. These are orphan subtrees
        // (never linked into the enclosing AST); deleteTree() is immediate
        // which is what we want since stateSigp lifetime ends with this scope.
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtxr : c.ನಕ್ಷೆ.m_ನಕ್ಷೆ.vertices()) {
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = static_cast<Sva_State_ಶೃಂಗ*>(&vtxr);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*& sigp = vtxp->datap()->stateSigp;
            ಆದರೆ (sigp) VL_DO_DANGLING(sigp->delete_ವೃಕ್ಷ(), sigp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*& countp = vtxp->datap()->state_ಎಣಿಕೆ_Sigp;
            ಆದರೆ (countp) VL_DO_DANGLING(countp->delete_ವೃಕ್ಷ(), countp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*& overflowp = vtxp->datap()->state_ಎಣಿಕೆ_ಅತಿಪ್ರವಾಹ_Sigp;
            ಆದರೆ (overflowp) VL_DO_DANGLING(overflowp->delete_ವೃಕ್ಷ(), overflowp);
        }
        // Disable iff gating (IEEE 1800-2023 16.12) on the current level value.
        ಆದರೆ (c.disableExprp) {
            // terminalActivep is always set, so gate it unconditionally.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ notTermp
                = ಹೊಸ AstLogNot{c.flp, c.disableExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
            sigs.terminalActivep = ಹೊಸ AstLogAnd{c.flp, sigs.terminalActivep, notTermp};
            ಆದರೆ (sigs.rejectBasep) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ notDisp
                    = ಹೊಸ AstLogNot{c.flp, c.disableExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                sigs.rejectBasep = ಹೊಸ AstLogAnd{c.flp, sigs.rejectBasep, notDisp};
            }
            ಆದರೆ (sigs.throughoutRejectp) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ notDisp
                    = ಹೊಸ AstLogNot{c.flp, c.disableExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                sigs.throughoutRejectp = ಹೊಸ AstLogAnd{c.flp, sigs.throughoutRejectp, notDisp};
            }
            ಆದರೆ (sigs.required_ಹೆಜ್ಜೆ_Rejectp) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ notDisp
                    = ಹೊಸ AstLogNot{c.flp, c.disableExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                sigs.required_ಹೆಜ್ಜೆ_Rejectp = ಹೊಸ AstLogAnd{c.flp, sigs.required_ಹೆಜ್ಜೆ_Rejectp, notDisp};
            }
            ಆದರೆ (sigs.ಹೊಂದಿಕೆ_Countp) {
                sigs.ಹೊಂದಿಕೆ_Countp = gateCheckedCountp(
                    c.flp, sigs.ಹೊಂದಿಕೆ_Countp, sigs.ಹೊಂದಿಕೆ_ಎಣಿಕೆ_Overflowp,
                    ಹೊಸ AstLogNot{c.flp, c.disableExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)});
            }
        }

        ಆದರೆ (sigs.ಹೊಂದಿಕೆ_ಎಣಿಕೆ_Overflowp) {
            m_modp->ಕೂಡಿಸು_Stmtsp(
                ಹೊಸ AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು),
                              new_ಎಣಿಕೆ_ಅತಿಪ್ರವಾಹ_Warning(c.flp, sigs.ಹೊಂದಿಕೆ_ಎಣಿಕೆ_Overflowp)});
            sigs.ಹೊಂದಿಕೆ_ಎಣಿಕೆ_Overflowp = ಶೂನ್ಯಸೂಚಕ;
        }

        ಹಿಂತಿರುಗಿ sigs;
    }

    // Phase 1: Resolve combinational Links via fixed-point propagation.
    ಶೂನ್ಯ resolveLinks(ಅಧೋ_Ctx& c, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* triggerExprp, ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಎಣಿಕೆ_Matches) {
        // datap() was freshly allocated in lower() -- all state signals start null.
        c.ನಕ್ಷೆ.m_ಆರಂಭ_Vertexp->datap()->stateSigp = triggerExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        c.ನಕ್ಷೆ.m_ಆರಂಭ_Vertexp->datap()->state_ಎಣಿಕೆ_Sigp
            = ಹೊಸ AstExtend{c.flp, triggerExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), m_u32DTypep->ಅಗಲ()};
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtxr : c.ನಕ್ಷೆ.m_ನಕ್ಷೆ.vertices()) {
            Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = static_cast<Sva_State_ಶೃಂಗ*>(&vtxr);
            ಆದರೆ (vtxp->datap()->stateVarp) {
                vtxp->datap()->stateSigp
                    = ಹೊಸ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
                vtxp->datap()->state_ಎಣಿಕೆ_Sigp = ಹೊಸ AstExtend{
                    c.flp, ಹೊಸ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ},
                    m_u32DTypep->ಅಗಲ()};
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (vtxp->datap()->delayRingVarp) {
                ಆದರೆ (vtxp->m_isFixedDelayRing) {
                    // state_count = wrapped ? head : 0; state = state_count != 0;
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ countp = delay_Ring_Output_ಎಣಿಕೆ(c.flp, vtxp);
                    vtxp->datap()->stateSigp = ಹೊಸ AstNeq{c.flp, countp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                                                          newTypedConstp(c.flp, m_u32DTypep, 0)};
                    vtxp->datap()->state_ಎಣಿಕೆ_Sigp = countp;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    vtxp->datap()->stateSigp
                        = delay_Ring_Has_ಜೀವಂತ_Threadsp(c.flp, vtxp->datap()->delay_Ring_ಜೀವಂತ_ಎಣಿಕೆ_Varp);
                    vtxp->datap()->state_ಎಣಿಕೆ_Sigp = ಹೊಸ AstVarRef{
                        c.flp, vtxp->datap()->delay_Ring_ಜೀವಂತ_ಎಣಿಕೆ_Varp, VAccess::READ};
                }
            }
        }
        // Fixed-point propagation along zero-delay (Link) edges. Rebuild each
        // derived signal from its incoming edges on every pass; appending the same
        // contributions repeatedly makes merge expressions grow exponentially.
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t passes = 2 * c.ನಕ್ಷೆ.m_ನಕ್ಷೆ.vertices().ಗಾತ್ರ_2() + 2;
        ಗಾಗಿ (ಗಾತ್ರ_t pass = 0; pass < passes; ++pass) {
            // Rebuild SAnd combiners once both sub-NFA terminals are available.
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtxr : c.ನಕ್ಷೆ.m_ನಕ್ಷೆ.vertices()) {
                Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = static_cast<Sva_State_ಶೃಂಗ*>(&vtxr);
                ಆದರೆ (!vtxp->m_isAndCombiner) ಮುಂದುವರಿ;
                // AndCombiner vertices always have both terminal pointers set.
                UASSERT_OBJ(vtxp->m_andLhsTermp && vtxp->m_andRhsTermp, vtxp,
                            "AndCombiner vertex missing LHS/RHS terminal");
                ಸ್ಥಿರಾಂಕ Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsTermp = vtxp->m_andLhsTermp;
                ಸ್ಥಿರಾಂಕ Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsTermp = vtxp->m_andRhsTermp;
                ಆದರೆ (!lhsTermp->datap()->stateSigp || !rhsTermp->datap()->stateSigp) ಮುಂದುವರಿ;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಹೊಂದಿಕೆ_Lp
                    = build_ಹೊಂದಿಕೆ_Now(c.flp, lhsTermp->datap()->stateSigp, vtxp->m_andLhsCondp);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಹೊಂದಿಕೆ_Rp
                    = build_ಹೊಂದಿಕೆ_Now(c.flp, rhsTermp->datap()->stateSigp, vtxp->m_andRhsCondp);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಮುಗಿದ_LOrp = ಹೊಸ AstLogOr{
                    c.flp, ಹೊಸ AstVarRef{c.flp, vtxp->datap()->ಮುಗಿದ_LVarp, VAccess::READ}, ಹೊಂದಿಕೆ_Lp};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಮುಗಿದ_ROrp = ಹೊಸ AstLogOr{
                    c.flp, ಹೊಸ AstVarRef{c.flp, vtxp->datap()->ಮುಗಿದ_RVarp, VAccess::READ}, ಹೊಂದಿಕೆ_Rp};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ bothp = ಹೊಸ AstLogAnd{c.flp, ಮುಗಿದ_LOrp, ಮುಗಿದ_ROrp};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ oneNowp = ಹೊಸ AstLogOr{c.flp, ಹೊಂದಿಕೆ_Lp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                                                          ಹೊಂದಿಕೆ_Rp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                ಆದರೆ (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_ವೃಕ್ಷ(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = ಹೊಸ AstLogAnd{c.flp, bothp, oneNowp};
                ಆದರೆ (vtxp->datap()->state_ಎಣಿಕೆ_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_ಎಣಿಕೆ_Sigp->delete_ವೃಕ್ಷ(),
                                   vtxp->datap()->state_ಎಣಿಕೆ_Sigp);
                }
                vtxp->datap()->state_ಎಣಿಕೆ_Sigp = ಹೊಸ AstExtend{
                    c.flp, vtxp->datap()->stateSigp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), m_u32DTypep->ಅಗಲ()};
            }

            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtxr : c.ನಕ್ಷೆ.m_ನಕ್ಷೆ.vertices()) {
                Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = static_cast<Sva_State_ಶೃಂಗ*>(&vtxr);
                ಆದರೆ (vtxp == c.ನಕ್ಷೆ.m_ಆರಂಭ_Vertexp || vtxp->datap()->stateVarp
                    || vtxp->datap()->delayRingVarp || vtxp->m_isAndCombiner || vtxp->m_is_ಹೊಂದಿಕೆ
                    || vtxp->m_isRejectSink) {
                    ಮುಂದುವರಿ;
                }
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಮುಂದಿನ_Statep = ಶೂನ್ಯಸೂಚಕ;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಮುಂದಿನ_State_Countp = ಶೂನ್ಯಸೂಚಕ;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಮುಂದಿನ_State_ಎಣಿಕೆ_Overflowp = ಶೂನ್ಯಸೂಚಕ;
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& er : vtxp->ಒಳ_ಅಂಚುಗಳು()) {
                    ಸ್ಥಿರಾಂಕ Sva_Trans_ಅಂಚು& te = static_cast<ಸ್ಥಿರಾಂಕ Sva_Trans_ಅಂಚು&>(er);
                    ಸ್ಥಿರಾಂಕ Sva_State_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fromVtxp = te.fromVtxp();
                    ಆದರೆ (!fromVtxp->datap()->stateSigp) ಮುಂದುವರಿ;
                    UASSERT_OBJ(fromVtxp->datap()->state_ಎಣಿಕೆ_Sigp, fromVtxp,
                                "Link source missing stateCountSig");
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ contributionp = andCond(
                        c.flp, fromVtxp->datap()->stateSigp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), te.m_condp);
                    ಮುಂದಿನ_Statep = orExprs(c.flp, ಮುಂದಿನ_Statep, contributionp);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಎಣಿಕೆ_Contributionp
                        = fromVtxp->datap()->state_ಎಣಿಕೆ_Sigp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಎಣಿಕೆ_Contribution_Overflowp
                        = ಎಣಿಕೆ_Matches && fromVtxp->datap()->state_ಎಣಿಕೆ_ಅತಿಪ್ರವಾಹ_Sigp
                              ? fromVtxp->datap()->state_ಎಣಿಕೆ_ಅತಿಪ್ರವಾಹ_Sigp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)
                              : ಶೂನ್ಯಸೂಚಕ;
                    ಆದರೆ (te.m_condp) {
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ enablep = te.m_condp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                        ಎಣಿಕೆ_Contributionp
                            = ಎಣಿಕೆ_Matches
                                  ? gateCheckedCountp(c.flp, ಎಣಿಕೆ_Contributionp,
                                                      ಎಣಿಕೆ_Contribution_Overflowp, enablep)
                                  : ಕೂಡಿಸು_Countp(c.flp, ಶೂನ್ಯಸೂಚಕ, ಎಣಿಕೆ_Contributionp, enablep);
                    }
                    ಮುಂದಿನ_State_Countp
                        = ಎಣಿಕೆ_Matches
                              ? ಕೂಡಿಸು_Checked_Countp(c.flp, ಮುಂದಿನ_State_Countp, ಎಣಿಕೆ_Contributionp,
                                                 ಮುಂದಿನ_State_ಎಣಿಕೆ_Overflowp,
                                                 ಎಣಿಕೆ_Contribution_Overflowp)
                              : ಕೂಡಿಸು_Countp(c.flp, ಮುಂದಿನ_State_Countp, ಎಣಿಕೆ_Contributionp);
                }
                ಆದರೆ (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_ವೃಕ್ಷ(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = ಮುಂದಿನ_Statep;
                ಆದರೆ (vtxp->datap()->state_ಎಣಿಕೆ_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_ಎಣಿಕೆ_Sigp->delete_ವೃಕ್ಷ(),
                                   vtxp->datap()->state_ಎಣಿಕೆ_Sigp);
                }
                vtxp->datap()->state_ಎಣಿಕೆ_Sigp = ಮುಂದಿನ_State_Countp;
                ಆದರೆ (vtxp->datap()->state_ಎಣಿಕೆ_ಅತಿಪ್ರವಾಹ_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_ಎಣಿಕೆ_ಅತಿಪ್ರವಾಹ_Sigp->delete_ವೃಕ್ಷ(),
                                   vtxp->datap()->state_ಎಣಿಕೆ_ಅತಿಪ್ರವಾಹ_Sigp);
                }
                vtxp->datap()->state_ಎಣಿಕೆ_ಅತಿಪ್ರವಾಹ_Sigp = ಮುಂದಿನ_State_ಎಣಿಕೆ_Overflowp;
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    // Combine terminal/reject signals into final output expression.
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* assemble_ಫಲಿತಾಂಶ(Ast_ಗಂಟು_Cover_Or_Assert* ಸ್ಥಿರಾಂಕ assertp, ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ negated,
                                       Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಹೊಂದಿಕೆ_Condp, ಸ್ಥಿರಾಂಕ Signal_ಗಣ& sigs,
                                       Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ** ಸ್ಥಿರಾಂಕ ಹೊರಹೋಗು_Matchpp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = assertp->fileline();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isCover = VN_IS(assertp, Cover);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ terminalActivep = sigs.terminalActivep;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rejectBasep = sigs.rejectBasep;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ throughoutRejectp = sigs.throughoutRejectp;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ required_ಹೆಜ್ಜೆ_Rejectp = sigs.required_ಹೆಜ್ಜೆ_Rejectp;
        // Property negation (IEEE 1800-2023 16.12.1 `not`): invert match/reject.
        ಆದರೆ (negated) {
            ಆದರೆ (isCover) {
                ಆದರೆ (terminalActivep)
                    VL_DO_DANGLING(terminalActivep->delete_ವೃಕ್ಷ(), terminalActivep);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* negRejectp = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (ಹೊಂದಿಕೆ_Condp && rejectBasep) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ sampledCondp = sampled(ಹೊಂದಿಕೆ_Condp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ notCondp = ಹೊಸ AstLogNot{flp, sampledCondp};
                    negRejectp = ಹೊಸ AstLogAnd{flp, rejectBasep, notCondp};
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (rejectBasep) {
                    VL_DO_DANGLING(rejectBasep->delete_ವೃಕ್ಷ(), rejectBasep);
                }
                ಆದರೆ (throughoutRejectp) negRejectp = orExprs(flp, negRejectp, throughoutRejectp);
                ಆದರೆ (required_ಹೆಜ್ಜೆ_Rejectp)
                    negRejectp = orExprs(flp, negRejectp, required_ಹೆಜ್ಜೆ_Rejectp);
                ಹಿಂತಿರುಗಿ negRejectp ? negRejectp : ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}};
            }
            // Negated assert/assume: output = !match.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* matchp = terminalActivep;
            ಆದರೆ (ಹೊಂದಿಕೆ_Condp) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ sampledCondp = sampled(ಹೊಂದಿಕೆ_Condp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
                matchp = ಹೊಸ AstLogAnd{flp, matchp, sampledCondp};
            }
            ಆದರೆ (ಹೊರಹೋಗು_Matchpp) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* notPMatchp = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (ಹೊಂದಿಕೆ_Condp && rejectBasep) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ sampledCondp = sampled(ಹೊಂದಿಕೆ_Condp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
                    notPMatchp = ಹೊಸ AstLogAnd{flp, rejectBasep->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                                               ಹೊಸ AstLogNot{flp, sampledCondp}};
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (rejectBasep) {
                    notPMatchp = rejectBasep->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                }
                ಆದರೆ (throughoutRejectp)
                    notPMatchp = orExprs(flp, notPMatchp, throughoutRejectp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
                ಆದರೆ (required_ಹೆಜ್ಜೆ_Rejectp)
                    notPMatchp
                        = orExprs(flp, notPMatchp, required_ಹೆಜ್ಜೆ_Rejectp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
                *ಹೊರಹೋಗು_Matchpp = notPMatchp;
            }
            ಆದರೆ (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_ವೃಕ್ಷ(), throughoutRejectp);
            ಆದರೆ (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_ವೃಕ್ಷ(), rejectBasep);
            ಆದರೆ (required_ಹೆಜ್ಜೆ_Rejectp)
                VL_DO_DANGLING(required_ಹೆಜ್ಜೆ_Rejectp->delete_ವೃಕ್ಷ(), required_ಹೆಜ್ಜೆ_Rejectp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಫಲಿತಾಂಶ_Exprp = ಹೊಸ AstLogNot{flp, matchp};
            ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ_Exprp;
        }
        ಆದರೆ (isCover) {
            ಆದರೆ (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_ವೃಕ್ಷ(), throughoutRejectp);
            ಆದರೆ (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_ವೃಕ್ಷ(), rejectBasep);
            ಆದರೆ (required_ಹೆಜ್ಜೆ_Rejectp)
                VL_DO_DANGLING(required_ಹೆಜ್ಜೆ_Rejectp->delete_ವೃಕ್ಷ(), required_ಹೆಜ್ಜೆ_Rejectp);
            ಆದರೆ (ಹೊಂದಿಕೆ_Condp) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ sampledCondp = sampled(ಹೊಂದಿಕೆ_Condp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
                ಹಿಂತಿರುಗಿ ಹೊಸ AstLogAnd{flp, terminalActivep, sampledCondp};
            }
            ಹಿಂತಿರುಗಿ terminalActivep;
        }
        // Assert/assume: output = !reject
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rejectp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (ಹೊಂದಿಕೆ_Condp && rejectBasep) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ sampledCondp = sampled(ಹೊಂದಿಕೆ_Condp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
            rejectp = ಹೊಸ AstLogAnd{flp, rejectBasep, ಹೊಸ AstLogNot{flp, sampledCondp}};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (rejectBasep) {
            VL_DO_DANGLING(rejectBasep->delete_ವೃಕ್ಷ(), rejectBasep);
        }
        ಆದರೆ (ಹೊರಹೋಗು_Matchpp) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಹೊಂದಿಕೆ_Exprp = terminalActivep->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            ಆದರೆ (ಹೊಂದಿಕೆ_Condp) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ sp = sampled(ಹೊಂದಿಕೆ_Condp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
                ಹೊಂದಿಕೆ_Exprp = ಹೊಸ AstLogAnd{flp, ಹೊಂದಿಕೆ_Exprp, sp};
            }
            *ಹೊರಹೋಗು_Matchpp = ಹೊಂದಿಕೆ_Exprp;
        }
        ಆದರೆ (terminalActivep) VL_DO_DANGLING(terminalActivep->delete_ವೃಕ್ಷ(), terminalActivep);
        ಆದರೆ (throughoutRejectp) rejectp = orExprs(flp, rejectp, throughoutRejectp);
        ಆದರೆ (required_ಹೆಜ್ಜೆ_Rejectp) rejectp = orExprs(flp, rejectp, required_ಹೆಜ್ಜೆ_Rejectp);
        ಆದರೆ (!rejectp) ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಫಲಿತಾಂಶ_Exprp = ಹೊಸ AstLogNot{flp, rejectp};
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ_Exprp;
    }

    ಸ್ಪಷ್ಟ SvaNfaLowering(Ast_ಗಂಟು_Module* modp)
        : m_modp{modp}
        , m_u32DTypep{modp->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::UINT32)} {}
    ~SvaNfaLowering() {
        V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("Assertions, NFA delay ring edge visits", m_stat_Delay_Ring_ಅಂಚು_Visits);
    }

    // Lower NFA graph to synthesizable AstAlways blocks and raw result signals.
    // Links are combinational; Edges are registered (NBA).
    Signal_ಗಣ ಅಧೋ(Ast_ಗಂಟು_Cover_Or_Assert* ಸ್ಥಿರಾಂಕ assertp, Sva_ನಕ್ಷೆ& ನಕ್ಷೆ,
                    Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಹೊಂದಿಕೆ_Condp,
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ disableExprp, ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ need_Thread_Fail_ಎಣಿಕೆ,
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ need_Throughout_Thread_Fail_ಎಣಿಕೆ) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = assertp->fileline();
        AstCover* ಸ್ಥಿರಾಂಕ coverp = VN_CAST(assertp, Cover);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isSeqEvent = coverp && coverp->isSeqEvent();
        ಸ್ಥಿರಾಂಕ VAssertType assertType
            = isSeqEvent ? VAssertType{VAssertType::ಆಂತರಿಕ} : assertp->userType();
        ಸ್ಥಿರಾಂಕ VAssertDirectiveType directiveType
            = isSeqEvent ? VAssertDirectiveType{VAssertDirectiveType::ಆಂತರಿಕ}
                         : assertp->directive();
        ಸ್ಥಿರಾಂಕ std::string ತಳ_Name = m_ಹೆಸರುಗಳು.get("");

        // Number vertices with sequential colors for array indexing.
        ಪೂರ್ಣಾಂಕ N = 0;
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtxr : ನಕ್ಷೆ.m_ನಕ್ಷೆ.vertices()) { vtxr.color(N++); }
        std::ಸದಿಶ<Sva_State_ಶೃಂಗ*> vtx(N, ಶೂನ್ಯಸೂಚಕ);
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtxr : ನಕ್ಷೆ.m_ನಕ್ಷೆ.vertices()) {
            vtx[vtxr.color()] = static_cast<Sva_State_ಶೃಂಗ*>(&vtxr);
        }

        // Allocate per-vertex lowering data (stored via V3GraphVertex::userp()).
        std::ಸದಿಶ<Sva_ಶೃಂಗ_ದತ್ತಾಂಶ> ಶೃಂಗ_ದತ್ತಾಂಶ(N);
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < N; ++i) vtx[i]->userp(&ಶೃಂಗ_ದತ್ತಾಂಶ[i]);

        // Identify registered vertices (targets of clocked edges).
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < N; ++i) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& edger : vtx[i]->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                ಸ್ಥಿರಾಂಕ Sva_Trans_ಅಂಚು& tedger = static_cast<ಸ್ಥಿರಾಂಕ Sva_Trans_ಅಂಚು&>(edger);
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಗಮ್ಯ_Idx = tedger.ಗಮ್ಯ_Vtxp()->color();
                ಆದರೆ (tedger.m_consumes_ಚಕ್ರ && tedger.ಗಮ್ಯ_Vtxp() != ನಕ್ಷೆ.m_ಹೊಂದಿಕೆ_Vertexp
                    && !tedger.ಗಮ್ಯ_Vtxp()->m_isRejectSink) {
                    vtx[ಗಮ್ಯ_Idx]->datap()->needsReg = ನಿಜ;
                }
            }
        }

        // A packed range ring can emit several live endpoints without needing per-slot counts.
        // Widen only a ring whose input can already carry multiplicity.
        std::ಸದಿಶ<ತಾರ್ಕಿಕ> stateMayHaveMultiplicity(N);
        std::ಸದಿಶ<ತಾರ್ಕಿಕ> ringNeedsMultiplicity(N);
        std::ಸದಿಶ<ಪೂರ್ಣಾಂಕ> multiplicityWork;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಎಣಿಕೆ_Matches = coverp && coverp->isCoverSeq() && !isSeqEvent;
        ಆದರೆ (ಎಣಿಕೆ_Matches) {
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < N; ++i) {
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಅಪವರ್ತ್ಯ_Inputs = vtx[i]->ಒಳ_ಅಂಚುಗಳು().ಗಾತ್ರ_2() > 1;
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isRing = vtx[i]->m_delay_Ring_ಗಾತ್ರ;
                ringNeedsMultiplicity[i] = isRing && ಅಪವರ್ತ್ಯ_Inputs;
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ startsMultiplicity = isRing
                                                    ? !vtx[i]->m_isFixedDelayRing
                                                    : ಅಪವರ್ತ್ಯ_Inputs && !vtx[i]->datap()->needsReg;
                ಆದರೆ (startsMultiplicity) {
                    stateMayHaveMultiplicity[i] = ನಿಜ;
                    multiplicityWork.ತಳ್ಳು_ಹಿಂಭಾಗ(i);
                }
            }
            ಗಾಗಿ (ಗಾತ್ರ_t wi = 0; wi < multiplicityWork.ಗಾತ್ರ_2(); ++wi) {
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& edger : vtx[multiplicityWork[wi]]->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                    ಸ್ಥಿರಾಂಕ Sva_Trans_ಅಂಚು& tedger = static_cast<ಸ್ಥಿರಾಂಕ Sva_Trans_ಅಂಚು&>(edger);
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ti = tedger.ಗಮ್ಯ_Vtxp()->color();
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isRing = vtx[ti]->m_delay_Ring_ಗಾತ್ರ;
                    ಆದರೆ (isRing) ringNeedsMultiplicity[ti] = ನಿಜ;
                    // Match and reject sinks have no outgoing edges, so they stop naturally.
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ stopsMultiplicity = !isRing && vtx[ti]->datap()->needsReg;
                    ಆದರೆ (stateMayHaveMultiplicity[ti] || stopsMultiplicity) { ಮುಂದುವರಿ; }
                    stateMayHaveMultiplicity[ti] = ನಿಜ;
                    multiplicityWork.ತಳ್ಳು_ಹಿಂಭಾಗ(ti);
                }
            }
        }

        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ asyncDisable = isAsyncDisable(disableExprp);
        // Registers cleared on a rising disable are also written by their clocked block.
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ regFlp = asyncDisable ? ಹೊಸ File_ಸಾಲು{flp} : flp;
        ಆದರೆ (asyncDisable) regFlp->warnOff(V3_Error_ಸಂಕೇತ::MULTIDRIVEN, ನಿಜ);

        AstVar* ಸ್ಥಿರಾಂಕ killVarp
            = ಹೊಸ AstVar{flp, VVarType::MODULETEMP, ತಳ_Name + "__kill", m_u32DTypep};
        killVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->ಕೂಡಿಸು_Stmtsp(killVarp);
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < N; ++i) {
            ಆದರೆ (vtx[i]->m_isAndCombiner) {
                ಸ್ಥಿರಾಂಕ std::string ತಳ = ತಳ_Name + "__a" + std::ಗಮ್ಯ_string(i);
                AstVar* ಸ್ಥಿರಾಂಕ lp = ಹೊಸ AstVar{regFlp, VVarType::MODULETEMP, ತಳ + "_doneL",
                                              m_modp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType()};
                lp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->ಕೂಡಿಸು_Stmtsp(lp);
                vtx[i]->datap()->ಮುಗಿದ_LVarp = lp;
                AstVar* ಸ್ಥಿರಾಂಕ rp = ಹೊಸ AstVar{regFlp, VVarType::MODULETEMP, ತಳ + "_doneR",
                                              m_modp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType()};
                rp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->ಕೂಡಿಸು_Stmtsp(rp);
                vtx[i]->datap()->ಮುಗಿದ_RVarp = rp;
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (vtx[i]->m_delay_Ring_ಗಾತ್ರ) {
                ಸ್ಥಿರಾಂಕ std::string ತಳ = ತಳ_Name + "__d" + std::ಗಮ್ಯ_string(i);
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ storesCounts = ಎಣಿಕೆ_Matches && ringNeedsMultiplicity[i];
                Ast_ಗಂಟು_DType* ringDTypep = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (storesCounts) {
                    // int unsigned ring [size];
                    ringDTypep = ಹೊಸ AstUnpackArrayDType{
                        flp, m_u32DTypep,
                        ಹೊಸ AstRange{flp, static_cast<ಪೂರ್ಣಾಂಕ>(vtx[i]->m_delay_Ring_ಗಾತ್ರ - 1), 0}};
                    v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(ringDTypep);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // bit [size-1:0] ring;
                    ringDTypep = m_modp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(vtx[i]->m_delay_Ring_ಗಾತ್ರ,
                                                      vtx[i]->m_delay_Ring_ಗಾತ್ರ, VSigning::UNSIGNED);
                }
                AstVar* ಸ್ಥಿರಾಂಕ ringp
                    = ಹೊಸ AstVar{flp, VVarType::MODULETEMP, ತಳ + "_ring", ringDTypep};
                ringp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->ಕೂಡಿಸು_Stmtsp(ringp);
                vtx[i]->datap()->delayRingVarp = ringp;
                vtx[i]->datap()->delayRingCounts = storesCounts;
                AstVar* ಸ್ಥಿರಾಂಕ headp
                    = ಹೊಸ AstVar{flp, VVarType::MODULETEMP, ತಳ + "_head",
                                 storesCounts ? m_u32DTypep : m_modp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType()};
                headp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->ಕೂಡಿಸು_Stmtsp(headp);
                vtx[i]->datap()->delay_Ring_ತಲೆ_Varp = headp;
                // int unsigned idx;
                AstVar* ಸ್ಥಿರಾಂಕ idxp
                    = ಹೊಸ AstVar{regFlp, VVarType::MODULETEMP, ತಳ + "_idx", m_u32DTypep};
                idxp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->ಕೂಡಿಸು_Stmtsp(idxp);
                vtx[i]->datap()->delayRingIdxVarp = idxp;
                // int unsigned live_count;
                AstVar* ಸ್ಥಿರಾಂಕ ಜೀವಂತ_ಎಣಿಕೆ_Varp
                    = ಹೊಸ AstVar{regFlp, VVarType::MODULETEMP, ತಳ + "_liveCount", m_u32DTypep};
                ಜೀವಂತ_ಎಣಿಕೆ_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->ಕೂಡಿಸು_Stmtsp(ಜೀವಂತ_ಎಣಿಕೆ_Varp);
                vtx[i]->datap()->delay_Ring_ಜೀವಂತ_ಎಣಿಕೆ_Varp = ಜೀವಂತ_ಎಣಿಕೆ_Varp;
                AstVar* ಸ್ಥಿರಾಂಕ wrappedp = ಹೊಸ AstVar{regFlp, VVarType::MODULETEMP,
                                                    ತಳ + "_wrapped", m_modp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType()};
                wrappedp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->ಕೂಡಿಸು_Stmtsp(wrappedp);
                vtx[i]->datap()->delayRingWrappedVarp = wrappedp;
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (!vtx[i]->datap()->needsReg) ಮುಂದುವರಿ;
            ಆದರೆ (vtx[i] == ನಕ್ಷೆ.m_ಆರಂಭ_Vertexp || vtx[i]->m_is_ಹೊಂದಿಕೆ) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ std::string varName = ತಳ_Name + "__s" + std::ಗಮ್ಯ_string(i);
            AstVar* ಸ್ಥಿರಾಂಕ varp
                = ಹೊಸ AstVar{regFlp, VVarType::MODULETEMP, varName, m_modp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->ಕೂಡಿಸು_Stmtsp(varp);
            vtx[i]->datap()->stateVarp = varp;
        }

        // Build lowering context for phase sub-functions.
        ಅಧೋ_Ctx c{flp,        senTreep,      disableExprp, ಹೊಂದಿಕೆ_Condp,
                   assertType, directiveType, killVarp,     ನಕ್ಷೆ};

        // Phase 1: Resolve combinational Links via fixed-point propagation.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ triggerExprp
            = isSeqEvent ? ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}}
                         : assertOnCond(flp, assertp->userType(), assertp->directive());
        resolveLinks(c, triggerExprp, ಎಣಿಕೆ_Matches);
        VL_DO_DANGLING(triggerExprp->delete_ವೃಕ್ಷ(), triggerExprp);

        // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
        emit_State_ರಿಜಿಸ್ಟರ್_Nba(c);
        emitDelayRingUpdates(c, ಎಣಿಕೆ_Matches);
        emit_And_Combiner_ಮುಗಿದ_Latch_Nba(c);
        emitKillAckNba(c);

        // Phase 3/3a/3b: Compute terminal match/reject signals (cleans up stateSig).
        ಸ್ಥಿರಾಂಕ Signal_ಗಣ sigs
            = compute_ಸಂಕೇತಗಳು(c, need_Thread_Fail_ಎಣಿಕೆ, need_Throughout_Thread_Fail_ಎಣಿಕೆ, ಎಣಿಕೆ_Matches);

        // Strong s_always[m:n] end-of-simulation liveness: if any in-window state
        // is still set at $finish, the universal-quantifier window never completed
        // (IEEE 1800-2023 16.12.11 strong semantics). Fire the assertion failure
        // from a final block; V3Assert turns the DT_ERROR display into the standard
        // "Assertion failed in %m" message.
        // pending = |{strong state registers, strong delay live counts != 0};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pendingp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < N; ++i) {
            ಆದರೆ (!vtx[i]->m_strongPending) ಮುಂದುವರಿ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pendingExprp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (vtx[i]->datap()->stateVarp) {
                pendingExprp = ಹೊಸ AstVarRef{flp, vtx[i]->datap()->stateVarp, VAccess::READ};
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (vtx[i]->m_strongAlwaysRing) {
                pendingExprp
                    = delay_Ring_Has_ಜೀವಂತ_Threadsp(flp, vtx[i]->datap()->delay_Ring_ಜೀವಂತ_ಎಣಿಕೆ_Varp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (!pendingp) {
                pendingp = pendingExprp;
            } ಇಲ್ಲದಿದ್ದರೆ {
                pendingp = ಹೊಸ AstLogOr{flp, pendingp, pendingExprp};
            }
        }
        ಆದರೆ (pendingp) {
            AstCExpr* ಸ್ಥಿರಾಂಕ assertOnp
                = ಹೊಸ AstCExpr{flp, AstCExpr::ಶುದ್ಧ{}, "vlSymsp->_vm_contextp__->assertOn()", 1};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = ಹೊಸ AstLogAnd{flp, assertOnp, pendingp};
            AstDisplay* ಸ್ಥಿರಾಂಕ dispp
                = ಹೊಸ AstDisplay{flp, VDisplayType::DT_ERROR, "", ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
            dispp->fmtp()->timeunit(m_modp->timeunit());
            Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ firep = dispp;
            ಆದರೆ (v3Global.opt.stopFail()) firep->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ AstStop{flp, ಸುಳ್ಳು});
            m_modp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstFinal{flp, ಹೊಸ AstIf{flp, condp, firep}});
        }

        // Clear userp on every vertex before vertexData is destroyed.
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < N; ++i) vtx[i]->userp(ಶೂನ್ಯಸೂಚಕ);
        ಹಿಂತಿರುಗಿ sigs;
    }
};

}  // namespace

//######################################################################
// Top-level visitor

ವರ್ಗ AssertNfaVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module being processed
    AstClocking* m_defaultClockingp = ಶೂನ್ಯಸೂಚಕ;  // Default clocking
    AstDefaultDisable* m_defaultDisablep = ಶೂನ್ಯಸೂಚಕ;  // Default disable iff
    SvaNfaLowering* m_loweringp = ಶೂನ್ಯಸೂಚಕ;  // NFA-to-hardware lowering engine
    Ast_Sen_ವೃಕ್ಷ* m_sampled_ಮೌಲ್ಯ_Clockp = ಶೂನ್ಯಸೂಚಕ;  // Inherited clock during scoped attachment
    V3_Unique_ಹೆಸರುಗಳು m_prop_Var_ಹೆಸರುಗಳು{"__Vpropvar"};  // Property-local variable names
    V3_Unique_ಹೆಸರುಗಳು m_prop_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು{"__VnfaSampled"};  // Hoisted $sampled(propp) temps
    V3_Unique_ಹೆಸರುಗಳು m_fail_ಎಣಿಕೆ_ಹೆಸರುಗಳು{"__VnfaRemainingFailCount"};  // Fail replay counter names
    std::ಗಣ<ಸ್ಥಿರಾಂಕ AstProperty*> m_inliningProps;  // Recursion guard for inlineNamedProperty

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_ಗಂಟು>
    ಶೂನ್ಯ ಭೇಟಿ_Sampled_ಮೌಲ್ಯ(T_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) {
        ಆದರೆ (m_sampled_ಮೌಲ್ಯ_Clockp && !nodep->sentreep()) {
            nodep->sentreep(m_sampled_ಮೌಲ್ಯ_Clockp->clone_ವೃಕ್ಷ(ನಿಜ));
        }
        iterateChildren(nodep);
    }

    // Wire match vertex and mid-window sources for a successful NFA build.
    ಸ್ಥಿರ ಶೂನ್ಯ wire_ಹೊಂದಿಕೆ_And_Mid_Sources(Sva_ನಕ್ಷೆ& ನಕ್ಷೆ, ಸ್ಥಿರಾಂಕ Build_ಫಲಿತಾಂಶ& ಫಲಿತಾಂಶ, File_ಸಾಲು* flp) {
        ನಕ್ಷೆ.create_ಹೊಂದಿಕೆ_ಶೃಂಗ();
        // Skip the main term Link when midSources already cover every
        // end-of-match (cover_sequence path); otherwise the per-mid extraction
        // double-counts via the merge vertex.
        ಆದರೆ (!ಫಲಿತಾಂಶ.ಪದ_Is_Mid_ವಿಲೀನ) { ನಕ್ಷೆ.ಕೂಡಿಸು_Link(ಫಲಿತಾಂಶ.ಪದ_Vertexp, ನಕ್ಷೆ.m_ಹೊಂದಿಕೆ_Vertexp); }
        ಗಾಗಿ (Sva_State_ಶೃಂಗ* srcVtxp : ಫಲಿತಾಂಶ.midSources) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ tc : srcVtxp->m_throughoutConds) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ tcClone = tc->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                condp = condp ? ಹೊಸ AstLogAnd{flp, condp, tcClone} : tcClone;
            }
            ನಕ್ಷೆ.ಕೂಡಿಸು_Link(srcVtxp, ನಕ್ಷೆ.m_ಹೊಂದಿಕೆ_Vertexp, condp);
            srcVtxp->m_isUnbounded = ನಿಜ;
        }
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* getSequenceBodyExprp(ಸ್ಥಿರಾಂಕ AstSequence* seqp) {
        Ast_ಗಂಟು* bodyp = seqp->stmtsp();
        ಆಗ (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        ಹಿಂತಿರುಗಿ VN_CAST(bodyp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
    }

    ಸ್ಥಿರ AstPropSpec* getPropertySpecp(ಸ್ಥಿರಾಂಕ AstProperty* propp) {
        Ast_ಗಂಟು* stmtp = propp->stmtsp();
        // V3LinkParse emits InitialStaticStmt for property-local variable
        // initialisers; the InitialAutomaticStmt variant only appears for
        // task/function-scope automatic lifetime, not properties.
        ಆಗ (stmtp
               && (VN_IS(stmtp, Var) || VN_IS(stmtp, InitialStaticStmt)
                   || VN_IS(stmtp, InitialAutomaticStmt))) {  // LCOV_EXCL_LINE
            stmtp = stmtp->nextp();
        }
        ಹಿಂತಿರುಗಿ VN_CAST(stmtp, PropSpec);
    }

    ಶೂನ್ಯ inlineNamedProperty(AstPropSpec* outerSpecp, AstFuncRef* funcrefp,
                             ಸ್ಥಿರಾಂಕ AstProperty* propyp) {
        // Recursion guard: IEEE 1800-2023 16.12.1 forbids recursive properties.
        // V3Width emits "Recursive property call" for direct recursion before this
        // pass runs; this catches any nested-inlining cycle that slips past.
        ಆದರೆ (!m_inliningProps.emplace(propyp).ಎರಡನೇ) {
            funcrefp->v3error("Illegal recursive property reference");  // LCOV_EXCL_LINE
            ಹಿಂತಿರುಗಿ;  // LCOV_EXCL_LINE
        }
        ರಚನೆ Guard final {
            std::ಗಣ<ಸ್ಥಿರಾಂಕ AstProperty*>& setr;
            ಸ್ಥಿರಾಂಕ AstProperty* keyp;
            ~Guard() { setr.erase(keyp); }
        } guard{m_inliningProps, propyp};
        AstPropSpec* propSpecp = getPropertySpecp(propyp);
        UASSERT_OBJ(propSpecp, funcrefp, "Property has no body PropSpec");
        propSpecp = propSpecp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);

        ಸ್ಥಿರಾಂಕ V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propyp->stmtsp());
        std::unordered_map<ಸ್ಥಿರಾಂಕ AstVar*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ಪೋರ್ಟ್_Map;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& tconnect : tconnects) {
            ಪೋರ್ಟ್_Map[tconnect.ಮೊದಲ] = tconnect.ಎರಡನೇ->exprp();
        }

        // Promote property-local variables to module-level temps (IEEE 16.10).
        std::unordered_map<ಸ್ಥಿರಾಂಕ AstVar*, AstVar*> localVarMap;
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = propyp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtp, Var)) {
                ಆದರೆ (!varp->isIO()) {
                    ಸ್ಥಿರಾಂಕ string newName = m_prop_Var_ಹೆಸರುಗಳು.get(varp);
                    AstVar* ಸ್ಥಿರಾಂಕ newVarp = ಹೊಸ AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->ಕೂಡಿಸು_Stmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        propSpecp->foreach([&](AstVarRef* refp) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಪೋರ್ಟ್_It = ಪೋರ್ಟ್_Map.ಕಂಡುಹಿಡಿ(refp->varp());
            ಆದರೆ (ಪೋರ್ಟ್_It != ಪೋರ್ಟ್_Map.ಕೊನೆ()) {
                refp->replaceWith(ಪೋರ್ಟ್_It->ಎರಡನೇ->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
                ಹಿಂತಿರುಗಿ;
            }
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ localIt = localVarMap.ಕಂಡುಹಿಡಿ(refp->varp());
            ಆದರೆ (localIt != localVarMap.ಕೊನೆ()) refp->varp(localIt->ಎರಡನೇ);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& tconnect : tconnects) {
            ತಳ್ಳು_Deletep(tconnect.ಎರಡನೇ->exprp()->unlink_Fr_ಹಿಂಭಾಗ());
        }
        V3AssertCommon::checkSvaInstanceDelays(propSpecp, funcrefp);

        // Merge disable iff (IEEE 1800-2023 16.12.1)
        ಆದರೆ (outerSpecp->disablep() && propSpecp->disablep()) {
            outerSpecp->v3error("disable iff expression before property call "
                                "and in its body is not legal");
            ತಳ್ಳು_Deletep(propSpecp->disablep()->unlink_Fr_ಹಿಂಭಾಗ());
        }
        ಆದರೆ (outerSpecp->disablep()) {
            propSpecp->disablep(outerSpecp->disablep()->unlink_Fr_ಹಿಂಭಾಗ());
        }

        ಆದರೆ (outerSpecp->sensesp() && propSpecp->sensesp()) {
            outerSpecp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Clock event before property call and in its body");
            ತಳ್ಳು_Deletep(propSpecp->sensesp()->unlink_Fr_ಹಿಂಭಾಗ());
        }
        ಆದರೆ (outerSpecp->sensesp()) {
            Ast_Sen_ಅಂಶ* ಸ್ಥಿರಾಂಕ sensesp = outerSpecp->sensesp();
            sensesp->unlink_Fr_ಹಿಂಭಾಗ();
            propSpecp->sensesp(sensesp);
        }

        outerSpecp->replaceWith(propSpecp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(outerSpecp), outerSpecp);
    }

    ಶೂನ್ಯ inlineSequenceRef(AstFuncRef* funcrefp, AstSequence* seqp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ clonedp = bodyExprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);

        ಸ್ಥಿರಾಂಕ V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ಸ್ಥಿರಾಂಕ AstVar*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ಪೋರ್ಟ್_Map;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& tconnect : tconnects) {
            ಪೋರ್ಟ್_Map[tconnect.ಮೊದಲ] = tconnect.ಎರಡನೇ->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = ಪೋರ್ಟ್_Map.ಕಂಡುಹಿಡಿ(refp->varp());
            ಆದರೆ (it != ಪೋರ್ಟ್_Map.ಕೊನೆ()) {
                refp->replaceWith(it->ಎರಡನೇ->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
            }
        });
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& tconnect : tconnects) {
            ತಳ್ಳು_Deletep(tconnect.ಎರಡನೇ->exprp()->unlink_Fr_ಹಿಂಭಾಗ());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(funcrefp), funcrefp);
        // Clear referenced flag so V3AssertPre cleanup does not emit
        // spurious UNSUPPORTED for sequences that were already inlined here.
        seqp->isReferenced(ಸುಳ್ಳು);
    }

    // Must run before hasMultiCycleExpr() so NFA sees sequence bodies.
    ಶೂನ್ಯ inlineAllSequenceRefs(Ast_ಗಂಟು* rootp) {
        ತಾರ್ಕಿಕ ಬದಲಾಯಿತು = ನಿಜ;
        ಆಗ (ಬದಲಾಯಿತು) {
            ಬದಲಾಯಿತು = ಸುಳ್ಳು;
            rootp->foreach([&](AstFuncRef* funcrefp) {
                ಆದರೆ (ಬದಲಾಯಿತು) ಹಿಂತಿರುಗಿ;
                ಆದರೆ (AstSequence* ಸ್ಥಿರಾಂಕ seqp = VN_CAST(funcrefp->taskp(), Sequence)) {
                    inlineSequenceRef(funcrefp, seqp);
                    ಬದಲಾಯಿತು = ನಿಜ;
                }
            });
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ has_Multi_ಚಕ್ರ_ಅಭಿವ್ಯಕ್ತಿ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಹಿಂತಿರುಗಿ nodep->exists([](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* np) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ ep = VN_CAST(np, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) ಹಿಂತಿರುಗಿ ep->is_Multi_ಚಕ್ರ_Sva();
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
    }

    ಸ್ಥಿರ VPropStrength effectiveAssertPropStrength(ಸ್ಥಿರಾಂಕ AstPropSpec* ಸ್ಥಿರಾಂಕ propSpecp) {
        ಆದರೆ (propSpecp->propStrength() != VPropStrength::DEFAULT) ಹಿಂತಿರುಗಿ propSpecp->propStrength();
        ಹಿಂತಿರುಗಿ propSpecp->fileline()->language() <= V3_Lang_ಸಂಕೇತ::L1800_2005 ? VPropStrength::STRONG
                                                                           : VPropStrength::WEAK;
    }

    // Bare `assert property (p until q)` with boolean operands stays on
    // V3AssertPre's AstLoop lowering, which preserves per-attempt action-block
    // firings that this NFA's single-bit aggregated state cannot. Strong bare
    // forms are also lowered there. NFA still owns sequence operands and any
    // embedding inside a multi-cycle context (implication consequent, or/and
    // operands, etc.).
    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_Bare_ತುದಿ_Level_Until(Ast_ಗಂಟು* propp) {
        Ast_ಗಂಟು* p = propp;
        ಆದರೆ (AstPropSpec* ಸ್ಥಿರಾಂಕ specp = VN_CAST(p, PropSpec)) p = specp->propp();
        ಆಗ (AstLogNot* ಸ್ಥಿರಾಂಕ notp = VN_CAST(p, LogNot)) p = notp->lhsp();
        AstUntil* ಸ್ಥಿರಾಂಕ untilp = VN_CAST(p, Until);
        ಆದರೆ (!untilp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ hasSeq = [](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ep) {
            ಹಿಂತಿರುಗಿ ep->exists([](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* np) { ಹಿಂತಿರುಗಿ np->is_Multi_ಚಕ್ರ_Sva(); });
        };
        ಆದರೆ (hasSeq(untilp->lhsp()) || hasSeq(untilp->rhsp())) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ರಚನೆ PropertyParts final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* triggerExprp = ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* seqExprp = ಶೂನ್ಯಸೂಚಕ;
        ತಾರ್ಕಿಕ isOverlapped = ನಿಜ;
        ತಾರ್ಕಿಕ hasImplication = ಸುಳ್ಳು;
        ತಾರ್ಕಿಕ isFollowedBy = ಸುಳ್ಳು;  // True for #-# / #=# (non-vacuous-fail on antecedent miss)
    };

    ಸ್ಥಿರ PropertyParts decomposeProperty(Ast_ಗಂಟು* propp) {
        PropertyParts parts;
        ಆದರೆ (AstPropSpec* ಸ್ಥಿರಾಂಕ specp = VN_CAST(propp, PropSpec)) { propp = specp->propp(); }
        ಆದರೆ (AstImplication* ಸ್ಥಿರಾಂಕ implp = VN_CAST(propp, Implication)) {
            parts.hasImplication = ನಿಜ;
            parts.isOverlapped = implp->isOverlapped();
            parts.isFollowedBy = implp->isFollowedBy();
            parts.triggerExprp = implp->lhsp();
            parts.seqExprp = implp->rhsp();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = VN_CAST(propp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
            parts.triggerExprp = ಶೂನ್ಯಸೂಚಕ;
            parts.seqExprp = exprp;
        }
        ಹಿಂತಿರುಗಿ parts;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ can_ವಿಭಜಿಸು_Implication_Pass_Actions(ಸ್ಥಿರಾಂಕ PropertyParts& parts) {
        UASSERT(parts.hasImplication,
                "Implication pass action split requested without implication");
        UASSERT(parts.triggerExprp, "Implication pass action split requested without trigger");
        // Direct vacuous/nonvacuous classification uses the antecedent value in the current
        // assertion attempt. Leave delayed antecedents on the existing NFA pass path.
        ಹಿಂತಿರುಗಿ !has_Multi_ಚಕ್ರ_ಅಭಿವ್ಯಕ್ತಿ(parts.triggerExprp);
    }

    ಸ್ಥಿರ ಶೂನ್ಯ ವಿಭಜಿಸು_Implication_Pass_Actions(AstAssert* assertp, ಸ್ಥಿರಾಂಕ PropertyParts& parts,
                                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nonvacuousMatchp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = assertp->fileline();

        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ passsp = assertp->passsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
        Ast_ಗಂಟು* splitsp = ಶೂನ್ಯಸೂಚಕ;

        ಆದರೆ (!parts.isFollowedBy) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ vacuousp
                = ಹೊಸ AstLogNot{flp, sampled(parts.triggerExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು))};
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ vacuousBodyp = passsp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            splitsp = newPassOnIf(flp, vacuousp, vacuousBodyp, assertp->userType(),
                                  assertp->directive(), /*vacuous=*/ನಿಜ);
        }

        AstIf* ಸ್ಥಿರಾಂಕ nonvacuousp
            = newPassOnIf(flp, nonvacuousMatchp, passsp, assertp->userType(), assertp->directive(),
                          /*vacuous=*/ಸುಳ್ಳು);
        splitsp = splitsp ? Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ<Ast_ಗಂಟು, Ast_ಗಂಟು>(splitsp, nonvacuousp)
                          : static_cast<Ast_ಗಂಟು*>(nonvacuousp);
        assertp->ಕೂಡಿಸು_Passsp(splitsp);
    }

    // On a PropSpec-wrapped assertion whose NFA build failed with a semantic
    // error (errorEmitted), replace the body with a BitFalse const so later
    // passes see a well-formed AST. Returns true if replaced.
    ಶೂನ್ಯ replaceBodyOnBuildError(File_ಸಾಲು* flp, AstPropSpec* propSpecp, ತಾರ್ಕಿಕ errorEmitted) {
        ಆದರೆ (!errorEmitted) ಹಿಂತಿರುಗಿ;
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ innerPropp = propSpecp->propp();
        innerPropp->replaceWith(ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(innerPropp), innerPropp);
    }

    // Hoist a leading clocking event (IEEE 1800-2023 16.7):
    ತಾರ್ಕಿಕ hoistClockedSeq(AstPropSpec* specp) {
        ಆಗ (AstSClocked* ಸ್ಥಿರಾಂಕ clockedp = VN_CAST(specp->propp(), SClocked)) {
            ಆದರೆ (specp->sensesp()) {
                clockedp->v3warn(E_UNSUPPORTED, "Unsupported: multiclocked sequence or property");
                replaceBodyOnBuildError(specp->fileline(), specp, ನಿಜ);
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ* sp = clockedp->sensesp(); sp;
                 sp = VN_CAST(sp->nextp(), Sen_ಅಂಶ)) {
                ಆದರೆ (!sp->ಅಂಚು_Type().an_ಅಂಚು()) {
                    clockedp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: non-edge clocking event on a sequence; "
                                     "use an edge such as @(posedge clk)");
                    replaceBodyOnBuildError(specp->fileline(), specp, ನಿಜ);
                    ಹಿಂತಿರುಗಿ ನಿಜ;
                }
            }
            specp->sensesp(clockedp->sensesp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ bodyp = clockedp->exprp()->unlink_Fr_ಹಿಂಭಾಗ();
            clockedp->replaceWith(bodyp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(clockedp), clockedp);
        }
        // A clocking event anywhere else in the sequence is not supported.
        ಸ್ಥಿರಾಂಕ AstSClocked* nestedp = ಶೂನ್ಯಸೂಚಕ;
        specp->propp()->foreach([&](ಸ್ಥಿರಾಂಕ AstSClocked* p) {
            ಆದರೆ (!nestedp) nestedp = p;
        });
        ಆದರೆ (nestedp) {
            nestedp->v3warn(E_UNSUPPORTED,
                            "Unsupported: clocking event inside sequence expression");
            replaceBodyOnBuildError(specp->fileline(), specp, ನಿಜ);
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Build the NFA graph for a property body, handling both the antecedent
    // |-> consequent and simple sequence cases. Returns the consequent/body
    // BuildResult (invalid on parse/build failure).
    Build_ಫಲಿತಾಂಶ build_Assertion_ನಕ್ಷೆ(SvaNfaBuilder& builder, Sva_ನಕ್ಷೆ& ನಕ್ಷೆ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* seqBodyp,
                                    ಸ್ಥಿರಾಂಕ PropertyParts& parts, File_ಸಾಲು* flp) {
        ಆದರೆ (!parts.hasImplication) ಹಿಂತಿರುಗಿ builder.build(seqBodyp);

        ನಕ್ಷೆ.m_ಆರಂಭ_Vertexp = ನಕ್ಷೆ.create_State_ಶೃಂಗ();
        ಹಿಂತಿರುಗಿ builder.build_Implication_ಅಂಚುಗಳು(parts.triggerExprp, seqBodyp, ನಕ್ಷೆ.m_ಆರಂಭ_Vertexp,
                                             parts.isOverlapped, parts.isFollowedBy,
                                             parts.triggerExprp, flp);
    }

    // Install pass-action gating and replay simultaneous per-thread failures.
    ಶೂನ್ಯ attachActionHandlers(AstAssert* ಸ್ಥಿರಾಂಕ assertp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಹೊಂದಿಕೆ_Exprp,
                              Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ threadFailReplaySenTreep,
                              Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ threadFailCountp) {
        // Gate pass handler on match to prevent vacuous-pass firings.
        ಆದರೆ (ಹೊಂದಿಕೆ_Exprp) {
            // needMatch implies passsp() was non-null when evaluated above;
            // lowering does not mutate the assert's pass-action between the
            // two reads, so passsp() is still non-null here.
            Ast_ಗಂಟು* passsp = assertp->passsp();
            UASSERT_OBJ(passsp, assertp, "needMatch set but passsp is null");
            passsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = assertp->fileline();
            AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, ಹೊಂದಿಕೆ_Exprp, passsp, ಶೂನ್ಯಸೂಚಕ};
            assertp->ಕೂಡಿಸು_Passsp(ifp);
            // Fail-handler prefix for overlapping instances (IEEE 16.12):
            // fires when reject=1 && match=1 in the same cycle.
            ಆದರೆ (assertp->failsp()) assertp->prependFailsp(ifp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
        }

        ಆದರೆ (threadFailCountp) {
            UASSERT_OBJ(threadFailReplaySenTreep, assertp,
                        "Thread fail count missing sensitivity tree");
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ failsp = assertp->failsp();
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = assertp->fileline();
            // IEEE 1800-2023 16.12 requires one action-block evaluation per failed
            // thread. AstAssert handles the first, so replay the rest here.
            AstVar* ಸ್ಥಿರಾಂಕ ಉಳಿದ_Fail_ಎಣಿಕೆ_Varp
                = ಹೊಸ AstVar{flp, VVarType::MODULETEMP, m_fail_ಎಣಿಕೆ_ಹೆಸರುಗಳು.get(""),
                             m_modp->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::UINT32)};
            ಉಳಿದ_Fail_ಎಣಿಕೆ_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->ಕೂಡಿಸು_Stmtsp(ಉಳಿದ_Fail_ಎಣಿಕೆ_Varp);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ replayStmtsp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                flp, ಹೊಸ AstVarRef{flp, ಉಳಿದ_Fail_ಎಣಿಕೆ_Varp, VAccess::WRITE}, threadFailCountp};
            AstLoop* ಸ್ಥಿರಾಂಕ replayLoopp = ಹೊಸ AstLoop{flp};
            replayLoopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{
                flp, replayLoopp,
                ಹೊಸ AstGt{flp, ಹೊಸ AstVarRef{flp, ಉಳಿದ_Fail_ಎಣಿಕೆ_Varp, VAccess::READ},
                          newTypedConstp(flp, ಉಳಿದ_Fail_ಎಣಿಕೆ_Varp->dtypep(), 1)}});
            replayLoopp->ಕೂಡಿಸು_Stmtsp(newIfAssertFailOn(failsp->clone_ವೃಕ್ಷ(ನಿಜ), assertp->directive(),
                                                     assertp->userType()));
            AstSub* ಸ್ಥಿರಾಂಕ decrementedFailCountp
                = ಹೊಸ AstSub{flp, ಹೊಸ AstVarRef{flp, ಉಳಿದ_Fail_ಎಣಿಕೆ_Varp, VAccess::READ},
                             newTypedConstp(flp, ಉಳಿದ_Fail_ಎಣಿಕೆ_Varp->dtypep(), 1)};
            replayLoopp->ಕೂಡಿಸು_Stmtsp(
                ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, ಉಳಿದ_Fail_ಎಣಿಕೆ_Varp, VAccess::WRITE},
                              decrementedFailCountp});
            replayStmtsp->ಕೂಡಿಸು_ಮುಂದಿನ(replayLoopp);
            m_modp->ಕೂಡಿಸು_Stmtsp(
                ಹೊಸ AstAlways{flp, VAlwaysKwd::ALWAYS, threadFailReplaySenTreep, replayStmtsp});
        }
    }

    // Replace one VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). No-op if refp is not in matchMap.
    ಶೂನ್ಯ substitute_ಹೊಂದಿಕೆ_ಅಂಶ_Ref(AstVarRef* refp, ಚಿಹ್ನೆರಹಿತ K,
                                ಸ್ಥಿರಾಂಕ std::unordered_map<ಸ್ಥಿರಾಂಕ AstVar*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& ಹೊಂದಿಕೆ_Map) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = ಹೊಂದಿಕೆ_Map.ಕಂಡುಹಿಡಿ(refp->varp());
        ಆದರೆ (it == ಹೊಂದಿಕೆ_Map.ಕೊನೆ()) ಹಿಂತಿರುಗಿ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = it->ಎರಡನೇ->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        ಆದರೆ (K > 0) {
            AstConst* ಸ್ಥಿರಾಂಕ ticksp = ಹೊಸ AstConst{refp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, 32,
                                                  static_cast<uint32_t>(K)};
            AstPast* ಸ್ಥಿರಾಂಕ pastp = ಹೊಸ AstPast{refp->fileline(), newp, ticksp, ಶೂನ್ಯಸೂಚಕ,
                                               /* propertyTiming */ ನಿಜ};
            pastp->dtypeFrom(newp);
            newp = pastp;
        }
        refp->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
        ಹಿಂತಿರುಗಿ;
    }

    // Recursively walk a consequent. Returns cycle length consumed and
    // substitutes each VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). Reports E_UNSUPPORTED on non-constant
    // delays or composite sequence operators and returns -1 on failure.
    ಪೂರ್ಣಾಂಕ
    walk_Substitute_ಹೊಂದಿಕೆ_ವಸ್ತುಗಳು(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಚಿಹ್ನೆರಹಿತ K,
                             ಸ್ಥಿರಾಂಕ std::unordered_map<ಸ್ಥಿರಾಂಕ AstVar*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& ಹೊಂದಿಕೆ_ವಸ್ತುಗಳು) {
        ಆದರೆ (AstSExpr* ಸ್ಥಿರಾಂಕ sexprp = VN_CAST(nodep, SExpr)) {
            // IEEE 1800-2023 16.9.2: cycle_delay's lhsp is a constant_expression
            // and the delay form in a sequence is always `##N`, folded by
            // V3Const + V3Param before V3AssertNfa. Range form `##[m:n]` is the
            // only user-visible reject here.
            AstDelay* ಸ್ಥಿರಾಂಕ delayp = VN_AS(sexprp->delayp(), Delay);
            UASSERT_OBJ(delayp->is_ಚಕ್ರ_Delay() && VN_IS(delayp->lhsp(), Const), sexprp,
                        "SVA cycle delay must have a constant lhsp");
            ಆದರೆ (delayp->isRangeDelay()) {
                sexprp->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                              "non-constant cycle delay in consequent"
                                              " (IEEE 1800-2023 16.10)");
                ಹಿಂತಿರುಗಿ -1;
            }
            ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ delay_ಚಕ್ರಗಳು = VN_AS(delayp->lhsp(), Const)->ಗಮ್ಯ_UInt();
            ಪೂರ್ಣಾಂಕ preLen = 0;
            ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ prep = sexprp->preExprp()) {
                preLen = walk_Substitute_ಹೊಂದಿಕೆ_ವಸ್ತುಗಳು(prep, K, ಹೊಂದಿಕೆ_ವಸ್ತುಗಳು);
                ಆದರೆ (preLen < 0) ಹಿಂತಿರುಗಿ -1;
            }
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ bodyLen
                = walk_Substitute_ಹೊಂದಿಕೆ_ವಸ್ತುಗಳು(sexprp->exprp(), K + preLen + delay_ಚಕ್ರಗಳು, ಹೊಂದಿಕೆ_ವಸ್ತುಗಳು);
            ಆದರೆ (bodyLen < 0) ಹಿಂತಿರುಗಿ -1;
            ಹಿಂತಿರುಗಿ preLen + delay_ಚಕ್ರಗಳು + bodyLen;
        }
        ಆದರೆ (nodep->is_Multi_ಚಕ್ರ_Sva()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                         "composite sequence operator in consequent"
                                         " (IEEE 1800-2023 16.10)");
            ಹಿಂತಿರುಗಿ -1;
        }
        std::ಸದಿಶ<AstVarRef*> refs;
        nodep->foreach([&refs](AstVarRef* p) { refs.ತಳ್ಳು_ಹಿಂಭಾಗ(p); });
        ಗಾಗಿ (AstVarRef* ಸ್ಥಿರಾಂಕ refp : refs) substitute_ಹೊಂದಿಕೆ_ಅಂಶ_Ref(refp, K, ಹೊಂದಿಕೆ_ವಸ್ತುಗಳು);
        ಹಿಂತಿರುಗಿ 0;
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
    ತಾರ್ಕಿಕ lift_ಹೊಂದಿಕೆ_ಅಂಶ_Substitutions(PropertyParts& parts, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* seqBodyp) {
        ಆದರೆ (!parts.hasImplication) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಸ್ಥಿರಾಂಕ ಅಭಿವ್ಯಕ್ತಿ_Stmtp = VN_CAST(parts.triggerExprp, ಅಭಿವ್ಯಕ್ತಿ_Stmt);
        ಆದರೆ (!ಅಭಿವ್ಯಕ್ತಿ_Stmtp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // IEEE 1800-2023 16.10 BNF requires `(expr, match_item {, match_item})`
        // with at least one match item; V3LinkParse only emits ExprStmt for
        // this form and only emits AstAssign with VarRef LHS for each item.
        std::unordered_map<ಸ್ಥಿರಾಂಕ AstVar*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ಹೊಂದಿಕೆ_ವಸ್ತುಗಳು;
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = ಅಭಿವ್ಯಕ್ತಿ_Stmtp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = VN_AS(stmtp, ನಿಯೋಜಿಸು);
            AstVarRef* ಸ್ಥಿರಾಂಕ lhsRefp = VN_AS(assignp->lhsp(), VarRef);
            ಹೊಂದಿಕೆ_ವಸ್ತುಗಳು[lhsRefp->varp()] = assignp->rhsp();
        }
        ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ ಆರಂಭ_K = parts.isOverlapped ? 0 : 1;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಉದ್ದಳತೆ = walk_Substitute_ಹೊಂದಿಕೆ_ವಸ್ತುಗಳು(seqBodyp, ಆರಂಭ_K, ಹೊಂದಿಕೆ_ವಸ್ತುಗಳು);
        // Match-item substitution / strip mutates ancestor purity. Release
        // builds don't auto-clear caches on edits, so refresh here.
        VIsCached::clear_Cache_ವೃಕ್ಷ();
        ಆದರೆ (ಉದ್ದಳತೆ < 0) ಹಿಂತಿರುಗಿ ನಿಜ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ antBoolp = ಅಭಿವ್ಯಕ್ತಿ_Stmtp->resultp()->unlink_Fr_ಹಿಂಭಾಗ();
        ಅಭಿವ್ಯಕ್ತಿ_Stmtp->replaceWith(antBoolp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(ಅಭಿವ್ಯಕ್ತಿ_Stmtp), ಅಭಿವ್ಯಕ್ತಿ_Stmtp);
        parts.triggerExprp = antBoolp;
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Outcome counts for a property if/case are wrong in an outcome-multiplying
    // context. Returns that context, or nullptr when the shape is supported.
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* unsupportedPropertyControl(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Cover_Or_Assert* assertp,
                                                  ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* seqBodyp, ತಾರ್ಕಿಕ negated) {
        ಆದರೆ (!hasPropertyControlConjunction(seqBodyp)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (negated) ಹಿಂತಿರುಗಿ "negation";
        ಆದರೆ (VN_IS(assertp, Cover)) ಹಿಂತಿರುಗಿ "cover";
        ಆದರೆ (VN_AS(assertp, Assert)->passsp()) ಹಿಂತಿರುಗಿ "a pass action";
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }

    // Inline property/sequence refs and reject unsupported shapes.
    // Returns the PropSpec to lower, or nullptr when fully handled here.
    AstPropSpec* prepareAssertionProp(Ast_ಗಂಟು_Cover_Or_Assert* assertp) {
        ಆದರೆ (AstPropSpec* ಸ್ಥಿರಾಂಕ specp = VN_CAST(assertp->propp(), PropSpec)) {
            ಆದರೆ (AstFuncRef* ಸ್ಥಿರಾಂಕ funcrefp = VN_CAST(specp->propp(), FuncRef)) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstProperty* ಸ್ಥಿರಾಂಕ propyp = VN_CAST(funcrefp->taskp(), Property)) {
                    inlineNamedProperty(specp, funcrefp, propyp);
                }
            }
        }

        inlineAllSequenceRefs(assertp->propp());

        ಆದರೆ (AstPropSpec* ಸ್ಥಿರಾಂಕ specp = VN_CAST(assertp->propp(), PropSpec)) {
            ಆದರೆ (hoistClockedSeq(specp)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }

        AstPropSpec* ಸ್ಥಿರಾಂಕ propp = VN_AS(assertp->propp(), PropSpec);
        ಆದರೆ (!VN_IS(assertp, Cover)
            && effectiveAssertPropStrength(propp) == VPropStrength::STRONG) {
            propp->v3warn(E_UNSUPPORTED,
                          "Unsupported: strong property in " + assertp->verilogKwd() + ".");
            replaceBodyOnBuildError(assertp->fileline(), propp, /*errorEmitted=*/ನಿಜ);
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }

        ಆದರೆ (!has_Multi_ಚಕ್ರ_ಅಭಿವ್ಯಕ್ತಿ(propp)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        // A nested property instance keeps its body behind the call; lowering would drop it.
        ಆದರೆ (propp->exists([](ಸ್ಥಿರಾಂಕ AstFuncRef* refp) { ಹಿಂತಿರುಗಿ VN_IS(refp->taskp(), Property); })) {
            assertp->v3warn(E_UNSUPPORTED,
                            "Unsupported: property instance inside a multi-cycle property "
                            "expression");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(assertp->unlink_Fr_ಹಿಂಭಾಗ()), assertp);
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        ಆದರೆ (is_Bare_ತುದಿ_Level_Until(propp)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಹಿಂತಿರುಗಿ propp;
    }

    ಶೂನ್ಯ processAssertion(Ast_ಗಂಟು_Cover_Or_Assert* assertp) {
        ಆದರೆ (assertp->ತಕ್ಷಣ()) ಹಿಂತಿರುಗಿ;

        AstPropSpec* ಸ್ಥಿರಾಂಕ propp = prepareAssertionProp(assertp);
        ಆದರೆ (!propp) ಹಿಂತಿರುಗಿ;

        PropertyParts parts = decomposeProperty(propp);
        UASSERT_OBJ(parts.seqExprp, propp, "Property body must be an expression");

        // Unwrap `not` (IEEE 1800-2023 16.12.1); odd count -> negated semantics.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* seqBodyp = parts.seqExprp;
        ತಾರ್ಕಿಕ negated = ಸುಳ್ಳು;
        ಆಗ (AstLogNot* ಸ್ಥಿರಾಂಕ notp = VN_CAST(seqBodyp, LogNot)) {
            negated = !negated;
            seqBodyp = notp->lhsp();
        }

        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ propertyControlp
            = unsupportedPropertyControl(assertp, seqBodyp, negated);

        // Substitute property-local match-item refs in consequent with
        // $past(rhs, K) before NFA build (IEEE 1800-2023 16.10).
        ಆದರೆ (lift_ಹೊಂದಿಕೆ_ಅಂಶ_Substitutions(parts, seqBodyp)) {
            AstPropSpec* ಸ್ಥಿರಾಂಕ psp = VN_CAST(assertp->propp(), PropSpec);
            UASSERT_OBJ(psp, assertp, "Concurrent assertion must have PropSpec");
            replaceBodyOnBuildError(assertp->fileline(), psp, /*errorEmitted=*/ನಿಜ);
            ಹಿಂತಿರುಗಿ;
        }

        AstCover* ಸ್ಥಿರಾಂಕ coverp = VN_CAST(assertp, Cover);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isCoverSeq = coverp && coverp->isCoverSeq();
        // A sequence event control is not an assertion directive; no default
        // disable iff, no assertion control
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isSeqEvent = coverp && coverp->isSeqEvent();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಎಣಿಕೆ_Matches = isCoverSeq && !isSeqEvent;
        // Inherit module defaults (IEEE 14.12, 16.15) when assertion has none.
        ಆದರೆ (!propp->sensesp() && m_defaultClockingp) {
            propp->sensesp(m_defaultClockingp->sentreep()->sensesp()->clone_ವೃಕ್ಷ(ನಿಜ));
        }
        ಆದರೆ (!propp->disablep() && m_defaultDisablep && !isSeqEvent) {
            propp->disablep(m_defaultDisablep->condp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ));
        }
        ಆದರೆ (!propp->sensesp()) ಹಿಂತಿರುಗಿ;
        Ast_Sen_ವೃಕ್ಷ* senTreep
            = ಹೊಸ Ast_Sen_ವೃಕ್ಷ{propp->fileline(), propp->sensesp()->clone_ವೃಕ್ಷ(ನಿಜ)};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ disableExprp = propp->disablep();

        // NFA lowering clones repeated operands and may hoist them into an
        // always_comb block. Resolve implicit sampled-value clocks first, while
        // the enclosing assertion clock is still available.
        {
            VL_RESTORER(m_sampled_ಮೌಲ್ಯ_Clockp);
            m_sampled_ಮೌಲ್ಯ_Clockp = senTreep;
            iterate(propp->propp());
        }

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = assertp->fileline();

        Sva_ನಕ್ಷೆ ನಕ್ಷೆ;
        SvaNfaBuilder builder{ನಕ್ಷೆ,      m_modp,     m_prop_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು,
                              isCoverSeq, isSeqEvent, coverp != ಶೂನ್ಯಸೂಚಕ};

        ಸ್ಥಿರಾಂಕ Build_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = build_Assertion_ನಕ್ಷೆ(builder, ನಕ್ಷೆ, seqBodyp, parts, flp);
        ಆದರೆ (ಫಲಿತಾಂಶ.valid()) wire_ಹೊಂದಿಕೆ_And_Mid_Sources(ನಕ್ಷೆ, ಫಲಿತಾಂಶ, flp);
        ಆದರೆ (!ಫಲಿತಾಂಶ.valid()) {
            // Fall through to V3AssertPre for unsupported constructs; only
            // replace the body on real semantic errors. Any hoisted temps
            // from this attempt become orphan MODULETEMPs; V3Dead removes
            // them along with the dead always_comb driver.
            replaceBodyOnBuildError(flp, propp, ಫಲಿತಾಂಶ.errorEmitted);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(senTreep), senTreep);
            ಹಿಂತಿರುಗಿ;
        }
        // After the build, so a construct the builder rejects reports itself.
        ಆದರೆ (propertyControlp) {
            seqBodyp->v3warn(E_UNSUPPORTED,
                             "Unsupported: temporal property if/case with " << propertyControlp);
            replaceBodyOnBuildError(flp, propp, /*errorEmitted=*/ನಿಜ);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(senTreep), senTreep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (ಎಣಿಕೆ_Matches && !v3Global.opt.coverageUser()) {
            SvaNfaBuilder::freeUnlinkedCondp(ಫಲಿತಾಂಶ.finalCondp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(senTreep), senTreep);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ innerPropp = propp->propp();
            innerPropp->replaceWith(ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(innerPropp), innerPropp);
            ಹಿಂತಿರುಗಿ;
        }

        AstAssert* ಸ್ಥಿರಾಂಕ assertAssertp = VN_CAST(assertp, Assert);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ವಿಭಜಿಸು_Implication_Passsp = assertAssertp && assertAssertp->passsp()
                                            && parts.hasImplication
                                            && can_ವಿಭಜಿಸು_Implication_Pass_Actions(parts);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ need_ಹೊಂದಿಕೆ = assertAssertp && assertAssertp->passsp()
                               && (!parts.hasImplication || ವಿಭಜಿಸು_Implication_Passsp);

        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ needsThreadFailReplay
            = assertAssertp && assertAssertp->failsp() && !parts.hasImplication;
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಸಂಕೇತಗಳು
            = m_loweringp->ಅಧೋ(assertp, ನಕ್ಷೆ, senTreep, ಫಲಿತಾಂಶ.finalCondp, disableExprp,
                                 needsThreadFailReplay, needsThreadFailReplay && !negated);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಹೊಂದಿಕೆ_Exprp = ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* outputExprp = m_loweringp->assemble_ಫಲಿತಾಂಶ(
            assertp, negated, ಫಲಿತಾಂಶ.finalCondp, ಸಂಕೇತಗಳು, need_ಹೊಂದಿಕೆ ? &ಹೊಂದಿಕೆ_Exprp : ಶೂನ್ಯಸೂಚಕ);
        ಆದರೆ (ಎಣಿಕೆ_Matches) {
            UASSERT_OBJ(ಸಂಕೇತಗಳು.ಹೊಂದಿಕೆ_Countp, coverp, "Cover sequence missing match count");
            VL_DO_DANGLING(outputExprp->delete_ವೃಕ್ಷ(), outputExprp);
            propp->ಹೊಂದಿಕೆ_Countp(ಸಂಕೇತಗಳು.ಹೊಂದಿಕೆ_Countp);
            outputExprp = ಹೊಸ AstNeq{flp, ಸಂಕೇತಗಳು.ಹೊಂದಿಕೆ_Countp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                                     newTypedConstp(flp, ಸಂಕೇತಗಳು.ಹೊಂದಿಕೆ_Countp->dtypep(), 0)};
        }

        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ threadFailReplaySenTreep
            = ಸಂಕೇತಗಳು.threadFailCountp ? senTreep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು) : ಶೂನ್ಯಸೂಚಕ;

        VL_DO_DANGLING(ತಳ್ಳು_Deletep(senTreep), senTreep);
        ಆದರೆ (ಫಲಿತಾಂಶ.finalCondp && !ಫಲಿತಾಂಶ.finalCondp->backp()) ತಳ್ಳು_Deletep(ಫಲಿತಾಂಶ.finalCondp);

        ಆದರೆ (ವಿಭಜಿಸು_Implication_Passsp) {
            ವಿಭಜಿಸು_Implication_Pass_Actions(assertAssertp, parts, ಹೊಂದಿಕೆ_Exprp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            attachActionHandlers(assertAssertp, ಹೊಂದಿಕೆ_Exprp, threadFailReplaySenTreep,
                                 ಸಂಕೇತಗಳು.threadFailCountp);
        }

        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ innerPropp = propp->propp();
        innerPropp->replaceWith(outputExprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(innerPropp), innerPropp);

        UINFO(4, "NFA converted assertion at " << flp << ಸಾಲು_ಅಂತ್ಯ);

        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) ನಕ್ಷೆ.m_ನಕ್ಷೆ.dumpDotFilePrefixed("assert-nfa");
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_loweringp);
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultDisablep);
        m_modp = nodep;
        // A class nested in a module is in the scope of the module's default clocking
        ಆದರೆ (!VN_IS(nodep, Class)) m_defaultClockingp = ಶೂನ್ಯಸೂಚಕ;
        m_defaultDisablep = nodep->defaultDisablep();
        SvaNfaLowering lowering{nodep};
        m_loweringp = &lowering;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClocking* nodep) override {
        ಆದರೆ (nodep->isDefault() && !m_defaultClockingp) m_defaultClockingp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Gen_ಖಂಡ* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDefaultDisable* nodep) override {}
    ಶೂನ್ಯ ಭೇಟಿ(AstFell* nodep) override { ಭೇಟಿ_Sampled_ಮೌಲ್ಯ(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstPast* nodep) override { ಭೇಟಿ_Sampled_ಮೌಲ್ಯ(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstRose* nodep) override { ಭೇಟಿ_Sampled_ಮೌಲ್ಯ(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstStable* nodep) override { ಭೇಟಿ_Sampled_ಮೌಲ್ಯ(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstAssert* nodep) override { processAssertion(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstCover* nodep) override { processAssertion(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstRestrict* nodep) override {
        // Restrict property is ignored by simulators (IEEE 1800-2023 16.12.2).
        // Remove here so temporal SExpr don't leak to V3AssertPre.
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAssertIntrinsic* nodep) override {}
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ AssertNfaVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Top entry point

ಶೂನ್ಯ V3AssertNfa::assertNfaAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":" << ಸಾಲು_ಅಂತ್ಯ);
    { AssertNfaVisitor{nodep}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("assertnfa", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
