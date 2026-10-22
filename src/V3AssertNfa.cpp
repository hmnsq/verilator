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

ವರ್ಗ SvaStateVertex;

// Per-vertex algorithm data, stored via V3GraphVertex::userp() during lowering
ಸಂರಚನಾ SvaVertexData final {
    AstVar* stateVarp = nullptr;  // NBA state register for this vertex
    AstVar* delayRingVarp = nullptr;  // Packed occupancy bits or per-slot match counts
    AstVar* delayRingHeadVarp
        = nullptr;  // Cached outgoing slot; isolates readers from in-place writes
    AstVar* delayRingIdxVarp = nullptr;  // Next slot written in delayRingVarp
    AstVar* delay_Ring_Live_ಎಣಿಕೆ_Varp = nullptr;  // Number of threads in delayRingVarp
    AstVar* delayRingWrappedVarp = nullptr;  // All slots written since the last clear
    AstVar* doneLVarp = nullptr;  // SAnd LHS done-latch
    AstVar* doneRVarp = nullptr;  // SAnd RHS done-latch
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* stateSigp = nullptr;  // Combinational state signal; OWNED during lowering
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* state_ಎಣಿಕೆ_Sigp = nullptr;  // Cover-sequence match count; OWNED during lowering
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* state_ಎಣಿಕೆ_Overflow_Sigp = nullptr;  // Count overflow; OWNED during lowering
    ತಾರ್ಕಿಕ delayRingCounts = ಅಸತ್ಯ;  // Ring slots hold cover-sequence match counts
    ತಾರ್ಕಿಕ needsReg = ಅಸತ್ಯ;  // True if vertex has incoming clocked edge
};

// NFA state vertex -- one per NFA position in the sequence evaluation
ವರ್ಗ SvaStateVertex final : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(SvaStateVertex, V3GraphVertex)
ಸಾರ್ವಜನಿಕ:
    // True if this is the sequence-match terminal vertex
    ತಾರ್ಕಿಕ m_isMatch = ಅಸತ್ಯ;
    // OWNED throughout-guard condition clones; IEEE 1800-2023 16.9.9
    std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> m_throughoutConds;
    // Nonzero for a ring-buffer vertex for ## delays.
    ತಾರ್ಕಿಕ m_isFixedDelayRing = ಅಸತ್ಯ;
    unsigned m_delayRingSize = 0;  // Number of ring slots. Range: max-min+1.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_delayRingClearCondp = nullptr;  // local RHS for pure-boolean range
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_delayRingAdvanceCondp = nullptr;  // Advance only when this condition holds
    ತಾರ್ಕಿಕ m_replayAbortReject = ಅಸತ್ಯ;  // Compressed repetition needs per-thread abort replay
    // OWNED; enclosing-abort fire condition clearing state or suppressing guard rejection
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_abortClearp = nullptr;
    // OWNED; reject-abort fire condition rejecting all represented live threads
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_abortRejectp = nullptr;
    // Liveness terminal (IEEE weak semantics): reject must not fire from this source
    ತಾರ್ಕಿಕ m_isUnbounded = ಅಸತ್ಯ;
    // Temporal sequence AND combiner; IEEE 1800-2023 16.9.5
    ತಾರ್ಕಿಕ m_isAndCombiner = ಅಸತ್ಯ;
    SvaStateVertex* m_andLhsTermp = nullptr;  // LHS sub-NFA terminal vertex
    SvaStateVertex* m_andRhsTermp = nullptr;  // RHS sub-NFA terminal vertex
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_andLhsCondp = nullptr;  // OWNED; LHS final condition (may be nullptr)
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_andRhsCondp = nullptr;  // OWNED; RHS final condition (may be nullptr)
    // Reject sink for SAnd rejectOnFail wiring; not a state-signal source
    ತಾರ್ಕಿಕ m_isRejectSink = ಅಸತ್ಯ;
    // In-window vertex of a strong s_always[m:n]: if its state is still set at
    // end-of-simulation the universal-quantifier window never completed, which is
    // a liveness failure (IEEE 1800-2023 16.12.11 strong semantics).
    ತಾರ್ಕಿಕ m_strongPending = ಅಸತ್ಯ;
    // Ring introduced for the checked portion of a bounded strong s_always window.
    ತಾರ್ಕಿಕ m_strongAlwaysRing = ಅಸತ್ಯ;

    // CONSTRUCTORS
    explicit SvaStateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~SvaStateVertex() override {
        ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* cp : m_throughoutConds) VL_DO_DANGLING(cp->delete_ವೃಕ್ಷ(), cp);
        ಯದಿ (m_delayRingClearCondp)
            VL_DO_DANGLING(m_delayRingClearCondp->delete_ವೃಕ್ಷ(), m_delayRingClearCondp);
        ಯದಿ (m_delayRingAdvanceCondp)
            VL_DO_DANGLING(m_delayRingAdvanceCondp->delete_ವೃಕ್ಷ(), m_delayRingAdvanceCondp);
        ಯದಿ (m_abortClearp) VL_DO_DANGLING(m_abortClearp->delete_ವೃಕ್ಷ(), m_abortClearp);
        ಯದಿ (m_abortRejectp) VL_DO_DANGLING(m_abortRejectp->delete_ವೃಕ್ಷ(), m_abortRejectp);
        ಯದಿ (m_andLhsCondp) VL_DO_DANGLING(m_andLhsCondp->delete_ವೃಕ್ಷ(), m_andLhsCondp);
        ಯದಿ (m_andRhsCondp) VL_DO_DANGLING(m_andRhsCondp->delete_ವೃಕ್ಷ(), m_andRhsCondp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string name() ಧ್ರುವ override {
        string name = "s" + cvtToStr(color());
        ಯದಿ (m_delayRingSize) {
            name += "\\n";
            name += m_isFixedDelayRing ? "fixed chain " : "range chain ";
            name += cvtToStr(m_delayRingSize) + " slots";
        }
        ಪ್ರತ್ಯಾಗಮ name;
    }
    string dotColor() ಧ್ರುವ override {
        ಯದಿ (m_isMatch) ಪ್ರತ್ಯಾಗಮ "red";
        ಯದಿ (m_delayRingSize) ಪ್ರತ್ಯಾಗಮ "blue";
        ಯದಿ (m_isAndCombiner) ಪ್ರತ್ಯಾಗಮ "purple";
        ಪ್ರತ್ಯಾಗಮ "black";
    }
    // LCOV_EXCL_STOP
    // Access per-vertex algorithm data (valid only during lowering phase)
    SvaVertexData* datap() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ static_cast<SvaVertexData*>(userp()); }
};

// NFA transition edge -- clocked (##1) or combinational link (##0)
ವರ್ಗ SvaTransEdge final : ಸಾರ್ವಜನಿಕ V3GraphEdge {
    VL_RTTI_IMPL(SvaTransEdge, V3GraphEdge)
ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_condp;  // Transition condition; nullptr = unconditional; OWNED
    ತಾರ್ಕಿಕ m_consumesCycle;  // true = clocked edge (##1), false = link (##0/boolean)
    // Reject when source is active and condp is false; set only on
    // outermost required-step Link
    ತಾರ್ಕಿಕ m_rejectOnFail = ಅಸತ್ಯ;
    // Optional dynamic condition vertex for m_rejectOnFail. Used when the
    // success condition is another NFA state rather than a static expression.
    SvaStateVertex* m_condVtxp = nullptr;

    // CONSTRUCTORS
    SvaTransEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ತುದಿ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp,
                 ತಾರ್ಕಿಕ consumesCycle)
        : V3GraphEdge{graphp, fromp, ತುದಿ, /*weight=*/1}
        , m_condp{condp}
        , m_consumesCycle{consumesCycle} {}
    ~SvaTransEdge() override {
        ಯದಿ (m_condp) VL_DO_DANGLING(m_condp->delete_ವೃಕ್ಷ(), m_condp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string dotLabel() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_consumesCycle ? "##1" : "link"; }
    string dotStyle() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_consumesCycle ? "" : "dashed"; }
    // LCOV_EXCL_STOP
    // Typed accessors for NFA vertices
    SvaStateVertex* fromVtxp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ static_cast<SvaStateVertex*>(fromp()); }
    SvaStateVertex* toVtxp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ static_cast<SvaStateVertex*>(ತುದಿ()); }
};

// NFA graph container
ವರ್ಗ SvaGraph final {
ಸಾರ್ವಜನಿಕ:
    V3Graph m_graph;  // Owns all vertices and edges
    SvaStateVertex* m_startVertexp = nullptr;  // Trigger/start vertex
    SvaStateVertex* m_matchVertexp = nullptr;  // Sequence-match terminal vertex

    // Create a new state vertex
    SvaStateVertex* createStateVertex() { ಪ್ರತ್ಯಾಗಮ ನೂತನ SvaStateVertex{&m_graph}; }
    // Create the match terminal vertex
    SvaStateVertex* createMatchVertex() {
        SvaStateVertex* ಧ್ರುವ vtxp = createStateVertex();
        vtxp->m_isMatch = ಸತ್ಯ;
        m_matchVertexp = vtxp;
        ಪ್ರತ್ಯಾಗಮ vtxp;
    }
    // Add a clocked transition edge (##1)
    SvaTransEdge* addClockedEdge(SvaStateVertex* fromp, SvaStateVertex* ತುದಿ,
                                 Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = nullptr) {
        ಪ್ರತ್ಯಾಗಮ ನೂತನ SvaTransEdge{&m_graph, fromp, ತುದಿ, condp, /*consumesCycle=*/ಸತ್ಯ};
    }
    // Add a combinational link (##0 / boolean condition)
    SvaTransEdge* addLink(SvaStateVertex* fromp, SvaStateVertex* ತುದಿ,
                          Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = nullptr) {
        ಪ್ರತ್ಯಾಗಮ ನೂತನ SvaTransEdge{&m_graph, fromp, ತುದಿ, condp, /*consumesCycle=*/ಅಸತ್ಯ};
    }
    // Collect all edges into a flat vector for iteration.
    // Used by the lowering phase which needs global edge scans.
    std::vector<ಧ್ರುವ SvaTransEdge*> allEdges() ಧ್ರುವ {
        std::vector<ಧ್ರುವ SvaTransEdge*> result;
        ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtxr : m_graph.vertices()) {
            ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edger : vtxr.outEdges()) {
                result.ತಳ್ಳು_back(static_cast<ಧ್ರುವ SvaTransEdge*>(&edger));
            }
        }
        ಪ್ರತ್ಯಾಗಮ result;
    }
};

//######################################################################
// Builder result: terminal vertex + optional final condition (match Link condition).
ಸಂರಚನಾ BuildResult final {
    SvaStateVertex* termVertexp;  // Primary terminal; contributes to both match and reject
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* finalCondp;  // nullptr = unconditional
    // Mid-window sources for range delays (pure boolean RHS): match-only (isUnbounded)
    std::vector<SvaStateVertex*> midSources;
    ತಾರ್ಕಿಕ errorEmitted = ಅಸತ್ಯ;  // Builder already emitted specific error; skip generic
    // For cover_sequence: when true, midSources already enumerate every
    // end-of-match, so wireMatchAndMidSources must NOT add the main
    // termVtxp -> matchVertex Link (would double-count via the merge vertex).
    ತಾರ್ಕಿಕ termIsMidMerge = ಅಸತ್ಯ;
    ತಾರ್ಕಿಕ valid() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ termVertexp != nullptr; }
    ಸ್ಥಿರ BuildResult fail(ತಾರ್ಕಿಕ errored = ಅಸತ್ಯ) { ಪ್ರತ್ಯಾಗಮ {nullptr, nullptr, {}, errored}; }
    ಸ್ಥಿರ BuildResult failWithError() { ಪ್ರತ್ಯಾಗಮ {nullptr, nullptr, {}, ಸತ್ಯ}; }
};

// Parser-marked SAnd of overlapped implications: a property if/else/case.
ಸ್ಥಿರ ತಾರ್ಕಿಕ hasPropertyControlConjunction(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
    ಪ್ರತ್ಯಾಗಮ nodep->exists([](ಧ್ರುವ AstSAnd* andp) { ಪ್ರತ್ಯಾಗಮ andp->propertyControl(); });
}

ಸ್ಥಿರ AstConst* newTypedConstp(File_ಸಾಲು* ಧ್ರುವ flp, ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep,
                                ಧ್ರುವ uint32_t ಮೌಲ್ಯ) {
    AstConst* ಧ್ರುವ constp = ನೂತನ AstConst{flp, AstConst::DTyped{}, dtypep};
    constp->ಅಂಕೆ().setLong(ಮೌಲ್ಯ);
    ಪ್ರತ್ಯಾಗಮ constp;
}

ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sampled(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
    ಪ್ರತ್ಯಾಗಮ ನೂತನ AstSampled{exprp->fileline(), exprp, exprp->dtypep(), ಸತ್ಯ};
}

ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* assertPassOnQuery(ತಾರ್ಕಿಕ vacuous) {
    ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* queries[2]
        = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
    ಪ್ರತ್ಯಾಗಮ queries[vacuous];
}

ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* assertOnCond(File_ಸಾಲು* flp, VAssertType type,
                                 VAssertDirectiveType directiveType) {
    ಯದಿ (!v3Global.opt.assertOn()) { ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{flp, AstConst::BitFalse{}}; }
    ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
}

ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* assertKillGet(File_ಸಾಲು* flp, VAssertType type,
                                  VAssertDirectiveType directiveType) {
    ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_KILL", type, directiveType),
                        32};
}

ಸ್ಥಿರ string assertActionControlPrefix(VAssertDirectiveType directiveType) {
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ controlled = !!(static_cast<ಪೂರ್ಣಾಙ್ಕ>(directiveType)
                              & (static_cast<ಪೂರ್ಣಾಙ್ಕ>(VAssertDirectiveType::ASSERT)
                                 | static_cast<ಪೂರ್ಣಾಙ್ಕ>(VAssertDirectiveType::COVER)
                                 | static_cast<ಪೂರ್ಣಾಙ್ಕ>(VAssertDirectiveType::ASSUME)));
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ checkRuntime = controlled & static_cast<ಪೂರ್ಣಾಙ್ಕ>(v3Global.opt.assertOn());
    ಪ್ರತ್ಯಾಗಮ "("s + std::to_string(controlled ^ 1) + " || ("s + std::to_string(checkRuntime)
           + " && "s;
}

ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* assertPassOnCond(File_ಸಾಲು* flp, VAssertType type,
                                     VAssertDirectiveType directiveType, ತಾರ್ಕಿಕ vacuous) {
    ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType)
            + "))"s,
        1};
}

ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* assertFailOnCond(File_ಸಾಲು* flp, VAssertType type,
                                     VAssertDirectiveType directiveType) {
    ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType) + "))"s,
        1};
}

ಸ್ಥಿರ AstIf* newPassOnIf(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* firep, Ast_ಗಂಟು* bodyp, VAssertType type,
                          VAssertDirectiveType directiveType, ತಾರ್ಕಿಕ vacuous) {
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp
        = ನೂತನ AstLogAnd{flp, firep, assertPassOnCond(flp, type, directiveType, vacuous)};
    AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(ಸತ್ಯ);
    ifp->user1(ಸತ್ಯ);
    ಪ್ರತ್ಯಾಗಮ ifp;
}

ಸ್ಥಿರ Ast_ಗಂಟು_Stmt* newIfAssertFailOn(Ast_ಗಂಟು* bodyp, VAssertDirectiveType directiveType,
                                      VAssertType type) {
    File_ಸಾಲು* ಧ್ರುವ flp = bodyp->fileline();
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = assertFailOnCond(flp, type, directiveType);
    AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(ಸತ್ಯ);
    ifp->user1(ಸತ್ಯ);
    ಪ್ರತ್ಯಾಗಮ ifp;
}

//######################################################################
// NFA Builder

