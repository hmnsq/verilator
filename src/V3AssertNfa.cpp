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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3AssertNfa.h"

#समावेश "V3Assert.h"
#समावेश "V3Const.h"
#समावेश "V3Graph.h"
#समावेश "V3Stats.h"
#समावेश "V3Task.h"
#समावेश "V3UniqueNames.h"

#समावेश <set>
#समावेश <unordered_set>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// NFA Graph Data Structures (V3Graph-derived per upstream convention)

namespace {

श्रेणी SvaStateVertex;

// Per-vertex algorithm data, stored via V3GraphVertex::userp() during lowering
काष्ठा Sva_Vertex_तथ्य final {
    AstVar* stateVarp = nullptr;  // NBA state register for this vertex
    AstVar* delayRingVarp = nullptr;  // Packed occupancy bits or per-slot match counts
    AstVar* delay_Ring_शीर्ष_Varp
        = nullptr;  // Cached outgoing slot; isolates readers from in-place writes
    AstVar* delayRingIdxVarp = nullptr;  // Next slot written in delayRingVarp
    AstVar* delay_Ring_Live_गिनती_Varp = nullptr;  // Number of threads in delayRingVarp
    AstVar* delayRingWrappedVarp = nullptr;  // All slots written since the last clear
    AstVar* doneLVarp = nullptr;  // SAnd LHS done-latch
    AstVar* doneRVarp = nullptr;  // SAnd RHS done-latch
    Ast_गाँठ_व्यंजक* stateSigp = nullptr;  // Combinational state signal; OWNED during lowering
    Ast_गाँठ_व्यंजक* state_गिनती_Sigp = nullptr;  // Cover-sequence match count; OWNED during lowering
    Ast_गाँठ_व्यंजक* state_गिनती_Overflow_Sigp = nullptr;  // Count overflow; OWNED during lowering
    bool delayRingCounts = false;  // Ring slots hold cover-sequence match counts
    bool needsReg = false;  // True if vertex has incoming clocked edge
};

// NFA state vertex -- one per NFA position in the sequence evaluation
श्रेणी SvaStateVertex final : खुला V3GraphVertex {
    VL_RTTI_IMPL(SvaStateVertex, V3GraphVertex)
खुला:
    // True if this is the sequence-match terminal vertex
    bool m_isMatch = false;
    // OWNED throughout-guard condition clones; IEEE 1800-2023 16.9.9
    std::vector<Ast_गाँठ_व्यंजक*> m_throughoutConds;
    // Nonzero for a ring-buffer vertex for ## delays.
    bool m_isFixedDelayRing = false;
    अचिन्हित m_delay_Ring_आकार = 0;  // Number of ring slots. Range: max-min+1.
    Ast_गाँठ_व्यंजक* m_delayRingClearCondp = nullptr;  // local RHS for pure-boolean range
    Ast_गाँठ_व्यंजक* m_delayRingAdvanceCondp = nullptr;  // Advance only when this condition holds
    bool m_replayAbortReject = false;  // Compressed repetition needs per-thread abort replay
    // OWNED; enclosing-abort fire condition clearing state or suppressing guard rejection
    Ast_गाँठ_व्यंजक* m_abortClearp = nullptr;
    // OWNED; reject-abort fire condition rejecting all represented live threads
    Ast_गाँठ_व्यंजक* m_abortRejectp = nullptr;
    // Liveness terminal (IEEE weak semantics): reject must not fire from this source
    bool m_isUnbounded = false;
    // Temporal sequence AND combiner; IEEE 1800-2023 16.9.5
    bool m_isAndCombiner = false;
    SvaStateVertex* m_andLhsTermp = nullptr;  // LHS sub-NFA terminal vertex
    SvaStateVertex* m_andRhsTermp = nullptr;  // RHS sub-NFA terminal vertex
    Ast_गाँठ_व्यंजक* m_andLhsCondp = nullptr;  // OWNED; LHS final condition (may be nullptr)
    Ast_गाँठ_व्यंजक* m_andRhsCondp = nullptr;  // OWNED; RHS final condition (may be nullptr)
    // Reject sink for SAnd rejectOnFail wiring; not a state-signal source
    bool m_isRejectSink = false;
    // In-window vertex of a strong s_always[m:n]: if its state is still set at
    // end-of-simulation the universal-quantifier window never completed, which is
    // a liveness failure (IEEE 1800-2023 16.12.11 strong semantics).
    bool m_strongPending = false;
    // Ring introduced for the checked portion of a bounded strong s_always window.
    bool m_strongAlwaysRing = false;

    // CONSTRUCTORS
    explicit SvaStateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~SvaStateVertex() override {
        क्रम (Ast_गाँठ_व्यंजक* cp : m_throughoutConds) VL_DO_DANGLING(cp->delete_वृक्ष(), cp);
        अगर (m_delayRingClearCondp)
            VL_DO_DANGLING(m_delayRingClearCondp->delete_वृक्ष(), m_delayRingClearCondp);
        अगर (m_delayRingAdvanceCondp)
            VL_DO_DANGLING(m_delayRingAdvanceCondp->delete_वृक्ष(), m_delayRingAdvanceCondp);
        अगर (m_abortClearp) VL_DO_DANGLING(m_abortClearp->delete_वृक्ष(), m_abortClearp);
        अगर (m_abortRejectp) VL_DO_DANGLING(m_abortRejectp->delete_वृक्ष(), m_abortRejectp);
        अगर (m_andLhsCondp) VL_DO_DANGLING(m_andLhsCondp->delete_वृक्ष(), m_andLhsCondp);
        अगर (m_andRhsCondp) VL_DO_DANGLING(m_andRhsCondp->delete_वृक्ष(), m_andRhsCondp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string name() स्थिर override {
        string name = "s" + cvtToStr(color());
        अगर (m_delay_Ring_आकार) {
            name += "\\n";
            name += m_isFixedDelayRing ? "fixed chain " : "range chain ";
            name += cvtToStr(m_delay_Ring_आकार) + " slots";
        }
        वापस name;
    }
    string dotColor() स्थिर override {
        अगर (m_isMatch) वापस "red";
        अगर (m_delay_Ring_आकार) वापस "blue";
        अगर (m_isAndCombiner) वापस "purple";
        वापस "black";
    }
    // LCOV_EXCL_STOP
    // Access per-vertex algorithm data (valid only during lowering phase)
    Sva_Vertex_तथ्य* datap() स्थिर { वापस static_cast<Sva_Vertex_तथ्य*>(userp()); }
};

// NFA transition edge -- clocked (##1) or combinational link (##0)
श्रेणी SvaTransEdge final : खुला V3GraphEdge {
    VL_RTTI_IMPL(SvaTransEdge, V3GraphEdge)
खुला:
    Ast_गाँठ_व्यंजक* m_condp;  // Transition condition; nullptr = unconditional; OWNED
    bool m_consumesCycle;  // true = clocked edge (##1), false = link (##0/boolean)
    // Reject when source is active and condp is false; set only on
    // outermost required-step Link
    bool m_rejectOnFail = false;
    // Optional dynamic condition vertex for m_rejectOnFail. Used when the
    // success condition is another NFA state rather than a static expression.
    SvaStateVertex* m_condVtxp = nullptr;

    // CONSTRUCTORS
    SvaTransEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष, Ast_गाँठ_व्यंजक* condp,
                 bool consumesCycle)
        : V3GraphEdge{graphp, fromp, शीर्ष, /*weight=*/1}
        , m_condp{condp}
        , m_consumesCycle{consumesCycle} {}
    ~SvaTransEdge() override {
        अगर (m_condp) VL_DO_DANGLING(m_condp->delete_वृक्ष(), m_condp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string dotLabel() स्थिर override { वापस m_consumesCycle ? "##1" : "link"; }
    string dotStyle() स्थिर override { वापस m_consumesCycle ? "" : "dashed"; }
    // LCOV_EXCL_STOP
    // Typed accessors for NFA vertices
    SvaStateVertex* fromVtxp() स्थिर { वापस static_cast<SvaStateVertex*>(fromp()); }
    SvaStateVertex* toVtxp() स्थिर { वापस static_cast<SvaStateVertex*>(शीर्ष()); }
};

// NFA graph container
श्रेणी SvaGraph final {
खुला:
    V3Graph m_graph;  // Owns all vertices and edges
    SvaStateVertex* m_startVertexp = nullptr;  // Trigger/start vertex
    SvaStateVertex* m_matchVertexp = nullptr;  // Sequence-match terminal vertex

    // Create a new state vertex
    SvaStateVertex* createStateVertex() { वापस नया SvaStateVertex{&m_graph}; }
    // Create the match terminal vertex
    SvaStateVertex* createMatchVertex() {
        SvaStateVertex* स्थिर vtxp = createStateVertex();
        vtxp->m_isMatch = true;
        m_matchVertexp = vtxp;
        वापस vtxp;
    }
    // Add a clocked transition edge (##1)
    SvaTransEdge* addClockedEdge(SvaStateVertex* fromp, SvaStateVertex* शीर्ष,
                                 Ast_गाँठ_व्यंजक* condp = nullptr) {
        वापस नया SvaTransEdge{&m_graph, fromp, शीर्ष, condp, /*consumesCycle=*/true};
    }
    // Add a combinational link (##0 / boolean condition)
    SvaTransEdge* addLink(SvaStateVertex* fromp, SvaStateVertex* शीर्ष,
                          Ast_गाँठ_व्यंजक* condp = nullptr) {
        वापस नया SvaTransEdge{&m_graph, fromp, शीर्ष, condp, /*consumesCycle=*/false};
    }
    // Collect all edges into a flat vector for iteration.
    // Used by the lowering phase which needs global edge scans.
    std::vector<स्थिर SvaTransEdge*> allEdges() स्थिर {
        std::vector<स्थिर SvaTransEdge*> परिणाम;
        क्रम (स्थिर V3GraphVertex& vtxr : m_graph.vertices()) {
            क्रम (स्थिर V3GraphEdge& edger : vtxr.outEdges()) {
                परिणाम.धकेलो_back(static_cast<स्थिर SvaTransEdge*>(&edger));
            }
        }
        वापस परिणाम;
    }
};

//######################################################################
// Builder result: terminal vertex + optional final condition (match Link condition).
काष्ठा Build_परिणाम final {
    SvaStateVertex* termVertexp;  // Primary terminal; contributes to both match and reject
    Ast_गाँठ_व्यंजक* finalCondp;  // nullptr = unconditional
    // Mid-window sources for range delays (pure boolean RHS): match-only (isUnbounded)
    std::vector<SvaStateVertex*> midSources;
    bool errorEmitted = false;  // Builder already emitted specific error; skip generic
    // For cover_sequence: when true, midSources already enumerate every
    // end-of-match, so wireMatchAndMidSources must NOT add the main
    // termVtxp -> matchVertex Link (would double-count via the merge vertex).
    bool termIsMidMerge = false;
    bool valid() स्थिर { वापस termVertexp != nullptr; }
    अटल Build_परिणाम हार(bool errored = false) { वापस {nullptr, nullptr, {}, errored}; }
    अटल Build_परिणाम failWithError() { वापस {nullptr, nullptr, {}, true}; }
};

// Parser-marked SAnd of overlapped implications: a property if/else/case.
अटल bool hasPropertyControlConjunction(स्थिर Ast_गाँठ_व्यंजक* nodep) {
    वापस nodep->exists([](स्थिर AstSAnd* andp) { वापस andp->propertyControl(); });
}

अटल AstConst* newTypedConstp(File_पंक्ति* स्थिर flp, स्थिर Ast_गाँठ_DType* स्थिर dtypep,
                                स्थिर uint32_t मूल्य) {
    AstConst* स्थिर constp = नया AstConst{flp, AstConst::DTyped{}, dtypep};
    constp->अंक().setLong(मूल्य);
    वापस constp;
}

अटल Ast_गाँठ_व्यंजक* sampled(Ast_गाँठ_व्यंजक* exprp) {
    वापस नया AstSampled{exprp->fileline(), exprp, exprp->dtypep(), true};
}

अटल स्थिर अक्षर* assertPassOnQuery(bool vacuous) {
    अटल constexpr स्थिर अक्षर* queries[2]
        = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
    वापस queries[vacuous];
}

अटल Ast_गाँठ_व्यंजक* assertOnCond(File_पंक्ति* flp, VAssertType type,
                                 VAssertDirectiveType directiveType) {
    अगर (!v3Global.opt.assertOn()) { वापस नया AstConst{flp, AstConst::BitFalse{}}; }
    वापस नया AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
}

अटल Ast_गाँठ_व्यंजक* assertKillGet(File_पंक्ति* flp, VAssertType type,
                                  VAssertDirectiveType directiveType) {
    वापस नया AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_KILL", type, directiveType),
                        32};
}

अटल string assertActionControlPrefix(VAssertDirectiveType directiveType) {
    स्थिर पूर्णांक controlled = !!(static_cast<पूर्णांक>(directiveType)
                              & (static_cast<पूर्णांक>(VAssertDirectiveType::ASSERT)
                                 | static_cast<पूर्णांक>(VAssertDirectiveType::COVER)
                                 | static_cast<पूर्णांक>(VAssertDirectiveType::ASSUME)));
    स्थिर पूर्णांक checkRuntime = controlled & static_cast<पूर्णांक>(v3Global.opt.assertOn());
    वापस "("s + std::to_string(controlled ^ 1) + " || ("s + std::to_string(checkRuntime)
           + " && "s;
}

अटल Ast_गाँठ_व्यंजक* assertPassOnCond(File_पंक्ति* flp, VAssertType type,
                                     VAssertDirectiveType directiveType, bool vacuous) {
    वापस नया AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType)
            + "))"s,
        1};
}

अटल Ast_गाँठ_व्यंजक* assertFailOnCond(File_पंक्ति* flp, VAssertType type,
                                     VAssertDirectiveType directiveType) {
    वापस नया AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType) + "))"s,
        1};
}

अटल AstIf* newPassOnIf(File_पंक्ति* flp, Ast_गाँठ_व्यंजक* firep, Ast_गाँठ* bodyp, VAssertType type,
                          VAssertDirectiveType directiveType, bool vacuous) {
    Ast_गाँठ_व्यंजक* स्थिर condp
        = नया AstLogAnd{flp, firep, assertPassOnCond(flp, type, directiveType, vacuous)};
    AstIf* स्थिर ifp = नया AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(true);
    ifp->user1(true);
    वापस ifp;
}

अटल Ast_गाँठ_Stmt* newIfAssertFailOn(Ast_गाँठ* bodyp, VAssertDirectiveType directiveType,
                                      VAssertType type) {
    File_पंक्ति* स्थिर flp = bodyp->fileline();
    Ast_गाँठ_व्यंजक* स्थिर condp = assertFailOnCond(flp, type, directiveType);
    AstIf* स्थिर ifp = नया AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(true);
    ifp->user1(true);
    वापस ifp;
}

//######################################################################
// NFA Builder

