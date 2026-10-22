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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3AssertNfa.h"

#અન્તર્ભાવ "V3Assert.h"
#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3Graph.h"
#અન્તર્ભાવ "V3Stats.h"
#અન્તર્ભાવ "V3Task.h"
#અન્તર્ભાવ "V3UniqueNames.h"

#અન્તર્ભાવ <set>
#અન્તર્ભાવ <unordered_set>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// NFA Graph Data Structures (V3Graph-derived per upstream convention)

નામાકાશ {

વર્ગ SvaStateVertex;

// Per-vertex algorithm data, stored via V3GraphVertex::userp() during lowering
સંરચના SvaVertexData final {
    AstVar* stateVarp = nullptr;  // NBA state register for this vertex
    AstVar* delayRingVarp = nullptr;  // Packed occupancy bits or per-slot match counts
    AstVar* delayRingHeadVarp
        = nullptr;  // Cached outgoing slot; isolates readers from in-place writes
    AstVar* delayRingIdxVarp = nullptr;  // Next slot written in delayRingVarp
    AstVar* delay_Ring_Live_ગણતરી_Varp = nullptr;  // Number of threads in delayRingVarp
    AstVar* delayRingWrappedVarp = nullptr;  // All slots written since the last clear
    AstVar* doneLVarp = nullptr;  // SAnd LHS done-latch
    AstVar* doneRVarp = nullptr;  // SAnd RHS done-latch
    Ast_ગાંઠ_પદાવલિ* stateSigp = nullptr;  // Combinational state signal; OWNED during lowering
    Ast_ગાંઠ_પદાવલિ* state_ગણતરી_Sigp = nullptr;  // Cover-sequence match count; OWNED during lowering
    Ast_ગાંઠ_પદાવલિ* state_ગણતરી_Overflow_Sigp = nullptr;  // Count overflow; OWNED during lowering
    તાર્કિક delayRingCounts = ખોટું;  // Ring slots hold cover-sequence match counts
    તાર્કિક needsReg = ખોટું;  // True if vertex has incoming clocked edge
};

// NFA state vertex -- one per NFA position in the sequence evaluation
વર્ગ SvaStateVertex final : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(SvaStateVertex, V3GraphVertex)
જાહેર:
    // True if this is the sequence-match terminal vertex
    તાર્કિક m_isMatch = ખોટું;
    // OWNED throughout-guard condition clones; IEEE 1800-2023 16.9.9
    std::vector<Ast_ગાંઠ_પદાવલિ*> m_throughoutConds;
    // Nonzero for a ring-buffer vertex for ## delays.
    તાર્કિક m_isFixedDelayRing = ખોટું;
    unsigned m_delayRingSize = 0;  // Number of ring slots. Range: max-min+1.
    Ast_ગાંઠ_પદાવલિ* m_delayRingClearCondp = nullptr;  // local RHS for pure-boolean range
    Ast_ગાંઠ_પદાવલિ* m_delayRingAdvanceCondp = nullptr;  // Advance only when this condition holds
    તાર્કિક m_replayAbortReject = ખોટું;  // Compressed repetition needs per-thread abort replay
    // OWNED; enclosing-abort fire condition clearing state or suppressing guard rejection
    Ast_ગાંઠ_પદાવલિ* m_abortClearp = nullptr;
    // OWNED; reject-abort fire condition rejecting all represented live threads
    Ast_ગાંઠ_પદાવલિ* m_abortRejectp = nullptr;
    // Liveness terminal (IEEE weak semantics): reject must not fire from this source
    તાર્કિક m_isUnbounded = ખોટું;
    // Temporal sequence AND combiner; IEEE 1800-2023 16.9.5
    તાર્કિક m_isAndCombiner = ખોટું;
    SvaStateVertex* m_andLhsTermp = nullptr;  // LHS sub-NFA terminal vertex
    SvaStateVertex* m_andRhsTermp = nullptr;  // RHS sub-NFA terminal vertex
    Ast_ગાંઠ_પદાવલિ* m_andLhsCondp = nullptr;  // OWNED; LHS final condition (may be nullptr)
    Ast_ગાંઠ_પદાવલિ* m_andRhsCondp = nullptr;  // OWNED; RHS final condition (may be nullptr)
    // Reject sink for SAnd rejectOnFail wiring; not a state-signal source
    તાર્કિક m_isRejectSink = ખોટું;
    // In-window vertex of a strong s_always[m:n]: if its state is still set at
    // end-of-simulation the universal-quantifier window never completed, which is
    // a liveness failure (IEEE 1800-2023 16.12.11 strong semantics).
    તાર્કિક m_strongPending = ખોટું;
    // Ring introduced for the checked portion of a bounded strong s_always window.
    તાર્કિક m_strongAlwaysRing = ખોટું;

    // CONSTRUCTORS
    explicit SvaStateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~SvaStateVertex() override {
        માટે (Ast_ગાંઠ_પદાવલિ* cp : m_throughoutConds) VL_DO_DANGLING(cp->delete_વૃક્ષ(), cp);
        જો (m_delayRingClearCondp)
            VL_DO_DANGLING(m_delayRingClearCondp->delete_વૃક્ષ(), m_delayRingClearCondp);
        જો (m_delayRingAdvanceCondp)
            VL_DO_DANGLING(m_delayRingAdvanceCondp->delete_વૃક્ષ(), m_delayRingAdvanceCondp);
        જો (m_abortClearp) VL_DO_DANGLING(m_abortClearp->delete_વૃક્ષ(), m_abortClearp);
        જો (m_abortRejectp) VL_DO_DANGLING(m_abortRejectp->delete_વૃક્ષ(), m_abortRejectp);
        જો (m_andLhsCondp) VL_DO_DANGLING(m_andLhsCondp->delete_વૃક્ષ(), m_andLhsCondp);
        જો (m_andRhsCondp) VL_DO_DANGLING(m_andRhsCondp->delete_વૃક્ષ(), m_andRhsCondp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string name() અચલ override {
        string name = "s" + cvtToStr(color());
        જો (m_delayRingSize) {
            name += "\\n";
            name += m_isFixedDelayRing ? "fixed chain " : "range chain ";
            name += cvtToStr(m_delayRingSize) + " slots";
        }
        પાછા name;
    }
    string dotColor() અચલ override {
        જો (m_isMatch) પાછા "red";
        જો (m_delayRingSize) પાછા "blue";
        જો (m_isAndCombiner) પાછા "purple";
        પાછા "black";
    }
    // LCOV_EXCL_STOP
    // Access per-vertex algorithm data (valid only during lowering phase)
    SvaVertexData* datap() અચલ { પાછા static_cast<SvaVertexData*>(userp()); }
};

// NFA transition edge -- clocked (##1) or combinational link (##0)
વર્ગ SvaTransEdge final : જાહેર V3GraphEdge {
    VL_RTTI_IMPL(SvaTransEdge, V3GraphEdge)
જાહેર:
    Ast_ગાંઠ_પદાવલિ* m_condp;  // Transition condition; nullptr = unconditional; OWNED
    તાર્કિક m_consumesCycle;  // true = clocked edge (##1), false = link (##0/boolean)
    // Reject when source is active and condp is false; set only on
    // outermost required-step Link
    તાર્કિક m_rejectOnFail = ખોટું;
    // Optional dynamic condition vertex for m_rejectOnFail. Used when the
    // success condition is another NFA state rather than a static expression.
    SvaStateVertex* m_condVtxp = nullptr;

    // CONSTRUCTORS
    SvaTransEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ટોચ, Ast_ગાંઠ_પદાવલિ* condp,
                 તાર્કિક consumesCycle)
        : V3GraphEdge{graphp, fromp, ટોચ, /*weight=*/1}
        , m_condp{condp}
        , m_consumesCycle{consumesCycle} {}
    ~SvaTransEdge() override {
        જો (m_condp) VL_DO_DANGLING(m_condp->delete_વૃક્ષ(), m_condp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string dotLabel() અચલ override { પાછા m_consumesCycle ? "##1" : "link"; }
    string dotStyle() અચલ override { પાછા m_consumesCycle ? "" : "dashed"; }
    // LCOV_EXCL_STOP
    // Typed accessors for NFA vertices
    SvaStateVertex* fromVtxp() અચલ { પાછા static_cast<SvaStateVertex*>(fromp()); }
    SvaStateVertex* toVtxp() અચલ { પાછા static_cast<SvaStateVertex*>(ટોચ()); }
};

// NFA graph container
વર્ગ SvaGraph final {
જાહેર:
    V3Graph m_graph;  // Owns all vertices and edges
    SvaStateVertex* m_startVertexp = nullptr;  // Trigger/start vertex
    SvaStateVertex* m_matchVertexp = nullptr;  // Sequence-match terminal vertex

    // Create a new state vertex
    SvaStateVertex* createStateVertex() { પાછા નવું SvaStateVertex{&m_graph}; }
    // Create the match terminal vertex
    SvaStateVertex* createMatchVertex() {
        SvaStateVertex* અચલ vtxp = createStateVertex();
        vtxp->m_isMatch = સાચું;
        m_matchVertexp = vtxp;
        પાછા vtxp;
    }
    // Add a clocked transition edge (##1)
    SvaTransEdge* addClockedEdge(SvaStateVertex* fromp, SvaStateVertex* ટોચ,
                                 Ast_ગાંઠ_પદાવલિ* condp = nullptr) {
        પાછા નવું SvaTransEdge{&m_graph, fromp, ટોચ, condp, /*consumesCycle=*/સાચું};
    }
    // Add a combinational link (##0 / boolean condition)
    SvaTransEdge* addLink(SvaStateVertex* fromp, SvaStateVertex* ટોચ,
                          Ast_ગાંઠ_પદાવલિ* condp = nullptr) {
        પાછા નવું SvaTransEdge{&m_graph, fromp, ટોચ, condp, /*consumesCycle=*/ખોટું};
    }
    // Collect all edges into a flat vector for iteration.
    // Used by the lowering phase which needs global edge scans.
    std::vector<અચલ SvaTransEdge*> allEdges() અચલ {
        std::vector<અચલ SvaTransEdge*> result;
        માટે (અચલ V3GraphVertex& vtxr : m_graph.vertices()) {
            માટે (અચલ V3GraphEdge& edger : vtxr.outEdges()) {
                result.ધકેલો_back(static_cast<અચલ SvaTransEdge*>(&edger));
            }
        }
        પાછા result;
    }
};

//######################################################################
// Builder result: terminal vertex + optional final condition (match Link condition).
સંરચના BuildResult final {
    SvaStateVertex* termVertexp;  // Primary terminal; contributes to both match and reject
    Ast_ગાંઠ_પદાવલિ* finalCondp;  // nullptr = unconditional
    // Mid-window sources for range delays (pure boolean RHS): match-only (isUnbounded)
    std::vector<SvaStateVertex*> midSources;
    તાર્કિક errorEmitted = ખોટું;  // Builder already emitted specific error; skip generic
    // For cover_sequence: when true, midSources already enumerate every
    // end-of-match, so wireMatchAndMidSources must NOT add the main
    // termVtxp -> matchVertex Link (would double-count via the merge vertex).
    તાર્કિક termIsMidMerge = ખોટું;
    તાર્કિક valid() અચલ { પાછા termVertexp != nullptr; }
    સ્થિર BuildResult fail(તાર્કિક errored = ખોટું) { પાછા {nullptr, nullptr, {}, errored}; }
    સ્થિર BuildResult failWithError() { પાછા {nullptr, nullptr, {}, સાચું}; }
};

// Parser-marked SAnd of overlapped implications: a property if/else/case.
સ્થિર તાર્કિક hasPropertyControlConjunction(અચલ Ast_ગાંઠ_પદાવલિ* nodep) {
    પાછા nodep->exists([](અચલ AstSAnd* andp) { પાછા andp->propertyControl(); });
}

સ્થિર AstConst* newTypedConstp(File_લીટી* અચલ flp, અચલ Ast_ગાંઠ_DType* અચલ dtypep,
                                અચલ uint32_t કિંમત) {
    AstConst* અચલ constp = નવું AstConst{flp, AstConst::DTyped{}, dtypep};
    constp->અંક().setLong(કિંમત);
    પાછા constp;
}

સ્થિર Ast_ગાંઠ_પદાવલિ* sampled(Ast_ગાંઠ_પદાવલિ* exprp) {
    પાછા નવું AstSampled{exprp->fileline(), exprp, exprp->dtypep(), સાચું};
}

સ્થિર અચલ અક્ષર* assertPassOnQuery(તાર્કિક vacuous) {
    સ્થિર constexpr અચલ અક્ષર* queries[2]
        = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
    પાછા queries[vacuous];
}

સ્થિર Ast_ગાંઠ_પદાવલિ* assertOnCond(File_લીટી* flp, VAssertType type,
                                 VAssertDirectiveType directiveType) {
    જો (!v3Global.opt.assertOn()) { પાછા નવું AstConst{flp, AstConst::BitFalse{}}; }
    પાછા નવું AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
}

સ્થિર Ast_ગાંઠ_પદાવલિ* assertKillGet(File_લીટી* flp, VAssertType type,
                                  VAssertDirectiveType directiveType) {
    પાછા નવું AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_KILL", type, directiveType),
                        32};
}

સ્થિર string assertActionControlPrefix(VAssertDirectiveType directiveType) {
    અચલ પૂર્ણાંક controlled = !!(static_cast<પૂર્ણાંક>(directiveType)
                              & (static_cast<પૂર્ણાંક>(VAssertDirectiveType::ASSERT)
                                 | static_cast<પૂર્ણાંક>(VAssertDirectiveType::COVER)
                                 | static_cast<પૂર્ણાંક>(VAssertDirectiveType::ASSUME)));
    અચલ પૂર્ણાંક checkRuntime = controlled & static_cast<પૂર્ણાંક>(v3Global.opt.assertOn());
    પાછા "("s + std::to_string(controlled ^ 1) + " || ("s + std::to_string(checkRuntime)
           + " && "s;
}

સ્થિર Ast_ગાંઠ_પદાવલિ* assertPassOnCond(File_લીટી* flp, VAssertType type,
                                     VAssertDirectiveType directiveType, તાર્કિક vacuous) {
    પાછા નવું AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType)
            + "))"s,
        1};
}

સ્થિર Ast_ગાંઠ_પદાવલિ* assertFailOnCond(File_લીટી* flp, VAssertType type,
                                     VAssertDirectiveType directiveType) {
    પાછા નવું AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType) + "))"s,
        1};
}

સ્થિર AstIf* newPassOnIf(File_લીટી* flp, Ast_ગાંઠ_પદાવલિ* firep, Ast_ગાંઠ* bodyp, VAssertType type,
                          VAssertDirectiveType directiveType, તાર્કિક vacuous) {
    Ast_ગાંઠ_પદાવલિ* અચલ condp
        = નવું AstLogAnd{flp, firep, assertPassOnCond(flp, type, directiveType, vacuous)};
    AstIf* અચલ ifp = નવું AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(સાચું);
    ifp->user1(સાચું);
    પાછા ifp;
}

સ્થિર Ast_ગાંઠ_Stmt* newIfAssertFailOn(Ast_ગાંઠ* bodyp, VAssertDirectiveType directiveType,
                                      VAssertType type) {
    File_લીટી* અચલ flp = bodyp->fileline();
    Ast_ગાંઠ_પદાવલિ* અચલ condp = assertFailOnCond(flp, type, directiveType);
    AstIf* અચલ ifp = નવું AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(સાચું);
    ifp->user1(સાચું);
    પાછા ifp;
}

//######################################################################
// NFA Builder

