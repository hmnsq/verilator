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
    AstVar* stateVarp = خالي_اشارو;  // NBA state register for this vertex
    AstVar* delayRingVarp = خالي_اشارو;  // Packed occupancy bits or per-slot match counts
    AstVar* delay_Ring_سر_Varp
        = خالي_اشارو;  // Cached outgoing slot; isolates readers from in-place writes
    AstVar* delayRingIdxVarp = خالي_اشارو;  // Next slot written in delayRingVarp
    AstVar* delay_Ring_جيئرو_ڳڻپ_Varp = خالي_اشارو;  // Number of threads in delayRingVarp
    AstVar* delayRingWrappedVarp = خالي_اشارو;  // All slots written since the last clear
    AstVar* پورو_LVarp = خالي_اشارو;  // SAnd LHS done-latch
    AstVar* پورو_RVarp = خالي_اشارو;  // SAnd RHS done-latch
    Ast_ڳنڍ_اظهار* stateSigp = خالي_اشارو;  // Combinational state signal; OWNED during lowering
    Ast_ڳنڍ_اظهار* state_ڳڻپ_Sigp = خالي_اشارو;  // Cover-sequence match count; OWNED during lowering
    Ast_ڳنڍ_اظهار* state_ڳڻپ_حد_کان_مٿي_Sigp = خالي_اشارو;  // Count overflow; OWNED during lowering
    منطقي delayRingCounts = خطأ;  // Ring slots hold cover-sequence match counts
    منطقي needsReg = خطأ;  // True if vertex has incoming clocked edge
};

