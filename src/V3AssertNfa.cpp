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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3AssertNfa.h"

#অন্তর্ভাব "V3Assert.h"
#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3Graph.h"
#অন্তর্ভাব "V3Stats.h"
#অন্তর্ভাব "V3Task.h"
#অন্তর্ভাব "V3UniqueNames.h"

#অন্তর্ভাব <set>
#অন্তর্ভাব <unordered_set>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// NFA Graph Data Structures (V3Graph-derived per upstream convention)

নামাকাশ {

শ্রেণী SvaStateVertex;

// Per-vertex algorithm data, stored via V3GraphVertex::userp() during lowering
কাঠামো Sva_Vertex_তথ্য final {
    AstVar* stateVarp = nullptr;  // NBA state register for this vertex
    AstVar* delayRingVarp = nullptr;  // Packed occupancy bits or per-slot match counts
    AstVar* delay_Ring_শীর্ষ_Varp
        = nullptr;  // Cached outgoing slot; isolates readers from in-place writes
    AstVar* delayRingIdxVarp = nullptr;  // Next slot written in delayRingVarp
    AstVar* delay_Ring_Live_গুনতি_Varp = nullptr;  // Number of threads in delayRingVarp
    AstVar* delayRingWrappedVarp = nullptr;  // All slots written since the last clear
    AstVar* doneLVarp = nullptr;  // SAnd LHS done-latch
    AstVar* doneRVarp = nullptr;  // SAnd RHS done-latch
    Ast_গ্রন্থি_রাশিমালা* stateSigp = nullptr;  // Combinational state signal; OWNED during lowering
    Ast_গ্রন্থি_রাশিমালা* state_গুনতি_Sigp = nullptr;  // Cover-sequence match count; OWNED during lowering
    Ast_গ্রন্থি_রাশিমালা* state_গুনতি_Overflow_Sigp = nullptr;  // Count overflow; OWNED during lowering
    যৌক্তিক delayRingCounts = মিথ্যা;  // Ring slots hold cover-sequence match counts
    যৌক্তিক needsReg = মিথ্যা;  // True if vertex has incoming clocked edge
};

// NFA state vertex -- one per NFA position in the sequence evaluation
শ্রেণী SvaStateVertex final : সর্বজনীন V3GraphVertex {
    VL_RTTI_IMPL(SvaStateVertex, V3GraphVertex)
সর্বজনীন:
    // True if this is the sequence-match terminal vertex
    যৌক্তিক m_isMatch = মিথ্যা;
    // OWNED throughout-guard condition clones; IEEE 1800-2023 16.9.9
    std::vector<Ast_গ্রন্থি_রাশিমালা*> m_throughoutConds;
    // Nonzero for a ring-buffer vertex for ## delays.
    যৌক্তিক m_isFixedDelayRing = মিথ্যা;
    unsigned m_delay_Ring_আকার = 0;  // Number of ring slots. Range: max-min+1.
    Ast_গ্রন্থি_রাশিমালা* m_delayRingClearCondp = nullptr;  // local RHS for pure-boolean range
    Ast_গ্রন্থি_রাশিমালা* m_delayRingAdvanceCondp = nullptr;  // Advance only when this condition holds
    যৌক্তিক m_replayAbortReject = মিথ্যা;  // Compressed repetition needs per-thread abort replay
    // OWNED; enclosing-abort fire condition clearing state or suppressing guard rejection
    Ast_গ্রন্থি_রাশিমালা* m_abortClearp = nullptr;
    // OWNED; reject-abort fire condition rejecting all represented live threads
    Ast_গ্রন্থি_রাশিমালা* m_abortRejectp = nullptr;
    // Liveness terminal (IEEE weak semantics): reject must not fire from this source
    যৌক্তিক m_isUnbounded = মিথ্যা;
    // Temporal sequence AND combiner; IEEE 1800-2023 16.9.5
    যৌক্তিক m_isAndCombiner = মিথ্যা;
    SvaStateVertex* m_andLhsTermp = nullptr;  // LHS sub-NFA terminal vertex
    SvaStateVertex* m_andRhsTermp = nullptr;  // RHS sub-NFA terminal vertex
    Ast_গ্রন্থি_রাশিমালা* m_andLhsCondp = nullptr;  // OWNED; LHS final condition (may be nullptr)
    Ast_গ্রন্থি_রাশিমালা* m_andRhsCondp = nullptr;  // OWNED; RHS final condition (may be nullptr)
    // Reject sink for SAnd rejectOnFail wiring; not a state-signal source
    যৌক্তিক m_isRejectSink = মিথ্যা;
    // In-window vertex of a strong s_always[m:n]: if its state is still set at
    // end-of-simulation the universal-quantifier window never completed, which is
    // a liveness failure (IEEE 1800-2023 16.12.11 strong semantics).
    যৌক্তিক m_strongPending = মিথ্যা;
    // Ring introduced for the checked portion of a bounded strong s_always window.
    যৌক্তিক m_strongAlwaysRing = মিথ্যা;

    // CONSTRUCTORS
    explicit SvaStateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~SvaStateVertex() override {
        জন্য (Ast_গ্রন্থি_রাশিমালা* cp : m_throughoutConds) VL_DO_DANGLING(cp->delete_বৃক্ষ(), cp);
        যদি (m_delayRingClearCondp)
            VL_DO_DANGLING(m_delayRingClearCondp->delete_বৃক্ষ(), m_delayRingClearCondp);
        যদি (m_delayRingAdvanceCondp)
            VL_DO_DANGLING(m_delayRingAdvanceCondp->delete_বৃক্ষ(), m_delayRingAdvanceCondp);
        যদি (m_abortClearp) VL_DO_DANGLING(m_abortClearp->delete_বৃক্ষ(), m_abortClearp);
        যদি (m_abortRejectp) VL_DO_DANGLING(m_abortRejectp->delete_বৃক্ষ(), m_abortRejectp);
        যদি (m_andLhsCondp) VL_DO_DANGLING(m_andLhsCondp->delete_বৃক্ষ(), m_andLhsCondp);
        যদি (m_andRhsCondp) VL_DO_DANGLING(m_andRhsCondp->delete_বৃক্ষ(), m_andRhsCondp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string name() ধ্রুবক override {
        string name = "s" + cvtToStr(color());
        যদি (m_delay_Ring_আকার) {
            name += "\\n";
            name += m_isFixedDelayRing ? "fixed chain " : "range chain ";
            name += cvtToStr(m_delay_Ring_আকার) + " slots";
        }
        ফেরত name;
    }
    string dotColor() ধ্রুবক override {
        যদি (m_isMatch) ফেরত "red";
        যদি (m_delay_Ring_আকার) ফেরত "blue";
        যদি (m_isAndCombiner) ফেরত "purple";
        ফেরত "black";
    }
    // LCOV_EXCL_STOP
    // Access per-vertex algorithm data (valid only during lowering phase)
    Sva_Vertex_তথ্য* datap() ধ্রুবক { ফেরত static_cast<Sva_Vertex_তথ্য*>(userp()); }
};

// NFA transition edge -- clocked (##1) or combinational link (##0)
শ্রেণী SvaTransEdge final : সর্বজনীন V3GraphEdge {
    VL_RTTI_IMPL(SvaTransEdge, V3GraphEdge)
সর্বজনীন:
    Ast_গ্রন্থি_রাশিমালা* m_condp;  // Transition condition; nullptr = unconditional; OWNED
    যৌক্তিক m_consumesCycle;  // true = clocked edge (##1), false = link (##0/boolean)
    // Reject when source is active and condp is false; set only on
    // outermost required-step Link
    যৌক্তিক m_rejectOnFail = মিথ্যা;
    // Optional dynamic condition vertex for m_rejectOnFail. Used when the
    // success condition is another NFA state rather than a static expression.
    SvaStateVertex* m_condVtxp = nullptr;

    // CONSTRUCTORS
    SvaTransEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীর্ষ, Ast_গ্রন্থি_রাশিমালা* condp,
                 যৌক্তিক consumesCycle)
        : V3GraphEdge{graphp, fromp, শীর্ষ, /*weight=*/1}
        , m_condp{condp}
        , m_consumesCycle{consumesCycle} {}
    ~SvaTransEdge() override {
        যদি (m_condp) VL_DO_DANGLING(m_condp->delete_বৃক্ষ(), m_condp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string dotLabel() ধ্রুবক override { ফেরত m_consumesCycle ? "##1" : "link"; }
    string dotStyle() ধ্রুবক override { ফেরত m_consumesCycle ? "" : "dashed"; }
    // LCOV_EXCL_STOP
    // Typed accessors for NFA vertices
    SvaStateVertex* fromVtxp() ধ্রুবক { ফেরত static_cast<SvaStateVertex*>(fromp()); }
    SvaStateVertex* toVtxp() ধ্রুবক { ফেরত static_cast<SvaStateVertex*>(শীর্ষ()); }
};

// NFA graph container
শ্রেণী SvaGraph final {
সর্বজনীন:
    V3Graph m_graph;  // Owns all vertices and edges
    SvaStateVertex* m_startVertexp = nullptr;  // Trigger/start vertex
    SvaStateVertex* m_matchVertexp = nullptr;  // Sequence-match terminal vertex

    // Create a new state vertex
    SvaStateVertex* createStateVertex() { ফেরত নতুন SvaStateVertex{&m_graph}; }
    // Create the match terminal vertex
    SvaStateVertex* createMatchVertex() {
        SvaStateVertex* ধ্রুবক vtxp = createStateVertex();
        vtxp->m_isMatch = সত্য;
        m_matchVertexp = vtxp;
        ফেরত vtxp;
    }
    // Add a clocked transition edge (##1)
    SvaTransEdge* addClockedEdge(SvaStateVertex* fromp, SvaStateVertex* শীর্ষ,
                                 Ast_গ্রন্থি_রাশিমালা* condp = nullptr) {
        ফেরত নতুন SvaTransEdge{&m_graph, fromp, শীর্ষ, condp, /*consumesCycle=*/সত্য};
    }
    // Add a combinational link (##0 / boolean condition)
    SvaTransEdge* addLink(SvaStateVertex* fromp, SvaStateVertex* শীর্ষ,
                          Ast_গ্রন্থি_রাশিমালা* condp = nullptr) {
        ফেরত নতুন SvaTransEdge{&m_graph, fromp, শীর্ষ, condp, /*consumesCycle=*/মিথ্যা};
    }
    // Collect all edges into a flat vector for iteration.
    // Used by the lowering phase which needs global edge scans.
    std::vector<ধ্রুবক SvaTransEdge*> allEdges() ধ্রুবক {
        std::vector<ধ্রুবক SvaTransEdge*> ফলাফল;
        জন্য (ধ্রুবক V3GraphVertex& vtxr : m_graph.vertices()) {
            জন্য (ধ্রুবক V3GraphEdge& edger : vtxr.outEdges()) {
                ফলাফল.ঠেলো_back(static_cast<ধ্রুবক SvaTransEdge*>(&edger));
            }
        }
        ফেরত ফলাফল;
    }
};

//######################################################################
// Builder result: terminal vertex + optional final condition (match Link condition).
কাঠামো Build_ফলাফল final {
    SvaStateVertex* termVertexp;  // Primary terminal; contributes to both match and reject
    Ast_গ্রন্থি_রাশিমালা* finalCondp;  // nullptr = unconditional
    // Mid-window sources for range delays (pure boolean RHS): match-only (isUnbounded)
    std::vector<SvaStateVertex*> midSources;
    যৌক্তিক errorEmitted = মিথ্যা;  // Builder already emitted specific error; skip generic
    // For cover_sequence: when true, midSources already enumerate every
    // end-of-match, so wireMatchAndMidSources must NOT add the main
    // termVtxp -> matchVertex Link (would double-count via the merge vertex).
    যৌক্তিক termIsMidMerge = মিথ্যা;
    যৌক্তিক valid() ধ্রুবক { ফেরত termVertexp != nullptr; }
    স্থির Build_ফলাফল fail(যৌক্তিক errored = মিথ্যা) { ফেরত {nullptr, nullptr, {}, errored}; }
    স্থির Build_ফলাফল failWithError() { ফেরত {nullptr, nullptr, {}, সত্য}; }
};

// Parser-marked SAnd of overlapped implications: a property if/else/case.
স্থির যৌক্তিক hasPropertyControlConjunction(ধ্রুবক Ast_গ্রন্থি_রাশিমালা* nodep) {
    ফেরত nodep->exists([](ধ্রুবক AstSAnd* andp) { ফেরত andp->propertyControl(); });
}

স্থির AstConst* newTypedConstp(File_সারি* ধ্রুবক flp, ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep,
                                ধ্রুবক uint32_t মান) {
    AstConst* ধ্রুবক constp = নতুন AstConst{flp, AstConst::DTyped{}, dtypep};
    constp->অঙ্ক().setLong(মান);
    ফেরত constp;
}

স্থির Ast_গ্রন্থি_রাশিমালা* sampled(Ast_গ্রন্থি_রাশিমালা* exprp) {
    ফেরত নতুন AstSampled{exprp->fileline(), exprp, exprp->dtypep(), সত্য};
}

স্থির ধ্রুবক অক্ষর* assertPassOnQuery(যৌক্তিক vacuous) {
    স্থির constexpr ধ্রুবক অক্ষর* queries[2]
        = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
    ফেরত queries[vacuous];
}

স্থির Ast_গ্রন্থি_রাশিমালা* assertOnCond(File_সারি* flp, VAssertType type,
                                 VAssertDirectiveType directiveType) {
    যদি (!v3Global.opt.assertOn()) { ফেরত নতুন AstConst{flp, AstConst::BitFalse{}}; }
    ফেরত নতুন AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
}

স্থির Ast_গ্রন্থি_রাশিমালা* assertKillGet(File_সারি* flp, VAssertType type,
                                  VAssertDirectiveType directiveType) {
    ফেরত নতুন AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_KILL", type, directiveType),
                        32};
}

স্থির string assertActionControlPrefix(VAssertDirectiveType directiveType) {
    ধ্রুবক পূর্ণসংখ্যা controlled = !!(static_cast<পূর্ণসংখ্যা>(directiveType)
                              & (static_cast<পূর্ণসংখ্যা>(VAssertDirectiveType::ASSERT)
                                 | static_cast<পূর্ণসংখ্যা>(VAssertDirectiveType::COVER)
                                 | static_cast<পূর্ণসংখ্যা>(VAssertDirectiveType::ASSUME)));
    ধ্রুবক পূর্ণসংখ্যা checkRuntime = controlled & static_cast<পূর্ণসংখ্যা>(v3Global.opt.assertOn());
    ফেরত "("s + std::to_string(controlled ^ 1) + " || ("s + std::to_string(checkRuntime)
           + " && "s;
}

স্থির Ast_গ্রন্থি_রাশিমালা* assertPassOnCond(File_সারি* flp, VAssertType type,
                                     VAssertDirectiveType directiveType, যৌক্তিক vacuous) {
    ফেরত নতুন AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType)
            + "))"s,
        1};
}

স্থির Ast_গ্রন্থি_রাশিমালা* assertFailOnCond(File_সারি* flp, VAssertType type,
                                     VAssertDirectiveType directiveType) {
    ফেরত নতুন AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType) + "))"s,
        1};
}

স্থির AstIf* newPassOnIf(File_সারি* flp, Ast_গ্রন্থি_রাশিমালা* firep, Ast_গ্রন্থি* bodyp, VAssertType type,
                          VAssertDirectiveType directiveType, যৌক্তিক vacuous) {
    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক condp
        = নতুন AstLogAnd{flp, firep, assertPassOnCond(flp, type, directiveType, vacuous)};
    AstIf* ধ্রুবক ifp = নতুন AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(সত্য);
    ifp->user1(সত্য);
    ফেরত ifp;
}

স্থির Ast_গ্রন্থি_Stmt* newIfAssertFailOn(Ast_গ্রন্থি* bodyp, VAssertDirectiveType directiveType,
                                      VAssertType type) {
    File_সারি* ধ্রুবক flp = bodyp->fileline();
    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক condp = assertFailOnCond(flp, type, directiveType);
    AstIf* ধ্রুবক ifp = নতুন AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(সত্য);
    ifp->user1(সত্য);
    ফেরত ifp;
}

//######################################################################
// NFA Builder

