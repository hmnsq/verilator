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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3AssertNfa.h"

#अन्तर्भाव "V3Assert.h"
#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3Task.h"
#अन्तर्भाव "V3UniqueNames.h"

#अन्तर्भाव <set>
#अन्तर्भाव <unordered_set>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// NFA Graph Data Structures (V3Graph-derived per upstream convention)

नामाकाश {

वग्ग SvaStateVertex;

// Per-vertex algorithm data, stored via V3GraphVertex::userp() during lowering
संरचना SvaVertexData final {
    AstVar* stateVarp = nullptr;  // NBA state register for this vertex
    AstVar* delayRingVarp = nullptr;  // Packed occupancy bits or per-slot match counts
    AstVar* delayRingHeadVarp
        = nullptr;  // Cached outgoing slot; isolates readers from in-place writes
    AstVar* delayRingIdxVarp = nullptr;  // Next slot written in delayRingVarp
    AstVar* delay_Ring_Live_गणनम्_Varp = nullptr;  // Number of threads in delayRingVarp
    AstVar* delayRingWrappedVarp = nullptr;  // All slots written since the last clear
    AstVar* doneLVarp = nullptr;  // SAnd LHS done-latch
    AstVar* doneRVarp = nullptr;  // SAnd RHS done-latch
    Ast_ग्रन्थिः_व्यञ्जकम्* stateSigp = nullptr;  // Combinational state signal; OWNED during lowering
    Ast_ग्रन्थिः_व्यञ्जकम्* state_गणनम्_Sigp = nullptr;  // Cover-sequence match count; OWNED during lowering
    Ast_ग्रन्थिः_व्यञ्जकम्* state_गणनम्_Overflow_Sigp = nullptr;  // Count overflow; OWNED during lowering
    तार्किक delayRingCounts = मिच्छा;  // Ring slots hold cover-sequence match counts
    तार्किक needsReg = मिच्छा;  // True if vertex has incoming clocked edge
};

// NFA state vertex -- one per NFA position in the sequence evaluation
वग्ग SvaStateVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(SvaStateVertex, V3GraphVertex)
सार्वजनिक:
    // True if this is the sequence-match terminal vertex
    तार्किक m_isMatch = मिच्छा;
    // OWNED throughout-guard condition clones; IEEE 1800-2023 16.9.9
    std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> m_throughoutConds;
    // Nonzero for a ring-buffer vertex for ## delays.
    तार्किक m_isFixedDelayRing = मिच्छा;
    unsigned m_delayRingSize = 0;  // Number of ring slots. Range: max-min+1.
    Ast_ग्रन्थिः_व्यञ्जकम्* m_delayRingClearCondp = nullptr;  // local RHS for pure-boolean range
    Ast_ग्रन्थिः_व्यञ्जकम्* m_delayRingAdvanceCondp = nullptr;  // Advance only when this condition holds
    तार्किक m_replayAbortReject = मिच्छा;  // Compressed repetition needs per-thread abort replay
    // OWNED; enclosing-abort fire condition clearing state or suppressing guard rejection
    Ast_ग्रन्थिः_व्यञ्जकम्* m_abortClearp = nullptr;
    // OWNED; reject-abort fire condition rejecting all represented live threads
    Ast_ग्रन्थिः_व्यञ्जकम्* m_abortRejectp = nullptr;
    // Liveness terminal (IEEE weak semantics): reject must not fire from this source
    तार्किक m_isUnbounded = मिच्छा;
    // Temporal sequence AND combiner; IEEE 1800-2023 16.9.5
    तार्किक m_isAndCombiner = मिच्छा;
    SvaStateVertex* m_andLhsTermp = nullptr;  // LHS sub-NFA terminal vertex
    SvaStateVertex* m_andRhsTermp = nullptr;  // RHS sub-NFA terminal vertex
    Ast_ग्रन्थिः_व्यञ्जकम्* m_andLhsCondp = nullptr;  // OWNED; LHS final condition (may be nullptr)
    Ast_ग्रन्थिः_व्यञ्जकम्* m_andRhsCondp = nullptr;  // OWNED; RHS final condition (may be nullptr)
    // Reject sink for SAnd rejectOnFail wiring; not a state-signal source
    तार्किक m_isRejectSink = मिच्छा;
    // In-window vertex of a strong s_always[m:n]: if its state is still set at
    // end-of-simulation the universal-quantifier window never completed, which is
    // a liveness failure (IEEE 1800-2023 16.12.11 strong semantics).
    तार्किक m_strongPending = मिच्छा;
    // Ring introduced for the checked portion of a bounded strong s_always window.
    तार्किक m_strongAlwaysRing = मिच्छा;

    // CONSTRUCTORS
    explicit SvaStateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~SvaStateVertex() override {
        अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम्* cp : m_throughoutConds) VL_DO_DANGLING(cp->delete_वृक्षः(), cp);
        सचे (m_delayRingClearCondp)
            VL_DO_DANGLING(m_delayRingClearCondp->delete_वृक्षः(), m_delayRingClearCondp);
        सचे (m_delayRingAdvanceCondp)
            VL_DO_DANGLING(m_delayRingAdvanceCondp->delete_वृक्षः(), m_delayRingAdvanceCondp);
        सचे (m_abortClearp) VL_DO_DANGLING(m_abortClearp->delete_वृक्षः(), m_abortClearp);
        सचे (m_abortRejectp) VL_DO_DANGLING(m_abortRejectp->delete_वृक्षः(), m_abortRejectp);
        सचे (m_andLhsCondp) VL_DO_DANGLING(m_andLhsCondp->delete_वृक्षः(), m_andLhsCondp);
        सचे (m_andRhsCondp) VL_DO_DANGLING(m_andRhsCondp->delete_वृक्षः(), m_andRhsCondp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string name() ध्रुव override {
        string name = "s" + cvtToStr(color());
        सचे (m_delayRingSize) {
            name += "\\n";
            name += m_isFixedDelayRing ? "fixed chain " : "range chain ";
            name += cvtToStr(m_delayRingSize) + " slots";
        }
        पटिनिवत्त name;
    }
    string dotColor() ध्रुव override {
        सचे (m_isMatch) पटिनिवत्त "red";
        सचे (m_delayRingSize) पटिनिवत्त "blue";
        सचे (m_isAndCombiner) पटिनिवत्त "purple";
        पटिनिवत्त "black";
    }
    // LCOV_EXCL_STOP
    // Access per-vertex algorithm data (valid only during lowering phase)
    SvaVertexData* datap() ध्रुव { पटिनिवत्त static_cast<SvaVertexData*>(userp()); }
};

// NFA transition edge -- clocked (##1) or combinational link (##0)
वग्ग SvaTransEdge final : सार्वजनिक V3GraphEdge {
    VL_RTTI_IMPL(SvaTransEdge, V3GraphEdge)
सार्वजनिक:
    Ast_ग्रन्थिः_व्यञ्जकम्* m_condp;  // Transition condition; nullptr = unconditional; OWNED
    तार्किक m_consumesCycle;  // true = clocked edge (##1), false = link (##0/boolean)
    // Reject when source is active and condp is false; set only on
    // outermost required-step Link
    तार्किक m_rejectOnFail = मिच्छा;
    // Optional dynamic condition vertex for m_rejectOnFail. Used when the
    // success condition is another NFA state rather than a static expression.
    SvaStateVertex* m_condVtxp = nullptr;

    // CONSTRUCTORS
    SvaTransEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शिखरम्, Ast_ग्रन्थिः_व्यञ्जकम्* condp,
                 तार्किक consumesCycle)
        : V3GraphEdge{graphp, fromp, शिखरम्, /*weight=*/1}
        , m_condp{condp}
        , m_consumesCycle{consumesCycle} {}
    ~SvaTransEdge() override {
        सचे (m_condp) VL_DO_DANGLING(m_condp->delete_वृक्षः(), m_condp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string dotLabel() ध्रुव override { पटिनिवत्त m_consumesCycle ? "##1" : "link"; }
    string dotStyle() ध्रुव override { पटिनिवत्त m_consumesCycle ? "" : "dashed"; }
    // LCOV_EXCL_STOP
    // Typed accessors for NFA vertices
    SvaStateVertex* fromVtxp() ध्रुव { पटिनिवत्त static_cast<SvaStateVertex*>(fromp()); }
    SvaStateVertex* toVtxp() ध्रुव { पटिनिवत्त static_cast<SvaStateVertex*>(शिखरम्()); }
};

// NFA graph container
वग्ग SvaGraph final {
सार्वजनिक:
    V3Graph m_graph;  // Owns all vertices and edges
    SvaStateVertex* m_startVertexp = nullptr;  // Trigger/start vertex
    SvaStateVertex* m_matchVertexp = nullptr;  // Sequence-match terminal vertex

    // Create a new state vertex
    SvaStateVertex* createStateVertex() { पटिनिवत्त नूतन SvaStateVertex{&m_graph}; }
    // Create the match terminal vertex
    SvaStateVertex* createMatchVertex() {
        SvaStateVertex* ध्रुव vtxp = createStateVertex();
        vtxp->m_isMatch = सच्च;
        m_matchVertexp = vtxp;
        पटिनिवत्त vtxp;
    }
    // Add a clocked transition edge (##1)
    SvaTransEdge* addClockedEdge(SvaStateVertex* fromp, SvaStateVertex* शिखरम्,
                                 Ast_ग्रन्थिः_व्यञ्जकम्* condp = nullptr) {
        पटिनिवत्त नूतन SvaTransEdge{&m_graph, fromp, शिखरम्, condp, /*consumesCycle=*/सच्च};
    }
    // Add a combinational link (##0 / boolean condition)
    SvaTransEdge* addLink(SvaStateVertex* fromp, SvaStateVertex* शिखरम्,
                          Ast_ग्रन्थिः_व्यञ्जकम्* condp = nullptr) {
        पटिनिवत्त नूतन SvaTransEdge{&m_graph, fromp, शिखरम्, condp, /*consumesCycle=*/मिच्छा};
    }
    // Collect all edges into a flat vector for iteration.
    // Used by the lowering phase which needs global edge scans.
    std::vector<ध्रुव SvaTransEdge*> allEdges() ध्रुव {
        std::vector<ध्रुव SvaTransEdge*> result;
        अत्थाय (ध्रुव V3GraphVertex& vtxr : m_graph.vertices()) {
            अत्थाय (ध्रुव V3GraphEdge& edger : vtxr.outEdges()) {
                result.प्रक्षिप_back(static_cast<ध्रुव SvaTransEdge*>(&edger));
            }
        }
        पटिनिवत्त result;
    }
};

//######################################################################
// Builder result: terminal vertex + optional final condition (match Link condition).
संरचना BuildResult final {
    SvaStateVertex* termVertexp;  // Primary terminal; contributes to both match and reject
    Ast_ग्रन्थिः_व्यञ्जकम्* finalCondp;  // nullptr = unconditional
    // Mid-window sources for range delays (pure boolean RHS): match-only (isUnbounded)
    std::vector<SvaStateVertex*> midSources;
    तार्किक errorEmitted = मिच्छा;  // Builder already emitted specific error; skip generic
    // For cover_sequence: when true, midSources already enumerate every
    // end-of-match, so wireMatchAndMidSources must NOT add the main
    // termVtxp -> matchVertex Link (would double-count via the merge vertex).
    तार्किक termIsMidMerge = मिच्छा;
    तार्किक valid() ध्रुव { पटिनिवत्त termVertexp != nullptr; }
    स्थिर BuildResult fail(तार्किक errored = मिच्छा) { पटिनिवत्त {nullptr, nullptr, {}, errored}; }
    स्थिर BuildResult failWithError() { पटिनिवत्त {nullptr, nullptr, {}, सच्च}; }
};

// Parser-marked SAnd of overlapped implications: a property if/else/case.
स्थिर तार्किक hasPropertyControlConjunction(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
    पटिनिवत्त nodep->exists([](ध्रुव AstSAnd* andp) { पटिनिवत्त andp->propertyControl(); });
}

स्थिर AstConst* newTypedConstp(File_पङ्क्तिः* ध्रुव flp, ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep,
                                ध्रुव uint32_t मानम्) {
    AstConst* ध्रुव constp = नूतन AstConst{flp, AstConst::DTyped{}, dtypep};
    constp->अङ्कः().setLong(मानम्);
    पटिनिवत्त constp;
}

स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* sampled(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
    पटिनिवत्त नूतन AstSampled{exprp->fileline(), exprp, exprp->dtypep(), सच्च};
}

स्थिर ध्रुव वर्ण* assertPassOnQuery(तार्किक vacuous) {
    स्थिर constexpr ध्रुव वर्ण* queries[2]
        = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
    पटिनिवत्त queries[vacuous];
}

स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* assertOnCond(File_पङ्क्तिः* flp, VAssertType type,
                                 VAssertDirectiveType directiveType) {
    सचे (!v3Global.opt.assertOn()) { पटिनिवत्त नूतन AstConst{flp, AstConst::BitFalse{}}; }
    पटिनिवत्त नूतन AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
}

स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* assertKillGet(File_पङ्क्तिः* flp, VAssertType type,
                                  VAssertDirectiveType directiveType) {
    पटिनिवत्त नूतन AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_KILL", type, directiveType),
                        32};
}

स्थिर string assertActionControlPrefix(VAssertDirectiveType directiveType) {
    ध्रुव पूर्णाङ्क controlled = !!(static_cast<पूर्णाङ्क>(directiveType)
                              & (static_cast<पूर्णाङ्क>(VAssertDirectiveType::ASSERT)
                                 | static_cast<पूर्णाङ्क>(VAssertDirectiveType::COVER)
                                 | static_cast<पूर्णाङ्क>(VAssertDirectiveType::ASSUME)));
    ध्रुव पूर्णाङ्क checkRuntime = controlled & static_cast<पूर्णाङ्क>(v3Global.opt.assertOn());
    पटिनिवत्त "("s + std::to_string(controlled ^ 1) + " || ("s + std::to_string(checkRuntime)
           + " && "s;
}

स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* assertPassOnCond(File_पङ्क्तिः* flp, VAssertType type,
                                     VAssertDirectiveType directiveType, तार्किक vacuous) {
    पटिनिवत्त नूतन AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType)
            + "))"s,
        1};
}

स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* assertFailOnCond(File_पङ्क्तिः* flp, VAssertType type,
                                     VAssertDirectiveType directiveType) {
    पटिनिवत्त नूतन AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType) + "))"s,
        1};
}

स्थिर AstIf* newPassOnIf(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* firep, Ast_ग्रन्थिः* bodyp, VAssertType type,
                          VAssertDirectiveType directiveType, तार्किक vacuous) {
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp
        = नूतन AstLogAnd{flp, firep, assertPassOnCond(flp, type, directiveType, vacuous)};
    AstIf* ध्रुव ifp = नूतन AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(सच्च);
    ifp->user1(सच्च);
    पटिनिवत्त ifp;
}

स्थिर Ast_ग्रन्थिः_Stmt* newIfAssertFailOn(Ast_ग्रन्थिः* bodyp, VAssertDirectiveType directiveType,
                                      VAssertType type) {
    File_पङ्क्तिः* ध्रुव flp = bodyp->fileline();
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = assertFailOnCond(flp, type, directiveType);
    AstIf* ध्रुव ifp = नूतन AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(सच्च);
    ifp->user1(सच्च);
    पटिनिवत्त ifp;
}

//######################################################################
// NFA Builder