ವರ್ಗ SvaNfaBuilder final {
    SvaGraph& m_graph;  // NFA graph being built
    Ast_ಗಂಟು_Module* ಧ್ರುವ m_modp;  // Module to receive hoisted sampled-prop temps
    V3UniqueNames& m_propTempNames;  // Module-shared temp-var name source
    std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> m_temporal_Guard_ರಾಶಿ;  // Guards active across nested temporal states
    // Outer abort conditions, AND-ed as !cond into inner abort edges
    // (IEEE 1800-2023 16.12.14 outer-wraps-inner).
    std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> m_outer_Abort_ರಾಶಿ;
    ತಾರ್ಕಿಕ m_inUnboundedScope = ಅಸತ್ಯ;  // Sticky: nodes created after inherit liveness
    ತಾರ್ಕಿಕ m_markStrongPending = ಅಸತ್ಯ;  // Mark new vertices as strong s_always in-window
    ತಾರ್ಕಿಕ m_isCover = ಅಸತ್ಯ;  // Cover directives do not fail at end-of-simulation
    // IEEE 1800-2023 16.14.3 cover sequence: each end-of-match fires the action,
    // not just the first. Builder builds parallel-branch (no first-match-wins)
    // topology when true. Default false preserves cover_property semantics.
    ತಾರ್ಕಿಕ m_isCoverSeq = ಅಸತ್ಯ;
    // Unsupported endpoint topology must reject, not ignore, or the wait hangs
    ತಾರ್ಕಿಕ m_isSeqEvent = ಅಸತ್ಯ;
    ತಾರ್ಕಿಕ m_inSequencePrefix = ಅಸತ್ಯ;  // Prefix composition does not preserve every endpoint
    ತಾರ್ಕಿಕ m_wantBoolLeaf = ಅಸತ್ಯ;  // Building a |-> / |=> or top-level body (see buildOrMerge)

    ಸಂರಚನಾ RangeDelayRejectInfo final {
        SvaStateVertex* startp = nullptr;
        unsigned range = 0;
        ಪೂರ್ಣಾಙ್ಕ rhsLen = 0;
    };

    ಶೂನ್ಯ warnEndpointUnsupported(File_ಸಾಲು* flp, ಧ್ರುವ string& what) ಧ್ರುವ {
        ಯದಿ (m_isSeqEvent) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: sequence used as an event control with " << what);
        } ಅನ್ಯಥಾ {
            flp->v3warn(COVERIGN, "Ignoring unsupported: cover sequence with " << what);
        }
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* throughoutCond(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* baseCondp, File_ಸಾಲು* flp) {
        ಯದಿ (m_temporal_Guard_ರಾಶಿ.empty()) ಪ್ರತ್ಯಾಗಮ baseCondp;
        // AND all active temporal guards (supports nesting)
        // Each must use $sampled values.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* guardp = nullptr;
        ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp : m_temporal_Guard_ರಾಶಿ) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ clonep = sampled(condp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
            ಯದಿ (!guardp) {
                guardp = clonep;
            } ಅನ್ಯಥಾ {
                guardp = ನೂತನ AstLogAnd{flp, guardp, clonep};
            }
        }
        ಯದಿ (baseCondp) { guardp = ನೂತನ AstLogAnd{flp, baseCondp, guardp}; }
        ಪ್ರತ್ಯಾಗಮ guardp;
    }

    ಸ್ಥಿರ unsigned getConstUInt(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ constp = V3Const::constifyEdit(exprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
        ಧ್ರುವ AstConst* ಧ್ರುವ cp = VN_CAST(constp, Const);
        ಧ್ರುವ unsigned val = cp ? cp->toUInt() : 0;
        VL_DO_DANGLING(constp->delete_ವೃಕ್ಷ(), constp);
        ಪ್ರತ್ಯಾಗಮ val;
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
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ fixedLength(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಯದಿ (AstSExpr* ಧ್ರುವ sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ಧ್ರುವ delayp = VN_CAST(sexprp->delayp(), Delay);
            ಯದಿ (!delayp || !delayp->isCycleDelay()) ಪ್ರತ್ಯಾಗಮ -1;
            unsigned delayCycles;
            ಯದಿ (delayp->isRangeDelay()) {
                ಯದಿ (delayp->isUnbounded()) ಪ್ರತ್ಯಾಗಮ -1;  // LCOV_EXCL_LINE
                ಧ್ರುವ unsigned minD = getConstUInt(delayp->lhsp());
                ಧ್ರುವ unsigned maxD = getConstUInt(delayp->rhsp());
                ಯದಿ (minD != maxD) ಪ್ರತ್ಯಾಗಮ -1;
                delayCycles = minD;
            } ಅನ್ಯಥಾ {
                delayCycles = getConstUInt(delayp->lhsp());
            }
            ಪೂರ್ಣಾಙ್ಕ preLen = 0;
            ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ prep = sexprp->preExprp()) {
                preLen = fixedLength(prep);
                ಯದಿ (preLen < 0) ಪ್ರತ್ಯಾಗಮ -1;  // LCOV_EXCL_LINE
            }
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ bodyLen = fixedLength(sexprp->exprp());
            ಯದಿ (bodyLen < 0) ಪ್ರತ್ಯಾಗಮ -1;  // LCOV_EXCL_LINE
            ಪ್ರತ್ಯಾಗಮ preLen + delayCycles + bodyLen;
        }
        ಯದಿ (AstSThroughout* ಧ್ರುವ throughp = VN_CAST(nodep, SThroughout)) {
            ಪ್ರತ್ಯಾಗಮ fixedLength(throughp->rhsp());
        }
        ಯದಿ (AstSOr* ಧ್ರುವ orp = VN_CAST(nodep, SOr)) {
            // Alternatives must share one end cycle; buildSWithin relies on
            // this to pair the OR with an SIntersect.
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lhsLen = fixedLength(orp->lhsp());
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ rhsLen = fixedLength(orp->rhsp());
            ಯದಿ (lhsLen < 0 || rhsLen < 0 || lhsLen != rhsLen) ಪ್ರತ್ಯಾಗಮ -1;
            ಪ್ರತ್ಯಾಗಮ lhsLen;
        }
        ಯದಿ (AstSWithin* ಧ್ರುವ withinp = VN_CAST(nodep, SWithin)) {
            // `seq1 within seq2` ends at seq2's end cycle (IEEE 16.9.10).
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lhsLen = fixedLength(withinp->lhsp());
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ rhsLen = fixedLength(withinp->rhsp());
            ಯದಿ (lhsLen < 0 || rhsLen < 0 || lhsLen > rhsLen) ಪ್ರತ್ಯಾಗಮ -1;
            ಪ್ರತ್ಯಾಗಮ rhsLen;
        }
        // LCOV_EXCL_START -- defensive: V3AssertPre rejects composite SVA ops
        // nested in an intersect arm before fixedLength runs (clock-context
        // resolution fails). Kept as a guard in case future parser relaxations
        // permit it.
        ಯದಿ (nodep->isMultiCycleSva()) ಪ್ರತ್ಯಾಗಮ -1;
        // LCOV_EXCL_STOP
        // Plain boolean expression (no SVA constructs) -- 0 cycles.
        ಪ್ರತ್ಯಾಗಮ 0;
    }

    // Contiguous match-length range [lo,hi] for an operand whose length varies
    // from AT MOST ONE ranged cycle delay; {-1,-1} otherwise. Drives the
    // variable-length `intersect` lowering (IEEE 1800-2023 16.9.6): more than
    // one ranged delay would make the per-length realization ambiguous, so it
    // is reported unsupported rather than mis-paired.
    ಸ್ಥಿರ std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ> lengthRange(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಯದಿ (AstSExpr* ಧ್ರುವ sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ಧ್ರುವ delayp = VN_CAST(sexprp->delayp(), Delay);
            ಯದಿ (!delayp || !delayp->isCycleDelay()) ಪ್ರತ್ಯಾಗಮ {-1, -1};
            std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ> delayRange;
            ಯದಿ (delayp->isRangeDelay()) {
                ಯದಿ (delayp->isUnbounded()) ಪ್ರತ್ಯಾಗಮ {-1, -1};
                ಧ್ರುವ unsigned minD = getConstUInt(delayp->lhsp());
                ಧ್ರುವ unsigned maxD = getConstUInt(delayp->rhsp());
                delayRange = {minD, maxD};
            } ಅನ್ಯಥಾ {
                ಧ್ರುವ unsigned d = getConstUInt(delayp->lhsp());
                delayRange = {d, d};
            }
            std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ> preRange{0, 0};
            ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ prep = sexprp->preExprp()) {
                preRange = lengthRange(prep);
                ಯದಿ (preRange.first < 0) ಪ್ರತ್ಯಾಗಮ {-1, -1};
            }
            ಧ್ರುವ std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ> bodyRange = lengthRange(sexprp->exprp());
            ಯದಿ (bodyRange.first < 0) ಪ್ರತ್ಯಾಗಮ {-1, -1};
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ variableParts = (preRange.first != preRange.second)
                                      + (delayRange.first != delayRange.second)
                                      + (bodyRange.first != bodyRange.second);
            ಯದಿ (variableParts > 1) ಪ್ರತ್ಯಾಗಮ {-1, -1};
            ಪ್ರತ್ಯಾಗಮ {preRange.first + delayRange.first + bodyRange.first,
                    preRange.second + delayRange.second + bodyRange.second};
        }
        ಯದಿ (AstSThroughout* ಧ್ರುವ throughp = VN_CAST(nodep, SThroughout)) {
            ಪ್ರತ್ಯಾಗಮ lengthRange(throughp->rhsp());
        }
        ಯದಿ (nodep->isMultiCycleSva()) ಪ್ರತ್ಯಾಗಮ {-1, -1};
        ಪ್ರತ್ಯಾಗಮ {0, 0};  // plain boolean -- 0 cycles
    }

    // Clone `operand` with its sole variable ranged cycle delay pinned so the
    // total match length is exactly `len`. `lo` is the operand's minimum length
    // (lengthRange().first). A fixed operand has no such delay and is returned
    // as a plain clone (callers only request its single achievable length).
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* realizeAtLength(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* operand, ಪೂರ್ಣಾಙ್ಕ len, ಪೂರ್ಣಾಙ್ಕ lo) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ clonep = operand->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        AstDelay* rangeDelayp = nullptr;
        clonep->foreach([&](AstDelay* dp) {
            ಯದಿ (!rangeDelayp && dp->isRangeDelay() && !dp->isUnbounded()
                && getConstUInt(dp->lhsp()) != getConstUInt(dp->rhsp())) {
                rangeDelayp = dp;
            }
        });
        ಯದಿ (rangeDelayp) {
            File_ಸಾಲು* ಧ್ರುವ flp = rangeDelayp->fileline();
            ಧ್ರುವ unsigned pinned = getConstUInt(rangeDelayp->lhsp()) + (len - lo);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ oldMinp = rangeDelayp->lhsp();
            oldMinp->replaceWith(ನೂತನ AstConst{flp, pinned});
            VL_DO_DANGLING(oldMinp->delete_ವೃಕ್ಷ(), oldMinp);
            // Drop the max bound so it lowers as a fixed `##d`, not `##[d:d]`.
            Ast_ಗಂಟು* ಧ್ರುವ oldMaxp = rangeDelayp->rhsp()->unlinkFrBack();
            VL_DO_DANGLING(oldMaxp->delete_ವೃಕ್ಷ(), oldMaxp);
        }
        ಪ್ರತ್ಯಾಗಮ clonep;
    }

    // Cuts AST size from O(N * sizeof(exprp)) to O(N) + O(sizeof(exprp)) by
    // sharing a single `VarRef` across N check edges. Hoist also matches the
    // IEEE 1800-2023 16.9.9 "single preponed-region snapshot" semantic for
    // any exprp -- even an impure one would now evaluate exactly once per
    // clock instead of N times. Orphan temps from failed builds are unused
    // MODULETEMPs and are removed by V3Dead.
    AstVar* tryHoistSampled(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, File_ಸಾಲು* flp, unsigned clone_ಎಣಿಕೆ) {
        constexpr unsigned kHoistThreshold = 2;
        ಯದಿ (clone_ಎಣಿಕೆ < kHoistThreshold) ಪ್ರತ್ಯಾಗಮ nullptr;
        AstVar* ಧ್ರುವ tempVarp
            = ನೂತನ AstVar{flp, VVarType::MODULETEMP, m_propTempNames.get(exprp), exprp->dtypep()};
        m_modp->addStmtsp(tempVarp);
        AstAssign* ಧ್ರುವ assignp = ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, tempVarp, VAccess::WRITE},
                                                 sampled(exprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ))};
        m_modp->addStmtsp(ನೂತನ AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, assignp});
        ಪ್ರತ್ಯಾಗಮ tempVarp;
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sampledRefOrClone(AstVar* hoistVarp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, File_ಸಾಲು* flp) {
        ಯದಿ (hoistVarp) ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{flp, hoistVarp, VAccess::READ};
        ಪ್ರತ್ಯಾಗಮ sampled(exprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
    }

    // Create vertex and inherit temporal guards from the current scope.
    SvaStateVertex* scopedCreateVertex() {
        SvaStateVertex* ಧ್ರುವ vtxp = m_graph.createStateVertex();
        ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ cp : m_temporal_Guard_ರಾಶಿ) {
            vtxp->m_throughoutConds.ತಳ್ಳು_back(cp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
        }
        ಯದಿ (m_inUnboundedScope) vtxp->m_isUnbounded = ಸತ್ಯ;
        ಯದಿ (m_markStrongPending) vtxp->m_strongPending = ಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ vtxp;
    }

    // AND current temporal guards into every edge/link.
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* ತುದಿ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp,
                              File_ಸಾಲು* flp) {
        ಪ್ರತ್ಯಾಗಮ m_graph.addLink(fromp, ತುದಿ, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* ತುದಿ, File_ಸಾಲು* flp) {
        ಪ್ರತ್ಯಾಗಮ m_graph.addLink(fromp, ತುದಿ, throughoutCond(nullptr, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* ತುದಿ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp,
                              File_ಸಾಲು* flp) {
        ಪ್ರತ್ಯಾಗಮ m_graph.addClockedEdge(fromp, ತುದಿ, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* ತುದಿ, File_ಸಾಲು* flp) {
        ಪ್ರತ್ಯಾಗಮ m_graph.addClockedEdge(fromp, ತುದಿ, throughoutCond(nullptr, flp));
    }

    SvaStateVertex* addDelayChain(SvaStateVertex* startp, unsigned size, File_ಸಾಲು* flp,
                                  ತಾರ್ಕಿಕ isFixed = ಸತ್ಯ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* clearCondp = nullptr,
                                  Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* advanceCondp = nullptr) {
        ಯದಿ (isFixed && size == 0) ಪ್ರತ್ಯಾಗಮ startp;
        UASSERT_OBJ(size > 0, startp, "Delay chain needs at least one slot");
        ಯದಿ (isFixed && size == 1 && !advanceCondp) {
            SvaStateVertex* ಧ್ರುವ nextp = scopedCreateVertex();
            guardedEdge(startp, nextp, flp);
            ಪ್ರತ್ಯಾಗಮ nextp;
        }
        SvaStateVertex* ಧ್ರುವ ringVtxp = scopedCreateVertex();
        ringVtxp->m_isFixedDelayRing = isFixed;
        ringVtxp->m_delayRingSize = size;
        ಯದಿ (clearCondp) {
            UASSERT_OBJ(!isFixed, startp, "Fixed delay cannot have a clear condition");
            ringVtxp->m_delayRingClearCondp = clearCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        }
        ringVtxp->m_delayRingAdvanceCondp = advanceCondp;
        ಯದಿ (isFixed) {
            guardedEdge(startp, ringVtxp, flp);
        } ಅನ್ಯಥಾ {
            guardedLink(startp, ringVtxp, flp);
        }
        ಪ್ರತ್ಯಾಗಮ ringVtxp;
    }

    // Build NFA for an SExpr. finalCond = RHS (not yet added as a vertex).
    // isTopLevelStep: marks outermost required boolean check as rejectOnFail.
    // Apply a range delay `##[M:N]` to currentp. Returns true on success. On
    // failure, emits a diagnostic and returns false.
    ತಾರ್ಕಿಕ applyRangeDelay(AstSExpr* sexprp, SvaStateVertex*& currentp,
                         std::vector<SvaStateVertex*>& midSources,
                         RangeDelayRejectInfo* rangeRejectInfop = nullptr) {
        File_ಸಾಲು* ಧ್ರುವ flp = sexprp->fileline();
        AstDelay* ಧ್ರುವ delayp = VN_AS(sexprp->delayp(), Delay);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsExprp = sexprp->exprp();
        ಧ್ರುವ unsigned minDelay = getConstUInt(delayp->lhsp());
        ಯದಿ (delayp->isUnbounded()) {
            // `##[M:$]`: wait M cycles, then retain every matured attempt in a
            // one-slot ring while also exposing the first eligible tick.
            currentp = addDelayChain(currentp, minDelay, flp);
            SvaStateVertex* ಧ್ರುವ waitRingp = addDelayChain(currentp, 1, flp, ಅಸತ್ಯ);
            // The ring's live count is updated in NBA, so this link retains the previous
            // tick's live attempts alongside new arrivals for the next tick.
            guardedLink(waitRingp, waitRingp, flp);
            SvaStateVertex* ಧ್ರುವ mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(waitRingp, mergeVtxp, flp);
            currentp = mergeVtxp;
            currentp->m_isUnbounded = ಸತ್ಯ;  // Liveness, so no reject.
            m_inUnboundedScope = ಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಧ್ರುವ unsigned maxDelay = getConstUInt(delayp->rhsp());
        ಯದಿ (minDelay == maxDelay) {
            currentp = addDelayChain(currentp, minDelay, flp);
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಧ್ರುವ unsigned range = maxDelay - minDelay;
        currentp = addDelayChain(currentp, minDelay, flp);
        ಧ್ರುವ ತಾರ್ಕಿಕ multiCycleRhs = rhsExprp->isMultiCycleSva();
        ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ preExprp = sexprp->preExprp();
        // Prefix and nested-sequence composition can lose endpoints or their multiplicity
        // required by cover sequence (IEEE 1800-2023 16.14.3), regardless of the range width.
        ಯದಿ (m_isCoverSeq
            && (multiCycleRhs || m_inSequencePrefix
                || (preExprp && preExprp->isMultiCycleSva()))) {
            warnEndpointUnsupported(flp, "this ranged cycle delay");
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಯದಿ (m_inSequencePrefix) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: Bounded ranged cycle delay in a sequence prefix");
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಯದಿ (multiCycleRhs) {
            // Launch a candidate at every eligible tick. Candidate-local misses are not
            // assertion rejects while a later position can still match.
            ಯದಿ (rangeRejectInfop) {
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ rhsLen = fixedLength(rhsExprp);
                ಯದಿ (rhsLen >= 0) *rangeRejectInfop = {currentp, range, rhsLen};
            }
            SvaStateVertex* ಧ್ರುವ tailp = addDelayChain(currentp, range + 1U, flp, ಅಸತ್ಯ);
            SvaStateVertex* ಧ್ರುವ mergeVtxp = scopedCreateVertex();
            mergeVtxp->m_isUnbounded = ಸತ್ಯ;
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(tailp, mergeVtxp, flp);
            currentp = mergeVtxp;
            m_inUnboundedScope = ಸತ್ಯ;
        } ಅನ್ಯಥಾ {
            // The first eligible tick is explicit. The ring retains attempts for the
            // remaining ticks and exposes the final tick for rejection. Only properties
            // clear on success, since cover sequence counts every endpoint.
            midSources.ತಳ್ಳು_back(currentp);
            currentp = addDelayChain(currentp, range + 1U, flp, ಅಸತ್ಯ,
                                     m_isCoverSeq ? nullptr : rhsExprp);
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಶೂನ್ಯ addFiniteRangeReject(ಧ್ರುವ RangeDelayRejectInfo& info, ಧ್ರುವ BuildResult& result,
                              File_ಸಾಲು* flp) {
        ಯದಿ (!info.startp) ಪ್ರತ್ಯಾಗಮ;

        SvaStateVertex* ಧ್ರುವ expiryVtxp
            = addDelayChain(info.startp, info.range + info.rhsLen, flp);
        SvaStateVertex* ಧ್ರುವ expiryMatchp = scopedCreateVertex();
        std::vector<SvaStateVertex*> sources = result.midSources;
        sources.ತಳ್ಳು_back(result.termVertexp);
        ಪ್ರತಿ (SvaStateVertex* ಧ್ರುವ srcp : sources) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp
                = result.finalCondp ? sampled(result.finalCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)) : nullptr;
            SvaStateVertex* ಧ್ರುವ successNowp = scopedCreateVertex();
            guardedLink(srcp, successNowp, condp, flp);
            // Retain a success through the latest deadline it can satisfy.
            SvaStateVertex* ಧ್ರುವ historyp
                = addDelayChain(successNowp, info.range + 1U, flp, ಅಸತ್ಯ);
            guardedLink(successNowp, expiryMatchp, flp);
            guardedLink(historyp, expiryMatchp, flp);
        }

        SvaStateVertex* ಧ್ರುವ sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = ಸತ್ಯ;
        SvaTransEdge* ಧ್ರುವ rejectp = m_graph.addLink(expiryVtxp, sinkVtxp);
        rejectp->m_rejectOnFail = ಸತ್ಯ;
        rejectp->m_condVtxp = expiryMatchp;
    }

    BuildResult buildSExpr(AstSExpr* sexprp, SvaStateVertex* entryVtxp,
                           ತಾರ್ಕಿಕ is_ತುದಿ_Level_Step = ಅಸತ್ಯ) {
        AstDelay* ಧ್ರುವ delayp = VN_CAST(sexprp->delayp(), Delay);
        ಯದಿ (!delayp || !delayp->isCycleDelay()) ಪ್ರತ್ಯಾಗಮ BuildResult::fail();

        File_ಸಾಲು* ಧ್ರುವ flp = sexprp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = sexprp->exprp();

        // Handle LHS (preExpr)
        SvaStateVertex* currentp = entryVtxp;
        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ preExprp = sexprp->preExprp()) {
            VL_RESTORER(m_inSequencePrefix);
            m_inSequencePrefix = ಸತ್ಯ;
            ಧ್ರುವ BuildResult pre = build_ಅಭಿವ್ಯಕ್ತಿ(preExprp, currentp, is_ತುದಿ_Level_Step);
            ಯದಿ (!pre.valid()) ಪ್ರತ್ಯಾಗಮ BuildResult::fail(pre.errorEmitted);  // LCOV_EXCL_LINE
            ಯದಿ (pre.finalCondp) {
                SvaStateVertex* ಧ್ರುವ condVtxp = scopedCreateVertex();
                SvaTransEdge* ಧ್ರುವ edgep = guardedLink(
                    pre.termVertexp, condVtxp, sampled(pre.finalCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)), flp);
                ಯದಿ (is_ತುದಿ_Level_Step && !pre.termVertexp->m_isUnbounded && !m_inUnboundedScope) {
                    // Do not mark liveness sources: first boolean check is deferred.
                    edgep->m_rejectOnFail = ಸತ್ಯ;
                }
                freeUnlinkedCondp(pre.finalCondp);
                currentp = condVtxp;
            } ಅನ್ಯಥಾ {
                currentp = pre.termVertexp;
            }
        }

        // Handle delay
        std::vector<SvaStateVertex*> rangeMidSources;
        RangeDelayRejectInfo rangeRejectInfo;
        ಧ್ರುವ ತಾರ್ಕಿಕ addRangeReject = is_ತುದಿ_Level_Step && !m_inUnboundedScope;
        ಯದಿ (delayp->isRangeDelay()) {
            ಯದಿ (!applyRangeDelay(sexprp, currentp, rangeMidSources,
                                 addRangeReject ? &rangeRejectInfo : nullptr)) {
                ಪ್ರತ್ಯಾಗಮ BuildResult::failWithError();
            }
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ unsigned delayCycles = getConstUInt(delayp->lhsp());
            currentp = addDelayChain(currentp, delayCycles, flp);
        }

        // Multi-cycle RHS: recurse (only plain boolean is returned as finalCondp).
        ಯದಿ (exprp->isMultiCycleSva()) {
            ಧ್ರುವ BuildResult result = build_ಅಭಿವ್ಯಕ್ತಿ(exprp, currentp, is_ತುದಿ_Level_Step);
            ಯದಿ (result.valid()) addFiniteRangeReject(rangeRejectInfo, result, flp);
            ಪ್ರತ್ಯಾಗಮ result;
        }
        ಪ್ರತ್ಯಾಗಮ {currentp, exprp, std::move(rangeMidSources)};
    }

    BuildResult buildConsRep(AstSConsRep* repp, SvaStateVertex* entryVtxp,
                             ತಾರ್ಕಿಕ is_ತುದಿ_Level_Step = ಅಸತ್ಯ) {
        File_ಸಾಲು* ಧ್ರುವ flp = repp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = repp->exprp();
        // Multi-cycle expr in ConsRep not yet supported; bail to avoid invalid AST.
        ಯದಿ (exprp->isMultiCycleSva()) {
            repp->v3warn(E_UNSUPPORTED, "Unsupported: multi-cycle sequence expression inside"
                                        " consecutive repetition (IEEE 1800-2023 16.9.2)");
            ಪ್ರತ್ಯಾಗಮ BuildResult::failWithError();
        }
        ಧ್ರುವ unsigned minN = getConstUInt(repp->countp());

        // Sum sites across prefix + unbounded/range tail so one hoist covers
        // every check edge of this repetition.
        unsigned totalSites = minN;
        ಯದಿ (repp->unbounded()) {
            totalSites += 1;
        } ಅನ್ಯಥಾ ಯದಿ (repp->maxCountp()) {
            totalSites += getConstUInt(repp->maxCountp()) - minN;
        }
        AstVar* ಧ್ರುವ hoistVarp = tryHoistSampled(exprp, flp, totalSites);

        // Cover-sequence (IEEE 1800-2023 16.14.3): collect each end-of-match
        // position so they all fire the action, not just the merged terminal.
        std::vector<SvaStateVertex*> consMidSources;

        SvaStateVertex* currentp = entryVtxp;
        ಪ್ರತಿ (unsigned i = 0; i < minN; ++i) {
            // Keep the first repetition explicit, collapse all remaining checks into the ring.
            ಯದಿ (i == 1) {
                currentp = addDelayChain(currentp, minN - 1, flp);
                currentp->m_delayRingClearCondp
                    = ನೂತನ AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                currentp->m_replayAbortReject = ಸತ್ಯ;
                ಯದಿ (is_ತುದಿ_Level_Step) {
                    currentp->m_throughoutConds.ತಳ್ಳು_back(
                        sampledRefOrClone(hoistVarp, exprp, flp));
                }
                i = minN - 1;
            }
            // Every repetition in the minimum prefix is required.
            SvaStateVertex* ಧ್ರುವ condVtxp = scopedCreateVertex();
            SvaTransEdge* ಧ್ರುವ linkp
                = guardedLink(currentp, condVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            // Only an outermost required step rejects the property. Its first check is explicit,
            // later required checks reject through the ring's throughout guard.
            ಯದಿ (is_ತುದಿ_Level_Step && i == 0) linkp->m_rejectOnFail = ಸತ್ಯ;
            currentp = condVtxp;
        }
        // After minN: currentp is the first valid end-of-match position for [*m:n].
        ಯದಿ (m_isCoverSeq && (repp->unbounded() || repp->maxCountp())) {
            consMidSources.ತಳ್ಳು_back(currentp);
        }

        ಯದಿ (repp->unbounded()) {
            ಯದಿ (minN == 0) {
                SvaStateVertex* ಧ್ರುವ waitVtxp = scopedCreateVertex();
                guardedEdge(currentp, waitVtxp, flp);
                SvaStateVertex* ಧ್ರುವ checkVtxp = scopedCreateVertex();
                guardedLink(waitVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedEdge(checkVtxp, waitVtxp, flp);
                guardedLink(currentp, checkVtxp, flp);
                currentp = checkVtxp;
            } ಅನ್ಯಥಾ {
                SvaStateVertex* ಧ್ರುವ loopBackVtxp = scopedCreateVertex();
                guardedEdge(currentp, loopBackVtxp, flp);
                SvaStateVertex* ಧ್ರುವ reCheckVtxp = scopedCreateVertex();
                guardedLink(loopBackVtxp, reCheckVtxp, sampledRefOrClone(hoistVarp, exprp, flp),
                            flp);
                guardedEdge(reCheckVtxp, loopBackVtxp, flp);
                guardedLink(reCheckVtxp, currentp, flp);
            }
            currentp->m_isUnbounded = ಸತ್ಯ;
            m_inUnboundedScope = ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (repp->maxCountp()) {
            ಧ್ರುವ unsigned maxN = getConstUInt(repp->maxCountp());
            SvaStateVertex* ಧ್ರುವ mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            unsigned tailMinN = minN;
            SvaStateVertex* tailStartp = currentp;
            ಯದಿ (minN == 0) {
                // Build the first optional iteration explicitly. Feeding the empty endpoint
                // directly into a range ring would incorrectly keep that match alive.
                SvaStateVertex* ಧ್ರುವ nextVtxp = scopedCreateVertex();
                guardedEdge(currentp, nextVtxp, flp);
                SvaStateVertex* ಧ್ರುವ checkVtxp = scopedCreateVertex();
                guardedLink(nextVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                ಯದಿ (m_isCoverSeq) consMidSources.ತಳ್ಳು_back(checkVtxp);
                tailStartp = checkVtxp;
                tailMinN = 1;
            }
            ಯದಿ (maxN > tailMinN) {
                // Add tail-ring only if the tail is non-empty.
                SvaStateVertex* ಧ್ರುವ nextVtxp
                    = addDelayChain(tailStartp, maxN - tailMinN + 1, flp, ಅಸತ್ಯ);
                nextVtxp->m_delayRingClearCondp
                    = ನೂತನ AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                nextVtxp->m_replayAbortReject = ಸತ್ಯ;
                SvaStateVertex* ಧ್ರುವ checkVtxp = scopedCreateVertex();
                guardedLink(nextVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                ಯದಿ (m_isCoverSeq) consMidSources.ತಳ್ಳು_back(checkVtxp);
            }
            currentp = mergeVtxp;
        }
        // finalCond = nullptr (already checked via Links)
        BuildResult res;
        res.termVertexp = currentp;
        res.finalCondp = nullptr;
        res.midSources = std::move(consMidSources);
        // mergeVtxp is the OR of all the end-positions we already pushed to
        // midSources, so the main termVtxp -> matchVertex Link would duplicate.
        res.termIsMidMerge = m_isCoverSeq && !res.midSources.empty();
        ಪ್ರತ್ಯಾಗಮ res;
    }

    // always[lo:hi] / s_always[lo:hi] (IEEE 1800-2023 16.12.11).
    BuildResult buildPropAlways(AstPropAlways* nodep, SvaStateVertex* entryVtxp,
                                ತಾರ್ಕಿಕ is_ತುದಿ_Level_Step = ಅಸತ್ಯ) {
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ propp = nodep->propp();
        ಧ್ರುವ unsigned lo = getConstUInt(nodep->loBoundp());
        ಯದಿ (VN_IS(nodep->hiBoundp(), Unbounded)) {
            // Weak always [lo:$]: unbounded upper bound (IEEE 1800-2023 16.12.11).
            // p must hold at every clock tick at least lo cycles after the attempt
            // start; those ticks are not required to exist, so there is no
            // end-of-trace obligation (weak). The self-loop keeps the attempt live
            // every cycle; each observed cycle is a safety obligation, so a false p
            // rejects immediately.
            UASSERT_OBJ(!nodep->isStrong(), nodep, "Unbounded always must be weak (V3Width)");
            SvaStateVertex* ಧ್ರುವ livep = addDelayChain(entryVtxp, lo, flp);
            livep->m_isUnbounded = ಸತ್ಯ;
            guardedEdge(livep, livep, flp);  // stay active every subsequent cycle
            SvaStateVertex* ಧ್ರುವ sinkp = m_graph.createStateVertex();
            sinkp->m_isRejectSink = ಸತ್ಯ;
            SvaTransEdge* ಧ್ರುವ rejEdgep
                = guardedLink(livep, sinkp, sampled(propp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)), flp);
            ಯದಿ (is_ತುದಿ_Level_Step) rejEdgep->m_rejectOnFail = ಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ {livep, nullptr, {}};
        }
        ಧ್ರುವ unsigned hi = getConstUInt(nodep->hiBoundp());
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
        SvaStateVertex* currentp = addDelayChain(entryVtxp, lo, flp);
        SvaStateVertex* ಧ್ರುವ checkp = scopedCreateVertex();
        SvaTransEdge* ಧ್ರುವ linkp
            = guardedLink(currentp, checkp, sampled(propp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)), flp);
        ಯದಿ (is_ತುದಿ_Level_Step) linkp->m_rejectOnFail = ಸತ್ಯ;
        currentp = checkp;
        m_temporal_Guard_ರಾಶಿ.ತಳ್ಳು_back(propp);
        currentp = addDelayChain(currentp, hi - lo, flp);
        ಯದಿ (nodep->isStrong() && currentp->m_delayRingSize) currentp->m_strongAlwaysRing = ಸತ್ಯ;
        m_temporal_Guard_ರಾಶಿ.ತೆಗೆ_back();
        ಪ್ರತ್ಯಾಗಮ {currentp, propp, {}};
    }

    BuildResult buildGotoRep(AstSGotoRep* repp, SvaStateVertex* entryVtxp) {
        File_ಸಾಲು* ಧ್ರುವ flp = repp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = repp->exprp();
        ಧ್ರುವ unsigned minN = getConstUInt(repp->countp());
        ಯದಿ (minN == 0) ಪ್ರತ್ಯಾಗಮ BuildResult::fail();
        ಧ್ರುವ ತಾರ್ಕಿಕ hasMax = repp->maxCountp() != nullptr;
        ಧ್ರುವ unsigned maxN = hasMax ? getConstUInt(repp->maxCountp()) : minN;
        ಯದಿ (m_isCoverSeq) {
            // Several attempts may coalesce in the one-bit wait vertex before reaching the ring,
            // so a cover sequence action block could run too few times.
            warnEndpointUnsupported(flp, "a goto repetition");
            ಪ್ರತ್ಯಾಗಮ BuildResult::failWithError();
        }

        AstVar* ಧ್ರುವ hoistVarp = tryHoistSampled(exprp, flp, 2);

        // The first guardedEdge is the ##1 before waiting for a match. In the wait state, false
        // takes the clocked self-loop, while true takes the zero-delay guardedLink on that tick.
        SvaStateVertex* ಧ್ರುವ waitVtxp = scopedCreateVertex();
        guardedEdge(entryVtxp, waitVtxp, flp);
        guardedEdge(waitVtxp, waitVtxp,
                    ನೂತನ AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)}, flp);
        SvaStateVertex* currentp = scopedCreateVertex();
        guardedLink(waitVtxp, currentp, sampledRefOrClone(hoistVarp, exprp, flp), flp);

        ಯದಿ (minN > 1) {
            SvaStateVertex* ಧ್ರುವ ringVtxp = addDelayChain(
                currentp, minN - 1, flp, ಸತ್ಯ, nullptr, sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = ಸತ್ಯ;
            SvaStateVertex* ಧ್ರುವ checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            currentp = checkVtxp;
        }
        ಯದಿ (!hasMax) {
            currentp->m_isUnbounded = ಸತ್ಯ;  // [->N] waits unboundedly
            m_inUnboundedScope = ಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ {currentp, nullptr, {}};
        }

        // [->M:N]: the range ring holds matches from M through N and advances
        // only on expr, preserving arbitrarily long gaps between occurrences.
        SvaStateVertex* ಧ್ರುವ mergeVtxp = scopedCreateVertex();
        guardedLink(currentp, mergeVtxp, flp);  // accept at match_M
        ಯದಿ (maxN > minN) {
            SvaStateVertex* ಧ್ರುವ ringVtxp
                = addDelayChain(currentp, maxN - minN + 1, flp, ಅಸತ್ಯ, nullptr,
                                sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = ಸತ್ಯ;
            SvaStateVertex* ಧ್ರುವ checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            guardedLink(checkVtxp, mergeVtxp, flp);
        }
        mergeVtxp->m_isUnbounded = ಸತ್ಯ;  // [->M:N] still has unbounded waits between matches
        m_inUnboundedScope = ಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ {mergeVtxp, nullptr, {}};
    }

    // True if the sequence has a zero-minimum consecutive repetition, which may match empty
    ಸ್ಥಿರ ತಾರ್ಕಿಕ hasZeroMinRep(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* seqp) {
        ಪ್ರತ್ಯಾಗಮ seqp->exists(
            [](ಧ್ರುವ AstSConsRep* repp) { ಪ್ರತ್ಯಾಗಮ getConstUInt(repp->countp()) == 0; });
    }

    // True if a leaf's condition has no implication or multi-cycle sequence in it
    // (a leaf can be e.g. 'not (x |-> y)')
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isPlainBoolean(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        ಪ್ರತ್ಯಾಗಮ !condp->exists(
            [](ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* np) { ಪ್ರತ್ಯಾಗಮ np->isMultiCycleSva() || VN_IS(np, Implication); });
    }

    // Build merge vertex for SOr / LogOr: both branches feed into one vertex.
    BuildResult buildOrMerge(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, SvaStateVertex* entryVtxp,
                             File_ಸಾಲು* flp, ತಾರ್ಕಿಕ wantBoolLeaf) {
        ಧ್ರುವ BuildResult lhs = build_ಅಭಿವ್ಯಕ್ತಿ(lhsp, entryVtxp);
        ಧ್ರುವ BuildResult rhs = build_ಅಭಿವ್ಯಕ್ತಿ(rhsp, entryVtxp);
        ಯದಿ (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ಪ್ರತ್ಯಾಗಮ BuildResult::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP
        // IEEE 1800-2023 16.14.3: a cover sequence counts every end-of-match. A
        // sequence operand of 'or' can end more than once, but only its final
        // end reaches the merge vertex below, so reject sequence operands rather
        // than under-count. Plain boolean disjunction has one end per cycle and
        // is handled by the OR-fold.
        ಧ್ರುವ ತಾರ್ಕಿಕ booleanOnly = lhs.termVertexp == entryVtxp && lhs.finalCondp
                                 && rhs.termVertexp == entryVtxp && rhs.finalCondp;
        ಯದಿ (m_isCoverSeq && !booleanOnly) {
            warnEndpointUnsupported(flp, "a sequence operand of 'or'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ಪ್ರತ್ಯಾಗಮ BuildResult::failWithError();
        }
        ಯದಿ (booleanOnly && wantBoolLeaf) {
            ಯದಿ (isPlainBoolean(lhs.finalCondp) && isPlainBoolean(rhs.finalCondp)) {
                // As a |-> / |=> or top-level body, a disjunction of two plain booleans must
                // be able to reject: return it as a boolean finalCondp. The merge vertex
                // below drops the attempt silently when both operands are false.
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ orCondp
                    = ನೂತನ AstLogOr{flp, lhs.finalCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                                   rhs.finalCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                freeUnlinkedCondp(lhs.finalCondp);
                freeUnlinkedCondp(rhs.finalCondp);
                ಪ್ರತ್ಯಾಗಮ {entryVtxp, orCondp, {}};
            }
        }
        SvaStateVertex* ಧ್ರುವ mergeVtxp = scopedCreateVertex();
        ಯದಿ (booleanOnly) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsp,
                        "Single-cycle SOr operands must have finalCondp");
            guardedLink(entryVtxp, mergeVtxp,
                        ನೂತನ AstLogOr{flp, sampled(lhs.finalCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)),
                                     sampled(rhs.finalCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ))},
                        flp);
        } ಅನ್ಯಥಾ {
            ಯದಿ (lhs.finalCondp) {
                guardedLink(lhs.termVertexp, mergeVtxp,
                            sampled(lhs.finalCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)), flp);
            } ಅನ್ಯಥಾ {
                guardedLink(lhs.termVertexp, mergeVtxp, flp);
            }
            ಯದಿ (rhs.finalCondp) {
                guardedLink(rhs.termVertexp, mergeVtxp,
                            sampled(rhs.finalCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)), flp);
            } ಅನ್ಯಥಾ {
                guardedLink(rhs.termVertexp, mergeVtxp, flp);
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        ಪ್ರತ್ಯಾಗಮ {mergeVtxp, nullptr, {}};
    }

    // Build done-latch combiner for SAnd/SIntersect (IEEE 1800-2023 16.9.5).
    BuildResult buildAndCombiner(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsExprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsExprp,
                                 SvaStateVertex* entryVtxp, File_ಸಾಲು* flp) {
        // Snapshot-restore scope so LHS liveness does not leak into RHS.
        ಧ್ರುವ ತಾರ್ಕಿಕ savedScope = m_inUnboundedScope;
        ಧ್ರುವ BuildResult lhs = build_ಅಭಿವ್ಯಕ್ತಿ(lhsExprp, entryVtxp);
        ಧ್ರುವ ತಾರ್ಕಿಕ lhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope;
        ಧ್ರುವ BuildResult rhs = build_ಅಭಿವ್ಯಕ್ತಿ(rhsExprp, entryVtxp);
        ಧ್ರುವ ತಾರ್ಕಿಕ rhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope || lhsScope || rhsScope;
        ಯದಿ (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ಪ್ರತ್ಯಾಗಮ BuildResult::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP

        // Single-cycle operands: use boolean AND (done-latch would fire across cycles).
        // If both operands stayed at entry, they must be boolean leaves which
        // buildExpr returns with finalCondp=nodep (non-null).
        ಯದಿ (lhs.termVertexp == entryVtxp && rhs.termVertexp == entryVtxp) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsExprp,
                        "Single-cycle SAnd operands must have finalCondp");
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = ನೂತನ AstLogAnd{flp, lhs.finalCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                                                     rhs.finalCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ಪ್ರತ್ಯಾಗಮ {entryVtxp, condp, {}};
        }
        // Range-delay mid-window sources in either sub-branch would need
        // to be folded into the latch's match-now signal, which the
        // current combiner does not support.
        ಯದಿ (!lhs.midSources.empty() || !rhs.midSources.empty()) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: ranged cycle delay in an operand of property 'and'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ಪ್ರತ್ಯಾಗಮ BuildResult::failWithError();
        }
        SvaStateVertex* ಧ್ರುವ combVtxp = scopedCreateVertex();
        combVtxp->m_isAndCombiner = ಸತ್ಯ;
        combVtxp->m_andLhsTermp = lhs.termVertexp;
        combVtxp->m_andRhsTermp = rhs.termVertexp;
        ಯದಿ (lhs.finalCondp) combVtxp->m_andLhsCondp = lhs.finalCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        ಯದಿ (rhs.finalCondp) combVtxp->m_andRhsCondp = rhs.finalCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        ಯದಿ (lhs.termVertexp->m_isUnbounded || rhs.termVertexp->m_isUnbounded) {
            combVtxp->m_isUnbounded = ಸತ್ಯ;
        }
        // Wire terminal-boolean rejects to a dedicated sink so each side can fail
        // the AND independently. Skip for liveness or single-cycle operands
        // (single-cycle termVertexp == entryVtxp would fire every cycle).
        ಯದಿ (!combVtxp->m_isUnbounded) {
            ತಾರ್ಕಿಕ needSink = ಅಸತ್ಯ;
            ಧ್ರುವ ತಾರ್ಕಿಕ lhsMultiCycle = (lhs.termVertexp != entryVtxp);
            ಧ್ರುವ ತಾರ್ಕಿಕ rhsMultiCycle = (rhs.termVertexp != entryVtxp);
            ಯದಿ (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                needSink = ಸತ್ಯ;
            }
            ಯದಿ (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                needSink = ಸತ್ಯ;
            }
            ಯದಿ (needSink) {
                SvaStateVertex* ಧ್ರುವ sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = ಸತ್ಯ;
                ಯದಿ (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* ಧ್ರುವ ep = m_graph.addLink(
                        lhs.termVertexp, sinkVtxp, sampled(lhs.finalCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)));
                    ep->m_rejectOnFail = ಸತ್ಯ;
                }
                ಯದಿ (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* ಧ್ರುವ ep = m_graph.addLink(
                        rhs.termVertexp, sinkVtxp, sampled(rhs.finalCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)));
                    ep->m_rejectOnFail = ಸತ್ಯ;
                }
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        ಪ್ರತ್ಯಾಗಮ {combVtxp, nullptr, {}};
    }

    // Lower `seq1 within seq2` (IEEE 1800-2023 16.9.10) as:
    //   (OR_{i in 0..slack} 1 ##i seq1 ##(slack-i) 1) intersect seq2
    // Both operands must have fixed length (no ranged cycle delays).
    // The OR lives inside a single SIntersect so one AndCombiner done-latch
    // serves all offsets; lifting it out would double-count matches where
    // multiple offsets accept on the same seq2 end cycle.
    BuildResult buildSWithin(AstSWithin* nodep, SvaStateVertex* entryVtxp,
                             ತಾರ್ಕಿಕ is_ತುದಿ_Level_Step = ಅಸತ್ಯ) {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ innerLen = fixedLength(nodep->lhsp());
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ outerLen = fixedLength(nodep->rhsp());
        ಯದಿ (innerLen < 0 || outerLen < 0) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: within with ranged cycle-delay operand");
            ಪ್ರತ್ಯಾಗಮ BuildResult::failWithError();
        }
        ಯದಿ (innerLen > outerLen) {
            ಪ್ರತ್ಯಾಗಮ buildNeverMatchIntersect(
                nodep, entryVtxp, is_ತುದಿ_Level_Step,
                "the inner sequence is longer than the outer sequence");
        }
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ slack = outerLen - innerLen;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* innerOrp = nullptr;
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i <= slack; ++i) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ postPad = slack - i;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* branchp = nodep->lhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            ಯದಿ (i > 0) {
                AstConst* ಧ್ರುವ prePadp = ನೂತನ AstConst{flp, AstConst::BitTrue{}};
                AstDelay* ಧ್ರುವ delayp
                    = ನೂತನ AstDelay{flp, ನೂತನ AstConst{flp, static_cast<uint32_t>(i)}, ಸತ್ಯ};
                AstSExpr* ಧ್ರುವ wrapped = ನೂತನ AstSExpr{flp, prePadp, delayp, branchp};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            ಯದಿ (postPad > 0) {
                AstConst* ಧ್ರುವ postTruep = ನೂತನ AstConst{flp, AstConst::BitTrue{}};
                AstDelay* ಧ್ರುವ delayp
                    = ನೂತನ AstDelay{flp, ನೂತನ AstConst{flp, static_cast<uint32_t>(postPad)}, ಸತ್ಯ};
                AstSExpr* ಧ್ರುವ wrapped = ನೂತನ AstSExpr{flp, branchp, delayp, postTruep};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            innerOrp = innerOrp ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstSOr{flp, innerOrp, branchp})
                                : branchp;
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ outerClonep = nodep->rhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ combinedp = ನೂತನ AstSIntersect{flp, innerOrp, outerClonep};
        BuildResult result = build_ಅಭಿವ್ಯಕ್ತಿ(combinedp, entryVtxp, is_ತುದಿ_Level_Step);
        VL_DO_DANGLING(combinedp->delete_ವೃಕ್ಷ(), combinedp);
        // When both operands are plain booleans, buildAndCombiner returns a
        // freshly-allocated AstAnd as finalCondp with no parent. Callers up
        // the chain clone-and-discard finalCondp, so leave it parent-less and
        // it leaks; anchor it in the graph now via an edge.
        ಯದಿ (result.valid() && result.finalCondp && !result.finalCondp->backp()) {
            SvaStateVertex* ಧ್ರುವ wrapVtxp = scopedCreateVertex();
            guardedLink(result.termVertexp, wrapVtxp, sampled(result.finalCondp), flp);
            result = {wrapVtxp, nullptr, result.midSources};
        }
        ಪ್ರತ್ಯಾಗಮ result;
    }

    // Collect the boolean leaf checks of a fixed-length sequence keyed by their
    // clock offset from the start. Returns false for anything other than nested
    // AstSExpr with fixed cycle delays over boolean leaves (e.g. throughout).
    ಸ್ಥಿರ ತಾರ್ಕಿಕ flattenFixedSeq(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಪೂರ್ಣಾಙ್ಕ baseOffset,
                                std::map<ಪೂರ್ಣಾಙ್ಕ, std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>>& out) {
        ಯದಿ (AstSExpr* ಧ್ರುವ sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ಧ್ರುವ delayp = VN_CAST(sexprp->delayp(), Delay);
            ಯದಿ (!delayp || !delayp->isCycleDelay() || delayp->isUnbounded()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಧ್ರುವ unsigned delayCycles = getConstUInt(delayp->lhsp());
            ಯದಿ (delayp->isRangeDelay() && getConstUInt(delayp->rhsp()) != delayCycles)
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಪೂರ್ಣಾಙ್ಕ preLen = 0;
            ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ prep = sexprp->preExprp()) {
                ಯದಿ (!flattenFixedSeq(prep, baseOffset, out)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                preLen = fixedLength(prep);
                ಯದಿ (preLen < 0) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            ಪ್ರತ್ಯಾಗಮ flattenFixedSeq(sexprp->exprp(), baseOffset + preLen + delayCycles, out);
        }
        ಯದಿ (nodep->isMultiCycleSva()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        out[baseOffset].ತಳ್ಳು_back(nodep);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // Conjoin two equal-length fixed sequences into one: at each clock offset
    // AND the boolean checks of both operands (IEEE 1800-2023 16.9.6 -- both
    // operands match the same window). Returns null if either operand is not a
    // plain fixed sequence of boolean leaves.
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* conjoinFixedSeqs(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, File_ಸಾಲು* flp) {
        std::map<ಪೂರ್ಣಾಙ್ಕ, std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>> checks;
        ಯದಿ (!flattenFixedSeq(lhsp, 0, checks) || !flattenFixedSeq(rhsp, 0, checks)) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಯದಿ (checks.empty()) ಪ್ರತ್ಯಾಗಮ nullptr;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = nullptr;
        ಪೂರ್ಣಾಙ್ಕ prevOffset = 0;
        ಪ್ರತಿ (ಧ್ರುವ auto& offsetChecks : checks) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ offset = offsetChecks.first;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = nullptr;
            ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ leafp : offsetChecks.second) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ clonep = leafp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                ಯದಿ (!condp) {
                    condp = clonep;
                } ಅನ್ಯಥಾ {
                    condp = ನೂತನ AstLogAnd{flp, condp, clonep};
                    condp->dtypeSetBit();
                }
            }
            ಯದಿ (!resultp) {
                ಯದಿ (offset > 0) {
                    AstDelay* ಧ್ರುವ delayp = ನೂತನ AstDelay{
                        flp, ನೂತನ AstConst{flp, static_cast<uint32_t>(offset)}, /*isCycle=*/ಸತ್ಯ};
                    resultp
                        = ನೂತನ AstSExpr{flp, ನೂತನ AstConst{flp, AstConst::BitTrue{}}, delayp, condp};
                    resultp->dtypeSetBit();
                } ಅನ್ಯಥಾ {
                    resultp = condp;
                }
            } ಅನ್ಯಥಾ {
                AstDelay* ಧ್ರುವ delayp = ನೂತನ AstDelay{
                    flp, ನೂತನ AstConst{flp, static_cast<uint32_t>(offset - prevOffset)},
                    /*isCycle=*/ಸತ್ಯ};
                resultp = ನೂತನ AstSExpr{flp, resultp, delayp, condp};
                resultp->dtypeSetBit();
            }
            prevOffset = offset;
        }
        ಪ್ರತ್ಯಾಗಮ resultp;
    }

    // `seq` is a simple ranged sequence `start ##[m:n] end` (start/end boolean,
    // start may be absent). Used to collapse a both-variable intersect to one
    // ranged delay.
    ಸಂರಚನಾ SimpleRanged final {
        ತಾರ್ಕಿಕ ok = ಅಸತ್ಯ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* startp = nullptr;  // may be null (absent start)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* endp = nullptr;
    };
    ಸ್ಥಿರ SimpleRanged asSimpleRanged(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        AstSExpr* ಧ್ರುವ sexprp = VN_CAST(nodep, SExpr);
        ಯದಿ (!sexprp) ಪ್ರತ್ಯಾಗಮ {};
        AstDelay* ಧ್ರುವ delayp = VN_CAST(sexprp->delayp(), Delay);
        ಯದಿ (!delayp || !delayp->isCycleDelay() || !delayp->isRangeDelay() || delayp->isUnbounded())
            ಪ್ರತ್ಯಾಗಮ {};
        ಯದಿ (getConstUInt(delayp->lhsp()) == getConstUInt(delayp->rhsp())) ಪ್ರತ್ಯಾಗಮ {};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ prep = sexprp->preExprp();
        ಯದಿ (prep && fixedLength(prep) != 0) ಪ್ರತ್ಯಾಗಮ {};
        ಯದಿ (fixedLength(sexprp->exprp()) != 0) ಪ್ರತ್ಯಾಗಮ {};
        ಪ್ರತ್ಯಾಗಮ {ಸತ್ಯ, prep, sexprp->exprp()};
    }

    // Build the NFA for a synthesized intersect lowering tree, then free it.
    // buildExpr returns the terminal condition (finalCondp) by reference into the
    // tree; detach a clone so the tree can be freed here. The graph already holds
    // clones/hoists of every edge condition, so nothing else dangles.
    BuildResult build_From_Lowering_ವೃಕ್ಷ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* treep, SvaStateVertex* entryVtxp,
                                      ತಾರ್ಕಿಕ is_ತುದಿ_Level_Step) {
        BuildResult result = build_ಅಭಿವ್ಯಕ್ತಿ(treep, entryVtxp, is_ತುದಿ_Level_Step);
        ಯದಿ (result.valid() && result.finalCondp) {
            result.finalCondp = result.finalCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        }
        VL_DO_DANGLING(treep->delete_ವೃಕ್ಷ(), treep);
        ಪ್ರತ್ಯಾಗಮ result;
    }

    // Empty common-length intersection -- unequal fixed lengths, or disjoint
    // ranged lengths. IEEE 1800-2023 16.9.6 requires both operands to match
    // over a window of the same length, so with no common length the intersect
    // simply never matches. This is legal (matching nothing), not an error, so
    // lower to a constant false rather than rejecting legal code.
    BuildResult buildNeverMatchIntersect(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, SvaStateVertex* entryVtxp,
                                         ತಾರ್ಕಿಕ is_ತುದಿ_Level_Step, ಧ್ರುವ ವರ್ಣ* reason) {
        nodep->v3warn(NEVERMATCH, "Sequence can never match because " << reason << ".");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ falsep = ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalse{}};
        ಪ್ರತ್ಯಾಗಮ build_From_Lowering_ವೃಕ್ಷ(falsep, entryVtxp, is_ತುದಿ_Level_Step);
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
    BuildResult buildVarLenIntersect(AstSIntersect* nodep, SvaStateVertex* entryVtxp,
                                     ತಾರ್ಕಿಕ is_ತುದಿ_Level_Step) {
        ಧ್ರುವ std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ> lhsRange = lengthRange(nodep->lhsp());
        ಧ್ರುವ std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ> rhsRange = lengthRange(nodep->rhsp());
        ಯದಿ (lhsRange.first < 0 || rhsRange.first < 0) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect with this variable-length operand");
            ಪ್ರತ್ಯಾಗಮ BuildResult::failWithError();
        }
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lo = std::max(lhsRange.first, rhsRange.first);
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ hi = std::min(lhsRange.second, rhsRange.second);
        ಯದಿ (lo > hi) {
            // Disjoint length ranges share no common length -> never matches.
            ಪ್ರತ್ಯಾಗಮ buildNeverMatchIntersect(nodep, entryVtxp, is_ತುದಿ_Level_Step,
                                            "intersect operands have no common length");
        }
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        ಯದಿ (lo == hi) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lp = realizeAtLength(nodep->lhsp(), lo, lhsRange.first);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rp = realizeAtLength(nodep->rhsp(), lo, rhsRange.first);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ conjp = conjoinFixedSeqs(lp, rp, flp);
            VL_DO_DANGLING(lp->delete_ವೃಕ್ಷ(), lp);
            VL_DO_DANGLING(rp->delete_ವೃಕ್ಷ(), rp);
            ಯದಿ (!conjp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: intersect operand is not a plain boolean sequence");
                ಪ್ರತ್ಯಾಗಮ BuildResult::failWithError();
            }
            ಪ್ರತ್ಯಾಗಮ build_From_Lowering_ವೃಕ್ಷ(conjp, entryVtxp, is_ತುದಿ_Level_Step);
        }
        ಧ್ರುವ SimpleRanged sl = asSimpleRanged(nodep->lhsp());
        ಧ್ರುವ SimpleRanged sr = asSimpleRanged(nodep->rhsp());
        ಯದಿ (!sl.ok || !sr.ok) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect of two sequences that each vary in length over a"
                          " range with internal structure");
            ಪ್ರತ್ಯಾಗಮ BuildResult::failWithError();
        }
        ಧ್ರುವ auto andBool = [&](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ap, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bp) -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ aClonep
                = ap ? ap->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ) : ನೂತನ AstConst{flp, AstConst::BitTrue{}};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ bClonep
                = bp ? bp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ) : ನೂತನ AstConst{flp, AstConst::BitTrue{}};
            AstLogAnd* ಧ್ರುವ andp = ನೂತನ AstLogAnd{flp, aClonep, bClonep};
            andp->dtypeSetBit();
            ಪ್ರತ್ಯಾಗಮ andp;
        };
        AstDelay* ಧ್ರುವ delayp = ನೂತನ AstDelay{flp, ನೂತನ AstConst{flp, static_cast<uint32_t>(lo)},
                                              /*isCycle=*/ಸತ್ಯ};
        delayp->rhsp(ನೂತನ AstConst{flp, static_cast<uint32_t>(hi)});
        AstSExpr* ಧ್ರುವ reducedp
            = ನೂತನ AstSExpr{flp, andBool(sl.startp, sr.startp), delayp, andBool(sl.endp, sr.endp)};
        reducedp->dtypeSetBit();
        ಪ್ರತ್ಯಾಗಮ build_From_Lowering_ವೃಕ್ಷ(reducedp, entryVtxp, is_ತುದಿ_Level_Step);
    }

    BuildResult buildThroughout(AstSThroughout* nodep, SvaStateVertex* entryVtxp,
                                ತಾರ್ಕಿಕ is_ತುದಿ_Level_Step = ಅಸತ್ಯ) {
        // Mark entryVtxp so "cond false at tick 0" is detected as throughout-drop.
        entryVtxp->m_throughoutConds.ತಳ್ಳು_back(nodep->lhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
        m_temporal_Guard_ರಾಶಿ.ತಳ್ಳು_back(nodep->lhsp());
        ಧ್ರುವ BuildResult result = build_ಅಭಿವ್ಯಕ್ತಿ(nodep->rhsp(), entryVtxp, is_ತುದಿ_Level_Step);
        m_temporal_Guard_ರಾಶಿ.ತೆಗೆ_back();
        ಪ್ರತ್ಯಾಗಮ result;
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
    BuildResult buildUntil(AstUntil* nodep, SvaStateVertex* entryVtxp, ತಾರ್ಕಿಕ is_ತುದಿ_Level_Step) {
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        ಯದಿ (!is_ತುದಿ_Level_Step) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            ಪ್ರತ್ಯಾಗಮ BuildResult::failWithError();
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp();
        ಧ್ರುವ auto hasSeq = [](ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ep) {
            ಪ್ರತ್ಯಾಗಮ ep->exists([](ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* np) { ಪ್ರತ್ಯಾಗಮ np->isMultiCycleSva(); });
        };
        ಯದಿ (hasSeq(lhsp) || hasSeq(rhsp)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            ಪ್ರತ್ಯಾಗಮ BuildResult::failWithError();
        }

        ಧ್ರುವ ತಾರ್ಕಿಕ ov = nodep->isOverlapping();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsBitp = nodep->lhsp();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsBitp = nodep->rhsp();
        // p hoist count: continue, require (ov: 1 use; nov: 1 use). At least 2 uses.
        AstVar* ಧ್ರುವ pHoistp = tryHoistSampled(lhsBitp, flp, 2);
        // q hoist count: continue (1) + require nov (1) = 2; ov: continue only (1).
        AstVar* ಧ್ರುವ qHoistp = ov ? nullptr : tryHoistSampled(rhsBitp, flp, 2);

        SvaStateVertex* ಧ್ರುವ waitCp = scopedCreateVertex();
        SvaStateVertex* ಧ್ರುವ waitRp = scopedCreateVertex();
        waitCp->m_isUnbounded = ಸತ್ಯ;
        waitRp->m_strongPending = nodep->isStrong() && !m_isCover;

        // Entry and back-loop Links carry no condition; throughout-folding still applies.
        guardedLink(entryVtxp, waitCp, flp);
        guardedLink(waitRp, waitCp, flp);

        // Continue clocked edge: p && !q advances to next-cycle wait.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ contCondp
            = ನೂತನ AstLogAnd{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                            ನೂತನ AstLogNot{flp, sampledRefOrClone(qHoistp, rhsBitp, flp)}};
        guardedEdge(waitCp, waitRp, contCondp, flp);

        // Reject sink: fires when require-condition is false.
        SvaStateVertex* ಧ್ರುವ sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = ಸತ್ಯ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* requireCondp;
        ಯದಿ (ov) {
            requireCondp = sampledRefOrClone(pHoistp, lhsBitp, flp);
        } ಅನ್ಯಥಾ {
            requireCondp = ನೂತನ AstLogOr{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                                        sampledRefOrClone(qHoistp, rhsBitp, flp)};
        }
        SvaTransEdge* ಧ್ರುವ rejEdgep = m_graph.addLink(waitCp, sinkVtxp, requireCondp);
        rejEdgep->m_rejectOnFail = ಸತ್ಯ;

        // Accept condition rides via finalCondp; assembleResult $sampled-wraps it.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* acceptCondp;
        ಯದಿ (ov) {
            acceptCondp
                = ನೂತನ AstLogAnd{flp, lhsBitp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), rhsBitp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
        } ಅನ್ಯಥಾ {
            acceptCondp = rhsBitp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        }
        ಪ್ರತ್ಯಾಗಮ {waitCp, acceptCondp, {}};
    }

    // IEEE 1800-2023 16.12.14 property abort operators. Sync and async share
    // the same NFA encoding: AstSampled already gives matured values at every
    // maturing clocking event, and async firing "between clocks" is not
    // observable in a cycle-based model. VAbortKind selects accept vs reject
    // verdict (sync vs async only changes the user-visible spelling).

    // Build `condp && !outer_1 && !outer_2 ...` (unsampled).
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* abort_Fire_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, File_ಸಾಲು* flp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = condp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ op : m_outer_Abort_ರಾಶಿ)
            resultp = ನೂತನ AstLogAnd{flp, resultp, ನೂತನ AstLogNot{flp, op->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)}};
        ಪ್ರತ್ಯಾಗಮ resultp;
    }

    // True when unguarded ring-wide rejection or a same-tick Link chain already accounts the
    // attempt: a required-step Link covers both outcomes; followed-by pairs both edges.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ chainAccountsSource(ಧ್ರುವ SvaStateVertex* srcp,
                                    ಧ್ರುವ std::unordered_set<ಧ್ರುವ V3GraphEdge*>& preEdges) {
        ಯದಿ (srcp->m_delayRingSize && srcp->m_throughoutConds.empty()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edger : srcp->inEdges()) {
            ಯದಿ (preEdges.ಎಣಿಕೆ(&edger)) ಅನುವರ್ತ;
            ಧ್ರುವ SvaTransEdge& tedger = static_cast<ಧ್ರುವ SvaTransEdge&>(edger);
            ಯದಿ (tedger.m_consumesCycle) ಅನುವರ್ತ;
            ಧ್ರುವ auto* ಧ್ರುವ fromp = static_cast<ಧ್ರುವ SvaStateVertex*>(tedger.fromVtxp());
            ಯದಿ (fromp->m_abortRejectp) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ತಾರ್ಕಿಕ plainNonSink = ಅಸತ್ಯ;
        ತಾರ್ಕಿಕ markedSink = ಅಸತ್ಯ;
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edger : srcp->outEdges()) {
            ಯದಿ (preEdges.ಎಣಿಕೆ(&edger)) ಅನುವರ್ತ;
            ಧ್ರುವ SvaTransEdge& tedger = static_cast<ಧ್ರುವ SvaTransEdge&>(edger);
            ಯದಿ (tedger.m_consumesCycle) ಅನುವರ್ತ;
            ಧ್ರುವ ತಾರ್ಕಿಕ sink = static_cast<ಧ್ರುವ SvaStateVertex*>(tedger.toVtxp())->m_isRejectSink;
            ಯದಿ (tedger.m_rejectOnFail) {
                ಯದಿ (!sink) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                markedSink = ಸತ್ಯ;
            } ಅನ್ಯಥಾ ಯದಿ (!sink) {
                ಯದಿ (srcp->m_abortRejectp) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                plainNonSink = ಸತ್ಯ;
            }
        }
        ಪ್ರತ್ಯಾಗಮ plainNonSink && markedSink;
    }

    // Reject edge: fires when the source is live and the abort samples true.
    ಶೂನ್ಯ addAbortRejectEdge(SvaStateVertex* srcp, SvaStateVertex* sinkp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp,
                            File_ಸಾಲು* flp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ notFirep = ನೂತನ AstLogNot{flp, sampled(abort_Fire_ಅಭಿವ್ಯಕ್ತಿ(condp, flp))};
        m_graph.addLink(srcp, sinkp, notFirep)->m_rejectOnFail = ಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ;
    }

    // On the fire tick: kill body threads; accept kinds also forgive step misses.
    ಶೂನ್ಯ gateBodyEdgesOnAbort(ಧ್ರುವ std::unordered_set<ಧ್ರುವ V3GraphEdge*>& preEdges,
                              Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, VAbortKind kind, File_ಸಾಲು* flp) {
        ಪ್ರತಿ (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            ಪ್ರತಿ (V3GraphEdge& edger : vtxr.outEdges()) {
                ಯದಿ (preEdges.ಎಣಿಕೆ(&edger)) ಅನುವರ್ತ;
                SvaTransEdge* ಧ್ರುವ tedgep = static_cast<SvaTransEdge*>(&edger);
                ಯದಿ (tedgep->m_rejectOnFail) {
                    ಯದಿ (!kind.isAccept()) ಅನುವರ್ತ;
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ firep = sampled(abort_Fire_ಅಭಿವ್ಯಕ್ತಿ(condp, flp));
                    tedgep->m_condp
                        = tedgep->m_condp ? ನೂತನ AstLogOr{flp, tedgep->m_condp, firep} : firep;
                } ಅನ್ಯಥಾ ಯದಿ (tedgep->m_consumesCycle) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ notFirep
                        = ನೂತನ AstLogNot{flp, sampled(abort_Fire_ಅಭಿವ್ಯಕ್ತಿ(condp, flp))};
                    tedgep->m_condp = tedgep->m_condp
                                          ? ನೂತನ AstLogAnd{flp, tedgep->m_condp, notFirep}
                                          : notFirep;
                }
            }
        }
    }

    BuildResult buildAbortOn(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bodyp, SvaStateVertex* entryVtxp,
                             VAbortKind kind, File_ಸಾಲು* flp, ತಾರ್ಕಿಕ is_ತುದಿ_Level_Step) {
        // Snapshot pre-body vertices/edges so post-build diff yields the body's sub-NFA.
        std::unordered_set<ಧ್ರುವ V3GraphVertex*> preExisting;
        std::unordered_set<ಧ್ರುವ V3GraphEdge*> preEdges;
        ಪ್ರತಿ (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            preExisting.ಸೇರಿಸು(&vtxr);
            ಪ್ರತಿ (V3GraphEdge& edger : vtxr.outEdges()) preEdges.ಸೇರಿಸು(&edger);
        }

        m_outer_Abort_ರಾಶಿ.ತಳ್ಳು_back(condp);
        ಧ್ರುವ BuildResult bodyResult = build_ಅಭಿವ್ಯಕ್ತಿ(bodyp, entryVtxp, is_ತುದಿ_Level_Step);
        m_outer_Abort_ರಾಶಿ.ತೆಗೆ_back();
        ಯದಿ (!bodyResult.valid()) ಪ್ರತ್ಯಾಗಮ bodyResult;

        gateBodyEdgesOnAbort(preEdges, condp, kind, flp);

        // Live-thread sources for the abort edge: entry + new body vertices,
        // minus reject sinks (they carry reject fuel, not live-thread fuel).
        std::vector<SvaStateVertex*> abortSources;
        abortSources.ತಳ್ಳು_back(entryVtxp);
        ಪ್ರತಿ (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            ಯದಿ (preExisting.ಎಣಿಕೆ(&vtxr)) ಅನುವರ್ತ;
            auto* ಧ್ರುವ sp = static_cast<SvaStateVertex*>(&vtxr);
            ಯದಿ (sp->m_delayRingSize || !sp->m_throughoutConds.empty()) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ firep = abort_Fire_ಅಭಿವ್ಯಕ್ತಿ(condp, flp);
                sp->m_abortClearp
                    = sp->m_abortClearp ? ನೂತನ AstLogOr{flp, sp->m_abortClearp, firep} : firep;
            }
            ಯದಿ (!kind.isAccept()
                && ((sp->m_delayRingSize && sp->m_throughoutConds.empty())
                    || sp->m_replayAbortReject)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ firep = abort_Fire_ಅಭಿವ್ಯಕ್ತಿ(condp, flp);
                sp->m_abortRejectp
                    = sp->m_abortRejectp ? ನೂತನ AstLogOr{flp, sp->m_abortRejectp, firep} : firep;
            }
            ಯದಿ (sp->m_isRejectSink) ಅನುವರ್ತ;
            abortSources.ತಳ್ಳು_back(sp);
        }

        auto sampledAbortFire = [&]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ಅಭಿವ್ಯಕ್ತಿ = abort_Fire_ಅಭಿವ್ಯಕ್ತಿ(condp, flp);
            ಪ್ರತ್ಯಾಗಮ sampled(ಅಭಿವ್ಯಕ್ತಿ);
        };

        ಯದಿ (kind.isAccept()) {
            // Match-only sink fed by $sampled(abort-fire) from every live source;
            // registered as midSource so it never contributes a reject.
            SvaStateVertex* ಧ್ರುವ acceptSinkp = scopedCreateVertex();
            ಪ್ರತಿ (SvaStateVertex* ಧ್ರುವ srcp : abortSources)
                guardedLink(srcp, acceptSinkp, sampledAbortFire(), flp);
            std::vector<SvaStateVertex*> midSources = bodyResult.midSources;
            midSources.ತಳ್ಳು_back(acceptSinkp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* finalCondp = bodyResult.finalCondp;
            ಯದಿ (finalCondp) {
                ಯದಿ (finalCondp->backp()) finalCondp = finalCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                finalCondp = ನೂತನ AstLogOr{flp, finalCondp, abort_Fire_ಅಭಿವ್ಯಕ್ತಿ(condp, flp)};
            }
            ಪ್ರತ್ಯಾಗಮ {bodyResult.termVertexp, finalCondp, std::move(midSources)};
        }

        // rejectOnFail treats m_condp as the success condition and fires on
        // !condp, so the edge carries !sampledAbortFire().
        SvaStateVertex* ಧ್ರುವ rejectSinkp = m_graph.createStateVertex();
        rejectSinkp->m_isRejectSink = ಸತ್ಯ;
        ಪ್ರತಿ (SvaStateVertex* ಧ್ರುವ srcp : abortSources)
            ಯದಿ (!chainAccountsSource(srcp, preEdges))
                addAbortRejectEdge(srcp, rejectSinkp, condp, flp);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* finalCondp = bodyResult.finalCondp;
        ಯದಿ (finalCondp) {
            ಯದಿ (finalCondp->backp()) finalCondp = finalCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            finalCondp
                = ನೂತನ AstLogAnd{flp, finalCondp, ನೂತನ AstLogNot{flp, abort_Fire_ಅಭಿವ್ಯಕ್ತಿ(condp, flp)}};
        }
        ಪ್ರತ್ಯಾಗಮ {bodyResult.termVertexp, finalCondp, bodyResult.midSources};
    }