// NFA state vertex -- one per NFA position in the sequence evaluation
صنف Sva_State_راس final : عام V3_گراف_راس {
    VL_RTTI_IMPL(Sva_State_راس, V3_گراف_راس)
عام:
    // True if this is the sequence-match terminal vertex
    منطقي m_is_ميلاپ = خطأ;
    // OWNED throughout-guard condition clones; IEEE 1800-2023 16.9.9
    std::ويڪٽر<Ast_ڳنڍ_اظهار*> m_throughoutConds;
    // Nonzero for a ring-buffer vertex for ## delays.
    منطقي m_isFixedDelayRing = خطأ;
    بنانشان m_delay_Ring_سائيز = 0;  // Number of ring slots. Range: max-min+1.
    Ast_ڳنڍ_اظهار* m_delayRingClearCondp = خالي_اشارو;  // local RHS for pure-boolean range
    Ast_ڳنڍ_اظهار* m_delayRingAdvanceCondp = خالي_اشارو;  // Advance only when this condition holds
    منطقي m_replayAbortReject = خطأ;  // Compressed repetition needs per-thread abort replay
    // OWNED; enclosing-abort fire condition clearing state or suppressing guard rejection
    Ast_ڳنڍ_اظهار* m_abortClearp = خالي_اشارو;
    // OWNED; reject-abort fire condition rejecting all represented live threads
    Ast_ڳنڍ_اظهار* m_abortRejectp = خالي_اشارو;
    // Liveness terminal (IEEE weak semantics): reject must not fire from this source
    منطقي m_isUnbounded = خطأ;
    // Temporal sequence AND combiner; IEEE 1800-2023 16.9.5
    منطقي m_isAndCombiner = خطأ;
    Sva_State_راس* m_andLhsTermp = خالي_اشارو;  // LHS sub-NFA terminal vertex
    Sva_State_راس* m_andRhsTermp = خالي_اشارو;  // RHS sub-NFA terminal vertex
    Ast_ڳنڍ_اظهار* m_andLhsCondp = خالي_اشارو;  // OWNED; LHS final condition (may be nullptr)
    Ast_ڳنڍ_اظهار* m_andRhsCondp = خالي_اشارو;  // OWNED; RHS final condition (may be nullptr)
    // Reject sink for SAnd rejectOnFail wiring; not a state-signal source
    منطقي m_isRejectSink = خطأ;
    // In-window vertex of a strong s_always[m:n]: if its state is still set at
    // end-of-simulation the universal-quantifier window never completed, which is
    // a liveness failure (IEEE 1800-2023 16.12.11 strong semantics).
    منطقي m_strongPending = خطأ;
    // Ring introduced for the checked portion of a bounded strong s_always window.
    منطقي m_strongAlwaysRing = خطأ;

    // CONSTRUCTORS
    واضح Sva_State_راس(V3_گراف* graphp)
        : V3_گراف_راس{graphp} {}
    ~Sva_State_راس() override {
        لكل (Ast_ڳنڍ_اظهار* cp : m_throughoutConds) VL_DO_DANGLING(cp->delete_وڻ(), cp);
        إذا (m_delayRingClearCondp)
            VL_DO_DANGLING(m_delayRingClearCondp->delete_وڻ(), m_delayRingClearCondp);
        إذا (m_delayRingAdvanceCondp)
            VL_DO_DANGLING(m_delayRingAdvanceCondp->delete_وڻ(), m_delayRingAdvanceCondp);
        إذا (m_abortClearp) VL_DO_DANGLING(m_abortClearp->delete_وڻ(), m_abortClearp);
        إذا (m_abortRejectp) VL_DO_DANGLING(m_abortRejectp->delete_وڻ(), m_abortRejectp);
        إذا (m_andLhsCondp) VL_DO_DANGLING(m_andLhsCondp->delete_وڻ(), m_andLhsCondp);
        إذا (m_andRhsCondp) VL_DO_DANGLING(m_andRhsCondp->delete_وڻ(), m_andRhsCondp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string name() ثابت override {
        string name = "s" + cvt_منزل_Str(color());
        إذا (m_delay_Ring_سائيز) {
            name += "\\n";
            name += m_isFixedDelayRing ? "fixed chain " : "range chain ";
            name += cvt_منزل_Str(m_delay_Ring_سائيز) + " slots";
        }
        أرجع name;
    }
    string dotColor() ثابت override {
        إذا (m_is_ميلاپ) أرجع "red";
        إذا (m_delay_Ring_سائيز) أرجع "blue";
        إذا (m_isAndCombiner) أرجع "purple";
        أرجع "black";
    }
    // LCOV_EXCL_STOP
    // Access per-vertex algorithm data (valid only during lowering phase)
    Sva_راس_معلومات* datap() ثابت { أرجع static_cast<Sva_راس_معلومات*>(userp()); }
};

// NFA transition edge -- clocked (##1) or combinational link (##0)
صنف Sva_Trans_ڪنارو final : عام V3_گراف_ڪنارو {
    VL_RTTI_IMPL(Sva_Trans_ڪنارو, V3_گراف_ڪنارو)
عام:
    Ast_ڳنڍ_اظهار* m_condp;  // Transition condition; nullptr = unconditional; OWNED
    منطقي m_consumes_چڪر;  // true = clocked edge (##1), false = link (##0/boolean)
    // Reject when source is active and condp is false; set only on
    // outermost required-step Link
    منطقي m_rejectOnFail = خطأ;
    // Optional dynamic condition vertex for m_rejectOnFail. Used when the
    // success condition is another NFA state rather than a static expression.
    Sva_State_راس* m_condVtxp = خالي_اشارو;

    // CONSTRUCTORS
    Sva_Trans_ڪنارو(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٽي, Ast_ڳنڍ_اظهار* condp,
                 منطقي consumes_چڪر)
        : V3_گراف_ڪنارو{graphp, fromp, چوٽي, /*weight=*/1}
        , m_condp{condp}
        , m_consumes_چڪر{consumes_چڪر} {}
    ~Sva_Trans_ڪنارو() override {
        إذا (m_condp) VL_DO_DANGLING(m_condp->delete_وڻ(), m_condp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string dotLabel() ثابت override { أرجع m_consumes_چڪر ? "##1" : "link"; }
    string dotStyle() ثابت override { أرجع m_consumes_چڪر ? "" : "dashed"; }
    // LCOV_EXCL_STOP
    // Typed accessors for NFA vertices
    Sva_State_راس* fromVtxp() ثابت { أرجع static_cast<Sva_State_راس*>(fromp()); }
    Sva_State_راس* منزل_Vtxp() ثابت { أرجع static_cast<Sva_State_راس*>(چوٽي()); }
};

// NFA graph container
صنف Sva_گراف final {
عام:
    V3_گراف m_گراف;  // Owns all vertices and edges
    Sva_State_راس* m_شروعات_Vertexp = خالي_اشارو;  // Trigger/start vertex
    Sva_State_راس* m_ميلاپ_Vertexp = خالي_اشارو;  // Sequence-match terminal vertex

    // Create a new state vertex
    Sva_State_راس* create_State_راس() { أرجع جديد Sva_State_راس{&m_گراف}; }
    // Create the match terminal vertex
    Sva_State_راس* create_ميلاپ_راس() {
        Sva_State_راس* ثابت vtxp = create_State_راس();
        vtxp->m_is_ميلاپ = سچ;
        m_ميلاپ_Vertexp = vtxp;
        أرجع vtxp;
    }
    // Add a clocked transition edge (##1)
    Sva_Trans_ڪنارو* جوڙيو_Clocked_ڪنارو(Sva_State_راس* fromp, Sva_State_راس* چوٽي,
                                 Ast_ڳنڍ_اظهار* condp = خالي_اشارو) {
        أرجع جديد Sva_Trans_ڪنارو{&m_گراف, fromp, چوٽي, condp, /*consumesCycle=*/سچ};
    }
    // Add a combinational link (##0 / boolean condition)
    Sva_Trans_ڪنارو* جوڙيو_Link(Sva_State_راس* fromp, Sva_State_راس* چوٽي,
                          Ast_ڳنڍ_اظهار* condp = خالي_اشارو) {
        أرجع جديد Sva_Trans_ڪنارو{&m_گراف, fromp, چوٽي, condp, /*consumesCycle=*/خطأ};
    }
    // Collect all edges into a flat vector for iteration.
    // Used by the lowering phase which needs global edge scans.
    std::ويڪٽر<ثابت Sva_Trans_ڪنارو*> all_ڪنارا() ثابت {
        std::ويڪٽر<ثابت Sva_Trans_ڪنارو*> نتيجو;
        لكل (ثابت V3_گراف_راس& vtxr : m_گراف.vertices()) {
            لكل (ثابت V3_گراف_ڪنارو& edger : vtxr.ٻاهريون_ڪنارا()) {
                نتيجو.ڌڪو_پٺ(static_cast<ثابت Sva_Trans_ڪنارو*>(&edger));
            }
        }
        أرجع نتيجو;
    }
};

//######################################################################
// Builder result: terminal vertex + optional final condition (match Link condition).
بنية Build_نتيجو final {
    Sva_State_راس* رڪن_Vertexp;  // Primary terminal; contributes to both match and reject
    Ast_ڳنڍ_اظهار* finalCondp;  // nullptr = unconditional
    // Mid-window sources for range delays (pure boolean RHS): match-only (isUnbounded)
    std::ويڪٽر<Sva_State_راس*> midSources;
    منطقي errorEmitted = خطأ;  // Builder already emitted specific error; skip generic
    // For cover_sequence: when true, midSources already enumerate every
    // end-of-match, so wireMatchAndMidSources must NOT add the main
    // termVtxp -> matchVertex Link (would double-count via the merge vertex).
    منطقي رڪن_Is_Mid_ملايو = خطأ;
    منطقي valid() ثابت { أرجع رڪن_Vertexp != خالي_اشارو; }
    ساكن Build_نتيجو fail(منطقي errored = خطأ) { أرجع {خالي_اشارو, خالي_اشارو, {}, errored}; }
    ساكن Build_نتيجو failWithError() { أرجع {خالي_اشارو, خالي_اشارو, {}, سچ}; }
};

// Parser-marked SAnd of overlapped implications: a property if/else/case.
ساكن منطقي hasPropertyControlConjunction(ثابت Ast_ڳنڍ_اظهار* nodep) {
    أرجع nodep->exists([](ثابت AstSAnd* andp) { أرجع andp->propertyControl(); });
}

ساكن AstConst* newTypedConstp(File_سٽ* ثابت flp, ثابت Ast_ڳنڍ_DType* ثابت dtypep,
                                ثابت uint32_t قدر) {
    AstConst* ثابت constp = جديد AstConst{flp, AstConst::DTyped{}, dtypep};
    constp->انگ().سيٽ_Long(قدر);
    أرجع constp;
}

ساكن Ast_ڳنڍ_اظهار* sampled(Ast_ڳنڍ_اظهار* exprp) {
    أرجع جديد AstSampled{exprp->fileline(), exprp, exprp->dtypep(), سچ};
}

ساكن ثابت حرف* assertPassOnQuery(منطقي vacuous) {
    ساكن constexpr ثابت حرف* queries[2]
        = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
    أرجع queries[vacuous];
}

ساكن Ast_ڳنڍ_اظهار* assertOnCond(File_سٽ* flp, VAssertType type,
                                 VAssertDirectiveType directiveType) {
    إذا (!v3Global.opt.assertOn()) { أرجع جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}}; }
    أرجع جديد AstCExpr{flp, AstCExpr::خالص{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
}

ساكن Ast_ڳنڍ_اظهار* assertKillGet(File_سٽ* flp, VAssertType type,
                                  VAssertDirectiveType directiveType) {
    أرجع جديد AstCExpr{flp, AstCExpr::خالص{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_KILL", type, directiveType),
                        32};
}

ساكن string assert_Action_Control_اڳياڙي(VAssertDirectiveType directiveType) {
    ثابت صحيح controlled = !!(static_cast<صحيح>(directiveType)
                              & (static_cast<صحيح>(VAssertDirectiveType::ASSERT)
                                 | static_cast<صحيح>(VAssertDirectiveType::COVER)
                                 | static_cast<صحيح>(VAssertDirectiveType::ASSUME)));
    ثابت صحيح checkRuntime = controlled & static_cast<صحيح>(v3Global.opt.assertOn());
    أرجع "("s + std::منزل_string(controlled ^ 1) + " || ("s + std::منزل_string(checkRuntime)
           + " && "s;
}

ساكن Ast_ڳنڍ_اظهار* assertPassOnCond(File_سٽ* flp, VAssertType type,
                                     VAssertDirectiveType directiveType, منطقي vacuous) {
    أرجع جديد AstCExpr{
        flp, AstCExpr::خالص{},
        assert_Action_Control_اڳياڙي(directiveType)
            + V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType)
            + "))"s,
        1};
}

ساكن Ast_ڳنڍ_اظهار* assertFailOnCond(File_سٽ* flp, VAssertType type,
                                     VAssertDirectiveType directiveType) {
    أرجع جديد AstCExpr{
        flp, AstCExpr::خالص{},
        assert_Action_Control_اڳياڙي(directiveType)
            + V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType) + "))"s,
        1};
}

ساكن AstIf* newPassOnIf(File_سٽ* flp, Ast_ڳنڍ_اظهار* firep, Ast_ڳنڍ* bodyp, VAssertType type,
                          VAssertDirectiveType directiveType, منطقي vacuous) {
    Ast_ڳنڍ_اظهار* ثابت condp
        = جديد AstLogAnd{flp, firep, assertPassOnCond(flp, type, directiveType, vacuous)};
    AstIf* ثابت ifp = جديد AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(سچ);
    ifp->user1(سچ);
    أرجع ifp;
}

ساكن Ast_ڳنڍ_Stmt* newIfAssertFailOn(Ast_ڳنڍ* bodyp, VAssertDirectiveType directiveType,
                                      VAssertType type) {
    File_سٽ* ثابت flp = bodyp->fileline();
    Ast_ڳنڍ_اظهار* ثابت condp = assertFailOnCond(flp, type, directiveType);
    AstIf* ثابت ifp = جديد AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(سچ);
    ifp->user1(سچ);
    أرجع ifp;
}

//######################################################################
// NFA Builder

صنف SvaNfaBuilder final {
    Sva_گراف& m_گراف;  // NFA graph being built
    Ast_ڳنڍ_Module* ثابت m_modp;  // Module to receive hoisted sampled-prop temps
    V3_Unique_نالا& m_prop_عارضي_نالا;  // Module-shared temp-var name source
    std::ويڪٽر<Ast_ڳنڍ_اظهار*> m_temporal_Guard_ڍير;  // Guards active across nested temporal states
    // Outer abort conditions, AND-ed as !cond into inner abort edges
    // (IEEE 1800-2023 16.12.14 outer-wraps-inner).
    std::ويڪٽر<Ast_ڳنڍ_اظهار*> m_outer_Abort_ڍير;
    منطقي m_اندريون_Unbounded_Scope = خطأ;  // Sticky: nodes created after inherit liveness
    منطقي m_markStrongPending = خطأ;  // Mark new vertices as strong s_always in-window
    منطقي m_isCover = خطأ;  // Cover directives do not fail at end-of-simulation
    // IEEE 1800-2023 16.14.3 cover sequence: each end-of-match fires the action,
    // not just the first. Builder builds parallel-branch (no first-match-wins)
    // topology when true. Default false preserves cover_property semantics.
    منطقي m_isCoverSeq = خطأ;
    // Unsupported endpoint topology must reject, not ignore, or the wait hangs
    منطقي m_isSeqEvent = خطأ;
    منطقي m_اندريون_Sequence_اڳياڙي = خطأ;  // Prefix composition does not preserve every endpoint
    منطقي m_want_Bool_پن = خطأ;  // Building a |-> / |=> or top-level body (see buildOrMerge)

    بنية RangeDelayRejectInfo final {
        Sva_State_راس* startp = خالي_اشارو;
        بنانشان range = 0;
        صحيح rhsLen = 0;
    };

    فراغ warnEndpointUnsupported(File_سٽ* flp, ثابت string& what) ثابت {
        إذا (m_isSeqEvent) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: sequence used as an event control with " << what);
        } وإلا {
            flp->v3warn(COVERIGN, "Ignoring unsupported: cover sequence with " << what);
        }
    }

    Ast_ڳنڍ_اظهار* throughoutCond(Ast_ڳنڍ_اظهار* بنياد_Condp, File_سٽ* flp) {
        إذا (m_temporal_Guard_ڍير.خالي_جايون()) أرجع بنياد_Condp;
        // AND all active temporal guards (supports nesting)
        // Each must use $sampled values.
        Ast_ڳنڍ_اظهار* guardp = خالي_اشارو;
        لكل (Ast_ڳنڍ_اظهار* ثابت condp : m_temporal_Guard_ڍير) {
            Ast_ڳنڍ_اظهار* ثابت clonep = sampled(condp->clone_وڻ_خالص(خطأ));
            إذا (!guardp) {
                guardp = clonep;
            } وإلا {
                guardp = جديد AstLogAnd{flp, guardp, clonep};
            }
        }
        إذا (بنياد_Condp) { guardp = جديد AstLogAnd{flp, بنياد_Condp, guardp}; }
        أرجع guardp;
    }

    ساكن بنانشان getConstUInt(Ast_ڳنڍ_اظهار* exprp) {
        Ast_ڳنڍ_اظهار* ثابت constp = V3Const::constifyEdit(exprp->clone_وڻ_خالص(خطأ));
        ثابت AstConst* ثابت cp = VN_CAST(constp, Const);
        ثابت بنانشان val = cp ? cp->منزل_UInt() : 0;
        VL_DO_DANGLING(constp->delete_وڻ(), constp);
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
    ساكن صحيح fixed_ڊيگهائي(Ast_ڳنڍ_اظهار* nodep) {
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
            إذا (!delayp || !delayp->is_چڪر_Delay()) أرجع -1;
            بنانشان delay_چڪرن;
            إذا (delayp->isRangeDelay()) {
                إذا (delayp->isUnbounded()) أرجع -1;  // LCOV_EXCL_LINE
                ثابت بنانشان minD = getConstUInt(delayp->lhsp());
                ثابت بنانشان maxD = getConstUInt(delayp->rhsp());
                إذا (minD != maxD) أرجع -1;
                delay_چڪرن = minD;
            } وإلا {
                delay_چڪرن = getConstUInt(delayp->lhsp());
            }
            صحيح preLen = 0;
            إذا (Ast_ڳنڍ_اظهار* ثابت prep = sexprp->preExprp()) {
                preLen = fixed_ڊيگهائي(prep);
                إذا (preLen < 0) أرجع -1;  // LCOV_EXCL_LINE
            }
            ثابت صحيح bodyLen = fixed_ڊيگهائي(sexprp->exprp());
            إذا (bodyLen < 0) أرجع -1;  // LCOV_EXCL_LINE
            أرجع preLen + delay_چڪرن + bodyLen;
        }
        إذا (AstSThroughout* ثابت throughp = VN_CAST(nodep, SThroughout)) {
            أرجع fixed_ڊيگهائي(throughp->rhsp());
        }
        إذا (AstSOr* ثابت orp = VN_CAST(nodep, SOr)) {
            // Alternatives must share one end cycle; buildSWithin relies on
            // this to pair the OR with an SIntersect.
            ثابت صحيح lhsLen = fixed_ڊيگهائي(orp->lhsp());
            ثابت صحيح rhsLen = fixed_ڊيگهائي(orp->rhsp());
            إذا (lhsLen < 0 || rhsLen < 0 || lhsLen != rhsLen) أرجع -1;
            أرجع lhsLen;
        }
        إذا (AstSWithin* ثابت withinp = VN_CAST(nodep, SWithin)) {
            // `seq1 within seq2` ends at seq2's end cycle (IEEE 16.9.10).
            ثابت صحيح lhsLen = fixed_ڊيگهائي(withinp->lhsp());
            ثابت صحيح rhsLen = fixed_ڊيگهائي(withinp->rhsp());
            إذا (lhsLen < 0 || rhsLen < 0 || lhsLen > rhsLen) أرجع -1;
            أرجع rhsLen;
        }
        // LCOV_EXCL_START -- defensive: V3AssertPre rejects composite SVA ops
        // nested in an intersect arm before fixedLength runs (clock-context
        // resolution fails). Kept as a guard in case future parser relaxations
        // permit it.
        إذا (nodep->is_Multi_چڪر_Sva()) أرجع -1;
        // LCOV_EXCL_STOP
        // Plain boolean expression (no SVA constructs) -- 0 cycles.
        أرجع 0;
    }

    // Contiguous match-length range [lo,hi] for an operand whose length varies
    // from AT MOST ONE ranged cycle delay; {-1,-1} otherwise. Drives the
    // variable-length `intersect` lowering (IEEE 1800-2023 16.9.6): more than
    // one ranged delay would make the per-length realization ambiguous, so it
    // is reported unsupported rather than mis-paired.
    ساكن std::pair<صحيح, صحيح> ڊيگهائي_Range(Ast_ڳنڍ_اظهار* nodep) {
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
            إذا (!delayp || !delayp->is_چڪر_Delay()) أرجع {-1, -1};
            std::pair<صحيح, صحيح> delayRange;
            إذا (delayp->isRangeDelay()) {
                إذا (delayp->isUnbounded()) أرجع {-1, -1};
                ثابت بنانشان minD = getConstUInt(delayp->lhsp());
                ثابت بنانشان maxD = getConstUInt(delayp->rhsp());
                delayRange = {minD, maxD};
            } وإلا {
                ثابت بنانشان d = getConstUInt(delayp->lhsp());
                delayRange = {d, d};
            }
            std::pair<صحيح, صحيح> preRange{0, 0};
            إذا (Ast_ڳنڍ_اظهار* ثابت prep = sexprp->preExprp()) {
                preRange = ڊيگهائي_Range(prep);
                إذا (preRange.پهريون < 0) أرجع {-1, -1};
            }
            ثابت std::pair<صحيح, صحيح> bodyRange = ڊيگهائي_Range(sexprp->exprp());
            إذا (bodyRange.پهريون < 0) أرجع {-1, -1};
            ثابت صحيح variableParts = (preRange.پهريون != preRange.ٻيو)
                                      + (delayRange.پهريون != delayRange.ٻيو)
                                      + (bodyRange.پهريون != bodyRange.ٻيو);
            إذا (variableParts > 1) أرجع {-1, -1};
            أرجع {preRange.پهريون + delayRange.پهريون + bodyRange.پهريون,
                    preRange.ٻيو + delayRange.ٻيو + bodyRange.ٻيو};
        }
        إذا (AstSThroughout* ثابت throughp = VN_CAST(nodep, SThroughout)) {
            أرجع ڊيگهائي_Range(throughp->rhsp());
        }
        إذا (nodep->is_Multi_چڪر_Sva()) أرجع {-1, -1};
        أرجع {0, 0};  // plain boolean -- 0 cycles
    }

    // Clone `operand` with its sole variable ranged cycle delay pinned so the
    // total match length is exactly `len`. `lo` is the operand's minimum length
    // (lengthRange().first). A fixed operand has no such delay and is returned
    // as a plain clone (callers only request its single achievable length).
    ساكن Ast_ڳنڍ_اظهار* realize_At_ڊيگهائي(Ast_ڳنڍ_اظهار* عمليو, صحيح len, صحيح lo) {
        Ast_ڳنڍ_اظهار* ثابت clonep = عمليو->clone_وڻ_خالص(خطأ);
        AstDelay* rangeDelayp = خالي_اشارو;
        clonep->foreach([&](AstDelay* dp) {
            إذا (!rangeDelayp && dp->isRangeDelay() && !dp->isUnbounded()
                && getConstUInt(dp->lhsp()) != getConstUInt(dp->rhsp())) {
                rangeDelayp = dp;
            }
        });
        إذا (rangeDelayp) {
            File_سٽ* ثابت flp = rangeDelayp->fileline();
            ثابت بنانشان pinned = getConstUInt(rangeDelayp->lhsp()) + (len - lo);
            Ast_ڳنڍ_اظهار* ثابت oldMinp = rangeDelayp->lhsp();
            oldMinp->replaceWith(جديد AstConst{flp, pinned});
            VL_DO_DANGLING(oldMinp->delete_وڻ(), oldMinp);
            // Drop the max bound so it lowers as a fixed `##d`, not `##[d:d]`.
            Ast_ڳنڍ* ثابت oldMaxp = rangeDelayp->rhsp()->unlink_Fr_پٺ();
            VL_DO_DANGLING(oldMaxp->delete_وڻ(), oldMaxp);
        }
        أرجع clonep;
    }

    // Cuts AST size from O(N * sizeof(exprp)) to O(N) + O(sizeof(exprp)) by
    // sharing a single `VarRef` across N check edges. Hoist also matches the
    // IEEE 1800-2023 16.9.9 "single preponed-region snapshot" semantic for
    // any exprp -- even an impure one would now evaluate exactly once per
    // clock instead of N times. Orphan temps from failed builds are unused
    // MODULETEMPs and are removed by V3Dead.
    AstVar* tryHoistSampled(Ast_ڳنڍ_اظهار* exprp, File_سٽ* flp, بنانشان clone_ڳڻپ) {
        constexpr بنانشان k_Hoist_حد = 2;
        إذا (clone_ڳڻپ < k_Hoist_حد) أرجع خالي_اشارو;
        AstVar* ثابت عارضي_Varp
            = جديد AstVar{flp, VVarType::MODULETEMP, m_prop_عارضي_نالا.get(exprp), exprp->dtypep()};
        m_modp->جوڙيو_Stmtsp(عارضي_Varp);
        Ast_سونپيو* ثابت assignp = جديد Ast_سونپيو{flp, جديد AstVarRef{flp, عارضي_Varp, VAccess::WRITE},
                                                 sampled(exprp->clone_وڻ_خالص(خطأ))};
        m_modp->جوڙيو_Stmtsp(جديد AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, خالي_اشارو, assignp});
        أرجع عارضي_Varp;
    }

    ساكن Ast_ڳنڍ_اظهار* sampledRefOrClone(AstVar* hoistVarp, Ast_ڳنڍ_اظهار* exprp, File_سٽ* flp) {
        إذا (hoistVarp) أرجع جديد AstVarRef{flp, hoistVarp, VAccess::READ};
        أرجع sampled(exprp->clone_وڻ_خالص(خطأ));
    }

    // Create vertex and inherit temporal guards from the current scope.
    Sva_State_راس* scoped_Create_راس() {
        Sva_State_راس* ثابت vtxp = m_گراف.create_State_راس();
        لكل (Ast_ڳنڍ_اظهار* ثابت cp : m_temporal_Guard_ڍير) {
            vtxp->m_throughoutConds.ڌڪو_پٺ(cp->clone_وڻ_خالص(خطأ));
        }
        إذا (m_اندريون_Unbounded_Scope) vtxp->m_isUnbounded = سچ;
        إذا (m_markStrongPending) vtxp->m_strongPending = سچ;
        أرجع vtxp;
    }

    // AND current temporal guards into every edge/link.
    Sva_Trans_ڪنارو* guardedLink(Sva_State_راس* fromp, Sva_State_راس* چوٽي, Ast_ڳنڍ_اظهار* condp,
                              File_سٽ* flp) {
        أرجع m_گراف.جوڙيو_Link(fromp, چوٽي, throughoutCond(condp, flp));
    }
    Sva_Trans_ڪنارو* guardedLink(Sva_State_راس* fromp, Sva_State_راس* چوٽي, File_سٽ* flp) {
        أرجع m_گراف.جوڙيو_Link(fromp, چوٽي, throughoutCond(خالي_اشارو, flp));
    }
    Sva_Trans_ڪنارو* guarded_ڪنارو(Sva_State_راس* fromp, Sva_State_راس* چوٽي, Ast_ڳنڍ_اظهار* condp,
                              File_سٽ* flp) {
        أرجع m_گراف.جوڙيو_Clocked_ڪنارو(fromp, چوٽي, throughoutCond(condp, flp));
    }
    Sva_Trans_ڪنارو* guarded_ڪنارو(Sva_State_راس* fromp, Sva_State_راس* چوٽي, File_سٽ* flp) {
        أرجع m_گراف.جوڙيو_Clocked_ڪنارو(fromp, چوٽي, throughoutCond(خالي_اشارو, flp));
    }

    Sva_State_راس* جوڙيو_Delay_Chain(Sva_State_راس* startp, بنانشان سائيز, File_سٽ* flp,
                                  منطقي isFixed = سچ, Ast_ڳنڍ_اظهار* clearCondp = خالي_اشارو,
                                  Ast_ڳنڍ_اظهار* advanceCondp = خالي_اشارو) {
        إذا (isFixed && سائيز == 0) أرجع startp;
        UASSERT_OBJ(سائيز > 0, startp, "Delay chain needs at least one slot");
        إذا (isFixed && سائيز == 1 && !advanceCondp) {
            Sva_State_راس* ثابت nextp = scoped_Create_راس();
            guarded_ڪنارو(startp, nextp, flp);
            أرجع nextp;
        }
        Sva_State_راس* ثابت ringVtxp = scoped_Create_راس();
        ringVtxp->m_isFixedDelayRing = isFixed;
        ringVtxp->m_delay_Ring_سائيز = سائيز;
        إذا (clearCondp) {
            UASSERT_OBJ(!isFixed, startp, "Fixed delay cannot have a clear condition");
            ringVtxp->m_delayRingClearCondp = clearCondp->clone_وڻ_خالص(خطأ);
        }
        ringVtxp->m_delayRingAdvanceCondp = advanceCondp;
        إذا (isFixed) {
            guarded_ڪنارو(startp, ringVtxp, flp);
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
                         std::ويڪٽر<Sva_State_راس*>& midSources,
                         RangeDelayRejectInfo* rangeRejectInfop = خالي_اشارو) {
        File_سٽ* ثابت flp = sexprp->fileline();
        AstDelay* ثابت delayp = VN_AS(sexprp->delayp(), Delay);
        Ast_ڳنڍ_اظهار* ثابت rhsExprp = sexprp->exprp();
        ثابت بنانشان minDelay = getConstUInt(delayp->lhsp());
        إذا (delayp->isUnbounded()) {
            // `##[M:$]`: wait M cycles, then retain every matured attempt in a
            // one-slot ring while also exposing the first eligible tick.
            currentp = جوڙيو_Delay_Chain(currentp, minDelay, flp);
            Sva_State_راس* ثابت waitRingp = جوڙيو_Delay_Chain(currentp, 1, flp, خطأ);
            // The ring's live count is updated in NBA, so this link retains the previous
            // tick's live attempts alongside new arrivals for the next tick.
            guardedLink(waitRingp, waitRingp, flp);
            Sva_State_راس* ثابت ملايو_Vtxp = scoped_Create_راس();
            guardedLink(currentp, ملايو_Vtxp, flp);
            guardedLink(waitRingp, ملايو_Vtxp, flp);
            currentp = ملايو_Vtxp;
            currentp->m_isUnbounded = سچ;  // Liveness, so no reject.
            m_اندريون_Unbounded_Scope = سچ;
            أرجع سچ;
        }
        ثابت بنانشان maxDelay = getConstUInt(delayp->rhsp());
        إذا (minDelay == maxDelay) {
            currentp = جوڙيو_Delay_Chain(currentp, minDelay, flp);
            أرجع سچ;
        }
        ثابت بنانشان range = maxDelay - minDelay;
        currentp = جوڙيو_Delay_Chain(currentp, minDelay, flp);
        ثابت منطقي multi_چڪر_Rhs = rhsExprp->is_Multi_چڪر_Sva();
        ثابت Ast_ڳنڍ_اظهار* ثابت preExprp = sexprp->preExprp();
        // Prefix and nested-sequence composition can lose endpoints or their multiplicity
        // required by cover sequence (IEEE 1800-2023 16.14.3), regardless of the range width.
        إذا (m_isCoverSeq
            && (multi_چڪر_Rhs || m_اندريون_Sequence_اڳياڙي
                || (preExprp && preExprp->is_Multi_چڪر_Sva()))) {
            warnEndpointUnsupported(flp, "this ranged cycle delay");
            أرجع خطأ;
        }
        إذا (m_اندريون_Sequence_اڳياڙي) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: Bounded ranged cycle delay in a sequence prefix");
            أرجع خطأ;
        }
        إذا (multi_چڪر_Rhs) {
            // Launch a candidate at every eligible tick. Candidate-local misses are not
            // assertion rejects while a later position can still match.
            إذا (rangeRejectInfop) {
                ثابت صحيح rhsLen = fixed_ڊيگهائي(rhsExprp);
                إذا (rhsLen >= 0) *rangeRejectInfop = {currentp, range, rhsLen};
            }
            Sva_State_راس* ثابت tailp = جوڙيو_Delay_Chain(currentp, range + 1U, flp, خطأ);
            Sva_State_راس* ثابت ملايو_Vtxp = scoped_Create_راس();
            ملايو_Vtxp->m_isUnbounded = سچ;
            guardedLink(currentp, ملايو_Vtxp, flp);
            guardedLink(tailp, ملايو_Vtxp, flp);
            currentp = ملايو_Vtxp;
            m_اندريون_Unbounded_Scope = سچ;
        } وإلا {
            // The first eligible tick is explicit. The ring retains attempts for the
            // remaining ticks and exposes the final tick for rejection. Only properties
            // clear on success, since cover sequence counts every endpoint.
            midSources.ڌڪو_پٺ(currentp);
            currentp = جوڙيو_Delay_Chain(currentp, range + 1U, flp, خطأ,
                                     m_isCoverSeq ? خالي_اشارو : rhsExprp);
        }
        أرجع سچ;
    }

    فراغ جوڙيو_Finite_Range_Reject(ثابت RangeDelayRejectInfo& info, ثابت Build_نتيجو& نتيجو,
                              File_سٽ* flp) {
        إذا (!info.startp) أرجع;

        Sva_State_راس* ثابت expiryVtxp
            = جوڙيو_Delay_Chain(info.startp, info.range + info.rhsLen, flp);
        Sva_State_راس* ثابت expiryMatchp = scoped_Create_راس();
        std::ويڪٽر<Sva_State_راس*> sources = نتيجو.midSources;
        sources.ڌڪو_پٺ(نتيجو.رڪن_Vertexp);
        لكل (Sva_State_راس* ثابت srcp : sources) {
            Ast_ڳنڍ_اظهار* ثابت condp
                = نتيجو.finalCondp ? sampled(نتيجو.finalCondp->clone_وڻ_خالص(خطأ)) : خالي_اشارو;
            Sva_State_راس* ثابت successNowp = scoped_Create_راس();
            guardedLink(srcp, successNowp, condp, flp);
            // Retain a success through the latest deadline it can satisfy.
            Sva_State_راس* ثابت historyp
                = جوڙيو_Delay_Chain(successNowp, info.range + 1U, flp, خطأ);
            guardedLink(successNowp, expiryMatchp, flp);
            guardedLink(historyp, expiryMatchp, flp);
        }

        Sva_State_راس* ثابت sinkVtxp = m_گراف.create_State_راس();
        sinkVtxp->m_isRejectSink = سچ;
        Sva_Trans_ڪنارو* ثابت rejectp = m_گراف.جوڙيو_Link(expiryVtxp, sinkVtxp);
        rejectp->m_rejectOnFail = سچ;
        rejectp->m_condVtxp = expiryMatchp;
    }

    Build_نتيجو buildSExpr(AstSExpr* sexprp, Sva_State_راس* داخلا_Vtxp,
                           منطقي is_چوٽي_Level_قدم = خطأ) {
        AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
        إذا (!delayp || !delayp->is_چڪر_Delay()) أرجع Build_نتيجو::fail();

        File_سٽ* ثابت flp = sexprp->fileline();
        Ast_ڳنڍ_اظهار* ثابت exprp = sexprp->exprp();

        // Handle LHS (preExpr)
        Sva_State_راس* currentp = داخلا_Vtxp;
        إذا (Ast_ڳنڍ_اظهار* ثابت preExprp = sexprp->preExprp()) {
            VL_RESTORER(m_اندريون_Sequence_اڳياڙي);
            m_اندريون_Sequence_اڳياڙي = سچ;
            ثابت Build_نتيجو pre = build_اظهار(preExprp, currentp, is_چوٽي_Level_قدم);
            إذا (!pre.valid()) أرجع Build_نتيجو::fail(pre.errorEmitted);  // LCOV_EXCL_LINE
            إذا (pre.finalCondp) {
                Sva_State_راس* ثابت condVtxp = scoped_Create_راس();
                Sva_Trans_ڪنارو* ثابت edgep = guardedLink(
                    pre.رڪن_Vertexp, condVtxp, sampled(pre.finalCondp->clone_وڻ_خالص(خطأ)), flp);
                إذا (is_چوٽي_Level_قدم && !pre.رڪن_Vertexp->m_isUnbounded && !m_اندريون_Unbounded_Scope) {
                    // Do not mark liveness sources: first boolean check is deferred.
                    edgep->m_rejectOnFail = سچ;
                }
                freeUnlinkedCondp(pre.finalCondp);
                currentp = condVtxp;
            } وإلا {
                currentp = pre.رڪن_Vertexp;
            }
        }

        // Handle delay
        std::ويڪٽر<Sva_State_راس*> rangeMidSources;
        RangeDelayRejectInfo rangeRejectInfo;
        ثابت منطقي جوڙيو_Range_Reject = is_چوٽي_Level_قدم && !m_اندريون_Unbounded_Scope;
        إذا (delayp->isRangeDelay()) {
            إذا (!applyRangeDelay(sexprp, currentp, rangeMidSources,
                                 جوڙيو_Range_Reject ? &rangeRejectInfo : خالي_اشارو)) {
                أرجع Build_نتيجو::failWithError();
            }
        } وإلا {
            ثابت بنانشان delay_چڪرن = getConstUInt(delayp->lhsp());
            currentp = جوڙيو_Delay_Chain(currentp, delay_چڪرن, flp);
        }

        // Multi-cycle RHS: recurse (only plain boolean is returned as finalCondp).
        إذا (exprp->is_Multi_چڪر_Sva()) {
            ثابت Build_نتيجو نتيجو = build_اظهار(exprp, currentp, is_چوٽي_Level_قدم);
            إذا (نتيجو.valid()) جوڙيو_Finite_Range_Reject(rangeRejectInfo, نتيجو, flp);
            أرجع نتيجو;
        }
        أرجع {currentp, exprp, std::move(rangeMidSources)};
    }

    Build_نتيجو buildConsRep(AstSConsRep* repp, Sva_State_راس* داخلا_Vtxp,
                             منطقي is_چوٽي_Level_قدم = خطأ) {
        File_سٽ* ثابت flp = repp->fileline();
        Ast_ڳنڍ_اظهار* ثابت exprp = repp->exprp();
        // Multi-cycle expr in ConsRep not yet supported; bail to avoid invalid AST.
        إذا (exprp->is_Multi_چڪر_Sva()) {
            repp->v3warn(E_UNSUPPORTED, "Unsupported: multi-cycle sequence expression inside"
                                        " consecutive repetition (IEEE 1800-2023 16.9.2)");
            أرجع Build_نتيجو::failWithError();
        }
        ثابت بنانشان minN = getConstUInt(repp->countp());

        // Sum sites across prefix + unbounded/range tail so one hoist covers
        // every check edge of this repetition.
        بنانشان ڪل_Sites = minN;
        إذا (repp->unbounded()) {
            ڪل_Sites += 1;
        } وإلا إذا (repp->maxCountp()) {
            ڪل_Sites += getConstUInt(repp->maxCountp()) - minN;
        }
        AstVar* ثابت hoistVarp = tryHoistSampled(exprp, flp, ڪل_Sites);

        // Cover-sequence (IEEE 1800-2023 16.14.3): collect each end-of-match
        // position so they all fire the action, not just the merged terminal.
        std::ويڪٽر<Sva_State_راس*> consMidSources;

        Sva_State_راس* currentp = داخلا_Vtxp;
        لكل (بنانشان i = 0; i < minN; ++i) {
            // Keep the first repetition explicit, collapse all remaining checks into the ring.
            إذا (i == 1) {
                currentp = جوڙيو_Delay_Chain(currentp, minN - 1, flp);
                currentp->m_delayRingClearCondp
                    = جديد AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                currentp->m_replayAbortReject = سچ;
                إذا (is_چوٽي_Level_قدم) {
                    currentp->m_throughoutConds.ڌڪو_پٺ(
                        sampledRefOrClone(hoistVarp, exprp, flp));
                }
                i = minN - 1;
            }
            // Every repetition in the minimum prefix is required.
            Sva_State_راس* ثابت condVtxp = scoped_Create_راس();
            Sva_Trans_ڪنارو* ثابت linkp
                = guardedLink(currentp, condVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            // Only an outermost required step rejects the property. Its first check is explicit,
            // later required checks reject through the ring's throughout guard.
            إذا (is_چوٽي_Level_قدم && i == 0) linkp->m_rejectOnFail = سچ;
            currentp = condVtxp;
        }
        // After minN: currentp is the first valid end-of-match position for [*m:n].
        إذا (m_isCoverSeq && (repp->unbounded() || repp->maxCountp())) {
            consMidSources.ڌڪو_پٺ(currentp);
        }

        إذا (repp->unbounded()) {
            إذا (minN == 0) {
                Sva_State_راس* ثابت waitVtxp = scoped_Create_راس();
                guarded_ڪنارو(currentp, waitVtxp, flp);
                Sva_State_راس* ثابت checkVtxp = scoped_Create_راس();
                guardedLink(waitVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guarded_ڪنارو(checkVtxp, waitVtxp, flp);
                guardedLink(currentp, checkVtxp, flp);
                currentp = checkVtxp;
            } وإلا {
                Sva_State_راس* ثابت loop_پٺ_Vtxp = scoped_Create_راس();
                guarded_ڪنارو(currentp, loop_پٺ_Vtxp, flp);
                Sva_State_راس* ثابت reCheckVtxp = scoped_Create_راس();
                guardedLink(loop_پٺ_Vtxp, reCheckVtxp, sampledRefOrClone(hoistVarp, exprp, flp),
                            flp);
                guarded_ڪنارو(reCheckVtxp, loop_پٺ_Vtxp, flp);
                guardedLink(reCheckVtxp, currentp, flp);
            }
            currentp->m_isUnbounded = سچ;
            m_اندريون_Unbounded_Scope = سچ;
        } وإلا إذا (repp->maxCountp()) {
            ثابت بنانشان maxN = getConstUInt(repp->maxCountp());
            Sva_State_راس* ثابت ملايو_Vtxp = scoped_Create_راس();
            guardedLink(currentp, ملايو_Vtxp, flp);
            بنانشان پڇ_Min_N = minN;
            Sva_State_راس* پڇ_Startp = currentp;
            إذا (minN == 0) {
                // Build the first optional iteration explicitly. Feeding the empty endpoint
                // directly into a range ring would incorrectly keep that match alive.
                Sva_State_راس* ثابت ايندڙ_Vtxp = scoped_Create_راس();
                guarded_ڪنارو(currentp, ايندڙ_Vtxp, flp);
                Sva_State_راس* ثابت checkVtxp = scoped_Create_راس();
                guardedLink(ايندڙ_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, ملايو_Vtxp, flp);
                إذا (m_isCoverSeq) consMidSources.ڌڪو_پٺ(checkVtxp);
                پڇ_Startp = checkVtxp;
                پڇ_Min_N = 1;
            }
            إذا (maxN > پڇ_Min_N) {
                // Add tail-ring only if the tail is non-empty.
                Sva_State_راس* ثابت ايندڙ_Vtxp
                    = جوڙيو_Delay_Chain(پڇ_Startp, maxN - پڇ_Min_N + 1, flp, خطأ);
                ايندڙ_Vtxp->m_delayRingClearCondp
                    = جديد AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                ايندڙ_Vtxp->m_replayAbortReject = سچ;
                Sva_State_راس* ثابت checkVtxp = scoped_Create_راس();
                guardedLink(ايندڙ_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, ملايو_Vtxp, flp);
                إذا (m_isCoverSeq) consMidSources.ڌڪو_پٺ(checkVtxp);
            }
            currentp = ملايو_Vtxp;
        }
        // finalCond = nullptr (already checked via Links)
        Build_نتيجو res;
        res.رڪن_Vertexp = currentp;
        res.finalCondp = خالي_اشارو;
        res.midSources = std::move(consMidSources);
        // mergeVtxp is the OR of all the end-positions we already pushed to
        // midSources, so the main termVtxp -> matchVertex Link would duplicate.
        res.رڪن_Is_Mid_ملايو = m_isCoverSeq && !res.midSources.خالي_جايون();
        أرجع res;
    }

    // always[lo:hi] / s_always[lo:hi] (IEEE 1800-2023 16.12.11).
    Build_نتيجو buildPropAlways(AstPropAlways* nodep, Sva_State_راس* داخلا_Vtxp,
                                منطقي is_چوٽي_Level_قدم = خطأ) {
        File_سٽ* ثابت flp = nodep->fileline();
        Ast_ڳنڍ_اظهار* ثابت propp = nodep->propp();
        ثابت بنانشان lo = getConstUInt(nodep->loBoundp());
        إذا (VN_IS(nodep->hiBoundp(), Unbounded)) {
            // Weak always [lo:$]: unbounded upper bound (IEEE 1800-2023 16.12.11).
            // p must hold at every clock tick at least lo cycles after the attempt
            // start; those ticks are not required to exist, so there is no
            // end-of-trace obligation (weak). The self-loop keeps the attempt live
            // every cycle; each observed cycle is a safety obligation, so a false p
            // rejects immediately.
            UASSERT_OBJ(!nodep->isStrong(), nodep, "Unbounded always must be weak (V3Width)");
            Sva_State_راس* ثابت livep = جوڙيو_Delay_Chain(داخلا_Vtxp, lo, flp);
            livep->m_isUnbounded = سچ;
            guarded_ڪنارو(livep, livep, flp);  // stay active every subsequent cycle
            Sva_State_راس* ثابت sinkp = m_گراف.create_State_راس();
            sinkp->m_isRejectSink = سچ;
            Sva_Trans_ڪنارو* ثابت rejEdgep
                = guardedLink(livep, sinkp, sampled(propp->clone_وڻ_خالص(خطأ)), flp);
            إذا (is_چوٽي_Level_قدم) rejEdgep->m_rejectOnFail = سچ;
            أرجع {livep, خالي_اشارو, {}};
        }
        ثابت بنانشان hi = getConstUInt(nodep->hiBoundp());
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
        Sva_State_راس* currentp = جوڙيو_Delay_Chain(داخلا_Vtxp, lo, flp);
        Sva_State_راس* ثابت checkp = scoped_Create_راس();
        Sva_Trans_ڪنارو* ثابت linkp
            = guardedLink(currentp, checkp, sampled(propp->clone_وڻ_خالص(خطأ)), flp);
        إذا (is_چوٽي_Level_قدم) linkp->m_rejectOnFail = سچ;
        currentp = checkp;
        m_temporal_Guard_ڍير.ڌڪو_پٺ(propp);
        currentp = جوڙيو_Delay_Chain(currentp, hi - lo, flp);
        إذا (nodep->isStrong() && currentp->m_delay_Ring_سائيز) currentp->m_strongAlwaysRing = سچ;
        m_temporal_Guard_ڍير.ڪڍو_پٺ();
        أرجع {currentp, propp, {}};
    }

    Build_نتيجو buildGotoRep(AstSGotoRep* repp, Sva_State_راس* داخلا_Vtxp) {
        File_سٽ* ثابت flp = repp->fileline();
        Ast_ڳنڍ_اظهار* ثابت exprp = repp->exprp();
        ثابت بنانشان minN = getConstUInt(repp->countp());
        إذا (minN == 0) أرجع Build_نتيجو::fail();
        ثابت منطقي hasMax = repp->maxCountp() != خالي_اشارو;
        ثابت بنانشان maxN = hasMax ? getConstUInt(repp->maxCountp()) : minN;
        إذا (m_isCoverSeq) {
            // Several attempts may coalesce in the one-bit wait vertex before reaching the ring,
            // so a cover sequence action block could run too few times.
            warnEndpointUnsupported(flp, "a goto repetition");
            أرجع Build_نتيجو::failWithError();
        }

        AstVar* ثابت hoistVarp = tryHoistSampled(exprp, flp, 2);

        // The first guardedEdge is the ##1 before waiting for a match. In the wait state, false
        // takes the clocked self-loop, while true takes the zero-delay guardedLink on that tick.
        Sva_State_راس* ثابت waitVtxp = scoped_Create_راس();
        guarded_ڪنارو(داخلا_Vtxp, waitVtxp, flp);
        guarded_ڪنارو(waitVtxp, waitVtxp,
                    جديد AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)}, flp);
        Sva_State_راس* currentp = scoped_Create_راس();
        guardedLink(waitVtxp, currentp, sampledRefOrClone(hoistVarp, exprp, flp), flp);

        إذا (minN > 1) {
            Sva_State_راس* ثابت ringVtxp = جوڙيو_Delay_Chain(
                currentp, minN - 1, flp, سچ, خالي_اشارو, sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = سچ;
            Sva_State_راس* ثابت checkVtxp = scoped_Create_راس();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            currentp = checkVtxp;
        }
        إذا (!hasMax) {
            currentp->m_isUnbounded = سچ;  // [->N] waits unboundedly
            m_اندريون_Unbounded_Scope = سچ;
            أرجع {currentp, خالي_اشارو, {}};
        }

        // [->M:N]: the range ring holds matches from M through N and advances
        // only on expr, preserving arbitrarily long gaps between occurrences.
        Sva_State_راس* ثابت ملايو_Vtxp = scoped_Create_راس();
        guardedLink(currentp, ملايو_Vtxp, flp);  // accept at match_M
        إذا (maxN > minN) {
            Sva_State_راس* ثابت ringVtxp
                = جوڙيو_Delay_Chain(currentp, maxN - minN + 1, flp, خطأ, خالي_اشارو,
                                sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = سچ;
            Sva_State_راس* ثابت checkVtxp = scoped_Create_راس();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            guardedLink(checkVtxp, ملايو_Vtxp, flp);
        }
        ملايو_Vtxp->m_isUnbounded = سچ;  // [->M:N] still has unbounded waits between matches
        m_اندريون_Unbounded_Scope = سچ;
        أرجع {ملايو_Vtxp, خالي_اشارو, {}};
    }

    // True if the sequence has a zero-minimum consecutive repetition, which may match empty
    ساكن منطقي hasZeroMinRep(ثابت Ast_ڳنڍ_اظهار* seqp) {
        أرجع seqp->exists(
            [](ثابت AstSConsRep* repp) { أرجع getConstUInt(repp->countp()) == 0; });
    }

    // True if a leaf's condition has no implication or multi-cycle sequence in it
    // (a leaf can be e.g. 'not (x |-> y)')
    ساكن منطقي isPlainBoolean(ثابت Ast_ڳنڍ_اظهار* condp) {
        أرجع !condp->exists(
            [](ثابت Ast_ڳنڍ_اظهار* np) { أرجع np->is_Multi_چڪر_Sva() || VN_IS(np, Implication); });
    }

    // Build merge vertex for SOr / LogOr: both branches feed into one vertex.
    Build_نتيجو build_Or_ملايو(Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp, Sva_State_راس* داخلا_Vtxp,
                             File_سٽ* flp, منطقي want_Bool_پن) {
        ثابت Build_نتيجو lhs = build_اظهار(lhsp, داخلا_Vtxp);
        ثابت Build_نتيجو rhs = build_اظهار(rhsp, داخلا_Vtxp);
        إذا (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع Build_نتيجو::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP
        // IEEE 1800-2023 16.14.3: a cover sequence counts every end-of-match. A
        // sequence operand of 'or' can end more than once, but only its final
        // end reaches the merge vertex below, so reject sequence operands rather
        // than under-count. Plain boolean disjunction has one end per cycle and
        // is handled by the OR-fold.
        ثابت منطقي booleanOnly = lhs.رڪن_Vertexp == داخلا_Vtxp && lhs.finalCondp
                                 && rhs.رڪن_Vertexp == داخلا_Vtxp && rhs.finalCondp;
        إذا (m_isCoverSeq && !booleanOnly) {
            warnEndpointUnsupported(flp, "a sequence operand of 'or'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع Build_نتيجو::failWithError();
        }
        إذا (booleanOnly && want_Bool_پن) {
            إذا (isPlainBoolean(lhs.finalCondp) && isPlainBoolean(rhs.finalCondp)) {
                // As a |-> / |=> or top-level body, a disjunction of two plain booleans must
                // be able to reject: return it as a boolean finalCondp. The merge vertex
                // below drops the attempt silently when both operands are false.
                Ast_ڳنڍ_اظهار* ثابت orCondp
                    = جديد AstLogOr{flp, lhs.finalCondp->clone_وڻ_خالص(خطأ),
                                   rhs.finalCondp->clone_وڻ_خالص(خطأ)};
                freeUnlinkedCondp(lhs.finalCondp);
                freeUnlinkedCondp(rhs.finalCondp);
                أرجع {داخلا_Vtxp, orCondp, {}};
            }
        }
        Sva_State_راس* ثابت ملايو_Vtxp = scoped_Create_راس();
        إذا (booleanOnly) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsp,
                        "Single-cycle SOr operands must have finalCondp");
            guardedLink(داخلا_Vtxp, ملايو_Vtxp,
                        جديد AstLogOr{flp, sampled(lhs.finalCondp->clone_وڻ_خالص(خطأ)),
                                     sampled(rhs.finalCondp->clone_وڻ_خالص(خطأ))},
                        flp);
        } وإلا {
            إذا (lhs.finalCondp) {
                guardedLink(lhs.رڪن_Vertexp, ملايو_Vtxp,
                            sampled(lhs.finalCondp->clone_وڻ_خالص(خطأ)), flp);
            } وإلا {
                guardedLink(lhs.رڪن_Vertexp, ملايو_Vtxp, flp);
            }
            إذا (rhs.finalCondp) {
                guardedLink(rhs.رڪن_Vertexp, ملايو_Vtxp,
                            sampled(rhs.finalCondp->clone_وڻ_خالص(خطأ)), flp);
            } وإلا {
                guardedLink(rhs.رڪن_Vertexp, ملايو_Vtxp, flp);
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        أرجع {ملايو_Vtxp, خالي_اشارو, {}};
    }

    // Build done-latch combiner for SAnd/SIntersect (IEEE 1800-2023 16.9.5).
    Build_نتيجو buildAndCombiner(Ast_ڳنڍ_اظهار* lhsExprp, Ast_ڳنڍ_اظهار* rhsExprp,
                                 Sva_State_راس* داخلا_Vtxp, File_سٽ* flp) {
        // Snapshot-restore scope so LHS liveness does not leak into RHS.
        ثابت منطقي savedScope = m_اندريون_Unbounded_Scope;
        ثابت Build_نتيجو lhs = build_اظهار(lhsExprp, داخلا_Vtxp);
        ثابت منطقي lhsScope = m_اندريون_Unbounded_Scope;
        m_اندريون_Unbounded_Scope = savedScope;
        ثابت Build_نتيجو rhs = build_اظهار(rhsExprp, داخلا_Vtxp);
        ثابت منطقي rhsScope = m_اندريون_Unbounded_Scope;
        m_اندريون_Unbounded_Scope = savedScope || lhsScope || rhsScope;
        إذا (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع Build_نتيجو::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP

        // Single-cycle operands: use boolean AND (done-latch would fire across cycles).
        // If both operands stayed at entry, they must be boolean leaves which
        // buildExpr returns with finalCondp=nodep (non-null).
        إذا (lhs.رڪن_Vertexp == داخلا_Vtxp && rhs.رڪن_Vertexp == داخلا_Vtxp) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsExprp,
                        "Single-cycle SAnd operands must have finalCondp");
            Ast_ڳنڍ_اظهار* ثابت condp = جديد AstLogAnd{flp, lhs.finalCondp->clone_وڻ_خالص(خطأ),
                                                     rhs.finalCondp->clone_وڻ_خالص(خطأ)};
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع {داخلا_Vtxp, condp, {}};
        }
        // Range-delay mid-window sources in either sub-branch would need
        // to be folded into the latch's match-now signal, which the
        // current combiner does not support.
        إذا (!lhs.midSources.خالي_جايون() || !rhs.midSources.خالي_جايون()) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: ranged cycle delay in an operand of property 'and'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع Build_نتيجو::failWithError();
        }
        Sva_State_راس* ثابت combVtxp = scoped_Create_راس();
        combVtxp->m_isAndCombiner = سچ;
        combVtxp->m_andLhsTermp = lhs.رڪن_Vertexp;
        combVtxp->m_andRhsTermp = rhs.رڪن_Vertexp;
        إذا (lhs.finalCondp) combVtxp->m_andLhsCondp = lhs.finalCondp->clone_وڻ_خالص(خطأ);
        إذا (rhs.finalCondp) combVtxp->m_andRhsCondp = rhs.finalCondp->clone_وڻ_خالص(خطأ);
        إذا (lhs.رڪن_Vertexp->m_isUnbounded || rhs.رڪن_Vertexp->m_isUnbounded) {
            combVtxp->m_isUnbounded = سچ;
        }
        // Wire terminal-boolean rejects to a dedicated sink so each side can fail
        // the AND independently. Skip for liveness or single-cycle operands
        // (single-cycle termVertexp == entryVtxp would fire every cycle).
        إذا (!combVtxp->m_isUnbounded) {
            منطقي needSink = خطأ;
            ثابت منطقي lhs_Multi_چڪر = (lhs.رڪن_Vertexp != داخلا_Vtxp);
            ثابت منطقي rhs_Multi_چڪر = (rhs.رڪن_Vertexp != داخلا_Vtxp);
            إذا (lhs.finalCondp && lhs_Multi_چڪر && !lhs.رڪن_Vertexp->m_isUnbounded) {
                needSink = سچ;
            }
            إذا (rhs.finalCondp && rhs_Multi_چڪر && !rhs.رڪن_Vertexp->m_isUnbounded) {
                needSink = سچ;
            }
            إذا (needSink) {
                Sva_State_راس* ثابت sinkVtxp = m_گراف.create_State_راس();
                sinkVtxp->m_isRejectSink = سچ;
                إذا (lhs.finalCondp && lhs_Multi_چڪر && !lhs.رڪن_Vertexp->m_isUnbounded) {
                    Sva_Trans_ڪنارو* ثابت ep = m_گراف.جوڙيو_Link(
                        lhs.رڪن_Vertexp, sinkVtxp, sampled(lhs.finalCondp->clone_وڻ_خالص(خطأ)));
                    ep->m_rejectOnFail = سچ;
                }
                إذا (rhs.finalCondp && rhs_Multi_چڪر && !rhs.رڪن_Vertexp->m_isUnbounded) {
                    Sva_Trans_ڪنارو* ثابت ep = m_گراف.جوڙيو_Link(
                        rhs.رڪن_Vertexp, sinkVtxp, sampled(rhs.finalCondp->clone_وڻ_خالص(خطأ)));
                    ep->m_rejectOnFail = سچ;
                }
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        أرجع {combVtxp, خالي_اشارو, {}};
    }

    // Lower `seq1 within seq2` (IEEE 1800-2023 16.9.10) as:
    //   (OR_{i in 0..slack} 1 ##i seq1 ##(slack-i) 1) intersect seq2
    // Both operands must have fixed length (no ranged cycle delays).
    // The OR lives inside a single SIntersect so one AndCombiner done-latch
    // serves all offsets; lifting it out would double-count matches where
    // multiple offsets accept on the same seq2 end cycle.
    Build_نتيجو buildSWithin(AstSWithin* nodep, Sva_State_راس* داخلا_Vtxp,
                             منطقي is_چوٽي_Level_قدم = خطأ) {
        ثابت صحيح innerLen = fixed_ڊيگهائي(nodep->lhsp());
        ثابت صحيح outerLen = fixed_ڊيگهائي(nodep->rhsp());
        إذا (innerLen < 0 || outerLen < 0) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: within with ranged cycle-delay operand");
            أرجع Build_نتيجو::failWithError();
        }
        إذا (innerLen > outerLen) {
            أرجع build_Never_ميلاپ_Intersect(
                nodep, داخلا_Vtxp, is_چوٽي_Level_قدم,
                "the inner sequence is longer than the outer sequence");
        }
        File_سٽ* ثابت flp = nodep->fileline();
        ثابت صحيح slack = outerLen - innerLen;
        Ast_ڳنڍ_اظهار* innerOrp = خالي_اشارو;
        لكل (صحيح i = 0; i <= slack; ++i) {
            ثابت صحيح postPad = slack - i;
            Ast_ڳنڍ_اظهار* branchp = nodep->lhsp()->clone_وڻ_خالص(خطأ);
            إذا (i > 0) {
                AstConst* ثابت prePadp = جديد AstConst{flp, AstConst::ٻٽو_انگ_True{}};
                AstDelay* ثابت delayp
                    = جديد AstDelay{flp, جديد AstConst{flp, static_cast<uint32_t>(i)}, سچ};
                AstSExpr* ثابت wrapped = جديد AstSExpr{flp, prePadp, delayp, branchp};
                wrapped->dtype_سيٽ_ٻٽو_انگ();
                branchp = wrapped;
            }
            إذا (postPad > 0) {
                AstConst* ثابت postTruep = جديد AstConst{flp, AstConst::ٻٽو_انگ_True{}};
                AstDelay* ثابت delayp
                    = جديد AstDelay{flp, جديد AstConst{flp, static_cast<uint32_t>(postPad)}, سچ};
                AstSExpr* ثابت wrapped = جديد AstSExpr{flp, branchp, delayp, postTruep};
                wrapped->dtype_سيٽ_ٻٽو_انگ();
                branchp = wrapped;
            }
            innerOrp = innerOrp ? static_cast<Ast_ڳنڍ_اظهار*>(جديد AstSOr{flp, innerOrp, branchp})
                                : branchp;
        }
        Ast_ڳنڍ_اظهار* ثابت outerClonep = nodep->rhsp()->clone_وڻ_خالص(خطأ);
        Ast_ڳنڍ_اظهار* ثابت combinedp = جديد AstSIntersect{flp, innerOrp, outerClonep};
        Build_نتيجو نتيجو = build_اظهار(combinedp, داخلا_Vtxp, is_چوٽي_Level_قدم);
        VL_DO_DANGLING(combinedp->delete_وڻ(), combinedp);
        // When both operands are plain booleans, buildAndCombiner returns a
        // freshly-allocated AstAnd as finalCondp with no parent. Callers up
        // the chain clone-and-discard finalCondp, so leave it parent-less and
        // it leaks; anchor it in the graph now via an edge.
        إذا (نتيجو.valid() && نتيجو.finalCondp && !نتيجو.finalCondp->backp()) {
            Sva_State_راس* ثابت wrapVtxp = scoped_Create_راس();
            guardedLink(نتيجو.رڪن_Vertexp, wrapVtxp, sampled(نتيجو.finalCondp), flp);
            نتيجو = {wrapVtxp, خالي_اشارو, نتيجو.midSources};
        }
        أرجع نتيجو;
    }

    // Collect the boolean leaf checks of a fixed-length sequence keyed by their
    // clock offset from the start. Returns false for anything other than nested
    // AstSExpr with fixed cycle delays over boolean leaves (e.g. throughout).
    ساكن منطقي flattenFixedSeq(Ast_ڳنڍ_اظهار* nodep, صحيح بنياد_وٿي,
                                std::map<صحيح, std::ويڪٽر<Ast_ڳنڍ_اظهار*>>& ٻاهريون) {
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
            إذا (!delayp || !delayp->is_چڪر_Delay() || delayp->isUnbounded()) أرجع خطأ;
            ثابت بنانشان delay_چڪرن = getConstUInt(delayp->lhsp());
            إذا (delayp->isRangeDelay() && getConstUInt(delayp->rhsp()) != delay_چڪرن)
                أرجع خطأ;
            صحيح preLen = 0;
            إذا (Ast_ڳنڍ_اظهار* ثابت prep = sexprp->preExprp()) {
                إذا (!flattenFixedSeq(prep, بنياد_وٿي, ٻاهريون)) أرجع خطأ;
                preLen = fixed_ڊيگهائي(prep);
                إذا (preLen < 0) أرجع خطأ;
            }
            أرجع flattenFixedSeq(sexprp->exprp(), بنياد_وٿي + preLen + delay_چڪرن, ٻاهريون);
        }
        إذا (nodep->is_Multi_چڪر_Sva()) أرجع خطأ;
        ٻاهريون[بنياد_وٿي].ڌڪو_پٺ(nodep);
        أرجع سچ;
    }

    // Conjoin two equal-length fixed sequences into one: at each clock offset
    // AND the boolean checks of both operands (IEEE 1800-2023 16.9.6 -- both
    // operands match the same window). Returns null if either operand is not a
    // plain fixed sequence of boolean leaves.
    ساكن Ast_ڳنڍ_اظهار* conjoinFixedSeqs(Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp, File_سٽ* flp) {
        std::map<صحيح, std::ويڪٽر<Ast_ڳنڍ_اظهار*>> checks;
        إذا (!flattenFixedSeq(lhsp, 0, checks) || !flattenFixedSeq(rhsp, 0, checks)) أرجع خالي_اشارو;
        إذا (checks.خالي_جايون()) أرجع خالي_اشارو;
        Ast_ڳنڍ_اظهار* resultp = خالي_اشارو;
        صحيح prev_وٿي = 0;
        لكل (ثابت خودڪار& وٿي_Checks : checks) {
            ثابت صحيح وٿي = وٿي_Checks.پهريون;
            Ast_ڳنڍ_اظهار* condp = خالي_اشارو;
            لكل (Ast_ڳنڍ_اظهار* ثابت leafp : وٿي_Checks.ٻيو) {
                Ast_ڳنڍ_اظهار* ثابت clonep = leafp->clone_وڻ_خالص(خطأ);
                إذا (!condp) {
                    condp = clonep;
                } وإلا {
                    condp = جديد AstLogAnd{flp, condp, clonep};
                    condp->dtype_سيٽ_ٻٽو_انگ();
                }
            }
            إذا (!resultp) {
                إذا (وٿي > 0) {
                    AstDelay* ثابت delayp = جديد AstDelay{
                        flp, جديد AstConst{flp, static_cast<uint32_t>(وٿي)}, /*isCycle=*/سچ};
                    resultp
                        = جديد AstSExpr{flp, جديد AstConst{flp, AstConst::ٻٽو_انگ_True{}}, delayp, condp};
                    resultp->dtype_سيٽ_ٻٽو_انگ();
                } وإلا {
                    resultp = condp;
                }
            } وإلا {
                AstDelay* ثابت delayp = جديد AstDelay{
                    flp, جديد AstConst{flp, static_cast<uint32_t>(وٿي - prev_وٿي)},
                    /*isCycle=*/سچ};
                resultp = جديد AstSExpr{flp, resultp, delayp, condp};
                resultp->dtype_سيٽ_ٻٽو_انگ();
            }
            prev_وٿي = وٿي;
        }
        أرجع resultp;
    }

    // `seq` is a simple ranged sequence `start ##[m:n] end` (start/end boolean,
    // start may be absent). Used to collapse a both-variable intersect to one
    // ranged delay.
    بنية SimpleRanged final {
        منطقي ok = خطأ;
        Ast_ڳنڍ_اظهار* startp = خالي_اشارو;  // may be null (absent start)
        Ast_ڳنڍ_اظهار* endp = خالي_اشارو;
    };
    ساكن SimpleRanged asSimpleRanged(Ast_ڳنڍ_اظهار* nodep) {
        AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr);
        إذا (!sexprp) أرجع {};
        AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
        إذا (!delayp || !delayp->is_چڪر_Delay() || !delayp->isRangeDelay() || delayp->isUnbounded())
            أرجع {};
        إذا (getConstUInt(delayp->lhsp()) == getConstUInt(delayp->rhsp())) أرجع {};
        Ast_ڳنڍ_اظهار* ثابت prep = sexprp->preExprp();
        إذا (prep && fixed_ڊيگهائي(prep) != 0) أرجع {};
        إذا (fixed_ڊيگهائي(sexprp->exprp()) != 0) أرجع {};
        أرجع {سچ, prep, sexprp->exprp()};
    }

    // Build the NFA for a synthesized intersect lowering tree, then free it.
    // buildExpr returns the terminal condition (finalCondp) by reference into the
    // tree; detach a clone so the tree can be freed here. The graph already holds
    // clones/hoists of every edge condition, so nothing else dangles.
    Build_نتيجو build_From_Lowering_وڻ(Ast_ڳنڍ_اظهار* treep, Sva_State_راس* داخلا_Vtxp,
                                      منطقي is_چوٽي_Level_قدم) {
        Build_نتيجو نتيجو = build_اظهار(treep, داخلا_Vtxp, is_چوٽي_Level_قدم);
        إذا (نتيجو.valid() && نتيجو.finalCondp) {
            نتيجو.finalCondp = نتيجو.finalCondp->clone_وڻ_خالص(خطأ);
        }
        VL_DO_DANGLING(treep->delete_وڻ(), treep);
        أرجع نتيجو;
    }

    // Empty common-length intersection -- unequal fixed lengths, or disjoint
    // ranged lengths. IEEE 1800-2023 16.9.6 requires both operands to match
    // over a window of the same length, so with no common length the intersect
    // simply never matches. This is legal (matching nothing), not an error, so
    // lower to a constant false rather than rejecting legal code.
    Build_نتيجو build_Never_ميلاپ_Intersect(Ast_ڳنڍ_اظهار* nodep, Sva_State_راس* داخلا_Vtxp,
                                         منطقي is_چوٽي_Level_قدم, ثابت حرف* reason) {
        nodep->v3warn(NEVERMATCH, "Sequence can never match because " << reason << ".");
        Ast_ڳنڍ_اظهار* ثابت falsep = جديد AstConst{nodep->fileline(), AstConst::ٻٽو_انگ_False{}};
        أرجع build_From_Lowering_وڻ(falsep, داخلا_Vtxp, is_چوٽي_Level_قدم);
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
    Build_نتيجو buildVarLenIntersect(AstSIntersect* nodep, Sva_State_راس* داخلا_Vtxp,
                                     منطقي is_چوٽي_Level_قدم) {
        ثابت std::pair<صحيح, صحيح> lhsRange = ڊيگهائي_Range(nodep->lhsp());
        ثابت std::pair<صحيح, صحيح> rhsRange = ڊيگهائي_Range(nodep->rhsp());
        إذا (lhsRange.پهريون < 0 || rhsRange.پهريون < 0) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect with this variable-length operand");
            أرجع Build_نتيجو::failWithError();
        }
        ثابت صحيح lo = std::max(lhsRange.پهريون, rhsRange.پهريون);
        ثابت صحيح hi = std::min(lhsRange.ٻيو, rhsRange.ٻيو);
        إذا (lo > hi) {
            // Disjoint length ranges share no common length -> never matches.
            أرجع build_Never_ميلاپ_Intersect(nodep, داخلا_Vtxp, is_چوٽي_Level_قدم,
                                            "intersect operands have no common length");
        }
        File_سٽ* ثابت flp = nodep->fileline();
        إذا (lo == hi) {
            Ast_ڳنڍ_اظهار* ثابت lp = realize_At_ڊيگهائي(nodep->lhsp(), lo, lhsRange.پهريون);
            Ast_ڳنڍ_اظهار* ثابت rp = realize_At_ڊيگهائي(nodep->rhsp(), lo, rhsRange.پهريون);
            Ast_ڳنڍ_اظهار* ثابت conjp = conjoinFixedSeqs(lp, rp, flp);
            VL_DO_DANGLING(lp->delete_وڻ(), lp);
            VL_DO_DANGLING(rp->delete_وڻ(), rp);
            إذا (!conjp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: intersect operand is not a plain boolean sequence");
                أرجع Build_نتيجو::failWithError();
            }
            أرجع build_From_Lowering_وڻ(conjp, داخلا_Vtxp, is_چوٽي_Level_قدم);
        }
        ثابت SimpleRanged sl = asSimpleRanged(nodep->lhsp());
        ثابت SimpleRanged sr = asSimpleRanged(nodep->rhsp());
        إذا (!sl.ok || !sr.ok) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect of two sequences that each vary in length over a"
                          " range with internal structure");
            أرجع Build_نتيجو::failWithError();
        }
        ثابت خودڪار andBool = [&](Ast_ڳنڍ_اظهار* ap, Ast_ڳنڍ_اظهار* bp) -> Ast_ڳنڍ_اظهار* {
            Ast_ڳنڍ_اظهار* ثابت aClonep
                = ap ? ap->clone_وڻ_خالص(خطأ) : جديد AstConst{flp, AstConst::ٻٽو_انگ_True{}};
            Ast_ڳنڍ_اظهار* ثابت bClonep
                = bp ? bp->clone_وڻ_خالص(خطأ) : جديد AstConst{flp, AstConst::ٻٽو_انگ_True{}};
            AstLogAnd* ثابت andp = جديد AstLogAnd{flp, aClonep, bClonep};
            andp->dtype_سيٽ_ٻٽو_انگ();
            أرجع andp;
        };
        AstDelay* ثابت delayp = جديد AstDelay{flp, جديد AstConst{flp, static_cast<uint32_t>(lo)},
                                              /*isCycle=*/سچ};
        delayp->rhsp(جديد AstConst{flp, static_cast<uint32_t>(hi)});
        AstSExpr* ثابت reducedp
            = جديد AstSExpr{flp, andBool(sl.startp, sr.startp), delayp, andBool(sl.endp, sr.endp)};
        reducedp->dtype_سيٽ_ٻٽو_انگ();
        أرجع build_From_Lowering_وڻ(reducedp, داخلا_Vtxp, is_چوٽي_Level_قدم);
    }

    Build_نتيجو buildThroughout(AstSThroughout* nodep, Sva_State_راس* داخلا_Vtxp,
                                منطقي is_چوٽي_Level_قدم = خطأ) {
        // Mark entryVtxp so "cond false at tick 0" is detected as throughout-drop.
        داخلا_Vtxp->m_throughoutConds.ڌڪو_پٺ(nodep->lhsp()->clone_وڻ_خالص(خطأ));
        m_temporal_Guard_ڍير.ڌڪو_پٺ(nodep->lhsp());
        ثابت Build_نتيجو نتيجو = build_اظهار(nodep->rhsp(), داخلا_Vtxp, is_چوٽي_Level_قدم);
        m_temporal_Guard_ڍير.ڪڍو_پٺ();
        أرجع نتيجو;
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
    Build_نتيجو buildUntil(AstUntil* nodep, Sva_State_راس* داخلا_Vtxp, منطقي is_چوٽي_Level_قدم) {
        File_سٽ* ثابت flp = nodep->fileline();
        إذا (!is_چوٽي_Level_قدم) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            أرجع Build_نتيجو::failWithError();
        }
        Ast_ڳنڍ_اظهار* ثابت lhsp = nodep->lhsp();
        Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp();
        ثابت خودڪار hasSeq = [](ثابت Ast_ڳنڍ_اظهار* ep) {
            أرجع ep->exists([](ثابت Ast_ڳنڍ_اظهار* np) { أرجع np->is_Multi_چڪر_Sva(); });
        };
        إذا (hasSeq(lhsp) || hasSeq(rhsp)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            أرجع Build_نتيجو::failWithError();
        }

        ثابت منطقي ov = nodep->isOverlapping();
        Ast_ڳنڍ_اظهار* ثابت lhsBitp = nodep->lhsp();
        Ast_ڳنڍ_اظهار* ثابت rhsBitp = nodep->rhsp();
        // p hoist count: continue, require (ov: 1 use; nov: 1 use). At least 2 uses.
        AstVar* ثابت pHoistp = tryHoistSampled(lhsBitp, flp, 2);
        // q hoist count: continue (1) + require nov (1) = 2; ov: continue only (1).
        AstVar* ثابت qHoistp = ov ? خالي_اشارو : tryHoistSampled(rhsBitp, flp, 2);

        Sva_State_راس* ثابت waitCp = scoped_Create_راس();
        Sva_State_راس* ثابت waitRp = scoped_Create_راس();
        waitCp->m_isUnbounded = سچ;
        waitRp->m_strongPending = nodep->isStrong() && !m_isCover;

        // Entry and back-loop Links carry no condition; throughout-folding still applies.
        guardedLink(داخلا_Vtxp, waitCp, flp);
        guardedLink(waitRp, waitCp, flp);

        // Continue clocked edge: p && !q advances to next-cycle wait.
        Ast_ڳنڍ_اظهار* ثابت contCondp
            = جديد AstLogAnd{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                            جديد AstLogNot{flp, sampledRefOrClone(qHoistp, rhsBitp, flp)}};
        guarded_ڪنارو(waitCp, waitRp, contCondp, flp);

        // Reject sink: fires when require-condition is false.
        Sva_State_راس* ثابت sinkVtxp = m_گراف.create_State_راس();
        sinkVtxp->m_isRejectSink = سچ;
        Ast_ڳنڍ_اظهار* requireCondp;
        إذا (ov) {
            requireCondp = sampledRefOrClone(pHoistp, lhsBitp, flp);
        } وإلا {
            requireCondp = جديد AstLogOr{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                                        sampledRefOrClone(qHoistp, rhsBitp, flp)};
        }
        Sva_Trans_ڪنارو* ثابت rejEdgep = m_گراف.جوڙيو_Link(waitCp, sinkVtxp, requireCondp);
        rejEdgep->m_rejectOnFail = سچ;

        // Accept condition rides via finalCondp; assembleResult $sampled-wraps it.
        Ast_ڳنڍ_اظهار* acceptCondp;
        إذا (ov) {
            acceptCondp
                = جديد AstLogAnd{flp, lhsBitp->clone_وڻ_خالص(خطأ), rhsBitp->clone_وڻ_خالص(خطأ)};
        } وإلا {
            acceptCondp = rhsBitp->clone_وڻ_خالص(خطأ);
        }
        أرجع {waitCp, acceptCondp, {}};
    }

    // IEEE 1800-2023 16.12.14 property abort operators. Sync and async share
    // the same NFA encoding: AstSampled already gives matured values at every
    // maturing clocking event, and async firing "between clocks" is not
    // observable in a cycle-based model. VAbortKind selects accept vs reject
    // verdict (sync vs async only changes the user-visible spelling).

    // Build `condp && !outer_1 && !outer_2 ...` (unsampled).
    Ast_ڳنڍ_اظهار* abort_Fire_اظهار(Ast_ڳنڍ_اظهار* condp, File_سٽ* flp) {
        Ast_ڳنڍ_اظهار* resultp = condp->clone_وڻ_خالص(خطأ);
        لكل (Ast_ڳنڍ_اظهار* ثابت op : m_outer_Abort_ڍير)
            resultp = جديد AstLogAnd{flp, resultp, جديد AstLogNot{flp, op->clone_وڻ_خالص(خطأ)}};
        أرجع resultp;
    }

    // True when unguarded ring-wide rejection or a same-tick Link chain already accounts the
    // attempt: a required-step Link covers both outcomes; followed-by pairs both edges.
    ساكن منطقي chain_Accounts_ذريعو(ثابت Sva_State_راس* srcp,
                                    ثابت std::unordered_سيٽ<ثابت V3_گراف_ڪنارو*>& pre_ڪنارا) {
        إذا (srcp->m_delay_Ring_سائيز && srcp->m_throughoutConds.خالي_جايون()) أرجع سچ;
        لكل (ثابت V3_گراف_ڪنارو& edger : srcp->اندريون_ڪنارا()) {
            إذا (pre_ڪنارا.ڳڻپ(&edger)) استمر;
            ثابت Sva_Trans_ڪنارو& tedger = static_cast<ثابت Sva_Trans_ڪنارو&>(edger);
            إذا (tedger.m_consumes_چڪر) استمر;
            ثابت خودڪار* ثابت fromp = static_cast<ثابت Sva_State_راس*>(tedger.fromVtxp());
            إذا (fromp->m_abortRejectp) أرجع سچ;
        }
        منطقي plainNonSink = خطأ;
        منطقي markedSink = خطأ;
        لكل (ثابت V3_گراف_ڪنارو& edger : srcp->ٻاهريون_ڪنارا()) {
            إذا (pre_ڪنارا.ڳڻپ(&edger)) استمر;
            ثابت Sva_Trans_ڪنارو& tedger = static_cast<ثابت Sva_Trans_ڪنارو&>(edger);
            إذا (tedger.m_consumes_چڪر) استمر;
            ثابت منطقي sink = static_cast<ثابت Sva_State_راس*>(tedger.منزل_Vtxp())->m_isRejectSink;
            إذا (tedger.m_rejectOnFail) {
                إذا (!sink) أرجع سچ;
                markedSink = سچ;
            } وإلا إذا (!sink) {
                إذا (srcp->m_abortRejectp) أرجع سچ;
                plainNonSink = سچ;
            }
        }
        أرجع plainNonSink && markedSink;
    }

    // Reject edge: fires when the source is live and the abort samples true.
    فراغ جوڙيو_Abort_Reject_ڪنارو(Sva_State_راس* srcp, Sva_State_راس* sinkp, Ast_ڳنڍ_اظهار* condp,
                            File_سٽ* flp) {
        Ast_ڳنڍ_اظهار* ثابت notFirep = جديد AstLogNot{flp, sampled(abort_Fire_اظهار(condp, flp))};
        m_گراف.جوڙيو_Link(srcp, sinkp, notFirep)->m_rejectOnFail = سچ;
        أرجع;
    }

    // On the fire tick: kill body threads; accept kinds also forgive step misses.
    فراغ gate_Body_ڪنارا_On_Abort(ثابت std::unordered_سيٽ<ثابت V3_گراف_ڪنارو*>& pre_ڪنارا,
                              Ast_ڳنڍ_اظهار* condp, VAbort_قسم قسم, File_سٽ* flp) {
        لكل (V3_گراف_راس& vtxr : m_گراف.m_گراف.vertices()) {
            لكل (V3_گراف_ڪنارو& edger : vtxr.ٻاهريون_ڪنارا()) {
                إذا (pre_ڪنارا.ڳڻپ(&edger)) استمر;
                Sva_Trans_ڪنارو* ثابت tedgep = static_cast<Sva_Trans_ڪنارو*>(&edger);
                إذا (tedgep->m_rejectOnFail) {
                    إذا (!قسم.isAccept()) استمر;
                    Ast_ڳنڍ_اظهار* ثابت firep = sampled(abort_Fire_اظهار(condp, flp));
                    tedgep->m_condp
                        = tedgep->m_condp ? جديد AstLogOr{flp, tedgep->m_condp, firep} : firep;
                } وإلا إذا (tedgep->m_consumes_چڪر) {
                    Ast_ڳنڍ_اظهار* ثابت notFirep
                        = جديد AstLogNot{flp, sampled(abort_Fire_اظهار(condp, flp))};
                    tedgep->m_condp = tedgep->m_condp
                                          ? جديد AstLogAnd{flp, tedgep->m_condp, notFirep}
                                          : notFirep;
                }
            }
        }
    }

    Build_نتيجو buildAbortOn(Ast_ڳنڍ_اظهار* condp, Ast_ڳنڍ_اظهار* bodyp, Sva_State_راس* داخلا_Vtxp,
                             VAbort_قسم قسم, File_سٽ* flp, منطقي is_چوٽي_Level_قدم) {
        // Snapshot pre-body vertices/edges so post-build diff yields the body's sub-NFA.
        std::unordered_سيٽ<ثابت V3_گراف_راس*> preExisting;
        std::unordered_سيٽ<ثابت V3_گراف_ڪنارو*> pre_ڪنارا;
        لكل (V3_گراف_راس& vtxr : m_گراف.m_گراف.vertices()) {
            preExisting.وجهو(&vtxr);
            لكل (V3_گراف_ڪنارو& edger : vtxr.ٻاهريون_ڪنارا()) pre_ڪنارا.وجهو(&edger);
        }

        m_outer_Abort_ڍير.ڌڪو_پٺ(condp);
        ثابت Build_نتيجو body_نتيجو = build_اظهار(bodyp, داخلا_Vtxp, is_چوٽي_Level_قدم);
        m_outer_Abort_ڍير.ڪڍو_پٺ();
        إذا (!body_نتيجو.valid()) أرجع body_نتيجو;

        gate_Body_ڪنارا_On_Abort(pre_ڪنارا, condp, قسم, flp);

        // Live-thread sources for the abort edge: entry + new body vertices,
        // minus reject sinks (they carry reject fuel, not live-thread fuel).
        std::ويڪٽر<Sva_State_راس*> abortSources;
        abortSources.ڌڪو_پٺ(داخلا_Vtxp);
        لكل (V3_گراف_راس& vtxr : m_گراف.m_گراف.vertices()) {
            إذا (preExisting.ڳڻپ(&vtxr)) استمر;
            خودڪار* ثابت sp = static_cast<Sva_State_راس*>(&vtxr);
            إذا (sp->m_delay_Ring_سائيز || !sp->m_throughoutConds.خالي_جايون()) {
                Ast_ڳنڍ_اظهار* ثابت firep = abort_Fire_اظهار(condp, flp);
                sp->m_abortClearp
                    = sp->m_abortClearp ? جديد AstLogOr{flp, sp->m_abortClearp, firep} : firep;
            }
            إذا (!قسم.isAccept()
                && ((sp->m_delay_Ring_سائيز && sp->m_throughoutConds.خالي_جايون())
                    || sp->m_replayAbortReject)) {
                Ast_ڳنڍ_اظهار* ثابت firep = abort_Fire_اظهار(condp, flp);
                sp->m_abortRejectp
                    = sp->m_abortRejectp ? جديد AstLogOr{flp, sp->m_abortRejectp, firep} : firep;
            }
            إذا (sp->m_isRejectSink) استمر;
            abortSources.ڌڪو_پٺ(sp);
        }

        خودڪار sampledAbortFire = [&]() -> Ast_ڳنڍ_اظهار* {
            Ast_ڳنڍ_اظهار* ثابت اظهار = abort_Fire_اظهار(condp, flp);
            أرجع sampled(اظهار);
        };

        إذا (قسم.isAccept()) {
            // Match-only sink fed by $sampled(abort-fire) from every live source;
            // registered as midSource so it never contributes a reject.
            Sva_State_راس* ثابت acceptSinkp = scoped_Create_راس();
            لكل (Sva_State_راس* ثابت srcp : abortSources)
                guardedLink(srcp, acceptSinkp, sampledAbortFire(), flp);
            std::ويڪٽر<Sva_State_راس*> midSources = body_نتيجو.midSources;
            midSources.ڌڪو_پٺ(acceptSinkp);
            Ast_ڳنڍ_اظهار* finalCondp = body_نتيجو.finalCondp;
            إذا (finalCondp) {
                إذا (finalCondp->backp()) finalCondp = finalCondp->clone_وڻ_خالص(خطأ);
                finalCondp = جديد AstLogOr{flp, finalCondp, abort_Fire_اظهار(condp, flp)};
            }
            أرجع {body_نتيجو.رڪن_Vertexp, finalCondp, std::move(midSources)};
        }

        // rejectOnFail treats m_condp as the success condition and fires on
        // !condp, so the edge carries !sampledAbortFire().
        Sva_State_راس* ثابت rejectSinkp = m_گراف.create_State_راس();
        rejectSinkp->m_isRejectSink = سچ;
        لكل (Sva_State_راس* ثابت srcp : abortSources)
            إذا (!chain_Accounts_ذريعو(srcp, pre_ڪنارا))
                جوڙيو_Abort_Reject_ڪنارو(srcp, rejectSinkp, condp, flp);
        Ast_ڳنڍ_اظهار* finalCondp = body_نتيجو.finalCondp;
        إذا (finalCondp) {
            إذا (finalCondp->backp()) finalCondp = finalCondp->clone_وڻ_خالص(خطأ);
            finalCondp
                = جديد AstLogAnd{flp, finalCondp, جديد AstLogNot{flp, abort_Fire_اظهار(condp, flp)}};
        }
        أرجع {body_نتيجو.رڪن_Vertexp, finalCondp, body_نتيجو.midSources};
    }