শ্রেণী SvaNfaBuilder final {
    SvaGraph& m_graph;  // NFA graph being built
    Ast_গ্রন্থি_Module* ধ্রুবক m_modp;  // Module to receive hoisted sampled-prop temps
    V3UniqueNames& m_prop_অস্থায়ী_Names;  // Module-shared temp-var name source
    std::vector<Ast_গ্রন্থি_রাশিমালা*> m_temporal_Guard_স্তূপ;  // Guards active across nested temporal states
    // Outer abort conditions, AND-ed as !cond into inner abort edges
    // (IEEE 1800-2023 16.12.14 outer-wraps-inner).
    std::vector<Ast_গ্রন্থি_রাশিমালা*> m_outer_Abort_স্তূপ;
    যৌক্তিক m_inUnboundedScope = মিথ্যা;  // Sticky: nodes created after inherit liveness
    যৌক্তিক m_markStrongPending = মিথ্যা;  // Mark new vertices as strong s_always in-window
    যৌক্তিক m_isCover = মিথ্যা;  // Cover directives do not fail at end-of-simulation
    // IEEE 1800-2023 16.14.3 cover sequence: each end-of-match fires the action,
    // not just the first. Builder builds parallel-branch (no first-match-wins)
    // topology when true. Default false preserves cover_property semantics.
    যৌক্তিক m_isCoverSeq = মিথ্যা;
    // Unsupported endpoint topology must reject, not ignore, or the wait hangs
    যৌক্তিক m_isSeqEvent = মিথ্যা;
    যৌক্তিক m_inSequencePrefix = মিথ্যা;  // Prefix composition does not preserve every endpoint
    যৌক্তিক m_wantBoolLeaf = মিথ্যা;  // Building a |-> / |=> or top-level body (see buildOrMerge)

    কাঠামো RangeDelayRejectInfo final {
        SvaStateVertex* startp = nullptr;
        unsigned range = 0;
        পূর্ণসংখ্যা rhsLen = 0;
    };

    শূন্য warnEndpointUnsupported(File_সারি* flp, ধ্রুবক string& what) ধ্রুবক {
        যদি (m_isSeqEvent) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: sequence used as an event control with " << what);
        } অন্যথা {
            flp->v3warn(COVERIGN, "Ignoring unsupported: cover sequence with " << what);
        }
    }

    Ast_গ্রন্থি_রাশিমালা* throughoutCond(Ast_গ্রন্থি_রাশিমালা* baseCondp, File_সারি* flp) {
        যদি (m_temporal_Guard_স্তূপ.empty()) ফেরত baseCondp;
        // AND all active temporal guards (supports nesting)
        // Each must use $sampled values.
        Ast_গ্রন্থি_রাশিমালা* guardp = nullptr;
        জন্য (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক condp : m_temporal_Guard_স্তূপ) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক clonep = sampled(condp->clone_বৃক্ষ_Pure(মিথ্যা));
            যদি (!guardp) {
                guardp = clonep;
            } অন্যথা {
                guardp = নতুন AstLogAnd{flp, guardp, clonep};
            }
        }
        যদি (baseCondp) { guardp = নতুন AstLogAnd{flp, baseCondp, guardp}; }
        ফেরত guardp;
    }

    স্থির unsigned getConstUInt(Ast_গ্রন্থি_রাশিমালা* exprp) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক constp = V3Const::constifyEdit(exprp->clone_বৃক্ষ_Pure(মিথ্যা));
        ধ্রুবক AstConst* ধ্রুবক cp = VN_CAST(constp, Const);
        ধ্রুবক unsigned val = cp ? cp->toUInt() : 0;
        VL_DO_DANGLING(constp->delete_বৃক্ষ(), constp);
        ফেরত val;
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
    স্থির পূর্ণসংখ্যা fixed_দৈর্ঘ্য(Ast_গ্রন্থি_রাশিমালা* nodep) {
        যদি (AstSExpr* ধ্রুবক sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ধ্রুবক delayp = VN_CAST(sexprp->delayp(), Delay);
            যদি (!delayp || !delayp->isCycleDelay()) ফেরত -1;
            unsigned delayCycles;
            যদি (delayp->isRangeDelay()) {
                যদি (delayp->isUnbounded()) ফেরত -1;  // LCOV_EXCL_LINE
                ধ্রুবক unsigned minD = getConstUInt(delayp->lhsp());
                ধ্রুবক unsigned maxD = getConstUInt(delayp->rhsp());
                যদি (minD != maxD) ফেরত -1;
                delayCycles = minD;
            } অন্যথা {
                delayCycles = getConstUInt(delayp->lhsp());
            }
            পূর্ণসংখ্যা preLen = 0;
            যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক prep = sexprp->preExprp()) {
                preLen = fixed_দৈর্ঘ্য(prep);
                যদি (preLen < 0) ফেরত -1;  // LCOV_EXCL_LINE
            }
            ধ্রুবক পূর্ণসংখ্যা bodyLen = fixed_দৈর্ঘ্য(sexprp->exprp());
            যদি (bodyLen < 0) ফেরত -1;  // LCOV_EXCL_LINE
            ফেরত preLen + delayCycles + bodyLen;
        }
        যদি (AstSThroughout* ধ্রুবক throughp = VN_CAST(nodep, SThroughout)) {
            ফেরত fixed_দৈর্ঘ্য(throughp->rhsp());
        }
        যদি (AstSOr* ধ্রুবক orp = VN_CAST(nodep, SOr)) {
            // Alternatives must share one end cycle; buildSWithin relies on
            // this to pair the OR with an SIntersect.
            ধ্রুবক পূর্ণসংখ্যা lhsLen = fixed_দৈর্ঘ্য(orp->lhsp());
            ধ্রুবক পূর্ণসংখ্যা rhsLen = fixed_দৈর্ঘ্য(orp->rhsp());
            যদি (lhsLen < 0 || rhsLen < 0 || lhsLen != rhsLen) ফেরত -1;
            ফেরত lhsLen;
        }
        যদি (AstSWithin* ধ্রুবক withinp = VN_CAST(nodep, SWithin)) {
            // `seq1 within seq2` ends at seq2's end cycle (IEEE 16.9.10).
            ধ্রুবক পূর্ণসংখ্যা lhsLen = fixed_দৈর্ঘ্য(withinp->lhsp());
            ধ্রুবক পূর্ণসংখ্যা rhsLen = fixed_দৈর্ঘ্য(withinp->rhsp());
            যদি (lhsLen < 0 || rhsLen < 0 || lhsLen > rhsLen) ফেরত -1;
            ফেরত rhsLen;
        }
        // LCOV_EXCL_START -- defensive: V3AssertPre rejects composite SVA ops
        // nested in an intersect arm before fixedLength runs (clock-context
        // resolution fails). Kept as a guard in case future parser relaxations
        // permit it.
        যদি (nodep->isMultiCycleSva()) ফেরত -1;
        // LCOV_EXCL_STOP
        // Plain boolean expression (no SVA constructs) -- 0 cycles.
        ফেরত 0;
    }

    // Contiguous match-length range [lo,hi] for an operand whose length varies
    // from AT MOST ONE ranged cycle delay; {-1,-1} otherwise. Drives the
    // variable-length `intersect` lowering (IEEE 1800-2023 16.9.6): more than
    // one ranged delay would make the per-length realization ambiguous, so it
    // is reported unsupported rather than mis-paired.
    স্থির std::pair<পূর্ণসংখ্যা, পূর্ণসংখ্যা> দৈর্ঘ্য_Range(Ast_গ্রন্থি_রাশিমালা* nodep) {
        যদি (AstSExpr* ধ্রুবক sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ধ্রুবক delayp = VN_CAST(sexprp->delayp(), Delay);
            যদি (!delayp || !delayp->isCycleDelay()) ফেরত {-1, -1};
            std::pair<পূর্ণসংখ্যা, পূর্ণসংখ্যা> delayRange;
            যদি (delayp->isRangeDelay()) {
                যদি (delayp->isUnbounded()) ফেরত {-1, -1};
                ধ্রুবক unsigned minD = getConstUInt(delayp->lhsp());
                ধ্রুবক unsigned maxD = getConstUInt(delayp->rhsp());
                delayRange = {minD, maxD};
            } অন্যথা {
                ধ্রুবক unsigned d = getConstUInt(delayp->lhsp());
                delayRange = {d, d};
            }
            std::pair<পূর্ণসংখ্যা, পূর্ণসংখ্যা> preRange{0, 0};
            যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক prep = sexprp->preExprp()) {
                preRange = দৈর্ঘ্য_Range(prep);
                যদি (preRange.first < 0) ফেরত {-1, -1};
            }
            ধ্রুবক std::pair<পূর্ণসংখ্যা, পূর্ণসংখ্যা> bodyRange = দৈর্ঘ্য_Range(sexprp->exprp());
            যদি (bodyRange.first < 0) ফেরত {-1, -1};
            ধ্রুবক পূর্ণসংখ্যা variableParts = (preRange.first != preRange.second)
                                      + (delayRange.first != delayRange.second)
                                      + (bodyRange.first != bodyRange.second);
            যদি (variableParts > 1) ফেরত {-1, -1};
            ফেরত {preRange.first + delayRange.first + bodyRange.first,
                    preRange.second + delayRange.second + bodyRange.second};
        }
        যদি (AstSThroughout* ধ্রুবক throughp = VN_CAST(nodep, SThroughout)) {
            ফেরত দৈর্ঘ্য_Range(throughp->rhsp());
        }
        যদি (nodep->isMultiCycleSva()) ফেরত {-1, -1};
        ফেরত {0, 0};  // plain boolean -- 0 cycles
    }

    // Clone `operand` with its sole variable ranged cycle delay pinned so the
    // total match length is exactly `len`. `lo` is the operand's minimum length
    // (lengthRange().first). A fixed operand has no such delay and is returned
    // as a plain clone (callers only request its single achievable length).
    স্থির Ast_গ্রন্থি_রাশিমালা* realize_At_দৈর্ঘ্য(Ast_গ্রন্থি_রাশিমালা* operand, পূর্ণসংখ্যা len, পূর্ণসংখ্যা lo) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক clonep = operand->clone_বৃক্ষ_Pure(মিথ্যা);
        AstDelay* rangeDelayp = nullptr;
        clonep->foreach([&](AstDelay* dp) {
            যদি (!rangeDelayp && dp->isRangeDelay() && !dp->isUnbounded()
                && getConstUInt(dp->lhsp()) != getConstUInt(dp->rhsp())) {
                rangeDelayp = dp;
            }
        });
        যদি (rangeDelayp) {
            File_সারি* ধ্রুবক flp = rangeDelayp->fileline();
            ধ্রুবক unsigned pinned = getConstUInt(rangeDelayp->lhsp()) + (len - lo);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক oldMinp = rangeDelayp->lhsp();
            oldMinp->replaceWith(নতুন AstConst{flp, pinned});
            VL_DO_DANGLING(oldMinp->delete_বৃক্ষ(), oldMinp);
            // Drop the max bound so it lowers as a fixed `##d`, not `##[d:d]`.
            Ast_গ্রন্থি* ধ্রুবক oldMaxp = rangeDelayp->rhsp()->unlinkFrBack();
            VL_DO_DANGLING(oldMaxp->delete_বৃক্ষ(), oldMaxp);
        }
        ফেরত clonep;
    }

    // Cuts AST size from O(N * sizeof(exprp)) to O(N) + O(sizeof(exprp)) by
    // sharing a single `VarRef` across N check edges. Hoist also matches the
    // IEEE 1800-2023 16.9.9 "single preponed-region snapshot" semantic for
    // any exprp -- even an impure one would now evaluate exactly once per
    // clock instead of N times. Orphan temps from failed builds are unused
    // MODULETEMPs and are removed by V3Dead.
    AstVar* tryHoistSampled(Ast_গ্রন্থি_রাশিমালা* exprp, File_সারি* flp, unsigned clone_গুনতি) {
        constexpr unsigned kHoistThreshold = 2;
        যদি (clone_গুনতি < kHoistThreshold) ফেরত nullptr;
        AstVar* ধ্রুবক অস্থায়ী_Varp
            = নতুন AstVar{flp, VVarType::MODULETEMP, m_prop_অস্থায়ী_Names.get(exprp), exprp->dtypep()};
        m_modp->addStmtsp(অস্থায়ী_Varp);
        AstAssign* ধ্রুবক assignp = নতুন AstAssign{flp, নতুন AstVarRef{flp, অস্থায়ী_Varp, VAccess::WRITE},
                                                 sampled(exprp->clone_বৃক্ষ_Pure(মিথ্যা))};
        m_modp->addStmtsp(নতুন AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, assignp});
        ফেরত অস্থায়ী_Varp;
    }

    স্থির Ast_গ্রন্থি_রাশিমালা* sampledRefOrClone(AstVar* hoistVarp, Ast_গ্রন্থি_রাশিমালা* exprp, File_সারি* flp) {
        যদি (hoistVarp) ফেরত নতুন AstVarRef{flp, hoistVarp, VAccess::READ};
        ফেরত sampled(exprp->clone_বৃক্ষ_Pure(মিথ্যা));
    }

    // Create vertex and inherit temporal guards from the current scope.
    SvaStateVertex* scopedCreateVertex() {
        SvaStateVertex* ধ্রুবক vtxp = m_graph.createStateVertex();
        জন্য (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক cp : m_temporal_Guard_স্তূপ) {
            vtxp->m_throughoutConds.ঠেলো_back(cp->clone_বৃক্ষ_Pure(মিথ্যা));
        }
        যদি (m_inUnboundedScope) vtxp->m_isUnbounded = সত্য;
        যদি (m_markStrongPending) vtxp->m_strongPending = সত্য;
        ফেরত vtxp;
    }

    // AND current temporal guards into every edge/link.
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* শীর্ষ, Ast_গ্রন্থি_রাশিমালা* condp,
                              File_সারি* flp) {
        ফেরত m_graph.addLink(fromp, শীর্ষ, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* শীর্ষ, File_সারি* flp) {
        ফেরত m_graph.addLink(fromp, শীর্ষ, throughoutCond(nullptr, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* শীর্ষ, Ast_গ্রন্থি_রাশিমালা* condp,
                              File_সারি* flp) {
        ফেরত m_graph.addClockedEdge(fromp, শীর্ষ, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* শীর্ষ, File_সারি* flp) {
        ফেরত m_graph.addClockedEdge(fromp, শীর্ষ, throughoutCond(nullptr, flp));
    }

    SvaStateVertex* addDelayChain(SvaStateVertex* startp, unsigned আকার_2, File_সারি* flp,
                                  যৌক্তিক isFixed = সত্য, Ast_গ্রন্থি_রাশিমালা* clearCondp = nullptr,
                                  Ast_গ্রন্থি_রাশিমালা* advanceCondp = nullptr) {
        যদি (isFixed && আকার_2 == 0) ফেরত startp;
        UASSERT_OBJ(আকার_2 > 0, startp, "Delay chain needs at least one slot");
        যদি (isFixed && আকার_2 == 1 && !advanceCondp) {
            SvaStateVertex* ধ্রুবক nextp = scopedCreateVertex();
            guardedEdge(startp, nextp, flp);
            ফেরত nextp;
        }
        SvaStateVertex* ধ্রুবক ringVtxp = scopedCreateVertex();
        ringVtxp->m_isFixedDelayRing = isFixed;
        ringVtxp->m_delay_Ring_আকার = আকার_2;
        যদি (clearCondp) {
            UASSERT_OBJ(!isFixed, startp, "Fixed delay cannot have a clear condition");
            ringVtxp->m_delayRingClearCondp = clearCondp->clone_বৃক্ষ_Pure(মিথ্যা);
        }
        ringVtxp->m_delayRingAdvanceCondp = advanceCondp;
        যদি (isFixed) {
            guardedEdge(startp, ringVtxp, flp);
        } অন্যথা {
            guardedLink(startp, ringVtxp, flp);
        }
        ফেরত ringVtxp;
    }

    // Build NFA for an SExpr. finalCond = RHS (not yet added as a vertex).
    // isTopLevelStep: marks outermost required boolean check as rejectOnFail.
    // Apply a range delay `##[M:N]` to currentp. Returns true on success. On
    // failure, emits a diagnostic and returns false.
    যৌক্তিক applyRangeDelay(AstSExpr* sexprp, SvaStateVertex*& currentp,
                         std::vector<SvaStateVertex*>& midSources,
                         RangeDelayRejectInfo* rangeRejectInfop = nullptr) {
        File_সারি* ধ্রুবক flp = sexprp->fileline();
        AstDelay* ধ্রুবক delayp = VN_AS(sexprp->delayp(), Delay);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsExprp = sexprp->exprp();
        ধ্রুবক unsigned minDelay = getConstUInt(delayp->lhsp());
        যদি (delayp->isUnbounded()) {
            // `##[M:$]`: wait M cycles, then retain every matured attempt in a
            // one-slot ring while also exposing the first eligible tick.
            currentp = addDelayChain(currentp, minDelay, flp);
            SvaStateVertex* ধ্রুবক waitRingp = addDelayChain(currentp, 1, flp, মিথ্যা);
            // The ring's live count is updated in NBA, so this link retains the previous
            // tick's live attempts alongside new arrivals for the next tick.
            guardedLink(waitRingp, waitRingp, flp);
            SvaStateVertex* ধ্রুবক mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(waitRingp, mergeVtxp, flp);
            currentp = mergeVtxp;
            currentp->m_isUnbounded = সত্য;  // Liveness, so no reject.
            m_inUnboundedScope = সত্য;
            ফেরত সত্য;
        }
        ধ্রুবক unsigned maxDelay = getConstUInt(delayp->rhsp());
        যদি (minDelay == maxDelay) {
            currentp = addDelayChain(currentp, minDelay, flp);
            ফেরত সত্য;
        }
        ধ্রুবক unsigned range = maxDelay - minDelay;
        currentp = addDelayChain(currentp, minDelay, flp);
        ধ্রুবক যৌক্তিক multiCycleRhs = rhsExprp->isMultiCycleSva();
        ধ্রুবক Ast_গ্রন্থি_রাশিমালা* ধ্রুবক preExprp = sexprp->preExprp();
        // Prefix and nested-sequence composition can lose endpoints or their multiplicity
        // required by cover sequence (IEEE 1800-2023 16.14.3), regardless of the range width.
        যদি (m_isCoverSeq
            && (multiCycleRhs || m_inSequencePrefix
                || (preExprp && preExprp->isMultiCycleSva()))) {
            warnEndpointUnsupported(flp, "this ranged cycle delay");
            ফেরত মিথ্যা;
        }
        যদি (m_inSequencePrefix) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: Bounded ranged cycle delay in a sequence prefix");
            ফেরত মিথ্যা;
        }
        যদি (multiCycleRhs) {
            // Launch a candidate at every eligible tick. Candidate-local misses are not
            // assertion rejects while a later position can still match.
            যদি (rangeRejectInfop) {
                ধ্রুবক পূর্ণসংখ্যা rhsLen = fixed_দৈর্ঘ্য(rhsExprp);
                যদি (rhsLen >= 0) *rangeRejectInfop = {currentp, range, rhsLen};
            }
            SvaStateVertex* ধ্রুবক tailp = addDelayChain(currentp, range + 1U, flp, মিথ্যা);
            SvaStateVertex* ধ্রুবক mergeVtxp = scopedCreateVertex();
            mergeVtxp->m_isUnbounded = সত্য;
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(tailp, mergeVtxp, flp);
            currentp = mergeVtxp;
            m_inUnboundedScope = সত্য;
        } অন্যথা {
            // The first eligible tick is explicit. The ring retains attempts for the
            // remaining ticks and exposes the final tick for rejection. Only properties
            // clear on success, since cover sequence counts every endpoint.
            midSources.ঠেলো_back(currentp);
            currentp = addDelayChain(currentp, range + 1U, flp, মিথ্যা,
                                     m_isCoverSeq ? nullptr : rhsExprp);
        }
        ফেরত সত্য;
    }

    শূন্য addFiniteRangeReject(ধ্রুবক RangeDelayRejectInfo& info, ধ্রুবক Build_ফলাফল& ফলাফল,
                              File_সারি* flp) {
        যদি (!info.startp) ফেরত;

        SvaStateVertex* ধ্রুবক expiryVtxp
            = addDelayChain(info.startp, info.range + info.rhsLen, flp);
        SvaStateVertex* ধ্রুবক expiryMatchp = scopedCreateVertex();
        std::vector<SvaStateVertex*> sources = ফলাফল.midSources;
        sources.ঠেলো_back(ফলাফল.termVertexp);
        জন্য (SvaStateVertex* ধ্রুবক srcp : sources) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক condp
                = ফলাফল.finalCondp ? sampled(ফলাফল.finalCondp->clone_বৃক্ষ_Pure(মিথ্যা)) : nullptr;
            SvaStateVertex* ধ্রুবক successNowp = scopedCreateVertex();
            guardedLink(srcp, successNowp, condp, flp);
            // Retain a success through the latest deadline it can satisfy.
            SvaStateVertex* ধ্রুবক historyp
                = addDelayChain(successNowp, info.range + 1U, flp, মিথ্যা);
            guardedLink(successNowp, expiryMatchp, flp);
            guardedLink(historyp, expiryMatchp, flp);
        }

        SvaStateVertex* ধ্রুবক sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = সত্য;
        SvaTransEdge* ধ্রুবক rejectp = m_graph.addLink(expiryVtxp, sinkVtxp);
        rejectp->m_rejectOnFail = সত্য;
        rejectp->m_condVtxp = expiryMatchp;
    }

    Build_ফলাফল buildSExpr(AstSExpr* sexprp, SvaStateVertex* entryVtxp,
                           যৌক্তিক is_শীর্ষ_Level_Step = মিথ্যা) {
        AstDelay* ধ্রুবক delayp = VN_CAST(sexprp->delayp(), Delay);
        যদি (!delayp || !delayp->isCycleDelay()) ফেরত Build_ফলাফল::fail();

        File_সারি* ধ্রুবক flp = sexprp->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp = sexprp->exprp();

        // Handle LHS (preExpr)
        SvaStateVertex* currentp = entryVtxp;
        যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক preExprp = sexprp->preExprp()) {
            VL_RESTORER(m_inSequencePrefix);
            m_inSequencePrefix = সত্য;
            ধ্রুবক Build_ফলাফল pre = build_রাশিমালা(preExprp, currentp, is_শীর্ষ_Level_Step);
            যদি (!pre.valid()) ফেরত Build_ফলাফল::fail(pre.errorEmitted);  // LCOV_EXCL_LINE
            যদি (pre.finalCondp) {
                SvaStateVertex* ধ্রুবক condVtxp = scopedCreateVertex();
                SvaTransEdge* ধ্রুবক edgep = guardedLink(
                    pre.termVertexp, condVtxp, sampled(pre.finalCondp->clone_বৃক্ষ_Pure(মিথ্যা)), flp);
                যদি (is_শীর্ষ_Level_Step && !pre.termVertexp->m_isUnbounded && !m_inUnboundedScope) {
                    // Do not mark liveness sources: first boolean check is deferred.
                    edgep->m_rejectOnFail = সত্য;
                }
                freeUnlinkedCondp(pre.finalCondp);
                currentp = condVtxp;
            } অন্যথা {
                currentp = pre.termVertexp;
            }
        }

        // Handle delay
        std::vector<SvaStateVertex*> rangeMidSources;
        RangeDelayRejectInfo rangeRejectInfo;
        ধ্রুবক যৌক্তিক addRangeReject = is_শীর্ষ_Level_Step && !m_inUnboundedScope;
        যদি (delayp->isRangeDelay()) {
            যদি (!applyRangeDelay(sexprp, currentp, rangeMidSources,
                                 addRangeReject ? &rangeRejectInfo : nullptr)) {
                ফেরত Build_ফলাফল::failWithError();
            }
        } অন্যথা {
            ধ্রুবক unsigned delayCycles = getConstUInt(delayp->lhsp());
            currentp = addDelayChain(currentp, delayCycles, flp);
        }

        // Multi-cycle RHS: recurse (only plain boolean is returned as finalCondp).
        যদি (exprp->isMultiCycleSva()) {
            ধ্রুবক Build_ফলাফল ফলাফল = build_রাশিমালা(exprp, currentp, is_শীর্ষ_Level_Step);
            যদি (ফলাফল.valid()) addFiniteRangeReject(rangeRejectInfo, ফলাফল, flp);
            ফেরত ফলাফল;
        }
        ফেরত {currentp, exprp, std::move(rangeMidSources)};
    }

    Build_ফলাফল buildConsRep(AstSConsRep* repp, SvaStateVertex* entryVtxp,
                             যৌক্তিক is_শীর্ষ_Level_Step = মিথ্যা) {
        File_সারি* ধ্রুবক flp = repp->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp = repp->exprp();
        // Multi-cycle expr in ConsRep not yet supported; bail to avoid invalid AST.
        যদি (exprp->isMultiCycleSva()) {
            repp->v3warn(E_UNSUPPORTED, "Unsupported: multi-cycle sequence expression inside"
                                        " consecutive repetition (IEEE 1800-2023 16.9.2)");
            ফেরত Build_ফলাফল::failWithError();
        }
        ধ্রুবক unsigned minN = getConstUInt(repp->countp());

        // Sum sites across prefix + unbounded/range tail so one hoist covers
        // every check edge of this repetition.
        unsigned যোগ_Sites = minN;
        যদি (repp->unbounded()) {
            যোগ_Sites += 1;
        } অন্যথা যদি (repp->maxCountp()) {
            যোগ_Sites += getConstUInt(repp->maxCountp()) - minN;
        }
        AstVar* ধ্রুবক hoistVarp = tryHoistSampled(exprp, flp, যোগ_Sites);

        // Cover-sequence (IEEE 1800-2023 16.14.3): collect each end-of-match
        // position so they all fire the action, not just the merged terminal.
        std::vector<SvaStateVertex*> consMidSources;

        SvaStateVertex* currentp = entryVtxp;
        জন্য (unsigned i = 0; i < minN; ++i) {
            // Keep the first repetition explicit, collapse all remaining checks into the ring.
            যদি (i == 1) {
                currentp = addDelayChain(currentp, minN - 1, flp);
                currentp->m_delayRingClearCondp
                    = নতুন AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                currentp->m_replayAbortReject = সত্য;
                যদি (is_শীর্ষ_Level_Step) {
                    currentp->m_throughoutConds.ঠেলো_back(
                        sampledRefOrClone(hoistVarp, exprp, flp));
                }
                i = minN - 1;
            }
            // Every repetition in the minimum prefix is required.
            SvaStateVertex* ধ্রুবক condVtxp = scopedCreateVertex();
            SvaTransEdge* ধ্রুবক linkp
                = guardedLink(currentp, condVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            // Only an outermost required step rejects the property. Its first check is explicit,
            // later required checks reject through the ring's throughout guard.
            যদি (is_শীর্ষ_Level_Step && i == 0) linkp->m_rejectOnFail = সত্য;
            currentp = condVtxp;
        }
        // After minN: currentp is the first valid end-of-match position for [*m:n].
        যদি (m_isCoverSeq && (repp->unbounded() || repp->maxCountp())) {
            consMidSources.ঠেলো_back(currentp);
        }

        যদি (repp->unbounded()) {
            যদি (minN == 0) {
                SvaStateVertex* ধ্রুবক waitVtxp = scopedCreateVertex();
                guardedEdge(currentp, waitVtxp, flp);
                SvaStateVertex* ধ্রুবক checkVtxp = scopedCreateVertex();
                guardedLink(waitVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedEdge(checkVtxp, waitVtxp, flp);
                guardedLink(currentp, checkVtxp, flp);
                currentp = checkVtxp;
            } অন্যথা {
                SvaStateVertex* ধ্রুবক loopBackVtxp = scopedCreateVertex();
                guardedEdge(currentp, loopBackVtxp, flp);
                SvaStateVertex* ধ্রুবক reCheckVtxp = scopedCreateVertex();
                guardedLink(loopBackVtxp, reCheckVtxp, sampledRefOrClone(hoistVarp, exprp, flp),
                            flp);
                guardedEdge(reCheckVtxp, loopBackVtxp, flp);
                guardedLink(reCheckVtxp, currentp, flp);
            }
            currentp->m_isUnbounded = সত্য;
            m_inUnboundedScope = সত্য;
        } অন্যথা যদি (repp->maxCountp()) {
            ধ্রুবক unsigned maxN = getConstUInt(repp->maxCountp());
            SvaStateVertex* ধ্রুবক mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            unsigned লেজ_Min_N = minN;
            SvaStateVertex* লেজ_Startp = currentp;
            যদি (minN == 0) {
                // Build the first optional iteration explicitly. Feeding the empty endpoint
                // directly into a range ring would incorrectly keep that match alive.
                SvaStateVertex* ধ্রুবক পরবর্তী_Vtxp = scopedCreateVertex();
                guardedEdge(currentp, পরবর্তী_Vtxp, flp);
                SvaStateVertex* ধ্রুবক checkVtxp = scopedCreateVertex();
                guardedLink(পরবর্তী_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                যদি (m_isCoverSeq) consMidSources.ঠেলো_back(checkVtxp);
                লেজ_Startp = checkVtxp;
                লেজ_Min_N = 1;
            }
            যদি (maxN > লেজ_Min_N) {
                // Add tail-ring only if the tail is non-empty.
                SvaStateVertex* ধ্রুবক পরবর্তী_Vtxp
                    = addDelayChain(লেজ_Startp, maxN - লেজ_Min_N + 1, flp, মিথ্যা);
                পরবর্তী_Vtxp->m_delayRingClearCondp
                    = নতুন AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                পরবর্তী_Vtxp->m_replayAbortReject = সত্য;
                SvaStateVertex* ধ্রুবক checkVtxp = scopedCreateVertex();
                guardedLink(পরবর্তী_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                যদি (m_isCoverSeq) consMidSources.ঠেলো_back(checkVtxp);
            }
            currentp = mergeVtxp;
        }
        // finalCond = nullptr (already checked via Links)
        Build_ফলাফল res;
        res.termVertexp = currentp;
        res.finalCondp = nullptr;
        res.midSources = std::move(consMidSources);
        // mergeVtxp is the OR of all the end-positions we already pushed to
        // midSources, so the main termVtxp -> matchVertex Link would duplicate.
        res.termIsMidMerge = m_isCoverSeq && !res.midSources.empty();
        ফেরত res;
    }

    // always[lo:hi] / s_always[lo:hi] (IEEE 1800-2023 16.12.11).
    Build_ফলাফল buildPropAlways(AstPropAlways* nodep, SvaStateVertex* entryVtxp,
                                যৌক্তিক is_শীর্ষ_Level_Step = মিথ্যা) {
        File_সারি* ধ্রুবক flp = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক propp = nodep->propp();
        ধ্রুবক unsigned lo = getConstUInt(nodep->loBoundp());
        যদি (VN_IS(nodep->hiBoundp(), Unbounded)) {
            // Weak always [lo:$]: unbounded upper bound (IEEE 1800-2023 16.12.11).
            // p must hold at every clock tick at least lo cycles after the attempt
            // start; those ticks are not required to exist, so there is no
            // end-of-trace obligation (weak). The self-loop keeps the attempt live
            // every cycle; each observed cycle is a safety obligation, so a false p
            // rejects immediately.
            UASSERT_OBJ(!nodep->isStrong(), nodep, "Unbounded always must be weak (V3Width)");
            SvaStateVertex* ধ্রুবক livep = addDelayChain(entryVtxp, lo, flp);
            livep->m_isUnbounded = সত্য;
            guardedEdge(livep, livep, flp);  // stay active every subsequent cycle
            SvaStateVertex* ধ্রুবক sinkp = m_graph.createStateVertex();
            sinkp->m_isRejectSink = সত্য;
            SvaTransEdge* ধ্রুবক rejEdgep
                = guardedLink(livep, sinkp, sampled(propp->clone_বৃক্ষ_Pure(মিথ্যা)), flp);
            যদি (is_শীর্ষ_Level_Step) rejEdgep->m_rejectOnFail = সত্য;
            ফেরত {livep, nullptr, {}};
        }
        ধ্রুবক unsigned hi = getConstUInt(nodep->hiBoundp());
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
        SvaStateVertex* ধ্রুবক checkp = scopedCreateVertex();
        SvaTransEdge* ধ্রুবক linkp
            = guardedLink(currentp, checkp, sampled(propp->clone_বৃক্ষ_Pure(মিথ্যা)), flp);
        যদি (is_শীর্ষ_Level_Step) linkp->m_rejectOnFail = সত্য;
        currentp = checkp;
        m_temporal_Guard_স্তূপ.ঠেলো_back(propp);
        currentp = addDelayChain(currentp, hi - lo, flp);
        যদি (nodep->isStrong() && currentp->m_delay_Ring_আকার) currentp->m_strongAlwaysRing = সত্য;
        m_temporal_Guard_স্তূপ.বের_করো_back();
        ফেরত {currentp, propp, {}};
    }

    Build_ফলাফল buildGotoRep(AstSGotoRep* repp, SvaStateVertex* entryVtxp) {
        File_সারি* ধ্রুবক flp = repp->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp = repp->exprp();
        ধ্রুবক unsigned minN = getConstUInt(repp->countp());
        যদি (minN == 0) ফেরত Build_ফলাফল::fail();
        ধ্রুবক যৌক্তিক hasMax = repp->maxCountp() != nullptr;
        ধ্রুবক unsigned maxN = hasMax ? getConstUInt(repp->maxCountp()) : minN;
        যদি (m_isCoverSeq) {
            // Several attempts may coalesce in the one-bit wait vertex before reaching the ring,
            // so a cover sequence action block could run too few times.
            warnEndpointUnsupported(flp, "a goto repetition");
            ফেরত Build_ফলাফল::failWithError();
        }

        AstVar* ধ্রুবক hoistVarp = tryHoistSampled(exprp, flp, 2);

        // The first guardedEdge is the ##1 before waiting for a match. In the wait state, false
        // takes the clocked self-loop, while true takes the zero-delay guardedLink on that tick.
        SvaStateVertex* ধ্রুবক waitVtxp = scopedCreateVertex();
        guardedEdge(entryVtxp, waitVtxp, flp);
        guardedEdge(waitVtxp, waitVtxp,
                    নতুন AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)}, flp);
        SvaStateVertex* currentp = scopedCreateVertex();
        guardedLink(waitVtxp, currentp, sampledRefOrClone(hoistVarp, exprp, flp), flp);

        যদি (minN > 1) {
            SvaStateVertex* ধ্রুবক ringVtxp = addDelayChain(
                currentp, minN - 1, flp, সত্য, nullptr, sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = সত্য;
            SvaStateVertex* ধ্রুবক checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            currentp = checkVtxp;
        }
        যদি (!hasMax) {
            currentp->m_isUnbounded = সত্য;  // [->N] waits unboundedly
            m_inUnboundedScope = সত্য;
            ফেরত {currentp, nullptr, {}};
        }

        // [->M:N]: the range ring holds matches from M through N and advances
        // only on expr, preserving arbitrarily long gaps between occurrences.
        SvaStateVertex* ধ্রুবক mergeVtxp = scopedCreateVertex();
        guardedLink(currentp, mergeVtxp, flp);  // accept at match_M
        যদি (maxN > minN) {
            SvaStateVertex* ধ্রুবক ringVtxp
                = addDelayChain(currentp, maxN - minN + 1, flp, মিথ্যা, nullptr,
                                sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = সত্য;
            SvaStateVertex* ধ্রুবক checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            guardedLink(checkVtxp, mergeVtxp, flp);
        }
        mergeVtxp->m_isUnbounded = সত্য;  // [->M:N] still has unbounded waits between matches
        m_inUnboundedScope = সত্য;
        ফেরত {mergeVtxp, nullptr, {}};
    }

    // True if the sequence has a zero-minimum consecutive repetition, which may match empty
    স্থির যৌক্তিক hasZeroMinRep(ধ্রুবক Ast_গ্রন্থি_রাশিমালা* seqp) {
        ফেরত seqp->exists(
            [](ধ্রুবক AstSConsRep* repp) { ফেরত getConstUInt(repp->countp()) == 0; });
    }

    // True if a leaf's condition has no implication or multi-cycle sequence in it
    // (a leaf can be e.g. 'not (x |-> y)')
    স্থির যৌক্তিক isPlainBoolean(ধ্রুবক Ast_গ্রন্থি_রাশিমালা* condp) {
        ফেরত !condp->exists(
            [](ধ্রুবক Ast_গ্রন্থি_রাশিমালা* np) { ফেরত np->isMultiCycleSva() || VN_IS(np, Implication); });
    }

    // Build merge vertex for SOr / LogOr: both branches feed into one vertex.
    Build_ফলাফল buildOrMerge(Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* rhsp, SvaStateVertex* entryVtxp,
                             File_সারি* flp, যৌক্তিক wantBoolLeaf) {
        ধ্রুবক Build_ফলাফল lhs = build_রাশিমালা(lhsp, entryVtxp);
        ধ্রুবক Build_ফলাফল rhs = build_রাশিমালা(rhsp, entryVtxp);
        যদি (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ফেরত Build_ফলাফল::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP
        // IEEE 1800-2023 16.14.3: a cover sequence counts every end-of-match. A
        // sequence operand of 'or' can end more than once, but only its final
        // end reaches the merge vertex below, so reject sequence operands rather
        // than under-count. Plain boolean disjunction has one end per cycle and
        // is handled by the OR-fold.
        ধ্রুবক যৌক্তিক booleanOnly = lhs.termVertexp == entryVtxp && lhs.finalCondp
                                 && rhs.termVertexp == entryVtxp && rhs.finalCondp;
        যদি (m_isCoverSeq && !booleanOnly) {
            warnEndpointUnsupported(flp, "a sequence operand of 'or'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ফেরত Build_ফলাফল::failWithError();
        }
        যদি (booleanOnly && wantBoolLeaf) {
            যদি (isPlainBoolean(lhs.finalCondp) && isPlainBoolean(rhs.finalCondp)) {
                // As a |-> / |=> or top-level body, a disjunction of two plain booleans must
                // be able to reject: return it as a boolean finalCondp. The merge vertex
                // below drops the attempt silently when both operands are false.
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক orCondp
                    = নতুন AstLogOr{flp, lhs.finalCondp->clone_বৃক্ষ_Pure(মিথ্যা),
                                   rhs.finalCondp->clone_বৃক্ষ_Pure(মিথ্যা)};
                freeUnlinkedCondp(lhs.finalCondp);
                freeUnlinkedCondp(rhs.finalCondp);
                ফেরত {entryVtxp, orCondp, {}};
            }
        }
        SvaStateVertex* ধ্রুবক mergeVtxp = scopedCreateVertex();
        যদি (booleanOnly) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsp,
                        "Single-cycle SOr operands must have finalCondp");
            guardedLink(entryVtxp, mergeVtxp,
                        নতুন AstLogOr{flp, sampled(lhs.finalCondp->clone_বৃক্ষ_Pure(মিথ্যা)),
                                     sampled(rhs.finalCondp->clone_বৃক্ষ_Pure(মিথ্যা))},
                        flp);
        } অন্যথা {
            যদি (lhs.finalCondp) {
                guardedLink(lhs.termVertexp, mergeVtxp,
                            sampled(lhs.finalCondp->clone_বৃক্ষ_Pure(মিথ্যা)), flp);
            } অন্যথা {
                guardedLink(lhs.termVertexp, mergeVtxp, flp);
            }
            যদি (rhs.finalCondp) {
                guardedLink(rhs.termVertexp, mergeVtxp,
                            sampled(rhs.finalCondp->clone_বৃক্ষ_Pure(মিথ্যা)), flp);
            } অন্যথা {
                guardedLink(rhs.termVertexp, mergeVtxp, flp);
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        ফেরত {mergeVtxp, nullptr, {}};
    }

    // Build done-latch combiner for SAnd/SIntersect (IEEE 1800-2023 16.9.5).
    Build_ফলাফল buildAndCombiner(Ast_গ্রন্থি_রাশিমালা* lhsExprp, Ast_গ্রন্থি_রাশিমালা* rhsExprp,
                                 SvaStateVertex* entryVtxp, File_সারি* flp) {
        // Snapshot-restore scope so LHS liveness does not leak into RHS.
        ধ্রুবক যৌক্তিক savedScope = m_inUnboundedScope;
        ধ্রুবক Build_ফলাফল lhs = build_রাশিমালা(lhsExprp, entryVtxp);
        ধ্রুবক যৌক্তিক lhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope;
        ধ্রুবক Build_ফলাফল rhs = build_রাশিমালা(rhsExprp, entryVtxp);
        ধ্রুবক যৌক্তিক rhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope || lhsScope || rhsScope;
        যদি (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ফেরত Build_ফলাফল::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP

        // Single-cycle operands: use boolean AND (done-latch would fire across cycles).
        // If both operands stayed at entry, they must be boolean leaves which
        // buildExpr returns with finalCondp=nodep (non-null).
        যদি (lhs.termVertexp == entryVtxp && rhs.termVertexp == entryVtxp) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsExprp,
                        "Single-cycle SAnd operands must have finalCondp");
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক condp = নতুন AstLogAnd{flp, lhs.finalCondp->clone_বৃক্ষ_Pure(মিথ্যা),
                                                     rhs.finalCondp->clone_বৃক্ষ_Pure(মিথ্যা)};
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ফেরত {entryVtxp, condp, {}};
        }
        // Range-delay mid-window sources in either sub-branch would need
        // to be folded into the latch's match-now signal, which the
        // current combiner does not support.
        যদি (!lhs.midSources.empty() || !rhs.midSources.empty()) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: ranged cycle delay in an operand of property 'and'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ফেরত Build_ফলাফল::failWithError();
        }
        SvaStateVertex* ধ্রুবক combVtxp = scopedCreateVertex();
        combVtxp->m_isAndCombiner = সত্য;
        combVtxp->m_andLhsTermp = lhs.termVertexp;
        combVtxp->m_andRhsTermp = rhs.termVertexp;
        যদি (lhs.finalCondp) combVtxp->m_andLhsCondp = lhs.finalCondp->clone_বৃক্ষ_Pure(মিথ্যা);
        যদি (rhs.finalCondp) combVtxp->m_andRhsCondp = rhs.finalCondp->clone_বৃক্ষ_Pure(মিথ্যা);
        যদি (lhs.termVertexp->m_isUnbounded || rhs.termVertexp->m_isUnbounded) {
            combVtxp->m_isUnbounded = সত্য;
        }
        // Wire terminal-boolean rejects to a dedicated sink so each side can fail
        // the AND independently. Skip for liveness or single-cycle operands
        // (single-cycle termVertexp == entryVtxp would fire every cycle).
        যদি (!combVtxp->m_isUnbounded) {
            যৌক্তিক needSink = মিথ্যা;
            ধ্রুবক যৌক্তিক lhsMultiCycle = (lhs.termVertexp != entryVtxp);
            ধ্রুবক যৌক্তিক rhsMultiCycle = (rhs.termVertexp != entryVtxp);
            যদি (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                needSink = সত্য;
            }
            যদি (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                needSink = সত্য;
            }
            যদি (needSink) {
                SvaStateVertex* ধ্রুবক sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = সত্য;
                যদি (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* ধ্রুবক ep = m_graph.addLink(
                        lhs.termVertexp, sinkVtxp, sampled(lhs.finalCondp->clone_বৃক্ষ_Pure(মিথ্যা)));
                    ep->m_rejectOnFail = সত্য;
                }
                যদি (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* ধ্রুবক ep = m_graph.addLink(
                        rhs.termVertexp, sinkVtxp, sampled(rhs.finalCondp->clone_বৃক্ষ_Pure(মিথ্যা)));
                    ep->m_rejectOnFail = সত্য;
                }
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        ফেরত {combVtxp, nullptr, {}};
    }

    // Lower `seq1 within seq2` (IEEE 1800-2023 16.9.10) as:
    //   (OR_{i in 0..slack} 1 ##i seq1 ##(slack-i) 1) intersect seq2
    // Both operands must have fixed length (no ranged cycle delays).
    // The OR lives inside a single SIntersect so one AndCombiner done-latch
    // serves all offsets; lifting it out would double-count matches where
    // multiple offsets accept on the same seq2 end cycle.
    Build_ফলাফল buildSWithin(AstSWithin* nodep, SvaStateVertex* entryVtxp,
                             যৌক্তিক is_শীর্ষ_Level_Step = মিথ্যা) {
        ধ্রুবক পূর্ণসংখ্যা innerLen = fixed_দৈর্ঘ্য(nodep->lhsp());
        ধ্রুবক পূর্ণসংখ্যা outerLen = fixed_দৈর্ঘ্য(nodep->rhsp());
        যদি (innerLen < 0 || outerLen < 0) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: within with ranged cycle-delay operand");
            ফেরত Build_ফলাফল::failWithError();
        }
        যদি (innerLen > outerLen) {
            ফেরত buildNeverMatchIntersect(
                nodep, entryVtxp, is_শীর্ষ_Level_Step,
                "the inner sequence is longer than the outer sequence");
        }
        File_সারি* ধ্রুবক flp = nodep->fileline();
        ধ্রুবক পূর্ণসংখ্যা slack = outerLen - innerLen;
        Ast_গ্রন্থি_রাশিমালা* innerOrp = nullptr;
        জন্য (পূর্ণসংখ্যা i = 0; i <= slack; ++i) {
            ধ্রুবক পূর্ণসংখ্যা postPad = slack - i;
            Ast_গ্রন্থি_রাশিমালা* branchp = nodep->lhsp()->clone_বৃক্ষ_Pure(মিথ্যা);
            যদি (i > 0) {
                AstConst* ধ্রুবক prePadp = নতুন AstConst{flp, AstConst::BitTrue{}};
                AstDelay* ধ্রুবক delayp
                    = নতুন AstDelay{flp, নতুন AstConst{flp, static_cast<uint32_t>(i)}, সত্য};
                AstSExpr* ধ্রুবক wrapped = নতুন AstSExpr{flp, prePadp, delayp, branchp};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            যদি (postPad > 0) {
                AstConst* ধ্রুবক postTruep = নতুন AstConst{flp, AstConst::BitTrue{}};
                AstDelay* ধ্রুবক delayp
                    = নতুন AstDelay{flp, নতুন AstConst{flp, static_cast<uint32_t>(postPad)}, সত্য};
                AstSExpr* ধ্রুবক wrapped = নতুন AstSExpr{flp, branchp, delayp, postTruep};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            innerOrp = innerOrp ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নতুন AstSOr{flp, innerOrp, branchp})
                                : branchp;
        }
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক outerClonep = nodep->rhsp()->clone_বৃক্ষ_Pure(মিথ্যা);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক combinedp = নতুন AstSIntersect{flp, innerOrp, outerClonep};
        Build_ফলাফল ফলাফল = build_রাশিমালা(combinedp, entryVtxp, is_শীর্ষ_Level_Step);
        VL_DO_DANGLING(combinedp->delete_বৃক্ষ(), combinedp);
        // When both operands are plain booleans, buildAndCombiner returns a
        // freshly-allocated AstAnd as finalCondp with no parent. Callers up
        // the chain clone-and-discard finalCondp, so leave it parent-less and
        // it leaks; anchor it in the graph now via an edge.
        যদি (ফলাফল.valid() && ফলাফল.finalCondp && !ফলাফল.finalCondp->backp()) {
            SvaStateVertex* ধ্রুবক wrapVtxp = scopedCreateVertex();
            guardedLink(ফলাফল.termVertexp, wrapVtxp, sampled(ফলাফল.finalCondp), flp);
            ফলাফল = {wrapVtxp, nullptr, ফলাফল.midSources};
        }
        ফেরত ফলাফল;
    }

    // Collect the boolean leaf checks of a fixed-length sequence keyed by their
    // clock offset from the start. Returns false for anything other than nested
    // AstSExpr with fixed cycle delays over boolean leaves (e.g. throughout).
    স্থির যৌক্তিক flattenFixedSeq(Ast_গ্রন্থি_রাশিমালা* nodep, পূর্ণসংখ্যা baseOffset,
                                std::map<পূর্ণসংখ্যা, std::vector<Ast_গ্রন্থি_রাশিমালা*>>& out) {
        যদি (AstSExpr* ধ্রুবক sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ধ্রুবক delayp = VN_CAST(sexprp->delayp(), Delay);
            যদি (!delayp || !delayp->isCycleDelay() || delayp->isUnbounded()) ফেরত মিথ্যা;
            ধ্রুবক unsigned delayCycles = getConstUInt(delayp->lhsp());
            যদি (delayp->isRangeDelay() && getConstUInt(delayp->rhsp()) != delayCycles)
                ফেরত মিথ্যা;
            পূর্ণসংখ্যা preLen = 0;
            যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক prep = sexprp->preExprp()) {
                যদি (!flattenFixedSeq(prep, baseOffset, out)) ফেরত মিথ্যা;
                preLen = fixed_দৈর্ঘ্য(prep);
                যদি (preLen < 0) ফেরত মিথ্যা;
            }
            ফেরত flattenFixedSeq(sexprp->exprp(), baseOffset + preLen + delayCycles, out);
        }
        যদি (nodep->isMultiCycleSva()) ফেরত মিথ্যা;
        out[baseOffset].ঠেলো_back(nodep);
        ফেরত সত্য;
    }

    // Conjoin two equal-length fixed sequences into one: at each clock offset
    // AND the boolean checks of both operands (IEEE 1800-2023 16.9.6 -- both
    // operands match the same window). Returns null if either operand is not a
    // plain fixed sequence of boolean leaves.
    স্থির Ast_গ্রন্থি_রাশিমালা* conjoinFixedSeqs(Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* rhsp, File_সারি* flp) {
        std::map<পূর্ণসংখ্যা, std::vector<Ast_গ্রন্থি_রাশিমালা*>> checks;
        যদি (!flattenFixedSeq(lhsp, 0, checks) || !flattenFixedSeq(rhsp, 0, checks)) ফেরত nullptr;
        যদি (checks.empty()) ফেরত nullptr;
        Ast_গ্রন্থি_রাশিমালা* resultp = nullptr;
        পূর্ণসংখ্যা পূর্ব_Offset = 0;
        জন্য (ধ্রুবক auto& offsetChecks : checks) {
            ধ্রুবক পূর্ণসংখ্যা offset = offsetChecks.first;
            Ast_গ্রন্থি_রাশিমালা* condp = nullptr;
            জন্য (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক leafp : offsetChecks.second) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক clonep = leafp->clone_বৃক্ষ_Pure(মিথ্যা);
                যদি (!condp) {
                    condp = clonep;
                } অন্যথা {
                    condp = নতুন AstLogAnd{flp, condp, clonep};
                    condp->dtypeSetBit();
                }
            }
            যদি (!resultp) {
                যদি (offset > 0) {
                    AstDelay* ধ্রুবক delayp = নতুন AstDelay{
                        flp, নতুন AstConst{flp, static_cast<uint32_t>(offset)}, /*isCycle=*/সত্য};
                    resultp
                        = নতুন AstSExpr{flp, নতুন AstConst{flp, AstConst::BitTrue{}}, delayp, condp};
                    resultp->dtypeSetBit();
                } অন্যথা {
                    resultp = condp;
                }
            } অন্যথা {
                AstDelay* ধ্রুবক delayp = নতুন AstDelay{
                    flp, নতুন AstConst{flp, static_cast<uint32_t>(offset - পূর্ব_Offset)},
                    /*isCycle=*/সত্য};
                resultp = নতুন AstSExpr{flp, resultp, delayp, condp};
                resultp->dtypeSetBit();
            }
            পূর্ব_Offset = offset;
        }
        ফেরত resultp;
    }

    // `seq` is a simple ranged sequence `start ##[m:n] end` (start/end boolean,
    // start may be absent). Used to collapse a both-variable intersect to one
    // ranged delay.
    কাঠামো SimpleRanged final {
        যৌক্তিক ok = মিথ্যা;
        Ast_গ্রন্থি_রাশিমালা* startp = nullptr;  // may be null (absent start)
        Ast_গ্রন্থি_রাশিমালা* endp = nullptr;
    };
    স্থির SimpleRanged asSimpleRanged(Ast_গ্রন্থি_রাশিমালা* nodep) {
        AstSExpr* ধ্রুবক sexprp = VN_CAST(nodep, SExpr);
        যদি (!sexprp) ফেরত {};
        AstDelay* ধ্রুবক delayp = VN_CAST(sexprp->delayp(), Delay);
        যদি (!delayp || !delayp->isCycleDelay() || !delayp->isRangeDelay() || delayp->isUnbounded())
            ফেরত {};
        যদি (getConstUInt(delayp->lhsp()) == getConstUInt(delayp->rhsp())) ফেরত {};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক prep = sexprp->preExprp();
        যদি (prep && fixed_দৈর্ঘ্য(prep) != 0) ফেরত {};
        যদি (fixed_দৈর্ঘ্য(sexprp->exprp()) != 0) ফেরত {};
        ফেরত {সত্য, prep, sexprp->exprp()};
    }

    // Build the NFA for a synthesized intersect lowering tree, then free it.
    // buildExpr returns the terminal condition (finalCondp) by reference into the
    // tree; detach a clone so the tree can be freed here. The graph already holds
    // clones/hoists of every edge condition, so nothing else dangles.
    Build_ফলাফল build_From_Lowering_বৃক্ষ(Ast_গ্রন্থি_রাশিমালা* treep, SvaStateVertex* entryVtxp,
                                      যৌক্তিক is_শীর্ষ_Level_Step) {
        Build_ফলাফল ফলাফল = build_রাশিমালা(treep, entryVtxp, is_শীর্ষ_Level_Step);
        যদি (ফলাফল.valid() && ফলাফল.finalCondp) {
            ফলাফল.finalCondp = ফলাফল.finalCondp->clone_বৃক্ষ_Pure(মিথ্যা);
        }
        VL_DO_DANGLING(treep->delete_বৃক্ষ(), treep);
        ফেরত ফলাফল;
    }

    // Empty common-length intersection -- unequal fixed lengths, or disjoint
    // ranged lengths. IEEE 1800-2023 16.9.6 requires both operands to match
    // over a window of the same length, so with no common length the intersect
    // simply never matches. This is legal (matching nothing), not an error, so
    // lower to a constant false rather than rejecting legal code.
    Build_ফলাফল buildNeverMatchIntersect(Ast_গ্রন্থি_রাশিমালা* nodep, SvaStateVertex* entryVtxp,
                                         যৌক্তিক is_শীর্ষ_Level_Step, ধ্রুবক অক্ষর* reason) {
        nodep->v3warn(NEVERMATCH, "Sequence can never match because " << reason << ".");
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক falsep = নতুন AstConst{nodep->fileline(), AstConst::BitFalse{}};
        ফেরত build_From_Lowering_বৃক্ষ(falsep, entryVtxp, is_শীর্ষ_Level_Step);
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
    Build_ফলাফল buildVarLenIntersect(AstSIntersect* nodep, SvaStateVertex* entryVtxp,
                                     যৌক্তিক is_শীর্ষ_Level_Step) {
        ধ্রুবক std::pair<পূর্ণসংখ্যা, পূর্ণসংখ্যা> lhsRange = দৈর্ঘ্য_Range(nodep->lhsp());
        ধ্রুবক std::pair<পূর্ণসংখ্যা, পূর্ণসংখ্যা> rhsRange = দৈর্ঘ্য_Range(nodep->rhsp());
        যদি (lhsRange.first < 0 || rhsRange.first < 0) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect with this variable-length operand");
            ফেরত Build_ফলাফল::failWithError();
        }
        ধ্রুবক পূর্ণসংখ্যা lo = std::max(lhsRange.first, rhsRange.first);
        ধ্রুবক পূর্ণসংখ্যা hi = std::min(lhsRange.second, rhsRange.second);
        যদি (lo > hi) {
            // Disjoint length ranges share no common length -> never matches.
            ফেরত buildNeverMatchIntersect(nodep, entryVtxp, is_শীর্ষ_Level_Step,
                                            "intersect operands have no common length");
        }
        File_সারি* ধ্রুবক flp = nodep->fileline();
        যদি (lo == hi) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lp = realize_At_দৈর্ঘ্য(nodep->lhsp(), lo, lhsRange.first);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rp = realize_At_দৈর্ঘ্য(nodep->rhsp(), lo, rhsRange.first);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক conjp = conjoinFixedSeqs(lp, rp, flp);
            VL_DO_DANGLING(lp->delete_বৃক্ষ(), lp);
            VL_DO_DANGLING(rp->delete_বৃক্ষ(), rp);
            যদি (!conjp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: intersect operand is not a plain boolean sequence");
                ফেরত Build_ফলাফল::failWithError();
            }
            ফেরত build_From_Lowering_বৃক্ষ(conjp, entryVtxp, is_শীর্ষ_Level_Step);
        }
        ধ্রুবক SimpleRanged sl = asSimpleRanged(nodep->lhsp());
        ধ্রুবক SimpleRanged sr = asSimpleRanged(nodep->rhsp());
        যদি (!sl.ok || !sr.ok) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect of two sequences that each vary in length over a"
                          " range with internal structure");
            ফেরত Build_ফলাফল::failWithError();
        }
        ধ্রুবক auto andBool = [&](Ast_গ্রন্থি_রাশিমালা* ap, Ast_গ্রন্থি_রাশিমালা* bp) -> Ast_গ্রন্থি_রাশিমালা* {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক aClonep
                = ap ? ap->clone_বৃক্ষ_Pure(মিথ্যা) : নতুন AstConst{flp, AstConst::BitTrue{}};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক bClonep
                = bp ? bp->clone_বৃক্ষ_Pure(মিথ্যা) : নতুন AstConst{flp, AstConst::BitTrue{}};
            AstLogAnd* ধ্রুবক andp = নতুন AstLogAnd{flp, aClonep, bClonep};
            andp->dtypeSetBit();
            ফেরত andp;
        };
        AstDelay* ধ্রুবক delayp = নতুন AstDelay{flp, নতুন AstConst{flp, static_cast<uint32_t>(lo)},
                                              /*isCycle=*/সত্য};
        delayp->rhsp(নতুন AstConst{flp, static_cast<uint32_t>(hi)});
        AstSExpr* ধ্রুবক reducedp
            = নতুন AstSExpr{flp, andBool(sl.startp, sr.startp), delayp, andBool(sl.endp, sr.endp)};
        reducedp->dtypeSetBit();
        ফেরত build_From_Lowering_বৃক্ষ(reducedp, entryVtxp, is_শীর্ষ_Level_Step);
    }

    Build_ফলাফল buildThroughout(AstSThroughout* nodep, SvaStateVertex* entryVtxp,
                                যৌক্তিক is_শীর্ষ_Level_Step = মিথ্যা) {
        // Mark entryVtxp so "cond false at tick 0" is detected as throughout-drop.
        entryVtxp->m_throughoutConds.ঠেলো_back(nodep->lhsp()->clone_বৃক্ষ_Pure(মিথ্যা));
        m_temporal_Guard_স্তূপ.ঠেলো_back(nodep->lhsp());
        ধ্রুবক Build_ফলাফল ফলাফল = build_রাশিমালা(nodep->rhsp(), entryVtxp, is_শীর্ষ_Level_Step);
        m_temporal_Guard_স্তূপ.বের_করো_back();
        ফেরত ফলাফল;
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
    Build_ফলাফল buildUntil(AstUntil* nodep, SvaStateVertex* entryVtxp, যৌক্তিক is_শীর্ষ_Level_Step) {
        File_সারি* ধ্রুবক flp = nodep->fileline();
        যদি (!is_শীর্ষ_Level_Step) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            ফেরত Build_ফলাফল::failWithError();
        }
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = nodep->lhsp();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsp = nodep->rhsp();
        ধ্রুবক auto hasSeq = [](ধ্রুবক Ast_গ্রন্থি_রাশিমালা* ep) {
            ফেরত ep->exists([](ধ্রুবক Ast_গ্রন্থি_রাশিমালা* np) { ফেরত np->isMultiCycleSva(); });
        };
        যদি (hasSeq(lhsp) || hasSeq(rhsp)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            ফেরত Build_ফলাফল::failWithError();
        }

        ধ্রুবক যৌক্তিক ov = nodep->isOverlapping();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsBitp = nodep->lhsp();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsBitp = nodep->rhsp();
        // p hoist count: continue, require (ov: 1 use; nov: 1 use). At least 2 uses.
        AstVar* ধ্রুবক pHoistp = tryHoistSampled(lhsBitp, flp, 2);
        // q hoist count: continue (1) + require nov (1) = 2; ov: continue only (1).
        AstVar* ধ্রুবক qHoistp = ov ? nullptr : tryHoistSampled(rhsBitp, flp, 2);

        SvaStateVertex* ধ্রুবক waitCp = scopedCreateVertex();
        SvaStateVertex* ধ্রুবক waitRp = scopedCreateVertex();
        waitCp->m_isUnbounded = সত্য;
        waitRp->m_strongPending = nodep->isStrong() && !m_isCover;

        // Entry and back-loop Links carry no condition; throughout-folding still applies.
        guardedLink(entryVtxp, waitCp, flp);
        guardedLink(waitRp, waitCp, flp);

        // Continue clocked edge: p && !q advances to next-cycle wait.
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক contCondp
            = নতুন AstLogAnd{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                            নতুন AstLogNot{flp, sampledRefOrClone(qHoistp, rhsBitp, flp)}};
        guardedEdge(waitCp, waitRp, contCondp, flp);

        // Reject sink: fires when require-condition is false.
        SvaStateVertex* ধ্রুবক sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = সত্য;
        Ast_গ্রন্থি_রাশিমালা* requireCondp;
        যদি (ov) {
            requireCondp = sampledRefOrClone(pHoistp, lhsBitp, flp);
        } অন্যথা {
            requireCondp = নতুন AstLogOr{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                                        sampledRefOrClone(qHoistp, rhsBitp, flp)};
        }
        SvaTransEdge* ধ্রুবক rejEdgep = m_graph.addLink(waitCp, sinkVtxp, requireCondp);
        rejEdgep->m_rejectOnFail = সত্য;

        // Accept condition rides via finalCondp; assembleResult $sampled-wraps it.
        Ast_গ্রন্থি_রাশিমালা* acceptCondp;
        যদি (ov) {
            acceptCondp
                = নতুন AstLogAnd{flp, lhsBitp->clone_বৃক্ষ_Pure(মিথ্যা), rhsBitp->clone_বৃক্ষ_Pure(মিথ্যা)};
        } অন্যথা {
            acceptCondp = rhsBitp->clone_বৃক্ষ_Pure(মিথ্যা);
        }
        ফেরত {waitCp, acceptCondp, {}};
    }

    // IEEE 1800-2023 16.12.14 property abort operators. Sync and async share
    // the same NFA encoding: AstSampled already gives matured values at every
    // maturing clocking event, and async firing "between clocks" is not
    // observable in a cycle-based model. VAbortKind selects accept vs reject
    // verdict (sync vs async only changes the user-visible spelling).

    // Build `condp && !outer_1 && !outer_2 ...` (unsampled).
    Ast_গ্রন্থি_রাশিমালা* abort_Fire_রাশিমালা(Ast_গ্রন্থি_রাশিমালা* condp, File_সারি* flp) {
        Ast_গ্রন্থি_রাশিমালা* resultp = condp->clone_বৃক্ষ_Pure(মিথ্যা);
        জন্য (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক op : m_outer_Abort_স্তূপ)
            resultp = নতুন AstLogAnd{flp, resultp, নতুন AstLogNot{flp, op->clone_বৃক্ষ_Pure(মিথ্যা)}};
        ফেরত resultp;
    }

    // True when unguarded ring-wide rejection or a same-tick Link chain already accounts the
    // attempt: a required-step Link covers both outcomes; followed-by pairs both edges.
    স্থির যৌক্তিক chainAccountsSource(ধ্রুবক SvaStateVertex* srcp,
                                    ধ্রুবক std::unordered_set<ধ্রুবক V3GraphEdge*>& preEdges) {
        যদি (srcp->m_delay_Ring_আকার && srcp->m_throughoutConds.empty()) ফেরত সত্য;
        জন্য (ধ্রুবক V3GraphEdge& edger : srcp->inEdges()) {
            যদি (preEdges.গুনতি(&edger)) চালু;
            ধ্রুবক SvaTransEdge& tedger = static_cast<ধ্রুবক SvaTransEdge&>(edger);
            যদি (tedger.m_consumesCycle) চালু;
            ধ্রুবক auto* ধ্রুবক fromp = static_cast<ধ্রুবক SvaStateVertex*>(tedger.fromVtxp());
            যদি (fromp->m_abortRejectp) ফেরত সত্য;
        }
        যৌক্তিক plainNonSink = মিথ্যা;
        যৌক্তিক markedSink = মিথ্যা;
        জন্য (ধ্রুবক V3GraphEdge& edger : srcp->outEdges()) {
            যদি (preEdges.গুনতি(&edger)) চালু;
            ধ্রুবক SvaTransEdge& tedger = static_cast<ধ্রুবক SvaTransEdge&>(edger);
            যদি (tedger.m_consumesCycle) চালু;
            ধ্রুবক যৌক্তিক sink = static_cast<ধ্রুবক SvaStateVertex*>(tedger.toVtxp())->m_isRejectSink;
            যদি (tedger.m_rejectOnFail) {
                যদি (!sink) ফেরত সত্য;
                markedSink = সত্য;
            } অন্যথা যদি (!sink) {
                যদি (srcp->m_abortRejectp) ফেরত সত্য;
                plainNonSink = সত্য;
            }
        }
        ফেরত plainNonSink && markedSink;
    }

    // Reject edge: fires when the source is live and the abort samples true.
    শূন্য addAbortRejectEdge(SvaStateVertex* srcp, SvaStateVertex* sinkp, Ast_গ্রন্থি_রাশিমালা* condp,
                            File_সারি* flp) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক notFirep = নতুন AstLogNot{flp, sampled(abort_Fire_রাশিমালা(condp, flp))};
        m_graph.addLink(srcp, sinkp, notFirep)->m_rejectOnFail = সত্য;
        ফেরত;
    }

    // On the fire tick: kill body threads; accept kinds also forgive step misses.
    শূন্য gateBodyEdgesOnAbort(ধ্রুবক std::unordered_set<ধ্রুবক V3GraphEdge*>& preEdges,
                              Ast_গ্রন্থি_রাশিমালা* condp, VAbortKind kind, File_সারি* flp) {
        জন্য (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            জন্য (V3GraphEdge& edger : vtxr.outEdges()) {
                যদি (preEdges.গুনতি(&edger)) চালু;
                SvaTransEdge* ধ্রুবক tedgep = static_cast<SvaTransEdge*>(&edger);
                যদি (tedgep->m_rejectOnFail) {
                    যদি (!kind.isAccept()) চালু;
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক firep = sampled(abort_Fire_রাশিমালা(condp, flp));
                    tedgep->m_condp
                        = tedgep->m_condp ? নতুন AstLogOr{flp, tedgep->m_condp, firep} : firep;
                } অন্যথা যদি (tedgep->m_consumesCycle) {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক notFirep
                        = নতুন AstLogNot{flp, sampled(abort_Fire_রাশিমালা(condp, flp))};
                    tedgep->m_condp = tedgep->m_condp
                                          ? নতুন AstLogAnd{flp, tedgep->m_condp, notFirep}
                                          : notFirep;
                }
            }
        }
    }

    Build_ফলাফল buildAbortOn(Ast_গ্রন্থি_রাশিমালা* condp, Ast_গ্রন্থি_রাশিমালা* bodyp, SvaStateVertex* entryVtxp,
                             VAbortKind kind, File_সারি* flp, যৌক্তিক is_শীর্ষ_Level_Step) {
        // Snapshot pre-body vertices/edges so post-build diff yields the body's sub-NFA.
        std::unordered_set<ধ্রুবক V3GraphVertex*> preExisting;
        std::unordered_set<ধ্রুবক V3GraphEdge*> preEdges;
        জন্য (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            preExisting.ঢোকাও(&vtxr);
            জন্য (V3GraphEdge& edger : vtxr.outEdges()) preEdges.ঢোকাও(&edger);
        }

        m_outer_Abort_স্তূপ.ঠেলো_back(condp);
        ধ্রুবক Build_ফলাফল body_ফলাফল = build_রাশিমালা(bodyp, entryVtxp, is_শীর্ষ_Level_Step);
        m_outer_Abort_স্তূপ.বের_করো_back();
        যদি (!body_ফলাফল.valid()) ফেরত body_ফলাফল;

        gateBodyEdgesOnAbort(preEdges, condp, kind, flp);

        // Live-thread sources for the abort edge: entry + new body vertices,
        // minus reject sinks (they carry reject fuel, not live-thread fuel).
        std::vector<SvaStateVertex*> abortSources;
        abortSources.ঠেলো_back(entryVtxp);
        জন্য (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            যদি (preExisting.গুনতি(&vtxr)) চালু;
            auto* ধ্রুবক sp = static_cast<SvaStateVertex*>(&vtxr);
            যদি (sp->m_delay_Ring_আকার || !sp->m_throughoutConds.empty()) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক firep = abort_Fire_রাশিমালা(condp, flp);
                sp->m_abortClearp
                    = sp->m_abortClearp ? নতুন AstLogOr{flp, sp->m_abortClearp, firep} : firep;
            }
            যদি (!kind.isAccept()
                && ((sp->m_delay_Ring_আকার && sp->m_throughoutConds.empty())
                    || sp->m_replayAbortReject)) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক firep = abort_Fire_রাশিমালা(condp, flp);
                sp->m_abortRejectp
                    = sp->m_abortRejectp ? নতুন AstLogOr{flp, sp->m_abortRejectp, firep} : firep;
            }
            যদি (sp->m_isRejectSink) চালু;
            abortSources.ঠেলো_back(sp);
        }

        auto sampledAbortFire = [&]() -> Ast_গ্রন্থি_রাশিমালা* {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক রাশিমালা = abort_Fire_রাশিমালা(condp, flp);
            ফেরত sampled(রাশিমালা);
        };

        যদি (kind.isAccept()) {
            // Match-only sink fed by $sampled(abort-fire) from every live source;
            // registered as midSource so it never contributes a reject.
            SvaStateVertex* ধ্রুবক acceptSinkp = scopedCreateVertex();
            জন্য (SvaStateVertex* ধ্রুবক srcp : abortSources)
                guardedLink(srcp, acceptSinkp, sampledAbortFire(), flp);
            std::vector<SvaStateVertex*> midSources = body_ফলাফল.midSources;
            midSources.ঠেলো_back(acceptSinkp);
            Ast_গ্রন্থি_রাশিমালা* finalCondp = body_ফলাফল.finalCondp;
            যদি (finalCondp) {
                যদি (finalCondp->backp()) finalCondp = finalCondp->clone_বৃক্ষ_Pure(মিথ্যা);
                finalCondp = নতুন AstLogOr{flp, finalCondp, abort_Fire_রাশিমালা(condp, flp)};
            }
            ফেরত {body_ফলাফল.termVertexp, finalCondp, std::move(midSources)};
        }

        // rejectOnFail treats m_condp as the success condition and fires on
        // !condp, so the edge carries !sampledAbortFire().
        SvaStateVertex* ধ্রুবক rejectSinkp = m_graph.createStateVertex();
        rejectSinkp->m_isRejectSink = সত্য;
        জন্য (SvaStateVertex* ধ্রুবক srcp : abortSources)
            যদি (!chainAccountsSource(srcp, preEdges))
                addAbortRejectEdge(srcp, rejectSinkp, condp, flp);
        Ast_গ্রন্থি_রাশিমালা* finalCondp = body_ফলাফল.finalCondp;
        যদি (finalCondp) {
            যদি (finalCondp->backp()) finalCondp = finalCondp->clone_বৃক্ষ_Pure(মিথ্যা);
            finalCondp
                = নতুন AstLogAnd{flp, finalCondp, নতুন AstLogNot{flp, abort_Fire_রাশিমালা(condp, flp)}};
        }
        ফেরত {body_ফলাফল.termVertexp, finalCondp, body_ফলাফল.midSources};
    }

