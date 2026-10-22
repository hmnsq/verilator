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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3AssertNfa.h"

#ضمن "V3Assert.h"
#ضمن "V3Const.h"
#ضمن "V3Graph.h"
#ضمن "V3Stats.h"
#ضمن "V3Task.h"
#ضمن "V3UniqueNames.h"

#ضمن <set>
#ضمن <unordered_set>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// NFA Graph Data Structures (V3Graph-derived per upstream convention)

نطاق {

صنف SvaStateVertex;

// Per-vertex algorithm data, stored via V3GraphVertex::userp() during lowering
بنية SvaVertexData final {
    AstVar* stateVarp = nullptr;  // NBA state register for this vertex
    AstVar* delayRingVarp = nullptr;  // Packed occupancy bits or per-slot match counts
    AstVar* delayRingHeadVarp
        = nullptr;  // Cached outgoing slot; isolates readers from in-place writes
    AstVar* delayRingIdxVarp = nullptr;  // Next slot written in delayRingVarp
    AstVar* delay_Ring_Live_عدد_Varp = nullptr;  // Number of threads in delayRingVarp
    AstVar* delayRingWrappedVarp = nullptr;  // All slots written since the last clear
    AstVar* doneLVarp = nullptr;  // SAnd LHS done-latch
    AstVar* doneRVarp = nullptr;  // SAnd RHS done-latch
    Ast_عقدة_تعبير* stateSigp = nullptr;  // Combinational state signal; OWNED during lowering
    Ast_عقدة_تعبير* state_عدد_Sigp = nullptr;  // Cover-sequence match count; OWNED during lowering
    Ast_عقدة_تعبير* state_عدد_Overflow_Sigp = nullptr;  // Count overflow; OWNED during lowering
    منطقي delayRingCounts = خطأ;  // Ring slots hold cover-sequence match counts
    منطقي needsReg = خطأ;  // True if vertex has incoming clocked edge
};

// NFA state vertex -- one per NFA position in the sequence evaluation
صنف SvaStateVertex final : عام V3GraphVertex {
    VL_RTTI_IMPL(SvaStateVertex, V3GraphVertex)
عام:
    // True if this is the sequence-match terminal vertex
    منطقي m_isMatch = خطأ;
    // OWNED throughout-guard condition clones; IEEE 1800-2023 16.9.9
    std::vector<Ast_عقدة_تعبير*> m_throughoutConds;
    // Nonzero for a ring-buffer vertex for ## delays.
    منطقي m_isFixedDelayRing = خطأ;
    unsigned m_delayRingSize = 0;  // Number of ring slots. Range: max-min+1.
    Ast_عقدة_تعبير* m_delayRingClearCondp = nullptr;  // local RHS for pure-boolean range
    Ast_عقدة_تعبير* m_delayRingAdvanceCondp = nullptr;  // Advance only when this condition holds
    منطقي m_replayAbortReject = خطأ;  // Compressed repetition needs per-thread abort replay
    // OWNED; enclosing-abort fire condition clearing state or suppressing guard rejection
    Ast_عقدة_تعبير* m_abortClearp = nullptr;
    // OWNED; reject-abort fire condition rejecting all represented live threads
    Ast_عقدة_تعبير* m_abortRejectp = nullptr;
    // Liveness terminal (IEEE weak semantics): reject must not fire from this source
    منطقي m_isUnbounded = خطأ;
    // Temporal sequence AND combiner; IEEE 1800-2023 16.9.5
    منطقي m_isAndCombiner = خطأ;
    SvaStateVertex* m_andLhsTermp = nullptr;  // LHS sub-NFA terminal vertex
    SvaStateVertex* m_andRhsTermp = nullptr;  // RHS sub-NFA terminal vertex
    Ast_عقدة_تعبير* m_andLhsCondp = nullptr;  // OWNED; LHS final condition (may be nullptr)
    Ast_عقدة_تعبير* m_andRhsCondp = nullptr;  // OWNED; RHS final condition (may be nullptr)
    // Reject sink for SAnd rejectOnFail wiring; not a state-signal source
    منطقي m_isRejectSink = خطأ;
    // In-window vertex of a strong s_always[m:n]: if its state is still set at
    // end-of-simulation the universal-quantifier window never completed, which is
    // a liveness failure (IEEE 1800-2023 16.12.11 strong semantics).
    منطقي m_strongPending = خطأ;
    // Ring introduced for the checked portion of a bounded strong s_always window.
    منطقي m_strongAlwaysRing = خطأ;

    // CONSTRUCTORS
    explicit SvaStateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~SvaStateVertex() override {
        لكل (Ast_عقدة_تعبير* cp : m_throughoutConds) VL_DO_DANGLING(cp->delete_شجرة(), cp);
        إذا (m_delayRingClearCondp)
            VL_DO_DANGLING(m_delayRingClearCondp->delete_شجرة(), m_delayRingClearCondp);
        إذا (m_delayRingAdvanceCondp)
            VL_DO_DANGLING(m_delayRingAdvanceCondp->delete_شجرة(), m_delayRingAdvanceCondp);
        إذا (m_abortClearp) VL_DO_DANGLING(m_abortClearp->delete_شجرة(), m_abortClearp);
        إذا (m_abortRejectp) VL_DO_DANGLING(m_abortRejectp->delete_شجرة(), m_abortRejectp);
        إذا (m_andLhsCondp) VL_DO_DANGLING(m_andLhsCondp->delete_شجرة(), m_andLhsCondp);
        إذا (m_andRhsCondp) VL_DO_DANGLING(m_andRhsCondp->delete_شجرة(), m_andRhsCondp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string name() ثابت override {
        string name = "s" + cvtToStr(color());
        إذا (m_delayRingSize) {
            name += "\\n";
            name += m_isFixedDelayRing ? "fixed chain " : "range chain ";
            name += cvtToStr(m_delayRingSize) + " slots";
        }
        أرجع name;
    }
    string dotColor() ثابت override {
        إذا (m_isMatch) أرجع "red";
        إذا (m_delayRingSize) أرجع "blue";
        إذا (m_isAndCombiner) أرجع "purple";
        أرجع "black";
    }
    // LCOV_EXCL_STOP
    // Access per-vertex algorithm data (valid only during lowering phase)
    SvaVertexData* datap() ثابت { أرجع static_cast<SvaVertexData*>(userp()); }
};

// NFA transition edge -- clocked (##1) or combinational link (##0)
صنف SvaTransEdge final : عام V3GraphEdge {
    VL_RTTI_IMPL(SvaTransEdge, V3GraphEdge)
عام:
    Ast_عقدة_تعبير* m_condp;  // Transition condition; nullptr = unconditional; OWNED
    منطقي m_consumesCycle;  // true = clocked edge (##1), false = link (##0/boolean)
    // Reject when source is active and condp is false; set only on
    // outermost required-step Link
    منطقي m_rejectOnFail = خطأ;
    // Optional dynamic condition vertex for m_rejectOnFail. Used when the
    // success condition is another NFA state rather than a static expression.
    SvaStateVertex* m_condVtxp = nullptr;

    // CONSTRUCTORS
    SvaTransEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* قمة, Ast_عقدة_تعبير* condp,
                 منطقي consumesCycle)
        : V3GraphEdge{graphp, fromp, قمة, /*weight=*/1}
        , m_condp{condp}
        , m_consumesCycle{consumesCycle} {}
    ~SvaTransEdge() override {
        إذا (m_condp) VL_DO_DANGLING(m_condp->delete_شجرة(), m_condp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string dotLabel() ثابت override { أرجع m_consumesCycle ? "##1" : "link"; }
    string dotStyle() ثابت override { أرجع m_consumesCycle ? "" : "dashed"; }
    // LCOV_EXCL_STOP
    // Typed accessors for NFA vertices
    SvaStateVertex* fromVtxp() ثابت { أرجع static_cast<SvaStateVertex*>(fromp()); }
    SvaStateVertex* toVtxp() ثابت { أرجع static_cast<SvaStateVertex*>(قمة()); }
};

// NFA graph container
صنف SvaGraph final {
عام:
    V3Graph m_graph;  // Owns all vertices and edges
    SvaStateVertex* m_startVertexp = nullptr;  // Trigger/start vertex
    SvaStateVertex* m_matchVertexp = nullptr;  // Sequence-match terminal vertex

    // Create a new state vertex
    SvaStateVertex* createStateVertex() { أرجع جديد SvaStateVertex{&m_graph}; }
    // Create the match terminal vertex
    SvaStateVertex* createMatchVertex() {
        SvaStateVertex* ثابت vtxp = createStateVertex();
        vtxp->m_isMatch = true;
        m_matchVertexp = vtxp;
        أرجع vtxp;
    }
    // Add a clocked transition edge (##1)
    SvaTransEdge* addClockedEdge(SvaStateVertex* fromp, SvaStateVertex* قمة,
                                 Ast_عقدة_تعبير* condp = nullptr) {
        أرجع جديد SvaTransEdge{&m_graph, fromp, قمة, condp, /*consumesCycle=*/true};
    }
    // Add a combinational link (##0 / boolean condition)
    SvaTransEdge* addLink(SvaStateVertex* fromp, SvaStateVertex* قمة,
                          Ast_عقدة_تعبير* condp = nullptr) {
        أرجع جديد SvaTransEdge{&m_graph, fromp, قمة, condp, /*consumesCycle=*/خطأ};
    }
    // Collect all edges into a flat vector for iteration.
    // Used by the lowering phase which needs global edge scans.
    std::vector<ثابت SvaTransEdge*> allEdges() ثابت {
        std::vector<ثابت SvaTransEdge*> result;
        لكل (ثابت V3GraphVertex& vtxr : m_graph.vertices()) {
            لكل (ثابت V3GraphEdge& edger : vtxr.outEdges()) {
                result.ادفع_back(static_cast<ثابت SvaTransEdge*>(&edger));
            }
        }
        أرجع result;
    }
};

//######################################################################
// Builder result: terminal vertex + optional final condition (match Link condition).
بنية BuildResult final {
    SvaStateVertex* termVertexp;  // Primary terminal; contributes to both match and reject
    Ast_عقدة_تعبير* finalCondp;  // nullptr = unconditional
    // Mid-window sources for range delays (pure boolean RHS): match-only (isUnbounded)
    std::vector<SvaStateVertex*> midSources;
    منطقي errorEmitted = خطأ;  // Builder already emitted specific error; skip generic
    // For cover_sequence: when true, midSources already enumerate every
    // end-of-match, so wireMatchAndMidSources must NOT add the main
    // termVtxp -> matchVertex Link (would double-count via the merge vertex).
    منطقي termIsMidMerge = خطأ;
    منطقي valid() ثابت { أرجع termVertexp != nullptr; }
    ساكن BuildResult fail(منطقي errored = خطأ) { أرجع {nullptr, nullptr, {}, errored}; }
    ساكن BuildResult failWithError() { أرجع {nullptr, nullptr, {}, true}; }
};

// Parser-marked SAnd of overlapped implications: a property if/else/case.
ساكن منطقي hasPropertyControlConjunction(ثابت Ast_عقدة_تعبير* nodep) {
    أرجع nodep->exists([](ثابت AstSAnd* andp) { أرجع andp->propertyControl(); });
}

ساكن AstConst* newTypedConstp(File_سطر* ثابت flp, ثابت Ast_عقدة_DType* ثابت dtypep,
                                ثابت uint32_t قيمة) {
    AstConst* ثابت constp = جديد AstConst{flp, AstConst::DTyped{}, dtypep};
    constp->رقم().setLong(قيمة);
    أرجع constp;
}

ساكن Ast_عقدة_تعبير* sampled(Ast_عقدة_تعبير* exprp) {
    أرجع جديد AstSampled{exprp->fileline(), exprp, exprp->dtypep(), true};
}

ساكن ثابت حرف* assertPassOnQuery(منطقي vacuous) {
    ساكن constexpr ثابت حرف* queries[2]
        = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
    أرجع queries[vacuous];
}

ساكن Ast_عقدة_تعبير* assertOnCond(File_سطر* flp, VAssertType type,
                                 VAssertDirectiveType directiveType) {
    إذا (!v3Global.opt.assertOn()) { أرجع جديد AstConst{flp, AstConst::BitFalse{}}; }
    أرجع جديد AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
}

ساكن Ast_عقدة_تعبير* assertKillGet(File_سطر* flp, VAssertType type,
                                  VAssertDirectiveType directiveType) {
    أرجع جديد AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_KILL", type, directiveType),
                        32};
}

ساكن string assertActionControlPrefix(VAssertDirectiveType directiveType) {
    ثابت صحيح controlled = !!(static_cast<صحيح>(directiveType)
                              & (static_cast<صحيح>(VAssertDirectiveType::ASSERT)
                                 | static_cast<صحيح>(VAssertDirectiveType::COVER)
                                 | static_cast<صحيح>(VAssertDirectiveType::ASSUME)));
    ثابت صحيح checkRuntime = controlled & static_cast<صحيح>(v3Global.opt.assertOn());
    أرجع "("s + std::to_string(controlled ^ 1) + " || ("s + std::to_string(checkRuntime)
           + " && "s;
}

ساكن Ast_عقدة_تعبير* assertPassOnCond(File_سطر* flp, VAssertType type,
                                     VAssertDirectiveType directiveType, منطقي vacuous) {
    أرجع جديد AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType)
            + "))"s,
        1};
}

ساكن Ast_عقدة_تعبير* assertFailOnCond(File_سطر* flp, VAssertType type,
                                     VAssertDirectiveType directiveType) {
    أرجع جديد AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType) + "))"s,
        1};
}

ساكن AstIf* newPassOnIf(File_سطر* flp, Ast_عقدة_تعبير* firep, Ast_عقدة* bodyp, VAssertType type,
                          VAssertDirectiveType directiveType, منطقي vacuous) {
    Ast_عقدة_تعبير* ثابت condp
        = جديد AstLogAnd{flp, firep, assertPassOnCond(flp, type, directiveType, vacuous)};
    AstIf* ثابت ifp = جديد AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(true);
    ifp->user1(true);
    أرجع ifp;
}

ساكن Ast_عقدة_Stmt* newIfAssertFailOn(Ast_عقدة* bodyp, VAssertDirectiveType directiveType,
                                      VAssertType type) {
    File_سطر* ثابت flp = bodyp->fileline();
    Ast_عقدة_تعبير* ثابت condp = assertFailOnCond(flp, type, directiveType);
    AstIf* ثابت ifp = جديد AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(true);
    ifp->user1(true);
    أرجع ifp;
}

//######################################################################
// NFA Builder