عام:
    // Free a dropped sub-result condition that is not linked into the AST
    // (abort folds synthesize unparented finalCondp trees).
    ساكن فراغ freeUnlinkedCondp(Ast_ڳنڍ_اظهار* condp) {
        إذا (condp && !condp->backp()) VL_DO_DANGLING(condp->delete_وڻ(), condp);
    }

    SvaNfaBuilder(Sva_گراف& گراف, Ast_ڳنڍ_Module* modp, V3_Unique_نالا& prop_عارضي_نالا,
                  منطقي isCoverSeq = خطأ, منطقي isSeqEvent = خطأ, منطقي isCover = خطأ)
        : m_گراف{گراف}
        , m_modp{modp}
        , m_prop_عارضي_نالا{prop_عارضي_نالا}
        , m_isCover{isCover}
        , m_isCoverSeq{isCoverSeq}
        , m_isSeqEvent{isSeqEvent} {}

    // Reset scope between antecedent and consequent: liveness must not leak.
    // m_outerAbortStack survives: an abort wrapping the implication covers the
    // consequent too (IEEE 1800-2023 16.12.14).
    فراغ resetScope() {
        m_اندريون_Unbounded_Scope = خطأ;
        m_temporal_Guard_ڍير.clear();
    }

    Build_نتيجو build_اظهار(Ast_ڳنڍ_اظهار* nodep, Sva_State_راس* داخلا_Vtxp,
                          منطقي is_چوٽي_Level_قدم = خطأ) {
        VL_RESTORER(m_want_Bool_پن);
        ثابت منطقي want_Bool_پن = m_want_Bool_پن;
        m_want_Bool_پن = خطأ;
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            أرجع buildSExpr(sexprp, داخلا_Vtxp, is_چوٽي_Level_قدم);
        }
        إذا (AstSConsRep* ثابت repp = VN_CAST(nodep, SConsRep)) {
            أرجع buildConsRep(repp, داخلا_Vtxp, is_چوٽي_Level_قدم);
        }
        إذا (AstPropAlways* ثابت alwaysp = VN_CAST(nodep, PropAlways)) {
            أرجع buildPropAlways(alwaysp, داخلا_Vtxp, is_چوٽي_Level_قدم);
        }
        إذا (AstSGotoRep* ثابت repp = VN_CAST(nodep, SGotoRep)) {
            أرجع buildGotoRep(repp, داخلا_Vtxp);
        }
        إذا (AstSThroughout* ثابت throughoutp = VN_CAST(nodep, SThroughout)) {
            أرجع buildThroughout(throughoutp, داخلا_Vtxp, is_چوٽي_Level_قدم);
        }
        إذا (AstSOr* ثابت orp = VN_CAST(nodep, SOr)) {
            أرجع build_Or_ملايو(orp->lhsp(), orp->rhsp(), داخلا_Vtxp, orp->fileline(),
                                want_Bool_پن);
        }
        إذا (AstLogOr* ثابت orp = VN_CAST(nodep, LogOr)) {
            أرجع build_Or_ملايو(orp->lhsp(), orp->rhsp(), داخلا_Vtxp, orp->fileline(),
                                want_Bool_پن);
        }
        إذا (AstSAnd* ثابت andp = VN_CAST(nodep, SAnd)) {
            أرجع buildAndCombiner(andp->lhsp(), andp->rhsp(), داخلا_Vtxp, andp->fileline());
        }
        إذا (AstSIntersect* ثابت intp = VN_CAST(nodep, SIntersect)) {
            // IEEE 1800-2023 16.9.6: both operands match over one window with
            // equal start and end (equal length). Lower to a single sequence
            // that conjoins both operands' per-cycle checks -- correct under
            // concurrent attempts, where the done-latch combiner conflates the
            // two operands' start times and over-accepts. The combiner remains
            // only as a fallback for operands that do not flatten.
            ثابت صحيح lhsLen = fixed_ڊيگهائي(intp->lhsp());
            ثابت صحيح rhsLen = fixed_ڊيگهائي(intp->rhsp());
            إذا (lhsLen >= 0 && rhsLen >= 0) {
                إذا (lhsLen != rhsLen) {
                    // Unequal fixed lengths share no common length -> never matches.
                    أرجع build_Never_ميلاپ_Intersect(intp, داخلا_Vtxp, is_چوٽي_Level_قدم,
                                                    "intersect operands have no common length");
                }
                إذا (Ast_ڳنڍ_اظهار* ثابت conjp
                    = conjoinFixedSeqs(intp->lhsp(), intp->rhsp(), intp->fileline())) {
                    أرجع build_From_Lowering_وڻ(conjp, داخلا_Vtxp, is_چوٽي_Level_قدم);
                }
                أرجع buildAndCombiner(intp->lhsp(), intp->rhsp(), داخلا_Vtxp, intp->fileline());
            }
            أرجع buildVarLenIntersect(intp, داخلا_Vtxp, is_چوٽي_Level_قدم);
        }
        إذا (AstSWithin* ثابت withinp = VN_CAST(nodep, SWithin)) {
            أرجع buildSWithin(withinp, داخلا_Vtxp, is_چوٽي_Level_قدم);
        }
        إذا (AstAbortOn* ثابت ap = VN_CAST(nodep, AbortOn)) {
            أرجع buildAbortOn(ap->condp(), ap->propp(), داخلا_Vtxp, ap->قسم(), ap->fileline(),
                                is_چوٽي_Level_قدم);
        }
        إذا (VN_IS(nodep, SNonConsRep)) أرجع Build_نتيجو::fail();
        إذا (AstImplication* ثابت implp = VN_CAST(nodep, Implication)) {
            أرجع build_Implication_ڪنارا(implp->lhsp(), implp->rhsp(), داخلا_Vtxp,
                                         implp->isOverlapped(), implp->isFollowedBy(),
                                         implp->lhsp(), implp->fileline());
        }
        إذا (AstUntil* ثابت untilp = VN_CAST(nodep, Until)) {
            أرجع buildUntil(untilp, داخلا_Vtxp, is_چوٽي_Level_قدم);
        }
        // Boolean leaf (including LogAnd): return as finalCond
        أرجع {داخلا_Vtxp, nodep, {}};
    }

    // Wire an implication / followed-by from `entryVtxp`: builds the antecedent,
    // emits the match-link (and for followed-by the reject-sink edge), inserts a
    // delay vertex for non-overlapped forms, and builds the body. Used both for
    // nested AstImplication in pexpr position and for the top-level assertion
    // antecedent -- `errorNodep` anchors the "unsupported sequence antecedent"
    // error, which differs between the two call sites.
    Build_نتيجو build_Implication_ڪنارا(Ast_ڳنڍ_اظهار* antExprp, Ast_ڳنڍ_اظهار* bodyExprp,
                                      Sva_State_راس* داخلا_Vtxp, منطقي isOverlapped,
                                      منطقي isFollowedBy, Ast_ڳنڍ* errorNodep, File_سٽ* flp) {
        ثابت Build_نتيجو ant_نتيجو = build_اظهار(antExprp, داخلا_Vtxp);
        إذا (!ant_نتيجو.valid()) أرجع ant_نتيجو;

        // Followed-by requires pure-boolean antecedent for non-vacuous-fail at
        // the attempt-start cycle. IEEE 1800-2023 16.12.9 permits a multi-cycle
        // sequence LHS, so this is an implementation gap rather than illegal SV.
        إذا (isFollowedBy && ant_نتيجو.رڪن_Vertexp != داخلا_Vtxp) {
            errorNodep->v3warn(E_UNSUPPORTED,
                               "Unsupported: sequence expression as antecedent of followed-by"
                               " (#-# / #=#) (IEEE 1800-2023 16.12.9)");
            أرجع Build_نتيجو::failWithError();
        }
        UASSERT_OBJ(!isFollowedBy || ant_نتيجو.finalCondp, errorNodep,
                    "followed-by antecedent terminal at entry must carry finalCondp");

        // Use raw createStateVertex() so trigVtxp starts without liveness --
        // reaching the antecedent terminal is a definitive event.
        Sva_State_راس* ثابت trigVtxp = m_گراف.create_State_راس();
        إذا (ant_نتيجو.finalCondp) {
            m_گراف.جوڙيو_Link(ant_نتيجو.رڪن_Vertexp, trigVtxp,
                            sampled(ant_نتيجو.finalCondp->clone_وڻ_خالص(خطأ)));
            // Followed-by non-vacuous fail: rejectOnFail fires when the attempt
            // is live (termVtx reachable) and sampled(antecedent) is false.
            إذا (isFollowedBy) {
                Sva_State_راس* ثابت sinkVtxp = m_گراف.create_State_راس();
                sinkVtxp->m_isRejectSink = سچ;
                Sva_Trans_ڪنارو* ثابت ep
                    = m_گراف.جوڙيو_Link(ant_نتيجو.رڪن_Vertexp, sinkVtxp,
                                      sampled(ant_نتيجو.finalCondp->clone_وڻ_خالص(خطأ)));
                ep->m_rejectOnFail = سچ;
            }
            // finalCondp is cloned into the Sampled nodes; if the original is
            // not parented anywhere in the AST anymore it must be freed here
            // or ASan flags it as a leak (e.g. t_sequence_bool_ops).
            إذا (!ant_نتيجو.finalCondp->backp()) {
                VL_DO_DANGLING(ant_نتيجو.finalCondp->delete_وڻ(), ant_نتيجو.finalCondp);
            }
        } وإلا {
            m_گراف.جوڙيو_Link(ant_نتيجو.رڪن_Vertexp, trigVtxp);
        }
        resetScope();

        Sva_State_راس* bodyEntryp = trigVtxp;
        إذا (!isOverlapped) {
            Sva_State_راس* ثابت delayVtxp = m_گراف.create_State_راس();
            m_گراف.جوڙيو_Clocked_ڪنارو(trigVtxp, delayVtxp);
            bodyEntryp = delayVtxp;
        }
        // An empty antecedent match must not start the consequent, but a zero-minimum
        // repetition's empty match is not handled here; keep the merge vertex for those
        VL_RESTORER(m_want_Bool_پن);
        m_want_Bool_پن = !isFollowedBy && !hasZeroMinRep(antExprp);
        أرجع build_اظهار(bodyExprp, bodyEntryp, /*isTopLevelStep=*/سچ);
    }

    Build_نتيجو build(Ast_ڳنڍ_اظهار* exprp) {
        m_گراف.m_شروعات_Vertexp = scoped_Create_راس();
        VL_RESTORER(m_want_Bool_پن);
        m_want_Bool_پن = سچ;
        أرجع build_اظهار(exprp, m_گراف.m_شروعات_Vertexp, /*isTopLevelStep=*/سچ);
    }
};