वग्ग SvaNfaBuilder final {
    SvaGraph& m_graph;  // NFA graph being built
    Ast_ग्रन्थिः_Module* ध्रुव m_modp;  // Module to receive hoisted sampled-prop temps
    V3UniqueNames& m_propTempNames;  // Module-shared temp-var name source
    std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> m_temporal_Guard_राशिः;  // Guards active across nested temporal states
    // Outer abort conditions, AND-ed as !cond into inner abort edges
    // (IEEE 1800-2023 16.12.14 outer-wraps-inner).
    std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> m_outer_Abort_राशिः;
    तार्किक m_inUnboundedScope = मिच्छा;  // Sticky: nodes created after inherit liveness
    तार्किक m_markStrongPending = मिच्छा;  // Mark new vertices as strong s_always in-window
    तार्किक m_isCover = मिच्छा;  // Cover directives do not fail at end-of-simulation
    // IEEE 1800-2023 16.14.3 cover sequence: each end-of-match fires the action,
    // not just the first. Builder builds parallel-branch (no first-match-wins)
    // topology when true. Default false preserves cover_property semantics.
    तार्किक m_isCoverSeq = मिच्छा;
    // Unsupported endpoint topology must reject, not ignore, or the wait hangs
    तार्किक m_isSeqEvent = मिच्छा;
    तार्किक m_inSequencePrefix = मिच्छा;  // Prefix composition does not preserve every endpoint
    तार्किक m_wantBoolLeaf = मिच्छा;  // Building a |-> / |=> or top-level body (see buildOrMerge)

    संरचना RangeDelayRejectInfo final {
        SvaStateVertex* startp = nullptr;
        unsigned range = 0;
        पूर्णाङ्क rhsLen = 0;
    };

    शून्य warnEndpointUnsupported(File_पङ्क्तिः* flp, ध्रुव string& what) ध्रुव {
        सचे (m_isSeqEvent) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: sequence used as an event control with " << what);
        } अञ्ञथा {
            flp->v3warn(COVERIGN, "Ignoring unsupported: cover sequence with " << what);
        }
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* throughoutCond(Ast_ग्रन्थिः_व्यञ्जकम्* baseCondp, File_पङ्क्तिः* flp) {
        सचे (m_temporal_Guard_राशिः.empty()) पटिनिवत्त baseCondp;
        // AND all active temporal guards (supports nesting)
        // Each must use $sampled values.
        Ast_ग्रन्थिः_व्यञ्जकम्* guardp = nullptr;
        अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp : m_temporal_Guard_राशिः) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव clonep = sampled(condp->clone_वृक्षः_Pure(मिच्छा));
            सचे (!guardp) {
                guardp = clonep;
            } अञ्ञथा {
                guardp = नूतन AstLogAnd{flp, guardp, clonep};
            }
        }
        सचे (baseCondp) { guardp = नूतन AstLogAnd{flp, baseCondp, guardp}; }
        पटिनिवत्त guardp;
    }

    स्थिर unsigned getConstUInt(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव constp = V3Const::constifyEdit(exprp->clone_वृक्षः_Pure(मिच्छा));
        ध्रुव AstConst* ध्रुव cp = VN_CAST(constp, Const);
        ध्रुव unsigned val = cp ? cp->toUInt() : 0;
        VL_DO_DANGLING(constp->delete_वृक्षः(), constp);
        पटिनिवत्त val;
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
    स्थिर पूर्णाङ्क fixedLength(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        सचे (AstSExpr* ध्रुव sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ध्रुव delayp = VN_CAST(sexprp->delayp(), Delay);
            सचे (!delayp || !delayp->isCycleDelay()) पटिनिवत्त -1;
            unsigned delayCycles;
            सचे (delayp->isRangeDelay()) {
                सचे (delayp->isUnbounded()) पटिनिवत्त -1;  // LCOV_EXCL_LINE
                ध्रुव unsigned minD = getConstUInt(delayp->lhsp());
                ध्रुव unsigned maxD = getConstUInt(delayp->rhsp());
                सचे (minD != maxD) पटिनिवत्त -1;
                delayCycles = minD;
            } अञ्ञथा {
                delayCycles = getConstUInt(delayp->lhsp());
            }
            पूर्णाङ्क preLen = 0;
            सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव prep = sexprp->preExprp()) {
                preLen = fixedLength(prep);
                सचे (preLen < 0) पटिनिवत्त -1;  // LCOV_EXCL_LINE
            }
            ध्रुव पूर्णाङ्क bodyLen = fixedLength(sexprp->exprp());
            सचे (bodyLen < 0) पटिनिवत्त -1;  // LCOV_EXCL_LINE
            पटिनिवत्त preLen + delayCycles + bodyLen;
        }
        सचे (AstSThroughout* ध्रुव throughp = VN_CAST(nodep, SThroughout)) {
            पटिनिवत्त fixedLength(throughp->rhsp());
        }
        सचे (AstSOr* ध्रुव orp = VN_CAST(nodep, SOr)) {
            // Alternatives must share one end cycle; buildSWithin relies on
            // this to pair the OR with an SIntersect.
            ध्रुव पूर्णाङ्क lhsLen = fixedLength(orp->lhsp());
            ध्रुव पूर्णाङ्क rhsLen = fixedLength(orp->rhsp());
            सचे (lhsLen < 0 || rhsLen < 0 || lhsLen != rhsLen) पटिनिवत्त -1;
            पटिनिवत्त lhsLen;
        }
        सचे (AstSWithin* ध्रुव withinp = VN_CAST(nodep, SWithin)) {
            // `seq1 within seq2` ends at seq2's end cycle (IEEE 16.9.10).
            ध्रुव पूर्णाङ्क lhsLen = fixedLength(withinp->lhsp());
            ध्रुव पूर्णाङ्क rhsLen = fixedLength(withinp->rhsp());
            सचे (lhsLen < 0 || rhsLen < 0 || lhsLen > rhsLen) पटिनिवत्त -1;
            पटिनिवत्त rhsLen;
        }
        // LCOV_EXCL_START -- defensive: V3AssertPre rejects composite SVA ops
        // nested in an intersect arm before fixedLength runs (clock-context
        // resolution fails). Kept as a guard in case future parser relaxations
        // permit it.
        सचे (nodep->isMultiCycleSva()) पटिनिवत्त -1;
        // LCOV_EXCL_STOP
        // Plain boolean expression (no SVA constructs) -- 0 cycles.
        पटिनिवत्त 0;
    }

    // Contiguous match-length range [lo,hi] for an operand whose length varies
    // from AT MOST ONE ranged cycle delay; {-1,-1} otherwise. Drives the
    // variable-length `intersect` lowering (IEEE 1800-2023 16.9.6): more than
    // one ranged delay would make the per-length realization ambiguous, so it
    // is reported unsupported rather than mis-paired.
    स्थिर std::pair<पूर्णाङ्क, पूर्णाङ्क> lengthRange(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        सचे (AstSExpr* ध्रुव sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ध्रुव delayp = VN_CAST(sexprp->delayp(), Delay);
            सचे (!delayp || !delayp->isCycleDelay()) पटिनिवत्त {-1, -1};
            std::pair<पूर्णाङ्क, पूर्णाङ्क> delayRange;
            सचे (delayp->isRangeDelay()) {
                सचे (delayp->isUnbounded()) पटिनिवत्त {-1, -1};
                ध्रुव unsigned minD = getConstUInt(delayp->lhsp());
                ध्रुव unsigned maxD = getConstUInt(delayp->rhsp());
                delayRange = {minD, maxD};
            } अञ्ञथा {
                ध्रुव unsigned d = getConstUInt(delayp->lhsp());
                delayRange = {d, d};
            }
            std::pair<पूर्णाङ्क, पूर्णाङ्क> preRange{0, 0};
            सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव prep = sexprp->preExprp()) {
                preRange = lengthRange(prep);
                सचे (preRange.first < 0) पटिनिवत्त {-1, -1};
            }
            ध्रुव std::pair<पूर्णाङ्क, पूर्णाङ्क> bodyRange = lengthRange(sexprp->exprp());
            सचे (bodyRange.first < 0) पटिनिवत्त {-1, -1};
            ध्रुव पूर्णाङ्क variableParts = (preRange.first != preRange.second)
                                      + (delayRange.first != delayRange.second)
                                      + (bodyRange.first != bodyRange.second);
            सचे (variableParts > 1) पटिनिवत्त {-1, -1};
            पटिनिवत्त {preRange.first + delayRange.first + bodyRange.first,
                    preRange.second + delayRange.second + bodyRange.second};
        }
        सचे (AstSThroughout* ध्रुव throughp = VN_CAST(nodep, SThroughout)) {
            पटिनिवत्त lengthRange(throughp->rhsp());
        }
        सचे (nodep->isMultiCycleSva()) पटिनिवत्त {-1, -1};
        पटिनिवत्त {0, 0};  // plain boolean -- 0 cycles
    }

    // Clone `operand` with its sole variable ranged cycle delay pinned so the
    // total match length is exactly `len`. `lo` is the operand's minimum length
    // (lengthRange().first). A fixed operand has no such delay and is returned
    // as a plain clone (callers only request its single achievable length).
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* realizeAtLength(Ast_ग्रन्थिः_व्यञ्जकम्* operand, पूर्णाङ्क len, पूर्णाङ्क lo) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव clonep = operand->clone_वृक्षः_Pure(मिच्छा);
        AstDelay* rangeDelayp = nullptr;
        clonep->foreach([&](AstDelay* dp) {
            सचे (!rangeDelayp && dp->isRangeDelay() && !dp->isUnbounded()
                && getConstUInt(dp->lhsp()) != getConstUInt(dp->rhsp())) {
                rangeDelayp = dp;
            }
        });
        सचे (rangeDelayp) {
            File_पङ्क्तिः* ध्रुव flp = rangeDelayp->fileline();
            ध्रुव unsigned pinned = getConstUInt(rangeDelayp->lhsp()) + (len - lo);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव oldMinp = rangeDelayp->lhsp();
            oldMinp->replaceWith(नूतन AstConst{flp, pinned});
            VL_DO_DANGLING(oldMinp->delete_वृक्षः(), oldMinp);
            // Drop the max bound so it lowers as a fixed `##d`, not `##[d:d]`.
            Ast_ग्रन्थिः* ध्रुव oldMaxp = rangeDelayp->rhsp()->unlinkFrBack();
            VL_DO_DANGLING(oldMaxp->delete_वृक्षः(), oldMaxp);
        }
        पटिनिवत्त clonep;
    }

    // Cuts AST size from O(N * sizeof(exprp)) to O(N) + O(sizeof(exprp)) by
    // sharing a single `VarRef` across N check edges. Hoist also matches the
    // IEEE 1800-2023 16.9.9 "single preponed-region snapshot" semantic for
    // any exprp -- even an impure one would now evaluate exactly once per
    // clock instead of N times. Orphan temps from failed builds are unused
    // MODULETEMPs and are removed by V3Dead.
    AstVar* tryHoistSampled(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, File_पङ्क्तिः* flp, unsigned clone_गणनम्) {
        constexpr unsigned kHoistThreshold = 2;
        सचे (clone_गणनम् < kHoistThreshold) पटिनिवत्त nullptr;
        AstVar* ध्रुव tempVarp
            = नूतन AstVar{flp, VVarType::MODULETEMP, m_propTempNames.get(exprp), exprp->dtypep()};
        m_modp->addStmtsp(tempVarp);
        AstAssign* ध्रुव assignp = नूतन AstAssign{flp, नूतन AstVarRef{flp, tempVarp, VAccess::WRITE},
                                                 sampled(exprp->clone_वृक्षः_Pure(मिच्छा))};
        m_modp->addStmtsp(नूतन AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, assignp});
        पटिनिवत्त tempVarp;
    }

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* sampledRefOrClone(AstVar* hoistVarp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, File_पङ्क्तिः* flp) {
        सचे (hoistVarp) पटिनिवत्त नूतन AstVarRef{flp, hoistVarp, VAccess::READ};
        पटिनिवत्त sampled(exprp->clone_वृक्षः_Pure(मिच्छा));
    }

    // Create vertex and inherit temporal guards from the current scope.
    SvaStateVertex* scopedCreateVertex() {
        SvaStateVertex* ध्रुव vtxp = m_graph.createStateVertex();
        अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cp : m_temporal_Guard_राशिः) {
            vtxp->m_throughoutConds.प्रक्षिप_back(cp->clone_वृक्षः_Pure(मिच्छा));
        }
        सचे (m_inUnboundedScope) vtxp->m_isUnbounded = सच्च;
        सचे (m_markStrongPending) vtxp->m_strongPending = सच्च;
        पटिनिवत्त vtxp;
    }

    // AND current temporal guards into every edge/link.
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* शिखरम्, Ast_ग्रन्थिः_व्यञ्जकम्* condp,
                              File_पङ्क्तिः* flp) {
        पटिनिवत्त m_graph.addLink(fromp, शिखरम्, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* शिखरम्, File_पङ्क्तिः* flp) {
        पटिनिवत्त m_graph.addLink(fromp, शिखरम्, throughoutCond(nullptr, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* शिखरम्, Ast_ग्रन्थिः_व्यञ्जकम्* condp,
                              File_पङ्क्तिः* flp) {
        पटिनिवत्त m_graph.addClockedEdge(fromp, शिखरम्, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* शिखरम्, File_पङ्क्तिः* flp) {
        पटिनिवत्त m_graph.addClockedEdge(fromp, शिखरम्, throughoutCond(nullptr, flp));
    }

    SvaStateVertex* addDelayChain(SvaStateVertex* startp, unsigned size, File_पङ्क्तिः* flp,
                                  तार्किक isFixed = सच्च, Ast_ग्रन्थिः_व्यञ्जकम्* clearCondp = nullptr,
                                  Ast_ग्रन्थिः_व्यञ्जकम्* advanceCondp = nullptr) {
        सचे (isFixed && size == 0) पटिनिवत्त startp;
        UASSERT_OBJ(size > 0, startp, "Delay chain needs at least one slot");
        सचे (isFixed && size == 1 && !advanceCondp) {
            SvaStateVertex* ध्रुव nextp = scopedCreateVertex();
            guardedEdge(startp, nextp, flp);
            पटिनिवत्त nextp;
        }
        SvaStateVertex* ध्रुव ringVtxp = scopedCreateVertex();
        ringVtxp->m_isFixedDelayRing = isFixed;
        ringVtxp->m_delayRingSize = size;
        सचे (clearCondp) {
            UASSERT_OBJ(!isFixed, startp, "Fixed delay cannot have a clear condition");
            ringVtxp->m_delayRingClearCondp = clearCondp->clone_वृक्षः_Pure(मिच्छा);
        }
        ringVtxp->m_delayRingAdvanceCondp = advanceCondp;
        सचे (isFixed) {
            guardedEdge(startp, ringVtxp, flp);
        } अञ्ञथा {
            guardedLink(startp, ringVtxp, flp);
        }
        पटिनिवत्त ringVtxp;
    }

    // Build NFA for an SExpr. finalCond = RHS (not yet added as a vertex).
    // isTopLevelStep: marks outermost required boolean check as rejectOnFail.
    // Apply a range delay `##[M:N]` to currentp. Returns true on success. On
    // failure, emits a diagnostic and returns false.
    तार्किक applyRangeDelay(AstSExpr* sexprp, SvaStateVertex*& currentp,
                         std::vector<SvaStateVertex*>& midSources,
                         RangeDelayRejectInfo* rangeRejectInfop = nullptr) {
        File_पङ्क्तिः* ध्रुव flp = sexprp->fileline();
        AstDelay* ध्रुव delayp = VN_AS(sexprp->delayp(), Delay);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsExprp = sexprp->exprp();
        ध्रुव unsigned minDelay = getConstUInt(delayp->lhsp());
        सचे (delayp->isUnbounded()) {
            // `##[M:$]`: wait M cycles, then retain every matured attempt in a
            // one-slot ring while also exposing the first eligible tick.
            currentp = addDelayChain(currentp, minDelay, flp);
            SvaStateVertex* ध्रुव waitRingp = addDelayChain(currentp, 1, flp, मिच्छा);
            // The ring's live count is updated in NBA, so this link retains the previous
            // tick's live attempts alongside new arrivals for the next tick.
            guardedLink(waitRingp, waitRingp, flp);
            SvaStateVertex* ध्रुव mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(waitRingp, mergeVtxp, flp);
            currentp = mergeVtxp;
            currentp->m_isUnbounded = सच्च;  // Liveness, so no reject.
            m_inUnboundedScope = सच्च;
            पटिनिवत्त सच्च;
        }
        ध्रुव unsigned maxDelay = getConstUInt(delayp->rhsp());
        सचे (minDelay == maxDelay) {
            currentp = addDelayChain(currentp, minDelay, flp);
            पटिनिवत्त सच्च;
        }
        ध्रुव unsigned range = maxDelay - minDelay;
        currentp = addDelayChain(currentp, minDelay, flp);
        ध्रुव तार्किक multiCycleRhs = rhsExprp->isMultiCycleSva();
        ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव preExprp = sexprp->preExprp();
        // Prefix and nested-sequence composition can lose endpoints or their multiplicity
        // required by cover sequence (IEEE 1800-2023 16.14.3), regardless of the range width.
        सचे (m_isCoverSeq
            && (multiCycleRhs || m_inSequencePrefix
                || (preExprp && preExprp->isMultiCycleSva()))) {
            warnEndpointUnsupported(flp, "this ranged cycle delay");
            पटिनिवत्त मिच्छा;
        }
        सचे (m_inSequencePrefix) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: Bounded ranged cycle delay in a sequence prefix");
            पटिनिवत्त मिच्छा;
        }
        सचे (multiCycleRhs) {
            // Launch a candidate at every eligible tick. Candidate-local misses are not
            // assertion rejects while a later position can still match.
            सचे (rangeRejectInfop) {
                ध्रुव पूर्णाङ्क rhsLen = fixedLength(rhsExprp);
                सचे (rhsLen >= 0) *rangeRejectInfop = {currentp, range, rhsLen};
            }
            SvaStateVertex* ध्रुव tailp = addDelayChain(currentp, range + 1U, flp, मिच्छा);
            SvaStateVertex* ध्रुव mergeVtxp = scopedCreateVertex();
            mergeVtxp->m_isUnbounded = सच्च;
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(tailp, mergeVtxp, flp);
            currentp = mergeVtxp;
            m_inUnboundedScope = सच्च;
        } अञ्ञथा {
            // The first eligible tick is explicit. The ring retains attempts for the
            // remaining ticks and exposes the final tick for rejection. Only properties
            // clear on success, since cover sequence counts every endpoint.
            midSources.प्रक्षिप_back(currentp);
            currentp = addDelayChain(currentp, range + 1U, flp, मिच्छा,
                                     m_isCoverSeq ? nullptr : rhsExprp);
        }
        पटिनिवत्त सच्च;
    }

    शून्य addFiniteRangeReject(ध्रुव RangeDelayRejectInfo& info, ध्रुव BuildResult& result,
                              File_पङ्क्तिः* flp) {
        सचे (!info.startp) पटिनिवत्त;

        SvaStateVertex* ध्रुव expiryVtxp
            = addDelayChain(info.startp, info.range + info.rhsLen, flp);
        SvaStateVertex* ध्रुव expiryMatchp = scopedCreateVertex();
        std::vector<SvaStateVertex*> sources = result.midSources;
        sources.प्रक्षिप_back(result.termVertexp);
        अत्थाय (SvaStateVertex* ध्रुव srcp : sources) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp
                = result.finalCondp ? sampled(result.finalCondp->clone_वृक्षः_Pure(मिच्छा)) : nullptr;
            SvaStateVertex* ध्रुव successNowp = scopedCreateVertex();
            guardedLink(srcp, successNowp, condp, flp);
            // Retain a success through the latest deadline it can satisfy.
            SvaStateVertex* ध्रुव historyp
                = addDelayChain(successNowp, info.range + 1U, flp, मिच्छा);
            guardedLink(successNowp, expiryMatchp, flp);
            guardedLink(historyp, expiryMatchp, flp);
        }

        SvaStateVertex* ध्रुव sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = सच्च;
        SvaTransEdge* ध्रुव rejectp = m_graph.addLink(expiryVtxp, sinkVtxp);
        rejectp->m_rejectOnFail = सच्च;
        rejectp->m_condVtxp = expiryMatchp;
    }

    BuildResult buildSExpr(AstSExpr* sexprp, SvaStateVertex* entryVtxp,
                           तार्किक is_शिखरम्_Level_Step = मिच्छा) {
        AstDelay* ध्रुव delayp = VN_CAST(sexprp->delayp(), Delay);
        सचे (!delayp || !delayp->isCycleDelay()) पटिनिवत्त BuildResult::fail();

        File_पङ्क्तिः* ध्रुव flp = sexprp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = sexprp->exprp();

        // Handle LHS (preExpr)
        SvaStateVertex* currentp = entryVtxp;
        सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव preExprp = sexprp->preExprp()) {
            VL_RESTORER(m_inSequencePrefix);
            m_inSequencePrefix = सच्च;
            ध्रुव BuildResult pre = build_व्यञ्जकम्(preExprp, currentp, is_शिखरम्_Level_Step);
            सचे (!pre.valid()) पटिनिवत्त BuildResult::fail(pre.errorEmitted);  // LCOV_EXCL_LINE
            सचे (pre.finalCondp) {
                SvaStateVertex* ध्रुव condVtxp = scopedCreateVertex();
                SvaTransEdge* ध्रुव edgep = guardedLink(
                    pre.termVertexp, condVtxp, sampled(pre.finalCondp->clone_वृक्षः_Pure(मिच्छा)), flp);
                सचे (is_शिखरम्_Level_Step && !pre.termVertexp->m_isUnbounded && !m_inUnboundedScope) {
                    // Do not mark liveness sources: first boolean check is deferred.
                    edgep->m_rejectOnFail = सच्च;
                }
                freeUnlinkedCondp(pre.finalCondp);
                currentp = condVtxp;
            } अञ्ञथा {
                currentp = pre.termVertexp;
            }
        }

        // Handle delay
        std::vector<SvaStateVertex*> rangeMidSources;
        RangeDelayRejectInfo rangeRejectInfo;
        ध्रुव तार्किक addRangeReject = is_शिखरम्_Level_Step && !m_inUnboundedScope;
        सचे (delayp->isRangeDelay()) {
            सचे (!applyRangeDelay(sexprp, currentp, rangeMidSources,
                                 addRangeReject ? &rangeRejectInfo : nullptr)) {
                पटिनिवत्त BuildResult::failWithError();
            }
        } अञ्ञथा {
            ध्रुव unsigned delayCycles = getConstUInt(delayp->lhsp());
            currentp = addDelayChain(currentp, delayCycles, flp);
        }

        // Multi-cycle RHS: recurse (only plain boolean is returned as finalCondp).
        सचे (exprp->isMultiCycleSva()) {
            ध्रुव BuildResult result = build_व्यञ्जकम्(exprp, currentp, is_शिखरम्_Level_Step);
            सचे (result.valid()) addFiniteRangeReject(rangeRejectInfo, result, flp);
            पटिनिवत्त result;
        }
        पटिनिवत्त {currentp, exprp, std::move(rangeMidSources)};
    }

    BuildResult buildConsRep(AstSConsRep* repp, SvaStateVertex* entryVtxp,
                             तार्किक is_शिखरम्_Level_Step = मिच्छा) {
        File_पङ्क्तिः* ध्रुव flp = repp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = repp->exprp();
        // Multi-cycle expr in ConsRep not yet supported; bail to avoid invalid AST.
        सचे (exprp->isMultiCycleSva()) {
            repp->v3warn(E_UNSUPPORTED, "Unsupported: multi-cycle sequence expression inside"
                                        " consecutive repetition (IEEE 1800-2023 16.9.2)");
            पटिनिवत्त BuildResult::failWithError();
        }
        ध्रुव unsigned minN = getConstUInt(repp->countp());

        // Sum sites across prefix + unbounded/range tail so one hoist covers
        // every check edge of this repetition.
        unsigned totalSites = minN;
        सचे (repp->unbounded()) {
            totalSites += 1;
        } अञ्ञथा सचे (repp->maxCountp()) {
            totalSites += getConstUInt(repp->maxCountp()) - minN;
        }
        AstVar* ध्रुव hoistVarp = tryHoistSampled(exprp, flp, totalSites);

        // Cover-sequence (IEEE 1800-2023 16.14.3): collect each end-of-match
        // position so they all fire the action, not just the merged terminal.
        std::vector<SvaStateVertex*> consMidSources;

        SvaStateVertex* currentp = entryVtxp;
        अत्थाय (unsigned i = 0; i < minN; ++i) {
            // Keep the first repetition explicit, collapse all remaining checks into the ring.
            सचे (i == 1) {
                currentp = addDelayChain(currentp, minN - 1, flp);
                currentp->m_delayRingClearCondp
                    = नूतन AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                currentp->m_replayAbortReject = सच्च;
                सचे (is_शिखरम्_Level_Step) {
                    currentp->m_throughoutConds.प्रक्षिप_back(
                        sampledRefOrClone(hoistVarp, exprp, flp));
                }
                i = minN - 1;
            }
            // Every repetition in the minimum prefix is required.
            SvaStateVertex* ध्रुव condVtxp = scopedCreateVertex();
            SvaTransEdge* ध्रुव linkp
                = guardedLink(currentp, condVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            // Only an outermost required step rejects the property. Its first check is explicit,
            // later required checks reject through the ring's throughout guard.
            सचे (is_शिखरम्_Level_Step && i == 0) linkp->m_rejectOnFail = सच्च;
            currentp = condVtxp;
        }
        // After minN: currentp is the first valid end-of-match position for [*m:n].
        सचे (m_isCoverSeq && (repp->unbounded() || repp->maxCountp())) {
            consMidSources.प्रक्षिप_back(currentp);
        }

        सचे (repp->unbounded()) {
            सचे (minN == 0) {
                SvaStateVertex* ध्रुव waitVtxp = scopedCreateVertex();
                guardedEdge(currentp, waitVtxp, flp);
                SvaStateVertex* ध्रुव checkVtxp = scopedCreateVertex();
                guardedLink(waitVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedEdge(checkVtxp, waitVtxp, flp);
                guardedLink(currentp, checkVtxp, flp);
                currentp = checkVtxp;
            } अञ्ञथा {
                SvaStateVertex* ध्रुव loopBackVtxp = scopedCreateVertex();
                guardedEdge(currentp, loopBackVtxp, flp);
                SvaStateVertex* ध्रुव reCheckVtxp = scopedCreateVertex();
                guardedLink(loopBackVtxp, reCheckVtxp, sampledRefOrClone(hoistVarp, exprp, flp),
                            flp);
                guardedEdge(reCheckVtxp, loopBackVtxp, flp);
                guardedLink(reCheckVtxp, currentp, flp);
            }
            currentp->m_isUnbounded = सच्च;
            m_inUnboundedScope = सच्च;
        } अञ्ञथा सचे (repp->maxCountp()) {
            ध्रुव unsigned maxN = getConstUInt(repp->maxCountp());
            SvaStateVertex* ध्रुव mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            unsigned tailMinN = minN;
            SvaStateVertex* tailStartp = currentp;
            सचे (minN == 0) {
                // Build the first optional iteration explicitly. Feeding the empty endpoint
                // directly into a range ring would incorrectly keep that match alive.
                SvaStateVertex* ध्रुव nextVtxp = scopedCreateVertex();
                guardedEdge(currentp, nextVtxp, flp);
                SvaStateVertex* ध्रुव checkVtxp = scopedCreateVertex();
                guardedLink(nextVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                सचे (m_isCoverSeq) consMidSources.प्रक्षिप_back(checkVtxp);
                tailStartp = checkVtxp;
                tailMinN = 1;
            }
            सचे (maxN > tailMinN) {
                // Add tail-ring only if the tail is non-empty.
                SvaStateVertex* ध्रुव nextVtxp
                    = addDelayChain(tailStartp, maxN - tailMinN + 1, flp, मिच्छा);
                nextVtxp->m_delayRingClearCondp
                    = नूतन AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                nextVtxp->m_replayAbortReject = सच्च;
                SvaStateVertex* ध्रुव checkVtxp = scopedCreateVertex();
                guardedLink(nextVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                सचे (m_isCoverSeq) consMidSources.प्रक्षिप_back(checkVtxp);
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
        पटिनिवत्त res;
    }

    // always[lo:hi] / s_always[lo:hi] (IEEE 1800-2023 16.12.11).
    BuildResult buildPropAlways(AstPropAlways* nodep, SvaStateVertex* entryVtxp,
                                तार्किक is_शिखरम्_Level_Step = मिच्छा) {
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव propp = nodep->propp();
        ध्रुव unsigned lo = getConstUInt(nodep->loBoundp());
        सचे (VN_IS(nodep->hiBoundp(), Unbounded)) {
            // Weak always [lo:$]: unbounded upper bound (IEEE 1800-2023 16.12.11).
            // p must hold at every clock tick at least lo cycles after the attempt
            // start; those ticks are not required to exist, so there is no
            // end-of-trace obligation (weak). The self-loop keeps the attempt live
            // every cycle; each observed cycle is a safety obligation, so a false p
            // rejects immediately.
            UASSERT_OBJ(!nodep->isStrong(), nodep, "Unbounded always must be weak (V3Width)");
            SvaStateVertex* ध्रुव livep = addDelayChain(entryVtxp, lo, flp);
            livep->m_isUnbounded = सच्च;
            guardedEdge(livep, livep, flp);  // stay active every subsequent cycle
            SvaStateVertex* ध्रुव sinkp = m_graph.createStateVertex();
            sinkp->m_isRejectSink = सच्च;
            SvaTransEdge* ध्रुव rejEdgep
                = guardedLink(livep, sinkp, sampled(propp->clone_वृक्षः_Pure(मिच्छा)), flp);
            सचे (is_शिखरम्_Level_Step) rejEdgep->m_rejectOnFail = सच्च;
            पटिनिवत्त {livep, nullptr, {}};
        }
        ध्रुव unsigned hi = getConstUInt(nodep->hiBoundp());
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
        SvaStateVertex* ध्रुव checkp = scopedCreateVertex();
        SvaTransEdge* ध्रुव linkp
            = guardedLink(currentp, checkp, sampled(propp->clone_वृक्षः_Pure(मिच्छा)), flp);
        सचे (is_शिखरम्_Level_Step) linkp->m_rejectOnFail = सच्च;
        currentp = checkp;
        m_temporal_Guard_राशिः.प्रक्षिप_back(propp);
        currentp = addDelayChain(currentp, hi - lo, flp);
        सचे (nodep->isStrong() && currentp->m_delayRingSize) currentp->m_strongAlwaysRing = सच्च;
        m_temporal_Guard_राशिः.निष्कासय_back();
        पटिनिवत्त {currentp, propp, {}};
    }

    BuildResult buildGotoRep(AstSGotoRep* repp, SvaStateVertex* entryVtxp) {
        File_पङ्क्तिः* ध्रुव flp = repp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = repp->exprp();
        ध्रुव unsigned minN = getConstUInt(repp->countp());
        सचे (minN == 0) पटिनिवत्त BuildResult::fail();
        ध्रुव तार्किक hasMax = repp->maxCountp() != nullptr;
        ध्रुव unsigned maxN = hasMax ? getConstUInt(repp->maxCountp()) : minN;
        सचे (m_isCoverSeq) {
            // Several attempts may coalesce in the one-bit wait vertex before reaching the ring,
            // so a cover sequence action block could run too few times.
            warnEndpointUnsupported(flp, "a goto repetition");
            पटिनिवत्त BuildResult::failWithError();
        }

        AstVar* ध्रुव hoistVarp = tryHoistSampled(exprp, flp, 2);

        // The first guardedEdge is the ##1 before waiting for a match. In the wait state, false
        // takes the clocked self-loop, while true takes the zero-delay guardedLink on that tick.
        SvaStateVertex* ध्रुव waitVtxp = scopedCreateVertex();
        guardedEdge(entryVtxp, waitVtxp, flp);
        guardedEdge(waitVtxp, waitVtxp,
                    नूतन AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)}, flp);
        SvaStateVertex* currentp = scopedCreateVertex();
        guardedLink(waitVtxp, currentp, sampledRefOrClone(hoistVarp, exprp, flp), flp);

        सचे (minN > 1) {
            SvaStateVertex* ध्रुव ringVtxp = addDelayChain(
                currentp, minN - 1, flp, सच्च, nullptr, sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = सच्च;
            SvaStateVertex* ध्रुव checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            currentp = checkVtxp;
        }
        सचे (!hasMax) {
            currentp->m_isUnbounded = सच्च;  // [->N] waits unboundedly
            m_inUnboundedScope = सच्च;
            पटिनिवत्त {currentp, nullptr, {}};
        }

        // [->M:N]: the range ring holds matches from M through N and advances
        // only on expr, preserving arbitrarily long gaps between occurrences.
        SvaStateVertex* ध्रुव mergeVtxp = scopedCreateVertex();
        guardedLink(currentp, mergeVtxp, flp);  // accept at match_M
        सचे (maxN > minN) {
            SvaStateVertex* ध्रुव ringVtxp
                = addDelayChain(currentp, maxN - minN + 1, flp, मिच्छा, nullptr,
                                sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = सच्च;
            SvaStateVertex* ध्रुव checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            guardedLink(checkVtxp, mergeVtxp, flp);
        }
        mergeVtxp->m_isUnbounded = सच्च;  // [->M:N] still has unbounded waits between matches
        m_inUnboundedScope = सच्च;
        पटिनिवत्त {mergeVtxp, nullptr, {}};
    }

    // True if the sequence has a zero-minimum consecutive repetition, which may match empty
    स्थिर तार्किक hasZeroMinRep(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* seqp) {
        पटिनिवत्त seqp->exists(
            [](ध्रुव AstSConsRep* repp) { पटिनिवत्त getConstUInt(repp->countp()) == 0; });
    }

    // True if a leaf's condition has no implication or multi-cycle sequence in it
    // (a leaf can be e.g. 'not (x |-> y)')
    स्थिर तार्किक isPlainBoolean(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* condp) {
        पटिनिवत्त !condp->exists(
            [](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* np) { पटिनिवत्त np->isMultiCycleSva() || VN_IS(np, Implication); });
    }

    // Build merge vertex for SOr / LogOr: both branches feed into one vertex.
    BuildResult buildOrMerge(Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, SvaStateVertex* entryVtxp,
                             File_पङ्क्तिः* flp, तार्किक wantBoolLeaf) {
        ध्रुव BuildResult lhs = build_व्यञ्जकम्(lhsp, entryVtxp);
        ध्रुव BuildResult rhs = build_व्यञ्जकम्(rhsp, entryVtxp);
        सचे (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            पटिनिवत्त BuildResult::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP
        // IEEE 1800-2023 16.14.3: a cover sequence counts every end-of-match. A
        // sequence operand of 'or' can end more than once, but only its final
        // end reaches the merge vertex below, so reject sequence operands rather
        // than under-count. Plain boolean disjunction has one end per cycle and
        // is handled by the OR-fold.
        ध्रुव तार्किक booleanOnly = lhs.termVertexp == entryVtxp && lhs.finalCondp
                                 && rhs.termVertexp == entryVtxp && rhs.finalCondp;
        सचे (m_isCoverSeq && !booleanOnly) {
            warnEndpointUnsupported(flp, "a sequence operand of 'or'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            पटिनिवत्त BuildResult::failWithError();
        }
        सचे (booleanOnly && wantBoolLeaf) {
            सचे (isPlainBoolean(lhs.finalCondp) && isPlainBoolean(rhs.finalCondp)) {
                // As a |-> / |=> or top-level body, a disjunction of two plain booleans must
                // be able to reject: return it as a boolean finalCondp. The merge vertex
                // below drops the attempt silently when both operands are false.
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव orCondp
                    = नूतन AstLogOr{flp, lhs.finalCondp->clone_वृक्षः_Pure(मिच्छा),
                                   rhs.finalCondp->clone_वृक्षः_Pure(मिच्छा)};
                freeUnlinkedCondp(lhs.finalCondp);
                freeUnlinkedCondp(rhs.finalCondp);
                पटिनिवत्त {entryVtxp, orCondp, {}};
            }
        }
        SvaStateVertex* ध्रुव mergeVtxp = scopedCreateVertex();
        सचे (booleanOnly) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsp,
                        "Single-cycle SOr operands must have finalCondp");
            guardedLink(entryVtxp, mergeVtxp,
                        नूतन AstLogOr{flp, sampled(lhs.finalCondp->clone_वृक्षः_Pure(मिच्छा)),
                                     sampled(rhs.finalCondp->clone_वृक्षः_Pure(मिच्छा))},
                        flp);
        } अञ्ञथा {
            सचे (lhs.finalCondp) {
                guardedLink(lhs.termVertexp, mergeVtxp,
                            sampled(lhs.finalCondp->clone_वृक्षः_Pure(मिच्छा)), flp);
            } अञ्ञथा {
                guardedLink(lhs.termVertexp, mergeVtxp, flp);
            }
            सचे (rhs.finalCondp) {
                guardedLink(rhs.termVertexp, mergeVtxp,
                            sampled(rhs.finalCondp->clone_वृक्षः_Pure(मिच्छा)), flp);
            } अञ्ञथा {
                guardedLink(rhs.termVertexp, mergeVtxp, flp);
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        पटिनिवत्त {mergeVtxp, nullptr, {}};
    }

    // Build done-latch combiner for SAnd/SIntersect (IEEE 1800-2023 16.9.5).
    BuildResult buildAndCombiner(Ast_ग्रन्थिः_व्यञ्जकम्* lhsExprp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsExprp,
                                 SvaStateVertex* entryVtxp, File_पङ्क्तिः* flp) {
        // Snapshot-restore scope so LHS liveness does not leak into RHS.
        ध्रुव तार्किक savedScope = m_inUnboundedScope;
        ध्रुव BuildResult lhs = build_व्यञ्जकम्(lhsExprp, entryVtxp);
        ध्रुव तार्किक lhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope;
        ध्रुव BuildResult rhs = build_व्यञ्जकम्(rhsExprp, entryVtxp);
        ध्रुव तार्किक rhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope || lhsScope || rhsScope;
        सचे (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            पटिनिवत्त BuildResult::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP

        // Single-cycle operands: use boolean AND (done-latch would fire across cycles).
        // If both operands stayed at entry, they must be boolean leaves which
        // buildExpr returns with finalCondp=nodep (non-null).
        सचे (lhs.termVertexp == entryVtxp && rhs.termVertexp == entryVtxp) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsExprp,
                        "Single-cycle SAnd operands must have finalCondp");
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = नूतन AstLogAnd{flp, lhs.finalCondp->clone_वृक्षः_Pure(मिच्छा),
                                                     rhs.finalCondp->clone_वृक्षः_Pure(मिच्छा)};
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            पटिनिवत्त {entryVtxp, condp, {}};
        }
        // Range-delay mid-window sources in either sub-branch would need
        // to be folded into the latch's match-now signal, which the
        // current combiner does not support.
        सचे (!lhs.midSources.empty() || !rhs.midSources.empty()) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: ranged cycle delay in an operand of property 'and'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            पटिनिवत्त BuildResult::failWithError();
        }
        SvaStateVertex* ध्रुव combVtxp = scopedCreateVertex();
        combVtxp->m_isAndCombiner = सच्च;
        combVtxp->m_andLhsTermp = lhs.termVertexp;
        combVtxp->m_andRhsTermp = rhs.termVertexp;
        सचे (lhs.finalCondp) combVtxp->m_andLhsCondp = lhs.finalCondp->clone_वृक्षः_Pure(मिच्छा);
        सचे (rhs.finalCondp) combVtxp->m_andRhsCondp = rhs.finalCondp->clone_वृक्षः_Pure(मिच्छा);
        सचे (lhs.termVertexp->m_isUnbounded || rhs.termVertexp->m_isUnbounded) {
            combVtxp->m_isUnbounded = सच्च;
        }
        // Wire terminal-boolean rejects to a dedicated sink so each side can fail
        // the AND independently. Skip for liveness or single-cycle operands
        // (single-cycle termVertexp == entryVtxp would fire every cycle).
        सचे (!combVtxp->m_isUnbounded) {
            तार्किक needSink = मिच्छा;
            ध्रुव तार्किक lhsMultiCycle = (lhs.termVertexp != entryVtxp);
            ध्रुव तार्किक rhsMultiCycle = (rhs.termVertexp != entryVtxp);
            सचे (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                needSink = सच्च;
            }
            सचे (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                needSink = सच्च;
            }
            सचे (needSink) {
                SvaStateVertex* ध्रुव sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = सच्च;
                सचे (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* ध्रुव ep = m_graph.addLink(
                        lhs.termVertexp, sinkVtxp, sampled(lhs.finalCondp->clone_वृक्षः_Pure(मिच्छा)));
                    ep->m_rejectOnFail = सच्च;
                }
                सचे (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* ध्रुव ep = m_graph.addLink(
                        rhs.termVertexp, sinkVtxp, sampled(rhs.finalCondp->clone_वृक्षः_Pure(मिच्छा)));
                    ep->m_rejectOnFail = सच्च;
                }
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        पटिनिवत्त {combVtxp, nullptr, {}};
    }

    // Lower `seq1 within seq2` (IEEE 1800-2023 16.9.10) as:
    //   (OR_{i in 0..slack} 1 ##i seq1 ##(slack-i) 1) intersect seq2
    // Both operands must have fixed length (no ranged cycle delays).
    // The OR lives inside a single SIntersect so one AndCombiner done-latch
    // serves all offsets; lifting it out would double-count matches where
    // multiple offsets accept on the same seq2 end cycle.
    BuildResult buildSWithin(AstSWithin* nodep, SvaStateVertex* entryVtxp,
                             तार्किक is_शिखरम्_Level_Step = मिच्छा) {
        ध्रुव पूर्णाङ्क innerLen = fixedLength(nodep->lhsp());
        ध्रुव पूर्णाङ्क outerLen = fixedLength(nodep->rhsp());
        सचे (innerLen < 0 || outerLen < 0) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: within with ranged cycle-delay operand");
            पटिनिवत्त BuildResult::failWithError();
        }
        सचे (innerLen > outerLen) {
            पटिनिवत्त buildNeverMatchIntersect(
                nodep, entryVtxp, is_शिखरम्_Level_Step,
                "the inner sequence is longer than the outer sequence");
        }
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        ध्रुव पूर्णाङ्क slack = outerLen - innerLen;
        Ast_ग्रन्थिः_व्यञ्जकम्* innerOrp = nullptr;
        अत्थाय (पूर्णाङ्क i = 0; i <= slack; ++i) {
            ध्रुव पूर्णाङ्क postPad = slack - i;
            Ast_ग्रन्थिः_व्यञ्जकम्* branchp = nodep->lhsp()->clone_वृक्षः_Pure(मिच्छा);
            सचे (i > 0) {
                AstConst* ध्रुव prePadp = नूतन AstConst{flp, AstConst::BitTrue{}};
                AstDelay* ध्रुव delayp
                    = नूतन AstDelay{flp, नूतन AstConst{flp, static_cast<uint32_t>(i)}, सच्च};
                AstSExpr* ध्रुव wrapped = नूतन AstSExpr{flp, prePadp, delayp, branchp};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            सचे (postPad > 0) {
                AstConst* ध्रुव postTruep = नूतन AstConst{flp, AstConst::BitTrue{}};
                AstDelay* ध्रुव delayp
                    = नूतन AstDelay{flp, नूतन AstConst{flp, static_cast<uint32_t>(postPad)}, सच्च};
                AstSExpr* ध्रुव wrapped = नूतन AstSExpr{flp, branchp, delayp, postTruep};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            innerOrp = innerOrp ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstSOr{flp, innerOrp, branchp})
                                : branchp;
        }
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव outerClonep = nodep->rhsp()->clone_वृक्षः_Pure(मिच्छा);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव combinedp = नूतन AstSIntersect{flp, innerOrp, outerClonep};
        BuildResult result = build_व्यञ्जकम्(combinedp, entryVtxp, is_शिखरम्_Level_Step);
        VL_DO_DANGLING(combinedp->delete_वृक्षः(), combinedp);
        // When both operands are plain booleans, buildAndCombiner returns a
        // freshly-allocated AstAnd as finalCondp with no parent. Callers up
        // the chain clone-and-discard finalCondp, so leave it parent-less and
        // it leaks; anchor it in the graph now via an edge.
        सचे (result.valid() && result.finalCondp && !result.finalCondp->backp()) {
            SvaStateVertex* ध्रुव wrapVtxp = scopedCreateVertex();
            guardedLink(result.termVertexp, wrapVtxp, sampled(result.finalCondp), flp);
            result = {wrapVtxp, nullptr, result.midSources};
        }
        पटिनिवत्त result;
    }

    // Collect the boolean leaf checks of a fixed-length sequence keyed by their
    // clock offset from the start. Returns false for anything other than nested
    // AstSExpr with fixed cycle delays over boolean leaves (e.g. throughout).
    स्थिर तार्किक flattenFixedSeq(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, पूर्णाङ्क baseOffset,
                                std::map<पूर्णाङ्क, std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*>>& out) {
        सचे (AstSExpr* ध्रुव sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ध्रुव delayp = VN_CAST(sexprp->delayp(), Delay);
            सचे (!delayp || !delayp->isCycleDelay() || delayp->isUnbounded()) पटिनिवत्त मिच्छा;
            ध्रुव unsigned delayCycles = getConstUInt(delayp->lhsp());
            सचे (delayp->isRangeDelay() && getConstUInt(delayp->rhsp()) != delayCycles)
                पटिनिवत्त मिच्छा;
            पूर्णाङ्क preLen = 0;
            सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव prep = sexprp->preExprp()) {
                सचे (!flattenFixedSeq(prep, baseOffset, out)) पटिनिवत्त मिच्छा;
                preLen = fixedLength(prep);
                सचे (preLen < 0) पटिनिवत्त मिच्छा;
            }
            पटिनिवत्त flattenFixedSeq(sexprp->exprp(), baseOffset + preLen + delayCycles, out);
        }
        सचे (nodep->isMultiCycleSva()) पटिनिवत्त मिच्छा;
        out[baseOffset].प्रक्षिप_back(nodep);
        पटिनिवत्त सच्च;
    }

    // Conjoin two equal-length fixed sequences into one: at each clock offset
    // AND the boolean checks of both operands (IEEE 1800-2023 16.9.6 -- both
    // operands match the same window). Returns null if either operand is not a
    // plain fixed sequence of boolean leaves.
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* conjoinFixedSeqs(Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, File_पङ्क्तिः* flp) {
        std::map<पूर्णाङ्क, std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*>> checks;
        सचे (!flattenFixedSeq(lhsp, 0, checks) || !flattenFixedSeq(rhsp, 0, checks)) पटिनिवत्त nullptr;
        सचे (checks.empty()) पटिनिवत्त nullptr;
        Ast_ग्रन्थिः_व्यञ्जकम्* resultp = nullptr;
        पूर्णाङ्क prevOffset = 0;
        अत्थाय (ध्रुव auto& offsetChecks : checks) {
            ध्रुव पूर्णाङ्क offset = offsetChecks.first;
            Ast_ग्रन्थिः_व्यञ्जकम्* condp = nullptr;
            अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव leafp : offsetChecks.second) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव clonep = leafp->clone_वृक्षः_Pure(मिच्छा);
                सचे (!condp) {
                    condp = clonep;
                } अञ्ञथा {
                    condp = नूतन AstLogAnd{flp, condp, clonep};
                    condp->dtypeSetBit();
                }
            }
            सचे (!resultp) {
                सचे (offset > 0) {
                    AstDelay* ध्रुव delayp = नूतन AstDelay{
                        flp, नूतन AstConst{flp, static_cast<uint32_t>(offset)}, /*isCycle=*/सच्च};
                    resultp
                        = नूतन AstSExpr{flp, नूतन AstConst{flp, AstConst::BitTrue{}}, delayp, condp};
                    resultp->dtypeSetBit();
                } अञ्ञथा {
                    resultp = condp;
                }
            } अञ्ञथा {
                AstDelay* ध्रुव delayp = नूतन AstDelay{
                    flp, नूतन AstConst{flp, static_cast<uint32_t>(offset - prevOffset)},
                    /*isCycle=*/सच्च};
                resultp = नूतन AstSExpr{flp, resultp, delayp, condp};
                resultp->dtypeSetBit();
            }
            prevOffset = offset;
        }
        पटिनिवत्त resultp;
    }

    // `seq` is a simple ranged sequence `start ##[m:n] end` (start/end boolean,
    // start may be absent). Used to collapse a both-variable intersect to one
    // ranged delay.
    संरचना SimpleRanged final {
        तार्किक ok = मिच्छा;
        Ast_ग्रन्थिः_व्यञ्जकम्* startp = nullptr;  // may be null (absent start)
        Ast_ग्रन्थिः_व्यञ्जकम्* endp = nullptr;
    };
    स्थिर SimpleRanged asSimpleRanged(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        AstSExpr* ध्रुव sexprp = VN_CAST(nodep, SExpr);
        सचे (!sexprp) पटिनिवत्त {};
        AstDelay* ध्रुव delayp = VN_CAST(sexprp->delayp(), Delay);
        सचे (!delayp || !delayp->isCycleDelay() || !delayp->isRangeDelay() || delayp->isUnbounded())
            पटिनिवत्त {};
        सचे (getConstUInt(delayp->lhsp()) == getConstUInt(delayp->rhsp())) पटिनिवत्त {};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव prep = sexprp->preExprp();
        सचे (prep && fixedLength(prep) != 0) पटिनिवत्त {};
        सचे (fixedLength(sexprp->exprp()) != 0) पटिनिवत्त {};
        पटिनिवत्त {सच्च, prep, sexprp->exprp()};
    }

    // Build the NFA for a synthesized intersect lowering tree, then free it.
    // buildExpr returns the terminal condition (finalCondp) by reference into the
    // tree; detach a clone so the tree can be freed here. The graph already holds
    // clones/hoists of every edge condition, so nothing else dangles.
    BuildResult build_From_Lowering_वृक्षः(Ast_ग्रन्थिः_व्यञ्जकम्* treep, SvaStateVertex* entryVtxp,
                                      तार्किक is_शिखरम्_Level_Step) {
        BuildResult result = build_व्यञ्जकम्(treep, entryVtxp, is_शिखरम्_Level_Step);
        सचे (result.valid() && result.finalCondp) {
            result.finalCondp = result.finalCondp->clone_वृक्षः_Pure(मिच्छा);
        }
        VL_DO_DANGLING(treep->delete_वृक्षः(), treep);
        पटिनिवत्त result;
    }

    // Empty common-length intersection -- unequal fixed lengths, or disjoint
    // ranged lengths. IEEE 1800-2023 16.9.6 requires both operands to match
    // over a window of the same length, so with no common length the intersect
    // simply never matches. This is legal (matching nothing), not an error, so
    // lower to a constant false rather than rejecting legal code.
    BuildResult buildNeverMatchIntersect(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, SvaStateVertex* entryVtxp,
                                         तार्किक is_शिखरम्_Level_Step, ध्रुव वर्ण* reason) {
        nodep->v3warn(NEVERMATCH, "Sequence can never match because " << reason << ".");
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव falsep = नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}};
        पटिनिवत्त build_From_Lowering_वृक्षः(falsep, entryVtxp, is_शिखरम्_Level_Step);
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
                                     तार्किक is_शिखरम्_Level_Step) {
        ध्रुव std::pair<पूर्णाङ्क, पूर्णाङ्क> lhsRange = lengthRange(nodep->lhsp());
        ध्रुव std::pair<पूर्णाङ्क, पूर्णाङ्क> rhsRange = lengthRange(nodep->rhsp());
        सचे (lhsRange.first < 0 || rhsRange.first < 0) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect with this variable-length operand");
            पटिनिवत्त BuildResult::failWithError();
        }
        ध्रुव पूर्णाङ्क lo = std::max(lhsRange.first, rhsRange.first);
        ध्रुव पूर्णाङ्क hi = std::min(lhsRange.second, rhsRange.second);
        सचे (lo > hi) {
            // Disjoint length ranges share no common length -> never matches.
            पटिनिवत्त buildNeverMatchIntersect(nodep, entryVtxp, is_शिखरम्_Level_Step,
                                            "intersect operands have no common length");
        }
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        सचे (lo == hi) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lp = realizeAtLength(nodep->lhsp(), lo, lhsRange.first);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rp = realizeAtLength(nodep->rhsp(), lo, rhsRange.first);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव conjp = conjoinFixedSeqs(lp, rp, flp);
            VL_DO_DANGLING(lp->delete_वृक्षः(), lp);
            VL_DO_DANGLING(rp->delete_वृक्षः(), rp);
            सचे (!conjp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: intersect operand is not a plain boolean sequence");
                पटिनिवत्त BuildResult::failWithError();
            }
            पटिनिवत्त build_From_Lowering_वृक्षः(conjp, entryVtxp, is_शिखरम्_Level_Step);
        }
        ध्रुव SimpleRanged sl = asSimpleRanged(nodep->lhsp());
        ध्रुव SimpleRanged sr = asSimpleRanged(nodep->rhsp());
        सचे (!sl.ok || !sr.ok) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect of two sequences that each vary in length over a"
                          " range with internal structure");
            पटिनिवत्त BuildResult::failWithError();
        }
        ध्रुव auto andBool = [&](Ast_ग्रन्थिः_व्यञ्जकम्* ap, Ast_ग्रन्थिः_व्यञ्जकम्* bp) -> Ast_ग्रन्थिः_व्यञ्जकम्* {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव aClonep
                = ap ? ap->clone_वृक्षः_Pure(मिच्छा) : नूतन AstConst{flp, AstConst::BitTrue{}};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bClonep
                = bp ? bp->clone_वृक्षः_Pure(मिच्छा) : नूतन AstConst{flp, AstConst::BitTrue{}};
            AstLogAnd* ध्रुव andp = नूतन AstLogAnd{flp, aClonep, bClonep};
            andp->dtypeSetBit();
            पटिनिवत्त andp;
        };
        AstDelay* ध्रुव delayp = नूतन AstDelay{flp, नूतन AstConst{flp, static_cast<uint32_t>(lo)},
                                              /*isCycle=*/सच्च};
        delayp->rhsp(नूतन AstConst{flp, static_cast<uint32_t>(hi)});
        AstSExpr* ध्रुव reducedp
            = नूतन AstSExpr{flp, andBool(sl.startp, sr.startp), delayp, andBool(sl.endp, sr.endp)};
        reducedp->dtypeSetBit();
        पटिनिवत्त build_From_Lowering_वृक्षः(reducedp, entryVtxp, is_शिखरम्_Level_Step);
    }

    BuildResult buildThroughout(AstSThroughout* nodep, SvaStateVertex* entryVtxp,
                                तार्किक is_शिखरम्_Level_Step = मिच्छा) {
        // Mark entryVtxp so "cond false at tick 0" is detected as throughout-drop.
        entryVtxp->m_throughoutConds.प्रक्षिप_back(nodep->lhsp()->clone_वृक्षः_Pure(मिच्छा));
        m_temporal_Guard_राशिः.प्रक्षिप_back(nodep->lhsp());
        ध्रुव BuildResult result = build_व्यञ्जकम्(nodep->rhsp(), entryVtxp, is_शिखरम्_Level_Step);
        m_temporal_Guard_राशिः.निष्कासय_back();
        पटिनिवत्त result;
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
    BuildResult buildUntil(AstUntil* nodep, SvaStateVertex* entryVtxp, तार्किक is_शिखरम्_Level_Step) {
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        सचे (!is_शिखरम्_Level_Step) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            पटिनिवत्त BuildResult::failWithError();
        }
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp();
        ध्रुव auto hasSeq = [](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ep) {
            पटिनिवत्त ep->exists([](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* np) { पटिनिवत्त np->isMultiCycleSva(); });
        };
        सचे (hasSeq(lhsp) || hasSeq(rhsp)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            पटिनिवत्त BuildResult::failWithError();
        }

        ध्रुव तार्किक ov = nodep->isOverlapping();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsBitp = nodep->lhsp();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsBitp = nodep->rhsp();
        // p hoist count: continue, require (ov: 1 use; nov: 1 use). At least 2 uses.
        AstVar* ध्रुव pHoistp = tryHoistSampled(lhsBitp, flp, 2);
        // q hoist count: continue (1) + require nov (1) = 2; ov: continue only (1).
        AstVar* ध्रुव qHoistp = ov ? nullptr : tryHoistSampled(rhsBitp, flp, 2);

        SvaStateVertex* ध्रुव waitCp = scopedCreateVertex();
        SvaStateVertex* ध्रुव waitRp = scopedCreateVertex();
        waitCp->m_isUnbounded = सच्च;
        waitRp->m_strongPending = nodep->isStrong() && !m_isCover;

        // Entry and back-loop Links carry no condition; throughout-folding still applies.
        guardedLink(entryVtxp, waitCp, flp);
        guardedLink(waitRp, waitCp, flp);

        // Continue clocked edge: p && !q advances to next-cycle wait.
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव contCondp
            = नूतन AstLogAnd{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                            नूतन AstLogNot{flp, sampledRefOrClone(qHoistp, rhsBitp, flp)}};
        guardedEdge(waitCp, waitRp, contCondp, flp);

        // Reject sink: fires when require-condition is false.
        SvaStateVertex* ध्रुव sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = सच्च;
        Ast_ग्रन्थिः_व्यञ्जकम्* requireCondp;
        सचे (ov) {
            requireCondp = sampledRefOrClone(pHoistp, lhsBitp, flp);
        } अञ्ञथा {
            requireCondp = नूतन AstLogOr{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                                        sampledRefOrClone(qHoistp, rhsBitp, flp)};
        }
        SvaTransEdge* ध्रुव rejEdgep = m_graph.addLink(waitCp, sinkVtxp, requireCondp);
        rejEdgep->m_rejectOnFail = सच्च;

        // Accept condition rides via finalCondp; assembleResult $sampled-wraps it.
        Ast_ग्रन्थिः_व्यञ्जकम्* acceptCondp;
        सचे (ov) {
            acceptCondp
                = नूतन AstLogAnd{flp, lhsBitp->clone_वृक्षः_Pure(मिच्छा), rhsBitp->clone_वृक्षः_Pure(मिच्छा)};
        } अञ्ञथा {
            acceptCondp = rhsBitp->clone_वृक्षः_Pure(मिच्छा);
        }
        पटिनिवत्त {waitCp, acceptCondp, {}};
    }

    // IEEE 1800-2023 16.12.14 property abort operators. Sync and async share
    // the same NFA encoding: AstSampled already gives matured values at every
    // maturing clocking event, and async firing "between clocks" is not
    // observable in a cycle-based model. VAbortKind selects accept vs reject
    // verdict (sync vs async only changes the user-visible spelling).

    // Build `condp && !outer_1 && !outer_2 ...` (unsampled).
    Ast_ग्रन्थिः_व्यञ्जकम्* abort_Fire_व्यञ्जकम्(Ast_ग्रन्थिः_व्यञ्जकम्* condp, File_पङ्क्तिः* flp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* resultp = condp->clone_वृक्षः_Pure(मिच्छा);
        अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव op : m_outer_Abort_राशिः)
            resultp = नूतन AstLogAnd{flp, resultp, नूतन AstLogNot{flp, op->clone_वृक्षः_Pure(मिच्छा)}};
        पटिनिवत्त resultp;
    }

    // True when unguarded ring-wide rejection or a same-tick Link chain already accounts the
    // attempt: a required-step Link covers both outcomes; followed-by pairs both edges.
    स्थिर तार्किक chainAccountsSource(ध्रुव SvaStateVertex* srcp,
                                    ध्रुव std::unordered_set<ध्रुव V3GraphEdge*>& preEdges) {
        सचे (srcp->m_delayRingSize && srcp->m_throughoutConds.empty()) पटिनिवत्त सच्च;
        अत्थाय (ध्रुव V3GraphEdge& edger : srcp->inEdges()) {
            सचे (preEdges.गणनम्(&edger)) अनुवर्त;
            ध्रुव SvaTransEdge& tedger = static_cast<ध्रुव SvaTransEdge&>(edger);
            सचे (tedger.m_consumesCycle) अनुवर्त;
            ध्रुव auto* ध्रुव fromp = static_cast<ध्रुव SvaStateVertex*>(tedger.fromVtxp());
            सचे (fromp->m_abortRejectp) पटिनिवत्त सच्च;
        }
        तार्किक plainNonSink = मिच्छा;
        तार्किक markedSink = मिच्छा;
        अत्थाय (ध्रुव V3GraphEdge& edger : srcp->outEdges()) {
            सचे (preEdges.गणनम्(&edger)) अनुवर्त;
            ध्रुव SvaTransEdge& tedger = static_cast<ध्रुव SvaTransEdge&>(edger);
            सचे (tedger.m_consumesCycle) अनुवर्त;
            ध्रुव तार्किक sink = static_cast<ध्रुव SvaStateVertex*>(tedger.toVtxp())->m_isRejectSink;
            सचे (tedger.m_rejectOnFail) {
                सचे (!sink) पटिनिवत्त सच्च;
                markedSink = सच्च;
            } अञ्ञथा सचे (!sink) {
                सचे (srcp->m_abortRejectp) पटिनिवत्त सच्च;
                plainNonSink = सच्च;
            }
        }
        पटिनिवत्त plainNonSink && markedSink;
    }

    // Reject edge: fires when the source is live and the abort samples true.
    शून्य addAbortRejectEdge(SvaStateVertex* srcp, SvaStateVertex* sinkp, Ast_ग्रन्थिः_व्यञ्जकम्* condp,
                            File_पङ्क्तिः* flp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notFirep = नूतन AstLogNot{flp, sampled(abort_Fire_व्यञ्जकम्(condp, flp))};
        m_graph.addLink(srcp, sinkp, notFirep)->m_rejectOnFail = सच्च;
        पटिनिवत्त;
    }

    // On the fire tick: kill body threads; accept kinds also forgive step misses.
    शून्य gateBodyEdgesOnAbort(ध्रुव std::unordered_set<ध्रुव V3GraphEdge*>& preEdges,
                              Ast_ग्रन्थिः_व्यञ्जकम्* condp, VAbortKind kind, File_पङ्क्तिः* flp) {
        अत्थाय (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            अत्थाय (V3GraphEdge& edger : vtxr.outEdges()) {
                सचे (preEdges.गणनम्(&edger)) अनुवर्त;
                SvaTransEdge* ध्रुव tedgep = static_cast<SvaTransEdge*>(&edger);
                सचे (tedgep->m_rejectOnFail) {
                    सचे (!kind.isAccept()) अनुवर्त;
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव firep = sampled(abort_Fire_व्यञ्जकम्(condp, flp));
                    tedgep->m_condp
                        = tedgep->m_condp ? नूतन AstLogOr{flp, tedgep->m_condp, firep} : firep;
                } अञ्ञथा सचे (tedgep->m_consumesCycle) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notFirep
                        = नूतन AstLogNot{flp, sampled(abort_Fire_व्यञ्जकम्(condp, flp))};
                    tedgep->m_condp = tedgep->m_condp
                                          ? नूतन AstLogAnd{flp, tedgep->m_condp, notFirep}
                                          : notFirep;
                }
            }
        }
    }

    BuildResult buildAbortOn(Ast_ग्रन्थिः_व्यञ्जकम्* condp, Ast_ग्रन्थिः_व्यञ्जकम्* bodyp, SvaStateVertex* entryVtxp,
                             VAbortKind kind, File_पङ्क्तिः* flp, तार्किक is_शिखरम्_Level_Step) {
        // Snapshot pre-body vertices/edges so post-build diff yields the body's sub-NFA.
        std::unordered_set<ध्रुव V3GraphVertex*> preExisting;
        std::unordered_set<ध्रुव V3GraphEdge*> preEdges;
        अत्थाय (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            preExisting.निवेशय(&vtxr);
            अत्थाय (V3GraphEdge& edger : vtxr.outEdges()) preEdges.निवेशय(&edger);
        }

        m_outer_Abort_राशिः.प्रक्षिप_back(condp);
        ध्रुव BuildResult bodyResult = build_व्यञ्जकम्(bodyp, entryVtxp, is_शिखरम्_Level_Step);
        m_outer_Abort_राशिः.निष्कासय_back();
        सचे (!bodyResult.valid()) पटिनिवत्त bodyResult;

        gateBodyEdgesOnAbort(preEdges, condp, kind, flp);

        // Live-thread sources for the abort edge: entry + new body vertices,
        // minus reject sinks (they carry reject fuel, not live-thread fuel).
        std::vector<SvaStateVertex*> abortSources;
        abortSources.प्रक्षिप_back(entryVtxp);
        अत्थाय (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            सचे (preExisting.गणनम्(&vtxr)) अनुवर्त;
            auto* ध्रुव sp = static_cast<SvaStateVertex*>(&vtxr);
            सचे (sp->m_delayRingSize || !sp->m_throughoutConds.empty()) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव firep = abort_Fire_व्यञ्जकम्(condp, flp);
                sp->m_abortClearp
                    = sp->m_abortClearp ? नूतन AstLogOr{flp, sp->m_abortClearp, firep} : firep;
            }
            सचे (!kind.isAccept()
                && ((sp->m_delayRingSize && sp->m_throughoutConds.empty())
                    || sp->m_replayAbortReject)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव firep = abort_Fire_व्यञ्जकम्(condp, flp);
                sp->m_abortRejectp
                    = sp->m_abortRejectp ? नूतन AstLogOr{flp, sp->m_abortRejectp, firep} : firep;
            }
            सचे (sp->m_isRejectSink) अनुवर्त;
            abortSources.प्रक्षिप_back(sp);
        }

        auto sampledAbortFire = [&]() -> Ast_ग्रन्थिः_व्यञ्जकम्* {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव व्यञ्जकम् = abort_Fire_व्यञ्जकम्(condp, flp);
            पटिनिवत्त sampled(व्यञ्जकम्);
        };

        सचे (kind.isAccept()) {
            // Match-only sink fed by $sampled(abort-fire) from every live source;
            // registered as midSource so it never contributes a reject.
            SvaStateVertex* ध्रुव acceptSinkp = scopedCreateVertex();
            अत्थाय (SvaStateVertex* ध्रुव srcp : abortSources)
                guardedLink(srcp, acceptSinkp, sampledAbortFire(), flp);
            std::vector<SvaStateVertex*> midSources = bodyResult.midSources;
            midSources.प्रक्षिप_back(acceptSinkp);
            Ast_ग्रन्थिः_व्यञ्जकम्* finalCondp = bodyResult.finalCondp;
            सचे (finalCondp) {
                सचे (finalCondp->backp()) finalCondp = finalCondp->clone_वृक्षः_Pure(मिच्छा);
                finalCondp = नूतन AstLogOr{flp, finalCondp, abort_Fire_व्यञ्जकम्(condp, flp)};
            }
            पटिनिवत्त {bodyResult.termVertexp, finalCondp, std::move(midSources)};
        }

        // rejectOnFail treats m_condp as the success condition and fires on
        // !condp, so the edge carries !sampledAbortFire().
        SvaStateVertex* ध्रुव rejectSinkp = m_graph.createStateVertex();
        rejectSinkp->m_isRejectSink = सच्च;
        अत्थाय (SvaStateVertex* ध्रुव srcp : abortSources)
            सचे (!chainAccountsSource(srcp, preEdges))
                addAbortRejectEdge(srcp, rejectSinkp, condp, flp);
        Ast_ग्रन्थिः_व्यञ्जकम्* finalCondp = bodyResult.finalCondp;
        सचे (finalCondp) {
            सचे (finalCondp->backp()) finalCondp = finalCondp->clone_वृक्षः_Pure(मिच्छा);
            finalCondp
                = नूतन AstLogAnd{flp, finalCondp, नूतन AstLogNot{flp, abort_Fire_व्यञ्जकम्(condp, flp)}};
        }
        पटिनिवत्त {bodyResult.termVertexp, finalCondp, bodyResult.midSources};
    }

