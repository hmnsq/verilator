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
بنية Sva_راس_انکڑے final {
    AstVar* stateVarp = خالی_سوچک;  // NBA state register for this vertex
    AstVar* delayRingVarp = خالی_سوچک;  // Packed occupancy bits or per-slot match counts
    AstVar* delay_Ring_سر_Varp
        = خالی_سوچک;  // Cached outgoing slot; isolates readers from in-place writes
    AstVar* delayRingIdxVarp = خالی_سوچک;  // Next slot written in delayRingVarp
    AstVar* delay_Ring_جیوندا_گنتی_Varp = خالی_سوچک;  // Number of threads in delayRingVarp
    AstVar* delayRingWrappedVarp = خالی_سوچک;  // All slots written since the last clear
    AstVar* پورا_LVarp = خالی_سوچک;  // SAnd LHS done-latch
    AstVar* پورا_RVarp = خالی_سوچک;  // SAnd RHS done-latch
    Ast_گرہ_اظہاریہ* stateSigp = خالی_سوچک;  // Combinational state signal; OWNED during lowering
    Ast_گرہ_اظہاریہ* state_گنتی_Sigp = خالی_سوچک;  // Cover-sequence match count; OWNED during lowering
    Ast_گرہ_اظہاریہ* state_گنتی_حد_توں_ودھ_Sigp = خالی_سوچک;  // Count overflow; OWNED during lowering
    منطقي delayRingCounts = خطأ;  // Ring slots hold cover-sequence match counts
    منطقي needsReg = خطأ;  // True if vertex has incoming clocked edge
};