//######################################################################
// NFA Lowering (converts NFA graph to synthesizable AstAlways blocks)

صنف SvaNfaLowering final {
    Ast_ڳنڍ_Module* ثابت m_modp;  // Module to add state vars and always blocks to
    Ast_ڳنڍ_DType* ثابت m_u32DTypep;  // Shared unsigned counter dtype
    V3_Unique_نالا m_نالا{"__Vnfa"};
    سائيز_t m_stat_Delay_Ring_ڪنارو_Visits = 0;  // Delay-ring incoming edges visited

    // Per-lowering shared context (passed to phase sub-functions)
    // Per-vertex lowering state is stored in SvaVertexData and accessed via
    // V3GraphVertex::userp() (see SvaStateVertex::datap()).
    بنية هيٺاهون_Ctx final {
        File_سٽ* flp;  // Source location for generated AST
        Ast_Sen_وڻ* senTreep;  // Clock sensitivity tree
        Ast_ڳنڍ_اظهار* disableExprp;  // disable iff expression (may be nullptr)
        Ast_ڳنڍ_اظهار* ميلاپ_Condp;  // Final boolean match condition (may be nullptr)
        VAssertType assertType;  // Assertion type for control tasks
        VAssertDirectiveType directiveType;  // Directive type for control tasks
        AstVar* killVarp;  // Last observed kill generation
        Sva_گراف& گراف;  // NFA graph
    };

    // Build a match-now expression: stateSig[i] && $sampled(condp)
    ساكن Ast_ڳنڍ_اظهار* build_ميلاپ_Now(File_سٽ* flp, Ast_ڳنڍ_اظهار* stateExprp, Ast_ڳنڍ_اظهار* condp) {
        Ast_ڳنڍ_اظهار* ثابت statep = stateExprp->clone_وڻ_خالص(خطأ);
        إذا (!condp) أرجع statep;
        أرجع جديد AstLogAnd{flp, statep, sampled(condp->clone_وڻ_خالص(خطأ))};
    }
    ساكن Ast_ڳنڍ_اظهار* andCond(File_سٽ* flp, Ast_ڳنڍ_اظهار* exprp, Ast_ڳنڍ_اظهار* condp) {
        إذا (!condp) أرجع exprp;
        أرجع جديد AstLogAnd{flp, exprp, condp->clone_وڻ_خالص(خطأ)};
    }
    // bp is always non-null; only ap can be null (serving as accumulator).
    ساكن Ast_ڳنڍ_اظهار* orExprs(File_سٽ* flp, Ast_ڳنڍ_اظهار* ap, Ast_ڳنڍ_اظهار* bp) {
        إذا (!ap) أرجع bp;
        أرجع جديد AstLogOr{flp, ap, bp};
    }
    ساكن Ast_ڳنڍ_اظهار* جوڙيو_Countp(File_سٽ* ثابت flp, Ast_ڳنڍ_اظهار* ثابت ڪل_Countp,
                                  Ast_ڳنڍ_اظهار* ثابت contributionp,
                                  Ast_ڳنڍ_اظهار* ثابت enablep = خالي_اشارو) {
        // contribution = enable ? contribution : 0;
        Ast_ڳنڍ_اظهار* ثابت enabledContributionp
            = enablep ? جديد AstCond{flp, enablep, contributionp,
                                    newTypedConstp(flp, contributionp->dtypep(), 0)}
                      : contributionp;
        إذا (!ڪل_Countp) أرجع enabledContributionp;
        أرجع جديد Ast_جوڙيو{flp, ڪل_Countp, enabledContributionp};
    }
    ساكن Ast_ڳنڍ_اظهار* gateCheckedCountp(File_سٽ* ثابت flp, Ast_ڳنڍ_اظهار* ثابت countp,
                                          Ast_ڳنڍ_اظهار*& overflowp, Ast_ڳنڍ_اظهار* ثابت enablep) {
        إذا (overflowp) {
            overflowp = جديد AstLogAnd{flp, enablep->clone_وڻ_خالص(خطأ), overflowp};
        }
        أرجع جوڙيو_Countp(flp, خالي_اشارو, countp, enablep);
    }
    ساكن Ast_ڳنڍ_اظهار* جوڙيو_Checked_Countp(File_سٽ* ثابت flp, Ast_ڳنڍ_اظهار* ثابت ڪل_Countp,
                                         Ast_ڳنڍ_اظهار* ثابت contributionp, Ast_ڳنڍ_اظهار*& overflowp,
                                         Ast_ڳنڍ_اظهار* ثابت contributionOverflowp = خالي_اشارو) {
        إذا (contributionOverflowp) { overflowp = orExprs(flp, overflowp, contributionOverflowp); }
        إذا (!ڪل_Countp) أرجع contributionp;
        Ast_ڳنڍ_اظهار* ثابت oldTotalp = ڪل_Countp->clone_وڻ_خالص(خطأ);
        Ast_ڳنڍ_اظهار* ثابت resultp = جديد Ast_جوڙيو{flp, ڪل_Countp, contributionp};
        overflowp
            = orExprs(flp, overflowp, جديد AstLt{flp, resultp->clone_وڻ_خالص(خطأ), oldTotalp});
        أرجع resultp;
    }
    AstIf* new_ڳڻپ_حد_کان_مٿي_Warning(File_سٽ* ثابت flp, Ast_ڳنڍ_اظهار* ثابت condp) ثابت {
        AstDisplay* ثابت dispp = جديد AstDisplay{
            flp, VDisplayType::DT_WARNING,
            "Cover sequence match count overflowed 32-bit storage; results may be incorrect.",
            خالي_اشارو, خالي_اشارو};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstIf* ثابت ifp = جديد AstIf{flp, condp, dispp};
        ifp->branchPred(VBranchPred::BP_UNLIKELY);
        أرجع ifp;
    }
    ساكن Ast_ڳنڍ_اظهار* kill_سرگرم(هيٺاهون_Ctx& c) {
        أرجع جديد AstNeq{c.flp, جديد AstVarRef{c.flp, c.killVarp, VAccess::READ},
                          assertKillGet(c.flp, c.assertType, c.directiveType)};
    }
    ساكن Ast_ڳنڍ_اظهار* not_Kill_سرگرم(هيٺاهون_Ctx& c) { أرجع جديد AstLogNot{c.flp, kill_سرگرم(c)}; }
    ساكن Ast_ڳنڍ_اظهار* gateNotKill(هيٺاهون_Ctx& c, Ast_ڳنڍ_اظهار* exprp) {
        إذا (!exprp) أرجع خالي_اشارو;
        أرجع جديد AstLogAnd{c.flp, exprp, not_Kill_سرگرم(c)};
    }
    ساكن Ast_ڳنڍ_اظهار* ايندڙ_Ring_اشاريو(File_سٽ* flp, Ast_ڳنڍ_اظهار* idxExprp, uint32_t سائيز) {
        ثابت خودڪار u32Const = [flp](uint32_t قدر) {
            أرجع جديد AstConst{flp, AstConst::Widthed_قدر{}, 32, قدر};
        };
        UASSERT_OBJ(سائيز > 0, idxExprp, "Ring size must be positive");
        إذا (سائيز == 1) {
            VL_DO_DANGLING(idxExprp->delete_وڻ(), idxExprp);
            أرجع u32Const(0);
        }
        // idx == size - 1 ? 0 : idx + 1
        Ast_جوڙيو* ثابت addp = جديد Ast_جوڙيو{flp, idxExprp->clone_وڻ_خالص(خطأ), u32Const(1)};
        addp->dtypeFrom(idxExprp);
        AstCond* ثابت condp
            = جديد AstCond{flp, جديد AstEq{flp, idxExprp, u32Const(سائيز - 1)}, u32Const(0), addp};
        condp->dtypeFrom(idxExprp);
        أرجع condp;
    }
    ساكن Ast_ڳنڍ_اظهار* delay_Ring_جڳهه(File_سٽ* flp, Sva_State_راس* vtxp, Ast_ڳنڍ_اظهار* idxExprp,
                                      VAccess access = VAccess::READ) {
        // ring[idx]
        AstVar* ثابت ringp = vtxp->datap()->delayRingVarp;
        AstVarRef* ثابت refp = جديد AstVarRef{flp, ringp, access};
        إذا (vtxp->datap()->delayRingCounts) أرجع جديد AstArraySel{flp, refp, idxExprp};
        أرجع جديد AstSel{flp, refp, idxExprp, 1};
    }
    ساكن Ast_ڳنڍ_اظهار* delay_Ring_سر_ڳڻپ(File_سٽ* flp, Sva_State_راس* vtxp) {
        Ast_ڳنڍ_اظهار* ثابت headp
            = جديد AstVarRef{flp, vtxp->datap()->delay_Ring_سر_Varp, VAccess::READ};
        إذا (vtxp->datap()->delayRingCounts) أرجع headp;
        أرجع جديد AstExtend{flp, headp, vtxp->datap()->delay_Ring_جيئرو_ڳڻپ_Varp->dtypep()->ويڪر()};
    }
    ساكن Ast_ڳنڍ_اظهار* delay_Ring_At_آخري_اشاريو(File_سٽ* ثابت flp, AstVar* ثابت idxp,
                                             ثابت uint32_t سائيز) {
        أرجع جديد AstEq{flp, جديد AstVarRef{flp, idxp, VAccess::READ},
                         جديد AstConst{flp, AstConst::Widthed_قدر{}, 32, سائيز - 1}};
    }
    ساكن Ast_ڳنڍ_اظهار* delay_Ring_Output_ڳڻپ(File_سٽ* ثابت flp, Sva_State_راس* ثابت vtxp) {
        AstVar* ثابت idxp = vtxp->datap()->delayRingIdxVarp;
        ثابت uint32_t سائيز = vtxp->m_delay_Ring_سائيز;
        Ast_ڳنڍ_اظهار* outgoingValidp
            = جديد AstVarRef{flp, vtxp->datap()->delayRingWrappedVarp, VAccess::READ};
        إذا (!vtxp->m_isFixedDelayRing) {
            outgoingValidp
                = جديد AstLogOr{flp, outgoingValidp, delay_Ring_At_آخري_اشاريو(flp, idxp, سائيز)};
        }
        AstVar* ثابت جيئرو_ڳڻپ_Varp = vtxp->datap()->delay_Ring_جيئرو_ڳڻپ_Varp;
        أرجع جديد AstCond{flp, outgoingValidp, delay_Ring_سر_ڳڻپ(flp, vtxp),
                           newTypedConstp(flp, جيئرو_ڳڻپ_Varp->dtypep(), 0)};
    }
    ساكن Ast_ڳنڍ_اظهار* delay_Ring_Has_جيئرو_Threadsp(File_سٽ* ثابت flp,
                                                 AstVar* ثابت جيئرو_ڳڻپ_Varp) {
        // active = live_count != 0;
        أرجع جديد AstNeq{flp, جديد AstVarRef{flp, جيئرو_ڳڻپ_Varp, VAccess::READ},
                          newTypedConstp(flp, جيئرو_ڳڻپ_Varp->dtypep(), 0)};
    }

    // Phase 3 output signals
    بنية Signal_سيٽ final {
        Ast_ڳنڍ_اظهار* terminalActivep = خالي_اشارو;  // OR of all successful terminal matches
        Ast_ڳنڍ_اظهار* ميلاپ_Countp = خالي_اشارو;  // NFA paths completing the sequence this tick
        Ast_ڳنڍ_اظهار* ميلاپ_ڳڻپ_Overflowp = خالي_اشارو;  // Match count overflow this tick
        Ast_ڳنڍ_اظهار* rejectBasep = خالي_اشارو;  // Reject when a terminal match fails
        Ast_ڳنڍ_اظهار* required_قدم_Rejectp = خالي_اشارو;  // Per-source reject from rejectOnFail Links
        Ast_ڳنڍ_اظهار* throughoutRejectp = خالي_اشارو;  // Reject when a throughout guard drops
        Ast_ڳنڍ_اظهار* threadFailCountp = خالي_اشارو;  // Number of threads rejected on this tick
    };

    // Disable condition also aborts attempts between clock events. A constant disable
    // never rises, and $sampled is unsupported in a sensitivity list.
    ساكن منطقي isAsyncDisable(ثابت Ast_ڳنڍ_اظهار* ثابت disableExprp) {
        أرجع disableExprp && !VN_IS(disableExprp, Const)
               && !disableExprp->exists([](ثابت AstSampled*) { أرجع سچ; });
    }

    // Emit the clocked update block of registers that hold pending attempts. The clearp
    // statements reset the same registers: a disable rising between clock events also
    // aborts pending attempts (IEEE 1800-2023 16.12), so clearp runs on that rising edge
    // in a separate block. It must clear unconditionally, as the disable may already be
    // false again when it runs, and the update block must not run on that edge, as it
    // would advance the attempts as if the clock had ticked. Blocks that hold no attempt
    // state, such as the $assertkill acknowledge in emitKillAckNba, do not go through here.
    // Callers pass a null clearp when the disable cannot rise between clock events.
    فراغ جوڙيو_State_Always(هيٺاهون_Ctx& c, Ast_ڳنڍ* bodyp, Ast_ڳنڍ* clearp) {
        m_modp->جوڙيو_Stmtsp(
            جديد AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_وڻ(خطأ), bodyp});
        إذا (!clearp) أرجع;
        m_modp->جوڙيو_Stmtsp(جديد AstAlways{
            c.flp, VAlwaysKwd::ALWAYS,
            جديد Ast_Sen_وڻ{c.flp, جديد Ast_Sen_شي{c.flp, VEdgeType::ET_POSEDGE,
                                                 c.disableExprp->clone_وڻ_خالص(خطأ)}},
            clearp});
    }

    // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
    // Phase 2: State register NBA always block. Each clocked-edge target
    // latches the OR of its incoming contributions.
    فراغ emit_State_رجسٽر_Nba(هيٺاهون_Ctx& c) {
        ثابت منطقي asyncDisable = isAsyncDisable(c.disableExprp);
        Ast_ڳنڍ* bodyp = خالي_اشارو;
        Ast_ڳنڍ* clearp = خالي_اشارو;
        لكل (ثابت V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
            ثابت Sva_State_راس* ثابت vtxp = static_cast<ثابت Sva_State_راس*>(&vtxr);
            إذا (!vtxp->datap()->stateVarp) استمر;

            Ast_ڳنڍ_اظهار* ايندڙ_Statep = خالي_اشارو;
            لكل (ثابت V3_گراف_ڪنارو& edger : vtxp->اندريون_ڪنارا()) {
                ثابت Sva_Trans_ڪنارو& tedger = static_cast<ثابت Sva_Trans_ڪنارو&>(edger);
                إذا (!tedger.m_consumes_چڪر) استمر;
                ثابت Sva_State_راس* ثابت fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Clocked-edge source missing stateSig");

                Ast_ڳنڍ_اظهار* srcSigp = fromVtxp->datap()->stateSigp->clone_وڻ_خالص(خطأ);
                srcSigp = andCond(c.flp, srcSigp, tedger.m_condp);

                إذا (c.disableExprp) {
                    Ast_ڳنڍ_اظهار* ثابت notDisp
                        = جديد AstLogNot{c.flp, c.disableExprp->clone_وڻ_خالص(خطأ)};
                    srcSigp = جديد AstLogAnd{c.flp, srcSigp, notDisp};
                }
                ايندڙ_Statep = orExprs(c.flp, ايندڙ_Statep, srcSigp);
            }

            UASSERT_OBJ(ايندڙ_Statep, vtxp,
                        "Registered vertex has no clocked incoming contribution");
            ايندڙ_Statep = gateNotKill(c, ايندڙ_Statep);

            Ast_سونپيو_Dly* ثابت assignp = جديد Ast_سونپيو_Dly{
                c.flp, جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE}, ايندڙ_Statep};
            bodyp = Ast_ڳنڍ::جوڙيو_ايندڙ_Null(bodyp, assignp);
            إذا (asyncDisable) {
                clearp = Ast_ڳنڍ::جوڙيو_ايندڙ_Null(
                    clearp,
                    جديد Ast_سونپيو_Dly{
                        c.flp, جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE},
                        جديد AstConst{c.flp, AstConst::ٻٽو_انگ_False{}}});
            }
        }
        إذا (bodyp) جوڙيو_State_Always(c, bodyp, clearp);
    }

    // Phase 2b: In-place ring storage and registered ring metadata.
    فراغ emitDelayRingUpdates(هيٺاهون_Ctx& c, ثابت منطقي ڳڻپ_Matches) {
        ثابت منطقي asyncDisable = isAsyncDisable(c.disableExprp);
        لكل (V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
            Sva_State_راس* ثابت vtxp = static_cast<Sva_State_راس*>(&vtxr);
            إذا (!vtxp->datap()->delayRingVarp) استمر;
            AstVar* ثابت idxp = vtxp->datap()->delayRingIdxVarp;
            AstVar* ثابت جيئرو_ڳڻپ_Varp = vtxp->datap()->delay_Ring_جيئرو_ڳڻپ_Varp;
            AstVar* ثابت wrappedp = vtxp->datap()->delayRingWrappedVarp;
            ثابت uint32_t سائيز = static_cast<uint32_t>(vtxp->m_delay_Ring_سائيز);

            Ast_ڳنڍ_اظهار* incomingp = خالي_اشارو;
            Ast_ڳنڍ_اظهار* incomingOverflowp = خالي_اشارو;
            لكل (ثابت V3_گراف_ڪنارو& edger : vtxp->اندريون_ڪنارا()) {
                ++m_stat_Delay_Ring_ڪنارو_Visits;
                ثابت Sva_Trans_ڪنارو& tedger = static_cast<ثابت Sva_Trans_ڪنارو&>(edger);
                UASSERT_OBJ(tedger.m_consumes_چڪر == vtxp->m_isFixedDelayRing, vtxp,
                            "Delay-ring incoming edge kind mismatch");
                ثابت Sva_State_راس* ثابت fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Delay-ring incoming source missing stateSig");
                UASSERT_OBJ(!ڳڻپ_Matches || fromVtxp->datap()->state_ڳڻپ_Sigp, fromVtxp,
                            "Delay-ring incoming source missing stateCountSig");
                // IEEE 1800-2023 16.14.1/16.14.3: assertion alternatives belong to one
                // evaluation; only cover sequence counts each end-of-match.
                Ast_ڳنڍ_اظهار* contribp
                    = ڳڻپ_Matches
                          ? fromVtxp->datap()->state_ڳڻپ_Sigp->clone_وڻ_خالص(خطأ)
                          : جديد AstExtend{c.flp,
                                          fromVtxp->datap()->stateSigp->clone_وڻ_خالص(خطأ),
                                          m_u32DTypep->ويڪر()};
                Ast_ڳنڍ_اظهار* contribOverflowp
                    = ڳڻپ_Matches && fromVtxp->datap()->state_ڳڻپ_حد_کان_مٿي_Sigp
                          ? fromVtxp->datap()->state_ڳڻپ_حد_کان_مٿي_Sigp->clone_وڻ_خالص(خطأ)
                          : خالي_اشارو;
                إذا (tedger.m_condp) {
                    Ast_ڳنڍ_اظهار* ثابت enablep = tedger.m_condp->clone_وڻ_خالص(خطأ);
                    contribp = ڳڻپ_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, enablep)
                                   : جوڙيو_Countp(c.flp, خالي_اشارو, contribp, enablep);
                }
                إذا (c.disableExprp) {
                    Ast_ڳنڍ_اظهار* ثابت notDisp
                        = جديد AstLogNot{c.flp, c.disableExprp->clone_وڻ_خالص(خطأ)};
                    contribp = ڳڻپ_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, notDisp)
                                   : جوڙيو_Countp(c.flp, خالي_اشارو, contribp, notDisp);
                }
                incomingp = ڳڻپ_Matches ? جوڙيو_Checked_Countp(c.flp, incomingp, contribp,
                                                            incomingOverflowp, contribOverflowp)
                                         : جوڙيو_Countp(c.flp, incomingp, contribp);
            }
            UASSERT_OBJ(incomingp, vtxp, "Delay ring has no incoming edge");
            ثابت منطقي storesCounts = vtxp->datap()->delayRingCounts;
            Ast_ڳنڍ_اظهار* ثابت incomingCountp
                = storesCounts
                      ? incomingp->clone_وڻ_خالص(خطأ)
                      : جديد AstExtend{c.flp,
                                      جديد AstNeq{c.flp, incomingp->clone_وڻ_خالص(خطأ),
                                                 newTypedConstp(c.flp, incomingp->dtypep(), 0)},
                                      m_u32DTypep->ويڪر()};
            Ast_ڳنڍ_اظهار* ثابت storedIncomingp
                = storesCounts ? incomingp
                               : جديد AstNeq{c.flp, incomingp,
                                            newTypedConstp(c.flp, incomingp->dtypep(), 0)};
            // Only this block accesses the storage. Other processes read the cached head, so
            // the ring can be updated in place without an NBA shadow copy.
            Ast_سونپيو* ثابت writeIncomingp = جديد Ast_سونپيو{
                c.flp,
                delay_Ring_جڳهه(c.flp, vtxp, جديد AstVarRef{c.flp, idxp, VAccess::READ},
                              VAccess::WRITE),
                storedIncomingp};
            Ast_ڳنڍ* updateBodyp = writeIncomingp;
            // live_count <= live_count - outgoing_count + incoming_count;
            Ast_ڳنڍ_اظهار* ثابت outgoingp = delay_Ring_Output_ڳڻپ(c.flp, vtxp);
            Ast_ڳنڍ_اظهار* ثابت afterOutgoingp
                = جديد AstSub{c.flp, جديد AstVarRef{c.flp, جيئرو_ڳڻپ_Varp, VAccess::READ}, outgoingp};
            Ast_ڳنڍ_اظهار* ثابت oldAfterOutgoingp = afterOutgoingp->clone_وڻ_خالص(خطأ);
            Ast_جوڙيو* ثابت ايندڙ_جيئرو_Countp = جديد Ast_جوڙيو{c.flp, afterOutgoingp, incomingCountp};
            إذا (storesCounts) {
                Ast_ڳنڍ_اظهار* ثابت جيئرو_Overflowp
                    = جديد AstLt{c.flp, ايندڙ_جيئرو_Countp->clone_وڻ_خالص(خطأ), oldAfterOutgoingp};
                Ast_ڳنڍ_اظهار* ثابت overflowp = orExprs(c.flp, incomingOverflowp, جيئرو_Overflowp);
                updateBodyp = new_ڳڻپ_حد_کان_مٿي_Warning(c.flp, overflowp);
                updateBodyp->جوڙيو_ايندڙ(writeIncomingp);
            } وإلا {
                VL_DO_DANGLING(oldAfterOutgoingp->delete_وڻ(), oldAfterOutgoingp);
            }
            // Cache the outgoing slot for the next index after the write. Reading after
            // writing also handles one-slot rings and two-slot range rings without alias cases.
            Ast_ڳنڍ_اظهار* ايندڙ_سر_Idxp
                = ايندڙ_Ring_اشاريو(c.flp, جديد AstVarRef{c.flp, idxp, VAccess::READ}, سائيز);
            إذا (!vtxp->m_isFixedDelayRing) {
                ايندڙ_سر_Idxp = ايندڙ_Ring_اشاريو(c.flp, ايندڙ_سر_Idxp, سائيز);
            }
            updateBodyp->جوڙيو_ايندڙ(جديد Ast_سونپيو_Dly{
                c.flp, جديد AstVarRef{c.flp, vtxp->datap()->delay_Ring_سر_Varp, VAccess::WRITE},
                delay_Ring_جڳهه(c.flp, vtxp, ايندڙ_سر_Idxp)});
            updateBodyp->جوڙيو_ايندڙ(جديد Ast_سونپيو_Dly{
                c.flp, جديد AstVarRef{c.flp, جيئرو_ڳڻپ_Varp, VAccess::WRITE}, ايندڙ_جيئرو_Countp});
            // wrapped <= wrapped || idx == size - 1;
            updateBodyp->جوڙيو_ايندڙ(
                جديد Ast_سونپيو_Dly{c.flp, جديد AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                 جديد AstLogOr{c.flp, جديد AstVarRef{c.flp, wrappedp, VAccess::READ},
                                              delay_Ring_At_آخري_اشاريو(c.flp, idxp, سائيز)}});
            // idx <= next_idx;
            updateBodyp->جوڙيو_ايندڙ(جديد Ast_سونپيو_Dly{
                c.flp, جديد AstVarRef{c.flp, idxp, VAccess::WRITE},
                ايندڙ_Ring_اشاريو(c.flp, جديد AstVarRef{c.flp, idxp, VAccess::READ}, سائيز)});
            إذا (vtxp->m_delayRingAdvanceCondp) {
                updateBodyp = جديد AstIf{
                    c.flp, sampled(vtxp->m_delayRingAdvanceCondp->clone_وڻ_خالص(خطأ)),
                    updateBodyp};
            }

            Ast_ڳنڍ_اظهار* clearCondp = kill_سرگرم(c);
            إذا (vtxp->m_delayRingClearCondp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_delayRingClearCondp->clone_وڻ_خالص(خطأ)));
            }
            إذا (vtxp->m_abortClearp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_abortClearp->clone_وڻ_خالص(خطأ)));
            }
            إذا (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_وڻ_خالص(خطأ));
            }
            Ast_ڳنڍ_اظهار* guardp = خالي_اشارو;
            لكل (Ast_ڳنڍ_اظهار* ثابت cp : vtxp->m_throughoutConds) {
                Ast_ڳنڍ_اظهار* ثابت sampledp = sampled(cp->clone_وڻ_خالص(خطأ));
                guardp = guardp ? static_cast<Ast_ڳنڍ_اظهار*>(جديد AstLogAnd{c.flp, guardp, sampledp})
                                : sampledp;
            }
            إذا (guardp) clearCondp = orExprs(c.flp, clearCondp, جديد AstLogNot{c.flp, guardp});
            // Logically clear the ring without touching its wide storage.
            Ast_سونپيو_Dly* ثابت clearCountp
                = جديد Ast_سونپيو_Dly{c.flp, جديد AstVarRef{c.flp, جيئرو_ڳڻپ_Varp, VAccess::WRITE},
                                   newTypedConstp(c.flp, جيئرو_ڳڻپ_Varp->dtypep(), 0)};
            clearCountp->جوڙيو_ايندڙ(جديد Ast_سونپيو_Dly{c.flp,
                                                  جديد AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                                  جديد AstConst{c.flp, AstConst::ٻٽو_انگ_False{}}});
            clearCountp->جوڙيو_ايندڙ(جديد Ast_سونپيو_Dly{c.flp,
                                                  جديد AstVarRef{c.flp, idxp, VAccess::WRITE},
                                                  newTypedConstp(c.flp, idxp->dtypep(), 0)});
            updateBodyp = جديد AstIf{c.flp, clearCondp, clearCountp, updateBodyp};

            جوڙيو_State_Always(c, updateBodyp, asyncDisable ? clearCountp->clone_وڻ(سچ) : خالي_اشارو);
        }
    }

    // Phase 2c: SAnd combiner done-latch always block.
    // NBA semantics ensure doneL/doneR read pre-update values (IEEE 16.9.5).
    فراغ emit_And_Combiner_پورو_Latch_Nba(هيٺاهون_Ctx& c) {
        ثابت منطقي asyncDisable = isAsyncDisable(c.disableExprp);
        لكل (ثابت V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
            ثابت Sva_State_راس* ثابت avp = static_cast<ثابت Sva_State_راس*>(&vtxr);
            إذا (!avp->datap()->پورو_LVarp) استمر;
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

            Ast_سونپيو_Dly* ثابت clearLp = جديد Ast_سونپيو_Dly{
                c.flp, جديد AstVarRef{c.flp, avp->datap()->پورو_LVarp, VAccess::WRITE},
                جديد AstConst{c.flp, AstConst::ٻٽو_انگ_False{}}};
            Ast_سونپيو_Dly* ثابت clearRp = جديد Ast_سونپيو_Dly{
                c.flp, جديد AstVarRef{c.flp, avp->datap()->پورو_RVarp, VAccess::WRITE},
                جديد AstConst{c.flp, AstConst::ٻٽو_انگ_False{}}};
            clearLp->جوڙيو_ايندڙ(clearRp);

            Ast_ڳنڍ_اظهار* ثابت ميلاپ_LNowp
                = build_ميلاپ_Now(c.flp, lhsTermp->datap()->stateSigp, avp->m_andLhsCondp);
            Ast_ڳنڍ_اظهار* ثابت ميلاپ_RNowp
                = build_ميلاپ_Now(c.flp, rhsTermp->datap()->stateSigp, avp->m_andRhsCondp);
            Ast_ڳنڍ_اظهار* gateLp = ميلاپ_LNowp;
            Ast_ڳنڍ_اظهار* gateRp = ميلاپ_RNowp;
            إذا (c.disableExprp) {
                Ast_ڳنڍ_اظهار* ثابت notDisLp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_وڻ_خالص(خطأ)};
                gateLp = جديد AstLogAnd{c.flp, gateLp, notDisLp};
                Ast_ڳنڍ_اظهار* ثابت notDisRp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_وڻ_خالص(خطأ)};
                gateRp = جديد AstLogAnd{c.flp, gateRp, notDisRp};
            }
            Ast_سونپيو_Dly* ثابت سيٽ_Lp = جديد Ast_سونپيو_Dly{
                c.flp, جديد AstVarRef{c.flp, avp->datap()->پورو_LVarp, VAccess::WRITE},
                جديد AstConst{c.flp, AstConst::ٻٽو_انگ_True{}}};
            AstIf* ثابت سيٽ_LIfp = جديد AstIf{c.flp, gateLp, سيٽ_Lp, خالي_اشارو};
            Ast_سونپيو_Dly* ثابت سيٽ_Rp = جديد Ast_سونپيو_Dly{
                c.flp, جديد AstVarRef{c.flp, avp->datap()->پورو_RVarp, VAccess::WRITE},
                جديد AstConst{c.flp, AstConst::ٻٽو_انگ_True{}}};
            AstIf* ثابت سيٽ_RIfp = جديد AstIf{c.flp, gateRp, سيٽ_Rp, خالي_اشارو};
            سيٽ_LIfp->جوڙيو_ايندڙ(سيٽ_RIfp);

            Ast_ڳنڍ_اظهار* clearCondp = جديد AstLogOr{c.flp, kill_سرگرم(c),
                                                   avp->datap()->stateSigp->clone_وڻ_خالص(خطأ)};
            إذا (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_وڻ_خالص(خطأ));
            }
            AstIf* ثابت topp = جديد AstIf{c.flp, clearCondp, clearLp, سيٽ_LIfp};
            جوڙيو_State_Always(c, topp, asyncDisable ? clearLp->clone_وڻ(سچ) : خالي_اشارو);
        }
    }

    فراغ emitKillAckNba(هيٺاهون_Ctx& c) {
        Ast_سونپيو_Dly* ثابت ackp
            = جديد Ast_سونپيو_Dly{c.flp, جديد AstVarRef{c.flp, c.killVarp, VAccess::WRITE},
                               assertKillGet(c.flp, c.assertType, c.directiveType)};
        m_modp->جوڙيو_Stmtsp(
            جديد AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_وڻ(خطأ), ackp});
    }

    // Phase 3/3a/3b: Compute terminal match/reject signals, required-step reject,
    // throughout-drop reject; clean up intermediate state signals.
    // Phase 3: terminalActive and rejectBase from Links to matchVertex.
    // Builder only adds Links (non-clocked) to matchVertex via addLink in
    // wireMatchAndMidSources. For cover sequence, also count each end-of-match
    // so the action can be replayed without unrolling endpoints.
    فراغ compute_Terminal_ميلاپ_And_Reject(هيٺاهون_Ctx& c, Signal_سيٽ& sigs, ثابت منطقي need_ميلاپ_ڳڻپ) {
        لكل (ثابت Sva_Trans_ڪنارو* ثابت tedgep : c.گراف.all_ڪنارا()) {
            إذا (tedgep->منزل_Vtxp() != c.گراف.m_ميلاپ_Vertexp) استمر;
            Sva_State_راس* ثابت fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                        "Terminal-link source missing stateSig");

            Ast_ڳنڍ_اظهار* srcSigp = fromVtxp->datap()->stateSigp->clone_وڻ_خالص(خطأ);
            srcSigp = andCond(c.flp, srcSigp, tedgep->m_condp);
            إذا (need_ميلاپ_ڳڻپ) {
                UASSERT_OBJ(fromVtxp->datap()->state_ڳڻپ_Sigp, fromVtxp,
                            "Terminal-link source missing stateCountSig");
                Ast_ڳنڍ_اظهار* contributionp
                    = fromVtxp->datap()->state_ڳڻپ_Sigp->clone_وڻ_خالص(خطأ);
                Ast_ڳنڍ_اظهار* contributionOverflowp
                    = fromVtxp->datap()->state_ڳڻپ_حد_کان_مٿي_Sigp
                          ? fromVtxp->datap()->state_ڳڻپ_حد_کان_مٿي_Sigp->clone_وڻ_خالص(خطأ)
                          : خالي_اشارو;
                إذا (tedgep->m_condp) {
                    contributionp = gateCheckedCountp(c.flp, contributionp, contributionOverflowp,
                                                      tedgep->m_condp->clone_وڻ_خالص(خطأ));
                }
                sigs.ميلاپ_Countp
                    = جوڙيو_Checked_Countp(c.flp, sigs.ميلاپ_Countp, contributionp,
                                       sigs.ميلاپ_ڳڻپ_Overflowp, contributionOverflowp);
            }

            إذا (fromVtxp->m_delay_Ring_سائيز && !fromVtxp->m_isFixedDelayRing) {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_وڻ_خالص(خطأ));
                // reject |= (wrapped || idx == size - 1) && head && final_condition;
                Ast_ڳنڍ_اظهار* expireContribp = delay_Ring_Output_ڳڻپ(c.flp, fromVtxp);
                expireContribp = andCond(c.flp, expireContribp, tedgep->m_condp);
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, expireContribp);
                VL_DO_DANGLING(srcSigp->delete_وڻ(), srcSigp);
            } وإلا إذا (fromVtxp->m_isUnbounded || fromVtxp->m_isAndCombiner) {
                sigs.terminalActivep = orExprs(c.flp, sigs.terminalActivep, srcSigp);
            } وإلا {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_وڻ_خالص(خطأ));
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, srcSigp);
            }
        }
        // wireMatchAndMidSources always adds a Link from result.termVertexp
        // to m_matchVertexp, so the loop above always sets terminalActivep.
        UASSERT_OBJ(sigs.terminalActivep, c.گراف.m_ميلاپ_Vertexp,
                    "No terminal edge to match vertex");
    }

    Ast_ڳنڍ_اظهار* newThroughoutThreadFailCountp(هيٺاهون_Ctx& c, AstVar* ثابت delay_Ring_جيئرو_ڳڻپ_Varp,
                                               Ast_ڳنڍ_اظهار* ثابت stateExprp,
                                               Ast_ڳنڍ_اظهار* ثابت enablep) {
        Ast_ڳنڍ_اظهار* ثابت سرگرم_Thread_Countp
            = delay_Ring_جيئرو_ڳڻپ_Varp
                  ? static_cast<Ast_ڳنڍ_اظهار*>(
                        جديد AstVarRef{c.flp, delay_Ring_جيئرو_ڳڻپ_Varp, VAccess::READ})
                  : جديد AstExtend{c.flp, stateExprp->clone_وڻ_خالص(خطأ), m_u32DTypep->ويڪر()};
        أرجع جوڙيو_Countp(c.flp, خالي_اشارو, سرگرم_Thread_Countp, enablep);
    }

    // Phase 3b: Throughout-drop and ring-wide abort rejection.
    فراغ computeThroughoutReject(هيٺاهون_Ctx& c, Signal_سيٽ& sigs, ثابت منطقي need_Thread_Fail_ڳڻپ) {
        لكل (ثابت V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
            ثابت Sva_State_راس* ثابت vtxp = static_cast<ثابت Sva_State_راس*>(&vtxr);
            ثابت خودڪار& conds = vtxp->m_throughoutConds;
            إذا (conds.خالي_جايون() && !vtxp->m_abortRejectp) استمر;
            إذا (vtxp->m_isAndCombiner) استمر;
            Ast_ڳنڍ_اظهار* stateExprp = خالي_اشارو;
            إذا (vtxp->datap()->stateVarp) {
                stateExprp = جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
            } وإلا إذا (vtxp->datap()->delayRingVarp && vtxp->m_isFixedDelayRing) {
                stateExprp
                    = delay_Ring_Has_جيئرو_Threadsp(c.flp, vtxp->datap()->delay_Ring_جيئرو_ڳڻپ_Varp);
            } وإلا {
                UASSERT_OBJ(vtxp->datap()->stateSigp, vtxp,
                            "Throughout-conds vertex missing state representation");
                stateExprp = vtxp->datap()->stateSigp->clone_وڻ_خالص(خطأ);
            }
            Ast_ڳنڍ_اظهار* guardp = خالي_اشارو;
            لكل (Ast_ڳنڍ_اظهار* ثابت cp : conds) {
                Ast_ڳنڍ_اظهار* ثابت sp = sampled(cp->clone_وڻ_خالص(خطأ));
                guardp = guardp ? static_cast<Ast_ڳنڍ_اظهار*>(جديد AstLogAnd{c.flp, guardp, sp}) : sp;
            }
            إذا (vtxp->m_abortRejectp) {
                Ast_ڳنڍ_اظهار* ثابت notAbortp
                    = جديد AstLogNot{c.flp, sampled(vtxp->m_abortRejectp->clone_وڻ_خالص(خطأ))};
                guardp = guardp
                             ? static_cast<Ast_ڳنڍ_اظهار*>(جديد AstLogAnd{c.flp, guardp, notAbortp})
                             : notAbortp;
            }
            Ast_ڳنڍ_اظهار* rejectCondp = جديد AstLogNot{c.flp, guardp};
            إذا (vtxp->m_abortClearp) {
                // Any abort clears the ring. Accept aborts suppress a simultaneous guard failure;
                // reject aborts are restored below so they still force rejection.
                Ast_ڳنڍ_اظهار* ثابت notAbortClearp
                    = جديد AstLogNot{c.flp, sampled(vtxp->m_abortClearp->clone_وڻ_خالص(خطأ))};
                rejectCondp = جديد AstLogAnd{c.flp, rejectCondp, notAbortClearp};
                إذا (vtxp->m_abortRejectp) {
                    rejectCondp = جديد AstLogOr{
                        c.flp, rejectCondp, sampled(vtxp->m_abortRejectp->clone_وڻ_خالص(خطأ))};
                }
            }
            إذا (need_Thread_Fail_ڳڻپ) {
                Ast_ڳنڍ_اظهار* ثابت contributionp
                    = newThroughoutThreadFailCountp(c, vtxp->datap()->delay_Ring_جيئرو_ڳڻپ_Varp,
                                                    stateExprp, rejectCondp->clone_وڻ_خالص(خطأ));
                sigs.threadFailCountp = جوڙيو_Countp(c.flp, sigs.threadFailCountp, contributionp);
                إذا (vtxp->m_abortRejectp && vtxp->m_delayRingAdvanceCondp) {
                    // An advancing ring thread also occupies its same-tick match vertex in the
                    // unrolled NFA, so abort rejection must replay both fail actions.
                    Ast_ڳنڍ_اظهار* ثابت abortAndAdvancep = جديد AstLogAnd{
                        c.flp, sampled(vtxp->m_abortRejectp->clone_وڻ_خالص(خطأ)),
                        sampled(vtxp->m_delayRingAdvanceCondp->clone_وڻ_خالص(خطأ))};
                    Ast_ڳنڍ_اظهار* ثابت ميلاپ_Contributionp = newThroughoutThreadFailCountp(
                        c, vtxp->datap()->delay_Ring_جيئرو_ڳڻپ_Varp, stateExprp, abortAndAdvancep);
                    sigs.threadFailCountp
                        = جوڙيو_Countp(c.flp, sigs.threadFailCountp, ميلاپ_Contributionp);
                }
            }
            sigs.throughoutRejectp = orExprs(c.flp, sigs.throughoutRejectp,
                                             جديد AstLogAnd{c.flp, stateExprp, rejectCondp});
        }
    }

    Signal_سيٽ compute_سگنل(هيٺاهون_Ctx& c, ثابت منطقي need_Thread_Fail_ڳڻپ,
                             ثابت منطقي need_Throughout_Thread_Fail_ڳڻپ, ثابت منطقي need_ميلاپ_ڳڻپ) {
        Signal_سيٽ sigs;

        compute_Terminal_ميلاپ_And_Reject(c, sigs, need_ميلاپ_ڳڻپ);

        // Phase 3a: required-step rejection.
        // Builder only sets m_rejectOnFail on non-clocked Links with m_condp
        // or m_condVtxp, and the source always has a resolved stateSig.
        لكل (ثابت Sva_Trans_ڪنارو* ثابت tedgep : c.گراف.all_ڪنارا()) {
            إذا (!tedgep->m_rejectOnFail) استمر;
            ثابت Sva_State_راس* ثابت fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp && (tedgep->m_condp || tedgep->m_condVtxp),
                        fromVtxp,
                        "rejectOnFail Link must have condp/condVtxp and source stateSig");
            Ast_ڳنڍ_اظهار* ثابت srcSigp = fromVtxp->datap()->stateSigp->clone_وڻ_خالص(خطأ);
            Ast_ڳنڍ_اظهار* condp = خالي_اشارو;
            إذا (tedgep->m_condVtxp) {
                UASSERT_OBJ(tedgep->m_condVtxp->datap()->stateSigp, tedgep->m_condVtxp,
                            "rejectOnFail condVtxp missing stateSig");
                condp = tedgep->m_condVtxp->datap()->stateSigp->clone_وڻ_خالص(خطأ);
                إذا (tedgep->m_condp) {
                    condp = جديد AstLogOr{c.flp, condp, tedgep->m_condp->clone_وڻ_خالص(خطأ)};
                }
            } وإلا {
                condp = tedgep->m_condp->clone_وڻ_خالص(خطأ);
            }
            Ast_ڳنڍ_اظهار* ثابت notCondp = جديد AstLogNot{c.flp, condp};
            Ast_ڳنڍ_اظهار* ثابت rawFailp = جديد AstLogAnd{c.flp, srcSigp, notCondp};
            إذا (need_Thread_Fail_ڳڻپ) {
                // thread_fail_count += fail;
                sigs.threadFailCountp = جوڙيو_Countp(
                    c.flp, sigs.threadFailCountp,
                    جديد AstExtend{c.flp, rawFailp->clone_وڻ_خالص(خطأ), m_u32DTypep->ويڪر()});
            }
            Ast_ڳنڍ_اظهار* ثابت failp = gateNotKill(c, rawFailp);
            sigs.required_قدم_Rejectp = orExprs(c.flp, sigs.required_قدم_Rejectp, failp);
        }

        computeThroughoutReject(c, sigs, need_Throughout_Thread_Fail_ڳڻپ);
        إذا (sigs.threadFailCountp) {
            sigs.threadFailCountp
                = جوڙيو_Countp(c.flp, خالي_اشارو, sigs.threadFailCountp, not_Kill_سرگرم(c));
        }
        إذا (sigs.ميلاپ_Countp) {
            إذا (c.ميلاپ_Condp) {
                sigs.ميلاپ_Countp
                    = gateCheckedCountp(c.flp, sigs.ميلاپ_Countp, sigs.ميلاپ_ڳڻپ_Overflowp,
                                        sampled(c.ميلاپ_Condp->clone_وڻ_خالص(خطأ)));
            }
            sigs.ميلاپ_Countp = gateCheckedCountp(c.flp, sigs.ميلاپ_Countp, sigs.ميلاپ_ڳڻپ_Overflowp,
                                                 not_Kill_سرگرم(c));
        }
        sigs.terminalActivep = gateNotKill(c, sigs.terminalActivep);
        sigs.rejectBasep = gateNotKill(c, sigs.rejectBasep);
        sigs.throughoutRejectp = gateNotKill(c, sigs.throughoutRejectp);

        // Clean up intermediate state signals. These are orphan subtrees
        // (never linked into the enclosing AST); deleteTree() is immediate
        // which is what we want since stateSigp lifetime ends with this scope.
        لكل (V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
            Sva_State_راس* ثابت vtxp = static_cast<Sva_State_راس*>(&vtxr);
            Ast_ڳنڍ_اظهار*& sigp = vtxp->datap()->stateSigp;
            إذا (sigp) VL_DO_DANGLING(sigp->delete_وڻ(), sigp);
            Ast_ڳنڍ_اظهار*& countp = vtxp->datap()->state_ڳڻپ_Sigp;
            إذا (countp) VL_DO_DANGLING(countp->delete_وڻ(), countp);
            Ast_ڳنڍ_اظهار*& overflowp = vtxp->datap()->state_ڳڻپ_حد_کان_مٿي_Sigp;
            إذا (overflowp) VL_DO_DANGLING(overflowp->delete_وڻ(), overflowp);
        }
        // Disable iff gating (IEEE 1800-2023 16.12) on the current level value.
        إذا (c.disableExprp) {
            // terminalActivep is always set, so gate it unconditionally.
            Ast_ڳنڍ_اظهار* ثابت notTermp
                = جديد AstLogNot{c.flp, c.disableExprp->clone_وڻ_خالص(خطأ)};
            sigs.terminalActivep = جديد AstLogAnd{c.flp, sigs.terminalActivep, notTermp};
            إذا (sigs.rejectBasep) {
                Ast_ڳنڍ_اظهار* ثابت notDisp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_وڻ_خالص(خطأ)};
                sigs.rejectBasep = جديد AstLogAnd{c.flp, sigs.rejectBasep, notDisp};
            }
            إذا (sigs.throughoutRejectp) {
                Ast_ڳنڍ_اظهار* ثابت notDisp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_وڻ_خالص(خطأ)};
                sigs.throughoutRejectp = جديد AstLogAnd{c.flp, sigs.throughoutRejectp, notDisp};
            }
            إذا (sigs.required_قدم_Rejectp) {
                Ast_ڳنڍ_اظهار* ثابت notDisp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_وڻ_خالص(خطأ)};
                sigs.required_قدم_Rejectp = جديد AstLogAnd{c.flp, sigs.required_قدم_Rejectp, notDisp};
            }
            إذا (sigs.ميلاپ_Countp) {
                sigs.ميلاپ_Countp = gateCheckedCountp(
                    c.flp, sigs.ميلاپ_Countp, sigs.ميلاپ_ڳڻپ_Overflowp,
                    جديد AstLogNot{c.flp, c.disableExprp->clone_وڻ_خالص(خطأ)});
            }
        }

        إذا (sigs.ميلاپ_ڳڻپ_Overflowp) {
            m_modp->جوڙيو_Stmtsp(
                جديد AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_وڻ(خطأ),
                              new_ڳڻپ_حد_کان_مٿي_Warning(c.flp, sigs.ميلاپ_ڳڻپ_Overflowp)});
            sigs.ميلاپ_ڳڻپ_Overflowp = خالي_اشارو;
        }

        أرجع sigs;
    }

    // Phase 1: Resolve combinational Links via fixed-point propagation.
    فراغ resolveLinks(هيٺاهون_Ctx& c, Ast_ڳنڍ_اظهار* triggerExprp, ثابت منطقي ڳڻپ_Matches) {
        // datap() was freshly allocated in lower() -- all state signals start null.
        c.گراف.m_شروعات_Vertexp->datap()->stateSigp = triggerExprp->clone_وڻ_خالص(خطأ);
        c.گراف.m_شروعات_Vertexp->datap()->state_ڳڻپ_Sigp
            = جديد AstExtend{c.flp, triggerExprp->clone_وڻ_خالص(خطأ), m_u32DTypep->ويڪر()};
        لكل (V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
            Sva_State_راس* ثابت vtxp = static_cast<Sva_State_راس*>(&vtxr);
            إذا (vtxp->datap()->stateVarp) {
                vtxp->datap()->stateSigp
                    = جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
                vtxp->datap()->state_ڳڻپ_Sigp = جديد AstExtend{
                    c.flp, جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ},
                    m_u32DTypep->ويڪر()};
            } وإلا إذا (vtxp->datap()->delayRingVarp) {
                إذا (vtxp->m_isFixedDelayRing) {
                    // state_count = wrapped ? head : 0; state = state_count != 0;
                    Ast_ڳنڍ_اظهار* ثابت countp = delay_Ring_Output_ڳڻپ(c.flp, vtxp);
                    vtxp->datap()->stateSigp = جديد AstNeq{c.flp, countp->clone_وڻ_خالص(خطأ),
                                                          newTypedConstp(c.flp, m_u32DTypep, 0)};
                    vtxp->datap()->state_ڳڻپ_Sigp = countp;
                } وإلا {
                    vtxp->datap()->stateSigp
                        = delay_Ring_Has_جيئرو_Threadsp(c.flp, vtxp->datap()->delay_Ring_جيئرو_ڳڻپ_Varp);
                    vtxp->datap()->state_ڳڻپ_Sigp = جديد AstVarRef{
                        c.flp, vtxp->datap()->delay_Ring_جيئرو_ڳڻپ_Varp, VAccess::READ};
                }
            }
        }
        // Fixed-point propagation along zero-delay (Link) edges. Rebuild each
        // derived signal from its incoming edges on every pass; appending the same
        // contributions repeatedly makes merge expressions grow exponentially.
        ثابت سائيز_t passes = 2 * c.گراف.m_گراف.vertices().سائيز() + 2;
        لكل (سائيز_t pass = 0; pass < passes; ++pass) {
            // Rebuild SAnd combiners once both sub-NFA terminals are available.
            لكل (V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
                Sva_State_راس* ثابت vtxp = static_cast<Sva_State_راس*>(&vtxr);
                إذا (!vtxp->m_isAndCombiner) استمر;
                // AndCombiner vertices always have both terminal pointers set.
                UASSERT_OBJ(vtxp->m_andLhsTermp && vtxp->m_andRhsTermp, vtxp,
                            "AndCombiner vertex missing LHS/RHS terminal");
                ثابت Sva_State_راس* ثابت lhsTermp = vtxp->m_andLhsTermp;
                ثابت Sva_State_راس* ثابت rhsTermp = vtxp->m_andRhsTermp;
                إذا (!lhsTermp->datap()->stateSigp || !rhsTermp->datap()->stateSigp) استمر;
                Ast_ڳنڍ_اظهار* ثابت ميلاپ_Lp
                    = build_ميلاپ_Now(c.flp, lhsTermp->datap()->stateSigp, vtxp->m_andLhsCondp);
                Ast_ڳنڍ_اظهار* ثابت ميلاپ_Rp
                    = build_ميلاپ_Now(c.flp, rhsTermp->datap()->stateSigp, vtxp->m_andRhsCondp);
                Ast_ڳنڍ_اظهار* ثابت پورو_LOrp = جديد AstLogOr{
                    c.flp, جديد AstVarRef{c.flp, vtxp->datap()->پورو_LVarp, VAccess::READ}, ميلاپ_Lp};
                Ast_ڳنڍ_اظهار* ثابت پورو_ROrp = جديد AstLogOr{
                    c.flp, جديد AstVarRef{c.flp, vtxp->datap()->پورو_RVarp, VAccess::READ}, ميلاپ_Rp};
                Ast_ڳنڍ_اظهار* ثابت bothp = جديد AstLogAnd{c.flp, پورو_LOrp, پورو_ROrp};
                Ast_ڳنڍ_اظهار* ثابت oneNowp = جديد AstLogOr{c.flp, ميلاپ_Lp->clone_وڻ_خالص(خطأ),
                                                          ميلاپ_Rp->clone_وڻ_خالص(خطأ)};
                إذا (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_وڻ(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = جديد AstLogAnd{c.flp, bothp, oneNowp};
                إذا (vtxp->datap()->state_ڳڻپ_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_ڳڻپ_Sigp->delete_وڻ(),
                                   vtxp->datap()->state_ڳڻپ_Sigp);
                }
                vtxp->datap()->state_ڳڻپ_Sigp = جديد AstExtend{
                    c.flp, vtxp->datap()->stateSigp->clone_وڻ_خالص(خطأ), m_u32DTypep->ويڪر()};
            }

            لكل (V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
                Sva_State_راس* ثابت vtxp = static_cast<Sva_State_راس*>(&vtxr);
                إذا (vtxp == c.گراف.m_شروعات_Vertexp || vtxp->datap()->stateVarp
                    || vtxp->datap()->delayRingVarp || vtxp->m_isAndCombiner || vtxp->m_is_ميلاپ
                    || vtxp->m_isRejectSink) {
                    استمر;
                }
                Ast_ڳنڍ_اظهار* ايندڙ_Statep = خالي_اشارو;
                Ast_ڳنڍ_اظهار* ايندڙ_State_Countp = خالي_اشارو;
                Ast_ڳنڍ_اظهار* ايندڙ_State_ڳڻپ_Overflowp = خالي_اشارو;
                لكل (ثابت V3_گراف_ڪنارو& er : vtxp->اندريون_ڪنارا()) {
                    ثابت Sva_Trans_ڪنارو& te = static_cast<ثابت Sva_Trans_ڪنارو&>(er);
                    ثابت Sva_State_راس* ثابت fromVtxp = te.fromVtxp();
                    إذا (!fromVtxp->datap()->stateSigp) استمر;
                    UASSERT_OBJ(fromVtxp->datap()->state_ڳڻپ_Sigp, fromVtxp,
                                "Link source missing stateCountSig");
                    Ast_ڳنڍ_اظهار* ثابت contributionp = andCond(
                        c.flp, fromVtxp->datap()->stateSigp->clone_وڻ_خالص(خطأ), te.m_condp);
                    ايندڙ_Statep = orExprs(c.flp, ايندڙ_Statep, contributionp);
                    Ast_ڳنڍ_اظهار* ڳڻپ_Contributionp
                        = fromVtxp->datap()->state_ڳڻپ_Sigp->clone_وڻ_خالص(خطأ);
                    Ast_ڳنڍ_اظهار* ڳڻپ_Contribution_Overflowp
                        = ڳڻپ_Matches && fromVtxp->datap()->state_ڳڻپ_حد_کان_مٿي_Sigp
                              ? fromVtxp->datap()->state_ڳڻپ_حد_کان_مٿي_Sigp->clone_وڻ_خالص(خطأ)
                              : خالي_اشارو;
                    إذا (te.m_condp) {
                        Ast_ڳنڍ_اظهار* ثابت enablep = te.m_condp->clone_وڻ_خالص(خطأ);
                        ڳڻپ_Contributionp
                            = ڳڻپ_Matches
                                  ? gateCheckedCountp(c.flp, ڳڻپ_Contributionp,
                                                      ڳڻپ_Contribution_Overflowp, enablep)
                                  : جوڙيو_Countp(c.flp, خالي_اشارو, ڳڻپ_Contributionp, enablep);
                    }
                    ايندڙ_State_Countp
                        = ڳڻپ_Matches
                              ? جوڙيو_Checked_Countp(c.flp, ايندڙ_State_Countp, ڳڻپ_Contributionp,
                                                 ايندڙ_State_ڳڻپ_Overflowp,
                                                 ڳڻپ_Contribution_Overflowp)
                              : جوڙيو_Countp(c.flp, ايندڙ_State_Countp, ڳڻپ_Contributionp);
                }
                إذا (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_وڻ(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = ايندڙ_Statep;
                إذا (vtxp->datap()->state_ڳڻپ_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_ڳڻپ_Sigp->delete_وڻ(),
                                   vtxp->datap()->state_ڳڻپ_Sigp);
                }
                vtxp->datap()->state_ڳڻپ_Sigp = ايندڙ_State_Countp;
                إذا (vtxp->datap()->state_ڳڻپ_حد_کان_مٿي_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_ڳڻپ_حد_کان_مٿي_Sigp->delete_وڻ(),
                                   vtxp->datap()->state_ڳڻپ_حد_کان_مٿي_Sigp);
                }
                vtxp->datap()->state_ڳڻپ_حد_کان_مٿي_Sigp = ايندڙ_State_ڳڻپ_Overflowp;
            }
        }
    }

