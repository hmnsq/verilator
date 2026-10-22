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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3AssertNfa.h"

#ଅନ୍ତର୍ଭାଵ "V3Assert.h"
#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"
#ଅନ୍ତର୍ଭାଵ "V3Task.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"

#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <unordered_set>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// NFA Graph Data Structures (V3Graph-derived per upstream convention)

ନାମାକାଶ {

ଶ୍ରେଣୀ SvaStateVertex;

// Per-vertex algorithm data, stored via V3GraphVertex::userp() during lowering
ସଂରଚନା SvaVertexData final {
    AstVar* stateVarp = nullptr;  // NBA state register for this vertex
    AstVar* delayRingVarp = nullptr;  // Packed occupancy bits or per-slot match counts
    AstVar* delayRingHeadVarp
        = nullptr;  // Cached outgoing slot; isolates readers from in-place writes
    AstVar* delayRingIdxVarp = nullptr;  // Next slot written in delayRingVarp
    AstVar* delay_Ring_Live_ଗଣନା_Varp = nullptr;  // Number of threads in delayRingVarp
    AstVar* delayRingWrappedVarp = nullptr;  // All slots written since the last clear
    AstVar* doneLVarp = nullptr;  // SAnd LHS done-latch
    AstVar* doneRVarp = nullptr;  // SAnd RHS done-latch
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* stateSigp = nullptr;  // Combinational state signal; OWNED during lowering
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* state_ଗଣନା_Sigp = nullptr;  // Cover-sequence match count; OWNED during lowering
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* state_ଗଣନା_Overflow_Sigp = nullptr;  // Count overflow; OWNED during lowering
    ତାର୍କିକ delayRingCounts = ମିଥ୍ୟା;  // Ring slots hold cover-sequence match counts
    ତାର୍କିକ needsReg = ମିଥ୍ୟା;  // True if vertex has incoming clocked edge
};

// NFA state vertex -- one per NFA position in the sequence evaluation
ଶ୍ରେଣୀ SvaStateVertex final : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(SvaStateVertex, V3GraphVertex)
ସର୍ବସାଧାରଣ:
    // True if this is the sequence-match terminal vertex
    ତାର୍କିକ m_isMatch = ମିଥ୍ୟା;
    // OWNED throughout-guard condition clones; IEEE 1800-2023 16.9.9
    std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> m_throughoutConds;
    // Nonzero for a ring-buffer vertex for ## delays.
    ତାର୍କିକ m_isFixedDelayRing = ମିଥ୍ୟା;
    unsigned m_delayRingSize = 0;  // Number of ring slots. Range: max-min+1.
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_delayRingClearCondp = nullptr;  // local RHS for pure-boolean range
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_delayRingAdvanceCondp = nullptr;  // Advance only when this condition holds
    ତାର୍କିକ m_replayAbortReject = ମିଥ୍ୟା;  // Compressed repetition needs per-thread abort replay
    // OWNED; enclosing-abort fire condition clearing state or suppressing guard rejection
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_abortClearp = nullptr;
    // OWNED; reject-abort fire condition rejecting all represented live threads
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_abortRejectp = nullptr;
    // Liveness terminal (IEEE weak semantics): reject must not fire from this source
    ତାର୍କିକ m_isUnbounded = ମିଥ୍ୟା;
    // Temporal sequence AND combiner; IEEE 1800-2023 16.9.5
    ତାର୍କିକ m_isAndCombiner = ମିଥ୍ୟା;
    SvaStateVertex* m_andLhsTermp = nullptr;  // LHS sub-NFA terminal vertex
    SvaStateVertex* m_andRhsTermp = nullptr;  // RHS sub-NFA terminal vertex
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_andLhsCondp = nullptr;  // OWNED; LHS final condition (may be nullptr)
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_andRhsCondp = nullptr;  // OWNED; RHS final condition (may be nullptr)
    // Reject sink for SAnd rejectOnFail wiring; not a state-signal source
    ତାର୍କିକ m_isRejectSink = ମିଥ୍ୟା;
    // In-window vertex of a strong s_always[m:n]: if its state is still set at
    // end-of-simulation the universal-quantifier window never completed, which is
    // a liveness failure (IEEE 1800-2023 16.12.11 strong semantics).
    ତାର୍କିକ m_strongPending = ମିଥ୍ୟା;
    // Ring introduced for the checked portion of a bounded strong s_always window.
    ତାର୍କିକ m_strongAlwaysRing = ମିଥ୍ୟା;

    // CONSTRUCTORS
    explicit SvaStateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~SvaStateVertex() override {
        ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* cp : m_throughoutConds) VL_DO_DANGLING(cp->delete_ବୃକ୍ଷ(), cp);
        ଯଦି (m_delayRingClearCondp)
            VL_DO_DANGLING(m_delayRingClearCondp->delete_ବୃକ୍ଷ(), m_delayRingClearCondp);
        ଯଦି (m_delayRingAdvanceCondp)
            VL_DO_DANGLING(m_delayRingAdvanceCondp->delete_ବୃକ୍ଷ(), m_delayRingAdvanceCondp);
        ଯଦି (m_abortClearp) VL_DO_DANGLING(m_abortClearp->delete_ବୃକ୍ଷ(), m_abortClearp);
        ଯଦି (m_abortRejectp) VL_DO_DANGLING(m_abortRejectp->delete_ବୃକ୍ଷ(), m_abortRejectp);
        ଯଦି (m_andLhsCondp) VL_DO_DANGLING(m_andLhsCondp->delete_ବୃକ୍ଷ(), m_andLhsCondp);
        ଯଦି (m_andRhsCondp) VL_DO_DANGLING(m_andRhsCondp->delete_ବୃକ୍ଷ(), m_andRhsCondp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string name() ଅଚଳ override {
        string name = "s" + cvtToStr(color());
        ଯଦି (m_delayRingSize) {
            name += "\\n";
            name += m_isFixedDelayRing ? "fixed chain " : "range chain ";
            name += cvtToStr(m_delayRingSize) + " slots";
        }
        ଫେରନ୍ତୁ name;
    }
    string dotColor() ଅଚଳ override {
        ଯଦି (m_isMatch) ଫେରନ୍ତୁ "red";
        ଯଦି (m_delayRingSize) ଫେରନ୍ତୁ "blue";
        ଯଦି (m_isAndCombiner) ଫେରନ୍ତୁ "purple";
        ଫେରନ୍ତୁ "black";
    }
    // LCOV_EXCL_STOP
    // Access per-vertex algorithm data (valid only during lowering phase)
    SvaVertexData* datap() ଅଚଳ { ଫେରନ୍ତୁ static_cast<SvaVertexData*>(userp()); }
};

// NFA transition edge -- clocked (##1) or combinational link (##0)
ଶ୍ରେଣୀ SvaTransEdge final : ସର୍ବସାଧାରଣ V3GraphEdge {
    VL_RTTI_IMPL(SvaTransEdge, V3GraphEdge)
ସର୍ବସାଧାରଣ:
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_condp;  // Transition condition; nullptr = unconditional; OWNED
    ତାର୍କିକ m_consumesCycle;  // true = clocked edge (##1), false = link (##0/boolean)
    // Reject when source is active and condp is false; set only on
    // outermost required-step Link
    ତାର୍କିକ m_rejectOnFail = ମିଥ୍ୟା;
    // Optional dynamic condition vertex for m_rejectOnFail. Used when the
    // success condition is another NFA state rather than a static expression.
    SvaStateVertex* m_condVtxp = nullptr;

    // CONSTRUCTORS
    SvaTransEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ଶୀର୍ଷ, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp,
                 ତାର୍କିକ consumesCycle)
        : V3GraphEdge{graphp, fromp, ଶୀର୍ଷ, /*weight=*/1}
        , m_condp{condp}
        , m_consumesCycle{consumesCycle} {}
    ~SvaTransEdge() override {
        ଯଦି (m_condp) VL_DO_DANGLING(m_condp->delete_ବୃକ୍ଷ(), m_condp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string dotLabel() ଅଚଳ override { ଫେରନ୍ତୁ m_consumesCycle ? "##1" : "link"; }
    string dotStyle() ଅଚଳ override { ଫେରନ୍ତୁ m_consumesCycle ? "" : "dashed"; }
    // LCOV_EXCL_STOP
    // Typed accessors for NFA vertices
    SvaStateVertex* fromVtxp() ଅଚଳ { ଫେରନ୍ତୁ static_cast<SvaStateVertex*>(fromp()); }
    SvaStateVertex* toVtxp() ଅଚଳ { ଫେରନ୍ତୁ static_cast<SvaStateVertex*>(ଶୀର୍ଷ()); }
};

// NFA graph container
ଶ୍ରେଣୀ SvaGraph final {
ସର୍ବସାଧାରଣ:
    V3Graph m_graph;  // Owns all vertices and edges
    SvaStateVertex* m_startVertexp = nullptr;  // Trigger/start vertex
    SvaStateVertex* m_matchVertexp = nullptr;  // Sequence-match terminal vertex

    // Create a new state vertex
    SvaStateVertex* createStateVertex() { ଫେରନ୍ତୁ ନୂତନ SvaStateVertex{&m_graph}; }
    // Create the match terminal vertex
    SvaStateVertex* createMatchVertex() {
        SvaStateVertex* ଅଚଳ vtxp = createStateVertex();
        vtxp->m_isMatch = ସତ୍ୟ;
        m_matchVertexp = vtxp;
        ଫେରନ୍ତୁ vtxp;
    }
    // Add a clocked transition edge (##1)
    SvaTransEdge* addClockedEdge(SvaStateVertex* fromp, SvaStateVertex* ଶୀର୍ଷ,
                                 Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp = nullptr) {
        ଫେରନ୍ତୁ ନୂତନ SvaTransEdge{&m_graph, fromp, ଶୀର୍ଷ, condp, /*consumesCycle=*/ସତ୍ୟ};
    }
    // Add a combinational link (##0 / boolean condition)
    SvaTransEdge* addLink(SvaStateVertex* fromp, SvaStateVertex* ଶୀର୍ଷ,
                          Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp = nullptr) {
        ଫେରନ୍ତୁ ନୂତନ SvaTransEdge{&m_graph, fromp, ଶୀର୍ଷ, condp, /*consumesCycle=*/ମିଥ୍ୟା};
    }
    // Collect all edges into a flat vector for iteration.
    // Used by the lowering phase which needs global edge scans.
    std::vector<ଅଚଳ SvaTransEdge*> allEdges() ଅଚଳ {
        std::vector<ଅଚଳ SvaTransEdge*> result;
        ପାଇଁ (ଅଚଳ V3GraphVertex& vtxr : m_graph.vertices()) {
            ପାଇଁ (ଅଚଳ V3GraphEdge& edger : vtxr.outEdges()) {
                result.ଠେଲ_back(static_cast<ଅଚଳ SvaTransEdge*>(&edger));
            }
        }
        ଫେରନ୍ତୁ result;
    }
};

//######################################################################
// Builder result: terminal vertex + optional final condition (match Link condition).
ସଂରଚନା BuildResult final {
    SvaStateVertex* termVertexp;  // Primary terminal; contributes to both match and reject
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* finalCondp;  // nullptr = unconditional
    // Mid-window sources for range delays (pure boolean RHS): match-only (isUnbounded)
    std::vector<SvaStateVertex*> midSources;
    ତାର୍କିକ errorEmitted = ମିଥ୍ୟା;  // Builder already emitted specific error; skip generic
    // For cover_sequence: when true, midSources already enumerate every
    // end-of-match, so wireMatchAndMidSources must NOT add the main
    // termVtxp -> matchVertex Link (would double-count via the merge vertex).
    ତାର୍କିକ termIsMidMerge = ମିଥ୍ୟା;
    ତାର୍କିକ valid() ଅଚଳ { ଫେରନ୍ତୁ termVertexp != nullptr; }
    ସ୍ଥିର BuildResult fail(ତାର୍କିକ errored = ମିଥ୍ୟା) { ଫେରନ୍ତୁ {nullptr, nullptr, {}, errored}; }
    ସ୍ଥିର BuildResult failWithError() { ଫେରନ୍ତୁ {nullptr, nullptr, {}, ସତ୍ୟ}; }
};

// Parser-marked SAnd of overlapped implications: a property if/else/case.
ସ୍ଥିର ତାର୍କିକ hasPropertyControlConjunction(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
    ଫେରନ୍ତୁ nodep->exists([](ଅଚଳ AstSAnd* andp) { ଫେରନ୍ତୁ andp->propertyControl(); });
}

ସ୍ଥିର AstConst* newTypedConstp(File_ଧାଡ଼ି* ଅଚଳ flp, ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep,
                                ଅଚଳ uint32_t ମୂଲ୍ୟ) {
    AstConst* ଅଚଳ constp = ନୂତନ AstConst{flp, AstConst::DTyped{}, dtypep};
    constp->ଅଙ୍କ().setLong(ମୂଲ୍ୟ);
    ଫେରନ୍ତୁ constp;
}

ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* sampled(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
    ଫେରନ୍ତୁ ନୂତନ AstSampled{exprp->fileline(), exprp, exprp->dtypep(), ସତ୍ୟ};
}

ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* assertPassOnQuery(ତାର୍କିକ vacuous) {
    ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* queries[2]
        = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
    ଫେରନ୍ତୁ queries[vacuous];
}

ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* assertOnCond(File_ଧାଡ଼ି* flp, VAssertType type,
                                 VAssertDirectiveType directiveType) {
    ଯଦି (!v3Global.opt.assertOn()) { ଫେରନ୍ତୁ ନୂତନ AstConst{flp, AstConst::BitFalse{}}; }
    ଫେରନ୍ତୁ ନୂତନ AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
}

ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* assertKillGet(File_ଧାଡ଼ି* flp, VAssertType type,
                                  VAssertDirectiveType directiveType) {
    ଫେରନ୍ତୁ ନୂତନ AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_KILL", type, directiveType),
                        32};
}

ସ୍ଥିର string assertActionControlPrefix(VAssertDirectiveType directiveType) {
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା controlled = !!(static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(directiveType)
                              & (static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(VAssertDirectiveType::ASSERT)
                                 | static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(VAssertDirectiveType::COVER)
                                 | static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(VAssertDirectiveType::ASSUME)));
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା checkRuntime = controlled & static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(v3Global.opt.assertOn());
    ଫେରନ୍ତୁ "("s + std::to_string(controlled ^ 1) + " || ("s + std::to_string(checkRuntime)
           + " && "s;
}

ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* assertPassOnCond(File_ଧାଡ଼ି* flp, VAssertType type,
                                     VAssertDirectiveType directiveType, ତାର୍କିକ vacuous) {
    ଫେରନ୍ତୁ ନୂତନ AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType)
            + "))"s,
        1};
}

ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* assertFailOnCond(File_ଧାଡ଼ି* flp, VAssertType type,
                                     VAssertDirectiveType directiveType) {
    ଫେରନ୍ତୁ ନୂତନ AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType) + "))"s,
        1};
}

ସ୍ଥିର AstIf* newPassOnIf(File_ଧାଡ଼ି* flp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* firep, Ast_ଗଣ୍ଠି* bodyp, VAssertType type,
                          VAssertDirectiveType directiveType, ତାର୍କିକ vacuous) {
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp
        = ନୂତନ AstLogAnd{flp, firep, assertPassOnCond(flp, type, directiveType, vacuous)};
    AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(ସତ୍ୟ);
    ifp->user1(ସତ୍ୟ);
    ଫେରନ୍ତୁ ifp;
}

ସ୍ଥିର Ast_ଗଣ୍ଠି_Stmt* newIfAssertFailOn(Ast_ଗଣ୍ଠି* bodyp, VAssertDirectiveType directiveType,
                                      VAssertType type) {
    File_ଧାଡ଼ି* ଅଚଳ flp = bodyp->fileline();
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = assertFailOnCond(flp, type, directiveType);
    AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(ସତ୍ୟ);
    ifp->user1(ସତ୍ୟ);
    ଫେରନ୍ତୁ ifp;
}

//######################################################################
// NFA Builder

