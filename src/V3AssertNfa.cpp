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

বর্গ SvaStateVertex;

// Per-vertex algorithm data, stored via V3GraphVertex::userp() during lowering
সংরচনা SvaVertexData final {
    AstVar* stateVarp = nullptr;  // NBA state register for this vertex
    AstVar* delayRingVarp = nullptr;  // Packed occupancy bits or per-slot match counts
    AstVar* delayRingHeadVarp
        = nullptr;  // Cached outgoing slot; isolates readers from in-place writes
    AstVar* delayRingIdxVarp = nullptr;  // Next slot written in delayRingVarp
    AstVar* delay_Ring_Live_গুনতি_Varp = nullptr;  // Number of threads in delayRingVarp
    AstVar* delayRingWrappedVarp = nullptr;  // All slots written since the last clear
    AstVar* doneLVarp = nullptr;  // SAnd LHS done-latch
    AstVar* doneRVarp = nullptr;  // SAnd RHS done-latch
    Ast_গ্রন্থি_রাশিমালা* stateSigp = nullptr;  // Combinational state signal; OWNED during lowering
    Ast_গ্রন্থি_রাশিমালা* state_গুনতি_Sigp = nullptr;  // Cover-sequence match count; OWNED during lowering
    Ast_গ্রন্থি_রাশিমালা* state_গুনতি_Overflow_Sigp = nullptr;  // Count overflow; OWNED during lowering
    তার্কিক delayRingCounts = অসত্য;  // Ring slots hold cover-sequence match counts
    তার্কিক needsReg = অসত্য;  // True if vertex has incoming clocked edge
};

// NFA state vertex -- one per NFA position in the sequence evaluation
বর্গ SvaStateVertex final : সার্বজনিক V3GraphVertex {
    VL_RTTI_IMPL(SvaStateVertex, V3GraphVertex)
সার্বজনিক:
    // True if this is the sequence-match terminal vertex
    তার্কিক m_isMatch = অসত্য;
    // OWNED throughout-guard condition clones; IEEE 1800-2023 16.9.9
    std::vector<Ast_গ্রন্থি_রাশিমালা*> m_throughoutConds;
    // Nonzero for a ring-buffer vertex for ## delays.
    তার্কিক m_isFixedDelayRing = অসত্য;
    unsigned m_delayRingSize = 0;  // Number of ring slots. Range: max-min+1.
    Ast_গ্রন্থি_রাশিমালা* m_delayRingClearCondp = nullptr;  // local RHS for pure-boolean range
    Ast_গ্রন্থি_রাশিমালা* m_delayRingAdvanceCondp = nullptr;  // Advance only when this condition holds
    তার্কিক m_replayAbortReject = অসত্য;  // Compressed repetition needs per-thread abort replay
    // OWNED; enclosing-abort fire condition clearing state or suppressing guard rejection
    Ast_গ্রন্থি_রাশিমালা* m_abortClearp = nullptr;
    // OWNED; reject-abort fire condition rejecting all represented live threads
    Ast_গ্রন্থি_রাশিমালা* m_abortRejectp = nullptr;
    // Liveness terminal (IEEE weak semantics): reject must not fire from this source
    তার্কিক m_isUnbounded = অসত্য;
    // Temporal sequence AND combiner; IEEE 1800-2023 16.9.5
    তার্কিক m_isAndCombiner = অসত্য;
    SvaStateVertex* m_andLhsTermp = nullptr;  // LHS sub-NFA terminal vertex
    SvaStateVertex* m_andRhsTermp = nullptr;  // RHS sub-NFA terminal vertex
    Ast_গ্রন্থি_রাশিমালা* m_andLhsCondp = nullptr;  // OWNED; LHS final condition (may be nullptr)
    Ast_গ্রন্থি_রাশিমালা* m_andRhsCondp = nullptr;  // OWNED; RHS final condition (may be nullptr)
    // Reject sink for SAnd rejectOnFail wiring; not a state-signal source
    তার্কিক m_isRejectSink = অসত্য;
    // In-window vertex of a strong s_always[m:n]: if its state is still set at
    // end-of-simulation the universal-quantifier window never completed, which is
    // a liveness failure (IEEE 1800-2023 16.12.11 strong semantics).
    তার্কিক m_strongPending = অসত্য;
    // Ring introduced for the checked portion of a bounded strong s_always window.
    তার্কিক m_strongAlwaysRing = অসত্য;

    // CONSTRUCTORS
    explicit SvaStateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~SvaStateVertex() override {
        প্রতি (Ast_গ্রন্থি_রাশিমালা* cp : m_throughoutConds) VL_DO_DANGLING(cp->delete_বৃক্ষ(), cp);
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
    string name() ধ্রুব override {
        string name = "s" + cvtToStr(color());
        যদি (m_delayRingSize) {
            name += "\\n";
            name += m_isFixedDelayRing ? "fixed chain " : "range chain ";
            name += cvtToStr(m_delayRingSize) + " slots";
        }
        প্রত্যাগম name;
    }
    string dotColor() ধ্রুব override {
        যদি (m_isMatch) প্রত্যাগম "red";
        যদি (m_delayRingSize) প্রত্যাগম "blue";
        যদি (m_isAndCombiner) প্রত্যাগম "purple";
        প্রত্যাগম "black";
    }
    // LCOV_EXCL_STOP
    // Access per-vertex algorithm data (valid only during lowering phase)
    SvaVertexData* datap() ধ্রুব { প্রত্যাগম static_cast<SvaVertexData*>(userp()); }
};

// NFA transition edge -- clocked (##1) or combinational link (##0)
বর্গ SvaTransEdge final : সার্বজনিক V3GraphEdge {
    VL_RTTI_IMPL(SvaTransEdge, V3GraphEdge)
সার্বজনিক:
    Ast_গ্রন্থি_রাশিমালা* m_condp;  // Transition condition; nullptr = unconditional; OWNED
    তার্কিক m_consumesCycle;  // true = clocked edge (##1), false = link (##0/boolean)
    // Reject when source is active and condp is false; set only on
    // outermost required-step Link
    তার্কিক m_rejectOnFail = অসত্য;
    // Optional dynamic condition vertex for m_rejectOnFail. Used when the
    // success condition is another NFA state rather than a static expression.
    SvaStateVertex* m_condVtxp = nullptr;

    // CONSTRUCTORS
    SvaTransEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীর্ষ, Ast_গ্রন্থি_রাশিমালা* condp,
                 তার্কিক consumesCycle)
        : V3GraphEdge{graphp, fromp, শীর্ষ, /*weight=*/1}
        , m_condp{condp}
        , m_consumesCycle{consumesCycle} {}
    ~SvaTransEdge() override {
        যদি (m_condp) VL_DO_DANGLING(m_condp->delete_বৃক্ষ(), m_condp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string dotLabel() ধ্রুব override { প্রত্যাগম m_consumesCycle ? "##1" : "link"; }
    string dotStyle() ধ্রুব override { প্রত্যাগম m_consumesCycle ? "" : "dashed"; }
    // LCOV_EXCL_STOP
    // Typed accessors for NFA vertices
    SvaStateVertex* fromVtxp() ধ্রুব { প্রত্যাগম static_cast<SvaStateVertex*>(fromp()); }
    SvaStateVertex* toVtxp() ধ্রুব { প্রত্যাগম static_cast<SvaStateVertex*>(শীর্ষ()); }
};

// NFA graph container
বর্গ SvaGraph final {
সার্বজনিক:
    V3Graph m_graph;  // Owns all vertices and edges
    SvaStateVertex* m_startVertexp = nullptr;  // Trigger/start vertex
    SvaStateVertex* m_matchVertexp = nullptr;  // Sequence-match terminal vertex

    // Create a new state vertex
    SvaStateVertex* createStateVertex() { প্রত্যাগম নূতন SvaStateVertex{&m_graph}; }
    // Create the match terminal vertex
    SvaStateVertex* createMatchVertex() {
        SvaStateVertex* ধ্রুব vtxp = createStateVertex();
        vtxp->m_isMatch = সত্য;
        m_matchVertexp = vtxp;
        প্রত্যাগম vtxp;
    }
    // Add a clocked transition edge (##1)
    SvaTransEdge* addClockedEdge(SvaStateVertex* fromp, SvaStateVertex* শীর্ষ,
                                 Ast_গ্রন্থি_রাশিমালা* condp = nullptr) {
        প্রত্যাগম নূতন SvaTransEdge{&m_graph, fromp, শীর্ষ, condp, /*consumesCycle=*/সত্য};
    }
    // Add a combinational link (##0 / boolean condition)
    SvaTransEdge* addLink(SvaStateVertex* fromp, SvaStateVertex* শীর্ষ,
                          Ast_গ্রন্থি_রাশিমালা* condp = nullptr) {
        প্রত্যাগম নূতন SvaTransEdge{&m_graph, fromp, শীর্ষ, condp, /*consumesCycle=*/অসত্য};
    }
    // Collect all edges into a flat vector for iteration.
    // Used by the lowering phase which needs global edge scans.
    std::vector<ধ্রুব SvaTransEdge*> allEdges() ধ্রুব {
        std::vector<ধ্রুব SvaTransEdge*> result;
        প্রতি (ধ্রুব V3GraphVertex& vtxr : m_graph.vertices()) {
            প্রতি (ধ্রুব V3GraphEdge& edger : vtxr.outEdges()) {
                result.ঠেলো_back(static_cast<ধ্রুব SvaTransEdge*>(&edger));
            }
        }
        প্রত্যাগম result;
    }
};

//######################################################################
// Builder result: terminal vertex + optional final condition (match Link condition).
সংরচনা BuildResult final {
    SvaStateVertex* termVertexp;  // Primary terminal; contributes to both match and reject
    Ast_গ্রন্থি_রাশিমালা* finalCondp;  // nullptr = unconditional
    // Mid-window sources for range delays (pure boolean RHS): match-only (isUnbounded)
    std::vector<SvaStateVertex*> midSources;
    তার্কিক errorEmitted = অসত্য;  // Builder already emitted specific error; skip generic
    // For cover_sequence: when true, midSources already enumerate every
    // end-of-match, so wireMatchAndMidSources must NOT add the main
    // termVtxp -> matchVertex Link (would double-count via the merge vertex).
    তার্কিক termIsMidMerge = অসত্য;
    তার্কিক valid() ধ্রুব { প্রত্যাগম termVertexp != nullptr; }
    স্থির BuildResult fail(তার্কিক errored = অসত্য) { প্রত্যাগম {nullptr, nullptr, {}, errored}; }
    স্থির BuildResult failWithError() { প্রত্যাগম {nullptr, nullptr, {}, সত্য}; }
};

// Parser-marked SAnd of overlapped implications: a property if/else/case.
স্থির তার্কিক hasPropertyControlConjunction(ধ্রুব Ast_গ্রন্থি_রাশিমালা* nodep) {
    প্রত্যাগম nodep->exists([](ধ্রুব AstSAnd* andp) { প্রত্যাগম andp->propertyControl(); });
}

স্থির AstConst* newTypedConstp(File_সারি* ধ্রুব flp, ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব dtypep,
                                ধ্রুব uint32_t মান) {
    AstConst* ধ্রুব constp = নূতন AstConst{flp, AstConst::DTyped{}, dtypep};
    constp->অঙ্ক().setLong(মান);
    প্রত্যাগম constp;
}

স্থির Ast_গ্রন্থি_রাশিমালা* sampled(Ast_গ্রন্থি_রাশিমালা* exprp) {
    প্রত্যাগম নূতন AstSampled{exprp->fileline(), exprp, exprp->dtypep(), সত্য};
}

স্থির ধ্রুব বর্ণ* assertPassOnQuery(তার্কিক vacuous) {
    স্থির constexpr ধ্রুব বর্ণ* queries[2]
        = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
    প্রত্যাগম queries[vacuous];
}

স্থির Ast_গ্রন্থি_রাশিমালা* assertOnCond(File_সারি* flp, VAssertType type,
                                 VAssertDirectiveType directiveType) {
    যদি (!v3Global.opt.assertOn()) { প্রত্যাগম নূতন AstConst{flp, AstConst::BitFalse{}}; }
    প্রত্যাগম নূতন AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
}

স্থির Ast_গ্রন্থি_রাশিমালা* assertKillGet(File_সারি* flp, VAssertType type,
                                  VAssertDirectiveType directiveType) {
    প্রত্যাগম নূতন AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_KILL", type, directiveType),
                        32};
}

স্থির string assertActionControlPrefix(VAssertDirectiveType directiveType) {
    ধ্রুব পূর্ণাঙ্ক controlled = !!(static_cast<পূর্ণাঙ্ক>(directiveType)
                              & (static_cast<পূর্ণাঙ্ক>(VAssertDirectiveType::ASSERT)
                                 | static_cast<পূর্ণাঙ্ক>(VAssertDirectiveType::COVER)
                                 | static_cast<পূর্ণাঙ্ক>(VAssertDirectiveType::ASSUME)));
    ধ্রুব পূর্ণাঙ্ক checkRuntime = controlled & static_cast<পূর্ণাঙ্ক>(v3Global.opt.assertOn());
    প্রত্যাগম "("s + std::to_string(controlled ^ 1) + " || ("s + std::to_string(checkRuntime)
           + " && "s;
}

স্থির Ast_গ্রন্থি_রাশিমালা* assertPassOnCond(File_সারি* flp, VAssertType type,
                                     VAssertDirectiveType directiveType, তার্কিক vacuous) {
    প্রত্যাগম নূতন AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType)
            + "))"s,
        1};
}

স্থির Ast_গ্রন্থি_রাশিমালা* assertFailOnCond(File_সারি* flp, VAssertType type,
                                     VAssertDirectiveType directiveType) {
    প্রত্যাগম নূতন AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType) + "))"s,
        1};
}

স্থির AstIf* newPassOnIf(File_সারি* flp, Ast_গ্রন্থি_রাশিমালা* firep, Ast_গ্রন্থি* bodyp, VAssertType type,
                          VAssertDirectiveType directiveType, তার্কিক vacuous) {
    Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp
        = নূতন AstLogAnd{flp, firep, assertPassOnCond(flp, type, directiveType, vacuous)};
    AstIf* ধ্রুব ifp = নূতন AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(সত্য);
    ifp->user1(সত্য);
    প্রত্যাগম ifp;
}

স্থির Ast_গ্রন্থি_Stmt* newIfAssertFailOn(Ast_গ্রন্থি* bodyp, VAssertDirectiveType directiveType,
                                      VAssertType type) {
    File_সারি* ধ্রুব flp = bodyp->fileline();
    Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp = assertFailOnCond(flp, type, directiveType);
    AstIf* ধ্রুব ifp = নূতন AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(সত্য);
    ifp->user1(সত্য);
    প্রত্যাগম ifp;
}

//######################################################################
// NFA Builder

