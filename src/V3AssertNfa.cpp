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

صنف Sva_State_راس;

// Per-vertex algorithm data, stored via V3GraphVertex::userp() during lowering
بنية Sva_راس_معلومات final {
    AstVar* stateVarp = تش_اشاره;  // NBA state register for this vertex
    AstVar* delayRingVarp = تش_اشاره;  // Packed occupancy bits or per-slot match counts
    AstVar* delay_Ring_سر_Varp
        = تش_اشاره;  // Cached outgoing slot; isolates readers from in-place writes
    AstVar* delayRingIdxVarp = تش_اشاره;  // Next slot written in delayRingVarp
    AstVar* delay_Ring_ژوندی_شمېر_Varp = تش_اشاره;  // Number of threads in delayRingVarp
    AstVar* delayRingWrappedVarp = تش_اشاره;  // All slots written since the last clear
    AstVar* بشپړ_LVarp = تش_اشاره;  // SAnd LHS done-latch
    AstVar* بشپړ_RVarp = تش_اشاره;  // SAnd RHS done-latch
    Ast_غوټه_بیان* stateSigp = تش_اشاره;  // Combinational state signal; OWNED during lowering
    Ast_غوټه_بیان* state_شمېر_Sigp = تش_اشاره;  // Cover-sequence match count; OWNED during lowering
    Ast_غوټه_بیان* state_شمېر_ډکېدنه_Sigp = تش_اشاره;  // Count overflow; OWNED during lowering
    منطقي delayRingCounts = خطأ;  // Ring slots hold cover-sequence match counts
    منطقي needsReg = خطأ;  // True if vertex has incoming clocked edge
};