सार्वजनिक:
    // Free a dropped sub-result condition that is not linked into the AST
    // (abort folds synthesize unparented finalCondp trees).
    स्थिर शून्य freeUnlinkedCondp(Ast_ग्रन्थिः_व्यञ्जकम्* condp) {
        सचे (condp && !condp->backp()) VL_DO_DANGLING(condp->delete_वृक्षः(), condp);
    }

    SvaNfaBuilder(SvaGraph& graph, Ast_ग्रन्थिः_Module* modp, V3UniqueNames& propTempNames,
                  तार्किक isCoverSeq = मिच्छा, तार्किक isSeqEvent = मिच्छा, तार्किक isCover = मिच्छा)
        : m_graph{graph}
        , m_modp{modp}
        , m_propTempNames{propTempNames}
        , m_isCover{isCover}
        , m_isCoverSeq{isCoverSeq}
        , m_isSeqEvent{isSeqEvent} {}

    // Reset scope between antecedent and consequent: liveness must not leak.
    // m_outerAbortStack survives: an abort wrapping the implication covers the
    // consequent too (IEEE 1800-2023 16.12.14).
    शून्य resetScope() {
        m_inUnboundedScope = मिच्छा;
        m_temporal_Guard_राशिः.clear();
    }

    BuildResult build_व्यञ्जकम्(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, SvaStateVertex* entryVtxp,
                          तार्किक is_शिखरम्_Level_Step = मिच्छा) {
        VL_RESTORER(m_wantBoolLeaf);
        ध्रुव तार्किक wantBoolLeaf = m_wantBoolLeaf;
        m_wantBoolLeaf = मिच्छा;
        सचे (AstSExpr* ध्रुव sexprp = VN_CAST(nodep, SExpr)) {
            पटिनिवत्त buildSExpr(sexprp, entryVtxp, is_शिखरम्_Level_Step);
        }
        सचे (AstSConsRep* ध्रुव repp = VN_CAST(nodep, SConsRep)) {
            पटिनिवत्त buildConsRep(repp, entryVtxp, is_शिखरम्_Level_Step);
        }
        सचे (AstPropAlways* ध्रुव alwaysp = VN_CAST(nodep, PropAlways)) {
            पटिनिवत्त buildPropAlways(alwaysp, entryVtxp, is_शिखरम्_Level_Step);
        }
        सचे (AstSGotoRep* ध्रुव repp = VN_CAST(nodep, SGotoRep)) {
            पटिनिवत्त buildGotoRep(repp, entryVtxp);
        }
        सचे (AstSThroughout* ध्रुव throughoutp = VN_CAST(nodep, SThroughout)) {
            पटिनिवत्त buildThroughout(throughoutp, entryVtxp, is_शिखरम्_Level_Step);
        }
        सचे (AstSOr* ध्रुव orp = VN_CAST(nodep, SOr)) {
            पटिनिवत्त buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        सचे (AstLogOr* ध्रुव orp = VN_CAST(nodep, LogOr)) {
            पटिनिवत्त buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        सचे (AstSAnd* ध्रुव andp = VN_CAST(nodep, SAnd)) {
            पटिनिवत्त buildAndCombiner(andp->lhsp(), andp->rhsp(), entryVtxp, andp->fileline());
        }
        सचे (AstSIntersect* ध्रुव intp = VN_CAST(nodep, SIntersect)) {
            // IEEE 1800-2023 16.9.6: both operands match over one window with
            // equal start and end (equal length). Lower to a single sequence
            // that conjoins both operands' per-cycle checks -- correct under
            // concurrent attempts, where the done-latch combiner conflates the
            // two operands' start times and over-accepts. The combiner remains
            // only as a fallback for operands that do not flatten.
            ध्रुव पूर्णाङ्क lhsLen = fixedLength(intp->lhsp());
            ध्रुव पूर्णाङ्क rhsLen = fixedLength(intp->rhsp());
            सचे (lhsLen >= 0 && rhsLen >= 0) {
                सचे (lhsLen != rhsLen) {
                    // Unequal fixed lengths share no common length -> never matches.
                    पटिनिवत्त buildNeverMatchIntersect(intp, entryVtxp, is_शिखरम्_Level_Step,
                                                    "intersect operands have no common length");
                }
                सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव conjp
                    = conjoinFixedSeqs(intp->lhsp(), intp->rhsp(), intp->fileline())) {
                    पटिनिवत्त build_From_Lowering_वृक्षः(conjp, entryVtxp, is_शिखरम्_Level_Step);
                }
                पटिनिवत्त buildAndCombiner(intp->lhsp(), intp->rhsp(), entryVtxp, intp->fileline());
            }
            पटिनिवत्त buildVarLenIntersect(intp, entryVtxp, is_शिखरम्_Level_Step);
        }
        सचे (AstSWithin* ध्रुव withinp = VN_CAST(nodep, SWithin)) {
            पटिनिवत्त buildSWithin(withinp, entryVtxp, is_शिखरम्_Level_Step);
        }
        सचे (AstAbortOn* ध्रुव ap = VN_CAST(nodep, AbortOn)) {
            पटिनिवत्त buildAbortOn(ap->condp(), ap->propp(), entryVtxp, ap->kind(), ap->fileline(),
                                is_शिखरम्_Level_Step);
        }
        सचे (VN_IS(nodep, SNonConsRep)) पटिनिवत्त BuildResult::fail();
        सचे (AstImplication* ध्रुव implp = VN_CAST(nodep, Implication)) {
            पटिनिवत्त buildImplicationEdges(implp->lhsp(), implp->rhsp(), entryVtxp,
                                         implp->isOverlapped(), implp->isFollowedBy(),
                                         implp->lhsp(), implp->fileline());
        }
        सचे (AstUntil* ध्रुव untilp = VN_CAST(nodep, Until)) {
            पटिनिवत्त buildUntil(untilp, entryVtxp, is_शिखरम्_Level_Step);
        }
        // Boolean leaf (including LogAnd): return as finalCond
        पटिनिवत्त {entryVtxp, nodep, {}};
    }

    // Wire an implication / followed-by from `entryVtxp`: builds the antecedent,
    // emits the match-link (and for followed-by the reject-sink edge), inserts a
    // delay vertex for non-overlapped forms, and builds the body. Used both for
    // nested AstImplication in pexpr position and for the top-level assertion
    // antecedent -- `errorNodep` anchors the "unsupported sequence antecedent"
    // error, which differs between the two call sites.
    BuildResult buildImplicationEdges(Ast_ग्रन्थिः_व्यञ्जकम्* antExprp, Ast_ग्रन्थिः_व्यञ्जकम्* bodyExprp,
                                      SvaStateVertex* entryVtxp, तार्किक isOverlapped,
                                      तार्किक isFollowedBy, Ast_ग्रन्थिः* errorNodep, File_पङ्क्तिः* flp) {
        ध्रुव BuildResult antResult = build_व्यञ्जकम्(antExprp, entryVtxp);
        सचे (!antResult.valid()) पटिनिवत्त antResult;

        // Followed-by requires pure-boolean antecedent for non-vacuous-fail at
        // the attempt-start cycle. IEEE 1800-2023 16.12.9 permits a multi-cycle
        // sequence LHS, so this is an implementation gap rather than illegal SV.
        सचे (isFollowedBy && antResult.termVertexp != entryVtxp) {
            errorNodep->v3warn(E_UNSUPPORTED,
                               "Unsupported: sequence expression as antecedent of followed-by"
                               " (#-# / #=#) (IEEE 1800-2023 16.12.9)");
            पटिनिवत्त BuildResult::failWithError();
        }
        UASSERT_OBJ(!isFollowedBy || antResult.finalCondp, errorNodep,
                    "followed-by antecedent terminal at entry must carry finalCondp");

        // Use raw createStateVertex() so trigVtxp starts without liveness --
        // reaching the antecedent terminal is a definitive event.
        SvaStateVertex* ध्रुव trigVtxp = m_graph.createStateVertex();
        सचे (antResult.finalCondp) {
            m_graph.addLink(antResult.termVertexp, trigVtxp,
                            sampled(antResult.finalCondp->clone_वृक्षः_Pure(मिच्छा)));
            // Followed-by non-vacuous fail: rejectOnFail fires when the attempt
            // is live (termVtx reachable) and sampled(antecedent) is false.
            सचे (isFollowedBy) {
                SvaStateVertex* ध्रुव sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = सच्च;
                SvaTransEdge* ध्रुव ep
                    = m_graph.addLink(antResult.termVertexp, sinkVtxp,
                                      sampled(antResult.finalCondp->clone_वृक्षः_Pure(मिच्छा)));
                ep->m_rejectOnFail = सच्च;
            }
            // finalCondp is cloned into the Sampled nodes; if the original is
            // not parented anywhere in the AST anymore it must be freed here
            // or ASan flags it as a leak (e.g. t_sequence_bool_ops).
            सचे (!antResult.finalCondp->backp()) {
                VL_DO_DANGLING(antResult.finalCondp->delete_वृक्षः(), antResult.finalCondp);
            }
        } अञ्ञथा {
            m_graph.addLink(antResult.termVertexp, trigVtxp);
        }
        resetScope();

        SvaStateVertex* bodyEntryp = trigVtxp;
        सचे (!isOverlapped) {
            SvaStateVertex* ध्रुव delayVtxp = m_graph.createStateVertex();
            m_graph.addClockedEdge(trigVtxp, delayVtxp);
            bodyEntryp = delayVtxp;
        }
        // An empty antecedent match must not start the consequent, but a zero-minimum
        // repetition's empty match is not handled here; keep the merge vertex for those
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = !isFollowedBy && !hasZeroMinRep(antExprp);
        पटिनिवत्त build_व्यञ्जकम्(bodyExprp, bodyEntryp, /*isTopLevelStep=*/सच्च);
    }

    BuildResult build(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        m_graph.m_startVertexp = scopedCreateVertex();
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = सच्च;
        पटिनिवत्त build_व्यञ्जकम्(exprp, m_graph.m_startVertexp, /*isTopLevelStep=*/सच्च);
    }
};