বর্গ SvaNfaBuilder final {
    SvaGraph& m_graph;  // NFA graph being built
    Ast_গ্রন্থি_Module* ধ্রুব m_modp;  // Module to receive hoisted sampled-prop temps
    V3UniqueNames& m_propTempNames;  // Module-shared temp-var name source
    std::vector<Ast_গ্রন্থি_রাশিমালা*> m_temporal_Guard_স্তূপ;  // Guards active across nested temporal states
    // Outer abort conditions, AND-ed as !cond into inner abort edges
    // (IEEE 1800-2023 16.12.14 outer-wraps-inner).
    std::vector<Ast_গ্রন্থি_রাশিমালা*> m_outer_Abort_স্তূপ;
    তার্কিক m_inUnboundedScope = অসত্য;  // Sticky: nodes created after inherit liveness
    তার্কিক m_markStrongPending = অসত্য;  // Mark new vertices as strong s_always in-window
    তার্কিক m_isCover = অসত্য;  // Cover directives do not fail at end-of-simulation
    // IEEE 1800-2023 16.14.3 cover sequence: each end-of-match fires the action,
    // not just the first. Builder builds parallel-branch (no first-match-wins)
    // topology when true. Default false preserves cover_property semantics.
    তার্কিক m_isCoverSeq = অসত্য;
    // Unsupported endpoint topology must reject, not ignore, or the wait hangs
    তার্কিক m_isSeqEvent = অসত্য;
    তার্কিক m_inSequencePrefix = অসত্য;  // Prefix composition does not preserve every endpoint
    তার্কিক m_wantBoolLeaf = অসত্য;  // Building a |-> / |=> or top-level body (see buildOrMerge)

    সংরচনা RangeDelayRejectInfo final {
        SvaStateVertex* startp = nullptr;
        unsigned range = 0;
        পূর্ণাঙ্ক rhsLen = 0;
    };

    শূন্য warnEndpointUnsupported(File_সারি* flp, ধ্রুব string& what) ধ্রুব {
        যদি (m_isSeqEvent) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: sequence used as an event control with " << what);
        } অন্যথা {
            flp->v3warn(COVERIGN, "Ignoring unsupported: cover sequence with " << what);
        }
    }

    Ast_গ্রন্থি_রাশিমালা* throughoutCond(Ast_গ্রন্থি_রাশিমালা* baseCondp, File_সারি* flp) {
        যদি (m_temporal_Guard_স্তূপ.empty()) প্রত্যাগম baseCondp;
        // AND all active temporal guards (supports nesting)
        // Each must use $sampled values.
        Ast_গ্রন্থি_রাশিমালা* guardp = nullptr;
        প্রতি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp : m_temporal_Guard_স্তূপ) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব clonep = sampled(condp->clone_বৃক্ষ_Pure(অসত্য));
            যদি (!guardp) {
                guardp = clonep;
            } অন্যথা {
                guardp = নূতন AstLogAnd{flp, guardp, clonep};
            }
        }
        যদি (baseCondp) { guardp = নূতন AstLogAnd{flp, baseCondp, guardp}; }
        প্রত্যাগম guardp;
    }

    স্থির unsigned getConstUInt(Ast_গ্রন্থি_রাশিমালা* exprp) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব constp = V3Const::constifyEdit(exprp->clone_বৃক্ষ_Pure(অসত্য));
        ধ্রুব AstConst* ধ্রুব cp = VN_CAST(constp, Const);
        ধ্রুব unsigned val = cp ? cp->toUInt() : 0;
        VL_DO_DANGLING(constp->delete_বৃক্ষ(), constp);
        প্রত্যাগম val;
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
    স্থির পূর্ণাঙ্ক fixedLength(Ast_গ্রন্থি_রাশিমালা* nodep) {
        যদি (AstSExpr* ধ্রুব sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ধ্রুব delayp = VN_CAST(sexprp->delayp(), Delay);
            যদি (!delayp || !delayp->isCycleDelay()) প্রত্যাগম -1;
            unsigned delayCycles;
            যদি (delayp->isRangeDelay()) {
                যদি (delayp->isUnbounded()) প্রত্যাগম -1;  // LCOV_EXCL_LINE
                ধ্রুব unsigned minD = getConstUInt(delayp->lhsp());
                ধ্রুব unsigned maxD = getConstUInt(delayp->rhsp());
                যদি (minD != maxD) প্রত্যাগম -1;
                delayCycles = minD;
            } অন্যথা {
                delayCycles = getConstUInt(delayp->lhsp());
            }
            পূর্ণাঙ্ক preLen = 0;
            যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব prep = sexprp->preExprp()) {
                preLen = fixedLength(prep);
                যদি (preLen < 0) প্রত্যাগম -1;  // LCOV_EXCL_LINE
            }
            ধ্রুব পূর্ণাঙ্ক bodyLen = fixedLength(sexprp->exprp());
            যদি (bodyLen < 0) প্রত্যাগম -1;  // LCOV_EXCL_LINE
            প্রত্যাগম preLen + delayCycles + bodyLen;
        }
        যদি (AstSThroughout* ধ্রুব throughp = VN_CAST(nodep, SThroughout)) {
            প্রত্যাগম fixedLength(throughp->rhsp());
        }
        যদি (AstSOr* ধ্রুব orp = VN_CAST(nodep, SOr)) {
            // Alternatives must share one end cycle; buildSWithin relies on
            // this to pair the OR with an SIntersect.
            ধ্রুব পূর্ণাঙ্ক lhsLen = fixedLength(orp->lhsp());
            ধ্রুব পূর্ণাঙ্ক rhsLen = fixedLength(orp->rhsp());
            যদি (lhsLen < 0 || rhsLen < 0 || lhsLen != rhsLen) প্রত্যাগম -1;
            প্রত্যাগম lhsLen;
        }
        যদি (AstSWithin* ধ্রুব withinp = VN_CAST(nodep, SWithin)) {
            // `seq1 within seq2` ends at seq2's end cycle (IEEE 16.9.10).
            ধ্রুব পূর্ণাঙ্ক lhsLen = fixedLength(withinp->lhsp());
            ধ্রুব পূর্ণাঙ্ক rhsLen = fixedLength(withinp->rhsp());
            যদি (lhsLen < 0 || rhsLen < 0 || lhsLen > rhsLen) প্রত্যাগম -1;
            প্রত্যাগম rhsLen;
        }
        // LCOV_EXCL_START -- defensive: V3AssertPre rejects composite SVA ops
        // nested in an intersect arm before fixedLength runs (clock-context
        // resolution fails). Kept as a guard in case future parser relaxations
        // permit it.
        যদি (nodep->isMultiCycleSva()) প্রত্যাগম -1;
        // LCOV_EXCL_STOP
        // Plain boolean expression (no SVA constructs) -- 0 cycles.
        প্রত্যাগম 0;
    }

    // Contiguous match-length range [lo,hi] for an operand whose length varies
    // from AT MOST ONE ranged cycle delay; {-1,-1} otherwise. Drives the
    // variable-length `intersect` lowering (IEEE 1800-2023 16.9.6): more than
    // one ranged delay would make the per-length realization ambiguous, so it
    // is reported unsupported rather than mis-paired.
    স্থির std::pair<পূর্ণাঙ্ক, পূর্ণাঙ্ক> lengthRange(Ast_গ্রন্থি_রাশিমালা* nodep) {
        যদি (AstSExpr* ধ্রুব sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ধ্রুব delayp = VN_CAST(sexprp->delayp(), Delay);
            যদি (!delayp || !delayp->isCycleDelay()) প্রত্যাগম {-1, -1};
            std::pair<পূর্ণাঙ্ক, পূর্ণাঙ্ক> delayRange;
            যদি (delayp->isRangeDelay()) {
                যদি (delayp->isUnbounded()) প্রত্যাগম {-1, -1};
                ধ্রুব unsigned minD = getConstUInt(delayp->lhsp());
                ধ্রুব unsigned maxD = getConstUInt(delayp->rhsp());
                delayRange = {minD, maxD};
            } অন্যথা {
                ধ্রুব unsigned d = getConstUInt(delayp->lhsp());
                delayRange = {d, d};
            }
            std::pair<পূর্ণাঙ্ক, পূর্ণাঙ্ক> preRange{0, 0};
            যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব prep = sexprp->preExprp()) {
                preRange = lengthRange(prep);
                যদি (preRange.first < 0) প্রত্যাগম {-1, -1};
            }
            ধ্রুব std::pair<পূর্ণাঙ্ক, পূর্ণাঙ্ক> bodyRange = lengthRange(sexprp->exprp());
            যদি (bodyRange.first < 0) প্রত্যাগম {-1, -1};
            ধ্রুব পূর্ণাঙ্ক variableParts = (preRange.first != preRange.second)
                                      + (delayRange.first != delayRange.second)
                                      + (bodyRange.first != bodyRange.second);
            যদি (variableParts > 1) প্রত্যাগম {-1, -1};
            প্রত্যাগম {preRange.first + delayRange.first + bodyRange.first,
                    preRange.second + delayRange.second + bodyRange.second};
        }
        যদি (AstSThroughout* ধ্রুব throughp = VN_CAST(nodep, SThroughout)) {
            প্রত্যাগম lengthRange(throughp->rhsp());
        }
        যদি (nodep->isMultiCycleSva()) প্রত্যাগম {-1, -1};
        প্রত্যাগম {0, 0};  // plain boolean -- 0 cycles
    }

    // Clone `operand` with its sole variable ranged cycle delay pinned so the
    // total match length is exactly `len`. `lo` is the operand's minimum length
    // (lengthRange().first). A fixed operand has no such delay and is returned
    // as a plain clone (callers only request its single achievable length).
    স্থির Ast_গ্রন্থি_রাশিমালা* realizeAtLength(Ast_গ্রন্থি_রাশিমালা* operand, পূর্ণাঙ্ক len, পূর্ণাঙ্ক lo) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব clonep = operand->clone_বৃক্ষ_Pure(অসত্য);
        AstDelay* rangeDelayp = nullptr;
        clonep->foreach([&](AstDelay* dp) {
            যদি (!rangeDelayp && dp->isRangeDelay() && !dp->isUnbounded()
                && getConstUInt(dp->lhsp()) != getConstUInt(dp->rhsp())) {
                rangeDelayp = dp;
            }
        });
        যদি (rangeDelayp) {
            File_সারি* ধ্রুব flp = rangeDelayp->fileline();
            ধ্রুব unsigned pinned = getConstUInt(rangeDelayp->lhsp()) + (len - lo);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব oldMinp = rangeDelayp->lhsp();
            oldMinp->replaceWith(নূতন AstConst{flp, pinned});
            VL_DO_DANGLING(oldMinp->delete_বৃক্ষ(), oldMinp);
            // Drop the max bound so it lowers as a fixed `##d`, not `##[d:d]`.
            Ast_গ্রন্থি* ধ্রুব oldMaxp = rangeDelayp->rhsp()->unlinkFrBack();
            VL_DO_DANGLING(oldMaxp->delete_বৃক্ষ(), oldMaxp);
        }
        প্রত্যাগম clonep;
    }

    // Cuts AST size from O(N * sizeof(exprp)) to O(N) + O(sizeof(exprp)) by
    // sharing a single `VarRef` across N check edges. Hoist also matches the
    // IEEE 1800-2023 16.9.9 "single preponed-region snapshot" semantic for
    // any exprp -- even an impure one would now evaluate exactly once per
    // clock instead of N times. Orphan temps from failed builds are unused
    // MODULETEMPs and are removed by V3Dead.
    AstVar* tryHoistSampled(Ast_গ্রন্থি_রাশিমালা* exprp, File_সারি* flp, unsigned clone_গুনতি) {
        constexpr unsigned kHoistThreshold = 2;
        যদি (clone_গুনতি < kHoistThreshold) প্রত্যাগম nullptr;
        AstVar* ধ্রুব tempVarp
            = নূতন AstVar{flp, VVarType::MODULETEMP, m_propTempNames.get(exprp), exprp->dtypep()};
        m_modp->addStmtsp(tempVarp);
        AstAssign* ধ্রুব assignp = নূতন AstAssign{flp, নূতন AstVarRef{flp, tempVarp, VAccess::WRITE},
                                                 sampled(exprp->clone_বৃক্ষ_Pure(অসত্য))};
        m_modp->addStmtsp(নূতন AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, assignp});
        প্রত্যাগম tempVarp;
    }

    স্থির Ast_গ্রন্থি_রাশিমালা* sampledRefOrClone(AstVar* hoistVarp, Ast_গ্রন্থি_রাশিমালা* exprp, File_সারি* flp) {
        যদি (hoistVarp) প্রত্যাগম নূতন AstVarRef{flp, hoistVarp, VAccess::READ};
        প্রত্যাগম sampled(exprp->clone_বৃক্ষ_Pure(অসত্য));
    }

    // Create vertex and inherit temporal guards from the current scope.
    SvaStateVertex* scopedCreateVertex() {
        SvaStateVertex* ধ্রুব vtxp = m_graph.createStateVertex();
        প্রতি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব cp : m_temporal_Guard_স্তূপ) {
            vtxp->m_throughoutConds.ঠেলো_back(cp->clone_বৃক্ষ_Pure(অসত্য));
        }
        যদি (m_inUnboundedScope) vtxp->m_isUnbounded = সত্য;
        যদি (m_markStrongPending) vtxp->m_strongPending = সত্য;
        প্রত্যাগম vtxp;
    }

    // AND current temporal guards into every edge/link.
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* শীর্ষ, Ast_গ্রন্থি_রাশিমালা* condp,
                              File_সারি* flp) {
        প্রত্যাগম m_graph.addLink(fromp, শীর্ষ, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* শীর্ষ, File_সারি* flp) {
        প্রত্যাগম m_graph.addLink(fromp, শীর্ষ, throughoutCond(nullptr, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* শীর্ষ, Ast_গ্রন্থি_রাশিমালা* condp,
                              File_সারি* flp) {
        প্রত্যাগম m_graph.addClockedEdge(fromp, শীর্ষ, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* শীর্ষ, File_সারি* flp) {
        প্রত্যাগম m_graph.addClockedEdge(fromp, শীর্ষ, throughoutCond(nullptr, flp));
    }

    SvaStateVertex* addDelayChain(SvaStateVertex* startp, unsigned size, File_সারি* flp,
                                  তার্কিক isFixed = সত্য, Ast_গ্রন্থি_রাশিমালা* clearCondp = nullptr,
                                  Ast_গ্রন্থি_রাশিমালা* advanceCondp = nullptr) {
        যদি (isFixed && size == 0) প্রত্যাগম startp;
        UASSERT_OBJ(size > 0, startp, "Delay chain needs at least one slot");
        যদি (isFixed && size == 1 && !advanceCondp) {
            SvaStateVertex* ধ্রুব nextp = scopedCreateVertex();
            guardedEdge(startp, nextp, flp);
            প্রত্যাগম nextp;
        }
        SvaStateVertex* ধ্রুব ringVtxp = scopedCreateVertex();
        ringVtxp->m_isFixedDelayRing = isFixed;
        ringVtxp->m_delayRingSize = size;
        যদি (clearCondp) {
            UASSERT_OBJ(!isFixed, startp, "Fixed delay cannot have a clear condition");
            ringVtxp->m_delayRingClearCondp = clearCondp->clone_বৃক্ষ_Pure(অসত্য);
        }
        ringVtxp->m_delayRingAdvanceCondp = advanceCondp;
        যদি (isFixed) {
            guardedEdge(startp, ringVtxp, flp);
        } অন্যথা {
            guardedLink(startp, ringVtxp, flp);
        }
        প্রত্যাগম ringVtxp;
    }

    // Build NFA for an SExpr. finalCond = RHS (not yet added as a vertex).
    // isTopLevelStep: marks outermost required boolean check as rejectOnFail.
    // Apply a range delay `##[M:N]` to currentp. Returns true on success. On
    // failure, emits a diagnostic and returns false.
    তার্কিক applyRangeDelay(AstSExpr* sexprp, SvaStateVertex*& currentp,
                         std::vector<SvaStateVertex*>& midSources,
                         RangeDelayRejectInfo* rangeRejectInfop = nullptr) {
        File_সারি* ধ্রুব flp = sexprp->fileline();
        AstDelay* ধ্রুব delayp = VN_AS(sexprp->delayp(), Delay);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsExprp = sexprp->exprp();
        ধ্রুব unsigned minDelay = getConstUInt(delayp->lhsp());
        যদি (delayp->isUnbounded()) {
            // `##[M:$]`: wait M cycles, then retain every matured attempt in a
            // one-slot ring while also exposing the first eligible tick.
            currentp = addDelayChain(currentp, minDelay, flp);
            SvaStateVertex* ধ্রুব waitRingp = addDelayChain(currentp, 1, flp, অসত্য);
            // The ring's live count is updated in NBA, so this link retains the previous
            // tick's live attempts alongside new arrivals for the next tick.
            guardedLink(waitRingp, waitRingp, flp);
            SvaStateVertex* ধ্রুব mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(waitRingp, mergeVtxp, flp);
            currentp = mergeVtxp;
            currentp->m_isUnbounded = সত্য;  // Liveness, so no reject.
            m_inUnboundedScope = সত্য;
            প্রত্যাগম সত্য;
        }
        ধ্রুব unsigned maxDelay = getConstUInt(delayp->rhsp());
        যদি (minDelay == maxDelay) {
            currentp = addDelayChain(currentp, minDelay, flp);
            প্রত্যাগম সত্য;
        }
        ধ্রুব unsigned range = maxDelay - minDelay;
        currentp = addDelayChain(currentp, minDelay, flp);
        ধ্রুব তার্কিক multiCycleRhs = rhsExprp->isMultiCycleSva();
        ধ্রুব Ast_গ্রন্থি_রাশিমালা* ধ্রুব preExprp = sexprp->preExprp();
        // Prefix and nested-sequence composition can lose endpoints or their multiplicity
        // required by cover sequence (IEEE 1800-2023 16.14.3), regardless of the range width.
        যদি (m_isCoverSeq
            && (multiCycleRhs || m_inSequencePrefix
                || (preExprp && preExprp->isMultiCycleSva()))) {
            warnEndpointUnsupported(flp, "this ranged cycle delay");
            প্রত্যাগম অসত্য;
        }
        যদি (m_inSequencePrefix) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: Bounded ranged cycle delay in a sequence prefix");
            প্রত্যাগম অসত্য;
        }
        যদি (multiCycleRhs) {
            // Launch a candidate at every eligible tick. Candidate-local misses are not
            // assertion rejects while a later position can still match.
            যদি (rangeRejectInfop) {
                ধ্রুব পূর্ণাঙ্ক rhsLen = fixedLength(rhsExprp);
                যদি (rhsLen >= 0) *rangeRejectInfop = {currentp, range, rhsLen};
            }
            SvaStateVertex* ধ্রুব tailp = addDelayChain(currentp, range + 1U, flp, অসত্য);
            SvaStateVertex* ধ্রুব mergeVtxp = scopedCreateVertex();
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
            currentp = addDelayChain(currentp, range + 1U, flp, অসত্য,
                                     m_isCoverSeq ? nullptr : rhsExprp);
        }
        প্রত্যাগম সত্য;
    }

    শূন্য addFiniteRangeReject(ধ্রুব RangeDelayRejectInfo& info, ধ্রুব BuildResult& result,
                              File_সারি* flp) {
        যদি (!info.startp) প্রত্যাগম;

        SvaStateVertex* ধ্রুব expiryVtxp
            = addDelayChain(info.startp, info.range + info.rhsLen, flp);
        SvaStateVertex* ধ্রুব expiryMatchp = scopedCreateVertex();
        std::vector<SvaStateVertex*> sources = result.midSources;
        sources.ঠেলো_back(result.termVertexp);
        প্রতি (SvaStateVertex* ধ্রুব srcp : sources) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp
                = result.finalCondp ? sampled(result.finalCondp->clone_বৃক্ষ_Pure(অসত্য)) : nullptr;
            SvaStateVertex* ধ্রুব successNowp = scopedCreateVertex();
            guardedLink(srcp, successNowp, condp, flp);
            // Retain a success through the latest deadline it can satisfy.
            SvaStateVertex* ধ্রুব historyp
                = addDelayChain(successNowp, info.range + 1U, flp, অসত্য);
            guardedLink(successNowp, expiryMatchp, flp);
            guardedLink(historyp, expiryMatchp, flp);
        }

        SvaStateVertex* ধ্রুব sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = সত্য;
        SvaTransEdge* ধ্রুব rejectp = m_graph.addLink(expiryVtxp, sinkVtxp);
        rejectp->m_rejectOnFail = সত্য;
        rejectp->m_condVtxp = expiryMatchp;
    }

    BuildResult buildSExpr(AstSExpr* sexprp, SvaStateVertex* entryVtxp,
                           তার্কিক is_শীর্ষ_Level_Step = অসত্য) {
        AstDelay* ধ্রুব delayp = VN_CAST(sexprp->delayp(), Delay);
        যদি (!delayp || !delayp->isCycleDelay()) প্রত্যাগম BuildResult::fail();

        File_সারি* ধ্রুব flp = sexprp->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = sexprp->exprp();

        // Handle LHS (preExpr)
        SvaStateVertex* currentp = entryVtxp;
        যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব preExprp = sexprp->preExprp()) {
            VL_RESTORER(m_inSequencePrefix);
            m_inSequencePrefix = সত্য;
            ধ্রুব BuildResult pre = build_রাশিমালা(preExprp, currentp, is_শীর্ষ_Level_Step);
            যদি (!pre.valid()) প্রত্যাগম BuildResult::fail(pre.errorEmitted);  // LCOV_EXCL_LINE
            যদি (pre.finalCondp) {
                SvaStateVertex* ধ্রুব condVtxp = scopedCreateVertex();
                SvaTransEdge* ধ্রুব edgep = guardedLink(
                    pre.termVertexp, condVtxp, sampled(pre.finalCondp->clone_বৃক্ষ_Pure(অসত্য)), flp);
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
        ধ্রুব তার্কিক addRangeReject = is_শীর্ষ_Level_Step && !m_inUnboundedScope;
        যদি (delayp->isRangeDelay()) {
            যদি (!applyRangeDelay(sexprp, currentp, rangeMidSources,
                                 addRangeReject ? &rangeRejectInfo : nullptr)) {
                প্রত্যাগম BuildResult::failWithError();
            }
        } অন্যথা {
            ধ্রুব unsigned delayCycles = getConstUInt(delayp->lhsp());
            currentp = addDelayChain(currentp, delayCycles, flp);
        }

        // Multi-cycle RHS: recurse (only plain boolean is returned as finalCondp).
        যদি (exprp->isMultiCycleSva()) {
            ধ্রুব BuildResult result = build_রাশিমালা(exprp, currentp, is_শীর্ষ_Level_Step);
            যদি (result.valid()) addFiniteRangeReject(rangeRejectInfo, result, flp);
            প্রত্যাগম result;
        }
        প্রত্যাগম {currentp, exprp, std::move(rangeMidSources)};
    }

    BuildResult buildConsRep(AstSConsRep* repp, SvaStateVertex* entryVtxp,
                             তার্কিক is_শীর্ষ_Level_Step = অসত্য) {
        File_সারি* ধ্রুব flp = repp->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = repp->exprp();
        // Multi-cycle expr in ConsRep not yet supported; bail to avoid invalid AST.
        যদি (exprp->isMultiCycleSva()) {
            repp->v3warn(E_UNSUPPORTED, "Unsupported: multi-cycle sequence expression inside"
                                        " consecutive repetition (IEEE 1800-2023 16.9.2)");
            প্রত্যাগম BuildResult::failWithError();
        }
        ধ্রুব unsigned minN = getConstUInt(repp->countp());

        // Sum sites across prefix + unbounded/range tail so one hoist covers
        // every check edge of this repetition.
        unsigned totalSites = minN;
        যদি (repp->unbounded()) {
            totalSites += 1;
        } অন্যথা যদি (repp->maxCountp()) {
            totalSites += getConstUInt(repp->maxCountp()) - minN;
        }
        AstVar* ধ্রুব hoistVarp = tryHoistSampled(exprp, flp, totalSites);

        // Cover-sequence (IEEE 1800-2023 16.14.3): collect each end-of-match
        // position so they all fire the action, not just the merged terminal.
        std::vector<SvaStateVertex*> consMidSources;

        SvaStateVertex* currentp = entryVtxp;
        প্রতি (unsigned i = 0; i < minN; ++i) {
            // Keep the first repetition explicit, collapse all remaining checks into the ring.
            যদি (i == 1) {
                currentp = addDelayChain(currentp, minN - 1, flp);
                currentp->m_delayRingClearCondp
                    = নূতন AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                currentp->m_replayAbortReject = সত্য;
                যদি (is_শীর্ষ_Level_Step) {
                    currentp->m_throughoutConds.ঠেলো_back(
                        sampledRefOrClone(hoistVarp, exprp, flp));
                }
                i = minN - 1;
            }
            // Every repetition in the minimum prefix is required.
            SvaStateVertex* ধ্রুব condVtxp = scopedCreateVertex();
            SvaTransEdge* ধ্রুব linkp
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
                SvaStateVertex* ধ্রুব waitVtxp = scopedCreateVertex();
                guardedEdge(currentp, waitVtxp, flp);
                SvaStateVertex* ধ্রুব checkVtxp = scopedCreateVertex();
                guardedLink(waitVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedEdge(checkVtxp, waitVtxp, flp);
                guardedLink(currentp, checkVtxp, flp);
                currentp = checkVtxp;
            } অন্যথা {
                SvaStateVertex* ধ্রুব loopBackVtxp = scopedCreateVertex();
                guardedEdge(currentp, loopBackVtxp, flp);
                SvaStateVertex* ধ্রুব reCheckVtxp = scopedCreateVertex();
                guardedLink(loopBackVtxp, reCheckVtxp, sampledRefOrClone(hoistVarp, exprp, flp),
                            flp);
                guardedEdge(reCheckVtxp, loopBackVtxp, flp);
                guardedLink(reCheckVtxp, currentp, flp);
            }
            currentp->m_isUnbounded = সত্য;
            m_inUnboundedScope = সত্য;
        } অন্যথা যদি (repp->maxCountp()) {
            ধ্রুব unsigned maxN = getConstUInt(repp->maxCountp());
            SvaStateVertex* ধ্রুব mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            unsigned tailMinN = minN;
            SvaStateVertex* tailStartp = currentp;
            যদি (minN == 0) {
                // Build the first optional iteration explicitly. Feeding the empty endpoint
                // directly into a range ring would incorrectly keep that match alive.
                SvaStateVertex* ধ্রুব nextVtxp = scopedCreateVertex();
                guardedEdge(currentp, nextVtxp, flp);
                SvaStateVertex* ধ্রুব checkVtxp = scopedCreateVertex();
                guardedLink(nextVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                যদি (m_isCoverSeq) consMidSources.ঠেলো_back(checkVtxp);
                tailStartp = checkVtxp;
                tailMinN = 1;
            }
            যদি (maxN > tailMinN) {
                // Add tail-ring only if the tail is non-empty.
                SvaStateVertex* ধ্রুব nextVtxp
                    = addDelayChain(tailStartp, maxN - tailMinN + 1, flp, অসত্য);
                nextVtxp->m_delayRingClearCondp
                    = নূতন AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                nextVtxp->m_replayAbortReject = সত্য;
                SvaStateVertex* ধ্রুব checkVtxp = scopedCreateVertex();
                guardedLink(nextVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                যদি (m_isCoverSeq) consMidSources.ঠেলো_back(checkVtxp);
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
        প্রত্যাগম res;
    }

    // always[lo:hi] / s_always[lo:hi] (IEEE 1800-2023 16.12.11).
    BuildResult buildPropAlways(AstPropAlways* nodep, SvaStateVertex* entryVtxp,
                                তার্কিক is_শীর্ষ_Level_Step = অসত্য) {
        File_সারি* ধ্রুব flp = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব propp = nodep->propp();
        ধ্রুব unsigned lo = getConstUInt(nodep->loBoundp());
        যদি (VN_IS(nodep->hiBoundp(), Unbounded)) {
            // Weak always [lo:$]: unbounded upper bound (IEEE 1800-2023 16.12.11).
            // p must hold at every clock tick at least lo cycles after the attempt
            // start; those ticks are not required to exist, so there is no
            // end-of-trace obligation (weak). The self-loop keeps the attempt live
            // every cycle; each observed cycle is a safety obligation, so a false p
            // rejects immediately.
            UASSERT_OBJ(!nodep->isStrong(), nodep, "Unbounded always must be weak (V3Width)");
            SvaStateVertex* ধ্রুব livep = addDelayChain(entryVtxp, lo, flp);
            livep->m_isUnbounded = সত্য;
            guardedEdge(livep, livep, flp);  // stay active every subsequent cycle
            SvaStateVertex* ধ্রুব sinkp = m_graph.createStateVertex();
            sinkp->m_isRejectSink = সত্য;
            SvaTransEdge* ধ্রুব rejEdgep
                = guardedLink(livep, sinkp, sampled(propp->clone_বৃক্ষ_Pure(অসত্য)), flp);
            যদি (is_শীর্ষ_Level_Step) rejEdgep->m_rejectOnFail = সত্য;
            প্রত্যাগম {livep, nullptr, {}};
        }
        ধ্রুব unsigned hi = getConstUInt(nodep->hiBoundp());
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
        SvaStateVertex* ধ্রুব checkp = scopedCreateVertex();
        SvaTransEdge* ধ্রুব linkp
            = guardedLink(currentp, checkp, sampled(propp->clone_বৃক্ষ_Pure(অসত্য)), flp);
        যদি (is_শীর্ষ_Level_Step) linkp->m_rejectOnFail = সত্য;
        currentp = checkp;
        m_temporal_Guard_স্তূপ.ঠেলো_back(propp);
        currentp = addDelayChain(currentp, hi - lo, flp);
        যদি (nodep->isStrong() && currentp->m_delayRingSize) currentp->m_strongAlwaysRing = সত্য;
        m_temporal_Guard_স্তূপ.বের_করো_back();
        প্রত্যাগম {currentp, propp, {}};
    }

    BuildResult buildGotoRep(AstSGotoRep* repp, SvaStateVertex* entryVtxp) {
        File_সারি* ধ্রুব flp = repp->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = repp->exprp();
        ধ্রুব unsigned minN = getConstUInt(repp->countp());
        যদি (minN == 0) প্রত্যাগম BuildResult::fail();
        ধ্রুব তার্কিক hasMax = repp->maxCountp() != nullptr;
        ধ্রুব unsigned maxN = hasMax ? getConstUInt(repp->maxCountp()) : minN;
        যদি (m_isCoverSeq) {
            // Several attempts may coalesce in the one-bit wait vertex before reaching the ring,
            // so a cover sequence action block could run too few times.
            warnEndpointUnsupported(flp, "a goto repetition");
            প্রত্যাগম BuildResult::failWithError();
        }

        AstVar* ধ্রুব hoistVarp = tryHoistSampled(exprp, flp, 2);

        // The first guardedEdge is the ##1 before waiting for a match. In the wait state, false
        // takes the clocked self-loop, while true takes the zero-delay guardedLink on that tick.
        SvaStateVertex* ধ্রুব waitVtxp = scopedCreateVertex();
        guardedEdge(entryVtxp, waitVtxp, flp);
        guardedEdge(waitVtxp, waitVtxp,
                    নূতন AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)}, flp);
        SvaStateVertex* currentp = scopedCreateVertex();
        guardedLink(waitVtxp, currentp, sampledRefOrClone(hoistVarp, exprp, flp), flp);

        যদি (minN > 1) {
            SvaStateVertex* ধ্রুব ringVtxp = addDelayChain(
                currentp, minN - 1, flp, সত্য, nullptr, sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = সত্য;
            SvaStateVertex* ধ্রুব checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            currentp = checkVtxp;
        }
        যদি (!hasMax) {
            currentp->m_isUnbounded = সত্য;  // [->N] waits unboundedly
            m_inUnboundedScope = সত্য;
            প্রত্যাগম {currentp, nullptr, {}};
        }

        // [->M:N]: the range ring holds matches from M through N and advances
        // only on expr, preserving arbitrarily long gaps between occurrences.
        SvaStateVertex* ধ্রুব mergeVtxp = scopedCreateVertex();
        guardedLink(currentp, mergeVtxp, flp);  // accept at match_M
        যদি (maxN > minN) {
            SvaStateVertex* ধ্রুব ringVtxp
                = addDelayChain(currentp, maxN - minN + 1, flp, অসত্য, nullptr,
                                sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = সত্য;
            SvaStateVertex* ধ্রুব checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            guardedLink(checkVtxp, mergeVtxp, flp);
        }
        mergeVtxp->m_isUnbounded = সত্য;  // [->M:N] still has unbounded waits between matches
        m_inUnboundedScope = সত্য;
        প্রত্যাগম {mergeVtxp, nullptr, {}};
    }

    // True if the sequence has a zero-minimum consecutive repetition, which may match empty
    স্থির তার্কিক hasZeroMinRep(ধ্রুব Ast_গ্রন্থি_রাশিমালা* seqp) {
        প্রত্যাগম seqp->exists(
            [](ধ্রুব AstSConsRep* repp) { প্রত্যাগম getConstUInt(repp->countp()) == 0; });
    }

    // True if a leaf's condition has no implication or multi-cycle sequence in it
    // (a leaf can be e.g. 'not (x |-> y)')
    স্থির তার্কিক isPlainBoolean(ধ্রুব Ast_গ্রন্থি_রাশিমালা* condp) {
        প্রত্যাগম !condp->exists(
            [](ধ্রুব Ast_গ্রন্থি_রাশিমালা* np) { প্রত্যাগম np->isMultiCycleSva() || VN_IS(np, Implication); });
    }

    // Build merge vertex for SOr / LogOr: both branches feed into one vertex.
    BuildResult buildOrMerge(Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* rhsp, SvaStateVertex* entryVtxp,
                             File_সারি* flp, তার্কিক wantBoolLeaf) {
        ধ্রুব BuildResult lhs = build_রাশিমালা(lhsp, entryVtxp);
        ধ্রুব BuildResult rhs = build_রাশিমালা(rhsp, entryVtxp);
        যদি (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            প্রত্যাগম BuildResult::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP
        // IEEE 1800-2023 16.14.3: a cover sequence counts every end-of-match. A
        // sequence operand of 'or' can end more than once, but only its final
        // end reaches the merge vertex below, so reject sequence operands rather
        // than under-count. Plain boolean disjunction has one end per cycle and
        // is handled by the OR-fold.
        ধ্রুব তার্কিক booleanOnly = lhs.termVertexp == entryVtxp && lhs.finalCondp
                                 && rhs.termVertexp == entryVtxp && rhs.finalCondp;
        যদি (m_isCoverSeq && !booleanOnly) {
            warnEndpointUnsupported(flp, "a sequence operand of 'or'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            প্রত্যাগম BuildResult::failWithError();
        }
        যদি (booleanOnly && wantBoolLeaf) {
            যদি (isPlainBoolean(lhs.finalCondp) && isPlainBoolean(rhs.finalCondp)) {
                // As a |-> / |=> or top-level body, a disjunction of two plain booleans must
                // be able to reject: return it as a boolean finalCondp. The merge vertex
                // below drops the attempt silently when both operands are false.
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব orCondp
                    = নূতন AstLogOr{flp, lhs.finalCondp->clone_বৃক্ষ_Pure(অসত্য),
                                   rhs.finalCondp->clone_বৃক্ষ_Pure(অসত্য)};
                freeUnlinkedCondp(lhs.finalCondp);
                freeUnlinkedCondp(rhs.finalCondp);
                প্রত্যাগম {entryVtxp, orCondp, {}};
            }
        }
        SvaStateVertex* ধ্রুব mergeVtxp = scopedCreateVertex();
        যদি (booleanOnly) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsp,
                        "Single-cycle SOr operands must have finalCondp");
            guardedLink(entryVtxp, mergeVtxp,
                        নূতন AstLogOr{flp, sampled(lhs.finalCondp->clone_বৃক্ষ_Pure(অসত্য)),
                                     sampled(rhs.finalCondp->clone_বৃক্ষ_Pure(অসত্য))},
                        flp);
        } অন্যথা {
            যদি (lhs.finalCondp) {
                guardedLink(lhs.termVertexp, mergeVtxp,
                            sampled(lhs.finalCondp->clone_বৃক্ষ_Pure(অসত্য)), flp);
            } অন্যথা {
                guardedLink(lhs.termVertexp, mergeVtxp, flp);
            }
            যদি (rhs.finalCondp) {
                guardedLink(rhs.termVertexp, mergeVtxp,
                            sampled(rhs.finalCondp->clone_বৃক্ষ_Pure(অসত্য)), flp);
            } অন্যথা {
                guardedLink(rhs.termVertexp, mergeVtxp, flp);
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        প্রত্যাগম {mergeVtxp, nullptr, {}};
    }

    // Build done-latch combiner for SAnd/SIntersect (IEEE 1800-2023 16.9.5).
    BuildResult buildAndCombiner(Ast_গ্রন্থি_রাশিমালা* lhsExprp, Ast_গ্রন্থি_রাশিমালা* rhsExprp,
                                 SvaStateVertex* entryVtxp, File_সারি* flp) {
        // Snapshot-restore scope so LHS liveness does not leak into RHS.
        ধ্রুব তার্কিক savedScope = m_inUnboundedScope;
        ধ্রুব BuildResult lhs = build_রাশিমালা(lhsExprp, entryVtxp);
        ধ্রুব তার্কিক lhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope;
        ধ্রুব BuildResult rhs = build_রাশিমালা(rhsExprp, entryVtxp);
        ধ্রুব তার্কিক rhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope || lhsScope || rhsScope;
        যদি (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            প্রত্যাগম BuildResult::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP

        // Single-cycle operands: use boolean AND (done-latch would fire across cycles).
        // If both operands stayed at entry, they must be boolean leaves which
        // buildExpr returns with finalCondp=nodep (non-null).
        যদি (lhs.termVertexp == entryVtxp && rhs.termVertexp == entryVtxp) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsExprp,
                        "Single-cycle SAnd operands must have finalCondp");
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp = নূতন AstLogAnd{flp, lhs.finalCondp->clone_বৃক্ষ_Pure(অসত্য),
                                                     rhs.finalCondp->clone_বৃক্ষ_Pure(অসত্য)};
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            প্রত্যাগম {entryVtxp, condp, {}};
        }
        // Range-delay mid-window sources in either sub-branch would need
        // to be folded into the latch's match-now signal, which the
        // current combiner does not support.
        যদি (!lhs.midSources.empty() || !rhs.midSources.empty()) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: ranged cycle delay in an operand of property 'and'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            প্রত্যাগম BuildResult::failWithError();
        }
        SvaStateVertex* ধ্রুব combVtxp = scopedCreateVertex();
        combVtxp->m_isAndCombiner = সত্য;
        combVtxp->m_andLhsTermp = lhs.termVertexp;
        combVtxp->m_andRhsTermp = rhs.termVertexp;
        যদি (lhs.finalCondp) combVtxp->m_andLhsCondp = lhs.finalCondp->clone_বৃক্ষ_Pure(অসত্য);
        যদি (rhs.finalCondp) combVtxp->m_andRhsCondp = rhs.finalCondp->clone_বৃক্ষ_Pure(অসত্য);
        যদি (lhs.termVertexp->m_isUnbounded || rhs.termVertexp->m_isUnbounded) {
            combVtxp->m_isUnbounded = সত্য;
        }
        // Wire terminal-boolean rejects to a dedicated sink so each side can fail
        // the AND independently. Skip for liveness or single-cycle operands
        // (single-cycle termVertexp == entryVtxp would fire every cycle).
        যদি (!combVtxp->m_isUnbounded) {
            তার্কিক needSink = অসত্য;
            ধ্রুব তার্কিক lhsMultiCycle = (lhs.termVertexp != entryVtxp);
            ধ্রুব তার্কিক rhsMultiCycle = (rhs.termVertexp != entryVtxp);
            যদি (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                needSink = সত্য;
            }
            যদি (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                needSink = সত্য;
            }
            যদি (needSink) {
                SvaStateVertex* ধ্রুব sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = সত্য;
                যদি (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* ধ্রুব ep = m_graph.addLink(
                        lhs.termVertexp, sinkVtxp, sampled(lhs.finalCondp->clone_বৃক্ষ_Pure(অসত্য)));
                    ep->m_rejectOnFail = সত্য;
                }
                যদি (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* ধ্রুব ep = m_graph.addLink(
                        rhs.termVertexp, sinkVtxp, sampled(rhs.finalCondp->clone_বৃক্ষ_Pure(অসত্য)));
                    ep->m_rejectOnFail = সত্য;
                }
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        প্রত্যাগম {combVtxp, nullptr, {}};
    }

    // Lower `seq1 within seq2` (IEEE 1800-2023 16.9.10) as:
    //   (OR_{i in 0..slack} 1 ##i seq1 ##(slack-i) 1) intersect seq2
    // Both operands must have fixed length (no ranged cycle delays).
    // The OR lives inside a single SIntersect so one AndCombiner done-latch
    // serves all offsets; lifting it out would double-count matches where
    // multiple offsets accept on the same seq2 end cycle.
    BuildResult buildSWithin(AstSWithin* nodep, SvaStateVertex* entryVtxp,
                             তার্কিক is_শীর্ষ_Level_Step = অসত্য) {
        ধ্রুব পূর্ণাঙ্ক innerLen = fixedLength(nodep->lhsp());
        ধ্রুব পূর্ণাঙ্ক outerLen = fixedLength(nodep->rhsp());
        যদি (innerLen < 0 || outerLen < 0) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: within with ranged cycle-delay operand");
            প্রত্যাগম BuildResult::failWithError();
        }
        যদি (innerLen > outerLen) {
            প্রত্যাগম buildNeverMatchIntersect(
                nodep, entryVtxp, is_শীর্ষ_Level_Step,
                "the inner sequence is longer than the outer sequence");
        }
        File_সারি* ধ্রুব flp = nodep->fileline();
        ধ্রুব পূর্ণাঙ্ক slack = outerLen - innerLen;
        Ast_গ্রন্থি_রাশিমালা* innerOrp = nullptr;
        প্রতি (পূর্ণাঙ্ক i = 0; i <= slack; ++i) {
            ধ্রুব পূর্ণাঙ্ক postPad = slack - i;
            Ast_গ্রন্থি_রাশিমালা* branchp = nodep->lhsp()->clone_বৃক্ষ_Pure(অসত্য);
            যদি (i > 0) {
                AstConst* ধ্রুব prePadp = নূতন AstConst{flp, AstConst::BitTrue{}};
                AstDelay* ধ্রুব delayp
                    = নূতন AstDelay{flp, নূতন AstConst{flp, static_cast<uint32_t>(i)}, সত্য};
                AstSExpr* ধ্রুব wrapped = নূতন AstSExpr{flp, prePadp, delayp, branchp};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            যদি (postPad > 0) {
                AstConst* ধ্রুব postTruep = নূতন AstConst{flp, AstConst::BitTrue{}};
                AstDelay* ধ্রুব delayp
                    = নূতন AstDelay{flp, নূতন AstConst{flp, static_cast<uint32_t>(postPad)}, সত্য};
                AstSExpr* ধ্রুব wrapped = নূতন AstSExpr{flp, branchp, delayp, postTruep};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            innerOrp = innerOrp ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নূতন AstSOr{flp, innerOrp, branchp})
                                : branchp;
        }
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব outerClonep = nodep->rhsp()->clone_বৃক্ষ_Pure(অসত্য);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব combinedp = নূতন AstSIntersect{flp, innerOrp, outerClonep};
        BuildResult result = build_রাশিমালা(combinedp, entryVtxp, is_শীর্ষ_Level_Step);
        VL_DO_DANGLING(combinedp->delete_বৃক্ষ(), combinedp);
        // When both operands are plain booleans, buildAndCombiner returns a
        // freshly-allocated AstAnd as finalCondp with no parent. Callers up
        // the chain clone-and-discard finalCondp, so leave it parent-less and
        // it leaks; anchor it in the graph now via an edge.
        যদি (result.valid() && result.finalCondp && !result.finalCondp->backp()) {
            SvaStateVertex* ধ্রুব wrapVtxp = scopedCreateVertex();
            guardedLink(result.termVertexp, wrapVtxp, sampled(result.finalCondp), flp);
            result = {wrapVtxp, nullptr, result.midSources};
        }
        প্রত্যাগম result;
    }

    // Collect the boolean leaf checks of a fixed-length sequence keyed by their
    // clock offset from the start. Returns false for anything other than nested
    // AstSExpr with fixed cycle delays over boolean leaves (e.g. throughout).
    স্থির তার্কিক flattenFixedSeq(Ast_গ্রন্থি_রাশিমালা* nodep, পূর্ণাঙ্ক baseOffset,
                                std::map<পূর্ণাঙ্ক, std::vector<Ast_গ্রন্থি_রাশিমালা*>>& out) {
        যদি (AstSExpr* ধ্রুব sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ধ্রুব delayp = VN_CAST(sexprp->delayp(), Delay);
            যদি (!delayp || !delayp->isCycleDelay() || delayp->isUnbounded()) প্রত্যাগম অসত্য;
            ধ্রুব unsigned delayCycles = getConstUInt(delayp->lhsp());
            যদি (delayp->isRangeDelay() && getConstUInt(delayp->rhsp()) != delayCycles)
                প্রত্যাগম অসত্য;
            পূর্ণাঙ্ক preLen = 0;
            যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব prep = sexprp->preExprp()) {
                যদি (!flattenFixedSeq(prep, baseOffset, out)) প্রত্যাগম অসত্য;
                preLen = fixedLength(prep);
                যদি (preLen < 0) প্রত্যাগম অসত্য;
            }
            প্রত্যাগম flattenFixedSeq(sexprp->exprp(), baseOffset + preLen + delayCycles, out);
        }
        যদি (nodep->isMultiCycleSva()) প্রত্যাগম অসত্য;
        out[baseOffset].ঠেলো_back(nodep);
        প্রত্যাগম সত্য;
    }

    // Conjoin two equal-length fixed sequences into one: at each clock offset
    // AND the boolean checks of both operands (IEEE 1800-2023 16.9.6 -- both
    // operands match the same window). Returns null if either operand is not a
    // plain fixed sequence of boolean leaves.
    স্থির Ast_গ্রন্থি_রাশিমালা* conjoinFixedSeqs(Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* rhsp, File_সারি* flp) {
        std::map<পূর্ণাঙ্ক, std::vector<Ast_গ্রন্থি_রাশিমালা*>> checks;
        যদি (!flattenFixedSeq(lhsp, 0, checks) || !flattenFixedSeq(rhsp, 0, checks)) প্রত্যাগম nullptr;
        যদি (checks.empty()) প্রত্যাগম nullptr;
        Ast_গ্রন্থি_রাশিমালা* resultp = nullptr;
        পূর্ণাঙ্ক prevOffset = 0;
        প্রতি (ধ্রুব auto& offsetChecks : checks) {
            ধ্রুব পূর্ণাঙ্ক offset = offsetChecks.first;
            Ast_গ্রন্থি_রাশিমালা* condp = nullptr;
            প্রতি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব leafp : offsetChecks.second) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব clonep = leafp->clone_বৃক্ষ_Pure(অসত্য);
                যদি (!condp) {
                    condp = clonep;
                } অন্যথা {
                    condp = নূতন AstLogAnd{flp, condp, clonep};
                    condp->dtypeSetBit();
                }
            }
            যদি (!resultp) {
                যদি (offset > 0) {
                    AstDelay* ধ্রুব delayp = নূতন AstDelay{
                        flp, নূতন AstConst{flp, static_cast<uint32_t>(offset)}, /*isCycle=*/সত্য};
                    resultp
                        = নূতন AstSExpr{flp, নূতন AstConst{flp, AstConst::BitTrue{}}, delayp, condp};
                    resultp->dtypeSetBit();
                } অন্যথা {
                    resultp = condp;
                }
            } অন্যথা {
                AstDelay* ধ্রুব delayp = নূতন AstDelay{
                    flp, নূতন AstConst{flp, static_cast<uint32_t>(offset - prevOffset)},
                    /*isCycle=*/সত্য};
                resultp = নূতন AstSExpr{flp, resultp, delayp, condp};
                resultp->dtypeSetBit();
            }
            prevOffset = offset;
        }
        প্রত্যাগম resultp;
    }

    // `seq` is a simple ranged sequence `start ##[m:n] end` (start/end boolean,
    // start may be absent). Used to collapse a both-variable intersect to one
    // ranged delay.
    সংরচনা SimpleRanged final {
        তার্কিক ok = অসত্য;
        Ast_গ্রন্থি_রাশিমালা* startp = nullptr;  // may be null (absent start)
        Ast_গ্রন্থি_রাশিমালা* endp = nullptr;
    };
    স্থির SimpleRanged asSimpleRanged(Ast_গ্রন্থি_রাশিমালা* nodep) {
        AstSExpr* ধ্রুব sexprp = VN_CAST(nodep, SExpr);
        যদি (!sexprp) প্রত্যাগম {};
        AstDelay* ধ্রুব delayp = VN_CAST(sexprp->delayp(), Delay);
        যদি (!delayp || !delayp->isCycleDelay() || !delayp->isRangeDelay() || delayp->isUnbounded())
            প্রত্যাগম {};
        যদি (getConstUInt(delayp->lhsp()) == getConstUInt(delayp->rhsp())) প্রত্যাগম {};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব prep = sexprp->preExprp();
        যদি (prep && fixedLength(prep) != 0) প্রত্যাগম {};
        যদি (fixedLength(sexprp->exprp()) != 0) প্রত্যাগম {};
        প্রত্যাগম {সত্য, prep, sexprp->exprp()};
    }

    // Build the NFA for a synthesized intersect lowering tree, then free it.
    // buildExpr returns the terminal condition (finalCondp) by reference into the
    // tree; detach a clone so the tree can be freed here. The graph already holds
    // clones/hoists of every edge condition, so nothing else dangles.
    BuildResult build_From_Lowering_বৃক্ষ(Ast_গ্রন্থি_রাশিমালা* treep, SvaStateVertex* entryVtxp,
                                      তার্কিক is_শীর্ষ_Level_Step) {
        BuildResult result = build_রাশিমালা(treep, entryVtxp, is_শীর্ষ_Level_Step);
        যদি (result.valid() && result.finalCondp) {
            result.finalCondp = result.finalCondp->clone_বৃক্ষ_Pure(অসত্য);
        }
        VL_DO_DANGLING(treep->delete_বৃক্ষ(), treep);
        প্রত্যাগম result;
    }

    // Empty common-length intersection -- unequal fixed lengths, or disjoint
    // ranged lengths. IEEE 1800-2023 16.9.6 requires both operands to match
    // over a window of the same length, so with no common length the intersect
    // simply never matches. This is legal (matching nothing), not an error, so
    // lower to a constant false rather than rejecting legal code.
    BuildResult buildNeverMatchIntersect(Ast_গ্রন্থি_রাশিমালা* nodep, SvaStateVertex* entryVtxp,
                                         তার্কিক is_শীর্ষ_Level_Step, ধ্রুব বর্ণ* reason) {
        nodep->v3warn(NEVERMATCH, "Sequence can never match because " << reason << ".");
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব falsep = নূতন AstConst{nodep->fileline(), AstConst::BitFalse{}};
        প্রত্যাগম build_From_Lowering_বৃক্ষ(falsep, entryVtxp, is_শীর্ষ_Level_Step);
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
                                     তার্কিক is_শীর্ষ_Level_Step) {
        ধ্রুব std::pair<পূর্ণাঙ্ক, পূর্ণাঙ্ক> lhsRange = lengthRange(nodep->lhsp());
        ধ্রুব std::pair<পূর্ণাঙ্ক, পূর্ণাঙ্ক> rhsRange = lengthRange(nodep->rhsp());
        যদি (lhsRange.first < 0 || rhsRange.first < 0) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect with this variable-length operand");
            প্রত্যাগম BuildResult::failWithError();
        }
        ধ্রুব পূর্ণাঙ্ক lo = std::max(lhsRange.first, rhsRange.first);
        ধ্রুব পূর্ণাঙ্ক hi = std::min(lhsRange.second, rhsRange.second);
        যদি (lo > hi) {
            // Disjoint length ranges share no common length -> never matches.
            প্রত্যাগম buildNeverMatchIntersect(nodep, entryVtxp, is_শীর্ষ_Level_Step,
                                            "intersect operands have no common length");
        }
        File_সারি* ধ্রুব flp = nodep->fileline();
        যদি (lo == hi) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব lp = realizeAtLength(nodep->lhsp(), lo, lhsRange.first);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব rp = realizeAtLength(nodep->rhsp(), lo, rhsRange.first);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব conjp = conjoinFixedSeqs(lp, rp, flp);
            VL_DO_DANGLING(lp->delete_বৃক্ষ(), lp);
            VL_DO_DANGLING(rp->delete_বৃক্ষ(), rp);
            যদি (!conjp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: intersect operand is not a plain boolean sequence");
                প্রত্যাগম BuildResult::failWithError();
            }
            প্রত্যাগম build_From_Lowering_বৃক্ষ(conjp, entryVtxp, is_শীর্ষ_Level_Step);
        }
        ধ্রুব SimpleRanged sl = asSimpleRanged(nodep->lhsp());
        ধ্রুব SimpleRanged sr = asSimpleRanged(nodep->rhsp());
        যদি (!sl.ok || !sr.ok) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect of two sequences that each vary in length over a"
                          " range with internal structure");
            প্রত্যাগম BuildResult::failWithError();
        }
        ধ্রুব auto andBool = [&](Ast_গ্রন্থি_রাশিমালা* ap, Ast_গ্রন্থি_রাশিমালা* bp) -> Ast_গ্রন্থি_রাশিমালা* {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব aClonep
                = ap ? ap->clone_বৃক্ষ_Pure(অসত্য) : নূতন AstConst{flp, AstConst::BitTrue{}};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব bClonep
                = bp ? bp->clone_বৃক্ষ_Pure(অসত্য) : নূতন AstConst{flp, AstConst::BitTrue{}};
            AstLogAnd* ধ্রুব andp = নূতন AstLogAnd{flp, aClonep, bClonep};
            andp->dtypeSetBit();
            প্রত্যাগম andp;
        };
        AstDelay* ধ্রুব delayp = নূতন AstDelay{flp, নূতন AstConst{flp, static_cast<uint32_t>(lo)},
                                              /*isCycle=*/সত্য};
        delayp->rhsp(নূতন AstConst{flp, static_cast<uint32_t>(hi)});
        AstSExpr* ধ্রুব reducedp
            = নূতন AstSExpr{flp, andBool(sl.startp, sr.startp), delayp, andBool(sl.endp, sr.endp)};
        reducedp->dtypeSetBit();
        প্রত্যাগম build_From_Lowering_বৃক্ষ(reducedp, entryVtxp, is_শীর্ষ_Level_Step);
    }

    BuildResult buildThroughout(AstSThroughout* nodep, SvaStateVertex* entryVtxp,
                                তার্কিক is_শীর্ষ_Level_Step = অসত্য) {
        // Mark entryVtxp so "cond false at tick 0" is detected as throughout-drop.
        entryVtxp->m_throughoutConds.ঠেলো_back(nodep->lhsp()->clone_বৃক্ষ_Pure(অসত্য));
        m_temporal_Guard_স্তূপ.ঠেলো_back(nodep->lhsp());
        ধ্রুব BuildResult result = build_রাশিমালা(nodep->rhsp(), entryVtxp, is_শীর্ষ_Level_Step);
        m_temporal_Guard_স্তূপ.বের_করো_back();
        প্রত্যাগম result;
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
    BuildResult buildUntil(AstUntil* nodep, SvaStateVertex* entryVtxp, তার্কিক is_শীর্ষ_Level_Step) {
        File_সারি* ধ্রুব flp = nodep->fileline();
        যদি (!is_শীর্ষ_Level_Step) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            প্রত্যাগম BuildResult::failWithError();
        }
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব lhsp = nodep->lhsp();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsp = nodep->rhsp();
        ধ্রুব auto hasSeq = [](ধ্রুব Ast_গ্রন্থি_রাশিমালা* ep) {
            প্রত্যাগম ep->exists([](ধ্রুব Ast_গ্রন্থি_রাশিমালা* np) { প্রত্যাগম np->isMultiCycleSva(); });
        };
        যদি (hasSeq(lhsp) || hasSeq(rhsp)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            প্রত্যাগম BuildResult::failWithError();
        }

        ধ্রুব তার্কিক ov = nodep->isOverlapping();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব lhsBitp = nodep->lhsp();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsBitp = nodep->rhsp();
        // p hoist count: continue, require (ov: 1 use; nov: 1 use). At least 2 uses.
        AstVar* ধ্রুব pHoistp = tryHoistSampled(lhsBitp, flp, 2);
        // q hoist count: continue (1) + require nov (1) = 2; ov: continue only (1).
        AstVar* ধ্রুব qHoistp = ov ? nullptr : tryHoistSampled(rhsBitp, flp, 2);

        SvaStateVertex* ধ্রুব waitCp = scopedCreateVertex();
        SvaStateVertex* ধ্রুব waitRp = scopedCreateVertex();
        waitCp->m_isUnbounded = সত্য;
        waitRp->m_strongPending = nodep->isStrong() && !m_isCover;

        // Entry and back-loop Links carry no condition; throughout-folding still applies.
        guardedLink(entryVtxp, waitCp, flp);
        guardedLink(waitRp, waitCp, flp);

        // Continue clocked edge: p && !q advances to next-cycle wait.
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব contCondp
            = নূতন AstLogAnd{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                            নূতন AstLogNot{flp, sampledRefOrClone(qHoistp, rhsBitp, flp)}};
        guardedEdge(waitCp, waitRp, contCondp, flp);

        // Reject sink: fires when require-condition is false.
        SvaStateVertex* ধ্রুব sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = সত্য;
        Ast_গ্রন্থি_রাশিমালা* requireCondp;
        যদি (ov) {
            requireCondp = sampledRefOrClone(pHoistp, lhsBitp, flp);
        } অন্যথা {
            requireCondp = নূতন AstLogOr{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                                        sampledRefOrClone(qHoistp, rhsBitp, flp)};
        }
        SvaTransEdge* ধ্রুব rejEdgep = m_graph.addLink(waitCp, sinkVtxp, requireCondp);
        rejEdgep->m_rejectOnFail = সত্য;

        // Accept condition rides via finalCondp; assembleResult $sampled-wraps it.
        Ast_গ্রন্থি_রাশিমালা* acceptCondp;
        যদি (ov) {
            acceptCondp
                = নূতন AstLogAnd{flp, lhsBitp->clone_বৃক্ষ_Pure(অসত্য), rhsBitp->clone_বৃক্ষ_Pure(অসত্য)};
        } অন্যথা {
            acceptCondp = rhsBitp->clone_বৃক্ষ_Pure(অসত্য);
        }
        প্রত্যাগম {waitCp, acceptCondp, {}};
    }

    // IEEE 1800-2023 16.12.14 property abort operators. Sync and async share
    // the same NFA encoding: AstSampled already gives matured values at every
    // maturing clocking event, and async firing "between clocks" is not
    // observable in a cycle-based model. VAbortKind selects accept vs reject
    // verdict (sync vs async only changes the user-visible spelling).

    // Build `condp && !outer_1 && !outer_2 ...` (unsampled).
    Ast_গ্রন্থি_রাশিমালা* abort_Fire_রাশিমালা(Ast_গ্রন্থি_রাশিমালা* condp, File_সারি* flp) {
        Ast_গ্রন্থি_রাশিমালা* resultp = condp->clone_বৃক্ষ_Pure(অসত্য);
        প্রতি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব op : m_outer_Abort_স্তূপ)
            resultp = নূতন AstLogAnd{flp, resultp, নূতন AstLogNot{flp, op->clone_বৃক্ষ_Pure(অসত্য)}};
        প্রত্যাগম resultp;
    }

    // True when unguarded ring-wide rejection or a same-tick Link chain already accounts the
    // attempt: a required-step Link covers both outcomes; followed-by pairs both edges.
    স্থির তার্কিক chainAccountsSource(ধ্রুব SvaStateVertex* srcp,
                                    ধ্রুব std::unordered_set<ধ্রুব V3GraphEdge*>& preEdges) {
        যদি (srcp->m_delayRingSize && srcp->m_throughoutConds.empty()) প্রত্যাগম সত্য;
        প্রতি (ধ্রুব V3GraphEdge& edger : srcp->inEdges()) {
            যদি (preEdges.গুনতি(&edger)) অনুবর্ত;
            ধ্রুব SvaTransEdge& tedger = static_cast<ধ্রুব SvaTransEdge&>(edger);
            যদি (tedger.m_consumesCycle) অনুবর্ত;
            ধ্রুব auto* ধ্রুব fromp = static_cast<ধ্রুব SvaStateVertex*>(tedger.fromVtxp());
            যদি (fromp->m_abortRejectp) প্রত্যাগম সত্য;
        }
        তার্কিক plainNonSink = অসত্য;
        তার্কিক markedSink = অসত্য;
        প্রতি (ধ্রুব V3GraphEdge& edger : srcp->outEdges()) {
            যদি (preEdges.গুনতি(&edger)) অনুবর্ত;
            ধ্রুব SvaTransEdge& tedger = static_cast<ধ্রুব SvaTransEdge&>(edger);
            যদি (tedger.m_consumesCycle) অনুবর্ত;
            ধ্রুব তার্কিক sink = static_cast<ধ্রুব SvaStateVertex*>(tedger.toVtxp())->m_isRejectSink;
            যদি (tedger.m_rejectOnFail) {
                যদি (!sink) প্রত্যাগম সত্য;
                markedSink = সত্য;
            } অন্যথা যদি (!sink) {
                যদি (srcp->m_abortRejectp) প্রত্যাগম সত্য;
                plainNonSink = সত্য;
            }
        }
        প্রত্যাগম plainNonSink && markedSink;
    }

    // Reject edge: fires when the source is live and the abort samples true.
    শূন্য addAbortRejectEdge(SvaStateVertex* srcp, SvaStateVertex* sinkp, Ast_গ্রন্থি_রাশিমালা* condp,
                            File_সারি* flp) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব notFirep = নূতন AstLogNot{flp, sampled(abort_Fire_রাশিমালা(condp, flp))};
        m_graph.addLink(srcp, sinkp, notFirep)->m_rejectOnFail = সত্য;
        প্রত্যাগম;
    }

    // On the fire tick: kill body threads; accept kinds also forgive step misses.
    শূন্য gateBodyEdgesOnAbort(ধ্রুব std::unordered_set<ধ্রুব V3GraphEdge*>& preEdges,
                              Ast_গ্রন্থি_রাশিমালা* condp, VAbortKind kind, File_সারি* flp) {
        প্রতি (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            প্রতি (V3GraphEdge& edger : vtxr.outEdges()) {
                যদি (preEdges.গুনতি(&edger)) অনুবর্ত;
                SvaTransEdge* ধ্রুব tedgep = static_cast<SvaTransEdge*>(&edger);
                যদি (tedgep->m_rejectOnFail) {
                    যদি (!kind.isAccept()) অনুবর্ত;
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব firep = sampled(abort_Fire_রাশিমালা(condp, flp));
                    tedgep->m_condp
                        = tedgep->m_condp ? নূতন AstLogOr{flp, tedgep->m_condp, firep} : firep;
                } অন্যথা যদি (tedgep->m_consumesCycle) {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব notFirep
                        = নূতন AstLogNot{flp, sampled(abort_Fire_রাশিমালা(condp, flp))};
                    tedgep->m_condp = tedgep->m_condp
                                          ? নূতন AstLogAnd{flp, tedgep->m_condp, notFirep}
                                          : notFirep;
                }
            }
        }
    }

    BuildResult buildAbortOn(Ast_গ্রন্থি_রাশিমালা* condp, Ast_গ্রন্থি_রাশিমালা* bodyp, SvaStateVertex* entryVtxp,
                             VAbortKind kind, File_সারি* flp, তার্কিক is_শীর্ষ_Level_Step) {
        // Snapshot pre-body vertices/edges so post-build diff yields the body's sub-NFA.
        std::unordered_set<ধ্রুব V3GraphVertex*> preExisting;
        std::unordered_set<ধ্রুব V3GraphEdge*> preEdges;
        প্রতি (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            preExisting.ঢোকাও(&vtxr);
            প্রতি (V3GraphEdge& edger : vtxr.outEdges()) preEdges.ঢোকাও(&edger);
        }

        m_outer_Abort_স্তূপ.ঠেলো_back(condp);
        ধ্রুব BuildResult bodyResult = build_রাশিমালা(bodyp, entryVtxp, is_শীর্ষ_Level_Step);
        m_outer_Abort_স্তূপ.বের_করো_back();
        যদি (!bodyResult.valid()) প্রত্যাগম bodyResult;

        gateBodyEdgesOnAbort(preEdges, condp, kind, flp);

        // Live-thread sources for the abort edge: entry + new body vertices,
        // minus reject sinks (they carry reject fuel, not live-thread fuel).
        std::vector<SvaStateVertex*> abortSources;
        abortSources.ঠেলো_back(entryVtxp);
        প্রতি (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            যদি (preExisting.গুনতি(&vtxr)) অনুবর্ত;
            auto* ধ্রুব sp = static_cast<SvaStateVertex*>(&vtxr);
            যদি (sp->m_delayRingSize || !sp->m_throughoutConds.empty()) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব firep = abort_Fire_রাশিমালা(condp, flp);
                sp->m_abortClearp
                    = sp->m_abortClearp ? নূতন AstLogOr{flp, sp->m_abortClearp, firep} : firep;
            }
            যদি (!kind.isAccept()
                && ((sp->m_delayRingSize && sp->m_throughoutConds.empty())
                    || sp->m_replayAbortReject)) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব firep = abort_Fire_রাশিমালা(condp, flp);
                sp->m_abortRejectp
                    = sp->m_abortRejectp ? নূতন AstLogOr{flp, sp->m_abortRejectp, firep} : firep;
            }
            যদি (sp->m_isRejectSink) অনুবর্ত;
            abortSources.ঠেলো_back(sp);
        }

        auto sampledAbortFire = [&]() -> Ast_গ্রন্থি_রাশিমালা* {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব রাশিমালা = abort_Fire_রাশিমালা(condp, flp);
            প্রত্যাগম sampled(রাশিমালা);
        };

        যদি (kind.isAccept()) {
            // Match-only sink fed by $sampled(abort-fire) from every live source;
            // registered as midSource so it never contributes a reject.
            SvaStateVertex* ধ্রুব acceptSinkp = scopedCreateVertex();
            প্রতি (SvaStateVertex* ধ্রুব srcp : abortSources)
                guardedLink(srcp, acceptSinkp, sampledAbortFire(), flp);
            std::vector<SvaStateVertex*> midSources = bodyResult.midSources;
            midSources.ঠেলো_back(acceptSinkp);
            Ast_গ্রন্থি_রাশিমালা* finalCondp = bodyResult.finalCondp;
            যদি (finalCondp) {
                যদি (finalCondp->backp()) finalCondp = finalCondp->clone_বৃক্ষ_Pure(অসত্য);
                finalCondp = নূতন AstLogOr{flp, finalCondp, abort_Fire_রাশিমালা(condp, flp)};
            }
            প্রত্যাগম {bodyResult.termVertexp, finalCondp, std::move(midSources)};
        }

        // rejectOnFail treats m_condp as the success condition and fires on
        // !condp, so the edge carries !sampledAbortFire().
        SvaStateVertex* ধ্রুব rejectSinkp = m_graph.createStateVertex();
        rejectSinkp->m_isRejectSink = সত্য;
        প্রতি (SvaStateVertex* ধ্রুব srcp : abortSources)
            যদি (!chainAccountsSource(srcp, preEdges))
                addAbortRejectEdge(srcp, rejectSinkp, condp, flp);
        Ast_গ্রন্থি_রাশিমালা* finalCondp = bodyResult.finalCondp;
        যদি (finalCondp) {
            যদি (finalCondp->backp()) finalCondp = finalCondp->clone_বৃক্ষ_Pure(অসত্য);
            finalCondp
                = নূতন AstLogAnd{flp, finalCondp, নূতন AstLogNot{flp, abort_Fire_রাশিমালা(condp, flp)}};
        }
        প্রত্যাগম {bodyResult.termVertexp, finalCondp, bodyResult.midSources};
    }

