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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3AssertNfa.h"

#شامل "V3Assert.h"
#شامل "V3Const.h"
#شامل "V3Graph.h"
#شامل "V3Stats.h"
#شامل "V3Task.h"
#شامل "V3UniqueNames.h"

#شامل <set>
#شامل <unordered_set>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// NFA Graph Data Structures (V3Graph-derived per upstream convention)

فضاینام {

رده SvaStateVertex;

// Per-vertex algorithm data, stored via V3GraphVertex::userp() during lowering
ساختار SvaVertexData final {
    AstVar* stateVarp = nullptr;  // NBA state register for this vertex
    AstVar* delayRingVarp = nullptr;  // Packed occupancy bits or per-slot match counts
    AstVar* delayRingHeadVarp
        = nullptr;  // Cached outgoing slot; isolates readers from in-place writes
    AstVar* delayRingIdxVarp = nullptr;  // Next slot written in delayRingVarp
    AstVar* delay_Ring_Live_شمار_Varp = nullptr;  // Number of threads in delayRingVarp
    AstVar* delayRingWrappedVarp = nullptr;  // All slots written since the last clear
    AstVar* doneLVarp = nullptr;  // SAnd LHS done-latch
    AstVar* doneRVarp = nullptr;  // SAnd RHS done-latch
    Ast_گره_عبارت* stateSigp = nullptr;  // Combinational state signal; OWNED during lowering
    Ast_گره_عبارت* state_شمار_Sigp = nullptr;  // Cover-sequence match count; OWNED during lowering
    Ast_گره_عبارت* state_شمار_Overflow_Sigp = nullptr;  // Count overflow; OWNED during lowering
    منطقی delayRingCounts = نادرست;  // Ring slots hold cover-sequence match counts
    منطقی needsReg = نادرست;  // True if vertex has incoming clocked edge
};

// NFA state vertex -- one per NFA position in the sequence evaluation
رده SvaStateVertex final : عمومی V3GraphVertex {
    VL_RTTI_IMPL(SvaStateVertex, V3GraphVertex)
عمومی:
    // True if this is the sequence-match terminal vertex
    منطقی m_isMatch = نادرست;
    // OWNED throughout-guard condition clones; IEEE 1800-2023 16.9.9
    std::vector<Ast_گره_عبارت*> m_throughoutConds;
    // Nonzero for a ring-buffer vertex for ## delays.
    منطقی m_isFixedDelayRing = نادرست;
    unsigned m_delayRingSize = 0;  // Number of ring slots. Range: max-min+1.
    Ast_گره_عبارت* m_delayRingClearCondp = nullptr;  // local RHS for pure-boolean range
    Ast_گره_عبارت* m_delayRingAdvanceCondp = nullptr;  // Advance only when this condition holds
    منطقی m_replayAbortReject = نادرست;  // Compressed repetition needs per-thread abort replay
    // OWNED; enclosing-abort fire condition clearing state or suppressing guard rejection
    Ast_گره_عبارت* m_abortClearp = nullptr;
    // OWNED; reject-abort fire condition rejecting all represented live threads
    Ast_گره_عبارت* m_abortRejectp = nullptr;
    // Liveness terminal (IEEE weak semantics): reject must not fire from this source
    منطقی m_isUnbounded = نادرست;
    // Temporal sequence AND combiner; IEEE 1800-2023 16.9.5
    منطقی m_isAndCombiner = نادرست;
    SvaStateVertex* m_andLhsTermp = nullptr;  // LHS sub-NFA terminal vertex
    SvaStateVertex* m_andRhsTermp = nullptr;  // RHS sub-NFA terminal vertex
    Ast_گره_عبارت* m_andLhsCondp = nullptr;  // OWNED; LHS final condition (may be nullptr)
    Ast_گره_عبارت* m_andRhsCondp = nullptr;  // OWNED; RHS final condition (may be nullptr)
    // Reject sink for SAnd rejectOnFail wiring; not a state-signal source
    منطقی m_isRejectSink = نادرست;
    // In-window vertex of a strong s_always[m:n]: if its state is still set at
    // end-of-simulation the universal-quantifier window never completed, which is
    // a liveness failure (IEEE 1800-2023 16.12.11 strong semantics).
    منطقی m_strongPending = نادرست;
    // Ring introduced for the checked portion of a bounded strong s_always window.
    منطقی m_strongAlwaysRing = نادرست;

    // CONSTRUCTORS
    explicit SvaStateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~SvaStateVertex() override {
        برای (Ast_گره_عبارت* cp : m_throughoutConds) VL_DO_DANGLING(cp->delete_درخت(), cp);
        اگر (m_delayRingClearCondp)
            VL_DO_DANGLING(m_delayRingClearCondp->delete_درخت(), m_delayRingClearCondp);
        اگر (m_delayRingAdvanceCondp)
            VL_DO_DANGLING(m_delayRingAdvanceCondp->delete_درخت(), m_delayRingAdvanceCondp);
        اگر (m_abortClearp) VL_DO_DANGLING(m_abortClearp->delete_درخت(), m_abortClearp);
        اگر (m_abortRejectp) VL_DO_DANGLING(m_abortRejectp->delete_درخت(), m_abortRejectp);
        اگر (m_andLhsCondp) VL_DO_DANGLING(m_andLhsCondp->delete_درخت(), m_andLhsCondp);
        اگر (m_andRhsCondp) VL_DO_DANGLING(m_andRhsCondp->delete_درخت(), m_andRhsCondp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string name() ثابت override {
        string name = "s" + cvtToStr(color());
        اگر (m_delayRingSize) {
            name += "\\n";
            name += m_isFixedDelayRing ? "fixed chain " : "range chain ";
            name += cvtToStr(m_delayRingSize) + " slots";
        }
        بازگشت name;
    }
    string dotColor() ثابت override {
        اگر (m_isMatch) بازگشت "red";
        اگر (m_delayRingSize) بازگشت "blue";
        اگر (m_isAndCombiner) بازگشت "purple";
        بازگشت "black";
    }
    // LCOV_EXCL_STOP
    // Access per-vertex algorithm data (valid only during lowering phase)
    SvaVertexData* datap() ثابت { بازگشت static_cast<SvaVertexData*>(userp()); }
};

// NFA transition edge -- clocked (##1) or combinational link (##0)
رده SvaTransEdge final : عمومی V3GraphEdge {
    VL_RTTI_IMPL(SvaTransEdge, V3GraphEdge)
عمومی:
    Ast_گره_عبارت* m_condp;  // Transition condition; nullptr = unconditional; OWNED
    منطقی m_consumesCycle;  // true = clocked edge (##1), false = link (##0/boolean)
    // Reject when source is active and condp is false; set only on
    // outermost required-step Link
    منطقی m_rejectOnFail = نادرست;
    // Optional dynamic condition vertex for m_rejectOnFail. Used when the
    // success condition is another NFA state rather than a static expression.
    SvaStateVertex* m_condVtxp = nullptr;

    // CONSTRUCTORS
    SvaTransEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* بالاترین, Ast_گره_عبارت* condp,
                 منطقی consumesCycle)
        : V3GraphEdge{graphp, fromp, بالاترین, /*weight=*/1}
        , m_condp{condp}
        , m_consumesCycle{consumesCycle} {}
    ~SvaTransEdge() override {
        اگر (m_condp) VL_DO_DANGLING(m_condp->delete_درخت(), m_condp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string dotLabel() ثابت override { بازگشت m_consumesCycle ? "##1" : "link"; }
    string dotStyle() ثابت override { بازگشت m_consumesCycle ? "" : "dashed"; }
    // LCOV_EXCL_STOP
    // Typed accessors for NFA vertices
    SvaStateVertex* fromVtxp() ثابت { بازگشت static_cast<SvaStateVertex*>(fromp()); }
    SvaStateVertex* toVtxp() ثابت { بازگشت static_cast<SvaStateVertex*>(بالاترین()); }
};

// NFA graph container
رده SvaGraph final {
عمومی:
    V3Graph m_graph;  // Owns all vertices and edges
    SvaStateVertex* m_startVertexp = nullptr;  // Trigger/start vertex
    SvaStateVertex* m_matchVertexp = nullptr;  // Sequence-match terminal vertex

    // Create a new state vertex
    SvaStateVertex* createStateVertex() { بازگشت جدید SvaStateVertex{&m_graph}; }
    // Create the match terminal vertex
    SvaStateVertex* createMatchVertex() {
        SvaStateVertex* ثابت vtxp = createStateVertex();
        vtxp->m_isMatch = درست;
        m_matchVertexp = vtxp;
        بازگشت vtxp;
    }
    // Add a clocked transition edge (##1)
    SvaTransEdge* addClockedEdge(SvaStateVertex* fromp, SvaStateVertex* بالاترین,
                                 Ast_گره_عبارت* condp = nullptr) {
        بازگشت جدید SvaTransEdge{&m_graph, fromp, بالاترین, condp, /*consumesCycle=*/درست};
    }
    // Add a combinational link (##0 / boolean condition)
    SvaTransEdge* addLink(SvaStateVertex* fromp, SvaStateVertex* بالاترین,
                          Ast_گره_عبارت* condp = nullptr) {
        بازگشت جدید SvaTransEdge{&m_graph, fromp, بالاترین, condp, /*consumesCycle=*/نادرست};
    }
    // Collect all edges into a flat vector for iteration.
    // Used by the lowering phase which needs global edge scans.
    std::vector<ثابت SvaTransEdge*> allEdges() ثابت {
        std::vector<ثابت SvaTransEdge*> result;
        برای (ثابت V3GraphVertex& vtxr : m_graph.vertices()) {
            برای (ثابت V3GraphEdge& edger : vtxr.outEdges()) {
                result.افزودن_back(static_cast<ثابت SvaTransEdge*>(&edger));
            }
        }
        بازگشت result;
    }
};

//######################################################################
// Builder result: terminal vertex + optional final condition (match Link condition).
ساختار BuildResult final {
    SvaStateVertex* termVertexp;  // Primary terminal; contributes to both match and reject
    Ast_گره_عبارت* finalCondp;  // nullptr = unconditional
    // Mid-window sources for range delays (pure boolean RHS): match-only (isUnbounded)
    std::vector<SvaStateVertex*> midSources;
    منطقی errorEmitted = نادرست;  // Builder already emitted specific error; skip generic
    // For cover_sequence: when true, midSources already enumerate every
    // end-of-match, so wireMatchAndMidSources must NOT add the main
    // termVtxp -> matchVertex Link (would double-count via the merge vertex).
    منطقی termIsMidMerge = نادرست;
    منطقی valid() ثابت { بازگشت termVertexp != nullptr; }
    ایستا BuildResult fail(منطقی errored = نادرست) { بازگشت {nullptr, nullptr, {}, errored}; }
    ایستا BuildResult failWithError() { بازگشت {nullptr, nullptr, {}, درست}; }
};

// Parser-marked SAnd of overlapped implications: a property if/else/case.
ایستا منطقی hasPropertyControlConjunction(ثابت Ast_گره_عبارت* nodep) {
    بازگشت nodep->exists([](ثابت AstSAnd* andp) { بازگشت andp->propertyControl(); });
}

ایستا AstConst* newTypedConstp(File_سطر* ثابت flp, ثابت Ast_گره_DType* ثابت dtypep,
                                ثابت uint32_t مقدار) {
    AstConst* ثابت constp = جدید AstConst{flp, AstConst::DTyped{}, dtypep};
    constp->رقم().setLong(مقدار);
    بازگشت constp;
}

ایستا Ast_گره_عبارت* sampled(Ast_گره_عبارت* exprp) {
    بازگشت جدید AstSampled{exprp->fileline(), exprp, exprp->dtypep(), درست};
}

ایستا ثابت نویسه* assertPassOnQuery(منطقی vacuous) {
    ایستا constexpr ثابت نویسه* queries[2]
        = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
    بازگشت queries[vacuous];
}

ایستا Ast_گره_عبارت* assertOnCond(File_سطر* flp, VAssertType type,
                                 VAssertDirectiveType directiveType) {
    اگر (!v3Global.opt.assertOn()) { بازگشت جدید AstConst{flp, AstConst::BitFalse{}}; }
    بازگشت جدید AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
}

ایستا Ast_گره_عبارت* assertKillGet(File_سطر* flp, VAssertType type,
                                  VAssertDirectiveType directiveType) {
    بازگشت جدید AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_KILL", type, directiveType),
                        32};
}

ایستا string assertActionControlPrefix(VAssertDirectiveType directiveType) {
    ثابت صحیح controlled = !!(static_cast<صحیح>(directiveType)
                              & (static_cast<صحیح>(VAssertDirectiveType::ASSERT)
                                 | static_cast<صحیح>(VAssertDirectiveType::COVER)
                                 | static_cast<صحیح>(VAssertDirectiveType::ASSUME)));
    ثابت صحیح checkRuntime = controlled & static_cast<صحیح>(v3Global.opt.assertOn());
    بازگشت "("s + std::to_string(controlled ^ 1) + " || ("s + std::to_string(checkRuntime)
           + " && "s;
}

ایستا Ast_گره_عبارت* assertPassOnCond(File_سطر* flp, VAssertType type,
                                     VAssertDirectiveType directiveType, منطقی vacuous) {
    بازگشت جدید AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType)
            + "))"s,
        1};
}

ایستا Ast_گره_عبارت* assertFailOnCond(File_سطر* flp, VAssertType type,
                                     VAssertDirectiveType directiveType) {
    بازگشت جدید AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType) + "))"s,
        1};
}

ایستا AstIf* newPassOnIf(File_سطر* flp, Ast_گره_عبارت* firep, Ast_گره* bodyp, VAssertType type,
                          VAssertDirectiveType directiveType, منطقی vacuous) {
    Ast_گره_عبارت* ثابت condp
        = جدید AstLogAnd{flp, firep, assertPassOnCond(flp, type, directiveType, vacuous)};
    AstIf* ثابت ifp = جدید AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(درست);
    ifp->user1(درست);
    بازگشت ifp;
}

ایستا Ast_گره_Stmt* newIfAssertFailOn(Ast_گره* bodyp, VAssertDirectiveType directiveType,
                                      VAssertType type) {
    File_سطر* ثابت flp = bodyp->fileline();
    Ast_گره_عبارت* ثابت condp = assertFailOnCond(flp, type, directiveType);
    AstIf* ثابت ifp = جدید AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(درست);
    ifp->user1(درست);
    بازگشت ifp;
}

//######################################################################
// NFA Builder

