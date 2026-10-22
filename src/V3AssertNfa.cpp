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
    AstVar* delay_Ring_Live_गन्ती_Varp = nullptr;  // Number of threads in delayRingVarp
    AstVar* delayRingWrappedVarp = nullptr;  // All slots written since the last clear
    AstVar* doneLVarp = nullptr;  // SAnd LHS done-latch
    AstVar* doneRVarp = nullptr;  // SAnd RHS done-latch
    Ast_गाँठो_अभिव्यक्ति* stateSigp = nullptr;  // Combinational state signal; OWNED during lowering
    Ast_गाँठो_अभिव्यक्ति* state_गन्ती_Sigp = nullptr;  // Cover-sequence match count; OWNED during lowering
    Ast_गाँठो_अभिव्यक्ति* state_गन्ती_Overflow_Sigp = nullptr;  // Count overflow; OWNED during lowering
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
    std::vector<Ast_गाँठो_अभिव्यक्ति*> m_throughoutConds;
    // Nonzero for a ring-buffer vertex for ## delays.
    तार्किक m_isFixedDelayRing = असत्य;
    अचिन्हित m_delayRingSize = 0;  // Number of ring slots. Range: max-min+1.
    Ast_गाँठो_अभिव्यक्ति* m_delayRingClearCondp = nullptr;  // local RHS for pure-boolean range
    Ast_गाँठो_अभिव्यक्ति* m_delayRingAdvanceCondp = nullptr;  // Advance only when this condition holds
    तार्किक m_replayAbortReject = असत्य;  // Compressed repetition needs per-thread abort replay
    // OWNED; enclosing-abort fire condition clearing state or suppressing guard rejection
    Ast_गाँठो_अभिव्यक्ति* m_abortClearp = nullptr;
    // OWNED; reject-abort fire condition rejecting all represented live threads
    Ast_गाँठो_अभिव्यक्ति* m_abortRejectp = nullptr;
    // Liveness terminal (IEEE weak semantics): reject must not fire from this source
    तार्किक m_isUnbounded = असत्य;
    // Temporal sequence AND combiner; IEEE 1800-2023 16.9.5
    तार्किक m_isAndCombiner = असत्य;
    SvaStateVertex* m_andLhsTermp = nullptr;  // LHS sub-NFA terminal vertex
    SvaStateVertex* m_andRhsTermp = nullptr;  // RHS sub-NFA terminal vertex
    Ast_गाँठो_अभिव्यक्ति* m_andLhsCondp = nullptr;  // OWNED; LHS final condition (may be nullptr)
    Ast_गाँठो_अभिव्यक्ति* m_andRhsCondp = nullptr;  // OWNED; RHS final condition (may be nullptr)
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
        लागि (Ast_गाँठो_अभिव्यक्ति* cp : m_throughoutConds) VL_DO_DANGLING(cp->delete_रूख(), cp);
        यदि (m_delayRingClearCondp)
            VL_DO_DANGLING(m_delayRingClearCondp->delete_रूख(), m_delayRingClearCondp);
        यदि (m_delayRingAdvanceCondp)
            VL_DO_DANGLING(m_delayRingAdvanceCondp->delete_रूख(), m_delayRingAdvanceCondp);
        यदि (m_abortClearp) VL_DO_DANGLING(m_abortClearp->delete_रूख(), m_abortClearp);
        यदि (m_abortRejectp) VL_DO_DANGLING(m_abortRejectp->delete_रूख(), m_abortRejectp);
        यदि (m_andLhsCondp) VL_DO_DANGLING(m_andLhsCondp->delete_रूख(), m_andLhsCondp);
        यदि (m_andRhsCondp) VL_DO_DANGLING(m_andRhsCondp->delete_रूख(), m_andRhsCondp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string name() अचर override {
        string name = "s" + cvtToStr(color());
        यदि (m_delayRingSize) {
            name += "\\n";
            name += m_isFixedDelayRing ? "fixed chain " : "range chain ";
            name += cvtToStr(m_delayRingSize) + " slots";
        }
        फर्काऊ name;
    }
    string dotColor() अचर override {
        यदि (m_isMatch) फर्काऊ "red";
        यदि (m_delayRingSize) फर्काऊ "blue";
        यदि (m_isAndCombiner) फर्काऊ "purple";
        फर्काऊ "black";
    }
    // LCOV_EXCL_STOP
    // Access per-vertex algorithm data (valid only during lowering phase)
    SvaVertexData* datap() अचर { फर्काऊ static_cast<SvaVertexData*>(userp()); }
};

// NFA transition edge -- clocked (##1) or combinational link (##0)
वर्ग SvaTransEdge final : सार्वजनिक V3GraphEdge {
    VL_RTTI_IMPL(SvaTransEdge, V3GraphEdge)
सार्वजनिक:
    Ast_गाँठो_अभिव्यक्ति* m_condp;  // Transition condition; nullptr = unconditional; OWNED
    तार्किक m_consumesCycle;  // true = clocked edge (##1), false = link (##0/boolean)
    // Reject when source is active and condp is false; set only on
    // outermost required-step Link
    तार्किक m_rejectOnFail = असत्य;
    // Optional dynamic condition vertex for m_rejectOnFail. Used when the
    // success condition is another NFA state rather than a static expression.
    SvaStateVertex* m_condVtxp = nullptr;

    // CONSTRUCTORS
    SvaTransEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष, Ast_गाँठो_अभिव्यक्ति* condp,
                 तार्किक consumesCycle)
        : V3GraphEdge{graphp, fromp, शीर्ष, /*weight=*/1}
        , m_condp{condp}
        , m_consumesCycle{consumesCycle} {}
    ~SvaTransEdge() override {
        यदि (m_condp) VL_DO_DANGLING(m_condp->delete_रूख(), m_condp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string dotLabel() अचर override { फर्काऊ m_consumesCycle ? "##1" : "link"; }
    string dotStyle() अचर override { फर्काऊ m_consumesCycle ? "" : "dashed"; }
    // LCOV_EXCL_STOP
    // Typed accessors for NFA vertices
    SvaStateVertex* fromVtxp() अचर { फर्काऊ static_cast<SvaStateVertex*>(fromp()); }
    SvaStateVertex* toVtxp() अचर { फर्काऊ static_cast<SvaStateVertex*>(शीर्ष()); }
};

// NFA graph container
वर्ग SvaGraph final {
सार्वजनिक:
    V3Graph m_graph;  // Owns all vertices and edges
    SvaStateVertex* m_startVertexp = nullptr;  // Trigger/start vertex
    SvaStateVertex* m_matchVertexp = nullptr;  // Sequence-match terminal vertex

    // Create a new state vertex
    SvaStateVertex* createStateVertex() { फर्काऊ नूतन SvaStateVertex{&m_graph}; }
    // Create the match terminal vertex
    SvaStateVertex* createMatchVertex() {
        SvaStateVertex* अचर vtxp = createStateVertex();
        vtxp->m_isMatch = सत्य;
        m_matchVertexp = vtxp;
        फर्काऊ vtxp;
    }
    // Add a clocked transition edge (##1)
    SvaTransEdge* addClockedEdge(SvaStateVertex* fromp, SvaStateVertex* शीर्ष,
                                 Ast_गाँठो_अभिव्यक्ति* condp = nullptr) {
        फर्काऊ नूतन SvaTransEdge{&m_graph, fromp, शीर्ष, condp, /*consumesCycle=*/सत्य};
    }
    // Add a combinational link (##0 / boolean condition)
    SvaTransEdge* addLink(SvaStateVertex* fromp, SvaStateVertex* शीर्ष,
                          Ast_गाँठो_अभिव्यक्ति* condp = nullptr) {
        फर्काऊ नूतन SvaTransEdge{&m_graph, fromp, शीर्ष, condp, /*consumesCycle=*/असत्य};
    }
    // Collect all edges into a flat vector for iteration.
    // Used by the lowering phase which needs global edge scans.
    std::vector<अचर SvaTransEdge*> allEdges() अचर {
        std::vector<अचर SvaTransEdge*> result;
        लागि (अचर V3GraphVertex& vtxr : m_graph.vertices()) {
            लागि (अचर V3GraphEdge& edger : vtxr.outEdges()) {
                result.थप_back(static_cast<अचर SvaTransEdge*>(&edger));
            }
        }
        फर्काऊ result;
    }
};

//######################################################################
// Builder result: terminal vertex + optional final condition (match Link condition).
संरचना BuildResult final {
    SvaStateVertex* termVertexp;  // Primary terminal; contributes to both match and reject
    Ast_गाँठो_अभिव्यक्ति* finalCondp;  // nullptr = unconditional
    // Mid-window sources for range delays (pure boolean RHS): match-only (isUnbounded)
    std::vector<SvaStateVertex*> midSources;
    तार्किक errorEmitted = असत्य;  // Builder already emitted specific error; skip generic
    // For cover_sequence: when true, midSources already enumerate every
    // end-of-match, so wireMatchAndMidSources must NOT add the main
    // termVtxp -> matchVertex Link (would double-count via the merge vertex).
    तार्किक termIsMidMerge = असत्य;
    तार्किक valid() अचर { फर्काऊ termVertexp != nullptr; }
    स्थिर BuildResult fail(तार्किक errored = असत्य) { फर्काऊ {nullptr, nullptr, {}, errored}; }
    स्थिर BuildResult failWithError() { फर्काऊ {nullptr, nullptr, {}, सत्य}; }
};

// Parser-marked SAnd of overlapped implications: a property if/else/case.
स्थिर तार्किक hasPropertyControlConjunction(अचर Ast_गाँठो_अभिव्यक्ति* nodep) {
    फर्काऊ nodep->exists([](अचर AstSAnd* andp) { फर्काऊ andp->propertyControl(); });
}

स्थिर AstConst* newTypedConstp(File_पङ्क्ति* अचर flp, अचर Ast_गाँठो_DType* अचर dtypep,
                                अचर uint32_t मान) {
    AstConst* अचर constp = नूतन AstConst{flp, AstConst::DTyped{}, dtypep};
    constp->अङ्क().setLong(मान);
    फर्काऊ constp;
}

स्थिर Ast_गाँठो_अभिव्यक्ति* sampled(Ast_गाँठो_अभिव्यक्ति* exprp) {
    फर्काऊ नूतन AstSampled{exprp->fileline(), exprp, exprp->dtypep(), सत्य};
}

स्थिर अचर वर्ण* assertPassOnQuery(तार्किक vacuous) {
    स्थिर constexpr अचर वर्ण* queries[2]
        = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
    फर्काऊ queries[vacuous];
}

स्थिर Ast_गाँठो_अभिव्यक्ति* assertOnCond(File_पङ्क्ति* flp, VAssertType type,
                                 VAssertDirectiveType directiveType) {
    यदि (!v3Global.opt.assertOn()) { फर्काऊ नूतन AstConst{flp, AstConst::BitFalse{}}; }
    फर्काऊ नूतन AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
}

स्थिर Ast_गाँठो_अभिव्यक्ति* assertKillGet(File_पङ्क्ति* flp, VAssertType type,
                                  VAssertDirectiveType directiveType) {
    फर्काऊ नूतन AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_KILL", type, directiveType),
                        32};
}

स्थिर string assertActionControlPrefix(VAssertDirectiveType directiveType) {
    अचर पूर्णाङ्क controlled = !!(static_cast<पूर्णाङ्क>(directiveType)
                              & (static_cast<पूर्णाङ्क>(VAssertDirectiveType::ASSERT)
                                 | static_cast<पूर्णाङ्क>(VAssertDirectiveType::COVER)
                                 | static_cast<पूर्णाङ्क>(VAssertDirectiveType::ASSUME)));
    अचर पूर्णाङ्क checkRuntime = controlled & static_cast<पूर्णाङ्क>(v3Global.opt.assertOn());
    फर्काऊ "("s + std::to_string(controlled ^ 1) + " || ("s + std::to_string(checkRuntime)
           + " && "s;
}

स्थिर Ast_गाँठो_अभिव्यक्ति* assertPassOnCond(File_पङ्क्ति* flp, VAssertType type,
                                     VAssertDirectiveType directiveType, तार्किक vacuous) {
    फर्काऊ नूतन AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType)
            + "))"s,
        1};
}

स्थिर Ast_गाँठो_अभिव्यक्ति* assertFailOnCond(File_पङ्क्ति* flp, VAssertType type,
                                     VAssertDirectiveType directiveType) {
    फर्काऊ नूतन AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType) + "))"s,
        1};
}

स्थिर AstIf* newPassOnIf(File_पङ्क्ति* flp, Ast_गाँठो_अभिव्यक्ति* firep, Ast_गाँठो* bodyp, VAssertType type,
                          VAssertDirectiveType directiveType, तार्किक vacuous) {
    Ast_गाँठो_अभिव्यक्ति* अचर condp
        = नूतन AstLogAnd{flp, firep, assertPassOnCond(flp, type, directiveType, vacuous)};
    AstIf* अचर ifp = नूतन AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(सत्य);
    ifp->user1(सत्य);
    फर्काऊ ifp;
}

स्थिर Ast_गाँठो_Stmt* newIfAssertFailOn(Ast_गाँठो* bodyp, VAssertDirectiveType directiveType,
                                      VAssertType type) {
    File_पङ्क्ति* अचर flp = bodyp->fileline();
    Ast_गाँठो_अभिव्यक्ति* अचर condp = assertFailOnCond(flp, type, directiveType);
    AstIf* अचर ifp = नूतन AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(सत्य);
    ifp->user1(सत्य);
    फर्काऊ ifp;
}

//######################################################################
// NFA Builder

