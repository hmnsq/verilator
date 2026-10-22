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

वर्ग SvaStateVertex;

// Per-vertex algorithm data, stored via V3GraphVertex::userp() during lowering
संरचना SvaVertexData final {
    AstVar* stateVarp = nullptr;  // NBA state register for this vertex
    AstVar* delayRingVarp = nullptr;  // Packed occupancy bits or per-slot match counts
    AstVar* delayRingHeadVarp
        = nullptr;  // Cached outgoing slot; isolates readers from in-place writes
    AstVar* delayRingIdxVarp = nullptr;  // Next slot written in delayRingVarp
    AstVar* delay_Ring_Live_मोजणी_Varp = nullptr;  // Number of threads in delayRingVarp
    AstVar* delayRingWrappedVarp = nullptr;  // All slots written since the last clear
    AstVar* doneLVarp = nullptr;  // SAnd LHS done-latch
    AstVar* doneRVarp = nullptr;  // SAnd RHS done-latch
    Ast_गाठ_अभिव्यक्ती* stateSigp = nullptr;  // Combinational state signal; OWNED during lowering
    Ast_गाठ_अभिव्यक्ती* state_मोजणी_Sigp = nullptr;  // Cover-sequence match count; OWNED during lowering
    Ast_गाठ_अभिव्यक्ती* state_मोजणी_Overflow_Sigp = nullptr;  // Count overflow; OWNED during lowering
    तार्किक delayRingCounts = असत्य;  // Ring slots hold cover-sequence match counts
    तार्किक needsReg = असत्य;  // True if vertex has incoming clocked edge
};

// NFA state vertex -- one per NFA position in the sequence evaluation
वर्ग SvaStateVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(SvaStateVertex, V3GraphVertex)
सार्वजनिक:
    // True if this is the sequence-match terminal vertex
    तार्किक m_isMatch = असत्य;
    // OWNED throughout-guard condition clones; IEEE 1800-2023 16.9.9
    std::vector<Ast_गाठ_अभिव्यक्ती*> m_throughoutConds;
    // Nonzero for a ring-buffer vertex for ## delays.
    तार्किक m_isFixedDelayRing = असत्य;
    अचिन्हित m_delayRingSize = 0;  // Number of ring slots. Range: max-min+1.
    Ast_गाठ_अभिव्यक्ती* m_delayRingClearCondp = nullptr;  // local RHS for pure-boolean range
    Ast_गाठ_अभिव्यक्ती* m_delayRingAdvanceCondp = nullptr;  // Advance only when this condition holds
    तार्किक m_replayAbortReject = असत्य;  // Compressed repetition needs per-thread abort replay
    // OWNED; enclosing-abort fire condition clearing state or suppressing guard rejection
    Ast_गाठ_अभिव्यक्ती* m_abortClearp = nullptr;
    // OWNED; reject-abort fire condition rejecting all represented live threads
    Ast_गाठ_अभिव्यक्ती* m_abortRejectp = nullptr;
    // Liveness terminal (IEEE weak semantics): reject must not fire from this source
    तार्किक m_isUnbounded = असत्य;
    // Temporal sequence AND combiner; IEEE 1800-2023 16.9.5
    तार्किक m_isAndCombiner = असत्य;
    SvaStateVertex* m_andLhsTermp = nullptr;  // LHS sub-NFA terminal vertex
    SvaStateVertex* m_andRhsTermp = nullptr;  // RHS sub-NFA terminal vertex
    Ast_गाठ_अभिव्यक्ती* m_andLhsCondp = nullptr;  // OWNED; LHS final condition (may be nullptr)
    Ast_गाठ_अभिव्यक्ती* m_andRhsCondp = nullptr;  // OWNED; RHS final condition (may be nullptr)
    // Reject sink for SAnd rejectOnFail wiring; not a state-signal source
    तार्किक m_isRejectSink = असत्य;
    // In-window vertex of a strong s_always[m:n]: if its state is still set at
    // end-of-simulation the universal-quantifier window never completed, which is
    // a liveness failure (IEEE 1800-2023 16.12.11 strong semantics).
    तार्किक m_strongPending = असत्य;
    // Ring introduced for the checked portion of a bounded strong s_always window.
    तार्किक m_strongAlwaysRing = असत्य;

    // CONSTRUCTORS
    explicit SvaStateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~SvaStateVertex() override {
        साठी (Ast_गाठ_अभिव्यक्ती* cp : m_throughoutConds) VL_DO_DANGLING(cp->delete_वृक्ष(), cp);
        जर (m_delayRingClearCondp)
            VL_DO_DANGLING(m_delayRingClearCondp->delete_वृक्ष(), m_delayRingClearCondp);
        जर (m_delayRingAdvanceCondp)
            VL_DO_DANGLING(m_delayRingAdvanceCondp->delete_वृक्ष(), m_delayRingAdvanceCondp);
        जर (m_abortClearp) VL_DO_DANGLING(m_abortClearp->delete_वृक्ष(), m_abortClearp);
        जर (m_abortRejectp) VL_DO_DANGLING(m_abortRejectp->delete_वृक्ष(), m_abortRejectp);
        जर (m_andLhsCondp) VL_DO_DANGLING(m_andLhsCondp->delete_वृक्ष(), m_andLhsCondp);
        जर (m_andRhsCondp) VL_DO_DANGLING(m_andRhsCondp->delete_वृक्ष(), m_andRhsCondp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string name() स्थिरांक override {
        string name = "s" + cvtToStr(color());
        जर (m_delayRingSize) {
            name += "\\n";
            name += m_isFixedDelayRing ? "fixed chain " : "range chain ";
            name += cvtToStr(m_delayRingSize) + " slots";
        }
        परत name;
    }
    string dotColor() स्थिरांक override {
        जर (m_isMatch) परत "red";
        जर (m_delayRingSize) परत "blue";
        जर (m_isAndCombiner) परत "purple";
        परत "black";
    }
    // LCOV_EXCL_STOP
    // Access per-vertex algorithm data (valid only during lowering phase)
    SvaVertexData* datap() स्थिरांक { परत static_cast<SvaVertexData*>(userp()); }
};

// NFA transition edge -- clocked (##1) or combinational link (##0)
वर्ग SvaTransEdge final : सार्वजनिक V3GraphEdge {
    VL_RTTI_IMPL(SvaTransEdge, V3GraphEdge)
सार्वजनिक:
    Ast_गाठ_अभिव्यक्ती* m_condp;  // Transition condition; nullptr = unconditional; OWNED
    तार्किक m_consumesCycle;  // true = clocked edge (##1), false = link (##0/boolean)
    // Reject when source is active and condp is false; set only on
    // outermost required-step Link
    तार्किक m_rejectOnFail = असत्य;
    // Optional dynamic condition vertex for m_rejectOnFail. Used when the
    // success condition is another NFA state rather than a static expression.
    SvaStateVertex* m_condVtxp = nullptr;

    // CONSTRUCTORS
    SvaTransEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष, Ast_गाठ_अभिव्यक्ती* condp,
                 तार्किक consumesCycle)
        : V3GraphEdge{graphp, fromp, शीर्ष, /*weight=*/1}
        , m_condp{condp}
        , m_consumesCycle{consumesCycle} {}
    ~SvaTransEdge() override {
        जर (m_condp) VL_DO_DANGLING(m_condp->delete_वृक्ष(), m_condp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string dotLabel() स्थिरांक override { परत m_consumesCycle ? "##1" : "link"; }
    string dotStyle() स्थिरांक override { परत m_consumesCycle ? "" : "dashed"; }
    // LCOV_EXCL_STOP
    // Typed accessors for NFA vertices
    SvaStateVertex* fromVtxp() स्थिरांक { परत static_cast<SvaStateVertex*>(fromp()); }
    SvaStateVertex* toVtxp() स्थिरांक { परत static_cast<SvaStateVertex*>(शीर्ष()); }
};

// NFA graph container
वर्ग SvaGraph final {
सार्वजनिक:
    V3Graph m_graph;  // Owns all vertices and edges
    SvaStateVertex* m_startVertexp = nullptr;  // Trigger/start vertex
    SvaStateVertex* m_matchVertexp = nullptr;  // Sequence-match terminal vertex

    // Create a new state vertex
    SvaStateVertex* createStateVertex() { परत नवीन SvaStateVertex{&m_graph}; }
    // Create the match terminal vertex
    SvaStateVertex* createMatchVertex() {
        SvaStateVertex* स्थिरांक vtxp = createStateVertex();
        vtxp->m_isMatch = सत्य;
        m_matchVertexp = vtxp;
        परत vtxp;
    }
    // Add a clocked transition edge (##1)
    SvaTransEdge* addClockedEdge(SvaStateVertex* fromp, SvaStateVertex* शीर्ष,
                                 Ast_गाठ_अभिव्यक्ती* condp = nullptr) {
        परत नवीन SvaTransEdge{&m_graph, fromp, शीर्ष, condp, /*consumesCycle=*/सत्य};
    }
    // Add a combinational link (##0 / boolean condition)
    SvaTransEdge* addLink(SvaStateVertex* fromp, SvaStateVertex* शीर्ष,
                          Ast_गाठ_अभिव्यक्ती* condp = nullptr) {
        परत नवीन SvaTransEdge{&m_graph, fromp, शीर्ष, condp, /*consumesCycle=*/असत्य};
    }
    // Collect all edges into a flat vector for iteration.
    // Used by the lowering phase which needs global edge scans.
    std::vector<स्थिरांक SvaTransEdge*> allEdges() स्थिरांक {
        std::vector<स्थिरांक SvaTransEdge*> result;
        साठी (स्थिरांक V3GraphVertex& vtxr : m_graph.vertices()) {
            साठी (स्थिरांक V3GraphEdge& edger : vtxr.outEdges()) {
                result.ढकला_back(static_cast<स्थिरांक SvaTransEdge*>(&edger));
            }
        }
        परत result;
    }
};

//######################################################################
// Builder result: terminal vertex + optional final condition (match Link condition).
संरचना BuildResult final {
    SvaStateVertex* termVertexp;  // Primary terminal; contributes to both match and reject
    Ast_गाठ_अभिव्यक्ती* finalCondp;  // nullptr = unconditional
    // Mid-window sources for range delays (pure boolean RHS): match-only (isUnbounded)
    std::vector<SvaStateVertex*> midSources;
    तार्किक errorEmitted = असत्य;  // Builder already emitted specific error; skip generic
    // For cover_sequence: when true, midSources already enumerate every
    // end-of-match, so wireMatchAndMidSources must NOT add the main
    // termVtxp -> matchVertex Link (would double-count via the merge vertex).
    तार्किक termIsMidMerge = असत्य;
    तार्किक valid() स्थिरांक { परत termVertexp != nullptr; }
    स्थिर BuildResult fail(तार्किक errored = असत्य) { परत {nullptr, nullptr, {}, errored}; }
    स्थिर BuildResult failWithError() { परत {nullptr, nullptr, {}, सत्य}; }
};

// Parser-marked SAnd of overlapped implications: a property if/else/case.
स्थिर तार्किक hasPropertyControlConjunction(स्थिरांक Ast_गाठ_अभिव्यक्ती* nodep) {
    परत nodep->exists([](स्थिरांक AstSAnd* andp) { परत andp->propertyControl(); });
}

स्थिर AstConst* newTypedConstp(File_ओळ* स्थिरांक flp, स्थिरांक Ast_गाठ_DType* स्थिरांक dtypep,
                                स्थिरांक uint32_t मूल्य) {
    AstConst* स्थिरांक constp = नवीन AstConst{flp, AstConst::DTyped{}, dtypep};
    constp->अंक().setLong(मूल्य);
    परत constp;
}

स्थिर Ast_गाठ_अभिव्यक्ती* sampled(Ast_गाठ_अभिव्यक्ती* exprp) {
    परत नवीन AstSampled{exprp->fileline(), exprp, exprp->dtypep(), सत्य};
}

स्थिर स्थिरांक अक्षर* assertPassOnQuery(तार्किक vacuous) {
    स्थिर constexpr स्थिरांक अक्षर* queries[2]
        = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
    परत queries[vacuous];
}

स्थिर Ast_गाठ_अभिव्यक्ती* assertOnCond(File_ओळ* flp, VAssertType type,
                                 VAssertDirectiveType directiveType) {
    जर (!v3Global.opt.assertOn()) { परत नवीन AstConst{flp, AstConst::BitFalse{}}; }
    परत नवीन AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
}

स्थिर Ast_गाठ_अभिव्यक्ती* assertKillGet(File_ओळ* flp, VAssertType type,
                                  VAssertDirectiveType directiveType) {
    परत नवीन AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_KILL", type, directiveType),
                        32};
}

स्थिर string assertActionControlPrefix(VAssertDirectiveType directiveType) {
    स्थिरांक पूर्णांक controlled = !!(static_cast<पूर्णांक>(directiveType)
                              & (static_cast<पूर्णांक>(VAssertDirectiveType::ASSERT)
                                 | static_cast<पूर्णांक>(VAssertDirectiveType::COVER)
                                 | static_cast<पूर्णांक>(VAssertDirectiveType::ASSUME)));
    स्थिरांक पूर्णांक checkRuntime = controlled & static_cast<पूर्णांक>(v3Global.opt.assertOn());
    परत "("s + std::to_string(controlled ^ 1) + " || ("s + std::to_string(checkRuntime)
           + " && "s;
}

स्थिर Ast_गाठ_अभिव्यक्ती* assertPassOnCond(File_ओळ* flp, VAssertType type,
                                     VAssertDirectiveType directiveType, तार्किक vacuous) {
    परत नवीन AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType)
            + "))"s,
        1};
}

स्थिर Ast_गाठ_अभिव्यक्ती* assertFailOnCond(File_ओळ* flp, VAssertType type,
                                     VAssertDirectiveType directiveType) {
    परत नवीन AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType) + "))"s,
        1};
}

स्थिर AstIf* newPassOnIf(File_ओळ* flp, Ast_गाठ_अभिव्यक्ती* firep, Ast_गाठ* bodyp, VAssertType type,
                          VAssertDirectiveType directiveType, तार्किक vacuous) {
    Ast_गाठ_अभिव्यक्ती* स्थिरांक condp
        = नवीन AstLogAnd{flp, firep, assertPassOnCond(flp, type, directiveType, vacuous)};
    AstIf* स्थिरांक ifp = नवीन AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(सत्य);
    ifp->user1(सत्य);
    परत ifp;
}

स्थिर Ast_गाठ_Stmt* newIfAssertFailOn(Ast_गाठ* bodyp, VAssertDirectiveType directiveType,
                                      VAssertType type) {
    File_ओळ* स्थिरांक flp = bodyp->fileline();
    Ast_गाठ_अभिव्यक्ती* स्थिरांक condp = assertFailOnCond(flp, type, directiveType);
    AstIf* स्थिरांक ifp = नवीन AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(सत्य);
    ifp->user1(सत्य);
    परत ifp;
}

//######################################################################
// NFA Builder