// NFA state vertex -- one per NFA position in the sequence evaluation
صنف Sva_State_راس final : عام V3_گراف_راس {
    VL_RTTI_IMPL(Sva_State_راس, V3_گراف_راس)
عام:
    // True if this is the sequence-match terminal vertex
    منطقي m_is_میل = خطأ;
    // OWNED throughout-guard condition clones; IEEE 1800-2023 16.9.9
    std::سمتیہ<Ast_گرہ_اظہاریہ*> m_throughoutConds;
    // Nonzero for a ring-buffer vertex for ## delays.
    منطقي m_isFixedDelayRing = خطأ;
    بنانشانی m_delay_Ring_ناپ = 0;  // Number of ring slots. Range: max-min+1.
    Ast_گرہ_اظہاریہ* m_delayRingClearCondp = خالی_سوچک;  // local RHS for pure-boolean range
    Ast_گرہ_اظہاریہ* m_delayRingAdvanceCondp = خالی_سوچک;  // Advance only when this condition holds
    منطقي m_replayAbortReject = خطأ;  // Compressed repetition needs per-thread abort replay
    // OWNED; enclosing-abort fire condition clearing state or suppressing guard rejection
    Ast_گرہ_اظہاریہ* m_abortClearp = خالی_سوچک;
    // OWNED; reject-abort fire condition rejecting all represented live threads
    Ast_گرہ_اظہاریہ* m_abortRejectp = خالی_سوچک;
    // Liveness terminal (IEEE weak semantics): reject must not fire from this source
    منطقي m_isUnbounded = خطأ;
    // Temporal sequence AND combiner; IEEE 1800-2023 16.9.5
    منطقي m_isAndCombiner = خطأ;
    Sva_State_راس* m_andLhsTermp = خالی_سوچک;  // LHS sub-NFA terminal vertex
    Sva_State_راس* m_andRhsTermp = خالی_سوچک;  // RHS sub-NFA terminal vertex
    Ast_گرہ_اظہاریہ* m_andLhsCondp = خالی_سوچک;  // OWNED; LHS final condition (may be nullptr)
    Ast_گرہ_اظہاریہ* m_andRhsCondp = خالی_سوچک;  // OWNED; RHS final condition (may be nullptr)
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
        لكل (Ast_گرہ_اظہاریہ* cp : m_throughoutConds) VL_DO_DANGLING(cp->delete_درخت(), cp);
        إذا (m_delayRingClearCondp)
            VL_DO_DANGLING(m_delayRingClearCondp->delete_درخت(), m_delayRingClearCondp);
        إذا (m_delayRingAdvanceCondp)
            VL_DO_DANGLING(m_delayRingAdvanceCondp->delete_درخت(), m_delayRingAdvanceCondp);
        إذا (m_abortClearp) VL_DO_DANGLING(m_abortClearp->delete_درخت(), m_abortClearp);
        إذا (m_abortRejectp) VL_DO_DANGLING(m_abortRejectp->delete_درخت(), m_abortRejectp);
        إذا (m_andLhsCondp) VL_DO_DANGLING(m_andLhsCondp->delete_درخت(), m_andLhsCondp);
        إذا (m_andRhsCondp) VL_DO_DANGLING(m_andRhsCondp->delete_درخت(), m_andRhsCondp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string name() ثابت override {
        string name = "s" + cvt_منزل_Str(color());
        إذا (m_delay_Ring_ناپ) {
            name += "\\n";
            name += m_isFixedDelayRing ? "fixed chain " : "range chain ";
            name += cvt_منزل_Str(m_delay_Ring_ناپ) + " slots";
        }
        أرجع name;
    }
    string dotColor() ثابت override {
        إذا (m_is_میل) أرجع "red";
        إذا (m_delay_Ring_ناپ) أرجع "blue";
        إذا (m_isAndCombiner) أرجع "purple";
        أرجع "black";
    }
    // LCOV_EXCL_STOP
    // Access per-vertex algorithm data (valid only during lowering phase)
    Sva_راس_انکڑے* datap() ثابت { أرجع static_cast<Sva_راس_انکڑے*>(userp()); }
};

// NFA transition edge -- clocked (##1) or combinational link (##0)
صنف Sva_Trans_کنارا final : عام V3_گراف_کنارا {
    VL_RTTI_IMPL(Sva_Trans_کنارا, V3_گراف_کنارا)
عام:
    Ast_گرہ_اظہاریہ* m_condp;  // Transition condition; nullptr = unconditional; OWNED
    منطقي m_consumes_چکر;  // true = clocked edge (##1), false = link (##0/boolean)
    // Reject when source is active and condp is false; set only on
    // outermost required-step Link
    منطقي m_rejectOnFail = خطأ;
    // Optional dynamic condition vertex for m_rejectOnFail. Used when the
    // success condition is another NFA state rather than a static expression.
    Sva_State_راس* m_condVtxp = خالی_سوچک;

    // CONSTRUCTORS
    Sva_Trans_کنارا(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی, Ast_گرہ_اظہاریہ* condp,
                 منطقي consumes_چکر)
        : V3_گراف_کنارا{graphp, fromp, چوٹی, /*weight=*/1}
        , m_condp{condp}
        , m_consumes_چکر{consumes_چکر} {}
    ~Sva_Trans_کنارا() override {
        إذا (m_condp) VL_DO_DANGLING(m_condp->delete_درخت(), m_condp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string dotLabel() ثابت override { أرجع m_consumes_چکر ? "##1" : "link"; }
    string dotStyle() ثابت override { أرجع m_consumes_چکر ? "" : "dashed"; }
    // LCOV_EXCL_STOP
    // Typed accessors for NFA vertices
    Sva_State_راس* fromVtxp() ثابت { أرجع static_cast<Sva_State_راس*>(fromp()); }
    Sva_State_راس* منزل_Vtxp() ثابت { أرجع static_cast<Sva_State_راس*>(چوٹی()); }
};

// NFA graph container
صنف Sva_گراف final {
عام:
    V3_گراف m_گراف;  // Owns all vertices and edges
    Sva_State_راس* m_شروع_Vertexp = خالی_سوچک;  // Trigger/start vertex
    Sva_State_راس* m_میل_Vertexp = خالی_سوچک;  // Sequence-match terminal vertex

    // Create a new state vertex
    Sva_State_راس* create_State_راس() { أرجع جديد Sva_State_راس{&m_گراف}; }
    // Create the match terminal vertex
    Sva_State_راس* create_میل_راس() {
        Sva_State_راس* ثابت vtxp = create_State_راس();
        vtxp->m_is_میل = سچ;
        m_میل_Vertexp = vtxp;
        أرجع vtxp;
    }
    // Add a clocked transition edge (##1)
    Sva_Trans_کنارا* جوڑو_Clocked_کنارا(Sva_State_راس* fromp, Sva_State_راس* چوٹی,
                                 Ast_گرہ_اظہاریہ* condp = خالی_سوچک) {
        أرجع جديد Sva_Trans_کنارا{&m_گراف, fromp, چوٹی, condp, /*consumesCycle=*/سچ};
    }
    // Add a combinational link (##0 / boolean condition)
    Sva_Trans_کنارا* جوڑو_Link(Sva_State_راس* fromp, Sva_State_راس* چوٹی,
                          Ast_گرہ_اظہاریہ* condp = خالی_سوچک) {
        أرجع جديد Sva_Trans_کنارا{&m_گراف, fromp, چوٹی, condp, /*consumesCycle=*/خطأ};
    }
    // Collect all edges into a flat vector for iteration.
    // Used by the lowering phase which needs global edge scans.
    std::سمتیہ<ثابت Sva_Trans_کنارا*> all_کنارے() ثابت {
        std::سمتیہ<ثابت Sva_Trans_کنارا*> نتیجہ;
        لكل (ثابت V3_گراف_راس& vtxr : m_گراف.vertices()) {
            لكل (ثابت V3_گراف_کنارا& edger : vtxr.باہرلا_کنارے()) {
                نتیجہ.ڈالو_پٹھ(static_cast<ثابت Sva_Trans_کنارا*>(&edger));
            }
        }
        أرجع نتیجہ;
    }
};

//######################################################################
// Builder result: terminal vertex + optional final condition (match Link condition).
بنية Build_نتیجہ final {
    Sva_State_راس* رکن_Vertexp;  // Primary terminal; contributes to both match and reject
    Ast_گرہ_اظہاریہ* finalCondp;  // nullptr = unconditional
    // Mid-window sources for range delays (pure boolean RHS): match-only (isUnbounded)
    std::سمتیہ<Sva_State_راس*> midSources;
    منطقي errorEmitted = خطأ;  // Builder already emitted specific error; skip generic
    // For cover_sequence: when true, midSources already enumerate every
    // end-of-match, so wireMatchAndMidSources must NOT add the main
    // termVtxp -> matchVertex Link (would double-count via the merge vertex).
    منطقي رکن_Is_Mid_ملاؤ = خطأ;
    منطقي valid() ثابت { أرجع رکن_Vertexp != خالی_سوچک; }
    ساكن Build_نتیجہ fail(منطقي errored = خطأ) { أرجع {خالی_سوچک, خالی_سوچک, {}, errored}; }
    ساكن Build_نتیجہ failWithError() { أرجع {خالی_سوچک, خالی_سوچک, {}, سچ}; }
};

// Parser-marked SAnd of overlapped implications: a property if/else/case.
ساكن منطقي hasPropertyControlConjunction(ثابت Ast_گرہ_اظہاریہ* nodep) {
    أرجع nodep->exists([](ثابت AstSAnd* andp) { أرجع andp->propertyControl(); });
}

ساكن AstConst* newTypedConstp(File_سطر* ثابت flp, ثابت Ast_گرہ_DType* ثابت dtypep,
                                ثابت uint32_t قدر) {
    AstConst* ثابت constp = جديد AstConst{flp, AstConst::DTyped{}, dtypep};
    constp->عدد().سیٹ_Long(قدر);
    أرجع constp;
}

ساكن Ast_گرہ_اظہاریہ* sampled(Ast_گرہ_اظہاریہ* exprp) {
    أرجع جديد AstSampled{exprp->fileline(), exprp, exprp->dtypep(), سچ};
}

ساكن ثابت حرف* assertPassOnQuery(منطقي vacuous) {
    ساكن constexpr ثابت حرف* queries[2]
        = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
    أرجع queries[vacuous];
}

ساكن Ast_گرہ_اظہاریہ* assertOnCond(File_سطر* flp, VAssertType type,
                                 VAssertDirectiveType directiveType) {
    إذا (!v3Global.opt.assertOn()) { أرجع جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}; }
    أرجع جديد AstCExpr{flp, AstCExpr::خالص{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
}

ساكن Ast_گرہ_اظہاریہ* assertKillGet(File_سطر* flp, VAssertType type,
                                  VAssertDirectiveType directiveType) {
    أرجع جديد AstCExpr{flp, AstCExpr::خالص{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_KILL", type, directiveType),
                        32};
}

ساكن string assert_Action_Control_اگیتر(VAssertDirectiveType directiveType) {
    ثابت صحيح controlled = !!(static_cast<صحيح>(directiveType)
                              & (static_cast<صحيح>(VAssertDirectiveType::ASSERT)
                                 | static_cast<صحيح>(VAssertDirectiveType::COVER)
                                 | static_cast<صحيح>(VAssertDirectiveType::ASSUME)));
    ثابت صحيح checkRuntime = controlled & static_cast<صحيح>(v3Global.opt.assertOn());
    أرجع "("s + std::منزل_string(controlled ^ 1) + " || ("s + std::منزل_string(checkRuntime)
           + " && "s;
}

ساكن Ast_گرہ_اظہاریہ* assertPassOnCond(File_سطر* flp, VAssertType type,
                                     VAssertDirectiveType directiveType, منطقي vacuous) {
    أرجع جديد AstCExpr{
        flp, AstCExpr::خالص{},
        assert_Action_Control_اگیتر(directiveType)
            + V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType)
            + "))"s,
        1};
}

ساكن Ast_گرہ_اظہاریہ* assertFailOnCond(File_سطر* flp, VAssertType type,
                                     VAssertDirectiveType directiveType) {
    أرجع جديد AstCExpr{
        flp, AstCExpr::خالص{},
        assert_Action_Control_اگیتر(directiveType)
            + V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType) + "))"s,
        1};
}

ساكن AstIf* newPassOnIf(File_سطر* flp, Ast_گرہ_اظہاریہ* firep, Ast_گرہ* bodyp, VAssertType type,
                          VAssertDirectiveType directiveType, منطقي vacuous) {
    Ast_گرہ_اظہاریہ* ثابت condp
        = جديد AstLogAnd{flp, firep, assertPassOnCond(flp, type, directiveType, vacuous)};
    AstIf* ثابت ifp = جديد AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(سچ);
    ifp->user1(سچ);
    أرجع ifp;
}

ساكن Ast_گرہ_Stmt* newIfAssertFailOn(Ast_گرہ* bodyp, VAssertDirectiveType directiveType,
                                      VAssertType type) {
    File_سطر* ثابت flp = bodyp->fileline();
    Ast_گرہ_اظہاریہ* ثابت condp = assertFailOnCond(flp, type, directiveType);
    AstIf* ثابت ifp = جديد AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(سچ);
    ifp->user1(سچ);
    أرجع ifp;
}

//######################################################################
// NFA Builder

صنف SvaNfaBuilder final {
    Sva_گراف& m_گراف;  // NFA graph being built
    Ast_گرہ_Module* ثابت m_modp;  // Module to receive hoisted sampled-prop temps
    V3_Unique_ناں& m_prop_عارضی_ناں;  // Module-shared temp-var name source
    std::سمتیہ<Ast_گرہ_اظہاریہ*> m_temporal_Guard_ڈھیر;  // Guards active across nested temporal states
    // Outer abort conditions, AND-ed as !cond into inner abort edges
    // (IEEE 1800-2023 16.12.14 outer-wraps-inner).
    std::سمتیہ<Ast_گرہ_اظہاریہ*> m_outer_Abort_ڈھیر;
    منطقي m_وچ_Unbounded_Scope = خطأ;  // Sticky: nodes created after inherit liveness
    منطقي m_markStrongPending = خطأ;  // Mark new vertices as strong s_always in-window
    منطقي m_isCover = خطأ;  // Cover directives do not fail at end-of-simulation
    // IEEE 1800-2023 16.14.3 cover sequence: each end-of-match fires the action,
    // not just the first. Builder builds parallel-branch (no first-match-wins)
    // topology when true. Default false preserves cover_property semantics.
    منطقي m_isCoverSeq = خطأ;
    // Unsupported endpoint topology must reject, not ignore, or the wait hangs
    منطقي m_isSeqEvent = خطأ;
    منطقي m_وچ_Sequence_اگیتر = خطأ;  // Prefix composition does not preserve every endpoint
    منطقي m_want_Bool_پتی = خطأ;  // Building a |-> / |=> or top-level body (see buildOrMerge)

    بنية RangeDelayRejectInfo final {
        Sva_State_راس* startp = خالی_سوچک;
        بنانشانی range = 0;
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

    Ast_گرہ_اظہاریہ* throughoutCond(Ast_گرہ_اظہاریہ* ادھار_Condp, File_سطر* flp) {
        إذا (m_temporal_Guard_ڈھیر.خالی_تھاواں()) أرجع ادھار_Condp;
        // AND all active temporal guards (supports nesting)
        // Each must use $sampled values.
        Ast_گرہ_اظہاریہ* guardp = خالی_سوچک;
        لكل (Ast_گرہ_اظہاریہ* ثابت condp : m_temporal_Guard_ڈھیر) {
            Ast_گرہ_اظہاریہ* ثابت clonep = sampled(condp->clone_درخت_خالص(خطأ));
            إذا (!guardp) {
                guardp = clonep;
            } وإلا {
                guardp = جديد AstLogAnd{flp, guardp, clonep};
            }
        }
        إذا (ادھار_Condp) { guardp = جديد AstLogAnd{flp, ادھار_Condp, guardp}; }
        أرجع guardp;
    }

    ساكن بنانشانی getConstUInt(Ast_گرہ_اظہاریہ* exprp) {
        Ast_گرہ_اظہاریہ* ثابت constp = V3Const::constifyEdit(exprp->clone_درخت_خالص(خطأ));
        ثابت AstConst* ثابت cp = VN_CAST(constp, Const);
        ثابت بنانشانی val = cp ? cp->منزل_UInt() : 0;
        VL_DO_DANGLING(constp->delete_درخت(), constp);
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
    ساكن صحيح fixed_لمائی(Ast_گرہ_اظہاریہ* nodep) {
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
            إذا (!delayp || !delayp->is_چکر_Delay()) أرجع -1;
            بنانشانی delay_چکراں;
            إذا (delayp->isRangeDelay()) {
                إذا (delayp->isUnbounded()) أرجع -1;  // LCOV_EXCL_LINE
                ثابت بنانشانی minD = getConstUInt(delayp->lhsp());
                ثابت بنانشانی maxD = getConstUInt(delayp->rhsp());
                إذا (minD != maxD) أرجع -1;
                delay_چکراں = minD;
            } وإلا {
                delay_چکراں = getConstUInt(delayp->lhsp());
            }
            صحيح preLen = 0;
            إذا (Ast_گرہ_اظہاریہ* ثابت prep = sexprp->preExprp()) {
                preLen = fixed_لمائی(prep);
                إذا (preLen < 0) أرجع -1;  // LCOV_EXCL_LINE
            }
            ثابت صحيح bodyLen = fixed_لمائی(sexprp->exprp());
            إذا (bodyLen < 0) أرجع -1;  // LCOV_EXCL_LINE
            أرجع preLen + delay_چکراں + bodyLen;
        }
        إذا (AstSThroughout* ثابت throughp = VN_CAST(nodep, SThroughout)) {
            أرجع fixed_لمائی(throughp->rhsp());
        }
        إذا (AstSOr* ثابت orp = VN_CAST(nodep, SOr)) {
            // Alternatives must share one end cycle; buildSWithin relies on
            // this to pair the OR with an SIntersect.
            ثابت صحيح lhsLen = fixed_لمائی(orp->lhsp());
            ثابت صحيح rhsLen = fixed_لمائی(orp->rhsp());
            إذا (lhsLen < 0 || rhsLen < 0 || lhsLen != rhsLen) أرجع -1;
            أرجع lhsLen;
        }
        إذا (AstSWithin* ثابت withinp = VN_CAST(nodep, SWithin)) {
            // `seq1 within seq2` ends at seq2's end cycle (IEEE 16.9.10).
            ثابت صحيح lhsLen = fixed_لمائی(withinp->lhsp());
            ثابت صحيح rhsLen = fixed_لمائی(withinp->rhsp());
            إذا (lhsLen < 0 || rhsLen < 0 || lhsLen > rhsLen) أرجع -1;
            أرجع rhsLen;
        }
        // LCOV_EXCL_START -- defensive: V3AssertPre rejects composite SVA ops
        // nested in an intersect arm before fixedLength runs (clock-context
        // resolution fails). Kept as a guard in case future parser relaxations
        // permit it.
        إذا (nodep->is_Multi_چکر_Sva()) أرجع -1;
        // LCOV_EXCL_STOP
        // Plain boolean expression (no SVA constructs) -- 0 cycles.
        أرجع 0;
    }

    // Contiguous match-length range [lo,hi] for an operand whose length varies
    // from AT MOST ONE ranged cycle delay; {-1,-1} otherwise. Drives the
    // variable-length `intersect` lowering (IEEE 1800-2023 16.9.6): more than
    // one ranged delay would make the per-length realization ambiguous, so it
    // is reported unsupported rather than mis-paired.
    ساكن std::pair<صحيح, صحيح> لمائی_Range(Ast_گرہ_اظہاریہ* nodep) {
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
            إذا (!delayp || !delayp->is_چکر_Delay()) أرجع {-1, -1};
            std::pair<صحيح, صحيح> delayRange;
            إذا (delayp->isRangeDelay()) {
                إذا (delayp->isUnbounded()) أرجع {-1, -1};
                ثابت بنانشانی minD = getConstUInt(delayp->lhsp());
                ثابت بنانشانی maxD = getConstUInt(delayp->rhsp());
                delayRange = {minD, maxD};
            } وإلا {
                ثابت بنانشانی d = getConstUInt(delayp->lhsp());
                delayRange = {d, d};
            }
            std::pair<صحيح, صحيح> preRange{0, 0};
            إذا (Ast_گرہ_اظہاریہ* ثابت prep = sexprp->preExprp()) {
                preRange = لمائی_Range(prep);
                إذا (preRange.پہلا < 0) أرجع {-1, -1};
            }
            ثابت std::pair<صحيح, صحيح> bodyRange = لمائی_Range(sexprp->exprp());
            إذا (bodyRange.پہلا < 0) أرجع {-1, -1};
            ثابت صحيح variableParts = (preRange.پہلا != preRange.دوجا)
                                      + (delayRange.پہلا != delayRange.دوجا)
                                      + (bodyRange.پہلا != bodyRange.دوجا);
            إذا (variableParts > 1) أرجع {-1, -1};
            أرجع {preRange.پہلا + delayRange.پہلا + bodyRange.پہلا,
                    preRange.دوجا + delayRange.دوجا + bodyRange.دوجا};
        }
        إذا (AstSThroughout* ثابت throughp = VN_CAST(nodep, SThroughout)) {
            أرجع لمائی_Range(throughp->rhsp());
        }
        إذا (nodep->is_Multi_چکر_Sva()) أرجع {-1, -1};
        أرجع {0, 0};  // plain boolean -- 0 cycles
    }

    // Clone `operand` with its sole variable ranged cycle delay pinned so the
    // total match length is exactly `len`. `lo` is the operand's minimum length
    // (lengthRange().first). A fixed operand has no such delay and is returned
    // as a plain clone (callers only request its single achievable length).
    ساكن Ast_گرہ_اظہاریہ* realize_At_لمائی(Ast_گرہ_اظہاریہ* عملیہ, صحيح len, صحيح lo) {
        Ast_گرہ_اظہاریہ* ثابت clonep = عملیہ->clone_درخت_خالص(خطأ);
        AstDelay* rangeDelayp = خالی_سوچک;
        clonep->foreach([&](AstDelay* dp) {
            إذا (!rangeDelayp && dp->isRangeDelay() && !dp->isUnbounded()
                && getConstUInt(dp->lhsp()) != getConstUInt(dp->rhsp())) {
                rangeDelayp = dp;
            }
        });
        إذا (rangeDelayp) {
            File_سطر* ثابت flp = rangeDelayp->fileline();
            ثابت بنانشانی pinned = getConstUInt(rangeDelayp->lhsp()) + (len - lo);
            Ast_گرہ_اظہاریہ* ثابت oldMinp = rangeDelayp->lhsp();
            oldMinp->replaceWith(جديد AstConst{flp, pinned});
            VL_DO_DANGLING(oldMinp->delete_درخت(), oldMinp);
            // Drop the max bound so it lowers as a fixed `##d`, not `##[d:d]`.
            Ast_گرہ* ثابت oldMaxp = rangeDelayp->rhsp()->unlink_Fr_پٹھ();
            VL_DO_DANGLING(oldMaxp->delete_درخت(), oldMaxp);
        }
        أرجع clonep;
    }

    // Cuts AST size from O(N * sizeof(exprp)) to O(N) + O(sizeof(exprp)) by
    // sharing a single `VarRef` across N check edges. Hoist also matches the
    // IEEE 1800-2023 16.9.9 "single preponed-region snapshot" semantic for
    // any exprp -- even an impure one would now evaluate exactly once per
    // clock instead of N times. Orphan temps from failed builds are unused
    // MODULETEMPs and are removed by V3Dead.
    AstVar* tryHoistSampled(Ast_گرہ_اظہاریہ* exprp, File_سطر* flp, بنانشانی clone_گنتی) {
        constexpr بنانشانی k_Hoist_حد = 2;
        إذا (clone_گنتی < k_Hoist_حد) أرجع خالی_سوچک;
        AstVar* ثابت عارضی_Varp
            = جديد AstVar{flp, VVarType::MODULETEMP, m_prop_عارضی_ناں.get(exprp), exprp->dtypep()};
        m_modp->جوڑو_Stmtsp(عارضی_Varp);
        Ast_سونپو* ثابت assignp = جديد Ast_سونپو{flp, جديد AstVarRef{flp, عارضی_Varp, VAccess::WRITE},
                                                 sampled(exprp->clone_درخت_خالص(خطأ))};
        m_modp->جوڑو_Stmtsp(جديد AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, خالی_سوچک, assignp});
        أرجع عارضی_Varp;
    }

    ساكن Ast_گرہ_اظہاریہ* sampledRefOrClone(AstVar* hoistVarp, Ast_گرہ_اظہاریہ* exprp, File_سطر* flp) {
        إذا (hoistVarp) أرجع جديد AstVarRef{flp, hoistVarp, VAccess::READ};
        أرجع sampled(exprp->clone_درخت_خالص(خطأ));
    }

    // Create vertex and inherit temporal guards from the current scope.
    Sva_State_راس* scoped_Create_راس() {
        Sva_State_راس* ثابت vtxp = m_گراف.create_State_راس();
        لكل (Ast_گرہ_اظہاریہ* ثابت cp : m_temporal_Guard_ڈھیر) {
            vtxp->m_throughoutConds.ڈالو_پٹھ(cp->clone_درخت_خالص(خطأ));
        }
        إذا (m_وچ_Unbounded_Scope) vtxp->m_isUnbounded = سچ;
        إذا (m_markStrongPending) vtxp->m_strongPending = سچ;
        أرجع vtxp;
    }

    // AND current temporal guards into every edge/link.
    Sva_Trans_کنارا* guardedLink(Sva_State_راس* fromp, Sva_State_راس* چوٹی, Ast_گرہ_اظہاریہ* condp,
                              File_سطر* flp) {
        أرجع m_گراف.جوڑو_Link(fromp, چوٹی, throughoutCond(condp, flp));
    }
    Sva_Trans_کنارا* guardedLink(Sva_State_راس* fromp, Sva_State_راس* چوٹی, File_سطر* flp) {
        أرجع m_گراف.جوڑو_Link(fromp, چوٹی, throughoutCond(خالی_سوچک, flp));
    }
    Sva_Trans_کنارا* guarded_کنارا(Sva_State_راس* fromp, Sva_State_راس* چوٹی, Ast_گرہ_اظہاریہ* condp,
                              File_سطر* flp) {
        أرجع m_گراف.جوڑو_Clocked_کنارا(fromp, چوٹی, throughoutCond(condp, flp));
    }
    Sva_Trans_کنارا* guarded_کنارا(Sva_State_راس* fromp, Sva_State_راس* چوٹی, File_سطر* flp) {
        أرجع m_گراف.جوڑو_Clocked_کنارا(fromp, چوٹی, throughoutCond(خالی_سوچک, flp));
    }

    Sva_State_راس* جوڑو_Delay_Chain(Sva_State_راس* startp, بنانشانی ناپ, File_سطر* flp,
                                  منطقي isFixed = سچ, Ast_گرہ_اظہاریہ* clearCondp = خالی_سوچک,
                                  Ast_گرہ_اظہاریہ* advanceCondp = خالی_سوچک) {
        إذا (isFixed && ناپ == 0) أرجع startp;
        UASSERT_OBJ(ناپ > 0, startp, "Delay chain needs at least one slot");
        إذا (isFixed && ناپ == 1 && !advanceCondp) {
            Sva_State_راس* ثابت nextp = scoped_Create_راس();
            guarded_کنارا(startp, nextp, flp);
            أرجع nextp;
        }
        Sva_State_راس* ثابت ringVtxp = scoped_Create_راس();
        ringVtxp->m_isFixedDelayRing = isFixed;
        ringVtxp->m_delay_Ring_ناپ = ناپ;
        إذا (clearCondp) {
            UASSERT_OBJ(!isFixed, startp, "Fixed delay cannot have a clear condition");
            ringVtxp->m_delayRingClearCondp = clearCondp->clone_درخت_خالص(خطأ);
        }
        ringVtxp->m_delayRingAdvanceCondp = advanceCondp;
        إذا (isFixed) {
            guarded_کنارا(startp, ringVtxp, flp);
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
                         std::سمتیہ<Sva_State_راس*>& midSources,
                         RangeDelayRejectInfo* rangeRejectInfop = خالی_سوچک) {
        File_سطر* ثابت flp = sexprp->fileline();
        AstDelay* ثابت delayp = VN_AS(sexprp->delayp(), Delay);
        Ast_گرہ_اظہاریہ* ثابت rhsExprp = sexprp->exprp();
        ثابت بنانشانی minDelay = getConstUInt(delayp->lhsp());
        إذا (delayp->isUnbounded()) {
            // `##[M:$]`: wait M cycles, then retain every matured attempt in a
            // one-slot ring while also exposing the first eligible tick.
            currentp = جوڑو_Delay_Chain(currentp, minDelay, flp);
            Sva_State_راس* ثابت waitRingp = جوڑو_Delay_Chain(currentp, 1, flp, خطأ);
            // The ring's live count is updated in NBA, so this link retains the previous
            // tick's live attempts alongside new arrivals for the next tick.
            guardedLink(waitRingp, waitRingp, flp);
            Sva_State_راس* ثابت ملاؤ_Vtxp = scoped_Create_راس();
            guardedLink(currentp, ملاؤ_Vtxp, flp);
            guardedLink(waitRingp, ملاؤ_Vtxp, flp);
            currentp = ملاؤ_Vtxp;
            currentp->m_isUnbounded = سچ;  // Liveness, so no reject.
            m_وچ_Unbounded_Scope = سچ;
            أرجع سچ;
        }
        ثابت بنانشانی maxDelay = getConstUInt(delayp->rhsp());
        إذا (minDelay == maxDelay) {
            currentp = جوڑو_Delay_Chain(currentp, minDelay, flp);
            أرجع سچ;
        }
        ثابت بنانشانی range = maxDelay - minDelay;
        currentp = جوڑو_Delay_Chain(currentp, minDelay, flp);
        ثابت منطقي multi_چکر_Rhs = rhsExprp->is_Multi_چکر_Sva();
        ثابت Ast_گرہ_اظہاریہ* ثابت preExprp = sexprp->preExprp();
        // Prefix and nested-sequence composition can lose endpoints or their multiplicity
        // required by cover sequence (IEEE 1800-2023 16.14.3), regardless of the range width.
        إذا (m_isCoverSeq
            && (multi_چکر_Rhs || m_وچ_Sequence_اگیتر
                || (preExprp && preExprp->is_Multi_چکر_Sva()))) {
            warnEndpointUnsupported(flp, "this ranged cycle delay");
            أرجع خطأ;
        }
        إذا (m_وچ_Sequence_اگیتر) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: Bounded ranged cycle delay in a sequence prefix");
            أرجع خطأ;
        }
        إذا (multi_چکر_Rhs) {
            // Launch a candidate at every eligible tick. Candidate-local misses are not
            // assertion rejects while a later position can still match.
            إذا (rangeRejectInfop) {
                ثابت صحيح rhsLen = fixed_لمائی(rhsExprp);
                إذا (rhsLen >= 0) *rangeRejectInfop = {currentp, range, rhsLen};
            }
            Sva_State_راس* ثابت tailp = جوڑو_Delay_Chain(currentp, range + 1U, flp, خطأ);
            Sva_State_راس* ثابت ملاؤ_Vtxp = scoped_Create_راس();
            ملاؤ_Vtxp->m_isUnbounded = سچ;
            guardedLink(currentp, ملاؤ_Vtxp, flp);
            guardedLink(tailp, ملاؤ_Vtxp, flp);
            currentp = ملاؤ_Vtxp;
            m_وچ_Unbounded_Scope = سچ;
        } وإلا {
            // The first eligible tick is explicit. The ring retains attempts for the
            // remaining ticks and exposes the final tick for rejection. Only properties
            // clear on success, since cover sequence counts every endpoint.
            midSources.ڈالو_پٹھ(currentp);
            currentp = جوڑو_Delay_Chain(currentp, range + 1U, flp, خطأ,
                                     m_isCoverSeq ? خالی_سوچک : rhsExprp);
        }
        أرجع سچ;
    }

    فراغ جوڑو_Finite_Range_Reject(ثابت RangeDelayRejectInfo& info, ثابت Build_نتیجہ& نتیجہ,
                              File_سطر* flp) {
        إذا (!info.startp) أرجع;

        Sva_State_راس* ثابت expiryVtxp
            = جوڑو_Delay_Chain(info.startp, info.range + info.rhsLen, flp);
        Sva_State_راس* ثابت expiryMatchp = scoped_Create_راس();
        std::سمتیہ<Sva_State_راس*> sources = نتیجہ.midSources;
        sources.ڈالو_پٹھ(نتیجہ.رکن_Vertexp);
        لكل (Sva_State_راس* ثابت srcp : sources) {
            Ast_گرہ_اظہاریہ* ثابت condp
                = نتیجہ.finalCondp ? sampled(نتیجہ.finalCondp->clone_درخت_خالص(خطأ)) : خالی_سوچک;
            Sva_State_راس* ثابت successNowp = scoped_Create_راس();
            guardedLink(srcp, successNowp, condp, flp);
            // Retain a success through the latest deadline it can satisfy.
            Sva_State_راس* ثابت historyp
                = جوڑو_Delay_Chain(successNowp, info.range + 1U, flp, خطأ);
            guardedLink(successNowp, expiryMatchp, flp);
            guardedLink(historyp, expiryMatchp, flp);
        }

        Sva_State_راس* ثابت sinkVtxp = m_گراف.create_State_راس();
        sinkVtxp->m_isRejectSink = سچ;
        Sva_Trans_کنارا* ثابت rejectp = m_گراف.جوڑو_Link(expiryVtxp, sinkVtxp);
        rejectp->m_rejectOnFail = سچ;
        rejectp->m_condVtxp = expiryMatchp;
    }

    Build_نتیجہ buildSExpr(AstSExpr* sexprp, Sva_State_راس* اندراج_Vtxp,
                           منطقي is_چوٹی_Level_قدم = خطأ) {
        AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
        إذا (!delayp || !delayp->is_چکر_Delay()) أرجع Build_نتیجہ::fail();

        File_سطر* ثابت flp = sexprp->fileline();
        Ast_گرہ_اظہاریہ* ثابت exprp = sexprp->exprp();

        // Handle LHS (preExpr)
        Sva_State_راس* currentp = اندراج_Vtxp;
        إذا (Ast_گرہ_اظہاریہ* ثابت preExprp = sexprp->preExprp()) {
            VL_RESTORER(m_وچ_Sequence_اگیتر);
            m_وچ_Sequence_اگیتر = سچ;
            ثابت Build_نتیجہ pre = build_اظہاریہ(preExprp, currentp, is_چوٹی_Level_قدم);
            إذا (!pre.valid()) أرجع Build_نتیجہ::fail(pre.errorEmitted);  // LCOV_EXCL_LINE
            إذا (pre.finalCondp) {
                Sva_State_راس* ثابت condVtxp = scoped_Create_راس();
                Sva_Trans_کنارا* ثابت edgep = guardedLink(
                    pre.رکن_Vertexp, condVtxp, sampled(pre.finalCondp->clone_درخت_خالص(خطأ)), flp);
                إذا (is_چوٹی_Level_قدم && !pre.رکن_Vertexp->m_isUnbounded && !m_وچ_Unbounded_Scope) {
                    // Do not mark liveness sources: first boolean check is deferred.
                    edgep->m_rejectOnFail = سچ;
                }
                freeUnlinkedCondp(pre.finalCondp);
                currentp = condVtxp;
            } وإلا {
                currentp = pre.رکن_Vertexp;
            }
        }

        // Handle delay
        std::سمتیہ<Sva_State_راس*> rangeMidSources;
        RangeDelayRejectInfo rangeRejectInfo;
        ثابت منطقي جوڑو_Range_Reject = is_چوٹی_Level_قدم && !m_وچ_Unbounded_Scope;
        إذا (delayp->isRangeDelay()) {
            إذا (!applyRangeDelay(sexprp, currentp, rangeMidSources,
                                 جوڑو_Range_Reject ? &rangeRejectInfo : خالی_سوچک)) {
                أرجع Build_نتیجہ::failWithError();
            }
        } وإلا {
            ثابت بنانشانی delay_چکراں = getConstUInt(delayp->lhsp());
            currentp = جوڑو_Delay_Chain(currentp, delay_چکراں, flp);
        }

        // Multi-cycle RHS: recurse (only plain boolean is returned as finalCondp).
        إذا (exprp->is_Multi_چکر_Sva()) {
            ثابت Build_نتیجہ نتیجہ = build_اظہاریہ(exprp, currentp, is_چوٹی_Level_قدم);
            إذا (نتیجہ.valid()) جوڑو_Finite_Range_Reject(rangeRejectInfo, نتیجہ, flp);
            أرجع نتیجہ;
        }
        أرجع {currentp, exprp, std::move(rangeMidSources)};
    }

    Build_نتیجہ buildConsRep(AstSConsRep* repp, Sva_State_راس* اندراج_Vtxp,
                             منطقي is_چوٹی_Level_قدم = خطأ) {
        File_سطر* ثابت flp = repp->fileline();
        Ast_گرہ_اظہاریہ* ثابت exprp = repp->exprp();
        // Multi-cycle expr in ConsRep not yet supported; bail to avoid invalid AST.
        إذا (exprp->is_Multi_چکر_Sva()) {
            repp->v3warn(E_UNSUPPORTED, "Unsupported: multi-cycle sequence expression inside"
                                        " consecutive repetition (IEEE 1800-2023 16.9.2)");
            أرجع Build_نتیجہ::failWithError();
        }
        ثابت بنانشانی minN = getConstUInt(repp->countp());

        // Sum sites across prefix + unbounded/range tail so one hoist covers
        // every check edge of this repetition.
        بنانشانی کل_Sites = minN;
        إذا (repp->unbounded()) {
            کل_Sites += 1;
        } وإلا إذا (repp->maxCountp()) {
            کل_Sites += getConstUInt(repp->maxCountp()) - minN;
        }
        AstVar* ثابت hoistVarp = tryHoistSampled(exprp, flp, کل_Sites);

        // Cover-sequence (IEEE 1800-2023 16.14.3): collect each end-of-match
        // position so they all fire the action, not just the merged terminal.
        std::سمتیہ<Sva_State_راس*> consMidSources;

        Sva_State_راس* currentp = اندراج_Vtxp;
        لكل (بنانشانی i = 0; i < minN; ++i) {
            // Keep the first repetition explicit, collapse all remaining checks into the ring.
            إذا (i == 1) {
                currentp = جوڑو_Delay_Chain(currentp, minN - 1, flp);
                currentp->m_delayRingClearCondp
                    = جديد AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                currentp->m_replayAbortReject = سچ;
                إذا (is_چوٹی_Level_قدم) {
                    currentp->m_throughoutConds.ڈالو_پٹھ(
                        sampledRefOrClone(hoistVarp, exprp, flp));
                }
                i = minN - 1;
            }
            // Every repetition in the minimum prefix is required.
            Sva_State_راس* ثابت condVtxp = scoped_Create_راس();
            Sva_Trans_کنارا* ثابت linkp
                = guardedLink(currentp, condVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            // Only an outermost required step rejects the property. Its first check is explicit,
            // later required checks reject through the ring's throughout guard.
            إذا (is_چوٹی_Level_قدم && i == 0) linkp->m_rejectOnFail = سچ;
            currentp = condVtxp;
        }
        // After minN: currentp is the first valid end-of-match position for [*m:n].
        إذا (m_isCoverSeq && (repp->unbounded() || repp->maxCountp())) {
            consMidSources.ڈالو_پٹھ(currentp);
        }

        إذا (repp->unbounded()) {
            إذا (minN == 0) {
                Sva_State_راس* ثابت waitVtxp = scoped_Create_راس();
                guarded_کنارا(currentp, waitVtxp, flp);
                Sva_State_راس* ثابت checkVtxp = scoped_Create_راس();
                guardedLink(waitVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guarded_کنارا(checkVtxp, waitVtxp, flp);
                guardedLink(currentp, checkVtxp, flp);
                currentp = checkVtxp;
            } وإلا {
                Sva_State_راس* ثابت loop_پٹھ_Vtxp = scoped_Create_راس();
                guarded_کنارا(currentp, loop_پٹھ_Vtxp, flp);
                Sva_State_راس* ثابت reCheckVtxp = scoped_Create_راس();
                guardedLink(loop_پٹھ_Vtxp, reCheckVtxp, sampledRefOrClone(hoistVarp, exprp, flp),
                            flp);
                guarded_کنارا(reCheckVtxp, loop_پٹھ_Vtxp, flp);
                guardedLink(reCheckVtxp, currentp, flp);
            }
            currentp->m_isUnbounded = سچ;
            m_وچ_Unbounded_Scope = سچ;
        } وإلا إذا (repp->maxCountp()) {
            ثابت بنانشانی maxN = getConstUInt(repp->maxCountp());
            Sva_State_راس* ثابت ملاؤ_Vtxp = scoped_Create_راس();
            guardedLink(currentp, ملاؤ_Vtxp, flp);
            بنانشانی پوچھ_Min_N = minN;
            Sva_State_راس* پوچھ_Startp = currentp;
            إذا (minN == 0) {
                // Build the first optional iteration explicitly. Feeding the empty endpoint
                // directly into a range ring would incorrectly keep that match alive.
                Sva_State_راس* ثابت اگلا_Vtxp = scoped_Create_راس();
                guarded_کنارا(currentp, اگلا_Vtxp, flp);
                Sva_State_راس* ثابت checkVtxp = scoped_Create_راس();
                guardedLink(اگلا_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, ملاؤ_Vtxp, flp);
                إذا (m_isCoverSeq) consMidSources.ڈالو_پٹھ(checkVtxp);
                پوچھ_Startp = checkVtxp;
                پوچھ_Min_N = 1;
            }
            إذا (maxN > پوچھ_Min_N) {
                // Add tail-ring only if the tail is non-empty.
                Sva_State_راس* ثابت اگلا_Vtxp
                    = جوڑو_Delay_Chain(پوچھ_Startp, maxN - پوچھ_Min_N + 1, flp, خطأ);
                اگلا_Vtxp->m_delayRingClearCondp
                    = جديد AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                اگلا_Vtxp->m_replayAbortReject = سچ;
                Sva_State_راس* ثابت checkVtxp = scoped_Create_راس();
                guardedLink(اگلا_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, ملاؤ_Vtxp, flp);
                إذا (m_isCoverSeq) consMidSources.ڈالو_پٹھ(checkVtxp);
            }
            currentp = ملاؤ_Vtxp;
        }
        // finalCond = nullptr (already checked via Links)
        Build_نتیجہ res;
        res.رکن_Vertexp = currentp;
        res.finalCondp = خالی_سوچک;
        res.midSources = std::move(consMidSources);
        // mergeVtxp is the OR of all the end-positions we already pushed to
        // midSources, so the main termVtxp -> matchVertex Link would duplicate.
        res.رکن_Is_Mid_ملاؤ = m_isCoverSeq && !res.midSources.خالی_تھاواں();
        أرجع res;
    }

    // always[lo:hi] / s_always[lo:hi] (IEEE 1800-2023 16.12.11).
    Build_نتیجہ buildPropAlways(AstPropAlways* nodep, Sva_State_راس* اندراج_Vtxp,
                                منطقي is_چوٹی_Level_قدم = خطأ) {
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت propp = nodep->propp();
        ثابت بنانشانی lo = getConstUInt(nodep->loBoundp());
        إذا (VN_IS(nodep->hiBoundp(), Unbounded)) {
            // Weak always [lo:$]: unbounded upper bound (IEEE 1800-2023 16.12.11).
            // p must hold at every clock tick at least lo cycles after the attempt
            // start; those ticks are not required to exist, so there is no
            // end-of-trace obligation (weak). The self-loop keeps the attempt live
            // every cycle; each observed cycle is a safety obligation, so a false p
            // rejects immediately.
            UASSERT_OBJ(!nodep->isStrong(), nodep, "Unbounded always must be weak (V3Width)");
            Sva_State_راس* ثابت livep = جوڑو_Delay_Chain(اندراج_Vtxp, lo, flp);
            livep->m_isUnbounded = سچ;
            guarded_کنارا(livep, livep, flp);  // stay active every subsequent cycle
            Sva_State_راس* ثابت sinkp = m_گراف.create_State_راس();
            sinkp->m_isRejectSink = سچ;
            Sva_Trans_کنارا* ثابت rejEdgep
                = guardedLink(livep, sinkp, sampled(propp->clone_درخت_خالص(خطأ)), flp);
            إذا (is_چوٹی_Level_قدم) rejEdgep->m_rejectOnFail = سچ;
            أرجع {livep, خالی_سوچک, {}};
        }
        ثابت بنانشانی hi = getConstUInt(nodep->hiBoundp());
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
        Sva_State_راس* currentp = جوڑو_Delay_Chain(اندراج_Vtxp, lo, flp);
        Sva_State_راس* ثابت checkp = scoped_Create_راس();
        Sva_Trans_کنارا* ثابت linkp
            = guardedLink(currentp, checkp, sampled(propp->clone_درخت_خالص(خطأ)), flp);
        إذا (is_چوٹی_Level_قدم) linkp->m_rejectOnFail = سچ;
        currentp = checkp;
        m_temporal_Guard_ڈھیر.ڈالو_پٹھ(propp);
        currentp = جوڑو_Delay_Chain(currentp, hi - lo, flp);
        إذا (nodep->isStrong() && currentp->m_delay_Ring_ناپ) currentp->m_strongAlwaysRing = سچ;
        m_temporal_Guard_ڈھیر.نکالو_پٹھ();
        أرجع {currentp, propp, {}};
    }

    Build_نتیجہ buildGotoRep(AstSGotoRep* repp, Sva_State_راس* اندراج_Vtxp) {
        File_سطر* ثابت flp = repp->fileline();
        Ast_گرہ_اظہاریہ* ثابت exprp = repp->exprp();
        ثابت بنانشانی minN = getConstUInt(repp->countp());
        إذا (minN == 0) أرجع Build_نتیجہ::fail();
        ثابت منطقي hasMax = repp->maxCountp() != خالی_سوچک;
        ثابت بنانشانی maxN = hasMax ? getConstUInt(repp->maxCountp()) : minN;
        إذا (m_isCoverSeq) {
            // Several attempts may coalesce in the one-bit wait vertex before reaching the ring,
            // so a cover sequence action block could run too few times.
            warnEndpointUnsupported(flp, "a goto repetition");
            أرجع Build_نتیجہ::failWithError();
        }

        AstVar* ثابت hoistVarp = tryHoistSampled(exprp, flp, 2);

        // The first guardedEdge is the ##1 before waiting for a match. In the wait state, false
        // takes the clocked self-loop, while true takes the zero-delay guardedLink on that tick.
        Sva_State_راس* ثابت waitVtxp = scoped_Create_راس();
        guarded_کنارا(اندراج_Vtxp, waitVtxp, flp);
        guarded_کنارا(waitVtxp, waitVtxp,
                    جديد AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)}, flp);
        Sva_State_راس* currentp = scoped_Create_راس();
        guardedLink(waitVtxp, currentp, sampledRefOrClone(hoistVarp, exprp, flp), flp);

        إذا (minN > 1) {
            Sva_State_راس* ثابت ringVtxp = جوڑو_Delay_Chain(
                currentp, minN - 1, flp, سچ, خالی_سوچک, sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = سچ;
            Sva_State_راس* ثابت checkVtxp = scoped_Create_راس();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            currentp = checkVtxp;
        }
        إذا (!hasMax) {
            currentp->m_isUnbounded = سچ;  // [->N] waits unboundedly
            m_وچ_Unbounded_Scope = سچ;
            أرجع {currentp, خالی_سوچک, {}};
        }

        // [->M:N]: the range ring holds matches from M through N and advances
        // only on expr, preserving arbitrarily long gaps between occurrences.
        Sva_State_راس* ثابت ملاؤ_Vtxp = scoped_Create_راس();
        guardedLink(currentp, ملاؤ_Vtxp, flp);  // accept at match_M
        إذا (maxN > minN) {
            Sva_State_راس* ثابت ringVtxp
                = جوڑو_Delay_Chain(currentp, maxN - minN + 1, flp, خطأ, خالی_سوچک,
                                sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = سچ;
            Sva_State_راس* ثابت checkVtxp = scoped_Create_راس();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            guardedLink(checkVtxp, ملاؤ_Vtxp, flp);
        }
        ملاؤ_Vtxp->m_isUnbounded = سچ;  // [->M:N] still has unbounded waits between matches
        m_وچ_Unbounded_Scope = سچ;
        أرجع {ملاؤ_Vtxp, خالی_سوچک, {}};
    }

    // True if the sequence has a zero-minimum consecutive repetition, which may match empty
    ساكن منطقي hasZeroMinRep(ثابت Ast_گرہ_اظہاریہ* seqp) {
        أرجع seqp->exists(
            [](ثابت AstSConsRep* repp) { أرجع getConstUInt(repp->countp()) == 0; });
    }

    // True if a leaf's condition has no implication or multi-cycle sequence in it
    // (a leaf can be e.g. 'not (x |-> y)')
    ساكن منطقي isPlainBoolean(ثابت Ast_گرہ_اظہاریہ* condp) {
        أرجع !condp->exists(
            [](ثابت Ast_گرہ_اظہاریہ* np) { أرجع np->is_Multi_چکر_Sva() || VN_IS(np, Implication); });
    }

    // Build merge vertex for SOr / LogOr: both branches feed into one vertex.
    Build_نتیجہ build_Or_ملاؤ(Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp, Sva_State_راس* اندراج_Vtxp,
                             File_سطر* flp, منطقي want_Bool_پتی) {
        ثابت Build_نتیجہ lhs = build_اظہاریہ(lhsp, اندراج_Vtxp);
        ثابت Build_نتیجہ rhs = build_اظہاریہ(rhsp, اندراج_Vtxp);
        إذا (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع Build_نتیجہ::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP
        // IEEE 1800-2023 16.14.3: a cover sequence counts every end-of-match. A
        // sequence operand of 'or' can end more than once, but only its final
        // end reaches the merge vertex below, so reject sequence operands rather
        // than under-count. Plain boolean disjunction has one end per cycle and
        // is handled by the OR-fold.
        ثابت منطقي booleanOnly = lhs.رکن_Vertexp == اندراج_Vtxp && lhs.finalCondp
                                 && rhs.رکن_Vertexp == اندراج_Vtxp && rhs.finalCondp;
        إذا (m_isCoverSeq && !booleanOnly) {
            warnEndpointUnsupported(flp, "a sequence operand of 'or'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع Build_نتیجہ::failWithError();
        }
        إذا (booleanOnly && want_Bool_پتی) {
            إذا (isPlainBoolean(lhs.finalCondp) && isPlainBoolean(rhs.finalCondp)) {
                // As a |-> / |=> or top-level body, a disjunction of two plain booleans must
                // be able to reject: return it as a boolean finalCondp. The merge vertex
                // below drops the attempt silently when both operands are false.
                Ast_گرہ_اظہاریہ* ثابت orCondp
                    = جديد AstLogOr{flp, lhs.finalCondp->clone_درخت_خالص(خطأ),
                                   rhs.finalCondp->clone_درخت_خالص(خطأ)};
                freeUnlinkedCondp(lhs.finalCondp);
                freeUnlinkedCondp(rhs.finalCondp);
                أرجع {اندراج_Vtxp, orCondp, {}};
            }
        }
        Sva_State_راس* ثابت ملاؤ_Vtxp = scoped_Create_راس();
        إذا (booleanOnly) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsp,
                        "Single-cycle SOr operands must have finalCondp");
            guardedLink(اندراج_Vtxp, ملاؤ_Vtxp,
                        جديد AstLogOr{flp, sampled(lhs.finalCondp->clone_درخت_خالص(خطأ)),
                                     sampled(rhs.finalCondp->clone_درخت_خالص(خطأ))},
                        flp);
        } وإلا {
            إذا (lhs.finalCondp) {
                guardedLink(lhs.رکن_Vertexp, ملاؤ_Vtxp,
                            sampled(lhs.finalCondp->clone_درخت_خالص(خطأ)), flp);
            } وإلا {
                guardedLink(lhs.رکن_Vertexp, ملاؤ_Vtxp, flp);
            }
            إذا (rhs.finalCondp) {
                guardedLink(rhs.رکن_Vertexp, ملاؤ_Vtxp,
                            sampled(rhs.finalCondp->clone_درخت_خالص(خطأ)), flp);
            } وإلا {
                guardedLink(rhs.رکن_Vertexp, ملاؤ_Vtxp, flp);
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        أرجع {ملاؤ_Vtxp, خالی_سوچک, {}};
    }

    // Build done-latch combiner for SAnd/SIntersect (IEEE 1800-2023 16.9.5).
    Build_نتیجہ buildAndCombiner(Ast_گرہ_اظہاریہ* lhsExprp, Ast_گرہ_اظہاریہ* rhsExprp,
                                 Sva_State_راس* اندراج_Vtxp, File_سطر* flp) {
        // Snapshot-restore scope so LHS liveness does not leak into RHS.
        ثابت منطقي savedScope = m_وچ_Unbounded_Scope;
        ثابت Build_نتیجہ lhs = build_اظہاریہ(lhsExprp, اندراج_Vtxp);
        ثابت منطقي lhsScope = m_وچ_Unbounded_Scope;
        m_وچ_Unbounded_Scope = savedScope;
        ثابت Build_نتیجہ rhs = build_اظہاریہ(rhsExprp, اندراج_Vtxp);
        ثابت منطقي rhsScope = m_وچ_Unbounded_Scope;
        m_وچ_Unbounded_Scope = savedScope || lhsScope || rhsScope;
        إذا (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع Build_نتیجہ::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP

        // Single-cycle operands: use boolean AND (done-latch would fire across cycles).
        // If both operands stayed at entry, they must be boolean leaves which
        // buildExpr returns with finalCondp=nodep (non-null).
        إذا (lhs.رکن_Vertexp == اندراج_Vtxp && rhs.رکن_Vertexp == اندراج_Vtxp) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsExprp,
                        "Single-cycle SAnd operands must have finalCondp");
            Ast_گرہ_اظہاریہ* ثابت condp = جديد AstLogAnd{flp, lhs.finalCondp->clone_درخت_خالص(خطأ),
                                                     rhs.finalCondp->clone_درخت_خالص(خطأ)};
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع {اندراج_Vtxp, condp, {}};
        }
        // Range-delay mid-window sources in either sub-branch would need
        // to be folded into the latch's match-now signal, which the
        // current combiner does not support.
        إذا (!lhs.midSources.خالی_تھاواں() || !rhs.midSources.خالی_تھاواں()) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: ranged cycle delay in an operand of property 'and'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع Build_نتیجہ::failWithError();
        }
        Sva_State_راس* ثابت combVtxp = scoped_Create_راس();
        combVtxp->m_isAndCombiner = سچ;
        combVtxp->m_andLhsTermp = lhs.رکن_Vertexp;
        combVtxp->m_andRhsTermp = rhs.رکن_Vertexp;
        إذا (lhs.finalCondp) combVtxp->m_andLhsCondp = lhs.finalCondp->clone_درخت_خالص(خطأ);
        إذا (rhs.finalCondp) combVtxp->m_andRhsCondp = rhs.finalCondp->clone_درخت_خالص(خطأ);
        إذا (lhs.رکن_Vertexp->m_isUnbounded || rhs.رکن_Vertexp->m_isUnbounded) {
            combVtxp->m_isUnbounded = سچ;
        }
        // Wire terminal-boolean rejects to a dedicated sink so each side can fail
        // the AND independently. Skip for liveness or single-cycle operands
        // (single-cycle termVertexp == entryVtxp would fire every cycle).
        إذا (!combVtxp->m_isUnbounded) {
            منطقي needSink = خطأ;
            ثابت منطقي lhs_Multi_چکر = (lhs.رکن_Vertexp != اندراج_Vtxp);
            ثابت منطقي rhs_Multi_چکر = (rhs.رکن_Vertexp != اندراج_Vtxp);
            إذا (lhs.finalCondp && lhs_Multi_چکر && !lhs.رکن_Vertexp->m_isUnbounded) {
                needSink = سچ;
            }
            إذا (rhs.finalCondp && rhs_Multi_چکر && !rhs.رکن_Vertexp->m_isUnbounded) {
                needSink = سچ;
            }
            إذا (needSink) {
                Sva_State_راس* ثابت sinkVtxp = m_گراف.create_State_راس();
                sinkVtxp->m_isRejectSink = سچ;
                إذا (lhs.finalCondp && lhs_Multi_چکر && !lhs.رکن_Vertexp->m_isUnbounded) {
                    Sva_Trans_کنارا* ثابت ep = m_گراف.جوڑو_Link(
                        lhs.رکن_Vertexp, sinkVtxp, sampled(lhs.finalCondp->clone_درخت_خالص(خطأ)));
                    ep->m_rejectOnFail = سچ;
                }
                إذا (rhs.finalCondp && rhs_Multi_چکر && !rhs.رکن_Vertexp->m_isUnbounded) {
                    Sva_Trans_کنارا* ثابت ep = m_گراف.جوڑو_Link(
                        rhs.رکن_Vertexp, sinkVtxp, sampled(rhs.finalCondp->clone_درخت_خالص(خطأ)));
                    ep->m_rejectOnFail = سچ;
                }
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        أرجع {combVtxp, خالی_سوچک, {}};
    }

    // Lower `seq1 within seq2` (IEEE 1800-2023 16.9.10) as:
    //   (OR_{i in 0..slack} 1 ##i seq1 ##(slack-i) 1) intersect seq2
    // Both operands must have fixed length (no ranged cycle delays).
    // The OR lives inside a single SIntersect so one AndCombiner done-latch
    // serves all offsets; lifting it out would double-count matches where
    // multiple offsets accept on the same seq2 end cycle.
    Build_نتیجہ buildSWithin(AstSWithin* nodep, Sva_State_راس* اندراج_Vtxp,
                             منطقي is_چوٹی_Level_قدم = خطأ) {
        ثابت صحيح innerLen = fixed_لمائی(nodep->lhsp());
        ثابت صحيح outerLen = fixed_لمائی(nodep->rhsp());
        إذا (innerLen < 0 || outerLen < 0) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: within with ranged cycle-delay operand");
            أرجع Build_نتیجہ::failWithError();
        }
        إذا (innerLen > outerLen) {
            أرجع build_Never_میل_Intersect(
                nodep, اندراج_Vtxp, is_چوٹی_Level_قدم,
                "the inner sequence is longer than the outer sequence");
        }
        File_سطر* ثابت flp = nodep->fileline();
        ثابت صحيح slack = outerLen - innerLen;
        Ast_گرہ_اظہاریہ* innerOrp = خالی_سوچک;
        لكل (صحيح i = 0; i <= slack; ++i) {
            ثابت صحيح postPad = slack - i;
            Ast_گرہ_اظہاریہ* branchp = nodep->lhsp()->clone_درخت_خالص(خطأ);
            إذا (i > 0) {
                AstConst* ثابت prePadp = جديد AstConst{flp, AstConst::ثنائی_ہندسہ_True{}};
                AstDelay* ثابت delayp
                    = جديد AstDelay{flp, جديد AstConst{flp, static_cast<uint32_t>(i)}, سچ};
                AstSExpr* ثابت wrapped = جديد AstSExpr{flp, prePadp, delayp, branchp};
                wrapped->dtype_سیٹ_ثنائی_ہندسہ();
                branchp = wrapped;
            }
            إذا (postPad > 0) {
                AstConst* ثابت postTruep = جديد AstConst{flp, AstConst::ثنائی_ہندسہ_True{}};
                AstDelay* ثابت delayp
                    = جديد AstDelay{flp, جديد AstConst{flp, static_cast<uint32_t>(postPad)}, سچ};
                AstSExpr* ثابت wrapped = جديد AstSExpr{flp, branchp, delayp, postTruep};
                wrapped->dtype_سیٹ_ثنائی_ہندسہ();
                branchp = wrapped;
            }
            innerOrp = innerOrp ? static_cast<Ast_گرہ_اظہاریہ*>(جديد AstSOr{flp, innerOrp, branchp})
                                : branchp;
        }
        Ast_گرہ_اظہاریہ* ثابت outerClonep = nodep->rhsp()->clone_درخت_خالص(خطأ);
        Ast_گرہ_اظہاریہ* ثابت combinedp = جديد AstSIntersect{flp, innerOrp, outerClonep};
        Build_نتیجہ نتیجہ = build_اظہاریہ(combinedp, اندراج_Vtxp, is_چوٹی_Level_قدم);
        VL_DO_DANGLING(combinedp->delete_درخت(), combinedp);
        // When both operands are plain booleans, buildAndCombiner returns a
        // freshly-allocated AstAnd as finalCondp with no parent. Callers up
        // the chain clone-and-discard finalCondp, so leave it parent-less and
        // it leaks; anchor it in the graph now via an edge.
        إذا (نتیجہ.valid() && نتیجہ.finalCondp && !نتیجہ.finalCondp->backp()) {
            Sva_State_راس* ثابت wrapVtxp = scoped_Create_راس();
            guardedLink(نتیجہ.رکن_Vertexp, wrapVtxp, sampled(نتیجہ.finalCondp), flp);
            نتیجہ = {wrapVtxp, خالی_سوچک, نتیجہ.midSources};
        }
        أرجع نتیجہ;
    }

    // Collect the boolean leaf checks of a fixed-length sequence keyed by their
    // clock offset from the start. Returns false for anything other than nested
    // AstSExpr with fixed cycle delays over boolean leaves (e.g. throughout).
    ساكن منطقي flattenFixedSeq(Ast_گرہ_اظہاریہ* nodep, صحيح ادھار_وتھ,
                                std::map<صحيح, std::سمتیہ<Ast_گرہ_اظہاریہ*>>& باہرلا) {
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
            إذا (!delayp || !delayp->is_چکر_Delay() || delayp->isUnbounded()) أرجع خطأ;
            ثابت بنانشانی delay_چکراں = getConstUInt(delayp->lhsp());
            إذا (delayp->isRangeDelay() && getConstUInt(delayp->rhsp()) != delay_چکراں)
                أرجع خطأ;
            صحيح preLen = 0;
            إذا (Ast_گرہ_اظہاریہ* ثابت prep = sexprp->preExprp()) {
                إذا (!flattenFixedSeq(prep, ادھار_وتھ, باہرلا)) أرجع خطأ;
                preLen = fixed_لمائی(prep);
                إذا (preLen < 0) أرجع خطأ;
            }
            أرجع flattenFixedSeq(sexprp->exprp(), ادھار_وتھ + preLen + delay_چکراں, باہرلا);
        }
        إذا (nodep->is_Multi_چکر_Sva()) أرجع خطأ;
        باہرلا[ادھار_وتھ].ڈالو_پٹھ(nodep);
        أرجع سچ;
    }

    // Conjoin two equal-length fixed sequences into one: at each clock offset
    // AND the boolean checks of both operands (IEEE 1800-2023 16.9.6 -- both
    // operands match the same window). Returns null if either operand is not a
    // plain fixed sequence of boolean leaves.
    ساكن Ast_گرہ_اظہاریہ* conjoinFixedSeqs(Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp, File_سطر* flp) {
        std::map<صحيح, std::سمتیہ<Ast_گرہ_اظہاریہ*>> checks;
        إذا (!flattenFixedSeq(lhsp, 0, checks) || !flattenFixedSeq(rhsp, 0, checks)) أرجع خالی_سوچک;
        إذا (checks.خالی_تھاواں()) أرجع خالی_سوچک;
        Ast_گرہ_اظہاریہ* resultp = خالی_سوچک;
        صحيح prev_وتھ = 0;
        لكل (ثابت خودکار& وتھ_Checks : checks) {
            ثابت صحيح وتھ = وتھ_Checks.پہلا;
            Ast_گرہ_اظہاریہ* condp = خالی_سوچک;
            لكل (Ast_گرہ_اظہاریہ* ثابت leafp : وتھ_Checks.دوجا) {
                Ast_گرہ_اظہاریہ* ثابت clonep = leafp->clone_درخت_خالص(خطأ);
                إذا (!condp) {
                    condp = clonep;
                } وإلا {
                    condp = جديد AstLogAnd{flp, condp, clonep};
                    condp->dtype_سیٹ_ثنائی_ہندسہ();
                }
            }
            إذا (!resultp) {
                إذا (وتھ > 0) {
                    AstDelay* ثابت delayp = جديد AstDelay{
                        flp, جديد AstConst{flp, static_cast<uint32_t>(وتھ)}, /*isCycle=*/سچ};
                    resultp
                        = جديد AstSExpr{flp, جديد AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}, delayp, condp};
                    resultp->dtype_سیٹ_ثنائی_ہندسہ();
                } وإلا {
                    resultp = condp;
                }
            } وإلا {
                AstDelay* ثابت delayp = جديد AstDelay{
                    flp, جديد AstConst{flp, static_cast<uint32_t>(وتھ - prev_وتھ)},
                    /*isCycle=*/سچ};
                resultp = جديد AstSExpr{flp, resultp, delayp, condp};
                resultp->dtype_سیٹ_ثنائی_ہندسہ();
            }
            prev_وتھ = وتھ;
        }
        أرجع resultp;
    }

    // `seq` is a simple ranged sequence `start ##[m:n] end` (start/end boolean,
    // start may be absent). Used to collapse a both-variable intersect to one
    // ranged delay.
    بنية SimpleRanged final {
        منطقي ok = خطأ;
        Ast_گرہ_اظہاریہ* startp = خالی_سوچک;  // may be null (absent start)
        Ast_گرہ_اظہاریہ* endp = خالی_سوچک;
    };
    ساكن SimpleRanged asSimpleRanged(Ast_گرہ_اظہاریہ* nodep) {
        AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr);
        إذا (!sexprp) أرجع {};
        AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
        إذا (!delayp || !delayp->is_چکر_Delay() || !delayp->isRangeDelay() || delayp->isUnbounded())
            أرجع {};
        إذا (getConstUInt(delayp->lhsp()) == getConstUInt(delayp->rhsp())) أرجع {};
        Ast_گرہ_اظہاریہ* ثابت prep = sexprp->preExprp();
        إذا (prep && fixed_لمائی(prep) != 0) أرجع {};
        إذا (fixed_لمائی(sexprp->exprp()) != 0) أرجع {};
        أرجع {سچ, prep, sexprp->exprp()};
    }

    // Build the NFA for a synthesized intersect lowering tree, then free it.
    // buildExpr returns the terminal condition (finalCondp) by reference into the
    // tree; detach a clone so the tree can be freed here. The graph already holds
    // clones/hoists of every edge condition, so nothing else dangles.
    Build_نتیجہ build_From_Lowering_درخت(Ast_گرہ_اظہاریہ* treep, Sva_State_راس* اندراج_Vtxp,
                                      منطقي is_چوٹی_Level_قدم) {
        Build_نتیجہ نتیجہ = build_اظہاریہ(treep, اندراج_Vtxp, is_چوٹی_Level_قدم);
        إذا (نتیجہ.valid() && نتیجہ.finalCondp) {
            نتیجہ.finalCondp = نتیجہ.finalCondp->clone_درخت_خالص(خطأ);
        }
        VL_DO_DANGLING(treep->delete_درخت(), treep);
        أرجع نتیجہ;
    }

    // Empty common-length intersection -- unequal fixed lengths, or disjoint
    // ranged lengths. IEEE 1800-2023 16.9.6 requires both operands to match
    // over a window of the same length, so with no common length the intersect
    // simply never matches. This is legal (matching nothing), not an error, so
    // lower to a constant false rather than rejecting legal code.
    Build_نتیجہ build_Never_میل_Intersect(Ast_گرہ_اظہاریہ* nodep, Sva_State_راس* اندراج_Vtxp,
                                         منطقي is_چوٹی_Level_قدم, ثابت حرف* reason) {
        nodep->v3warn(NEVERMATCH, "Sequence can never match because " << reason << ".");
        Ast_گرہ_اظہاریہ* ثابت falsep = جديد AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}};
        أرجع build_From_Lowering_درخت(falsep, اندراج_Vtxp, is_چوٹی_Level_قدم);
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
    Build_نتیجہ buildVarLenIntersect(AstSIntersect* nodep, Sva_State_راس* اندراج_Vtxp,
                                     منطقي is_چوٹی_Level_قدم) {
        ثابت std::pair<صحيح, صحيح> lhsRange = لمائی_Range(nodep->lhsp());
        ثابت std::pair<صحيح, صحيح> rhsRange = لمائی_Range(nodep->rhsp());
        إذا (lhsRange.پہلا < 0 || rhsRange.پہلا < 0) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect with this variable-length operand");
            أرجع Build_نتیجہ::failWithError();
        }
        ثابت صحيح lo = std::max(lhsRange.پہلا, rhsRange.پہلا);
        ثابت صحيح hi = std::min(lhsRange.دوجا, rhsRange.دوجا);
        إذا (lo > hi) {
            // Disjoint length ranges share no common length -> never matches.
            أرجع build_Never_میل_Intersect(nodep, اندراج_Vtxp, is_چوٹی_Level_قدم,
                                            "intersect operands have no common length");
        }
        File_سطر* ثابت flp = nodep->fileline();
        إذا (lo == hi) {
            Ast_گرہ_اظہاریہ* ثابت lp = realize_At_لمائی(nodep->lhsp(), lo, lhsRange.پہلا);
            Ast_گرہ_اظہاریہ* ثابت rp = realize_At_لمائی(nodep->rhsp(), lo, rhsRange.پہلا);
            Ast_گرہ_اظہاریہ* ثابت conjp = conjoinFixedSeqs(lp, rp, flp);
            VL_DO_DANGLING(lp->delete_درخت(), lp);
            VL_DO_DANGLING(rp->delete_درخت(), rp);
            إذا (!conjp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: intersect operand is not a plain boolean sequence");
                أرجع Build_نتیجہ::failWithError();
            }
            أرجع build_From_Lowering_درخت(conjp, اندراج_Vtxp, is_چوٹی_Level_قدم);
        }
        ثابت SimpleRanged sl = asSimpleRanged(nodep->lhsp());
        ثابت SimpleRanged sr = asSimpleRanged(nodep->rhsp());
        إذا (!sl.ok || !sr.ok) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect of two sequences that each vary in length over a"
                          " range with internal structure");
            أرجع Build_نتیجہ::failWithError();
        }
        ثابت خودکار andBool = [&](Ast_گرہ_اظہاریہ* ap, Ast_گرہ_اظہاریہ* bp) -> Ast_گرہ_اظہاریہ* {
            Ast_گرہ_اظہاریہ* ثابت aClonep
                = ap ? ap->clone_درخت_خالص(خطأ) : جديد AstConst{flp, AstConst::ثنائی_ہندسہ_True{}};
            Ast_گرہ_اظہاریہ* ثابت bClonep
                = bp ? bp->clone_درخت_خالص(خطأ) : جديد AstConst{flp, AstConst::ثنائی_ہندسہ_True{}};
            AstLogAnd* ثابت andp = جديد AstLogAnd{flp, aClonep, bClonep};
            andp->dtype_سیٹ_ثنائی_ہندسہ();
            أرجع andp;
        };
        AstDelay* ثابت delayp = جديد AstDelay{flp, جديد AstConst{flp, static_cast<uint32_t>(lo)},
                                              /*isCycle=*/سچ};
        delayp->rhsp(جديد AstConst{flp, static_cast<uint32_t>(hi)});
        AstSExpr* ثابت reducedp
            = جديد AstSExpr{flp, andBool(sl.startp, sr.startp), delayp, andBool(sl.endp, sr.endp)};
        reducedp->dtype_سیٹ_ثنائی_ہندسہ();
        أرجع build_From_Lowering_درخت(reducedp, اندراج_Vtxp, is_چوٹی_Level_قدم);
    }

    Build_نتیجہ buildThroughout(AstSThroughout* nodep, Sva_State_راس* اندراج_Vtxp,
                                منطقي is_چوٹی_Level_قدم = خطأ) {
        // Mark entryVtxp so "cond false at tick 0" is detected as throughout-drop.
        اندراج_Vtxp->m_throughoutConds.ڈالو_پٹھ(nodep->lhsp()->clone_درخت_خالص(خطأ));
        m_temporal_Guard_ڈھیر.ڈالو_پٹھ(nodep->lhsp());
        ثابت Build_نتیجہ نتیجہ = build_اظہاریہ(nodep->rhsp(), اندراج_Vtxp, is_چوٹی_Level_قدم);
        m_temporal_Guard_ڈھیر.نکالو_پٹھ();
        أرجع نتیجہ;
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
    Build_نتیجہ buildUntil(AstUntil* nodep, Sva_State_راس* اندراج_Vtxp, منطقي is_چوٹی_Level_قدم) {
        File_سطر* ثابت flp = nodep->fileline();
        إذا (!is_چوٹی_Level_قدم) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            أرجع Build_نتیجہ::failWithError();
        }
        Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp();
        Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp();
        ثابت خودکار hasSeq = [](ثابت Ast_گرہ_اظہاریہ* ep) {
            أرجع ep->exists([](ثابت Ast_گرہ_اظہاریہ* np) { أرجع np->is_Multi_چکر_Sva(); });
        };
        إذا (hasSeq(lhsp) || hasSeq(rhsp)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            أرجع Build_نتیجہ::failWithError();
        }

        ثابت منطقي ov = nodep->isOverlapping();
        Ast_گرہ_اظہاریہ* ثابت lhsBitp = nodep->lhsp();
        Ast_گرہ_اظہاریہ* ثابت rhsBitp = nodep->rhsp();
        // p hoist count: continue, require (ov: 1 use; nov: 1 use). At least 2 uses.
        AstVar* ثابت pHoistp = tryHoistSampled(lhsBitp, flp, 2);
        // q hoist count: continue (1) + require nov (1) = 2; ov: continue only (1).
        AstVar* ثابت qHoistp = ov ? خالی_سوچک : tryHoistSampled(rhsBitp, flp, 2);

        Sva_State_راس* ثابت waitCp = scoped_Create_راس();
        Sva_State_راس* ثابت waitRp = scoped_Create_راس();
        waitCp->m_isUnbounded = سچ;
        waitRp->m_strongPending = nodep->isStrong() && !m_isCover;

        // Entry and back-loop Links carry no condition; throughout-folding still applies.
        guardedLink(اندراج_Vtxp, waitCp, flp);
        guardedLink(waitRp, waitCp, flp);

        // Continue clocked edge: p && !q advances to next-cycle wait.
        Ast_گرہ_اظہاریہ* ثابت contCondp
            = جديد AstLogAnd{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                            جديد AstLogNot{flp, sampledRefOrClone(qHoistp, rhsBitp, flp)}};
        guarded_کنارا(waitCp, waitRp, contCondp, flp);

        // Reject sink: fires when require-condition is false.
        Sva_State_راس* ثابت sinkVtxp = m_گراف.create_State_راس();
        sinkVtxp->m_isRejectSink = سچ;
        Ast_گرہ_اظہاریہ* requireCondp;
        إذا (ov) {
            requireCondp = sampledRefOrClone(pHoistp, lhsBitp, flp);
        } وإلا {
            requireCondp = جديد AstLogOr{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                                        sampledRefOrClone(qHoistp, rhsBitp, flp)};
        }
        Sva_Trans_کنارا* ثابت rejEdgep = m_گراف.جوڑو_Link(waitCp, sinkVtxp, requireCondp);
        rejEdgep->m_rejectOnFail = سچ;

        // Accept condition rides via finalCondp; assembleResult $sampled-wraps it.
        Ast_گرہ_اظہاریہ* acceptCondp;
        إذا (ov) {
            acceptCondp
                = جديد AstLogAnd{flp, lhsBitp->clone_درخت_خالص(خطأ), rhsBitp->clone_درخت_خالص(خطأ)};
        } وإلا {
            acceptCondp = rhsBitp->clone_درخت_خالص(خطأ);
        }
        أرجع {waitCp, acceptCondp, {}};
    }

    // IEEE 1800-2023 16.12.14 property abort operators. Sync and async share
    // the same NFA encoding: AstSampled already gives matured values at every
    // maturing clocking event, and async firing "between clocks" is not
    // observable in a cycle-based model. VAbortKind selects accept vs reject
    // verdict (sync vs async only changes the user-visible spelling).

    // Build `condp && !outer_1 && !outer_2 ...` (unsampled).
    Ast_گرہ_اظہاریہ* abort_Fire_اظہاریہ(Ast_گرہ_اظہاریہ* condp, File_سطر* flp) {
        Ast_گرہ_اظہاریہ* resultp = condp->clone_درخت_خالص(خطأ);
        لكل (Ast_گرہ_اظہاریہ* ثابت op : m_outer_Abort_ڈھیر)
            resultp = جديد AstLogAnd{flp, resultp, جديد AstLogNot{flp, op->clone_درخت_خالص(خطأ)}};
        أرجع resultp;
    }

    // True when unguarded ring-wide rejection or a same-tick Link chain already accounts the
    // attempt: a required-step Link covers both outcomes; followed-by pairs both edges.
    ساكن منطقي chain_Accounts_سروت(ثابت Sva_State_راس* srcp,
                                    ثابت std::unordered_سیٹ<ثابت V3_گراف_کنارا*>& pre_کنارے) {
        إذا (srcp->m_delay_Ring_ناپ && srcp->m_throughoutConds.خالی_تھاواں()) أرجع سچ;
        لكل (ثابت V3_گراف_کنارا& edger : srcp->وچ_کنارے()) {
            إذا (pre_کنارے.گنتی(&edger)) استمر;
            ثابت Sva_Trans_کنارا& tedger = static_cast<ثابت Sva_Trans_کنارا&>(edger);
            إذا (tedger.m_consumes_چکر) استمر;
            ثابت خودکار* ثابت fromp = static_cast<ثابت Sva_State_راس*>(tedger.fromVtxp());
            إذا (fromp->m_abortRejectp) أرجع سچ;
        }
        منطقي plainNonSink = خطأ;
        منطقي markedSink = خطأ;
        لكل (ثابت V3_گراف_کنارا& edger : srcp->باہرلا_کنارے()) {
            إذا (pre_کنارے.گنتی(&edger)) استمر;
            ثابت Sva_Trans_کنارا& tedger = static_cast<ثابت Sva_Trans_کنارا&>(edger);
            إذا (tedger.m_consumes_چکر) استمر;
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
    فراغ جوڑو_Abort_Reject_کنارا(Sva_State_راس* srcp, Sva_State_راس* sinkp, Ast_گرہ_اظہاریہ* condp,
                            File_سطر* flp) {
        Ast_گرہ_اظہاریہ* ثابت notFirep = جديد AstLogNot{flp, sampled(abort_Fire_اظہاریہ(condp, flp))};
        m_گراف.جوڑو_Link(srcp, sinkp, notFirep)->m_rejectOnFail = سچ;
        أرجع;
    }

    // On the fire tick: kill body threads; accept kinds also forgive step misses.
    فراغ gate_Body_کنارے_On_Abort(ثابت std::unordered_سیٹ<ثابت V3_گراف_کنارا*>& pre_کنارے,
                              Ast_گرہ_اظہاریہ* condp, VAbort_قسم قسم, File_سطر* flp) {
        لكل (V3_گراف_راس& vtxr : m_گراف.m_گراف.vertices()) {
            لكل (V3_گراف_کنارا& edger : vtxr.باہرلا_کنارے()) {
                إذا (pre_کنارے.گنتی(&edger)) استمر;
                Sva_Trans_کنارا* ثابت tedgep = static_cast<Sva_Trans_کنارا*>(&edger);
                إذا (tedgep->m_rejectOnFail) {
                    إذا (!قسم.isAccept()) استمر;
                    Ast_گرہ_اظہاریہ* ثابت firep = sampled(abort_Fire_اظہاریہ(condp, flp));
                    tedgep->m_condp
                        = tedgep->m_condp ? جديد AstLogOr{flp, tedgep->m_condp, firep} : firep;
                } وإلا إذا (tedgep->m_consumes_چکر) {
                    Ast_گرہ_اظہاریہ* ثابت notFirep
                        = جديد AstLogNot{flp, sampled(abort_Fire_اظہاریہ(condp, flp))};
                    tedgep->m_condp = tedgep->m_condp
                                          ? جديد AstLogAnd{flp, tedgep->m_condp, notFirep}
                                          : notFirep;
                }
            }
        }
    }

    Build_نتیجہ buildAbortOn(Ast_گرہ_اظہاریہ* condp, Ast_گرہ_اظہاریہ* bodyp, Sva_State_راس* اندراج_Vtxp,
                             VAbort_قسم قسم, File_سطر* flp, منطقي is_چوٹی_Level_قدم) {
        // Snapshot pre-body vertices/edges so post-build diff yields the body's sub-NFA.
        std::unordered_سیٹ<ثابت V3_گراف_راس*> preExisting;
        std::unordered_سیٹ<ثابت V3_گراف_کنارا*> pre_کنارے;
        لكل (V3_گراف_راس& vtxr : m_گراف.m_گراف.vertices()) {
            preExisting.داخل_کرو(&vtxr);
            لكل (V3_گراف_کنارا& edger : vtxr.باہرلا_کنارے()) pre_کنارے.داخل_کرو(&edger);
        }

        m_outer_Abort_ڈھیر.ڈالو_پٹھ(condp);
        ثابت Build_نتیجہ body_نتیجہ = build_اظہاریہ(bodyp, اندراج_Vtxp, is_چوٹی_Level_قدم);
        m_outer_Abort_ڈھیر.نکالو_پٹھ();
        إذا (!body_نتیجہ.valid()) أرجع body_نتیجہ;

        gate_Body_کنارے_On_Abort(pre_کنارے, condp, قسم, flp);

        // Live-thread sources for the abort edge: entry + new body vertices,
        // minus reject sinks (they carry reject fuel, not live-thread fuel).
        std::سمتیہ<Sva_State_راس*> abortSources;
        abortSources.ڈالو_پٹھ(اندراج_Vtxp);
        لكل (V3_گراف_راس& vtxr : m_گراف.m_گراف.vertices()) {
            إذا (preExisting.گنتی(&vtxr)) استمر;
            خودکار* ثابت sp = static_cast<Sva_State_راس*>(&vtxr);
            إذا (sp->m_delay_Ring_ناپ || !sp->m_throughoutConds.خالی_تھاواں()) {
                Ast_گرہ_اظہاریہ* ثابت firep = abort_Fire_اظہاریہ(condp, flp);
                sp->m_abortClearp
                    = sp->m_abortClearp ? جديد AstLogOr{flp, sp->m_abortClearp, firep} : firep;
            }
            إذا (!قسم.isAccept()
                && ((sp->m_delay_Ring_ناپ && sp->m_throughoutConds.خالی_تھاواں())
                    || sp->m_replayAbortReject)) {
                Ast_گرہ_اظہاریہ* ثابت firep = abort_Fire_اظہاریہ(condp, flp);
                sp->m_abortRejectp
                    = sp->m_abortRejectp ? جديد AstLogOr{flp, sp->m_abortRejectp, firep} : firep;
            }
            إذا (sp->m_isRejectSink) استمر;
            abortSources.ڈالو_پٹھ(sp);
        }

        خودکار sampledAbortFire = [&]() -> Ast_گرہ_اظہاریہ* {
            Ast_گرہ_اظہاریہ* ثابت اظہاریہ = abort_Fire_اظہاریہ(condp, flp);
            أرجع sampled(اظہاریہ);
        };

        إذا (قسم.isAccept()) {
            // Match-only sink fed by $sampled(abort-fire) from every live source;
            // registered as midSource so it never contributes a reject.
            Sva_State_راس* ثابت acceptSinkp = scoped_Create_راس();
            لكل (Sva_State_راس* ثابت srcp : abortSources)
                guardedLink(srcp, acceptSinkp, sampledAbortFire(), flp);
            std::سمتیہ<Sva_State_راس*> midSources = body_نتیجہ.midSources;
            midSources.ڈالو_پٹھ(acceptSinkp);
            Ast_گرہ_اظہاریہ* finalCondp = body_نتیجہ.finalCondp;
            إذا (finalCondp) {
                إذا (finalCondp->backp()) finalCondp = finalCondp->clone_درخت_خالص(خطأ);
                finalCondp = جديد AstLogOr{flp, finalCondp, abort_Fire_اظہاریہ(condp, flp)};
            }
            أرجع {body_نتیجہ.رکن_Vertexp, finalCondp, std::move(midSources)};
        }

        // rejectOnFail treats m_condp as the success condition and fires on
        // !condp, so the edge carries !sampledAbortFire().
        Sva_State_راس* ثابت rejectSinkp = m_گراف.create_State_راس();
        rejectSinkp->m_isRejectSink = سچ;
        لكل (Sva_State_راس* ثابت srcp : abortSources)
            إذا (!chain_Accounts_سروت(srcp, pre_کنارے))
                جوڑو_Abort_Reject_کنارا(srcp, rejectSinkp, condp, flp);
        Ast_گرہ_اظہاریہ* finalCondp = body_نتیجہ.finalCondp;
        إذا (finalCondp) {
            إذا (finalCondp->backp()) finalCondp = finalCondp->clone_درخت_خالص(خطأ);
            finalCondp
                = جديد AstLogAnd{flp, finalCondp, جديد AstLogNot{flp, abort_Fire_اظہاریہ(condp, flp)}};
        }
        أرجع {body_نتیجہ.رکن_Vertexp, finalCondp, body_نتیجہ.midSources};
    }

عام:
    // Free a dropped sub-result condition that is not linked into the AST
    // (abort folds synthesize unparented finalCondp trees).
    ساكن فراغ freeUnlinkedCondp(Ast_گرہ_اظہاریہ* condp) {
        إذا (condp && !condp->backp()) VL_DO_DANGLING(condp->delete_درخت(), condp);
    }

    SvaNfaBuilder(Sva_گراف& گراف, Ast_گرہ_Module* modp, V3_Unique_ناں& prop_عارضی_ناں,
                  منطقي isCoverSeq = خطأ, منطقي isSeqEvent = خطأ, منطقي isCover = خطأ)
        : m_گراف{گراف}
        , m_modp{modp}
        , m_prop_عارضی_ناں{prop_عارضی_ناں}
        , m_isCover{isCover}
        , m_isCoverSeq{isCoverSeq}
        , m_isSeqEvent{isSeqEvent} {}

    // Reset scope between antecedent and consequent: liveness must not leak.
    // m_outerAbortStack survives: an abort wrapping the implication covers the
    // consequent too (IEEE 1800-2023 16.12.14).
    فراغ resetScope() {
        m_وچ_Unbounded_Scope = خطأ;
        m_temporal_Guard_ڈھیر.clear();
    }

    Build_نتیجہ build_اظہاریہ(Ast_گرہ_اظہاریہ* nodep, Sva_State_راس* اندراج_Vtxp,
                          منطقي is_چوٹی_Level_قدم = خطأ) {
        VL_RESTORER(m_want_Bool_پتی);
        ثابت منطقي want_Bool_پتی = m_want_Bool_پتی;
        m_want_Bool_پتی = خطأ;
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            أرجع buildSExpr(sexprp, اندراج_Vtxp, is_چوٹی_Level_قدم);
        }
        إذا (AstSConsRep* ثابت repp = VN_CAST(nodep, SConsRep)) {
            أرجع buildConsRep(repp, اندراج_Vtxp, is_چوٹی_Level_قدم);
        }
        إذا (AstPropAlways* ثابت alwaysp = VN_CAST(nodep, PropAlways)) {
            أرجع buildPropAlways(alwaysp, اندراج_Vtxp, is_چوٹی_Level_قدم);
        }
        إذا (AstSGotoRep* ثابت repp = VN_CAST(nodep, SGotoRep)) {
            أرجع buildGotoRep(repp, اندراج_Vtxp);
        }
        إذا (AstSThroughout* ثابت throughoutp = VN_CAST(nodep, SThroughout)) {
            أرجع buildThroughout(throughoutp, اندراج_Vtxp, is_چوٹی_Level_قدم);
        }
        إذا (AstSOr* ثابت orp = VN_CAST(nodep, SOr)) {
            أرجع build_Or_ملاؤ(orp->lhsp(), orp->rhsp(), اندراج_Vtxp, orp->fileline(),
                                want_Bool_پتی);
        }
        إذا (AstLogOr* ثابت orp = VN_CAST(nodep, LogOr)) {
            أرجع build_Or_ملاؤ(orp->lhsp(), orp->rhsp(), اندراج_Vtxp, orp->fileline(),
                                want_Bool_پتی);
        }
        إذا (AstSAnd* ثابت andp = VN_CAST(nodep, SAnd)) {
            أرجع buildAndCombiner(andp->lhsp(), andp->rhsp(), اندراج_Vtxp, andp->fileline());
        }
        إذا (AstSIntersect* ثابت intp = VN_CAST(nodep, SIntersect)) {
            // IEEE 1800-2023 16.9.6: both operands match over one window with
            // equal start and end (equal length). Lower to a single sequence
            // that conjoins both operands' per-cycle checks -- correct under
            // concurrent attempts, where the done-latch combiner conflates the
            // two operands' start times and over-accepts. The combiner remains
            // only as a fallback for operands that do not flatten.
            ثابت صحيح lhsLen = fixed_لمائی(intp->lhsp());
            ثابت صحيح rhsLen = fixed_لمائی(intp->rhsp());
            إذا (lhsLen >= 0 && rhsLen >= 0) {
                إذا (lhsLen != rhsLen) {
                    // Unequal fixed lengths share no common length -> never matches.
                    أرجع build_Never_میل_Intersect(intp, اندراج_Vtxp, is_چوٹی_Level_قدم,
                                                    "intersect operands have no common length");
                }
                إذا (Ast_گرہ_اظہاریہ* ثابت conjp
                    = conjoinFixedSeqs(intp->lhsp(), intp->rhsp(), intp->fileline())) {
                    أرجع build_From_Lowering_درخت(conjp, اندراج_Vtxp, is_چوٹی_Level_قدم);
                }
                أرجع buildAndCombiner(intp->lhsp(), intp->rhsp(), اندراج_Vtxp, intp->fileline());
            }
            أرجع buildVarLenIntersect(intp, اندراج_Vtxp, is_چوٹی_Level_قدم);
        }
        إذا (AstSWithin* ثابت withinp = VN_CAST(nodep, SWithin)) {
            أرجع buildSWithin(withinp, اندراج_Vtxp, is_چوٹی_Level_قدم);
        }
        إذا (AstAbortOn* ثابت ap = VN_CAST(nodep, AbortOn)) {
            أرجع buildAbortOn(ap->condp(), ap->propp(), اندراج_Vtxp, ap->قسم(), ap->fileline(),
                                is_چوٹی_Level_قدم);
        }
        إذا (VN_IS(nodep, SNonConsRep)) أرجع Build_نتیجہ::fail();
        إذا (AstImplication* ثابت implp = VN_CAST(nodep, Implication)) {
            أرجع build_Implication_کنارے(implp->lhsp(), implp->rhsp(), اندراج_Vtxp,
                                         implp->isOverlapped(), implp->isFollowedBy(),
                                         implp->lhsp(), implp->fileline());
        }
        إذا (AstUntil* ثابت untilp = VN_CAST(nodep, Until)) {
            أرجع buildUntil(untilp, اندراج_Vtxp, is_چوٹی_Level_قدم);
        }
        // Boolean leaf (including LogAnd): return as finalCond
        أرجع {اندراج_Vtxp, nodep, {}};
    }

    // Wire an implication / followed-by from `entryVtxp`: builds the antecedent,
    // emits the match-link (and for followed-by the reject-sink edge), inserts a
    // delay vertex for non-overlapped forms, and builds the body. Used both for
    // nested AstImplication in pexpr position and for the top-level assertion
    // antecedent -- `errorNodep` anchors the "unsupported sequence antecedent"
    // error, which differs between the two call sites.
    Build_نتیجہ build_Implication_کنارے(Ast_گرہ_اظہاریہ* antExprp, Ast_گرہ_اظہاریہ* bodyExprp,
                                      Sva_State_راس* اندراج_Vtxp, منطقي isOverlapped,
                                      منطقي isFollowedBy, Ast_گرہ* errorNodep, File_سطر* flp) {
        ثابت Build_نتیجہ ant_نتیجہ = build_اظہاریہ(antExprp, اندراج_Vtxp);
        إذا (!ant_نتیجہ.valid()) أرجع ant_نتیجہ;

        // Followed-by requires pure-boolean antecedent for non-vacuous-fail at
        // the attempt-start cycle. IEEE 1800-2023 16.12.9 permits a multi-cycle
        // sequence LHS, so this is an implementation gap rather than illegal SV.
        إذا (isFollowedBy && ant_نتیجہ.رکن_Vertexp != اندراج_Vtxp) {
            errorNodep->v3warn(E_UNSUPPORTED,
                               "Unsupported: sequence expression as antecedent of followed-by"
                               " (#-# / #=#) (IEEE 1800-2023 16.12.9)");
            أرجع Build_نتیجہ::failWithError();
        }
        UASSERT_OBJ(!isFollowedBy || ant_نتیجہ.finalCondp, errorNodep,
                    "followed-by antecedent terminal at entry must carry finalCondp");

        // Use raw createStateVertex() so trigVtxp starts without liveness --
        // reaching the antecedent terminal is a definitive event.
        Sva_State_راس* ثابت trigVtxp = m_گراف.create_State_راس();
        إذا (ant_نتیجہ.finalCondp) {
            m_گراف.جوڑو_Link(ant_نتیجہ.رکن_Vertexp, trigVtxp,
                            sampled(ant_نتیجہ.finalCondp->clone_درخت_خالص(خطأ)));
            // Followed-by non-vacuous fail: rejectOnFail fires when the attempt
            // is live (termVtx reachable) and sampled(antecedent) is false.
            إذا (isFollowedBy) {
                Sva_State_راس* ثابت sinkVtxp = m_گراف.create_State_راس();
                sinkVtxp->m_isRejectSink = سچ;
                Sva_Trans_کنارا* ثابت ep
                    = m_گراف.جوڑو_Link(ant_نتیجہ.رکن_Vertexp, sinkVtxp,
                                      sampled(ant_نتیجہ.finalCondp->clone_درخت_خالص(خطأ)));
                ep->m_rejectOnFail = سچ;
            }
            // finalCondp is cloned into the Sampled nodes; if the original is
            // not parented anywhere in the AST anymore it must be freed here
            // or ASan flags it as a leak (e.g. t_sequence_bool_ops).
            إذا (!ant_نتیجہ.finalCondp->backp()) {
                VL_DO_DANGLING(ant_نتیجہ.finalCondp->delete_درخت(), ant_نتیجہ.finalCondp);
            }
        } وإلا {
            m_گراف.جوڑو_Link(ant_نتیجہ.رکن_Vertexp, trigVtxp);
        }
        resetScope();

        Sva_State_راس* bodyEntryp = trigVtxp;
        إذا (!isOverlapped) {
            Sva_State_راس* ثابت delayVtxp = m_گراف.create_State_راس();
            m_گراف.جوڑو_Clocked_کنارا(trigVtxp, delayVtxp);
            bodyEntryp = delayVtxp;
        }
        // An empty antecedent match must not start the consequent, but a zero-minimum
        // repetition's empty match is not handled here; keep the merge vertex for those
        VL_RESTORER(m_want_Bool_پتی);
        m_want_Bool_پتی = !isFollowedBy && !hasZeroMinRep(antExprp);
        أرجع build_اظہاریہ(bodyExprp, bodyEntryp, /*isTopLevelStep=*/سچ);
    }

    Build_نتیجہ build(Ast_گرہ_اظہاریہ* exprp) {
        m_گراف.m_شروع_Vertexp = scoped_Create_راس();
        VL_RESTORER(m_want_Bool_پتی);
        m_want_Bool_پتی = سچ;
        أرجع build_اظہاریہ(exprp, m_گراف.m_شروع_Vertexp, /*isTopLevelStep=*/سچ);
    }
};