वर्ग SvaNfaBuilder final {
    SvaGraph& m_graph;  // NFA graph being built
    Ast_गाँठो_Module* अचर m_modp;  // Module to receive hoisted sampled-prop temps
    V3UniqueNames& m_propTempNames;  // Module-shared temp-var name source
    std::vector<Ast_गाँठो_अभिव्यक्ति*> m_temporal_Guard_थाक;  // Guards active across nested temporal states
    // Outer abort conditions, AND-ed as !cond into inner abort edges
    // (IEEE 1800-2023 16.12.14 outer-wraps-inner).
    std::vector<Ast_गाँठो_अभिव्यक्ति*> m_outer_Abort_थाक;
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
        पूर्णाङ्क rhsLen = 0;
    };

    शून्य warnEndpointUnsupported(File_पङ्क्ति* flp, अचर string& what) अचर {
        यदि (m_isSeqEvent) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: sequence used as an event control with " << what);
        } अन्यथा {
            flp->v3warn(COVERIGN, "Ignoring unsupported: cover sequence with " << what);
        }
    }

    Ast_गाँठो_अभिव्यक्ति* throughoutCond(Ast_गाँठो_अभिव्यक्ति* baseCondp, File_पङ्क्ति* flp) {
        यदि (m_temporal_Guard_थाक.empty()) फर्काऊ baseCondp;
        // AND all active temporal guards (supports nesting)
        // Each must use $sampled values.
        Ast_गाँठो_अभिव्यक्ति* guardp = nullptr;
        लागि (Ast_गाँठो_अभिव्यक्ति* अचर condp : m_temporal_Guard_थाक) {
            Ast_गाँठो_अभिव्यक्ति* अचर clonep = sampled(condp->clone_रूख_Pure(असत्य));
            यदि (!guardp) {
                guardp = clonep;
            } अन्यथा {
                guardp = नूतन AstLogAnd{flp, guardp, clonep};
            }
        }
        यदि (baseCondp) { guardp = नूतन AstLogAnd{flp, baseCondp, guardp}; }
        फर्काऊ guardp;
    }

    स्थिर अचिन्हित getConstUInt(Ast_गाँठो_अभिव्यक्ति* exprp) {
        Ast_गाँठो_अभिव्यक्ति* अचर constp = V3Const::constifyEdit(exprp->clone_रूख_Pure(असत्य));
        अचर AstConst* अचर cp = VN_CAST(constp, Const);
        अचर अचिन्हित val = cp ? cp->toUInt() : 0;
        VL_DO_DANGLING(constp->delete_रूख(), constp);
        फर्काऊ val;
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
    स्थिर पूर्णाङ्क fixedLength(Ast_गाँठो_अभिव्यक्ति* nodep) {
        यदि (AstSExpr* अचर sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* अचर delayp = VN_CAST(sexprp->delayp(), Delay);
            यदि (!delayp || !delayp->isCycleDelay()) फर्काऊ -1;
            अचिन्हित delayCycles;
            यदि (delayp->isRangeDelay()) {
                यदि (delayp->isUnbounded()) फर्काऊ -1;  // LCOV_EXCL_LINE
                अचर अचिन्हित minD = getConstUInt(delayp->lhsp());
                अचर अचिन्हित maxD = getConstUInt(delayp->rhsp());
                यदि (minD != maxD) फर्काऊ -1;
                delayCycles = minD;
            } अन्यथा {
                delayCycles = getConstUInt(delayp->lhsp());
            }
            पूर्णाङ्क preLen = 0;
            यदि (Ast_गाँठो_अभिव्यक्ति* अचर prep = sexprp->preExprp()) {
                preLen = fixedLength(prep);
                यदि (preLen < 0) फर्काऊ -1;  // LCOV_EXCL_LINE
            }
            अचर पूर्णाङ्क bodyLen = fixedLength(sexprp->exprp());
            यदि (bodyLen < 0) फर्काऊ -1;  // LCOV_EXCL_LINE
            फर्काऊ preLen + delayCycles + bodyLen;
        }
        यदि (AstSThroughout* अचर throughp = VN_CAST(nodep, SThroughout)) {
            फर्काऊ fixedLength(throughp->rhsp());
        }
        यदि (AstSOr* अचर orp = VN_CAST(nodep, SOr)) {
            // Alternatives must share one end cycle; buildSWithin relies on
            // this to pair the OR with an SIntersect.
            अचर पूर्णाङ्क lhsLen = fixedLength(orp->lhsp());
            अचर पूर्णाङ्क rhsLen = fixedLength(orp->rhsp());
            यदि (lhsLen < 0 || rhsLen < 0 || lhsLen != rhsLen) फर्काऊ -1;
            फर्काऊ lhsLen;
        }
        यदि (AstSWithin* अचर withinp = VN_CAST(nodep, SWithin)) {
            // `seq1 within seq2` ends at seq2's end cycle (IEEE 16.9.10).
            अचर पूर्णाङ्क lhsLen = fixedLength(withinp->lhsp());
            अचर पूर्णाङ्क rhsLen = fixedLength(withinp->rhsp());
            यदि (lhsLen < 0 || rhsLen < 0 || lhsLen > rhsLen) फर्काऊ -1;
            फर्काऊ rhsLen;
        }
        // LCOV_EXCL_START -- defensive: V3AssertPre rejects composite SVA ops
        // nested in an intersect arm before fixedLength runs (clock-context
        // resolution fails). Kept as a guard in case future parser relaxations
        // permit it.
        यदि (nodep->isMultiCycleSva()) फर्काऊ -1;
        // LCOV_EXCL_STOP
        // Plain boolean expression (no SVA constructs) -- 0 cycles.
        फर्काऊ 0;
    }

    // Contiguous match-length range [lo,hi] for an operand whose length varies
    // from AT MOST ONE ranged cycle delay; {-1,-1} otherwise. Drives the
    // variable-length `intersect` lowering (IEEE 1800-2023 16.9.6): more than
    // one ranged delay would make the per-length realization ambiguous, so it
    // is reported unsupported rather than mis-paired.
    स्थिर std::pair<पूर्णाङ्क, पूर्णाङ्क> lengthRange(Ast_गाँठो_अभिव्यक्ति* nodep) {
        यदि (AstSExpr* अचर sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* अचर delayp = VN_CAST(sexprp->delayp(), Delay);
            यदि (!delayp || !delayp->isCycleDelay()) फर्काऊ {-1, -1};
            std::pair<पूर्णाङ्क, पूर्णाङ्क> delayRange;
            यदि (delayp->isRangeDelay()) {
                यदि (delayp->isUnbounded()) फर्काऊ {-1, -1};
                अचर अचिन्हित minD = getConstUInt(delayp->lhsp());
                अचर अचिन्हित maxD = getConstUInt(delayp->rhsp());
                delayRange = {minD, maxD};
            } अन्यथा {
                अचर अचिन्हित d = getConstUInt(delayp->lhsp());
                delayRange = {d, d};
            }
            std::pair<पूर्णाङ्क, पूर्णाङ्क> preRange{0, 0};
            यदि (Ast_गाँठो_अभिव्यक्ति* अचर prep = sexprp->preExprp()) {
                preRange = lengthRange(prep);
                यदि (preRange.first < 0) फर्काऊ {-1, -1};
            }
            अचर std::pair<पूर्णाङ्क, पूर्णाङ्क> bodyRange = lengthRange(sexprp->exprp());
            यदि (bodyRange.first < 0) फर्काऊ {-1, -1};
            अचर पूर्णाङ्क variableParts = (preRange.first != preRange.second)
                                      + (delayRange.first != delayRange.second)
                                      + (bodyRange.first != bodyRange.second);
            यदि (variableParts > 1) फर्काऊ {-1, -1};
            फर्काऊ {preRange.first + delayRange.first + bodyRange.first,
                    preRange.second + delayRange.second + bodyRange.second};
        }
        यदि (AstSThroughout* अचर throughp = VN_CAST(nodep, SThroughout)) {
            फर्काऊ lengthRange(throughp->rhsp());
        }
        यदि (nodep->isMultiCycleSva()) फर्काऊ {-1, -1};
        फर्काऊ {0, 0};  // plain boolean -- 0 cycles
    }

    // Clone `operand` with its sole variable ranged cycle delay pinned so the
    // total match length is exactly `len`. `lo` is the operand's minimum length
    // (lengthRange().first). A fixed operand has no such delay and is returned
    // as a plain clone (callers only request its single achievable length).
    स्थिर Ast_गाँठो_अभिव्यक्ति* realizeAtLength(Ast_गाँठो_अभिव्यक्ति* operand, पूर्णाङ्क len, पूर्णाङ्क lo) {
        Ast_गाँठो_अभिव्यक्ति* अचर clonep = operand->clone_रूख_Pure(असत्य);
        AstDelay* rangeDelayp = nullptr;
        clonep->foreach([&](AstDelay* dp) {
            यदि (!rangeDelayp && dp->isRangeDelay() && !dp->isUnbounded()
                && getConstUInt(dp->lhsp()) != getConstUInt(dp->rhsp())) {
                rangeDelayp = dp;
            }
        });
        यदि (rangeDelayp) {
            File_पङ्क्ति* अचर flp = rangeDelayp->fileline();
            अचर अचिन्हित pinned = getConstUInt(rangeDelayp->lhsp()) + (len - lo);
            Ast_गाँठो_अभिव्यक्ति* अचर oldMinp = rangeDelayp->lhsp();
            oldMinp->replaceWith(नूतन AstConst{flp, pinned});
            VL_DO_DANGLING(oldMinp->delete_रूख(), oldMinp);
            // Drop the max bound so it lowers as a fixed `##d`, not `##[d:d]`.
            Ast_गाँठो* अचर oldMaxp = rangeDelayp->rhsp()->unlinkFrBack();
            VL_DO_DANGLING(oldMaxp->delete_रूख(), oldMaxp);
        }
        फर्काऊ clonep;
    }

    // Cuts AST size from O(N * sizeof(exprp)) to O(N) + O(sizeof(exprp)) by
    // sharing a single `VarRef` across N check edges. Hoist also matches the
    // IEEE 1800-2023 16.9.9 "single preponed-region snapshot" semantic for
    // any exprp -- even an impure one would now evaluate exactly once per
    // clock instead of N times. Orphan temps from failed builds are unused
    // MODULETEMPs and are removed by V3Dead.
    AstVar* tryHoistSampled(Ast_गाँठो_अभिव्यक्ति* exprp, File_पङ्क्ति* flp, अचिन्हित clone_गन्ती) {
        constexpr अचिन्हित kHoistThreshold = 2;
        यदि (clone_गन्ती < kHoistThreshold) फर्काऊ nullptr;
        AstVar* अचर tempVarp
            = नूतन AstVar{flp, VVarType::MODULETEMP, m_propTempNames.get(exprp), exprp->dtypep()};
        m_modp->addStmtsp(tempVarp);
        AstAssign* अचर assignp = नूतन AstAssign{flp, नूतन AstVarRef{flp, tempVarp, VAccess::WRITE},
                                                 sampled(exprp->clone_रूख_Pure(असत्य))};
        m_modp->addStmtsp(नूतन AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, assignp});
        फर्काऊ tempVarp;
    }

    स्थिर Ast_गाँठो_अभिव्यक्ति* sampledRefOrClone(AstVar* hoistVarp, Ast_गाँठो_अभिव्यक्ति* exprp, File_पङ्क्ति* flp) {
        यदि (hoistVarp) फर्काऊ नूतन AstVarRef{flp, hoistVarp, VAccess::READ};
        फर्काऊ sampled(exprp->clone_रूख_Pure(असत्य));
    }

    // Create vertex and inherit temporal guards from the current scope.
    SvaStateVertex* scopedCreateVertex() {
        SvaStateVertex* अचर vtxp = m_graph.createStateVertex();
        लागि (Ast_गाँठो_अभिव्यक्ति* अचर cp : m_temporal_Guard_थाक) {
            vtxp->m_throughoutConds.थप_back(cp->clone_रूख_Pure(असत्य));
        }
        यदि (m_inUnboundedScope) vtxp->m_isUnbounded = सत्य;
        यदि (m_markStrongPending) vtxp->m_strongPending = सत्य;
        फर्काऊ vtxp;
    }

    // AND current temporal guards into every edge/link.
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* शीर्ष, Ast_गाँठो_अभिव्यक्ति* condp,
                              File_पङ्क्ति* flp) {
        फर्काऊ m_graph.addLink(fromp, शीर्ष, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* शीर्ष, File_पङ्क्ति* flp) {
        फर्काऊ m_graph.addLink(fromp, शीर्ष, throughoutCond(nullptr, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* शीर्ष, Ast_गाँठो_अभिव्यक्ति* condp,
                              File_पङ्क्ति* flp) {
        फर्काऊ m_graph.addClockedEdge(fromp, शीर्ष, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* शीर्ष, File_पङ्क्ति* flp) {
        फर्काऊ m_graph.addClockedEdge(fromp, शीर्ष, throughoutCond(nullptr, flp));
    }

    SvaStateVertex* addDelayChain(SvaStateVertex* startp, अचिन्हित size, File_पङ्क्ति* flp,
                                  तार्किक isFixed = सत्य, Ast_गाँठो_अभिव्यक्ति* clearCondp = nullptr,
                                  Ast_गाँठो_अभिव्यक्ति* advanceCondp = nullptr) {
        यदि (isFixed && size == 0) फर्काऊ startp;
        UASSERT_OBJ(size > 0, startp, "Delay chain needs at least one slot");
        यदि (isFixed && size == 1 && !advanceCondp) {
            SvaStateVertex* अचर nextp = scopedCreateVertex();
            guardedEdge(startp, nextp, flp);
            फर्काऊ nextp;
        }
        SvaStateVertex* अचर ringVtxp = scopedCreateVertex();
        ringVtxp->m_isFixedDelayRing = isFixed;
        ringVtxp->m_delayRingSize = size;
        यदि (clearCondp) {
            UASSERT_OBJ(!isFixed, startp, "Fixed delay cannot have a clear condition");
            ringVtxp->m_delayRingClearCondp = clearCondp->clone_रूख_Pure(असत्य);
        }
        ringVtxp->m_delayRingAdvanceCondp = advanceCondp;
        यदि (isFixed) {
            guardedEdge(startp, ringVtxp, flp);
        } अन्यथा {
            guardedLink(startp, ringVtxp, flp);
        }
        फर्काऊ ringVtxp;
    }

    // Build NFA for an SExpr. finalCond = RHS (not yet added as a vertex).
    // isTopLevelStep: marks outermost required boolean check as rejectOnFail.
    // Apply a range delay `##[M:N]` to currentp. Returns true on success. On
    // failure, emits a diagnostic and returns false.
    तार्किक applyRangeDelay(AstSExpr* sexprp, SvaStateVertex*& currentp,
                         std::vector<SvaStateVertex*>& midSources,
                         RangeDelayRejectInfo* rangeRejectInfop = nullptr) {
        File_पङ्क्ति* अचर flp = sexprp->fileline();
        AstDelay* अचर delayp = VN_AS(sexprp->delayp(), Delay);
        Ast_गाँठो_अभिव्यक्ति* अचर rhsExprp = sexprp->exprp();
        अचर अचिन्हित minDelay = getConstUInt(delayp->lhsp());
        यदि (delayp->isUnbounded()) {
            // `##[M:$]`: wait M cycles, then retain every matured attempt in a
            // one-slot ring while also exposing the first eligible tick.
            currentp = addDelayChain(currentp, minDelay, flp);
            SvaStateVertex* अचर waitRingp = addDelayChain(currentp, 1, flp, असत्य);
            // The ring's live count is updated in NBA, so this link retains the previous
            // tick's live attempts alongside new arrivals for the next tick.
            guardedLink(waitRingp, waitRingp, flp);
            SvaStateVertex* अचर mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(waitRingp, mergeVtxp, flp);
            currentp = mergeVtxp;
            currentp->m_isUnbounded = सत्य;  // Liveness, so no reject.
            m_inUnboundedScope = सत्य;
            फर्काऊ सत्य;
        }
        अचर अचिन्हित maxDelay = getConstUInt(delayp->rhsp());
        यदि (minDelay == maxDelay) {
            currentp = addDelayChain(currentp, minDelay, flp);
            फर्काऊ सत्य;
        }
        अचर अचिन्हित range = maxDelay - minDelay;
        currentp = addDelayChain(currentp, minDelay, flp);
        अचर तार्किक multiCycleRhs = rhsExprp->isMultiCycleSva();
        अचर Ast_गाँठो_अभिव्यक्ति* अचर preExprp = sexprp->preExprp();
        // Prefix and nested-sequence composition can lose endpoints or their multiplicity
        // required by cover sequence (IEEE 1800-2023 16.14.3), regardless of the range width.
        यदि (m_isCoverSeq
            && (multiCycleRhs || m_inSequencePrefix
                || (preExprp && preExprp->isMultiCycleSva()))) {
            warnEndpointUnsupported(flp, "this ranged cycle delay");
            फर्काऊ असत्य;
        }
        यदि (m_inSequencePrefix) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: Bounded ranged cycle delay in a sequence prefix");
            फर्काऊ असत्य;
        }
        यदि (multiCycleRhs) {
            // Launch a candidate at every eligible tick. Candidate-local misses are not
            // assertion rejects while a later position can still match.
            यदि (rangeRejectInfop) {
                अचर पूर्णाङ्क rhsLen = fixedLength(rhsExprp);
                यदि (rhsLen >= 0) *rangeRejectInfop = {currentp, range, rhsLen};
            }
            SvaStateVertex* अचर tailp = addDelayChain(currentp, range + 1U, flp, असत्य);
            SvaStateVertex* अचर mergeVtxp = scopedCreateVertex();
            mergeVtxp->m_isUnbounded = सत्य;
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(tailp, mergeVtxp, flp);
            currentp = mergeVtxp;
            m_inUnboundedScope = सत्य;
        } अन्यथा {
            // The first eligible tick is explicit. The ring retains attempts for the
            // remaining ticks and exposes the final tick for rejection. Only properties
            // clear on success, since cover sequence counts every endpoint.
            midSources.थप_back(currentp);
            currentp = addDelayChain(currentp, range + 1U, flp, असत्य,
                                     m_isCoverSeq ? nullptr : rhsExprp);
        }
        फर्काऊ सत्य;
    }

    शून्य addFiniteRangeReject(अचर RangeDelayRejectInfo& info, अचर BuildResult& result,
                              File_पङ्क्ति* flp) {
        यदि (!info.startp) फर्काऊ;

        SvaStateVertex* अचर expiryVtxp
            = addDelayChain(info.startp, info.range + info.rhsLen, flp);
        SvaStateVertex* अचर expiryMatchp = scopedCreateVertex();
        std::vector<SvaStateVertex*> sources = result.midSources;
        sources.थप_back(result.termVertexp);
        लागि (SvaStateVertex* अचर srcp : sources) {
            Ast_गाँठो_अभिव्यक्ति* अचर condp
                = result.finalCondp ? sampled(result.finalCondp->clone_रूख_Pure(असत्य)) : nullptr;
            SvaStateVertex* अचर successNowp = scopedCreateVertex();
            guardedLink(srcp, successNowp, condp, flp);
            // Retain a success through the latest deadline it can satisfy.
            SvaStateVertex* अचर historyp
                = addDelayChain(successNowp, info.range + 1U, flp, असत्य);
            guardedLink(successNowp, expiryMatchp, flp);
            guardedLink(historyp, expiryMatchp, flp);
        }

        SvaStateVertex* अचर sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = सत्य;
        SvaTransEdge* अचर rejectp = m_graph.addLink(expiryVtxp, sinkVtxp);
        rejectp->m_rejectOnFail = सत्य;
        rejectp->m_condVtxp = expiryMatchp;
    }

    BuildResult buildSExpr(AstSExpr* sexprp, SvaStateVertex* entryVtxp,
                           तार्किक is_शीर्ष_Level_Step = असत्य) {
        AstDelay* अचर delayp = VN_CAST(sexprp->delayp(), Delay);
        यदि (!delayp || !delayp->isCycleDelay()) फर्काऊ BuildResult::fail();

        File_पङ्क्ति* अचर flp = sexprp->fileline();
        Ast_गाँठो_अभिव्यक्ति* अचर exprp = sexprp->exprp();

        // Handle LHS (preExpr)
        SvaStateVertex* currentp = entryVtxp;
        यदि (Ast_गाँठो_अभिव्यक्ति* अचर preExprp = sexprp->preExprp()) {
            VL_RESTORER(m_inSequencePrefix);
            m_inSequencePrefix = सत्य;
            अचर BuildResult pre = build_अभिव्यक्ति(preExprp, currentp, is_शीर्ष_Level_Step);
            यदि (!pre.valid()) फर्काऊ BuildResult::fail(pre.errorEmitted);  // LCOV_EXCL_LINE
            यदि (pre.finalCondp) {
                SvaStateVertex* अचर condVtxp = scopedCreateVertex();
                SvaTransEdge* अचर edgep = guardedLink(
                    pre.termVertexp, condVtxp, sampled(pre.finalCondp->clone_रूख_Pure(असत्य)), flp);
                यदि (is_शीर्ष_Level_Step && !pre.termVertexp->m_isUnbounded && !m_inUnboundedScope) {
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
        अचर तार्किक addRangeReject = is_शीर्ष_Level_Step && !m_inUnboundedScope;
        यदि (delayp->isRangeDelay()) {
            यदि (!applyRangeDelay(sexprp, currentp, rangeMidSources,
                                 addRangeReject ? &rangeRejectInfo : nullptr)) {
                फर्काऊ BuildResult::failWithError();
            }
        } अन्यथा {
            अचर अचिन्हित delayCycles = getConstUInt(delayp->lhsp());
            currentp = addDelayChain(currentp, delayCycles, flp);
        }

        // Multi-cycle RHS: recurse (only plain boolean is returned as finalCondp).
        यदि (exprp->isMultiCycleSva()) {
            अचर BuildResult result = build_अभिव्यक्ति(exprp, currentp, is_शीर्ष_Level_Step);
            यदि (result.valid()) addFiniteRangeReject(rangeRejectInfo, result, flp);
            फर्काऊ result;
        }
        फर्काऊ {currentp, exprp, std::move(rangeMidSources)};
    }

    BuildResult buildConsRep(AstSConsRep* repp, SvaStateVertex* entryVtxp,
                             तार्किक is_शीर्ष_Level_Step = असत्य) {
        File_पङ्क्ति* अचर flp = repp->fileline();
        Ast_गाँठो_अभिव्यक्ति* अचर exprp = repp->exprp();
        // Multi-cycle expr in ConsRep not yet supported; bail to avoid invalid AST.
        यदि (exprp->isMultiCycleSva()) {
            repp->v3warn(E_UNSUPPORTED, "Unsupported: multi-cycle sequence expression inside"
                                        " consecutive repetition (IEEE 1800-2023 16.9.2)");
            फर्काऊ BuildResult::failWithError();
        }
        अचर अचिन्हित minN = getConstUInt(repp->countp());

        // Sum sites across prefix + unbounded/range tail so one hoist covers
        // every check edge of this repetition.
        अचिन्हित totalSites = minN;
        यदि (repp->unbounded()) {
            totalSites += 1;
        } अन्यथा यदि (repp->maxCountp()) {
            totalSites += getConstUInt(repp->maxCountp()) - minN;
        }
        AstVar* अचर hoistVarp = tryHoistSampled(exprp, flp, totalSites);

        // Cover-sequence (IEEE 1800-2023 16.14.3): collect each end-of-match
        // position so they all fire the action, not just the merged terminal.
        std::vector<SvaStateVertex*> consMidSources;

        SvaStateVertex* currentp = entryVtxp;
        लागि (अचिन्हित i = 0; i < minN; ++i) {
            // Keep the first repetition explicit, collapse all remaining checks into the ring.
            यदि (i == 1) {
                currentp = addDelayChain(currentp, minN - 1, flp);
                currentp->m_delayRingClearCondp
                    = नूतन AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                currentp->m_replayAbortReject = सत्य;
                यदि (is_शीर्ष_Level_Step) {
                    currentp->m_throughoutConds.थप_back(
                        sampledRefOrClone(hoistVarp, exprp, flp));
                }
                i = minN - 1;
            }
            // Every repetition in the minimum prefix is required.
            SvaStateVertex* अचर condVtxp = scopedCreateVertex();
            SvaTransEdge* अचर linkp
                = guardedLink(currentp, condVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            // Only an outermost required step rejects the property. Its first check is explicit,
            // later required checks reject through the ring's throughout guard.
            यदि (is_शीर्ष_Level_Step && i == 0) linkp->m_rejectOnFail = सत्य;
            currentp = condVtxp;
        }
        // After minN: currentp is the first valid end-of-match position for [*m:n].
        यदि (m_isCoverSeq && (repp->unbounded() || repp->maxCountp())) {
            consMidSources.थप_back(currentp);
        }

        यदि (repp->unbounded()) {
            यदि (minN == 0) {
                SvaStateVertex* अचर waitVtxp = scopedCreateVertex();
                guardedEdge(currentp, waitVtxp, flp);
                SvaStateVertex* अचर checkVtxp = scopedCreateVertex();
                guardedLink(waitVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedEdge(checkVtxp, waitVtxp, flp);
                guardedLink(currentp, checkVtxp, flp);
                currentp = checkVtxp;
            } अन्यथा {
                SvaStateVertex* अचर loopBackVtxp = scopedCreateVertex();
                guardedEdge(currentp, loopBackVtxp, flp);
                SvaStateVertex* अचर reCheckVtxp = scopedCreateVertex();
                guardedLink(loopBackVtxp, reCheckVtxp, sampledRefOrClone(hoistVarp, exprp, flp),
                            flp);
                guardedEdge(reCheckVtxp, loopBackVtxp, flp);
                guardedLink(reCheckVtxp, currentp, flp);
            }
            currentp->m_isUnbounded = सत्य;
            m_inUnboundedScope = सत्य;
        } अन्यथा यदि (repp->maxCountp()) {
            अचर अचिन्हित maxN = getConstUInt(repp->maxCountp());
            SvaStateVertex* अचर mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            अचिन्हित tailMinN = minN;
            SvaStateVertex* tailStartp = currentp;
            यदि (minN == 0) {
                // Build the first optional iteration explicitly. Feeding the empty endpoint
                // directly into a range ring would incorrectly keep that match alive.
                SvaStateVertex* अचर nextVtxp = scopedCreateVertex();
                guardedEdge(currentp, nextVtxp, flp);
                SvaStateVertex* अचर checkVtxp = scopedCreateVertex();
                guardedLink(nextVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                यदि (m_isCoverSeq) consMidSources.थप_back(checkVtxp);
                tailStartp = checkVtxp;
                tailMinN = 1;
            }
            यदि (maxN > tailMinN) {
                // Add tail-ring only if the tail is non-empty.
                SvaStateVertex* अचर nextVtxp
                    = addDelayChain(tailStartp, maxN - tailMinN + 1, flp, असत्य);
                nextVtxp->m_delayRingClearCondp
                    = नूतन AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                nextVtxp->m_replayAbortReject = सत्य;
                SvaStateVertex* अचर checkVtxp = scopedCreateVertex();
                guardedLink(nextVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                यदि (m_isCoverSeq) consMidSources.थप_back(checkVtxp);
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
        फर्काऊ res;
    }

    // always[lo:hi] / s_always[lo:hi] (IEEE 1800-2023 16.12.11).
    BuildResult buildPropAlways(AstPropAlways* nodep, SvaStateVertex* entryVtxp,
                                तार्किक is_शीर्ष_Level_Step = असत्य) {
        File_पङ्क्ति* अचर flp = nodep->fileline();
        Ast_गाँठो_अभिव्यक्ति* अचर propp = nodep->propp();
        अचर अचिन्हित lo = getConstUInt(nodep->loBoundp());
        यदि (VN_IS(nodep->hiBoundp(), Unbounded)) {
            // Weak always [lo:$]: unbounded upper bound (IEEE 1800-2023 16.12.11).
            // p must hold at every clock tick at least lo cycles after the attempt
            // start; those ticks are not required to exist, so there is no
            // end-of-trace obligation (weak). The self-loop keeps the attempt live
            // every cycle; each observed cycle is a safety obligation, so a false p
            // rejects immediately.
            UASSERT_OBJ(!nodep->isStrong(), nodep, "Unbounded always must be weak (V3Width)");
            SvaStateVertex* अचर livep = addDelayChain(entryVtxp, lo, flp);
            livep->m_isUnbounded = सत्य;
            guardedEdge(livep, livep, flp);  // stay active every subsequent cycle
            SvaStateVertex* अचर sinkp = m_graph.createStateVertex();
            sinkp->m_isRejectSink = सत्य;
            SvaTransEdge* अचर rejEdgep
                = guardedLink(livep, sinkp, sampled(propp->clone_रूख_Pure(असत्य)), flp);
            यदि (is_शीर्ष_Level_Step) rejEdgep->m_rejectOnFail = सत्य;
            फर्काऊ {livep, nullptr, {}};
        }
        अचर अचिन्हित hi = getConstUInt(nodep->hiBoundp());
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
        SvaStateVertex* अचर checkp = scopedCreateVertex();
        SvaTransEdge* अचर linkp
            = guardedLink(currentp, checkp, sampled(propp->clone_रूख_Pure(असत्य)), flp);
        यदि (is_शीर्ष_Level_Step) linkp->m_rejectOnFail = सत्य;
        currentp = checkp;
        m_temporal_Guard_थाक.थप_back(propp);
        currentp = addDelayChain(currentp, hi - lo, flp);
        यदि (nodep->isStrong() && currentp->m_delayRingSize) currentp->m_strongAlwaysRing = सत्य;
        m_temporal_Guard_थाक.झिक_back();
        फर्काऊ {currentp, propp, {}};
    }

    BuildResult buildGotoRep(AstSGotoRep* repp, SvaStateVertex* entryVtxp) {
        File_पङ्क्ति* अचर flp = repp->fileline();
        Ast_गाँठो_अभिव्यक्ति* अचर exprp = repp->exprp();
        अचर अचिन्हित minN = getConstUInt(repp->countp());
        यदि (minN == 0) फर्काऊ BuildResult::fail();
        अचर तार्किक hasMax = repp->maxCountp() != nullptr;
        अचर अचिन्हित maxN = hasMax ? getConstUInt(repp->maxCountp()) : minN;
        यदि (m_isCoverSeq) {
            // Several attempts may coalesce in the one-bit wait vertex before reaching the ring,
            // so a cover sequence action block could run too few times.
            warnEndpointUnsupported(flp, "a goto repetition");
            फर्काऊ BuildResult::failWithError();
        }

        AstVar* अचर hoistVarp = tryHoistSampled(exprp, flp, 2);

        // The first guardedEdge is the ##1 before waiting for a match. In the wait state, false
        // takes the clocked self-loop, while true takes the zero-delay guardedLink on that tick.
        SvaStateVertex* अचर waitVtxp = scopedCreateVertex();
        guardedEdge(entryVtxp, waitVtxp, flp);
        guardedEdge(waitVtxp, waitVtxp,
                    नूतन AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)}, flp);
        SvaStateVertex* currentp = scopedCreateVertex();
        guardedLink(waitVtxp, currentp, sampledRefOrClone(hoistVarp, exprp, flp), flp);

        यदि (minN > 1) {
            SvaStateVertex* अचर ringVtxp = addDelayChain(
                currentp, minN - 1, flp, सत्य, nullptr, sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = सत्य;
            SvaStateVertex* अचर checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            currentp = checkVtxp;
        }
        यदि (!hasMax) {
            currentp->m_isUnbounded = सत्य;  // [->N] waits unboundedly
            m_inUnboundedScope = सत्य;
            फर्काऊ {currentp, nullptr, {}};
        }

        // [->M:N]: the range ring holds matches from M through N and advances
        // only on expr, preserving arbitrarily long gaps between occurrences.
        SvaStateVertex* अचर mergeVtxp = scopedCreateVertex();
        guardedLink(currentp, mergeVtxp, flp);  // accept at match_M
        यदि (maxN > minN) {
            SvaStateVertex* अचर ringVtxp
                = addDelayChain(currentp, maxN - minN + 1, flp, असत्य, nullptr,
                                sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = सत्य;
            SvaStateVertex* अचर checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            guardedLink(checkVtxp, mergeVtxp, flp);
        }
        mergeVtxp->m_isUnbounded = सत्य;  // [->M:N] still has unbounded waits between matches
        m_inUnboundedScope = सत्य;
        फर्काऊ {mergeVtxp, nullptr, {}};
    }

    // True if the sequence has a zero-minimum consecutive repetition, which may match empty
    स्थिर तार्किक hasZeroMinRep(अचर Ast_गाँठो_अभिव्यक्ति* seqp) {
        फर्काऊ seqp->exists(
            [](अचर AstSConsRep* repp) { फर्काऊ getConstUInt(repp->countp()) == 0; });
    }

    // True if a leaf's condition has no implication or multi-cycle sequence in it
    // (a leaf can be e.g. 'not (x |-> y)')
    स्थिर तार्किक isPlainBoolean(अचर Ast_गाँठो_अभिव्यक्ति* condp) {
        फर्काऊ !condp->exists(
            [](अचर Ast_गाँठो_अभिव्यक्ति* np) { फर्काऊ np->isMultiCycleSva() || VN_IS(np, Implication); });
    }

    // Build merge vertex for SOr / LogOr: both branches feed into one vertex.
    BuildResult buildOrMerge(Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp, SvaStateVertex* entryVtxp,
                             File_पङ्क्ति* flp, तार्किक wantBoolLeaf) {
        अचर BuildResult lhs = build_अभिव्यक्ति(lhsp, entryVtxp);
        अचर BuildResult rhs = build_अभिव्यक्ति(rhsp, entryVtxp);
        यदि (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            फर्काऊ BuildResult::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP
        // IEEE 1800-2023 16.14.3: a cover sequence counts every end-of-match. A
        // sequence operand of 'or' can end more than once, but only its final
        // end reaches the merge vertex below, so reject sequence operands rather
        // than under-count. Plain boolean disjunction has one end per cycle and
        // is handled by the OR-fold.
        अचर तार्किक booleanOnly = lhs.termVertexp == entryVtxp && lhs.finalCondp
                                 && rhs.termVertexp == entryVtxp && rhs.finalCondp;
        यदि (m_isCoverSeq && !booleanOnly) {
            warnEndpointUnsupported(flp, "a sequence operand of 'or'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            फर्काऊ BuildResult::failWithError();
        }
        यदि (booleanOnly && wantBoolLeaf) {
            यदि (isPlainBoolean(lhs.finalCondp) && isPlainBoolean(rhs.finalCondp)) {
                // As a |-> / |=> or top-level body, a disjunction of two plain booleans must
                // be able to reject: return it as a boolean finalCondp. The merge vertex
                // below drops the attempt silently when both operands are false.
                Ast_गाँठो_अभिव्यक्ति* अचर orCondp
                    = नूतन AstLogOr{flp, lhs.finalCondp->clone_रूख_Pure(असत्य),
                                   rhs.finalCondp->clone_रूख_Pure(असत्य)};
                freeUnlinkedCondp(lhs.finalCondp);
                freeUnlinkedCondp(rhs.finalCondp);
                फर्काऊ {entryVtxp, orCondp, {}};
            }
        }
        SvaStateVertex* अचर mergeVtxp = scopedCreateVertex();
        यदि (booleanOnly) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsp,
                        "Single-cycle SOr operands must have finalCondp");
            guardedLink(entryVtxp, mergeVtxp,
                        नूतन AstLogOr{flp, sampled(lhs.finalCondp->clone_रूख_Pure(असत्य)),
                                     sampled(rhs.finalCondp->clone_रूख_Pure(असत्य))},
                        flp);
        } अन्यथा {
            यदि (lhs.finalCondp) {
                guardedLink(lhs.termVertexp, mergeVtxp,
                            sampled(lhs.finalCondp->clone_रूख_Pure(असत्य)), flp);
            } अन्यथा {
                guardedLink(lhs.termVertexp, mergeVtxp, flp);
            }
            यदि (rhs.finalCondp) {
                guardedLink(rhs.termVertexp, mergeVtxp,
                            sampled(rhs.finalCondp->clone_रूख_Pure(असत्य)), flp);
            } अन्यथा {
                guardedLink(rhs.termVertexp, mergeVtxp, flp);
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        फर्काऊ {mergeVtxp, nullptr, {}};
    }

    // Build done-latch combiner for SAnd/SIntersect (IEEE 1800-2023 16.9.5).
    BuildResult buildAndCombiner(Ast_गाँठो_अभिव्यक्ति* lhsExprp, Ast_गाँठो_अभिव्यक्ति* rhsExprp,
                                 SvaStateVertex* entryVtxp, File_पङ्क्ति* flp) {
        // Snapshot-restore scope so LHS liveness does not leak into RHS.
        अचर तार्किक savedScope = m_inUnboundedScope;
        अचर BuildResult lhs = build_अभिव्यक्ति(lhsExprp, entryVtxp);
        अचर तार्किक lhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope;
        अचर BuildResult rhs = build_अभिव्यक्ति(rhsExprp, entryVtxp);
        अचर तार्किक rhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope || lhsScope || rhsScope;
        यदि (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            फर्काऊ BuildResult::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP

        // Single-cycle operands: use boolean AND (done-latch would fire across cycles).
        // If both operands stayed at entry, they must be boolean leaves which
        // buildExpr returns with finalCondp=nodep (non-null).
        यदि (lhs.termVertexp == entryVtxp && rhs.termVertexp == entryVtxp) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsExprp,
                        "Single-cycle SAnd operands must have finalCondp");
            Ast_गाँठो_अभिव्यक्ति* अचर condp = नूतन AstLogAnd{flp, lhs.finalCondp->clone_रूख_Pure(असत्य),
                                                     rhs.finalCondp->clone_रूख_Pure(असत्य)};
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            फर्काऊ {entryVtxp, condp, {}};
        }
        // Range-delay mid-window sources in either sub-branch would need
        // to be folded into the latch's match-now signal, which the
        // current combiner does not support.
        यदि (!lhs.midSources.empty() || !rhs.midSources.empty()) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: ranged cycle delay in an operand of property 'and'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            फर्काऊ BuildResult::failWithError();
        }
        SvaStateVertex* अचर combVtxp = scopedCreateVertex();
        combVtxp->m_isAndCombiner = सत्य;
        combVtxp->m_andLhsTermp = lhs.termVertexp;
        combVtxp->m_andRhsTermp = rhs.termVertexp;
        यदि (lhs.finalCondp) combVtxp->m_andLhsCondp = lhs.finalCondp->clone_रूख_Pure(असत्य);
        यदि (rhs.finalCondp) combVtxp->m_andRhsCondp = rhs.finalCondp->clone_रूख_Pure(असत्य);
        यदि (lhs.termVertexp->m_isUnbounded || rhs.termVertexp->m_isUnbounded) {
            combVtxp->m_isUnbounded = सत्य;
        }
        // Wire terminal-boolean rejects to a dedicated sink so each side can fail
        // the AND independently. Skip for liveness or single-cycle operands
        // (single-cycle termVertexp == entryVtxp would fire every cycle).
        यदि (!combVtxp->m_isUnbounded) {
            तार्किक needSink = असत्य;
            अचर तार्किक lhsMultiCycle = (lhs.termVertexp != entryVtxp);
            अचर तार्किक rhsMultiCycle = (rhs.termVertexp != entryVtxp);
            यदि (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                needSink = सत्य;
            }
            यदि (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                needSink = सत्य;
            }
            यदि (needSink) {
                SvaStateVertex* अचर sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = सत्य;
                यदि (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* अचर ep = m_graph.addLink(
                        lhs.termVertexp, sinkVtxp, sampled(lhs.finalCondp->clone_रूख_Pure(असत्य)));
                    ep->m_rejectOnFail = सत्य;
                }
                यदि (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* अचर ep = m_graph.addLink(
                        rhs.termVertexp, sinkVtxp, sampled(rhs.finalCondp->clone_रूख_Pure(असत्य)));
                    ep->m_rejectOnFail = सत्य;
                }
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        फर्काऊ {combVtxp, nullptr, {}};
    }

    // Lower `seq1 within seq2` (IEEE 1800-2023 16.9.10) as:
    //   (OR_{i in 0..slack} 1 ##i seq1 ##(slack-i) 1) intersect seq2
    // Both operands must have fixed length (no ranged cycle delays).
    // The OR lives inside a single SIntersect so one AndCombiner done-latch
    // serves all offsets; lifting it out would double-count matches where
    // multiple offsets accept on the same seq2 end cycle.
    BuildResult buildSWithin(AstSWithin* nodep, SvaStateVertex* entryVtxp,
                             तार्किक is_शीर्ष_Level_Step = असत्य) {
        अचर पूर्णाङ्क innerLen = fixedLength(nodep->lhsp());
        अचर पूर्णाङ्क outerLen = fixedLength(nodep->rhsp());
        यदि (innerLen < 0 || outerLen < 0) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: within with ranged cycle-delay operand");
            फर्काऊ BuildResult::failWithError();
        }
        यदि (innerLen > outerLen) {
            फर्काऊ buildNeverMatchIntersect(
                nodep, entryVtxp, is_शीर्ष_Level_Step,
                "the inner sequence is longer than the outer sequence");
        }
        File_पङ्क्ति* अचर flp = nodep->fileline();
        अचर पूर्णाङ्क slack = outerLen - innerLen;
        Ast_गाँठो_अभिव्यक्ति* innerOrp = nullptr;
        लागि (पूर्णाङ्क i = 0; i <= slack; ++i) {
            अचर पूर्णाङ्क postPad = slack - i;
            Ast_गाँठो_अभिव्यक्ति* branchp = nodep->lhsp()->clone_रूख_Pure(असत्य);
            यदि (i > 0) {
                AstConst* अचर prePadp = नूतन AstConst{flp, AstConst::BitTrue{}};
                AstDelay* अचर delayp
                    = नूतन AstDelay{flp, नूतन AstConst{flp, static_cast<uint32_t>(i)}, सत्य};
                AstSExpr* अचर wrapped = नूतन AstSExpr{flp, prePadp, delayp, branchp};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            यदि (postPad > 0) {
                AstConst* अचर postTruep = नूतन AstConst{flp, AstConst::BitTrue{}};
                AstDelay* अचर delayp
                    = नूतन AstDelay{flp, नूतन AstConst{flp, static_cast<uint32_t>(postPad)}, सत्य};
                AstSExpr* अचर wrapped = नूतन AstSExpr{flp, branchp, delayp, postTruep};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            innerOrp = innerOrp ? static_cast<Ast_गाँठो_अभिव्यक्ति*>(नूतन AstSOr{flp, innerOrp, branchp})
                                : branchp;
        }
        Ast_गाँठो_अभिव्यक्ति* अचर outerClonep = nodep->rhsp()->clone_रूख_Pure(असत्य);
        Ast_गाँठो_अभिव्यक्ति* अचर combinedp = नूतन AstSIntersect{flp, innerOrp, outerClonep};
        BuildResult result = build_अभिव्यक्ति(combinedp, entryVtxp, is_शीर्ष_Level_Step);
        VL_DO_DANGLING(combinedp->delete_रूख(), combinedp);
        // When both operands are plain booleans, buildAndCombiner returns a
        // freshly-allocated AstAnd as finalCondp with no parent. Callers up
        // the chain clone-and-discard finalCondp, so leave it parent-less and
        // it leaks; anchor it in the graph now via an edge.
        यदि (result.valid() && result.finalCondp && !result.finalCondp->backp()) {
            SvaStateVertex* अचर wrapVtxp = scopedCreateVertex();
            guardedLink(result.termVertexp, wrapVtxp, sampled(result.finalCondp), flp);
            result = {wrapVtxp, nullptr, result.midSources};
        }
        फर्काऊ result;
    }

    // Collect the boolean leaf checks of a fixed-length sequence keyed by their
    // clock offset from the start. Returns false for anything other than nested
    // AstSExpr with fixed cycle delays over boolean leaves (e.g. throughout).
    स्थिर तार्किक flattenFixedSeq(Ast_गाँठो_अभिव्यक्ति* nodep, पूर्णाङ्क baseOffset,
                                std::map<पूर्णाङ्क, std::vector<Ast_गाँठो_अभिव्यक्ति*>>& out) {
        यदि (AstSExpr* अचर sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* अचर delayp = VN_CAST(sexprp->delayp(), Delay);
            यदि (!delayp || !delayp->isCycleDelay() || delayp->isUnbounded()) फर्काऊ असत्य;
            अचर अचिन्हित delayCycles = getConstUInt(delayp->lhsp());
            यदि (delayp->isRangeDelay() && getConstUInt(delayp->rhsp()) != delayCycles)
                फर्काऊ असत्य;
            पूर्णाङ्क preLen = 0;
            यदि (Ast_गाँठो_अभिव्यक्ति* अचर prep = sexprp->preExprp()) {
                यदि (!flattenFixedSeq(prep, baseOffset, out)) फर्काऊ असत्य;
                preLen = fixedLength(prep);
                यदि (preLen < 0) फर्काऊ असत्य;
            }
            फर्काऊ flattenFixedSeq(sexprp->exprp(), baseOffset + preLen + delayCycles, out);
        }
        यदि (nodep->isMultiCycleSva()) फर्काऊ असत्य;
        out[baseOffset].थप_back(nodep);
        फर्काऊ सत्य;
    }

    // Conjoin two equal-length fixed sequences into one: at each clock offset
    // AND the boolean checks of both operands (IEEE 1800-2023 16.9.6 -- both
    // operands match the same window). Returns null if either operand is not a
    // plain fixed sequence of boolean leaves.
    स्थिर Ast_गाँठो_अभिव्यक्ति* conjoinFixedSeqs(Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp, File_पङ्क्ति* flp) {
        std::map<पूर्णाङ्क, std::vector<Ast_गाँठो_अभिव्यक्ति*>> checks;
        यदि (!flattenFixedSeq(lhsp, 0, checks) || !flattenFixedSeq(rhsp, 0, checks)) फर्काऊ nullptr;
        यदि (checks.empty()) फर्काऊ nullptr;
        Ast_गाँठो_अभिव्यक्ति* resultp = nullptr;
        पूर्णाङ्क prevOffset = 0;
        लागि (अचर स्वतः& offsetChecks : checks) {
            अचर पूर्णाङ्क offset = offsetChecks.first;
            Ast_गाँठो_अभिव्यक्ति* condp = nullptr;
            लागि (Ast_गाँठो_अभिव्यक्ति* अचर leafp : offsetChecks.second) {
                Ast_गाँठो_अभिव्यक्ति* अचर clonep = leafp->clone_रूख_Pure(असत्य);
                यदि (!condp) {
                    condp = clonep;
                } अन्यथा {
                    condp = नूतन AstLogAnd{flp, condp, clonep};
                    condp->dtypeSetBit();
                }
            }
            यदि (!resultp) {
                यदि (offset > 0) {
                    AstDelay* अचर delayp = नूतन AstDelay{
                        flp, नूतन AstConst{flp, static_cast<uint32_t>(offset)}, /*isCycle=*/सत्य};
                    resultp
                        = नूतन AstSExpr{flp, नूतन AstConst{flp, AstConst::BitTrue{}}, delayp, condp};
                    resultp->dtypeSetBit();
                } अन्यथा {
                    resultp = condp;
                }
            } अन्यथा {
                AstDelay* अचर delayp = नूतन AstDelay{
                    flp, नूतन AstConst{flp, static_cast<uint32_t>(offset - prevOffset)},
                    /*isCycle=*/सत्य};
                resultp = नूतन AstSExpr{flp, resultp, delayp, condp};
                resultp->dtypeSetBit();
            }
            prevOffset = offset;
        }
        फर्काऊ resultp;
    }

    // `seq` is a simple ranged sequence `start ##[m:n] end` (start/end boolean,
    // start may be absent). Used to collapse a both-variable intersect to one
    // ranged delay.
    संरचना SimpleRanged final {
        तार्किक ok = असत्य;
        Ast_गाँठो_अभिव्यक्ति* startp = nullptr;  // may be null (absent start)
        Ast_गाँठो_अभिव्यक्ति* endp = nullptr;
    };
    स्थिर SimpleRanged asSimpleRanged(Ast_गाँठो_अभिव्यक्ति* nodep) {
        AstSExpr* अचर sexprp = VN_CAST(nodep, SExpr);
        यदि (!sexprp) फर्काऊ {};
        AstDelay* अचर delayp = VN_CAST(sexprp->delayp(), Delay);
        यदि (!delayp || !delayp->isCycleDelay() || !delayp->isRangeDelay() || delayp->isUnbounded())
            फर्काऊ {};
        यदि (getConstUInt(delayp->lhsp()) == getConstUInt(delayp->rhsp())) फर्काऊ {};
        Ast_गाँठो_अभिव्यक्ति* अचर prep = sexprp->preExprp();
        यदि (prep && fixedLength(prep) != 0) फर्काऊ {};
        यदि (fixedLength(sexprp->exprp()) != 0) फर्काऊ {};
        फर्काऊ {सत्य, prep, sexprp->exprp()};
    }

    // Build the NFA for a synthesized intersect lowering tree, then free it.
    // buildExpr returns the terminal condition (finalCondp) by reference into the
    // tree; detach a clone so the tree can be freed here. The graph already holds
    // clones/hoists of every edge condition, so nothing else dangles.
    BuildResult build_From_Lowering_रूख(Ast_गाँठो_अभिव्यक्ति* treep, SvaStateVertex* entryVtxp,
                                      तार्किक is_शीर्ष_Level_Step) {
        BuildResult result = build_अभिव्यक्ति(treep, entryVtxp, is_शीर्ष_Level_Step);
        यदि (result.valid() && result.finalCondp) {
            result.finalCondp = result.finalCondp->clone_रूख_Pure(असत्य);
        }
        VL_DO_DANGLING(treep->delete_रूख(), treep);
        फर्काऊ result;
    }

    // Empty common-length intersection -- unequal fixed lengths, or disjoint
    // ranged lengths. IEEE 1800-2023 16.9.6 requires both operands to match
    // over a window of the same length, so with no common length the intersect
    // simply never matches. This is legal (matching nothing), not an error, so
    // lower to a constant false rather than rejecting legal code.
    BuildResult buildNeverMatchIntersect(Ast_गाँठो_अभिव्यक्ति* nodep, SvaStateVertex* entryVtxp,
                                         तार्किक is_शीर्ष_Level_Step, अचर वर्ण* reason) {
        nodep->v3warn(NEVERMATCH, "Sequence can never match because " << reason << ".");
        Ast_गाँठो_अभिव्यक्ति* अचर falsep = नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}};
        फर्काऊ build_From_Lowering_रूख(falsep, entryVtxp, is_शीर्ष_Level_Step);
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
        अचर std::pair<पूर्णाङ्क, पूर्णाङ्क> lhsRange = lengthRange(nodep->lhsp());
        अचर std::pair<पूर्णाङ्क, पूर्णाङ्क> rhsRange = lengthRange(nodep->rhsp());
        यदि (lhsRange.first < 0 || rhsRange.first < 0) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect with this variable-length operand");
            फर्काऊ BuildResult::failWithError();
        }
        अचर पूर्णाङ्क lo = std::max(lhsRange.first, rhsRange.first);
        अचर पूर्णाङ्क hi = std::min(lhsRange.second, rhsRange.second);
        यदि (lo > hi) {
            // Disjoint length ranges share no common length -> never matches.
            फर्काऊ buildNeverMatchIntersect(nodep, entryVtxp, is_शीर्ष_Level_Step,
                                            "intersect operands have no common length");
        }
        File_पङ्क्ति* अचर flp = nodep->fileline();
        यदि (lo == hi) {
            Ast_गाँठो_अभिव्यक्ति* अचर lp = realizeAtLength(nodep->lhsp(), lo, lhsRange.first);
            Ast_गाँठो_अभिव्यक्ति* अचर rp = realizeAtLength(nodep->rhsp(), lo, rhsRange.first);
            Ast_गाँठो_अभिव्यक्ति* अचर conjp = conjoinFixedSeqs(lp, rp, flp);
            VL_DO_DANGLING(lp->delete_रूख(), lp);
            VL_DO_DANGLING(rp->delete_रूख(), rp);
            यदि (!conjp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: intersect operand is not a plain boolean sequence");
                फर्काऊ BuildResult::failWithError();
            }
            फर्काऊ build_From_Lowering_रूख(conjp, entryVtxp, is_शीर्ष_Level_Step);
        }
        अचर SimpleRanged sl = asSimpleRanged(nodep->lhsp());
        अचर SimpleRanged sr = asSimpleRanged(nodep->rhsp());
        यदि (!sl.ok || !sr.ok) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect of two sequences that each vary in length over a"
                          " range with internal structure");
            फर्काऊ BuildResult::failWithError();
        }
        अचर स्वतः andBool = [&](Ast_गाँठो_अभिव्यक्ति* ap, Ast_गाँठो_अभिव्यक्ति* bp) -> Ast_गाँठो_अभिव्यक्ति* {
            Ast_गाँठो_अभिव्यक्ति* अचर aClonep
                = ap ? ap->clone_रूख_Pure(असत्य) : नूतन AstConst{flp, AstConst::BitTrue{}};
            Ast_गाँठो_अभिव्यक्ति* अचर bClonep
                = bp ? bp->clone_रूख_Pure(असत्य) : नूतन AstConst{flp, AstConst::BitTrue{}};
            AstLogAnd* अचर andp = नूतन AstLogAnd{flp, aClonep, bClonep};
            andp->dtypeSetBit();
            फर्काऊ andp;
        };
        AstDelay* अचर delayp = नूतन AstDelay{flp, नूतन AstConst{flp, static_cast<uint32_t>(lo)},
                                              /*isCycle=*/सत्य};
        delayp->rhsp(नूतन AstConst{flp, static_cast<uint32_t>(hi)});
        AstSExpr* अचर reducedp
            = नूतन AstSExpr{flp, andBool(sl.startp, sr.startp), delayp, andBool(sl.endp, sr.endp)};
        reducedp->dtypeSetBit();
        फर्काऊ build_From_Lowering_रूख(reducedp, entryVtxp, is_शीर्ष_Level_Step);
    }

    BuildResult buildThroughout(AstSThroughout* nodep, SvaStateVertex* entryVtxp,
                                तार्किक is_शीर्ष_Level_Step = असत्य) {
        // Mark entryVtxp so "cond false at tick 0" is detected as throughout-drop.
        entryVtxp->m_throughoutConds.थप_back(nodep->lhsp()->clone_रूख_Pure(असत्य));
        m_temporal_Guard_थाक.थप_back(nodep->lhsp());
        अचर BuildResult result = build_अभिव्यक्ति(nodep->rhsp(), entryVtxp, is_शीर्ष_Level_Step);
        m_temporal_Guard_थाक.झिक_back();
        फर्काऊ result;
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
        File_पङ्क्ति* अचर flp = nodep->fileline();
        यदि (!is_शीर्ष_Level_Step) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            फर्काऊ BuildResult::failWithError();
        }
        Ast_गाँठो_अभिव्यक्ति* अचर lhsp = nodep->lhsp();
        Ast_गाँठो_अभिव्यक्ति* अचर rhsp = nodep->rhsp();
        अचर स्वतः hasSeq = [](अचर Ast_गाँठो_अभिव्यक्ति* ep) {
            फर्काऊ ep->exists([](अचर Ast_गाँठो_अभिव्यक्ति* np) { फर्काऊ np->isMultiCycleSva(); });
        };
        यदि (hasSeq(lhsp) || hasSeq(rhsp)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            फर्काऊ BuildResult::failWithError();
        }

        अचर तार्किक ov = nodep->isOverlapping();
        Ast_गाँठो_अभिव्यक्ति* अचर lhsBitp = nodep->lhsp();
        Ast_गाँठो_अभिव्यक्ति* अचर rhsBitp = nodep->rhsp();
        // p hoist count: continue, require (ov: 1 use; nov: 1 use). At least 2 uses.
        AstVar* अचर pHoistp = tryHoistSampled(lhsBitp, flp, 2);
        // q hoist count: continue (1) + require nov (1) = 2; ov: continue only (1).
        AstVar* अचर qHoistp = ov ? nullptr : tryHoistSampled(rhsBitp, flp, 2);

        SvaStateVertex* अचर waitCp = scopedCreateVertex();
        SvaStateVertex* अचर waitRp = scopedCreateVertex();
        waitCp->m_isUnbounded = सत्य;
        waitRp->m_strongPending = nodep->isStrong() && !m_isCover;

        // Entry and back-loop Links carry no condition; throughout-folding still applies.
        guardedLink(entryVtxp, waitCp, flp);
        guardedLink(waitRp, waitCp, flp);

        // Continue clocked edge: p && !q advances to next-cycle wait.
        Ast_गाँठो_अभिव्यक्ति* अचर contCondp
            = नूतन AstLogAnd{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                            नूतन AstLogNot{flp, sampledRefOrClone(qHoistp, rhsBitp, flp)}};
        guardedEdge(waitCp, waitRp, contCondp, flp);

        // Reject sink: fires when require-condition is false.
        SvaStateVertex* अचर sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = सत्य;
        Ast_गाँठो_अभिव्यक्ति* requireCondp;
        यदि (ov) {
            requireCondp = sampledRefOrClone(pHoistp, lhsBitp, flp);
        } अन्यथा {
            requireCondp = नूतन AstLogOr{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                                        sampledRefOrClone(qHoistp, rhsBitp, flp)};
        }
        SvaTransEdge* अचर rejEdgep = m_graph.addLink(waitCp, sinkVtxp, requireCondp);
        rejEdgep->m_rejectOnFail = सत्य;

        // Accept condition rides via finalCondp; assembleResult $sampled-wraps it.
        Ast_गाँठो_अभिव्यक्ति* acceptCondp;
        यदि (ov) {
            acceptCondp
                = नूतन AstLogAnd{flp, lhsBitp->clone_रूख_Pure(असत्य), rhsBitp->clone_रूख_Pure(असत्य)};
        } अन्यथा {
            acceptCondp = rhsBitp->clone_रूख_Pure(असत्य);
        }
        फर्काऊ {waitCp, acceptCondp, {}};
    }

    // IEEE 1800-2023 16.12.14 property abort operators. Sync and async share
    // the same NFA encoding: AstSampled already gives matured values at every
    // maturing clocking event, and async firing "between clocks" is not
    // observable in a cycle-based model. VAbortKind selects accept vs reject
    // verdict (sync vs async only changes the user-visible spelling).

    // Build `condp && !outer_1 && !outer_2 ...` (unsampled).
    Ast_गाँठो_अभिव्यक्ति* abort_Fire_अभिव्यक्ति(Ast_गाँठो_अभिव्यक्ति* condp, File_पङ्क्ति* flp) {
        Ast_गाँठो_अभिव्यक्ति* resultp = condp->clone_रूख_Pure(असत्य);
        लागि (Ast_गाँठो_अभिव्यक्ति* अचर op : m_outer_Abort_थाक)
            resultp = नूतन AstLogAnd{flp, resultp, नूतन AstLogNot{flp, op->clone_रूख_Pure(असत्य)}};
        फर्काऊ resultp;
    }

    // True when unguarded ring-wide rejection or a same-tick Link chain already accounts the
    // attempt: a required-step Link covers both outcomes; followed-by pairs both edges.
    स्थिर तार्किक chainAccountsSource(अचर SvaStateVertex* srcp,
                                    अचर std::unordered_set<अचर V3GraphEdge*>& preEdges) {
        यदि (srcp->m_delayRingSize && srcp->m_throughoutConds.empty()) फर्काऊ सत्य;
        लागि (अचर V3GraphEdge& edger : srcp->inEdges()) {
            यदि (preEdges.गन्ती(&edger)) जारी;
            अचर SvaTransEdge& tedger = static_cast<अचर SvaTransEdge&>(edger);
            यदि (tedger.m_consumesCycle) जारी;
            अचर स्वतः* अचर fromp = static_cast<अचर SvaStateVertex*>(tedger.fromVtxp());
            यदि (fromp->m_abortRejectp) फर्काऊ सत्य;
        }
        तार्किक plainNonSink = असत्य;
        तार्किक markedSink = असत्य;
        लागि (अचर V3GraphEdge& edger : srcp->outEdges()) {
            यदि (preEdges.गन्ती(&edger)) जारी;
            अचर SvaTransEdge& tedger = static_cast<अचर SvaTransEdge&>(edger);
            यदि (tedger.m_consumesCycle) जारी;
            अचर तार्किक sink = static_cast<अचर SvaStateVertex*>(tedger.toVtxp())->m_isRejectSink;
            यदि (tedger.m_rejectOnFail) {
                यदि (!sink) फर्काऊ सत्य;
                markedSink = सत्य;
            } अन्यथा यदि (!sink) {
                यदि (srcp->m_abortRejectp) फर्काऊ सत्य;
                plainNonSink = सत्य;
            }
        }
        फर्काऊ plainNonSink && markedSink;
    }

    // Reject edge: fires when the source is live and the abort samples true.
    शून्य addAbortRejectEdge(SvaStateVertex* srcp, SvaStateVertex* sinkp, Ast_गाँठो_अभिव्यक्ति* condp,
                            File_पङ्क्ति* flp) {
        Ast_गाँठो_अभिव्यक्ति* अचर notFirep = नूतन AstLogNot{flp, sampled(abort_Fire_अभिव्यक्ति(condp, flp))};
        m_graph.addLink(srcp, sinkp, notFirep)->m_rejectOnFail = सत्य;
        फर्काऊ;
    }

    // On the fire tick: kill body threads; accept kinds also forgive step misses.
    शून्य gateBodyEdgesOnAbort(अचर std::unordered_set<अचर V3GraphEdge*>& preEdges,
                              Ast_गाँठो_अभिव्यक्ति* condp, VAbortKind kind, File_पङ्क्ति* flp) {
        लागि (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            लागि (V3GraphEdge& edger : vtxr.outEdges()) {
                यदि (preEdges.गन्ती(&edger)) जारी;
                SvaTransEdge* अचर tedgep = static_cast<SvaTransEdge*>(&edger);
                यदि (tedgep->m_rejectOnFail) {
                    यदि (!kind.isAccept()) जारी;
                    Ast_गाँठो_अभिव्यक्ति* अचर firep = sampled(abort_Fire_अभिव्यक्ति(condp, flp));
                    tedgep->m_condp
                        = tedgep->m_condp ? नूतन AstLogOr{flp, tedgep->m_condp, firep} : firep;
                } अन्यथा यदि (tedgep->m_consumesCycle) {
                    Ast_गाँठो_अभिव्यक्ति* अचर notFirep
                        = नूतन AstLogNot{flp, sampled(abort_Fire_अभिव्यक्ति(condp, flp))};
                    tedgep->m_condp = tedgep->m_condp
                                          ? नूतन AstLogAnd{flp, tedgep->m_condp, notFirep}
                                          : notFirep;
                }
            }
        }
    }

    BuildResult buildAbortOn(Ast_गाँठो_अभिव्यक्ति* condp, Ast_गाँठो_अभिव्यक्ति* bodyp, SvaStateVertex* entryVtxp,
                             VAbortKind kind, File_पङ्क्ति* flp, तार्किक is_शीर्ष_Level_Step) {
        // Snapshot pre-body vertices/edges so post-build diff yields the body's sub-NFA.
        std::unordered_set<अचर V3GraphVertex*> preExisting;
        std::unordered_set<अचर V3GraphEdge*> preEdges;
        लागि (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            preExisting.घुसाउ(&vtxr);
            लागि (V3GraphEdge& edger : vtxr.outEdges()) preEdges.घुसाउ(&edger);
        }

        m_outer_Abort_थाक.थप_back(condp);
        अचर BuildResult bodyResult = build_अभिव्यक्ति(bodyp, entryVtxp, is_शीर्ष_Level_Step);
        m_outer_Abort_थाक.झिक_back();
        यदि (!bodyResult.valid()) फर्काऊ bodyResult;

        gateBodyEdgesOnAbort(preEdges, condp, kind, flp);

        // Live-thread sources for the abort edge: entry + new body vertices,
        // minus reject sinks (they carry reject fuel, not live-thread fuel).
        std::vector<SvaStateVertex*> abortSources;
        abortSources.थप_back(entryVtxp);
        लागि (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            यदि (preExisting.गन्ती(&vtxr)) जारी;
            स्वतः* अचर sp = static_cast<SvaStateVertex*>(&vtxr);
            यदि (sp->m_delayRingSize || !sp->m_throughoutConds.empty()) {
                Ast_गाँठो_अभिव्यक्ति* अचर firep = abort_Fire_अभिव्यक्ति(condp, flp);
                sp->m_abortClearp
                    = sp->m_abortClearp ? नूतन AstLogOr{flp, sp->m_abortClearp, firep} : firep;
            }
            यदि (!kind.isAccept()
                && ((sp->m_delayRingSize && sp->m_throughoutConds.empty())
                    || sp->m_replayAbortReject)) {
                Ast_गाँठो_अभिव्यक्ति* अचर firep = abort_Fire_अभिव्यक्ति(condp, flp);
                sp->m_abortRejectp
                    = sp->m_abortRejectp ? नूतन AstLogOr{flp, sp->m_abortRejectp, firep} : firep;
            }
            यदि (sp->m_isRejectSink) जारी;
            abortSources.थप_back(sp);
        }

        स्वतः sampledAbortFire = [&]() -> Ast_गाँठो_अभिव्यक्ति* {
            Ast_गाँठो_अभिव्यक्ति* अचर अभिव्यक्ति = abort_Fire_अभिव्यक्ति(condp, flp);
            फर्काऊ sampled(अभिव्यक्ति);
        };

        यदि (kind.isAccept()) {
            // Match-only sink fed by $sampled(abort-fire) from every live source;
            // registered as midSource so it never contributes a reject.
            SvaStateVertex* अचर acceptSinkp = scopedCreateVertex();
            लागि (SvaStateVertex* अचर srcp : abortSources)
                guardedLink(srcp, acceptSinkp, sampledAbortFire(), flp);
            std::vector<SvaStateVertex*> midSources = bodyResult.midSources;
            midSources.थप_back(acceptSinkp);
            Ast_गाँठो_अभिव्यक्ति* finalCondp = bodyResult.finalCondp;
            यदि (finalCondp) {
                यदि (finalCondp->backp()) finalCondp = finalCondp->clone_रूख_Pure(असत्य);
                finalCondp = नूतन AstLogOr{flp, finalCondp, abort_Fire_अभिव्यक्ति(condp, flp)};
            }
            फर्काऊ {bodyResult.termVertexp, finalCondp, std::move(midSources)};
        }

        // rejectOnFail treats m_condp as the success condition and fires on
        // !condp, so the edge carries !sampledAbortFire().
        SvaStateVertex* अचर rejectSinkp = m_graph.createStateVertex();
        rejectSinkp->m_isRejectSink = सत्य;
        लागि (SvaStateVertex* अचर srcp : abortSources)
            यदि (!chainAccountsSource(srcp, preEdges))
                addAbortRejectEdge(srcp, rejectSinkp, condp, flp);
        Ast_गाँठो_अभिव्यक्ति* finalCondp = bodyResult.finalCondp;
        यदि (finalCondp) {
            यदि (finalCondp->backp()) finalCondp = finalCondp->clone_रूख_Pure(असत्य);
            finalCondp
                = नूतन AstLogAnd{flp, finalCondp, नूतन AstLogNot{flp, abort_Fire_अभिव्यक्ति(condp, flp)}};
        }
        फर्काऊ {bodyResult.termVertexp, finalCondp, bodyResult.midSources};
    }

सार्वजनिक:
    // Free a dropped sub-result condition that is not linked into the AST
    // (abort folds synthesize unparented finalCondp trees).
    स्थिर शून्य freeUnlinkedCondp(Ast_गाँठो_अभिव्यक्ति* condp) {
        यदि (condp && !condp->backp()) VL_DO_DANGLING(condp->delete_रूख(), condp);
    }

    SvaNfaBuilder(SvaGraph& graph, Ast_गाँठो_Module* modp, V3UniqueNames& propTempNames,
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
        m_temporal_Guard_थाक.clear();
    }

    BuildResult build_अभिव्यक्ति(Ast_गाँठो_अभिव्यक्ति* nodep, SvaStateVertex* entryVtxp,
                          तार्किक is_शीर्ष_Level_Step = असत्य) {
        VL_RESTORER(m_wantBoolLeaf);
        अचर तार्किक wantBoolLeaf = m_wantBoolLeaf;
        m_wantBoolLeaf = असत्य;
        यदि (AstSExpr* अचर sexprp = VN_CAST(nodep, SExpr)) {
            फर्काऊ buildSExpr(sexprp, entryVtxp, is_शीर्ष_Level_Step);
        }
        यदि (AstSConsRep* अचर repp = VN_CAST(nodep, SConsRep)) {
            फर्काऊ buildConsRep(repp, entryVtxp, is_शीर्ष_Level_Step);
        }
        यदि (AstPropAlways* अचर alwaysp = VN_CAST(nodep, PropAlways)) {
            फर्काऊ buildPropAlways(alwaysp, entryVtxp, is_शीर्ष_Level_Step);
        }
        यदि (AstSGotoRep* अचर repp = VN_CAST(nodep, SGotoRep)) {
            फर्काऊ buildGotoRep(repp, entryVtxp);
        }
        यदि (AstSThroughout* अचर throughoutp = VN_CAST(nodep, SThroughout)) {
            फर्काऊ buildThroughout(throughoutp, entryVtxp, is_शीर्ष_Level_Step);
        }
        यदि (AstSOr* अचर orp = VN_CAST(nodep, SOr)) {
            फर्काऊ buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        यदि (AstLogOr* अचर orp = VN_CAST(nodep, LogOr)) {
            फर्काऊ buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        यदि (AstSAnd* अचर andp = VN_CAST(nodep, SAnd)) {
            फर्काऊ buildAndCombiner(andp->lhsp(), andp->rhsp(), entryVtxp, andp->fileline());
        }
        यदि (AstSIntersect* अचर intp = VN_CAST(nodep, SIntersect)) {
            // IEEE 1800-2023 16.9.6: both operands match over one window with
            // equal start and end (equal length). Lower to a single sequence
            // that conjoins both operands' per-cycle checks -- correct under
            // concurrent attempts, where the done-latch combiner conflates the
            // two operands' start times and over-accepts. The combiner remains
            // only as a fallback for operands that do not flatten.
            अचर पूर्णाङ्क lhsLen = fixedLength(intp->lhsp());
            अचर पूर्णाङ्क rhsLen = fixedLength(intp->rhsp());
            यदि (lhsLen >= 0 && rhsLen >= 0) {
                यदि (lhsLen != rhsLen) {
                    // Unequal fixed lengths share no common length -> never matches.
                    फर्काऊ buildNeverMatchIntersect(intp, entryVtxp, is_शीर्ष_Level_Step,
                                                    "intersect operands have no common length");
                }
                यदि (Ast_गाँठो_अभिव्यक्ति* अचर conjp
                    = conjoinFixedSeqs(intp->lhsp(), intp->rhsp(), intp->fileline())) {
                    फर्काऊ build_From_Lowering_रूख(conjp, entryVtxp, is_शीर्ष_Level_Step);
                }
                फर्काऊ buildAndCombiner(intp->lhsp(), intp->rhsp(), entryVtxp, intp->fileline());
            }
            फर्काऊ buildVarLenIntersect(intp, entryVtxp, is_शीर्ष_Level_Step);
        }
        यदि (AstSWithin* अचर withinp = VN_CAST(nodep, SWithin)) {
            फर्काऊ buildSWithin(withinp, entryVtxp, is_शीर्ष_Level_Step);
        }
        यदि (AstAbortOn* अचर ap = VN_CAST(nodep, AbortOn)) {
            फर्काऊ buildAbortOn(ap->condp(), ap->propp(), entryVtxp, ap->kind(), ap->fileline(),
                                is_शीर्ष_Level_Step);
        }
        यदि (VN_IS(nodep, SNonConsRep)) फर्काऊ BuildResult::fail();
        यदि (AstImplication* अचर implp = VN_CAST(nodep, Implication)) {
            फर्काऊ buildImplicationEdges(implp->lhsp(), implp->rhsp(), entryVtxp,
                                         implp->isOverlapped(), implp->isFollowedBy(),
                                         implp->lhsp(), implp->fileline());
        }
        यदि (AstUntil* अचर untilp = VN_CAST(nodep, Until)) {
            फर्काऊ buildUntil(untilp, entryVtxp, is_शीर्ष_Level_Step);
        }
        // Boolean leaf (including LogAnd): return as finalCond
        फर्काऊ {entryVtxp, nodep, {}};
    }

    // Wire an implication / followed-by from `entryVtxp`: builds the antecedent,
    // emits the match-link (and for followed-by the reject-sink edge), inserts a
    // delay vertex for non-overlapped forms, and builds the body. Used both for
    // nested AstImplication in pexpr position and for the top-level assertion
    // antecedent -- `errorNodep` anchors the "unsupported sequence antecedent"
    // error, which differs between the two call sites.
    BuildResult buildImplicationEdges(Ast_गाँठो_अभिव्यक्ति* antExprp, Ast_गाँठो_अभिव्यक्ति* bodyExprp,
                                      SvaStateVertex* entryVtxp, तार्किक isOverlapped,
                                      तार्किक isFollowedBy, Ast_गाँठो* errorNodep, File_पङ्क्ति* flp) {
        अचर BuildResult antResult = build_अभिव्यक्ति(antExprp, entryVtxp);
        यदि (!antResult.valid()) फर्काऊ antResult;

        // Followed-by requires pure-boolean antecedent for non-vacuous-fail at
        // the attempt-start cycle. IEEE 1800-2023 16.12.9 permits a multi-cycle
        // sequence LHS, so this is an implementation gap rather than illegal SV.
        यदि (isFollowedBy && antResult.termVertexp != entryVtxp) {
            errorNodep->v3warn(E_UNSUPPORTED,
                               "Unsupported: sequence expression as antecedent of followed-by"
                               " (#-# / #=#) (IEEE 1800-2023 16.12.9)");
            फर्काऊ BuildResult::failWithError();
        }
        UASSERT_OBJ(!isFollowedBy || antResult.finalCondp, errorNodep,
                    "followed-by antecedent terminal at entry must carry finalCondp");

        // Use raw createStateVertex() so trigVtxp starts without liveness --
        // reaching the antecedent terminal is a definitive event.
        SvaStateVertex* अचर trigVtxp = m_graph.createStateVertex();
        यदि (antResult.finalCondp) {
            m_graph.addLink(antResult.termVertexp, trigVtxp,
                            sampled(antResult.finalCondp->clone_रूख_Pure(असत्य)));
            // Followed-by non-vacuous fail: rejectOnFail fires when the attempt
            // is live (termVtx reachable) and sampled(antecedent) is false.
            यदि (isFollowedBy) {
                SvaStateVertex* अचर sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = सत्य;
                SvaTransEdge* अचर ep
                    = m_graph.addLink(antResult.termVertexp, sinkVtxp,
                                      sampled(antResult.finalCondp->clone_रूख_Pure(असत्य)));
                ep->m_rejectOnFail = सत्य;
            }
            // finalCondp is cloned into the Sampled nodes; if the original is
            // not parented anywhere in the AST anymore it must be freed here
            // or ASan flags it as a leak (e.g. t_sequence_bool_ops).
            यदि (!antResult.finalCondp->backp()) {
                VL_DO_DANGLING(antResult.finalCondp->delete_रूख(), antResult.finalCondp);
            }
        } अन्यथा {
            m_graph.addLink(antResult.termVertexp, trigVtxp);
        }
        resetScope();

        SvaStateVertex* bodyEntryp = trigVtxp;
        यदि (!isOverlapped) {
            SvaStateVertex* अचर delayVtxp = m_graph.createStateVertex();
            m_graph.addClockedEdge(trigVtxp, delayVtxp);
            bodyEntryp = delayVtxp;
        }
        // An empty antecedent match must not start the consequent, but a zero-minimum
        // repetition's empty match is not handled here; keep the merge vertex for those
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = !isFollowedBy && !hasZeroMinRep(antExprp);
        फर्काऊ build_अभिव्यक्ति(bodyExprp, bodyEntryp, /*isTopLevelStep=*/सत्य);
    }

    BuildResult build(Ast_गाँठो_अभिव्यक्ति* exprp) {
        m_graph.m_startVertexp = scopedCreateVertex();
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = सत्य;
        फर्काऊ build_अभिव्यक्ति(exprp, m_graph.m_startVertexp, /*isTopLevelStep=*/सत्य);
    }
};

//######################################################################
// NFA Lowering (converts NFA graph to synthesizable AstAlways blocks)

वर्ग SvaNfaLowering final {
    Ast_गाँठो_Module* अचर m_modp;  // Module to add state vars and always blocks to
    Ast_गाँठो_DType* अचर m_u32DTypep;  // Shared unsigned counter dtype
    V3UniqueNames m_names{"__Vnfa"};
    माप_प्रकार m_statDelayRingEdgeVisits = 0;  // Delay-ring incoming edges visited

    // Per-lowering shared context (passed to phase sub-functions)
    // Per-vertex lowering state is stored in SvaVertexData and accessed via
    // V3GraphVertex::userp() (see SvaStateVertex::datap()).
    संरचना LowerCtx final {
        File_पङ्क्ति* flp;  // Source location for generated AST
        Ast_Sen_रूख* senTreep;  // Clock sensitivity tree
        Ast_गाँठो_अभिव्यक्ति* disableExprp;  // disable iff expression (may be nullptr)
        Ast_गाँठो_अभिव्यक्ति* matchCondp;  // Final boolean match condition (may be nullptr)
        VAssertType assertType;  // Assertion type for control tasks
        VAssertDirectiveType directiveType;  // Directive type for control tasks
        AstVar* killVarp;  // Last observed kill generation
        SvaGraph& graph;  // NFA graph
    };

    // Build a match-now expression: stateSig[i] && $sampled(condp)
    स्थिर Ast_गाँठो_अभिव्यक्ति* buildMatchNow(File_पङ्क्ति* flp, Ast_गाँठो_अभिव्यक्ति* stateExprp, Ast_गाँठो_अभिव्यक्ति* condp) {
        Ast_गाँठो_अभिव्यक्ति* अचर statep = stateExprp->clone_रूख_Pure(असत्य);
        यदि (!condp) फर्काऊ statep;
        फर्काऊ नूतन AstLogAnd{flp, statep, sampled(condp->clone_रूख_Pure(असत्य))};
    }
    स्थिर Ast_गाँठो_अभिव्यक्ति* andCond(File_पङ्क्ति* flp, Ast_गाँठो_अभिव्यक्ति* exprp, Ast_गाँठो_अभिव्यक्ति* condp) {
        यदि (!condp) फर्काऊ exprp;
        फर्काऊ नूतन AstLogAnd{flp, exprp, condp->clone_रूख_Pure(असत्य)};
    }
    // bp is always non-null; only ap can be null (serving as accumulator).
    स्थिर Ast_गाँठो_अभिव्यक्ति* orExprs(File_पङ्क्ति* flp, Ast_गाँठो_अभिव्यक्ति* ap, Ast_गाँठो_अभिव्यक्ति* bp) {
        यदि (!ap) फर्काऊ bp;
        फर्काऊ नूतन AstLogOr{flp, ap, bp};
    }
    स्थिर Ast_गाँठो_अभिव्यक्ति* addCountp(File_पङ्क्ति* अचर flp, Ast_गाँठो_अभिव्यक्ति* अचर totalCountp,
                                  Ast_गाँठो_अभिव्यक्ति* अचर contributionp,
                                  Ast_गाँठो_अभिव्यक्ति* अचर enablep = nullptr) {
        // contribution = enable ? contribution : 0;
        Ast_गाँठो_अभिव्यक्ति* अचर enabledContributionp
            = enablep ? नूतन AstCond{flp, enablep, contributionp,
                                    newTypedConstp(flp, contributionp->dtypep(), 0)}
                      : contributionp;
        यदि (!totalCountp) फर्काऊ enabledContributionp;
        फर्काऊ नूतन AstAdd{flp, totalCountp, enabledContributionp};
    }
    स्थिर Ast_गाँठो_अभिव्यक्ति* gateCheckedCountp(File_पङ्क्ति* अचर flp, Ast_गाँठो_अभिव्यक्ति* अचर countp,
                                          Ast_गाँठो_अभिव्यक्ति*& overflowp, Ast_गाँठो_अभिव्यक्ति* अचर enablep) {
        यदि (overflowp) {
            overflowp = नूतन AstLogAnd{flp, enablep->clone_रूख_Pure(असत्य), overflowp};
        }
        फर्काऊ addCountp(flp, nullptr, countp, enablep);
    }
    स्थिर Ast_गाँठो_अभिव्यक्ति* addCheckedCountp(File_पङ्क्ति* अचर flp, Ast_गाँठो_अभिव्यक्ति* अचर totalCountp,
                                         Ast_गाँठो_अभिव्यक्ति* अचर contributionp, Ast_गाँठो_अभिव्यक्ति*& overflowp,
                                         Ast_गाँठो_अभिव्यक्ति* अचर contributionOverflowp = nullptr) {
        यदि (contributionOverflowp) { overflowp = orExprs(flp, overflowp, contributionOverflowp); }
        यदि (!totalCountp) फर्काऊ contributionp;
        Ast_गाँठो_अभिव्यक्ति* अचर oldTotalp = totalCountp->clone_रूख_Pure(असत्य);
        Ast_गाँठो_अभिव्यक्ति* अचर resultp = नूतन AstAdd{flp, totalCountp, contributionp};
        overflowp
            = orExprs(flp, overflowp, नूतन AstLt{flp, resultp->clone_रूख_Pure(असत्य), oldTotalp});
        फर्काऊ resultp;
    }
    AstIf* new_गन्ती_Overflow_Warning(File_पङ्क्ति* अचर flp, Ast_गाँठो_अभिव्यक्ति* अचर condp) अचर {
        AstDisplay* अचर dispp = नूतन AstDisplay{
            flp, VDisplayType::DT_WARNING,
            "Cover sequence match count overflowed 32-bit storage; results may be incorrect.",
            nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstIf* अचर ifp = नूतन AstIf{flp, condp, dispp};
        ifp->branchPred(VBranchPred::BP_UNLIKELY);
        फर्काऊ ifp;
    }
    स्थिर Ast_गाँठो_अभिव्यक्ति* killActive(LowerCtx& c) {
        फर्काऊ नूतन AstNeq{c.flp, नूतन AstVarRef{c.flp, c.killVarp, VAccess::READ},
                          assertKillGet(c.flp, c.assertType, c.directiveType)};
    }
    स्थिर Ast_गाँठो_अभिव्यक्ति* notKillActive(LowerCtx& c) { फर्काऊ नूतन AstLogNot{c.flp, killActive(c)}; }
    स्थिर Ast_गाँठो_अभिव्यक्ति* gateNotKill(LowerCtx& c, Ast_गाँठो_अभिव्यक्ति* exprp) {
        यदि (!exprp) फर्काऊ nullptr;
        फर्काऊ नूतन AstLogAnd{c.flp, exprp, notKillActive(c)};
    }
    स्थिर Ast_गाँठो_अभिव्यक्ति* nextRingIndex(File_पङ्क्ति* flp, Ast_गाँठो_अभिव्यक्ति* idxExprp, uint32_t size) {
        अचर स्वतः u32Const = [flp](uint32_t मान) {
            फर्काऊ नूतन AstConst{flp, AstConst::Widthed_मान{}, 32, मान};
        };
        UASSERT_OBJ(size > 0, idxExprp, "Ring size must be positive");
        यदि (size == 1) {
            VL_DO_DANGLING(idxExprp->delete_रूख(), idxExprp);
            फर्काऊ u32Const(0);
        }
        // idx == size - 1 ? 0 : idx + 1
        AstAdd* अचर addp = नूतन AstAdd{flp, idxExprp->clone_रूख_Pure(असत्य), u32Const(1)};
        addp->dtypeFrom(idxExprp);
        AstCond* अचर condp
            = नूतन AstCond{flp, नूतन AstEq{flp, idxExprp, u32Const(size - 1)}, u32Const(0), addp};
        condp->dtypeFrom(idxExprp);
        फर्काऊ condp;
    }
    स्थिर Ast_गाँठो_अभिव्यक्ति* delayRingSlot(File_पङ्क्ति* flp, SvaStateVertex* vtxp, Ast_गाँठो_अभिव्यक्ति* idxExprp,
                                      VAccess access = VAccess::READ) {
        // ring[idx]
        AstVar* अचर ringp = vtxp->datap()->delayRingVarp;
        AstVarRef* अचर refp = नूतन AstVarRef{flp, ringp, access};
        यदि (vtxp->datap()->delayRingCounts) फर्काऊ नूतन AstArraySel{flp, refp, idxExprp};
        फर्काऊ नूतन AstSel{flp, refp, idxExprp, 1};
    }
    स्थिर Ast_गाँठो_अभिव्यक्ति* delay_Ring_Head_गन्ती(File_पङ्क्ति* flp, SvaStateVertex* vtxp) {
        Ast_गाँठो_अभिव्यक्ति* अचर headp
            = नूतन AstVarRef{flp, vtxp->datap()->delayRingHeadVarp, VAccess::READ};
        यदि (vtxp->datap()->delayRingCounts) फर्काऊ headp;
        फर्काऊ नूतन AstExtend{flp, headp, vtxp->datap()->delay_Ring_Live_गन्ती_Varp->dtypep()->width()};
    }
    स्थिर Ast_गाँठो_अभिव्यक्ति* delayRingAtLastIndex(File_पङ्क्ति* अचर flp, AstVar* अचर idxp,
                                             अचर uint32_t size) {
        फर्काऊ नूतन AstEq{flp, नूतन AstVarRef{flp, idxp, VAccess::READ},
                         नूतन AstConst{flp, AstConst::Widthed_मान{}, 32, size - 1}};
    }
    स्थिर Ast_गाँठो_अभिव्यक्ति* delay_Ring_Output_गन्ती(File_पङ्क्ति* अचर flp, SvaStateVertex* अचर vtxp) {
        AstVar* अचर idxp = vtxp->datap()->delayRingIdxVarp;
        अचर uint32_t size = vtxp->m_delayRingSize;
        Ast_गाँठो_अभिव्यक्ति* outgoingValidp
            = नूतन AstVarRef{flp, vtxp->datap()->delayRingWrappedVarp, VAccess::READ};
        यदि (!vtxp->m_isFixedDelayRing) {
            outgoingValidp
                = नूतन AstLogOr{flp, outgoingValidp, delayRingAtLastIndex(flp, idxp, size)};
        }
        AstVar* अचर live_गन्ती_Varp = vtxp->datap()->delay_Ring_Live_गन्ती_Varp;
        फर्काऊ नूतन AstCond{flp, outgoingValidp, delay_Ring_Head_गन्ती(flp, vtxp),
                           newTypedConstp(flp, live_गन्ती_Varp->dtypep(), 0)};
    }
    स्थिर Ast_गाँठो_अभिव्यक्ति* delayRingHasLiveThreadsp(File_पङ्क्ति* अचर flp,
                                                 AstVar* अचर live_गन्ती_Varp) {
        // active = live_count != 0;
        फर्काऊ नूतन AstNeq{flp, नूतन AstVarRef{flp, live_गन्ती_Varp, VAccess::READ},
                          newTypedConstp(flp, live_गन्ती_Varp->dtypep(), 0)};
    }

    // Phase 3 output signals
    संरचना SignalSet final {
        Ast_गाँठो_अभिव्यक्ति* terminalActivep = nullptr;  // OR of all successful terminal matches
        Ast_गाँठो_अभिव्यक्ति* matchCountp = nullptr;  // NFA paths completing the sequence this tick
        Ast_गाँठो_अभिव्यक्ति* match_गन्ती_Overflowp = nullptr;  // Match count overflow this tick
        Ast_गाँठो_अभिव्यक्ति* rejectBasep = nullptr;  // Reject when a terminal match fails
        Ast_गाँठो_अभिव्यक्ति* requiredStepRejectp = nullptr;  // Per-source reject from rejectOnFail Links
        Ast_गाँठो_अभिव्यक्ति* throughoutRejectp = nullptr;  // Reject when a throughout guard drops
        Ast_गाँठो_अभिव्यक्ति* threadFailCountp = nullptr;  // Number of threads rejected on this tick
    };

    // Disable condition also aborts attempts between clock events. A constant disable
    // never rises, and $sampled is unsupported in a sensitivity list.
    स्थिर तार्किक isAsyncDisable(अचर Ast_गाँठो_अभिव्यक्ति* अचर disableExprp) {
        फर्काऊ disableExprp && !VN_IS(disableExprp, Const)
               && !disableExprp->exists([](अचर AstSampled*) { फर्काऊ सत्य; });
    }

    // Emit the clocked update block of registers that hold pending attempts. The clearp
    // statements reset the same registers: a disable rising between clock events also
    // aborts pending attempts (IEEE 1800-2023 16.12), so clearp runs on that rising edge
    // in a separate block. It must clear unconditionally, as the disable may already be
    // false again when it runs, and the update block must not run on that edge, as it
    // would advance the attempts as if the clock had ticked. Blocks that hold no attempt
    // state, such as the $assertkill acknowledge in emitKillAckNba, do not go through here.
    // Callers pass a null clearp when the disable cannot rise between clock events.
    शून्य addStateAlways(LowerCtx& c, Ast_गाँठो* bodyp, Ast_गाँठो* clearp) {
        m_modp->addStmtsp(
            नूतन AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_रूख(असत्य), bodyp});
        यदि (!clearp) फर्काऊ;
        m_modp->addStmtsp(नूतन AstAlways{
            c.flp, VAlwaysKwd::ALWAYS,
            नूतन Ast_Sen_रूख{c.flp, नूतन AstSenItem{c.flp, VEdgeType::ET_POSEDGE,
                                                 c.disableExprp->clone_रूख_Pure(असत्य)}},
            clearp});
    }

    // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
    // Phase 2: State register NBA always block. Each clocked-edge target
    // latches the OR of its incoming contributions.
    शून्य emitStateRegisterNba(LowerCtx& c) {
        अचर तार्किक asyncDisable = isAsyncDisable(c.disableExprp);
        Ast_गाँठो* bodyp = nullptr;
        Ast_गाँठो* clearp = nullptr;
        लागि (अचर V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            अचर SvaStateVertex* अचर vtxp = static_cast<अचर SvaStateVertex*>(&vtxr);
            यदि (!vtxp->datap()->stateVarp) जारी;

            Ast_गाँठो_अभिव्यक्ति* nextStatep = nullptr;
            लागि (अचर V3GraphEdge& edger : vtxp->inEdges()) {
                अचर SvaTransEdge& tedger = static_cast<अचर SvaTransEdge&>(edger);
                यदि (!tedger.m_consumesCycle) जारी;
                अचर SvaStateVertex* अचर fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Clocked-edge source missing stateSig");

                Ast_गाँठो_अभिव्यक्ति* srcSigp = fromVtxp->datap()->stateSigp->clone_रूख_Pure(असत्य);
                srcSigp = andCond(c.flp, srcSigp, tedger.m_condp);

                यदि (c.disableExprp) {
                    Ast_गाँठो_अभिव्यक्ति* अचर notDisp
                        = नूतन AstLogNot{c.flp, c.disableExprp->clone_रूख_Pure(असत्य)};
                    srcSigp = नूतन AstLogAnd{c.flp, srcSigp, notDisp};
                }
                nextStatep = orExprs(c.flp, nextStatep, srcSigp);
            }

            UASSERT_OBJ(nextStatep, vtxp,
                        "Registered vertex has no clocked incoming contribution");
            nextStatep = gateNotKill(c, nextStatep);

            AstAssignDly* अचर assignp = नूतन AstAssignDly{
                c.flp, नूतन AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE}, nextStatep};
            bodyp = Ast_गाँठो::addNextNull(bodyp, assignp);
            यदि (asyncDisable) {
                clearp = Ast_गाँठो::addNextNull(
                    clearp,
                    नूतन AstAssignDly{
                        c.flp, नूतन AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE},
                        नूतन AstConst{c.flp, AstConst::BitFalse{}}});
            }
        }
        यदि (bodyp) addStateAlways(c, bodyp, clearp);
    }

    // Phase 2b: In-place ring storage and registered ring metadata.
    शून्य emitDelayRingUpdates(LowerCtx& c, अचर तार्किक गन्ती_Matches) {
        अचर तार्किक asyncDisable = isAsyncDisable(c.disableExprp);
        लागि (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* अचर vtxp = static_cast<SvaStateVertex*>(&vtxr);
            यदि (!vtxp->datap()->delayRingVarp) जारी;
            AstVar* अचर idxp = vtxp->datap()->delayRingIdxVarp;
            AstVar* अचर live_गन्ती_Varp = vtxp->datap()->delay_Ring_Live_गन्ती_Varp;
            AstVar* अचर wrappedp = vtxp->datap()->delayRingWrappedVarp;
            अचर uint32_t size = static_cast<uint32_t>(vtxp->m_delayRingSize);

            Ast_गाँठो_अभिव्यक्ति* incomingp = nullptr;
            Ast_गाँठो_अभिव्यक्ति* incomingOverflowp = nullptr;
            लागि (अचर V3GraphEdge& edger : vtxp->inEdges()) {
                ++m_statDelayRingEdgeVisits;
                अचर SvaTransEdge& tedger = static_cast<अचर SvaTransEdge&>(edger);
                UASSERT_OBJ(tedger.m_consumesCycle == vtxp->m_isFixedDelayRing, vtxp,
                            "Delay-ring incoming edge kind mismatch");
                अचर SvaStateVertex* अचर fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Delay-ring incoming source missing stateSig");
                UASSERT_OBJ(!गन्ती_Matches || fromVtxp->datap()->state_गन्ती_Sigp, fromVtxp,
                            "Delay-ring incoming source missing stateCountSig");
                // IEEE 1800-2023 16.14.1/16.14.3: assertion alternatives belong to one
                // evaluation; only cover sequence counts each end-of-match.
                Ast_गाँठो_अभिव्यक्ति* contribp
                    = गन्ती_Matches
                          ? fromVtxp->datap()->state_गन्ती_Sigp->clone_रूख_Pure(असत्य)
                          : नूतन AstExtend{c.flp,
                                          fromVtxp->datap()->stateSigp->clone_रूख_Pure(असत्य),
                                          m_u32DTypep->width()};
                Ast_गाँठो_अभिव्यक्ति* contribOverflowp
                    = गन्ती_Matches && fromVtxp->datap()->state_गन्ती_Overflow_Sigp
                          ? fromVtxp->datap()->state_गन्ती_Overflow_Sigp->clone_रूख_Pure(असत्य)
                          : nullptr;
                यदि (tedger.m_condp) {
                    Ast_गाँठो_अभिव्यक्ति* अचर enablep = tedger.m_condp->clone_रूख_Pure(असत्य);
                    contribp = गन्ती_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, enablep)
                                   : addCountp(c.flp, nullptr, contribp, enablep);
                }
                यदि (c.disableExprp) {
                    Ast_गाँठो_अभिव्यक्ति* अचर notDisp
                        = नूतन AstLogNot{c.flp, c.disableExprp->clone_रूख_Pure(असत्य)};
                    contribp = गन्ती_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, notDisp)
                                   : addCountp(c.flp, nullptr, contribp, notDisp);
                }
                incomingp = गन्ती_Matches ? addCheckedCountp(c.flp, incomingp, contribp,
                                                            incomingOverflowp, contribOverflowp)
                                         : addCountp(c.flp, incomingp, contribp);
            }
            UASSERT_OBJ(incomingp, vtxp, "Delay ring has no incoming edge");
            अचर तार्किक storesCounts = vtxp->datap()->delayRingCounts;
            Ast_गाँठो_अभिव्यक्ति* अचर incomingCountp
                = storesCounts
                      ? incomingp->clone_रूख_Pure(असत्य)
                      : नूतन AstExtend{c.flp,
                                      नूतन AstNeq{c.flp, incomingp->clone_रूख_Pure(असत्य),
                                                 newTypedConstp(c.flp, incomingp->dtypep(), 0)},
                                      m_u32DTypep->width()};
            Ast_गाँठो_अभिव्यक्ति* अचर storedIncomingp
                = storesCounts ? incomingp
                               : नूतन AstNeq{c.flp, incomingp,
                                            newTypedConstp(c.flp, incomingp->dtypep(), 0)};
            // Only this block accesses the storage. Other processes read the cached head, so
            // the ring can be updated in place without an NBA shadow copy.
            AstAssign* अचर writeIncomingp = नूतन AstAssign{
                c.flp,
                delayRingSlot(c.flp, vtxp, नूतन AstVarRef{c.flp, idxp, VAccess::READ},
                              VAccess::WRITE),
                storedIncomingp};
            Ast_गाँठो* updateBodyp = writeIncomingp;
            // live_count <= live_count - outgoing_count + incoming_count;
            Ast_गाँठो_अभिव्यक्ति* अचर outgoingp = delay_Ring_Output_गन्ती(c.flp, vtxp);
            Ast_गाँठो_अभिव्यक्ति* अचर afterOutgoingp
                = नूतन AstSub{c.flp, नूतन AstVarRef{c.flp, live_गन्ती_Varp, VAccess::READ}, outgoingp};
            Ast_गाँठो_अभिव्यक्ति* अचर oldAfterOutgoingp = afterOutgoingp->clone_रूख_Pure(असत्य);
            AstAdd* अचर nextLiveCountp = नूतन AstAdd{c.flp, afterOutgoingp, incomingCountp};
            यदि (storesCounts) {
                Ast_गाँठो_अभिव्यक्ति* अचर liveOverflowp
                    = नूतन AstLt{c.flp, nextLiveCountp->clone_रूख_Pure(असत्य), oldAfterOutgoingp};
                Ast_गाँठो_अभिव्यक्ति* अचर overflowp = orExprs(c.flp, incomingOverflowp, liveOverflowp);
                updateBodyp = new_गन्ती_Overflow_Warning(c.flp, overflowp);
                updateBodyp->addNext(writeIncomingp);
            } अन्यथा {
                VL_DO_DANGLING(oldAfterOutgoingp->delete_रूख(), oldAfterOutgoingp);
            }
            // Cache the outgoing slot for the next index after the write. Reading after
            // writing also handles one-slot rings and two-slot range rings without alias cases.
            Ast_गाँठो_अभिव्यक्ति* nextHeadIdxp
                = nextRingIndex(c.flp, नूतन AstVarRef{c.flp, idxp, VAccess::READ}, size);
            यदि (!vtxp->m_isFixedDelayRing) {
                nextHeadIdxp = nextRingIndex(c.flp, nextHeadIdxp, size);
            }
            updateBodyp->addNext(नूतन AstAssignDly{
                c.flp, नूतन AstVarRef{c.flp, vtxp->datap()->delayRingHeadVarp, VAccess::WRITE},
                delayRingSlot(c.flp, vtxp, nextHeadIdxp)});
            updateBodyp->addNext(नूतन AstAssignDly{
                c.flp, नूतन AstVarRef{c.flp, live_गन्ती_Varp, VAccess::WRITE}, nextLiveCountp});
            // wrapped <= wrapped || idx == size - 1;
            updateBodyp->addNext(
                नूतन AstAssignDly{c.flp, नूतन AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                 नूतन AstLogOr{c.flp, नूतन AstVarRef{c.flp, wrappedp, VAccess::READ},
                                              delayRingAtLastIndex(c.flp, idxp, size)}});
            // idx <= next_idx;
            updateBodyp->addNext(नूतन AstAssignDly{
                c.flp, नूतन AstVarRef{c.flp, idxp, VAccess::WRITE},
                nextRingIndex(c.flp, नूतन AstVarRef{c.flp, idxp, VAccess::READ}, size)});
            यदि (vtxp->m_delayRingAdvanceCondp) {
                updateBodyp = नूतन AstIf{
                    c.flp, sampled(vtxp->m_delayRingAdvanceCondp->clone_रूख_Pure(असत्य)),
                    updateBodyp};
            }

            Ast_गाँठो_अभिव्यक्ति* clearCondp = killActive(c);
            यदि (vtxp->m_delayRingClearCondp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_delayRingClearCondp->clone_रूख_Pure(असत्य)));
            }
            यदि (vtxp->m_abortClearp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_abortClearp->clone_रूख_Pure(असत्य)));
            }
            यदि (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_रूख_Pure(असत्य));
            }
            Ast_गाँठो_अभिव्यक्ति* guardp = nullptr;
            लागि (Ast_गाँठो_अभिव्यक्ति* अचर cp : vtxp->m_throughoutConds) {
                Ast_गाँठो_अभिव्यक्ति* अचर sampledp = sampled(cp->clone_रूख_Pure(असत्य));
                guardp = guardp ? static_cast<Ast_गाँठो_अभिव्यक्ति*>(नूतन AstLogAnd{c.flp, guardp, sampledp})
                                : sampledp;
            }
            यदि (guardp) clearCondp = orExprs(c.flp, clearCondp, नूतन AstLogNot{c.flp, guardp});
            // Logically clear the ring without touching its wide storage.
            AstAssignDly* अचर clearCountp
                = नूतन AstAssignDly{c.flp, नूतन AstVarRef{c.flp, live_गन्ती_Varp, VAccess::WRITE},
                                   newTypedConstp(c.flp, live_गन्ती_Varp->dtypep(), 0)};
            clearCountp->addNext(नूतन AstAssignDly{c.flp,
                                                  नूतन AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                                  नूतन AstConst{c.flp, AstConst::BitFalse{}}});
            clearCountp->addNext(नूतन AstAssignDly{c.flp,
                                                  नूतन AstVarRef{c.flp, idxp, VAccess::WRITE},
                                                  newTypedConstp(c.flp, idxp->dtypep(), 0)});
            updateBodyp = नूतन AstIf{c.flp, clearCondp, clearCountp, updateBodyp};

            addStateAlways(c, updateBodyp, asyncDisable ? clearCountp->clone_रूख(सत्य) : nullptr);
        }
    }

    // Phase 2c: SAnd combiner done-latch always block.
    // NBA semantics ensure doneL/doneR read pre-update values (IEEE 16.9.5).
    शून्य emitAndCombinerDoneLatchNba(LowerCtx& c) {
        अचर तार्किक asyncDisable = isAsyncDisable(c.disableExprp);
        लागि (अचर V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            अचर SvaStateVertex* अचर avp = static_cast<अचर SvaStateVertex*>(&vtxr);
            यदि (!avp->datap()->doneLVarp) जारी;
            // doneLVars is non-null only for AndCombiner vertices, which always
            // have both m_andLhsTermp and m_andRhsTermp set at build time.
            UASSERT_OBJ(avp->m_andLhsTermp && avp->m_andRhsTermp, avp,
                        "AndCombiner vertex missing LHS/RHS terminal");
            अचर SvaStateVertex* अचर lhsTermp = avp->m_andLhsTermp;
            अचर SvaStateVertex* अचर rhsTermp = avp->m_andRhsTermp;
            // resolveLinks' 2*N+2 fixed-point pass is guaranteed to populate
            // stateSigp on every AndCombiner and its LHS/RHS terminals.
            UASSERT_OBJ(lhsTermp->datap()->stateSigp && rhsTermp->datap()->stateSigp
                            && avp->datap()->stateSigp,
                        avp, "AndCombiner stateSigp chain unresolved after resolveLinks");

            AstAssignDly* अचर clearLp = नूतन AstAssignDly{
                c.flp, नूतन AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                नूतन AstConst{c.flp, AstConst::BitFalse{}}};
            AstAssignDly* अचर clearRp = नूतन AstAssignDly{
                c.flp, नूतन AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                नूतन AstConst{c.flp, AstConst::BitFalse{}}};
            clearLp->addNext(clearRp);

            Ast_गाँठो_अभिव्यक्ति* अचर matchLNowp
                = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, avp->m_andLhsCondp);
            Ast_गाँठो_अभिव्यक्ति* अचर matchRNowp
                = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, avp->m_andRhsCondp);
            Ast_गाँठो_अभिव्यक्ति* gateLp = matchLNowp;
            Ast_गाँठो_अभिव्यक्ति* gateRp = matchRNowp;
            यदि (c.disableExprp) {
                Ast_गाँठो_अभिव्यक्ति* अचर notDisLp
                    = नूतन AstLogNot{c.flp, c.disableExprp->clone_रूख_Pure(असत्य)};
                gateLp = नूतन AstLogAnd{c.flp, gateLp, notDisLp};
                Ast_गाँठो_अभिव्यक्ति* अचर notDisRp
                    = नूतन AstLogNot{c.flp, c.disableExprp->clone_रूख_Pure(असत्य)};
                gateRp = नूतन AstLogAnd{c.flp, gateRp, notDisRp};
            }
            AstAssignDly* अचर setLp = नूतन AstAssignDly{
                c.flp, नूतन AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                नूतन AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* अचर setLIfp = नूतन AstIf{c.flp, gateLp, setLp, nullptr};
            AstAssignDly* अचर setRp = नूतन AstAssignDly{
                c.flp, नूतन AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                नूतन AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* अचर setRIfp = नूतन AstIf{c.flp, gateRp, setRp, nullptr};
            setLIfp->addNext(setRIfp);

            Ast_गाँठो_अभिव्यक्ति* clearCondp = नूतन AstLogOr{c.flp, killActive(c),
                                                   avp->datap()->stateSigp->clone_रूख_Pure(असत्य)};
            यदि (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_रूख_Pure(असत्य));
            }
            AstIf* अचर topp = नूतन AstIf{c.flp, clearCondp, clearLp, setLIfp};
            addStateAlways(c, topp, asyncDisable ? clearLp->clone_रूख(सत्य) : nullptr);
        }
    }

    शून्य emitKillAckNba(LowerCtx& c) {
        AstAssignDly* अचर ackp
            = नूतन AstAssignDly{c.flp, नूतन AstVarRef{c.flp, c.killVarp, VAccess::WRITE},
                               assertKillGet(c.flp, c.assertType, c.directiveType)};
        m_modp->addStmtsp(
            नूतन AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_रूख(असत्य), ackp});
    }

    // Phase 3/3a/3b: Compute terminal match/reject signals, required-step reject,
    // throughout-drop reject; clean up intermediate state signals.
    // Phase 3: terminalActive and rejectBase from Links to matchVertex.
    // Builder only adds Links (non-clocked) to matchVertex via addLink in
    // wireMatchAndMidSources. For cover sequence, also count each end-of-match
    // so the action can be replayed without unrolling endpoints.
    शून्य computeTerminalMatchAndReject(LowerCtx& c, SignalSet& sigs, अचर तार्किक need_Match_गन्ती) {
        लागि (अचर SvaTransEdge* अचर tedgep : c.graph.allEdges()) {
            यदि (tedgep->toVtxp() != c.graph.m_matchVertexp) जारी;
            SvaStateVertex* अचर fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                        "Terminal-link source missing stateSig");

            Ast_गाँठो_अभिव्यक्ति* srcSigp = fromVtxp->datap()->stateSigp->clone_रूख_Pure(असत्य);
            srcSigp = andCond(c.flp, srcSigp, tedgep->m_condp);
            यदि (need_Match_गन्ती) {
                UASSERT_OBJ(fromVtxp->datap()->state_गन्ती_Sigp, fromVtxp,
                            "Terminal-link source missing stateCountSig");
                Ast_गाँठो_अभिव्यक्ति* contributionp
                    = fromVtxp->datap()->state_गन्ती_Sigp->clone_रूख_Pure(असत्य);
                Ast_गाँठो_अभिव्यक्ति* contributionOverflowp
                    = fromVtxp->datap()->state_गन्ती_Overflow_Sigp
                          ? fromVtxp->datap()->state_गन्ती_Overflow_Sigp->clone_रूख_Pure(असत्य)
                          : nullptr;
                यदि (tedgep->m_condp) {
                    contributionp = gateCheckedCountp(c.flp, contributionp, contributionOverflowp,
                                                      tedgep->m_condp->clone_रूख_Pure(असत्य));
                }
                sigs.matchCountp
                    = addCheckedCountp(c.flp, sigs.matchCountp, contributionp,
                                       sigs.match_गन्ती_Overflowp, contributionOverflowp);
            }

            यदि (fromVtxp->m_delayRingSize && !fromVtxp->m_isFixedDelayRing) {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_रूख_Pure(असत्य));
                // reject |= (wrapped || idx == size - 1) && head && final_condition;
                Ast_गाँठो_अभिव्यक्ति* expireContribp = delay_Ring_Output_गन्ती(c.flp, fromVtxp);
                expireContribp = andCond(c.flp, expireContribp, tedgep->m_condp);
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, expireContribp);
                VL_DO_DANGLING(srcSigp->delete_रूख(), srcSigp);
            } अन्यथा यदि (fromVtxp->m_isUnbounded || fromVtxp->m_isAndCombiner) {
                sigs.terminalActivep = orExprs(c.flp, sigs.terminalActivep, srcSigp);
            } अन्यथा {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_रूख_Pure(असत्य));
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, srcSigp);
            }
        }
        // wireMatchAndMidSources always adds a Link from result.termVertexp
        // to m_matchVertexp, so the loop above always sets terminalActivep.
        UASSERT_OBJ(sigs.terminalActivep, c.graph.m_matchVertexp,
                    "No terminal edge to match vertex");
    }

    Ast_गाँठो_अभिव्यक्ति* newThroughoutThreadFailCountp(LowerCtx& c, AstVar* अचर delay_Ring_Live_गन्ती_Varp,
                                               Ast_गाँठो_अभिव्यक्ति* अचर stateExprp,
                                               Ast_गाँठो_अभिव्यक्ति* अचर enablep) {
        Ast_गाँठो_अभिव्यक्ति* अचर activeThreadCountp
            = delay_Ring_Live_गन्ती_Varp
                  ? static_cast<Ast_गाँठो_अभिव्यक्ति*>(
                        नूतन AstVarRef{c.flp, delay_Ring_Live_गन्ती_Varp, VAccess::READ})
                  : नूतन AstExtend{c.flp, stateExprp->clone_रूख_Pure(असत्य), m_u32DTypep->width()};
        फर्काऊ addCountp(c.flp, nullptr, activeThreadCountp, enablep);
    }

    // Phase 3b: Throughout-drop and ring-wide abort rejection.
    शून्य computeThroughoutReject(LowerCtx& c, SignalSet& sigs, अचर तार्किक need_Thread_Fail_गन्ती) {
        लागि (अचर V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            अचर SvaStateVertex* अचर vtxp = static_cast<अचर SvaStateVertex*>(&vtxr);
            अचर स्वतः& conds = vtxp->m_throughoutConds;
            यदि (conds.empty() && !vtxp->m_abortRejectp) जारी;
            यदि (vtxp->m_isAndCombiner) जारी;
            Ast_गाँठो_अभिव्यक्ति* stateExprp = nullptr;
            यदि (vtxp->datap()->stateVarp) {
                stateExprp = नूतन AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
            } अन्यथा यदि (vtxp->datap()->delayRingVarp && vtxp->m_isFixedDelayRing) {
                stateExprp
                    = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_गन्ती_Varp);
            } अन्यथा {
                UASSERT_OBJ(vtxp->datap()->stateSigp, vtxp,
                            "Throughout-conds vertex missing state representation");
                stateExprp = vtxp->datap()->stateSigp->clone_रूख_Pure(असत्य);
            }
            Ast_गाँठो_अभिव्यक्ति* guardp = nullptr;
            लागि (Ast_गाँठो_अभिव्यक्ति* अचर cp : conds) {
                Ast_गाँठो_अभिव्यक्ति* अचर sp = sampled(cp->clone_रूख_Pure(असत्य));
                guardp = guardp ? static_cast<Ast_गाँठो_अभिव्यक्ति*>(नूतन AstLogAnd{c.flp, guardp, sp}) : sp;
            }
            यदि (vtxp->m_abortRejectp) {
                Ast_गाँठो_अभिव्यक्ति* अचर notAbortp
                    = नूतन AstLogNot{c.flp, sampled(vtxp->m_abortRejectp->clone_रूख_Pure(असत्य))};
                guardp = guardp
                             ? static_cast<Ast_गाँठो_अभिव्यक्ति*>(नूतन AstLogAnd{c.flp, guardp, notAbortp})
                             : notAbortp;
            }
            Ast_गाँठो_अभिव्यक्ति* rejectCondp = नूतन AstLogNot{c.flp, guardp};
            यदि (vtxp->m_abortClearp) {
                // Any abort clears the ring. Accept aborts suppress a simultaneous guard failure;
                // reject aborts are restored below so they still force rejection.
                Ast_गाँठो_अभिव्यक्ति* अचर notAbortClearp
                    = नूतन AstLogNot{c.flp, sampled(vtxp->m_abortClearp->clone_रूख_Pure(असत्य))};
                rejectCondp = नूतन AstLogAnd{c.flp, rejectCondp, notAbortClearp};
                यदि (vtxp->m_abortRejectp) {
                    rejectCondp = नूतन AstLogOr{
                        c.flp, rejectCondp, sampled(vtxp->m_abortRejectp->clone_रूख_Pure(असत्य))};
                }
            }
            यदि (need_Thread_Fail_गन्ती) {
                Ast_गाँठो_अभिव्यक्ति* अचर contributionp
                    = newThroughoutThreadFailCountp(c, vtxp->datap()->delay_Ring_Live_गन्ती_Varp,
                                                    stateExprp, rejectCondp->clone_रूख_Pure(असत्य));
                sigs.threadFailCountp = addCountp(c.flp, sigs.threadFailCountp, contributionp);
                यदि (vtxp->m_abortRejectp && vtxp->m_delayRingAdvanceCondp) {
                    // An advancing ring thread also occupies its same-tick match vertex in the
                    // unrolled NFA, so abort rejection must replay both fail actions.
                    Ast_गाँठो_अभिव्यक्ति* अचर abortAndAdvancep = नूतन AstLogAnd{
                        c.flp, sampled(vtxp->m_abortRejectp->clone_रूख_Pure(असत्य)),
                        sampled(vtxp->m_delayRingAdvanceCondp->clone_रूख_Pure(असत्य))};
                    Ast_गाँठो_अभिव्यक्ति* अचर matchContributionp = newThroughoutThreadFailCountp(
                        c, vtxp->datap()->delay_Ring_Live_गन्ती_Varp, stateExprp, abortAndAdvancep);
                    sigs.threadFailCountp
                        = addCountp(c.flp, sigs.threadFailCountp, matchContributionp);
                }
            }
            sigs.throughoutRejectp = orExprs(c.flp, sigs.throughoutRejectp,
                                             नूतन AstLogAnd{c.flp, stateExprp, rejectCondp});
        }
    }

    SignalSet computeSignals(LowerCtx& c, अचर तार्किक need_Thread_Fail_गन्ती,
                             अचर तार्किक need_Throughout_Thread_Fail_गन्ती, अचर तार्किक need_Match_गन्ती) {
        SignalSet sigs;

        computeTerminalMatchAndReject(c, sigs, need_Match_गन्ती);

        // Phase 3a: required-step rejection.
        // Builder only sets m_rejectOnFail on non-clocked Links with m_condp
        // or m_condVtxp, and the source always has a resolved stateSig.
        लागि (अचर SvaTransEdge* अचर tedgep : c.graph.allEdges()) {
            यदि (!tedgep->m_rejectOnFail) जारी;
            अचर SvaStateVertex* अचर fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp && (tedgep->m_condp || tedgep->m_condVtxp),
                        fromVtxp,
                        "rejectOnFail Link must have condp/condVtxp and source stateSig");
            Ast_गाँठो_अभिव्यक्ति* अचर srcSigp = fromVtxp->datap()->stateSigp->clone_रूख_Pure(असत्य);
            Ast_गाँठो_अभिव्यक्ति* condp = nullptr;
            यदि (tedgep->m_condVtxp) {
                UASSERT_OBJ(tedgep->m_condVtxp->datap()->stateSigp, tedgep->m_condVtxp,
                            "rejectOnFail condVtxp missing stateSig");
                condp = tedgep->m_condVtxp->datap()->stateSigp->clone_रूख_Pure(असत्य);
                यदि (tedgep->m_condp) {
                    condp = नूतन AstLogOr{c.flp, condp, tedgep->m_condp->clone_रूख_Pure(असत्य)};
                }
            } अन्यथा {
                condp = tedgep->m_condp->clone_रूख_Pure(असत्य);
            }
            Ast_गाँठो_अभिव्यक्ति* अचर notCondp = नूतन AstLogNot{c.flp, condp};
            Ast_गाँठो_अभिव्यक्ति* अचर rawFailp = नूतन AstLogAnd{c.flp, srcSigp, notCondp};
            यदि (need_Thread_Fail_गन्ती) {
                // thread_fail_count += fail;
                sigs.threadFailCountp = addCountp(
                    c.flp, sigs.threadFailCountp,
                    नूतन AstExtend{c.flp, rawFailp->clone_रूख_Pure(असत्य), m_u32DTypep->width()});
            }
            Ast_गाँठो_अभिव्यक्ति* अचर failp = gateNotKill(c, rawFailp);
            sigs.requiredStepRejectp = orExprs(c.flp, sigs.requiredStepRejectp, failp);
        }

        computeThroughoutReject(c, sigs, need_Throughout_Thread_Fail_गन्ती);
        यदि (sigs.threadFailCountp) {
            sigs.threadFailCountp
                = addCountp(c.flp, nullptr, sigs.threadFailCountp, notKillActive(c));
        }
        यदि (sigs.matchCountp) {
            यदि (c.matchCondp) {
                sigs.matchCountp
                    = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_गन्ती_Overflowp,
                                        sampled(c.matchCondp->clone_रूख_Pure(असत्य)));
            }
            sigs.matchCountp = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_गन्ती_Overflowp,
                                                 notKillActive(c));
        }
        sigs.terminalActivep = gateNotKill(c, sigs.terminalActivep);
        sigs.rejectBasep = gateNotKill(c, sigs.rejectBasep);
        sigs.throughoutRejectp = gateNotKill(c, sigs.throughoutRejectp);

        // Clean up intermediate state signals. These are orphan subtrees
        // (never linked into the enclosing AST); deleteTree() is immediate
        // which is what we want since stateSigp lifetime ends with this scope.
        लागि (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* अचर vtxp = static_cast<SvaStateVertex*>(&vtxr);
            Ast_गाँठो_अभिव्यक्ति*& sigp = vtxp->datap()->stateSigp;
            यदि (sigp) VL_DO_DANGLING(sigp->delete_रूख(), sigp);
            Ast_गाँठो_अभिव्यक्ति*& countp = vtxp->datap()->state_गन्ती_Sigp;
            यदि (countp) VL_DO_DANGLING(countp->delete_रूख(), countp);
            Ast_गाँठो_अभिव्यक्ति*& overflowp = vtxp->datap()->state_गन्ती_Overflow_Sigp;
            यदि (overflowp) VL_DO_DANGLING(overflowp->delete_रूख(), overflowp);
        }
        // Disable iff gating (IEEE 1800-2023 16.12) on the current level value.
        यदि (c.disableExprp) {
            // terminalActivep is always set, so gate it unconditionally.
            Ast_गाँठो_अभिव्यक्ति* अचर notTermp
                = नूतन AstLogNot{c.flp, c.disableExprp->clone_रूख_Pure(असत्य)};
            sigs.terminalActivep = नूतन AstLogAnd{c.flp, sigs.terminalActivep, notTermp};
            यदि (sigs.rejectBasep) {
                Ast_गाँठो_अभिव्यक्ति* अचर notDisp
                    = नूतन AstLogNot{c.flp, c.disableExprp->clone_रूख_Pure(असत्य)};
                sigs.rejectBasep = नूतन AstLogAnd{c.flp, sigs.rejectBasep, notDisp};
            }
            यदि (sigs.throughoutRejectp) {
                Ast_गाँठो_अभिव्यक्ति* अचर notDisp
                    = नूतन AstLogNot{c.flp, c.disableExprp->clone_रूख_Pure(असत्य)};
                sigs.throughoutRejectp = नूतन AstLogAnd{c.flp, sigs.throughoutRejectp, notDisp};
            }
            यदि (sigs.requiredStepRejectp) {
                Ast_गाँठो_अभिव्यक्ति* अचर notDisp
                    = नूतन AstLogNot{c.flp, c.disableExprp->clone_रूख_Pure(असत्य)};
                sigs.requiredStepRejectp = नूतन AstLogAnd{c.flp, sigs.requiredStepRejectp, notDisp};
            }
            यदि (sigs.matchCountp) {
                sigs.matchCountp = gateCheckedCountp(
                    c.flp, sigs.matchCountp, sigs.match_गन्ती_Overflowp,
                    नूतन AstLogNot{c.flp, c.disableExprp->clone_रूख_Pure(असत्य)});
            }
        }

        यदि (sigs.match_गन्ती_Overflowp) {
            m_modp->addStmtsp(
                नूतन AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_रूख(असत्य),
                              new_गन्ती_Overflow_Warning(c.flp, sigs.match_गन्ती_Overflowp)});
            sigs.match_गन्ती_Overflowp = nullptr;
        }

        फर्काऊ sigs;
    }

    // Phase 1: Resolve combinational Links via fixed-point propagation.
    शून्य resolveLinks(LowerCtx& c, Ast_गाँठो_अभिव्यक्ति* triggerExprp, अचर तार्किक गन्ती_Matches) {
        // datap() was freshly allocated in lower() -- all state signals start null.
        c.graph.m_startVertexp->datap()->stateSigp = triggerExprp->clone_रूख_Pure(असत्य);
        c.graph.m_startVertexp->datap()->state_गन्ती_Sigp
            = नूतन AstExtend{c.flp, triggerExprp->clone_रूख_Pure(असत्य), m_u32DTypep->width()};
        लागि (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* अचर vtxp = static_cast<SvaStateVertex*>(&vtxr);
            यदि (vtxp->datap()->stateVarp) {
                vtxp->datap()->stateSigp
                    = नूतन AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
                vtxp->datap()->state_गन्ती_Sigp = नूतन AstExtend{
                    c.flp, नूतन AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ},
                    m_u32DTypep->width()};
            } अन्यथा यदि (vtxp->datap()->delayRingVarp) {
                यदि (vtxp->m_isFixedDelayRing) {
                    // state_count = wrapped ? head : 0; state = state_count != 0;
                    Ast_गाँठो_अभिव्यक्ति* अचर countp = delay_Ring_Output_गन्ती(c.flp, vtxp);
                    vtxp->datap()->stateSigp = नूतन AstNeq{c.flp, countp->clone_रूख_Pure(असत्य),
                                                          newTypedConstp(c.flp, m_u32DTypep, 0)};
                    vtxp->datap()->state_गन्ती_Sigp = countp;
                } अन्यथा {
                    vtxp->datap()->stateSigp
                        = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_गन्ती_Varp);
                    vtxp->datap()->state_गन्ती_Sigp = नूतन AstVarRef{
                        c.flp, vtxp->datap()->delay_Ring_Live_गन्ती_Varp, VAccess::READ};
                }
            }
        }
        // Fixed-point propagation along zero-delay (Link) edges. Rebuild each
        // derived signal from its incoming edges on every pass; appending the same
        // contributions repeatedly makes merge expressions grow exponentially.
        अचर माप_प्रकार passes = 2 * c.graph.m_graph.vertices().size() + 2;
        लागि (माप_प्रकार pass = 0; pass < passes; ++pass) {
            // Rebuild SAnd combiners once both sub-NFA terminals are available.
            लागि (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* अचर vtxp = static_cast<SvaStateVertex*>(&vtxr);
                यदि (!vtxp->m_isAndCombiner) जारी;
                // AndCombiner vertices always have both terminal pointers set.
                UASSERT_OBJ(vtxp->m_andLhsTermp && vtxp->m_andRhsTermp, vtxp,
                            "AndCombiner vertex missing LHS/RHS terminal");
                अचर SvaStateVertex* अचर lhsTermp = vtxp->m_andLhsTermp;
                अचर SvaStateVertex* अचर rhsTermp = vtxp->m_andRhsTermp;
                यदि (!lhsTermp->datap()->stateSigp || !rhsTermp->datap()->stateSigp) जारी;
                Ast_गाँठो_अभिव्यक्ति* अचर matchLp
                    = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, vtxp->m_andLhsCondp);
                Ast_गाँठो_अभिव्यक्ति* अचर matchRp
                    = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, vtxp->m_andRhsCondp);
                Ast_गाँठो_अभिव्यक्ति* अचर doneLOrp = नूतन AstLogOr{
                    c.flp, नूतन AstVarRef{c.flp, vtxp->datap()->doneLVarp, VAccess::READ}, matchLp};
                Ast_गाँठो_अभिव्यक्ति* अचर doneROrp = नूतन AstLogOr{
                    c.flp, नूतन AstVarRef{c.flp, vtxp->datap()->doneRVarp, VAccess::READ}, matchRp};
                Ast_गाँठो_अभिव्यक्ति* अचर bothp = नूतन AstLogAnd{c.flp, doneLOrp, doneROrp};
                Ast_गाँठो_अभिव्यक्ति* अचर oneNowp = नूतन AstLogOr{c.flp, matchLp->clone_रूख_Pure(असत्य),
                                                          matchRp->clone_रूख_Pure(असत्य)};
                यदि (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_रूख(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = नूतन AstLogAnd{c.flp, bothp, oneNowp};
                यदि (vtxp->datap()->state_गन्ती_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_गन्ती_Sigp->delete_रूख(),
                                   vtxp->datap()->state_गन्ती_Sigp);
                }
                vtxp->datap()->state_गन्ती_Sigp = नूतन AstExtend{
                    c.flp, vtxp->datap()->stateSigp->clone_रूख_Pure(असत्य), m_u32DTypep->width()};
            }

            लागि (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* अचर vtxp = static_cast<SvaStateVertex*>(&vtxr);
                यदि (vtxp == c.graph.m_startVertexp || vtxp->datap()->stateVarp
                    || vtxp->datap()->delayRingVarp || vtxp->m_isAndCombiner || vtxp->m_isMatch
                    || vtxp->m_isRejectSink) {
                    जारी;
                }
                Ast_गाँठो_अभिव्यक्ति* nextStatep = nullptr;
                Ast_गाँठो_अभिव्यक्ति* nextStateCountp = nullptr;
                Ast_गाँठो_अभिव्यक्ति* next_State_गन्ती_Overflowp = nullptr;
                लागि (अचर V3GraphEdge& er : vtxp->inEdges()) {
                    अचर SvaTransEdge& te = static_cast<अचर SvaTransEdge&>(er);
                    अचर SvaStateVertex* अचर fromVtxp = te.fromVtxp();
                    यदि (!fromVtxp->datap()->stateSigp) जारी;
                    UASSERT_OBJ(fromVtxp->datap()->state_गन्ती_Sigp, fromVtxp,
                                "Link source missing stateCountSig");
                    Ast_गाँठो_अभिव्यक्ति* अचर contributionp = andCond(
                        c.flp, fromVtxp->datap()->stateSigp->clone_रूख_Pure(असत्य), te.m_condp);
                    nextStatep = orExprs(c.flp, nextStatep, contributionp);
                    Ast_गाँठो_अभिव्यक्ति* गन्ती_Contributionp
                        = fromVtxp->datap()->state_गन्ती_Sigp->clone_रूख_Pure(असत्य);
                    Ast_गाँठो_अभिव्यक्ति* गन्ती_Contribution_Overflowp
                        = गन्ती_Matches && fromVtxp->datap()->state_गन्ती_Overflow_Sigp
                              ? fromVtxp->datap()->state_गन्ती_Overflow_Sigp->clone_रूख_Pure(असत्य)
                              : nullptr;
                    यदि (te.m_condp) {
                        Ast_गाँठो_अभिव्यक्ति* अचर enablep = te.m_condp->clone_रूख_Pure(असत्य);
                        गन्ती_Contributionp
                            = गन्ती_Matches
                                  ? gateCheckedCountp(c.flp, गन्ती_Contributionp,
                                                      गन्ती_Contribution_Overflowp, enablep)
                                  : addCountp(c.flp, nullptr, गन्ती_Contributionp, enablep);
                    }
                    nextStateCountp
                        = गन्ती_Matches
                              ? addCheckedCountp(c.flp, nextStateCountp, गन्ती_Contributionp,
                                                 next_State_गन्ती_Overflowp,
                                                 गन्ती_Contribution_Overflowp)
                              : addCountp(c.flp, nextStateCountp, गन्ती_Contributionp);
                }
                यदि (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_रूख(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = nextStatep;
                यदि (vtxp->datap()->state_गन्ती_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_गन्ती_Sigp->delete_रूख(),
                                   vtxp->datap()->state_गन्ती_Sigp);
                }
                vtxp->datap()->state_गन्ती_Sigp = nextStateCountp;
                यदि (vtxp->datap()->state_गन्ती_Overflow_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_गन्ती_Overflow_Sigp->delete_रूख(),
                                   vtxp->datap()->state_गन्ती_Overflow_Sigp);
                }
                vtxp->datap()->state_गन्ती_Overflow_Sigp = next_State_गन्ती_Overflowp;
            }
        }
    }