ଶ୍ରେଣୀ SvaNfaBuilder final {
    SvaGraph& m_graph;  // NFA graph being built
    Ast_ଗଣ୍ଠି_Module* ଅଚଳ m_modp;  // Module to receive hoisted sampled-prop temps
    V3UniqueNames& m_propTempNames;  // Module-shared temp-var name source
    std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> m_temporal_Guard_ଗଦା;  // Guards active across nested temporal states
    // Outer abort conditions, AND-ed as !cond into inner abort edges
    // (IEEE 1800-2023 16.12.14 outer-wraps-inner).
    std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> m_outer_Abort_ଗଦା;
    ତାର୍କିକ m_inUnboundedScope = ମିଥ୍ୟା;  // Sticky: nodes created after inherit liveness
    ତାର୍କିକ m_markStrongPending = ମିଥ୍ୟା;  // Mark new vertices as strong s_always in-window
    ତାର୍କିକ m_isCover = ମିଥ୍ୟା;  // Cover directives do not fail at end-of-simulation
    // IEEE 1800-2023 16.14.3 cover sequence: each end-of-match fires the action,
    // not just the first. Builder builds parallel-branch (no first-match-wins)
    // topology when true. Default false preserves cover_property semantics.
    ତାର୍କିକ m_isCoverSeq = ମିଥ୍ୟା;
    // Unsupported endpoint topology must reject, not ignore, or the wait hangs
    ତାର୍କିକ m_isSeqEvent = ମିଥ୍ୟା;
    ତାର୍କିକ m_inSequencePrefix = ମିଥ୍ୟା;  // Prefix composition does not preserve every endpoint
    ତାର୍କିକ m_wantBoolLeaf = ମିଥ୍ୟା;  // Building a |-> / |=> or top-level body (see buildOrMerge)

    ସଂରଚନା RangeDelayRejectInfo final {
        SvaStateVertex* startp = nullptr;
        unsigned range = 0;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା rhsLen = 0;
    };

    ଶୂନ୍ୟ warnEndpointUnsupported(File_ଧାଡ଼ି* flp, ଅଚଳ string& what) ଅଚଳ {
        ଯଦି (m_isSeqEvent) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: sequence used as an event control with " << what);
        } ଅନ୍ୟଥା {
            flp->v3warn(COVERIGN, "Ignoring unsupported: cover sequence with " << what);
        }
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* throughoutCond(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* baseCondp, File_ଧାଡ଼ି* flp) {
        ଯଦି (m_temporal_Guard_ଗଦା.empty()) ଫେରନ୍ତୁ baseCondp;
        // AND all active temporal guards (supports nesting)
        // Each must use $sampled values.
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* guardp = nullptr;
        ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp : m_temporal_Guard_ଗଦା) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ clonep = sampled(condp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
            ଯଦି (!guardp) {
                guardp = clonep;
            } ଅନ୍ୟଥା {
                guardp = ନୂତନ AstLogAnd{flp, guardp, clonep};
            }
        }
        ଯଦି (baseCondp) { guardp = ନୂତନ AstLogAnd{flp, baseCondp, guardp}; }
        ଫେରନ୍ତୁ guardp;
    }

    ସ୍ଥିର unsigned getConstUInt(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ constp = V3Const::constifyEdit(exprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
        ଅଚଳ AstConst* ଅଚଳ cp = VN_CAST(constp, Const);
        ଅଚଳ unsigned val = cp ? cp->toUInt() : 0;
        VL_DO_DANGLING(constp->delete_ବୃକ୍ଷ(), constp);
        ଫେରନ୍ତୁ val;
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
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା fixedLength(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଯଦି (AstSExpr* ଅଚଳ sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ଅଚଳ delayp = VN_CAST(sexprp->delayp(), Delay);
            ଯଦି (!delayp || !delayp->isCycleDelay()) ଫେରନ୍ତୁ -1;
            unsigned delayCycles;
            ଯଦି (delayp->isRangeDelay()) {
                ଯଦି (delayp->isUnbounded()) ଫେରନ୍ତୁ -1;  // LCOV_EXCL_LINE
                ଅଚଳ unsigned minD = getConstUInt(delayp->lhsp());
                ଅଚଳ unsigned maxD = getConstUInt(delayp->rhsp());
                ଯଦି (minD != maxD) ଫେରନ୍ତୁ -1;
                delayCycles = minD;
            } ଅନ୍ୟଥା {
                delayCycles = getConstUInt(delayp->lhsp());
            }
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା preLen = 0;
            ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ prep = sexprp->preExprp()) {
                preLen = fixedLength(prep);
                ଯଦି (preLen < 0) ଫେରନ୍ତୁ -1;  // LCOV_EXCL_LINE
            }
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା bodyLen = fixedLength(sexprp->exprp());
            ଯଦି (bodyLen < 0) ଫେରନ୍ତୁ -1;  // LCOV_EXCL_LINE
            ଫେରନ୍ତୁ preLen + delayCycles + bodyLen;
        }
        ଯଦି (AstSThroughout* ଅଚଳ throughp = VN_CAST(nodep, SThroughout)) {
            ଫେରନ୍ତୁ fixedLength(throughp->rhsp());
        }
        ଯଦି (AstSOr* ଅଚଳ orp = VN_CAST(nodep, SOr)) {
            // Alternatives must share one end cycle; buildSWithin relies on
            // this to pair the OR with an SIntersect.
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lhsLen = fixedLength(orp->lhsp());
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା rhsLen = fixedLength(orp->rhsp());
            ଯଦି (lhsLen < 0 || rhsLen < 0 || lhsLen != rhsLen) ଫେରନ୍ତୁ -1;
            ଫେରନ୍ତୁ lhsLen;
        }
        ଯଦି (AstSWithin* ଅଚଳ withinp = VN_CAST(nodep, SWithin)) {
            // `seq1 within seq2` ends at seq2's end cycle (IEEE 16.9.10).
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lhsLen = fixedLength(withinp->lhsp());
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା rhsLen = fixedLength(withinp->rhsp());
            ଯଦି (lhsLen < 0 || rhsLen < 0 || lhsLen > rhsLen) ଫେରନ୍ତୁ -1;
            ଫେରନ୍ତୁ rhsLen;
        }
        // LCOV_EXCL_START -- defensive: V3AssertPre rejects composite SVA ops
        // nested in an intersect arm before fixedLength runs (clock-context
        // resolution fails). Kept as a guard in case future parser relaxations
        // permit it.
        ଯଦି (nodep->isMultiCycleSva()) ଫେରନ୍ତୁ -1;
        // LCOV_EXCL_STOP
        // Plain boolean expression (no SVA constructs) -- 0 cycles.
        ଫେରନ୍ତୁ 0;
    }

    // Contiguous match-length range [lo,hi] for an operand whose length varies
    // from AT MOST ONE ranged cycle delay; {-1,-1} otherwise. Drives the
    // variable-length `intersect` lowering (IEEE 1800-2023 16.9.6): more than
    // one ranged delay would make the per-length realization ambiguous, so it
    // is reported unsupported rather than mis-paired.
    ସ୍ଥିର std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା> lengthRange(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଯଦି (AstSExpr* ଅଚଳ sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ଅଚଳ delayp = VN_CAST(sexprp->delayp(), Delay);
            ଯଦି (!delayp || !delayp->isCycleDelay()) ଫେରନ୍ତୁ {-1, -1};
            std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା> delayRange;
            ଯଦି (delayp->isRangeDelay()) {
                ଯଦି (delayp->isUnbounded()) ଫେରନ୍ତୁ {-1, -1};
                ଅଚଳ unsigned minD = getConstUInt(delayp->lhsp());
                ଅଚଳ unsigned maxD = getConstUInt(delayp->rhsp());
                delayRange = {minD, maxD};
            } ଅନ୍ୟଥା {
                ଅଚଳ unsigned d = getConstUInt(delayp->lhsp());
                delayRange = {d, d};
            }
            std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା> preRange{0, 0};
            ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ prep = sexprp->preExprp()) {
                preRange = lengthRange(prep);
                ଯଦି (preRange.first < 0) ଫେରନ୍ତୁ {-1, -1};
            }
            ଅଚଳ std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା> bodyRange = lengthRange(sexprp->exprp());
            ଯଦି (bodyRange.first < 0) ଫେରନ୍ତୁ {-1, -1};
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା variableParts = (preRange.first != preRange.second)
                                      + (delayRange.first != delayRange.second)
                                      + (bodyRange.first != bodyRange.second);
            ଯଦି (variableParts > 1) ଫେରନ୍ତୁ {-1, -1};
            ଫେରନ୍ତୁ {preRange.first + delayRange.first + bodyRange.first,
                    preRange.second + delayRange.second + bodyRange.second};
        }
        ଯଦି (AstSThroughout* ଅଚଳ throughp = VN_CAST(nodep, SThroughout)) {
            ଫେରନ୍ତୁ lengthRange(throughp->rhsp());
        }
        ଯଦି (nodep->isMultiCycleSva()) ଫେରନ୍ତୁ {-1, -1};
        ଫେରନ୍ତୁ {0, 0};  // plain boolean -- 0 cycles
    }

    // Clone `operand` with its sole variable ranged cycle delay pinned so the
    // total match length is exactly `len`. `lo` is the operand's minimum length
    // (lengthRange().first). A fixed operand has no such delay and is returned
    // as a plain clone (callers only request its single achievable length).
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* realizeAtLength(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* operand, ପୂର୍ଣ୍ଣସଂଖ୍ୟା len, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lo) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ clonep = operand->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        AstDelay* rangeDelayp = nullptr;
        clonep->foreach([&](AstDelay* dp) {
            ଯଦି (!rangeDelayp && dp->isRangeDelay() && !dp->isUnbounded()
                && getConstUInt(dp->lhsp()) != getConstUInt(dp->rhsp())) {
                rangeDelayp = dp;
            }
        });
        ଯଦି (rangeDelayp) {
            File_ଧାଡ଼ି* ଅଚଳ flp = rangeDelayp->fileline();
            ଅଚଳ unsigned pinned = getConstUInt(rangeDelayp->lhsp()) + (len - lo);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ oldMinp = rangeDelayp->lhsp();
            oldMinp->replaceWith(ନୂତନ AstConst{flp, pinned});
            VL_DO_DANGLING(oldMinp->delete_ବୃକ୍ଷ(), oldMinp);
            // Drop the max bound so it lowers as a fixed `##d`, not `##[d:d]`.
            Ast_ଗଣ୍ଠି* ଅଚଳ oldMaxp = rangeDelayp->rhsp()->unlinkFrBack();
            VL_DO_DANGLING(oldMaxp->delete_ବୃକ୍ଷ(), oldMaxp);
        }
        ଫେରନ୍ତୁ clonep;
    }

    // Cuts AST size from O(N * sizeof(exprp)) to O(N) + O(sizeof(exprp)) by
    // sharing a single `VarRef` across N check edges. Hoist also matches the
    // IEEE 1800-2023 16.9.9 "single preponed-region snapshot" semantic for
    // any exprp -- even an impure one would now evaluate exactly once per
    // clock instead of N times. Orphan temps from failed builds are unused
    // MODULETEMPs and are removed by V3Dead.
    AstVar* tryHoistSampled(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, File_ଧାଡ଼ି* flp, unsigned clone_ଗଣନା) {
        constexpr unsigned kHoistThreshold = 2;
        ଯଦି (clone_ଗଣନା < kHoistThreshold) ଫେରନ୍ତୁ nullptr;
        AstVar* ଅଚଳ tempVarp
            = ନୂତନ AstVar{flp, VVarType::MODULETEMP, m_propTempNames.get(exprp), exprp->dtypep()};
        m_modp->addStmtsp(tempVarp);
        AstAssign* ଅଚଳ assignp = ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, tempVarp, VAccess::WRITE},
                                                 sampled(exprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା))};
        m_modp->addStmtsp(ନୂତନ AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, assignp});
        ଫେରନ୍ତୁ tempVarp;
    }

    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* sampledRefOrClone(AstVar* hoistVarp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, File_ଧାଡ଼ି* flp) {
        ଯଦି (hoistVarp) ଫେରନ୍ତୁ ନୂତନ AstVarRef{flp, hoistVarp, VAccess::READ};
        ଫେରନ୍ତୁ sampled(exprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
    }

    // Create vertex and inherit temporal guards from the current scope.
    SvaStateVertex* scopedCreateVertex() {
        SvaStateVertex* ଅଚଳ vtxp = m_graph.createStateVertex();
        ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ cp : m_temporal_Guard_ଗଦା) {
            vtxp->m_throughoutConds.ଠେଲ_back(cp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
        }
        ଯଦି (m_inUnboundedScope) vtxp->m_isUnbounded = ସତ୍ୟ;
        ଯଦି (m_markStrongPending) vtxp->m_strongPending = ସତ୍ୟ;
        ଫେରନ୍ତୁ vtxp;
    }

    // AND current temporal guards into every edge/link.
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* ଶୀର୍ଷ, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp,
                              File_ଧାଡ଼ି* flp) {
        ଫେରନ୍ତୁ m_graph.addLink(fromp, ଶୀର୍ଷ, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* ଶୀର୍ଷ, File_ଧାଡ଼ି* flp) {
        ଫେରନ୍ତୁ m_graph.addLink(fromp, ଶୀର୍ଷ, throughoutCond(nullptr, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* ଶୀର୍ଷ, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp,
                              File_ଧାଡ଼ି* flp) {
        ଫେରନ୍ତୁ m_graph.addClockedEdge(fromp, ଶୀର୍ଷ, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* ଶୀର୍ଷ, File_ଧାଡ଼ି* flp) {
        ଫେରନ୍ତୁ m_graph.addClockedEdge(fromp, ଶୀର୍ଷ, throughoutCond(nullptr, flp));
    }

    SvaStateVertex* addDelayChain(SvaStateVertex* startp, unsigned size, File_ଧାଡ଼ି* flp,
                                  ତାର୍କିକ isFixed = ସତ୍ୟ, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* clearCondp = nullptr,
                                  Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* advanceCondp = nullptr) {
        ଯଦି (isFixed && size == 0) ଫେରନ୍ତୁ startp;
        UASSERT_OBJ(size > 0, startp, "Delay chain needs at least one slot");
        ଯଦି (isFixed && size == 1 && !advanceCondp) {
            SvaStateVertex* ଅଚଳ nextp = scopedCreateVertex();
            guardedEdge(startp, nextp, flp);
            ଫେରନ୍ତୁ nextp;
        }
        SvaStateVertex* ଅଚଳ ringVtxp = scopedCreateVertex();
        ringVtxp->m_isFixedDelayRing = isFixed;
        ringVtxp->m_delayRingSize = size;
        ଯଦି (clearCondp) {
            UASSERT_OBJ(!isFixed, startp, "Fixed delay cannot have a clear condition");
            ringVtxp->m_delayRingClearCondp = clearCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        }
        ringVtxp->m_delayRingAdvanceCondp = advanceCondp;
        ଯଦି (isFixed) {
            guardedEdge(startp, ringVtxp, flp);
        } ଅନ୍ୟଥା {
            guardedLink(startp, ringVtxp, flp);
        }
        ଫେରନ୍ତୁ ringVtxp;
    }

    // Build NFA for an SExpr. finalCond = RHS (not yet added as a vertex).
    // isTopLevelStep: marks outermost required boolean check as rejectOnFail.
    // Apply a range delay `##[M:N]` to currentp. Returns true on success. On
    // failure, emits a diagnostic and returns false.
    ତାର୍କିକ applyRangeDelay(AstSExpr* sexprp, SvaStateVertex*& currentp,
                         std::vector<SvaStateVertex*>& midSources,
                         RangeDelayRejectInfo* rangeRejectInfop = nullptr) {
        File_ଧାଡ଼ି* ଅଚଳ flp = sexprp->fileline();
        AstDelay* ଅଚଳ delayp = VN_AS(sexprp->delayp(), Delay);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsExprp = sexprp->exprp();
        ଅଚଳ unsigned minDelay = getConstUInt(delayp->lhsp());
        ଯଦି (delayp->isUnbounded()) {
            // `##[M:$]`: wait M cycles, then retain every matured attempt in a
            // one-slot ring while also exposing the first eligible tick.
            currentp = addDelayChain(currentp, minDelay, flp);
            SvaStateVertex* ଅଚଳ waitRingp = addDelayChain(currentp, 1, flp, ମିଥ୍ୟା);
            // The ring's live count is updated in NBA, so this link retains the previous
            // tick's live attempts alongside new arrivals for the next tick.
            guardedLink(waitRingp, waitRingp, flp);
            SvaStateVertex* ଅଚଳ mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(waitRingp, mergeVtxp, flp);
            currentp = mergeVtxp;
            currentp->m_isUnbounded = ସତ୍ୟ;  // Liveness, so no reject.
            m_inUnboundedScope = ସତ୍ୟ;
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଅଚଳ unsigned maxDelay = getConstUInt(delayp->rhsp());
        ଯଦି (minDelay == maxDelay) {
            currentp = addDelayChain(currentp, minDelay, flp);
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଅଚଳ unsigned range = maxDelay - minDelay;
        currentp = addDelayChain(currentp, minDelay, flp);
        ଅଚଳ ତାର୍କିକ multiCycleRhs = rhsExprp->isMultiCycleSva();
        ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ preExprp = sexprp->preExprp();
        // Prefix and nested-sequence composition can lose endpoints or their multiplicity
        // required by cover sequence (IEEE 1800-2023 16.14.3), regardless of the range width.
        ଯଦି (m_isCoverSeq
            && (multiCycleRhs || m_inSequencePrefix
                || (preExprp && preExprp->isMultiCycleSva()))) {
            warnEndpointUnsupported(flp, "this ranged cycle delay");
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଯଦି (m_inSequencePrefix) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: Bounded ranged cycle delay in a sequence prefix");
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଯଦି (multiCycleRhs) {
            // Launch a candidate at every eligible tick. Candidate-local misses are not
            // assertion rejects while a later position can still match.
            ଯଦି (rangeRejectInfop) {
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା rhsLen = fixedLength(rhsExprp);
                ଯଦି (rhsLen >= 0) *rangeRejectInfop = {currentp, range, rhsLen};
            }
            SvaStateVertex* ଅଚଳ tailp = addDelayChain(currentp, range + 1U, flp, ମିଥ୍ୟା);
            SvaStateVertex* ଅଚଳ mergeVtxp = scopedCreateVertex();
            mergeVtxp->m_isUnbounded = ସତ୍ୟ;
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(tailp, mergeVtxp, flp);
            currentp = mergeVtxp;
            m_inUnboundedScope = ସତ୍ୟ;
        } ଅନ୍ୟଥା {
            // The first eligible tick is explicit. The ring retains attempts for the
            // remaining ticks and exposes the final tick for rejection. Only properties
            // clear on success, since cover sequence counts every endpoint.
            midSources.ଠେଲ_back(currentp);
            currentp = addDelayChain(currentp, range + 1U, flp, ମିଥ୍ୟା,
                                     m_isCoverSeq ? nullptr : rhsExprp);
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ଶୂନ୍ୟ addFiniteRangeReject(ଅଚଳ RangeDelayRejectInfo& info, ଅଚଳ BuildResult& result,
                              File_ଧାଡ଼ି* flp) {
        ଯଦି (!info.startp) ଫେରନ୍ତୁ;

        SvaStateVertex* ଅଚଳ expiryVtxp
            = addDelayChain(info.startp, info.range + info.rhsLen, flp);
        SvaStateVertex* ଅଚଳ expiryMatchp = scopedCreateVertex();
        std::vector<SvaStateVertex*> sources = result.midSources;
        sources.ଠେଲ_back(result.termVertexp);
        ପାଇଁ (SvaStateVertex* ଅଚଳ srcp : sources) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp
                = result.finalCondp ? sampled(result.finalCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)) : nullptr;
            SvaStateVertex* ଅଚଳ successNowp = scopedCreateVertex();
            guardedLink(srcp, successNowp, condp, flp);
            // Retain a success through the latest deadline it can satisfy.
            SvaStateVertex* ଅଚଳ historyp
                = addDelayChain(successNowp, info.range + 1U, flp, ମିଥ୍ୟା);
            guardedLink(successNowp, expiryMatchp, flp);
            guardedLink(historyp, expiryMatchp, flp);
        }

        SvaStateVertex* ଅଚଳ sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = ସତ୍ୟ;
        SvaTransEdge* ଅଚଳ rejectp = m_graph.addLink(expiryVtxp, sinkVtxp);
        rejectp->m_rejectOnFail = ସତ୍ୟ;
        rejectp->m_condVtxp = expiryMatchp;
    }

    BuildResult buildSExpr(AstSExpr* sexprp, SvaStateVertex* entryVtxp,
                           ତାର୍କିକ is_ଶୀର୍ଷ_Level_Step = ମିଥ୍ୟା) {
        AstDelay* ଅଚଳ delayp = VN_CAST(sexprp->delayp(), Delay);
        ଯଦି (!delayp || !delayp->isCycleDelay()) ଫେରନ୍ତୁ BuildResult::fail();

        File_ଧାଡ଼ି* ଅଚଳ flp = sexprp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = sexprp->exprp();

        // Handle LHS (preExpr)
        SvaStateVertex* currentp = entryVtxp;
        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ preExprp = sexprp->preExprp()) {
            VL_RESTORER(m_inSequencePrefix);
            m_inSequencePrefix = ସତ୍ୟ;
            ଅଚଳ BuildResult pre = build_ପଦସମଷ୍ଟି(preExprp, currentp, is_ଶୀର୍ଷ_Level_Step);
            ଯଦି (!pre.valid()) ଫେରନ୍ତୁ BuildResult::fail(pre.errorEmitted);  // LCOV_EXCL_LINE
            ଯଦି (pre.finalCondp) {
                SvaStateVertex* ଅଚଳ condVtxp = scopedCreateVertex();
                SvaTransEdge* ଅଚଳ edgep = guardedLink(
                    pre.termVertexp, condVtxp, sampled(pre.finalCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)), flp);
                ଯଦି (is_ଶୀର୍ଷ_Level_Step && !pre.termVertexp->m_isUnbounded && !m_inUnboundedScope) {
                    // Do not mark liveness sources: first boolean check is deferred.
                    edgep->m_rejectOnFail = ସତ୍ୟ;
                }
                freeUnlinkedCondp(pre.finalCondp);
                currentp = condVtxp;
            } ଅନ୍ୟଥା {
                currentp = pre.termVertexp;
            }
        }

        // Handle delay
        std::vector<SvaStateVertex*> rangeMidSources;
        RangeDelayRejectInfo rangeRejectInfo;
        ଅଚଳ ତାର୍କିକ addRangeReject = is_ଶୀର୍ଷ_Level_Step && !m_inUnboundedScope;
        ଯଦି (delayp->isRangeDelay()) {
            ଯଦି (!applyRangeDelay(sexprp, currentp, rangeMidSources,
                                 addRangeReject ? &rangeRejectInfo : nullptr)) {
                ଫେରନ୍ତୁ BuildResult::failWithError();
            }
        } ଅନ୍ୟଥା {
            ଅଚଳ unsigned delayCycles = getConstUInt(delayp->lhsp());
            currentp = addDelayChain(currentp, delayCycles, flp);
        }

        // Multi-cycle RHS: recurse (only plain boolean is returned as finalCondp).
        ଯଦି (exprp->isMultiCycleSva()) {
            ଅଚଳ BuildResult result = build_ପଦସମଷ୍ଟି(exprp, currentp, is_ଶୀର୍ଷ_Level_Step);
            ଯଦି (result.valid()) addFiniteRangeReject(rangeRejectInfo, result, flp);
            ଫେରନ୍ତୁ result;
        }
        ଫେରନ୍ତୁ {currentp, exprp, std::move(rangeMidSources)};
    }

    BuildResult buildConsRep(AstSConsRep* repp, SvaStateVertex* entryVtxp,
                             ତାର୍କିକ is_ଶୀର୍ଷ_Level_Step = ମିଥ୍ୟା) {
        File_ଧାଡ଼ି* ଅଚଳ flp = repp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = repp->exprp();
        // Multi-cycle expr in ConsRep not yet supported; bail to avoid invalid AST.
        ଯଦି (exprp->isMultiCycleSva()) {
            repp->v3warn(E_UNSUPPORTED, "Unsupported: multi-cycle sequence expression inside"
                                        " consecutive repetition (IEEE 1800-2023 16.9.2)");
            ଫେରନ୍ତୁ BuildResult::failWithError();
        }
        ଅଚଳ unsigned minN = getConstUInt(repp->countp());

        // Sum sites across prefix + unbounded/range tail so one hoist covers
        // every check edge of this repetition.
        unsigned totalSites = minN;
        ଯଦି (repp->unbounded()) {
            totalSites += 1;
        } ଅନ୍ୟଥା ଯଦି (repp->maxCountp()) {
            totalSites += getConstUInt(repp->maxCountp()) - minN;
        }
        AstVar* ଅଚଳ hoistVarp = tryHoistSampled(exprp, flp, totalSites);

        // Cover-sequence (IEEE 1800-2023 16.14.3): collect each end-of-match
        // position so they all fire the action, not just the merged terminal.
        std::vector<SvaStateVertex*> consMidSources;

        SvaStateVertex* currentp = entryVtxp;
        ପାଇଁ (unsigned i = 0; i < minN; ++i) {
            // Keep the first repetition explicit, collapse all remaining checks into the ring.
            ଯଦି (i == 1) {
                currentp = addDelayChain(currentp, minN - 1, flp);
                currentp->m_delayRingClearCondp
                    = ନୂତନ AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                currentp->m_replayAbortReject = ସତ୍ୟ;
                ଯଦି (is_ଶୀର୍ଷ_Level_Step) {
                    currentp->m_throughoutConds.ଠେଲ_back(
                        sampledRefOrClone(hoistVarp, exprp, flp));
                }
                i = minN - 1;
            }
            // Every repetition in the minimum prefix is required.
            SvaStateVertex* ଅଚଳ condVtxp = scopedCreateVertex();
            SvaTransEdge* ଅଚଳ linkp
                = guardedLink(currentp, condVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            // Only an outermost required step rejects the property. Its first check is explicit,
            // later required checks reject through the ring's throughout guard.
            ଯଦି (is_ଶୀର୍ଷ_Level_Step && i == 0) linkp->m_rejectOnFail = ସତ୍ୟ;
            currentp = condVtxp;
        }
        // After minN: currentp is the first valid end-of-match position for [*m:n].
        ଯଦି (m_isCoverSeq && (repp->unbounded() || repp->maxCountp())) {
            consMidSources.ଠେଲ_back(currentp);
        }

        ଯଦି (repp->unbounded()) {
            ଯଦି (minN == 0) {
                SvaStateVertex* ଅଚଳ waitVtxp = scopedCreateVertex();
                guardedEdge(currentp, waitVtxp, flp);
                SvaStateVertex* ଅଚଳ checkVtxp = scopedCreateVertex();
                guardedLink(waitVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedEdge(checkVtxp, waitVtxp, flp);
                guardedLink(currentp, checkVtxp, flp);
                currentp = checkVtxp;
            } ଅନ୍ୟଥା {
                SvaStateVertex* ଅଚଳ loopBackVtxp = scopedCreateVertex();
                guardedEdge(currentp, loopBackVtxp, flp);
                SvaStateVertex* ଅଚଳ reCheckVtxp = scopedCreateVertex();
                guardedLink(loopBackVtxp, reCheckVtxp, sampledRefOrClone(hoistVarp, exprp, flp),
                            flp);
                guardedEdge(reCheckVtxp, loopBackVtxp, flp);
                guardedLink(reCheckVtxp, currentp, flp);
            }
            currentp->m_isUnbounded = ସତ୍ୟ;
            m_inUnboundedScope = ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (repp->maxCountp()) {
            ଅଚଳ unsigned maxN = getConstUInt(repp->maxCountp());
            SvaStateVertex* ଅଚଳ mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            unsigned tailMinN = minN;
            SvaStateVertex* tailStartp = currentp;
            ଯଦି (minN == 0) {
                // Build the first optional iteration explicitly. Feeding the empty endpoint
                // directly into a range ring would incorrectly keep that match alive.
                SvaStateVertex* ଅଚଳ nextVtxp = scopedCreateVertex();
                guardedEdge(currentp, nextVtxp, flp);
                SvaStateVertex* ଅଚଳ checkVtxp = scopedCreateVertex();
                guardedLink(nextVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                ଯଦି (m_isCoverSeq) consMidSources.ଠେଲ_back(checkVtxp);
                tailStartp = checkVtxp;
                tailMinN = 1;
            }
            ଯଦି (maxN > tailMinN) {
                // Add tail-ring only if the tail is non-empty.
                SvaStateVertex* ଅଚଳ nextVtxp
                    = addDelayChain(tailStartp, maxN - tailMinN + 1, flp, ମିଥ୍ୟା);
                nextVtxp->m_delayRingClearCondp
                    = ନୂତନ AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                nextVtxp->m_replayAbortReject = ସତ୍ୟ;
                SvaStateVertex* ଅଚଳ checkVtxp = scopedCreateVertex();
                guardedLink(nextVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                ଯଦି (m_isCoverSeq) consMidSources.ଠେଲ_back(checkVtxp);
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
        ଫେରନ୍ତୁ res;
    }

    // always[lo:hi] / s_always[lo:hi] (IEEE 1800-2023 16.12.11).
    BuildResult buildPropAlways(AstPropAlways* nodep, SvaStateVertex* entryVtxp,
                                ତାର୍କିକ is_ଶୀର୍ଷ_Level_Step = ମିଥ୍ୟା) {
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ propp = nodep->propp();
        ଅଚଳ unsigned lo = getConstUInt(nodep->loBoundp());
        ଯଦି (VN_IS(nodep->hiBoundp(), Unbounded)) {
            // Weak always [lo:$]: unbounded upper bound (IEEE 1800-2023 16.12.11).
            // p must hold at every clock tick at least lo cycles after the attempt
            // start; those ticks are not required to exist, so there is no
            // end-of-trace obligation (weak). The self-loop keeps the attempt live
            // every cycle; each observed cycle is a safety obligation, so a false p
            // rejects immediately.
            UASSERT_OBJ(!nodep->isStrong(), nodep, "Unbounded always must be weak (V3Width)");
            SvaStateVertex* ଅଚଳ livep = addDelayChain(entryVtxp, lo, flp);
            livep->m_isUnbounded = ସତ୍ୟ;
            guardedEdge(livep, livep, flp);  // stay active every subsequent cycle
            SvaStateVertex* ଅଚଳ sinkp = m_graph.createStateVertex();
            sinkp->m_isRejectSink = ସତ୍ୟ;
            SvaTransEdge* ଅଚଳ rejEdgep
                = guardedLink(livep, sinkp, sampled(propp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)), flp);
            ଯଦି (is_ଶୀର୍ଷ_Level_Step) rejEdgep->m_rejectOnFail = ସତ୍ୟ;
            ଫେରନ୍ତୁ {livep, nullptr, {}};
        }
        ଅଚଳ unsigned hi = getConstUInt(nodep->hiBoundp());
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
        SvaStateVertex* ଅଚଳ checkp = scopedCreateVertex();
        SvaTransEdge* ଅଚଳ linkp
            = guardedLink(currentp, checkp, sampled(propp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)), flp);
        ଯଦି (is_ଶୀର୍ଷ_Level_Step) linkp->m_rejectOnFail = ସତ୍ୟ;
        currentp = checkp;
        m_temporal_Guard_ଗଦା.ଠେଲ_back(propp);
        currentp = addDelayChain(currentp, hi - lo, flp);
        ଯଦି (nodep->isStrong() && currentp->m_delayRingSize) currentp->m_strongAlwaysRing = ସତ୍ୟ;
        m_temporal_Guard_ଗଦା.କାଢ଼_back();
        ଫେରନ୍ତୁ {currentp, propp, {}};
    }

    BuildResult buildGotoRep(AstSGotoRep* repp, SvaStateVertex* entryVtxp) {
        File_ଧାଡ଼ି* ଅଚଳ flp = repp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = repp->exprp();
        ଅଚଳ unsigned minN = getConstUInt(repp->countp());
        ଯଦି (minN == 0) ଫେରନ୍ତୁ BuildResult::fail();
        ଅଚଳ ତାର୍କିକ hasMax = repp->maxCountp() != nullptr;
        ଅଚଳ unsigned maxN = hasMax ? getConstUInt(repp->maxCountp()) : minN;
        ଯଦି (m_isCoverSeq) {
            // Several attempts may coalesce in the one-bit wait vertex before reaching the ring,
            // so a cover sequence action block could run too few times.
            warnEndpointUnsupported(flp, "a goto repetition");
            ଫେରନ୍ତୁ BuildResult::failWithError();
        }

        AstVar* ଅଚଳ hoistVarp = tryHoistSampled(exprp, flp, 2);

        // The first guardedEdge is the ##1 before waiting for a match. In the wait state, false
        // takes the clocked self-loop, while true takes the zero-delay guardedLink on that tick.
        SvaStateVertex* ଅଚଳ waitVtxp = scopedCreateVertex();
        guardedEdge(entryVtxp, waitVtxp, flp);
        guardedEdge(waitVtxp, waitVtxp,
                    ନୂତନ AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)}, flp);
        SvaStateVertex* currentp = scopedCreateVertex();
        guardedLink(waitVtxp, currentp, sampledRefOrClone(hoistVarp, exprp, flp), flp);

        ଯଦି (minN > 1) {
            SvaStateVertex* ଅଚଳ ringVtxp = addDelayChain(
                currentp, minN - 1, flp, ସତ୍ୟ, nullptr, sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = ସତ୍ୟ;
            SvaStateVertex* ଅଚଳ checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            currentp = checkVtxp;
        }
        ଯଦି (!hasMax) {
            currentp->m_isUnbounded = ସତ୍ୟ;  // [->N] waits unboundedly
            m_inUnboundedScope = ସତ୍ୟ;
            ଫେରନ୍ତୁ {currentp, nullptr, {}};
        }

        // [->M:N]: the range ring holds matches from M through N and advances
        // only on expr, preserving arbitrarily long gaps between occurrences.
        SvaStateVertex* ଅଚଳ mergeVtxp = scopedCreateVertex();
        guardedLink(currentp, mergeVtxp, flp);  // accept at match_M
        ଯଦି (maxN > minN) {
            SvaStateVertex* ଅଚଳ ringVtxp
                = addDelayChain(currentp, maxN - minN + 1, flp, ମିଥ୍ୟା, nullptr,
                                sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = ସତ୍ୟ;
            SvaStateVertex* ଅଚଳ checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            guardedLink(checkVtxp, mergeVtxp, flp);
        }
        mergeVtxp->m_isUnbounded = ସତ୍ୟ;  // [->M:N] still has unbounded waits between matches
        m_inUnboundedScope = ସତ୍ୟ;
        ଫେରନ୍ତୁ {mergeVtxp, nullptr, {}};
    }

    // True if the sequence has a zero-minimum consecutive repetition, which may match empty
    ସ୍ଥିର ତାର୍କିକ hasZeroMinRep(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* seqp) {
        ଫେରନ୍ତୁ seqp->exists(
            [](ଅଚଳ AstSConsRep* repp) { ଫେରନ୍ତୁ getConstUInt(repp->countp()) == 0; });
    }

    // True if a leaf's condition has no implication or multi-cycle sequence in it
    // (a leaf can be e.g. 'not (x |-> y)')
    ସ୍ଥିର ତାର୍କିକ isPlainBoolean(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp) {
        ଫେରନ୍ତୁ !condp->exists(
            [](ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* np) { ଫେରନ୍ତୁ np->isMultiCycleSva() || VN_IS(np, Implication); });
    }

    // Build merge vertex for SOr / LogOr: both branches feed into one vertex.
    BuildResult buildOrMerge(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, SvaStateVertex* entryVtxp,
                             File_ଧାଡ଼ି* flp, ତାର୍କିକ wantBoolLeaf) {
        ଅଚଳ BuildResult lhs = build_ପଦସମଷ୍ଟି(lhsp, entryVtxp);
        ଅଚଳ BuildResult rhs = build_ପଦସମଷ୍ଟି(rhsp, entryVtxp);
        ଯଦି (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ଫେରନ୍ତୁ BuildResult::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP
        // IEEE 1800-2023 16.14.3: a cover sequence counts every end-of-match. A
        // sequence operand of 'or' can end more than once, but only its final
        // end reaches the merge vertex below, so reject sequence operands rather
        // than under-count. Plain boolean disjunction has one end per cycle and
        // is handled by the OR-fold.
        ଅଚଳ ତାର୍କିକ booleanOnly = lhs.termVertexp == entryVtxp && lhs.finalCondp
                                 && rhs.termVertexp == entryVtxp && rhs.finalCondp;
        ଯଦି (m_isCoverSeq && !booleanOnly) {
            warnEndpointUnsupported(flp, "a sequence operand of 'or'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ଫେରନ୍ତୁ BuildResult::failWithError();
        }
        ଯଦି (booleanOnly && wantBoolLeaf) {
            ଯଦି (isPlainBoolean(lhs.finalCondp) && isPlainBoolean(rhs.finalCondp)) {
                // As a |-> / |=> or top-level body, a disjunction of two plain booleans must
                // be able to reject: return it as a boolean finalCondp. The merge vertex
                // below drops the attempt silently when both operands are false.
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ orCondp
                    = ନୂତନ AstLogOr{flp, lhs.finalCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                                   rhs.finalCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                freeUnlinkedCondp(lhs.finalCondp);
                freeUnlinkedCondp(rhs.finalCondp);
                ଫେରନ୍ତୁ {entryVtxp, orCondp, {}};
            }
        }
        SvaStateVertex* ଅଚଳ mergeVtxp = scopedCreateVertex();
        ଯଦି (booleanOnly) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsp,
                        "Single-cycle SOr operands must have finalCondp");
            guardedLink(entryVtxp, mergeVtxp,
                        ନୂତନ AstLogOr{flp, sampled(lhs.finalCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)),
                                     sampled(rhs.finalCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା))},
                        flp);
        } ଅନ୍ୟଥା {
            ଯଦି (lhs.finalCondp) {
                guardedLink(lhs.termVertexp, mergeVtxp,
                            sampled(lhs.finalCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)), flp);
            } ଅନ୍ୟଥା {
                guardedLink(lhs.termVertexp, mergeVtxp, flp);
            }
            ଯଦି (rhs.finalCondp) {
                guardedLink(rhs.termVertexp, mergeVtxp,
                            sampled(rhs.finalCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)), flp);
            } ଅନ୍ୟଥା {
                guardedLink(rhs.termVertexp, mergeVtxp, flp);
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        ଫେରନ୍ତୁ {mergeVtxp, nullptr, {}};
    }

    // Build done-latch combiner for SAnd/SIntersect (IEEE 1800-2023 16.9.5).
    BuildResult buildAndCombiner(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsExprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsExprp,
                                 SvaStateVertex* entryVtxp, File_ଧାଡ଼ି* flp) {
        // Snapshot-restore scope so LHS liveness does not leak into RHS.
        ଅଚଳ ତାର୍କିକ savedScope = m_inUnboundedScope;
        ଅଚଳ BuildResult lhs = build_ପଦସମଷ୍ଟି(lhsExprp, entryVtxp);
        ଅଚଳ ତାର୍କିକ lhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope;
        ଅଚଳ BuildResult rhs = build_ପଦସମଷ୍ଟି(rhsExprp, entryVtxp);
        ଅଚଳ ତାର୍କିକ rhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope || lhsScope || rhsScope;
        ଯଦି (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ଫେରନ୍ତୁ BuildResult::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP

        // Single-cycle operands: use boolean AND (done-latch would fire across cycles).
        // If both operands stayed at entry, they must be boolean leaves which
        // buildExpr returns with finalCondp=nodep (non-null).
        ଯଦି (lhs.termVertexp == entryVtxp && rhs.termVertexp == entryVtxp) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsExprp,
                        "Single-cycle SAnd operands must have finalCondp");
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = ନୂତନ AstLogAnd{flp, lhs.finalCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                                                     rhs.finalCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ଫେରନ୍ତୁ {entryVtxp, condp, {}};
        }
        // Range-delay mid-window sources in either sub-branch would need
        // to be folded into the latch's match-now signal, which the
        // current combiner does not support.
        ଯଦି (!lhs.midSources.empty() || !rhs.midSources.empty()) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: ranged cycle delay in an operand of property 'and'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            ଫେରନ୍ତୁ BuildResult::failWithError();
        }
        SvaStateVertex* ଅଚଳ combVtxp = scopedCreateVertex();
        combVtxp->m_isAndCombiner = ସତ୍ୟ;
        combVtxp->m_andLhsTermp = lhs.termVertexp;
        combVtxp->m_andRhsTermp = rhs.termVertexp;
        ଯଦି (lhs.finalCondp) combVtxp->m_andLhsCondp = lhs.finalCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        ଯଦି (rhs.finalCondp) combVtxp->m_andRhsCondp = rhs.finalCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        ଯଦି (lhs.termVertexp->m_isUnbounded || rhs.termVertexp->m_isUnbounded) {
            combVtxp->m_isUnbounded = ସତ୍ୟ;
        }
        // Wire terminal-boolean rejects to a dedicated sink so each side can fail
        // the AND independently. Skip for liveness or single-cycle operands
        // (single-cycle termVertexp == entryVtxp would fire every cycle).
        ଯଦି (!combVtxp->m_isUnbounded) {
            ତାର୍କିକ needSink = ମିଥ୍ୟା;
            ଅଚଳ ତାର୍କିକ lhsMultiCycle = (lhs.termVertexp != entryVtxp);
            ଅଚଳ ତାର୍କିକ rhsMultiCycle = (rhs.termVertexp != entryVtxp);
            ଯଦି (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                needSink = ସତ୍ୟ;
            }
            ଯଦି (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                needSink = ସତ୍ୟ;
            }
            ଯଦି (needSink) {
                SvaStateVertex* ଅଚଳ sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = ସତ୍ୟ;
                ଯଦି (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* ଅଚଳ ep = m_graph.addLink(
                        lhs.termVertexp, sinkVtxp, sampled(lhs.finalCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)));
                    ep->m_rejectOnFail = ସତ୍ୟ;
                }
                ଯଦି (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* ଅଚଳ ep = m_graph.addLink(
                        rhs.termVertexp, sinkVtxp, sampled(rhs.finalCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)));
                    ep->m_rejectOnFail = ସତ୍ୟ;
                }
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        ଫେରନ୍ତୁ {combVtxp, nullptr, {}};
    }

    // Lower `seq1 within seq2` (IEEE 1800-2023 16.9.10) as:
    //   (OR_{i in 0..slack} 1 ##i seq1 ##(slack-i) 1) intersect seq2
    // Both operands must have fixed length (no ranged cycle delays).
    // The OR lives inside a single SIntersect so one AndCombiner done-latch
    // serves all offsets; lifting it out would double-count matches where
    // multiple offsets accept on the same seq2 end cycle.
    BuildResult buildSWithin(AstSWithin* nodep, SvaStateVertex* entryVtxp,
                             ତାର୍କିକ is_ଶୀର୍ଷ_Level_Step = ମିଥ୍ୟା) {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା innerLen = fixedLength(nodep->lhsp());
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା outerLen = fixedLength(nodep->rhsp());
        ଯଦି (innerLen < 0 || outerLen < 0) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: within with ranged cycle-delay operand");
            ଫେରନ୍ତୁ BuildResult::failWithError();
        }
        ଯଦି (innerLen > outerLen) {
            ଫେରନ୍ତୁ buildNeverMatchIntersect(
                nodep, entryVtxp, is_ଶୀର୍ଷ_Level_Step,
                "the inner sequence is longer than the outer sequence");
        }
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା slack = outerLen - innerLen;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* innerOrp = nullptr;
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i <= slack; ++i) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା postPad = slack - i;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* branchp = nodep->lhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            ଯଦି (i > 0) {
                AstConst* ଅଚଳ prePadp = ନୂତନ AstConst{flp, AstConst::BitTrue{}};
                AstDelay* ଅଚଳ delayp
                    = ନୂତନ AstDelay{flp, ନୂତନ AstConst{flp, static_cast<uint32_t>(i)}, ସତ୍ୟ};
                AstSExpr* ଅଚଳ wrapped = ନୂତନ AstSExpr{flp, prePadp, delayp, branchp};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            ଯଦି (postPad > 0) {
                AstConst* ଅଚଳ postTruep = ନୂତନ AstConst{flp, AstConst::BitTrue{}};
                AstDelay* ଅଚଳ delayp
                    = ନୂତନ AstDelay{flp, ନୂତନ AstConst{flp, static_cast<uint32_t>(postPad)}, ସତ୍ୟ};
                AstSExpr* ଅଚଳ wrapped = ନୂତନ AstSExpr{flp, branchp, delayp, postTruep};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            innerOrp = innerOrp ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstSOr{flp, innerOrp, branchp})
                                : branchp;
        }
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ outerClonep = nodep->rhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ combinedp = ନୂତନ AstSIntersect{flp, innerOrp, outerClonep};
        BuildResult result = build_ପଦସମଷ୍ଟି(combinedp, entryVtxp, is_ଶୀର୍ଷ_Level_Step);
        VL_DO_DANGLING(combinedp->delete_ବୃକ୍ଷ(), combinedp);
        // When both operands are plain booleans, buildAndCombiner returns a
        // freshly-allocated AstAnd as finalCondp with no parent. Callers up
        // the chain clone-and-discard finalCondp, so leave it parent-less and
        // it leaks; anchor it in the graph now via an edge.
        ଯଦି (result.valid() && result.finalCondp && !result.finalCondp->backp()) {
            SvaStateVertex* ଅଚଳ wrapVtxp = scopedCreateVertex();
            guardedLink(result.termVertexp, wrapVtxp, sampled(result.finalCondp), flp);
            result = {wrapVtxp, nullptr, result.midSources};
        }
        ଫେରନ୍ତୁ result;
    }

    // Collect the boolean leaf checks of a fixed-length sequence keyed by their
    // clock offset from the start. Returns false for anything other than nested
    // AstSExpr with fixed cycle delays over boolean leaves (e.g. throughout).
    ସ୍ଥିର ତାର୍କିକ flattenFixedSeq(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା baseOffset,
                                std::map<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>>& out) {
        ଯଦି (AstSExpr* ଅଚଳ sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ଅଚଳ delayp = VN_CAST(sexprp->delayp(), Delay);
            ଯଦି (!delayp || !delayp->isCycleDelay() || delayp->isUnbounded()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଅଚଳ unsigned delayCycles = getConstUInt(delayp->lhsp());
            ଯଦି (delayp->isRangeDelay() && getConstUInt(delayp->rhsp()) != delayCycles)
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା preLen = 0;
            ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ prep = sexprp->preExprp()) {
                ଯଦି (!flattenFixedSeq(prep, baseOffset, out)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                preLen = fixedLength(prep);
                ଯଦି (preLen < 0) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            ଫେରନ୍ତୁ flattenFixedSeq(sexprp->exprp(), baseOffset + preLen + delayCycles, out);
        }
        ଯଦି (nodep->isMultiCycleSva()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        out[baseOffset].ଠେଲ_back(nodep);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // Conjoin two equal-length fixed sequences into one: at each clock offset
    // AND the boolean checks of both operands (IEEE 1800-2023 16.9.6 -- both
    // operands match the same window). Returns null if either operand is not a
    // plain fixed sequence of boolean leaves.
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* conjoinFixedSeqs(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, File_ଧାଡ଼ି* flp) {
        std::map<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>> checks;
        ଯଦି (!flattenFixedSeq(lhsp, 0, checks) || !flattenFixedSeq(rhsp, 0, checks)) ଫେରନ୍ତୁ nullptr;
        ଯଦି (checks.empty()) ଫେରନ୍ତୁ nullptr;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* resultp = nullptr;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା prevOffset = 0;
        ପାଇଁ (ଅଚଳ auto& offsetChecks : checks) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା offset = offsetChecks.first;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp = nullptr;
            ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ leafp : offsetChecks.second) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ clonep = leafp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                ଯଦି (!condp) {
                    condp = clonep;
                } ଅନ୍ୟଥା {
                    condp = ନୂତନ AstLogAnd{flp, condp, clonep};
                    condp->dtypeSetBit();
                }
            }
            ଯଦି (!resultp) {
                ଯଦି (offset > 0) {
                    AstDelay* ଅଚଳ delayp = ନୂତନ AstDelay{
                        flp, ନୂତନ AstConst{flp, static_cast<uint32_t>(offset)}, /*isCycle=*/ସତ୍ୟ};
                    resultp
                        = ନୂତନ AstSExpr{flp, ନୂତନ AstConst{flp, AstConst::BitTrue{}}, delayp, condp};
                    resultp->dtypeSetBit();
                } ଅନ୍ୟଥା {
                    resultp = condp;
                }
            } ଅନ୍ୟଥା {
                AstDelay* ଅଚଳ delayp = ନୂତନ AstDelay{
                    flp, ନୂତନ AstConst{flp, static_cast<uint32_t>(offset - prevOffset)},
                    /*isCycle=*/ସତ୍ୟ};
                resultp = ନୂତନ AstSExpr{flp, resultp, delayp, condp};
                resultp->dtypeSetBit();
            }
            prevOffset = offset;
        }
        ଫେରନ୍ତୁ resultp;
    }

    // `seq` is a simple ranged sequence `start ##[m:n] end` (start/end boolean,
    // start may be absent). Used to collapse a both-variable intersect to one
    // ranged delay.
    ସଂରଚନା SimpleRanged final {
        ତାର୍କିକ ok = ମିଥ୍ୟା;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* startp = nullptr;  // may be null (absent start)
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* endp = nullptr;
    };
    ସ୍ଥିର SimpleRanged asSimpleRanged(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        AstSExpr* ଅଚଳ sexprp = VN_CAST(nodep, SExpr);
        ଯଦି (!sexprp) ଫେରନ୍ତୁ {};
        AstDelay* ଅଚଳ delayp = VN_CAST(sexprp->delayp(), Delay);
        ଯଦି (!delayp || !delayp->isCycleDelay() || !delayp->isRangeDelay() || delayp->isUnbounded())
            ଫେରନ୍ତୁ {};
        ଯଦି (getConstUInt(delayp->lhsp()) == getConstUInt(delayp->rhsp())) ଫେରନ୍ତୁ {};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ prep = sexprp->preExprp();
        ଯଦି (prep && fixedLength(prep) != 0) ଫେରନ୍ତୁ {};
        ଯଦି (fixedLength(sexprp->exprp()) != 0) ଫେରନ୍ତୁ {};
        ଫେରନ୍ତୁ {ସତ୍ୟ, prep, sexprp->exprp()};
    }

    // Build the NFA for a synthesized intersect lowering tree, then free it.
    // buildExpr returns the terminal condition (finalCondp) by reference into the
    // tree; detach a clone so the tree can be freed here. The graph already holds
    // clones/hoists of every edge condition, so nothing else dangles.
    BuildResult build_From_Lowering_ବୃକ୍ଷ(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* treep, SvaStateVertex* entryVtxp,
                                      ତାର୍କିକ is_ଶୀର୍ଷ_Level_Step) {
        BuildResult result = build_ପଦସମଷ୍ଟି(treep, entryVtxp, is_ଶୀର୍ଷ_Level_Step);
        ଯଦି (result.valid() && result.finalCondp) {
            result.finalCondp = result.finalCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        }
        VL_DO_DANGLING(treep->delete_ବୃକ୍ଷ(), treep);
        ଫେରନ୍ତୁ result;
    }

    // Empty common-length intersection -- unequal fixed lengths, or disjoint
    // ranged lengths. IEEE 1800-2023 16.9.6 requires both operands to match
    // over a window of the same length, so with no common length the intersect
    // simply never matches. This is legal (matching nothing), not an error, so
    // lower to a constant false rather than rejecting legal code.
    BuildResult buildNeverMatchIntersect(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, SvaStateVertex* entryVtxp,
                                         ତାର୍କିକ is_ଶୀର୍ଷ_Level_Step, ଅଚଳ ଅକ୍ଷର* reason) {
        nodep->v3warn(NEVERMATCH, "Sequence can never match because " << reason << ".");
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ falsep = ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalse{}};
        ଫେରନ୍ତୁ build_From_Lowering_ବୃକ୍ଷ(falsep, entryVtxp, is_ଶୀର୍ଷ_Level_Step);
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
                                     ତାର୍କିକ is_ଶୀର୍ଷ_Level_Step) {
        ଅଚଳ std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା> lhsRange = lengthRange(nodep->lhsp());
        ଅଚଳ std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା> rhsRange = lengthRange(nodep->rhsp());
        ଯଦି (lhsRange.first < 0 || rhsRange.first < 0) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect with this variable-length operand");
            ଫେରନ୍ତୁ BuildResult::failWithError();
        }
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lo = std::max(lhsRange.first, rhsRange.first);
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା hi = std::min(lhsRange.second, rhsRange.second);
        ଯଦି (lo > hi) {
            // Disjoint length ranges share no common length -> never matches.
            ଫେରନ୍ତୁ buildNeverMatchIntersect(nodep, entryVtxp, is_ଶୀର୍ଷ_Level_Step,
                                            "intersect operands have no common length");
        }
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        ଯଦି (lo == hi) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lp = realizeAtLength(nodep->lhsp(), lo, lhsRange.first);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rp = realizeAtLength(nodep->rhsp(), lo, rhsRange.first);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ conjp = conjoinFixedSeqs(lp, rp, flp);
            VL_DO_DANGLING(lp->delete_ବୃକ୍ଷ(), lp);
            VL_DO_DANGLING(rp->delete_ବୃକ୍ଷ(), rp);
            ଯଦି (!conjp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: intersect operand is not a plain boolean sequence");
                ଫେରନ୍ତୁ BuildResult::failWithError();
            }
            ଫେରନ୍ତୁ build_From_Lowering_ବୃକ୍ଷ(conjp, entryVtxp, is_ଶୀର୍ଷ_Level_Step);
        }
        ଅଚଳ SimpleRanged sl = asSimpleRanged(nodep->lhsp());
        ଅଚଳ SimpleRanged sr = asSimpleRanged(nodep->rhsp());
        ଯଦି (!sl.ok || !sr.ok) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect of two sequences that each vary in length over a"
                          " range with internal structure");
            ଫେରନ୍ତୁ BuildResult::failWithError();
        }
        ଅଚଳ auto andBool = [&](Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ap, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* bp) -> Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ aClonep
                = ap ? ap->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା) : ନୂତନ AstConst{flp, AstConst::BitTrue{}};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ bClonep
                = bp ? bp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା) : ନୂତନ AstConst{flp, AstConst::BitTrue{}};
            AstLogAnd* ଅଚଳ andp = ନୂତନ AstLogAnd{flp, aClonep, bClonep};
            andp->dtypeSetBit();
            ଫେରନ୍ତୁ andp;
        };
        AstDelay* ଅଚଳ delayp = ନୂତନ AstDelay{flp, ନୂତନ AstConst{flp, static_cast<uint32_t>(lo)},
                                              /*isCycle=*/ସତ୍ୟ};
        delayp->rhsp(ନୂତନ AstConst{flp, static_cast<uint32_t>(hi)});
        AstSExpr* ଅଚଳ reducedp
            = ନୂତନ AstSExpr{flp, andBool(sl.startp, sr.startp), delayp, andBool(sl.endp, sr.endp)};
        reducedp->dtypeSetBit();
        ଫେରନ୍ତୁ build_From_Lowering_ବୃକ୍ଷ(reducedp, entryVtxp, is_ଶୀର୍ଷ_Level_Step);
    }

    BuildResult buildThroughout(AstSThroughout* nodep, SvaStateVertex* entryVtxp,
                                ତାର୍କିକ is_ଶୀର୍ଷ_Level_Step = ମିଥ୍ୟା) {
        // Mark entryVtxp so "cond false at tick 0" is detected as throughout-drop.
        entryVtxp->m_throughoutConds.ଠେଲ_back(nodep->lhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
        m_temporal_Guard_ଗଦା.ଠେଲ_back(nodep->lhsp());
        ଅଚଳ BuildResult result = build_ପଦସମଷ୍ଟି(nodep->rhsp(), entryVtxp, is_ଶୀର୍ଷ_Level_Step);
        m_temporal_Guard_ଗଦା.କାଢ଼_back();
        ଫେରନ୍ତୁ result;
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
    BuildResult buildUntil(AstUntil* nodep, SvaStateVertex* entryVtxp, ତାର୍କିକ is_ଶୀର୍ଷ_Level_Step) {
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        ଯଦି (!is_ଶୀର୍ଷ_Level_Step) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            ଫେରନ୍ତୁ BuildResult::failWithError();
        }
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp();
        ଅଚଳ auto hasSeq = [](ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ep) {
            ଫେରନ୍ତୁ ep->exists([](ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* np) { ଫେରନ୍ତୁ np->isMultiCycleSva(); });
        };
        ଯଦି (hasSeq(lhsp) || hasSeq(rhsp)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            ଫେରନ୍ତୁ BuildResult::failWithError();
        }

        ଅଚଳ ତାର୍କିକ ov = nodep->isOverlapping();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsBitp = nodep->lhsp();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsBitp = nodep->rhsp();
        // p hoist count: continue, require (ov: 1 use; nov: 1 use). At least 2 uses.
        AstVar* ଅଚଳ pHoistp = tryHoistSampled(lhsBitp, flp, 2);
        // q hoist count: continue (1) + require nov (1) = 2; ov: continue only (1).
        AstVar* ଅଚଳ qHoistp = ov ? nullptr : tryHoistSampled(rhsBitp, flp, 2);

        SvaStateVertex* ଅଚଳ waitCp = scopedCreateVertex();
        SvaStateVertex* ଅଚଳ waitRp = scopedCreateVertex();
        waitCp->m_isUnbounded = ସତ୍ୟ;
        waitRp->m_strongPending = nodep->isStrong() && !m_isCover;

        // Entry and back-loop Links carry no condition; throughout-folding still applies.
        guardedLink(entryVtxp, waitCp, flp);
        guardedLink(waitRp, waitCp, flp);

        // Continue clocked edge: p && !q advances to next-cycle wait.
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ contCondp
            = ନୂତନ AstLogAnd{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                            ନୂତନ AstLogNot{flp, sampledRefOrClone(qHoistp, rhsBitp, flp)}};
        guardedEdge(waitCp, waitRp, contCondp, flp);

        // Reject sink: fires when require-condition is false.
        SvaStateVertex* ଅଚଳ sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = ସତ୍ୟ;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* requireCondp;
        ଯଦି (ov) {
            requireCondp = sampledRefOrClone(pHoistp, lhsBitp, flp);
        } ଅନ୍ୟଥା {
            requireCondp = ନୂତନ AstLogOr{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                                        sampledRefOrClone(qHoistp, rhsBitp, flp)};
        }
        SvaTransEdge* ଅଚଳ rejEdgep = m_graph.addLink(waitCp, sinkVtxp, requireCondp);
        rejEdgep->m_rejectOnFail = ସତ୍ୟ;

        // Accept condition rides via finalCondp; assembleResult $sampled-wraps it.
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* acceptCondp;
        ଯଦି (ov) {
            acceptCondp
                = ନୂତନ AstLogAnd{flp, lhsBitp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), rhsBitp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
        } ଅନ୍ୟଥା {
            acceptCondp = rhsBitp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        }
        ଫେରନ୍ତୁ {waitCp, acceptCondp, {}};
    }

    // IEEE 1800-2023 16.12.14 property abort operators. Sync and async share
    // the same NFA encoding: AstSampled already gives matured values at every
    // maturing clocking event, and async firing "between clocks" is not
    // observable in a cycle-based model. VAbortKind selects accept vs reject
    // verdict (sync vs async only changes the user-visible spelling).

    // Build `condp && !outer_1 && !outer_2 ...` (unsampled).
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* abort_Fire_ପଦସମଷ୍ଟି(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp, File_ଧାଡ଼ି* flp) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* resultp = condp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ op : m_outer_Abort_ଗଦା)
            resultp = ନୂତନ AstLogAnd{flp, resultp, ନୂତନ AstLogNot{flp, op->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)}};
        ଫେରନ୍ତୁ resultp;
    }

    // True when unguarded ring-wide rejection or a same-tick Link chain already accounts the
    // attempt: a required-step Link covers both outcomes; followed-by pairs both edges.
    ସ୍ଥିର ତାର୍କିକ chainAccountsSource(ଅଚଳ SvaStateVertex* srcp,
                                    ଅଚଳ std::unordered_set<ଅଚଳ V3GraphEdge*>& preEdges) {
        ଯଦି (srcp->m_delayRingSize && srcp->m_throughoutConds.empty()) ଫେରନ୍ତୁ ସତ୍ୟ;
        ପାଇଁ (ଅଚଳ V3GraphEdge& edger : srcp->inEdges()) {
            ଯଦି (preEdges.ଗଣନା_2(&edger)) ଜାରି;
            ଅଚଳ SvaTransEdge& tedger = static_cast<ଅଚଳ SvaTransEdge&>(edger);
            ଯଦି (tedger.m_consumesCycle) ଜାରି;
            ଅଚଳ auto* ଅଚଳ fromp = static_cast<ଅଚଳ SvaStateVertex*>(tedger.fromVtxp());
            ଯଦି (fromp->m_abortRejectp) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ତାର୍କିକ plainNonSink = ମିଥ୍ୟା;
        ତାର୍କିକ markedSink = ମିଥ୍ୟା;
        ପାଇଁ (ଅଚଳ V3GraphEdge& edger : srcp->outEdges()) {
            ଯଦି (preEdges.ଗଣନା_2(&edger)) ଜାରି;
            ଅଚଳ SvaTransEdge& tedger = static_cast<ଅଚଳ SvaTransEdge&>(edger);
            ଯଦି (tedger.m_consumesCycle) ଜାରି;
            ଅଚଳ ତାର୍କିକ sink = static_cast<ଅଚଳ SvaStateVertex*>(tedger.toVtxp())->m_isRejectSink;
            ଯଦି (tedger.m_rejectOnFail) {
                ଯଦି (!sink) ଫେରନ୍ତୁ ସତ୍ୟ;
                markedSink = ସତ୍ୟ;
            } ଅନ୍ୟଥା ଯଦି (!sink) {
                ଯଦି (srcp->m_abortRejectp) ଫେରନ୍ତୁ ସତ୍ୟ;
                plainNonSink = ସତ୍ୟ;
            }
        }
        ଫେରନ୍ତୁ plainNonSink && markedSink;
    }

    // Reject edge: fires when the source is live and the abort samples true.
    ଶୂନ୍ୟ addAbortRejectEdge(SvaStateVertex* srcp, SvaStateVertex* sinkp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp,
                            File_ଧାଡ଼ି* flp) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ notFirep = ନୂତନ AstLogNot{flp, sampled(abort_Fire_ପଦସମଷ୍ଟି(condp, flp))};
        m_graph.addLink(srcp, sinkp, notFirep)->m_rejectOnFail = ସତ୍ୟ;
        ଫେରନ୍ତୁ;
    }

    // On the fire tick: kill body threads; accept kinds also forgive step misses.
    ଶୂନ୍ୟ gateBodyEdgesOnAbort(ଅଚଳ std::unordered_set<ଅଚଳ V3GraphEdge*>& preEdges,
                              Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp, VAbortKind kind, File_ଧାଡ଼ି* flp) {
        ପାଇଁ (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            ପାଇଁ (V3GraphEdge& edger : vtxr.outEdges()) {
                ଯଦି (preEdges.ଗଣନା_2(&edger)) ଜାରି;
                SvaTransEdge* ଅଚଳ tedgep = static_cast<SvaTransEdge*>(&edger);
                ଯଦି (tedgep->m_rejectOnFail) {
                    ଯଦି (!kind.isAccept()) ଜାରି;
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ firep = sampled(abort_Fire_ପଦସମଷ୍ଟି(condp, flp));
                    tedgep->m_condp
                        = tedgep->m_condp ? ନୂତନ AstLogOr{flp, tedgep->m_condp, firep} : firep;
                } ଅନ୍ୟଥା ଯଦି (tedgep->m_consumesCycle) {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ notFirep
                        = ନୂତନ AstLogNot{flp, sampled(abort_Fire_ପଦସମଷ୍ଟି(condp, flp))};
                    tedgep->m_condp = tedgep->m_condp
                                          ? ନୂତନ AstLogAnd{flp, tedgep->m_condp, notFirep}
                                          : notFirep;
                }
            }
        }
    }

    BuildResult buildAbortOn(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* bodyp, SvaStateVertex* entryVtxp,
                             VAbortKind kind, File_ଧାଡ଼ି* flp, ତାର୍କିକ is_ଶୀର୍ଷ_Level_Step) {
        // Snapshot pre-body vertices/edges so post-build diff yields the body's sub-NFA.
        std::unordered_set<ଅଚଳ V3GraphVertex*> preExisting;
        std::unordered_set<ଅଚଳ V3GraphEdge*> preEdges;
        ପାଇଁ (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            preExisting.ଭର୍ତ୍ତି(&vtxr);
            ପାଇଁ (V3GraphEdge& edger : vtxr.outEdges()) preEdges.ଭର୍ତ୍ତି(&edger);
        }

        m_outer_Abort_ଗଦା.ଠେଲ_back(condp);
        ଅଚଳ BuildResult bodyResult = build_ପଦସମଷ୍ଟି(bodyp, entryVtxp, is_ଶୀର୍ଷ_Level_Step);
        m_outer_Abort_ଗଦା.କାଢ଼_back();
        ଯଦି (!bodyResult.valid()) ଫେରନ୍ତୁ bodyResult;

        gateBodyEdgesOnAbort(preEdges, condp, kind, flp);

        // Live-thread sources for the abort edge: entry + new body vertices,
        // minus reject sinks (they carry reject fuel, not live-thread fuel).
        std::vector<SvaStateVertex*> abortSources;
        abortSources.ଠେଲ_back(entryVtxp);
        ପାଇଁ (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            ଯଦି (preExisting.ଗଣନା_2(&vtxr)) ଜାରି;
            auto* ଅଚଳ sp = static_cast<SvaStateVertex*>(&vtxr);
            ଯଦି (sp->m_delayRingSize || !sp->m_throughoutConds.empty()) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ firep = abort_Fire_ପଦସମଷ୍ଟି(condp, flp);
                sp->m_abortClearp
                    = sp->m_abortClearp ? ନୂତନ AstLogOr{flp, sp->m_abortClearp, firep} : firep;
            }
            ଯଦି (!kind.isAccept()
                && ((sp->m_delayRingSize && sp->m_throughoutConds.empty())
                    || sp->m_replayAbortReject)) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ firep = abort_Fire_ପଦସମଷ୍ଟି(condp, flp);
                sp->m_abortRejectp
                    = sp->m_abortRejectp ? ନୂତନ AstLogOr{flp, sp->m_abortRejectp, firep} : firep;
            }
            ଯଦି (sp->m_isRejectSink) ଜାରି;
            abortSources.ଠେଲ_back(sp);
        }

        auto sampledAbortFire = [&]() -> Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ ପଦସମଷ୍ଟି = abort_Fire_ପଦସମଷ୍ଟି(condp, flp);
            ଫେରନ୍ତୁ sampled(ପଦସମଷ୍ଟି);
        };

        ଯଦି (kind.isAccept()) {
            // Match-only sink fed by $sampled(abort-fire) from every live source;
            // registered as midSource so it never contributes a reject.
            SvaStateVertex* ଅଚଳ acceptSinkp = scopedCreateVertex();
            ପାଇଁ (SvaStateVertex* ଅଚଳ srcp : abortSources)
                guardedLink(srcp, acceptSinkp, sampledAbortFire(), flp);
            std::vector<SvaStateVertex*> midSources = bodyResult.midSources;
            midSources.ଠେଲ_back(acceptSinkp);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* finalCondp = bodyResult.finalCondp;
            ଯଦି (finalCondp) {
                ଯଦି (finalCondp->backp()) finalCondp = finalCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                finalCondp = ନୂତନ AstLogOr{flp, finalCondp, abort_Fire_ପଦସମଷ୍ଟି(condp, flp)};
            }
            ଫେରନ୍ତୁ {bodyResult.termVertexp, finalCondp, std::move(midSources)};
        }

        // rejectOnFail treats m_condp as the success condition and fires on
        // !condp, so the edge carries !sampledAbortFire().
        SvaStateVertex* ଅଚଳ rejectSinkp = m_graph.createStateVertex();
        rejectSinkp->m_isRejectSink = ସତ୍ୟ;
        ପାଇଁ (SvaStateVertex* ଅଚଳ srcp : abortSources)
            ଯଦି (!chainAccountsSource(srcp, preEdges))
                addAbortRejectEdge(srcp, rejectSinkp, condp, flp);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* finalCondp = bodyResult.finalCondp;
        ଯଦି (finalCondp) {
            ଯଦି (finalCondp->backp()) finalCondp = finalCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            finalCondp
                = ନୂତନ AstLogAnd{flp, finalCondp, ନୂତନ AstLogNot{flp, abort_Fire_ପଦସମଷ୍ଟି(condp, flp)}};
        }
        ଫେରନ୍ତୁ {bodyResult.termVertexp, finalCondp, bodyResult.midSources};
    }