ಸಾರ್ವಜನಿಕ:
    // Free a dropped sub-result condition that is not linked into the AST
    // (abort folds synthesize unparented finalCondp trees).
    ಸ್ಥಿರ ಶೂನ್ಯ freeUnlinkedCondp(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        ಯದಿ (condp && !condp->backp()) VL_DO_DANGLING(condp->delete_ವೃಕ್ಷ(), condp);
    }

    SvaNfaBuilder(SvaGraph& graph, Ast_ಗಂಟು_Module* modp, V3UniqueNames& propTempNames,
                  ತಾರ್ಕಿಕ isCoverSeq = ಅಸತ್ಯ, ತಾರ್ಕಿಕ isSeqEvent = ಅಸತ್ಯ, ತಾರ್ಕಿಕ isCover = ಅಸತ್ಯ)
        : m_graph{graph}
        , m_modp{modp}
        , m_propTempNames{propTempNames}
        , m_isCover{isCover}
        , m_isCoverSeq{isCoverSeq}
        , m_isSeqEvent{isSeqEvent} {}

    // Reset scope between antecedent and consequent: liveness must not leak.
    // m_outerAbortStack survives: an abort wrapping the implication covers the
    // consequent too (IEEE 1800-2023 16.12.14).
    ಶೂನ್ಯ resetScope() {
        m_inUnboundedScope = ಅಸತ್ಯ;
        m_temporal_Guard_ರಾಶಿ.clear();
    }

    BuildResult build_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, SvaStateVertex* entryVtxp,
                          ತಾರ್ಕಿಕ is_ತುದಿ_Level_Step = ಅಸತ್ಯ) {
        VL_RESTORER(m_wantBoolLeaf);
        ಧ್ರುವ ತಾರ್ಕಿಕ wantBoolLeaf = m_wantBoolLeaf;
        m_wantBoolLeaf = ಅಸತ್ಯ;
        ಯದಿ (AstSExpr* ಧ್ರುವ sexprp = VN_CAST(nodep, SExpr)) {
            ಪ್ರತ್ಯಾಗಮ buildSExpr(sexprp, entryVtxp, is_ತುದಿ_Level_Step);
        }
        ಯದಿ (AstSConsRep* ಧ್ರುವ repp = VN_CAST(nodep, SConsRep)) {
            ಪ್ರತ್ಯಾಗಮ buildConsRep(repp, entryVtxp, is_ತುದಿ_Level_Step);
        }
        ಯದಿ (AstPropAlways* ಧ್ರುವ alwaysp = VN_CAST(nodep, PropAlways)) {
            ಪ್ರತ್ಯಾಗಮ buildPropAlways(alwaysp, entryVtxp, is_ತುದಿ_Level_Step);
        }
        ಯದಿ (AstSGotoRep* ಧ್ರುವ repp = VN_CAST(nodep, SGotoRep)) {
            ಪ್ರತ್ಯಾಗಮ buildGotoRep(repp, entryVtxp);
        }
        ಯದಿ (AstSThroughout* ಧ್ರುವ throughoutp = VN_CAST(nodep, SThroughout)) {
            ಪ್ರತ್ಯಾಗಮ buildThroughout(throughoutp, entryVtxp, is_ತುದಿ_Level_Step);
        }
        ಯದಿ (AstSOr* ಧ್ರುವ orp = VN_CAST(nodep, SOr)) {
            ಪ್ರತ್ಯಾಗಮ buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        ಯದಿ (AstLogOr* ಧ್ರುವ orp = VN_CAST(nodep, LogOr)) {
            ಪ್ರತ್ಯಾಗಮ buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        ಯದಿ (AstSAnd* ಧ್ರುವ andp = VN_CAST(nodep, SAnd)) {
            ಪ್ರತ್ಯಾಗಮ buildAndCombiner(andp->lhsp(), andp->rhsp(), entryVtxp, andp->fileline());
        }
        ಯದಿ (AstSIntersect* ಧ್ರುವ intp = VN_CAST(nodep, SIntersect)) {
            // IEEE 1800-2023 16.9.6: both operands match over one window with
            // equal start and end (equal length). Lower to a single sequence
            // that conjoins both operands' per-cycle checks -- correct under
            // concurrent attempts, where the done-latch combiner conflates the
            // two operands' start times and over-accepts. The combiner remains
            // only as a fallback for operands that do not flatten.
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lhsLen = fixedLength(intp->lhsp());
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ rhsLen = fixedLength(intp->rhsp());
            ಯದಿ (lhsLen >= 0 && rhsLen >= 0) {
                ಯದಿ (lhsLen != rhsLen) {
                    // Unequal fixed lengths share no common length -> never matches.
                    ಪ್ರತ್ಯಾಗಮ buildNeverMatchIntersect(intp, entryVtxp, is_ತುದಿ_Level_Step,
                                                    "intersect operands have no common length");
                }
                ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ conjp
                    = conjoinFixedSeqs(intp->lhsp(), intp->rhsp(), intp->fileline())) {
                    ಪ್ರತ್ಯಾಗಮ build_From_Lowering_ವೃಕ್ಷ(conjp, entryVtxp, is_ತುದಿ_Level_Step);
                }
                ಪ್ರತ್ಯಾಗಮ buildAndCombiner(intp->lhsp(), intp->rhsp(), entryVtxp, intp->fileline());
            }
            ಪ್ರತ್ಯಾಗಮ buildVarLenIntersect(intp, entryVtxp, is_ತುದಿ_Level_Step);
        }
        ಯದಿ (AstSWithin* ಧ್ರುವ withinp = VN_CAST(nodep, SWithin)) {
            ಪ್ರತ್ಯಾಗಮ buildSWithin(withinp, entryVtxp, is_ತುದಿ_Level_Step);
        }
        ಯದಿ (AstAbortOn* ಧ್ರುವ ap = VN_CAST(nodep, AbortOn)) {
            ಪ್ರತ್ಯಾಗಮ buildAbortOn(ap->condp(), ap->propp(), entryVtxp, ap->kind(), ap->fileline(),
                                is_ತುದಿ_Level_Step);
        }
        ಯದಿ (VN_IS(nodep, SNonConsRep)) ಪ್ರತ್ಯಾಗಮ BuildResult::fail();
        ಯದಿ (AstImplication* ಧ್ರುವ implp = VN_CAST(nodep, Implication)) {
            ಪ್ರತ್ಯಾಗಮ buildImplicationEdges(implp->lhsp(), implp->rhsp(), entryVtxp,
                                         implp->isOverlapped(), implp->isFollowedBy(),
                                         implp->lhsp(), implp->fileline());
        }
        ಯದಿ (AstUntil* ಧ್ರುವ untilp = VN_CAST(nodep, Until)) {
            ಪ್ರತ್ಯಾಗಮ buildUntil(untilp, entryVtxp, is_ತುದಿ_Level_Step);
        }
        // Boolean leaf (including LogAnd): return as finalCond
        ಪ್ರತ್ಯಾಗಮ {entryVtxp, nodep, {}};
    }

    // Wire an implication / followed-by from `entryVtxp`: builds the antecedent,
    // emits the match-link (and for followed-by the reject-sink edge), inserts a
    // delay vertex for non-overlapped forms, and builds the body. Used both for
    // nested AstImplication in pexpr position and for the top-level assertion
    // antecedent -- `errorNodep` anchors the "unsupported sequence antecedent"
    // error, which differs between the two call sites.
    BuildResult buildImplicationEdges(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* antExprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bodyExprp,
                                      SvaStateVertex* entryVtxp, ತಾರ್ಕಿಕ isOverlapped,
                                      ತಾರ್ಕಿಕ isFollowedBy, Ast_ಗಂಟು* errorNodep, File_ಸಾಲು* flp) {
        ಧ್ರುವ BuildResult antResult = build_ಅಭಿವ್ಯಕ್ತಿ(antExprp, entryVtxp);
        ಯದಿ (!antResult.valid()) ಪ್ರತ್ಯಾಗಮ antResult;

        // Followed-by requires pure-boolean antecedent for non-vacuous-fail at
        // the attempt-start cycle. IEEE 1800-2023 16.12.9 permits a multi-cycle
        // sequence LHS, so this is an implementation gap rather than illegal SV.
        ಯದಿ (isFollowedBy && antResult.termVertexp != entryVtxp) {
            errorNodep->v3warn(E_UNSUPPORTED,
                               "Unsupported: sequence expression as antecedent of followed-by"
                               " (#-# / #=#) (IEEE 1800-2023 16.12.9)");
            ಪ್ರತ್ಯಾಗಮ BuildResult::failWithError();
        }
        UASSERT_OBJ(!isFollowedBy || antResult.finalCondp, errorNodep,
                    "followed-by antecedent terminal at entry must carry finalCondp");

        // Use raw createStateVertex() so trigVtxp starts without liveness --
        // reaching the antecedent terminal is a definitive event.
        SvaStateVertex* ಧ್ರುವ trigVtxp = m_graph.createStateVertex();
        ಯದಿ (antResult.finalCondp) {
            m_graph.addLink(antResult.termVertexp, trigVtxp,
                            sampled(antResult.finalCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)));
            // Followed-by non-vacuous fail: rejectOnFail fires when the attempt
            // is live (termVtx reachable) and sampled(antecedent) is false.
            ಯದಿ (isFollowedBy) {
                SvaStateVertex* ಧ್ರುವ sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = ಸತ್ಯ;
                SvaTransEdge* ಧ್ರುವ ep
                    = m_graph.addLink(antResult.termVertexp, sinkVtxp,
                                      sampled(antResult.finalCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)));
                ep->m_rejectOnFail = ಸತ್ಯ;
            }
            // finalCondp is cloned into the Sampled nodes; if the original is
            // not parented anywhere in the AST anymore it must be freed here
            // or ASan flags it as a leak (e.g. t_sequence_bool_ops).
            ಯದಿ (!antResult.finalCondp->backp()) {
                VL_DO_DANGLING(antResult.finalCondp->delete_ವೃಕ್ಷ(), antResult.finalCondp);
            }
        } ಅನ್ಯಥಾ {
            m_graph.addLink(antResult.termVertexp, trigVtxp);
        }
        resetScope();

        SvaStateVertex* bodyEntryp = trigVtxp;
        ಯದಿ (!isOverlapped) {
            SvaStateVertex* ಧ್ರುವ delayVtxp = m_graph.createStateVertex();
            m_graph.addClockedEdge(trigVtxp, delayVtxp);
            bodyEntryp = delayVtxp;
        }
        // An empty antecedent match must not start the consequent, but a zero-minimum
        // repetition's empty match is not handled here; keep the merge vertex for those
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = !isFollowedBy && !hasZeroMinRep(antExprp);
        ಪ್ರತ್ಯಾಗಮ build_ಅಭಿವ್ಯಕ್ತಿ(bodyExprp, bodyEntryp, /*isTopLevelStep=*/ಸತ್ಯ);
    }

    BuildResult build(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        m_graph.m_startVertexp = scopedCreateVertex();
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = ಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ build_ಅಭಿವ್ಯಕ್ತಿ(exprp, m_graph.m_startVertexp, /*isTopLevelStep=*/ಸತ್ಯ);
    }
};