صنف SvaNfaBuilder final {
    SvaGraph& m_graph;  // NFA graph being built
    Ast_عقدة_Module* ثابت m_modp;  // Module to receive hoisted sampled-prop temps
    V3UniqueNames& m_propTempNames;  // Module-shared temp-var name source
    std::vector<Ast_عقدة_تعبير*> m_temporal_Guard_مكدس;  // Guards active across nested temporal states
    // Outer abort conditions, AND-ed as !cond into inner abort edges
    // (IEEE 1800-2023 16.12.14 outer-wraps-inner).
    std::vector<Ast_عقدة_تعبير*> m_outer_Abort_مكدس;
    منطقي m_inUnboundedScope = خطأ;  // Sticky: nodes created after inherit liveness
    منطقي m_markStrongPending = خطأ;  // Mark new vertices as strong s_always in-window
    منطقي m_isCover = خطأ;  // Cover directives do not fail at end-of-simulation
    // IEEE 1800-2023 16.14.3 cover sequence: each end-of-match fires the action,
    // not just the first. Builder builds parallel-branch (no first-match-wins)
    // topology when true. Default false preserves cover_property semantics.
    منطقي m_isCoverSeq = خطأ;
    // Unsupported endpoint topology must reject, not ignore, or the wait hangs
    منطقي m_isSeqEvent = خطأ;
    منطقي m_inSequencePrefix = خطأ;  // Prefix composition does not preserve every endpoint
    منطقي m_wantBoolLeaf = خطأ;  // Building a |-> / |=> or top-level body (see buildOrMerge)

    بنية RangeDelayRejectInfo final {
        SvaStateVertex* startp = nullptr;
        unsigned range = 0;
        صحيح rhsLen = 0;
    };

    فراغ warnEndpointUnsupported(File_سطر* flp, ثابت string& what) ثابت {
        إذا (m_isSeqEvent) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: sequence used as an event control with " << what);
        } وإلا {
            flp->v3warn(COVERIGN, "Ignoring unsupported: cover sequence with " << what);
        }
    }

    Ast_عقدة_تعبير* throughoutCond(Ast_عقدة_تعبير* baseCondp, File_سطر* flp) {
        إذا (m_temporal_Guard_مكدس.empty()) أرجع baseCondp;
        // AND all active temporal guards (supports nesting)
        // Each must use $sampled values.
        Ast_عقدة_تعبير* guardp = nullptr;
        لكل (Ast_عقدة_تعبير* ثابت condp : m_temporal_Guard_مكدس) {
            Ast_عقدة_تعبير* ثابت clonep = sampled(condp->clone_شجرة_Pure(خطأ));
            إذا (!guardp) {
                guardp = clonep;
            } وإلا {
                guardp = جديد AstLogAnd{flp, guardp, clonep};
            }
        }
        إذا (baseCondp) { guardp = جديد AstLogAnd{flp, baseCondp, guardp}; }
        أرجع guardp;
    }

    ساكن unsigned getConstUInt(Ast_عقدة_تعبير* exprp) {
        Ast_عقدة_تعبير* ثابت constp = V3Const::constifyEdit(exprp->clone_شجرة_Pure(خطأ));
        ثابت AstConst* ثابت cp = VN_CAST(constp, Const);
        ثابت unsigned val = cp ? cp->toUInt() : 0;
        VL_DO_DANGLING(constp->delete_شجرة(), constp);
        أرجع val;
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
    ساكن صحيح fixedLength(Ast_عقدة_تعبير* nodep) {
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
            إذا (!delayp || !delayp->isCycleDelay()) أرجع -1;
            unsigned delayCycles;
            إذا (delayp->isRangeDelay()) {
                إذا (delayp->isUnbounded()) أرجع -1;  // LCOV_EXCL_LINE
                ثابت unsigned minD = getConstUInt(delayp->lhsp());
                ثابت unsigned maxD = getConstUInt(delayp->rhsp());
                إذا (minD != maxD) أرجع -1;
                delayCycles = minD;
            } وإلا {
                delayCycles = getConstUInt(delayp->lhsp());
            }
            صحيح preLen = 0;
            إذا (Ast_عقدة_تعبير* ثابت prep = sexprp->preExprp()) {
                preLen = fixedLength(prep);
                إذا (preLen < 0) أرجع -1;  // LCOV_EXCL_LINE
            }
            ثابت صحيح bodyLen = fixedLength(sexprp->exprp());
            إذا (bodyLen < 0) أرجع -1;  // LCOV_EXCL_LINE
            أرجع preLen + delayCycles + bodyLen;
        }
        إذا (AstSThroughout* ثابت throughp = VN_CAST(nodep, SThroughout)) {
            أرجع fixedLength(throughp->rhsp());
        }
        إذا (AstSOr* ثابت orp = VN_CAST(nodep, SOr)) {
            // Alternatives must share one end cycle; buildSWithin relies on
            // this to pair the OR with an SIntersect.
            ثابت صحيح lhsLen = fixedLength(orp->lhsp());
            ثابت صحيح rhsLen = fixedLength(orp->rhsp());
            إذا (lhsLen < 0 || rhsLen < 0 || lhsLen != rhsLen) أرجع -1;
            أرجع lhsLen;
        }
        إذا (AstSWithin* ثابت withinp = VN_CAST(nodep, SWithin)) {
            // `seq1 within seq2` ends at seq2's end cycle (IEEE 16.9.10).
            ثابت صحيح lhsLen = fixedLength(withinp->lhsp());
            ثابت صحيح rhsLen = fixedLength(withinp->rhsp());
            إذا (lhsLen < 0 || rhsLen < 0 || lhsLen > rhsLen) أرجع -1;
            أرجع rhsLen;
        }
        // LCOV_EXCL_START -- defensive: V3AssertPre rejects composite SVA ops
        // nested in an intersect arm before fixedLength runs (clock-context
        // resolution fails). Kept as a guard in case future parser relaxations
        // permit it.
        إذا (nodep->isMultiCycleSva()) أرجع -1;
        // LCOV_EXCL_STOP
        // Plain boolean expression (no SVA constructs) -- 0 cycles.
        أرجع 0;
    }

    // Contiguous match-length range [lo,hi] for an operand whose length varies
    // from AT MOST ONE ranged cycle delay; {-1,-1} otherwise. Drives the
    // variable-length `intersect` lowering (IEEE 1800-2023 16.9.6): more than
    // one ranged delay would make the per-length realization ambiguous, so it
    // is reported unsupported rather than mis-paired.
    ساكن std::pair<صحيح, صحيح> lengthRange(Ast_عقدة_تعبير* nodep) {
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
            إذا (!delayp || !delayp->isCycleDelay()) أرجع {-1, -1};
            std::pair<صحيح, صحيح> delayRange;
            إذا (delayp->isRangeDelay()) {
                إذا (delayp->isUnbounded()) أرجع {-1, -1};
                ثابت unsigned minD = getConstUInt(delayp->lhsp());
                ثابت unsigned maxD = getConstUInt(delayp->rhsp());
                delayRange = {minD, maxD};
            } وإلا {
                ثابت unsigned d = getConstUInt(delayp->lhsp());
                delayRange = {d, d};
            }
            std::pair<صحيح, صحيح> preRange{0, 0};
            إذا (Ast_عقدة_تعبير* ثابت prep = sexprp->preExprp()) {
                preRange = lengthRange(prep);
                إذا (preRange.first < 0) أرجع {-1, -1};
            }
            ثابت std::pair<صحيح, صحيح> bodyRange = lengthRange(sexprp->exprp());
            إذا (bodyRange.first < 0) أرجع {-1, -1};
            ثابت صحيح variableParts = (preRange.first != preRange.second)
                                      + (delayRange.first != delayRange.second)
                                      + (bodyRange.first != bodyRange.second);
            إذا (variableParts > 1) أرجع {-1, -1};
            أرجع {preRange.first + delayRange.first + bodyRange.first,
                    preRange.second + delayRange.second + bodyRange.second};
        }
        إذا (AstSThroughout* ثابت throughp = VN_CAST(nodep, SThroughout)) {
            أرجع lengthRange(throughp->rhsp());
        }
        إذا (nodep->isMultiCycleSva()) أرجع {-1, -1};
        أرجع {0, 0};  // plain boolean -- 0 cycles
    }

    // Clone `operand` with its sole variable ranged cycle delay pinned so the
    // total match length is exactly `len`. `lo` is the operand's minimum length
    // (lengthRange().first). A fixed operand has no such delay and is returned
    // as a plain clone (callers only request its single achievable length).
    ساكن Ast_عقدة_تعبير* realizeAtLength(Ast_عقدة_تعبير* operand, صحيح len, صحيح lo) {
        Ast_عقدة_تعبير* ثابت clonep = operand->clone_شجرة_Pure(خطأ);
        AstDelay* rangeDelayp = nullptr;
        clonep->foreach([&](AstDelay* dp) {
            إذا (!rangeDelayp && dp->isRangeDelay() && !dp->isUnbounded()
                && getConstUInt(dp->lhsp()) != getConstUInt(dp->rhsp())) {
                rangeDelayp = dp;
            }
        });
        إذا (rangeDelayp) {
            File_سطر* ثابت flp = rangeDelayp->fileline();
            ثابت unsigned pinned = getConstUInt(rangeDelayp->lhsp()) + (len - lo);
            Ast_عقدة_تعبير* ثابت oldMinp = rangeDelayp->lhsp();
            oldMinp->replaceWith(جديد AstConst{flp, pinned});
            VL_DO_DANGLING(oldMinp->delete_شجرة(), oldMinp);
            // Drop the max bound so it lowers as a fixed `##d`, not `##[d:d]`.
            Ast_عقدة* ثابت oldMaxp = rangeDelayp->rhsp()->unlinkFrBack();
            VL_DO_DANGLING(oldMaxp->delete_شجرة(), oldMaxp);
        }
        أرجع clonep;
    }

    // Cuts AST size from O(N * sizeof(exprp)) to O(N) + O(sizeof(exprp)) by
    // sharing a single `VarRef` across N check edges. Hoist also matches the
    // IEEE 1800-2023 16.9.9 "single preponed-region snapshot" semantic for
    // any exprp -- even an impure one would now evaluate exactly once per
    // clock instead of N times. Orphan temps from failed builds are unused
    // MODULETEMPs and are removed by V3Dead.
    AstVar* tryHoistSampled(Ast_عقدة_تعبير* exprp, File_سطر* flp, unsigned clone_عدد) {
        constexpr unsigned kHoistThreshold = 2;
        إذا (clone_عدد < kHoistThreshold) أرجع nullptr;
        AstVar* ثابت tempVarp
            = جديد AstVar{flp, VVarType::MODULETEMP, m_propTempNames.get(exprp), exprp->dtypep()};
        m_modp->addStmtsp(tempVarp);
        AstAssign* ثابت assignp = جديد AstAssign{flp, جديد AstVarRef{flp, tempVarp, VAccess::WRITE},
                                                 sampled(exprp->clone_شجرة_Pure(خطأ))};
        m_modp->addStmtsp(جديد AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, assignp});
        أرجع tempVarp;
    }

    ساكن Ast_عقدة_تعبير* sampledRefOrClone(AstVar* hoistVarp, Ast_عقدة_تعبير* exprp, File_سطر* flp) {
        إذا (hoistVarp) أرجع جديد AstVarRef{flp, hoistVarp, VAccess::READ};
        أرجع sampled(exprp->clone_شجرة_Pure(خطأ));
    }

    // Create vertex and inherit temporal guards from the current scope.
    SvaStateVertex* scopedCreateVertex() {
        SvaStateVertex* ثابت vtxp = m_graph.createStateVertex();
        لكل (Ast_عقدة_تعبير* ثابت cp : m_temporal_Guard_مكدس) {
            vtxp->m_throughoutConds.ادفع_back(cp->clone_شجرة_Pure(خطأ));
        }
        إذا (m_inUnboundedScope) vtxp->m_isUnbounded = true;
        إذا (m_markStrongPending) vtxp->m_strongPending = true;
        أرجع vtxp;
    }

    // AND current temporal guards into every edge/link.
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* قمة, Ast_عقدة_تعبير* condp,
                              File_سطر* flp) {
        أرجع m_graph.addLink(fromp, قمة, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* قمة, File_سطر* flp) {
        أرجع m_graph.addLink(fromp, قمة, throughoutCond(nullptr, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* قمة, Ast_عقدة_تعبير* condp,
                              File_سطر* flp) {
        أرجع m_graph.addClockedEdge(fromp, قمة, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* قمة, File_سطر* flp) {
        أرجع m_graph.addClockedEdge(fromp, قمة, throughoutCond(nullptr, flp));
    }

    SvaStateVertex* addDelayChain(SvaStateVertex* startp, unsigned size, File_سطر* flp,
                                  منطقي isFixed = true, Ast_عقدة_تعبير* clearCondp = nullptr,
                                  Ast_عقدة_تعبير* advanceCondp = nullptr) {
        إذا (isFixed && size == 0) أرجع startp;
        UASSERT_OBJ(size > 0, startp, "Delay chain needs at least one slot");
        إذا (isFixed && size == 1 && !advanceCondp) {
            SvaStateVertex* ثابت nextp = scopedCreateVertex();
            guardedEdge(startp, nextp, flp);
            أرجع nextp;
        }
        SvaStateVertex* ثابت ringVtxp = scopedCreateVertex();
        ringVtxp->m_isFixedDelayRing = isFixed;
        ringVtxp->m_delayRingSize = size;
        إذا (clearCondp) {
            UASSERT_OBJ(!isFixed, startp, "Fixed delay cannot have a clear condition");
            ringVtxp->m_delayRingClearCondp = clearCondp->clone_شجرة_Pure(خطأ);
        }
        ringVtxp->m_delayRingAdvanceCondp = advanceCondp;
        إذا (isFixed) {
            guardedEdge(startp, ringVtxp, flp);
        } وإلا {
            guardedLink(startp, ringVtxp, flp);
        }
        أرجع ringVtxp;
    }

    // Build NFA for an SExpr. finalCond = RHS (not yet added as a vertex).
    // isTopLevelStep: marks outermost required boolean check as rejectOnFail.
    // Apply a range delay `##[M:N]` to currentp. Returns true on success. On
    // failure, emits a diagnostic and returns false.
    منطقي applyRangeDelay(AstSExpr* sexprp, SvaStateVertex*& currentp,
                         std::vector<SvaStateVertex*>& midSources,
                         RangeDelayRejectInfo* rangeRejectInfop = nullptr) {
        File_سطر* ثابت flp = sexprp->fileline();
        AstDelay* ثابت delayp = VN_AS(sexprp->delayp(), Delay);
        Ast_عقدة_تعبير* ثابت rhsExprp = sexprp->exprp();
        ثابت unsigned minDelay = getConstUInt(delayp->lhsp());
        إذا (delayp->isUnbounded()) {
            // `##[M:$]`: wait M cycles, then retain every matured attempt in a
            // one-slot ring while also exposing the first eligible tick.
            currentp = addDelayChain(currentp, minDelay, flp);
            SvaStateVertex* ثابت waitRingp = addDelayChain(currentp, 1, flp, خطأ);
            // The ring's live count is updated in NBA, so this link retains the previous
            // tick's live attempts alongside new arrivals for the next tick.
            guardedLink(waitRingp, waitRingp, flp);
            SvaStateVertex* ثابت mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(waitRingp, mergeVtxp, flp);
            currentp = mergeVtxp;
            currentp->m_isUnbounded = true;  // Liveness, so no reject.
            m_inUnboundedScope = true;
            أرجع true;
        }
        ثابت unsigned maxDelay = getConstUInt(delayp->rhsp());
        إذا (minDelay == maxDelay) {
            currentp = addDelayChain(currentp, minDelay, flp);
            أرجع true;
        }
        ثابت unsigned range = maxDelay - minDelay;
        currentp = addDelayChain(currentp, minDelay, flp);
        ثابت منطقي multiCycleRhs = rhsExprp->isMultiCycleSva();
        ثابت Ast_عقدة_تعبير* ثابت preExprp = sexprp->preExprp();
        // Prefix and nested-sequence composition can lose endpoints or their multiplicity
        // required by cover sequence (IEEE 1800-2023 16.14.3), regardless of the range width.
        إذا (m_isCoverSeq
            && (multiCycleRhs || m_inSequencePrefix
                || (preExprp && preExprp->isMultiCycleSva()))) {
            warnEndpointUnsupported(flp, "this ranged cycle delay");
            أرجع خطأ;
        }
        إذا (m_inSequencePrefix) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: Bounded ranged cycle delay in a sequence prefix");
            أرجع خطأ;
        }
        إذا (multiCycleRhs) {
            // Launch a candidate at every eligible tick. Candidate-local misses are not
            // assertion rejects while a later position can still match.
            إذا (rangeRejectInfop) {
                ثابت صحيح rhsLen = fixedLength(rhsExprp);
                إذا (rhsLen >= 0) *rangeRejectInfop = {currentp, range, rhsLen};
            }
            SvaStateVertex* ثابت tailp = addDelayChain(currentp, range + 1U, flp, خطأ);
            SvaStateVertex* ثابت mergeVtxp = scopedCreateVertex();
            mergeVtxp->m_isUnbounded = true;
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(tailp, mergeVtxp, flp);
            currentp = mergeVtxp;
            m_inUnboundedScope = true;
        } وإلا {
            // The first eligible tick is explicit. The ring retains attempts for the
            // remaining ticks and exposes the final tick for rejection. Only properties
            // clear on success, since cover sequence counts every endpoint.
            midSources.ادفع_back(currentp);
            currentp = addDelayChain(currentp, range + 1U, flp, خطأ,
                                     m_isCoverSeq ? nullptr : rhsExprp);
        }
        أرجع true;
    }

    فراغ addFiniteRangeReject(ثابت RangeDelayRejectInfo& info, ثابت BuildResult& result,
                              File_سطر* flp) {
        إذا (!info.startp) أرجع;

        SvaStateVertex* ثابت expiryVtxp
            = addDelayChain(info.startp, info.range + info.rhsLen, flp);
        SvaStateVertex* ثابت expiryMatchp = scopedCreateVertex();
        std::vector<SvaStateVertex*> sources = result.midSources;
        sources.ادفع_back(result.termVertexp);
        لكل (SvaStateVertex* ثابت srcp : sources) {
            Ast_عقدة_تعبير* ثابت condp
                = result.finalCondp ? sampled(result.finalCondp->clone_شجرة_Pure(خطأ)) : nullptr;
            SvaStateVertex* ثابت successNowp = scopedCreateVertex();
            guardedLink(srcp, successNowp, condp, flp);
            // Retain a success through the latest deadline it can satisfy.
            SvaStateVertex* ثابت historyp
                = addDelayChain(successNowp, info.range + 1U, flp, خطأ);
            guardedLink(successNowp, expiryMatchp, flp);
            guardedLink(historyp, expiryMatchp, flp);
        }

        SvaStateVertex* ثابت sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = true;
        SvaTransEdge* ثابت rejectp = m_graph.addLink(expiryVtxp, sinkVtxp);
        rejectp->m_rejectOnFail = true;
        rejectp->m_condVtxp = expiryMatchp;
    }

    BuildResult buildSExpr(AstSExpr* sexprp, SvaStateVertex* entryVtxp,
                           منطقي is_قمة_Level_Step = خطأ) {
        AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
        إذا (!delayp || !delayp->isCycleDelay()) أرجع BuildResult::fail();

        File_سطر* ثابت flp = sexprp->fileline();
        Ast_عقدة_تعبير* ثابت exprp = sexprp->exprp();

        // Handle LHS (preExpr)
        SvaStateVertex* currentp = entryVtxp;
        إذا (Ast_عقدة_تعبير* ثابت preExprp = sexprp->preExprp()) {
            VL_RESTORER(m_inSequencePrefix);
            m_inSequencePrefix = true;
            ثابت BuildResult pre = build_تعبير(preExprp, currentp, is_قمة_Level_Step);
            إذا (!pre.valid()) أرجع BuildResult::fail(pre.errorEmitted);  // LCOV_EXCL_LINE
            إذا (pre.finalCondp) {
                SvaStateVertex* ثابت condVtxp = scopedCreateVertex();
                SvaTransEdge* ثابت edgep = guardedLink(
                    pre.termVertexp, condVtxp, sampled(pre.finalCondp->clone_شجرة_Pure(خطأ)), flp);
                إذا (is_قمة_Level_Step && !pre.termVertexp->m_isUnbounded && !m_inUnboundedScope) {
                    // Do not mark liveness sources: first boolean check is deferred.
                    edgep->m_rejectOnFail = true;
                }
                freeUnlinkedCondp(pre.finalCondp);
                currentp = condVtxp;
            } وإلا {
                currentp = pre.termVertexp;
            }
        }

        // Handle delay
        std::vector<SvaStateVertex*> rangeMidSources;
        RangeDelayRejectInfo rangeRejectInfo;
        ثابت منطقي addRangeReject = is_قمة_Level_Step && !m_inUnboundedScope;
        إذا (delayp->isRangeDelay()) {
            إذا (!applyRangeDelay(sexprp, currentp, rangeMidSources,
                                 addRangeReject ? &rangeRejectInfo : nullptr)) {
                أرجع BuildResult::failWithError();
            }
        } وإلا {
            ثابت unsigned delayCycles = getConstUInt(delayp->lhsp());
            currentp = addDelayChain(currentp, delayCycles, flp);
        }

        // Multi-cycle RHS: recurse (only plain boolean is returned as finalCondp).
        إذا (exprp->isMultiCycleSva()) {
            ثابت BuildResult result = build_تعبير(exprp, currentp, is_قمة_Level_Step);
            إذا (result.valid()) addFiniteRangeReject(rangeRejectInfo, result, flp);
            أرجع result;
        }
        أرجع {currentp, exprp, std::move(rangeMidSources)};
    }

    BuildResult buildConsRep(AstSConsRep* repp, SvaStateVertex* entryVtxp,
                             منطقي is_قمة_Level_Step = خطأ) {
        File_سطر* ثابت flp = repp->fileline();
        Ast_عقدة_تعبير* ثابت exprp = repp->exprp();
        // Multi-cycle expr in ConsRep not yet supported; bail to avoid invalid AST.
        إذا (exprp->isMultiCycleSva()) {
            repp->v3warn(E_UNSUPPORTED, "Unsupported: multi-cycle sequence expression inside"
                                        " consecutive repetition (IEEE 1800-2023 16.9.2)");
            أرجع BuildResult::failWithError();
        }
        ثابت unsigned minN = getConstUInt(repp->countp());

        // Sum sites across prefix + unbounded/range tail so one hoist covers
        // every check edge of this repetition.
        unsigned totalSites = minN;
        إذا (repp->unbounded()) {
            totalSites += 1;
        } وإلا إذا (repp->maxCountp()) {
            totalSites += getConstUInt(repp->maxCountp()) - minN;
        }
        AstVar* ثابت hoistVarp = tryHoistSampled(exprp, flp, totalSites);

        // Cover-sequence (IEEE 1800-2023 16.14.3): collect each end-of-match
        // position so they all fire the action, not just the merged terminal.
        std::vector<SvaStateVertex*> consMidSources;

        SvaStateVertex* currentp = entryVtxp;
        لكل (unsigned i = 0; i < minN; ++i) {
            // Keep the first repetition explicit, collapse all remaining checks into the ring.
            إذا (i == 1) {
                currentp = addDelayChain(currentp, minN - 1, flp);
                currentp->m_delayRingClearCondp
                    = جديد AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                currentp->m_replayAbortReject = true;
                إذا (is_قمة_Level_Step) {
                    currentp->m_throughoutConds.ادفع_back(
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
            إذا (is_قمة_Level_Step && i == 0) linkp->m_rejectOnFail = true;
            currentp = condVtxp;
        }
        // After minN: currentp is the first valid end-of-match position for [*m:n].
        إذا (m_isCoverSeq && (repp->unbounded() || repp->maxCountp())) {
            consMidSources.ادفع_back(currentp);
        }

        إذا (repp->unbounded()) {
            إذا (minN == 0) {
                SvaStateVertex* ثابت waitVtxp = scopedCreateVertex();
                guardedEdge(currentp, waitVtxp, flp);
                SvaStateVertex* ثابت checkVtxp = scopedCreateVertex();
                guardedLink(waitVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedEdge(checkVtxp, waitVtxp, flp);
                guardedLink(currentp, checkVtxp, flp);
                currentp = checkVtxp;
            } وإلا {
                SvaStateVertex* ثابت loopBackVtxp = scopedCreateVertex();
                guardedEdge(currentp, loopBackVtxp, flp);
                SvaStateVertex* ثابت reCheckVtxp = scopedCreateVertex();
                guardedLink(loopBackVtxp, reCheckVtxp, sampledRefOrClone(hoistVarp, exprp, flp),
                            flp);
                guardedEdge(reCheckVtxp, loopBackVtxp, flp);
                guardedLink(reCheckVtxp, currentp, flp);
            }
            currentp->m_isUnbounded = true;
            m_inUnboundedScope = true;
        } وإلا إذا (repp->maxCountp()) {
            ثابت unsigned maxN = getConstUInt(repp->maxCountp());
            SvaStateVertex* ثابت mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            unsigned tailMinN = minN;
            SvaStateVertex* tailStartp = currentp;
            إذا (minN == 0) {
                // Build the first optional iteration explicitly. Feeding the empty endpoint
                // directly into a range ring would incorrectly keep that match alive.
                SvaStateVertex* ثابت nextVtxp = scopedCreateVertex();
                guardedEdge(currentp, nextVtxp, flp);
                SvaStateVertex* ثابت checkVtxp = scopedCreateVertex();
                guardedLink(nextVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                إذا (m_isCoverSeq) consMidSources.ادفع_back(checkVtxp);
                tailStartp = checkVtxp;
                tailMinN = 1;
            }
            إذا (maxN > tailMinN) {
                // Add tail-ring only if the tail is non-empty.
                SvaStateVertex* ثابت nextVtxp
                    = addDelayChain(tailStartp, maxN - tailMinN + 1, flp, خطأ);
                nextVtxp->m_delayRingClearCondp
                    = جديد AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                nextVtxp->m_replayAbortReject = true;
                SvaStateVertex* ثابت checkVtxp = scopedCreateVertex();
                guardedLink(nextVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                إذا (m_isCoverSeq) consMidSources.ادفع_back(checkVtxp);
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
        أرجع res;
    }

    // always[lo:hi] / s_always[lo:hi] (IEEE 1800-2023 16.12.11).
    BuildResult buildPropAlways(AstPropAlways* nodep, SvaStateVertex* entryVtxp,
                                منطقي is_قمة_Level_Step = خطأ) {
        File_سطر* ثابت flp = nodep->fileline();
        Ast_عقدة_تعبير* ثابت propp = nodep->propp();
        ثابت unsigned lo = getConstUInt(nodep->loBoundp());
        إذا (VN_IS(nodep->hiBoundp(), Unbounded)) {
            // Weak always [lo:$]: unbounded upper bound (IEEE 1800-2023 16.12.11).
            // p must hold at every clock tick at least lo cycles after the attempt
            // start; those ticks are not required to exist, so there is no
            // end-of-trace obligation (weak). The self-loop keeps the attempt live
            // every cycle; each observed cycle is a safety obligation, so a false p
            // rejects immediately.
            UASSERT_OBJ(!nodep->isStrong(), nodep, "Unbounded always must be weak (V3Width)");
            SvaStateVertex* ثابت livep = addDelayChain(entryVtxp, lo, flp);
            livep->m_isUnbounded = true;
            guardedEdge(livep, livep, flp);  // stay active every subsequent cycle
            SvaStateVertex* ثابت sinkp = m_graph.createStateVertex();
            sinkp->m_isRejectSink = true;
            SvaTransEdge* ثابت rejEdgep
                = guardedLink(livep, sinkp, sampled(propp->clone_شجرة_Pure(خطأ)), flp);
            إذا (is_قمة_Level_Step) rejEdgep->m_rejectOnFail = true;
            أرجع {livep, nullptr, {}};
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
            = guardedLink(currentp, checkp, sampled(propp->clone_شجرة_Pure(خطأ)), flp);
        إذا (is_قمة_Level_Step) linkp->m_rejectOnFail = true;
        currentp = checkp;
        m_temporal_Guard_مكدس.ادفع_back(propp);
        currentp = addDelayChain(currentp, hi - lo, flp);
        إذا (nodep->isStrong() && currentp->m_delayRingSize) currentp->m_strongAlwaysRing = true;
        m_temporal_Guard_مكدس.اسحب_back();
        أرجع {currentp, propp, {}};
    }

    BuildResult buildGotoRep(AstSGotoRep* repp, SvaStateVertex* entryVtxp) {
        File_سطر* ثابت flp = repp->fileline();
        Ast_عقدة_تعبير* ثابت exprp = repp->exprp();
        ثابت unsigned minN = getConstUInt(repp->countp());
        إذا (minN == 0) أرجع BuildResult::fail();
        ثابت منطقي hasMax = repp->maxCountp() != nullptr;
        ثابت unsigned maxN = hasMax ? getConstUInt(repp->maxCountp()) : minN;
        إذا (m_isCoverSeq) {
            // Several attempts may coalesce in the one-bit wait vertex before reaching the ring,
            // so a cover sequence action block could run too few times.
            warnEndpointUnsupported(flp, "a goto repetition");
            أرجع BuildResult::failWithError();
        }

        AstVar* ثابت hoistVarp = tryHoistSampled(exprp, flp, 2);

        // The first guardedEdge is the ##1 before waiting for a match. In the wait state, false
        // takes the clocked self-loop, while true takes the zero-delay guardedLink on that tick.
        SvaStateVertex* ثابت waitVtxp = scopedCreateVertex();
        guardedEdge(entryVtxp, waitVtxp, flp);
        guardedEdge(waitVtxp, waitVtxp,
                    جديد AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)}, flp);
        SvaStateVertex* currentp = scopedCreateVertex();
        guardedLink(waitVtxp, currentp, sampledRefOrClone(hoistVarp, exprp, flp), flp);

        إذا (minN > 1) {
            SvaStateVertex* ثابت ringVtxp = addDelayChain(
                currentp, minN - 1, flp, true, nullptr, sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = true;
            SvaStateVertex* ثابت checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            currentp = checkVtxp;
        }
        إذا (!hasMax) {
            currentp->m_isUnbounded = true;  // [->N] waits unboundedly
            m_inUnboundedScope = true;
            أرجع {currentp, nullptr, {}};
        }

        // [->M:N]: the range ring holds matches from M through N and advances
        // only on expr, preserving arbitrarily long gaps between occurrences.
        SvaStateVertex* ثابت mergeVtxp = scopedCreateVertex();
        guardedLink(currentp, mergeVtxp, flp);  // accept at match_M
        إذا (maxN > minN) {
            SvaStateVertex* ثابت ringVtxp
                = addDelayChain(currentp, maxN - minN + 1, flp, خطأ, nullptr,
                                sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = true;
            SvaStateVertex* ثابت checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            guardedLink(checkVtxp, mergeVtxp, flp);
        }
        mergeVtxp->m_isUnbounded = true;  // [->M:N] still has unbounded waits between matches
        m_inUnboundedScope = true;
        أرجع {mergeVtxp, nullptr, {}};
    }

    // True if the sequence has a zero-minimum consecutive repetition, which may match empty
    ساكن منطقي hasZeroMinRep(ثابت Ast_عقدة_تعبير* seqp) {
        أرجع seqp->exists(
            [](ثابت AstSConsRep* repp) { أرجع getConstUInt(repp->countp()) == 0; });
    }

    // True if a leaf's condition has no implication or multi-cycle sequence in it
    // (a leaf can be e.g. 'not (x |-> y)')
    ساكن منطقي isPlainBoolean(ثابت Ast_عقدة_تعبير* condp) {
        أرجع !condp->exists(
            [](ثابت Ast_عقدة_تعبير* np) { أرجع np->isMultiCycleSva() || VN_IS(np, Implication); });
    }

    // Build merge vertex for SOr / LogOr: both branches feed into one vertex.
    BuildResult buildOrMerge(Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp, SvaStateVertex* entryVtxp,
                             File_سطر* flp, منطقي wantBoolLeaf) {
        ثابت BuildResult lhs = build_تعبير(lhsp, entryVtxp);
        ثابت BuildResult rhs = build_تعبير(rhsp, entryVtxp);
        إذا (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع BuildResult::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP
        // IEEE 1800-2023 16.14.3: a cover sequence counts every end-of-match. A
        // sequence operand of 'or' can end more than once, but only its final
        // end reaches the merge vertex below, so reject sequence operands rather
        // than under-count. Plain boolean disjunction has one end per cycle and
        // is handled by the OR-fold.
        ثابت منطقي booleanOnly = lhs.termVertexp == entryVtxp && lhs.finalCondp
                                 && rhs.termVertexp == entryVtxp && rhs.finalCondp;
        إذا (m_isCoverSeq && !booleanOnly) {
            warnEndpointUnsupported(flp, "a sequence operand of 'or'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع BuildResult::failWithError();
        }
        إذا (booleanOnly && wantBoolLeaf) {
            إذا (isPlainBoolean(lhs.finalCondp) && isPlainBoolean(rhs.finalCondp)) {
                // As a |-> / |=> or top-level body, a disjunction of two plain booleans must
                // be able to reject: return it as a boolean finalCondp. The merge vertex
                // below drops the attempt silently when both operands are false.
                Ast_عقدة_تعبير* ثابت orCondp
                    = جديد AstLogOr{flp, lhs.finalCondp->clone_شجرة_Pure(خطأ),
                                   rhs.finalCondp->clone_شجرة_Pure(خطأ)};
                freeUnlinkedCondp(lhs.finalCondp);
                freeUnlinkedCondp(rhs.finalCondp);
                أرجع {entryVtxp, orCondp, {}};
            }
        }
        SvaStateVertex* ثابت mergeVtxp = scopedCreateVertex();
        إذا (booleanOnly) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsp,
                        "Single-cycle SOr operands must have finalCondp");
            guardedLink(entryVtxp, mergeVtxp,
                        جديد AstLogOr{flp, sampled(lhs.finalCondp->clone_شجرة_Pure(خطأ)),
                                     sampled(rhs.finalCondp->clone_شجرة_Pure(خطأ))},
                        flp);
        } وإلا {
            إذا (lhs.finalCondp) {
                guardedLink(lhs.termVertexp, mergeVtxp,
                            sampled(lhs.finalCondp->clone_شجرة_Pure(خطأ)), flp);
            } وإلا {
                guardedLink(lhs.termVertexp, mergeVtxp, flp);
            }
            إذا (rhs.finalCondp) {
                guardedLink(rhs.termVertexp, mergeVtxp,
                            sampled(rhs.finalCondp->clone_شجرة_Pure(خطأ)), flp);
            } وإلا {
                guardedLink(rhs.termVertexp, mergeVtxp, flp);
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        أرجع {mergeVtxp, nullptr, {}};
    }

    // Build done-latch combiner for SAnd/SIntersect (IEEE 1800-2023 16.9.5).
    BuildResult buildAndCombiner(Ast_عقدة_تعبير* lhsExprp, Ast_عقدة_تعبير* rhsExprp,
                                 SvaStateVertex* entryVtxp, File_سطر* flp) {
        // Snapshot-restore scope so LHS liveness does not leak into RHS.
        ثابت منطقي savedScope = m_inUnboundedScope;
        ثابت BuildResult lhs = build_تعبير(lhsExprp, entryVtxp);
        ثابت منطقي lhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope;
        ثابت BuildResult rhs = build_تعبير(rhsExprp, entryVtxp);
        ثابت منطقي rhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope || lhsScope || rhsScope;
        إذا (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع BuildResult::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP

        // Single-cycle operands: use boolean AND (done-latch would fire across cycles).
        // If both operands stayed at entry, they must be boolean leaves which
        // buildExpr returns with finalCondp=nodep (non-null).
        إذا (lhs.termVertexp == entryVtxp && rhs.termVertexp == entryVtxp) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsExprp,
                        "Single-cycle SAnd operands must have finalCondp");
            Ast_عقدة_تعبير* ثابت condp = جديد AstLogAnd{flp, lhs.finalCondp->clone_شجرة_Pure(خطأ),
                                                     rhs.finalCondp->clone_شجرة_Pure(خطأ)};
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع {entryVtxp, condp, {}};
        }
        // Range-delay mid-window sources in either sub-branch would need
        // to be folded into the latch's match-now signal, which the
        // current combiner does not support.
        إذا (!lhs.midSources.empty() || !rhs.midSources.empty()) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: ranged cycle delay in an operand of property 'and'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع BuildResult::failWithError();
        }
        SvaStateVertex* ثابت combVtxp = scopedCreateVertex();
        combVtxp->m_isAndCombiner = true;
        combVtxp->m_andLhsTermp = lhs.termVertexp;
        combVtxp->m_andRhsTermp = rhs.termVertexp;
        إذا (lhs.finalCondp) combVtxp->m_andLhsCondp = lhs.finalCondp->clone_شجرة_Pure(خطأ);
        إذا (rhs.finalCondp) combVtxp->m_andRhsCondp = rhs.finalCondp->clone_شجرة_Pure(خطأ);
        إذا (lhs.termVertexp->m_isUnbounded || rhs.termVertexp->m_isUnbounded) {
            combVtxp->m_isUnbounded = true;
        }
        // Wire terminal-boolean rejects to a dedicated sink so each side can fail
        // the AND independently. Skip for liveness or single-cycle operands
        // (single-cycle termVertexp == entryVtxp would fire every cycle).
        إذا (!combVtxp->m_isUnbounded) {
            منطقي needSink = خطأ;
            ثابت منطقي lhsMultiCycle = (lhs.termVertexp != entryVtxp);
            ثابت منطقي rhsMultiCycle = (rhs.termVertexp != entryVtxp);
            إذا (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                needSink = true;
            }
            إذا (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                needSink = true;
            }
            إذا (needSink) {
                SvaStateVertex* ثابت sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = true;
                إذا (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* ثابت ep = m_graph.addLink(
                        lhs.termVertexp, sinkVtxp, sampled(lhs.finalCondp->clone_شجرة_Pure(خطأ)));
                    ep->m_rejectOnFail = true;
                }
                إذا (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* ثابت ep = m_graph.addLink(
                        rhs.termVertexp, sinkVtxp, sampled(rhs.finalCondp->clone_شجرة_Pure(خطأ)));
                    ep->m_rejectOnFail = true;
                }
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        أرجع {combVtxp, nullptr, {}};
    }

    // Lower `seq1 within seq2` (IEEE 1800-2023 16.9.10) as:
    //   (OR_{i in 0..slack} 1 ##i seq1 ##(slack-i) 1) intersect seq2
    // Both operands must have fixed length (no ranged cycle delays).
    // The OR lives inside a single SIntersect so one AndCombiner done-latch
    // serves all offsets; lifting it out would double-count matches where
    // multiple offsets accept on the same seq2 end cycle.
    BuildResult buildSWithin(AstSWithin* nodep, SvaStateVertex* entryVtxp,
                             منطقي is_قمة_Level_Step = خطأ) {
        ثابت صحيح innerLen = fixedLength(nodep->lhsp());
        ثابت صحيح outerLen = fixedLength(nodep->rhsp());
        إذا (innerLen < 0 || outerLen < 0) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: within with ranged cycle-delay operand");
            أرجع BuildResult::failWithError();
        }
        إذا (innerLen > outerLen) {
            أرجع buildNeverMatchIntersect(
                nodep, entryVtxp, is_قمة_Level_Step,
                "the inner sequence is longer than the outer sequence");
        }
        File_سطر* ثابت flp = nodep->fileline();
        ثابت صحيح slack = outerLen - innerLen;
        Ast_عقدة_تعبير* innerOrp = nullptr;
        لكل (صحيح i = 0; i <= slack; ++i) {
            ثابت صحيح postPad = slack - i;
            Ast_عقدة_تعبير* branchp = nodep->lhsp()->clone_شجرة_Pure(خطأ);
            إذا (i > 0) {
                AstConst* ثابت prePadp = جديد AstConst{flp, AstConst::BitTrue{}};
                AstDelay* ثابت delayp
                    = جديد AstDelay{flp, جديد AstConst{flp, static_cast<uint32_t>(i)}, true};
                AstSExpr* ثابت wrapped = جديد AstSExpr{flp, prePadp, delayp, branchp};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            إذا (postPad > 0) {
                AstConst* ثابت postTruep = جديد AstConst{flp, AstConst::BitTrue{}};
                AstDelay* ثابت delayp
                    = جديد AstDelay{flp, جديد AstConst{flp, static_cast<uint32_t>(postPad)}, true};
                AstSExpr* ثابت wrapped = جديد AstSExpr{flp, branchp, delayp, postTruep};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            innerOrp = innerOrp ? static_cast<Ast_عقدة_تعبير*>(جديد AstSOr{flp, innerOrp, branchp})
                                : branchp;
        }
        Ast_عقدة_تعبير* ثابت outerClonep = nodep->rhsp()->clone_شجرة_Pure(خطأ);
        Ast_عقدة_تعبير* ثابت combinedp = جديد AstSIntersect{flp, innerOrp, outerClonep};
        BuildResult result = build_تعبير(combinedp, entryVtxp, is_قمة_Level_Step);
        VL_DO_DANGLING(combinedp->delete_شجرة(), combinedp);
        // When both operands are plain booleans, buildAndCombiner returns a
        // freshly-allocated AstAnd as finalCondp with no parent. Callers up
        // the chain clone-and-discard finalCondp, so leave it parent-less and
        // it leaks; anchor it in the graph now via an edge.
        إذا (result.valid() && result.finalCondp && !result.finalCondp->backp()) {
            SvaStateVertex* ثابت wrapVtxp = scopedCreateVertex();
            guardedLink(result.termVertexp, wrapVtxp, sampled(result.finalCondp), flp);
            result = {wrapVtxp, nullptr, result.midSources};
        }
        أرجع result;
    }

    // Collect the boolean leaf checks of a fixed-length sequence keyed by their
    // clock offset from the start. Returns false for anything other than nested
    // AstSExpr with fixed cycle delays over boolean leaves (e.g. throughout).
    ساكن منطقي flattenFixedSeq(Ast_عقدة_تعبير* nodep, صحيح baseOffset,
                                std::map<صحيح, std::vector<Ast_عقدة_تعبير*>>& out) {
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
            إذا (!delayp || !delayp->isCycleDelay() || delayp->isUnbounded()) أرجع خطأ;
            ثابت unsigned delayCycles = getConstUInt(delayp->lhsp());
            إذا (delayp->isRangeDelay() && getConstUInt(delayp->rhsp()) != delayCycles)
                أرجع خطأ;
            صحيح preLen = 0;
            إذا (Ast_عقدة_تعبير* ثابت prep = sexprp->preExprp()) {
                إذا (!flattenFixedSeq(prep, baseOffset, out)) أرجع خطأ;
                preLen = fixedLength(prep);
                إذا (preLen < 0) أرجع خطأ;
            }
            أرجع flattenFixedSeq(sexprp->exprp(), baseOffset + preLen + delayCycles, out);
        }
        إذا (nodep->isMultiCycleSva()) أرجع خطأ;
        out[baseOffset].ادفع_back(nodep);
        أرجع true;
    }

    // Conjoin two equal-length fixed sequences into one: at each clock offset
    // AND the boolean checks of both operands (IEEE 1800-2023 16.9.6 -- both
    // operands match the same window). Returns null if either operand is not a
    // plain fixed sequence of boolean leaves.
    ساكن Ast_عقدة_تعبير* conjoinFixedSeqs(Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp, File_سطر* flp) {
        std::map<صحيح, std::vector<Ast_عقدة_تعبير*>> checks;
        إذا (!flattenFixedSeq(lhsp, 0, checks) || !flattenFixedSeq(rhsp, 0, checks)) أرجع nullptr;
        إذا (checks.empty()) أرجع nullptr;
        Ast_عقدة_تعبير* resultp = nullptr;
        صحيح prevOffset = 0;
        لكل (ثابت auto& offsetChecks : checks) {
            ثابت صحيح offset = offsetChecks.first;
            Ast_عقدة_تعبير* condp = nullptr;
            لكل (Ast_عقدة_تعبير* ثابت leafp : offsetChecks.second) {
                Ast_عقدة_تعبير* ثابت clonep = leafp->clone_شجرة_Pure(خطأ);
                إذا (!condp) {
                    condp = clonep;
                } وإلا {
                    condp = جديد AstLogAnd{flp, condp, clonep};
                    condp->dtypeSetBit();
                }
            }
            إذا (!resultp) {
                إذا (offset > 0) {
                    AstDelay* ثابت delayp = جديد AstDelay{
                        flp, جديد AstConst{flp, static_cast<uint32_t>(offset)}, /*isCycle=*/true};
                    resultp
                        = جديد AstSExpr{flp, جديد AstConst{flp, AstConst::BitTrue{}}, delayp, condp};
                    resultp->dtypeSetBit();
                } وإلا {
                    resultp = condp;
                }
            } وإلا {
                AstDelay* ثابت delayp = جديد AstDelay{
                    flp, جديد AstConst{flp, static_cast<uint32_t>(offset - prevOffset)},
                    /*isCycle=*/true};
                resultp = جديد AstSExpr{flp, resultp, delayp, condp};
                resultp->dtypeSetBit();
            }
            prevOffset = offset;
        }
        أرجع resultp;
    }

    // `seq` is a simple ranged sequence `start ##[m:n] end` (start/end boolean,
    // start may be absent). Used to collapse a both-variable intersect to one
    // ranged delay.
    بنية SimpleRanged final {
        منطقي ok = خطأ;
        Ast_عقدة_تعبير* startp = nullptr;  // may be null (absent start)
        Ast_عقدة_تعبير* endp = nullptr;
    };
    ساكن SimpleRanged asSimpleRanged(Ast_عقدة_تعبير* nodep) {
        AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr);
        إذا (!sexprp) أرجع {};
        AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
        إذا (!delayp || !delayp->isCycleDelay() || !delayp->isRangeDelay() || delayp->isUnbounded())
            أرجع {};
        إذا (getConstUInt(delayp->lhsp()) == getConstUInt(delayp->rhsp())) أرجع {};
        Ast_عقدة_تعبير* ثابت prep = sexprp->preExprp();
        إذا (prep && fixedLength(prep) != 0) أرجع {};
        إذا (fixedLength(sexprp->exprp()) != 0) أرجع {};
        أرجع {true, prep, sexprp->exprp()};
    }

    // Build the NFA for a synthesized intersect lowering tree, then free it.
    // buildExpr returns the terminal condition (finalCondp) by reference into the
    // tree; detach a clone so the tree can be freed here. The graph already holds
    // clones/hoists of every edge condition, so nothing else dangles.
    BuildResult build_From_Lowering_شجرة(Ast_عقدة_تعبير* treep, SvaStateVertex* entryVtxp,
                                      منطقي is_قمة_Level_Step) {
        BuildResult result = build_تعبير(treep, entryVtxp, is_قمة_Level_Step);
        إذا (result.valid() && result.finalCondp) {
            result.finalCondp = result.finalCondp->clone_شجرة_Pure(خطأ);
        }
        VL_DO_DANGLING(treep->delete_شجرة(), treep);
        أرجع result;
    }

    // Empty common-length intersection -- unequal fixed lengths, or disjoint
    // ranged lengths. IEEE 1800-2023 16.9.6 requires both operands to match
    // over a window of the same length, so with no common length the intersect
    // simply never matches. This is legal (matching nothing), not an error, so
    // lower to a constant false rather than rejecting legal code.
    BuildResult buildNeverMatchIntersect(Ast_عقدة_تعبير* nodep, SvaStateVertex* entryVtxp,
                                         منطقي is_قمة_Level_Step, ثابت حرف* reason) {
        nodep->v3warn(NEVERMATCH, "Sequence can never match because " << reason << ".");
        Ast_عقدة_تعبير* ثابت falsep = جديد AstConst{nodep->fileline(), AstConst::BitFalse{}};
        أرجع build_From_Lowering_شجرة(falsep, entryVtxp, is_قمة_Level_Step);
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
                                     منطقي is_قمة_Level_Step) {
        ثابت std::pair<صحيح, صحيح> lhsRange = lengthRange(nodep->lhsp());
        ثابت std::pair<صحيح, صحيح> rhsRange = lengthRange(nodep->rhsp());
        إذا (lhsRange.first < 0 || rhsRange.first < 0) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect with this variable-length operand");
            أرجع BuildResult::failWithError();
        }
        ثابت صحيح lo = std::max(lhsRange.first, rhsRange.first);
        ثابت صحيح hi = std::min(lhsRange.second, rhsRange.second);
        إذا (lo > hi) {
            // Disjoint length ranges share no common length -> never matches.
            أرجع buildNeverMatchIntersect(nodep, entryVtxp, is_قمة_Level_Step,
                                            "intersect operands have no common length");
        }
        File_سطر* ثابت flp = nodep->fileline();
        إذا (lo == hi) {
            Ast_عقدة_تعبير* ثابت lp = realizeAtLength(nodep->lhsp(), lo, lhsRange.first);
            Ast_عقدة_تعبير* ثابت rp = realizeAtLength(nodep->rhsp(), lo, rhsRange.first);
            Ast_عقدة_تعبير* ثابت conjp = conjoinFixedSeqs(lp, rp, flp);
            VL_DO_DANGLING(lp->delete_شجرة(), lp);
            VL_DO_DANGLING(rp->delete_شجرة(), rp);
            إذا (!conjp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: intersect operand is not a plain boolean sequence");
                أرجع BuildResult::failWithError();
            }
            أرجع build_From_Lowering_شجرة(conjp, entryVtxp, is_قمة_Level_Step);
        }
        ثابت SimpleRanged sl = asSimpleRanged(nodep->lhsp());
        ثابت SimpleRanged sr = asSimpleRanged(nodep->rhsp());
        إذا (!sl.ok || !sr.ok) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect of two sequences that each vary in length over a"
                          " range with internal structure");
            أرجع BuildResult::failWithError();
        }
        ثابت auto andBool = [&](Ast_عقدة_تعبير* ap, Ast_عقدة_تعبير* bp) -> Ast_عقدة_تعبير* {
            Ast_عقدة_تعبير* ثابت aClonep
                = ap ? ap->clone_شجرة_Pure(خطأ) : جديد AstConst{flp, AstConst::BitTrue{}};
            Ast_عقدة_تعبير* ثابت bClonep
                = bp ? bp->clone_شجرة_Pure(خطأ) : جديد AstConst{flp, AstConst::BitTrue{}};
            AstLogAnd* ثابت andp = جديد AstLogAnd{flp, aClonep, bClonep};
            andp->dtypeSetBit();
            أرجع andp;
        };
        AstDelay* ثابت delayp = جديد AstDelay{flp, جديد AstConst{flp, static_cast<uint32_t>(lo)},
                                              /*isCycle=*/true};
        delayp->rhsp(جديد AstConst{flp, static_cast<uint32_t>(hi)});
        AstSExpr* ثابت reducedp
            = جديد AstSExpr{flp, andBool(sl.startp, sr.startp), delayp, andBool(sl.endp, sr.endp)};
        reducedp->dtypeSetBit();
        أرجع build_From_Lowering_شجرة(reducedp, entryVtxp, is_قمة_Level_Step);
    }

    BuildResult buildThroughout(AstSThroughout* nodep, SvaStateVertex* entryVtxp,
                                منطقي is_قمة_Level_Step = خطأ) {
        // Mark entryVtxp so "cond false at tick 0" is detected as throughout-drop.
        entryVtxp->m_throughoutConds.ادفع_back(nodep->lhsp()->clone_شجرة_Pure(خطأ));
        m_temporal_Guard_مكدس.ادفع_back(nodep->lhsp());
        ثابت BuildResult result = build_تعبير(nodep->rhsp(), entryVtxp, is_قمة_Level_Step);
        m_temporal_Guard_مكدس.اسحب_back();
        أرجع result;
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
    BuildResult buildUntil(AstUntil* nodep, SvaStateVertex* entryVtxp, منطقي is_قمة_Level_Step) {
        File_سطر* ثابت flp = nodep->fileline();
        إذا (!is_قمة_Level_Step) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            أرجع BuildResult::failWithError();
        }
        Ast_عقدة_تعبير* ثابت lhsp = nodep->lhsp();
        Ast_عقدة_تعبير* ثابت rhsp = nodep->rhsp();
        ثابت auto hasSeq = [](ثابت Ast_عقدة_تعبير* ep) {
            أرجع ep->exists([](ثابت Ast_عقدة_تعبير* np) { أرجع np->isMultiCycleSva(); });
        };
        إذا (hasSeq(lhsp) || hasSeq(rhsp)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            أرجع BuildResult::failWithError();
        }

        ثابت منطقي ov = nodep->isOverlapping();
        Ast_عقدة_تعبير* ثابت lhsBitp = nodep->lhsp();
        Ast_عقدة_تعبير* ثابت rhsBitp = nodep->rhsp();
        // p hoist count: continue, require (ov: 1 use; nov: 1 use). At least 2 uses.
        AstVar* ثابت pHoistp = tryHoistSampled(lhsBitp, flp, 2);
        // q hoist count: continue (1) + require nov (1) = 2; ov: continue only (1).
        AstVar* ثابت qHoistp = ov ? nullptr : tryHoistSampled(rhsBitp, flp, 2);

        SvaStateVertex* ثابت waitCp = scopedCreateVertex();
        SvaStateVertex* ثابت waitRp = scopedCreateVertex();
        waitCp->m_isUnbounded = true;
        waitRp->m_strongPending = nodep->isStrong() && !m_isCover;

        // Entry and back-loop Links carry no condition; throughout-folding still applies.
        guardedLink(entryVtxp, waitCp, flp);
        guardedLink(waitRp, waitCp, flp);

        // Continue clocked edge: p && !q advances to next-cycle wait.
        Ast_عقدة_تعبير* ثابت contCondp
            = جديد AstLogAnd{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                            جديد AstLogNot{flp, sampledRefOrClone(qHoistp, rhsBitp, flp)}};
        guardedEdge(waitCp, waitRp, contCondp, flp);

        // Reject sink: fires when require-condition is false.
        SvaStateVertex* ثابت sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = true;
        Ast_عقدة_تعبير* requireCondp;
        إذا (ov) {
            requireCondp = sampledRefOrClone(pHoistp, lhsBitp, flp);
        } وإلا {
            requireCondp = جديد AstLogOr{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                                        sampledRefOrClone(qHoistp, rhsBitp, flp)};
        }
        SvaTransEdge* ثابت rejEdgep = m_graph.addLink(waitCp, sinkVtxp, requireCondp);
        rejEdgep->m_rejectOnFail = true;

        // Accept condition rides via finalCondp; assembleResult $sampled-wraps it.
        Ast_عقدة_تعبير* acceptCondp;
        إذا (ov) {
            acceptCondp
                = جديد AstLogAnd{flp, lhsBitp->clone_شجرة_Pure(خطأ), rhsBitp->clone_شجرة_Pure(خطأ)};
        } وإلا {
            acceptCondp = rhsBitp->clone_شجرة_Pure(خطأ);
        }
        أرجع {waitCp, acceptCondp, {}};
    }

    // IEEE 1800-2023 16.12.14 property abort operators. Sync and async share
    // the same NFA encoding: AstSampled already gives matured values at every
    // maturing clocking event, and async firing "between clocks" is not
    // observable in a cycle-based model. VAbortKind selects accept vs reject
    // verdict (sync vs async only changes the user-visible spelling).

    // Build `condp && !outer_1 && !outer_2 ...` (unsampled).
    Ast_عقدة_تعبير* abort_Fire_تعبير(Ast_عقدة_تعبير* condp, File_سطر* flp) {
        Ast_عقدة_تعبير* resultp = condp->clone_شجرة_Pure(خطأ);
        لكل (Ast_عقدة_تعبير* ثابت op : m_outer_Abort_مكدس)
            resultp = جديد AstLogAnd{flp, resultp, جديد AstLogNot{flp, op->clone_شجرة_Pure(خطأ)}};
        أرجع resultp;
    }

    // True when unguarded ring-wide rejection or a same-tick Link chain already accounts the
    // attempt: a required-step Link covers both outcomes; followed-by pairs both edges.
    ساكن منطقي chainAccountsSource(ثابت SvaStateVertex* srcp,
                                    ثابت std::unordered_set<ثابت V3GraphEdge*>& preEdges) {
        إذا (srcp->m_delayRingSize && srcp->m_throughoutConds.empty()) أرجع true;
        لكل (ثابت V3GraphEdge& edger : srcp->inEdges()) {
            إذا (preEdges.عدد(&edger)) استمر;
            ثابت SvaTransEdge& tedger = static_cast<ثابت SvaTransEdge&>(edger);
            إذا (tedger.m_consumesCycle) استمر;
            ثابت auto* ثابت fromp = static_cast<ثابت SvaStateVertex*>(tedger.fromVtxp());
            إذا (fromp->m_abortRejectp) أرجع true;
        }
        منطقي plainNonSink = خطأ;
        منطقي markedSink = خطأ;
        لكل (ثابت V3GraphEdge& edger : srcp->outEdges()) {
            إذا (preEdges.عدد(&edger)) استمر;
            ثابت SvaTransEdge& tedger = static_cast<ثابت SvaTransEdge&>(edger);
            إذا (tedger.m_consumesCycle) استمر;
            ثابت منطقي sink = static_cast<ثابت SvaStateVertex*>(tedger.toVtxp())->m_isRejectSink;
            إذا (tedger.m_rejectOnFail) {
                إذا (!sink) أرجع true;
                markedSink = true;
            } وإلا إذا (!sink) {
                إذا (srcp->m_abortRejectp) أرجع true;
                plainNonSink = true;
            }
        }
        أرجع plainNonSink && markedSink;
    }

    // Reject edge: fires when the source is live and the abort samples true.
    فراغ addAbortRejectEdge(SvaStateVertex* srcp, SvaStateVertex* sinkp, Ast_عقدة_تعبير* condp,
                            File_سطر* flp) {
        Ast_عقدة_تعبير* ثابت notFirep = جديد AstLogNot{flp, sampled(abort_Fire_تعبير(condp, flp))};
        m_graph.addLink(srcp, sinkp, notFirep)->m_rejectOnFail = true;
        أرجع;
    }

    // On the fire tick: kill body threads; accept kinds also forgive step misses.
    فراغ gateBodyEdgesOnAbort(ثابت std::unordered_set<ثابت V3GraphEdge*>& preEdges,
                              Ast_عقدة_تعبير* condp, VAbortKind kind, File_سطر* flp) {
        لكل (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            لكل (V3GraphEdge& edger : vtxr.outEdges()) {
                إذا (preEdges.عدد(&edger)) استمر;
                SvaTransEdge* ثابت tedgep = static_cast<SvaTransEdge*>(&edger);
                إذا (tedgep->m_rejectOnFail) {
                    إذا (!kind.isAccept()) استمر;
                    Ast_عقدة_تعبير* ثابت firep = sampled(abort_Fire_تعبير(condp, flp));
                    tedgep->m_condp
                        = tedgep->m_condp ? جديد AstLogOr{flp, tedgep->m_condp, firep} : firep;
                } وإلا إذا (tedgep->m_consumesCycle) {
                    Ast_عقدة_تعبير* ثابت notFirep
                        = جديد AstLogNot{flp, sampled(abort_Fire_تعبير(condp, flp))};
                    tedgep->m_condp = tedgep->m_condp
                                          ? جديد AstLogAnd{flp, tedgep->m_condp, notFirep}
                                          : notFirep;
                }
            }
        }
    }

    BuildResult buildAbortOn(Ast_عقدة_تعبير* condp, Ast_عقدة_تعبير* bodyp, SvaStateVertex* entryVtxp,
                             VAbortKind kind, File_سطر* flp, منطقي is_قمة_Level_Step) {
        // Snapshot pre-body vertices/edges so post-build diff yields the body's sub-NFA.
        std::unordered_set<ثابت V3GraphVertex*> preExisting;
        std::unordered_set<ثابت V3GraphEdge*> preEdges;
        لكل (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            preExisting.أدرج(&vtxr);
            لكل (V3GraphEdge& edger : vtxr.outEdges()) preEdges.أدرج(&edger);
        }

        m_outer_Abort_مكدس.ادفع_back(condp);
        ثابت BuildResult bodyResult = build_تعبير(bodyp, entryVtxp, is_قمة_Level_Step);
        m_outer_Abort_مكدس.اسحب_back();
        إذا (!bodyResult.valid()) أرجع bodyResult;

        gateBodyEdgesOnAbort(preEdges, condp, kind, flp);

        // Live-thread sources for the abort edge: entry + new body vertices,
        // minus reject sinks (they carry reject fuel, not live-thread fuel).
        std::vector<SvaStateVertex*> abortSources;
        abortSources.ادفع_back(entryVtxp);
        لكل (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            إذا (preExisting.عدد(&vtxr)) استمر;
            auto* ثابت sp = static_cast<SvaStateVertex*>(&vtxr);
            إذا (sp->m_delayRingSize || !sp->m_throughoutConds.empty()) {
                Ast_عقدة_تعبير* ثابت firep = abort_Fire_تعبير(condp, flp);
                sp->m_abortClearp
                    = sp->m_abortClearp ? جديد AstLogOr{flp, sp->m_abortClearp, firep} : firep;
            }
            إذا (!kind.isAccept()
                && ((sp->m_delayRingSize && sp->m_throughoutConds.empty())
                    || sp->m_replayAbortReject)) {
                Ast_عقدة_تعبير* ثابت firep = abort_Fire_تعبير(condp, flp);
                sp->m_abortRejectp
                    = sp->m_abortRejectp ? جديد AstLogOr{flp, sp->m_abortRejectp, firep} : firep;
            }
            إذا (sp->m_isRejectSink) استمر;
            abortSources.ادفع_back(sp);
        }

        auto sampledAbortFire = [&]() -> Ast_عقدة_تعبير* {
            Ast_عقدة_تعبير* ثابت تعبير = abort_Fire_تعبير(condp, flp);
            أرجع sampled(تعبير);
        };

        إذا (kind.isAccept()) {
            // Match-only sink fed by $sampled(abort-fire) from every live source;
            // registered as midSource so it never contributes a reject.
            SvaStateVertex* ثابت acceptSinkp = scopedCreateVertex();
            لكل (SvaStateVertex* ثابت srcp : abortSources)
                guardedLink(srcp, acceptSinkp, sampledAbortFire(), flp);
            std::vector<SvaStateVertex*> midSources = bodyResult.midSources;
            midSources.ادفع_back(acceptSinkp);
            Ast_عقدة_تعبير* finalCondp = bodyResult.finalCondp;
            إذا (finalCondp) {
                إذا (finalCondp->backp()) finalCondp = finalCondp->clone_شجرة_Pure(خطأ);
                finalCondp = جديد AstLogOr{flp, finalCondp, abort_Fire_تعبير(condp, flp)};
            }
            أرجع {bodyResult.termVertexp, finalCondp, std::move(midSources)};
        }

        // rejectOnFail treats m_condp as the success condition and fires on
        // !condp, so the edge carries !sampledAbortFire().
        SvaStateVertex* ثابت rejectSinkp = m_graph.createStateVertex();
        rejectSinkp->m_isRejectSink = true;
        لكل (SvaStateVertex* ثابت srcp : abortSources)
            إذا (!chainAccountsSource(srcp, preEdges))
                addAbortRejectEdge(srcp, rejectSinkp, condp, flp);
        Ast_عقدة_تعبير* finalCondp = bodyResult.finalCondp;
        إذا (finalCondp) {
            إذا (finalCondp->backp()) finalCondp = finalCondp->clone_شجرة_Pure(خطأ);
            finalCondp
                = جديد AstLogAnd{flp, finalCondp, جديد AstLogNot{flp, abort_Fire_تعبير(condp, flp)}};
        }
        أرجع {bodyResult.termVertexp, finalCondp, bodyResult.midSources};
    }

عام:
    // Free a dropped sub-result condition that is not linked into the AST
    // (abort folds synthesize unparented finalCondp trees).
    ساكن فراغ freeUnlinkedCondp(Ast_عقدة_تعبير* condp) {
        إذا (condp && !condp->backp()) VL_DO_DANGLING(condp->delete_شجرة(), condp);
    }

    SvaNfaBuilder(SvaGraph& graph, Ast_عقدة_Module* modp, V3UniqueNames& propTempNames,
                  منطقي isCoverSeq = خطأ, منطقي isSeqEvent = خطأ, منطقي isCover = خطأ)
        : m_graph{graph}
        , m_modp{modp}
        , m_propTempNames{propTempNames}
        , m_isCover{isCover}
        , m_isCoverSeq{isCoverSeq}
        , m_isSeqEvent{isSeqEvent} {}

    // Reset scope between antecedent and consequent: liveness must not leak.
    // m_outerAbortStack survives: an abort wrapping the implication covers the
    // consequent too (IEEE 1800-2023 16.12.14).
    فراغ resetScope() {
        m_inUnboundedScope = خطأ;
        m_temporal_Guard_مكدس.clear();
    }

    BuildResult build_تعبير(Ast_عقدة_تعبير* nodep, SvaStateVertex* entryVtxp,
                          منطقي is_قمة_Level_Step = خطأ) {
        VL_RESTORER(m_wantBoolLeaf);
        ثابت منطقي wantBoolLeaf = m_wantBoolLeaf;
        m_wantBoolLeaf = خطأ;
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            أرجع buildSExpr(sexprp, entryVtxp, is_قمة_Level_Step);
        }
        إذا (AstSConsRep* ثابت repp = VN_CAST(nodep, SConsRep)) {
            أرجع buildConsRep(repp, entryVtxp, is_قمة_Level_Step);
        }
        إذا (AstPropAlways* ثابت alwaysp = VN_CAST(nodep, PropAlways)) {
            أرجع buildPropAlways(alwaysp, entryVtxp, is_قمة_Level_Step);
        }
        إذا (AstSGotoRep* ثابت repp = VN_CAST(nodep, SGotoRep)) {
            أرجع buildGotoRep(repp, entryVtxp);
        }
        إذا (AstSThroughout* ثابت throughoutp = VN_CAST(nodep, SThroughout)) {
            أرجع buildThroughout(throughoutp, entryVtxp, is_قمة_Level_Step);
        }
        إذا (AstSOr* ثابت orp = VN_CAST(nodep, SOr)) {
            أرجع buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        إذا (AstLogOr* ثابت orp = VN_CAST(nodep, LogOr)) {
            أرجع buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        إذا (AstSAnd* ثابت andp = VN_CAST(nodep, SAnd)) {
            أرجع buildAndCombiner(andp->lhsp(), andp->rhsp(), entryVtxp, andp->fileline());
        }
        إذا (AstSIntersect* ثابت intp = VN_CAST(nodep, SIntersect)) {
            // IEEE 1800-2023 16.9.6: both operands match over one window with
            // equal start and end (equal length). Lower to a single sequence
            // that conjoins both operands' per-cycle checks -- correct under
            // concurrent attempts, where the done-latch combiner conflates the
            // two operands' start times and over-accepts. The combiner remains
            // only as a fallback for operands that do not flatten.
            ثابت صحيح lhsLen = fixedLength(intp->lhsp());
            ثابت صحيح rhsLen = fixedLength(intp->rhsp());
            إذا (lhsLen >= 0 && rhsLen >= 0) {
                إذا (lhsLen != rhsLen) {
                    // Unequal fixed lengths share no common length -> never matches.
                    أرجع buildNeverMatchIntersect(intp, entryVtxp, is_قمة_Level_Step,
                                                    "intersect operands have no common length");
                }
                إذا (Ast_عقدة_تعبير* ثابت conjp
                    = conjoinFixedSeqs(intp->lhsp(), intp->rhsp(), intp->fileline())) {
                    أرجع build_From_Lowering_شجرة(conjp, entryVtxp, is_قمة_Level_Step);
                }
                أرجع buildAndCombiner(intp->lhsp(), intp->rhsp(), entryVtxp, intp->fileline());
            }
            أرجع buildVarLenIntersect(intp, entryVtxp, is_قمة_Level_Step);
        }
        إذا (AstSWithin* ثابت withinp = VN_CAST(nodep, SWithin)) {
            أرجع buildSWithin(withinp, entryVtxp, is_قمة_Level_Step);
        }
        إذا (AstAbortOn* ثابت ap = VN_CAST(nodep, AbortOn)) {
            أرجع buildAbortOn(ap->condp(), ap->propp(), entryVtxp, ap->kind(), ap->fileline(),
                                is_قمة_Level_Step);
        }
        إذا (VN_IS(nodep, SNonConsRep)) أرجع BuildResult::fail();
        إذا (AstImplication* ثابت implp = VN_CAST(nodep, Implication)) {
            أرجع buildImplicationEdges(implp->lhsp(), implp->rhsp(), entryVtxp,
                                         implp->isOverlapped(), implp->isFollowedBy(),
                                         implp->lhsp(), implp->fileline());
        }
        إذا (AstUntil* ثابت untilp = VN_CAST(nodep, Until)) {
            أرجع buildUntil(untilp, entryVtxp, is_قمة_Level_Step);
        }
        // Boolean leaf (including LogAnd): return as finalCond
        أرجع {entryVtxp, nodep, {}};
    }

    // Wire an implication / followed-by from `entryVtxp`: builds the antecedent,
    // emits the match-link (and for followed-by the reject-sink edge), inserts a
    // delay vertex for non-overlapped forms, and builds the body. Used both for
    // nested AstImplication in pexpr position and for the top-level assertion
    // antecedent -- `errorNodep` anchors the "unsupported sequence antecedent"
    // error, which differs between the two call sites.
    BuildResult buildImplicationEdges(Ast_عقدة_تعبير* antExprp, Ast_عقدة_تعبير* bodyExprp,
                                      SvaStateVertex* entryVtxp, منطقي isOverlapped,
                                      منطقي isFollowedBy, Ast_عقدة* errorNodep, File_سطر* flp) {
        ثابت BuildResult antResult = build_تعبير(antExprp, entryVtxp);
        إذا (!antResult.valid()) أرجع antResult;

        // Followed-by requires pure-boolean antecedent for non-vacuous-fail at
        // the attempt-start cycle. IEEE 1800-2023 16.12.9 permits a multi-cycle
        // sequence LHS, so this is an implementation gap rather than illegal SV.
        إذا (isFollowedBy && antResult.termVertexp != entryVtxp) {
            errorNodep->v3warn(E_UNSUPPORTED,
                               "Unsupported: sequence expression as antecedent of followed-by"
                               " (#-# / #=#) (IEEE 1800-2023 16.12.9)");
            أرجع BuildResult::failWithError();
        }
        UASSERT_OBJ(!isFollowedBy || antResult.finalCondp, errorNodep,
                    "followed-by antecedent terminal at entry must carry finalCondp");

        // Use raw createStateVertex() so trigVtxp starts without liveness --
        // reaching the antecedent terminal is a definitive event.
        SvaStateVertex* ثابت trigVtxp = m_graph.createStateVertex();
        إذا (antResult.finalCondp) {
            m_graph.addLink(antResult.termVertexp, trigVtxp,
                            sampled(antResult.finalCondp->clone_شجرة_Pure(خطأ)));
            // Followed-by non-vacuous fail: rejectOnFail fires when the attempt
            // is live (termVtx reachable) and sampled(antecedent) is false.
            إذا (isFollowedBy) {
                SvaStateVertex* ثابت sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = true;
                SvaTransEdge* ثابت ep
                    = m_graph.addLink(antResult.termVertexp, sinkVtxp,
                                      sampled(antResult.finalCondp->clone_شجرة_Pure(خطأ)));
                ep->m_rejectOnFail = true;
            }
            // finalCondp is cloned into the Sampled nodes; if the original is
            // not parented anywhere in the AST anymore it must be freed here
            // or ASan flags it as a leak (e.g. t_sequence_bool_ops).
            إذا (!antResult.finalCondp->backp()) {
                VL_DO_DANGLING(antResult.finalCondp->delete_شجرة(), antResult.finalCondp);
            }
        } وإلا {
            m_graph.addLink(antResult.termVertexp, trigVtxp);
        }
        resetScope();

        SvaStateVertex* bodyEntryp = trigVtxp;
        إذا (!isOverlapped) {
            SvaStateVertex* ثابت delayVtxp = m_graph.createStateVertex();
            m_graph.addClockedEdge(trigVtxp, delayVtxp);
            bodyEntryp = delayVtxp;
        }
        // An empty antecedent match must not start the consequent, but a zero-minimum
        // repetition's empty match is not handled here; keep the merge vertex for those
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = !isFollowedBy && !hasZeroMinRep(antExprp);
        أرجع build_تعبير(bodyExprp, bodyEntryp, /*isTopLevelStep=*/true);
    }

    BuildResult build(Ast_عقدة_تعبير* exprp) {
        m_graph.m_startVertexp = scopedCreateVertex();
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = true;
        أرجع build_تعبير(exprp, m_graph.m_startVertexp, /*isTopLevelStep=*/true);
    }
};

//######################################################################
// NFA Lowering (converts NFA graph to synthesizable AstAlways blocks)

صنف SvaNfaLowering final {
    Ast_عقدة_Module* ثابت m_modp;  // Module to add state vars and always blocks to
    Ast_عقدة_DType* ثابت m_u32DTypep;  // Shared unsigned counter dtype
    V3UniqueNames m_names{"__Vnfa"};
    size_t m_statDelayRingEdgeVisits = 0;  // Delay-ring incoming edges visited

    // Per-lowering shared context (passed to phase sub-functions)
    // Per-vertex lowering state is stored in SvaVertexData and accessed via
    // V3GraphVertex::userp() (see SvaStateVertex::datap()).
    بنية LowerCtx final {
        File_سطر* flp;  // Source location for generated AST
        Ast_Sen_شجرة* senTreep;  // Clock sensitivity tree
        Ast_عقدة_تعبير* disableExprp;  // disable iff expression (may be nullptr)
        Ast_عقدة_تعبير* matchCondp;  // Final boolean match condition (may be nullptr)
        VAssertType assertType;  // Assertion type for control tasks
        VAssertDirectiveType directiveType;  // Directive type for control tasks
        AstVar* killVarp;  // Last observed kill generation
        SvaGraph& graph;  // NFA graph
    };

    // Build a match-now expression: stateSig[i] && $sampled(condp)
    ساكن Ast_عقدة_تعبير* buildMatchNow(File_سطر* flp, Ast_عقدة_تعبير* stateExprp, Ast_عقدة_تعبير* condp) {
        Ast_عقدة_تعبير* ثابت statep = stateExprp->clone_شجرة_Pure(خطأ);
        إذا (!condp) أرجع statep;
        أرجع جديد AstLogAnd{flp, statep, sampled(condp->clone_شجرة_Pure(خطأ))};
    }
    ساكن Ast_عقدة_تعبير* andCond(File_سطر* flp, Ast_عقدة_تعبير* exprp, Ast_عقدة_تعبير* condp) {
        إذا (!condp) أرجع exprp;
        أرجع جديد AstLogAnd{flp, exprp, condp->clone_شجرة_Pure(خطأ)};
    }
    // bp is always non-null; only ap can be null (serving as accumulator).
    ساكن Ast_عقدة_تعبير* orExprs(File_سطر* flp, Ast_عقدة_تعبير* ap, Ast_عقدة_تعبير* bp) {
        إذا (!ap) أرجع bp;
        أرجع جديد AstLogOr{flp, ap, bp};
    }
    ساكن Ast_عقدة_تعبير* addCountp(File_سطر* ثابت flp, Ast_عقدة_تعبير* ثابت totalCountp,
                                  Ast_عقدة_تعبير* ثابت contributionp,
                                  Ast_عقدة_تعبير* ثابت enablep = nullptr) {
        // contribution = enable ? contribution : 0;
        Ast_عقدة_تعبير* ثابت enabledContributionp
            = enablep ? جديد AstCond{flp, enablep, contributionp,
                                    newTypedConstp(flp, contributionp->dtypep(), 0)}
                      : contributionp;
        إذا (!totalCountp) أرجع enabledContributionp;
        أرجع جديد AstAdd{flp, totalCountp, enabledContributionp};
    }
    ساكن Ast_عقدة_تعبير* gateCheckedCountp(File_سطر* ثابت flp, Ast_عقدة_تعبير* ثابت countp,
                                          Ast_عقدة_تعبير*& overflowp, Ast_عقدة_تعبير* ثابت enablep) {
        إذا (overflowp) {
            overflowp = جديد AstLogAnd{flp, enablep->clone_شجرة_Pure(خطأ), overflowp};
        }
        أرجع addCountp(flp, nullptr, countp, enablep);
    }
    ساكن Ast_عقدة_تعبير* addCheckedCountp(File_سطر* ثابت flp, Ast_عقدة_تعبير* ثابت totalCountp,
                                         Ast_عقدة_تعبير* ثابت contributionp, Ast_عقدة_تعبير*& overflowp,
                                         Ast_عقدة_تعبير* ثابت contributionOverflowp = nullptr) {
        إذا (contributionOverflowp) { overflowp = orExprs(flp, overflowp, contributionOverflowp); }
        إذا (!totalCountp) أرجع contributionp;
        Ast_عقدة_تعبير* ثابت oldTotalp = totalCountp->clone_شجرة_Pure(خطأ);
        Ast_عقدة_تعبير* ثابت resultp = جديد AstAdd{flp, totalCountp, contributionp};
        overflowp
            = orExprs(flp, overflowp, جديد AstLt{flp, resultp->clone_شجرة_Pure(خطأ), oldTotalp});
        أرجع resultp;
    }
    AstIf* new_عدد_Overflow_Warning(File_سطر* ثابت flp, Ast_عقدة_تعبير* ثابت condp) ثابت {
        AstDisplay* ثابت dispp = جديد AstDisplay{
            flp, VDisplayType::DT_WARNING,
            "Cover sequence match count overflowed 32-bit storage; results may be incorrect.",
            nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstIf* ثابت ifp = جديد AstIf{flp, condp, dispp};
        ifp->branchPred(VBranchPred::BP_UNLIKELY);
        أرجع ifp;
    }
    ساكن Ast_عقدة_تعبير* killActive(LowerCtx& c) {
        أرجع جديد AstNeq{c.flp, جديد AstVarRef{c.flp, c.killVarp, VAccess::READ},
                          assertKillGet(c.flp, c.assertType, c.directiveType)};
    }
    ساكن Ast_عقدة_تعبير* notKillActive(LowerCtx& c) { أرجع جديد AstLogNot{c.flp, killActive(c)}; }
    ساكن Ast_عقدة_تعبير* gateNotKill(LowerCtx& c, Ast_عقدة_تعبير* exprp) {
        إذا (!exprp) أرجع nullptr;
        أرجع جديد AstLogAnd{c.flp, exprp, notKillActive(c)};
    }
    ساكن Ast_عقدة_تعبير* nextRingIndex(File_سطر* flp, Ast_عقدة_تعبير* idxExprp, uint32_t size) {
        ثابت auto u32Const = [flp](uint32_t قيمة) {
            أرجع جديد AstConst{flp, AstConst::Widthed_قيمة{}, 32, قيمة};
        };
        UASSERT_OBJ(size > 0, idxExprp, "Ring size must be positive");
        إذا (size == 1) {
            VL_DO_DANGLING(idxExprp->delete_شجرة(), idxExprp);
            أرجع u32Const(0);
        }
        // idx == size - 1 ? 0 : idx + 1
        AstAdd* ثابت addp = جديد AstAdd{flp, idxExprp->clone_شجرة_Pure(خطأ), u32Const(1)};
        addp->dtypeFrom(idxExprp);
        AstCond* ثابت condp
            = جديد AstCond{flp, جديد AstEq{flp, idxExprp, u32Const(size - 1)}, u32Const(0), addp};
        condp->dtypeFrom(idxExprp);
        أرجع condp;
    }
    ساكن Ast_عقدة_تعبير* delayRingSlot(File_سطر* flp, SvaStateVertex* vtxp, Ast_عقدة_تعبير* idxExprp,
                                      VAccess access = VAccess::READ) {
        // ring[idx]
        AstVar* ثابت ringp = vtxp->datap()->delayRingVarp;
        AstVarRef* ثابت refp = جديد AstVarRef{flp, ringp, access};
        إذا (vtxp->datap()->delayRingCounts) أرجع جديد AstArraySel{flp, refp, idxExprp};
        أرجع جديد AstSel{flp, refp, idxExprp, 1};
    }
    ساكن Ast_عقدة_تعبير* delay_Ring_Head_عدد(File_سطر* flp, SvaStateVertex* vtxp) {
        Ast_عقدة_تعبير* ثابت headp
            = جديد AstVarRef{flp, vtxp->datap()->delayRingHeadVarp, VAccess::READ};
        إذا (vtxp->datap()->delayRingCounts) أرجع headp;
        أرجع جديد AstExtend{flp, headp, vtxp->datap()->delay_Ring_Live_عدد_Varp->dtypep()->width()};
    }
    ساكن Ast_عقدة_تعبير* delayRingAtLastIndex(File_سطر* ثابت flp, AstVar* ثابت idxp,
                                             ثابت uint32_t size) {
        أرجع جديد AstEq{flp, جديد AstVarRef{flp, idxp, VAccess::READ},
                         جديد AstConst{flp, AstConst::Widthed_قيمة{}, 32, size - 1}};
    }
    ساكن Ast_عقدة_تعبير* delay_Ring_Output_عدد(File_سطر* ثابت flp, SvaStateVertex* ثابت vtxp) {
        AstVar* ثابت idxp = vtxp->datap()->delayRingIdxVarp;
        ثابت uint32_t size = vtxp->m_delayRingSize;
        Ast_عقدة_تعبير* outgoingValidp
            = جديد AstVarRef{flp, vtxp->datap()->delayRingWrappedVarp, VAccess::READ};
        إذا (!vtxp->m_isFixedDelayRing) {
            outgoingValidp
                = جديد AstLogOr{flp, outgoingValidp, delayRingAtLastIndex(flp, idxp, size)};
        }
        AstVar* ثابت live_عدد_Varp = vtxp->datap()->delay_Ring_Live_عدد_Varp;
        أرجع جديد AstCond{flp, outgoingValidp, delay_Ring_Head_عدد(flp, vtxp),
                           newTypedConstp(flp, live_عدد_Varp->dtypep(), 0)};
    }
    ساكن Ast_عقدة_تعبير* delayRingHasLiveThreadsp(File_سطر* ثابت flp,
                                                 AstVar* ثابت live_عدد_Varp) {
        // active = live_count != 0;
        أرجع جديد AstNeq{flp, جديد AstVarRef{flp, live_عدد_Varp, VAccess::READ},
                          newTypedConstp(flp, live_عدد_Varp->dtypep(), 0)};
    }

    // Phase 3 output signals
    بنية SignalSet final {
        Ast_عقدة_تعبير* terminalActivep = nullptr;  // OR of all successful terminal matches
        Ast_عقدة_تعبير* matchCountp = nullptr;  // NFA paths completing the sequence this tick
        Ast_عقدة_تعبير* match_عدد_Overflowp = nullptr;  // Match count overflow this tick
        Ast_عقدة_تعبير* rejectBasep = nullptr;  // Reject when a terminal match fails
        Ast_عقدة_تعبير* requiredStepRejectp = nullptr;  // Per-source reject from rejectOnFail Links
        Ast_عقدة_تعبير* throughoutRejectp = nullptr;  // Reject when a throughout guard drops
        Ast_عقدة_تعبير* threadFailCountp = nullptr;  // Number of threads rejected on this tick
    };

    // Disable condition also aborts attempts between clock events. A constant disable
    // never rises, and $sampled is unsupported in a sensitivity list.
    ساكن منطقي isAsyncDisable(ثابت Ast_عقدة_تعبير* ثابت disableExprp) {
        أرجع disableExprp && !VN_IS(disableExprp, Const)
               && !disableExprp->exists([](ثابت AstSampled*) { أرجع true; });
    }

    // Emit the clocked update block of registers that hold pending attempts. The clearp
    // statements reset the same registers: a disable rising between clock events also
    // aborts pending attempts (IEEE 1800-2023 16.12), so clearp runs on that rising edge
    // in a separate block. It must clear unconditionally, as the disable may already be
    // false again when it runs, and the update block must not run on that edge, as it
    // would advance the attempts as if the clock had ticked. Blocks that hold no attempt
    // state, such as the $assertkill acknowledge in emitKillAckNba, do not go through here.
    // Callers pass a null clearp when the disable cannot rise between clock events.
    فراغ addStateAlways(LowerCtx& c, Ast_عقدة* bodyp, Ast_عقدة* clearp) {
        m_modp->addStmtsp(
            جديد AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_شجرة(خطأ), bodyp});
        إذا (!clearp) أرجع;
        m_modp->addStmtsp(جديد AstAlways{
            c.flp, VAlwaysKwd::ALWAYS,
            جديد Ast_Sen_شجرة{c.flp, جديد AstSenItem{c.flp, VEdgeType::ET_POSEDGE,
                                                 c.disableExprp->clone_شجرة_Pure(خطأ)}},
            clearp});
    }

    // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
    // Phase 2: State register NBA always block. Each clocked-edge target
    // latches the OR of its incoming contributions.
    فراغ emitStateRegisterNba(LowerCtx& c) {
        ثابت منطقي asyncDisable = isAsyncDisable(c.disableExprp);
        Ast_عقدة* bodyp = nullptr;
        Ast_عقدة* clearp = nullptr;
        لكل (ثابت V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ثابت SvaStateVertex* ثابت vtxp = static_cast<ثابت SvaStateVertex*>(&vtxr);
            إذا (!vtxp->datap()->stateVarp) استمر;

            Ast_عقدة_تعبير* nextStatep = nullptr;
            لكل (ثابت V3GraphEdge& edger : vtxp->inEdges()) {
                ثابت SvaTransEdge& tedger = static_cast<ثابت SvaTransEdge&>(edger);
                إذا (!tedger.m_consumesCycle) استمر;
                ثابت SvaStateVertex* ثابت fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Clocked-edge source missing stateSig");

                Ast_عقدة_تعبير* srcSigp = fromVtxp->datap()->stateSigp->clone_شجرة_Pure(خطأ);
                srcSigp = andCond(c.flp, srcSigp, tedger.m_condp);

                إذا (c.disableExprp) {
                    Ast_عقدة_تعبير* ثابت notDisp
                        = جديد AstLogNot{c.flp, c.disableExprp->clone_شجرة_Pure(خطأ)};
                    srcSigp = جديد AstLogAnd{c.flp, srcSigp, notDisp};
                }
                nextStatep = orExprs(c.flp, nextStatep, srcSigp);
            }

            UASSERT_OBJ(nextStatep, vtxp,
                        "Registered vertex has no clocked incoming contribution");
            nextStatep = gateNotKill(c, nextStatep);

            AstAssignDly* ثابت assignp = جديد AstAssignDly{
                c.flp, جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE}, nextStatep};
            bodyp = Ast_عقدة::addNextNull(bodyp, assignp);
            إذا (asyncDisable) {
                clearp = Ast_عقدة::addNextNull(
                    clearp,
                    جديد AstAssignDly{
                        c.flp, جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE},
                        جديد AstConst{c.flp, AstConst::BitFalse{}}});
            }
        }
        إذا (bodyp) addStateAlways(c, bodyp, clearp);
    }

    // Phase 2b: In-place ring storage and registered ring metadata.
    فراغ emitDelayRingUpdates(LowerCtx& c, ثابت منطقي عدد_Matches) {
        ثابت منطقي asyncDisable = isAsyncDisable(c.disableExprp);
        لكل (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ثابت vtxp = static_cast<SvaStateVertex*>(&vtxr);
            إذا (!vtxp->datap()->delayRingVarp) استمر;
            AstVar* ثابت idxp = vtxp->datap()->delayRingIdxVarp;
            AstVar* ثابت live_عدد_Varp = vtxp->datap()->delay_Ring_Live_عدد_Varp;
            AstVar* ثابت wrappedp = vtxp->datap()->delayRingWrappedVarp;
            ثابت uint32_t size = static_cast<uint32_t>(vtxp->m_delayRingSize);

            Ast_عقدة_تعبير* incomingp = nullptr;
            Ast_عقدة_تعبير* incomingOverflowp = nullptr;
            لكل (ثابت V3GraphEdge& edger : vtxp->inEdges()) {
                ++m_statDelayRingEdgeVisits;
                ثابت SvaTransEdge& tedger = static_cast<ثابت SvaTransEdge&>(edger);
                UASSERT_OBJ(tedger.m_consumesCycle == vtxp->m_isFixedDelayRing, vtxp,
                            "Delay-ring incoming edge kind mismatch");
                ثابت SvaStateVertex* ثابت fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Delay-ring incoming source missing stateSig");
                UASSERT_OBJ(!عدد_Matches || fromVtxp->datap()->state_عدد_Sigp, fromVtxp,
                            "Delay-ring incoming source missing stateCountSig");
                // IEEE 1800-2023 16.14.1/16.14.3: assertion alternatives belong to one
                // evaluation; only cover sequence counts each end-of-match.
                Ast_عقدة_تعبير* contribp
                    = عدد_Matches
                          ? fromVtxp->datap()->state_عدد_Sigp->clone_شجرة_Pure(خطأ)
                          : جديد AstExtend{c.flp,
                                          fromVtxp->datap()->stateSigp->clone_شجرة_Pure(خطأ),
                                          m_u32DTypep->width()};
                Ast_عقدة_تعبير* contribOverflowp
                    = عدد_Matches && fromVtxp->datap()->state_عدد_Overflow_Sigp
                          ? fromVtxp->datap()->state_عدد_Overflow_Sigp->clone_شجرة_Pure(خطأ)
                          : nullptr;
                إذا (tedger.m_condp) {
                    Ast_عقدة_تعبير* ثابت enablep = tedger.m_condp->clone_شجرة_Pure(خطأ);
                    contribp = عدد_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, enablep)
                                   : addCountp(c.flp, nullptr, contribp, enablep);
                }
                إذا (c.disableExprp) {
                    Ast_عقدة_تعبير* ثابت notDisp
                        = جديد AstLogNot{c.flp, c.disableExprp->clone_شجرة_Pure(خطأ)};
                    contribp = عدد_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, notDisp)
                                   : addCountp(c.flp, nullptr, contribp, notDisp);
                }
                incomingp = عدد_Matches ? addCheckedCountp(c.flp, incomingp, contribp,
                                                            incomingOverflowp, contribOverflowp)
                                         : addCountp(c.flp, incomingp, contribp);
            }
            UASSERT_OBJ(incomingp, vtxp, "Delay ring has no incoming edge");
            ثابت منطقي storesCounts = vtxp->datap()->delayRingCounts;
            Ast_عقدة_تعبير* ثابت incomingCountp
                = storesCounts
                      ? incomingp->clone_شجرة_Pure(خطأ)
                      : جديد AstExtend{c.flp,
                                      جديد AstNeq{c.flp, incomingp->clone_شجرة_Pure(خطأ),
                                                 newTypedConstp(c.flp, incomingp->dtypep(), 0)},
                                      m_u32DTypep->width()};
            Ast_عقدة_تعبير* ثابت storedIncomingp
                = storesCounts ? incomingp
                               : جديد AstNeq{c.flp, incomingp,
                                            newTypedConstp(c.flp, incomingp->dtypep(), 0)};
            // Only this block accesses the storage. Other processes read the cached head, so
            // the ring can be updated in place without an NBA shadow copy.
            AstAssign* ثابت writeIncomingp = جديد AstAssign{
                c.flp,
                delayRingSlot(c.flp, vtxp, جديد AstVarRef{c.flp, idxp, VAccess::READ},
                              VAccess::WRITE),
                storedIncomingp};
            Ast_عقدة* updateBodyp = writeIncomingp;
            // live_count <= live_count - outgoing_count + incoming_count;
            Ast_عقدة_تعبير* ثابت outgoingp = delay_Ring_Output_عدد(c.flp, vtxp);
            Ast_عقدة_تعبير* ثابت afterOutgoingp
                = جديد AstSub{c.flp, جديد AstVarRef{c.flp, live_عدد_Varp, VAccess::READ}, outgoingp};
            Ast_عقدة_تعبير* ثابت oldAfterOutgoingp = afterOutgoingp->clone_شجرة_Pure(خطأ);
            AstAdd* ثابت nextLiveCountp = جديد AstAdd{c.flp, afterOutgoingp, incomingCountp};
            إذا (storesCounts) {
                Ast_عقدة_تعبير* ثابت liveOverflowp
                    = جديد AstLt{c.flp, nextLiveCountp->clone_شجرة_Pure(خطأ), oldAfterOutgoingp};
                Ast_عقدة_تعبير* ثابت overflowp = orExprs(c.flp, incomingOverflowp, liveOverflowp);
                updateBodyp = new_عدد_Overflow_Warning(c.flp, overflowp);
                updateBodyp->addNext(writeIncomingp);
            } وإلا {
                VL_DO_DANGLING(oldAfterOutgoingp->delete_شجرة(), oldAfterOutgoingp);
            }
            // Cache the outgoing slot for the next index after the write. Reading after
            // writing also handles one-slot rings and two-slot range rings without alias cases.
            Ast_عقدة_تعبير* nextHeadIdxp
                = nextRingIndex(c.flp, جديد AstVarRef{c.flp, idxp, VAccess::READ}, size);
            إذا (!vtxp->m_isFixedDelayRing) {
                nextHeadIdxp = nextRingIndex(c.flp, nextHeadIdxp, size);
            }
            updateBodyp->addNext(جديد AstAssignDly{
                c.flp, جديد AstVarRef{c.flp, vtxp->datap()->delayRingHeadVarp, VAccess::WRITE},
                delayRingSlot(c.flp, vtxp, nextHeadIdxp)});
            updateBodyp->addNext(جديد AstAssignDly{
                c.flp, جديد AstVarRef{c.flp, live_عدد_Varp, VAccess::WRITE}, nextLiveCountp});
            // wrapped <= wrapped || idx == size - 1;
            updateBodyp->addNext(
                جديد AstAssignDly{c.flp, جديد AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                 جديد AstLogOr{c.flp, جديد AstVarRef{c.flp, wrappedp, VAccess::READ},
                                              delayRingAtLastIndex(c.flp, idxp, size)}});
            // idx <= next_idx;
            updateBodyp->addNext(جديد AstAssignDly{
                c.flp, جديد AstVarRef{c.flp, idxp, VAccess::WRITE},
                nextRingIndex(c.flp, جديد AstVarRef{c.flp, idxp, VAccess::READ}, size)});
            إذا (vtxp->m_delayRingAdvanceCondp) {
                updateBodyp = جديد AstIf{
                    c.flp, sampled(vtxp->m_delayRingAdvanceCondp->clone_شجرة_Pure(خطأ)),
                    updateBodyp};
            }

            Ast_عقدة_تعبير* clearCondp = killActive(c);
            إذا (vtxp->m_delayRingClearCondp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_delayRingClearCondp->clone_شجرة_Pure(خطأ)));
            }
            إذا (vtxp->m_abortClearp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_abortClearp->clone_شجرة_Pure(خطأ)));
            }
            إذا (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_شجرة_Pure(خطأ));
            }
            Ast_عقدة_تعبير* guardp = nullptr;
            لكل (Ast_عقدة_تعبير* ثابت cp : vtxp->m_throughoutConds) {
                Ast_عقدة_تعبير* ثابت sampledp = sampled(cp->clone_شجرة_Pure(خطأ));
                guardp = guardp ? static_cast<Ast_عقدة_تعبير*>(جديد AstLogAnd{c.flp, guardp, sampledp})
                                : sampledp;
            }
            إذا (guardp) clearCondp = orExprs(c.flp, clearCondp, جديد AstLogNot{c.flp, guardp});
            // Logically clear the ring without touching its wide storage.
            AstAssignDly* ثابت clearCountp
                = جديد AstAssignDly{c.flp, جديد AstVarRef{c.flp, live_عدد_Varp, VAccess::WRITE},
                                   newTypedConstp(c.flp, live_عدد_Varp->dtypep(), 0)};
            clearCountp->addNext(جديد AstAssignDly{c.flp,
                                                  جديد AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                                  جديد AstConst{c.flp, AstConst::BitFalse{}}});
            clearCountp->addNext(جديد AstAssignDly{c.flp,
                                                  جديد AstVarRef{c.flp, idxp, VAccess::WRITE},
                                                  newTypedConstp(c.flp, idxp->dtypep(), 0)});
            updateBodyp = جديد AstIf{c.flp, clearCondp, clearCountp, updateBodyp};

            addStateAlways(c, updateBodyp, asyncDisable ? clearCountp->clone_شجرة(true) : nullptr);
        }
    }

    // Phase 2c: SAnd combiner done-latch always block.
    // NBA semantics ensure doneL/doneR read pre-update values (IEEE 16.9.5).
    فراغ emitAndCombinerDoneLatchNba(LowerCtx& c) {
        ثابت منطقي asyncDisable = isAsyncDisable(c.disableExprp);
        لكل (ثابت V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ثابت SvaStateVertex* ثابت avp = static_cast<ثابت SvaStateVertex*>(&vtxr);
            إذا (!avp->datap()->doneLVarp) استمر;
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

            AstAssignDly* ثابت clearLp = جديد AstAssignDly{
                c.flp, جديد AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                جديد AstConst{c.flp, AstConst::BitFalse{}}};
            AstAssignDly* ثابت clearRp = جديد AstAssignDly{
                c.flp, جديد AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                جديد AstConst{c.flp, AstConst::BitFalse{}}};
            clearLp->addNext(clearRp);

            Ast_عقدة_تعبير* ثابت matchLNowp
                = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, avp->m_andLhsCondp);
            Ast_عقدة_تعبير* ثابت matchRNowp
                = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, avp->m_andRhsCondp);
            Ast_عقدة_تعبير* gateLp = matchLNowp;
            Ast_عقدة_تعبير* gateRp = matchRNowp;
            إذا (c.disableExprp) {
                Ast_عقدة_تعبير* ثابت notDisLp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_شجرة_Pure(خطأ)};
                gateLp = جديد AstLogAnd{c.flp, gateLp, notDisLp};
                Ast_عقدة_تعبير* ثابت notDisRp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_شجرة_Pure(خطأ)};
                gateRp = جديد AstLogAnd{c.flp, gateRp, notDisRp};
            }
            AstAssignDly* ثابت setLp = جديد AstAssignDly{
                c.flp, جديد AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                جديد AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* ثابت setLIfp = جديد AstIf{c.flp, gateLp, setLp, nullptr};
            AstAssignDly* ثابت setRp = جديد AstAssignDly{
                c.flp, جديد AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                جديد AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* ثابت setRIfp = جديد AstIf{c.flp, gateRp, setRp, nullptr};
            setLIfp->addNext(setRIfp);

            Ast_عقدة_تعبير* clearCondp = جديد AstLogOr{c.flp, killActive(c),
                                                   avp->datap()->stateSigp->clone_شجرة_Pure(خطأ)};
            إذا (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_شجرة_Pure(خطأ));
            }
            AstIf* ثابت topp = جديد AstIf{c.flp, clearCondp, clearLp, setLIfp};
            addStateAlways(c, topp, asyncDisable ? clearLp->clone_شجرة(true) : nullptr);
        }
    }

    فراغ emitKillAckNba(LowerCtx& c) {
        AstAssignDly* ثابت ackp
            = جديد AstAssignDly{c.flp, جديد AstVarRef{c.flp, c.killVarp, VAccess::WRITE},
                               assertKillGet(c.flp, c.assertType, c.directiveType)};
        m_modp->addStmtsp(
            جديد AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_شجرة(خطأ), ackp});
    }

    // Phase 3/3a/3b: Compute terminal match/reject signals, required-step reject,
    // throughout-drop reject; clean up intermediate state signals.
    // Phase 3: terminalActive and rejectBase from Links to matchVertex.
    // Builder only adds Links (non-clocked) to matchVertex via addLink in
    // wireMatchAndMidSources. For cover sequence, also count each end-of-match
    // so the action can be replayed without unrolling endpoints.
    فراغ computeTerminalMatchAndReject(LowerCtx& c, SignalSet& sigs, ثابت منطقي need_Match_عدد) {
        لكل (ثابت SvaTransEdge* ثابت tedgep : c.graph.allEdges()) {
            إذا (tedgep->toVtxp() != c.graph.m_matchVertexp) استمر;
            SvaStateVertex* ثابت fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                        "Terminal-link source missing stateSig");

            Ast_عقدة_تعبير* srcSigp = fromVtxp->datap()->stateSigp->clone_شجرة_Pure(خطأ);
            srcSigp = andCond(c.flp, srcSigp, tedgep->m_condp);
            إذا (need_Match_عدد) {
                UASSERT_OBJ(fromVtxp->datap()->state_عدد_Sigp, fromVtxp,
                            "Terminal-link source missing stateCountSig");
                Ast_عقدة_تعبير* contributionp
                    = fromVtxp->datap()->state_عدد_Sigp->clone_شجرة_Pure(خطأ);
                Ast_عقدة_تعبير* contributionOverflowp
                    = fromVtxp->datap()->state_عدد_Overflow_Sigp
                          ? fromVtxp->datap()->state_عدد_Overflow_Sigp->clone_شجرة_Pure(خطأ)
                          : nullptr;
                إذا (tedgep->m_condp) {
                    contributionp = gateCheckedCountp(c.flp, contributionp, contributionOverflowp,
                                                      tedgep->m_condp->clone_شجرة_Pure(خطأ));
                }
                sigs.matchCountp
                    = addCheckedCountp(c.flp, sigs.matchCountp, contributionp,
                                       sigs.match_عدد_Overflowp, contributionOverflowp);
            }

            إذا (fromVtxp->m_delayRingSize && !fromVtxp->m_isFixedDelayRing) {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_شجرة_Pure(خطأ));
                // reject |= (wrapped || idx == size - 1) && head && final_condition;
                Ast_عقدة_تعبير* expireContribp = delay_Ring_Output_عدد(c.flp, fromVtxp);
                expireContribp = andCond(c.flp, expireContribp, tedgep->m_condp);
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, expireContribp);
                VL_DO_DANGLING(srcSigp->delete_شجرة(), srcSigp);
            } وإلا إذا (fromVtxp->m_isUnbounded || fromVtxp->m_isAndCombiner) {
                sigs.terminalActivep = orExprs(c.flp, sigs.terminalActivep, srcSigp);
            } وإلا {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_شجرة_Pure(خطأ));
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, srcSigp);
            }
        }
        // wireMatchAndMidSources always adds a Link from result.termVertexp
        // to m_matchVertexp, so the loop above always sets terminalActivep.
        UASSERT_OBJ(sigs.terminalActivep, c.graph.m_matchVertexp,
                    "No terminal edge to match vertex");
    }

    Ast_عقدة_تعبير* newThroughoutThreadFailCountp(LowerCtx& c, AstVar* ثابت delay_Ring_Live_عدد_Varp,
                                               Ast_عقدة_تعبير* ثابت stateExprp,
                                               Ast_عقدة_تعبير* ثابت enablep) {
        Ast_عقدة_تعبير* ثابت activeThreadCountp
            = delay_Ring_Live_عدد_Varp
                  ? static_cast<Ast_عقدة_تعبير*>(
                        جديد AstVarRef{c.flp, delay_Ring_Live_عدد_Varp, VAccess::READ})
                  : جديد AstExtend{c.flp, stateExprp->clone_شجرة_Pure(خطأ), m_u32DTypep->width()};
        أرجع addCountp(c.flp, nullptr, activeThreadCountp, enablep);
    }

    // Phase 3b: Throughout-drop and ring-wide abort rejection.
    فراغ computeThroughoutReject(LowerCtx& c, SignalSet& sigs, ثابت منطقي need_Thread_Fail_عدد) {
        لكل (ثابت V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ثابت SvaStateVertex* ثابت vtxp = static_cast<ثابت SvaStateVertex*>(&vtxr);
            ثابت auto& conds = vtxp->m_throughoutConds;
            إذا (conds.empty() && !vtxp->m_abortRejectp) استمر;
            إذا (vtxp->m_isAndCombiner) استمر;
            Ast_عقدة_تعبير* stateExprp = nullptr;
            إذا (vtxp->datap()->stateVarp) {
                stateExprp = جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
            } وإلا إذا (vtxp->datap()->delayRingVarp && vtxp->m_isFixedDelayRing) {
                stateExprp
                    = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_عدد_Varp);
            } وإلا {
                UASSERT_OBJ(vtxp->datap()->stateSigp, vtxp,
                            "Throughout-conds vertex missing state representation");
                stateExprp = vtxp->datap()->stateSigp->clone_شجرة_Pure(خطأ);
            }
            Ast_عقدة_تعبير* guardp = nullptr;
            لكل (Ast_عقدة_تعبير* ثابت cp : conds) {
                Ast_عقدة_تعبير* ثابت sp = sampled(cp->clone_شجرة_Pure(خطأ));
                guardp = guardp ? static_cast<Ast_عقدة_تعبير*>(جديد AstLogAnd{c.flp, guardp, sp}) : sp;
            }
            إذا (vtxp->m_abortRejectp) {
                Ast_عقدة_تعبير* ثابت notAbortp
                    = جديد AstLogNot{c.flp, sampled(vtxp->m_abortRejectp->clone_شجرة_Pure(خطأ))};
                guardp = guardp
                             ? static_cast<Ast_عقدة_تعبير*>(جديد AstLogAnd{c.flp, guardp, notAbortp})
                             : notAbortp;
            }
            Ast_عقدة_تعبير* rejectCondp = جديد AstLogNot{c.flp, guardp};
            إذا (vtxp->m_abortClearp) {
                // Any abort clears the ring. Accept aborts suppress a simultaneous guard failure;
                // reject aborts are restored below so they still force rejection.
                Ast_عقدة_تعبير* ثابت notAbortClearp
                    = جديد AstLogNot{c.flp, sampled(vtxp->m_abortClearp->clone_شجرة_Pure(خطأ))};
                rejectCondp = جديد AstLogAnd{c.flp, rejectCondp, notAbortClearp};
                إذا (vtxp->m_abortRejectp) {
                    rejectCondp = جديد AstLogOr{
                        c.flp, rejectCondp, sampled(vtxp->m_abortRejectp->clone_شجرة_Pure(خطأ))};
                }
            }
            إذا (need_Thread_Fail_عدد) {
                Ast_عقدة_تعبير* ثابت contributionp
                    = newThroughoutThreadFailCountp(c, vtxp->datap()->delay_Ring_Live_عدد_Varp,
                                                    stateExprp, rejectCondp->clone_شجرة_Pure(خطأ));
                sigs.threadFailCountp = addCountp(c.flp, sigs.threadFailCountp, contributionp);
                إذا (vtxp->m_abortRejectp && vtxp->m_delayRingAdvanceCondp) {
                    // An advancing ring thread also occupies its same-tick match vertex in the
                    // unrolled NFA, so abort rejection must replay both fail actions.
                    Ast_عقدة_تعبير* ثابت abortAndAdvancep = جديد AstLogAnd{
                        c.flp, sampled(vtxp->m_abortRejectp->clone_شجرة_Pure(خطأ)),
                        sampled(vtxp->m_delayRingAdvanceCondp->clone_شجرة_Pure(خطأ))};
                    Ast_عقدة_تعبير* ثابت matchContributionp = newThroughoutThreadFailCountp(
                        c, vtxp->datap()->delay_Ring_Live_عدد_Varp, stateExprp, abortAndAdvancep);
                    sigs.threadFailCountp
                        = addCountp(c.flp, sigs.threadFailCountp, matchContributionp);
                }
            }
            sigs.throughoutRejectp = orExprs(c.flp, sigs.throughoutRejectp,
                                             جديد AstLogAnd{c.flp, stateExprp, rejectCondp});
        }
    }

    SignalSet computeSignals(LowerCtx& c, ثابت منطقي need_Thread_Fail_عدد,
                             ثابت منطقي need_Throughout_Thread_Fail_عدد, ثابت منطقي need_Match_عدد) {
        SignalSet sigs;

        computeTerminalMatchAndReject(c, sigs, need_Match_عدد);

        // Phase 3a: required-step rejection.
        // Builder only sets m_rejectOnFail on non-clocked Links with m_condp
        // or m_condVtxp, and the source always has a resolved stateSig.
        لكل (ثابت SvaTransEdge* ثابت tedgep : c.graph.allEdges()) {
            إذا (!tedgep->m_rejectOnFail) استمر;
            ثابت SvaStateVertex* ثابت fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp && (tedgep->m_condp || tedgep->m_condVtxp),
                        fromVtxp,
                        "rejectOnFail Link must have condp/condVtxp and source stateSig");
            Ast_عقدة_تعبير* ثابت srcSigp = fromVtxp->datap()->stateSigp->clone_شجرة_Pure(خطأ);
            Ast_عقدة_تعبير* condp = nullptr;
            إذا (tedgep->m_condVtxp) {
                UASSERT_OBJ(tedgep->m_condVtxp->datap()->stateSigp, tedgep->m_condVtxp,
                            "rejectOnFail condVtxp missing stateSig");
                condp = tedgep->m_condVtxp->datap()->stateSigp->clone_شجرة_Pure(خطأ);
                إذا (tedgep->m_condp) {
                    condp = جديد AstLogOr{c.flp, condp, tedgep->m_condp->clone_شجرة_Pure(خطأ)};
                }
            } وإلا {
                condp = tedgep->m_condp->clone_شجرة_Pure(خطأ);
            }
            Ast_عقدة_تعبير* ثابت notCondp = جديد AstLogNot{c.flp, condp};
            Ast_عقدة_تعبير* ثابت rawFailp = جديد AstLogAnd{c.flp, srcSigp, notCondp};
            إذا (need_Thread_Fail_عدد) {
                // thread_fail_count += fail;
                sigs.threadFailCountp = addCountp(
                    c.flp, sigs.threadFailCountp,
                    جديد AstExtend{c.flp, rawFailp->clone_شجرة_Pure(خطأ), m_u32DTypep->width()});
            }
            Ast_عقدة_تعبير* ثابت failp = gateNotKill(c, rawFailp);
            sigs.requiredStepRejectp = orExprs(c.flp, sigs.requiredStepRejectp, failp);
        }

        computeThroughoutReject(c, sigs, need_Throughout_Thread_Fail_عدد);
        إذا (sigs.threadFailCountp) {
            sigs.threadFailCountp
                = addCountp(c.flp, nullptr, sigs.threadFailCountp, notKillActive(c));
        }
        إذا (sigs.matchCountp) {
            إذا (c.matchCondp) {
                sigs.matchCountp
                    = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_عدد_Overflowp,
                                        sampled(c.matchCondp->clone_شجرة_Pure(خطأ)));
            }
            sigs.matchCountp = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_عدد_Overflowp,
                                                 notKillActive(c));
        }
        sigs.terminalActivep = gateNotKill(c, sigs.terminalActivep);
        sigs.rejectBasep = gateNotKill(c, sigs.rejectBasep);
        sigs.throughoutRejectp = gateNotKill(c, sigs.throughoutRejectp);

        // Clean up intermediate state signals. These are orphan subtrees
        // (never linked into the enclosing AST); deleteTree() is immediate
        // which is what we want since stateSigp lifetime ends with this scope.
        لكل (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ثابت vtxp = static_cast<SvaStateVertex*>(&vtxr);
            Ast_عقدة_تعبير*& sigp = vtxp->datap()->stateSigp;
            إذا (sigp) VL_DO_DANGLING(sigp->delete_شجرة(), sigp);
            Ast_عقدة_تعبير*& countp = vtxp->datap()->state_عدد_Sigp;
            إذا (countp) VL_DO_DANGLING(countp->delete_شجرة(), countp);
            Ast_عقدة_تعبير*& overflowp = vtxp->datap()->state_عدد_Overflow_Sigp;
            إذا (overflowp) VL_DO_DANGLING(overflowp->delete_شجرة(), overflowp);
        }
        // Disable iff gating (IEEE 1800-2023 16.12) on the current level value.
        إذا (c.disableExprp) {
            // terminalActivep is always set, so gate it unconditionally.
            Ast_عقدة_تعبير* ثابت notTermp
                = جديد AstLogNot{c.flp, c.disableExprp->clone_شجرة_Pure(خطأ)};
            sigs.terminalActivep = جديد AstLogAnd{c.flp, sigs.terminalActivep, notTermp};
            إذا (sigs.rejectBasep) {
                Ast_عقدة_تعبير* ثابت notDisp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_شجرة_Pure(خطأ)};
                sigs.rejectBasep = جديد AstLogAnd{c.flp, sigs.rejectBasep, notDisp};
            }
            إذا (sigs.throughoutRejectp) {
                Ast_عقدة_تعبير* ثابت notDisp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_شجرة_Pure(خطأ)};
                sigs.throughoutRejectp = جديد AstLogAnd{c.flp, sigs.throughoutRejectp, notDisp};
            }
            إذا (sigs.requiredStepRejectp) {
                Ast_عقدة_تعبير* ثابت notDisp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_شجرة_Pure(خطأ)};
                sigs.requiredStepRejectp = جديد AstLogAnd{c.flp, sigs.requiredStepRejectp, notDisp};
            }
            إذا (sigs.matchCountp) {
                sigs.matchCountp = gateCheckedCountp(
                    c.flp, sigs.matchCountp, sigs.match_عدد_Overflowp,
                    جديد AstLogNot{c.flp, c.disableExprp->clone_شجرة_Pure(خطأ)});
            }
        }

        إذا (sigs.match_عدد_Overflowp) {
            m_modp->addStmtsp(
                جديد AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_شجرة(خطأ),
                              new_عدد_Overflow_Warning(c.flp, sigs.match_عدد_Overflowp)});
            sigs.match_عدد_Overflowp = nullptr;
        }

        أرجع sigs;
    }

    // Phase 1: Resolve combinational Links via fixed-point propagation.
    فراغ resolveLinks(LowerCtx& c, Ast_عقدة_تعبير* triggerExprp, ثابت منطقي عدد_Matches) {
        // datap() was freshly allocated in lower() -- all state signals start null.
        c.graph.m_startVertexp->datap()->stateSigp = triggerExprp->clone_شجرة_Pure(خطأ);
        c.graph.m_startVertexp->datap()->state_عدد_Sigp
            = جديد AstExtend{c.flp, triggerExprp->clone_شجرة_Pure(خطأ), m_u32DTypep->width()};
        لكل (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ثابت vtxp = static_cast<SvaStateVertex*>(&vtxr);
            إذا (vtxp->datap()->stateVarp) {
                vtxp->datap()->stateSigp
                    = جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
                vtxp->datap()->state_عدد_Sigp = جديد AstExtend{
                    c.flp, جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ},
                    m_u32DTypep->width()};
            } وإلا إذا (vtxp->datap()->delayRingVarp) {
                إذا (vtxp->m_isFixedDelayRing) {
                    // state_count = wrapped ? head : 0; state = state_count != 0;
                    Ast_عقدة_تعبير* ثابت countp = delay_Ring_Output_عدد(c.flp, vtxp);
                    vtxp->datap()->stateSigp = جديد AstNeq{c.flp, countp->clone_شجرة_Pure(خطأ),
                                                          newTypedConstp(c.flp, m_u32DTypep, 0)};
                    vtxp->datap()->state_عدد_Sigp = countp;
                } وإلا {
                    vtxp->datap()->stateSigp
                        = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_عدد_Varp);
                    vtxp->datap()->state_عدد_Sigp = جديد AstVarRef{
                        c.flp, vtxp->datap()->delay_Ring_Live_عدد_Varp, VAccess::READ};
                }
            }
        }
        // Fixed-point propagation along zero-delay (Link) edges. Rebuild each
        // derived signal from its incoming edges on every pass; appending the same
        // contributions repeatedly makes merge expressions grow exponentially.
        ثابت size_t passes = 2 * c.graph.m_graph.vertices().size() + 2;
        لكل (size_t pass = 0; pass < passes; ++pass) {
            // Rebuild SAnd combiners once both sub-NFA terminals are available.
            لكل (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* ثابت vtxp = static_cast<SvaStateVertex*>(&vtxr);
                إذا (!vtxp->m_isAndCombiner) استمر;
                // AndCombiner vertices always have both terminal pointers set.
                UASSERT_OBJ(vtxp->m_andLhsTermp && vtxp->m_andRhsTermp, vtxp,
                            "AndCombiner vertex missing LHS/RHS terminal");
                ثابت SvaStateVertex* ثابت lhsTermp = vtxp->m_andLhsTermp;
                ثابت SvaStateVertex* ثابت rhsTermp = vtxp->m_andRhsTermp;
                إذا (!lhsTermp->datap()->stateSigp || !rhsTermp->datap()->stateSigp) استمر;
                Ast_عقدة_تعبير* ثابت matchLp
                    = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, vtxp->m_andLhsCondp);
                Ast_عقدة_تعبير* ثابت matchRp
                    = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, vtxp->m_andRhsCondp);
                Ast_عقدة_تعبير* ثابت doneLOrp = جديد AstLogOr{
                    c.flp, جديد AstVarRef{c.flp, vtxp->datap()->doneLVarp, VAccess::READ}, matchLp};
                Ast_عقدة_تعبير* ثابت doneROrp = جديد AstLogOr{
                    c.flp, جديد AstVarRef{c.flp, vtxp->datap()->doneRVarp, VAccess::READ}, matchRp};
                Ast_عقدة_تعبير* ثابت bothp = جديد AstLogAnd{c.flp, doneLOrp, doneROrp};
                Ast_عقدة_تعبير* ثابت oneNowp = جديد AstLogOr{c.flp, matchLp->clone_شجرة_Pure(خطأ),
                                                          matchRp->clone_شجرة_Pure(خطأ)};
                إذا (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_شجرة(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = جديد AstLogAnd{c.flp, bothp, oneNowp};
                إذا (vtxp->datap()->state_عدد_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_عدد_Sigp->delete_شجرة(),
                                   vtxp->datap()->state_عدد_Sigp);
                }
                vtxp->datap()->state_عدد_Sigp = جديد AstExtend{
                    c.flp, vtxp->datap()->stateSigp->clone_شجرة_Pure(خطأ), m_u32DTypep->width()};
            }

            لكل (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* ثابت vtxp = static_cast<SvaStateVertex*>(&vtxr);
                إذا (vtxp == c.graph.m_startVertexp || vtxp->datap()->stateVarp
                    || vtxp->datap()->delayRingVarp || vtxp->m_isAndCombiner || vtxp->m_isMatch
                    || vtxp->m_isRejectSink) {
                    استمر;
                }
                Ast_عقدة_تعبير* nextStatep = nullptr;
                Ast_عقدة_تعبير* nextStateCountp = nullptr;
                Ast_عقدة_تعبير* next_State_عدد_Overflowp = nullptr;
                لكل (ثابت V3GraphEdge& er : vtxp->inEdges()) {
                    ثابت SvaTransEdge& te = static_cast<ثابت SvaTransEdge&>(er);
                    ثابت SvaStateVertex* ثابت fromVtxp = te.fromVtxp();
                    إذا (!fromVtxp->datap()->stateSigp) استمر;
                    UASSERT_OBJ(fromVtxp->datap()->state_عدد_Sigp, fromVtxp,
                                "Link source missing stateCountSig");
                    Ast_عقدة_تعبير* ثابت contributionp = andCond(
                        c.flp, fromVtxp->datap()->stateSigp->clone_شجرة_Pure(خطأ), te.m_condp);
                    nextStatep = orExprs(c.flp, nextStatep, contributionp);
                    Ast_عقدة_تعبير* عدد_Contributionp
                        = fromVtxp->datap()->state_عدد_Sigp->clone_شجرة_Pure(خطأ);
                    Ast_عقدة_تعبير* عدد_Contribution_Overflowp
                        = عدد_Matches && fromVtxp->datap()->state_عدد_Overflow_Sigp
                              ? fromVtxp->datap()->state_عدد_Overflow_Sigp->clone_شجرة_Pure(خطأ)
                              : nullptr;
                    إذا (te.m_condp) {
                        Ast_عقدة_تعبير* ثابت enablep = te.m_condp->clone_شجرة_Pure(خطأ);
                        عدد_Contributionp
                            = عدد_Matches
                                  ? gateCheckedCountp(c.flp, عدد_Contributionp,
                                                      عدد_Contribution_Overflowp, enablep)
                                  : addCountp(c.flp, nullptr, عدد_Contributionp, enablep);
                    }
                    nextStateCountp
                        = عدد_Matches
                              ? addCheckedCountp(c.flp, nextStateCountp, عدد_Contributionp,
                                                 next_State_عدد_Overflowp,
                                                 عدد_Contribution_Overflowp)
                              : addCountp(c.flp, nextStateCountp, عدد_Contributionp);
                }
                إذا (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_شجرة(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = nextStatep;
                إذا (vtxp->datap()->state_عدد_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_عدد_Sigp->delete_شجرة(),
                                   vtxp->datap()->state_عدد_Sigp);
                }
                vtxp->datap()->state_عدد_Sigp = nextStateCountp;
                إذا (vtxp->datap()->state_عدد_Overflow_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_عدد_Overflow_Sigp->delete_شجرة(),
                                   vtxp->datap()->state_عدد_Overflow_Sigp);
                }
                vtxp->datap()->state_عدد_Overflow_Sigp = next_State_عدد_Overflowp;
            }
        }
    }