ସର୍ବସାଧାରଣ:
    // Free a dropped sub-result condition that is not linked into the AST
    // (abort folds synthesize unparented finalCondp trees).
    ସ୍ଥିର ଶୂନ୍ୟ freeUnlinkedCondp(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp) {
        ଯଦି (condp && !condp->backp()) VL_DO_DANGLING(condp->delete_ବୃକ୍ଷ(), condp);
    }

    SvaNfaBuilder(SvaGraph& graph, Ast_ଗଣ୍ଠି_Module* modp, V3UniqueNames& propTempNames,
                  ତାର୍କିକ isCoverSeq = ମିଥ୍ୟା, ତାର୍କିକ isSeqEvent = ମିଥ୍ୟା, ତାର୍କିକ isCover = ମିଥ୍ୟା)
        : m_graph{graph}
        , m_modp{modp}
        , m_propTempNames{propTempNames}
        , m_isCover{isCover}
        , m_isCoverSeq{isCoverSeq}
        , m_isSeqEvent{isSeqEvent} {}

    // Reset scope between antecedent and consequent: liveness must not leak.
    // m_outerAbortStack survives: an abort wrapping the implication covers the
    // consequent too (IEEE 1800-2023 16.12.14).
    ଶୂନ୍ୟ resetScope() {
        m_inUnboundedScope = ମିଥ୍ୟା;
        m_temporal_Guard_ଗଦା.clear();
    }

    BuildResult build_ପଦସମଷ୍ଟି(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, SvaStateVertex* entryVtxp,
                          ତାର୍କିକ is_ଶୀର୍ଷ_Level_Step = ମିଥ୍ୟା) {
        VL_RESTORER(m_wantBoolLeaf);
        ଅଚଳ ତାର୍କିକ wantBoolLeaf = m_wantBoolLeaf;
        m_wantBoolLeaf = ମିଥ୍ୟା;
        ଯଦି (AstSExpr* ଅଚଳ sexprp = VN_CAST(nodep, SExpr)) {
            ଫେରନ୍ତୁ buildSExpr(sexprp, entryVtxp, is_ଶୀର୍ଷ_Level_Step);
        }
        ଯଦି (AstSConsRep* ଅଚଳ repp = VN_CAST(nodep, SConsRep)) {
            ଫେରନ୍ତୁ buildConsRep(repp, entryVtxp, is_ଶୀର୍ଷ_Level_Step);
        }
        ଯଦି (AstPropAlways* ଅଚଳ alwaysp = VN_CAST(nodep, PropAlways)) {
            ଫେରନ୍ତୁ buildPropAlways(alwaysp, entryVtxp, is_ଶୀର୍ଷ_Level_Step);
        }
        ଯଦି (AstSGotoRep* ଅଚଳ repp = VN_CAST(nodep, SGotoRep)) {
            ଫେରନ୍ତୁ buildGotoRep(repp, entryVtxp);
        }
        ଯଦି (AstSThroughout* ଅଚଳ throughoutp = VN_CAST(nodep, SThroughout)) {
            ଫେରନ୍ତୁ buildThroughout(throughoutp, entryVtxp, is_ଶୀର୍ଷ_Level_Step);
        }
        ଯଦି (AstSOr* ଅଚଳ orp = VN_CAST(nodep, SOr)) {
            ଫେରନ୍ତୁ buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        ଯଦି (AstLogOr* ଅଚଳ orp = VN_CAST(nodep, LogOr)) {
            ଫେରନ୍ତୁ buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        ଯଦି (AstSAnd* ଅଚଳ andp = VN_CAST(nodep, SAnd)) {
            ଫେରନ୍ତୁ buildAndCombiner(andp->lhsp(), andp->rhsp(), entryVtxp, andp->fileline());
        }
        ଯଦି (AstSIntersect* ଅଚଳ intp = VN_CAST(nodep, SIntersect)) {
            // IEEE 1800-2023 16.9.6: both operands match over one window with
            // equal start and end (equal length). Lower to a single sequence
            // that conjoins both operands' per-cycle checks -- correct under
            // concurrent attempts, where the done-latch combiner conflates the
            // two operands' start times and over-accepts. The combiner remains
            // only as a fallback for operands that do not flatten.
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lhsLen = fixedLength(intp->lhsp());
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା rhsLen = fixedLength(intp->rhsp());
            ଯଦି (lhsLen >= 0 && rhsLen >= 0) {
                ଯଦି (lhsLen != rhsLen) {
                    // Unequal fixed lengths share no common length -> never matches.
                    ଫେରନ୍ତୁ buildNeverMatchIntersect(intp, entryVtxp, is_ଶୀର୍ଷ_Level_Step,
                                                    "intersect operands have no common length");
                }
                ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ conjp
                    = conjoinFixedSeqs(intp->lhsp(), intp->rhsp(), intp->fileline())) {
                    ଫେରନ୍ତୁ build_From_Lowering_ବୃକ୍ଷ(conjp, entryVtxp, is_ଶୀର୍ଷ_Level_Step);
                }
                ଫେରନ୍ତୁ buildAndCombiner(intp->lhsp(), intp->rhsp(), entryVtxp, intp->fileline());
            }
            ଫେରନ୍ତୁ buildVarLenIntersect(intp, entryVtxp, is_ଶୀର୍ଷ_Level_Step);
        }
        ଯଦି (AstSWithin* ଅଚଳ withinp = VN_CAST(nodep, SWithin)) {
            ଫେରନ୍ତୁ buildSWithin(withinp, entryVtxp, is_ଶୀର୍ଷ_Level_Step);
        }
        ଯଦି (AstAbortOn* ଅଚଳ ap = VN_CAST(nodep, AbortOn)) {
            ଫେରନ୍ତୁ buildAbortOn(ap->condp(), ap->propp(), entryVtxp, ap->kind(), ap->fileline(),
                                is_ଶୀର୍ଷ_Level_Step);
        }
        ଯଦି (VN_IS(nodep, SNonConsRep)) ଫେରନ୍ତୁ BuildResult::fail();
        ଯଦି (AstImplication* ଅଚଳ implp = VN_CAST(nodep, Implication)) {
            ଫେରନ୍ତୁ buildImplicationEdges(implp->lhsp(), implp->rhsp(), entryVtxp,
                                         implp->isOverlapped(), implp->isFollowedBy(),
                                         implp->lhsp(), implp->fileline());
        }
        ଯଦି (AstUntil* ଅଚଳ untilp = VN_CAST(nodep, Until)) {
            ଫେରନ୍ତୁ buildUntil(untilp, entryVtxp, is_ଶୀର୍ଷ_Level_Step);
        }
        // Boolean leaf (including LogAnd): return as finalCond
        ଫେରନ୍ତୁ {entryVtxp, nodep, {}};
    }

    // Wire an implication / followed-by from `entryVtxp`: builds the antecedent,
    // emits the match-link (and for followed-by the reject-sink edge), inserts a
    // delay vertex for non-overlapped forms, and builds the body. Used both for
    // nested AstImplication in pexpr position and for the top-level assertion
    // antecedent -- `errorNodep` anchors the "unsupported sequence antecedent"
    // error, which differs between the two call sites.
    BuildResult buildImplicationEdges(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* antExprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* bodyExprp,
                                      SvaStateVertex* entryVtxp, ତାର୍କିକ isOverlapped,
                                      ତାର୍କିକ isFollowedBy, Ast_ଗଣ୍ଠି* errorNodep, File_ଧାଡ଼ି* flp) {
        ଅଚଳ BuildResult antResult = build_ପଦସମଷ୍ଟି(antExprp, entryVtxp);
        ଯଦି (!antResult.valid()) ଫେରନ୍ତୁ antResult;

        // Followed-by requires pure-boolean antecedent for non-vacuous-fail at
        // the attempt-start cycle. IEEE 1800-2023 16.12.9 permits a multi-cycle
        // sequence LHS, so this is an implementation gap rather than illegal SV.
        ଯଦି (isFollowedBy && antResult.termVertexp != entryVtxp) {
            errorNodep->v3warn(E_UNSUPPORTED,
                               "Unsupported: sequence expression as antecedent of followed-by"
                               " (#-# / #=#) (IEEE 1800-2023 16.12.9)");
            ଫେରନ୍ତୁ BuildResult::failWithError();
        }
        UASSERT_OBJ(!isFollowedBy || antResult.finalCondp, errorNodep,
                    "followed-by antecedent terminal at entry must carry finalCondp");

        // Use raw createStateVertex() so trigVtxp starts without liveness --
        // reaching the antecedent terminal is a definitive event.
        SvaStateVertex* ଅଚଳ trigVtxp = m_graph.createStateVertex();
        ଯଦି (antResult.finalCondp) {
            m_graph.addLink(antResult.termVertexp, trigVtxp,
                            sampled(antResult.finalCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)));
            // Followed-by non-vacuous fail: rejectOnFail fires when the attempt
            // is live (termVtx reachable) and sampled(antecedent) is false.
            ଯଦି (isFollowedBy) {
                SvaStateVertex* ଅଚଳ sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = ସତ୍ୟ;
                SvaTransEdge* ଅଚଳ ep
                    = m_graph.addLink(antResult.termVertexp, sinkVtxp,
                                      sampled(antResult.finalCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)));
                ep->m_rejectOnFail = ସତ୍ୟ;
            }
            // finalCondp is cloned into the Sampled nodes; if the original is
            // not parented anywhere in the AST anymore it must be freed here
            // or ASan flags it as a leak (e.g. t_sequence_bool_ops).
            ଯଦି (!antResult.finalCondp->backp()) {
                VL_DO_DANGLING(antResult.finalCondp->delete_ବୃକ୍ଷ(), antResult.finalCondp);
            }
        } ଅନ୍ୟଥା {
            m_graph.addLink(antResult.termVertexp, trigVtxp);
        }
        resetScope();

        SvaStateVertex* bodyEntryp = trigVtxp;
        ଯଦି (!isOverlapped) {
            SvaStateVertex* ଅଚଳ delayVtxp = m_graph.createStateVertex();
            m_graph.addClockedEdge(trigVtxp, delayVtxp);
            bodyEntryp = delayVtxp;
        }
        // An empty antecedent match must not start the consequent, but a zero-minimum
        // repetition's empty match is not handled here; keep the merge vertex for those
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = !isFollowedBy && !hasZeroMinRep(antExprp);
        ଫେରନ୍ତୁ build_ପଦସମଷ୍ଟି(bodyExprp, bodyEntryp, /*isTopLevelStep=*/ସତ୍ୟ);
    }

    BuildResult build(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        m_graph.m_startVertexp = scopedCreateVertex();
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = ସତ୍ୟ;
        ଫେରନ୍ତୁ build_ପଦସମଷ୍ଟି(exprp, m_graph.m_startVertexp, /*isTopLevelStep=*/ସତ୍ୟ);
    }
};