श्रेणी SvaNfaBuilder final {
    SvaGraph& m_graph;  // NFA graph being built
    Ast_गाँठ_Module* स्थिर m_modp;  // Module to receive hoisted sampled-prop temps
    V3UniqueNames& m_prop_अस्थायी_Names;  // Module-shared temp-var name source
    std::vector<Ast_गाँठ_व्यंजक*> m_temporal_Guard_ढेर;  // Guards active across nested temporal states
    // Outer abort conditions, AND-ed as !cond into inner abort edges
    // (IEEE 1800-2023 16.12.14 outer-wraps-inner).
    std::vector<Ast_गाँठ_व्यंजक*> m_outer_Abort_ढेर;
    bool m_inUnboundedScope = false;  // Sticky: nodes created after inherit liveness
    bool m_markStrongPending = false;  // Mark new vertices as strong s_always in-window
    bool m_isCover = false;  // Cover directives do not fail at end-of-simulation
    // IEEE 1800-2023 16.14.3 cover sequence: each end-of-match fires the action,
    // not just the first. Builder builds parallel-branch (no first-match-wins)
    // topology when true. Default false preserves cover_property semantics.
    bool m_isCoverSeq = false;
    // Unsupported endpoint topology must reject, not ignore, or the wait hangs
    bool m_isSeqEvent = false;
    bool m_inSequencePrefix = false;  // Prefix composition does not preserve every endpoint
    bool m_wantBoolLeaf = false;  // Building a |-> / |=> or top-level body (see buildOrMerge)

    काष्ठा RangeDelayRejectInfo final {
        SvaStateVertex* startp = nullptr;
        अचिन्हित range = 0;
        पूर्णांक rhsLen = 0;
    };

    व्योम warnEndpointUnsupported(File_पंक्ति* flp, स्थिर string& what) स्थिर {
        अगर (m_isSeqEvent) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: sequence used as an event control with " << what);
        } अन्यथा {
            flp->v3warn(COVERIGN, "Ignoring unsupported: cover sequence with " << what);
        }
    }

    Ast_गाँठ_व्यंजक* throughoutCond(Ast_गाँठ_व्यंजक* baseCondp, File_पंक्ति* flp) {
        अगर (m_temporal_Guard_ढेर.empty()) वापस baseCondp;
        // AND all active temporal guards (supports nesting)
        // Each must use $sampled values.
        Ast_गाँठ_व्यंजक* guardp = nullptr;
        क्रम (Ast_गाँठ_व्यंजक* स्थिर condp : m_temporal_Guard_ढेर) {
            Ast_गाँठ_व्यंजक* स्थिर clonep = sampled(condp->clone_वृक्ष_Pure(false));
            अगर (!guardp) {
                guardp = clonep;
            } अन्यथा {
                guardp = नया AstLogAnd{flp, guardp, clonep};
            }
        }
        अगर (baseCondp) { guardp = नया AstLogAnd{flp, baseCondp, guardp}; }
        वापस guardp;
    }

    अटल अचिन्हित getConstUInt(Ast_गाँठ_व्यंजक* exprp) {
        Ast_गाँठ_व्यंजक* स्थिर constp = V3Const::constifyEdit(exprp->clone_वृक्ष_Pure(false));
        स्थिर AstConst* स्थिर cp = VN_CAST(constp, Const);
        स्थिर अचिन्हित val = cp ? cp->toUInt() : 0;
        VL_DO_DANGLING(constp->delete_वृक्ष(), constp);
        वापस val;
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
    अटल पूर्णांक fixed_लंबाई(Ast_गाँठ_व्यंजक* nodep) {
        अगर (AstSExpr* स्थिर sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* स्थिर delayp = VN_CAST(sexprp->delayp(), Delay);
            अगर (!delayp || !delayp->isCycleDelay()) वापस -1;
            अचिन्हित delayCycles;
            अगर (delayp->isRangeDelay()) {
                अगर (delayp->isUnbounded()) वापस -1;  // LCOV_EXCL_LINE
                स्थिर अचिन्हित minD = getConstUInt(delayp->lhsp());
                स्थिर अचिन्हित maxD = getConstUInt(delayp->rhsp());
                अगर (minD != maxD) वापस -1;
                delayCycles = minD;
            } अन्यथा {
                delayCycles = getConstUInt(delayp->lhsp());
            }
            पूर्णांक preLen = 0;
            अगर (Ast_गाँठ_व्यंजक* स्थिर prep = sexprp->preExprp()) {
                preLen = fixed_लंबाई(prep);
                अगर (preLen < 0) वापस -1;  // LCOV_EXCL_LINE
            }
            स्थिर पूर्णांक bodyLen = fixed_लंबाई(sexprp->exprp());
            अगर (bodyLen < 0) वापस -1;  // LCOV_EXCL_LINE
            वापस preLen + delayCycles + bodyLen;
        }
        अगर (AstSThroughout* स्थिर throughp = VN_CAST(nodep, SThroughout)) {
            वापस fixed_लंबाई(throughp->rhsp());
        }
        अगर (AstSOr* स्थिर orp = VN_CAST(nodep, SOr)) {
            // Alternatives must share one end cycle; buildSWithin relies on
            // this to pair the OR with an SIntersect.
            स्थिर पूर्णांक lhsLen = fixed_लंबाई(orp->lhsp());
            स्थिर पूर्णांक rhsLen = fixed_लंबाई(orp->rhsp());
            अगर (lhsLen < 0 || rhsLen < 0 || lhsLen != rhsLen) वापस -1;
            वापस lhsLen;
        }
        अगर (AstSWithin* स्थिर withinp = VN_CAST(nodep, SWithin)) {
            // `seq1 within seq2` ends at seq2's end cycle (IEEE 16.9.10).
            स्थिर पूर्णांक lhsLen = fixed_लंबाई(withinp->lhsp());
            स्थिर पूर्णांक rhsLen = fixed_लंबाई(withinp->rhsp());
            अगर (lhsLen < 0 || rhsLen < 0 || lhsLen > rhsLen) वापस -1;
            वापस rhsLen;
        }
        // LCOV_EXCL_START -- defensive: V3AssertPre rejects composite SVA ops
        // nested in an intersect arm before fixedLength runs (clock-context
        // resolution fails). Kept as a guard in case future parser relaxations
        // permit it.
        अगर (nodep->isMultiCycleSva()) वापस -1;
        // LCOV_EXCL_STOP
        // Plain boolean expression (no SVA constructs) -- 0 cycles.
        वापस 0;
    }

    // Contiguous match-length range [lo,hi] for an operand whose length varies
    // from AT MOST ONE ranged cycle delay; {-1,-1} otherwise. Drives the
    // variable-length `intersect` lowering (IEEE 1800-2023 16.9.6): more than
    // one ranged delay would make the per-length realization ambiguous, so it
    // is reported unsupported rather than mis-paired.
    अटल std::pair<पूर्णांक, पूर्णांक> लंबाई_Range(Ast_गाँठ_व्यंजक* nodep) {
        अगर (AstSExpr* स्थिर sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* स्थिर delayp = VN_CAST(sexprp->delayp(), Delay);
            अगर (!delayp || !delayp->isCycleDelay()) वापस {-1, -1};
            std::pair<पूर्णांक, पूर्णांक> delayRange;
            अगर (delayp->isRangeDelay()) {
                अगर (delayp->isUnbounded()) वापस {-1, -1};
                स्थिर अचिन्हित minD = getConstUInt(delayp->lhsp());
                स्थिर अचिन्हित maxD = getConstUInt(delayp->rhsp());
                delayRange = {minD, maxD};
            } अन्यथा {
                स्थिर अचिन्हित d = getConstUInt(delayp->lhsp());
                delayRange = {d, d};
            }
            std::pair<पूर्णांक, पूर्णांक> preRange{0, 0};
            अगर (Ast_गाँठ_व्यंजक* स्थिर prep = sexprp->preExprp()) {
                preRange = लंबाई_Range(prep);
                अगर (preRange.first < 0) वापस {-1, -1};
            }
            स्थिर std::pair<पूर्णांक, पूर्णांक> bodyRange = लंबाई_Range(sexprp->exprp());
            अगर (bodyRange.first < 0) वापस {-1, -1};
            स्थिर पूर्णांक variableParts = (preRange.first != preRange.second)
                                      + (delayRange.first != delayRange.second)
                                      + (bodyRange.first != bodyRange.second);
            अगर (variableParts > 1) वापस {-1, -1};
            वापस {preRange.first + delayRange.first + bodyRange.first,
                    preRange.second + delayRange.second + bodyRange.second};
        }
        अगर (AstSThroughout* स्थिर throughp = VN_CAST(nodep, SThroughout)) {
            वापस लंबाई_Range(throughp->rhsp());
        }
        अगर (nodep->isMultiCycleSva()) वापस {-1, -1};
        वापस {0, 0};  // plain boolean -- 0 cycles
    }

    // Clone `operand` with its sole variable ranged cycle delay pinned so the
    // total match length is exactly `len`. `lo` is the operand's minimum length
    // (lengthRange().first). A fixed operand has no such delay and is returned
    // as a plain clone (callers only request its single achievable length).
    अटल Ast_गाँठ_व्यंजक* realize_At_लंबाई(Ast_गाँठ_व्यंजक* operand, पूर्णांक len, पूर्णांक lo) {
        Ast_गाँठ_व्यंजक* स्थिर clonep = operand->clone_वृक्ष_Pure(false);
        AstDelay* rangeDelayp = nullptr;
        clonep->foreach([&](AstDelay* dp) {
            अगर (!rangeDelayp && dp->isRangeDelay() && !dp->isUnbounded()
                && getConstUInt(dp->lhsp()) != getConstUInt(dp->rhsp())) {
                rangeDelayp = dp;
            }
        });
        अगर (rangeDelayp) {
            File_पंक्ति* स्थिर flp = rangeDelayp->fileline();
            स्थिर अचिन्हित pinned = getConstUInt(rangeDelayp->lhsp()) + (len - lo);
            Ast_गाँठ_व्यंजक* स्थिर oldMinp = rangeDelayp->lhsp();
            oldMinp->replaceWith(नया AstConst{flp, pinned});
            VL_DO_DANGLING(oldMinp->delete_वृक्ष(), oldMinp);
            // Drop the max bound so it lowers as a fixed `##d`, not `##[d:d]`.
            Ast_गाँठ* स्थिर oldMaxp = rangeDelayp->rhsp()->unlinkFrBack();
            VL_DO_DANGLING(oldMaxp->delete_वृक्ष(), oldMaxp);
        }
        वापस clonep;
    }

    // Cuts AST size from O(N * sizeof(exprp)) to O(N) + O(sizeof(exprp)) by
    // sharing a single `VarRef` across N check edges. Hoist also matches the
    // IEEE 1800-2023 16.9.9 "single preponed-region snapshot" semantic for
    // any exprp -- even an impure one would now evaluate exactly once per
    // clock instead of N times. Orphan temps from failed builds are unused
    // MODULETEMPs and are removed by V3Dead.
    AstVar* tryHoistSampled(Ast_गाँठ_व्यंजक* exprp, File_पंक्ति* flp, अचिन्हित clone_गिनती) {
        constexpr अचिन्हित kHoistThreshold = 2;
        अगर (clone_गिनती < kHoistThreshold) वापस nullptr;
        AstVar* स्थिर अस्थायी_Varp
            = नया AstVar{flp, VVarType::MODULETEMP, m_prop_अस्थायी_Names.लो(exprp), exprp->dtypep()};
        m_modp->addStmtsp(अस्थायी_Varp);
        AstAssign* स्थिर assignp = नया AstAssign{flp, नया AstVarRef{flp, अस्थायी_Varp, VAccess::WRITE},
                                                 sampled(exprp->clone_वृक्ष_Pure(false))};
        m_modp->addStmtsp(नया AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, assignp});
        वापस अस्थायी_Varp;
    }

    अटल Ast_गाँठ_व्यंजक* sampledRefOrClone(AstVar* hoistVarp, Ast_गाँठ_व्यंजक* exprp, File_पंक्ति* flp) {
        अगर (hoistVarp) वापस नया AstVarRef{flp, hoistVarp, VAccess::READ};
        वापस sampled(exprp->clone_वृक्ष_Pure(false));
    }

    // Create vertex and inherit temporal guards from the current scope.
    SvaStateVertex* scopedCreateVertex() {
        SvaStateVertex* स्थिर vtxp = m_graph.createStateVertex();
        क्रम (Ast_गाँठ_व्यंजक* स्थिर cp : m_temporal_Guard_ढेर) {
            vtxp->m_throughoutConds.धकेलो_back(cp->clone_वृक्ष_Pure(false));
        }
        अगर (m_inUnboundedScope) vtxp->m_isUnbounded = true;
        अगर (m_markStrongPending) vtxp->m_strongPending = true;
        वापस vtxp;
    }

    // AND current temporal guards into every edge/link.
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* शीर्ष, Ast_गाँठ_व्यंजक* condp,
                              File_पंक्ति* flp) {
        वापस m_graph.addLink(fromp, शीर्ष, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* शीर्ष, File_पंक्ति* flp) {
        वापस m_graph.addLink(fromp, शीर्ष, throughoutCond(nullptr, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* शीर्ष, Ast_गाँठ_व्यंजक* condp,
                              File_पंक्ति* flp) {
        वापस m_graph.addClockedEdge(fromp, शीर्ष, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* शीर्ष, File_पंक्ति* flp) {
        वापस m_graph.addClockedEdge(fromp, शीर्ष, throughoutCond(nullptr, flp));
    }

    SvaStateVertex* addDelayChain(SvaStateVertex* startp, अचिन्हित आकार, File_पंक्ति* flp,
                                  bool isFixed = true, Ast_गाँठ_व्यंजक* clearCondp = nullptr,
                                  Ast_गाँठ_व्यंजक* advanceCondp = nullptr) {
        अगर (isFixed && आकार == 0) वापस startp;
        UASSERT_OBJ(आकार > 0, startp, "Delay chain needs at least one slot");
        अगर (isFixed && आकार == 1 && !advanceCondp) {
            SvaStateVertex* स्थिर nextp = scopedCreateVertex();
            guardedEdge(startp, nextp, flp);
            वापस nextp;
        }
        SvaStateVertex* स्थिर ringVtxp = scopedCreateVertex();
        ringVtxp->m_isFixedDelayRing = isFixed;
        ringVtxp->m_delay_Ring_आकार = आकार;
        अगर (clearCondp) {
            UASSERT_OBJ(!isFixed, startp, "Fixed delay cannot have a clear condition");
            ringVtxp->m_delayRingClearCondp = clearCondp->clone_वृक्ष_Pure(false);
        }
        ringVtxp->m_delayRingAdvanceCondp = advanceCondp;
        अगर (isFixed) {
            guardedEdge(startp, ringVtxp, flp);
        } अन्यथा {
            guardedLink(startp, ringVtxp, flp);
        }
        वापस ringVtxp;
    }

    // Build NFA for an SExpr. finalCond = RHS (not yet added as a vertex).
    // isTopLevelStep: marks outermost required boolean check as rejectOnFail.
    // Apply a range delay `##[M:N]` to currentp. Returns true on success. On
    // failure, emits a diagnostic and returns false.
    bool applyRangeDelay(AstSExpr* sexprp, SvaStateVertex*& currentp,
                         std::vector<SvaStateVertex*>& midSources,
                         RangeDelayRejectInfo* rangeRejectInfop = nullptr) {
        File_पंक्ति* स्थिर flp = sexprp->fileline();
        AstDelay* स्थिर delayp = VN_AS(sexprp->delayp(), Delay);
        Ast_गाँठ_व्यंजक* स्थिर rhsExprp = sexprp->exprp();
        स्थिर अचिन्हित minDelay = getConstUInt(delayp->lhsp());
        अगर (delayp->isUnbounded()) {
            // `##[M:$]`: wait M cycles, then retain every matured attempt in a
            // one-slot ring while also exposing the first eligible tick.
            currentp = addDelayChain(currentp, minDelay, flp);
            SvaStateVertex* स्थिर waitRingp = addDelayChain(currentp, 1, flp, false);
            // The ring's live count is updated in NBA, so this link retains the previous
            // tick's live attempts alongside new arrivals for the next tick.
            guardedLink(waitRingp, waitRingp, flp);
            SvaStateVertex* स्थिर mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(waitRingp, mergeVtxp, flp);
            currentp = mergeVtxp;
            currentp->m_isUnbounded = true;  // Liveness, so no reject.
            m_inUnboundedScope = true;
            वापस true;
        }
        स्थिर अचिन्हित maxDelay = getConstUInt(delayp->rhsp());
        अगर (minDelay == maxDelay) {
            currentp = addDelayChain(currentp, minDelay, flp);
            वापस true;
        }
        स्थिर अचिन्हित range = maxDelay - minDelay;
        currentp = addDelayChain(currentp, minDelay, flp);
        स्थिर bool multiCycleRhs = rhsExprp->isMultiCycleSva();
        स्थिर Ast_गाँठ_व्यंजक* स्थिर preExprp = sexprp->preExprp();
        // Prefix and nested-sequence composition can lose endpoints or their multiplicity
        // required by cover sequence (IEEE 1800-2023 16.14.3), regardless of the range width.
        अगर (m_isCoverSeq
            && (multiCycleRhs || m_inSequencePrefix
                || (preExprp && preExprp->isMultiCycleSva()))) {
            warnEndpointUnsupported(flp, "this ranged cycle delay");
            वापस false;
        }
        अगर (m_inSequencePrefix) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: Bounded ranged cycle delay in a sequence prefix");
            वापस false;
        }
        अगर (multiCycleRhs) {
            // Launch a candidate at every eligible tick. Candidate-local misses are not
            // assertion rejects while a later position can still match.
            अगर (rangeRejectInfop) {
                स्थिर पूर्णांक rhsLen = fixed_लंबाई(rhsExprp);
                अगर (rhsLen >= 0) *rangeRejectInfop = {currentp, range, rhsLen};
            }
            SvaStateVertex* स्थिर tailp = addDelayChain(currentp, range + 1U, flp, false);
            SvaStateVertex* स्थिर mergeVtxp = scopedCreateVertex();
            mergeVtxp->m_isUnbounded = true;
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(tailp, mergeVtxp, flp);
            currentp = mergeVtxp;
            m_inUnboundedScope = true;
        } अन्यथा {
            // The first eligible tick is explicit. The ring retains attempts for the
            // remaining ticks and exposes the final tick for rejection. Only properties
            // clear on success, since cover sequence counts every endpoint.
            midSources.धकेलो_back(currentp);
            currentp = addDelayChain(currentp, range + 1U, flp, false,
                                     m_isCoverSeq ? nullptr : rhsExprp);
        }
        वापस true;
    }

    व्योम addFiniteRangeReject(स्थिर RangeDelayRejectInfo& info, स्थिर Build_परिणाम& परिणाम,
                              File_पंक्ति* flp) {
        अगर (!info.startp) वापस;

        SvaStateVertex* स्थिर expiryVtxp
            = addDelayChain(info.startp, info.range + info.rhsLen, flp);
        SvaStateVertex* स्थिर expiryMatchp = scopedCreateVertex();
        std::vector<SvaStateVertex*> sources = परिणाम.midSources;
        sources.धकेलो_back(परिणाम.termVertexp);
        क्रम (SvaStateVertex* स्थिर srcp : sources) {
            Ast_गाँठ_व्यंजक* स्थिर condp
                = परिणाम.finalCondp ? sampled(परिणाम.finalCondp->clone_वृक्ष_Pure(false)) : nullptr;
            SvaStateVertex* स्थिर successNowp = scopedCreateVertex();
            guardedLink(srcp, successNowp, condp, flp);
            // Retain a success through the latest deadline it can satisfy.
            SvaStateVertex* स्थिर historyp
                = addDelayChain(successNowp, info.range + 1U, flp, false);
            guardedLink(successNowp, expiryMatchp, flp);
            guardedLink(historyp, expiryMatchp, flp);
        }

        SvaStateVertex* स्थिर sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = true;
        SvaTransEdge* स्थिर rejectp = m_graph.addLink(expiryVtxp, sinkVtxp);
        rejectp->m_rejectOnFail = true;
        rejectp->m_condVtxp = expiryMatchp;
    }

    Build_परिणाम buildSExpr(AstSExpr* sexprp, SvaStateVertex* entryVtxp,
                           bool is_शीर्ष_Level_Step = false) {
        AstDelay* स्थिर delayp = VN_CAST(sexprp->delayp(), Delay);
        अगर (!delayp || !delayp->isCycleDelay()) वापस Build_परिणाम::हार();

        File_पंक्ति* स्थिर flp = sexprp->fileline();
        Ast_गाँठ_व्यंजक* स्थिर exprp = sexprp->exprp();

        // Handle LHS (preExpr)
        SvaStateVertex* currentp = entryVtxp;
        अगर (Ast_गाँठ_व्यंजक* स्थिर preExprp = sexprp->preExprp()) {
            VL_RESTORER(m_inSequencePrefix);
            m_inSequencePrefix = true;
            स्थिर Build_परिणाम pre = build_व्यंजक(preExprp, currentp, is_शीर्ष_Level_Step);
            अगर (!pre.valid()) वापस Build_परिणाम::हार(pre.errorEmitted);  // LCOV_EXCL_LINE
            अगर (pre.finalCondp) {
                SvaStateVertex* स्थिर condVtxp = scopedCreateVertex();
                SvaTransEdge* स्थिर edgep = guardedLink(
                    pre.termVertexp, condVtxp, sampled(pre.finalCondp->clone_वृक्ष_Pure(false)), flp);
                अगर (is_शीर्ष_Level_Step && !pre.termVertexp->m_isUnbounded && !m_inUnboundedScope) {
                    // Do not mark liveness sources: first boolean check is deferred.
                    edgep->m_rejectOnFail = true;
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
        स्थिर bool addRangeReject = is_शीर्ष_Level_Step && !m_inUnboundedScope;
        अगर (delayp->isRangeDelay()) {
            अगर (!applyRangeDelay(sexprp, currentp, rangeMidSources,
                                 addRangeReject ? &rangeRejectInfo : nullptr)) {
                वापस Build_परिणाम::failWithError();
            }
        } अन्यथा {
            स्थिर अचिन्हित delayCycles = getConstUInt(delayp->lhsp());
            currentp = addDelayChain(currentp, delayCycles, flp);
        }

        // Multi-cycle RHS: recurse (only plain boolean is returned as finalCondp).
        अगर (exprp->isMultiCycleSva()) {
            स्थिर Build_परिणाम परिणाम = build_व्यंजक(exprp, currentp, is_शीर्ष_Level_Step);
            अगर (परिणाम.valid()) addFiniteRangeReject(rangeRejectInfo, परिणाम, flp);
            वापस परिणाम;
        }
        वापस {currentp, exprp, std::move(rangeMidSources)};
    }

    Build_परिणाम buildConsRep(AstSConsRep* repp, SvaStateVertex* entryVtxp,
                             bool is_शीर्ष_Level_Step = false) {
        File_पंक्ति* स्थिर flp = repp->fileline();
        Ast_गाँठ_व्यंजक* स्थिर exprp = repp->exprp();
        // Multi-cycle expr in ConsRep not yet supported; bail to avoid invalid AST.
        अगर (exprp->isMultiCycleSva()) {
            repp->v3warn(E_UNSUPPORTED, "Unsupported: multi-cycle sequence expression inside"
                                        " consecutive repetition (IEEE 1800-2023 16.9.2)");
            वापस Build_परिणाम::failWithError();
        }
        स्थिर अचिन्हित minN = getConstUInt(repp->countp());

        // Sum sites across prefix + unbounded/range tail so one hoist covers
        // every check edge of this repetition.
        अचिन्हित योग_Sites = minN;
        अगर (repp->unbounded()) {
            योग_Sites += 1;
        } अन्यथा अगर (repp->maxCountp()) {
            योग_Sites += getConstUInt(repp->maxCountp()) - minN;
        }
        AstVar* स्थिर hoistVarp = tryHoistSampled(exprp, flp, योग_Sites);

        // Cover-sequence (IEEE 1800-2023 16.14.3): collect each end-of-match
        // position so they all fire the action, not just the merged terminal.
        std::vector<SvaStateVertex*> consMidSources;

        SvaStateVertex* currentp = entryVtxp;
        क्रम (अचिन्हित i = 0; i < minN; ++i) {
            // Keep the first repetition explicit, collapse all remaining checks into the ring.
            अगर (i == 1) {
                currentp = addDelayChain(currentp, minN - 1, flp);
                currentp->m_delayRingClearCondp
                    = नया AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                currentp->m_replayAbortReject = true;
                अगर (is_शीर्ष_Level_Step) {
                    currentp->m_throughoutConds.धकेलो_back(
                        sampledRefOrClone(hoistVarp, exprp, flp));
                }
                i = minN - 1;
            }
            // Every repetition in the minimum prefix is required.
            SvaStateVertex* स्थिर condVtxp = scopedCreateVertex();
            SvaTransEdge* स्थिर linkp
                = guardedLink(currentp, condVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            // Only an outermost required step rejects the property. Its first check is explicit,
            // later required checks reject through the ring's throughout guard.
            अगर (is_शीर्ष_Level_Step && i == 0) linkp->m_rejectOnFail = true;
            currentp = condVtxp;
        }
        // After minN: currentp is the first valid end-of-match position for [*m:n].
        अगर (m_isCoverSeq && (repp->unbounded() || repp->maxCountp())) {
            consMidSources.धकेलो_back(currentp);
        }

        अगर (repp->unbounded()) {
            अगर (minN == 0) {
                SvaStateVertex* स्थिर waitVtxp = scopedCreateVertex();
                guardedEdge(currentp, waitVtxp, flp);
                SvaStateVertex* स्थिर checkVtxp = scopedCreateVertex();
                guardedLink(waitVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedEdge(checkVtxp, waitVtxp, flp);
                guardedLink(currentp, checkVtxp, flp);
                currentp = checkVtxp;
            } अन्यथा {
                SvaStateVertex* स्थिर loopBackVtxp = scopedCreateVertex();
                guardedEdge(currentp, loopBackVtxp, flp);
                SvaStateVertex* स्थिर reCheckVtxp = scopedCreateVertex();
                guardedLink(loopBackVtxp, reCheckVtxp, sampledRefOrClone(hoistVarp, exprp, flp),
                            flp);
                guardedEdge(reCheckVtxp, loopBackVtxp, flp);
                guardedLink(reCheckVtxp, currentp, flp);
            }
            currentp->m_isUnbounded = true;
            m_inUnboundedScope = true;
        } अन्यथा अगर (repp->maxCountp()) {
            स्थिर अचिन्हित maxN = getConstUInt(repp->maxCountp());
            SvaStateVertex* स्थिर mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            अचिन्हित पुच्छ_Min_N = minN;
            SvaStateVertex* पुच्छ_Startp = currentp;
            अगर (minN == 0) {
                // Build the first optional iteration explicitly. Feeding the empty endpoint
                // directly into a range ring would incorrectly keep that match alive.
                SvaStateVertex* स्थिर अग्र_Vtxp = scopedCreateVertex();
                guardedEdge(currentp, अग्र_Vtxp, flp);
                SvaStateVertex* स्थिर checkVtxp = scopedCreateVertex();
                guardedLink(अग्र_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                अगर (m_isCoverSeq) consMidSources.धकेलो_back(checkVtxp);
                पुच्छ_Startp = checkVtxp;
                पुच्छ_Min_N = 1;
            }
            अगर (maxN > पुच्छ_Min_N) {
                // Add tail-ring only if the tail is non-empty.
                SvaStateVertex* स्थिर अग्र_Vtxp
                    = addDelayChain(पुच्छ_Startp, maxN - पुच्छ_Min_N + 1, flp, false);
                अग्र_Vtxp->m_delayRingClearCondp
                    = नया AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                अग्र_Vtxp->m_replayAbortReject = true;
                SvaStateVertex* स्थिर checkVtxp = scopedCreateVertex();
                guardedLink(अग्र_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                अगर (m_isCoverSeq) consMidSources.धकेलो_back(checkVtxp);
            }
            currentp = mergeVtxp;
        }
        // finalCond = nullptr (already checked via Links)
        Build_परिणाम res;
        res.termVertexp = currentp;
        res.finalCondp = nullptr;
        res.midSources = std::move(consMidSources);
        // mergeVtxp is the OR of all the end-positions we already pushed to
        // midSources, so the main termVtxp -> matchVertex Link would duplicate.
        res.termIsMidMerge = m_isCoverSeq && !res.midSources.empty();
        वापस res;
    }

    // always[lo:hi] / s_always[lo:hi] (IEEE 1800-2023 16.12.11).
    Build_परिणाम buildPropAlways(AstPropAlways* nodep, SvaStateVertex* entryVtxp,
                                bool is_शीर्ष_Level_Step = false) {
        File_पंक्ति* स्थिर flp = nodep->fileline();
        Ast_गाँठ_व्यंजक* स्थिर propp = nodep->propp();
        स्थिर अचिन्हित lo = getConstUInt(nodep->loBoundp());
        अगर (VN_IS(nodep->hiBoundp(), Unbounded)) {
            // Weak always [lo:$]: unbounded upper bound (IEEE 1800-2023 16.12.11).
            // p must hold at every clock tick at least lo cycles after the attempt
            // start; those ticks are not required to exist, so there is no
            // end-of-trace obligation (weak). The self-loop keeps the attempt live
            // every cycle; each observed cycle is a safety obligation, so a false p
            // rejects immediately.
            UASSERT_OBJ(!nodep->isStrong(), nodep, "Unbounded always must be weak (V3Width)");
            SvaStateVertex* स्थिर livep = addDelayChain(entryVtxp, lo, flp);
            livep->m_isUnbounded = true;
            guardedEdge(livep, livep, flp);  // stay active every subsequent cycle
            SvaStateVertex* स्थिर sinkp = m_graph.createStateVertex();
            sinkp->m_isRejectSink = true;
            SvaTransEdge* स्थिर rejEdgep
                = guardedLink(livep, sinkp, sampled(propp->clone_वृक्ष_Pure(false)), flp);
            अगर (is_शीर्ष_Level_Step) rejEdgep->m_rejectOnFail = true;
            वापस {livep, nullptr, {}};
        }
        स्थिर अचिन्हित hi = getConstUInt(nodep->hiBoundp());
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
        SvaStateVertex* स्थिर checkp = scopedCreateVertex();
        SvaTransEdge* स्थिर linkp
            = guardedLink(currentp, checkp, sampled(propp->clone_वृक्ष_Pure(false)), flp);
        अगर (is_शीर्ष_Level_Step) linkp->m_rejectOnFail = true;
        currentp = checkp;
        m_temporal_Guard_ढेर.धकेलो_back(propp);
        currentp = addDelayChain(currentp, hi - lo, flp);
        अगर (nodep->isStrong() && currentp->m_delay_Ring_आकार) currentp->m_strongAlwaysRing = true;
        m_temporal_Guard_ढेर.निकालो_back();
        वापस {currentp, propp, {}};
    }

    Build_परिणाम buildGotoRep(AstSGotoRep* repp, SvaStateVertex* entryVtxp) {
        File_पंक्ति* स्थिर flp = repp->fileline();
        Ast_गाँठ_व्यंजक* स्थिर exprp = repp->exprp();
        स्थिर अचिन्हित minN = getConstUInt(repp->countp());
        अगर (minN == 0) वापस Build_परिणाम::हार();
        स्थिर bool hasMax = repp->maxCountp() != nullptr;
        स्थिर अचिन्हित maxN = hasMax ? getConstUInt(repp->maxCountp()) : minN;
        अगर (m_isCoverSeq) {
            // Several attempts may coalesce in the one-bit wait vertex before reaching the ring,
            // so a cover sequence action block could run too few times.
            warnEndpointUnsupported(flp, "a goto repetition");
            वापस Build_परिणाम::failWithError();
        }

        AstVar* स्थिर hoistVarp = tryHoistSampled(exprp, flp, 2);

        // The first guardedEdge is the ##1 before waiting for a match. In the wait state, false
        // takes the clocked self-loop, while true takes the zero-delay guardedLink on that tick.
        SvaStateVertex* स्थिर waitVtxp = scopedCreateVertex();
        guardedEdge(entryVtxp, waitVtxp, flp);
        guardedEdge(waitVtxp, waitVtxp,
                    नया AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)}, flp);
        SvaStateVertex* currentp = scopedCreateVertex();
        guardedLink(waitVtxp, currentp, sampledRefOrClone(hoistVarp, exprp, flp), flp);

        अगर (minN > 1) {
            SvaStateVertex* स्थिर ringVtxp = addDelayChain(
                currentp, minN - 1, flp, true, nullptr, sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = true;
            SvaStateVertex* स्थिर checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            currentp = checkVtxp;
        }
        अगर (!hasMax) {
            currentp->m_isUnbounded = true;  // [->N] waits unboundedly
            m_inUnboundedScope = true;
            वापस {currentp, nullptr, {}};
        }

        // [->M:N]: the range ring holds matches from M through N and advances
        // only on expr, preserving arbitrarily long gaps between occurrences.
        SvaStateVertex* स्थिर mergeVtxp = scopedCreateVertex();
        guardedLink(currentp, mergeVtxp, flp);  // accept at match_M
        अगर (maxN > minN) {
            SvaStateVertex* स्थिर ringVtxp
                = addDelayChain(currentp, maxN - minN + 1, flp, false, nullptr,
                                sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = true;
            SvaStateVertex* स्थिर checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            guardedLink(checkVtxp, mergeVtxp, flp);
        }
        mergeVtxp->m_isUnbounded = true;  // [->M:N] still has unbounded waits between matches
        m_inUnboundedScope = true;
        वापस {mergeVtxp, nullptr, {}};
    }

    // True if the sequence has a zero-minimum consecutive repetition, which may match empty
    अटल bool hasZeroMinRep(स्थिर Ast_गाँठ_व्यंजक* seqp) {
        वापस seqp->exists(
            [](स्थिर AstSConsRep* repp) { वापस getConstUInt(repp->countp()) == 0; });
    }

    // True if a leaf's condition has no implication or multi-cycle sequence in it
    // (a leaf can be e.g. 'not (x |-> y)')
    अटल bool isPlainBoolean(स्थिर Ast_गाँठ_व्यंजक* condp) {
        वापस !condp->exists(
            [](स्थिर Ast_गाँठ_व्यंजक* np) { वापस np->isMultiCycleSva() || VN_IS(np, Implication); });
    }

    // Build merge vertex for SOr / LogOr: both branches feed into one vertex.
    Build_परिणाम buildOrMerge(Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp, SvaStateVertex* entryVtxp,
                             File_पंक्ति* flp, bool wantBoolLeaf) {
        स्थिर Build_परिणाम lhs = build_व्यंजक(lhsp, entryVtxp);
        स्थिर Build_परिणाम rhs = build_व्यंजक(rhsp, entryVtxp);
        अगर (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            वापस Build_परिणाम::हार(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP
        // IEEE 1800-2023 16.14.3: a cover sequence counts every end-of-match. A
        // sequence operand of 'or' can end more than once, but only its final
        // end reaches the merge vertex below, so reject sequence operands rather
        // than under-count. Plain boolean disjunction has one end per cycle and
        // is handled by the OR-fold.
        स्थिर bool booleanOnly = lhs.termVertexp == entryVtxp && lhs.finalCondp
                                 && rhs.termVertexp == entryVtxp && rhs.finalCondp;
        अगर (m_isCoverSeq && !booleanOnly) {
            warnEndpointUnsupported(flp, "a sequence operand of 'or'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            वापस Build_परिणाम::failWithError();
        }
        अगर (booleanOnly && wantBoolLeaf) {
            अगर (isPlainBoolean(lhs.finalCondp) && isPlainBoolean(rhs.finalCondp)) {
                // As a |-> / |=> or top-level body, a disjunction of two plain booleans must
                // be able to reject: return it as a boolean finalCondp. The merge vertex
                // below drops the attempt silently when both operands are false.
                Ast_गाँठ_व्यंजक* स्थिर orCondp
                    = नया AstLogOr{flp, lhs.finalCondp->clone_वृक्ष_Pure(false),
                                   rhs.finalCondp->clone_वृक्ष_Pure(false)};
                freeUnlinkedCondp(lhs.finalCondp);
                freeUnlinkedCondp(rhs.finalCondp);
                वापस {entryVtxp, orCondp, {}};
            }
        }
        SvaStateVertex* स्थिर mergeVtxp = scopedCreateVertex();
        अगर (booleanOnly) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsp,
                        "Single-cycle SOr operands must have finalCondp");
            guardedLink(entryVtxp, mergeVtxp,
                        नया AstLogOr{flp, sampled(lhs.finalCondp->clone_वृक्ष_Pure(false)),
                                     sampled(rhs.finalCondp->clone_वृक्ष_Pure(false))},
                        flp);
        } अन्यथा {
            अगर (lhs.finalCondp) {
                guardedLink(lhs.termVertexp, mergeVtxp,
                            sampled(lhs.finalCondp->clone_वृक्ष_Pure(false)), flp);
            } अन्यथा {
                guardedLink(lhs.termVertexp, mergeVtxp, flp);
            }
            अगर (rhs.finalCondp) {
                guardedLink(rhs.termVertexp, mergeVtxp,
                            sampled(rhs.finalCondp->clone_वृक्ष_Pure(false)), flp);
            } अन्यथा {
                guardedLink(rhs.termVertexp, mergeVtxp, flp);
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        वापस {mergeVtxp, nullptr, {}};
    }

    // Build done-latch combiner for SAnd/SIntersect (IEEE 1800-2023 16.9.5).
    Build_परिणाम buildAndCombiner(Ast_गाँठ_व्यंजक* lhsExprp, Ast_गाँठ_व्यंजक* rhsExprp,
                                 SvaStateVertex* entryVtxp, File_पंक्ति* flp) {
        // Snapshot-restore scope so LHS liveness does not leak into RHS.
        स्थिर bool savedScope = m_inUnboundedScope;
        स्थिर Build_परिणाम lhs = build_व्यंजक(lhsExprp, entryVtxp);
        स्थिर bool lhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope;
        स्थिर Build_परिणाम rhs = build_व्यंजक(rhsExprp, entryVtxp);
        स्थिर bool rhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope || lhsScope || rhsScope;
        अगर (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            वापस Build_परिणाम::हार(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP

        // Single-cycle operands: use boolean AND (done-latch would fire across cycles).
        // If both operands stayed at entry, they must be boolean leaves which
        // buildExpr returns with finalCondp=nodep (non-null).
        अगर (lhs.termVertexp == entryVtxp && rhs.termVertexp == entryVtxp) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsExprp,
                        "Single-cycle SAnd operands must have finalCondp");
            Ast_गाँठ_व्यंजक* स्थिर condp = नया AstLogAnd{flp, lhs.finalCondp->clone_वृक्ष_Pure(false),
                                                     rhs.finalCondp->clone_वृक्ष_Pure(false)};
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            वापस {entryVtxp, condp, {}};
        }
        // Range-delay mid-window sources in either sub-branch would need
        // to be folded into the latch's match-now signal, which the
        // current combiner does not support.
        अगर (!lhs.midSources.empty() || !rhs.midSources.empty()) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: ranged cycle delay in an operand of property 'and'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            वापस Build_परिणाम::failWithError();
        }
        SvaStateVertex* स्थिर combVtxp = scopedCreateVertex();
        combVtxp->m_isAndCombiner = true;
        combVtxp->m_andLhsTermp = lhs.termVertexp;
        combVtxp->m_andRhsTermp = rhs.termVertexp;
        अगर (lhs.finalCondp) combVtxp->m_andLhsCondp = lhs.finalCondp->clone_वृक्ष_Pure(false);
        अगर (rhs.finalCondp) combVtxp->m_andRhsCondp = rhs.finalCondp->clone_वृक्ष_Pure(false);
        अगर (lhs.termVertexp->m_isUnbounded || rhs.termVertexp->m_isUnbounded) {
            combVtxp->m_isUnbounded = true;
        }
        // Wire terminal-boolean rejects to a dedicated sink so each side can fail
        // the AND independently. Skip for liveness or single-cycle operands
        // (single-cycle termVertexp == entryVtxp would fire every cycle).
        अगर (!combVtxp->m_isUnbounded) {
            bool needSink = false;
            स्थिर bool lhsMultiCycle = (lhs.termVertexp != entryVtxp);
            स्थिर bool rhsMultiCycle = (rhs.termVertexp != entryVtxp);
            अगर (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                needSink = true;
            }
            अगर (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                needSink = true;
            }
            अगर (needSink) {
                SvaStateVertex* स्थिर sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = true;
                अगर (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* स्थिर ep = m_graph.addLink(
                        lhs.termVertexp, sinkVtxp, sampled(lhs.finalCondp->clone_वृक्ष_Pure(false)));
                    ep->m_rejectOnFail = true;
                }
                अगर (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* स्थिर ep = m_graph.addLink(
                        rhs.termVertexp, sinkVtxp, sampled(rhs.finalCondp->clone_वृक्ष_Pure(false)));
                    ep->m_rejectOnFail = true;
                }
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        वापस {combVtxp, nullptr, {}};
    }

    // Lower `seq1 within seq2` (IEEE 1800-2023 16.9.10) as:
    //   (OR_{i in 0..slack} 1 ##i seq1 ##(slack-i) 1) intersect seq2
    // Both operands must have fixed length (no ranged cycle delays).
    // The OR lives inside a single SIntersect so one AndCombiner done-latch
    // serves all offsets; lifting it out would double-count matches where
    // multiple offsets accept on the same seq2 end cycle.
    Build_परिणाम buildSWithin(AstSWithin* nodep, SvaStateVertex* entryVtxp,
                             bool is_शीर्ष_Level_Step = false) {
        स्थिर पूर्णांक innerLen = fixed_लंबाई(nodep->lhsp());
        स्थिर पूर्णांक outerLen = fixed_लंबाई(nodep->rhsp());
        अगर (innerLen < 0 || outerLen < 0) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: within with ranged cycle-delay operand");
            वापस Build_परिणाम::failWithError();
        }
        अगर (innerLen > outerLen) {
            वापस buildNeverMatchIntersect(
                nodep, entryVtxp, is_शीर्ष_Level_Step,
                "the inner sequence is longer than the outer sequence");
        }
        File_पंक्ति* स्थिर flp = nodep->fileline();
        स्थिर पूर्णांक slack = outerLen - innerLen;
        Ast_गाँठ_व्यंजक* innerOrp = nullptr;
        क्रम (पूर्णांक i = 0; i <= slack; ++i) {
            स्थिर पूर्णांक postPad = slack - i;
            Ast_गाँठ_व्यंजक* branchp = nodep->lhsp()->clone_वृक्ष_Pure(false);
            अगर (i > 0) {
                AstConst* स्थिर prePadp = नया AstConst{flp, AstConst::BitTrue{}};
                AstDelay* स्थिर delayp
                    = नया AstDelay{flp, नया AstConst{flp, static_cast<uint32_t>(i)}, true};
                AstSExpr* स्थिर wrapped = नया AstSExpr{flp, prePadp, delayp, branchp};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            अगर (postPad > 0) {
                AstConst* स्थिर postTruep = नया AstConst{flp, AstConst::BitTrue{}};
                AstDelay* स्थिर delayp
                    = नया AstDelay{flp, नया AstConst{flp, static_cast<uint32_t>(postPad)}, true};
                AstSExpr* स्थिर wrapped = नया AstSExpr{flp, branchp, delayp, postTruep};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            innerOrp = innerOrp ? static_cast<Ast_गाँठ_व्यंजक*>(नया AstSOr{flp, innerOrp, branchp})
                                : branchp;
        }
        Ast_गाँठ_व्यंजक* स्थिर outerClonep = nodep->rhsp()->clone_वृक्ष_Pure(false);
        Ast_गाँठ_व्यंजक* स्थिर combinedp = नया AstSIntersect{flp, innerOrp, outerClonep};
        Build_परिणाम परिणाम = build_व्यंजक(combinedp, entryVtxp, is_शीर्ष_Level_Step);
        VL_DO_DANGLING(combinedp->delete_वृक्ष(), combinedp);
        // When both operands are plain booleans, buildAndCombiner returns a
        // freshly-allocated AstAnd as finalCondp with no parent. Callers up
        // the chain clone-and-discard finalCondp, so leave it parent-less and
        // it leaks; anchor it in the graph now via an edge.
        अगर (परिणाम.valid() && परिणाम.finalCondp && !परिणाम.finalCondp->backp()) {
            SvaStateVertex* स्थिर wrapVtxp = scopedCreateVertex();
            guardedLink(परिणाम.termVertexp, wrapVtxp, sampled(परिणाम.finalCondp), flp);
            परिणाम = {wrapVtxp, nullptr, परिणाम.midSources};
        }
        वापस परिणाम;
    }

    // Collect the boolean leaf checks of a fixed-length sequence keyed by their
    // clock offset from the start. Returns false for anything other than nested
    // AstSExpr with fixed cycle delays over boolean leaves (e.g. throughout).
    अटल bool flattenFixedSeq(Ast_गाँठ_व्यंजक* nodep, पूर्णांक baseOffset,
                                std::map<पूर्णांक, std::vector<Ast_गाँठ_व्यंजक*>>& out) {
        अगर (AstSExpr* स्थिर sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* स्थिर delayp = VN_CAST(sexprp->delayp(), Delay);
            अगर (!delayp || !delayp->isCycleDelay() || delayp->isUnbounded()) वापस false;
            स्थिर अचिन्हित delayCycles = getConstUInt(delayp->lhsp());
            अगर (delayp->isRangeDelay() && getConstUInt(delayp->rhsp()) != delayCycles)
                वापस false;
            पूर्णांक preLen = 0;
            अगर (Ast_गाँठ_व्यंजक* स्थिर prep = sexprp->preExprp()) {
                अगर (!flattenFixedSeq(prep, baseOffset, out)) वापस false;
                preLen = fixed_लंबाई(prep);
                अगर (preLen < 0) वापस false;
            }
            वापस flattenFixedSeq(sexprp->exprp(), baseOffset + preLen + delayCycles, out);
        }
        अगर (nodep->isMultiCycleSva()) वापस false;
        out[baseOffset].धकेलो_back(nodep);
        वापस true;
    }

    // Conjoin two equal-length fixed sequences into one: at each clock offset
    // AND the boolean checks of both operands (IEEE 1800-2023 16.9.6 -- both
    // operands match the same window). Returns null if either operand is not a
    // plain fixed sequence of boolean leaves.
    अटल Ast_गाँठ_व्यंजक* conjoinFixedSeqs(Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp, File_पंक्ति* flp) {
        std::map<पूर्णांक, std::vector<Ast_गाँठ_व्यंजक*>> checks;
        अगर (!flattenFixedSeq(lhsp, 0, checks) || !flattenFixedSeq(rhsp, 0, checks)) वापस nullptr;
        अगर (checks.empty()) वापस nullptr;
        Ast_गाँठ_व्यंजक* resultp = nullptr;
        पूर्णांक पूर्व_Offset = 0;
        क्रम (स्थिर स्वतः& offsetChecks : checks) {
            स्थिर पूर्णांक offset = offsetChecks.first;
            Ast_गाँठ_व्यंजक* condp = nullptr;
            क्रम (Ast_गाँठ_व्यंजक* स्थिर leafp : offsetChecks.second) {
                Ast_गाँठ_व्यंजक* स्थिर clonep = leafp->clone_वृक्ष_Pure(false);
                अगर (!condp) {
                    condp = clonep;
                } अन्यथा {
                    condp = नया AstLogAnd{flp, condp, clonep};
                    condp->dtypeSetBit();
                }
            }
            अगर (!resultp) {
                अगर (offset > 0) {
                    AstDelay* स्थिर delayp = नया AstDelay{
                        flp, नया AstConst{flp, static_cast<uint32_t>(offset)}, /*isCycle=*/true};
                    resultp
                        = नया AstSExpr{flp, नया AstConst{flp, AstConst::BitTrue{}}, delayp, condp};
                    resultp->dtypeSetBit();
                } अन्यथा {
                    resultp = condp;
                }
            } अन्यथा {
                AstDelay* स्थिर delayp = नया AstDelay{
                    flp, नया AstConst{flp, static_cast<uint32_t>(offset - पूर्व_Offset)},
                    /*isCycle=*/true};
                resultp = नया AstSExpr{flp, resultp, delayp, condp};
                resultp->dtypeSetBit();
            }
            पूर्व_Offset = offset;
        }
        वापस resultp;
    }

    // `seq` is a simple ranged sequence `start ##[m:n] end` (start/end boolean,
    // start may be absent). Used to collapse a both-variable intersect to one
    // ranged delay.
    काष्ठा SimpleRanged final {
        bool ok = false;
        Ast_गाँठ_व्यंजक* startp = nullptr;  // may be null (absent start)
        Ast_गाँठ_व्यंजक* endp = nullptr;
    };
    अटल SimpleRanged asSimpleRanged(Ast_गाँठ_व्यंजक* nodep) {
        AstSExpr* स्थिर sexprp = VN_CAST(nodep, SExpr);
        अगर (!sexprp) वापस {};
        AstDelay* स्थिर delayp = VN_CAST(sexprp->delayp(), Delay);
        अगर (!delayp || !delayp->isCycleDelay() || !delayp->isRangeDelay() || delayp->isUnbounded())
            वापस {};
        अगर (getConstUInt(delayp->lhsp()) == getConstUInt(delayp->rhsp())) वापस {};
        Ast_गाँठ_व्यंजक* स्थिर prep = sexprp->preExprp();
        अगर (prep && fixed_लंबाई(prep) != 0) वापस {};
        अगर (fixed_लंबाई(sexprp->exprp()) != 0) वापस {};
        वापस {true, prep, sexprp->exprp()};
    }

    // Build the NFA for a synthesized intersect lowering tree, then free it.
    // buildExpr returns the terminal condition (finalCondp) by reference into the
    // tree; detach a clone so the tree can be freed here. The graph already holds
    // clones/hoists of every edge condition, so nothing else dangles.
    Build_परिणाम build_From_Lowering_वृक्ष(Ast_गाँठ_व्यंजक* treep, SvaStateVertex* entryVtxp,
                                      bool is_शीर्ष_Level_Step) {
        Build_परिणाम परिणाम = build_व्यंजक(treep, entryVtxp, is_शीर्ष_Level_Step);
        अगर (परिणाम.valid() && परिणाम.finalCondp) {
            परिणाम.finalCondp = परिणाम.finalCondp->clone_वृक्ष_Pure(false);
        }
        VL_DO_DANGLING(treep->delete_वृक्ष(), treep);
        वापस परिणाम;
    }

    // Empty common-length intersection -- unequal fixed lengths, or disjoint
    // ranged lengths. IEEE 1800-2023 16.9.6 requires both operands to match
    // over a window of the same length, so with no common length the intersect
    // simply never matches. This is legal (matching nothing), not an error, so
    // lower to a constant false rather than rejecting legal code.
    Build_परिणाम buildNeverMatchIntersect(Ast_गाँठ_व्यंजक* nodep, SvaStateVertex* entryVtxp,
                                         bool is_शीर्ष_Level_Step, स्थिर अक्षर* reason) {
        nodep->v3warn(NEVERMATCH, "Sequence can never match because " << reason << ".");
        Ast_गाँठ_व्यंजक* स्थिर falsep = नया AstConst{nodep->fileline(), AstConst::BitFalse{}};
        वापस build_From_Lowering_वृक्ष(falsep, entryVtxp, is_शीर्ष_Level_Step);
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
    Build_परिणाम buildVarLenIntersect(AstSIntersect* nodep, SvaStateVertex* entryVtxp,
                                     bool is_शीर्ष_Level_Step) {
        स्थिर std::pair<पूर्णांक, पूर्णांक> lhsRange = लंबाई_Range(nodep->lhsp());
        स्थिर std::pair<पूर्णांक, पूर्णांक> rhsRange = लंबाई_Range(nodep->rhsp());
        अगर (lhsRange.first < 0 || rhsRange.first < 0) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect with this variable-length operand");
            वापस Build_परिणाम::failWithError();
        }
        स्थिर पूर्णांक lo = std::max(lhsRange.first, rhsRange.first);
        स्थिर पूर्णांक hi = std::min(lhsRange.second, rhsRange.second);
        अगर (lo > hi) {
            // Disjoint length ranges share no common length -> never matches.
            वापस buildNeverMatchIntersect(nodep, entryVtxp, is_शीर्ष_Level_Step,
                                            "intersect operands have no common length");
        }
        File_पंक्ति* स्थिर flp = nodep->fileline();
        अगर (lo == hi) {
            Ast_गाँठ_व्यंजक* स्थिर lp = realize_At_लंबाई(nodep->lhsp(), lo, lhsRange.first);
            Ast_गाँठ_व्यंजक* स्थिर rp = realize_At_लंबाई(nodep->rhsp(), lo, rhsRange.first);
            Ast_गाँठ_व्यंजक* स्थिर conjp = conjoinFixedSeqs(lp, rp, flp);
            VL_DO_DANGLING(lp->delete_वृक्ष(), lp);
            VL_DO_DANGLING(rp->delete_वृक्ष(), rp);
            अगर (!conjp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: intersect operand is not a plain boolean sequence");
                वापस Build_परिणाम::failWithError();
            }
            वापस build_From_Lowering_वृक्ष(conjp, entryVtxp, is_शीर्ष_Level_Step);
        }
        स्थिर SimpleRanged sl = asSimpleRanged(nodep->lhsp());
        स्थिर SimpleRanged sr = asSimpleRanged(nodep->rhsp());
        अगर (!sl.ok || !sr.ok) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect of two sequences that each vary in length over a"
                          " range with internal structure");
            वापस Build_परिणाम::failWithError();
        }
        स्थिर स्वतः andBool = [&](Ast_गाँठ_व्यंजक* ap, Ast_गाँठ_व्यंजक* bp) -> Ast_गाँठ_व्यंजक* {
            Ast_गाँठ_व्यंजक* स्थिर aClonep
                = ap ? ap->clone_वृक्ष_Pure(false) : नया AstConst{flp, AstConst::BitTrue{}};
            Ast_गाँठ_व्यंजक* स्थिर bClonep
                = bp ? bp->clone_वृक्ष_Pure(false) : नया AstConst{flp, AstConst::BitTrue{}};
            AstLogAnd* स्थिर andp = नया AstLogAnd{flp, aClonep, bClonep};
            andp->dtypeSetBit();
            वापस andp;
        };
        AstDelay* स्थिर delayp = नया AstDelay{flp, नया AstConst{flp, static_cast<uint32_t>(lo)},
                                              /*isCycle=*/true};
        delayp->rhsp(नया AstConst{flp, static_cast<uint32_t>(hi)});
        AstSExpr* स्थिर reducedp
            = नया AstSExpr{flp, andBool(sl.startp, sr.startp), delayp, andBool(sl.endp, sr.endp)};
        reducedp->dtypeSetBit();
        वापस build_From_Lowering_वृक्ष(reducedp, entryVtxp, is_शीर्ष_Level_Step);
    }

    Build_परिणाम buildThroughout(AstSThroughout* nodep, SvaStateVertex* entryVtxp,
                                bool is_शीर्ष_Level_Step = false) {
        // Mark entryVtxp so "cond false at tick 0" is detected as throughout-drop.
        entryVtxp->m_throughoutConds.धकेलो_back(nodep->lhsp()->clone_वृक्ष_Pure(false));
        m_temporal_Guard_ढेर.धकेलो_back(nodep->lhsp());
        स्थिर Build_परिणाम परिणाम = build_व्यंजक(nodep->rhsp(), entryVtxp, is_शीर्ष_Level_Step);
        m_temporal_Guard_ढेर.निकालो_back();
        वापस परिणाम;
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
    Build_परिणाम buildUntil(AstUntil* nodep, SvaStateVertex* entryVtxp, bool is_शीर्ष_Level_Step) {
        File_पंक्ति* स्थिर flp = nodep->fileline();
        अगर (!is_शीर्ष_Level_Step) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            वापस Build_परिणाम::failWithError();
        }
        Ast_गाँठ_व्यंजक* स्थिर lhsp = nodep->lhsp();
        Ast_गाँठ_व्यंजक* स्थिर rhsp = nodep->rhsp();
        स्थिर स्वतः hasSeq = [](स्थिर Ast_गाँठ_व्यंजक* ep) {
            वापस ep->exists([](स्थिर Ast_गाँठ_व्यंजक* np) { वापस np->isMultiCycleSva(); });
        };
        अगर (hasSeq(lhsp) || hasSeq(rhsp)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            वापस Build_परिणाम::failWithError();
        }

        स्थिर bool ov = nodep->isOverlapping();
        Ast_गाँठ_व्यंजक* स्थिर lhsBitp = nodep->lhsp();
        Ast_गाँठ_व्यंजक* स्थिर rhsBitp = nodep->rhsp();
        // p hoist count: continue, require (ov: 1 use; nov: 1 use). At least 2 uses.
        AstVar* स्थिर pHoistp = tryHoistSampled(lhsBitp, flp, 2);
        // q hoist count: continue (1) + require nov (1) = 2; ov: continue only (1).
        AstVar* स्थिर qHoistp = ov ? nullptr : tryHoistSampled(rhsBitp, flp, 2);

        SvaStateVertex* स्थिर waitCp = scopedCreateVertex();
        SvaStateVertex* स्थिर waitRp = scopedCreateVertex();
        waitCp->m_isUnbounded = true;
        waitRp->m_strongPending = nodep->isStrong() && !m_isCover;

        // Entry and back-loop Links carry no condition; throughout-folding still applies.
        guardedLink(entryVtxp, waitCp, flp);
        guardedLink(waitRp, waitCp, flp);

        // Continue clocked edge: p && !q advances to next-cycle wait.
        Ast_गाँठ_व्यंजक* स्थिर contCondp
            = नया AstLogAnd{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                            नया AstLogNot{flp, sampledRefOrClone(qHoistp, rhsBitp, flp)}};
        guardedEdge(waitCp, waitRp, contCondp, flp);

        // Reject sink: fires when require-condition is false.
        SvaStateVertex* स्थिर sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = true;
        Ast_गाँठ_व्यंजक* requireCondp;
        अगर (ov) {
            requireCondp = sampledRefOrClone(pHoistp, lhsBitp, flp);
        } अन्यथा {
            requireCondp = नया AstLogOr{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                                        sampledRefOrClone(qHoistp, rhsBitp, flp)};
        }
        SvaTransEdge* स्थिर rejEdgep = m_graph.addLink(waitCp, sinkVtxp, requireCondp);
        rejEdgep->m_rejectOnFail = true;

        // Accept condition rides via finalCondp; assembleResult $sampled-wraps it.
        Ast_गाँठ_व्यंजक* acceptCondp;
        अगर (ov) {
            acceptCondp
                = नया AstLogAnd{flp, lhsBitp->clone_वृक्ष_Pure(false), rhsBitp->clone_वृक्ष_Pure(false)};
        } अन्यथा {
            acceptCondp = rhsBitp->clone_वृक्ष_Pure(false);
        }
        वापस {waitCp, acceptCondp, {}};
    }

    // IEEE 1800-2023 16.12.14 property abort operators. Sync and async share
    // the same NFA encoding: AstSampled already gives matured values at every
    // maturing clocking event, and async firing "between clocks" is not
    // observable in a cycle-based model. VAbortKind selects accept vs reject
    // verdict (sync vs async only changes the user-visible spelling).

    // Build `condp && !outer_1 && !outer_2 ...` (unsampled).
    Ast_गाँठ_व्यंजक* abort_Fire_व्यंजक(Ast_गाँठ_व्यंजक* condp, File_पंक्ति* flp) {
        Ast_गाँठ_व्यंजक* resultp = condp->clone_वृक्ष_Pure(false);
        क्रम (Ast_गाँठ_व्यंजक* स्थिर op : m_outer_Abort_ढेर)
            resultp = नया AstLogAnd{flp, resultp, नया AstLogNot{flp, op->clone_वृक्ष_Pure(false)}};
        वापस resultp;
    }

    // True when unguarded ring-wide rejection or a same-tick Link chain already accounts the
    // attempt: a required-step Link covers both outcomes; followed-by pairs both edges.
    अटल bool chainAccountsSource(स्थिर SvaStateVertex* srcp,
                                    स्थिर std::unordered_set<स्थिर V3GraphEdge*>& preEdges) {
        अगर (srcp->m_delay_Ring_आकार && srcp->m_throughoutConds.empty()) वापस true;
        क्रम (स्थिर V3GraphEdge& edger : srcp->inEdges()) {
            अगर (preEdges.गिनती(&edger)) जारी;
            स्थिर SvaTransEdge& tedger = static_cast<स्थिर SvaTransEdge&>(edger);
            अगर (tedger.m_consumesCycle) जारी;
            स्थिर स्वतः* स्थिर fromp = static_cast<स्थिर SvaStateVertex*>(tedger.fromVtxp());
            अगर (fromp->m_abortRejectp) वापस true;
        }
        bool plainNonSink = false;
        bool markedSink = false;
        क्रम (स्थिर V3GraphEdge& edger : srcp->outEdges()) {
            अगर (preEdges.गिनती(&edger)) जारी;
            स्थिर SvaTransEdge& tedger = static_cast<स्थिर SvaTransEdge&>(edger);
            अगर (tedger.m_consumesCycle) जारी;
            स्थिर bool sink = static_cast<स्थिर SvaStateVertex*>(tedger.toVtxp())->m_isRejectSink;
            अगर (tedger.m_rejectOnFail) {
                अगर (!sink) वापस true;
                markedSink = true;
            } अन्यथा अगर (!sink) {
                अगर (srcp->m_abortRejectp) वापस true;
                plainNonSink = true;
            }
        }
        वापस plainNonSink && markedSink;
    }

    // Reject edge: fires when the source is live and the abort samples true.
    व्योम addAbortRejectEdge(SvaStateVertex* srcp, SvaStateVertex* sinkp, Ast_गाँठ_व्यंजक* condp,
                            File_पंक्ति* flp) {
        Ast_गाँठ_व्यंजक* स्थिर notFirep = नया AstLogNot{flp, sampled(abort_Fire_व्यंजक(condp, flp))};
        m_graph.addLink(srcp, sinkp, notFirep)->m_rejectOnFail = true;
        वापस;
    }

    // On the fire tick: kill body threads; accept kinds also forgive step misses.
    व्योम gateBodyEdgesOnAbort(स्थिर std::unordered_set<स्थिर V3GraphEdge*>& preEdges,
                              Ast_गाँठ_व्यंजक* condp, VAbortKind kind, File_पंक्ति* flp) {
        क्रम (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            क्रम (V3GraphEdge& edger : vtxr.outEdges()) {
                अगर (preEdges.गिनती(&edger)) जारी;
                SvaTransEdge* स्थिर tedgep = static_cast<SvaTransEdge*>(&edger);
                अगर (tedgep->m_rejectOnFail) {
                    अगर (!kind.isAccept()) जारी;
                    Ast_गाँठ_व्यंजक* स्थिर firep = sampled(abort_Fire_व्यंजक(condp, flp));
                    tedgep->m_condp
                        = tedgep->m_condp ? नया AstLogOr{flp, tedgep->m_condp, firep} : firep;
                } अन्यथा अगर (tedgep->m_consumesCycle) {
                    Ast_गाँठ_व्यंजक* स्थिर notFirep
                        = नया AstLogNot{flp, sampled(abort_Fire_व्यंजक(condp, flp))};
                    tedgep->m_condp = tedgep->m_condp
                                          ? नया AstLogAnd{flp, tedgep->m_condp, notFirep}
                                          : notFirep;
                }
            }
        }
    }

    Build_परिणाम buildAbortOn(Ast_गाँठ_व्यंजक* condp, Ast_गाँठ_व्यंजक* bodyp, SvaStateVertex* entryVtxp,
                             VAbortKind kind, File_पंक्ति* flp, bool is_शीर्ष_Level_Step) {
        // Snapshot pre-body vertices/edges so post-build diff yields the body's sub-NFA.
        std::unordered_set<स्थिर V3GraphVertex*> preExisting;
        std::unordered_set<स्थिर V3GraphEdge*> preEdges;
        क्रम (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            preExisting.जोड़ो(&vtxr);
            क्रम (V3GraphEdge& edger : vtxr.outEdges()) preEdges.जोड़ो(&edger);
        }

        m_outer_Abort_ढेर.धकेलो_back(condp);
        स्थिर Build_परिणाम body_परिणाम = build_व्यंजक(bodyp, entryVtxp, is_शीर्ष_Level_Step);
        m_outer_Abort_ढेर.निकालो_back();
        अगर (!body_परिणाम.valid()) वापस body_परिणाम;

        gateBodyEdgesOnAbort(preEdges, condp, kind, flp);

        // Live-thread sources for the abort edge: entry + new body vertices,
        // minus reject sinks (they carry reject fuel, not live-thread fuel).
        std::vector<SvaStateVertex*> abortSources;
        abortSources.धकेलो_back(entryVtxp);
        क्रम (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            अगर (preExisting.गिनती(&vtxr)) जारी;
            स्वतः* स्थिर sp = static_cast<SvaStateVertex*>(&vtxr);
            अगर (sp->m_delay_Ring_आकार || !sp->m_throughoutConds.empty()) {
                Ast_गाँठ_व्यंजक* स्थिर firep = abort_Fire_व्यंजक(condp, flp);
                sp->m_abortClearp
                    = sp->m_abortClearp ? नया AstLogOr{flp, sp->m_abortClearp, firep} : firep;
            }
            अगर (!kind.isAccept()
                && ((sp->m_delay_Ring_आकार && sp->m_throughoutConds.empty())
                    || sp->m_replayAbortReject)) {
                Ast_गाँठ_व्यंजक* स्थिर firep = abort_Fire_व्यंजक(condp, flp);
                sp->m_abortRejectp
                    = sp->m_abortRejectp ? नया AstLogOr{flp, sp->m_abortRejectp, firep} : firep;
            }
            अगर (sp->m_isRejectSink) जारी;
            abortSources.धकेलो_back(sp);
        }

        स्वतः sampledAbortFire = [&]() -> Ast_गाँठ_व्यंजक* {
            Ast_गाँठ_व्यंजक* स्थिर व्यंजक = abort_Fire_व्यंजक(condp, flp);
            वापस sampled(व्यंजक);
        };

        अगर (kind.isAccept()) {
            // Match-only sink fed by $sampled(abort-fire) from every live source;
            // registered as midSource so it never contributes a reject.
            SvaStateVertex* स्थिर acceptSinkp = scopedCreateVertex();
            क्रम (SvaStateVertex* स्थिर srcp : abortSources)
                guardedLink(srcp, acceptSinkp, sampledAbortFire(), flp);
            std::vector<SvaStateVertex*> midSources = body_परिणाम.midSources;
            midSources.धकेलो_back(acceptSinkp);
            Ast_गाँठ_व्यंजक* finalCondp = body_परिणाम.finalCondp;
            अगर (finalCondp) {
                अगर (finalCondp->backp()) finalCondp = finalCondp->clone_वृक्ष_Pure(false);
                finalCondp = नया AstLogOr{flp, finalCondp, abort_Fire_व्यंजक(condp, flp)};
            }
            वापस {body_परिणाम.termVertexp, finalCondp, std::move(midSources)};
        }

        // rejectOnFail treats m_condp as the success condition and fires on
        // !condp, so the edge carries !sampledAbortFire().
        SvaStateVertex* स्थिर rejectSinkp = m_graph.createStateVertex();
        rejectSinkp->m_isRejectSink = true;
        क्रम (SvaStateVertex* स्थिर srcp : abortSources)
            अगर (!chainAccountsSource(srcp, preEdges))
                addAbortRejectEdge(srcp, rejectSinkp, condp, flp);
        Ast_गाँठ_व्यंजक* finalCondp = body_परिणाम.finalCondp;
        अगर (finalCondp) {
            अगर (finalCondp->backp()) finalCondp = finalCondp->clone_वृक्ष_Pure(false);
            finalCondp
                = नया AstLogAnd{flp, finalCondp, नया AstLogNot{flp, abort_Fire_व्यंजक(condp, flp)}};
        }
        वापस {body_परिणाम.termVertexp, finalCondp, body_परिणाम.midSources};
    }

खुला:
    // Free a dropped sub-result condition that is not linked into the AST
    // (abort folds synthesize unparented finalCondp trees).
    अटल व्योम freeUnlinkedCondp(Ast_गाँठ_व्यंजक* condp) {
        अगर (condp && !condp->backp()) VL_DO_DANGLING(condp->delete_वृक्ष(), condp);
    }

    SvaNfaBuilder(SvaGraph& graph, Ast_गाँठ_Module* modp, V3UniqueNames& prop_अस्थायी_Names,
                  bool isCoverSeq = false, bool isSeqEvent = false, bool isCover = false)
        : m_graph{graph}
        , m_modp{modp}
        , m_prop_अस्थायी_Names{prop_अस्थायी_Names}
        , m_isCover{isCover}
        , m_isCoverSeq{isCoverSeq}
        , m_isSeqEvent{isSeqEvent} {}

    // Reset scope between antecedent and consequent: liveness must not leak.
    // m_outerAbortStack survives: an abort wrapping the implication covers the
    // consequent too (IEEE 1800-2023 16.12.14).
    व्योम resetScope() {
        m_inUnboundedScope = false;
        m_temporal_Guard_ढेर.सफा();
    }

    Build_परिणाम build_व्यंजक(Ast_गाँठ_व्यंजक* nodep, SvaStateVertex* entryVtxp,
                          bool is_शीर्ष_Level_Step = false) {
        VL_RESTORER(m_wantBoolLeaf);
        स्थिर bool wantBoolLeaf = m_wantBoolLeaf;
        m_wantBoolLeaf = false;
        अगर (AstSExpr* स्थिर sexprp = VN_CAST(nodep, SExpr)) {
            वापस buildSExpr(sexprp, entryVtxp, is_शीर्ष_Level_Step);
        }
        अगर (AstSConsRep* स्थिर repp = VN_CAST(nodep, SConsRep)) {
            वापस buildConsRep(repp, entryVtxp, is_शीर्ष_Level_Step);
        }
        अगर (AstPropAlways* स्थिर alwaysp = VN_CAST(nodep, PropAlways)) {
            वापस buildPropAlways(alwaysp, entryVtxp, is_शीर्ष_Level_Step);
        }
        अगर (AstSGotoRep* स्थिर repp = VN_CAST(nodep, SGotoRep)) {
            वापस buildGotoRep(repp, entryVtxp);
        }
        अगर (AstSThroughout* स्थिर throughoutp = VN_CAST(nodep, SThroughout)) {
            वापस buildThroughout(throughoutp, entryVtxp, is_शीर्ष_Level_Step);
        }
        अगर (AstSOr* स्थिर orp = VN_CAST(nodep, SOr)) {
            वापस buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        अगर (AstLogOr* स्थिर orp = VN_CAST(nodep, LogOr)) {
            वापस buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        अगर (AstSAnd* स्थिर andp = VN_CAST(nodep, SAnd)) {
            वापस buildAndCombiner(andp->lhsp(), andp->rhsp(), entryVtxp, andp->fileline());
        }
        अगर (AstSIntersect* स्थिर intp = VN_CAST(nodep, SIntersect)) {
            // IEEE 1800-2023 16.9.6: both operands match over one window with
            // equal start and end (equal length). Lower to a single sequence
            // that conjoins both operands' per-cycle checks -- correct under
            // concurrent attempts, where the done-latch combiner conflates the
            // two operands' start times and over-accepts. The combiner remains
            // only as a fallback for operands that do not flatten.
            स्थिर पूर्णांक lhsLen = fixed_लंबाई(intp->lhsp());
            स्थिर पूर्णांक rhsLen = fixed_लंबाई(intp->rhsp());
            अगर (lhsLen >= 0 && rhsLen >= 0) {
                अगर (lhsLen != rhsLen) {
                    // Unequal fixed lengths share no common length -> never matches.
                    वापस buildNeverMatchIntersect(intp, entryVtxp, is_शीर्ष_Level_Step,
                                                    "intersect operands have no common length");
                }
                अगर (Ast_गाँठ_व्यंजक* स्थिर conjp
                    = conjoinFixedSeqs(intp->lhsp(), intp->rhsp(), intp->fileline())) {
                    वापस build_From_Lowering_वृक्ष(conjp, entryVtxp, is_शीर्ष_Level_Step);
                }
                वापस buildAndCombiner(intp->lhsp(), intp->rhsp(), entryVtxp, intp->fileline());
            }
            वापस buildVarLenIntersect(intp, entryVtxp, is_शीर्ष_Level_Step);
        }
        अगर (AstSWithin* स्थिर withinp = VN_CAST(nodep, SWithin)) {
            वापस buildSWithin(withinp, entryVtxp, is_शीर्ष_Level_Step);
        }
        अगर (AstAbortOn* स्थिर ap = VN_CAST(nodep, AbortOn)) {
            वापस buildAbortOn(ap->condp(), ap->propp(), entryVtxp, ap->kind(), ap->fileline(),
                                is_शीर्ष_Level_Step);
        }
        अगर (VN_IS(nodep, SNonConsRep)) वापस Build_परिणाम::हार();
        अगर (AstImplication* स्थिर implp = VN_CAST(nodep, Implication)) {
            वापस buildImplicationEdges(implp->lhsp(), implp->rhsp(), entryVtxp,
                                         implp->isOverlapped(), implp->isFollowedBy(),
                                         implp->lhsp(), implp->fileline());
        }
        अगर (AstUntil* स्थिर untilp = VN_CAST(nodep, Until)) {
            वापस buildUntil(untilp, entryVtxp, is_शीर्ष_Level_Step);
        }
        // Boolean leaf (including LogAnd): return as finalCond
        वापस {entryVtxp, nodep, {}};
    }

    // Wire an implication / followed-by from `entryVtxp`: builds the antecedent,
    // emits the match-link (and for followed-by the reject-sink edge), inserts a
    // delay vertex for non-overlapped forms, and builds the body. Used both for
    // nested AstImplication in pexpr position and for the top-level assertion
    // antecedent -- `errorNodep` anchors the "unsupported sequence antecedent"
    // error, which differs between the two call sites.
    Build_परिणाम buildImplicationEdges(Ast_गाँठ_व्यंजक* antExprp, Ast_गाँठ_व्यंजक* bodyExprp,
                                      SvaStateVertex* entryVtxp, bool isOverlapped,
                                      bool isFollowedBy, Ast_गाँठ* errorNodep, File_पंक्ति* flp) {
        स्थिर Build_परिणाम ant_परिणाम = build_व्यंजक(antExprp, entryVtxp);
        अगर (!ant_परिणाम.valid()) वापस ant_परिणाम;

        // Followed-by requires pure-boolean antecedent for non-vacuous-fail at
        // the attempt-start cycle. IEEE 1800-2023 16.12.9 permits a multi-cycle
        // sequence LHS, so this is an implementation gap rather than illegal SV.
        अगर (isFollowedBy && ant_परिणाम.termVertexp != entryVtxp) {
            errorNodep->v3warn(E_UNSUPPORTED,
                               "Unsupported: sequence expression as antecedent of followed-by"
                               " (#-# / #=#) (IEEE 1800-2023 16.12.9)");
            वापस Build_परिणाम::failWithError();
        }
        UASSERT_OBJ(!isFollowedBy || ant_परिणाम.finalCondp, errorNodep,
                    "followed-by antecedent terminal at entry must carry finalCondp");

        // Use raw createStateVertex() so trigVtxp starts without liveness --
        // reaching the antecedent terminal is a definitive event.
        SvaStateVertex* स्थिर trigVtxp = m_graph.createStateVertex();
        अगर (ant_परिणाम.finalCondp) {
            m_graph.addLink(ant_परिणाम.termVertexp, trigVtxp,
                            sampled(ant_परिणाम.finalCondp->clone_वृक्ष_Pure(false)));
            // Followed-by non-vacuous fail: rejectOnFail fires when the attempt
            // is live (termVtx reachable) and sampled(antecedent) is false.
            अगर (isFollowedBy) {
                SvaStateVertex* स्थिर sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = true;
                SvaTransEdge* स्थिर ep
                    = m_graph.addLink(ant_परिणाम.termVertexp, sinkVtxp,
                                      sampled(ant_परिणाम.finalCondp->clone_वृक्ष_Pure(false)));
                ep->m_rejectOnFail = true;
            }
            // finalCondp is cloned into the Sampled nodes; if the original is
            // not parented anywhere in the AST anymore it must be freed here
            // or ASan flags it as a leak (e.g. t_sequence_bool_ops).
            अगर (!ant_परिणाम.finalCondp->backp()) {
                VL_DO_DANGLING(ant_परिणाम.finalCondp->delete_वृक्ष(), ant_परिणाम.finalCondp);
            }
        } अन्यथा {
            m_graph.addLink(ant_परिणाम.termVertexp, trigVtxp);
        }
        resetScope();

        SvaStateVertex* bodyEntryp = trigVtxp;
        अगर (!isOverlapped) {
            SvaStateVertex* स्थिर delayVtxp = m_graph.createStateVertex();
            m_graph.addClockedEdge(trigVtxp, delayVtxp);
            bodyEntryp = delayVtxp;
        }
        // An empty antecedent match must not start the consequent, but a zero-minimum
        // repetition's empty match is not handled here; keep the merge vertex for those
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = !isFollowedBy && !hasZeroMinRep(antExprp);
        वापस build_व्यंजक(bodyExprp, bodyEntryp, /*isTopLevelStep=*/true);
    }

    Build_परिणाम build(Ast_गाँठ_व्यंजक* exprp) {
        m_graph.m_startVertexp = scopedCreateVertex();
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = true;
        वापस build_व्यंजक(exprp, m_graph.m_startVertexp, /*isTopLevelStep=*/true);
    }
};

//######################################################################
// NFA Lowering (converts NFA graph to synthesizable AstAlways blocks)

श्रेणी SvaNfaLowering final {
    Ast_गाँठ_Module* स्थिर m_modp;  // Module to add state vars and always blocks to
    Ast_गाँठ_DType* स्थिर m_u32DTypep;  // Shared unsigned counter dtype
    V3UniqueNames m_names{"__Vnfa"};
    माप_प्रकार m_statDelayRingEdgeVisits = 0;  // Delay-ring incoming edges visited

    // Per-lowering shared context (passed to phase sub-functions)
    // Per-vertex lowering state is stored in SvaVertexData and accessed via
    // V3GraphVertex::userp() (see SvaStateVertex::datap()).
    काष्ठा LowerCtx final {
        File_पंक्ति* flp;  // Source location for generated AST
        Ast_Sen_वृक्ष* senTreep;  // Clock sensitivity tree
        Ast_गाँठ_व्यंजक* disableExprp;  // disable iff expression (may be nullptr)
        Ast_गाँठ_व्यंजक* matchCondp;  // Final boolean match condition (may be nullptr)
        VAssertType assertType;  // Assertion type for control tasks
        VAssertDirectiveType directiveType;  // Directive type for control tasks
        AstVar* killVarp;  // Last observed kill generation
        SvaGraph& graph;  // NFA graph
    };

    // Build a match-now expression: stateSig[i] && $sampled(condp)
    अटल Ast_गाँठ_व्यंजक* buildMatchNow(File_पंक्ति* flp, Ast_गाँठ_व्यंजक* stateExprp, Ast_गाँठ_व्यंजक* condp) {
        Ast_गाँठ_व्यंजक* स्थिर statep = stateExprp->clone_वृक्ष_Pure(false);
        अगर (!condp) वापस statep;
        वापस नया AstLogAnd{flp, statep, sampled(condp->clone_वृक्ष_Pure(false))};
    }
    अटल Ast_गाँठ_व्यंजक* andCond(File_पंक्ति* flp, Ast_गाँठ_व्यंजक* exprp, Ast_गाँठ_व्यंजक* condp) {
        अगर (!condp) वापस exprp;
        वापस नया AstLogAnd{flp, exprp, condp->clone_वृक्ष_Pure(false)};
    }
    // bp is always non-null; only ap can be null (serving as accumulator).
    अटल Ast_गाँठ_व्यंजक* orExprs(File_पंक्ति* flp, Ast_गाँठ_व्यंजक* ap, Ast_गाँठ_व्यंजक* bp) {
        अगर (!ap) वापस bp;
        वापस नया AstLogOr{flp, ap, bp};
    }
    अटल Ast_गाँठ_व्यंजक* addCountp(File_पंक्ति* स्थिर flp, Ast_गाँठ_व्यंजक* स्थिर योग_Countp,
                                  Ast_गाँठ_व्यंजक* स्थिर contributionp,
                                  Ast_गाँठ_व्यंजक* स्थिर enablep = nullptr) {
        // contribution = enable ? contribution : 0;
        Ast_गाँठ_व्यंजक* स्थिर enabledContributionp
            = enablep ? नया AstCond{flp, enablep, contributionp,
                                    newTypedConstp(flp, contributionp->dtypep(), 0)}
                      : contributionp;
        अगर (!योग_Countp) वापस enabledContributionp;
        वापस नया AstAdd{flp, योग_Countp, enabledContributionp};
    }
    अटल Ast_गाँठ_व्यंजक* gateCheckedCountp(File_पंक्ति* स्थिर flp, Ast_गाँठ_व्यंजक* स्थिर countp,
                                          Ast_गाँठ_व्यंजक*& overflowp, Ast_गाँठ_व्यंजक* स्थिर enablep) {
        अगर (overflowp) {
            overflowp = नया AstLogAnd{flp, enablep->clone_वृक्ष_Pure(false), overflowp};
        }
        वापस addCountp(flp, nullptr, countp, enablep);
    }
    अटल Ast_गाँठ_व्यंजक* addCheckedCountp(File_पंक्ति* स्थिर flp, Ast_गाँठ_व्यंजक* स्थिर योग_Countp,
                                         Ast_गाँठ_व्यंजक* स्थिर contributionp, Ast_गाँठ_व्यंजक*& overflowp,
                                         Ast_गाँठ_व्यंजक* स्थिर contributionOverflowp = nullptr) {
        अगर (contributionOverflowp) { overflowp = orExprs(flp, overflowp, contributionOverflowp); }
        अगर (!योग_Countp) वापस contributionp;
        Ast_गाँठ_व्यंजक* स्थिर oldTotalp = योग_Countp->clone_वृक्ष_Pure(false);
        Ast_गाँठ_व्यंजक* स्थिर resultp = नया AstAdd{flp, योग_Countp, contributionp};
        overflowp
            = orExprs(flp, overflowp, नया AstLt{flp, resultp->clone_वृक्ष_Pure(false), oldTotalp});
        वापस resultp;
    }
    AstIf* new_गिनती_Overflow_Warning(File_पंक्ति* स्थिर flp, Ast_गाँठ_व्यंजक* स्थिर condp) स्थिर {
        AstDisplay* स्थिर dispp = नया AstDisplay{
            flp, VDisplayType::DT_WARNING,
            "Cover sequence match count overflowed 32-bit storage; results may be incorrect.",
            nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstIf* स्थिर ifp = नया AstIf{flp, condp, dispp};
        ifp->branchPred(VBranchPred::BP_UNLIKELY);
        वापस ifp;
    }
    अटल Ast_गाँठ_व्यंजक* killActive(LowerCtx& c) {
        वापस नया AstNeq{c.flp, नया AstVarRef{c.flp, c.killVarp, VAccess::READ},
                          assertKillGet(c.flp, c.assertType, c.directiveType)};
    }
    अटल Ast_गाँठ_व्यंजक* notKillActive(LowerCtx& c) { वापस नया AstLogNot{c.flp, killActive(c)}; }
    अटल Ast_गाँठ_व्यंजक* gateNotKill(LowerCtx& c, Ast_गाँठ_व्यंजक* exprp) {
        अगर (!exprp) वापस nullptr;
        वापस नया AstLogAnd{c.flp, exprp, notKillActive(c)};
    }
    अटल Ast_गाँठ_व्यंजक* अग्र_Ring_सूचकांक(File_पंक्ति* flp, Ast_गाँठ_व्यंजक* idxExprp, uint32_t आकार) {
        स्थिर स्वतः u32Const = [flp](uint32_t मूल्य) {
            वापस नया AstConst{flp, AstConst::Widthed_मूल्य{}, 32, मूल्य};
        };
        UASSERT_OBJ(आकार > 0, idxExprp, "Ring size must be positive");
        अगर (आकार == 1) {
            VL_DO_DANGLING(idxExprp->delete_वृक्ष(), idxExprp);
            वापस u32Const(0);
        }
        // idx == size - 1 ? 0 : idx + 1
        AstAdd* स्थिर addp = नया AstAdd{flp, idxExprp->clone_वृक्ष_Pure(false), u32Const(1)};
        addp->dtypeFrom(idxExprp);
        AstCond* स्थिर condp
            = नया AstCond{flp, नया AstEq{flp, idxExprp, u32Const(आकार - 1)}, u32Const(0), addp};
        condp->dtypeFrom(idxExprp);
        वापस condp;
    }
    अटल Ast_गाँठ_व्यंजक* delayRingSlot(File_पंक्ति* flp, SvaStateVertex* vtxp, Ast_गाँठ_व्यंजक* idxExprp,
                                      VAccess access = VAccess::READ) {
        // ring[idx]
        AstVar* स्थिर ringp = vtxp->datap()->delayRingVarp;
        AstVarRef* स्थिर refp = नया AstVarRef{flp, ringp, access};
        अगर (vtxp->datap()->delayRingCounts) वापस नया AstArraySel{flp, refp, idxExprp};
        वापस नया AstSel{flp, refp, idxExprp, 1};
    }
    अटल Ast_गाँठ_व्यंजक* delay_Ring_शीर्ष_गिनती(File_पंक्ति* flp, SvaStateVertex* vtxp) {
        Ast_गाँठ_व्यंजक* स्थिर headp
            = नया AstVarRef{flp, vtxp->datap()->delay_Ring_शीर्ष_Varp, VAccess::READ};
        अगर (vtxp->datap()->delayRingCounts) वापस headp;
        वापस नया AstExtend{flp, headp, vtxp->datap()->delay_Ring_Live_गिनती_Varp->dtypep()->width()};
    }
    अटल Ast_गाँठ_व्यंजक* delay_Ring_At_Last_सूचकांक(File_पंक्ति* स्थिर flp, AstVar* स्थिर idxp,
                                             स्थिर uint32_t आकार) {
        वापस नया AstEq{flp, नया AstVarRef{flp, idxp, VAccess::READ},
                         नया AstConst{flp, AstConst::Widthed_मूल्य{}, 32, आकार - 1}};
    }
    अटल Ast_गाँठ_व्यंजक* delay_Ring_Output_गिनती(File_पंक्ति* स्थिर flp, SvaStateVertex* स्थिर vtxp) {
        AstVar* स्थिर idxp = vtxp->datap()->delayRingIdxVarp;
        स्थिर uint32_t आकार = vtxp->m_delay_Ring_आकार;
        Ast_गाँठ_व्यंजक* outgoingValidp
            = नया AstVarRef{flp, vtxp->datap()->delayRingWrappedVarp, VAccess::READ};
        अगर (!vtxp->m_isFixedDelayRing) {
            outgoingValidp
                = नया AstLogOr{flp, outgoingValidp, delay_Ring_At_Last_सूचकांक(flp, idxp, आकार)};
        }
        AstVar* स्थिर live_गिनती_Varp = vtxp->datap()->delay_Ring_Live_गिनती_Varp;
        वापस नया AstCond{flp, outgoingValidp, delay_Ring_शीर्ष_गिनती(flp, vtxp),
                           newTypedConstp(flp, live_गिनती_Varp->dtypep(), 0)};
    }
    अटल Ast_गाँठ_व्यंजक* delayRingHasLiveThreadsp(File_पंक्ति* स्थिर flp,
                                                 AstVar* स्थिर live_गिनती_Varp) {
        // active = live_count != 0;
        वापस नया AstNeq{flp, नया AstVarRef{flp, live_गिनती_Varp, VAccess::READ},
                          newTypedConstp(flp, live_गिनती_Varp->dtypep(), 0)};
    }

    // Phase 3 output signals
    काष्ठा SignalSet final {
        Ast_गाँठ_व्यंजक* terminalActivep = nullptr;  // OR of all successful terminal matches
        Ast_गाँठ_व्यंजक* matchCountp = nullptr;  // NFA paths completing the sequence this tick
        Ast_गाँठ_व्यंजक* match_गिनती_Overflowp = nullptr;  // Match count overflow this tick
        Ast_गाँठ_व्यंजक* rejectBasep = nullptr;  // Reject when a terminal match fails
        Ast_गाँठ_व्यंजक* requiredStepRejectp = nullptr;  // Per-source reject from rejectOnFail Links
        Ast_गाँठ_व्यंजक* throughoutRejectp = nullptr;  // Reject when a throughout guard drops
        Ast_गाँठ_व्यंजक* threadFailCountp = nullptr;  // Number of threads rejected on this tick
    };

    // Disable condition also aborts attempts between clock events. A constant disable
    // never rises, and $sampled is unsupported in a sensitivity list.
    अटल bool isAsyncDisable(स्थिर Ast_गाँठ_व्यंजक* स्थिर disableExprp) {
        वापस disableExprp && !VN_IS(disableExprp, Const)
               && !disableExprp->exists([](स्थिर AstSampled*) { वापस true; });
    }

    // Emit the clocked update block of registers that hold pending attempts. The clearp
    // statements reset the same registers: a disable rising between clock events also
    // aborts pending attempts (IEEE 1800-2023 16.12), so clearp runs on that rising edge
    // in a separate block. It must clear unconditionally, as the disable may already be
    // false again when it runs, and the update block must not run on that edge, as it
    // would advance the attempts as if the clock had ticked. Blocks that hold no attempt
    // state, such as the $assertkill acknowledge in emitKillAckNba, do not go through here.
    // Callers pass a null clearp when the disable cannot rise between clock events.
    व्योम addStateAlways(LowerCtx& c, Ast_गाँठ* bodyp, Ast_गाँठ* clearp) {
        m_modp->addStmtsp(
            नया AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_वृक्ष(false), bodyp});
        अगर (!clearp) वापस;
        m_modp->addStmtsp(नया AstAlways{
            c.flp, VAlwaysKwd::ALWAYS,
            नया Ast_Sen_वृक्ष{c.flp, नया AstSenItem{c.flp, VEdgeType::ET_POSEDGE,
                                                 c.disableExprp->clone_वृक्ष_Pure(false)}},
            clearp});
    }

    // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
    // Phase 2: State register NBA always block. Each clocked-edge target
    // latches the OR of its incoming contributions.
    व्योम emitStateRegisterNba(LowerCtx& c) {
        स्थिर bool asyncDisable = isAsyncDisable(c.disableExprp);
        Ast_गाँठ* bodyp = nullptr;
        Ast_गाँठ* clearp = nullptr;
        क्रम (स्थिर V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            स्थिर SvaStateVertex* स्थिर vtxp = static_cast<स्थिर SvaStateVertex*>(&vtxr);
            अगर (!vtxp->datap()->stateVarp) जारी;

            Ast_गाँठ_व्यंजक* अग्र_Statep = nullptr;
            क्रम (स्थिर V3GraphEdge& edger : vtxp->inEdges()) {
                स्थिर SvaTransEdge& tedger = static_cast<स्थिर SvaTransEdge&>(edger);
                अगर (!tedger.m_consumesCycle) जारी;
                स्थिर SvaStateVertex* स्थिर fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Clocked-edge source missing stateSig");

                Ast_गाँठ_व्यंजक* srcSigp = fromVtxp->datap()->stateSigp->clone_वृक्ष_Pure(false);
                srcSigp = andCond(c.flp, srcSigp, tedger.m_condp);

                अगर (c.disableExprp) {
                    Ast_गाँठ_व्यंजक* स्थिर notDisp
                        = नया AstLogNot{c.flp, c.disableExprp->clone_वृक्ष_Pure(false)};
                    srcSigp = नया AstLogAnd{c.flp, srcSigp, notDisp};
                }
                अग्र_Statep = orExprs(c.flp, अग्र_Statep, srcSigp);
            }

            UASSERT_OBJ(अग्र_Statep, vtxp,
                        "Registered vertex has no clocked incoming contribution");
            अग्र_Statep = gateNotKill(c, अग्र_Statep);

            AstAssignDly* स्थिर assignp = नया AstAssignDly{
                c.flp, नया AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE}, अग्र_Statep};
            bodyp = Ast_गाँठ::add_अग्र_Null(bodyp, assignp);
            अगर (asyncDisable) {
                clearp = Ast_गाँठ::add_अग्र_Null(
                    clearp,
                    नया AstAssignDly{
                        c.flp, नया AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE},
                        नया AstConst{c.flp, AstConst::BitFalse{}}});
            }
        }
        अगर (bodyp) addStateAlways(c, bodyp, clearp);
    }

    // Phase 2b: In-place ring storage and registered ring metadata.
    व्योम emitDelayRingUpdates(LowerCtx& c, स्थिर bool गिनती_Matches) {
        स्थिर bool asyncDisable = isAsyncDisable(c.disableExprp);
        क्रम (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* स्थिर vtxp = static_cast<SvaStateVertex*>(&vtxr);
            अगर (!vtxp->datap()->delayRingVarp) जारी;
            AstVar* स्थिर idxp = vtxp->datap()->delayRingIdxVarp;
            AstVar* स्थिर live_गिनती_Varp = vtxp->datap()->delay_Ring_Live_गिनती_Varp;
            AstVar* स्थिर wrappedp = vtxp->datap()->delayRingWrappedVarp;
            स्थिर uint32_t आकार = static_cast<uint32_t>(vtxp->m_delay_Ring_आकार);

            Ast_गाँठ_व्यंजक* incomingp = nullptr;
            Ast_गाँठ_व्यंजक* incomingOverflowp = nullptr;
            क्रम (स्थिर V3GraphEdge& edger : vtxp->inEdges()) {
                ++m_statDelayRingEdgeVisits;
                स्थिर SvaTransEdge& tedger = static_cast<स्थिर SvaTransEdge&>(edger);
                UASSERT_OBJ(tedger.m_consumesCycle == vtxp->m_isFixedDelayRing, vtxp,
                            "Delay-ring incoming edge kind mismatch");
                स्थिर SvaStateVertex* स्थिर fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Delay-ring incoming source missing stateSig");
                UASSERT_OBJ(!गिनती_Matches || fromVtxp->datap()->state_गिनती_Sigp, fromVtxp,
                            "Delay-ring incoming source missing stateCountSig");
                // IEEE 1800-2023 16.14.1/16.14.3: assertion alternatives belong to one
                // evaluation; only cover sequence counts each end-of-match.
                Ast_गाँठ_व्यंजक* contribp
                    = गिनती_Matches
                          ? fromVtxp->datap()->state_गिनती_Sigp->clone_वृक्ष_Pure(false)
                          : नया AstExtend{c.flp,
                                          fromVtxp->datap()->stateSigp->clone_वृक्ष_Pure(false),
                                          m_u32DTypep->width()};
                Ast_गाँठ_व्यंजक* contribOverflowp
                    = गिनती_Matches && fromVtxp->datap()->state_गिनती_Overflow_Sigp
                          ? fromVtxp->datap()->state_गिनती_Overflow_Sigp->clone_वृक्ष_Pure(false)
                          : nullptr;
                अगर (tedger.m_condp) {
                    Ast_गाँठ_व्यंजक* स्थिर enablep = tedger.m_condp->clone_वृक्ष_Pure(false);
                    contribp = गिनती_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, enablep)
                                   : addCountp(c.flp, nullptr, contribp, enablep);
                }
                अगर (c.disableExprp) {
                    Ast_गाँठ_व्यंजक* स्थिर notDisp
                        = नया AstLogNot{c.flp, c.disableExprp->clone_वृक्ष_Pure(false)};
                    contribp = गिनती_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, notDisp)
                                   : addCountp(c.flp, nullptr, contribp, notDisp);
                }
                incomingp = गिनती_Matches ? addCheckedCountp(c.flp, incomingp, contribp,
                                                            incomingOverflowp, contribOverflowp)
                                         : addCountp(c.flp, incomingp, contribp);
            }
            UASSERT_OBJ(incomingp, vtxp, "Delay ring has no incoming edge");
            स्थिर bool storesCounts = vtxp->datap()->delayRingCounts;
            Ast_गाँठ_व्यंजक* स्थिर incomingCountp
                = storesCounts
                      ? incomingp->clone_वृक्ष_Pure(false)
                      : नया AstExtend{c.flp,
                                      नया AstNeq{c.flp, incomingp->clone_वृक्ष_Pure(false),
                                                 newTypedConstp(c.flp, incomingp->dtypep(), 0)},
                                      m_u32DTypep->width()};
            Ast_गाँठ_व्यंजक* स्थिर storedIncomingp
                = storesCounts ? incomingp
                               : नया AstNeq{c.flp, incomingp,
                                            newTypedConstp(c.flp, incomingp->dtypep(), 0)};
            // Only this block accesses the storage. Other processes read the cached head, so
            // the ring can be updated in place without an NBA shadow copy.
            AstAssign* स्थिर writeIncomingp = नया AstAssign{
                c.flp,
                delayRingSlot(c.flp, vtxp, नया AstVarRef{c.flp, idxp, VAccess::READ},
                              VAccess::WRITE),
                storedIncomingp};
            Ast_गाँठ* updateBodyp = writeIncomingp;
            // live_count <= live_count - outgoing_count + incoming_count;
            Ast_गाँठ_व्यंजक* स्थिर outgoingp = delay_Ring_Output_गिनती(c.flp, vtxp);
            Ast_गाँठ_व्यंजक* स्थिर afterOutgoingp
                = नया AstSub{c.flp, नया AstVarRef{c.flp, live_गिनती_Varp, VAccess::READ}, outgoingp};
            Ast_गाँठ_व्यंजक* स्थिर oldAfterOutgoingp = afterOutgoingp->clone_वृक्ष_Pure(false);
            AstAdd* स्थिर अग्र_Live_Countp = नया AstAdd{c.flp, afterOutgoingp, incomingCountp};
            अगर (storesCounts) {
                Ast_गाँठ_व्यंजक* स्थिर liveOverflowp
                    = नया AstLt{c.flp, अग्र_Live_Countp->clone_वृक्ष_Pure(false), oldAfterOutgoingp};
                Ast_गाँठ_व्यंजक* स्थिर overflowp = orExprs(c.flp, incomingOverflowp, liveOverflowp);
                updateBodyp = new_गिनती_Overflow_Warning(c.flp, overflowp);
                updateBodyp->add_अग्र(writeIncomingp);
            } अन्यथा {
                VL_DO_DANGLING(oldAfterOutgoingp->delete_वृक्ष(), oldAfterOutgoingp);
            }
            // Cache the outgoing slot for the next index after the write. Reading after
            // writing also handles one-slot rings and two-slot range rings without alias cases.
            Ast_गाँठ_व्यंजक* अग्र_शीर्ष_Idxp
                = अग्र_Ring_सूचकांक(c.flp, नया AstVarRef{c.flp, idxp, VAccess::READ}, आकार);
            अगर (!vtxp->m_isFixedDelayRing) {
                अग्र_शीर्ष_Idxp = अग्र_Ring_सूचकांक(c.flp, अग्र_शीर्ष_Idxp, आकार);
            }
            updateBodyp->add_अग्र(नया AstAssignDly{
                c.flp, नया AstVarRef{c.flp, vtxp->datap()->delay_Ring_शीर्ष_Varp, VAccess::WRITE},
                delayRingSlot(c.flp, vtxp, अग्र_शीर्ष_Idxp)});
            updateBodyp->add_अग्र(नया AstAssignDly{
                c.flp, नया AstVarRef{c.flp, live_गिनती_Varp, VAccess::WRITE}, अग्र_Live_Countp});
            // wrapped <= wrapped || idx == size - 1;
            updateBodyp->add_अग्र(
                नया AstAssignDly{c.flp, नया AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                 नया AstLogOr{c.flp, नया AstVarRef{c.flp, wrappedp, VAccess::READ},
                                              delay_Ring_At_Last_सूचकांक(c.flp, idxp, आकार)}});
            // idx <= next_idx;
            updateBodyp->add_अग्र(नया AstAssignDly{
                c.flp, नया AstVarRef{c.flp, idxp, VAccess::WRITE},
                अग्र_Ring_सूचकांक(c.flp, नया AstVarRef{c.flp, idxp, VAccess::READ}, आकार)});
            अगर (vtxp->m_delayRingAdvanceCondp) {
                updateBodyp = नया AstIf{
                    c.flp, sampled(vtxp->m_delayRingAdvanceCondp->clone_वृक्ष_Pure(false)),
                    updateBodyp};
            }

            Ast_गाँठ_व्यंजक* clearCondp = killActive(c);
            अगर (vtxp->m_delayRingClearCondp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_delayRingClearCondp->clone_वृक्ष_Pure(false)));
            }
            अगर (vtxp->m_abortClearp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_abortClearp->clone_वृक्ष_Pure(false)));
            }
            अगर (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_वृक्ष_Pure(false));
            }
            Ast_गाँठ_व्यंजक* guardp = nullptr;
            क्रम (Ast_गाँठ_व्यंजक* स्थिर cp : vtxp->m_throughoutConds) {
                Ast_गाँठ_व्यंजक* स्थिर sampledp = sampled(cp->clone_वृक्ष_Pure(false));
                guardp = guardp ? static_cast<Ast_गाँठ_व्यंजक*>(नया AstLogAnd{c.flp, guardp, sampledp})
                                : sampledp;
            }
            अगर (guardp) clearCondp = orExprs(c.flp, clearCondp, नया AstLogNot{c.flp, guardp});
            // Logically clear the ring without touching its wide storage.
            AstAssignDly* स्थिर clearCountp
                = नया AstAssignDly{c.flp, नया AstVarRef{c.flp, live_गिनती_Varp, VAccess::WRITE},
                                   newTypedConstp(c.flp, live_गिनती_Varp->dtypep(), 0)};
            clearCountp->add_अग्र(नया AstAssignDly{c.flp,
                                                  नया AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                                  नया AstConst{c.flp, AstConst::BitFalse{}}});
            clearCountp->add_अग्र(नया AstAssignDly{c.flp,
                                                  नया AstVarRef{c.flp, idxp, VAccess::WRITE},
                                                  newTypedConstp(c.flp, idxp->dtypep(), 0)});
            updateBodyp = नया AstIf{c.flp, clearCondp, clearCountp, updateBodyp};

            addStateAlways(c, updateBodyp, asyncDisable ? clearCountp->clone_वृक्ष(true) : nullptr);
        }
    }

    // Phase 2c: SAnd combiner done-latch always block.
    // NBA semantics ensure doneL/doneR read pre-update values (IEEE 16.9.5).
    व्योम emitAndCombinerDoneLatchNba(LowerCtx& c) {
        स्थिर bool asyncDisable = isAsyncDisable(c.disableExprp);
        क्रम (स्थिर V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            स्थिर SvaStateVertex* स्थिर avp = static_cast<स्थिर SvaStateVertex*>(&vtxr);
            अगर (!avp->datap()->doneLVarp) जारी;
            // doneLVars is non-null only for AndCombiner vertices, which always
            // have both m_andLhsTermp and m_andRhsTermp set at build time.
            UASSERT_OBJ(avp->m_andLhsTermp && avp->m_andRhsTermp, avp,
                        "AndCombiner vertex missing LHS/RHS terminal");
            स्थिर SvaStateVertex* स्थिर lhsTermp = avp->m_andLhsTermp;
            स्थिर SvaStateVertex* स्थिर rhsTermp = avp->m_andRhsTermp;
            // resolveLinks' 2*N+2 fixed-point pass is guaranteed to populate
            // stateSigp on every AndCombiner and its LHS/RHS terminals.
            UASSERT_OBJ(lhsTermp->datap()->stateSigp && rhsTermp->datap()->stateSigp
                            && avp->datap()->stateSigp,
                        avp, "AndCombiner stateSigp chain unresolved after resolveLinks");

            AstAssignDly* स्थिर clearLp = नया AstAssignDly{
                c.flp, नया AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                नया AstConst{c.flp, AstConst::BitFalse{}}};
            AstAssignDly* स्थिर clearRp = नया AstAssignDly{
                c.flp, नया AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                नया AstConst{c.flp, AstConst::BitFalse{}}};
            clearLp->add_अग्र(clearRp);

            Ast_गाँठ_व्यंजक* स्थिर matchLNowp
                = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, avp->m_andLhsCondp);
            Ast_गाँठ_व्यंजक* स्थिर matchRNowp
                = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, avp->m_andRhsCondp);
            Ast_गाँठ_व्यंजक* gateLp = matchLNowp;
            Ast_गाँठ_व्यंजक* gateRp = matchRNowp;
            अगर (c.disableExprp) {
                Ast_गाँठ_व्यंजक* स्थिर notDisLp
                    = नया AstLogNot{c.flp, c.disableExprp->clone_वृक्ष_Pure(false)};
                gateLp = नया AstLogAnd{c.flp, gateLp, notDisLp};
                Ast_गाँठ_व्यंजक* स्थिर notDisRp
                    = नया AstLogNot{c.flp, c.disableExprp->clone_वृक्ष_Pure(false)};
                gateRp = नया AstLogAnd{c.flp, gateRp, notDisRp};
            }
            AstAssignDly* स्थिर setLp = नया AstAssignDly{
                c.flp, नया AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                नया AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* स्थिर setLIfp = नया AstIf{c.flp, gateLp, setLp, nullptr};
            AstAssignDly* स्थिर setRp = नया AstAssignDly{
                c.flp, नया AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                नया AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* स्थिर setRIfp = नया AstIf{c.flp, gateRp, setRp, nullptr};
            setLIfp->add_अग्र(setRIfp);

            Ast_गाँठ_व्यंजक* clearCondp = नया AstLogOr{c.flp, killActive(c),
                                                   avp->datap()->stateSigp->clone_वृक्ष_Pure(false)};
            अगर (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_वृक्ष_Pure(false));
            }
            AstIf* स्थिर topp = नया AstIf{c.flp, clearCondp, clearLp, setLIfp};
            addStateAlways(c, topp, asyncDisable ? clearLp->clone_वृक्ष(true) : nullptr);
        }
    }

    व्योम emitKillAckNba(LowerCtx& c) {
        AstAssignDly* स्थिर ackp
            = नया AstAssignDly{c.flp, नया AstVarRef{c.flp, c.killVarp, VAccess::WRITE},
                               assertKillGet(c.flp, c.assertType, c.directiveType)};
        m_modp->addStmtsp(
            नया AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_वृक्ष(false), ackp});
    }

    // Phase 3/3a/3b: Compute terminal match/reject signals, required-step reject,
    // throughout-drop reject; clean up intermediate state signals.
    // Phase 3: terminalActive and rejectBase from Links to matchVertex.
    // Builder only adds Links (non-clocked) to matchVertex via addLink in
    // wireMatchAndMidSources. For cover sequence, also count each end-of-match
    // so the action can be replayed without unrolling endpoints.
    व्योम computeTerminalMatchAndReject(LowerCtx& c, SignalSet& sigs, स्थिर bool need_Match_गिनती) {
        क्रम (स्थिर SvaTransEdge* स्थिर tedgep : c.graph.allEdges()) {
            अगर (tedgep->toVtxp() != c.graph.m_matchVertexp) जारी;
            SvaStateVertex* स्थिर fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                        "Terminal-link source missing stateSig");

            Ast_गाँठ_व्यंजक* srcSigp = fromVtxp->datap()->stateSigp->clone_वृक्ष_Pure(false);
            srcSigp = andCond(c.flp, srcSigp, tedgep->m_condp);
            अगर (need_Match_गिनती) {
                UASSERT_OBJ(fromVtxp->datap()->state_गिनती_Sigp, fromVtxp,
                            "Terminal-link source missing stateCountSig");
                Ast_गाँठ_व्यंजक* contributionp
                    = fromVtxp->datap()->state_गिनती_Sigp->clone_वृक्ष_Pure(false);
                Ast_गाँठ_व्यंजक* contributionOverflowp
                    = fromVtxp->datap()->state_गिनती_Overflow_Sigp
                          ? fromVtxp->datap()->state_गिनती_Overflow_Sigp->clone_वृक्ष_Pure(false)
                          : nullptr;
                अगर (tedgep->m_condp) {
                    contributionp = gateCheckedCountp(c.flp, contributionp, contributionOverflowp,
                                                      tedgep->m_condp->clone_वृक्ष_Pure(false));
                }
                sigs.matchCountp
                    = addCheckedCountp(c.flp, sigs.matchCountp, contributionp,
                                       sigs.match_गिनती_Overflowp, contributionOverflowp);
            }

            अगर (fromVtxp->m_delay_Ring_आकार && !fromVtxp->m_isFixedDelayRing) {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_वृक्ष_Pure(false));
                // reject |= (wrapped || idx == size - 1) && head && final_condition;
                Ast_गाँठ_व्यंजक* expireContribp = delay_Ring_Output_गिनती(c.flp, fromVtxp);
                expireContribp = andCond(c.flp, expireContribp, tedgep->m_condp);
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, expireContribp);
                VL_DO_DANGLING(srcSigp->delete_वृक्ष(), srcSigp);
            } अन्यथा अगर (fromVtxp->m_isUnbounded || fromVtxp->m_isAndCombiner) {
                sigs.terminalActivep = orExprs(c.flp, sigs.terminalActivep, srcSigp);
            } अन्यथा {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_वृक्ष_Pure(false));
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, srcSigp);
            }
        }
        // wireMatchAndMidSources always adds a Link from result.termVertexp
        // to m_matchVertexp, so the loop above always sets terminalActivep.
        UASSERT_OBJ(sigs.terminalActivep, c.graph.m_matchVertexp,
                    "No terminal edge to match vertex");
    }

    Ast_गाँठ_व्यंजक* newThroughoutThreadFailCountp(LowerCtx& c, AstVar* स्थिर delay_Ring_Live_गिनती_Varp,
                                               Ast_गाँठ_व्यंजक* स्थिर stateExprp,
                                               Ast_गाँठ_व्यंजक* स्थिर enablep) {
        Ast_गाँठ_व्यंजक* स्थिर activeThreadCountp
            = delay_Ring_Live_गिनती_Varp
                  ? static_cast<Ast_गाँठ_व्यंजक*>(
                        नया AstVarRef{c.flp, delay_Ring_Live_गिनती_Varp, VAccess::READ})
                  : नया AstExtend{c.flp, stateExprp->clone_वृक्ष_Pure(false), m_u32DTypep->width()};
        वापस addCountp(c.flp, nullptr, activeThreadCountp, enablep);
    }

    // Phase 3b: Throughout-drop and ring-wide abort rejection.
    व्योम computeThroughoutReject(LowerCtx& c, SignalSet& sigs, स्थिर bool need_Thread_Fail_गिनती) {
        क्रम (स्थिर V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            स्थिर SvaStateVertex* स्थिर vtxp = static_cast<स्थिर SvaStateVertex*>(&vtxr);
            स्थिर स्वतः& conds = vtxp->m_throughoutConds;
            अगर (conds.empty() && !vtxp->m_abortRejectp) जारी;
            अगर (vtxp->m_isAndCombiner) जारी;
            Ast_गाँठ_व्यंजक* stateExprp = nullptr;
            अगर (vtxp->datap()->stateVarp) {
                stateExprp = नया AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
            } अन्यथा अगर (vtxp->datap()->delayRingVarp && vtxp->m_isFixedDelayRing) {
                stateExprp
                    = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_गिनती_Varp);
            } अन्यथा {
                UASSERT_OBJ(vtxp->datap()->stateSigp, vtxp,
                            "Throughout-conds vertex missing state representation");
                stateExprp = vtxp->datap()->stateSigp->clone_वृक्ष_Pure(false);
            }
            Ast_गाँठ_व्यंजक* guardp = nullptr;
            क्रम (Ast_गाँठ_व्यंजक* स्थिर cp : conds) {
                Ast_गाँठ_व्यंजक* स्थिर sp = sampled(cp->clone_वृक्ष_Pure(false));
                guardp = guardp ? static_cast<Ast_गाँठ_व्यंजक*>(नया AstLogAnd{c.flp, guardp, sp}) : sp;
            }
            अगर (vtxp->m_abortRejectp) {
                Ast_गाँठ_व्यंजक* स्थिर notAbortp
                    = नया AstLogNot{c.flp, sampled(vtxp->m_abortRejectp->clone_वृक्ष_Pure(false))};
                guardp = guardp
                             ? static_cast<Ast_गाँठ_व्यंजक*>(नया AstLogAnd{c.flp, guardp, notAbortp})
                             : notAbortp;
            }
            Ast_गाँठ_व्यंजक* rejectCondp = नया AstLogNot{c.flp, guardp};
            अगर (vtxp->m_abortClearp) {
                // Any abort clears the ring. Accept aborts suppress a simultaneous guard failure;
                // reject aborts are restored below so they still force rejection.
                Ast_गाँठ_व्यंजक* स्थिर notAbortClearp
                    = नया AstLogNot{c.flp, sampled(vtxp->m_abortClearp->clone_वृक्ष_Pure(false))};
                rejectCondp = नया AstLogAnd{c.flp, rejectCondp, notAbortClearp};
                अगर (vtxp->m_abortRejectp) {
                    rejectCondp = नया AstLogOr{
                        c.flp, rejectCondp, sampled(vtxp->m_abortRejectp->clone_वृक्ष_Pure(false))};
                }
            }
            अगर (need_Thread_Fail_गिनती) {
                Ast_गाँठ_व्यंजक* स्थिर contributionp
                    = newThroughoutThreadFailCountp(c, vtxp->datap()->delay_Ring_Live_गिनती_Varp,
                                                    stateExprp, rejectCondp->clone_वृक्ष_Pure(false));
                sigs.threadFailCountp = addCountp(c.flp, sigs.threadFailCountp, contributionp);
                अगर (vtxp->m_abortRejectp && vtxp->m_delayRingAdvanceCondp) {
                    // An advancing ring thread also occupies its same-tick match vertex in the
                    // unrolled NFA, so abort rejection must replay both fail actions.
                    Ast_गाँठ_व्यंजक* स्थिर abortAndAdvancep = नया AstLogAnd{
                        c.flp, sampled(vtxp->m_abortRejectp->clone_वृक्ष_Pure(false)),
                        sampled(vtxp->m_delayRingAdvanceCondp->clone_वृक्ष_Pure(false))};
                    Ast_गाँठ_व्यंजक* स्थिर matchContributionp = newThroughoutThreadFailCountp(
                        c, vtxp->datap()->delay_Ring_Live_गिनती_Varp, stateExprp, abortAndAdvancep);
                    sigs.threadFailCountp
                        = addCountp(c.flp, sigs.threadFailCountp, matchContributionp);
                }
            }
            sigs.throughoutRejectp = orExprs(c.flp, sigs.throughoutRejectp,
                                             नया AstLogAnd{c.flp, stateExprp, rejectCondp});
        }
    }

    SignalSet computeSignals(LowerCtx& c, स्थिर bool need_Thread_Fail_गिनती,
                             स्थिर bool need_Throughout_Thread_Fail_गिनती, स्थिर bool need_Match_गिनती) {
        SignalSet sigs;

        computeTerminalMatchAndReject(c, sigs, need_Match_गिनती);

        // Phase 3a: required-step rejection.
        // Builder only sets m_rejectOnFail on non-clocked Links with m_condp
        // or m_condVtxp, and the source always has a resolved stateSig.
        क्रम (स्थिर SvaTransEdge* स्थिर tedgep : c.graph.allEdges()) {
            अगर (!tedgep->m_rejectOnFail) जारी;
            स्थिर SvaStateVertex* स्थिर fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp && (tedgep->m_condp || tedgep->m_condVtxp),
                        fromVtxp,
                        "rejectOnFail Link must have condp/condVtxp and source stateSig");
            Ast_गाँठ_व्यंजक* स्थिर srcSigp = fromVtxp->datap()->stateSigp->clone_वृक्ष_Pure(false);
            Ast_गाँठ_व्यंजक* condp = nullptr;
            अगर (tedgep->m_condVtxp) {
                UASSERT_OBJ(tedgep->m_condVtxp->datap()->stateSigp, tedgep->m_condVtxp,
                            "rejectOnFail condVtxp missing stateSig");
                condp = tedgep->m_condVtxp->datap()->stateSigp->clone_वृक्ष_Pure(false);
                अगर (tedgep->m_condp) {
                    condp = नया AstLogOr{c.flp, condp, tedgep->m_condp->clone_वृक्ष_Pure(false)};
                }
            } अन्यथा {
                condp = tedgep->m_condp->clone_वृक्ष_Pure(false);
            }
            Ast_गाँठ_व्यंजक* स्थिर notCondp = नया AstLogNot{c.flp, condp};
            Ast_गाँठ_व्यंजक* स्थिर rawFailp = नया AstLogAnd{c.flp, srcSigp, notCondp};
            अगर (need_Thread_Fail_गिनती) {
                // thread_fail_count += fail;
                sigs.threadFailCountp = addCountp(
                    c.flp, sigs.threadFailCountp,
                    नया AstExtend{c.flp, rawFailp->clone_वृक्ष_Pure(false), m_u32DTypep->width()});
            }
            Ast_गाँठ_व्यंजक* स्थिर failp = gateNotKill(c, rawFailp);
            sigs.requiredStepRejectp = orExprs(c.flp, sigs.requiredStepRejectp, failp);
        }

        computeThroughoutReject(c, sigs, need_Throughout_Thread_Fail_गिनती);
        अगर (sigs.threadFailCountp) {
            sigs.threadFailCountp
                = addCountp(c.flp, nullptr, sigs.threadFailCountp, notKillActive(c));
        }
        अगर (sigs.matchCountp) {
            अगर (c.matchCondp) {
                sigs.matchCountp
                    = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_गिनती_Overflowp,
                                        sampled(c.matchCondp->clone_वृक्ष_Pure(false)));
            }
            sigs.matchCountp = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_गिनती_Overflowp,
                                                 notKillActive(c));
        }
        sigs.terminalActivep = gateNotKill(c, sigs.terminalActivep);
        sigs.rejectBasep = gateNotKill(c, sigs.rejectBasep);
        sigs.throughoutRejectp = gateNotKill(c, sigs.throughoutRejectp);

        // Clean up intermediate state signals. These are orphan subtrees
        // (never linked into the enclosing AST); deleteTree() is immediate
        // which is what we want since stateSigp lifetime ends with this scope.
        क्रम (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* स्थिर vtxp = static_cast<SvaStateVertex*>(&vtxr);
            Ast_गाँठ_व्यंजक*& sigp = vtxp->datap()->stateSigp;
            अगर (sigp) VL_DO_DANGLING(sigp->delete_वृक्ष(), sigp);
            Ast_गाँठ_व्यंजक*& countp = vtxp->datap()->state_गिनती_Sigp;
            अगर (countp) VL_DO_DANGLING(countp->delete_वृक्ष(), countp);
            Ast_गाँठ_व्यंजक*& overflowp = vtxp->datap()->state_गिनती_Overflow_Sigp;
            अगर (overflowp) VL_DO_DANGLING(overflowp->delete_वृक्ष(), overflowp);
        }
        // Disable iff gating (IEEE 1800-2023 16.12) on the current level value.
        अगर (c.disableExprp) {
            // terminalActivep is always set, so gate it unconditionally.
            Ast_गाँठ_व्यंजक* स्थिर notTermp
                = नया AstLogNot{c.flp, c.disableExprp->clone_वृक्ष_Pure(false)};
            sigs.terminalActivep = नया AstLogAnd{c.flp, sigs.terminalActivep, notTermp};
            अगर (sigs.rejectBasep) {
                Ast_गाँठ_व्यंजक* स्थिर notDisp
                    = नया AstLogNot{c.flp, c.disableExprp->clone_वृक्ष_Pure(false)};
                sigs.rejectBasep = नया AstLogAnd{c.flp, sigs.rejectBasep, notDisp};
            }
            अगर (sigs.throughoutRejectp) {
                Ast_गाँठ_व्यंजक* स्थिर notDisp
                    = नया AstLogNot{c.flp, c.disableExprp->clone_वृक्ष_Pure(false)};
                sigs.throughoutRejectp = नया AstLogAnd{c.flp, sigs.throughoutRejectp, notDisp};
            }
            अगर (sigs.requiredStepRejectp) {
                Ast_गाँठ_व्यंजक* स्थिर notDisp
                    = नया AstLogNot{c.flp, c.disableExprp->clone_वृक्ष_Pure(false)};
                sigs.requiredStepRejectp = नया AstLogAnd{c.flp, sigs.requiredStepRejectp, notDisp};
            }
            अगर (sigs.matchCountp) {
                sigs.matchCountp = gateCheckedCountp(
                    c.flp, sigs.matchCountp, sigs.match_गिनती_Overflowp,
                    नया AstLogNot{c.flp, c.disableExprp->clone_वृक्ष_Pure(false)});
            }
        }

        अगर (sigs.match_गिनती_Overflowp) {
            m_modp->addStmtsp(
                नया AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_वृक्ष(false),
                              new_गिनती_Overflow_Warning(c.flp, sigs.match_गिनती_Overflowp)});
            sigs.match_गिनती_Overflowp = nullptr;
        }

        वापस sigs;
    }

    // Phase 1: Resolve combinational Links via fixed-point propagation.
    व्योम resolveLinks(LowerCtx& c, Ast_गाँठ_व्यंजक* triggerExprp, स्थिर bool गिनती_Matches) {
        // datap() was freshly allocated in lower() -- all state signals start null.
        c.graph.m_startVertexp->datap()->stateSigp = triggerExprp->clone_वृक्ष_Pure(false);
        c.graph.m_startVertexp->datap()->state_गिनती_Sigp
            = नया AstExtend{c.flp, triggerExprp->clone_वृक्ष_Pure(false), m_u32DTypep->width()};
        क्रम (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* स्थिर vtxp = static_cast<SvaStateVertex*>(&vtxr);
            अगर (vtxp->datap()->stateVarp) {
                vtxp->datap()->stateSigp
                    = नया AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
                vtxp->datap()->state_गिनती_Sigp = नया AstExtend{
                    c.flp, नया AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ},
                    m_u32DTypep->width()};
            } अन्यथा अगर (vtxp->datap()->delayRingVarp) {
                अगर (vtxp->m_isFixedDelayRing) {
                    // state_count = wrapped ? head : 0; state = state_count != 0;
                    Ast_गाँठ_व्यंजक* स्थिर countp = delay_Ring_Output_गिनती(c.flp, vtxp);
                    vtxp->datap()->stateSigp = नया AstNeq{c.flp, countp->clone_वृक्ष_Pure(false),
                                                          newTypedConstp(c.flp, m_u32DTypep, 0)};
                    vtxp->datap()->state_गिनती_Sigp = countp;
                } अन्यथा {
                    vtxp->datap()->stateSigp
                        = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_गिनती_Varp);
                    vtxp->datap()->state_गिनती_Sigp = नया AstVarRef{
                        c.flp, vtxp->datap()->delay_Ring_Live_गिनती_Varp, VAccess::READ};
                }
            }
        }
        // Fixed-point propagation along zero-delay (Link) edges. Rebuild each
        // derived signal from its incoming edges on every pass; appending the same
        // contributions repeatedly makes merge expressions grow exponentially.
        स्थिर माप_प्रकार passes = 2 * c.graph.m_graph.vertices().आकार() + 2;
        क्रम (माप_प्रकार pass = 0; pass < passes; ++pass) {
            // Rebuild SAnd combiners once both sub-NFA terminals are available.
            क्रम (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* स्थिर vtxp = static_cast<SvaStateVertex*>(&vtxr);
                अगर (!vtxp->m_isAndCombiner) जारी;
                // AndCombiner vertices always have both terminal pointers set.
                UASSERT_OBJ(vtxp->m_andLhsTermp && vtxp->m_andRhsTermp, vtxp,
                            "AndCombiner vertex missing LHS/RHS terminal");
                स्थिर SvaStateVertex* स्थिर lhsTermp = vtxp->m_andLhsTermp;
                स्थिर SvaStateVertex* स्थिर rhsTermp = vtxp->m_andRhsTermp;
                अगर (!lhsTermp->datap()->stateSigp || !rhsTermp->datap()->stateSigp) जारी;
                Ast_गाँठ_व्यंजक* स्थिर matchLp
                    = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, vtxp->m_andLhsCondp);
                Ast_गाँठ_व्यंजक* स्थिर matchRp
                    = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, vtxp->m_andRhsCondp);
                Ast_गाँठ_व्यंजक* स्थिर doneLOrp = नया AstLogOr{
                    c.flp, नया AstVarRef{c.flp, vtxp->datap()->doneLVarp, VAccess::READ}, matchLp};
                Ast_गाँठ_व्यंजक* स्थिर doneROrp = नया AstLogOr{
                    c.flp, नया AstVarRef{c.flp, vtxp->datap()->doneRVarp, VAccess::READ}, matchRp};
                Ast_गाँठ_व्यंजक* स्थिर bothp = नया AstLogAnd{c.flp, doneLOrp, doneROrp};
                Ast_गाँठ_व्यंजक* स्थिर oneNowp = नया AstLogOr{c.flp, matchLp->clone_वृक्ष_Pure(false),
                                                          matchRp->clone_वृक्ष_Pure(false)};
                अगर (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_वृक्ष(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = नया AstLogAnd{c.flp, bothp, oneNowp};
                अगर (vtxp->datap()->state_गिनती_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_गिनती_Sigp->delete_वृक्ष(),
                                   vtxp->datap()->state_गिनती_Sigp);
                }
                vtxp->datap()->state_गिनती_Sigp = नया AstExtend{
                    c.flp, vtxp->datap()->stateSigp->clone_वृक्ष_Pure(false), m_u32DTypep->width()};
            }

            क्रम (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* स्थिर vtxp = static_cast<SvaStateVertex*>(&vtxr);
                अगर (vtxp == c.graph.m_startVertexp || vtxp->datap()->stateVarp
                    || vtxp->datap()->delayRingVarp || vtxp->m_isAndCombiner || vtxp->m_isMatch
                    || vtxp->m_isRejectSink) {
                    जारी;
                }
                Ast_गाँठ_व्यंजक* अग्र_Statep = nullptr;
                Ast_गाँठ_व्यंजक* अग्र_State_Countp = nullptr;
                Ast_गाँठ_व्यंजक* अग्र_State_गिनती_Overflowp = nullptr;
                क्रम (स्थिर V3GraphEdge& er : vtxp->inEdges()) {
                    स्थिर SvaTransEdge& te = static_cast<स्थिर SvaTransEdge&>(er);
                    स्थिर SvaStateVertex* स्थिर fromVtxp = te.fromVtxp();
                    अगर (!fromVtxp->datap()->stateSigp) जारी;
                    UASSERT_OBJ(fromVtxp->datap()->state_गिनती_Sigp, fromVtxp,
                                "Link source missing stateCountSig");
                    Ast_गाँठ_व्यंजक* स्थिर contributionp = andCond(
                        c.flp, fromVtxp->datap()->stateSigp->clone_वृक्ष_Pure(false), te.m_condp);
                    अग्र_Statep = orExprs(c.flp, अग्र_Statep, contributionp);
                    Ast_गाँठ_व्यंजक* गिनती_Contributionp
                        = fromVtxp->datap()->state_गिनती_Sigp->clone_वृक्ष_Pure(false);
                    Ast_गाँठ_व्यंजक* गिनती_Contribution_Overflowp
                        = गिनती_Matches && fromVtxp->datap()->state_गिनती_Overflow_Sigp
                              ? fromVtxp->datap()->state_गिनती_Overflow_Sigp->clone_वृक्ष_Pure(false)
                              : nullptr;
                    अगर (te.m_condp) {
                        Ast_गाँठ_व्यंजक* स्थिर enablep = te.m_condp->clone_वृक्ष_Pure(false);
                        गिनती_Contributionp
                            = गिनती_Matches
                                  ? gateCheckedCountp(c.flp, गिनती_Contributionp,
                                                      गिनती_Contribution_Overflowp, enablep)
                                  : addCountp(c.flp, nullptr, गिनती_Contributionp, enablep);
                    }
                    अग्र_State_Countp
                        = गिनती_Matches
                              ? addCheckedCountp(c.flp, अग्र_State_Countp, गिनती_Contributionp,
                                                 अग्र_State_गिनती_Overflowp,
                                                 गिनती_Contribution_Overflowp)
                              : addCountp(c.flp, अग्र_State_Countp, गिनती_Contributionp);
                }
                अगर (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_वृक्ष(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = अग्र_Statep;
                अगर (vtxp->datap()->state_गिनती_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_गिनती_Sigp->delete_वृक्ष(),
                                   vtxp->datap()->state_गिनती_Sigp);
                }
                vtxp->datap()->state_गिनती_Sigp = अग्र_State_Countp;
                अगर (vtxp->datap()->state_गिनती_Overflow_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_गिनती_Overflow_Sigp->delete_वृक्ष(),
                                   vtxp->datap()->state_गिनती_Overflow_Sigp);
                }
                vtxp->datap()->state_गिनती_Overflow_Sigp = अग्र_State_गिनती_Overflowp;
            }
        }
    }