সর্বজনীন:
    // Free a dropped sub-result condition that is not linked into the AST
    // (abort folds synthesize unparented finalCondp trees).
    স্থির শূন্য freeUnlinkedCondp(Ast_গ্রন্থি_রাশিমালা* condp) {
        যদি (condp && !condp->backp()) VL_DO_DANGLING(condp->delete_বৃক্ষ(), condp);
    }

    SvaNfaBuilder(SvaGraph& graph, Ast_গ্রন্থি_Module* modp, V3UniqueNames& prop_অস্থায়ী_Names,
                  যৌক্তিক isCoverSeq = মিথ্যা, যৌক্তিক isSeqEvent = মিথ্যা, যৌক্তিক isCover = মিথ্যা)
        : m_graph{graph}
        , m_modp{modp}
        , m_prop_অস্থায়ী_Names{prop_অস্থায়ী_Names}
        , m_isCover{isCover}
        , m_isCoverSeq{isCoverSeq}
        , m_isSeqEvent{isSeqEvent} {}

    // Reset scope between antecedent and consequent: liveness must not leak.
    // m_outerAbortStack survives: an abort wrapping the implication covers the
    // consequent too (IEEE 1800-2023 16.12.14).
    শূন্য resetScope() {
        m_inUnboundedScope = মিথ্যা;
        m_temporal_Guard_স্তূপ.clear();
    }

    Build_ফলাফল build_রাশিমালা(Ast_গ্রন্থি_রাশিমালা* nodep, SvaStateVertex* entryVtxp,
                          যৌক্তিক is_শীর্ষ_Level_Step = মিথ্যা) {
        VL_RESTORER(m_wantBoolLeaf);
        ধ্রুবক যৌক্তিক wantBoolLeaf = m_wantBoolLeaf;
        m_wantBoolLeaf = মিথ্যা;
        যদি (AstSExpr* ধ্রুবক sexprp = VN_CAST(nodep, SExpr)) {
            ফেরত buildSExpr(sexprp, entryVtxp, is_শীর্ষ_Level_Step);
        }
        যদি (AstSConsRep* ধ্রুবক repp = VN_CAST(nodep, SConsRep)) {
            ফেরত buildConsRep(repp, entryVtxp, is_শীর্ষ_Level_Step);
        }
        যদি (AstPropAlways* ধ্রুবক alwaysp = VN_CAST(nodep, PropAlways)) {
            ফেরত buildPropAlways(alwaysp, entryVtxp, is_শীর্ষ_Level_Step);
        }
        যদি (AstSGotoRep* ধ্রুবক repp = VN_CAST(nodep, SGotoRep)) {
            ফেরত buildGotoRep(repp, entryVtxp);
        }
        যদি (AstSThroughout* ধ্রুবক throughoutp = VN_CAST(nodep, SThroughout)) {
            ফেরত buildThroughout(throughoutp, entryVtxp, is_শীর্ষ_Level_Step);
        }
        যদি (AstSOr* ধ্রুবক orp = VN_CAST(nodep, SOr)) {
            ফেরত buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        যদি (AstLogOr* ধ্রুবক orp = VN_CAST(nodep, LogOr)) {
            ফেরত buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        যদি (AstSAnd* ধ্রুবক andp = VN_CAST(nodep, SAnd)) {
            ফেরত buildAndCombiner(andp->lhsp(), andp->rhsp(), entryVtxp, andp->fileline());
        }
        যদি (AstSIntersect* ধ্রুবক intp = VN_CAST(nodep, SIntersect)) {
            // IEEE 1800-2023 16.9.6: both operands match over one window with
            // equal start and end (equal length). Lower to a single sequence
            // that conjoins both operands' per-cycle checks -- correct under
            // concurrent attempts, where the done-latch combiner conflates the
            // two operands' start times and over-accepts. The combiner remains
            // only as a fallback for operands that do not flatten.
            ধ্রুবক পূর্ণসংখ্যা lhsLen = fixed_দৈর্ঘ্য(intp->lhsp());
            ধ্রুবক পূর্ণসংখ্যা rhsLen = fixed_দৈর্ঘ্য(intp->rhsp());
            যদি (lhsLen >= 0 && rhsLen >= 0) {
                যদি (lhsLen != rhsLen) {
                    // Unequal fixed lengths share no common length -> never matches.
                    ফেরত buildNeverMatchIntersect(intp, entryVtxp, is_শীর্ষ_Level_Step,
                                                    "intersect operands have no common length");
                }
                যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক conjp
                    = conjoinFixedSeqs(intp->lhsp(), intp->rhsp(), intp->fileline())) {
                    ফেরত build_From_Lowering_বৃক্ষ(conjp, entryVtxp, is_শীর্ষ_Level_Step);
                }
                ফেরত buildAndCombiner(intp->lhsp(), intp->rhsp(), entryVtxp, intp->fileline());
            }
            ফেরত buildVarLenIntersect(intp, entryVtxp, is_শীর্ষ_Level_Step);
        }
        যদি (AstSWithin* ধ্রুবক withinp = VN_CAST(nodep, SWithin)) {
            ফেরত buildSWithin(withinp, entryVtxp, is_শীর্ষ_Level_Step);
        }
        যদি (AstAbortOn* ধ্রুবক ap = VN_CAST(nodep, AbortOn)) {
            ফেরত buildAbortOn(ap->condp(), ap->propp(), entryVtxp, ap->kind(), ap->fileline(),
                                is_শীর্ষ_Level_Step);
        }
        যদি (VN_IS(nodep, SNonConsRep)) ফেরত Build_ফলাফল::fail();
        যদি (AstImplication* ধ্রুবক implp = VN_CAST(nodep, Implication)) {
            ফেরত buildImplicationEdges(implp->lhsp(), implp->rhsp(), entryVtxp,
                                         implp->isOverlapped(), implp->isFollowedBy(),
                                         implp->lhsp(), implp->fileline());
        }
        যদি (AstUntil* ধ্রুবক untilp = VN_CAST(nodep, Until)) {
            ফেরত buildUntil(untilp, entryVtxp, is_শীর্ষ_Level_Step);
        }
        // Boolean leaf (including LogAnd): return as finalCond
        ফেরত {entryVtxp, nodep, {}};
    }

    // Wire an implication / followed-by from `entryVtxp`: builds the antecedent,
    // emits the match-link (and for followed-by the reject-sink edge), inserts a
    // delay vertex for non-overlapped forms, and builds the body. Used both for
    // nested AstImplication in pexpr position and for the top-level assertion
    // antecedent -- `errorNodep` anchors the "unsupported sequence antecedent"
    // error, which differs between the two call sites.
    Build_ফলাফল buildImplicationEdges(Ast_গ্রন্থি_রাশিমালা* antExprp, Ast_গ্রন্থি_রাশিমালা* bodyExprp,
                                      SvaStateVertex* entryVtxp, যৌক্তিক isOverlapped,
                                      যৌক্তিক isFollowedBy, Ast_গ্রন্থি* errorNodep, File_সারি* flp) {
        ধ্রুবক Build_ফলাফল ant_ফলাফল = build_রাশিমালা(antExprp, entryVtxp);
        যদি (!ant_ফলাফল.valid()) ফেরত ant_ফলাফল;

        // Followed-by requires pure-boolean antecedent for non-vacuous-fail at
        // the attempt-start cycle. IEEE 1800-2023 16.12.9 permits a multi-cycle
        // sequence LHS, so this is an implementation gap rather than illegal SV.
        যদি (isFollowedBy && ant_ফলাফল.termVertexp != entryVtxp) {
            errorNodep->v3warn(E_UNSUPPORTED,
                               "Unsupported: sequence expression as antecedent of followed-by"
                               " (#-# / #=#) (IEEE 1800-2023 16.12.9)");
            ফেরত Build_ফলাফল::failWithError();
        }
        UASSERT_OBJ(!isFollowedBy || ant_ফলাফল.finalCondp, errorNodep,
                    "followed-by antecedent terminal at entry must carry finalCondp");

        // Use raw createStateVertex() so trigVtxp starts without liveness --
        // reaching the antecedent terminal is a definitive event.
        SvaStateVertex* ধ্রুবক trigVtxp = m_graph.createStateVertex();
        যদি (ant_ফলাফল.finalCondp) {
            m_graph.addLink(ant_ফলাফল.termVertexp, trigVtxp,
                            sampled(ant_ফলাফল.finalCondp->clone_বৃক্ষ_Pure(মিথ্যা)));
            // Followed-by non-vacuous fail: rejectOnFail fires when the attempt
            // is live (termVtx reachable) and sampled(antecedent) is false.
            যদি (isFollowedBy) {
                SvaStateVertex* ধ্রুবক sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = সত্য;
                SvaTransEdge* ধ্রুবক ep
                    = m_graph.addLink(ant_ফলাফল.termVertexp, sinkVtxp,
                                      sampled(ant_ফলাফল.finalCondp->clone_বৃক্ষ_Pure(মিথ্যা)));
                ep->m_rejectOnFail = সত্য;
            }
            // finalCondp is cloned into the Sampled nodes; if the original is
            // not parented anywhere in the AST anymore it must be freed here
            // or ASan flags it as a leak (e.g. t_sequence_bool_ops).
            যদি (!ant_ফলাফল.finalCondp->backp()) {
                VL_DO_DANGLING(ant_ফলাফল.finalCondp->delete_বৃক্ষ(), ant_ফলাফল.finalCondp);
            }
        } অন্যথা {
            m_graph.addLink(ant_ফলাফল.termVertexp, trigVtxp);
        }
        resetScope();

        SvaStateVertex* bodyEntryp = trigVtxp;
        যদি (!isOverlapped) {
            SvaStateVertex* ধ্রুবক delayVtxp = m_graph.createStateVertex();
            m_graph.addClockedEdge(trigVtxp, delayVtxp);
            bodyEntryp = delayVtxp;
        }
        // An empty antecedent match must not start the consequent, but a zero-minimum
        // repetition's empty match is not handled here; keep the merge vertex for those
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = !isFollowedBy && !hasZeroMinRep(antExprp);
        ফেরত build_রাশিমালা(bodyExprp, bodyEntryp, /*isTopLevelStep=*/সত্য);
    }

    Build_ফলাফল build(Ast_গ্রন্থি_রাশিমালা* exprp) {
        m_graph.m_startVertexp = scopedCreateVertex();
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = সত্য;
        ফেরত build_রাশিমালা(exprp, m_graph.m_startVertexp, /*isTopLevelStep=*/সত্য);
    }
};