//######################################################################
// NFA Lowering (converts NFA graph to synthesizable AstAlways blocks)

ଶ୍ରେଣୀ SvaNfaLowering final {
    Ast_ଗଣ୍ଠି_Module* ଅଚଳ m_modp;  // Module to add state vars and always blocks to
    Ast_ଗଣ୍ଠି_DType* ଅଚଳ m_u32DTypep;  // Shared unsigned counter dtype
    V3UniqueNames m_names{"__Vnfa"};
    size_t m_statDelayRingEdgeVisits = 0;  // Delay-ring incoming edges visited

    // Per-lowering shared context (passed to phase sub-functions)
    // Per-vertex lowering state is stored in SvaVertexData and accessed via
    // V3GraphVertex::userp() (see SvaStateVertex::datap()).
    ସଂରଚନା LowerCtx final {
        File_ଧାଡ଼ି* flp;  // Source location for generated AST
        Ast_Sen_ବୃକ୍ଷ* senTreep;  // Clock sensitivity tree
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* disableExprp;  // disable iff expression (may be nullptr)
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* matchCondp;  // Final boolean match condition (may be nullptr)
        VAssertType assertType;  // Assertion type for control tasks
        VAssertDirectiveType directiveType;  // Directive type for control tasks
        AstVar* killVarp;  // Last observed kill generation
        SvaGraph& graph;  // NFA graph
    };

    // Build a match-now expression: stateSig[i] && $sampled(condp)
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* buildMatchNow(File_ଧାଡ଼ି* flp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* stateExprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ statep = stateExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        ଯଦି (!condp) ଫେରନ୍ତୁ statep;
        ଫେରନ୍ତୁ ନୂତନ AstLogAnd{flp, statep, sampled(condp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା))};
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* andCond(File_ଧାଡ଼ି* flp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp) {
        ଯଦି (!condp) ଫେରନ୍ତୁ exprp;
        ଫେରନ୍ତୁ ନୂତନ AstLogAnd{flp, exprp, condp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
    }
    // bp is always non-null; only ap can be null (serving as accumulator).
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* orExprs(File_ଧାଡ଼ି* flp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ap, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* bp) {
        ଯଦି (!ap) ଫେରନ୍ତୁ bp;
        ଫେରନ୍ତୁ ନୂତନ AstLogOr{flp, ap, bp};
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* addCountp(File_ଧାଡ଼ି* ଅଚଳ flp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ totalCountp,
                                  Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ contributionp,
                                  Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enablep = nullptr) {
        // contribution = enable ? contribution : 0;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enabledContributionp
            = enablep ? ନୂତନ AstCond{flp, enablep, contributionp,
                                    newTypedConstp(flp, contributionp->dtypep(), 0)}
                      : contributionp;
        ଯଦି (!totalCountp) ଫେରନ୍ତୁ enabledContributionp;
        ଫେରନ୍ତୁ ନୂତନ AstAdd{flp, totalCountp, enabledContributionp};
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* gateCheckedCountp(File_ଧାଡ଼ି* ଅଚଳ flp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ countp,
                                          Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*& overflowp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enablep) {
        ଯଦି (overflowp) {
            overflowp = ନୂତନ AstLogAnd{flp, enablep->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), overflowp};
        }
        ଫେରନ୍ତୁ addCountp(flp, nullptr, countp, enablep);
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* addCheckedCountp(File_ଧାଡ଼ି* ଅଚଳ flp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ totalCountp,
                                         Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ contributionp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*& overflowp,
                                         Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ contributionOverflowp = nullptr) {
        ଯଦି (contributionOverflowp) { overflowp = orExprs(flp, overflowp, contributionOverflowp); }
        ଯଦି (!totalCountp) ଫେରନ୍ତୁ contributionp;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ oldTotalp = totalCountp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ resultp = ନୂତନ AstAdd{flp, totalCountp, contributionp};
        overflowp
            = orExprs(flp, overflowp, ନୂତନ AstLt{flp, resultp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), oldTotalp});
        ଫେରନ୍ତୁ resultp;
    }
    AstIf* new_ଗଣନା_Overflow_Warning(File_ଧାଡ଼ି* ଅଚଳ flp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp) ଅଚଳ {
        AstDisplay* ଅଚଳ dispp = ନୂତନ AstDisplay{
            flp, VDisplayType::DT_WARNING,
            "Cover sequence match count overflowed 32-bit storage; results may be incorrect.",
            nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, condp, dispp};
        ifp->branchPred(VBranchPred::BP_UNLIKELY);
        ଫେରନ୍ତୁ ifp;
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* killActive(LowerCtx& c) {
        ଫେରନ୍ତୁ ନୂତନ AstNeq{c.flp, ନୂତନ AstVarRef{c.flp, c.killVarp, VAccess::READ},
                          assertKillGet(c.flp, c.assertType, c.directiveType)};
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* notKillActive(LowerCtx& c) { ଫେରନ୍ତୁ ନୂତନ AstLogNot{c.flp, killActive(c)}; }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* gateNotKill(LowerCtx& c, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        ଯଦି (!exprp) ଫେରନ୍ତୁ nullptr;
        ଫେରନ୍ତୁ ନୂତନ AstLogAnd{c.flp, exprp, notKillActive(c)};
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nextRingIndex(File_ଧାଡ଼ି* flp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* idxExprp, uint32_t size) {
        ଅଚଳ auto u32Const = [flp](uint32_t ମୂଲ୍ୟ) {
            ଫେରନ୍ତୁ ନୂତନ AstConst{flp, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, ମୂଲ୍ୟ};
        };
        UASSERT_OBJ(size > 0, idxExprp, "Ring size must be positive");
        ଯଦି (size == 1) {
            VL_DO_DANGLING(idxExprp->delete_ବୃକ୍ଷ(), idxExprp);
            ଫେରନ୍ତୁ u32Const(0);
        }
        // idx == size - 1 ? 0 : idx + 1
        AstAdd* ଅଚଳ addp = ନୂତନ AstAdd{flp, idxExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), u32Const(1)};
        addp->dtypeFrom(idxExprp);
        AstCond* ଅଚଳ condp
            = ନୂତନ AstCond{flp, ନୂତନ AstEq{flp, idxExprp, u32Const(size - 1)}, u32Const(0), addp};
        condp->dtypeFrom(idxExprp);
        ଫେରନ୍ତୁ condp;
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* delayRingSlot(File_ଧାଡ଼ି* flp, SvaStateVertex* vtxp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* idxExprp,
                                      VAccess access = VAccess::READ) {
        // ring[idx]
        AstVar* ଅଚଳ ringp = vtxp->datap()->delayRingVarp;
        AstVarRef* ଅଚଳ refp = ନୂତନ AstVarRef{flp, ringp, access};
        ଯଦି (vtxp->datap()->delayRingCounts) ଫେରନ୍ତୁ ନୂତନ AstArraySel{flp, refp, idxExprp};
        ଫେରନ୍ତୁ ନୂତନ AstSel{flp, refp, idxExprp, 1};
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* delay_Ring_Head_ଗଣନା(File_ଧାଡ଼ି* flp, SvaStateVertex* vtxp) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ headp
            = ନୂତନ AstVarRef{flp, vtxp->datap()->delayRingHeadVarp, VAccess::READ};
        ଯଦି (vtxp->datap()->delayRingCounts) ଫେରନ୍ତୁ headp;
        ଫେରନ୍ତୁ ନୂତନ AstExtend{flp, headp, vtxp->datap()->delay_Ring_Live_ଗଣନା_Varp->dtypep()->width()};
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* delayRingAtLastIndex(File_ଧାଡ଼ି* ଅଚଳ flp, AstVar* ଅଚଳ idxp,
                                             ଅଚଳ uint32_t size) {
        ଫେରନ୍ତୁ ନୂତନ AstEq{flp, ନୂତନ AstVarRef{flp, idxp, VAccess::READ},
                         ନୂତନ AstConst{flp, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, size - 1}};
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* delay_Ring_Output_ଗଣନା(File_ଧାଡ଼ି* ଅଚଳ flp, SvaStateVertex* ଅଚଳ vtxp) {
        AstVar* ଅଚଳ idxp = vtxp->datap()->delayRingIdxVarp;
        ଅଚଳ uint32_t size = vtxp->m_delayRingSize;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* outgoingValidp
            = ନୂତନ AstVarRef{flp, vtxp->datap()->delayRingWrappedVarp, VAccess::READ};
        ଯଦି (!vtxp->m_isFixedDelayRing) {
            outgoingValidp
                = ନୂତନ AstLogOr{flp, outgoingValidp, delayRingAtLastIndex(flp, idxp, size)};
        }
        AstVar* ଅଚଳ live_ଗଣନା_Varp = vtxp->datap()->delay_Ring_Live_ଗଣନା_Varp;
        ଫେରନ୍ତୁ ନୂତନ AstCond{flp, outgoingValidp, delay_Ring_Head_ଗଣନା(flp, vtxp),
                           newTypedConstp(flp, live_ଗଣନା_Varp->dtypep(), 0)};
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* delayRingHasLiveThreadsp(File_ଧାଡ଼ି* ଅଚଳ flp,
                                                 AstVar* ଅଚଳ live_ଗଣନା_Varp) {
        // active = live_count != 0;
        ଫେରନ୍ତୁ ନୂତନ AstNeq{flp, ନୂତନ AstVarRef{flp, live_ଗଣନା_Varp, VAccess::READ},
                          newTypedConstp(flp, live_ଗଣନା_Varp->dtypep(), 0)};
    }

    // Phase 3 output signals
    ସଂରଚନା SignalSet final {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* terminalActivep = nullptr;  // OR of all successful terminal matches
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* matchCountp = nullptr;  // NFA paths completing the sequence this tick
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* match_ଗଣନା_Overflowp = nullptr;  // Match count overflow this tick
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rejectBasep = nullptr;  // Reject when a terminal match fails
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* requiredStepRejectp = nullptr;  // Per-source reject from rejectOnFail Links
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* throughoutRejectp = nullptr;  // Reject when a throughout guard drops
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* threadFailCountp = nullptr;  // Number of threads rejected on this tick
    };

    // Disable condition also aborts attempts between clock events. A constant disable
    // never rises, and $sampled is unsupported in a sensitivity list.
    ସ୍ଥିର ତାର୍କିକ isAsyncDisable(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ disableExprp) {
        ଫେରନ୍ତୁ disableExprp && !VN_IS(disableExprp, Const)
               && !disableExprp->exists([](ଅଚଳ AstSampled*) { ଫେରନ୍ତୁ ସତ୍ୟ; });
    }

    // Emit the clocked update block of registers that hold pending attempts. The clearp
    // statements reset the same registers: a disable rising between clock events also
    // aborts pending attempts (IEEE 1800-2023 16.12), so clearp runs on that rising edge
    // in a separate block. It must clear unconditionally, as the disable may already be
    // false again when it runs, and the update block must not run on that edge, as it
    // would advance the attempts as if the clock had ticked. Blocks that hold no attempt
    // state, such as the $assertkill acknowledge in emitKillAckNba, do not go through here.
    // Callers pass a null clearp when the disable cannot rise between clock events.
    ଶୂନ୍ୟ addStateAlways(LowerCtx& c, Ast_ଗଣ୍ଠି* bodyp, Ast_ଗଣ୍ଠି* clearp) {
        m_modp->addStmtsp(
            ନୂତନ AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), bodyp});
        ଯଦି (!clearp) ଫେରନ୍ତୁ;
        m_modp->addStmtsp(ନୂତନ AstAlways{
            c.flp, VAlwaysKwd::ALWAYS,
            ନୂତନ Ast_Sen_ବୃକ୍ଷ{c.flp, ନୂତନ AstSenItem{c.flp, VEdgeType::ET_POSEDGE,
                                                 c.disableExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)}},
            clearp});
    }

    // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
    // Phase 2: State register NBA always block. Each clocked-edge target
    // latches the OR of its incoming contributions.
    ଶୂନ୍ୟ emitStateRegisterNba(LowerCtx& c) {
        ଅଚଳ ତାର୍କିକ asyncDisable = isAsyncDisable(c.disableExprp);
        Ast_ଗଣ୍ଠି* bodyp = nullptr;
        Ast_ଗଣ୍ଠି* clearp = nullptr;
        ପାଇଁ (ଅଚଳ V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ଅଚଳ SvaStateVertex* ଅଚଳ vtxp = static_cast<ଅଚଳ SvaStateVertex*>(&vtxr);
            ଯଦି (!vtxp->datap()->stateVarp) ଜାରି;

            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nextStatep = nullptr;
            ପାଇଁ (ଅଚଳ V3GraphEdge& edger : vtxp->inEdges()) {
                ଅଚଳ SvaTransEdge& tedger = static_cast<ଅଚଳ SvaTransEdge&>(edger);
                ଯଦି (!tedger.m_consumesCycle) ଜାରି;
                ଅଚଳ SvaStateVertex* ଅଚଳ fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Clocked-edge source missing stateSig");

                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* srcSigp = fromVtxp->datap()->stateSigp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                srcSigp = andCond(c.flp, srcSigp, tedger.m_condp);

                ଯଦି (c.disableExprp) {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ notDisp
                        = ନୂତନ AstLogNot{c.flp, c.disableExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                    srcSigp = ନୂତନ AstLogAnd{c.flp, srcSigp, notDisp};
                }
                nextStatep = orExprs(c.flp, nextStatep, srcSigp);
            }

            UASSERT_OBJ(nextStatep, vtxp,
                        "Registered vertex has no clocked incoming contribution");
            nextStatep = gateNotKill(c, nextStatep);

            AstAssignDly* ଅଚଳ assignp = ନୂତନ AstAssignDly{
                c.flp, ନୂତନ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE}, nextStatep};
            bodyp = Ast_ଗଣ୍ଠି::addNextNull(bodyp, assignp);
            ଯଦି (asyncDisable) {
                clearp = Ast_ଗଣ୍ଠି::addNextNull(
                    clearp,
                    ନୂତନ AstAssignDly{
                        c.flp, ନୂତନ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE},
                        ନୂତନ AstConst{c.flp, AstConst::BitFalse{}}});
            }
        }
        ଯଦି (bodyp) addStateAlways(c, bodyp, clearp);
    }

    // Phase 2b: In-place ring storage and registered ring metadata.
    ଶୂନ୍ୟ emitDelayRingUpdates(LowerCtx& c, ଅଚଳ ତାର୍କିକ ଗଣନା_Matches) {
        ଅଚଳ ତାର୍କିକ asyncDisable = isAsyncDisable(c.disableExprp);
        ପାଇଁ (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ଅଚଳ vtxp = static_cast<SvaStateVertex*>(&vtxr);
            ଯଦି (!vtxp->datap()->delayRingVarp) ଜାରି;
            AstVar* ଅଚଳ idxp = vtxp->datap()->delayRingIdxVarp;
            AstVar* ଅଚଳ live_ଗଣନା_Varp = vtxp->datap()->delay_Ring_Live_ଗଣନା_Varp;
            AstVar* ଅଚଳ wrappedp = vtxp->datap()->delayRingWrappedVarp;
            ଅଚଳ uint32_t size = static_cast<uint32_t>(vtxp->m_delayRingSize);

            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* incomingp = nullptr;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* incomingOverflowp = nullptr;
            ପାଇଁ (ଅଚଳ V3GraphEdge& edger : vtxp->inEdges()) {
                ++m_statDelayRingEdgeVisits;
                ଅଚଳ SvaTransEdge& tedger = static_cast<ଅଚଳ SvaTransEdge&>(edger);
                UASSERT_OBJ(tedger.m_consumesCycle == vtxp->m_isFixedDelayRing, vtxp,
                            "Delay-ring incoming edge kind mismatch");
                ଅଚଳ SvaStateVertex* ଅଚଳ fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Delay-ring incoming source missing stateSig");
                UASSERT_OBJ(!ଗଣନା_Matches || fromVtxp->datap()->state_ଗଣନା_Sigp, fromVtxp,
                            "Delay-ring incoming source missing stateCountSig");
                // IEEE 1800-2023 16.14.1/16.14.3: assertion alternatives belong to one
                // evaluation; only cover sequence counts each end-of-match.
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* contribp
                    = ଗଣନା_Matches
                          ? fromVtxp->datap()->state_ଗଣନା_Sigp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)
                          : ନୂତନ AstExtend{c.flp,
                                          fromVtxp->datap()->stateSigp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                                          m_u32DTypep->width()};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* contribOverflowp
                    = ଗଣନା_Matches && fromVtxp->datap()->state_ଗଣନା_Overflow_Sigp
                          ? fromVtxp->datap()->state_ଗଣନା_Overflow_Sigp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)
                          : nullptr;
                ଯଦି (tedger.m_condp) {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enablep = tedger.m_condp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                    contribp = ଗଣନା_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, enablep)
                                   : addCountp(c.flp, nullptr, contribp, enablep);
                }
                ଯଦି (c.disableExprp) {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ notDisp
                        = ନୂତନ AstLogNot{c.flp, c.disableExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                    contribp = ଗଣନା_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, notDisp)
                                   : addCountp(c.flp, nullptr, contribp, notDisp);
                }
                incomingp = ଗଣନା_Matches ? addCheckedCountp(c.flp, incomingp, contribp,
                                                            incomingOverflowp, contribOverflowp)
                                         : addCountp(c.flp, incomingp, contribp);
            }
            UASSERT_OBJ(incomingp, vtxp, "Delay ring has no incoming edge");
            ଅଚଳ ତାର୍କିକ storesCounts = vtxp->datap()->delayRingCounts;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ incomingCountp
                = storesCounts
                      ? incomingp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)
                      : ନୂତନ AstExtend{c.flp,
                                      ନୂତନ AstNeq{c.flp, incomingp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                                                 newTypedConstp(c.flp, incomingp->dtypep(), 0)},
                                      m_u32DTypep->width()};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ storedIncomingp
                = storesCounts ? incomingp
                               : ନୂତନ AstNeq{c.flp, incomingp,
                                            newTypedConstp(c.flp, incomingp->dtypep(), 0)};
            // Only this block accesses the storage. Other processes read the cached head, so
            // the ring can be updated in place without an NBA shadow copy.
            AstAssign* ଅଚଳ writeIncomingp = ନୂତନ AstAssign{
                c.flp,
                delayRingSlot(c.flp, vtxp, ନୂତନ AstVarRef{c.flp, idxp, VAccess::READ},
                              VAccess::WRITE),
                storedIncomingp};
            Ast_ଗଣ୍ଠି* updateBodyp = writeIncomingp;
            // live_count <= live_count - outgoing_count + incoming_count;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ outgoingp = delay_Ring_Output_ଗଣନା(c.flp, vtxp);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ afterOutgoingp
                = ନୂତନ AstSub{c.flp, ନୂତନ AstVarRef{c.flp, live_ଗଣନା_Varp, VAccess::READ}, outgoingp};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ oldAfterOutgoingp = afterOutgoingp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            AstAdd* ଅଚଳ nextLiveCountp = ନୂତନ AstAdd{c.flp, afterOutgoingp, incomingCountp};
            ଯଦି (storesCounts) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ liveOverflowp
                    = ନୂତନ AstLt{c.flp, nextLiveCountp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), oldAfterOutgoingp};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ overflowp = orExprs(c.flp, incomingOverflowp, liveOverflowp);
                updateBodyp = new_ଗଣନା_Overflow_Warning(c.flp, overflowp);
                updateBodyp->addNext(writeIncomingp);
            } ଅନ୍ୟଥା {
                VL_DO_DANGLING(oldAfterOutgoingp->delete_ବୃକ୍ଷ(), oldAfterOutgoingp);
            }
            // Cache the outgoing slot for the next index after the write. Reading after
            // writing also handles one-slot rings and two-slot range rings without alias cases.
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nextHeadIdxp
                = nextRingIndex(c.flp, ନୂତନ AstVarRef{c.flp, idxp, VAccess::READ}, size);
            ଯଦି (!vtxp->m_isFixedDelayRing) {
                nextHeadIdxp = nextRingIndex(c.flp, nextHeadIdxp, size);
            }
            updateBodyp->addNext(ନୂତନ AstAssignDly{
                c.flp, ନୂତନ AstVarRef{c.flp, vtxp->datap()->delayRingHeadVarp, VAccess::WRITE},
                delayRingSlot(c.flp, vtxp, nextHeadIdxp)});
            updateBodyp->addNext(ନୂତନ AstAssignDly{
                c.flp, ନୂତନ AstVarRef{c.flp, live_ଗଣନା_Varp, VAccess::WRITE}, nextLiveCountp});
            // wrapped <= wrapped || idx == size - 1;
            updateBodyp->addNext(
                ନୂତନ AstAssignDly{c.flp, ନୂତନ AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                 ନୂତନ AstLogOr{c.flp, ନୂତନ AstVarRef{c.flp, wrappedp, VAccess::READ},
                                              delayRingAtLastIndex(c.flp, idxp, size)}});
            // idx <= next_idx;
            updateBodyp->addNext(ନୂତନ AstAssignDly{
                c.flp, ନୂତନ AstVarRef{c.flp, idxp, VAccess::WRITE},
                nextRingIndex(c.flp, ନୂତନ AstVarRef{c.flp, idxp, VAccess::READ}, size)});
            ଯଦି (vtxp->m_delayRingAdvanceCondp) {
                updateBodyp = ନୂତନ AstIf{
                    c.flp, sampled(vtxp->m_delayRingAdvanceCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)),
                    updateBodyp};
            }

            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* clearCondp = killActive(c);
            ଯଦି (vtxp->m_delayRingClearCondp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_delayRingClearCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)));
            }
            ଯଦି (vtxp->m_abortClearp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_abortClearp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)));
            }
            ଯଦି (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
            }
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* guardp = nullptr;
            ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ cp : vtxp->m_throughoutConds) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ sampledp = sampled(cp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
                guardp = guardp ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstLogAnd{c.flp, guardp, sampledp})
                                : sampledp;
            }
            ଯଦି (guardp) clearCondp = orExprs(c.flp, clearCondp, ନୂତନ AstLogNot{c.flp, guardp});
            // Logically clear the ring without touching its wide storage.
            AstAssignDly* ଅଚଳ clearCountp
                = ନୂତନ AstAssignDly{c.flp, ନୂତନ AstVarRef{c.flp, live_ଗଣନା_Varp, VAccess::WRITE},
                                   newTypedConstp(c.flp, live_ଗଣନା_Varp->dtypep(), 0)};
            clearCountp->addNext(ନୂତନ AstAssignDly{c.flp,
                                                  ନୂତନ AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                                  ନୂତନ AstConst{c.flp, AstConst::BitFalse{}}});
            clearCountp->addNext(ନୂତନ AstAssignDly{c.flp,
                                                  ନୂତନ AstVarRef{c.flp, idxp, VAccess::WRITE},
                                                  newTypedConstp(c.flp, idxp->dtypep(), 0)});
            updateBodyp = ନୂତନ AstIf{c.flp, clearCondp, clearCountp, updateBodyp};

            addStateAlways(c, updateBodyp, asyncDisable ? clearCountp->clone_ବୃକ୍ଷ(ସତ୍ୟ) : nullptr);
        }
    }

    // Phase 2c: SAnd combiner done-latch always block.
    // NBA semantics ensure doneL/doneR read pre-update values (IEEE 16.9.5).
    ଶୂନ୍ୟ emitAndCombinerDoneLatchNba(LowerCtx& c) {
        ଅଚଳ ତାର୍କିକ asyncDisable = isAsyncDisable(c.disableExprp);
        ପାଇଁ (ଅଚଳ V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ଅଚଳ SvaStateVertex* ଅଚଳ avp = static_cast<ଅଚଳ SvaStateVertex*>(&vtxr);
            ଯଦି (!avp->datap()->doneLVarp) ଜାରି;
            // doneLVars is non-null only for AndCombiner vertices, which always
            // have both m_andLhsTermp and m_andRhsTermp set at build time.
            UASSERT_OBJ(avp->m_andLhsTermp && avp->m_andRhsTermp, avp,
                        "AndCombiner vertex missing LHS/RHS terminal");
            ଅଚଳ SvaStateVertex* ଅଚଳ lhsTermp = avp->m_andLhsTermp;
            ଅଚଳ SvaStateVertex* ଅଚଳ rhsTermp = avp->m_andRhsTermp;
            // resolveLinks' 2*N+2 fixed-point pass is guaranteed to populate
            // stateSigp on every AndCombiner and its LHS/RHS terminals.
            UASSERT_OBJ(lhsTermp->datap()->stateSigp && rhsTermp->datap()->stateSigp
                            && avp->datap()->stateSigp,
                        avp, "AndCombiner stateSigp chain unresolved after resolveLinks");

            AstAssignDly* ଅଚଳ clearLp = ନୂତନ AstAssignDly{
                c.flp, ନୂତନ AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                ନୂତନ AstConst{c.flp, AstConst::BitFalse{}}};
            AstAssignDly* ଅଚଳ clearRp = ନୂତନ AstAssignDly{
                c.flp, ନୂତନ AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                ନୂତନ AstConst{c.flp, AstConst::BitFalse{}}};
            clearLp->addNext(clearRp);

            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ matchLNowp
                = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, avp->m_andLhsCondp);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ matchRNowp
                = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, avp->m_andRhsCondp);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* gateLp = matchLNowp;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* gateRp = matchRNowp;
            ଯଦି (c.disableExprp) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ notDisLp
                    = ନୂତନ AstLogNot{c.flp, c.disableExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                gateLp = ନୂତନ AstLogAnd{c.flp, gateLp, notDisLp};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ notDisRp
                    = ନୂତନ AstLogNot{c.flp, c.disableExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                gateRp = ନୂତନ AstLogAnd{c.flp, gateRp, notDisRp};
            }
            AstAssignDly* ଅଚଳ setLp = ନୂତନ AstAssignDly{
                c.flp, ନୂତନ AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                ନୂତନ AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* ଅଚଳ setLIfp = ନୂତନ AstIf{c.flp, gateLp, setLp, nullptr};
            AstAssignDly* ଅଚଳ setRp = ନୂତନ AstAssignDly{
                c.flp, ନୂତନ AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                ନୂତନ AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* ଅଚଳ setRIfp = ନୂତନ AstIf{c.flp, gateRp, setRp, nullptr};
            setLIfp->addNext(setRIfp);

            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* clearCondp = ନୂତନ AstLogOr{c.flp, killActive(c),
                                                   avp->datap()->stateSigp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
            ଯଦି (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
            }
            AstIf* ଅଚଳ topp = ନୂତନ AstIf{c.flp, clearCondp, clearLp, setLIfp};
            addStateAlways(c, topp, asyncDisable ? clearLp->clone_ବୃକ୍ଷ(ସତ୍ୟ) : nullptr);
        }
    }

    ଶୂନ୍ୟ emitKillAckNba(LowerCtx& c) {
        AstAssignDly* ଅଚଳ ackp
            = ନୂତନ AstAssignDly{c.flp, ନୂତନ AstVarRef{c.flp, c.killVarp, VAccess::WRITE},
                               assertKillGet(c.flp, c.assertType, c.directiveType)};
        m_modp->addStmtsp(
            ନୂତନ AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), ackp});
    }

    // Phase 3/3a/3b: Compute terminal match/reject signals, required-step reject,
    // throughout-drop reject; clean up intermediate state signals.
    // Phase 3: terminalActive and rejectBase from Links to matchVertex.
    // Builder only adds Links (non-clocked) to matchVertex via addLink in
    // wireMatchAndMidSources. For cover sequence, also count each end-of-match
    // so the action can be replayed without unrolling endpoints.
    ଶୂନ୍ୟ computeTerminalMatchAndReject(LowerCtx& c, SignalSet& sigs, ଅଚଳ ତାର୍କିକ need_Match_ଗଣନା) {
        ପାଇଁ (ଅଚଳ SvaTransEdge* ଅଚଳ tedgep : c.graph.allEdges()) {
            ଯଦି (tedgep->toVtxp() != c.graph.m_matchVertexp) ଜାରି;
            SvaStateVertex* ଅଚଳ fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                        "Terminal-link source missing stateSig");

            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* srcSigp = fromVtxp->datap()->stateSigp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            srcSigp = andCond(c.flp, srcSigp, tedgep->m_condp);
            ଯଦି (need_Match_ଗଣନା) {
                UASSERT_OBJ(fromVtxp->datap()->state_ଗଣନା_Sigp, fromVtxp,
                            "Terminal-link source missing stateCountSig");
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* contributionp
                    = fromVtxp->datap()->state_ଗଣନା_Sigp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* contributionOverflowp
                    = fromVtxp->datap()->state_ଗଣନା_Overflow_Sigp
                          ? fromVtxp->datap()->state_ଗଣନା_Overflow_Sigp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)
                          : nullptr;
                ଯଦି (tedgep->m_condp) {
                    contributionp = gateCheckedCountp(c.flp, contributionp, contributionOverflowp,
                                                      tedgep->m_condp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
                }
                sigs.matchCountp
                    = addCheckedCountp(c.flp, sigs.matchCountp, contributionp,
                                       sigs.match_ଗଣନା_Overflowp, contributionOverflowp);
            }

            ଯଦି (fromVtxp->m_delayRingSize && !fromVtxp->m_isFixedDelayRing) {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
                // reject |= (wrapped || idx == size - 1) && head && final_condition;
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* expireContribp = delay_Ring_Output_ଗଣନା(c.flp, fromVtxp);
                expireContribp = andCond(c.flp, expireContribp, tedgep->m_condp);
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, expireContribp);
                VL_DO_DANGLING(srcSigp->delete_ବୃକ୍ଷ(), srcSigp);
            } ଅନ୍ୟଥା ଯଦି (fromVtxp->m_isUnbounded || fromVtxp->m_isAndCombiner) {
                sigs.terminalActivep = orExprs(c.flp, sigs.terminalActivep, srcSigp);
            } ଅନ୍ୟଥା {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, srcSigp);
            }
        }
        // wireMatchAndMidSources always adds a Link from result.termVertexp
        // to m_matchVertexp, so the loop above always sets terminalActivep.
        UASSERT_OBJ(sigs.terminalActivep, c.graph.m_matchVertexp,
                    "No terminal edge to match vertex");
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newThroughoutThreadFailCountp(LowerCtx& c, AstVar* ଅଚଳ delay_Ring_Live_ଗଣନା_Varp,
                                               Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ stateExprp,
                                               Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enablep) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ activeThreadCountp
            = delay_Ring_Live_ଗଣନା_Varp
                  ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(
                        ନୂତନ AstVarRef{c.flp, delay_Ring_Live_ଗଣନା_Varp, VAccess::READ})
                  : ନୂତନ AstExtend{c.flp, stateExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), m_u32DTypep->width()};
        ଫେରନ୍ତୁ addCountp(c.flp, nullptr, activeThreadCountp, enablep);
    }

    // Phase 3b: Throughout-drop and ring-wide abort rejection.
    ଶୂନ୍ୟ computeThroughoutReject(LowerCtx& c, SignalSet& sigs, ଅଚଳ ତାର୍କିକ need_Thread_Fail_ଗଣନା) {
        ପାଇଁ (ଅଚଳ V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ଅଚଳ SvaStateVertex* ଅଚଳ vtxp = static_cast<ଅଚଳ SvaStateVertex*>(&vtxr);
            ଅଚଳ auto& conds = vtxp->m_throughoutConds;
            ଯଦି (conds.empty() && !vtxp->m_abortRejectp) ଜାରି;
            ଯଦି (vtxp->m_isAndCombiner) ଜାରି;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* stateExprp = nullptr;
            ଯଦି (vtxp->datap()->stateVarp) {
                stateExprp = ନୂତନ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
            } ଅନ୍ୟଥା ଯଦି (vtxp->datap()->delayRingVarp && vtxp->m_isFixedDelayRing) {
                stateExprp
                    = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_ଗଣନା_Varp);
            } ଅନ୍ୟଥା {
                UASSERT_OBJ(vtxp->datap()->stateSigp, vtxp,
                            "Throughout-conds vertex missing state representation");
                stateExprp = vtxp->datap()->stateSigp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            }
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* guardp = nullptr;
            ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ cp : conds) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ sp = sampled(cp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
                guardp = guardp ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstLogAnd{c.flp, guardp, sp}) : sp;
            }
            ଯଦି (vtxp->m_abortRejectp) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ notAbortp
                    = ନୂତନ AstLogNot{c.flp, sampled(vtxp->m_abortRejectp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା))};
                guardp = guardp
                             ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstLogAnd{c.flp, guardp, notAbortp})
                             : notAbortp;
            }
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rejectCondp = ନୂତନ AstLogNot{c.flp, guardp};
            ଯଦି (vtxp->m_abortClearp) {
                // Any abort clears the ring. Accept aborts suppress a simultaneous guard failure;
                // reject aborts are restored below so they still force rejection.
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ notAbortClearp
                    = ନୂତନ AstLogNot{c.flp, sampled(vtxp->m_abortClearp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା))};
                rejectCondp = ନୂତନ AstLogAnd{c.flp, rejectCondp, notAbortClearp};
                ଯଦି (vtxp->m_abortRejectp) {
                    rejectCondp = ନୂତନ AstLogOr{
                        c.flp, rejectCondp, sampled(vtxp->m_abortRejectp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା))};
                }
            }
            ଯଦି (need_Thread_Fail_ଗଣନା) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ contributionp
                    = newThroughoutThreadFailCountp(c, vtxp->datap()->delay_Ring_Live_ଗଣନା_Varp,
                                                    stateExprp, rejectCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
                sigs.threadFailCountp = addCountp(c.flp, sigs.threadFailCountp, contributionp);
                ଯଦି (vtxp->m_abortRejectp && vtxp->m_delayRingAdvanceCondp) {
                    // An advancing ring thread also occupies its same-tick match vertex in the
                    // unrolled NFA, so abort rejection must replay both fail actions.
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ abortAndAdvancep = ନୂତନ AstLogAnd{
                        c.flp, sampled(vtxp->m_abortRejectp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)),
                        sampled(vtxp->m_delayRingAdvanceCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା))};
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ matchContributionp = newThroughoutThreadFailCountp(
                        c, vtxp->datap()->delay_Ring_Live_ଗଣନା_Varp, stateExprp, abortAndAdvancep);
                    sigs.threadFailCountp
                        = addCountp(c.flp, sigs.threadFailCountp, matchContributionp);
                }
            }
            sigs.throughoutRejectp = orExprs(c.flp, sigs.throughoutRejectp,
                                             ନୂତନ AstLogAnd{c.flp, stateExprp, rejectCondp});
        }
    }

    SignalSet computeSignals(LowerCtx& c, ଅଚଳ ତାର୍କିକ need_Thread_Fail_ଗଣନା,
                             ଅଚଳ ତାର୍କିକ need_Throughout_Thread_Fail_ଗଣନା, ଅଚଳ ତାର୍କିକ need_Match_ଗଣନା) {
        SignalSet sigs;

        computeTerminalMatchAndReject(c, sigs, need_Match_ଗଣନା);

        // Phase 3a: required-step rejection.
        // Builder only sets m_rejectOnFail on non-clocked Links with m_condp
        // or m_condVtxp, and the source always has a resolved stateSig.
        ପାଇଁ (ଅଚଳ SvaTransEdge* ଅଚଳ tedgep : c.graph.allEdges()) {
            ଯଦି (!tedgep->m_rejectOnFail) ଜାରି;
            ଅଚଳ SvaStateVertex* ଅଚଳ fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp && (tedgep->m_condp || tedgep->m_condVtxp),
                        fromVtxp,
                        "rejectOnFail Link must have condp/condVtxp and source stateSig");
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ srcSigp = fromVtxp->datap()->stateSigp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp = nullptr;
            ଯଦି (tedgep->m_condVtxp) {
                UASSERT_OBJ(tedgep->m_condVtxp->datap()->stateSigp, tedgep->m_condVtxp,
                            "rejectOnFail condVtxp missing stateSig");
                condp = tedgep->m_condVtxp->datap()->stateSigp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                ଯଦି (tedgep->m_condp) {
                    condp = ନୂତନ AstLogOr{c.flp, condp, tedgep->m_condp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                }
            } ଅନ୍ୟଥା {
                condp = tedgep->m_condp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            }
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ notCondp = ନୂତନ AstLogNot{c.flp, condp};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rawFailp = ନୂତନ AstLogAnd{c.flp, srcSigp, notCondp};
            ଯଦି (need_Thread_Fail_ଗଣନା) {
                // thread_fail_count += fail;
                sigs.threadFailCountp = addCountp(
                    c.flp, sigs.threadFailCountp,
                    ନୂତନ AstExtend{c.flp, rawFailp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), m_u32DTypep->width()});
            }
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ failp = gateNotKill(c, rawFailp);
            sigs.requiredStepRejectp = orExprs(c.flp, sigs.requiredStepRejectp, failp);
        }

        computeThroughoutReject(c, sigs, need_Throughout_Thread_Fail_ଗଣନା);
        ଯଦି (sigs.threadFailCountp) {
            sigs.threadFailCountp
                = addCountp(c.flp, nullptr, sigs.threadFailCountp, notKillActive(c));
        }
        ଯଦି (sigs.matchCountp) {
            ଯଦି (c.matchCondp) {
                sigs.matchCountp
                    = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_ଗଣନା_Overflowp,
                                        sampled(c.matchCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)));
            }
            sigs.matchCountp = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_ଗଣନା_Overflowp,
                                                 notKillActive(c));
        }
        sigs.terminalActivep = gateNotKill(c, sigs.terminalActivep);
        sigs.rejectBasep = gateNotKill(c, sigs.rejectBasep);
        sigs.throughoutRejectp = gateNotKill(c, sigs.throughoutRejectp);

        // Clean up intermediate state signals. These are orphan subtrees
        // (never linked into the enclosing AST); deleteTree() is immediate
        // which is what we want since stateSigp lifetime ends with this scope.
        ପାଇଁ (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ଅଚଳ vtxp = static_cast<SvaStateVertex*>(&vtxr);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*& sigp = vtxp->datap()->stateSigp;
            ଯଦି (sigp) VL_DO_DANGLING(sigp->delete_ବୃକ୍ଷ(), sigp);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*& countp = vtxp->datap()->state_ଗଣନା_Sigp;
            ଯଦି (countp) VL_DO_DANGLING(countp->delete_ବୃକ୍ଷ(), countp);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*& overflowp = vtxp->datap()->state_ଗଣନା_Overflow_Sigp;
            ଯଦି (overflowp) VL_DO_DANGLING(overflowp->delete_ବୃକ୍ଷ(), overflowp);
        }
        // Disable iff gating (IEEE 1800-2023 16.12) on the current level value.
        ଯଦି (c.disableExprp) {
            // terminalActivep is always set, so gate it unconditionally.
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ notTermp
                = ନୂତନ AstLogNot{c.flp, c.disableExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
            sigs.terminalActivep = ନୂତନ AstLogAnd{c.flp, sigs.terminalActivep, notTermp};
            ଯଦି (sigs.rejectBasep) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ notDisp
                    = ନୂତନ AstLogNot{c.flp, c.disableExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                sigs.rejectBasep = ନୂତନ AstLogAnd{c.flp, sigs.rejectBasep, notDisp};
            }
            ଯଦି (sigs.throughoutRejectp) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ notDisp
                    = ନୂତନ AstLogNot{c.flp, c.disableExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                sigs.throughoutRejectp = ନୂତନ AstLogAnd{c.flp, sigs.throughoutRejectp, notDisp};
            }
            ଯଦି (sigs.requiredStepRejectp) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ notDisp
                    = ନୂତନ AstLogNot{c.flp, c.disableExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                sigs.requiredStepRejectp = ନୂତନ AstLogAnd{c.flp, sigs.requiredStepRejectp, notDisp};
            }
            ଯଦି (sigs.matchCountp) {
                sigs.matchCountp = gateCheckedCountp(
                    c.flp, sigs.matchCountp, sigs.match_ଗଣନା_Overflowp,
                    ନୂତନ AstLogNot{c.flp, c.disableExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)});
            }
        }

        ଯଦି (sigs.match_ଗଣନା_Overflowp) {
            m_modp->addStmtsp(
                ନୂତନ AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା),
                              new_ଗଣନା_Overflow_Warning(c.flp, sigs.match_ଗଣନା_Overflowp)});
            sigs.match_ଗଣନା_Overflowp = nullptr;
        }

        ଫେରନ୍ତୁ sigs;
    }

    // Phase 1: Resolve combinational Links via fixed-point propagation.
    ଶୂନ୍ୟ resolveLinks(LowerCtx& c, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* triggerExprp, ଅଚଳ ତାର୍କିକ ଗଣନା_Matches) {
        // datap() was freshly allocated in lower() -- all state signals start null.
        c.graph.m_startVertexp->datap()->stateSigp = triggerExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        c.graph.m_startVertexp->datap()->state_ଗଣନା_Sigp
            = ନୂତନ AstExtend{c.flp, triggerExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), m_u32DTypep->width()};
        ପାଇଁ (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ଅଚଳ vtxp = static_cast<SvaStateVertex*>(&vtxr);
            ଯଦି (vtxp->datap()->stateVarp) {
                vtxp->datap()->stateSigp
                    = ନୂତନ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
                vtxp->datap()->state_ଗଣନା_Sigp = ନୂତନ AstExtend{
                    c.flp, ନୂତନ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ},
                    m_u32DTypep->width()};
            } ଅନ୍ୟଥା ଯଦି (vtxp->datap()->delayRingVarp) {
                ଯଦି (vtxp->m_isFixedDelayRing) {
                    // state_count = wrapped ? head : 0; state = state_count != 0;
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ countp = delay_Ring_Output_ଗଣନା(c.flp, vtxp);
                    vtxp->datap()->stateSigp = ନୂତନ AstNeq{c.flp, countp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                                                          newTypedConstp(c.flp, m_u32DTypep, 0)};
                    vtxp->datap()->state_ଗଣନା_Sigp = countp;
                } ଅନ୍ୟଥା {
                    vtxp->datap()->stateSigp
                        = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_ଗଣନା_Varp);
                    vtxp->datap()->state_ଗଣନା_Sigp = ନୂତନ AstVarRef{
                        c.flp, vtxp->datap()->delay_Ring_Live_ଗଣନା_Varp, VAccess::READ};
                }
            }
        }
        // Fixed-point propagation along zero-delay (Link) edges. Rebuild each
        // derived signal from its incoming edges on every pass; appending the same
        // contributions repeatedly makes merge expressions grow exponentially.
        ଅଚଳ size_t passes = 2 * c.graph.m_graph.vertices().size() + 2;
        ପାଇଁ (size_t pass = 0; pass < passes; ++pass) {
            // Rebuild SAnd combiners once both sub-NFA terminals are available.
            ପାଇଁ (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* ଅଚଳ vtxp = static_cast<SvaStateVertex*>(&vtxr);
                ଯଦି (!vtxp->m_isAndCombiner) ଜାରି;
                // AndCombiner vertices always have both terminal pointers set.
                UASSERT_OBJ(vtxp->m_andLhsTermp && vtxp->m_andRhsTermp, vtxp,
                            "AndCombiner vertex missing LHS/RHS terminal");
                ଅଚଳ SvaStateVertex* ଅଚଳ lhsTermp = vtxp->m_andLhsTermp;
                ଅଚଳ SvaStateVertex* ଅଚଳ rhsTermp = vtxp->m_andRhsTermp;
                ଯଦି (!lhsTermp->datap()->stateSigp || !rhsTermp->datap()->stateSigp) ଜାରି;
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ matchLp
                    = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, vtxp->m_andLhsCondp);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ matchRp
                    = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, vtxp->m_andRhsCondp);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ doneLOrp = ନୂତନ AstLogOr{
                    c.flp, ନୂତନ AstVarRef{c.flp, vtxp->datap()->doneLVarp, VAccess::READ}, matchLp};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ doneROrp = ନୂତନ AstLogOr{
                    c.flp, ନୂତନ AstVarRef{c.flp, vtxp->datap()->doneRVarp, VAccess::READ}, matchRp};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ bothp = ନୂତନ AstLogAnd{c.flp, doneLOrp, doneROrp};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ oneNowp = ନୂତନ AstLogOr{c.flp, matchLp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                                                          matchRp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                ଯଦି (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_ବୃକ୍ଷ(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = ନୂତନ AstLogAnd{c.flp, bothp, oneNowp};
                ଯଦି (vtxp->datap()->state_ଗଣନା_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_ଗଣନା_Sigp->delete_ବୃକ୍ଷ(),
                                   vtxp->datap()->state_ଗଣନା_Sigp);
                }
                vtxp->datap()->state_ଗଣନା_Sigp = ନୂତନ AstExtend{
                    c.flp, vtxp->datap()->stateSigp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), m_u32DTypep->width()};
            }

            ପାଇଁ (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* ଅଚଳ vtxp = static_cast<SvaStateVertex*>(&vtxr);
                ଯଦି (vtxp == c.graph.m_startVertexp || vtxp->datap()->stateVarp
                    || vtxp->datap()->delayRingVarp || vtxp->m_isAndCombiner || vtxp->m_isMatch
                    || vtxp->m_isRejectSink) {
                    ଜାରି;
                }
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nextStatep = nullptr;
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nextStateCountp = nullptr;
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* next_State_ଗଣନା_Overflowp = nullptr;
                ପାଇଁ (ଅଚଳ V3GraphEdge& er : vtxp->inEdges()) {
                    ଅଚଳ SvaTransEdge& te = static_cast<ଅଚଳ SvaTransEdge&>(er);
                    ଅଚଳ SvaStateVertex* ଅଚଳ fromVtxp = te.fromVtxp();
                    ଯଦି (!fromVtxp->datap()->stateSigp) ଜାରି;
                    UASSERT_OBJ(fromVtxp->datap()->state_ଗଣନା_Sigp, fromVtxp,
                                "Link source missing stateCountSig");
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ contributionp = andCond(
                        c.flp, fromVtxp->datap()->stateSigp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), te.m_condp);
                    nextStatep = orExprs(c.flp, nextStatep, contributionp);
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଗଣନା_Contributionp
                        = fromVtxp->datap()->state_ଗଣନା_Sigp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଗଣନା_Contribution_Overflowp
                        = ଗଣନା_Matches && fromVtxp->datap()->state_ଗଣନା_Overflow_Sigp
                              ? fromVtxp->datap()->state_ଗଣନା_Overflow_Sigp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)
                              : nullptr;
                    ଯଦି (te.m_condp) {
                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ enablep = te.m_condp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                        ଗଣନା_Contributionp
                            = ଗଣନା_Matches
                                  ? gateCheckedCountp(c.flp, ଗଣନା_Contributionp,
                                                      ଗଣନା_Contribution_Overflowp, enablep)
                                  : addCountp(c.flp, nullptr, ଗଣନା_Contributionp, enablep);
                    }
                    nextStateCountp
                        = ଗଣନା_Matches
                              ? addCheckedCountp(c.flp, nextStateCountp, ଗଣନା_Contributionp,
                                                 next_State_ଗଣନା_Overflowp,
                                                 ଗଣନା_Contribution_Overflowp)
                              : addCountp(c.flp, nextStateCountp, ଗଣନା_Contributionp);
                }
                ଯଦି (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_ବୃକ୍ଷ(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = nextStatep;
                ଯଦି (vtxp->datap()->state_ଗଣନା_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_ଗଣନା_Sigp->delete_ବୃକ୍ଷ(),
                                   vtxp->datap()->state_ଗଣନା_Sigp);
                }
                vtxp->datap()->state_ଗଣନା_Sigp = nextStateCountp;
                ଯଦି (vtxp->datap()->state_ଗଣନା_Overflow_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_ଗଣନା_Overflow_Sigp->delete_ବୃକ୍ଷ(),
                                   vtxp->datap()->state_ଗଣନା_Overflow_Sigp);
                }
                vtxp->datap()->state_ଗଣନା_Overflow_Sigp = next_State_ଗଣନା_Overflowp;
            }
        }
    }