खुला:
    // Combine terminal/reject signals into final output expression.
    अटल Ast_गाँठ_व्यंजक* assemble_परिणाम(Ast_गाँठ_Cover_Or_Assert* स्थिर assertp, स्थिर bool negated,
                                       Ast_गाँठ_व्यंजक* स्थिर matchCondp, स्थिर SignalSet& sigs,
                                       Ast_गाँठ_व्यंजक** स्थिर outMatchpp) {
        File_पंक्ति* स्थिर flp = assertp->fileline();
        स्थिर bool isCover = VN_IS(assertp, Cover);
        Ast_गाँठ_व्यंजक* स्थिर terminalActivep = sigs.terminalActivep;
        Ast_गाँठ_व्यंजक* स्थिर rejectBasep = sigs.rejectBasep;
        Ast_गाँठ_व्यंजक* स्थिर throughoutRejectp = sigs.throughoutRejectp;
        Ast_गाँठ_व्यंजक* स्थिर requiredStepRejectp = sigs.requiredStepRejectp;
        // Property negation (IEEE 1800-2023 16.12.1 `not`): invert match/reject.
        अगर (negated) {
            अगर (isCover) {
                अगर (terminalActivep)
                    VL_DO_DANGLING(terminalActivep->delete_वृक्ष(), terminalActivep);
                Ast_गाँठ_व्यंजक* negRejectp = nullptr;
                अगर (matchCondp && rejectBasep) {
                    Ast_गाँठ_व्यंजक* स्थिर sampledCondp = sampled(matchCondp->clone_वृक्ष_Pure(false));
                    Ast_गाँठ_व्यंजक* स्थिर notCondp = नया AstLogNot{flp, sampledCondp};
                    negRejectp = नया AstLogAnd{flp, rejectBasep, notCondp};
                } अन्यथा अगर (rejectBasep) {
                    VL_DO_DANGLING(rejectBasep->delete_वृक्ष(), rejectBasep);
                }
                अगर (throughoutRejectp) negRejectp = orExprs(flp, negRejectp, throughoutRejectp);
                अगर (requiredStepRejectp)
                    negRejectp = orExprs(flp, negRejectp, requiredStepRejectp);
                वापस negRejectp ? negRejectp : नया AstConst{flp, AstConst::BitFalse{}};
            }
            // Negated assert/assume: output = !match.
            Ast_गाँठ_व्यंजक* matchp = terminalActivep;
            अगर (matchCondp) {
                Ast_गाँठ_व्यंजक* स्थिर sampledCondp = sampled(matchCondp->clone_वृक्ष_Pure(false));
                matchp = नया AstLogAnd{flp, matchp, sampledCondp};
            }
            अगर (outMatchpp) {
                Ast_गाँठ_व्यंजक* notPMatchp = nullptr;
                अगर (matchCondp && rejectBasep) {
                    Ast_गाँठ_व्यंजक* स्थिर sampledCondp = sampled(matchCondp->clone_वृक्ष_Pure(false));
                    notPMatchp = नया AstLogAnd{flp, rejectBasep->clone_वृक्ष_Pure(false),
                                               नया AstLogNot{flp, sampledCondp}};
                } अन्यथा अगर (rejectBasep) {
                    notPMatchp = rejectBasep->clone_वृक्ष_Pure(false);
                }
                अगर (throughoutRejectp)
                    notPMatchp = orExprs(flp, notPMatchp, throughoutRejectp->clone_वृक्ष_Pure(false));
                अगर (requiredStepRejectp)
                    notPMatchp
                        = orExprs(flp, notPMatchp, requiredStepRejectp->clone_वृक्ष_Pure(false));
                *outMatchpp = notPMatchp;
            }
            अगर (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_वृक्ष(), throughoutRejectp);
            अगर (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_वृक्ष(), rejectBasep);
            अगर (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_वृक्ष(), requiredStepRejectp);
            Ast_गाँठ_व्यंजक* स्थिर परिणाम_Exprp = नया AstLogNot{flp, matchp};
            वापस परिणाम_Exprp;
        }
        अगर (isCover) {
            अगर (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_वृक्ष(), throughoutRejectp);
            अगर (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_वृक्ष(), rejectBasep);
            अगर (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_वृक्ष(), requiredStepRejectp);
            अगर (matchCondp) {
                Ast_गाँठ_व्यंजक* स्थिर sampledCondp = sampled(matchCondp->clone_वृक्ष_Pure(false));
                वापस नया AstLogAnd{flp, terminalActivep, sampledCondp};
            }
            वापस terminalActivep;
        }
        // Assert/assume: output = !reject
        Ast_गाँठ_व्यंजक* rejectp = nullptr;
        अगर (matchCondp && rejectBasep) {
            Ast_गाँठ_व्यंजक* स्थिर sampledCondp = sampled(matchCondp->clone_वृक्ष_Pure(false));
            rejectp = नया AstLogAnd{flp, rejectBasep, नया AstLogNot{flp, sampledCondp}};
        } अन्यथा अगर (rejectBasep) {
            VL_DO_DANGLING(rejectBasep->delete_वृक्ष(), rejectBasep);
        }
        अगर (outMatchpp) {
            Ast_गाँठ_व्यंजक* matchExprp = terminalActivep->clone_वृक्ष_Pure(false);
            अगर (matchCondp) {
                Ast_गाँठ_व्यंजक* स्थिर sp = sampled(matchCondp->clone_वृक्ष_Pure(false));
                matchExprp = नया AstLogAnd{flp, matchExprp, sp};
            }
            *outMatchpp = matchExprp;
        }
        अगर (terminalActivep) VL_DO_DANGLING(terminalActivep->delete_वृक्ष(), terminalActivep);
        अगर (throughoutRejectp) rejectp = orExprs(flp, rejectp, throughoutRejectp);
        अगर (requiredStepRejectp) rejectp = orExprs(flp, rejectp, requiredStepRejectp);
        अगर (!rejectp) वापस नया AstConst{flp, AstConst::BitTrue{}};
        Ast_गाँठ_व्यंजक* स्थिर परिणाम_Exprp = नया AstLogNot{flp, rejectp};
        वापस परिणाम_Exprp;
    }

    explicit SvaNfaLowering(Ast_गाँठ_Module* modp)
        : m_modp{modp}
        , m_u32DTypep{modp->findBasicDType(VBasicDTypeKwd::UINT32)} {}
    ~SvaNfaLowering() {
        V3Stats::add_Stat_योग("Assertions, NFA delay ring edge visits", m_statDelayRingEdgeVisits);
    }

    // Lower NFA graph to synthesizable AstAlways blocks and raw result signals.
    // Links are combinational; Edges are registered (NBA).
    SignalSet lower(Ast_गाँठ_Cover_Or_Assert* स्थिर assertp, SvaGraph& graph,
                    Ast_Sen_वृक्ष* स्थिर senTreep, Ast_गाँठ_व्यंजक* स्थिर matchCondp,
                    Ast_गाँठ_व्यंजक* स्थिर disableExprp, स्थिर bool need_Thread_Fail_गिनती,
                    स्थिर bool need_Throughout_Thread_Fail_गिनती) {
        File_पंक्ति* स्थिर flp = assertp->fileline();
        AstCover* स्थिर coverp = VN_CAST(assertp, Cover);
        स्थिर bool isSeqEvent = coverp && coverp->isSeqEvent();
        स्थिर VAssertType assertType
            = isSeqEvent ? VAssertType{VAssertType::INTERNAL} : assertp->userType();
        स्थिर VAssertDirectiveType directiveType
            = isSeqEvent ? VAssertDirectiveType{VAssertDirectiveType::INTERNAL}
                         : assertp->directive();
        स्थिर std::string baseName = m_names.लो("");

        // Number vertices with sequential colors for array indexing.
        पूर्णांक N = 0;
        क्रम (V3GraphVertex& vtxr : graph.m_graph.vertices()) { vtxr.color(N++); }
        std::vector<SvaStateVertex*> vtx(N, nullptr);
        क्रम (V3GraphVertex& vtxr : graph.m_graph.vertices()) {
            vtx[vtxr.color()] = static_cast<SvaStateVertex*>(&vtxr);
        }

        // Allocate per-vertex lowering data (stored via V3GraphVertex::userp()).
        std::vector<Sva_Vertex_तथ्य> vertex_तथ्य(N);
        क्रम (पूर्णांक i = 0; i < N; ++i) vtx[i]->userp(&vertex_तथ्य[i]);

        // Identify registered vertices (targets of clocked edges).
        क्रम (पूर्णांक i = 0; i < N; ++i) {
            क्रम (स्थिर V3GraphEdge& edger : vtx[i]->outEdges()) {
                स्थिर SvaTransEdge& tedger = static_cast<स्थिर SvaTransEdge&>(edger);
                स्थिर पूर्णांक toIdx = tedger.toVtxp()->color();
                अगर (tedger.m_consumesCycle && tedger.toVtxp() != graph.m_matchVertexp
                    && !tedger.toVtxp()->m_isRejectSink) {
                    vtx[toIdx]->datap()->needsReg = true;
                }
            }
        }

        // A packed range ring can emit several live endpoints without needing per-slot counts.
        // Widen only a ring whose input can already carry multiplicity.
        std::vector<bool> stateMayHaveMultiplicity(N);
        std::vector<bool> ringNeedsMultiplicity(N);
        std::vector<पूर्णांक> multiplicityWork;
        स्थिर bool गिनती_Matches = coverp && coverp->isCoverSeq() && !isSeqEvent;
        अगर (गिनती_Matches) {
            क्रम (पूर्णांक i = 0; i < N; ++i) {
                स्थिर bool multipleInputs = vtx[i]->inEdges().आकार() > 1;
                स्थिर bool isRing = vtx[i]->m_delay_Ring_आकार;
                ringNeedsMultiplicity[i] = isRing && multipleInputs;
                स्थिर bool startsMultiplicity = isRing
                                                    ? !vtx[i]->m_isFixedDelayRing
                                                    : multipleInputs && !vtx[i]->datap()->needsReg;
                अगर (startsMultiplicity) {
                    stateMayHaveMultiplicity[i] = true;
                    multiplicityWork.धकेलो_back(i);
                }
            }
            क्रम (माप_प्रकार wi = 0; wi < multiplicityWork.आकार(); ++wi) {
                क्रम (स्थिर V3GraphEdge& edger : vtx[multiplicityWork[wi]]->outEdges()) {
                    स्थिर SvaTransEdge& tedger = static_cast<स्थिर SvaTransEdge&>(edger);
                    स्थिर पूर्णांक ti = tedger.toVtxp()->color();
                    स्थिर bool isRing = vtx[ti]->m_delay_Ring_आकार;
                    अगर (isRing) ringNeedsMultiplicity[ti] = true;
                    // Match and reject sinks have no outgoing edges, so they stop naturally.
                    स्थिर bool stopsMultiplicity = !isRing && vtx[ti]->datap()->needsReg;
                    अगर (stateMayHaveMultiplicity[ti] || stopsMultiplicity) { जारी; }
                    stateMayHaveMultiplicity[ti] = true;
                    multiplicityWork.धकेलो_back(ti);
                }
            }
        }

        स्थिर bool asyncDisable = isAsyncDisable(disableExprp);
        // Registers cleared on a rising disable are also written by their clocked block.
        File_पंक्ति* स्थिर regFlp = asyncDisable ? नया File_पंक्ति{flp} : flp;
        अगर (asyncDisable) regFlp->warnOff(V3ErrorCode::MULTIDRIVEN, true);

        AstVar* स्थिर killVarp
            = नया AstVar{flp, VVarType::MODULETEMP, baseName + "__kill", m_u32DTypep};
        killVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(killVarp);
        क्रम (पूर्णांक i = 0; i < N; ++i) {
            अगर (vtx[i]->m_isAndCombiner) {
                स्थिर std::string base = baseName + "__a" + std::to_string(i);
                AstVar* स्थिर lp = नया AstVar{regFlp, VVarType::MODULETEMP, base + "_doneL",
                                              m_modp->findBitDType()};
                lp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(lp);
                vtx[i]->datap()->doneLVarp = lp;
                AstVar* स्थिर rp = नया AstVar{regFlp, VVarType::MODULETEMP, base + "_doneR",
                                              m_modp->findBitDType()};
                rp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(rp);
                vtx[i]->datap()->doneRVarp = rp;
                जारी;
            }
            अगर (vtx[i]->m_delay_Ring_आकार) {
                स्थिर std::string base = baseName + "__d" + std::to_string(i);
                स्थिर bool storesCounts = गिनती_Matches && ringNeedsMultiplicity[i];
                Ast_गाँठ_DType* ringDTypep = nullptr;
                अगर (storesCounts) {
                    // int unsigned ring [size];
                    ringDTypep = नया AstUnpackArrayDType{
                        flp, m_u32DTypep,
                        नया AstRange{flp, static_cast<पूर्णांक>(vtx[i]->m_delay_Ring_आकार - 1), 0}};
                    v3Global.rootp()->typeTablep()->addTypesp(ringDTypep);
                } अन्यथा {
                    // bit [size-1:0] ring;
                    ringDTypep = m_modp->findBitDType(vtx[i]->m_delay_Ring_आकार,
                                                      vtx[i]->m_delay_Ring_आकार, VSigning::UNSIGNED);
                }
                AstVar* स्थिर ringp
                    = नया AstVar{flp, VVarType::MODULETEMP, base + "_ring", ringDTypep};
                ringp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(ringp);
                vtx[i]->datap()->delayRingVarp = ringp;
                vtx[i]->datap()->delayRingCounts = storesCounts;
                AstVar* स्थिर headp
                    = नया AstVar{flp, VVarType::MODULETEMP, base + "_head",
                                 storesCounts ? m_u32DTypep : m_modp->findBitDType()};
                headp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(headp);
                vtx[i]->datap()->delay_Ring_शीर्ष_Varp = headp;
                // int unsigned idx;
                AstVar* स्थिर idxp
                    = नया AstVar{regFlp, VVarType::MODULETEMP, base + "_idx", m_u32DTypep};
                idxp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(idxp);
                vtx[i]->datap()->delayRingIdxVarp = idxp;
                // int unsigned live_count;
                AstVar* स्थिर live_गिनती_Varp
                    = नया AstVar{regFlp, VVarType::MODULETEMP, base + "_liveCount", m_u32DTypep};
                live_गिनती_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(live_गिनती_Varp);
                vtx[i]->datap()->delay_Ring_Live_गिनती_Varp = live_गिनती_Varp;
                AstVar* स्थिर wrappedp = नया AstVar{regFlp, VVarType::MODULETEMP,
                                                    base + "_wrapped", m_modp->findBitDType()};
                wrappedp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(wrappedp);
                vtx[i]->datap()->delayRingWrappedVarp = wrappedp;
                जारी;
            }
            अगर (!vtx[i]->datap()->needsReg) जारी;
            अगर (vtx[i] == graph.m_startVertexp || vtx[i]->m_isMatch) जारी;
            स्थिर std::string varName = baseName + "__s" + std::to_string(i);
            AstVar* स्थिर varp
                = नया AstVar{regFlp, VVarType::MODULETEMP, varName, m_modp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(varp);
            vtx[i]->datap()->stateVarp = varp;
        }

        // Build lowering context for phase sub-functions.
        LowerCtx c{flp,        senTreep,      disableExprp, matchCondp,
                   assertType, directiveType, killVarp,     graph};

        // Phase 1: Resolve combinational Links via fixed-point propagation.
        Ast_गाँठ_व्यंजक* स्थिर triggerExprp
            = isSeqEvent ? नया AstConst{flp, AstConst::BitTrue{}}
                         : assertOnCond(flp, assertp->userType(), assertp->directive());
        resolveLinks(c, triggerExprp, गिनती_Matches);
        VL_DO_DANGLING(triggerExprp->delete_वृक्ष(), triggerExprp);

        // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
        emitStateRegisterNba(c);
        emitDelayRingUpdates(c, गिनती_Matches);
        emitAndCombinerDoneLatchNba(c);
        emitKillAckNba(c);

        // Phase 3/3a/3b: Compute terminal match/reject signals (cleans up stateSig).
        स्थिर SignalSet sigs
            = computeSignals(c, need_Thread_Fail_गिनती, need_Throughout_Thread_Fail_गिनती, गिनती_Matches);

        // Strong s_always[m:n] end-of-simulation liveness: if any in-window state
        // is still set at $finish, the universal-quantifier window never completed
        // (IEEE 1800-2023 16.12.11 strong semantics). Fire the assertion failure
        // from a final block; V3Assert turns the DT_ERROR display into the standard
        // "Assertion failed in %m" message.
        // pending = |{strong state registers, strong delay live counts != 0};
        Ast_गाँठ_व्यंजक* pendingp = nullptr;
        क्रम (पूर्णांक i = 0; i < N; ++i) {
            अगर (!vtx[i]->m_strongPending) जारी;
            Ast_गाँठ_व्यंजक* pendingExprp = nullptr;
            अगर (vtx[i]->datap()->stateVarp) {
                pendingExprp = नया AstVarRef{flp, vtx[i]->datap()->stateVarp, VAccess::READ};
            } अन्यथा अगर (vtx[i]->m_strongAlwaysRing) {
                pendingExprp
                    = delayRingHasLiveThreadsp(flp, vtx[i]->datap()->delay_Ring_Live_गिनती_Varp);
            } अन्यथा {
                जारी;
            }
            अगर (!pendingp) {
                pendingp = pendingExprp;
            } अन्यथा {
                pendingp = नया AstLogOr{flp, pendingp, pendingExprp};
            }
        }
        अगर (pendingp) {
            AstCExpr* स्थिर assertOnp
                = नया AstCExpr{flp, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()", 1};
            Ast_गाँठ_व्यंजक* स्थिर condp = नया AstLogAnd{flp, assertOnp, pendingp};
            AstDisplay* स्थिर dispp
                = नया AstDisplay{flp, VDisplayType::DT_ERROR, "", nullptr, nullptr};
            dispp->fmtp()->timeunit(m_modp->timeunit());
            Ast_गाँठ_Stmt* स्थिर firep = dispp;
            अगर (v3Global.opt.stopFail()) firep->add_अग्र(नया AstStop{flp, false});
            m_modp->addStmtsp(नया AstFinal{flp, नया AstIf{flp, condp, firep}});
        }

        // Clear userp on every vertex before vertexData is destroyed.
        क्रम (पूर्णांक i = 0; i < N; ++i) vtx[i]->userp(nullptr);
        वापस sigs;
    }
};

}  // namespace