// NFA state vertex -- one per NFA position in the sequence evaluation
صنف Sva_State_راس final : عام V3_ګراف_راس {
    VL_RTTI_IMPL(Sva_State_راس, V3_ګراف_راس)
عام:
    // True if this is the sequence-match terminal vertex
    منطقي m_is_سمون = خطأ;
    // OWNED throughout-guard condition clones; IEEE 1800-2023 16.9.9
    std::ویکټور<Ast_غوټه_بیان*> m_throughoutConds;
    // Nonzero for a ring-buffer vertex for ## delays.
    منطقي m_isFixedDelayRing = خطأ;
    بینښه m_delay_Ring_اندازه = 0;  // Number of ring slots. Range: max-min+1.
    Ast_غوټه_بیان* m_delayRingClearCondp = تش_اشاره;  // local RHS for pure-boolean range
    Ast_غوټه_بیان* m_delayRingAdvanceCondp = تش_اشاره;  // Advance only when this condition holds
    منطقي m_replayAbortReject = خطأ;  // Compressed repetition needs per-thread abort replay
    // OWNED; enclosing-abort fire condition clearing state or suppressing guard rejection
    Ast_غوټه_بیان* m_abortClearp = تش_اشاره;
    // OWNED; reject-abort fire condition rejecting all represented live threads
    Ast_غوټه_بیان* m_abortRejectp = تش_اشاره;
    // Liveness terminal (IEEE weak semantics): reject must not fire from this source
    منطقي m_isUnbounded = خطأ;
    // Temporal sequence AND combiner; IEEE 1800-2023 16.9.5
    منطقي m_isAndCombiner = خطأ;
    Sva_State_راس* m_andLhsTermp = تش_اشاره;  // LHS sub-NFA terminal vertex
    Sva_State_راس* m_andRhsTermp = تش_اشاره;  // RHS sub-NFA terminal vertex
    Ast_غوټه_بیان* m_andLhsCondp = تش_اشاره;  // OWNED; LHS final condition (may be nullptr)
    Ast_غوټه_بیان* m_andRhsCondp = تش_اشاره;  // OWNED; RHS final condition (may be nullptr)
    // Reject sink for SAnd rejectOnFail wiring; not a state-signal source
    منطقي m_isRejectSink = خطأ;
    // In-window vertex of a strong s_always[m:n]: if its state is still set at
    // end-of-simulation the universal-quantifier window never completed, which is
    // a liveness failure (IEEE 1800-2023 16.12.11 strong semantics).
    منطقي m_strongPending = خطأ;
    // Ring introduced for the checked portion of a bounded strong s_always window.
    منطقي m_strongAlwaysRing = خطأ;

    // CONSTRUCTORS
    څرګند Sva_State_راس(V3_ګراف* graphp)
        : V3_ګراف_راس{graphp} {}
    ~Sva_State_راس() override {
        لكل (Ast_غوټه_بیان* cp : m_throughoutConds) VL_DO_DANGLING(cp->delete_ونه(), cp);
        إذا (m_delayRingClearCondp)
            VL_DO_DANGLING(m_delayRingClearCondp->delete_ونه(), m_delayRingClearCondp);
        إذا (m_delayRingAdvanceCondp)
            VL_DO_DANGLING(m_delayRingAdvanceCondp->delete_ونه(), m_delayRingAdvanceCondp);
        إذا (m_abortClearp) VL_DO_DANGLING(m_abortClearp->delete_ونه(), m_abortClearp);
        إذا (m_abortRejectp) VL_DO_DANGLING(m_abortRejectp->delete_ونه(), m_abortRejectp);
        إذا (m_andLhsCondp) VL_DO_DANGLING(m_andLhsCondp->delete_ونه(), m_andLhsCondp);
        إذا (m_andRhsCondp) VL_DO_DANGLING(m_andRhsCondp->delete_ونه(), m_andRhsCondp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string name() ثابت override {
        string name = "s" + cvt_منزل_Str(color());
        إذا (m_delay_Ring_اندازه) {
            name += "\\n";
            name += m_isFixedDelayRing ? "fixed chain " : "range chain ";
            name += cvt_منزل_Str(m_delay_Ring_اندازه) + " slots";
        }
        أرجع name;
    }
    string dotColor() ثابت override {
        إذا (m_is_سمون) أرجع "red";
        إذا (m_delay_Ring_اندازه) أرجع "blue";
        إذا (m_isAndCombiner) أرجع "purple";
        أرجع "black";
    }
    // LCOV_EXCL_STOP
    // Access per-vertex algorithm data (valid only during lowering phase)
    Sva_راس_معلومات* datap() ثابت { أرجع static_cast<Sva_راس_معلومات*>(userp()); }
};

// NFA transition edge -- clocked (##1) or combinational link (##0)
صنف Sva_Trans_څنډه final : عام V3_ګراف_څنډه {
    VL_RTTI_IMPL(Sva_Trans_څنډه, V3_ګراف_څنډه)
عام:
    Ast_غوټه_بیان* m_condp;  // Transition condition; nullptr = unconditional; OWNED
    منطقي m_consumes_دوره;  // true = clocked edge (##1), false = link (##0/boolean)
    // Reject when source is active and condp is false; set only on
    // outermost required-step Link
    منطقي m_rejectOnFail = خطأ;
    // Optional dynamic condition vertex for m_rejectOnFail. Used when the
    // success condition is another NFA state rather than a static expression.
    Sva_State_راس* m_condVtxp = تش_اشاره;

    // CONSTRUCTORS
    Sva_Trans_څنډه(V3_ګراف* graphp, V3_ګراف_راس* fromp, V3_ګراف_راس* سر, Ast_غوټه_بیان* condp,
                 منطقي consumes_دوره)
        : V3_ګراف_څنډه{graphp, fromp, سر, /*weight=*/1}
        , m_condp{condp}
        , m_consumes_دوره{consumes_دوره} {}
    ~Sva_Trans_څنډه() override {
        إذا (m_condp) VL_DO_DANGLING(m_condp->delete_ونه(), m_condp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string dotLabel() ثابت override { أرجع m_consumes_دوره ? "##1" : "link"; }
    string dotStyle() ثابت override { أرجع m_consumes_دوره ? "" : "dashed"; }
    // LCOV_EXCL_STOP
    // Typed accessors for NFA vertices
    Sva_State_راس* fromVtxp() ثابت { أرجع static_cast<Sva_State_راس*>(fromp()); }
    Sva_State_راس* منزل_Vtxp() ثابت { أرجع static_cast<Sva_State_راس*>(سر()); }
};

// NFA graph container
صنف Sva_ګراف final {
عام:
    V3_ګراف m_ګراف;  // Owns all vertices and edges
    Sva_State_راس* m_پیل_Vertexp = تش_اشاره;  // Trigger/start vertex
    Sva_State_راس* m_سمون_Vertexp = تش_اشاره;  // Sequence-match terminal vertex

    // Create a new state vertex
    Sva_State_راس* create_State_راس() { أرجع جديد Sva_State_راس{&m_ګراف}; }
    // Create the match terminal vertex
    Sva_State_راس* create_سمون_راس() {
        Sva_State_راس* ثابت vtxp = create_State_راس();
        vtxp->m_is_سمون = ریښتیا;
        m_سمون_Vertexp = vtxp;
        أرجع vtxp;
    }
    // Add a clocked transition edge (##1)
    Sva_Trans_څنډه* جمع_کړه_Clocked_څنډه(Sva_State_راس* fromp, Sva_State_راس* سر,
                                 Ast_غوټه_بیان* condp = تش_اشاره) {
        أرجع جديد Sva_Trans_څنډه{&m_ګراف, fromp, سر, condp, /*consumesCycle=*/ریښتیا};
    }
    // Add a combinational link (##0 / boolean condition)
    Sva_Trans_څنډه* جمع_کړه_Link(Sva_State_راس* fromp, Sva_State_راس* سر,
                          Ast_غوټه_بیان* condp = تش_اشاره) {
        أرجع جديد Sva_Trans_څنډه{&m_ګراف, fromp, سر, condp, /*consumesCycle=*/خطأ};
    }
    // Collect all edges into a flat vector for iteration.
    // Used by the lowering phase which needs global edge scans.
    std::ویکټور<ثابت Sva_Trans_څنډه*> all_څنډې() ثابت {
        std::ویکټور<ثابت Sva_Trans_څنډه*> پایله;
        لكل (ثابت V3_ګراف_راس& vtxr : m_ګراف.vertices()) {
            لكل (ثابت V3_ګراف_څنډه& edger : vtxr.بهر_څنډې()) {
                پایله.ټېل_شا(static_cast<ثابت Sva_Trans_څنډه*>(&edger));
            }
        }
        أرجع پایله;
    }
};

//######################################################################
// Builder result: terminal vertex + optional final condition (match Link condition).
بنية Build_پایله final {
    Sva_State_راس* غړی_Vertexp;  // Primary terminal; contributes to both match and reject
    Ast_غوټه_بیان* finalCondp;  // nullptr = unconditional
    // Mid-window sources for range delays (pure boolean RHS): match-only (isUnbounded)
    std::ویکټور<Sva_State_راس*> midSources;
    منطقي errorEmitted = خطأ;  // Builder already emitted specific error; skip generic
    // For cover_sequence: when true, midSources already enumerate every
    // end-of-match, so wireMatchAndMidSources must NOT add the main
    // termVtxp -> matchVertex Link (would double-count via the merge vertex).
    منطقي غړی_Is_Mid_یوځای = خطأ;
    منطقي valid() ثابت { أرجع غړی_Vertexp != تش_اشاره; }
    ساكن Build_پایله fail(منطقي errored = خطأ) { أرجع {تش_اشاره, تش_اشاره, {}, errored}; }
    ساكن Build_پایله failWithError() { أرجع {تش_اشاره, تش_اشاره, {}, ریښتیا}; }
};

// Parser-marked SAnd of overlapped implications: a property if/else/case.
ساكن منطقي hasPropertyControlConjunction(ثابت Ast_غوټه_بیان* nodep) {
    أرجع nodep->exists([](ثابت AstSAnd* andp) { أرجع andp->propertyControl(); });
}

ساكن AstConst* newTypedConstp(File_کرښه* ثابت flp, ثابت Ast_غوټه_DType* ثابت dtypep,
                                ثابت uint32_t ارزښت) {
    AstConst* ثابت constp = جديد AstConst{flp, AstConst::DTyped{}, dtypep};
    constp->عدد().سېټ_Long(ارزښت);
    أرجع constp;
}

ساكن Ast_غوټه_بیان* sampled(Ast_غوټه_بیان* exprp) {
    أرجع جديد AstSampled{exprp->fileline(), exprp, exprp->dtypep(), ریښتیا};
}

ساكن ثابت حرف* assertPassOnQuery(منطقي vacuous) {
    ساكن constexpr ثابت حرف* queries[2]
        = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
    أرجع queries[vacuous];
}

ساكن Ast_غوټه_بیان* assertOnCond(File_کرښه* flp, VAssertType type,
                                 VAssertDirectiveType directiveType) {
    إذا (!v3Global.opt.assertOn()) { أرجع جديد AstConst{flp, AstConst::بټ_False{}}; }
    أرجع جديد AstCExpr{flp, AstCExpr::خالص{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
}

ساكن Ast_غوټه_بیان* assertKillGet(File_کرښه* flp, VAssertType type,
                                  VAssertDirectiveType directiveType) {
    أرجع جديد AstCExpr{flp, AstCExpr::خالص{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_KILL", type, directiveType),
                        32};
}

ساكن string assert_Action_Control_مختاړی(VAssertDirectiveType directiveType) {
    ثابت صحيح controlled = !!(static_cast<صحيح>(directiveType)
                              & (static_cast<صحيح>(VAssertDirectiveType::ASSERT)
                                 | static_cast<صحيح>(VAssertDirectiveType::COVER)
                                 | static_cast<صحيح>(VAssertDirectiveType::ASSUME)));
    ثابت صحيح checkRuntime = controlled & static_cast<صحيح>(v3Global.opt.assertOn());
    أرجع "("s + std::منزل_string(controlled ^ 1) + " || ("s + std::منزل_string(checkRuntime)
           + " && "s;
}

ساكن Ast_غوټه_بیان* assertPassOnCond(File_کرښه* flp, VAssertType type,
                                     VAssertDirectiveType directiveType, منطقي vacuous) {
    أرجع جديد AstCExpr{
        flp, AstCExpr::خالص{},
        assert_Action_Control_مختاړی(directiveType)
            + V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType)
            + "))"s,
        1};
}

ساكن Ast_غوټه_بیان* assertFailOnCond(File_کرښه* flp, VAssertType type,
                                     VAssertDirectiveType directiveType) {
    أرجع جديد AstCExpr{
        flp, AstCExpr::خالص{},
        assert_Action_Control_مختاړی(directiveType)
            + V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType) + "))"s,
        1};
}

ساكن AstIf* newPassOnIf(File_کرښه* flp, Ast_غوټه_بیان* firep, Ast_غوټه* bodyp, VAssertType type,
                          VAssertDirectiveType directiveType, منطقي vacuous) {
    Ast_غوټه_بیان* ثابت condp
        = جديد AstLogAnd{flp, firep, assertPassOnCond(flp, type, directiveType, vacuous)};
    AstIf* ثابت ifp = جديد AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(ریښتیا);
    ifp->user1(ریښتیا);
    أرجع ifp;
}

ساكن Ast_غوټه_Stmt* newIfAssertFailOn(Ast_غوټه* bodyp, VAssertDirectiveType directiveType,
                                      VAssertType type) {
    File_کرښه* ثابت flp = bodyp->fileline();
    Ast_غوټه_بیان* ثابت condp = assertFailOnCond(flp, type, directiveType);
    AstIf* ثابت ifp = جديد AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(ریښتیا);
    ifp->user1(ریښتیا);
    أرجع ifp;
}

//######################################################################
// NFA Builder

صنف SvaNfaBuilder final {
    Sva_ګراف& m_ګراف;  // NFA graph being built
    Ast_غوټه_Module* ثابت m_modp;  // Module to receive hoisted sampled-prop temps
    V3_Unique_نومونه& m_prop_لنډمهاله_نومونه;  // Module-shared temp-var name source
    std::ویکټور<Ast_غوټه_بیان*> m_temporal_Guard_ډېری;  // Guards active across nested temporal states
    // Outer abort conditions, AND-ed as !cond into inner abort edges
    // (IEEE 1800-2023 16.12.14 outer-wraps-inner).
    std::ویکټور<Ast_غوټه_بیان*> m_outer_Abort_ډېری;
    منطقي m_دننه_Unbounded_Scope = خطأ;  // Sticky: nodes created after inherit liveness
    منطقي m_markStrongPending = خطأ;  // Mark new vertices as strong s_always in-window
    منطقي m_isCover = خطأ;  // Cover directives do not fail at end-of-simulation
    // IEEE 1800-2023 16.14.3 cover sequence: each end-of-match fires the action,
    // not just the first. Builder builds parallel-branch (no first-match-wins)
    // topology when true. Default false preserves cover_property semantics.
    منطقي m_isCoverSeq = خطأ;
    // Unsupported endpoint topology must reject, not ignore, or the wait hangs
    منطقي m_isSeqEvent = خطأ;
    منطقي m_دننه_Sequence_مختاړی = خطأ;  // Prefix composition does not preserve every endpoint
    منطقي m_want_Bool_ونې_پاڼه = خطأ;  // Building a |-> / |=> or top-level body (see buildOrMerge)

    بنية RangeDelayRejectInfo final {
        Sva_State_راس* startp = تش_اشاره;
        بینښه range = 0;
        صحيح rhsLen = 0;
    };

    فراغ warnEndpointUnsupported(File_کرښه* flp, ثابت string& what) ثابت {
        إذا (m_isSeqEvent) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: sequence used as an event control with " << what);
        } وإلا {
            flp->v3warn(COVERIGN, "Ignoring unsupported: cover sequence with " << what);
        }
    }

    Ast_غوټه_بیان* throughoutCond(Ast_غوټه_بیان* بنسټ_Condp, File_کرښه* flp) {
        إذا (m_temporal_Guard_ډېری.تشه()) أرجع بنسټ_Condp;
        // AND all active temporal guards (supports nesting)
        // Each must use $sampled values.
        Ast_غوټه_بیان* guardp = تش_اشاره;
        لكل (Ast_غوټه_بیان* ثابت condp : m_temporal_Guard_ډېری) {
            Ast_غوټه_بیان* ثابت clonep = sampled(condp->clone_ونه_خالص(خطأ));
            إذا (!guardp) {
                guardp = clonep;
            } وإلا {
                guardp = جديد AstLogAnd{flp, guardp, clonep};
            }
        }
        إذا (بنسټ_Condp) { guardp = جديد AstLogAnd{flp, بنسټ_Condp, guardp}; }
        أرجع guardp;
    }

    ساكن بینښه getConstUInt(Ast_غوټه_بیان* exprp) {
        Ast_غوټه_بیان* ثابت constp = V3Const::constifyEdit(exprp->clone_ونه_خالص(خطأ));
        ثابت AstConst* ثابت cp = VN_CAST(constp, Const);
        ثابت بینښه val = cp ? cp->منزل_UInt() : 0;
        VL_DO_DANGLING(constp->delete_ونه(), constp);
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
    ساكن صحيح fixed_اوږدتیا(Ast_غوټه_بیان* nodep) {
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
            إذا (!delayp || !delayp->is_دوره_Delay()) أرجع -1;
            بینښه delay_دورې;
            إذا (delayp->isRangeDelay()) {
                إذا (delayp->isUnbounded()) أرجع -1;  // LCOV_EXCL_LINE
                ثابت بینښه minD = getConstUInt(delayp->lhsp());
                ثابت بینښه maxD = getConstUInt(delayp->rhsp());
                إذا (minD != maxD) أرجع -1;
                delay_دورې = minD;
            } وإلا {
                delay_دورې = getConstUInt(delayp->lhsp());
            }
            صحيح preLen = 0;
            إذا (Ast_غوټه_بیان* ثابت prep = sexprp->preExprp()) {
                preLen = fixed_اوږدتیا(prep);
                إذا (preLen < 0) أرجع -1;  // LCOV_EXCL_LINE
            }
            ثابت صحيح bodyLen = fixed_اوږدتیا(sexprp->exprp());
            إذا (bodyLen < 0) أرجع -1;  // LCOV_EXCL_LINE
            أرجع preLen + delay_دورې + bodyLen;
        }
        إذا (AstSThroughout* ثابت throughp = VN_CAST(nodep, SThroughout)) {
            أرجع fixed_اوږدتیا(throughp->rhsp());
        }
        إذا (AstSOr* ثابت orp = VN_CAST(nodep, SOr)) {
            // Alternatives must share one end cycle; buildSWithin relies on
            // this to pair the OR with an SIntersect.
            ثابت صحيح lhsLen = fixed_اوږدتیا(orp->lhsp());
            ثابت صحيح rhsLen = fixed_اوږدتیا(orp->rhsp());
            إذا (lhsLen < 0 || rhsLen < 0 || lhsLen != rhsLen) أرجع -1;
            أرجع lhsLen;
        }
        إذا (AstSWithin* ثابت withinp = VN_CAST(nodep, SWithin)) {
            // `seq1 within seq2` ends at seq2's end cycle (IEEE 16.9.10).
            ثابت صحيح lhsLen = fixed_اوږدتیا(withinp->lhsp());
            ثابت صحيح rhsLen = fixed_اوږدتیا(withinp->rhsp());
            إذا (lhsLen < 0 || rhsLen < 0 || lhsLen > rhsLen) أرجع -1;
            أرجع rhsLen;
        }
        // LCOV_EXCL_START -- defensive: V3AssertPre rejects composite SVA ops
        // nested in an intersect arm before fixedLength runs (clock-context
        // resolution fails). Kept as a guard in case future parser relaxations
        // permit it.
        إذا (nodep->is_Multi_دوره_Sva()) أرجع -1;
        // LCOV_EXCL_STOP
        // Plain boolean expression (no SVA constructs) -- 0 cycles.
        أرجع 0;
    }

    // Contiguous match-length range [lo,hi] for an operand whose length varies
    // from AT MOST ONE ranged cycle delay; {-1,-1} otherwise. Drives the
    // variable-length `intersect` lowering (IEEE 1800-2023 16.9.6): more than
    // one ranged delay would make the per-length realization ambiguous, so it
    // is reported unsupported rather than mis-paired.
    ساكن std::pair<صحيح, صحيح> اوږدتیا_Range(Ast_غوټه_بیان* nodep) {
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
            إذا (!delayp || !delayp->is_دوره_Delay()) أرجع {-1, -1};
            std::pair<صحيح, صحيح> delayRange;
            إذا (delayp->isRangeDelay()) {
                إذا (delayp->isUnbounded()) أرجع {-1, -1};
                ثابت بینښه minD = getConstUInt(delayp->lhsp());
                ثابت بینښه maxD = getConstUInt(delayp->rhsp());
                delayRange = {minD, maxD};
            } وإلا {
                ثابت بینښه d = getConstUInt(delayp->lhsp());
                delayRange = {d, d};
            }
            std::pair<صحيح, صحيح> preRange{0, 0};
            إذا (Ast_غوټه_بیان* ثابت prep = sexprp->preExprp()) {
                preRange = اوږدتیا_Range(prep);
                إذا (preRange.لومړی < 0) أرجع {-1, -1};
            }
            ثابت std::pair<صحيح, صحيح> bodyRange = اوږدتیا_Range(sexprp->exprp());
            إذا (bodyRange.لومړی < 0) أرجع {-1, -1};
            ثابت صحيح variableParts = (preRange.لومړی != preRange.دوهم)
                                      + (delayRange.لومړی != delayRange.دوهم)
                                      + (bodyRange.لومړی != bodyRange.دوهم);
            إذا (variableParts > 1) أرجع {-1, -1};
            أرجع {preRange.لومړی + delayRange.لومړی + bodyRange.لومړی,
                    preRange.دوهم + delayRange.دوهم + bodyRange.دوهم};
        }
        إذا (AstSThroughout* ثابت throughp = VN_CAST(nodep, SThroughout)) {
            أرجع اوږدتیا_Range(throughp->rhsp());
        }
        إذا (nodep->is_Multi_دوره_Sva()) أرجع {-1, -1};
        أرجع {0, 0};  // plain boolean -- 0 cycles
    }

    // Clone `operand` with its sole variable ranged cycle delay pinned so the
    // total match length is exactly `len`. `lo` is the operand's minimum length
    // (lengthRange().first). A fixed operand has no such delay and is returned
    // as a plain clone (callers only request its single achievable length).
    ساكن Ast_غوټه_بیان* realize_At_اوږدتیا(Ast_غوټه_بیان* عملوند, صحيح len, صحيح lo) {
        Ast_غوټه_بیان* ثابت clonep = عملوند->clone_ونه_خالص(خطأ);
        AstDelay* rangeDelayp = تش_اشاره;
        clonep->foreach([&](AstDelay* dp) {
            إذا (!rangeDelayp && dp->isRangeDelay() && !dp->isUnbounded()
                && getConstUInt(dp->lhsp()) != getConstUInt(dp->rhsp())) {
                rangeDelayp = dp;
            }
        });
        إذا (rangeDelayp) {
            File_کرښه* ثابت flp = rangeDelayp->fileline();
            ثابت بینښه pinned = getConstUInt(rangeDelayp->lhsp()) + (len - lo);
            Ast_غوټه_بیان* ثابت oldMinp = rangeDelayp->lhsp();
            oldMinp->replaceWith(جديد AstConst{flp, pinned});
            VL_DO_DANGLING(oldMinp->delete_ونه(), oldMinp);
            // Drop the max bound so it lowers as a fixed `##d`, not `##[d:d]`.
            Ast_غوټه* ثابت oldMaxp = rangeDelayp->rhsp()->unlink_Fr_شا();
            VL_DO_DANGLING(oldMaxp->delete_ونه(), oldMaxp);
        }
        أرجع clonep;
    }

    // Cuts AST size from O(N * sizeof(exprp)) to O(N) + O(sizeof(exprp)) by
    // sharing a single `VarRef` across N check edges. Hoist also matches the
    // IEEE 1800-2023 16.9.9 "single preponed-region snapshot" semantic for
    // any exprp -- even an impure one would now evaluate exactly once per
    // clock instead of N times. Orphan temps from failed builds are unused
    // MODULETEMPs and are removed by V3Dead.
    AstVar* tryHoistSampled(Ast_غوټه_بیان* exprp, File_کرښه* flp, بینښه clone_شمېر) {
        constexpr بینښه k_Hoist_پوله = 2;
        إذا (clone_شمېر < k_Hoist_پوله) أرجع تش_اشاره;
        AstVar* ثابت لنډمهاله_Varp
            = جديد AstVar{flp, VVarType::MODULETEMP, m_prop_لنډمهاله_نومونه.get(exprp), exprp->dtypep()};
        m_modp->جمع_کړه_Stmtsp(لنډمهاله_Varp);
        Ast_ټاکل* ثابت assignp = جديد Ast_ټاکل{flp, جديد AstVarRef{flp, لنډمهاله_Varp, VAccess::WRITE},
                                                 sampled(exprp->clone_ونه_خالص(خطأ))};
        m_modp->جمع_کړه_Stmtsp(جديد AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, تش_اشاره, assignp});
        أرجع لنډمهاله_Varp;
    }

    ساكن Ast_غوټه_بیان* sampledRefOrClone(AstVar* hoistVarp, Ast_غوټه_بیان* exprp, File_کرښه* flp) {
        إذا (hoistVarp) أرجع جديد AstVarRef{flp, hoistVarp, VAccess::READ};
        أرجع sampled(exprp->clone_ونه_خالص(خطأ));
    }

    // Create vertex and inherit temporal guards from the current scope.
    Sva_State_راس* scoped_Create_راس() {
        Sva_State_راس* ثابت vtxp = m_ګراف.create_State_راس();
        لكل (Ast_غوټه_بیان* ثابت cp : m_temporal_Guard_ډېری) {
            vtxp->m_throughoutConds.ټېل_شا(cp->clone_ونه_خالص(خطأ));
        }
        إذا (m_دننه_Unbounded_Scope) vtxp->m_isUnbounded = ریښتیا;
        إذا (m_markStrongPending) vtxp->m_strongPending = ریښتیا;
        أرجع vtxp;
    }

    // AND current temporal guards into every edge/link.
    Sva_Trans_څنډه* guardedLink(Sva_State_راس* fromp, Sva_State_راس* سر, Ast_غوټه_بیان* condp,
                              File_کرښه* flp) {
        أرجع m_ګراف.جمع_کړه_Link(fromp, سر, throughoutCond(condp, flp));
    }
    Sva_Trans_څنډه* guardedLink(Sva_State_راس* fromp, Sva_State_راس* سر, File_کرښه* flp) {
        أرجع m_ګراف.جمع_کړه_Link(fromp, سر, throughoutCond(تش_اشاره, flp));
    }
    Sva_Trans_څنډه* guarded_څنډه(Sva_State_راس* fromp, Sva_State_راس* سر, Ast_غوټه_بیان* condp,
                              File_کرښه* flp) {
        أرجع m_ګراف.جمع_کړه_Clocked_څنډه(fromp, سر, throughoutCond(condp, flp));
    }
    Sva_Trans_څنډه* guarded_څنډه(Sva_State_راس* fromp, Sva_State_راس* سر, File_کرښه* flp) {
        أرجع m_ګراف.جمع_کړه_Clocked_څنډه(fromp, سر, throughoutCond(تش_اشاره, flp));
    }

    Sva_State_راس* جمع_کړه_Delay_Chain(Sva_State_راس* startp, بینښه اندازه, File_کرښه* flp,
                                  منطقي isFixed = ریښتیا, Ast_غوټه_بیان* clearCondp = تش_اشاره,
                                  Ast_غوټه_بیان* advanceCondp = تش_اشاره) {
        إذا (isFixed && اندازه == 0) أرجع startp;
        UASSERT_OBJ(اندازه > 0, startp, "Delay chain needs at least one slot");
        إذا (isFixed && اندازه == 1 && !advanceCondp) {
            Sva_State_راس* ثابت nextp = scoped_Create_راس();
            guarded_څنډه(startp, nextp, flp);
            أرجع nextp;
        }
        Sva_State_راس* ثابت ringVtxp = scoped_Create_راس();
        ringVtxp->m_isFixedDelayRing = isFixed;
        ringVtxp->m_delay_Ring_اندازه = اندازه;
        إذا (clearCondp) {
            UASSERT_OBJ(!isFixed, startp, "Fixed delay cannot have a clear condition");
            ringVtxp->m_delayRingClearCondp = clearCondp->clone_ونه_خالص(خطأ);
        }
        ringVtxp->m_delayRingAdvanceCondp = advanceCondp;
        إذا (isFixed) {
            guarded_څنډه(startp, ringVtxp, flp);
        } وإلا {
            guardedLink(startp, ringVtxp, flp);
        }
        أرجع ringVtxp;
    }

    // Build NFA for an SExpr. finalCond = RHS (not yet added as a vertex).
    // isTopLevelStep: marks outermost required boolean check as rejectOnFail.
    // Apply a range delay `##[M:N]` to currentp. Returns true on success. On
    // failure, emits a diagnostic and returns false.
    منطقي applyRangeDelay(AstSExpr* sexprp, Sva_State_راس*& currentp,
                         std::ویکټور<Sva_State_راس*>& midSources,
                         RangeDelayRejectInfo* rangeRejectInfop = تش_اشاره) {
        File_کرښه* ثابت flp = sexprp->fileline();
        AstDelay* ثابت delayp = VN_AS(sexprp->delayp(), Delay);
        Ast_غوټه_بیان* ثابت rhsExprp = sexprp->exprp();
        ثابت بینښه minDelay = getConstUInt(delayp->lhsp());
        إذا (delayp->isUnbounded()) {
            // `##[M:$]`: wait M cycles, then retain every matured attempt in a
            // one-slot ring while also exposing the first eligible tick.
            currentp = جمع_کړه_Delay_Chain(currentp, minDelay, flp);
            Sva_State_راس* ثابت waitRingp = جمع_کړه_Delay_Chain(currentp, 1, flp, خطأ);
            // The ring's live count is updated in NBA, so this link retains the previous
            // tick's live attempts alongside new arrivals for the next tick.
            guardedLink(waitRingp, waitRingp, flp);
            Sva_State_راس* ثابت یوځای_Vtxp = scoped_Create_راس();
            guardedLink(currentp, یوځای_Vtxp, flp);
            guardedLink(waitRingp, یوځای_Vtxp, flp);
            currentp = یوځای_Vtxp;
            currentp->m_isUnbounded = ریښتیا;  // Liveness, so no reject.
            m_دننه_Unbounded_Scope = ریښتیا;
            أرجع ریښتیا;
        }
        ثابت بینښه maxDelay = getConstUInt(delayp->rhsp());
        إذا (minDelay == maxDelay) {
            currentp = جمع_کړه_Delay_Chain(currentp, minDelay, flp);
            أرجع ریښتیا;
        }
        ثابت بینښه range = maxDelay - minDelay;
        currentp = جمع_کړه_Delay_Chain(currentp, minDelay, flp);
        ثابت منطقي multi_دوره_Rhs = rhsExprp->is_Multi_دوره_Sva();
        ثابت Ast_غوټه_بیان* ثابت preExprp = sexprp->preExprp();
        // Prefix and nested-sequence composition can lose endpoints or their multiplicity
        // required by cover sequence (IEEE 1800-2023 16.14.3), regardless of the range width.
        إذا (m_isCoverSeq
            && (multi_دوره_Rhs || m_دننه_Sequence_مختاړی
                || (preExprp && preExprp->is_Multi_دوره_Sva()))) {
            warnEndpointUnsupported(flp, "this ranged cycle delay");
            أرجع خطأ;
        }
        إذا (m_دننه_Sequence_مختاړی) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: Bounded ranged cycle delay in a sequence prefix");
            أرجع خطأ;
        }
        إذا (multi_دوره_Rhs) {
            // Launch a candidate at every eligible tick. Candidate-local misses are not
            // assertion rejects while a later position can still match.
            إذا (rangeRejectInfop) {
                ثابت صحيح rhsLen = fixed_اوږدتیا(rhsExprp);
                إذا (rhsLen >= 0) *rangeRejectInfop = {currentp, range, rhsLen};
            }
            Sva_State_راس* ثابت tailp = جمع_کړه_Delay_Chain(currentp, range + 1U, flp, خطأ);
            Sva_State_راس* ثابت یوځای_Vtxp = scoped_Create_راس();
            یوځای_Vtxp->m_isUnbounded = ریښتیا;
            guardedLink(currentp, یوځای_Vtxp, flp);
            guardedLink(tailp, یوځای_Vtxp, flp);
            currentp = یوځای_Vtxp;
            m_دننه_Unbounded_Scope = ریښتیا;
        } وإلا {
            // The first eligible tick is explicit. The ring retains attempts for the
            // remaining ticks and exposes the final tick for rejection. Only properties
            // clear on success, since cover sequence counts every endpoint.
            midSources.ټېل_شا(currentp);
            currentp = جمع_کړه_Delay_Chain(currentp, range + 1U, flp, خطأ,
                                     m_isCoverSeq ? تش_اشاره : rhsExprp);
        }
        أرجع ریښتیا;
    }

    فراغ جمع_کړه_Finite_Range_Reject(ثابت RangeDelayRejectInfo& info, ثابت Build_پایله& پایله,
                              File_کرښه* flp) {
        إذا (!info.startp) أرجع;

        Sva_State_راس* ثابت expiryVtxp
            = جمع_کړه_Delay_Chain(info.startp, info.range + info.rhsLen, flp);
        Sva_State_راس* ثابت expiryMatchp = scoped_Create_راس();
        std::ویکټور<Sva_State_راس*> sources = پایله.midSources;
        sources.ټېل_شا(پایله.غړی_Vertexp);
        لكل (Sva_State_راس* ثابت srcp : sources) {
            Ast_غوټه_بیان* ثابت condp
                = پایله.finalCondp ? sampled(پایله.finalCondp->clone_ونه_خالص(خطأ)) : تش_اشاره;
            Sva_State_راس* ثابت successNowp = scoped_Create_راس();
            guardedLink(srcp, successNowp, condp, flp);
            // Retain a success through the latest deadline it can satisfy.
            Sva_State_راس* ثابت historyp
                = جمع_کړه_Delay_Chain(successNowp, info.range + 1U, flp, خطأ);
            guardedLink(successNowp, expiryMatchp, flp);
            guardedLink(historyp, expiryMatchp, flp);
        }

        Sva_State_راس* ثابت sinkVtxp = m_ګراف.create_State_راس();
        sinkVtxp->m_isRejectSink = ریښتیا;
        Sva_Trans_څنډه* ثابت rejectp = m_ګراف.جمع_کړه_Link(expiryVtxp, sinkVtxp);
        rejectp->m_rejectOnFail = ریښتیا;
        rejectp->m_condVtxp = expiryMatchp;
    }

    Build_پایله buildSExpr(AstSExpr* sexprp, Sva_State_راس* ثبت_Vtxp,
                           منطقي is_سر_Level_ګام = خطأ) {
        AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
        إذا (!delayp || !delayp->is_دوره_Delay()) أرجع Build_پایله::fail();

        File_کرښه* ثابت flp = sexprp->fileline();
        Ast_غوټه_بیان* ثابت exprp = sexprp->exprp();

        // Handle LHS (preExpr)
        Sva_State_راس* currentp = ثبت_Vtxp;
        إذا (Ast_غوټه_بیان* ثابت preExprp = sexprp->preExprp()) {
            VL_RESTORER(m_دننه_Sequence_مختاړی);
            m_دننه_Sequence_مختاړی = ریښتیا;
            ثابت Build_پایله pre = build_بیان(preExprp, currentp, is_سر_Level_ګام);
            إذا (!pre.valid()) أرجع Build_پایله::fail(pre.errorEmitted);  // LCOV_EXCL_LINE
            إذا (pre.finalCondp) {
                Sva_State_راس* ثابت condVtxp = scoped_Create_راس();
                Sva_Trans_څنډه* ثابت edgep = guardedLink(
                    pre.غړی_Vertexp, condVtxp, sampled(pre.finalCondp->clone_ونه_خالص(خطأ)), flp);
                إذا (is_سر_Level_ګام && !pre.غړی_Vertexp->m_isUnbounded && !m_دننه_Unbounded_Scope) {
                    // Do not mark liveness sources: first boolean check is deferred.
                    edgep->m_rejectOnFail = ریښتیا;
                }
                freeUnlinkedCondp(pre.finalCondp);
                currentp = condVtxp;
            } وإلا {
                currentp = pre.غړی_Vertexp;
            }
        }

        // Handle delay
        std::ویکټور<Sva_State_راس*> rangeMidSources;
        RangeDelayRejectInfo rangeRejectInfo;
        ثابت منطقي جمع_کړه_Range_Reject = is_سر_Level_ګام && !m_دننه_Unbounded_Scope;
        إذا (delayp->isRangeDelay()) {
            إذا (!applyRangeDelay(sexprp, currentp, rangeMidSources,
                                 جمع_کړه_Range_Reject ? &rangeRejectInfo : تش_اشاره)) {
                أرجع Build_پایله::failWithError();
            }
        } وإلا {
            ثابت بینښه delay_دورې = getConstUInt(delayp->lhsp());
            currentp = جمع_کړه_Delay_Chain(currentp, delay_دورې, flp);
        }

        // Multi-cycle RHS: recurse (only plain boolean is returned as finalCondp).
        إذا (exprp->is_Multi_دوره_Sva()) {
            ثابت Build_پایله پایله = build_بیان(exprp, currentp, is_سر_Level_ګام);
            إذا (پایله.valid()) جمع_کړه_Finite_Range_Reject(rangeRejectInfo, پایله, flp);
            أرجع پایله;
        }
        أرجع {currentp, exprp, std::move(rangeMidSources)};
    }

    Build_پایله buildConsRep(AstSConsRep* repp, Sva_State_راس* ثبت_Vtxp,
                             منطقي is_سر_Level_ګام = خطأ) {
        File_کرښه* ثابت flp = repp->fileline();
        Ast_غوټه_بیان* ثابت exprp = repp->exprp();
        // Multi-cycle expr in ConsRep not yet supported; bail to avoid invalid AST.
        إذا (exprp->is_Multi_دوره_Sva()) {
            repp->v3warn(E_UNSUPPORTED, "Unsupported: multi-cycle sequence expression inside"
                                        " consecutive repetition (IEEE 1800-2023 16.9.2)");
            أرجع Build_پایله::failWithError();
        }
        ثابت بینښه minN = getConstUInt(repp->countp());

        // Sum sites across prefix + unbounded/range tail so one hoist covers
        // every check edge of this repetition.
        بینښه ټول_Sites = minN;
        إذا (repp->unbounded()) {
            ټول_Sites += 1;
        } وإلا إذا (repp->maxCountp()) {
            ټول_Sites += getConstUInt(repp->maxCountp()) - minN;
        }
        AstVar* ثابت hoistVarp = tryHoistSampled(exprp, flp, ټول_Sites);

        // Cover-sequence (IEEE 1800-2023 16.14.3): collect each end-of-match
        // position so they all fire the action, not just the merged terminal.
        std::ویکټور<Sva_State_راس*> consMidSources;

        Sva_State_راس* currentp = ثبت_Vtxp;
        لكل (بینښه i = 0; i < minN; ++i) {
            // Keep the first repetition explicit, collapse all remaining checks into the ring.
            إذا (i == 1) {
                currentp = جمع_کړه_Delay_Chain(currentp, minN - 1, flp);
                currentp->m_delayRingClearCondp
                    = جديد AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                currentp->m_replayAbortReject = ریښتیا;
                إذا (is_سر_Level_ګام) {
                    currentp->m_throughoutConds.ټېل_شا(
                        sampledRefOrClone(hoistVarp, exprp, flp));
                }
                i = minN - 1;
            }
            // Every repetition in the minimum prefix is required.
            Sva_State_راس* ثابت condVtxp = scoped_Create_راس();
            Sva_Trans_څنډه* ثابت linkp
                = guardedLink(currentp, condVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            // Only an outermost required step rejects the property. Its first check is explicit,
            // later required checks reject through the ring's throughout guard.
            إذا (is_سر_Level_ګام && i == 0) linkp->m_rejectOnFail = ریښتیا;
            currentp = condVtxp;
        }
        // After minN: currentp is the first valid end-of-match position for [*m:n].
        إذا (m_isCoverSeq && (repp->unbounded() || repp->maxCountp())) {
            consMidSources.ټېل_شا(currentp);
        }

        إذا (repp->unbounded()) {
            إذا (minN == 0) {
                Sva_State_راس* ثابت waitVtxp = scoped_Create_راس();
                guarded_څنډه(currentp, waitVtxp, flp);
                Sva_State_راس* ثابت checkVtxp = scoped_Create_راس();
                guardedLink(waitVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guarded_څنډه(checkVtxp, waitVtxp, flp);
                guardedLink(currentp, checkVtxp, flp);
                currentp = checkVtxp;
            } وإلا {
                Sva_State_راس* ثابت loop_شا_Vtxp = scoped_Create_راس();
                guarded_څنډه(currentp, loop_شا_Vtxp, flp);
                Sva_State_راس* ثابت reCheckVtxp = scoped_Create_راس();
                guardedLink(loop_شا_Vtxp, reCheckVtxp, sampledRefOrClone(hoistVarp, exprp, flp),
                            flp);
                guarded_څنډه(reCheckVtxp, loop_شا_Vtxp, flp);
                guardedLink(reCheckVtxp, currentp, flp);
            }
            currentp->m_isUnbounded = ریښتیا;
            m_دننه_Unbounded_Scope = ریښتیا;
        } وإلا إذا (repp->maxCountp()) {
            ثابت بینښه maxN = getConstUInt(repp->maxCountp());
            Sva_State_راس* ثابت یوځای_Vtxp = scoped_Create_راس();
            guardedLink(currentp, یوځای_Vtxp, flp);
            بینښه لکۍ_Min_N = minN;
            Sva_State_راس* لکۍ_Startp = currentp;
            إذا (minN == 0) {
                // Build the first optional iteration explicitly. Feeding the empty endpoint
                // directly into a range ring would incorrectly keep that match alive.
                Sva_State_راس* ثابت بل_Vtxp = scoped_Create_راس();
                guarded_څنډه(currentp, بل_Vtxp, flp);
                Sva_State_راس* ثابت checkVtxp = scoped_Create_راس();
                guardedLink(بل_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, یوځای_Vtxp, flp);
                إذا (m_isCoverSeq) consMidSources.ټېل_شا(checkVtxp);
                لکۍ_Startp = checkVtxp;
                لکۍ_Min_N = 1;
            }
            إذا (maxN > لکۍ_Min_N) {
                // Add tail-ring only if the tail is non-empty.
                Sva_State_راس* ثابت بل_Vtxp
                    = جمع_کړه_Delay_Chain(لکۍ_Startp, maxN - لکۍ_Min_N + 1, flp, خطأ);
                بل_Vtxp->m_delayRingClearCondp
                    = جديد AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                بل_Vtxp->m_replayAbortReject = ریښتیا;
                Sva_State_راس* ثابت checkVtxp = scoped_Create_راس();
                guardedLink(بل_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, یوځای_Vtxp, flp);
                إذا (m_isCoverSeq) consMidSources.ټېل_شا(checkVtxp);
            }
            currentp = یوځای_Vtxp;
        }
        // finalCond = nullptr (already checked via Links)
        Build_پایله res;
        res.غړی_Vertexp = currentp;
        res.finalCondp = تش_اشاره;
        res.midSources = std::move(consMidSources);
        // mergeVtxp is the OR of all the end-positions we already pushed to
        // midSources, so the main termVtxp -> matchVertex Link would duplicate.
        res.غړی_Is_Mid_یوځای = m_isCoverSeq && !res.midSources.تشه();
        أرجع res;
    }

    // always[lo:hi] / s_always[lo:hi] (IEEE 1800-2023 16.12.11).
    Build_پایله buildPropAlways(AstPropAlways* nodep, Sva_State_راس* ثبت_Vtxp,
                                منطقي is_سر_Level_ګام = خطأ) {
        File_کرښه* ثابت flp = nodep->fileline();
        Ast_غوټه_بیان* ثابت propp = nodep->propp();
        ثابت بینښه lo = getConstUInt(nodep->loBoundp());
        إذا (VN_IS(nodep->hiBoundp(), Unbounded)) {
            // Weak always [lo:$]: unbounded upper bound (IEEE 1800-2023 16.12.11).
            // p must hold at every clock tick at least lo cycles after the attempt
            // start; those ticks are not required to exist, so there is no
            // end-of-trace obligation (weak). The self-loop keeps the attempt live
            // every cycle; each observed cycle is a safety obligation, so a false p
            // rejects immediately.
            UASSERT_OBJ(!nodep->isStrong(), nodep, "Unbounded always must be weak (V3Width)");
            Sva_State_راس* ثابت livep = جمع_کړه_Delay_Chain(ثبت_Vtxp, lo, flp);
            livep->m_isUnbounded = ریښتیا;
            guarded_څنډه(livep, livep, flp);  // stay active every subsequent cycle
            Sva_State_راس* ثابت sinkp = m_ګراف.create_State_راس();
            sinkp->m_isRejectSink = ریښتیا;
            Sva_Trans_څنډه* ثابت rejEdgep
                = guardedLink(livep, sinkp, sampled(propp->clone_ونه_خالص(خطأ)), flp);
            إذا (is_سر_Level_ګام) rejEdgep->m_rejectOnFail = ریښتیا;
            أرجع {livep, تش_اشاره, {}};
        }
        ثابت بینښه hi = getConstUInt(nodep->hiBoundp());
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
        Sva_State_راس* currentp = جمع_کړه_Delay_Chain(ثبت_Vtxp, lo, flp);
        Sva_State_راس* ثابت checkp = scoped_Create_راس();
        Sva_Trans_څنډه* ثابت linkp
            = guardedLink(currentp, checkp, sampled(propp->clone_ونه_خالص(خطأ)), flp);
        إذا (is_سر_Level_ګام) linkp->m_rejectOnFail = ریښتیا;
        currentp = checkp;
        m_temporal_Guard_ډېری.ټېل_شا(propp);
        currentp = جمع_کړه_Delay_Chain(currentp, hi - lo, flp);
        إذا (nodep->isStrong() && currentp->m_delay_Ring_اندازه) currentp->m_strongAlwaysRing = ریښتیا;
        m_temporal_Guard_ډېری.راوباسه_شا();
        أرجع {currentp, propp, {}};
    }

    Build_پایله buildGotoRep(AstSGotoRep* repp, Sva_State_راس* ثبت_Vtxp) {
        File_کرښه* ثابت flp = repp->fileline();
        Ast_غوټه_بیان* ثابت exprp = repp->exprp();
        ثابت بینښه minN = getConstUInt(repp->countp());
        إذا (minN == 0) أرجع Build_پایله::fail();
        ثابت منطقي hasMax = repp->maxCountp() != تش_اشاره;
        ثابت بینښه maxN = hasMax ? getConstUInt(repp->maxCountp()) : minN;
        إذا (m_isCoverSeq) {
            // Several attempts may coalesce in the one-bit wait vertex before reaching the ring,
            // so a cover sequence action block could run too few times.
            warnEndpointUnsupported(flp, "a goto repetition");
            أرجع Build_پایله::failWithError();
        }

        AstVar* ثابت hoistVarp = tryHoistSampled(exprp, flp, 2);

        // The first guardedEdge is the ##1 before waiting for a match. In the wait state, false
        // takes the clocked self-loop, while true takes the zero-delay guardedLink on that tick.
        Sva_State_راس* ثابت waitVtxp = scoped_Create_راس();
        guarded_څنډه(ثبت_Vtxp, waitVtxp, flp);
        guarded_څنډه(waitVtxp, waitVtxp,
                    جديد AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)}, flp);
        Sva_State_راس* currentp = scoped_Create_راس();
        guardedLink(waitVtxp, currentp, sampledRefOrClone(hoistVarp, exprp, flp), flp);

        إذا (minN > 1) {
            Sva_State_راس* ثابت ringVtxp = جمع_کړه_Delay_Chain(
                currentp, minN - 1, flp, ریښتیا, تش_اشاره, sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = ریښتیا;
            Sva_State_راس* ثابت checkVtxp = scoped_Create_راس();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            currentp = checkVtxp;
        }
        إذا (!hasMax) {
            currentp->m_isUnbounded = ریښتیا;  // [->N] waits unboundedly
            m_دننه_Unbounded_Scope = ریښتیا;
            أرجع {currentp, تش_اشاره, {}};
        }

        // [->M:N]: the range ring holds matches from M through N and advances
        // only on expr, preserving arbitrarily long gaps between occurrences.
        Sva_State_راس* ثابت یوځای_Vtxp = scoped_Create_راس();
        guardedLink(currentp, یوځای_Vtxp, flp);  // accept at match_M
        إذا (maxN > minN) {
            Sva_State_راس* ثابت ringVtxp
                = جمع_کړه_Delay_Chain(currentp, maxN - minN + 1, flp, خطأ, تش_اشاره,
                                sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = ریښتیا;
            Sva_State_راس* ثابت checkVtxp = scoped_Create_راس();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            guardedLink(checkVtxp, یوځای_Vtxp, flp);
        }
        یوځای_Vtxp->m_isUnbounded = ریښتیا;  // [->M:N] still has unbounded waits between matches
        m_دننه_Unbounded_Scope = ریښتیا;
        أرجع {یوځای_Vtxp, تش_اشاره, {}};
    }

    // True if the sequence has a zero-minimum consecutive repetition, which may match empty
    ساكن منطقي hasZeroMinRep(ثابت Ast_غوټه_بیان* seqp) {
        أرجع seqp->exists(
            [](ثابت AstSConsRep* repp) { أرجع getConstUInt(repp->countp()) == 0; });
    }

    // True if a leaf's condition has no implication or multi-cycle sequence in it
    // (a leaf can be e.g. 'not (x |-> y)')
    ساكن منطقي isPlainBoolean(ثابت Ast_غوټه_بیان* condp) {
        أرجع !condp->exists(
            [](ثابت Ast_غوټه_بیان* np) { أرجع np->is_Multi_دوره_Sva() || VN_IS(np, Implication); });
    }

    // Build merge vertex for SOr / LogOr: both branches feed into one vertex.
    Build_پایله build_Or_یوځای(Ast_غوټه_بیان* lhsp, Ast_غوټه_بیان* rhsp, Sva_State_راس* ثبت_Vtxp,
                             File_کرښه* flp, منطقي want_Bool_ونې_پاڼه) {
        ثابت Build_پایله lhs = build_بیان(lhsp, ثبت_Vtxp);
        ثابت Build_پایله rhs = build_بیان(rhsp, ثبت_Vtxp);
        إذا (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع Build_پایله::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP
        // IEEE 1800-2023 16.14.3: a cover sequence counts every end-of-match. A
        // sequence operand of 'or' can end more than once, but only its final
        // end reaches the merge vertex below, so reject sequence operands rather
        // than under-count. Plain boolean disjunction has one end per cycle and
        // is handled by the OR-fold.
        ثابت منطقي booleanOnly = lhs.غړی_Vertexp == ثبت_Vtxp && lhs.finalCondp
                                 && rhs.غړی_Vertexp == ثبت_Vtxp && rhs.finalCondp;
        إذا (m_isCoverSeq && !booleanOnly) {
            warnEndpointUnsupported(flp, "a sequence operand of 'or'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع Build_پایله::failWithError();
        }
        إذا (booleanOnly && want_Bool_ونې_پاڼه) {
            إذا (isPlainBoolean(lhs.finalCondp) && isPlainBoolean(rhs.finalCondp)) {
                // As a |-> / |=> or top-level body, a disjunction of two plain booleans must
                // be able to reject: return it as a boolean finalCondp. The merge vertex
                // below drops the attempt silently when both operands are false.
                Ast_غوټه_بیان* ثابت orCondp
                    = جديد AstLogOr{flp, lhs.finalCondp->clone_ونه_خالص(خطأ),
                                   rhs.finalCondp->clone_ونه_خالص(خطأ)};
                freeUnlinkedCondp(lhs.finalCondp);
                freeUnlinkedCondp(rhs.finalCondp);
                أرجع {ثبت_Vtxp, orCondp, {}};
            }
        }
        Sva_State_راس* ثابت یوځای_Vtxp = scoped_Create_راس();
        إذا (booleanOnly) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsp,
                        "Single-cycle SOr operands must have finalCondp");
            guardedLink(ثبت_Vtxp, یوځای_Vtxp,
                        جديد AstLogOr{flp, sampled(lhs.finalCondp->clone_ونه_خالص(خطأ)),
                                     sampled(rhs.finalCondp->clone_ونه_خالص(خطأ))},
                        flp);
        } وإلا {
            إذا (lhs.finalCondp) {
                guardedLink(lhs.غړی_Vertexp, یوځای_Vtxp,
                            sampled(lhs.finalCondp->clone_ونه_خالص(خطأ)), flp);
            } وإلا {
                guardedLink(lhs.غړی_Vertexp, یوځای_Vtxp, flp);
            }
            إذا (rhs.finalCondp) {
                guardedLink(rhs.غړی_Vertexp, یوځای_Vtxp,
                            sampled(rhs.finalCondp->clone_ونه_خالص(خطأ)), flp);
            } وإلا {
                guardedLink(rhs.غړی_Vertexp, یوځای_Vtxp, flp);
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        أرجع {یوځای_Vtxp, تش_اشاره, {}};
    }

    // Build done-latch combiner for SAnd/SIntersect (IEEE 1800-2023 16.9.5).
    Build_پایله buildAndCombiner(Ast_غوټه_بیان* lhsExprp, Ast_غوټه_بیان* rhsExprp,
                                 Sva_State_راس* ثبت_Vtxp, File_کرښه* flp) {
        // Snapshot-restore scope so LHS liveness does not leak into RHS.
        ثابت منطقي savedScope = m_دننه_Unbounded_Scope;
        ثابت Build_پایله lhs = build_بیان(lhsExprp, ثبت_Vtxp);
        ثابت منطقي lhsScope = m_دننه_Unbounded_Scope;
        m_دننه_Unbounded_Scope = savedScope;
        ثابت Build_پایله rhs = build_بیان(rhsExprp, ثبت_Vtxp);
        ثابت منطقي rhsScope = m_دننه_Unbounded_Scope;
        m_دننه_Unbounded_Scope = savedScope || lhsScope || rhsScope;
        إذا (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع Build_پایله::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP

        // Single-cycle operands: use boolean AND (done-latch would fire across cycles).
        // If both operands stayed at entry, they must be boolean leaves which
        // buildExpr returns with finalCondp=nodep (non-null).
        إذا (lhs.غړی_Vertexp == ثبت_Vtxp && rhs.غړی_Vertexp == ثبت_Vtxp) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsExprp,
                        "Single-cycle SAnd operands must have finalCondp");
            Ast_غوټه_بیان* ثابت condp = جديد AstLogAnd{flp, lhs.finalCondp->clone_ونه_خالص(خطأ),
                                                     rhs.finalCondp->clone_ونه_خالص(خطأ)};
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع {ثبت_Vtxp, condp, {}};
        }
        // Range-delay mid-window sources in either sub-branch would need
        // to be folded into the latch's match-now signal, which the
        // current combiner does not support.
        إذا (!lhs.midSources.تشه() || !rhs.midSources.تشه()) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: ranged cycle delay in an operand of property 'and'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع Build_پایله::failWithError();
        }
        Sva_State_راس* ثابت combVtxp = scoped_Create_راس();
        combVtxp->m_isAndCombiner = ریښتیا;
        combVtxp->m_andLhsTermp = lhs.غړی_Vertexp;
        combVtxp->m_andRhsTermp = rhs.غړی_Vertexp;
        إذا (lhs.finalCondp) combVtxp->m_andLhsCondp = lhs.finalCondp->clone_ونه_خالص(خطأ);
        إذا (rhs.finalCondp) combVtxp->m_andRhsCondp = rhs.finalCondp->clone_ونه_خالص(خطأ);
        إذا (lhs.غړی_Vertexp->m_isUnbounded || rhs.غړی_Vertexp->m_isUnbounded) {
            combVtxp->m_isUnbounded = ریښتیا;
        }
        // Wire terminal-boolean rejects to a dedicated sink so each side can fail
        // the AND independently. Skip for liveness or single-cycle operands
        // (single-cycle termVertexp == entryVtxp would fire every cycle).
        إذا (!combVtxp->m_isUnbounded) {
            منطقي needSink = خطأ;
            ثابت منطقي lhs_Multi_دوره = (lhs.غړی_Vertexp != ثبت_Vtxp);
            ثابت منطقي rhs_Multi_دوره = (rhs.غړی_Vertexp != ثبت_Vtxp);
            إذا (lhs.finalCondp && lhs_Multi_دوره && !lhs.غړی_Vertexp->m_isUnbounded) {
                needSink = ریښتیا;
            }
            إذا (rhs.finalCondp && rhs_Multi_دوره && !rhs.غړی_Vertexp->m_isUnbounded) {
                needSink = ریښتیا;
            }
            إذا (needSink) {
                Sva_State_راس* ثابت sinkVtxp = m_ګراف.create_State_راس();
                sinkVtxp->m_isRejectSink = ریښتیا;
                إذا (lhs.finalCondp && lhs_Multi_دوره && !lhs.غړی_Vertexp->m_isUnbounded) {
                    Sva_Trans_څنډه* ثابت ep = m_ګراف.جمع_کړه_Link(
                        lhs.غړی_Vertexp, sinkVtxp, sampled(lhs.finalCondp->clone_ونه_خالص(خطأ)));
                    ep->m_rejectOnFail = ریښتیا;
                }
                إذا (rhs.finalCondp && rhs_Multi_دوره && !rhs.غړی_Vertexp->m_isUnbounded) {
                    Sva_Trans_څنډه* ثابت ep = m_ګراف.جمع_کړه_Link(
                        rhs.غړی_Vertexp, sinkVtxp, sampled(rhs.finalCondp->clone_ونه_خالص(خطأ)));
                    ep->m_rejectOnFail = ریښتیا;
                }
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        أرجع {combVtxp, تش_اشاره, {}};
    }

    // Lower `seq1 within seq2` (IEEE 1800-2023 16.9.10) as:
    //   (OR_{i in 0..slack} 1 ##i seq1 ##(slack-i) 1) intersect seq2
    // Both operands must have fixed length (no ranged cycle delays).
    // The OR lives inside a single SIntersect so one AndCombiner done-latch
    // serves all offsets; lifting it out would double-count matches where
    // multiple offsets accept on the same seq2 end cycle.
    Build_پایله buildSWithin(AstSWithin* nodep, Sva_State_راس* ثبت_Vtxp,
                             منطقي is_سر_Level_ګام = خطأ) {
        ثابت صحيح innerLen = fixed_اوږدتیا(nodep->lhsp());
        ثابت صحيح outerLen = fixed_اوږدتیا(nodep->rhsp());
        إذا (innerLen < 0 || outerLen < 0) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: within with ranged cycle-delay operand");
            أرجع Build_پایله::failWithError();
        }
        إذا (innerLen > outerLen) {
            أرجع build_Never_سمون_Intersect(
                nodep, ثبت_Vtxp, is_سر_Level_ګام,
                "the inner sequence is longer than the outer sequence");
        }
        File_کرښه* ثابت flp = nodep->fileline();
        ثابت صحيح slack = outerLen - innerLen;
        Ast_غوټه_بیان* innerOrp = تش_اشاره;
        لكل (صحيح i = 0; i <= slack; ++i) {
            ثابت صحيح postPad = slack - i;
            Ast_غوټه_بیان* branchp = nodep->lhsp()->clone_ونه_خالص(خطأ);
            إذا (i > 0) {
                AstConst* ثابت prePadp = جديد AstConst{flp, AstConst::بټ_True{}};
                AstDelay* ثابت delayp
                    = جديد AstDelay{flp, جديد AstConst{flp, static_cast<uint32_t>(i)}, ریښتیا};
                AstSExpr* ثابت wrapped = جديد AstSExpr{flp, prePadp, delayp, branchp};
                wrapped->dtype_سېټ_بټ();
                branchp = wrapped;
            }
            إذا (postPad > 0) {
                AstConst* ثابت postTruep = جديد AstConst{flp, AstConst::بټ_True{}};
                AstDelay* ثابت delayp
                    = جديد AstDelay{flp, جديد AstConst{flp, static_cast<uint32_t>(postPad)}, ریښتیا};
                AstSExpr* ثابت wrapped = جديد AstSExpr{flp, branchp, delayp, postTruep};
                wrapped->dtype_سېټ_بټ();
                branchp = wrapped;
            }
            innerOrp = innerOrp ? static_cast<Ast_غوټه_بیان*>(جديد AstSOr{flp, innerOrp, branchp})
                                : branchp;
        }
        Ast_غوټه_بیان* ثابت outerClonep = nodep->rhsp()->clone_ونه_خالص(خطأ);
        Ast_غوټه_بیان* ثابت combinedp = جديد AstSIntersect{flp, innerOrp, outerClonep};
        Build_پایله پایله = build_بیان(combinedp, ثبت_Vtxp, is_سر_Level_ګام);
        VL_DO_DANGLING(combinedp->delete_ونه(), combinedp);
        // When both operands are plain booleans, buildAndCombiner returns a
        // freshly-allocated AstAnd as finalCondp with no parent. Callers up
        // the chain clone-and-discard finalCondp, so leave it parent-less and
        // it leaks; anchor it in the graph now via an edge.
        إذا (پایله.valid() && پایله.finalCondp && !پایله.finalCondp->backp()) {
            Sva_State_راس* ثابت wrapVtxp = scoped_Create_راس();
            guardedLink(پایله.غړی_Vertexp, wrapVtxp, sampled(پایله.finalCondp), flp);
            پایله = {wrapVtxp, تش_اشاره, پایله.midSources};
        }
        أرجع پایله;
    }

    // Collect the boolean leaf checks of a fixed-length sequence keyed by their
    // clock offset from the start. Returns false for anything other than nested
    // AstSExpr with fixed cycle delays over boolean leaves (e.g. throughout).
    ساكن منطقي flattenFixedSeq(Ast_غوټه_بیان* nodep, صحيح بنسټ_دننه_واټن,
                                std::map<صحيح, std::ویکټور<Ast_غوټه_بیان*>>& بهر) {
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
            إذا (!delayp || !delayp->is_دوره_Delay() || delayp->isUnbounded()) أرجع خطأ;
            ثابت بینښه delay_دورې = getConstUInt(delayp->lhsp());
            إذا (delayp->isRangeDelay() && getConstUInt(delayp->rhsp()) != delay_دورې)
                أرجع خطأ;
            صحيح preLen = 0;
            إذا (Ast_غوټه_بیان* ثابت prep = sexprp->preExprp()) {
                إذا (!flattenFixedSeq(prep, بنسټ_دننه_واټن, بهر)) أرجع خطأ;
                preLen = fixed_اوږدتیا(prep);
                إذا (preLen < 0) أرجع خطأ;
            }
            أرجع flattenFixedSeq(sexprp->exprp(), بنسټ_دننه_واټن + preLen + delay_دورې, بهر);
        }
        إذا (nodep->is_Multi_دوره_Sva()) أرجع خطأ;
        بهر[بنسټ_دننه_واټن].ټېل_شا(nodep);
        أرجع ریښتیا;
    }

    // Conjoin two equal-length fixed sequences into one: at each clock offset
    // AND the boolean checks of both operands (IEEE 1800-2023 16.9.6 -- both
    // operands match the same window). Returns null if either operand is not a
    // plain fixed sequence of boolean leaves.
    ساكن Ast_غوټه_بیان* conjoinFixedSeqs(Ast_غوټه_بیان* lhsp, Ast_غوټه_بیان* rhsp, File_کرښه* flp) {
        std::map<صحيح, std::ویکټور<Ast_غوټه_بیان*>> checks;
        إذا (!flattenFixedSeq(lhsp, 0, checks) || !flattenFixedSeq(rhsp, 0, checks)) أرجع تش_اشاره;
        إذا (checks.تشه()) أرجع تش_اشاره;
        Ast_غوټه_بیان* resultp = تش_اشاره;
        صحيح prev_دننه_واټن = 0;
        لكل (ثابت خپلکاری& دننه_واټن_Checks : checks) {
            ثابت صحيح دننه_واټن = دننه_واټن_Checks.لومړی;
            Ast_غوټه_بیان* condp = تش_اشاره;
            لكل (Ast_غوټه_بیان* ثابت leafp : دننه_واټن_Checks.دوهم) {
                Ast_غوټه_بیان* ثابت clonep = leafp->clone_ونه_خالص(خطأ);
                إذا (!condp) {
                    condp = clonep;
                } وإلا {
                    condp = جديد AstLogAnd{flp, condp, clonep};
                    condp->dtype_سېټ_بټ();
                }
            }
            إذا (!resultp) {
                إذا (دننه_واټن > 0) {
                    AstDelay* ثابت delayp = جديد AstDelay{
                        flp, جديد AstConst{flp, static_cast<uint32_t>(دننه_واټن)}, /*isCycle=*/ریښتیا};
                    resultp
                        = جديد AstSExpr{flp, جديد AstConst{flp, AstConst::بټ_True{}}, delayp, condp};
                    resultp->dtype_سېټ_بټ();
                } وإلا {
                    resultp = condp;
                }
            } وإلا {
                AstDelay* ثابت delayp = جديد AstDelay{
                    flp, جديد AstConst{flp, static_cast<uint32_t>(دننه_واټن - prev_دننه_واټن)},
                    /*isCycle=*/ریښتیا};
                resultp = جديد AstSExpr{flp, resultp, delayp, condp};
                resultp->dtype_سېټ_بټ();
            }
            prev_دننه_واټن = دننه_واټن;
        }
        أرجع resultp;
    }

    // `seq` is a simple ranged sequence `start ##[m:n] end` (start/end boolean,
    // start may be absent). Used to collapse a both-variable intersect to one
    // ranged delay.
    بنية SimpleRanged final {
        منطقي ok = خطأ;
        Ast_غوټه_بیان* startp = تش_اشاره;  // may be null (absent start)
        Ast_غوټه_بیان* endp = تش_اشاره;
    };
    ساكن SimpleRanged asSimpleRanged(Ast_غوټه_بیان* nodep) {
        AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr);
        إذا (!sexprp) أرجع {};
        AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
        إذا (!delayp || !delayp->is_دوره_Delay() || !delayp->isRangeDelay() || delayp->isUnbounded())
            أرجع {};
        إذا (getConstUInt(delayp->lhsp()) == getConstUInt(delayp->rhsp())) أرجع {};
        Ast_غوټه_بیان* ثابت prep = sexprp->preExprp();
        إذا (prep && fixed_اوږدتیا(prep) != 0) أرجع {};
        إذا (fixed_اوږدتیا(sexprp->exprp()) != 0) أرجع {};
        أرجع {ریښتیا, prep, sexprp->exprp()};
    }

    // Build the NFA for a synthesized intersect lowering tree, then free it.
    // buildExpr returns the terminal condition (finalCondp) by reference into the
    // tree; detach a clone so the tree can be freed here. The graph already holds
    // clones/hoists of every edge condition, so nothing else dangles.
    Build_پایله build_From_Lowering_ونه(Ast_غوټه_بیان* treep, Sva_State_راس* ثبت_Vtxp,
                                      منطقي is_سر_Level_ګام) {
        Build_پایله پایله = build_بیان(treep, ثبت_Vtxp, is_سر_Level_ګام);
        إذا (پایله.valid() && پایله.finalCondp) {
            پایله.finalCondp = پایله.finalCondp->clone_ونه_خالص(خطأ);
        }
        VL_DO_DANGLING(treep->delete_ونه(), treep);
        أرجع پایله;
    }

    // Empty common-length intersection -- unequal fixed lengths, or disjoint
    // ranged lengths. IEEE 1800-2023 16.9.6 requires both operands to match
    // over a window of the same length, so with no common length the intersect
    // simply never matches. This is legal (matching nothing), not an error, so
    // lower to a constant false rather than rejecting legal code.
    Build_پایله build_Never_سمون_Intersect(Ast_غوټه_بیان* nodep, Sva_State_راس* ثبت_Vtxp,
                                         منطقي is_سر_Level_ګام, ثابت حرف* reason) {
        nodep->v3warn(NEVERMATCH, "Sequence can never match because " << reason << ".");
        Ast_غوټه_بیان* ثابت falsep = جديد AstConst{nodep->fileline(), AstConst::بټ_False{}};
        أرجع build_From_Lowering_ونه(falsep, ثبت_Vtxp, is_سر_Level_ګام);
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
    Build_پایله buildVarLenIntersect(AstSIntersect* nodep, Sva_State_راس* ثبت_Vtxp,
                                     منطقي is_سر_Level_ګام) {
        ثابت std::pair<صحيح, صحيح> lhsRange = اوږدتیا_Range(nodep->lhsp());
        ثابت std::pair<صحيح, صحيح> rhsRange = اوږدتیا_Range(nodep->rhsp());
        إذا (lhsRange.لومړی < 0 || rhsRange.لومړی < 0) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect with this variable-length operand");
            أرجع Build_پایله::failWithError();
        }
        ثابت صحيح lo = std::max(lhsRange.لومړی, rhsRange.لومړی);
        ثابت صحيح hi = std::min(lhsRange.دوهم, rhsRange.دوهم);
        إذا (lo > hi) {
            // Disjoint length ranges share no common length -> never matches.
            أرجع build_Never_سمون_Intersect(nodep, ثبت_Vtxp, is_سر_Level_ګام,
                                            "intersect operands have no common length");
        }
        File_کرښه* ثابت flp = nodep->fileline();
        إذا (lo == hi) {
            Ast_غوټه_بیان* ثابت lp = realize_At_اوږدتیا(nodep->lhsp(), lo, lhsRange.لومړی);
            Ast_غوټه_بیان* ثابت rp = realize_At_اوږدتیا(nodep->rhsp(), lo, rhsRange.لومړی);
            Ast_غوټه_بیان* ثابت conjp = conjoinFixedSeqs(lp, rp, flp);
            VL_DO_DANGLING(lp->delete_ونه(), lp);
            VL_DO_DANGLING(rp->delete_ونه(), rp);
            إذا (!conjp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: intersect operand is not a plain boolean sequence");
                أرجع Build_پایله::failWithError();
            }
            أرجع build_From_Lowering_ونه(conjp, ثبت_Vtxp, is_سر_Level_ګام);
        }
        ثابت SimpleRanged sl = asSimpleRanged(nodep->lhsp());
        ثابت SimpleRanged sr = asSimpleRanged(nodep->rhsp());
        إذا (!sl.ok || !sr.ok) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect of two sequences that each vary in length over a"
                          " range with internal structure");
            أرجع Build_پایله::failWithError();
        }
        ثابت خپلکاری andBool = [&](Ast_غوټه_بیان* ap, Ast_غوټه_بیان* bp) -> Ast_غوټه_بیان* {
            Ast_غوټه_بیان* ثابت aClonep
                = ap ? ap->clone_ونه_خالص(خطأ) : جديد AstConst{flp, AstConst::بټ_True{}};
            Ast_غوټه_بیان* ثابت bClonep
                = bp ? bp->clone_ونه_خالص(خطأ) : جديد AstConst{flp, AstConst::بټ_True{}};
            AstLogAnd* ثابت andp = جديد AstLogAnd{flp, aClonep, bClonep};
            andp->dtype_سېټ_بټ();
            أرجع andp;
        };
        AstDelay* ثابت delayp = جديد AstDelay{flp, جديد AstConst{flp, static_cast<uint32_t>(lo)},
                                              /*isCycle=*/ریښتیا};
        delayp->rhsp(جديد AstConst{flp, static_cast<uint32_t>(hi)});
        AstSExpr* ثابت reducedp
            = جديد AstSExpr{flp, andBool(sl.startp, sr.startp), delayp, andBool(sl.endp, sr.endp)};
        reducedp->dtype_سېټ_بټ();
        أرجع build_From_Lowering_ونه(reducedp, ثبت_Vtxp, is_سر_Level_ګام);
    }

    Build_پایله buildThroughout(AstSThroughout* nodep, Sva_State_راس* ثبت_Vtxp,
                                منطقي is_سر_Level_ګام = خطأ) {
        // Mark entryVtxp so "cond false at tick 0" is detected as throughout-drop.
        ثبت_Vtxp->m_throughoutConds.ټېل_شا(nodep->lhsp()->clone_ونه_خالص(خطأ));
        m_temporal_Guard_ډېری.ټېل_شا(nodep->lhsp());
        ثابت Build_پایله پایله = build_بیان(nodep->rhsp(), ثبت_Vtxp, is_سر_Level_ګام);
        m_temporal_Guard_ډېری.راوباسه_شا();
        أرجع پایله;
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
    Build_پایله buildUntil(AstUntil* nodep, Sva_State_راس* ثبت_Vtxp, منطقي is_سر_Level_ګام) {
        File_کرښه* ثابت flp = nodep->fileline();
        إذا (!is_سر_Level_ګام) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            أرجع Build_پایله::failWithError();
        }
        Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp();
        Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp();
        ثابت خپلکاری hasSeq = [](ثابت Ast_غوټه_بیان* ep) {
            أرجع ep->exists([](ثابت Ast_غوټه_بیان* np) { أرجع np->is_Multi_دوره_Sva(); });
        };
        إذا (hasSeq(lhsp) || hasSeq(rhsp)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            أرجع Build_پایله::failWithError();
        }

        ثابت منطقي ov = nodep->isOverlapping();
        Ast_غوټه_بیان* ثابت lhsBitp = nodep->lhsp();
        Ast_غوټه_بیان* ثابت rhsBitp = nodep->rhsp();
        // p hoist count: continue, require (ov: 1 use; nov: 1 use). At least 2 uses.
        AstVar* ثابت pHoistp = tryHoistSampled(lhsBitp, flp, 2);
        // q hoist count: continue (1) + require nov (1) = 2; ov: continue only (1).
        AstVar* ثابت qHoistp = ov ? تش_اشاره : tryHoistSampled(rhsBitp, flp, 2);

        Sva_State_راس* ثابت waitCp = scoped_Create_راس();
        Sva_State_راس* ثابت waitRp = scoped_Create_راس();
        waitCp->m_isUnbounded = ریښتیا;
        waitRp->m_strongPending = nodep->isStrong() && !m_isCover;

        // Entry and back-loop Links carry no condition; throughout-folding still applies.
        guardedLink(ثبت_Vtxp, waitCp, flp);
        guardedLink(waitRp, waitCp, flp);

        // Continue clocked edge: p && !q advances to next-cycle wait.
        Ast_غوټه_بیان* ثابت contCondp
            = جديد AstLogAnd{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                            جديد AstLogNot{flp, sampledRefOrClone(qHoistp, rhsBitp, flp)}};
        guarded_څنډه(waitCp, waitRp, contCondp, flp);

        // Reject sink: fires when require-condition is false.
        Sva_State_راس* ثابت sinkVtxp = m_ګراف.create_State_راس();
        sinkVtxp->m_isRejectSink = ریښتیا;
        Ast_غوټه_بیان* requireCondp;
        إذا (ov) {
            requireCondp = sampledRefOrClone(pHoistp, lhsBitp, flp);
        } وإلا {
            requireCondp = جديد AstLogOr{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                                        sampledRefOrClone(qHoistp, rhsBitp, flp)};
        }
        Sva_Trans_څنډه* ثابت rejEdgep = m_ګراف.جمع_کړه_Link(waitCp, sinkVtxp, requireCondp);
        rejEdgep->m_rejectOnFail = ریښتیا;

        // Accept condition rides via finalCondp; assembleResult $sampled-wraps it.
        Ast_غوټه_بیان* acceptCondp;
        إذا (ov) {
            acceptCondp
                = جديد AstLogAnd{flp, lhsBitp->clone_ونه_خالص(خطأ), rhsBitp->clone_ونه_خالص(خطأ)};
        } وإلا {
            acceptCondp = rhsBitp->clone_ونه_خالص(خطأ);
        }
        أرجع {waitCp, acceptCondp, {}};
    }

    // IEEE 1800-2023 16.12.14 property abort operators. Sync and async share
    // the same NFA encoding: AstSampled already gives matured values at every
    // maturing clocking event, and async firing "between clocks" is not
    // observable in a cycle-based model. VAbortKind selects accept vs reject
    // verdict (sync vs async only changes the user-visible spelling).

    // Build `condp && !outer_1 && !outer_2 ...` (unsampled).
    Ast_غوټه_بیان* abort_Fire_بیان(Ast_غوټه_بیان* condp, File_کرښه* flp) {
        Ast_غوټه_بیان* resultp = condp->clone_ونه_خالص(خطأ);
        لكل (Ast_غوټه_بیان* ثابت op : m_outer_Abort_ډېری)
            resultp = جديد AstLogAnd{flp, resultp, جديد AstLogNot{flp, op->clone_ونه_خالص(خطأ)}};
        أرجع resultp;
    }

    // True when unguarded ring-wide rejection or a same-tick Link chain already accounts the
    // attempt: a required-step Link covers both outcomes; followed-by pairs both edges.
    ساكن منطقي chain_Accounts_سرچینه(ثابت Sva_State_راس* srcp,
                                    ثابت std::unordered_سېټ<ثابت V3_ګراف_څنډه*>& pre_څنډې) {
        إذا (srcp->m_delay_Ring_اندازه && srcp->m_throughoutConds.تشه()) أرجع ریښتیا;
        لكل (ثابت V3_ګراف_څنډه& edger : srcp->دننه_څنډې()) {
            إذا (pre_څنډې.شمېر(&edger)) استمر;
            ثابت Sva_Trans_څنډه& tedger = static_cast<ثابت Sva_Trans_څنډه&>(edger);
            إذا (tedger.m_consumes_دوره) استمر;
            ثابت خپلکاری* ثابت fromp = static_cast<ثابت Sva_State_راس*>(tedger.fromVtxp());
            إذا (fromp->m_abortRejectp) أرجع ریښتیا;
        }
        منطقي plainNonSink = خطأ;
        منطقي markedSink = خطأ;
        لكل (ثابت V3_ګراف_څنډه& edger : srcp->بهر_څنډې()) {
            إذا (pre_څنډې.شمېر(&edger)) استمر;
            ثابت Sva_Trans_څنډه& tedger = static_cast<ثابت Sva_Trans_څنډه&>(edger);
            إذا (tedger.m_consumes_دوره) استمر;
            ثابت منطقي sink = static_cast<ثابت Sva_State_راس*>(tedger.منزل_Vtxp())->m_isRejectSink;
            إذا (tedger.m_rejectOnFail) {
                إذا (!sink) أرجع ریښتیا;
                markedSink = ریښتیا;
            } وإلا إذا (!sink) {
                إذا (srcp->m_abortRejectp) أرجع ریښتیا;
                plainNonSink = ریښتیا;
            }
        }
        أرجع plainNonSink && markedSink;
    }

    // Reject edge: fires when the source is live and the abort samples true.
    فراغ جمع_کړه_Abort_Reject_څنډه(Sva_State_راس* srcp, Sva_State_راس* sinkp, Ast_غوټه_بیان* condp,
                            File_کرښه* flp) {
        Ast_غوټه_بیان* ثابت notFirep = جديد AstLogNot{flp, sampled(abort_Fire_بیان(condp, flp))};
        m_ګراف.جمع_کړه_Link(srcp, sinkp, notFirep)->m_rejectOnFail = ریښتیا;
        أرجع;
    }

    // On the fire tick: kill body threads; accept kinds also forgive step misses.
    فراغ gate_Body_څنډې_On_Abort(ثابت std::unordered_سېټ<ثابت V3_ګراف_څنډه*>& pre_څنډې,
                              Ast_غوټه_بیان* condp, VAbort_ډول ډول, File_کرښه* flp) {
        لكل (V3_ګراف_راس& vtxr : m_ګراف.m_ګراف.vertices()) {
            لكل (V3_ګراف_څنډه& edger : vtxr.بهر_څنډې()) {
                إذا (pre_څنډې.شمېر(&edger)) استمر;
                Sva_Trans_څنډه* ثابت tedgep = static_cast<Sva_Trans_څنډه*>(&edger);
                إذا (tedgep->m_rejectOnFail) {
                    إذا (!ډول.isAccept()) استمر;
                    Ast_غوټه_بیان* ثابت firep = sampled(abort_Fire_بیان(condp, flp));
                    tedgep->m_condp
                        = tedgep->m_condp ? جديد AstLogOr{flp, tedgep->m_condp, firep} : firep;
                } وإلا إذا (tedgep->m_consumes_دوره) {
                    Ast_غوټه_بیان* ثابت notFirep
                        = جديد AstLogNot{flp, sampled(abort_Fire_بیان(condp, flp))};
                    tedgep->m_condp = tedgep->m_condp
                                          ? جديد AstLogAnd{flp, tedgep->m_condp, notFirep}
                                          : notFirep;
                }
            }
        }
    }

    Build_پایله buildAbortOn(Ast_غوټه_بیان* condp, Ast_غوټه_بیان* bodyp, Sva_State_راس* ثبت_Vtxp,
                             VAbort_ډول ډول, File_کرښه* flp, منطقي is_سر_Level_ګام) {
        // Snapshot pre-body vertices/edges so post-build diff yields the body's sub-NFA.
        std::unordered_سېټ<ثابت V3_ګراف_راس*> preExisting;
        std::unordered_سېټ<ثابت V3_ګراف_څنډه*> pre_څنډې;
        لكل (V3_ګراف_راس& vtxr : m_ګراف.m_ګراف.vertices()) {
            preExisting.ورننباسه(&vtxr);
            لكل (V3_ګراف_څنډه& edger : vtxr.بهر_څنډې()) pre_څنډې.ورننباسه(&edger);
        }

        m_outer_Abort_ډېری.ټېل_شا(condp);
        ثابت Build_پایله body_پایله = build_بیان(bodyp, ثبت_Vtxp, is_سر_Level_ګام);
        m_outer_Abort_ډېری.راوباسه_شا();
        إذا (!body_پایله.valid()) أرجع body_پایله;

        gate_Body_څنډې_On_Abort(pre_څنډې, condp, ډول, flp);

        // Live-thread sources for the abort edge: entry + new body vertices,
        // minus reject sinks (they carry reject fuel, not live-thread fuel).
        std::ویکټور<Sva_State_راس*> abortSources;
        abortSources.ټېل_شا(ثبت_Vtxp);
        لكل (V3_ګراف_راس& vtxr : m_ګراف.m_ګراف.vertices()) {
            إذا (preExisting.شمېر(&vtxr)) استمر;
            خپلکاری* ثابت sp = static_cast<Sva_State_راس*>(&vtxr);
            إذا (sp->m_delay_Ring_اندازه || !sp->m_throughoutConds.تشه()) {
                Ast_غوټه_بیان* ثابت firep = abort_Fire_بیان(condp, flp);
                sp->m_abortClearp
                    = sp->m_abortClearp ? جديد AstLogOr{flp, sp->m_abortClearp, firep} : firep;
            }
            إذا (!ډول.isAccept()
                && ((sp->m_delay_Ring_اندازه && sp->m_throughoutConds.تشه())
                    || sp->m_replayAbortReject)) {
                Ast_غوټه_بیان* ثابت firep = abort_Fire_بیان(condp, flp);
                sp->m_abortRejectp
                    = sp->m_abortRejectp ? جديد AstLogOr{flp, sp->m_abortRejectp, firep} : firep;
            }
            إذا (sp->m_isRejectSink) استمر;
            abortSources.ټېل_شا(sp);
        }

        خپلکاری sampledAbortFire = [&]() -> Ast_غوټه_بیان* {
            Ast_غوټه_بیان* ثابت بیان = abort_Fire_بیان(condp, flp);
            أرجع sampled(بیان);
        };

        إذا (ډول.isAccept()) {
            // Match-only sink fed by $sampled(abort-fire) from every live source;
            // registered as midSource so it never contributes a reject.
            Sva_State_راس* ثابت acceptSinkp = scoped_Create_راس();
            لكل (Sva_State_راس* ثابت srcp : abortSources)
                guardedLink(srcp, acceptSinkp, sampledAbortFire(), flp);
            std::ویکټور<Sva_State_راس*> midSources = body_پایله.midSources;
            midSources.ټېل_شا(acceptSinkp);
            Ast_غوټه_بیان* finalCondp = body_پایله.finalCondp;
            إذا (finalCondp) {
                إذا (finalCondp->backp()) finalCondp = finalCondp->clone_ونه_خالص(خطأ);
                finalCondp = جديد AstLogOr{flp, finalCondp, abort_Fire_بیان(condp, flp)};
            }
            أرجع {body_پایله.غړی_Vertexp, finalCondp, std::move(midSources)};
        }

        // rejectOnFail treats m_condp as the success condition and fires on
        // !condp, so the edge carries !sampledAbortFire().
        Sva_State_راس* ثابت rejectSinkp = m_ګراف.create_State_راس();
        rejectSinkp->m_isRejectSink = ریښتیا;
        لكل (Sva_State_راس* ثابت srcp : abortSources)
            إذا (!chain_Accounts_سرچینه(srcp, pre_څنډې))
                جمع_کړه_Abort_Reject_څنډه(srcp, rejectSinkp, condp, flp);
        Ast_غوټه_بیان* finalCondp = body_پایله.finalCondp;
        إذا (finalCondp) {
            إذا (finalCondp->backp()) finalCondp = finalCondp->clone_ونه_خالص(خطأ);
            finalCondp
                = جديد AstLogAnd{flp, finalCondp, جديد AstLogNot{flp, abort_Fire_بیان(condp, flp)}};
        }
        أرجع {body_پایله.غړی_Vertexp, finalCondp, body_پایله.midSources};
    }

عام:
    // Free a dropped sub-result condition that is not linked into the AST
    // (abort folds synthesize unparented finalCondp trees).
    ساكن فراغ freeUnlinkedCondp(Ast_غوټه_بیان* condp) {
        إذا (condp && !condp->backp()) VL_DO_DANGLING(condp->delete_ونه(), condp);
    }

    SvaNfaBuilder(Sva_ګراف& ګراف, Ast_غوټه_Module* modp, V3_Unique_نومونه& prop_لنډمهاله_نومونه,
                  منطقي isCoverSeq = خطأ, منطقي isSeqEvent = خطأ, منطقي isCover = خطأ)
        : m_ګراف{ګراف}
        , m_modp{modp}
        , m_prop_لنډمهاله_نومونه{prop_لنډمهاله_نومونه}
        , m_isCover{isCover}
        , m_isCoverSeq{isCoverSeq}
        , m_isSeqEvent{isSeqEvent} {}

    // Reset scope between antecedent and consequent: liveness must not leak.
    // m_outerAbortStack survives: an abort wrapping the implication covers the
    // consequent too (IEEE 1800-2023 16.12.14).
    فراغ resetScope() {
        m_دننه_Unbounded_Scope = خطأ;
        m_temporal_Guard_ډېری.clear();
    }

    Build_پایله build_بیان(Ast_غوټه_بیان* nodep, Sva_State_راس* ثبت_Vtxp,
                          منطقي is_سر_Level_ګام = خطأ) {
        VL_RESTORER(m_want_Bool_ونې_پاڼه);
        ثابت منطقي want_Bool_ونې_پاڼه = m_want_Bool_ونې_پاڼه;
        m_want_Bool_ونې_پاڼه = خطأ;
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            أرجع buildSExpr(sexprp, ثبت_Vtxp, is_سر_Level_ګام);
        }
        إذا (AstSConsRep* ثابت repp = VN_CAST(nodep, SConsRep)) {
            أرجع buildConsRep(repp, ثبت_Vtxp, is_سر_Level_ګام);
        }
        إذا (AstPropAlways* ثابت alwaysp = VN_CAST(nodep, PropAlways)) {
            أرجع buildPropAlways(alwaysp, ثبت_Vtxp, is_سر_Level_ګام);
        }
        إذا (AstSGotoRep* ثابت repp = VN_CAST(nodep, SGotoRep)) {
            أرجع buildGotoRep(repp, ثبت_Vtxp);
        }
        إذا (AstSThroughout* ثابت throughoutp = VN_CAST(nodep, SThroughout)) {
            أرجع buildThroughout(throughoutp, ثبت_Vtxp, is_سر_Level_ګام);
        }
        إذا (AstSOr* ثابت orp = VN_CAST(nodep, SOr)) {
            أرجع build_Or_یوځای(orp->lhsp(), orp->rhsp(), ثبت_Vtxp, orp->fileline(),
                                want_Bool_ونې_پاڼه);
        }
        إذا (AstLogOr* ثابت orp = VN_CAST(nodep, LogOr)) {
            أرجع build_Or_یوځای(orp->lhsp(), orp->rhsp(), ثبت_Vtxp, orp->fileline(),
                                want_Bool_ونې_پاڼه);
        }
        إذا (AstSAnd* ثابت andp = VN_CAST(nodep, SAnd)) {
            أرجع buildAndCombiner(andp->lhsp(), andp->rhsp(), ثبت_Vtxp, andp->fileline());
        }
        إذا (AstSIntersect* ثابت intp = VN_CAST(nodep, SIntersect)) {
            // IEEE 1800-2023 16.9.6: both operands match over one window with
            // equal start and end (equal length). Lower to a single sequence
            // that conjoins both operands' per-cycle checks -- correct under
            // concurrent attempts, where the done-latch combiner conflates the
            // two operands' start times and over-accepts. The combiner remains
            // only as a fallback for operands that do not flatten.
            ثابت صحيح lhsLen = fixed_اوږدتیا(intp->lhsp());
            ثابت صحيح rhsLen = fixed_اوږدتیا(intp->rhsp());
            إذا (lhsLen >= 0 && rhsLen >= 0) {
                إذا (lhsLen != rhsLen) {
                    // Unequal fixed lengths share no common length -> never matches.
                    أرجع build_Never_سمون_Intersect(intp, ثبت_Vtxp, is_سر_Level_ګام,
                                                    "intersect operands have no common length");
                }
                إذا (Ast_غوټه_بیان* ثابت conjp
                    = conjoinFixedSeqs(intp->lhsp(), intp->rhsp(), intp->fileline())) {
                    أرجع build_From_Lowering_ونه(conjp, ثبت_Vtxp, is_سر_Level_ګام);
                }
                أرجع buildAndCombiner(intp->lhsp(), intp->rhsp(), ثبت_Vtxp, intp->fileline());
            }
            أرجع buildVarLenIntersect(intp, ثبت_Vtxp, is_سر_Level_ګام);
        }
        إذا (AstSWithin* ثابت withinp = VN_CAST(nodep, SWithin)) {
            أرجع buildSWithin(withinp, ثبت_Vtxp, is_سر_Level_ګام);
        }
        إذا (AstAbortOn* ثابت ap = VN_CAST(nodep, AbortOn)) {
            أرجع buildAbortOn(ap->condp(), ap->propp(), ثبت_Vtxp, ap->ډول(), ap->fileline(),
                                is_سر_Level_ګام);
        }
        إذا (VN_IS(nodep, SNonConsRep)) أرجع Build_پایله::fail();
        إذا (AstImplication* ثابت implp = VN_CAST(nodep, Implication)) {
            أرجع build_Implication_څنډې(implp->lhsp(), implp->rhsp(), ثبت_Vtxp,
                                         implp->isOverlapped(), implp->isFollowedBy(),
                                         implp->lhsp(), implp->fileline());
        }
        إذا (AstUntil* ثابت untilp = VN_CAST(nodep, Until)) {
            أرجع buildUntil(untilp, ثبت_Vtxp, is_سر_Level_ګام);
        }
        // Boolean leaf (including LogAnd): return as finalCond
        أرجع {ثبت_Vtxp, nodep, {}};
    }

    // Wire an implication / followed-by from `entryVtxp`: builds the antecedent,
    // emits the match-link (and for followed-by the reject-sink edge), inserts a
    // delay vertex for non-overlapped forms, and builds the body. Used both for
    // nested AstImplication in pexpr position and for the top-level assertion
    // antecedent -- `errorNodep` anchors the "unsupported sequence antecedent"
    // error, which differs between the two call sites.
    Build_پایله build_Implication_څنډې(Ast_غوټه_بیان* antExprp, Ast_غوټه_بیان* bodyExprp,
                                      Sva_State_راس* ثبت_Vtxp, منطقي isOverlapped,
                                      منطقي isFollowedBy, Ast_غوټه* errorNodep, File_کرښه* flp) {
        ثابت Build_پایله ant_پایله = build_بیان(antExprp, ثبت_Vtxp);
        إذا (!ant_پایله.valid()) أرجع ant_پایله;

        // Followed-by requires pure-boolean antecedent for non-vacuous-fail at
        // the attempt-start cycle. IEEE 1800-2023 16.12.9 permits a multi-cycle
        // sequence LHS, so this is an implementation gap rather than illegal SV.
        إذا (isFollowedBy && ant_پایله.غړی_Vertexp != ثبت_Vtxp) {
            errorNodep->v3warn(E_UNSUPPORTED,
                               "Unsupported: sequence expression as antecedent of followed-by"
                               " (#-# / #=#) (IEEE 1800-2023 16.12.9)");
            أرجع Build_پایله::failWithError();
        }
        UASSERT_OBJ(!isFollowedBy || ant_پایله.finalCondp, errorNodep,
                    "followed-by antecedent terminal at entry must carry finalCondp");

        // Use raw createStateVertex() so trigVtxp starts without liveness --
        // reaching the antecedent terminal is a definitive event.
        Sva_State_راس* ثابت trigVtxp = m_ګراف.create_State_راس();
        إذا (ant_پایله.finalCondp) {
            m_ګراف.جمع_کړه_Link(ant_پایله.غړی_Vertexp, trigVtxp,
                            sampled(ant_پایله.finalCondp->clone_ونه_خالص(خطأ)));
            // Followed-by non-vacuous fail: rejectOnFail fires when the attempt
            // is live (termVtx reachable) and sampled(antecedent) is false.
            إذا (isFollowedBy) {
                Sva_State_راس* ثابت sinkVtxp = m_ګراف.create_State_راس();
                sinkVtxp->m_isRejectSink = ریښتیا;
                Sva_Trans_څنډه* ثابت ep
                    = m_ګراف.جمع_کړه_Link(ant_پایله.غړی_Vertexp, sinkVtxp,
                                      sampled(ant_پایله.finalCondp->clone_ونه_خالص(خطأ)));
                ep->m_rejectOnFail = ریښتیا;
            }
            // finalCondp is cloned into the Sampled nodes; if the original is
            // not parented anywhere in the AST anymore it must be freed here
            // or ASan flags it as a leak (e.g. t_sequence_bool_ops).
            إذا (!ant_پایله.finalCondp->backp()) {
                VL_DO_DANGLING(ant_پایله.finalCondp->delete_ونه(), ant_پایله.finalCondp);
            }
        } وإلا {
            m_ګراف.جمع_کړه_Link(ant_پایله.غړی_Vertexp, trigVtxp);
        }
        resetScope();

        Sva_State_راس* bodyEntryp = trigVtxp;
        إذا (!isOverlapped) {
            Sva_State_راس* ثابت delayVtxp = m_ګراف.create_State_راس();
            m_ګراف.جمع_کړه_Clocked_څنډه(trigVtxp, delayVtxp);
            bodyEntryp = delayVtxp;
        }
        // An empty antecedent match must not start the consequent, but a zero-minimum
        // repetition's empty match is not handled here; keep the merge vertex for those
        VL_RESTORER(m_want_Bool_ونې_پاڼه);
        m_want_Bool_ونې_پاڼه = !isFollowedBy && !hasZeroMinRep(antExprp);
        أرجع build_بیان(bodyExprp, bodyEntryp, /*isTopLevelStep=*/ریښتیا);
    }

    Build_پایله build(Ast_غوټه_بیان* exprp) {
        m_ګراف.m_پیل_Vertexp = scoped_Create_راس();
        VL_RESTORER(m_want_Bool_ونې_پاڼه);
        m_want_Bool_ونې_پاڼه = ریښتیا;
        أرجع build_بیان(exprp, m_ګراف.m_پیل_Vertexp, /*isTopLevelStep=*/ریښتیا);
    }
};