عام:
    // Combine terminal/reject signals into final output expression.
    ساكن Ast_عقدة_تعبير* assembleResult(Ast_عقدة_Cover_Or_Assert* ثابت assertp, ثابت منطقي negated,
                                       Ast_عقدة_تعبير* ثابت matchCondp, ثابت SignalSet& sigs,
                                       Ast_عقدة_تعبير** ثابت outMatchpp) {
        File_سطر* ثابت flp = assertp->fileline();
        ثابت منطقي isCover = VN_IS(assertp, Cover);
        Ast_عقدة_تعبير* ثابت terminalActivep = sigs.terminalActivep;
        Ast_عقدة_تعبير* ثابت rejectBasep = sigs.rejectBasep;
        Ast_عقدة_تعبير* ثابت throughoutRejectp = sigs.throughoutRejectp;
        Ast_عقدة_تعبير* ثابت requiredStepRejectp = sigs.requiredStepRejectp;
        // Property negation (IEEE 1800-2023 16.12.1 `not`): invert match/reject.
        إذا (negated) {
            إذا (isCover) {
                إذا (terminalActivep)
                    VL_DO_DANGLING(terminalActivep->delete_شجرة(), terminalActivep);
                Ast_عقدة_تعبير* negRejectp = nullptr;
                إذا (matchCondp && rejectBasep) {
                    Ast_عقدة_تعبير* ثابت sampledCondp = sampled(matchCondp->clone_شجرة_Pure(خطأ));
                    Ast_عقدة_تعبير* ثابت notCondp = جديد AstLogNot{flp, sampledCondp};
                    negRejectp = جديد AstLogAnd{flp, rejectBasep, notCondp};
                } وإلا إذا (rejectBasep) {
                    VL_DO_DANGLING(rejectBasep->delete_شجرة(), rejectBasep);
                }
                إذا (throughoutRejectp) negRejectp = orExprs(flp, negRejectp, throughoutRejectp);
                إذا (requiredStepRejectp)
                    negRejectp = orExprs(flp, negRejectp, requiredStepRejectp);
                أرجع negRejectp ? negRejectp : جديد AstConst{flp, AstConst::BitFalse{}};
            }
            // Negated assert/assume: output = !match.
            Ast_عقدة_تعبير* matchp = terminalActivep;
            إذا (matchCondp) {
                Ast_عقدة_تعبير* ثابت sampledCondp = sampled(matchCondp->clone_شجرة_Pure(خطأ));
                matchp = جديد AstLogAnd{flp, matchp, sampledCondp};
            }
            إذا (outMatchpp) {
                Ast_عقدة_تعبير* notPMatchp = nullptr;
                إذا (matchCondp && rejectBasep) {
                    Ast_عقدة_تعبير* ثابت sampledCondp = sampled(matchCondp->clone_شجرة_Pure(خطأ));
                    notPMatchp = جديد AstLogAnd{flp, rejectBasep->clone_شجرة_Pure(خطأ),
                                               جديد AstLogNot{flp, sampledCondp}};
                } وإلا إذا (rejectBasep) {
                    notPMatchp = rejectBasep->clone_شجرة_Pure(خطأ);
                }
                إذا (throughoutRejectp)
                    notPMatchp = orExprs(flp, notPMatchp, throughoutRejectp->clone_شجرة_Pure(خطأ));
                إذا (requiredStepRejectp)
                    notPMatchp
                        = orExprs(flp, notPMatchp, requiredStepRejectp->clone_شجرة_Pure(خطأ));
                *outMatchpp = notPMatchp;
            }
            إذا (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_شجرة(), throughoutRejectp);
            إذا (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_شجرة(), rejectBasep);
            إذا (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_شجرة(), requiredStepRejectp);
            Ast_عقدة_تعبير* ثابت resultExprp = جديد AstLogNot{flp, matchp};
            أرجع resultExprp;
        }
        إذا (isCover) {
            إذا (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_شجرة(), throughoutRejectp);
            إذا (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_شجرة(), rejectBasep);
            إذا (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_شجرة(), requiredStepRejectp);
            إذا (matchCondp) {
                Ast_عقدة_تعبير* ثابت sampledCondp = sampled(matchCondp->clone_شجرة_Pure(خطأ));
                أرجع جديد AstLogAnd{flp, terminalActivep, sampledCondp};
            }
            أرجع terminalActivep;
        }
        // Assert/assume: output = !reject
        Ast_عقدة_تعبير* rejectp = nullptr;
        إذا (matchCondp && rejectBasep) {
            Ast_عقدة_تعبير* ثابت sampledCondp = sampled(matchCondp->clone_شجرة_Pure(خطأ));
            rejectp = جديد AstLogAnd{flp, rejectBasep, جديد AstLogNot{flp, sampledCondp}};
        } وإلا إذا (rejectBasep) {
            VL_DO_DANGLING(rejectBasep->delete_شجرة(), rejectBasep);
        }
        إذا (outMatchpp) {
            Ast_عقدة_تعبير* matchExprp = terminalActivep->clone_شجرة_Pure(خطأ);
            إذا (matchCondp) {
                Ast_عقدة_تعبير* ثابت sp = sampled(matchCondp->clone_شجرة_Pure(خطأ));
                matchExprp = جديد AstLogAnd{flp, matchExprp, sp};
            }
            *outMatchpp = matchExprp;
        }
        إذا (terminalActivep) VL_DO_DANGLING(terminalActivep->delete_شجرة(), terminalActivep);
        إذا (throughoutRejectp) rejectp = orExprs(flp, rejectp, throughoutRejectp);
        إذا (requiredStepRejectp) rejectp = orExprs(flp, rejectp, requiredStepRejectp);
        إذا (!rejectp) أرجع جديد AstConst{flp, AstConst::BitTrue{}};
        Ast_عقدة_تعبير* ثابت resultExprp = جديد AstLogNot{flp, rejectp};
        أرجع resultExprp;
    }

    explicit SvaNfaLowering(Ast_عقدة_Module* modp)
        : m_modp{modp}
        , m_u32DTypep{modp->findBasicDType(VBasicDTypeKwd::UINT32)} {}
    ~SvaNfaLowering() {
        V3Stats::addStatSum("Assertions, NFA delay ring edge visits", m_statDelayRingEdgeVisits);
    }

    // Lower NFA graph to synthesizable AstAlways blocks and raw result signals.
    // Links are combinational; Edges are registered (NBA).
    SignalSet lower(Ast_عقدة_Cover_Or_Assert* ثابت assertp, SvaGraph& graph,
                    Ast_Sen_شجرة* ثابت senTreep, Ast_عقدة_تعبير* ثابت matchCondp,
                    Ast_عقدة_تعبير* ثابت disableExprp, ثابت منطقي need_Thread_Fail_عدد,
                    ثابت منطقي need_Throughout_Thread_Fail_عدد) {
        File_سطر* ثابت flp = assertp->fileline();
        AstCover* ثابت coverp = VN_CAST(assertp, Cover);
        ثابت منطقي isSeqEvent = coverp && coverp->isSeqEvent();
        ثابت VAssertType assertType
            = isSeqEvent ? VAssertType{VAssertType::INTERNAL} : assertp->userType();
        ثابت VAssertDirectiveType directiveType
            = isSeqEvent ? VAssertDirectiveType{VAssertDirectiveType::INTERNAL}
                         : assertp->directive();
        ثابت std::string baseName = m_names.get("");

        // Number vertices with sequential colors for array indexing.
        صحيح N = 0;
        لكل (V3GraphVertex& vtxr : graph.m_graph.vertices()) { vtxr.color(N++); }
        std::vector<SvaStateVertex*> vtx(N, nullptr);
        لكل (V3GraphVertex& vtxr : graph.m_graph.vertices()) {
            vtx[vtxr.color()] = static_cast<SvaStateVertex*>(&vtxr);
        }

        // Allocate per-vertex lowering data (stored via V3GraphVertex::userp()).
        std::vector<SvaVertexData> vertexData(N);
        لكل (صحيح i = 0; i < N; ++i) vtx[i]->userp(&vertexData[i]);

        // Identify registered vertices (targets of clocked edges).
        لكل (صحيح i = 0; i < N; ++i) {
            لكل (ثابت V3GraphEdge& edger : vtx[i]->outEdges()) {
                ثابت SvaTransEdge& tedger = static_cast<ثابت SvaTransEdge&>(edger);
                ثابت صحيح toIdx = tedger.toVtxp()->color();
                إذا (tedger.m_consumesCycle && tedger.toVtxp() != graph.m_matchVertexp
                    && !tedger.toVtxp()->m_isRejectSink) {
                    vtx[toIdx]->datap()->needsReg = true;
                }
            }
        }

        // A packed range ring can emit several live endpoints without needing per-slot counts.
        // Widen only a ring whose input can already carry multiplicity.
        std::vector<منطقي> stateMayHaveMultiplicity(N);
        std::vector<منطقي> ringNeedsMultiplicity(N);
        std::vector<صحيح> multiplicityWork;
        ثابت منطقي عدد_Matches = coverp && coverp->isCoverSeq() && !isSeqEvent;
        إذا (عدد_Matches) {
            لكل (صحيح i = 0; i < N; ++i) {
                ثابت منطقي multipleInputs = vtx[i]->inEdges().size() > 1;
                ثابت منطقي isRing = vtx[i]->m_delayRingSize;
                ringNeedsMultiplicity[i] = isRing && multipleInputs;
                ثابت منطقي startsMultiplicity = isRing
                                                    ? !vtx[i]->m_isFixedDelayRing
                                                    : multipleInputs && !vtx[i]->datap()->needsReg;
                إذا (startsMultiplicity) {
                    stateMayHaveMultiplicity[i] = true;
                    multiplicityWork.ادفع_back(i);
                }
            }
            لكل (size_t wi = 0; wi < multiplicityWork.size(); ++wi) {
                لكل (ثابت V3GraphEdge& edger : vtx[multiplicityWork[wi]]->outEdges()) {
                    ثابت SvaTransEdge& tedger = static_cast<ثابت SvaTransEdge&>(edger);
                    ثابت صحيح ti = tedger.toVtxp()->color();
                    ثابت منطقي isRing = vtx[ti]->m_delayRingSize;
                    إذا (isRing) ringNeedsMultiplicity[ti] = true;
                    // Match and reject sinks have no outgoing edges, so they stop naturally.
                    ثابت منطقي stopsMultiplicity = !isRing && vtx[ti]->datap()->needsReg;
                    إذا (stateMayHaveMultiplicity[ti] || stopsMultiplicity) { استمر; }
                    stateMayHaveMultiplicity[ti] = true;
                    multiplicityWork.ادفع_back(ti);
                }
            }
        }

        ثابت منطقي asyncDisable = isAsyncDisable(disableExprp);
        // Registers cleared on a rising disable are also written by their clocked block.
        File_سطر* ثابت regFlp = asyncDisable ? جديد File_سطر{flp} : flp;
        إذا (asyncDisable) regFlp->warnOff(V3ErrorCode::MULTIDRIVEN, true);

        AstVar* ثابت killVarp
            = جديد AstVar{flp, VVarType::MODULETEMP, baseName + "__kill", m_u32DTypep};
        killVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(killVarp);
        لكل (صحيح i = 0; i < N; ++i) {
            إذا (vtx[i]->m_isAndCombiner) {
                ثابت std::string base = baseName + "__a" + std::to_string(i);
                AstVar* ثابت lp = جديد AstVar{regFlp, VVarType::MODULETEMP, base + "_doneL",
                                              m_modp->findBitDType()};
                lp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(lp);
                vtx[i]->datap()->doneLVarp = lp;
                AstVar* ثابت rp = جديد AstVar{regFlp, VVarType::MODULETEMP, base + "_doneR",
                                              m_modp->findBitDType()};
                rp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(rp);
                vtx[i]->datap()->doneRVarp = rp;
                استمر;
            }
            إذا (vtx[i]->m_delayRingSize) {
                ثابت std::string base = baseName + "__d" + std::to_string(i);
                ثابت منطقي storesCounts = عدد_Matches && ringNeedsMultiplicity[i];
                Ast_عقدة_DType* ringDTypep = nullptr;
                إذا (storesCounts) {
                    // int unsigned ring [size];
                    ringDTypep = جديد AstUnpackArrayDType{
                        flp, m_u32DTypep,
                        جديد AstRange{flp, static_cast<صحيح>(vtx[i]->m_delayRingSize - 1), 0}};
                    v3Global.rootp()->typeTablep()->addTypesp(ringDTypep);
                } وإلا {
                    // bit [size-1:0] ring;
                    ringDTypep = m_modp->findBitDType(vtx[i]->m_delayRingSize,
                                                      vtx[i]->m_delayRingSize, VSigning::UNSIGNED);
                }
                AstVar* ثابت ringp
                    = جديد AstVar{flp, VVarType::MODULETEMP, base + "_ring", ringDTypep};
                ringp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(ringp);
                vtx[i]->datap()->delayRingVarp = ringp;
                vtx[i]->datap()->delayRingCounts = storesCounts;
                AstVar* ثابت headp
                    = جديد AstVar{flp, VVarType::MODULETEMP, base + "_head",
                                 storesCounts ? m_u32DTypep : m_modp->findBitDType()};
                headp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(headp);
                vtx[i]->datap()->delayRingHeadVarp = headp;
                // int unsigned idx;
                AstVar* ثابت idxp
                    = جديد AstVar{regFlp, VVarType::MODULETEMP, base + "_idx", m_u32DTypep};
                idxp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(idxp);
                vtx[i]->datap()->delayRingIdxVarp = idxp;
                // int unsigned live_count;
                AstVar* ثابت live_عدد_Varp
                    = جديد AstVar{regFlp, VVarType::MODULETEMP, base + "_liveCount", m_u32DTypep};
                live_عدد_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(live_عدد_Varp);
                vtx[i]->datap()->delay_Ring_Live_عدد_Varp = live_عدد_Varp;
                AstVar* ثابت wrappedp = جديد AstVar{regFlp, VVarType::MODULETEMP,
                                                    base + "_wrapped", m_modp->findBitDType()};
                wrappedp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(wrappedp);
                vtx[i]->datap()->delayRingWrappedVarp = wrappedp;
                استمر;
            }
            إذا (!vtx[i]->datap()->needsReg) استمر;
            إذا (vtx[i] == graph.m_startVertexp || vtx[i]->m_isMatch) استمر;
            ثابت std::string varName = baseName + "__s" + std::to_string(i);
            AstVar* ثابت varp
                = جديد AstVar{regFlp, VVarType::MODULETEMP, varName, m_modp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(varp);
            vtx[i]->datap()->stateVarp = varp;
        }

        // Build lowering context for phase sub-functions.
        LowerCtx c{flp,        senTreep,      disableExprp, matchCondp,
                   assertType, directiveType, killVarp,     graph};

        // Phase 1: Resolve combinational Links via fixed-point propagation.
        Ast_عقدة_تعبير* ثابت triggerExprp
            = isSeqEvent ? جديد AstConst{flp, AstConst::BitTrue{}}
                         : assertOnCond(flp, assertp->userType(), assertp->directive());
        resolveLinks(c, triggerExprp, عدد_Matches);
        VL_DO_DANGLING(triggerExprp->delete_شجرة(), triggerExprp);

        // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
        emitStateRegisterNba(c);
        emitDelayRingUpdates(c, عدد_Matches);
        emitAndCombinerDoneLatchNba(c);
        emitKillAckNba(c);

        // Phase 3/3a/3b: Compute terminal match/reject signals (cleans up stateSig).
        ثابت SignalSet sigs
            = computeSignals(c, need_Thread_Fail_عدد, need_Throughout_Thread_Fail_عدد, عدد_Matches);

        // Strong s_always[m:n] end-of-simulation liveness: if any in-window state
        // is still set at $finish, the universal-quantifier window never completed
        // (IEEE 1800-2023 16.12.11 strong semantics). Fire the assertion failure
        // from a final block; V3Assert turns the DT_ERROR display into the standard
        // "Assertion failed in %m" message.
        // pending = |{strong state registers, strong delay live counts != 0};
        Ast_عقدة_تعبير* pendingp = nullptr;
        لكل (صحيح i = 0; i < N; ++i) {
            إذا (!vtx[i]->m_strongPending) استمر;
            Ast_عقدة_تعبير* pendingExprp = nullptr;
            إذا (vtx[i]->datap()->stateVarp) {
                pendingExprp = جديد AstVarRef{flp, vtx[i]->datap()->stateVarp, VAccess::READ};
            } وإلا إذا (vtx[i]->m_strongAlwaysRing) {
                pendingExprp
                    = delayRingHasLiveThreadsp(flp, vtx[i]->datap()->delay_Ring_Live_عدد_Varp);
            } وإلا {
                استمر;
            }
            إذا (!pendingp) {
                pendingp = pendingExprp;
            } وإلا {
                pendingp = جديد AstLogOr{flp, pendingp, pendingExprp};
            }
        }
        إذا (pendingp) {
            AstCExpr* ثابت assertOnp
                = جديد AstCExpr{flp, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()", 1};
            Ast_عقدة_تعبير* ثابت condp = جديد AstLogAnd{flp, assertOnp, pendingp};
            AstDisplay* ثابت dispp
                = جديد AstDisplay{flp, VDisplayType::DT_ERROR, "", nullptr, nullptr};
            dispp->fmtp()->timeunit(m_modp->timeunit());
            Ast_عقدة_Stmt* ثابت firep = dispp;
            إذا (v3Global.opt.stopFail()) firep->addNext(جديد AstStop{flp, خطأ});
            m_modp->addStmtsp(جديد AstFinal{flp, جديد AstIf{flp, condp, firep}});
        }

        // Clear userp on every vertex before vertexData is destroyed.
        لكل (صحيح i = 0; i < N; ++i) vtx[i]->userp(nullptr);
        أرجع sigs;
    }
};

}  // namespace