//######################################################################
// NFA Lowering (converts NFA graph to synthesizable AstAlways blocks)

वग्ग SvaNfaLowering final {
    Ast_ग्रन्थिः_Module* ध्रुव m_modp;  // Module to add state vars and always blocks to
    Ast_ग्रन्थिः_DType* ध्रुव m_u32DTypep;  // Shared unsigned counter dtype
    V3UniqueNames m_names{"__Vnfa"};
    size_t m_statDelayRingEdgeVisits = 0;  // Delay-ring incoming edges visited

    // Per-lowering shared context (passed to phase sub-functions)
    // Per-vertex lowering state is stored in SvaVertexData and accessed via
    // V3GraphVertex::userp() (see SvaStateVertex::datap()).
    संरचना LowerCtx final {
        File_पङ्क्तिः* flp;  // Source location for generated AST
        Ast_Sen_वृक्षः* senTreep;  // Clock sensitivity tree
        Ast_ग्रन्थिः_व्यञ्जकम्* disableExprp;  // disable iff expression (may be nullptr)
        Ast_ग्रन्थिः_व्यञ्जकम्* matchCondp;  // Final boolean match condition (may be nullptr)
        VAssertType assertType;  // Assertion type for control tasks
        VAssertDirectiveType directiveType;  // Directive type for control tasks
        AstVar* killVarp;  // Last observed kill generation
        SvaGraph& graph;  // NFA graph
    };

    // Build a match-now expression: stateSig[i] && $sampled(condp)
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* buildMatchNow(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* stateExprp, Ast_ग्रन्थिः_व्यञ्जकम्* condp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव statep = stateExprp->clone_वृक्षः_Pure(मिच्छा);
        सचे (!condp) पटिनिवत्त statep;
        पटिनिवत्त नूतन AstLogAnd{flp, statep, sampled(condp->clone_वृक्षः_Pure(मिच्छा))};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* andCond(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* condp) {
        सचे (!condp) पटिनिवत्त exprp;
        पटिनिवत्त नूतन AstLogAnd{flp, exprp, condp->clone_वृक्षः_Pure(मिच्छा)};
    }
    // bp is always non-null; only ap can be null (serving as accumulator).
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* orExprs(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* ap, Ast_ग्रन्थिः_व्यञ्जकम्* bp) {
        सचे (!ap) पटिनिवत्त bp;
        पटिनिवत्त नूतन AstLogOr{flp, ap, bp};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* addCountp(File_पङ्क्तिः* ध्रुव flp, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव totalCountp,
                                  Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव contributionp,
                                  Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enablep = nullptr) {
        // contribution = enable ? contribution : 0;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enabledContributionp
            = enablep ? नूतन AstCond{flp, enablep, contributionp,
                                    newTypedConstp(flp, contributionp->dtypep(), 0)}
                      : contributionp;
        सचे (!totalCountp) पटिनिवत्त enabledContributionp;
        पटिनिवत्त नूतन AstAdd{flp, totalCountp, enabledContributionp};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* gateCheckedCountp(File_पङ्क्तिः* ध्रुव flp, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव countp,
                                          Ast_ग्रन्थिः_व्यञ्जकम्*& overflowp, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enablep) {
        सचे (overflowp) {
            overflowp = नूतन AstLogAnd{flp, enablep->clone_वृक्षः_Pure(मिच्छा), overflowp};
        }
        पटिनिवत्त addCountp(flp, nullptr, countp, enablep);
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* addCheckedCountp(File_पङ्क्तिः* ध्रुव flp, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव totalCountp,
                                         Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव contributionp, Ast_ग्रन्थिः_व्यञ्जकम्*& overflowp,
                                         Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव contributionOverflowp = nullptr) {
        सचे (contributionOverflowp) { overflowp = orExprs(flp, overflowp, contributionOverflowp); }
        सचे (!totalCountp) पटिनिवत्त contributionp;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव oldTotalp = totalCountp->clone_वृक्षः_Pure(मिच्छा);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव resultp = नूतन AstAdd{flp, totalCountp, contributionp};
        overflowp
            = orExprs(flp, overflowp, नूतन AstLt{flp, resultp->clone_वृक्षः_Pure(मिच्छा), oldTotalp});
        पटिनिवत्त resultp;
    }
    AstIf* new_गणनम्_Overflow_Warning(File_पङ्क्तिः* ध्रुव flp, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp) ध्रुव {
        AstDisplay* ध्रुव dispp = नूतन AstDisplay{
            flp, VDisplayType::DT_WARNING,
            "Cover sequence match count overflowed 32-bit storage; results may be incorrect.",
            nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstIf* ध्रुव ifp = नूतन AstIf{flp, condp, dispp};
        ifp->branchPred(VBranchPred::BP_UNLIKELY);
        पटिनिवत्त ifp;
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* killActive(LowerCtx& c) {
        पटिनिवत्त नूतन AstNeq{c.flp, नूतन AstVarRef{c.flp, c.killVarp, VAccess::READ},
                          assertKillGet(c.flp, c.assertType, c.directiveType)};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* notKillActive(LowerCtx& c) { पटिनिवत्त नूतन AstLogNot{c.flp, killActive(c)}; }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* gateNotKill(LowerCtx& c, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        सचे (!exprp) पटिनिवत्त nullptr;
        पटिनिवत्त नूतन AstLogAnd{c.flp, exprp, notKillActive(c)};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* nextRingIndex(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* idxExprp, uint32_t size) {
        ध्रुव auto u32Const = [flp](uint32_t मानम्) {
            पटिनिवत्त नूतन AstConst{flp, AstConst::Widthed_मानम्{}, 32, मानम्};
        };
        UASSERT_OBJ(size > 0, idxExprp, "Ring size must be positive");
        सचे (size == 1) {
            VL_DO_DANGLING(idxExprp->delete_वृक्षः(), idxExprp);
            पटिनिवत्त u32Const(0);
        }
        // idx == size - 1 ? 0 : idx + 1
        AstAdd* ध्रुव addp = नूतन AstAdd{flp, idxExprp->clone_वृक्षः_Pure(मिच्छा), u32Const(1)};
        addp->dtypeFrom(idxExprp);
        AstCond* ध्रुव condp
            = नूतन AstCond{flp, नूतन AstEq{flp, idxExprp, u32Const(size - 1)}, u32Const(0), addp};
        condp->dtypeFrom(idxExprp);
        पटिनिवत्त condp;
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* delayRingSlot(File_पङ्क्तिः* flp, SvaStateVertex* vtxp, Ast_ग्रन्थिः_व्यञ्जकम्* idxExprp,
                                      VAccess access = VAccess::READ) {
        // ring[idx]
        AstVar* ध्रुव ringp = vtxp->datap()->delayRingVarp;
        AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, ringp, access};
        सचे (vtxp->datap()->delayRingCounts) पटिनिवत्त नूतन AstArraySel{flp, refp, idxExprp};
        पटिनिवत्त नूतन AstSel{flp, refp, idxExprp, 1};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* delay_Ring_Head_गणनम्(File_पङ्क्तिः* flp, SvaStateVertex* vtxp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव headp
            = नूतन AstVarRef{flp, vtxp->datap()->delayRingHeadVarp, VAccess::READ};
        सचे (vtxp->datap()->delayRingCounts) पटिनिवत्त headp;
        पटिनिवत्त नूतन AstExtend{flp, headp, vtxp->datap()->delay_Ring_Live_गणनम्_Varp->dtypep()->width()};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* delayRingAtLastIndex(File_पङ्क्तिः* ध्रुव flp, AstVar* ध्रुव idxp,
                                             ध्रुव uint32_t size) {
        पटिनिवत्त नूतन AstEq{flp, नूतन AstVarRef{flp, idxp, VAccess::READ},
                         नूतन AstConst{flp, AstConst::Widthed_मानम्{}, 32, size - 1}};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* delay_Ring_Output_गणनम्(File_पङ्क्तिः* ध्रुव flp, SvaStateVertex* ध्रुव vtxp) {
        AstVar* ध्रुव idxp = vtxp->datap()->delayRingIdxVarp;
        ध्रुव uint32_t size = vtxp->m_delayRingSize;
        Ast_ग्रन्थिः_व्यञ्जकम्* outgoingValidp
            = नूतन AstVarRef{flp, vtxp->datap()->delayRingWrappedVarp, VAccess::READ};
        सचे (!vtxp->m_isFixedDelayRing) {
            outgoingValidp
                = नूतन AstLogOr{flp, outgoingValidp, delayRingAtLastIndex(flp, idxp, size)};
        }
        AstVar* ध्रुव live_गणनम्_Varp = vtxp->datap()->delay_Ring_Live_गणनम्_Varp;
        पटिनिवत्त नूतन AstCond{flp, outgoingValidp, delay_Ring_Head_गणनम्(flp, vtxp),
                           newTypedConstp(flp, live_गणनम्_Varp->dtypep(), 0)};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* delayRingHasLiveThreadsp(File_पङ्क्तिः* ध्रुव flp,
                                                 AstVar* ध्रुव live_गणनम्_Varp) {
        // active = live_count != 0;
        पटिनिवत्त नूतन AstNeq{flp, नूतन AstVarRef{flp, live_गणनम्_Varp, VAccess::READ},
                          newTypedConstp(flp, live_गणनम्_Varp->dtypep(), 0)};
    }

    // Phase 3 output signals
    संरचना SignalSet final {
        Ast_ग्रन्थिः_व्यञ्जकम्* terminalActivep = nullptr;  // OR of all successful terminal matches
        Ast_ग्रन्थिः_व्यञ्जकम्* matchCountp = nullptr;  // NFA paths completing the sequence this tick
        Ast_ग्रन्थिः_व्यञ्जकम्* match_गणनम्_Overflowp = nullptr;  // Match count overflow this tick
        Ast_ग्रन्थिः_व्यञ्जकम्* rejectBasep = nullptr;  // Reject when a terminal match fails
        Ast_ग्रन्थिः_व्यञ्जकम्* requiredStepRejectp = nullptr;  // Per-source reject from rejectOnFail Links
        Ast_ग्रन्थिः_व्यञ्जकम्* throughoutRejectp = nullptr;  // Reject when a throughout guard drops
        Ast_ग्रन्थिः_व्यञ्जकम्* threadFailCountp = nullptr;  // Number of threads rejected on this tick
    };

    // Disable condition also aborts attempts between clock events. A constant disable
    // never rises, and $sampled is unsupported in a sensitivity list.
    स्थिर तार्किक isAsyncDisable(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव disableExprp) {
        पटिनिवत्त disableExprp && !VN_IS(disableExprp, Const)
               && !disableExprp->exists([](ध्रुव AstSampled*) { पटिनिवत्त सच्च; });
    }

    // Emit the clocked update block of registers that hold pending attempts. The clearp
    // statements reset the same registers: a disable rising between clock events also
    // aborts pending attempts (IEEE 1800-2023 16.12), so clearp runs on that rising edge
    // in a separate block. It must clear unconditionally, as the disable may already be
    // false again when it runs, and the update block must not run on that edge, as it
    // would advance the attempts as if the clock had ticked. Blocks that hold no attempt
    // state, such as the $assertkill acknowledge in emitKillAckNba, do not go through here.
    // Callers pass a null clearp when the disable cannot rise between clock events.
    शून्य addStateAlways(LowerCtx& c, Ast_ग्रन्थिः* bodyp, Ast_ग्रन्थिः* clearp) {
        m_modp->addStmtsp(
            नूतन AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_वृक्षः(मिच्छा), bodyp});
        सचे (!clearp) पटिनिवत्त;
        m_modp->addStmtsp(नूतन AstAlways{
            c.flp, VAlwaysKwd::ALWAYS,
            नूतन Ast_Sen_वृक्षः{c.flp, नूतन AstSenItem{c.flp, VEdgeType::ET_POSEDGE,
                                                 c.disableExprp->clone_वृक्षः_Pure(मिच्छा)}},
            clearp});
    }

    // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
    // Phase 2: State register NBA always block. Each clocked-edge target
    // latches the OR of its incoming contributions.
    शून्य emitStateRegisterNba(LowerCtx& c) {
        ध्रुव तार्किक asyncDisable = isAsyncDisable(c.disableExprp);
        Ast_ग्रन्थिः* bodyp = nullptr;
        Ast_ग्रन्थिः* clearp = nullptr;
        अत्थाय (ध्रुव V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ध्रुव SvaStateVertex* ध्रुव vtxp = static_cast<ध्रुव SvaStateVertex*>(&vtxr);
            सचे (!vtxp->datap()->stateVarp) अनुवर्त;

            Ast_ग्रन्थिः_व्यञ्जकम्* nextStatep = nullptr;
            अत्थाय (ध्रुव V3GraphEdge& edger : vtxp->inEdges()) {
                ध्रुव SvaTransEdge& tedger = static_cast<ध्रुव SvaTransEdge&>(edger);
                सचे (!tedger.m_consumesCycle) अनुवर्त;
                ध्रुव SvaStateVertex* ध्रुव fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Clocked-edge source missing stateSig");

                Ast_ग्रन्थिः_व्यञ्जकम्* srcSigp = fromVtxp->datap()->stateSigp->clone_वृक्षः_Pure(मिच्छा);
                srcSigp = andCond(c.flp, srcSigp, tedger.m_condp);

                सचे (c.disableExprp) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notDisp
                        = नूतन AstLogNot{c.flp, c.disableExprp->clone_वृक्षः_Pure(मिच्छा)};
                    srcSigp = नूतन AstLogAnd{c.flp, srcSigp, notDisp};
                }
                nextStatep = orExprs(c.flp, nextStatep, srcSigp);
            }

            UASSERT_OBJ(nextStatep, vtxp,
                        "Registered vertex has no clocked incoming contribution");
            nextStatep = gateNotKill(c, nextStatep);

            AstAssignDly* ध्रुव assignp = नूतन AstAssignDly{
                c.flp, नूतन AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE}, nextStatep};
            bodyp = Ast_ग्रन्थिः::addNextNull(bodyp, assignp);
            सचे (asyncDisable) {
                clearp = Ast_ग्रन्थिः::addNextNull(
                    clearp,
                    नूतन AstAssignDly{
                        c.flp, नूतन AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE},
                        नूतन AstConst{c.flp, AstConst::BitFalse{}}});
            }
        }
        सचे (bodyp) addStateAlways(c, bodyp, clearp);
    }

    // Phase 2b: In-place ring storage and registered ring metadata.
    शून्य emitDelayRingUpdates(LowerCtx& c, ध्रुव तार्किक गणनम्_Matches) {
        ध्रुव तार्किक asyncDisable = isAsyncDisable(c.disableExprp);
        अत्थाय (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ध्रुव vtxp = static_cast<SvaStateVertex*>(&vtxr);
            सचे (!vtxp->datap()->delayRingVarp) अनुवर्त;
            AstVar* ध्रुव idxp = vtxp->datap()->delayRingIdxVarp;
            AstVar* ध्रुव live_गणनम्_Varp = vtxp->datap()->delay_Ring_Live_गणनम्_Varp;
            AstVar* ध्रुव wrappedp = vtxp->datap()->delayRingWrappedVarp;
            ध्रुव uint32_t size = static_cast<uint32_t>(vtxp->m_delayRingSize);

            Ast_ग्रन्थिः_व्यञ्जकम्* incomingp = nullptr;
            Ast_ग्रन्थिः_व्यञ्जकम्* incomingOverflowp = nullptr;
            अत्थाय (ध्रुव V3GraphEdge& edger : vtxp->inEdges()) {
                ++m_statDelayRingEdgeVisits;
                ध्रुव SvaTransEdge& tedger = static_cast<ध्रुव SvaTransEdge&>(edger);
                UASSERT_OBJ(tedger.m_consumesCycle == vtxp->m_isFixedDelayRing, vtxp,
                            "Delay-ring incoming edge kind mismatch");
                ध्रुव SvaStateVertex* ध्रुव fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Delay-ring incoming source missing stateSig");
                UASSERT_OBJ(!गणनम्_Matches || fromVtxp->datap()->state_गणनम्_Sigp, fromVtxp,
                            "Delay-ring incoming source missing stateCountSig");
                // IEEE 1800-2023 16.14.1/16.14.3: assertion alternatives belong to one
                // evaluation; only cover sequence counts each end-of-match.
                Ast_ग्रन्थिः_व्यञ्जकम्* contribp
                    = गणनम्_Matches
                          ? fromVtxp->datap()->state_गणनम्_Sigp->clone_वृक्षः_Pure(मिच्छा)
                          : नूतन AstExtend{c.flp,
                                          fromVtxp->datap()->stateSigp->clone_वृक्षः_Pure(मिच्छा),
                                          m_u32DTypep->width()};
                Ast_ग्रन्थिः_व्यञ्जकम्* contribOverflowp
                    = गणनम्_Matches && fromVtxp->datap()->state_गणनम्_Overflow_Sigp
                          ? fromVtxp->datap()->state_गणनम्_Overflow_Sigp->clone_वृक्षः_Pure(मिच्छा)
                          : nullptr;
                सचे (tedger.m_condp) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enablep = tedger.m_condp->clone_वृक्षः_Pure(मिच्छा);
                    contribp = गणनम्_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, enablep)
                                   : addCountp(c.flp, nullptr, contribp, enablep);
                }
                सचे (c.disableExprp) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notDisp
                        = नूतन AstLogNot{c.flp, c.disableExprp->clone_वृक्षः_Pure(मिच्छा)};
                    contribp = गणनम्_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, notDisp)
                                   : addCountp(c.flp, nullptr, contribp, notDisp);
                }
                incomingp = गणनम्_Matches ? addCheckedCountp(c.flp, incomingp, contribp,
                                                            incomingOverflowp, contribOverflowp)
                                         : addCountp(c.flp, incomingp, contribp);
            }
            UASSERT_OBJ(incomingp, vtxp, "Delay ring has no incoming edge");
            ध्रुव तार्किक storesCounts = vtxp->datap()->delayRingCounts;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव incomingCountp
                = storesCounts
                      ? incomingp->clone_वृक्षः_Pure(मिच्छा)
                      : नूतन AstExtend{c.flp,
                                      नूतन AstNeq{c.flp, incomingp->clone_वृक्षः_Pure(मिच्छा),
                                                 newTypedConstp(c.flp, incomingp->dtypep(), 0)},
                                      m_u32DTypep->width()};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव storedIncomingp
                = storesCounts ? incomingp
                               : नूतन AstNeq{c.flp, incomingp,
                                            newTypedConstp(c.flp, incomingp->dtypep(), 0)};
            // Only this block accesses the storage. Other processes read the cached head, so
            // the ring can be updated in place without an NBA shadow copy.
            AstAssign* ध्रुव writeIncomingp = नूतन AstAssign{
                c.flp,
                delayRingSlot(c.flp, vtxp, नूतन AstVarRef{c.flp, idxp, VAccess::READ},
                              VAccess::WRITE),
                storedIncomingp};
            Ast_ग्रन्थिः* updateBodyp = writeIncomingp;
            // live_count <= live_count - outgoing_count + incoming_count;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव outgoingp = delay_Ring_Output_गणनम्(c.flp, vtxp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव afterOutgoingp
                = नूतन AstSub{c.flp, नूतन AstVarRef{c.flp, live_गणनम्_Varp, VAccess::READ}, outgoingp};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव oldAfterOutgoingp = afterOutgoingp->clone_वृक्षः_Pure(मिच्छा);
            AstAdd* ध्रुव nextLiveCountp = नूतन AstAdd{c.flp, afterOutgoingp, incomingCountp};
            सचे (storesCounts) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव liveOverflowp
                    = नूतन AstLt{c.flp, nextLiveCountp->clone_वृक्षः_Pure(मिच्छा), oldAfterOutgoingp};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव overflowp = orExprs(c.flp, incomingOverflowp, liveOverflowp);
                updateBodyp = new_गणनम्_Overflow_Warning(c.flp, overflowp);
                updateBodyp->addNext(writeIncomingp);
            } अञ्ञथा {
                VL_DO_DANGLING(oldAfterOutgoingp->delete_वृक्षः(), oldAfterOutgoingp);
            }
            // Cache the outgoing slot for the next index after the write. Reading after
            // writing also handles one-slot rings and two-slot range rings without alias cases.
            Ast_ग्रन्थिः_व्यञ्जकम्* nextHeadIdxp
                = nextRingIndex(c.flp, नूतन AstVarRef{c.flp, idxp, VAccess::READ}, size);
            सचे (!vtxp->m_isFixedDelayRing) {
                nextHeadIdxp = nextRingIndex(c.flp, nextHeadIdxp, size);
            }
            updateBodyp->addNext(नूतन AstAssignDly{
                c.flp, नूतन AstVarRef{c.flp, vtxp->datap()->delayRingHeadVarp, VAccess::WRITE},
                delayRingSlot(c.flp, vtxp, nextHeadIdxp)});
            updateBodyp->addNext(नूतन AstAssignDly{
                c.flp, नूतन AstVarRef{c.flp, live_गणनम्_Varp, VAccess::WRITE}, nextLiveCountp});
            // wrapped <= wrapped || idx == size - 1;
            updateBodyp->addNext(
                नूतन AstAssignDly{c.flp, नूतन AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                 नूतन AstLogOr{c.flp, नूतन AstVarRef{c.flp, wrappedp, VAccess::READ},
                                              delayRingAtLastIndex(c.flp, idxp, size)}});
            // idx <= next_idx;
            updateBodyp->addNext(नूतन AstAssignDly{
                c.flp, नूतन AstVarRef{c.flp, idxp, VAccess::WRITE},
                nextRingIndex(c.flp, नूतन AstVarRef{c.flp, idxp, VAccess::READ}, size)});
            सचे (vtxp->m_delayRingAdvanceCondp) {
                updateBodyp = नूतन AstIf{
                    c.flp, sampled(vtxp->m_delayRingAdvanceCondp->clone_वृक्षः_Pure(मिच्छा)),
                    updateBodyp};
            }

            Ast_ग्रन्थिः_व्यञ्जकम्* clearCondp = killActive(c);
            सचे (vtxp->m_delayRingClearCondp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_delayRingClearCondp->clone_वृक्षः_Pure(मिच्छा)));
            }
            सचे (vtxp->m_abortClearp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_abortClearp->clone_वृक्षः_Pure(मिच्छा)));
            }
            सचे (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_वृक्षः_Pure(मिच्छा));
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* guardp = nullptr;
            अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cp : vtxp->m_throughoutConds) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sampledp = sampled(cp->clone_वृक्षः_Pure(मिच्छा));
                guardp = guardp ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstLogAnd{c.flp, guardp, sampledp})
                                : sampledp;
            }
            सचे (guardp) clearCondp = orExprs(c.flp, clearCondp, नूतन AstLogNot{c.flp, guardp});
            // Logically clear the ring without touching its wide storage.
            AstAssignDly* ध्रुव clearCountp
                = नूतन AstAssignDly{c.flp, नूतन AstVarRef{c.flp, live_गणनम्_Varp, VAccess::WRITE},
                                   newTypedConstp(c.flp, live_गणनम्_Varp->dtypep(), 0)};
            clearCountp->addNext(नूतन AstAssignDly{c.flp,
                                                  नूतन AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                                  नूतन AstConst{c.flp, AstConst::BitFalse{}}});
            clearCountp->addNext(नूतन AstAssignDly{c.flp,
                                                  नूतन AstVarRef{c.flp, idxp, VAccess::WRITE},
                                                  newTypedConstp(c.flp, idxp->dtypep(), 0)});
            updateBodyp = नूतन AstIf{c.flp, clearCondp, clearCountp, updateBodyp};

            addStateAlways(c, updateBodyp, asyncDisable ? clearCountp->clone_वृक्षः(सच्च) : nullptr);
        }
    }

    // Phase 2c: SAnd combiner done-latch always block.
    // NBA semantics ensure doneL/doneR read pre-update values (IEEE 16.9.5).
    शून्य emitAndCombinerDoneLatchNba(LowerCtx& c) {
        ध्रुव तार्किक asyncDisable = isAsyncDisable(c.disableExprp);
        अत्थाय (ध्रुव V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ध्रुव SvaStateVertex* ध्रुव avp = static_cast<ध्रुव SvaStateVertex*>(&vtxr);
            सचे (!avp->datap()->doneLVarp) अनुवर्त;
            // doneLVars is non-null only for AndCombiner vertices, which always
            // have both m_andLhsTermp and m_andRhsTermp set at build time.
            UASSERT_OBJ(avp->m_andLhsTermp && avp->m_andRhsTermp, avp,
                        "AndCombiner vertex missing LHS/RHS terminal");
            ध्रुव SvaStateVertex* ध्रुव lhsTermp = avp->m_andLhsTermp;
            ध्रुव SvaStateVertex* ध्रुव rhsTermp = avp->m_andRhsTermp;
            // resolveLinks' 2*N+2 fixed-point pass is guaranteed to populate
            // stateSigp on every AndCombiner and its LHS/RHS terminals.
            UASSERT_OBJ(lhsTermp->datap()->stateSigp && rhsTermp->datap()->stateSigp
                            && avp->datap()->stateSigp,
                        avp, "AndCombiner stateSigp chain unresolved after resolveLinks");

            AstAssignDly* ध्रुव clearLp = नूतन AstAssignDly{
                c.flp, नूतन AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                नूतन AstConst{c.flp, AstConst::BitFalse{}}};
            AstAssignDly* ध्रुव clearRp = नूतन AstAssignDly{
                c.flp, नूतन AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                नूतन AstConst{c.flp, AstConst::BitFalse{}}};
            clearLp->addNext(clearRp);

            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव matchLNowp
                = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, avp->m_andLhsCondp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव matchRNowp
                = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, avp->m_andRhsCondp);
            Ast_ग्रन्थिः_व्यञ्जकम्* gateLp = matchLNowp;
            Ast_ग्रन्थिः_व्यञ्जकम्* gateRp = matchRNowp;
            सचे (c.disableExprp) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notDisLp
                    = नूतन AstLogNot{c.flp, c.disableExprp->clone_वृक्षः_Pure(मिच्छा)};
                gateLp = नूतन AstLogAnd{c.flp, gateLp, notDisLp};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notDisRp
                    = नूतन AstLogNot{c.flp, c.disableExprp->clone_वृक्षः_Pure(मिच्छा)};
                gateRp = नूतन AstLogAnd{c.flp, gateRp, notDisRp};
            }
            AstAssignDly* ध्रुव setLp = नूतन AstAssignDly{
                c.flp, नूतन AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                नूतन AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* ध्रुव setLIfp = नूतन AstIf{c.flp, gateLp, setLp, nullptr};
            AstAssignDly* ध्रुव setRp = नूतन AstAssignDly{
                c.flp, नूतन AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                नूतन AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* ध्रुव setRIfp = नूतन AstIf{c.flp, gateRp, setRp, nullptr};
            setLIfp->addNext(setRIfp);

            Ast_ग्रन्थिः_व्यञ्जकम्* clearCondp = नूतन AstLogOr{c.flp, killActive(c),
                                                   avp->datap()->stateSigp->clone_वृक्षः_Pure(मिच्छा)};
            सचे (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_वृक्षः_Pure(मिच्छा));
            }
            AstIf* ध्रुव topp = नूतन AstIf{c.flp, clearCondp, clearLp, setLIfp};
            addStateAlways(c, topp, asyncDisable ? clearLp->clone_वृक्षः(सच्च) : nullptr);
        }
    }

    शून्य emitKillAckNba(LowerCtx& c) {
        AstAssignDly* ध्रुव ackp
            = नूतन AstAssignDly{c.flp, नूतन AstVarRef{c.flp, c.killVarp, VAccess::WRITE},
                               assertKillGet(c.flp, c.assertType, c.directiveType)};
        m_modp->addStmtsp(
            नूतन AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_वृक्षः(मिच्छा), ackp});
    }

    // Phase 3/3a/3b: Compute terminal match/reject signals, required-step reject,
    // throughout-drop reject; clean up intermediate state signals.
    // Phase 3: terminalActive and rejectBase from Links to matchVertex.
    // Builder only adds Links (non-clocked) to matchVertex via addLink in
    // wireMatchAndMidSources. For cover sequence, also count each end-of-match
    // so the action can be replayed without unrolling endpoints.
    शून्य computeTerminalMatchAndReject(LowerCtx& c, SignalSet& sigs, ध्रुव तार्किक need_Match_गणनम्) {
        अत्थाय (ध्रुव SvaTransEdge* ध्रुव tedgep : c.graph.allEdges()) {
            सचे (tedgep->toVtxp() != c.graph.m_matchVertexp) अनुवर्त;
            SvaStateVertex* ध्रुव fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                        "Terminal-link source missing stateSig");

            Ast_ग्रन्थिः_व्यञ्जकम्* srcSigp = fromVtxp->datap()->stateSigp->clone_वृक्षः_Pure(मिच्छा);
            srcSigp = andCond(c.flp, srcSigp, tedgep->m_condp);
            सचे (need_Match_गणनम्) {
                UASSERT_OBJ(fromVtxp->datap()->state_गणनम्_Sigp, fromVtxp,
                            "Terminal-link source missing stateCountSig");
                Ast_ग्रन्थिः_व्यञ्जकम्* contributionp
                    = fromVtxp->datap()->state_गणनम्_Sigp->clone_वृक्षः_Pure(मिच्छा);
                Ast_ग्रन्थिः_व्यञ्जकम्* contributionOverflowp
                    = fromVtxp->datap()->state_गणनम्_Overflow_Sigp
                          ? fromVtxp->datap()->state_गणनम्_Overflow_Sigp->clone_वृक्षः_Pure(मिच्छा)
                          : nullptr;
                सचे (tedgep->m_condp) {
                    contributionp = gateCheckedCountp(c.flp, contributionp, contributionOverflowp,
                                                      tedgep->m_condp->clone_वृक्षः_Pure(मिच्छा));
                }
                sigs.matchCountp
                    = addCheckedCountp(c.flp, sigs.matchCountp, contributionp,
                                       sigs.match_गणनम्_Overflowp, contributionOverflowp);
            }

            सचे (fromVtxp->m_delayRingSize && !fromVtxp->m_isFixedDelayRing) {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_वृक्षः_Pure(मिच्छा));
                // reject |= (wrapped || idx == size - 1) && head && final_condition;
                Ast_ग्रन्थिः_व्यञ्जकम्* expireContribp = delay_Ring_Output_गणनम्(c.flp, fromVtxp);
                expireContribp = andCond(c.flp, expireContribp, tedgep->m_condp);
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, expireContribp);
                VL_DO_DANGLING(srcSigp->delete_वृक्षः(), srcSigp);
            } अञ्ञथा सचे (fromVtxp->m_isUnbounded || fromVtxp->m_isAndCombiner) {
                sigs.terminalActivep = orExprs(c.flp, sigs.terminalActivep, srcSigp);
            } अञ्ञथा {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_वृक्षः_Pure(मिच्छा));
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, srcSigp);
            }
        }
        // wireMatchAndMidSources always adds a Link from result.termVertexp
        // to m_matchVertexp, so the loop above always sets terminalActivep.
        UASSERT_OBJ(sigs.terminalActivep, c.graph.m_matchVertexp,
                    "No terminal edge to match vertex");
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* newThroughoutThreadFailCountp(LowerCtx& c, AstVar* ध्रुव delay_Ring_Live_गणनम्_Varp,
                                               Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव stateExprp,
                                               Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enablep) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव activeThreadCountp
            = delay_Ring_Live_गणनम्_Varp
                  ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                        नूतन AstVarRef{c.flp, delay_Ring_Live_गणनम्_Varp, VAccess::READ})
                  : नूतन AstExtend{c.flp, stateExprp->clone_वृक्षः_Pure(मिच्छा), m_u32DTypep->width()};
        पटिनिवत्त addCountp(c.flp, nullptr, activeThreadCountp, enablep);
    }

    // Phase 3b: Throughout-drop and ring-wide abort rejection.
    शून्य computeThroughoutReject(LowerCtx& c, SignalSet& sigs, ध्रुव तार्किक need_Thread_Fail_गणनम्) {
        अत्थाय (ध्रुव V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ध्रुव SvaStateVertex* ध्रुव vtxp = static_cast<ध्रुव SvaStateVertex*>(&vtxr);
            ध्रुव auto& conds = vtxp->m_throughoutConds;
            सचे (conds.empty() && !vtxp->m_abortRejectp) अनुवर्त;
            सचे (vtxp->m_isAndCombiner) अनुवर्त;
            Ast_ग्रन्थिः_व्यञ्जकम्* stateExprp = nullptr;
            सचे (vtxp->datap()->stateVarp) {
                stateExprp = नूतन AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
            } अञ्ञथा सचे (vtxp->datap()->delayRingVarp && vtxp->m_isFixedDelayRing) {
                stateExprp
                    = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_गणनम्_Varp);
            } अञ्ञथा {
                UASSERT_OBJ(vtxp->datap()->stateSigp, vtxp,
                            "Throughout-conds vertex missing state representation");
                stateExprp = vtxp->datap()->stateSigp->clone_वृक्षः_Pure(मिच्छा);
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* guardp = nullptr;
            अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cp : conds) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sp = sampled(cp->clone_वृक्षः_Pure(मिच्छा));
                guardp = guardp ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstLogAnd{c.flp, guardp, sp}) : sp;
            }
            सचे (vtxp->m_abortRejectp) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notAbortp
                    = नूतन AstLogNot{c.flp, sampled(vtxp->m_abortRejectp->clone_वृक्षः_Pure(मिच्छा))};
                guardp = guardp
                             ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstLogAnd{c.flp, guardp, notAbortp})
                             : notAbortp;
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* rejectCondp = नूतन AstLogNot{c.flp, guardp};
            सचे (vtxp->m_abortClearp) {
                // Any abort clears the ring. Accept aborts suppress a simultaneous guard failure;
                // reject aborts are restored below so they still force rejection.
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notAbortClearp
                    = नूतन AstLogNot{c.flp, sampled(vtxp->m_abortClearp->clone_वृक्षः_Pure(मिच्छा))};
                rejectCondp = नूतन AstLogAnd{c.flp, rejectCondp, notAbortClearp};
                सचे (vtxp->m_abortRejectp) {
                    rejectCondp = नूतन AstLogOr{
                        c.flp, rejectCondp, sampled(vtxp->m_abortRejectp->clone_वृक्षः_Pure(मिच्छा))};
                }
            }
            सचे (need_Thread_Fail_गणनम्) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव contributionp
                    = newThroughoutThreadFailCountp(c, vtxp->datap()->delay_Ring_Live_गणनम्_Varp,
                                                    stateExprp, rejectCondp->clone_वृक्षः_Pure(मिच्छा));
                sigs.threadFailCountp = addCountp(c.flp, sigs.threadFailCountp, contributionp);
                सचे (vtxp->m_abortRejectp && vtxp->m_delayRingAdvanceCondp) {
                    // An advancing ring thread also occupies its same-tick match vertex in the
                    // unrolled NFA, so abort rejection must replay both fail actions.
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव abortAndAdvancep = नूतन AstLogAnd{
                        c.flp, sampled(vtxp->m_abortRejectp->clone_वृक्षः_Pure(मिच्छा)),
                        sampled(vtxp->m_delayRingAdvanceCondp->clone_वृक्षः_Pure(मिच्छा))};
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव matchContributionp = newThroughoutThreadFailCountp(
                        c, vtxp->datap()->delay_Ring_Live_गणनम्_Varp, stateExprp, abortAndAdvancep);
                    sigs.threadFailCountp
                        = addCountp(c.flp, sigs.threadFailCountp, matchContributionp);
                }
            }
            sigs.throughoutRejectp = orExprs(c.flp, sigs.throughoutRejectp,
                                             नूतन AstLogAnd{c.flp, stateExprp, rejectCondp});
        }
    }

    SignalSet computeSignals(LowerCtx& c, ध्रुव तार्किक need_Thread_Fail_गणनम्,
                             ध्रुव तार्किक need_Throughout_Thread_Fail_गणनम्, ध्रुव तार्किक need_Match_गणनम्) {
        SignalSet sigs;

        computeTerminalMatchAndReject(c, sigs, need_Match_गणनम्);

        // Phase 3a: required-step rejection.
        // Builder only sets m_rejectOnFail on non-clocked Links with m_condp
        // or m_condVtxp, and the source always has a resolved stateSig.
        अत्थाय (ध्रुव SvaTransEdge* ध्रुव tedgep : c.graph.allEdges()) {
            सचे (!tedgep->m_rejectOnFail) अनुवर्त;
            ध्रुव SvaStateVertex* ध्रुव fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp && (tedgep->m_condp || tedgep->m_condVtxp),
                        fromVtxp,
                        "rejectOnFail Link must have condp/condVtxp and source stateSig");
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव srcSigp = fromVtxp->datap()->stateSigp->clone_वृक्षः_Pure(मिच्छा);
            Ast_ग्रन्थिः_व्यञ्जकम्* condp = nullptr;
            सचे (tedgep->m_condVtxp) {
                UASSERT_OBJ(tedgep->m_condVtxp->datap()->stateSigp, tedgep->m_condVtxp,
                            "rejectOnFail condVtxp missing stateSig");
                condp = tedgep->m_condVtxp->datap()->stateSigp->clone_वृक्षः_Pure(मिच्छा);
                सचे (tedgep->m_condp) {
                    condp = नूतन AstLogOr{c.flp, condp, tedgep->m_condp->clone_वृक्षः_Pure(मिच्छा)};
                }
            } अञ्ञथा {
                condp = tedgep->m_condp->clone_वृक्षः_Pure(मिच्छा);
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notCondp = नूतन AstLogNot{c.flp, condp};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rawFailp = नूतन AstLogAnd{c.flp, srcSigp, notCondp};
            सचे (need_Thread_Fail_गणनम्) {
                // thread_fail_count += fail;
                sigs.threadFailCountp = addCountp(
                    c.flp, sigs.threadFailCountp,
                    नूतन AstExtend{c.flp, rawFailp->clone_वृक्षः_Pure(मिच्छा), m_u32DTypep->width()});
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव failp = gateNotKill(c, rawFailp);
            sigs.requiredStepRejectp = orExprs(c.flp, sigs.requiredStepRejectp, failp);
        }

        computeThroughoutReject(c, sigs, need_Throughout_Thread_Fail_गणनम्);
        सचे (sigs.threadFailCountp) {
            sigs.threadFailCountp
                = addCountp(c.flp, nullptr, sigs.threadFailCountp, notKillActive(c));
        }
        सचे (sigs.matchCountp) {
            सचे (c.matchCondp) {
                sigs.matchCountp
                    = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_गणनम्_Overflowp,
                                        sampled(c.matchCondp->clone_वृक्षः_Pure(मिच्छा)));
            }
            sigs.matchCountp = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_गणनम्_Overflowp,
                                                 notKillActive(c));
        }
        sigs.terminalActivep = gateNotKill(c, sigs.terminalActivep);
        sigs.rejectBasep = gateNotKill(c, sigs.rejectBasep);
        sigs.throughoutRejectp = gateNotKill(c, sigs.throughoutRejectp);

        // Clean up intermediate state signals. These are orphan subtrees
        // (never linked into the enclosing AST); deleteTree() is immediate
        // which is what we want since stateSigp lifetime ends with this scope.
        अत्थाय (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ध्रुव vtxp = static_cast<SvaStateVertex*>(&vtxr);
            Ast_ग्रन्थिः_व्यञ्जकम्*& sigp = vtxp->datap()->stateSigp;
            सचे (sigp) VL_DO_DANGLING(sigp->delete_वृक्षः(), sigp);
            Ast_ग्रन्थिः_व्यञ्जकम्*& countp = vtxp->datap()->state_गणनम्_Sigp;
            सचे (countp) VL_DO_DANGLING(countp->delete_वृक्षः(), countp);
            Ast_ग्रन्थिः_व्यञ्जकम्*& overflowp = vtxp->datap()->state_गणनम्_Overflow_Sigp;
            सचे (overflowp) VL_DO_DANGLING(overflowp->delete_वृक्षः(), overflowp);
        }
        // Disable iff gating (IEEE 1800-2023 16.12) on the current level value.
        सचे (c.disableExprp) {
            // terminalActivep is always set, so gate it unconditionally.
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notTermp
                = नूतन AstLogNot{c.flp, c.disableExprp->clone_वृक्षः_Pure(मिच्छा)};
            sigs.terminalActivep = नूतन AstLogAnd{c.flp, sigs.terminalActivep, notTermp};
            सचे (sigs.rejectBasep) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notDisp
                    = नूतन AstLogNot{c.flp, c.disableExprp->clone_वृक्षः_Pure(मिच्छा)};
                sigs.rejectBasep = नूतन AstLogAnd{c.flp, sigs.rejectBasep, notDisp};
            }
            सचे (sigs.throughoutRejectp) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notDisp
                    = नूतन AstLogNot{c.flp, c.disableExprp->clone_वृक्षः_Pure(मिच्छा)};
                sigs.throughoutRejectp = नूतन AstLogAnd{c.flp, sigs.throughoutRejectp, notDisp};
            }
            सचे (sigs.requiredStepRejectp) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notDisp
                    = नूतन AstLogNot{c.flp, c.disableExprp->clone_वृक्षः_Pure(मिच्छा)};
                sigs.requiredStepRejectp = नूतन AstLogAnd{c.flp, sigs.requiredStepRejectp, notDisp};
            }
            सचे (sigs.matchCountp) {
                sigs.matchCountp = gateCheckedCountp(
                    c.flp, sigs.matchCountp, sigs.match_गणनम्_Overflowp,
                    नूतन AstLogNot{c.flp, c.disableExprp->clone_वृक्षः_Pure(मिच्छा)});
            }
        }

        सचे (sigs.match_गणनम्_Overflowp) {
            m_modp->addStmtsp(
                नूतन AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_वृक्षः(मिच्छा),
                              new_गणनम्_Overflow_Warning(c.flp, sigs.match_गणनम्_Overflowp)});
            sigs.match_गणनम्_Overflowp = nullptr;
        }

        पटिनिवत्त sigs;
    }

    // Phase 1: Resolve combinational Links via fixed-point propagation.
    शून्य resolveLinks(LowerCtx& c, Ast_ग्रन्थिः_व्यञ्जकम्* triggerExprp, ध्रुव तार्किक गणनम्_Matches) {
        // datap() was freshly allocated in lower() -- all state signals start null.
        c.graph.m_startVertexp->datap()->stateSigp = triggerExprp->clone_वृक्षः_Pure(मिच्छा);
        c.graph.m_startVertexp->datap()->state_गणनम्_Sigp
            = नूतन AstExtend{c.flp, triggerExprp->clone_वृक्षः_Pure(मिच्छा), m_u32DTypep->width()};
        अत्थाय (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ध्रुव vtxp = static_cast<SvaStateVertex*>(&vtxr);
            सचे (vtxp->datap()->stateVarp) {
                vtxp->datap()->stateSigp
                    = नूतन AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
                vtxp->datap()->state_गणनम्_Sigp = नूतन AstExtend{
                    c.flp, नूतन AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ},
                    m_u32DTypep->width()};
            } अञ्ञथा सचे (vtxp->datap()->delayRingVarp) {
                सचे (vtxp->m_isFixedDelayRing) {
                    // state_count = wrapped ? head : 0; state = state_count != 0;
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव countp = delay_Ring_Output_गणनम्(c.flp, vtxp);
                    vtxp->datap()->stateSigp = नूतन AstNeq{c.flp, countp->clone_वृक्षः_Pure(मिच्छा),
                                                          newTypedConstp(c.flp, m_u32DTypep, 0)};
                    vtxp->datap()->state_गणनम्_Sigp = countp;
                } अञ्ञथा {
                    vtxp->datap()->stateSigp
                        = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_गणनम्_Varp);
                    vtxp->datap()->state_गणनम्_Sigp = नूतन AstVarRef{
                        c.flp, vtxp->datap()->delay_Ring_Live_गणनम्_Varp, VAccess::READ};
                }
            }
        }
        // Fixed-point propagation along zero-delay (Link) edges. Rebuild each
        // derived signal from its incoming edges on every pass; appending the same
        // contributions repeatedly makes merge expressions grow exponentially.
        ध्रुव size_t passes = 2 * c.graph.m_graph.vertices().size() + 2;
        अत्थाय (size_t pass = 0; pass < passes; ++pass) {
            // Rebuild SAnd combiners once both sub-NFA terminals are available.
            अत्थाय (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* ध्रुव vtxp = static_cast<SvaStateVertex*>(&vtxr);
                सचे (!vtxp->m_isAndCombiner) अनुवर्त;
                // AndCombiner vertices always have both terminal pointers set.
                UASSERT_OBJ(vtxp->m_andLhsTermp && vtxp->m_andRhsTermp, vtxp,
                            "AndCombiner vertex missing LHS/RHS terminal");
                ध्रुव SvaStateVertex* ध्रुव lhsTermp = vtxp->m_andLhsTermp;
                ध्रुव SvaStateVertex* ध्रुव rhsTermp = vtxp->m_andRhsTermp;
                सचे (!lhsTermp->datap()->stateSigp || !rhsTermp->datap()->stateSigp) अनुवर्त;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव matchLp
                    = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, vtxp->m_andLhsCondp);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव matchRp
                    = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, vtxp->m_andRhsCondp);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव doneLOrp = नूतन AstLogOr{
                    c.flp, नूतन AstVarRef{c.flp, vtxp->datap()->doneLVarp, VAccess::READ}, matchLp};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव doneROrp = नूतन AstLogOr{
                    c.flp, नूतन AstVarRef{c.flp, vtxp->datap()->doneRVarp, VAccess::READ}, matchRp};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bothp = नूतन AstLogAnd{c.flp, doneLOrp, doneROrp};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव oneNowp = नूतन AstLogOr{c.flp, matchLp->clone_वृक्षः_Pure(मिच्छा),
                                                          matchRp->clone_वृक्षः_Pure(मिच्छा)};
                सचे (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_वृक्षः(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = नूतन AstLogAnd{c.flp, bothp, oneNowp};
                सचे (vtxp->datap()->state_गणनम्_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_गणनम्_Sigp->delete_वृक्षः(),
                                   vtxp->datap()->state_गणनम्_Sigp);
                }
                vtxp->datap()->state_गणनम्_Sigp = नूतन AstExtend{
                    c.flp, vtxp->datap()->stateSigp->clone_वृक्षः_Pure(मिच्छा), m_u32DTypep->width()};
            }

            अत्थाय (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* ध्रुव vtxp = static_cast<SvaStateVertex*>(&vtxr);
                सचे (vtxp == c.graph.m_startVertexp || vtxp->datap()->stateVarp
                    || vtxp->datap()->delayRingVarp || vtxp->m_isAndCombiner || vtxp->m_isMatch
                    || vtxp->m_isRejectSink) {
                    अनुवर्त;
                }
                Ast_ग्रन्थिः_व्यञ्जकम्* nextStatep = nullptr;
                Ast_ग्रन्थिः_व्यञ्जकम्* nextStateCountp = nullptr;
                Ast_ग्रन्थिः_व्यञ्जकम्* next_State_गणनम्_Overflowp = nullptr;
                अत्थाय (ध्रुव V3GraphEdge& er : vtxp->inEdges()) {
                    ध्रुव SvaTransEdge& te = static_cast<ध्रुव SvaTransEdge&>(er);
                    ध्रुव SvaStateVertex* ध्रुव fromVtxp = te.fromVtxp();
                    सचे (!fromVtxp->datap()->stateSigp) अनुवर्त;
                    UASSERT_OBJ(fromVtxp->datap()->state_गणनम्_Sigp, fromVtxp,
                                "Link source missing stateCountSig");
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव contributionp = andCond(
                        c.flp, fromVtxp->datap()->stateSigp->clone_वृक्षः_Pure(मिच्छा), te.m_condp);
                    nextStatep = orExprs(c.flp, nextStatep, contributionp);
                    Ast_ग्रन्थिः_व्यञ्जकम्* गणनम्_Contributionp
                        = fromVtxp->datap()->state_गणनम्_Sigp->clone_वृक्षः_Pure(मिच्छा);
                    Ast_ग्रन्थिः_व्यञ्जकम्* गणनम्_Contribution_Overflowp
                        = गणनम्_Matches && fromVtxp->datap()->state_गणनम्_Overflow_Sigp
                              ? fromVtxp->datap()->state_गणनम्_Overflow_Sigp->clone_वृक्षः_Pure(मिच्छा)
                              : nullptr;
                    सचे (te.m_condp) {
                        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enablep = te.m_condp->clone_वृक्षः_Pure(मिच्छा);
                        गणनम्_Contributionp
                            = गणनम्_Matches
                                  ? gateCheckedCountp(c.flp, गणनम्_Contributionp,
                                                      गणनम्_Contribution_Overflowp, enablep)
                                  : addCountp(c.flp, nullptr, गणनम्_Contributionp, enablep);
                    }
                    nextStateCountp
                        = गणनम्_Matches
                              ? addCheckedCountp(c.flp, nextStateCountp, गणनम्_Contributionp,
                                                 next_State_गणनम्_Overflowp,
                                                 गणनम्_Contribution_Overflowp)
                              : addCountp(c.flp, nextStateCountp, गणनम्_Contributionp);
                }
                सचे (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_वृक्षः(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = nextStatep;
                सचे (vtxp->datap()->state_गणनम्_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_गणनम्_Sigp->delete_वृक्षः(),
                                   vtxp->datap()->state_गणनम्_Sigp);
                }
                vtxp->datap()->state_गणनम्_Sigp = nextStateCountp;
                सचे (vtxp->datap()->state_गणनम्_Overflow_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_गणनम्_Overflow_Sigp->delete_वृक्षः(),
                                   vtxp->datap()->state_गणनम्_Overflow_Sigp);
                }
                vtxp->datap()->state_गणनम्_Overflow_Sigp = next_State_गणनम्_Overflowp;
            }
        }
    }