//######################################################################
// NFA Lowering (converts NFA graph to synthesizable AstAlways blocks)

صنف SvaNfaLowering final {
    Ast_غوټه_Module* ثابت m_modp;  // Module to add state vars and always blocks to
    Ast_غوټه_DType* ثابت m_u32DTypep;  // Shared unsigned counter dtype
    V3_Unique_نومونه m_نومونه{"__Vnfa"};
    اندازه_t m_stat_Delay_Ring_څنډه_Visits = 0;  // Delay-ring incoming edges visited

    // Per-lowering shared context (passed to phase sub-functions)
    // Per-vertex lowering state is stored in SvaVertexData and accessed via
    // V3GraphVertex::userp() (see SvaStateVertex::datap()).
    بنية ښکتنی_Ctx final {
        File_کرښه* flp;  // Source location for generated AST
        Ast_Sen_ونه* senTreep;  // Clock sensitivity tree
        Ast_غوټه_بیان* disableExprp;  // disable iff expression (may be nullptr)
        Ast_غوټه_بیان* سمون_Condp;  // Final boolean match condition (may be nullptr)
        VAssertType assertType;  // Assertion type for control tasks
        VAssertDirectiveType directiveType;  // Directive type for control tasks
        AstVar* killVarp;  // Last observed kill generation
        Sva_ګراف& ګراف;  // NFA graph
    };

    // Build a match-now expression: stateSig[i] && $sampled(condp)
    ساكن Ast_غوټه_بیان* build_سمون_Now(File_کرښه* flp, Ast_غوټه_بیان* stateExprp, Ast_غوټه_بیان* condp) {
        Ast_غوټه_بیان* ثابت statep = stateExprp->clone_ونه_خالص(خطأ);
        إذا (!condp) أرجع statep;
        أرجع جديد AstLogAnd{flp, statep, sampled(condp->clone_ونه_خالص(خطأ))};
    }
    ساكن Ast_غوټه_بیان* andCond(File_کرښه* flp, Ast_غوټه_بیان* exprp, Ast_غوټه_بیان* condp) {
        إذا (!condp) أرجع exprp;
        أرجع جديد AstLogAnd{flp, exprp, condp->clone_ونه_خالص(خطأ)};
    }
    // bp is always non-null; only ap can be null (serving as accumulator).
    ساكن Ast_غوټه_بیان* orExprs(File_کرښه* flp, Ast_غوټه_بیان* ap, Ast_غوټه_بیان* bp) {
        إذا (!ap) أرجع bp;
        أرجع جديد AstLogOr{flp, ap, bp};
    }
    ساكن Ast_غوټه_بیان* جمع_کړه_Countp(File_کرښه* ثابت flp, Ast_غوټه_بیان* ثابت ټول_Countp,
                                  Ast_غوټه_بیان* ثابت contributionp,
                                  Ast_غوټه_بیان* ثابت enablep = تش_اشاره) {
        // contribution = enable ? contribution : 0;
        Ast_غوټه_بیان* ثابت enabledContributionp
            = enablep ? جديد AstCond{flp, enablep, contributionp,
                                    newTypedConstp(flp, contributionp->dtypep(), 0)}
                      : contributionp;
        إذا (!ټول_Countp) أرجع enabledContributionp;
        أرجع جديد Ast_جمع_کړه{flp, ټول_Countp, enabledContributionp};
    }
    ساكن Ast_غوټه_بیان* gateCheckedCountp(File_کرښه* ثابت flp, Ast_غوټه_بیان* ثابت countp,
                                          Ast_غوټه_بیان*& overflowp, Ast_غوټه_بیان* ثابت enablep) {
        إذا (overflowp) {
            overflowp = جديد AstLogAnd{flp, enablep->clone_ونه_خالص(خطأ), overflowp};
        }
        أرجع جمع_کړه_Countp(flp, تش_اشاره, countp, enablep);
    }
    ساكن Ast_غوټه_بیان* جمع_کړه_Checked_Countp(File_کرښه* ثابت flp, Ast_غوټه_بیان* ثابت ټول_Countp,
                                         Ast_غوټه_بیان* ثابت contributionp, Ast_غوټه_بیان*& overflowp,
                                         Ast_غوټه_بیان* ثابت contributionOverflowp = تش_اشاره) {
        إذا (contributionOverflowp) { overflowp = orExprs(flp, overflowp, contributionOverflowp); }
        إذا (!ټول_Countp) أرجع contributionp;
        Ast_غوټه_بیان* ثابت oldTotalp = ټول_Countp->clone_ونه_خالص(خطأ);
        Ast_غوټه_بیان* ثابت resultp = جديد Ast_جمع_کړه{flp, ټول_Countp, contributionp};
        overflowp
            = orExprs(flp, overflowp, جديد AstLt{flp, resultp->clone_ونه_خالص(خطأ), oldTotalp});
        أرجع resultp;
    }
    AstIf* new_شمېر_ډکېدنه_Warning(File_کرښه* ثابت flp, Ast_غوټه_بیان* ثابت condp) ثابت {
        AstDisplay* ثابت dispp = جديد AstDisplay{
            flp, VDisplayType::DT_WARNING,
            "Cover sequence match count overflowed 32-bit storage; results may be incorrect.",
            تش_اشاره, تش_اشاره};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstIf* ثابت ifp = جديد AstIf{flp, condp, dispp};
        ifp->branchPred(VBranchPred::BP_UNLIKELY);
        أرجع ifp;
    }
    ساكن Ast_غوټه_بیان* kill_فعال(ښکتنی_Ctx& c) {
        أرجع جديد AstNeq{c.flp, جديد AstVarRef{c.flp, c.killVarp, VAccess::READ},
                          assertKillGet(c.flp, c.assertType, c.directiveType)};
    }
    ساكن Ast_غوټه_بیان* not_Kill_فعال(ښکتنی_Ctx& c) { أرجع جديد AstLogNot{c.flp, kill_فعال(c)}; }
    ساكن Ast_غوټه_بیان* gateNotKill(ښکتنی_Ctx& c, Ast_غوټه_بیان* exprp) {
        إذا (!exprp) أرجع تش_اشاره;
        أرجع جديد AstLogAnd{c.flp, exprp, not_Kill_فعال(c)};
    }
    ساكن Ast_غوټه_بیان* بل_Ring_شاخص(File_کرښه* flp, Ast_غوټه_بیان* idxExprp, uint32_t اندازه) {
        ثابت خپلکاری u32Const = [flp](uint32_t ارزښت) {
            أرجع جديد AstConst{flp, AstConst::Widthed_ارزښت{}, 32, ارزښت};
        };
        UASSERT_OBJ(اندازه > 0, idxExprp, "Ring size must be positive");
        إذا (اندازه == 1) {
            VL_DO_DANGLING(idxExprp->delete_ونه(), idxExprp);
            أرجع u32Const(0);
        }
        // idx == size - 1 ? 0 : idx + 1
        Ast_جمع_کړه* ثابت addp = جديد Ast_جمع_کړه{flp, idxExprp->clone_ونه_خالص(خطأ), u32Const(1)};
        addp->dtypeFrom(idxExprp);
        AstCond* ثابت condp
            = جديد AstCond{flp, جديد AstEq{flp, idxExprp, u32Const(اندازه - 1)}, u32Const(0), addp};
        condp->dtypeFrom(idxExprp);
        أرجع condp;
    }
    ساكن Ast_غوټه_بیان* delay_Ring_خونه(File_کرښه* flp, Sva_State_راس* vtxp, Ast_غوټه_بیان* idxExprp,
                                      VAccess access = VAccess::READ) {
        // ring[idx]
        AstVar* ثابت ringp = vtxp->datap()->delayRingVarp;
        AstVarRef* ثابت refp = جديد AstVarRef{flp, ringp, access};
        إذا (vtxp->datap()->delayRingCounts) أرجع جديد AstArraySel{flp, refp, idxExprp};
        أرجع جديد AstSel{flp, refp, idxExprp, 1};
    }
    ساكن Ast_غوټه_بیان* delay_Ring_سر_شمېر(File_کرښه* flp, Sva_State_راس* vtxp) {
        Ast_غوټه_بیان* ثابت headp
            = جديد AstVarRef{flp, vtxp->datap()->delay_Ring_سر_Varp, VAccess::READ};
        إذا (vtxp->datap()->delayRingCounts) أرجع headp;
        أرجع جديد AstExtend{flp, headp, vtxp->datap()->delay_Ring_ژوندی_شمېر_Varp->dtypep()->پلنوالی()};
    }
    ساكن Ast_غوټه_بیان* delay_Ring_At_وروستی_شاخص(File_کرښه* ثابت flp, AstVar* ثابت idxp,
                                             ثابت uint32_t اندازه) {
        أرجع جديد AstEq{flp, جديد AstVarRef{flp, idxp, VAccess::READ},
                         جديد AstConst{flp, AstConst::Widthed_ارزښت{}, 32, اندازه - 1}};
    }
    ساكن Ast_غوټه_بیان* delay_Ring_Output_شمېر(File_کرښه* ثابت flp, Sva_State_راس* ثابت vtxp) {
        AstVar* ثابت idxp = vtxp->datap()->delayRingIdxVarp;
        ثابت uint32_t اندازه = vtxp->m_delay_Ring_اندازه;
        Ast_غوټه_بیان* outgoingValidp
            = جديد AstVarRef{flp, vtxp->datap()->delayRingWrappedVarp, VAccess::READ};
        إذا (!vtxp->m_isFixedDelayRing) {
            outgoingValidp
                = جديد AstLogOr{flp, outgoingValidp, delay_Ring_At_وروستی_شاخص(flp, idxp, اندازه)};
        }
        AstVar* ثابت ژوندی_شمېر_Varp = vtxp->datap()->delay_Ring_ژوندی_شمېر_Varp;
        أرجع جديد AstCond{flp, outgoingValidp, delay_Ring_سر_شمېر(flp, vtxp),
                           newTypedConstp(flp, ژوندی_شمېر_Varp->dtypep(), 0)};
    }
    ساكن Ast_غوټه_بیان* delay_Ring_Has_ژوندی_Threadsp(File_کرښه* ثابت flp,
                                                 AstVar* ثابت ژوندی_شمېر_Varp) {
        // active = live_count != 0;
        أرجع جديد AstNeq{flp, جديد AstVarRef{flp, ژوندی_شمېر_Varp, VAccess::READ},
                          newTypedConstp(flp, ژوندی_شمېر_Varp->dtypep(), 0)};
    }

    // Phase 3 output signals
    بنية Signal_سېټ final {
        Ast_غوټه_بیان* terminalActivep = تش_اشاره;  // OR of all successful terminal matches
        Ast_غوټه_بیان* سمون_Countp = تش_اشاره;  // NFA paths completing the sequence this tick
        Ast_غوټه_بیان* سمون_شمېر_Overflowp = تش_اشاره;  // Match count overflow this tick
        Ast_غوټه_بیان* rejectBasep = تش_اشاره;  // Reject when a terminal match fails
        Ast_غوټه_بیان* required_ګام_Rejectp = تش_اشاره;  // Per-source reject from rejectOnFail Links
        Ast_غوټه_بیان* throughoutRejectp = تش_اشاره;  // Reject when a throughout guard drops
        Ast_غوټه_بیان* threadFailCountp = تش_اشاره;  // Number of threads rejected on this tick
    };

    // Disable condition also aborts attempts between clock events. A constant disable
    // never rises, and $sampled is unsupported in a sensitivity list.
    ساكن منطقي isAsyncDisable(ثابت Ast_غوټه_بیان* ثابت disableExprp) {
        أرجع disableExprp && !VN_IS(disableExprp, Const)
               && !disableExprp->exists([](ثابت AstSampled*) { أرجع ریښتیا; });
    }

    // Emit the clocked update block of registers that hold pending attempts. The clearp
    // statements reset the same registers: a disable rising between clock events also
    // aborts pending attempts (IEEE 1800-2023 16.12), so clearp runs on that rising edge
    // in a separate block. It must clear unconditionally, as the disable may already be
    // false again when it runs, and the update block must not run on that edge, as it
    // would advance the attempts as if the clock had ticked. Blocks that hold no attempt
    // state, such as the $assertkill acknowledge in emitKillAckNba, do not go through here.
    // Callers pass a null clearp when the disable cannot rise between clock events.
    فراغ جمع_کړه_State_Always(ښکتنی_Ctx& c, Ast_غوټه* bodyp, Ast_غوټه* clearp) {
        m_modp->جمع_کړه_Stmtsp(
            جديد AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_ونه(خطأ), bodyp});
        إذا (!clearp) أرجع;
        m_modp->جمع_کړه_Stmtsp(جديد AstAlways{
            c.flp, VAlwaysKwd::ALWAYS,
            جديد Ast_Sen_ونه{c.flp, جديد Ast_Sen_توکی{c.flp, VEdgeType::ET_POSEDGE,
                                                 c.disableExprp->clone_ونه_خالص(خطأ)}},
            clearp});
    }

    // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
    // Phase 2: State register NBA always block. Each clocked-edge target
    // latches the OR of its incoming contributions.
    فراغ emit_State_ثبتوونکی_Nba(ښکتنی_Ctx& c) {
        ثابت منطقي asyncDisable = isAsyncDisable(c.disableExprp);
        Ast_غوټه* bodyp = تش_اشاره;
        Ast_غوټه* clearp = تش_اشاره;
        لكل (ثابت V3_ګراف_راس& vtxr : c.ګراف.m_ګراف.vertices()) {
            ثابت Sva_State_راس* ثابت vtxp = static_cast<ثابت Sva_State_راس*>(&vtxr);
            إذا (!vtxp->datap()->stateVarp) استمر;

            Ast_غوټه_بیان* بل_Statep = تش_اشاره;
            لكل (ثابت V3_ګراف_څنډه& edger : vtxp->دننه_څنډې()) {
                ثابت Sva_Trans_څنډه& tedger = static_cast<ثابت Sva_Trans_څنډه&>(edger);
                إذا (!tedger.m_consumes_دوره) استمر;
                ثابت Sva_State_راس* ثابت fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Clocked-edge source missing stateSig");

                Ast_غوټه_بیان* srcSigp = fromVtxp->datap()->stateSigp->clone_ونه_خالص(خطأ);
                srcSigp = andCond(c.flp, srcSigp, tedger.m_condp);

                إذا (c.disableExprp) {
                    Ast_غوټه_بیان* ثابت notDisp
                        = جديد AstLogNot{c.flp, c.disableExprp->clone_ونه_خالص(خطأ)};
                    srcSigp = جديد AstLogAnd{c.flp, srcSigp, notDisp};
                }
                بل_Statep = orExprs(c.flp, بل_Statep, srcSigp);
            }

            UASSERT_OBJ(بل_Statep, vtxp,
                        "Registered vertex has no clocked incoming contribution");
            بل_Statep = gateNotKill(c, بل_Statep);

            Ast_ټاکل_Dly* ثابت assignp = جديد Ast_ټاکل_Dly{
                c.flp, جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE}, بل_Statep};
            bodyp = Ast_غوټه::جمع_کړه_بل_Null(bodyp, assignp);
            إذا (asyncDisable) {
                clearp = Ast_غوټه::جمع_کړه_بل_Null(
                    clearp,
                    جديد Ast_ټاکل_Dly{
                        c.flp, جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE},
                        جديد AstConst{c.flp, AstConst::بټ_False{}}});
            }
        }
        إذا (bodyp) جمع_کړه_State_Always(c, bodyp, clearp);
    }

    // Phase 2b: In-place ring storage and registered ring metadata.
    فراغ emitDelayRingUpdates(ښکتنی_Ctx& c, ثابت منطقي شمېر_Matches) {
        ثابت منطقي asyncDisable = isAsyncDisable(c.disableExprp);
        لكل (V3_ګراف_راس& vtxr : c.ګراف.m_ګراف.vertices()) {
            Sva_State_راس* ثابت vtxp = static_cast<Sva_State_راس*>(&vtxr);
            إذا (!vtxp->datap()->delayRingVarp) استمر;
            AstVar* ثابت idxp = vtxp->datap()->delayRingIdxVarp;
            AstVar* ثابت ژوندی_شمېر_Varp = vtxp->datap()->delay_Ring_ژوندی_شمېر_Varp;
            AstVar* ثابت wrappedp = vtxp->datap()->delayRingWrappedVarp;
            ثابت uint32_t اندازه = static_cast<uint32_t>(vtxp->m_delay_Ring_اندازه);

            Ast_غوټه_بیان* incomingp = تش_اشاره;
            Ast_غوټه_بیان* incomingOverflowp = تش_اشاره;
            لكل (ثابت V3_ګراف_څنډه& edger : vtxp->دننه_څنډې()) {
                ++m_stat_Delay_Ring_څنډه_Visits;
                ثابت Sva_Trans_څنډه& tedger = static_cast<ثابت Sva_Trans_څنډه&>(edger);
                UASSERT_OBJ(tedger.m_consumes_دوره == vtxp->m_isFixedDelayRing, vtxp,
                            "Delay-ring incoming edge kind mismatch");
                ثابت Sva_State_راس* ثابت fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Delay-ring incoming source missing stateSig");
                UASSERT_OBJ(!شمېر_Matches || fromVtxp->datap()->state_شمېر_Sigp, fromVtxp,
                            "Delay-ring incoming source missing stateCountSig");
                // IEEE 1800-2023 16.14.1/16.14.3: assertion alternatives belong to one
                // evaluation; only cover sequence counts each end-of-match.
                Ast_غوټه_بیان* contribp
                    = شمېر_Matches
                          ? fromVtxp->datap()->state_شمېر_Sigp->clone_ونه_خالص(خطأ)
                          : جديد AstExtend{c.flp,
                                          fromVtxp->datap()->stateSigp->clone_ونه_خالص(خطأ),
                                          m_u32DTypep->پلنوالی()};
                Ast_غوټه_بیان* contribOverflowp
                    = شمېر_Matches && fromVtxp->datap()->state_شمېر_ډکېدنه_Sigp
                          ? fromVtxp->datap()->state_شمېر_ډکېدنه_Sigp->clone_ونه_خالص(خطأ)
                          : تش_اشاره;
                إذا (tedger.m_condp) {
                    Ast_غوټه_بیان* ثابت enablep = tedger.m_condp->clone_ونه_خالص(خطأ);
                    contribp = شمېر_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, enablep)
                                   : جمع_کړه_Countp(c.flp, تش_اشاره, contribp, enablep);
                }
                إذا (c.disableExprp) {
                    Ast_غوټه_بیان* ثابت notDisp
                        = جديد AstLogNot{c.flp, c.disableExprp->clone_ونه_خالص(خطأ)};
                    contribp = شمېر_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, notDisp)
                                   : جمع_کړه_Countp(c.flp, تش_اشاره, contribp, notDisp);
                }
                incomingp = شمېر_Matches ? جمع_کړه_Checked_Countp(c.flp, incomingp, contribp,
                                                            incomingOverflowp, contribOverflowp)
                                         : جمع_کړه_Countp(c.flp, incomingp, contribp);
            }
            UASSERT_OBJ(incomingp, vtxp, "Delay ring has no incoming edge");
            ثابت منطقي storesCounts = vtxp->datap()->delayRingCounts;
            Ast_غوټه_بیان* ثابت incomingCountp
                = storesCounts
                      ? incomingp->clone_ونه_خالص(خطأ)
                      : جديد AstExtend{c.flp,
                                      جديد AstNeq{c.flp, incomingp->clone_ونه_خالص(خطأ),
                                                 newTypedConstp(c.flp, incomingp->dtypep(), 0)},
                                      m_u32DTypep->پلنوالی()};
            Ast_غوټه_بیان* ثابت storedIncomingp
                = storesCounts ? incomingp
                               : جديد AstNeq{c.flp, incomingp,
                                            newTypedConstp(c.flp, incomingp->dtypep(), 0)};
            // Only this block accesses the storage. Other processes read the cached head, so
            // the ring can be updated in place without an NBA shadow copy.
            Ast_ټاکل* ثابت writeIncomingp = جديد Ast_ټاکل{
                c.flp,
                delay_Ring_خونه(c.flp, vtxp, جديد AstVarRef{c.flp, idxp, VAccess::READ},
                              VAccess::WRITE),
                storedIncomingp};
            Ast_غوټه* updateBodyp = writeIncomingp;
            // live_count <= live_count - outgoing_count + incoming_count;
            Ast_غوټه_بیان* ثابت outgoingp = delay_Ring_Output_شمېر(c.flp, vtxp);
            Ast_غوټه_بیان* ثابت afterOutgoingp
                = جديد AstSub{c.flp, جديد AstVarRef{c.flp, ژوندی_شمېر_Varp, VAccess::READ}, outgoingp};
            Ast_غوټه_بیان* ثابت oldAfterOutgoingp = afterOutgoingp->clone_ونه_خالص(خطأ);
            Ast_جمع_کړه* ثابت بل_ژوندی_Countp = جديد Ast_جمع_کړه{c.flp, afterOutgoingp, incomingCountp};
            إذا (storesCounts) {
                Ast_غوټه_بیان* ثابت ژوندی_Overflowp
                    = جديد AstLt{c.flp, بل_ژوندی_Countp->clone_ونه_خالص(خطأ), oldAfterOutgoingp};
                Ast_غوټه_بیان* ثابت overflowp = orExprs(c.flp, incomingOverflowp, ژوندی_Overflowp);
                updateBodyp = new_شمېر_ډکېدنه_Warning(c.flp, overflowp);
                updateBodyp->جمع_کړه_بل(writeIncomingp);
            } وإلا {
                VL_DO_DANGLING(oldAfterOutgoingp->delete_ونه(), oldAfterOutgoingp);
            }
            // Cache the outgoing slot for the next index after the write. Reading after
            // writing also handles one-slot rings and two-slot range rings without alias cases.
            Ast_غوټه_بیان* بل_سر_Idxp
                = بل_Ring_شاخص(c.flp, جديد AstVarRef{c.flp, idxp, VAccess::READ}, اندازه);
            إذا (!vtxp->m_isFixedDelayRing) {
                بل_سر_Idxp = بل_Ring_شاخص(c.flp, بل_سر_Idxp, اندازه);
            }
            updateBodyp->جمع_کړه_بل(جديد Ast_ټاکل_Dly{
                c.flp, جديد AstVarRef{c.flp, vtxp->datap()->delay_Ring_سر_Varp, VAccess::WRITE},
                delay_Ring_خونه(c.flp, vtxp, بل_سر_Idxp)});
            updateBodyp->جمع_کړه_بل(جديد Ast_ټاکل_Dly{
                c.flp, جديد AstVarRef{c.flp, ژوندی_شمېر_Varp, VAccess::WRITE}, بل_ژوندی_Countp});
            // wrapped <= wrapped || idx == size - 1;
            updateBodyp->جمع_کړه_بل(
                جديد Ast_ټاکل_Dly{c.flp, جديد AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                 جديد AstLogOr{c.flp, جديد AstVarRef{c.flp, wrappedp, VAccess::READ},
                                              delay_Ring_At_وروستی_شاخص(c.flp, idxp, اندازه)}});
            // idx <= next_idx;
            updateBodyp->جمع_کړه_بل(جديد Ast_ټاکل_Dly{
                c.flp, جديد AstVarRef{c.flp, idxp, VAccess::WRITE},
                بل_Ring_شاخص(c.flp, جديد AstVarRef{c.flp, idxp, VAccess::READ}, اندازه)});
            إذا (vtxp->m_delayRingAdvanceCondp) {
                updateBodyp = جديد AstIf{
                    c.flp, sampled(vtxp->m_delayRingAdvanceCondp->clone_ونه_خالص(خطأ)),
                    updateBodyp};
            }

            Ast_غوټه_بیان* clearCondp = kill_فعال(c);
            إذا (vtxp->m_delayRingClearCondp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_delayRingClearCondp->clone_ونه_خالص(خطأ)));
            }
            إذا (vtxp->m_abortClearp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_abortClearp->clone_ونه_خالص(خطأ)));
            }
            إذا (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_ونه_خالص(خطأ));
            }
            Ast_غوټه_بیان* guardp = تش_اشاره;
            لكل (Ast_غوټه_بیان* ثابت cp : vtxp->m_throughoutConds) {
                Ast_غوټه_بیان* ثابت sampledp = sampled(cp->clone_ونه_خالص(خطأ));
                guardp = guardp ? static_cast<Ast_غوټه_بیان*>(جديد AstLogAnd{c.flp, guardp, sampledp})
                                : sampledp;
            }
            إذا (guardp) clearCondp = orExprs(c.flp, clearCondp, جديد AstLogNot{c.flp, guardp});
            // Logically clear the ring without touching its wide storage.
            Ast_ټاکل_Dly* ثابت clearCountp
                = جديد Ast_ټاکل_Dly{c.flp, جديد AstVarRef{c.flp, ژوندی_شمېر_Varp, VAccess::WRITE},
                                   newTypedConstp(c.flp, ژوندی_شمېر_Varp->dtypep(), 0)};
            clearCountp->جمع_کړه_بل(جديد Ast_ټاکل_Dly{c.flp,
                                                  جديد AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                                  جديد AstConst{c.flp, AstConst::بټ_False{}}});
            clearCountp->جمع_کړه_بل(جديد Ast_ټاکل_Dly{c.flp,
                                                  جديد AstVarRef{c.flp, idxp, VAccess::WRITE},
                                                  newTypedConstp(c.flp, idxp->dtypep(), 0)});
            updateBodyp = جديد AstIf{c.flp, clearCondp, clearCountp, updateBodyp};

            جمع_کړه_State_Always(c, updateBodyp, asyncDisable ? clearCountp->clone_ونه(ریښتیا) : تش_اشاره);
        }
    }

    // Phase 2c: SAnd combiner done-latch always block.
    // NBA semantics ensure doneL/doneR read pre-update values (IEEE 16.9.5).
    فراغ emit_And_Combiner_بشپړ_Latch_Nba(ښکتنی_Ctx& c) {
        ثابت منطقي asyncDisable = isAsyncDisable(c.disableExprp);
        لكل (ثابت V3_ګراف_راس& vtxr : c.ګراف.m_ګراف.vertices()) {
            ثابت Sva_State_راس* ثابت avp = static_cast<ثابت Sva_State_راس*>(&vtxr);
            إذا (!avp->datap()->بشپړ_LVarp) استمر;
            // doneLVars is non-null only for AndCombiner vertices, which always
            // have both m_andLhsTermp and m_andRhsTermp set at build time.
            UASSERT_OBJ(avp->m_andLhsTermp && avp->m_andRhsTermp, avp,
                        "AndCombiner vertex missing LHS/RHS terminal");
            ثابت Sva_State_راس* ثابت lhsTermp = avp->m_andLhsTermp;
            ثابت Sva_State_راس* ثابت rhsTermp = avp->m_andRhsTermp;
            // resolveLinks' 2*N+2 fixed-point pass is guaranteed to populate
            // stateSigp on every AndCombiner and its LHS/RHS terminals.
            UASSERT_OBJ(lhsTermp->datap()->stateSigp && rhsTermp->datap()->stateSigp
                            && avp->datap()->stateSigp,
                        avp, "AndCombiner stateSigp chain unresolved after resolveLinks");

            Ast_ټاکل_Dly* ثابت clearLp = جديد Ast_ټاکل_Dly{
                c.flp, جديد AstVarRef{c.flp, avp->datap()->بشپړ_LVarp, VAccess::WRITE},
                جديد AstConst{c.flp, AstConst::بټ_False{}}};
            Ast_ټاکل_Dly* ثابت clearRp = جديد Ast_ټاکل_Dly{
                c.flp, جديد AstVarRef{c.flp, avp->datap()->بشپړ_RVarp, VAccess::WRITE},
                جديد AstConst{c.flp, AstConst::بټ_False{}}};
            clearLp->جمع_کړه_بل(clearRp);

            Ast_غوټه_بیان* ثابت سمون_LNowp
                = build_سمون_Now(c.flp, lhsTermp->datap()->stateSigp, avp->m_andLhsCondp);
            Ast_غوټه_بیان* ثابت سمون_RNowp
                = build_سمون_Now(c.flp, rhsTermp->datap()->stateSigp, avp->m_andRhsCondp);
            Ast_غوټه_بیان* gateLp = سمون_LNowp;
            Ast_غوټه_بیان* gateRp = سمون_RNowp;
            إذا (c.disableExprp) {
                Ast_غوټه_بیان* ثابت notDisLp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_ونه_خالص(خطأ)};
                gateLp = جديد AstLogAnd{c.flp, gateLp, notDisLp};
                Ast_غوټه_بیان* ثابت notDisRp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_ونه_خالص(خطأ)};
                gateRp = جديد AstLogAnd{c.flp, gateRp, notDisRp};
            }
            Ast_ټاکل_Dly* ثابت سېټ_Lp = جديد Ast_ټاکل_Dly{
                c.flp, جديد AstVarRef{c.flp, avp->datap()->بشپړ_LVarp, VAccess::WRITE},
                جديد AstConst{c.flp, AstConst::بټ_True{}}};
            AstIf* ثابت سېټ_LIfp = جديد AstIf{c.flp, gateLp, سېټ_Lp, تش_اشاره};
            Ast_ټاکل_Dly* ثابت سېټ_Rp = جديد Ast_ټاکل_Dly{
                c.flp, جديد AstVarRef{c.flp, avp->datap()->بشپړ_RVarp, VAccess::WRITE},
                جديد AstConst{c.flp, AstConst::بټ_True{}}};
            AstIf* ثابت سېټ_RIfp = جديد AstIf{c.flp, gateRp, سېټ_Rp, تش_اشاره};
            سېټ_LIfp->جمع_کړه_بل(سېټ_RIfp);

            Ast_غوټه_بیان* clearCondp = جديد AstLogOr{c.flp, kill_فعال(c),
                                                   avp->datap()->stateSigp->clone_ونه_خالص(خطأ)};
            إذا (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_ونه_خالص(خطأ));
            }
            AstIf* ثابت topp = جديد AstIf{c.flp, clearCondp, clearLp, سېټ_LIfp};
            جمع_کړه_State_Always(c, topp, asyncDisable ? clearLp->clone_ونه(ریښتیا) : تش_اشاره);
        }
    }

    فراغ emitKillAckNba(ښکتنی_Ctx& c) {
        Ast_ټاکل_Dly* ثابت ackp
            = جديد Ast_ټاکل_Dly{c.flp, جديد AstVarRef{c.flp, c.killVarp, VAccess::WRITE},
                               assertKillGet(c.flp, c.assertType, c.directiveType)};
        m_modp->جمع_کړه_Stmtsp(
            جديد AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_ونه(خطأ), ackp});
    }

    // Phase 3/3a/3b: Compute terminal match/reject signals, required-step reject,
    // throughout-drop reject; clean up intermediate state signals.
    // Phase 3: terminalActive and rejectBase from Links to matchVertex.
    // Builder only adds Links (non-clocked) to matchVertex via addLink in
    // wireMatchAndMidSources. For cover sequence, also count each end-of-match
    // so the action can be replayed without unrolling endpoints.
    فراغ compute_Terminal_سمون_And_Reject(ښکتنی_Ctx& c, Signal_سېټ& sigs, ثابت منطقي need_سمون_شمېر) {
        لكل (ثابت Sva_Trans_څنډه* ثابت tedgep : c.ګراف.all_څنډې()) {
            إذا (tedgep->منزل_Vtxp() != c.ګراف.m_سمون_Vertexp) استمر;
            Sva_State_راس* ثابت fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                        "Terminal-link source missing stateSig");

            Ast_غوټه_بیان* srcSigp = fromVtxp->datap()->stateSigp->clone_ونه_خالص(خطأ);
            srcSigp = andCond(c.flp, srcSigp, tedgep->m_condp);
            إذا (need_سمون_شمېر) {
                UASSERT_OBJ(fromVtxp->datap()->state_شمېر_Sigp, fromVtxp,
                            "Terminal-link source missing stateCountSig");
                Ast_غوټه_بیان* contributionp
                    = fromVtxp->datap()->state_شمېر_Sigp->clone_ونه_خالص(خطأ);
                Ast_غوټه_بیان* contributionOverflowp
                    = fromVtxp->datap()->state_شمېر_ډکېدنه_Sigp
                          ? fromVtxp->datap()->state_شمېر_ډکېدنه_Sigp->clone_ونه_خالص(خطأ)
                          : تش_اشاره;
                إذا (tedgep->m_condp) {
                    contributionp = gateCheckedCountp(c.flp, contributionp, contributionOverflowp,
                                                      tedgep->m_condp->clone_ونه_خالص(خطأ));
                }
                sigs.سمون_Countp
                    = جمع_کړه_Checked_Countp(c.flp, sigs.سمون_Countp, contributionp,
                                       sigs.سمون_شمېر_Overflowp, contributionOverflowp);
            }

            إذا (fromVtxp->m_delay_Ring_اندازه && !fromVtxp->m_isFixedDelayRing) {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_ونه_خالص(خطأ));
                // reject |= (wrapped || idx == size - 1) && head && final_condition;
                Ast_غوټه_بیان* expireContribp = delay_Ring_Output_شمېر(c.flp, fromVtxp);
                expireContribp = andCond(c.flp, expireContribp, tedgep->m_condp);
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, expireContribp);
                VL_DO_DANGLING(srcSigp->delete_ونه(), srcSigp);
            } وإلا إذا (fromVtxp->m_isUnbounded || fromVtxp->m_isAndCombiner) {
                sigs.terminalActivep = orExprs(c.flp, sigs.terminalActivep, srcSigp);
            } وإلا {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_ونه_خالص(خطأ));
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, srcSigp);
            }
        }
        // wireMatchAndMidSources always adds a Link from result.termVertexp
        // to m_matchVertexp, so the loop above always sets terminalActivep.
        UASSERT_OBJ(sigs.terminalActivep, c.ګراف.m_سمون_Vertexp,
                    "No terminal edge to match vertex");
    }

    Ast_غوټه_بیان* newThroughoutThreadFailCountp(ښکتنی_Ctx& c, AstVar* ثابت delay_Ring_ژوندی_شمېر_Varp,
                                               Ast_غوټه_بیان* ثابت stateExprp,
                                               Ast_غوټه_بیان* ثابت enablep) {
        Ast_غوټه_بیان* ثابت فعال_Thread_Countp
            = delay_Ring_ژوندی_شمېر_Varp
                  ? static_cast<Ast_غوټه_بیان*>(
                        جديد AstVarRef{c.flp, delay_Ring_ژوندی_شمېر_Varp, VAccess::READ})
                  : جديد AstExtend{c.flp, stateExprp->clone_ونه_خالص(خطأ), m_u32DTypep->پلنوالی()};
        أرجع جمع_کړه_Countp(c.flp, تش_اشاره, فعال_Thread_Countp, enablep);
    }

    // Phase 3b: Throughout-drop and ring-wide abort rejection.
    فراغ computeThroughoutReject(ښکتنی_Ctx& c, Signal_سېټ& sigs, ثابت منطقي need_Thread_Fail_شمېر) {
        لكل (ثابت V3_ګراف_راس& vtxr : c.ګراف.m_ګراف.vertices()) {
            ثابت Sva_State_راس* ثابت vtxp = static_cast<ثابت Sva_State_راس*>(&vtxr);
            ثابت خپلکاری& conds = vtxp->m_throughoutConds;
            إذا (conds.تشه() && !vtxp->m_abortRejectp) استمر;
            إذا (vtxp->m_isAndCombiner) استمر;
            Ast_غوټه_بیان* stateExprp = تش_اشاره;
            إذا (vtxp->datap()->stateVarp) {
                stateExprp = جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
            } وإلا إذا (vtxp->datap()->delayRingVarp && vtxp->m_isFixedDelayRing) {
                stateExprp
                    = delay_Ring_Has_ژوندی_Threadsp(c.flp, vtxp->datap()->delay_Ring_ژوندی_شمېر_Varp);
            } وإلا {
                UASSERT_OBJ(vtxp->datap()->stateSigp, vtxp,
                            "Throughout-conds vertex missing state representation");
                stateExprp = vtxp->datap()->stateSigp->clone_ونه_خالص(خطأ);
            }
            Ast_غوټه_بیان* guardp = تش_اشاره;
            لكل (Ast_غوټه_بیان* ثابت cp : conds) {
                Ast_غوټه_بیان* ثابت sp = sampled(cp->clone_ونه_خالص(خطأ));
                guardp = guardp ? static_cast<Ast_غوټه_بیان*>(جديد AstLogAnd{c.flp, guardp, sp}) : sp;
            }
            إذا (vtxp->m_abortRejectp) {
                Ast_غوټه_بیان* ثابت notAbortp
                    = جديد AstLogNot{c.flp, sampled(vtxp->m_abortRejectp->clone_ونه_خالص(خطأ))};
                guardp = guardp
                             ? static_cast<Ast_غوټه_بیان*>(جديد AstLogAnd{c.flp, guardp, notAbortp})
                             : notAbortp;
            }
            Ast_غوټه_بیان* rejectCondp = جديد AstLogNot{c.flp, guardp};
            إذا (vtxp->m_abortClearp) {
                // Any abort clears the ring. Accept aborts suppress a simultaneous guard failure;
                // reject aborts are restored below so they still force rejection.
                Ast_غوټه_بیان* ثابت notAbortClearp
                    = جديد AstLogNot{c.flp, sampled(vtxp->m_abortClearp->clone_ونه_خالص(خطأ))};
                rejectCondp = جديد AstLogAnd{c.flp, rejectCondp, notAbortClearp};
                إذا (vtxp->m_abortRejectp) {
                    rejectCondp = جديد AstLogOr{
                        c.flp, rejectCondp, sampled(vtxp->m_abortRejectp->clone_ونه_خالص(خطأ))};
                }
            }
            إذا (need_Thread_Fail_شمېر) {
                Ast_غوټه_بیان* ثابت contributionp
                    = newThroughoutThreadFailCountp(c, vtxp->datap()->delay_Ring_ژوندی_شمېر_Varp,
                                                    stateExprp, rejectCondp->clone_ونه_خالص(خطأ));
                sigs.threadFailCountp = جمع_کړه_Countp(c.flp, sigs.threadFailCountp, contributionp);
                إذا (vtxp->m_abortRejectp && vtxp->m_delayRingAdvanceCondp) {
                    // An advancing ring thread also occupies its same-tick match vertex in the
                    // unrolled NFA, so abort rejection must replay both fail actions.
                    Ast_غوټه_بیان* ثابت abortAndAdvancep = جديد AstLogAnd{
                        c.flp, sampled(vtxp->m_abortRejectp->clone_ونه_خالص(خطأ)),
                        sampled(vtxp->m_delayRingAdvanceCondp->clone_ونه_خالص(خطأ))};
                    Ast_غوټه_بیان* ثابت سمون_Contributionp = newThroughoutThreadFailCountp(
                        c, vtxp->datap()->delay_Ring_ژوندی_شمېر_Varp, stateExprp, abortAndAdvancep);
                    sigs.threadFailCountp
                        = جمع_کړه_Countp(c.flp, sigs.threadFailCountp, سمون_Contributionp);
                }
            }
            sigs.throughoutRejectp = orExprs(c.flp, sigs.throughoutRejectp,
                                             جديد AstLogAnd{c.flp, stateExprp, rejectCondp});
        }
    }

    Signal_سېټ compute_سیګنالونه(ښکتنی_Ctx& c, ثابت منطقي need_Thread_Fail_شمېر,
                             ثابت منطقي need_Throughout_Thread_Fail_شمېر, ثابت منطقي need_سمون_شمېر) {
        Signal_سېټ sigs;

        compute_Terminal_سمون_And_Reject(c, sigs, need_سمون_شمېر);

        // Phase 3a: required-step rejection.
        // Builder only sets m_rejectOnFail on non-clocked Links with m_condp
        // or m_condVtxp, and the source always has a resolved stateSig.
        لكل (ثابت Sva_Trans_څنډه* ثابت tedgep : c.ګراف.all_څنډې()) {
            إذا (!tedgep->m_rejectOnFail) استمر;
            ثابت Sva_State_راس* ثابت fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp && (tedgep->m_condp || tedgep->m_condVtxp),
                        fromVtxp,
                        "rejectOnFail Link must have condp/condVtxp and source stateSig");
            Ast_غوټه_بیان* ثابت srcSigp = fromVtxp->datap()->stateSigp->clone_ونه_خالص(خطأ);
            Ast_غوټه_بیان* condp = تش_اشاره;
            إذا (tedgep->m_condVtxp) {
                UASSERT_OBJ(tedgep->m_condVtxp->datap()->stateSigp, tedgep->m_condVtxp,
                            "rejectOnFail condVtxp missing stateSig");
                condp = tedgep->m_condVtxp->datap()->stateSigp->clone_ونه_خالص(خطأ);
                إذا (tedgep->m_condp) {
                    condp = جديد AstLogOr{c.flp, condp, tedgep->m_condp->clone_ونه_خالص(خطأ)};
                }
            } وإلا {
                condp = tedgep->m_condp->clone_ونه_خالص(خطأ);
            }
            Ast_غوټه_بیان* ثابت notCondp = جديد AstLogNot{c.flp, condp};
            Ast_غوټه_بیان* ثابت rawFailp = جديد AstLogAnd{c.flp, srcSigp, notCondp};
            إذا (need_Thread_Fail_شمېر) {
                // thread_fail_count += fail;
                sigs.threadFailCountp = جمع_کړه_Countp(
                    c.flp, sigs.threadFailCountp,
                    جديد AstExtend{c.flp, rawFailp->clone_ونه_خالص(خطأ), m_u32DTypep->پلنوالی()});
            }
            Ast_غوټه_بیان* ثابت failp = gateNotKill(c, rawFailp);
            sigs.required_ګام_Rejectp = orExprs(c.flp, sigs.required_ګام_Rejectp, failp);
        }

        computeThroughoutReject(c, sigs, need_Throughout_Thread_Fail_شمېر);
        إذا (sigs.threadFailCountp) {
            sigs.threadFailCountp
                = جمع_کړه_Countp(c.flp, تش_اشاره, sigs.threadFailCountp, not_Kill_فعال(c));
        }
        إذا (sigs.سمون_Countp) {
            إذا (c.سمون_Condp) {
                sigs.سمون_Countp
                    = gateCheckedCountp(c.flp, sigs.سمون_Countp, sigs.سمون_شمېر_Overflowp,
                                        sampled(c.سمون_Condp->clone_ونه_خالص(خطأ)));
            }
            sigs.سمون_Countp = gateCheckedCountp(c.flp, sigs.سمون_Countp, sigs.سمون_شمېر_Overflowp,
                                                 not_Kill_فعال(c));
        }
        sigs.terminalActivep = gateNotKill(c, sigs.terminalActivep);
        sigs.rejectBasep = gateNotKill(c, sigs.rejectBasep);
        sigs.throughoutRejectp = gateNotKill(c, sigs.throughoutRejectp);

        // Clean up intermediate state signals. These are orphan subtrees
        // (never linked into the enclosing AST); deleteTree() is immediate
        // which is what we want since stateSigp lifetime ends with this scope.
        لكل (V3_ګراف_راس& vtxr : c.ګراف.m_ګراف.vertices()) {
            Sva_State_راس* ثابت vtxp = static_cast<Sva_State_راس*>(&vtxr);
            Ast_غوټه_بیان*& sigp = vtxp->datap()->stateSigp;
            إذا (sigp) VL_DO_DANGLING(sigp->delete_ونه(), sigp);
            Ast_غوټه_بیان*& countp = vtxp->datap()->state_شمېر_Sigp;
            إذا (countp) VL_DO_DANGLING(countp->delete_ونه(), countp);
            Ast_غوټه_بیان*& overflowp = vtxp->datap()->state_شمېر_ډکېدنه_Sigp;
            إذا (overflowp) VL_DO_DANGLING(overflowp->delete_ونه(), overflowp);
        }
        // Disable iff gating (IEEE 1800-2023 16.12) on the current level value.
        إذا (c.disableExprp) {
            // terminalActivep is always set, so gate it unconditionally.
            Ast_غوټه_بیان* ثابت notTermp
                = جديد AstLogNot{c.flp, c.disableExprp->clone_ونه_خالص(خطأ)};
            sigs.terminalActivep = جديد AstLogAnd{c.flp, sigs.terminalActivep, notTermp};
            إذا (sigs.rejectBasep) {
                Ast_غوټه_بیان* ثابت notDisp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_ونه_خالص(خطأ)};
                sigs.rejectBasep = جديد AstLogAnd{c.flp, sigs.rejectBasep, notDisp};
            }
            إذا (sigs.throughoutRejectp) {
                Ast_غوټه_بیان* ثابت notDisp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_ونه_خالص(خطأ)};
                sigs.throughoutRejectp = جديد AstLogAnd{c.flp, sigs.throughoutRejectp, notDisp};
            }
            إذا (sigs.required_ګام_Rejectp) {
                Ast_غوټه_بیان* ثابت notDisp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_ونه_خالص(خطأ)};
                sigs.required_ګام_Rejectp = جديد AstLogAnd{c.flp, sigs.required_ګام_Rejectp, notDisp};
            }
            إذا (sigs.سمون_Countp) {
                sigs.سمون_Countp = gateCheckedCountp(
                    c.flp, sigs.سمون_Countp, sigs.سمون_شمېر_Overflowp,
                    جديد AstLogNot{c.flp, c.disableExprp->clone_ونه_خالص(خطأ)});
            }
        }

        إذا (sigs.سمون_شمېر_Overflowp) {
            m_modp->جمع_کړه_Stmtsp(
                جديد AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_ونه(خطأ),
                              new_شمېر_ډکېدنه_Warning(c.flp, sigs.سمون_شمېر_Overflowp)});
            sigs.سمون_شمېر_Overflowp = تش_اشاره;
        }

        أرجع sigs;
    }

    // Phase 1: Resolve combinational Links via fixed-point propagation.
    فراغ resolveLinks(ښکتنی_Ctx& c, Ast_غوټه_بیان* triggerExprp, ثابت منطقي شمېر_Matches) {
        // datap() was freshly allocated in lower() -- all state signals start null.
        c.ګراف.m_پیل_Vertexp->datap()->stateSigp = triggerExprp->clone_ونه_خالص(خطأ);
        c.ګراف.m_پیل_Vertexp->datap()->state_شمېر_Sigp
            = جديد AstExtend{c.flp, triggerExprp->clone_ونه_خالص(خطأ), m_u32DTypep->پلنوالی()};
        لكل (V3_ګراف_راس& vtxr : c.ګراف.m_ګراف.vertices()) {
            Sva_State_راس* ثابت vtxp = static_cast<Sva_State_راس*>(&vtxr);
            إذا (vtxp->datap()->stateVarp) {
                vtxp->datap()->stateSigp
                    = جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
                vtxp->datap()->state_شمېر_Sigp = جديد AstExtend{
                    c.flp, جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ},
                    m_u32DTypep->پلنوالی()};
            } وإلا إذا (vtxp->datap()->delayRingVarp) {
                إذا (vtxp->m_isFixedDelayRing) {
                    // state_count = wrapped ? head : 0; state = state_count != 0;
                    Ast_غوټه_بیان* ثابت countp = delay_Ring_Output_شمېر(c.flp, vtxp);
                    vtxp->datap()->stateSigp = جديد AstNeq{c.flp, countp->clone_ونه_خالص(خطأ),
                                                          newTypedConstp(c.flp, m_u32DTypep, 0)};
                    vtxp->datap()->state_شمېر_Sigp = countp;
                } وإلا {
                    vtxp->datap()->stateSigp
                        = delay_Ring_Has_ژوندی_Threadsp(c.flp, vtxp->datap()->delay_Ring_ژوندی_شمېر_Varp);
                    vtxp->datap()->state_شمېر_Sigp = جديد AstVarRef{
                        c.flp, vtxp->datap()->delay_Ring_ژوندی_شمېر_Varp, VAccess::READ};
                }
            }
        }
        // Fixed-point propagation along zero-delay (Link) edges. Rebuild each
        // derived signal from its incoming edges on every pass; appending the same
        // contributions repeatedly makes merge expressions grow exponentially.
        ثابت اندازه_t passes = 2 * c.ګراف.m_ګراف.vertices().اندازه() + 2;
        لكل (اندازه_t pass = 0; pass < passes; ++pass) {
            // Rebuild SAnd combiners once both sub-NFA terminals are available.
            لكل (V3_ګراف_راس& vtxr : c.ګراف.m_ګراف.vertices()) {
                Sva_State_راس* ثابت vtxp = static_cast<Sva_State_راس*>(&vtxr);
                إذا (!vtxp->m_isAndCombiner) استمر;
                // AndCombiner vertices always have both terminal pointers set.
                UASSERT_OBJ(vtxp->m_andLhsTermp && vtxp->m_andRhsTermp, vtxp,
                            "AndCombiner vertex missing LHS/RHS terminal");
                ثابت Sva_State_راس* ثابت lhsTermp = vtxp->m_andLhsTermp;
                ثابت Sva_State_راس* ثابت rhsTermp = vtxp->m_andRhsTermp;
                إذا (!lhsTermp->datap()->stateSigp || !rhsTermp->datap()->stateSigp) استمر;
                Ast_غوټه_بیان* ثابت سمون_Lp
                    = build_سمون_Now(c.flp, lhsTermp->datap()->stateSigp, vtxp->m_andLhsCondp);
                Ast_غوټه_بیان* ثابت سمون_Rp
                    = build_سمون_Now(c.flp, rhsTermp->datap()->stateSigp, vtxp->m_andRhsCondp);
                Ast_غوټه_بیان* ثابت بشپړ_LOrp = جديد AstLogOr{
                    c.flp, جديد AstVarRef{c.flp, vtxp->datap()->بشپړ_LVarp, VAccess::READ}, سمون_Lp};
                Ast_غوټه_بیان* ثابت بشپړ_ROrp = جديد AstLogOr{
                    c.flp, جديد AstVarRef{c.flp, vtxp->datap()->بشپړ_RVarp, VAccess::READ}, سمون_Rp};
                Ast_غوټه_بیان* ثابت bothp = جديد AstLogAnd{c.flp, بشپړ_LOrp, بشپړ_ROrp};
                Ast_غوټه_بیان* ثابت oneNowp = جديد AstLogOr{c.flp, سمون_Lp->clone_ونه_خالص(خطأ),
                                                          سمون_Rp->clone_ونه_خالص(خطأ)};
                إذا (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_ونه(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = جديد AstLogAnd{c.flp, bothp, oneNowp};
                إذا (vtxp->datap()->state_شمېر_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_شمېر_Sigp->delete_ونه(),
                                   vtxp->datap()->state_شمېر_Sigp);
                }
                vtxp->datap()->state_شمېر_Sigp = جديد AstExtend{
                    c.flp, vtxp->datap()->stateSigp->clone_ونه_خالص(خطأ), m_u32DTypep->پلنوالی()};
            }

            لكل (V3_ګراف_راس& vtxr : c.ګراف.m_ګراف.vertices()) {
                Sva_State_راس* ثابت vtxp = static_cast<Sva_State_راس*>(&vtxr);
                إذا (vtxp == c.ګراف.m_پیل_Vertexp || vtxp->datap()->stateVarp
                    || vtxp->datap()->delayRingVarp || vtxp->m_isAndCombiner || vtxp->m_is_سمون
                    || vtxp->m_isRejectSink) {
                    استمر;
                }
                Ast_غوټه_بیان* بل_Statep = تش_اشاره;
                Ast_غوټه_بیان* بل_State_Countp = تش_اشاره;
                Ast_غوټه_بیان* بل_State_شمېر_Overflowp = تش_اشاره;
                لكل (ثابت V3_ګراف_څنډه& er : vtxp->دننه_څنډې()) {
                    ثابت Sva_Trans_څنډه& te = static_cast<ثابت Sva_Trans_څنډه&>(er);
                    ثابت Sva_State_راس* ثابت fromVtxp = te.fromVtxp();
                    إذا (!fromVtxp->datap()->stateSigp) استمر;
                    UASSERT_OBJ(fromVtxp->datap()->state_شمېر_Sigp, fromVtxp,
                                "Link source missing stateCountSig");
                    Ast_غوټه_بیان* ثابت contributionp = andCond(
                        c.flp, fromVtxp->datap()->stateSigp->clone_ونه_خالص(خطأ), te.m_condp);
                    بل_Statep = orExprs(c.flp, بل_Statep, contributionp);
                    Ast_غوټه_بیان* شمېر_Contributionp
                        = fromVtxp->datap()->state_شمېر_Sigp->clone_ونه_خالص(خطأ);
                    Ast_غوټه_بیان* شمېر_Contribution_Overflowp
                        = شمېر_Matches && fromVtxp->datap()->state_شمېر_ډکېدنه_Sigp
                              ? fromVtxp->datap()->state_شمېر_ډکېدنه_Sigp->clone_ونه_خالص(خطأ)
                              : تش_اشاره;
                    إذا (te.m_condp) {
                        Ast_غوټه_بیان* ثابت enablep = te.m_condp->clone_ونه_خالص(خطأ);
                        شمېر_Contributionp
                            = شمېر_Matches
                                  ? gateCheckedCountp(c.flp, شمېر_Contributionp,
                                                      شمېر_Contribution_Overflowp, enablep)
                                  : جمع_کړه_Countp(c.flp, تش_اشاره, شمېر_Contributionp, enablep);
                    }
                    بل_State_Countp
                        = شمېر_Matches
                              ? جمع_کړه_Checked_Countp(c.flp, بل_State_Countp, شمېر_Contributionp,
                                                 بل_State_شمېر_Overflowp,
                                                 شمېر_Contribution_Overflowp)
                              : جمع_کړه_Countp(c.flp, بل_State_Countp, شمېر_Contributionp);
                }
                إذا (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_ونه(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = بل_Statep;
                إذا (vtxp->datap()->state_شمېر_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_شمېر_Sigp->delete_ونه(),
                                   vtxp->datap()->state_شمېر_Sigp);
                }
                vtxp->datap()->state_شمېر_Sigp = بل_State_Countp;
                إذا (vtxp->datap()->state_شمېر_ډکېدنه_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_شمېر_ډکېدنه_Sigp->delete_ونه(),
                                   vtxp->datap()->state_شمېر_ډکېدنه_Sigp);
                }
                vtxp->datap()->state_شمېر_ډکېدنه_Sigp = بل_State_شمېر_Overflowp;
            }
        }
    }