વર્ગ SvaNfaBuilder final {
    SvaGraph& m_graph;  // NFA graph being built
    Ast_ગાંઠ_Module* અચલ m_modp;  // Module to receive hoisted sampled-prop temps
    V3UniqueNames& m_propTempNames;  // Module-shared temp-var name source
    std::vector<Ast_ગાંઠ_પદાવલિ*> m_temporal_Guard_ઢગલો;  // Guards active across nested temporal states
    // Outer abort conditions, AND-ed as !cond into inner abort edges
    // (IEEE 1800-2023 16.12.14 outer-wraps-inner).
    std::vector<Ast_ગાંઠ_પદાવલિ*> m_outer_Abort_ઢગલો;
    તાર્કિક m_inUnboundedScope = ખોટું;  // Sticky: nodes created after inherit liveness
    તાર્કિક m_markStrongPending = ખોટું;  // Mark new vertices as strong s_always in-window
    તાર્કિક m_isCover = ખોટું;  // Cover directives do not fail at end-of-simulation
    // IEEE 1800-2023 16.14.3 cover sequence: each end-of-match fires the action,
    // not just the first. Builder builds parallel-branch (no first-match-wins)
    // topology when true. Default false preserves cover_property semantics.
    તાર્કિક m_isCoverSeq = ખોટું;
    // Unsupported endpoint topology must reject, not ignore, or the wait hangs
    તાર્કિક m_isSeqEvent = ખોટું;
    તાર્કિક m_inSequencePrefix = ખોટું;  // Prefix composition does not preserve every endpoint
    તાર્કિક m_wantBoolLeaf = ખોટું;  // Building a |-> / |=> or top-level body (see buildOrMerge)

    સંરચના RangeDelayRejectInfo final {
        SvaStateVertex* startp = nullptr;
        unsigned range = 0;
        પૂર્ણાંક rhsLen = 0;
    };

    શૂન્ય warnEndpointUnsupported(File_લીટી* flp, અચલ string& what) અચલ {
        જો (m_isSeqEvent) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: sequence used as an event control with " << what);
        } નહીંતર {
            flp->v3warn(COVERIGN, "Ignoring unsupported: cover sequence with " << what);
        }
    }

    Ast_ગાંઠ_પદાવલિ* throughoutCond(Ast_ગાંઠ_પદાવલિ* baseCondp, File_લીટી* flp) {
        જો (m_temporal_Guard_ઢગલો.empty()) પાછા baseCondp;
        // AND all active temporal guards (supports nesting)
        // Each must use $sampled values.
        Ast_ગાંઠ_પદાવલિ* guardp = nullptr;
        માટે (Ast_ગાંઠ_પદાવલિ* અચલ condp : m_temporal_Guard_ઢગલો) {
            Ast_ગાંઠ_પદાવલિ* અચલ clonep = sampled(condp->clone_વૃક્ષ_Pure(ખોટું));
            જો (!guardp) {
                guardp = clonep;
            } નહીંતર {
                guardp = નવું AstLogAnd{flp, guardp, clonep};
            }
        }
        જો (baseCondp) { guardp = નવું AstLogAnd{flp, baseCondp, guardp}; }
        પાછા guardp;
    }

    સ્થિર unsigned getConstUInt(Ast_ગાંઠ_પદાવલિ* exprp) {
        Ast_ગાંઠ_પદાવલિ* અચલ constp = V3Const::constifyEdit(exprp->clone_વૃક્ષ_Pure(ખોટું));
        અચલ AstConst* અચલ cp = VN_CAST(constp, Const);
        અચલ unsigned val = cp ? cp->toUInt() : 0;
        VL_DO_DANGLING(constp->delete_વૃક્ષ(), constp);
        પાછા val;
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
    સ્થિર પૂર્ણાંક fixedLength(Ast_ગાંઠ_પદાવલિ* nodep) {
        જો (AstSExpr* અચલ sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* અચલ delayp = VN_CAST(sexprp->delayp(), Delay);
            જો (!delayp || !delayp->isCycleDelay()) પાછા -1;
            unsigned delayCycles;
            જો (delayp->isRangeDelay()) {
                જો (delayp->isUnbounded()) પાછા -1;  // LCOV_EXCL_LINE
                અચલ unsigned minD = getConstUInt(delayp->lhsp());
                અચલ unsigned maxD = getConstUInt(delayp->rhsp());
                જો (minD != maxD) પાછા -1;
                delayCycles = minD;
            } નહીંતર {
                delayCycles = getConstUInt(delayp->lhsp());
            }
            પૂર્ણાંક preLen = 0;
            જો (Ast_ગાંઠ_પદાવલિ* અચલ prep = sexprp->preExprp()) {
                preLen = fixedLength(prep);
                જો (preLen < 0) પાછા -1;  // LCOV_EXCL_LINE
            }
            અચલ પૂર્ણાંક bodyLen = fixedLength(sexprp->exprp());
            જો (bodyLen < 0) પાછા -1;  // LCOV_EXCL_LINE
            પાછા preLen + delayCycles + bodyLen;
        }
        જો (AstSThroughout* અચલ throughp = VN_CAST(nodep, SThroughout)) {
            પાછા fixedLength(throughp->rhsp());
        }
        જો (AstSOr* અચલ orp = VN_CAST(nodep, SOr)) {
            // Alternatives must share one end cycle; buildSWithin relies on
            // this to pair the OR with an SIntersect.
            અચલ પૂર્ણાંક lhsLen = fixedLength(orp->lhsp());
            અચલ પૂર્ણાંક rhsLen = fixedLength(orp->rhsp());
            જો (lhsLen < 0 || rhsLen < 0 || lhsLen != rhsLen) પાછા -1;
            પાછા lhsLen;
        }
        જો (AstSWithin* અચલ withinp = VN_CAST(nodep, SWithin)) {
            // `seq1 within seq2` ends at seq2's end cycle (IEEE 16.9.10).
            અચલ પૂર્ણાંક lhsLen = fixedLength(withinp->lhsp());
            અચલ પૂર્ણાંક rhsLen = fixedLength(withinp->rhsp());
            જો (lhsLen < 0 || rhsLen < 0 || lhsLen > rhsLen) પાછા -1;
            પાછા rhsLen;
        }
        // LCOV_EXCL_START -- defensive: V3AssertPre rejects composite SVA ops
        // nested in an intersect arm before fixedLength runs (clock-context
        // resolution fails). Kept as a guard in case future parser relaxations
        // permit it.
        જો (nodep->isMultiCycleSva()) પાછા -1;
        // LCOV_EXCL_STOP
        // Plain boolean expression (no SVA constructs) -- 0 cycles.
        પાછા 0;
    }

    // Contiguous match-length range [lo,hi] for an operand whose length varies
    // from AT MOST ONE ranged cycle delay; {-1,-1} otherwise. Drives the
    // variable-length `intersect` lowering (IEEE 1800-2023 16.9.6): more than
    // one ranged delay would make the per-length realization ambiguous, so it
    // is reported unsupported rather than mis-paired.
    સ્થિર std::pair<પૂર્ણાંક, પૂર્ણાંક> lengthRange(Ast_ગાંઠ_પદાવલિ* nodep) {
        જો (AstSExpr* અચલ sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* અચલ delayp = VN_CAST(sexprp->delayp(), Delay);
            જો (!delayp || !delayp->isCycleDelay()) પાછા {-1, -1};
            std::pair<પૂર્ણાંક, પૂર્ણાંક> delayRange;
            જો (delayp->isRangeDelay()) {
                જો (delayp->isUnbounded()) પાછા {-1, -1};
                અચલ unsigned minD = getConstUInt(delayp->lhsp());
                અચલ unsigned maxD = getConstUInt(delayp->rhsp());
                delayRange = {minD, maxD};
            } નહીંતર {
                અચલ unsigned d = getConstUInt(delayp->lhsp());
                delayRange = {d, d};
            }
            std::pair<પૂર્ણાંક, પૂર્ણાંક> preRange{0, 0};
            જો (Ast_ગાંઠ_પદાવલિ* અચલ prep = sexprp->preExprp()) {
                preRange = lengthRange(prep);
                જો (preRange.first < 0) પાછા {-1, -1};
            }
            અચલ std::pair<પૂર્ણાંક, પૂર્ણાંક> bodyRange = lengthRange(sexprp->exprp());
            જો (bodyRange.first < 0) પાછા {-1, -1};
            અચલ પૂર્ણાંક variableParts = (preRange.first != preRange.second)
                                      + (delayRange.first != delayRange.second)
                                      + (bodyRange.first != bodyRange.second);
            જો (variableParts > 1) પાછા {-1, -1};
            પાછા {preRange.first + delayRange.first + bodyRange.first,
                    preRange.second + delayRange.second + bodyRange.second};
        }
        જો (AstSThroughout* અચલ throughp = VN_CAST(nodep, SThroughout)) {
            પાછા lengthRange(throughp->rhsp());
        }
        જો (nodep->isMultiCycleSva()) પાછા {-1, -1};
        પાછા {0, 0};  // plain boolean -- 0 cycles
    }

    // Clone `operand` with its sole variable ranged cycle delay pinned so the
    // total match length is exactly `len`. `lo` is the operand's minimum length
    // (lengthRange().first). A fixed operand has no such delay and is returned
    // as a plain clone (callers only request its single achievable length).
    સ્થિર Ast_ગાંઠ_પદાવલિ* realizeAtLength(Ast_ગાંઠ_પદાવલિ* operand, પૂર્ણાંક len, પૂર્ણાંક lo) {
        Ast_ગાંઠ_પદાવલિ* અચલ clonep = operand->clone_વૃક્ષ_Pure(ખોટું);
        AstDelay* rangeDelayp = nullptr;
        clonep->foreach([&](AstDelay* dp) {
            જો (!rangeDelayp && dp->isRangeDelay() && !dp->isUnbounded()
                && getConstUInt(dp->lhsp()) != getConstUInt(dp->rhsp())) {
                rangeDelayp = dp;
            }
        });
        જો (rangeDelayp) {
            File_લીટી* અચલ flp = rangeDelayp->fileline();
            અચલ unsigned pinned = getConstUInt(rangeDelayp->lhsp()) + (len - lo);
            Ast_ગાંઠ_પદાવલિ* અચલ oldMinp = rangeDelayp->lhsp();
            oldMinp->replaceWith(નવું AstConst{flp, pinned});
            VL_DO_DANGLING(oldMinp->delete_વૃક્ષ(), oldMinp);
            // Drop the max bound so it lowers as a fixed `##d`, not `##[d:d]`.
            Ast_ગાંઠ* અચલ oldMaxp = rangeDelayp->rhsp()->unlinkFrBack();
            VL_DO_DANGLING(oldMaxp->delete_વૃક્ષ(), oldMaxp);
        }
        પાછા clonep;
    }

    // Cuts AST size from O(N * sizeof(exprp)) to O(N) + O(sizeof(exprp)) by
    // sharing a single `VarRef` across N check edges. Hoist also matches the
    // IEEE 1800-2023 16.9.9 "single preponed-region snapshot" semantic for
    // any exprp -- even an impure one would now evaluate exactly once per
    // clock instead of N times. Orphan temps from failed builds are unused
    // MODULETEMPs and are removed by V3Dead.
    AstVar* tryHoistSampled(Ast_ગાંઠ_પદાવલિ* exprp, File_લીટી* flp, unsigned clone_ગણતરી) {
        constexpr unsigned kHoistThreshold = 2;
        જો (clone_ગણતરી < kHoistThreshold) પાછા nullptr;
        AstVar* અચલ tempVarp
            = નવું AstVar{flp, VVarType::MODULETEMP, m_propTempNames.get(exprp), exprp->dtypep()};
        m_modp->addStmtsp(tempVarp);
        AstAssign* અચલ assignp = નવું AstAssign{flp, નવું AstVarRef{flp, tempVarp, VAccess::WRITE},
                                                 sampled(exprp->clone_વૃક્ષ_Pure(ખોટું))};
        m_modp->addStmtsp(નવું AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, assignp});
        પાછા tempVarp;
    }

    સ્થિર Ast_ગાંઠ_પદાવલિ* sampledRefOrClone(AstVar* hoistVarp, Ast_ગાંઠ_પદાવલિ* exprp, File_લીટી* flp) {
        જો (hoistVarp) પાછા નવું AstVarRef{flp, hoistVarp, VAccess::READ};
        પાછા sampled(exprp->clone_વૃક્ષ_Pure(ખોટું));
    }

    // Create vertex and inherit temporal guards from the current scope.
    SvaStateVertex* scopedCreateVertex() {
        SvaStateVertex* અચલ vtxp = m_graph.createStateVertex();
        માટે (Ast_ગાંઠ_પદાવલિ* અચલ cp : m_temporal_Guard_ઢગલો) {
            vtxp->m_throughoutConds.ધકેલો_back(cp->clone_વૃક્ષ_Pure(ખોટું));
        }
        જો (m_inUnboundedScope) vtxp->m_isUnbounded = સાચું;
        જો (m_markStrongPending) vtxp->m_strongPending = સાચું;
        પાછા vtxp;
    }

    // AND current temporal guards into every edge/link.
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* ટોચ, Ast_ગાંઠ_પદાવલિ* condp,
                              File_લીટી* flp) {
        પાછા m_graph.addLink(fromp, ટોચ, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* ટોચ, File_લીટી* flp) {
        પાછા m_graph.addLink(fromp, ટોચ, throughoutCond(nullptr, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* ટોચ, Ast_ગાંઠ_પદાવલિ* condp,
                              File_લીટી* flp) {
        પાછા m_graph.addClockedEdge(fromp, ટોચ, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* ટોચ, File_લીટી* flp) {
        પાછા m_graph.addClockedEdge(fromp, ટોચ, throughoutCond(nullptr, flp));
    }

    SvaStateVertex* addDelayChain(SvaStateVertex* startp, unsigned size, File_લીટી* flp,
                                  તાર્કિક isFixed = સાચું, Ast_ગાંઠ_પદાવલિ* clearCondp = nullptr,
                                  Ast_ગાંઠ_પદાવલિ* advanceCondp = nullptr) {
        જો (isFixed && size == 0) પાછા startp;
        UASSERT_OBJ(size > 0, startp, "Delay chain needs at least one slot");
        જો (isFixed && size == 1 && !advanceCondp) {
            SvaStateVertex* અચલ nextp = scopedCreateVertex();
            guardedEdge(startp, nextp, flp);
            પાછા nextp;
        }
        SvaStateVertex* અચલ ringVtxp = scopedCreateVertex();
        ringVtxp->m_isFixedDelayRing = isFixed;
        ringVtxp->m_delayRingSize = size;
        જો (clearCondp) {
            UASSERT_OBJ(!isFixed, startp, "Fixed delay cannot have a clear condition");
            ringVtxp->m_delayRingClearCondp = clearCondp->clone_વૃક્ષ_Pure(ખોટું);
        }
        ringVtxp->m_delayRingAdvanceCondp = advanceCondp;
        જો (isFixed) {
            guardedEdge(startp, ringVtxp, flp);
        } નહીંતર {
            guardedLink(startp, ringVtxp, flp);
        }
        પાછા ringVtxp;
    }

    // Build NFA for an SExpr. finalCond = RHS (not yet added as a vertex).
    // isTopLevelStep: marks outermost required boolean check as rejectOnFail.
    // Apply a range delay `##[M:N]` to currentp. Returns true on success. On
    // failure, emits a diagnostic and returns false.
    તાર્કિક applyRangeDelay(AstSExpr* sexprp, SvaStateVertex*& currentp,
                         std::vector<SvaStateVertex*>& midSources,
                         RangeDelayRejectInfo* rangeRejectInfop = nullptr) {
        File_લીટી* અચલ flp = sexprp->fileline();
        AstDelay* અચલ delayp = VN_AS(sexprp->delayp(), Delay);
        Ast_ગાંઠ_પદાવલિ* અચલ rhsExprp = sexprp->exprp();
        અચલ unsigned minDelay = getConstUInt(delayp->lhsp());
        જો (delayp->isUnbounded()) {
            // `##[M:$]`: wait M cycles, then retain every matured attempt in a
            // one-slot ring while also exposing the first eligible tick.
            currentp = addDelayChain(currentp, minDelay, flp);
            SvaStateVertex* અચલ waitRingp = addDelayChain(currentp, 1, flp, ખોટું);
            // The ring's live count is updated in NBA, so this link retains the previous
            // tick's live attempts alongside new arrivals for the next tick.
            guardedLink(waitRingp, waitRingp, flp);
            SvaStateVertex* અચલ mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(waitRingp, mergeVtxp, flp);
            currentp = mergeVtxp;
            currentp->m_isUnbounded = સાચું;  // Liveness, so no reject.
            m_inUnboundedScope = સાચું;
            પાછા સાચું;
        }
        અચલ unsigned maxDelay = getConstUInt(delayp->rhsp());
        જો (minDelay == maxDelay) {
            currentp = addDelayChain(currentp, minDelay, flp);
            પાછા સાચું;
        }
        અચલ unsigned range = maxDelay - minDelay;
        currentp = addDelayChain(currentp, minDelay, flp);
        અચલ તાર્કિક multiCycleRhs = rhsExprp->isMultiCycleSva();
        અચલ Ast_ગાંઠ_પદાવલિ* અચલ preExprp = sexprp->preExprp();
        // Prefix and nested-sequence composition can lose endpoints or their multiplicity
        // required by cover sequence (IEEE 1800-2023 16.14.3), regardless of the range width.
        જો (m_isCoverSeq
            && (multiCycleRhs || m_inSequencePrefix
                || (preExprp && preExprp->isMultiCycleSva()))) {
            warnEndpointUnsupported(flp, "this ranged cycle delay");
            પાછા ખોટું;
        }
        જો (m_inSequencePrefix) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: Bounded ranged cycle delay in a sequence prefix");
            પાછા ખોટું;
        }
        જો (multiCycleRhs) {
            // Launch a candidate at every eligible tick. Candidate-local misses are not
            // assertion rejects while a later position can still match.
            જો (rangeRejectInfop) {
                અચલ પૂર્ણાંક rhsLen = fixedLength(rhsExprp);
                જો (rhsLen >= 0) *rangeRejectInfop = {currentp, range, rhsLen};
            }
            SvaStateVertex* અચલ tailp = addDelayChain(currentp, range + 1U, flp, ખોટું);
            SvaStateVertex* અચલ mergeVtxp = scopedCreateVertex();
            mergeVtxp->m_isUnbounded = સાચું;
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(tailp, mergeVtxp, flp);
            currentp = mergeVtxp;
            m_inUnboundedScope = સાચું;
        } નહીંતર {
            // The first eligible tick is explicit. The ring retains attempts for the
            // remaining ticks and exposes the final tick for rejection. Only properties
            // clear on success, since cover sequence counts every endpoint.
            midSources.ધકેલો_back(currentp);
            currentp = addDelayChain(currentp, range + 1U, flp, ખોટું,
                                     m_isCoverSeq ? nullptr : rhsExprp);
        }
        પાછા સાચું;
    }

    શૂન્ય addFiniteRangeReject(અચલ RangeDelayRejectInfo& info, અચલ BuildResult& result,
                              File_લીટી* flp) {
        જો (!info.startp) પાછા;

        SvaStateVertex* અચલ expiryVtxp
            = addDelayChain(info.startp, info.range + info.rhsLen, flp);
        SvaStateVertex* અચલ expiryMatchp = scopedCreateVertex();
        std::vector<SvaStateVertex*> sources = result.midSources;
        sources.ધકેલો_back(result.termVertexp);
        માટે (SvaStateVertex* અચલ srcp : sources) {
            Ast_ગાંઠ_પદાવલિ* અચલ condp
                = result.finalCondp ? sampled(result.finalCondp->clone_વૃક્ષ_Pure(ખોટું)) : nullptr;
            SvaStateVertex* અચલ successNowp = scopedCreateVertex();
            guardedLink(srcp, successNowp, condp, flp);
            // Retain a success through the latest deadline it can satisfy.
            SvaStateVertex* અચલ historyp
                = addDelayChain(successNowp, info.range + 1U, flp, ખોટું);
            guardedLink(successNowp, expiryMatchp, flp);
            guardedLink(historyp, expiryMatchp, flp);
        }

        SvaStateVertex* અચલ sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = સાચું;
        SvaTransEdge* અચલ rejectp = m_graph.addLink(expiryVtxp, sinkVtxp);
        rejectp->m_rejectOnFail = સાચું;
        rejectp->m_condVtxp = expiryMatchp;
    }

    BuildResult buildSExpr(AstSExpr* sexprp, SvaStateVertex* entryVtxp,
                           તાર્કિક is_ટોચ_Level_Step = ખોટું) {
        AstDelay* અચલ delayp = VN_CAST(sexprp->delayp(), Delay);
        જો (!delayp || !delayp->isCycleDelay()) પાછા BuildResult::fail();

        File_લીટી* અચલ flp = sexprp->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ exprp = sexprp->exprp();

        // Handle LHS (preExpr)
        SvaStateVertex* currentp = entryVtxp;
        જો (Ast_ગાંઠ_પદાવલિ* અચલ preExprp = sexprp->preExprp()) {
            VL_RESTORER(m_inSequencePrefix);
            m_inSequencePrefix = સાચું;
            અચલ BuildResult pre = build_પદાવલિ(preExprp, currentp, is_ટોચ_Level_Step);
            જો (!pre.valid()) પાછા BuildResult::fail(pre.errorEmitted);  // LCOV_EXCL_LINE
            જો (pre.finalCondp) {
                SvaStateVertex* અચલ condVtxp = scopedCreateVertex();
                SvaTransEdge* અચલ edgep = guardedLink(
                    pre.termVertexp, condVtxp, sampled(pre.finalCondp->clone_વૃક્ષ_Pure(ખોટું)), flp);
                જો (is_ટોચ_Level_Step && !pre.termVertexp->m_isUnbounded && !m_inUnboundedScope) {
                    // Do not mark liveness sources: first boolean check is deferred.
                    edgep->m_rejectOnFail = સાચું;
                }
                freeUnlinkedCondp(pre.finalCondp);
                currentp = condVtxp;
            } નહીંતર {
                currentp = pre.termVertexp;
            }
        }

        // Handle delay
        std::vector<SvaStateVertex*> rangeMidSources;
        RangeDelayRejectInfo rangeRejectInfo;
        અચલ તાર્કિક addRangeReject = is_ટોચ_Level_Step && !m_inUnboundedScope;
        જો (delayp->isRangeDelay()) {
            જો (!applyRangeDelay(sexprp, currentp, rangeMidSources,
                                 addRangeReject ? &rangeRejectInfo : nullptr)) {
                પાછા BuildResult::failWithError();
            }
        } નહીંતર {
            અચલ unsigned delayCycles = getConstUInt(delayp->lhsp());
            currentp = addDelayChain(currentp, delayCycles, flp);
        }

        // Multi-cycle RHS: recurse (only plain boolean is returned as finalCondp).
        જો (exprp->isMultiCycleSva()) {
            અચલ BuildResult result = build_પદાવલિ(exprp, currentp, is_ટોચ_Level_Step);
            જો (result.valid()) addFiniteRangeReject(rangeRejectInfo, result, flp);
            પાછા result;
        }
        પાછા {currentp, exprp, std::move(rangeMidSources)};
    }

    BuildResult buildConsRep(AstSConsRep* repp, SvaStateVertex* entryVtxp,
                             તાર્કિક is_ટોચ_Level_Step = ખોટું) {
        File_લીટી* અચલ flp = repp->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ exprp = repp->exprp();
        // Multi-cycle expr in ConsRep not yet supported; bail to avoid invalid AST.
        જો (exprp->isMultiCycleSva()) {
            repp->v3warn(E_UNSUPPORTED, "Unsupported: multi-cycle sequence expression inside"
                                        " consecutive repetition (IEEE 1800-2023 16.9.2)");
            પાછા BuildResult::failWithError();
        }
        અચલ unsigned minN = getConstUInt(repp->countp());

        // Sum sites across prefix + unbounded/range tail so one hoist covers
        // every check edge of this repetition.
        unsigned totalSites = minN;
        જો (repp->unbounded()) {
            totalSites += 1;
        } નહીંતર જો (repp->maxCountp()) {
            totalSites += getConstUInt(repp->maxCountp()) - minN;
        }
        AstVar* અચલ hoistVarp = tryHoistSampled(exprp, flp, totalSites);

        // Cover-sequence (IEEE 1800-2023 16.14.3): collect each end-of-match
        // position so they all fire the action, not just the merged terminal.
        std::vector<SvaStateVertex*> consMidSources;

        SvaStateVertex* currentp = entryVtxp;
        માટે (unsigned i = 0; i < minN; ++i) {
            // Keep the first repetition explicit, collapse all remaining checks into the ring.
            જો (i == 1) {
                currentp = addDelayChain(currentp, minN - 1, flp);
                currentp->m_delayRingClearCondp
                    = નવું AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                currentp->m_replayAbortReject = સાચું;
                જો (is_ટોચ_Level_Step) {
                    currentp->m_throughoutConds.ધકેલો_back(
                        sampledRefOrClone(hoistVarp, exprp, flp));
                }
                i = minN - 1;
            }
            // Every repetition in the minimum prefix is required.
            SvaStateVertex* અચલ condVtxp = scopedCreateVertex();
            SvaTransEdge* અચલ linkp
                = guardedLink(currentp, condVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            // Only an outermost required step rejects the property. Its first check is explicit,
            // later required checks reject through the ring's throughout guard.
            જો (is_ટોચ_Level_Step && i == 0) linkp->m_rejectOnFail = સાચું;
            currentp = condVtxp;
        }
        // After minN: currentp is the first valid end-of-match position for [*m:n].
        જો (m_isCoverSeq && (repp->unbounded() || repp->maxCountp())) {
            consMidSources.ધકેલો_back(currentp);
        }

        જો (repp->unbounded()) {
            જો (minN == 0) {
                SvaStateVertex* અચલ waitVtxp = scopedCreateVertex();
                guardedEdge(currentp, waitVtxp, flp);
                SvaStateVertex* અચલ checkVtxp = scopedCreateVertex();
                guardedLink(waitVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedEdge(checkVtxp, waitVtxp, flp);
                guardedLink(currentp, checkVtxp, flp);
                currentp = checkVtxp;
            } નહીંતર {
                SvaStateVertex* અચલ loopBackVtxp = scopedCreateVertex();
                guardedEdge(currentp, loopBackVtxp, flp);
                SvaStateVertex* અચલ reCheckVtxp = scopedCreateVertex();
                guardedLink(loopBackVtxp, reCheckVtxp, sampledRefOrClone(hoistVarp, exprp, flp),
                            flp);
                guardedEdge(reCheckVtxp, loopBackVtxp, flp);
                guardedLink(reCheckVtxp, currentp, flp);
            }
            currentp->m_isUnbounded = સાચું;
            m_inUnboundedScope = સાચું;
        } નહીંતર જો (repp->maxCountp()) {
            અચલ unsigned maxN = getConstUInt(repp->maxCountp());
            SvaStateVertex* અચલ mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            unsigned tailMinN = minN;
            SvaStateVertex* tailStartp = currentp;
            જો (minN == 0) {
                // Build the first optional iteration explicitly. Feeding the empty endpoint
                // directly into a range ring would incorrectly keep that match alive.
                SvaStateVertex* અચલ nextVtxp = scopedCreateVertex();
                guardedEdge(currentp, nextVtxp, flp);
                SvaStateVertex* અચલ checkVtxp = scopedCreateVertex();
                guardedLink(nextVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                જો (m_isCoverSeq) consMidSources.ધકેલો_back(checkVtxp);
                tailStartp = checkVtxp;
                tailMinN = 1;
            }
            જો (maxN > tailMinN) {
                // Add tail-ring only if the tail is non-empty.
                SvaStateVertex* અચલ nextVtxp
                    = addDelayChain(tailStartp, maxN - tailMinN + 1, flp, ખોટું);
                nextVtxp->m_delayRingClearCondp
                    = નવું AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                nextVtxp->m_replayAbortReject = સાચું;
                SvaStateVertex* અચલ checkVtxp = scopedCreateVertex();
                guardedLink(nextVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                જો (m_isCoverSeq) consMidSources.ધકેલો_back(checkVtxp);
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
        પાછા res;
    }

    // always[lo:hi] / s_always[lo:hi] (IEEE 1800-2023 16.12.11).
    BuildResult buildPropAlways(AstPropAlways* nodep, SvaStateVertex* entryVtxp,
                                તાર્કિક is_ટોચ_Level_Step = ખોટું) {
        File_લીટી* અચલ flp = nodep->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ propp = nodep->propp();
        અચલ unsigned lo = getConstUInt(nodep->loBoundp());
        જો (VN_IS(nodep->hiBoundp(), Unbounded)) {
            // Weak always [lo:$]: unbounded upper bound (IEEE 1800-2023 16.12.11).
            // p must hold at every clock tick at least lo cycles after the attempt
            // start; those ticks are not required to exist, so there is no
            // end-of-trace obligation (weak). The self-loop keeps the attempt live
            // every cycle; each observed cycle is a safety obligation, so a false p
            // rejects immediately.
            UASSERT_OBJ(!nodep->isStrong(), nodep, "Unbounded always must be weak (V3Width)");
            SvaStateVertex* અચલ livep = addDelayChain(entryVtxp, lo, flp);
            livep->m_isUnbounded = સાચું;
            guardedEdge(livep, livep, flp);  // stay active every subsequent cycle
            SvaStateVertex* અચલ sinkp = m_graph.createStateVertex();
            sinkp->m_isRejectSink = સાચું;
            SvaTransEdge* અચલ rejEdgep
                = guardedLink(livep, sinkp, sampled(propp->clone_વૃક્ષ_Pure(ખોટું)), flp);
            જો (is_ટોચ_Level_Step) rejEdgep->m_rejectOnFail = સાચું;
            પાછા {livep, nullptr, {}};
        }
        અચલ unsigned hi = getConstUInt(nodep->hiBoundp());
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
        SvaStateVertex* અચલ checkp = scopedCreateVertex();
        SvaTransEdge* અચલ linkp
            = guardedLink(currentp, checkp, sampled(propp->clone_વૃક્ષ_Pure(ખોટું)), flp);
        જો (is_ટોચ_Level_Step) linkp->m_rejectOnFail = સાચું;
        currentp = checkp;
        m_temporal_Guard_ઢગલો.ધકેલો_back(propp);
        currentp = addDelayChain(currentp, hi - lo, flp);
        જો (nodep->isStrong() && currentp->m_delayRingSize) currentp->m_strongAlwaysRing = સાચું;
        m_temporal_Guard_ઢગલો.કાઢો_back();
        પાછા {currentp, propp, {}};
    }

    BuildResult buildGotoRep(AstSGotoRep* repp, SvaStateVertex* entryVtxp) {
        File_લીટી* અચલ flp = repp->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ exprp = repp->exprp();
        અચલ unsigned minN = getConstUInt(repp->countp());
        જો (minN == 0) પાછા BuildResult::fail();
        અચલ તાર્કિક hasMax = repp->maxCountp() != nullptr;
        અચલ unsigned maxN = hasMax ? getConstUInt(repp->maxCountp()) : minN;
        જો (m_isCoverSeq) {
            // Several attempts may coalesce in the one-bit wait vertex before reaching the ring,
            // so a cover sequence action block could run too few times.
            warnEndpointUnsupported(flp, "a goto repetition");
            પાછા BuildResult::failWithError();
        }

        AstVar* અચલ hoistVarp = tryHoistSampled(exprp, flp, 2);

        // The first guardedEdge is the ##1 before waiting for a match. In the wait state, false
        // takes the clocked self-loop, while true takes the zero-delay guardedLink on that tick.
        SvaStateVertex* અચલ waitVtxp = scopedCreateVertex();
        guardedEdge(entryVtxp, waitVtxp, flp);
        guardedEdge(waitVtxp, waitVtxp,
                    નવું AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)}, flp);
        SvaStateVertex* currentp = scopedCreateVertex();
        guardedLink(waitVtxp, currentp, sampledRefOrClone(hoistVarp, exprp, flp), flp);

        જો (minN > 1) {
            SvaStateVertex* અચલ ringVtxp = addDelayChain(
                currentp, minN - 1, flp, સાચું, nullptr, sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = સાચું;
            SvaStateVertex* અચલ checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            currentp = checkVtxp;
        }
        જો (!hasMax) {
            currentp->m_isUnbounded = સાચું;  // [->N] waits unboundedly
            m_inUnboundedScope = સાચું;
            પાછા {currentp, nullptr, {}};
        }

        // [->M:N]: the range ring holds matches from M through N and advances
        // only on expr, preserving arbitrarily long gaps between occurrences.
        SvaStateVertex* અચલ mergeVtxp = scopedCreateVertex();
        guardedLink(currentp, mergeVtxp, flp);  // accept at match_M
        જો (maxN > minN) {
            SvaStateVertex* અચલ ringVtxp
                = addDelayChain(currentp, maxN - minN + 1, flp, ખોટું, nullptr,
                                sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = સાચું;
            SvaStateVertex* અચલ checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            guardedLink(checkVtxp, mergeVtxp, flp);
        }
        mergeVtxp->m_isUnbounded = સાચું;  // [->M:N] still has unbounded waits between matches
        m_inUnboundedScope = સાચું;
        પાછા {mergeVtxp, nullptr, {}};
    }

    // True if the sequence has a zero-minimum consecutive repetition, which may match empty
    સ્થિર તાર્કિક hasZeroMinRep(અચલ Ast_ગાંઠ_પદાવલિ* seqp) {
        પાછા seqp->exists(
            [](અચલ AstSConsRep* repp) { પાછા getConstUInt(repp->countp()) == 0; });
    }

    // True if a leaf's condition has no implication or multi-cycle sequence in it
    // (a leaf can be e.g. 'not (x |-> y)')
    સ્થિર તાર્કિક isPlainBoolean(અચલ Ast_ગાંઠ_પદાવલિ* condp) {
        પાછા !condp->exists(
            [](અચલ Ast_ગાંઠ_પદાવલિ* np) { પાછા np->isMultiCycleSva() || VN_IS(np, Implication); });
    }

    // Build merge vertex for SOr / LogOr: both branches feed into one vertex.
    BuildResult buildOrMerge(Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp, SvaStateVertex* entryVtxp,
                             File_લીટી* flp, તાર્કિક wantBoolLeaf) {
        અચલ BuildResult lhs = build_પદાવલિ(lhsp, entryVtxp);
        અચલ BuildResult rhs = build_પદાવલિ(rhsp, entryVtxp);
        જો (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            પાછા BuildResult::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP
        // IEEE 1800-2023 16.14.3: a cover sequence counts every end-of-match. A
        // sequence operand of 'or' can end more than once, but only its final
        // end reaches the merge vertex below, so reject sequence operands rather
        // than under-count. Plain boolean disjunction has one end per cycle and
        // is handled by the OR-fold.
        અચલ તાર્કિક booleanOnly = lhs.termVertexp == entryVtxp && lhs.finalCondp
                                 && rhs.termVertexp == entryVtxp && rhs.finalCondp;
        જો (m_isCoverSeq && !booleanOnly) {
            warnEndpointUnsupported(flp, "a sequence operand of 'or'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            પાછા BuildResult::failWithError();
        }
        જો (booleanOnly && wantBoolLeaf) {
            જો (isPlainBoolean(lhs.finalCondp) && isPlainBoolean(rhs.finalCondp)) {
                // As a |-> / |=> or top-level body, a disjunction of two plain booleans must
                // be able to reject: return it as a boolean finalCondp. The merge vertex
                // below drops the attempt silently when both operands are false.
                Ast_ગાંઠ_પદાવલિ* અચલ orCondp
                    = નવું AstLogOr{flp, lhs.finalCondp->clone_વૃક્ષ_Pure(ખોટું),
                                   rhs.finalCondp->clone_વૃક્ષ_Pure(ખોટું)};
                freeUnlinkedCondp(lhs.finalCondp);
                freeUnlinkedCondp(rhs.finalCondp);
                પાછા {entryVtxp, orCondp, {}};
            }
        }
        SvaStateVertex* અચલ mergeVtxp = scopedCreateVertex();
        જો (booleanOnly) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsp,
                        "Single-cycle SOr operands must have finalCondp");
            guardedLink(entryVtxp, mergeVtxp,
                        નવું AstLogOr{flp, sampled(lhs.finalCondp->clone_વૃક્ષ_Pure(ખોટું)),
                                     sampled(rhs.finalCondp->clone_વૃક્ષ_Pure(ખોટું))},
                        flp);
        } નહીંતર {
            જો (lhs.finalCondp) {
                guardedLink(lhs.termVertexp, mergeVtxp,
                            sampled(lhs.finalCondp->clone_વૃક્ષ_Pure(ખોટું)), flp);
            } નહીંતર {
                guardedLink(lhs.termVertexp, mergeVtxp, flp);
            }
            જો (rhs.finalCondp) {
                guardedLink(rhs.termVertexp, mergeVtxp,
                            sampled(rhs.finalCondp->clone_વૃક્ષ_Pure(ખોટું)), flp);
            } નહીંતર {
                guardedLink(rhs.termVertexp, mergeVtxp, flp);
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        પાછા {mergeVtxp, nullptr, {}};
    }

    // Build done-latch combiner for SAnd/SIntersect (IEEE 1800-2023 16.9.5).
    BuildResult buildAndCombiner(Ast_ગાંઠ_પદાવલિ* lhsExprp, Ast_ગાંઠ_પદાવલિ* rhsExprp,
                                 SvaStateVertex* entryVtxp, File_લીટી* flp) {
        // Snapshot-restore scope so LHS liveness does not leak into RHS.
        અચલ તાર્કિક savedScope = m_inUnboundedScope;
        અચલ BuildResult lhs = build_પદાવલિ(lhsExprp, entryVtxp);
        અચલ તાર્કિક lhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope;
        અચલ BuildResult rhs = build_પદાવલિ(rhsExprp, entryVtxp);
        અચલ તાર્કિક rhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope || lhsScope || rhsScope;
        જો (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            પાછા BuildResult::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP

        // Single-cycle operands: use boolean AND (done-latch would fire across cycles).
        // If both operands stayed at entry, they must be boolean leaves which
        // buildExpr returns with finalCondp=nodep (non-null).
        જો (lhs.termVertexp == entryVtxp && rhs.termVertexp == entryVtxp) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsExprp,
                        "Single-cycle SAnd operands must have finalCondp");
            Ast_ગાંઠ_પદાવલિ* અચલ condp = નવું AstLogAnd{flp, lhs.finalCondp->clone_વૃક્ષ_Pure(ખોટું),
                                                     rhs.finalCondp->clone_વૃક્ષ_Pure(ખોટું)};
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            પાછા {entryVtxp, condp, {}};
        }
        // Range-delay mid-window sources in either sub-branch would need
        // to be folded into the latch's match-now signal, which the
        // current combiner does not support.
        જો (!lhs.midSources.empty() || !rhs.midSources.empty()) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: ranged cycle delay in an operand of property 'and'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            પાછા BuildResult::failWithError();
        }
        SvaStateVertex* અચલ combVtxp = scopedCreateVertex();
        combVtxp->m_isAndCombiner = સાચું;
        combVtxp->m_andLhsTermp = lhs.termVertexp;
        combVtxp->m_andRhsTermp = rhs.termVertexp;
        જો (lhs.finalCondp) combVtxp->m_andLhsCondp = lhs.finalCondp->clone_વૃક્ષ_Pure(ખોટું);
        જો (rhs.finalCondp) combVtxp->m_andRhsCondp = rhs.finalCondp->clone_વૃક્ષ_Pure(ખોટું);
        જો (lhs.termVertexp->m_isUnbounded || rhs.termVertexp->m_isUnbounded) {
            combVtxp->m_isUnbounded = સાચું;
        }
        // Wire terminal-boolean rejects to a dedicated sink so each side can fail
        // the AND independently. Skip for liveness or single-cycle operands
        // (single-cycle termVertexp == entryVtxp would fire every cycle).
        જો (!combVtxp->m_isUnbounded) {
            તાર્કિક needSink = ખોટું;
            અચલ તાર્કિક lhsMultiCycle = (lhs.termVertexp != entryVtxp);
            અચલ તાર્કિક rhsMultiCycle = (rhs.termVertexp != entryVtxp);
            જો (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                needSink = સાચું;
            }
            જો (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                needSink = સાચું;
            }
            જો (needSink) {
                SvaStateVertex* અચલ sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = સાચું;
                જો (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* અચલ ep = m_graph.addLink(
                        lhs.termVertexp, sinkVtxp, sampled(lhs.finalCondp->clone_વૃક્ષ_Pure(ખોટું)));
                    ep->m_rejectOnFail = સાચું;
                }
                જો (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* અચલ ep = m_graph.addLink(
                        rhs.termVertexp, sinkVtxp, sampled(rhs.finalCondp->clone_વૃક્ષ_Pure(ખોટું)));
                    ep->m_rejectOnFail = સાચું;
                }
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        પાછા {combVtxp, nullptr, {}};
    }

    // Lower `seq1 within seq2` (IEEE 1800-2023 16.9.10) as:
    //   (OR_{i in 0..slack} 1 ##i seq1 ##(slack-i) 1) intersect seq2
    // Both operands must have fixed length (no ranged cycle delays).
    // The OR lives inside a single SIntersect so one AndCombiner done-latch
    // serves all offsets; lifting it out would double-count matches where
    // multiple offsets accept on the same seq2 end cycle.
    BuildResult buildSWithin(AstSWithin* nodep, SvaStateVertex* entryVtxp,
                             તાર્કિક is_ટોચ_Level_Step = ખોટું) {
        અચલ પૂર્ણાંક innerLen = fixedLength(nodep->lhsp());
        અચલ પૂર્ણાંક outerLen = fixedLength(nodep->rhsp());
        જો (innerLen < 0 || outerLen < 0) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: within with ranged cycle-delay operand");
            પાછા BuildResult::failWithError();
        }
        જો (innerLen > outerLen) {
            પાછા buildNeverMatchIntersect(
                nodep, entryVtxp, is_ટોચ_Level_Step,
                "the inner sequence is longer than the outer sequence");
        }
        File_લીટી* અચલ flp = nodep->fileline();
        અચલ પૂર્ણાંક slack = outerLen - innerLen;
        Ast_ગાંઠ_પદાવલિ* innerOrp = nullptr;
        માટે (પૂર્ણાંક i = 0; i <= slack; ++i) {
            અચલ પૂર્ણાંક postPad = slack - i;
            Ast_ગાંઠ_પદાવલિ* branchp = nodep->lhsp()->clone_વૃક્ષ_Pure(ખોટું);
            જો (i > 0) {
                AstConst* અચલ prePadp = નવું AstConst{flp, AstConst::BitTrue{}};
                AstDelay* અચલ delayp
                    = નવું AstDelay{flp, નવું AstConst{flp, static_cast<uint32_t>(i)}, સાચું};
                AstSExpr* અચલ wrapped = નવું AstSExpr{flp, prePadp, delayp, branchp};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            જો (postPad > 0) {
                AstConst* અચલ postTruep = નવું AstConst{flp, AstConst::BitTrue{}};
                AstDelay* અચલ delayp
                    = નવું AstDelay{flp, નવું AstConst{flp, static_cast<uint32_t>(postPad)}, સાચું};
                AstSExpr* અચલ wrapped = નવું AstSExpr{flp, branchp, delayp, postTruep};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            innerOrp = innerOrp ? static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstSOr{flp, innerOrp, branchp})
                                : branchp;
        }
        Ast_ગાંઠ_પદાવલિ* અચલ outerClonep = nodep->rhsp()->clone_વૃક્ષ_Pure(ખોટું);
        Ast_ગાંઠ_પદાવલિ* અચલ combinedp = નવું AstSIntersect{flp, innerOrp, outerClonep};
        BuildResult result = build_પદાવલિ(combinedp, entryVtxp, is_ટોચ_Level_Step);
        VL_DO_DANGLING(combinedp->delete_વૃક્ષ(), combinedp);
        // When both operands are plain booleans, buildAndCombiner returns a
        // freshly-allocated AstAnd as finalCondp with no parent. Callers up
        // the chain clone-and-discard finalCondp, so leave it parent-less and
        // it leaks; anchor it in the graph now via an edge.
        જો (result.valid() && result.finalCondp && !result.finalCondp->backp()) {
            SvaStateVertex* અચલ wrapVtxp = scopedCreateVertex();
            guardedLink(result.termVertexp, wrapVtxp, sampled(result.finalCondp), flp);
            result = {wrapVtxp, nullptr, result.midSources};
        }
        પાછા result;
    }

    // Collect the boolean leaf checks of a fixed-length sequence keyed by their
    // clock offset from the start. Returns false for anything other than nested
    // AstSExpr with fixed cycle delays over boolean leaves (e.g. throughout).
    સ્થિર તાર્કિક flattenFixedSeq(Ast_ગાંઠ_પદાવલિ* nodep, પૂર્ણાંક baseOffset,
                                std::map<પૂર્ણાંક, std::vector<Ast_ગાંઠ_પદાવલિ*>>& out) {
        જો (AstSExpr* અચલ sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* અચલ delayp = VN_CAST(sexprp->delayp(), Delay);
            જો (!delayp || !delayp->isCycleDelay() || delayp->isUnbounded()) પાછા ખોટું;
            અચલ unsigned delayCycles = getConstUInt(delayp->lhsp());
            જો (delayp->isRangeDelay() && getConstUInt(delayp->rhsp()) != delayCycles)
                પાછા ખોટું;
            પૂર્ણાંક preLen = 0;
            જો (Ast_ગાંઠ_પદાવલિ* અચલ prep = sexprp->preExprp()) {
                જો (!flattenFixedSeq(prep, baseOffset, out)) પાછા ખોટું;
                preLen = fixedLength(prep);
                જો (preLen < 0) પાછા ખોટું;
            }
            પાછા flattenFixedSeq(sexprp->exprp(), baseOffset + preLen + delayCycles, out);
        }
        જો (nodep->isMultiCycleSva()) પાછા ખોટું;
        out[baseOffset].ધકેલો_back(nodep);
        પાછા સાચું;
    }

    // Conjoin two equal-length fixed sequences into one: at each clock offset
    // AND the boolean checks of both operands (IEEE 1800-2023 16.9.6 -- both
    // operands match the same window). Returns null if either operand is not a
    // plain fixed sequence of boolean leaves.
    સ્થિર Ast_ગાંઠ_પદાવલિ* conjoinFixedSeqs(Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp, File_લીટી* flp) {
        std::map<પૂર્ણાંક, std::vector<Ast_ગાંઠ_પદાવલિ*>> checks;
        જો (!flattenFixedSeq(lhsp, 0, checks) || !flattenFixedSeq(rhsp, 0, checks)) પાછા nullptr;
        જો (checks.empty()) પાછા nullptr;
        Ast_ગાંઠ_પદાવલિ* resultp = nullptr;
        પૂર્ણાંક prevOffset = 0;
        માટે (અચલ auto& offsetChecks : checks) {
            અચલ પૂર્ણાંક offset = offsetChecks.first;
            Ast_ગાંઠ_પદાવલિ* condp = nullptr;
            માટે (Ast_ગાંઠ_પદાવલિ* અચલ leafp : offsetChecks.second) {
                Ast_ગાંઠ_પદાવલિ* અચલ clonep = leafp->clone_વૃક્ષ_Pure(ખોટું);
                જો (!condp) {
                    condp = clonep;
                } નહીંતર {
                    condp = નવું AstLogAnd{flp, condp, clonep};
                    condp->dtypeSetBit();
                }
            }
            જો (!resultp) {
                જો (offset > 0) {
                    AstDelay* અચલ delayp = નવું AstDelay{
                        flp, નવું AstConst{flp, static_cast<uint32_t>(offset)}, /*isCycle=*/સાચું};
                    resultp
                        = નવું AstSExpr{flp, નવું AstConst{flp, AstConst::BitTrue{}}, delayp, condp};
                    resultp->dtypeSetBit();
                } નહીંતર {
                    resultp = condp;
                }
            } નહીંતર {
                AstDelay* અચલ delayp = નવું AstDelay{
                    flp, નવું AstConst{flp, static_cast<uint32_t>(offset - prevOffset)},
                    /*isCycle=*/સાચું};
                resultp = નવું AstSExpr{flp, resultp, delayp, condp};
                resultp->dtypeSetBit();
            }
            prevOffset = offset;
        }
        પાછા resultp;
    }

    // `seq` is a simple ranged sequence `start ##[m:n] end` (start/end boolean,
    // start may be absent). Used to collapse a both-variable intersect to one
    // ranged delay.
    સંરચના SimpleRanged final {
        તાર્કિક ok = ખોટું;
        Ast_ગાંઠ_પદાવલિ* startp = nullptr;  // may be null (absent start)
        Ast_ગાંઠ_પદાવલિ* endp = nullptr;
    };
    સ્થિર SimpleRanged asSimpleRanged(Ast_ગાંઠ_પદાવલિ* nodep) {
        AstSExpr* અચલ sexprp = VN_CAST(nodep, SExpr);
        જો (!sexprp) પાછા {};
        AstDelay* અચલ delayp = VN_CAST(sexprp->delayp(), Delay);
        જો (!delayp || !delayp->isCycleDelay() || !delayp->isRangeDelay() || delayp->isUnbounded())
            પાછા {};
        જો (getConstUInt(delayp->lhsp()) == getConstUInt(delayp->rhsp())) પાછા {};
        Ast_ગાંઠ_પદાવલિ* અચલ prep = sexprp->preExprp();
        જો (prep && fixedLength(prep) != 0) પાછા {};
        જો (fixedLength(sexprp->exprp()) != 0) પાછા {};
        પાછા {સાચું, prep, sexprp->exprp()};
    }

    // Build the NFA for a synthesized intersect lowering tree, then free it.
    // buildExpr returns the terminal condition (finalCondp) by reference into the
    // tree; detach a clone so the tree can be freed here. The graph already holds
    // clones/hoists of every edge condition, so nothing else dangles.
    BuildResult build_From_Lowering_વૃક્ષ(Ast_ગાંઠ_પદાવલિ* treep, SvaStateVertex* entryVtxp,
                                      તાર્કિક is_ટોચ_Level_Step) {
        BuildResult result = build_પદાવલિ(treep, entryVtxp, is_ટોચ_Level_Step);
        જો (result.valid() && result.finalCondp) {
            result.finalCondp = result.finalCondp->clone_વૃક્ષ_Pure(ખોટું);
        }
        VL_DO_DANGLING(treep->delete_વૃક્ષ(), treep);
        પાછા result;
    }

    // Empty common-length intersection -- unequal fixed lengths, or disjoint
    // ranged lengths. IEEE 1800-2023 16.9.6 requires both operands to match
    // over a window of the same length, so with no common length the intersect
    // simply never matches. This is legal (matching nothing), not an error, so
    // lower to a constant false rather than rejecting legal code.
    BuildResult buildNeverMatchIntersect(Ast_ગાંઠ_પદાવલિ* nodep, SvaStateVertex* entryVtxp,
                                         તાર્કિક is_ટોચ_Level_Step, અચલ અક્ષર* reason) {
        nodep->v3warn(NEVERMATCH, "Sequence can never match because " << reason << ".");
        Ast_ગાંઠ_પદાવલિ* અચલ falsep = નવું AstConst{nodep->fileline(), AstConst::BitFalse{}};
        પાછા build_From_Lowering_વૃક્ષ(falsep, entryVtxp, is_ટોચ_Level_Step);
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
                                     તાર્કિક is_ટોચ_Level_Step) {
        અચલ std::pair<પૂર્ણાંક, પૂર્ણાંક> lhsRange = lengthRange(nodep->lhsp());
        અચલ std::pair<પૂર્ણાંક, પૂર્ણાંક> rhsRange = lengthRange(nodep->rhsp());
        જો (lhsRange.first < 0 || rhsRange.first < 0) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect with this variable-length operand");
            પાછા BuildResult::failWithError();
        }
        અચલ પૂર્ણાંક lo = std::max(lhsRange.first, rhsRange.first);
        અચલ પૂર્ણાંક hi = std::min(lhsRange.second, rhsRange.second);
        જો (lo > hi) {
            // Disjoint length ranges share no common length -> never matches.
            પાછા buildNeverMatchIntersect(nodep, entryVtxp, is_ટોચ_Level_Step,
                                            "intersect operands have no common length");
        }
        File_લીટી* અચલ flp = nodep->fileline();
        જો (lo == hi) {
            Ast_ગાંઠ_પદાવલિ* અચલ lp = realizeAtLength(nodep->lhsp(), lo, lhsRange.first);
            Ast_ગાંઠ_પદાવલિ* અચલ rp = realizeAtLength(nodep->rhsp(), lo, rhsRange.first);
            Ast_ગાંઠ_પદાવલિ* અચલ conjp = conjoinFixedSeqs(lp, rp, flp);
            VL_DO_DANGLING(lp->delete_વૃક્ષ(), lp);
            VL_DO_DANGLING(rp->delete_વૃક્ષ(), rp);
            જો (!conjp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: intersect operand is not a plain boolean sequence");
                પાછા BuildResult::failWithError();
            }
            પાછા build_From_Lowering_વૃક્ષ(conjp, entryVtxp, is_ટોચ_Level_Step);
        }
        અચલ SimpleRanged sl = asSimpleRanged(nodep->lhsp());
        અચલ SimpleRanged sr = asSimpleRanged(nodep->rhsp());
        જો (!sl.ok || !sr.ok) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect of two sequences that each vary in length over a"
                          " range with internal structure");
            પાછા BuildResult::failWithError();
        }
        અચલ auto andBool = [&](Ast_ગાંઠ_પદાવલિ* ap, Ast_ગાંઠ_પદાવલિ* bp) -> Ast_ગાંઠ_પદાવલિ* {
            Ast_ગાંઠ_પદાવલિ* અચલ aClonep
                = ap ? ap->clone_વૃક્ષ_Pure(ખોટું) : નવું AstConst{flp, AstConst::BitTrue{}};
            Ast_ગાંઠ_પદાવલિ* અચલ bClonep
                = bp ? bp->clone_વૃક્ષ_Pure(ખોટું) : નવું AstConst{flp, AstConst::BitTrue{}};
            AstLogAnd* અચલ andp = નવું AstLogAnd{flp, aClonep, bClonep};
            andp->dtypeSetBit();
            પાછા andp;
        };
        AstDelay* અચલ delayp = નવું AstDelay{flp, નવું AstConst{flp, static_cast<uint32_t>(lo)},
                                              /*isCycle=*/સાચું};
        delayp->rhsp(નવું AstConst{flp, static_cast<uint32_t>(hi)});
        AstSExpr* અચલ reducedp
            = નવું AstSExpr{flp, andBool(sl.startp, sr.startp), delayp, andBool(sl.endp, sr.endp)};
        reducedp->dtypeSetBit();
        પાછા build_From_Lowering_વૃક્ષ(reducedp, entryVtxp, is_ટોચ_Level_Step);
    }

    BuildResult buildThroughout(AstSThroughout* nodep, SvaStateVertex* entryVtxp,
                                તાર્કિક is_ટોચ_Level_Step = ખોટું) {
        // Mark entryVtxp so "cond false at tick 0" is detected as throughout-drop.
        entryVtxp->m_throughoutConds.ધકેલો_back(nodep->lhsp()->clone_વૃક્ષ_Pure(ખોટું));
        m_temporal_Guard_ઢગલો.ધકેલો_back(nodep->lhsp());
        અચલ BuildResult result = build_પદાવલિ(nodep->rhsp(), entryVtxp, is_ટોચ_Level_Step);
        m_temporal_Guard_ઢગલો.કાઢો_back();
        પાછા result;
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
    BuildResult buildUntil(AstUntil* nodep, SvaStateVertex* entryVtxp, તાર્કિક is_ટોચ_Level_Step) {
        File_લીટી* અચલ flp = nodep->fileline();
        જો (!is_ટોચ_Level_Step) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            પાછા BuildResult::failWithError();
        }
        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->lhsp();
        Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp();
        અચલ auto hasSeq = [](અચલ Ast_ગાંઠ_પદાવલિ* ep) {
            પાછા ep->exists([](અચલ Ast_ગાંઠ_પદાવલિ* np) { પાછા np->isMultiCycleSva(); });
        };
        જો (hasSeq(lhsp) || hasSeq(rhsp)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            પાછા BuildResult::failWithError();
        }

        અચલ તાર્કિક ov = nodep->isOverlapping();
        Ast_ગાંઠ_પદાવલિ* અચલ lhsBitp = nodep->lhsp();
        Ast_ગાંઠ_પદાવલિ* અચલ rhsBitp = nodep->rhsp();
        // p hoist count: continue, require (ov: 1 use; nov: 1 use). At least 2 uses.
        AstVar* અચલ pHoistp = tryHoistSampled(lhsBitp, flp, 2);
        // q hoist count: continue (1) + require nov (1) = 2; ov: continue only (1).
        AstVar* અચલ qHoistp = ov ? nullptr : tryHoistSampled(rhsBitp, flp, 2);

        SvaStateVertex* અચલ waitCp = scopedCreateVertex();
        SvaStateVertex* અચલ waitRp = scopedCreateVertex();
        waitCp->m_isUnbounded = સાચું;
        waitRp->m_strongPending = nodep->isStrong() && !m_isCover;

        // Entry and back-loop Links carry no condition; throughout-folding still applies.
        guardedLink(entryVtxp, waitCp, flp);
        guardedLink(waitRp, waitCp, flp);

        // Continue clocked edge: p && !q advances to next-cycle wait.
        Ast_ગાંઠ_પદાવલિ* અચલ contCondp
            = નવું AstLogAnd{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                            નવું AstLogNot{flp, sampledRefOrClone(qHoistp, rhsBitp, flp)}};
        guardedEdge(waitCp, waitRp, contCondp, flp);

        // Reject sink: fires when require-condition is false.
        SvaStateVertex* અચલ sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = સાચું;
        Ast_ગાંઠ_પદાવલિ* requireCondp;
        જો (ov) {
            requireCondp = sampledRefOrClone(pHoistp, lhsBitp, flp);
        } નહીંતર {
            requireCondp = નવું AstLogOr{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                                        sampledRefOrClone(qHoistp, rhsBitp, flp)};
        }
        SvaTransEdge* અચલ rejEdgep = m_graph.addLink(waitCp, sinkVtxp, requireCondp);
        rejEdgep->m_rejectOnFail = સાચું;

        // Accept condition rides via finalCondp; assembleResult $sampled-wraps it.
        Ast_ગાંઠ_પદાવલિ* acceptCondp;
        જો (ov) {
            acceptCondp
                = નવું AstLogAnd{flp, lhsBitp->clone_વૃક્ષ_Pure(ખોટું), rhsBitp->clone_વૃક્ષ_Pure(ખોટું)};
        } નહીંતર {
            acceptCondp = rhsBitp->clone_વૃક્ષ_Pure(ખોટું);
        }
        પાછા {waitCp, acceptCondp, {}};
    }

    // IEEE 1800-2023 16.12.14 property abort operators. Sync and async share
    // the same NFA encoding: AstSampled already gives matured values at every
    // maturing clocking event, and async firing "between clocks" is not
    // observable in a cycle-based model. VAbortKind selects accept vs reject
    // verdict (sync vs async only changes the user-visible spelling).

    // Build `condp && !outer_1 && !outer_2 ...` (unsampled).
    Ast_ગાંઠ_પદાવલિ* abort_Fire_પદાવલિ(Ast_ગાંઠ_પદાવલિ* condp, File_લીટી* flp) {
        Ast_ગાંઠ_પદાવલિ* resultp = condp->clone_વૃક્ષ_Pure(ખોટું);
        માટે (Ast_ગાંઠ_પદાવલિ* અચલ op : m_outer_Abort_ઢગલો)
            resultp = નવું AstLogAnd{flp, resultp, નવું AstLogNot{flp, op->clone_વૃક્ષ_Pure(ખોટું)}};
        પાછા resultp;
    }

    // True when unguarded ring-wide rejection or a same-tick Link chain already accounts the
    // attempt: a required-step Link covers both outcomes; followed-by pairs both edges.
    સ્થિર તાર્કિક chainAccountsSource(અચલ SvaStateVertex* srcp,
                                    અચલ std::unordered_set<અચલ V3GraphEdge*>& preEdges) {
        જો (srcp->m_delayRingSize && srcp->m_throughoutConds.empty()) પાછા સાચું;
        માટે (અચલ V3GraphEdge& edger : srcp->inEdges()) {
            જો (preEdges.ગણતરી(&edger)) ચાલુ;
            અચલ SvaTransEdge& tedger = static_cast<અચલ SvaTransEdge&>(edger);
            જો (tedger.m_consumesCycle) ચાલુ;
            અચલ auto* અચલ fromp = static_cast<અચલ SvaStateVertex*>(tedger.fromVtxp());
            જો (fromp->m_abortRejectp) પાછા સાચું;
        }
        તાર્કિક plainNonSink = ખોટું;
        તાર્કિક markedSink = ખોટું;
        માટે (અચલ V3GraphEdge& edger : srcp->outEdges()) {
            જો (preEdges.ગણતરી(&edger)) ચાલુ;
            અચલ SvaTransEdge& tedger = static_cast<અચલ SvaTransEdge&>(edger);
            જો (tedger.m_consumesCycle) ચાલુ;
            અચલ તાર્કિક sink = static_cast<અચલ SvaStateVertex*>(tedger.toVtxp())->m_isRejectSink;
            જો (tedger.m_rejectOnFail) {
                જો (!sink) પાછા સાચું;
                markedSink = સાચું;
            } નહીંતર જો (!sink) {
                જો (srcp->m_abortRejectp) પાછા સાચું;
                plainNonSink = સાચું;
            }
        }
        પાછા plainNonSink && markedSink;
    }

    // Reject edge: fires when the source is live and the abort samples true.
    શૂન્ય addAbortRejectEdge(SvaStateVertex* srcp, SvaStateVertex* sinkp, Ast_ગાંઠ_પદાવલિ* condp,
                            File_લીટી* flp) {
        Ast_ગાંઠ_પદાવલિ* અચલ notFirep = નવું AstLogNot{flp, sampled(abort_Fire_પદાવલિ(condp, flp))};
        m_graph.addLink(srcp, sinkp, notFirep)->m_rejectOnFail = સાચું;
        પાછા;
    }

    // On the fire tick: kill body threads; accept kinds also forgive step misses.
    શૂન્ય gateBodyEdgesOnAbort(અચલ std::unordered_set<અચલ V3GraphEdge*>& preEdges,
                              Ast_ગાંઠ_પદાવલિ* condp, VAbortKind kind, File_લીટી* flp) {
        માટે (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            માટે (V3GraphEdge& edger : vtxr.outEdges()) {
                જો (preEdges.ગણતરી(&edger)) ચાલુ;
                SvaTransEdge* અચલ tedgep = static_cast<SvaTransEdge*>(&edger);
                જો (tedgep->m_rejectOnFail) {
                    જો (!kind.isAccept()) ચાલુ;
                    Ast_ગાંઠ_પદાવલિ* અચલ firep = sampled(abort_Fire_પદાવલિ(condp, flp));
                    tedgep->m_condp
                        = tedgep->m_condp ? નવું AstLogOr{flp, tedgep->m_condp, firep} : firep;
                } નહીંતર જો (tedgep->m_consumesCycle) {
                    Ast_ગાંઠ_પદાવલિ* અચલ notFirep
                        = નવું AstLogNot{flp, sampled(abort_Fire_પદાવલિ(condp, flp))};
                    tedgep->m_condp = tedgep->m_condp
                                          ? નવું AstLogAnd{flp, tedgep->m_condp, notFirep}
                                          : notFirep;
                }
            }
        }
    }

    BuildResult buildAbortOn(Ast_ગાંઠ_પદાવલિ* condp, Ast_ગાંઠ_પદાવલિ* bodyp, SvaStateVertex* entryVtxp,
                             VAbortKind kind, File_લીટી* flp, તાર્કિક is_ટોચ_Level_Step) {
        // Snapshot pre-body vertices/edges so post-build diff yields the body's sub-NFA.
        std::unordered_set<અચલ V3GraphVertex*> preExisting;
        std::unordered_set<અચલ V3GraphEdge*> preEdges;
        માટે (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            preExisting.ઉમેરો(&vtxr);
            માટે (V3GraphEdge& edger : vtxr.outEdges()) preEdges.ઉમેરો(&edger);
        }

        m_outer_Abort_ઢગલો.ધકેલો_back(condp);
        અચલ BuildResult bodyResult = build_પદાવલિ(bodyp, entryVtxp, is_ટોચ_Level_Step);
        m_outer_Abort_ઢગલો.કાઢો_back();
        જો (!bodyResult.valid()) પાછા bodyResult;

        gateBodyEdgesOnAbort(preEdges, condp, kind, flp);

        // Live-thread sources for the abort edge: entry + new body vertices,
        // minus reject sinks (they carry reject fuel, not live-thread fuel).
        std::vector<SvaStateVertex*> abortSources;
        abortSources.ધકેલો_back(entryVtxp);
        માટે (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            જો (preExisting.ગણતરી(&vtxr)) ચાલુ;
            auto* અચલ sp = static_cast<SvaStateVertex*>(&vtxr);
            જો (sp->m_delayRingSize || !sp->m_throughoutConds.empty()) {
                Ast_ગાંઠ_પદાવલિ* અચલ firep = abort_Fire_પદાવલિ(condp, flp);
                sp->m_abortClearp
                    = sp->m_abortClearp ? નવું AstLogOr{flp, sp->m_abortClearp, firep} : firep;
            }
            જો (!kind.isAccept()
                && ((sp->m_delayRingSize && sp->m_throughoutConds.empty())
                    || sp->m_replayAbortReject)) {
                Ast_ગાંઠ_પદાવલિ* અચલ firep = abort_Fire_પદાવલિ(condp, flp);
                sp->m_abortRejectp
                    = sp->m_abortRejectp ? નવું AstLogOr{flp, sp->m_abortRejectp, firep} : firep;
            }
            જો (sp->m_isRejectSink) ચાલુ;
            abortSources.ધકેલો_back(sp);
        }

        auto sampledAbortFire = [&]() -> Ast_ગાંઠ_પદાવલિ* {
            Ast_ગાંઠ_પદાવલિ* અચલ પદાવલિ = abort_Fire_પદાવલિ(condp, flp);
            પાછા sampled(પદાવલિ);
        };

        જો (kind.isAccept()) {
            // Match-only sink fed by $sampled(abort-fire) from every live source;
            // registered as midSource so it never contributes a reject.
            SvaStateVertex* અચલ acceptSinkp = scopedCreateVertex();
            માટે (SvaStateVertex* અચલ srcp : abortSources)
                guardedLink(srcp, acceptSinkp, sampledAbortFire(), flp);
            std::vector<SvaStateVertex*> midSources = bodyResult.midSources;
            midSources.ધકેલો_back(acceptSinkp);
            Ast_ગાંઠ_પદાવલિ* finalCondp = bodyResult.finalCondp;
            જો (finalCondp) {
                જો (finalCondp->backp()) finalCondp = finalCondp->clone_વૃક્ષ_Pure(ખોટું);
                finalCondp = નવું AstLogOr{flp, finalCondp, abort_Fire_પદાવલિ(condp, flp)};
            }
            પાછા {bodyResult.termVertexp, finalCondp, std::move(midSources)};
        }

        // rejectOnFail treats m_condp as the success condition and fires on
        // !condp, so the edge carries !sampledAbortFire().
        SvaStateVertex* અચલ rejectSinkp = m_graph.createStateVertex();
        rejectSinkp->m_isRejectSink = સાચું;
        માટે (SvaStateVertex* અચલ srcp : abortSources)
            જો (!chainAccountsSource(srcp, preEdges))
                addAbortRejectEdge(srcp, rejectSinkp, condp, flp);
        Ast_ગાંઠ_પદાવલિ* finalCondp = bodyResult.finalCondp;
        જો (finalCondp) {
            જો (finalCondp->backp()) finalCondp = finalCondp->clone_વૃક્ષ_Pure(ખોટું);
            finalCondp
                = નવું AstLogAnd{flp, finalCondp, નવું AstLogNot{flp, abort_Fire_પદાવલિ(condp, flp)}};
        }
        પાછા {bodyResult.termVertexp, finalCondp, bodyResult.midSources};
    }

જાહેર:
    // Free a dropped sub-result condition that is not linked into the AST
    // (abort folds synthesize unparented finalCondp trees).
    સ્થિર શૂન્ય freeUnlinkedCondp(Ast_ગાંઠ_પદાવલિ* condp) {
        જો (condp && !condp->backp()) VL_DO_DANGLING(condp->delete_વૃક્ષ(), condp);
    }

    SvaNfaBuilder(SvaGraph& graph, Ast_ગાંઠ_Module* modp, V3UniqueNames& propTempNames,
                  તાર્કિક isCoverSeq = ખોટું, તાર્કિક isSeqEvent = ખોટું, તાર્કિક isCover = ખોટું)
        : m_graph{graph}
        , m_modp{modp}
        , m_propTempNames{propTempNames}
        , m_isCover{isCover}
        , m_isCoverSeq{isCoverSeq}
        , m_isSeqEvent{isSeqEvent} {}

    // Reset scope between antecedent and consequent: liveness must not leak.
    // m_outerAbortStack survives: an abort wrapping the implication covers the
    // consequent too (IEEE 1800-2023 16.12.14).
    શૂન્ય resetScope() {
        m_inUnboundedScope = ખોટું;
        m_temporal_Guard_ઢગલો.clear();
    }

    BuildResult build_પદાવલિ(Ast_ગાંઠ_પદાવલિ* nodep, SvaStateVertex* entryVtxp,
                          તાર્કિક is_ટોચ_Level_Step = ખોટું) {
        VL_RESTORER(m_wantBoolLeaf);
        અચલ તાર્કિક wantBoolLeaf = m_wantBoolLeaf;
        m_wantBoolLeaf = ખોટું;
        જો (AstSExpr* અચલ sexprp = VN_CAST(nodep, SExpr)) {
            પાછા buildSExpr(sexprp, entryVtxp, is_ટોચ_Level_Step);
        }
        જો (AstSConsRep* અચલ repp = VN_CAST(nodep, SConsRep)) {
            પાછા buildConsRep(repp, entryVtxp, is_ટોચ_Level_Step);
        }
        જો (AstPropAlways* અચલ alwaysp = VN_CAST(nodep, PropAlways)) {
            પાછા buildPropAlways(alwaysp, entryVtxp, is_ટોચ_Level_Step);
        }
        જો (AstSGotoRep* અચલ repp = VN_CAST(nodep, SGotoRep)) {
            પાછા buildGotoRep(repp, entryVtxp);
        }
        જો (AstSThroughout* અચલ throughoutp = VN_CAST(nodep, SThroughout)) {
            પાછા buildThroughout(throughoutp, entryVtxp, is_ટોચ_Level_Step);
        }
        જો (AstSOr* અચલ orp = VN_CAST(nodep, SOr)) {
            પાછા buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        જો (AstLogOr* અચલ orp = VN_CAST(nodep, LogOr)) {
            પાછા buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        જો (AstSAnd* અચલ andp = VN_CAST(nodep, SAnd)) {
            પાછા buildAndCombiner(andp->lhsp(), andp->rhsp(), entryVtxp, andp->fileline());
        }
        જો (AstSIntersect* અચલ intp = VN_CAST(nodep, SIntersect)) {
            // IEEE 1800-2023 16.9.6: both operands match over one window with
            // equal start and end (equal length). Lower to a single sequence
            // that conjoins both operands' per-cycle checks -- correct under
            // concurrent attempts, where the done-latch combiner conflates the
            // two operands' start times and over-accepts. The combiner remains
            // only as a fallback for operands that do not flatten.
            અચલ પૂર્ણાંક lhsLen = fixedLength(intp->lhsp());
            અચલ પૂર્ણાંક rhsLen = fixedLength(intp->rhsp());
            જો (lhsLen >= 0 && rhsLen >= 0) {
                જો (lhsLen != rhsLen) {
                    // Unequal fixed lengths share no common length -> never matches.
                    પાછા buildNeverMatchIntersect(intp, entryVtxp, is_ટોચ_Level_Step,
                                                    "intersect operands have no common length");
                }
                જો (Ast_ગાંઠ_પદાવલિ* અચલ conjp
                    = conjoinFixedSeqs(intp->lhsp(), intp->rhsp(), intp->fileline())) {
                    પાછા build_From_Lowering_વૃક્ષ(conjp, entryVtxp, is_ટોચ_Level_Step);
                }
                પાછા buildAndCombiner(intp->lhsp(), intp->rhsp(), entryVtxp, intp->fileline());
            }
            પાછા buildVarLenIntersect(intp, entryVtxp, is_ટોચ_Level_Step);
        }
        જો (AstSWithin* અચલ withinp = VN_CAST(nodep, SWithin)) {
            પાછા buildSWithin(withinp, entryVtxp, is_ટોચ_Level_Step);
        }
        જો (AstAbortOn* અચલ ap = VN_CAST(nodep, AbortOn)) {
            પાછા buildAbortOn(ap->condp(), ap->propp(), entryVtxp, ap->kind(), ap->fileline(),
                                is_ટોચ_Level_Step);
        }
        જો (VN_IS(nodep, SNonConsRep)) પાછા BuildResult::fail();
        જો (AstImplication* અચલ implp = VN_CAST(nodep, Implication)) {
            પાછા buildImplicationEdges(implp->lhsp(), implp->rhsp(), entryVtxp,
                                         implp->isOverlapped(), implp->isFollowedBy(),
                                         implp->lhsp(), implp->fileline());
        }
        જો (AstUntil* અચલ untilp = VN_CAST(nodep, Until)) {
            પાછા buildUntil(untilp, entryVtxp, is_ટોચ_Level_Step);
        }
        // Boolean leaf (including LogAnd): return as finalCond
        પાછા {entryVtxp, nodep, {}};
    }

    // Wire an implication / followed-by from `entryVtxp`: builds the antecedent,
    // emits the match-link (and for followed-by the reject-sink edge), inserts a
    // delay vertex for non-overlapped forms, and builds the body. Used both for
    // nested AstImplication in pexpr position and for the top-level assertion
    // antecedent -- `errorNodep` anchors the "unsupported sequence antecedent"
    // error, which differs between the two call sites.
    BuildResult buildImplicationEdges(Ast_ગાંઠ_પદાવલિ* antExprp, Ast_ગાંઠ_પદાવલિ* bodyExprp,
                                      SvaStateVertex* entryVtxp, તાર્કિક isOverlapped,
                                      તાર્કિક isFollowedBy, Ast_ગાંઠ* errorNodep, File_લીટી* flp) {
        અચલ BuildResult antResult = build_પદાવલિ(antExprp, entryVtxp);
        જો (!antResult.valid()) પાછા antResult;

        // Followed-by requires pure-boolean antecedent for non-vacuous-fail at
        // the attempt-start cycle. IEEE 1800-2023 16.12.9 permits a multi-cycle
        // sequence LHS, so this is an implementation gap rather than illegal SV.
        જો (isFollowedBy && antResult.termVertexp != entryVtxp) {
            errorNodep->v3warn(E_UNSUPPORTED,
                               "Unsupported: sequence expression as antecedent of followed-by"
                               " (#-# / #=#) (IEEE 1800-2023 16.12.9)");
            પાછા BuildResult::failWithError();
        }
        UASSERT_OBJ(!isFollowedBy || antResult.finalCondp, errorNodep,
                    "followed-by antecedent terminal at entry must carry finalCondp");

        // Use raw createStateVertex() so trigVtxp starts without liveness --
        // reaching the antecedent terminal is a definitive event.
        SvaStateVertex* અચલ trigVtxp = m_graph.createStateVertex();
        જો (antResult.finalCondp) {
            m_graph.addLink(antResult.termVertexp, trigVtxp,
                            sampled(antResult.finalCondp->clone_વૃક્ષ_Pure(ખોટું)));
            // Followed-by non-vacuous fail: rejectOnFail fires when the attempt
            // is live (termVtx reachable) and sampled(antecedent) is false.
            જો (isFollowedBy) {
                SvaStateVertex* અચલ sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = સાચું;
                SvaTransEdge* અચલ ep
                    = m_graph.addLink(antResult.termVertexp, sinkVtxp,
                                      sampled(antResult.finalCondp->clone_વૃક્ષ_Pure(ખોટું)));
                ep->m_rejectOnFail = સાચું;
            }
            // finalCondp is cloned into the Sampled nodes; if the original is
            // not parented anywhere in the AST anymore it must be freed here
            // or ASan flags it as a leak (e.g. t_sequence_bool_ops).
            જો (!antResult.finalCondp->backp()) {
                VL_DO_DANGLING(antResult.finalCondp->delete_વૃક્ષ(), antResult.finalCondp);
            }
        } નહીંતર {
            m_graph.addLink(antResult.termVertexp, trigVtxp);
        }
        resetScope();

        SvaStateVertex* bodyEntryp = trigVtxp;
        જો (!isOverlapped) {
            SvaStateVertex* અચલ delayVtxp = m_graph.createStateVertex();
            m_graph.addClockedEdge(trigVtxp, delayVtxp);
            bodyEntryp = delayVtxp;
        }
        // An empty antecedent match must not start the consequent, but a zero-minimum
        // repetition's empty match is not handled here; keep the merge vertex for those
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = !isFollowedBy && !hasZeroMinRep(antExprp);
        પાછા build_પદાવલિ(bodyExprp, bodyEntryp, /*isTopLevelStep=*/સાચું);
    }

    BuildResult build(Ast_ગાંઠ_પદાવલિ* exprp) {
        m_graph.m_startVertexp = scopedCreateVertex();
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = સાચું;
        પાછા build_પદાવલિ(exprp, m_graph.m_startVertexp, /*isTopLevelStep=*/સાચું);
    }
};

//######################################################################
// NFA Lowering (converts NFA graph to synthesizable AstAlways blocks)

વર્ગ SvaNfaLowering final {
    Ast_ગાંઠ_Module* અચલ m_modp;  // Module to add state vars and always blocks to
    Ast_ગાંઠ_DType* અચલ m_u32DTypep;  // Shared unsigned counter dtype
    V3UniqueNames m_names{"__Vnfa"};
    size_t m_statDelayRingEdgeVisits = 0;  // Delay-ring incoming edges visited

    // Per-lowering shared context (passed to phase sub-functions)
    // Per-vertex lowering state is stored in SvaVertexData and accessed via
    // V3GraphVertex::userp() (see SvaStateVertex::datap()).
    સંરચના LowerCtx final {
        File_લીટી* flp;  // Source location for generated AST
        Ast_Sen_વૃક્ષ* senTreep;  // Clock sensitivity tree
        Ast_ગાંઠ_પદાવલિ* disableExprp;  // disable iff expression (may be nullptr)
        Ast_ગાંઠ_પદાવલિ* matchCondp;  // Final boolean match condition (may be nullptr)
        VAssertType assertType;  // Assertion type for control tasks
        VAssertDirectiveType directiveType;  // Directive type for control tasks
        AstVar* killVarp;  // Last observed kill generation
        SvaGraph& graph;  // NFA graph
    };

    // Build a match-now expression: stateSig[i] && $sampled(condp)
    સ્થિર Ast_ગાંઠ_પદાવલિ* buildMatchNow(File_લીટી* flp, Ast_ગાંઠ_પદાવલિ* stateExprp, Ast_ગાંઠ_પદાવલિ* condp) {
        Ast_ગાંઠ_પદાવલિ* અચલ statep = stateExprp->clone_વૃક્ષ_Pure(ખોટું);
        જો (!condp) પાછા statep;
        પાછા નવું AstLogAnd{flp, statep, sampled(condp->clone_વૃક્ષ_Pure(ખોટું))};
    }
    સ્થિર Ast_ગાંઠ_પદાવલિ* andCond(File_લીટી* flp, Ast_ગાંઠ_પદાવલિ* exprp, Ast_ગાંઠ_પદાવલિ* condp) {
        જો (!condp) પાછા exprp;
        પાછા નવું AstLogAnd{flp, exprp, condp->clone_વૃક્ષ_Pure(ખોટું)};
    }
    // bp is always non-null; only ap can be null (serving as accumulator).
    સ્થિર Ast_ગાંઠ_પદાવલિ* orExprs(File_લીટી* flp, Ast_ગાંઠ_પદાવલિ* ap, Ast_ગાંઠ_પદાવલિ* bp) {
        જો (!ap) પાછા bp;
        પાછા નવું AstLogOr{flp, ap, bp};
    }
    સ્થિર Ast_ગાંઠ_પદાવલિ* addCountp(File_લીટી* અચલ flp, Ast_ગાંઠ_પદાવલિ* અચલ totalCountp,
                                  Ast_ગાંઠ_પદાવલિ* અચલ contributionp,
                                  Ast_ગાંઠ_પદાવલિ* અચલ enablep = nullptr) {
        // contribution = enable ? contribution : 0;
        Ast_ગાંઠ_પદાવલિ* અચલ enabledContributionp
            = enablep ? નવું AstCond{flp, enablep, contributionp,
                                    newTypedConstp(flp, contributionp->dtypep(), 0)}
                      : contributionp;
        જો (!totalCountp) પાછા enabledContributionp;
        પાછા નવું AstAdd{flp, totalCountp, enabledContributionp};
    }
    સ્થિર Ast_ગાંઠ_પદાવલિ* gateCheckedCountp(File_લીટી* અચલ flp, Ast_ગાંઠ_પદાવલિ* અચલ countp,
                                          Ast_ગાંઠ_પદાવલિ*& overflowp, Ast_ગાંઠ_પદાવલિ* અચલ enablep) {
        જો (overflowp) {
            overflowp = નવું AstLogAnd{flp, enablep->clone_વૃક્ષ_Pure(ખોટું), overflowp};
        }
        પાછા addCountp(flp, nullptr, countp, enablep);
    }
    સ્થિર Ast_ગાંઠ_પદાવલિ* addCheckedCountp(File_લીટી* અચલ flp, Ast_ગાંઠ_પદાવલિ* અચલ totalCountp,
                                         Ast_ગાંઠ_પદાવલિ* અચલ contributionp, Ast_ગાંઠ_પદાવલિ*& overflowp,
                                         Ast_ગાંઠ_પદાવલિ* અચલ contributionOverflowp = nullptr) {
        જો (contributionOverflowp) { overflowp = orExprs(flp, overflowp, contributionOverflowp); }
        જો (!totalCountp) પાછા contributionp;
        Ast_ગાંઠ_પદાવલિ* અચલ oldTotalp = totalCountp->clone_વૃક્ષ_Pure(ખોટું);
        Ast_ગાંઠ_પદાવલિ* અચલ resultp = નવું AstAdd{flp, totalCountp, contributionp};
        overflowp
            = orExprs(flp, overflowp, નવું AstLt{flp, resultp->clone_વૃક્ષ_Pure(ખોટું), oldTotalp});
        પાછા resultp;
    }
    AstIf* new_ગણતરી_Overflow_Warning(File_લીટી* અચલ flp, Ast_ગાંઠ_પદાવલિ* અચલ condp) અચલ {
        AstDisplay* અચલ dispp = નવું AstDisplay{
            flp, VDisplayType::DT_WARNING,
            "Cover sequence match count overflowed 32-bit storage; results may be incorrect.",
            nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstIf* અચલ ifp = નવું AstIf{flp, condp, dispp};
        ifp->branchPred(VBranchPred::BP_UNLIKELY);
        પાછા ifp;
    }
    સ્થિર Ast_ગાંઠ_પદાવલિ* killActive(LowerCtx& c) {
        પાછા નવું AstNeq{c.flp, નવું AstVarRef{c.flp, c.killVarp, VAccess::READ},
                          assertKillGet(c.flp, c.assertType, c.directiveType)};
    }
    સ્થિર Ast_ગાંઠ_પદાવલિ* notKillActive(LowerCtx& c) { પાછા નવું AstLogNot{c.flp, killActive(c)}; }
    સ્થિર Ast_ગાંઠ_પદાવલિ* gateNotKill(LowerCtx& c, Ast_ગાંઠ_પદાવલિ* exprp) {
        જો (!exprp) પાછા nullptr;
        પાછા નવું AstLogAnd{c.flp, exprp, notKillActive(c)};
    }
    સ્થિર Ast_ગાંઠ_પદાવલિ* nextRingIndex(File_લીટી* flp, Ast_ગાંઠ_પદાવલિ* idxExprp, uint32_t size) {
        અચલ auto u32Const = [flp](uint32_t કિંમત) {
            પાછા નવું AstConst{flp, AstConst::Widthed_કિંમત{}, 32, કિંમત};
        };
        UASSERT_OBJ(size > 0, idxExprp, "Ring size must be positive");
        જો (size == 1) {
            VL_DO_DANGLING(idxExprp->delete_વૃક્ષ(), idxExprp);
            પાછા u32Const(0);
        }
        // idx == size - 1 ? 0 : idx + 1
        AstAdd* અચલ addp = નવું AstAdd{flp, idxExprp->clone_વૃક્ષ_Pure(ખોટું), u32Const(1)};
        addp->dtypeFrom(idxExprp);
        AstCond* અચલ condp
            = નવું AstCond{flp, નવું AstEq{flp, idxExprp, u32Const(size - 1)}, u32Const(0), addp};
        condp->dtypeFrom(idxExprp);
        પાછા condp;
    }
    સ્થિર Ast_ગાંઠ_પદાવલિ* delayRingSlot(File_લીટી* flp, SvaStateVertex* vtxp, Ast_ગાંઠ_પદાવલિ* idxExprp,
                                      VAccess access = VAccess::READ) {
        // ring[idx]
        AstVar* અચલ ringp = vtxp->datap()->delayRingVarp;
        AstVarRef* અચલ refp = નવું AstVarRef{flp, ringp, access};
        જો (vtxp->datap()->delayRingCounts) પાછા નવું AstArraySel{flp, refp, idxExprp};
        પાછા નવું AstSel{flp, refp, idxExprp, 1};
    }
    સ્થિર Ast_ગાંઠ_પદાવલિ* delay_Ring_Head_ગણતરી(File_લીટી* flp, SvaStateVertex* vtxp) {
        Ast_ગાંઠ_પદાવલિ* અચલ headp
            = નવું AstVarRef{flp, vtxp->datap()->delayRingHeadVarp, VAccess::READ};
        જો (vtxp->datap()->delayRingCounts) પાછા headp;
        પાછા નવું AstExtend{flp, headp, vtxp->datap()->delay_Ring_Live_ગણતરી_Varp->dtypep()->width()};
    }
    સ્થિર Ast_ગાંઠ_પદાવલિ* delayRingAtLastIndex(File_લીટી* અચલ flp, AstVar* અચલ idxp,
                                             અચલ uint32_t size) {
        પાછા નવું AstEq{flp, નવું AstVarRef{flp, idxp, VAccess::READ},
                         નવું AstConst{flp, AstConst::Widthed_કિંમત{}, 32, size - 1}};
    }
    સ્થિર Ast_ગાંઠ_પદાવલિ* delay_Ring_Output_ગણતરી(File_લીટી* અચલ flp, SvaStateVertex* અચલ vtxp) {
        AstVar* અચલ idxp = vtxp->datap()->delayRingIdxVarp;
        અચલ uint32_t size = vtxp->m_delayRingSize;
        Ast_ગાંઠ_પદાવલિ* outgoingValidp
            = નવું AstVarRef{flp, vtxp->datap()->delayRingWrappedVarp, VAccess::READ};
        જો (!vtxp->m_isFixedDelayRing) {
            outgoingValidp
                = નવું AstLogOr{flp, outgoingValidp, delayRingAtLastIndex(flp, idxp, size)};
        }
        AstVar* અચલ live_ગણતરી_Varp = vtxp->datap()->delay_Ring_Live_ગણતરી_Varp;
        પાછા નવું AstCond{flp, outgoingValidp, delay_Ring_Head_ગણતરી(flp, vtxp),
                           newTypedConstp(flp, live_ગણતરી_Varp->dtypep(), 0)};
    }
    સ્થિર Ast_ગાંઠ_પદાવલિ* delayRingHasLiveThreadsp(File_લીટી* અચલ flp,
                                                 AstVar* અચલ live_ગણતરી_Varp) {
        // active = live_count != 0;
        પાછા નવું AstNeq{flp, નવું AstVarRef{flp, live_ગણતરી_Varp, VAccess::READ},
                          newTypedConstp(flp, live_ગણતરી_Varp->dtypep(), 0)};
    }

    // Phase 3 output signals
    સંરચના SignalSet final {
        Ast_ગાંઠ_પદાવલિ* terminalActivep = nullptr;  // OR of all successful terminal matches
        Ast_ગાંઠ_પદાવલિ* matchCountp = nullptr;  // NFA paths completing the sequence this tick
        Ast_ગાંઠ_પદાવલિ* match_ગણતરી_Overflowp = nullptr;  // Match count overflow this tick
        Ast_ગાંઠ_પદાવલિ* rejectBasep = nullptr;  // Reject when a terminal match fails
        Ast_ગાંઠ_પદાવલિ* requiredStepRejectp = nullptr;  // Per-source reject from rejectOnFail Links
        Ast_ગાંઠ_પદાવલિ* throughoutRejectp = nullptr;  // Reject when a throughout guard drops
        Ast_ગાંઠ_પદાવલિ* threadFailCountp = nullptr;  // Number of threads rejected on this tick
    };

    // Disable condition also aborts attempts between clock events. A constant disable
    // never rises, and $sampled is unsupported in a sensitivity list.
    સ્થિર તાર્કિક isAsyncDisable(અચલ Ast_ગાંઠ_પદાવલિ* અચલ disableExprp) {
        પાછા disableExprp && !VN_IS(disableExprp, Const)
               && !disableExprp->exists([](અચલ AstSampled*) { પાછા સાચું; });
    }

    // Emit the clocked update block of registers that hold pending attempts. The clearp
    // statements reset the same registers: a disable rising between clock events also
    // aborts pending attempts (IEEE 1800-2023 16.12), so clearp runs on that rising edge
    // in a separate block. It must clear unconditionally, as the disable may already be
    // false again when it runs, and the update block must not run on that edge, as it
    // would advance the attempts as if the clock had ticked. Blocks that hold no attempt
    // state, such as the $assertkill acknowledge in emitKillAckNba, do not go through here.
    // Callers pass a null clearp when the disable cannot rise between clock events.
    શૂન્ય addStateAlways(LowerCtx& c, Ast_ગાંઠ* bodyp, Ast_ગાંઠ* clearp) {
        m_modp->addStmtsp(
            નવું AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_વૃક્ષ(ખોટું), bodyp});
        જો (!clearp) પાછા;
        m_modp->addStmtsp(નવું AstAlways{
            c.flp, VAlwaysKwd::ALWAYS,
            નવું Ast_Sen_વૃક્ષ{c.flp, નવું AstSenItem{c.flp, VEdgeType::ET_POSEDGE,
                                                 c.disableExprp->clone_વૃક્ષ_Pure(ખોટું)}},
            clearp});
    }

    // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
    // Phase 2: State register NBA always block. Each clocked-edge target
    // latches the OR of its incoming contributions.
    શૂન્ય emitStateRegisterNba(LowerCtx& c) {
        અચલ તાર્કિક asyncDisable = isAsyncDisable(c.disableExprp);
        Ast_ગાંઠ* bodyp = nullptr;
        Ast_ગાંઠ* clearp = nullptr;
        માટે (અચલ V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            અચલ SvaStateVertex* અચલ vtxp = static_cast<અચલ SvaStateVertex*>(&vtxr);
            જો (!vtxp->datap()->stateVarp) ચાલુ;

            Ast_ગાંઠ_પદાવલિ* nextStatep = nullptr;
            માટે (અચલ V3GraphEdge& edger : vtxp->inEdges()) {
                અચલ SvaTransEdge& tedger = static_cast<અચલ SvaTransEdge&>(edger);
                જો (!tedger.m_consumesCycle) ચાલુ;
                અચલ SvaStateVertex* અચલ fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Clocked-edge source missing stateSig");

                Ast_ગાંઠ_પદાવલિ* srcSigp = fromVtxp->datap()->stateSigp->clone_વૃક્ષ_Pure(ખોટું);
                srcSigp = andCond(c.flp, srcSigp, tedger.m_condp);

                જો (c.disableExprp) {
                    Ast_ગાંઠ_પદાવલિ* અચલ notDisp
                        = નવું AstLogNot{c.flp, c.disableExprp->clone_વૃક્ષ_Pure(ખોટું)};
                    srcSigp = નવું AstLogAnd{c.flp, srcSigp, notDisp};
                }
                nextStatep = orExprs(c.flp, nextStatep, srcSigp);
            }

            UASSERT_OBJ(nextStatep, vtxp,
                        "Registered vertex has no clocked incoming contribution");
            nextStatep = gateNotKill(c, nextStatep);

            AstAssignDly* અચલ assignp = નવું AstAssignDly{
                c.flp, નવું AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE}, nextStatep};
            bodyp = Ast_ગાંઠ::addNextNull(bodyp, assignp);
            જો (asyncDisable) {
                clearp = Ast_ગાંઠ::addNextNull(
                    clearp,
                    નવું AstAssignDly{
                        c.flp, નવું AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE},
                        નવું AstConst{c.flp, AstConst::BitFalse{}}});
            }
        }
        જો (bodyp) addStateAlways(c, bodyp, clearp);
    }

    // Phase 2b: In-place ring storage and registered ring metadata.
    શૂન્ય emitDelayRingUpdates(LowerCtx& c, અચલ તાર્કિક ગણતરી_Matches) {
        અચલ તાર્કિક asyncDisable = isAsyncDisable(c.disableExprp);
        માટે (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* અચલ vtxp = static_cast<SvaStateVertex*>(&vtxr);
            જો (!vtxp->datap()->delayRingVarp) ચાલુ;
            AstVar* અચલ idxp = vtxp->datap()->delayRingIdxVarp;
            AstVar* અચલ live_ગણતરી_Varp = vtxp->datap()->delay_Ring_Live_ગણતરી_Varp;
            AstVar* અચલ wrappedp = vtxp->datap()->delayRingWrappedVarp;
            અચલ uint32_t size = static_cast<uint32_t>(vtxp->m_delayRingSize);

            Ast_ગાંઠ_પદાવલિ* incomingp = nullptr;
            Ast_ગાંઠ_પદાવલિ* incomingOverflowp = nullptr;
            માટે (અચલ V3GraphEdge& edger : vtxp->inEdges()) {
                ++m_statDelayRingEdgeVisits;
                અચલ SvaTransEdge& tedger = static_cast<અચલ SvaTransEdge&>(edger);
                UASSERT_OBJ(tedger.m_consumesCycle == vtxp->m_isFixedDelayRing, vtxp,
                            "Delay-ring incoming edge kind mismatch");
                અચલ SvaStateVertex* અચલ fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Delay-ring incoming source missing stateSig");
                UASSERT_OBJ(!ગણતરી_Matches || fromVtxp->datap()->state_ગણતરી_Sigp, fromVtxp,
                            "Delay-ring incoming source missing stateCountSig");
                // IEEE 1800-2023 16.14.1/16.14.3: assertion alternatives belong to one
                // evaluation; only cover sequence counts each end-of-match.
                Ast_ગાંઠ_પદાવલિ* contribp
                    = ગણતરી_Matches
                          ? fromVtxp->datap()->state_ગણતરી_Sigp->clone_વૃક્ષ_Pure(ખોટું)
                          : નવું AstExtend{c.flp,
                                          fromVtxp->datap()->stateSigp->clone_વૃક્ષ_Pure(ખોટું),
                                          m_u32DTypep->width()};
                Ast_ગાંઠ_પદાવલિ* contribOverflowp
                    = ગણતરી_Matches && fromVtxp->datap()->state_ગણતરી_Overflow_Sigp
                          ? fromVtxp->datap()->state_ગણતરી_Overflow_Sigp->clone_વૃક્ષ_Pure(ખોટું)
                          : nullptr;
                જો (tedger.m_condp) {
                    Ast_ગાંઠ_પદાવલિ* અચલ enablep = tedger.m_condp->clone_વૃક્ષ_Pure(ખોટું);
                    contribp = ગણતરી_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, enablep)
                                   : addCountp(c.flp, nullptr, contribp, enablep);
                }
                જો (c.disableExprp) {
                    Ast_ગાંઠ_પદાવલિ* અચલ notDisp
                        = નવું AstLogNot{c.flp, c.disableExprp->clone_વૃક્ષ_Pure(ખોટું)};
                    contribp = ગણતરી_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, notDisp)
                                   : addCountp(c.flp, nullptr, contribp, notDisp);
                }
                incomingp = ગણતરી_Matches ? addCheckedCountp(c.flp, incomingp, contribp,
                                                            incomingOverflowp, contribOverflowp)
                                         : addCountp(c.flp, incomingp, contribp);
            }
            UASSERT_OBJ(incomingp, vtxp, "Delay ring has no incoming edge");
            અચલ તાર્કિક storesCounts = vtxp->datap()->delayRingCounts;
            Ast_ગાંઠ_પદાવલિ* અચલ incomingCountp
                = storesCounts
                      ? incomingp->clone_વૃક્ષ_Pure(ખોટું)
                      : નવું AstExtend{c.flp,
                                      નવું AstNeq{c.flp, incomingp->clone_વૃક્ષ_Pure(ખોટું),
                                                 newTypedConstp(c.flp, incomingp->dtypep(), 0)},
                                      m_u32DTypep->width()};
            Ast_ગાંઠ_પદાવલિ* અચલ storedIncomingp
                = storesCounts ? incomingp
                               : નવું AstNeq{c.flp, incomingp,
                                            newTypedConstp(c.flp, incomingp->dtypep(), 0)};
            // Only this block accesses the storage. Other processes read the cached head, so
            // the ring can be updated in place without an NBA shadow copy.
            AstAssign* અચલ writeIncomingp = નવું AstAssign{
                c.flp,
                delayRingSlot(c.flp, vtxp, નવું AstVarRef{c.flp, idxp, VAccess::READ},
                              VAccess::WRITE),
                storedIncomingp};
            Ast_ગાંઠ* updateBodyp = writeIncomingp;
            // live_count <= live_count - outgoing_count + incoming_count;
            Ast_ગાંઠ_પદાવલિ* અચલ outgoingp = delay_Ring_Output_ગણતરી(c.flp, vtxp);
            Ast_ગાંઠ_પદાવલિ* અચલ afterOutgoingp
                = નવું AstSub{c.flp, નવું AstVarRef{c.flp, live_ગણતરી_Varp, VAccess::READ}, outgoingp};
            Ast_ગાંઠ_પદાવલિ* અચલ oldAfterOutgoingp = afterOutgoingp->clone_વૃક્ષ_Pure(ખોટું);
            AstAdd* અચલ nextLiveCountp = નવું AstAdd{c.flp, afterOutgoingp, incomingCountp};
            જો (storesCounts) {
                Ast_ગાંઠ_પદાવલિ* અચલ liveOverflowp
                    = નવું AstLt{c.flp, nextLiveCountp->clone_વૃક્ષ_Pure(ખોટું), oldAfterOutgoingp};
                Ast_ગાંઠ_પદાવલિ* અચલ overflowp = orExprs(c.flp, incomingOverflowp, liveOverflowp);
                updateBodyp = new_ગણતરી_Overflow_Warning(c.flp, overflowp);
                updateBodyp->addNext(writeIncomingp);
            } નહીંતર {
                VL_DO_DANGLING(oldAfterOutgoingp->delete_વૃક્ષ(), oldAfterOutgoingp);
            }
            // Cache the outgoing slot for the next index after the write. Reading after
            // writing also handles one-slot rings and two-slot range rings without alias cases.
            Ast_ગાંઠ_પદાવલિ* nextHeadIdxp
                = nextRingIndex(c.flp, નવું AstVarRef{c.flp, idxp, VAccess::READ}, size);
            જો (!vtxp->m_isFixedDelayRing) {
                nextHeadIdxp = nextRingIndex(c.flp, nextHeadIdxp, size);
            }
            updateBodyp->addNext(નવું AstAssignDly{
                c.flp, નવું AstVarRef{c.flp, vtxp->datap()->delayRingHeadVarp, VAccess::WRITE},
                delayRingSlot(c.flp, vtxp, nextHeadIdxp)});
            updateBodyp->addNext(નવું AstAssignDly{
                c.flp, નવું AstVarRef{c.flp, live_ગણતરી_Varp, VAccess::WRITE}, nextLiveCountp});
            // wrapped <= wrapped || idx == size - 1;
            updateBodyp->addNext(
                નવું AstAssignDly{c.flp, નવું AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                 નવું AstLogOr{c.flp, નવું AstVarRef{c.flp, wrappedp, VAccess::READ},
                                              delayRingAtLastIndex(c.flp, idxp, size)}});
            // idx <= next_idx;
            updateBodyp->addNext(નવું AstAssignDly{
                c.flp, નવું AstVarRef{c.flp, idxp, VAccess::WRITE},
                nextRingIndex(c.flp, નવું AstVarRef{c.flp, idxp, VAccess::READ}, size)});
            જો (vtxp->m_delayRingAdvanceCondp) {
                updateBodyp = નવું AstIf{
                    c.flp, sampled(vtxp->m_delayRingAdvanceCondp->clone_વૃક્ષ_Pure(ખોટું)),
                    updateBodyp};
            }

            Ast_ગાંઠ_પદાવલિ* clearCondp = killActive(c);
            જો (vtxp->m_delayRingClearCondp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_delayRingClearCondp->clone_વૃક્ષ_Pure(ખોટું)));
            }
            જો (vtxp->m_abortClearp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_abortClearp->clone_વૃક્ષ_Pure(ખોટું)));
            }
            જો (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_વૃક્ષ_Pure(ખોટું));
            }
            Ast_ગાંઠ_પદાવલિ* guardp = nullptr;
            માટે (Ast_ગાંઠ_પદાવલિ* અચલ cp : vtxp->m_throughoutConds) {
                Ast_ગાંઠ_પદાવલિ* અચલ sampledp = sampled(cp->clone_વૃક્ષ_Pure(ખોટું));
                guardp = guardp ? static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstLogAnd{c.flp, guardp, sampledp})
                                : sampledp;
            }
            જો (guardp) clearCondp = orExprs(c.flp, clearCondp, નવું AstLogNot{c.flp, guardp});
            // Logically clear the ring without touching its wide storage.
            AstAssignDly* અચલ clearCountp
                = નવું AstAssignDly{c.flp, નવું AstVarRef{c.flp, live_ગણતરી_Varp, VAccess::WRITE},
                                   newTypedConstp(c.flp, live_ગણતરી_Varp->dtypep(), 0)};
            clearCountp->addNext(નવું AstAssignDly{c.flp,
                                                  નવું AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                                  નવું AstConst{c.flp, AstConst::BitFalse{}}});
            clearCountp->addNext(નવું AstAssignDly{c.flp,
                                                  નવું AstVarRef{c.flp, idxp, VAccess::WRITE},
                                                  newTypedConstp(c.flp, idxp->dtypep(), 0)});
            updateBodyp = નવું AstIf{c.flp, clearCondp, clearCountp, updateBodyp};

            addStateAlways(c, updateBodyp, asyncDisable ? clearCountp->clone_વૃક્ષ(સાચું) : nullptr);
        }
    }

    // Phase 2c: SAnd combiner done-latch always block.
    // NBA semantics ensure doneL/doneR read pre-update values (IEEE 16.9.5).
    શૂન્ય emitAndCombinerDoneLatchNba(LowerCtx& c) {
        અચલ તાર્કિક asyncDisable = isAsyncDisable(c.disableExprp);
        માટે (અચલ V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            અચલ SvaStateVertex* અચલ avp = static_cast<અચલ SvaStateVertex*>(&vtxr);
            જો (!avp->datap()->doneLVarp) ચાલુ;
            // doneLVars is non-null only for AndCombiner vertices, which always
            // have both m_andLhsTermp and m_andRhsTermp set at build time.
            UASSERT_OBJ(avp->m_andLhsTermp && avp->m_andRhsTermp, avp,
                        "AndCombiner vertex missing LHS/RHS terminal");
            અચલ SvaStateVertex* અચલ lhsTermp = avp->m_andLhsTermp;
            અચલ SvaStateVertex* અચલ rhsTermp = avp->m_andRhsTermp;
            // resolveLinks' 2*N+2 fixed-point pass is guaranteed to populate
            // stateSigp on every AndCombiner and its LHS/RHS terminals.
            UASSERT_OBJ(lhsTermp->datap()->stateSigp && rhsTermp->datap()->stateSigp
                            && avp->datap()->stateSigp,
                        avp, "AndCombiner stateSigp chain unresolved after resolveLinks");

            AstAssignDly* અચલ clearLp = નવું AstAssignDly{
                c.flp, નવું AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                નવું AstConst{c.flp, AstConst::BitFalse{}}};
            AstAssignDly* અચલ clearRp = નવું AstAssignDly{
                c.flp, નવું AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                નવું AstConst{c.flp, AstConst::BitFalse{}}};
            clearLp->addNext(clearRp);

            Ast_ગાંઠ_પદાવલિ* અચલ matchLNowp
                = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, avp->m_andLhsCondp);
            Ast_ગાંઠ_પદાવલિ* અચલ matchRNowp
                = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, avp->m_andRhsCondp);
            Ast_ગાંઠ_પદાવલિ* gateLp = matchLNowp;
            Ast_ગાંઠ_પદાવલિ* gateRp = matchRNowp;
            જો (c.disableExprp) {
                Ast_ગાંઠ_પદાવલિ* અચલ notDisLp
                    = નવું AstLogNot{c.flp, c.disableExprp->clone_વૃક્ષ_Pure(ખોટું)};
                gateLp = નવું AstLogAnd{c.flp, gateLp, notDisLp};
                Ast_ગાંઠ_પદાવલિ* અચલ notDisRp
                    = નવું AstLogNot{c.flp, c.disableExprp->clone_વૃક્ષ_Pure(ખોટું)};
                gateRp = નવું AstLogAnd{c.flp, gateRp, notDisRp};
            }
            AstAssignDly* અચલ setLp = નવું AstAssignDly{
                c.flp, નવું AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                નવું AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* અચલ setLIfp = નવું AstIf{c.flp, gateLp, setLp, nullptr};
            AstAssignDly* અચલ setRp = નવું AstAssignDly{
                c.flp, નવું AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                નવું AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* અચલ setRIfp = નવું AstIf{c.flp, gateRp, setRp, nullptr};
            setLIfp->addNext(setRIfp);

            Ast_ગાંઠ_પદાવલિ* clearCondp = નવું AstLogOr{c.flp, killActive(c),
                                                   avp->datap()->stateSigp->clone_વૃક્ષ_Pure(ખોટું)};
            જો (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_વૃક્ષ_Pure(ખોટું));
            }
            AstIf* અચલ topp = નવું AstIf{c.flp, clearCondp, clearLp, setLIfp};
            addStateAlways(c, topp, asyncDisable ? clearLp->clone_વૃક્ષ(સાચું) : nullptr);
        }
    }

    શૂન્ય emitKillAckNba(LowerCtx& c) {
        AstAssignDly* અચલ ackp
            = નવું AstAssignDly{c.flp, નવું AstVarRef{c.flp, c.killVarp, VAccess::WRITE},
                               assertKillGet(c.flp, c.assertType, c.directiveType)};
        m_modp->addStmtsp(
            નવું AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_વૃક્ષ(ખોટું), ackp});
    }

    // Phase 3/3a/3b: Compute terminal match/reject signals, required-step reject,
    // throughout-drop reject; clean up intermediate state signals.
    // Phase 3: terminalActive and rejectBase from Links to matchVertex.
    // Builder only adds Links (non-clocked) to matchVertex via addLink in
    // wireMatchAndMidSources. For cover sequence, also count each end-of-match
    // so the action can be replayed without unrolling endpoints.
    શૂન્ય computeTerminalMatchAndReject(LowerCtx& c, SignalSet& sigs, અચલ તાર્કિક need_Match_ગણતરી) {
        માટે (અચલ SvaTransEdge* અચલ tedgep : c.graph.allEdges()) {
            જો (tedgep->toVtxp() != c.graph.m_matchVertexp) ચાલુ;
            SvaStateVertex* અચલ fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                        "Terminal-link source missing stateSig");

            Ast_ગાંઠ_પદાવલિ* srcSigp = fromVtxp->datap()->stateSigp->clone_વૃક્ષ_Pure(ખોટું);
            srcSigp = andCond(c.flp, srcSigp, tedgep->m_condp);
            જો (need_Match_ગણતરી) {
                UASSERT_OBJ(fromVtxp->datap()->state_ગણતરી_Sigp, fromVtxp,
                            "Terminal-link source missing stateCountSig");
                Ast_ગાંઠ_પદાવલિ* contributionp
                    = fromVtxp->datap()->state_ગણતરી_Sigp->clone_વૃક્ષ_Pure(ખોટું);
                Ast_ગાંઠ_પદાવલિ* contributionOverflowp
                    = fromVtxp->datap()->state_ગણતરી_Overflow_Sigp
                          ? fromVtxp->datap()->state_ગણતરી_Overflow_Sigp->clone_વૃક્ષ_Pure(ખોટું)
                          : nullptr;
                જો (tedgep->m_condp) {
                    contributionp = gateCheckedCountp(c.flp, contributionp, contributionOverflowp,
                                                      tedgep->m_condp->clone_વૃક્ષ_Pure(ખોટું));
                }
                sigs.matchCountp
                    = addCheckedCountp(c.flp, sigs.matchCountp, contributionp,
                                       sigs.match_ગણતરી_Overflowp, contributionOverflowp);
            }

            જો (fromVtxp->m_delayRingSize && !fromVtxp->m_isFixedDelayRing) {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_વૃક્ષ_Pure(ખોટું));
                // reject |= (wrapped || idx == size - 1) && head && final_condition;
                Ast_ગાંઠ_પદાવલિ* expireContribp = delay_Ring_Output_ગણતરી(c.flp, fromVtxp);
                expireContribp = andCond(c.flp, expireContribp, tedgep->m_condp);
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, expireContribp);
                VL_DO_DANGLING(srcSigp->delete_વૃક્ષ(), srcSigp);
            } નહીંતર જો (fromVtxp->m_isUnbounded || fromVtxp->m_isAndCombiner) {
                sigs.terminalActivep = orExprs(c.flp, sigs.terminalActivep, srcSigp);
            } નહીંતર {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_વૃક્ષ_Pure(ખોટું));
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, srcSigp);
            }
        }
        // wireMatchAndMidSources always adds a Link from result.termVertexp
        // to m_matchVertexp, so the loop above always sets terminalActivep.
        UASSERT_OBJ(sigs.terminalActivep, c.graph.m_matchVertexp,
                    "No terminal edge to match vertex");
    }

    Ast_ગાંઠ_પદાવલિ* newThroughoutThreadFailCountp(LowerCtx& c, AstVar* અચલ delay_Ring_Live_ગણતરી_Varp,
                                               Ast_ગાંઠ_પદાવલિ* અચલ stateExprp,
                                               Ast_ગાંઠ_પદાવલિ* અચલ enablep) {
        Ast_ગાંઠ_પદાવલિ* અચલ activeThreadCountp
            = delay_Ring_Live_ગણતરી_Varp
                  ? static_cast<Ast_ગાંઠ_પદાવલિ*>(
                        નવું AstVarRef{c.flp, delay_Ring_Live_ગણતરી_Varp, VAccess::READ})
                  : નવું AstExtend{c.flp, stateExprp->clone_વૃક્ષ_Pure(ખોટું), m_u32DTypep->width()};
        પાછા addCountp(c.flp, nullptr, activeThreadCountp, enablep);
    }

    // Phase 3b: Throughout-drop and ring-wide abort rejection.
    શૂન્ય computeThroughoutReject(LowerCtx& c, SignalSet& sigs, અચલ તાર્કિક need_Thread_Fail_ગણતરી) {
        માટે (અચલ V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            અચલ SvaStateVertex* અચલ vtxp = static_cast<અચલ SvaStateVertex*>(&vtxr);
            અચલ auto& conds = vtxp->m_throughoutConds;
            જો (conds.empty() && !vtxp->m_abortRejectp) ચાલુ;
            જો (vtxp->m_isAndCombiner) ચાલુ;
            Ast_ગાંઠ_પદાવલિ* stateExprp = nullptr;
            જો (vtxp->datap()->stateVarp) {
                stateExprp = નવું AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
            } નહીંતર જો (vtxp->datap()->delayRingVarp && vtxp->m_isFixedDelayRing) {
                stateExprp
                    = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_ગણતરી_Varp);
            } નહીંતર {
                UASSERT_OBJ(vtxp->datap()->stateSigp, vtxp,
                            "Throughout-conds vertex missing state representation");
                stateExprp = vtxp->datap()->stateSigp->clone_વૃક્ષ_Pure(ખોટું);
            }
            Ast_ગાંઠ_પદાવલિ* guardp = nullptr;
            માટે (Ast_ગાંઠ_પદાવલિ* અચલ cp : conds) {
                Ast_ગાંઠ_પદાવલિ* અચલ sp = sampled(cp->clone_વૃક્ષ_Pure(ખોટું));
                guardp = guardp ? static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstLogAnd{c.flp, guardp, sp}) : sp;
            }
            જો (vtxp->m_abortRejectp) {
                Ast_ગાંઠ_પદાવલિ* અચલ notAbortp
                    = નવું AstLogNot{c.flp, sampled(vtxp->m_abortRejectp->clone_વૃક્ષ_Pure(ખોટું))};
                guardp = guardp
                             ? static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstLogAnd{c.flp, guardp, notAbortp})
                             : notAbortp;
            }
            Ast_ગાંઠ_પદાવલિ* rejectCondp = નવું AstLogNot{c.flp, guardp};
            જો (vtxp->m_abortClearp) {
                // Any abort clears the ring. Accept aborts suppress a simultaneous guard failure;
                // reject aborts are restored below so they still force rejection.
                Ast_ગાંઠ_પદાવલિ* અચલ notAbortClearp
                    = નવું AstLogNot{c.flp, sampled(vtxp->m_abortClearp->clone_વૃક્ષ_Pure(ખોટું))};
                rejectCondp = નવું AstLogAnd{c.flp, rejectCondp, notAbortClearp};
                જો (vtxp->m_abortRejectp) {
                    rejectCondp = નવું AstLogOr{
                        c.flp, rejectCondp, sampled(vtxp->m_abortRejectp->clone_વૃક્ષ_Pure(ખોટું))};
                }
            }
            જો (need_Thread_Fail_ગણતરી) {
                Ast_ગાંઠ_પદાવલિ* અચલ contributionp
                    = newThroughoutThreadFailCountp(c, vtxp->datap()->delay_Ring_Live_ગણતરી_Varp,
                                                    stateExprp, rejectCondp->clone_વૃક્ષ_Pure(ખોટું));
                sigs.threadFailCountp = addCountp(c.flp, sigs.threadFailCountp, contributionp);
                જો (vtxp->m_abortRejectp && vtxp->m_delayRingAdvanceCondp) {
                    // An advancing ring thread also occupies its same-tick match vertex in the
                    // unrolled NFA, so abort rejection must replay both fail actions.
                    Ast_ગાંઠ_પદાવલિ* અચલ abortAndAdvancep = નવું AstLogAnd{
                        c.flp, sampled(vtxp->m_abortRejectp->clone_વૃક્ષ_Pure(ખોટું)),
                        sampled(vtxp->m_delayRingAdvanceCondp->clone_વૃક્ષ_Pure(ખોટું))};
                    Ast_ગાંઠ_પદાવલિ* અચલ matchContributionp = newThroughoutThreadFailCountp(
                        c, vtxp->datap()->delay_Ring_Live_ગણતરી_Varp, stateExprp, abortAndAdvancep);
                    sigs.threadFailCountp
                        = addCountp(c.flp, sigs.threadFailCountp, matchContributionp);
                }
            }
            sigs.throughoutRejectp = orExprs(c.flp, sigs.throughoutRejectp,
                                             નવું AstLogAnd{c.flp, stateExprp, rejectCondp});
        }
    }

    SignalSet computeSignals(LowerCtx& c, અચલ તાર્કિક need_Thread_Fail_ગણતરી,
                             અચલ તાર્કિક need_Throughout_Thread_Fail_ગણતરી, અચલ તાર્કિક need_Match_ગણતરી) {
        SignalSet sigs;

        computeTerminalMatchAndReject(c, sigs, need_Match_ગણતરી);

        // Phase 3a: required-step rejection.
        // Builder only sets m_rejectOnFail on non-clocked Links with m_condp
        // or m_condVtxp, and the source always has a resolved stateSig.
        માટે (અચલ SvaTransEdge* અચલ tedgep : c.graph.allEdges()) {
            જો (!tedgep->m_rejectOnFail) ચાલુ;
            અચલ SvaStateVertex* અચલ fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp && (tedgep->m_condp || tedgep->m_condVtxp),
                        fromVtxp,
                        "rejectOnFail Link must have condp/condVtxp and source stateSig");
            Ast_ગાંઠ_પદાવલિ* અચલ srcSigp = fromVtxp->datap()->stateSigp->clone_વૃક્ષ_Pure(ખોટું);
            Ast_ગાંઠ_પદાવલિ* condp = nullptr;
            જો (tedgep->m_condVtxp) {
                UASSERT_OBJ(tedgep->m_condVtxp->datap()->stateSigp, tedgep->m_condVtxp,
                            "rejectOnFail condVtxp missing stateSig");
                condp = tedgep->m_condVtxp->datap()->stateSigp->clone_વૃક્ષ_Pure(ખોટું);
                જો (tedgep->m_condp) {
                    condp = નવું AstLogOr{c.flp, condp, tedgep->m_condp->clone_વૃક્ષ_Pure(ખોટું)};
                }
            } નહીંતર {
                condp = tedgep->m_condp->clone_વૃક્ષ_Pure(ખોટું);
            }
            Ast_ગાંઠ_પદાવલિ* અચલ notCondp = નવું AstLogNot{c.flp, condp};
            Ast_ગાંઠ_પદાવલિ* અચલ rawFailp = નવું AstLogAnd{c.flp, srcSigp, notCondp};
            જો (need_Thread_Fail_ગણતરી) {
                // thread_fail_count += fail;
                sigs.threadFailCountp = addCountp(
                    c.flp, sigs.threadFailCountp,
                    નવું AstExtend{c.flp, rawFailp->clone_વૃક્ષ_Pure(ખોટું), m_u32DTypep->width()});
            }
            Ast_ગાંઠ_પદાવલિ* અચલ failp = gateNotKill(c, rawFailp);
            sigs.requiredStepRejectp = orExprs(c.flp, sigs.requiredStepRejectp, failp);
        }

        computeThroughoutReject(c, sigs, need_Throughout_Thread_Fail_ગણતરી);
        જો (sigs.threadFailCountp) {
            sigs.threadFailCountp
                = addCountp(c.flp, nullptr, sigs.threadFailCountp, notKillActive(c));
        }
        જો (sigs.matchCountp) {
            જો (c.matchCondp) {
                sigs.matchCountp
                    = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_ગણતરી_Overflowp,
                                        sampled(c.matchCondp->clone_વૃક્ષ_Pure(ખોટું)));
            }
            sigs.matchCountp = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_ગણતરી_Overflowp,
                                                 notKillActive(c));
        }
        sigs.terminalActivep = gateNotKill(c, sigs.terminalActivep);
        sigs.rejectBasep = gateNotKill(c, sigs.rejectBasep);
        sigs.throughoutRejectp = gateNotKill(c, sigs.throughoutRejectp);

        // Clean up intermediate state signals. These are orphan subtrees
        // (never linked into the enclosing AST); deleteTree() is immediate
        // which is what we want since stateSigp lifetime ends with this scope.
        માટે (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* અચલ vtxp = static_cast<SvaStateVertex*>(&vtxr);
            Ast_ગાંઠ_પદાવલિ*& sigp = vtxp->datap()->stateSigp;
            જો (sigp) VL_DO_DANGLING(sigp->delete_વૃક્ષ(), sigp);
            Ast_ગાંઠ_પદાવલિ*& countp = vtxp->datap()->state_ગણતરી_Sigp;
            જો (countp) VL_DO_DANGLING(countp->delete_વૃક્ષ(), countp);
            Ast_ગાંઠ_પદાવલિ*& overflowp = vtxp->datap()->state_ગણતરી_Overflow_Sigp;
            જો (overflowp) VL_DO_DANGLING(overflowp->delete_વૃક્ષ(), overflowp);
        }
        // Disable iff gating (IEEE 1800-2023 16.12) on the current level value.
        જો (c.disableExprp) {
            // terminalActivep is always set, so gate it unconditionally.
            Ast_ગાંઠ_પદાવલિ* અચલ notTermp
                = નવું AstLogNot{c.flp, c.disableExprp->clone_વૃક્ષ_Pure(ખોટું)};
            sigs.terminalActivep = નવું AstLogAnd{c.flp, sigs.terminalActivep, notTermp};
            જો (sigs.rejectBasep) {
                Ast_ગાંઠ_પદાવલિ* અચલ notDisp
                    = નવું AstLogNot{c.flp, c.disableExprp->clone_વૃક્ષ_Pure(ખોટું)};
                sigs.rejectBasep = નવું AstLogAnd{c.flp, sigs.rejectBasep, notDisp};
            }
            જો (sigs.throughoutRejectp) {
                Ast_ગાંઠ_પદાવલિ* અચલ notDisp
                    = નવું AstLogNot{c.flp, c.disableExprp->clone_વૃક્ષ_Pure(ખોટું)};
                sigs.throughoutRejectp = નવું AstLogAnd{c.flp, sigs.throughoutRejectp, notDisp};
            }
            જો (sigs.requiredStepRejectp) {
                Ast_ગાંઠ_પદાવલિ* અચલ notDisp
                    = નવું AstLogNot{c.flp, c.disableExprp->clone_વૃક્ષ_Pure(ખોટું)};
                sigs.requiredStepRejectp = નવું AstLogAnd{c.flp, sigs.requiredStepRejectp, notDisp};
            }
            જો (sigs.matchCountp) {
                sigs.matchCountp = gateCheckedCountp(
                    c.flp, sigs.matchCountp, sigs.match_ગણતરી_Overflowp,
                    નવું AstLogNot{c.flp, c.disableExprp->clone_વૃક્ષ_Pure(ખોટું)});
            }
        }

        જો (sigs.match_ગણતરી_Overflowp) {
            m_modp->addStmtsp(
                નવું AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_વૃક્ષ(ખોટું),
                              new_ગણતરી_Overflow_Warning(c.flp, sigs.match_ગણતરી_Overflowp)});
            sigs.match_ગણતરી_Overflowp = nullptr;
        }

        પાછા sigs;
    }

    // Phase 1: Resolve combinational Links via fixed-point propagation.
    શૂન્ય resolveLinks(LowerCtx& c, Ast_ગાંઠ_પદાવલિ* triggerExprp, અચલ તાર્કિક ગણતરી_Matches) {
        // datap() was freshly allocated in lower() -- all state signals start null.
        c.graph.m_startVertexp->datap()->stateSigp = triggerExprp->clone_વૃક્ષ_Pure(ખોટું);
        c.graph.m_startVertexp->datap()->state_ગણતરી_Sigp
            = નવું AstExtend{c.flp, triggerExprp->clone_વૃક્ષ_Pure(ખોટું), m_u32DTypep->width()};
        માટે (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* અચલ vtxp = static_cast<SvaStateVertex*>(&vtxr);
            જો (vtxp->datap()->stateVarp) {
                vtxp->datap()->stateSigp
                    = નવું AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
                vtxp->datap()->state_ગણતરી_Sigp = નવું AstExtend{
                    c.flp, નવું AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ},
                    m_u32DTypep->width()};
            } નહીંતર જો (vtxp->datap()->delayRingVarp) {
                જો (vtxp->m_isFixedDelayRing) {
                    // state_count = wrapped ? head : 0; state = state_count != 0;
                    Ast_ગાંઠ_પદાવલિ* અચલ countp = delay_Ring_Output_ગણતરી(c.flp, vtxp);
                    vtxp->datap()->stateSigp = નવું AstNeq{c.flp, countp->clone_વૃક્ષ_Pure(ખોટું),
                                                          newTypedConstp(c.flp, m_u32DTypep, 0)};
                    vtxp->datap()->state_ગણતરી_Sigp = countp;
                } નહીંતર {
                    vtxp->datap()->stateSigp
                        = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_ગણતરી_Varp);
                    vtxp->datap()->state_ગણતરી_Sigp = નવું AstVarRef{
                        c.flp, vtxp->datap()->delay_Ring_Live_ગણતરી_Varp, VAccess::READ};
                }
            }
        }
        // Fixed-point propagation along zero-delay (Link) edges. Rebuild each
        // derived signal from its incoming edges on every pass; appending the same
        // contributions repeatedly makes merge expressions grow exponentially.
        અચલ size_t passes = 2 * c.graph.m_graph.vertices().size() + 2;
        માટે (size_t pass = 0; pass < passes; ++pass) {
            // Rebuild SAnd combiners once both sub-NFA terminals are available.
            માટે (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* અચલ vtxp = static_cast<SvaStateVertex*>(&vtxr);
                જો (!vtxp->m_isAndCombiner) ચાલુ;
                // AndCombiner vertices always have both terminal pointers set.
                UASSERT_OBJ(vtxp->m_andLhsTermp && vtxp->m_andRhsTermp, vtxp,
                            "AndCombiner vertex missing LHS/RHS terminal");
                અચલ SvaStateVertex* અચલ lhsTermp = vtxp->m_andLhsTermp;
                અચલ SvaStateVertex* અચલ rhsTermp = vtxp->m_andRhsTermp;
                જો (!lhsTermp->datap()->stateSigp || !rhsTermp->datap()->stateSigp) ચાલુ;
                Ast_ગાંઠ_પદાવલિ* અચલ matchLp
                    = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, vtxp->m_andLhsCondp);
                Ast_ગાંઠ_પદાવલિ* અચલ matchRp
                    = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, vtxp->m_andRhsCondp);
                Ast_ગાંઠ_પદાવલિ* અચલ doneLOrp = નવું AstLogOr{
                    c.flp, નવું AstVarRef{c.flp, vtxp->datap()->doneLVarp, VAccess::READ}, matchLp};
                Ast_ગાંઠ_પદાવલિ* અચલ doneROrp = નવું AstLogOr{
                    c.flp, નવું AstVarRef{c.flp, vtxp->datap()->doneRVarp, VAccess::READ}, matchRp};
                Ast_ગાંઠ_પદાવલિ* અચલ bothp = નવું AstLogAnd{c.flp, doneLOrp, doneROrp};
                Ast_ગાંઠ_પદાવલિ* અચલ oneNowp = નવું AstLogOr{c.flp, matchLp->clone_વૃક્ષ_Pure(ખોટું),
                                                          matchRp->clone_વૃક્ષ_Pure(ખોટું)};
                જો (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_વૃક્ષ(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = નવું AstLogAnd{c.flp, bothp, oneNowp};
                જો (vtxp->datap()->state_ગણતરી_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_ગણતરી_Sigp->delete_વૃક્ષ(),
                                   vtxp->datap()->state_ગણતરી_Sigp);
                }
                vtxp->datap()->state_ગણતરી_Sigp = નવું AstExtend{
                    c.flp, vtxp->datap()->stateSigp->clone_વૃક્ષ_Pure(ખોટું), m_u32DTypep->width()};
            }

            માટે (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* અચલ vtxp = static_cast<SvaStateVertex*>(&vtxr);
                જો (vtxp == c.graph.m_startVertexp || vtxp->datap()->stateVarp
                    || vtxp->datap()->delayRingVarp || vtxp->m_isAndCombiner || vtxp->m_isMatch
                    || vtxp->m_isRejectSink) {
                    ચાલુ;
                }
                Ast_ગાંઠ_પદાવલિ* nextStatep = nullptr;
                Ast_ગાંઠ_પદાવલિ* nextStateCountp = nullptr;
                Ast_ગાંઠ_પદાવલિ* next_State_ગણતરી_Overflowp = nullptr;
                માટે (અચલ V3GraphEdge& er : vtxp->inEdges()) {
                    અચલ SvaTransEdge& te = static_cast<અચલ SvaTransEdge&>(er);
                    અચલ SvaStateVertex* અચલ fromVtxp = te.fromVtxp();
                    જો (!fromVtxp->datap()->stateSigp) ચાલુ;
                    UASSERT_OBJ(fromVtxp->datap()->state_ગણતરી_Sigp, fromVtxp,
                                "Link source missing stateCountSig");
                    Ast_ગાંઠ_પદાવલિ* અચલ contributionp = andCond(
                        c.flp, fromVtxp->datap()->stateSigp->clone_વૃક્ષ_Pure(ખોટું), te.m_condp);
                    nextStatep = orExprs(c.flp, nextStatep, contributionp);
                    Ast_ગાંઠ_પદાવલિ* ગણતરી_Contributionp
                        = fromVtxp->datap()->state_ગણતરી_Sigp->clone_વૃક્ષ_Pure(ખોટું);
                    Ast_ગાંઠ_પદાવલિ* ગણતરી_Contribution_Overflowp
                        = ગણતરી_Matches && fromVtxp->datap()->state_ગણતરી_Overflow_Sigp
                              ? fromVtxp->datap()->state_ગણતરી_Overflow_Sigp->clone_વૃક્ષ_Pure(ખોટું)
                              : nullptr;
                    જો (te.m_condp) {
                        Ast_ગાંઠ_પદાવલિ* અચલ enablep = te.m_condp->clone_વૃક્ષ_Pure(ખોટું);
                        ગણતરી_Contributionp
                            = ગણતરી_Matches
                                  ? gateCheckedCountp(c.flp, ગણતરી_Contributionp,
                                                      ગણતરી_Contribution_Overflowp, enablep)
                                  : addCountp(c.flp, nullptr, ગણતરી_Contributionp, enablep);
                    }
                    nextStateCountp
                        = ગણતરી_Matches
                              ? addCheckedCountp(c.flp, nextStateCountp, ગણતરી_Contributionp,
                                                 next_State_ગણતરી_Overflowp,
                                                 ગણતરી_Contribution_Overflowp)
                              : addCountp(c.flp, nextStateCountp, ગણતરી_Contributionp);
                }
                જો (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_વૃક્ષ(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = nextStatep;
                જો (vtxp->datap()->state_ગણતરી_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_ગણતરી_Sigp->delete_વૃક્ષ(),
                                   vtxp->datap()->state_ગણતરી_Sigp);
                }
                vtxp->datap()->state_ગણતરી_Sigp = nextStateCountp;
                જો (vtxp->datap()->state_ગણતરી_Overflow_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_ગણતરી_Overflow_Sigp->delete_વૃક્ષ(),
                                   vtxp->datap()->state_ગણતરી_Overflow_Sigp);
                }
                vtxp->datap()->state_ગણતરી_Overflow_Sigp = next_State_ગણતરી_Overflowp;
            }
        }
    }