ସର୍ବସାଧାରଣ:
    // Combine terminal/reject signals into final output expression.
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* assembleResult(Ast_ଗଣ୍ଠି_Cover_Or_Assert* ଅଚଳ assertp, ଅଚଳ ତାର୍କିକ negated,
                                       Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ matchCondp, ଅଚଳ SignalSet& sigs,
                                       Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି** ଅଚଳ outMatchpp) {
        File_ଧାଡ଼ି* ଅଚଳ flp = assertp->fileline();
        ଅଚଳ ତାର୍କିକ isCover = VN_IS(assertp, Cover);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ terminalActivep = sigs.terminalActivep;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rejectBasep = sigs.rejectBasep;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ throughoutRejectp = sigs.throughoutRejectp;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ requiredStepRejectp = sigs.requiredStepRejectp;
        // Property negation (IEEE 1800-2023 16.12.1 `not`): invert match/reject.
        ଯଦି (negated) {
            ଯଦି (isCover) {
                ଯଦି (terminalActivep)
                    VL_DO_DANGLING(terminalActivep->delete_ବୃକ୍ଷ(), terminalActivep);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* negRejectp = nullptr;
                ଯଦି (matchCondp && rejectBasep) {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ sampledCondp = sampled(matchCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ notCondp = ନୂତନ AstLogNot{flp, sampledCondp};
                    negRejectp = ନୂତନ AstLogAnd{flp, rejectBasep, notCondp};
                } ଅନ୍ୟଥା ଯଦି (rejectBasep) {
                    VL_DO_DANGLING(rejectBasep->delete_ବୃକ୍ଷ(), rejectBasep);
                }
                ଯଦି (throughoutRejectp) negRejectp = orExprs(flp, negRejectp, throughoutRejectp);
                ଯଦି (requiredStepRejectp)
                    negRejectp = orExprs(flp, negRejectp, requiredStepRejectp);
                ଫେରନ୍ତୁ negRejectp ? negRejectp : ନୂତନ AstConst{flp, AstConst::BitFalse{}};
            }
            // Negated assert/assume: output = !match.
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* matchp = terminalActivep;
            ଯଦି (matchCondp) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ sampledCondp = sampled(matchCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
                matchp = ନୂତନ AstLogAnd{flp, matchp, sampledCondp};
            }
            ଯଦି (outMatchpp) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* notPMatchp = nullptr;
                ଯଦି (matchCondp && rejectBasep) {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ sampledCondp = sampled(matchCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
                    notPMatchp = ନୂତନ AstLogAnd{flp, rejectBasep->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                                               ନୂତନ AstLogNot{flp, sampledCondp}};
                } ଅନ୍ୟଥା ଯଦି (rejectBasep) {
                    notPMatchp = rejectBasep->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                }
                ଯଦି (throughoutRejectp)
                    notPMatchp = orExprs(flp, notPMatchp, throughoutRejectp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
                ଯଦି (requiredStepRejectp)
                    notPMatchp
                        = orExprs(flp, notPMatchp, requiredStepRejectp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
                *outMatchpp = notPMatchp;
            }
            ଯଦି (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_ବୃକ୍ଷ(), throughoutRejectp);
            ଯଦି (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_ବୃକ୍ଷ(), rejectBasep);
            ଯଦି (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_ବୃକ୍ଷ(), requiredStepRejectp);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ resultExprp = ନୂତନ AstLogNot{flp, matchp};
            ଫେରନ୍ତୁ resultExprp;
        }
        ଯଦି (isCover) {
            ଯଦି (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_ବୃକ୍ଷ(), throughoutRejectp);
            ଯଦି (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_ବୃକ୍ଷ(), rejectBasep);
            ଯଦି (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_ବୃକ୍ଷ(), requiredStepRejectp);
            ଯଦି (matchCondp) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ sampledCondp = sampled(matchCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
                ଫେରନ୍ତୁ ନୂତନ AstLogAnd{flp, terminalActivep, sampledCondp};
            }
            ଫେରନ୍ତୁ terminalActivep;
        }
        // Assert/assume: output = !reject
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rejectp = nullptr;
        ଯଦି (matchCondp && rejectBasep) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ sampledCondp = sampled(matchCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
            rejectp = ନୂତନ AstLogAnd{flp, rejectBasep, ନୂତନ AstLogNot{flp, sampledCondp}};
        } ଅନ୍ୟଥା ଯଦି (rejectBasep) {
            VL_DO_DANGLING(rejectBasep->delete_ବୃକ୍ଷ(), rejectBasep);
        }
        ଯଦି (outMatchpp) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* matchExprp = terminalActivep->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            ଯଦି (matchCondp) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ sp = sampled(matchCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
                matchExprp = ନୂତନ AstLogAnd{flp, matchExprp, sp};
            }
            *outMatchpp = matchExprp;
        }
        ଯଦି (terminalActivep) VL_DO_DANGLING(terminalActivep->delete_ବୃକ୍ଷ(), terminalActivep);
        ଯଦି (throughoutRejectp) rejectp = orExprs(flp, rejectp, throughoutRejectp);
        ଯଦି (requiredStepRejectp) rejectp = orExprs(flp, rejectp, requiredStepRejectp);
        ଯଦି (!rejectp) ଫେରନ୍ତୁ ନୂତନ AstConst{flp, AstConst::BitTrue{}};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ resultExprp = ନୂତନ AstLogNot{flp, rejectp};
        ଫେରନ୍ତୁ resultExprp;
    }

    explicit SvaNfaLowering(Ast_ଗଣ୍ଠି_Module* modp)
        : m_modp{modp}
        , m_u32DTypep{modp->findBasicDType(VBasicDTypeKwd::UINT32)} {}
    ~SvaNfaLowering() {
        V3Stats::addStatSum("Assertions, NFA delay ring edge visits", m_statDelayRingEdgeVisits);
    }

    // Lower NFA graph to synthesizable AstAlways blocks and raw result signals.
    // Links are combinational; Edges are registered (NBA).
    SignalSet lower(Ast_ଗଣ୍ଠି_Cover_Or_Assert* ଅଚଳ assertp, SvaGraph& graph,
                    Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ matchCondp,
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ disableExprp, ଅଚଳ ତାର୍କିକ need_Thread_Fail_ଗଣନା,
                    ଅଚଳ ତାର୍କିକ need_Throughout_Thread_Fail_ଗଣନା) {
        File_ଧାଡ଼ି* ଅଚଳ flp = assertp->fileline();
        AstCover* ଅଚଳ coverp = VN_CAST(assertp, Cover);
        ଅଚଳ ତାର୍କିକ isSeqEvent = coverp && coverp->isSeqEvent();
        ଅଚଳ VAssertType assertType
            = isSeqEvent ? VAssertType{VAssertType::INTERNAL} : assertp->userType();
        ଅଚଳ VAssertDirectiveType directiveType
            = isSeqEvent ? VAssertDirectiveType{VAssertDirectiveType::INTERNAL}
                         : assertp->directive();
        ଅଚଳ std::string baseName = m_names.get("");

        // Number vertices with sequential colors for array indexing.
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା N = 0;
        ପାଇଁ (V3GraphVertex& vtxr : graph.m_graph.vertices()) { vtxr.color(N++); }
        std::vector<SvaStateVertex*> vtx(N, nullptr);
        ପାଇଁ (V3GraphVertex& vtxr : graph.m_graph.vertices()) {
            vtx[vtxr.color()] = static_cast<SvaStateVertex*>(&vtxr);
        }

        // Allocate per-vertex lowering data (stored via V3GraphVertex::userp()).
        std::vector<SvaVertexData> vertexData(N);
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < N; ++i) vtx[i]->userp(&vertexData[i]);

        // Identify registered vertices (targets of clocked edges).
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < N; ++i) {
            ପାଇଁ (ଅଚଳ V3GraphEdge& edger : vtx[i]->outEdges()) {
                ଅଚଳ SvaTransEdge& tedger = static_cast<ଅଚଳ SvaTransEdge&>(edger);
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା toIdx = tedger.toVtxp()->color();
                ଯଦି (tedger.m_consumesCycle && tedger.toVtxp() != graph.m_matchVertexp
                    && !tedger.toVtxp()->m_isRejectSink) {
                    vtx[toIdx]->datap()->needsReg = ସତ୍ୟ;
                }
            }
        }

        // A packed range ring can emit several live endpoints without needing per-slot counts.
        // Widen only a ring whose input can already carry multiplicity.
        std::vector<ତାର୍କିକ> stateMayHaveMultiplicity(N);
        std::vector<ତାର୍କିକ> ringNeedsMultiplicity(N);
        std::vector<ପୂର୍ଣ୍ଣସଂଖ୍ୟା> multiplicityWork;
        ଅଚଳ ତାର୍କିକ ଗଣନା_Matches = coverp && coverp->isCoverSeq() && !isSeqEvent;
        ଯଦି (ଗଣନା_Matches) {
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < N; ++i) {
                ଅଚଳ ତାର୍କିକ multipleInputs = vtx[i]->inEdges().size() > 1;
                ଅଚଳ ତାର୍କିକ isRing = vtx[i]->m_delayRingSize;
                ringNeedsMultiplicity[i] = isRing && multipleInputs;
                ଅଚଳ ତାର୍କିକ startsMultiplicity = isRing
                                                    ? !vtx[i]->m_isFixedDelayRing
                                                    : multipleInputs && !vtx[i]->datap()->needsReg;
                ଯଦି (startsMultiplicity) {
                    stateMayHaveMultiplicity[i] = ସତ୍ୟ;
                    multiplicityWork.ଠେଲ_back(i);
                }
            }
            ପାଇଁ (size_t wi = 0; wi < multiplicityWork.size(); ++wi) {
                ପାଇଁ (ଅଚଳ V3GraphEdge& edger : vtx[multiplicityWork[wi]]->outEdges()) {
                    ଅଚଳ SvaTransEdge& tedger = static_cast<ଅଚଳ SvaTransEdge&>(edger);
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ti = tedger.toVtxp()->color();
                    ଅଚଳ ତାର୍କିକ isRing = vtx[ti]->m_delayRingSize;
                    ଯଦି (isRing) ringNeedsMultiplicity[ti] = ସତ୍ୟ;
                    // Match and reject sinks have no outgoing edges, so they stop naturally.
                    ଅଚଳ ତାର୍କିକ stopsMultiplicity = !isRing && vtx[ti]->datap()->needsReg;
                    ଯଦି (stateMayHaveMultiplicity[ti] || stopsMultiplicity) { ଜାରି; }
                    stateMayHaveMultiplicity[ti] = ସତ୍ୟ;
                    multiplicityWork.ଠେଲ_back(ti);
                }
            }
        }

        ଅଚଳ ତାର୍କିକ asyncDisable = isAsyncDisable(disableExprp);
        // Registers cleared on a rising disable are also written by their clocked block.
        File_ଧାଡ଼ି* ଅଚଳ regFlp = asyncDisable ? ନୂତନ File_ଧାଡ଼ି{flp} : flp;
        ଯଦି (asyncDisable) regFlp->warnOff(V3ErrorCode::MULTIDRIVEN, ସତ୍ୟ);

        AstVar* ଅଚଳ killVarp
            = ନୂତନ AstVar{flp, VVarType::MODULETEMP, baseName + "__kill", m_u32DTypep};
        killVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(killVarp);
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < N; ++i) {
            ଯଦି (vtx[i]->m_isAndCombiner) {
                ଅଚଳ std::string base = baseName + "__a" + std::to_string(i);
                AstVar* ଅଚଳ lp = ନୂତନ AstVar{regFlp, VVarType::MODULETEMP, base + "_doneL",
                                              m_modp->findBitDType()};
                lp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(lp);
                vtx[i]->datap()->doneLVarp = lp;
                AstVar* ଅଚଳ rp = ନୂତନ AstVar{regFlp, VVarType::MODULETEMP, base + "_doneR",
                                              m_modp->findBitDType()};
                rp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(rp);
                vtx[i]->datap()->doneRVarp = rp;
                ଜାରି;
            }
            ଯଦି (vtx[i]->m_delayRingSize) {
                ଅଚଳ std::string base = baseName + "__d" + std::to_string(i);
                ଅଚଳ ତାର୍କିକ storesCounts = ଗଣନା_Matches && ringNeedsMultiplicity[i];
                Ast_ଗଣ୍ଠି_DType* ringDTypep = nullptr;
                ଯଦି (storesCounts) {
                    // int unsigned ring [size];
                    ringDTypep = ନୂତନ AstUnpackArrayDType{
                        flp, m_u32DTypep,
                        ନୂତନ AstRange{flp, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(vtx[i]->m_delayRingSize - 1), 0}};
                    v3Global.rootp()->typeTablep()->addTypesp(ringDTypep);
                } ଅନ୍ୟଥା {
                    // bit [size-1:0] ring;
                    ringDTypep = m_modp->findBitDType(vtx[i]->m_delayRingSize,
                                                      vtx[i]->m_delayRingSize, VSigning::UNSIGNED);
                }
                AstVar* ଅଚଳ ringp
                    = ନୂତନ AstVar{flp, VVarType::MODULETEMP, base + "_ring", ringDTypep};
                ringp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(ringp);
                vtx[i]->datap()->delayRingVarp = ringp;
                vtx[i]->datap()->delayRingCounts = storesCounts;
                AstVar* ଅଚଳ headp
                    = ନୂତନ AstVar{flp, VVarType::MODULETEMP, base + "_head",
                                 storesCounts ? m_u32DTypep : m_modp->findBitDType()};
                headp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(headp);
                vtx[i]->datap()->delayRingHeadVarp = headp;
                // int unsigned idx;
                AstVar* ଅଚଳ idxp
                    = ନୂତନ AstVar{regFlp, VVarType::MODULETEMP, base + "_idx", m_u32DTypep};
                idxp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(idxp);
                vtx[i]->datap()->delayRingIdxVarp = idxp;
                // int unsigned live_count;
                AstVar* ଅଚଳ live_ଗଣନା_Varp
                    = ନୂତନ AstVar{regFlp, VVarType::MODULETEMP, base + "_liveCount", m_u32DTypep};
                live_ଗଣନା_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(live_ଗଣନା_Varp);
                vtx[i]->datap()->delay_Ring_Live_ଗଣନା_Varp = live_ଗଣନା_Varp;
                AstVar* ଅଚଳ wrappedp = ନୂତନ AstVar{regFlp, VVarType::MODULETEMP,
                                                    base + "_wrapped", m_modp->findBitDType()};
                wrappedp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(wrappedp);
                vtx[i]->datap()->delayRingWrappedVarp = wrappedp;
                ଜାରି;
            }
            ଯଦି (!vtx[i]->datap()->needsReg) ଜାରି;
            ଯଦି (vtx[i] == graph.m_startVertexp || vtx[i]->m_isMatch) ଜାରି;
            ଅଚଳ std::string varName = baseName + "__s" + std::to_string(i);
            AstVar* ଅଚଳ varp
                = ନୂତନ AstVar{regFlp, VVarType::MODULETEMP, varName, m_modp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(varp);
            vtx[i]->datap()->stateVarp = varp;
        }

        // Build lowering context for phase sub-functions.
        LowerCtx c{flp,        senTreep,      disableExprp, matchCondp,
                   assertType, directiveType, killVarp,     graph};

        // Phase 1: Resolve combinational Links via fixed-point propagation.
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ triggerExprp
            = isSeqEvent ? ନୂତନ AstConst{flp, AstConst::BitTrue{}}
                         : assertOnCond(flp, assertp->userType(), assertp->directive());
        resolveLinks(c, triggerExprp, ଗଣନା_Matches);
        VL_DO_DANGLING(triggerExprp->delete_ବୃକ୍ଷ(), triggerExprp);

        // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
        emitStateRegisterNba(c);
        emitDelayRingUpdates(c, ଗଣନା_Matches);
        emitAndCombinerDoneLatchNba(c);
        emitKillAckNba(c);

        // Phase 3/3a/3b: Compute terminal match/reject signals (cleans up stateSig).
        ଅଚଳ SignalSet sigs
            = computeSignals(c, need_Thread_Fail_ଗଣନା, need_Throughout_Thread_Fail_ଗଣନା, ଗଣନା_Matches);

        // Strong s_always[m:n] end-of-simulation liveness: if any in-window state
        // is still set at $finish, the universal-quantifier window never completed
        // (IEEE 1800-2023 16.12.11 strong semantics). Fire the assertion failure
        // from a final block; V3Assert turns the DT_ERROR display into the standard
        // "Assertion failed in %m" message.
        // pending = |{strong state registers, strong delay live counts != 0};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* pendingp = nullptr;
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < N; ++i) {
            ଯଦି (!vtx[i]->m_strongPending) ଜାରି;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* pendingExprp = nullptr;
            ଯଦି (vtx[i]->datap()->stateVarp) {
                pendingExprp = ନୂତନ AstVarRef{flp, vtx[i]->datap()->stateVarp, VAccess::READ};
            } ଅନ୍ୟଥା ଯଦି (vtx[i]->m_strongAlwaysRing) {
                pendingExprp
                    = delayRingHasLiveThreadsp(flp, vtx[i]->datap()->delay_Ring_Live_ଗଣନା_Varp);
            } ଅନ୍ୟଥା {
                ଜାରି;
            }
            ଯଦି (!pendingp) {
                pendingp = pendingExprp;
            } ଅନ୍ୟଥା {
                pendingp = ନୂତନ AstLogOr{flp, pendingp, pendingExprp};
            }
        }
        ଯଦି (pendingp) {
            AstCExpr* ଅଚଳ assertOnp
                = ନୂତନ AstCExpr{flp, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()", 1};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = ନୂତନ AstLogAnd{flp, assertOnp, pendingp};
            AstDisplay* ଅଚଳ dispp
                = ନୂତନ AstDisplay{flp, VDisplayType::DT_ERROR, "", nullptr, nullptr};
            dispp->fmtp()->timeunit(m_modp->timeunit());
            Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ firep = dispp;
            ଯଦି (v3Global.opt.stopFail()) firep->addNext(ନୂତନ AstStop{flp, ମିଥ୍ୟା});
            m_modp->addStmtsp(ନୂତନ AstFinal{flp, ନୂତନ AstIf{flp, condp, firep}});
        }

        // Clear userp on every vertex before vertexData is destroyed.
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < N; ++i) vtx[i]->userp(nullptr);
        ଫେରନ୍ତୁ sigs;
    }
};

}  // namespace