वर्ग SvaNfaBuilder final {
    SvaGraph& m_graph;  // NFA graph being built
    Ast_गाठ_Module* स्थिरांक m_modp;  // Module to receive hoisted sampled-prop temps
    V3UniqueNames& m_propTempNames;  // Module-shared temp-var name source
    std::vector<Ast_गाठ_अभिव्यक्ती*> m_temporal_Guard_चळत;  // Guards active across nested temporal states
    // Outer abort conditions, AND-ed as !cond into inner abort edges
    // (IEEE 1800-2023 16.12.14 outer-wraps-inner).
    std::vector<Ast_गाठ_अभिव्यक्ती*> m_outer_Abort_चळत;
    तार्किक m_inUnboundedScope = असत्य;  // Sticky: nodes created after inherit liveness
    तार्किक m_markStrongPending = असत्य;  // Mark new vertices as strong s_always in-window
    तार्किक m_isCover = असत्य;  // Cover directives do not fail at end-of-simulation
    // IEEE 1800-2023 16.14.3 cover sequence: each end-of-match fires the action,
    // not just the first. Builder builds parallel-branch (no first-match-wins)
    // topology when true. Default false preserves cover_property semantics.
    तार्किक m_isCoverSeq = असत्य;
    // Unsupported endpoint topology must reject, not ignore, or the wait hangs
    तार्किक m_isSeqEvent = असत्य;
    तार्किक m_inSequencePrefix = असत्य;  // Prefix composition does not preserve every endpoint
    तार्किक m_wantBoolLeaf = असत्य;  // Building a |-> / |=> or top-level body (see buildOrMerge)

    संरचना RangeDelayRejectInfo final {
        SvaStateVertex* startp = nullptr;
        अचिन्हित range = 0;
        पूर्णांक rhsLen = 0;
    };

    शून्य warnEndpointUnsupported(File_ओळ* flp, स्थिरांक string& what) स्थिरांक {
        जर (m_isSeqEvent) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: sequence used as an event control with " << what);
        } अन्यथा {
            flp->v3warn(COVERIGN, "Ignoring unsupported: cover sequence with " << what);
        }
    }

    Ast_गाठ_अभिव्यक्ती* throughoutCond(Ast_गाठ_अभिव्यक्ती* baseCondp, File_ओळ* flp) {
        जर (m_temporal_Guard_चळत.empty()) परत baseCondp;
        // AND all active temporal guards (supports nesting)
        // Each must use $sampled values.
        Ast_गाठ_अभिव्यक्ती* guardp = nullptr;
        साठी (Ast_गाठ_अभिव्यक्ती* स्थिरांक condp : m_temporal_Guard_चळत) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक clonep = sampled(condp->clone_वृक्ष_Pure(असत्य));
            जर (!guardp) {
                guardp = clonep;
            } अन्यथा {
                guardp = नवीन AstLogAnd{flp, guardp, clonep};
            }
        }
        जर (baseCondp) { guardp = नवीन AstLogAnd{flp, baseCondp, guardp}; }
        परत guardp;
    }

    स्थिर अचिन्हित getConstUInt(Ast_गाठ_अभिव्यक्ती* exprp) {
        Ast_गाठ_अभिव्यक्ती* स्थिरांक constp = V3Const::constifyEdit(exprp->clone_वृक्ष_Pure(असत्य));
        स्थिरांक AstConst* स्थिरांक cp = VN_CAST(constp, Const);
        स्थिरांक अचिन्हित val = cp ? cp->toUInt() : 0;
        VL_DO_DANGLING(constp->delete_वृक्ष(), constp);
        परत val;
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
    स्थिर पूर्णांक fixedLength(Ast_गाठ_अभिव्यक्ती* nodep) {
        जर (AstSExpr* स्थिरांक sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* स्थिरांक delayp = VN_CAST(sexprp->delayp(), Delay);
            जर (!delayp || !delayp->isCycleDelay()) परत -1;
            अचिन्हित delayCycles;
            जर (delayp->isRangeDelay()) {
                जर (delayp->isUnbounded()) परत -1;  // LCOV_EXCL_LINE
                स्थिरांक अचिन्हित minD = getConstUInt(delayp->lhsp());
                स्थिरांक अचिन्हित maxD = getConstUInt(delayp->rhsp());
                जर (minD != maxD) परत -1;
                delayCycles = minD;
            } अन्यथा {
                delayCycles = getConstUInt(delayp->lhsp());
            }
            पूर्णांक preLen = 0;
            जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक prep = sexprp->preExprp()) {
                preLen = fixedLength(prep);
                जर (preLen < 0) परत -1;  // LCOV_EXCL_LINE
            }
            स्थिरांक पूर्णांक bodyLen = fixedLength(sexprp->exprp());
            जर (bodyLen < 0) परत -1;  // LCOV_EXCL_LINE
            परत preLen + delayCycles + bodyLen;
        }
        जर (AstSThroughout* स्थिरांक throughp = VN_CAST(nodep, SThroughout)) {
            परत fixedLength(throughp->rhsp());
        }
        जर (AstSOr* स्थिरांक orp = VN_CAST(nodep, SOr)) {
            // Alternatives must share one end cycle; buildSWithin relies on
            // this to pair the OR with an SIntersect.
            स्थिरांक पूर्णांक lhsLen = fixedLength(orp->lhsp());
            स्थिरांक पूर्णांक rhsLen = fixedLength(orp->rhsp());
            जर (lhsLen < 0 || rhsLen < 0 || lhsLen != rhsLen) परत -1;
            परत lhsLen;
        }
        जर (AstSWithin* स्थिरांक withinp = VN_CAST(nodep, SWithin)) {
            // `seq1 within seq2` ends at seq2's end cycle (IEEE 16.9.10).
            स्थिरांक पूर्णांक lhsLen = fixedLength(withinp->lhsp());
            स्थिरांक पूर्णांक rhsLen = fixedLength(withinp->rhsp());
            जर (lhsLen < 0 || rhsLen < 0 || lhsLen > rhsLen) परत -1;
            परत rhsLen;
        }
        // LCOV_EXCL_START -- defensive: V3AssertPre rejects composite SVA ops
        // nested in an intersect arm before fixedLength runs (clock-context
        // resolution fails). Kept as a guard in case future parser relaxations
        // permit it.
        जर (nodep->isMultiCycleSva()) परत -1;
        // LCOV_EXCL_STOP
        // Plain boolean expression (no SVA constructs) -- 0 cycles.
        परत 0;
    }

    // Contiguous match-length range [lo,hi] for an operand whose length varies
    // from AT MOST ONE ranged cycle delay; {-1,-1} otherwise. Drives the
    // variable-length `intersect` lowering (IEEE 1800-2023 16.9.6): more than
    // one ranged delay would make the per-length realization ambiguous, so it
    // is reported unsupported rather than mis-paired.
    स्थिर std::pair<पूर्णांक, पूर्णांक> lengthRange(Ast_गाठ_अभिव्यक्ती* nodep) {
        जर (AstSExpr* स्थिरांक sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* स्थिरांक delayp = VN_CAST(sexprp->delayp(), Delay);
            जर (!delayp || !delayp->isCycleDelay()) परत {-1, -1};
            std::pair<पूर्णांक, पूर्णांक> delayRange;
            जर (delayp->isRangeDelay()) {
                जर (delayp->isUnbounded()) परत {-1, -1};
                स्थिरांक अचिन्हित minD = getConstUInt(delayp->lhsp());
                स्थिरांक अचिन्हित maxD = getConstUInt(delayp->rhsp());
                delayRange = {minD, maxD};
            } अन्यथा {
                स्थिरांक अचिन्हित d = getConstUInt(delayp->lhsp());
                delayRange = {d, d};
            }
            std::pair<पूर्णांक, पूर्णांक> preRange{0, 0};
            जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक prep = sexprp->preExprp()) {
                preRange = lengthRange(prep);
                जर (preRange.first < 0) परत {-1, -1};
            }
            स्थिरांक std::pair<पूर्णांक, पूर्णांक> bodyRange = lengthRange(sexprp->exprp());
            जर (bodyRange.first < 0) परत {-1, -1};
            स्थिरांक पूर्णांक variableParts = (preRange.first != preRange.second)
                                      + (delayRange.first != delayRange.second)
                                      + (bodyRange.first != bodyRange.second);
            जर (variableParts > 1) परत {-1, -1};
            परत {preRange.first + delayRange.first + bodyRange.first,
                    preRange.second + delayRange.second + bodyRange.second};
        }
        जर (AstSThroughout* स्थिरांक throughp = VN_CAST(nodep, SThroughout)) {
            परत lengthRange(throughp->rhsp());
        }
        जर (nodep->isMultiCycleSva()) परत {-1, -1};
        परत {0, 0};  // plain boolean -- 0 cycles
    }

    // Clone `operand` with its sole variable ranged cycle delay pinned so the
    // total match length is exactly `len`. `lo` is the operand's minimum length
    // (lengthRange().first). A fixed operand has no such delay and is returned
    // as a plain clone (callers only request its single achievable length).
    स्थिर Ast_गाठ_अभिव्यक्ती* realizeAtLength(Ast_गाठ_अभिव्यक्ती* operand, पूर्णांक len, पूर्णांक lo) {
        Ast_गाठ_अभिव्यक्ती* स्थिरांक clonep = operand->clone_वृक्ष_Pure(असत्य);
        AstDelay* rangeDelayp = nullptr;
        clonep->foreach([&](AstDelay* dp) {
            जर (!rangeDelayp && dp->isRangeDelay() && !dp->isUnbounded()
                && getConstUInt(dp->lhsp()) != getConstUInt(dp->rhsp())) {
                rangeDelayp = dp;
            }
        });
        जर (rangeDelayp) {
            File_ओळ* स्थिरांक flp = rangeDelayp->fileline();
            स्थिरांक अचिन्हित pinned = getConstUInt(rangeDelayp->lhsp()) + (len - lo);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक oldMinp = rangeDelayp->lhsp();
            oldMinp->replaceWith(नवीन AstConst{flp, pinned});
            VL_DO_DANGLING(oldMinp->delete_वृक्ष(), oldMinp);
            // Drop the max bound so it lowers as a fixed `##d`, not `##[d:d]`.
            Ast_गाठ* स्थिरांक oldMaxp = rangeDelayp->rhsp()->unlinkFrBack();
            VL_DO_DANGLING(oldMaxp->delete_वृक्ष(), oldMaxp);
        }
        परत clonep;
    }

    // Cuts AST size from O(N * sizeof(exprp)) to O(N) + O(sizeof(exprp)) by
    // sharing a single `VarRef` across N check edges. Hoist also matches the
    // IEEE 1800-2023 16.9.9 "single preponed-region snapshot" semantic for
    // any exprp -- even an impure one would now evaluate exactly once per
    // clock instead of N times. Orphan temps from failed builds are unused
    // MODULETEMPs and are removed by V3Dead.
    AstVar* tryHoistSampled(Ast_गाठ_अभिव्यक्ती* exprp, File_ओळ* flp, अचिन्हित clone_मोजणी) {
        constexpr अचिन्हित kHoistThreshold = 2;
        जर (clone_मोजणी < kHoistThreshold) परत nullptr;
        AstVar* स्थिरांक tempVarp
            = नवीन AstVar{flp, VVarType::MODULETEMP, m_propTempNames.get(exprp), exprp->dtypep()};
        m_modp->addStmtsp(tempVarp);
        AstAssign* स्थिरांक assignp = नवीन AstAssign{flp, नवीन AstVarRef{flp, tempVarp, VAccess::WRITE},
                                                 sampled(exprp->clone_वृक्ष_Pure(असत्य))};
        m_modp->addStmtsp(नवीन AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, assignp});
        परत tempVarp;
    }

    स्थिर Ast_गाठ_अभिव्यक्ती* sampledRefOrClone(AstVar* hoistVarp, Ast_गाठ_अभिव्यक्ती* exprp, File_ओळ* flp) {
        जर (hoistVarp) परत नवीन AstVarRef{flp, hoistVarp, VAccess::READ};
        परत sampled(exprp->clone_वृक्ष_Pure(असत्य));
    }

    // Create vertex and inherit temporal guards from the current scope.
    SvaStateVertex* scopedCreateVertex() {
        SvaStateVertex* स्थिरांक vtxp = m_graph.createStateVertex();
        साठी (Ast_गाठ_अभिव्यक्ती* स्थिरांक cp : m_temporal_Guard_चळत) {
            vtxp->m_throughoutConds.ढकला_back(cp->clone_वृक्ष_Pure(असत्य));
        }
        जर (m_inUnboundedScope) vtxp->m_isUnbounded = सत्य;
        जर (m_markStrongPending) vtxp->m_strongPending = सत्य;
        परत vtxp;
    }

    // AND current temporal guards into every edge/link.
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* शीर्ष, Ast_गाठ_अभिव्यक्ती* condp,
                              File_ओळ* flp) {
        परत m_graph.addLink(fromp, शीर्ष, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* शीर्ष, File_ओळ* flp) {
        परत m_graph.addLink(fromp, शीर्ष, throughoutCond(nullptr, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* शीर्ष, Ast_गाठ_अभिव्यक्ती* condp,
                              File_ओळ* flp) {
        परत m_graph.addClockedEdge(fromp, शीर्ष, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* शीर्ष, File_ओळ* flp) {
        परत m_graph.addClockedEdge(fromp, शीर्ष, throughoutCond(nullptr, flp));
    }

    SvaStateVertex* addDelayChain(SvaStateVertex* startp, अचिन्हित size, File_ओळ* flp,
                                  तार्किक isFixed = सत्य, Ast_गाठ_अभिव्यक्ती* clearCondp = nullptr,
                                  Ast_गाठ_अभिव्यक्ती* advanceCondp = nullptr) {
        जर (isFixed && size == 0) परत startp;
        UASSERT_OBJ(size > 0, startp, "Delay chain needs at least one slot");
        जर (isFixed && size == 1 && !advanceCondp) {
            SvaStateVertex* स्थिरांक nextp = scopedCreateVertex();
            guardedEdge(startp, nextp, flp);
            परत nextp;
        }
        SvaStateVertex* स्थिरांक ringVtxp = scopedCreateVertex();
        ringVtxp->m_isFixedDelayRing = isFixed;
        ringVtxp->m_delayRingSize = size;
        जर (clearCondp) {
            UASSERT_OBJ(!isFixed, startp, "Fixed delay cannot have a clear condition");
            ringVtxp->m_delayRingClearCondp = clearCondp->clone_वृक्ष_Pure(असत्य);
        }
        ringVtxp->m_delayRingAdvanceCondp = advanceCondp;
        जर (isFixed) {
            guardedEdge(startp, ringVtxp, flp);
        } अन्यथा {
            guardedLink(startp, ringVtxp, flp);
        }
        परत ringVtxp;
    }

    // Build NFA for an SExpr. finalCond = RHS (not yet added as a vertex).
    // isTopLevelStep: marks outermost required boolean check as rejectOnFail.
    // Apply a range delay `##[M:N]` to currentp. Returns true on success. On
    // failure, emits a diagnostic and returns false.
    तार्किक applyRangeDelay(AstSExpr* sexprp, SvaStateVertex*& currentp,
                         std::vector<SvaStateVertex*>& midSources,
                         RangeDelayRejectInfo* rangeRejectInfop = nullptr) {
        File_ओळ* स्थिरांक flp = sexprp->fileline();
        AstDelay* स्थिरांक delayp = VN_AS(sexprp->delayp(), Delay);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsExprp = sexprp->exprp();
        स्थिरांक अचिन्हित minDelay = getConstUInt(delayp->lhsp());
        जर (delayp->isUnbounded()) {
            // `##[M:$]`: wait M cycles, then retain every matured attempt in a
            // one-slot ring while also exposing the first eligible tick.
            currentp = addDelayChain(currentp, minDelay, flp);
            SvaStateVertex* स्थिरांक waitRingp = addDelayChain(currentp, 1, flp, असत्य);
            // The ring's live count is updated in NBA, so this link retains the previous
            // tick's live attempts alongside new arrivals for the next tick.
            guardedLink(waitRingp, waitRingp, flp);
            SvaStateVertex* स्थिरांक mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(waitRingp, mergeVtxp, flp);
            currentp = mergeVtxp;
            currentp->m_isUnbounded = सत्य;  // Liveness, so no reject.
            m_inUnboundedScope = सत्य;
            परत सत्य;
        }
        स्थिरांक अचिन्हित maxDelay = getConstUInt(delayp->rhsp());
        जर (minDelay == maxDelay) {
            currentp = addDelayChain(currentp, minDelay, flp);
            परत सत्य;
        }
        स्थिरांक अचिन्हित range = maxDelay - minDelay;
        currentp = addDelayChain(currentp, minDelay, flp);
        स्थिरांक तार्किक multiCycleRhs = rhsExprp->isMultiCycleSva();
        स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक preExprp = sexprp->preExprp();
        // Prefix and nested-sequence composition can lose endpoints or their multiplicity
        // required by cover sequence (IEEE 1800-2023 16.14.3), regardless of the range width.
        जर (m_isCoverSeq
            && (multiCycleRhs || m_inSequencePrefix
                || (preExprp && preExprp->isMultiCycleSva()))) {
            warnEndpointUnsupported(flp, "this ranged cycle delay");
            परत असत्य;
        }
        जर (m_inSequencePrefix) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: Bounded ranged cycle delay in a sequence prefix");
            परत असत्य;
        }
        जर (multiCycleRhs) {
            // Launch a candidate at every eligible tick. Candidate-local misses are not
            // assertion rejects while a later position can still match.
            जर (rangeRejectInfop) {
                स्थिरांक पूर्णांक rhsLen = fixedLength(rhsExprp);
                जर (rhsLen >= 0) *rangeRejectInfop = {currentp, range, rhsLen};
            }
            SvaStateVertex* स्थिरांक tailp = addDelayChain(currentp, range + 1U, flp, असत्य);
            SvaStateVertex* स्थिरांक mergeVtxp = scopedCreateVertex();
            mergeVtxp->m_isUnbounded = सत्य;
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(tailp, mergeVtxp, flp);
            currentp = mergeVtxp;
            m_inUnboundedScope = सत्य;
        } अन्यथा {
            // The first eligible tick is explicit. The ring retains attempts for the
            // remaining ticks and exposes the final tick for rejection. Only properties
            // clear on success, since cover sequence counts every endpoint.
            midSources.ढकला_back(currentp);
            currentp = addDelayChain(currentp, range + 1U, flp, असत्य,
                                     m_isCoverSeq ? nullptr : rhsExprp);
        }
        परत सत्य;
    }

    शून्य addFiniteRangeReject(स्थिरांक RangeDelayRejectInfo& info, स्थिरांक BuildResult& result,
                              File_ओळ* flp) {
        जर (!info.startp) परत;

        SvaStateVertex* स्थिरांक expiryVtxp
            = addDelayChain(info.startp, info.range + info.rhsLen, flp);
        SvaStateVertex* स्थिरांक expiryMatchp = scopedCreateVertex();
        std::vector<SvaStateVertex*> sources = result.midSources;
        sources.ढकला_back(result.termVertexp);
        साठी (SvaStateVertex* स्थिरांक srcp : sources) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक condp
                = result.finalCondp ? sampled(result.finalCondp->clone_वृक्ष_Pure(असत्य)) : nullptr;
            SvaStateVertex* स्थिरांक successNowp = scopedCreateVertex();
            guardedLink(srcp, successNowp, condp, flp);
            // Retain a success through the latest deadline it can satisfy.
            SvaStateVertex* स्थिरांक historyp
                = addDelayChain(successNowp, info.range + 1U, flp, असत्य);
            guardedLink(successNowp, expiryMatchp, flp);
            guardedLink(historyp, expiryMatchp, flp);
        }

        SvaStateVertex* स्थिरांक sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = सत्य;
        SvaTransEdge* स्थिरांक rejectp = m_graph.addLink(expiryVtxp, sinkVtxp);
        rejectp->m_rejectOnFail = सत्य;
        rejectp->m_condVtxp = expiryMatchp;
    }

    BuildResult buildSExpr(AstSExpr* sexprp, SvaStateVertex* entryVtxp,
                           तार्किक is_शीर्ष_Level_Step = असत्य) {
        AstDelay* स्थिरांक delayp = VN_CAST(sexprp->delayp(), Delay);
        जर (!delayp || !delayp->isCycleDelay()) परत BuildResult::fail();

        File_ओळ* स्थिरांक flp = sexprp->fileline();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = sexprp->exprp();

        // Handle LHS (preExpr)
        SvaStateVertex* currentp = entryVtxp;
        जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक preExprp = sexprp->preExprp()) {
            VL_RESTORER(m_inSequencePrefix);
            m_inSequencePrefix = सत्य;
            स्थिरांक BuildResult pre = build_अभिव्यक्ती(preExprp, currentp, is_शीर्ष_Level_Step);
            जर (!pre.valid()) परत BuildResult::fail(pre.errorEmitted);  // LCOV_EXCL_LINE
            जर (pre.finalCondp) {
                SvaStateVertex* स्थिरांक condVtxp = scopedCreateVertex();
                SvaTransEdge* स्थिरांक edgep = guardedLink(
                    pre.termVertexp, condVtxp, sampled(pre.finalCondp->clone_वृक्ष_Pure(असत्य)), flp);
                जर (is_शीर्ष_Level_Step && !pre.termVertexp->m_isUnbounded && !m_inUnboundedScope) {
                    // Do not mark liveness sources: first boolean check is deferred.
                    edgep->m_rejectOnFail = सत्य;
                }
                freeUnlinkedCondp(pre.finalCondp);
                currentp = condVtxp;
            } अन्यथा {
                currentp = pre.termVertexp;
            }
        }

        // Handle delay
        std::vector<SvaStateVertex*> rangeMidSources;
        RangeDelayRejectInfo rangeRejectInfo;
        स्थिरांक तार्किक addRangeReject = is_शीर्ष_Level_Step && !m_inUnboundedScope;
        जर (delayp->isRangeDelay()) {
            जर (!applyRangeDelay(sexprp, currentp, rangeMidSources,
                                 addRangeReject ? &rangeRejectInfo : nullptr)) {
                परत BuildResult::failWithError();
            }
        } अन्यथा {
            स्थिरांक अचिन्हित delayCycles = getConstUInt(delayp->lhsp());
            currentp = addDelayChain(currentp, delayCycles, flp);
        }

        // Multi-cycle RHS: recurse (only plain boolean is returned as finalCondp).
        जर (exprp->isMultiCycleSva()) {
            स्थिरांक BuildResult result = build_अभिव्यक्ती(exprp, currentp, is_शीर्ष_Level_Step);
            जर (result.valid()) addFiniteRangeReject(rangeRejectInfo, result, flp);
            परत result;
        }
        परत {currentp, exprp, std::move(rangeMidSources)};
    }

    BuildResult buildConsRep(AstSConsRep* repp, SvaStateVertex* entryVtxp,
                             तार्किक is_शीर्ष_Level_Step = असत्य) {
        File_ओळ* स्थिरांक flp = repp->fileline();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = repp->exprp();
        // Multi-cycle expr in ConsRep not yet supported; bail to avoid invalid AST.
        जर (exprp->isMultiCycleSva()) {
            repp->v3warn(E_UNSUPPORTED, "Unsupported: multi-cycle sequence expression inside"
                                        " consecutive repetition (IEEE 1800-2023 16.9.2)");
            परत BuildResult::failWithError();
        }
        स्थिरांक अचिन्हित minN = getConstUInt(repp->countp());

        // Sum sites across prefix + unbounded/range tail so one hoist covers
        // every check edge of this repetition.
        अचिन्हित totalSites = minN;
        जर (repp->unbounded()) {
            totalSites += 1;
        } अन्यथा जर (repp->maxCountp()) {
            totalSites += getConstUInt(repp->maxCountp()) - minN;
        }
        AstVar* स्थिरांक hoistVarp = tryHoistSampled(exprp, flp, totalSites);

        // Cover-sequence (IEEE 1800-2023 16.14.3): collect each end-of-match
        // position so they all fire the action, not just the merged terminal.
        std::vector<SvaStateVertex*> consMidSources;

        SvaStateVertex* currentp = entryVtxp;
        साठी (अचिन्हित i = 0; i < minN; ++i) {
            // Keep the first repetition explicit, collapse all remaining checks into the ring.
            जर (i == 1) {
                currentp = addDelayChain(currentp, minN - 1, flp);
                currentp->m_delayRingClearCondp
                    = नवीन AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                currentp->m_replayAbortReject = सत्य;
                जर (is_शीर्ष_Level_Step) {
                    currentp->m_throughoutConds.ढकला_back(
                        sampledRefOrClone(hoistVarp, exprp, flp));
                }
                i = minN - 1;
            }
            // Every repetition in the minimum prefix is required.
            SvaStateVertex* स्थिरांक condVtxp = scopedCreateVertex();
            SvaTransEdge* स्थिरांक linkp
                = guardedLink(currentp, condVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            // Only an outermost required step rejects the property. Its first check is explicit,
            // later required checks reject through the ring's throughout guard.
            जर (is_शीर्ष_Level_Step && i == 0) linkp->m_rejectOnFail = सत्य;
            currentp = condVtxp;
        }
        // After minN: currentp is the first valid end-of-match position for [*m:n].
        जर (m_isCoverSeq && (repp->unbounded() || repp->maxCountp())) {
            consMidSources.ढकला_back(currentp);
        }

        जर (repp->unbounded()) {
            जर (minN == 0) {
                SvaStateVertex* स्थिरांक waitVtxp = scopedCreateVertex();
                guardedEdge(currentp, waitVtxp, flp);
                SvaStateVertex* स्थिरांक checkVtxp = scopedCreateVertex();
                guardedLink(waitVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedEdge(checkVtxp, waitVtxp, flp);
                guardedLink(currentp, checkVtxp, flp);
                currentp = checkVtxp;
            } अन्यथा {
                SvaStateVertex* स्थिरांक loopBackVtxp = scopedCreateVertex();
                guardedEdge(currentp, loopBackVtxp, flp);
                SvaStateVertex* स्थिरांक reCheckVtxp = scopedCreateVertex();
                guardedLink(loopBackVtxp, reCheckVtxp, sampledRefOrClone(hoistVarp, exprp, flp),
                            flp);
                guardedEdge(reCheckVtxp, loopBackVtxp, flp);
                guardedLink(reCheckVtxp, currentp, flp);
            }
            currentp->m_isUnbounded = सत्य;
            m_inUnboundedScope = सत्य;
        } अन्यथा जर (repp->maxCountp()) {
            स्थिरांक अचिन्हित maxN = getConstUInt(repp->maxCountp());
            SvaStateVertex* स्थिरांक mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            अचिन्हित tailMinN = minN;
            SvaStateVertex* tailStartp = currentp;
            जर (minN == 0) {
                // Build the first optional iteration explicitly. Feeding the empty endpoint
                // directly into a range ring would incorrectly keep that match alive.
                SvaStateVertex* स्थिरांक nextVtxp = scopedCreateVertex();
                guardedEdge(currentp, nextVtxp, flp);
                SvaStateVertex* स्थिरांक checkVtxp = scopedCreateVertex();
                guardedLink(nextVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                जर (m_isCoverSeq) consMidSources.ढकला_back(checkVtxp);
                tailStartp = checkVtxp;
                tailMinN = 1;
            }
            जर (maxN > tailMinN) {
                // Add tail-ring only if the tail is non-empty.
                SvaStateVertex* स्थिरांक nextVtxp
                    = addDelayChain(tailStartp, maxN - tailMinN + 1, flp, असत्य);
                nextVtxp->m_delayRingClearCondp
                    = नवीन AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                nextVtxp->m_replayAbortReject = सत्य;
                SvaStateVertex* स्थिरांक checkVtxp = scopedCreateVertex();
                guardedLink(nextVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                जर (m_isCoverSeq) consMidSources.ढकला_back(checkVtxp);
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
        परत res;
    }

    // always[lo:hi] / s_always[lo:hi] (IEEE 1800-2023 16.12.11).
    BuildResult buildPropAlways(AstPropAlways* nodep, SvaStateVertex* entryVtxp,
                                तार्किक is_शीर्ष_Level_Step = असत्य) {
        File_ओळ* स्थिरांक flp = nodep->fileline();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक propp = nodep->propp();
        स्थिरांक अचिन्हित lo = getConstUInt(nodep->loBoundp());
        जर (VN_IS(nodep->hiBoundp(), Unbounded)) {
            // Weak always [lo:$]: unbounded upper bound (IEEE 1800-2023 16.12.11).
            // p must hold at every clock tick at least lo cycles after the attempt
            // start; those ticks are not required to exist, so there is no
            // end-of-trace obligation (weak). The self-loop keeps the attempt live
            // every cycle; each observed cycle is a safety obligation, so a false p
            // rejects immediately.
            UASSERT_OBJ(!nodep->isStrong(), nodep, "Unbounded always must be weak (V3Width)");
            SvaStateVertex* स्थिरांक livep = addDelayChain(entryVtxp, lo, flp);
            livep->m_isUnbounded = सत्य;
            guardedEdge(livep, livep, flp);  // stay active every subsequent cycle
            SvaStateVertex* स्थिरांक sinkp = m_graph.createStateVertex();
            sinkp->m_isRejectSink = सत्य;
            SvaTransEdge* स्थिरांक rejEdgep
                = guardedLink(livep, sinkp, sampled(propp->clone_वृक्ष_Pure(असत्य)), flp);
            जर (is_शीर्ष_Level_Step) rejEdgep->m_rejectOnFail = सत्य;
            परत {livep, nullptr, {}};
        }
        स्थिरांक अचिन्हित hi = getConstUInt(nodep->hiBoundp());
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
        SvaStateVertex* स्थिरांक checkp = scopedCreateVertex();
        SvaTransEdge* स्थिरांक linkp
            = guardedLink(currentp, checkp, sampled(propp->clone_वृक्ष_Pure(असत्य)), flp);
        जर (is_शीर्ष_Level_Step) linkp->m_rejectOnFail = सत्य;
        currentp = checkp;
        m_temporal_Guard_चळत.ढकला_back(propp);
        currentp = addDelayChain(currentp, hi - lo, flp);
        जर (nodep->isStrong() && currentp->m_delayRingSize) currentp->m_strongAlwaysRing = सत्य;
        m_temporal_Guard_चळत.काढा_back();
        परत {currentp, propp, {}};
    }

    BuildResult buildGotoRep(AstSGotoRep* repp, SvaStateVertex* entryVtxp) {
        File_ओळ* स्थिरांक flp = repp->fileline();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = repp->exprp();
        स्थिरांक अचिन्हित minN = getConstUInt(repp->countp());
        जर (minN == 0) परत BuildResult::fail();
        स्थिरांक तार्किक hasMax = repp->maxCountp() != nullptr;
        स्थिरांक अचिन्हित maxN = hasMax ? getConstUInt(repp->maxCountp()) : minN;
        जर (m_isCoverSeq) {
            // Several attempts may coalesce in the one-bit wait vertex before reaching the ring,
            // so a cover sequence action block could run too few times.
            warnEndpointUnsupported(flp, "a goto repetition");
            परत BuildResult::failWithError();
        }

        AstVar* स्थिरांक hoistVarp = tryHoistSampled(exprp, flp, 2);

        // The first guardedEdge is the ##1 before waiting for a match. In the wait state, false
        // takes the clocked self-loop, while true takes the zero-delay guardedLink on that tick.
        SvaStateVertex* स्थिरांक waitVtxp = scopedCreateVertex();
        guardedEdge(entryVtxp, waitVtxp, flp);
        guardedEdge(waitVtxp, waitVtxp,
                    नवीन AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)}, flp);
        SvaStateVertex* currentp = scopedCreateVertex();
        guardedLink(waitVtxp, currentp, sampledRefOrClone(hoistVarp, exprp, flp), flp);

        जर (minN > 1) {
            SvaStateVertex* स्थिरांक ringVtxp = addDelayChain(
                currentp, minN - 1, flp, सत्य, nullptr, sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = सत्य;
            SvaStateVertex* स्थिरांक checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            currentp = checkVtxp;
        }
        जर (!hasMax) {
            currentp->m_isUnbounded = सत्य;  // [->N] waits unboundedly
            m_inUnboundedScope = सत्य;
            परत {currentp, nullptr, {}};
        }

        // [->M:N]: the range ring holds matches from M through N and advances
        // only on expr, preserving arbitrarily long gaps between occurrences.
        SvaStateVertex* स्थिरांक mergeVtxp = scopedCreateVertex();
        guardedLink(currentp, mergeVtxp, flp);  // accept at match_M
        जर (maxN > minN) {
            SvaStateVertex* स्थिरांक ringVtxp
                = addDelayChain(currentp, maxN - minN + 1, flp, असत्य, nullptr,
                                sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = सत्य;
            SvaStateVertex* स्थिरांक checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            guardedLink(checkVtxp, mergeVtxp, flp);
        }
        mergeVtxp->m_isUnbounded = सत्य;  // [->M:N] still has unbounded waits between matches
        m_inUnboundedScope = सत्य;
        परत {mergeVtxp, nullptr, {}};
    }

    // True if the sequence has a zero-minimum consecutive repetition, which may match empty
    स्थिर तार्किक hasZeroMinRep(स्थिरांक Ast_गाठ_अभिव्यक्ती* seqp) {
        परत seqp->exists(
            [](स्थिरांक AstSConsRep* repp) { परत getConstUInt(repp->countp()) == 0; });
    }

    // True if a leaf's condition has no implication or multi-cycle sequence in it
    // (a leaf can be e.g. 'not (x |-> y)')
    स्थिर तार्किक isPlainBoolean(स्थिरांक Ast_गाठ_अभिव्यक्ती* condp) {
        परत !condp->exists(
            [](स्थिरांक Ast_गाठ_अभिव्यक्ती* np) { परत np->isMultiCycleSva() || VN_IS(np, Implication); });
    }

    // Build merge vertex for SOr / LogOr: both branches feed into one vertex.
    BuildResult buildOrMerge(Ast_गाठ_अभिव्यक्ती* lhsp, Ast_गाठ_अभिव्यक्ती* rhsp, SvaStateVertex* entryVtxp,
                             File_ओळ* flp, तार्किक wantBoolLeaf) {
        स्थिरांक BuildResult lhs = build_अभिव्यक्ती(lhsp, entryVtxp);
        स्थिरांक BuildResult rhs = build_अभिव्यक्ती(rhsp, entryVtxp);
        जर (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            परत BuildResult::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP
        // IEEE 1800-2023 16.14.3: a cover sequence counts every end-of-match. A
        // sequence operand of 'or' can end more than once, but only its final
        // end reaches the merge vertex below, so reject sequence operands rather
        // than under-count. Plain boolean disjunction has one end per cycle and
        // is handled by the OR-fold.
        स्थिरांक तार्किक booleanOnly = lhs.termVertexp == entryVtxp && lhs.finalCondp
                                 && rhs.termVertexp == entryVtxp && rhs.finalCondp;
        जर (m_isCoverSeq && !booleanOnly) {
            warnEndpointUnsupported(flp, "a sequence operand of 'or'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            परत BuildResult::failWithError();
        }
        जर (booleanOnly && wantBoolLeaf) {
            जर (isPlainBoolean(lhs.finalCondp) && isPlainBoolean(rhs.finalCondp)) {
                // As a |-> / |=> or top-level body, a disjunction of two plain booleans must
                // be able to reject: return it as a boolean finalCondp. The merge vertex
                // below drops the attempt silently when both operands are false.
                Ast_गाठ_अभिव्यक्ती* स्थिरांक orCondp
                    = नवीन AstLogOr{flp, lhs.finalCondp->clone_वृक्ष_Pure(असत्य),
                                   rhs.finalCondp->clone_वृक्ष_Pure(असत्य)};
                freeUnlinkedCondp(lhs.finalCondp);
                freeUnlinkedCondp(rhs.finalCondp);
                परत {entryVtxp, orCondp, {}};
            }
        }
        SvaStateVertex* स्थिरांक mergeVtxp = scopedCreateVertex();
        जर (booleanOnly) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsp,
                        "Single-cycle SOr operands must have finalCondp");
            guardedLink(entryVtxp, mergeVtxp,
                        नवीन AstLogOr{flp, sampled(lhs.finalCondp->clone_वृक्ष_Pure(असत्य)),
                                     sampled(rhs.finalCondp->clone_वृक्ष_Pure(असत्य))},
                        flp);
        } अन्यथा {
            जर (lhs.finalCondp) {
                guardedLink(lhs.termVertexp, mergeVtxp,
                            sampled(lhs.finalCondp->clone_वृक्ष_Pure(असत्य)), flp);
            } अन्यथा {
                guardedLink(lhs.termVertexp, mergeVtxp, flp);
            }
            जर (rhs.finalCondp) {
                guardedLink(rhs.termVertexp, mergeVtxp,
                            sampled(rhs.finalCondp->clone_वृक्ष_Pure(असत्य)), flp);
            } अन्यथा {
                guardedLink(rhs.termVertexp, mergeVtxp, flp);
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        परत {mergeVtxp, nullptr, {}};
    }

    // Build done-latch combiner for SAnd/SIntersect (IEEE 1800-2023 16.9.5).
    BuildResult buildAndCombiner(Ast_गाठ_अभिव्यक्ती* lhsExprp, Ast_गाठ_अभिव्यक्ती* rhsExprp,
                                 SvaStateVertex* entryVtxp, File_ओळ* flp) {
        // Snapshot-restore scope so LHS liveness does not leak into RHS.
        स्थिरांक तार्किक savedScope = m_inUnboundedScope;
        स्थिरांक BuildResult lhs = build_अभिव्यक्ती(lhsExprp, entryVtxp);
        स्थिरांक तार्किक lhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope;
        स्थिरांक BuildResult rhs = build_अभिव्यक्ती(rhsExprp, entryVtxp);
        स्थिरांक तार्किक rhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope || lhsScope || rhsScope;
        जर (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            परत BuildResult::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP

        // Single-cycle operands: use boolean AND (done-latch would fire across cycles).
        // If both operands stayed at entry, they must be boolean leaves which
        // buildExpr returns with finalCondp=nodep (non-null).
        जर (lhs.termVertexp == entryVtxp && rhs.termVertexp == entryVtxp) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsExprp,
                        "Single-cycle SAnd operands must have finalCondp");
            Ast_गाठ_अभिव्यक्ती* स्थिरांक condp = नवीन AstLogAnd{flp, lhs.finalCondp->clone_वृक्ष_Pure(असत्य),
                                                     rhs.finalCondp->clone_वृक्ष_Pure(असत्य)};
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            परत {entryVtxp, condp, {}};
        }
        // Range-delay mid-window sources in either sub-branch would need
        // to be folded into the latch's match-now signal, which the
        // current combiner does not support.
        जर (!lhs.midSources.empty() || !rhs.midSources.empty()) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: ranged cycle delay in an operand of property 'and'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            परत BuildResult::failWithError();
        }
        SvaStateVertex* स्थिरांक combVtxp = scopedCreateVertex();
        combVtxp->m_isAndCombiner = सत्य;
        combVtxp->m_andLhsTermp = lhs.termVertexp;
        combVtxp->m_andRhsTermp = rhs.termVertexp;
        जर (lhs.finalCondp) combVtxp->m_andLhsCondp = lhs.finalCondp->clone_वृक्ष_Pure(असत्य);
        जर (rhs.finalCondp) combVtxp->m_andRhsCondp = rhs.finalCondp->clone_वृक्ष_Pure(असत्य);
        जर (lhs.termVertexp->m_isUnbounded || rhs.termVertexp->m_isUnbounded) {
            combVtxp->m_isUnbounded = सत्य;
        }
        // Wire terminal-boolean rejects to a dedicated sink so each side can fail
        // the AND independently. Skip for liveness or single-cycle operands
        // (single-cycle termVertexp == entryVtxp would fire every cycle).
        जर (!combVtxp->m_isUnbounded) {
            तार्किक needSink = असत्य;
            स्थिरांक तार्किक lhsMultiCycle = (lhs.termVertexp != entryVtxp);
            स्थिरांक तार्किक rhsMultiCycle = (rhs.termVertexp != entryVtxp);
            जर (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                needSink = सत्य;
            }
            जर (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                needSink = सत्य;
            }
            जर (needSink) {
                SvaStateVertex* स्थिरांक sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = सत्य;
                जर (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* स्थिरांक ep = m_graph.addLink(
                        lhs.termVertexp, sinkVtxp, sampled(lhs.finalCondp->clone_वृक्ष_Pure(असत्य)));
                    ep->m_rejectOnFail = सत्य;
                }
                जर (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* स्थिरांक ep = m_graph.addLink(
                        rhs.termVertexp, sinkVtxp, sampled(rhs.finalCondp->clone_वृक्ष_Pure(असत्य)));
                    ep->m_rejectOnFail = सत्य;
                }
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        परत {combVtxp, nullptr, {}};
    }

    // Lower `seq1 within seq2` (IEEE 1800-2023 16.9.10) as:
    //   (OR_{i in 0..slack} 1 ##i seq1 ##(slack-i) 1) intersect seq2
    // Both operands must have fixed length (no ranged cycle delays).
    // The OR lives inside a single SIntersect so one AndCombiner done-latch
    // serves all offsets; lifting it out would double-count matches where
    // multiple offsets accept on the same seq2 end cycle.
    BuildResult buildSWithin(AstSWithin* nodep, SvaStateVertex* entryVtxp,
                             तार्किक is_शीर्ष_Level_Step = असत्य) {
        स्थिरांक पूर्णांक innerLen = fixedLength(nodep->lhsp());
        स्थिरांक पूर्णांक outerLen = fixedLength(nodep->rhsp());
        जर (innerLen < 0 || outerLen < 0) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: within with ranged cycle-delay operand");
            परत BuildResult::failWithError();
        }
        जर (innerLen > outerLen) {
            परत buildNeverMatchIntersect(
                nodep, entryVtxp, is_शीर्ष_Level_Step,
                "the inner sequence is longer than the outer sequence");
        }
        File_ओळ* स्थिरांक flp = nodep->fileline();
        स्थिरांक पूर्णांक slack = outerLen - innerLen;
        Ast_गाठ_अभिव्यक्ती* innerOrp = nullptr;
        साठी (पूर्णांक i = 0; i <= slack; ++i) {
            स्थिरांक पूर्णांक postPad = slack - i;
            Ast_गाठ_अभिव्यक्ती* branchp = nodep->lhsp()->clone_वृक्ष_Pure(असत्य);
            जर (i > 0) {
                AstConst* स्थिरांक prePadp = नवीन AstConst{flp, AstConst::BitTrue{}};
                AstDelay* स्थिरांक delayp
                    = नवीन AstDelay{flp, नवीन AstConst{flp, static_cast<uint32_t>(i)}, सत्य};
                AstSExpr* स्थिरांक wrapped = नवीन AstSExpr{flp, prePadp, delayp, branchp};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            जर (postPad > 0) {
                AstConst* स्थिरांक postTruep = नवीन AstConst{flp, AstConst::BitTrue{}};
                AstDelay* स्थिरांक delayp
                    = नवीन AstDelay{flp, नवीन AstConst{flp, static_cast<uint32_t>(postPad)}, सत्य};
                AstSExpr* स्थिरांक wrapped = नवीन AstSExpr{flp, branchp, delayp, postTruep};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            innerOrp = innerOrp ? static_cast<Ast_गाठ_अभिव्यक्ती*>(नवीन AstSOr{flp, innerOrp, branchp})
                                : branchp;
        }
        Ast_गाठ_अभिव्यक्ती* स्थिरांक outerClonep = nodep->rhsp()->clone_वृक्ष_Pure(असत्य);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक combinedp = नवीन AstSIntersect{flp, innerOrp, outerClonep};
        BuildResult result = build_अभिव्यक्ती(combinedp, entryVtxp, is_शीर्ष_Level_Step);
        VL_DO_DANGLING(combinedp->delete_वृक्ष(), combinedp);
        // When both operands are plain booleans, buildAndCombiner returns a
        // freshly-allocated AstAnd as finalCondp with no parent. Callers up
        // the chain clone-and-discard finalCondp, so leave it parent-less and
        // it leaks; anchor it in the graph now via an edge.
        जर (result.valid() && result.finalCondp && !result.finalCondp->backp()) {
            SvaStateVertex* स्थिरांक wrapVtxp = scopedCreateVertex();
            guardedLink(result.termVertexp, wrapVtxp, sampled(result.finalCondp), flp);
            result = {wrapVtxp, nullptr, result.midSources};
        }
        परत result;
    }

    // Collect the boolean leaf checks of a fixed-length sequence keyed by their
    // clock offset from the start. Returns false for anything other than nested
    // AstSExpr with fixed cycle delays over boolean leaves (e.g. throughout).
    स्थिर तार्किक flattenFixedSeq(Ast_गाठ_अभिव्यक्ती* nodep, पूर्णांक baseOffset,
                                std::map<पूर्णांक, std::vector<Ast_गाठ_अभिव्यक्ती*>>& out) {
        जर (AstSExpr* स्थिरांक sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* स्थिरांक delayp = VN_CAST(sexprp->delayp(), Delay);
            जर (!delayp || !delayp->isCycleDelay() || delayp->isUnbounded()) परत असत्य;
            स्थिरांक अचिन्हित delayCycles = getConstUInt(delayp->lhsp());
            जर (delayp->isRangeDelay() && getConstUInt(delayp->rhsp()) != delayCycles)
                परत असत्य;
            पूर्णांक preLen = 0;
            जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक prep = sexprp->preExprp()) {
                जर (!flattenFixedSeq(prep, baseOffset, out)) परत असत्य;
                preLen = fixedLength(prep);
                जर (preLen < 0) परत असत्य;
            }
            परत flattenFixedSeq(sexprp->exprp(), baseOffset + preLen + delayCycles, out);
        }
        जर (nodep->isMultiCycleSva()) परत असत्य;
        out[baseOffset].ढकला_back(nodep);
        परत सत्य;
    }

    // Conjoin two equal-length fixed sequences into one: at each clock offset
    // AND the boolean checks of both operands (IEEE 1800-2023 16.9.6 -- both
    // operands match the same window). Returns null if either operand is not a
    // plain fixed sequence of boolean leaves.
    स्थिर Ast_गाठ_अभिव्यक्ती* conjoinFixedSeqs(Ast_गाठ_अभिव्यक्ती* lhsp, Ast_गाठ_अभिव्यक्ती* rhsp, File_ओळ* flp) {
        std::map<पूर्णांक, std::vector<Ast_गाठ_अभिव्यक्ती*>> checks;
        जर (!flattenFixedSeq(lhsp, 0, checks) || !flattenFixedSeq(rhsp, 0, checks)) परत nullptr;
        जर (checks.empty()) परत nullptr;
        Ast_गाठ_अभिव्यक्ती* resultp = nullptr;
        पूर्णांक prevOffset = 0;
        साठी (स्थिरांक स्वतः& offsetChecks : checks) {
            स्थिरांक पूर्णांक offset = offsetChecks.first;
            Ast_गाठ_अभिव्यक्ती* condp = nullptr;
            साठी (Ast_गाठ_अभिव्यक्ती* स्थिरांक leafp : offsetChecks.second) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक clonep = leafp->clone_वृक्ष_Pure(असत्य);
                जर (!condp) {
                    condp = clonep;
                } अन्यथा {
                    condp = नवीन AstLogAnd{flp, condp, clonep};
                    condp->dtypeSetBit();
                }
            }
            जर (!resultp) {
                जर (offset > 0) {
                    AstDelay* स्थिरांक delayp = नवीन AstDelay{
                        flp, नवीन AstConst{flp, static_cast<uint32_t>(offset)}, /*isCycle=*/सत्य};
                    resultp
                        = नवीन AstSExpr{flp, नवीन AstConst{flp, AstConst::BitTrue{}}, delayp, condp};
                    resultp->dtypeSetBit();
                } अन्यथा {
                    resultp = condp;
                }
            } अन्यथा {
                AstDelay* स्थिरांक delayp = नवीन AstDelay{
                    flp, नवीन AstConst{flp, static_cast<uint32_t>(offset - prevOffset)},
                    /*isCycle=*/सत्य};
                resultp = नवीन AstSExpr{flp, resultp, delayp, condp};
                resultp->dtypeSetBit();
            }
            prevOffset = offset;
        }
        परत resultp;
    }

    // `seq` is a simple ranged sequence `start ##[m:n] end` (start/end boolean,
    // start may be absent). Used to collapse a both-variable intersect to one
    // ranged delay.
    संरचना SimpleRanged final {
        तार्किक ok = असत्य;
        Ast_गाठ_अभिव्यक्ती* startp = nullptr;  // may be null (absent start)
        Ast_गाठ_अभिव्यक्ती* endp = nullptr;
    };
    स्थिर SimpleRanged asSimpleRanged(Ast_गाठ_अभिव्यक्ती* nodep) {
        AstSExpr* स्थिरांक sexprp = VN_CAST(nodep, SExpr);
        जर (!sexprp) परत {};
        AstDelay* स्थिरांक delayp = VN_CAST(sexprp->delayp(), Delay);
        जर (!delayp || !delayp->isCycleDelay() || !delayp->isRangeDelay() || delayp->isUnbounded())
            परत {};
        जर (getConstUInt(delayp->lhsp()) == getConstUInt(delayp->rhsp())) परत {};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक prep = sexprp->preExprp();
        जर (prep && fixedLength(prep) != 0) परत {};
        जर (fixedLength(sexprp->exprp()) != 0) परत {};
        परत {सत्य, prep, sexprp->exprp()};
    }

    // Build the NFA for a synthesized intersect lowering tree, then free it.
    // buildExpr returns the terminal condition (finalCondp) by reference into the
    // tree; detach a clone so the tree can be freed here. The graph already holds
    // clones/hoists of every edge condition, so nothing else dangles.
    BuildResult build_From_Lowering_वृक्ष(Ast_गाठ_अभिव्यक्ती* treep, SvaStateVertex* entryVtxp,
                                      तार्किक is_शीर्ष_Level_Step) {
        BuildResult result = build_अभिव्यक्ती(treep, entryVtxp, is_शीर्ष_Level_Step);
        जर (result.valid() && result.finalCondp) {
            result.finalCondp = result.finalCondp->clone_वृक्ष_Pure(असत्य);
        }
        VL_DO_DANGLING(treep->delete_वृक्ष(), treep);
        परत result;
    }

    // Empty common-length intersection -- unequal fixed lengths, or disjoint
    // ranged lengths. IEEE 1800-2023 16.9.6 requires both operands to match
    // over a window of the same length, so with no common length the intersect
    // simply never matches. This is legal (matching nothing), not an error, so
    // lower to a constant false rather than rejecting legal code.
    BuildResult buildNeverMatchIntersect(Ast_गाठ_अभिव्यक्ती* nodep, SvaStateVertex* entryVtxp,
                                         तार्किक is_शीर्ष_Level_Step, स्थिरांक अक्षर* reason) {
        nodep->v3warn(NEVERMATCH, "Sequence can never match because " << reason << ".");
        Ast_गाठ_अभिव्यक्ती* स्थिरांक falsep = नवीन AstConst{nodep->fileline(), AstConst::BitFalse{}};
        परत build_From_Lowering_वृक्ष(falsep, entryVtxp, is_शीर्ष_Level_Step);
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
                                     तार्किक is_शीर्ष_Level_Step) {
        स्थिरांक std::pair<पूर्णांक, पूर्णांक> lhsRange = lengthRange(nodep->lhsp());
        स्थिरांक std::pair<पूर्णांक, पूर्णांक> rhsRange = lengthRange(nodep->rhsp());
        जर (lhsRange.first < 0 || rhsRange.first < 0) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect with this variable-length operand");
            परत BuildResult::failWithError();
        }
        स्थिरांक पूर्णांक lo = std::max(lhsRange.first, rhsRange.first);
        स्थिरांक पूर्णांक hi = std::min(lhsRange.second, rhsRange.second);
        जर (lo > hi) {
            // Disjoint length ranges share no common length -> never matches.
            परत buildNeverMatchIntersect(nodep, entryVtxp, is_शीर्ष_Level_Step,
                                            "intersect operands have no common length");
        }
        File_ओळ* स्थिरांक flp = nodep->fileline();
        जर (lo == hi) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक lp = realizeAtLength(nodep->lhsp(), lo, lhsRange.first);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rp = realizeAtLength(nodep->rhsp(), lo, rhsRange.first);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक conjp = conjoinFixedSeqs(lp, rp, flp);
            VL_DO_DANGLING(lp->delete_वृक्ष(), lp);
            VL_DO_DANGLING(rp->delete_वृक्ष(), rp);
            जर (!conjp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: intersect operand is not a plain boolean sequence");
                परत BuildResult::failWithError();
            }
            परत build_From_Lowering_वृक्ष(conjp, entryVtxp, is_शीर्ष_Level_Step);
        }
        स्थिरांक SimpleRanged sl = asSimpleRanged(nodep->lhsp());
        स्थिरांक SimpleRanged sr = asSimpleRanged(nodep->rhsp());
        जर (!sl.ok || !sr.ok) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect of two sequences that each vary in length over a"
                          " range with internal structure");
            परत BuildResult::failWithError();
        }
        स्थिरांक स्वतः andBool = [&](Ast_गाठ_अभिव्यक्ती* ap, Ast_गाठ_अभिव्यक्ती* bp) -> Ast_गाठ_अभिव्यक्ती* {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक aClonep
                = ap ? ap->clone_वृक्ष_Pure(असत्य) : नवीन AstConst{flp, AstConst::BitTrue{}};
            Ast_गाठ_अभिव्यक्ती* स्थिरांक bClonep
                = bp ? bp->clone_वृक्ष_Pure(असत्य) : नवीन AstConst{flp, AstConst::BitTrue{}};
            AstLogAnd* स्थिरांक andp = नवीन AstLogAnd{flp, aClonep, bClonep};
            andp->dtypeSetBit();
            परत andp;
        };
        AstDelay* स्थिरांक delayp = नवीन AstDelay{flp, नवीन AstConst{flp, static_cast<uint32_t>(lo)},
                                              /*isCycle=*/सत्य};
        delayp->rhsp(नवीन AstConst{flp, static_cast<uint32_t>(hi)});
        AstSExpr* स्थिरांक reducedp
            = नवीन AstSExpr{flp, andBool(sl.startp, sr.startp), delayp, andBool(sl.endp, sr.endp)};
        reducedp->dtypeSetBit();
        परत build_From_Lowering_वृक्ष(reducedp, entryVtxp, is_शीर्ष_Level_Step);
    }

    BuildResult buildThroughout(AstSThroughout* nodep, SvaStateVertex* entryVtxp,
                                तार्किक is_शीर्ष_Level_Step = असत्य) {
        // Mark entryVtxp so "cond false at tick 0" is detected as throughout-drop.
        entryVtxp->m_throughoutConds.ढकला_back(nodep->lhsp()->clone_वृक्ष_Pure(असत्य));
        m_temporal_Guard_चळत.ढकला_back(nodep->lhsp());
        स्थिरांक BuildResult result = build_अभिव्यक्ती(nodep->rhsp(), entryVtxp, is_शीर्ष_Level_Step);
        m_temporal_Guard_चळत.काढा_back();
        परत result;
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
    BuildResult buildUntil(AstUntil* nodep, SvaStateVertex* entryVtxp, तार्किक is_शीर्ष_Level_Step) {
        File_ओळ* स्थिरांक flp = nodep->fileline();
        जर (!is_शीर्ष_Level_Step) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            परत BuildResult::failWithError();
        }
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = nodep->lhsp();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp();
        स्थिरांक स्वतः hasSeq = [](स्थिरांक Ast_गाठ_अभिव्यक्ती* ep) {
            परत ep->exists([](स्थिरांक Ast_गाठ_अभिव्यक्ती* np) { परत np->isMultiCycleSva(); });
        };
        जर (hasSeq(lhsp) || hasSeq(rhsp)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            परत BuildResult::failWithError();
        }

        स्थिरांक तार्किक ov = nodep->isOverlapping();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsBitp = nodep->lhsp();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsBitp = nodep->rhsp();
        // p hoist count: continue, require (ov: 1 use; nov: 1 use). At least 2 uses.
        AstVar* स्थिरांक pHoistp = tryHoistSampled(lhsBitp, flp, 2);
        // q hoist count: continue (1) + require nov (1) = 2; ov: continue only (1).
        AstVar* स्थिरांक qHoistp = ov ? nullptr : tryHoistSampled(rhsBitp, flp, 2);

        SvaStateVertex* स्थिरांक waitCp = scopedCreateVertex();
        SvaStateVertex* स्थिरांक waitRp = scopedCreateVertex();
        waitCp->m_isUnbounded = सत्य;
        waitRp->m_strongPending = nodep->isStrong() && !m_isCover;

        // Entry and back-loop Links carry no condition; throughout-folding still applies.
        guardedLink(entryVtxp, waitCp, flp);
        guardedLink(waitRp, waitCp, flp);

        // Continue clocked edge: p && !q advances to next-cycle wait.
        Ast_गाठ_अभिव्यक्ती* स्थिरांक contCondp
            = नवीन AstLogAnd{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                            नवीन AstLogNot{flp, sampledRefOrClone(qHoistp, rhsBitp, flp)}};
        guardedEdge(waitCp, waitRp, contCondp, flp);

        // Reject sink: fires when require-condition is false.
        SvaStateVertex* स्थिरांक sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = सत्य;
        Ast_गाठ_अभिव्यक्ती* requireCondp;
        जर (ov) {
            requireCondp = sampledRefOrClone(pHoistp, lhsBitp, flp);
        } अन्यथा {
            requireCondp = नवीन AstLogOr{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                                        sampledRefOrClone(qHoistp, rhsBitp, flp)};
        }
        SvaTransEdge* स्थिरांक rejEdgep = m_graph.addLink(waitCp, sinkVtxp, requireCondp);
        rejEdgep->m_rejectOnFail = सत्य;

        // Accept condition rides via finalCondp; assembleResult $sampled-wraps it.
        Ast_गाठ_अभिव्यक्ती* acceptCondp;
        जर (ov) {
            acceptCondp
                = नवीन AstLogAnd{flp, lhsBitp->clone_वृक्ष_Pure(असत्य), rhsBitp->clone_वृक्ष_Pure(असत्य)};
        } अन्यथा {
            acceptCondp = rhsBitp->clone_वृक्ष_Pure(असत्य);
        }
        परत {waitCp, acceptCondp, {}};
    }

    // IEEE 1800-2023 16.12.14 property abort operators. Sync and async share
    // the same NFA encoding: AstSampled already gives matured values at every
    // maturing clocking event, and async firing "between clocks" is not
    // observable in a cycle-based model. VAbortKind selects accept vs reject
    // verdict (sync vs async only changes the user-visible spelling).

    // Build `condp && !outer_1 && !outer_2 ...` (unsampled).
    Ast_गाठ_अभिव्यक्ती* abort_Fire_अभिव्यक्ती(Ast_गाठ_अभिव्यक्ती* condp, File_ओळ* flp) {
        Ast_गाठ_अभिव्यक्ती* resultp = condp->clone_वृक्ष_Pure(असत्य);
        साठी (Ast_गाठ_अभिव्यक्ती* स्थिरांक op : m_outer_Abort_चळत)
            resultp = नवीन AstLogAnd{flp, resultp, नवीन AstLogNot{flp, op->clone_वृक्ष_Pure(असत्य)}};
        परत resultp;
    }

    // True when unguarded ring-wide rejection or a same-tick Link chain already accounts the
    // attempt: a required-step Link covers both outcomes; followed-by pairs both edges.
    स्थिर तार्किक chainAccountsSource(स्थिरांक SvaStateVertex* srcp,
                                    स्थिरांक std::unordered_set<स्थिरांक V3GraphEdge*>& preEdges) {
        जर (srcp->m_delayRingSize && srcp->m_throughoutConds.empty()) परत सत्य;
        साठी (स्थिरांक V3GraphEdge& edger : srcp->inEdges()) {
            जर (preEdges.मोजणी(&edger)) सुरूठेवा;
            स्थिरांक SvaTransEdge& tedger = static_cast<स्थिरांक SvaTransEdge&>(edger);
            जर (tedger.m_consumesCycle) सुरूठेवा;
            स्थिरांक स्वतः* स्थिरांक fromp = static_cast<स्थिरांक SvaStateVertex*>(tedger.fromVtxp());
            जर (fromp->m_abortRejectp) परत सत्य;
        }
        तार्किक plainNonSink = असत्य;
        तार्किक markedSink = असत्य;
        साठी (स्थिरांक V3GraphEdge& edger : srcp->outEdges()) {
            जर (preEdges.मोजणी(&edger)) सुरूठेवा;
            स्थिरांक SvaTransEdge& tedger = static_cast<स्थिरांक SvaTransEdge&>(edger);
            जर (tedger.m_consumesCycle) सुरूठेवा;
            स्थिरांक तार्किक sink = static_cast<स्थिरांक SvaStateVertex*>(tedger.toVtxp())->m_isRejectSink;
            जर (tedger.m_rejectOnFail) {
                जर (!sink) परत सत्य;
                markedSink = सत्य;
            } अन्यथा जर (!sink) {
                जर (srcp->m_abortRejectp) परत सत्य;
                plainNonSink = सत्य;
            }
        }
        परत plainNonSink && markedSink;
    }

    // Reject edge: fires when the source is live and the abort samples true.
    शून्य addAbortRejectEdge(SvaStateVertex* srcp, SvaStateVertex* sinkp, Ast_गाठ_अभिव्यक्ती* condp,
                            File_ओळ* flp) {
        Ast_गाठ_अभिव्यक्ती* स्थिरांक notFirep = नवीन AstLogNot{flp, sampled(abort_Fire_अभिव्यक्ती(condp, flp))};
        m_graph.addLink(srcp, sinkp, notFirep)->m_rejectOnFail = सत्य;
        परत;
    }

    // On the fire tick: kill body threads; accept kinds also forgive step misses.
    शून्य gateBodyEdgesOnAbort(स्थिरांक std::unordered_set<स्थिरांक V3GraphEdge*>& preEdges,
                              Ast_गाठ_अभिव्यक्ती* condp, VAbortKind kind, File_ओळ* flp) {
        साठी (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            साठी (V3GraphEdge& edger : vtxr.outEdges()) {
                जर (preEdges.मोजणी(&edger)) सुरूठेवा;
                SvaTransEdge* स्थिरांक tedgep = static_cast<SvaTransEdge*>(&edger);
                जर (tedgep->m_rejectOnFail) {
                    जर (!kind.isAccept()) सुरूठेवा;
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक firep = sampled(abort_Fire_अभिव्यक्ती(condp, flp));
                    tedgep->m_condp
                        = tedgep->m_condp ? नवीन AstLogOr{flp, tedgep->m_condp, firep} : firep;
                } अन्यथा जर (tedgep->m_consumesCycle) {
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक notFirep
                        = नवीन AstLogNot{flp, sampled(abort_Fire_अभिव्यक्ती(condp, flp))};
                    tedgep->m_condp = tedgep->m_condp
                                          ? नवीन AstLogAnd{flp, tedgep->m_condp, notFirep}
                                          : notFirep;
                }
            }
        }
    }

    BuildResult buildAbortOn(Ast_गाठ_अभिव्यक्ती* condp, Ast_गाठ_अभिव्यक्ती* bodyp, SvaStateVertex* entryVtxp,
                             VAbortKind kind, File_ओळ* flp, तार्किक is_शीर्ष_Level_Step) {
        // Snapshot pre-body vertices/edges so post-build diff yields the body's sub-NFA.
        std::unordered_set<स्थिरांक V3GraphVertex*> preExisting;
        std::unordered_set<स्थिरांक V3GraphEdge*> preEdges;
        साठी (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            preExisting.घाला(&vtxr);
            साठी (V3GraphEdge& edger : vtxr.outEdges()) preEdges.घाला(&edger);
        }

        m_outer_Abort_चळत.ढकला_back(condp);
        स्थिरांक BuildResult bodyResult = build_अभिव्यक्ती(bodyp, entryVtxp, is_शीर्ष_Level_Step);
        m_outer_Abort_चळत.काढा_back();
        जर (!bodyResult.valid()) परत bodyResult;

        gateBodyEdgesOnAbort(preEdges, condp, kind, flp);

        // Live-thread sources for the abort edge: entry + new body vertices,
        // minus reject sinks (they carry reject fuel, not live-thread fuel).
        std::vector<SvaStateVertex*> abortSources;
        abortSources.ढकला_back(entryVtxp);
        साठी (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            जर (preExisting.मोजणी(&vtxr)) सुरूठेवा;
            स्वतः* स्थिरांक sp = static_cast<SvaStateVertex*>(&vtxr);
            जर (sp->m_delayRingSize || !sp->m_throughoutConds.empty()) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक firep = abort_Fire_अभिव्यक्ती(condp, flp);
                sp->m_abortClearp
                    = sp->m_abortClearp ? नवीन AstLogOr{flp, sp->m_abortClearp, firep} : firep;
            }
            जर (!kind.isAccept()
                && ((sp->m_delayRingSize && sp->m_throughoutConds.empty())
                    || sp->m_replayAbortReject)) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक firep = abort_Fire_अभिव्यक्ती(condp, flp);
                sp->m_abortRejectp
                    = sp->m_abortRejectp ? नवीन AstLogOr{flp, sp->m_abortRejectp, firep} : firep;
            }
            जर (sp->m_isRejectSink) सुरूठेवा;
            abortSources.ढकला_back(sp);
        }

        स्वतः sampledAbortFire = [&]() -> Ast_गाठ_अभिव्यक्ती* {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक अभिव्यक्ती = abort_Fire_अभिव्यक्ती(condp, flp);
            परत sampled(अभिव्यक्ती);
        };

        जर (kind.isAccept()) {
            // Match-only sink fed by $sampled(abort-fire) from every live source;
            // registered as midSource so it never contributes a reject.
            SvaStateVertex* स्थिरांक acceptSinkp = scopedCreateVertex();
            साठी (SvaStateVertex* स्थिरांक srcp : abortSources)
                guardedLink(srcp, acceptSinkp, sampledAbortFire(), flp);
            std::vector<SvaStateVertex*> midSources = bodyResult.midSources;
            midSources.ढकला_back(acceptSinkp);
            Ast_गाठ_अभिव्यक्ती* finalCondp = bodyResult.finalCondp;
            जर (finalCondp) {
                जर (finalCondp->backp()) finalCondp = finalCondp->clone_वृक्ष_Pure(असत्य);
                finalCondp = नवीन AstLogOr{flp, finalCondp, abort_Fire_अभिव्यक्ती(condp, flp)};
            }
            परत {bodyResult.termVertexp, finalCondp, std::move(midSources)};
        }

        // rejectOnFail treats m_condp as the success condition and fires on
        // !condp, so the edge carries !sampledAbortFire().
        SvaStateVertex* स्थिरांक rejectSinkp = m_graph.createStateVertex();
        rejectSinkp->m_isRejectSink = सत्य;
        साठी (SvaStateVertex* स्थिरांक srcp : abortSources)
            जर (!chainAccountsSource(srcp, preEdges))
                addAbortRejectEdge(srcp, rejectSinkp, condp, flp);
        Ast_गाठ_अभिव्यक्ती* finalCondp = bodyResult.finalCondp;
        जर (finalCondp) {
            जर (finalCondp->backp()) finalCondp = finalCondp->clone_वृक्ष_Pure(असत्य);
            finalCondp
                = नवीन AstLogAnd{flp, finalCondp, नवीन AstLogNot{flp, abort_Fire_अभिव्यक्ती(condp, flp)}};
        }
        परत {bodyResult.termVertexp, finalCondp, bodyResult.midSources};
    }

सार्वजनिक:
    // Free a dropped sub-result condition that is not linked into the AST
    // (abort folds synthesize unparented finalCondp trees).
    स्थिर शून्य freeUnlinkedCondp(Ast_गाठ_अभिव्यक्ती* condp) {
        जर (condp && !condp->backp()) VL_DO_DANGLING(condp->delete_वृक्ष(), condp);
    }

    SvaNfaBuilder(SvaGraph& graph, Ast_गाठ_Module* modp, V3UniqueNames& propTempNames,
                  तार्किक isCoverSeq = असत्य, तार्किक isSeqEvent = असत्य, तार्किक isCover = असत्य)
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
        m_inUnboundedScope = असत्य;
        m_temporal_Guard_चळत.clear();
    }

    BuildResult build_अभिव्यक्ती(Ast_गाठ_अभिव्यक्ती* nodep, SvaStateVertex* entryVtxp,
                          तार्किक is_शीर्ष_Level_Step = असत्य) {
        VL_RESTORER(m_wantBoolLeaf);
        स्थिरांक तार्किक wantBoolLeaf = m_wantBoolLeaf;
        m_wantBoolLeaf = असत्य;
        जर (AstSExpr* स्थिरांक sexprp = VN_CAST(nodep, SExpr)) {
            परत buildSExpr(sexprp, entryVtxp, is_शीर्ष_Level_Step);
        }
        जर (AstSConsRep* स्थिरांक repp = VN_CAST(nodep, SConsRep)) {
            परत buildConsRep(repp, entryVtxp, is_शीर्ष_Level_Step);
        }
        जर (AstPropAlways* स्थिरांक alwaysp = VN_CAST(nodep, PropAlways)) {
            परत buildPropAlways(alwaysp, entryVtxp, is_शीर्ष_Level_Step);
        }
        जर (AstSGotoRep* स्थिरांक repp = VN_CAST(nodep, SGotoRep)) {
            परत buildGotoRep(repp, entryVtxp);
        }
        जर (AstSThroughout* स्थिरांक throughoutp = VN_CAST(nodep, SThroughout)) {
            परत buildThroughout(throughoutp, entryVtxp, is_शीर्ष_Level_Step);
        }
        जर (AstSOr* स्थिरांक orp = VN_CAST(nodep, SOr)) {
            परत buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        जर (AstLogOr* स्थिरांक orp = VN_CAST(nodep, LogOr)) {
            परत buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        जर (AstSAnd* स्थिरांक andp = VN_CAST(nodep, SAnd)) {
            परत buildAndCombiner(andp->lhsp(), andp->rhsp(), entryVtxp, andp->fileline());
        }
        जर (AstSIntersect* स्थिरांक intp = VN_CAST(nodep, SIntersect)) {
            // IEEE 1800-2023 16.9.6: both operands match over one window with
            // equal start and end (equal length). Lower to a single sequence
            // that conjoins both operands' per-cycle checks -- correct under
            // concurrent attempts, where the done-latch combiner conflates the
            // two operands' start times and over-accepts. The combiner remains
            // only as a fallback for operands that do not flatten.
            स्थिरांक पूर्णांक lhsLen = fixedLength(intp->lhsp());
            स्थिरांक पूर्णांक rhsLen = fixedLength(intp->rhsp());
            जर (lhsLen >= 0 && rhsLen >= 0) {
                जर (lhsLen != rhsLen) {
                    // Unequal fixed lengths share no common length -> never matches.
                    परत buildNeverMatchIntersect(intp, entryVtxp, is_शीर्ष_Level_Step,
                                                    "intersect operands have no common length");
                }
                जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक conjp
                    = conjoinFixedSeqs(intp->lhsp(), intp->rhsp(), intp->fileline())) {
                    परत build_From_Lowering_वृक्ष(conjp, entryVtxp, is_शीर्ष_Level_Step);
                }
                परत buildAndCombiner(intp->lhsp(), intp->rhsp(), entryVtxp, intp->fileline());
            }
            परत buildVarLenIntersect(intp, entryVtxp, is_शीर्ष_Level_Step);
        }
        जर (AstSWithin* स्थिरांक withinp = VN_CAST(nodep, SWithin)) {
            परत buildSWithin(withinp, entryVtxp, is_शीर्ष_Level_Step);
        }
        जर (AstAbortOn* स्थिरांक ap = VN_CAST(nodep, AbortOn)) {
            परत buildAbortOn(ap->condp(), ap->propp(), entryVtxp, ap->kind(), ap->fileline(),
                                is_शीर्ष_Level_Step);
        }
        जर (VN_IS(nodep, SNonConsRep)) परत BuildResult::fail();
        जर (AstImplication* स्थिरांक implp = VN_CAST(nodep, Implication)) {
            परत buildImplicationEdges(implp->lhsp(), implp->rhsp(), entryVtxp,
                                         implp->isOverlapped(), implp->isFollowedBy(),
                                         implp->lhsp(), implp->fileline());
        }
        जर (AstUntil* स्थिरांक untilp = VN_CAST(nodep, Until)) {
            परत buildUntil(untilp, entryVtxp, is_शीर्ष_Level_Step);
        }
        // Boolean leaf (including LogAnd): return as finalCond
        परत {entryVtxp, nodep, {}};
    }

    // Wire an implication / followed-by from `entryVtxp`: builds the antecedent,
    // emits the match-link (and for followed-by the reject-sink edge), inserts a
    // delay vertex for non-overlapped forms, and builds the body. Used both for
    // nested AstImplication in pexpr position and for the top-level assertion
    // antecedent -- `errorNodep` anchors the "unsupported sequence antecedent"
    // error, which differs between the two call sites.
    BuildResult buildImplicationEdges(Ast_गाठ_अभिव्यक्ती* antExprp, Ast_गाठ_अभिव्यक्ती* bodyExprp,
                                      SvaStateVertex* entryVtxp, तार्किक isOverlapped,
                                      तार्किक isFollowedBy, Ast_गाठ* errorNodep, File_ओळ* flp) {
        स्थिरांक BuildResult antResult = build_अभिव्यक्ती(antExprp, entryVtxp);
        जर (!antResult.valid()) परत antResult;

        // Followed-by requires pure-boolean antecedent for non-vacuous-fail at
        // the attempt-start cycle. IEEE 1800-2023 16.12.9 permits a multi-cycle
        // sequence LHS, so this is an implementation gap rather than illegal SV.
        जर (isFollowedBy && antResult.termVertexp != entryVtxp) {
            errorNodep->v3warn(E_UNSUPPORTED,
                               "Unsupported: sequence expression as antecedent of followed-by"
                               " (#-# / #=#) (IEEE 1800-2023 16.12.9)");
            परत BuildResult::failWithError();
        }
        UASSERT_OBJ(!isFollowedBy || antResult.finalCondp, errorNodep,
                    "followed-by antecedent terminal at entry must carry finalCondp");

        // Use raw createStateVertex() so trigVtxp starts without liveness --
        // reaching the antecedent terminal is a definitive event.
        SvaStateVertex* स्थिरांक trigVtxp = m_graph.createStateVertex();
        जर (antResult.finalCondp) {
            m_graph.addLink(antResult.termVertexp, trigVtxp,
                            sampled(antResult.finalCondp->clone_वृक्ष_Pure(असत्य)));
            // Followed-by non-vacuous fail: rejectOnFail fires when the attempt
            // is live (termVtx reachable) and sampled(antecedent) is false.
            जर (isFollowedBy) {
                SvaStateVertex* स्थिरांक sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = सत्य;
                SvaTransEdge* स्थिरांक ep
                    = m_graph.addLink(antResult.termVertexp, sinkVtxp,
                                      sampled(antResult.finalCondp->clone_वृक्ष_Pure(असत्य)));
                ep->m_rejectOnFail = सत्य;
            }
            // finalCondp is cloned into the Sampled nodes; if the original is
            // not parented anywhere in the AST anymore it must be freed here
            // or ASan flags it as a leak (e.g. t_sequence_bool_ops).
            जर (!antResult.finalCondp->backp()) {
                VL_DO_DANGLING(antResult.finalCondp->delete_वृक्ष(), antResult.finalCondp);
            }
        } अन्यथा {
            m_graph.addLink(antResult.termVertexp, trigVtxp);
        }
        resetScope();

        SvaStateVertex* bodyEntryp = trigVtxp;
        जर (!isOverlapped) {
            SvaStateVertex* स्थिरांक delayVtxp = m_graph.createStateVertex();
            m_graph.addClockedEdge(trigVtxp, delayVtxp);
            bodyEntryp = delayVtxp;
        }
        // An empty antecedent match must not start the consequent, but a zero-minimum
        // repetition's empty match is not handled here; keep the merge vertex for those
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = !isFollowedBy && !hasZeroMinRep(antExprp);
        परत build_अभिव्यक्ती(bodyExprp, bodyEntryp, /*isTopLevelStep=*/सत्य);
    }

    BuildResult build(Ast_गाठ_अभिव्यक्ती* exprp) {
        m_graph.m_startVertexp = scopedCreateVertex();
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = सत्य;
        परत build_अभिव्यक्ती(exprp, m_graph.m_startVertexp, /*isTopLevelStep=*/सत्य);
    }
};

//######################################################################
// NFA Lowering (converts NFA graph to synthesizable AstAlways blocks)

वर्ग SvaNfaLowering final {
    Ast_गाठ_Module* स्थिरांक m_modp;  // Module to add state vars and always blocks to
    Ast_गाठ_DType* स्थिरांक m_u32DTypep;  // Shared unsigned counter dtype
    V3UniqueNames m_names{"__Vnfa"};
    माप_प्रकार m_statDelayRingEdgeVisits = 0;  // Delay-ring incoming edges visited

    // Per-lowering shared context (passed to phase sub-functions)
    // Per-vertex lowering state is stored in SvaVertexData and accessed via
    // V3GraphVertex::userp() (see SvaStateVertex::datap()).
    संरचना LowerCtx final {
        File_ओळ* flp;  // Source location for generated AST
        Ast_Sen_वृक्ष* senTreep;  // Clock sensitivity tree
        Ast_गाठ_अभिव्यक्ती* disableExprp;  // disable iff expression (may be nullptr)
        Ast_गाठ_अभिव्यक्ती* matchCondp;  // Final boolean match condition (may be nullptr)
        VAssertType assertType;  // Assertion type for control tasks
        VAssertDirectiveType directiveType;  // Directive type for control tasks
        AstVar* killVarp;  // Last observed kill generation
        SvaGraph& graph;  // NFA graph
    };

    // Build a match-now expression: stateSig[i] && $sampled(condp)
    स्थिर Ast_गाठ_अभिव्यक्ती* buildMatchNow(File_ओळ* flp, Ast_गाठ_अभिव्यक्ती* stateExprp, Ast_गाठ_अभिव्यक्ती* condp) {
        Ast_गाठ_अभिव्यक्ती* स्थिरांक statep = stateExprp->clone_वृक्ष_Pure(असत्य);
        जर (!condp) परत statep;
        परत नवीन AstLogAnd{flp, statep, sampled(condp->clone_वृक्ष_Pure(असत्य))};
    }
    स्थिर Ast_गाठ_अभिव्यक्ती* andCond(File_ओळ* flp, Ast_गाठ_अभिव्यक्ती* exprp, Ast_गाठ_अभिव्यक्ती* condp) {
        जर (!condp) परत exprp;
        परत नवीन AstLogAnd{flp, exprp, condp->clone_वृक्ष_Pure(असत्य)};
    }
    // bp is always non-null; only ap can be null (serving as accumulator).
    स्थिर Ast_गाठ_अभिव्यक्ती* orExprs(File_ओळ* flp, Ast_गाठ_अभिव्यक्ती* ap, Ast_गाठ_अभिव्यक्ती* bp) {
        जर (!ap) परत bp;
        परत नवीन AstLogOr{flp, ap, bp};
    }
    स्थिर Ast_गाठ_अभिव्यक्ती* addCountp(File_ओळ* स्थिरांक flp, Ast_गाठ_अभिव्यक्ती* स्थिरांक totalCountp,
                                  Ast_गाठ_अभिव्यक्ती* स्थिरांक contributionp,
                                  Ast_गाठ_अभिव्यक्ती* स्थिरांक enablep = nullptr) {
        // contribution = enable ? contribution : 0;
        Ast_गाठ_अभिव्यक्ती* स्थिरांक enabledContributionp
            = enablep ? नवीन AstCond{flp, enablep, contributionp,
                                    newTypedConstp(flp, contributionp->dtypep(), 0)}
                      : contributionp;
        जर (!totalCountp) परत enabledContributionp;
        परत नवीन AstAdd{flp, totalCountp, enabledContributionp};
    }
    स्थिर Ast_गाठ_अभिव्यक्ती* gateCheckedCountp(File_ओळ* स्थिरांक flp, Ast_गाठ_अभिव्यक्ती* स्थिरांक countp,
                                          Ast_गाठ_अभिव्यक्ती*& overflowp, Ast_गाठ_अभिव्यक्ती* स्थिरांक enablep) {
        जर (overflowp) {
            overflowp = नवीन AstLogAnd{flp, enablep->clone_वृक्ष_Pure(असत्य), overflowp};
        }
        परत addCountp(flp, nullptr, countp, enablep);
    }
    स्थिर Ast_गाठ_अभिव्यक्ती* addCheckedCountp(File_ओळ* स्थिरांक flp, Ast_गाठ_अभिव्यक्ती* स्थिरांक totalCountp,
                                         Ast_गाठ_अभिव्यक्ती* स्थिरांक contributionp, Ast_गाठ_अभिव्यक्ती*& overflowp,
                                         Ast_गाठ_अभिव्यक्ती* स्थिरांक contributionOverflowp = nullptr) {
        जर (contributionOverflowp) { overflowp = orExprs(flp, overflowp, contributionOverflowp); }
        जर (!totalCountp) परत contributionp;
        Ast_गाठ_अभिव्यक्ती* स्थिरांक oldTotalp = totalCountp->clone_वृक्ष_Pure(असत्य);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक resultp = नवीन AstAdd{flp, totalCountp, contributionp};
        overflowp
            = orExprs(flp, overflowp, नवीन AstLt{flp, resultp->clone_वृक्ष_Pure(असत्य), oldTotalp});
        परत resultp;
    }
    AstIf* new_मोजणी_Overflow_Warning(File_ओळ* स्थिरांक flp, Ast_गाठ_अभिव्यक्ती* स्थिरांक condp) स्थिरांक {
        AstDisplay* स्थिरांक dispp = नवीन AstDisplay{
            flp, VDisplayType::DT_WARNING,
            "Cover sequence match count overflowed 32-bit storage; results may be incorrect.",
            nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstIf* स्थिरांक ifp = नवीन AstIf{flp, condp, dispp};
        ifp->branchPred(VBranchPred::BP_UNLIKELY);
        परत ifp;
    }
    स्थिर Ast_गाठ_अभिव्यक्ती* killActive(LowerCtx& c) {
        परत नवीन AstNeq{c.flp, नवीन AstVarRef{c.flp, c.killVarp, VAccess::READ},
                          assertKillGet(c.flp, c.assertType, c.directiveType)};
    }
    स्थिर Ast_गाठ_अभिव्यक्ती* notKillActive(LowerCtx& c) { परत नवीन AstLogNot{c.flp, killActive(c)}; }
    स्थिर Ast_गाठ_अभिव्यक्ती* gateNotKill(LowerCtx& c, Ast_गाठ_अभिव्यक्ती* exprp) {
        जर (!exprp) परत nullptr;
        परत नवीन AstLogAnd{c.flp, exprp, notKillActive(c)};
    }
    स्थिर Ast_गाठ_अभिव्यक्ती* nextRingIndex(File_ओळ* flp, Ast_गाठ_अभिव्यक्ती* idxExprp, uint32_t size) {
        स्थिरांक स्वतः u32Const = [flp](uint32_t मूल्य) {
            परत नवीन AstConst{flp, AstConst::Widthed_मूल्य{}, 32, मूल्य};
        };
        UASSERT_OBJ(size > 0, idxExprp, "Ring size must be positive");
        जर (size == 1) {
            VL_DO_DANGLING(idxExprp->delete_वृक्ष(), idxExprp);
            परत u32Const(0);
        }
        // idx == size - 1 ? 0 : idx + 1
        AstAdd* स्थिरांक addp = नवीन AstAdd{flp, idxExprp->clone_वृक्ष_Pure(असत्य), u32Const(1)};
        addp->dtypeFrom(idxExprp);
        AstCond* स्थिरांक condp
            = नवीन AstCond{flp, नवीन AstEq{flp, idxExprp, u32Const(size - 1)}, u32Const(0), addp};
        condp->dtypeFrom(idxExprp);
        परत condp;
    }
    स्थिर Ast_गाठ_अभिव्यक्ती* delayRingSlot(File_ओळ* flp, SvaStateVertex* vtxp, Ast_गाठ_अभिव्यक्ती* idxExprp,
                                      VAccess access = VAccess::READ) {
        // ring[idx]
        AstVar* स्थिरांक ringp = vtxp->datap()->delayRingVarp;
        AstVarRef* स्थिरांक refp = नवीन AstVarRef{flp, ringp, access};
        जर (vtxp->datap()->delayRingCounts) परत नवीन AstArraySel{flp, refp, idxExprp};
        परत नवीन AstSel{flp, refp, idxExprp, 1};
    }
    स्थिर Ast_गाठ_अभिव्यक्ती* delay_Ring_Head_मोजणी(File_ओळ* flp, SvaStateVertex* vtxp) {
        Ast_गाठ_अभिव्यक्ती* स्थिरांक headp
            = नवीन AstVarRef{flp, vtxp->datap()->delayRingHeadVarp, VAccess::READ};
        जर (vtxp->datap()->delayRingCounts) परत headp;
        परत नवीन AstExtend{flp, headp, vtxp->datap()->delay_Ring_Live_मोजणी_Varp->dtypep()->width()};
    }
    स्थिर Ast_गाठ_अभिव्यक्ती* delayRingAtLastIndex(File_ओळ* स्थिरांक flp, AstVar* स्थिरांक idxp,
                                             स्थिरांक uint32_t size) {
        परत नवीन AstEq{flp, नवीन AstVarRef{flp, idxp, VAccess::READ},
                         नवीन AstConst{flp, AstConst::Widthed_मूल्य{}, 32, size - 1}};
    }
    स्थिर Ast_गाठ_अभिव्यक्ती* delay_Ring_Output_मोजणी(File_ओळ* स्थिरांक flp, SvaStateVertex* स्थिरांक vtxp) {
        AstVar* स्थिरांक idxp = vtxp->datap()->delayRingIdxVarp;
        स्थिरांक uint32_t size = vtxp->m_delayRingSize;
        Ast_गाठ_अभिव्यक्ती* outgoingValidp
            = नवीन AstVarRef{flp, vtxp->datap()->delayRingWrappedVarp, VAccess::READ};
        जर (!vtxp->m_isFixedDelayRing) {
            outgoingValidp
                = नवीन AstLogOr{flp, outgoingValidp, delayRingAtLastIndex(flp, idxp, size)};
        }
        AstVar* स्थिरांक live_मोजणी_Varp = vtxp->datap()->delay_Ring_Live_मोजणी_Varp;
        परत नवीन AstCond{flp, outgoingValidp, delay_Ring_Head_मोजणी(flp, vtxp),
                           newTypedConstp(flp, live_मोजणी_Varp->dtypep(), 0)};
    }
    स्थिर Ast_गाठ_अभिव्यक्ती* delayRingHasLiveThreadsp(File_ओळ* स्थिरांक flp,
                                                 AstVar* स्थिरांक live_मोजणी_Varp) {
        // active = live_count != 0;
        परत नवीन AstNeq{flp, नवीन AstVarRef{flp, live_मोजणी_Varp, VAccess::READ},
                          newTypedConstp(flp, live_मोजणी_Varp->dtypep(), 0)};
    }

    // Phase 3 output signals
    संरचना SignalSet final {
        Ast_गाठ_अभिव्यक्ती* terminalActivep = nullptr;  // OR of all successful terminal matches
        Ast_गाठ_अभिव्यक्ती* matchCountp = nullptr;  // NFA paths completing the sequence this tick
        Ast_गाठ_अभिव्यक्ती* match_मोजणी_Overflowp = nullptr;  // Match count overflow this tick
        Ast_गाठ_अभिव्यक्ती* rejectBasep = nullptr;  // Reject when a terminal match fails
        Ast_गाठ_अभिव्यक्ती* requiredStepRejectp = nullptr;  // Per-source reject from rejectOnFail Links
        Ast_गाठ_अभिव्यक्ती* throughoutRejectp = nullptr;  // Reject when a throughout guard drops
        Ast_गाठ_अभिव्यक्ती* threadFailCountp = nullptr;  // Number of threads rejected on this tick
    };

    // Disable condition also aborts attempts between clock events. A constant disable
    // never rises, and $sampled is unsupported in a sensitivity list.
    स्थिर तार्किक isAsyncDisable(स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक disableExprp) {
        परत disableExprp && !VN_IS(disableExprp, Const)
               && !disableExprp->exists([](स्थिरांक AstSampled*) { परत सत्य; });
    }

    // Emit the clocked update block of registers that hold pending attempts. The clearp
    // statements reset the same registers: a disable rising between clock events also
    // aborts pending attempts (IEEE 1800-2023 16.12), so clearp runs on that rising edge
    // in a separate block. It must clear unconditionally, as the disable may already be
    // false again when it runs, and the update block must not run on that edge, as it
    // would advance the attempts as if the clock had ticked. Blocks that hold no attempt
    // state, such as the $assertkill acknowledge in emitKillAckNba, do not go through here.
    // Callers pass a null clearp when the disable cannot rise between clock events.
    शून्य addStateAlways(LowerCtx& c, Ast_गाठ* bodyp, Ast_गाठ* clearp) {
        m_modp->addStmtsp(
            नवीन AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_वृक्ष(असत्य), bodyp});
        जर (!clearp) परत;
        m_modp->addStmtsp(नवीन AstAlways{
            c.flp, VAlwaysKwd::ALWAYS,
            नवीन Ast_Sen_वृक्ष{c.flp, नवीन AstSenItem{c.flp, VEdgeType::ET_POSEDGE,
                                                 c.disableExprp->clone_वृक्ष_Pure(असत्य)}},
            clearp});
    }

    // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
    // Phase 2: State register NBA always block. Each clocked-edge target
    // latches the OR of its incoming contributions.
    शून्य emitStateRegisterNba(LowerCtx& c) {
        स्थिरांक तार्किक asyncDisable = isAsyncDisable(c.disableExprp);
        Ast_गाठ* bodyp = nullptr;
        Ast_गाठ* clearp = nullptr;
        साठी (स्थिरांक V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            स्थिरांक SvaStateVertex* स्थिरांक vtxp = static_cast<स्थिरांक SvaStateVertex*>(&vtxr);
            जर (!vtxp->datap()->stateVarp) सुरूठेवा;

            Ast_गाठ_अभिव्यक्ती* nextStatep = nullptr;
            साठी (स्थिरांक V3GraphEdge& edger : vtxp->inEdges()) {
                स्थिरांक SvaTransEdge& tedger = static_cast<स्थिरांक SvaTransEdge&>(edger);
                जर (!tedger.m_consumesCycle) सुरूठेवा;
                स्थिरांक SvaStateVertex* स्थिरांक fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Clocked-edge source missing stateSig");

                Ast_गाठ_अभिव्यक्ती* srcSigp = fromVtxp->datap()->stateSigp->clone_वृक्ष_Pure(असत्य);
                srcSigp = andCond(c.flp, srcSigp, tedger.m_condp);

                जर (c.disableExprp) {
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक notDisp
                        = नवीन AstLogNot{c.flp, c.disableExprp->clone_वृक्ष_Pure(असत्य)};
                    srcSigp = नवीन AstLogAnd{c.flp, srcSigp, notDisp};
                }
                nextStatep = orExprs(c.flp, nextStatep, srcSigp);
            }

            UASSERT_OBJ(nextStatep, vtxp,
                        "Registered vertex has no clocked incoming contribution");
            nextStatep = gateNotKill(c, nextStatep);

            AstAssignDly* स्थिरांक assignp = नवीन AstAssignDly{
                c.flp, नवीन AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE}, nextStatep};
            bodyp = Ast_गाठ::addNextNull(bodyp, assignp);
            जर (asyncDisable) {
                clearp = Ast_गाठ::addNextNull(
                    clearp,
                    नवीन AstAssignDly{
                        c.flp, नवीन AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE},
                        नवीन AstConst{c.flp, AstConst::BitFalse{}}});
            }
        }
        जर (bodyp) addStateAlways(c, bodyp, clearp);
    }

    // Phase 2b: In-place ring storage and registered ring metadata.
    शून्य emitDelayRingUpdates(LowerCtx& c, स्थिरांक तार्किक मोजणी_Matches) {
        स्थिरांक तार्किक asyncDisable = isAsyncDisable(c.disableExprp);
        साठी (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* स्थिरांक vtxp = static_cast<SvaStateVertex*>(&vtxr);
            जर (!vtxp->datap()->delayRingVarp) सुरूठेवा;
            AstVar* स्थिरांक idxp = vtxp->datap()->delayRingIdxVarp;
            AstVar* स्थिरांक live_मोजणी_Varp = vtxp->datap()->delay_Ring_Live_मोजणी_Varp;
            AstVar* स्थिरांक wrappedp = vtxp->datap()->delayRingWrappedVarp;
            स्थिरांक uint32_t size = static_cast<uint32_t>(vtxp->m_delayRingSize);

            Ast_गाठ_अभिव्यक्ती* incomingp = nullptr;
            Ast_गाठ_अभिव्यक्ती* incomingOverflowp = nullptr;
            साठी (स्थिरांक V3GraphEdge& edger : vtxp->inEdges()) {
                ++m_statDelayRingEdgeVisits;
                स्थिरांक SvaTransEdge& tedger = static_cast<स्थिरांक SvaTransEdge&>(edger);
                UASSERT_OBJ(tedger.m_consumesCycle == vtxp->m_isFixedDelayRing, vtxp,
                            "Delay-ring incoming edge kind mismatch");
                स्थिरांक SvaStateVertex* स्थिरांक fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Delay-ring incoming source missing stateSig");
                UASSERT_OBJ(!मोजणी_Matches || fromVtxp->datap()->state_मोजणी_Sigp, fromVtxp,
                            "Delay-ring incoming source missing stateCountSig");
                // IEEE 1800-2023 16.14.1/16.14.3: assertion alternatives belong to one
                // evaluation; only cover sequence counts each end-of-match.
                Ast_गाठ_अभिव्यक्ती* contribp
                    = मोजणी_Matches
                          ? fromVtxp->datap()->state_मोजणी_Sigp->clone_वृक्ष_Pure(असत्य)
                          : नवीन AstExtend{c.flp,
                                          fromVtxp->datap()->stateSigp->clone_वृक्ष_Pure(असत्य),
                                          m_u32DTypep->width()};
                Ast_गाठ_अभिव्यक्ती* contribOverflowp
                    = मोजणी_Matches && fromVtxp->datap()->state_मोजणी_Overflow_Sigp
                          ? fromVtxp->datap()->state_मोजणी_Overflow_Sigp->clone_वृक्ष_Pure(असत्य)
                          : nullptr;
                जर (tedger.m_condp) {
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक enablep = tedger.m_condp->clone_वृक्ष_Pure(असत्य);
                    contribp = मोजणी_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, enablep)
                                   : addCountp(c.flp, nullptr, contribp, enablep);
                }
                जर (c.disableExprp) {
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक notDisp
                        = नवीन AstLogNot{c.flp, c.disableExprp->clone_वृक्ष_Pure(असत्य)};
                    contribp = मोजणी_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, notDisp)
                                   : addCountp(c.flp, nullptr, contribp, notDisp);
                }
                incomingp = मोजणी_Matches ? addCheckedCountp(c.flp, incomingp, contribp,
                                                            incomingOverflowp, contribOverflowp)
                                         : addCountp(c.flp, incomingp, contribp);
            }
            UASSERT_OBJ(incomingp, vtxp, "Delay ring has no incoming edge");
            स्थिरांक तार्किक storesCounts = vtxp->datap()->delayRingCounts;
            Ast_गाठ_अभिव्यक्ती* स्थिरांक incomingCountp
                = storesCounts
                      ? incomingp->clone_वृक्ष_Pure(असत्य)
                      : नवीन AstExtend{c.flp,
                                      नवीन AstNeq{c.flp, incomingp->clone_वृक्ष_Pure(असत्य),
                                                 newTypedConstp(c.flp, incomingp->dtypep(), 0)},
                                      m_u32DTypep->width()};
            Ast_गाठ_अभिव्यक्ती* स्थिरांक storedIncomingp
                = storesCounts ? incomingp
                               : नवीन AstNeq{c.flp, incomingp,
                                            newTypedConstp(c.flp, incomingp->dtypep(), 0)};
            // Only this block accesses the storage. Other processes read the cached head, so
            // the ring can be updated in place without an NBA shadow copy.
            AstAssign* स्थिरांक writeIncomingp = नवीन AstAssign{
                c.flp,
                delayRingSlot(c.flp, vtxp, नवीन AstVarRef{c.flp, idxp, VAccess::READ},
                              VAccess::WRITE),
                storedIncomingp};
            Ast_गाठ* updateBodyp = writeIncomingp;
            // live_count <= live_count - outgoing_count + incoming_count;
            Ast_गाठ_अभिव्यक्ती* स्थिरांक outgoingp = delay_Ring_Output_मोजणी(c.flp, vtxp);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक afterOutgoingp
                = नवीन AstSub{c.flp, नवीन AstVarRef{c.flp, live_मोजणी_Varp, VAccess::READ}, outgoingp};
            Ast_गाठ_अभिव्यक्ती* स्थिरांक oldAfterOutgoingp = afterOutgoingp->clone_वृक्ष_Pure(असत्य);
            AstAdd* स्थिरांक nextLiveCountp = नवीन AstAdd{c.flp, afterOutgoingp, incomingCountp};
            जर (storesCounts) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक liveOverflowp
                    = नवीन AstLt{c.flp, nextLiveCountp->clone_वृक्ष_Pure(असत्य), oldAfterOutgoingp};
                Ast_गाठ_अभिव्यक्ती* स्थिरांक overflowp = orExprs(c.flp, incomingOverflowp, liveOverflowp);
                updateBodyp = new_मोजणी_Overflow_Warning(c.flp, overflowp);
                updateBodyp->addNext(writeIncomingp);
            } अन्यथा {
                VL_DO_DANGLING(oldAfterOutgoingp->delete_वृक्ष(), oldAfterOutgoingp);
            }
            // Cache the outgoing slot for the next index after the write. Reading after
            // writing also handles one-slot rings and two-slot range rings without alias cases.
            Ast_गाठ_अभिव्यक्ती* nextHeadIdxp
                = nextRingIndex(c.flp, नवीन AstVarRef{c.flp, idxp, VAccess::READ}, size);
            जर (!vtxp->m_isFixedDelayRing) {
                nextHeadIdxp = nextRingIndex(c.flp, nextHeadIdxp, size);
            }
            updateBodyp->addNext(नवीन AstAssignDly{
                c.flp, नवीन AstVarRef{c.flp, vtxp->datap()->delayRingHeadVarp, VAccess::WRITE},
                delayRingSlot(c.flp, vtxp, nextHeadIdxp)});
            updateBodyp->addNext(नवीन AstAssignDly{
                c.flp, नवीन AstVarRef{c.flp, live_मोजणी_Varp, VAccess::WRITE}, nextLiveCountp});
            // wrapped <= wrapped || idx == size - 1;
            updateBodyp->addNext(
                नवीन AstAssignDly{c.flp, नवीन AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                 नवीन AstLogOr{c.flp, नवीन AstVarRef{c.flp, wrappedp, VAccess::READ},
                                              delayRingAtLastIndex(c.flp, idxp, size)}});
            // idx <= next_idx;
            updateBodyp->addNext(नवीन AstAssignDly{
                c.flp, नवीन AstVarRef{c.flp, idxp, VAccess::WRITE},
                nextRingIndex(c.flp, नवीन AstVarRef{c.flp, idxp, VAccess::READ}, size)});
            जर (vtxp->m_delayRingAdvanceCondp) {
                updateBodyp = नवीन AstIf{
                    c.flp, sampled(vtxp->m_delayRingAdvanceCondp->clone_वृक्ष_Pure(असत्य)),
                    updateBodyp};
            }

            Ast_गाठ_अभिव्यक्ती* clearCondp = killActive(c);
            जर (vtxp->m_delayRingClearCondp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_delayRingClearCondp->clone_वृक्ष_Pure(असत्य)));
            }
            जर (vtxp->m_abortClearp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_abortClearp->clone_वृक्ष_Pure(असत्य)));
            }
            जर (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_वृक्ष_Pure(असत्य));
            }
            Ast_गाठ_अभिव्यक्ती* guardp = nullptr;
            साठी (Ast_गाठ_अभिव्यक्ती* स्थिरांक cp : vtxp->m_throughoutConds) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक sampledp = sampled(cp->clone_वृक्ष_Pure(असत्य));
                guardp = guardp ? static_cast<Ast_गाठ_अभिव्यक्ती*>(नवीन AstLogAnd{c.flp, guardp, sampledp})
                                : sampledp;
            }
            जर (guardp) clearCondp = orExprs(c.flp, clearCondp, नवीन AstLogNot{c.flp, guardp});
            // Logically clear the ring without touching its wide storage.
            AstAssignDly* स्थिरांक clearCountp
                = नवीन AstAssignDly{c.flp, नवीन AstVarRef{c.flp, live_मोजणी_Varp, VAccess::WRITE},
                                   newTypedConstp(c.flp, live_मोजणी_Varp->dtypep(), 0)};
            clearCountp->addNext(नवीन AstAssignDly{c.flp,
                                                  नवीन AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                                  नवीन AstConst{c.flp, AstConst::BitFalse{}}});
            clearCountp->addNext(नवीन AstAssignDly{c.flp,
                                                  नवीन AstVarRef{c.flp, idxp, VAccess::WRITE},
                                                  newTypedConstp(c.flp, idxp->dtypep(), 0)});
            updateBodyp = नवीन AstIf{c.flp, clearCondp, clearCountp, updateBodyp};

            addStateAlways(c, updateBodyp, asyncDisable ? clearCountp->clone_वृक्ष(सत्य) : nullptr);
        }
    }

    // Phase 2c: SAnd combiner done-latch always block.
    // NBA semantics ensure doneL/doneR read pre-update values (IEEE 16.9.5).
    शून्य emitAndCombinerDoneLatchNba(LowerCtx& c) {
        स्थिरांक तार्किक asyncDisable = isAsyncDisable(c.disableExprp);
        साठी (स्थिरांक V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            स्थिरांक SvaStateVertex* स्थिरांक avp = static_cast<स्थिरांक SvaStateVertex*>(&vtxr);
            जर (!avp->datap()->doneLVarp) सुरूठेवा;
            // doneLVars is non-null only for AndCombiner vertices, which always
            // have both m_andLhsTermp and m_andRhsTermp set at build time.
            UASSERT_OBJ(avp->m_andLhsTermp && avp->m_andRhsTermp, avp,
                        "AndCombiner vertex missing LHS/RHS terminal");
            स्थिरांक SvaStateVertex* स्थिरांक lhsTermp = avp->m_andLhsTermp;
            स्थिरांक SvaStateVertex* स्थिरांक rhsTermp = avp->m_andRhsTermp;
            // resolveLinks' 2*N+2 fixed-point pass is guaranteed to populate
            // stateSigp on every AndCombiner and its LHS/RHS terminals.
            UASSERT_OBJ(lhsTermp->datap()->stateSigp && rhsTermp->datap()->stateSigp
                            && avp->datap()->stateSigp,
                        avp, "AndCombiner stateSigp chain unresolved after resolveLinks");

            AstAssignDly* स्थिरांक clearLp = नवीन AstAssignDly{
                c.flp, नवीन AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                नवीन AstConst{c.flp, AstConst::BitFalse{}}};
            AstAssignDly* स्थिरांक clearRp = नवीन AstAssignDly{
                c.flp, नवीन AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                नवीन AstConst{c.flp, AstConst::BitFalse{}}};
            clearLp->addNext(clearRp);

            Ast_गाठ_अभिव्यक्ती* स्थिरांक matchLNowp
                = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, avp->m_andLhsCondp);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक matchRNowp
                = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, avp->m_andRhsCondp);
            Ast_गाठ_अभिव्यक्ती* gateLp = matchLNowp;
            Ast_गाठ_अभिव्यक्ती* gateRp = matchRNowp;
            जर (c.disableExprp) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक notDisLp
                    = नवीन AstLogNot{c.flp, c.disableExprp->clone_वृक्ष_Pure(असत्य)};
                gateLp = नवीन AstLogAnd{c.flp, gateLp, notDisLp};
                Ast_गाठ_अभिव्यक्ती* स्थिरांक notDisRp
                    = नवीन AstLogNot{c.flp, c.disableExprp->clone_वृक्ष_Pure(असत्य)};
                gateRp = नवीन AstLogAnd{c.flp, gateRp, notDisRp};
            }
            AstAssignDly* स्थिरांक setLp = नवीन AstAssignDly{
                c.flp, नवीन AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                नवीन AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* स्थिरांक setLIfp = नवीन AstIf{c.flp, gateLp, setLp, nullptr};
            AstAssignDly* स्थिरांक setRp = नवीन AstAssignDly{
                c.flp, नवीन AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                नवीन AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* स्थिरांक setRIfp = नवीन AstIf{c.flp, gateRp, setRp, nullptr};
            setLIfp->addNext(setRIfp);

            Ast_गाठ_अभिव्यक्ती* clearCondp = नवीन AstLogOr{c.flp, killActive(c),
                                                   avp->datap()->stateSigp->clone_वृक्ष_Pure(असत्य)};
            जर (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_वृक्ष_Pure(असत्य));
            }
            AstIf* स्थिरांक topp = नवीन AstIf{c.flp, clearCondp, clearLp, setLIfp};
            addStateAlways(c, topp, asyncDisable ? clearLp->clone_वृक्ष(सत्य) : nullptr);
        }
    }

    शून्य emitKillAckNba(LowerCtx& c) {
        AstAssignDly* स्थिरांक ackp
            = नवीन AstAssignDly{c.flp, नवीन AstVarRef{c.flp, c.killVarp, VAccess::WRITE},
                               assertKillGet(c.flp, c.assertType, c.directiveType)};
        m_modp->addStmtsp(
            नवीन AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_वृक्ष(असत्य), ackp});
    }

    // Phase 3/3a/3b: Compute terminal match/reject signals, required-step reject,
    // throughout-drop reject; clean up intermediate state signals.
    // Phase 3: terminalActive and rejectBase from Links to matchVertex.
    // Builder only adds Links (non-clocked) to matchVertex via addLink in
    // wireMatchAndMidSources. For cover sequence, also count each end-of-match
    // so the action can be replayed without unrolling endpoints.
    शून्य computeTerminalMatchAndReject(LowerCtx& c, SignalSet& sigs, स्थिरांक तार्किक need_Match_मोजणी) {
        साठी (स्थिरांक SvaTransEdge* स्थिरांक tedgep : c.graph.allEdges()) {
            जर (tedgep->toVtxp() != c.graph.m_matchVertexp) सुरूठेवा;
            SvaStateVertex* स्थिरांक fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                        "Terminal-link source missing stateSig");

            Ast_गाठ_अभिव्यक्ती* srcSigp = fromVtxp->datap()->stateSigp->clone_वृक्ष_Pure(असत्य);
            srcSigp = andCond(c.flp, srcSigp, tedgep->m_condp);
            जर (need_Match_मोजणी) {
                UASSERT_OBJ(fromVtxp->datap()->state_मोजणी_Sigp, fromVtxp,
                            "Terminal-link source missing stateCountSig");
                Ast_गाठ_अभिव्यक्ती* contributionp
                    = fromVtxp->datap()->state_मोजणी_Sigp->clone_वृक्ष_Pure(असत्य);
                Ast_गाठ_अभिव्यक्ती* contributionOverflowp
                    = fromVtxp->datap()->state_मोजणी_Overflow_Sigp
                          ? fromVtxp->datap()->state_मोजणी_Overflow_Sigp->clone_वृक्ष_Pure(असत्य)
                          : nullptr;
                जर (tedgep->m_condp) {
                    contributionp = gateCheckedCountp(c.flp, contributionp, contributionOverflowp,
                                                      tedgep->m_condp->clone_वृक्ष_Pure(असत्य));
                }
                sigs.matchCountp
                    = addCheckedCountp(c.flp, sigs.matchCountp, contributionp,
                                       sigs.match_मोजणी_Overflowp, contributionOverflowp);
            }

            जर (fromVtxp->m_delayRingSize && !fromVtxp->m_isFixedDelayRing) {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_वृक्ष_Pure(असत्य));
                // reject |= (wrapped || idx == size - 1) && head && final_condition;
                Ast_गाठ_अभिव्यक्ती* expireContribp = delay_Ring_Output_मोजणी(c.flp, fromVtxp);
                expireContribp = andCond(c.flp, expireContribp, tedgep->m_condp);
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, expireContribp);
                VL_DO_DANGLING(srcSigp->delete_वृक्ष(), srcSigp);
            } अन्यथा जर (fromVtxp->m_isUnbounded || fromVtxp->m_isAndCombiner) {
                sigs.terminalActivep = orExprs(c.flp, sigs.terminalActivep, srcSigp);
            } अन्यथा {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_वृक्ष_Pure(असत्य));
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, srcSigp);
            }
        }
        // wireMatchAndMidSources always adds a Link from result.termVertexp
        // to m_matchVertexp, so the loop above always sets terminalActivep.
        UASSERT_OBJ(sigs.terminalActivep, c.graph.m_matchVertexp,
                    "No terminal edge to match vertex");
    }

    Ast_गाठ_अभिव्यक्ती* newThroughoutThreadFailCountp(LowerCtx& c, AstVar* स्थिरांक delay_Ring_Live_मोजणी_Varp,
                                               Ast_गाठ_अभिव्यक्ती* स्थिरांक stateExprp,
                                               Ast_गाठ_अभिव्यक्ती* स्थिरांक enablep) {
        Ast_गाठ_अभिव्यक्ती* स्थिरांक activeThreadCountp
            = delay_Ring_Live_मोजणी_Varp
                  ? static_cast<Ast_गाठ_अभिव्यक्ती*>(
                        नवीन AstVarRef{c.flp, delay_Ring_Live_मोजणी_Varp, VAccess::READ})
                  : नवीन AstExtend{c.flp, stateExprp->clone_वृक्ष_Pure(असत्य), m_u32DTypep->width()};
        परत addCountp(c.flp, nullptr, activeThreadCountp, enablep);
    }

    // Phase 3b: Throughout-drop and ring-wide abort rejection.
    शून्य computeThroughoutReject(LowerCtx& c, SignalSet& sigs, स्थिरांक तार्किक need_Thread_Fail_मोजणी) {
        साठी (स्थिरांक V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            स्थिरांक SvaStateVertex* स्थिरांक vtxp = static_cast<स्थिरांक SvaStateVertex*>(&vtxr);
            स्थिरांक स्वतः& conds = vtxp->m_throughoutConds;
            जर (conds.empty() && !vtxp->m_abortRejectp) सुरूठेवा;
            जर (vtxp->m_isAndCombiner) सुरूठेवा;
            Ast_गाठ_अभिव्यक्ती* stateExprp = nullptr;
            जर (vtxp->datap()->stateVarp) {
                stateExprp = नवीन AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
            } अन्यथा जर (vtxp->datap()->delayRingVarp && vtxp->m_isFixedDelayRing) {
                stateExprp
                    = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_मोजणी_Varp);
            } अन्यथा {
                UASSERT_OBJ(vtxp->datap()->stateSigp, vtxp,
                            "Throughout-conds vertex missing state representation");
                stateExprp = vtxp->datap()->stateSigp->clone_वृक्ष_Pure(असत्य);
            }
            Ast_गाठ_अभिव्यक्ती* guardp = nullptr;
            साठी (Ast_गाठ_अभिव्यक्ती* स्थिरांक cp : conds) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक sp = sampled(cp->clone_वृक्ष_Pure(असत्य));
                guardp = guardp ? static_cast<Ast_गाठ_अभिव्यक्ती*>(नवीन AstLogAnd{c.flp, guardp, sp}) : sp;
            }
            जर (vtxp->m_abortRejectp) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक notAbortp
                    = नवीन AstLogNot{c.flp, sampled(vtxp->m_abortRejectp->clone_वृक्ष_Pure(असत्य))};
                guardp = guardp
                             ? static_cast<Ast_गाठ_अभिव्यक्ती*>(नवीन AstLogAnd{c.flp, guardp, notAbortp})
                             : notAbortp;
            }
            Ast_गाठ_अभिव्यक्ती* rejectCondp = नवीन AstLogNot{c.flp, guardp};
            जर (vtxp->m_abortClearp) {
                // Any abort clears the ring. Accept aborts suppress a simultaneous guard failure;
                // reject aborts are restored below so they still force rejection.
                Ast_गाठ_अभिव्यक्ती* स्थिरांक notAbortClearp
                    = नवीन AstLogNot{c.flp, sampled(vtxp->m_abortClearp->clone_वृक्ष_Pure(असत्य))};
                rejectCondp = नवीन AstLogAnd{c.flp, rejectCondp, notAbortClearp};
                जर (vtxp->m_abortRejectp) {
                    rejectCondp = नवीन AstLogOr{
                        c.flp, rejectCondp, sampled(vtxp->m_abortRejectp->clone_वृक्ष_Pure(असत्य))};
                }
            }
            जर (need_Thread_Fail_मोजणी) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक contributionp
                    = newThroughoutThreadFailCountp(c, vtxp->datap()->delay_Ring_Live_मोजणी_Varp,
                                                    stateExprp, rejectCondp->clone_वृक्ष_Pure(असत्य));
                sigs.threadFailCountp = addCountp(c.flp, sigs.threadFailCountp, contributionp);
                जर (vtxp->m_abortRejectp && vtxp->m_delayRingAdvanceCondp) {
                    // An advancing ring thread also occupies its same-tick match vertex in the
                    // unrolled NFA, so abort rejection must replay both fail actions.
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक abortAndAdvancep = नवीन AstLogAnd{
                        c.flp, sampled(vtxp->m_abortRejectp->clone_वृक्ष_Pure(असत्य)),
                        sampled(vtxp->m_delayRingAdvanceCondp->clone_वृक्ष_Pure(असत्य))};
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक matchContributionp = newThroughoutThreadFailCountp(
                        c, vtxp->datap()->delay_Ring_Live_मोजणी_Varp, stateExprp, abortAndAdvancep);
                    sigs.threadFailCountp
                        = addCountp(c.flp, sigs.threadFailCountp, matchContributionp);
                }
            }
            sigs.throughoutRejectp = orExprs(c.flp, sigs.throughoutRejectp,
                                             नवीन AstLogAnd{c.flp, stateExprp, rejectCondp});
        }
    }

    SignalSet computeSignals(LowerCtx& c, स्थिरांक तार्किक need_Thread_Fail_मोजणी,
                             स्थिरांक तार्किक need_Throughout_Thread_Fail_मोजणी, स्थिरांक तार्किक need_Match_मोजणी) {
        SignalSet sigs;

        computeTerminalMatchAndReject(c, sigs, need_Match_मोजणी);

        // Phase 3a: required-step rejection.
        // Builder only sets m_rejectOnFail on non-clocked Links with m_condp
        // or m_condVtxp, and the source always has a resolved stateSig.
        साठी (स्थिरांक SvaTransEdge* स्थिरांक tedgep : c.graph.allEdges()) {
            जर (!tedgep->m_rejectOnFail) सुरूठेवा;
            स्थिरांक SvaStateVertex* स्थिरांक fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp && (tedgep->m_condp || tedgep->m_condVtxp),
                        fromVtxp,
                        "rejectOnFail Link must have condp/condVtxp and source stateSig");
            Ast_गाठ_अभिव्यक्ती* स्थिरांक srcSigp = fromVtxp->datap()->stateSigp->clone_वृक्ष_Pure(असत्य);
            Ast_गाठ_अभिव्यक्ती* condp = nullptr;
            जर (tedgep->m_condVtxp) {
                UASSERT_OBJ(tedgep->m_condVtxp->datap()->stateSigp, tedgep->m_condVtxp,
                            "rejectOnFail condVtxp missing stateSig");
                condp = tedgep->m_condVtxp->datap()->stateSigp->clone_वृक्ष_Pure(असत्य);
                जर (tedgep->m_condp) {
                    condp = नवीन AstLogOr{c.flp, condp, tedgep->m_condp->clone_वृक्ष_Pure(असत्य)};
                }
            } अन्यथा {
                condp = tedgep->m_condp->clone_वृक्ष_Pure(असत्य);
            }
            Ast_गाठ_अभिव्यक्ती* स्थिरांक notCondp = नवीन AstLogNot{c.flp, condp};
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rawFailp = नवीन AstLogAnd{c.flp, srcSigp, notCondp};
            जर (need_Thread_Fail_मोजणी) {
                // thread_fail_count += fail;
                sigs.threadFailCountp = addCountp(
                    c.flp, sigs.threadFailCountp,
                    नवीन AstExtend{c.flp, rawFailp->clone_वृक्ष_Pure(असत्य), m_u32DTypep->width()});
            }
            Ast_गाठ_अभिव्यक्ती* स्थिरांक failp = gateNotKill(c, rawFailp);
            sigs.requiredStepRejectp = orExprs(c.flp, sigs.requiredStepRejectp, failp);
        }

        computeThroughoutReject(c, sigs, need_Throughout_Thread_Fail_मोजणी);
        जर (sigs.threadFailCountp) {
            sigs.threadFailCountp
                = addCountp(c.flp, nullptr, sigs.threadFailCountp, notKillActive(c));
        }
        जर (sigs.matchCountp) {
            जर (c.matchCondp) {
                sigs.matchCountp
                    = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_मोजणी_Overflowp,
                                        sampled(c.matchCondp->clone_वृक्ष_Pure(असत्य)));
            }
            sigs.matchCountp = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_मोजणी_Overflowp,
                                                 notKillActive(c));
        }
        sigs.terminalActivep = gateNotKill(c, sigs.terminalActivep);
        sigs.rejectBasep = gateNotKill(c, sigs.rejectBasep);
        sigs.throughoutRejectp = gateNotKill(c, sigs.throughoutRejectp);

        // Clean up intermediate state signals. These are orphan subtrees
        // (never linked into the enclosing AST); deleteTree() is immediate
        // which is what we want since stateSigp lifetime ends with this scope.
        साठी (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* स्थिरांक vtxp = static_cast<SvaStateVertex*>(&vtxr);
            Ast_गाठ_अभिव्यक्ती*& sigp = vtxp->datap()->stateSigp;
            जर (sigp) VL_DO_DANGLING(sigp->delete_वृक्ष(), sigp);
            Ast_गाठ_अभिव्यक्ती*& countp = vtxp->datap()->state_मोजणी_Sigp;
            जर (countp) VL_DO_DANGLING(countp->delete_वृक्ष(), countp);
            Ast_गाठ_अभिव्यक्ती*& overflowp = vtxp->datap()->state_मोजणी_Overflow_Sigp;
            जर (overflowp) VL_DO_DANGLING(overflowp->delete_वृक्ष(), overflowp);
        }
        // Disable iff gating (IEEE 1800-2023 16.12) on the current level value.
        जर (c.disableExprp) {
            // terminalActivep is always set, so gate it unconditionally.
            Ast_गाठ_अभिव्यक्ती* स्थिरांक notTermp
                = नवीन AstLogNot{c.flp, c.disableExprp->clone_वृक्ष_Pure(असत्य)};
            sigs.terminalActivep = नवीन AstLogAnd{c.flp, sigs.terminalActivep, notTermp};
            जर (sigs.rejectBasep) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक notDisp
                    = नवीन AstLogNot{c.flp, c.disableExprp->clone_वृक्ष_Pure(असत्य)};
                sigs.rejectBasep = नवीन AstLogAnd{c.flp, sigs.rejectBasep, notDisp};
            }
            जर (sigs.throughoutRejectp) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक notDisp
                    = नवीन AstLogNot{c.flp, c.disableExprp->clone_वृक्ष_Pure(असत्य)};
                sigs.throughoutRejectp = नवीन AstLogAnd{c.flp, sigs.throughoutRejectp, notDisp};
            }
            जर (sigs.requiredStepRejectp) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक notDisp
                    = नवीन AstLogNot{c.flp, c.disableExprp->clone_वृक्ष_Pure(असत्य)};
                sigs.requiredStepRejectp = नवीन AstLogAnd{c.flp, sigs.requiredStepRejectp, notDisp};
            }
            जर (sigs.matchCountp) {
                sigs.matchCountp = gateCheckedCountp(
                    c.flp, sigs.matchCountp, sigs.match_मोजणी_Overflowp,
                    नवीन AstLogNot{c.flp, c.disableExprp->clone_वृक्ष_Pure(असत्य)});
            }
        }

        जर (sigs.match_मोजणी_Overflowp) {
            m_modp->addStmtsp(
                नवीन AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_वृक्ष(असत्य),
                              new_मोजणी_Overflow_Warning(c.flp, sigs.match_मोजणी_Overflowp)});
            sigs.match_मोजणी_Overflowp = nullptr;
        }

        परत sigs;
    }

    // Phase 1: Resolve combinational Links via fixed-point propagation.
    शून्य resolveLinks(LowerCtx& c, Ast_गाठ_अभिव्यक्ती* triggerExprp, स्थिरांक तार्किक मोजणी_Matches) {
        // datap() was freshly allocated in lower() -- all state signals start null.
        c.graph.m_startVertexp->datap()->stateSigp = triggerExprp->clone_वृक्ष_Pure(असत्य);
        c.graph.m_startVertexp->datap()->state_मोजणी_Sigp
            = नवीन AstExtend{c.flp, triggerExprp->clone_वृक्ष_Pure(असत्य), m_u32DTypep->width()};
        साठी (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* स्थिरांक vtxp = static_cast<SvaStateVertex*>(&vtxr);
            जर (vtxp->datap()->stateVarp) {
                vtxp->datap()->stateSigp
                    = नवीन AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
                vtxp->datap()->state_मोजणी_Sigp = नवीन AstExtend{
                    c.flp, नवीन AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ},
                    m_u32DTypep->width()};
            } अन्यथा जर (vtxp->datap()->delayRingVarp) {
                जर (vtxp->m_isFixedDelayRing) {
                    // state_count = wrapped ? head : 0; state = state_count != 0;
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक countp = delay_Ring_Output_मोजणी(c.flp, vtxp);
                    vtxp->datap()->stateSigp = नवीन AstNeq{c.flp, countp->clone_वृक्ष_Pure(असत्य),
                                                          newTypedConstp(c.flp, m_u32DTypep, 0)};
                    vtxp->datap()->state_मोजणी_Sigp = countp;
                } अन्यथा {
                    vtxp->datap()->stateSigp
                        = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_मोजणी_Varp);
                    vtxp->datap()->state_मोजणी_Sigp = नवीन AstVarRef{
                        c.flp, vtxp->datap()->delay_Ring_Live_मोजणी_Varp, VAccess::READ};
                }
            }
        }
        // Fixed-point propagation along zero-delay (Link) edges. Rebuild each
        // derived signal from its incoming edges on every pass; appending the same
        // contributions repeatedly makes merge expressions grow exponentially.
        स्थिरांक माप_प्रकार passes = 2 * c.graph.m_graph.vertices().size() + 2;
        साठी (माप_प्रकार pass = 0; pass < passes; ++pass) {
            // Rebuild SAnd combiners once both sub-NFA terminals are available.
            साठी (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* स्थिरांक vtxp = static_cast<SvaStateVertex*>(&vtxr);
                जर (!vtxp->m_isAndCombiner) सुरूठेवा;
                // AndCombiner vertices always have both terminal pointers set.
                UASSERT_OBJ(vtxp->m_andLhsTermp && vtxp->m_andRhsTermp, vtxp,
                            "AndCombiner vertex missing LHS/RHS terminal");
                स्थिरांक SvaStateVertex* स्थिरांक lhsTermp = vtxp->m_andLhsTermp;
                स्थिरांक SvaStateVertex* स्थिरांक rhsTermp = vtxp->m_andRhsTermp;
                जर (!lhsTermp->datap()->stateSigp || !rhsTermp->datap()->stateSigp) सुरूठेवा;
                Ast_गाठ_अभिव्यक्ती* स्थिरांक matchLp
                    = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, vtxp->m_andLhsCondp);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक matchRp
                    = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, vtxp->m_andRhsCondp);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक doneLOrp = नवीन AstLogOr{
                    c.flp, नवीन AstVarRef{c.flp, vtxp->datap()->doneLVarp, VAccess::READ}, matchLp};
                Ast_गाठ_अभिव्यक्ती* स्थिरांक doneROrp = नवीन AstLogOr{
                    c.flp, नवीन AstVarRef{c.flp, vtxp->datap()->doneRVarp, VAccess::READ}, matchRp};
                Ast_गाठ_अभिव्यक्ती* स्थिरांक bothp = नवीन AstLogAnd{c.flp, doneLOrp, doneROrp};
                Ast_गाठ_अभिव्यक्ती* स्थिरांक oneNowp = नवीन AstLogOr{c.flp, matchLp->clone_वृक्ष_Pure(असत्य),
                                                          matchRp->clone_वृक्ष_Pure(असत्य)};
                जर (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_वृक्ष(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = नवीन AstLogAnd{c.flp, bothp, oneNowp};
                जर (vtxp->datap()->state_मोजणी_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_मोजणी_Sigp->delete_वृक्ष(),
                                   vtxp->datap()->state_मोजणी_Sigp);
                }
                vtxp->datap()->state_मोजणी_Sigp = नवीन AstExtend{
                    c.flp, vtxp->datap()->stateSigp->clone_वृक्ष_Pure(असत्य), m_u32DTypep->width()};
            }

            साठी (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* स्थिरांक vtxp = static_cast<SvaStateVertex*>(&vtxr);
                जर (vtxp == c.graph.m_startVertexp || vtxp->datap()->stateVarp
                    || vtxp->datap()->delayRingVarp || vtxp->m_isAndCombiner || vtxp->m_isMatch
                    || vtxp->m_isRejectSink) {
                    सुरूठेवा;
                }
                Ast_गाठ_अभिव्यक्ती* nextStatep = nullptr;
                Ast_गाठ_अभिव्यक्ती* nextStateCountp = nullptr;
                Ast_गाठ_अभिव्यक्ती* next_State_मोजणी_Overflowp = nullptr;
                साठी (स्थिरांक V3GraphEdge& er : vtxp->inEdges()) {
                    स्थिरांक SvaTransEdge& te = static_cast<स्थिरांक SvaTransEdge&>(er);
                    स्थिरांक SvaStateVertex* स्थिरांक fromVtxp = te.fromVtxp();
                    जर (!fromVtxp->datap()->stateSigp) सुरूठेवा;
                    UASSERT_OBJ(fromVtxp->datap()->state_मोजणी_Sigp, fromVtxp,
                                "Link source missing stateCountSig");
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक contributionp = andCond(
                        c.flp, fromVtxp->datap()->stateSigp->clone_वृक्ष_Pure(असत्य), te.m_condp);
                    nextStatep = orExprs(c.flp, nextStatep, contributionp);
                    Ast_गाठ_अभिव्यक्ती* मोजणी_Contributionp
                        = fromVtxp->datap()->state_मोजणी_Sigp->clone_वृक्ष_Pure(असत्य);
                    Ast_गाठ_अभिव्यक्ती* मोजणी_Contribution_Overflowp
                        = मोजणी_Matches && fromVtxp->datap()->state_मोजणी_Overflow_Sigp
                              ? fromVtxp->datap()->state_मोजणी_Overflow_Sigp->clone_वृक्ष_Pure(असत्य)
                              : nullptr;
                    जर (te.m_condp) {
                        Ast_गाठ_अभिव्यक्ती* स्थिरांक enablep = te.m_condp->clone_वृक्ष_Pure(असत्य);
                        मोजणी_Contributionp
                            = मोजणी_Matches
                                  ? gateCheckedCountp(c.flp, मोजणी_Contributionp,
                                                      मोजणी_Contribution_Overflowp, enablep)
                                  : addCountp(c.flp, nullptr, मोजणी_Contributionp, enablep);
                    }
                    nextStateCountp
                        = मोजणी_Matches
                              ? addCheckedCountp(c.flp, nextStateCountp, मोजणी_Contributionp,
                                                 next_State_मोजणी_Overflowp,
                                                 मोजणी_Contribution_Overflowp)
                              : addCountp(c.flp, nextStateCountp, मोजणी_Contributionp);
                }
                जर (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_वृक्ष(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = nextStatep;
                जर (vtxp->datap()->state_मोजणी_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_मोजणी_Sigp->delete_वृक्ष(),
                                   vtxp->datap()->state_मोजणी_Sigp);
                }
                vtxp->datap()->state_मोजणी_Sigp = nextStateCountp;
                जर (vtxp->datap()->state_मोजणी_Overflow_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_मोजणी_Overflow_Sigp->delete_वृक्ष(),
                                   vtxp->datap()->state_मोजणी_Overflow_Sigp);
                }
                vtxp->datap()->state_मोजणी_Overflow_Sigp = next_State_मोजणी_Overflowp;
            }
        }
    }