//######################################################################
// NFA Lowering (converts NFA graph to synthesizable AstAlways blocks)

শ্রেণী SvaNfaLowering final {
    Ast_গ্রন্থি_Module* ধ্রুবক m_modp;  // Module to add state vars and always blocks to
    Ast_গ্রন্থি_DType* ধ্রুবক m_u32DTypep;  // Shared unsigned counter dtype
    V3UniqueNames m_names{"__Vnfa"};
    আকার_t m_statDelayRingEdgeVisits = 0;  // Delay-ring incoming edges visited

    // Per-lowering shared context (passed to phase sub-functions)
    // Per-vertex lowering state is stored in SvaVertexData and accessed via
    // V3GraphVertex::userp() (see SvaStateVertex::datap()).
    কাঠামো LowerCtx final {
        File_সারি* flp;  // Source location for generated AST
        Ast_Sen_বৃক্ষ* senTreep;  // Clock sensitivity tree
        Ast_গ্রন্থি_রাশিমালা* disableExprp;  // disable iff expression (may be nullptr)
        Ast_গ্রন্থি_রাশিমালা* matchCondp;  // Final boolean match condition (may be nullptr)
        VAssertType assertType;  // Assertion type for control tasks
        VAssertDirectiveType directiveType;  // Directive type for control tasks
        AstVar* killVarp;  // Last observed kill generation
        SvaGraph& graph;  // NFA graph
    };

    // Build a match-now expression: stateSig[i] && $sampled(condp)
    স্থির Ast_গ্রন্থি_রাশিমালা* buildMatchNow(File_সারি* flp, Ast_গ্রন্থি_রাশিমালা* stateExprp, Ast_গ্রন্থি_রাশিমালা* condp) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক statep = stateExprp->clone_বৃক্ষ_Pure(মিথ্যা);
        যদি (!condp) ফেরত statep;
        ফেরত নতুন AstLogAnd{flp, statep, sampled(condp->clone_বৃক্ষ_Pure(মিথ্যা))};
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* andCond(File_সারি* flp, Ast_গ্রন্থি_রাশিমালা* exprp, Ast_গ্রন্থি_রাশিমালা* condp) {
        যদি (!condp) ফেরত exprp;
        ফেরত নতুন AstLogAnd{flp, exprp, condp->clone_বৃক্ষ_Pure(মিথ্যা)};
    }
    // bp is always non-null; only ap can be null (serving as accumulator).
    স্থির Ast_গ্রন্থি_রাশিমালা* orExprs(File_সারি* flp, Ast_গ্রন্থি_রাশিমালা* ap, Ast_গ্রন্থি_রাশিমালা* bp) {
        যদি (!ap) ফেরত bp;
        ফেরত নতুন AstLogOr{flp, ap, bp};
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* addCountp(File_সারি* ধ্রুবক flp, Ast_গ্রন্থি_রাশিমালা* ধ্রুবক যোগ_Countp,
                                  Ast_গ্রন্থি_রাশিমালা* ধ্রুবক contributionp,
                                  Ast_গ্রন্থি_রাশিমালা* ধ্রুবক enablep = nullptr) {
        // contribution = enable ? contribution : 0;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক enabledContributionp
            = enablep ? নতুন AstCond{flp, enablep, contributionp,
                                    newTypedConstp(flp, contributionp->dtypep(), 0)}
                      : contributionp;
        যদি (!যোগ_Countp) ফেরত enabledContributionp;
        ফেরত নতুন AstAdd{flp, যোগ_Countp, enabledContributionp};
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* gateCheckedCountp(File_সারি* ধ্রুবক flp, Ast_গ্রন্থি_রাশিমালা* ধ্রুবক countp,
                                          Ast_গ্রন্থি_রাশিমালা*& overflowp, Ast_গ্রন্থি_রাশিমালা* ধ্রুবক enablep) {
        যদি (overflowp) {
            overflowp = নতুন AstLogAnd{flp, enablep->clone_বৃক্ষ_Pure(মিথ্যা), overflowp};
        }
        ফেরত addCountp(flp, nullptr, countp, enablep);
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* addCheckedCountp(File_সারি* ধ্রুবক flp, Ast_গ্রন্থি_রাশিমালা* ধ্রুবক যোগ_Countp,
                                         Ast_গ্রন্থি_রাশিমালা* ধ্রুবক contributionp, Ast_গ্রন্থি_রাশিমালা*& overflowp,
                                         Ast_গ্রন্থি_রাশিমালা* ধ্রুবক contributionOverflowp = nullptr) {
        যদি (contributionOverflowp) { overflowp = orExprs(flp, overflowp, contributionOverflowp); }
        যদি (!যোগ_Countp) ফেরত contributionp;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক oldTotalp = যোগ_Countp->clone_বৃক্ষ_Pure(মিথ্যা);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক resultp = নতুন AstAdd{flp, যোগ_Countp, contributionp};
        overflowp
            = orExprs(flp, overflowp, নতুন AstLt{flp, resultp->clone_বৃক্ষ_Pure(মিথ্যা), oldTotalp});
        ফেরত resultp;
    }
    AstIf* new_গুনতি_Overflow_Warning(File_সারি* ধ্রুবক flp, Ast_গ্রন্থি_রাশিমালা* ধ্রুবক condp) ধ্রুবক {
        AstDisplay* ধ্রুবক dispp = নতুন AstDisplay{
            flp, VDisplayType::DT_WARNING,
            "Cover sequence match count overflowed 32-bit storage; results may be incorrect.",
            nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstIf* ধ্রুবক ifp = নতুন AstIf{flp, condp, dispp};
        ifp->branchPred(VBranchPred::BP_UNLIKELY);
        ফেরত ifp;
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* killActive(LowerCtx& c) {
        ফেরত নতুন AstNeq{c.flp, নতুন AstVarRef{c.flp, c.killVarp, VAccess::READ},
                          assertKillGet(c.flp, c.assertType, c.directiveType)};
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* notKillActive(LowerCtx& c) { ফেরত নতুন AstLogNot{c.flp, killActive(c)}; }
    স্থির Ast_গ্রন্থি_রাশিমালা* gateNotKill(LowerCtx& c, Ast_গ্রন্থি_রাশিমালা* exprp) {
        যদি (!exprp) ফেরত nullptr;
        ফেরত নতুন AstLogAnd{c.flp, exprp, notKillActive(c)};
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* পরবর্তী_Ring_সূচক(File_সারি* flp, Ast_গ্রন্থি_রাশিমালা* idxExprp, uint32_t আকার_2) {
        ধ্রুবক auto u32Const = [flp](uint32_t মান) {
            ফেরত নতুন AstConst{flp, AstConst::Widthed_মান{}, 32, মান};
        };
        UASSERT_OBJ(আকার_2 > 0, idxExprp, "Ring size must be positive");
        যদি (আকার_2 == 1) {
            VL_DO_DANGLING(idxExprp->delete_বৃক্ষ(), idxExprp);
            ফেরত u32Const(0);
        }
        // idx == size - 1 ? 0 : idx + 1
        AstAdd* ধ্রুবক addp = নতুন AstAdd{flp, idxExprp->clone_বৃক্ষ_Pure(মিথ্যা), u32Const(1)};
        addp->dtypeFrom(idxExprp);
        AstCond* ধ্রুবক condp
            = নতুন AstCond{flp, নতুন AstEq{flp, idxExprp, u32Const(আকার_2 - 1)}, u32Const(0), addp};
        condp->dtypeFrom(idxExprp);
        ফেরত condp;
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* delayRingSlot(File_সারি* flp, SvaStateVertex* vtxp, Ast_গ্রন্থি_রাশিমালা* idxExprp,
                                      VAccess access = VAccess::READ) {
        // ring[idx]
        AstVar* ধ্রুবক ringp = vtxp->datap()->delayRingVarp;
        AstVarRef* ধ্রুবক refp = নতুন AstVarRef{flp, ringp, access};
        যদি (vtxp->datap()->delayRingCounts) ফেরত নতুন AstArraySel{flp, refp, idxExprp};
        ফেরত নতুন AstSel{flp, refp, idxExprp, 1};
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* delay_Ring_শীর্ষ_গুনতি(File_সারি* flp, SvaStateVertex* vtxp) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক headp
            = নতুন AstVarRef{flp, vtxp->datap()->delay_Ring_শীর্ষ_Varp, VAccess::READ};
        যদি (vtxp->datap()->delayRingCounts) ফেরত headp;
        ফেরত নতুন AstExtend{flp, headp, vtxp->datap()->delay_Ring_Live_গুনতি_Varp->dtypep()->width()};
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* delay_Ring_At_Last_সূচক(File_সারি* ধ্রুবক flp, AstVar* ধ্রুবক idxp,
                                             ধ্রুবক uint32_t আকার_2) {
        ফেরত নতুন AstEq{flp, নতুন AstVarRef{flp, idxp, VAccess::READ},
                         নতুন AstConst{flp, AstConst::Widthed_মান{}, 32, আকার_2 - 1}};
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* delay_Ring_Output_গুনতি(File_সারি* ধ্রুবক flp, SvaStateVertex* ধ্রুবক vtxp) {
        AstVar* ধ্রুবক idxp = vtxp->datap()->delayRingIdxVarp;
        ধ্রুবক uint32_t আকার_2 = vtxp->m_delay_Ring_আকার;
        Ast_গ্রন্থি_রাশিমালা* outgoingValidp
            = নতুন AstVarRef{flp, vtxp->datap()->delayRingWrappedVarp, VAccess::READ};
        যদি (!vtxp->m_isFixedDelayRing) {
            outgoingValidp
                = নতুন AstLogOr{flp, outgoingValidp, delay_Ring_At_Last_সূচক(flp, idxp, আকার_2)};
        }
        AstVar* ধ্রুবক live_গুনতি_Varp = vtxp->datap()->delay_Ring_Live_গুনতি_Varp;
        ফেরত নতুন AstCond{flp, outgoingValidp, delay_Ring_শীর্ষ_গুনতি(flp, vtxp),
                           newTypedConstp(flp, live_গুনতি_Varp->dtypep(), 0)};
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* delayRingHasLiveThreadsp(File_সারি* ধ্রুবক flp,
                                                 AstVar* ধ্রুবক live_গুনতি_Varp) {
        // active = live_count != 0;
        ফেরত নতুন AstNeq{flp, নতুন AstVarRef{flp, live_গুনতি_Varp, VAccess::READ},
                          newTypedConstp(flp, live_গুনতি_Varp->dtypep(), 0)};
    }

    // Phase 3 output signals
    কাঠামো SignalSet final {
        Ast_গ্রন্থি_রাশিমালা* terminalActivep = nullptr;  // OR of all successful terminal matches
        Ast_গ্রন্থি_রাশিমালা* matchCountp = nullptr;  // NFA paths completing the sequence this tick
        Ast_গ্রন্থি_রাশিমালা* match_গুনতি_Overflowp = nullptr;  // Match count overflow this tick
        Ast_গ্রন্থি_রাশিমালা* rejectBasep = nullptr;  // Reject when a terminal match fails
        Ast_গ্রন্থি_রাশিমালা* requiredStepRejectp = nullptr;  // Per-source reject from rejectOnFail Links
        Ast_গ্রন্থি_রাশিমালা* throughoutRejectp = nullptr;  // Reject when a throughout guard drops
        Ast_গ্রন্থি_রাশিমালা* threadFailCountp = nullptr;  // Number of threads rejected on this tick
    };

    // Disable condition also aborts attempts between clock events. A constant disable
    // never rises, and $sampled is unsupported in a sensitivity list.
    স্থির যৌক্তিক isAsyncDisable(ধ্রুবক Ast_গ্রন্থি_রাশিমালা* ধ্রুবক disableExprp) {
        ফেরত disableExprp && !VN_IS(disableExprp, Const)
               && !disableExprp->exists([](ধ্রুবক AstSampled*) { ফেরত সত্য; });
    }

    // Emit the clocked update block of registers that hold pending attempts. The clearp
    // statements reset the same registers: a disable rising between clock events also
    // aborts pending attempts (IEEE 1800-2023 16.12), so clearp runs on that rising edge
    // in a separate block. It must clear unconditionally, as the disable may already be
    // false again when it runs, and the update block must not run on that edge, as it
    // would advance the attempts as if the clock had ticked. Blocks that hold no attempt
    // state, such as the $assertkill acknowledge in emitKillAckNba, do not go through here.
    // Callers pass a null clearp when the disable cannot rise between clock events.
    শূন্য addStateAlways(LowerCtx& c, Ast_গ্রন্থি* bodyp, Ast_গ্রন্থি* clearp) {
        m_modp->addStmtsp(
            নতুন AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_বৃক্ষ(মিথ্যা), bodyp});
        যদি (!clearp) ফেরত;
        m_modp->addStmtsp(নতুন AstAlways{
            c.flp, VAlwaysKwd::ALWAYS,
            নতুন Ast_Sen_বৃক্ষ{c.flp, নতুন AstSenItem{c.flp, VEdgeType::ET_POSEDGE,
                                                 c.disableExprp->clone_বৃক্ষ_Pure(মিথ্যা)}},
            clearp});
    }

    // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
    // Phase 2: State register NBA always block. Each clocked-edge target
    // latches the OR of its incoming contributions.
    শূন্য emitStateRegisterNba(LowerCtx& c) {
        ধ্রুবক যৌক্তিক asyncDisable = isAsyncDisable(c.disableExprp);
        Ast_গ্রন্থি* bodyp = nullptr;
        Ast_গ্রন্থি* clearp = nullptr;
        জন্য (ধ্রুবক V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ধ্রুবক SvaStateVertex* ধ্রুবক vtxp = static_cast<ধ্রুবক SvaStateVertex*>(&vtxr);
            যদি (!vtxp->datap()->stateVarp) চালু;

            Ast_গ্রন্থি_রাশিমালা* পরবর্তী_Statep = nullptr;
            জন্য (ধ্রুবক V3GraphEdge& edger : vtxp->inEdges()) {
                ধ্রুবক SvaTransEdge& tedger = static_cast<ধ্রুবক SvaTransEdge&>(edger);
                যদি (!tedger.m_consumesCycle) চালু;
                ধ্রুবক SvaStateVertex* ধ্রুবক fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Clocked-edge source missing stateSig");

                Ast_গ্রন্থি_রাশিমালা* srcSigp = fromVtxp->datap()->stateSigp->clone_বৃক্ষ_Pure(মিথ্যা);
                srcSigp = andCond(c.flp, srcSigp, tedger.m_condp);

                যদি (c.disableExprp) {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক notDisp
                        = নতুন AstLogNot{c.flp, c.disableExprp->clone_বৃক্ষ_Pure(মিথ্যা)};
                    srcSigp = নতুন AstLogAnd{c.flp, srcSigp, notDisp};
                }
                পরবর্তী_Statep = orExprs(c.flp, পরবর্তী_Statep, srcSigp);
            }

            UASSERT_OBJ(পরবর্তী_Statep, vtxp,
                        "Registered vertex has no clocked incoming contribution");
            পরবর্তী_Statep = gateNotKill(c, পরবর্তী_Statep);

            AstAssignDly* ধ্রুবক assignp = নতুন AstAssignDly{
                c.flp, নতুন AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE}, পরবর্তী_Statep};
            bodyp = Ast_গ্রন্থি::add_পরবর্তী_Null(bodyp, assignp);
            যদি (asyncDisable) {
                clearp = Ast_গ্রন্থি::add_পরবর্তী_Null(
                    clearp,
                    নতুন AstAssignDly{
                        c.flp, নতুন AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE},
                        নতুন AstConst{c.flp, AstConst::BitFalse{}}});
            }
        }
        যদি (bodyp) addStateAlways(c, bodyp, clearp);
    }

    // Phase 2b: In-place ring storage and registered ring metadata.
    শূন্য emitDelayRingUpdates(LowerCtx& c, ধ্রুবক যৌক্তিক গুনতি_Matches) {
        ধ্রুবক যৌক্তিক asyncDisable = isAsyncDisable(c.disableExprp);
        জন্য (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ধ্রুবক vtxp = static_cast<SvaStateVertex*>(&vtxr);
            যদি (!vtxp->datap()->delayRingVarp) চালু;
            AstVar* ধ্রুবক idxp = vtxp->datap()->delayRingIdxVarp;
            AstVar* ধ্রুবক live_গুনতি_Varp = vtxp->datap()->delay_Ring_Live_গুনতি_Varp;
            AstVar* ধ্রুবক wrappedp = vtxp->datap()->delayRingWrappedVarp;
            ধ্রুবক uint32_t আকার_2 = static_cast<uint32_t>(vtxp->m_delay_Ring_আকার);

            Ast_গ্রন্থি_রাশিমালা* incomingp = nullptr;
            Ast_গ্রন্থি_রাশিমালা* incomingOverflowp = nullptr;
            জন্য (ধ্রুবক V3GraphEdge& edger : vtxp->inEdges()) {
                ++m_statDelayRingEdgeVisits;
                ধ্রুবক SvaTransEdge& tedger = static_cast<ধ্রুবক SvaTransEdge&>(edger);
                UASSERT_OBJ(tedger.m_consumesCycle == vtxp->m_isFixedDelayRing, vtxp,
                            "Delay-ring incoming edge kind mismatch");
                ধ্রুবক SvaStateVertex* ধ্রুবক fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Delay-ring incoming source missing stateSig");
                UASSERT_OBJ(!গুনতি_Matches || fromVtxp->datap()->state_গুনতি_Sigp, fromVtxp,
                            "Delay-ring incoming source missing stateCountSig");
                // IEEE 1800-2023 16.14.1/16.14.3: assertion alternatives belong to one
                // evaluation; only cover sequence counts each end-of-match.
                Ast_গ্রন্থি_রাশিমালা* contribp
                    = গুনতি_Matches
                          ? fromVtxp->datap()->state_গুনতি_Sigp->clone_বৃক্ষ_Pure(মিথ্যা)
                          : নতুন AstExtend{c.flp,
                                          fromVtxp->datap()->stateSigp->clone_বৃক্ষ_Pure(মিথ্যা),
                                          m_u32DTypep->width()};
                Ast_গ্রন্থি_রাশিমালা* contribOverflowp
                    = গুনতি_Matches && fromVtxp->datap()->state_গুনতি_Overflow_Sigp
                          ? fromVtxp->datap()->state_গুনতি_Overflow_Sigp->clone_বৃক্ষ_Pure(মিথ্যা)
                          : nullptr;
                যদি (tedger.m_condp) {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক enablep = tedger.m_condp->clone_বৃক্ষ_Pure(মিথ্যা);
                    contribp = গুনতি_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, enablep)
                                   : addCountp(c.flp, nullptr, contribp, enablep);
                }
                যদি (c.disableExprp) {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক notDisp
                        = নতুন AstLogNot{c.flp, c.disableExprp->clone_বৃক্ষ_Pure(মিথ্যা)};
                    contribp = গুনতি_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, notDisp)
                                   : addCountp(c.flp, nullptr, contribp, notDisp);
                }
                incomingp = গুনতি_Matches ? addCheckedCountp(c.flp, incomingp, contribp,
                                                            incomingOverflowp, contribOverflowp)
                                         : addCountp(c.flp, incomingp, contribp);
            }
            UASSERT_OBJ(incomingp, vtxp, "Delay ring has no incoming edge");
            ধ্রুবক যৌক্তিক storesCounts = vtxp->datap()->delayRingCounts;
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক incomingCountp
                = storesCounts
                      ? incomingp->clone_বৃক্ষ_Pure(মিথ্যা)
                      : নতুন AstExtend{c.flp,
                                      নতুন AstNeq{c.flp, incomingp->clone_বৃক্ষ_Pure(মিথ্যা),
                                                 newTypedConstp(c.flp, incomingp->dtypep(), 0)},
                                      m_u32DTypep->width()};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক storedIncomingp
                = storesCounts ? incomingp
                               : নতুন AstNeq{c.flp, incomingp,
                                            newTypedConstp(c.flp, incomingp->dtypep(), 0)};
            // Only this block accesses the storage. Other processes read the cached head, so
            // the ring can be updated in place without an NBA shadow copy.
            AstAssign* ধ্রুবক writeIncomingp = নতুন AstAssign{
                c.flp,
                delayRingSlot(c.flp, vtxp, নতুন AstVarRef{c.flp, idxp, VAccess::READ},
                              VAccess::WRITE),
                storedIncomingp};
            Ast_গ্রন্থি* updateBodyp = writeIncomingp;
            // live_count <= live_count - outgoing_count + incoming_count;
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক outgoingp = delay_Ring_Output_গুনতি(c.flp, vtxp);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক afterOutgoingp
                = নতুন AstSub{c.flp, নতুন AstVarRef{c.flp, live_গুনতি_Varp, VAccess::READ}, outgoingp};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক oldAfterOutgoingp = afterOutgoingp->clone_বৃক্ষ_Pure(মিথ্যা);
            AstAdd* ধ্রুবক পরবর্তী_Live_Countp = নতুন AstAdd{c.flp, afterOutgoingp, incomingCountp};
            যদি (storesCounts) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক liveOverflowp
                    = নতুন AstLt{c.flp, পরবর্তী_Live_Countp->clone_বৃক্ষ_Pure(মিথ্যা), oldAfterOutgoingp};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক overflowp = orExprs(c.flp, incomingOverflowp, liveOverflowp);
                updateBodyp = new_গুনতি_Overflow_Warning(c.flp, overflowp);
                updateBodyp->add_পরবর্তী(writeIncomingp);
            } অন্যথা {
                VL_DO_DANGLING(oldAfterOutgoingp->delete_বৃক্ষ(), oldAfterOutgoingp);
            }
            // Cache the outgoing slot for the next index after the write. Reading after
            // writing also handles one-slot rings and two-slot range rings without alias cases.
            Ast_গ্রন্থি_রাশিমালা* পরবর্তী_শীর্ষ_Idxp
                = পরবর্তী_Ring_সূচক(c.flp, নতুন AstVarRef{c.flp, idxp, VAccess::READ}, আকার_2);
            যদি (!vtxp->m_isFixedDelayRing) {
                পরবর্তী_শীর্ষ_Idxp = পরবর্তী_Ring_সূচক(c.flp, পরবর্তী_শীর্ষ_Idxp, আকার_2);
            }
            updateBodyp->add_পরবর্তী(নতুন AstAssignDly{
                c.flp, নতুন AstVarRef{c.flp, vtxp->datap()->delay_Ring_শীর্ষ_Varp, VAccess::WRITE},
                delayRingSlot(c.flp, vtxp, পরবর্তী_শীর্ষ_Idxp)});
            updateBodyp->add_পরবর্তী(নতুন AstAssignDly{
                c.flp, নতুন AstVarRef{c.flp, live_গুনতি_Varp, VAccess::WRITE}, পরবর্তী_Live_Countp});
            // wrapped <= wrapped || idx == size - 1;
            updateBodyp->add_পরবর্তী(
                নতুন AstAssignDly{c.flp, নতুন AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                 নতুন AstLogOr{c.flp, নতুন AstVarRef{c.flp, wrappedp, VAccess::READ},
                                              delay_Ring_At_Last_সূচক(c.flp, idxp, আকার_2)}});
            // idx <= next_idx;
            updateBodyp->add_পরবর্তী(নতুন AstAssignDly{
                c.flp, নতুন AstVarRef{c.flp, idxp, VAccess::WRITE},
                পরবর্তী_Ring_সূচক(c.flp, নতুন AstVarRef{c.flp, idxp, VAccess::READ}, আকার_2)});
            যদি (vtxp->m_delayRingAdvanceCondp) {
                updateBodyp = নতুন AstIf{
                    c.flp, sampled(vtxp->m_delayRingAdvanceCondp->clone_বৃক্ষ_Pure(মিথ্যা)),
                    updateBodyp};
            }

            Ast_গ্রন্থি_রাশিমালা* clearCondp = killActive(c);
            যদি (vtxp->m_delayRingClearCondp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_delayRingClearCondp->clone_বৃক্ষ_Pure(মিথ্যা)));
            }
            যদি (vtxp->m_abortClearp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_abortClearp->clone_বৃক্ষ_Pure(মিথ্যা)));
            }
            যদি (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_বৃক্ষ_Pure(মিথ্যা));
            }
            Ast_গ্রন্থি_রাশিমালা* guardp = nullptr;
            জন্য (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক cp : vtxp->m_throughoutConds) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক sampledp = sampled(cp->clone_বৃক্ষ_Pure(মিথ্যা));
                guardp = guardp ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নতুন AstLogAnd{c.flp, guardp, sampledp})
                                : sampledp;
            }
            যদি (guardp) clearCondp = orExprs(c.flp, clearCondp, নতুন AstLogNot{c.flp, guardp});
            // Logically clear the ring without touching its wide storage.
            AstAssignDly* ধ্রুবক clearCountp
                = নতুন AstAssignDly{c.flp, নতুন AstVarRef{c.flp, live_গুনতি_Varp, VAccess::WRITE},
                                   newTypedConstp(c.flp, live_গুনতি_Varp->dtypep(), 0)};
            clearCountp->add_পরবর্তী(নতুন AstAssignDly{c.flp,
                                                  নতুন AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                                  নতুন AstConst{c.flp, AstConst::BitFalse{}}});
            clearCountp->add_পরবর্তী(নতুন AstAssignDly{c.flp,
                                                  নতুন AstVarRef{c.flp, idxp, VAccess::WRITE},
                                                  newTypedConstp(c.flp, idxp->dtypep(), 0)});
            updateBodyp = নতুন AstIf{c.flp, clearCondp, clearCountp, updateBodyp};

            addStateAlways(c, updateBodyp, asyncDisable ? clearCountp->clone_বৃক্ষ(সত্য) : nullptr);
        }
    }

    // Phase 2c: SAnd combiner done-latch always block.
    // NBA semantics ensure doneL/doneR read pre-update values (IEEE 16.9.5).
    শূন্য emitAndCombinerDoneLatchNba(LowerCtx& c) {
        ধ্রুবক যৌক্তিক asyncDisable = isAsyncDisable(c.disableExprp);
        জন্য (ধ্রুবক V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ধ্রুবক SvaStateVertex* ধ্রুবক avp = static_cast<ধ্রুবক SvaStateVertex*>(&vtxr);
            যদি (!avp->datap()->doneLVarp) চালু;
            // doneLVars is non-null only for AndCombiner vertices, which always
            // have both m_andLhsTermp and m_andRhsTermp set at build time.
            UASSERT_OBJ(avp->m_andLhsTermp && avp->m_andRhsTermp, avp,
                        "AndCombiner vertex missing LHS/RHS terminal");
            ধ্রুবক SvaStateVertex* ধ্রুবক lhsTermp = avp->m_andLhsTermp;
            ধ্রুবক SvaStateVertex* ধ্রুবক rhsTermp = avp->m_andRhsTermp;
            // resolveLinks' 2*N+2 fixed-point pass is guaranteed to populate
            // stateSigp on every AndCombiner and its LHS/RHS terminals.
            UASSERT_OBJ(lhsTermp->datap()->stateSigp && rhsTermp->datap()->stateSigp
                            && avp->datap()->stateSigp,
                        avp, "AndCombiner stateSigp chain unresolved after resolveLinks");

            AstAssignDly* ধ্রুবক clearLp = নতুন AstAssignDly{
                c.flp, নতুন AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                নতুন AstConst{c.flp, AstConst::BitFalse{}}};
            AstAssignDly* ধ্রুবক clearRp = নতুন AstAssignDly{
                c.flp, নতুন AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                নতুন AstConst{c.flp, AstConst::BitFalse{}}};
            clearLp->add_পরবর্তী(clearRp);

            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক matchLNowp
                = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, avp->m_andLhsCondp);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক matchRNowp
                = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, avp->m_andRhsCondp);
            Ast_গ্রন্থি_রাশিমালা* gateLp = matchLNowp;
            Ast_গ্রন্থি_রাশিমালা* gateRp = matchRNowp;
            যদি (c.disableExprp) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক notDisLp
                    = নতুন AstLogNot{c.flp, c.disableExprp->clone_বৃক্ষ_Pure(মিথ্যা)};
                gateLp = নতুন AstLogAnd{c.flp, gateLp, notDisLp};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক notDisRp
                    = নতুন AstLogNot{c.flp, c.disableExprp->clone_বৃক্ষ_Pure(মিথ্যা)};
                gateRp = নতুন AstLogAnd{c.flp, gateRp, notDisRp};
            }
            AstAssignDly* ধ্রুবক setLp = নতুন AstAssignDly{
                c.flp, নতুন AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                নতুন AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* ধ্রুবক setLIfp = নতুন AstIf{c.flp, gateLp, setLp, nullptr};
            AstAssignDly* ধ্রুবক setRp = নতুন AstAssignDly{
                c.flp, নতুন AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                নতুন AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* ধ্রুবক setRIfp = নতুন AstIf{c.flp, gateRp, setRp, nullptr};
            setLIfp->add_পরবর্তী(setRIfp);

            Ast_গ্রন্থি_রাশিমালা* clearCondp = নতুন AstLogOr{c.flp, killActive(c),
                                                   avp->datap()->stateSigp->clone_বৃক্ষ_Pure(মিথ্যা)};
            যদি (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_বৃক্ষ_Pure(মিথ্যা));
            }
            AstIf* ধ্রুবক topp = নতুন AstIf{c.flp, clearCondp, clearLp, setLIfp};
            addStateAlways(c, topp, asyncDisable ? clearLp->clone_বৃক্ষ(সত্য) : nullptr);
        }
    }

    শূন্য emitKillAckNba(LowerCtx& c) {
        AstAssignDly* ধ্রুবক ackp
            = নতুন AstAssignDly{c.flp, নতুন AstVarRef{c.flp, c.killVarp, VAccess::WRITE},
                               assertKillGet(c.flp, c.assertType, c.directiveType)};
        m_modp->addStmtsp(
            নতুন AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_বৃক্ষ(মিথ্যা), ackp});
    }

    // Phase 3/3a/3b: Compute terminal match/reject signals, required-step reject,
    // throughout-drop reject; clean up intermediate state signals.
    // Phase 3: terminalActive and rejectBase from Links to matchVertex.
    // Builder only adds Links (non-clocked) to matchVertex via addLink in
    // wireMatchAndMidSources. For cover sequence, also count each end-of-match
    // so the action can be replayed without unrolling endpoints.
    শূন্য computeTerminalMatchAndReject(LowerCtx& c, SignalSet& sigs, ধ্রুবক যৌক্তিক need_Match_গুনতি) {
        জন্য (ধ্রুবক SvaTransEdge* ধ্রুবক tedgep : c.graph.allEdges()) {
            যদি (tedgep->toVtxp() != c.graph.m_matchVertexp) চালু;
            SvaStateVertex* ধ্রুবক fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                        "Terminal-link source missing stateSig");

            Ast_গ্রন্থি_রাশিমালা* srcSigp = fromVtxp->datap()->stateSigp->clone_বৃক্ষ_Pure(মিথ্যা);
            srcSigp = andCond(c.flp, srcSigp, tedgep->m_condp);
            যদি (need_Match_গুনতি) {
                UASSERT_OBJ(fromVtxp->datap()->state_গুনতি_Sigp, fromVtxp,
                            "Terminal-link source missing stateCountSig");
                Ast_গ্রন্থি_রাশিমালা* contributionp
                    = fromVtxp->datap()->state_গুনতি_Sigp->clone_বৃক্ষ_Pure(মিথ্যা);
                Ast_গ্রন্থি_রাশিমালা* contributionOverflowp
                    = fromVtxp->datap()->state_গুনতি_Overflow_Sigp
                          ? fromVtxp->datap()->state_গুনতি_Overflow_Sigp->clone_বৃক্ষ_Pure(মিথ্যা)
                          : nullptr;
                যদি (tedgep->m_condp) {
                    contributionp = gateCheckedCountp(c.flp, contributionp, contributionOverflowp,
                                                      tedgep->m_condp->clone_বৃক্ষ_Pure(মিথ্যা));
                }
                sigs.matchCountp
                    = addCheckedCountp(c.flp, sigs.matchCountp, contributionp,
                                       sigs.match_গুনতি_Overflowp, contributionOverflowp);
            }

            যদি (fromVtxp->m_delay_Ring_আকার && !fromVtxp->m_isFixedDelayRing) {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_বৃক্ষ_Pure(মিথ্যা));
                // reject |= (wrapped || idx == size - 1) && head && final_condition;
                Ast_গ্রন্থি_রাশিমালা* expireContribp = delay_Ring_Output_গুনতি(c.flp, fromVtxp);
                expireContribp = andCond(c.flp, expireContribp, tedgep->m_condp);
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, expireContribp);
                VL_DO_DANGLING(srcSigp->delete_বৃক্ষ(), srcSigp);
            } অন্যথা যদি (fromVtxp->m_isUnbounded || fromVtxp->m_isAndCombiner) {
                sigs.terminalActivep = orExprs(c.flp, sigs.terminalActivep, srcSigp);
            } অন্যথা {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_বৃক্ষ_Pure(মিথ্যা));
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, srcSigp);
            }
        }
        // wireMatchAndMidSources always adds a Link from result.termVertexp
        // to m_matchVertexp, so the loop above always sets terminalActivep.
        UASSERT_OBJ(sigs.terminalActivep, c.graph.m_matchVertexp,
                    "No terminal edge to match vertex");
    }

    Ast_গ্রন্থি_রাশিমালা* newThroughoutThreadFailCountp(LowerCtx& c, AstVar* ধ্রুবক delay_Ring_Live_গুনতি_Varp,
                                               Ast_গ্রন্থি_রাশিমালা* ধ্রুবক stateExprp,
                                               Ast_গ্রন্থি_রাশিমালা* ধ্রুবক enablep) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক activeThreadCountp
            = delay_Ring_Live_গুনতি_Varp
                  ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(
                        নতুন AstVarRef{c.flp, delay_Ring_Live_গুনতি_Varp, VAccess::READ})
                  : নতুন AstExtend{c.flp, stateExprp->clone_বৃক্ষ_Pure(মিথ্যা), m_u32DTypep->width()};
        ফেরত addCountp(c.flp, nullptr, activeThreadCountp, enablep);
    }

    // Phase 3b: Throughout-drop and ring-wide abort rejection.
    শূন্য computeThroughoutReject(LowerCtx& c, SignalSet& sigs, ধ্রুবক যৌক্তিক need_Thread_Fail_গুনতি) {
        জন্য (ধ্রুবক V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ধ্রুবক SvaStateVertex* ধ্রুবক vtxp = static_cast<ধ্রুবক SvaStateVertex*>(&vtxr);
            ধ্রুবক auto& conds = vtxp->m_throughoutConds;
            যদি (conds.empty() && !vtxp->m_abortRejectp) চালু;
            যদি (vtxp->m_isAndCombiner) চালু;
            Ast_গ্রন্থি_রাশিমালা* stateExprp = nullptr;
            যদি (vtxp->datap()->stateVarp) {
                stateExprp = নতুন AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
            } অন্যথা যদি (vtxp->datap()->delayRingVarp && vtxp->m_isFixedDelayRing) {
                stateExprp
                    = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_গুনতি_Varp);
            } অন্যথা {
                UASSERT_OBJ(vtxp->datap()->stateSigp, vtxp,
                            "Throughout-conds vertex missing state representation");
                stateExprp = vtxp->datap()->stateSigp->clone_বৃক্ষ_Pure(মিথ্যা);
            }
            Ast_গ্রন্থি_রাশিমালা* guardp = nullptr;
            জন্য (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক cp : conds) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক sp = sampled(cp->clone_বৃক্ষ_Pure(মিথ্যা));
                guardp = guardp ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নতুন AstLogAnd{c.flp, guardp, sp}) : sp;
            }
            যদি (vtxp->m_abortRejectp) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক notAbortp
                    = নতুন AstLogNot{c.flp, sampled(vtxp->m_abortRejectp->clone_বৃক্ষ_Pure(মিথ্যা))};
                guardp = guardp
                             ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নতুন AstLogAnd{c.flp, guardp, notAbortp})
                             : notAbortp;
            }
            Ast_গ্রন্থি_রাশিমালা* rejectCondp = নতুন AstLogNot{c.flp, guardp};
            যদি (vtxp->m_abortClearp) {
                // Any abort clears the ring. Accept aborts suppress a simultaneous guard failure;
                // reject aborts are restored below so they still force rejection.
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক notAbortClearp
                    = নতুন AstLogNot{c.flp, sampled(vtxp->m_abortClearp->clone_বৃক্ষ_Pure(মিথ্যা))};
                rejectCondp = নতুন AstLogAnd{c.flp, rejectCondp, notAbortClearp};
                যদি (vtxp->m_abortRejectp) {
                    rejectCondp = নতুন AstLogOr{
                        c.flp, rejectCondp, sampled(vtxp->m_abortRejectp->clone_বৃক্ষ_Pure(মিথ্যা))};
                }
            }
            যদি (need_Thread_Fail_গুনতি) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক contributionp
                    = newThroughoutThreadFailCountp(c, vtxp->datap()->delay_Ring_Live_গুনতি_Varp,
                                                    stateExprp, rejectCondp->clone_বৃক্ষ_Pure(মিথ্যা));
                sigs.threadFailCountp = addCountp(c.flp, sigs.threadFailCountp, contributionp);
                যদি (vtxp->m_abortRejectp && vtxp->m_delayRingAdvanceCondp) {
                    // An advancing ring thread also occupies its same-tick match vertex in the
                    // unrolled NFA, so abort rejection must replay both fail actions.
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক abortAndAdvancep = নতুন AstLogAnd{
                        c.flp, sampled(vtxp->m_abortRejectp->clone_বৃক্ষ_Pure(মিথ্যা)),
                        sampled(vtxp->m_delayRingAdvanceCondp->clone_বৃক্ষ_Pure(মিথ্যা))};
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক matchContributionp = newThroughoutThreadFailCountp(
                        c, vtxp->datap()->delay_Ring_Live_গুনতি_Varp, stateExprp, abortAndAdvancep);
                    sigs.threadFailCountp
                        = addCountp(c.flp, sigs.threadFailCountp, matchContributionp);
                }
            }
            sigs.throughoutRejectp = orExprs(c.flp, sigs.throughoutRejectp,
                                             নতুন AstLogAnd{c.flp, stateExprp, rejectCondp});
        }
    }

    SignalSet computeSignals(LowerCtx& c, ধ্রুবক যৌক্তিক need_Thread_Fail_গুনতি,
                             ধ্রুবক যৌক্তিক need_Throughout_Thread_Fail_গুনতি, ধ্রুবক যৌক্তিক need_Match_গুনতি) {
        SignalSet sigs;

        computeTerminalMatchAndReject(c, sigs, need_Match_গুনতি);

        // Phase 3a: required-step rejection.
        // Builder only sets m_rejectOnFail on non-clocked Links with m_condp
        // or m_condVtxp, and the source always has a resolved stateSig.
        জন্য (ধ্রুবক SvaTransEdge* ধ্রুবক tedgep : c.graph.allEdges()) {
            যদি (!tedgep->m_rejectOnFail) চালু;
            ধ্রুবক SvaStateVertex* ধ্রুবক fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp && (tedgep->m_condp || tedgep->m_condVtxp),
                        fromVtxp,
                        "rejectOnFail Link must have condp/condVtxp and source stateSig");
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক srcSigp = fromVtxp->datap()->stateSigp->clone_বৃক্ষ_Pure(মিথ্যা);
            Ast_গ্রন্থি_রাশিমালা* condp = nullptr;
            যদি (tedgep->m_condVtxp) {
                UASSERT_OBJ(tedgep->m_condVtxp->datap()->stateSigp, tedgep->m_condVtxp,
                            "rejectOnFail condVtxp missing stateSig");
                condp = tedgep->m_condVtxp->datap()->stateSigp->clone_বৃক্ষ_Pure(মিথ্যা);
                যদি (tedgep->m_condp) {
                    condp = নতুন AstLogOr{c.flp, condp, tedgep->m_condp->clone_বৃক্ষ_Pure(মিথ্যা)};
                }
            } অন্যথা {
                condp = tedgep->m_condp->clone_বৃক্ষ_Pure(মিথ্যা);
            }
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক notCondp = নতুন AstLogNot{c.flp, condp};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rawFailp = নতুন AstLogAnd{c.flp, srcSigp, notCondp};
            যদি (need_Thread_Fail_গুনতি) {
                // thread_fail_count += fail;
                sigs.threadFailCountp = addCountp(
                    c.flp, sigs.threadFailCountp,
                    নতুন AstExtend{c.flp, rawFailp->clone_বৃক্ষ_Pure(মিথ্যা), m_u32DTypep->width()});
            }
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক failp = gateNotKill(c, rawFailp);
            sigs.requiredStepRejectp = orExprs(c.flp, sigs.requiredStepRejectp, failp);
        }

        computeThroughoutReject(c, sigs, need_Throughout_Thread_Fail_গুনতি);
        যদি (sigs.threadFailCountp) {
            sigs.threadFailCountp
                = addCountp(c.flp, nullptr, sigs.threadFailCountp, notKillActive(c));
        }
        যদি (sigs.matchCountp) {
            যদি (c.matchCondp) {
                sigs.matchCountp
                    = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_গুনতি_Overflowp,
                                        sampled(c.matchCondp->clone_বৃক্ষ_Pure(মিথ্যা)));
            }
            sigs.matchCountp = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_গুনতি_Overflowp,
                                                 notKillActive(c));
        }
        sigs.terminalActivep = gateNotKill(c, sigs.terminalActivep);
        sigs.rejectBasep = gateNotKill(c, sigs.rejectBasep);
        sigs.throughoutRejectp = gateNotKill(c, sigs.throughoutRejectp);

        // Clean up intermediate state signals. These are orphan subtrees
        // (never linked into the enclosing AST); deleteTree() is immediate
        // which is what we want since stateSigp lifetime ends with this scope.
        জন্য (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ধ্রুবক vtxp = static_cast<SvaStateVertex*>(&vtxr);
            Ast_গ্রন্থি_রাশিমালা*& sigp = vtxp->datap()->stateSigp;
            যদি (sigp) VL_DO_DANGLING(sigp->delete_বৃক্ষ(), sigp);
            Ast_গ্রন্থি_রাশিমালা*& countp = vtxp->datap()->state_গুনতি_Sigp;
            যদি (countp) VL_DO_DANGLING(countp->delete_বৃক্ষ(), countp);
            Ast_গ্রন্থি_রাশিমালা*& overflowp = vtxp->datap()->state_গুনতি_Overflow_Sigp;
            যদি (overflowp) VL_DO_DANGLING(overflowp->delete_বৃক্ষ(), overflowp);
        }
        // Disable iff gating (IEEE 1800-2023 16.12) on the current level value.
        যদি (c.disableExprp) {
            // terminalActivep is always set, so gate it unconditionally.
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক notTermp
                = নতুন AstLogNot{c.flp, c.disableExprp->clone_বৃক্ষ_Pure(মিথ্যা)};
            sigs.terminalActivep = নতুন AstLogAnd{c.flp, sigs.terminalActivep, notTermp};
            যদি (sigs.rejectBasep) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক notDisp
                    = নতুন AstLogNot{c.flp, c.disableExprp->clone_বৃক্ষ_Pure(মিথ্যা)};
                sigs.rejectBasep = নতুন AstLogAnd{c.flp, sigs.rejectBasep, notDisp};
            }
            যদি (sigs.throughoutRejectp) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক notDisp
                    = নতুন AstLogNot{c.flp, c.disableExprp->clone_বৃক্ষ_Pure(মিথ্যা)};
                sigs.throughoutRejectp = নতুন AstLogAnd{c.flp, sigs.throughoutRejectp, notDisp};
            }
            যদি (sigs.requiredStepRejectp) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক notDisp
                    = নতুন AstLogNot{c.flp, c.disableExprp->clone_বৃক্ষ_Pure(মিথ্যা)};
                sigs.requiredStepRejectp = নতুন AstLogAnd{c.flp, sigs.requiredStepRejectp, notDisp};
            }
            যদি (sigs.matchCountp) {
                sigs.matchCountp = gateCheckedCountp(
                    c.flp, sigs.matchCountp, sigs.match_গুনতি_Overflowp,
                    নতুন AstLogNot{c.flp, c.disableExprp->clone_বৃক্ষ_Pure(মিথ্যা)});
            }
        }

        যদি (sigs.match_গুনতি_Overflowp) {
            m_modp->addStmtsp(
                নতুন AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_বৃক্ষ(মিথ্যা),
                              new_গুনতি_Overflow_Warning(c.flp, sigs.match_গুনতি_Overflowp)});
            sigs.match_গুনতি_Overflowp = nullptr;
        }

        ফেরত sigs;
    }

    // Phase 1: Resolve combinational Links via fixed-point propagation.
    শূন্য resolveLinks(LowerCtx& c, Ast_গ্রন্থি_রাশিমালা* triggerExprp, ধ্রুবক যৌক্তিক গুনতি_Matches) {
        // datap() was freshly allocated in lower() -- all state signals start null.
        c.graph.m_startVertexp->datap()->stateSigp = triggerExprp->clone_বৃক্ষ_Pure(মিথ্যা);
        c.graph.m_startVertexp->datap()->state_গুনতি_Sigp
            = নতুন AstExtend{c.flp, triggerExprp->clone_বৃক্ষ_Pure(মিথ্যা), m_u32DTypep->width()};
        জন্য (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ধ্রুবক vtxp = static_cast<SvaStateVertex*>(&vtxr);
            যদি (vtxp->datap()->stateVarp) {
                vtxp->datap()->stateSigp
                    = নতুন AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
                vtxp->datap()->state_গুনতি_Sigp = নতুন AstExtend{
                    c.flp, নতুন AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ},
                    m_u32DTypep->width()};
            } অন্যথা যদি (vtxp->datap()->delayRingVarp) {
                যদি (vtxp->m_isFixedDelayRing) {
                    // state_count = wrapped ? head : 0; state = state_count != 0;
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক countp = delay_Ring_Output_গুনতি(c.flp, vtxp);
                    vtxp->datap()->stateSigp = নতুন AstNeq{c.flp, countp->clone_বৃক্ষ_Pure(মিথ্যা),
                                                          newTypedConstp(c.flp, m_u32DTypep, 0)};
                    vtxp->datap()->state_গুনতি_Sigp = countp;
                } অন্যথা {
                    vtxp->datap()->stateSigp
                        = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_গুনতি_Varp);
                    vtxp->datap()->state_গুনতি_Sigp = নতুন AstVarRef{
                        c.flp, vtxp->datap()->delay_Ring_Live_গুনতি_Varp, VAccess::READ};
                }
            }
        }
        // Fixed-point propagation along zero-delay (Link) edges. Rebuild each
        // derived signal from its incoming edges on every pass; appending the same
        // contributions repeatedly makes merge expressions grow exponentially.
        ধ্রুবক আকার_t passes = 2 * c.graph.m_graph.vertices().আকার_2() + 2;
        জন্য (আকার_t pass = 0; pass < passes; ++pass) {
            // Rebuild SAnd combiners once both sub-NFA terminals are available.
            জন্য (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* ধ্রুবক vtxp = static_cast<SvaStateVertex*>(&vtxr);
                যদি (!vtxp->m_isAndCombiner) চালু;
                // AndCombiner vertices always have both terminal pointers set.
                UASSERT_OBJ(vtxp->m_andLhsTermp && vtxp->m_andRhsTermp, vtxp,
                            "AndCombiner vertex missing LHS/RHS terminal");
                ধ্রুবক SvaStateVertex* ধ্রুবক lhsTermp = vtxp->m_andLhsTermp;
                ধ্রুবক SvaStateVertex* ধ্রুবক rhsTermp = vtxp->m_andRhsTermp;
                যদি (!lhsTermp->datap()->stateSigp || !rhsTermp->datap()->stateSigp) চালু;
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক matchLp
                    = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, vtxp->m_andLhsCondp);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক matchRp
                    = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, vtxp->m_andRhsCondp);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক doneLOrp = নতুন AstLogOr{
                    c.flp, নতুন AstVarRef{c.flp, vtxp->datap()->doneLVarp, VAccess::READ}, matchLp};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক doneROrp = নতুন AstLogOr{
                    c.flp, নতুন AstVarRef{c.flp, vtxp->datap()->doneRVarp, VAccess::READ}, matchRp};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক bothp = নতুন AstLogAnd{c.flp, doneLOrp, doneROrp};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক oneNowp = নতুন AstLogOr{c.flp, matchLp->clone_বৃক্ষ_Pure(মিথ্যা),
                                                          matchRp->clone_বৃক্ষ_Pure(মিথ্যা)};
                যদি (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_বৃক্ষ(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = নতুন AstLogAnd{c.flp, bothp, oneNowp};
                যদি (vtxp->datap()->state_গুনতি_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_গুনতি_Sigp->delete_বৃক্ষ(),
                                   vtxp->datap()->state_গুনতি_Sigp);
                }
                vtxp->datap()->state_গুনতি_Sigp = নতুন AstExtend{
                    c.flp, vtxp->datap()->stateSigp->clone_বৃক্ষ_Pure(মিথ্যা), m_u32DTypep->width()};
            }

            জন্য (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* ধ্রুবক vtxp = static_cast<SvaStateVertex*>(&vtxr);
                যদি (vtxp == c.graph.m_startVertexp || vtxp->datap()->stateVarp
                    || vtxp->datap()->delayRingVarp || vtxp->m_isAndCombiner || vtxp->m_isMatch
                    || vtxp->m_isRejectSink) {
                    চালু;
                }
                Ast_গ্রন্থি_রাশিমালা* পরবর্তী_Statep = nullptr;
                Ast_গ্রন্থি_রাশিমালা* পরবর্তী_State_Countp = nullptr;
                Ast_গ্রন্থি_রাশিমালা* পরবর্তী_State_গুনতি_Overflowp = nullptr;
                জন্য (ধ্রুবক V3GraphEdge& er : vtxp->inEdges()) {
                    ধ্রুবক SvaTransEdge& te = static_cast<ধ্রুবক SvaTransEdge&>(er);
                    ধ্রুবক SvaStateVertex* ধ্রুবক fromVtxp = te.fromVtxp();
                    যদি (!fromVtxp->datap()->stateSigp) চালু;
                    UASSERT_OBJ(fromVtxp->datap()->state_গুনতি_Sigp, fromVtxp,
                                "Link source missing stateCountSig");
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক contributionp = andCond(
                        c.flp, fromVtxp->datap()->stateSigp->clone_বৃক্ষ_Pure(মিথ্যা), te.m_condp);
                    পরবর্তী_Statep = orExprs(c.flp, পরবর্তী_Statep, contributionp);
                    Ast_গ্রন্থি_রাশিমালা* গুনতি_Contributionp
                        = fromVtxp->datap()->state_গুনতি_Sigp->clone_বৃক্ষ_Pure(মিথ্যা);
                    Ast_গ্রন্থি_রাশিমালা* গুনতি_Contribution_Overflowp
                        = গুনতি_Matches && fromVtxp->datap()->state_গুনতি_Overflow_Sigp
                              ? fromVtxp->datap()->state_গুনতি_Overflow_Sigp->clone_বৃক্ষ_Pure(মিথ্যা)
                              : nullptr;
                    যদি (te.m_condp) {
                        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক enablep = te.m_condp->clone_বৃক্ষ_Pure(মিথ্যা);
                        গুনতি_Contributionp
                            = গুনতি_Matches
                                  ? gateCheckedCountp(c.flp, গুনতি_Contributionp,
                                                      গুনতি_Contribution_Overflowp, enablep)
                                  : addCountp(c.flp, nullptr, গুনতি_Contributionp, enablep);
                    }
                    পরবর্তী_State_Countp
                        = গুনতি_Matches
                              ? addCheckedCountp(c.flp, পরবর্তী_State_Countp, গুনতি_Contributionp,
                                                 পরবর্তী_State_গুনতি_Overflowp,
                                                 গুনতি_Contribution_Overflowp)
                              : addCountp(c.flp, পরবর্তী_State_Countp, গুনতি_Contributionp);
                }
                যদি (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_বৃক্ষ(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = পরবর্তী_Statep;
                যদি (vtxp->datap()->state_গুনতি_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_গুনতি_Sigp->delete_বৃক্ষ(),
                                   vtxp->datap()->state_গুনতি_Sigp);
                }
                vtxp->datap()->state_গুনতি_Sigp = পরবর্তী_State_Countp;
                যদি (vtxp->datap()->state_গুনতি_Overflow_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_গুনতি_Overflow_Sigp->delete_বৃক্ষ(),
                                   vtxp->datap()->state_গুনতি_Overflow_Sigp);
                }
                vtxp->datap()->state_গুনতি_Overflow_Sigp = পরবর্তী_State_গুনতি_Overflowp;
            }
        }
    }