সার্বজনিক:
    // Free a dropped sub-result condition that is not linked into the AST
    // (abort folds synthesize unparented finalCondp trees).
    স্থির শূন্য freeUnlinkedCondp(Ast_গ্রন্থি_রাশিমালা* condp) {
        যদি (condp && !condp->backp()) VL_DO_DANGLING(condp->delete_বৃক্ষ(), condp);
    }

    SvaNfaBuilder(SvaGraph& graph, Ast_গ্রন্থি_Module* modp, V3UniqueNames& propTempNames,
                  তার্কিক isCoverSeq = অসত্য, তার্কিক isSeqEvent = অসত্য, তার্কিক isCover = অসত্য)
        : m_graph{graph}
        , m_modp{modp}
        , m_propTempNames{propTempNames}
        , m_isCover{isCover}
        , m_isCoverSeq{isCoverSeq}
        , m_isSeqEvent{isSeqEvent} {}

    // Reset scope between antecedent and consequent: liveness must not leak.
    // m_outerAbortStack survives: an abort wrapping the implication covers the
    // consequent too (IEEE 1800-2023 16.12.14).
    শূন্য resetScope() {
        m_inUnboundedScope = অসত্য;
        m_temporal_Guard_স্তূপ.clear();
    }

    BuildResult build_রাশিমালা(Ast_গ্রন্থি_রাশিমালা* nodep, SvaStateVertex* entryVtxp,
                          তার্কিক is_শীর্ষ_Level_Step = অসত্য) {
        VL_RESTORER(m_wantBoolLeaf);
        ধ্রুব তার্কিক wantBoolLeaf = m_wantBoolLeaf;
        m_wantBoolLeaf = অসত্য;
        যদি (AstSExpr* ধ্রুব sexprp = VN_CAST(nodep, SExpr)) {
            প্রত্যাগম buildSExpr(sexprp, entryVtxp, is_শীর্ষ_Level_Step);
        }
        যদি (AstSConsRep* ধ্রুব repp = VN_CAST(nodep, SConsRep)) {
            প্রত্যাগম buildConsRep(repp, entryVtxp, is_শীর্ষ_Level_Step);
        }
        যদি (AstPropAlways* ধ্রুব alwaysp = VN_CAST(nodep, PropAlways)) {
            প্রত্যাগম buildPropAlways(alwaysp, entryVtxp, is_শীর্ষ_Level_Step);
        }
        যদি (AstSGotoRep* ধ্রুব repp = VN_CAST(nodep, SGotoRep)) {
            প্রত্যাগম buildGotoRep(repp, entryVtxp);
        }
        যদি (AstSThroughout* ধ্রুব throughoutp = VN_CAST(nodep, SThroughout)) {
            প্রত্যাগম buildThroughout(throughoutp, entryVtxp, is_শীর্ষ_Level_Step);
        }
        যদি (AstSOr* ধ্রুব orp = VN_CAST(nodep, SOr)) {
            প্রত্যাগম buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        যদি (AstLogOr* ধ্রুব orp = VN_CAST(nodep, LogOr)) {
            প্রত্যাগম buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        যদি (AstSAnd* ধ্রুব andp = VN_CAST(nodep, SAnd)) {
            প্রত্যাগম buildAndCombiner(andp->lhsp(), andp->rhsp(), entryVtxp, andp->fileline());
        }
        যদি (AstSIntersect* ধ্রুব intp = VN_CAST(nodep, SIntersect)) {
            // IEEE 1800-2023 16.9.6: both operands match over one window with
            // equal start and end (equal length). Lower to a single sequence
            // that conjoins both operands' per-cycle checks -- correct under
            // concurrent attempts, where the done-latch combiner conflates the
            // two operands' start times and over-accepts. The combiner remains
            // only as a fallback for operands that do not flatten.
            ধ্রুব পূর্ণাঙ্ক lhsLen = fixedLength(intp->lhsp());
            ধ্রুব পূর্ণাঙ্ক rhsLen = fixedLength(intp->rhsp());
            যদি (lhsLen >= 0 && rhsLen >= 0) {
                যদি (lhsLen != rhsLen) {
                    // Unequal fixed lengths share no common length -> never matches.
                    প্রত্যাগম buildNeverMatchIntersect(intp, entryVtxp, is_শীর্ষ_Level_Step,
                                                    "intersect operands have no common length");
                }
                যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব conjp
                    = conjoinFixedSeqs(intp->lhsp(), intp->rhsp(), intp->fileline())) {
                    প্রত্যাগম build_From_Lowering_বৃক্ষ(conjp, entryVtxp, is_শীর্ষ_Level_Step);
                }
                প্রত্যাগম buildAndCombiner(intp->lhsp(), intp->rhsp(), entryVtxp, intp->fileline());
            }
            প্রত্যাগম buildVarLenIntersect(intp, entryVtxp, is_শীর্ষ_Level_Step);
        }
        যদি (AstSWithin* ধ্রুব withinp = VN_CAST(nodep, SWithin)) {
            প্রত্যাগম buildSWithin(withinp, entryVtxp, is_শীর্ষ_Level_Step);
        }
        যদি (AstAbortOn* ধ্রুব ap = VN_CAST(nodep, AbortOn)) {
            প্রত্যাগম buildAbortOn(ap->condp(), ap->propp(), entryVtxp, ap->kind(), ap->fileline(),
                                is_শীর্ষ_Level_Step);
        }
        যদি (VN_IS(nodep, SNonConsRep)) প্রত্যাগম BuildResult::fail();
        যদি (AstImplication* ধ্রুব implp = VN_CAST(nodep, Implication)) {
            প্রত্যাগম buildImplicationEdges(implp->lhsp(), implp->rhsp(), entryVtxp,
                                         implp->isOverlapped(), implp->isFollowedBy(),
                                         implp->lhsp(), implp->fileline());
        }
        যদি (AstUntil* ধ্রুব untilp = VN_CAST(nodep, Until)) {
            প্রত্যাগম buildUntil(untilp, entryVtxp, is_শীর্ষ_Level_Step);
        }
        // Boolean leaf (including LogAnd): return as finalCond
        প্রত্যাগম {entryVtxp, nodep, {}};
    }

    // Wire an implication / followed-by from `entryVtxp`: builds the antecedent,
    // emits the match-link (and for followed-by the reject-sink edge), inserts a
    // delay vertex for non-overlapped forms, and builds the body. Used both for
    // nested AstImplication in pexpr position and for the top-level assertion
    // antecedent -- `errorNodep` anchors the "unsupported sequence antecedent"
    // error, which differs between the two call sites.
    BuildResult buildImplicationEdges(Ast_গ্রন্থি_রাশিমালা* antExprp, Ast_গ্রন্থি_রাশিমালা* bodyExprp,
                                      SvaStateVertex* entryVtxp, তার্কিক isOverlapped,
                                      তার্কিক isFollowedBy, Ast_গ্রন্থি* errorNodep, File_সারি* flp) {
        ধ্রুব BuildResult antResult = build_রাশিমালা(antExprp, entryVtxp);
        যদি (!antResult.valid()) প্রত্যাগম antResult;

        // Followed-by requires pure-boolean antecedent for non-vacuous-fail at
        // the attempt-start cycle. IEEE 1800-2023 16.12.9 permits a multi-cycle
        // sequence LHS, so this is an implementation gap rather than illegal SV.
        যদি (isFollowedBy && antResult.termVertexp != entryVtxp) {
            errorNodep->v3warn(E_UNSUPPORTED,
                               "Unsupported: sequence expression as antecedent of followed-by"
                               " (#-# / #=#) (IEEE 1800-2023 16.12.9)");
            প্রত্যাগম BuildResult::failWithError();
        }
        UASSERT_OBJ(!isFollowedBy || antResult.finalCondp, errorNodep,
                    "followed-by antecedent terminal at entry must carry finalCondp");

        // Use raw createStateVertex() so trigVtxp starts without liveness --
        // reaching the antecedent terminal is a definitive event.
        SvaStateVertex* ধ্রুব trigVtxp = m_graph.createStateVertex();
        যদি (antResult.finalCondp) {
            m_graph.addLink(antResult.termVertexp, trigVtxp,
                            sampled(antResult.finalCondp->clone_বৃক্ষ_Pure(অসত্য)));
            // Followed-by non-vacuous fail: rejectOnFail fires when the attempt
            // is live (termVtx reachable) and sampled(antecedent) is false.
            যদি (isFollowedBy) {
                SvaStateVertex* ধ্রুব sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = সত্য;
                SvaTransEdge* ধ্রুব ep
                    = m_graph.addLink(antResult.termVertexp, sinkVtxp,
                                      sampled(antResult.finalCondp->clone_বৃক্ষ_Pure(অসত্য)));
                ep->m_rejectOnFail = সত্য;
            }
            // finalCondp is cloned into the Sampled nodes; if the original is
            // not parented anywhere in the AST anymore it must be freed here
            // or ASan flags it as a leak (e.g. t_sequence_bool_ops).
            যদি (!antResult.finalCondp->backp()) {
                VL_DO_DANGLING(antResult.finalCondp->delete_বৃক্ষ(), antResult.finalCondp);
            }
        } অন্যথা {
            m_graph.addLink(antResult.termVertexp, trigVtxp);
        }
        resetScope();

        SvaStateVertex* bodyEntryp = trigVtxp;
        যদি (!isOverlapped) {
            SvaStateVertex* ধ্রুব delayVtxp = m_graph.createStateVertex();
            m_graph.addClockedEdge(trigVtxp, delayVtxp);
            bodyEntryp = delayVtxp;
        }
        // An empty antecedent match must not start the consequent, but a zero-minimum
        // repetition's empty match is not handled here; keep the merge vertex for those
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = !isFollowedBy && !hasZeroMinRep(antExprp);
        প্রত্যাগম build_রাশিমালা(bodyExprp, bodyEntryp, /*isTopLevelStep=*/সত্য);
    }

    BuildResult build(Ast_গ্রন্থি_রাশিমালা* exprp) {
        m_graph.m_startVertexp = scopedCreateVertex();
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = সত্য;
        প্রত্যাগম build_রাশিমালা(exprp, m_graph.m_startVertexp, /*isTopLevelStep=*/সত্য);
    }
};