सार्वजनिक:
    // Combine terminal/reject signals into final output expression.
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* assembleResult(Ast_ग्रन्थिः_Cover_Or_Assert* ध्रुव assertp, ध्रुव तार्किक negated,
                                       Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव matchCondp, ध्रुव SignalSet& sigs,
                                       Ast_ग्रन्थिः_व्यञ्जकम्** ध्रुव outMatchpp) {
        File_पङ्क्तिः* ध्रुव flp = assertp->fileline();
        ध्रुव तार्किक isCover = VN_IS(assertp, Cover);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव terminalActivep = sigs.terminalActivep;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rejectBasep = sigs.rejectBasep;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव throughoutRejectp = sigs.throughoutRejectp;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव requiredStepRejectp = sigs.requiredStepRejectp;
        // Property negation (IEEE 1800-2023 16.12.1 `not`): invert match/reject.
        सचे (negated) {
            सचे (isCover) {
                सचे (terminalActivep)
                    VL_DO_DANGLING(terminalActivep->delete_वृक्षः(), terminalActivep);
                Ast_ग्रन्थिः_व्यञ्जकम्* negRejectp = nullptr;
                सचे (matchCondp && rejectBasep) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sampledCondp = sampled(matchCondp->clone_वृक्षः_Pure(मिच्छा));
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notCondp = नूतन AstLogNot{flp, sampledCondp};
                    negRejectp = नूतन AstLogAnd{flp, rejectBasep, notCondp};
                } अञ्ञथा सचे (rejectBasep) {
                    VL_DO_DANGLING(rejectBasep->delete_वृक्षः(), rejectBasep);
                }
                सचे (throughoutRejectp) negRejectp = orExprs(flp, negRejectp, throughoutRejectp);
                सचे (requiredStepRejectp)
                    negRejectp = orExprs(flp, negRejectp, requiredStepRejectp);
                पटिनिवत्त negRejectp ? negRejectp : नूतन AstConst{flp, AstConst::BitFalse{}};
            }
            // Negated assert/assume: output = !match.
            Ast_ग्रन्थिः_व्यञ्जकम्* matchp = terminalActivep;
            सचे (matchCondp) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sampledCondp = sampled(matchCondp->clone_वृक्षः_Pure(मिच्छा));
                matchp = नूतन AstLogAnd{flp, matchp, sampledCondp};
            }
            सचे (outMatchpp) {
                Ast_ग्रन्थिः_व्यञ्जकम्* notPMatchp = nullptr;
                सचे (matchCondp && rejectBasep) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sampledCondp = sampled(matchCondp->clone_वृक्षः_Pure(मिच्छा));
                    notPMatchp = नूतन AstLogAnd{flp, rejectBasep->clone_वृक्षः_Pure(मिच्छा),
                                               नूतन AstLogNot{flp, sampledCondp}};
                } अञ्ञथा सचे (rejectBasep) {
                    notPMatchp = rejectBasep->clone_वृक्षः_Pure(मिच्छा);
                }
                सचे (throughoutRejectp)
                    notPMatchp = orExprs(flp, notPMatchp, throughoutRejectp->clone_वृक्षः_Pure(मिच्छा));
                सचे (requiredStepRejectp)
                    notPMatchp
                        = orExprs(flp, notPMatchp, requiredStepRejectp->clone_वृक्षः_Pure(मिच्छा));
                *outMatchpp = notPMatchp;
            }
            सचे (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_वृक्षः(), throughoutRejectp);
            सचे (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_वृक्षः(), rejectBasep);
            सचे (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_वृक्षः(), requiredStepRejectp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव resultExprp = नूतन AstLogNot{flp, matchp};
            पटिनिवत्त resultExprp;
        }
        सचे (isCover) {
            सचे (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_वृक्षः(), throughoutRejectp);
            सचे (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_वृक्षः(), rejectBasep);
            सचे (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_वृक्षः(), requiredStepRejectp);
            सचे (matchCondp) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sampledCondp = sampled(matchCondp->clone_वृक्षः_Pure(मिच्छा));
                पटिनिवत्त नूतन AstLogAnd{flp, terminalActivep, sampledCondp};
            }
            पटिनिवत्त terminalActivep;
        }
        // Assert/assume: output = !reject
        Ast_ग्रन्थिः_व्यञ्जकम्* rejectp = nullptr;
        सचे (matchCondp && rejectBasep) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sampledCondp = sampled(matchCondp->clone_वृक्षः_Pure(मिच्छा));
            rejectp = नूतन AstLogAnd{flp, rejectBasep, नूतन AstLogNot{flp, sampledCondp}};
        } अञ्ञथा सचे (rejectBasep) {
            VL_DO_DANGLING(rejectBasep->delete_वृक्षः(), rejectBasep);
        }
        सचे (outMatchpp) {
            Ast_ग्रन्थिः_व्यञ्जकम्* matchExprp = terminalActivep->clone_वृक्षः_Pure(मिच्छा);
            सचे (matchCondp) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sp = sampled(matchCondp->clone_वृक्षः_Pure(मिच्छा));
                matchExprp = नूतन AstLogAnd{flp, matchExprp, sp};
            }
            *outMatchpp = matchExprp;
        }
        सचे (terminalActivep) VL_DO_DANGLING(terminalActivep->delete_वृक्षः(), terminalActivep);
        सचे (throughoutRejectp) rejectp = orExprs(flp, rejectp, throughoutRejectp);
        सचे (requiredStepRejectp) rejectp = orExprs(flp, rejectp, requiredStepRejectp);
        सचे (!rejectp) पटिनिवत्त नूतन AstConst{flp, AstConst::BitTrue{}};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव resultExprp = नूतन AstLogNot{flp, rejectp};
        पटिनिवत्त resultExprp;
    }

    explicit SvaNfaLowering(Ast_ग्रन्थिः_Module* modp)
        : m_modp{modp}
        , m_u32DTypep{modp->findBasicDType(VBasicDTypeKwd::UINT32)} {}
    ~SvaNfaLowering() {
        V3Stats::addStatSum("Assertions, NFA delay ring edge visits", m_statDelayRingEdgeVisits);
    }

    // Lower NFA graph to synthesizable AstAlways blocks and raw result signals.
    // Links are combinational; Edges are registered (NBA).
    SignalSet lower(Ast_ग्रन्थिः_Cover_Or_Assert* ध्रुव assertp, SvaGraph& graph,
                    Ast_Sen_वृक्षः* ध्रुव senTreep, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव matchCondp,
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव disableExprp, ध्रुव तार्किक need_Thread_Fail_गणनम्,
                    ध्रुव तार्किक need_Throughout_Thread_Fail_गणनम्) {
        File_पङ्क्तिः* ध्रुव flp = assertp->fileline();
        AstCover* ध्रुव coverp = VN_CAST(assertp, Cover);
        ध्रुव तार्किक isSeqEvent = coverp && coverp->isSeqEvent();
        ध्रुव VAssertType assertType
            = isSeqEvent ? VAssertType{VAssertType::INTERNAL} : assertp->userType();
        ध्रुव VAssertDirectiveType directiveType
            = isSeqEvent ? VAssertDirectiveType{VAssertDirectiveType::INTERNAL}
                         : assertp->directive();
        ध्रुव std::string baseName = m_names.get("");

        // Number vertices with sequential colors for array indexing.
        पूर्णाङ्क N = 0;
        अत्थाय (V3GraphVertex& vtxr : graph.m_graph.vertices()) { vtxr.color(N++); }
        std::vector<SvaStateVertex*> vtx(N, nullptr);
        अत्थाय (V3GraphVertex& vtxr : graph.m_graph.vertices()) {
            vtx[vtxr.color()] = static_cast<SvaStateVertex*>(&vtxr);
        }

        // Allocate per-vertex lowering data (stored via V3GraphVertex::userp()).
        std::vector<SvaVertexData> vertexData(N);
        अत्थाय (पूर्णाङ्क i = 0; i < N; ++i) vtx[i]->userp(&vertexData[i]);

        // Identify registered vertices (targets of clocked edges).
        अत्थाय (पूर्णाङ्क i = 0; i < N; ++i) {
            अत्थाय (ध्रुव V3GraphEdge& edger : vtx[i]->outEdges()) {
                ध्रुव SvaTransEdge& tedger = static_cast<ध्रुव SvaTransEdge&>(edger);
                ध्रुव पूर्णाङ्क toIdx = tedger.toVtxp()->color();
                सचे (tedger.m_consumesCycle && tedger.toVtxp() != graph.m_matchVertexp
                    && !tedger.toVtxp()->m_isRejectSink) {
                    vtx[toIdx]->datap()->needsReg = सच्च;
                }
            }
        }

        // A packed range ring can emit several live endpoints without needing per-slot counts.
        // Widen only a ring whose input can already carry multiplicity.
        std::vector<तार्किक> stateMayHaveMultiplicity(N);
        std::vector<तार्किक> ringNeedsMultiplicity(N);
        std::vector<पूर्णाङ्क> multiplicityWork;
        ध्रुव तार्किक गणनम्_Matches = coverp && coverp->isCoverSeq() && !isSeqEvent;
        सचे (गणनम्_Matches) {
            अत्थाय (पूर्णाङ्क i = 0; i < N; ++i) {
                ध्रुव तार्किक multipleInputs = vtx[i]->inEdges().size() > 1;
                ध्रुव तार्किक isRing = vtx[i]->m_delayRingSize;
                ringNeedsMultiplicity[i] = isRing && multipleInputs;
                ध्रुव तार्किक startsMultiplicity = isRing
                                                    ? !vtx[i]->m_isFixedDelayRing
                                                    : multipleInputs && !vtx[i]->datap()->needsReg;
                सचे (startsMultiplicity) {
                    stateMayHaveMultiplicity[i] = सच्च;
                    multiplicityWork.प्रक्षिप_back(i);
                }
            }
            अत्थाय (size_t wi = 0; wi < multiplicityWork.size(); ++wi) {
                अत्थाय (ध्रुव V3GraphEdge& edger : vtx[multiplicityWork[wi]]->outEdges()) {
                    ध्रुव SvaTransEdge& tedger = static_cast<ध्रुव SvaTransEdge&>(edger);
                    ध्रुव पूर्णाङ्क ti = tedger.toVtxp()->color();
                    ध्रुव तार्किक isRing = vtx[ti]->m_delayRingSize;
                    सचे (isRing) ringNeedsMultiplicity[ti] = सच्च;
                    // Match and reject sinks have no outgoing edges, so they stop naturally.
                    ध्रुव तार्किक stopsMultiplicity = !isRing && vtx[ti]->datap()->needsReg;
                    सचे (stateMayHaveMultiplicity[ti] || stopsMultiplicity) { अनुवर्त; }
                    stateMayHaveMultiplicity[ti] = सच्च;
                    multiplicityWork.प्रक्षिप_back(ti);
                }
            }
        }

        ध्रुव तार्किक asyncDisable = isAsyncDisable(disableExprp);
        // Registers cleared on a rising disable are also written by their clocked block.
        File_पङ्क्तिः* ध्रुव regFlp = asyncDisable ? नूतन File_पङ्क्तिः{flp} : flp;
        सचे (asyncDisable) regFlp->warnOff(V3ErrorCode::MULTIDRIVEN, सच्च);

        AstVar* ध्रुव killVarp
            = नूतन AstVar{flp, VVarType::MODULETEMP, baseName + "__kill", m_u32DTypep};
        killVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(killVarp);
        अत्थाय (पूर्णाङ्क i = 0; i < N; ++i) {
            सचे (vtx[i]->m_isAndCombiner) {
                ध्रुव std::string base = baseName + "__a" + std::to_string(i);
                AstVar* ध्रुव lp = नूतन AstVar{regFlp, VVarType::MODULETEMP, base + "_doneL",
                                              m_modp->findBitDType()};
                lp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(lp);
                vtx[i]->datap()->doneLVarp = lp;
                AstVar* ध्रुव rp = नूतन AstVar{regFlp, VVarType::MODULETEMP, base + "_doneR",
                                              m_modp->findBitDType()};
                rp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(rp);
                vtx[i]->datap()->doneRVarp = rp;
                अनुवर्त;
            }
            सचे (vtx[i]->m_delayRingSize) {
                ध्रुव std::string base = baseName + "__d" + std::to_string(i);
                ध्रुव तार्किक storesCounts = गणनम्_Matches && ringNeedsMultiplicity[i];
                Ast_ग्रन्थिः_DType* ringDTypep = nullptr;
                सचे (storesCounts) {
                    // int unsigned ring [size];
                    ringDTypep = नूतन AstUnpackArrayDType{
                        flp, m_u32DTypep,
                        नूतन AstRange{flp, static_cast<पूर्णाङ्क>(vtx[i]->m_delayRingSize - 1), 0}};
                    v3Global.rootp()->typeTablep()->addTypesp(ringDTypep);
                } अञ्ञथा {
                    // bit [size-1:0] ring;
                    ringDTypep = m_modp->findBitDType(vtx[i]->m_delayRingSize,
                                                      vtx[i]->m_delayRingSize, VSigning::UNSIGNED);
                }
                AstVar* ध्रुव ringp
                    = नूतन AstVar{flp, VVarType::MODULETEMP, base + "_ring", ringDTypep};
                ringp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(ringp);
                vtx[i]->datap()->delayRingVarp = ringp;
                vtx[i]->datap()->delayRingCounts = storesCounts;
                AstVar* ध्रुव headp
                    = नूतन AstVar{flp, VVarType::MODULETEMP, base + "_head",
                                 storesCounts ? m_u32DTypep : m_modp->findBitDType()};
                headp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(headp);
                vtx[i]->datap()->delayRingHeadVarp = headp;
                // int unsigned idx;
                AstVar* ध्रुव idxp
                    = नूतन AstVar{regFlp, VVarType::MODULETEMP, base + "_idx", m_u32DTypep};
                idxp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(idxp);
                vtx[i]->datap()->delayRingIdxVarp = idxp;
                // int unsigned live_count;
                AstVar* ध्रुव live_गणनम्_Varp
                    = नूतन AstVar{regFlp, VVarType::MODULETEMP, base + "_liveCount", m_u32DTypep};
                live_गणनम्_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(live_गणनम्_Varp);
                vtx[i]->datap()->delay_Ring_Live_गणनम्_Varp = live_गणनम्_Varp;
                AstVar* ध्रुव wrappedp = नूतन AstVar{regFlp, VVarType::MODULETEMP,
                                                    base + "_wrapped", m_modp->findBitDType()};
                wrappedp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(wrappedp);
                vtx[i]->datap()->delayRingWrappedVarp = wrappedp;
                अनुवर्त;
            }
            सचे (!vtx[i]->datap()->needsReg) अनुवर्त;
            सचे (vtx[i] == graph.m_startVertexp || vtx[i]->m_isMatch) अनुवर्त;
            ध्रुव std::string varName = baseName + "__s" + std::to_string(i);
            AstVar* ध्रुव varp
                = नूतन AstVar{regFlp, VVarType::MODULETEMP, varName, m_modp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(varp);
            vtx[i]->datap()->stateVarp = varp;
        }

        // Build lowering context for phase sub-functions.
        LowerCtx c{flp,        senTreep,      disableExprp, matchCondp,
                   assertType, directiveType, killVarp,     graph};

        // Phase 1: Resolve combinational Links via fixed-point propagation.
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव triggerExprp
            = isSeqEvent ? नूतन AstConst{flp, AstConst::BitTrue{}}
                         : assertOnCond(flp, assertp->userType(), assertp->directive());
        resolveLinks(c, triggerExprp, गणनम्_Matches);
        VL_DO_DANGLING(triggerExprp->delete_वृक्षः(), triggerExprp);

        // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
        emitStateRegisterNba(c);
        emitDelayRingUpdates(c, गणनम्_Matches);
        emitAndCombinerDoneLatchNba(c);
        emitKillAckNba(c);

        // Phase 3/3a/3b: Compute terminal match/reject signals (cleans up stateSig).
        ध्रुव SignalSet sigs
            = computeSignals(c, need_Thread_Fail_गणनम्, need_Throughout_Thread_Fail_गणनम्, गणनम्_Matches);

        // Strong s_always[m:n] end-of-simulation liveness: if any in-window state
        // is still set at $finish, the universal-quantifier window never completed
        // (IEEE 1800-2023 16.12.11 strong semantics). Fire the assertion failure
        // from a final block; V3Assert turns the DT_ERROR display into the standard
        // "Assertion failed in %m" message.
        // pending = |{strong state registers, strong delay live counts != 0};
        Ast_ग्रन्थिः_व्यञ्जकम्* pendingp = nullptr;
        अत्थाय (पूर्णाङ्क i = 0; i < N; ++i) {
            सचे (!vtx[i]->m_strongPending) अनुवर्त;
            Ast_ग्रन्थिः_व्यञ्जकम्* pendingExprp = nullptr;
            सचे (vtx[i]->datap()->stateVarp) {
                pendingExprp = नूतन AstVarRef{flp, vtx[i]->datap()->stateVarp, VAccess::READ};
            } अञ्ञथा सचे (vtx[i]->m_strongAlwaysRing) {
                pendingExprp
                    = delayRingHasLiveThreadsp(flp, vtx[i]->datap()->delay_Ring_Live_गणनम्_Varp);
            } अञ्ञथा {
                अनुवर्त;
            }
            सचे (!pendingp) {
                pendingp = pendingExprp;
            } अञ्ञथा {
                pendingp = नूतन AstLogOr{flp, pendingp, pendingExprp};
            }
        }
        सचे (pendingp) {
            AstCExpr* ध्रुव assertOnp
                = नूतन AstCExpr{flp, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()", 1};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = नूतन AstLogAnd{flp, assertOnp, pendingp};
            AstDisplay* ध्रुव dispp
                = नूतन AstDisplay{flp, VDisplayType::DT_ERROR, "", nullptr, nullptr};
            dispp->fmtp()->timeunit(m_modp->timeunit());
            Ast_ग्रन्थिः_Stmt* ध्रुव firep = dispp;
            सचे (v3Global.opt.stopFail()) firep->addNext(नूतन AstStop{flp, मिच्छा});
            m_modp->addStmtsp(नूतन AstFinal{flp, नूतन AstIf{flp, condp, firep}});
        }

        // Clear userp on every vertex before vertexData is destroyed.
        अत्थाय (पूर्णाङ्क i = 0; i < N; ++i) vtx[i]->userp(nullptr);
        पटिनिवत्त sigs;
    }
};

}  // namespace