عام:
    // Combine terminal/reject signals into final output expression.
    ساكن Ast_غوټه_بیان* assemble_پایله(Ast_غوټه_Cover_Or_Assert* ثابت assertp, ثابت منطقي negated,
                                       Ast_غوټه_بیان* ثابت سمون_Condp, ثابت Signal_سېټ& sigs,
                                       Ast_غوټه_بیان** ثابت بهر_Matchpp) {
        File_کرښه* ثابت flp = assertp->fileline();
        ثابت منطقي isCover = VN_IS(assertp, Cover);
        Ast_غوټه_بیان* ثابت terminalActivep = sigs.terminalActivep;
        Ast_غوټه_بیان* ثابت rejectBasep = sigs.rejectBasep;
        Ast_غوټه_بیان* ثابت throughoutRejectp = sigs.throughoutRejectp;
        Ast_غوټه_بیان* ثابت required_ګام_Rejectp = sigs.required_ګام_Rejectp;
        // Property negation (IEEE 1800-2023 16.12.1 `not`): invert match/reject.
        إذا (negated) {
            إذا (isCover) {
                إذا (terminalActivep)
                    VL_DO_DANGLING(terminalActivep->delete_ونه(), terminalActivep);
                Ast_غوټه_بیان* negRejectp = تش_اشاره;
                إذا (سمون_Condp && rejectBasep) {
                    Ast_غوټه_بیان* ثابت sampledCondp = sampled(سمون_Condp->clone_ونه_خالص(خطأ));
                    Ast_غوټه_بیان* ثابت notCondp = جديد AstLogNot{flp, sampledCondp};
                    negRejectp = جديد AstLogAnd{flp, rejectBasep, notCondp};
                } وإلا إذا (rejectBasep) {
                    VL_DO_DANGLING(rejectBasep->delete_ونه(), rejectBasep);
                }
                إذا (throughoutRejectp) negRejectp = orExprs(flp, negRejectp, throughoutRejectp);
                إذا (required_ګام_Rejectp)
                    negRejectp = orExprs(flp, negRejectp, required_ګام_Rejectp);
                أرجع negRejectp ? negRejectp : جديد AstConst{flp, AstConst::بټ_False{}};
            }
            // Negated assert/assume: output = !match.
            Ast_غوټه_بیان* matchp = terminalActivep;
            إذا (سمون_Condp) {
                Ast_غوټه_بیان* ثابت sampledCondp = sampled(سمون_Condp->clone_ونه_خالص(خطأ));
                matchp = جديد AstLogAnd{flp, matchp, sampledCondp};
            }
            إذا (بهر_Matchpp) {
                Ast_غوټه_بیان* notPMatchp = تش_اشاره;
                إذا (سمون_Condp && rejectBasep) {
                    Ast_غوټه_بیان* ثابت sampledCondp = sampled(سمون_Condp->clone_ونه_خالص(خطأ));
                    notPMatchp = جديد AstLogAnd{flp, rejectBasep->clone_ونه_خالص(خطأ),
                                               جديد AstLogNot{flp, sampledCondp}};
                } وإلا إذا (rejectBasep) {
                    notPMatchp = rejectBasep->clone_ونه_خالص(خطأ);
                }
                إذا (throughoutRejectp)
                    notPMatchp = orExprs(flp, notPMatchp, throughoutRejectp->clone_ونه_خالص(خطأ));
                إذا (required_ګام_Rejectp)
                    notPMatchp
                        = orExprs(flp, notPMatchp, required_ګام_Rejectp->clone_ونه_خالص(خطأ));
                *بهر_Matchpp = notPMatchp;
            }
            إذا (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_ونه(), throughoutRejectp);
            إذا (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_ونه(), rejectBasep);
            إذا (required_ګام_Rejectp)
                VL_DO_DANGLING(required_ګام_Rejectp->delete_ونه(), required_ګام_Rejectp);
            Ast_غوټه_بیان* ثابت پایله_Exprp = جديد AstLogNot{flp, matchp};
            أرجع پایله_Exprp;
        }
        إذا (isCover) {
            إذا (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_ونه(), throughoutRejectp);
            إذا (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_ونه(), rejectBasep);
            إذا (required_ګام_Rejectp)
                VL_DO_DANGLING(required_ګام_Rejectp->delete_ونه(), required_ګام_Rejectp);
            إذا (سمون_Condp) {
                Ast_غوټه_بیان* ثابت sampledCondp = sampled(سمون_Condp->clone_ونه_خالص(خطأ));
                أرجع جديد AstLogAnd{flp, terminalActivep, sampledCondp};
            }
            أرجع terminalActivep;
        }
        // Assert/assume: output = !reject
        Ast_غوټه_بیان* rejectp = تش_اشاره;
        إذا (سمون_Condp && rejectBasep) {
            Ast_غوټه_بیان* ثابت sampledCondp = sampled(سمون_Condp->clone_ونه_خالص(خطأ));
            rejectp = جديد AstLogAnd{flp, rejectBasep, جديد AstLogNot{flp, sampledCondp}};
        } وإلا إذا (rejectBasep) {
            VL_DO_DANGLING(rejectBasep->delete_ونه(), rejectBasep);
        }
        إذا (بهر_Matchpp) {
            Ast_غوټه_بیان* سمون_Exprp = terminalActivep->clone_ونه_خالص(خطأ);
            إذا (سمون_Condp) {
                Ast_غوټه_بیان* ثابت sp = sampled(سمون_Condp->clone_ونه_خالص(خطأ));
                سمون_Exprp = جديد AstLogAnd{flp, سمون_Exprp, sp};
            }
            *بهر_Matchpp = سمون_Exprp;
        }
        إذا (terminalActivep) VL_DO_DANGLING(terminalActivep->delete_ونه(), terminalActivep);
        إذا (throughoutRejectp) rejectp = orExprs(flp, rejectp, throughoutRejectp);
        إذا (required_ګام_Rejectp) rejectp = orExprs(flp, rejectp, required_ګام_Rejectp);
        إذا (!rejectp) أرجع جديد AstConst{flp, AstConst::بټ_True{}};
        Ast_غوټه_بیان* ثابت پایله_Exprp = جديد AstLogNot{flp, rejectp};
        أرجع پایله_Exprp;
    }

    څرګند SvaNfaLowering(Ast_غوټه_Module* modp)
        : m_modp{modp}
        , m_u32DTypep{modp->پیدا_کړه_Basic_DType(VBasicDTypeKwd::UINT32)} {}
    ~SvaNfaLowering() {
        V3Stats::جمع_کړه_Stat_مجموعه("Assertions, NFA delay ring edge visits", m_stat_Delay_Ring_څنډه_Visits);
    }

    // Lower NFA graph to synthesizable AstAlways blocks and raw result signals.
    // Links are combinational; Edges are registered (NBA).
    Signal_سېټ ښکتنی(Ast_غوټه_Cover_Or_Assert* ثابت assertp, Sva_ګراف& ګراف,
                    Ast_Sen_ونه* ثابت senTreep, Ast_غوټه_بیان* ثابت سمون_Condp,
                    Ast_غوټه_بیان* ثابت disableExprp, ثابت منطقي need_Thread_Fail_شمېر,
                    ثابت منطقي need_Throughout_Thread_Fail_شمېر) {
        File_کرښه* ثابت flp = assertp->fileline();
        AstCover* ثابت coverp = VN_CAST(assertp, Cover);
        ثابت منطقي isSeqEvent = coverp && coverp->isSeqEvent();
        ثابت VAssertType assertType
            = isSeqEvent ? VAssertType{VAssertType::دننی} : assertp->userType();
        ثابت VAssertDirectiveType directiveType
            = isSeqEvent ? VAssertDirectiveType{VAssertDirectiveType::دننی}
                         : assertp->directive();
        ثابت std::string بنسټ_Name = m_نومونه.get("");

        // Number vertices with sequential colors for array indexing.
        صحيح N = 0;
        لكل (V3_ګراف_راس& vtxr : ګراف.m_ګراف.vertices()) { vtxr.color(N++); }
        std::ویکټور<Sva_State_راس*> vtx(N, تش_اشاره);
        لكل (V3_ګراف_راس& vtxr : ګراف.m_ګراف.vertices()) {
            vtx[vtxr.color()] = static_cast<Sva_State_راس*>(&vtxr);
        }

        // Allocate per-vertex lowering data (stored via V3GraphVertex::userp()).
        std::ویکټور<Sva_راس_معلومات> راس_معلومات(N);
        لكل (صحيح i = 0; i < N; ++i) vtx[i]->userp(&راس_معلومات[i]);

        // Identify registered vertices (targets of clocked edges).
        لكل (صحيح i = 0; i < N; ++i) {
            لكل (ثابت V3_ګراف_څنډه& edger : vtx[i]->بهر_څنډې()) {
                ثابت Sva_Trans_څنډه& tedger = static_cast<ثابت Sva_Trans_څنډه&>(edger);
                ثابت صحيح منزل_Idx = tedger.منزل_Vtxp()->color();
                إذا (tedger.m_consumes_دوره && tedger.منزل_Vtxp() != ګراف.m_سمون_Vertexp
                    && !tedger.منزل_Vtxp()->m_isRejectSink) {
                    vtx[منزل_Idx]->datap()->needsReg = ریښتیا;
                }
            }
        }

        // A packed range ring can emit several live endpoints without needing per-slot counts.
        // Widen only a ring whose input can already carry multiplicity.
        std::ویکټور<منطقي> stateMayHaveMultiplicity(N);
        std::ویکټور<منطقي> ringNeedsMultiplicity(N);
        std::ویکټور<صحيح> multiplicityWork;
        ثابت منطقي شمېر_Matches = coverp && coverp->isCoverSeq() && !isSeqEvent;
        إذا (شمېر_Matches) {
            لكل (صحيح i = 0; i < N; ++i) {
                ثابت منطقي مضرب_Inputs = vtx[i]->دننه_څنډې().اندازه() > 1;
                ثابت منطقي isRing = vtx[i]->m_delay_Ring_اندازه;
                ringNeedsMultiplicity[i] = isRing && مضرب_Inputs;
                ثابت منطقي startsMultiplicity = isRing
                                                    ? !vtx[i]->m_isFixedDelayRing
                                                    : مضرب_Inputs && !vtx[i]->datap()->needsReg;
                إذا (startsMultiplicity) {
                    stateMayHaveMultiplicity[i] = ریښتیا;
                    multiplicityWork.ټېل_شا(i);
                }
            }
            لكل (اندازه_t wi = 0; wi < multiplicityWork.اندازه(); ++wi) {
                لكل (ثابت V3_ګراف_څنډه& edger : vtx[multiplicityWork[wi]]->بهر_څنډې()) {
                    ثابت Sva_Trans_څنډه& tedger = static_cast<ثابت Sva_Trans_څنډه&>(edger);
                    ثابت صحيح ti = tedger.منزل_Vtxp()->color();
                    ثابت منطقي isRing = vtx[ti]->m_delay_Ring_اندازه;
                    إذا (isRing) ringNeedsMultiplicity[ti] = ریښتیا;
                    // Match and reject sinks have no outgoing edges, so they stop naturally.
                    ثابت منطقي stopsMultiplicity = !isRing && vtx[ti]->datap()->needsReg;
                    إذا (stateMayHaveMultiplicity[ti] || stopsMultiplicity) { استمر; }
                    stateMayHaveMultiplicity[ti] = ریښتیا;
                    multiplicityWork.ټېل_شا(ti);
                }
            }
        }

        ثابت منطقي asyncDisable = isAsyncDisable(disableExprp);
        // Registers cleared on a rising disable are also written by their clocked block.
        File_کرښه* ثابت regFlp = asyncDisable ? جديد File_کرښه{flp} : flp;
        إذا (asyncDisable) regFlp->warnOff(V3_Error_کوډ::MULTIDRIVEN, ریښتیا);

        AstVar* ثابت killVarp
            = جديد AstVar{flp, VVarType::MODULETEMP, بنسټ_Name + "__kill", m_u32DTypep};
        killVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->جمع_کړه_Stmtsp(killVarp);
        لكل (صحيح i = 0; i < N; ++i) {
            إذا (vtx[i]->m_isAndCombiner) {
                ثابت std::string بنسټ = بنسټ_Name + "__a" + std::منزل_string(i);
                AstVar* ثابت lp = جديد AstVar{regFlp, VVarType::MODULETEMP, بنسټ + "_doneL",
                                              m_modp->پیدا_کړه_بټ_DType()};
                lp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جمع_کړه_Stmtsp(lp);
                vtx[i]->datap()->بشپړ_LVarp = lp;
                AstVar* ثابت rp = جديد AstVar{regFlp, VVarType::MODULETEMP, بنسټ + "_doneR",
                                              m_modp->پیدا_کړه_بټ_DType()};
                rp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جمع_کړه_Stmtsp(rp);
                vtx[i]->datap()->بشپړ_RVarp = rp;
                استمر;
            }
            إذا (vtx[i]->m_delay_Ring_اندازه) {
                ثابت std::string بنسټ = بنسټ_Name + "__d" + std::منزل_string(i);
                ثابت منطقي storesCounts = شمېر_Matches && ringNeedsMultiplicity[i];
                Ast_غوټه_DType* ringDTypep = تش_اشاره;
                إذا (storesCounts) {
                    // int unsigned ring [size];
                    ringDTypep = جديد AstUnpackArrayDType{
                        flp, m_u32DTypep,
                        جديد AstRange{flp, static_cast<صحيح>(vtx[i]->m_delay_Ring_اندازه - 1), 0}};
                    v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(ringDTypep);
                } وإلا {
                    // bit [size-1:0] ring;
                    ringDTypep = m_modp->پیدا_کړه_بټ_DType(vtx[i]->m_delay_Ring_اندازه,
                                                      vtx[i]->m_delay_Ring_اندازه, VSigning::UNSIGNED);
                }
                AstVar* ثابت ringp
                    = جديد AstVar{flp, VVarType::MODULETEMP, بنسټ + "_ring", ringDTypep};
                ringp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جمع_کړه_Stmtsp(ringp);
                vtx[i]->datap()->delayRingVarp = ringp;
                vtx[i]->datap()->delayRingCounts = storesCounts;
                AstVar* ثابت headp
                    = جديد AstVar{flp, VVarType::MODULETEMP, بنسټ + "_head",
                                 storesCounts ? m_u32DTypep : m_modp->پیدا_کړه_بټ_DType()};
                headp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جمع_کړه_Stmtsp(headp);
                vtx[i]->datap()->delay_Ring_سر_Varp = headp;
                // int unsigned idx;
                AstVar* ثابت idxp
                    = جديد AstVar{regFlp, VVarType::MODULETEMP, بنسټ + "_idx", m_u32DTypep};
                idxp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جمع_کړه_Stmtsp(idxp);
                vtx[i]->datap()->delayRingIdxVarp = idxp;
                // int unsigned live_count;
                AstVar* ثابت ژوندی_شمېر_Varp
                    = جديد AstVar{regFlp, VVarType::MODULETEMP, بنسټ + "_liveCount", m_u32DTypep};
                ژوندی_شمېر_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جمع_کړه_Stmtsp(ژوندی_شمېر_Varp);
                vtx[i]->datap()->delay_Ring_ژوندی_شمېر_Varp = ژوندی_شمېر_Varp;
                AstVar* ثابت wrappedp = جديد AstVar{regFlp, VVarType::MODULETEMP,
                                                    بنسټ + "_wrapped", m_modp->پیدا_کړه_بټ_DType()};
                wrappedp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جمع_کړه_Stmtsp(wrappedp);
                vtx[i]->datap()->delayRingWrappedVarp = wrappedp;
                استمر;
            }
            إذا (!vtx[i]->datap()->needsReg) استمر;
            إذا (vtx[i] == ګراف.m_پیل_Vertexp || vtx[i]->m_is_سمون) استمر;
            ثابت std::string varName = بنسټ_Name + "__s" + std::منزل_string(i);
            AstVar* ثابت varp
                = جديد AstVar{regFlp, VVarType::MODULETEMP, varName, m_modp->پیدا_کړه_بټ_DType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جمع_کړه_Stmtsp(varp);
            vtx[i]->datap()->stateVarp = varp;
        }

        // Build lowering context for phase sub-functions.
        ښکتنی_Ctx c{flp,        senTreep,      disableExprp, سمون_Condp,
                   assertType, directiveType, killVarp,     ګراف};

        // Phase 1: Resolve combinational Links via fixed-point propagation.
        Ast_غوټه_بیان* ثابت triggerExprp
            = isSeqEvent ? جديد AstConst{flp, AstConst::بټ_True{}}
                         : assertOnCond(flp, assertp->userType(), assertp->directive());
        resolveLinks(c, triggerExprp, شمېر_Matches);
        VL_DO_DANGLING(triggerExprp->delete_ونه(), triggerExprp);

        // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
        emit_State_ثبتوونکی_Nba(c);
        emitDelayRingUpdates(c, شمېر_Matches);
        emit_And_Combiner_بشپړ_Latch_Nba(c);
        emitKillAckNba(c);

        // Phase 3/3a/3b: Compute terminal match/reject signals (cleans up stateSig).
        ثابت Signal_سېټ sigs
            = compute_سیګنالونه(c, need_Thread_Fail_شمېر, need_Throughout_Thread_Fail_شمېر, شمېر_Matches);

        // Strong s_always[m:n] end-of-simulation liveness: if any in-window state
        // is still set at $finish, the universal-quantifier window never completed
        // (IEEE 1800-2023 16.12.11 strong semantics). Fire the assertion failure
        // from a final block; V3Assert turns the DT_ERROR display into the standard
        // "Assertion failed in %m" message.
        // pending = |{strong state registers, strong delay live counts != 0};
        Ast_غوټه_بیان* pendingp = تش_اشاره;
        لكل (صحيح i = 0; i < N; ++i) {
            إذا (!vtx[i]->m_strongPending) استمر;
            Ast_غوټه_بیان* pendingExprp = تش_اشاره;
            إذا (vtx[i]->datap()->stateVarp) {
                pendingExprp = جديد AstVarRef{flp, vtx[i]->datap()->stateVarp, VAccess::READ};
            } وإلا إذا (vtx[i]->m_strongAlwaysRing) {
                pendingExprp
                    = delay_Ring_Has_ژوندی_Threadsp(flp, vtx[i]->datap()->delay_Ring_ژوندی_شمېر_Varp);
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
                = جديد AstCExpr{flp, AstCExpr::خالص{}, "vlSymsp->_vm_contextp__->assertOn()", 1};
            Ast_غوټه_بیان* ثابت condp = جديد AstLogAnd{flp, assertOnp, pendingp};
            AstDisplay* ثابت dispp
                = جديد AstDisplay{flp, VDisplayType::DT_ERROR, "", تش_اشاره, تش_اشاره};
            dispp->fmtp()->timeunit(m_modp->timeunit());
            Ast_غوټه_Stmt* ثابت firep = dispp;
            إذا (v3Global.opt.stopFail()) firep->جمع_کړه_بل(جديد AstStop{flp, خطأ});
            m_modp->جمع_کړه_Stmtsp(جديد AstFinal{flp, جديد AstIf{flp, condp, firep}});
        }

        // Clear userp on every vertex before vertexData is destroyed.
        لكل (صحيح i = 0; i < N; ++i) vtx[i]->userp(تش_اشاره);
        أرجع sigs;
    }
};

}  // namespace