//######################################################################
// Top-level visitor

श्रेणी AssertNfaVisitor final : खुला VNVisitor {
    // STATE
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module being processed
    AstClocking* m_defaultClockingp = nullptr;  // Default clocking
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff
    SvaNfaLowering* m_loweringp = nullptr;  // NFA-to-hardware lowering engine
    Ast_Sen_वृक्ष* m_sampled_मूल्य_Clockp = nullptr;  // Inherited clock during scoped attachment
    V3UniqueNames m_propVarNames{"__Vpropvar"};  // Property-local variable names
    V3UniqueNames m_prop_अस्थायी_Names{"__VnfaSampled"};  // Hoisted $sampled(propp) temps
    V3UniqueNames m_fail_गिनती_Names{"__VnfaRemainingFailCount"};  // Fail replay counter names
    std::set<स्थिर AstProperty*> m_inliningProps;  // Recursion guard for inlineNamedProperty

    ढाँचा <typename T_गाँठ>
    व्योम visit_Sampled_मूल्य(T_गाँठ* स्थिर nodep) {
        अगर (m_sampled_मूल्य_Clockp && !nodep->sentreep()) {
            nodep->sentreep(m_sampled_मूल्य_Clockp->clone_वृक्ष(true));
        }
        iterateChildren(nodep);
    }

    // Wire match vertex and mid-window sources for a successful NFA build.
    अटल व्योम wireMatchAndMidSources(SvaGraph& graph, स्थिर Build_परिणाम& परिणाम, File_पंक्ति* flp) {
        graph.createMatchVertex();
        // Skip the main term Link when midSources already cover every
        // end-of-match (cover_sequence path); otherwise the per-mid extraction
        // double-counts via the merge vertex.
        अगर (!परिणाम.termIsMidMerge) { graph.addLink(परिणाम.termVertexp, graph.m_matchVertexp); }
        क्रम (SvaStateVertex* srcVtxp : परिणाम.midSources) {
            Ast_गाँठ_व्यंजक* condp = nullptr;
            क्रम (Ast_गाँठ_व्यंजक* स्थिर tc : srcVtxp->m_throughoutConds) {
                Ast_गाँठ_व्यंजक* स्थिर tcClone = tc->clone_वृक्ष_Pure(false);
                condp = condp ? नया AstLogAnd{flp, condp, tcClone} : tcClone;
            }
            graph.addLink(srcVtxp, graph.m_matchVertexp, condp);
            srcVtxp->m_isUnbounded = true;
        }
    }

    अटल Ast_गाँठ_व्यंजक* getSequenceBodyExprp(स्थिर AstSequence* seqp) {
        Ast_गाँठ* bodyp = seqp->stmtsp();
        जबतक (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        वापस VN_CAST(bodyp, गाँठ_व्यंजक);
    }

    अटल AstPropSpec* getPropertySpecp(स्थिर AstProperty* propp) {
        Ast_गाँठ* stmtp = propp->stmtsp();
        // V3LinkParse emits InitialStaticStmt for property-local variable
        // initialisers; the InitialAutomaticStmt variant only appears for
        // task/function-scope automatic lifetime, not properties.
        जबतक (stmtp
               && (VN_IS(stmtp, Var) || VN_IS(stmtp, InitialStaticStmt)
                   || VN_IS(stmtp, InitialAutomaticStmt))) {  // LCOV_EXCL_LINE
            stmtp = stmtp->nextp();
        }
        वापस VN_CAST(stmtp, PropSpec);
    }

    व्योम inlineNamedProperty(AstPropSpec* outerSpecp, AstFuncRef* funcrefp,
                             स्थिर AstProperty* propyp) {
        // Recursion guard: IEEE 1800-2023 16.12.1 forbids recursive properties.
        // V3Width emits "Recursive property call" for direct recursion before this
        // pass runs; this catches any nested-inlining cycle that slips past.
        अगर (!m_inliningProps.emplace(propyp).second) {
            funcrefp->v3error("Illegal recursive property reference");  // LCOV_EXCL_LINE
            वापस;  // LCOV_EXCL_LINE
        }
        काष्ठा Guard final {
            std::set<स्थिर AstProperty*>& setr;
            स्थिर AstProperty* keyp;
            ~Guard() { setr.erase(keyp); }
        } guard{m_inliningProps, propyp};
        AstPropSpec* propSpecp = getPropertySpecp(propyp);
        UASSERT_OBJ(propSpecp, funcrefp, "Property has no body PropSpec");
        propSpecp = propSpecp->clone_वृक्ष(false);

        स्थिर V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propyp->stmtsp());
        std::unordered_map<स्थिर AstVar*, Ast_गाँठ_व्यंजक*> portMap;
        क्रम (स्थिर स्वतः& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables to module-level temps (IEEE 16.10).
        std::unordered_map<स्थिर AstVar*, AstVar*> localVarMap;
        क्रम (Ast_गाँठ* stmtp = propyp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (AstVar* स्थिर varp = VN_CAST(stmtp, Var)) {
                अगर (!varp->isIO()) {
                    स्थिर string newName = m_propVarNames.लो(varp);
                    AstVar* स्थिर newVarp = नया AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        propSpecp->foreach([&](AstVarRef* refp) {
            स्थिर स्वतः portIt = portMap.find(refp->varp());
            अगर (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_वृक्ष(false));
                VL_DO_DANGLING(धकेलो_Deletep(refp), refp);
                वापस;
            }
            स्थिर स्वतः localIt = localVarMap.find(refp->varp());
            अगर (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        क्रम (स्थिर स्वतः& tconnect : tconnects) {
            धकेलो_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propSpecp, funcrefp);

        // Merge disable iff (IEEE 1800-2023 16.12.1)
        अगर (outerSpecp->disablep() && propSpecp->disablep()) {
            outerSpecp->v3error("disable iff expression before property call "
                                "and in its body is not legal");
            धकेलो_Deletep(propSpecp->disablep()->unlinkFrBack());
        }
        अगर (outerSpecp->disablep()) {
            propSpecp->disablep(outerSpecp->disablep()->unlinkFrBack());
        }

        अगर (outerSpecp->sensesp() && propSpecp->sensesp()) {
            outerSpecp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Clock event before property call and in its body");
            धकेलो_Deletep(propSpecp->sensesp()->unlinkFrBack());
        }
        अगर (outerSpecp->sensesp()) {
            AstSenItem* स्थिर sensesp = outerSpecp->sensesp();
            sensesp->unlinkFrBack();
            propSpecp->sensesp(sensesp);
        }

        outerSpecp->replaceWith(propSpecp);
        VL_DO_DANGLING(धकेलो_Deletep(outerSpecp), outerSpecp);
    }

    व्योम inlineSequenceRef(AstFuncRef* funcrefp, AstSequence* seqp) {
        Ast_गाँठ_व्यंजक* स्थिर bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        Ast_गाँठ_व्यंजक* स्थिर clonedp = bodyExprp->clone_वृक्ष(false);

        स्थिर V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<स्थिर AstVar*, Ast_गाँठ_व्यंजक*> portMap;
        क्रम (स्थिर स्वतः& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            स्थिर स्वतः it = portMap.find(refp->varp());
            अगर (it != portMap.end()) {
                refp->replaceWith(it->second->clone_वृक्ष(false));
                VL_DO_DANGLING(धकेलो_Deletep(refp), refp);
            }
        });
        क्रम (स्थिर स्वतः& tconnect : tconnects) {
            धकेलो_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(धकेलो_Deletep(funcrefp), funcrefp);
        // Clear referenced flag so V3AssertPre cleanup does not emit
        // spurious UNSUPPORTED for sequences that were already inlined here.
        seqp->isReferenced(false);
    }

    // Must run before hasMultiCycleExpr() so NFA sees sequence bodies.
    व्योम inlineAllSequenceRefs(Ast_गाँठ* rootp) {
        bool changed = true;
        जबतक (changed) {
            changed = false;
            rootp->foreach([&](AstFuncRef* funcrefp) {
                अगर (changed) वापस;
                अगर (AstSequence* स्थिर seqp = VN_CAST(funcrefp->taskp(), Sequence)) {
                    inlineSequenceRef(funcrefp, seqp);
                    changed = true;
                }
            });
        }
    }

    अटल bool has_Multi_Cycle_व्यंजक(स्थिर Ast_गाँठ* nodep) {
        वापस nodep->exists([](स्थिर Ast_गाँठ* np) {
            अगर (स्थिर स्वतः* स्थिर ep = VN_CAST(np, गाँठ_व्यंजक)) वापस ep->isMultiCycleSva();
            वापस false;
        });
    }

    अटल VPropStrength effectiveAssertPropStrength(स्थिर AstPropSpec* स्थिर propSpecp) {
        अगर (propSpecp->propStrength() != VPropStrength::DEFAULT) वापस propSpecp->propStrength();
        वापस propSpecp->fileline()->language() <= V3LangCode::L1800_2005 ? VPropStrength::STRONG
                                                                           : VPropStrength::WEAK;
    }

    // Bare `assert property (p until q)` with boolean operands stays on
    // V3AssertPre's AstLoop lowering, which preserves per-attempt action-block
    // firings that this NFA's single-bit aggregated state cannot. Strong bare
    // forms are also lowered there. NFA still owns sequence operands and any
    // embedding inside a multi-cycle context (implication consequent, or/and
    // operands, etc.).
    अटल bool is_Bare_शीर्ष_Level_Until(Ast_गाँठ* propp) {
        Ast_गाँठ* p = propp;
        अगर (AstPropSpec* स्थिर specp = VN_CAST(p, PropSpec)) p = specp->propp();
        जबतक (AstLogNot* स्थिर notp = VN_CAST(p, LogNot)) p = notp->lhsp();
        AstUntil* स्थिर untilp = VN_CAST(p, Until);
        अगर (!untilp) वापस false;
        स्थिर स्वतः hasSeq = [](स्थिर Ast_गाँठ_व्यंजक* ep) {
            वापस ep->exists([](स्थिर Ast_गाँठ_व्यंजक* np) { वापस np->isMultiCycleSva(); });
        };
        अगर (hasSeq(untilp->lhsp()) || hasSeq(untilp->rhsp())) वापस false;
        वापस true;
    }

    काष्ठा PropertyParts final {
        Ast_गाँठ_व्यंजक* triggerExprp = nullptr;
        Ast_गाँठ_व्यंजक* seqExprp = nullptr;
        bool isOverlapped = true;
        bool hasImplication = false;
        bool isFollowedBy = false;  // True for #-# / #=# (non-vacuous-fail on antecedent miss)
    };

    अटल PropertyParts decomposeProperty(Ast_गाँठ* propp) {
        PropertyParts parts;
        अगर (AstPropSpec* स्थिर specp = VN_CAST(propp, PropSpec)) { propp = specp->propp(); }
        अगर (AstImplication* स्थिर implp = VN_CAST(propp, Implication)) {
            parts.hasImplication = true;
            parts.isOverlapped = implp->isOverlapped();
            parts.isFollowedBy = implp->isFollowedBy();
            parts.triggerExprp = implp->lhsp();
            parts.seqExprp = implp->rhsp();
        } अन्यथा अगर (Ast_गाँठ_व्यंजक* स्थिर exprp = VN_CAST(propp, गाँठ_व्यंजक)) {
            parts.triggerExprp = nullptr;
            parts.seqExprp = exprp;
        }
        वापस parts;
    }

    अटल bool canSplitImplicationPassActions(स्थिर PropertyParts& parts) {
        UASSERT(parts.hasImplication,
                "Implication pass action split requested without implication");
        UASSERT(parts.triggerExprp, "Implication pass action split requested without trigger");
        // Direct vacuous/nonvacuous classification uses the antecedent value in the current
        // assertion attempt. Leave delayed antecedents on the existing NFA pass path.
        वापस !has_Multi_Cycle_व्यंजक(parts.triggerExprp);
    }

    अटल व्योम splitImplicationPassActions(AstAssert* assertp, स्थिर PropertyParts& parts,
                                            Ast_गाँठ_व्यंजक* nonvacuousMatchp) {
        File_पंक्ति* स्थिर flp = assertp->fileline();

        Ast_गाँठ* स्थिर passsp = assertp->passsp()->unlink_Fr_Back_With_अग्र();
        Ast_गाँठ* splitsp = nullptr;

        अगर (!parts.isFollowedBy) {
            Ast_गाँठ_व्यंजक* स्थिर vacuousp
                = नया AstLogNot{flp, sampled(parts.triggerExprp->clone_वृक्ष_Pure(false))};
            Ast_गाँठ* स्थिर vacuousBodyp = passsp->clone_वृक्ष(false);
            splitsp = newPassOnIf(flp, vacuousp, vacuousBodyp, assertp->userType(),
                                  assertp->directive(), /*vacuous=*/true);
        }

        AstIf* स्थिर nonvacuousp
            = newPassOnIf(flp, nonvacuousMatchp, passsp, assertp->userType(), assertp->directive(),
                          /*vacuous=*/false);
        splitsp = splitsp ? Ast_गाँठ::add_अग्र<Ast_गाँठ, Ast_गाँठ>(splitsp, nonvacuousp)
                          : static_cast<Ast_गाँठ*>(nonvacuousp);
        assertp->addPasssp(splitsp);
    }

    // On a PropSpec-wrapped assertion whose NFA build failed with a semantic
    // error (errorEmitted), replace the body with a BitFalse const so later
    // passes see a well-formed AST. Returns true if replaced.
    व्योम replaceBodyOnBuildError(File_पंक्ति* flp, AstPropSpec* propSpecp, bool errorEmitted) {
        अगर (!errorEmitted) वापस;
        Ast_गाँठ* स्थिर innerPropp = propSpecp->propp();
        innerPropp->replaceWith(नया AstConst{flp, AstConst::BitFalse{}});
        VL_DO_DANGLING(धकेलो_Deletep(innerPropp), innerPropp);
    }

    // Hoist a leading clocking event (IEEE 1800-2023 16.7):
    bool hoistClockedSeq(AstPropSpec* specp) {
        जबतक (AstSClocked* स्थिर clockedp = VN_CAST(specp->propp(), SClocked)) {
            अगर (specp->sensesp()) {
                clockedp->v3warn(E_UNSUPPORTED, "Unsupported: multiclocked sequence or property");
                replaceBodyOnBuildError(specp->fileline(), specp, true);
                वापस true;
            }
            क्रम (स्थिर AstSenItem* sp = clockedp->sensesp(); sp;
                 sp = VN_CAST(sp->nextp(), SenItem)) {
                अगर (!sp->edgeType().anEdge()) {
                    clockedp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: non-edge clocking event on a sequence; "
                                     "use an edge such as @(posedge clk)");
                    replaceBodyOnBuildError(specp->fileline(), specp, true);
                    वापस true;
                }
            }
            specp->sensesp(clockedp->sensesp()->unlink_Fr_Back_With_अग्र());
            Ast_गाँठ_व्यंजक* स्थिर bodyp = clockedp->exprp()->unlinkFrBack();
            clockedp->replaceWith(bodyp);
            VL_DO_DANGLING(धकेलो_Deletep(clockedp), clockedp);
        }
        // A clocking event anywhere else in the sequence is not supported.
        स्थिर AstSClocked* nestedp = nullptr;
        specp->propp()->foreach([&](स्थिर AstSClocked* p) {
            अगर (!nestedp) nestedp = p;
        });
        अगर (nestedp) {
            nestedp->v3warn(E_UNSUPPORTED,
                            "Unsupported: clocking event inside sequence expression");
            replaceBodyOnBuildError(specp->fileline(), specp, true);
            वापस true;
        }
        वापस false;
    }

    // Build the NFA graph for a property body, handling both the antecedent
    // |-> consequent and simple sequence cases. Returns the consequent/body
    // BuildResult (invalid on parse/build failure).
    Build_परिणाम buildAssertionGraph(SvaNfaBuilder& builder, SvaGraph& graph, Ast_गाँठ_व्यंजक* seqBodyp,
                                    स्थिर PropertyParts& parts, File_पंक्ति* flp) {
        अगर (!parts.hasImplication) वापस builder.build(seqBodyp);

        graph.m_startVertexp = graph.createStateVertex();
        वापस builder.buildImplicationEdges(parts.triggerExprp, seqBodyp, graph.m_startVertexp,
                                             parts.isOverlapped, parts.isFollowedBy,
                                             parts.triggerExprp, flp);
    }

    // Install pass-action gating and replay simultaneous per-thread failures.
    व्योम attachActionHandlers(AstAssert* स्थिर assertp, Ast_गाँठ_व्यंजक* matchExprp,
                              Ast_Sen_वृक्ष* स्थिर threadFailReplaySenTreep,
                              Ast_गाँठ_व्यंजक* स्थिर threadFailCountp) {
        // Gate pass handler on match to prevent vacuous-pass firings.
        अगर (matchExprp) {
            // needMatch implies passsp() was non-null when evaluated above;
            // lowering does not mutate the assert's pass-action between the
            // two reads, so passsp() is still non-null here.
            Ast_गाँठ* passsp = assertp->passsp();
            UASSERT_OBJ(passsp, assertp, "needMatch set but passsp is null");
            passsp->unlink_Fr_Back_With_अग्र();
            File_पंक्ति* स्थिर flp = assertp->fileline();
            AstIf* स्थिर ifp = नया AstIf{flp, matchExprp, passsp, nullptr};
            assertp->addPasssp(ifp);
            // Fail-handler prefix for overlapping instances (IEEE 16.12):
            // fires when reject=1 && match=1 in the same cycle.
            अगर (assertp->failsp()) assertp->prependFailsp(ifp->clone_वृक्ष(false));
        }

        अगर (threadFailCountp) {
            UASSERT_OBJ(threadFailReplaySenTreep, assertp,
                        "Thread fail count missing sensitivity tree");
            Ast_गाँठ* स्थिर failsp = assertp->failsp();
            File_पंक्ति* स्थिर flp = assertp->fileline();
            // IEEE 1800-2023 16.12 requires one action-block evaluation per failed
            // thread. AstAssert handles the first, so replay the rest here.
            AstVar* स्थिर remaining_Fail_गिनती_Varp
                = नया AstVar{flp, VVarType::MODULETEMP, m_fail_गिनती_Names.लो(""),
                             m_modp->findBasicDType(VBasicDTypeKwd::UINT32)};
            remaining_Fail_गिनती_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remaining_Fail_गिनती_Varp);
            Ast_गाँठ* स्थिर replayStmtsp = नया AstAssign{
                flp, नया AstVarRef{flp, remaining_Fail_गिनती_Varp, VAccess::WRITE}, threadFailCountp};
            AstLoop* स्थिर replayLoopp = नया AstLoop{flp};
            replayLoopp->addStmtsp(नया AstLoopTest{
                flp, replayLoopp,
                नया AstGt{flp, नया AstVarRef{flp, remaining_Fail_गिनती_Varp, VAccess::READ},
                          newTypedConstp(flp, remaining_Fail_गिनती_Varp->dtypep(), 1)}});
            replayLoopp->addStmtsp(newIfAssertFailOn(failsp->clone_वृक्ष(true), assertp->directive(),
                                                     assertp->userType()));
            AstSub* स्थिर decrementedFailCountp
                = नया AstSub{flp, नया AstVarRef{flp, remaining_Fail_गिनती_Varp, VAccess::READ},
                             newTypedConstp(flp, remaining_Fail_गिनती_Varp->dtypep(), 1)};
            replayLoopp->addStmtsp(
                नया AstAssign{flp, नया AstVarRef{flp, remaining_Fail_गिनती_Varp, VAccess::WRITE},
                              decrementedFailCountp});
            replayStmtsp->add_अग्र(replayLoopp);
            m_modp->addStmtsp(
                नया AstAlways{flp, VAlwaysKwd::ALWAYS, threadFailReplaySenTreep, replayStmtsp});
        }
    }

    // Replace one VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). No-op if refp is not in matchMap.
    व्योम substituteMatchItemRef(AstVarRef* refp, अचिन्हित K,
                                स्थिर std::unordered_map<स्थिर AstVar*, Ast_गाँठ_व्यंजक*>& matchMap) {
        स्थिर स्वतः it = matchMap.find(refp->varp());
        अगर (it == matchMap.end()) वापस;
        Ast_गाँठ_व्यंजक* newp = it->second->clone_वृक्ष_Pure(false);
        अगर (K > 0) {
            AstConst* स्थिर ticksp = नया AstConst{refp->fileline(), AstConst::Widthed_मूल्य{}, 32,
                                                  static_cast<uint32_t>(K)};
            AstPast* स्थिर pastp = नया AstPast{refp->fileline(), newp, ticksp, nullptr,
                                               /* propertyTiming */ true};
            pastp->dtypeFrom(newp);
            newp = pastp;
        }
        refp->replaceWith(newp);
        VL_DO_DANGLING(धकेलो_Deletep(refp), refp);
        वापस;
    }

    // Recursively walk a consequent. Returns cycle length consumed and
    // substitutes each VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). Reports E_UNSUPPORTED on non-constant
    // delays or composite sequence operators and returns -1 on failure.
    पूर्णांक
    walk_Substitute_Match_वस्तुएँ(Ast_गाँठ_व्यंजक* nodep, अचिन्हित K,
                             स्थिर std::unordered_map<स्थिर AstVar*, Ast_गाँठ_व्यंजक*>& match_वस्तुएँ) {
        अगर (AstSExpr* स्थिर sexprp = VN_CAST(nodep, SExpr)) {
            // IEEE 1800-2023 16.9.2: cycle_delay's lhsp is a constant_expression
            // and the delay form in a sequence is always `##N`, folded by
            // V3Const + V3Param before V3AssertNfa. Range form `##[m:n]` is the
            // only user-visible reject here.
            AstDelay* स्थिर delayp = VN_AS(sexprp->delayp(), Delay);
            UASSERT_OBJ(delayp->isCycleDelay() && VN_IS(delayp->lhsp(), Const), sexprp,
                        "SVA cycle delay must have a constant lhsp");
            अगर (delayp->isRangeDelay()) {
                sexprp->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                              "non-constant cycle delay in consequent"
                                              " (IEEE 1800-2023 16.10)");
                वापस -1;
            }
            स्थिर अचिन्हित delayCycles = VN_AS(delayp->lhsp(), Const)->toUInt();
            पूर्णांक preLen = 0;
            अगर (Ast_गाँठ_व्यंजक* स्थिर prep = sexprp->preExprp()) {
                preLen = walk_Substitute_Match_वस्तुएँ(prep, K, match_वस्तुएँ);
                अगर (preLen < 0) वापस -1;
            }
            स्थिर पूर्णांक bodyLen
                = walk_Substitute_Match_वस्तुएँ(sexprp->exprp(), K + preLen + delayCycles, match_वस्तुएँ);
            अगर (bodyLen < 0) वापस -1;
            वापस preLen + delayCycles + bodyLen;
        }
        अगर (nodep->isMultiCycleSva()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                         "composite sequence operator in consequent"
                                         " (IEEE 1800-2023 16.10)");
            वापस -1;
        }
        std::vector<AstVarRef*> refs;
        nodep->foreach([&refs](AstVarRef* p) { refs.धकेलो_back(p); });
        क्रम (AstVarRef* स्थिर refp : refs) substituteMatchItemRef(refp, K, match_वस्तुएँ);
        वापस 0;
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
    bool liftMatchItemSubstitutions(PropertyParts& parts, Ast_गाँठ_व्यंजक* seqBodyp) {
        अगर (!parts.hasImplication) वापस false;
        Ast_व्यंजक_Stmt* स्थिर व्यंजक_Stmtp = VN_CAST(parts.triggerExprp, व्यंजक_Stmt);
        अगर (!व्यंजक_Stmtp) वापस false;
        // IEEE 1800-2023 16.10 BNF requires `(expr, match_item {, match_item})`
        // with at least one match item; V3LinkParse only emits ExprStmt for
        // this form and only emits AstAssign with VarRef LHS for each item.
        std::unordered_map<स्थिर AstVar*, Ast_गाँठ_व्यंजक*> match_वस्तुएँ;
        क्रम (Ast_गाँठ* stmtp = व्यंजक_Stmtp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstAssign* स्थिर assignp = VN_AS(stmtp, Assign);
            AstVarRef* स्थिर lhsRefp = VN_AS(assignp->lhsp(), VarRef);
            match_वस्तुएँ[lhsRefp->varp()] = assignp->rhsp();
        }
        स्थिर अचिन्हित startK = parts.isOverlapped ? 0 : 1;
        स्थिर पूर्णांक लंबाई = walk_Substitute_Match_वस्तुएँ(seqBodyp, startK, match_वस्तुएँ);
        // Match-item substitution / strip mutates ancestor purity. Release
        // builds don't auto-clear caches on edits, so refresh here.
        VIsCached::clear_Cache_वृक्ष();
        अगर (लंबाई < 0) वापस true;
        Ast_गाँठ_व्यंजक* स्थिर antBoolp = व्यंजक_Stmtp->resultp()->unlinkFrBack();
        व्यंजक_Stmtp->replaceWith(antBoolp);
        VL_DO_DANGLING(धकेलो_Deletep(व्यंजक_Stmtp), व्यंजक_Stmtp);
        parts.triggerExprp = antBoolp;
        वापस false;
    }

    // Outcome counts for a property if/case are wrong in an outcome-multiplying
    // context. Returns that context, or nullptr when the shape is supported.
    अटल स्थिर अक्षर* unsupportedPropertyControl(स्थिर Ast_गाँठ_Cover_Or_Assert* assertp,
                                                  स्थिर Ast_गाँठ_व्यंजक* seqBodyp, bool negated) {
        अगर (!hasPropertyControlConjunction(seqBodyp)) वापस nullptr;
        अगर (negated) वापस "negation";
        अगर (VN_IS(assertp, Cover)) वापस "cover";
        अगर (VN_AS(assertp, Assert)->passsp()) वापस "a pass action";
        वापस nullptr;
    }

    // Inline property/sequence refs and reject unsupported shapes.
    // Returns the PropSpec to lower, or nullptr when fully handled here.
    AstPropSpec* prepareAssertionProp(Ast_गाँठ_Cover_Or_Assert* assertp) {
        अगर (AstPropSpec* स्थिर specp = VN_CAST(assertp->propp(), PropSpec)) {
            अगर (AstFuncRef* स्थिर funcrefp = VN_CAST(specp->propp(), FuncRef)) {
                अगर (स्थिर AstProperty* स्थिर propyp = VN_CAST(funcrefp->taskp(), Property)) {
                    inlineNamedProperty(specp, funcrefp, propyp);
                }
            }
        }

        inlineAllSequenceRefs(assertp->propp());

        अगर (AstPropSpec* स्थिर specp = VN_CAST(assertp->propp(), PropSpec)) {
            अगर (hoistClockedSeq(specp)) वापस nullptr;
        }

        AstPropSpec* स्थिर propp = VN_AS(assertp->propp(), PropSpec);
        अगर (!VN_IS(assertp, Cover)
            && effectiveAssertPropStrength(propp) == VPropStrength::STRONG) {
            propp->v3warn(E_UNSUPPORTED,
                          "Unsupported: strong property in " + assertp->verilogKwd() + ".");
            replaceBodyOnBuildError(assertp->fileline(), propp, /*errorEmitted=*/true);
            वापस nullptr;
        }

        अगर (!has_Multi_Cycle_व्यंजक(propp)) वापस nullptr;
        // A nested property instance keeps its body behind the call; lowering would drop it.
        अगर (propp->exists([](स्थिर AstFuncRef* refp) { वापस VN_IS(refp->taskp(), Property); })) {
            assertp->v3warn(E_UNSUPPORTED,
                            "Unsupported: property instance inside a multi-cycle property "
                            "expression");
            VL_DO_DANGLING(धकेलो_Deletep(assertp->unlinkFrBack()), assertp);
            वापस nullptr;
        }
        अगर (is_Bare_शीर्ष_Level_Until(propp)) वापस nullptr;
        वापस propp;
    }

    व्योम processAssertion(Ast_गाँठ_Cover_Or_Assert* assertp) {
        अगर (assertp->immediate()) वापस;

        AstPropSpec* स्थिर propp = prepareAssertionProp(assertp);
        अगर (!propp) वापस;

        PropertyParts parts = decomposeProperty(propp);
        UASSERT_OBJ(parts.seqExprp, propp, "Property body must be an expression");

        // Unwrap `not` (IEEE 1800-2023 16.12.1); odd count -> negated semantics.
        Ast_गाँठ_व्यंजक* seqBodyp = parts.seqExprp;
        bool negated = false;
        जबतक (AstLogNot* स्थिर notp = VN_CAST(seqBodyp, LogNot)) {
            negated = !negated;
            seqBodyp = notp->lhsp();
        }

        स्थिर अक्षर* स्थिर propertyControlp
            = unsupportedPropertyControl(assertp, seqBodyp, negated);

        // Substitute property-local match-item refs in consequent with
        // $past(rhs, K) before NFA build (IEEE 1800-2023 16.10).
        अगर (liftMatchItemSubstitutions(parts, seqBodyp)) {
            AstPropSpec* स्थिर psp = VN_CAST(assertp->propp(), PropSpec);
            UASSERT_OBJ(psp, assertp, "Concurrent assertion must have PropSpec");
            replaceBodyOnBuildError(assertp->fileline(), psp, /*errorEmitted=*/true);
            वापस;
        }

        AstCover* स्थिर coverp = VN_CAST(assertp, Cover);
        स्थिर bool isCoverSeq = coverp && coverp->isCoverSeq();
        // A sequence event control is not an assertion directive; no default
        // disable iff, no assertion control
        स्थिर bool isSeqEvent = coverp && coverp->isSeqEvent();
        स्थिर bool गिनती_Matches = isCoverSeq && !isSeqEvent;
        // Inherit module defaults (IEEE 14.12, 16.15) when assertion has none.
        अगर (!propp->sensesp() && m_defaultClockingp) {
            propp->sensesp(m_defaultClockingp->sentreep()->sensesp()->clone_वृक्ष(true));
        }
        अगर (!propp->disablep() && m_defaultDisablep && !isSeqEvent) {
            propp->disablep(m_defaultDisablep->condp()->clone_वृक्ष_Pure(true));
        }
        अगर (!propp->sensesp()) वापस;
        Ast_Sen_वृक्ष* senTreep
            = नया Ast_Sen_वृक्ष{propp->fileline(), propp->sensesp()->clone_वृक्ष(true)};
        Ast_गाँठ_व्यंजक* स्थिर disableExprp = propp->disablep();

        // NFA lowering clones repeated operands and may hoist them into an
        // always_comb block. Resolve implicit sampled-value clocks first, while
        // the enclosing assertion clock is still available.
        {
            VL_RESTORER(m_sampled_मूल्य_Clockp);
            m_sampled_मूल्य_Clockp = senTreep;
            iterate(propp->propp());
        }

        File_पंक्ति* स्थिर flp = assertp->fileline();

        SvaGraph graph;
        SvaNfaBuilder builder{graph,      m_modp,     m_prop_अस्थायी_Names,
                              isCoverSeq, isSeqEvent, coverp != nullptr};

        स्थिर Build_परिणाम परिणाम = buildAssertionGraph(builder, graph, seqBodyp, parts, flp);
        अगर (परिणाम.valid()) wireMatchAndMidSources(graph, परिणाम, flp);
        अगर (!परिणाम.valid()) {
            // Fall through to V3AssertPre for unsupported constructs; only
            // replace the body on real semantic errors. Any hoisted temps
            // from this attempt become orphan MODULETEMPs; V3Dead removes
            // them along with the dead always_comb driver.
            replaceBodyOnBuildError(flp, propp, परिणाम.errorEmitted);
            VL_DO_DANGLING(धकेलो_Deletep(senTreep), senTreep);
            वापस;
        }
        // After the build, so a construct the builder rejects reports itself.
        अगर (propertyControlp) {
            seqBodyp->v3warn(E_UNSUPPORTED,
                             "Unsupported: temporal property if/case with " << propertyControlp);
            replaceBodyOnBuildError(flp, propp, /*errorEmitted=*/true);
            VL_DO_DANGLING(धकेलो_Deletep(senTreep), senTreep);
            वापस;
        }
        अगर (गिनती_Matches && !v3Global.opt.coverageUser()) {
            SvaNfaBuilder::freeUnlinkedCondp(परिणाम.finalCondp);
            VL_DO_DANGLING(धकेलो_Deletep(senTreep), senTreep);
            Ast_गाँठ* स्थिर innerPropp = propp->propp();
            innerPropp->replaceWith(नया AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(धकेलो_Deletep(innerPropp), innerPropp);
            वापस;
        }

        AstAssert* स्थिर assertAssertp = VN_CAST(assertp, Assert);
        स्थिर bool splitImplicationPasssp = assertAssertp && assertAssertp->passsp()
                                            && parts.hasImplication
                                            && canSplitImplicationPassActions(parts);
        स्थिर bool needMatch = assertAssertp && assertAssertp->passsp()
                               && (!parts.hasImplication || splitImplicationPasssp);

        स्थिर bool needsThreadFailReplay
            = assertAssertp && assertAssertp->failsp() && !parts.hasImplication;
        स्थिर स्वतः signals
            = m_loweringp->lower(assertp, graph, senTreep, परिणाम.finalCondp, disableExprp,
                                 needsThreadFailReplay, needsThreadFailReplay && !negated);
        Ast_गाँठ_व्यंजक* matchExprp = nullptr;
        Ast_गाँठ_व्यंजक* outputExprp = m_loweringp->assemble_परिणाम(
            assertp, negated, परिणाम.finalCondp, signals, needMatch ? &matchExprp : nullptr);
        अगर (गिनती_Matches) {
            UASSERT_OBJ(signals.matchCountp, coverp, "Cover sequence missing match count");
            VL_DO_DANGLING(outputExprp->delete_वृक्ष(), outputExprp);
            propp->matchCountp(signals.matchCountp);
            outputExprp = नया AstNeq{flp, signals.matchCountp->clone_वृक्ष_Pure(false),
                                     newTypedConstp(flp, signals.matchCountp->dtypep(), 0)};
        }

        Ast_Sen_वृक्ष* स्थिर threadFailReplaySenTreep
            = signals.threadFailCountp ? senTreep->clone_वृक्ष(false) : nullptr;

        VL_DO_DANGLING(धकेलो_Deletep(senTreep), senTreep);
        अगर (परिणाम.finalCondp && !परिणाम.finalCondp->backp()) धकेलो_Deletep(परिणाम.finalCondp);

        अगर (splitImplicationPasssp) {
            splitImplicationPassActions(assertAssertp, parts, matchExprp);
        } अन्यथा {
            attachActionHandlers(assertAssertp, matchExprp, threadFailReplaySenTreep,
                                 signals.threadFailCountp);
        }

        Ast_गाँठ* स्थिर innerPropp = propp->propp();
        innerPropp->replaceWith(outputExprp);
        VL_DO_DANGLING(धकेलो_Deletep(innerPropp), innerPropp);

        UINFO(4, "NFA converted assertion at " << flp << प_पूर्ण);

        अगर (dumpGraphLevel() >= 6) graph.m_graph.dumpDotFilePrefixed("assert-nfa");
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_loweringp);
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultDisablep);
        m_modp = nodep;
        // A class nested in a module is in the scope of the module's default clocking
        अगर (!VN_IS(nodep, Class)) m_defaultClockingp = nullptr;
        m_defaultDisablep = nodep->defaultDisablep();
        SvaNfaLowering lowering{nodep};
        m_loweringp = &lowering;
        iterateChildren(nodep);
    }
    व्योम visit(AstClocking* nodep) override {
        अगर (nodep->isDefault() && !m_defaultClockingp) m_defaultClockingp = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    व्योम visit(AstDefaultDisable* nodep) override {}
    व्योम visit(AstFell* nodep) override { visit_Sampled_मूल्य(nodep); }
    व्योम visit(AstPast* nodep) override { visit_Sampled_मूल्य(nodep); }
    व्योम visit(AstRose* nodep) override { visit_Sampled_मूल्य(nodep); }
    व्योम visit(AstStable* nodep) override { visit_Sampled_मूल्य(nodep); }
    व्योम visit(AstAssert* nodep) override { processAssertion(nodep); }
    व्योम visit(AstCover* nodep) override { processAssertion(nodep); }
    व्योम visit(AstRestrict* nodep) override {
        // Restrict property is ignored by simulators (IEEE 1800-2023 16.12.2).
        // Remove here so temporal SExpr don't leak to V3AssertPre.
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }
    व्योम visit(AstAssertIntrinsic* nodep) override {}
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    explicit AssertNfaVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Top entry point

व्योम V3AssertNfa::assertNfaAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":" << प_पूर्ण);
    { AssertNfaVisitor{nodep}; }
    V3Global::dump_Check_Global_वृक्ष("assertnfa", 0, dump_वृक्ष_Either_Level() >= 3);
}