//######################################################################
// NFA Lowering (converts NFA graph to synthesizable AstAlways blocks)

ವರ್ಗ SvaNfaLowering final {
    Ast_ಗಂಟು_Module* ಧ್ರುವ m_modp;  // Module to add state vars and always blocks to
    Ast_ಗಂಟು_DType* ಧ್ರುವ m_u32DTypep;  // Shared unsigned counter dtype
    V3UniqueNames m_names{"__Vnfa"};
    size_t m_statDelayRingEdgeVisits = 0;  // Delay-ring incoming edges visited

    // Per-lowering shared context (passed to phase sub-functions)
    // Per-vertex lowering state is stored in SvaVertexData and accessed via
    // V3GraphVertex::userp() (see SvaStateVertex::datap()).
    ಸಂರಚನಾ LowerCtx final {
        File_ಸಾಲು* flp;  // Source location for generated AST
        Ast_Sen_ವೃಕ್ಷ* senTreep;  // Clock sensitivity tree
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* disableExprp;  // disable iff expression (may be nullptr)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* matchCondp;  // Final boolean match condition (may be nullptr)
        VAssertType assertType;  // Assertion type for control tasks
        VAssertDirectiveType directiveType;  // Directive type for control tasks
        AstVar* killVarp;  // Last observed kill generation
        SvaGraph& graph;  // NFA graph
    };

    // Build a match-now expression: stateSig[i] && $sampled(condp)
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* buildMatchNow(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* stateExprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ statep = stateExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        ಯದಿ (!condp) ಪ್ರತ್ಯಾಗಮ statep;
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstLogAnd{flp, statep, sampled(condp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ))};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* andCond(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        ಯದಿ (!condp) ಪ್ರತ್ಯಾಗಮ exprp;
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstLogAnd{flp, exprp, condp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
    }
    // bp is always non-null; only ap can be null (serving as accumulator).
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* orExprs(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ap, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bp) {
        ಯದಿ (!ap) ಪ್ರತ್ಯಾಗಮ bp;
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstLogOr{flp, ap, bp};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* addCountp(File_ಸಾಲು* ಧ್ರುವ flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ totalCountp,
                                  Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ contributionp,
                                  Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enablep = nullptr) {
        // contribution = enable ? contribution : 0;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enabledContributionp
            = enablep ? ನೂತನ AstCond{flp, enablep, contributionp,
                                    newTypedConstp(flp, contributionp->dtypep(), 0)}
                      : contributionp;
        ಯದಿ (!totalCountp) ಪ್ರತ್ಯಾಗಮ enabledContributionp;
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAdd{flp, totalCountp, enabledContributionp};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* gateCheckedCountp(File_ಸಾಲು* ಧ್ರುವ flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ countp,
                                          Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*& overflowp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enablep) {
        ಯದಿ (overflowp) {
            overflowp = ನೂತನ AstLogAnd{flp, enablep->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), overflowp};
        }
        ಪ್ರತ್ಯಾಗಮ addCountp(flp, nullptr, countp, enablep);
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* addCheckedCountp(File_ಸಾಲು* ಧ್ರುವ flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ totalCountp,
                                         Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ contributionp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*& overflowp,
                                         Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ contributionOverflowp = nullptr) {
        ಯದಿ (contributionOverflowp) { overflowp = orExprs(flp, overflowp, contributionOverflowp); }
        ಯದಿ (!totalCountp) ಪ್ರತ್ಯಾಗಮ contributionp;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ oldTotalp = totalCountp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ resultp = ನೂತನ AstAdd{flp, totalCountp, contributionp};
        overflowp
            = orExprs(flp, overflowp, ನೂತನ AstLt{flp, resultp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), oldTotalp});
        ಪ್ರತ್ಯಾಗಮ resultp;
    }
    AstIf* new_ಎಣಿಕೆ_Overflow_Warning(File_ಸಾಲು* ಧ್ರುವ flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp) ಧ್ರುವ {
        AstDisplay* ಧ್ರುವ dispp = ನೂತನ AstDisplay{
            flp, VDisplayType::DT_WARNING,
            "Cover sequence match count overflowed 32-bit storage; results may be incorrect.",
            nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{flp, condp, dispp};
        ifp->branchPred(VBranchPred::BP_UNLIKELY);
        ಪ್ರತ್ಯಾಗಮ ifp;
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* killActive(LowerCtx& c) {
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstNeq{c.flp, ನೂತನ AstVarRef{c.flp, c.killVarp, VAccess::READ},
                          assertKillGet(c.flp, c.assertType, c.directiveType)};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* notKillActive(LowerCtx& c) { ಪ್ರತ್ಯಾಗಮ ನೂತನ AstLogNot{c.flp, killActive(c)}; }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* gateNotKill(LowerCtx& c, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಯದಿ (!exprp) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstLogAnd{c.flp, exprp, notKillActive(c)};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nextRingIndex(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* idxExprp, uint32_t size) {
        ಧ್ರುವ auto u32Const = [flp](uint32_t ಮೌಲ್ಯ) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, ಮೌಲ್ಯ};
        };
        UASSERT_OBJ(size > 0, idxExprp, "Ring size must be positive");
        ಯದಿ (size == 1) {
            VL_DO_DANGLING(idxExprp->delete_ವೃಕ್ಷ(), idxExprp);
            ಪ್ರತ್ಯಾಗಮ u32Const(0);
        }
        // idx == size - 1 ? 0 : idx + 1
        AstAdd* ಧ್ರುವ addp = ನೂತನ AstAdd{flp, idxExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), u32Const(1)};
        addp->dtypeFrom(idxExprp);
        AstCond* ಧ್ರುವ condp
            = ನೂತನ AstCond{flp, ನೂತನ AstEq{flp, idxExprp, u32Const(size - 1)}, u32Const(0), addp};
        condp->dtypeFrom(idxExprp);
        ಪ್ರತ್ಯಾಗಮ condp;
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* delayRingSlot(File_ಸಾಲು* flp, SvaStateVertex* vtxp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* idxExprp,
                                      VAccess access = VAccess::READ) {
        // ring[idx]
        AstVar* ಧ್ರುವ ringp = vtxp->datap()->delayRingVarp;
        AstVarRef* ಧ್ರುವ refp = ನೂತನ AstVarRef{flp, ringp, access};
        ಯದಿ (vtxp->datap()->delayRingCounts) ಪ್ರತ್ಯಾಗಮ ನೂತನ AstArraySel{flp, refp, idxExprp};
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstSel{flp, refp, idxExprp, 1};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* delay_Ring_Head_ಎಣಿಕೆ(File_ಸಾಲು* flp, SvaStateVertex* vtxp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ headp
            = ನೂತನ AstVarRef{flp, vtxp->datap()->delayRingHeadVarp, VAccess::READ};
        ಯದಿ (vtxp->datap()->delayRingCounts) ಪ್ರತ್ಯಾಗಮ headp;
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstExtend{flp, headp, vtxp->datap()->delay_Ring_Live_ಎಣಿಕೆ_Varp->dtypep()->width()};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* delayRingAtLastIndex(File_ಸಾಲು* ಧ್ರುವ flp, AstVar* ಧ್ರುವ idxp,
                                             ಧ್ರುವ uint32_t size) {
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstEq{flp, ನೂತನ AstVarRef{flp, idxp, VAccess::READ},
                         ನೂತನ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, size - 1}};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* delay_Ring_Output_ಎಣಿಕೆ(File_ಸಾಲು* ಧ್ರುವ flp, SvaStateVertex* ಧ್ರುವ vtxp) {
        AstVar* ಧ್ರುವ idxp = vtxp->datap()->delayRingIdxVarp;
        ಧ್ರುವ uint32_t size = vtxp->m_delayRingSize;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* outgoingValidp
            = ನೂತನ AstVarRef{flp, vtxp->datap()->delayRingWrappedVarp, VAccess::READ};
        ಯದಿ (!vtxp->m_isFixedDelayRing) {
            outgoingValidp
                = ನೂತನ AstLogOr{flp, outgoingValidp, delayRingAtLastIndex(flp, idxp, size)};
        }
        AstVar* ಧ್ರುವ live_ಎಣಿಕೆ_Varp = vtxp->datap()->delay_Ring_Live_ಎಣಿಕೆ_Varp;
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCond{flp, outgoingValidp, delay_Ring_Head_ಎಣಿಕೆ(flp, vtxp),
                           newTypedConstp(flp, live_ಎಣಿಕೆ_Varp->dtypep(), 0)};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* delayRingHasLiveThreadsp(File_ಸಾಲು* ಧ್ರುವ flp,
                                                 AstVar* ಧ್ರುವ live_ಎಣಿಕೆ_Varp) {
        // active = live_count != 0;
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstNeq{flp, ನೂತನ AstVarRef{flp, live_ಎಣಿಕೆ_Varp, VAccess::READ},
                          newTypedConstp(flp, live_ಎಣಿಕೆ_Varp->dtypep(), 0)};
    }

    // Phase 3 output signals
    ಸಂರಚನಾ SignalSet final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* terminalActivep = nullptr;  // OR of all successful terminal matches
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* matchCountp = nullptr;  // NFA paths completing the sequence this tick
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* match_ಎಣಿಕೆ_Overflowp = nullptr;  // Match count overflow this tick
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rejectBasep = nullptr;  // Reject when a terminal match fails
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* requiredStepRejectp = nullptr;  // Per-source reject from rejectOnFail Links
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* throughoutRejectp = nullptr;  // Reject when a throughout guard drops
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* threadFailCountp = nullptr;  // Number of threads rejected on this tick
    };

    // Disable condition also aborts attempts between clock events. A constant disable
    // never rises, and $sampled is unsupported in a sensitivity list.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isAsyncDisable(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ disableExprp) {
        ಪ್ರತ್ಯಾಗಮ disableExprp && !VN_IS(disableExprp, Const)
               && !disableExprp->exists([](ಧ್ರುವ AstSampled*) { ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ; });
    }

    // Emit the clocked update block of registers that hold pending attempts. The clearp
    // statements reset the same registers: a disable rising between clock events also
    // aborts pending attempts (IEEE 1800-2023 16.12), so clearp runs on that rising edge
    // in a separate block. It must clear unconditionally, as the disable may already be
    // false again when it runs, and the update block must not run on that edge, as it
    // would advance the attempts as if the clock had ticked. Blocks that hold no attempt
    // state, such as the $assertkill acknowledge in emitKillAckNba, do not go through here.
    // Callers pass a null clearp when the disable cannot rise between clock events.
    ಶೂನ್ಯ addStateAlways(LowerCtx& c, Ast_ಗಂಟು* bodyp, Ast_ಗಂಟು* clearp) {
        m_modp->addStmtsp(
            ನೂತನ AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), bodyp});
        ಯದಿ (!clearp) ಪ್ರತ್ಯಾಗಮ;
        m_modp->addStmtsp(ನೂತನ AstAlways{
            c.flp, VAlwaysKwd::ALWAYS,
            ನೂತನ Ast_Sen_ವೃಕ್ಷ{c.flp, ನೂತನ AstSenItem{c.flp, VEdgeType::ET_POSEDGE,
                                                 c.disableExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)}},
            clearp});
    }

    // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
    // Phase 2: State register NBA always block. Each clocked-edge target
    // latches the OR of its incoming contributions.
    ಶೂನ್ಯ emitStateRegisterNba(LowerCtx& c) {
        ಧ್ರುವ ತಾರ್ಕಿಕ asyncDisable = isAsyncDisable(c.disableExprp);
        Ast_ಗಂಟು* bodyp = nullptr;
        Ast_ಗಂಟು* clearp = nullptr;
        ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ಧ್ರುವ SvaStateVertex* ಧ್ರುವ vtxp = static_cast<ಧ್ರುವ SvaStateVertex*>(&vtxr);
            ಯದಿ (!vtxp->datap()->stateVarp) ಅನುವರ್ತ;

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nextStatep = nullptr;
            ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edger : vtxp->inEdges()) {
                ಧ್ರುವ SvaTransEdge& tedger = static_cast<ಧ್ರುವ SvaTransEdge&>(edger);
                ಯದಿ (!tedger.m_consumesCycle) ಅನುವರ್ತ;
                ಧ್ರುವ SvaStateVertex* ಧ್ರುವ fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Clocked-edge source missing stateSig");

                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcSigp = fromVtxp->datap()->stateSigp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                srcSigp = andCond(c.flp, srcSigp, tedger.m_condp);

                ಯದಿ (c.disableExprp) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ notDisp
                        = ನೂತನ AstLogNot{c.flp, c.disableExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                    srcSigp = ನೂತನ AstLogAnd{c.flp, srcSigp, notDisp};
                }
                nextStatep = orExprs(c.flp, nextStatep, srcSigp);
            }

            UASSERT_OBJ(nextStatep, vtxp,
                        "Registered vertex has no clocked incoming contribution");
            nextStatep = gateNotKill(c, nextStatep);

            AstAssignDly* ಧ್ರುವ assignp = ನೂತನ AstAssignDly{
                c.flp, ನೂತನ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE}, nextStatep};
            bodyp = Ast_ಗಂಟು::addNextNull(bodyp, assignp);
            ಯದಿ (asyncDisable) {
                clearp = Ast_ಗಂಟು::addNextNull(
                    clearp,
                    ನೂತನ AstAssignDly{
                        c.flp, ನೂತನ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE},
                        ನೂತನ AstConst{c.flp, AstConst::BitFalse{}}});
            }
        }
        ಯದಿ (bodyp) addStateAlways(c, bodyp, clearp);
    }

    // Phase 2b: In-place ring storage and registered ring metadata.
    ಶೂನ್ಯ emitDelayRingUpdates(LowerCtx& c, ಧ್ರುವ ತಾರ್ಕಿಕ ಎಣಿಕೆ_Matches) {
        ಧ್ರುವ ತಾರ್ಕಿಕ asyncDisable = isAsyncDisable(c.disableExprp);
        ಪ್ರತಿ (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ಧ್ರುವ vtxp = static_cast<SvaStateVertex*>(&vtxr);
            ಯದಿ (!vtxp->datap()->delayRingVarp) ಅನುವರ್ತ;
            AstVar* ಧ್ರುವ idxp = vtxp->datap()->delayRingIdxVarp;
            AstVar* ಧ್ರುವ live_ಎಣಿಕೆ_Varp = vtxp->datap()->delay_Ring_Live_ಎಣಿಕೆ_Varp;
            AstVar* ಧ್ರುವ wrappedp = vtxp->datap()->delayRingWrappedVarp;
            ಧ್ರುವ uint32_t size = static_cast<uint32_t>(vtxp->m_delayRingSize);

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* incomingp = nullptr;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* incomingOverflowp = nullptr;
            ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edger : vtxp->inEdges()) {
                ++m_statDelayRingEdgeVisits;
                ಧ್ರುವ SvaTransEdge& tedger = static_cast<ಧ್ರುವ SvaTransEdge&>(edger);
                UASSERT_OBJ(tedger.m_consumesCycle == vtxp->m_isFixedDelayRing, vtxp,
                            "Delay-ring incoming edge kind mismatch");
                ಧ್ರುವ SvaStateVertex* ಧ್ರುವ fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Delay-ring incoming source missing stateSig");
                UASSERT_OBJ(!ಎಣಿಕೆ_Matches || fromVtxp->datap()->state_ಎಣಿಕೆ_Sigp, fromVtxp,
                            "Delay-ring incoming source missing stateCountSig");
                // IEEE 1800-2023 16.14.1/16.14.3: assertion alternatives belong to one
                // evaluation; only cover sequence counts each end-of-match.
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* contribp
                    = ಎಣಿಕೆ_Matches
                          ? fromVtxp->datap()->state_ಎಣಿಕೆ_Sigp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)
                          : ನೂತನ AstExtend{c.flp,
                                          fromVtxp->datap()->stateSigp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                                          m_u32DTypep->width()};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* contribOverflowp
                    = ಎಣಿಕೆ_Matches && fromVtxp->datap()->state_ಎಣಿಕೆ_Overflow_Sigp
                          ? fromVtxp->datap()->state_ಎಣಿಕೆ_Overflow_Sigp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)
                          : nullptr;
                ಯದಿ (tedger.m_condp) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enablep = tedger.m_condp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                    contribp = ಎಣಿಕೆ_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, enablep)
                                   : addCountp(c.flp, nullptr, contribp, enablep);
                }
                ಯದಿ (c.disableExprp) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ notDisp
                        = ನೂತನ AstLogNot{c.flp, c.disableExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                    contribp = ಎಣಿಕೆ_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, notDisp)
                                   : addCountp(c.flp, nullptr, contribp, notDisp);
                }
                incomingp = ಎಣಿಕೆ_Matches ? addCheckedCountp(c.flp, incomingp, contribp,
                                                            incomingOverflowp, contribOverflowp)
                                         : addCountp(c.flp, incomingp, contribp);
            }
            UASSERT_OBJ(incomingp, vtxp, "Delay ring has no incoming edge");
            ಧ್ರುವ ತಾರ್ಕಿಕ storesCounts = vtxp->datap()->delayRingCounts;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ incomingCountp
                = storesCounts
                      ? incomingp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)
                      : ನೂತನ AstExtend{c.flp,
                                      ನೂತನ AstNeq{c.flp, incomingp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                                                 newTypedConstp(c.flp, incomingp->dtypep(), 0)},
                                      m_u32DTypep->width()};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ storedIncomingp
                = storesCounts ? incomingp
                               : ನೂತನ AstNeq{c.flp, incomingp,
                                            newTypedConstp(c.flp, incomingp->dtypep(), 0)};
            // Only this block accesses the storage. Other processes read the cached head, so
            // the ring can be updated in place without an NBA shadow copy.
            AstAssign* ಧ್ರುವ writeIncomingp = ನೂತನ AstAssign{
                c.flp,
                delayRingSlot(c.flp, vtxp, ನೂತನ AstVarRef{c.flp, idxp, VAccess::READ},
                              VAccess::WRITE),
                storedIncomingp};
            Ast_ಗಂಟು* updateBodyp = writeIncomingp;
            // live_count <= live_count - outgoing_count + incoming_count;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ outgoingp = delay_Ring_Output_ಎಣಿಕೆ(c.flp, vtxp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ afterOutgoingp
                = ನೂತನ AstSub{c.flp, ನೂತನ AstVarRef{c.flp, live_ಎಣಿಕೆ_Varp, VAccess::READ}, outgoingp};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ oldAfterOutgoingp = afterOutgoingp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            AstAdd* ಧ್ರುವ nextLiveCountp = ನೂತನ AstAdd{c.flp, afterOutgoingp, incomingCountp};
            ಯದಿ (storesCounts) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ liveOverflowp
                    = ನೂತನ AstLt{c.flp, nextLiveCountp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), oldAfterOutgoingp};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ overflowp = orExprs(c.flp, incomingOverflowp, liveOverflowp);
                updateBodyp = new_ಎಣಿಕೆ_Overflow_Warning(c.flp, overflowp);
                updateBodyp->addNext(writeIncomingp);
            } ಅನ್ಯಥಾ {
                VL_DO_DANGLING(oldAfterOutgoingp->delete_ವೃಕ್ಷ(), oldAfterOutgoingp);
            }
            // Cache the outgoing slot for the next index after the write. Reading after
            // writing also handles one-slot rings and two-slot range rings without alias cases.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nextHeadIdxp
                = nextRingIndex(c.flp, ನೂತನ AstVarRef{c.flp, idxp, VAccess::READ}, size);
            ಯದಿ (!vtxp->m_isFixedDelayRing) {
                nextHeadIdxp = nextRingIndex(c.flp, nextHeadIdxp, size);
            }
            updateBodyp->addNext(ನೂತನ AstAssignDly{
                c.flp, ನೂತನ AstVarRef{c.flp, vtxp->datap()->delayRingHeadVarp, VAccess::WRITE},
                delayRingSlot(c.flp, vtxp, nextHeadIdxp)});
            updateBodyp->addNext(ನೂತನ AstAssignDly{
                c.flp, ನೂತನ AstVarRef{c.flp, live_ಎಣಿಕೆ_Varp, VAccess::WRITE}, nextLiveCountp});
            // wrapped <= wrapped || idx == size - 1;
            updateBodyp->addNext(
                ನೂತನ AstAssignDly{c.flp, ನೂತನ AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                 ನೂತನ AstLogOr{c.flp, ನೂತನ AstVarRef{c.flp, wrappedp, VAccess::READ},
                                              delayRingAtLastIndex(c.flp, idxp, size)}});
            // idx <= next_idx;
            updateBodyp->addNext(ನೂತನ AstAssignDly{
                c.flp, ನೂತನ AstVarRef{c.flp, idxp, VAccess::WRITE},
                nextRingIndex(c.flp, ನೂತನ AstVarRef{c.flp, idxp, VAccess::READ}, size)});
            ಯದಿ (vtxp->m_delayRingAdvanceCondp) {
                updateBodyp = ನೂತನ AstIf{
                    c.flp, sampled(vtxp->m_delayRingAdvanceCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)),
                    updateBodyp};
            }

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* clearCondp = killActive(c);
            ಯದಿ (vtxp->m_delayRingClearCondp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_delayRingClearCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)));
            }
            ಯದಿ (vtxp->m_abortClearp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_abortClearp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)));
            }
            ಯದಿ (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* guardp = nullptr;
            ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ cp : vtxp->m_throughoutConds) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ sampledp = sampled(cp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
                guardp = guardp ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstLogAnd{c.flp, guardp, sampledp})
                                : sampledp;
            }
            ಯದಿ (guardp) clearCondp = orExprs(c.flp, clearCondp, ನೂತನ AstLogNot{c.flp, guardp});
            // Logically clear the ring without touching its wide storage.
            AstAssignDly* ಧ್ರುವ clearCountp
                = ನೂತನ AstAssignDly{c.flp, ನೂತನ AstVarRef{c.flp, live_ಎಣಿಕೆ_Varp, VAccess::WRITE},
                                   newTypedConstp(c.flp, live_ಎಣಿಕೆ_Varp->dtypep(), 0)};
            clearCountp->addNext(ನೂತನ AstAssignDly{c.flp,
                                                  ನೂತನ AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                                  ನೂತನ AstConst{c.flp, AstConst::BitFalse{}}});
            clearCountp->addNext(ನೂತನ AstAssignDly{c.flp,
                                                  ನೂತನ AstVarRef{c.flp, idxp, VAccess::WRITE},
                                                  newTypedConstp(c.flp, idxp->dtypep(), 0)});
            updateBodyp = ನೂತನ AstIf{c.flp, clearCondp, clearCountp, updateBodyp};

            addStateAlways(c, updateBodyp, asyncDisable ? clearCountp->clone_ವೃಕ್ಷ(ಸತ್ಯ) : nullptr);
        }
    }

    // Phase 2c: SAnd combiner done-latch always block.
    // NBA semantics ensure doneL/doneR read pre-update values (IEEE 16.9.5).
    ಶೂನ್ಯ emitAndCombinerDoneLatchNba(LowerCtx& c) {
        ಧ್ರುವ ತಾರ್ಕಿಕ asyncDisable = isAsyncDisable(c.disableExprp);
        ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ಧ್ರುವ SvaStateVertex* ಧ್ರುವ avp = static_cast<ಧ್ರುವ SvaStateVertex*>(&vtxr);
            ಯದಿ (!avp->datap()->doneLVarp) ಅನುವರ್ತ;
            // doneLVars is non-null only for AndCombiner vertices, which always
            // have both m_andLhsTermp and m_andRhsTermp set at build time.
            UASSERT_OBJ(avp->m_andLhsTermp && avp->m_andRhsTermp, avp,
                        "AndCombiner vertex missing LHS/RHS terminal");
            ಧ್ರುವ SvaStateVertex* ಧ್ರುವ lhsTermp = avp->m_andLhsTermp;
            ಧ್ರುವ SvaStateVertex* ಧ್ರುವ rhsTermp = avp->m_andRhsTermp;
            // resolveLinks' 2*N+2 fixed-point pass is guaranteed to populate
            // stateSigp on every AndCombiner and its LHS/RHS terminals.
            UASSERT_OBJ(lhsTermp->datap()->stateSigp && rhsTermp->datap()->stateSigp
                            && avp->datap()->stateSigp,
                        avp, "AndCombiner stateSigp chain unresolved after resolveLinks");

            AstAssignDly* ಧ್ರುವ clearLp = ನೂತನ AstAssignDly{
                c.flp, ನೂತನ AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                ನೂತನ AstConst{c.flp, AstConst::BitFalse{}}};
            AstAssignDly* ಧ್ರುವ clearRp = ನೂತನ AstAssignDly{
                c.flp, ನೂತನ AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                ನೂತನ AstConst{c.flp, AstConst::BitFalse{}}};
            clearLp->addNext(clearRp);

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ matchLNowp
                = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, avp->m_andLhsCondp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ matchRNowp
                = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, avp->m_andRhsCondp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* gateLp = matchLNowp;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* gateRp = matchRNowp;
            ಯದಿ (c.disableExprp) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ notDisLp
                    = ನೂತನ AstLogNot{c.flp, c.disableExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                gateLp = ನೂತನ AstLogAnd{c.flp, gateLp, notDisLp};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ notDisRp
                    = ನೂತನ AstLogNot{c.flp, c.disableExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                gateRp = ನೂತನ AstLogAnd{c.flp, gateRp, notDisRp};
            }
            AstAssignDly* ಧ್ರುವ setLp = ನೂತನ AstAssignDly{
                c.flp, ನೂತನ AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                ನೂತನ AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* ಧ್ರುವ setLIfp = ನೂತನ AstIf{c.flp, gateLp, setLp, nullptr};
            AstAssignDly* ಧ್ರುವ setRp = ನೂತನ AstAssignDly{
                c.flp, ನೂತನ AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                ನೂತನ AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* ಧ್ರುವ setRIfp = ನೂತನ AstIf{c.flp, gateRp, setRp, nullptr};
            setLIfp->addNext(setRIfp);

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* clearCondp = ನೂತನ AstLogOr{c.flp, killActive(c),
                                                   avp->datap()->stateSigp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
            ಯದಿ (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
            }
            AstIf* ಧ್ರುವ topp = ನೂತನ AstIf{c.flp, clearCondp, clearLp, setLIfp};
            addStateAlways(c, topp, asyncDisable ? clearLp->clone_ವೃಕ್ಷ(ಸತ್ಯ) : nullptr);
        }
    }

    ಶೂನ್ಯ emitKillAckNba(LowerCtx& c) {
        AstAssignDly* ಧ್ರುವ ackp
            = ನೂತನ AstAssignDly{c.flp, ನೂತನ AstVarRef{c.flp, c.killVarp, VAccess::WRITE},
                               assertKillGet(c.flp, c.assertType, c.directiveType)};
        m_modp->addStmtsp(
            ನೂತನ AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), ackp});
    }

    // Phase 3/3a/3b: Compute terminal match/reject signals, required-step reject,
    // throughout-drop reject; clean up intermediate state signals.
    // Phase 3: terminalActive and rejectBase from Links to matchVertex.
    // Builder only adds Links (non-clocked) to matchVertex via addLink in
    // wireMatchAndMidSources. For cover sequence, also count each end-of-match
    // so the action can be replayed without unrolling endpoints.
    ಶೂನ್ಯ computeTerminalMatchAndReject(LowerCtx& c, SignalSet& sigs, ಧ್ರುವ ತಾರ್ಕಿಕ need_Match_ಎಣಿಕೆ) {
        ಪ್ರತಿ (ಧ್ರುವ SvaTransEdge* ಧ್ರುವ tedgep : c.graph.allEdges()) {
            ಯದಿ (tedgep->toVtxp() != c.graph.m_matchVertexp) ಅನುವರ್ತ;
            SvaStateVertex* ಧ್ರುವ fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                        "Terminal-link source missing stateSig");

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcSigp = fromVtxp->datap()->stateSigp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            srcSigp = andCond(c.flp, srcSigp, tedgep->m_condp);
            ಯದಿ (need_Match_ಎಣಿಕೆ) {
                UASSERT_OBJ(fromVtxp->datap()->state_ಎಣಿಕೆ_Sigp, fromVtxp,
                            "Terminal-link source missing stateCountSig");
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* contributionp
                    = fromVtxp->datap()->state_ಎಣಿಕೆ_Sigp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* contributionOverflowp
                    = fromVtxp->datap()->state_ಎಣಿಕೆ_Overflow_Sigp
                          ? fromVtxp->datap()->state_ಎಣಿಕೆ_Overflow_Sigp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)
                          : nullptr;
                ಯದಿ (tedgep->m_condp) {
                    contributionp = gateCheckedCountp(c.flp, contributionp, contributionOverflowp,
                                                      tedgep->m_condp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
                }
                sigs.matchCountp
                    = addCheckedCountp(c.flp, sigs.matchCountp, contributionp,
                                       sigs.match_ಎಣಿಕೆ_Overflowp, contributionOverflowp);
            }

            ಯದಿ (fromVtxp->m_delayRingSize && !fromVtxp->m_isFixedDelayRing) {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
                // reject |= (wrapped || idx == size - 1) && head && final_condition;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* expireContribp = delay_Ring_Output_ಎಣಿಕೆ(c.flp, fromVtxp);
                expireContribp = andCond(c.flp, expireContribp, tedgep->m_condp);
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, expireContribp);
                VL_DO_DANGLING(srcSigp->delete_ವೃಕ್ಷ(), srcSigp);
            } ಅನ್ಯಥಾ ಯದಿ (fromVtxp->m_isUnbounded || fromVtxp->m_isAndCombiner) {
                sigs.terminalActivep = orExprs(c.flp, sigs.terminalActivep, srcSigp);
            } ಅನ್ಯಥಾ {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, srcSigp);
            }
        }
        // wireMatchAndMidSources always adds a Link from result.termVertexp
        // to m_matchVertexp, so the loop above always sets terminalActivep.
        UASSERT_OBJ(sigs.terminalActivep, c.graph.m_matchVertexp,
                    "No terminal edge to match vertex");
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newThroughoutThreadFailCountp(LowerCtx& c, AstVar* ಧ್ರುವ delay_Ring_Live_ಎಣಿಕೆ_Varp,
                                               Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ stateExprp,
                                               Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enablep) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ activeThreadCountp
            = delay_Ring_Live_ಎಣಿಕೆ_Varp
                  ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                        ನೂತನ AstVarRef{c.flp, delay_Ring_Live_ಎಣಿಕೆ_Varp, VAccess::READ})
                  : ನೂತನ AstExtend{c.flp, stateExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), m_u32DTypep->width()};
        ಪ್ರತ್ಯಾಗಮ addCountp(c.flp, nullptr, activeThreadCountp, enablep);
    }

    // Phase 3b: Throughout-drop and ring-wide abort rejection.
    ಶೂನ್ಯ computeThroughoutReject(LowerCtx& c, SignalSet& sigs, ಧ್ರುವ ತಾರ್ಕಿಕ need_Thread_Fail_ಎಣಿಕೆ) {
        ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ಧ್ರುವ SvaStateVertex* ಧ್ರುವ vtxp = static_cast<ಧ್ರುವ SvaStateVertex*>(&vtxr);
            ಧ್ರುವ auto& conds = vtxp->m_throughoutConds;
            ಯದಿ (conds.empty() && !vtxp->m_abortRejectp) ಅನುವರ್ತ;
            ಯದಿ (vtxp->m_isAndCombiner) ಅನುವರ್ತ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* stateExprp = nullptr;
            ಯದಿ (vtxp->datap()->stateVarp) {
                stateExprp = ನೂತನ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
            } ಅನ್ಯಥಾ ಯದಿ (vtxp->datap()->delayRingVarp && vtxp->m_isFixedDelayRing) {
                stateExprp
                    = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_ಎಣಿಕೆ_Varp);
            } ಅನ್ಯಥಾ {
                UASSERT_OBJ(vtxp->datap()->stateSigp, vtxp,
                            "Throughout-conds vertex missing state representation");
                stateExprp = vtxp->datap()->stateSigp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* guardp = nullptr;
            ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ cp : conds) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ sp = sampled(cp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
                guardp = guardp ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstLogAnd{c.flp, guardp, sp}) : sp;
            }
            ಯದಿ (vtxp->m_abortRejectp) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ notAbortp
                    = ನೂತನ AstLogNot{c.flp, sampled(vtxp->m_abortRejectp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ))};
                guardp = guardp
                             ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstLogAnd{c.flp, guardp, notAbortp})
                             : notAbortp;
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rejectCondp = ನೂತನ AstLogNot{c.flp, guardp};
            ಯದಿ (vtxp->m_abortClearp) {
                // Any abort clears the ring. Accept aborts suppress a simultaneous guard failure;
                // reject aborts are restored below so they still force rejection.
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ notAbortClearp
                    = ನೂತನ AstLogNot{c.flp, sampled(vtxp->m_abortClearp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ))};
                rejectCondp = ನೂತನ AstLogAnd{c.flp, rejectCondp, notAbortClearp};
                ಯದಿ (vtxp->m_abortRejectp) {
                    rejectCondp = ನೂತನ AstLogOr{
                        c.flp, rejectCondp, sampled(vtxp->m_abortRejectp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ))};
                }
            }
            ಯದಿ (need_Thread_Fail_ಎಣಿಕೆ) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ contributionp
                    = newThroughoutThreadFailCountp(c, vtxp->datap()->delay_Ring_Live_ಎಣಿಕೆ_Varp,
                                                    stateExprp, rejectCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
                sigs.threadFailCountp = addCountp(c.flp, sigs.threadFailCountp, contributionp);
                ಯದಿ (vtxp->m_abortRejectp && vtxp->m_delayRingAdvanceCondp) {
                    // An advancing ring thread also occupies its same-tick match vertex in the
                    // unrolled NFA, so abort rejection must replay both fail actions.
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ abortAndAdvancep = ನೂತನ AstLogAnd{
                        c.flp, sampled(vtxp->m_abortRejectp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)),
                        sampled(vtxp->m_delayRingAdvanceCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ))};
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ matchContributionp = newThroughoutThreadFailCountp(
                        c, vtxp->datap()->delay_Ring_Live_ಎಣಿಕೆ_Varp, stateExprp, abortAndAdvancep);
                    sigs.threadFailCountp
                        = addCountp(c.flp, sigs.threadFailCountp, matchContributionp);
                }
            }
            sigs.throughoutRejectp = orExprs(c.flp, sigs.throughoutRejectp,
                                             ನೂತನ AstLogAnd{c.flp, stateExprp, rejectCondp});
        }
    }

    SignalSet computeSignals(LowerCtx& c, ಧ್ರುವ ತಾರ್ಕಿಕ need_Thread_Fail_ಎಣಿಕೆ,
                             ಧ್ರುವ ತಾರ್ಕಿಕ need_Throughout_Thread_Fail_ಎಣಿಕೆ, ಧ್ರುವ ತಾರ್ಕಿಕ need_Match_ಎಣಿಕೆ) {
        SignalSet sigs;

        computeTerminalMatchAndReject(c, sigs, need_Match_ಎಣಿಕೆ);

        // Phase 3a: required-step rejection.
        // Builder only sets m_rejectOnFail on non-clocked Links with m_condp
        // or m_condVtxp, and the source always has a resolved stateSig.
        ಪ್ರತಿ (ಧ್ರುವ SvaTransEdge* ಧ್ರುವ tedgep : c.graph.allEdges()) {
            ಯದಿ (!tedgep->m_rejectOnFail) ಅನುವರ್ತ;
            ಧ್ರುವ SvaStateVertex* ಧ್ರುವ fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp && (tedgep->m_condp || tedgep->m_condVtxp),
                        fromVtxp,
                        "rejectOnFail Link must have condp/condVtxp and source stateSig");
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ srcSigp = fromVtxp->datap()->stateSigp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = nullptr;
            ಯದಿ (tedgep->m_condVtxp) {
                UASSERT_OBJ(tedgep->m_condVtxp->datap()->stateSigp, tedgep->m_condVtxp,
                            "rejectOnFail condVtxp missing stateSig");
                condp = tedgep->m_condVtxp->datap()->stateSigp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                ಯದಿ (tedgep->m_condp) {
                    condp = ನೂತನ AstLogOr{c.flp, condp, tedgep->m_condp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                }
            } ಅನ್ಯಥಾ {
                condp = tedgep->m_condp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ notCondp = ನೂತನ AstLogNot{c.flp, condp};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rawFailp = ನೂತನ AstLogAnd{c.flp, srcSigp, notCondp};
            ಯದಿ (need_Thread_Fail_ಎಣಿಕೆ) {
                // thread_fail_count += fail;
                sigs.threadFailCountp = addCountp(
                    c.flp, sigs.threadFailCountp,
                    ನೂತನ AstExtend{c.flp, rawFailp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), m_u32DTypep->width()});
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ failp = gateNotKill(c, rawFailp);
            sigs.requiredStepRejectp = orExprs(c.flp, sigs.requiredStepRejectp, failp);
        }

        computeThroughoutReject(c, sigs, need_Throughout_Thread_Fail_ಎಣಿಕೆ);
        ಯದಿ (sigs.threadFailCountp) {
            sigs.threadFailCountp
                = addCountp(c.flp, nullptr, sigs.threadFailCountp, notKillActive(c));
        }
        ಯದಿ (sigs.matchCountp) {
            ಯದಿ (c.matchCondp) {
                sigs.matchCountp
                    = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_ಎಣಿಕೆ_Overflowp,
                                        sampled(c.matchCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)));
            }
            sigs.matchCountp = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_ಎಣಿಕೆ_Overflowp,
                                                 notKillActive(c));
        }
        sigs.terminalActivep = gateNotKill(c, sigs.terminalActivep);
        sigs.rejectBasep = gateNotKill(c, sigs.rejectBasep);
        sigs.throughoutRejectp = gateNotKill(c, sigs.throughoutRejectp);

        // Clean up intermediate state signals. These are orphan subtrees
        // (never linked into the enclosing AST); deleteTree() is immediate
        // which is what we want since stateSigp lifetime ends with this scope.
        ಪ್ರತಿ (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ಧ್ರುವ vtxp = static_cast<SvaStateVertex*>(&vtxr);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*& sigp = vtxp->datap()->stateSigp;
            ಯದಿ (sigp) VL_DO_DANGLING(sigp->delete_ವೃಕ್ಷ(), sigp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*& countp = vtxp->datap()->state_ಎಣಿಕೆ_Sigp;
            ಯದಿ (countp) VL_DO_DANGLING(countp->delete_ವೃಕ್ಷ(), countp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*& overflowp = vtxp->datap()->state_ಎಣಿಕೆ_Overflow_Sigp;
            ಯದಿ (overflowp) VL_DO_DANGLING(overflowp->delete_ವೃಕ್ಷ(), overflowp);
        }
        // Disable iff gating (IEEE 1800-2023 16.12) on the current level value.
        ಯದಿ (c.disableExprp) {
            // terminalActivep is always set, so gate it unconditionally.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ notTermp
                = ನೂತನ AstLogNot{c.flp, c.disableExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
            sigs.terminalActivep = ನೂತನ AstLogAnd{c.flp, sigs.terminalActivep, notTermp};
            ಯದಿ (sigs.rejectBasep) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ notDisp
                    = ನೂತನ AstLogNot{c.flp, c.disableExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                sigs.rejectBasep = ನೂತನ AstLogAnd{c.flp, sigs.rejectBasep, notDisp};
            }
            ಯದಿ (sigs.throughoutRejectp) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ notDisp
                    = ನೂತನ AstLogNot{c.flp, c.disableExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                sigs.throughoutRejectp = ನೂತನ AstLogAnd{c.flp, sigs.throughoutRejectp, notDisp};
            }
            ಯದಿ (sigs.requiredStepRejectp) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ notDisp
                    = ನೂತನ AstLogNot{c.flp, c.disableExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                sigs.requiredStepRejectp = ನೂತನ AstLogAnd{c.flp, sigs.requiredStepRejectp, notDisp};
            }
            ಯದಿ (sigs.matchCountp) {
                sigs.matchCountp = gateCheckedCountp(
                    c.flp, sigs.matchCountp, sigs.match_ಎಣಿಕೆ_Overflowp,
                    ನೂತನ AstLogNot{c.flp, c.disableExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)});
            }
        }

        ಯದಿ (sigs.match_ಎಣಿಕೆ_Overflowp) {
            m_modp->addStmtsp(
                ನೂತನ AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ),
                              new_ಎಣಿಕೆ_Overflow_Warning(c.flp, sigs.match_ಎಣಿಕೆ_Overflowp)});
            sigs.match_ಎಣಿಕೆ_Overflowp = nullptr;
        }

        ಪ್ರತ್ಯಾಗಮ sigs;
    }

    // Phase 1: Resolve combinational Links via fixed-point propagation.
    ಶೂನ್ಯ resolveLinks(LowerCtx& c, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* triggerExprp, ಧ್ರುವ ತಾರ್ಕಿಕ ಎಣಿಕೆ_Matches) {
        // datap() was freshly allocated in lower() -- all state signals start null.
        c.graph.m_startVertexp->datap()->stateSigp = triggerExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        c.graph.m_startVertexp->datap()->state_ಎಣಿಕೆ_Sigp
            = ನೂತನ AstExtend{c.flp, triggerExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), m_u32DTypep->width()};
        ಪ್ರತಿ (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ಧ್ರುವ vtxp = static_cast<SvaStateVertex*>(&vtxr);
            ಯದಿ (vtxp->datap()->stateVarp) {
                vtxp->datap()->stateSigp
                    = ನೂತನ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
                vtxp->datap()->state_ಎಣಿಕೆ_Sigp = ನೂತನ AstExtend{
                    c.flp, ನೂತನ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ},
                    m_u32DTypep->width()};
            } ಅನ್ಯಥಾ ಯದಿ (vtxp->datap()->delayRingVarp) {
                ಯದಿ (vtxp->m_isFixedDelayRing) {
                    // state_count = wrapped ? head : 0; state = state_count != 0;
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ countp = delay_Ring_Output_ಎಣಿಕೆ(c.flp, vtxp);
                    vtxp->datap()->stateSigp = ನೂತನ AstNeq{c.flp, countp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                                                          newTypedConstp(c.flp, m_u32DTypep, 0)};
                    vtxp->datap()->state_ಎಣಿಕೆ_Sigp = countp;
                } ಅನ್ಯಥಾ {
                    vtxp->datap()->stateSigp
                        = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_ಎಣಿಕೆ_Varp);
                    vtxp->datap()->state_ಎಣಿಕೆ_Sigp = ನೂತನ AstVarRef{
                        c.flp, vtxp->datap()->delay_Ring_Live_ಎಣಿಕೆ_Varp, VAccess::READ};
                }
            }
        }
        // Fixed-point propagation along zero-delay (Link) edges. Rebuild each
        // derived signal from its incoming edges on every pass; appending the same
        // contributions repeatedly makes merge expressions grow exponentially.
        ಧ್ರುವ size_t passes = 2 * c.graph.m_graph.vertices().size() + 2;
        ಪ್ರತಿ (size_t pass = 0; pass < passes; ++pass) {
            // Rebuild SAnd combiners once both sub-NFA terminals are available.
            ಪ್ರತಿ (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* ಧ್ರುವ vtxp = static_cast<SvaStateVertex*>(&vtxr);
                ಯದಿ (!vtxp->m_isAndCombiner) ಅನುವರ್ತ;
                // AndCombiner vertices always have both terminal pointers set.
                UASSERT_OBJ(vtxp->m_andLhsTermp && vtxp->m_andRhsTermp, vtxp,
                            "AndCombiner vertex missing LHS/RHS terminal");
                ಧ್ರುವ SvaStateVertex* ಧ್ರುವ lhsTermp = vtxp->m_andLhsTermp;
                ಧ್ರುವ SvaStateVertex* ಧ್ರುವ rhsTermp = vtxp->m_andRhsTermp;
                ಯದಿ (!lhsTermp->datap()->stateSigp || !rhsTermp->datap()->stateSigp) ಅನುವರ್ತ;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ matchLp
                    = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, vtxp->m_andLhsCondp);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ matchRp
                    = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, vtxp->m_andRhsCondp);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ doneLOrp = ನೂತನ AstLogOr{
                    c.flp, ನೂತನ AstVarRef{c.flp, vtxp->datap()->doneLVarp, VAccess::READ}, matchLp};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ doneROrp = ನೂತನ AstLogOr{
                    c.flp, ನೂತನ AstVarRef{c.flp, vtxp->datap()->doneRVarp, VAccess::READ}, matchRp};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ bothp = ನೂತನ AstLogAnd{c.flp, doneLOrp, doneROrp};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ oneNowp = ನೂತನ AstLogOr{c.flp, matchLp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                                                          matchRp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                ಯದಿ (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_ವೃಕ್ಷ(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = ನೂತನ AstLogAnd{c.flp, bothp, oneNowp};
                ಯದಿ (vtxp->datap()->state_ಎಣಿಕೆ_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_ಎಣಿಕೆ_Sigp->delete_ವೃಕ್ಷ(),
                                   vtxp->datap()->state_ಎಣಿಕೆ_Sigp);
                }
                vtxp->datap()->state_ಎಣಿಕೆ_Sigp = ನೂತನ AstExtend{
                    c.flp, vtxp->datap()->stateSigp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), m_u32DTypep->width()};
            }

            ಪ್ರತಿ (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* ಧ್ರುವ vtxp = static_cast<SvaStateVertex*>(&vtxr);
                ಯದಿ (vtxp == c.graph.m_startVertexp || vtxp->datap()->stateVarp
                    || vtxp->datap()->delayRingVarp || vtxp->m_isAndCombiner || vtxp->m_isMatch
                    || vtxp->m_isRejectSink) {
                    ಅನುವರ್ತ;
                }
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nextStatep = nullptr;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nextStateCountp = nullptr;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* next_State_ಎಣಿಕೆ_Overflowp = nullptr;
                ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& er : vtxp->inEdges()) {
                    ಧ್ರುವ SvaTransEdge& te = static_cast<ಧ್ರುವ SvaTransEdge&>(er);
                    ಧ್ರುವ SvaStateVertex* ಧ್ರುವ fromVtxp = te.fromVtxp();
                    ಯದಿ (!fromVtxp->datap()->stateSigp) ಅನುವರ್ತ;
                    UASSERT_OBJ(fromVtxp->datap()->state_ಎಣಿಕೆ_Sigp, fromVtxp,
                                "Link source missing stateCountSig");
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ contributionp = andCond(
                        c.flp, fromVtxp->datap()->stateSigp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), te.m_condp);
                    nextStatep = orExprs(c.flp, nextStatep, contributionp);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಎಣಿಕೆ_Contributionp
                        = fromVtxp->datap()->state_ಎಣಿಕೆ_Sigp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಎಣಿಕೆ_Contribution_Overflowp
                        = ಎಣಿಕೆ_Matches && fromVtxp->datap()->state_ಎಣಿಕೆ_Overflow_Sigp
                              ? fromVtxp->datap()->state_ಎಣಿಕೆ_Overflow_Sigp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)
                              : nullptr;
                    ಯದಿ (te.m_condp) {
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ enablep = te.m_condp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                        ಎಣಿಕೆ_Contributionp
                            = ಎಣಿಕೆ_Matches
                                  ? gateCheckedCountp(c.flp, ಎಣಿಕೆ_Contributionp,
                                                      ಎಣಿಕೆ_Contribution_Overflowp, enablep)
                                  : addCountp(c.flp, nullptr, ಎಣಿಕೆ_Contributionp, enablep);
                    }
                    nextStateCountp
                        = ಎಣಿಕೆ_Matches
                              ? addCheckedCountp(c.flp, nextStateCountp, ಎಣಿಕೆ_Contributionp,
                                                 next_State_ಎಣಿಕೆ_Overflowp,
                                                 ಎಣಿಕೆ_Contribution_Overflowp)
                              : addCountp(c.flp, nextStateCountp, ಎಣಿಕೆ_Contributionp);
                }
                ಯದಿ (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_ವೃಕ್ಷ(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = nextStatep;
                ಯದಿ (vtxp->datap()->state_ಎಣಿಕೆ_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_ಎಣಿಕೆ_Sigp->delete_ವೃಕ್ಷ(),
                                   vtxp->datap()->state_ಎಣಿಕೆ_Sigp);
                }
                vtxp->datap()->state_ಎಣಿಕೆ_Sigp = nextStateCountp;
                ಯದಿ (vtxp->datap()->state_ಎಣಿಕೆ_Overflow_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_ಎಣಿಕೆ_Overflow_Sigp->delete_ವೃಕ್ಷ(),
                                   vtxp->datap()->state_ಎಣಿಕೆ_Overflow_Sigp);
                }
                vtxp->datap()->state_ಎಣಿಕೆ_Overflow_Sigp = next_State_ಎಣಿಕೆ_Overflowp;
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    // Combine terminal/reject signals into final output expression.
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* assembleResult(Ast_ಗಂಟು_Cover_Or_Assert* ಧ್ರುವ assertp, ಧ್ರುವ ತಾರ್ಕಿಕ negated,
                                       Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ matchCondp, ಧ್ರುವ SignalSet& sigs,
                                       Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ** ಧ್ರುವ outMatchpp) {
        File_ಸಾಲು* ಧ್ರುವ flp = assertp->fileline();
        ಧ್ರುವ ತಾರ್ಕಿಕ isCover = VN_IS(assertp, Cover);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ terminalActivep = sigs.terminalActivep;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rejectBasep = sigs.rejectBasep;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ throughoutRejectp = sigs.throughoutRejectp;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ requiredStepRejectp = sigs.requiredStepRejectp;
        // Property negation (IEEE 1800-2023 16.12.1 `not`): invert match/reject.
        ಯದಿ (negated) {
            ಯದಿ (isCover) {
                ಯದಿ (terminalActivep)
                    VL_DO_DANGLING(terminalActivep->delete_ವೃಕ್ಷ(), terminalActivep);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* negRejectp = nullptr;
                ಯದಿ (matchCondp && rejectBasep) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ sampledCondp = sampled(matchCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ notCondp = ನೂತನ AstLogNot{flp, sampledCondp};
                    negRejectp = ನೂತನ AstLogAnd{flp, rejectBasep, notCondp};
                } ಅನ್ಯಥಾ ಯದಿ (rejectBasep) {
                    VL_DO_DANGLING(rejectBasep->delete_ವೃಕ್ಷ(), rejectBasep);
                }
                ಯದಿ (throughoutRejectp) negRejectp = orExprs(flp, negRejectp, throughoutRejectp);
                ಯದಿ (requiredStepRejectp)
                    negRejectp = orExprs(flp, negRejectp, requiredStepRejectp);
                ಪ್ರತ್ಯಾಗಮ negRejectp ? negRejectp : ನೂತನ AstConst{flp, AstConst::BitFalse{}};
            }
            // Negated assert/assume: output = !match.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* matchp = terminalActivep;
            ಯದಿ (matchCondp) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ sampledCondp = sampled(matchCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
                matchp = ನೂತನ AstLogAnd{flp, matchp, sampledCondp};
            }
            ಯದಿ (outMatchpp) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* notPMatchp = nullptr;
                ಯದಿ (matchCondp && rejectBasep) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ sampledCondp = sampled(matchCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
                    notPMatchp = ನೂತನ AstLogAnd{flp, rejectBasep->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                                               ನೂತನ AstLogNot{flp, sampledCondp}};
                } ಅನ್ಯಥಾ ಯದಿ (rejectBasep) {
                    notPMatchp = rejectBasep->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                }
                ಯದಿ (throughoutRejectp)
                    notPMatchp = orExprs(flp, notPMatchp, throughoutRejectp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
                ಯದಿ (requiredStepRejectp)
                    notPMatchp
                        = orExprs(flp, notPMatchp, requiredStepRejectp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
                *outMatchpp = notPMatchp;
            }
            ಯದಿ (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_ವೃಕ್ಷ(), throughoutRejectp);
            ಯದಿ (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_ವೃಕ್ಷ(), rejectBasep);
            ಯದಿ (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_ವೃಕ್ಷ(), requiredStepRejectp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ resultExprp = ನೂತನ AstLogNot{flp, matchp};
            ಪ್ರತ್ಯಾಗಮ resultExprp;
        }
        ಯದಿ (isCover) {
            ಯದಿ (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_ವೃಕ್ಷ(), throughoutRejectp);
            ಯದಿ (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_ವೃಕ್ಷ(), rejectBasep);
            ಯದಿ (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_ವೃಕ್ಷ(), requiredStepRejectp);
            ಯದಿ (matchCondp) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ sampledCondp = sampled(matchCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstLogAnd{flp, terminalActivep, sampledCondp};
            }
            ಪ್ರತ್ಯಾಗಮ terminalActivep;
        }
        // Assert/assume: output = !reject
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rejectp = nullptr;
        ಯದಿ (matchCondp && rejectBasep) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ sampledCondp = sampled(matchCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
            rejectp = ನೂತನ AstLogAnd{flp, rejectBasep, ನೂತನ AstLogNot{flp, sampledCondp}};
        } ಅನ್ಯಥಾ ಯದಿ (rejectBasep) {
            VL_DO_DANGLING(rejectBasep->delete_ವೃಕ್ಷ(), rejectBasep);
        }
        ಯದಿ (outMatchpp) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* matchExprp = terminalActivep->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            ಯದಿ (matchCondp) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ sp = sampled(matchCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
                matchExprp = ನೂತನ AstLogAnd{flp, matchExprp, sp};
            }
            *outMatchpp = matchExprp;
        }
        ಯದಿ (terminalActivep) VL_DO_DANGLING(terminalActivep->delete_ವೃಕ್ಷ(), terminalActivep);
        ಯದಿ (throughoutRejectp) rejectp = orExprs(flp, rejectp, throughoutRejectp);
        ಯದಿ (requiredStepRejectp) rejectp = orExprs(flp, rejectp, requiredStepRejectp);
        ಯದಿ (!rejectp) ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{flp, AstConst::BitTrue{}};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ resultExprp = ನೂತನ AstLogNot{flp, rejectp};
        ಪ್ರತ್ಯಾಗಮ resultExprp;
    }

    explicit SvaNfaLowering(Ast_ಗಂಟು_Module* modp)
        : m_modp{modp}
        , m_u32DTypep{modp->findBasicDType(VBasicDTypeKwd::UINT32)} {}
    ~SvaNfaLowering() {
        V3Stats::addStatSum("Assertions, NFA delay ring edge visits", m_statDelayRingEdgeVisits);
    }

    // Lower NFA graph to synthesizable AstAlways blocks and raw result signals.
    // Links are combinational; Edges are registered (NBA).
    SignalSet lower(Ast_ಗಂಟು_Cover_Or_Assert* ಧ್ರುವ assertp, SvaGraph& graph,
                    Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ matchCondp,
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ disableExprp, ಧ್ರುವ ತಾರ್ಕಿಕ need_Thread_Fail_ಎಣಿಕೆ,
                    ಧ್ರುವ ತಾರ್ಕಿಕ need_Throughout_Thread_Fail_ಎಣಿಕೆ) {
        File_ಸಾಲು* ಧ್ರುವ flp = assertp->fileline();
        AstCover* ಧ್ರುವ coverp = VN_CAST(assertp, Cover);
        ಧ್ರುವ ತಾರ್ಕಿಕ isSeqEvent = coverp && coverp->isSeqEvent();
        ಧ್ರುವ VAssertType assertType
            = isSeqEvent ? VAssertType{VAssertType::INTERNAL} : assertp->userType();
        ಧ್ರುವ VAssertDirectiveType directiveType
            = isSeqEvent ? VAssertDirectiveType{VAssertDirectiveType::INTERNAL}
                         : assertp->directive();
        ಧ್ರುವ std::string baseName = m_names.get("");

        // Number vertices with sequential colors for array indexing.
        ಪೂರ್ಣಾಙ್ಕ N = 0;
        ಪ್ರತಿ (V3GraphVertex& vtxr : graph.m_graph.vertices()) { vtxr.color(N++); }
        std::vector<SvaStateVertex*> vtx(N, nullptr);
        ಪ್ರತಿ (V3GraphVertex& vtxr : graph.m_graph.vertices()) {
            vtx[vtxr.color()] = static_cast<SvaStateVertex*>(&vtxr);
        }

        // Allocate per-vertex lowering data (stored via V3GraphVertex::userp()).
        std::vector<SvaVertexData> vertexData(N);
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < N; ++i) vtx[i]->userp(&vertexData[i]);

        // Identify registered vertices (targets of clocked edges).
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < N; ++i) {
            ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edger : vtx[i]->outEdges()) {
                ಧ್ರುವ SvaTransEdge& tedger = static_cast<ಧ್ರುವ SvaTransEdge&>(edger);
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ toIdx = tedger.toVtxp()->color();
                ಯದಿ (tedger.m_consumesCycle && tedger.toVtxp() != graph.m_matchVertexp
                    && !tedger.toVtxp()->m_isRejectSink) {
                    vtx[toIdx]->datap()->needsReg = ಸತ್ಯ;
                }
            }
        }

        // A packed range ring can emit several live endpoints without needing per-slot counts.
        // Widen only a ring whose input can already carry multiplicity.
        std::vector<ತಾರ್ಕಿಕ> stateMayHaveMultiplicity(N);
        std::vector<ತಾರ್ಕಿಕ> ringNeedsMultiplicity(N);
        std::vector<ಪೂರ್ಣಾಙ್ಕ> multiplicityWork;
        ಧ್ರುವ ತಾರ್ಕಿಕ ಎಣಿಕೆ_Matches = coverp && coverp->isCoverSeq() && !isSeqEvent;
        ಯದಿ (ಎಣಿಕೆ_Matches) {
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < N; ++i) {
                ಧ್ರುವ ತಾರ್ಕಿಕ multipleInputs = vtx[i]->inEdges().size() > 1;
                ಧ್ರುವ ತಾರ್ಕಿಕ isRing = vtx[i]->m_delayRingSize;
                ringNeedsMultiplicity[i] = isRing && multipleInputs;
                ಧ್ರುವ ತಾರ್ಕಿಕ startsMultiplicity = isRing
                                                    ? !vtx[i]->m_isFixedDelayRing
                                                    : multipleInputs && !vtx[i]->datap()->needsReg;
                ಯದಿ (startsMultiplicity) {
                    stateMayHaveMultiplicity[i] = ಸತ್ಯ;
                    multiplicityWork.ತಳ್ಳು_back(i);
                }
            }
            ಪ್ರತಿ (size_t wi = 0; wi < multiplicityWork.size(); ++wi) {
                ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edger : vtx[multiplicityWork[wi]]->outEdges()) {
                    ಧ್ರುವ SvaTransEdge& tedger = static_cast<ಧ್ರುವ SvaTransEdge&>(edger);
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ti = tedger.toVtxp()->color();
                    ಧ್ರುವ ತಾರ್ಕಿಕ isRing = vtx[ti]->m_delayRingSize;
                    ಯದಿ (isRing) ringNeedsMultiplicity[ti] = ಸತ್ಯ;
                    // Match and reject sinks have no outgoing edges, so they stop naturally.
                    ಧ್ರುವ ತಾರ್ಕಿಕ stopsMultiplicity = !isRing && vtx[ti]->datap()->needsReg;
                    ಯದಿ (stateMayHaveMultiplicity[ti] || stopsMultiplicity) { ಅನುವರ್ತ; }
                    stateMayHaveMultiplicity[ti] = ಸತ್ಯ;
                    multiplicityWork.ತಳ್ಳು_back(ti);
                }
            }
        }

        ಧ್ರುವ ತಾರ್ಕಿಕ asyncDisable = isAsyncDisable(disableExprp);
        // Registers cleared on a rising disable are also written by their clocked block.
        File_ಸಾಲು* ಧ್ರುವ regFlp = asyncDisable ? ನೂತನ File_ಸಾಲು{flp} : flp;
        ಯದಿ (asyncDisable) regFlp->warnOff(V3ErrorCode::MULTIDRIVEN, ಸತ್ಯ);

        AstVar* ಧ್ರುವ killVarp
            = ನೂತನ AstVar{flp, VVarType::MODULETEMP, baseName + "__kill", m_u32DTypep};
        killVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(killVarp);
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < N; ++i) {
            ಯದಿ (vtx[i]->m_isAndCombiner) {
                ಧ್ರುವ std::string base = baseName + "__a" + std::to_string(i);
                AstVar* ಧ್ರುವ lp = ನೂತನ AstVar{regFlp, VVarType::MODULETEMP, base + "_doneL",
                                              m_modp->findBitDType()};
                lp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(lp);
                vtx[i]->datap()->doneLVarp = lp;
                AstVar* ಧ್ರುವ rp = ನೂತನ AstVar{regFlp, VVarType::MODULETEMP, base + "_doneR",
                                              m_modp->findBitDType()};
                rp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(rp);
                vtx[i]->datap()->doneRVarp = rp;
                ಅನುವರ್ತ;
            }
            ಯದಿ (vtx[i]->m_delayRingSize) {
                ಧ್ರುವ std::string base = baseName + "__d" + std::to_string(i);
                ಧ್ರುವ ತಾರ್ಕಿಕ storesCounts = ಎಣಿಕೆ_Matches && ringNeedsMultiplicity[i];
                Ast_ಗಂಟು_DType* ringDTypep = nullptr;
                ಯದಿ (storesCounts) {
                    // int unsigned ring [size];
                    ringDTypep = ನೂತನ AstUnpackArrayDType{
                        flp, m_u32DTypep,
                        ನೂತನ AstRange{flp, static_cast<ಪೂರ್ಣಾಙ್ಕ>(vtx[i]->m_delayRingSize - 1), 0}};
                    v3Global.rootp()->typeTablep()->addTypesp(ringDTypep);
                } ಅನ್ಯಥಾ {
                    // bit [size-1:0] ring;
                    ringDTypep = m_modp->findBitDType(vtx[i]->m_delayRingSize,
                                                      vtx[i]->m_delayRingSize, VSigning::UNSIGNED);
                }
                AstVar* ಧ್ರುವ ringp
                    = ನೂತನ AstVar{flp, VVarType::MODULETEMP, base + "_ring", ringDTypep};
                ringp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(ringp);
                vtx[i]->datap()->delayRingVarp = ringp;
                vtx[i]->datap()->delayRingCounts = storesCounts;
                AstVar* ಧ್ರುವ headp
                    = ನೂತನ AstVar{flp, VVarType::MODULETEMP, base + "_head",
                                 storesCounts ? m_u32DTypep : m_modp->findBitDType()};
                headp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(headp);
                vtx[i]->datap()->delayRingHeadVarp = headp;
                // int unsigned idx;
                AstVar* ಧ್ರುವ idxp
                    = ನೂತನ AstVar{regFlp, VVarType::MODULETEMP, base + "_idx", m_u32DTypep};
                idxp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(idxp);
                vtx[i]->datap()->delayRingIdxVarp = idxp;
                // int unsigned live_count;
                AstVar* ಧ್ರುವ live_ಎಣಿಕೆ_Varp
                    = ನೂತನ AstVar{regFlp, VVarType::MODULETEMP, base + "_liveCount", m_u32DTypep};
                live_ಎಣಿಕೆ_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(live_ಎಣಿಕೆ_Varp);
                vtx[i]->datap()->delay_Ring_Live_ಎಣಿಕೆ_Varp = live_ಎಣಿಕೆ_Varp;
                AstVar* ಧ್ರುವ wrappedp = ನೂತನ AstVar{regFlp, VVarType::MODULETEMP,
                                                    base + "_wrapped", m_modp->findBitDType()};
                wrappedp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(wrappedp);
                vtx[i]->datap()->delayRingWrappedVarp = wrappedp;
                ಅನುವರ್ತ;
            }
            ಯದಿ (!vtx[i]->datap()->needsReg) ಅನುವರ್ತ;
            ಯದಿ (vtx[i] == graph.m_startVertexp || vtx[i]->m_isMatch) ಅನುವರ್ತ;
            ಧ್ರುವ std::string varName = baseName + "__s" + std::to_string(i);
            AstVar* ಧ್ರುವ varp
                = ನೂತನ AstVar{regFlp, VVarType::MODULETEMP, varName, m_modp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(varp);
            vtx[i]->datap()->stateVarp = varp;
        }

        // Build lowering context for phase sub-functions.
        LowerCtx c{flp,        senTreep,      disableExprp, matchCondp,
                   assertType, directiveType, killVarp,     graph};

        // Phase 1: Resolve combinational Links via fixed-point propagation.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ triggerExprp
            = isSeqEvent ? ನೂತನ AstConst{flp, AstConst::BitTrue{}}
                         : assertOnCond(flp, assertp->userType(), assertp->directive());
        resolveLinks(c, triggerExprp, ಎಣಿಕೆ_Matches);
        VL_DO_DANGLING(triggerExprp->delete_ವೃಕ್ಷ(), triggerExprp);

        // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
        emitStateRegisterNba(c);
        emitDelayRingUpdates(c, ಎಣಿಕೆ_Matches);
        emitAndCombinerDoneLatchNba(c);
        emitKillAckNba(c);

        // Phase 3/3a/3b: Compute terminal match/reject signals (cleans up stateSig).
        ಧ್ರುವ SignalSet sigs
            = computeSignals(c, need_Thread_Fail_ಎಣಿಕೆ, need_Throughout_Thread_Fail_ಎಣಿಕೆ, ಎಣಿಕೆ_Matches);

        // Strong s_always[m:n] end-of-simulation liveness: if any in-window state
        // is still set at $finish, the universal-quantifier window never completed
        // (IEEE 1800-2023 16.12.11 strong semantics). Fire the assertion failure
        // from a final block; V3Assert turns the DT_ERROR display into the standard
        // "Assertion failed in %m" message.
        // pending = |{strong state registers, strong delay live counts != 0};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pendingp = nullptr;
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < N; ++i) {
            ಯದಿ (!vtx[i]->m_strongPending) ಅನುವರ್ತ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pendingExprp = nullptr;
            ಯದಿ (vtx[i]->datap()->stateVarp) {
                pendingExprp = ನೂತನ AstVarRef{flp, vtx[i]->datap()->stateVarp, VAccess::READ};
            } ಅನ್ಯಥಾ ಯದಿ (vtx[i]->m_strongAlwaysRing) {
                pendingExprp
                    = delayRingHasLiveThreadsp(flp, vtx[i]->datap()->delay_Ring_Live_ಎಣಿಕೆ_Varp);
            } ಅನ್ಯಥಾ {
                ಅನುವರ್ತ;
            }
            ಯದಿ (!pendingp) {
                pendingp = pendingExprp;
            } ಅನ್ಯಥಾ {
                pendingp = ನೂತನ AstLogOr{flp, pendingp, pendingExprp};
            }
        }
        ಯದಿ (pendingp) {
            AstCExpr* ಧ್ರುವ assertOnp
                = ನೂತನ AstCExpr{flp, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()", 1};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = ನೂತನ AstLogAnd{flp, assertOnp, pendingp};
            AstDisplay* ಧ್ರುವ dispp
                = ನೂತನ AstDisplay{flp, VDisplayType::DT_ERROR, "", nullptr, nullptr};
            dispp->fmtp()->timeunit(m_modp->timeunit());
            Ast_ಗಂಟು_Stmt* ಧ್ರುವ firep = dispp;
            ಯದಿ (v3Global.opt.stopFail()) firep->addNext(ನೂತನ AstStop{flp, ಅಸತ್ಯ});
            m_modp->addStmtsp(ನೂತನ AstFinal{flp, ನೂತನ AstIf{flp, condp, firep}});
        }

        // Clear userp on every vertex before vertexData is destroyed.
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < N; ++i) vtx[i]->userp(nullptr);
        ಪ್ರತ್ಯಾಗಮ sigs;
    }
};

}  // namespace