सार्वजनिक:
    // Combine terminal/reject signals into final output expression.
    स्थिर Ast_गाँठो_अभिव्यक्ति* assembleResult(Ast_गाँठो_Cover_Or_Assert* अचर assertp, अचर तार्किक negated,
                                       Ast_गाँठो_अभिव्यक्ति* अचर matchCondp, अचर SignalSet& sigs,
                                       Ast_गाँठो_अभिव्यक्ति** अचर outMatchpp) {
        File_पङ्क्ति* अचर flp = assertp->fileline();
        अचर तार्किक isCover = VN_IS(assertp, Cover);
        Ast_गाँठो_अभिव्यक्ति* अचर terminalActivep = sigs.terminalActivep;
        Ast_गाँठो_अभिव्यक्ति* अचर rejectBasep = sigs.rejectBasep;
        Ast_गाँठो_अभिव्यक्ति* अचर throughoutRejectp = sigs.throughoutRejectp;
        Ast_गाँठो_अभिव्यक्ति* अचर requiredStepRejectp = sigs.requiredStepRejectp;
        // Property negation (IEEE 1800-2023 16.12.1 `not`): invert match/reject.
        यदि (negated) {
            यदि (isCover) {
                यदि (terminalActivep)
                    VL_DO_DANGLING(terminalActivep->delete_रूख(), terminalActivep);
                Ast_गाँठो_अभिव्यक्ति* negRejectp = nullptr;
                यदि (matchCondp && rejectBasep) {
                    Ast_गाँठो_अभिव्यक्ति* अचर sampledCondp = sampled(matchCondp->clone_रूख_Pure(असत्य));
                    Ast_गाँठो_अभिव्यक्ति* अचर notCondp = नूतन AstLogNot{flp, sampledCondp};
                    negRejectp = नूतन AstLogAnd{flp, rejectBasep, notCondp};
                } अन्यथा यदि (rejectBasep) {
                    VL_DO_DANGLING(rejectBasep->delete_रूख(), rejectBasep);
                }
                यदि (throughoutRejectp) negRejectp = orExprs(flp, negRejectp, throughoutRejectp);
                यदि (requiredStepRejectp)
                    negRejectp = orExprs(flp, negRejectp, requiredStepRejectp);
                फर्काऊ negRejectp ? negRejectp : नूतन AstConst{flp, AstConst::BitFalse{}};
            }
            // Negated assert/assume: output = !match.
            Ast_गाँठो_अभिव्यक्ति* matchp = terminalActivep;
            यदि (matchCondp) {
                Ast_गाँठो_अभिव्यक्ति* अचर sampledCondp = sampled(matchCondp->clone_रूख_Pure(असत्य));
                matchp = नूतन AstLogAnd{flp, matchp, sampledCondp};
            }
            यदि (outMatchpp) {
                Ast_गाँठो_अभिव्यक्ति* notPMatchp = nullptr;
                यदि (matchCondp && rejectBasep) {
                    Ast_गाँठो_अभिव्यक्ति* अचर sampledCondp = sampled(matchCondp->clone_रूख_Pure(असत्य));
                    notPMatchp = नूतन AstLogAnd{flp, rejectBasep->clone_रूख_Pure(असत्य),
                                               नूतन AstLogNot{flp, sampledCondp}};
                } अन्यथा यदि (rejectBasep) {
                    notPMatchp = rejectBasep->clone_रूख_Pure(असत्य);
                }
                यदि (throughoutRejectp)
                    notPMatchp = orExprs(flp, notPMatchp, throughoutRejectp->clone_रूख_Pure(असत्य));
                यदि (requiredStepRejectp)
                    notPMatchp
                        = orExprs(flp, notPMatchp, requiredStepRejectp->clone_रूख_Pure(असत्य));
                *outMatchpp = notPMatchp;
            }
            यदि (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_रूख(), throughoutRejectp);
            यदि (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_रूख(), rejectBasep);
            यदि (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_रूख(), requiredStepRejectp);
            Ast_गाँठो_अभिव्यक्ति* अचर resultExprp = नूतन AstLogNot{flp, matchp};
            फर्काऊ resultExprp;
        }
        यदि (isCover) {
            यदि (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_रूख(), throughoutRejectp);
            यदि (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_रूख(), rejectBasep);
            यदि (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_रूख(), requiredStepRejectp);
            यदि (matchCondp) {
                Ast_गाँठो_अभिव्यक्ति* अचर sampledCondp = sampled(matchCondp->clone_रूख_Pure(असत्य));
                फर्काऊ नूतन AstLogAnd{flp, terminalActivep, sampledCondp};
            }
            फर्काऊ terminalActivep;
        }
        // Assert/assume: output = !reject
        Ast_गाँठो_अभिव्यक्ति* rejectp = nullptr;
        यदि (matchCondp && rejectBasep) {
            Ast_गाँठो_अभिव्यक्ति* अचर sampledCondp = sampled(matchCondp->clone_रूख_Pure(असत्य));
            rejectp = नूतन AstLogAnd{flp, rejectBasep, नूतन AstLogNot{flp, sampledCondp}};
        } अन्यथा यदि (rejectBasep) {
            VL_DO_DANGLING(rejectBasep->delete_रूख(), rejectBasep);
        }
        यदि (outMatchpp) {
            Ast_गाँठो_अभिव्यक्ति* matchExprp = terminalActivep->clone_रूख_Pure(असत्य);
            यदि (matchCondp) {
                Ast_गाँठो_अभिव्यक्ति* अचर sp = sampled(matchCondp->clone_रूख_Pure(असत्य));
                matchExprp = नूतन AstLogAnd{flp, matchExprp, sp};
            }
            *outMatchpp = matchExprp;
        }
        यदि (terminalActivep) VL_DO_DANGLING(terminalActivep->delete_रूख(), terminalActivep);
        यदि (throughoutRejectp) rejectp = orExprs(flp, rejectp, throughoutRejectp);
        यदि (requiredStepRejectp) rejectp = orExprs(flp, rejectp, requiredStepRejectp);
        यदि (!rejectp) फर्काऊ नूतन AstConst{flp, AstConst::BitTrue{}};
        Ast_गाँठो_अभिव्यक्ति* अचर resultExprp = नूतन AstLogNot{flp, rejectp};
        फर्काऊ resultExprp;
    }

    explicit SvaNfaLowering(Ast_गाँठो_Module* modp)
        : m_modp{modp}
        , m_u32DTypep{modp->findBasicDType(VBasicDTypeKwd::UINT32)} {}
    ~SvaNfaLowering() {
        V3Stats::addStatSum("Assertions, NFA delay ring edge visits", m_statDelayRingEdgeVisits);
    }

    // Lower NFA graph to synthesizable AstAlways blocks and raw result signals.
    // Links are combinational; Edges are registered (NBA).
    SignalSet lower(Ast_गाँठो_Cover_Or_Assert* अचर assertp, SvaGraph& graph,
                    Ast_Sen_रूख* अचर senTreep, Ast_गाँठो_अभिव्यक्ति* अचर matchCondp,
                    Ast_गाँठो_अभिव्यक्ति* अचर disableExprp, अचर तार्किक need_Thread_Fail_गन्ती,
                    अचर तार्किक need_Throughout_Thread_Fail_गन्ती) {
        File_पङ्क्ति* अचर flp = assertp->fileline();
        AstCover* अचर coverp = VN_CAST(assertp, Cover);
        अचर तार्किक isSeqEvent = coverp && coverp->isSeqEvent();
        अचर VAssertType assertType
            = isSeqEvent ? VAssertType{VAssertType::INTERNAL} : assertp->userType();
        अचर VAssertDirectiveType directiveType
            = isSeqEvent ? VAssertDirectiveType{VAssertDirectiveType::INTERNAL}
                         : assertp->directive();
        अचर std::string baseName = m_names.get("");

        // Number vertices with sequential colors for array indexing.
        पूर्णाङ्क N = 0;
        लागि (V3GraphVertex& vtxr : graph.m_graph.vertices()) { vtxr.color(N++); }
        std::vector<SvaStateVertex*> vtx(N, nullptr);
        लागि (V3GraphVertex& vtxr : graph.m_graph.vertices()) {
            vtx[vtxr.color()] = static_cast<SvaStateVertex*>(&vtxr);
        }

        // Allocate per-vertex lowering data (stored via V3GraphVertex::userp()).
        std::vector<SvaVertexData> vertexData(N);
        लागि (पूर्णाङ्क i = 0; i < N; ++i) vtx[i]->userp(&vertexData[i]);

        // Identify registered vertices (targets of clocked edges).
        लागि (पूर्णाङ्क i = 0; i < N; ++i) {
            लागि (अचर V3GraphEdge& edger : vtx[i]->outEdges()) {
                अचर SvaTransEdge& tedger = static_cast<अचर SvaTransEdge&>(edger);
                अचर पूर्णाङ्क toIdx = tedger.toVtxp()->color();
                यदि (tedger.m_consumesCycle && tedger.toVtxp() != graph.m_matchVertexp
                    && !tedger.toVtxp()->m_isRejectSink) {
                    vtx[toIdx]->datap()->needsReg = सत्य;
                }
            }
        }

        // A packed range ring can emit several live endpoints without needing per-slot counts.
        // Widen only a ring whose input can already carry multiplicity.
        std::vector<तार्किक> stateMayHaveMultiplicity(N);
        std::vector<तार्किक> ringNeedsMultiplicity(N);
        std::vector<पूर्णाङ्क> multiplicityWork;
        अचर तार्किक गन्ती_Matches = coverp && coverp->isCoverSeq() && !isSeqEvent;
        यदि (गन्ती_Matches) {
            लागि (पूर्णाङ्क i = 0; i < N; ++i) {
                अचर तार्किक multipleInputs = vtx[i]->inEdges().size() > 1;
                अचर तार्किक isRing = vtx[i]->m_delayRingSize;
                ringNeedsMultiplicity[i] = isRing && multipleInputs;
                अचर तार्किक startsMultiplicity = isRing
                                                    ? !vtx[i]->m_isFixedDelayRing
                                                    : multipleInputs && !vtx[i]->datap()->needsReg;
                यदि (startsMultiplicity) {
                    stateMayHaveMultiplicity[i] = सत्य;
                    multiplicityWork.थप_back(i);
                }
            }
            लागि (माप_प्रकार wi = 0; wi < multiplicityWork.size(); ++wi) {
                लागि (अचर V3GraphEdge& edger : vtx[multiplicityWork[wi]]->outEdges()) {
                    अचर SvaTransEdge& tedger = static_cast<अचर SvaTransEdge&>(edger);
                    अचर पूर्णाङ्क ti = tedger.toVtxp()->color();
                    अचर तार्किक isRing = vtx[ti]->m_delayRingSize;
                    यदि (isRing) ringNeedsMultiplicity[ti] = सत्य;
                    // Match and reject sinks have no outgoing edges, so they stop naturally.
                    अचर तार्किक stopsMultiplicity = !isRing && vtx[ti]->datap()->needsReg;
                    यदि (stateMayHaveMultiplicity[ti] || stopsMultiplicity) { जारी; }
                    stateMayHaveMultiplicity[ti] = सत्य;
                    multiplicityWork.थप_back(ti);
                }
            }
        }

        अचर तार्किक asyncDisable = isAsyncDisable(disableExprp);
        // Registers cleared on a rising disable are also written by their clocked block.
        File_पङ्क्ति* अचर regFlp = asyncDisable ? नूतन File_पङ्क्ति{flp} : flp;
        यदि (asyncDisable) regFlp->warnOff(V3ErrorCode::MULTIDRIVEN, सत्य);

        AstVar* अचर killVarp
            = नूतन AstVar{flp, VVarType::MODULETEMP, baseName + "__kill", m_u32DTypep};
        killVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(killVarp);
        लागि (पूर्णाङ्क i = 0; i < N; ++i) {
            यदि (vtx[i]->m_isAndCombiner) {
                अचर std::string base = baseName + "__a" + std::to_string(i);
                AstVar* अचर lp = नूतन AstVar{regFlp, VVarType::MODULETEMP, base + "_doneL",
                                              m_modp->findBitDType()};
                lp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(lp);
                vtx[i]->datap()->doneLVarp = lp;
                AstVar* अचर rp = नूतन AstVar{regFlp, VVarType::MODULETEMP, base + "_doneR",
                                              m_modp->findBitDType()};
                rp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(rp);
                vtx[i]->datap()->doneRVarp = rp;
                जारी;
            }
            यदि (vtx[i]->m_delayRingSize) {
                अचर std::string base = baseName + "__d" + std::to_string(i);
                अचर तार्किक storesCounts = गन्ती_Matches && ringNeedsMultiplicity[i];
                Ast_गाँठो_DType* ringDTypep = nullptr;
                यदि (storesCounts) {
                    // int unsigned ring [size];
                    ringDTypep = नूतन AstUnpackArrayDType{
                        flp, m_u32DTypep,
                        नूतन AstRange{flp, static_cast<पूर्णाङ्क>(vtx[i]->m_delayRingSize - 1), 0}};
                    v3Global.rootp()->typeTablep()->addTypesp(ringDTypep);
                } अन्यथा {
                    // bit [size-1:0] ring;
                    ringDTypep = m_modp->findBitDType(vtx[i]->m_delayRingSize,
                                                      vtx[i]->m_delayRingSize, VSigning::UNSIGNED);
                }
                AstVar* अचर ringp
                    = नूतन AstVar{flp, VVarType::MODULETEMP, base + "_ring", ringDTypep};
                ringp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(ringp);
                vtx[i]->datap()->delayRingVarp = ringp;
                vtx[i]->datap()->delayRingCounts = storesCounts;
                AstVar* अचर headp
                    = नूतन AstVar{flp, VVarType::MODULETEMP, base + "_head",
                                 storesCounts ? m_u32DTypep : m_modp->findBitDType()};
                headp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(headp);
                vtx[i]->datap()->delayRingHeadVarp = headp;
                // int unsigned idx;
                AstVar* अचर idxp
                    = नूतन AstVar{regFlp, VVarType::MODULETEMP, base + "_idx", m_u32DTypep};
                idxp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(idxp);
                vtx[i]->datap()->delayRingIdxVarp = idxp;
                // int unsigned live_count;
                AstVar* अचर live_गन्ती_Varp
                    = नूतन AstVar{regFlp, VVarType::MODULETEMP, base + "_liveCount", m_u32DTypep};
                live_गन्ती_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(live_गन्ती_Varp);
                vtx[i]->datap()->delay_Ring_Live_गन्ती_Varp = live_गन्ती_Varp;
                AstVar* अचर wrappedp = नूतन AstVar{regFlp, VVarType::MODULETEMP,
                                                    base + "_wrapped", m_modp->findBitDType()};
                wrappedp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(wrappedp);
                vtx[i]->datap()->delayRingWrappedVarp = wrappedp;
                जारी;
            }
            यदि (!vtx[i]->datap()->needsReg) जारी;
            यदि (vtx[i] == graph.m_startVertexp || vtx[i]->m_isMatch) जारी;
            अचर std::string varName = baseName + "__s" + std::to_string(i);
            AstVar* अचर varp
                = नूतन AstVar{regFlp, VVarType::MODULETEMP, varName, m_modp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(varp);
            vtx[i]->datap()->stateVarp = varp;
        }

        // Build lowering context for phase sub-functions.
        LowerCtx c{flp,        senTreep,      disableExprp, matchCondp,
                   assertType, directiveType, killVarp,     graph};

        // Phase 1: Resolve combinational Links via fixed-point propagation.
        Ast_गाँठो_अभिव्यक्ति* अचर triggerExprp
            = isSeqEvent ? नूतन AstConst{flp, AstConst::BitTrue{}}
                         : assertOnCond(flp, assertp->userType(), assertp->directive());
        resolveLinks(c, triggerExprp, गन्ती_Matches);
        VL_DO_DANGLING(triggerExprp->delete_रूख(), triggerExprp);

        // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
        emitStateRegisterNba(c);
        emitDelayRingUpdates(c, गन्ती_Matches);
        emitAndCombinerDoneLatchNba(c);
        emitKillAckNba(c);

        // Phase 3/3a/3b: Compute terminal match/reject signals (cleans up stateSig).
        अचर SignalSet sigs
            = computeSignals(c, need_Thread_Fail_गन्ती, need_Throughout_Thread_Fail_गन्ती, गन्ती_Matches);

        // Strong s_always[m:n] end-of-simulation liveness: if any in-window state
        // is still set at $finish, the universal-quantifier window never completed
        // (IEEE 1800-2023 16.12.11 strong semantics). Fire the assertion failure
        // from a final block; V3Assert turns the DT_ERROR display into the standard
        // "Assertion failed in %m" message.
        // pending = |{strong state registers, strong delay live counts != 0};
        Ast_गाँठो_अभिव्यक्ति* pendingp = nullptr;
        लागि (पूर्णाङ्क i = 0; i < N; ++i) {
            यदि (!vtx[i]->m_strongPending) जारी;
            Ast_गाँठो_अभिव्यक्ति* pendingExprp = nullptr;
            यदि (vtx[i]->datap()->stateVarp) {
                pendingExprp = नूतन AstVarRef{flp, vtx[i]->datap()->stateVarp, VAccess::READ};
            } अन्यथा यदि (vtx[i]->m_strongAlwaysRing) {
                pendingExprp
                    = delayRingHasLiveThreadsp(flp, vtx[i]->datap()->delay_Ring_Live_गन्ती_Varp);
            } अन्यथा {
                जारी;
            }
            यदि (!pendingp) {
                pendingp = pendingExprp;
            } अन्यथा {
                pendingp = नूतन AstLogOr{flp, pendingp, pendingExprp};
            }
        }
        यदि (pendingp) {
            AstCExpr* अचर assertOnp
                = नूतन AstCExpr{flp, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()", 1};
            Ast_गाँठो_अभिव्यक्ति* अचर condp = नूतन AstLogAnd{flp, assertOnp, pendingp};
            AstDisplay* अचर dispp
                = नूतन AstDisplay{flp, VDisplayType::DT_ERROR, "", nullptr, nullptr};
            dispp->fmtp()->timeunit(m_modp->timeunit());
            Ast_गाँठो_Stmt* अचर firep = dispp;
            यदि (v3Global.opt.stopFail()) firep->addNext(नूतन AstStop{flp, असत्य});
            m_modp->addStmtsp(नूतन AstFinal{flp, नूतन AstIf{flp, condp, firep}});
        }

        // Clear userp on every vertex before vertexData is destroyed.
        लागि (पूर्णाङ्क i = 0; i < N; ++i) vtx[i]->userp(nullptr);
        फर्काऊ sigs;
    }
};

}  // namespace