عام:
    // Combine terminal/reject signals into final output expression.
    ساكن Ast_ڳنڍ_اظهار* assemble_نتيجو(Ast_ڳنڍ_Cover_Or_Assert* ثابت assertp, ثابت منطقي negated,
                                       Ast_ڳنڍ_اظهار* ثابت ميلاپ_Condp, ثابت Signal_سيٽ& sigs,
                                       Ast_ڳنڍ_اظهار** ثابت ٻاهريون_Matchpp) {
        File_سٽ* ثابت flp = assertp->fileline();
        ثابت منطقي isCover = VN_IS(assertp, Cover);
        Ast_ڳنڍ_اظهار* ثابت terminalActivep = sigs.terminalActivep;
        Ast_ڳنڍ_اظهار* ثابت rejectBasep = sigs.rejectBasep;
        Ast_ڳنڍ_اظهار* ثابت throughoutRejectp = sigs.throughoutRejectp;
        Ast_ڳنڍ_اظهار* ثابت required_قدم_Rejectp = sigs.required_قدم_Rejectp;
        // Property negation (IEEE 1800-2023 16.12.1 `not`): invert match/reject.
        إذا (negated) {
            إذا (isCover) {
                إذا (terminalActivep)
                    VL_DO_DANGLING(terminalActivep->delete_وڻ(), terminalActivep);
                Ast_ڳنڍ_اظهار* negRejectp = خالي_اشارو;
                إذا (ميلاپ_Condp && rejectBasep) {
                    Ast_ڳنڍ_اظهار* ثابت sampledCondp = sampled(ميلاپ_Condp->clone_وڻ_خالص(خطأ));
                    Ast_ڳنڍ_اظهار* ثابت notCondp = جديد AstLogNot{flp, sampledCondp};
                    negRejectp = جديد AstLogAnd{flp, rejectBasep, notCondp};
                } وإلا إذا (rejectBasep) {
                    VL_DO_DANGLING(rejectBasep->delete_وڻ(), rejectBasep);
                }
                إذا (throughoutRejectp) negRejectp = orExprs(flp, negRejectp, throughoutRejectp);
                إذا (required_قدم_Rejectp)
                    negRejectp = orExprs(flp, negRejectp, required_قدم_Rejectp);
                أرجع negRejectp ? negRejectp : جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}};
            }
            // Negated assert/assume: output = !match.
            Ast_ڳنڍ_اظهار* matchp = terminalActivep;
            إذا (ميلاپ_Condp) {
                Ast_ڳنڍ_اظهار* ثابت sampledCondp = sampled(ميلاپ_Condp->clone_وڻ_خالص(خطأ));
                matchp = جديد AstLogAnd{flp, matchp, sampledCondp};
            }
            إذا (ٻاهريون_Matchpp) {
                Ast_ڳنڍ_اظهار* notPMatchp = خالي_اشارو;
                إذا (ميلاپ_Condp && rejectBasep) {
                    Ast_ڳنڍ_اظهار* ثابت sampledCondp = sampled(ميلاپ_Condp->clone_وڻ_خالص(خطأ));
                    notPMatchp = جديد AstLogAnd{flp, rejectBasep->clone_وڻ_خالص(خطأ),
                                               جديد AstLogNot{flp, sampledCondp}};
                } وإلا إذا (rejectBasep) {
                    notPMatchp = rejectBasep->clone_وڻ_خالص(خطأ);
                }
                إذا (throughoutRejectp)
                    notPMatchp = orExprs(flp, notPMatchp, throughoutRejectp->clone_وڻ_خالص(خطأ));
                إذا (required_قدم_Rejectp)
                    notPMatchp
                        = orExprs(flp, notPMatchp, required_قدم_Rejectp->clone_وڻ_خالص(خطأ));
                *ٻاهريون_Matchpp = notPMatchp;
            }
            إذا (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_وڻ(), throughoutRejectp);
            إذا (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_وڻ(), rejectBasep);
            إذا (required_قدم_Rejectp)
                VL_DO_DANGLING(required_قدم_Rejectp->delete_وڻ(), required_قدم_Rejectp);
            Ast_ڳنڍ_اظهار* ثابت نتيجو_Exprp = جديد AstLogNot{flp, matchp};
            أرجع نتيجو_Exprp;
        }
        إذا (isCover) {
            إذا (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_وڻ(), throughoutRejectp);
            إذا (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_وڻ(), rejectBasep);
            إذا (required_قدم_Rejectp)
                VL_DO_DANGLING(required_قدم_Rejectp->delete_وڻ(), required_قدم_Rejectp);
            إذا (ميلاپ_Condp) {
                Ast_ڳنڍ_اظهار* ثابت sampledCondp = sampled(ميلاپ_Condp->clone_وڻ_خالص(خطأ));
                أرجع جديد AstLogAnd{flp, terminalActivep, sampledCondp};
            }
            أرجع terminalActivep;
        }
        // Assert/assume: output = !reject
        Ast_ڳنڍ_اظهار* rejectp = خالي_اشارو;
        إذا (ميلاپ_Condp && rejectBasep) {
            Ast_ڳنڍ_اظهار* ثابت sampledCondp = sampled(ميلاپ_Condp->clone_وڻ_خالص(خطأ));
            rejectp = جديد AstLogAnd{flp, rejectBasep, جديد AstLogNot{flp, sampledCondp}};
        } وإلا إذا (rejectBasep) {
            VL_DO_DANGLING(rejectBasep->delete_وڻ(), rejectBasep);
        }
        إذا (ٻاهريون_Matchpp) {
            Ast_ڳنڍ_اظهار* ميلاپ_Exprp = terminalActivep->clone_وڻ_خالص(خطأ);
            إذا (ميلاپ_Condp) {
                Ast_ڳنڍ_اظهار* ثابت sp = sampled(ميلاپ_Condp->clone_وڻ_خالص(خطأ));
                ميلاپ_Exprp = جديد AstLogAnd{flp, ميلاپ_Exprp, sp};
            }
            *ٻاهريون_Matchpp = ميلاپ_Exprp;
        }
        إذا (terminalActivep) VL_DO_DANGLING(terminalActivep->delete_وڻ(), terminalActivep);
        إذا (throughoutRejectp) rejectp = orExprs(flp, rejectp, throughoutRejectp);
        إذا (required_قدم_Rejectp) rejectp = orExprs(flp, rejectp, required_قدم_Rejectp);
        إذا (!rejectp) أرجع جديد AstConst{flp, AstConst::ٻٽو_انگ_True{}};
        Ast_ڳنڍ_اظهار* ثابت نتيجو_Exprp = جديد AstLogNot{flp, rejectp};
        أرجع نتيجو_Exprp;
    }

    واضح SvaNfaLowering(Ast_ڳنڍ_Module* modp)
        : m_modp{modp}
        , m_u32DTypep{modp->ڳوليو_Basic_DType(VBasicDTypeKwd::UINT32)} {}
    ~SvaNfaLowering() {
        V3Stats::جوڙيو_Stat_جوڙ("Assertions, NFA delay ring edge visits", m_stat_Delay_Ring_ڪنارو_Visits);
    }

    // Lower NFA graph to synthesizable AstAlways blocks and raw result signals.
    // Links are combinational; Edges are registered (NBA).
    Signal_سيٽ هيٺاهون(Ast_ڳنڍ_Cover_Or_Assert* ثابت assertp, Sva_گراف& گراف,
                    Ast_Sen_وڻ* ثابت senTreep, Ast_ڳنڍ_اظهار* ثابت ميلاپ_Condp,
                    Ast_ڳنڍ_اظهار* ثابت disableExprp, ثابت منطقي need_Thread_Fail_ڳڻپ,
                    ثابت منطقي need_Throughout_Thread_Fail_ڳڻپ) {
        File_سٽ* ثابت flp = assertp->fileline();
        AstCover* ثابت coverp = VN_CAST(assertp, Cover);
        ثابت منطقي isSeqEvent = coverp && coverp->isSeqEvent();
        ثابت VAssertType assertType
            = isSeqEvent ? VAssertType{VAssertType::اندروني} : assertp->userType();
        ثابت VAssertDirectiveType directiveType
            = isSeqEvent ? VAssertDirectiveType{VAssertDirectiveType::اندروني}
                         : assertp->directive();
        ثابت std::string بنياد_Name = m_نالا.get("");

        // Number vertices with sequential colors for array indexing.
        صحيح N = 0;
        لكل (V3_گراف_راس& vtxr : گراف.m_گراف.vertices()) { vtxr.color(N++); }
        std::ويڪٽر<Sva_State_راس*> vtx(N, خالي_اشارو);
        لكل (V3_گراف_راس& vtxr : گراف.m_گراف.vertices()) {
            vtx[vtxr.color()] = static_cast<Sva_State_راس*>(&vtxr);
        }

        // Allocate per-vertex lowering data (stored via V3GraphVertex::userp()).
        std::ويڪٽر<Sva_راس_معلومات> راس_معلومات(N);
        لكل (صحيح i = 0; i < N; ++i) vtx[i]->userp(&راس_معلومات[i]);

        // Identify registered vertices (targets of clocked edges).
        لكل (صحيح i = 0; i < N; ++i) {
            لكل (ثابت V3_گراف_ڪنارو& edger : vtx[i]->ٻاهريون_ڪنارا()) {
                ثابت Sva_Trans_ڪنارو& tedger = static_cast<ثابت Sva_Trans_ڪنارو&>(edger);
                ثابت صحيح منزل_Idx = tedger.منزل_Vtxp()->color();
                إذا (tedger.m_consumes_چڪر && tedger.منزل_Vtxp() != گراف.m_ميلاپ_Vertexp
                    && !tedger.منزل_Vtxp()->m_isRejectSink) {
                    vtx[منزل_Idx]->datap()->needsReg = سچ;
                }
            }
        }

        // A packed range ring can emit several live endpoints without needing per-slot counts.
        // Widen only a ring whose input can already carry multiplicity.
        std::ويڪٽر<منطقي> stateMayHaveMultiplicity(N);
        std::ويڪٽر<منطقي> ringNeedsMultiplicity(N);
        std::ويڪٽر<صحيح> multiplicityWork;
        ثابت منطقي ڳڻپ_Matches = coverp && coverp->isCoverSeq() && !isSeqEvent;
        إذا (ڳڻپ_Matches) {
            لكل (صحيح i = 0; i < N; ++i) {
                ثابت منطقي گڻو_Inputs = vtx[i]->اندريون_ڪنارا().سائيز() > 1;
                ثابت منطقي isRing = vtx[i]->m_delay_Ring_سائيز;
                ringNeedsMultiplicity[i] = isRing && گڻو_Inputs;
                ثابت منطقي startsMultiplicity = isRing
                                                    ? !vtx[i]->m_isFixedDelayRing
                                                    : گڻو_Inputs && !vtx[i]->datap()->needsReg;
                إذا (startsMultiplicity) {
                    stateMayHaveMultiplicity[i] = سچ;
                    multiplicityWork.ڌڪو_پٺ(i);
                }
            }
            لكل (سائيز_t wi = 0; wi < multiplicityWork.سائيز(); ++wi) {
                لكل (ثابت V3_گراف_ڪنارو& edger : vtx[multiplicityWork[wi]]->ٻاهريون_ڪنارا()) {
                    ثابت Sva_Trans_ڪنارو& tedger = static_cast<ثابت Sva_Trans_ڪنارو&>(edger);
                    ثابت صحيح ti = tedger.منزل_Vtxp()->color();
                    ثابت منطقي isRing = vtx[ti]->m_delay_Ring_سائيز;
                    إذا (isRing) ringNeedsMultiplicity[ti] = سچ;
                    // Match and reject sinks have no outgoing edges, so they stop naturally.
                    ثابت منطقي stopsMultiplicity = !isRing && vtx[ti]->datap()->needsReg;
                    إذا (stateMayHaveMultiplicity[ti] || stopsMultiplicity) { استمر; }
                    stateMayHaveMultiplicity[ti] = سچ;
                    multiplicityWork.ڌڪو_پٺ(ti);
                }
            }
        }

        ثابت منطقي asyncDisable = isAsyncDisable(disableExprp);
        // Registers cleared on a rising disable are also written by their clocked block.
        File_سٽ* ثابت regFlp = asyncDisable ? جديد File_سٽ{flp} : flp;
        إذا (asyncDisable) regFlp->warnOff(V3_Error_ڪوڊ::MULTIDRIVEN, سچ);

        AstVar* ثابت killVarp
            = جديد AstVar{flp, VVarType::MODULETEMP, بنياد_Name + "__kill", m_u32DTypep};
        killVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->جوڙيو_Stmtsp(killVarp);
        لكل (صحيح i = 0; i < N; ++i) {
            إذا (vtx[i]->m_isAndCombiner) {
                ثابت std::string بنياد = بنياد_Name + "__a" + std::منزل_string(i);
                AstVar* ثابت lp = جديد AstVar{regFlp, VVarType::MODULETEMP, بنياد + "_doneL",
                                              m_modp->ڳوليو_ٻٽو_انگ_DType()};
                lp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڙيو_Stmtsp(lp);
                vtx[i]->datap()->پورو_LVarp = lp;
                AstVar* ثابت rp = جديد AstVar{regFlp, VVarType::MODULETEMP, بنياد + "_doneR",
                                              m_modp->ڳوليو_ٻٽو_انگ_DType()};
                rp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڙيو_Stmtsp(rp);
                vtx[i]->datap()->پورو_RVarp = rp;
                استمر;
            }
            إذا (vtx[i]->m_delay_Ring_سائيز) {
                ثابت std::string بنياد = بنياد_Name + "__d" + std::منزل_string(i);
                ثابت منطقي storesCounts = ڳڻپ_Matches && ringNeedsMultiplicity[i];
                Ast_ڳنڍ_DType* ringDTypep = خالي_اشارو;
                إذا (storesCounts) {
                    // int unsigned ring [size];
                    ringDTypep = جديد AstUnpackArrayDType{
                        flp, m_u32DTypep,
                        جديد AstRange{flp, static_cast<صحيح>(vtx[i]->m_delay_Ring_سائيز - 1), 0}};
                    v3Global.rootp()->typeTablep()->جوڙيو_Typesp(ringDTypep);
                } وإلا {
                    // bit [size-1:0] ring;
                    ringDTypep = m_modp->ڳوليو_ٻٽو_انگ_DType(vtx[i]->m_delay_Ring_سائيز,
                                                      vtx[i]->m_delay_Ring_سائيز, VSigning::UNSIGNED);
                }
                AstVar* ثابت ringp
                    = جديد AstVar{flp, VVarType::MODULETEMP, بنياد + "_ring", ringDTypep};
                ringp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڙيو_Stmtsp(ringp);
                vtx[i]->datap()->delayRingVarp = ringp;
                vtx[i]->datap()->delayRingCounts = storesCounts;
                AstVar* ثابت headp
                    = جديد AstVar{flp, VVarType::MODULETEMP, بنياد + "_head",
                                 storesCounts ? m_u32DTypep : m_modp->ڳوليو_ٻٽو_انگ_DType()};
                headp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڙيو_Stmtsp(headp);
                vtx[i]->datap()->delay_Ring_سر_Varp = headp;
                // int unsigned idx;
                AstVar* ثابت idxp
                    = جديد AstVar{regFlp, VVarType::MODULETEMP, بنياد + "_idx", m_u32DTypep};
                idxp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڙيو_Stmtsp(idxp);
                vtx[i]->datap()->delayRingIdxVarp = idxp;
                // int unsigned live_count;
                AstVar* ثابت جيئرو_ڳڻپ_Varp
                    = جديد AstVar{regFlp, VVarType::MODULETEMP, بنياد + "_liveCount", m_u32DTypep};
                جيئرو_ڳڻپ_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڙيو_Stmtsp(جيئرو_ڳڻپ_Varp);
                vtx[i]->datap()->delay_Ring_جيئرو_ڳڻپ_Varp = جيئرو_ڳڻپ_Varp;
                AstVar* ثابت wrappedp = جديد AstVar{regFlp, VVarType::MODULETEMP,
                                                    بنياد + "_wrapped", m_modp->ڳوليو_ٻٽو_انگ_DType()};
                wrappedp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڙيو_Stmtsp(wrappedp);
                vtx[i]->datap()->delayRingWrappedVarp = wrappedp;
                استمر;
            }
            إذا (!vtx[i]->datap()->needsReg) استمر;
            إذا (vtx[i] == گراف.m_شروعات_Vertexp || vtx[i]->m_is_ميلاپ) استمر;
            ثابت std::string varName = بنياد_Name + "__s" + std::منزل_string(i);
            AstVar* ثابت varp
                = جديد AstVar{regFlp, VVarType::MODULETEMP, varName, m_modp->ڳوليو_ٻٽو_انگ_DType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جوڙيو_Stmtsp(varp);
            vtx[i]->datap()->stateVarp = varp;
        }

        // Build lowering context for phase sub-functions.
        هيٺاهون_Ctx c{flp,        senTreep,      disableExprp, ميلاپ_Condp,
                   assertType, directiveType, killVarp,     گراف};

        // Phase 1: Resolve combinational Links via fixed-point propagation.
        Ast_ڳنڍ_اظهار* ثابت triggerExprp
            = isSeqEvent ? جديد AstConst{flp, AstConst::ٻٽو_انگ_True{}}
                         : assertOnCond(flp, assertp->userType(), assertp->directive());
        resolveLinks(c, triggerExprp, ڳڻپ_Matches);
        VL_DO_DANGLING(triggerExprp->delete_وڻ(), triggerExprp);

        // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
        emit_State_رجسٽر_Nba(c);
        emitDelayRingUpdates(c, ڳڻپ_Matches);
        emit_And_Combiner_پورو_Latch_Nba(c);
        emitKillAckNba(c);

        // Phase 3/3a/3b: Compute terminal match/reject signals (cleans up stateSig).
        ثابت Signal_سيٽ sigs
            = compute_سگنل(c, need_Thread_Fail_ڳڻپ, need_Throughout_Thread_Fail_ڳڻپ, ڳڻپ_Matches);

        // Strong s_always[m:n] end-of-simulation liveness: if any in-window state
        // is still set at $finish, the universal-quantifier window never completed
        // (IEEE 1800-2023 16.12.11 strong semantics). Fire the assertion failure
        // from a final block; V3Assert turns the DT_ERROR display into the standard
        // "Assertion failed in %m" message.
        // pending = |{strong state registers, strong delay live counts != 0};
        Ast_ڳنڍ_اظهار* pendingp = خالي_اشارو;
        لكل (صحيح i = 0; i < N; ++i) {
            إذا (!vtx[i]->m_strongPending) استمر;
            Ast_ڳنڍ_اظهار* pendingExprp = خالي_اشارو;
            إذا (vtx[i]->datap()->stateVarp) {
                pendingExprp = جديد AstVarRef{flp, vtx[i]->datap()->stateVarp, VAccess::READ};
            } وإلا إذا (vtx[i]->m_strongAlwaysRing) {
                pendingExprp
                    = delay_Ring_Has_جيئرو_Threadsp(flp, vtx[i]->datap()->delay_Ring_جيئرو_ڳڻپ_Varp);
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
            Ast_ڳنڍ_اظهار* ثابت condp = جديد AstLogAnd{flp, assertOnp, pendingp};
            AstDisplay* ثابت dispp
                = جديد AstDisplay{flp, VDisplayType::DT_ERROR, "", خالي_اشارو, خالي_اشارو};
            dispp->fmtp()->timeunit(m_modp->timeunit());
            Ast_ڳنڍ_Stmt* ثابت firep = dispp;
            إذا (v3Global.opt.stopFail()) firep->جوڙيو_ايندڙ(جديد AstStop{flp, خطأ});
            m_modp->جوڙيو_Stmtsp(جديد AstFinal{flp, جديد AstIf{flp, condp, firep}});
        }

        // Clear userp on every vertex before vertexData is destroyed.
        لكل (صحيح i = 0; i < N; ++i) vtx[i]->userp(خالي_اشارو);
        أرجع sigs;
    }
};

}  // namespace