//######################################################################
// Top-level visitor

वग्ग AssertNfaVisitor final : सार्वजनिक VNVisitor {
    // STATE
    Ast_ग्रन्थिः_Module* m_modp = nullptr;  // Current module being processed
    AstClocking* m_defaultClockingp = nullptr;  // Default clocking
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff
    SvaNfaLowering* m_loweringp = nullptr;  // NFA-to-hardware lowering engine
    Ast_Sen_वृक्षः* m_sampled_मानम्_Clockp = nullptr;  // Inherited clock during scoped attachment
    V3UniqueNames m_propVarNames{"__Vpropvar"};  // Property-local variable names
    V3UniqueNames m_propTempNames{"__VnfaSampled"};  // Hoisted $sampled(propp) temps
    V3UniqueNames m_fail_गणनम्_Names{"__VnfaRemainingFailCount"};  // Fail replay counter names
    std::set<ध्रुव AstProperty*> m_inliningProps;  // Recursion guard for inlineNamedProperty

    template <typename T_ग्रन्थिः>
    शून्य visit_Sampled_मानम्(T_ग्रन्थिः* ध्रुव nodep) {
        सचे (m_sampled_मानम्_Clockp && !nodep->sentreep()) {
            nodep->sentreep(m_sampled_मानम्_Clockp->clone_वृक्षः(सच्च));
        }
        iterateChildren(nodep);
    }

    // Wire match vertex and mid-window sources for a successful NFA build.
    स्थिर शून्य wireMatchAndMidSources(SvaGraph& graph, ध्रुव BuildResult& result, File_पङ्क्तिः* flp) {
        graph.createMatchVertex();
        // Skip the main term Link when midSources already cover every
        // end-of-match (cover_sequence path); otherwise the per-mid extraction
        // double-counts via the merge vertex.
        सचे (!result.termIsMidMerge) { graph.addLink(result.termVertexp, graph.m_matchVertexp); }
        अत्थाय (SvaStateVertex* srcVtxp : result.midSources) {
            Ast_ग्रन्थिः_व्यञ्जकम्* condp = nullptr;
            अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव tc : srcVtxp->m_throughoutConds) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव tcClone = tc->clone_वृक्षः_Pure(मिच्छा);
                condp = condp ? नूतन AstLogAnd{flp, condp, tcClone} : tcClone;
            }
            graph.addLink(srcVtxp, graph.m_matchVertexp, condp);
            srcVtxp->m_isUnbounded = सच्च;
        }
    }

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* getSequenceBodyExprp(ध्रुव AstSequence* seqp) {
        Ast_ग्रन्थिः* bodyp = seqp->stmtsp();
        याव (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        पटिनिवत्त VN_CAST(bodyp, ग्रन्थिः_व्यञ्जकम्);
    }

    स्थिर AstPropSpec* getPropertySpecp(ध्रुव AstProperty* propp) {
        Ast_ग्रन्थिः* stmtp = propp->stmtsp();
        // V3LinkParse emits InitialStaticStmt for property-local variable
        // initialisers; the InitialAutomaticStmt variant only appears for
        // task/function-scope automatic lifetime, not properties.
        याव (stmtp
               && (VN_IS(stmtp, Var) || VN_IS(stmtp, InitialStaticStmt)
                   || VN_IS(stmtp, InitialAutomaticStmt))) {  // LCOV_EXCL_LINE
            stmtp = stmtp->nextp();
        }
        पटिनिवत्त VN_CAST(stmtp, PropSpec);
    }

    शून्य inlineNamedProperty(AstPropSpec* outerSpecp, AstFuncRef* funcrefp,
                             ध्रुव AstProperty* propyp) {
        // Recursion guard: IEEE 1800-2023 16.12.1 forbids recursive properties.
        // V3Width emits "Recursive property call" for direct recursion before this
        // pass runs; this catches any nested-inlining cycle that slips past.
        सचे (!m_inliningProps.emplace(propyp).second) {
            funcrefp->v3error("Illegal recursive property reference");  // LCOV_EXCL_LINE
            पटिनिवत्त;  // LCOV_EXCL_LINE
        }
        संरचना Guard final {
            std::set<ध्रुव AstProperty*>& setr;
            ध्रुव AstProperty* keyp;
            ~Guard() { setr.erase(keyp); }
        } guard{m_inliningProps, propyp};
        AstPropSpec* propSpecp = getPropertySpecp(propyp);
        UASSERT_OBJ(propSpecp, funcrefp, "Property has no body PropSpec");
        propSpecp = propSpecp->clone_वृक्षः(मिच्छा);

        ध्रुव V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propyp->stmtsp());
        std::unordered_map<ध्रुव AstVar*, Ast_ग्रन्थिः_व्यञ्जकम्*> portMap;
        अत्थाय (ध्रुव auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables to module-level temps (IEEE 16.10).
        std::unordered_map<ध्रुव AstVar*, AstVar*> localVarMap;
        अत्थाय (Ast_ग्रन्थिः* stmtp = propyp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            सचे (AstVar* ध्रुव varp = VN_CAST(stmtp, Var)) {
                सचे (!varp->isIO()) {
                    ध्रुव string newName = m_propVarNames.get(varp);
                    AstVar* ध्रुव newVarp = नूतन AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        propSpecp->foreach([&](AstVarRef* refp) {
            ध्रुव auto portIt = portMap.find(refp->varp());
            सचे (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_वृक्षः(मिच्छा));
                VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
                पटिनिवत्त;
            }
            ध्रुव auto localIt = localVarMap.find(refp->varp());
            सचे (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        अत्थाय (ध्रुव auto& tconnect : tconnects) {
            प्रक्षिप_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propSpecp, funcrefp);

        // Merge disable iff (IEEE 1800-2023 16.12.1)
        सचे (outerSpecp->disablep() && propSpecp->disablep()) {
            outerSpecp->v3error("disable iff expression before property call "
                                "and in its body is not legal");
            प्रक्षिप_Deletep(propSpecp->disablep()->unlinkFrBack());
        }
        सचे (outerSpecp->disablep()) {
            propSpecp->disablep(outerSpecp->disablep()->unlinkFrBack());
        }

        सचे (outerSpecp->sensesp() && propSpecp->sensesp()) {
            outerSpecp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Clock event before property call and in its body");
            प्रक्षिप_Deletep(propSpecp->sensesp()->unlinkFrBack());
        }
        सचे (outerSpecp->sensesp()) {
            AstSenItem* ध्रुव sensesp = outerSpecp->sensesp();
            sensesp->unlinkFrBack();
            propSpecp->sensesp(sensesp);
        }

        outerSpecp->replaceWith(propSpecp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(outerSpecp), outerSpecp);
    }

    शून्य inlineSequenceRef(AstFuncRef* funcrefp, AstSequence* seqp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव clonedp = bodyExprp->clone_वृक्षः(मिच्छा);

        ध्रुव V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ध्रुव AstVar*, Ast_ग्रन्थिः_व्यञ्जकम्*> portMap;
        अत्थाय (ध्रुव auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ध्रुव auto it = portMap.find(refp->varp());
            सचे (it != portMap.end()) {
                refp->replaceWith(it->second->clone_वृक्षः(मिच्छा));
                VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
            }
        });
        अत्थाय (ध्रुव auto& tconnect : tconnects) {
            प्रक्षिप_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(funcrefp), funcrefp);
        // Clear referenced flag so V3AssertPre cleanup does not emit
        // spurious UNSUPPORTED for sequences that were already inlined here.
        seqp->isReferenced(मिच्छा);
    }

    // Must run before hasMultiCycleExpr() so NFA sees sequence bodies.
    शून्य inlineAllSequenceRefs(Ast_ग्रन्थिः* rootp) {
        तार्किक changed = सच्च;
        याव (changed) {
            changed = मिच्छा;
            rootp->foreach([&](AstFuncRef* funcrefp) {
                सचे (changed) पटिनिवत्त;
                सचे (AstSequence* ध्रुव seqp = VN_CAST(funcrefp->taskp(), Sequence)) {
                    inlineSequenceRef(funcrefp, seqp);
                    changed = सच्च;
                }
            });
        }
    }

    स्थिर तार्किक has_Multi_Cycle_व्यञ्जकम्(ध्रुव Ast_ग्रन्थिः* nodep) {
        पटिनिवत्त nodep->exists([](ध्रुव Ast_ग्रन्थिः* np) {
            सचे (ध्रुव auto* ध्रुव ep = VN_CAST(np, ग्रन्थिः_व्यञ्जकम्)) पटिनिवत्त ep->isMultiCycleSva();
            पटिनिवत्त मिच्छा;
        });
    }

    स्थिर VPropStrength effectiveAssertPropStrength(ध्रुव AstPropSpec* ध्रुव propSpecp) {
        सचे (propSpecp->propStrength() != VPropStrength::DEFAULT) पटिनिवत्त propSpecp->propStrength();
        पटिनिवत्त propSpecp->fileline()->language() <= V3LangCode::L1800_2005 ? VPropStrength::STRONG
                                                                           : VPropStrength::WEAK;
    }

    // Bare `assert property (p until q)` with boolean operands stays on
    // V3AssertPre's AstLoop lowering, which preserves per-attempt action-block
    // firings that this NFA's single-bit aggregated state cannot. Strong bare
    // forms are also lowered there. NFA still owns sequence operands and any
    // embedding inside a multi-cycle context (implication consequent, or/and
    // operands, etc.).
    स्थिर तार्किक is_Bare_शिखरम्_Level_Until(Ast_ग्रन्थिः* propp) {
        Ast_ग्रन्थिः* p = propp;
        सचे (AstPropSpec* ध्रुव specp = VN_CAST(p, PropSpec)) p = specp->propp();
        याव (AstLogNot* ध्रुव notp = VN_CAST(p, LogNot)) p = notp->lhsp();
        AstUntil* ध्रुव untilp = VN_CAST(p, Until);
        सचे (!untilp) पटिनिवत्त मिच्छा;
        ध्रुव auto hasSeq = [](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ep) {
            पटिनिवत्त ep->exists([](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* np) { पटिनिवत्त np->isMultiCycleSva(); });
        };
        सचे (hasSeq(untilp->lhsp()) || hasSeq(untilp->rhsp())) पटिनिवत्त मिच्छा;
        पटिनिवत्त सच्च;
    }

    संरचना PropertyParts final {
        Ast_ग्रन्थिः_व्यञ्जकम्* triggerExprp = nullptr;
        Ast_ग्रन्थिः_व्यञ्जकम्* seqExprp = nullptr;
        तार्किक isOverlapped = सच्च;
        तार्किक hasImplication = मिच्छा;
        तार्किक isFollowedBy = मिच्छा;  // True for #-# / #=# (non-vacuous-fail on antecedent miss)
    };

    स्थिर PropertyParts decomposeProperty(Ast_ग्रन्थिः* propp) {
        PropertyParts parts;
        सचे (AstPropSpec* ध्रुव specp = VN_CAST(propp, PropSpec)) { propp = specp->propp(); }
        सचे (AstImplication* ध्रुव implp = VN_CAST(propp, Implication)) {
            parts.hasImplication = सच्च;
            parts.isOverlapped = implp->isOverlapped();
            parts.isFollowedBy = implp->isFollowedBy();
            parts.triggerExprp = implp->lhsp();
            parts.seqExprp = implp->rhsp();
        } अञ्ञथा सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = VN_CAST(propp, ग्रन्थिः_व्यञ्जकम्)) {
            parts.triggerExprp = nullptr;
            parts.seqExprp = exprp;
        }
        पटिनिवत्त parts;
    }

    स्थिर तार्किक canSplitImplicationPassActions(ध्रुव PropertyParts& parts) {
        UASSERT(parts.hasImplication,
                "Implication pass action split requested without implication");
        UASSERT(parts.triggerExprp, "Implication pass action split requested without trigger");
        // Direct vacuous/nonvacuous classification uses the antecedent value in the current
        // assertion attempt. Leave delayed antecedents on the existing NFA pass path.
        पटिनिवत्त !has_Multi_Cycle_व्यञ्जकम्(parts.triggerExprp);
    }

    स्थिर शून्य splitImplicationPassActions(AstAssert* assertp, ध्रुव PropertyParts& parts,
                                            Ast_ग्रन्थिः_व्यञ्जकम्* nonvacuousMatchp) {
        File_पङ्क्तिः* ध्रुव flp = assertp->fileline();

        Ast_ग्रन्थिः* ध्रुव passsp = assertp->passsp()->unlinkFrBackWithNext();
        Ast_ग्रन्थिः* splitsp = nullptr;

        सचे (!parts.isFollowedBy) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव vacuousp
                = नूतन AstLogNot{flp, sampled(parts.triggerExprp->clone_वृक्षः_Pure(मिच्छा))};
            Ast_ग्रन्थिः* ध्रुव vacuousBodyp = passsp->clone_वृक्षः(मिच्छा);
            splitsp = newPassOnIf(flp, vacuousp, vacuousBodyp, assertp->userType(),
                                  assertp->directive(), /*vacuous=*/सच्च);
        }

        AstIf* ध्रुव nonvacuousp
            = newPassOnIf(flp, nonvacuousMatchp, passsp, assertp->userType(), assertp->directive(),
                          /*vacuous=*/मिच्छा);
        splitsp = splitsp ? Ast_ग्रन्थिः::addNext<Ast_ग्रन्थिः, Ast_ग्रन्थिः>(splitsp, nonvacuousp)
                          : static_cast<Ast_ग्रन्थिः*>(nonvacuousp);
        assertp->addPasssp(splitsp);
    }

    // On a PropSpec-wrapped assertion whose NFA build failed with a semantic
    // error (errorEmitted), replace the body with a BitFalse const so later
    // passes see a well-formed AST. Returns true if replaced.
    शून्य replaceBodyOnBuildError(File_पङ्क्तिः* flp, AstPropSpec* propSpecp, तार्किक errorEmitted) {
        सचे (!errorEmitted) पटिनिवत्त;
        Ast_ग्रन्थिः* ध्रुव innerPropp = propSpecp->propp();
        innerPropp->replaceWith(नूतन AstConst{flp, AstConst::BitFalse{}});
        VL_DO_DANGLING(प्रक्षिप_Deletep(innerPropp), innerPropp);
    }

    // Hoist a leading clocking event (IEEE 1800-2023 16.7):
    तार्किक hoistClockedSeq(AstPropSpec* specp) {
        याव (AstSClocked* ध्रुव clockedp = VN_CAST(specp->propp(), SClocked)) {
            सचे (specp->sensesp()) {
                clockedp->v3warn(E_UNSUPPORTED, "Unsupported: multiclocked sequence or property");
                replaceBodyOnBuildError(specp->fileline(), specp, सच्च);
                पटिनिवत्त सच्च;
            }
            अत्थाय (ध्रुव AstSenItem* sp = clockedp->sensesp(); sp;
                 sp = VN_CAST(sp->nextp(), SenItem)) {
                सचे (!sp->edgeType().anEdge()) {
                    clockedp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: non-edge clocking event on a sequence; "
                                     "use an edge such as @(posedge clk)");
                    replaceBodyOnBuildError(specp->fileline(), specp, सच्च);
                    पटिनिवत्त सच्च;
                }
            }
            specp->sensesp(clockedp->sensesp()->unlinkFrBackWithNext());
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bodyp = clockedp->exprp()->unlinkFrBack();
            clockedp->replaceWith(bodyp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(clockedp), clockedp);
        }
        // A clocking event anywhere else in the sequence is not supported.
        ध्रुव AstSClocked* nestedp = nullptr;
        specp->propp()->foreach([&](ध्रुव AstSClocked* p) {
            सचे (!nestedp) nestedp = p;
        });
        सचे (nestedp) {
            nestedp->v3warn(E_UNSUPPORTED,
                            "Unsupported: clocking event inside sequence expression");
            replaceBodyOnBuildError(specp->fileline(), specp, सच्च);
            पटिनिवत्त सच्च;
        }
        पटिनिवत्त मिच्छा;
    }

    // Build the NFA graph for a property body, handling both the antecedent
    // |-> consequent and simple sequence cases. Returns the consequent/body
    // BuildResult (invalid on parse/build failure).
    BuildResult buildAssertionGraph(SvaNfaBuilder& builder, SvaGraph& graph, Ast_ग्रन्थिः_व्यञ्जकम्* seqBodyp,
                                    ध्रुव PropertyParts& parts, File_पङ्क्तिः* flp) {
        सचे (!parts.hasImplication) पटिनिवत्त builder.build(seqBodyp);

        graph.m_startVertexp = graph.createStateVertex();
        पटिनिवत्त builder.buildImplicationEdges(parts.triggerExprp, seqBodyp, graph.m_startVertexp,
                                             parts.isOverlapped, parts.isFollowedBy,
                                             parts.triggerExprp, flp);
    }

    // Install pass-action gating and replay simultaneous per-thread failures.
    शून्य attachActionHandlers(AstAssert* ध्रुव assertp, Ast_ग्रन्थिः_व्यञ्जकम्* matchExprp,
                              Ast_Sen_वृक्षः* ध्रुव threadFailReplaySenTreep,
                              Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव threadFailCountp) {
        // Gate pass handler on match to prevent vacuous-pass firings.
        सचे (matchExprp) {
            // needMatch implies passsp() was non-null when evaluated above;
            // lowering does not mutate the assert's pass-action between the
            // two reads, so passsp() is still non-null here.
            Ast_ग्रन्थिः* passsp = assertp->passsp();
            UASSERT_OBJ(passsp, assertp, "needMatch set but passsp is null");
            passsp->unlinkFrBackWithNext();
            File_पङ्क्तिः* ध्रुव flp = assertp->fileline();
            AstIf* ध्रुव ifp = नूतन AstIf{flp, matchExprp, passsp, nullptr};
            assertp->addPasssp(ifp);
            // Fail-handler prefix for overlapping instances (IEEE 16.12):
            // fires when reject=1 && match=1 in the same cycle.
            सचे (assertp->failsp()) assertp->prependFailsp(ifp->clone_वृक्षः(मिच्छा));
        }

        सचे (threadFailCountp) {
            UASSERT_OBJ(threadFailReplaySenTreep, assertp,
                        "Thread fail count missing sensitivity tree");
            Ast_ग्रन्थिः* ध्रुव failsp = assertp->failsp();
            File_पङ्क्तिः* ध्रुव flp = assertp->fileline();
            // IEEE 1800-2023 16.12 requires one action-block evaluation per failed
            // thread. AstAssert handles the first, so replay the rest here.
            AstVar* ध्रुव remaining_Fail_गणनम्_Varp
                = नूतन AstVar{flp, VVarType::MODULETEMP, m_fail_गणनम्_Names.get(""),
                             m_modp->findBasicDType(VBasicDTypeKwd::UINT32)};
            remaining_Fail_गणनम्_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remaining_Fail_गणनम्_Varp);
            Ast_ग्रन्थिः* ध्रुव replayStmtsp = नूतन AstAssign{
                flp, नूतन AstVarRef{flp, remaining_Fail_गणनम्_Varp, VAccess::WRITE}, threadFailCountp};
            AstLoop* ध्रुव replayLoopp = नूतन AstLoop{flp};
            replayLoopp->addStmtsp(नूतन AstLoopTest{
                flp, replayLoopp,
                नूतन AstGt{flp, नूतन AstVarRef{flp, remaining_Fail_गणनम्_Varp, VAccess::READ},
                          newTypedConstp(flp, remaining_Fail_गणनम्_Varp->dtypep(), 1)}});
            replayLoopp->addStmtsp(newIfAssertFailOn(failsp->clone_वृक्षः(सच्च), assertp->directive(),
                                                     assertp->userType()));
            AstSub* ध्रुव decrementedFailCountp
                = नूतन AstSub{flp, नूतन AstVarRef{flp, remaining_Fail_गणनम्_Varp, VAccess::READ},
                             newTypedConstp(flp, remaining_Fail_गणनम्_Varp->dtypep(), 1)};
            replayLoopp->addStmtsp(
                नूतन AstAssign{flp, नूतन AstVarRef{flp, remaining_Fail_गणनम्_Varp, VAccess::WRITE},
                              decrementedFailCountp});
            replayStmtsp->addNext(replayLoopp);
            m_modp->addStmtsp(
                नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, threadFailReplaySenTreep, replayStmtsp});
        }
    }

    // Replace one VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). No-op if refp is not in matchMap.
    शून्य substituteMatchItemRef(AstVarRef* refp, unsigned K,
                                ध्रुव std::unordered_map<ध्रुव AstVar*, Ast_ग्रन्थिः_व्यञ्जकम्*>& matchMap) {
        ध्रुव auto it = matchMap.find(refp->varp());
        सचे (it == matchMap.end()) पटिनिवत्त;
        Ast_ग्रन्थिः_व्यञ्जकम्* newp = it->second->clone_वृक्षः_Pure(मिच्छा);
        सचे (K > 0) {
            AstConst* ध्रुव ticksp = नूतन AstConst{refp->fileline(), AstConst::Widthed_मानम्{}, 32,
                                                  static_cast<uint32_t>(K)};
            AstPast* ध्रुव pastp = नूतन AstPast{refp->fileline(), newp, ticksp, nullptr,
                                               /* propertyTiming */ सच्च};
            pastp->dtypeFrom(newp);
            newp = pastp;
        }
        refp->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
        पटिनिवत्त;
    }

    // Recursively walk a consequent. Returns cycle length consumed and
    // substitutes each VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). Reports E_UNSUPPORTED on non-constant
    // delays or composite sequence operators and returns -1 on failure.
    पूर्णाङ्क
    walk_Substitute_Match_वस्तूनि(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, unsigned K,
                             ध्रुव std::unordered_map<ध्रुव AstVar*, Ast_ग्रन्थिः_व्यञ्जकम्*>& match_वस्तूनि) {
        सचे (AstSExpr* ध्रुव sexprp = VN_CAST(nodep, SExpr)) {
            // IEEE 1800-2023 16.9.2: cycle_delay's lhsp is a constant_expression
            // and the delay form in a sequence is always `##N`, folded by
            // V3Const + V3Param before V3AssertNfa. Range form `##[m:n]` is the
            // only user-visible reject here.
            AstDelay* ध्रुव delayp = VN_AS(sexprp->delayp(), Delay);
            UASSERT_OBJ(delayp->isCycleDelay() && VN_IS(delayp->lhsp(), Const), sexprp,
                        "SVA cycle delay must have a constant lhsp");
            सचे (delayp->isRangeDelay()) {
                sexprp->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                              "non-constant cycle delay in consequent"
                                              " (IEEE 1800-2023 16.10)");
                पटिनिवत्त -1;
            }
            ध्रुव unsigned delayCycles = VN_AS(delayp->lhsp(), Const)->toUInt();
            पूर्णाङ्क preLen = 0;
            सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव prep = sexprp->preExprp()) {
                preLen = walk_Substitute_Match_वस्तूनि(prep, K, match_वस्तूनि);
                सचे (preLen < 0) पटिनिवत्त -1;
            }
            ध्रुव पूर्णाङ्क bodyLen
                = walk_Substitute_Match_वस्तूनि(sexprp->exprp(), K + preLen + delayCycles, match_वस्तूनि);
            सचे (bodyLen < 0) पटिनिवत्त -1;
            पटिनिवत्त preLen + delayCycles + bodyLen;
        }
        सचे (nodep->isMultiCycleSva()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                         "composite sequence operator in consequent"
                                         " (IEEE 1800-2023 16.10)");
            पटिनिवत्त -1;
        }
        std::vector<AstVarRef*> refs;
        nodep->foreach([&refs](AstVarRef* p) { refs.प्रक्षिप_back(p); });
        अत्थाय (AstVarRef* ध्रुव refp : refs) substituteMatchItemRef(refp, K, match_वस्तूनि);
        पटिनिवत्त 0;
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
    तार्किक liftMatchItemSubstitutions(PropertyParts& parts, Ast_ग्रन्थिः_व्यञ्जकम्* seqBodyp) {
        सचे (!parts.hasImplication) पटिनिवत्त मिच्छा;
        Ast_व्यञ्जकम्_Stmt* ध्रुव व्यञ्जकम्_Stmtp = VN_CAST(parts.triggerExprp, व्यञ्जकम्_Stmt);
        सचे (!व्यञ्जकम्_Stmtp) पटिनिवत्त मिच्छा;
        // IEEE 1800-2023 16.10 BNF requires `(expr, match_item {, match_item})`
        // with at least one match item; V3LinkParse only emits ExprStmt for
        // this form and only emits AstAssign with VarRef LHS for each item.
        std::unordered_map<ध्रुव AstVar*, Ast_ग्रन्थिः_व्यञ्जकम्*> match_वस्तूनि;
        अत्थाय (Ast_ग्रन्थिः* stmtp = व्यञ्जकम्_Stmtp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstAssign* ध्रुव assignp = VN_AS(stmtp, Assign);
            AstVarRef* ध्रुव lhsRefp = VN_AS(assignp->lhsp(), VarRef);
            match_वस्तूनि[lhsRefp->varp()] = assignp->rhsp();
        }
        ध्रुव unsigned startK = parts.isOverlapped ? 0 : 1;
        ध्रुव पूर्णाङ्क length = walk_Substitute_Match_वस्तूनि(seqBodyp, startK, match_वस्तूनि);
        // Match-item substitution / strip mutates ancestor purity. Release
        // builds don't auto-clear caches on edits, so refresh here.
        VIsCached::clear_Cache_वृक्षः();
        सचे (length < 0) पटिनिवत्त सच्च;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव antBoolp = व्यञ्जकम्_Stmtp->resultp()->unlinkFrBack();
        व्यञ्जकम्_Stmtp->replaceWith(antBoolp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(व्यञ्जकम्_Stmtp), व्यञ्जकम्_Stmtp);
        parts.triggerExprp = antBoolp;
        पटिनिवत्त मिच्छा;
    }

    // Outcome counts for a property if/case are wrong in an outcome-multiplying
    // context. Returns that context, or nullptr when the shape is supported.
    स्थिर ध्रुव वर्ण* unsupportedPropertyControl(ध्रुव Ast_ग्रन्थिः_Cover_Or_Assert* assertp,
                                                  ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* seqBodyp, तार्किक negated) {
        सचे (!hasPropertyControlConjunction(seqBodyp)) पटिनिवत्त nullptr;
        सचे (negated) पटिनिवत्त "negation";
        सचे (VN_IS(assertp, Cover)) पटिनिवत्त "cover";
        सचे (VN_AS(assertp, Assert)->passsp()) पटिनिवत्त "a pass action";
        पटिनिवत्त nullptr;
    }

    // Inline property/sequence refs and reject unsupported shapes.
    // Returns the PropSpec to lower, or nullptr when fully handled here.
    AstPropSpec* prepareAssertionProp(Ast_ग्रन्थिः_Cover_Or_Assert* assertp) {
        सचे (AstPropSpec* ध्रुव specp = VN_CAST(assertp->propp(), PropSpec)) {
            सचे (AstFuncRef* ध्रुव funcrefp = VN_CAST(specp->propp(), FuncRef)) {
                सचे (ध्रुव AstProperty* ध्रुव propyp = VN_CAST(funcrefp->taskp(), Property)) {
                    inlineNamedProperty(specp, funcrefp, propyp);
                }
            }
        }

        inlineAllSequenceRefs(assertp->propp());

        सचे (AstPropSpec* ध्रुव specp = VN_CAST(assertp->propp(), PropSpec)) {
            सचे (hoistClockedSeq(specp)) पटिनिवत्त nullptr;
        }

        AstPropSpec* ध्रुव propp = VN_AS(assertp->propp(), PropSpec);
        सचे (!VN_IS(assertp, Cover)
            && effectiveAssertPropStrength(propp) == VPropStrength::STRONG) {
            propp->v3warn(E_UNSUPPORTED,
                          "Unsupported: strong property in " + assertp->verilogKwd() + ".");
            replaceBodyOnBuildError(assertp->fileline(), propp, /*errorEmitted=*/सच्च);
            पटिनिवत्त nullptr;
        }

        सचे (!has_Multi_Cycle_व्यञ्जकम्(propp)) पटिनिवत्त nullptr;
        // A nested property instance keeps its body behind the call; lowering would drop it.
        सचे (propp->exists([](ध्रुव AstFuncRef* refp) { पटिनिवत्त VN_IS(refp->taskp(), Property); })) {
            assertp->v3warn(E_UNSUPPORTED,
                            "Unsupported: property instance inside a multi-cycle property "
                            "expression");
            VL_DO_DANGLING(प्रक्षिप_Deletep(assertp->unlinkFrBack()), assertp);
            पटिनिवत्त nullptr;
        }
        सचे (is_Bare_शिखरम्_Level_Until(propp)) पटिनिवत्त nullptr;
        पटिनिवत्त propp;
    }

    शून्य processAssertion(Ast_ग्रन्थिः_Cover_Or_Assert* assertp) {
        सचे (assertp->immediate()) पटिनिवत्त;

        AstPropSpec* ध्रुव propp = prepareAssertionProp(assertp);
        सचे (!propp) पटिनिवत्त;

        PropertyParts parts = decomposeProperty(propp);
        UASSERT_OBJ(parts.seqExprp, propp, "Property body must be an expression");

        // Unwrap `not` (IEEE 1800-2023 16.12.1); odd count -> negated semantics.
        Ast_ग्रन्थिः_व्यञ्जकम्* seqBodyp = parts.seqExprp;
        तार्किक negated = मिच्छा;
        याव (AstLogNot* ध्रुव notp = VN_CAST(seqBodyp, LogNot)) {
            negated = !negated;
            seqBodyp = notp->lhsp();
        }

        ध्रुव वर्ण* ध्रुव propertyControlp
            = unsupportedPropertyControl(assertp, seqBodyp, negated);

        // Substitute property-local match-item refs in consequent with
        // $past(rhs, K) before NFA build (IEEE 1800-2023 16.10).
        सचे (liftMatchItemSubstitutions(parts, seqBodyp)) {
            AstPropSpec* ध्रुव psp = VN_CAST(assertp->propp(), PropSpec);
            UASSERT_OBJ(psp, assertp, "Concurrent assertion must have PropSpec");
            replaceBodyOnBuildError(assertp->fileline(), psp, /*errorEmitted=*/सच्च);
            पटिनिवत्त;
        }

        AstCover* ध्रुव coverp = VN_CAST(assertp, Cover);
        ध्रुव तार्किक isCoverSeq = coverp && coverp->isCoverSeq();
        // A sequence event control is not an assertion directive; no default
        // disable iff, no assertion control
        ध्रुव तार्किक isSeqEvent = coverp && coverp->isSeqEvent();
        ध्रुव तार्किक गणनम्_Matches = isCoverSeq && !isSeqEvent;
        // Inherit module defaults (IEEE 14.12, 16.15) when assertion has none.
        सचे (!propp->sensesp() && m_defaultClockingp) {
            propp->sensesp(m_defaultClockingp->sentreep()->sensesp()->clone_वृक्षः(सच्च));
        }
        सचे (!propp->disablep() && m_defaultDisablep && !isSeqEvent) {
            propp->disablep(m_defaultDisablep->condp()->clone_वृक्षः_Pure(सच्च));
        }
        सचे (!propp->sensesp()) पटिनिवत्त;
        Ast_Sen_वृक्षः* senTreep
            = नूतन Ast_Sen_वृक्षः{propp->fileline(), propp->sensesp()->clone_वृक्षः(सच्च)};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव disableExprp = propp->disablep();

        // NFA lowering clones repeated operands and may hoist them into an
        // always_comb block. Resolve implicit sampled-value clocks first, while
        // the enclosing assertion clock is still available.
        {
            VL_RESTORER(m_sampled_मानम्_Clockp);
            m_sampled_मानम्_Clockp = senTreep;
            iterate(propp->propp());
        }

        File_पङ्क्तिः* ध्रुव flp = assertp->fileline();

        SvaGraph graph;
        SvaNfaBuilder builder{graph,      m_modp,     m_propTempNames,
                              isCoverSeq, isSeqEvent, coverp != nullptr};

        ध्रुव BuildResult result = buildAssertionGraph(builder, graph, seqBodyp, parts, flp);
        सचे (result.valid()) wireMatchAndMidSources(graph, result, flp);
        सचे (!result.valid()) {
            // Fall through to V3AssertPre for unsupported constructs; only
            // replace the body on real semantic errors. Any hoisted temps
            // from this attempt become orphan MODULETEMPs; V3Dead removes
            // them along with the dead always_comb driver.
            replaceBodyOnBuildError(flp, propp, result.errorEmitted);
            VL_DO_DANGLING(प्रक्षिप_Deletep(senTreep), senTreep);
            पटिनिवत्त;
        }
        // After the build, so a construct the builder rejects reports itself.
        सचे (propertyControlp) {
            seqBodyp->v3warn(E_UNSUPPORTED,
                             "Unsupported: temporal property if/case with " << propertyControlp);
            replaceBodyOnBuildError(flp, propp, /*errorEmitted=*/सच्च);
            VL_DO_DANGLING(प्रक्षिप_Deletep(senTreep), senTreep);
            पटिनिवत्त;
        }
        सचे (गणनम्_Matches && !v3Global.opt.coverageUser()) {
            SvaNfaBuilder::freeUnlinkedCondp(result.finalCondp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(senTreep), senTreep);
            Ast_ग्रन्थिः* ध्रुव innerPropp = propp->propp();
            innerPropp->replaceWith(नूतन AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(innerPropp), innerPropp);
            पटिनिवत्त;
        }

        AstAssert* ध्रुव assertAssertp = VN_CAST(assertp, Assert);
        ध्रुव तार्किक splitImplicationPasssp = assertAssertp && assertAssertp->passsp()
                                            && parts.hasImplication
                                            && canSplitImplicationPassActions(parts);
        ध्रुव तार्किक needMatch = assertAssertp && assertAssertp->passsp()
                               && (!parts.hasImplication || splitImplicationPasssp);

        ध्रुव तार्किक needsThreadFailReplay
            = assertAssertp && assertAssertp->failsp() && !parts.hasImplication;
        ध्रुव auto signals
            = m_loweringp->lower(assertp, graph, senTreep, result.finalCondp, disableExprp,
                                 needsThreadFailReplay, needsThreadFailReplay && !negated);
        Ast_ग्रन्थिः_व्यञ्जकम्* matchExprp = nullptr;
        Ast_ग्रन्थिः_व्यञ्जकम्* outputExprp = m_loweringp->assembleResult(
            assertp, negated, result.finalCondp, signals, needMatch ? &matchExprp : nullptr);
        सचे (गणनम्_Matches) {
            UASSERT_OBJ(signals.matchCountp, coverp, "Cover sequence missing match count");
            VL_DO_DANGLING(outputExprp->delete_वृक्षः(), outputExprp);
            propp->matchCountp(signals.matchCountp);
            outputExprp = नूतन AstNeq{flp, signals.matchCountp->clone_वृक्षः_Pure(मिच्छा),
                                     newTypedConstp(flp, signals.matchCountp->dtypep(), 0)};
        }

        Ast_Sen_वृक्षः* ध्रुव threadFailReplaySenTreep
            = signals.threadFailCountp ? senTreep->clone_वृक्षः(मिच्छा) : nullptr;

        VL_DO_DANGLING(प्रक्षिप_Deletep(senTreep), senTreep);
        सचे (result.finalCondp && !result.finalCondp->backp()) प्रक्षिप_Deletep(result.finalCondp);

        सचे (splitImplicationPasssp) {
            splitImplicationPassActions(assertAssertp, parts, matchExprp);
        } अञ्ञथा {
            attachActionHandlers(assertAssertp, matchExprp, threadFailReplaySenTreep,
                                 signals.threadFailCountp);
        }

        Ast_ग्रन्थिः* ध्रुव innerPropp = propp->propp();
        innerPropp->replaceWith(outputExprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(innerPropp), innerPropp);

        UINFO(4, "NFA converted assertion at " << flp << endl);

        सचे (dumpGraphLevel() >= 6) graph.m_graph.dumpDotFilePrefixed("assert-nfa");
    }

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_loweringp);
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultDisablep);
        m_modp = nodep;
        // A class nested in a module is in the scope of the module's default clocking
        सचे (!VN_IS(nodep, Class)) m_defaultClockingp = nullptr;
        m_defaultDisablep = nodep->defaultDisablep();
        SvaNfaLowering lowering{nodep};
        m_loweringp = &lowering;
        iterateChildren(nodep);
    }
    शून्य visit(AstClocking* nodep) override {
        सचे (nodep->isDefault() && !m_defaultClockingp) m_defaultClockingp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    शून्य visit(AstDefaultDisable* nodep) override {}
    शून्य visit(AstFell* nodep) override { visit_Sampled_मानम्(nodep); }
    शून्य visit(AstPast* nodep) override { visit_Sampled_मानम्(nodep); }
    शून्य visit(AstRose* nodep) override { visit_Sampled_मानम्(nodep); }
    शून्य visit(AstStable* nodep) override { visit_Sampled_मानम्(nodep); }
    शून्य visit(AstAssert* nodep) override { processAssertion(nodep); }
    शून्य visit(AstCover* nodep) override { processAssertion(nodep); }
    शून्य visit(AstRestrict* nodep) override {
        // Restrict property is ignored by simulators (IEEE 1800-2023 16.12.2).
        // Remove here so temporal SExpr don't leak to V3AssertPre.
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstAssertIntrinsic* nodep) override {}
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    explicit AssertNfaVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Top entry point

शून्य V3AssertNfa::assertNfaAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":" << endl);
    { AssertNfaVisitor{nodep}; }
    V3Global::dump_Check_Global_वृक्षः("assertnfa", 0, dump_वृक्षः_Either_Level() >= 3);
}