//######################################################################
// NFA Lowering (converts NFA graph to synthesizable AstAlways blocks)

বর্গ SvaNfaLowering final {
    Ast_গ্রন্থি_Module* ধ্রুব m_modp;  // Module to add state vars and always blocks to
    Ast_গ্রন্থি_DType* ধ্রুব m_u32DTypep;  // Shared unsigned counter dtype
    V3UniqueNames m_names{"__Vnfa"};
    size_t m_statDelayRingEdgeVisits = 0;  // Delay-ring incoming edges visited

    // Per-lowering shared context (passed to phase sub-functions)
    // Per-vertex lowering state is stored in SvaVertexData and accessed via
    // V3GraphVertex::userp() (see SvaStateVertex::datap()).
    সংরচনা LowerCtx final {
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
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব statep = stateExprp->clone_বৃক্ষ_Pure(অসত্য);
        যদি (!condp) প্রত্যাগম statep;
        প্রত্যাগম নূতন AstLogAnd{flp, statep, sampled(condp->clone_বৃক্ষ_Pure(অসত্য))};
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* andCond(File_সারি* flp, Ast_গ্রন্থি_রাশিমালা* exprp, Ast_গ্রন্থি_রাশিমালা* condp) {
        যদি (!condp) প্রত্যাগম exprp;
        প্রত্যাগম নূতন AstLogAnd{flp, exprp, condp->clone_বৃক্ষ_Pure(অসত্য)};
    }
    // bp is always non-null; only ap can be null (serving as accumulator).
    স্থির Ast_গ্রন্থি_রাশিমালা* orExprs(File_সারি* flp, Ast_গ্রন্থি_রাশিমালা* ap, Ast_গ্রন্থি_রাশিমালা* bp) {
        যদি (!ap) প্রত্যাগম bp;
        প্রত্যাগম নূতন AstLogOr{flp, ap, bp};
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* addCountp(File_সারি* ধ্রুব flp, Ast_গ্রন্থি_রাশিমালা* ধ্রুব totalCountp,
                                  Ast_গ্রন্থি_রাশিমালা* ধ্রুব contributionp,
                                  Ast_গ্রন্থি_রাশিমালা* ধ্রুব enablep = nullptr) {
        // contribution = enable ? contribution : 0;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব enabledContributionp
            = enablep ? নূতন AstCond{flp, enablep, contributionp,
                                    newTypedConstp(flp, contributionp->dtypep(), 0)}
                      : contributionp;
        যদি (!totalCountp) প্রত্যাগম enabledContributionp;
        প্রত্যাগম নূতন AstAdd{flp, totalCountp, enabledContributionp};
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* gateCheckedCountp(File_সারি* ধ্রুব flp, Ast_গ্রন্থি_রাশিমালা* ধ্রুব countp,
                                          Ast_গ্রন্থি_রাশিমালা*& overflowp, Ast_গ্রন্থি_রাশিমালা* ধ্রুব enablep) {
        যদি (overflowp) {
            overflowp = নূতন AstLogAnd{flp, enablep->clone_বৃক্ষ_Pure(অসত্য), overflowp};
        }
        প্রত্যাগম addCountp(flp, nullptr, countp, enablep);
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* addCheckedCountp(File_সারি* ধ্রুব flp, Ast_গ্রন্থি_রাশিমালা* ধ্রুব totalCountp,
                                         Ast_গ্রন্থি_রাশিমালা* ধ্রুব contributionp, Ast_গ্রন্থি_রাশিমালা*& overflowp,
                                         Ast_গ্রন্থি_রাশিমালা* ধ্রুব contributionOverflowp = nullptr) {
        যদি (contributionOverflowp) { overflowp = orExprs(flp, overflowp, contributionOverflowp); }
        যদি (!totalCountp) প্রত্যাগম contributionp;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব oldTotalp = totalCountp->clone_বৃক্ষ_Pure(অসত্য);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব resultp = নূতন AstAdd{flp, totalCountp, contributionp};
        overflowp
            = orExprs(flp, overflowp, নূতন AstLt{flp, resultp->clone_বৃক্ষ_Pure(অসত্য), oldTotalp});
        প্রত্যাগম resultp;
    }
    AstIf* new_গুনতি_Overflow_Warning(File_সারি* ধ্রুব flp, Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp) ধ্রুব {
        AstDisplay* ধ্রুব dispp = নূতন AstDisplay{
            flp, VDisplayType::DT_WARNING,
            "Cover sequence match count overflowed 32-bit storage; results may be incorrect.",
            nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstIf* ধ্রুব ifp = নূতন AstIf{flp, condp, dispp};
        ifp->branchPred(VBranchPred::BP_UNLIKELY);
        প্রত্যাগম ifp;
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* killActive(LowerCtx& c) {
        প্রত্যাগম নূতন AstNeq{c.flp, নূতন AstVarRef{c.flp, c.killVarp, VAccess::READ},
                          assertKillGet(c.flp, c.assertType, c.directiveType)};
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* notKillActive(LowerCtx& c) { প্রত্যাগম নূতন AstLogNot{c.flp, killActive(c)}; }
    স্থির Ast_গ্রন্থি_রাশিমালা* gateNotKill(LowerCtx& c, Ast_গ্রন্থি_রাশিমালা* exprp) {
        যদি (!exprp) প্রত্যাগম nullptr;
        প্রত্যাগম নূতন AstLogAnd{c.flp, exprp, notKillActive(c)};
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* nextRingIndex(File_সারি* flp, Ast_গ্রন্থি_রাশিমালা* idxExprp, uint32_t size) {
        ধ্রুব auto u32Const = [flp](uint32_t মান) {
            প্রত্যাগম নূতন AstConst{flp, AstConst::Widthed_মান{}, 32, মান};
        };
        UASSERT_OBJ(size > 0, idxExprp, "Ring size must be positive");
        যদি (size == 1) {
            VL_DO_DANGLING(idxExprp->delete_বৃক্ষ(), idxExprp);
            প্রত্যাগম u32Const(0);
        }
        // idx == size - 1 ? 0 : idx + 1
        AstAdd* ধ্রুব addp = নূতন AstAdd{flp, idxExprp->clone_বৃক্ষ_Pure(অসত্য), u32Const(1)};
        addp->dtypeFrom(idxExprp);
        AstCond* ধ্রুব condp
            = নূতন AstCond{flp, নূতন AstEq{flp, idxExprp, u32Const(size - 1)}, u32Const(0), addp};
        condp->dtypeFrom(idxExprp);
        প্রত্যাগম condp;
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* delayRingSlot(File_সারি* flp, SvaStateVertex* vtxp, Ast_গ্রন্থি_রাশিমালা* idxExprp,
                                      VAccess access = VAccess::READ) {
        // ring[idx]
        AstVar* ধ্রুব ringp = vtxp->datap()->delayRingVarp;
        AstVarRef* ধ্রুব refp = নূতন AstVarRef{flp, ringp, access};
        যদি (vtxp->datap()->delayRingCounts) প্রত্যাগম নূতন AstArraySel{flp, refp, idxExprp};
        প্রত্যাগম নূতন AstSel{flp, refp, idxExprp, 1};
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* delay_Ring_Head_গুনতি(File_সারি* flp, SvaStateVertex* vtxp) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব headp
            = নূতন AstVarRef{flp, vtxp->datap()->delayRingHeadVarp, VAccess::READ};
        যদি (vtxp->datap()->delayRingCounts) প্রত্যাগম headp;
        প্রত্যাগম নূতন AstExtend{flp, headp, vtxp->datap()->delay_Ring_Live_গুনতি_Varp->dtypep()->width()};
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* delayRingAtLastIndex(File_সারি* ধ্রুব flp, AstVar* ধ্রুব idxp,
                                             ধ্রুব uint32_t size) {
        প্রত্যাগম নূতন AstEq{flp, নূতন AstVarRef{flp, idxp, VAccess::READ},
                         নূতন AstConst{flp, AstConst::Widthed_মান{}, 32, size - 1}};
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* delay_Ring_Output_গুনতি(File_সারি* ধ্রুব flp, SvaStateVertex* ধ্রুব vtxp) {
        AstVar* ধ্রুব idxp = vtxp->datap()->delayRingIdxVarp;
        ধ্রুব uint32_t size = vtxp->m_delayRingSize;
        Ast_গ্রন্থি_রাশিমালা* outgoingValidp
            = নূতন AstVarRef{flp, vtxp->datap()->delayRingWrappedVarp, VAccess::READ};
        যদি (!vtxp->m_isFixedDelayRing) {
            outgoingValidp
                = নূতন AstLogOr{flp, outgoingValidp, delayRingAtLastIndex(flp, idxp, size)};
        }
        AstVar* ধ্রুব live_গুনতি_Varp = vtxp->datap()->delay_Ring_Live_গুনতি_Varp;
        প্রত্যাগম নূতন AstCond{flp, outgoingValidp, delay_Ring_Head_গুনতি(flp, vtxp),
                           newTypedConstp(flp, live_গুনতি_Varp->dtypep(), 0)};
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* delayRingHasLiveThreadsp(File_সারি* ধ্রুব flp,
                                                 AstVar* ধ্রুব live_গুনতি_Varp) {
        // active = live_count != 0;
        প্রত্যাগম নূতন AstNeq{flp, নূতন AstVarRef{flp, live_গুনতি_Varp, VAccess::READ},
                          newTypedConstp(flp, live_গুনতি_Varp->dtypep(), 0)};
    }

    // Phase 3 output signals
    সংরচনা SignalSet final {
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
    স্থির তার্কিক isAsyncDisable(ধ্রুব Ast_গ্রন্থি_রাশিমালা* ধ্রুব disableExprp) {
        প্রত্যাগম disableExprp && !VN_IS(disableExprp, Const)
               && !disableExprp->exists([](ধ্রুব AstSampled*) { প্রত্যাগম সত্য; });
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
            নূতন AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_বৃক্ষ(অসত্য), bodyp});
        যদি (!clearp) প্রত্যাগম;
        m_modp->addStmtsp(নূতন AstAlways{
            c.flp, VAlwaysKwd::ALWAYS,
            নূতন Ast_Sen_বৃক্ষ{c.flp, নূতন AstSenItem{c.flp, VEdgeType::ET_POSEDGE,
                                                 c.disableExprp->clone_বৃক্ষ_Pure(অসত্য)}},
            clearp});
    }

    // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
    // Phase 2: State register NBA always block. Each clocked-edge target
    // latches the OR of its incoming contributions.
    শূন্য emitStateRegisterNba(LowerCtx& c) {
        ধ্রুব তার্কিক asyncDisable = isAsyncDisable(c.disableExprp);
        Ast_গ্রন্থি* bodyp = nullptr;
        Ast_গ্রন্থি* clearp = nullptr;
        প্রতি (ধ্রুব V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ধ্রুব SvaStateVertex* ধ্রুব vtxp = static_cast<ধ্রুব SvaStateVertex*>(&vtxr);
            যদি (!vtxp->datap()->stateVarp) অনুবর্ত;

            Ast_গ্রন্থি_রাশিমালা* nextStatep = nullptr;
            প্রতি (ধ্রুব V3GraphEdge& edger : vtxp->inEdges()) {
                ধ্রুব SvaTransEdge& tedger = static_cast<ধ্রুব SvaTransEdge&>(edger);
                যদি (!tedger.m_consumesCycle) অনুবর্ত;
                ধ্রুব SvaStateVertex* ধ্রুব fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Clocked-edge source missing stateSig");

                Ast_গ্রন্থি_রাশিমালা* srcSigp = fromVtxp->datap()->stateSigp->clone_বৃক্ষ_Pure(অসত্য);
                srcSigp = andCond(c.flp, srcSigp, tedger.m_condp);

                যদি (c.disableExprp) {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব notDisp
                        = নূতন AstLogNot{c.flp, c.disableExprp->clone_বৃক্ষ_Pure(অসত্য)};
                    srcSigp = নূতন AstLogAnd{c.flp, srcSigp, notDisp};
                }
                nextStatep = orExprs(c.flp, nextStatep, srcSigp);
            }

            UASSERT_OBJ(nextStatep, vtxp,
                        "Registered vertex has no clocked incoming contribution");
            nextStatep = gateNotKill(c, nextStatep);

            AstAssignDly* ধ্রুব assignp = নূতন AstAssignDly{
                c.flp, নূতন AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE}, nextStatep};
            bodyp = Ast_গ্রন্থি::addNextNull(bodyp, assignp);
            যদি (asyncDisable) {
                clearp = Ast_গ্রন্থি::addNextNull(
                    clearp,
                    নূতন AstAssignDly{
                        c.flp, নূতন AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE},
                        নূতন AstConst{c.flp, AstConst::BitFalse{}}});
            }
        }
        যদি (bodyp) addStateAlways(c, bodyp, clearp);
    }

    // Phase 2b: In-place ring storage and registered ring metadata.
    শূন্য emitDelayRingUpdates(LowerCtx& c, ধ্রুব তার্কিক গুনতি_Matches) {
        ধ্রুব তার্কিক asyncDisable = isAsyncDisable(c.disableExprp);
        প্রতি (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ধ্রুব vtxp = static_cast<SvaStateVertex*>(&vtxr);
            যদি (!vtxp->datap()->delayRingVarp) অনুবর্ত;
            AstVar* ধ্রুব idxp = vtxp->datap()->delayRingIdxVarp;
            AstVar* ধ্রুব live_গুনতি_Varp = vtxp->datap()->delay_Ring_Live_গুনতি_Varp;
            AstVar* ধ্রুব wrappedp = vtxp->datap()->delayRingWrappedVarp;
            ধ্রুব uint32_t size = static_cast<uint32_t>(vtxp->m_delayRingSize);

            Ast_গ্রন্থি_রাশিমালা* incomingp = nullptr;
            Ast_গ্রন্থি_রাশিমালা* incomingOverflowp = nullptr;
            প্রতি (ধ্রুব V3GraphEdge& edger : vtxp->inEdges()) {
                ++m_statDelayRingEdgeVisits;
                ধ্রুব SvaTransEdge& tedger = static_cast<ধ্রুব SvaTransEdge&>(edger);
                UASSERT_OBJ(tedger.m_consumesCycle == vtxp->m_isFixedDelayRing, vtxp,
                            "Delay-ring incoming edge kind mismatch");
                ধ্রুব SvaStateVertex* ধ্রুব fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Delay-ring incoming source missing stateSig");
                UASSERT_OBJ(!গুনতি_Matches || fromVtxp->datap()->state_গুনতি_Sigp, fromVtxp,
                            "Delay-ring incoming source missing stateCountSig");
                // IEEE 1800-2023 16.14.1/16.14.3: assertion alternatives belong to one
                // evaluation; only cover sequence counts each end-of-match.
                Ast_গ্রন্থি_রাশিমালা* contribp
                    = গুনতি_Matches
                          ? fromVtxp->datap()->state_গুনতি_Sigp->clone_বৃক্ষ_Pure(অসত্য)
                          : নূতন AstExtend{c.flp,
                                          fromVtxp->datap()->stateSigp->clone_বৃক্ষ_Pure(অসত্য),
                                          m_u32DTypep->width()};
                Ast_গ্রন্থি_রাশিমালা* contribOverflowp
                    = গুনতি_Matches && fromVtxp->datap()->state_গুনতি_Overflow_Sigp
                          ? fromVtxp->datap()->state_গুনতি_Overflow_Sigp->clone_বৃক্ষ_Pure(অসত্য)
                          : nullptr;
                যদি (tedger.m_condp) {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব enablep = tedger.m_condp->clone_বৃক্ষ_Pure(অসত্য);
                    contribp = গুনতি_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, enablep)
                                   : addCountp(c.flp, nullptr, contribp, enablep);
                }
                যদি (c.disableExprp) {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব notDisp
                        = নূতন AstLogNot{c.flp, c.disableExprp->clone_বৃক্ষ_Pure(অসত্য)};
                    contribp = গুনতি_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, notDisp)
                                   : addCountp(c.flp, nullptr, contribp, notDisp);
                }
                incomingp = গুনতি_Matches ? addCheckedCountp(c.flp, incomingp, contribp,
                                                            incomingOverflowp, contribOverflowp)
                                         : addCountp(c.flp, incomingp, contribp);
            }
            UASSERT_OBJ(incomingp, vtxp, "Delay ring has no incoming edge");
            ধ্রুব তার্কিক storesCounts = vtxp->datap()->delayRingCounts;
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব incomingCountp
                = storesCounts
                      ? incomingp->clone_বৃক্ষ_Pure(অসত্য)
                      : নূতন AstExtend{c.flp,
                                      নূতন AstNeq{c.flp, incomingp->clone_বৃক্ষ_Pure(অসত্য),
                                                 newTypedConstp(c.flp, incomingp->dtypep(), 0)},
                                      m_u32DTypep->width()};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব storedIncomingp
                = storesCounts ? incomingp
                               : নূতন AstNeq{c.flp, incomingp,
                                            newTypedConstp(c.flp, incomingp->dtypep(), 0)};
            // Only this block accesses the storage. Other processes read the cached head, so
            // the ring can be updated in place without an NBA shadow copy.
            AstAssign* ধ্রুব writeIncomingp = নূতন AstAssign{
                c.flp,
                delayRingSlot(c.flp, vtxp, নূতন AstVarRef{c.flp, idxp, VAccess::READ},
                              VAccess::WRITE),
                storedIncomingp};
            Ast_গ্রন্থি* updateBodyp = writeIncomingp;
            // live_count <= live_count - outgoing_count + incoming_count;
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব outgoingp = delay_Ring_Output_গুনতি(c.flp, vtxp);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব afterOutgoingp
                = নূতন AstSub{c.flp, নূতন AstVarRef{c.flp, live_গুনতি_Varp, VAccess::READ}, outgoingp};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব oldAfterOutgoingp = afterOutgoingp->clone_বৃক্ষ_Pure(অসত্য);
            AstAdd* ধ্রুব nextLiveCountp = নূতন AstAdd{c.flp, afterOutgoingp, incomingCountp};
            যদি (storesCounts) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব liveOverflowp
                    = নূতন AstLt{c.flp, nextLiveCountp->clone_বৃক্ষ_Pure(অসত্য), oldAfterOutgoingp};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব overflowp = orExprs(c.flp, incomingOverflowp, liveOverflowp);
                updateBodyp = new_গুনতি_Overflow_Warning(c.flp, overflowp);
                updateBodyp->addNext(writeIncomingp);
            } অন্যথা {
                VL_DO_DANGLING(oldAfterOutgoingp->delete_বৃক্ষ(), oldAfterOutgoingp);
            }
            // Cache the outgoing slot for the next index after the write. Reading after
            // writing also handles one-slot rings and two-slot range rings without alias cases.
            Ast_গ্রন্থি_রাশিমালা* nextHeadIdxp
                = nextRingIndex(c.flp, নূতন AstVarRef{c.flp, idxp, VAccess::READ}, size);
            যদি (!vtxp->m_isFixedDelayRing) {
                nextHeadIdxp = nextRingIndex(c.flp, nextHeadIdxp, size);
            }
            updateBodyp->addNext(নূতন AstAssignDly{
                c.flp, নূতন AstVarRef{c.flp, vtxp->datap()->delayRingHeadVarp, VAccess::WRITE},
                delayRingSlot(c.flp, vtxp, nextHeadIdxp)});
            updateBodyp->addNext(নূতন AstAssignDly{
                c.flp, নূতন AstVarRef{c.flp, live_গুনতি_Varp, VAccess::WRITE}, nextLiveCountp});
            // wrapped <= wrapped || idx == size - 1;
            updateBodyp->addNext(
                নূতন AstAssignDly{c.flp, নূতন AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                 নূতন AstLogOr{c.flp, নূতন AstVarRef{c.flp, wrappedp, VAccess::READ},
                                              delayRingAtLastIndex(c.flp, idxp, size)}});
            // idx <= next_idx;
            updateBodyp->addNext(নূতন AstAssignDly{
                c.flp, নূতন AstVarRef{c.flp, idxp, VAccess::WRITE},
                nextRingIndex(c.flp, নূতন AstVarRef{c.flp, idxp, VAccess::READ}, size)});
            যদি (vtxp->m_delayRingAdvanceCondp) {
                updateBodyp = নূতন AstIf{
                    c.flp, sampled(vtxp->m_delayRingAdvanceCondp->clone_বৃক্ষ_Pure(অসত্য)),
                    updateBodyp};
            }

            Ast_গ্রন্থি_রাশিমালা* clearCondp = killActive(c);
            যদি (vtxp->m_delayRingClearCondp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_delayRingClearCondp->clone_বৃক্ষ_Pure(অসত্য)));
            }
            যদি (vtxp->m_abortClearp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_abortClearp->clone_বৃক্ষ_Pure(অসত্য)));
            }
            যদি (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_বৃক্ষ_Pure(অসত্য));
            }
            Ast_গ্রন্থি_রাশিমালা* guardp = nullptr;
            প্রতি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব cp : vtxp->m_throughoutConds) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব sampledp = sampled(cp->clone_বৃক্ষ_Pure(অসত্য));
                guardp = guardp ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নূতন AstLogAnd{c.flp, guardp, sampledp})
                                : sampledp;
            }
            যদি (guardp) clearCondp = orExprs(c.flp, clearCondp, নূতন AstLogNot{c.flp, guardp});
            // Logically clear the ring without touching its wide storage.
            AstAssignDly* ধ্রুব clearCountp
                = নূতন AstAssignDly{c.flp, নূতন AstVarRef{c.flp, live_গুনতি_Varp, VAccess::WRITE},
                                   newTypedConstp(c.flp, live_গুনতি_Varp->dtypep(), 0)};
            clearCountp->addNext(নূতন AstAssignDly{c.flp,
                                                  নূতন AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                                  নূতন AstConst{c.flp, AstConst::BitFalse{}}});
            clearCountp->addNext(নূতন AstAssignDly{c.flp,
                                                  নূতন AstVarRef{c.flp, idxp, VAccess::WRITE},
                                                  newTypedConstp(c.flp, idxp->dtypep(), 0)});
            updateBodyp = নূতন AstIf{c.flp, clearCondp, clearCountp, updateBodyp};

            addStateAlways(c, updateBodyp, asyncDisable ? clearCountp->clone_বৃক্ষ(সত্য) : nullptr);
        }
    }

    // Phase 2c: SAnd combiner done-latch always block.
    // NBA semantics ensure doneL/doneR read pre-update values (IEEE 16.9.5).
    শূন্য emitAndCombinerDoneLatchNba(LowerCtx& c) {
        ধ্রুব তার্কিক asyncDisable = isAsyncDisable(c.disableExprp);
        প্রতি (ধ্রুব V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ধ্রুব SvaStateVertex* ধ্রুব avp = static_cast<ধ্রুব SvaStateVertex*>(&vtxr);
            যদি (!avp->datap()->doneLVarp) অনুবর্ত;
            // doneLVars is non-null only for AndCombiner vertices, which always
            // have both m_andLhsTermp and m_andRhsTermp set at build time.
            UASSERT_OBJ(avp->m_andLhsTermp && avp->m_andRhsTermp, avp,
                        "AndCombiner vertex missing LHS/RHS terminal");
            ধ্রুব SvaStateVertex* ধ্রুব lhsTermp = avp->m_andLhsTermp;
            ধ্রুব SvaStateVertex* ধ্রুব rhsTermp = avp->m_andRhsTermp;
            // resolveLinks' 2*N+2 fixed-point pass is guaranteed to populate
            // stateSigp on every AndCombiner and its LHS/RHS terminals.
            UASSERT_OBJ(lhsTermp->datap()->stateSigp && rhsTermp->datap()->stateSigp
                            && avp->datap()->stateSigp,
                        avp, "AndCombiner stateSigp chain unresolved after resolveLinks");

            AstAssignDly* ধ্রুব clearLp = নূতন AstAssignDly{
                c.flp, নূতন AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                নূতন AstConst{c.flp, AstConst::BitFalse{}}};
            AstAssignDly* ধ্রুব clearRp = নূতন AstAssignDly{
                c.flp, নূতন AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                নূতন AstConst{c.flp, AstConst::BitFalse{}}};
            clearLp->addNext(clearRp);

            Ast_গ্রন্থি_রাশিমালা* ধ্রুব matchLNowp
                = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, avp->m_andLhsCondp);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব matchRNowp
                = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, avp->m_andRhsCondp);
            Ast_গ্রন্থি_রাশিমালা* gateLp = matchLNowp;
            Ast_গ্রন্থি_রাশিমালা* gateRp = matchRNowp;
            যদি (c.disableExprp) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব notDisLp
                    = নূতন AstLogNot{c.flp, c.disableExprp->clone_বৃক্ষ_Pure(অসত্য)};
                gateLp = নূতন AstLogAnd{c.flp, gateLp, notDisLp};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব notDisRp
                    = নূতন AstLogNot{c.flp, c.disableExprp->clone_বৃক্ষ_Pure(অসত্য)};
                gateRp = নূতন AstLogAnd{c.flp, gateRp, notDisRp};
            }
            AstAssignDly* ধ্রুব setLp = নূতন AstAssignDly{
                c.flp, নূতন AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                নূতন AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* ধ্রুব setLIfp = নূতন AstIf{c.flp, gateLp, setLp, nullptr};
            AstAssignDly* ধ্রুব setRp = নূতন AstAssignDly{
                c.flp, নূতন AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                নূতন AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* ধ্রুব setRIfp = নূতন AstIf{c.flp, gateRp, setRp, nullptr};
            setLIfp->addNext(setRIfp);

            Ast_গ্রন্থি_রাশিমালা* clearCondp = নূতন AstLogOr{c.flp, killActive(c),
                                                   avp->datap()->stateSigp->clone_বৃক্ষ_Pure(অসত্য)};
            যদি (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_বৃক্ষ_Pure(অসত্য));
            }
            AstIf* ধ্রুব topp = নূতন AstIf{c.flp, clearCondp, clearLp, setLIfp};
            addStateAlways(c, topp, asyncDisable ? clearLp->clone_বৃক্ষ(সত্য) : nullptr);
        }
    }

    শূন্য emitKillAckNba(LowerCtx& c) {
        AstAssignDly* ধ্রুব ackp
            = নূতন AstAssignDly{c.flp, নূতন AstVarRef{c.flp, c.killVarp, VAccess::WRITE},
                               assertKillGet(c.flp, c.assertType, c.directiveType)};
        m_modp->addStmtsp(
            নূতন AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_বৃক্ষ(অসত্য), ackp});
    }

    // Phase 3/3a/3b: Compute terminal match/reject signals, required-step reject,
    // throughout-drop reject; clean up intermediate state signals.
    // Phase 3: terminalActive and rejectBase from Links to matchVertex.
    // Builder only adds Links (non-clocked) to matchVertex via addLink in
    // wireMatchAndMidSources. For cover sequence, also count each end-of-match
    // so the action can be replayed without unrolling endpoints.
    শূন্য computeTerminalMatchAndReject(LowerCtx& c, SignalSet& sigs, ধ্রুব তার্কিক need_Match_গুনতি) {
        প্রতি (ধ্রুব SvaTransEdge* ধ্রুব tedgep : c.graph.allEdges()) {
            যদি (tedgep->toVtxp() != c.graph.m_matchVertexp) অনুবর্ত;
            SvaStateVertex* ধ্রুব fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                        "Terminal-link source missing stateSig");

            Ast_গ্রন্থি_রাশিমালা* srcSigp = fromVtxp->datap()->stateSigp->clone_বৃক্ষ_Pure(অসত্য);
            srcSigp = andCond(c.flp, srcSigp, tedgep->m_condp);
            যদি (need_Match_গুনতি) {
                UASSERT_OBJ(fromVtxp->datap()->state_গুনতি_Sigp, fromVtxp,
                            "Terminal-link source missing stateCountSig");
                Ast_গ্রন্থি_রাশিমালা* contributionp
                    = fromVtxp->datap()->state_গুনতি_Sigp->clone_বৃক্ষ_Pure(অসত্য);
                Ast_গ্রন্থি_রাশিমালা* contributionOverflowp
                    = fromVtxp->datap()->state_গুনতি_Overflow_Sigp
                          ? fromVtxp->datap()->state_গুনতি_Overflow_Sigp->clone_বৃক্ষ_Pure(অসত্য)
                          : nullptr;
                যদি (tedgep->m_condp) {
                    contributionp = gateCheckedCountp(c.flp, contributionp, contributionOverflowp,
                                                      tedgep->m_condp->clone_বৃক্ষ_Pure(অসত্য));
                }
                sigs.matchCountp
                    = addCheckedCountp(c.flp, sigs.matchCountp, contributionp,
                                       sigs.match_গুনতি_Overflowp, contributionOverflowp);
            }

            যদি (fromVtxp->m_delayRingSize && !fromVtxp->m_isFixedDelayRing) {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_বৃক্ষ_Pure(অসত্য));
                // reject |= (wrapped || idx == size - 1) && head && final_condition;
                Ast_গ্রন্থি_রাশিমালা* expireContribp = delay_Ring_Output_গুনতি(c.flp, fromVtxp);
                expireContribp = andCond(c.flp, expireContribp, tedgep->m_condp);
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, expireContribp);
                VL_DO_DANGLING(srcSigp->delete_বৃক্ষ(), srcSigp);
            } অন্যথা যদি (fromVtxp->m_isUnbounded || fromVtxp->m_isAndCombiner) {
                sigs.terminalActivep = orExprs(c.flp, sigs.terminalActivep, srcSigp);
            } অন্যথা {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_বৃক্ষ_Pure(অসত্য));
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, srcSigp);
            }
        }
        // wireMatchAndMidSources always adds a Link from result.termVertexp
        // to m_matchVertexp, so the loop above always sets terminalActivep.
        UASSERT_OBJ(sigs.terminalActivep, c.graph.m_matchVertexp,
                    "No terminal edge to match vertex");
    }

    Ast_গ্রন্থি_রাশিমালা* newThroughoutThreadFailCountp(LowerCtx& c, AstVar* ধ্রুব delay_Ring_Live_গুনতি_Varp,
                                               Ast_গ্রন্থি_রাশিমালা* ধ্রুব stateExprp,
                                               Ast_গ্রন্থি_রাশিমালা* ধ্রুব enablep) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব activeThreadCountp
            = delay_Ring_Live_গুনতি_Varp
                  ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(
                        নূতন AstVarRef{c.flp, delay_Ring_Live_গুনতি_Varp, VAccess::READ})
                  : নূতন AstExtend{c.flp, stateExprp->clone_বৃক্ষ_Pure(অসত্য), m_u32DTypep->width()};
        প্রত্যাগম addCountp(c.flp, nullptr, activeThreadCountp, enablep);
    }

    // Phase 3b: Throughout-drop and ring-wide abort rejection.
    শূন্য computeThroughoutReject(LowerCtx& c, SignalSet& sigs, ধ্রুব তার্কিক need_Thread_Fail_গুনতি) {
        প্রতি (ধ্রুব V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ধ্রুব SvaStateVertex* ধ্রুব vtxp = static_cast<ধ্রুব SvaStateVertex*>(&vtxr);
            ধ্রুব auto& conds = vtxp->m_throughoutConds;
            যদি (conds.empty() && !vtxp->m_abortRejectp) অনুবর্ত;
            যদি (vtxp->m_isAndCombiner) অনুবর্ত;
            Ast_গ্রন্থি_রাশিমালা* stateExprp = nullptr;
            যদি (vtxp->datap()->stateVarp) {
                stateExprp = নূতন AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
            } অন্যথা যদি (vtxp->datap()->delayRingVarp && vtxp->m_isFixedDelayRing) {
                stateExprp
                    = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_গুনতি_Varp);
            } অন্যথা {
                UASSERT_OBJ(vtxp->datap()->stateSigp, vtxp,
                            "Throughout-conds vertex missing state representation");
                stateExprp = vtxp->datap()->stateSigp->clone_বৃক্ষ_Pure(অসত্য);
            }
            Ast_গ্রন্থি_রাশিমালা* guardp = nullptr;
            প্রতি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব cp : conds) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব sp = sampled(cp->clone_বৃক্ষ_Pure(অসত্য));
                guardp = guardp ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নূতন AstLogAnd{c.flp, guardp, sp}) : sp;
            }
            যদি (vtxp->m_abortRejectp) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব notAbortp
                    = নূতন AstLogNot{c.flp, sampled(vtxp->m_abortRejectp->clone_বৃক্ষ_Pure(অসত্য))};
                guardp = guardp
                             ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নূতন AstLogAnd{c.flp, guardp, notAbortp})
                             : notAbortp;
            }
            Ast_গ্রন্থি_রাশিমালা* rejectCondp = নূতন AstLogNot{c.flp, guardp};
            যদি (vtxp->m_abortClearp) {
                // Any abort clears the ring. Accept aborts suppress a simultaneous guard failure;
                // reject aborts are restored below so they still force rejection.
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব notAbortClearp
                    = নূতন AstLogNot{c.flp, sampled(vtxp->m_abortClearp->clone_বৃক্ষ_Pure(অসত্য))};
                rejectCondp = নূতন AstLogAnd{c.flp, rejectCondp, notAbortClearp};
                যদি (vtxp->m_abortRejectp) {
                    rejectCondp = নূতন AstLogOr{
                        c.flp, rejectCondp, sampled(vtxp->m_abortRejectp->clone_বৃক্ষ_Pure(অসত্য))};
                }
            }
            যদি (need_Thread_Fail_গুনতি) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব contributionp
                    = newThroughoutThreadFailCountp(c, vtxp->datap()->delay_Ring_Live_গুনতি_Varp,
                                                    stateExprp, rejectCondp->clone_বৃক্ষ_Pure(অসত্য));
                sigs.threadFailCountp = addCountp(c.flp, sigs.threadFailCountp, contributionp);
                যদি (vtxp->m_abortRejectp && vtxp->m_delayRingAdvanceCondp) {
                    // An advancing ring thread also occupies its same-tick match vertex in the
                    // unrolled NFA, so abort rejection must replay both fail actions.
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব abortAndAdvancep = নূতন AstLogAnd{
                        c.flp, sampled(vtxp->m_abortRejectp->clone_বৃক্ষ_Pure(অসত্য)),
                        sampled(vtxp->m_delayRingAdvanceCondp->clone_বৃক্ষ_Pure(অসত্য))};
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব matchContributionp = newThroughoutThreadFailCountp(
                        c, vtxp->datap()->delay_Ring_Live_গুনতি_Varp, stateExprp, abortAndAdvancep);
                    sigs.threadFailCountp
                        = addCountp(c.flp, sigs.threadFailCountp, matchContributionp);
                }
            }
            sigs.throughoutRejectp = orExprs(c.flp, sigs.throughoutRejectp,
                                             নূতন AstLogAnd{c.flp, stateExprp, rejectCondp});
        }
    }

    SignalSet computeSignals(LowerCtx& c, ধ্রুব তার্কিক need_Thread_Fail_গুনতি,
                             ধ্রুব তার্কিক need_Throughout_Thread_Fail_গুনতি, ধ্রুব তার্কিক need_Match_গুনতি) {
        SignalSet sigs;

        computeTerminalMatchAndReject(c, sigs, need_Match_গুনতি);

        // Phase 3a: required-step rejection.
        // Builder only sets m_rejectOnFail on non-clocked Links with m_condp
        // or m_condVtxp, and the source always has a resolved stateSig.
        প্রতি (ধ্রুব SvaTransEdge* ধ্রুব tedgep : c.graph.allEdges()) {
            যদি (!tedgep->m_rejectOnFail) অনুবর্ত;
            ধ্রুব SvaStateVertex* ধ্রুব fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp && (tedgep->m_condp || tedgep->m_condVtxp),
                        fromVtxp,
                        "rejectOnFail Link must have condp/condVtxp and source stateSig");
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব srcSigp = fromVtxp->datap()->stateSigp->clone_বৃক্ষ_Pure(অসত্য);
            Ast_গ্রন্থি_রাশিমালা* condp = nullptr;
            যদি (tedgep->m_condVtxp) {
                UASSERT_OBJ(tedgep->m_condVtxp->datap()->stateSigp, tedgep->m_condVtxp,
                            "rejectOnFail condVtxp missing stateSig");
                condp = tedgep->m_condVtxp->datap()->stateSigp->clone_বৃক্ষ_Pure(অসত্য);
                যদি (tedgep->m_condp) {
                    condp = নূতন AstLogOr{c.flp, condp, tedgep->m_condp->clone_বৃক্ষ_Pure(অসত্য)};
                }
            } অন্যথা {
                condp = tedgep->m_condp->clone_বৃক্ষ_Pure(অসত্য);
            }
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব notCondp = নূতন AstLogNot{c.flp, condp};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব rawFailp = নূতন AstLogAnd{c.flp, srcSigp, notCondp};
            যদি (need_Thread_Fail_গুনতি) {
                // thread_fail_count += fail;
                sigs.threadFailCountp = addCountp(
                    c.flp, sigs.threadFailCountp,
                    নূতন AstExtend{c.flp, rawFailp->clone_বৃক্ষ_Pure(অসত্য), m_u32DTypep->width()});
            }
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব failp = gateNotKill(c, rawFailp);
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
                                        sampled(c.matchCondp->clone_বৃক্ষ_Pure(অসত্য)));
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
        প্রতি (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ধ্রুব vtxp = static_cast<SvaStateVertex*>(&vtxr);
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
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব notTermp
                = নূতন AstLogNot{c.flp, c.disableExprp->clone_বৃক্ষ_Pure(অসত্য)};
            sigs.terminalActivep = নূতন AstLogAnd{c.flp, sigs.terminalActivep, notTermp};
            যদি (sigs.rejectBasep) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব notDisp
                    = নূতন AstLogNot{c.flp, c.disableExprp->clone_বৃক্ষ_Pure(অসত্য)};
                sigs.rejectBasep = নূতন AstLogAnd{c.flp, sigs.rejectBasep, notDisp};
            }
            যদি (sigs.throughoutRejectp) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব notDisp
                    = নূতন AstLogNot{c.flp, c.disableExprp->clone_বৃক্ষ_Pure(অসত্য)};
                sigs.throughoutRejectp = নূতন AstLogAnd{c.flp, sigs.throughoutRejectp, notDisp};
            }
            যদি (sigs.requiredStepRejectp) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব notDisp
                    = নূতন AstLogNot{c.flp, c.disableExprp->clone_বৃক্ষ_Pure(অসত্য)};
                sigs.requiredStepRejectp = নূতন AstLogAnd{c.flp, sigs.requiredStepRejectp, notDisp};
            }
            যদি (sigs.matchCountp) {
                sigs.matchCountp = gateCheckedCountp(
                    c.flp, sigs.matchCountp, sigs.match_গুনতি_Overflowp,
                    নূতন AstLogNot{c.flp, c.disableExprp->clone_বৃক্ষ_Pure(অসত্য)});
            }
        }

        যদি (sigs.match_গুনতি_Overflowp) {
            m_modp->addStmtsp(
                নূতন AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_বৃক্ষ(অসত্য),
                              new_গুনতি_Overflow_Warning(c.flp, sigs.match_গুনতি_Overflowp)});
            sigs.match_গুনতি_Overflowp = nullptr;
        }

        প্রত্যাগম sigs;
    }

    // Phase 1: Resolve combinational Links via fixed-point propagation.
    শূন্য resolveLinks(LowerCtx& c, Ast_গ্রন্থি_রাশিমালা* triggerExprp, ধ্রুব তার্কিক গুনতি_Matches) {
        // datap() was freshly allocated in lower() -- all state signals start null.
        c.graph.m_startVertexp->datap()->stateSigp = triggerExprp->clone_বৃক্ষ_Pure(অসত্য);
        c.graph.m_startVertexp->datap()->state_গুনতি_Sigp
            = নূতন AstExtend{c.flp, triggerExprp->clone_বৃক্ষ_Pure(অসত্য), m_u32DTypep->width()};
        প্রতি (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ধ্রুব vtxp = static_cast<SvaStateVertex*>(&vtxr);
            যদি (vtxp->datap()->stateVarp) {
                vtxp->datap()->stateSigp
                    = নূতন AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
                vtxp->datap()->state_গুনতি_Sigp = নূতন AstExtend{
                    c.flp, নূতন AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ},
                    m_u32DTypep->width()};
            } অন্যথা যদি (vtxp->datap()->delayRingVarp) {
                যদি (vtxp->m_isFixedDelayRing) {
                    // state_count = wrapped ? head : 0; state = state_count != 0;
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব countp = delay_Ring_Output_গুনতি(c.flp, vtxp);
                    vtxp->datap()->stateSigp = নূতন AstNeq{c.flp, countp->clone_বৃক্ষ_Pure(অসত্য),
                                                          newTypedConstp(c.flp, m_u32DTypep, 0)};
                    vtxp->datap()->state_গুনতি_Sigp = countp;
                } অন্যথা {
                    vtxp->datap()->stateSigp
                        = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_গুনতি_Varp);
                    vtxp->datap()->state_গুনতি_Sigp = নূতন AstVarRef{
                        c.flp, vtxp->datap()->delay_Ring_Live_গুনতি_Varp, VAccess::READ};
                }
            }
        }
        // Fixed-point propagation along zero-delay (Link) edges. Rebuild each
        // derived signal from its incoming edges on every pass; appending the same
        // contributions repeatedly makes merge expressions grow exponentially.
        ধ্রুব size_t passes = 2 * c.graph.m_graph.vertices().size() + 2;
        প্রতি (size_t pass = 0; pass < passes; ++pass) {
            // Rebuild SAnd combiners once both sub-NFA terminals are available.
            প্রতি (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* ধ্রুব vtxp = static_cast<SvaStateVertex*>(&vtxr);
                যদি (!vtxp->m_isAndCombiner) অনুবর্ত;
                // AndCombiner vertices always have both terminal pointers set.
                UASSERT_OBJ(vtxp->m_andLhsTermp && vtxp->m_andRhsTermp, vtxp,
                            "AndCombiner vertex missing LHS/RHS terminal");
                ধ্রুব SvaStateVertex* ধ্রুব lhsTermp = vtxp->m_andLhsTermp;
                ধ্রুব SvaStateVertex* ধ্রুব rhsTermp = vtxp->m_andRhsTermp;
                যদি (!lhsTermp->datap()->stateSigp || !rhsTermp->datap()->stateSigp) অনুবর্ত;
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব matchLp
                    = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, vtxp->m_andLhsCondp);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব matchRp
                    = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, vtxp->m_andRhsCondp);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব doneLOrp = নূতন AstLogOr{
                    c.flp, নূতন AstVarRef{c.flp, vtxp->datap()->doneLVarp, VAccess::READ}, matchLp};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব doneROrp = নূতন AstLogOr{
                    c.flp, নূতন AstVarRef{c.flp, vtxp->datap()->doneRVarp, VAccess::READ}, matchRp};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব bothp = নূতন AstLogAnd{c.flp, doneLOrp, doneROrp};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব oneNowp = নূতন AstLogOr{c.flp, matchLp->clone_বৃক্ষ_Pure(অসত্য),
                                                          matchRp->clone_বৃক্ষ_Pure(অসত্য)};
                যদি (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_বৃক্ষ(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = নূতন AstLogAnd{c.flp, bothp, oneNowp};
                যদি (vtxp->datap()->state_গুনতি_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_গুনতি_Sigp->delete_বৃক্ষ(),
                                   vtxp->datap()->state_গুনতি_Sigp);
                }
                vtxp->datap()->state_গুনতি_Sigp = নূতন AstExtend{
                    c.flp, vtxp->datap()->stateSigp->clone_বৃক্ষ_Pure(অসত্য), m_u32DTypep->width()};
            }

            প্রতি (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* ধ্রুব vtxp = static_cast<SvaStateVertex*>(&vtxr);
                যদি (vtxp == c.graph.m_startVertexp || vtxp->datap()->stateVarp
                    || vtxp->datap()->delayRingVarp || vtxp->m_isAndCombiner || vtxp->m_isMatch
                    || vtxp->m_isRejectSink) {
                    অনুবর্ত;
                }
                Ast_গ্রন্থি_রাশিমালা* nextStatep = nullptr;
                Ast_গ্রন্থি_রাশিমালা* nextStateCountp = nullptr;
                Ast_গ্রন্থি_রাশিমালা* next_State_গুনতি_Overflowp = nullptr;
                প্রতি (ধ্রুব V3GraphEdge& er : vtxp->inEdges()) {
                    ধ্রুব SvaTransEdge& te = static_cast<ধ্রুব SvaTransEdge&>(er);
                    ধ্রুব SvaStateVertex* ধ্রুব fromVtxp = te.fromVtxp();
                    যদি (!fromVtxp->datap()->stateSigp) অনুবর্ত;
                    UASSERT_OBJ(fromVtxp->datap()->state_গুনতি_Sigp, fromVtxp,
                                "Link source missing stateCountSig");
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব contributionp = andCond(
                        c.flp, fromVtxp->datap()->stateSigp->clone_বৃক্ষ_Pure(অসত্য), te.m_condp);
                    nextStatep = orExprs(c.flp, nextStatep, contributionp);
                    Ast_গ্রন্থি_রাশিমালা* গুনতি_Contributionp
                        = fromVtxp->datap()->state_গুনতি_Sigp->clone_বৃক্ষ_Pure(অসত্য);
                    Ast_গ্রন্থি_রাশিমালা* গুনতি_Contribution_Overflowp
                        = গুনতি_Matches && fromVtxp->datap()->state_গুনতি_Overflow_Sigp
                              ? fromVtxp->datap()->state_গুনতি_Overflow_Sigp->clone_বৃক্ষ_Pure(অসত্য)
                              : nullptr;
                    যদি (te.m_condp) {
                        Ast_গ্রন্থি_রাশিমালা* ধ্রুব enablep = te.m_condp->clone_বৃক্ষ_Pure(অসত্য);
                        গুনতি_Contributionp
                            = গুনতি_Matches
                                  ? gateCheckedCountp(c.flp, গুনতি_Contributionp,
                                                      গুনতি_Contribution_Overflowp, enablep)
                                  : addCountp(c.flp, nullptr, গুনতি_Contributionp, enablep);
                    }
                    nextStateCountp
                        = গুনতি_Matches
                              ? addCheckedCountp(c.flp, nextStateCountp, গুনতি_Contributionp,
                                                 next_State_গুনতি_Overflowp,
                                                 গুনতি_Contribution_Overflowp)
                              : addCountp(c.flp, nextStateCountp, গুনতি_Contributionp);
                }
                যদি (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_বৃক্ষ(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = nextStatep;
                যদি (vtxp->datap()->state_গুনতি_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_গুনতি_Sigp->delete_বৃক্ষ(),
                                   vtxp->datap()->state_গুনতি_Sigp);
                }
                vtxp->datap()->state_গুনতি_Sigp = nextStateCountp;
                যদি (vtxp->datap()->state_গুনতি_Overflow_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_গুনতি_Overflow_Sigp->delete_বৃক্ষ(),
                                   vtxp->datap()->state_গুনতি_Overflow_Sigp);
                }
                vtxp->datap()->state_গুনতি_Overflow_Sigp = next_State_গুনতি_Overflowp;
            }
        }
    }