//######################################################################
// NFA Lowering (converts NFA graph to synthesizable AstAlways blocks)

صنف SvaNfaLowering final {
    Ast_گرہ_Module* ثابت m_modp;  // Module to add state vars and always blocks to
    Ast_گرہ_DType* ثابت m_u32DTypep;  // Shared unsigned counter dtype
    V3_Unique_ناں m_ناں{"__Vnfa"};
    ناپ_t m_stat_Delay_Ring_کنارا_Visits = 0;  // Delay-ring incoming edges visited

    // Per-lowering shared context (passed to phase sub-functions)
    // Per-vertex lowering state is stored in SvaVertexData and accessed via
    // V3GraphVertex::userp() (see SvaStateVertex::datap()).
    بنية ہیٹھلا_Ctx final {
        File_سطر* flp;  // Source location for generated AST
        Ast_Sen_درخت* senTreep;  // Clock sensitivity tree
        Ast_گرہ_اظہاریہ* disableExprp;  // disable iff expression (may be nullptr)
        Ast_گرہ_اظہاریہ* میل_Condp;  // Final boolean match condition (may be nullptr)
        VAssertType assertType;  // Assertion type for control tasks
        VAssertDirectiveType directiveType;  // Directive type for control tasks
        AstVar* killVarp;  // Last observed kill generation
        Sva_گراف& گراف;  // NFA graph
    };

    // Build a match-now expression: stateSig[i] && $sampled(condp)
    ساكن Ast_گرہ_اظہاریہ* build_میل_Now(File_سطر* flp, Ast_گرہ_اظہاریہ* stateExprp, Ast_گرہ_اظہاریہ* condp) {
        Ast_گرہ_اظہاریہ* ثابت statep = stateExprp->clone_درخت_خالص(خطأ);
        إذا (!condp) أرجع statep;
        أرجع جديد AstLogAnd{flp, statep, sampled(condp->clone_درخت_خالص(خطأ))};
    }
    ساكن Ast_گرہ_اظہاریہ* andCond(File_سطر* flp, Ast_گرہ_اظہاریہ* exprp, Ast_گرہ_اظہاریہ* condp) {
        إذا (!condp) أرجع exprp;
        أرجع جديد AstLogAnd{flp, exprp, condp->clone_درخت_خالص(خطأ)};
    }
    // bp is always non-null; only ap can be null (serving as accumulator).
    ساكن Ast_گرہ_اظہاریہ* orExprs(File_سطر* flp, Ast_گرہ_اظہاریہ* ap, Ast_گرہ_اظہاریہ* bp) {
        إذا (!ap) أرجع bp;
        أرجع جديد AstLogOr{flp, ap, bp};
    }
    ساكن Ast_گرہ_اظہاریہ* جوڑو_Countp(File_سطر* ثابت flp, Ast_گرہ_اظہاریہ* ثابت کل_Countp,
                                  Ast_گرہ_اظہاریہ* ثابت contributionp,
                                  Ast_گرہ_اظہاریہ* ثابت enablep = خالی_سوچک) {
        // contribution = enable ? contribution : 0;
        Ast_گرہ_اظہاریہ* ثابت enabledContributionp
            = enablep ? جديد AstCond{flp, enablep, contributionp,
                                    newTypedConstp(flp, contributionp->dtypep(), 0)}
                      : contributionp;
        إذا (!کل_Countp) أرجع enabledContributionp;
        أرجع جديد Ast_جوڑو{flp, کل_Countp, enabledContributionp};
    }
    ساكن Ast_گرہ_اظہاریہ* gateCheckedCountp(File_سطر* ثابت flp, Ast_گرہ_اظہاریہ* ثابت countp,
                                          Ast_گرہ_اظہاریہ*& overflowp, Ast_گرہ_اظہاریہ* ثابت enablep) {
        إذا (overflowp) {
            overflowp = جديد AstLogAnd{flp, enablep->clone_درخت_خالص(خطأ), overflowp};
        }
        أرجع جوڑو_Countp(flp, خالی_سوچک, countp, enablep);
    }
    ساكن Ast_گرہ_اظہاریہ* جوڑو_Checked_Countp(File_سطر* ثابت flp, Ast_گرہ_اظہاریہ* ثابت کل_Countp,
                                         Ast_گرہ_اظہاریہ* ثابت contributionp, Ast_گرہ_اظہاریہ*& overflowp,
                                         Ast_گرہ_اظہاریہ* ثابت contributionOverflowp = خالی_سوچک) {
        إذا (contributionOverflowp) { overflowp = orExprs(flp, overflowp, contributionOverflowp); }
        إذا (!کل_Countp) أرجع contributionp;
        Ast_گرہ_اظہاریہ* ثابت oldTotalp = کل_Countp->clone_درخت_خالص(خطأ);
        Ast_گرہ_اظہاریہ* ثابت resultp = جديد Ast_جوڑو{flp, کل_Countp, contributionp};
        overflowp
            = orExprs(flp, overflowp, جديد AstLt{flp, resultp->clone_درخت_خالص(خطأ), oldTotalp});
        أرجع resultp;
    }
    AstIf* new_گنتی_حد_توں_ودھ_Warning(File_سطر* ثابت flp, Ast_گرہ_اظہاریہ* ثابت condp) ثابت {
        AstDisplay* ثابت dispp = جديد AstDisplay{
            flp, VDisplayType::DT_WARNING,
            "Cover sequence match count overflowed 32-bit storage; results may be incorrect.",
            خالی_سوچک, خالی_سوچک};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstIf* ثابت ifp = جديد AstIf{flp, condp, dispp};
        ifp->branchPred(VBranchPred::BP_UNLIKELY);
        أرجع ifp;
    }
    ساكن Ast_گرہ_اظہاریہ* kill_سرگرم(ہیٹھلا_Ctx& c) {
        أرجع جديد AstNeq{c.flp, جديد AstVarRef{c.flp, c.killVarp, VAccess::READ},
                          assertKillGet(c.flp, c.assertType, c.directiveType)};
    }
    ساكن Ast_گرہ_اظہاریہ* not_Kill_سرگرم(ہیٹھلا_Ctx& c) { أرجع جديد AstLogNot{c.flp, kill_سرگرم(c)}; }
    ساكن Ast_گرہ_اظہاریہ* gateNotKill(ہیٹھلا_Ctx& c, Ast_گرہ_اظہاریہ* exprp) {
        إذا (!exprp) أرجع خالی_سوچک;
        أرجع جديد AstLogAnd{c.flp, exprp, not_Kill_سرگرم(c)};
    }
    ساكن Ast_گرہ_اظہاریہ* اگلا_Ring_اشاریہ(File_سطر* flp, Ast_گرہ_اظہاریہ* idxExprp, uint32_t ناپ) {
        ثابت خودکار u32Const = [flp](uint32_t قدر) {
            أرجع جديد AstConst{flp, AstConst::Widthed_قدر{}, 32, قدر};
        };
        UASSERT_OBJ(ناپ > 0, idxExprp, "Ring size must be positive");
        إذا (ناپ == 1) {
            VL_DO_DANGLING(idxExprp->delete_درخت(), idxExprp);
            أرجع u32Const(0);
        }
        // idx == size - 1 ? 0 : idx + 1
        Ast_جوڑو* ثابت addp = جديد Ast_جوڑو{flp, idxExprp->clone_درخت_خالص(خطأ), u32Const(1)};
        addp->dtypeFrom(idxExprp);
        AstCond* ثابت condp
            = جديد AstCond{flp, جديد AstEq{flp, idxExprp, u32Const(ناپ - 1)}, u32Const(0), addp};
        condp->dtypeFrom(idxExprp);
        أرجع condp;
    }
    ساكن Ast_گرہ_اظہاریہ* delay_Ring_تھاں(File_سطر* flp, Sva_State_راس* vtxp, Ast_گرہ_اظہاریہ* idxExprp,
                                      VAccess access = VAccess::READ) {
        // ring[idx]
        AstVar* ثابت ringp = vtxp->datap()->delayRingVarp;
        AstVarRef* ثابت refp = جديد AstVarRef{flp, ringp, access};
        إذا (vtxp->datap()->delayRingCounts) أرجع جديد AstArraySel{flp, refp, idxExprp};
        أرجع جديد AstSel{flp, refp, idxExprp, 1};
    }
    ساكن Ast_گرہ_اظہاریہ* delay_Ring_سر_گنتی(File_سطر* flp, Sva_State_راس* vtxp) {
        Ast_گرہ_اظہاریہ* ثابت headp
            = جديد AstVarRef{flp, vtxp->datap()->delay_Ring_سر_Varp, VAccess::READ};
        إذا (vtxp->datap()->delayRingCounts) أرجع headp;
        أرجع جديد AstExtend{flp, headp, vtxp->datap()->delay_Ring_جیوندا_گنتی_Varp->dtypep()->چوڑائی()};
    }
    ساكن Ast_گرہ_اظہاریہ* delay_Ring_At_آخری_اشاریہ(File_سطر* ثابت flp, AstVar* ثابت idxp,
                                             ثابت uint32_t ناپ) {
        أرجع جديد AstEq{flp, جديد AstVarRef{flp, idxp, VAccess::READ},
                         جديد AstConst{flp, AstConst::Widthed_قدر{}, 32, ناپ - 1}};
    }
    ساكن Ast_گرہ_اظہاریہ* delay_Ring_Output_گنتی(File_سطر* ثابت flp, Sva_State_راس* ثابت vtxp) {
        AstVar* ثابت idxp = vtxp->datap()->delayRingIdxVarp;
        ثابت uint32_t ناپ = vtxp->m_delay_Ring_ناپ;
        Ast_گرہ_اظہاریہ* outgoingValidp
            = جديد AstVarRef{flp, vtxp->datap()->delayRingWrappedVarp, VAccess::READ};
        إذا (!vtxp->m_isFixedDelayRing) {
            outgoingValidp
                = جديد AstLogOr{flp, outgoingValidp, delay_Ring_At_آخری_اشاریہ(flp, idxp, ناپ)};
        }
        AstVar* ثابت جیوندا_گنتی_Varp = vtxp->datap()->delay_Ring_جیوندا_گنتی_Varp;
        أرجع جديد AstCond{flp, outgoingValidp, delay_Ring_سر_گنتی(flp, vtxp),
                           newTypedConstp(flp, جیوندا_گنتی_Varp->dtypep(), 0)};
    }
    ساكن Ast_گرہ_اظہاریہ* delay_Ring_Has_جیوندا_Threadsp(File_سطر* ثابت flp,
                                                 AstVar* ثابت جیوندا_گنتی_Varp) {
        // active = live_count != 0;
        أرجع جديد AstNeq{flp, جديد AstVarRef{flp, جیوندا_گنتی_Varp, VAccess::READ},
                          newTypedConstp(flp, جیوندا_گنتی_Varp->dtypep(), 0)};
    }

    // Phase 3 output signals
    بنية Signal_سیٹ final {
        Ast_گرہ_اظہاریہ* terminalActivep = خالی_سوچک;  // OR of all successful terminal matches
        Ast_گرہ_اظہاریہ* میل_Countp = خالی_سوچک;  // NFA paths completing the sequence this tick
        Ast_گرہ_اظہاریہ* میل_گنتی_Overflowp = خالی_سوچک;  // Match count overflow this tick
        Ast_گرہ_اظہاریہ* rejectBasep = خالی_سوچک;  // Reject when a terminal match fails
        Ast_گرہ_اظہاریہ* required_قدم_Rejectp = خالی_سوچک;  // Per-source reject from rejectOnFail Links
        Ast_گرہ_اظہاریہ* throughoutRejectp = خالی_سوچک;  // Reject when a throughout guard drops
        Ast_گرہ_اظہاریہ* threadFailCountp = خالی_سوچک;  // Number of threads rejected on this tick
    };

    // Disable condition also aborts attempts between clock events. A constant disable
    // never rises, and $sampled is unsupported in a sensitivity list.
    ساكن منطقي isAsyncDisable(ثابت Ast_گرہ_اظہاریہ* ثابت disableExprp) {
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
    فراغ جوڑو_State_Always(ہیٹھلا_Ctx& c, Ast_گرہ* bodyp, Ast_گرہ* clearp) {
        m_modp->جوڑو_Stmtsp(
            جديد AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_درخت(خطأ), bodyp});
        إذا (!clearp) أرجع;
        m_modp->جوڑو_Stmtsp(جديد AstAlways{
            c.flp, VAlwaysKwd::ALWAYS,
            جديد Ast_Sen_درخت{c.flp, جديد Ast_Sen_شے{c.flp, VEdgeType::ET_POSEDGE,
                                                 c.disableExprp->clone_درخت_خالص(خطأ)}},
            clearp});
    }

    // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
    // Phase 2: State register NBA always block. Each clocked-edge target
    // latches the OR of its incoming contributions.
    فراغ emit_State_رجسٹر_Nba(ہیٹھلا_Ctx& c) {
        ثابت منطقي asyncDisable = isAsyncDisable(c.disableExprp);
        Ast_گرہ* bodyp = خالی_سوچک;
        Ast_گرہ* clearp = خالی_سوچک;
        لكل (ثابت V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
            ثابت Sva_State_راس* ثابت vtxp = static_cast<ثابت Sva_State_راس*>(&vtxr);
            إذا (!vtxp->datap()->stateVarp) استمر;

            Ast_گرہ_اظہاریہ* اگلا_Statep = خالی_سوچک;
            لكل (ثابت V3_گراف_کنارا& edger : vtxp->وچ_کنارے()) {
                ثابت Sva_Trans_کنارا& tedger = static_cast<ثابت Sva_Trans_کنارا&>(edger);
                إذا (!tedger.m_consumes_چکر) استمر;
                ثابت Sva_State_راس* ثابت fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Clocked-edge source missing stateSig");

                Ast_گرہ_اظہاریہ* srcSigp = fromVtxp->datap()->stateSigp->clone_درخت_خالص(خطأ);
                srcSigp = andCond(c.flp, srcSigp, tedger.m_condp);

                إذا (c.disableExprp) {
                    Ast_گرہ_اظہاریہ* ثابت notDisp
                        = جديد AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(خطأ)};
                    srcSigp = جديد AstLogAnd{c.flp, srcSigp, notDisp};
                }
                اگلا_Statep = orExprs(c.flp, اگلا_Statep, srcSigp);
            }

            UASSERT_OBJ(اگلا_Statep, vtxp,
                        "Registered vertex has no clocked incoming contribution");
            اگلا_Statep = gateNotKill(c, اگلا_Statep);

            Ast_سونپو_Dly* ثابت assignp = جديد Ast_سونپو_Dly{
                c.flp, جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE}, اگلا_Statep};
            bodyp = Ast_گرہ::جوڑو_اگلا_Null(bodyp, assignp);
            إذا (asyncDisable) {
                clearp = Ast_گرہ::جوڑو_اگلا_Null(
                    clearp,
                    جديد Ast_سونپو_Dly{
                        c.flp, جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE},
                        جديد AstConst{c.flp, AstConst::ثنائی_ہندسہ_False{}}});
            }
        }
        إذا (bodyp) جوڑو_State_Always(c, bodyp, clearp);
    }

    // Phase 2b: In-place ring storage and registered ring metadata.
    فراغ emitDelayRingUpdates(ہیٹھلا_Ctx& c, ثابت منطقي گنتی_Matches) {
        ثابت منطقي asyncDisable = isAsyncDisable(c.disableExprp);
        لكل (V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
            Sva_State_راس* ثابت vtxp = static_cast<Sva_State_راس*>(&vtxr);
            إذا (!vtxp->datap()->delayRingVarp) استمر;
            AstVar* ثابت idxp = vtxp->datap()->delayRingIdxVarp;
            AstVar* ثابت جیوندا_گنتی_Varp = vtxp->datap()->delay_Ring_جیوندا_گنتی_Varp;
            AstVar* ثابت wrappedp = vtxp->datap()->delayRingWrappedVarp;
            ثابت uint32_t ناپ = static_cast<uint32_t>(vtxp->m_delay_Ring_ناپ);

            Ast_گرہ_اظہاریہ* incomingp = خالی_سوچک;
            Ast_گرہ_اظہاریہ* incomingOverflowp = خالی_سوچک;
            لكل (ثابت V3_گراف_کنارا& edger : vtxp->وچ_کنارے()) {
                ++m_stat_Delay_Ring_کنارا_Visits;
                ثابت Sva_Trans_کنارا& tedger = static_cast<ثابت Sva_Trans_کنارا&>(edger);
                UASSERT_OBJ(tedger.m_consumes_چکر == vtxp->m_isFixedDelayRing, vtxp,
                            "Delay-ring incoming edge kind mismatch");
                ثابت Sva_State_راس* ثابت fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Delay-ring incoming source missing stateSig");
                UASSERT_OBJ(!گنتی_Matches || fromVtxp->datap()->state_گنتی_Sigp, fromVtxp,
                            "Delay-ring incoming source missing stateCountSig");
                // IEEE 1800-2023 16.14.1/16.14.3: assertion alternatives belong to one
                // evaluation; only cover sequence counts each end-of-match.
                Ast_گرہ_اظہاریہ* contribp
                    = گنتی_Matches
                          ? fromVtxp->datap()->state_گنتی_Sigp->clone_درخت_خالص(خطأ)
                          : جديد AstExtend{c.flp,
                                          fromVtxp->datap()->stateSigp->clone_درخت_خالص(خطأ),
                                          m_u32DTypep->چوڑائی()};
                Ast_گرہ_اظہاریہ* contribOverflowp
                    = گنتی_Matches && fromVtxp->datap()->state_گنتی_حد_توں_ودھ_Sigp
                          ? fromVtxp->datap()->state_گنتی_حد_توں_ودھ_Sigp->clone_درخت_خالص(خطأ)
                          : خالی_سوچک;
                إذا (tedger.m_condp) {
                    Ast_گرہ_اظہاریہ* ثابت enablep = tedger.m_condp->clone_درخت_خالص(خطأ);
                    contribp = گنتی_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, enablep)
                                   : جوڑو_Countp(c.flp, خالی_سوچک, contribp, enablep);
                }
                إذا (c.disableExprp) {
                    Ast_گرہ_اظہاریہ* ثابت notDisp
                        = جديد AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(خطأ)};
                    contribp = گنتی_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, notDisp)
                                   : جوڑو_Countp(c.flp, خالی_سوچک, contribp, notDisp);
                }
                incomingp = گنتی_Matches ? جوڑو_Checked_Countp(c.flp, incomingp, contribp,
                                                            incomingOverflowp, contribOverflowp)
                                         : جوڑو_Countp(c.flp, incomingp, contribp);
            }
            UASSERT_OBJ(incomingp, vtxp, "Delay ring has no incoming edge");
            ثابت منطقي storesCounts = vtxp->datap()->delayRingCounts;
            Ast_گرہ_اظہاریہ* ثابت incomingCountp
                = storesCounts
                      ? incomingp->clone_درخت_خالص(خطأ)
                      : جديد AstExtend{c.flp,
                                      جديد AstNeq{c.flp, incomingp->clone_درخت_خالص(خطأ),
                                                 newTypedConstp(c.flp, incomingp->dtypep(), 0)},
                                      m_u32DTypep->چوڑائی()};
            Ast_گرہ_اظہاریہ* ثابت storedIncomingp
                = storesCounts ? incomingp
                               : جديد AstNeq{c.flp, incomingp,
                                            newTypedConstp(c.flp, incomingp->dtypep(), 0)};
            // Only this block accesses the storage. Other processes read the cached head, so
            // the ring can be updated in place without an NBA shadow copy.
            Ast_سونپو* ثابت writeIncomingp = جديد Ast_سونپو{
                c.flp,
                delay_Ring_تھاں(c.flp, vtxp, جديد AstVarRef{c.flp, idxp, VAccess::READ},
                              VAccess::WRITE),
                storedIncomingp};
            Ast_گرہ* updateBodyp = writeIncomingp;
            // live_count <= live_count - outgoing_count + incoming_count;
            Ast_گرہ_اظہاریہ* ثابت outgoingp = delay_Ring_Output_گنتی(c.flp, vtxp);
            Ast_گرہ_اظہاریہ* ثابت afterOutgoingp
                = جديد AstSub{c.flp, جديد AstVarRef{c.flp, جیوندا_گنتی_Varp, VAccess::READ}, outgoingp};
            Ast_گرہ_اظہاریہ* ثابت oldAfterOutgoingp = afterOutgoingp->clone_درخت_خالص(خطأ);
            Ast_جوڑو* ثابت اگلا_جیوندا_Countp = جديد Ast_جوڑو{c.flp, afterOutgoingp, incomingCountp};
            إذا (storesCounts) {
                Ast_گرہ_اظہاریہ* ثابت جیوندا_Overflowp
                    = جديد AstLt{c.flp, اگلا_جیوندا_Countp->clone_درخت_خالص(خطأ), oldAfterOutgoingp};
                Ast_گرہ_اظہاریہ* ثابت overflowp = orExprs(c.flp, incomingOverflowp, جیوندا_Overflowp);
                updateBodyp = new_گنتی_حد_توں_ودھ_Warning(c.flp, overflowp);
                updateBodyp->جوڑو_اگلا(writeIncomingp);
            } وإلا {
                VL_DO_DANGLING(oldAfterOutgoingp->delete_درخت(), oldAfterOutgoingp);
            }
            // Cache the outgoing slot for the next index after the write. Reading after
            // writing also handles one-slot rings and two-slot range rings without alias cases.
            Ast_گرہ_اظہاریہ* اگلا_سر_Idxp
                = اگلا_Ring_اشاریہ(c.flp, جديد AstVarRef{c.flp, idxp, VAccess::READ}, ناپ);
            إذا (!vtxp->m_isFixedDelayRing) {
                اگلا_سر_Idxp = اگلا_Ring_اشاریہ(c.flp, اگلا_سر_Idxp, ناپ);
            }
            updateBodyp->جوڑو_اگلا(جديد Ast_سونپو_Dly{
                c.flp, جديد AstVarRef{c.flp, vtxp->datap()->delay_Ring_سر_Varp, VAccess::WRITE},
                delay_Ring_تھاں(c.flp, vtxp, اگلا_سر_Idxp)});
            updateBodyp->جوڑو_اگلا(جديد Ast_سونپو_Dly{
                c.flp, جديد AstVarRef{c.flp, جیوندا_گنتی_Varp, VAccess::WRITE}, اگلا_جیوندا_Countp});
            // wrapped <= wrapped || idx == size - 1;
            updateBodyp->جوڑو_اگلا(
                جديد Ast_سونپو_Dly{c.flp, جديد AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                 جديد AstLogOr{c.flp, جديد AstVarRef{c.flp, wrappedp, VAccess::READ},
                                              delay_Ring_At_آخری_اشاریہ(c.flp, idxp, ناپ)}});
            // idx <= next_idx;
            updateBodyp->جوڑو_اگلا(جديد Ast_سونپو_Dly{
                c.flp, جديد AstVarRef{c.flp, idxp, VAccess::WRITE},
                اگلا_Ring_اشاریہ(c.flp, جديد AstVarRef{c.flp, idxp, VAccess::READ}, ناپ)});
            إذا (vtxp->m_delayRingAdvanceCondp) {
                updateBodyp = جديد AstIf{
                    c.flp, sampled(vtxp->m_delayRingAdvanceCondp->clone_درخت_خالص(خطأ)),
                    updateBodyp};
            }

            Ast_گرہ_اظہاریہ* clearCondp = kill_سرگرم(c);
            إذا (vtxp->m_delayRingClearCondp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_delayRingClearCondp->clone_درخت_خالص(خطأ)));
            }
            إذا (vtxp->m_abortClearp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_abortClearp->clone_درخت_خالص(خطأ)));
            }
            إذا (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_درخت_خالص(خطأ));
            }
            Ast_گرہ_اظہاریہ* guardp = خالی_سوچک;
            لكل (Ast_گرہ_اظہاریہ* ثابت cp : vtxp->m_throughoutConds) {
                Ast_گرہ_اظہاریہ* ثابت sampledp = sampled(cp->clone_درخت_خالص(خطأ));
                guardp = guardp ? static_cast<Ast_گرہ_اظہاریہ*>(جديد AstLogAnd{c.flp, guardp, sampledp})
                                : sampledp;
            }
            إذا (guardp) clearCondp = orExprs(c.flp, clearCondp, جديد AstLogNot{c.flp, guardp});
            // Logically clear the ring without touching its wide storage.
            Ast_سونپو_Dly* ثابت clearCountp
                = جديد Ast_سونپو_Dly{c.flp, جديد AstVarRef{c.flp, جیوندا_گنتی_Varp, VAccess::WRITE},
                                   newTypedConstp(c.flp, جیوندا_گنتی_Varp->dtypep(), 0)};
            clearCountp->جوڑو_اگلا(جديد Ast_سونپو_Dly{c.flp,
                                                  جديد AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                                  جديد AstConst{c.flp, AstConst::ثنائی_ہندسہ_False{}}});
            clearCountp->جوڑو_اگلا(جديد Ast_سونپو_Dly{c.flp,
                                                  جديد AstVarRef{c.flp, idxp, VAccess::WRITE},
                                                  newTypedConstp(c.flp, idxp->dtypep(), 0)});
            updateBodyp = جديد AstIf{c.flp, clearCondp, clearCountp, updateBodyp};

            جوڑو_State_Always(c, updateBodyp, asyncDisable ? clearCountp->clone_درخت(سچ) : خالی_سوچک);
        }
    }

    // Phase 2c: SAnd combiner done-latch always block.
    // NBA semantics ensure doneL/doneR read pre-update values (IEEE 16.9.5).
    فراغ emit_And_Combiner_پورا_Latch_Nba(ہیٹھلا_Ctx& c) {
        ثابت منطقي asyncDisable = isAsyncDisable(c.disableExprp);
        لكل (ثابت V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
            ثابت Sva_State_راس* ثابت avp = static_cast<ثابت Sva_State_راس*>(&vtxr);
            إذا (!avp->datap()->پورا_LVarp) استمر;
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

            Ast_سونپو_Dly* ثابت clearLp = جديد Ast_سونپو_Dly{
                c.flp, جديد AstVarRef{c.flp, avp->datap()->پورا_LVarp, VAccess::WRITE},
                جديد AstConst{c.flp, AstConst::ثنائی_ہندسہ_False{}}};
            Ast_سونپو_Dly* ثابت clearRp = جديد Ast_سونپو_Dly{
                c.flp, جديد AstVarRef{c.flp, avp->datap()->پورا_RVarp, VAccess::WRITE},
                جديد AstConst{c.flp, AstConst::ثنائی_ہندسہ_False{}}};
            clearLp->جوڑو_اگلا(clearRp);

            Ast_گرہ_اظہاریہ* ثابت میل_LNowp
                = build_میل_Now(c.flp, lhsTermp->datap()->stateSigp, avp->m_andLhsCondp);
            Ast_گرہ_اظہاریہ* ثابت میل_RNowp
                = build_میل_Now(c.flp, rhsTermp->datap()->stateSigp, avp->m_andRhsCondp);
            Ast_گرہ_اظہاریہ* gateLp = میل_LNowp;
            Ast_گرہ_اظہاریہ* gateRp = میل_RNowp;
            إذا (c.disableExprp) {
                Ast_گرہ_اظہاریہ* ثابت notDisLp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(خطأ)};
                gateLp = جديد AstLogAnd{c.flp, gateLp, notDisLp};
                Ast_گرہ_اظہاریہ* ثابت notDisRp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(خطأ)};
                gateRp = جديد AstLogAnd{c.flp, gateRp, notDisRp};
            }
            Ast_سونپو_Dly* ثابت سیٹ_Lp = جديد Ast_سونپو_Dly{
                c.flp, جديد AstVarRef{c.flp, avp->datap()->پورا_LVarp, VAccess::WRITE},
                جديد AstConst{c.flp, AstConst::ثنائی_ہندسہ_True{}}};
            AstIf* ثابت سیٹ_LIfp = جديد AstIf{c.flp, gateLp, سیٹ_Lp, خالی_سوچک};
            Ast_سونپو_Dly* ثابت سیٹ_Rp = جديد Ast_سونپو_Dly{
                c.flp, جديد AstVarRef{c.flp, avp->datap()->پورا_RVarp, VAccess::WRITE},
                جديد AstConst{c.flp, AstConst::ثنائی_ہندسہ_True{}}};
            AstIf* ثابت سیٹ_RIfp = جديد AstIf{c.flp, gateRp, سیٹ_Rp, خالی_سوچک};
            سیٹ_LIfp->جوڑو_اگلا(سیٹ_RIfp);

            Ast_گرہ_اظہاریہ* clearCondp = جديد AstLogOr{c.flp, kill_سرگرم(c),
                                                   avp->datap()->stateSigp->clone_درخت_خالص(خطأ)};
            إذا (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_درخت_خالص(خطأ));
            }
            AstIf* ثابت topp = جديد AstIf{c.flp, clearCondp, clearLp, سیٹ_LIfp};
            جوڑو_State_Always(c, topp, asyncDisable ? clearLp->clone_درخت(سچ) : خالی_سوچک);
        }
    }

    فراغ emitKillAckNba(ہیٹھلا_Ctx& c) {
        Ast_سونپو_Dly* ثابت ackp
            = جديد Ast_سونپو_Dly{c.flp, جديد AstVarRef{c.flp, c.killVarp, VAccess::WRITE},
                               assertKillGet(c.flp, c.assertType, c.directiveType)};
        m_modp->جوڑو_Stmtsp(
            جديد AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_درخت(خطأ), ackp});
    }

    // Phase 3/3a/3b: Compute terminal match/reject signals, required-step reject,
    // throughout-drop reject; clean up intermediate state signals.
    // Phase 3: terminalActive and rejectBase from Links to matchVertex.
    // Builder only adds Links (non-clocked) to matchVertex via addLink in
    // wireMatchAndMidSources. For cover sequence, also count each end-of-match
    // so the action can be replayed without unrolling endpoints.
    فراغ compute_Terminal_میل_And_Reject(ہیٹھلا_Ctx& c, Signal_سیٹ& sigs, ثابت منطقي need_میل_گنتی) {
        لكل (ثابت Sva_Trans_کنارا* ثابت tedgep : c.گراف.all_کنارے()) {
            إذا (tedgep->منزل_Vtxp() != c.گراف.m_میل_Vertexp) استمر;
            Sva_State_راس* ثابت fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                        "Terminal-link source missing stateSig");

            Ast_گرہ_اظہاریہ* srcSigp = fromVtxp->datap()->stateSigp->clone_درخت_خالص(خطأ);
            srcSigp = andCond(c.flp, srcSigp, tedgep->m_condp);
            إذا (need_میل_گنتی) {
                UASSERT_OBJ(fromVtxp->datap()->state_گنتی_Sigp, fromVtxp,
                            "Terminal-link source missing stateCountSig");
                Ast_گرہ_اظہاریہ* contributionp
                    = fromVtxp->datap()->state_گنتی_Sigp->clone_درخت_خالص(خطأ);
                Ast_گرہ_اظہاریہ* contributionOverflowp
                    = fromVtxp->datap()->state_گنتی_حد_توں_ودھ_Sigp
                          ? fromVtxp->datap()->state_گنتی_حد_توں_ودھ_Sigp->clone_درخت_خالص(خطأ)
                          : خالی_سوچک;
                إذا (tedgep->m_condp) {
                    contributionp = gateCheckedCountp(c.flp, contributionp, contributionOverflowp,
                                                      tedgep->m_condp->clone_درخت_خالص(خطأ));
                }
                sigs.میل_Countp
                    = جوڑو_Checked_Countp(c.flp, sigs.میل_Countp, contributionp,
                                       sigs.میل_گنتی_Overflowp, contributionOverflowp);
            }

            إذا (fromVtxp->m_delay_Ring_ناپ && !fromVtxp->m_isFixedDelayRing) {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_درخت_خالص(خطأ));
                // reject |= (wrapped || idx == size - 1) && head && final_condition;
                Ast_گرہ_اظہاریہ* expireContribp = delay_Ring_Output_گنتی(c.flp, fromVtxp);
                expireContribp = andCond(c.flp, expireContribp, tedgep->m_condp);
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, expireContribp);
                VL_DO_DANGLING(srcSigp->delete_درخت(), srcSigp);
            } وإلا إذا (fromVtxp->m_isUnbounded || fromVtxp->m_isAndCombiner) {
                sigs.terminalActivep = orExprs(c.flp, sigs.terminalActivep, srcSigp);
            } وإلا {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_درخت_خالص(خطأ));
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, srcSigp);
            }
        }
        // wireMatchAndMidSources always adds a Link from result.termVertexp
        // to m_matchVertexp, so the loop above always sets terminalActivep.
        UASSERT_OBJ(sigs.terminalActivep, c.گراف.m_میل_Vertexp,
                    "No terminal edge to match vertex");
    }

    Ast_گرہ_اظہاریہ* newThroughoutThreadFailCountp(ہیٹھلا_Ctx& c, AstVar* ثابت delay_Ring_جیوندا_گنتی_Varp,
                                               Ast_گرہ_اظہاریہ* ثابت stateExprp,
                                               Ast_گرہ_اظہاریہ* ثابت enablep) {
        Ast_گرہ_اظہاریہ* ثابت سرگرم_Thread_Countp
            = delay_Ring_جیوندا_گنتی_Varp
                  ? static_cast<Ast_گرہ_اظہاریہ*>(
                        جديد AstVarRef{c.flp, delay_Ring_جیوندا_گنتی_Varp, VAccess::READ})
                  : جديد AstExtend{c.flp, stateExprp->clone_درخت_خالص(خطأ), m_u32DTypep->چوڑائی()};
        أرجع جوڑو_Countp(c.flp, خالی_سوچک, سرگرم_Thread_Countp, enablep);
    }

    // Phase 3b: Throughout-drop and ring-wide abort rejection.
    فراغ computeThroughoutReject(ہیٹھلا_Ctx& c, Signal_سیٹ& sigs, ثابت منطقي need_Thread_Fail_گنتی) {
        لكل (ثابت V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
            ثابت Sva_State_راس* ثابت vtxp = static_cast<ثابت Sva_State_راس*>(&vtxr);
            ثابت خودکار& conds = vtxp->m_throughoutConds;
            إذا (conds.خالی_تھاواں() && !vtxp->m_abortRejectp) استمر;
            إذا (vtxp->m_isAndCombiner) استمر;
            Ast_گرہ_اظہاریہ* stateExprp = خالی_سوچک;
            إذا (vtxp->datap()->stateVarp) {
                stateExprp = جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
            } وإلا إذا (vtxp->datap()->delayRingVarp && vtxp->m_isFixedDelayRing) {
                stateExprp
                    = delay_Ring_Has_جیوندا_Threadsp(c.flp, vtxp->datap()->delay_Ring_جیوندا_گنتی_Varp);
            } وإلا {
                UASSERT_OBJ(vtxp->datap()->stateSigp, vtxp,
                            "Throughout-conds vertex missing state representation");
                stateExprp = vtxp->datap()->stateSigp->clone_درخت_خالص(خطأ);
            }
            Ast_گرہ_اظہاریہ* guardp = خالی_سوچک;
            لكل (Ast_گرہ_اظہاریہ* ثابت cp : conds) {
                Ast_گرہ_اظہاریہ* ثابت sp = sampled(cp->clone_درخت_خالص(خطأ));
                guardp = guardp ? static_cast<Ast_گرہ_اظہاریہ*>(جديد AstLogAnd{c.flp, guardp, sp}) : sp;
            }
            إذا (vtxp->m_abortRejectp) {
                Ast_گرہ_اظہاریہ* ثابت notAbortp
                    = جديد AstLogNot{c.flp, sampled(vtxp->m_abortRejectp->clone_درخت_خالص(خطأ))};
                guardp = guardp
                             ? static_cast<Ast_گرہ_اظہاریہ*>(جديد AstLogAnd{c.flp, guardp, notAbortp})
                             : notAbortp;
            }
            Ast_گرہ_اظہاریہ* rejectCondp = جديد AstLogNot{c.flp, guardp};
            إذا (vtxp->m_abortClearp) {
                // Any abort clears the ring. Accept aborts suppress a simultaneous guard failure;
                // reject aborts are restored below so they still force rejection.
                Ast_گرہ_اظہاریہ* ثابت notAbortClearp
                    = جديد AstLogNot{c.flp, sampled(vtxp->m_abortClearp->clone_درخت_خالص(خطأ))};
                rejectCondp = جديد AstLogAnd{c.flp, rejectCondp, notAbortClearp};
                إذا (vtxp->m_abortRejectp) {
                    rejectCondp = جديد AstLogOr{
                        c.flp, rejectCondp, sampled(vtxp->m_abortRejectp->clone_درخت_خالص(خطأ))};
                }
            }
            إذا (need_Thread_Fail_گنتی) {
                Ast_گرہ_اظہاریہ* ثابت contributionp
                    = newThroughoutThreadFailCountp(c, vtxp->datap()->delay_Ring_جیوندا_گنتی_Varp,
                                                    stateExprp, rejectCondp->clone_درخت_خالص(خطأ));
                sigs.threadFailCountp = جوڑو_Countp(c.flp, sigs.threadFailCountp, contributionp);
                إذا (vtxp->m_abortRejectp && vtxp->m_delayRingAdvanceCondp) {
                    // An advancing ring thread also occupies its same-tick match vertex in the
                    // unrolled NFA, so abort rejection must replay both fail actions.
                    Ast_گرہ_اظہاریہ* ثابت abortAndAdvancep = جديد AstLogAnd{
                        c.flp, sampled(vtxp->m_abortRejectp->clone_درخت_خالص(خطأ)),
                        sampled(vtxp->m_delayRingAdvanceCondp->clone_درخت_خالص(خطأ))};
                    Ast_گرہ_اظہاریہ* ثابت میل_Contributionp = newThroughoutThreadFailCountp(
                        c, vtxp->datap()->delay_Ring_جیوندا_گنتی_Varp, stateExprp, abortAndAdvancep);
                    sigs.threadFailCountp
                        = جوڑو_Countp(c.flp, sigs.threadFailCountp, میل_Contributionp);
                }
            }
            sigs.throughoutRejectp = orExprs(c.flp, sigs.throughoutRejectp,
                                             جديد AstLogAnd{c.flp, stateExprp, rejectCondp});
        }
    }

    Signal_سیٹ compute_اشارے(ہیٹھلا_Ctx& c, ثابت منطقي need_Thread_Fail_گنتی,
                             ثابت منطقي need_Throughout_Thread_Fail_گنتی, ثابت منطقي need_میل_گنتی) {
        Signal_سیٹ sigs;

        compute_Terminal_میل_And_Reject(c, sigs, need_میل_گنتی);

        // Phase 3a: required-step rejection.
        // Builder only sets m_rejectOnFail on non-clocked Links with m_condp
        // or m_condVtxp, and the source always has a resolved stateSig.
        لكل (ثابت Sva_Trans_کنارا* ثابت tedgep : c.گراف.all_کنارے()) {
            إذا (!tedgep->m_rejectOnFail) استمر;
            ثابت Sva_State_راس* ثابت fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp && (tedgep->m_condp || tedgep->m_condVtxp),
                        fromVtxp,
                        "rejectOnFail Link must have condp/condVtxp and source stateSig");
            Ast_گرہ_اظہاریہ* ثابت srcSigp = fromVtxp->datap()->stateSigp->clone_درخت_خالص(خطأ);
            Ast_گرہ_اظہاریہ* condp = خالی_سوچک;
            إذا (tedgep->m_condVtxp) {
                UASSERT_OBJ(tedgep->m_condVtxp->datap()->stateSigp, tedgep->m_condVtxp,
                            "rejectOnFail condVtxp missing stateSig");
                condp = tedgep->m_condVtxp->datap()->stateSigp->clone_درخت_خالص(خطأ);
                إذا (tedgep->m_condp) {
                    condp = جديد AstLogOr{c.flp, condp, tedgep->m_condp->clone_درخت_خالص(خطأ)};
                }
            } وإلا {
                condp = tedgep->m_condp->clone_درخت_خالص(خطأ);
            }
            Ast_گرہ_اظہاریہ* ثابت notCondp = جديد AstLogNot{c.flp, condp};
            Ast_گرہ_اظہاریہ* ثابت rawFailp = جديد AstLogAnd{c.flp, srcSigp, notCondp};
            إذا (need_Thread_Fail_گنتی) {
                // thread_fail_count += fail;
                sigs.threadFailCountp = جوڑو_Countp(
                    c.flp, sigs.threadFailCountp,
                    جديد AstExtend{c.flp, rawFailp->clone_درخت_خالص(خطأ), m_u32DTypep->چوڑائی()});
            }
            Ast_گرہ_اظہاریہ* ثابت failp = gateNotKill(c, rawFailp);
            sigs.required_قدم_Rejectp = orExprs(c.flp, sigs.required_قدم_Rejectp, failp);
        }

        computeThroughoutReject(c, sigs, need_Throughout_Thread_Fail_گنتی);
        إذا (sigs.threadFailCountp) {
            sigs.threadFailCountp
                = جوڑو_Countp(c.flp, خالی_سوچک, sigs.threadFailCountp, not_Kill_سرگرم(c));
        }
        إذا (sigs.میل_Countp) {
            إذا (c.میل_Condp) {
                sigs.میل_Countp
                    = gateCheckedCountp(c.flp, sigs.میل_Countp, sigs.میل_گنتی_Overflowp,
                                        sampled(c.میل_Condp->clone_درخت_خالص(خطأ)));
            }
            sigs.میل_Countp = gateCheckedCountp(c.flp, sigs.میل_Countp, sigs.میل_گنتی_Overflowp,
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
            Ast_گرہ_اظہاریہ*& sigp = vtxp->datap()->stateSigp;
            إذا (sigp) VL_DO_DANGLING(sigp->delete_درخت(), sigp);
            Ast_گرہ_اظہاریہ*& countp = vtxp->datap()->state_گنتی_Sigp;
            إذا (countp) VL_DO_DANGLING(countp->delete_درخت(), countp);
            Ast_گرہ_اظہاریہ*& overflowp = vtxp->datap()->state_گنتی_حد_توں_ودھ_Sigp;
            إذا (overflowp) VL_DO_DANGLING(overflowp->delete_درخت(), overflowp);
        }
        // Disable iff gating (IEEE 1800-2023 16.12) on the current level value.
        إذا (c.disableExprp) {
            // terminalActivep is always set, so gate it unconditionally.
            Ast_گرہ_اظہاریہ* ثابت notTermp
                = جديد AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(خطأ)};
            sigs.terminalActivep = جديد AstLogAnd{c.flp, sigs.terminalActivep, notTermp};
            إذا (sigs.rejectBasep) {
                Ast_گرہ_اظہاریہ* ثابت notDisp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(خطأ)};
                sigs.rejectBasep = جديد AstLogAnd{c.flp, sigs.rejectBasep, notDisp};
            }
            إذا (sigs.throughoutRejectp) {
                Ast_گرہ_اظہاریہ* ثابت notDisp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(خطأ)};
                sigs.throughoutRejectp = جديد AstLogAnd{c.flp, sigs.throughoutRejectp, notDisp};
            }
            إذا (sigs.required_قدم_Rejectp) {
                Ast_گرہ_اظہاریہ* ثابت notDisp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(خطأ)};
                sigs.required_قدم_Rejectp = جديد AstLogAnd{c.flp, sigs.required_قدم_Rejectp, notDisp};
            }
            إذا (sigs.میل_Countp) {
                sigs.میل_Countp = gateCheckedCountp(
                    c.flp, sigs.میل_Countp, sigs.میل_گنتی_Overflowp,
                    جديد AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(خطأ)});
            }
        }

        إذا (sigs.میل_گنتی_Overflowp) {
            m_modp->جوڑو_Stmtsp(
                جديد AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_درخت(خطأ),
                              new_گنتی_حد_توں_ودھ_Warning(c.flp, sigs.میل_گنتی_Overflowp)});
            sigs.میل_گنتی_Overflowp = خالی_سوچک;
        }

        أرجع sigs;
    }

    // Phase 1: Resolve combinational Links via fixed-point propagation.
    فراغ resolveLinks(ہیٹھلا_Ctx& c, Ast_گرہ_اظہاریہ* triggerExprp, ثابت منطقي گنتی_Matches) {
        // datap() was freshly allocated in lower() -- all state signals start null.
        c.گراف.m_شروع_Vertexp->datap()->stateSigp = triggerExprp->clone_درخت_خالص(خطأ);
        c.گراف.m_شروع_Vertexp->datap()->state_گنتی_Sigp
            = جديد AstExtend{c.flp, triggerExprp->clone_درخت_خالص(خطأ), m_u32DTypep->چوڑائی()};
        لكل (V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
            Sva_State_راس* ثابت vtxp = static_cast<Sva_State_راس*>(&vtxr);
            إذا (vtxp->datap()->stateVarp) {
                vtxp->datap()->stateSigp
                    = جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
                vtxp->datap()->state_گنتی_Sigp = جديد AstExtend{
                    c.flp, جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ},
                    m_u32DTypep->چوڑائی()};
            } وإلا إذا (vtxp->datap()->delayRingVarp) {
                إذا (vtxp->m_isFixedDelayRing) {
                    // state_count = wrapped ? head : 0; state = state_count != 0;
                    Ast_گرہ_اظہاریہ* ثابت countp = delay_Ring_Output_گنتی(c.flp, vtxp);
                    vtxp->datap()->stateSigp = جديد AstNeq{c.flp, countp->clone_درخت_خالص(خطأ),
                                                          newTypedConstp(c.flp, m_u32DTypep, 0)};
                    vtxp->datap()->state_گنتی_Sigp = countp;
                } وإلا {
                    vtxp->datap()->stateSigp
                        = delay_Ring_Has_جیوندا_Threadsp(c.flp, vtxp->datap()->delay_Ring_جیوندا_گنتی_Varp);
                    vtxp->datap()->state_گنتی_Sigp = جديد AstVarRef{
                        c.flp, vtxp->datap()->delay_Ring_جیوندا_گنتی_Varp, VAccess::READ};
                }
            }
        }
        // Fixed-point propagation along zero-delay (Link) edges. Rebuild each
        // derived signal from its incoming edges on every pass; appending the same
        // contributions repeatedly makes merge expressions grow exponentially.
        ثابت ناپ_t passes = 2 * c.گراف.m_گراف.vertices().ناپ() + 2;
        لكل (ناپ_t pass = 0; pass < passes; ++pass) {
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
                Ast_گرہ_اظہاریہ* ثابت میل_Lp
                    = build_میل_Now(c.flp, lhsTermp->datap()->stateSigp, vtxp->m_andLhsCondp);
                Ast_گرہ_اظہاریہ* ثابت میل_Rp
                    = build_میل_Now(c.flp, rhsTermp->datap()->stateSigp, vtxp->m_andRhsCondp);
                Ast_گرہ_اظہاریہ* ثابت پورا_LOrp = جديد AstLogOr{
                    c.flp, جديد AstVarRef{c.flp, vtxp->datap()->پورا_LVarp, VAccess::READ}, میل_Lp};
                Ast_گرہ_اظہاریہ* ثابت پورا_ROrp = جديد AstLogOr{
                    c.flp, جديد AstVarRef{c.flp, vtxp->datap()->پورا_RVarp, VAccess::READ}, میل_Rp};
                Ast_گرہ_اظہاریہ* ثابت bothp = جديد AstLogAnd{c.flp, پورا_LOrp, پورا_ROrp};
                Ast_گرہ_اظہاریہ* ثابت oneNowp = جديد AstLogOr{c.flp, میل_Lp->clone_درخت_خالص(خطأ),
                                                          میل_Rp->clone_درخت_خالص(خطأ)};
                إذا (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_درخت(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = جديد AstLogAnd{c.flp, bothp, oneNowp};
                إذا (vtxp->datap()->state_گنتی_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_گنتی_Sigp->delete_درخت(),
                                   vtxp->datap()->state_گنتی_Sigp);
                }
                vtxp->datap()->state_گنتی_Sigp = جديد AstExtend{
                    c.flp, vtxp->datap()->stateSigp->clone_درخت_خالص(خطأ), m_u32DTypep->چوڑائی()};
            }

            لكل (V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
                Sva_State_راس* ثابت vtxp = static_cast<Sva_State_راس*>(&vtxr);
                إذا (vtxp == c.گراف.m_شروع_Vertexp || vtxp->datap()->stateVarp
                    || vtxp->datap()->delayRingVarp || vtxp->m_isAndCombiner || vtxp->m_is_میل
                    || vtxp->m_isRejectSink) {
                    استمر;
                }
                Ast_گرہ_اظہاریہ* اگلا_Statep = خالی_سوچک;
                Ast_گرہ_اظہاریہ* اگلا_State_Countp = خالی_سوچک;
                Ast_گرہ_اظہاریہ* اگلا_State_گنتی_Overflowp = خالی_سوچک;
                لكل (ثابت V3_گراف_کنارا& er : vtxp->وچ_کنارے()) {
                    ثابت Sva_Trans_کنارا& te = static_cast<ثابت Sva_Trans_کنارا&>(er);
                    ثابت Sva_State_راس* ثابت fromVtxp = te.fromVtxp();
                    إذا (!fromVtxp->datap()->stateSigp) استمر;
                    UASSERT_OBJ(fromVtxp->datap()->state_گنتی_Sigp, fromVtxp,
                                "Link source missing stateCountSig");
                    Ast_گرہ_اظہاریہ* ثابت contributionp = andCond(
                        c.flp, fromVtxp->datap()->stateSigp->clone_درخت_خالص(خطأ), te.m_condp);
                    اگلا_Statep = orExprs(c.flp, اگلا_Statep, contributionp);
                    Ast_گرہ_اظہاریہ* گنتی_Contributionp
                        = fromVtxp->datap()->state_گنتی_Sigp->clone_درخت_خالص(خطأ);
                    Ast_گرہ_اظہاریہ* گنتی_Contribution_Overflowp
                        = گنتی_Matches && fromVtxp->datap()->state_گنتی_حد_توں_ودھ_Sigp
                              ? fromVtxp->datap()->state_گنتی_حد_توں_ودھ_Sigp->clone_درخت_خالص(خطأ)
                              : خالی_سوچک;
                    إذا (te.m_condp) {
                        Ast_گرہ_اظہاریہ* ثابت enablep = te.m_condp->clone_درخت_خالص(خطأ);
                        گنتی_Contributionp
                            = گنتی_Matches
                                  ? gateCheckedCountp(c.flp, گنتی_Contributionp,
                                                      گنتی_Contribution_Overflowp, enablep)
                                  : جوڑو_Countp(c.flp, خالی_سوچک, گنتی_Contributionp, enablep);
                    }
                    اگلا_State_Countp
                        = گنتی_Matches
                              ? جوڑو_Checked_Countp(c.flp, اگلا_State_Countp, گنتی_Contributionp,
                                                 اگلا_State_گنتی_Overflowp,
                                                 گنتی_Contribution_Overflowp)
                              : جوڑو_Countp(c.flp, اگلا_State_Countp, گنتی_Contributionp);
                }
                إذا (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_درخت(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = اگلا_Statep;
                إذا (vtxp->datap()->state_گنتی_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_گنتی_Sigp->delete_درخت(),
                                   vtxp->datap()->state_گنتی_Sigp);
                }
                vtxp->datap()->state_گنتی_Sigp = اگلا_State_Countp;
                إذا (vtxp->datap()->state_گنتی_حد_توں_ودھ_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_گنتی_حد_توں_ودھ_Sigp->delete_درخت(),
                                   vtxp->datap()->state_گنتی_حد_توں_ودھ_Sigp);
                }
                vtxp->datap()->state_گنتی_حد_توں_ودھ_Sigp = اگلا_State_گنتی_Overflowp;
            }
        }
    }