//######################################################################
// Top-level visitor

ଶ୍ରେଣୀ AssertNfaVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // STATE
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module being processed
    AstClocking* m_defaultClockingp = nullptr;  // Default clocking
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff
    SvaNfaLowering* m_loweringp = nullptr;  // NFA-to-hardware lowering engine
    Ast_Sen_ବୃକ୍ଷ* m_sampled_ମୂଲ୍ୟ_Clockp = nullptr;  // Inherited clock during scoped attachment
    V3UniqueNames m_propVarNames{"__Vpropvar"};  // Property-local variable names
    V3UniqueNames m_propTempNames{"__VnfaSampled"};  // Hoisted $sampled(propp) temps
    V3UniqueNames m_fail_ଗଣନା_Names{"__VnfaRemainingFailCount"};  // Fail replay counter names
    std::set<ଅଚଳ AstProperty*> m_inliningProps;  // Recursion guard for inlineNamedProperty

    template <typename T_ଗଣ୍ଠି>
    ଶୂନ୍ୟ visit_Sampled_ମୂଲ୍ୟ(T_ଗଣ୍ଠି* ଅଚଳ nodep) {
        ଯଦି (m_sampled_ମୂଲ୍ୟ_Clockp && !nodep->sentreep()) {
            nodep->sentreep(m_sampled_ମୂଲ୍ୟ_Clockp->clone_ବୃକ୍ଷ(ସତ୍ୟ));
        }
        iterateChildren(nodep);
    }

    // Wire match vertex and mid-window sources for a successful NFA build.
    ସ୍ଥିର ଶୂନ୍ୟ wireMatchAndMidSources(SvaGraph& graph, ଅଚଳ BuildResult& result, File_ଧାଡ଼ି* flp) {
        graph.createMatchVertex();
        // Skip the main term Link when midSources already cover every
        // end-of-match (cover_sequence path); otherwise the per-mid extraction
        // double-counts via the merge vertex.
        ଯଦି (!result.termIsMidMerge) { graph.addLink(result.termVertexp, graph.m_matchVertexp); }
        ପାଇଁ (SvaStateVertex* srcVtxp : result.midSources) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp = nullptr;
            ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ tc : srcVtxp->m_throughoutConds) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ tcClone = tc->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                condp = condp ? ନୂତନ AstLogAnd{flp, condp, tcClone} : tcClone;
            }
            graph.addLink(srcVtxp, graph.m_matchVertexp, condp);
            srcVtxp->m_isUnbounded = ସତ୍ୟ;
        }
    }

    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* getSequenceBodyExprp(ଅଚଳ AstSequence* seqp) {
        Ast_ଗଣ୍ଠି* bodyp = seqp->stmtsp();
        ଯେତେବେଳେ (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        ଫେରନ୍ତୁ VN_CAST(bodyp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
    }

    ସ୍ଥିର AstPropSpec* getPropertySpecp(ଅଚଳ AstProperty* propp) {
        Ast_ଗଣ୍ଠି* stmtp = propp->stmtsp();
        // V3LinkParse emits InitialStaticStmt for property-local variable
        // initialisers; the InitialAutomaticStmt variant only appears for
        // task/function-scope automatic lifetime, not properties.
        ଯେତେବେଳେ (stmtp
               && (VN_IS(stmtp, Var) || VN_IS(stmtp, InitialStaticStmt)
                   || VN_IS(stmtp, InitialAutomaticStmt))) {  // LCOV_EXCL_LINE
            stmtp = stmtp->nextp();
        }
        ଫେରନ୍ତୁ VN_CAST(stmtp, PropSpec);
    }

    ଶୂନ୍ୟ inlineNamedProperty(AstPropSpec* outerSpecp, AstFuncRef* funcrefp,
                             ଅଚଳ AstProperty* propyp) {
        // Recursion guard: IEEE 1800-2023 16.12.1 forbids recursive properties.
        // V3Width emits "Recursive property call" for direct recursion before this
        // pass runs; this catches any nested-inlining cycle that slips past.
        ଯଦି (!m_inliningProps.emplace(propyp).second) {
            funcrefp->v3error("Illegal recursive property reference");  // LCOV_EXCL_LINE
            ଫେରନ୍ତୁ;  // LCOV_EXCL_LINE
        }
        ସଂରଚନା Guard final {
            std::set<ଅଚଳ AstProperty*>& setr;
            ଅଚଳ AstProperty* keyp;
            ~Guard() { setr.erase(keyp); }
        } guard{m_inliningProps, propyp};
        AstPropSpec* propSpecp = getPropertySpecp(propyp);
        UASSERT_OBJ(propSpecp, funcrefp, "Property has no body PropSpec");
        propSpecp = propSpecp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);

        ଅଚଳ V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propyp->stmtsp());
        std::unordered_map<ଅଚଳ AstVar*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> portMap;
        ପାଇଁ (ଅଚଳ auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables to module-level temps (IEEE 16.10).
        std::unordered_map<ଅଚଳ AstVar*, AstVar*> localVarMap;
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = propyp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(stmtp, Var)) {
                ଯଦି (!varp->isIO()) {
                    ଅଚଳ string newName = m_propVarNames.get(varp);
                    AstVar* ଅଚଳ newVarp = ନୂତନ AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        propSpecp->foreach([&](AstVarRef* refp) {
            ଅଚଳ auto portIt = portMap.find(refp->varp());
            ଯଦି (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                VL_DO_DANGLING(ଠେଲ_Deletep(refp), refp);
                ଫେରନ୍ତୁ;
            }
            ଅଚଳ auto localIt = localVarMap.find(refp->varp());
            ଯଦି (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        ପାଇଁ (ଅଚଳ auto& tconnect : tconnects) {
            ଠେଲ_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propSpecp, funcrefp);

        // Merge disable iff (IEEE 1800-2023 16.12.1)
        ଯଦି (outerSpecp->disablep() && propSpecp->disablep()) {
            outerSpecp->v3error("disable iff expression before property call "
                                "and in its body is not legal");
            ଠେଲ_Deletep(propSpecp->disablep()->unlinkFrBack());
        }
        ଯଦି (outerSpecp->disablep()) {
            propSpecp->disablep(outerSpecp->disablep()->unlinkFrBack());
        }

        ଯଦି (outerSpecp->sensesp() && propSpecp->sensesp()) {
            outerSpecp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Clock event before property call and in its body");
            ଠେଲ_Deletep(propSpecp->sensesp()->unlinkFrBack());
        }
        ଯଦି (outerSpecp->sensesp()) {
            AstSenItem* ଅଚଳ sensesp = outerSpecp->sensesp();
            sensesp->unlinkFrBack();
            propSpecp->sensesp(sensesp);
        }

        outerSpecp->replaceWith(propSpecp);
        VL_DO_DANGLING(ଠେଲ_Deletep(outerSpecp), outerSpecp);
    }

    ଶୂନ୍ୟ inlineSequenceRef(AstFuncRef* funcrefp, AstSequence* seqp) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ clonedp = bodyExprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);

        ଅଚଳ V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ଅଚଳ AstVar*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> portMap;
        ପାଇଁ (ଅଚଳ auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ଅଚଳ auto it = portMap.find(refp->varp());
            ଯଦି (it != portMap.end()) {
                refp->replaceWith(it->second->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                VL_DO_DANGLING(ଠେଲ_Deletep(refp), refp);
            }
        });
        ପାଇଁ (ଅଚଳ auto& tconnect : tconnects) {
            ଠେଲ_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ଠେଲ_Deletep(funcrefp), funcrefp);
        // Clear referenced flag so V3AssertPre cleanup does not emit
        // spurious UNSUPPORTED for sequences that were already inlined here.
        seqp->isReferenced(ମିଥ୍ୟା);
    }

    // Must run before hasMultiCycleExpr() so NFA sees sequence bodies.
    ଶୂନ୍ୟ inlineAllSequenceRefs(Ast_ଗଣ୍ଠି* rootp) {
        ତାର୍କିକ changed = ସତ୍ୟ;
        ଯେତେବେଳେ (changed) {
            changed = ମିଥ୍ୟା;
            rootp->foreach([&](AstFuncRef* funcrefp) {
                ଯଦି (changed) ଫେରନ୍ତୁ;
                ଯଦି (AstSequence* ଅଚଳ seqp = VN_CAST(funcrefp->taskp(), Sequence)) {
                    inlineSequenceRef(funcrefp, seqp);
                    changed = ସତ୍ୟ;
                }
            });
        }
    }

    ସ୍ଥିର ତାର୍କିକ has_Multi_Cycle_ପଦସମଷ୍ଟି(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଫେରନ୍ତୁ nodep->exists([](ଅଚଳ Ast_ଗଣ୍ଠି* np) {
            ଯଦି (ଅଚଳ auto* ଅଚଳ ep = VN_CAST(np, ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) ଫେରନ୍ତୁ ep->isMultiCycleSva();
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });
    }

    ସ୍ଥିର VPropStrength effectiveAssertPropStrength(ଅଚଳ AstPropSpec* ଅଚଳ propSpecp) {
        ଯଦି (propSpecp->propStrength() != VPropStrength::DEFAULT) ଫେରନ୍ତୁ propSpecp->propStrength();
        ଫେରନ୍ତୁ propSpecp->fileline()->language() <= V3LangCode::L1800_2005 ? VPropStrength::STRONG
                                                                           : VPropStrength::WEAK;
    }

    // Bare `assert property (p until q)` with boolean operands stays on
    // V3AssertPre's AstLoop lowering, which preserves per-attempt action-block
    // firings that this NFA's single-bit aggregated state cannot. Strong bare
    // forms are also lowered there. NFA still owns sequence operands and any
    // embedding inside a multi-cycle context (implication consequent, or/and
    // operands, etc.).
    ସ୍ଥିର ତାର୍କିକ is_Bare_ଶୀର୍ଷ_Level_Until(Ast_ଗଣ୍ଠି* propp) {
        Ast_ଗଣ୍ଠି* p = propp;
        ଯଦି (AstPropSpec* ଅଚଳ specp = VN_CAST(p, PropSpec)) p = specp->propp();
        ଯେତେବେଳେ (AstLogNot* ଅଚଳ notp = VN_CAST(p, LogNot)) p = notp->lhsp();
        AstUntil* ଅଚଳ untilp = VN_CAST(p, Until);
        ଯଦି (!untilp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ auto hasSeq = [](ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ep) {
            ଫେରନ୍ତୁ ep->exists([](ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* np) { ଫେରନ୍ତୁ np->isMultiCycleSva(); });
        };
        ଯଦି (hasSeq(untilp->lhsp()) || hasSeq(untilp->rhsp())) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ସଂରଚନା PropertyParts final {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* triggerExprp = nullptr;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* seqExprp = nullptr;
        ତାର୍କିକ isOverlapped = ସତ୍ୟ;
        ତାର୍କିକ hasImplication = ମିଥ୍ୟା;
        ତାର୍କିକ isFollowedBy = ମିଥ୍ୟା;  // True for #-# / #=# (non-vacuous-fail on antecedent miss)
    };

    ସ୍ଥିର PropertyParts decomposeProperty(Ast_ଗଣ୍ଠି* propp) {
        PropertyParts parts;
        ଯଦି (AstPropSpec* ଅଚଳ specp = VN_CAST(propp, PropSpec)) { propp = specp->propp(); }
        ଯଦି (AstImplication* ଅଚଳ implp = VN_CAST(propp, Implication)) {
            parts.hasImplication = ସତ୍ୟ;
            parts.isOverlapped = implp->isOverlapped();
            parts.isFollowedBy = implp->isFollowedBy();
            parts.triggerExprp = implp->lhsp();
            parts.seqExprp = implp->rhsp();
        } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = VN_CAST(propp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) {
            parts.triggerExprp = nullptr;
            parts.seqExprp = exprp;
        }
        ଫେରନ୍ତୁ parts;
    }

    ସ୍ଥିର ତାର୍କିକ canSplitImplicationPassActions(ଅଚଳ PropertyParts& parts) {
        UASSERT(parts.hasImplication,
                "Implication pass action split requested without implication");
        UASSERT(parts.triggerExprp, "Implication pass action split requested without trigger");
        // Direct vacuous/nonvacuous classification uses the antecedent value in the current
        // assertion attempt. Leave delayed antecedents on the existing NFA pass path.
        ଫେରନ୍ତୁ !has_Multi_Cycle_ପଦସମଷ୍ଟି(parts.triggerExprp);
    }

    ସ୍ଥିର ଶୂନ୍ୟ splitImplicationPassActions(AstAssert* assertp, ଅଚଳ PropertyParts& parts,
                                            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nonvacuousMatchp) {
        File_ଧାଡ଼ି* ଅଚଳ flp = assertp->fileline();

        Ast_ଗଣ୍ଠି* ଅଚଳ passsp = assertp->passsp()->unlinkFrBackWithNext();
        Ast_ଗଣ୍ଠି* splitsp = nullptr;

        ଯଦି (!parts.isFollowedBy) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ vacuousp
                = ନୂତନ AstLogNot{flp, sampled(parts.triggerExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା))};
            Ast_ଗଣ୍ଠି* ଅଚଳ vacuousBodyp = passsp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            splitsp = newPassOnIf(flp, vacuousp, vacuousBodyp, assertp->userType(),
                                  assertp->directive(), /*vacuous=*/ସତ୍ୟ);
        }

        AstIf* ଅଚଳ nonvacuousp
            = newPassOnIf(flp, nonvacuousMatchp, passsp, assertp->userType(), assertp->directive(),
                          /*vacuous=*/ମିଥ୍ୟା);
        splitsp = splitsp ? Ast_ଗଣ୍ଠି::addNext<Ast_ଗଣ୍ଠି, Ast_ଗଣ୍ଠି>(splitsp, nonvacuousp)
                          : static_cast<Ast_ଗଣ୍ଠି*>(nonvacuousp);
        assertp->addPasssp(splitsp);
    }

    // On a PropSpec-wrapped assertion whose NFA build failed with a semantic
    // error (errorEmitted), replace the body with a BitFalse const so later
    // passes see a well-formed AST. Returns true if replaced.
    ଶୂନ୍ୟ replaceBodyOnBuildError(File_ଧାଡ଼ି* flp, AstPropSpec* propSpecp, ତାର୍କିକ errorEmitted) {
        ଯଦି (!errorEmitted) ଫେରନ୍ତୁ;
        Ast_ଗଣ୍ଠି* ଅଚଳ innerPropp = propSpecp->propp();
        innerPropp->replaceWith(ନୂତନ AstConst{flp, AstConst::BitFalse{}});
        VL_DO_DANGLING(ଠେଲ_Deletep(innerPropp), innerPropp);
    }

    // Hoist a leading clocking event (IEEE 1800-2023 16.7):
    ତାର୍କିକ hoistClockedSeq(AstPropSpec* specp) {
        ଯେତେବେଳେ (AstSClocked* ଅଚଳ clockedp = VN_CAST(specp->propp(), SClocked)) {
            ଯଦି (specp->sensesp()) {
                clockedp->v3warn(E_UNSUPPORTED, "Unsupported: multiclocked sequence or property");
                replaceBodyOnBuildError(specp->fileline(), specp, ସତ୍ୟ);
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
            ପାଇଁ (ଅଚଳ AstSenItem* sp = clockedp->sensesp(); sp;
                 sp = VN_CAST(sp->nextp(), SenItem)) {
                ଯଦି (!sp->edgeType().anEdge()) {
                    clockedp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: non-edge clocking event on a sequence; "
                                     "use an edge such as @(posedge clk)");
                    replaceBodyOnBuildError(specp->fileline(), specp, ସତ୍ୟ);
                    ଫେରନ୍ତୁ ସତ୍ୟ;
                }
            }
            specp->sensesp(clockedp->sensesp()->unlinkFrBackWithNext());
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ bodyp = clockedp->exprp()->unlinkFrBack();
            clockedp->replaceWith(bodyp);
            VL_DO_DANGLING(ଠେଲ_Deletep(clockedp), clockedp);
        }
        // A clocking event anywhere else in the sequence is not supported.
        ଅଚଳ AstSClocked* nestedp = nullptr;
        specp->propp()->foreach([&](ଅଚଳ AstSClocked* p) {
            ଯଦି (!nestedp) nestedp = p;
        });
        ଯଦି (nestedp) {
            nestedp->v3warn(E_UNSUPPORTED,
                            "Unsupported: clocking event inside sequence expression");
            replaceBodyOnBuildError(specp->fileline(), specp, ସତ୍ୟ);
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Build the NFA graph for a property body, handling both the antecedent
    // |-> consequent and simple sequence cases. Returns the consequent/body
    // BuildResult (invalid on parse/build failure).
    BuildResult buildAssertionGraph(SvaNfaBuilder& builder, SvaGraph& graph, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* seqBodyp,
                                    ଅଚଳ PropertyParts& parts, File_ଧାଡ଼ି* flp) {
        ଯଦି (!parts.hasImplication) ଫେରନ୍ତୁ builder.build(seqBodyp);

        graph.m_startVertexp = graph.createStateVertex();
        ଫେରନ୍ତୁ builder.buildImplicationEdges(parts.triggerExprp, seqBodyp, graph.m_startVertexp,
                                             parts.isOverlapped, parts.isFollowedBy,
                                             parts.triggerExprp, flp);
    }

    // Install pass-action gating and replay simultaneous per-thread failures.
    ଶୂନ୍ୟ attachActionHandlers(AstAssert* ଅଚଳ assertp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* matchExprp,
                              Ast_Sen_ବୃକ୍ଷ* ଅଚଳ threadFailReplaySenTreep,
                              Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ threadFailCountp) {
        // Gate pass handler on match to prevent vacuous-pass firings.
        ଯଦି (matchExprp) {
            // needMatch implies passsp() was non-null when evaluated above;
            // lowering does not mutate the assert's pass-action between the
            // two reads, so passsp() is still non-null here.
            Ast_ଗଣ୍ଠି* passsp = assertp->passsp();
            UASSERT_OBJ(passsp, assertp, "needMatch set but passsp is null");
            passsp->unlinkFrBackWithNext();
            File_ଧାଡ଼ି* ଅଚଳ flp = assertp->fileline();
            AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, matchExprp, passsp, nullptr};
            assertp->addPasssp(ifp);
            // Fail-handler prefix for overlapping instances (IEEE 16.12):
            // fires when reject=1 && match=1 in the same cycle.
            ଯଦି (assertp->failsp()) assertp->prependFailsp(ifp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
        }

        ଯଦି (threadFailCountp) {
            UASSERT_OBJ(threadFailReplaySenTreep, assertp,
                        "Thread fail count missing sensitivity tree");
            Ast_ଗଣ୍ଠି* ଅଚଳ failsp = assertp->failsp();
            File_ଧାଡ଼ି* ଅଚଳ flp = assertp->fileline();
            // IEEE 1800-2023 16.12 requires one action-block evaluation per failed
            // thread. AstAssert handles the first, so replay the rest here.
            AstVar* ଅଚଳ remaining_Fail_ଗଣନା_Varp
                = ନୂତନ AstVar{flp, VVarType::MODULETEMP, m_fail_ଗଣନା_Names.get(""),
                             m_modp->findBasicDType(VBasicDTypeKwd::UINT32)};
            remaining_Fail_ଗଣନା_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remaining_Fail_ଗଣନା_Varp);
            Ast_ଗଣ୍ଠି* ଅଚଳ replayStmtsp = ନୂତନ AstAssign{
                flp, ନୂତନ AstVarRef{flp, remaining_Fail_ଗଣନା_Varp, VAccess::WRITE}, threadFailCountp};
            AstLoop* ଅଚଳ replayLoopp = ନୂତନ AstLoop{flp};
            replayLoopp->addStmtsp(ନୂତନ AstLoopTest{
                flp, replayLoopp,
                ନୂତନ AstGt{flp, ନୂତନ AstVarRef{flp, remaining_Fail_ଗଣନା_Varp, VAccess::READ},
                          newTypedConstp(flp, remaining_Fail_ଗଣନା_Varp->dtypep(), 1)}});
            replayLoopp->addStmtsp(newIfAssertFailOn(failsp->clone_ବୃକ୍ଷ(ସତ୍ୟ), assertp->directive(),
                                                     assertp->userType()));
            AstSub* ଅଚଳ decrementedFailCountp
                = ନୂତନ AstSub{flp, ନୂତନ AstVarRef{flp, remaining_Fail_ଗଣନା_Varp, VAccess::READ},
                             newTypedConstp(flp, remaining_Fail_ଗଣନା_Varp->dtypep(), 1)};
            replayLoopp->addStmtsp(
                ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, remaining_Fail_ଗଣନା_Varp, VAccess::WRITE},
                              decrementedFailCountp});
            replayStmtsp->addNext(replayLoopp);
            m_modp->addStmtsp(
                ନୂତନ AstAlways{flp, VAlwaysKwd::ALWAYS, threadFailReplaySenTreep, replayStmtsp});
        }
    }

    // Replace one VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). No-op if refp is not in matchMap.
    ଶୂନ୍ୟ substituteMatchItemRef(AstVarRef* refp, unsigned K,
                                ଅଚଳ std::unordered_map<ଅଚଳ AstVar*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>& matchMap) {
        ଅଚଳ auto it = matchMap.find(refp->varp());
        ଯଦି (it == matchMap.end()) ଫେରନ୍ତୁ;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = it->second->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        ଯଦି (K > 0) {
            AstConst* ଅଚଳ ticksp = ନୂତନ AstConst{refp->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{}, 32,
                                                  static_cast<uint32_t>(K)};
            AstPast* ଅଚଳ pastp = ନୂତନ AstPast{refp->fileline(), newp, ticksp, nullptr,
                                               /* propertyTiming */ ସତ୍ୟ};
            pastp->dtypeFrom(newp);
            newp = pastp;
        }
        refp->replaceWith(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(refp), refp);
        ଫେରନ୍ତୁ;
    }

    // Recursively walk a consequent. Returns cycle length consumed and
    // substitutes each VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). Reports E_UNSUPPORTED on non-constant
    // delays or composite sequence operators and returns -1 on failure.
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା
    walk_Substitute_Match_ବସ୍ତୁଗୁଡ଼ିକ(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, unsigned K,
                             ଅଚଳ std::unordered_map<ଅଚଳ AstVar*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>& match_ବସ୍ତୁଗୁଡ଼ିକ) {
        ଯଦି (AstSExpr* ଅଚଳ sexprp = VN_CAST(nodep, SExpr)) {
            // IEEE 1800-2023 16.9.2: cycle_delay's lhsp is a constant_expression
            // and the delay form in a sequence is always `##N`, folded by
            // V3Const + V3Param before V3AssertNfa. Range form `##[m:n]` is the
            // only user-visible reject here.
            AstDelay* ଅଚଳ delayp = VN_AS(sexprp->delayp(), Delay);
            UASSERT_OBJ(delayp->isCycleDelay() && VN_IS(delayp->lhsp(), Const), sexprp,
                        "SVA cycle delay must have a constant lhsp");
            ଯଦି (delayp->isRangeDelay()) {
                sexprp->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                              "non-constant cycle delay in consequent"
                                              " (IEEE 1800-2023 16.10)");
                ଫେରନ୍ତୁ -1;
            }
            ଅଚଳ unsigned delayCycles = VN_AS(delayp->lhsp(), Const)->toUInt();
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା preLen = 0;
            ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ prep = sexprp->preExprp()) {
                preLen = walk_Substitute_Match_ବସ୍ତୁଗୁଡ଼ିକ(prep, K, match_ବସ୍ତୁଗୁଡ଼ିକ);
                ଯଦି (preLen < 0) ଫେରନ୍ତୁ -1;
            }
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା bodyLen
                = walk_Substitute_Match_ବସ୍ତୁଗୁଡ଼ିକ(sexprp->exprp(), K + preLen + delayCycles, match_ବସ୍ତୁଗୁଡ଼ିକ);
            ଯଦି (bodyLen < 0) ଫେରନ୍ତୁ -1;
            ଫେରନ୍ତୁ preLen + delayCycles + bodyLen;
        }
        ଯଦି (nodep->isMultiCycleSva()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                         "composite sequence operator in consequent"
                                         " (IEEE 1800-2023 16.10)");
            ଫେରନ୍ତୁ -1;
        }
        std::vector<AstVarRef*> refs;
        nodep->foreach([&refs](AstVarRef* p) { refs.ଠେଲ_back(p); });
        ପାଇଁ (AstVarRef* ଅଚଳ refp : refs) substituteMatchItemRef(refp, K, match_ବସ୍ତୁଗୁଡ଼ିକ);
        ଫେରନ୍ତୁ 0;
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
    ତାର୍କିକ liftMatchItemSubstitutions(PropertyParts& parts, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* seqBodyp) {
        ଯଦି (!parts.hasImplication) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        Ast_ପଦସମଷ୍ଟି_Stmt* ଅଚଳ ପଦସମଷ୍ଟି_Stmtp = VN_CAST(parts.triggerExprp, ପଦସମଷ୍ଟି_Stmt);
        ଯଦି (!ପଦସମଷ୍ଟି_Stmtp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // IEEE 1800-2023 16.10 BNF requires `(expr, match_item {, match_item})`
        // with at least one match item; V3LinkParse only emits ExprStmt for
        // this form and only emits AstAssign with VarRef LHS for each item.
        std::unordered_map<ଅଚଳ AstVar*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> match_ବସ୍ତୁଗୁଡ଼ିକ;
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = ପଦସମଷ୍ଟି_Stmtp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstAssign* ଅଚଳ assignp = VN_AS(stmtp, Assign);
            AstVarRef* ଅଚଳ lhsRefp = VN_AS(assignp->lhsp(), VarRef);
            match_ବସ୍ତୁଗୁଡ଼ିକ[lhsRefp->varp()] = assignp->rhsp();
        }
        ଅଚଳ unsigned startK = parts.isOverlapped ? 0 : 1;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା length = walk_Substitute_Match_ବସ୍ତୁଗୁଡ଼ିକ(seqBodyp, startK, match_ବସ୍ତୁଗୁଡ଼ିକ);
        // Match-item substitution / strip mutates ancestor purity. Release
        // builds don't auto-clear caches on edits, so refresh here.
        VIsCached::clear_Cache_ବୃକ୍ଷ();
        ଯଦି (length < 0) ଫେରନ୍ତୁ ସତ୍ୟ;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ antBoolp = ପଦସମଷ୍ଟି_Stmtp->resultp()->unlinkFrBack();
        ପଦସମଷ୍ଟି_Stmtp->replaceWith(antBoolp);
        VL_DO_DANGLING(ଠେଲ_Deletep(ପଦସମଷ୍ଟି_Stmtp), ପଦସମଷ୍ଟି_Stmtp);
        parts.triggerExprp = antBoolp;
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Outcome counts for a property if/case are wrong in an outcome-multiplying
    // context. Returns that context, or nullptr when the shape is supported.
    ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* unsupportedPropertyControl(ଅଚଳ Ast_ଗଣ୍ଠି_Cover_Or_Assert* assertp,
                                                  ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* seqBodyp, ତାର୍କିକ negated) {
        ଯଦି (!hasPropertyControlConjunction(seqBodyp)) ଫେରନ୍ତୁ nullptr;
        ଯଦି (negated) ଫେରନ୍ତୁ "negation";
        ଯଦି (VN_IS(assertp, Cover)) ଫେରନ୍ତୁ "cover";
        ଯଦି (VN_AS(assertp, Assert)->passsp()) ଫେରନ୍ତୁ "a pass action";
        ଫେରନ୍ତୁ nullptr;
    }

    // Inline property/sequence refs and reject unsupported shapes.
    // Returns the PropSpec to lower, or nullptr when fully handled here.
    AstPropSpec* prepareAssertionProp(Ast_ଗଣ୍ଠି_Cover_Or_Assert* assertp) {
        ଯଦି (AstPropSpec* ଅଚଳ specp = VN_CAST(assertp->propp(), PropSpec)) {
            ଯଦି (AstFuncRef* ଅଚଳ funcrefp = VN_CAST(specp->propp(), FuncRef)) {
                ଯଦି (ଅଚଳ AstProperty* ଅଚଳ propyp = VN_CAST(funcrefp->taskp(), Property)) {
                    inlineNamedProperty(specp, funcrefp, propyp);
                }
            }
        }

        inlineAllSequenceRefs(assertp->propp());

        ଯଦି (AstPropSpec* ଅଚଳ specp = VN_CAST(assertp->propp(), PropSpec)) {
            ଯଦି (hoistClockedSeq(specp)) ଫେରନ୍ତୁ nullptr;
        }

        AstPropSpec* ଅଚଳ propp = VN_AS(assertp->propp(), PropSpec);
        ଯଦି (!VN_IS(assertp, Cover)
            && effectiveAssertPropStrength(propp) == VPropStrength::STRONG) {
            propp->v3warn(E_UNSUPPORTED,
                          "Unsupported: strong property in " + assertp->verilogKwd() + ".");
            replaceBodyOnBuildError(assertp->fileline(), propp, /*errorEmitted=*/ସତ୍ୟ);
            ଫେରନ୍ତୁ nullptr;
        }

        ଯଦି (!has_Multi_Cycle_ପଦସମଷ୍ଟି(propp)) ଫେରନ୍ତୁ nullptr;
        // A nested property instance keeps its body behind the call; lowering would drop it.
        ଯଦି (propp->exists([](ଅଚଳ AstFuncRef* refp) { ଫେରନ୍ତୁ VN_IS(refp->taskp(), Property); })) {
            assertp->v3warn(E_UNSUPPORTED,
                            "Unsupported: property instance inside a multi-cycle property "
                            "expression");
            VL_DO_DANGLING(ଠେଲ_Deletep(assertp->unlinkFrBack()), assertp);
            ଫେରନ୍ତୁ nullptr;
        }
        ଯଦି (is_Bare_ଶୀର୍ଷ_Level_Until(propp)) ଫେରନ୍ତୁ nullptr;
        ଫେରନ୍ତୁ propp;
    }

    ଶୂନ୍ୟ processAssertion(Ast_ଗଣ୍ଠି_Cover_Or_Assert* assertp) {
        ଯଦି (assertp->immediate()) ଫେରନ୍ତୁ;

        AstPropSpec* ଅଚଳ propp = prepareAssertionProp(assertp);
        ଯଦି (!propp) ଫେରନ୍ତୁ;

        PropertyParts parts = decomposeProperty(propp);
        UASSERT_OBJ(parts.seqExprp, propp, "Property body must be an expression");

        // Unwrap `not` (IEEE 1800-2023 16.12.1); odd count -> negated semantics.
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* seqBodyp = parts.seqExprp;
        ତାର୍କିକ negated = ମିଥ୍ୟା;
        ଯେତେବେଳେ (AstLogNot* ଅଚଳ notp = VN_CAST(seqBodyp, LogNot)) {
            negated = !negated;
            seqBodyp = notp->lhsp();
        }

        ଅଚଳ ଅକ୍ଷର* ଅଚଳ propertyControlp
            = unsupportedPropertyControl(assertp, seqBodyp, negated);

        // Substitute property-local match-item refs in consequent with
        // $past(rhs, K) before NFA build (IEEE 1800-2023 16.10).
        ଯଦି (liftMatchItemSubstitutions(parts, seqBodyp)) {
            AstPropSpec* ଅଚଳ psp = VN_CAST(assertp->propp(), PropSpec);
            UASSERT_OBJ(psp, assertp, "Concurrent assertion must have PropSpec");
            replaceBodyOnBuildError(assertp->fileline(), psp, /*errorEmitted=*/ସତ୍ୟ);
            ଫେରନ୍ତୁ;
        }

        AstCover* ଅଚଳ coverp = VN_CAST(assertp, Cover);
        ଅଚଳ ତାର୍କିକ isCoverSeq = coverp && coverp->isCoverSeq();
        // A sequence event control is not an assertion directive; no default
        // disable iff, no assertion control
        ଅଚଳ ତାର୍କିକ isSeqEvent = coverp && coverp->isSeqEvent();
        ଅଚଳ ତାର୍କିକ ଗଣନା_Matches = isCoverSeq && !isSeqEvent;
        // Inherit module defaults (IEEE 14.12, 16.15) when assertion has none.
        ଯଦି (!propp->sensesp() && m_defaultClockingp) {
            propp->sensesp(m_defaultClockingp->sentreep()->sensesp()->clone_ବୃକ୍ଷ(ସତ୍ୟ));
        }
        ଯଦି (!propp->disablep() && m_defaultDisablep && !isSeqEvent) {
            propp->disablep(m_defaultDisablep->condp()->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ));
        }
        ଯଦି (!propp->sensesp()) ଫେରନ୍ତୁ;
        Ast_Sen_ବୃକ୍ଷ* senTreep
            = ନୂତନ Ast_Sen_ବୃକ୍ଷ{propp->fileline(), propp->sensesp()->clone_ବୃକ୍ଷ(ସତ୍ୟ)};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ disableExprp = propp->disablep();

        // NFA lowering clones repeated operands and may hoist them into an
        // always_comb block. Resolve implicit sampled-value clocks first, while
        // the enclosing assertion clock is still available.
        {
            VL_RESTORER(m_sampled_ମୂଲ୍ୟ_Clockp);
            m_sampled_ମୂଲ୍ୟ_Clockp = senTreep;
            iterate(propp->propp());
        }

        File_ଧାଡ଼ି* ଅଚଳ flp = assertp->fileline();

        SvaGraph graph;
        SvaNfaBuilder builder{graph,      m_modp,     m_propTempNames,
                              isCoverSeq, isSeqEvent, coverp != nullptr};

        ଅଚଳ BuildResult result = buildAssertionGraph(builder, graph, seqBodyp, parts, flp);
        ଯଦି (result.valid()) wireMatchAndMidSources(graph, result, flp);
        ଯଦି (!result.valid()) {
            // Fall through to V3AssertPre for unsupported constructs; only
            // replace the body on real semantic errors. Any hoisted temps
            // from this attempt become orphan MODULETEMPs; V3Dead removes
            // them along with the dead always_comb driver.
            replaceBodyOnBuildError(flp, propp, result.errorEmitted);
            VL_DO_DANGLING(ଠେଲ_Deletep(senTreep), senTreep);
            ଫେରନ୍ତୁ;
        }
        // After the build, so a construct the builder rejects reports itself.
        ଯଦି (propertyControlp) {
            seqBodyp->v3warn(E_UNSUPPORTED,
                             "Unsupported: temporal property if/case with " << propertyControlp);
            replaceBodyOnBuildError(flp, propp, /*errorEmitted=*/ସତ୍ୟ);
            VL_DO_DANGLING(ଠେଲ_Deletep(senTreep), senTreep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (ଗଣନା_Matches && !v3Global.opt.coverageUser()) {
            SvaNfaBuilder::freeUnlinkedCondp(result.finalCondp);
            VL_DO_DANGLING(ଠେଲ_Deletep(senTreep), senTreep);
            Ast_ଗଣ୍ଠି* ଅଚଳ innerPropp = propp->propp();
            innerPropp->replaceWith(ନୂତନ AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(ଠେଲ_Deletep(innerPropp), innerPropp);
            ଫେରନ୍ତୁ;
        }

        AstAssert* ଅଚଳ assertAssertp = VN_CAST(assertp, Assert);
        ଅଚଳ ତାର୍କିକ splitImplicationPasssp = assertAssertp && assertAssertp->passsp()
                                            && parts.hasImplication
                                            && canSplitImplicationPassActions(parts);
        ଅଚଳ ତାର୍କିକ needMatch = assertAssertp && assertAssertp->passsp()
                               && (!parts.hasImplication || splitImplicationPasssp);

        ଅଚଳ ତାର୍କିକ needsThreadFailReplay
            = assertAssertp && assertAssertp->failsp() && !parts.hasImplication;
        ଅଚଳ auto signals
            = m_loweringp->lower(assertp, graph, senTreep, result.finalCondp, disableExprp,
                                 needsThreadFailReplay, needsThreadFailReplay && !negated);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* matchExprp = nullptr;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* outputExprp = m_loweringp->assembleResult(
            assertp, negated, result.finalCondp, signals, needMatch ? &matchExprp : nullptr);
        ଯଦି (ଗଣନା_Matches) {
            UASSERT_OBJ(signals.matchCountp, coverp, "Cover sequence missing match count");
            VL_DO_DANGLING(outputExprp->delete_ବୃକ୍ଷ(), outputExprp);
            propp->matchCountp(signals.matchCountp);
            outputExprp = ନୂତନ AstNeq{flp, signals.matchCountp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                                     newTypedConstp(flp, signals.matchCountp->dtypep(), 0)};
        }

        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ threadFailReplaySenTreep
            = signals.threadFailCountp ? senTreep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା) : nullptr;

        VL_DO_DANGLING(ଠେଲ_Deletep(senTreep), senTreep);
        ଯଦି (result.finalCondp && !result.finalCondp->backp()) ଠେଲ_Deletep(result.finalCondp);

        ଯଦି (splitImplicationPasssp) {
            splitImplicationPassActions(assertAssertp, parts, matchExprp);
        } ଅନ୍ୟଥା {
            attachActionHandlers(assertAssertp, matchExprp, threadFailReplaySenTreep,
                                 signals.threadFailCountp);
        }

        Ast_ଗଣ୍ଠି* ଅଚଳ innerPropp = propp->propp();
        innerPropp->replaceWith(outputExprp);
        VL_DO_DANGLING(ଠେଲ_Deletep(innerPropp), innerPropp);

        UINFO(4, "NFA converted assertion at " << flp << endl);

        ଯଦି (dumpGraphLevel() >= 6) graph.m_graph.dumpDotFilePrefixed("assert-nfa");
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_loweringp);
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultDisablep);
        m_modp = nodep;
        // A class nested in a module is in the scope of the module's default clocking
        ଯଦି (!VN_IS(nodep, Class)) m_defaultClockingp = nullptr;
        m_defaultDisablep = nodep->defaultDisablep();
        SvaNfaLowering lowering{nodep};
        m_loweringp = &lowering;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstClocking* nodep) override {
        ଯଦି (nodep->isDefault() && !m_defaultClockingp) m_defaultClockingp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstDefaultDisable* nodep) override {}
    ଶୂନ୍ୟ visit(AstFell* nodep) override { visit_Sampled_ମୂଲ୍ୟ(nodep); }
    ଶୂନ୍ୟ visit(AstPast* nodep) override { visit_Sampled_ମୂଲ୍ୟ(nodep); }
    ଶୂନ୍ୟ visit(AstRose* nodep) override { visit_Sampled_ମୂଲ୍ୟ(nodep); }
    ଶୂନ୍ୟ visit(AstStable* nodep) override { visit_Sampled_ମୂଲ୍ୟ(nodep); }
    ଶୂନ୍ୟ visit(AstAssert* nodep) override { processAssertion(nodep); }
    ଶୂନ୍ୟ visit(AstCover* nodep) override { processAssertion(nodep); }
    ଶୂନ୍ୟ visit(AstRestrict* nodep) override {
        // Restrict property is ignored by simulators (IEEE 1800-2023 16.12.2).
        // Remove here so temporal SExpr don't leak to V3AssertPre.
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ଶୂନ୍ୟ visit(AstAssertIntrinsic* nodep) override {}
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    explicit AssertNfaVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Top entry point

ଶୂନ୍ୟ V3AssertNfa::assertNfaAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":" << endl);
    { AssertNfaVisitor{nodep}; }
    V3Global::dump_Check_Global_ବୃକ୍ଷ("assertnfa", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