সার্বজনিক:
    // Combine terminal/reject signals into final output expression.
    স্থির Ast_গ্রন্থি_রাশিমালা* assembleResult(Ast_গ্রন্থি_Cover_Or_Assert* ধ্রুব assertp, ধ্রুব তার্কিক negated,
                                       Ast_গ্রন্থি_রাশিমালা* ধ্রুব matchCondp, ধ্রুব SignalSet& sigs,
                                       Ast_গ্রন্থি_রাশিমালা** ধ্রুব outMatchpp) {
        File_সারি* ধ্রুব flp = assertp->fileline();
        ধ্রুব তার্কিক isCover = VN_IS(assertp, Cover);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব terminalActivep = sigs.terminalActivep;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব rejectBasep = sigs.rejectBasep;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব throughoutRejectp = sigs.throughoutRejectp;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব requiredStepRejectp = sigs.requiredStepRejectp;
        // Property negation (IEEE 1800-2023 16.12.1 `not`): invert match/reject.
        যদি (negated) {
            যদি (isCover) {
                যদি (terminalActivep)
                    VL_DO_DANGLING(terminalActivep->delete_বৃক্ষ(), terminalActivep);
                Ast_গ্রন্থি_রাশিমালা* negRejectp = nullptr;
                যদি (matchCondp && rejectBasep) {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব sampledCondp = sampled(matchCondp->clone_বৃক্ষ_Pure(অসত্য));
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব notCondp = নূতন AstLogNot{flp, sampledCondp};
                    negRejectp = নূতন AstLogAnd{flp, rejectBasep, notCondp};
                } অন্যথা যদি (rejectBasep) {
                    VL_DO_DANGLING(rejectBasep->delete_বৃক্ষ(), rejectBasep);
                }
                যদি (throughoutRejectp) negRejectp = orExprs(flp, negRejectp, throughoutRejectp);
                যদি (requiredStepRejectp)
                    negRejectp = orExprs(flp, negRejectp, requiredStepRejectp);
                প্রত্যাগম negRejectp ? negRejectp : নূতন AstConst{flp, AstConst::BitFalse{}};
            }
            // Negated assert/assume: output = !match.
            Ast_গ্রন্থি_রাশিমালা* matchp = terminalActivep;
            যদি (matchCondp) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব sampledCondp = sampled(matchCondp->clone_বৃক্ষ_Pure(অসত্য));
                matchp = নূতন AstLogAnd{flp, matchp, sampledCondp};
            }
            যদি (outMatchpp) {
                Ast_গ্রন্থি_রাশিমালা* notPMatchp = nullptr;
                যদি (matchCondp && rejectBasep) {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব sampledCondp = sampled(matchCondp->clone_বৃক্ষ_Pure(অসত্য));
                    notPMatchp = নূতন AstLogAnd{flp, rejectBasep->clone_বৃক্ষ_Pure(অসত্য),
                                               নূতন AstLogNot{flp, sampledCondp}};
                } অন্যথা যদি (rejectBasep) {
                    notPMatchp = rejectBasep->clone_বৃক্ষ_Pure(অসত্য);
                }
                যদি (throughoutRejectp)
                    notPMatchp = orExprs(flp, notPMatchp, throughoutRejectp->clone_বৃক্ষ_Pure(অসত্য));
                যদি (requiredStepRejectp)
                    notPMatchp
                        = orExprs(flp, notPMatchp, requiredStepRejectp->clone_বৃক্ষ_Pure(অসত্য));
                *outMatchpp = notPMatchp;
            }
            যদি (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_বৃক্ষ(), throughoutRejectp);
            যদি (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_বৃক্ষ(), rejectBasep);
            যদি (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_বৃক্ষ(), requiredStepRejectp);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব resultExprp = নূতন AstLogNot{flp, matchp};
            প্রত্যাগম resultExprp;
        }
        যদি (isCover) {
            যদি (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_বৃক্ষ(), throughoutRejectp);
            যদি (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_বৃক্ষ(), rejectBasep);
            যদি (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_বৃক্ষ(), requiredStepRejectp);
            যদি (matchCondp) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব sampledCondp = sampled(matchCondp->clone_বৃক্ষ_Pure(অসত্য));
                প্রত্যাগম নূতন AstLogAnd{flp, terminalActivep, sampledCondp};
            }
            প্রত্যাগম terminalActivep;
        }
        // Assert/assume: output = !reject
        Ast_গ্রন্থি_রাশিমালা* rejectp = nullptr;
        যদি (matchCondp && rejectBasep) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব sampledCondp = sampled(matchCondp->clone_বৃক্ষ_Pure(অসত্য));
            rejectp = নূতন AstLogAnd{flp, rejectBasep, নূতন AstLogNot{flp, sampledCondp}};
        } অন্যথা যদি (rejectBasep) {
            VL_DO_DANGLING(rejectBasep->delete_বৃক্ষ(), rejectBasep);
        }
        যদি (outMatchpp) {
            Ast_গ্রন্থি_রাশিমালা* matchExprp = terminalActivep->clone_বৃক্ষ_Pure(অসত্য);
            যদি (matchCondp) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব sp = sampled(matchCondp->clone_বৃক্ষ_Pure(অসত্য));
                matchExprp = নূতন AstLogAnd{flp, matchExprp, sp};
            }
            *outMatchpp = matchExprp;
        }
        যদি (terminalActivep) VL_DO_DANGLING(terminalActivep->delete_বৃক্ষ(), terminalActivep);
        যদি (throughoutRejectp) rejectp = orExprs(flp, rejectp, throughoutRejectp);
        যদি (requiredStepRejectp) rejectp = orExprs(flp, rejectp, requiredStepRejectp);
        যদি (!rejectp) প্রত্যাগম নূতন AstConst{flp, AstConst::BitTrue{}};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব resultExprp = নূতন AstLogNot{flp, rejectp};
        প্রত্যাগম resultExprp;
    }

    explicit SvaNfaLowering(Ast_গ্রন্থি_Module* modp)
        : m_modp{modp}
        , m_u32DTypep{modp->findBasicDType(VBasicDTypeKwd::UINT32)} {}
    ~SvaNfaLowering() {
        V3Stats::addStatSum("Assertions, NFA delay ring edge visits", m_statDelayRingEdgeVisits);
    }

    // Lower NFA graph to synthesizable AstAlways blocks and raw result signals.
    // Links are combinational; Edges are registered (NBA).
    SignalSet lower(Ast_গ্রন্থি_Cover_Or_Assert* ধ্রুব assertp, SvaGraph& graph,
                    Ast_Sen_বৃক্ষ* ধ্রুব senTreep, Ast_গ্রন্থি_রাশিমালা* ধ্রুব matchCondp,
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব disableExprp, ধ্রুব তার্কিক need_Thread_Fail_গুনতি,
                    ধ্রুব তার্কিক need_Throughout_Thread_Fail_গুনতি) {
        File_সারি* ধ্রুব flp = assertp->fileline();
        AstCover* ধ্রুব coverp = VN_CAST(assertp, Cover);
        ধ্রুব তার্কিক isSeqEvent = coverp && coverp->isSeqEvent();
        ধ্রুব VAssertType assertType
            = isSeqEvent ? VAssertType{VAssertType::INTERNAL} : assertp->userType();
        ধ্রুব VAssertDirectiveType directiveType
            = isSeqEvent ? VAssertDirectiveType{VAssertDirectiveType::INTERNAL}
                         : assertp->directive();
        ধ্রুব std::string baseName = m_names.get("");

        // Number vertices with sequential colors for array indexing.
        পূর্ণাঙ্ক N = 0;
        প্রতি (V3GraphVertex& vtxr : graph.m_graph.vertices()) { vtxr.color(N++); }
        std::vector<SvaStateVertex*> vtx(N, nullptr);
        প্রতি (V3GraphVertex& vtxr : graph.m_graph.vertices()) {
            vtx[vtxr.color()] = static_cast<SvaStateVertex*>(&vtxr);
        }

        // Allocate per-vertex lowering data (stored via V3GraphVertex::userp()).
        std::vector<SvaVertexData> vertexData(N);
        প্রতি (পূর্ণাঙ্ক i = 0; i < N; ++i) vtx[i]->userp(&vertexData[i]);

        // Identify registered vertices (targets of clocked edges).
        প্রতি (পূর্ণাঙ্ক i = 0; i < N; ++i) {
            প্রতি (ধ্রুব V3GraphEdge& edger : vtx[i]->outEdges()) {
                ধ্রুব SvaTransEdge& tedger = static_cast<ধ্রুব SvaTransEdge&>(edger);
                ধ্রুব পূর্ণাঙ্ক toIdx = tedger.toVtxp()->color();
                যদি (tedger.m_consumesCycle && tedger.toVtxp() != graph.m_matchVertexp
                    && !tedger.toVtxp()->m_isRejectSink) {
                    vtx[toIdx]->datap()->needsReg = সত্য;
                }
            }
        }

        // A packed range ring can emit several live endpoints without needing per-slot counts.
        // Widen only a ring whose input can already carry multiplicity.
        std::vector<তার্কিক> stateMayHaveMultiplicity(N);
        std::vector<তার্কিক> ringNeedsMultiplicity(N);
        std::vector<পূর্ণাঙ্ক> multiplicityWork;
        ধ্রুব তার্কিক গুনতি_Matches = coverp && coverp->isCoverSeq() && !isSeqEvent;
        যদি (গুনতি_Matches) {
            প্রতি (পূর্ণাঙ্ক i = 0; i < N; ++i) {
                ধ্রুব তার্কিক multipleInputs = vtx[i]->inEdges().size() > 1;
                ধ্রুব তার্কিক isRing = vtx[i]->m_delayRingSize;
                ringNeedsMultiplicity[i] = isRing && multipleInputs;
                ধ্রুব তার্কিক startsMultiplicity = isRing
                                                    ? !vtx[i]->m_isFixedDelayRing
                                                    : multipleInputs && !vtx[i]->datap()->needsReg;
                যদি (startsMultiplicity) {
                    stateMayHaveMultiplicity[i] = সত্য;
                    multiplicityWork.ঠেলো_back(i);
                }
            }
            প্রতি (size_t wi = 0; wi < multiplicityWork.size(); ++wi) {
                প্রতি (ধ্রুব V3GraphEdge& edger : vtx[multiplicityWork[wi]]->outEdges()) {
                    ধ্রুব SvaTransEdge& tedger = static_cast<ধ্রুব SvaTransEdge&>(edger);
                    ধ্রুব পূর্ণাঙ্ক ti = tedger.toVtxp()->color();
                    ধ্রুব তার্কিক isRing = vtx[ti]->m_delayRingSize;
                    যদি (isRing) ringNeedsMultiplicity[ti] = সত্য;
                    // Match and reject sinks have no outgoing edges, so they stop naturally.
                    ধ্রুব তার্কিক stopsMultiplicity = !isRing && vtx[ti]->datap()->needsReg;
                    যদি (stateMayHaveMultiplicity[ti] || stopsMultiplicity) { অনুবর্ত; }
                    stateMayHaveMultiplicity[ti] = সত্য;
                    multiplicityWork.ঠেলো_back(ti);
                }
            }
        }

        ধ্রুব তার্কিক asyncDisable = isAsyncDisable(disableExprp);
        // Registers cleared on a rising disable are also written by their clocked block.
        File_সারি* ধ্রুব regFlp = asyncDisable ? নূতন File_সারি{flp} : flp;
        যদি (asyncDisable) regFlp->warnOff(V3ErrorCode::MULTIDRIVEN, সত্য);

        AstVar* ধ্রুব killVarp
            = নূতন AstVar{flp, VVarType::MODULETEMP, baseName + "__kill", m_u32DTypep};
        killVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(killVarp);
        প্রতি (পূর্ণাঙ্ক i = 0; i < N; ++i) {
            যদি (vtx[i]->m_isAndCombiner) {
                ধ্রুব std::string base = baseName + "__a" + std::to_string(i);
                AstVar* ধ্রুব lp = নূতন AstVar{regFlp, VVarType::MODULETEMP, base + "_doneL",
                                              m_modp->findBitDType()};
                lp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(lp);
                vtx[i]->datap()->doneLVarp = lp;
                AstVar* ধ্রুব rp = নূতন AstVar{regFlp, VVarType::MODULETEMP, base + "_doneR",
                                              m_modp->findBitDType()};
                rp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(rp);
                vtx[i]->datap()->doneRVarp = rp;
                অনুবর্ত;
            }
            যদি (vtx[i]->m_delayRingSize) {
                ধ্রুব std::string base = baseName + "__d" + std::to_string(i);
                ধ্রুব তার্কিক storesCounts = গুনতি_Matches && ringNeedsMultiplicity[i];
                Ast_গ্রন্থি_DType* ringDTypep = nullptr;
                যদি (storesCounts) {
                    // int unsigned ring [size];
                    ringDTypep = নূতন AstUnpackArrayDType{
                        flp, m_u32DTypep,
                        নূতন AstRange{flp, static_cast<পূর্ণাঙ্ক>(vtx[i]->m_delayRingSize - 1), 0}};
                    v3Global.rootp()->typeTablep()->addTypesp(ringDTypep);
                } অন্যথা {
                    // bit [size-1:0] ring;
                    ringDTypep = m_modp->findBitDType(vtx[i]->m_delayRingSize,
                                                      vtx[i]->m_delayRingSize, VSigning::UNSIGNED);
                }
                AstVar* ধ্রুব ringp
                    = নূতন AstVar{flp, VVarType::MODULETEMP, base + "_ring", ringDTypep};
                ringp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(ringp);
                vtx[i]->datap()->delayRingVarp = ringp;
                vtx[i]->datap()->delayRingCounts = storesCounts;
                AstVar* ধ্রুব headp
                    = নূতন AstVar{flp, VVarType::MODULETEMP, base + "_head",
                                 storesCounts ? m_u32DTypep : m_modp->findBitDType()};
                headp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(headp);
                vtx[i]->datap()->delayRingHeadVarp = headp;
                // int unsigned idx;
                AstVar* ধ্রুব idxp
                    = নূতন AstVar{regFlp, VVarType::MODULETEMP, base + "_idx", m_u32DTypep};
                idxp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(idxp);
                vtx[i]->datap()->delayRingIdxVarp = idxp;
                // int unsigned live_count;
                AstVar* ধ্রুব live_গুনতি_Varp
                    = নূতন AstVar{regFlp, VVarType::MODULETEMP, base + "_liveCount", m_u32DTypep};
                live_গুনতি_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(live_গুনতি_Varp);
                vtx[i]->datap()->delay_Ring_Live_গুনতি_Varp = live_গুনতি_Varp;
                AstVar* ধ্রুব wrappedp = নূতন AstVar{regFlp, VVarType::MODULETEMP,
                                                    base + "_wrapped", m_modp->findBitDType()};
                wrappedp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(wrappedp);
                vtx[i]->datap()->delayRingWrappedVarp = wrappedp;
                অনুবর্ত;
            }
            যদি (!vtx[i]->datap()->needsReg) অনুবর্ত;
            যদি (vtx[i] == graph.m_startVertexp || vtx[i]->m_isMatch) অনুবর্ত;
            ধ্রুব std::string varName = baseName + "__s" + std::to_string(i);
            AstVar* ধ্রুব varp
                = নূতন AstVar{regFlp, VVarType::MODULETEMP, varName, m_modp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(varp);
            vtx[i]->datap()->stateVarp = varp;
        }

        // Build lowering context for phase sub-functions.
        LowerCtx c{flp,        senTreep,      disableExprp, matchCondp,
                   assertType, directiveType, killVarp,     graph};

        // Phase 1: Resolve combinational Links via fixed-point propagation.
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব triggerExprp
            = isSeqEvent ? নূতন AstConst{flp, AstConst::BitTrue{}}
                         : assertOnCond(flp, assertp->userType(), assertp->directive());
        resolveLinks(c, triggerExprp, গুনতি_Matches);
        VL_DO_DANGLING(triggerExprp->delete_বৃক্ষ(), triggerExprp);

        // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
        emitStateRegisterNba(c);
        emitDelayRingUpdates(c, গুনতি_Matches);
        emitAndCombinerDoneLatchNba(c);
        emitKillAckNba(c);

        // Phase 3/3a/3b: Compute terminal match/reject signals (cleans up stateSig).
        ধ্রুব SignalSet sigs
            = computeSignals(c, need_Thread_Fail_গুনতি, need_Throughout_Thread_Fail_গুনতি, গুনতি_Matches);

        // Strong s_always[m:n] end-of-simulation liveness: if any in-window state
        // is still set at $finish, the universal-quantifier window never completed
        // (IEEE 1800-2023 16.12.11 strong semantics). Fire the assertion failure
        // from a final block; V3Assert turns the DT_ERROR display into the standard
        // "Assertion failed in %m" message.
        // pending = |{strong state registers, strong delay live counts != 0};
        Ast_গ্রন্থি_রাশিমালা* pendingp = nullptr;
        প্রতি (পূর্ণাঙ্ক i = 0; i < N; ++i) {
            যদি (!vtx[i]->m_strongPending) অনুবর্ত;
            Ast_গ্রন্থি_রাশিমালা* pendingExprp = nullptr;
            যদি (vtx[i]->datap()->stateVarp) {
                pendingExprp = নূতন AstVarRef{flp, vtx[i]->datap()->stateVarp, VAccess::READ};
            } অন্যথা যদি (vtx[i]->m_strongAlwaysRing) {
                pendingExprp
                    = delayRingHasLiveThreadsp(flp, vtx[i]->datap()->delay_Ring_Live_গুনতি_Varp);
            } অন্যথা {
                অনুবর্ত;
            }
            যদি (!pendingp) {
                pendingp = pendingExprp;
            } অন্যথা {
                pendingp = নূতন AstLogOr{flp, pendingp, pendingExprp};
            }
        }
        যদি (pendingp) {
            AstCExpr* ধ্রুব assertOnp
                = নূতন AstCExpr{flp, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()", 1};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp = নূতন AstLogAnd{flp, assertOnp, pendingp};
            AstDisplay* ধ্রুব dispp
                = নূতন AstDisplay{flp, VDisplayType::DT_ERROR, "", nullptr, nullptr};
            dispp->fmtp()->timeunit(m_modp->timeunit());
            Ast_গ্রন্থি_Stmt* ধ্রুব firep = dispp;
            যদি (v3Global.opt.stopFail()) firep->addNext(নূতন AstStop{flp, অসত্য});
            m_modp->addStmtsp(নূতন AstFinal{flp, নূতন AstIf{flp, condp, firep}});
        }

        // Clear userp on every vertex before vertexData is destroyed.
        প্রতি (পূর্ণাঙ্ক i = 0; i < N; ++i) vtx[i]->userp(nullptr);
        প্রত্যাগম sigs;
    }
};

}  // namespace