رده SvaNfaBuilder final {
    SvaGraph& m_graph;  // NFA graph being built
    Ast_گره_Module* ثابت m_modp;  // Module to receive hoisted sampled-prop temps
    V3UniqueNames& m_propTempNames;  // Module-shared temp-var name source
    std::vector<Ast_گره_عبارت*> m_temporal_Guard_پشته;  // Guards active across nested temporal states
    // Outer abort conditions, AND-ed as !cond into inner abort edges
    // (IEEE 1800-2023 16.12.14 outer-wraps-inner).
    std::vector<Ast_گره_عبارت*> m_outer_Abort_پشته;
    منطقی m_inUnboundedScope = نادرست;  // Sticky: nodes created after inherit liveness
    منطقی m_markStrongPending = نادرست;  // Mark new vertices as strong s_always in-window
    منطقی m_isCover = نادرست;  // Cover directives do not fail at end-of-simulation
    // IEEE 1800-2023 16.14.3 cover sequence: each end-of-match fires the action,
    // not just the first. Builder builds parallel-branch (no first-match-wins)
    // topology when true. Default false preserves cover_property semantics.
    منطقی m_isCoverSeq = نادرست;
    // Unsupported endpoint topology must reject, not ignore, or the wait hangs
    منطقی m_isSeqEvent = نادرست;
    منطقی m_inSequencePrefix = نادرست;  // Prefix composition does not preserve every endpoint
    منطقی m_wantBoolLeaf = نادرست;  // Building a |-> / |=> or top-level body (see buildOrMerge)

    ساختار RangeDelayRejectInfo final {
        SvaStateVertex* startp = nullptr;
        unsigned range = 0;
        صحیح rhsLen = 0;
    };

    تهی warnEndpointUnsupported(File_سطر* flp, ثابت string& what) ثابت {
        اگر (m_isSeqEvent) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: sequence used as an event control with " << what);
        } وگرنه {
            flp->v3warn(COVERIGN, "Ignoring unsupported: cover sequence with " << what);
        }
    }

    Ast_گره_عبارت* throughoutCond(Ast_گره_عبارت* baseCondp, File_سطر* flp) {
        اگر (m_temporal_Guard_پشته.empty()) بازگشت baseCondp;
        // AND all active temporal guards (supports nesting)
        // Each must use $sampled values.
        Ast_گره_عبارت* guardp = nullptr;
        برای (Ast_گره_عبارت* ثابت condp : m_temporal_Guard_پشته) {
            Ast_گره_عبارت* ثابت clonep = sampled(condp->clone_درخت_Pure(نادرست));
            اگر (!guardp) {
                guardp = clonep;
            } وگرنه {
                guardp = جدید AstLogAnd{flp, guardp, clonep};
            }
        }
        اگر (baseCondp) { guardp = جدید AstLogAnd{flp, baseCondp, guardp}; }
        بازگشت guardp;
    }

    ایستا unsigned getConstUInt(Ast_گره_عبارت* exprp) {
        Ast_گره_عبارت* ثابت constp = V3Const::constifyEdit(exprp->clone_درخت_Pure(نادرست));
        ثابت AstConst* ثابت cp = VN_CAST(constp, Const);
        ثابت unsigned val = cp ? cp->toUInt() : 0;
        VL_DO_DANGLING(constp->delete_درخت(), constp);
        بازگشت val;
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
    ایستا صحیح fixedLength(Ast_گره_عبارت* nodep) {
        اگر (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
            اگر (!delayp || !delayp->isCycleDelay()) بازگشت -1;
            unsigned delayCycles;
            اگر (delayp->isRangeDelay()) {
                اگر (delayp->isUnbounded()) بازگشت -1;  // LCOV_EXCL_LINE
                ثابت unsigned minD = getConstUInt(delayp->lhsp());
                ثابت unsigned maxD = getConstUInt(delayp->rhsp());
                اگر (minD != maxD) بازگشت -1;
                delayCycles = minD;
            } وگرنه {
                delayCycles = getConstUInt(delayp->lhsp());
            }
            صحیح preLen = 0;
            اگر (Ast_گره_عبارت* ثابت prep = sexprp->preExprp()) {
                preLen = fixedLength(prep);
                اگر (preLen < 0) بازگشت -1;  // LCOV_EXCL_LINE
            }
            ثابت صحیح bodyLen = fixedLength(sexprp->exprp());
            اگر (bodyLen < 0) بازگشت -1;  // LCOV_EXCL_LINE
            بازگشت preLen + delayCycles + bodyLen;
        }
        اگر (AstSThroughout* ثابت throughp = VN_CAST(nodep, SThroughout)) {
            بازگشت fixedLength(throughp->rhsp());
        }
        اگر (AstSOr* ثابت orp = VN_CAST(nodep, SOr)) {
            // Alternatives must share one end cycle; buildSWithin relies on
            // this to pair the OR with an SIntersect.
            ثابت صحیح lhsLen = fixedLength(orp->lhsp());
            ثابت صحیح rhsLen = fixedLength(orp->rhsp());
            اگر (lhsLen < 0 || rhsLen < 0 || lhsLen != rhsLen) بازگشت -1;
            بازگشت lhsLen;
        }
        اگر (AstSWithin* ثابت withinp = VN_CAST(nodep, SWithin)) {
            // `seq1 within seq2` ends at seq2's end cycle (IEEE 16.9.10).
            ثابت صحیح lhsLen = fixedLength(withinp->lhsp());
            ثابت صحیح rhsLen = fixedLength(withinp->rhsp());
            اگر (lhsLen < 0 || rhsLen < 0 || lhsLen > rhsLen) بازگشت -1;
            بازگشت rhsLen;
        }
        // LCOV_EXCL_START -- defensive: V3AssertPre rejects composite SVA ops
        // nested in an intersect arm before fixedLength runs (clock-context
        // resolution fails). Kept as a guard in case future parser relaxations
        // permit it.
        اگر (nodep->isMultiCycleSva()) بازگشت -1;
        // LCOV_EXCL_STOP
        // Plain boolean expression (no SVA constructs) -- 0 cycles.
        بازگشت 0;
    }

    // Contiguous match-length range [lo,hi] for an operand whose length varies
    // from AT MOST ONE ranged cycle delay; {-1,-1} otherwise. Drives the
    // variable-length `intersect` lowering (IEEE 1800-2023 16.9.6): more than
    // one ranged delay would make the per-length realization ambiguous, so it
    // is reported unsupported rather than mis-paired.
    ایستا std::pair<صحیح, صحیح> lengthRange(Ast_گره_عبارت* nodep) {
        اگر (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
            اگر (!delayp || !delayp->isCycleDelay()) بازگشت {-1, -1};
            std::pair<صحیح, صحیح> delayRange;
            اگر (delayp->isRangeDelay()) {
                اگر (delayp->isUnbounded()) بازگشت {-1, -1};
                ثابت unsigned minD = getConstUInt(delayp->lhsp());
                ثابت unsigned maxD = getConstUInt(delayp->rhsp());
                delayRange = {minD, maxD};
            } وگرنه {
                ثابت unsigned d = getConstUInt(delayp->lhsp());
                delayRange = {d, d};
            }
            std::pair<صحیح, صحیح> preRange{0, 0};
            اگر (Ast_گره_عبارت* ثابت prep = sexprp->preExprp()) {
                preRange = lengthRange(prep);
                اگر (preRange.first < 0) بازگشت {-1, -1};
            }
            ثابت std::pair<صحیح, صحیح> bodyRange = lengthRange(sexprp->exprp());
            اگر (bodyRange.first < 0) بازگشت {-1, -1};
            ثابت صحیح variableParts = (preRange.first != preRange.second)
                                      + (delayRange.first != delayRange.second)
                                      + (bodyRange.first != bodyRange.second);
            اگر (variableParts > 1) بازگشت {-1, -1};
            بازگشت {preRange.first + delayRange.first + bodyRange.first,
                    preRange.second + delayRange.second + bodyRange.second};
        }
        اگر (AstSThroughout* ثابت throughp = VN_CAST(nodep, SThroughout)) {
            بازگشت lengthRange(throughp->rhsp());
        }
        اگر (nodep->isMultiCycleSva()) بازگشت {-1, -1};
        بازگشت {0, 0};  // plain boolean -- 0 cycles
    }

    // Clone `operand` with its sole variable ranged cycle delay pinned so the
    // total match length is exactly `len`. `lo` is the operand's minimum length
    // (lengthRange().first). A fixed operand has no such delay and is returned
    // as a plain clone (callers only request its single achievable length).
    ایستا Ast_گره_عبارت* realizeAtLength(Ast_گره_عبارت* operand, صحیح len, صحیح lo) {
        Ast_گره_عبارت* ثابت clonep = operand->clone_درخت_Pure(نادرست);
        AstDelay* rangeDelayp = nullptr;
        clonep->foreach([&](AstDelay* dp) {
            اگر (!rangeDelayp && dp->isRangeDelay() && !dp->isUnbounded()
                && getConstUInt(dp->lhsp()) != getConstUInt(dp->rhsp())) {
                rangeDelayp = dp;
            }
        });
        اگر (rangeDelayp) {
            File_سطر* ثابت flp = rangeDelayp->fileline();
            ثابت unsigned pinned = getConstUInt(rangeDelayp->lhsp()) + (len - lo);
            Ast_گره_عبارت* ثابت oldMinp = rangeDelayp->lhsp();
            oldMinp->replaceWith(جدید AstConst{flp, pinned});
            VL_DO_DANGLING(oldMinp->delete_درخت(), oldMinp);
            // Drop the max bound so it lowers as a fixed `##d`, not `##[d:d]`.
            Ast_گره* ثابت oldMaxp = rangeDelayp->rhsp()->unlinkFrBack();
            VL_DO_DANGLING(oldMaxp->delete_درخت(), oldMaxp);
        }
        بازگشت clonep;
    }

    // Cuts AST size from O(N * sizeof(exprp)) to O(N) + O(sizeof(exprp)) by
    // sharing a single `VarRef` across N check edges. Hoist also matches the
    // IEEE 1800-2023 16.9.9 "single preponed-region snapshot" semantic for
    // any exprp -- even an impure one would now evaluate exactly once per
    // clock instead of N times. Orphan temps from failed builds are unused
    // MODULETEMPs and are removed by V3Dead.
    AstVar* tryHoistSampled(Ast_گره_عبارت* exprp, File_سطر* flp, unsigned clone_شمار) {
        constexpr unsigned kHoistThreshold = 2;
        اگر (clone_شمار < kHoistThreshold) بازگشت nullptr;
        AstVar* ثابت tempVarp
            = جدید AstVar{flp, VVarType::MODULETEMP, m_propTempNames.get(exprp), exprp->dtypep()};
        m_modp->addStmtsp(tempVarp);
        AstAssign* ثابت assignp = جدید AstAssign{flp, جدید AstVarRef{flp, tempVarp, VAccess::WRITE},
                                                 sampled(exprp->clone_درخت_Pure(نادرست))};
        m_modp->addStmtsp(جدید AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, assignp});
        بازگشت tempVarp;
    }

    ایستا Ast_گره_عبارت* sampledRefOrClone(AstVar* hoistVarp, Ast_گره_عبارت* exprp, File_سطر* flp) {
        اگر (hoistVarp) بازگشت جدید AstVarRef{flp, hoistVarp, VAccess::READ};
        بازگشت sampled(exprp->clone_درخت_Pure(نادرست));
    }

    // Create vertex and inherit temporal guards from the current scope.
    SvaStateVertex* scopedCreateVertex() {
        SvaStateVertex* ثابت vtxp = m_graph.createStateVertex();
        برای (Ast_گره_عبارت* ثابت cp : m_temporal_Guard_پشته) {
            vtxp->m_throughoutConds.افزودن_back(cp->clone_درخت_Pure(نادرست));
        }
        اگر (m_inUnboundedScope) vtxp->m_isUnbounded = درست;
        اگر (m_markStrongPending) vtxp->m_strongPending = درست;
        بازگشت vtxp;
    }

    // AND current temporal guards into every edge/link.
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* بالاترین, Ast_گره_عبارت* condp,
                              File_سطر* flp) {
        بازگشت m_graph.addLink(fromp, بالاترین, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* بالاترین, File_سطر* flp) {
        بازگشت m_graph.addLink(fromp, بالاترین, throughoutCond(nullptr, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* بالاترین, Ast_گره_عبارت* condp,
                              File_سطر* flp) {
        بازگشت m_graph.addClockedEdge(fromp, بالاترین, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* بالاترین, File_سطر* flp) {
        بازگشت m_graph.addClockedEdge(fromp, بالاترین, throughoutCond(nullptr, flp));
    }

    SvaStateVertex* addDelayChain(SvaStateVertex* startp, unsigned size, File_سطر* flp,
                                  منطقی isFixed = درست, Ast_گره_عبارت* clearCondp = nullptr,
                                  Ast_گره_عبارت* advanceCondp = nullptr) {
        اگر (isFixed && size == 0) بازگشت startp;
        UASSERT_OBJ(size > 0, startp, "Delay chain needs at least one slot");
        اگر (isFixed && size == 1 && !advanceCondp) {
            SvaStateVertex* ثابت nextp = scopedCreateVertex();
            guardedEdge(startp, nextp, flp);
            بازگشت nextp;
        }
        SvaStateVertex* ثابت ringVtxp = scopedCreateVertex();
        ringVtxp->m_isFixedDelayRing = isFixed;
        ringVtxp->m_delayRingSize = size;
        اگر (clearCondp) {
            UASSERT_OBJ(!isFixed, startp, "Fixed delay cannot have a clear condition");
            ringVtxp->m_delayRingClearCondp = clearCondp->clone_درخت_Pure(نادرست);
        }
        ringVtxp->m_delayRingAdvanceCondp = advanceCondp;
        اگر (isFixed) {
            guardedEdge(startp, ringVtxp, flp);
        } وگرنه {
            guardedLink(startp, ringVtxp, flp);
        }
        بازگشت ringVtxp;
    }

    // Build NFA for an SExpr. finalCond = RHS (not yet added as a vertex).
    // isTopLevelStep: marks outermost required boolean check as rejectOnFail.
    // Apply a range delay `##[M:N]` to currentp. Returns true on success. On
    // failure, emits a diagnostic and returns false.
    منطقی applyRangeDelay(AstSExpr* sexprp, SvaStateVertex*& currentp,
                         std::vector<SvaStateVertex*>& midSources,
                         RangeDelayRejectInfo* rangeRejectInfop = nullptr) {
        File_سطر* ثابت flp = sexprp->fileline();
        AstDelay* ثابت delayp = VN_AS(sexprp->delayp(), Delay);
        Ast_گره_عبارت* ثابت rhsExprp = sexprp->exprp();
        ثابت unsigned minDelay = getConstUInt(delayp->lhsp());
        اگر (delayp->isUnbounded()) {
            // `##[M:$]`: wait M cycles, then retain every matured attempt in a
            // one-slot ring while also exposing the first eligible tick.
            currentp = addDelayChain(currentp, minDelay, flp);
            SvaStateVertex* ثابت waitRingp = addDelayChain(currentp, 1, flp, نادرست);
            // The ring's live count is updated in NBA, so this link retains the previous
            // tick's live attempts alongside new arrivals for the next tick.
            guardedLink(waitRingp, waitRingp, flp);
            SvaStateVertex* ثابت mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(waitRingp, mergeVtxp, flp);
            currentp = mergeVtxp;
            currentp->m_isUnbounded = درست;  // Liveness, so no reject.
            m_inUnboundedScope = درست;
            بازگشت درست;
        }
        ثابت unsigned maxDelay = getConstUInt(delayp->rhsp());
        اگر (minDelay == maxDelay) {
            currentp = addDelayChain(currentp, minDelay, flp);
            بازگشت درست;
        }
        ثابت unsigned range = maxDelay - minDelay;
        currentp = addDelayChain(currentp, minDelay, flp);
        ثابت منطقی multiCycleRhs = rhsExprp->isMultiCycleSva();
        ثابت Ast_گره_عبارت* ثابت preExprp = sexprp->preExprp();
        // Prefix and nested-sequence composition can lose endpoints or their multiplicity
        // required by cover sequence (IEEE 1800-2023 16.14.3), regardless of the range width.
        اگر (m_isCoverSeq
            && (multiCycleRhs || m_inSequencePrefix
                || (preExprp && preExprp->isMultiCycleSva()))) {
            warnEndpointUnsupported(flp, "this ranged cycle delay");
            بازگشت نادرست;
        }
        اگر (m_inSequencePrefix) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: Bounded ranged cycle delay in a sequence prefix");
            بازگشت نادرست;
        }
        اگر (multiCycleRhs) {
            // Launch a candidate at every eligible tick. Candidate-local misses are not
            // assertion rejects while a later position can still match.
            اگر (rangeRejectInfop) {
                ثابت صحیح rhsLen = fixedLength(rhsExprp);
                اگر (rhsLen >= 0) *rangeRejectInfop = {currentp, range, rhsLen};
            }
            SvaStateVertex* ثابت tailp = addDelayChain(currentp, range + 1U, flp, نادرست);
            SvaStateVertex* ثابت mergeVtxp = scopedCreateVertex();
            mergeVtxp->m_isUnbounded = درست;
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(tailp, mergeVtxp, flp);
            currentp = mergeVtxp;
            m_inUnboundedScope = درست;
        } وگرنه {
            // The first eligible tick is explicit. The ring retains attempts for the
            // remaining ticks and exposes the final tick for rejection. Only properties
            // clear on success, since cover sequence counts every endpoint.
            midSources.افزودن_back(currentp);
            currentp = addDelayChain(currentp, range + 1U, flp, نادرست,
                                     m_isCoverSeq ? nullptr : rhsExprp);
        }
        بازگشت درست;
    }

    تهی addFiniteRangeReject(ثابت RangeDelayRejectInfo& info, ثابت BuildResult& result,
                              File_سطر* flp) {
        اگر (!info.startp) بازگشت;

        SvaStateVertex* ثابت expiryVtxp
            = addDelayChain(info.startp, info.range + info.rhsLen, flp);
        SvaStateVertex* ثابت expiryMatchp = scopedCreateVertex();
        std::vector<SvaStateVertex*> sources = result.midSources;
        sources.افزودن_back(result.termVertexp);
        برای (SvaStateVertex* ثابت srcp : sources) {
            Ast_گره_عبارت* ثابت condp
                = result.finalCondp ? sampled(result.finalCondp->clone_درخت_Pure(نادرست)) : nullptr;
            SvaStateVertex* ثابت successNowp = scopedCreateVertex();
            guardedLink(srcp, successNowp, condp, flp);
            // Retain a success through the latest deadline it can satisfy.
            SvaStateVertex* ثابت historyp
                = addDelayChain(successNowp, info.range + 1U, flp, نادرست);
            guardedLink(successNowp, expiryMatchp, flp);
            guardedLink(historyp, expiryMatchp, flp);
        }

        SvaStateVertex* ثابت sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = درست;
        SvaTransEdge* ثابت rejectp = m_graph.addLink(expiryVtxp, sinkVtxp);
        rejectp->m_rejectOnFail = درست;
        rejectp->m_condVtxp = expiryMatchp;
    }

    BuildResult buildSExpr(AstSExpr* sexprp, SvaStateVertex* entryVtxp,
                           منطقی is_بالاترین_Level_Step = نادرست) {
        AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
        اگر (!delayp || !delayp->isCycleDelay()) بازگشت BuildResult::fail();

        File_سطر* ثابت flp = sexprp->fileline();
        Ast_گره_عبارت* ثابت exprp = sexprp->exprp();

        // Handle LHS (preExpr)
        SvaStateVertex* currentp = entryVtxp;
        اگر (Ast_گره_عبارت* ثابت preExprp = sexprp->preExprp()) {
            VL_RESTORER(m_inSequencePrefix);
            m_inSequencePrefix = درست;
            ثابت BuildResult pre = build_عبارت(preExprp, currentp, is_بالاترین_Level_Step);
            اگر (!pre.valid()) بازگشت BuildResult::fail(pre.errorEmitted);  // LCOV_EXCL_LINE
            اگر (pre.finalCondp) {
                SvaStateVertex* ثابت condVtxp = scopedCreateVertex();
                SvaTransEdge* ثابت edgep = guardedLink(
                    pre.termVertexp, condVtxp, sampled(pre.finalCondp->clone_درخت_Pure(نادرست)), flp);
                اگر (is_بالاترین_Level_Step && !pre.termVertexp->m_isUnbounded && !m_inUnboundedScope) {
                    // Do not mark liveness sources: first boolean check is deferred.
                    edgep->m_rejectOnFail = درست;
                }
                freeUnlinkedCondp(pre.finalCondp);
                currentp = condVtxp;
            } وگرنه {
                currentp = pre.termVertexp;
            }
        }

        // Handle delay
        std::vector<SvaStateVertex*> rangeMidSources;
        RangeDelayRejectInfo rangeRejectInfo;
        ثابت منطقی addRangeReject = is_بالاترین_Level_Step && !m_inUnboundedScope;
        اگر (delayp->isRangeDelay()) {
            اگر (!applyRangeDelay(sexprp, currentp, rangeMidSources,
                                 addRangeReject ? &rangeRejectInfo : nullptr)) {
                بازگشت BuildResult::failWithError();
            }
        } وگرنه {
            ثابت unsigned delayCycles = getConstUInt(delayp->lhsp());
            currentp = addDelayChain(currentp, delayCycles, flp);
        }

        // Multi-cycle RHS: recurse (only plain boolean is returned as finalCondp).
        اگر (exprp->isMultiCycleSva()) {
            ثابت BuildResult result = build_عبارت(exprp, currentp, is_بالاترین_Level_Step);
            اگر (result.valid()) addFiniteRangeReject(rangeRejectInfo, result, flp);
            بازگشت result;
        }
        بازگشت {currentp, exprp, std::move(rangeMidSources)};
    }

    BuildResult buildConsRep(AstSConsRep* repp, SvaStateVertex* entryVtxp,
                             منطقی is_بالاترین_Level_Step = نادرست) {
        File_سطر* ثابت flp = repp->fileline();
        Ast_گره_عبارت* ثابت exprp = repp->exprp();
        // Multi-cycle expr in ConsRep not yet supported; bail to avoid invalid AST.
        اگر (exprp->isMultiCycleSva()) {
            repp->v3warn(E_UNSUPPORTED, "Unsupported: multi-cycle sequence expression inside"
                                        " consecutive repetition (IEEE 1800-2023 16.9.2)");
            بازگشت BuildResult::failWithError();
        }
        ثابت unsigned minN = getConstUInt(repp->countp());

        // Sum sites across prefix + unbounded/range tail so one hoist covers
        // every check edge of this repetition.
        unsigned totalSites = minN;
        اگر (repp->unbounded()) {
            totalSites += 1;
        } وگرنه اگر (repp->maxCountp()) {
            totalSites += getConstUInt(repp->maxCountp()) - minN;
        }
        AstVar* ثابت hoistVarp = tryHoistSampled(exprp, flp, totalSites);

        // Cover-sequence (IEEE 1800-2023 16.14.3): collect each end-of-match
        // position so they all fire the action, not just the merged terminal.
        std::vector<SvaStateVertex*> consMidSources;

        SvaStateVertex* currentp = entryVtxp;
        برای (unsigned i = 0; i < minN; ++i) {
            // Keep the first repetition explicit, collapse all remaining checks into the ring.
            اگر (i == 1) {
                currentp = addDelayChain(currentp, minN - 1, flp);
                currentp->m_delayRingClearCondp
                    = جدید AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                currentp->m_replayAbortReject = درست;
                اگر (is_بالاترین_Level_Step) {
                    currentp->m_throughoutConds.افزودن_back(
                        sampledRefOrClone(hoistVarp, exprp, flp));
                }
                i = minN - 1;
            }
            // Every repetition in the minimum prefix is required.
            SvaStateVertex* ثابت condVtxp = scopedCreateVertex();
            SvaTransEdge* ثابت linkp
                = guardedLink(currentp, condVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            // Only an outermost required step rejects the property. Its first check is explicit,
            // later required checks reject through the ring's throughout guard.
            اگر (is_بالاترین_Level_Step && i == 0) linkp->m_rejectOnFail = درست;
            currentp = condVtxp;
        }
        // After minN: currentp is the first valid end-of-match position for [*m:n].
        اگر (m_isCoverSeq && (repp->unbounded() || repp->maxCountp())) {
            consMidSources.افزودن_back(currentp);
        }

        اگر (repp->unbounded()) {
            اگر (minN == 0) {
                SvaStateVertex* ثابت waitVtxp = scopedCreateVertex();
                guardedEdge(currentp, waitVtxp, flp);
                SvaStateVertex* ثابت checkVtxp = scopedCreateVertex();
                guardedLink(waitVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedEdge(checkVtxp, waitVtxp, flp);
                guardedLink(currentp, checkVtxp, flp);
                currentp = checkVtxp;
            } وگرنه {
                SvaStateVertex* ثابت loopBackVtxp = scopedCreateVertex();
                guardedEdge(currentp, loopBackVtxp, flp);
                SvaStateVertex* ثابت reCheckVtxp = scopedCreateVertex();
                guardedLink(loopBackVtxp, reCheckVtxp, sampledRefOrClone(hoistVarp, exprp, flp),
                            flp);
                guardedEdge(reCheckVtxp, loopBackVtxp, flp);
                guardedLink(reCheckVtxp, currentp, flp);
            }
            currentp->m_isUnbounded = درست;
            m_inUnboundedScope = درست;
        } وگرنه اگر (repp->maxCountp()) {
            ثابت unsigned maxN = getConstUInt(repp->maxCountp());
            SvaStateVertex* ثابت mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            unsigned tailMinN = minN;
            SvaStateVertex* tailStartp = currentp;
            اگر (minN == 0) {
                // Build the first optional iteration explicitly. Feeding the empty endpoint
                // directly into a range ring would incorrectly keep that match alive.
                SvaStateVertex* ثابت nextVtxp = scopedCreateVertex();
                guardedEdge(currentp, nextVtxp, flp);
                SvaStateVertex* ثابت checkVtxp = scopedCreateVertex();
                guardedLink(nextVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                اگر (m_isCoverSeq) consMidSources.افزودن_back(checkVtxp);
                tailStartp = checkVtxp;
                tailMinN = 1;
            }
            اگر (maxN > tailMinN) {
                // Add tail-ring only if the tail is non-empty.
                SvaStateVertex* ثابت nextVtxp
                    = addDelayChain(tailStartp, maxN - tailMinN + 1, flp, نادرست);
                nextVtxp->m_delayRingClearCondp
                    = جدید AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                nextVtxp->m_replayAbortReject = درست;
                SvaStateVertex* ثابت checkVtxp = scopedCreateVertex();
                guardedLink(nextVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                اگر (m_isCoverSeq) consMidSources.افزودن_back(checkVtxp);
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
        بازگشت res;
    }

    // always[lo:hi] / s_always[lo:hi] (IEEE 1800-2023 16.12.11).
    BuildResult buildPropAlways(AstPropAlways* nodep, SvaStateVertex* entryVtxp,
                                منطقی is_بالاترین_Level_Step = نادرست) {
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گره_عبارت* ثابت propp = nodep->propp();
        ثابت unsigned lo = getConstUInt(nodep->loBoundp());
        اگر (VN_IS(nodep->hiBoundp(), Unbounded)) {
            // Weak always [lo:$]: unbounded upper bound (IEEE 1800-2023 16.12.11).
            // p must hold at every clock tick at least lo cycles after the attempt
            // start; those ticks are not required to exist, so there is no
            // end-of-trace obligation (weak). The self-loop keeps the attempt live
            // every cycle; each observed cycle is a safety obligation, so a false p
            // rejects immediately.
            UASSERT_OBJ(!nodep->isStrong(), nodep, "Unbounded always must be weak (V3Width)");
            SvaStateVertex* ثابت livep = addDelayChain(entryVtxp, lo, flp);
            livep->m_isUnbounded = درست;
            guardedEdge(livep, livep, flp);  // stay active every subsequent cycle
            SvaStateVertex* ثابت sinkp = m_graph.createStateVertex();
            sinkp->m_isRejectSink = درست;
            SvaTransEdge* ثابت rejEdgep
                = guardedLink(livep, sinkp, sampled(propp->clone_درخت_Pure(نادرست)), flp);
            اگر (is_بالاترین_Level_Step) rejEdgep->m_rejectOnFail = درست;
            بازگشت {livep, nullptr, {}};
        }
        ثابت unsigned hi = getConstUInt(nodep->hiBoundp());
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
        SvaStateVertex* ثابت checkp = scopedCreateVertex();
        SvaTransEdge* ثابت linkp
            = guardedLink(currentp, checkp, sampled(propp->clone_درخت_Pure(نادرست)), flp);
        اگر (is_بالاترین_Level_Step) linkp->m_rejectOnFail = درست;
        currentp = checkp;
        m_temporal_Guard_پشته.افزودن_back(propp);
        currentp = addDelayChain(currentp, hi - lo, flp);
        اگر (nodep->isStrong() && currentp->m_delayRingSize) currentp->m_strongAlwaysRing = درست;
        m_temporal_Guard_پشته.برداشت_back();
        بازگشت {currentp, propp, {}};
    }

    BuildResult buildGotoRep(AstSGotoRep* repp, SvaStateVertex* entryVtxp) {
        File_سطر* ثابت flp = repp->fileline();
        Ast_گره_عبارت* ثابت exprp = repp->exprp();
        ثابت unsigned minN = getConstUInt(repp->countp());
        اگر (minN == 0) بازگشت BuildResult::fail();
        ثابت منطقی hasMax = repp->maxCountp() != nullptr;
        ثابت unsigned maxN = hasMax ? getConstUInt(repp->maxCountp()) : minN;
        اگر (m_isCoverSeq) {
            // Several attempts may coalesce in the one-bit wait vertex before reaching the ring,
            // so a cover sequence action block could run too few times.
            warnEndpointUnsupported(flp, "a goto repetition");
            بازگشت BuildResult::failWithError();
        }

        AstVar* ثابت hoistVarp = tryHoistSampled(exprp, flp, 2);

        // The first guardedEdge is the ##1 before waiting for a match. In the wait state, false
        // takes the clocked self-loop, while true takes the zero-delay guardedLink on that tick.
        SvaStateVertex* ثابت waitVtxp = scopedCreateVertex();
        guardedEdge(entryVtxp, waitVtxp, flp);
        guardedEdge(waitVtxp, waitVtxp,
                    جدید AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)}, flp);
        SvaStateVertex* currentp = scopedCreateVertex();
        guardedLink(waitVtxp, currentp, sampledRefOrClone(hoistVarp, exprp, flp), flp);

        اگر (minN > 1) {
            SvaStateVertex* ثابت ringVtxp = addDelayChain(
                currentp, minN - 1, flp, درست, nullptr, sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = درست;
            SvaStateVertex* ثابت checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            currentp = checkVtxp;
        }
        اگر (!hasMax) {
            currentp->m_isUnbounded = درست;  // [->N] waits unboundedly
            m_inUnboundedScope = درست;
            بازگشت {currentp, nullptr, {}};
        }

        // [->M:N]: the range ring holds matches from M through N and advances
        // only on expr, preserving arbitrarily long gaps between occurrences.
        SvaStateVertex* ثابت mergeVtxp = scopedCreateVertex();
        guardedLink(currentp, mergeVtxp, flp);  // accept at match_M
        اگر (maxN > minN) {
            SvaStateVertex* ثابت ringVtxp
                = addDelayChain(currentp, maxN - minN + 1, flp, نادرست, nullptr,
                                sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = درست;
            SvaStateVertex* ثابت checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            guardedLink(checkVtxp, mergeVtxp, flp);
        }
        mergeVtxp->m_isUnbounded = درست;  // [->M:N] still has unbounded waits between matches
        m_inUnboundedScope = درست;
        بازگشت {mergeVtxp, nullptr, {}};
    }

    // True if the sequence has a zero-minimum consecutive repetition, which may match empty
    ایستا منطقی hasZeroMinRep(ثابت Ast_گره_عبارت* seqp) {
        بازگشت seqp->exists(
            [](ثابت AstSConsRep* repp) { بازگشت getConstUInt(repp->countp()) == 0; });
    }

    // True if a leaf's condition has no implication or multi-cycle sequence in it
    // (a leaf can be e.g. 'not (x |-> y)')
    ایستا منطقی isPlainBoolean(ثابت Ast_گره_عبارت* condp) {
        بازگشت !condp->exists(
            [](ثابت Ast_گره_عبارت* np) { بازگشت np->isMultiCycleSva() || VN_IS(np, Implication); });
    }

    // Build merge vertex for SOr / LogOr: both branches feed into one vertex.
    BuildResult buildOrMerge(Ast_گره_عبارت* lhsp, Ast_گره_عبارت* rhsp, SvaStateVertex* entryVtxp,
                             File_سطر* flp, منطقی wantBoolLeaf) {
        ثابت BuildResult lhs = build_عبارت(lhsp, entryVtxp);
        ثابت BuildResult rhs = build_عبارت(rhsp, entryVtxp);
        اگر (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            بازگشت BuildResult::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP
        // IEEE 1800-2023 16.14.3: a cover sequence counts every end-of-match. A
        // sequence operand of 'or' can end more than once, but only its final
        // end reaches the merge vertex below, so reject sequence operands rather
        // than under-count. Plain boolean disjunction has one end per cycle and
        // is handled by the OR-fold.
        ثابت منطقی booleanOnly = lhs.termVertexp == entryVtxp && lhs.finalCondp
                                 && rhs.termVertexp == entryVtxp && rhs.finalCondp;
        اگر (m_isCoverSeq && !booleanOnly) {
            warnEndpointUnsupported(flp, "a sequence operand of 'or'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            بازگشت BuildResult::failWithError();
        }
        اگر (booleanOnly && wantBoolLeaf) {
            اگر (isPlainBoolean(lhs.finalCondp) && isPlainBoolean(rhs.finalCondp)) {
                // As a |-> / |=> or top-level body, a disjunction of two plain booleans must
                // be able to reject: return it as a boolean finalCondp. The merge vertex
                // below drops the attempt silently when both operands are false.
                Ast_گره_عبارت* ثابت orCondp
                    = جدید AstLogOr{flp, lhs.finalCondp->clone_درخت_Pure(نادرست),
                                   rhs.finalCondp->clone_درخت_Pure(نادرست)};
                freeUnlinkedCondp(lhs.finalCondp);
                freeUnlinkedCondp(rhs.finalCondp);
                بازگشت {entryVtxp, orCondp, {}};
            }
        }
        SvaStateVertex* ثابت mergeVtxp = scopedCreateVertex();
        اگر (booleanOnly) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsp,
                        "Single-cycle SOr operands must have finalCondp");
            guardedLink(entryVtxp, mergeVtxp,
                        جدید AstLogOr{flp, sampled(lhs.finalCondp->clone_درخت_Pure(نادرست)),
                                     sampled(rhs.finalCondp->clone_درخت_Pure(نادرست))},
                        flp);
        } وگرنه {
            اگر (lhs.finalCondp) {
                guardedLink(lhs.termVertexp, mergeVtxp,
                            sampled(lhs.finalCondp->clone_درخت_Pure(نادرست)), flp);
            } وگرنه {
                guardedLink(lhs.termVertexp, mergeVtxp, flp);
            }
            اگر (rhs.finalCondp) {
                guardedLink(rhs.termVertexp, mergeVtxp,
                            sampled(rhs.finalCondp->clone_درخت_Pure(نادرست)), flp);
            } وگرنه {
                guardedLink(rhs.termVertexp, mergeVtxp, flp);
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        بازگشت {mergeVtxp, nullptr, {}};
    }

    // Build done-latch combiner for SAnd/SIntersect (IEEE 1800-2023 16.9.5).
    BuildResult buildAndCombiner(Ast_گره_عبارت* lhsExprp, Ast_گره_عبارت* rhsExprp,
                                 SvaStateVertex* entryVtxp, File_سطر* flp) {
        // Snapshot-restore scope so LHS liveness does not leak into RHS.
        ثابت منطقی savedScope = m_inUnboundedScope;
        ثابت BuildResult lhs = build_عبارت(lhsExprp, entryVtxp);
        ثابت منطقی lhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope;
        ثابت BuildResult rhs = build_عبارت(rhsExprp, entryVtxp);
        ثابت منطقی rhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope || lhsScope || rhsScope;
        اگر (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            بازگشت BuildResult::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP

        // Single-cycle operands: use boolean AND (done-latch would fire across cycles).
        // If both operands stayed at entry, they must be boolean leaves which
        // buildExpr returns with finalCondp=nodep (non-null).
        اگر (lhs.termVertexp == entryVtxp && rhs.termVertexp == entryVtxp) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsExprp,
                        "Single-cycle SAnd operands must have finalCondp");
            Ast_گره_عبارت* ثابت condp = جدید AstLogAnd{flp, lhs.finalCondp->clone_درخت_Pure(نادرست),
                                                     rhs.finalCondp->clone_درخت_Pure(نادرست)};
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            بازگشت {entryVtxp, condp, {}};
        }
        // Range-delay mid-window sources in either sub-branch would need
        // to be folded into the latch's match-now signal, which the
        // current combiner does not support.
        اگر (!lhs.midSources.empty() || !rhs.midSources.empty()) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: ranged cycle delay in an operand of property 'and'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            بازگشت BuildResult::failWithError();
        }
        SvaStateVertex* ثابت combVtxp = scopedCreateVertex();
        combVtxp->m_isAndCombiner = درست;
        combVtxp->m_andLhsTermp = lhs.termVertexp;
        combVtxp->m_andRhsTermp = rhs.termVertexp;
        اگر (lhs.finalCondp) combVtxp->m_andLhsCondp = lhs.finalCondp->clone_درخت_Pure(نادرست);
        اگر (rhs.finalCondp) combVtxp->m_andRhsCondp = rhs.finalCondp->clone_درخت_Pure(نادرست);
        اگر (lhs.termVertexp->m_isUnbounded || rhs.termVertexp->m_isUnbounded) {
            combVtxp->m_isUnbounded = درست;
        }
        // Wire terminal-boolean rejects to a dedicated sink so each side can fail
        // the AND independently. Skip for liveness or single-cycle operands
        // (single-cycle termVertexp == entryVtxp would fire every cycle).
        اگر (!combVtxp->m_isUnbounded) {
            منطقی needSink = نادرست;
            ثابت منطقی lhsMultiCycle = (lhs.termVertexp != entryVtxp);
            ثابت منطقی rhsMultiCycle = (rhs.termVertexp != entryVtxp);
            اگر (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                needSink = درست;
            }
            اگر (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                needSink = درست;
            }
            اگر (needSink) {
                SvaStateVertex* ثابت sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = درست;
                اگر (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* ثابت ep = m_graph.addLink(
                        lhs.termVertexp, sinkVtxp, sampled(lhs.finalCondp->clone_درخت_Pure(نادرست)));
                    ep->m_rejectOnFail = درست;
                }
                اگر (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* ثابت ep = m_graph.addLink(
                        rhs.termVertexp, sinkVtxp, sampled(rhs.finalCondp->clone_درخت_Pure(نادرست)));
                    ep->m_rejectOnFail = درست;
                }
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        بازگشت {combVtxp, nullptr, {}};
    }

    // Lower `seq1 within seq2` (IEEE 1800-2023 16.9.10) as:
    //   (OR_{i in 0..slack} 1 ##i seq1 ##(slack-i) 1) intersect seq2
    // Both operands must have fixed length (no ranged cycle delays).
    // The OR lives inside a single SIntersect so one AndCombiner done-latch
    // serves all offsets; lifting it out would double-count matches where
    // multiple offsets accept on the same seq2 end cycle.
    BuildResult buildSWithin(AstSWithin* nodep, SvaStateVertex* entryVtxp,
                             منطقی is_بالاترین_Level_Step = نادرست) {
        ثابت صحیح innerLen = fixedLength(nodep->lhsp());
        ثابت صحیح outerLen = fixedLength(nodep->rhsp());
        اگر (innerLen < 0 || outerLen < 0) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: within with ranged cycle-delay operand");
            بازگشت BuildResult::failWithError();
        }
        اگر (innerLen > outerLen) {
            بازگشت buildNeverMatchIntersect(
                nodep, entryVtxp, is_بالاترین_Level_Step,
                "the inner sequence is longer than the outer sequence");
        }
        File_سطر* ثابت flp = nodep->fileline();
        ثابت صحیح slack = outerLen - innerLen;
        Ast_گره_عبارت* innerOrp = nullptr;
        برای (صحیح i = 0; i <= slack; ++i) {
            ثابت صحیح postPad = slack - i;
            Ast_گره_عبارت* branchp = nodep->lhsp()->clone_درخت_Pure(نادرست);
            اگر (i > 0) {
                AstConst* ثابت prePadp = جدید AstConst{flp, AstConst::BitTrue{}};
                AstDelay* ثابت delayp
                    = جدید AstDelay{flp, جدید AstConst{flp, static_cast<uint32_t>(i)}, درست};
                AstSExpr* ثابت wrapped = جدید AstSExpr{flp, prePadp, delayp, branchp};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            اگر (postPad > 0) {
                AstConst* ثابت postTruep = جدید AstConst{flp, AstConst::BitTrue{}};
                AstDelay* ثابت delayp
                    = جدید AstDelay{flp, جدید AstConst{flp, static_cast<uint32_t>(postPad)}, درست};
                AstSExpr* ثابت wrapped = جدید AstSExpr{flp, branchp, delayp, postTruep};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            innerOrp = innerOrp ? static_cast<Ast_گره_عبارت*>(جدید AstSOr{flp, innerOrp, branchp})
                                : branchp;
        }
        Ast_گره_عبارت* ثابت outerClonep = nodep->rhsp()->clone_درخت_Pure(نادرست);
        Ast_گره_عبارت* ثابت combinedp = جدید AstSIntersect{flp, innerOrp, outerClonep};
        BuildResult result = build_عبارت(combinedp, entryVtxp, is_بالاترین_Level_Step);
        VL_DO_DANGLING(combinedp->delete_درخت(), combinedp);
        // When both operands are plain booleans, buildAndCombiner returns a
        // freshly-allocated AstAnd as finalCondp with no parent. Callers up
        // the chain clone-and-discard finalCondp, so leave it parent-less and
        // it leaks; anchor it in the graph now via an edge.
        اگر (result.valid() && result.finalCondp && !result.finalCondp->backp()) {
            SvaStateVertex* ثابت wrapVtxp = scopedCreateVertex();
            guardedLink(result.termVertexp, wrapVtxp, sampled(result.finalCondp), flp);
            result = {wrapVtxp, nullptr, result.midSources};
        }
        بازگشت result;
    }

    // Collect the boolean leaf checks of a fixed-length sequence keyed by their
    // clock offset from the start. Returns false for anything other than nested
    // AstSExpr with fixed cycle delays over boolean leaves (e.g. throughout).
    ایستا منطقی flattenFixedSeq(Ast_گره_عبارت* nodep, صحیح baseOffset,
                                std::map<صحیح, std::vector<Ast_گره_عبارت*>>& out) {
        اگر (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
            اگر (!delayp || !delayp->isCycleDelay() || delayp->isUnbounded()) بازگشت نادرست;
            ثابت unsigned delayCycles = getConstUInt(delayp->lhsp());
            اگر (delayp->isRangeDelay() && getConstUInt(delayp->rhsp()) != delayCycles)
                بازگشت نادرست;
            صحیح preLen = 0;
            اگر (Ast_گره_عبارت* ثابت prep = sexprp->preExprp()) {
                اگر (!flattenFixedSeq(prep, baseOffset, out)) بازگشت نادرست;
                preLen = fixedLength(prep);
                اگر (preLen < 0) بازگشت نادرست;
            }
            بازگشت flattenFixedSeq(sexprp->exprp(), baseOffset + preLen + delayCycles, out);
        }
        اگر (nodep->isMultiCycleSva()) بازگشت نادرست;
        out[baseOffset].افزودن_back(nodep);
        بازگشت درست;
    }

    // Conjoin two equal-length fixed sequences into one: at each clock offset
    // AND the boolean checks of both operands (IEEE 1800-2023 16.9.6 -- both
    // operands match the same window). Returns null if either operand is not a
    // plain fixed sequence of boolean leaves.
    ایستا Ast_گره_عبارت* conjoinFixedSeqs(Ast_گره_عبارت* lhsp, Ast_گره_عبارت* rhsp, File_سطر* flp) {
        std::map<صحیح, std::vector<Ast_گره_عبارت*>> checks;
        اگر (!flattenFixedSeq(lhsp, 0, checks) || !flattenFixedSeq(rhsp, 0, checks)) بازگشت nullptr;
        اگر (checks.empty()) بازگشت nullptr;
        Ast_گره_عبارت* resultp = nullptr;
        صحیح prevOffset = 0;
        برای (ثابت auto& offsetChecks : checks) {
            ثابت صحیح offset = offsetChecks.first;
            Ast_گره_عبارت* condp = nullptr;
            برای (Ast_گره_عبارت* ثابت leafp : offsetChecks.second) {
                Ast_گره_عبارت* ثابت clonep = leafp->clone_درخت_Pure(نادرست);
                اگر (!condp) {
                    condp = clonep;
                } وگرنه {
                    condp = جدید AstLogAnd{flp, condp, clonep};
                    condp->dtypeSetBit();
                }
            }
            اگر (!resultp) {
                اگر (offset > 0) {
                    AstDelay* ثابت delayp = جدید AstDelay{
                        flp, جدید AstConst{flp, static_cast<uint32_t>(offset)}, /*isCycle=*/درست};
                    resultp
                        = جدید AstSExpr{flp, جدید AstConst{flp, AstConst::BitTrue{}}, delayp, condp};
                    resultp->dtypeSetBit();
                } وگرنه {
                    resultp = condp;
                }
            } وگرنه {
                AstDelay* ثابت delayp = جدید AstDelay{
                    flp, جدید AstConst{flp, static_cast<uint32_t>(offset - prevOffset)},
                    /*isCycle=*/درست};
                resultp = جدید AstSExpr{flp, resultp, delayp, condp};
                resultp->dtypeSetBit();
            }
            prevOffset = offset;
        }
        بازگشت resultp;
    }

    // `seq` is a simple ranged sequence `start ##[m:n] end` (start/end boolean,
    // start may be absent). Used to collapse a both-variable intersect to one
    // ranged delay.
    ساختار SimpleRanged final {
        منطقی ok = نادرست;
        Ast_گره_عبارت* startp = nullptr;  // may be null (absent start)
        Ast_گره_عبارت* endp = nullptr;
    };
    ایستا SimpleRanged asSimpleRanged(Ast_گره_عبارت* nodep) {
        AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr);
        اگر (!sexprp) بازگشت {};
        AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
        اگر (!delayp || !delayp->isCycleDelay() || !delayp->isRangeDelay() || delayp->isUnbounded())
            بازگشت {};
        اگر (getConstUInt(delayp->lhsp()) == getConstUInt(delayp->rhsp())) بازگشت {};
        Ast_گره_عبارت* ثابت prep = sexprp->preExprp();
        اگر (prep && fixedLength(prep) != 0) بازگشت {};
        اگر (fixedLength(sexprp->exprp()) != 0) بازگشت {};
        بازگشت {درست, prep, sexprp->exprp()};
    }

    // Build the NFA for a synthesized intersect lowering tree, then free it.
    // buildExpr returns the terminal condition (finalCondp) by reference into the
    // tree; detach a clone so the tree can be freed here. The graph already holds
    // clones/hoists of every edge condition, so nothing else dangles.
    BuildResult build_From_Lowering_درخت(Ast_گره_عبارت* treep, SvaStateVertex* entryVtxp,
                                      منطقی is_بالاترین_Level_Step) {
        BuildResult result = build_عبارت(treep, entryVtxp, is_بالاترین_Level_Step);
        اگر (result.valid() && result.finalCondp) {
            result.finalCondp = result.finalCondp->clone_درخت_Pure(نادرست);
        }
        VL_DO_DANGLING(treep->delete_درخت(), treep);
        بازگشت result;
    }

    // Empty common-length intersection -- unequal fixed lengths, or disjoint
    // ranged lengths. IEEE 1800-2023 16.9.6 requires both operands to match
    // over a window of the same length, so with no common length the intersect
    // simply never matches. This is legal (matching nothing), not an error, so
    // lower to a constant false rather than rejecting legal code.
    BuildResult buildNeverMatchIntersect(Ast_گره_عبارت* nodep, SvaStateVertex* entryVtxp,
                                         منطقی is_بالاترین_Level_Step, ثابت نویسه* reason) {
        nodep->v3warn(NEVERMATCH, "Sequence can never match because " << reason << ".");
        Ast_گره_عبارت* ثابت falsep = جدید AstConst{nodep->fileline(), AstConst::BitFalse{}};
        بازگشت build_From_Lowering_درخت(falsep, entryVtxp, is_بالاترین_Level_Step);
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
                                     منطقی is_بالاترین_Level_Step) {
        ثابت std::pair<صحیح, صحیح> lhsRange = lengthRange(nodep->lhsp());
        ثابت std::pair<صحیح, صحیح> rhsRange = lengthRange(nodep->rhsp());
        اگر (lhsRange.first < 0 || rhsRange.first < 0) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect with this variable-length operand");
            بازگشت BuildResult::failWithError();
        }
        ثابت صحیح lo = std::max(lhsRange.first, rhsRange.first);
        ثابت صحیح hi = std::min(lhsRange.second, rhsRange.second);
        اگر (lo > hi) {
            // Disjoint length ranges share no common length -> never matches.
            بازگشت buildNeverMatchIntersect(nodep, entryVtxp, is_بالاترین_Level_Step,
                                            "intersect operands have no common length");
        }
        File_سطر* ثابت flp = nodep->fileline();
        اگر (lo == hi) {
            Ast_گره_عبارت* ثابت lp = realizeAtLength(nodep->lhsp(), lo, lhsRange.first);
            Ast_گره_عبارت* ثابت rp = realizeAtLength(nodep->rhsp(), lo, rhsRange.first);
            Ast_گره_عبارت* ثابت conjp = conjoinFixedSeqs(lp, rp, flp);
            VL_DO_DANGLING(lp->delete_درخت(), lp);
            VL_DO_DANGLING(rp->delete_درخت(), rp);
            اگر (!conjp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: intersect operand is not a plain boolean sequence");
                بازگشت BuildResult::failWithError();
            }
            بازگشت build_From_Lowering_درخت(conjp, entryVtxp, is_بالاترین_Level_Step);
        }
        ثابت SimpleRanged sl = asSimpleRanged(nodep->lhsp());
        ثابت SimpleRanged sr = asSimpleRanged(nodep->rhsp());
        اگر (!sl.ok || !sr.ok) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect of two sequences that each vary in length over a"
                          " range with internal structure");
            بازگشت BuildResult::failWithError();
        }
        ثابت auto andBool = [&](Ast_گره_عبارت* ap, Ast_گره_عبارت* bp) -> Ast_گره_عبارت* {
            Ast_گره_عبارت* ثابت aClonep
                = ap ? ap->clone_درخت_Pure(نادرست) : جدید AstConst{flp, AstConst::BitTrue{}};
            Ast_گره_عبارت* ثابت bClonep
                = bp ? bp->clone_درخت_Pure(نادرست) : جدید AstConst{flp, AstConst::BitTrue{}};
            AstLogAnd* ثابت andp = جدید AstLogAnd{flp, aClonep, bClonep};
            andp->dtypeSetBit();
            بازگشت andp;
        };
        AstDelay* ثابت delayp = جدید AstDelay{flp, جدید AstConst{flp, static_cast<uint32_t>(lo)},
                                              /*isCycle=*/درست};
        delayp->rhsp(جدید AstConst{flp, static_cast<uint32_t>(hi)});
        AstSExpr* ثابت reducedp
            = جدید AstSExpr{flp, andBool(sl.startp, sr.startp), delayp, andBool(sl.endp, sr.endp)};
        reducedp->dtypeSetBit();
        بازگشت build_From_Lowering_درخت(reducedp, entryVtxp, is_بالاترین_Level_Step);
    }

    BuildResult buildThroughout(AstSThroughout* nodep, SvaStateVertex* entryVtxp,
                                منطقی is_بالاترین_Level_Step = نادرست) {
        // Mark entryVtxp so "cond false at tick 0" is detected as throughout-drop.
        entryVtxp->m_throughoutConds.افزودن_back(nodep->lhsp()->clone_درخت_Pure(نادرست));
        m_temporal_Guard_پشته.افزودن_back(nodep->lhsp());
        ثابت BuildResult result = build_عبارت(nodep->rhsp(), entryVtxp, is_بالاترین_Level_Step);
        m_temporal_Guard_پشته.برداشت_back();
        بازگشت result;
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
    BuildResult buildUntil(AstUntil* nodep, SvaStateVertex* entryVtxp, منطقی is_بالاترین_Level_Step) {
        File_سطر* ثابت flp = nodep->fileline();
        اگر (!is_بالاترین_Level_Step) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            بازگشت BuildResult::failWithError();
        }
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp();
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp();
        ثابت auto hasSeq = [](ثابت Ast_گره_عبارت* ep) {
            بازگشت ep->exists([](ثابت Ast_گره_عبارت* np) { بازگشت np->isMultiCycleSva(); });
        };
        اگر (hasSeq(lhsp) || hasSeq(rhsp)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            بازگشت BuildResult::failWithError();
        }

        ثابت منطقی ov = nodep->isOverlapping();
        Ast_گره_عبارت* ثابت lhsBitp = nodep->lhsp();
        Ast_گره_عبارت* ثابت rhsBitp = nodep->rhsp();
        // p hoist count: continue, require (ov: 1 use; nov: 1 use). At least 2 uses.
        AstVar* ثابت pHoistp = tryHoistSampled(lhsBitp, flp, 2);
        // q hoist count: continue (1) + require nov (1) = 2; ov: continue only (1).
        AstVar* ثابت qHoistp = ov ? nullptr : tryHoistSampled(rhsBitp, flp, 2);

        SvaStateVertex* ثابت waitCp = scopedCreateVertex();
        SvaStateVertex* ثابت waitRp = scopedCreateVertex();
        waitCp->m_isUnbounded = درست;
        waitRp->m_strongPending = nodep->isStrong() && !m_isCover;

        // Entry and back-loop Links carry no condition; throughout-folding still applies.
        guardedLink(entryVtxp, waitCp, flp);
        guardedLink(waitRp, waitCp, flp);

        // Continue clocked edge: p && !q advances to next-cycle wait.
        Ast_گره_عبارت* ثابت contCondp
            = جدید AstLogAnd{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                            جدید AstLogNot{flp, sampledRefOrClone(qHoistp, rhsBitp, flp)}};
        guardedEdge(waitCp, waitRp, contCondp, flp);

        // Reject sink: fires when require-condition is false.
        SvaStateVertex* ثابت sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = درست;
        Ast_گره_عبارت* requireCondp;
        اگر (ov) {
            requireCondp = sampledRefOrClone(pHoistp, lhsBitp, flp);
        } وگرنه {
            requireCondp = جدید AstLogOr{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                                        sampledRefOrClone(qHoistp, rhsBitp, flp)};
        }
        SvaTransEdge* ثابت rejEdgep = m_graph.addLink(waitCp, sinkVtxp, requireCondp);
        rejEdgep->m_rejectOnFail = درست;

        // Accept condition rides via finalCondp; assembleResult $sampled-wraps it.
        Ast_گره_عبارت* acceptCondp;
        اگر (ov) {
            acceptCondp
                = جدید AstLogAnd{flp, lhsBitp->clone_درخت_Pure(نادرست), rhsBitp->clone_درخت_Pure(نادرست)};
        } وگرنه {
            acceptCondp = rhsBitp->clone_درخت_Pure(نادرست);
        }
        بازگشت {waitCp, acceptCondp, {}};
    }

    // IEEE 1800-2023 16.12.14 property abort operators. Sync and async share
    // the same NFA encoding: AstSampled already gives matured values at every
    // maturing clocking event, and async firing "between clocks" is not
    // observable in a cycle-based model. VAbortKind selects accept vs reject
    // verdict (sync vs async only changes the user-visible spelling).

    // Build `condp && !outer_1 && !outer_2 ...` (unsampled).
    Ast_گره_عبارت* abort_Fire_عبارت(Ast_گره_عبارت* condp, File_سطر* flp) {
        Ast_گره_عبارت* resultp = condp->clone_درخت_Pure(نادرست);
        برای (Ast_گره_عبارت* ثابت op : m_outer_Abort_پشته)
            resultp = جدید AstLogAnd{flp, resultp, جدید AstLogNot{flp, op->clone_درخت_Pure(نادرست)}};
        بازگشت resultp;
    }

    // True when unguarded ring-wide rejection or a same-tick Link chain already accounts the
    // attempt: a required-step Link covers both outcomes; followed-by pairs both edges.
    ایستا منطقی chainAccountsSource(ثابت SvaStateVertex* srcp,
                                    ثابت std::unordered_set<ثابت V3GraphEdge*>& preEdges) {
        اگر (srcp->m_delayRingSize && srcp->m_throughoutConds.empty()) بازگشت درست;
        برای (ثابت V3GraphEdge& edger : srcp->inEdges()) {
            اگر (preEdges.شمار(&edger)) ادامه;
            ثابت SvaTransEdge& tedger = static_cast<ثابت SvaTransEdge&>(edger);
            اگر (tedger.m_consumesCycle) ادامه;
            ثابت auto* ثابت fromp = static_cast<ثابت SvaStateVertex*>(tedger.fromVtxp());
            اگر (fromp->m_abortRejectp) بازگشت درست;
        }
        منطقی plainNonSink = نادرست;
        منطقی markedSink = نادرست;
        برای (ثابت V3GraphEdge& edger : srcp->outEdges()) {
            اگر (preEdges.شمار(&edger)) ادامه;
            ثابت SvaTransEdge& tedger = static_cast<ثابت SvaTransEdge&>(edger);
            اگر (tedger.m_consumesCycle) ادامه;
            ثابت منطقی sink = static_cast<ثابت SvaStateVertex*>(tedger.toVtxp())->m_isRejectSink;
            اگر (tedger.m_rejectOnFail) {
                اگر (!sink) بازگشت درست;
                markedSink = درست;
            } وگرنه اگر (!sink) {
                اگر (srcp->m_abortRejectp) بازگشت درست;
                plainNonSink = درست;
            }
        }
        بازگشت plainNonSink && markedSink;
    }

    // Reject edge: fires when the source is live and the abort samples true.
    تهی addAbortRejectEdge(SvaStateVertex* srcp, SvaStateVertex* sinkp, Ast_گره_عبارت* condp,
                            File_سطر* flp) {
        Ast_گره_عبارت* ثابت notFirep = جدید AstLogNot{flp, sampled(abort_Fire_عبارت(condp, flp))};
        m_graph.addLink(srcp, sinkp, notFirep)->m_rejectOnFail = درست;
        بازگشت;
    }

    // On the fire tick: kill body threads; accept kinds also forgive step misses.
    تهی gateBodyEdgesOnAbort(ثابت std::unordered_set<ثابت V3GraphEdge*>& preEdges,
                              Ast_گره_عبارت* condp, VAbortKind kind, File_سطر* flp) {
        برای (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            برای (V3GraphEdge& edger : vtxr.outEdges()) {
                اگر (preEdges.شمار(&edger)) ادامه;
                SvaTransEdge* ثابت tedgep = static_cast<SvaTransEdge*>(&edger);
                اگر (tedgep->m_rejectOnFail) {
                    اگر (!kind.isAccept()) ادامه;
                    Ast_گره_عبارت* ثابت firep = sampled(abort_Fire_عبارت(condp, flp));
                    tedgep->m_condp
                        = tedgep->m_condp ? جدید AstLogOr{flp, tedgep->m_condp, firep} : firep;
                } وگرنه اگر (tedgep->m_consumesCycle) {
                    Ast_گره_عبارت* ثابت notFirep
                        = جدید AstLogNot{flp, sampled(abort_Fire_عبارت(condp, flp))};
                    tedgep->m_condp = tedgep->m_condp
                                          ? جدید AstLogAnd{flp, tedgep->m_condp, notFirep}
                                          : notFirep;
                }
            }
        }
    }

    BuildResult buildAbortOn(Ast_گره_عبارت* condp, Ast_گره_عبارت* bodyp, SvaStateVertex* entryVtxp,
                             VAbortKind kind, File_سطر* flp, منطقی is_بالاترین_Level_Step) {
        // Snapshot pre-body vertices/edges so post-build diff yields the body's sub-NFA.
        std::unordered_set<ثابت V3GraphVertex*> preExisting;
        std::unordered_set<ثابت V3GraphEdge*> preEdges;
        برای (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            preExisting.درج(&vtxr);
            برای (V3GraphEdge& edger : vtxr.outEdges()) preEdges.درج(&edger);
        }

        m_outer_Abort_پشته.افزودن_back(condp);
        ثابت BuildResult bodyResult = build_عبارت(bodyp, entryVtxp, is_بالاترین_Level_Step);
        m_outer_Abort_پشته.برداشت_back();
        اگر (!bodyResult.valid()) بازگشت bodyResult;

        gateBodyEdgesOnAbort(preEdges, condp, kind, flp);

        // Live-thread sources for the abort edge: entry + new body vertices,
        // minus reject sinks (they carry reject fuel, not live-thread fuel).
        std::vector<SvaStateVertex*> abortSources;
        abortSources.افزودن_back(entryVtxp);
        برای (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            اگر (preExisting.شمار(&vtxr)) ادامه;
            auto* ثابت sp = static_cast<SvaStateVertex*>(&vtxr);
            اگر (sp->m_delayRingSize || !sp->m_throughoutConds.empty()) {
                Ast_گره_عبارت* ثابت firep = abort_Fire_عبارت(condp, flp);
                sp->m_abortClearp
                    = sp->m_abortClearp ? جدید AstLogOr{flp, sp->m_abortClearp, firep} : firep;
            }
            اگر (!kind.isAccept()
                && ((sp->m_delayRingSize && sp->m_throughoutConds.empty())
                    || sp->m_replayAbortReject)) {
                Ast_گره_عبارت* ثابت firep = abort_Fire_عبارت(condp, flp);
                sp->m_abortRejectp
                    = sp->m_abortRejectp ? جدید AstLogOr{flp, sp->m_abortRejectp, firep} : firep;
            }
            اگر (sp->m_isRejectSink) ادامه;
            abortSources.افزودن_back(sp);
        }

        auto sampledAbortFire = [&]() -> Ast_گره_عبارت* {
            Ast_گره_عبارت* ثابت عبارت = abort_Fire_عبارت(condp, flp);
            بازگشت sampled(عبارت);
        };

        اگر (kind.isAccept()) {
            // Match-only sink fed by $sampled(abort-fire) from every live source;
            // registered as midSource so it never contributes a reject.
            SvaStateVertex* ثابت acceptSinkp = scopedCreateVertex();
            برای (SvaStateVertex* ثابت srcp : abortSources)
                guardedLink(srcp, acceptSinkp, sampledAbortFire(), flp);
            std::vector<SvaStateVertex*> midSources = bodyResult.midSources;
            midSources.افزودن_back(acceptSinkp);
            Ast_گره_عبارت* finalCondp = bodyResult.finalCondp;
            اگر (finalCondp) {
                اگر (finalCondp->backp()) finalCondp = finalCondp->clone_درخت_Pure(نادرست);
                finalCondp = جدید AstLogOr{flp, finalCondp, abort_Fire_عبارت(condp, flp)};
            }
            بازگشت {bodyResult.termVertexp, finalCondp, std::move(midSources)};
        }

        // rejectOnFail treats m_condp as the success condition and fires on
        // !condp, so the edge carries !sampledAbortFire().
        SvaStateVertex* ثابت rejectSinkp = m_graph.createStateVertex();
        rejectSinkp->m_isRejectSink = درست;
        برای (SvaStateVertex* ثابت srcp : abortSources)
            اگر (!chainAccountsSource(srcp, preEdges))
                addAbortRejectEdge(srcp, rejectSinkp, condp, flp);
        Ast_گره_عبارت* finalCondp = bodyResult.finalCondp;
        اگر (finalCondp) {
            اگر (finalCondp->backp()) finalCondp = finalCondp->clone_درخت_Pure(نادرست);
            finalCondp
                = جدید AstLogAnd{flp, finalCondp, جدید AstLogNot{flp, abort_Fire_عبارت(condp, flp)}};
        }
        بازگشت {bodyResult.termVertexp, finalCondp, bodyResult.midSources};
    }

عمومی:
    // Free a dropped sub-result condition that is not linked into the AST
    // (abort folds synthesize unparented finalCondp trees).
    ایستا تهی freeUnlinkedCondp(Ast_گره_عبارت* condp) {
        اگر (condp && !condp->backp()) VL_DO_DANGLING(condp->delete_درخت(), condp);
    }

    SvaNfaBuilder(SvaGraph& graph, Ast_گره_Module* modp, V3UniqueNames& propTempNames,
                  منطقی isCoverSeq = نادرست, منطقی isSeqEvent = نادرست, منطقی isCover = نادرست)
        : m_graph{graph}
        , m_modp{modp}
        , m_propTempNames{propTempNames}
        , m_isCover{isCover}
        , m_isCoverSeq{isCoverSeq}
        , m_isSeqEvent{isSeqEvent} {}

    // Reset scope between antecedent and consequent: liveness must not leak.
    // m_outerAbortStack survives: an abort wrapping the implication covers the
    // consequent too (IEEE 1800-2023 16.12.14).
    تهی resetScope() {
        m_inUnboundedScope = نادرست;
        m_temporal_Guard_پشته.clear();
    }

    BuildResult build_عبارت(Ast_گره_عبارت* nodep, SvaStateVertex* entryVtxp,
                          منطقی is_بالاترین_Level_Step = نادرست) {
        VL_RESTORER(m_wantBoolLeaf);
        ثابت منطقی wantBoolLeaf = m_wantBoolLeaf;
        m_wantBoolLeaf = نادرست;
        اگر (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            بازگشت buildSExpr(sexprp, entryVtxp, is_بالاترین_Level_Step);
        }
        اگر (AstSConsRep* ثابت repp = VN_CAST(nodep, SConsRep)) {
            بازگشت buildConsRep(repp, entryVtxp, is_بالاترین_Level_Step);
        }
        اگر (AstPropAlways* ثابت alwaysp = VN_CAST(nodep, PropAlways)) {
            بازگشت buildPropAlways(alwaysp, entryVtxp, is_بالاترین_Level_Step);
        }
        اگر (AstSGotoRep* ثابت repp = VN_CAST(nodep, SGotoRep)) {
            بازگشت buildGotoRep(repp, entryVtxp);
        }
        اگر (AstSThroughout* ثابت throughoutp = VN_CAST(nodep, SThroughout)) {
            بازگشت buildThroughout(throughoutp, entryVtxp, is_بالاترین_Level_Step);
        }
        اگر (AstSOr* ثابت orp = VN_CAST(nodep, SOr)) {
            بازگشت buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        اگر (AstLogOr* ثابت orp = VN_CAST(nodep, LogOr)) {
            بازگشت buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        اگر (AstSAnd* ثابت andp = VN_CAST(nodep, SAnd)) {
            بازگشت buildAndCombiner(andp->lhsp(), andp->rhsp(), entryVtxp, andp->fileline());
        }
        اگر (AstSIntersect* ثابت intp = VN_CAST(nodep, SIntersect)) {
            // IEEE 1800-2023 16.9.6: both operands match over one window with
            // equal start and end (equal length). Lower to a single sequence
            // that conjoins both operands' per-cycle checks -- correct under
            // concurrent attempts, where the done-latch combiner conflates the
            // two operands' start times and over-accepts. The combiner remains
            // only as a fallback for operands that do not flatten.
            ثابت صحیح lhsLen = fixedLength(intp->lhsp());
            ثابت صحیح rhsLen = fixedLength(intp->rhsp());
            اگر (lhsLen >= 0 && rhsLen >= 0) {
                اگر (lhsLen != rhsLen) {
                    // Unequal fixed lengths share no common length -> never matches.
                    بازگشت buildNeverMatchIntersect(intp, entryVtxp, is_بالاترین_Level_Step,
                                                    "intersect operands have no common length");
                }
                اگر (Ast_گره_عبارت* ثابت conjp
                    = conjoinFixedSeqs(intp->lhsp(), intp->rhsp(), intp->fileline())) {
                    بازگشت build_From_Lowering_درخت(conjp, entryVtxp, is_بالاترین_Level_Step);
                }
                بازگشت buildAndCombiner(intp->lhsp(), intp->rhsp(), entryVtxp, intp->fileline());
            }
            بازگشت buildVarLenIntersect(intp, entryVtxp, is_بالاترین_Level_Step);
        }
        اگر (AstSWithin* ثابت withinp = VN_CAST(nodep, SWithin)) {
            بازگشت buildSWithin(withinp, entryVtxp, is_بالاترین_Level_Step);
        }
        اگر (AstAbortOn* ثابت ap = VN_CAST(nodep, AbortOn)) {
            بازگشت buildAbortOn(ap->condp(), ap->propp(), entryVtxp, ap->kind(), ap->fileline(),
                                is_بالاترین_Level_Step);
        }
        اگر (VN_IS(nodep, SNonConsRep)) بازگشت BuildResult::fail();
        اگر (AstImplication* ثابت implp = VN_CAST(nodep, Implication)) {
            بازگشت buildImplicationEdges(implp->lhsp(), implp->rhsp(), entryVtxp,
                                         implp->isOverlapped(), implp->isFollowedBy(),
                                         implp->lhsp(), implp->fileline());
        }
        اگر (AstUntil* ثابت untilp = VN_CAST(nodep, Until)) {
            بازگشت buildUntil(untilp, entryVtxp, is_بالاترین_Level_Step);
        }
        // Boolean leaf (including LogAnd): return as finalCond
        بازگشت {entryVtxp, nodep, {}};
    }

    // Wire an implication / followed-by from `entryVtxp`: builds the antecedent,
    // emits the match-link (and for followed-by the reject-sink edge), inserts a
    // delay vertex for non-overlapped forms, and builds the body. Used both for
    // nested AstImplication in pexpr position and for the top-level assertion
    // antecedent -- `errorNodep` anchors the "unsupported sequence antecedent"
    // error, which differs between the two call sites.
    BuildResult buildImplicationEdges(Ast_گره_عبارت* antExprp, Ast_گره_عبارت* bodyExprp,
                                      SvaStateVertex* entryVtxp, منطقی isOverlapped,
                                      منطقی isFollowedBy, Ast_گره* errorNodep, File_سطر* flp) {
        ثابت BuildResult antResult = build_عبارت(antExprp, entryVtxp);
        اگر (!antResult.valid()) بازگشت antResult;

        // Followed-by requires pure-boolean antecedent for non-vacuous-fail at
        // the attempt-start cycle. IEEE 1800-2023 16.12.9 permits a multi-cycle
        // sequence LHS, so this is an implementation gap rather than illegal SV.
        اگر (isFollowedBy && antResult.termVertexp != entryVtxp) {
            errorNodep->v3warn(E_UNSUPPORTED,
                               "Unsupported: sequence expression as antecedent of followed-by"
                               " (#-# / #=#) (IEEE 1800-2023 16.12.9)");
            بازگشت BuildResult::failWithError();
        }
        UASSERT_OBJ(!isFollowedBy || antResult.finalCondp, errorNodep,
                    "followed-by antecedent terminal at entry must carry finalCondp");

        // Use raw createStateVertex() so trigVtxp starts without liveness --
        // reaching the antecedent terminal is a definitive event.
        SvaStateVertex* ثابت trigVtxp = m_graph.createStateVertex();
        اگر (antResult.finalCondp) {
            m_graph.addLink(antResult.termVertexp, trigVtxp,
                            sampled(antResult.finalCondp->clone_درخت_Pure(نادرست)));
            // Followed-by non-vacuous fail: rejectOnFail fires when the attempt
            // is live (termVtx reachable) and sampled(antecedent) is false.
            اگر (isFollowedBy) {
                SvaStateVertex* ثابت sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = درست;
                SvaTransEdge* ثابت ep
                    = m_graph.addLink(antResult.termVertexp, sinkVtxp,
                                      sampled(antResult.finalCondp->clone_درخت_Pure(نادرست)));
                ep->m_rejectOnFail = درست;
            }
            // finalCondp is cloned into the Sampled nodes; if the original is
            // not parented anywhere in the AST anymore it must be freed here
            // or ASan flags it as a leak (e.g. t_sequence_bool_ops).
            اگر (!antResult.finalCondp->backp()) {
                VL_DO_DANGLING(antResult.finalCondp->delete_درخت(), antResult.finalCondp);
            }
        } وگرنه {
            m_graph.addLink(antResult.termVertexp, trigVtxp);
        }
        resetScope();

        SvaStateVertex* bodyEntryp = trigVtxp;
        اگر (!isOverlapped) {
            SvaStateVertex* ثابت delayVtxp = m_graph.createStateVertex();
            m_graph.addClockedEdge(trigVtxp, delayVtxp);
            bodyEntryp = delayVtxp;
        }
        // An empty antecedent match must not start the consequent, but a zero-minimum
        // repetition's empty match is not handled here; keep the merge vertex for those
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = !isFollowedBy && !hasZeroMinRep(antExprp);
        بازگشت build_عبارت(bodyExprp, bodyEntryp, /*isTopLevelStep=*/درست);
    }

    BuildResult build(Ast_گره_عبارت* exprp) {
        m_graph.m_startVertexp = scopedCreateVertex();
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = درست;
        بازگشت build_عبارت(exprp, m_graph.m_startVertexp, /*isTopLevelStep=*/درست);
    }
};

//######################################################################
// NFA Lowering (converts NFA graph to synthesizable AstAlways blocks)

رده SvaNfaLowering final {
    Ast_گره_Module* ثابت m_modp;  // Module to add state vars and always blocks to
    Ast_گره_DType* ثابت m_u32DTypep;  // Shared unsigned counter dtype
    V3UniqueNames m_names{"__Vnfa"};
    size_t m_statDelayRingEdgeVisits = 0;  // Delay-ring incoming edges visited

    // Per-lowering shared context (passed to phase sub-functions)
    // Per-vertex lowering state is stored in SvaVertexData and accessed via
    // V3GraphVertex::userp() (see SvaStateVertex::datap()).
    ساختار LowerCtx final {
        File_سطر* flp;  // Source location for generated AST
        Ast_Sen_درخت* senTreep;  // Clock sensitivity tree
        Ast_گره_عبارت* disableExprp;  // disable iff expression (may be nullptr)
        Ast_گره_عبارت* matchCondp;  // Final boolean match condition (may be nullptr)
        VAssertType assertType;  // Assertion type for control tasks
        VAssertDirectiveType directiveType;  // Directive type for control tasks
        AstVar* killVarp;  // Last observed kill generation
        SvaGraph& graph;  // NFA graph
    };

    // Build a match-now expression: stateSig[i] && $sampled(condp)
    ایستا Ast_گره_عبارت* buildMatchNow(File_سطر* flp, Ast_گره_عبارت* stateExprp, Ast_گره_عبارت* condp) {
        Ast_گره_عبارت* ثابت statep = stateExprp->clone_درخت_Pure(نادرست);
        اگر (!condp) بازگشت statep;
        بازگشت جدید AstLogAnd{flp, statep, sampled(condp->clone_درخت_Pure(نادرست))};
    }
    ایستا Ast_گره_عبارت* andCond(File_سطر* flp, Ast_گره_عبارت* exprp, Ast_گره_عبارت* condp) {
        اگر (!condp) بازگشت exprp;
        بازگشت جدید AstLogAnd{flp, exprp, condp->clone_درخت_Pure(نادرست)};
    }
    // bp is always non-null; only ap can be null (serving as accumulator).
    ایستا Ast_گره_عبارت* orExprs(File_سطر* flp, Ast_گره_عبارت* ap, Ast_گره_عبارت* bp) {
        اگر (!ap) بازگشت bp;
        بازگشت جدید AstLogOr{flp, ap, bp};
    }
    ایستا Ast_گره_عبارت* addCountp(File_سطر* ثابت flp, Ast_گره_عبارت* ثابت totalCountp,
                                  Ast_گره_عبارت* ثابت contributionp,
                                  Ast_گره_عبارت* ثابت enablep = nullptr) {
        // contribution = enable ? contribution : 0;
        Ast_گره_عبارت* ثابت enabledContributionp
            = enablep ? جدید AstCond{flp, enablep, contributionp,
                                    newTypedConstp(flp, contributionp->dtypep(), 0)}
                      : contributionp;
        اگر (!totalCountp) بازگشت enabledContributionp;
        بازگشت جدید AstAdd{flp, totalCountp, enabledContributionp};
    }
    ایستا Ast_گره_عبارت* gateCheckedCountp(File_سطر* ثابت flp, Ast_گره_عبارت* ثابت countp,
                                          Ast_گره_عبارت*& overflowp, Ast_گره_عبارت* ثابت enablep) {
        اگر (overflowp) {
            overflowp = جدید AstLogAnd{flp, enablep->clone_درخت_Pure(نادرست), overflowp};
        }
        بازگشت addCountp(flp, nullptr, countp, enablep);
    }
    ایستا Ast_گره_عبارت* addCheckedCountp(File_سطر* ثابت flp, Ast_گره_عبارت* ثابت totalCountp,
                                         Ast_گره_عبارت* ثابت contributionp, Ast_گره_عبارت*& overflowp,
                                         Ast_گره_عبارت* ثابت contributionOverflowp = nullptr) {
        اگر (contributionOverflowp) { overflowp = orExprs(flp, overflowp, contributionOverflowp); }
        اگر (!totalCountp) بازگشت contributionp;
        Ast_گره_عبارت* ثابت oldTotalp = totalCountp->clone_درخت_Pure(نادرست);
        Ast_گره_عبارت* ثابت resultp = جدید AstAdd{flp, totalCountp, contributionp};
        overflowp
            = orExprs(flp, overflowp, جدید AstLt{flp, resultp->clone_درخت_Pure(نادرست), oldTotalp});
        بازگشت resultp;
    }
    AstIf* new_شمار_Overflow_Warning(File_سطر* ثابت flp, Ast_گره_عبارت* ثابت condp) ثابت {
        AstDisplay* ثابت dispp = جدید AstDisplay{
            flp, VDisplayType::DT_WARNING,
            "Cover sequence match count overflowed 32-bit storage; results may be incorrect.",
            nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstIf* ثابت ifp = جدید AstIf{flp, condp, dispp};
        ifp->branchPred(VBranchPred::BP_UNLIKELY);
        بازگشت ifp;
    }
    ایستا Ast_گره_عبارت* killActive(LowerCtx& c) {
        بازگشت جدید AstNeq{c.flp, جدید AstVarRef{c.flp, c.killVarp, VAccess::READ},
                          assertKillGet(c.flp, c.assertType, c.directiveType)};
    }
    ایستا Ast_گره_عبارت* notKillActive(LowerCtx& c) { بازگشت جدید AstLogNot{c.flp, killActive(c)}; }
    ایستا Ast_گره_عبارت* gateNotKill(LowerCtx& c, Ast_گره_عبارت* exprp) {
        اگر (!exprp) بازگشت nullptr;
        بازگشت جدید AstLogAnd{c.flp, exprp, notKillActive(c)};
    }
    ایستا Ast_گره_عبارت* nextRingIndex(File_سطر* flp, Ast_گره_عبارت* idxExprp, uint32_t size) {
        ثابت auto u32Const = [flp](uint32_t مقدار) {
            بازگشت جدید AstConst{flp, AstConst::Widthed_مقدار{}, 32, مقدار};
        };
        UASSERT_OBJ(size > 0, idxExprp, "Ring size must be positive");
        اگر (size == 1) {
            VL_DO_DANGLING(idxExprp->delete_درخت(), idxExprp);
            بازگشت u32Const(0);
        }
        // idx == size - 1 ? 0 : idx + 1
        AstAdd* ثابت addp = جدید AstAdd{flp, idxExprp->clone_درخت_Pure(نادرست), u32Const(1)};
        addp->dtypeFrom(idxExprp);
        AstCond* ثابت condp
            = جدید AstCond{flp, جدید AstEq{flp, idxExprp, u32Const(size - 1)}, u32Const(0), addp};
        condp->dtypeFrom(idxExprp);
        بازگشت condp;
    }
    ایستا Ast_گره_عبارت* delayRingSlot(File_سطر* flp, SvaStateVertex* vtxp, Ast_گره_عبارت* idxExprp,
                                      VAccess access = VAccess::READ) {
        // ring[idx]
        AstVar* ثابت ringp = vtxp->datap()->delayRingVarp;
        AstVarRef* ثابت refp = جدید AstVarRef{flp, ringp, access};
        اگر (vtxp->datap()->delayRingCounts) بازگشت جدید AstArraySel{flp, refp, idxExprp};
        بازگشت جدید AstSel{flp, refp, idxExprp, 1};
    }
    ایستا Ast_گره_عبارت* delay_Ring_Head_شمار(File_سطر* flp, SvaStateVertex* vtxp) {
        Ast_گره_عبارت* ثابت headp
            = جدید AstVarRef{flp, vtxp->datap()->delayRingHeadVarp, VAccess::READ};
        اگر (vtxp->datap()->delayRingCounts) بازگشت headp;
        بازگشت جدید AstExtend{flp, headp, vtxp->datap()->delay_Ring_Live_شمار_Varp->dtypep()->width()};
    }
    ایستا Ast_گره_عبارت* delayRingAtLastIndex(File_سطر* ثابت flp, AstVar* ثابت idxp,
                                             ثابت uint32_t size) {
        بازگشت جدید AstEq{flp, جدید AstVarRef{flp, idxp, VAccess::READ},
                         جدید AstConst{flp, AstConst::Widthed_مقدار{}, 32, size - 1}};
    }
    ایستا Ast_گره_عبارت* delay_Ring_Output_شمار(File_سطر* ثابت flp, SvaStateVertex* ثابت vtxp) {
        AstVar* ثابت idxp = vtxp->datap()->delayRingIdxVarp;
        ثابت uint32_t size = vtxp->m_delayRingSize;
        Ast_گره_عبارت* outgoingValidp
            = جدید AstVarRef{flp, vtxp->datap()->delayRingWrappedVarp, VAccess::READ};
        اگر (!vtxp->m_isFixedDelayRing) {
            outgoingValidp
                = جدید AstLogOr{flp, outgoingValidp, delayRingAtLastIndex(flp, idxp, size)};
        }
        AstVar* ثابت live_شمار_Varp = vtxp->datap()->delay_Ring_Live_شمار_Varp;
        بازگشت جدید AstCond{flp, outgoingValidp, delay_Ring_Head_شمار(flp, vtxp),
                           newTypedConstp(flp, live_شمار_Varp->dtypep(), 0)};
    }
    ایستا Ast_گره_عبارت* delayRingHasLiveThreadsp(File_سطر* ثابت flp,
                                                 AstVar* ثابت live_شمار_Varp) {
        // active = live_count != 0;
        بازگشت جدید AstNeq{flp, جدید AstVarRef{flp, live_شمار_Varp, VAccess::READ},
                          newTypedConstp(flp, live_شمار_Varp->dtypep(), 0)};
    }

    // Phase 3 output signals
    ساختار SignalSet final {
        Ast_گره_عبارت* terminalActivep = nullptr;  // OR of all successful terminal matches
        Ast_گره_عبارت* matchCountp = nullptr;  // NFA paths completing the sequence this tick
        Ast_گره_عبارت* match_شمار_Overflowp = nullptr;  // Match count overflow this tick
        Ast_گره_عبارت* rejectBasep = nullptr;  // Reject when a terminal match fails
        Ast_گره_عبارت* requiredStepRejectp = nullptr;  // Per-source reject from rejectOnFail Links
        Ast_گره_عبارت* throughoutRejectp = nullptr;  // Reject when a throughout guard drops
        Ast_گره_عبارت* threadFailCountp = nullptr;  // Number of threads rejected on this tick
    };

    // Disable condition also aborts attempts between clock events. A constant disable
    // never rises, and $sampled is unsupported in a sensitivity list.
    ایستا منطقی isAsyncDisable(ثابت Ast_گره_عبارت* ثابت disableExprp) {
        بازگشت disableExprp && !VN_IS(disableExprp, Const)
               && !disableExprp->exists([](ثابت AstSampled*) { بازگشت درست; });
    }

    // Emit the clocked update block of registers that hold pending attempts. The clearp
    // statements reset the same registers: a disable rising between clock events also
    // aborts pending attempts (IEEE 1800-2023 16.12), so clearp runs on that rising edge
    // in a separate block. It must clear unconditionally, as the disable may already be
    // false again when it runs, and the update block must not run on that edge, as it
    // would advance the attempts as if the clock had ticked. Blocks that hold no attempt
    // state, such as the $assertkill acknowledge in emitKillAckNba, do not go through here.
    // Callers pass a null clearp when the disable cannot rise between clock events.
    تهی addStateAlways(LowerCtx& c, Ast_گره* bodyp, Ast_گره* clearp) {
        m_modp->addStmtsp(
            جدید AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_درخت(نادرست), bodyp});
        اگر (!clearp) بازگشت;
        m_modp->addStmtsp(جدید AstAlways{
            c.flp, VAlwaysKwd::ALWAYS,
            جدید Ast_Sen_درخت{c.flp, جدید AstSenItem{c.flp, VEdgeType::ET_POSEDGE,
                                                 c.disableExprp->clone_درخت_Pure(نادرست)}},
            clearp});
    }

    // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
    // Phase 2: State register NBA always block. Each clocked-edge target
    // latches the OR of its incoming contributions.
    تهی emitStateRegisterNba(LowerCtx& c) {
        ثابت منطقی asyncDisable = isAsyncDisable(c.disableExprp);
        Ast_گره* bodyp = nullptr;
        Ast_گره* clearp = nullptr;
        برای (ثابت V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ثابت SvaStateVertex* ثابت vtxp = static_cast<ثابت SvaStateVertex*>(&vtxr);
            اگر (!vtxp->datap()->stateVarp) ادامه;

            Ast_گره_عبارت* nextStatep = nullptr;
            برای (ثابت V3GraphEdge& edger : vtxp->inEdges()) {
                ثابت SvaTransEdge& tedger = static_cast<ثابت SvaTransEdge&>(edger);
                اگر (!tedger.m_consumesCycle) ادامه;
                ثابت SvaStateVertex* ثابت fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Clocked-edge source missing stateSig");

                Ast_گره_عبارت* srcSigp = fromVtxp->datap()->stateSigp->clone_درخت_Pure(نادرست);
                srcSigp = andCond(c.flp, srcSigp, tedger.m_condp);

                اگر (c.disableExprp) {
                    Ast_گره_عبارت* ثابت notDisp
                        = جدید AstLogNot{c.flp, c.disableExprp->clone_درخت_Pure(نادرست)};
                    srcSigp = جدید AstLogAnd{c.flp, srcSigp, notDisp};
                }
                nextStatep = orExprs(c.flp, nextStatep, srcSigp);
            }

            UASSERT_OBJ(nextStatep, vtxp,
                        "Registered vertex has no clocked incoming contribution");
            nextStatep = gateNotKill(c, nextStatep);

            AstAssignDly* ثابت assignp = جدید AstAssignDly{
                c.flp, جدید AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE}, nextStatep};
            bodyp = Ast_گره::addNextNull(bodyp, assignp);
            اگر (asyncDisable) {
                clearp = Ast_گره::addNextNull(
                    clearp,
                    جدید AstAssignDly{
                        c.flp, جدید AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE},
                        جدید AstConst{c.flp, AstConst::BitFalse{}}});
            }
        }
        اگر (bodyp) addStateAlways(c, bodyp, clearp);
    }

    // Phase 2b: In-place ring storage and registered ring metadata.
    تهی emitDelayRingUpdates(LowerCtx& c, ثابت منطقی شمار_Matches) {
        ثابت منطقی asyncDisable = isAsyncDisable(c.disableExprp);
        برای (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ثابت vtxp = static_cast<SvaStateVertex*>(&vtxr);
            اگر (!vtxp->datap()->delayRingVarp) ادامه;
            AstVar* ثابت idxp = vtxp->datap()->delayRingIdxVarp;
            AstVar* ثابت live_شمار_Varp = vtxp->datap()->delay_Ring_Live_شمار_Varp;
            AstVar* ثابت wrappedp = vtxp->datap()->delayRingWrappedVarp;
            ثابت uint32_t size = static_cast<uint32_t>(vtxp->m_delayRingSize);

            Ast_گره_عبارت* incomingp = nullptr;
            Ast_گره_عبارت* incomingOverflowp = nullptr;
            برای (ثابت V3GraphEdge& edger : vtxp->inEdges()) {
                ++m_statDelayRingEdgeVisits;
                ثابت SvaTransEdge& tedger = static_cast<ثابت SvaTransEdge&>(edger);
                UASSERT_OBJ(tedger.m_consumesCycle == vtxp->m_isFixedDelayRing, vtxp,
                            "Delay-ring incoming edge kind mismatch");
                ثابت SvaStateVertex* ثابت fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Delay-ring incoming source missing stateSig");
                UASSERT_OBJ(!شمار_Matches || fromVtxp->datap()->state_شمار_Sigp, fromVtxp,
                            "Delay-ring incoming source missing stateCountSig");
                // IEEE 1800-2023 16.14.1/16.14.3: assertion alternatives belong to one
                // evaluation; only cover sequence counts each end-of-match.
                Ast_گره_عبارت* contribp
                    = شمار_Matches
                          ? fromVtxp->datap()->state_شمار_Sigp->clone_درخت_Pure(نادرست)
                          : جدید AstExtend{c.flp,
                                          fromVtxp->datap()->stateSigp->clone_درخت_Pure(نادرست),
                                          m_u32DTypep->width()};
                Ast_گره_عبارت* contribOverflowp
                    = شمار_Matches && fromVtxp->datap()->state_شمار_Overflow_Sigp
                          ? fromVtxp->datap()->state_شمار_Overflow_Sigp->clone_درخت_Pure(نادرست)
                          : nullptr;
                اگر (tedger.m_condp) {
                    Ast_گره_عبارت* ثابت enablep = tedger.m_condp->clone_درخت_Pure(نادرست);
                    contribp = شمار_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, enablep)
                                   : addCountp(c.flp, nullptr, contribp, enablep);
                }
                اگر (c.disableExprp) {
                    Ast_گره_عبارت* ثابت notDisp
                        = جدید AstLogNot{c.flp, c.disableExprp->clone_درخت_Pure(نادرست)};
                    contribp = شمار_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, notDisp)
                                   : addCountp(c.flp, nullptr, contribp, notDisp);
                }
                incomingp = شمار_Matches ? addCheckedCountp(c.flp, incomingp, contribp,
                                                            incomingOverflowp, contribOverflowp)
                                         : addCountp(c.flp, incomingp, contribp);
            }
            UASSERT_OBJ(incomingp, vtxp, "Delay ring has no incoming edge");
            ثابت منطقی storesCounts = vtxp->datap()->delayRingCounts;
            Ast_گره_عبارت* ثابت incomingCountp
                = storesCounts
                      ? incomingp->clone_درخت_Pure(نادرست)
                      : جدید AstExtend{c.flp,
                                      جدید AstNeq{c.flp, incomingp->clone_درخت_Pure(نادرست),
                                                 newTypedConstp(c.flp, incomingp->dtypep(), 0)},
                                      m_u32DTypep->width()};
            Ast_گره_عبارت* ثابت storedIncomingp
                = storesCounts ? incomingp
                               : جدید AstNeq{c.flp, incomingp,
                                            newTypedConstp(c.flp, incomingp->dtypep(), 0)};
            // Only this block accesses the storage. Other processes read the cached head, so
            // the ring can be updated in place without an NBA shadow copy.
            AstAssign* ثابت writeIncomingp = جدید AstAssign{
                c.flp,
                delayRingSlot(c.flp, vtxp, جدید AstVarRef{c.flp, idxp, VAccess::READ},
                              VAccess::WRITE),
                storedIncomingp};
            Ast_گره* updateBodyp = writeIncomingp;
            // live_count <= live_count - outgoing_count + incoming_count;
            Ast_گره_عبارت* ثابت outgoingp = delay_Ring_Output_شمار(c.flp, vtxp);
            Ast_گره_عبارت* ثابت afterOutgoingp
                = جدید AstSub{c.flp, جدید AstVarRef{c.flp, live_شمار_Varp, VAccess::READ}, outgoingp};
            Ast_گره_عبارت* ثابت oldAfterOutgoingp = afterOutgoingp->clone_درخت_Pure(نادرست);
            AstAdd* ثابت nextLiveCountp = جدید AstAdd{c.flp, afterOutgoingp, incomingCountp};
            اگر (storesCounts) {
                Ast_گره_عبارت* ثابت liveOverflowp
                    = جدید AstLt{c.flp, nextLiveCountp->clone_درخت_Pure(نادرست), oldAfterOutgoingp};
                Ast_گره_عبارت* ثابت overflowp = orExprs(c.flp, incomingOverflowp, liveOverflowp);
                updateBodyp = new_شمار_Overflow_Warning(c.flp, overflowp);
                updateBodyp->addNext(writeIncomingp);
            } وگرنه {
                VL_DO_DANGLING(oldAfterOutgoingp->delete_درخت(), oldAfterOutgoingp);
            }
            // Cache the outgoing slot for the next index after the write. Reading after
            // writing also handles one-slot rings and two-slot range rings without alias cases.
            Ast_گره_عبارت* nextHeadIdxp
                = nextRingIndex(c.flp, جدید AstVarRef{c.flp, idxp, VAccess::READ}, size);
            اگر (!vtxp->m_isFixedDelayRing) {
                nextHeadIdxp = nextRingIndex(c.flp, nextHeadIdxp, size);
            }
            updateBodyp->addNext(جدید AstAssignDly{
                c.flp, جدید AstVarRef{c.flp, vtxp->datap()->delayRingHeadVarp, VAccess::WRITE},
                delayRingSlot(c.flp, vtxp, nextHeadIdxp)});
            updateBodyp->addNext(جدید AstAssignDly{
                c.flp, جدید AstVarRef{c.flp, live_شمار_Varp, VAccess::WRITE}, nextLiveCountp});
            // wrapped <= wrapped || idx == size - 1;
            updateBodyp->addNext(
                جدید AstAssignDly{c.flp, جدید AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                 جدید AstLogOr{c.flp, جدید AstVarRef{c.flp, wrappedp, VAccess::READ},
                                              delayRingAtLastIndex(c.flp, idxp, size)}});
            // idx <= next_idx;
            updateBodyp->addNext(جدید AstAssignDly{
                c.flp, جدید AstVarRef{c.flp, idxp, VAccess::WRITE},
                nextRingIndex(c.flp, جدید AstVarRef{c.flp, idxp, VAccess::READ}, size)});
            اگر (vtxp->m_delayRingAdvanceCondp) {
                updateBodyp = جدید AstIf{
                    c.flp, sampled(vtxp->m_delayRingAdvanceCondp->clone_درخت_Pure(نادرست)),
                    updateBodyp};
            }

            Ast_گره_عبارت* clearCondp = killActive(c);
            اگر (vtxp->m_delayRingClearCondp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_delayRingClearCondp->clone_درخت_Pure(نادرست)));
            }
            اگر (vtxp->m_abortClearp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_abortClearp->clone_درخت_Pure(نادرست)));
            }
            اگر (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_درخت_Pure(نادرست));
            }
            Ast_گره_عبارت* guardp = nullptr;
            برای (Ast_گره_عبارت* ثابت cp : vtxp->m_throughoutConds) {
                Ast_گره_عبارت* ثابت sampledp = sampled(cp->clone_درخت_Pure(نادرست));
                guardp = guardp ? static_cast<Ast_گره_عبارت*>(جدید AstLogAnd{c.flp, guardp, sampledp})
                                : sampledp;
            }
            اگر (guardp) clearCondp = orExprs(c.flp, clearCondp, جدید AstLogNot{c.flp, guardp});
            // Logically clear the ring without touching its wide storage.
            AstAssignDly* ثابت clearCountp
                = جدید AstAssignDly{c.flp, جدید AstVarRef{c.flp, live_شمار_Varp, VAccess::WRITE},
                                   newTypedConstp(c.flp, live_شمار_Varp->dtypep(), 0)};
            clearCountp->addNext(جدید AstAssignDly{c.flp,
                                                  جدید AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                                  جدید AstConst{c.flp, AstConst::BitFalse{}}});
            clearCountp->addNext(جدید AstAssignDly{c.flp,
                                                  جدید AstVarRef{c.flp, idxp, VAccess::WRITE},
                                                  newTypedConstp(c.flp, idxp->dtypep(), 0)});
            updateBodyp = جدید AstIf{c.flp, clearCondp, clearCountp, updateBodyp};

            addStateAlways(c, updateBodyp, asyncDisable ? clearCountp->clone_درخت(درست) : nullptr);
        }
    }

    // Phase 2c: SAnd combiner done-latch always block.
    // NBA semantics ensure doneL/doneR read pre-update values (IEEE 16.9.5).
    تهی emitAndCombinerDoneLatchNba(LowerCtx& c) {
        ثابت منطقی asyncDisable = isAsyncDisable(c.disableExprp);
        برای (ثابت V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ثابت SvaStateVertex* ثابت avp = static_cast<ثابت SvaStateVertex*>(&vtxr);
            اگر (!avp->datap()->doneLVarp) ادامه;
            // doneLVars is non-null only for AndCombiner vertices, which always
            // have both m_andLhsTermp and m_andRhsTermp set at build time.
            UASSERT_OBJ(avp->m_andLhsTermp && avp->m_andRhsTermp, avp,
                        "AndCombiner vertex missing LHS/RHS terminal");
            ثابت SvaStateVertex* ثابت lhsTermp = avp->m_andLhsTermp;
            ثابت SvaStateVertex* ثابت rhsTermp = avp->m_andRhsTermp;
            // resolveLinks' 2*N+2 fixed-point pass is guaranteed to populate
            // stateSigp on every AndCombiner and its LHS/RHS terminals.
            UASSERT_OBJ(lhsTermp->datap()->stateSigp && rhsTermp->datap()->stateSigp
                            && avp->datap()->stateSigp,
                        avp, "AndCombiner stateSigp chain unresolved after resolveLinks");

            AstAssignDly* ثابت clearLp = جدید AstAssignDly{
                c.flp, جدید AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                جدید AstConst{c.flp, AstConst::BitFalse{}}};
            AstAssignDly* ثابت clearRp = جدید AstAssignDly{
                c.flp, جدید AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                جدید AstConst{c.flp, AstConst::BitFalse{}}};
            clearLp->addNext(clearRp);

            Ast_گره_عبارت* ثابت matchLNowp
                = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, avp->m_andLhsCondp);
            Ast_گره_عبارت* ثابت matchRNowp
                = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, avp->m_andRhsCondp);
            Ast_گره_عبارت* gateLp = matchLNowp;
            Ast_گره_عبارت* gateRp = matchRNowp;
            اگر (c.disableExprp) {
                Ast_گره_عبارت* ثابت notDisLp
                    = جدید AstLogNot{c.flp, c.disableExprp->clone_درخت_Pure(نادرست)};
                gateLp = جدید AstLogAnd{c.flp, gateLp, notDisLp};
                Ast_گره_عبارت* ثابت notDisRp
                    = جدید AstLogNot{c.flp, c.disableExprp->clone_درخت_Pure(نادرست)};
                gateRp = جدید AstLogAnd{c.flp, gateRp, notDisRp};
            }
            AstAssignDly* ثابت setLp = جدید AstAssignDly{
                c.flp, جدید AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                جدید AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* ثابت setLIfp = جدید AstIf{c.flp, gateLp, setLp, nullptr};
            AstAssignDly* ثابت setRp = جدید AstAssignDly{
                c.flp, جدید AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                جدید AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* ثابت setRIfp = جدید AstIf{c.flp, gateRp, setRp, nullptr};
            setLIfp->addNext(setRIfp);

            Ast_گره_عبارت* clearCondp = جدید AstLogOr{c.flp, killActive(c),
                                                   avp->datap()->stateSigp->clone_درخت_Pure(نادرست)};
            اگر (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_درخت_Pure(نادرست));
            }
            AstIf* ثابت topp = جدید AstIf{c.flp, clearCondp, clearLp, setLIfp};
            addStateAlways(c, topp, asyncDisable ? clearLp->clone_درخت(درست) : nullptr);
        }
    }

    تهی emitKillAckNba(LowerCtx& c) {
        AstAssignDly* ثابت ackp
            = جدید AstAssignDly{c.flp, جدید AstVarRef{c.flp, c.killVarp, VAccess::WRITE},
                               assertKillGet(c.flp, c.assertType, c.directiveType)};
        m_modp->addStmtsp(
            جدید AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_درخت(نادرست), ackp});
    }

    // Phase 3/3a/3b: Compute terminal match/reject signals, required-step reject,
    // throughout-drop reject; clean up intermediate state signals.
    // Phase 3: terminalActive and rejectBase from Links to matchVertex.
    // Builder only adds Links (non-clocked) to matchVertex via addLink in
    // wireMatchAndMidSources. For cover sequence, also count each end-of-match
    // so the action can be replayed without unrolling endpoints.
    تهی computeTerminalMatchAndReject(LowerCtx& c, SignalSet& sigs, ثابت منطقی need_Match_شمار) {
        برای (ثابت SvaTransEdge* ثابت tedgep : c.graph.allEdges()) {
            اگر (tedgep->toVtxp() != c.graph.m_matchVertexp) ادامه;
            SvaStateVertex* ثابت fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                        "Terminal-link source missing stateSig");

            Ast_گره_عبارت* srcSigp = fromVtxp->datap()->stateSigp->clone_درخت_Pure(نادرست);
            srcSigp = andCond(c.flp, srcSigp, tedgep->m_condp);
            اگر (need_Match_شمار) {
                UASSERT_OBJ(fromVtxp->datap()->state_شمار_Sigp, fromVtxp,
                            "Terminal-link source missing stateCountSig");
                Ast_گره_عبارت* contributionp
                    = fromVtxp->datap()->state_شمار_Sigp->clone_درخت_Pure(نادرست);
                Ast_گره_عبارت* contributionOverflowp
                    = fromVtxp->datap()->state_شمار_Overflow_Sigp
                          ? fromVtxp->datap()->state_شمار_Overflow_Sigp->clone_درخت_Pure(نادرست)
                          : nullptr;
                اگر (tedgep->m_condp) {
                    contributionp = gateCheckedCountp(c.flp, contributionp, contributionOverflowp,
                                                      tedgep->m_condp->clone_درخت_Pure(نادرست));
                }
                sigs.matchCountp
                    = addCheckedCountp(c.flp, sigs.matchCountp, contributionp,
                                       sigs.match_شمار_Overflowp, contributionOverflowp);
            }

            اگر (fromVtxp->m_delayRingSize && !fromVtxp->m_isFixedDelayRing) {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_درخت_Pure(نادرست));
                // reject |= (wrapped || idx == size - 1) && head && final_condition;
                Ast_گره_عبارت* expireContribp = delay_Ring_Output_شمار(c.flp, fromVtxp);
                expireContribp = andCond(c.flp, expireContribp, tedgep->m_condp);
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, expireContribp);
                VL_DO_DANGLING(srcSigp->delete_درخت(), srcSigp);
            } وگرنه اگر (fromVtxp->m_isUnbounded || fromVtxp->m_isAndCombiner) {
                sigs.terminalActivep = orExprs(c.flp, sigs.terminalActivep, srcSigp);
            } وگرنه {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_درخت_Pure(نادرست));
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, srcSigp);
            }
        }
        // wireMatchAndMidSources always adds a Link from result.termVertexp
        // to m_matchVertexp, so the loop above always sets terminalActivep.
        UASSERT_OBJ(sigs.terminalActivep, c.graph.m_matchVertexp,
                    "No terminal edge to match vertex");
    }

    Ast_گره_عبارت* newThroughoutThreadFailCountp(LowerCtx& c, AstVar* ثابت delay_Ring_Live_شمار_Varp,
                                               Ast_گره_عبارت* ثابت stateExprp,
                                               Ast_گره_عبارت* ثابت enablep) {
        Ast_گره_عبارت* ثابت activeThreadCountp
            = delay_Ring_Live_شمار_Varp
                  ? static_cast<Ast_گره_عبارت*>(
                        جدید AstVarRef{c.flp, delay_Ring_Live_شمار_Varp, VAccess::READ})
                  : جدید AstExtend{c.flp, stateExprp->clone_درخت_Pure(نادرست), m_u32DTypep->width()};
        بازگشت addCountp(c.flp, nullptr, activeThreadCountp, enablep);
    }

    // Phase 3b: Throughout-drop and ring-wide abort rejection.
    تهی computeThroughoutReject(LowerCtx& c, SignalSet& sigs, ثابت منطقی need_Thread_Fail_شمار) {
        برای (ثابت V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ثابت SvaStateVertex* ثابت vtxp = static_cast<ثابت SvaStateVertex*>(&vtxr);
            ثابت auto& conds = vtxp->m_throughoutConds;
            اگر (conds.empty() && !vtxp->m_abortRejectp) ادامه;
            اگر (vtxp->m_isAndCombiner) ادامه;
            Ast_گره_عبارت* stateExprp = nullptr;
            اگر (vtxp->datap()->stateVarp) {
                stateExprp = جدید AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
            } وگرنه اگر (vtxp->datap()->delayRingVarp && vtxp->m_isFixedDelayRing) {
                stateExprp
                    = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_شمار_Varp);
            } وگرنه {
                UASSERT_OBJ(vtxp->datap()->stateSigp, vtxp,
                            "Throughout-conds vertex missing state representation");
                stateExprp = vtxp->datap()->stateSigp->clone_درخت_Pure(نادرست);
            }
            Ast_گره_عبارت* guardp = nullptr;
            برای (Ast_گره_عبارت* ثابت cp : conds) {
                Ast_گره_عبارت* ثابت sp = sampled(cp->clone_درخت_Pure(نادرست));
                guardp = guardp ? static_cast<Ast_گره_عبارت*>(جدید AstLogAnd{c.flp, guardp, sp}) : sp;
            }
            اگر (vtxp->m_abortRejectp) {
                Ast_گره_عبارت* ثابت notAbortp
                    = جدید AstLogNot{c.flp, sampled(vtxp->m_abortRejectp->clone_درخت_Pure(نادرست))};
                guardp = guardp
                             ? static_cast<Ast_گره_عبارت*>(جدید AstLogAnd{c.flp, guardp, notAbortp})
                             : notAbortp;
            }
            Ast_گره_عبارت* rejectCondp = جدید AstLogNot{c.flp, guardp};
            اگر (vtxp->m_abortClearp) {
                // Any abort clears the ring. Accept aborts suppress a simultaneous guard failure;
                // reject aborts are restored below so they still force rejection.
                Ast_گره_عبارت* ثابت notAbortClearp
                    = جدید AstLogNot{c.flp, sampled(vtxp->m_abortClearp->clone_درخت_Pure(نادرست))};
                rejectCondp = جدید AstLogAnd{c.flp, rejectCondp, notAbortClearp};
                اگر (vtxp->m_abortRejectp) {
                    rejectCondp = جدید AstLogOr{
                        c.flp, rejectCondp, sampled(vtxp->m_abortRejectp->clone_درخت_Pure(نادرست))};
                }
            }
            اگر (need_Thread_Fail_شمار) {
                Ast_گره_عبارت* ثابت contributionp
                    = newThroughoutThreadFailCountp(c, vtxp->datap()->delay_Ring_Live_شمار_Varp,
                                                    stateExprp, rejectCondp->clone_درخت_Pure(نادرست));
                sigs.threadFailCountp = addCountp(c.flp, sigs.threadFailCountp, contributionp);
                اگر (vtxp->m_abortRejectp && vtxp->m_delayRingAdvanceCondp) {
                    // An advancing ring thread also occupies its same-tick match vertex in the
                    // unrolled NFA, so abort rejection must replay both fail actions.
                    Ast_گره_عبارت* ثابت abortAndAdvancep = جدید AstLogAnd{
                        c.flp, sampled(vtxp->m_abortRejectp->clone_درخت_Pure(نادرست)),
                        sampled(vtxp->m_delayRingAdvanceCondp->clone_درخت_Pure(نادرست))};
                    Ast_گره_عبارت* ثابت matchContributionp = newThroughoutThreadFailCountp(
                        c, vtxp->datap()->delay_Ring_Live_شمار_Varp, stateExprp, abortAndAdvancep);
                    sigs.threadFailCountp
                        = addCountp(c.flp, sigs.threadFailCountp, matchContributionp);
                }
            }
            sigs.throughoutRejectp = orExprs(c.flp, sigs.throughoutRejectp,
                                             جدید AstLogAnd{c.flp, stateExprp, rejectCondp});
        }
    }

    SignalSet computeSignals(LowerCtx& c, ثابت منطقی need_Thread_Fail_شمار,
                             ثابت منطقی need_Throughout_Thread_Fail_شمار, ثابت منطقی need_Match_شمار) {
        SignalSet sigs;

        computeTerminalMatchAndReject(c, sigs, need_Match_شمار);

        // Phase 3a: required-step rejection.
        // Builder only sets m_rejectOnFail on non-clocked Links with m_condp
        // or m_condVtxp, and the source always has a resolved stateSig.
        برای (ثابت SvaTransEdge* ثابت tedgep : c.graph.allEdges()) {
            اگر (!tedgep->m_rejectOnFail) ادامه;
            ثابت SvaStateVertex* ثابت fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp && (tedgep->m_condp || tedgep->m_condVtxp),
                        fromVtxp,
                        "rejectOnFail Link must have condp/condVtxp and source stateSig");
            Ast_گره_عبارت* ثابت srcSigp = fromVtxp->datap()->stateSigp->clone_درخت_Pure(نادرست);
            Ast_گره_عبارت* condp = nullptr;
            اگر (tedgep->m_condVtxp) {
                UASSERT_OBJ(tedgep->m_condVtxp->datap()->stateSigp, tedgep->m_condVtxp,
                            "rejectOnFail condVtxp missing stateSig");
                condp = tedgep->m_condVtxp->datap()->stateSigp->clone_درخت_Pure(نادرست);
                اگر (tedgep->m_condp) {
                    condp = جدید AstLogOr{c.flp, condp, tedgep->m_condp->clone_درخت_Pure(نادرست)};
                }
            } وگرنه {
                condp = tedgep->m_condp->clone_درخت_Pure(نادرست);
            }
            Ast_گره_عبارت* ثابت notCondp = جدید AstLogNot{c.flp, condp};
            Ast_گره_عبارت* ثابت rawFailp = جدید AstLogAnd{c.flp, srcSigp, notCondp};
            اگر (need_Thread_Fail_شمار) {
                // thread_fail_count += fail;
                sigs.threadFailCountp = addCountp(
                    c.flp, sigs.threadFailCountp,
                    جدید AstExtend{c.flp, rawFailp->clone_درخت_Pure(نادرست), m_u32DTypep->width()});
            }
            Ast_گره_عبارت* ثابت failp = gateNotKill(c, rawFailp);
            sigs.requiredStepRejectp = orExprs(c.flp, sigs.requiredStepRejectp, failp);
        }

        computeThroughoutReject(c, sigs, need_Throughout_Thread_Fail_شمار);
        اگر (sigs.threadFailCountp) {
            sigs.threadFailCountp
                = addCountp(c.flp, nullptr, sigs.threadFailCountp, notKillActive(c));
        }
        اگر (sigs.matchCountp) {
            اگر (c.matchCondp) {
                sigs.matchCountp
                    = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_شمار_Overflowp,
                                        sampled(c.matchCondp->clone_درخت_Pure(نادرست)));
            }
            sigs.matchCountp = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_شمار_Overflowp,
                                                 notKillActive(c));
        }
        sigs.terminalActivep = gateNotKill(c, sigs.terminalActivep);
        sigs.rejectBasep = gateNotKill(c, sigs.rejectBasep);
        sigs.throughoutRejectp = gateNotKill(c, sigs.throughoutRejectp);

        // Clean up intermediate state signals. These are orphan subtrees
        // (never linked into the enclosing AST); deleteTree() is immediate
        // which is what we want since stateSigp lifetime ends with this scope.
        برای (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ثابت vtxp = static_cast<SvaStateVertex*>(&vtxr);
            Ast_گره_عبارت*& sigp = vtxp->datap()->stateSigp;
            اگر (sigp) VL_DO_DANGLING(sigp->delete_درخت(), sigp);
            Ast_گره_عبارت*& countp = vtxp->datap()->state_شمار_Sigp;
            اگر (countp) VL_DO_DANGLING(countp->delete_درخت(), countp);
            Ast_گره_عبارت*& overflowp = vtxp->datap()->state_شمار_Overflow_Sigp;
            اگر (overflowp) VL_DO_DANGLING(overflowp->delete_درخت(), overflowp);
        }
        // Disable iff gating (IEEE 1800-2023 16.12) on the current level value.
        اگر (c.disableExprp) {
            // terminalActivep is always set, so gate it unconditionally.
            Ast_گره_عبارت* ثابت notTermp
                = جدید AstLogNot{c.flp, c.disableExprp->clone_درخت_Pure(نادرست)};
            sigs.terminalActivep = جدید AstLogAnd{c.flp, sigs.terminalActivep, notTermp};
            اگر (sigs.rejectBasep) {
                Ast_گره_عبارت* ثابت notDisp
                    = جدید AstLogNot{c.flp, c.disableExprp->clone_درخت_Pure(نادرست)};
                sigs.rejectBasep = جدید AstLogAnd{c.flp, sigs.rejectBasep, notDisp};
            }
            اگر (sigs.throughoutRejectp) {
                Ast_گره_عبارت* ثابت notDisp
                    = جدید AstLogNot{c.flp, c.disableExprp->clone_درخت_Pure(نادرست)};
                sigs.throughoutRejectp = جدید AstLogAnd{c.flp, sigs.throughoutRejectp, notDisp};
            }
            اگر (sigs.requiredStepRejectp) {
                Ast_گره_عبارت* ثابت notDisp
                    = جدید AstLogNot{c.flp, c.disableExprp->clone_درخت_Pure(نادرست)};
                sigs.requiredStepRejectp = جدید AstLogAnd{c.flp, sigs.requiredStepRejectp, notDisp};
            }
            اگر (sigs.matchCountp) {
                sigs.matchCountp = gateCheckedCountp(
                    c.flp, sigs.matchCountp, sigs.match_شمار_Overflowp,
                    جدید AstLogNot{c.flp, c.disableExprp->clone_درخت_Pure(نادرست)});
            }
        }

        اگر (sigs.match_شمار_Overflowp) {
            m_modp->addStmtsp(
                جدید AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_درخت(نادرست),
                              new_شمار_Overflow_Warning(c.flp, sigs.match_شمار_Overflowp)});
            sigs.match_شمار_Overflowp = nullptr;
        }

        بازگشت sigs;
    }

    // Phase 1: Resolve combinational Links via fixed-point propagation.
    تهی resolveLinks(LowerCtx& c, Ast_گره_عبارت* triggerExprp, ثابت منطقی شمار_Matches) {
        // datap() was freshly allocated in lower() -- all state signals start null.
        c.graph.m_startVertexp->datap()->stateSigp = triggerExprp->clone_درخت_Pure(نادرست);
        c.graph.m_startVertexp->datap()->state_شمار_Sigp
            = جدید AstExtend{c.flp, triggerExprp->clone_درخت_Pure(نادرست), m_u32DTypep->width()};
        برای (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ثابت vtxp = static_cast<SvaStateVertex*>(&vtxr);
            اگر (vtxp->datap()->stateVarp) {
                vtxp->datap()->stateSigp
                    = جدید AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
                vtxp->datap()->state_شمار_Sigp = جدید AstExtend{
                    c.flp, جدید AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ},
                    m_u32DTypep->width()};
            } وگرنه اگر (vtxp->datap()->delayRingVarp) {
                اگر (vtxp->m_isFixedDelayRing) {
                    // state_count = wrapped ? head : 0; state = state_count != 0;
                    Ast_گره_عبارت* ثابت countp = delay_Ring_Output_شمار(c.flp, vtxp);
                    vtxp->datap()->stateSigp = جدید AstNeq{c.flp, countp->clone_درخت_Pure(نادرست),
                                                          newTypedConstp(c.flp, m_u32DTypep, 0)};
                    vtxp->datap()->state_شمار_Sigp = countp;
                } وگرنه {
                    vtxp->datap()->stateSigp
                        = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_شمار_Varp);
                    vtxp->datap()->state_شمار_Sigp = جدید AstVarRef{
                        c.flp, vtxp->datap()->delay_Ring_Live_شمار_Varp, VAccess::READ};
                }
            }
        }
        // Fixed-point propagation along zero-delay (Link) edges. Rebuild each
        // derived signal from its incoming edges on every pass; appending the same
        // contributions repeatedly makes merge expressions grow exponentially.
        ثابت size_t passes = 2 * c.graph.m_graph.vertices().size() + 2;
        برای (size_t pass = 0; pass < passes; ++pass) {
            // Rebuild SAnd combiners once both sub-NFA terminals are available.
            برای (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* ثابت vtxp = static_cast<SvaStateVertex*>(&vtxr);
                اگر (!vtxp->m_isAndCombiner) ادامه;
                // AndCombiner vertices always have both terminal pointers set.
                UASSERT_OBJ(vtxp->m_andLhsTermp && vtxp->m_andRhsTermp, vtxp,
                            "AndCombiner vertex missing LHS/RHS terminal");
                ثابت SvaStateVertex* ثابت lhsTermp = vtxp->m_andLhsTermp;
                ثابت SvaStateVertex* ثابت rhsTermp = vtxp->m_andRhsTermp;
                اگر (!lhsTermp->datap()->stateSigp || !rhsTermp->datap()->stateSigp) ادامه;
                Ast_گره_عبارت* ثابت matchLp
                    = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, vtxp->m_andLhsCondp);
                Ast_گره_عبارت* ثابت matchRp
                    = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, vtxp->m_andRhsCondp);
                Ast_گره_عبارت* ثابت doneLOrp = جدید AstLogOr{
                    c.flp, جدید AstVarRef{c.flp, vtxp->datap()->doneLVarp, VAccess::READ}, matchLp};
                Ast_گره_عبارت* ثابت doneROrp = جدید AstLogOr{
                    c.flp, جدید AstVarRef{c.flp, vtxp->datap()->doneRVarp, VAccess::READ}, matchRp};
                Ast_گره_عبارت* ثابت bothp = جدید AstLogAnd{c.flp, doneLOrp, doneROrp};
                Ast_گره_عبارت* ثابت oneNowp = جدید AstLogOr{c.flp, matchLp->clone_درخت_Pure(نادرست),
                                                          matchRp->clone_درخت_Pure(نادرست)};
                اگر (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_درخت(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = جدید AstLogAnd{c.flp, bothp, oneNowp};
                اگر (vtxp->datap()->state_شمار_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_شمار_Sigp->delete_درخت(),
                                   vtxp->datap()->state_شمار_Sigp);
                }
                vtxp->datap()->state_شمار_Sigp = جدید AstExtend{
                    c.flp, vtxp->datap()->stateSigp->clone_درخت_Pure(نادرست), m_u32DTypep->width()};
            }

            برای (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* ثابت vtxp = static_cast<SvaStateVertex*>(&vtxr);
                اگر (vtxp == c.graph.m_startVertexp || vtxp->datap()->stateVarp
                    || vtxp->datap()->delayRingVarp || vtxp->m_isAndCombiner || vtxp->m_isMatch
                    || vtxp->m_isRejectSink) {
                    ادامه;
                }
                Ast_گره_عبارت* nextStatep = nullptr;
                Ast_گره_عبارت* nextStateCountp = nullptr;
                Ast_گره_عبارت* next_State_شمار_Overflowp = nullptr;
                برای (ثابت V3GraphEdge& er : vtxp->inEdges()) {
                    ثابت SvaTransEdge& te = static_cast<ثابت SvaTransEdge&>(er);
                    ثابت SvaStateVertex* ثابت fromVtxp = te.fromVtxp();
                    اگر (!fromVtxp->datap()->stateSigp) ادامه;
                    UASSERT_OBJ(fromVtxp->datap()->state_شمار_Sigp, fromVtxp,
                                "Link source missing stateCountSig");
                    Ast_گره_عبارت* ثابت contributionp = andCond(
                        c.flp, fromVtxp->datap()->stateSigp->clone_درخت_Pure(نادرست), te.m_condp);
                    nextStatep = orExprs(c.flp, nextStatep, contributionp);
                    Ast_گره_عبارت* شمار_Contributionp
                        = fromVtxp->datap()->state_شمار_Sigp->clone_درخت_Pure(نادرست);
                    Ast_گره_عبارت* شمار_Contribution_Overflowp
                        = شمار_Matches && fromVtxp->datap()->state_شمار_Overflow_Sigp
                              ? fromVtxp->datap()->state_شمار_Overflow_Sigp->clone_درخت_Pure(نادرست)
                              : nullptr;
                    اگر (te.m_condp) {
                        Ast_گره_عبارت* ثابت enablep = te.m_condp->clone_درخت_Pure(نادرست);
                        شمار_Contributionp
                            = شمار_Matches
                                  ? gateCheckedCountp(c.flp, شمار_Contributionp,
                                                      شمار_Contribution_Overflowp, enablep)
                                  : addCountp(c.flp, nullptr, شمار_Contributionp, enablep);
                    }
                    nextStateCountp
                        = شمار_Matches
                              ? addCheckedCountp(c.flp, nextStateCountp, شمار_Contributionp,
                                                 next_State_شمار_Overflowp,
                                                 شمار_Contribution_Overflowp)
                              : addCountp(c.flp, nextStateCountp, شمار_Contributionp);
                }
                اگر (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_درخت(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = nextStatep;
                اگر (vtxp->datap()->state_شمار_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_شمار_Sigp->delete_درخت(),
                                   vtxp->datap()->state_شمار_Sigp);
                }
                vtxp->datap()->state_شمار_Sigp = nextStateCountp;
                اگر (vtxp->datap()->state_شمار_Overflow_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_شمار_Overflow_Sigp->delete_درخت(),
                                   vtxp->datap()->state_شمار_Overflow_Sigp);
                }
                vtxp->datap()->state_شمار_Overflow_Sigp = next_State_شمار_Overflowp;
            }
        }
    }