//######################################################################
// Top-level visitor

वर्ग AssertNfaVisitor final : सार्वजनिक VNVisitor {
    // STATE
    Ast_गाँठो_Module* m_modp = nullptr;  // Current module being processed
    AstClocking* m_defaultClockingp = nullptr;  // Default clocking
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff
    SvaNfaLowering* m_loweringp = nullptr;  // NFA-to-hardware lowering engine
    Ast_Sen_रूख* m_sampled_मान_Clockp = nullptr;  // Inherited clock during scoped attachment
    V3UniqueNames m_propVarNames{"__Vpropvar"};  // Property-local variable names
    V3UniqueNames m_propTempNames{"__VnfaSampled"};  // Hoisted $sampled(propp) temps
    V3UniqueNames m_fail_गन्ती_Names{"__VnfaRemainingFailCount"};  // Fail replay counter names
    std::set<अचर AstProperty*> m_inliningProps;  // Recursion guard for inlineNamedProperty

    ढाँचा <typename T_गाँठो>
    शून्य visit_Sampled_मान(T_गाँठो* अचर nodep) {
        यदि (m_sampled_मान_Clockp && !nodep->sentreep()) {
            nodep->sentreep(m_sampled_मान_Clockp->clone_रूख(सत्य));
        }
        iterateChildren(nodep);
    }

    // Wire match vertex and mid-window sources for a successful NFA build.
    स्थिर शून्य wireMatchAndMidSources(SvaGraph& graph, अचर BuildResult& result, File_पङ्क्ति* flp) {
        graph.createMatchVertex();
        // Skip the main term Link when midSources already cover every
        // end-of-match (cover_sequence path); otherwise the per-mid extraction
        // double-counts via the merge vertex.
        यदि (!result.termIsMidMerge) { graph.addLink(result.termVertexp, graph.m_matchVertexp); }
        लागि (SvaStateVertex* srcVtxp : result.midSources) {
            Ast_गाँठो_अभिव्यक्ति* condp = nullptr;
            लागि (Ast_गाँठो_अभिव्यक्ति* अचर tc : srcVtxp->m_throughoutConds) {
                Ast_गाँठो_अभिव्यक्ति* अचर tcClone = tc->clone_रूख_Pure(असत्य);
                condp = condp ? नूतन AstLogAnd{flp, condp, tcClone} : tcClone;
            }
            graph.addLink(srcVtxp, graph.m_matchVertexp, condp);
            srcVtxp->m_isUnbounded = सत्य;
        }
    }

    स्थिर Ast_गाँठो_अभिव्यक्ति* getSequenceBodyExprp(अचर AstSequence* seqp) {
        Ast_गाँठो* bodyp = seqp->stmtsp();
        जबसम्म (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        फर्काऊ VN_CAST(bodyp, गाँठो_अभिव्यक्ति);
    }

    स्थिर AstPropSpec* getPropertySpecp(अचर AstProperty* propp) {
        Ast_गाँठो* stmtp = propp->stmtsp();
        // V3LinkParse emits InitialStaticStmt for property-local variable
        // initialisers; the InitialAutomaticStmt variant only appears for
        // task/function-scope automatic lifetime, not properties.
        जबसम्म (stmtp
               && (VN_IS(stmtp, Var) || VN_IS(stmtp, InitialStaticStmt)
                   || VN_IS(stmtp, InitialAutomaticStmt))) {  // LCOV_EXCL_LINE
            stmtp = stmtp->nextp();
        }
        फर्काऊ VN_CAST(stmtp, PropSpec);
    }

    शून्य inlineNamedProperty(AstPropSpec* outerSpecp, AstFuncRef* funcrefp,
                             अचर AstProperty* propyp) {
        // Recursion guard: IEEE 1800-2023 16.12.1 forbids recursive properties.
        // V3Width emits "Recursive property call" for direct recursion before this
        // pass runs; this catches any nested-inlining cycle that slips past.
        यदि (!m_inliningProps.emplace(propyp).second) {
            funcrefp->v3error("Illegal recursive property reference");  // LCOV_EXCL_LINE
            फर्काऊ;  // LCOV_EXCL_LINE
        }
        संरचना Guard final {
            std::set<अचर AstProperty*>& setr;
            अचर AstProperty* keyp;
            ~Guard() { setr.erase(keyp); }
        } guard{m_inliningProps, propyp};
        AstPropSpec* propSpecp = getPropertySpecp(propyp);
        UASSERT_OBJ(propSpecp, funcrefp, "Property has no body PropSpec");
        propSpecp = propSpecp->clone_रूख(असत्य);

        अचर V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propyp->stmtsp());
        std::unordered_map<अचर AstVar*, Ast_गाँठो_अभिव्यक्ति*> portMap;
        लागि (अचर स्वतः& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables to module-level temps (IEEE 16.10).
        std::unordered_map<अचर AstVar*, AstVar*> localVarMap;
        लागि (Ast_गाँठो* stmtp = propyp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstVar* अचर varp = VN_CAST(stmtp, Var)) {
                यदि (!varp->isIO()) {
                    अचर string newName = m_propVarNames.get(varp);
                    AstVar* अचर newVarp = नूतन AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        propSpecp->foreach([&](AstVarRef* refp) {
            अचर स्वतः portIt = portMap.find(refp->varp());
            यदि (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_रूख(असत्य));
                VL_DO_DANGLING(थप_Deletep(refp), refp);
                फर्काऊ;
            }
            अचर स्वतः localIt = localVarMap.find(refp->varp());
            यदि (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        लागि (अचर स्वतः& tconnect : tconnects) {
            थप_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propSpecp, funcrefp);

        // Merge disable iff (IEEE 1800-2023 16.12.1)
        यदि (outerSpecp->disablep() && propSpecp->disablep()) {
            outerSpecp->v3error("disable iff expression before property call "
                                "and in its body is not legal");
            थप_Deletep(propSpecp->disablep()->unlinkFrBack());
        }
        यदि (outerSpecp->disablep()) {
            propSpecp->disablep(outerSpecp->disablep()->unlinkFrBack());
        }

        यदि (outerSpecp->sensesp() && propSpecp->sensesp()) {
            outerSpecp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Clock event before property call and in its body");
            थप_Deletep(propSpecp->sensesp()->unlinkFrBack());
        }
        यदि (outerSpecp->sensesp()) {
            AstSenItem* अचर sensesp = outerSpecp->sensesp();
            sensesp->unlinkFrBack();
            propSpecp->sensesp(sensesp);
        }

        outerSpecp->replaceWith(propSpecp);
        VL_DO_DANGLING(थप_Deletep(outerSpecp), outerSpecp);
    }

    शून्य inlineSequenceRef(AstFuncRef* funcrefp, AstSequence* seqp) {
        Ast_गाँठो_अभिव्यक्ति* अचर bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        Ast_गाँठो_अभिव्यक्ति* अचर clonedp = bodyExprp->clone_रूख(असत्य);

        अचर V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<अचर AstVar*, Ast_गाँठो_अभिव्यक्ति*> portMap;
        लागि (अचर स्वतः& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            अचर स्वतः it = portMap.find(refp->varp());
            यदि (it != portMap.end()) {
                refp->replaceWith(it->second->clone_रूख(असत्य));
                VL_DO_DANGLING(थप_Deletep(refp), refp);
            }
        });
        लागि (अचर स्वतः& tconnect : tconnects) {
            थप_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(थप_Deletep(funcrefp), funcrefp);
        // Clear referenced flag so V3AssertPre cleanup does not emit
        // spurious UNSUPPORTED for sequences that were already inlined here.
        seqp->isReferenced(असत्य);
    }

    // Must run before hasMultiCycleExpr() so NFA sees sequence bodies.
    शून्य inlineAllSequenceRefs(Ast_गाँठो* rootp) {
        तार्किक changed = सत्य;
        जबसम्म (changed) {
            changed = असत्य;
            rootp->foreach([&](AstFuncRef* funcrefp) {
                यदि (changed) फर्काऊ;
                यदि (AstSequence* अचर seqp = VN_CAST(funcrefp->taskp(), Sequence)) {
                    inlineSequenceRef(funcrefp, seqp);
                    changed = सत्य;
                }
            });
        }
    }

    स्थिर तार्किक has_Multi_Cycle_अभिव्यक्ति(अचर Ast_गाँठो* nodep) {
        फर्काऊ nodep->exists([](अचर Ast_गाँठो* np) {
            यदि (अचर स्वतः* अचर ep = VN_CAST(np, गाँठो_अभिव्यक्ति)) फर्काऊ ep->isMultiCycleSva();
            फर्काऊ असत्य;
        });
    }

    स्थिर VPropStrength effectiveAssertPropStrength(अचर AstPropSpec* अचर propSpecp) {
        यदि (propSpecp->propStrength() != VPropStrength::DEFAULT) फर्काऊ propSpecp->propStrength();
        फर्काऊ propSpecp->fileline()->language() <= V3LangCode::L1800_2005 ? VPropStrength::STRONG
                                                                           : VPropStrength::WEAK;
    }

    // Bare `assert property (p until q)` with boolean operands stays on
    // V3AssertPre's AstLoop lowering, which preserves per-attempt action-block
    // firings that this NFA's single-bit aggregated state cannot. Strong bare
    // forms are also lowered there. NFA still owns sequence operands and any
    // embedding inside a multi-cycle context (implication consequent, or/and
    // operands, etc.).
    स्थिर तार्किक is_Bare_शीर्ष_Level_Until(Ast_गाँठो* propp) {
        Ast_गाँठो* p = propp;
        यदि (AstPropSpec* अचर specp = VN_CAST(p, PropSpec)) p = specp->propp();
        जबसम्म (AstLogNot* अचर notp = VN_CAST(p, LogNot)) p = notp->lhsp();
        AstUntil* अचर untilp = VN_CAST(p, Until);
        यदि (!untilp) फर्काऊ असत्य;
        अचर स्वतः hasSeq = [](अचर Ast_गाँठो_अभिव्यक्ति* ep) {
            फर्काऊ ep->exists([](अचर Ast_गाँठो_अभिव्यक्ति* np) { फर्काऊ np->isMultiCycleSva(); });
        };
        यदि (hasSeq(untilp->lhsp()) || hasSeq(untilp->rhsp())) फर्काऊ असत्य;
        फर्काऊ सत्य;
    }

    संरचना PropertyParts final {
        Ast_गाँठो_अभिव्यक्ति* triggerExprp = nullptr;
        Ast_गाँठो_अभिव्यक्ति* seqExprp = nullptr;
        तार्किक isOverlapped = सत्य;
        तार्किक hasImplication = असत्य;
        तार्किक isFollowedBy = असत्य;  // True for #-# / #=# (non-vacuous-fail on antecedent miss)
    };

    स्थिर PropertyParts decomposeProperty(Ast_गाँठो* propp) {
        PropertyParts parts;
        यदि (AstPropSpec* अचर specp = VN_CAST(propp, PropSpec)) { propp = specp->propp(); }
        यदि (AstImplication* अचर implp = VN_CAST(propp, Implication)) {
            parts.hasImplication = सत्य;
            parts.isOverlapped = implp->isOverlapped();
            parts.isFollowedBy = implp->isFollowedBy();
            parts.triggerExprp = implp->lhsp();
            parts.seqExprp = implp->rhsp();
        } अन्यथा यदि (Ast_गाँठो_अभिव्यक्ति* अचर exprp = VN_CAST(propp, गाँठो_अभिव्यक्ति)) {
            parts.triggerExprp = nullptr;
            parts.seqExprp = exprp;
        }
        फर्काऊ parts;
    }

    स्थिर तार्किक canSplitImplicationPassActions(अचर PropertyParts& parts) {
        UASSERT(parts.hasImplication,
                "Implication pass action split requested without implication");
        UASSERT(parts.triggerExprp, "Implication pass action split requested without trigger");
        // Direct vacuous/nonvacuous classification uses the antecedent value in the current
        // assertion attempt. Leave delayed antecedents on the existing NFA pass path.
        फर्काऊ !has_Multi_Cycle_अभिव्यक्ति(parts.triggerExprp);
    }

    स्थिर शून्य splitImplicationPassActions(AstAssert* assertp, अचर PropertyParts& parts,
                                            Ast_गाँठो_अभिव्यक्ति* nonvacuousMatchp) {
        File_पङ्क्ति* अचर flp = assertp->fileline();

        Ast_गाँठो* अचर passsp = assertp->passsp()->unlinkFrBackWithNext();
        Ast_गाँठो* splitsp = nullptr;

        यदि (!parts.isFollowedBy) {
            Ast_गाँठो_अभिव्यक्ति* अचर vacuousp
                = नूतन AstLogNot{flp, sampled(parts.triggerExprp->clone_रूख_Pure(असत्य))};
            Ast_गाँठो* अचर vacuousBodyp = passsp->clone_रूख(असत्य);
            splitsp = newPassOnIf(flp, vacuousp, vacuousBodyp, assertp->userType(),
                                  assertp->directive(), /*vacuous=*/सत्य);
        }

        AstIf* अचर nonvacuousp
            = newPassOnIf(flp, nonvacuousMatchp, passsp, assertp->userType(), assertp->directive(),
                          /*vacuous=*/असत्य);
        splitsp = splitsp ? Ast_गाँठो::addNext<Ast_गाँठो, Ast_गाँठो>(splitsp, nonvacuousp)
                          : static_cast<Ast_गाँठो*>(nonvacuousp);
        assertp->addPasssp(splitsp);
    }

    // On a PropSpec-wrapped assertion whose NFA build failed with a semantic
    // error (errorEmitted), replace the body with a BitFalse const so later
    // passes see a well-formed AST. Returns true if replaced.
    शून्य replaceBodyOnBuildError(File_पङ्क्ति* flp, AstPropSpec* propSpecp, तार्किक errorEmitted) {
        यदि (!errorEmitted) फर्काऊ;
        Ast_गाँठो* अचर innerPropp = propSpecp->propp();
        innerPropp->replaceWith(नूतन AstConst{flp, AstConst::BitFalse{}});
        VL_DO_DANGLING(थप_Deletep(innerPropp), innerPropp);
    }

    // Hoist a leading clocking event (IEEE 1800-2023 16.7):
    तार्किक hoistClockedSeq(AstPropSpec* specp) {
        जबसम्म (AstSClocked* अचर clockedp = VN_CAST(specp->propp(), SClocked)) {
            यदि (specp->sensesp()) {
                clockedp->v3warn(E_UNSUPPORTED, "Unsupported: multiclocked sequence or property");
                replaceBodyOnBuildError(specp->fileline(), specp, सत्य);
                फर्काऊ सत्य;
            }
            लागि (अचर AstSenItem* sp = clockedp->sensesp(); sp;
                 sp = VN_CAST(sp->nextp(), SenItem)) {
                यदि (!sp->edgeType().anEdge()) {
                    clockedp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: non-edge clocking event on a sequence; "
                                     "use an edge such as @(posedge clk)");
                    replaceBodyOnBuildError(specp->fileline(), specp, सत्य);
                    फर्काऊ सत्य;
                }
            }
            specp->sensesp(clockedp->sensesp()->unlinkFrBackWithNext());
            Ast_गाँठो_अभिव्यक्ति* अचर bodyp = clockedp->exprp()->unlinkFrBack();
            clockedp->replaceWith(bodyp);
            VL_DO_DANGLING(थप_Deletep(clockedp), clockedp);
        }
        // A clocking event anywhere else in the sequence is not supported.
        अचर AstSClocked* nestedp = nullptr;
        specp->propp()->foreach([&](अचर AstSClocked* p) {
            यदि (!nestedp) nestedp = p;
        });
        यदि (nestedp) {
            nestedp->v3warn(E_UNSUPPORTED,
                            "Unsupported: clocking event inside sequence expression");
            replaceBodyOnBuildError(specp->fileline(), specp, सत्य);
            फर्काऊ सत्य;
        }
        फर्काऊ असत्य;
    }

    // Build the NFA graph for a property body, handling both the antecedent
    // |-> consequent and simple sequence cases. Returns the consequent/body
    // BuildResult (invalid on parse/build failure).
    BuildResult buildAssertionGraph(SvaNfaBuilder& builder, SvaGraph& graph, Ast_गाँठो_अभिव्यक्ति* seqBodyp,
                                    अचर PropertyParts& parts, File_पङ्क्ति* flp) {
        यदि (!parts.hasImplication) फर्काऊ builder.build(seqBodyp);

        graph.m_startVertexp = graph.createStateVertex();
        फर्काऊ builder.buildImplicationEdges(parts.triggerExprp, seqBodyp, graph.m_startVertexp,
                                             parts.isOverlapped, parts.isFollowedBy,
                                             parts.triggerExprp, flp);
    }

    // Install pass-action gating and replay simultaneous per-thread failures.
    शून्य attachActionHandlers(AstAssert* अचर assertp, Ast_गाँठो_अभिव्यक्ति* matchExprp,
                              Ast_Sen_रूख* अचर threadFailReplaySenTreep,
                              Ast_गाँठो_अभिव्यक्ति* अचर threadFailCountp) {
        // Gate pass handler on match to prevent vacuous-pass firings.
        यदि (matchExprp) {
            // needMatch implies passsp() was non-null when evaluated above;
            // lowering does not mutate the assert's pass-action between the
            // two reads, so passsp() is still non-null here.
            Ast_गाँठो* passsp = assertp->passsp();
            UASSERT_OBJ(passsp, assertp, "needMatch set but passsp is null");
            passsp->unlinkFrBackWithNext();
            File_पङ्क्ति* अचर flp = assertp->fileline();
            AstIf* अचर ifp = नूतन AstIf{flp, matchExprp, passsp, nullptr};
            assertp->addPasssp(ifp);
            // Fail-handler prefix for overlapping instances (IEEE 16.12):
            // fires when reject=1 && match=1 in the same cycle.
            यदि (assertp->failsp()) assertp->prependFailsp(ifp->clone_रूख(असत्य));
        }

        यदि (threadFailCountp) {
            UASSERT_OBJ(threadFailReplaySenTreep, assertp,
                        "Thread fail count missing sensitivity tree");
            Ast_गाँठो* अचर failsp = assertp->failsp();
            File_पङ्क्ति* अचर flp = assertp->fileline();
            // IEEE 1800-2023 16.12 requires one action-block evaluation per failed
            // thread. AstAssert handles the first, so replay the rest here.
            AstVar* अचर remaining_Fail_गन्ती_Varp
                = नूतन AstVar{flp, VVarType::MODULETEMP, m_fail_गन्ती_Names.get(""),
                             m_modp->findBasicDType(VBasicDTypeKwd::UINT32)};
            remaining_Fail_गन्ती_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remaining_Fail_गन्ती_Varp);
            Ast_गाँठो* अचर replayStmtsp = नूतन AstAssign{
                flp, नूतन AstVarRef{flp, remaining_Fail_गन्ती_Varp, VAccess::WRITE}, threadFailCountp};
            AstLoop* अचर replayLoopp = नूतन AstLoop{flp};
            replayLoopp->addStmtsp(नूतन AstLoopTest{
                flp, replayLoopp,
                नूतन AstGt{flp, नूतन AstVarRef{flp, remaining_Fail_गन्ती_Varp, VAccess::READ},
                          newTypedConstp(flp, remaining_Fail_गन्ती_Varp->dtypep(), 1)}});
            replayLoopp->addStmtsp(newIfAssertFailOn(failsp->clone_रूख(सत्य), assertp->directive(),
                                                     assertp->userType()));
            AstSub* अचर decrementedFailCountp
                = नूतन AstSub{flp, नूतन AstVarRef{flp, remaining_Fail_गन्ती_Varp, VAccess::READ},
                             newTypedConstp(flp, remaining_Fail_गन्ती_Varp->dtypep(), 1)};
            replayLoopp->addStmtsp(
                नूतन AstAssign{flp, नूतन AstVarRef{flp, remaining_Fail_गन्ती_Varp, VAccess::WRITE},
                              decrementedFailCountp});
            replayStmtsp->addNext(replayLoopp);
            m_modp->addStmtsp(
                नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, threadFailReplaySenTreep, replayStmtsp});
        }
    }

    // Replace one VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). No-op if refp is not in matchMap.
    शून्य substituteMatchItemRef(AstVarRef* refp, अचिन्हित K,
                                अचर std::unordered_map<अचर AstVar*, Ast_गाँठो_अभिव्यक्ति*>& matchMap) {
        अचर स्वतः it = matchMap.find(refp->varp());
        यदि (it == matchMap.end()) फर्काऊ;
        Ast_गाँठो_अभिव्यक्ति* newp = it->second->clone_रूख_Pure(असत्य);
        यदि (K > 0) {
            AstConst* अचर ticksp = नूतन AstConst{refp->fileline(), AstConst::Widthed_मान{}, 32,
                                                  static_cast<uint32_t>(K)};
            AstPast* अचर pastp = नूतन AstPast{refp->fileline(), newp, ticksp, nullptr,
                                               /* propertyTiming */ सत्य};
            pastp->dtypeFrom(newp);
            newp = pastp;
        }
        refp->replaceWith(newp);
        VL_DO_DANGLING(थप_Deletep(refp), refp);
        फर्काऊ;
    }

    // Recursively walk a consequent. Returns cycle length consumed and
    // substitutes each VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). Reports E_UNSUPPORTED on non-constant
    // delays or composite sequence operators and returns -1 on failure.
    पूर्णाङ्क
    walk_Substitute_Match_वस्तुहरू(Ast_गाँठो_अभिव्यक्ति* nodep, अचिन्हित K,
                             अचर std::unordered_map<अचर AstVar*, Ast_गाँठो_अभिव्यक्ति*>& match_वस्तुहरू) {
        यदि (AstSExpr* अचर sexprp = VN_CAST(nodep, SExpr)) {
            // IEEE 1800-2023 16.9.2: cycle_delay's lhsp is a constant_expression
            // and the delay form in a sequence is always `##N`, folded by
            // V3Const + V3Param before V3AssertNfa. Range form `##[m:n]` is the
            // only user-visible reject here.
            AstDelay* अचर delayp = VN_AS(sexprp->delayp(), Delay);
            UASSERT_OBJ(delayp->isCycleDelay() && VN_IS(delayp->lhsp(), Const), sexprp,
                        "SVA cycle delay must have a constant lhsp");
            यदि (delayp->isRangeDelay()) {
                sexprp->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                              "non-constant cycle delay in consequent"
                                              " (IEEE 1800-2023 16.10)");
                फर्काऊ -1;
            }
            अचर अचिन्हित delayCycles = VN_AS(delayp->lhsp(), Const)->toUInt();
            पूर्णाङ्क preLen = 0;
            यदि (Ast_गाँठो_अभिव्यक्ति* अचर prep = sexprp->preExprp()) {
                preLen = walk_Substitute_Match_वस्तुहरू(prep, K, match_वस्तुहरू);
                यदि (preLen < 0) फर्काऊ -1;
            }
            अचर पूर्णाङ्क bodyLen
                = walk_Substitute_Match_वस्तुहरू(sexprp->exprp(), K + preLen + delayCycles, match_वस्तुहरू);
            यदि (bodyLen < 0) फर्काऊ -1;
            फर्काऊ preLen + delayCycles + bodyLen;
        }
        यदि (nodep->isMultiCycleSva()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                         "composite sequence operator in consequent"
                                         " (IEEE 1800-2023 16.10)");
            फर्काऊ -1;
        }
        std::vector<AstVarRef*> refs;
        nodep->foreach([&refs](AstVarRef* p) { refs.थप_back(p); });
        लागि (AstVarRef* अचर refp : refs) substituteMatchItemRef(refp, K, match_वस्तुहरू);
        फर्काऊ 0;
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
    तार्किक liftMatchItemSubstitutions(PropertyParts& parts, Ast_गाँठो_अभिव्यक्ति* seqBodyp) {
        यदि (!parts.hasImplication) फर्काऊ असत्य;
        Ast_अभिव्यक्ति_Stmt* अचर अभिव्यक्ति_Stmtp = VN_CAST(parts.triggerExprp, अभिव्यक्ति_Stmt);
        यदि (!अभिव्यक्ति_Stmtp) फर्काऊ असत्य;
        // IEEE 1800-2023 16.10 BNF requires `(expr, match_item {, match_item})`
        // with at least one match item; V3LinkParse only emits ExprStmt for
        // this form and only emits AstAssign with VarRef LHS for each item.
        std::unordered_map<अचर AstVar*, Ast_गाँठो_अभिव्यक्ति*> match_वस्तुहरू;
        लागि (Ast_गाँठो* stmtp = अभिव्यक्ति_Stmtp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstAssign* अचर assignp = VN_AS(stmtp, Assign);
            AstVarRef* अचर lhsRefp = VN_AS(assignp->lhsp(), VarRef);
            match_वस्तुहरू[lhsRefp->varp()] = assignp->rhsp();
        }
        अचर अचिन्हित startK = parts.isOverlapped ? 0 : 1;
        अचर पूर्णाङ्क length = walk_Substitute_Match_वस्तुहरू(seqBodyp, startK, match_वस्तुहरू);
        // Match-item substitution / strip mutates ancestor purity. Release
        // builds don't auto-clear caches on edits, so refresh here.
        VIsCached::clear_Cache_रूख();
        यदि (length < 0) फर्काऊ सत्य;
        Ast_गाँठो_अभिव्यक्ति* अचर antBoolp = अभिव्यक्ति_Stmtp->resultp()->unlinkFrBack();
        अभिव्यक्ति_Stmtp->replaceWith(antBoolp);
        VL_DO_DANGLING(थप_Deletep(अभिव्यक्ति_Stmtp), अभिव्यक्ति_Stmtp);
        parts.triggerExprp = antBoolp;
        फर्काऊ असत्य;
    }

    // Outcome counts for a property if/case are wrong in an outcome-multiplying
    // context. Returns that context, or nullptr when the shape is supported.
    स्थिर अचर वर्ण* unsupportedPropertyControl(अचर Ast_गाँठो_Cover_Or_Assert* assertp,
                                                  अचर Ast_गाँठो_अभिव्यक्ति* seqBodyp, तार्किक negated) {
        यदि (!hasPropertyControlConjunction(seqBodyp)) फर्काऊ nullptr;
        यदि (negated) फर्काऊ "negation";
        यदि (VN_IS(assertp, Cover)) फर्काऊ "cover";
        यदि (VN_AS(assertp, Assert)->passsp()) फर्काऊ "a pass action";
        फर्काऊ nullptr;
    }

    // Inline property/sequence refs and reject unsupported shapes.
    // Returns the PropSpec to lower, or nullptr when fully handled here.
    AstPropSpec* prepareAssertionProp(Ast_गाँठो_Cover_Or_Assert* assertp) {
        यदि (AstPropSpec* अचर specp = VN_CAST(assertp->propp(), PropSpec)) {
            यदि (AstFuncRef* अचर funcrefp = VN_CAST(specp->propp(), FuncRef)) {
                यदि (अचर AstProperty* अचर propyp = VN_CAST(funcrefp->taskp(), Property)) {
                    inlineNamedProperty(specp, funcrefp, propyp);
                }
            }
        }

        inlineAllSequenceRefs(assertp->propp());

        यदि (AstPropSpec* अचर specp = VN_CAST(assertp->propp(), PropSpec)) {
            यदि (hoistClockedSeq(specp)) फर्काऊ nullptr;
        }

        AstPropSpec* अचर propp = VN_AS(assertp->propp(), PropSpec);
        यदि (!VN_IS(assertp, Cover)
            && effectiveAssertPropStrength(propp) == VPropStrength::STRONG) {
            propp->v3warn(E_UNSUPPORTED,
                          "Unsupported: strong property in " + assertp->verilogKwd() + ".");
            replaceBodyOnBuildError(assertp->fileline(), propp, /*errorEmitted=*/सत्य);
            फर्काऊ nullptr;
        }

        यदि (!has_Multi_Cycle_अभिव्यक्ति(propp)) फर्काऊ nullptr;
        // A nested property instance keeps its body behind the call; lowering would drop it.
        यदि (propp->exists([](अचर AstFuncRef* refp) { फर्काऊ VN_IS(refp->taskp(), Property); })) {
            assertp->v3warn(E_UNSUPPORTED,
                            "Unsupported: property instance inside a multi-cycle property "
                            "expression");
            VL_DO_DANGLING(थप_Deletep(assertp->unlinkFrBack()), assertp);
            फर्काऊ nullptr;
        }
        यदि (is_Bare_शीर्ष_Level_Until(propp)) फर्काऊ nullptr;
        फर्काऊ propp;
    }

    शून्य processAssertion(Ast_गाँठो_Cover_Or_Assert* assertp) {
        यदि (assertp->immediate()) फर्काऊ;

        AstPropSpec* अचर propp = prepareAssertionProp(assertp);
        यदि (!propp) फर्काऊ;

        PropertyParts parts = decomposeProperty(propp);
        UASSERT_OBJ(parts.seqExprp, propp, "Property body must be an expression");

        // Unwrap `not` (IEEE 1800-2023 16.12.1); odd count -> negated semantics.
        Ast_गाँठो_अभिव्यक्ति* seqBodyp = parts.seqExprp;
        तार्किक negated = असत्य;
        जबसम्म (AstLogNot* अचर notp = VN_CAST(seqBodyp, LogNot)) {
            negated = !negated;
            seqBodyp = notp->lhsp();
        }

        अचर वर्ण* अचर propertyControlp
            = unsupportedPropertyControl(assertp, seqBodyp, negated);

        // Substitute property-local match-item refs in consequent with
        // $past(rhs, K) before NFA build (IEEE 1800-2023 16.10).
        यदि (liftMatchItemSubstitutions(parts, seqBodyp)) {
            AstPropSpec* अचर psp = VN_CAST(assertp->propp(), PropSpec);
            UASSERT_OBJ(psp, assertp, "Concurrent assertion must have PropSpec");
            replaceBodyOnBuildError(assertp->fileline(), psp, /*errorEmitted=*/सत्य);
            फर्काऊ;
        }

        AstCover* अचर coverp = VN_CAST(assertp, Cover);
        अचर तार्किक isCoverSeq = coverp && coverp->isCoverSeq();
        // A sequence event control is not an assertion directive; no default
        // disable iff, no assertion control
        अचर तार्किक isSeqEvent = coverp && coverp->isSeqEvent();
        अचर तार्किक गन्ती_Matches = isCoverSeq && !isSeqEvent;
        // Inherit module defaults (IEEE 14.12, 16.15) when assertion has none.
        यदि (!propp->sensesp() && m_defaultClockingp) {
            propp->sensesp(m_defaultClockingp->sentreep()->sensesp()->clone_रूख(सत्य));
        }
        यदि (!propp->disablep() && m_defaultDisablep && !isSeqEvent) {
            propp->disablep(m_defaultDisablep->condp()->clone_रूख_Pure(सत्य));
        }
        यदि (!propp->sensesp()) फर्काऊ;
        Ast_Sen_रूख* senTreep
            = नूतन Ast_Sen_रूख{propp->fileline(), propp->sensesp()->clone_रूख(सत्य)};
        Ast_गाँठो_अभिव्यक्ति* अचर disableExprp = propp->disablep();

        // NFA lowering clones repeated operands and may hoist them into an
        // always_comb block. Resolve implicit sampled-value clocks first, while
        // the enclosing assertion clock is still available.
        {
            VL_RESTORER(m_sampled_मान_Clockp);
            m_sampled_मान_Clockp = senTreep;
            iterate(propp->propp());
        }

        File_पङ्क्ति* अचर flp = assertp->fileline();

        SvaGraph graph;
        SvaNfaBuilder builder{graph,      m_modp,     m_propTempNames,
                              isCoverSeq, isSeqEvent, coverp != nullptr};

        अचर BuildResult result = buildAssertionGraph(builder, graph, seqBodyp, parts, flp);
        यदि (result.valid()) wireMatchAndMidSources(graph, result, flp);
        यदि (!result.valid()) {
            // Fall through to V3AssertPre for unsupported constructs; only
            // replace the body on real semantic errors. Any hoisted temps
            // from this attempt become orphan MODULETEMPs; V3Dead removes
            // them along with the dead always_comb driver.
            replaceBodyOnBuildError(flp, propp, result.errorEmitted);
            VL_DO_DANGLING(थप_Deletep(senTreep), senTreep);
            फर्काऊ;
        }
        // After the build, so a construct the builder rejects reports itself.
        यदि (propertyControlp) {
            seqBodyp->v3warn(E_UNSUPPORTED,
                             "Unsupported: temporal property if/case with " << propertyControlp);
            replaceBodyOnBuildError(flp, propp, /*errorEmitted=*/सत्य);
            VL_DO_DANGLING(थप_Deletep(senTreep), senTreep);
            फर्काऊ;
        }
        यदि (गन्ती_Matches && !v3Global.opt.coverageUser()) {
            SvaNfaBuilder::freeUnlinkedCondp(result.finalCondp);
            VL_DO_DANGLING(थप_Deletep(senTreep), senTreep);
            Ast_गाँठो* अचर innerPropp = propp->propp();
            innerPropp->replaceWith(नूतन AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(थप_Deletep(innerPropp), innerPropp);
            फर्काऊ;
        }

        AstAssert* अचर assertAssertp = VN_CAST(assertp, Assert);
        अचर तार्किक splitImplicationPasssp = assertAssertp && assertAssertp->passsp()
                                            && parts.hasImplication
                                            && canSplitImplicationPassActions(parts);
        अचर तार्किक needMatch = assertAssertp && assertAssertp->passsp()
                               && (!parts.hasImplication || splitImplicationPasssp);

        अचर तार्किक needsThreadFailReplay
            = assertAssertp && assertAssertp->failsp() && !parts.hasImplication;
        अचर स्वतः signals
            = m_loweringp->lower(assertp, graph, senTreep, result.finalCondp, disableExprp,
                                 needsThreadFailReplay, needsThreadFailReplay && !negated);
        Ast_गाँठो_अभिव्यक्ति* matchExprp = nullptr;
        Ast_गाँठो_अभिव्यक्ति* outputExprp = m_loweringp->assembleResult(
            assertp, negated, result.finalCondp, signals, needMatch ? &matchExprp : nullptr);
        यदि (गन्ती_Matches) {
            UASSERT_OBJ(signals.matchCountp, coverp, "Cover sequence missing match count");
            VL_DO_DANGLING(outputExprp->delete_रूख(), outputExprp);
            propp->matchCountp(signals.matchCountp);
            outputExprp = नूतन AstNeq{flp, signals.matchCountp->clone_रूख_Pure(असत्य),
                                     newTypedConstp(flp, signals.matchCountp->dtypep(), 0)};
        }

        Ast_Sen_रूख* अचर threadFailReplaySenTreep
            = signals.threadFailCountp ? senTreep->clone_रूख(असत्य) : nullptr;

        VL_DO_DANGLING(थप_Deletep(senTreep), senTreep);
        यदि (result.finalCondp && !result.finalCondp->backp()) थप_Deletep(result.finalCondp);

        यदि (splitImplicationPasssp) {
            splitImplicationPassActions(assertAssertp, parts, matchExprp);
        } अन्यथा {
            attachActionHandlers(assertAssertp, matchExprp, threadFailReplaySenTreep,
                                 signals.threadFailCountp);
        }

        Ast_गाँठो* अचर innerPropp = propp->propp();
        innerPropp->replaceWith(outputExprp);
        VL_DO_DANGLING(थप_Deletep(innerPropp), innerPropp);

        UINFO(4, "NFA converted assertion at " << flp << endl);

        यदि (dumpGraphLevel() >= 6) graph.m_graph.dumpDotFilePrefixed("assert-nfa");
    }

    // VISITORS
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_loweringp);
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultDisablep);
        m_modp = nodep;
        // A class nested in a module is in the scope of the module's default clocking
        यदि (!VN_IS(nodep, Class)) m_defaultClockingp = nullptr;
        m_defaultDisablep = nodep->defaultDisablep();
        SvaNfaLowering lowering{nodep};
        m_loweringp = &lowering;
        iterateChildren(nodep);
    }
    शून्य visit(AstClocking* nodep) override {
        यदि (nodep->isDefault() && !m_defaultClockingp) m_defaultClockingp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    शून्य visit(AstDefaultDisable* nodep) override {}
    शून्य visit(AstFell* nodep) override { visit_Sampled_मान(nodep); }
    शून्य visit(AstPast* nodep) override { visit_Sampled_मान(nodep); }
    शून्य visit(AstRose* nodep) override { visit_Sampled_मान(nodep); }
    शून्य visit(AstStable* nodep) override { visit_Sampled_मान(nodep); }
    शून्य visit(AstAssert* nodep) override { processAssertion(nodep); }
    शून्य visit(AstCover* nodep) override { processAssertion(nodep); }
    शून्य visit(AstRestrict* nodep) override {
        // Restrict property is ignored by simulators (IEEE 1800-2023 16.12.2).
        // Remove here so temporal SExpr don't leak to V3AssertPre.
        VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstAssertIntrinsic* nodep) override {}
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    explicit AssertNfaVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Top entry point

शून्य V3AssertNfa::assertNfaAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":" << endl);
    { AssertNfaVisitor{nodep}; }
    V3Global::dump_Check_Global_रूख("assertnfa", 0, dump_रूख_Either_Level() >= 3);
}