عام:
    // Combine terminal/reject signals into final output expression.
    ساكن Ast_گرہ_اظہاریہ* assemble_نتیجہ(Ast_گرہ_Cover_Or_Assert* ثابت assertp, ثابت منطقي negated,
                                       Ast_گرہ_اظہاریہ* ثابت میل_Condp, ثابت Signal_سیٹ& sigs,
                                       Ast_گرہ_اظہاریہ** ثابت باہرلا_Matchpp) {
        File_سطر* ثابت flp = assertp->fileline();
        ثابت منطقي isCover = VN_IS(assertp, Cover);
        Ast_گرہ_اظہاریہ* ثابت terminalActivep = sigs.terminalActivep;
        Ast_گرہ_اظہاریہ* ثابت rejectBasep = sigs.rejectBasep;
        Ast_گرہ_اظہاریہ* ثابت throughoutRejectp = sigs.throughoutRejectp;
        Ast_گرہ_اظہاریہ* ثابت required_قدم_Rejectp = sigs.required_قدم_Rejectp;
        // Property negation (IEEE 1800-2023 16.12.1 `not`): invert match/reject.
        إذا (negated) {
            إذا (isCover) {
                إذا (terminalActivep)
                    VL_DO_DANGLING(terminalActivep->delete_درخت(), terminalActivep);
                Ast_گرہ_اظہاریہ* negRejectp = خالی_سوچک;
                إذا (میل_Condp && rejectBasep) {
                    Ast_گرہ_اظہاریہ* ثابت sampledCondp = sampled(میل_Condp->clone_درخت_خالص(خطأ));
                    Ast_گرہ_اظہاریہ* ثابت notCondp = جديد AstLogNot{flp, sampledCondp};
                    negRejectp = جديد AstLogAnd{flp, rejectBasep, notCondp};
                } وإلا إذا (rejectBasep) {
                    VL_DO_DANGLING(rejectBasep->delete_درخت(), rejectBasep);
                }
                إذا (throughoutRejectp) negRejectp = orExprs(flp, negRejectp, throughoutRejectp);
                إذا (required_قدم_Rejectp)
                    negRejectp = orExprs(flp, negRejectp, required_قدم_Rejectp);
                أرجع negRejectp ? negRejectp : جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}};
            }
            // Negated assert/assume: output = !match.
            Ast_گرہ_اظہاریہ* matchp = terminalActivep;
            إذا (میل_Condp) {
                Ast_گرہ_اظہاریہ* ثابت sampledCondp = sampled(میل_Condp->clone_درخت_خالص(خطأ));
                matchp = جديد AstLogAnd{flp, matchp, sampledCondp};
            }
            إذا (باہرلا_Matchpp) {
                Ast_گرہ_اظہاریہ* notPMatchp = خالی_سوچک;
                إذا (میل_Condp && rejectBasep) {
                    Ast_گرہ_اظہاریہ* ثابت sampledCondp = sampled(میل_Condp->clone_درخت_خالص(خطأ));
                    notPMatchp = جديد AstLogAnd{flp, rejectBasep->clone_درخت_خالص(خطأ),
                                               جديد AstLogNot{flp, sampledCondp}};
                } وإلا إذا (rejectBasep) {
                    notPMatchp = rejectBasep->clone_درخت_خالص(خطأ);
                }
                إذا (throughoutRejectp)
                    notPMatchp = orExprs(flp, notPMatchp, throughoutRejectp->clone_درخت_خالص(خطأ));
                إذا (required_قدم_Rejectp)
                    notPMatchp
                        = orExprs(flp, notPMatchp, required_قدم_Rejectp->clone_درخت_خالص(خطأ));
                *باہرلا_Matchpp = notPMatchp;
            }
            إذا (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_درخت(), throughoutRejectp);
            إذا (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_درخت(), rejectBasep);
            إذا (required_قدم_Rejectp)
                VL_DO_DANGLING(required_قدم_Rejectp->delete_درخت(), required_قدم_Rejectp);
            Ast_گرہ_اظہاریہ* ثابت نتیجہ_Exprp = جديد AstLogNot{flp, matchp};
            أرجع نتیجہ_Exprp;
        }
        إذا (isCover) {
            إذا (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_درخت(), throughoutRejectp);
            إذا (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_درخت(), rejectBasep);
            إذا (required_قدم_Rejectp)
                VL_DO_DANGLING(required_قدم_Rejectp->delete_درخت(), required_قدم_Rejectp);
            إذا (میل_Condp) {
                Ast_گرہ_اظہاریہ* ثابت sampledCondp = sampled(میل_Condp->clone_درخت_خالص(خطأ));
                أرجع جديد AstLogAnd{flp, terminalActivep, sampledCondp};
            }
            أرجع terminalActivep;
        }
        // Assert/assume: output = !reject
        Ast_گرہ_اظہاریہ* rejectp = خالی_سوچک;
        إذا (میل_Condp && rejectBasep) {
            Ast_گرہ_اظہاریہ* ثابت sampledCondp = sampled(میل_Condp->clone_درخت_خالص(خطأ));
            rejectp = جديد AstLogAnd{flp, rejectBasep, جديد AstLogNot{flp, sampledCondp}};
        } وإلا إذا (rejectBasep) {
            VL_DO_DANGLING(rejectBasep->delete_درخت(), rejectBasep);
        }
        إذا (باہرلا_Matchpp) {
            Ast_گرہ_اظہاریہ* میل_Exprp = terminalActivep->clone_درخت_خالص(خطأ);
            إذا (میل_Condp) {
                Ast_گرہ_اظہاریہ* ثابت sp = sampled(میل_Condp->clone_درخت_خالص(خطأ));
                میل_Exprp = جديد AstLogAnd{flp, میل_Exprp, sp};
            }
            *باہرلا_Matchpp = میل_Exprp;
        }
        إذا (terminalActivep) VL_DO_DANGLING(terminalActivep->delete_درخت(), terminalActivep);
        إذا (throughoutRejectp) rejectp = orExprs(flp, rejectp, throughoutRejectp);
        إذا (required_قدم_Rejectp) rejectp = orExprs(flp, rejectp, required_قدم_Rejectp);
        إذا (!rejectp) أرجع جديد AstConst{flp, AstConst::ثنائی_ہندسہ_True{}};
        Ast_گرہ_اظہاریہ* ثابت نتیجہ_Exprp = جديد AstLogNot{flp, rejectp};
        أرجع نتیجہ_Exprp;
    }

    واضح SvaNfaLowering(Ast_گرہ_Module* modp)
        : m_modp{modp}
        , m_u32DTypep{modp->لبھو_Basic_DType(VBasicDTypeKwd::UINT32)} {}
    ~SvaNfaLowering() {
        V3Stats::جوڑو_Stat_جوڑ("Assertions, NFA delay ring edge visits", m_stat_Delay_Ring_کنارا_Visits);
    }

    // Lower NFA graph to synthesizable AstAlways blocks and raw result signals.
    // Links are combinational; Edges are registered (NBA).
    Signal_سیٹ ہیٹھلا(Ast_گرہ_Cover_Or_Assert* ثابت assertp, Sva_گراف& گراف,
                    Ast_Sen_درخت* ثابت senTreep, Ast_گرہ_اظہاریہ* ثابت میل_Condp,
                    Ast_گرہ_اظہاریہ* ثابت disableExprp, ثابت منطقي need_Thread_Fail_گنتی,
                    ثابت منطقي need_Throughout_Thread_Fail_گنتی) {
        File_سطر* ثابت flp = assertp->fileline();
        AstCover* ثابت coverp = VN_CAST(assertp, Cover);
        ثابت منطقي isSeqEvent = coverp && coverp->isSeqEvent();
        ثابت VAssertType assertType
            = isSeqEvent ? VAssertType{VAssertType::اندرونی} : assertp->userType();
        ثابت VAssertDirectiveType directiveType
            = isSeqEvent ? VAssertDirectiveType{VAssertDirectiveType::اندرونی}
                         : assertp->directive();
        ثابت std::string ادھار_Name = m_ناں.get("");

        // Number vertices with sequential colors for array indexing.
        صحيح N = 0;
        لكل (V3_گراف_راس& vtxr : گراف.m_گراف.vertices()) { vtxr.color(N++); }
        std::سمتیہ<Sva_State_راس*> vtx(N, خالی_سوچک);
        لكل (V3_گراف_راس& vtxr : گراف.m_گراف.vertices()) {
            vtx[vtxr.color()] = static_cast<Sva_State_راس*>(&vtxr);
        }

        // Allocate per-vertex lowering data (stored via V3GraphVertex::userp()).
        std::سمتیہ<Sva_راس_انکڑے> راس_انکڑے(N);
        لكل (صحيح i = 0; i < N; ++i) vtx[i]->userp(&راس_انکڑے[i]);

        // Identify registered vertices (targets of clocked edges).
        لكل (صحيح i = 0; i < N; ++i) {
            لكل (ثابت V3_گراف_کنارا& edger : vtx[i]->باہرلا_کنارے()) {
                ثابت Sva_Trans_کنارا& tedger = static_cast<ثابت Sva_Trans_کنارا&>(edger);
                ثابت صحيح منزل_Idx = tedger.منزل_Vtxp()->color();
                إذا (tedger.m_consumes_چکر && tedger.منزل_Vtxp() != گراف.m_میل_Vertexp
                    && !tedger.منزل_Vtxp()->m_isRejectSink) {
                    vtx[منزل_Idx]->datap()->needsReg = سچ;
                }
            }
        }

        // A packed range ring can emit several live endpoints without needing per-slot counts.
        // Widen only a ring whose input can already carry multiplicity.
        std::سمتیہ<منطقي> stateMayHaveMultiplicity(N);
        std::سمتیہ<منطقي> ringNeedsMultiplicity(N);
        std::سمتیہ<صحيح> multiplicityWork;
        ثابت منطقي گنتی_Matches = coverp && coverp->isCoverSeq() && !isSeqEvent;
        إذا (گنتی_Matches) {
            لكل (صحيح i = 0; i < N; ++i) {
                ثابت منطقي مضرب_Inputs = vtx[i]->وچ_کنارے().ناپ() > 1;
                ثابت منطقي isRing = vtx[i]->m_delay_Ring_ناپ;
                ringNeedsMultiplicity[i] = isRing && مضرب_Inputs;
                ثابت منطقي startsMultiplicity = isRing
                                                    ? !vtx[i]->m_isFixedDelayRing
                                                    : مضرب_Inputs && !vtx[i]->datap()->needsReg;
                إذا (startsMultiplicity) {
                    stateMayHaveMultiplicity[i] = سچ;
                    multiplicityWork.ڈالو_پٹھ(i);
                }
            }
            لكل (ناپ_t wi = 0; wi < multiplicityWork.ناپ(); ++wi) {
                لكل (ثابت V3_گراف_کنارا& edger : vtx[multiplicityWork[wi]]->باہرلا_کنارے()) {
                    ثابت Sva_Trans_کنارا& tedger = static_cast<ثابت Sva_Trans_کنارا&>(edger);
                    ثابت صحيح ti = tedger.منزل_Vtxp()->color();
                    ثابت منطقي isRing = vtx[ti]->m_delay_Ring_ناپ;
                    إذا (isRing) ringNeedsMultiplicity[ti] = سچ;
                    // Match and reject sinks have no outgoing edges, so they stop naturally.
                    ثابت منطقي stopsMultiplicity = !isRing && vtx[ti]->datap()->needsReg;
                    إذا (stateMayHaveMultiplicity[ti] || stopsMultiplicity) { استمر; }
                    stateMayHaveMultiplicity[ti] = سچ;
                    multiplicityWork.ڈالو_پٹھ(ti);
                }
            }
        }

        ثابت منطقي asyncDisable = isAsyncDisable(disableExprp);
        // Registers cleared on a rising disable are also written by their clocked block.
        File_سطر* ثابت regFlp = asyncDisable ? جديد File_سطر{flp} : flp;
        إذا (asyncDisable) regFlp->warnOff(V3_Error_کوڈ::MULTIDRIVEN, سچ);

        AstVar* ثابت killVarp
            = جديد AstVar{flp, VVarType::MODULETEMP, ادھار_Name + "__kill", m_u32DTypep};
        killVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->جوڑو_Stmtsp(killVarp);
        لكل (صحيح i = 0; i < N; ++i) {
            إذا (vtx[i]->m_isAndCombiner) {
                ثابت std::string ادھار = ادھار_Name + "__a" + std::منزل_string(i);
                AstVar* ثابت lp = جديد AstVar{regFlp, VVarType::MODULETEMP, ادھار + "_doneL",
                                              m_modp->لبھو_ثنائی_ہندسہ_DType()};
                lp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڑو_Stmtsp(lp);
                vtx[i]->datap()->پورا_LVarp = lp;
                AstVar* ثابت rp = جديد AstVar{regFlp, VVarType::MODULETEMP, ادھار + "_doneR",
                                              m_modp->لبھو_ثنائی_ہندسہ_DType()};
                rp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڑو_Stmtsp(rp);
                vtx[i]->datap()->پورا_RVarp = rp;
                استمر;
            }
            إذا (vtx[i]->m_delay_Ring_ناپ) {
                ثابت std::string ادھار = ادھار_Name + "__d" + std::منزل_string(i);
                ثابت منطقي storesCounts = گنتی_Matches && ringNeedsMultiplicity[i];
                Ast_گرہ_DType* ringDTypep = خالی_سوچک;
                إذا (storesCounts) {
                    // int unsigned ring [size];
                    ringDTypep = جديد AstUnpackArrayDType{
                        flp, m_u32DTypep,
                        جديد AstRange{flp, static_cast<صحيح>(vtx[i]->m_delay_Ring_ناپ - 1), 0}};
                    v3Global.rootp()->typeTablep()->جوڑو_Typesp(ringDTypep);
                } وإلا {
                    // bit [size-1:0] ring;
                    ringDTypep = m_modp->لبھو_ثنائی_ہندسہ_DType(vtx[i]->m_delay_Ring_ناپ,
                                                      vtx[i]->m_delay_Ring_ناپ, VSigning::UNSIGNED);
                }
                AstVar* ثابت ringp
                    = جديد AstVar{flp, VVarType::MODULETEMP, ادھار + "_ring", ringDTypep};
                ringp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڑو_Stmtsp(ringp);
                vtx[i]->datap()->delayRingVarp = ringp;
                vtx[i]->datap()->delayRingCounts = storesCounts;
                AstVar* ثابت headp
                    = جديد AstVar{flp, VVarType::MODULETEMP, ادھار + "_head",
                                 storesCounts ? m_u32DTypep : m_modp->لبھو_ثنائی_ہندسہ_DType()};
                headp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڑو_Stmtsp(headp);
                vtx[i]->datap()->delay_Ring_سر_Varp = headp;
                // int unsigned idx;
                AstVar* ثابت idxp
                    = جديد AstVar{regFlp, VVarType::MODULETEMP, ادھار + "_idx", m_u32DTypep};
                idxp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڑو_Stmtsp(idxp);
                vtx[i]->datap()->delayRingIdxVarp = idxp;
                // int unsigned live_count;
                AstVar* ثابت جیوندا_گنتی_Varp
                    = جديد AstVar{regFlp, VVarType::MODULETEMP, ادھار + "_liveCount", m_u32DTypep};
                جیوندا_گنتی_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڑو_Stmtsp(جیوندا_گنتی_Varp);
                vtx[i]->datap()->delay_Ring_جیوندا_گنتی_Varp = جیوندا_گنتی_Varp;
                AstVar* ثابت wrappedp = جديد AstVar{regFlp, VVarType::MODULETEMP,
                                                    ادھار + "_wrapped", m_modp->لبھو_ثنائی_ہندسہ_DType()};
                wrappedp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڑو_Stmtsp(wrappedp);
                vtx[i]->datap()->delayRingWrappedVarp = wrappedp;
                استمر;
            }
            إذا (!vtx[i]->datap()->needsReg) استمر;
            إذا (vtx[i] == گراف.m_شروع_Vertexp || vtx[i]->m_is_میل) استمر;
            ثابت std::string varName = ادھار_Name + "__s" + std::منزل_string(i);
            AstVar* ثابت varp
                = جديد AstVar{regFlp, VVarType::MODULETEMP, varName, m_modp->لبھو_ثنائی_ہندسہ_DType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جوڑو_Stmtsp(varp);
            vtx[i]->datap()->stateVarp = varp;
        }

        // Build lowering context for phase sub-functions.
        ہیٹھلا_Ctx c{flp,        senTreep,      disableExprp, میل_Condp,
                   assertType, directiveType, killVarp,     گراف};

        // Phase 1: Resolve combinational Links via fixed-point propagation.
        Ast_گرہ_اظہاریہ* ثابت triggerExprp
            = isSeqEvent ? جديد AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}
                         : assertOnCond(flp, assertp->userType(), assertp->directive());
        resolveLinks(c, triggerExprp, گنتی_Matches);
        VL_DO_DANGLING(triggerExprp->delete_درخت(), triggerExprp);

        // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
        emit_State_رجسٹر_Nba(c);
        emitDelayRingUpdates(c, گنتی_Matches);
        emit_And_Combiner_پورا_Latch_Nba(c);
        emitKillAckNba(c);

        // Phase 3/3a/3b: Compute terminal match/reject signals (cleans up stateSig).
        ثابت Signal_سیٹ sigs
            = compute_اشارے(c, need_Thread_Fail_گنتی, need_Throughout_Thread_Fail_گنتی, گنتی_Matches);

        // Strong s_always[m:n] end-of-simulation liveness: if any in-window state
        // is still set at $finish, the universal-quantifier window never completed
        // (IEEE 1800-2023 16.12.11 strong semantics). Fire the assertion failure
        // from a final block; V3Assert turns the DT_ERROR display into the standard
        // "Assertion failed in %m" message.
        // pending = |{strong state registers, strong delay live counts != 0};
        Ast_گرہ_اظہاریہ* pendingp = خالی_سوچک;
        لكل (صحيح i = 0; i < N; ++i) {
            إذا (!vtx[i]->m_strongPending) استمر;
            Ast_گرہ_اظہاریہ* pendingExprp = خالی_سوچک;
            إذا (vtx[i]->datap()->stateVarp) {
                pendingExprp = جديد AstVarRef{flp, vtx[i]->datap()->stateVarp, VAccess::READ};
            } وإلا إذا (vtx[i]->m_strongAlwaysRing) {
                pendingExprp
                    = delay_Ring_Has_جیوندا_Threadsp(flp, vtx[i]->datap()->delay_Ring_جیوندا_گنتی_Varp);
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
            Ast_گرہ_اظہاریہ* ثابت condp = جديد AstLogAnd{flp, assertOnp, pendingp};
            AstDisplay* ثابت dispp
                = جديد AstDisplay{flp, VDisplayType::DT_ERROR, "", خالی_سوچک, خالی_سوچک};
            dispp->fmtp()->timeunit(m_modp->timeunit());
            Ast_گرہ_Stmt* ثابت firep = dispp;
            إذا (v3Global.opt.stopFail()) firep->جوڑو_اگلا(جديد AstStop{flp, خطأ});
            m_modp->جوڑو_Stmtsp(جديد AstFinal{flp, جديد AstIf{flp, condp, firep}});
        }

        // Clear userp on every vertex before vertexData is destroyed.
        لكل (صحيح i = 0; i < N; ++i) vtx[i]->userp(خالی_سوچک);
        أرجع sigs;
    }
};

}  // namespace