عمومی:
    // Combine terminal/reject signals into final output expression.
    ایستا Ast_گره_عبارت* assembleResult(Ast_گره_Cover_Or_Assert* ثابت assertp, ثابت منطقی negated,
                                       Ast_گره_عبارت* ثابت matchCondp, ثابت SignalSet& sigs,
                                       Ast_گره_عبارت** ثابت outMatchpp) {
        File_سطر* ثابت flp = assertp->fileline();
        ثابت منطقی isCover = VN_IS(assertp, Cover);
        Ast_گره_عبارت* ثابت terminalActivep = sigs.terminalActivep;
        Ast_گره_عبارت* ثابت rejectBasep = sigs.rejectBasep;
        Ast_گره_عبارت* ثابت throughoutRejectp = sigs.throughoutRejectp;
        Ast_گره_عبارت* ثابت requiredStepRejectp = sigs.requiredStepRejectp;
        // Property negation (IEEE 1800-2023 16.12.1 `not`): invert match/reject.
        اگر (negated) {
            اگر (isCover) {
                اگر (terminalActivep)
                    VL_DO_DANGLING(terminalActivep->delete_درخت(), terminalActivep);
                Ast_گره_عبارت* negRejectp = nullptr;
                اگر (matchCondp && rejectBasep) {
                    Ast_گره_عبارت* ثابت sampledCondp = sampled(matchCondp->clone_درخت_Pure(نادرست));
                    Ast_گره_عبارت* ثابت notCondp = جدید AstLogNot{flp, sampledCondp};
                    negRejectp = جدید AstLogAnd{flp, rejectBasep, notCondp};
                } وگرنه اگر (rejectBasep) {
                    VL_DO_DANGLING(rejectBasep->delete_درخت(), rejectBasep);
                }
                اگر (throughoutRejectp) negRejectp = orExprs(flp, negRejectp, throughoutRejectp);
                اگر (requiredStepRejectp)
                    negRejectp = orExprs(flp, negRejectp, requiredStepRejectp);
                بازگشت negRejectp ? negRejectp : جدید AstConst{flp, AstConst::BitFalse{}};
            }
            // Negated assert/assume: output = !match.
            Ast_گره_عبارت* matchp = terminalActivep;
            اگر (matchCondp) {
                Ast_گره_عبارت* ثابت sampledCondp = sampled(matchCondp->clone_درخت_Pure(نادرست));
                matchp = جدید AstLogAnd{flp, matchp, sampledCondp};
            }
            اگر (outMatchpp) {
                Ast_گره_عبارت* notPMatchp = nullptr;
                اگر (matchCondp && rejectBasep) {
                    Ast_گره_عبارت* ثابت sampledCondp = sampled(matchCondp->clone_درخت_Pure(نادرست));
                    notPMatchp = جدید AstLogAnd{flp, rejectBasep->clone_درخت_Pure(نادرست),
                                               جدید AstLogNot{flp, sampledCondp}};
                } وگرنه اگر (rejectBasep) {
                    notPMatchp = rejectBasep->clone_درخت_Pure(نادرست);
                }
                اگر (throughoutRejectp)
                    notPMatchp = orExprs(flp, notPMatchp, throughoutRejectp->clone_درخت_Pure(نادرست));
                اگر (requiredStepRejectp)
                    notPMatchp
                        = orExprs(flp, notPMatchp, requiredStepRejectp->clone_درخت_Pure(نادرست));
                *outMatchpp = notPMatchp;
            }
            اگر (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_درخت(), throughoutRejectp);
            اگر (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_درخت(), rejectBasep);
            اگر (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_درخت(), requiredStepRejectp);
            Ast_گره_عبارت* ثابت resultExprp = جدید AstLogNot{flp, matchp};
            بازگشت resultExprp;
        }
        اگر (isCover) {
            اگر (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_درخت(), throughoutRejectp);
            اگر (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_درخت(), rejectBasep);
            اگر (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_درخت(), requiredStepRejectp);
            اگر (matchCondp) {
                Ast_گره_عبارت* ثابت sampledCondp = sampled(matchCondp->clone_درخت_Pure(نادرست));
                بازگشت جدید AstLogAnd{flp, terminalActivep, sampledCondp};
            }
            بازگشت terminalActivep;
        }
        // Assert/assume: output = !reject
        Ast_گره_عبارت* rejectp = nullptr;
        اگر (matchCondp && rejectBasep) {
            Ast_گره_عبارت* ثابت sampledCondp = sampled(matchCondp->clone_درخت_Pure(نادرست));
            rejectp = جدید AstLogAnd{flp, rejectBasep, جدید AstLogNot{flp, sampledCondp}};
        } وگرنه اگر (rejectBasep) {
            VL_DO_DANGLING(rejectBasep->delete_درخت(), rejectBasep);
        }
        اگر (outMatchpp) {
            Ast_گره_عبارت* matchExprp = terminalActivep->clone_درخت_Pure(نادرست);
            اگر (matchCondp) {
                Ast_گره_عبارت* ثابت sp = sampled(matchCondp->clone_درخت_Pure(نادرست));
                matchExprp = جدید AstLogAnd{flp, matchExprp, sp};
            }
            *outMatchpp = matchExprp;
        }
        اگر (terminalActivep) VL_DO_DANGLING(terminalActivep->delete_درخت(), terminalActivep);
        اگر (throughoutRejectp) rejectp = orExprs(flp, rejectp, throughoutRejectp);
        اگر (requiredStepRejectp) rejectp = orExprs(flp, rejectp, requiredStepRejectp);
        اگر (!rejectp) بازگشت جدید AstConst{flp, AstConst::BitTrue{}};
        Ast_گره_عبارت* ثابت resultExprp = جدید AstLogNot{flp, rejectp};
        بازگشت resultExprp;
    }

    explicit SvaNfaLowering(Ast_گره_Module* modp)
        : m_modp{modp}
        , m_u32DTypep{modp->findBasicDType(VBasicDTypeKwd::UINT32)} {}
    ~SvaNfaLowering() {
        V3Stats::addStatSum("Assertions, NFA delay ring edge visits", m_statDelayRingEdgeVisits);
    }

    // Lower NFA graph to synthesizable AstAlways blocks and raw result signals.
    // Links are combinational; Edges are registered (NBA).
    SignalSet lower(Ast_گره_Cover_Or_Assert* ثابت assertp, SvaGraph& graph,
                    Ast_Sen_درخت* ثابت senTreep, Ast_گره_عبارت* ثابت matchCondp,
                    Ast_گره_عبارت* ثابت disableExprp, ثابت منطقی need_Thread_Fail_شمار,
                    ثابت منطقی need_Throughout_Thread_Fail_شمار) {
        File_سطر* ثابت flp = assertp->fileline();
        AstCover* ثابت coverp = VN_CAST(assertp, Cover);
        ثابت منطقی isSeqEvent = coverp && coverp->isSeqEvent();
        ثابت VAssertType assertType
            = isSeqEvent ? VAssertType{VAssertType::INTERNAL} : assertp->userType();
        ثابت VAssertDirectiveType directiveType
            = isSeqEvent ? VAssertDirectiveType{VAssertDirectiveType::INTERNAL}
                         : assertp->directive();
        ثابت std::string baseName = m_names.get("");

        // Number vertices with sequential colors for array indexing.
        صحیح N = 0;
        برای (V3GraphVertex& vtxr : graph.m_graph.vertices()) { vtxr.color(N++); }
        std::vector<SvaStateVertex*> vtx(N, nullptr);
        برای (V3GraphVertex& vtxr : graph.m_graph.vertices()) {
            vtx[vtxr.color()] = static_cast<SvaStateVertex*>(&vtxr);
        }

        // Allocate per-vertex lowering data (stored via V3GraphVertex::userp()).
        std::vector<SvaVertexData> vertexData(N);
        برای (صحیح i = 0; i < N; ++i) vtx[i]->userp(&vertexData[i]);

        // Identify registered vertices (targets of clocked edges).
        برای (صحیح i = 0; i < N; ++i) {
            برای (ثابت V3GraphEdge& edger : vtx[i]->outEdges()) {
                ثابت SvaTransEdge& tedger = static_cast<ثابت SvaTransEdge&>(edger);
                ثابت صحیح toIdx = tedger.toVtxp()->color();
                اگر (tedger.m_consumesCycle && tedger.toVtxp() != graph.m_matchVertexp
                    && !tedger.toVtxp()->m_isRejectSink) {
                    vtx[toIdx]->datap()->needsReg = درست;
                }
            }
        }

        // A packed range ring can emit several live endpoints without needing per-slot counts.
        // Widen only a ring whose input can already carry multiplicity.
        std::vector<منطقی> stateMayHaveMultiplicity(N);
        std::vector<منطقی> ringNeedsMultiplicity(N);
        std::vector<صحیح> multiplicityWork;
        ثابت منطقی شمار_Matches = coverp && coverp->isCoverSeq() && !isSeqEvent;
        اگر (شمار_Matches) {
            برای (صحیح i = 0; i < N; ++i) {
                ثابت منطقی multipleInputs = vtx[i]->inEdges().size() > 1;
                ثابت منطقی isRing = vtx[i]->m_delayRingSize;
                ringNeedsMultiplicity[i] = isRing && multipleInputs;
                ثابت منطقی startsMultiplicity = isRing
                                                    ? !vtx[i]->m_isFixedDelayRing
                                                    : multipleInputs && !vtx[i]->datap()->needsReg;
                اگر (startsMultiplicity) {
                    stateMayHaveMultiplicity[i] = درست;
                    multiplicityWork.افزودن_back(i);
                }
            }
            برای (size_t wi = 0; wi < multiplicityWork.size(); ++wi) {
                برای (ثابت V3GraphEdge& edger : vtx[multiplicityWork[wi]]->outEdges()) {
                    ثابت SvaTransEdge& tedger = static_cast<ثابت SvaTransEdge&>(edger);
                    ثابت صحیح ti = tedger.toVtxp()->color();
                    ثابت منطقی isRing = vtx[ti]->m_delayRingSize;
                    اگر (isRing) ringNeedsMultiplicity[ti] = درست;
                    // Match and reject sinks have no outgoing edges, so they stop naturally.
                    ثابت منطقی stopsMultiplicity = !isRing && vtx[ti]->datap()->needsReg;
                    اگر (stateMayHaveMultiplicity[ti] || stopsMultiplicity) { ادامه; }
                    stateMayHaveMultiplicity[ti] = درست;
                    multiplicityWork.افزودن_back(ti);
                }
            }
        }

        ثابت منطقی asyncDisable = isAsyncDisable(disableExprp);
        // Registers cleared on a rising disable are also written by their clocked block.
        File_سطر* ثابت regFlp = asyncDisable ? جدید File_سطر{flp} : flp;
        اگر (asyncDisable) regFlp->warnOff(V3ErrorCode::MULTIDRIVEN, درست);

        AstVar* ثابت killVarp
            = جدید AstVar{flp, VVarType::MODULETEMP, baseName + "__kill", m_u32DTypep};
        killVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(killVarp);
        برای (صحیح i = 0; i < N; ++i) {
            اگر (vtx[i]->m_isAndCombiner) {
                ثابت std::string base = baseName + "__a" + std::to_string(i);
                AstVar* ثابت lp = جدید AstVar{regFlp, VVarType::MODULETEMP, base + "_doneL",
                                              m_modp->findBitDType()};
                lp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(lp);
                vtx[i]->datap()->doneLVarp = lp;
                AstVar* ثابت rp = جدید AstVar{regFlp, VVarType::MODULETEMP, base + "_doneR",
                                              m_modp->findBitDType()};
                rp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(rp);
                vtx[i]->datap()->doneRVarp = rp;
                ادامه;
            }
            اگر (vtx[i]->m_delayRingSize) {
                ثابت std::string base = baseName + "__d" + std::to_string(i);
                ثابت منطقی storesCounts = شمار_Matches && ringNeedsMultiplicity[i];
                Ast_گره_DType* ringDTypep = nullptr;
                اگر (storesCounts) {
                    // int unsigned ring [size];
                    ringDTypep = جدید AstUnpackArrayDType{
                        flp, m_u32DTypep,
                        جدید AstRange{flp, static_cast<صحیح>(vtx[i]->m_delayRingSize - 1), 0}};
                    v3Global.rootp()->typeTablep()->addTypesp(ringDTypep);
                } وگرنه {
                    // bit [size-1:0] ring;
                    ringDTypep = m_modp->findBitDType(vtx[i]->m_delayRingSize,
                                                      vtx[i]->m_delayRingSize, VSigning::UNSIGNED);
                }
                AstVar* ثابت ringp
                    = جدید AstVar{flp, VVarType::MODULETEMP, base + "_ring", ringDTypep};
                ringp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(ringp);
                vtx[i]->datap()->delayRingVarp = ringp;
                vtx[i]->datap()->delayRingCounts = storesCounts;
                AstVar* ثابت headp
                    = جدید AstVar{flp, VVarType::MODULETEMP, base + "_head",
                                 storesCounts ? m_u32DTypep : m_modp->findBitDType()};
                headp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(headp);
                vtx[i]->datap()->delayRingHeadVarp = headp;
                // int unsigned idx;
                AstVar* ثابت idxp
                    = جدید AstVar{regFlp, VVarType::MODULETEMP, base + "_idx", m_u32DTypep};
                idxp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(idxp);
                vtx[i]->datap()->delayRingIdxVarp = idxp;
                // int unsigned live_count;
                AstVar* ثابت live_شمار_Varp
                    = جدید AstVar{regFlp, VVarType::MODULETEMP, base + "_liveCount", m_u32DTypep};
                live_شمار_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(live_شمار_Varp);
                vtx[i]->datap()->delay_Ring_Live_شمار_Varp = live_شمار_Varp;
                AstVar* ثابت wrappedp = جدید AstVar{regFlp, VVarType::MODULETEMP,
                                                    base + "_wrapped", m_modp->findBitDType()};
                wrappedp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(wrappedp);
                vtx[i]->datap()->delayRingWrappedVarp = wrappedp;
                ادامه;
            }
            اگر (!vtx[i]->datap()->needsReg) ادامه;
            اگر (vtx[i] == graph.m_startVertexp || vtx[i]->m_isMatch) ادامه;
            ثابت std::string varName = baseName + "__s" + std::to_string(i);
            AstVar* ثابت varp
                = جدید AstVar{regFlp, VVarType::MODULETEMP, varName, m_modp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(varp);
            vtx[i]->datap()->stateVarp = varp;
        }

        // Build lowering context for phase sub-functions.
        LowerCtx c{flp,        senTreep,      disableExprp, matchCondp,
                   assertType, directiveType, killVarp,     graph};

        // Phase 1: Resolve combinational Links via fixed-point propagation.
        Ast_گره_عبارت* ثابت triggerExprp
            = isSeqEvent ? جدید AstConst{flp, AstConst::BitTrue{}}
                         : assertOnCond(flp, assertp->userType(), assertp->directive());
        resolveLinks(c, triggerExprp, شمار_Matches);
        VL_DO_DANGLING(triggerExprp->delete_درخت(), triggerExprp);

        // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
        emitStateRegisterNba(c);
        emitDelayRingUpdates(c, شمار_Matches);
        emitAndCombinerDoneLatchNba(c);
        emitKillAckNba(c);

        // Phase 3/3a/3b: Compute terminal match/reject signals (cleans up stateSig).
        ثابت SignalSet sigs
            = computeSignals(c, need_Thread_Fail_شمار, need_Throughout_Thread_Fail_شمار, شمار_Matches);

        // Strong s_always[m:n] end-of-simulation liveness: if any in-window state
        // is still set at $finish, the universal-quantifier window never completed
        // (IEEE 1800-2023 16.12.11 strong semantics). Fire the assertion failure
        // from a final block; V3Assert turns the DT_ERROR display into the standard
        // "Assertion failed in %m" message.
        // pending = |{strong state registers, strong delay live counts != 0};
        Ast_گره_عبارت* pendingp = nullptr;
        برای (صحیح i = 0; i < N; ++i) {
            اگر (!vtx[i]->m_strongPending) ادامه;
            Ast_گره_عبارت* pendingExprp = nullptr;
            اگر (vtx[i]->datap()->stateVarp) {
                pendingExprp = جدید AstVarRef{flp, vtx[i]->datap()->stateVarp, VAccess::READ};
            } وگرنه اگر (vtx[i]->m_strongAlwaysRing) {
                pendingExprp
                    = delayRingHasLiveThreadsp(flp, vtx[i]->datap()->delay_Ring_Live_شمار_Varp);
            } وگرنه {
                ادامه;
            }
            اگر (!pendingp) {
                pendingp = pendingExprp;
            } وگرنه {
                pendingp = جدید AstLogOr{flp, pendingp, pendingExprp};
            }
        }
        اگر (pendingp) {
            AstCExpr* ثابت assertOnp
                = جدید AstCExpr{flp, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()", 1};
            Ast_گره_عبارت* ثابت condp = جدید AstLogAnd{flp, assertOnp, pendingp};
            AstDisplay* ثابت dispp
                = جدید AstDisplay{flp, VDisplayType::DT_ERROR, "", nullptr, nullptr};
            dispp->fmtp()->timeunit(m_modp->timeunit());
            Ast_گره_Stmt* ثابت firep = dispp;
            اگر (v3Global.opt.stopFail()) firep->addNext(جدید AstStop{flp, نادرست});
            m_modp->addStmtsp(جدید AstFinal{flp, جدید AstIf{flp, condp, firep}});
        }

        // Clear userp on every vertex before vertexData is destroyed.
        برای (صحیح i = 0; i < N; ++i) vtx[i]->userp(nullptr);
        بازگشت sigs;
    }
};

}  // namespace