//######################################################################
// Top-level visitor

ವರ್ಗ AssertNfaVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module being processed
    AstClocking* m_defaultClockingp = nullptr;  // Default clocking
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff
    SvaNfaLowering* m_loweringp = nullptr;  // NFA-to-hardware lowering engine
    Ast_Sen_ವೃಕ್ಷ* m_sampled_ಮೌಲ್ಯ_Clockp = nullptr;  // Inherited clock during scoped attachment
    V3UniqueNames m_propVarNames{"__Vpropvar"};  // Property-local variable names
    V3UniqueNames m_propTempNames{"__VnfaSampled"};  // Hoisted $sampled(propp) temps
    V3UniqueNames m_fail_ಎಣಿಕೆ_Names{"__VnfaRemainingFailCount"};  // Fail replay counter names
    std::set<ಧ್ರುವ AstProperty*> m_inliningProps;  // Recursion guard for inlineNamedProperty

    template <typename T_ಗಂಟು>
    ಶೂನ್ಯ visit_Sampled_ಮೌಲ್ಯ(T_ಗಂಟು* ಧ್ರುವ nodep) {
        ಯದಿ (m_sampled_ಮೌಲ್ಯ_Clockp && !nodep->sentreep()) {
            nodep->sentreep(m_sampled_ಮೌಲ್ಯ_Clockp->clone_ವೃಕ್ಷ(ಸತ್ಯ));
        }
        iterateChildren(nodep);
    }

    // Wire match vertex and mid-window sources for a successful NFA build.
    ಸ್ಥಿರ ಶೂನ್ಯ wireMatchAndMidSources(SvaGraph& graph, ಧ್ರುವ BuildResult& result, File_ಸಾಲು* flp) {
        graph.createMatchVertex();
        // Skip the main term Link when midSources already cover every
        // end-of-match (cover_sequence path); otherwise the per-mid extraction
        // double-counts via the merge vertex.
        ಯದಿ (!result.termIsMidMerge) { graph.addLink(result.termVertexp, graph.m_matchVertexp); }
        ಪ್ರತಿ (SvaStateVertex* srcVtxp : result.midSources) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = nullptr;
            ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ tc : srcVtxp->m_throughoutConds) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ tcClone = tc->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                condp = condp ? ನೂತನ AstLogAnd{flp, condp, tcClone} : tcClone;
            }
            graph.addLink(srcVtxp, graph.m_matchVertexp, condp);
            srcVtxp->m_isUnbounded = ಸತ್ಯ;
        }
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* getSequenceBodyExprp(ಧ್ರುವ AstSequence* seqp) {
        Ast_ಗಂಟು* bodyp = seqp->stmtsp();
        ಯಾವತ್ (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        ಪ್ರತ್ಯಾಗಮ VN_CAST(bodyp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
    }

    ಸ್ಥಿರ AstPropSpec* getPropertySpecp(ಧ್ರುವ AstProperty* propp) {
        Ast_ಗಂಟು* stmtp = propp->stmtsp();
        // V3LinkParse emits InitialStaticStmt for property-local variable
        // initialisers; the InitialAutomaticStmt variant only appears for
        // task/function-scope automatic lifetime, not properties.
        ಯಾವತ್ (stmtp
               && (VN_IS(stmtp, Var) || VN_IS(stmtp, InitialStaticStmt)
                   || VN_IS(stmtp, InitialAutomaticStmt))) {  // LCOV_EXCL_LINE
            stmtp = stmtp->nextp();
        }
        ಪ್ರತ್ಯಾಗಮ VN_CAST(stmtp, PropSpec);
    }

    ಶೂನ್ಯ inlineNamedProperty(AstPropSpec* outerSpecp, AstFuncRef* funcrefp,
                             ಧ್ರುವ AstProperty* propyp) {
        // Recursion guard: IEEE 1800-2023 16.12.1 forbids recursive properties.
        // V3Width emits "Recursive property call" for direct recursion before this
        // pass runs; this catches any nested-inlining cycle that slips past.
        ಯದಿ (!m_inliningProps.emplace(propyp).second) {
            funcrefp->v3error("Illegal recursive property reference");  // LCOV_EXCL_LINE
            ಪ್ರತ್ಯಾಗಮ;  // LCOV_EXCL_LINE
        }
        ಸಂರಚನಾ Guard final {
            std::set<ಧ್ರುವ AstProperty*>& setr;
            ಧ್ರುವ AstProperty* keyp;
            ~Guard() { setr.erase(keyp); }
        } guard{m_inliningProps, propyp};
        AstPropSpec* propSpecp = getPropertySpecp(propyp);
        UASSERT_OBJ(propSpecp, funcrefp, "Property has no body PropSpec");
        propSpecp = propSpecp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);

        ಧ್ರುವ V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propyp->stmtsp());
        std::unordered_map<ಧ್ರುವ AstVar*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> portMap;
        ಪ್ರತಿ (ಧ್ರುವ auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables to module-level temps (IEEE 16.10).
        std::unordered_map<ಧ್ರುವ AstVar*, AstVar*> localVarMap;
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = propyp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(stmtp, Var)) {
                ಯದಿ (!varp->isIO()) {
                    ಧ್ರುವ string newName = m_propVarNames.get(varp);
                    AstVar* ಧ್ರುವ newVarp = ನೂತನ AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        propSpecp->foreach([&](AstVarRef* refp) {
            ಧ್ರುವ auto portIt = portMap.find(refp->varp());
            ಯದಿ (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
                ಪ್ರತ್ಯಾಗಮ;
            }
            ಧ್ರುವ auto localIt = localVarMap.find(refp->varp());
            ಯದಿ (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        ಪ್ರತಿ (ಧ್ರುವ auto& tconnect : tconnects) {
            ತಳ್ಳು_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propSpecp, funcrefp);

        // Merge disable iff (IEEE 1800-2023 16.12.1)
        ಯದಿ (outerSpecp->disablep() && propSpecp->disablep()) {
            outerSpecp->v3error("disable iff expression before property call "
                                "and in its body is not legal");
            ತಳ್ಳು_Deletep(propSpecp->disablep()->unlinkFrBack());
        }
        ಯದಿ (outerSpecp->disablep()) {
            propSpecp->disablep(outerSpecp->disablep()->unlinkFrBack());
        }

        ಯದಿ (outerSpecp->sensesp() && propSpecp->sensesp()) {
            outerSpecp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Clock event before property call and in its body");
            ತಳ್ಳು_Deletep(propSpecp->sensesp()->unlinkFrBack());
        }
        ಯದಿ (outerSpecp->sensesp()) {
            AstSenItem* ಧ್ರುವ sensesp = outerSpecp->sensesp();
            sensesp->unlinkFrBack();
            propSpecp->sensesp(sensesp);
        }

        outerSpecp->replaceWith(propSpecp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(outerSpecp), outerSpecp);
    }

    ಶೂನ್ಯ inlineSequenceRef(AstFuncRef* funcrefp, AstSequence* seqp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ clonedp = bodyExprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);

        ಧ್ರುವ V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ಧ್ರುವ AstVar*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> portMap;
        ಪ್ರತಿ (ಧ್ರುವ auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ಧ್ರುವ auto it = portMap.find(refp->varp());
            ಯದಿ (it != portMap.end()) {
                refp->replaceWith(it->second->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
            }
        });
        ಪ್ರತಿ (ಧ್ರುವ auto& tconnect : tconnects) {
            ತಳ್ಳು_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(funcrefp), funcrefp);
        // Clear referenced flag so V3AssertPre cleanup does not emit
        // spurious UNSUPPORTED for sequences that were already inlined here.
        seqp->isReferenced(ಅಸತ್ಯ);
    }

    // Must run before hasMultiCycleExpr() so NFA sees sequence bodies.
    ಶೂನ್ಯ inlineAllSequenceRefs(Ast_ಗಂಟು* rootp) {
        ತಾರ್ಕಿಕ changed = ಸತ್ಯ;
        ಯಾವತ್ (changed) {
            changed = ಅಸತ್ಯ;
            rootp->foreach([&](AstFuncRef* funcrefp) {
                ಯದಿ (changed) ಪ್ರತ್ಯಾಗಮ;
                ಯದಿ (AstSequence* ಧ್ರುವ seqp = VN_CAST(funcrefp->taskp(), Sequence)) {
                    inlineSequenceRef(funcrefp, seqp);
                    changed = ಸತ್ಯ;
                }
            });
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ has_Multi_Cycle_ಅಭಿವ್ಯಕ್ತಿ(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಪ್ರತ್ಯಾಗಮ nodep->exists([](ಧ್ರುವ Ast_ಗಂಟು* np) {
            ಯದಿ (ಧ್ರುವ auto* ಧ್ರುವ ep = VN_CAST(np, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) ಪ್ರತ್ಯಾಗಮ ep->isMultiCycleSva();
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
    }

    ಸ್ಥಿರ VPropStrength effectiveAssertPropStrength(ಧ್ರುವ AstPropSpec* ಧ್ರುವ propSpecp) {
        ಯದಿ (propSpecp->propStrength() != VPropStrength::DEFAULT) ಪ್ರತ್ಯಾಗಮ propSpecp->propStrength();
        ಪ್ರತ್ಯಾಗಮ propSpecp->fileline()->language() <= V3LangCode::L1800_2005 ? VPropStrength::STRONG
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
        ಯದಿ (AstPropSpec* ಧ್ರುವ specp = VN_CAST(p, PropSpec)) p = specp->propp();
        ಯಾವತ್ (AstLogNot* ಧ್ರುವ notp = VN_CAST(p, LogNot)) p = notp->lhsp();
        AstUntil* ಧ್ರುವ untilp = VN_CAST(p, Until);
        ಯದಿ (!untilp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ auto hasSeq = [](ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ep) {
            ಪ್ರತ್ಯಾಗಮ ep->exists([](ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* np) { ಪ್ರತ್ಯಾಗಮ np->isMultiCycleSva(); });
        };
        ಯದಿ (hasSeq(untilp->lhsp()) || hasSeq(untilp->rhsp())) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಸಂರಚನಾ PropertyParts final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* triggerExprp = nullptr;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* seqExprp = nullptr;
        ತಾರ್ಕಿಕ isOverlapped = ಸತ್ಯ;
        ತಾರ್ಕಿಕ hasImplication = ಅಸತ್ಯ;
        ತಾರ್ಕಿಕ isFollowedBy = ಅಸತ್ಯ;  // True for #-# / #=# (non-vacuous-fail on antecedent miss)
    };

    ಸ್ಥಿರ PropertyParts decomposeProperty(Ast_ಗಂಟು* propp) {
        PropertyParts parts;
        ಯದಿ (AstPropSpec* ಧ್ರುವ specp = VN_CAST(propp, PropSpec)) { propp = specp->propp(); }
        ಯದಿ (AstImplication* ಧ್ರುವ implp = VN_CAST(propp, Implication)) {
            parts.hasImplication = ಸತ್ಯ;
            parts.isOverlapped = implp->isOverlapped();
            parts.isFollowedBy = implp->isFollowedBy();
            parts.triggerExprp = implp->lhsp();
            parts.seqExprp = implp->rhsp();
        } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = VN_CAST(propp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
            parts.triggerExprp = nullptr;
            parts.seqExprp = exprp;
        }
        ಪ್ರತ್ಯಾಗಮ parts;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ canSplitImplicationPassActions(ಧ್ರುವ PropertyParts& parts) {
        UASSERT(parts.hasImplication,
                "Implication pass action split requested without implication");
        UASSERT(parts.triggerExprp, "Implication pass action split requested without trigger");
        // Direct vacuous/nonvacuous classification uses the antecedent value in the current
        // assertion attempt. Leave delayed antecedents on the existing NFA pass path.
        ಪ್ರತ್ಯಾಗಮ !has_Multi_Cycle_ಅಭಿವ್ಯಕ್ತಿ(parts.triggerExprp);
    }

    ಸ್ಥಿರ ಶೂನ್ಯ splitImplicationPassActions(AstAssert* assertp, ಧ್ರುವ PropertyParts& parts,
                                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nonvacuousMatchp) {
        File_ಸಾಲು* ಧ್ರುವ flp = assertp->fileline();

        Ast_ಗಂಟು* ಧ್ರುವ passsp = assertp->passsp()->unlinkFrBackWithNext();
        Ast_ಗಂಟು* splitsp = nullptr;

        ಯದಿ (!parts.isFollowedBy) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ vacuousp
                = ನೂತನ AstLogNot{flp, sampled(parts.triggerExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ))};
            Ast_ಗಂಟು* ಧ್ರುವ vacuousBodyp = passsp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            splitsp = newPassOnIf(flp, vacuousp, vacuousBodyp, assertp->userType(),
                                  assertp->directive(), /*vacuous=*/ಸತ್ಯ);
        }

        AstIf* ಧ್ರುವ nonvacuousp
            = newPassOnIf(flp, nonvacuousMatchp, passsp, assertp->userType(), assertp->directive(),
                          /*vacuous=*/ಅಸತ್ಯ);
        splitsp = splitsp ? Ast_ಗಂಟು::addNext<Ast_ಗಂಟು, Ast_ಗಂಟು>(splitsp, nonvacuousp)
                          : static_cast<Ast_ಗಂಟು*>(nonvacuousp);
        assertp->addPasssp(splitsp);
    }

    // On a PropSpec-wrapped assertion whose NFA build failed with a semantic
    // error (errorEmitted), replace the body with a BitFalse const so later
    // passes see a well-formed AST. Returns true if replaced.
    ಶೂನ್ಯ replaceBodyOnBuildError(File_ಸಾಲು* flp, AstPropSpec* propSpecp, ತಾರ್ಕಿಕ errorEmitted) {
        ಯದಿ (!errorEmitted) ಪ್ರತ್ಯಾಗಮ;
        Ast_ಗಂಟು* ಧ್ರುವ innerPropp = propSpecp->propp();
        innerPropp->replaceWith(ನೂತನ AstConst{flp, AstConst::BitFalse{}});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(innerPropp), innerPropp);
    }

    // Hoist a leading clocking event (IEEE 1800-2023 16.7):
    ತಾರ್ಕಿಕ hoistClockedSeq(AstPropSpec* specp) {
        ಯಾವತ್ (AstSClocked* ಧ್ರುವ clockedp = VN_CAST(specp->propp(), SClocked)) {
            ಯದಿ (specp->sensesp()) {
                clockedp->v3warn(E_UNSUPPORTED, "Unsupported: multiclocked sequence or property");
                replaceBodyOnBuildError(specp->fileline(), specp, ಸತ್ಯ);
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
            ಪ್ರತಿ (ಧ್ರುವ AstSenItem* sp = clockedp->sensesp(); sp;
                 sp = VN_CAST(sp->nextp(), SenItem)) {
                ಯದಿ (!sp->edgeType().anEdge()) {
                    clockedp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: non-edge clocking event on a sequence; "
                                     "use an edge such as @(posedge clk)");
                    replaceBodyOnBuildError(specp->fileline(), specp, ಸತ್ಯ);
                    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                }
            }
            specp->sensesp(clockedp->sensesp()->unlinkFrBackWithNext());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ bodyp = clockedp->exprp()->unlinkFrBack();
            clockedp->replaceWith(bodyp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(clockedp), clockedp);
        }
        // A clocking event anywhere else in the sequence is not supported.
        ಧ್ರುವ AstSClocked* nestedp = nullptr;
        specp->propp()->foreach([&](ಧ್ರುವ AstSClocked* p) {
            ಯದಿ (!nestedp) nestedp = p;
        });
        ಯದಿ (nestedp) {
            nestedp->v3warn(E_UNSUPPORTED,
                            "Unsupported: clocking event inside sequence expression");
            replaceBodyOnBuildError(specp->fileline(), specp, ಸತ್ಯ);
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Build the NFA graph for a property body, handling both the antecedent
    // |-> consequent and simple sequence cases. Returns the consequent/body
    // BuildResult (invalid on parse/build failure).
    BuildResult buildAssertionGraph(SvaNfaBuilder& builder, SvaGraph& graph, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* seqBodyp,
                                    ಧ್ರುವ PropertyParts& parts, File_ಸಾಲು* flp) {
        ಯದಿ (!parts.hasImplication) ಪ್ರತ್ಯಾಗಮ builder.build(seqBodyp);

        graph.m_startVertexp = graph.createStateVertex();
        ಪ್ರತ್ಯಾಗಮ builder.buildImplicationEdges(parts.triggerExprp, seqBodyp, graph.m_startVertexp,
                                             parts.isOverlapped, parts.isFollowedBy,
                                             parts.triggerExprp, flp);
    }

    // Install pass-action gating and replay simultaneous per-thread failures.
    ಶೂನ್ಯ attachActionHandlers(AstAssert* ಧ್ರುವ assertp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* matchExprp,
                              Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ threadFailReplaySenTreep,
                              Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ threadFailCountp) {
        // Gate pass handler on match to prevent vacuous-pass firings.
        ಯದಿ (matchExprp) {
            // needMatch implies passsp() was non-null when evaluated above;
            // lowering does not mutate the assert's pass-action between the
            // two reads, so passsp() is still non-null here.
            Ast_ಗಂಟು* passsp = assertp->passsp();
            UASSERT_OBJ(passsp, assertp, "needMatch set but passsp is null");
            passsp->unlinkFrBackWithNext();
            File_ಸಾಲು* ಧ್ರುವ flp = assertp->fileline();
            AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{flp, matchExprp, passsp, nullptr};
            assertp->addPasssp(ifp);
            // Fail-handler prefix for overlapping instances (IEEE 16.12):
            // fires when reject=1 && match=1 in the same cycle.
            ಯದಿ (assertp->failsp()) assertp->prependFailsp(ifp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
        }

        ಯದಿ (threadFailCountp) {
            UASSERT_OBJ(threadFailReplaySenTreep, assertp,
                        "Thread fail count missing sensitivity tree");
            Ast_ಗಂಟು* ಧ್ರುವ failsp = assertp->failsp();
            File_ಸಾಲು* ಧ್ರುವ flp = assertp->fileline();
            // IEEE 1800-2023 16.12 requires one action-block evaluation per failed
            // thread. AstAssert handles the first, so replay the rest here.
            AstVar* ಧ್ರುವ remaining_Fail_ಎಣಿಕೆ_Varp
                = ನೂತನ AstVar{flp, VVarType::MODULETEMP, m_fail_ಎಣಿಕೆ_Names.get(""),
                             m_modp->findBasicDType(VBasicDTypeKwd::UINT32)};
            remaining_Fail_ಎಣಿಕೆ_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remaining_Fail_ಎಣಿಕೆ_Varp);
            Ast_ಗಂಟು* ಧ್ರುವ replayStmtsp = ನೂತನ AstAssign{
                flp, ನೂತನ AstVarRef{flp, remaining_Fail_ಎಣಿಕೆ_Varp, VAccess::WRITE}, threadFailCountp};
            AstLoop* ಧ್ರುವ replayLoopp = ನೂತನ AstLoop{flp};
            replayLoopp->addStmtsp(ನೂತನ AstLoopTest{
                flp, replayLoopp,
                ನೂತನ AstGt{flp, ನೂತನ AstVarRef{flp, remaining_Fail_ಎಣಿಕೆ_Varp, VAccess::READ},
                          newTypedConstp(flp, remaining_Fail_ಎಣಿಕೆ_Varp->dtypep(), 1)}});
            replayLoopp->addStmtsp(newIfAssertFailOn(failsp->clone_ವೃಕ್ಷ(ಸತ್ಯ), assertp->directive(),
                                                     assertp->userType()));
            AstSub* ಧ್ರುವ decrementedFailCountp
                = ನೂತನ AstSub{flp, ನೂತನ AstVarRef{flp, remaining_Fail_ಎಣಿಕೆ_Varp, VAccess::READ},
                             newTypedConstp(flp, remaining_Fail_ಎಣಿಕೆ_Varp->dtypep(), 1)};
            replayLoopp->addStmtsp(
                ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, remaining_Fail_ಎಣಿಕೆ_Varp, VAccess::WRITE},
                              decrementedFailCountp});
            replayStmtsp->addNext(replayLoopp);
            m_modp->addStmtsp(
                ನೂತನ AstAlways{flp, VAlwaysKwd::ALWAYS, threadFailReplaySenTreep, replayStmtsp});
        }
    }

    // Replace one VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). No-op if refp is not in matchMap.
    ಶೂನ್ಯ substituteMatchItemRef(AstVarRef* refp, unsigned K,
                                ಧ್ರುವ std::unordered_map<ಧ್ರುವ AstVar*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& matchMap) {
        ಧ್ರುವ auto it = matchMap.find(refp->varp());
        ಯದಿ (it == matchMap.end()) ಪ್ರತ್ಯಾಗಮ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = it->second->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        ಯದಿ (K > 0) {
            AstConst* ಧ್ರುವ ticksp = ನೂತನ AstConst{refp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, 32,
                                                  static_cast<uint32_t>(K)};
            AstPast* ಧ್ರುವ pastp = ನೂತನ AstPast{refp->fileline(), newp, ticksp, nullptr,
                                               /* propertyTiming */ ಸತ್ಯ};
            pastp->dtypeFrom(newp);
            newp = pastp;
        }
        refp->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
        ಪ್ರತ್ಯಾಗಮ;
    }

    // Recursively walk a consequent. Returns cycle length consumed and
    // substitutes each VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). Reports E_UNSUPPORTED on non-constant
    // delays or composite sequence operators and returns -1 on failure.
    ಪೂರ್ಣಾಙ್ಕ
    walk_Substitute_Match_ವಸ್ತುಗಳು(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, unsigned K,
                             ಧ್ರುವ std::unordered_map<ಧ್ರುವ AstVar*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& match_ವಸ್ತುಗಳು) {
        ಯದಿ (AstSExpr* ಧ್ರುವ sexprp = VN_CAST(nodep, SExpr)) {
            // IEEE 1800-2023 16.9.2: cycle_delay's lhsp is a constant_expression
            // and the delay form in a sequence is always `##N`, folded by
            // V3Const + V3Param before V3AssertNfa. Range form `##[m:n]` is the
            // only user-visible reject here.
            AstDelay* ಧ್ರುವ delayp = VN_AS(sexprp->delayp(), Delay);
            UASSERT_OBJ(delayp->isCycleDelay() && VN_IS(delayp->lhsp(), Const), sexprp,
                        "SVA cycle delay must have a constant lhsp");
            ಯದಿ (delayp->isRangeDelay()) {
                sexprp->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                              "non-constant cycle delay in consequent"
                                              " (IEEE 1800-2023 16.10)");
                ಪ್ರತ್ಯಾಗಮ -1;
            }
            ಧ್ರುವ unsigned delayCycles = VN_AS(delayp->lhsp(), Const)->toUInt();
            ಪೂರ್ಣಾಙ್ಕ preLen = 0;
            ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ prep = sexprp->preExprp()) {
                preLen = walk_Substitute_Match_ವಸ್ತುಗಳು(prep, K, match_ವಸ್ತುಗಳು);
                ಯದಿ (preLen < 0) ಪ್ರತ್ಯಾಗಮ -1;
            }
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ bodyLen
                = walk_Substitute_Match_ವಸ್ತುಗಳು(sexprp->exprp(), K + preLen + delayCycles, match_ವಸ್ತುಗಳು);
            ಯದಿ (bodyLen < 0) ಪ್ರತ್ಯಾಗಮ -1;
            ಪ್ರತ್ಯಾಗಮ preLen + delayCycles + bodyLen;
        }
        ಯದಿ (nodep->isMultiCycleSva()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                         "composite sequence operator in consequent"
                                         " (IEEE 1800-2023 16.10)");
            ಪ್ರತ್ಯಾಗಮ -1;
        }
        std::vector<AstVarRef*> refs;
        nodep->foreach([&refs](AstVarRef* p) { refs.ತಳ್ಳು_back(p); });
        ಪ್ರತಿ (AstVarRef* ಧ್ರುವ refp : refs) substituteMatchItemRef(refp, K, match_ವಸ್ತುಗಳು);
        ಪ್ರತ್ಯಾಗಮ 0;
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
    ತಾರ್ಕಿಕ liftMatchItemSubstitutions(PropertyParts& parts, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* seqBodyp) {
        ಯದಿ (!parts.hasImplication) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಧ್ರುವ ಅಭಿವ್ಯಕ್ತಿ_Stmtp = VN_CAST(parts.triggerExprp, ಅಭಿವ್ಯಕ್ತಿ_Stmt);
        ಯದಿ (!ಅಭಿವ್ಯಕ್ತಿ_Stmtp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // IEEE 1800-2023 16.10 BNF requires `(expr, match_item {, match_item})`
        // with at least one match item; V3LinkParse only emits ExprStmt for
        // this form and only emits AstAssign with VarRef LHS for each item.
        std::unordered_map<ಧ್ರುವ AstVar*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> match_ವಸ್ತುಗಳು;
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = ಅಭಿವ್ಯಕ್ತಿ_Stmtp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstAssign* ಧ್ರುವ assignp = VN_AS(stmtp, Assign);
            AstVarRef* ಧ್ರುವ lhsRefp = VN_AS(assignp->lhsp(), VarRef);
            match_ವಸ್ತುಗಳು[lhsRefp->varp()] = assignp->rhsp();
        }
        ಧ್ರುವ unsigned startK = parts.isOverlapped ? 0 : 1;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ length = walk_Substitute_Match_ವಸ್ತುಗಳು(seqBodyp, startK, match_ವಸ್ತುಗಳು);
        // Match-item substitution / strip mutates ancestor purity. Release
        // builds don't auto-clear caches on edits, so refresh here.
        VIsCached::clear_Cache_ವೃಕ್ಷ();
        ಯದಿ (length < 0) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ antBoolp = ಅಭಿವ್ಯಕ್ತಿ_Stmtp->resultp()->unlinkFrBack();
        ಅಭಿವ್ಯಕ್ತಿ_Stmtp->replaceWith(antBoolp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(ಅಭಿವ್ಯಕ್ತಿ_Stmtp), ಅಭಿವ್ಯಕ್ತಿ_Stmtp);
        parts.triggerExprp = antBoolp;
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Outcome counts for a property if/case are wrong in an outcome-multiplying
    // context. Returns that context, or nullptr when the shape is supported.
    ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* unsupportedPropertyControl(ಧ್ರುವ Ast_ಗಂಟು_Cover_Or_Assert* assertp,
                                                  ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* seqBodyp, ತಾರ್ಕಿಕ negated) {
        ಯದಿ (!hasPropertyControlConjunction(seqBodyp)) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಯದಿ (negated) ಪ್ರತ್ಯಾಗಮ "negation";
        ಯದಿ (VN_IS(assertp, Cover)) ಪ್ರತ್ಯಾಗಮ "cover";
        ಯದಿ (VN_AS(assertp, Assert)->passsp()) ಪ್ರತ್ಯಾಗಮ "a pass action";
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    // Inline property/sequence refs and reject unsupported shapes.
    // Returns the PropSpec to lower, or nullptr when fully handled here.
    AstPropSpec* prepareAssertionProp(Ast_ಗಂಟು_Cover_Or_Assert* assertp) {
        ಯದಿ (AstPropSpec* ಧ್ರುವ specp = VN_CAST(assertp->propp(), PropSpec)) {
            ಯದಿ (AstFuncRef* ಧ್ರುವ funcrefp = VN_CAST(specp->propp(), FuncRef)) {
                ಯದಿ (ಧ್ರುವ AstProperty* ಧ್ರುವ propyp = VN_CAST(funcrefp->taskp(), Property)) {
                    inlineNamedProperty(specp, funcrefp, propyp);
                }
            }
        }

        inlineAllSequenceRefs(assertp->propp());

        ಯದಿ (AstPropSpec* ಧ್ರುವ specp = VN_CAST(assertp->propp(), PropSpec)) {
            ಯದಿ (hoistClockedSeq(specp)) ಪ್ರತ್ಯಾಗಮ nullptr;
        }

        AstPropSpec* ಧ್ರುವ propp = VN_AS(assertp->propp(), PropSpec);
        ಯದಿ (!VN_IS(assertp, Cover)
            && effectiveAssertPropStrength(propp) == VPropStrength::STRONG) {
            propp->v3warn(E_UNSUPPORTED,
                          "Unsupported: strong property in " + assertp->verilogKwd() + ".");
            replaceBodyOnBuildError(assertp->fileline(), propp, /*errorEmitted=*/ಸತ್ಯ);
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }

        ಯದಿ (!has_Multi_Cycle_ಅಭಿವ್ಯಕ್ತಿ(propp)) ಪ್ರತ್ಯಾಗಮ nullptr;
        // A nested property instance keeps its body behind the call; lowering would drop it.
        ಯದಿ (propp->exists([](ಧ್ರುವ AstFuncRef* refp) { ಪ್ರತ್ಯಾಗಮ VN_IS(refp->taskp(), Property); })) {
            assertp->v3warn(E_UNSUPPORTED,
                            "Unsupported: property instance inside a multi-cycle property "
                            "expression");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(assertp->unlinkFrBack()), assertp);
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        ಯದಿ (is_Bare_ತುದಿ_Level_Until(propp)) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪ್ರತ್ಯಾಗಮ propp;
    }

    ಶೂನ್ಯ processAssertion(Ast_ಗಂಟು_Cover_Or_Assert* assertp) {
        ಯದಿ (assertp->immediate()) ಪ್ರತ್ಯಾಗಮ;

        AstPropSpec* ಧ್ರುವ propp = prepareAssertionProp(assertp);
        ಯದಿ (!propp) ಪ್ರತ್ಯಾಗಮ;

        PropertyParts parts = decomposeProperty(propp);
        UASSERT_OBJ(parts.seqExprp, propp, "Property body must be an expression");

        // Unwrap `not` (IEEE 1800-2023 16.12.1); odd count -> negated semantics.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* seqBodyp = parts.seqExprp;
        ತಾರ್ಕಿಕ negated = ಅಸತ್ಯ;
        ಯಾವತ್ (AstLogNot* ಧ್ರುವ notp = VN_CAST(seqBodyp, LogNot)) {
            negated = !negated;
            seqBodyp = notp->lhsp();
        }

        ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ propertyControlp
            = unsupportedPropertyControl(assertp, seqBodyp, negated);

        // Substitute property-local match-item refs in consequent with
        // $past(rhs, K) before NFA build (IEEE 1800-2023 16.10).
        ಯದಿ (liftMatchItemSubstitutions(parts, seqBodyp)) {
            AstPropSpec* ಧ್ರುವ psp = VN_CAST(assertp->propp(), PropSpec);
            UASSERT_OBJ(psp, assertp, "Concurrent assertion must have PropSpec");
            replaceBodyOnBuildError(assertp->fileline(), psp, /*errorEmitted=*/ಸತ್ಯ);
            ಪ್ರತ್ಯಾಗಮ;
        }

        AstCover* ಧ್ರುವ coverp = VN_CAST(assertp, Cover);
        ಧ್ರುವ ತಾರ್ಕಿಕ isCoverSeq = coverp && coverp->isCoverSeq();
        // A sequence event control is not an assertion directive; no default
        // disable iff, no assertion control
        ಧ್ರುವ ತಾರ್ಕಿಕ isSeqEvent = coverp && coverp->isSeqEvent();
        ಧ್ರುವ ತಾರ್ಕಿಕ ಎಣಿಕೆ_Matches = isCoverSeq && !isSeqEvent;
        // Inherit module defaults (IEEE 14.12, 16.15) when assertion has none.
        ಯದಿ (!propp->sensesp() && m_defaultClockingp) {
            propp->sensesp(m_defaultClockingp->sentreep()->sensesp()->clone_ವೃಕ್ಷ(ಸತ್ಯ));
        }
        ಯದಿ (!propp->disablep() && m_defaultDisablep && !isSeqEvent) {
            propp->disablep(m_defaultDisablep->condp()->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ));
        }
        ಯದಿ (!propp->sensesp()) ಪ್ರತ್ಯಾಗಮ;
        Ast_Sen_ವೃಕ್ಷ* senTreep
            = ನೂತನ Ast_Sen_ವೃಕ್ಷ{propp->fileline(), propp->sensesp()->clone_ವೃಕ್ಷ(ಸತ್ಯ)};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ disableExprp = propp->disablep();

        // NFA lowering clones repeated operands and may hoist them into an
        // always_comb block. Resolve implicit sampled-value clocks first, while
        // the enclosing assertion clock is still available.
        {
            VL_RESTORER(m_sampled_ಮೌಲ್ಯ_Clockp);
            m_sampled_ಮೌಲ್ಯ_Clockp = senTreep;
            iterate(propp->propp());
        }

        File_ಸಾಲು* ಧ್ರುವ flp = assertp->fileline();

        SvaGraph graph;
        SvaNfaBuilder builder{graph,      m_modp,     m_propTempNames,
                              isCoverSeq, isSeqEvent, coverp != nullptr};

        ಧ್ರುವ BuildResult result = buildAssertionGraph(builder, graph, seqBodyp, parts, flp);
        ಯದಿ (result.valid()) wireMatchAndMidSources(graph, result, flp);
        ಯದಿ (!result.valid()) {
            // Fall through to V3AssertPre for unsupported constructs; only
            // replace the body on real semantic errors. Any hoisted temps
            // from this attempt become orphan MODULETEMPs; V3Dead removes
            // them along with the dead always_comb driver.
            replaceBodyOnBuildError(flp, propp, result.errorEmitted);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(senTreep), senTreep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // After the build, so a construct the builder rejects reports itself.
        ಯದಿ (propertyControlp) {
            seqBodyp->v3warn(E_UNSUPPORTED,
                             "Unsupported: temporal property if/case with " << propertyControlp);
            replaceBodyOnBuildError(flp, propp, /*errorEmitted=*/ಸತ್ಯ);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(senTreep), senTreep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (ಎಣಿಕೆ_Matches && !v3Global.opt.coverageUser()) {
            SvaNfaBuilder::freeUnlinkedCondp(result.finalCondp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(senTreep), senTreep);
            Ast_ಗಂಟು* ಧ್ರುವ innerPropp = propp->propp();
            innerPropp->replaceWith(ನೂತನ AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(innerPropp), innerPropp);
            ಪ್ರತ್ಯಾಗಮ;
        }

        AstAssert* ಧ್ರುವ assertAssertp = VN_CAST(assertp, Assert);
        ಧ್ರುವ ತಾರ್ಕಿಕ splitImplicationPasssp = assertAssertp && assertAssertp->passsp()
                                            && parts.hasImplication
                                            && canSplitImplicationPassActions(parts);
        ಧ್ರುವ ತಾರ್ಕಿಕ needMatch = assertAssertp && assertAssertp->passsp()
                               && (!parts.hasImplication || splitImplicationPasssp);

        ಧ್ರುವ ತಾರ್ಕಿಕ needsThreadFailReplay
            = assertAssertp && assertAssertp->failsp() && !parts.hasImplication;
        ಧ್ರುವ auto signals
            = m_loweringp->lower(assertp, graph, senTreep, result.finalCondp, disableExprp,
                                 needsThreadFailReplay, needsThreadFailReplay && !negated);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* matchExprp = nullptr;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* outputExprp = m_loweringp->assembleResult(
            assertp, negated, result.finalCondp, signals, needMatch ? &matchExprp : nullptr);
        ಯದಿ (ಎಣಿಕೆ_Matches) {
            UASSERT_OBJ(signals.matchCountp, coverp, "Cover sequence missing match count");
            VL_DO_DANGLING(outputExprp->delete_ವೃಕ್ಷ(), outputExprp);
            propp->matchCountp(signals.matchCountp);
            outputExprp = ನೂತನ AstNeq{flp, signals.matchCountp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                                     newTypedConstp(flp, signals.matchCountp->dtypep(), 0)};
        }

        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ threadFailReplaySenTreep
            = signals.threadFailCountp ? senTreep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ) : nullptr;

        VL_DO_DANGLING(ತಳ್ಳು_Deletep(senTreep), senTreep);
        ಯದಿ (result.finalCondp && !result.finalCondp->backp()) ತಳ್ಳು_Deletep(result.finalCondp);

        ಯದಿ (splitImplicationPasssp) {
            splitImplicationPassActions(assertAssertp, parts, matchExprp);
        } ಅನ್ಯಥಾ {
            attachActionHandlers(assertAssertp, matchExprp, threadFailReplaySenTreep,
                                 signals.threadFailCountp);
        }

        Ast_ಗಂಟು* ಧ್ರುವ innerPropp = propp->propp();
        innerPropp->replaceWith(outputExprp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(innerPropp), innerPropp);

        UINFO(4, "NFA converted assertion at " << flp << endl);

        ಯದಿ (dumpGraphLevel() >= 6) graph.m_graph.dumpDotFilePrefixed("assert-nfa");
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_loweringp);
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultDisablep);
        m_modp = nodep;
        // A class nested in a module is in the scope of the module's default clocking
        ಯದಿ (!VN_IS(nodep, Class)) m_defaultClockingp = nullptr;
        m_defaultDisablep = nodep->defaultDisablep();
        SvaNfaLowering lowering{nodep};
        m_loweringp = &lowering;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstClocking* nodep) override {
        ಯದಿ (nodep->isDefault() && !m_defaultClockingp) m_defaultClockingp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstDefaultDisable* nodep) override {}
    ಶೂನ್ಯ visit(AstFell* nodep) override { visit_Sampled_ಮೌಲ್ಯ(nodep); }
    ಶೂನ್ಯ visit(AstPast* nodep) override { visit_Sampled_ಮೌಲ್ಯ(nodep); }
    ಶೂನ್ಯ visit(AstRose* nodep) override { visit_Sampled_ಮೌಲ್ಯ(nodep); }
    ಶೂನ್ಯ visit(AstStable* nodep) override { visit_Sampled_ಮೌಲ್ಯ(nodep); }
    ಶೂನ್ಯ visit(AstAssert* nodep) override { processAssertion(nodep); }
    ಶೂನ್ಯ visit(AstCover* nodep) override { processAssertion(nodep); }
    ಶೂನ್ಯ visit(AstRestrict* nodep) override {
        // Restrict property is ignored by simulators (IEEE 1800-2023 16.12.2).
        // Remove here so temporal SExpr don't leak to V3AssertPre.
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ಶೂನ್ಯ visit(AstAssertIntrinsic* nodep) override {}
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    explicit AssertNfaVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Top entry point

ಶೂನ್ಯ V3AssertNfa::assertNfaAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":" << endl);
    { AssertNfaVisitor{nodep}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("assertnfa", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