જાહેર:
    // Combine terminal/reject signals into final output expression.
    સ્થિર Ast_ગાંઠ_પદાવલિ* assembleResult(Ast_ગાંઠ_Cover_Or_Assert* અચલ assertp, અચલ તાર્કિક negated,
                                       Ast_ગાંઠ_પદાવલિ* અચલ matchCondp, અચલ SignalSet& sigs,
                                       Ast_ગાંઠ_પદાવલિ** અચલ outMatchpp) {
        File_લીટી* અચલ flp = assertp->fileline();
        અચલ તાર્કિક isCover = VN_IS(assertp, Cover);
        Ast_ગાંઠ_પદાવલિ* અચલ terminalActivep = sigs.terminalActivep;
        Ast_ગાંઠ_પદાવલિ* અચલ rejectBasep = sigs.rejectBasep;
        Ast_ગાંઠ_પદાવલિ* અચલ throughoutRejectp = sigs.throughoutRejectp;
        Ast_ગાંઠ_પદાવલિ* અચલ requiredStepRejectp = sigs.requiredStepRejectp;
        // Property negation (IEEE 1800-2023 16.12.1 `not`): invert match/reject.
        જો (negated) {
            જો (isCover) {
                જો (terminalActivep)
                    VL_DO_DANGLING(terminalActivep->delete_વૃક્ષ(), terminalActivep);
                Ast_ગાંઠ_પદાવલિ* negRejectp = nullptr;
                જો (matchCondp && rejectBasep) {
                    Ast_ગાંઠ_પદાવલિ* અચલ sampledCondp = sampled(matchCondp->clone_વૃક્ષ_Pure(ખોટું));
                    Ast_ગાંઠ_પદાવલિ* અચલ notCondp = નવું AstLogNot{flp, sampledCondp};
                    negRejectp = નવું AstLogAnd{flp, rejectBasep, notCondp};
                } નહીંતર જો (rejectBasep) {
                    VL_DO_DANGLING(rejectBasep->delete_વૃક્ષ(), rejectBasep);
                }
                જો (throughoutRejectp) negRejectp = orExprs(flp, negRejectp, throughoutRejectp);
                જો (requiredStepRejectp)
                    negRejectp = orExprs(flp, negRejectp, requiredStepRejectp);
                પાછા negRejectp ? negRejectp : નવું AstConst{flp, AstConst::BitFalse{}};
            }
            // Negated assert/assume: output = !match.
            Ast_ગાંઠ_પદાવલિ* matchp = terminalActivep;
            જો (matchCondp) {
                Ast_ગાંઠ_પદાવલિ* અચલ sampledCondp = sampled(matchCondp->clone_વૃક્ષ_Pure(ખોટું));
                matchp = નવું AstLogAnd{flp, matchp, sampledCondp};
            }
            જો (outMatchpp) {
                Ast_ગાંઠ_પદાવલિ* notPMatchp = nullptr;
                જો (matchCondp && rejectBasep) {
                    Ast_ગાંઠ_પદાવલિ* અચલ sampledCondp = sampled(matchCondp->clone_વૃક્ષ_Pure(ખોટું));
                    notPMatchp = નવું AstLogAnd{flp, rejectBasep->clone_વૃક્ષ_Pure(ખોટું),
                                               નવું AstLogNot{flp, sampledCondp}};
                } નહીંતર જો (rejectBasep) {
                    notPMatchp = rejectBasep->clone_વૃક્ષ_Pure(ખોટું);
                }
                જો (throughoutRejectp)
                    notPMatchp = orExprs(flp, notPMatchp, throughoutRejectp->clone_વૃક્ષ_Pure(ખોટું));
                જો (requiredStepRejectp)
                    notPMatchp
                        = orExprs(flp, notPMatchp, requiredStepRejectp->clone_વૃક્ષ_Pure(ખોટું));
                *outMatchpp = notPMatchp;
            }
            જો (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_વૃક્ષ(), throughoutRejectp);
            જો (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_વૃક્ષ(), rejectBasep);
            જો (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_વૃક્ષ(), requiredStepRejectp);
            Ast_ગાંઠ_પદાવલિ* અચલ resultExprp = નવું AstLogNot{flp, matchp};
            પાછા resultExprp;
        }
        જો (isCover) {
            જો (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_વૃક્ષ(), throughoutRejectp);
            જો (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_વૃક્ષ(), rejectBasep);
            જો (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_વૃક્ષ(), requiredStepRejectp);
            જો (matchCondp) {
                Ast_ગાંઠ_પદાવલિ* અચલ sampledCondp = sampled(matchCondp->clone_વૃક્ષ_Pure(ખોટું));
                પાછા નવું AstLogAnd{flp, terminalActivep, sampledCondp};
            }
            પાછા terminalActivep;
        }
        // Assert/assume: output = !reject
        Ast_ગાંઠ_પદાવલિ* rejectp = nullptr;
        જો (matchCondp && rejectBasep) {
            Ast_ગાંઠ_પદાવલિ* અચલ sampledCondp = sampled(matchCondp->clone_વૃક્ષ_Pure(ખોટું));
            rejectp = નવું AstLogAnd{flp, rejectBasep, નવું AstLogNot{flp, sampledCondp}};
        } નહીંતર જો (rejectBasep) {
            VL_DO_DANGLING(rejectBasep->delete_વૃક્ષ(), rejectBasep);
        }
        જો (outMatchpp) {
            Ast_ગાંઠ_પદાવલિ* matchExprp = terminalActivep->clone_વૃક્ષ_Pure(ખોટું);
            જો (matchCondp) {
                Ast_ગાંઠ_પદાવલિ* અચલ sp = sampled(matchCondp->clone_વૃક્ષ_Pure(ખોટું));
                matchExprp = નવું AstLogAnd{flp, matchExprp, sp};
            }
            *outMatchpp = matchExprp;
        }
        જો (terminalActivep) VL_DO_DANGLING(terminalActivep->delete_વૃક્ષ(), terminalActivep);
        જો (throughoutRejectp) rejectp = orExprs(flp, rejectp, throughoutRejectp);
        જો (requiredStepRejectp) rejectp = orExprs(flp, rejectp, requiredStepRejectp);
        જો (!rejectp) પાછા નવું AstConst{flp, AstConst::BitTrue{}};
        Ast_ગાંઠ_પદાવલિ* અચલ resultExprp = નવું AstLogNot{flp, rejectp};
        પાછા resultExprp;
    }

    explicit SvaNfaLowering(Ast_ગાંઠ_Module* modp)
        : m_modp{modp}
        , m_u32DTypep{modp->findBasicDType(VBasicDTypeKwd::UINT32)} {}
    ~SvaNfaLowering() {
        V3Stats::addStatSum("Assertions, NFA delay ring edge visits", m_statDelayRingEdgeVisits);
    }

    // Lower NFA graph to synthesizable AstAlways blocks and raw result signals.
    // Links are combinational; Edges are registered (NBA).
    SignalSet lower(Ast_ગાંઠ_Cover_Or_Assert* અચલ assertp, SvaGraph& graph,
                    Ast_Sen_વૃક્ષ* અચલ senTreep, Ast_ગાંઠ_પદાવલિ* અચલ matchCondp,
                    Ast_ગાંઠ_પદાવલિ* અચલ disableExprp, અચલ તાર્કિક need_Thread_Fail_ગણતરી,
                    અચલ તાર્કિક need_Throughout_Thread_Fail_ગણતરી) {
        File_લીટી* અચલ flp = assertp->fileline();
        AstCover* અચલ coverp = VN_CAST(assertp, Cover);
        અચલ તાર્કિક isSeqEvent = coverp && coverp->isSeqEvent();
        અચલ VAssertType assertType
            = isSeqEvent ? VAssertType{VAssertType::INTERNAL} : assertp->userType();
        અચલ VAssertDirectiveType directiveType
            = isSeqEvent ? VAssertDirectiveType{VAssertDirectiveType::INTERNAL}
                         : assertp->directive();
        અચલ std::string baseName = m_names.get("");

        // Number vertices with sequential colors for array indexing.
        પૂર્ણાંક N = 0;
        માટે (V3GraphVertex& vtxr : graph.m_graph.vertices()) { vtxr.color(N++); }
        std::vector<SvaStateVertex*> vtx(N, nullptr);
        માટે (V3GraphVertex& vtxr : graph.m_graph.vertices()) {
            vtx[vtxr.color()] = static_cast<SvaStateVertex*>(&vtxr);
        }

        // Allocate per-vertex lowering data (stored via V3GraphVertex::userp()).
        std::vector<SvaVertexData> vertexData(N);
        માટે (પૂર્ણાંક i = 0; i < N; ++i) vtx[i]->userp(&vertexData[i]);

        // Identify registered vertices (targets of clocked edges).
        માટે (પૂર્ણાંક i = 0; i < N; ++i) {
            માટે (અચલ V3GraphEdge& edger : vtx[i]->outEdges()) {
                અચલ SvaTransEdge& tedger = static_cast<અચલ SvaTransEdge&>(edger);
                અચલ પૂર્ણાંક toIdx = tedger.toVtxp()->color();
                જો (tedger.m_consumesCycle && tedger.toVtxp() != graph.m_matchVertexp
                    && !tedger.toVtxp()->m_isRejectSink) {
                    vtx[toIdx]->datap()->needsReg = સાચું;
                }
            }
        }

        // A packed range ring can emit several live endpoints without needing per-slot counts.
        // Widen only a ring whose input can already carry multiplicity.
        std::vector<તાર્કિક> stateMayHaveMultiplicity(N);
        std::vector<તાર્કિક> ringNeedsMultiplicity(N);
        std::vector<પૂર્ણાંક> multiplicityWork;
        અચલ તાર્કિક ગણતરી_Matches = coverp && coverp->isCoverSeq() && !isSeqEvent;
        જો (ગણતરી_Matches) {
            માટે (પૂર્ણાંક i = 0; i < N; ++i) {
                અચલ તાર્કિક multipleInputs = vtx[i]->inEdges().size() > 1;
                અચલ તાર્કિક isRing = vtx[i]->m_delayRingSize;
                ringNeedsMultiplicity[i] = isRing && multipleInputs;
                અચલ તાર્કિક startsMultiplicity = isRing
                                                    ? !vtx[i]->m_isFixedDelayRing
                                                    : multipleInputs && !vtx[i]->datap()->needsReg;
                જો (startsMultiplicity) {
                    stateMayHaveMultiplicity[i] = સાચું;
                    multiplicityWork.ધકેલો_back(i);
                }
            }
            માટે (size_t wi = 0; wi < multiplicityWork.size(); ++wi) {
                માટે (અચલ V3GraphEdge& edger : vtx[multiplicityWork[wi]]->outEdges()) {
                    અચલ SvaTransEdge& tedger = static_cast<અચલ SvaTransEdge&>(edger);
                    અચલ પૂર્ણાંક ti = tedger.toVtxp()->color();
                    અચલ તાર્કિક isRing = vtx[ti]->m_delayRingSize;
                    જો (isRing) ringNeedsMultiplicity[ti] = સાચું;
                    // Match and reject sinks have no outgoing edges, so they stop naturally.
                    અચલ તાર્કિક stopsMultiplicity = !isRing && vtx[ti]->datap()->needsReg;
                    જો (stateMayHaveMultiplicity[ti] || stopsMultiplicity) { ચાલુ; }
                    stateMayHaveMultiplicity[ti] = સાચું;
                    multiplicityWork.ધકેલો_back(ti);
                }
            }
        }

        અચલ તાર્કિક asyncDisable = isAsyncDisable(disableExprp);
        // Registers cleared on a rising disable are also written by their clocked block.
        File_લીટી* અચલ regFlp = asyncDisable ? નવું File_લીટી{flp} : flp;
        જો (asyncDisable) regFlp->warnOff(V3ErrorCode::MULTIDRIVEN, સાચું);

        AstVar* અચલ killVarp
            = નવું AstVar{flp, VVarType::MODULETEMP, baseName + "__kill", m_u32DTypep};
        killVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(killVarp);
        માટે (પૂર્ણાંક i = 0; i < N; ++i) {
            જો (vtx[i]->m_isAndCombiner) {
                અચલ std::string base = baseName + "__a" + std::to_string(i);
                AstVar* અચલ lp = નવું AstVar{regFlp, VVarType::MODULETEMP, base + "_doneL",
                                              m_modp->findBitDType()};
                lp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(lp);
                vtx[i]->datap()->doneLVarp = lp;
                AstVar* અચલ rp = નવું AstVar{regFlp, VVarType::MODULETEMP, base + "_doneR",
                                              m_modp->findBitDType()};
                rp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(rp);
                vtx[i]->datap()->doneRVarp = rp;
                ચાલુ;
            }
            જો (vtx[i]->m_delayRingSize) {
                અચલ std::string base = baseName + "__d" + std::to_string(i);
                અચલ તાર્કિક storesCounts = ગણતરી_Matches && ringNeedsMultiplicity[i];
                Ast_ગાંઠ_DType* ringDTypep = nullptr;
                જો (storesCounts) {
                    // int unsigned ring [size];
                    ringDTypep = નવું AstUnpackArrayDType{
                        flp, m_u32DTypep,
                        નવું AstRange{flp, static_cast<પૂર્ણાંક>(vtx[i]->m_delayRingSize - 1), 0}};
                    v3Global.rootp()->typeTablep()->addTypesp(ringDTypep);
                } નહીંતર {
                    // bit [size-1:0] ring;
                    ringDTypep = m_modp->findBitDType(vtx[i]->m_delayRingSize,
                                                      vtx[i]->m_delayRingSize, VSigning::UNSIGNED);
                }
                AstVar* અચલ ringp
                    = નવું AstVar{flp, VVarType::MODULETEMP, base + "_ring", ringDTypep};
                ringp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(ringp);
                vtx[i]->datap()->delayRingVarp = ringp;
                vtx[i]->datap()->delayRingCounts = storesCounts;
                AstVar* અચલ headp
                    = નવું AstVar{flp, VVarType::MODULETEMP, base + "_head",
                                 storesCounts ? m_u32DTypep : m_modp->findBitDType()};
                headp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(headp);
                vtx[i]->datap()->delayRingHeadVarp = headp;
                // int unsigned idx;
                AstVar* અચલ idxp
                    = નવું AstVar{regFlp, VVarType::MODULETEMP, base + "_idx", m_u32DTypep};
                idxp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(idxp);
                vtx[i]->datap()->delayRingIdxVarp = idxp;
                // int unsigned live_count;
                AstVar* અચલ live_ગણતરી_Varp
                    = નવું AstVar{regFlp, VVarType::MODULETEMP, base + "_liveCount", m_u32DTypep};
                live_ગણતરી_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(live_ગણતરી_Varp);
                vtx[i]->datap()->delay_Ring_Live_ગણતરી_Varp = live_ગણતરી_Varp;
                AstVar* અચલ wrappedp = નવું AstVar{regFlp, VVarType::MODULETEMP,
                                                    base + "_wrapped", m_modp->findBitDType()};
                wrappedp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(wrappedp);
                vtx[i]->datap()->delayRingWrappedVarp = wrappedp;
                ચાલુ;
            }
            જો (!vtx[i]->datap()->needsReg) ચાલુ;
            જો (vtx[i] == graph.m_startVertexp || vtx[i]->m_isMatch) ચાલુ;
            અચલ std::string varName = baseName + "__s" + std::to_string(i);
            AstVar* અચલ varp
                = નવું AstVar{regFlp, VVarType::MODULETEMP, varName, m_modp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(varp);
            vtx[i]->datap()->stateVarp = varp;
        }

        // Build lowering context for phase sub-functions.
        LowerCtx c{flp,        senTreep,      disableExprp, matchCondp,
                   assertType, directiveType, killVarp,     graph};

        // Phase 1: Resolve combinational Links via fixed-point propagation.
        Ast_ગાંઠ_પદાવલિ* અચલ triggerExprp
            = isSeqEvent ? નવું AstConst{flp, AstConst::BitTrue{}}
                         : assertOnCond(flp, assertp->userType(), assertp->directive());
        resolveLinks(c, triggerExprp, ગણતરી_Matches);
        VL_DO_DANGLING(triggerExprp->delete_વૃક્ષ(), triggerExprp);

        // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
        emitStateRegisterNba(c);
        emitDelayRingUpdates(c, ગણતરી_Matches);
        emitAndCombinerDoneLatchNba(c);
        emitKillAckNba(c);

        // Phase 3/3a/3b: Compute terminal match/reject signals (cleans up stateSig).
        અચલ SignalSet sigs
            = computeSignals(c, need_Thread_Fail_ગણતરી, need_Throughout_Thread_Fail_ગણતરી, ગણતરી_Matches);

        // Strong s_always[m:n] end-of-simulation liveness: if any in-window state
        // is still set at $finish, the universal-quantifier window never completed
        // (IEEE 1800-2023 16.12.11 strong semantics). Fire the assertion failure
        // from a final block; V3Assert turns the DT_ERROR display into the standard
        // "Assertion failed in %m" message.
        // pending = |{strong state registers, strong delay live counts != 0};
        Ast_ગાંઠ_પદાવલિ* pendingp = nullptr;
        માટે (પૂર્ણાંક i = 0; i < N; ++i) {
            જો (!vtx[i]->m_strongPending) ચાલુ;
            Ast_ગાંઠ_પદાવલિ* pendingExprp = nullptr;
            જો (vtx[i]->datap()->stateVarp) {
                pendingExprp = નવું AstVarRef{flp, vtx[i]->datap()->stateVarp, VAccess::READ};
            } નહીંતર જો (vtx[i]->m_strongAlwaysRing) {
                pendingExprp
                    = delayRingHasLiveThreadsp(flp, vtx[i]->datap()->delay_Ring_Live_ગણતરી_Varp);
            } નહીંતર {
                ચાલુ;
            }
            જો (!pendingp) {
                pendingp = pendingExprp;
            } નહીંતર {
                pendingp = નવું AstLogOr{flp, pendingp, pendingExprp};
            }
        }
        જો (pendingp) {
            AstCExpr* અચલ assertOnp
                = નવું AstCExpr{flp, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()", 1};
            Ast_ગાંઠ_પદાવલિ* અચલ condp = નવું AstLogAnd{flp, assertOnp, pendingp};
            AstDisplay* અચલ dispp
                = નવું AstDisplay{flp, VDisplayType::DT_ERROR, "", nullptr, nullptr};
            dispp->fmtp()->timeunit(m_modp->timeunit());
            Ast_ગાંઠ_Stmt* અચલ firep = dispp;
            જો (v3Global.opt.stopFail()) firep->addNext(નવું AstStop{flp, ખોટું});
            m_modp->addStmtsp(નવું AstFinal{flp, નવું AstIf{flp, condp, firep}});
        }

        // Clear userp on every vertex before vertexData is destroyed.
        માટે (પૂર્ણાંક i = 0; i < N; ++i) vtx[i]->userp(nullptr);
        પાછા sigs;
    }
};

}  // namespace