//######################################################################
// Top-level visitor

صنف AssertNfaVisitor final : عام VNVisitor {
    // STATE
    Ast_گرہ_Module* m_modp = خالی_سوچک;  // Current module being processed
    AstClocking* m_defaultClockingp = خالی_سوچک;  // Default clocking
    AstDefaultDisable* m_defaultDisablep = خالی_سوچک;  // Default disable iff
    SvaNfaLowering* m_loweringp = خالی_سوچک;  // NFA-to-hardware lowering engine
    Ast_Sen_درخت* m_sampled_قدر_Clockp = خالی_سوچک;  // Inherited clock during scoped attachment
    V3_Unique_ناں m_prop_Var_ناں{"__Vpropvar"};  // Property-local variable names
    V3_Unique_ناں m_prop_عارضی_ناں{"__VnfaSampled"};  // Hoisted $sampled(propp) temps
    V3_Unique_ناں m_fail_گنتی_ناں{"__VnfaRemainingFailCount"};  // Fail replay counter names
    std::سیٹ<ثابت AstProperty*> m_inliningProps;  // Recursion guard for inlineNamedProperty

    سانچا <قسم_ناں T_گرہ>
    فراغ ویکھو_Sampled_قدر(T_گرہ* ثابت nodep) {
        إذا (m_sampled_قدر_Clockp && !nodep->sentreep()) {
            nodep->sentreep(m_sampled_قدر_Clockp->clone_درخت(سچ));
        }
        iterateChildren(nodep);
    }

    // Wire match vertex and mid-window sources for a successful NFA build.
    ساكن فراغ wire_میل_And_Mid_Sources(Sva_گراف& گراف, ثابت Build_نتیجہ& نتیجہ, File_سطر* flp) {
        گراف.create_میل_راس();
        // Skip the main term Link when midSources already cover every
        // end-of-match (cover_sequence path); otherwise the per-mid extraction
        // double-counts via the merge vertex.
        إذا (!نتیجہ.رکن_Is_Mid_ملاؤ) { گراف.جوڑو_Link(نتیجہ.رکن_Vertexp, گراف.m_میل_Vertexp); }
        لكل (Sva_State_راس* srcVtxp : نتیجہ.midSources) {
            Ast_گرہ_اظہاریہ* condp = خالی_سوچک;
            لكل (Ast_گرہ_اظہاریہ* ثابت tc : srcVtxp->m_throughoutConds) {
                Ast_گرہ_اظہاریہ* ثابت tcClone = tc->clone_درخت_خالص(خطأ);
                condp = condp ? جديد AstLogAnd{flp, condp, tcClone} : tcClone;
            }
            گراف.جوڑو_Link(srcVtxp, گراف.m_میل_Vertexp, condp);
            srcVtxp->m_isUnbounded = سچ;
        }
    }

    ساكن Ast_گرہ_اظہاریہ* getSequenceBodyExprp(ثابت AstSequence* seqp) {
        Ast_گرہ* bodyp = seqp->stmtsp();
        طالما (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        أرجع VN_CAST(bodyp, گرہ_اظہاریہ);
    }

    ساكن AstPropSpec* getPropertySpecp(ثابت AstProperty* propp) {
        Ast_گرہ* stmtp = propp->stmtsp();
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
        إذا (!m_inliningProps.emplace(propyp).دوجا) {
            funcrefp->v3error("Illegal recursive property reference");  // LCOV_EXCL_LINE
            أرجع;  // LCOV_EXCL_LINE
        }
        بنية Guard final {
            std::سیٹ<ثابت AstProperty*>& setr;
            ثابت AstProperty* keyp;
            ~Guard() { setr.erase(keyp); }
        } guard{m_inliningProps, propyp};
        AstPropSpec* propSpecp = getPropertySpecp(propyp);
        UASSERT_OBJ(propSpecp, funcrefp, "Property has no body PropSpec");
        propSpecp = propSpecp->clone_درخت(خطأ);

        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propyp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_گرہ_اظہاریہ*> پورٹ_Map;
        لكل (ثابت خودکار& tconnect : tconnects) {
            پورٹ_Map[tconnect.پہلا] = tconnect.دوجا->exprp();
        }

        // Promote property-local variables to module-level temps (IEEE 16.10).
        std::unordered_map<ثابت AstVar*, AstVar*> localVarMap;
        لكل (Ast_گرہ* stmtp = propyp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (!varp->isIO()) {
                    ثابت string newName = m_prop_Var_ناں.get(varp);
                    AstVar* ثابت newVarp = جديد AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->جوڑو_Stmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        propSpecp->foreach([&](AstVarRef* refp) {
            ثابت خودکار پورٹ_It = پورٹ_Map.لبھو(refp->varp());
            إذا (پورٹ_It != پورٹ_Map.سرا()) {
                refp->replaceWith(پورٹ_It->دوجا->clone_درخت(خطأ));
                VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
                أرجع;
            }
            ثابت خودکار localIt = localVarMap.لبھو(refp->varp());
            إذا (localIt != localVarMap.سرا()) refp->varp(localIt->دوجا);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        لكل (ثابت خودکار& tconnect : tconnects) {
            ڈالو_Deletep(tconnect.دوجا->exprp()->unlink_Fr_پٹھ());
        }
        V3AssertCommon::checkSvaInstanceDelays(propSpecp, funcrefp);

        // Merge disable iff (IEEE 1800-2023 16.12.1)
        إذا (outerSpecp->disablep() && propSpecp->disablep()) {
            outerSpecp->v3error("disable iff expression before property call "
                                "and in its body is not legal");
            ڈالو_Deletep(propSpecp->disablep()->unlink_Fr_پٹھ());
        }
        إذا (outerSpecp->disablep()) {
            propSpecp->disablep(outerSpecp->disablep()->unlink_Fr_پٹھ());
        }

        إذا (outerSpecp->sensesp() && propSpecp->sensesp()) {
            outerSpecp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Clock event before property call and in its body");
            ڈالو_Deletep(propSpecp->sensesp()->unlink_Fr_پٹھ());
        }
        إذا (outerSpecp->sensesp()) {
            Ast_Sen_شے* ثابت sensesp = outerSpecp->sensesp();
            sensesp->unlink_Fr_پٹھ();
            propSpecp->sensesp(sensesp);
        }

        outerSpecp->replaceWith(propSpecp);
        VL_DO_DANGLING(ڈالو_Deletep(outerSpecp), outerSpecp);
    }

    فراغ inlineSequenceRef(AstFuncRef* funcrefp, AstSequence* seqp) {
        Ast_گرہ_اظہاریہ* ثابت bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        Ast_گرہ_اظہاریہ* ثابت clonedp = bodyExprp->clone_درخت(خطأ);

        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_گرہ_اظہاریہ*> پورٹ_Map;
        لكل (ثابت خودکار& tconnect : tconnects) {
            پورٹ_Map[tconnect.پہلا] = tconnect.دوجا->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ثابت خودکار it = پورٹ_Map.لبھو(refp->varp());
            إذا (it != پورٹ_Map.سرا()) {
                refp->replaceWith(it->دوجا->clone_درخت(خطأ));
                VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
            }
        });
        لكل (ثابت خودکار& tconnect : tconnects) {
            ڈالو_Deletep(tconnect.دوجا->exprp()->unlink_Fr_پٹھ());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ڈالو_Deletep(funcrefp), funcrefp);
        // Clear referenced flag so V3AssertPre cleanup does not emit
        // spurious UNSUPPORTED for sequences that were already inlined here.
        seqp->isReferenced(خطأ);
    }

    // Must run before hasMultiCycleExpr() so NFA sees sequence bodies.
    فراغ inlineAllSequenceRefs(Ast_گرہ* rootp) {
        منطقي بدلیا = سچ;
        طالما (بدلیا) {
            بدلیا = خطأ;
            rootp->foreach([&](AstFuncRef* funcrefp) {
                إذا (بدلیا) أرجع;
                إذا (AstSequence* ثابت seqp = VN_CAST(funcrefp->taskp(), Sequence)) {
                    inlineSequenceRef(funcrefp, seqp);
                    بدلیا = سچ;
                }
            });
        }
    }

    ساكن منطقي has_Multi_چکر_اظہاریہ(ثابت Ast_گرہ* nodep) {
        أرجع nodep->exists([](ثابت Ast_گرہ* np) {
            إذا (ثابت خودکار* ثابت ep = VN_CAST(np, گرہ_اظہاریہ)) أرجع ep->is_Multi_چکر_Sva();
            أرجع خطأ;
        });
    }

    ساكن VPropStrength effectiveAssertPropStrength(ثابت AstPropSpec* ثابت propSpecp) {
        إذا (propSpecp->propStrength() != VPropStrength::DEFAULT) أرجع propSpecp->propStrength();
        أرجع propSpecp->fileline()->language() <= V3_Lang_کوڈ::L1800_2005 ? VPropStrength::STRONG
                                                                           : VPropStrength::WEAK;
    }

    // Bare `assert property (p until q)` with boolean operands stays on
    // V3AssertPre's AstLoop lowering, which preserves per-attempt action-block
    // firings that this NFA's single-bit aggregated state cannot. Strong bare
    // forms are also lowered there. NFA still owns sequence operands and any
    // embedding inside a multi-cycle context (implication consequent, or/and
    // operands, etc.).
    ساكن منطقي is_Bare_چوٹی_Level_Until(Ast_گرہ* propp) {
        Ast_گرہ* p = propp;
        إذا (AstPropSpec* ثابت specp = VN_CAST(p, PropSpec)) p = specp->propp();
        طالما (AstLogNot* ثابت notp = VN_CAST(p, LogNot)) p = notp->lhsp();
        AstUntil* ثابت untilp = VN_CAST(p, Until);
        إذا (!untilp) أرجع خطأ;
        ثابت خودکار hasSeq = [](ثابت Ast_گرہ_اظہاریہ* ep) {
            أرجع ep->exists([](ثابت Ast_گرہ_اظہاریہ* np) { أرجع np->is_Multi_چکر_Sva(); });
        };
        إذا (hasSeq(untilp->lhsp()) || hasSeq(untilp->rhsp())) أرجع خطأ;
        أرجع سچ;
    }

    بنية PropertyParts final {
        Ast_گرہ_اظہاریہ* triggerExprp = خالی_سوچک;
        Ast_گرہ_اظہاریہ* seqExprp = خالی_سوچک;
        منطقي isOverlapped = سچ;
        منطقي hasImplication = خطأ;
        منطقي isFollowedBy = خطأ;  // True for #-# / #=# (non-vacuous-fail on antecedent miss)
    };

    ساكن PropertyParts decomposeProperty(Ast_گرہ* propp) {
        PropertyParts parts;
        إذا (AstPropSpec* ثابت specp = VN_CAST(propp, PropSpec)) { propp = specp->propp(); }
        إذا (AstImplication* ثابت implp = VN_CAST(propp, Implication)) {
            parts.hasImplication = سچ;
            parts.isOverlapped = implp->isOverlapped();
            parts.isFollowedBy = implp->isFollowedBy();
            parts.triggerExprp = implp->lhsp();
            parts.seqExprp = implp->rhsp();
        } وإلا إذا (Ast_گرہ_اظہاریہ* ثابت exprp = VN_CAST(propp, گرہ_اظہاریہ)) {
            parts.triggerExprp = خالی_سوچک;
            parts.seqExprp = exprp;
        }
        أرجع parts;
    }

    ساكن منطقي can_ونڈو_Implication_Pass_Actions(ثابت PropertyParts& parts) {
        UASSERT(parts.hasImplication,
                "Implication pass action split requested without implication");
        UASSERT(parts.triggerExprp, "Implication pass action split requested without trigger");
        // Direct vacuous/nonvacuous classification uses the antecedent value in the current
        // assertion attempt. Leave delayed antecedents on the existing NFA pass path.
        أرجع !has_Multi_چکر_اظہاریہ(parts.triggerExprp);
    }

    ساكن فراغ ونڈو_Implication_Pass_Actions(AstAssert* assertp, ثابت PropertyParts& parts,
                                            Ast_گرہ_اظہاریہ* nonvacuousMatchp) {
        File_سطر* ثابت flp = assertp->fileline();

        Ast_گرہ* ثابت passsp = assertp->passsp()->unlink_Fr_پٹھ_With_اگلا();
        Ast_گرہ* splitsp = خالی_سوچک;

        إذا (!parts.isFollowedBy) {
            Ast_گرہ_اظہاریہ* ثابت vacuousp
                = جديد AstLogNot{flp, sampled(parts.triggerExprp->clone_درخت_خالص(خطأ))};
            Ast_گرہ* ثابت vacuousBodyp = passsp->clone_درخت(خطأ);
            splitsp = newPassOnIf(flp, vacuousp, vacuousBodyp, assertp->userType(),
                                  assertp->directive(), /*vacuous=*/سچ);
        }

        AstIf* ثابت nonvacuousp
            = newPassOnIf(flp, nonvacuousMatchp, passsp, assertp->userType(), assertp->directive(),
                          /*vacuous=*/خطأ);
        splitsp = splitsp ? Ast_گرہ::جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(splitsp, nonvacuousp)
                          : static_cast<Ast_گرہ*>(nonvacuousp);
        assertp->جوڑو_Passsp(splitsp);
    }

    // On a PropSpec-wrapped assertion whose NFA build failed with a semantic
    // error (errorEmitted), replace the body with a BitFalse const so later
    // passes see a well-formed AST. Returns true if replaced.
    فراغ replaceBodyOnBuildError(File_سطر* flp, AstPropSpec* propSpecp, منطقي errorEmitted) {
        إذا (!errorEmitted) أرجع;
        Ast_گرہ* ثابت innerPropp = propSpecp->propp();
        innerPropp->replaceWith(جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}});
        VL_DO_DANGLING(ڈالو_Deletep(innerPropp), innerPropp);
    }

    // Hoist a leading clocking event (IEEE 1800-2023 16.7):
    منطقي hoistClockedSeq(AstPropSpec* specp) {
        طالما (AstSClocked* ثابت clockedp = VN_CAST(specp->propp(), SClocked)) {
            إذا (specp->sensesp()) {
                clockedp->v3warn(E_UNSUPPORTED, "Unsupported: multiclocked sequence or property");
                replaceBodyOnBuildError(specp->fileline(), specp, سچ);
                أرجع سچ;
            }
            لكل (ثابت Ast_Sen_شے* sp = clockedp->sensesp(); sp;
                 sp = VN_CAST(sp->nextp(), Sen_شے)) {
                إذا (!sp->کنارا_Type().an_کنارا()) {
                    clockedp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: non-edge clocking event on a sequence; "
                                     "use an edge such as @(posedge clk)");
                    replaceBodyOnBuildError(specp->fileline(), specp, سچ);
                    أرجع سچ;
                }
            }
            specp->sensesp(clockedp->sensesp()->unlink_Fr_پٹھ_With_اگلا());
            Ast_گرہ_اظہاریہ* ثابت bodyp = clockedp->exprp()->unlink_Fr_پٹھ();
            clockedp->replaceWith(bodyp);
            VL_DO_DANGLING(ڈالو_Deletep(clockedp), clockedp);
        }
        // A clocking event anywhere else in the sequence is not supported.
        ثابت AstSClocked* nestedp = خالی_سوچک;
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
    Build_نتیجہ build_Assertion_گراف(SvaNfaBuilder& builder, Sva_گراف& گراف, Ast_گرہ_اظہاریہ* seqBodyp,
                                    ثابت PropertyParts& parts, File_سطر* flp) {
        إذا (!parts.hasImplication) أرجع builder.build(seqBodyp);

        گراف.m_شروع_Vertexp = گراف.create_State_راس();
        أرجع builder.build_Implication_کنارے(parts.triggerExprp, seqBodyp, گراف.m_شروع_Vertexp,
                                             parts.isOverlapped, parts.isFollowedBy,
                                             parts.triggerExprp, flp);
    }

    // Install pass-action gating and replay simultaneous per-thread failures.
    فراغ attachActionHandlers(AstAssert* ثابت assertp, Ast_گرہ_اظہاریہ* میل_Exprp,
                              Ast_Sen_درخت* ثابت threadFailReplaySenTreep,
                              Ast_گرہ_اظہاریہ* ثابت threadFailCountp) {
        // Gate pass handler on match to prevent vacuous-pass firings.
        إذا (میل_Exprp) {
            // needMatch implies passsp() was non-null when evaluated above;
            // lowering does not mutate the assert's pass-action between the
            // two reads, so passsp() is still non-null here.
            Ast_گرہ* passsp = assertp->passsp();
            UASSERT_OBJ(passsp, assertp, "needMatch set but passsp is null");
            passsp->unlink_Fr_پٹھ_With_اگلا();
            File_سطر* ثابت flp = assertp->fileline();
            AstIf* ثابت ifp = جديد AstIf{flp, میل_Exprp, passsp, خالی_سوچک};
            assertp->جوڑو_Passsp(ifp);
            // Fail-handler prefix for overlapping instances (IEEE 16.12):
            // fires when reject=1 && match=1 in the same cycle.
            إذا (assertp->failsp()) assertp->prependFailsp(ifp->clone_درخت(خطأ));
        }

        إذا (threadFailCountp) {
            UASSERT_OBJ(threadFailReplaySenTreep, assertp,
                        "Thread fail count missing sensitivity tree");
            Ast_گرہ* ثابت failsp = assertp->failsp();
            File_سطر* ثابت flp = assertp->fileline();
            // IEEE 1800-2023 16.12 requires one action-block evaluation per failed
            // thread. AstAssert handles the first, so replay the rest here.
            AstVar* ثابت بچیا_Fail_گنتی_Varp
                = جديد AstVar{flp, VVarType::MODULETEMP, m_fail_گنتی_ناں.get(""),
                             m_modp->لبھو_Basic_DType(VBasicDTypeKwd::UINT32)};
            بچیا_Fail_گنتی_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جوڑو_Stmtsp(بچیا_Fail_گنتی_Varp);
            Ast_گرہ* ثابت replayStmtsp = جديد Ast_سونپو{
                flp, جديد AstVarRef{flp, بچیا_Fail_گنتی_Varp, VAccess::WRITE}, threadFailCountp};
            AstLoop* ثابت replayLoopp = جديد AstLoop{flp};
            replayLoopp->جوڑو_Stmtsp(جديد AstLoopTest{
                flp, replayLoopp,
                جديد AstGt{flp, جديد AstVarRef{flp, بچیا_Fail_گنتی_Varp, VAccess::READ},
                          newTypedConstp(flp, بچیا_Fail_گنتی_Varp->dtypep(), 1)}});
            replayLoopp->جوڑو_Stmtsp(newIfAssertFailOn(failsp->clone_درخت(سچ), assertp->directive(),
                                                     assertp->userType()));
            AstSub* ثابت decrementedFailCountp
                = جديد AstSub{flp, جديد AstVarRef{flp, بچیا_Fail_گنتی_Varp, VAccess::READ},
                             newTypedConstp(flp, بچیا_Fail_گنتی_Varp->dtypep(), 1)};
            replayLoopp->جوڑو_Stmtsp(
                جديد Ast_سونپو{flp, جديد AstVarRef{flp, بچیا_Fail_گنتی_Varp, VAccess::WRITE},
                              decrementedFailCountp});
            replayStmtsp->جوڑو_اگلا(replayLoopp);
            m_modp->جوڑو_Stmtsp(
                جديد AstAlways{flp, VAlwaysKwd::ALWAYS, threadFailReplaySenTreep, replayStmtsp});
        }
    }

    // Replace one VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). No-op if refp is not in matchMap.
    فراغ substitute_میل_شے_Ref(AstVarRef* refp, بنانشانی K,
                                ثابت std::unordered_map<ثابت AstVar*, Ast_گرہ_اظہاریہ*>& میل_Map) {
        ثابت خودکار it = میل_Map.لبھو(refp->varp());
        إذا (it == میل_Map.سرا()) أرجع;
        Ast_گرہ_اظہاریہ* newp = it->دوجا->clone_درخت_خالص(خطأ);
        إذا (K > 0) {
            AstConst* ثابت ticksp = جديد AstConst{refp->fileline(), AstConst::Widthed_قدر{}, 32,
                                                  static_cast<uint32_t>(K)};
            AstPast* ثابت pastp = جديد AstPast{refp->fileline(), newp, ticksp, خالی_سوچک,
                                               /* propertyTiming */ سچ};
            pastp->dtypeFrom(newp);
            newp = pastp;
        }
        refp->replaceWith(newp);
        VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
        أرجع;
    }

    // Recursively walk a consequent. Returns cycle length consumed and
    // substitutes each VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). Reports E_UNSUPPORTED on non-constant
    // delays or composite sequence operators and returns -1 on failure.
    صحيح
    walk_Substitute_میل_اشیاء(Ast_گرہ_اظہاریہ* nodep, بنانشانی K,
                             ثابت std::unordered_map<ثابت AstVar*, Ast_گرہ_اظہاریہ*>& میل_اشیاء) {
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            // IEEE 1800-2023 16.9.2: cycle_delay's lhsp is a constant_expression
            // and the delay form in a sequence is always `##N`, folded by
            // V3Const + V3Param before V3AssertNfa. Range form `##[m:n]` is the
            // only user-visible reject here.
            AstDelay* ثابت delayp = VN_AS(sexprp->delayp(), Delay);
            UASSERT_OBJ(delayp->is_چکر_Delay() && VN_IS(delayp->lhsp(), Const), sexprp,
                        "SVA cycle delay must have a constant lhsp");
            إذا (delayp->isRangeDelay()) {
                sexprp->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                              "non-constant cycle delay in consequent"
                                              " (IEEE 1800-2023 16.10)");
                أرجع -1;
            }
            ثابت بنانشانی delay_چکراں = VN_AS(delayp->lhsp(), Const)->منزل_UInt();
            صحيح preLen = 0;
            إذا (Ast_گرہ_اظہاریہ* ثابت prep = sexprp->preExprp()) {
                preLen = walk_Substitute_میل_اشیاء(prep, K, میل_اشیاء);
                إذا (preLen < 0) أرجع -1;
            }
            ثابت صحيح bodyLen
                = walk_Substitute_میل_اشیاء(sexprp->exprp(), K + preLen + delay_چکراں, میل_اشیاء);
            إذا (bodyLen < 0) أرجع -1;
            أرجع preLen + delay_چکراں + bodyLen;
        }
        إذا (nodep->is_Multi_چکر_Sva()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                         "composite sequence operator in consequent"
                                         " (IEEE 1800-2023 16.10)");
            أرجع -1;
        }
        std::سمتیہ<AstVarRef*> refs;
        nodep->foreach([&refs](AstVarRef* p) { refs.ڈالو_پٹھ(p); });
        لكل (AstVarRef* ثابت refp : refs) substitute_میل_شے_Ref(refp, K, میل_اشیاء);
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
    منطقي lift_میل_شے_Substitutions(PropertyParts& parts, Ast_گرہ_اظہاریہ* seqBodyp) {
        إذا (!parts.hasImplication) أرجع خطأ;
        Ast_اظہاریہ_Stmt* ثابت اظہاریہ_Stmtp = VN_CAST(parts.triggerExprp, اظہاریہ_Stmt);
        إذا (!اظہاریہ_Stmtp) أرجع خطأ;
        // IEEE 1800-2023 16.10 BNF requires `(expr, match_item {, match_item})`
        // with at least one match item; V3LinkParse only emits ExprStmt for
        // this form and only emits AstAssign with VarRef LHS for each item.
        std::unordered_map<ثابت AstVar*, Ast_گرہ_اظہاریہ*> میل_اشیاء;
        لكل (Ast_گرہ* stmtp = اظہاریہ_Stmtp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_سونپو* ثابت assignp = VN_AS(stmtp, سونپو);
            AstVarRef* ثابت lhsRefp = VN_AS(assignp->lhsp(), VarRef);
            میل_اشیاء[lhsRefp->varp()] = assignp->rhsp();
        }
        ثابت بنانشانی شروع_K = parts.isOverlapped ? 0 : 1;
        ثابت صحيح لمائی = walk_Substitute_میل_اشیاء(seqBodyp, شروع_K, میل_اشیاء);
        // Match-item substitution / strip mutates ancestor purity. Release
        // builds don't auto-clear caches on edits, so refresh here.
        VIsCached::clear_Cache_درخت();
        إذا (لمائی < 0) أرجع سچ;
        Ast_گرہ_اظہاریہ* ثابت antBoolp = اظہاریہ_Stmtp->resultp()->unlink_Fr_پٹھ();
        اظہاریہ_Stmtp->replaceWith(antBoolp);
        VL_DO_DANGLING(ڈالو_Deletep(اظہاریہ_Stmtp), اظہاریہ_Stmtp);
        parts.triggerExprp = antBoolp;
        أرجع خطأ;
    }

    // Outcome counts for a property if/case are wrong in an outcome-multiplying
    // context. Returns that context, or nullptr when the shape is supported.
    ساكن ثابت حرف* unsupportedPropertyControl(ثابت Ast_گرہ_Cover_Or_Assert* assertp,
                                                  ثابت Ast_گرہ_اظہاریہ* seqBodyp, منطقي negated) {
        إذا (!hasPropertyControlConjunction(seqBodyp)) أرجع خالی_سوچک;
        إذا (negated) أرجع "negation";
        إذا (VN_IS(assertp, Cover)) أرجع "cover";
        إذا (VN_AS(assertp, Assert)->passsp()) أرجع "a pass action";
        أرجع خالی_سوچک;
    }

    // Inline property/sequence refs and reject unsupported shapes.
    // Returns the PropSpec to lower, or nullptr when fully handled here.
    AstPropSpec* prepareAssertionProp(Ast_گرہ_Cover_Or_Assert* assertp) {
        إذا (AstPropSpec* ثابت specp = VN_CAST(assertp->propp(), PropSpec)) {
            إذا (AstFuncRef* ثابت funcrefp = VN_CAST(specp->propp(), FuncRef)) {
                إذا (ثابت AstProperty* ثابت propyp = VN_CAST(funcrefp->taskp(), Property)) {
                    inlineNamedProperty(specp, funcrefp, propyp);
                }
            }
        }

        inlineAllSequenceRefs(assertp->propp());

        إذا (AstPropSpec* ثابت specp = VN_CAST(assertp->propp(), PropSpec)) {
            إذا (hoistClockedSeq(specp)) أرجع خالی_سوچک;
        }

        AstPropSpec* ثابت propp = VN_AS(assertp->propp(), PropSpec);
        إذا (!VN_IS(assertp, Cover)
            && effectiveAssertPropStrength(propp) == VPropStrength::STRONG) {
            propp->v3warn(E_UNSUPPORTED,
                          "Unsupported: strong property in " + assertp->verilogKwd() + ".");
            replaceBodyOnBuildError(assertp->fileline(), propp, /*errorEmitted=*/سچ);
            أرجع خالی_سوچک;
        }

        إذا (!has_Multi_چکر_اظہاریہ(propp)) أرجع خالی_سوچک;
        // A nested property instance keeps its body behind the call; lowering would drop it.
        إذا (propp->exists([](ثابت AstFuncRef* refp) { أرجع VN_IS(refp->taskp(), Property); })) {
            assertp->v3warn(E_UNSUPPORTED,
                            "Unsupported: property instance inside a multi-cycle property "
                            "expression");
            VL_DO_DANGLING(ڈالو_Deletep(assertp->unlink_Fr_پٹھ()), assertp);
            أرجع خالی_سوچک;
        }
        إذا (is_Bare_چوٹی_Level_Until(propp)) أرجع خالی_سوچک;
        أرجع propp;
    }

    فراغ processAssertion(Ast_گرہ_Cover_Or_Assert* assertp) {
        إذا (assertp->فوری()) أرجع;

        AstPropSpec* ثابت propp = prepareAssertionProp(assertp);
        إذا (!propp) أرجع;

        PropertyParts parts = decomposeProperty(propp);
        UASSERT_OBJ(parts.seqExprp, propp, "Property body must be an expression");

        // Unwrap `not` (IEEE 1800-2023 16.12.1); odd count -> negated semantics.
        Ast_گرہ_اظہاریہ* seqBodyp = parts.seqExprp;
        منطقي negated = خطأ;
        طالما (AstLogNot* ثابت notp = VN_CAST(seqBodyp, LogNot)) {
            negated = !negated;
            seqBodyp = notp->lhsp();
        }

        ثابت حرف* ثابت propertyControlp
            = unsupportedPropertyControl(assertp, seqBodyp, negated);

        // Substitute property-local match-item refs in consequent with
        // $past(rhs, K) before NFA build (IEEE 1800-2023 16.10).
        إذا (lift_میل_شے_Substitutions(parts, seqBodyp)) {
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
        ثابت منطقي گنتی_Matches = isCoverSeq && !isSeqEvent;
        // Inherit module defaults (IEEE 14.12, 16.15) when assertion has none.
        إذا (!propp->sensesp() && m_defaultClockingp) {
            propp->sensesp(m_defaultClockingp->sentreep()->sensesp()->clone_درخت(سچ));
        }
        إذا (!propp->disablep() && m_defaultDisablep && !isSeqEvent) {
            propp->disablep(m_defaultDisablep->condp()->clone_درخت_خالص(سچ));
        }
        إذا (!propp->sensesp()) أرجع;
        Ast_Sen_درخت* senTreep
            = جديد Ast_Sen_درخت{propp->fileline(), propp->sensesp()->clone_درخت(سچ)};
        Ast_گرہ_اظہاریہ* ثابت disableExprp = propp->disablep();

        // NFA lowering clones repeated operands and may hoist them into an
        // always_comb block. Resolve implicit sampled-value clocks first, while
        // the enclosing assertion clock is still available.
        {
            VL_RESTORER(m_sampled_قدر_Clockp);
            m_sampled_قدر_Clockp = senTreep;
            iterate(propp->propp());
        }

        File_سطر* ثابت flp = assertp->fileline();

        Sva_گراف گراف;
        SvaNfaBuilder builder{گراف,      m_modp,     m_prop_عارضی_ناں,
                              isCoverSeq, isSeqEvent, coverp != خالی_سوچک};

        ثابت Build_نتیجہ نتیجہ = build_Assertion_گراف(builder, گراف, seqBodyp, parts, flp);
        إذا (نتیجہ.valid()) wire_میل_And_Mid_Sources(گراف, نتیجہ, flp);
        إذا (!نتیجہ.valid()) {
            // Fall through to V3AssertPre for unsupported constructs; only
            // replace the body on real semantic errors. Any hoisted temps
            // from this attempt become orphan MODULETEMPs; V3Dead removes
            // them along with the dead always_comb driver.
            replaceBodyOnBuildError(flp, propp, نتیجہ.errorEmitted);
            VL_DO_DANGLING(ڈالو_Deletep(senTreep), senTreep);
            أرجع;
        }
        // After the build, so a construct the builder rejects reports itself.
        إذا (propertyControlp) {
            seqBodyp->v3warn(E_UNSUPPORTED,
                             "Unsupported: temporal property if/case with " << propertyControlp);
            replaceBodyOnBuildError(flp, propp, /*errorEmitted=*/سچ);
            VL_DO_DANGLING(ڈالو_Deletep(senTreep), senTreep);
            أرجع;
        }
        إذا (گنتی_Matches && !v3Global.opt.coverageUser()) {
            SvaNfaBuilder::freeUnlinkedCondp(نتیجہ.finalCondp);
            VL_DO_DANGLING(ڈالو_Deletep(senTreep), senTreep);
            Ast_گرہ* ثابت innerPropp = propp->propp();
            innerPropp->replaceWith(جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}});
            VL_DO_DANGLING(ڈالو_Deletep(innerPropp), innerPropp);
            أرجع;
        }

        AstAssert* ثابت assertAssertp = VN_CAST(assertp, Assert);
        ثابت منطقي ونڈو_Implication_Passsp = assertAssertp && assertAssertp->passsp()
                                            && parts.hasImplication
                                            && can_ونڈو_Implication_Pass_Actions(parts);
        ثابت منطقي need_میل = assertAssertp && assertAssertp->passsp()
                               && (!parts.hasImplication || ونڈو_Implication_Passsp);

        ثابت منطقي needsThreadFailReplay
            = assertAssertp && assertAssertp->failsp() && !parts.hasImplication;
        ثابت خودکار اشارے
            = m_loweringp->ہیٹھلا(assertp, گراف, senTreep, نتیجہ.finalCondp, disableExprp,
                                 needsThreadFailReplay, needsThreadFailReplay && !negated);
        Ast_گرہ_اظہاریہ* میل_Exprp = خالی_سوچک;
        Ast_گرہ_اظہاریہ* outputExprp = m_loweringp->assemble_نتیجہ(
            assertp, negated, نتیجہ.finalCondp, اشارے, need_میل ? &میل_Exprp : خالی_سوچک);
        إذا (گنتی_Matches) {
            UASSERT_OBJ(اشارے.میل_Countp, coverp, "Cover sequence missing match count");
            VL_DO_DANGLING(outputExprp->delete_درخت(), outputExprp);
            propp->میل_Countp(اشارے.میل_Countp);
            outputExprp = جديد AstNeq{flp, اشارے.میل_Countp->clone_درخت_خالص(خطأ),
                                     newTypedConstp(flp, اشارے.میل_Countp->dtypep(), 0)};
        }

        Ast_Sen_درخت* ثابت threadFailReplaySenTreep
            = اشارے.threadFailCountp ? senTreep->clone_درخت(خطأ) : خالی_سوچک;

        VL_DO_DANGLING(ڈالو_Deletep(senTreep), senTreep);
        إذا (نتیجہ.finalCondp && !نتیجہ.finalCondp->backp()) ڈالو_Deletep(نتیجہ.finalCondp);

        إذا (ونڈو_Implication_Passsp) {
            ونڈو_Implication_Pass_Actions(assertAssertp, parts, میل_Exprp);
        } وإلا {
            attachActionHandlers(assertAssertp, میل_Exprp, threadFailReplaySenTreep,
                                 اشارے.threadFailCountp);
        }

        Ast_گرہ* ثابت innerPropp = propp->propp();
        innerPropp->replaceWith(outputExprp);
        VL_DO_DANGLING(ڈالو_Deletep(innerPropp), innerPropp);

        UINFO(4, "NFA converted assertion at " << flp << سطر_انت);

        إذا (dump_گراف_Level() >= 6) گراف.m_گراف.dumpDotFilePrefixed("assert-nfa");
    }

    // VISITORS
    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_loweringp);
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultDisablep);
        m_modp = nodep;
        // A class nested in a module is in the scope of the module's default clocking
        إذا (!VN_IS(nodep, Class)) m_defaultClockingp = خالی_سوچک;
        m_defaultDisablep = nodep->defaultDisablep();
        SvaNfaLowering lowering{nodep};
        m_loweringp = &lowering;
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstClocking* nodep) override {
        إذا (nodep->isDefault() && !m_defaultClockingp) m_defaultClockingp = nodep;
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_Gen_بلاک* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstDefaultDisable* nodep) override {}
    فراغ ویکھو(AstFell* nodep) override { ویکھو_Sampled_قدر(nodep); }
    فراغ ویکھو(AstPast* nodep) override { ویکھو_Sampled_قدر(nodep); }
    فراغ ویکھو(AstRose* nodep) override { ویکھو_Sampled_قدر(nodep); }
    فراغ ویکھو(AstStable* nodep) override { ویکھو_Sampled_قدر(nodep); }
    فراغ ویکھو(AstAssert* nodep) override { processAssertion(nodep); }
    فراغ ویکھو(AstCover* nodep) override { processAssertion(nodep); }
    فراغ ویکھو(AstRestrict* nodep) override {
        // Restrict property is ignored by simulators (IEEE 1800-2023 16.12.2).
        // Remove here so temporal SExpr don't leak to V3AssertPre.
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
    }
    فراغ ویکھو(AstAssertIntrinsic* nodep) override {}
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    واضح AssertNfaVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Top entry point

فراغ V3AssertNfa::assertNfaAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":" << سطر_انت);
    { AssertNfaVisitor{nodep}; }
    V3Global::dump_Check_Global_درخت("assertnfa", 0, dump_درخت_Either_Level() >= 3);
}