सार्वजनिक:
    // Combine terminal/reject signals into final output expression.
    स्थिर Ast_गाठ_अभिव्यक्ती* assembleResult(Ast_गाठ_Cover_Or_Assert* स्थिरांक assertp, स्थिरांक तार्किक negated,
                                       Ast_गाठ_अभिव्यक्ती* स्थिरांक matchCondp, स्थिरांक SignalSet& sigs,
                                       Ast_गाठ_अभिव्यक्ती** स्थिरांक outMatchpp) {
        File_ओळ* स्थिरांक flp = assertp->fileline();
        स्थिरांक तार्किक isCover = VN_IS(assertp, Cover);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक terminalActivep = sigs.terminalActivep;
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rejectBasep = sigs.rejectBasep;
        Ast_गाठ_अभिव्यक्ती* स्थिरांक throughoutRejectp = sigs.throughoutRejectp;
        Ast_गाठ_अभिव्यक्ती* स्थिरांक requiredStepRejectp = sigs.requiredStepRejectp;
        // Property negation (IEEE 1800-2023 16.12.1 `not`): invert match/reject.
        जर (negated) {
            जर (isCover) {
                जर (terminalActivep)
                    VL_DO_DANGLING(terminalActivep->delete_वृक्ष(), terminalActivep);
                Ast_गाठ_अभिव्यक्ती* negRejectp = nullptr;
                जर (matchCondp && rejectBasep) {
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक sampledCondp = sampled(matchCondp->clone_वृक्ष_Pure(असत्य));
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक notCondp = नवीन AstLogNot{flp, sampledCondp};
                    negRejectp = नवीन AstLogAnd{flp, rejectBasep, notCondp};
                } अन्यथा जर (rejectBasep) {
                    VL_DO_DANGLING(rejectBasep->delete_वृक्ष(), rejectBasep);
                }
                जर (throughoutRejectp) negRejectp = orExprs(flp, negRejectp, throughoutRejectp);
                जर (requiredStepRejectp)
                    negRejectp = orExprs(flp, negRejectp, requiredStepRejectp);
                परत negRejectp ? negRejectp : नवीन AstConst{flp, AstConst::BitFalse{}};
            }
            // Negated assert/assume: output = !match.
            Ast_गाठ_अभिव्यक्ती* matchp = terminalActivep;
            जर (matchCondp) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक sampledCondp = sampled(matchCondp->clone_वृक्ष_Pure(असत्य));
                matchp = नवीन AstLogAnd{flp, matchp, sampledCondp};
            }
            जर (outMatchpp) {
                Ast_गाठ_अभिव्यक्ती* notPMatchp = nullptr;
                जर (matchCondp && rejectBasep) {
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक sampledCondp = sampled(matchCondp->clone_वृक्ष_Pure(असत्य));
                    notPMatchp = नवीन AstLogAnd{flp, rejectBasep->clone_वृक्ष_Pure(असत्य),
                                               नवीन AstLogNot{flp, sampledCondp}};
                } अन्यथा जर (rejectBasep) {
                    notPMatchp = rejectBasep->clone_वृक्ष_Pure(असत्य);
                }
                जर (throughoutRejectp)
                    notPMatchp = orExprs(flp, notPMatchp, throughoutRejectp->clone_वृक्ष_Pure(असत्य));
                जर (requiredStepRejectp)
                    notPMatchp
                        = orExprs(flp, notPMatchp, requiredStepRejectp->clone_वृक्ष_Pure(असत्य));
                *outMatchpp = notPMatchp;
            }
            जर (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_वृक्ष(), throughoutRejectp);
            जर (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_वृक्ष(), rejectBasep);
            जर (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_वृक्ष(), requiredStepRejectp);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक resultExprp = नवीन AstLogNot{flp, matchp};
            परत resultExprp;
        }
        जर (isCover) {
            जर (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_वृक्ष(), throughoutRejectp);
            जर (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_वृक्ष(), rejectBasep);
            जर (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_वृक्ष(), requiredStepRejectp);
            जर (matchCondp) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक sampledCondp = sampled(matchCondp->clone_वृक्ष_Pure(असत्य));
                परत नवीन AstLogAnd{flp, terminalActivep, sampledCondp};
            }
            परत terminalActivep;
        }
        // Assert/assume: output = !reject
        Ast_गाठ_अभिव्यक्ती* rejectp = nullptr;
        जर (matchCondp && rejectBasep) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक sampledCondp = sampled(matchCondp->clone_वृक्ष_Pure(असत्य));
            rejectp = नवीन AstLogAnd{flp, rejectBasep, नवीन AstLogNot{flp, sampledCondp}};
        } अन्यथा जर (rejectBasep) {
            VL_DO_DANGLING(rejectBasep->delete_वृक्ष(), rejectBasep);
        }
        जर (outMatchpp) {
            Ast_गाठ_अभिव्यक्ती* matchExprp = terminalActivep->clone_वृक्ष_Pure(असत्य);
            जर (matchCondp) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक sp = sampled(matchCondp->clone_वृक्ष_Pure(असत्य));
                matchExprp = नवीन AstLogAnd{flp, matchExprp, sp};
            }
            *outMatchpp = matchExprp;
        }
        जर (terminalActivep) VL_DO_DANGLING(terminalActivep->delete_वृक्ष(), terminalActivep);
        जर (throughoutRejectp) rejectp = orExprs(flp, rejectp, throughoutRejectp);
        जर (requiredStepRejectp) rejectp = orExprs(flp, rejectp, requiredStepRejectp);
        जर (!rejectp) परत नवीन AstConst{flp, AstConst::BitTrue{}};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक resultExprp = नवीन AstLogNot{flp, rejectp};
        परत resultExprp;
    }

    explicit SvaNfaLowering(Ast_गाठ_Module* modp)
        : m_modp{modp}
        , m_u32DTypep{modp->findBasicDType(VBasicDTypeKwd::UINT32)} {}
    ~SvaNfaLowering() {
        V3Stats::addStatSum("Assertions, NFA delay ring edge visits", m_statDelayRingEdgeVisits);
    }

    // Lower NFA graph to synthesizable AstAlways blocks and raw result signals.
    // Links are combinational; Edges are registered (NBA).
    SignalSet lower(Ast_गाठ_Cover_Or_Assert* स्थिरांक assertp, SvaGraph& graph,
                    Ast_Sen_वृक्ष* स्थिरांक senTreep, Ast_गाठ_अभिव्यक्ती* स्थिरांक matchCondp,
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक disableExprp, स्थिरांक तार्किक need_Thread_Fail_मोजणी,
                    स्थिरांक तार्किक need_Throughout_Thread_Fail_मोजणी) {
        File_ओळ* स्थिरांक flp = assertp->fileline();
        AstCover* स्थिरांक coverp = VN_CAST(assertp, Cover);
        स्थिरांक तार्किक isSeqEvent = coverp && coverp->isSeqEvent();
        स्थिरांक VAssertType assertType
            = isSeqEvent ? VAssertType{VAssertType::INTERNAL} : assertp->userType();
        स्थिरांक VAssertDirectiveType directiveType
            = isSeqEvent ? VAssertDirectiveType{VAssertDirectiveType::INTERNAL}
                         : assertp->directive();
        स्थिरांक std::string baseName = m_names.get("");

        // Number vertices with sequential colors for array indexing.
        पूर्णांक N = 0;
        साठी (V3GraphVertex& vtxr : graph.m_graph.vertices()) { vtxr.color(N++); }
        std::vector<SvaStateVertex*> vtx(N, nullptr);
        साठी (V3GraphVertex& vtxr : graph.m_graph.vertices()) {
            vtx[vtxr.color()] = static_cast<SvaStateVertex*>(&vtxr);
        }

        // Allocate per-vertex lowering data (stored via V3GraphVertex::userp()).
        std::vector<SvaVertexData> vertexData(N);
        साठी (पूर्णांक i = 0; i < N; ++i) vtx[i]->userp(&vertexData[i]);

        // Identify registered vertices (targets of clocked edges).
        साठी (पूर्णांक i = 0; i < N; ++i) {
            साठी (स्थिरांक V3GraphEdge& edger : vtx[i]->outEdges()) {
                स्थिरांक SvaTransEdge& tedger = static_cast<स्थिरांक SvaTransEdge&>(edger);
                स्थिरांक पूर्णांक toIdx = tedger.toVtxp()->color();
                जर (tedger.m_consumesCycle && tedger.toVtxp() != graph.m_matchVertexp
                    && !tedger.toVtxp()->m_isRejectSink) {
                    vtx[toIdx]->datap()->needsReg = सत्य;
                }
            }
        }

        // A packed range ring can emit several live endpoints without needing per-slot counts.
        // Widen only a ring whose input can already carry multiplicity.
        std::vector<तार्किक> stateMayHaveMultiplicity(N);
        std::vector<तार्किक> ringNeedsMultiplicity(N);
        std::vector<पूर्णांक> multiplicityWork;
        स्थिरांक तार्किक मोजणी_Matches = coverp && coverp->isCoverSeq() && !isSeqEvent;
        जर (मोजणी_Matches) {
            साठी (पूर्णांक i = 0; i < N; ++i) {
                स्थिरांक तार्किक multipleInputs = vtx[i]->inEdges().size() > 1;
                स्थिरांक तार्किक isRing = vtx[i]->m_delayRingSize;
                ringNeedsMultiplicity[i] = isRing && multipleInputs;
                स्थिरांक तार्किक startsMultiplicity = isRing
                                                    ? !vtx[i]->m_isFixedDelayRing
                                                    : multipleInputs && !vtx[i]->datap()->needsReg;
                जर (startsMultiplicity) {
                    stateMayHaveMultiplicity[i] = सत्य;
                    multiplicityWork.ढकला_back(i);
                }
            }
            साठी (माप_प्रकार wi = 0; wi < multiplicityWork.size(); ++wi) {
                साठी (स्थिरांक V3GraphEdge& edger : vtx[multiplicityWork[wi]]->outEdges()) {
                    स्थिरांक SvaTransEdge& tedger = static_cast<स्थिरांक SvaTransEdge&>(edger);
                    स्थिरांक पूर्णांक ti = tedger.toVtxp()->color();
                    स्थिरांक तार्किक isRing = vtx[ti]->m_delayRingSize;
                    जर (isRing) ringNeedsMultiplicity[ti] = सत्य;
                    // Match and reject sinks have no outgoing edges, so they stop naturally.
                    स्थिरांक तार्किक stopsMultiplicity = !isRing && vtx[ti]->datap()->needsReg;
                    जर (stateMayHaveMultiplicity[ti] || stopsMultiplicity) { सुरूठेवा; }
                    stateMayHaveMultiplicity[ti] = सत्य;
                    multiplicityWork.ढकला_back(ti);
                }
            }
        }

        स्थिरांक तार्किक asyncDisable = isAsyncDisable(disableExprp);
        // Registers cleared on a rising disable are also written by their clocked block.
        File_ओळ* स्थिरांक regFlp = asyncDisable ? नवीन File_ओळ{flp} : flp;
        जर (asyncDisable) regFlp->warnOff(V3ErrorCode::MULTIDRIVEN, सत्य);

        AstVar* स्थिरांक killVarp
            = नवीन AstVar{flp, VVarType::MODULETEMP, baseName + "__kill", m_u32DTypep};
        killVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(killVarp);
        साठी (पूर्णांक i = 0; i < N; ++i) {
            जर (vtx[i]->m_isAndCombiner) {
                स्थिरांक std::string base = baseName + "__a" + std::to_string(i);
                AstVar* स्थिरांक lp = नवीन AstVar{regFlp, VVarType::MODULETEMP, base + "_doneL",
                                              m_modp->findBitDType()};
                lp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(lp);
                vtx[i]->datap()->doneLVarp = lp;
                AstVar* स्थिरांक rp = नवीन AstVar{regFlp, VVarType::MODULETEMP, base + "_doneR",
                                              m_modp->findBitDType()};
                rp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(rp);
                vtx[i]->datap()->doneRVarp = rp;
                सुरूठेवा;
            }
            जर (vtx[i]->m_delayRingSize) {
                स्थिरांक std::string base = baseName + "__d" + std::to_string(i);
                स्थिरांक तार्किक storesCounts = मोजणी_Matches && ringNeedsMultiplicity[i];
                Ast_गाठ_DType* ringDTypep = nullptr;
                जर (storesCounts) {
                    // int unsigned ring [size];
                    ringDTypep = नवीन AstUnpackArrayDType{
                        flp, m_u32DTypep,
                        नवीन AstRange{flp, static_cast<पूर्णांक>(vtx[i]->m_delayRingSize - 1), 0}};
                    v3Global.rootp()->typeTablep()->addTypesp(ringDTypep);
                } अन्यथा {
                    // bit [size-1:0] ring;
                    ringDTypep = m_modp->findBitDType(vtx[i]->m_delayRingSize,
                                                      vtx[i]->m_delayRingSize, VSigning::UNSIGNED);
                }
                AstVar* स्थिरांक ringp
                    = नवीन AstVar{flp, VVarType::MODULETEMP, base + "_ring", ringDTypep};
                ringp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(ringp);
                vtx[i]->datap()->delayRingVarp = ringp;
                vtx[i]->datap()->delayRingCounts = storesCounts;
                AstVar* स्थिरांक headp
                    = नवीन AstVar{flp, VVarType::MODULETEMP, base + "_head",
                                 storesCounts ? m_u32DTypep : m_modp->findBitDType()};
                headp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(headp);
                vtx[i]->datap()->delayRingHeadVarp = headp;
                // int unsigned idx;
                AstVar* स्थिरांक idxp
                    = नवीन AstVar{regFlp, VVarType::MODULETEMP, base + "_idx", m_u32DTypep};
                idxp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(idxp);
                vtx[i]->datap()->delayRingIdxVarp = idxp;
                // int unsigned live_count;
                AstVar* स्थिरांक live_मोजणी_Varp
                    = नवीन AstVar{regFlp, VVarType::MODULETEMP, base + "_liveCount", m_u32DTypep};
                live_मोजणी_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(live_मोजणी_Varp);
                vtx[i]->datap()->delay_Ring_Live_मोजणी_Varp = live_मोजणी_Varp;
                AstVar* स्थिरांक wrappedp = नवीन AstVar{regFlp, VVarType::MODULETEMP,
                                                    base + "_wrapped", m_modp->findBitDType()};
                wrappedp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(wrappedp);
                vtx[i]->datap()->delayRingWrappedVarp = wrappedp;
                सुरूठेवा;
            }
            जर (!vtx[i]->datap()->needsReg) सुरूठेवा;
            जर (vtx[i] == graph.m_startVertexp || vtx[i]->m_isMatch) सुरूठेवा;
            स्थिरांक std::string varName = baseName + "__s" + std::to_string(i);
            AstVar* स्थिरांक varp
                = नवीन AstVar{regFlp, VVarType::MODULETEMP, varName, m_modp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(varp);
            vtx[i]->datap()->stateVarp = varp;
        }

        // Build lowering context for phase sub-functions.
        LowerCtx c{flp,        senTreep,      disableExprp, matchCondp,
                   assertType, directiveType, killVarp,     graph};

        // Phase 1: Resolve combinational Links via fixed-point propagation.
        Ast_गाठ_अभिव्यक्ती* स्थिरांक triggerExprp
            = isSeqEvent ? नवीन AstConst{flp, AstConst::BitTrue{}}
                         : assertOnCond(flp, assertp->userType(), assertp->directive());
        resolveLinks(c, triggerExprp, मोजणी_Matches);
        VL_DO_DANGLING(triggerExprp->delete_वृक्ष(), triggerExprp);

        // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
        emitStateRegisterNba(c);
        emitDelayRingUpdates(c, मोजणी_Matches);
        emitAndCombinerDoneLatchNba(c);
        emitKillAckNba(c);

        // Phase 3/3a/3b: Compute terminal match/reject signals (cleans up stateSig).
        स्थिरांक SignalSet sigs
            = computeSignals(c, need_Thread_Fail_मोजणी, need_Throughout_Thread_Fail_मोजणी, मोजणी_Matches);

        // Strong s_always[m:n] end-of-simulation liveness: if any in-window state
        // is still set at $finish, the universal-quantifier window never completed
        // (IEEE 1800-2023 16.12.11 strong semantics). Fire the assertion failure
        // from a final block; V3Assert turns the DT_ERROR display into the standard
        // "Assertion failed in %m" message.
        // pending = |{strong state registers, strong delay live counts != 0};
        Ast_गाठ_अभिव्यक्ती* pendingp = nullptr;
        साठी (पूर्णांक i = 0; i < N; ++i) {
            जर (!vtx[i]->m_strongPending) सुरूठेवा;
            Ast_गाठ_अभिव्यक्ती* pendingExprp = nullptr;
            जर (vtx[i]->datap()->stateVarp) {
                pendingExprp = नवीन AstVarRef{flp, vtx[i]->datap()->stateVarp, VAccess::READ};
            } अन्यथा जर (vtx[i]->m_strongAlwaysRing) {
                pendingExprp
                    = delayRingHasLiveThreadsp(flp, vtx[i]->datap()->delay_Ring_Live_मोजणी_Varp);
            } अन्यथा {
                सुरूठेवा;
            }
            जर (!pendingp) {
                pendingp = pendingExprp;
            } अन्यथा {
                pendingp = नवीन AstLogOr{flp, pendingp, pendingExprp};
            }
        }
        जर (pendingp) {
            AstCExpr* स्थिरांक assertOnp
                = नवीन AstCExpr{flp, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()", 1};
            Ast_गाठ_अभिव्यक्ती* स्थिरांक condp = नवीन AstLogAnd{flp, assertOnp, pendingp};
            AstDisplay* स्थिरांक dispp
                = नवीन AstDisplay{flp, VDisplayType::DT_ERROR, "", nullptr, nullptr};
            dispp->fmtp()->timeunit(m_modp->timeunit());
            Ast_गाठ_Stmt* स्थिरांक firep = dispp;
            जर (v3Global.opt.stopFail()) firep->addNext(नवीन AstStop{flp, असत्य});
            m_modp->addStmtsp(नवीन AstFinal{flp, नवीन AstIf{flp, condp, firep}});
        }

        // Clear userp on every vertex before vertexData is destroyed.
        साठी (पूर्णांक i = 0; i < N; ++i) vtx[i]->userp(nullptr);
        परत sigs;
    }
};

}  // namespace