//######################################################################
// Top-level visitor

رده AssertNfaVisitor final : عمومی VNVisitor {
    // STATE
    Ast_گره_Module* m_modp = nullptr;  // Current module being processed
    AstClocking* m_defaultClockingp = nullptr;  // Default clocking
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff
    SvaNfaLowering* m_loweringp = nullptr;  // NFA-to-hardware lowering engine
    Ast_Sen_درخت* m_sampled_مقدار_Clockp = nullptr;  // Inherited clock during scoped attachment
    V3UniqueNames m_propVarNames{"__Vpropvar"};  // Property-local variable names
    V3UniqueNames m_propTempNames{"__VnfaSampled"};  // Hoisted $sampled(propp) temps
    V3UniqueNames m_fail_شمار_Names{"__VnfaRemainingFailCount"};  // Fail replay counter names
    std::set<ثابت AstProperty*> m_inliningProps;  // Recursion guard for inlineNamedProperty

    template <typename T_گره>
    تهی visit_Sampled_مقدار(T_گره* ثابت nodep) {
        اگر (m_sampled_مقدار_Clockp && !nodep->sentreep()) {
            nodep->sentreep(m_sampled_مقدار_Clockp->clone_درخت(درست));
        }
        iterateChildren(nodep);
    }

    // Wire match vertex and mid-window sources for a successful NFA build.
    ایستا تهی wireMatchAndMidSources(SvaGraph& graph, ثابت BuildResult& result, File_سطر* flp) {
        graph.createMatchVertex();
        // Skip the main term Link when midSources already cover every
        // end-of-match (cover_sequence path); otherwise the per-mid extraction
        // double-counts via the merge vertex.
        اگر (!result.termIsMidMerge) { graph.addLink(result.termVertexp, graph.m_matchVertexp); }
        برای (SvaStateVertex* srcVtxp : result.midSources) {
            Ast_گره_عبارت* condp = nullptr;
            برای (Ast_گره_عبارت* ثابت tc : srcVtxp->m_throughoutConds) {
                Ast_گره_عبارت* ثابت tcClone = tc->clone_درخت_Pure(نادرست);
                condp = condp ? جدید AstLogAnd{flp, condp, tcClone} : tcClone;
            }
            graph.addLink(srcVtxp, graph.m_matchVertexp, condp);
            srcVtxp->m_isUnbounded = درست;
        }
    }

    ایستا Ast_گره_عبارت* getSequenceBodyExprp(ثابت AstSequence* seqp) {
        Ast_گره* bodyp = seqp->stmtsp();
        وقتی (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        بازگشت VN_CAST(bodyp, گره_عبارت);
    }

    ایستا AstPropSpec* getPropertySpecp(ثابت AstProperty* propp) {
        Ast_گره* stmtp = propp->stmtsp();
        // V3LinkParse emits InitialStaticStmt for property-local variable
        // initialisers; the InitialAutomaticStmt variant only appears for
        // task/function-scope automatic lifetime, not properties.
        وقتی (stmtp
               && (VN_IS(stmtp, Var) || VN_IS(stmtp, InitialStaticStmt)
                   || VN_IS(stmtp, InitialAutomaticStmt))) {  // LCOV_EXCL_LINE
            stmtp = stmtp->nextp();
        }
        بازگشت VN_CAST(stmtp, PropSpec);
    }

    تهی inlineNamedProperty(AstPropSpec* outerSpecp, AstFuncRef* funcrefp,
                             ثابت AstProperty* propyp) {
        // Recursion guard: IEEE 1800-2023 16.12.1 forbids recursive properties.
        // V3Width emits "Recursive property call" for direct recursion before this
        // pass runs; this catches any nested-inlining cycle that slips past.
        اگر (!m_inliningProps.emplace(propyp).second) {
            funcrefp->v3error("Illegal recursive property reference");  // LCOV_EXCL_LINE
            بازگشت;  // LCOV_EXCL_LINE
        }
        ساختار Guard final {
            std::set<ثابت AstProperty*>& setr;
            ثابت AstProperty* keyp;
            ~Guard() { setr.erase(keyp); }
        } guard{m_inliningProps, propyp};
        AstPropSpec* propSpecp = getPropertySpecp(propyp);
        UASSERT_OBJ(propSpecp, funcrefp, "Property has no body PropSpec");
        propSpecp = propSpecp->clone_درخت(نادرست);

        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propyp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_گره_عبارت*> portMap;
        برای (ثابت auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables to module-level temps (IEEE 16.10).
        std::unordered_map<ثابت AstVar*, AstVar*> localVarMap;
        برای (Ast_گره* stmtp = propyp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                اگر (!varp->isIO()) {
                    ثابت string newName = m_propVarNames.get(varp);
                    AstVar* ثابت newVarp = جدید AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        propSpecp->foreach([&](AstVarRef* refp) {
            ثابت auto portIt = portMap.find(refp->varp());
            اگر (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_درخت(نادرست));
                VL_DO_DANGLING(افزودن_Deletep(refp), refp);
                بازگشت;
            }
            ثابت auto localIt = localVarMap.find(refp->varp());
            اگر (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        برای (ثابت auto& tconnect : tconnects) {
            افزودن_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propSpecp, funcrefp);

        // Merge disable iff (IEEE 1800-2023 16.12.1)
        اگر (outerSpecp->disablep() && propSpecp->disablep()) {
            outerSpecp->v3error("disable iff expression before property call "
                                "and in its body is not legal");
            افزودن_Deletep(propSpecp->disablep()->unlinkFrBack());
        }
        اگر (outerSpecp->disablep()) {
            propSpecp->disablep(outerSpecp->disablep()->unlinkFrBack());
        }

        اگر (outerSpecp->sensesp() && propSpecp->sensesp()) {
            outerSpecp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Clock event before property call and in its body");
            افزودن_Deletep(propSpecp->sensesp()->unlinkFrBack());
        }
        اگر (outerSpecp->sensesp()) {
            AstSenItem* ثابت sensesp = outerSpecp->sensesp();
            sensesp->unlinkFrBack();
            propSpecp->sensesp(sensesp);
        }

        outerSpecp->replaceWith(propSpecp);
        VL_DO_DANGLING(افزودن_Deletep(outerSpecp), outerSpecp);
    }

    تهی inlineSequenceRef(AstFuncRef* funcrefp, AstSequence* seqp) {
        Ast_گره_عبارت* ثابت bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        Ast_گره_عبارت* ثابت clonedp = bodyExprp->clone_درخت(نادرست);

        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_گره_عبارت*> portMap;
        برای (ثابت auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ثابت auto it = portMap.find(refp->varp());
            اگر (it != portMap.end()) {
                refp->replaceWith(it->second->clone_درخت(نادرست));
                VL_DO_DANGLING(افزودن_Deletep(refp), refp);
            }
        });
        برای (ثابت auto& tconnect : tconnects) {
            افزودن_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(افزودن_Deletep(funcrefp), funcrefp);
        // Clear referenced flag so V3AssertPre cleanup does not emit
        // spurious UNSUPPORTED for sequences that were already inlined here.
        seqp->isReferenced(نادرست);
    }

    // Must run before hasMultiCycleExpr() so NFA sees sequence bodies.
    تهی inlineAllSequenceRefs(Ast_گره* rootp) {
        منطقی changed = درست;
        وقتی (changed) {
            changed = نادرست;
            rootp->foreach([&](AstFuncRef* funcrefp) {
                اگر (changed) بازگشت;
                اگر (AstSequence* ثابت seqp = VN_CAST(funcrefp->taskp(), Sequence)) {
                    inlineSequenceRef(funcrefp, seqp);
                    changed = درست;
                }
            });
        }
    }

    ایستا منطقی has_Multi_Cycle_عبارت(ثابت Ast_گره* nodep) {
        بازگشت nodep->exists([](ثابت Ast_گره* np) {
            اگر (ثابت auto* ثابت ep = VN_CAST(np, گره_عبارت)) بازگشت ep->isMultiCycleSva();
            بازگشت نادرست;
        });
    }

    ایستا VPropStrength effectiveAssertPropStrength(ثابت AstPropSpec* ثابت propSpecp) {
        اگر (propSpecp->propStrength() != VPropStrength::DEFAULT) بازگشت propSpecp->propStrength();
        بازگشت propSpecp->fileline()->language() <= V3LangCode::L1800_2005 ? VPropStrength::STRONG
                                                                           : VPropStrength::WEAK;
    }

    // Bare `assert property (p until q)` with boolean operands stays on
    // V3AssertPre's AstLoop lowering, which preserves per-attempt action-block
    // firings that this NFA's single-bit aggregated state cannot. Strong bare
    // forms are also lowered there. NFA still owns sequence operands and any
    // embedding inside a multi-cycle context (implication consequent, or/and
    // operands, etc.).
    ایستا منطقی is_Bare_بالاترین_Level_Until(Ast_گره* propp) {
        Ast_گره* p = propp;
        اگر (AstPropSpec* ثابت specp = VN_CAST(p, PropSpec)) p = specp->propp();
        وقتی (AstLogNot* ثابت notp = VN_CAST(p, LogNot)) p = notp->lhsp();
        AstUntil* ثابت untilp = VN_CAST(p, Until);
        اگر (!untilp) بازگشت نادرست;
        ثابت auto hasSeq = [](ثابت Ast_گره_عبارت* ep) {
            بازگشت ep->exists([](ثابت Ast_گره_عبارت* np) { بازگشت np->isMultiCycleSva(); });
        };
        اگر (hasSeq(untilp->lhsp()) || hasSeq(untilp->rhsp())) بازگشت نادرست;
        بازگشت درست;
    }

    ساختار PropertyParts final {
        Ast_گره_عبارت* triggerExprp = nullptr;
        Ast_گره_عبارت* seqExprp = nullptr;
        منطقی isOverlapped = درست;
        منطقی hasImplication = نادرست;
        منطقی isFollowedBy = نادرست;  // True for #-# / #=# (non-vacuous-fail on antecedent miss)
    };

    ایستا PropertyParts decomposeProperty(Ast_گره* propp) {
        PropertyParts parts;
        اگر (AstPropSpec* ثابت specp = VN_CAST(propp, PropSpec)) { propp = specp->propp(); }
        اگر (AstImplication* ثابت implp = VN_CAST(propp, Implication)) {
            parts.hasImplication = درست;
            parts.isOverlapped = implp->isOverlapped();
            parts.isFollowedBy = implp->isFollowedBy();
            parts.triggerExprp = implp->lhsp();
            parts.seqExprp = implp->rhsp();
        } وگرنه اگر (Ast_گره_عبارت* ثابت exprp = VN_CAST(propp, گره_عبارت)) {
            parts.triggerExprp = nullptr;
            parts.seqExprp = exprp;
        }
        بازگشت parts;
    }

    ایستا منطقی canSplitImplicationPassActions(ثابت PropertyParts& parts) {
        UASSERT(parts.hasImplication,
                "Implication pass action split requested without implication");
        UASSERT(parts.triggerExprp, "Implication pass action split requested without trigger");
        // Direct vacuous/nonvacuous classification uses the antecedent value in the current
        // assertion attempt. Leave delayed antecedents on the existing NFA pass path.
        بازگشت !has_Multi_Cycle_عبارت(parts.triggerExprp);
    }

    ایستا تهی splitImplicationPassActions(AstAssert* assertp, ثابت PropertyParts& parts,
                                            Ast_گره_عبارت* nonvacuousMatchp) {
        File_سطر* ثابت flp = assertp->fileline();

        Ast_گره* ثابت passsp = assertp->passsp()->unlinkFrBackWithNext();
        Ast_گره* splitsp = nullptr;

        اگر (!parts.isFollowedBy) {
            Ast_گره_عبارت* ثابت vacuousp
                = جدید AstLogNot{flp, sampled(parts.triggerExprp->clone_درخت_Pure(نادرست))};
            Ast_گره* ثابت vacuousBodyp = passsp->clone_درخت(نادرست);
            splitsp = newPassOnIf(flp, vacuousp, vacuousBodyp, assertp->userType(),
                                  assertp->directive(), /*vacuous=*/درست);
        }

        AstIf* ثابت nonvacuousp
            = newPassOnIf(flp, nonvacuousMatchp, passsp, assertp->userType(), assertp->directive(),
                          /*vacuous=*/نادرست);
        splitsp = splitsp ? Ast_گره::addNext<Ast_گره, Ast_گره>(splitsp, nonvacuousp)
                          : static_cast<Ast_گره*>(nonvacuousp);
        assertp->addPasssp(splitsp);
    }

    // On a PropSpec-wrapped assertion whose NFA build failed with a semantic
    // error (errorEmitted), replace the body with a BitFalse const so later
    // passes see a well-formed AST. Returns true if replaced.
    تهی replaceBodyOnBuildError(File_سطر* flp, AstPropSpec* propSpecp, منطقی errorEmitted) {
        اگر (!errorEmitted) بازگشت;
        Ast_گره* ثابت innerPropp = propSpecp->propp();
        innerPropp->replaceWith(جدید AstConst{flp, AstConst::BitFalse{}});
        VL_DO_DANGLING(افزودن_Deletep(innerPropp), innerPropp);
    }

    // Hoist a leading clocking event (IEEE 1800-2023 16.7):
    منطقی hoistClockedSeq(AstPropSpec* specp) {
        وقتی (AstSClocked* ثابت clockedp = VN_CAST(specp->propp(), SClocked)) {
            اگر (specp->sensesp()) {
                clockedp->v3warn(E_UNSUPPORTED, "Unsupported: multiclocked sequence or property");
                replaceBodyOnBuildError(specp->fileline(), specp, درست);
                بازگشت درست;
            }
            برای (ثابت AstSenItem* sp = clockedp->sensesp(); sp;
                 sp = VN_CAST(sp->nextp(), SenItem)) {
                اگر (!sp->edgeType().anEdge()) {
                    clockedp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: non-edge clocking event on a sequence; "
                                     "use an edge such as @(posedge clk)");
                    replaceBodyOnBuildError(specp->fileline(), specp, درست);
                    بازگشت درست;
                }
            }
            specp->sensesp(clockedp->sensesp()->unlinkFrBackWithNext());
            Ast_گره_عبارت* ثابت bodyp = clockedp->exprp()->unlinkFrBack();
            clockedp->replaceWith(bodyp);
            VL_DO_DANGLING(افزودن_Deletep(clockedp), clockedp);
        }
        // A clocking event anywhere else in the sequence is not supported.
        ثابت AstSClocked* nestedp = nullptr;
        specp->propp()->foreach([&](ثابت AstSClocked* p) {
            اگر (!nestedp) nestedp = p;
        });
        اگر (nestedp) {
            nestedp->v3warn(E_UNSUPPORTED,
                            "Unsupported: clocking event inside sequence expression");
            replaceBodyOnBuildError(specp->fileline(), specp, درست);
            بازگشت درست;
        }
        بازگشت نادرست;
    }

    // Build the NFA graph for a property body, handling both the antecedent
    // |-> consequent and simple sequence cases. Returns the consequent/body
    // BuildResult (invalid on parse/build failure).
    BuildResult buildAssertionGraph(SvaNfaBuilder& builder, SvaGraph& graph, Ast_گره_عبارت* seqBodyp,
                                    ثابت PropertyParts& parts, File_سطر* flp) {
        اگر (!parts.hasImplication) بازگشت builder.build(seqBodyp);

        graph.m_startVertexp = graph.createStateVertex();
        بازگشت builder.buildImplicationEdges(parts.triggerExprp, seqBodyp, graph.m_startVertexp,
                                             parts.isOverlapped, parts.isFollowedBy,
                                             parts.triggerExprp, flp);
    }

    // Install pass-action gating and replay simultaneous per-thread failures.
    تهی attachActionHandlers(AstAssert* ثابت assertp, Ast_گره_عبارت* matchExprp,
                              Ast_Sen_درخت* ثابت threadFailReplaySenTreep,
                              Ast_گره_عبارت* ثابت threadFailCountp) {
        // Gate pass handler on match to prevent vacuous-pass firings.
        اگر (matchExprp) {
            // needMatch implies passsp() was non-null when evaluated above;
            // lowering does not mutate the assert's pass-action between the
            // two reads, so passsp() is still non-null here.
            Ast_گره* passsp = assertp->passsp();
            UASSERT_OBJ(passsp, assertp, "needMatch set but passsp is null");
            passsp->unlinkFrBackWithNext();
            File_سطر* ثابت flp = assertp->fileline();
            AstIf* ثابت ifp = جدید AstIf{flp, matchExprp, passsp, nullptr};
            assertp->addPasssp(ifp);
            // Fail-handler prefix for overlapping instances (IEEE 16.12):
            // fires when reject=1 && match=1 in the same cycle.
            اگر (assertp->failsp()) assertp->prependFailsp(ifp->clone_درخت(نادرست));
        }

        اگر (threadFailCountp) {
            UASSERT_OBJ(threadFailReplaySenTreep, assertp,
                        "Thread fail count missing sensitivity tree");
            Ast_گره* ثابت failsp = assertp->failsp();
            File_سطر* ثابت flp = assertp->fileline();
            // IEEE 1800-2023 16.12 requires one action-block evaluation per failed
            // thread. AstAssert handles the first, so replay the rest here.
            AstVar* ثابت remaining_Fail_شمار_Varp
                = جدید AstVar{flp, VVarType::MODULETEMP, m_fail_شمار_Names.get(""),
                             m_modp->findBasicDType(VBasicDTypeKwd::UINT32)};
            remaining_Fail_شمار_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remaining_Fail_شمار_Varp);
            Ast_گره* ثابت replayStmtsp = جدید AstAssign{
                flp, جدید AstVarRef{flp, remaining_Fail_شمار_Varp, VAccess::WRITE}, threadFailCountp};
            AstLoop* ثابت replayLoopp = جدید AstLoop{flp};
            replayLoopp->addStmtsp(جدید AstLoopTest{
                flp, replayLoopp,
                جدید AstGt{flp, جدید AstVarRef{flp, remaining_Fail_شمار_Varp, VAccess::READ},
                          newTypedConstp(flp, remaining_Fail_شمار_Varp->dtypep(), 1)}});
            replayLoopp->addStmtsp(newIfAssertFailOn(failsp->clone_درخت(درست), assertp->directive(),
                                                     assertp->userType()));
            AstSub* ثابت decrementedFailCountp
                = جدید AstSub{flp, جدید AstVarRef{flp, remaining_Fail_شمار_Varp, VAccess::READ},
                             newTypedConstp(flp, remaining_Fail_شمار_Varp->dtypep(), 1)};
            replayLoopp->addStmtsp(
                جدید AstAssign{flp, جدید AstVarRef{flp, remaining_Fail_شمار_Varp, VAccess::WRITE},
                              decrementedFailCountp});
            replayStmtsp->addNext(replayLoopp);
            m_modp->addStmtsp(
                جدید AstAlways{flp, VAlwaysKwd::ALWAYS, threadFailReplaySenTreep, replayStmtsp});
        }
    }

    // Replace one VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). No-op if refp is not in matchMap.
    تهی substituteMatchItemRef(AstVarRef* refp, unsigned K,
                                ثابت std::unordered_map<ثابت AstVar*, Ast_گره_عبارت*>& matchMap) {
        ثابت auto it = matchMap.find(refp->varp());
        اگر (it == matchMap.end()) بازگشت;
        Ast_گره_عبارت* newp = it->second->clone_درخت_Pure(نادرست);
        اگر (K > 0) {
            AstConst* ثابت ticksp = جدید AstConst{refp->fileline(), AstConst::Widthed_مقدار{}, 32,
                                                  static_cast<uint32_t>(K)};
            AstPast* ثابت pastp = جدید AstPast{refp->fileline(), newp, ticksp, nullptr,
                                               /* propertyTiming */ درست};
            pastp->dtypeFrom(newp);
            newp = pastp;
        }
        refp->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(refp), refp);
        بازگشت;
    }

    // Recursively walk a consequent. Returns cycle length consumed and
    // substitutes each VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). Reports E_UNSUPPORTED on non-constant
    // delays or composite sequence operators and returns -1 on failure.
    صحیح
    walk_Substitute_Match_اقلام(Ast_گره_عبارت* nodep, unsigned K,
                             ثابت std::unordered_map<ثابت AstVar*, Ast_گره_عبارت*>& match_اقلام) {
        اگر (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            // IEEE 1800-2023 16.9.2: cycle_delay's lhsp is a constant_expression
            // and the delay form in a sequence is always `##N`, folded by
            // V3Const + V3Param before V3AssertNfa. Range form `##[m:n]` is the
            // only user-visible reject here.
            AstDelay* ثابت delayp = VN_AS(sexprp->delayp(), Delay);
            UASSERT_OBJ(delayp->isCycleDelay() && VN_IS(delayp->lhsp(), Const), sexprp,
                        "SVA cycle delay must have a constant lhsp");
            اگر (delayp->isRangeDelay()) {
                sexprp->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                              "non-constant cycle delay in consequent"
                                              " (IEEE 1800-2023 16.10)");
                بازگشت -1;
            }
            ثابت unsigned delayCycles = VN_AS(delayp->lhsp(), Const)->toUInt();
            صحیح preLen = 0;
            اگر (Ast_گره_عبارت* ثابت prep = sexprp->preExprp()) {
                preLen = walk_Substitute_Match_اقلام(prep, K, match_اقلام);
                اگر (preLen < 0) بازگشت -1;
            }
            ثابت صحیح bodyLen
                = walk_Substitute_Match_اقلام(sexprp->exprp(), K + preLen + delayCycles, match_اقلام);
            اگر (bodyLen < 0) بازگشت -1;
            بازگشت preLen + delayCycles + bodyLen;
        }
        اگر (nodep->isMultiCycleSva()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                         "composite sequence operator in consequent"
                                         " (IEEE 1800-2023 16.10)");
            بازگشت -1;
        }
        std::vector<AstVarRef*> refs;
        nodep->foreach([&refs](AstVarRef* p) { refs.افزودن_back(p); });
        برای (AstVarRef* ثابت refp : refs) substituteMatchItemRef(refp, K, match_اقلام);
        بازگشت 0;
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
    منطقی liftMatchItemSubstitutions(PropertyParts& parts, Ast_گره_عبارت* seqBodyp) {
        اگر (!parts.hasImplication) بازگشت نادرست;
        Ast_عبارت_Stmt* ثابت عبارت_Stmtp = VN_CAST(parts.triggerExprp, عبارت_Stmt);
        اگر (!عبارت_Stmtp) بازگشت نادرست;
        // IEEE 1800-2023 16.10 BNF requires `(expr, match_item {, match_item})`
        // with at least one match item; V3LinkParse only emits ExprStmt for
        // this form and only emits AstAssign with VarRef LHS for each item.
        std::unordered_map<ثابت AstVar*, Ast_گره_عبارت*> match_اقلام;
        برای (Ast_گره* stmtp = عبارت_Stmtp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstAssign* ثابت assignp = VN_AS(stmtp, Assign);
            AstVarRef* ثابت lhsRefp = VN_AS(assignp->lhsp(), VarRef);
            match_اقلام[lhsRefp->varp()] = assignp->rhsp();
        }
        ثابت unsigned startK = parts.isOverlapped ? 0 : 1;
        ثابت صحیح length = walk_Substitute_Match_اقلام(seqBodyp, startK, match_اقلام);
        // Match-item substitution / strip mutates ancestor purity. Release
        // builds don't auto-clear caches on edits, so refresh here.
        VIsCached::clear_Cache_درخت();
        اگر (length < 0) بازگشت درست;
        Ast_گره_عبارت* ثابت antBoolp = عبارت_Stmtp->resultp()->unlinkFrBack();
        عبارت_Stmtp->replaceWith(antBoolp);
        VL_DO_DANGLING(افزودن_Deletep(عبارت_Stmtp), عبارت_Stmtp);
        parts.triggerExprp = antBoolp;
        بازگشت نادرست;
    }

    // Outcome counts for a property if/case are wrong in an outcome-multiplying
    // context. Returns that context, or nullptr when the shape is supported.
    ایستا ثابت نویسه* unsupportedPropertyControl(ثابت Ast_گره_Cover_Or_Assert* assertp,
                                                  ثابت Ast_گره_عبارت* seqBodyp, منطقی negated) {
        اگر (!hasPropertyControlConjunction(seqBodyp)) بازگشت nullptr;
        اگر (negated) بازگشت "negation";
        اگر (VN_IS(assertp, Cover)) بازگشت "cover";
        اگر (VN_AS(assertp, Assert)->passsp()) بازگشت "a pass action";
        بازگشت nullptr;
    }

    // Inline property/sequence refs and reject unsupported shapes.
    // Returns the PropSpec to lower, or nullptr when fully handled here.
    AstPropSpec* prepareAssertionProp(Ast_گره_Cover_Or_Assert* assertp) {
        اگر (AstPropSpec* ثابت specp = VN_CAST(assertp->propp(), PropSpec)) {
            اگر (AstFuncRef* ثابت funcrefp = VN_CAST(specp->propp(), FuncRef)) {
                اگر (ثابت AstProperty* ثابت propyp = VN_CAST(funcrefp->taskp(), Property)) {
                    inlineNamedProperty(specp, funcrefp, propyp);
                }
            }
        }

        inlineAllSequenceRefs(assertp->propp());

        اگر (AstPropSpec* ثابت specp = VN_CAST(assertp->propp(), PropSpec)) {
            اگر (hoistClockedSeq(specp)) بازگشت nullptr;
        }

        AstPropSpec* ثابت propp = VN_AS(assertp->propp(), PropSpec);
        اگر (!VN_IS(assertp, Cover)
            && effectiveAssertPropStrength(propp) == VPropStrength::STRONG) {
            propp->v3warn(E_UNSUPPORTED,
                          "Unsupported: strong property in " + assertp->verilogKwd() + ".");
            replaceBodyOnBuildError(assertp->fileline(), propp, /*errorEmitted=*/درست);
            بازگشت nullptr;
        }

        اگر (!has_Multi_Cycle_عبارت(propp)) بازگشت nullptr;
        // A nested property instance keeps its body behind the call; lowering would drop it.
        اگر (propp->exists([](ثابت AstFuncRef* refp) { بازگشت VN_IS(refp->taskp(), Property); })) {
            assertp->v3warn(E_UNSUPPORTED,
                            "Unsupported: property instance inside a multi-cycle property "
                            "expression");
            VL_DO_DANGLING(افزودن_Deletep(assertp->unlinkFrBack()), assertp);
            بازگشت nullptr;
        }
        اگر (is_Bare_بالاترین_Level_Until(propp)) بازگشت nullptr;
        بازگشت propp;
    }

    تهی processAssertion(Ast_گره_Cover_Or_Assert* assertp) {
        اگر (assertp->immediate()) بازگشت;

        AstPropSpec* ثابت propp = prepareAssertionProp(assertp);
        اگر (!propp) بازگشت;

        PropertyParts parts = decomposeProperty(propp);
        UASSERT_OBJ(parts.seqExprp, propp, "Property body must be an expression");

        // Unwrap `not` (IEEE 1800-2023 16.12.1); odd count -> negated semantics.
        Ast_گره_عبارت* seqBodyp = parts.seqExprp;
        منطقی negated = نادرست;
        وقتی (AstLogNot* ثابت notp = VN_CAST(seqBodyp, LogNot)) {
            negated = !negated;
            seqBodyp = notp->lhsp();
        }

        ثابت نویسه* ثابت propertyControlp
            = unsupportedPropertyControl(assertp, seqBodyp, negated);

        // Substitute property-local match-item refs in consequent with
        // $past(rhs, K) before NFA build (IEEE 1800-2023 16.10).
        اگر (liftMatchItemSubstitutions(parts, seqBodyp)) {
            AstPropSpec* ثابت psp = VN_CAST(assertp->propp(), PropSpec);
            UASSERT_OBJ(psp, assertp, "Concurrent assertion must have PropSpec");
            replaceBodyOnBuildError(assertp->fileline(), psp, /*errorEmitted=*/درست);
            بازگشت;
        }

        AstCover* ثابت coverp = VN_CAST(assertp, Cover);
        ثابت منطقی isCoverSeq = coverp && coverp->isCoverSeq();
        // A sequence event control is not an assertion directive; no default
        // disable iff, no assertion control
        ثابت منطقی isSeqEvent = coverp && coverp->isSeqEvent();
        ثابت منطقی شمار_Matches = isCoverSeq && !isSeqEvent;
        // Inherit module defaults (IEEE 14.12, 16.15) when assertion has none.
        اگر (!propp->sensesp() && m_defaultClockingp) {
            propp->sensesp(m_defaultClockingp->sentreep()->sensesp()->clone_درخت(درست));
        }
        اگر (!propp->disablep() && m_defaultDisablep && !isSeqEvent) {
            propp->disablep(m_defaultDisablep->condp()->clone_درخت_Pure(درست));
        }
        اگر (!propp->sensesp()) بازگشت;
        Ast_Sen_درخت* senTreep
            = جدید Ast_Sen_درخت{propp->fileline(), propp->sensesp()->clone_درخت(درست)};
        Ast_گره_عبارت* ثابت disableExprp = propp->disablep();

        // NFA lowering clones repeated operands and may hoist them into an
        // always_comb block. Resolve implicit sampled-value clocks first, while
        // the enclosing assertion clock is still available.
        {
            VL_RESTORER(m_sampled_مقدار_Clockp);
            m_sampled_مقدار_Clockp = senTreep;
            iterate(propp->propp());
        }

        File_سطر* ثابت flp = assertp->fileline();

        SvaGraph graph;
        SvaNfaBuilder builder{graph,      m_modp,     m_propTempNames,
                              isCoverSeq, isSeqEvent, coverp != nullptr};

        ثابت BuildResult result = buildAssertionGraph(builder, graph, seqBodyp, parts, flp);
        اگر (result.valid()) wireMatchAndMidSources(graph, result, flp);
        اگر (!result.valid()) {
            // Fall through to V3AssertPre for unsupported constructs; only
            // replace the body on real semantic errors. Any hoisted temps
            // from this attempt become orphan MODULETEMPs; V3Dead removes
            // them along with the dead always_comb driver.
            replaceBodyOnBuildError(flp, propp, result.errorEmitted);
            VL_DO_DANGLING(افزودن_Deletep(senTreep), senTreep);
            بازگشت;
        }
        // After the build, so a construct the builder rejects reports itself.
        اگر (propertyControlp) {
            seqBodyp->v3warn(E_UNSUPPORTED,
                             "Unsupported: temporal property if/case with " << propertyControlp);
            replaceBodyOnBuildError(flp, propp, /*errorEmitted=*/درست);
            VL_DO_DANGLING(افزودن_Deletep(senTreep), senTreep);
            بازگشت;
        }
        اگر (شمار_Matches && !v3Global.opt.coverageUser()) {
            SvaNfaBuilder::freeUnlinkedCondp(result.finalCondp);
            VL_DO_DANGLING(افزودن_Deletep(senTreep), senTreep);
            Ast_گره* ثابت innerPropp = propp->propp();
            innerPropp->replaceWith(جدید AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(افزودن_Deletep(innerPropp), innerPropp);
            بازگشت;
        }

        AstAssert* ثابت assertAssertp = VN_CAST(assertp, Assert);
        ثابت منطقی splitImplicationPasssp = assertAssertp && assertAssertp->passsp()
                                            && parts.hasImplication
                                            && canSplitImplicationPassActions(parts);
        ثابت منطقی needMatch = assertAssertp && assertAssertp->passsp()
                               && (!parts.hasImplication || splitImplicationPasssp);

        ثابت منطقی needsThreadFailReplay
            = assertAssertp && assertAssertp->failsp() && !parts.hasImplication;
        ثابت auto signals
            = m_loweringp->lower(assertp, graph, senTreep, result.finalCondp, disableExprp,
                                 needsThreadFailReplay, needsThreadFailReplay && !negated);
        Ast_گره_عبارت* matchExprp = nullptr;
        Ast_گره_عبارت* outputExprp = m_loweringp->assembleResult(
            assertp, negated, result.finalCondp, signals, needMatch ? &matchExprp : nullptr);
        اگر (شمار_Matches) {
            UASSERT_OBJ(signals.matchCountp, coverp, "Cover sequence missing match count");
            VL_DO_DANGLING(outputExprp->delete_درخت(), outputExprp);
            propp->matchCountp(signals.matchCountp);
            outputExprp = جدید AstNeq{flp, signals.matchCountp->clone_درخت_Pure(نادرست),
                                     newTypedConstp(flp, signals.matchCountp->dtypep(), 0)};
        }

        Ast_Sen_درخت* ثابت threadFailReplaySenTreep
            = signals.threadFailCountp ? senTreep->clone_درخت(نادرست) : nullptr;

        VL_DO_DANGLING(افزودن_Deletep(senTreep), senTreep);
        اگر (result.finalCondp && !result.finalCondp->backp()) افزودن_Deletep(result.finalCondp);

        اگر (splitImplicationPasssp) {
            splitImplicationPassActions(assertAssertp, parts, matchExprp);
        } وگرنه {
            attachActionHandlers(assertAssertp, matchExprp, threadFailReplaySenTreep,
                                 signals.threadFailCountp);
        }

        Ast_گره* ثابت innerPropp = propp->propp();
        innerPropp->replaceWith(outputExprp);
        VL_DO_DANGLING(افزودن_Deletep(innerPropp), innerPropp);

        UINFO(4, "NFA converted assertion at " << flp << endl);

        اگر (dumpGraphLevel() >= 6) graph.m_graph.dumpDotFilePrefixed("assert-nfa");
    }

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_loweringp);
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultDisablep);
        m_modp = nodep;
        // A class nested in a module is in the scope of the module's default clocking
        اگر (!VN_IS(nodep, Class)) m_defaultClockingp = nullptr;
        m_defaultDisablep = nodep->defaultDisablep();
        SvaNfaLowering lowering{nodep};
        m_loweringp = &lowering;
        iterateChildren(nodep);
    }
    تهی visit(AstClocking* nodep) override {
        اگر (nodep->isDefault() && !m_defaultClockingp) m_defaultClockingp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    تهی visit(AstDefaultDisable* nodep) override {}
    تهی visit(AstFell* nodep) override { visit_Sampled_مقدار(nodep); }
    تهی visit(AstPast* nodep) override { visit_Sampled_مقدار(nodep); }
    تهی visit(AstRose* nodep) override { visit_Sampled_مقدار(nodep); }
    تهی visit(AstStable* nodep) override { visit_Sampled_مقدار(nodep); }
    تهی visit(AstAssert* nodep) override { processAssertion(nodep); }
    تهی visit(AstCover* nodep) override { processAssertion(nodep); }
    تهی visit(AstRestrict* nodep) override {
        // Restrict property is ignored by simulators (IEEE 1800-2023 16.12.2).
        // Remove here so temporal SExpr don't leak to V3AssertPre.
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }
    تهی visit(AstAssertIntrinsic* nodep) override {}
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    explicit AssertNfaVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Top entry point

تهی V3AssertNfa::assertNfaAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":" << endl);
    { AssertNfaVisitor{nodep}; }
    V3Global::dump_Check_Global_درخت("assertnfa", 0, dump_درخت_Either_Level() >= 3);
}