//######################################################################
// Top-level visitor

વર્ગ AssertNfaVisitor final : જાહેર VNVisitor {
    // STATE
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module being processed
    AstClocking* m_defaultClockingp = nullptr;  // Default clocking
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff
    SvaNfaLowering* m_loweringp = nullptr;  // NFA-to-hardware lowering engine
    Ast_Sen_વૃક્ષ* m_sampled_કિંમત_Clockp = nullptr;  // Inherited clock during scoped attachment
    V3UniqueNames m_propVarNames{"__Vpropvar"};  // Property-local variable names
    V3UniqueNames m_propTempNames{"__VnfaSampled"};  // Hoisted $sampled(propp) temps
    V3UniqueNames m_fail_ગણતરી_Names{"__VnfaRemainingFailCount"};  // Fail replay counter names
    std::set<અચલ AstProperty*> m_inliningProps;  // Recursion guard for inlineNamedProperty

    template <typename T_ગાંઠ>
    શૂન્ય visit_Sampled_કિંમત(T_ગાંઠ* અચલ nodep) {
        જો (m_sampled_કિંમત_Clockp && !nodep->sentreep()) {
            nodep->sentreep(m_sampled_કિંમત_Clockp->clone_વૃક્ષ(સાચું));
        }
        iterateChildren(nodep);
    }

    // Wire match vertex and mid-window sources for a successful NFA build.
    સ્થિર શૂન્ય wireMatchAndMidSources(SvaGraph& graph, અચલ BuildResult& result, File_લીટી* flp) {
        graph.createMatchVertex();
        // Skip the main term Link when midSources already cover every
        // end-of-match (cover_sequence path); otherwise the per-mid extraction
        // double-counts via the merge vertex.
        જો (!result.termIsMidMerge) { graph.addLink(result.termVertexp, graph.m_matchVertexp); }
        માટે (SvaStateVertex* srcVtxp : result.midSources) {
            Ast_ગાંઠ_પદાવલિ* condp = nullptr;
            માટે (Ast_ગાંઠ_પદાવલિ* અચલ tc : srcVtxp->m_throughoutConds) {
                Ast_ગાંઠ_પદાવલિ* અચલ tcClone = tc->clone_વૃક્ષ_Pure(ખોટું);
                condp = condp ? નવું AstLogAnd{flp, condp, tcClone} : tcClone;
            }
            graph.addLink(srcVtxp, graph.m_matchVertexp, condp);
            srcVtxp->m_isUnbounded = સાચું;
        }
    }

    સ્થિર Ast_ગાંઠ_પદાવલિ* getSequenceBodyExprp(અચલ AstSequence* seqp) {
        Ast_ગાંઠ* bodyp = seqp->stmtsp();
        જ્યારે (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        પાછા VN_CAST(bodyp, ગાંઠ_પદાવલિ);
    }

    સ્થિર AstPropSpec* getPropertySpecp(અચલ AstProperty* propp) {
        Ast_ગાંઠ* stmtp = propp->stmtsp();
        // V3LinkParse emits InitialStaticStmt for property-local variable
        // initialisers; the InitialAutomaticStmt variant only appears for
        // task/function-scope automatic lifetime, not properties.
        જ્યારે (stmtp
               && (VN_IS(stmtp, Var) || VN_IS(stmtp, InitialStaticStmt)
                   || VN_IS(stmtp, InitialAutomaticStmt))) {  // LCOV_EXCL_LINE
            stmtp = stmtp->nextp();
        }
        પાછા VN_CAST(stmtp, PropSpec);
    }

    શૂન્ય inlineNamedProperty(AstPropSpec* outerSpecp, AstFuncRef* funcrefp,
                             અચલ AstProperty* propyp) {
        // Recursion guard: IEEE 1800-2023 16.12.1 forbids recursive properties.
        // V3Width emits "Recursive property call" for direct recursion before this
        // pass runs; this catches any nested-inlining cycle that slips past.
        જો (!m_inliningProps.emplace(propyp).second) {
            funcrefp->v3error("Illegal recursive property reference");  // LCOV_EXCL_LINE
            પાછા;  // LCOV_EXCL_LINE
        }
        સંરચના Guard final {
            std::set<અચલ AstProperty*>& setr;
            અચલ AstProperty* keyp;
            ~Guard() { setr.erase(keyp); }
        } guard{m_inliningProps, propyp};
        AstPropSpec* propSpecp = getPropertySpecp(propyp);
        UASSERT_OBJ(propSpecp, funcrefp, "Property has no body PropSpec");
        propSpecp = propSpecp->clone_વૃક્ષ(ખોટું);

        અચલ V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propyp->stmtsp());
        std::unordered_map<અચલ AstVar*, Ast_ગાંઠ_પદાવલિ*> portMap;
        માટે (અચલ auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables to module-level temps (IEEE 16.10).
        std::unordered_map<અચલ AstVar*, AstVar*> localVarMap;
        માટે (Ast_ગાંઠ* stmtp = propyp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (AstVar* અચલ varp = VN_CAST(stmtp, Var)) {
                જો (!varp->isIO()) {
                    અચલ string newName = m_propVarNames.get(varp);
                    AstVar* અચલ newVarp = નવું AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        propSpecp->foreach([&](AstVarRef* refp) {
            અચલ auto portIt = portMap.find(refp->varp());
            જો (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_વૃક્ષ(ખોટું));
                VL_DO_DANGLING(ધકેલો_Deletep(refp), refp);
                પાછા;
            }
            અચલ auto localIt = localVarMap.find(refp->varp());
            જો (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        માટે (અચલ auto& tconnect : tconnects) {
            ધકેલો_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propSpecp, funcrefp);

        // Merge disable iff (IEEE 1800-2023 16.12.1)
        જો (outerSpecp->disablep() && propSpecp->disablep()) {
            outerSpecp->v3error("disable iff expression before property call "
                                "and in its body is not legal");
            ધકેલો_Deletep(propSpecp->disablep()->unlinkFrBack());
        }
        જો (outerSpecp->disablep()) {
            propSpecp->disablep(outerSpecp->disablep()->unlinkFrBack());
        }

        જો (outerSpecp->sensesp() && propSpecp->sensesp()) {
            outerSpecp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Clock event before property call and in its body");
            ધકેલો_Deletep(propSpecp->sensesp()->unlinkFrBack());
        }
        જો (outerSpecp->sensesp()) {
            AstSenItem* અચલ sensesp = outerSpecp->sensesp();
            sensesp->unlinkFrBack();
            propSpecp->sensesp(sensesp);
        }

        outerSpecp->replaceWith(propSpecp);
        VL_DO_DANGLING(ધકેલો_Deletep(outerSpecp), outerSpecp);
    }

    શૂન્ય inlineSequenceRef(AstFuncRef* funcrefp, AstSequence* seqp) {
        Ast_ગાંઠ_પદાવલિ* અચલ bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        Ast_ગાંઠ_પદાવલિ* અચલ clonedp = bodyExprp->clone_વૃક્ષ(ખોટું);

        અચલ V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<અચલ AstVar*, Ast_ગાંઠ_પદાવલિ*> portMap;
        માટે (અચલ auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            અચલ auto it = portMap.find(refp->varp());
            જો (it != portMap.end()) {
                refp->replaceWith(it->second->clone_વૃક્ષ(ખોટું));
                VL_DO_DANGLING(ધકેલો_Deletep(refp), refp);
            }
        });
        માટે (અચલ auto& tconnect : tconnects) {
            ધકેલો_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ધકેલો_Deletep(funcrefp), funcrefp);
        // Clear referenced flag so V3AssertPre cleanup does not emit
        // spurious UNSUPPORTED for sequences that were already inlined here.
        seqp->isReferenced(ખોટું);
    }

    // Must run before hasMultiCycleExpr() so NFA sees sequence bodies.
    શૂન્ય inlineAllSequenceRefs(Ast_ગાંઠ* rootp) {
        તાર્કિક changed = સાચું;
        જ્યારે (changed) {
            changed = ખોટું;
            rootp->foreach([&](AstFuncRef* funcrefp) {
                જો (changed) પાછા;
                જો (AstSequence* અચલ seqp = VN_CAST(funcrefp->taskp(), Sequence)) {
                    inlineSequenceRef(funcrefp, seqp);
                    changed = સાચું;
                }
            });
        }
    }

    સ્થિર તાર્કિક has_Multi_Cycle_પદાવલિ(અચલ Ast_ગાંઠ* nodep) {
        પાછા nodep->exists([](અચલ Ast_ગાંઠ* np) {
            જો (અચલ auto* અચલ ep = VN_CAST(np, ગાંઠ_પદાવલિ)) પાછા ep->isMultiCycleSva();
            પાછા ખોટું;
        });
    }

    સ્થિર VPropStrength effectiveAssertPropStrength(અચલ AstPropSpec* અચલ propSpecp) {
        જો (propSpecp->propStrength() != VPropStrength::DEFAULT) પાછા propSpecp->propStrength();
        પાછા propSpecp->fileline()->language() <= V3LangCode::L1800_2005 ? VPropStrength::STRONG
                                                                           : VPropStrength::WEAK;
    }

    // Bare `assert property (p until q)` with boolean operands stays on
    // V3AssertPre's AstLoop lowering, which preserves per-attempt action-block
    // firings that this NFA's single-bit aggregated state cannot. Strong bare
    // forms are also lowered there. NFA still owns sequence operands and any
    // embedding inside a multi-cycle context (implication consequent, or/and
    // operands, etc.).
    સ્થિર તાર્કિક is_Bare_ટોચ_Level_Until(Ast_ગાંઠ* propp) {
        Ast_ગાંઠ* p = propp;
        જો (AstPropSpec* અચલ specp = VN_CAST(p, PropSpec)) p = specp->propp();
        જ્યારે (AstLogNot* અચલ notp = VN_CAST(p, LogNot)) p = notp->lhsp();
        AstUntil* અચલ untilp = VN_CAST(p, Until);
        જો (!untilp) પાછા ખોટું;
        અચલ auto hasSeq = [](અચલ Ast_ગાંઠ_પદાવલિ* ep) {
            પાછા ep->exists([](અચલ Ast_ગાંઠ_પદાવલિ* np) { પાછા np->isMultiCycleSva(); });
        };
        જો (hasSeq(untilp->lhsp()) || hasSeq(untilp->rhsp())) પાછા ખોટું;
        પાછા સાચું;
    }

    સંરચના PropertyParts final {
        Ast_ગાંઠ_પદાવલિ* triggerExprp = nullptr;
        Ast_ગાંઠ_પદાવલિ* seqExprp = nullptr;
        તાર્કિક isOverlapped = સાચું;
        તાર્કિક hasImplication = ખોટું;
        તાર્કિક isFollowedBy = ખોટું;  // True for #-# / #=# (non-vacuous-fail on antecedent miss)
    };

    સ્થિર PropertyParts decomposeProperty(Ast_ગાંઠ* propp) {
        PropertyParts parts;
        જો (AstPropSpec* અચલ specp = VN_CAST(propp, PropSpec)) { propp = specp->propp(); }
        જો (AstImplication* અચલ implp = VN_CAST(propp, Implication)) {
            parts.hasImplication = સાચું;
            parts.isOverlapped = implp->isOverlapped();
            parts.isFollowedBy = implp->isFollowedBy();
            parts.triggerExprp = implp->lhsp();
            parts.seqExprp = implp->rhsp();
        } નહીંતર જો (Ast_ગાંઠ_પદાવલિ* અચલ exprp = VN_CAST(propp, ગાંઠ_પદાવલિ)) {
            parts.triggerExprp = nullptr;
            parts.seqExprp = exprp;
        }
        પાછા parts;
    }

    સ્થિર તાર્કિક canSplitImplicationPassActions(અચલ PropertyParts& parts) {
        UASSERT(parts.hasImplication,
                "Implication pass action split requested without implication");
        UASSERT(parts.triggerExprp, "Implication pass action split requested without trigger");
        // Direct vacuous/nonvacuous classification uses the antecedent value in the current
        // assertion attempt. Leave delayed antecedents on the existing NFA pass path.
        પાછા !has_Multi_Cycle_પદાવલિ(parts.triggerExprp);
    }

    સ્થિર શૂન્ય splitImplicationPassActions(AstAssert* assertp, અચલ PropertyParts& parts,
                                            Ast_ગાંઠ_પદાવલિ* nonvacuousMatchp) {
        File_લીટી* અચલ flp = assertp->fileline();

        Ast_ગાંઠ* અચલ passsp = assertp->passsp()->unlinkFrBackWithNext();
        Ast_ગાંઠ* splitsp = nullptr;

        જો (!parts.isFollowedBy) {
            Ast_ગાંઠ_પદાવલિ* અચલ vacuousp
                = નવું AstLogNot{flp, sampled(parts.triggerExprp->clone_વૃક્ષ_Pure(ખોટું))};
            Ast_ગાંઠ* અચલ vacuousBodyp = passsp->clone_વૃક્ષ(ખોટું);
            splitsp = newPassOnIf(flp, vacuousp, vacuousBodyp, assertp->userType(),
                                  assertp->directive(), /*vacuous=*/સાચું);
        }

        AstIf* અચલ nonvacuousp
            = newPassOnIf(flp, nonvacuousMatchp, passsp, assertp->userType(), assertp->directive(),
                          /*vacuous=*/ખોટું);
        splitsp = splitsp ? Ast_ગાંઠ::addNext<Ast_ગાંઠ, Ast_ગાંઠ>(splitsp, nonvacuousp)
                          : static_cast<Ast_ગાંઠ*>(nonvacuousp);
        assertp->addPasssp(splitsp);
    }

    // On a PropSpec-wrapped assertion whose NFA build failed with a semantic
    // error (errorEmitted), replace the body with a BitFalse const so later
    // passes see a well-formed AST. Returns true if replaced.
    શૂન્ય replaceBodyOnBuildError(File_લીટી* flp, AstPropSpec* propSpecp, તાર્કિક errorEmitted) {
        જો (!errorEmitted) પાછા;
        Ast_ગાંઠ* અચલ innerPropp = propSpecp->propp();
        innerPropp->replaceWith(નવું AstConst{flp, AstConst::BitFalse{}});
        VL_DO_DANGLING(ધકેલો_Deletep(innerPropp), innerPropp);
    }

    // Hoist a leading clocking event (IEEE 1800-2023 16.7):
    તાર્કિક hoistClockedSeq(AstPropSpec* specp) {
        જ્યારે (AstSClocked* અચલ clockedp = VN_CAST(specp->propp(), SClocked)) {
            જો (specp->sensesp()) {
                clockedp->v3warn(E_UNSUPPORTED, "Unsupported: multiclocked sequence or property");
                replaceBodyOnBuildError(specp->fileline(), specp, સાચું);
                પાછા સાચું;
            }
            માટે (અચલ AstSenItem* sp = clockedp->sensesp(); sp;
                 sp = VN_CAST(sp->nextp(), SenItem)) {
                જો (!sp->edgeType().anEdge()) {
                    clockedp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: non-edge clocking event on a sequence; "
                                     "use an edge such as @(posedge clk)");
                    replaceBodyOnBuildError(specp->fileline(), specp, સાચું);
                    પાછા સાચું;
                }
            }
            specp->sensesp(clockedp->sensesp()->unlinkFrBackWithNext());
            Ast_ગાંઠ_પદાવલિ* અચલ bodyp = clockedp->exprp()->unlinkFrBack();
            clockedp->replaceWith(bodyp);
            VL_DO_DANGLING(ધકેલો_Deletep(clockedp), clockedp);
        }
        // A clocking event anywhere else in the sequence is not supported.
        અચલ AstSClocked* nestedp = nullptr;
        specp->propp()->foreach([&](અચલ AstSClocked* p) {
            જો (!nestedp) nestedp = p;
        });
        જો (nestedp) {
            nestedp->v3warn(E_UNSUPPORTED,
                            "Unsupported: clocking event inside sequence expression");
            replaceBodyOnBuildError(specp->fileline(), specp, સાચું);
            પાછા સાચું;
        }
        પાછા ખોટું;
    }

    // Build the NFA graph for a property body, handling both the antecedent
    // |-> consequent and simple sequence cases. Returns the consequent/body
    // BuildResult (invalid on parse/build failure).
    BuildResult buildAssertionGraph(SvaNfaBuilder& builder, SvaGraph& graph, Ast_ગાંઠ_પદાવલિ* seqBodyp,
                                    અચલ PropertyParts& parts, File_લીટી* flp) {
        જો (!parts.hasImplication) પાછા builder.build(seqBodyp);

        graph.m_startVertexp = graph.createStateVertex();
        પાછા builder.buildImplicationEdges(parts.triggerExprp, seqBodyp, graph.m_startVertexp,
                                             parts.isOverlapped, parts.isFollowedBy,
                                             parts.triggerExprp, flp);
    }

    // Install pass-action gating and replay simultaneous per-thread failures.
    શૂન્ય attachActionHandlers(AstAssert* અચલ assertp, Ast_ગાંઠ_પદાવલિ* matchExprp,
                              Ast_Sen_વૃક્ષ* અચલ threadFailReplaySenTreep,
                              Ast_ગાંઠ_પદાવલિ* અચલ threadFailCountp) {
        // Gate pass handler on match to prevent vacuous-pass firings.
        જો (matchExprp) {
            // needMatch implies passsp() was non-null when evaluated above;
            // lowering does not mutate the assert's pass-action between the
            // two reads, so passsp() is still non-null here.
            Ast_ગાંઠ* passsp = assertp->passsp();
            UASSERT_OBJ(passsp, assertp, "needMatch set but passsp is null");
            passsp->unlinkFrBackWithNext();
            File_લીટી* અચલ flp = assertp->fileline();
            AstIf* અચલ ifp = નવું AstIf{flp, matchExprp, passsp, nullptr};
            assertp->addPasssp(ifp);
            // Fail-handler prefix for overlapping instances (IEEE 16.12):
            // fires when reject=1 && match=1 in the same cycle.
            જો (assertp->failsp()) assertp->prependFailsp(ifp->clone_વૃક્ષ(ખોટું));
        }

        જો (threadFailCountp) {
            UASSERT_OBJ(threadFailReplaySenTreep, assertp,
                        "Thread fail count missing sensitivity tree");
            Ast_ગાંઠ* અચલ failsp = assertp->failsp();
            File_લીટી* અચલ flp = assertp->fileline();
            // IEEE 1800-2023 16.12 requires one action-block evaluation per failed
            // thread. AstAssert handles the first, so replay the rest here.
            AstVar* અચલ remaining_Fail_ગણતરી_Varp
                = નવું AstVar{flp, VVarType::MODULETEMP, m_fail_ગણતરી_Names.get(""),
                             m_modp->findBasicDType(VBasicDTypeKwd::UINT32)};
            remaining_Fail_ગણતરી_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remaining_Fail_ગણતરી_Varp);
            Ast_ગાંઠ* અચલ replayStmtsp = નવું AstAssign{
                flp, નવું AstVarRef{flp, remaining_Fail_ગણતરી_Varp, VAccess::WRITE}, threadFailCountp};
            AstLoop* અચલ replayLoopp = નવું AstLoop{flp};
            replayLoopp->addStmtsp(નવું AstLoopTest{
                flp, replayLoopp,
                નવું AstGt{flp, નવું AstVarRef{flp, remaining_Fail_ગણતરી_Varp, VAccess::READ},
                          newTypedConstp(flp, remaining_Fail_ગણતરી_Varp->dtypep(), 1)}});
            replayLoopp->addStmtsp(newIfAssertFailOn(failsp->clone_વૃક્ષ(સાચું), assertp->directive(),
                                                     assertp->userType()));
            AstSub* અચલ decrementedFailCountp
                = નવું AstSub{flp, નવું AstVarRef{flp, remaining_Fail_ગણતરી_Varp, VAccess::READ},
                             newTypedConstp(flp, remaining_Fail_ગણતરી_Varp->dtypep(), 1)};
            replayLoopp->addStmtsp(
                નવું AstAssign{flp, નવું AstVarRef{flp, remaining_Fail_ગણતરી_Varp, VAccess::WRITE},
                              decrementedFailCountp});
            replayStmtsp->addNext(replayLoopp);
            m_modp->addStmtsp(
                નવું AstAlways{flp, VAlwaysKwd::ALWAYS, threadFailReplaySenTreep, replayStmtsp});
        }
    }

    // Replace one VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). No-op if refp is not in matchMap.
    શૂન્ય substituteMatchItemRef(AstVarRef* refp, unsigned K,
                                અચલ std::unordered_map<અચલ AstVar*, Ast_ગાંઠ_પદાવલિ*>& matchMap) {
        અચલ auto it = matchMap.find(refp->varp());
        જો (it == matchMap.end()) પાછા;
        Ast_ગાંઠ_પદાવલિ* newp = it->second->clone_વૃક્ષ_Pure(ખોટું);
        જો (K > 0) {
            AstConst* અચલ ticksp = નવું AstConst{refp->fileline(), AstConst::Widthed_કિંમત{}, 32,
                                                  static_cast<uint32_t>(K)};
            AstPast* અચલ pastp = નવું AstPast{refp->fileline(), newp, ticksp, nullptr,
                                               /* propertyTiming */ સાચું};
            pastp->dtypeFrom(newp);
            newp = pastp;
        }
        refp->replaceWith(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(refp), refp);
        પાછા;
    }

    // Recursively walk a consequent. Returns cycle length consumed and
    // substitutes each VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). Reports E_UNSUPPORTED on non-constant
    // delays or composite sequence operators and returns -1 on failure.
    પૂર્ણાંક
    walk_Substitute_Match_વસ્તુઓ(Ast_ગાંઠ_પદાવલિ* nodep, unsigned K,
                             અચલ std::unordered_map<અચલ AstVar*, Ast_ગાંઠ_પદાવલિ*>& match_વસ્તુઓ) {
        જો (AstSExpr* અચલ sexprp = VN_CAST(nodep, SExpr)) {
            // IEEE 1800-2023 16.9.2: cycle_delay's lhsp is a constant_expression
            // and the delay form in a sequence is always `##N`, folded by
            // V3Const + V3Param before V3AssertNfa. Range form `##[m:n]` is the
            // only user-visible reject here.
            AstDelay* અચલ delayp = VN_AS(sexprp->delayp(), Delay);
            UASSERT_OBJ(delayp->isCycleDelay() && VN_IS(delayp->lhsp(), Const), sexprp,
                        "SVA cycle delay must have a constant lhsp");
            જો (delayp->isRangeDelay()) {
                sexprp->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                              "non-constant cycle delay in consequent"
                                              " (IEEE 1800-2023 16.10)");
                પાછા -1;
            }
            અચલ unsigned delayCycles = VN_AS(delayp->lhsp(), Const)->toUInt();
            પૂર્ણાંક preLen = 0;
            જો (Ast_ગાંઠ_પદાવલિ* અચલ prep = sexprp->preExprp()) {
                preLen = walk_Substitute_Match_વસ્તુઓ(prep, K, match_વસ્તુઓ);
                જો (preLen < 0) પાછા -1;
            }
            અચલ પૂર્ણાંક bodyLen
                = walk_Substitute_Match_વસ્તુઓ(sexprp->exprp(), K + preLen + delayCycles, match_વસ્તુઓ);
            જો (bodyLen < 0) પાછા -1;
            પાછા preLen + delayCycles + bodyLen;
        }
        જો (nodep->isMultiCycleSva()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                         "composite sequence operator in consequent"
                                         " (IEEE 1800-2023 16.10)");
            પાછા -1;
        }
        std::vector<AstVarRef*> refs;
        nodep->foreach([&refs](AstVarRef* p) { refs.ધકેલો_back(p); });
        માટે (AstVarRef* અચલ refp : refs) substituteMatchItemRef(refp, K, match_વસ્તુઓ);
        પાછા 0;
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
    તાર્કિક liftMatchItemSubstitutions(PropertyParts& parts, Ast_ગાંઠ_પદાવલિ* seqBodyp) {
        જો (!parts.hasImplication) પાછા ખોટું;
        Ast_પદાવલિ_Stmt* અચલ પદાવલિ_Stmtp = VN_CAST(parts.triggerExprp, પદાવલિ_Stmt);
        જો (!પદાવલિ_Stmtp) પાછા ખોટું;
        // IEEE 1800-2023 16.10 BNF requires `(expr, match_item {, match_item})`
        // with at least one match item; V3LinkParse only emits ExprStmt for
        // this form and only emits AstAssign with VarRef LHS for each item.
        std::unordered_map<અચલ AstVar*, Ast_ગાંઠ_પદાવલિ*> match_વસ્તુઓ;
        માટે (Ast_ગાંઠ* stmtp = પદાવલિ_Stmtp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstAssign* અચલ assignp = VN_AS(stmtp, Assign);
            AstVarRef* અચલ lhsRefp = VN_AS(assignp->lhsp(), VarRef);
            match_વસ્તુઓ[lhsRefp->varp()] = assignp->rhsp();
        }
        અચલ unsigned startK = parts.isOverlapped ? 0 : 1;
        અચલ પૂર્ણાંક length = walk_Substitute_Match_વસ્તુઓ(seqBodyp, startK, match_વસ્તુઓ);
        // Match-item substitution / strip mutates ancestor purity. Release
        // builds don't auto-clear caches on edits, so refresh here.
        VIsCached::clear_Cache_વૃક્ષ();
        જો (length < 0) પાછા સાચું;
        Ast_ગાંઠ_પદાવલિ* અચલ antBoolp = પદાવલિ_Stmtp->resultp()->unlinkFrBack();
        પદાવલિ_Stmtp->replaceWith(antBoolp);
        VL_DO_DANGLING(ધકેલો_Deletep(પદાવલિ_Stmtp), પદાવલિ_Stmtp);
        parts.triggerExprp = antBoolp;
        પાછા ખોટું;
    }

    // Outcome counts for a property if/case are wrong in an outcome-multiplying
    // context. Returns that context, or nullptr when the shape is supported.
    સ્થિર અચલ અક્ષર* unsupportedPropertyControl(અચલ Ast_ગાંઠ_Cover_Or_Assert* assertp,
                                                  અચલ Ast_ગાંઠ_પદાવલિ* seqBodyp, તાર્કિક negated) {
        જો (!hasPropertyControlConjunction(seqBodyp)) પાછા nullptr;
        જો (negated) પાછા "negation";
        જો (VN_IS(assertp, Cover)) પાછા "cover";
        જો (VN_AS(assertp, Assert)->passsp()) પાછા "a pass action";
        પાછા nullptr;
    }

    // Inline property/sequence refs and reject unsupported shapes.
    // Returns the PropSpec to lower, or nullptr when fully handled here.
    AstPropSpec* prepareAssertionProp(Ast_ગાંઠ_Cover_Or_Assert* assertp) {
        જો (AstPropSpec* અચલ specp = VN_CAST(assertp->propp(), PropSpec)) {
            જો (AstFuncRef* અચલ funcrefp = VN_CAST(specp->propp(), FuncRef)) {
                જો (અચલ AstProperty* અચલ propyp = VN_CAST(funcrefp->taskp(), Property)) {
                    inlineNamedProperty(specp, funcrefp, propyp);
                }
            }
        }

        inlineAllSequenceRefs(assertp->propp());

        જો (AstPropSpec* અચલ specp = VN_CAST(assertp->propp(), PropSpec)) {
            જો (hoistClockedSeq(specp)) પાછા nullptr;
        }

        AstPropSpec* અચલ propp = VN_AS(assertp->propp(), PropSpec);
        જો (!VN_IS(assertp, Cover)
            && effectiveAssertPropStrength(propp) == VPropStrength::STRONG) {
            propp->v3warn(E_UNSUPPORTED,
                          "Unsupported: strong property in " + assertp->verilogKwd() + ".");
            replaceBodyOnBuildError(assertp->fileline(), propp, /*errorEmitted=*/સાચું);
            પાછા nullptr;
        }

        જો (!has_Multi_Cycle_પદાવલિ(propp)) પાછા nullptr;
        // A nested property instance keeps its body behind the call; lowering would drop it.
        જો (propp->exists([](અચલ AstFuncRef* refp) { પાછા VN_IS(refp->taskp(), Property); })) {
            assertp->v3warn(E_UNSUPPORTED,
                            "Unsupported: property instance inside a multi-cycle property "
                            "expression");
            VL_DO_DANGLING(ધકેલો_Deletep(assertp->unlinkFrBack()), assertp);
            પાછા nullptr;
        }
        જો (is_Bare_ટોચ_Level_Until(propp)) પાછા nullptr;
        પાછા propp;
    }

    શૂન્ય processAssertion(Ast_ગાંઠ_Cover_Or_Assert* assertp) {
        જો (assertp->immediate()) પાછા;

        AstPropSpec* અચલ propp = prepareAssertionProp(assertp);
        જો (!propp) પાછા;

        PropertyParts parts = decomposeProperty(propp);
        UASSERT_OBJ(parts.seqExprp, propp, "Property body must be an expression");

        // Unwrap `not` (IEEE 1800-2023 16.12.1); odd count -> negated semantics.
        Ast_ગાંઠ_પદાવલિ* seqBodyp = parts.seqExprp;
        તાર્કિક negated = ખોટું;
        જ્યારે (AstLogNot* અચલ notp = VN_CAST(seqBodyp, LogNot)) {
            negated = !negated;
            seqBodyp = notp->lhsp();
        }

        અચલ અક્ષર* અચલ propertyControlp
            = unsupportedPropertyControl(assertp, seqBodyp, negated);

        // Substitute property-local match-item refs in consequent with
        // $past(rhs, K) before NFA build (IEEE 1800-2023 16.10).
        જો (liftMatchItemSubstitutions(parts, seqBodyp)) {
            AstPropSpec* અચલ psp = VN_CAST(assertp->propp(), PropSpec);
            UASSERT_OBJ(psp, assertp, "Concurrent assertion must have PropSpec");
            replaceBodyOnBuildError(assertp->fileline(), psp, /*errorEmitted=*/સાચું);
            પાછા;
        }

        AstCover* અચલ coverp = VN_CAST(assertp, Cover);
        અચલ તાર્કિક isCoverSeq = coverp && coverp->isCoverSeq();
        // A sequence event control is not an assertion directive; no default
        // disable iff, no assertion control
        અચલ તાર્કિક isSeqEvent = coverp && coverp->isSeqEvent();
        અચલ તાર્કિક ગણતરી_Matches = isCoverSeq && !isSeqEvent;
        // Inherit module defaults (IEEE 14.12, 16.15) when assertion has none.
        જો (!propp->sensesp() && m_defaultClockingp) {
            propp->sensesp(m_defaultClockingp->sentreep()->sensesp()->clone_વૃક્ષ(સાચું));
        }
        જો (!propp->disablep() && m_defaultDisablep && !isSeqEvent) {
            propp->disablep(m_defaultDisablep->condp()->clone_વૃક્ષ_Pure(સાચું));
        }
        જો (!propp->sensesp()) પાછા;
        Ast_Sen_વૃક્ષ* senTreep
            = નવું Ast_Sen_વૃક્ષ{propp->fileline(), propp->sensesp()->clone_વૃક્ષ(સાચું)};
        Ast_ગાંઠ_પદાવલિ* અચલ disableExprp = propp->disablep();

        // NFA lowering clones repeated operands and may hoist them into an
        // always_comb block. Resolve implicit sampled-value clocks first, while
        // the enclosing assertion clock is still available.
        {
            VL_RESTORER(m_sampled_કિંમત_Clockp);
            m_sampled_કિંમત_Clockp = senTreep;
            iterate(propp->propp());
        }

        File_લીટી* અચલ flp = assertp->fileline();

        SvaGraph graph;
        SvaNfaBuilder builder{graph,      m_modp,     m_propTempNames,
                              isCoverSeq, isSeqEvent, coverp != nullptr};

        અચલ BuildResult result = buildAssertionGraph(builder, graph, seqBodyp, parts, flp);
        જો (result.valid()) wireMatchAndMidSources(graph, result, flp);
        જો (!result.valid()) {
            // Fall through to V3AssertPre for unsupported constructs; only
            // replace the body on real semantic errors. Any hoisted temps
            // from this attempt become orphan MODULETEMPs; V3Dead removes
            // them along with the dead always_comb driver.
            replaceBodyOnBuildError(flp, propp, result.errorEmitted);
            VL_DO_DANGLING(ધકેલો_Deletep(senTreep), senTreep);
            પાછા;
        }
        // After the build, so a construct the builder rejects reports itself.
        જો (propertyControlp) {
            seqBodyp->v3warn(E_UNSUPPORTED,
                             "Unsupported: temporal property if/case with " << propertyControlp);
            replaceBodyOnBuildError(flp, propp, /*errorEmitted=*/સાચું);
            VL_DO_DANGLING(ધકેલો_Deletep(senTreep), senTreep);
            પાછા;
        }
        જો (ગણતરી_Matches && !v3Global.opt.coverageUser()) {
            SvaNfaBuilder::freeUnlinkedCondp(result.finalCondp);
            VL_DO_DANGLING(ધકેલો_Deletep(senTreep), senTreep);
            Ast_ગાંઠ* અચલ innerPropp = propp->propp();
            innerPropp->replaceWith(નવું AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(ધકેલો_Deletep(innerPropp), innerPropp);
            પાછા;
        }

        AstAssert* અચલ assertAssertp = VN_CAST(assertp, Assert);
        અચલ તાર્કિક splitImplicationPasssp = assertAssertp && assertAssertp->passsp()
                                            && parts.hasImplication
                                            && canSplitImplicationPassActions(parts);
        અચલ તાર્કિક needMatch = assertAssertp && assertAssertp->passsp()
                               && (!parts.hasImplication || splitImplicationPasssp);

        અચલ તાર્કિક needsThreadFailReplay
            = assertAssertp && assertAssertp->failsp() && !parts.hasImplication;
        અચલ auto signals
            = m_loweringp->lower(assertp, graph, senTreep, result.finalCondp, disableExprp,
                                 needsThreadFailReplay, needsThreadFailReplay && !negated);
        Ast_ગાંઠ_પદાવલિ* matchExprp = nullptr;
        Ast_ગાંઠ_પદાવલિ* outputExprp = m_loweringp->assembleResult(
            assertp, negated, result.finalCondp, signals, needMatch ? &matchExprp : nullptr);
        જો (ગણતરી_Matches) {
            UASSERT_OBJ(signals.matchCountp, coverp, "Cover sequence missing match count");
            VL_DO_DANGLING(outputExprp->delete_વૃક્ષ(), outputExprp);
            propp->matchCountp(signals.matchCountp);
            outputExprp = નવું AstNeq{flp, signals.matchCountp->clone_વૃક્ષ_Pure(ખોટું),
                                     newTypedConstp(flp, signals.matchCountp->dtypep(), 0)};
        }

        Ast_Sen_વૃક્ષ* અચલ threadFailReplaySenTreep
            = signals.threadFailCountp ? senTreep->clone_વૃક્ષ(ખોટું) : nullptr;

        VL_DO_DANGLING(ધકેલો_Deletep(senTreep), senTreep);
        જો (result.finalCondp && !result.finalCondp->backp()) ધકેલો_Deletep(result.finalCondp);

        જો (splitImplicationPasssp) {
            splitImplicationPassActions(assertAssertp, parts, matchExprp);
        } નહીંતર {
            attachActionHandlers(assertAssertp, matchExprp, threadFailReplaySenTreep,
                                 signals.threadFailCountp);
        }

        Ast_ગાંઠ* અચલ innerPropp = propp->propp();
        innerPropp->replaceWith(outputExprp);
        VL_DO_DANGLING(ધકેલો_Deletep(innerPropp), innerPropp);

        UINFO(4, "NFA converted assertion at " << flp << endl);

        જો (dumpGraphLevel() >= 6) graph.m_graph.dumpDotFilePrefixed("assert-nfa");
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_loweringp);
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultDisablep);
        m_modp = nodep;
        // A class nested in a module is in the scope of the module's default clocking
        જો (!VN_IS(nodep, Class)) m_defaultClockingp = nullptr;
        m_defaultDisablep = nodep->defaultDisablep();
        SvaNfaLowering lowering{nodep};
        m_loweringp = &lowering;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstClocking* nodep) override {
        જો (nodep->isDefault() && !m_defaultClockingp) m_defaultClockingp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstDefaultDisable* nodep) override {}
    શૂન્ય visit(AstFell* nodep) override { visit_Sampled_કિંમત(nodep); }
    શૂન્ય visit(AstPast* nodep) override { visit_Sampled_કિંમત(nodep); }
    શૂન્ય visit(AstRose* nodep) override { visit_Sampled_કિંમત(nodep); }
    શૂન્ય visit(AstStable* nodep) override { visit_Sampled_કિંમત(nodep); }
    શૂન્ય visit(AstAssert* nodep) override { processAssertion(nodep); }
    શૂન્ય visit(AstCover* nodep) override { processAssertion(nodep); }
    શૂન્ય visit(AstRestrict* nodep) override {
        // Restrict property is ignored by simulators (IEEE 1800-2023 16.12.2).
        // Remove here so temporal SExpr don't leak to V3AssertPre.
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }
    શૂન્ય visit(AstAssertIntrinsic* nodep) override {}
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    explicit AssertNfaVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Top entry point

શૂન્ય V3AssertNfa::assertNfaAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":" << endl);
    { AssertNfaVisitor{nodep}; }
    V3Global::dump_Check_Global_વૃક્ષ("assertnfa", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