//######################################################################
// Top-level visitor

صنف AssertNfaVisitor final : عام VNVisitor {
    // STATE
    Ast_عقدة_Module* m_modp = nullptr;  // Current module being processed
    AstClocking* m_defaultClockingp = nullptr;  // Default clocking
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff
    SvaNfaLowering* m_loweringp = nullptr;  // NFA-to-hardware lowering engine
    Ast_Sen_شجرة* m_sampled_قيمة_Clockp = nullptr;  // Inherited clock during scoped attachment
    V3UniqueNames m_propVarNames{"__Vpropvar"};  // Property-local variable names
    V3UniqueNames m_propTempNames{"__VnfaSampled"};  // Hoisted $sampled(propp) temps
    V3UniqueNames m_fail_عدد_Names{"__VnfaRemainingFailCount"};  // Fail replay counter names
    std::set<ثابت AstProperty*> m_inliningProps;  // Recursion guard for inlineNamedProperty

    template <typename T_عقدة>
    فراغ visit_Sampled_قيمة(T_عقدة* ثابت nodep) {
        إذا (m_sampled_قيمة_Clockp && !nodep->sentreep()) {
            nodep->sentreep(m_sampled_قيمة_Clockp->clone_شجرة(true));
        }
        iterateChildren(nodep);
    }

    // Wire match vertex and mid-window sources for a successful NFA build.
    ساكن فراغ wireMatchAndMidSources(SvaGraph& graph, ثابت BuildResult& result, File_سطر* flp) {
        graph.createMatchVertex();
        // Skip the main term Link when midSources already cover every
        // end-of-match (cover_sequence path); otherwise the per-mid extraction
        // double-counts via the merge vertex.
        إذا (!result.termIsMidMerge) { graph.addLink(result.termVertexp, graph.m_matchVertexp); }
        لكل (SvaStateVertex* srcVtxp : result.midSources) {
            Ast_عقدة_تعبير* condp = nullptr;
            لكل (Ast_عقدة_تعبير* ثابت tc : srcVtxp->m_throughoutConds) {
                Ast_عقدة_تعبير* ثابت tcClone = tc->clone_شجرة_Pure(خطأ);
                condp = condp ? جديد AstLogAnd{flp, condp, tcClone} : tcClone;
            }
            graph.addLink(srcVtxp, graph.m_matchVertexp, condp);
            srcVtxp->m_isUnbounded = true;
        }
    }

    ساكن Ast_عقدة_تعبير* getSequenceBodyExprp(ثابت AstSequence* seqp) {
        Ast_عقدة* bodyp = seqp->stmtsp();
        طالما (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        أرجع VN_CAST(bodyp, عقدة_تعبير);
    }

    ساكن AstPropSpec* getPropertySpecp(ثابت AstProperty* propp) {
        Ast_عقدة* stmtp = propp->stmtsp();
        // V3LinkParse emits InitialStaticStmt for property-local variable
        // initialisers; the InitialAutomaticStmt variant only appears for
        // task/function-scope automatic lifetime, not properties.
        طالما (stmtp
               && (VN_IS(stmtp, Var) || VN_IS(stmtp, InitialStaticStmt)
                   || VN_IS(stmtp, InitialAutomaticStmt))) {  // LCOV_EXCL_LINE
            stmtp = stmtp->nextp();
        }
        أرجع VN_CAST(stmtp, PropSpec);
    }

    فراغ inlineNamedProperty(AstPropSpec* outerSpecp, AstFuncRef* funcrefp,
                             ثابت AstProperty* propyp) {
        // Recursion guard: IEEE 1800-2023 16.12.1 forbids recursive properties.
        // V3Width emits "Recursive property call" for direct recursion before this
        // pass runs; this catches any nested-inlining cycle that slips past.
        إذا (!m_inliningProps.emplace(propyp).second) {
            funcrefp->v3error("Illegal recursive property reference");  // LCOV_EXCL_LINE
            أرجع;  // LCOV_EXCL_LINE
        }
        بنية Guard final {
            std::set<ثابت AstProperty*>& setr;
            ثابت AstProperty* keyp;
            ~Guard() { setr.erase(keyp); }
        } guard{m_inliningProps, propyp};
        AstPropSpec* propSpecp = getPropertySpecp(propyp);
        UASSERT_OBJ(propSpecp, funcrefp, "Property has no body PropSpec");
        propSpecp = propSpecp->clone_شجرة(خطأ);

        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propyp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_عقدة_تعبير*> portMap;
        لكل (ثابت auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables to module-level temps (IEEE 16.10).
        std::unordered_map<ثابت AstVar*, AstVar*> localVarMap;
        لكل (Ast_عقدة* stmtp = propyp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (!varp->isIO()) {
                    ثابت string newName = m_propVarNames.get(varp);
                    AstVar* ثابت newVarp = جديد AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        propSpecp->foreach([&](AstVarRef* refp) {
            ثابت auto portIt = portMap.find(refp->varp());
            إذا (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_شجرة(خطأ));
                VL_DO_DANGLING(ادفع_Deletep(refp), refp);
                أرجع;
            }
            ثابت auto localIt = localVarMap.find(refp->varp());
            إذا (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        لكل (ثابت auto& tconnect : tconnects) {
            ادفع_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propSpecp, funcrefp);

        // Merge disable iff (IEEE 1800-2023 16.12.1)
        إذا (outerSpecp->disablep() && propSpecp->disablep()) {
            outerSpecp->v3error("disable iff expression before property call "
                                "and in its body is not legal");
            ادفع_Deletep(propSpecp->disablep()->unlinkFrBack());
        }
        إذا (outerSpecp->disablep()) {
            propSpecp->disablep(outerSpecp->disablep()->unlinkFrBack());
        }

        إذا (outerSpecp->sensesp() && propSpecp->sensesp()) {
            outerSpecp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Clock event before property call and in its body");
            ادفع_Deletep(propSpecp->sensesp()->unlinkFrBack());
        }
        إذا (outerSpecp->sensesp()) {
            AstSenItem* ثابت sensesp = outerSpecp->sensesp();
            sensesp->unlinkFrBack();
            propSpecp->sensesp(sensesp);
        }

        outerSpecp->replaceWith(propSpecp);
        VL_DO_DANGLING(ادفع_Deletep(outerSpecp), outerSpecp);
    }

    فراغ inlineSequenceRef(AstFuncRef* funcrefp, AstSequence* seqp) {
        Ast_عقدة_تعبير* ثابت bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        Ast_عقدة_تعبير* ثابت clonedp = bodyExprp->clone_شجرة(خطأ);

        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_عقدة_تعبير*> portMap;
        لكل (ثابت auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ثابت auto it = portMap.find(refp->varp());
            إذا (it != portMap.end()) {
                refp->replaceWith(it->second->clone_شجرة(خطأ));
                VL_DO_DANGLING(ادفع_Deletep(refp), refp);
            }
        });
        لكل (ثابت auto& tconnect : tconnects) {
            ادفع_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ادفع_Deletep(funcrefp), funcrefp);
        // Clear referenced flag so V3AssertPre cleanup does not emit
        // spurious UNSUPPORTED for sequences that were already inlined here.
        seqp->isReferenced(خطأ);
    }

    // Must run before hasMultiCycleExpr() so NFA sees sequence bodies.
    فراغ inlineAllSequenceRefs(Ast_عقدة* rootp) {
        منطقي changed = true;
        طالما (changed) {
            changed = خطأ;
            rootp->foreach([&](AstFuncRef* funcrefp) {
                إذا (changed) أرجع;
                إذا (AstSequence* ثابت seqp = VN_CAST(funcrefp->taskp(), Sequence)) {
                    inlineSequenceRef(funcrefp, seqp);
                    changed = true;
                }
            });
        }
    }

    ساكن منطقي has_Multi_Cycle_تعبير(ثابت Ast_عقدة* nodep) {
        أرجع nodep->exists([](ثابت Ast_عقدة* np) {
            إذا (ثابت auto* ثابت ep = VN_CAST(np, عقدة_تعبير)) أرجع ep->isMultiCycleSva();
            أرجع خطأ;
        });
    }

    ساكن VPropStrength effectiveAssertPropStrength(ثابت AstPropSpec* ثابت propSpecp) {
        إذا (propSpecp->propStrength() != VPropStrength::DEFAULT) أرجع propSpecp->propStrength();
        أرجع propSpecp->fileline()->language() <= V3LangCode::L1800_2005 ? VPropStrength::STRONG
                                                                           : VPropStrength::WEAK;
    }

    // Bare `assert property (p until q)` with boolean operands stays on
    // V3AssertPre's AstLoop lowering, which preserves per-attempt action-block
    // firings that this NFA's single-bit aggregated state cannot. Strong bare
    // forms are also lowered there. NFA still owns sequence operands and any
    // embedding inside a multi-cycle context (implication consequent, or/and
    // operands, etc.).
    ساكن منطقي is_Bare_قمة_Level_Until(Ast_عقدة* propp) {
        Ast_عقدة* p = propp;
        إذا (AstPropSpec* ثابت specp = VN_CAST(p, PropSpec)) p = specp->propp();
        طالما (AstLogNot* ثابت notp = VN_CAST(p, LogNot)) p = notp->lhsp();
        AstUntil* ثابت untilp = VN_CAST(p, Until);
        إذا (!untilp) أرجع خطأ;
        ثابت auto hasSeq = [](ثابت Ast_عقدة_تعبير* ep) {
            أرجع ep->exists([](ثابت Ast_عقدة_تعبير* np) { أرجع np->isMultiCycleSva(); });
        };
        إذا (hasSeq(untilp->lhsp()) || hasSeq(untilp->rhsp())) أرجع خطأ;
        أرجع true;
    }

    بنية PropertyParts final {
        Ast_عقدة_تعبير* triggerExprp = nullptr;
        Ast_عقدة_تعبير* seqExprp = nullptr;
        منطقي isOverlapped = true;
        منطقي hasImplication = خطأ;
        منطقي isFollowedBy = خطأ;  // True for #-# / #=# (non-vacuous-fail on antecedent miss)
    };

    ساكن PropertyParts decomposeProperty(Ast_عقدة* propp) {
        PropertyParts parts;
        إذا (AstPropSpec* ثابت specp = VN_CAST(propp, PropSpec)) { propp = specp->propp(); }
        إذا (AstImplication* ثابت implp = VN_CAST(propp, Implication)) {
            parts.hasImplication = true;
            parts.isOverlapped = implp->isOverlapped();
            parts.isFollowedBy = implp->isFollowedBy();
            parts.triggerExprp = implp->lhsp();
            parts.seqExprp = implp->rhsp();
        } وإلا إذا (Ast_عقدة_تعبير* ثابت exprp = VN_CAST(propp, عقدة_تعبير)) {
            parts.triggerExprp = nullptr;
            parts.seqExprp = exprp;
        }
        أرجع parts;
    }

    ساكن منطقي canSplitImplicationPassActions(ثابت PropertyParts& parts) {
        UASSERT(parts.hasImplication,
                "Implication pass action split requested without implication");
        UASSERT(parts.triggerExprp, "Implication pass action split requested without trigger");
        // Direct vacuous/nonvacuous classification uses the antecedent value in the current
        // assertion attempt. Leave delayed antecedents on the existing NFA pass path.
        أرجع !has_Multi_Cycle_تعبير(parts.triggerExprp);
    }

    ساكن فراغ splitImplicationPassActions(AstAssert* assertp, ثابت PropertyParts& parts,
                                            Ast_عقدة_تعبير* nonvacuousMatchp) {
        File_سطر* ثابت flp = assertp->fileline();

        Ast_عقدة* ثابت passsp = assertp->passsp()->unlinkFrBackWithNext();
        Ast_عقدة* splitsp = nullptr;

        إذا (!parts.isFollowedBy) {
            Ast_عقدة_تعبير* ثابت vacuousp
                = جديد AstLogNot{flp, sampled(parts.triggerExprp->clone_شجرة_Pure(خطأ))};
            Ast_عقدة* ثابت vacuousBodyp = passsp->clone_شجرة(خطأ);
            splitsp = newPassOnIf(flp, vacuousp, vacuousBodyp, assertp->userType(),
                                  assertp->directive(), /*vacuous=*/true);
        }

        AstIf* ثابت nonvacuousp
            = newPassOnIf(flp, nonvacuousMatchp, passsp, assertp->userType(), assertp->directive(),
                          /*vacuous=*/خطأ);
        splitsp = splitsp ? Ast_عقدة::addNext<Ast_عقدة, Ast_عقدة>(splitsp, nonvacuousp)
                          : static_cast<Ast_عقدة*>(nonvacuousp);
        assertp->addPasssp(splitsp);
    }

    // On a PropSpec-wrapped assertion whose NFA build failed with a semantic
    // error (errorEmitted), replace the body with a BitFalse const so later
    // passes see a well-formed AST. Returns true if replaced.
    فراغ replaceBodyOnBuildError(File_سطر* flp, AstPropSpec* propSpecp, منطقي errorEmitted) {
        إذا (!errorEmitted) أرجع;
        Ast_عقدة* ثابت innerPropp = propSpecp->propp();
        innerPropp->replaceWith(جديد AstConst{flp, AstConst::BitFalse{}});
        VL_DO_DANGLING(ادفع_Deletep(innerPropp), innerPropp);
    }

    // Hoist a leading clocking event (IEEE 1800-2023 16.7):
    منطقي hoistClockedSeq(AstPropSpec* specp) {
        طالما (AstSClocked* ثابت clockedp = VN_CAST(specp->propp(), SClocked)) {
            إذا (specp->sensesp()) {
                clockedp->v3warn(E_UNSUPPORTED, "Unsupported: multiclocked sequence or property");
                replaceBodyOnBuildError(specp->fileline(), specp, true);
                أرجع true;
            }
            لكل (ثابت AstSenItem* sp = clockedp->sensesp(); sp;
                 sp = VN_CAST(sp->nextp(), SenItem)) {
                إذا (!sp->edgeType().anEdge()) {
                    clockedp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: non-edge clocking event on a sequence; "
                                     "use an edge such as @(posedge clk)");
                    replaceBodyOnBuildError(specp->fileline(), specp, true);
                    أرجع true;
                }
            }
            specp->sensesp(clockedp->sensesp()->unlinkFrBackWithNext());
            Ast_عقدة_تعبير* ثابت bodyp = clockedp->exprp()->unlinkFrBack();
            clockedp->replaceWith(bodyp);
            VL_DO_DANGLING(ادفع_Deletep(clockedp), clockedp);
        }
        // A clocking event anywhere else in the sequence is not supported.
        ثابت AstSClocked* nestedp = nullptr;
        specp->propp()->foreach([&](ثابت AstSClocked* p) {
            إذا (!nestedp) nestedp = p;
        });
        إذا (nestedp) {
            nestedp->v3warn(E_UNSUPPORTED,
                            "Unsupported: clocking event inside sequence expression");
            replaceBodyOnBuildError(specp->fileline(), specp, true);
            أرجع true;
        }
        أرجع خطأ;
    }

    // Build the NFA graph for a property body, handling both the antecedent
    // |-> consequent and simple sequence cases. Returns the consequent/body
    // BuildResult (invalid on parse/build failure).
    BuildResult buildAssertionGraph(SvaNfaBuilder& builder, SvaGraph& graph, Ast_عقدة_تعبير* seqBodyp,
                                    ثابت PropertyParts& parts, File_سطر* flp) {
        إذا (!parts.hasImplication) أرجع builder.build(seqBodyp);

        graph.m_startVertexp = graph.createStateVertex();
        أرجع builder.buildImplicationEdges(parts.triggerExprp, seqBodyp, graph.m_startVertexp,
                                             parts.isOverlapped, parts.isFollowedBy,
                                             parts.triggerExprp, flp);
    }

    // Install pass-action gating and replay simultaneous per-thread failures.
    فراغ attachActionHandlers(AstAssert* ثابت assertp, Ast_عقدة_تعبير* matchExprp,
                              Ast_Sen_شجرة* ثابت threadFailReplaySenTreep,
                              Ast_عقدة_تعبير* ثابت threadFailCountp) {
        // Gate pass handler on match to prevent vacuous-pass firings.
        إذا (matchExprp) {
            // needMatch implies passsp() was non-null when evaluated above;
            // lowering does not mutate the assert's pass-action between the
            // two reads, so passsp() is still non-null here.
            Ast_عقدة* passsp = assertp->passsp();
            UASSERT_OBJ(passsp, assertp, "needMatch set but passsp is null");
            passsp->unlinkFrBackWithNext();
            File_سطر* ثابت flp = assertp->fileline();
            AstIf* ثابت ifp = جديد AstIf{flp, matchExprp, passsp, nullptr};
            assertp->addPasssp(ifp);
            // Fail-handler prefix for overlapping instances (IEEE 16.12):
            // fires when reject=1 && match=1 in the same cycle.
            إذا (assertp->failsp()) assertp->prependFailsp(ifp->clone_شجرة(خطأ));
        }

        إذا (threadFailCountp) {
            UASSERT_OBJ(threadFailReplaySenTreep, assertp,
                        "Thread fail count missing sensitivity tree");
            Ast_عقدة* ثابت failsp = assertp->failsp();
            File_سطر* ثابت flp = assertp->fileline();
            // IEEE 1800-2023 16.12 requires one action-block evaluation per failed
            // thread. AstAssert handles the first, so replay the rest here.
            AstVar* ثابت remaining_Fail_عدد_Varp
                = جديد AstVar{flp, VVarType::MODULETEMP, m_fail_عدد_Names.get(""),
                             m_modp->findBasicDType(VBasicDTypeKwd::UINT32)};
            remaining_Fail_عدد_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remaining_Fail_عدد_Varp);
            Ast_عقدة* ثابت replayStmtsp = جديد AstAssign{
                flp, جديد AstVarRef{flp, remaining_Fail_عدد_Varp, VAccess::WRITE}, threadFailCountp};
            AstLoop* ثابت replayLoopp = جديد AstLoop{flp};
            replayLoopp->addStmtsp(جديد AstLoopTest{
                flp, replayLoopp,
                جديد AstGt{flp, جديد AstVarRef{flp, remaining_Fail_عدد_Varp, VAccess::READ},
                          newTypedConstp(flp, remaining_Fail_عدد_Varp->dtypep(), 1)}});
            replayLoopp->addStmtsp(newIfAssertFailOn(failsp->clone_شجرة(true), assertp->directive(),
                                                     assertp->userType()));
            AstSub* ثابت decrementedFailCountp
                = جديد AstSub{flp, جديد AstVarRef{flp, remaining_Fail_عدد_Varp, VAccess::READ},
                             newTypedConstp(flp, remaining_Fail_عدد_Varp->dtypep(), 1)};
            replayLoopp->addStmtsp(
                جديد AstAssign{flp, جديد AstVarRef{flp, remaining_Fail_عدد_Varp, VAccess::WRITE},
                              decrementedFailCountp});
            replayStmtsp->addNext(replayLoopp);
            m_modp->addStmtsp(
                جديد AstAlways{flp, VAlwaysKwd::ALWAYS, threadFailReplaySenTreep, replayStmtsp});
        }
    }

    // Replace one VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). No-op if refp is not in matchMap.
    فراغ substituteMatchItemRef(AstVarRef* refp, unsigned K,
                                ثابت std::unordered_map<ثابت AstVar*, Ast_عقدة_تعبير*>& matchMap) {
        ثابت auto it = matchMap.find(refp->varp());
        إذا (it == matchMap.end()) أرجع;
        Ast_عقدة_تعبير* newp = it->second->clone_شجرة_Pure(خطأ);
        إذا (K > 0) {
            AstConst* ثابت ticksp = جديد AstConst{refp->fileline(), AstConst::Widthed_قيمة{}, 32,
                                                  static_cast<uint32_t>(K)};
            AstPast* ثابت pastp = جديد AstPast{refp->fileline(), newp, ticksp, nullptr,
                                               /* propertyTiming */ true};
            pastp->dtypeFrom(newp);
            newp = pastp;
        }
        refp->replaceWith(newp);
        VL_DO_DANGLING(ادفع_Deletep(refp), refp);
        أرجع;
    }

    // Recursively walk a consequent. Returns cycle length consumed and
    // substitutes each VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). Reports E_UNSUPPORTED on non-constant
    // delays or composite sequence operators and returns -1 on failure.
    صحيح
    walk_Substitute_Match_عناصر(Ast_عقدة_تعبير* nodep, unsigned K,
                             ثابت std::unordered_map<ثابت AstVar*, Ast_عقدة_تعبير*>& match_عناصر) {
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            // IEEE 1800-2023 16.9.2: cycle_delay's lhsp is a constant_expression
            // and the delay form in a sequence is always `##N`, folded by
            // V3Const + V3Param before V3AssertNfa. Range form `##[m:n]` is the
            // only user-visible reject here.
            AstDelay* ثابت delayp = VN_AS(sexprp->delayp(), Delay);
            UASSERT_OBJ(delayp->isCycleDelay() && VN_IS(delayp->lhsp(), Const), sexprp,
                        "SVA cycle delay must have a constant lhsp");
            إذا (delayp->isRangeDelay()) {
                sexprp->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                              "non-constant cycle delay in consequent"
                                              " (IEEE 1800-2023 16.10)");
                أرجع -1;
            }
            ثابت unsigned delayCycles = VN_AS(delayp->lhsp(), Const)->toUInt();
            صحيح preLen = 0;
            إذا (Ast_عقدة_تعبير* ثابت prep = sexprp->preExprp()) {
                preLen = walk_Substitute_Match_عناصر(prep, K, match_عناصر);
                إذا (preLen < 0) أرجع -1;
            }
            ثابت صحيح bodyLen
                = walk_Substitute_Match_عناصر(sexprp->exprp(), K + preLen + delayCycles, match_عناصر);
            إذا (bodyLen < 0) أرجع -1;
            أرجع preLen + delayCycles + bodyLen;
        }
        إذا (nodep->isMultiCycleSva()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                         "composite sequence operator in consequent"
                                         " (IEEE 1800-2023 16.10)");
            أرجع -1;
        }
        std::vector<AstVarRef*> refs;
        nodep->foreach([&refs](AstVarRef* p) { refs.ادفع_back(p); });
        لكل (AstVarRef* ثابت refp : refs) substituteMatchItemRef(refp, K, match_عناصر);
        أرجع 0;
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
    منطقي liftMatchItemSubstitutions(PropertyParts& parts, Ast_عقدة_تعبير* seqBodyp) {
        إذا (!parts.hasImplication) أرجع خطأ;
        Ast_تعبير_Stmt* ثابت تعبير_Stmtp = VN_CAST(parts.triggerExprp, تعبير_Stmt);
        إذا (!تعبير_Stmtp) أرجع خطأ;
        // IEEE 1800-2023 16.10 BNF requires `(expr, match_item {, match_item})`
        // with at least one match item; V3LinkParse only emits ExprStmt for
        // this form and only emits AstAssign with VarRef LHS for each item.
        std::unordered_map<ثابت AstVar*, Ast_عقدة_تعبير*> match_عناصر;
        لكل (Ast_عقدة* stmtp = تعبير_Stmtp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstAssign* ثابت assignp = VN_AS(stmtp, Assign);
            AstVarRef* ثابت lhsRefp = VN_AS(assignp->lhsp(), VarRef);
            match_عناصر[lhsRefp->varp()] = assignp->rhsp();
        }
        ثابت unsigned startK = parts.isOverlapped ? 0 : 1;
        ثابت صحيح length = walk_Substitute_Match_عناصر(seqBodyp, startK, match_عناصر);
        // Match-item substitution / strip mutates ancestor purity. Release
        // builds don't auto-clear caches on edits, so refresh here.
        VIsCached::clear_Cache_شجرة();
        إذا (length < 0) أرجع true;
        Ast_عقدة_تعبير* ثابت antBoolp = تعبير_Stmtp->resultp()->unlinkFrBack();
        تعبير_Stmtp->replaceWith(antBoolp);
        VL_DO_DANGLING(ادفع_Deletep(تعبير_Stmtp), تعبير_Stmtp);
        parts.triggerExprp = antBoolp;
        أرجع خطأ;
    }

    // Outcome counts for a property if/case are wrong in an outcome-multiplying
    // context. Returns that context, or nullptr when the shape is supported.
    ساكن ثابت حرف* unsupportedPropertyControl(ثابت Ast_عقدة_Cover_Or_Assert* assertp,
                                                  ثابت Ast_عقدة_تعبير* seqBodyp, منطقي negated) {
        إذا (!hasPropertyControlConjunction(seqBodyp)) أرجع nullptr;
        إذا (negated) أرجع "negation";
        إذا (VN_IS(assertp, Cover)) أرجع "cover";
        إذا (VN_AS(assertp, Assert)->passsp()) أرجع "a pass action";
        أرجع nullptr;
    }

    // Inline property/sequence refs and reject unsupported shapes.
    // Returns the PropSpec to lower, or nullptr when fully handled here.
    AstPropSpec* prepareAssertionProp(Ast_عقدة_Cover_Or_Assert* assertp) {
        إذا (AstPropSpec* ثابت specp = VN_CAST(assertp->propp(), PropSpec)) {
            إذا (AstFuncRef* ثابت funcrefp = VN_CAST(specp->propp(), FuncRef)) {
                إذا (ثابت AstProperty* ثابت propyp = VN_CAST(funcrefp->taskp(), Property)) {
                    inlineNamedProperty(specp, funcrefp, propyp);
                }
            }
        }

        inlineAllSequenceRefs(assertp->propp());

        إذا (AstPropSpec* ثابت specp = VN_CAST(assertp->propp(), PropSpec)) {
            إذا (hoistClockedSeq(specp)) أرجع nullptr;
        }

        AstPropSpec* ثابت propp = VN_AS(assertp->propp(), PropSpec);
        إذا (!VN_IS(assertp, Cover)
            && effectiveAssertPropStrength(propp) == VPropStrength::STRONG) {
            propp->v3warn(E_UNSUPPORTED,
                          "Unsupported: strong property in " + assertp->verilogKwd() + ".");
            replaceBodyOnBuildError(assertp->fileline(), propp, /*errorEmitted=*/true);
            أرجع nullptr;
        }

        إذا (!has_Multi_Cycle_تعبير(propp)) أرجع nullptr;
        // A nested property instance keeps its body behind the call; lowering would drop it.
        إذا (propp->exists([](ثابت AstFuncRef* refp) { أرجع VN_IS(refp->taskp(), Property); })) {
            assertp->v3warn(E_UNSUPPORTED,
                            "Unsupported: property instance inside a multi-cycle property "
                            "expression");
            VL_DO_DANGLING(ادفع_Deletep(assertp->unlinkFrBack()), assertp);
            أرجع nullptr;
        }
        إذا (is_Bare_قمة_Level_Until(propp)) أرجع nullptr;
        أرجع propp;
    }

    فراغ processAssertion(Ast_عقدة_Cover_Or_Assert* assertp) {
        إذا (assertp->immediate()) أرجع;

        AstPropSpec* ثابت propp = prepareAssertionProp(assertp);
        إذا (!propp) أرجع;

        PropertyParts parts = decomposeProperty(propp);
        UASSERT_OBJ(parts.seqExprp, propp, "Property body must be an expression");

        // Unwrap `not` (IEEE 1800-2023 16.12.1); odd count -> negated semantics.
        Ast_عقدة_تعبير* seqBodyp = parts.seqExprp;
        منطقي negated = خطأ;
        طالما (AstLogNot* ثابت notp = VN_CAST(seqBodyp, LogNot)) {
            negated = !negated;
            seqBodyp = notp->lhsp();
        }

        ثابت حرف* ثابت propertyControlp
            = unsupportedPropertyControl(assertp, seqBodyp, negated);

        // Substitute property-local match-item refs in consequent with
        // $past(rhs, K) before NFA build (IEEE 1800-2023 16.10).
        إذا (liftMatchItemSubstitutions(parts, seqBodyp)) {
            AstPropSpec* ثابت psp = VN_CAST(assertp->propp(), PropSpec);
            UASSERT_OBJ(psp, assertp, "Concurrent assertion must have PropSpec");
            replaceBodyOnBuildError(assertp->fileline(), psp, /*errorEmitted=*/true);
            أرجع;
        }

        AstCover* ثابت coverp = VN_CAST(assertp, Cover);
        ثابت منطقي isCoverSeq = coverp && coverp->isCoverSeq();
        // A sequence event control is not an assertion directive; no default
        // disable iff, no assertion control
        ثابت منطقي isSeqEvent = coverp && coverp->isSeqEvent();
        ثابت منطقي عدد_Matches = isCoverSeq && !isSeqEvent;
        // Inherit module defaults (IEEE 14.12, 16.15) when assertion has none.
        إذا (!propp->sensesp() && m_defaultClockingp) {
            propp->sensesp(m_defaultClockingp->sentreep()->sensesp()->clone_شجرة(true));
        }
        إذا (!propp->disablep() && m_defaultDisablep && !isSeqEvent) {
            propp->disablep(m_defaultDisablep->condp()->clone_شجرة_Pure(true));
        }
        إذا (!propp->sensesp()) أرجع;
        Ast_Sen_شجرة* senTreep
            = جديد Ast_Sen_شجرة{propp->fileline(), propp->sensesp()->clone_شجرة(true)};
        Ast_عقدة_تعبير* ثابت disableExprp = propp->disablep();

        // NFA lowering clones repeated operands and may hoist them into an
        // always_comb block. Resolve implicit sampled-value clocks first, while
        // the enclosing assertion clock is still available.
        {
            VL_RESTORER(m_sampled_قيمة_Clockp);
            m_sampled_قيمة_Clockp = senTreep;
            iterate(propp->propp());
        }

        File_سطر* ثابت flp = assertp->fileline();

        SvaGraph graph;
        SvaNfaBuilder builder{graph,      m_modp,     m_propTempNames,
                              isCoverSeq, isSeqEvent, coverp != nullptr};

        ثابت BuildResult result = buildAssertionGraph(builder, graph, seqBodyp, parts, flp);
        إذا (result.valid()) wireMatchAndMidSources(graph, result, flp);
        إذا (!result.valid()) {
            // Fall through to V3AssertPre for unsupported constructs; only
            // replace the body on real semantic errors. Any hoisted temps
            // from this attempt become orphan MODULETEMPs; V3Dead removes
            // them along with the dead always_comb driver.
            replaceBodyOnBuildError(flp, propp, result.errorEmitted);
            VL_DO_DANGLING(ادفع_Deletep(senTreep), senTreep);
            أرجع;
        }
        // After the build, so a construct the builder rejects reports itself.
        إذا (propertyControlp) {
            seqBodyp->v3warn(E_UNSUPPORTED,
                             "Unsupported: temporal property if/case with " << propertyControlp);
            replaceBodyOnBuildError(flp, propp, /*errorEmitted=*/true);
            VL_DO_DANGLING(ادفع_Deletep(senTreep), senTreep);
            أرجع;
        }
        إذا (عدد_Matches && !v3Global.opt.coverageUser()) {
            SvaNfaBuilder::freeUnlinkedCondp(result.finalCondp);
            VL_DO_DANGLING(ادفع_Deletep(senTreep), senTreep);
            Ast_عقدة* ثابت innerPropp = propp->propp();
            innerPropp->replaceWith(جديد AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(ادفع_Deletep(innerPropp), innerPropp);
            أرجع;
        }

        AstAssert* ثابت assertAssertp = VN_CAST(assertp, Assert);
        ثابت منطقي splitImplicationPasssp = assertAssertp && assertAssertp->passsp()
                                            && parts.hasImplication
                                            && canSplitImplicationPassActions(parts);
        ثابت منطقي needMatch = assertAssertp && assertAssertp->passsp()
                               && (!parts.hasImplication || splitImplicationPasssp);

        ثابت منطقي needsThreadFailReplay
            = assertAssertp && assertAssertp->failsp() && !parts.hasImplication;
        ثابت auto signals
            = m_loweringp->lower(assertp, graph, senTreep, result.finalCondp, disableExprp,
                                 needsThreadFailReplay, needsThreadFailReplay && !negated);
        Ast_عقدة_تعبير* matchExprp = nullptr;
        Ast_عقدة_تعبير* outputExprp = m_loweringp->assembleResult(
            assertp, negated, result.finalCondp, signals, needMatch ? &matchExprp : nullptr);
        إذا (عدد_Matches) {
            UASSERT_OBJ(signals.matchCountp, coverp, "Cover sequence missing match count");
            VL_DO_DANGLING(outputExprp->delete_شجرة(), outputExprp);
            propp->matchCountp(signals.matchCountp);
            outputExprp = جديد AstNeq{flp, signals.matchCountp->clone_شجرة_Pure(خطأ),
                                     newTypedConstp(flp, signals.matchCountp->dtypep(), 0)};
        }

        Ast_Sen_شجرة* ثابت threadFailReplaySenTreep
            = signals.threadFailCountp ? senTreep->clone_شجرة(خطأ) : nullptr;

        VL_DO_DANGLING(ادفع_Deletep(senTreep), senTreep);
        إذا (result.finalCondp && !result.finalCondp->backp()) ادفع_Deletep(result.finalCondp);

        إذا (splitImplicationPasssp) {
            splitImplicationPassActions(assertAssertp, parts, matchExprp);
        } وإلا {
            attachActionHandlers(assertAssertp, matchExprp, threadFailReplaySenTreep,
                                 signals.threadFailCountp);
        }

        Ast_عقدة* ثابت innerPropp = propp->propp();
        innerPropp->replaceWith(outputExprp);
        VL_DO_DANGLING(ادفع_Deletep(innerPropp), innerPropp);

        UINFO(4, "NFA converted assertion at " << flp << endl);

        إذا (dumpGraphLevel() >= 6) graph.m_graph.dumpDotFilePrefixed("assert-nfa");
    }

    // VISITORS
    فراغ visit(Ast_عقدة_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_loweringp);
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultDisablep);
        m_modp = nodep;
        // A class nested in a module is in the scope of the module's default clocking
        إذا (!VN_IS(nodep, Class)) m_defaultClockingp = nullptr;
        m_defaultDisablep = nodep->defaultDisablep();
        SvaNfaLowering lowering{nodep};
        m_loweringp = &lowering;
        iterateChildren(nodep);
    }
    فراغ visit(AstClocking* nodep) override {
        إذا (nodep->isDefault() && !m_defaultClockingp) m_defaultClockingp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    فراغ visit(AstDefaultDisable* nodep) override {}
    فراغ visit(AstFell* nodep) override { visit_Sampled_قيمة(nodep); }
    فراغ visit(AstPast* nodep) override { visit_Sampled_قيمة(nodep); }
    فراغ visit(AstRose* nodep) override { visit_Sampled_قيمة(nodep); }
    فراغ visit(AstStable* nodep) override { visit_Sampled_قيمة(nodep); }
    فراغ visit(AstAssert* nodep) override { processAssertion(nodep); }
    فراغ visit(AstCover* nodep) override { processAssertion(nodep); }
    فراغ visit(AstRestrict* nodep) override {
        // Restrict property is ignored by simulators (IEEE 1800-2023 16.12.2).
        // Remove here so temporal SExpr don't leak to V3AssertPre.
        VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
    }
    فراغ visit(AstAssertIntrinsic* nodep) override {}
    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
    explicit AssertNfaVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Top entry point

فراغ V3AssertNfa::assertNfaAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":" << endl);
    { AssertNfaVisitor{nodep}; }
    V3Global::dump_Check_Global_شجرة("assertnfa", 0, dump_شجرة_Either_Level() >= 3);
}