//######################################################################
// Top-level visitor

صنف AssertNfaVisitor final : عام VNVisitor {
    // STATE
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Current module being processed
    AstClocking* m_defaultClockingp = تش_اشاره;  // Default clocking
    AstDefaultDisable* m_defaultDisablep = تش_اشاره;  // Default disable iff
    SvaNfaLowering* m_loweringp = تش_اشاره;  // NFA-to-hardware lowering engine
    Ast_Sen_ونه* m_sampled_ارزښت_Clockp = تش_اشاره;  // Inherited clock during scoped attachment
    V3_Unique_نومونه m_prop_Var_نومونه{"__Vpropvar"};  // Property-local variable names
    V3_Unique_نومونه m_prop_لنډمهاله_نومونه{"__VnfaSampled"};  // Hoisted $sampled(propp) temps
    V3_Unique_نومونه m_fail_شمېر_نومونه{"__VnfaRemainingFailCount"};  // Fail replay counter names
    std::سېټ<ثابت AstProperty*> m_inliningProps;  // Recursion guard for inlineNamedProperty

    کينډۍ <ډول_نوم T_غوټه>
    فراغ لیدنه_Sampled_ارزښت(T_غوټه* ثابت nodep) {
        إذا (m_sampled_ارزښت_Clockp && !nodep->sentreep()) {
            nodep->sentreep(m_sampled_ارزښت_Clockp->clone_ونه(ریښتیا));
        }
        iterateChildren(nodep);
    }

    // Wire match vertex and mid-window sources for a successful NFA build.
    ساكن فراغ wire_سمون_And_Mid_Sources(Sva_ګراف& ګراف, ثابت Build_پایله& پایله, File_کرښه* flp) {
        ګراف.create_سمون_راس();
        // Skip the main term Link when midSources already cover every
        // end-of-match (cover_sequence path); otherwise the per-mid extraction
        // double-counts via the merge vertex.
        إذا (!پایله.غړی_Is_Mid_یوځای) { ګراف.جمع_کړه_Link(پایله.غړی_Vertexp, ګراف.m_سمون_Vertexp); }
        لكل (Sva_State_راس* srcVtxp : پایله.midSources) {
            Ast_غوټه_بیان* condp = تش_اشاره;
            لكل (Ast_غوټه_بیان* ثابت tc : srcVtxp->m_throughoutConds) {
                Ast_غوټه_بیان* ثابت tcClone = tc->clone_ونه_خالص(خطأ);
                condp = condp ? جديد AstLogAnd{flp, condp, tcClone} : tcClone;
            }
            ګراف.جمع_کړه_Link(srcVtxp, ګراف.m_سمون_Vertexp, condp);
            srcVtxp->m_isUnbounded = ریښتیا;
        }
    }

    ساكن Ast_غوټه_بیان* getSequenceBodyExprp(ثابت AstSequence* seqp) {
        Ast_غوټه* bodyp = seqp->stmtsp();
        طالما (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        أرجع VN_CAST(bodyp, غوټه_بیان);
    }

    ساكن AstPropSpec* getPropertySpecp(ثابت AstProperty* propp) {
        Ast_غوټه* stmtp = propp->stmtsp();
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
        إذا (!m_inliningProps.emplace(propyp).دوهم) {
            funcrefp->v3error("Illegal recursive property reference");  // LCOV_EXCL_LINE
            أرجع;  // LCOV_EXCL_LINE
        }
        بنية Guard final {
            std::سېټ<ثابت AstProperty*>& setr;
            ثابت AstProperty* keyp;
            ~Guard() { setr.erase(keyp); }
        } guard{m_inliningProps, propyp};
        AstPropSpec* propSpecp = getPropertySpecp(propyp);
        UASSERT_OBJ(propSpecp, funcrefp, "Property has no body PropSpec");
        propSpecp = propSpecp->clone_ونه(خطأ);

        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propyp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_غوټه_بیان*> پورټ_Map;
        لكل (ثابت خپلکاری& tconnect : tconnects) {
            پورټ_Map[tconnect.لومړی] = tconnect.دوهم->exprp();
        }

        // Promote property-local variables to module-level temps (IEEE 16.10).
        std::unordered_map<ثابت AstVar*, AstVar*> localVarMap;
        لكل (Ast_غوټه* stmtp = propyp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (!varp->isIO()) {
                    ثابت string newName = m_prop_Var_نومونه.get(varp);
                    AstVar* ثابت newVarp = جديد AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->جمع_کړه_Stmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        propSpecp->foreach([&](AstVarRef* refp) {
            ثابت خپلکاری پورټ_It = پورټ_Map.پیدا_کړه(refp->varp());
            إذا (پورټ_It != پورټ_Map.پای_ټکی()) {
                refp->replaceWith(پورټ_It->دوهم->clone_ونه(خطأ));
                VL_DO_DANGLING(ټېل_Deletep(refp), refp);
                أرجع;
            }
            ثابت خپلکاری localIt = localVarMap.پیدا_کړه(refp->varp());
            إذا (localIt != localVarMap.پای_ټکی()) refp->varp(localIt->دوهم);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        لكل (ثابت خپلکاری& tconnect : tconnects) {
            ټېل_Deletep(tconnect.دوهم->exprp()->unlink_Fr_شا());
        }
        V3AssertCommon::checkSvaInstanceDelays(propSpecp, funcrefp);

        // Merge disable iff (IEEE 1800-2023 16.12.1)
        إذا (outerSpecp->disablep() && propSpecp->disablep()) {
            outerSpecp->v3error("disable iff expression before property call "
                                "and in its body is not legal");
            ټېل_Deletep(propSpecp->disablep()->unlink_Fr_شا());
        }
        إذا (outerSpecp->disablep()) {
            propSpecp->disablep(outerSpecp->disablep()->unlink_Fr_شا());
        }

        إذا (outerSpecp->sensesp() && propSpecp->sensesp()) {
            outerSpecp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Clock event before property call and in its body");
            ټېل_Deletep(propSpecp->sensesp()->unlink_Fr_شا());
        }
        إذا (outerSpecp->sensesp()) {
            Ast_Sen_توکی* ثابت sensesp = outerSpecp->sensesp();
            sensesp->unlink_Fr_شا();
            propSpecp->sensesp(sensesp);
        }

        outerSpecp->replaceWith(propSpecp);
        VL_DO_DANGLING(ټېل_Deletep(outerSpecp), outerSpecp);
    }

    فراغ inlineSequenceRef(AstFuncRef* funcrefp, AstSequence* seqp) {
        Ast_غوټه_بیان* ثابت bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        Ast_غوټه_بیان* ثابت clonedp = bodyExprp->clone_ونه(خطأ);

        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_غوټه_بیان*> پورټ_Map;
        لكل (ثابت خپلکاری& tconnect : tconnects) {
            پورټ_Map[tconnect.لومړی] = tconnect.دوهم->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ثابت خپلکاری it = پورټ_Map.پیدا_کړه(refp->varp());
            إذا (it != پورټ_Map.پای_ټکی()) {
                refp->replaceWith(it->دوهم->clone_ونه(خطأ));
                VL_DO_DANGLING(ټېل_Deletep(refp), refp);
            }
        });
        لكل (ثابت خپلکاری& tconnect : tconnects) {
            ټېل_Deletep(tconnect.دوهم->exprp()->unlink_Fr_شا());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ټېل_Deletep(funcrefp), funcrefp);
        // Clear referenced flag so V3AssertPre cleanup does not emit
        // spurious UNSUPPORTED for sequences that were already inlined here.
        seqp->isReferenced(خطأ);
    }

    // Must run before hasMultiCycleExpr() so NFA sees sequence bodies.
    فراغ inlineAllSequenceRefs(Ast_غوټه* rootp) {
        منطقي بدل_شوی = ریښتیا;
        طالما (بدل_شوی) {
            بدل_شوی = خطأ;
            rootp->foreach([&](AstFuncRef* funcrefp) {
                إذا (بدل_شوی) أرجع;
                إذا (AstSequence* ثابت seqp = VN_CAST(funcrefp->taskp(), Sequence)) {
                    inlineSequenceRef(funcrefp, seqp);
                    بدل_شوی = ریښتیا;
                }
            });
        }
    }

    ساكن منطقي has_Multi_دوره_بیان(ثابت Ast_غوټه* nodep) {
        أرجع nodep->exists([](ثابت Ast_غوټه* np) {
            إذا (ثابت خپلکاری* ثابت ep = VN_CAST(np, غوټه_بیان)) أرجع ep->is_Multi_دوره_Sva();
            أرجع خطأ;
        });
    }

    ساكن VPropStrength effectiveAssertPropStrength(ثابت AstPropSpec* ثابت propSpecp) {
        إذا (propSpecp->propStrength() != VPropStrength::DEFAULT) أرجع propSpecp->propStrength();
        أرجع propSpecp->fileline()->language() <= V3_Lang_کوډ::L1800_2005 ? VPropStrength::STRONG
                                                                           : VPropStrength::WEAK;
    }

    // Bare `assert property (p until q)` with boolean operands stays on
    // V3AssertPre's AstLoop lowering, which preserves per-attempt action-block
    // firings that this NFA's single-bit aggregated state cannot. Strong bare
    // forms are also lowered there. NFA still owns sequence operands and any
    // embedding inside a multi-cycle context (implication consequent, or/and
    // operands, etc.).
    ساكن منطقي is_Bare_سر_Level_Until(Ast_غوټه* propp) {
        Ast_غوټه* p = propp;
        إذا (AstPropSpec* ثابت specp = VN_CAST(p, PropSpec)) p = specp->propp();
        طالما (AstLogNot* ثابت notp = VN_CAST(p, LogNot)) p = notp->lhsp();
        AstUntil* ثابت untilp = VN_CAST(p, Until);
        إذا (!untilp) أرجع خطأ;
        ثابت خپلکاری hasSeq = [](ثابت Ast_غوټه_بیان* ep) {
            أرجع ep->exists([](ثابت Ast_غوټه_بیان* np) { أرجع np->is_Multi_دوره_Sva(); });
        };
        إذا (hasSeq(untilp->lhsp()) || hasSeq(untilp->rhsp())) أرجع خطأ;
        أرجع ریښتیا;
    }

    بنية PropertyParts final {
        Ast_غوټه_بیان* triggerExprp = تش_اشاره;
        Ast_غوټه_بیان* seqExprp = تش_اشاره;
        منطقي isOverlapped = ریښتیا;
        منطقي hasImplication = خطأ;
        منطقي isFollowedBy = خطأ;  // True for #-# / #=# (non-vacuous-fail on antecedent miss)
    };

    ساكن PropertyParts decomposeProperty(Ast_غوټه* propp) {
        PropertyParts parts;
        إذا (AstPropSpec* ثابت specp = VN_CAST(propp, PropSpec)) { propp = specp->propp(); }
        إذا (AstImplication* ثابت implp = VN_CAST(propp, Implication)) {
            parts.hasImplication = ریښتیا;
            parts.isOverlapped = implp->isOverlapped();
            parts.isFollowedBy = implp->isFollowedBy();
            parts.triggerExprp = implp->lhsp();
            parts.seqExprp = implp->rhsp();
        } وإلا إذا (Ast_غوټه_بیان* ثابت exprp = VN_CAST(propp, غوټه_بیان)) {
            parts.triggerExprp = تش_اشاره;
            parts.seqExprp = exprp;
        }
        أرجع parts;
    }

    ساكن منطقي can_وېشل_Implication_Pass_Actions(ثابت PropertyParts& parts) {
        UASSERT(parts.hasImplication,
                "Implication pass action split requested without implication");
        UASSERT(parts.triggerExprp, "Implication pass action split requested without trigger");
        // Direct vacuous/nonvacuous classification uses the antecedent value in the current
        // assertion attempt. Leave delayed antecedents on the existing NFA pass path.
        أرجع !has_Multi_دوره_بیان(parts.triggerExprp);
    }

    ساكن فراغ وېشل_Implication_Pass_Actions(AstAssert* assertp, ثابت PropertyParts& parts,
                                            Ast_غوټه_بیان* nonvacuousMatchp) {
        File_کرښه* ثابت flp = assertp->fileline();

        Ast_غوټه* ثابت passsp = assertp->passsp()->unlink_Fr_شا_With_بل();
        Ast_غوټه* splitsp = تش_اشاره;

        إذا (!parts.isFollowedBy) {
            Ast_غوټه_بیان* ثابت vacuousp
                = جديد AstLogNot{flp, sampled(parts.triggerExprp->clone_ونه_خالص(خطأ))};
            Ast_غوټه* ثابت vacuousBodyp = passsp->clone_ونه(خطأ);
            splitsp = newPassOnIf(flp, vacuousp, vacuousBodyp, assertp->userType(),
                                  assertp->directive(), /*vacuous=*/ریښتیا);
        }

        AstIf* ثابت nonvacuousp
            = newPassOnIf(flp, nonvacuousMatchp, passsp, assertp->userType(), assertp->directive(),
                          /*vacuous=*/خطأ);
        splitsp = splitsp ? Ast_غوټه::جمع_کړه_بل<Ast_غوټه, Ast_غوټه>(splitsp, nonvacuousp)
                          : static_cast<Ast_غوټه*>(nonvacuousp);
        assertp->جمع_کړه_Passsp(splitsp);
    }

    // On a PropSpec-wrapped assertion whose NFA build failed with a semantic
    // error (errorEmitted), replace the body with a BitFalse const so later
    // passes see a well-formed AST. Returns true if replaced.
    فراغ replaceBodyOnBuildError(File_کرښه* flp, AstPropSpec* propSpecp, منطقي errorEmitted) {
        إذا (!errorEmitted) أرجع;
        Ast_غوټه* ثابت innerPropp = propSpecp->propp();
        innerPropp->replaceWith(جديد AstConst{flp, AstConst::بټ_False{}});
        VL_DO_DANGLING(ټېل_Deletep(innerPropp), innerPropp);
    }

    // Hoist a leading clocking event (IEEE 1800-2023 16.7):
    منطقي hoistClockedSeq(AstPropSpec* specp) {
        طالما (AstSClocked* ثابت clockedp = VN_CAST(specp->propp(), SClocked)) {
            إذا (specp->sensesp()) {
                clockedp->v3warn(E_UNSUPPORTED, "Unsupported: multiclocked sequence or property");
                replaceBodyOnBuildError(specp->fileline(), specp, ریښتیا);
                أرجع ریښتیا;
            }
            لكل (ثابت Ast_Sen_توکی* sp = clockedp->sensesp(); sp;
                 sp = VN_CAST(sp->nextp(), Sen_توکی)) {
                إذا (!sp->څنډه_Type().an_څنډه()) {
                    clockedp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: non-edge clocking event on a sequence; "
                                     "use an edge such as @(posedge clk)");
                    replaceBodyOnBuildError(specp->fileline(), specp, ریښتیا);
                    أرجع ریښتیا;
                }
            }
            specp->sensesp(clockedp->sensesp()->unlink_Fr_شا_With_بل());
            Ast_غوټه_بیان* ثابت bodyp = clockedp->exprp()->unlink_Fr_شا();
            clockedp->replaceWith(bodyp);
            VL_DO_DANGLING(ټېل_Deletep(clockedp), clockedp);
        }
        // A clocking event anywhere else in the sequence is not supported.
        ثابت AstSClocked* nestedp = تش_اشاره;
        specp->propp()->foreach([&](ثابت AstSClocked* p) {
            إذا (!nestedp) nestedp = p;
        });
        إذا (nestedp) {
            nestedp->v3warn(E_UNSUPPORTED,
                            "Unsupported: clocking event inside sequence expression");
            replaceBodyOnBuildError(specp->fileline(), specp, ریښتیا);
            أرجع ریښتیا;
        }
        أرجع خطأ;
    }

    // Build the NFA graph for a property body, handling both the antecedent
    // |-> consequent and simple sequence cases. Returns the consequent/body
    // BuildResult (invalid on parse/build failure).
    Build_پایله build_Assertion_ګراف(SvaNfaBuilder& builder, Sva_ګراف& ګراف, Ast_غوټه_بیان* seqBodyp,
                                    ثابت PropertyParts& parts, File_کرښه* flp) {
        إذا (!parts.hasImplication) أرجع builder.build(seqBodyp);

        ګراف.m_پیل_Vertexp = ګراف.create_State_راس();
        أرجع builder.build_Implication_څنډې(parts.triggerExprp, seqBodyp, ګراف.m_پیل_Vertexp,
                                             parts.isOverlapped, parts.isFollowedBy,
                                             parts.triggerExprp, flp);
    }

    // Install pass-action gating and replay simultaneous per-thread failures.
    فراغ attachActionHandlers(AstAssert* ثابت assertp, Ast_غوټه_بیان* سمون_Exprp,
                              Ast_Sen_ونه* ثابت threadFailReplaySenTreep,
                              Ast_غوټه_بیان* ثابت threadFailCountp) {
        // Gate pass handler on match to prevent vacuous-pass firings.
        إذا (سمون_Exprp) {
            // needMatch implies passsp() was non-null when evaluated above;
            // lowering does not mutate the assert's pass-action between the
            // two reads, so passsp() is still non-null here.
            Ast_غوټه* passsp = assertp->passsp();
            UASSERT_OBJ(passsp, assertp, "needMatch set but passsp is null");
            passsp->unlink_Fr_شا_With_بل();
            File_کرښه* ثابت flp = assertp->fileline();
            AstIf* ثابت ifp = جديد AstIf{flp, سمون_Exprp, passsp, تش_اشاره};
            assertp->جمع_کړه_Passsp(ifp);
            // Fail-handler prefix for overlapping instances (IEEE 16.12):
            // fires when reject=1 && match=1 in the same cycle.
            إذا (assertp->failsp()) assertp->prependFailsp(ifp->clone_ونه(خطأ));
        }

        إذا (threadFailCountp) {
            UASSERT_OBJ(threadFailReplaySenTreep, assertp,
                        "Thread fail count missing sensitivity tree");
            Ast_غوټه* ثابت failsp = assertp->failsp();
            File_کرښه* ثابت flp = assertp->fileline();
            // IEEE 1800-2023 16.12 requires one action-block evaluation per failed
            // thread. AstAssert handles the first, so replay the rest here.
            AstVar* ثابت پاتې_شوی_Fail_شمېر_Varp
                = جديد AstVar{flp, VVarType::MODULETEMP, m_fail_شمېر_نومونه.get(""),
                             m_modp->پیدا_کړه_Basic_DType(VBasicDTypeKwd::UINT32)};
            پاتې_شوی_Fail_شمېر_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جمع_کړه_Stmtsp(پاتې_شوی_Fail_شمېر_Varp);
            Ast_غوټه* ثابت replayStmtsp = جديد Ast_ټاکل{
                flp, جديد AstVarRef{flp, پاتې_شوی_Fail_شمېر_Varp, VAccess::WRITE}, threadFailCountp};
            AstLoop* ثابت replayLoopp = جديد AstLoop{flp};
            replayLoopp->جمع_کړه_Stmtsp(جديد AstLoopTest{
                flp, replayLoopp,
                جديد AstGt{flp, جديد AstVarRef{flp, پاتې_شوی_Fail_شمېر_Varp, VAccess::READ},
                          newTypedConstp(flp, پاتې_شوی_Fail_شمېر_Varp->dtypep(), 1)}});
            replayLoopp->جمع_کړه_Stmtsp(newIfAssertFailOn(failsp->clone_ونه(ریښتیا), assertp->directive(),
                                                     assertp->userType()));
            AstSub* ثابت decrementedFailCountp
                = جديد AstSub{flp, جديد AstVarRef{flp, پاتې_شوی_Fail_شمېر_Varp, VAccess::READ},
                             newTypedConstp(flp, پاتې_شوی_Fail_شمېر_Varp->dtypep(), 1)};
            replayLoopp->جمع_کړه_Stmtsp(
                جديد Ast_ټاکل{flp, جديد AstVarRef{flp, پاتې_شوی_Fail_شمېر_Varp, VAccess::WRITE},
                              decrementedFailCountp});
            replayStmtsp->جمع_کړه_بل(replayLoopp);
            m_modp->جمع_کړه_Stmtsp(
                جديد AstAlways{flp, VAlwaysKwd::ALWAYS, threadFailReplaySenTreep, replayStmtsp});
        }
    }

    // Replace one VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). No-op if refp is not in matchMap.
    فراغ substitute_سمون_توکی_Ref(AstVarRef* refp, بینښه K,
                                ثابت std::unordered_map<ثابت AstVar*, Ast_غوټه_بیان*>& سمون_Map) {
        ثابت خپلکاری it = سمون_Map.پیدا_کړه(refp->varp());
        إذا (it == سمون_Map.پای_ټکی()) أرجع;
        Ast_غوټه_بیان* newp = it->دوهم->clone_ونه_خالص(خطأ);
        إذا (K > 0) {
            AstConst* ثابت ticksp = جديد AstConst{refp->fileline(), AstConst::Widthed_ارزښت{}, 32,
                                                  static_cast<uint32_t>(K)};
            AstPast* ثابت pastp = جديد AstPast{refp->fileline(), newp, ticksp, تش_اشاره,
                                               /* propertyTiming */ ریښتیا};
            pastp->dtypeFrom(newp);
            newp = pastp;
        }
        refp->replaceWith(newp);
        VL_DO_DANGLING(ټېل_Deletep(refp), refp);
        أرجع;
    }

    // Recursively walk a consequent. Returns cycle length consumed and
    // substitutes each VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). Reports E_UNSUPPORTED on non-constant
    // delays or composite sequence operators and returns -1 on failure.
    صحيح
    walk_Substitute_سمون_توکي(Ast_غوټه_بیان* nodep, بینښه K,
                             ثابت std::unordered_map<ثابت AstVar*, Ast_غوټه_بیان*>& سمون_توکي) {
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            // IEEE 1800-2023 16.9.2: cycle_delay's lhsp is a constant_expression
            // and the delay form in a sequence is always `##N`, folded by
            // V3Const + V3Param before V3AssertNfa. Range form `##[m:n]` is the
            // only user-visible reject here.
            AstDelay* ثابت delayp = VN_AS(sexprp->delayp(), Delay);
            UASSERT_OBJ(delayp->is_دوره_Delay() && VN_IS(delayp->lhsp(), Const), sexprp,
                        "SVA cycle delay must have a constant lhsp");
            إذا (delayp->isRangeDelay()) {
                sexprp->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                              "non-constant cycle delay in consequent"
                                              " (IEEE 1800-2023 16.10)");
                أرجع -1;
            }
            ثابت بینښه delay_دورې = VN_AS(delayp->lhsp(), Const)->منزل_UInt();
            صحيح preLen = 0;
            إذا (Ast_غوټه_بیان* ثابت prep = sexprp->preExprp()) {
                preLen = walk_Substitute_سمون_توکي(prep, K, سمون_توکي);
                إذا (preLen < 0) أرجع -1;
            }
            ثابت صحيح bodyLen
                = walk_Substitute_سمون_توکي(sexprp->exprp(), K + preLen + delay_دورې, سمون_توکي);
            إذا (bodyLen < 0) أرجع -1;
            أرجع preLen + delay_دورې + bodyLen;
        }
        إذا (nodep->is_Multi_دوره_Sva()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                         "composite sequence operator in consequent"
                                         " (IEEE 1800-2023 16.10)");
            أرجع -1;
        }
        std::ویکټور<AstVarRef*> refs;
        nodep->foreach([&refs](AstVarRef* p) { refs.ټېل_شا(p); });
        لكل (AstVarRef* ثابت refp : refs) substitute_سمون_توکی_Ref(refp, K, سمون_توکي);
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
    منطقي lift_سمون_توکی_Substitutions(PropertyParts& parts, Ast_غوټه_بیان* seqBodyp) {
        إذا (!parts.hasImplication) أرجع خطأ;
        Ast_بیان_Stmt* ثابت بیان_Stmtp = VN_CAST(parts.triggerExprp, بیان_Stmt);
        إذا (!بیان_Stmtp) أرجع خطأ;
        // IEEE 1800-2023 16.10 BNF requires `(expr, match_item {, match_item})`
        // with at least one match item; V3LinkParse only emits ExprStmt for
        // this form and only emits AstAssign with VarRef LHS for each item.
        std::unordered_map<ثابت AstVar*, Ast_غوټه_بیان*> سمون_توکي;
        لكل (Ast_غوټه* stmtp = بیان_Stmtp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_ټاکل* ثابت assignp = VN_AS(stmtp, ټاکل);
            AstVarRef* ثابت lhsRefp = VN_AS(assignp->lhsp(), VarRef);
            سمون_توکي[lhsRefp->varp()] = assignp->rhsp();
        }
        ثابت بینښه پیل_K = parts.isOverlapped ? 0 : 1;
        ثابت صحيح اوږدتیا = walk_Substitute_سمون_توکي(seqBodyp, پیل_K, سمون_توکي);
        // Match-item substitution / strip mutates ancestor purity. Release
        // builds don't auto-clear caches on edits, so refresh here.
        VIsCached::clear_Cache_ونه();
        إذا (اوږدتیا < 0) أرجع ریښتیا;
        Ast_غوټه_بیان* ثابت antBoolp = بیان_Stmtp->resultp()->unlink_Fr_شا();
        بیان_Stmtp->replaceWith(antBoolp);
        VL_DO_DANGLING(ټېل_Deletep(بیان_Stmtp), بیان_Stmtp);
        parts.triggerExprp = antBoolp;
        أرجع خطأ;
    }

    // Outcome counts for a property if/case are wrong in an outcome-multiplying
    // context. Returns that context, or nullptr when the shape is supported.
    ساكن ثابت حرف* unsupportedPropertyControl(ثابت Ast_غوټه_Cover_Or_Assert* assertp,
                                                  ثابت Ast_غوټه_بیان* seqBodyp, منطقي negated) {
        إذا (!hasPropertyControlConjunction(seqBodyp)) أرجع تش_اشاره;
        إذا (negated) أرجع "negation";
        إذا (VN_IS(assertp, Cover)) أرجع "cover";
        إذا (VN_AS(assertp, Assert)->passsp()) أرجع "a pass action";
        أرجع تش_اشاره;
    }

    // Inline property/sequence refs and reject unsupported shapes.
    // Returns the PropSpec to lower, or nullptr when fully handled here.
    AstPropSpec* prepareAssertionProp(Ast_غوټه_Cover_Or_Assert* assertp) {
        إذا (AstPropSpec* ثابت specp = VN_CAST(assertp->propp(), PropSpec)) {
            إذا (AstFuncRef* ثابت funcrefp = VN_CAST(specp->propp(), FuncRef)) {
                إذا (ثابت AstProperty* ثابت propyp = VN_CAST(funcrefp->taskp(), Property)) {
                    inlineNamedProperty(specp, funcrefp, propyp);
                }
            }
        }

        inlineAllSequenceRefs(assertp->propp());

        إذا (AstPropSpec* ثابت specp = VN_CAST(assertp->propp(), PropSpec)) {
            إذا (hoistClockedSeq(specp)) أرجع تش_اشاره;
        }

        AstPropSpec* ثابت propp = VN_AS(assertp->propp(), PropSpec);
        إذا (!VN_IS(assertp, Cover)
            && effectiveAssertPropStrength(propp) == VPropStrength::STRONG) {
            propp->v3warn(E_UNSUPPORTED,
                          "Unsupported: strong property in " + assertp->verilogKwd() + ".");
            replaceBodyOnBuildError(assertp->fileline(), propp, /*errorEmitted=*/ریښتیا);
            أرجع تش_اشاره;
        }

        إذا (!has_Multi_دوره_بیان(propp)) أرجع تش_اشاره;
        // A nested property instance keeps its body behind the call; lowering would drop it.
        إذا (propp->exists([](ثابت AstFuncRef* refp) { أرجع VN_IS(refp->taskp(), Property); })) {
            assertp->v3warn(E_UNSUPPORTED,
                            "Unsupported: property instance inside a multi-cycle property "
                            "expression");
            VL_DO_DANGLING(ټېل_Deletep(assertp->unlink_Fr_شا()), assertp);
            أرجع تش_اشاره;
        }
        إذا (is_Bare_سر_Level_Until(propp)) أرجع تش_اشاره;
        أرجع propp;
    }

    فراغ processAssertion(Ast_غوټه_Cover_Or_Assert* assertp) {
        إذا (assertp->سمدستي()) أرجع;

        AstPropSpec* ثابت propp = prepareAssertionProp(assertp);
        إذا (!propp) أرجع;

        PropertyParts parts = decomposeProperty(propp);
        UASSERT_OBJ(parts.seqExprp, propp, "Property body must be an expression");

        // Unwrap `not` (IEEE 1800-2023 16.12.1); odd count -> negated semantics.
        Ast_غوټه_بیان* seqBodyp = parts.seqExprp;
        منطقي negated = خطأ;
        طالما (AstLogNot* ثابت notp = VN_CAST(seqBodyp, LogNot)) {
            negated = !negated;
            seqBodyp = notp->lhsp();
        }

        ثابت حرف* ثابت propertyControlp
            = unsupportedPropertyControl(assertp, seqBodyp, negated);

        // Substitute property-local match-item refs in consequent with
        // $past(rhs, K) before NFA build (IEEE 1800-2023 16.10).
        إذا (lift_سمون_توکی_Substitutions(parts, seqBodyp)) {
            AstPropSpec* ثابت psp = VN_CAST(assertp->propp(), PropSpec);
            UASSERT_OBJ(psp, assertp, "Concurrent assertion must have PropSpec");
            replaceBodyOnBuildError(assertp->fileline(), psp, /*errorEmitted=*/ریښتیا);
            أرجع;
        }

        AstCover* ثابت coverp = VN_CAST(assertp, Cover);
        ثابت منطقي isCoverSeq = coverp && coverp->isCoverSeq();
        // A sequence event control is not an assertion directive; no default
        // disable iff, no assertion control
        ثابت منطقي isSeqEvent = coverp && coverp->isSeqEvent();
        ثابت منطقي شمېر_Matches = isCoverSeq && !isSeqEvent;
        // Inherit module defaults (IEEE 14.12, 16.15) when assertion has none.
        إذا (!propp->sensesp() && m_defaultClockingp) {
            propp->sensesp(m_defaultClockingp->sentreep()->sensesp()->clone_ونه(ریښتیا));
        }
        إذا (!propp->disablep() && m_defaultDisablep && !isSeqEvent) {
            propp->disablep(m_defaultDisablep->condp()->clone_ونه_خالص(ریښتیا));
        }
        إذا (!propp->sensesp()) أرجع;
        Ast_Sen_ونه* senTreep
            = جديد Ast_Sen_ونه{propp->fileline(), propp->sensesp()->clone_ونه(ریښتیا)};
        Ast_غوټه_بیان* ثابت disableExprp = propp->disablep();

        // NFA lowering clones repeated operands and may hoist them into an
        // always_comb block. Resolve implicit sampled-value clocks first, while
        // the enclosing assertion clock is still available.
        {
            VL_RESTORER(m_sampled_ارزښت_Clockp);
            m_sampled_ارزښت_Clockp = senTreep;
            iterate(propp->propp());
        }

        File_کرښه* ثابت flp = assertp->fileline();

        Sva_ګراف ګراف;
        SvaNfaBuilder builder{ګراف,      m_modp,     m_prop_لنډمهاله_نومونه,
                              isCoverSeq, isSeqEvent, coverp != تش_اشاره};

        ثابت Build_پایله پایله = build_Assertion_ګراف(builder, ګراف, seqBodyp, parts, flp);
        إذا (پایله.valid()) wire_سمون_And_Mid_Sources(ګراف, پایله, flp);
        إذا (!پایله.valid()) {
            // Fall through to V3AssertPre for unsupported constructs; only
            // replace the body on real semantic errors. Any hoisted temps
            // from this attempt become orphan MODULETEMPs; V3Dead removes
            // them along with the dead always_comb driver.
            replaceBodyOnBuildError(flp, propp, پایله.errorEmitted);
            VL_DO_DANGLING(ټېل_Deletep(senTreep), senTreep);
            أرجع;
        }
        // After the build, so a construct the builder rejects reports itself.
        إذا (propertyControlp) {
            seqBodyp->v3warn(E_UNSUPPORTED,
                             "Unsupported: temporal property if/case with " << propertyControlp);
            replaceBodyOnBuildError(flp, propp, /*errorEmitted=*/ریښتیا);
            VL_DO_DANGLING(ټېل_Deletep(senTreep), senTreep);
            أرجع;
        }
        إذا (شمېر_Matches && !v3Global.opt.coverageUser()) {
            SvaNfaBuilder::freeUnlinkedCondp(پایله.finalCondp);
            VL_DO_DANGLING(ټېل_Deletep(senTreep), senTreep);
            Ast_غوټه* ثابت innerPropp = propp->propp();
            innerPropp->replaceWith(جديد AstConst{flp, AstConst::بټ_False{}});
            VL_DO_DANGLING(ټېل_Deletep(innerPropp), innerPropp);
            أرجع;
        }

        AstAssert* ثابت assertAssertp = VN_CAST(assertp, Assert);
        ثابت منطقي وېشل_Implication_Passsp = assertAssertp && assertAssertp->passsp()
                                            && parts.hasImplication
                                            && can_وېشل_Implication_Pass_Actions(parts);
        ثابت منطقي need_سمون = assertAssertp && assertAssertp->passsp()
                               && (!parts.hasImplication || وېشل_Implication_Passsp);

        ثابت منطقي needsThreadFailReplay
            = assertAssertp && assertAssertp->failsp() && !parts.hasImplication;
        ثابت خپلکاری سیګنالونه
            = m_loweringp->ښکتنی(assertp, ګراف, senTreep, پایله.finalCondp, disableExprp,
                                 needsThreadFailReplay, needsThreadFailReplay && !negated);
        Ast_غوټه_بیان* سمون_Exprp = تش_اشاره;
        Ast_غوټه_بیان* outputExprp = m_loweringp->assemble_پایله(
            assertp, negated, پایله.finalCondp, سیګنالونه, need_سمون ? &سمون_Exprp : تش_اشاره);
        إذا (شمېر_Matches) {
            UASSERT_OBJ(سیګنالونه.سمون_Countp, coverp, "Cover sequence missing match count");
            VL_DO_DANGLING(outputExprp->delete_ونه(), outputExprp);
            propp->سمون_Countp(سیګنالونه.سمون_Countp);
            outputExprp = جديد AstNeq{flp, سیګنالونه.سمون_Countp->clone_ونه_خالص(خطأ),
                                     newTypedConstp(flp, سیګنالونه.سمون_Countp->dtypep(), 0)};
        }

        Ast_Sen_ونه* ثابت threadFailReplaySenTreep
            = سیګنالونه.threadFailCountp ? senTreep->clone_ونه(خطأ) : تش_اشاره;

        VL_DO_DANGLING(ټېل_Deletep(senTreep), senTreep);
        إذا (پایله.finalCondp && !پایله.finalCondp->backp()) ټېل_Deletep(پایله.finalCondp);

        إذا (وېشل_Implication_Passsp) {
            وېشل_Implication_Pass_Actions(assertAssertp, parts, سمون_Exprp);
        } وإلا {
            attachActionHandlers(assertAssertp, سمون_Exprp, threadFailReplaySenTreep,
                                 سیګنالونه.threadFailCountp);
        }

        Ast_غوټه* ثابت innerPropp = propp->propp();
        innerPropp->replaceWith(outputExprp);
        VL_DO_DANGLING(ټېل_Deletep(innerPropp), innerPropp);

        UINFO(4, "NFA converted assertion at " << flp << کرښه_پای);

        إذا (dump_ګراف_Level() >= 6) ګراف.m_ګراف.dumpDotFilePrefixed("assert-nfa");
    }

    // VISITORS
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_loweringp);
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultDisablep);
        m_modp = nodep;
        // A class nested in a module is in the scope of the module's default clocking
        إذا (!VN_IS(nodep, Class)) m_defaultClockingp = تش_اشاره;
        m_defaultDisablep = nodep->defaultDisablep();
        SvaNfaLowering lowering{nodep};
        m_loweringp = &lowering;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstClocking* nodep) override {
        إذا (nodep->isDefault() && !m_defaultClockingp) m_defaultClockingp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_Gen_بلاک* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstDefaultDisable* nodep) override {}
    فراغ لیدنه(AstFell* nodep) override { لیدنه_Sampled_ارزښت(nodep); }
    فراغ لیدنه(AstPast* nodep) override { لیدنه_Sampled_ارزښت(nodep); }
    فراغ لیدنه(AstRose* nodep) override { لیدنه_Sampled_ارزښت(nodep); }
    فراغ لیدنه(AstStable* nodep) override { لیدنه_Sampled_ارزښت(nodep); }
    فراغ لیدنه(AstAssert* nodep) override { processAssertion(nodep); }
    فراغ لیدنه(AstCover* nodep) override { processAssertion(nodep); }
    فراغ لیدنه(AstRestrict* nodep) override {
        // Restrict property is ignored by simulators (IEEE 1800-2023 16.12.2).
        // Remove here so temporal SExpr don't leak to V3AssertPre.
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }
    فراغ لیدنه(AstAssertIntrinsic* nodep) override {}
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    څرګند AssertNfaVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Top entry point

فراغ V3AssertNfa::assertNfaAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":" << کرښه_پای);
    { AssertNfaVisitor{nodep}; }
    V3Global::dump_Check_Global_ونه("assertnfa", 0, dump_ونه_Either_Level() >= 3);
}