সর্বজনীন:
    // Combine terminal/reject signals into final output expression.
    স্থির Ast_গ্রন্থি_রাশিমালা* assemble_ফলাফল(Ast_গ্রন্থি_Cover_Or_Assert* ধ্রুবক assertp, ধ্রুবক যৌক্তিক negated,
                                       Ast_গ্রন্থি_রাশিমালা* ধ্রুবক matchCondp, ধ্রুবক SignalSet& sigs,
                                       Ast_গ্রন্থি_রাশিমালা** ধ্রুবক outMatchpp) {
        File_সারি* ধ্রুবক flp = assertp->fileline();
        ধ্রুবক যৌক্তিক isCover = VN_IS(assertp, Cover);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক terminalActivep = sigs.terminalActivep;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rejectBasep = sigs.rejectBasep;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক throughoutRejectp = sigs.throughoutRejectp;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক requiredStepRejectp = sigs.requiredStepRejectp;
        // Property negation (IEEE 1800-2023 16.12.1 `not`): invert match/reject.
        যদি (negated) {
            যদি (isCover) {
                যদি (terminalActivep)
                    VL_DO_DANGLING(terminalActivep->delete_বৃক্ষ(), terminalActivep);
                Ast_গ্রন্থি_রাশিমালা* negRejectp = nullptr;
                যদি (matchCondp && rejectBasep) {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক sampledCondp = sampled(matchCondp->clone_বৃক্ষ_Pure(মিথ্যা));
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক notCondp = নতুন AstLogNot{flp, sampledCondp};
                    negRejectp = নতুন AstLogAnd{flp, rejectBasep, notCondp};
                } অন্যথা যদি (rejectBasep) {
                    VL_DO_DANGLING(rejectBasep->delete_বৃক্ষ(), rejectBasep);
                }
                যদি (throughoutRejectp) negRejectp = orExprs(flp, negRejectp, throughoutRejectp);
                যদি (requiredStepRejectp)
                    negRejectp = orExprs(flp, negRejectp, requiredStepRejectp);
                ফেরত negRejectp ? negRejectp : নতুন AstConst{flp, AstConst::BitFalse{}};
            }
            // Negated assert/assume: output = !match.
            Ast_গ্রন্থি_রাশিমালা* matchp = terminalActivep;
            যদি (matchCondp) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক sampledCondp = sampled(matchCondp->clone_বৃক্ষ_Pure(মিথ্যা));
                matchp = নতুন AstLogAnd{flp, matchp, sampledCondp};
            }
            যদি (outMatchpp) {
                Ast_গ্রন্থি_রাশিমালা* notPMatchp = nullptr;
                যদি (matchCondp && rejectBasep) {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক sampledCondp = sampled(matchCondp->clone_বৃক্ষ_Pure(মিথ্যা));
                    notPMatchp = নতুন AstLogAnd{flp, rejectBasep->clone_বৃক্ষ_Pure(মিথ্যা),
                                               নতুন AstLogNot{flp, sampledCondp}};
                } অন্যথা যদি (rejectBasep) {
                    notPMatchp = rejectBasep->clone_বৃক্ষ_Pure(মিথ্যা);
                }
                যদি (throughoutRejectp)
                    notPMatchp = orExprs(flp, notPMatchp, throughoutRejectp->clone_বৃক্ষ_Pure(মিথ্যা));
                যদি (requiredStepRejectp)
                    notPMatchp
                        = orExprs(flp, notPMatchp, requiredStepRejectp->clone_বৃক্ষ_Pure(মিথ্যা));
                *outMatchpp = notPMatchp;
            }
            যদি (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_বৃক্ষ(), throughoutRejectp);
            যদি (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_বৃক্ষ(), rejectBasep);
            যদি (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_বৃক্ষ(), requiredStepRejectp);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক ফলাফল_Exprp = নতুন AstLogNot{flp, matchp};
            ফেরত ফলাফল_Exprp;
        }
        যদি (isCover) {
            যদি (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_বৃক্ষ(), throughoutRejectp);
            যদি (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_বৃক্ষ(), rejectBasep);
            যদি (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_বৃক্ষ(), requiredStepRejectp);
            যদি (matchCondp) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক sampledCondp = sampled(matchCondp->clone_বৃক্ষ_Pure(মিথ্যা));
                ফেরত নতুন AstLogAnd{flp, terminalActivep, sampledCondp};
            }
            ফেরত terminalActivep;
        }
        // Assert/assume: output = !reject
        Ast_গ্রন্থি_রাশিমালা* rejectp = nullptr;
        যদি (matchCondp && rejectBasep) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক sampledCondp = sampled(matchCondp->clone_বৃক্ষ_Pure(মিথ্যা));
            rejectp = নতুন AstLogAnd{flp, rejectBasep, নতুন AstLogNot{flp, sampledCondp}};
        } অন্যথা যদি (rejectBasep) {
            VL_DO_DANGLING(rejectBasep->delete_বৃক্ষ(), rejectBasep);
        }
        যদি (outMatchpp) {
            Ast_গ্রন্থি_রাশিমালা* matchExprp = terminalActivep->clone_বৃক্ষ_Pure(মিথ্যা);
            যদি (matchCondp) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক sp = sampled(matchCondp->clone_বৃক্ষ_Pure(মিথ্যা));
                matchExprp = নতুন AstLogAnd{flp, matchExprp, sp};
            }
            *outMatchpp = matchExprp;
        }
        যদি (terminalActivep) VL_DO_DANGLING(terminalActivep->delete_বৃক্ষ(), terminalActivep);
        যদি (throughoutRejectp) rejectp = orExprs(flp, rejectp, throughoutRejectp);
        যদি (requiredStepRejectp) rejectp = orExprs(flp, rejectp, requiredStepRejectp);
        যদি (!rejectp) ফেরত নতুন AstConst{flp, AstConst::BitTrue{}};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক ফলাফল_Exprp = নতুন AstLogNot{flp, rejectp};
        ফেরত ফলাফল_Exprp;
    }

    explicit SvaNfaLowering(Ast_গ্রন্থি_Module* modp)
        : m_modp{modp}
        , m_u32DTypep{modp->findBasicDType(VBasicDTypeKwd::UINT32)} {}
    ~SvaNfaLowering() {
        V3Stats::add_Stat_যোগ("Assertions, NFA delay ring edge visits", m_statDelayRingEdgeVisits);
    }

    // Lower NFA graph to synthesizable AstAlways blocks and raw result signals.
    // Links are combinational; Edges are registered (NBA).
    SignalSet lower(Ast_গ্রন্থি_Cover_Or_Assert* ধ্রুবক assertp, SvaGraph& graph,
                    Ast_Sen_বৃক্ষ* ধ্রুবক senTreep, Ast_গ্রন্থি_রাশিমালা* ধ্রুবক matchCondp,
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক disableExprp, ধ্রুবক যৌক্তিক need_Thread_Fail_গুনতি,
                    ধ্রুবক যৌক্তিক need_Throughout_Thread_Fail_গুনতি) {
        File_সারি* ধ্রুবক flp = assertp->fileline();
        AstCover* ধ্রুবক coverp = VN_CAST(assertp, Cover);
        ধ্রুবক যৌক্তিক isSeqEvent = coverp && coverp->isSeqEvent();
        ধ্রুবক VAssertType assertType
            = isSeqEvent ? VAssertType{VAssertType::INTERNAL} : assertp->userType();
        ধ্রুবক VAssertDirectiveType directiveType
            = isSeqEvent ? VAssertDirectiveType{VAssertDirectiveType::INTERNAL}
                         : assertp->directive();
        ধ্রুবক std::string baseName = m_names.get("");

        // Number vertices with sequential colors for array indexing.
        পূর্ণসংখ্যা N = 0;
        জন্য (V3GraphVertex& vtxr : graph.m_graph.vertices()) { vtxr.color(N++); }
        std::vector<SvaStateVertex*> vtx(N, nullptr);
        জন্য (V3GraphVertex& vtxr : graph.m_graph.vertices()) {
            vtx[vtxr.color()] = static_cast<SvaStateVertex*>(&vtxr);
        }

        // Allocate per-vertex lowering data (stored via V3GraphVertex::userp()).
        std::vector<Sva_Vertex_তথ্য> vertex_তথ্য(N);
        জন্য (পূর্ণসংখ্যা i = 0; i < N; ++i) vtx[i]->userp(&vertex_তথ্য[i]);

        // Identify registered vertices (targets of clocked edges).
        জন্য (পূর্ণসংখ্যা i = 0; i < N; ++i) {
            জন্য (ধ্রুবক V3GraphEdge& edger : vtx[i]->outEdges()) {
                ধ্রুবক SvaTransEdge& tedger = static_cast<ধ্রুবক SvaTransEdge&>(edger);
                ধ্রুবক পূর্ণসংখ্যা toIdx = tedger.toVtxp()->color();
                যদি (tedger.m_consumesCycle && tedger.toVtxp() != graph.m_matchVertexp
                    && !tedger.toVtxp()->m_isRejectSink) {
                    vtx[toIdx]->datap()->needsReg = সত্য;
                }
            }
        }

        // A packed range ring can emit several live endpoints without needing per-slot counts.
        // Widen only a ring whose input can already carry multiplicity.
        std::vector<যৌক্তিক> stateMayHaveMultiplicity(N);
        std::vector<যৌক্তিক> ringNeedsMultiplicity(N);
        std::vector<পূর্ণসংখ্যা> multiplicityWork;
        ধ্রুবক যৌক্তিক গুনতি_Matches = coverp && coverp->isCoverSeq() && !isSeqEvent;
        যদি (গুনতি_Matches) {
            জন্য (পূর্ণসংখ্যা i = 0; i < N; ++i) {
                ধ্রুবক যৌক্তিক multipleInputs = vtx[i]->inEdges().আকার_2() > 1;
                ধ্রুবক যৌক্তিক isRing = vtx[i]->m_delay_Ring_আকার;
                ringNeedsMultiplicity[i] = isRing && multipleInputs;
                ধ্রুবক যৌক্তিক startsMultiplicity = isRing
                                                    ? !vtx[i]->m_isFixedDelayRing
                                                    : multipleInputs && !vtx[i]->datap()->needsReg;
                যদি (startsMultiplicity) {
                    stateMayHaveMultiplicity[i] = সত্য;
                    multiplicityWork.ঠেলো_back(i);
                }
            }
            জন্য (আকার_t wi = 0; wi < multiplicityWork.আকার_2(); ++wi) {
                জন্য (ধ্রুবক V3GraphEdge& edger : vtx[multiplicityWork[wi]]->outEdges()) {
                    ধ্রুবক SvaTransEdge& tedger = static_cast<ধ্রুবক SvaTransEdge&>(edger);
                    ধ্রুবক পূর্ণসংখ্যা ti = tedger.toVtxp()->color();
                    ধ্রুবক যৌক্তিক isRing = vtx[ti]->m_delay_Ring_আকার;
                    যদি (isRing) ringNeedsMultiplicity[ti] = সত্য;
                    // Match and reject sinks have no outgoing edges, so they stop naturally.
                    ধ্রুবক যৌক্তিক stopsMultiplicity = !isRing && vtx[ti]->datap()->needsReg;
                    যদি (stateMayHaveMultiplicity[ti] || stopsMultiplicity) { চালু; }
                    stateMayHaveMultiplicity[ti] = সত্য;
                    multiplicityWork.ঠেলো_back(ti);
                }
            }
        }

        ধ্রুবক যৌক্তিক asyncDisable = isAsyncDisable(disableExprp);
        // Registers cleared on a rising disable are also written by their clocked block.
        File_সারি* ধ্রুবক regFlp = asyncDisable ? নতুন File_সারি{flp} : flp;
        যদি (asyncDisable) regFlp->warnOff(V3ErrorCode::MULTIDRIVEN, সত্য);

        AstVar* ধ্রুবক killVarp
            = নতুন AstVar{flp, VVarType::MODULETEMP, baseName + "__kill", m_u32DTypep};
        killVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(killVarp);
        জন্য (পূর্ণসংখ্যা i = 0; i < N; ++i) {
            যদি (vtx[i]->m_isAndCombiner) {
                ধ্রুবক std::string base = baseName + "__a" + std::to_string(i);
                AstVar* ধ্রুবক lp = নতুন AstVar{regFlp, VVarType::MODULETEMP, base + "_doneL",
                                              m_modp->findBitDType()};
                lp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(lp);
                vtx[i]->datap()->doneLVarp = lp;
                AstVar* ধ্রুবক rp = নতুন AstVar{regFlp, VVarType::MODULETEMP, base + "_doneR",
                                              m_modp->findBitDType()};
                rp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(rp);
                vtx[i]->datap()->doneRVarp = rp;
                চালু;
            }
            যদি (vtx[i]->m_delay_Ring_আকার) {
                ধ্রুবক std::string base = baseName + "__d" + std::to_string(i);
                ধ্রুবক যৌক্তিক storesCounts = গুনতি_Matches && ringNeedsMultiplicity[i];
                Ast_গ্রন্থি_DType* ringDTypep = nullptr;
                যদি (storesCounts) {
                    // int unsigned ring [size];
                    ringDTypep = নতুন AstUnpackArrayDType{
                        flp, m_u32DTypep,
                        নতুন AstRange{flp, static_cast<পূর্ণসংখ্যা>(vtx[i]->m_delay_Ring_আকার - 1), 0}};
                    v3Global.rootp()->typeTablep()->addTypesp(ringDTypep);
                } অন্যথা {
                    // bit [size-1:0] ring;
                    ringDTypep = m_modp->findBitDType(vtx[i]->m_delay_Ring_আকার,
                                                      vtx[i]->m_delay_Ring_আকার, VSigning::UNSIGNED);
                }
                AstVar* ধ্রুবক ringp
                    = নতুন AstVar{flp, VVarType::MODULETEMP, base + "_ring", ringDTypep};
                ringp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(ringp);
                vtx[i]->datap()->delayRingVarp = ringp;
                vtx[i]->datap()->delayRingCounts = storesCounts;
                AstVar* ধ্রুবক headp
                    = নতুন AstVar{flp, VVarType::MODULETEMP, base + "_head",
                                 storesCounts ? m_u32DTypep : m_modp->findBitDType()};
                headp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(headp);
                vtx[i]->datap()->delay_Ring_শীর্ষ_Varp = headp;
                // int unsigned idx;
                AstVar* ধ্রুবক idxp
                    = নতুন AstVar{regFlp, VVarType::MODULETEMP, base + "_idx", m_u32DTypep};
                idxp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(idxp);
                vtx[i]->datap()->delayRingIdxVarp = idxp;
                // int unsigned live_count;
                AstVar* ধ্রুবক live_গুনতি_Varp
                    = নতুন AstVar{regFlp, VVarType::MODULETEMP, base + "_liveCount", m_u32DTypep};
                live_গুনতি_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(live_গুনতি_Varp);
                vtx[i]->datap()->delay_Ring_Live_গুনতি_Varp = live_গুনতি_Varp;
                AstVar* ধ্রুবক wrappedp = নতুন AstVar{regFlp, VVarType::MODULETEMP,
                                                    base + "_wrapped", m_modp->findBitDType()};
                wrappedp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(wrappedp);
                vtx[i]->datap()->delayRingWrappedVarp = wrappedp;
                চালু;
            }
            যদি (!vtx[i]->datap()->needsReg) চালু;
            যদি (vtx[i] == graph.m_startVertexp || vtx[i]->m_isMatch) চালু;
            ধ্রুবক std::string varName = baseName + "__s" + std::to_string(i);
            AstVar* ধ্রুবক varp
                = নতুন AstVar{regFlp, VVarType::MODULETEMP, varName, m_modp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(varp);
            vtx[i]->datap()->stateVarp = varp;
        }

        // Build lowering context for phase sub-functions.
        LowerCtx c{flp,        senTreep,      disableExprp, matchCondp,
                   assertType, directiveType, killVarp,     graph};

        // Phase 1: Resolve combinational Links via fixed-point propagation.
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক triggerExprp
            = isSeqEvent ? নতুন AstConst{flp, AstConst::BitTrue{}}
                         : assertOnCond(flp, assertp->userType(), assertp->directive());
        resolveLinks(c, triggerExprp, গুনতি_Matches);
        VL_DO_DANGLING(triggerExprp->delete_বৃক্ষ(), triggerExprp);

        // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
        emitStateRegisterNba(c);
        emitDelayRingUpdates(c, গুনতি_Matches);
        emitAndCombinerDoneLatchNba(c);
        emitKillAckNba(c);

        // Phase 3/3a/3b: Compute terminal match/reject signals (cleans up stateSig).
        ধ্রুবক SignalSet sigs
            = computeSignals(c, need_Thread_Fail_গুনতি, need_Throughout_Thread_Fail_গুনতি, গুনতি_Matches);

        // Strong s_always[m:n] end-of-simulation liveness: if any in-window state
        // is still set at $finish, the universal-quantifier window never completed
        // (IEEE 1800-2023 16.12.11 strong semantics). Fire the assertion failure
        // from a final block; V3Assert turns the DT_ERROR display into the standard
        // "Assertion failed in %m" message.
        // pending = |{strong state registers, strong delay live counts != 0};
        Ast_গ্রন্থি_রাশিমালা* pendingp = nullptr;
        জন্য (পূর্ণসংখ্যা i = 0; i < N; ++i) {
            যদি (!vtx[i]->m_strongPending) চালু;
            Ast_গ্রন্থি_রাশিমালা* pendingExprp = nullptr;
            যদি (vtx[i]->datap()->stateVarp) {
                pendingExprp = নতুন AstVarRef{flp, vtx[i]->datap()->stateVarp, VAccess::READ};
            } অন্যথা যদি (vtx[i]->m_strongAlwaysRing) {
                pendingExprp
                    = delayRingHasLiveThreadsp(flp, vtx[i]->datap()->delay_Ring_Live_গুনতি_Varp);
            } অন্যথা {
                চালু;
            }
            যদি (!pendingp) {
                pendingp = pendingExprp;
            } অন্যথা {
                pendingp = নতুন AstLogOr{flp, pendingp, pendingExprp};
            }
        }
        যদি (pendingp) {
            AstCExpr* ধ্রুবক assertOnp
                = নতুন AstCExpr{flp, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()", 1};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক condp = নতুন AstLogAnd{flp, assertOnp, pendingp};
            AstDisplay* ধ্রুবক dispp
                = নতুন AstDisplay{flp, VDisplayType::DT_ERROR, "", nullptr, nullptr};
            dispp->fmtp()->timeunit(m_modp->timeunit());
            Ast_গ্রন্থি_Stmt* ধ্রুবক firep = dispp;
            যদি (v3Global.opt.stopFail()) firep->add_পরবর্তী(নতুন AstStop{flp, মিথ্যা});
            m_modp->addStmtsp(নতুন AstFinal{flp, নতুন AstIf{flp, condp, firep}});
        }

        // Clear userp on every vertex before vertexData is destroyed.
        জন্য (পূর্ণসংখ্যা i = 0; i < N; ++i) vtx[i]->userp(nullptr);
        ফেরত sigs;
    }
};

}  // namespace