//######################################################################
// Top-level visitor

বর্গ AssertNfaVisitor final : সার্বজনিক VNVisitor {
    // STATE
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module being processed
    AstClocking* m_defaultClockingp = nullptr;  // Default clocking
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff
    SvaNfaLowering* m_loweringp = nullptr;  // NFA-to-hardware lowering engine
    Ast_Sen_বৃক্ষ* m_sampled_মান_Clockp = nullptr;  // Inherited clock during scoped attachment
    V3UniqueNames m_propVarNames{"__Vpropvar"};  // Property-local variable names
    V3UniqueNames m_propTempNames{"__VnfaSampled"};  // Hoisted $sampled(propp) temps
    V3UniqueNames m_fail_গুনতি_Names{"__VnfaRemainingFailCount"};  // Fail replay counter names
    std::set<ধ্রুব AstProperty*> m_inliningProps;  // Recursion guard for inlineNamedProperty

    template <typename T_গ্রন্থি>
    শূন্য visit_Sampled_মান(T_গ্রন্থি* ধ্রুব nodep) {
        যদি (m_sampled_মান_Clockp && !nodep->sentreep()) {
            nodep->sentreep(m_sampled_মান_Clockp->clone_বৃক্ষ(সত্য));
        }
        iterateChildren(nodep);
    }

    // Wire match vertex and mid-window sources for a successful NFA build.
    স্থির শূন্য wireMatchAndMidSources(SvaGraph& graph, ধ্রুব BuildResult& result, File_সারি* flp) {
        graph.createMatchVertex();
        // Skip the main term Link when midSources already cover every
        // end-of-match (cover_sequence path); otherwise the per-mid extraction
        // double-counts via the merge vertex.
        যদি (!result.termIsMidMerge) { graph.addLink(result.termVertexp, graph.m_matchVertexp); }
        প্রতি (SvaStateVertex* srcVtxp : result.midSources) {
            Ast_গ্রন্থি_রাশিমালা* condp = nullptr;
            প্রতি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব tc : srcVtxp->m_throughoutConds) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব tcClone = tc->clone_বৃক্ষ_Pure(অসত্য);
                condp = condp ? নূতন AstLogAnd{flp, condp, tcClone} : tcClone;
            }
            graph.addLink(srcVtxp, graph.m_matchVertexp, condp);
            srcVtxp->m_isUnbounded = সত্য;
        }
    }

    স্থির Ast_গ্রন্থি_রাশিমালা* getSequenceBodyExprp(ধ্রুব AstSequence* seqp) {
        Ast_গ্রন্থি* bodyp = seqp->stmtsp();
        যাবত্ (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        প্রত্যাগম VN_CAST(bodyp, গ্রন্থি_রাশিমালা);
    }

    স্থির AstPropSpec* getPropertySpecp(ধ্রুব AstProperty* propp) {
        Ast_গ্রন্থি* stmtp = propp->stmtsp();
        // V3LinkParse emits InitialStaticStmt for property-local variable
        // initialisers; the InitialAutomaticStmt variant only appears for
        // task/function-scope automatic lifetime, not properties.
        যাবত্ (stmtp
               && (VN_IS(stmtp, Var) || VN_IS(stmtp, InitialStaticStmt)
                   || VN_IS(stmtp, InitialAutomaticStmt))) {  // LCOV_EXCL_LINE
            stmtp = stmtp->nextp();
        }
        প্রত্যাগম VN_CAST(stmtp, PropSpec);
    }

    শূন্য inlineNamedProperty(AstPropSpec* outerSpecp, AstFuncRef* funcrefp,
                             ধ্রুব AstProperty* propyp) {
        // Recursion guard: IEEE 1800-2023 16.12.1 forbids recursive properties.
        // V3Width emits "Recursive property call" for direct recursion before this
        // pass runs; this catches any nested-inlining cycle that slips past.
        যদি (!m_inliningProps.emplace(propyp).second) {
            funcrefp->v3error("Illegal recursive property reference");  // LCOV_EXCL_LINE
            প্রত্যাগম;  // LCOV_EXCL_LINE
        }
        সংরচনা Guard final {
            std::set<ধ্রুব AstProperty*>& setr;
            ধ্রুব AstProperty* keyp;
            ~Guard() { setr.erase(keyp); }
        } guard{m_inliningProps, propyp};
        AstPropSpec* propSpecp = getPropertySpecp(propyp);
        UASSERT_OBJ(propSpecp, funcrefp, "Property has no body PropSpec");
        propSpecp = propSpecp->clone_বৃক্ষ(অসত্য);

        ধ্রুব V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propyp->stmtsp());
        std::unordered_map<ধ্রুব AstVar*, Ast_গ্রন্থি_রাশিমালা*> portMap;
        প্রতি (ধ্রুব auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables to module-level temps (IEEE 16.10).
        std::unordered_map<ধ্রুব AstVar*, AstVar*> localVarMap;
        প্রতি (Ast_গ্রন্থি* stmtp = propyp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstVar* ধ্রুব varp = VN_CAST(stmtp, Var)) {
                যদি (!varp->isIO()) {
                    ধ্রুব string newName = m_propVarNames.get(varp);
                    AstVar* ধ্রুব newVarp = নূতন AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        propSpecp->foreach([&](AstVarRef* refp) {
            ধ্রুব auto portIt = portMap.find(refp->varp());
            যদি (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_বৃক্ষ(অসত্য));
                VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
                প্রত্যাগম;
            }
            ধ্রুব auto localIt = localVarMap.find(refp->varp());
            যদি (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        প্রতি (ধ্রুব auto& tconnect : tconnects) {
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
            AstSenItem* ধ্রুব sensesp = outerSpecp->sensesp();
            sensesp->unlinkFrBack();
            propSpecp->sensesp(sensesp);
        }

        outerSpecp->replaceWith(propSpecp);
        VL_DO_DANGLING(ঠেলো_Deletep(outerSpecp), outerSpecp);
    }

    শূন্য inlineSequenceRef(AstFuncRef* funcrefp, AstSequence* seqp) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব clonedp = bodyExprp->clone_বৃক্ষ(অসত্য);

        ধ্রুব V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ধ্রুব AstVar*, Ast_গ্রন্থি_রাশিমালা*> portMap;
        প্রতি (ধ্রুব auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ধ্রুব auto it = portMap.find(refp->varp());
            যদি (it != portMap.end()) {
                refp->replaceWith(it->second->clone_বৃক্ষ(অসত্য));
                VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
            }
        });
        প্রতি (ধ্রুব auto& tconnect : tconnects) {
            ঠেলো_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ঠেলো_Deletep(funcrefp), funcrefp);
        // Clear referenced flag so V3AssertPre cleanup does not emit
        // spurious UNSUPPORTED for sequences that were already inlined here.
        seqp->isReferenced(অসত্য);
    }

    // Must run before hasMultiCycleExpr() so NFA sees sequence bodies.
    শূন্য inlineAllSequenceRefs(Ast_গ্রন্থি* rootp) {
        তার্কিক changed = সত্য;
        যাবত্ (changed) {
            changed = অসত্য;
            rootp->foreach([&](AstFuncRef* funcrefp) {
                যদি (changed) প্রত্যাগম;
                যদি (AstSequence* ধ্রুব seqp = VN_CAST(funcrefp->taskp(), Sequence)) {
                    inlineSequenceRef(funcrefp, seqp);
                    changed = সত্য;
                }
            });
        }
    }

    স্থির তার্কিক has_Multi_Cycle_রাশিমালা(ধ্রুব Ast_গ্রন্থি* nodep) {
        প্রত্যাগম nodep->exists([](ধ্রুব Ast_গ্রন্থি* np) {
            যদি (ধ্রুব auto* ধ্রুব ep = VN_CAST(np, গ্রন্থি_রাশিমালা)) প্রত্যাগম ep->isMultiCycleSva();
            প্রত্যাগম অসত্য;
        });
    }

    স্থির VPropStrength effectiveAssertPropStrength(ধ্রুব AstPropSpec* ধ্রুব propSpecp) {
        যদি (propSpecp->propStrength() != VPropStrength::DEFAULT) প্রত্যাগম propSpecp->propStrength();
        প্রত্যাগম propSpecp->fileline()->language() <= V3LangCode::L1800_2005 ? VPropStrength::STRONG
                                                                           : VPropStrength::WEAK;
    }

    // Bare `assert property (p until q)` with boolean operands stays on
    // V3AssertPre's AstLoop lowering, which preserves per-attempt action-block
    // firings that this NFA's single-bit aggregated state cannot. Strong bare
    // forms are also lowered there. NFA still owns sequence operands and any
    // embedding inside a multi-cycle context (implication consequent, or/and
    // operands, etc.).
    স্থির তার্কিক is_Bare_শীর্ষ_Level_Until(Ast_গ্রন্থি* propp) {
        Ast_গ্রন্থি* p = propp;
        যদি (AstPropSpec* ধ্রুব specp = VN_CAST(p, PropSpec)) p = specp->propp();
        যাবত্ (AstLogNot* ধ্রুব notp = VN_CAST(p, LogNot)) p = notp->lhsp();
        AstUntil* ধ্রুব untilp = VN_CAST(p, Until);
        যদি (!untilp) প্রত্যাগম অসত্য;
        ধ্রুব auto hasSeq = [](ধ্রুব Ast_গ্রন্থি_রাশিমালা* ep) {
            প্রত্যাগম ep->exists([](ধ্রুব Ast_গ্রন্থি_রাশিমালা* np) { প্রত্যাগম np->isMultiCycleSva(); });
        };
        যদি (hasSeq(untilp->lhsp()) || hasSeq(untilp->rhsp())) প্রত্যাগম অসত্য;
        প্রত্যাগম সত্য;
    }

    সংরচনা PropertyParts final {
        Ast_গ্রন্থি_রাশিমালা* triggerExprp = nullptr;
        Ast_গ্রন্থি_রাশিমালা* seqExprp = nullptr;
        তার্কিক isOverlapped = সত্য;
        তার্কিক hasImplication = অসত্য;
        তার্কিক isFollowedBy = অসত্য;  // True for #-# / #=# (non-vacuous-fail on antecedent miss)
    };

    স্থির PropertyParts decomposeProperty(Ast_গ্রন্থি* propp) {
        PropertyParts parts;
        যদি (AstPropSpec* ধ্রুব specp = VN_CAST(propp, PropSpec)) { propp = specp->propp(); }
        যদি (AstImplication* ধ্রুব implp = VN_CAST(propp, Implication)) {
            parts.hasImplication = সত্য;
            parts.isOverlapped = implp->isOverlapped();
            parts.isFollowedBy = implp->isFollowedBy();
            parts.triggerExprp = implp->lhsp();
            parts.seqExprp = implp->rhsp();
        } অন্যথা যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = VN_CAST(propp, গ্রন্থি_রাশিমালা)) {
            parts.triggerExprp = nullptr;
            parts.seqExprp = exprp;
        }
        প্রত্যাগম parts;
    }

    স্থির তার্কিক canSplitImplicationPassActions(ধ্রুব PropertyParts& parts) {
        UASSERT(parts.hasImplication,
                "Implication pass action split requested without implication");
        UASSERT(parts.triggerExprp, "Implication pass action split requested without trigger");
        // Direct vacuous/nonvacuous classification uses the antecedent value in the current
        // assertion attempt. Leave delayed antecedents on the existing NFA pass path.
        প্রত্যাগম !has_Multi_Cycle_রাশিমালা(parts.triggerExprp);
    }

    স্থির শূন্য splitImplicationPassActions(AstAssert* assertp, ধ্রুব PropertyParts& parts,
                                            Ast_গ্রন্থি_রাশিমালা* nonvacuousMatchp) {
        File_সারি* ধ্রুব flp = assertp->fileline();

        Ast_গ্রন্থি* ধ্রুব passsp = assertp->passsp()->unlinkFrBackWithNext();
        Ast_গ্রন্থি* splitsp = nullptr;

        যদি (!parts.isFollowedBy) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব vacuousp
                = নূতন AstLogNot{flp, sampled(parts.triggerExprp->clone_বৃক্ষ_Pure(অসত্য))};
            Ast_গ্রন্থি* ধ্রুব vacuousBodyp = passsp->clone_বৃক্ষ(অসত্য);
            splitsp = newPassOnIf(flp, vacuousp, vacuousBodyp, assertp->userType(),
                                  assertp->directive(), /*vacuous=*/সত্য);
        }

        AstIf* ধ্রুব nonvacuousp
            = newPassOnIf(flp, nonvacuousMatchp, passsp, assertp->userType(), assertp->directive(),
                          /*vacuous=*/অসত্য);
        splitsp = splitsp ? Ast_গ্রন্থি::addNext<Ast_গ্রন্থি, Ast_গ্রন্থি>(splitsp, nonvacuousp)
                          : static_cast<Ast_গ্রন্থি*>(nonvacuousp);
        assertp->addPasssp(splitsp);
    }

    // On a PropSpec-wrapped assertion whose NFA build failed with a semantic
    // error (errorEmitted), replace the body with a BitFalse const so later
    // passes see a well-formed AST. Returns true if replaced.
    শূন্য replaceBodyOnBuildError(File_সারি* flp, AstPropSpec* propSpecp, তার্কিক errorEmitted) {
        যদি (!errorEmitted) প্রত্যাগম;
        Ast_গ্রন্থি* ধ্রুব innerPropp = propSpecp->propp();
        innerPropp->replaceWith(নূতন AstConst{flp, AstConst::BitFalse{}});
        VL_DO_DANGLING(ঠেলো_Deletep(innerPropp), innerPropp);
    }

    // Hoist a leading clocking event (IEEE 1800-2023 16.7):
    তার্কিক hoistClockedSeq(AstPropSpec* specp) {
        যাবত্ (AstSClocked* ধ্রুব clockedp = VN_CAST(specp->propp(), SClocked)) {
            যদি (specp->sensesp()) {
                clockedp->v3warn(E_UNSUPPORTED, "Unsupported: multiclocked sequence or property");
                replaceBodyOnBuildError(specp->fileline(), specp, সত্য);
                প্রত্যাগম সত্য;
            }
            প্রতি (ধ্রুব AstSenItem* sp = clockedp->sensesp(); sp;
                 sp = VN_CAST(sp->nextp(), SenItem)) {
                যদি (!sp->edgeType().anEdge()) {
                    clockedp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: non-edge clocking event on a sequence; "
                                     "use an edge such as @(posedge clk)");
                    replaceBodyOnBuildError(specp->fileline(), specp, সত্য);
                    প্রত্যাগম সত্য;
                }
            }
            specp->sensesp(clockedp->sensesp()->unlinkFrBackWithNext());
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব bodyp = clockedp->exprp()->unlinkFrBack();
            clockedp->replaceWith(bodyp);
            VL_DO_DANGLING(ঠেলো_Deletep(clockedp), clockedp);
        }
        // A clocking event anywhere else in the sequence is not supported.
        ধ্রুব AstSClocked* nestedp = nullptr;
        specp->propp()->foreach([&](ধ্রুব AstSClocked* p) {
            যদি (!nestedp) nestedp = p;
        });
        যদি (nestedp) {
            nestedp->v3warn(E_UNSUPPORTED,
                            "Unsupported: clocking event inside sequence expression");
            replaceBodyOnBuildError(specp->fileline(), specp, সত্য);
            প্রত্যাগম সত্য;
        }
        প্রত্যাগম অসত্য;
    }

    // Build the NFA graph for a property body, handling both the antecedent
    // |-> consequent and simple sequence cases. Returns the consequent/body
    // BuildResult (invalid on parse/build failure).
    BuildResult buildAssertionGraph(SvaNfaBuilder& builder, SvaGraph& graph, Ast_গ্রন্থি_রাশিমালা* seqBodyp,
                                    ধ্রুব PropertyParts& parts, File_সারি* flp) {
        যদি (!parts.hasImplication) প্রত্যাগম builder.build(seqBodyp);

        graph.m_startVertexp = graph.createStateVertex();
        প্রত্যাগম builder.buildImplicationEdges(parts.triggerExprp, seqBodyp, graph.m_startVertexp,
                                             parts.isOverlapped, parts.isFollowedBy,
                                             parts.triggerExprp, flp);
    }

    // Install pass-action gating and replay simultaneous per-thread failures.
    শূন্য attachActionHandlers(AstAssert* ধ্রুব assertp, Ast_গ্রন্থি_রাশিমালা* matchExprp,
                              Ast_Sen_বৃক্ষ* ধ্রুব threadFailReplaySenTreep,
                              Ast_গ্রন্থি_রাশিমালা* ধ্রুব threadFailCountp) {
        // Gate pass handler on match to prevent vacuous-pass firings.
        যদি (matchExprp) {
            // needMatch implies passsp() was non-null when evaluated above;
            // lowering does not mutate the assert's pass-action between the
            // two reads, so passsp() is still non-null here.
            Ast_গ্রন্থি* passsp = assertp->passsp();
            UASSERT_OBJ(passsp, assertp, "needMatch set but passsp is null");
            passsp->unlinkFrBackWithNext();
            File_সারি* ধ্রুব flp = assertp->fileline();
            AstIf* ধ্রুব ifp = নূতন AstIf{flp, matchExprp, passsp, nullptr};
            assertp->addPasssp(ifp);
            // Fail-handler prefix for overlapping instances (IEEE 16.12):
            // fires when reject=1 && match=1 in the same cycle.
            যদি (assertp->failsp()) assertp->prependFailsp(ifp->clone_বৃক্ষ(অসত্য));
        }

        যদি (threadFailCountp) {
            UASSERT_OBJ(threadFailReplaySenTreep, assertp,
                        "Thread fail count missing sensitivity tree");
            Ast_গ্রন্থি* ধ্রুব failsp = assertp->failsp();
            File_সারি* ধ্রুব flp = assertp->fileline();
            // IEEE 1800-2023 16.12 requires one action-block evaluation per failed
            // thread. AstAssert handles the first, so replay the rest here.
            AstVar* ধ্রুব remaining_Fail_গুনতি_Varp
                = নূতন AstVar{flp, VVarType::MODULETEMP, m_fail_গুনতি_Names.get(""),
                             m_modp->findBasicDType(VBasicDTypeKwd::UINT32)};
            remaining_Fail_গুনতি_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remaining_Fail_গুনতি_Varp);
            Ast_গ্রন্থি* ধ্রুব replayStmtsp = নূতন AstAssign{
                flp, নূতন AstVarRef{flp, remaining_Fail_গুনতি_Varp, VAccess::WRITE}, threadFailCountp};
            AstLoop* ধ্রুব replayLoopp = নূতন AstLoop{flp};
            replayLoopp->addStmtsp(নূতন AstLoopTest{
                flp, replayLoopp,
                নূতন AstGt{flp, নূতন AstVarRef{flp, remaining_Fail_গুনতি_Varp, VAccess::READ},
                          newTypedConstp(flp, remaining_Fail_গুনতি_Varp->dtypep(), 1)}});
            replayLoopp->addStmtsp(newIfAssertFailOn(failsp->clone_বৃক্ষ(সত্য), assertp->directive(),
                                                     assertp->userType()));
            AstSub* ধ্রুব decrementedFailCountp
                = নূতন AstSub{flp, নূতন AstVarRef{flp, remaining_Fail_গুনতি_Varp, VAccess::READ},
                             newTypedConstp(flp, remaining_Fail_গুনতি_Varp->dtypep(), 1)};
            replayLoopp->addStmtsp(
                নূতন AstAssign{flp, নূতন AstVarRef{flp, remaining_Fail_গুনতি_Varp, VAccess::WRITE},
                              decrementedFailCountp});
            replayStmtsp->addNext(replayLoopp);
            m_modp->addStmtsp(
                নূতন AstAlways{flp, VAlwaysKwd::ALWAYS, threadFailReplaySenTreep, replayStmtsp});
        }
    }

    // Replace one VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). No-op if refp is not in matchMap.
    শূন্য substituteMatchItemRef(AstVarRef* refp, unsigned K,
                                ধ্রুব std::unordered_map<ধ্রুব AstVar*, Ast_গ্রন্থি_রাশিমালা*>& matchMap) {
        ধ্রুব auto it = matchMap.find(refp->varp());
        যদি (it == matchMap.end()) প্রত্যাগম;
        Ast_গ্রন্থি_রাশিমালা* newp = it->second->clone_বৃক্ষ_Pure(অসত্য);
        যদি (K > 0) {
            AstConst* ধ্রুব ticksp = নূতন AstConst{refp->fileline(), AstConst::Widthed_মান{}, 32,
                                                  static_cast<uint32_t>(K)};
            AstPast* ধ্রুব pastp = নূতন AstPast{refp->fileline(), newp, ticksp, nullptr,
                                               /* propertyTiming */ সত্য};
            pastp->dtypeFrom(newp);
            newp = pastp;
        }
        refp->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
        প্রত্যাগম;
    }

    // Recursively walk a consequent. Returns cycle length consumed and
    // substitutes each VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). Reports E_UNSUPPORTED on non-constant
    // delays or composite sequence operators and returns -1 on failure.
    পূর্ণাঙ্ক
    walk_Substitute_Match_বস্তুসমূহ(Ast_গ্রন্থি_রাশিমালা* nodep, unsigned K,
                             ধ্রুব std::unordered_map<ধ্রুব AstVar*, Ast_গ্রন্থি_রাশিমালা*>& match_বস্তুসমূহ) {
        যদি (AstSExpr* ধ্রুব sexprp = VN_CAST(nodep, SExpr)) {
            // IEEE 1800-2023 16.9.2: cycle_delay's lhsp is a constant_expression
            // and the delay form in a sequence is always `##N`, folded by
            // V3Const + V3Param before V3AssertNfa. Range form `##[m:n]` is the
            // only user-visible reject here.
            AstDelay* ধ্রুব delayp = VN_AS(sexprp->delayp(), Delay);
            UASSERT_OBJ(delayp->isCycleDelay() && VN_IS(delayp->lhsp(), Const), sexprp,
                        "SVA cycle delay must have a constant lhsp");
            যদি (delayp->isRangeDelay()) {
                sexprp->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                              "non-constant cycle delay in consequent"
                                              " (IEEE 1800-2023 16.10)");
                প্রত্যাগম -1;
            }
            ধ্রুব unsigned delayCycles = VN_AS(delayp->lhsp(), Const)->toUInt();
            পূর্ণাঙ্ক preLen = 0;
            যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুব prep = sexprp->preExprp()) {
                preLen = walk_Substitute_Match_বস্তুসমূহ(prep, K, match_বস্তুসমূহ);
                যদি (preLen < 0) প্রত্যাগম -1;
            }
            ধ্রুব পূর্ণাঙ্ক bodyLen
                = walk_Substitute_Match_বস্তুসমূহ(sexprp->exprp(), K + preLen + delayCycles, match_বস্তুসমূহ);
            যদি (bodyLen < 0) প্রত্যাগম -1;
            প্রত্যাগম preLen + delayCycles + bodyLen;
        }
        যদি (nodep->isMultiCycleSva()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                         "composite sequence operator in consequent"
                                         " (IEEE 1800-2023 16.10)");
            প্রত্যাগম -1;
        }
        std::vector<AstVarRef*> refs;
        nodep->foreach([&refs](AstVarRef* p) { refs.ঠেলো_back(p); });
        প্রতি (AstVarRef* ধ্রুব refp : refs) substituteMatchItemRef(refp, K, match_বস্তুসমূহ);
        প্রত্যাগম 0;
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
    তার্কিক liftMatchItemSubstitutions(PropertyParts& parts, Ast_গ্রন্থি_রাশিমালা* seqBodyp) {
        যদি (!parts.hasImplication) প্রত্যাগম অসত্য;
        Ast_রাশিমালা_Stmt* ধ্রুব রাশিমালা_Stmtp = VN_CAST(parts.triggerExprp, রাশিমালা_Stmt);
        যদি (!রাশিমালা_Stmtp) প্রত্যাগম অসত্য;
        // IEEE 1800-2023 16.10 BNF requires `(expr, match_item {, match_item})`
        // with at least one match item; V3LinkParse only emits ExprStmt for
        // this form and only emits AstAssign with VarRef LHS for each item.
        std::unordered_map<ধ্রুব AstVar*, Ast_গ্রন্থি_রাশিমালা*> match_বস্তুসমূহ;
        প্রতি (Ast_গ্রন্থি* stmtp = রাশিমালা_Stmtp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstAssign* ধ্রুব assignp = VN_AS(stmtp, Assign);
            AstVarRef* ধ্রুব lhsRefp = VN_AS(assignp->lhsp(), VarRef);
            match_বস্তুসমূহ[lhsRefp->varp()] = assignp->rhsp();
        }
        ধ্রুব unsigned startK = parts.isOverlapped ? 0 : 1;
        ধ্রুব পূর্ণাঙ্ক length = walk_Substitute_Match_বস্তুসমূহ(seqBodyp, startK, match_বস্তুসমূহ);
        // Match-item substitution / strip mutates ancestor purity. Release
        // builds don't auto-clear caches on edits, so refresh here.
        VIsCached::clear_Cache_বৃক্ষ();
        যদি (length < 0) প্রত্যাগম সত্য;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব antBoolp = রাশিমালা_Stmtp->resultp()->unlinkFrBack();
        রাশিমালা_Stmtp->replaceWith(antBoolp);
        VL_DO_DANGLING(ঠেলো_Deletep(রাশিমালা_Stmtp), রাশিমালা_Stmtp);
        parts.triggerExprp = antBoolp;
        প্রত্যাগম অসত্য;
    }

    // Outcome counts for a property if/case are wrong in an outcome-multiplying
    // context. Returns that context, or nullptr when the shape is supported.
    স্থির ধ্রুব বর্ণ* unsupportedPropertyControl(ধ্রুব Ast_গ্রন্থি_Cover_Or_Assert* assertp,
                                                  ধ্রুব Ast_গ্রন্থি_রাশিমালা* seqBodyp, তার্কিক negated) {
        যদি (!hasPropertyControlConjunction(seqBodyp)) প্রত্যাগম nullptr;
        যদি (negated) প্রত্যাগম "negation";
        যদি (VN_IS(assertp, Cover)) প্রত্যাগম "cover";
        যদি (VN_AS(assertp, Assert)->passsp()) প্রত্যাগম "a pass action";
        প্রত্যাগম nullptr;
    }

    // Inline property/sequence refs and reject unsupported shapes.
    // Returns the PropSpec to lower, or nullptr when fully handled here.
    AstPropSpec* prepareAssertionProp(Ast_গ্রন্থি_Cover_Or_Assert* assertp) {
        যদি (AstPropSpec* ধ্রুব specp = VN_CAST(assertp->propp(), PropSpec)) {
            যদি (AstFuncRef* ধ্রুব funcrefp = VN_CAST(specp->propp(), FuncRef)) {
                যদি (ধ্রুব AstProperty* ধ্রুব propyp = VN_CAST(funcrefp->taskp(), Property)) {
                    inlineNamedProperty(specp, funcrefp, propyp);
                }
            }
        }

        inlineAllSequenceRefs(assertp->propp());

        যদি (AstPropSpec* ধ্রুব specp = VN_CAST(assertp->propp(), PropSpec)) {
            যদি (hoistClockedSeq(specp)) প্রত্যাগম nullptr;
        }

        AstPropSpec* ধ্রুব propp = VN_AS(assertp->propp(), PropSpec);
        যদি (!VN_IS(assertp, Cover)
            && effectiveAssertPropStrength(propp) == VPropStrength::STRONG) {
            propp->v3warn(E_UNSUPPORTED,
                          "Unsupported: strong property in " + assertp->verilogKwd() + ".");
            replaceBodyOnBuildError(assertp->fileline(), propp, /*errorEmitted=*/সত্য);
            প্রত্যাগম nullptr;
        }

        যদি (!has_Multi_Cycle_রাশিমালা(propp)) প্রত্যাগম nullptr;
        // A nested property instance keeps its body behind the call; lowering would drop it.
        যদি (propp->exists([](ধ্রুব AstFuncRef* refp) { প্রত্যাগম VN_IS(refp->taskp(), Property); })) {
            assertp->v3warn(E_UNSUPPORTED,
                            "Unsupported: property instance inside a multi-cycle property "
                            "expression");
            VL_DO_DANGLING(ঠেলো_Deletep(assertp->unlinkFrBack()), assertp);
            প্রত্যাগম nullptr;
        }
        যদি (is_Bare_শীর্ষ_Level_Until(propp)) প্রত্যাগম nullptr;
        প্রত্যাগম propp;
    }

    শূন্য processAssertion(Ast_গ্রন্থি_Cover_Or_Assert* assertp) {
        যদি (assertp->immediate()) প্রত্যাগম;

        AstPropSpec* ধ্রুব propp = prepareAssertionProp(assertp);
        যদি (!propp) প্রত্যাগম;

        PropertyParts parts = decomposeProperty(propp);
        UASSERT_OBJ(parts.seqExprp, propp, "Property body must be an expression");

        // Unwrap `not` (IEEE 1800-2023 16.12.1); odd count -> negated semantics.
        Ast_গ্রন্থি_রাশিমালা* seqBodyp = parts.seqExprp;
        তার্কিক negated = অসত্য;
        যাবত্ (AstLogNot* ধ্রুব notp = VN_CAST(seqBodyp, LogNot)) {
            negated = !negated;
            seqBodyp = notp->lhsp();
        }

        ধ্রুব বর্ণ* ধ্রুব propertyControlp
            = unsupportedPropertyControl(assertp, seqBodyp, negated);

        // Substitute property-local match-item refs in consequent with
        // $past(rhs, K) before NFA build (IEEE 1800-2023 16.10).
        যদি (liftMatchItemSubstitutions(parts, seqBodyp)) {
            AstPropSpec* ধ্রুব psp = VN_CAST(assertp->propp(), PropSpec);
            UASSERT_OBJ(psp, assertp, "Concurrent assertion must have PropSpec");
            replaceBodyOnBuildError(assertp->fileline(), psp, /*errorEmitted=*/সত্য);
            প্রত্যাগম;
        }

        AstCover* ধ্রুব coverp = VN_CAST(assertp, Cover);
        ধ্রুব তার্কিক isCoverSeq = coverp && coverp->isCoverSeq();
        // A sequence event control is not an assertion directive; no default
        // disable iff, no assertion control
        ধ্রুব তার্কিক isSeqEvent = coverp && coverp->isSeqEvent();
        ধ্রুব তার্কিক গুনতি_Matches = isCoverSeq && !isSeqEvent;
        // Inherit module defaults (IEEE 14.12, 16.15) when assertion has none.
        যদি (!propp->sensesp() && m_defaultClockingp) {
            propp->sensesp(m_defaultClockingp->sentreep()->sensesp()->clone_বৃক্ষ(সত্য));
        }
        যদি (!propp->disablep() && m_defaultDisablep && !isSeqEvent) {
            propp->disablep(m_defaultDisablep->condp()->clone_বৃক্ষ_Pure(সত্য));
        }
        যদি (!propp->sensesp()) প্রত্যাগম;
        Ast_Sen_বৃক্ষ* senTreep
            = নূতন Ast_Sen_বৃক্ষ{propp->fileline(), propp->sensesp()->clone_বৃক্ষ(সত্য)};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব disableExprp = propp->disablep();

        // NFA lowering clones repeated operands and may hoist them into an
        // always_comb block. Resolve implicit sampled-value clocks first, while
        // the enclosing assertion clock is still available.
        {
            VL_RESTORER(m_sampled_মান_Clockp);
            m_sampled_মান_Clockp = senTreep;
            iterate(propp->propp());
        }

        File_সারি* ধ্রুব flp = assertp->fileline();

        SvaGraph graph;
        SvaNfaBuilder builder{graph,      m_modp,     m_propTempNames,
                              isCoverSeq, isSeqEvent, coverp != nullptr};

        ধ্রুব BuildResult result = buildAssertionGraph(builder, graph, seqBodyp, parts, flp);
        যদি (result.valid()) wireMatchAndMidSources(graph, result, flp);
        যদি (!result.valid()) {
            // Fall through to V3AssertPre for unsupported constructs; only
            // replace the body on real semantic errors. Any hoisted temps
            // from this attempt become orphan MODULETEMPs; V3Dead removes
            // them along with the dead always_comb driver.
            replaceBodyOnBuildError(flp, propp, result.errorEmitted);
            VL_DO_DANGLING(ঠেলো_Deletep(senTreep), senTreep);
            প্রত্যাগম;
        }
        // After the build, so a construct the builder rejects reports itself.
        যদি (propertyControlp) {
            seqBodyp->v3warn(E_UNSUPPORTED,
                             "Unsupported: temporal property if/case with " << propertyControlp);
            replaceBodyOnBuildError(flp, propp, /*errorEmitted=*/সত্য);
            VL_DO_DANGLING(ঠেলো_Deletep(senTreep), senTreep);
            প্রত্যাগম;
        }
        যদি (গুনতি_Matches && !v3Global.opt.coverageUser()) {
            SvaNfaBuilder::freeUnlinkedCondp(result.finalCondp);
            VL_DO_DANGLING(ঠেলো_Deletep(senTreep), senTreep);
            Ast_গ্রন্থি* ধ্রুব innerPropp = propp->propp();
            innerPropp->replaceWith(নূতন AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(ঠেলো_Deletep(innerPropp), innerPropp);
            প্রত্যাগম;
        }

        AstAssert* ধ্রুব assertAssertp = VN_CAST(assertp, Assert);
        ধ্রুব তার্কিক splitImplicationPasssp = assertAssertp && assertAssertp->passsp()
                                            && parts.hasImplication
                                            && canSplitImplicationPassActions(parts);
        ধ্রুব তার্কিক needMatch = assertAssertp && assertAssertp->passsp()
                               && (!parts.hasImplication || splitImplicationPasssp);

        ধ্রুব তার্কিক needsThreadFailReplay
            = assertAssertp && assertAssertp->failsp() && !parts.hasImplication;
        ধ্রুব auto signals
            = m_loweringp->lower(assertp, graph, senTreep, result.finalCondp, disableExprp,
                                 needsThreadFailReplay, needsThreadFailReplay && !negated);
        Ast_গ্রন্থি_রাশিমালা* matchExprp = nullptr;
        Ast_গ্রন্থি_রাশিমালা* outputExprp = m_loweringp->assembleResult(
            assertp, negated, result.finalCondp, signals, needMatch ? &matchExprp : nullptr);
        যদি (গুনতি_Matches) {
            UASSERT_OBJ(signals.matchCountp, coverp, "Cover sequence missing match count");
            VL_DO_DANGLING(outputExprp->delete_বৃক্ষ(), outputExprp);
            propp->matchCountp(signals.matchCountp);
            outputExprp = নূতন AstNeq{flp, signals.matchCountp->clone_বৃক্ষ_Pure(অসত্য),
                                     newTypedConstp(flp, signals.matchCountp->dtypep(), 0)};
        }

        Ast_Sen_বৃক্ষ* ধ্রুব threadFailReplaySenTreep
            = signals.threadFailCountp ? senTreep->clone_বৃক্ষ(অসত্য) : nullptr;

        VL_DO_DANGLING(ঠেলো_Deletep(senTreep), senTreep);
        যদি (result.finalCondp && !result.finalCondp->backp()) ঠেলো_Deletep(result.finalCondp);

        যদি (splitImplicationPasssp) {
            splitImplicationPassActions(assertAssertp, parts, matchExprp);
        } অন্যথা {
            attachActionHandlers(assertAssertp, matchExprp, threadFailReplaySenTreep,
                                 signals.threadFailCountp);
        }

        Ast_গ্রন্থি* ধ্রুব innerPropp = propp->propp();
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

সার্বজনিক:
    explicit AssertNfaVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Top entry point

শূন্য V3AssertNfa::assertNfaAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":" << endl);
    { AssertNfaVisitor{nodep}; }
    V3Global::dump_Check_Global_বৃক্ষ("assertnfa", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