//######################################################################
// Top-level visitor

वर्ग AssertNfaVisitor final : सार्वजनिक VNVisitor {
    // STATE
    Ast_गाठ_Module* m_modp = nullptr;  // Current module being processed
    AstClocking* m_defaultClockingp = nullptr;  // Default clocking
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff
    SvaNfaLowering* m_loweringp = nullptr;  // NFA-to-hardware lowering engine
    Ast_Sen_वृक्ष* m_sampled_मूल्य_Clockp = nullptr;  // Inherited clock during scoped attachment
    V3UniqueNames m_propVarNames{"__Vpropvar"};  // Property-local variable names
    V3UniqueNames m_propTempNames{"__VnfaSampled"};  // Hoisted $sampled(propp) temps
    V3UniqueNames m_fail_मोजणी_Names{"__VnfaRemainingFailCount"};  // Fail replay counter names
    std::set<स्थिरांक AstProperty*> m_inliningProps;  // Recursion guard for inlineNamedProperty

    ढाँचा <typename T_गाठ>
    शून्य visit_Sampled_मूल्य(T_गाठ* स्थिरांक nodep) {
        जर (m_sampled_मूल्य_Clockp && !nodep->sentreep()) {
            nodep->sentreep(m_sampled_मूल्य_Clockp->clone_वृक्ष(सत्य));
        }
        iterateChildren(nodep);
    }

    // Wire match vertex and mid-window sources for a successful NFA build.
    स्थिर शून्य wireMatchAndMidSources(SvaGraph& graph, स्थिरांक BuildResult& result, File_ओळ* flp) {
        graph.createMatchVertex();
        // Skip the main term Link when midSources already cover every
        // end-of-match (cover_sequence path); otherwise the per-mid extraction
        // double-counts via the merge vertex.
        जर (!result.termIsMidMerge) { graph.addLink(result.termVertexp, graph.m_matchVertexp); }
        साठी (SvaStateVertex* srcVtxp : result.midSources) {
            Ast_गाठ_अभिव्यक्ती* condp = nullptr;
            साठी (Ast_गाठ_अभिव्यक्ती* स्थिरांक tc : srcVtxp->m_throughoutConds) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक tcClone = tc->clone_वृक्ष_Pure(असत्य);
                condp = condp ? नवीन AstLogAnd{flp, condp, tcClone} : tcClone;
            }
            graph.addLink(srcVtxp, graph.m_matchVertexp, condp);
            srcVtxp->m_isUnbounded = सत्य;
        }
    }

    स्थिर Ast_गाठ_अभिव्यक्ती* getSequenceBodyExprp(स्थिरांक AstSequence* seqp) {
        Ast_गाठ* bodyp = seqp->stmtsp();
        पर्यंत (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        परत VN_CAST(bodyp, गाठ_अभिव्यक्ती);
    }

    स्थिर AstPropSpec* getPropertySpecp(स्थिरांक AstProperty* propp) {
        Ast_गाठ* stmtp = propp->stmtsp();
        // V3LinkParse emits InitialStaticStmt for property-local variable
        // initialisers; the InitialAutomaticStmt variant only appears for
        // task/function-scope automatic lifetime, not properties.
        पर्यंत (stmtp
               && (VN_IS(stmtp, Var) || VN_IS(stmtp, InitialStaticStmt)
                   || VN_IS(stmtp, InitialAutomaticStmt))) {  // LCOV_EXCL_LINE
            stmtp = stmtp->nextp();
        }
        परत VN_CAST(stmtp, PropSpec);
    }

    शून्य inlineNamedProperty(AstPropSpec* outerSpecp, AstFuncRef* funcrefp,
                             स्थिरांक AstProperty* propyp) {
        // Recursion guard: IEEE 1800-2023 16.12.1 forbids recursive properties.
        // V3Width emits "Recursive property call" for direct recursion before this
        // pass runs; this catches any nested-inlining cycle that slips past.
        जर (!m_inliningProps.emplace(propyp).second) {
            funcrefp->v3error("Illegal recursive property reference");  // LCOV_EXCL_LINE
            परत;  // LCOV_EXCL_LINE
        }
        संरचना Guard final {
            std::set<स्थिरांक AstProperty*>& setr;
            स्थिरांक AstProperty* keyp;
            ~Guard() { setr.erase(keyp); }
        } guard{m_inliningProps, propyp};
        AstPropSpec* propSpecp = getPropertySpecp(propyp);
        UASSERT_OBJ(propSpecp, funcrefp, "Property has no body PropSpec");
        propSpecp = propSpecp->clone_वृक्ष(असत्य);

        स्थिरांक V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propyp->stmtsp());
        std::unordered_map<स्थिरांक AstVar*, Ast_गाठ_अभिव्यक्ती*> portMap;
        साठी (स्थिरांक स्वतः& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables to module-level temps (IEEE 16.10).
        std::unordered_map<स्थिरांक AstVar*, AstVar*> localVarMap;
        साठी (Ast_गाठ* stmtp = propyp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            जर (AstVar* स्थिरांक varp = VN_CAST(stmtp, Var)) {
                जर (!varp->isIO()) {
                    स्थिरांक string newName = m_propVarNames.get(varp);
                    AstVar* स्थिरांक newVarp = नवीन AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        propSpecp->foreach([&](AstVarRef* refp) {
            स्थिरांक स्वतः portIt = portMap.find(refp->varp());
            जर (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_वृक्ष(असत्य));
                VL_DO_DANGLING(ढकला_Deletep(refp), refp);
                परत;
            }
            स्थिरांक स्वतः localIt = localVarMap.find(refp->varp());
            जर (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        साठी (स्थिरांक स्वतः& tconnect : tconnects) {
            ढकला_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propSpecp, funcrefp);

        // Merge disable iff (IEEE 1800-2023 16.12.1)
        जर (outerSpecp->disablep() && propSpecp->disablep()) {
            outerSpecp->v3error("disable iff expression before property call "
                                "and in its body is not legal");
            ढकला_Deletep(propSpecp->disablep()->unlinkFrBack());
        }
        जर (outerSpecp->disablep()) {
            propSpecp->disablep(outerSpecp->disablep()->unlinkFrBack());
        }

        जर (outerSpecp->sensesp() && propSpecp->sensesp()) {
            outerSpecp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Clock event before property call and in its body");
            ढकला_Deletep(propSpecp->sensesp()->unlinkFrBack());
        }
        जर (outerSpecp->sensesp()) {
            AstSenItem* स्थिरांक sensesp = outerSpecp->sensesp();
            sensesp->unlinkFrBack();
            propSpecp->sensesp(sensesp);
        }

        outerSpecp->replaceWith(propSpecp);
        VL_DO_DANGLING(ढकला_Deletep(outerSpecp), outerSpecp);
    }

    शून्य inlineSequenceRef(AstFuncRef* funcrefp, AstSequence* seqp) {
        Ast_गाठ_अभिव्यक्ती* स्थिरांक bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        Ast_गाठ_अभिव्यक्ती* स्थिरांक clonedp = bodyExprp->clone_वृक्ष(असत्य);

        स्थिरांक V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<स्थिरांक AstVar*, Ast_गाठ_अभिव्यक्ती*> portMap;
        साठी (स्थिरांक स्वतः& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            स्थिरांक स्वतः it = portMap.find(refp->varp());
            जर (it != portMap.end()) {
                refp->replaceWith(it->second->clone_वृक्ष(असत्य));
                VL_DO_DANGLING(ढकला_Deletep(refp), refp);
            }
        });
        साठी (स्थिरांक स्वतः& tconnect : tconnects) {
            ढकला_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ढकला_Deletep(funcrefp), funcrefp);
        // Clear referenced flag so V3AssertPre cleanup does not emit
        // spurious UNSUPPORTED for sequences that were already inlined here.
        seqp->isReferenced(असत्य);
    }

    // Must run before hasMultiCycleExpr() so NFA sees sequence bodies.
    शून्य inlineAllSequenceRefs(Ast_गाठ* rootp) {
        तार्किक changed = सत्य;
        पर्यंत (changed) {
            changed = असत्य;
            rootp->foreach([&](AstFuncRef* funcrefp) {
                जर (changed) परत;
                जर (AstSequence* स्थिरांक seqp = VN_CAST(funcrefp->taskp(), Sequence)) {
                    inlineSequenceRef(funcrefp, seqp);
                    changed = सत्य;
                }
            });
        }
    }

    स्थिर तार्किक has_Multi_Cycle_अभिव्यक्ती(स्थिरांक Ast_गाठ* nodep) {
        परत nodep->exists([](स्थिरांक Ast_गाठ* np) {
            जर (स्थिरांक स्वतः* स्थिरांक ep = VN_CAST(np, गाठ_अभिव्यक्ती)) परत ep->isMultiCycleSva();
            परत असत्य;
        });
    }

    स्थिर VPropStrength effectiveAssertPropStrength(स्थिरांक AstPropSpec* स्थिरांक propSpecp) {
        जर (propSpecp->propStrength() != VPropStrength::DEFAULT) परत propSpecp->propStrength();
        परत propSpecp->fileline()->language() <= V3LangCode::L1800_2005 ? VPropStrength::STRONG
                                                                           : VPropStrength::WEAK;
    }

    // Bare `assert property (p until q)` with boolean operands stays on
    // V3AssertPre's AstLoop lowering, which preserves per-attempt action-block
    // firings that this NFA's single-bit aggregated state cannot. Strong bare
    // forms are also lowered there. NFA still owns sequence operands and any
    // embedding inside a multi-cycle context (implication consequent, or/and
    // operands, etc.).
    स्थिर तार्किक is_Bare_शीर्ष_Level_Until(Ast_गाठ* propp) {
        Ast_गाठ* p = propp;
        जर (AstPropSpec* स्थिरांक specp = VN_CAST(p, PropSpec)) p = specp->propp();
        पर्यंत (AstLogNot* स्थिरांक notp = VN_CAST(p, LogNot)) p = notp->lhsp();
        AstUntil* स्थिरांक untilp = VN_CAST(p, Until);
        जर (!untilp) परत असत्य;
        स्थिरांक स्वतः hasSeq = [](स्थिरांक Ast_गाठ_अभिव्यक्ती* ep) {
            परत ep->exists([](स्थिरांक Ast_गाठ_अभिव्यक्ती* np) { परत np->isMultiCycleSva(); });
        };
        जर (hasSeq(untilp->lhsp()) || hasSeq(untilp->rhsp())) परत असत्य;
        परत सत्य;
    }

    संरचना PropertyParts final {
        Ast_गाठ_अभिव्यक्ती* triggerExprp = nullptr;
        Ast_गाठ_अभिव्यक्ती* seqExprp = nullptr;
        तार्किक isOverlapped = सत्य;
        तार्किक hasImplication = असत्य;
        तार्किक isFollowedBy = असत्य;  // True for #-# / #=# (non-vacuous-fail on antecedent miss)
    };

    स्थिर PropertyParts decomposeProperty(Ast_गाठ* propp) {
        PropertyParts parts;
        जर (AstPropSpec* स्थिरांक specp = VN_CAST(propp, PropSpec)) { propp = specp->propp(); }
        जर (AstImplication* स्थिरांक implp = VN_CAST(propp, Implication)) {
            parts.hasImplication = सत्य;
            parts.isOverlapped = implp->isOverlapped();
            parts.isFollowedBy = implp->isFollowedBy();
            parts.triggerExprp = implp->lhsp();
            parts.seqExprp = implp->rhsp();
        } अन्यथा जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = VN_CAST(propp, गाठ_अभिव्यक्ती)) {
            parts.triggerExprp = nullptr;
            parts.seqExprp = exprp;
        }
        परत parts;
    }

    स्थिर तार्किक canSplitImplicationPassActions(स्थिरांक PropertyParts& parts) {
        UASSERT(parts.hasImplication,
                "Implication pass action split requested without implication");
        UASSERT(parts.triggerExprp, "Implication pass action split requested without trigger");
        // Direct vacuous/nonvacuous classification uses the antecedent value in the current
        // assertion attempt. Leave delayed antecedents on the existing NFA pass path.
        परत !has_Multi_Cycle_अभिव्यक्ती(parts.triggerExprp);
    }

    स्थिर शून्य splitImplicationPassActions(AstAssert* assertp, स्थिरांक PropertyParts& parts,
                                            Ast_गाठ_अभिव्यक्ती* nonvacuousMatchp) {
        File_ओळ* स्थिरांक flp = assertp->fileline();

        Ast_गाठ* स्थिरांक passsp = assertp->passsp()->unlinkFrBackWithNext();
        Ast_गाठ* splitsp = nullptr;

        जर (!parts.isFollowedBy) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक vacuousp
                = नवीन AstLogNot{flp, sampled(parts.triggerExprp->clone_वृक्ष_Pure(असत्य))};
            Ast_गाठ* स्थिरांक vacuousBodyp = passsp->clone_वृक्ष(असत्य);
            splitsp = newPassOnIf(flp, vacuousp, vacuousBodyp, assertp->userType(),
                                  assertp->directive(), /*vacuous=*/सत्य);
        }

        AstIf* स्थिरांक nonvacuousp
            = newPassOnIf(flp, nonvacuousMatchp, passsp, assertp->userType(), assertp->directive(),
                          /*vacuous=*/असत्य);
        splitsp = splitsp ? Ast_गाठ::addNext<Ast_गाठ, Ast_गाठ>(splitsp, nonvacuousp)
                          : static_cast<Ast_गाठ*>(nonvacuousp);
        assertp->addPasssp(splitsp);
    }

    // On a PropSpec-wrapped assertion whose NFA build failed with a semantic
    // error (errorEmitted), replace the body with a BitFalse const so later
    // passes see a well-formed AST. Returns true if replaced.
    शून्य replaceBodyOnBuildError(File_ओळ* flp, AstPropSpec* propSpecp, तार्किक errorEmitted) {
        जर (!errorEmitted) परत;
        Ast_गाठ* स्थिरांक innerPropp = propSpecp->propp();
        innerPropp->replaceWith(नवीन AstConst{flp, AstConst::BitFalse{}});
        VL_DO_DANGLING(ढकला_Deletep(innerPropp), innerPropp);
    }

    // Hoist a leading clocking event (IEEE 1800-2023 16.7):
    तार्किक hoistClockedSeq(AstPropSpec* specp) {
        पर्यंत (AstSClocked* स्थिरांक clockedp = VN_CAST(specp->propp(), SClocked)) {
            जर (specp->sensesp()) {
                clockedp->v3warn(E_UNSUPPORTED, "Unsupported: multiclocked sequence or property");
                replaceBodyOnBuildError(specp->fileline(), specp, सत्य);
                परत सत्य;
            }
            साठी (स्थिरांक AstSenItem* sp = clockedp->sensesp(); sp;
                 sp = VN_CAST(sp->nextp(), SenItem)) {
                जर (!sp->edgeType().anEdge()) {
                    clockedp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: non-edge clocking event on a sequence; "
                                     "use an edge such as @(posedge clk)");
                    replaceBodyOnBuildError(specp->fileline(), specp, सत्य);
                    परत सत्य;
                }
            }
            specp->sensesp(clockedp->sensesp()->unlinkFrBackWithNext());
            Ast_गाठ_अभिव्यक्ती* स्थिरांक bodyp = clockedp->exprp()->unlinkFrBack();
            clockedp->replaceWith(bodyp);
            VL_DO_DANGLING(ढकला_Deletep(clockedp), clockedp);
        }
        // A clocking event anywhere else in the sequence is not supported.
        स्थिरांक AstSClocked* nestedp = nullptr;
        specp->propp()->foreach([&](स्थिरांक AstSClocked* p) {
            जर (!nestedp) nestedp = p;
        });
        जर (nestedp) {
            nestedp->v3warn(E_UNSUPPORTED,
                            "Unsupported: clocking event inside sequence expression");
            replaceBodyOnBuildError(specp->fileline(), specp, सत्य);
            परत सत्य;
        }
        परत असत्य;
    }

    // Build the NFA graph for a property body, handling both the antecedent
    // |-> consequent and simple sequence cases. Returns the consequent/body
    // BuildResult (invalid on parse/build failure).
    BuildResult buildAssertionGraph(SvaNfaBuilder& builder, SvaGraph& graph, Ast_गाठ_अभिव्यक्ती* seqBodyp,
                                    स्थिरांक PropertyParts& parts, File_ओळ* flp) {
        जर (!parts.hasImplication) परत builder.build(seqBodyp);

        graph.m_startVertexp = graph.createStateVertex();
        परत builder.buildImplicationEdges(parts.triggerExprp, seqBodyp, graph.m_startVertexp,
                                             parts.isOverlapped, parts.isFollowedBy,
                                             parts.triggerExprp, flp);
    }

    // Install pass-action gating and replay simultaneous per-thread failures.
    शून्य attachActionHandlers(AstAssert* स्थिरांक assertp, Ast_गाठ_अभिव्यक्ती* matchExprp,
                              Ast_Sen_वृक्ष* स्थिरांक threadFailReplaySenTreep,
                              Ast_गाठ_अभिव्यक्ती* स्थिरांक threadFailCountp) {
        // Gate pass handler on match to prevent vacuous-pass firings.
        जर (matchExprp) {
            // needMatch implies passsp() was non-null when evaluated above;
            // lowering does not mutate the assert's pass-action between the
            // two reads, so passsp() is still non-null here.
            Ast_गाठ* passsp = assertp->passsp();
            UASSERT_OBJ(passsp, assertp, "needMatch set but passsp is null");
            passsp->unlinkFrBackWithNext();
            File_ओळ* स्थिरांक flp = assertp->fileline();
            AstIf* स्थिरांक ifp = नवीन AstIf{flp, matchExprp, passsp, nullptr};
            assertp->addPasssp(ifp);
            // Fail-handler prefix for overlapping instances (IEEE 16.12):
            // fires when reject=1 && match=1 in the same cycle.
            जर (assertp->failsp()) assertp->prependFailsp(ifp->clone_वृक्ष(असत्य));
        }

        जर (threadFailCountp) {
            UASSERT_OBJ(threadFailReplaySenTreep, assertp,
                        "Thread fail count missing sensitivity tree");
            Ast_गाठ* स्थिरांक failsp = assertp->failsp();
            File_ओळ* स्थिरांक flp = assertp->fileline();
            // IEEE 1800-2023 16.12 requires one action-block evaluation per failed
            // thread. AstAssert handles the first, so replay the rest here.
            AstVar* स्थिरांक remaining_Fail_मोजणी_Varp
                = नवीन AstVar{flp, VVarType::MODULETEMP, m_fail_मोजणी_Names.get(""),
                             m_modp->findBasicDType(VBasicDTypeKwd::UINT32)};
            remaining_Fail_मोजणी_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remaining_Fail_मोजणी_Varp);
            Ast_गाठ* स्थिरांक replayStmtsp = नवीन AstAssign{
                flp, नवीन AstVarRef{flp, remaining_Fail_मोजणी_Varp, VAccess::WRITE}, threadFailCountp};
            AstLoop* स्थिरांक replayLoopp = नवीन AstLoop{flp};
            replayLoopp->addStmtsp(नवीन AstLoopTest{
                flp, replayLoopp,
                नवीन AstGt{flp, नवीन AstVarRef{flp, remaining_Fail_मोजणी_Varp, VAccess::READ},
                          newTypedConstp(flp, remaining_Fail_मोजणी_Varp->dtypep(), 1)}});
            replayLoopp->addStmtsp(newIfAssertFailOn(failsp->clone_वृक्ष(सत्य), assertp->directive(),
                                                     assertp->userType()));
            AstSub* स्थिरांक decrementedFailCountp
                = नवीन AstSub{flp, नवीन AstVarRef{flp, remaining_Fail_मोजणी_Varp, VAccess::READ},
                             newTypedConstp(flp, remaining_Fail_मोजणी_Varp->dtypep(), 1)};
            replayLoopp->addStmtsp(
                नवीन AstAssign{flp, नवीन AstVarRef{flp, remaining_Fail_मोजणी_Varp, VAccess::WRITE},
                              decrementedFailCountp});
            replayStmtsp->addNext(replayLoopp);
            m_modp->addStmtsp(
                नवीन AstAlways{flp, VAlwaysKwd::ALWAYS, threadFailReplaySenTreep, replayStmtsp});
        }
    }

    // Replace one VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). No-op if refp is not in matchMap.
    शून्य substituteMatchItemRef(AstVarRef* refp, अचिन्हित K,
                                स्थिरांक std::unordered_map<स्थिरांक AstVar*, Ast_गाठ_अभिव्यक्ती*>& matchMap) {
        स्थिरांक स्वतः it = matchMap.find(refp->varp());
        जर (it == matchMap.end()) परत;
        Ast_गाठ_अभिव्यक्ती* newp = it->second->clone_वृक्ष_Pure(असत्य);
        जर (K > 0) {
            AstConst* स्थिरांक ticksp = नवीन AstConst{refp->fileline(), AstConst::Widthed_मूल्य{}, 32,
                                                  static_cast<uint32_t>(K)};
            AstPast* स्थिरांक pastp = नवीन AstPast{refp->fileline(), newp, ticksp, nullptr,
                                               /* propertyTiming */ सत्य};
            pastp->dtypeFrom(newp);
            newp = pastp;
        }
        refp->replaceWith(newp);
        VL_DO_DANGLING(ढकला_Deletep(refp), refp);
        परत;
    }

    // Recursively walk a consequent. Returns cycle length consumed and
    // substitutes each VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). Reports E_UNSUPPORTED on non-constant
    // delays or composite sequence operators and returns -1 on failure.
    पूर्णांक
    walk_Substitute_Match_वस्तुसंच(Ast_गाठ_अभिव्यक्ती* nodep, अचिन्हित K,
                             स्थिरांक std::unordered_map<स्थिरांक AstVar*, Ast_गाठ_अभिव्यक्ती*>& match_वस्तुसंच) {
        जर (AstSExpr* स्थिरांक sexprp = VN_CAST(nodep, SExpr)) {
            // IEEE 1800-2023 16.9.2: cycle_delay's lhsp is a constant_expression
            // and the delay form in a sequence is always `##N`, folded by
            // V3Const + V3Param before V3AssertNfa. Range form `##[m:n]` is the
            // only user-visible reject here.
            AstDelay* स्थिरांक delayp = VN_AS(sexprp->delayp(), Delay);
            UASSERT_OBJ(delayp->isCycleDelay() && VN_IS(delayp->lhsp(), Const), sexprp,
                        "SVA cycle delay must have a constant lhsp");
            जर (delayp->isRangeDelay()) {
                sexprp->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                              "non-constant cycle delay in consequent"
                                              " (IEEE 1800-2023 16.10)");
                परत -1;
            }
            स्थिरांक अचिन्हित delayCycles = VN_AS(delayp->lhsp(), Const)->toUInt();
            पूर्णांक preLen = 0;
            जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक prep = sexprp->preExprp()) {
                preLen = walk_Substitute_Match_वस्तुसंच(prep, K, match_वस्तुसंच);
                जर (preLen < 0) परत -1;
            }
            स्थिरांक पूर्णांक bodyLen
                = walk_Substitute_Match_वस्तुसंच(sexprp->exprp(), K + preLen + delayCycles, match_वस्तुसंच);
            जर (bodyLen < 0) परत -1;
            परत preLen + delayCycles + bodyLen;
        }
        जर (nodep->isMultiCycleSva()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                         "composite sequence operator in consequent"
                                         " (IEEE 1800-2023 16.10)");
            परत -1;
        }
        std::vector<AstVarRef*> refs;
        nodep->foreach([&refs](AstVarRef* p) { refs.ढकला_back(p); });
        साठी (AstVarRef* स्थिरांक refp : refs) substituteMatchItemRef(refp, K, match_वस्तुसंच);
        परत 0;
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
    तार्किक liftMatchItemSubstitutions(PropertyParts& parts, Ast_गाठ_अभिव्यक्ती* seqBodyp) {
        जर (!parts.hasImplication) परत असत्य;
        Ast_अभिव्यक्ती_Stmt* स्थिरांक अभिव्यक्ती_Stmtp = VN_CAST(parts.triggerExprp, अभिव्यक्ती_Stmt);
        जर (!अभिव्यक्ती_Stmtp) परत असत्य;
        // IEEE 1800-2023 16.10 BNF requires `(expr, match_item {, match_item})`
        // with at least one match item; V3LinkParse only emits ExprStmt for
        // this form and only emits AstAssign with VarRef LHS for each item.
        std::unordered_map<स्थिरांक AstVar*, Ast_गाठ_अभिव्यक्ती*> match_वस्तुसंच;
        साठी (Ast_गाठ* stmtp = अभिव्यक्ती_Stmtp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstAssign* स्थिरांक assignp = VN_AS(stmtp, Assign);
            AstVarRef* स्थिरांक lhsRefp = VN_AS(assignp->lhsp(), VarRef);
            match_वस्तुसंच[lhsRefp->varp()] = assignp->rhsp();
        }
        स्थिरांक अचिन्हित startK = parts.isOverlapped ? 0 : 1;
        स्थिरांक पूर्णांक length = walk_Substitute_Match_वस्तुसंच(seqBodyp, startK, match_वस्तुसंच);
        // Match-item substitution / strip mutates ancestor purity. Release
        // builds don't auto-clear caches on edits, so refresh here.
        VIsCached::clear_Cache_वृक्ष();
        जर (length < 0) परत सत्य;
        Ast_गाठ_अभिव्यक्ती* स्थिरांक antBoolp = अभिव्यक्ती_Stmtp->resultp()->unlinkFrBack();
        अभिव्यक्ती_Stmtp->replaceWith(antBoolp);
        VL_DO_DANGLING(ढकला_Deletep(अभिव्यक्ती_Stmtp), अभिव्यक्ती_Stmtp);
        parts.triggerExprp = antBoolp;
        परत असत्य;
    }

    // Outcome counts for a property if/case are wrong in an outcome-multiplying
    // context. Returns that context, or nullptr when the shape is supported.
    स्थिर स्थिरांक अक्षर* unsupportedPropertyControl(स्थिरांक Ast_गाठ_Cover_Or_Assert* assertp,
                                                  स्थिरांक Ast_गाठ_अभिव्यक्ती* seqBodyp, तार्किक negated) {
        जर (!hasPropertyControlConjunction(seqBodyp)) परत nullptr;
        जर (negated) परत "negation";
        जर (VN_IS(assertp, Cover)) परत "cover";
        जर (VN_AS(assertp, Assert)->passsp()) परत "a pass action";
        परत nullptr;
    }

    // Inline property/sequence refs and reject unsupported shapes.
    // Returns the PropSpec to lower, or nullptr when fully handled here.
    AstPropSpec* prepareAssertionProp(Ast_गाठ_Cover_Or_Assert* assertp) {
        जर (AstPropSpec* स्थिरांक specp = VN_CAST(assertp->propp(), PropSpec)) {
            जर (AstFuncRef* स्थिरांक funcrefp = VN_CAST(specp->propp(), FuncRef)) {
                जर (स्थिरांक AstProperty* स्थिरांक propyp = VN_CAST(funcrefp->taskp(), Property)) {
                    inlineNamedProperty(specp, funcrefp, propyp);
                }
            }
        }

        inlineAllSequenceRefs(assertp->propp());

        जर (AstPropSpec* स्थिरांक specp = VN_CAST(assertp->propp(), PropSpec)) {
            जर (hoistClockedSeq(specp)) परत nullptr;
        }

        AstPropSpec* स्थिरांक propp = VN_AS(assertp->propp(), PropSpec);
        जर (!VN_IS(assertp, Cover)
            && effectiveAssertPropStrength(propp) == VPropStrength::STRONG) {
            propp->v3warn(E_UNSUPPORTED,
                          "Unsupported: strong property in " + assertp->verilogKwd() + ".");
            replaceBodyOnBuildError(assertp->fileline(), propp, /*errorEmitted=*/सत्य);
            परत nullptr;
        }

        जर (!has_Multi_Cycle_अभिव्यक्ती(propp)) परत nullptr;
        // A nested property instance keeps its body behind the call; lowering would drop it.
        जर (propp->exists([](स्थिरांक AstFuncRef* refp) { परत VN_IS(refp->taskp(), Property); })) {
            assertp->v3warn(E_UNSUPPORTED,
                            "Unsupported: property instance inside a multi-cycle property "
                            "expression");
            VL_DO_DANGLING(ढकला_Deletep(assertp->unlinkFrBack()), assertp);
            परत nullptr;
        }
        जर (is_Bare_शीर्ष_Level_Until(propp)) परत nullptr;
        परत propp;
    }

    शून्य processAssertion(Ast_गाठ_Cover_Or_Assert* assertp) {
        जर (assertp->immediate()) परत;

        AstPropSpec* स्थिरांक propp = prepareAssertionProp(assertp);
        जर (!propp) परत;

        PropertyParts parts = decomposeProperty(propp);
        UASSERT_OBJ(parts.seqExprp, propp, "Property body must be an expression");

        // Unwrap `not` (IEEE 1800-2023 16.12.1); odd count -> negated semantics.
        Ast_गाठ_अभिव्यक्ती* seqBodyp = parts.seqExprp;
        तार्किक negated = असत्य;
        पर्यंत (AstLogNot* स्थिरांक notp = VN_CAST(seqBodyp, LogNot)) {
            negated = !negated;
            seqBodyp = notp->lhsp();
        }

        स्थिरांक अक्षर* स्थिरांक propertyControlp
            = unsupportedPropertyControl(assertp, seqBodyp, negated);

        // Substitute property-local match-item refs in consequent with
        // $past(rhs, K) before NFA build (IEEE 1800-2023 16.10).
        जर (liftMatchItemSubstitutions(parts, seqBodyp)) {
            AstPropSpec* स्थिरांक psp = VN_CAST(assertp->propp(), PropSpec);
            UASSERT_OBJ(psp, assertp, "Concurrent assertion must have PropSpec");
            replaceBodyOnBuildError(assertp->fileline(), psp, /*errorEmitted=*/सत्य);
            परत;
        }

        AstCover* स्थिरांक coverp = VN_CAST(assertp, Cover);
        स्थिरांक तार्किक isCoverSeq = coverp && coverp->isCoverSeq();
        // A sequence event control is not an assertion directive; no default
        // disable iff, no assertion control
        स्थिरांक तार्किक isSeqEvent = coverp && coverp->isSeqEvent();
        स्थिरांक तार्किक मोजणी_Matches = isCoverSeq && !isSeqEvent;
        // Inherit module defaults (IEEE 14.12, 16.15) when assertion has none.
        जर (!propp->sensesp() && m_defaultClockingp) {
            propp->sensesp(m_defaultClockingp->sentreep()->sensesp()->clone_वृक्ष(सत्य));
        }
        जर (!propp->disablep() && m_defaultDisablep && !isSeqEvent) {
            propp->disablep(m_defaultDisablep->condp()->clone_वृक्ष_Pure(सत्य));
        }
        जर (!propp->sensesp()) परत;
        Ast_Sen_वृक्ष* senTreep
            = नवीन Ast_Sen_वृक्ष{propp->fileline(), propp->sensesp()->clone_वृक्ष(सत्य)};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक disableExprp = propp->disablep();

        // NFA lowering clones repeated operands and may hoist them into an
        // always_comb block. Resolve implicit sampled-value clocks first, while
        // the enclosing assertion clock is still available.
        {
            VL_RESTORER(m_sampled_मूल्य_Clockp);
            m_sampled_मूल्य_Clockp = senTreep;
            iterate(propp->propp());
        }

        File_ओळ* स्थिरांक flp = assertp->fileline();

        SvaGraph graph;
        SvaNfaBuilder builder{graph,      m_modp,     m_propTempNames,
                              isCoverSeq, isSeqEvent, coverp != nullptr};

        स्थिरांक BuildResult result = buildAssertionGraph(builder, graph, seqBodyp, parts, flp);
        जर (result.valid()) wireMatchAndMidSources(graph, result, flp);
        जर (!result.valid()) {
            // Fall through to V3AssertPre for unsupported constructs; only
            // replace the body on real semantic errors. Any hoisted temps
            // from this attempt become orphan MODULETEMPs; V3Dead removes
            // them along with the dead always_comb driver.
            replaceBodyOnBuildError(flp, propp, result.errorEmitted);
            VL_DO_DANGLING(ढकला_Deletep(senTreep), senTreep);
            परत;
        }
        // After the build, so a construct the builder rejects reports itself.
        जर (propertyControlp) {
            seqBodyp->v3warn(E_UNSUPPORTED,
                             "Unsupported: temporal property if/case with " << propertyControlp);
            replaceBodyOnBuildError(flp, propp, /*errorEmitted=*/सत्य);
            VL_DO_DANGLING(ढकला_Deletep(senTreep), senTreep);
            परत;
        }
        जर (मोजणी_Matches && !v3Global.opt.coverageUser()) {
            SvaNfaBuilder::freeUnlinkedCondp(result.finalCondp);
            VL_DO_DANGLING(ढकला_Deletep(senTreep), senTreep);
            Ast_गाठ* स्थिरांक innerPropp = propp->propp();
            innerPropp->replaceWith(नवीन AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(ढकला_Deletep(innerPropp), innerPropp);
            परत;
        }

        AstAssert* स्थिरांक assertAssertp = VN_CAST(assertp, Assert);
        स्थिरांक तार्किक splitImplicationPasssp = assertAssertp && assertAssertp->passsp()
                                            && parts.hasImplication
                                            && canSplitImplicationPassActions(parts);
        स्थिरांक तार्किक needMatch = assertAssertp && assertAssertp->passsp()
                               && (!parts.hasImplication || splitImplicationPasssp);

        स्थिरांक तार्किक needsThreadFailReplay
            = assertAssertp && assertAssertp->failsp() && !parts.hasImplication;
        स्थिरांक स्वतः signals
            = m_loweringp->lower(assertp, graph, senTreep, result.finalCondp, disableExprp,
                                 needsThreadFailReplay, needsThreadFailReplay && !negated);
        Ast_गाठ_अभिव्यक्ती* matchExprp = nullptr;
        Ast_गाठ_अभिव्यक्ती* outputExprp = m_loweringp->assembleResult(
            assertp, negated, result.finalCondp, signals, needMatch ? &matchExprp : nullptr);
        जर (मोजणी_Matches) {
            UASSERT_OBJ(signals.matchCountp, coverp, "Cover sequence missing match count");
            VL_DO_DANGLING(outputExprp->delete_वृक्ष(), outputExprp);
            propp->matchCountp(signals.matchCountp);
            outputExprp = नवीन AstNeq{flp, signals.matchCountp->clone_वृक्ष_Pure(असत्य),
                                     newTypedConstp(flp, signals.matchCountp->dtypep(), 0)};
        }

        Ast_Sen_वृक्ष* स्थिरांक threadFailReplaySenTreep
            = signals.threadFailCountp ? senTreep->clone_वृक्ष(असत्य) : nullptr;

        VL_DO_DANGLING(ढकला_Deletep(senTreep), senTreep);
        जर (result.finalCondp && !result.finalCondp->backp()) ढकला_Deletep(result.finalCondp);

        जर (splitImplicationPasssp) {
            splitImplicationPassActions(assertAssertp, parts, matchExprp);
        } अन्यथा {
            attachActionHandlers(assertAssertp, matchExprp, threadFailReplaySenTreep,
                                 signals.threadFailCountp);
        }

        Ast_गाठ* स्थिरांक innerPropp = propp->propp();
        innerPropp->replaceWith(outputExprp);
        VL_DO_DANGLING(ढकला_Deletep(innerPropp), innerPropp);

        UINFO(4, "NFA converted assertion at " << flp << endl);

        जर (dumpGraphLevel() >= 6) graph.m_graph.dumpDotFilePrefixed("assert-nfa");
    }

    // VISITORS
    शून्य visit(Ast_गाठ_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_loweringp);
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultDisablep);
        m_modp = nodep;
        // A class nested in a module is in the scope of the module's default clocking
        जर (!VN_IS(nodep, Class)) m_defaultClockingp = nullptr;
        m_defaultDisablep = nodep->defaultDisablep();
        SvaNfaLowering lowering{nodep};
        m_loweringp = &lowering;
        iterateChildren(nodep);
    }
    शून्य visit(AstClocking* nodep) override {
        जर (nodep->isDefault() && !m_defaultClockingp) m_defaultClockingp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    शून्य visit(AstDefaultDisable* nodep) override {}
    शून्य visit(AstFell* nodep) override { visit_Sampled_मूल्य(nodep); }
    शून्य visit(AstPast* nodep) override { visit_Sampled_मूल्य(nodep); }
    शून्य visit(AstRose* nodep) override { visit_Sampled_मूल्य(nodep); }
    शून्य visit(AstStable* nodep) override { visit_Sampled_मूल्य(nodep); }
    शून्य visit(AstAssert* nodep) override { processAssertion(nodep); }
    शून्य visit(AstCover* nodep) override { processAssertion(nodep); }
    शून्य visit(AstRestrict* nodep) override {
        // Restrict property is ignored by simulators (IEEE 1800-2023 16.12.2).
        // Remove here so temporal SExpr don't leak to V3AssertPre.
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstAssertIntrinsic* nodep) override {}
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    explicit AssertNfaVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Top entry point

शून्य V3AssertNfa::assertNfaAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":" << endl);
    { AssertNfaVisitor{nodep}; }
    V3Global::dump_Check_Global_वृक्ष("assertnfa", 0, dump_वृक्ष_Either_Level() >= 3);
}