//######################################################################
// Top-level visitor

শ্রেণী AssertNfaVisitor final : সর্বজনীন VNVisitor {
    // STATE
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module being processed
    AstClocking* m_defaultClockingp = nullptr;  // Default clocking
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff
    SvaNfaLowering* m_loweringp = nullptr;  // NFA-to-hardware lowering engine
    Ast_Sen_বৃক্ষ* m_sampled_মান_Clockp = nullptr;  // Inherited clock during scoped attachment
    V3UniqueNames m_propVarNames{"__Vpropvar"};  // Property-local variable names
    V3UniqueNames m_prop_অস্থায়ী_Names{"__VnfaSampled"};  // Hoisted $sampled(propp) temps
    V3UniqueNames m_fail_গুনতি_Names{"__VnfaRemainingFailCount"};  // Fail replay counter names
    std::set<ধ্রুবক AstProperty*> m_inliningProps;  // Recursion guard for inlineNamedProperty

    template <typename T_গ্রন্থি>
    শূন্য visit_Sampled_মান(T_গ্রন্থি* ধ্রুবক nodep) {
        যদি (m_sampled_মান_Clockp && !nodep->sentreep()) {
            nodep->sentreep(m_sampled_মান_Clockp->clone_বৃক্ষ(সত্য));
        }
        iterateChildren(nodep);
    }

    // Wire match vertex and mid-window sources for a successful NFA build.
    স্থির শূন্য wireMatchAndMidSources(SvaGraph& graph, ধ্রুবক Build_ফলাফল& ফলাফল, File_সারি* flp) {
        graph.createMatchVertex();
        // Skip the main term Link when midSources already cover every
        // end-of-match (cover_sequence path); otherwise the per-mid extraction
        // double-counts via the merge vertex.
        যদি (!ফলাফল.termIsMidMerge) { graph.addLink(ফলাফল.termVertexp, graph.m_matchVertexp); }
        জন্য (SvaStateVertex* srcVtxp : ফলাফল.midSources) {
            Ast_গ্রন্থি_রাশিমালা* condp = nullptr;
            জন্য (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক tc : srcVtxp->m_throughoutConds) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক tcClone = tc->clone_বৃক্ষ_Pure(মিথ্যা);
                condp = condp ? নতুন AstLogAnd{flp, condp, tcClone} : tcClone;
            }
            graph.addLink(srcVtxp, graph.m_matchVertexp, condp);
            srcVtxp->m_isUnbounded = সত্য;
        }
    }

    স্থির Ast_গ্রন্থি_রাশিমালা* getSequenceBodyExprp(ধ্রুবক AstSequence* seqp) {
        Ast_গ্রন্থি* bodyp = seqp->stmtsp();
        যখন (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        ফেরত VN_CAST(bodyp, গ্রন্থি_রাশিমালা);
    }

    স্থির AstPropSpec* getPropertySpecp(ধ্রুবক AstProperty* propp) {
        Ast_গ্রন্থি* stmtp = propp->stmtsp();
        // V3LinkParse emits InitialStaticStmt for property-local variable
        // initialisers; the InitialAutomaticStmt variant only appears for
        // task/function-scope automatic lifetime, not properties.
        যখন (stmtp
               && (VN_IS(stmtp, Var) || VN_IS(stmtp, InitialStaticStmt)
                   || VN_IS(stmtp, InitialAutomaticStmt))) {  // LCOV_EXCL_LINE
            stmtp = stmtp->nextp();
        }
        ফেরত VN_CAST(stmtp, PropSpec);
    }

    শূন্য inlineNamedProperty(AstPropSpec* outerSpecp, AstFuncRef* funcrefp,
                             ধ্রুবক AstProperty* propyp) {
        // Recursion guard: IEEE 1800-2023 16.12.1 forbids recursive properties.
        // V3Width emits "Recursive property call" for direct recursion before this
        // pass runs; this catches any nested-inlining cycle that slips past.
        যদি (!m_inliningProps.emplace(propyp).second) {
            funcrefp->v3error("Illegal recursive property reference");  // LCOV_EXCL_LINE
            ফেরত;  // LCOV_EXCL_LINE
        }
        কাঠামো Guard final {
            std::set<ধ্রুবক AstProperty*>& setr;
            ধ্রুবক AstProperty* keyp;
            ~Guard() { setr.erase(keyp); }
        } guard{m_inliningProps, propyp};
        AstPropSpec* propSpecp = getPropertySpecp(propyp);
        UASSERT_OBJ(propSpecp, funcrefp, "Property has no body PropSpec");
        propSpecp = propSpecp->clone_বৃক্ষ(মিথ্যা);

        ধ্রুবক V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propyp->stmtsp());
        std::unordered_map<ধ্রুবক AstVar*, Ast_গ্রন্থি_রাশিমালা*> portMap;
        জন্য (ধ্রুবক auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables to module-level temps (IEEE 16.10).
        std::unordered_map<ধ্রুবক AstVar*, AstVar*> localVarMap;
        জন্য (Ast_গ্রন্থি* stmtp = propyp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstVar* ধ্রুবক varp = VN_CAST(stmtp, Var)) {
                যদি (!varp->isIO()) {
                    ধ্রুবক string newName = m_propVarNames.get(varp);
                    AstVar* ধ্রুবক newVarp = নতুন AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        propSpecp->foreach([&](AstVarRef* refp) {
            ধ্রুবক auto portIt = portMap.find(refp->varp());
            যদি (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_বৃক্ষ(মিথ্যা));
                VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
                ফেরত;
            }
            ধ্রুবক auto localIt = localVarMap.find(refp->varp());
            যদি (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        জন্য (ধ্রুবক auto& tconnect : tconnects) {
            ঠেলো_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propSpecp, funcrefp);

        // Merge disable iff (IEEE 1800-2023 16.12.1)
        যদি (outerSpecp->disablep() && propSpecp->disablep()) {
            outerSpecp->v3error("disable iff expression before property call "
                                "and in its body is not legal");
            ঠেলো_Deletep(propSpecp->disablep()->unlinkFrBack());
        }
        যদি (outerSpecp->disablep()) {
            propSpecp->disablep(outerSpecp->disablep()->unlinkFrBack());
        }

        যদি (outerSpecp->sensesp() && propSpecp->sensesp()) {
            outerSpecp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Clock event before property call and in its body");
            ঠেলো_Deletep(propSpecp->sensesp()->unlinkFrBack());
        }
        যদি (outerSpecp->sensesp()) {
            AstSenItem* ধ্রুবক sensesp = outerSpecp->sensesp();
            sensesp->unlinkFrBack();
            propSpecp->sensesp(sensesp);
        }

        outerSpecp->replaceWith(propSpecp);
        VL_DO_DANGLING(ঠেলো_Deletep(outerSpecp), outerSpecp);
    }

    শূন্য inlineSequenceRef(AstFuncRef* funcrefp, AstSequence* seqp) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক clonedp = bodyExprp->clone_বৃক্ষ(মিথ্যা);

        ধ্রুবক V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ধ্রুবক AstVar*, Ast_গ্রন্থি_রাশিমালা*> portMap;
        জন্য (ধ্রুবক auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ধ্রুবক auto it = portMap.find(refp->varp());
            যদি (it != portMap.end()) {
                refp->replaceWith(it->second->clone_বৃক্ষ(মিথ্যা));
                VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
            }
        });
        জন্য (ধ্রুবক auto& tconnect : tconnects) {
            ঠেলো_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ঠেলো_Deletep(funcrefp), funcrefp);
        // Clear referenced flag so V3AssertPre cleanup does not emit
        // spurious UNSUPPORTED for sequences that were already inlined here.
        seqp->isReferenced(মিথ্যা);
    }

    // Must run before hasMultiCycleExpr() so NFA sees sequence bodies.
    শূন্য inlineAllSequenceRefs(Ast_গ্রন্থি* rootp) {
        যৌক্তিক changed = সত্য;
        যখন (changed) {
            changed = মিথ্যা;
            rootp->foreach([&](AstFuncRef* funcrefp) {
                যদি (changed) ফেরত;
                যদি (AstSequence* ধ্রুবক seqp = VN_CAST(funcrefp->taskp(), Sequence)) {
                    inlineSequenceRef(funcrefp, seqp);
                    changed = সত্য;
                }
            });
        }
    }

    স্থির যৌক্তিক has_Multi_Cycle_রাশিমালা(ধ্রুবক Ast_গ্রন্থি* nodep) {
        ফেরত nodep->exists([](ধ্রুবক Ast_গ্রন্থি* np) {
            যদি (ধ্রুবক auto* ধ্রুবক ep = VN_CAST(np, গ্রন্থি_রাশিমালা)) ফেরত ep->isMultiCycleSva();
            ফেরত মিথ্যা;
        });
    }

    স্থির VPropStrength effectiveAssertPropStrength(ধ্রুবক AstPropSpec* ধ্রুবক propSpecp) {
        যদি (propSpecp->propStrength() != VPropStrength::DEFAULT) ফেরত propSpecp->propStrength();
        ফেরত propSpecp->fileline()->language() <= V3LangCode::L1800_2005 ? VPropStrength::STRONG
                                                                           : VPropStrength::WEAK;
    }

    // Bare `assert property (p until q)` with boolean operands stays on
    // V3AssertPre's AstLoop lowering, which preserves per-attempt action-block
    // firings that this NFA's single-bit aggregated state cannot. Strong bare
    // forms are also lowered there. NFA still owns sequence operands and any
    // embedding inside a multi-cycle context (implication consequent, or/and
    // operands, etc.).
    স্থির যৌক্তিক is_Bare_শীর্ষ_Level_Until(Ast_গ্রন্থি* propp) {
        Ast_গ্রন্থি* p = propp;
        যদি (AstPropSpec* ধ্রুবক specp = VN_CAST(p, PropSpec)) p = specp->propp();
        যখন (AstLogNot* ধ্রুবক notp = VN_CAST(p, LogNot)) p = notp->lhsp();
        AstUntil* ধ্রুবক untilp = VN_CAST(p, Until);
        যদি (!untilp) ফেরত মিথ্যা;
        ধ্রুবক auto hasSeq = [](ধ্রুবক Ast_গ্রন্থি_রাশিমালা* ep) {
            ফেরত ep->exists([](ধ্রুবক Ast_গ্রন্থি_রাশিমালা* np) { ফেরত np->isMultiCycleSva(); });
        };
        যদি (hasSeq(untilp->lhsp()) || hasSeq(untilp->rhsp())) ফেরত মিথ্যা;
        ফেরত সত্য;
    }

    কাঠামো PropertyParts final {
        Ast_গ্রন্থি_রাশিমালা* triggerExprp = nullptr;
        Ast_গ্রন্থি_রাশিমালা* seqExprp = nullptr;
        যৌক্তিক isOverlapped = সত্য;
        যৌক্তিক hasImplication = মিথ্যা;
        যৌক্তিক isFollowedBy = মিথ্যা;  // True for #-# / #=# (non-vacuous-fail on antecedent miss)
    };

    স্থির PropertyParts decomposeProperty(Ast_গ্রন্থি* propp) {
        PropertyParts parts;
        যদি (AstPropSpec* ধ্রুবক specp = VN_CAST(propp, PropSpec)) { propp = specp->propp(); }
        যদি (AstImplication* ধ্রুবক implp = VN_CAST(propp, Implication)) {
            parts.hasImplication = সত্য;
            parts.isOverlapped = implp->isOverlapped();
            parts.isFollowedBy = implp->isFollowedBy();
            parts.triggerExprp = implp->lhsp();
            parts.seqExprp = implp->rhsp();
        } অন্যথা যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp = VN_CAST(propp, গ্রন্থি_রাশিমালা)) {
            parts.triggerExprp = nullptr;
            parts.seqExprp = exprp;
        }
        ফেরত parts;
    }

    স্থির যৌক্তিক canSplitImplicationPassActions(ধ্রুবক PropertyParts& parts) {
        UASSERT(parts.hasImplication,
                "Implication pass action split requested without implication");
        UASSERT(parts.triggerExprp, "Implication pass action split requested without trigger");
        // Direct vacuous/nonvacuous classification uses the antecedent value in the current
        // assertion attempt. Leave delayed antecedents on the existing NFA pass path.
        ফেরত !has_Multi_Cycle_রাশিমালা(parts.triggerExprp);
    }

    স্থির শূন্য splitImplicationPassActions(AstAssert* assertp, ধ্রুবক PropertyParts& parts,
                                            Ast_গ্রন্থি_রাশিমালা* nonvacuousMatchp) {
        File_সারি* ধ্রুবক flp = assertp->fileline();

        Ast_গ্রন্থি* ধ্রুবক passsp = assertp->passsp()->unlink_Fr_Back_With_পরবর্তী();
        Ast_গ্রন্থি* splitsp = nullptr;

        যদি (!parts.isFollowedBy) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক vacuousp
                = নতুন AstLogNot{flp, sampled(parts.triggerExprp->clone_বৃক্ষ_Pure(মিথ্যা))};
            Ast_গ্রন্থি* ধ্রুবক vacuousBodyp = passsp->clone_বৃক্ষ(মিথ্যা);
            splitsp = newPassOnIf(flp, vacuousp, vacuousBodyp, assertp->userType(),
                                  assertp->directive(), /*vacuous=*/সত্য);
        }

        AstIf* ধ্রুবক nonvacuousp
            = newPassOnIf(flp, nonvacuousMatchp, passsp, assertp->userType(), assertp->directive(),
                          /*vacuous=*/মিথ্যা);
        splitsp = splitsp ? Ast_গ্রন্থি::add_পরবর্তী<Ast_গ্রন্থি, Ast_গ্রন্থি>(splitsp, nonvacuousp)
                          : static_cast<Ast_গ্রন্থি*>(nonvacuousp);
        assertp->addPasssp(splitsp);
    }

    // On a PropSpec-wrapped assertion whose NFA build failed with a semantic
    // error (errorEmitted), replace the body with a BitFalse const so later
    // passes see a well-formed AST. Returns true if replaced.
    শূন্য replaceBodyOnBuildError(File_সারি* flp, AstPropSpec* propSpecp, যৌক্তিক errorEmitted) {
        যদি (!errorEmitted) ফেরত;
        Ast_গ্রন্থি* ধ্রুবক innerPropp = propSpecp->propp();
        innerPropp->replaceWith(নতুন AstConst{flp, AstConst::BitFalse{}});
        VL_DO_DANGLING(ঠেলো_Deletep(innerPropp), innerPropp);
    }

    // Hoist a leading clocking event (IEEE 1800-2023 16.7):
    যৌক্তিক hoistClockedSeq(AstPropSpec* specp) {
        যখন (AstSClocked* ধ্রুবক clockedp = VN_CAST(specp->propp(), SClocked)) {
            যদি (specp->sensesp()) {
                clockedp->v3warn(E_UNSUPPORTED, "Unsupported: multiclocked sequence or property");
                replaceBodyOnBuildError(specp->fileline(), specp, সত্য);
                ফেরত সত্য;
            }
            জন্য (ধ্রুবক AstSenItem* sp = clockedp->sensesp(); sp;
                 sp = VN_CAST(sp->nextp(), SenItem)) {
                যদি (!sp->edgeType().anEdge()) {
                    clockedp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: non-edge clocking event on a sequence; "
                                     "use an edge such as @(posedge clk)");
                    replaceBodyOnBuildError(specp->fileline(), specp, সত্য);
                    ফেরত সত্য;
                }
            }
            specp->sensesp(clockedp->sensesp()->unlink_Fr_Back_With_পরবর্তী());
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক bodyp = clockedp->exprp()->unlinkFrBack();
            clockedp->replaceWith(bodyp);
            VL_DO_DANGLING(ঠেলো_Deletep(clockedp), clockedp);
        }
        // A clocking event anywhere else in the sequence is not supported.
        ধ্রুবক AstSClocked* nestedp = nullptr;
        specp->propp()->foreach([&](ধ্রুবক AstSClocked* p) {
            যদি (!nestedp) nestedp = p;
        });
        যদি (nestedp) {
            nestedp->v3warn(E_UNSUPPORTED,
                            "Unsupported: clocking event inside sequence expression");
            replaceBodyOnBuildError(specp->fileline(), specp, সত্য);
            ফেরত সত্য;
        }
        ফেরত মিথ্যা;
    }

    // Build the NFA graph for a property body, handling both the antecedent
    // |-> consequent and simple sequence cases. Returns the consequent/body
    // BuildResult (invalid on parse/build failure).
    Build_ফলাফল buildAssertionGraph(SvaNfaBuilder& builder, SvaGraph& graph, Ast_গ্রন্থি_রাশিমালা* seqBodyp,
                                    ধ্রুবক PropertyParts& parts, File_সারি* flp) {
        যদি (!parts.hasImplication) ফেরত builder.build(seqBodyp);

        graph.m_startVertexp = graph.createStateVertex();
        ফেরত builder.buildImplicationEdges(parts.triggerExprp, seqBodyp, graph.m_startVertexp,
                                             parts.isOverlapped, parts.isFollowedBy,
                                             parts.triggerExprp, flp);
    }

    // Install pass-action gating and replay simultaneous per-thread failures.
    শূন্য attachActionHandlers(AstAssert* ধ্রুবক assertp, Ast_গ্রন্থি_রাশিমালা* matchExprp,
                              Ast_Sen_বৃক্ষ* ধ্রুবক threadFailReplaySenTreep,
                              Ast_গ্রন্থি_রাশিমালা* ধ্রুবক threadFailCountp) {
        // Gate pass handler on match to prevent vacuous-pass firings.
        যদি (matchExprp) {
            // needMatch implies passsp() was non-null when evaluated above;
            // lowering does not mutate the assert's pass-action between the
            // two reads, so passsp() is still non-null here.
            Ast_গ্রন্থি* passsp = assertp->passsp();
            UASSERT_OBJ(passsp, assertp, "needMatch set but passsp is null");
            passsp->unlink_Fr_Back_With_পরবর্তী();
            File_সারি* ধ্রুবক flp = assertp->fileline();
            AstIf* ধ্রুবক ifp = নতুন AstIf{flp, matchExprp, passsp, nullptr};
            assertp->addPasssp(ifp);
            // Fail-handler prefix for overlapping instances (IEEE 16.12):
            // fires when reject=1 && match=1 in the same cycle.
            যদি (assertp->failsp()) assertp->prependFailsp(ifp->clone_বৃক্ষ(মিথ্যা));
        }

        যদি (threadFailCountp) {
            UASSERT_OBJ(threadFailReplaySenTreep, assertp,
                        "Thread fail count missing sensitivity tree");
            Ast_গ্রন্থি* ধ্রুবক failsp = assertp->failsp();
            File_সারি* ধ্রুবক flp = assertp->fileline();
            // IEEE 1800-2023 16.12 requires one action-block evaluation per failed
            // thread. AstAssert handles the first, so replay the rest here.
            AstVar* ধ্রুবক remaining_Fail_গুনতি_Varp
                = নতুন AstVar{flp, VVarType::MODULETEMP, m_fail_গুনতি_Names.get(""),
                             m_modp->findBasicDType(VBasicDTypeKwd::UINT32)};
            remaining_Fail_গুনতি_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remaining_Fail_গুনতি_Varp);
            Ast_গ্রন্থি* ধ্রুবক replayStmtsp = নতুন AstAssign{
                flp, নতুন AstVarRef{flp, remaining_Fail_গুনতি_Varp, VAccess::WRITE}, threadFailCountp};
            AstLoop* ধ্রুবক replayLoopp = নতুন AstLoop{flp};
            replayLoopp->addStmtsp(নতুন AstLoopTest{
                flp, replayLoopp,
                নতুন AstGt{flp, নতুন AstVarRef{flp, remaining_Fail_গুনতি_Varp, VAccess::READ},
                          newTypedConstp(flp, remaining_Fail_গুনতি_Varp->dtypep(), 1)}});
            replayLoopp->addStmtsp(newIfAssertFailOn(failsp->clone_বৃক্ষ(সত্য), assertp->directive(),
                                                     assertp->userType()));
            AstSub* ধ্রুবক decrementedFailCountp
                = নতুন AstSub{flp, নতুন AstVarRef{flp, remaining_Fail_গুনতি_Varp, VAccess::READ},
                             newTypedConstp(flp, remaining_Fail_গুনতি_Varp->dtypep(), 1)};
            replayLoopp->addStmtsp(
                নতুন AstAssign{flp, নতুন AstVarRef{flp, remaining_Fail_গুনতি_Varp, VAccess::WRITE},
                              decrementedFailCountp});
            replayStmtsp->add_পরবর্তী(replayLoopp);
            m_modp->addStmtsp(
                নতুন AstAlways{flp, VAlwaysKwd::ALWAYS, threadFailReplaySenTreep, replayStmtsp});
        }
    }

    // Replace one VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). No-op if refp is not in matchMap.
    শূন্য substituteMatchItemRef(AstVarRef* refp, unsigned K,
                                ধ্রুবক std::unordered_map<ধ্রুবক AstVar*, Ast_গ্রন্থি_রাশিমালা*>& matchMap) {
        ধ্রুবক auto it = matchMap.find(refp->varp());
        যদি (it == matchMap.end()) ফেরত;
        Ast_গ্রন্থি_রাশিমালা* newp = it->second->clone_বৃক্ষ_Pure(মিথ্যা);
        যদি (K > 0) {
            AstConst* ধ্রুবক ticksp = নতুন AstConst{refp->fileline(), AstConst::Widthed_মান{}, 32,
                                                  static_cast<uint32_t>(K)};
            AstPast* ধ্রুবক pastp = নতুন AstPast{refp->fileline(), newp, ticksp, nullptr,
                                               /* propertyTiming */ সত্য};
            pastp->dtypeFrom(newp);
            newp = pastp;
        }
        refp->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
        ফেরত;
    }

    // Recursively walk a consequent. Returns cycle length consumed and
    // substitutes each VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). Reports E_UNSUPPORTED on non-constant
    // delays or composite sequence operators and returns -1 on failure.
    পূর্ণসংখ্যা
    walk_Substitute_Match_বস্তুসমূহ(Ast_গ্রন্থি_রাশিমালা* nodep, unsigned K,
                             ধ্রুবক std::unordered_map<ধ্রুবক AstVar*, Ast_গ্রন্থি_রাশিমালা*>& match_বস্তুসমূহ) {
        যদি (AstSExpr* ধ্রুবক sexprp = VN_CAST(nodep, SExpr)) {
            // IEEE 1800-2023 16.9.2: cycle_delay's lhsp is a constant_expression
            // and the delay form in a sequence is always `##N`, folded by
            // V3Const + V3Param before V3AssertNfa. Range form `##[m:n]` is the
            // only user-visible reject here.
            AstDelay* ধ্রুবক delayp = VN_AS(sexprp->delayp(), Delay);
            UASSERT_OBJ(delayp->isCycleDelay() && VN_IS(delayp->lhsp(), Const), sexprp,
                        "SVA cycle delay must have a constant lhsp");
            যদি (delayp->isRangeDelay()) {
                sexprp->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                              "non-constant cycle delay in consequent"
                                              " (IEEE 1800-2023 16.10)");
                ফেরত -1;
            }
            ধ্রুবক unsigned delayCycles = VN_AS(delayp->lhsp(), Const)->toUInt();
            পূর্ণসংখ্যা preLen = 0;
            যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক prep = sexprp->preExprp()) {
                preLen = walk_Substitute_Match_বস্তুসমূহ(prep, K, match_বস্তুসমূহ);
                যদি (preLen < 0) ফেরত -1;
            }
            ধ্রুবক পূর্ণসংখ্যা bodyLen
                = walk_Substitute_Match_বস্তুসমূহ(sexprp->exprp(), K + preLen + delayCycles, match_বস্তুসমূহ);
            যদি (bodyLen < 0) ফেরত -1;
            ফেরত preLen + delayCycles + bodyLen;
        }
        যদি (nodep->isMultiCycleSva()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                         "composite sequence operator in consequent"
                                         " (IEEE 1800-2023 16.10)");
            ফেরত -1;
        }
        std::vector<AstVarRef*> refs;
        nodep->foreach([&refs](AstVarRef* p) { refs.ঠেলো_back(p); });
        জন্য (AstVarRef* ধ্রুবক refp : refs) substituteMatchItemRef(refp, K, match_বস্তুসমূহ);
        ফেরত 0;
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
    যৌক্তিক liftMatchItemSubstitutions(PropertyParts& parts, Ast_গ্রন্থি_রাশিমালা* seqBodyp) {
        যদি (!parts.hasImplication) ফেরত মিথ্যা;
        Ast_রাশিমালা_Stmt* ধ্রুবক রাশিমালা_Stmtp = VN_CAST(parts.triggerExprp, রাশিমালা_Stmt);
        যদি (!রাশিমালা_Stmtp) ফেরত মিথ্যা;
        // IEEE 1800-2023 16.10 BNF requires `(expr, match_item {, match_item})`
        // with at least one match item; V3LinkParse only emits ExprStmt for
        // this form and only emits AstAssign with VarRef LHS for each item.
        std::unordered_map<ধ্রুবক AstVar*, Ast_গ্রন্থি_রাশিমালা*> match_বস্তুসমূহ;
        জন্য (Ast_গ্রন্থি* stmtp = রাশিমালা_Stmtp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstAssign* ধ্রুবক assignp = VN_AS(stmtp, Assign);
            AstVarRef* ধ্রুবক lhsRefp = VN_AS(assignp->lhsp(), VarRef);
            match_বস্তুসমূহ[lhsRefp->varp()] = assignp->rhsp();
        }
        ধ্রুবক unsigned startK = parts.isOverlapped ? 0 : 1;
        ধ্রুবক পূর্ণসংখ্যা দৈর্ঘ্য = walk_Substitute_Match_বস্তুসমূহ(seqBodyp, startK, match_বস্তুসমূহ);
        // Match-item substitution / strip mutates ancestor purity. Release
        // builds don't auto-clear caches on edits, so refresh here.
        VIsCached::clear_Cache_বৃক্ষ();
        যদি (দৈর্ঘ্য < 0) ফেরত সত্য;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক antBoolp = রাশিমালা_Stmtp->resultp()->unlinkFrBack();
        রাশিমালা_Stmtp->replaceWith(antBoolp);
        VL_DO_DANGLING(ঠেলো_Deletep(রাশিমালা_Stmtp), রাশিমালা_Stmtp);
        parts.triggerExprp = antBoolp;
        ফেরত মিথ্যা;
    }

    // Outcome counts for a property if/case are wrong in an outcome-multiplying
    // context. Returns that context, or nullptr when the shape is supported.
    স্থির ধ্রুবক অক্ষর* unsupportedPropertyControl(ধ্রুবক Ast_গ্রন্থি_Cover_Or_Assert* assertp,
                                                  ধ্রুবক Ast_গ্রন্থি_রাশিমালা* seqBodyp, যৌক্তিক negated) {
        যদি (!hasPropertyControlConjunction(seqBodyp)) ফেরত nullptr;
        যদি (negated) ফেরত "negation";
        যদি (VN_IS(assertp, Cover)) ফেরত "cover";
        যদি (VN_AS(assertp, Assert)->passsp()) ফেরত "a pass action";
        ফেরত nullptr;
    }

    // Inline property/sequence refs and reject unsupported shapes.
    // Returns the PropSpec to lower, or nullptr when fully handled here.
    AstPropSpec* prepareAssertionProp(Ast_গ্রন্থি_Cover_Or_Assert* assertp) {
        যদি (AstPropSpec* ধ্রুবক specp = VN_CAST(assertp->propp(), PropSpec)) {
            যদি (AstFuncRef* ধ্রুবক funcrefp = VN_CAST(specp->propp(), FuncRef)) {
                যদি (ধ্রুবক AstProperty* ধ্রুবক propyp = VN_CAST(funcrefp->taskp(), Property)) {
                    inlineNamedProperty(specp, funcrefp, propyp);
                }
            }
        }

        inlineAllSequenceRefs(assertp->propp());

        যদি (AstPropSpec* ধ্রুবক specp = VN_CAST(assertp->propp(), PropSpec)) {
            যদি (hoistClockedSeq(specp)) ফেরত nullptr;
        }

        AstPropSpec* ধ্রুবক propp = VN_AS(assertp->propp(), PropSpec);
        যদি (!VN_IS(assertp, Cover)
            && effectiveAssertPropStrength(propp) == VPropStrength::STRONG) {
            propp->v3warn(E_UNSUPPORTED,
                          "Unsupported: strong property in " + assertp->verilogKwd() + ".");
            replaceBodyOnBuildError(assertp->fileline(), propp, /*errorEmitted=*/সত্য);
            ফেরত nullptr;
        }

        যদি (!has_Multi_Cycle_রাশিমালা(propp)) ফেরত nullptr;
        // A nested property instance keeps its body behind the call; lowering would drop it.
        যদি (propp->exists([](ধ্রুবক AstFuncRef* refp) { ফেরত VN_IS(refp->taskp(), Property); })) {
            assertp->v3warn(E_UNSUPPORTED,
                            "Unsupported: property instance inside a multi-cycle property "
                            "expression");
            VL_DO_DANGLING(ঠেলো_Deletep(assertp->unlinkFrBack()), assertp);
            ফেরত nullptr;
        }
        যদি (is_Bare_শীর্ষ_Level_Until(propp)) ফেরত nullptr;
        ফেরত propp;
    }

    শূন্য processAssertion(Ast_গ্রন্থি_Cover_Or_Assert* assertp) {
        যদি (assertp->immediate()) ফেরত;

        AstPropSpec* ধ্রুবক propp = prepareAssertionProp(assertp);
        যদি (!propp) ফেরত;

        PropertyParts parts = decomposeProperty(propp);
        UASSERT_OBJ(parts.seqExprp, propp, "Property body must be an expression");

        // Unwrap `not` (IEEE 1800-2023 16.12.1); odd count -> negated semantics.
        Ast_গ্রন্থি_রাশিমালা* seqBodyp = parts.seqExprp;
        যৌক্তিক negated = মিথ্যা;
        যখন (AstLogNot* ধ্রুবক notp = VN_CAST(seqBodyp, LogNot)) {
            negated = !negated;
            seqBodyp = notp->lhsp();
        }

        ধ্রুবক অক্ষর* ধ্রুবক propertyControlp
            = unsupportedPropertyControl(assertp, seqBodyp, negated);

        // Substitute property-local match-item refs in consequent with
        // $past(rhs, K) before NFA build (IEEE 1800-2023 16.10).
        যদি (liftMatchItemSubstitutions(parts, seqBodyp)) {
            AstPropSpec* ধ্রুবক psp = VN_CAST(assertp->propp(), PropSpec);
            UASSERT_OBJ(psp, assertp, "Concurrent assertion must have PropSpec");
            replaceBodyOnBuildError(assertp->fileline(), psp, /*errorEmitted=*/সত্য);
            ফেরত;
        }

        AstCover* ধ্রুবক coverp = VN_CAST(assertp, Cover);
        ধ্রুবক যৌক্তিক isCoverSeq = coverp && coverp->isCoverSeq();
        // A sequence event control is not an assertion directive; no default
        // disable iff, no assertion control
        ধ্রুবক যৌক্তিক isSeqEvent = coverp && coverp->isSeqEvent();
        ধ্রুবক যৌক্তিক গুনতি_Matches = isCoverSeq && !isSeqEvent;
        // Inherit module defaults (IEEE 14.12, 16.15) when assertion has none.
        যদি (!propp->sensesp() && m_defaultClockingp) {
            propp->sensesp(m_defaultClockingp->sentreep()->sensesp()->clone_বৃক্ষ(সত্য));
        }
        যদি (!propp->disablep() && m_defaultDisablep && !isSeqEvent) {
            propp->disablep(m_defaultDisablep->condp()->clone_বৃক্ষ_Pure(সত্য));
        }
        যদি (!propp->sensesp()) ফেরত;
        Ast_Sen_বৃক্ষ* senTreep
            = নতুন Ast_Sen_বৃক্ষ{propp->fileline(), propp->sensesp()->clone_বৃক্ষ(সত্য)};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক disableExprp = propp->disablep();

        // NFA lowering clones repeated operands and may hoist them into an
        // always_comb block. Resolve implicit sampled-value clocks first, while
        // the enclosing assertion clock is still available.
        {
            VL_RESTORER(m_sampled_মান_Clockp);
            m_sampled_মান_Clockp = senTreep;
            iterate(propp->propp());
        }

        File_সারি* ধ্রুবক flp = assertp->fileline();

        SvaGraph graph;
        SvaNfaBuilder builder{graph,      m_modp,     m_prop_অস্থায়ী_Names,
                              isCoverSeq, isSeqEvent, coverp != nullptr};

        ধ্রুবক Build_ফলাফল ফলাফল = buildAssertionGraph(builder, graph, seqBodyp, parts, flp);
        যদি (ফলাফল.valid()) wireMatchAndMidSources(graph, ফলাফল, flp);
        যদি (!ফলাফল.valid()) {
            // Fall through to V3AssertPre for unsupported constructs; only
            // replace the body on real semantic errors. Any hoisted temps
            // from this attempt become orphan MODULETEMPs; V3Dead removes
            // them along with the dead always_comb driver.
            replaceBodyOnBuildError(flp, propp, ফলাফল.errorEmitted);
            VL_DO_DANGLING(ঠেলো_Deletep(senTreep), senTreep);
            ফেরত;
        }
        // After the build, so a construct the builder rejects reports itself.
        যদি (propertyControlp) {
            seqBodyp->v3warn(E_UNSUPPORTED,
                             "Unsupported: temporal property if/case with " << propertyControlp);
            replaceBodyOnBuildError(flp, propp, /*errorEmitted=*/সত্য);
            VL_DO_DANGLING(ঠেলো_Deletep(senTreep), senTreep);
            ফেরত;
        }
        যদি (গুনতি_Matches && !v3Global.opt.coverageUser()) {
            SvaNfaBuilder::freeUnlinkedCondp(ফলাফল.finalCondp);
            VL_DO_DANGLING(ঠেলো_Deletep(senTreep), senTreep);
            Ast_গ্রন্থি* ধ্রুবক innerPropp = propp->propp();
            innerPropp->replaceWith(নতুন AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(ঠেলো_Deletep(innerPropp), innerPropp);
            ফেরত;
        }

        AstAssert* ধ্রুবক assertAssertp = VN_CAST(assertp, Assert);
        ধ্রুবক যৌক্তিক splitImplicationPasssp = assertAssertp && assertAssertp->passsp()
                                            && parts.hasImplication
                                            && canSplitImplicationPassActions(parts);
        ধ্রুবক যৌক্তিক needMatch = assertAssertp && assertAssertp->passsp()
                               && (!parts.hasImplication || splitImplicationPasssp);

        ধ্রুবক যৌক্তিক needsThreadFailReplay
            = assertAssertp && assertAssertp->failsp() && !parts.hasImplication;
        ধ্রুবক auto signals
            = m_loweringp->lower(assertp, graph, senTreep, ফলাফল.finalCondp, disableExprp,
                                 needsThreadFailReplay, needsThreadFailReplay && !negated);
        Ast_গ্রন্থি_রাশিমালা* matchExprp = nullptr;
        Ast_গ্রন্থি_রাশিমালা* outputExprp = m_loweringp->assemble_ফলাফল(
            assertp, negated, ফলাফল.finalCondp, signals, needMatch ? &matchExprp : nullptr);
        যদি (গুনতি_Matches) {
            UASSERT_OBJ(signals.matchCountp, coverp, "Cover sequence missing match count");
            VL_DO_DANGLING(outputExprp->delete_বৃক্ষ(), outputExprp);
            propp->matchCountp(signals.matchCountp);
            outputExprp = নতুন AstNeq{flp, signals.matchCountp->clone_বৃক্ষ_Pure(মিথ্যা),
                                     newTypedConstp(flp, signals.matchCountp->dtypep(), 0)};
        }

        Ast_Sen_বৃক্ষ* ধ্রুবক threadFailReplaySenTreep
            = signals.threadFailCountp ? senTreep->clone_বৃক্ষ(মিথ্যা) : nullptr;

        VL_DO_DANGLING(ঠেলো_Deletep(senTreep), senTreep);
        যদি (ফলাফল.finalCondp && !ফলাফল.finalCondp->backp()) ঠেলো_Deletep(ফলাফল.finalCondp);

        যদি (splitImplicationPasssp) {
            splitImplicationPassActions(assertAssertp, parts, matchExprp);
        } অন্যথা {
            attachActionHandlers(assertAssertp, matchExprp, threadFailReplaySenTreep,
                                 signals.threadFailCountp);
        }

        Ast_গ্রন্থি* ধ্রুবক innerPropp = propp->propp();
        innerPropp->replaceWith(outputExprp);
        VL_DO_DANGLING(ঠেলো_Deletep(innerPropp), innerPropp);

        UINFO(4, "NFA converted assertion at " << flp << endl);

        যদি (dumpGraphLevel() >= 6) graph.m_graph.dumpDotFilePrefixed("assert-nfa");
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_loweringp);
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultDisablep);
        m_modp = nodep;
        // A class nested in a module is in the scope of the module's default clocking
        যদি (!VN_IS(nodep, Class)) m_defaultClockingp = nullptr;
        m_defaultDisablep = nodep->defaultDisablep();
        SvaNfaLowering lowering{nodep};
        m_loweringp = &lowering;
        iterateChildren(nodep);
    }
    শূন্য visit(AstClocking* nodep) override {
        যদি (nodep->isDefault() && !m_defaultClockingp) m_defaultClockingp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    শূন্য visit(AstDefaultDisable* nodep) override {}
    শূন্য visit(AstFell* nodep) override { visit_Sampled_মান(nodep); }
    শূন্য visit(AstPast* nodep) override { visit_Sampled_মান(nodep); }
    শূন্য visit(AstRose* nodep) override { visit_Sampled_মান(nodep); }
    শূন্য visit(AstStable* nodep) override { visit_Sampled_মান(nodep); }
    শূন্য visit(AstAssert* nodep) override { processAssertion(nodep); }
    শূন্য visit(AstCover* nodep) override { processAssertion(nodep); }
    শূন্য visit(AstRestrict* nodep) override {
        // Restrict property is ignored by simulators (IEEE 1800-2023 16.12.2).
        // Remove here so temporal SExpr don't leak to V3AssertPre.
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }
    শূন্য visit(AstAssertIntrinsic* nodep) override {}
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    explicit AssertNfaVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Top entry point

শূন্য V3AssertNfa::assertNfaAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":" << endl);
    { AssertNfaVisitor{nodep}; }
    V3Global::dump_Check_Global_বৃক্ষ("assertnfa", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