//######################################################################
// Top-level visitor

صنف AssertNfaVisitor final : عام VNVisitor {
    // STATE
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Current module being processed
    AstClocking* m_defaultClockingp = خالي_اشارو;  // Default clocking
    AstDefaultDisable* m_defaultDisablep = خالي_اشارو;  // Default disable iff
    SvaNfaLowering* m_loweringp = خالي_اشارو;  // NFA-to-hardware lowering engine
    Ast_Sen_وڻ* m_sampled_قدر_Clockp = خالي_اشارو;  // Inherited clock during scoped attachment
    V3_Unique_نالا m_prop_Var_نالا{"__Vpropvar"};  // Property-local variable names
    V3_Unique_نالا m_prop_عارضي_نالا{"__VnfaSampled"};  // Hoisted $sampled(propp) temps
    V3_Unique_نالا m_fail_ڳڻپ_نالا{"__VnfaRemainingFailCount"};  // Fail replay counter names
    std::سيٽ<ثابت AstProperty*> m_inliningProps;  // Recursion guard for inlineNamedProperty

    سانچو <قسم_نالو T_ڳنڍ>
    فراغ گهمو_Sampled_قدر(T_ڳنڍ* ثابت nodep) {
        إذا (m_sampled_قدر_Clockp && !nodep->sentreep()) {
            nodep->sentreep(m_sampled_قدر_Clockp->clone_وڻ(سچ));
        }
        iterateChildren(nodep);
    }

    // Wire match vertex and mid-window sources for a successful NFA build.
    ساكن فراغ wire_ميلاپ_And_Mid_Sources(Sva_گراف& گراف, ثابت Build_نتيجو& نتيجو, File_سٽ* flp) {
        گراف.create_ميلاپ_راس();
        // Skip the main term Link when midSources already cover every
        // end-of-match (cover_sequence path); otherwise the per-mid extraction
        // double-counts via the merge vertex.
        إذا (!نتيجو.رڪن_Is_Mid_ملايو) { گراف.جوڙيو_Link(نتيجو.رڪن_Vertexp, گراف.m_ميلاپ_Vertexp); }
        لكل (Sva_State_راس* srcVtxp : نتيجو.midSources) {
            Ast_ڳنڍ_اظهار* condp = خالي_اشارو;
            لكل (Ast_ڳنڍ_اظهار* ثابت tc : srcVtxp->m_throughoutConds) {
                Ast_ڳنڍ_اظهار* ثابت tcClone = tc->clone_وڻ_خالص(خطأ);
                condp = condp ? جديد AstLogAnd{flp, condp, tcClone} : tcClone;
            }
            گراف.جوڙيو_Link(srcVtxp, گراف.m_ميلاپ_Vertexp, condp);
            srcVtxp->m_isUnbounded = سچ;
        }
    }

    ساكن Ast_ڳنڍ_اظهار* getSequenceBodyExprp(ثابت AstSequence* seqp) {
        Ast_ڳنڍ* bodyp = seqp->stmtsp();
        طالما (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        أرجع VN_CAST(bodyp, ڳنڍ_اظهار);
    }

    ساكن AstPropSpec* getPropertySpecp(ثابت AstProperty* propp) {
        Ast_ڳنڍ* stmtp = propp->stmtsp();
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
        إذا (!m_inliningProps.emplace(propyp).ٻيو) {
            funcrefp->v3error("Illegal recursive property reference");  // LCOV_EXCL_LINE
            أرجع;  // LCOV_EXCL_LINE
        }
        بنية Guard final {
            std::سيٽ<ثابت AstProperty*>& setr;
            ثابت AstProperty* keyp;
            ~Guard() { setr.erase(keyp); }
        } guard{m_inliningProps, propyp};
        AstPropSpec* propSpecp = getPropertySpecp(propyp);
        UASSERT_OBJ(propSpecp, funcrefp, "Property has no body PropSpec");
        propSpecp = propSpecp->clone_وڻ(خطأ);

        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propyp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_ڳنڍ_اظهار*> پورٽ_Map;
        لكل (ثابت خودڪار& tconnect : tconnects) {
            پورٽ_Map[tconnect.پهريون] = tconnect.ٻيو->exprp();
        }

        // Promote property-local variables to module-level temps (IEEE 16.10).
        std::unordered_map<ثابت AstVar*, AstVar*> localVarMap;
        لكل (Ast_ڳنڍ* stmtp = propyp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (!varp->isIO()) {
                    ثابت string newName = m_prop_Var_نالا.get(varp);
                    AstVar* ثابت newVarp = جديد AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->جوڙيو_Stmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        propSpecp->foreach([&](AstVarRef* refp) {
            ثابت خودڪار پورٽ_It = پورٽ_Map.ڳوليو(refp->varp());
            إذا (پورٽ_It != پورٽ_Map.پڇاڙي()) {
                refp->replaceWith(پورٽ_It->ٻيو->clone_وڻ(خطأ));
                VL_DO_DANGLING(ڌڪو_Deletep(refp), refp);
                أرجع;
            }
            ثابت خودڪار localIt = localVarMap.ڳوليو(refp->varp());
            إذا (localIt != localVarMap.پڇاڙي()) refp->varp(localIt->ٻيو);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        لكل (ثابت خودڪار& tconnect : tconnects) {
            ڌڪو_Deletep(tconnect.ٻيو->exprp()->unlink_Fr_پٺ());
        }
        V3AssertCommon::checkSvaInstanceDelays(propSpecp, funcrefp);

        // Merge disable iff (IEEE 1800-2023 16.12.1)
        إذا (outerSpecp->disablep() && propSpecp->disablep()) {
            outerSpecp->v3error("disable iff expression before property call "
                                "and in its body is not legal");
            ڌڪو_Deletep(propSpecp->disablep()->unlink_Fr_پٺ());
        }
        إذا (outerSpecp->disablep()) {
            propSpecp->disablep(outerSpecp->disablep()->unlink_Fr_پٺ());
        }

        إذا (outerSpecp->sensesp() && propSpecp->sensesp()) {
            outerSpecp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Clock event before property call and in its body");
            ڌڪو_Deletep(propSpecp->sensesp()->unlink_Fr_پٺ());
        }
        إذا (outerSpecp->sensesp()) {
            Ast_Sen_شي* ثابت sensesp = outerSpecp->sensesp();
            sensesp->unlink_Fr_پٺ();
            propSpecp->sensesp(sensesp);
        }

        outerSpecp->replaceWith(propSpecp);
        VL_DO_DANGLING(ڌڪو_Deletep(outerSpecp), outerSpecp);
    }

    فراغ inlineSequenceRef(AstFuncRef* funcrefp, AstSequence* seqp) {
        Ast_ڳنڍ_اظهار* ثابت bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        Ast_ڳنڍ_اظهار* ثابت clonedp = bodyExprp->clone_وڻ(خطأ);

        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_ڳنڍ_اظهار*> پورٽ_Map;
        لكل (ثابت خودڪار& tconnect : tconnects) {
            پورٽ_Map[tconnect.پهريون] = tconnect.ٻيو->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ثابت خودڪار it = پورٽ_Map.ڳوليو(refp->varp());
            إذا (it != پورٽ_Map.پڇاڙي()) {
                refp->replaceWith(it->ٻيو->clone_وڻ(خطأ));
                VL_DO_DANGLING(ڌڪو_Deletep(refp), refp);
            }
        });
        لكل (ثابت خودڪار& tconnect : tconnects) {
            ڌڪو_Deletep(tconnect.ٻيو->exprp()->unlink_Fr_پٺ());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ڌڪو_Deletep(funcrefp), funcrefp);
        // Clear referenced flag so V3AssertPre cleanup does not emit
        // spurious UNSUPPORTED for sequences that were already inlined here.
        seqp->isReferenced(خطأ);
    }

    // Must run before hasMultiCycleExpr() so NFA sees sequence bodies.
    فراغ inlineAllSequenceRefs(Ast_ڳنڍ* rootp) {
        منطقي بدليو = سچ;
        طالما (بدليو) {
            بدليو = خطأ;
            rootp->foreach([&](AstFuncRef* funcrefp) {
                إذا (بدليو) أرجع;
                إذا (AstSequence* ثابت seqp = VN_CAST(funcrefp->taskp(), Sequence)) {
                    inlineSequenceRef(funcrefp, seqp);
                    بدليو = سچ;
                }
            });
        }
    }

    ساكن منطقي has_Multi_چڪر_اظهار(ثابت Ast_ڳنڍ* nodep) {
        أرجع nodep->exists([](ثابت Ast_ڳنڍ* np) {
            إذا (ثابت خودڪار* ثابت ep = VN_CAST(np, ڳنڍ_اظهار)) أرجع ep->is_Multi_چڪر_Sva();
            أرجع خطأ;
        });
    }

    ساكن VPropStrength effectiveAssertPropStrength(ثابت AstPropSpec* ثابت propSpecp) {
        إذا (propSpecp->propStrength() != VPropStrength::DEFAULT) أرجع propSpecp->propStrength();
        أرجع propSpecp->fileline()->language() <= V3_Lang_ڪوڊ::L1800_2005 ? VPropStrength::STRONG
                                                                           : VPropStrength::WEAK;
    }

    // Bare `assert property (p until q)` with boolean operands stays on
    // V3AssertPre's AstLoop lowering, which preserves per-attempt action-block
    // firings that this NFA's single-bit aggregated state cannot. Strong bare
    // forms are also lowered there. NFA still owns sequence operands and any
    // embedding inside a multi-cycle context (implication consequent, or/and
    // operands, etc.).
    ساكن منطقي is_Bare_چوٽي_Level_Until(Ast_ڳنڍ* propp) {
        Ast_ڳنڍ* p = propp;
        إذا (AstPropSpec* ثابت specp = VN_CAST(p, PropSpec)) p = specp->propp();
        طالما (AstLogNot* ثابت notp = VN_CAST(p, LogNot)) p = notp->lhsp();
        AstUntil* ثابت untilp = VN_CAST(p, Until);
        إذا (!untilp) أرجع خطأ;
        ثابت خودڪار hasSeq = [](ثابت Ast_ڳنڍ_اظهار* ep) {
            أرجع ep->exists([](ثابت Ast_ڳنڍ_اظهار* np) { أرجع np->is_Multi_چڪر_Sva(); });
        };
        إذا (hasSeq(untilp->lhsp()) || hasSeq(untilp->rhsp())) أرجع خطأ;
        أرجع سچ;
    }

    بنية PropertyParts final {
        Ast_ڳنڍ_اظهار* triggerExprp = خالي_اشارو;
        Ast_ڳنڍ_اظهار* seqExprp = خالي_اشارو;
        منطقي isOverlapped = سچ;
        منطقي hasImplication = خطأ;
        منطقي isFollowedBy = خطأ;  // True for #-# / #=# (non-vacuous-fail on antecedent miss)
    };

    ساكن PropertyParts decomposeProperty(Ast_ڳنڍ* propp) {
        PropertyParts parts;
        إذا (AstPropSpec* ثابت specp = VN_CAST(propp, PropSpec)) { propp = specp->propp(); }
        إذا (AstImplication* ثابت implp = VN_CAST(propp, Implication)) {
            parts.hasImplication = سچ;
            parts.isOverlapped = implp->isOverlapped();
            parts.isFollowedBy = implp->isFollowedBy();
            parts.triggerExprp = implp->lhsp();
            parts.seqExprp = implp->rhsp();
        } وإلا إذا (Ast_ڳنڍ_اظهار* ثابت exprp = VN_CAST(propp, ڳنڍ_اظهار)) {
            parts.triggerExprp = خالي_اشارو;
            parts.seqExprp = exprp;
        }
        أرجع parts;
    }

    ساكن منطقي can_ورهايو_Implication_Pass_Actions(ثابت PropertyParts& parts) {
        UASSERT(parts.hasImplication,
                "Implication pass action split requested without implication");
        UASSERT(parts.triggerExprp, "Implication pass action split requested without trigger");
        // Direct vacuous/nonvacuous classification uses the antecedent value in the current
        // assertion attempt. Leave delayed antecedents on the existing NFA pass path.
        أرجع !has_Multi_چڪر_اظهار(parts.triggerExprp);
    }

    ساكن فراغ ورهايو_Implication_Pass_Actions(AstAssert* assertp, ثابت PropertyParts& parts,
                                            Ast_ڳنڍ_اظهار* nonvacuousMatchp) {
        File_سٽ* ثابت flp = assertp->fileline();

        Ast_ڳنڍ* ثابت passsp = assertp->passsp()->unlink_Fr_پٺ_With_ايندڙ();
        Ast_ڳنڍ* splitsp = خالي_اشارو;

        إذا (!parts.isFollowedBy) {
            Ast_ڳنڍ_اظهار* ثابت vacuousp
                = جديد AstLogNot{flp, sampled(parts.triggerExprp->clone_وڻ_خالص(خطأ))};
            Ast_ڳنڍ* ثابت vacuousBodyp = passsp->clone_وڻ(خطأ);
            splitsp = newPassOnIf(flp, vacuousp, vacuousBodyp, assertp->userType(),
                                  assertp->directive(), /*vacuous=*/سچ);
        }

        AstIf* ثابت nonvacuousp
            = newPassOnIf(flp, nonvacuousMatchp, passsp, assertp->userType(), assertp->directive(),
                          /*vacuous=*/خطأ);
        splitsp = splitsp ? Ast_ڳنڍ::جوڙيو_ايندڙ<Ast_ڳنڍ, Ast_ڳنڍ>(splitsp, nonvacuousp)
                          : static_cast<Ast_ڳنڍ*>(nonvacuousp);
        assertp->جوڙيو_Passsp(splitsp);
    }

    // On a PropSpec-wrapped assertion whose NFA build failed with a semantic
    // error (errorEmitted), replace the body with a BitFalse const so later
    // passes see a well-formed AST. Returns true if replaced.
    فراغ replaceBodyOnBuildError(File_سٽ* flp, AstPropSpec* propSpecp, منطقي errorEmitted) {
        إذا (!errorEmitted) أرجع;
        Ast_ڳنڍ* ثابت innerPropp = propSpecp->propp();
        innerPropp->replaceWith(جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}});
        VL_DO_DANGLING(ڌڪو_Deletep(innerPropp), innerPropp);
    }

    // Hoist a leading clocking event (IEEE 1800-2023 16.7):
    منطقي hoistClockedSeq(AstPropSpec* specp) {
        طالما (AstSClocked* ثابت clockedp = VN_CAST(specp->propp(), SClocked)) {
            إذا (specp->sensesp()) {
                clockedp->v3warn(E_UNSUPPORTED, "Unsupported: multiclocked sequence or property");
                replaceBodyOnBuildError(specp->fileline(), specp, سچ);
                أرجع سچ;
            }
            لكل (ثابت Ast_Sen_شي* sp = clockedp->sensesp(); sp;
                 sp = VN_CAST(sp->nextp(), Sen_شي)) {
                إذا (!sp->ڪنارو_Type().an_ڪنارو()) {
                    clockedp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: non-edge clocking event on a sequence; "
                                     "use an edge such as @(posedge clk)");
                    replaceBodyOnBuildError(specp->fileline(), specp, سچ);
                    أرجع سچ;
                }
            }
            specp->sensesp(clockedp->sensesp()->unlink_Fr_پٺ_With_ايندڙ());
            Ast_ڳنڍ_اظهار* ثابت bodyp = clockedp->exprp()->unlink_Fr_پٺ();
            clockedp->replaceWith(bodyp);
            VL_DO_DANGLING(ڌڪو_Deletep(clockedp), clockedp);
        }
        // A clocking event anywhere else in the sequence is not supported.
        ثابت AstSClocked* nestedp = خالي_اشارو;
        specp->propp()->foreach([&](ثابت AstSClocked* p) {
            إذا (!nestedp) nestedp = p;
        });
        إذا (nestedp) {
            nestedp->v3warn(E_UNSUPPORTED,
                            "Unsupported: clocking event inside sequence expression");
            replaceBodyOnBuildError(specp->fileline(), specp, سچ);
            أرجع سچ;
        }
        أرجع خطأ;
    }

    // Build the NFA graph for a property body, handling both the antecedent
    // |-> consequent and simple sequence cases. Returns the consequent/body
    // BuildResult (invalid on parse/build failure).
    Build_نتيجو build_Assertion_گراف(SvaNfaBuilder& builder, Sva_گراف& گراف, Ast_ڳنڍ_اظهار* seqBodyp,
                                    ثابت PropertyParts& parts, File_سٽ* flp) {
        إذا (!parts.hasImplication) أرجع builder.build(seqBodyp);

        گراف.m_شروعات_Vertexp = گراف.create_State_راس();
        أرجع builder.build_Implication_ڪنارا(parts.triggerExprp, seqBodyp, گراف.m_شروعات_Vertexp,
                                             parts.isOverlapped, parts.isFollowedBy,
                                             parts.triggerExprp, flp);
    }

    // Install pass-action gating and replay simultaneous per-thread failures.
    فراغ attachActionHandlers(AstAssert* ثابت assertp, Ast_ڳنڍ_اظهار* ميلاپ_Exprp,
                              Ast_Sen_وڻ* ثابت threadFailReplaySenTreep,
                              Ast_ڳنڍ_اظهار* ثابت threadFailCountp) {
        // Gate pass handler on match to prevent vacuous-pass firings.
        إذا (ميلاپ_Exprp) {
            // needMatch implies passsp() was non-null when evaluated above;
            // lowering does not mutate the assert's pass-action between the
            // two reads, so passsp() is still non-null here.
            Ast_ڳنڍ* passsp = assertp->passsp();
            UASSERT_OBJ(passsp, assertp, "needMatch set but passsp is null");
            passsp->unlink_Fr_پٺ_With_ايندڙ();
            File_سٽ* ثابت flp = assertp->fileline();
            AstIf* ثابت ifp = جديد AstIf{flp, ميلاپ_Exprp, passsp, خالي_اشارو};
            assertp->جوڙيو_Passsp(ifp);
            // Fail-handler prefix for overlapping instances (IEEE 16.12):
            // fires when reject=1 && match=1 in the same cycle.
            إذا (assertp->failsp()) assertp->prependFailsp(ifp->clone_وڻ(خطأ));
        }

        إذا (threadFailCountp) {
            UASSERT_OBJ(threadFailReplaySenTreep, assertp,
                        "Thread fail count missing sensitivity tree");
            Ast_ڳنڍ* ثابت failsp = assertp->failsp();
            File_سٽ* ثابت flp = assertp->fileline();
            // IEEE 1800-2023 16.12 requires one action-block evaluation per failed
            // thread. AstAssert handles the first, so replay the rest here.
            AstVar* ثابت بچيل_Fail_ڳڻپ_Varp
                = جديد AstVar{flp, VVarType::MODULETEMP, m_fail_ڳڻپ_نالا.get(""),
                             m_modp->ڳوليو_Basic_DType(VBasicDTypeKwd::UINT32)};
            بچيل_Fail_ڳڻپ_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جوڙيو_Stmtsp(بچيل_Fail_ڳڻپ_Varp);
            Ast_ڳنڍ* ثابت replayStmtsp = جديد Ast_سونپيو{
                flp, جديد AstVarRef{flp, بچيل_Fail_ڳڻپ_Varp, VAccess::WRITE}, threadFailCountp};
            AstLoop* ثابت replayLoopp = جديد AstLoop{flp};
            replayLoopp->جوڙيو_Stmtsp(جديد AstLoopTest{
                flp, replayLoopp,
                جديد AstGt{flp, جديد AstVarRef{flp, بچيل_Fail_ڳڻپ_Varp, VAccess::READ},
                          newTypedConstp(flp, بچيل_Fail_ڳڻپ_Varp->dtypep(), 1)}});
            replayLoopp->جوڙيو_Stmtsp(newIfAssertFailOn(failsp->clone_وڻ(سچ), assertp->directive(),
                                                     assertp->userType()));
            AstSub* ثابت decrementedFailCountp
                = جديد AstSub{flp, جديد AstVarRef{flp, بچيل_Fail_ڳڻپ_Varp, VAccess::READ},
                             newTypedConstp(flp, بچيل_Fail_ڳڻپ_Varp->dtypep(), 1)};
            replayLoopp->جوڙيو_Stmtsp(
                جديد Ast_سونپيو{flp, جديد AstVarRef{flp, بچيل_Fail_ڳڻپ_Varp, VAccess::WRITE},
                              decrementedFailCountp});
            replayStmtsp->جوڙيو_ايندڙ(replayLoopp);
            m_modp->جوڙيو_Stmtsp(
                جديد AstAlways{flp, VAlwaysKwd::ALWAYS, threadFailReplaySenTreep, replayStmtsp});
        }
    }

    // Replace one VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). No-op if refp is not in matchMap.
    فراغ substitute_ميلاپ_شي_Ref(AstVarRef* refp, بنانشان K,
                                ثابت std::unordered_map<ثابت AstVar*, Ast_ڳنڍ_اظهار*>& ميلاپ_Map) {
        ثابت خودڪار it = ميلاپ_Map.ڳوليو(refp->varp());
        إذا (it == ميلاپ_Map.پڇاڙي()) أرجع;
        Ast_ڳنڍ_اظهار* newp = it->ٻيو->clone_وڻ_خالص(خطأ);
        إذا (K > 0) {
            AstConst* ثابت ticksp = جديد AstConst{refp->fileline(), AstConst::Widthed_قدر{}, 32,
                                                  static_cast<uint32_t>(K)};
            AstPast* ثابت pastp = جديد AstPast{refp->fileline(), newp, ticksp, خالي_اشارو,
                                               /* propertyTiming */ سچ};
            pastp->dtypeFrom(newp);
            newp = pastp;
        }
        refp->replaceWith(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(refp), refp);
        أرجع;
    }

    // Recursively walk a consequent. Returns cycle length consumed and
    // substitutes each VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). Reports E_UNSUPPORTED on non-constant
    // delays or composite sequence operators and returns -1 on failure.
    صحيح
    walk_Substitute_ميلاپ_شيون(Ast_ڳنڍ_اظهار* nodep, بنانشان K,
                             ثابت std::unordered_map<ثابت AstVar*, Ast_ڳنڍ_اظهار*>& ميلاپ_شيون) {
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            // IEEE 1800-2023 16.9.2: cycle_delay's lhsp is a constant_expression
            // and the delay form in a sequence is always `##N`, folded by
            // V3Const + V3Param before V3AssertNfa. Range form `##[m:n]` is the
            // only user-visible reject here.
            AstDelay* ثابت delayp = VN_AS(sexprp->delayp(), Delay);
            UASSERT_OBJ(delayp->is_چڪر_Delay() && VN_IS(delayp->lhsp(), Const), sexprp,
                        "SVA cycle delay must have a constant lhsp");
            إذا (delayp->isRangeDelay()) {
                sexprp->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                              "non-constant cycle delay in consequent"
                                              " (IEEE 1800-2023 16.10)");
                أرجع -1;
            }
            ثابت بنانشان delay_چڪرن = VN_AS(delayp->lhsp(), Const)->منزل_UInt();
            صحيح preLen = 0;
            إذا (Ast_ڳنڍ_اظهار* ثابت prep = sexprp->preExprp()) {
                preLen = walk_Substitute_ميلاپ_شيون(prep, K, ميلاپ_شيون);
                إذا (preLen < 0) أرجع -1;
            }
            ثابت صحيح bodyLen
                = walk_Substitute_ميلاپ_شيون(sexprp->exprp(), K + preLen + delay_چڪرن, ميلاپ_شيون);
            إذا (bodyLen < 0) أرجع -1;
            أرجع preLen + delay_چڪرن + bodyLen;
        }
        إذا (nodep->is_Multi_چڪر_Sva()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                         "composite sequence operator in consequent"
                                         " (IEEE 1800-2023 16.10)");
            أرجع -1;
        }
        std::ويڪٽر<AstVarRef*> refs;
        nodep->foreach([&refs](AstVarRef* p) { refs.ڌڪو_پٺ(p); });
        لكل (AstVarRef* ثابت refp : refs) substitute_ميلاپ_شي_Ref(refp, K, ميلاپ_شيون);
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
    منطقي lift_ميلاپ_شي_Substitutions(PropertyParts& parts, Ast_ڳنڍ_اظهار* seqBodyp) {
        إذا (!parts.hasImplication) أرجع خطأ;
        Ast_اظهار_Stmt* ثابت اظهار_Stmtp = VN_CAST(parts.triggerExprp, اظهار_Stmt);
        إذا (!اظهار_Stmtp) أرجع خطأ;
        // IEEE 1800-2023 16.10 BNF requires `(expr, match_item {, match_item})`
        // with at least one match item; V3LinkParse only emits ExprStmt for
        // this form and only emits AstAssign with VarRef LHS for each item.
        std::unordered_map<ثابت AstVar*, Ast_ڳنڍ_اظهار*> ميلاپ_شيون;
        لكل (Ast_ڳنڍ* stmtp = اظهار_Stmtp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_سونپيو* ثابت assignp = VN_AS(stmtp, سونپيو);
            AstVarRef* ثابت lhsRefp = VN_AS(assignp->lhsp(), VarRef);
            ميلاپ_شيون[lhsRefp->varp()] = assignp->rhsp();
        }
        ثابت بنانشان شروعات_K = parts.isOverlapped ? 0 : 1;
        ثابت صحيح ڊيگهائي = walk_Substitute_ميلاپ_شيون(seqBodyp, شروعات_K, ميلاپ_شيون);
        // Match-item substitution / strip mutates ancestor purity. Release
        // builds don't auto-clear caches on edits, so refresh here.
        VIsCached::clear_Cache_وڻ();
        إذا (ڊيگهائي < 0) أرجع سچ;
        Ast_ڳنڍ_اظهار* ثابت antBoolp = اظهار_Stmtp->resultp()->unlink_Fr_پٺ();
        اظهار_Stmtp->replaceWith(antBoolp);
        VL_DO_DANGLING(ڌڪو_Deletep(اظهار_Stmtp), اظهار_Stmtp);
        parts.triggerExprp = antBoolp;
        أرجع خطأ;
    }

    // Outcome counts for a property if/case are wrong in an outcome-multiplying
    // context. Returns that context, or nullptr when the shape is supported.
    ساكن ثابت حرف* unsupportedPropertyControl(ثابت Ast_ڳنڍ_Cover_Or_Assert* assertp,
                                                  ثابت Ast_ڳنڍ_اظهار* seqBodyp, منطقي negated) {
        إذا (!hasPropertyControlConjunction(seqBodyp)) أرجع خالي_اشارو;
        إذا (negated) أرجع "negation";
        إذا (VN_IS(assertp, Cover)) أرجع "cover";
        إذا (VN_AS(assertp, Assert)->passsp()) أرجع "a pass action";
        أرجع خالي_اشارو;
    }

    // Inline property/sequence refs and reject unsupported shapes.
    // Returns the PropSpec to lower, or nullptr when fully handled here.
    AstPropSpec* prepareAssertionProp(Ast_ڳنڍ_Cover_Or_Assert* assertp) {
        إذا (AstPropSpec* ثابت specp = VN_CAST(assertp->propp(), PropSpec)) {
            إذا (AstFuncRef* ثابت funcrefp = VN_CAST(specp->propp(), FuncRef)) {
                إذا (ثابت AstProperty* ثابت propyp = VN_CAST(funcrefp->taskp(), Property)) {
                    inlineNamedProperty(specp, funcrefp, propyp);
                }
            }
        }

        inlineAllSequenceRefs(assertp->propp());

        إذا (AstPropSpec* ثابت specp = VN_CAST(assertp->propp(), PropSpec)) {
            إذا (hoistClockedSeq(specp)) أرجع خالي_اشارو;
        }

        AstPropSpec* ثابت propp = VN_AS(assertp->propp(), PropSpec);
        إذا (!VN_IS(assertp, Cover)
            && effectiveAssertPropStrength(propp) == VPropStrength::STRONG) {
            propp->v3warn(E_UNSUPPORTED,
                          "Unsupported: strong property in " + assertp->verilogKwd() + ".");
            replaceBodyOnBuildError(assertp->fileline(), propp, /*errorEmitted=*/سچ);
            أرجع خالي_اشارو;
        }

        إذا (!has_Multi_چڪر_اظهار(propp)) أرجع خالي_اشارو;
        // A nested property instance keeps its body behind the call; lowering would drop it.
        إذا (propp->exists([](ثابت AstFuncRef* refp) { أرجع VN_IS(refp->taskp(), Property); })) {
            assertp->v3warn(E_UNSUPPORTED,
                            "Unsupported: property instance inside a multi-cycle property "
                            "expression");
            VL_DO_DANGLING(ڌڪو_Deletep(assertp->unlink_Fr_پٺ()), assertp);
            أرجع خالي_اشارو;
        }
        إذا (is_Bare_چوٽي_Level_Until(propp)) أرجع خالي_اشارو;
        أرجع propp;
    }

    فراغ processAssertion(Ast_ڳنڍ_Cover_Or_Assert* assertp) {
        إذا (assertp->فوري()) أرجع;

        AstPropSpec* ثابت propp = prepareAssertionProp(assertp);
        إذا (!propp) أرجع;

        PropertyParts parts = decomposeProperty(propp);
        UASSERT_OBJ(parts.seqExprp, propp, "Property body must be an expression");

        // Unwrap `not` (IEEE 1800-2023 16.12.1); odd count -> negated semantics.
        Ast_ڳنڍ_اظهار* seqBodyp = parts.seqExprp;
        منطقي negated = خطأ;
        طالما (AstLogNot* ثابت notp = VN_CAST(seqBodyp, LogNot)) {
            negated = !negated;
            seqBodyp = notp->lhsp();
        }

        ثابت حرف* ثابت propertyControlp
            = unsupportedPropertyControl(assertp, seqBodyp, negated);

        // Substitute property-local match-item refs in consequent with
        // $past(rhs, K) before NFA build (IEEE 1800-2023 16.10).
        إذا (lift_ميلاپ_شي_Substitutions(parts, seqBodyp)) {
            AstPropSpec* ثابت psp = VN_CAST(assertp->propp(), PropSpec);
            UASSERT_OBJ(psp, assertp, "Concurrent assertion must have PropSpec");
            replaceBodyOnBuildError(assertp->fileline(), psp, /*errorEmitted=*/سچ);
            أرجع;
        }

        AstCover* ثابت coverp = VN_CAST(assertp, Cover);
        ثابت منطقي isCoverSeq = coverp && coverp->isCoverSeq();
        // A sequence event control is not an assertion directive; no default
        // disable iff, no assertion control
        ثابت منطقي isSeqEvent = coverp && coverp->isSeqEvent();
        ثابت منطقي ڳڻپ_Matches = isCoverSeq && !isSeqEvent;
        // Inherit module defaults (IEEE 14.12, 16.15) when assertion has none.
        إذا (!propp->sensesp() && m_defaultClockingp) {
            propp->sensesp(m_defaultClockingp->sentreep()->sensesp()->clone_وڻ(سچ));
        }
        إذا (!propp->disablep() && m_defaultDisablep && !isSeqEvent) {
            propp->disablep(m_defaultDisablep->condp()->clone_وڻ_خالص(سچ));
        }
        إذا (!propp->sensesp()) أرجع;
        Ast_Sen_وڻ* senTreep
            = جديد Ast_Sen_وڻ{propp->fileline(), propp->sensesp()->clone_وڻ(سچ)};
        Ast_ڳنڍ_اظهار* ثابت disableExprp = propp->disablep();

        // NFA lowering clones repeated operands and may hoist them into an
        // always_comb block. Resolve implicit sampled-value clocks first, while
        // the enclosing assertion clock is still available.
        {
            VL_RESTORER(m_sampled_قدر_Clockp);
            m_sampled_قدر_Clockp = senTreep;
            iterate(propp->propp());
        }

        File_سٽ* ثابت flp = assertp->fileline();

        Sva_گراف گراف;
        SvaNfaBuilder builder{گراف,      m_modp,     m_prop_عارضي_نالا,
                              isCoverSeq, isSeqEvent, coverp != خالي_اشارو};

        ثابت Build_نتيجو نتيجو = build_Assertion_گراف(builder, گراف, seqBodyp, parts, flp);
        إذا (نتيجو.valid()) wire_ميلاپ_And_Mid_Sources(گراف, نتيجو, flp);
        إذا (!نتيجو.valid()) {
            // Fall through to V3AssertPre for unsupported constructs; only
            // replace the body on real semantic errors. Any hoisted temps
            // from this attempt become orphan MODULETEMPs; V3Dead removes
            // them along with the dead always_comb driver.
            replaceBodyOnBuildError(flp, propp, نتيجو.errorEmitted);
            VL_DO_DANGLING(ڌڪو_Deletep(senTreep), senTreep);
            أرجع;
        }
        // After the build, so a construct the builder rejects reports itself.
        إذا (propertyControlp) {
            seqBodyp->v3warn(E_UNSUPPORTED,
                             "Unsupported: temporal property if/case with " << propertyControlp);
            replaceBodyOnBuildError(flp, propp, /*errorEmitted=*/سچ);
            VL_DO_DANGLING(ڌڪو_Deletep(senTreep), senTreep);
            أرجع;
        }
        إذا (ڳڻپ_Matches && !v3Global.opt.coverageUser()) {
            SvaNfaBuilder::freeUnlinkedCondp(نتيجو.finalCondp);
            VL_DO_DANGLING(ڌڪو_Deletep(senTreep), senTreep);
            Ast_ڳنڍ* ثابت innerPropp = propp->propp();
            innerPropp->replaceWith(جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}});
            VL_DO_DANGLING(ڌڪو_Deletep(innerPropp), innerPropp);
            أرجع;
        }

        AstAssert* ثابت assertAssertp = VN_CAST(assertp, Assert);
        ثابت منطقي ورهايو_Implication_Passsp = assertAssertp && assertAssertp->passsp()
                                            && parts.hasImplication
                                            && can_ورهايو_Implication_Pass_Actions(parts);
        ثابت منطقي need_ميلاپ = assertAssertp && assertAssertp->passsp()
                               && (!parts.hasImplication || ورهايو_Implication_Passsp);

        ثابت منطقي needsThreadFailReplay
            = assertAssertp && assertAssertp->failsp() && !parts.hasImplication;
        ثابت خودڪار سگنل
            = m_loweringp->هيٺاهون(assertp, گراف, senTreep, نتيجو.finalCondp, disableExprp,
                                 needsThreadFailReplay, needsThreadFailReplay && !negated);
        Ast_ڳنڍ_اظهار* ميلاپ_Exprp = خالي_اشارو;
        Ast_ڳنڍ_اظهار* outputExprp = m_loweringp->assemble_نتيجو(
            assertp, negated, نتيجو.finalCondp, سگنل, need_ميلاپ ? &ميلاپ_Exprp : خالي_اشارو);
        إذا (ڳڻپ_Matches) {
            UASSERT_OBJ(سگنل.ميلاپ_Countp, coverp, "Cover sequence missing match count");
            VL_DO_DANGLING(outputExprp->delete_وڻ(), outputExprp);
            propp->ميلاپ_Countp(سگنل.ميلاپ_Countp);
            outputExprp = جديد AstNeq{flp, سگنل.ميلاپ_Countp->clone_وڻ_خالص(خطأ),
                                     newTypedConstp(flp, سگنل.ميلاپ_Countp->dtypep(), 0)};
        }

        Ast_Sen_وڻ* ثابت threadFailReplaySenTreep
            = سگنل.threadFailCountp ? senTreep->clone_وڻ(خطأ) : خالي_اشارو;

        VL_DO_DANGLING(ڌڪو_Deletep(senTreep), senTreep);
        إذا (نتيجو.finalCondp && !نتيجو.finalCondp->backp()) ڌڪو_Deletep(نتيجو.finalCondp);

        إذا (ورهايو_Implication_Passsp) {
            ورهايو_Implication_Pass_Actions(assertAssertp, parts, ميلاپ_Exprp);
        } وإلا {
            attachActionHandlers(assertAssertp, ميلاپ_Exprp, threadFailReplaySenTreep,
                                 سگنل.threadFailCountp);
        }

        Ast_ڳنڍ* ثابت innerPropp = propp->propp();
        innerPropp->replaceWith(outputExprp);
        VL_DO_DANGLING(ڌڪو_Deletep(innerPropp), innerPropp);

        UINFO(4, "NFA converted assertion at " << flp << سٽ_پڄاڻي);

        إذا (dump_گراف_Level() >= 6) گراف.m_گراف.dumpDotFilePrefixed("assert-nfa");
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_loweringp);
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultDisablep);
        m_modp = nodep;
        // A class nested in a module is in the scope of the module's default clocking
        إذا (!VN_IS(nodep, Class)) m_defaultClockingp = خالي_اشارو;
        m_defaultDisablep = nodep->defaultDisablep();
        SvaNfaLowering lowering{nodep};
        m_loweringp = &lowering;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstClocking* nodep) override {
        إذا (nodep->isDefault() && !m_defaultClockingp) m_defaultClockingp = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_Gen_بلاڪ* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    فراغ گهمو(AstDefaultDisable* nodep) override {}
    فراغ گهمو(AstFell* nodep) override { گهمو_Sampled_قدر(nodep); }
    فراغ گهمو(AstPast* nodep) override { گهمو_Sampled_قدر(nodep); }
    فراغ گهمو(AstRose* nodep) override { گهمو_Sampled_قدر(nodep); }
    فراغ گهمو(AstStable* nodep) override { گهمو_Sampled_قدر(nodep); }
    فراغ گهمو(AstAssert* nodep) override { processAssertion(nodep); }
    فراغ گهمو(AstCover* nodep) override { processAssertion(nodep); }
    فراغ گهمو(AstRestrict* nodep) override {
        // Restrict property is ignored by simulators (IEEE 1800-2023 16.12.2).
        // Remove here so temporal SExpr don't leak to V3AssertPre.
        VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
    }
    فراغ گهمو(AstAssertIntrinsic* nodep) override {}
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    واضح AssertNfaVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Top entry point

فراغ V3AssertNfa::assertNfaAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":" << سٽ_پڄاڻي);
    { AssertNfaVisitor{nodep}; }
    V3Global::dump_Check_Global_وڻ("assertnfa", 0, dump_وڻ_Either_Level() >= 3);
}
