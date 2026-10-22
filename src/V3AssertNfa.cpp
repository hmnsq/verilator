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

صنف Sva_State_رأس;

// Per-vertex algorithm data, stored via V3GraphVertex::userp() during lowering
بنية Sva_رأس_داده final {
    AstVar* stateVarp = اشاره_تهی;  // NBA state register for this vertex
    AstVar* delayRingVarp = اشاره_تهی;  // Packed occupancy bits or per-slot match counts
    AstVar* delay_Ring_سر_Varp
        = اشاره_تهی;  // Cached outgoing slot; isolates readers from in-place writes
    AstVar* delayRingIdxVarp = اشاره_تهی;  // Next slot written in delayRingVarp
    AstVar* delay_Ring_زنده_شمار_Varp = اشاره_تهی;  // Number of threads in delayRingVarp
    AstVar* delayRingWrappedVarp = اشاره_تهی;  // All slots written since the last clear
    AstVar* انجامشده_LVarp = اشاره_تهی;  // SAnd LHS done-latch
    AstVar* انجامشده_RVarp = اشاره_تهی;  // SAnd RHS done-latch
    Ast_گره_عبارت* stateSigp = اشاره_تهی;  // Combinational state signal; OWNED during lowering
    Ast_گره_عبارت* state_شمار_Sigp = اشاره_تهی;  // Cover-sequence match count; OWNED during lowering
    Ast_گره_عبارت* state_شمار_سرریز_Sigp = اشاره_تهی;  // Count overflow; OWNED during lowering
    منطقي delayRingCounts = خطأ;  // Ring slots hold cover-sequence match counts
    منطقي needsReg = خطأ;  // True if vertex has incoming clocked edge
};

// NFA state vertex -- one per NFA position in the sequence evaluation
صنف Sva_State_رأس final : عام V3_گراف_رأس {
    VL_RTTI_IMPL(Sva_State_رأس, V3_گراف_رأس)
عام:
    // True if this is the sequence-match terminal vertex
    منطقي m_is_تطابق = خطأ;
    // OWNED throughout-guard condition clones; IEEE 1800-2023 16.9.9
    std::وکتور<Ast_گره_عبارت*> m_throughoutConds;
    // Nonzero for a ring-buffer vertex for ## delays.
    منطقي m_isFixedDelayRing = خطأ;
    بیعلامت m_delay_Ring_اندازه = 0;  // Number of ring slots. Range: max-min+1.
    Ast_گره_عبارت* m_delayRingClearCondp = اشاره_تهی;  // local RHS for pure-boolean range
    Ast_گره_عبارت* m_delayRingAdvanceCondp = اشاره_تهی;  // Advance only when this condition holds
    منطقي m_replayAbortReject = خطأ;  // Compressed repetition needs per-thread abort replay
    // OWNED; enclosing-abort fire condition clearing state or suppressing guard rejection
    Ast_گره_عبارت* m_abortClearp = اشاره_تهی;
    // OWNED; reject-abort fire condition rejecting all represented live threads
    Ast_گره_عبارت* m_abortRejectp = اشاره_تهی;
    // Liveness terminal (IEEE weak semantics): reject must not fire from this source
    منطقي m_isUnbounded = خطأ;
    // Temporal sequence AND combiner; IEEE 1800-2023 16.9.5
    منطقي m_isAndCombiner = خطأ;
    Sva_State_رأس* m_andLhsTermp = اشاره_تهی;  // LHS sub-NFA terminal vertex
    Sva_State_رأس* m_andRhsTermp = اشاره_تهی;  // RHS sub-NFA terminal vertex
    Ast_گره_عبارت* m_andLhsCondp = اشاره_تهی;  // OWNED; LHS final condition (may be nullptr)
    Ast_گره_عبارت* m_andRhsCondp = اشاره_تهی;  // OWNED; RHS final condition (may be nullptr)
    // Reject sink for SAnd rejectOnFail wiring; not a state-signal source
    منطقي m_isRejectSink = خطأ;
    // In-window vertex of a strong s_always[m:n]: if its state is still set at
    // end-of-simulation the universal-quantifier window never completed, which is
    // a liveness failure (IEEE 1800-2023 16.12.11 strong semantics).
    منطقي m_strongPending = خطأ;
    // Ring introduced for the checked portion of a bounded strong s_always window.
    منطقي m_strongAlwaysRing = خطأ;

    // CONSTRUCTORS
    صریح Sva_State_رأس(V3_گراف* graphp)
        : V3_گراف_رأس{graphp} {}
    ~Sva_State_رأس() override {
        لكل (Ast_گره_عبارت* cp : m_throughoutConds) VL_DO_DANGLING(cp->delete_درخت(), cp);
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
        string name = "s" + cvt_مقصد_Str(color());
        إذا (m_delay_Ring_اندازه) {
            name += "\\n";
            name += m_isFixedDelayRing ? "fixed chain " : "range chain ";
            name += cvt_مقصد_Str(m_delay_Ring_اندازه) + " slots";
        }
        أرجع name;
    }
    string dotColor() ثابت override {
        إذا (m_is_تطابق) أرجع "red";
        إذا (m_delay_Ring_اندازه) أرجع "blue";
        إذا (m_isAndCombiner) أرجع "purple";
        أرجع "black";
    }
    // LCOV_EXCL_STOP
    // Access per-vertex algorithm data (valid only during lowering phase)
    Sva_رأس_داده* datap() ثابت { أرجع static_cast<Sva_رأس_داده*>(userp()); }
};

// NFA transition edge -- clocked (##1) or combinational link (##0)
صنف Sva_Trans_یال final : عام V3_گراف_یال {
    VL_RTTI_IMPL(Sva_Trans_یال, V3_گراف_یال)
عام:
    Ast_گره_عبارت* m_condp;  // Transition condition; nullptr = unconditional; OWNED
    منطقي m_consumes_چرخه;  // true = clocked edge (##1), false = link (##0/boolean)
    // Reject when source is active and condp is false; set only on
    // outermost required-step Link
    منطقي m_rejectOnFail = خطأ;
    // Optional dynamic condition vertex for m_rejectOnFail. Used when the
    // success condition is another NFA state rather than a static expression.
    Sva_State_رأس* m_condVtxp = اشاره_تهی;

    // CONSTRUCTORS
    Sva_Trans_یال(V3_گراف* graphp, V3_گراف_رأس* fromp, V3_گراف_رأس* بالاترین, Ast_گره_عبارت* condp,
                 منطقي consumes_چرخه)
        : V3_گراف_یال{graphp, fromp, بالاترین, /*weight=*/1}
        , m_condp{condp}
        , m_consumes_چرخه{consumes_چرخه} {}
    ~Sva_Trans_یال() override {
        إذا (m_condp) VL_DO_DANGLING(m_condp->delete_درخت(), m_condp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string dotLabel() ثابت override { أرجع m_consumes_چرخه ? "##1" : "link"; }
    string dotStyle() ثابت override { أرجع m_consumes_چرخه ? "" : "dashed"; }
    // LCOV_EXCL_STOP
    // Typed accessors for NFA vertices
    Sva_State_رأس* fromVtxp() ثابت { أرجع static_cast<Sva_State_رأس*>(fromp()); }
    Sva_State_رأس* مقصد_Vtxp() ثابت { أرجع static_cast<Sva_State_رأس*>(بالاترین()); }
};

// NFA graph container
صنف Sva_گراف final {
عام:
    V3_گراف m_گراف;  // Owns all vertices and edges
    Sva_State_رأس* m_شروع_Vertexp = اشاره_تهی;  // Trigger/start vertex
    Sva_State_رأس* m_تطابق_Vertexp = اشاره_تهی;  // Sequence-match terminal vertex

    // Create a new state vertex
    Sva_State_رأس* create_State_رأس() { أرجع جديد Sva_State_رأس{&m_گراف}; }
    // Create the match terminal vertex
    Sva_State_رأس* create_تطابق_رأس() {
        Sva_State_رأس* ثابت vtxp = create_State_رأس();
        vtxp->m_is_تطابق = درست;
        m_تطابق_Vertexp = vtxp;
        أرجع vtxp;
    }
    // Add a clocked transition edge (##1)
    Sva_Trans_یال* جمع_کن_Clocked_یال(Sva_State_رأس* fromp, Sva_State_رأس* بالاترین,
                                 Ast_گره_عبارت* condp = اشاره_تهی) {
        أرجع جديد Sva_Trans_یال{&m_گراف, fromp, بالاترین, condp, /*consumesCycle=*/درست};
    }
    // Add a combinational link (##0 / boolean condition)
    Sva_Trans_یال* جمع_کن_Link(Sva_State_رأس* fromp, Sva_State_رأس* بالاترین,
                          Ast_گره_عبارت* condp = اشاره_تهی) {
        أرجع جديد Sva_Trans_یال{&m_گراف, fromp, بالاترین, condp, /*consumesCycle=*/خطأ};
    }
    // Collect all edges into a flat vector for iteration.
    // Used by the lowering phase which needs global edge scans.
    std::وکتور<ثابت Sva_Trans_یال*> all_یالها() ثابت {
        std::وکتور<ثابت Sva_Trans_یال*> نتیجه;
        لكل (ثابت V3_گراف_رأس& vtxr : m_گراف.vertices()) {
            لكل (ثابت V3_گراف_یال& edger : vtxr.بیرون_یالها()) {
                نتیجه.افزودن_پشت(static_cast<ثابت Sva_Trans_یال*>(&edger));
            }
        }
        أرجع نتیجه;
    }
};

//######################################################################
// Builder result: terminal vertex + optional final condition (match Link condition).
بنية Build_نتیجه final {
    Sva_State_رأس* جمله_Vertexp;  // Primary terminal; contributes to both match and reject
    Ast_گره_عبارت* finalCondp;  // nullptr = unconditional
    // Mid-window sources for range delays (pure boolean RHS): match-only (isUnbounded)
    std::وکتور<Sva_State_رأس*> midSources;
    منطقي errorEmitted = خطأ;  // Builder already emitted specific error; skip generic
    // For cover_sequence: when true, midSources already enumerate every
    // end-of-match, so wireMatchAndMidSources must NOT add the main
    // termVtxp -> matchVertex Link (would double-count via the merge vertex).
    منطقي جمله_Is_Mid_ادغام = خطأ;
    منطقي valid() ثابت { أرجع جمله_Vertexp != اشاره_تهی; }
    ساكن Build_نتیجه fail(منطقي errored = خطأ) { أرجع {اشاره_تهی, اشاره_تهی, {}, errored}; }
    ساكن Build_نتیجه failWithError() { أرجع {اشاره_تهی, اشاره_تهی, {}, درست}; }
};

// Parser-marked SAnd of overlapped implications: a property if/else/case.
ساكن منطقي hasPropertyControlConjunction(ثابت Ast_گره_عبارت* nodep) {
    أرجع nodep->exists([](ثابت AstSAnd* andp) { أرجع andp->propertyControl(); });
}

ساكن AstConst* newTypedConstp(File_سطر* ثابت flp, ثابت Ast_گره_DType* ثابت dtypep,
                                ثابت uint32_t مقدار) {
    AstConst* ثابت constp = جديد AstConst{flp, AstConst::DTyped{}, dtypep};
    constp->رقم().مجموعه_Long(مقدار);
    أرجع constp;
}

ساكن Ast_گره_عبارت* sampled(Ast_گره_عبارت* exprp) {
    أرجع جديد AstSampled{exprp->fileline(), exprp, exprp->dtypep(), درست};
}

ساكن ثابت حرف* assertPassOnQuery(منطقي vacuous) {
    ساكن constexpr ثابت حرف* queries[2]
        = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
    أرجع queries[vacuous];
}

ساكن Ast_گره_عبارت* assertOnCond(File_سطر* flp, VAssertType type,
                                 VAssertDirectiveType directiveType) {
    إذا (!v3Global.opt.assertOn()) { أرجع جديد AstConst{flp, AstConst::بیت_False{}}; }
    أرجع جديد AstCExpr{flp, AstCExpr::خالص{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
}

ساكن Ast_گره_عبارت* assertKillGet(File_سطر* flp, VAssertType type,
                                  VAssertDirectiveType directiveType) {
    أرجع جديد AstCExpr{flp, AstCExpr::خالص{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_KILL", type, directiveType),
                        32};
}

ساكن string assert_Action_Control_پیشوند(VAssertDirectiveType directiveType) {
    ثابت صحيح controlled = !!(static_cast<صحيح>(directiveType)
                              & (static_cast<صحيح>(VAssertDirectiveType::ASSERT)
                                 | static_cast<صحيح>(VAssertDirectiveType::COVER)
                                 | static_cast<صحيح>(VAssertDirectiveType::ASSUME)));
    ثابت صحيح checkRuntime = controlled & static_cast<صحيح>(v3Global.opt.assertOn());
    أرجع "("s + std::مقصد_string(controlled ^ 1) + " || ("s + std::مقصد_string(checkRuntime)
           + " && "s;
}

ساكن Ast_گره_عبارت* assertPassOnCond(File_سطر* flp, VAssertType type,
                                     VAssertDirectiveType directiveType, منطقي vacuous) {
    أرجع جديد AstCExpr{
        flp, AstCExpr::خالص{},
        assert_Action_Control_پیشوند(directiveType)
            + V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType)
            + "))"s,
        1};
}

ساكن Ast_گره_عبارت* assertFailOnCond(File_سطر* flp, VAssertType type,
                                     VAssertDirectiveType directiveType) {
    أرجع جديد AstCExpr{
        flp, AstCExpr::خالص{},
        assert_Action_Control_پیشوند(directiveType)
            + V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType) + "))"s,
        1};
}

ساكن AstIf* newPassOnIf(File_سطر* flp, Ast_گره_عبارت* firep, Ast_گره* bodyp, VAssertType type,
                          VAssertDirectiveType directiveType, منطقي vacuous) {
    Ast_گره_عبارت* ثابت condp
        = جديد AstLogAnd{flp, firep, assertPassOnCond(flp, type, directiveType, vacuous)};
    AstIf* ثابت ifp = جديد AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(درست);
    ifp->user1(درست);
    أرجع ifp;
}

ساكن Ast_گره_Stmt* newIfAssertFailOn(Ast_گره* bodyp, VAssertDirectiveType directiveType,
                                      VAssertType type) {
    File_سطر* ثابت flp = bodyp->fileline();
    Ast_گره_عبارت* ثابت condp = assertFailOnCond(flp, type, directiveType);
    AstIf* ثابت ifp = جديد AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(درست);
    ifp->user1(درست);
    أرجع ifp;
}

//######################################################################
// NFA Builder

صنف SvaNfaBuilder final {
    Sva_گراف& m_گراف;  // NFA graph being built
    Ast_گره_Module* ثابت m_modp;  // Module to receive hoisted sampled-prop temps
    V3_Unique_نامها& m_prop_موقت_نامها;  // Module-shared temp-var name source
    std::وکتور<Ast_گره_عبارت*> m_temporal_Guard_پشته;  // Guards active across nested temporal states
    // Outer abort conditions, AND-ed as !cond into inner abort edges
    // (IEEE 1800-2023 16.12.14 outer-wraps-inner).
    std::وکتور<Ast_گره_عبارت*> m_outer_Abort_پشته;
    منطقي m_درون_Unbounded_Scope = خطأ;  // Sticky: nodes created after inherit liveness
    منطقي m_markStrongPending = خطأ;  // Mark new vertices as strong s_always in-window
    منطقي m_isCover = خطأ;  // Cover directives do not fail at end-of-simulation
    // IEEE 1800-2023 16.14.3 cover sequence: each end-of-match fires the action,
    // not just the first. Builder builds parallel-branch (no first-match-wins)
    // topology when true. Default false preserves cover_property semantics.
    منطقي m_isCoverSeq = خطأ;
    // Unsupported endpoint topology must reject, not ignore, or the wait hangs
    منطقي m_isSeqEvent = خطأ;
    منطقي m_درون_Sequence_پیشوند = خطأ;  // Prefix composition does not preserve every endpoint
    منطقي m_want_Bool_برگ = خطأ;  // Building a |-> / |=> or top-level body (see buildOrMerge)

    بنية RangeDelayRejectInfo final {
        Sva_State_رأس* startp = اشاره_تهی;
        بیعلامت range = 0;
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

    Ast_گره_عبارت* throughoutCond(Ast_گره_عبارت* پایه_Condp, File_سطر* flp) {
        إذا (m_temporal_Guard_پشته.خالی()) أرجع پایه_Condp;
        // AND all active temporal guards (supports nesting)
        // Each must use $sampled values.
        Ast_گره_عبارت* guardp = اشاره_تهی;
        لكل (Ast_گره_عبارت* ثابت condp : m_temporal_Guard_پشته) {
            Ast_گره_عبارت* ثابت clonep = sampled(condp->clone_درخت_خالص(خطأ));
            إذا (!guardp) {
                guardp = clonep;
            } وإلا {
                guardp = جديد AstLogAnd{flp, guardp, clonep};
            }
        }
        إذا (پایه_Condp) { guardp = جديد AstLogAnd{flp, پایه_Condp, guardp}; }
        أرجع guardp;
    }

    ساكن بیعلامت getConstUInt(Ast_گره_عبارت* exprp) {
        Ast_گره_عبارت* ثابت constp = V3Const::constifyEdit(exprp->clone_درخت_خالص(خطأ));
        ثابت AstConst* ثابت cp = VN_CAST(constp, Const);
        ثابت بیعلامت val = cp ? cp->مقصد_UInt() : 0;
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
    ساكن صحيح fixed_درازی(Ast_گره_عبارت* nodep) {
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
            إذا (!delayp || !delayp->is_چرخه_Delay()) أرجع -1;
            بیعلامت delay_چرخهها;
            إذا (delayp->isRangeDelay()) {
                إذا (delayp->isUnbounded()) أرجع -1;  // LCOV_EXCL_LINE
                ثابت بیعلامت minD = getConstUInt(delayp->lhsp());
                ثابت بیعلامت maxD = getConstUInt(delayp->rhsp());
                إذا (minD != maxD) أرجع -1;
                delay_چرخهها = minD;
            } وإلا {
                delay_چرخهها = getConstUInt(delayp->lhsp());
            }
            صحيح preLen = 0;
            إذا (Ast_گره_عبارت* ثابت prep = sexprp->preExprp()) {
                preLen = fixed_درازی(prep);
                إذا (preLen < 0) أرجع -1;  // LCOV_EXCL_LINE
            }
            ثابت صحيح bodyLen = fixed_درازی(sexprp->exprp());
            إذا (bodyLen < 0) أرجع -1;  // LCOV_EXCL_LINE
            أرجع preLen + delay_چرخهها + bodyLen;
        }
        إذا (AstSThroughout* ثابت throughp = VN_CAST(nodep, SThroughout)) {
            أرجع fixed_درازی(throughp->rhsp());
        }
        إذا (AstSOr* ثابت orp = VN_CAST(nodep, SOr)) {
            // Alternatives must share one end cycle; buildSWithin relies on
            // this to pair the OR with an SIntersect.
            ثابت صحيح lhsLen = fixed_درازی(orp->lhsp());
            ثابت صحيح rhsLen = fixed_درازی(orp->rhsp());
            إذا (lhsLen < 0 || rhsLen < 0 || lhsLen != rhsLen) أرجع -1;
            أرجع lhsLen;
        }
        إذا (AstSWithin* ثابت withinp = VN_CAST(nodep, SWithin)) {
            // `seq1 within seq2` ends at seq2's end cycle (IEEE 16.9.10).
            ثابت صحيح lhsLen = fixed_درازی(withinp->lhsp());
            ثابت صحيح rhsLen = fixed_درازی(withinp->rhsp());
            إذا (lhsLen < 0 || rhsLen < 0 || lhsLen > rhsLen) أرجع -1;
            أرجع rhsLen;
        }
        // LCOV_EXCL_START -- defensive: V3AssertPre rejects composite SVA ops
        // nested in an intersect arm before fixedLength runs (clock-context
        // resolution fails). Kept as a guard in case future parser relaxations
        // permit it.
        إذا (nodep->is_Multi_چرخه_Sva()) أرجع -1;
        // LCOV_EXCL_STOP
        // Plain boolean expression (no SVA constructs) -- 0 cycles.
        أرجع 0;
    }

    // Contiguous match-length range [lo,hi] for an operand whose length varies
    // from AT MOST ONE ranged cycle delay; {-1,-1} otherwise. Drives the
    // variable-length `intersect` lowering (IEEE 1800-2023 16.9.6): more than
    // one ranged delay would make the per-length realization ambiguous, so it
    // is reported unsupported rather than mis-paired.
    ساكن std::pair<صحيح, صحيح> درازی_Range(Ast_گره_عبارت* nodep) {
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
            إذا (!delayp || !delayp->is_چرخه_Delay()) أرجع {-1, -1};
            std::pair<صحيح, صحيح> delayRange;
            إذا (delayp->isRangeDelay()) {
                إذا (delayp->isUnbounded()) أرجع {-1, -1};
                ثابت بیعلامت minD = getConstUInt(delayp->lhsp());
                ثابت بیعلامت maxD = getConstUInt(delayp->rhsp());
                delayRange = {minD, maxD};
            } وإلا {
                ثابت بیعلامت d = getConstUInt(delayp->lhsp());
                delayRange = {d, d};
            }
            std::pair<صحيح, صحيح> preRange{0, 0};
            إذا (Ast_گره_عبارت* ثابت prep = sexprp->preExprp()) {
                preRange = درازی_Range(prep);
                إذا (preRange.اولی < 0) أرجع {-1, -1};
            }
            ثابت std::pair<صحيح, صحيح> bodyRange = درازی_Range(sexprp->exprp());
            إذا (bodyRange.اولی < 0) أرجع {-1, -1};
            ثابت صحيح variableParts = (preRange.اولی != preRange.دومی)
                                      + (delayRange.اولی != delayRange.دومی)
                                      + (bodyRange.اولی != bodyRange.دومی);
            إذا (variableParts > 1) أرجع {-1, -1};
            أرجع {preRange.اولی + delayRange.اولی + bodyRange.اولی,
                    preRange.دومی + delayRange.دومی + bodyRange.دومی};
        }
        إذا (AstSThroughout* ثابت throughp = VN_CAST(nodep, SThroughout)) {
            أرجع درازی_Range(throughp->rhsp());
        }
        إذا (nodep->is_Multi_چرخه_Sva()) أرجع {-1, -1};
        أرجع {0, 0};  // plain boolean -- 0 cycles
    }

    // Clone `operand` with its sole variable ranged cycle delay pinned so the
    // total match length is exactly `len`. `lo` is the operand's minimum length
    // (lengthRange().first). A fixed operand has no such delay and is returned
    // as a plain clone (callers only request its single achievable length).
    ساكن Ast_گره_عبارت* realize_At_درازی(Ast_گره_عبارت* عملوند, صحيح len, صحيح lo) {
        Ast_گره_عبارت* ثابت clonep = عملوند->clone_درخت_خالص(خطأ);
        AstDelay* rangeDelayp = اشاره_تهی;
        clonep->foreach([&](AstDelay* dp) {
            إذا (!rangeDelayp && dp->isRangeDelay() && !dp->isUnbounded()
                && getConstUInt(dp->lhsp()) != getConstUInt(dp->rhsp())) {
                rangeDelayp = dp;
            }
        });
        إذا (rangeDelayp) {
            File_سطر* ثابت flp = rangeDelayp->fileline();
            ثابت بیعلامت pinned = getConstUInt(rangeDelayp->lhsp()) + (len - lo);
            Ast_گره_عبارت* ثابت oldMinp = rangeDelayp->lhsp();
            oldMinp->replaceWith(جديد AstConst{flp, pinned});
            VL_DO_DANGLING(oldMinp->delete_درخت(), oldMinp);
            // Drop the max bound so it lowers as a fixed `##d`, not `##[d:d]`.
            Ast_گره* ثابت oldMaxp = rangeDelayp->rhsp()->unlink_Fr_پشت();
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
    AstVar* tryHoistSampled(Ast_گره_عبارت* exprp, File_سطر* flp, بیعلامت clone_شمار) {
        constexpr بیعلامت k_Hoist_آستانه = 2;
        إذا (clone_شمار < k_Hoist_آستانه) أرجع اشاره_تهی;
        AstVar* ثابت موقت_Varp
            = جديد AstVar{flp, VVarType::MODULETEMP, m_prop_موقت_نامها.get(exprp), exprp->dtypep()};
        m_modp->جمع_کن_Stmtsp(موقت_Varp);
        Ast_تخصیص_بده* ثابت assignp = جديد Ast_تخصیص_بده{flp, جديد AstVarRef{flp, موقت_Varp, VAccess::WRITE},
                                                 sampled(exprp->clone_درخت_خالص(خطأ))};
        m_modp->جمع_کن_Stmtsp(جديد AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, اشاره_تهی, assignp});
        أرجع موقت_Varp;
    }

    ساكن Ast_گره_عبارت* sampledRefOrClone(AstVar* hoistVarp, Ast_گره_عبارت* exprp, File_سطر* flp) {
        إذا (hoistVarp) أرجع جديد AstVarRef{flp, hoistVarp, VAccess::READ};
        أرجع sampled(exprp->clone_درخت_خالص(خطأ));
    }

    // Create vertex and inherit temporal guards from the current scope.
    Sva_State_رأس* scoped_Create_رأس() {
        Sva_State_رأس* ثابت vtxp = m_گراف.create_State_رأس();
        لكل (Ast_گره_عبارت* ثابت cp : m_temporal_Guard_پشته) {
            vtxp->m_throughoutConds.افزودن_پشت(cp->clone_درخت_خالص(خطأ));
        }
        إذا (m_درون_Unbounded_Scope) vtxp->m_isUnbounded = درست;
        إذا (m_markStrongPending) vtxp->m_strongPending = درست;
        أرجع vtxp;
    }

    // AND current temporal guards into every edge/link.
    Sva_Trans_یال* guardedLink(Sva_State_رأس* fromp, Sva_State_رأس* بالاترین, Ast_گره_عبارت* condp,
                              File_سطر* flp) {
        أرجع m_گراف.جمع_کن_Link(fromp, بالاترین, throughoutCond(condp, flp));
    }
    Sva_Trans_یال* guardedLink(Sva_State_رأس* fromp, Sva_State_رأس* بالاترین, File_سطر* flp) {
        أرجع m_گراف.جمع_کن_Link(fromp, بالاترین, throughoutCond(اشاره_تهی, flp));
    }
    Sva_Trans_یال* guarded_یال(Sva_State_رأس* fromp, Sva_State_رأس* بالاترین, Ast_گره_عبارت* condp,
                              File_سطر* flp) {
        أرجع m_گراف.جمع_کن_Clocked_یال(fromp, بالاترین, throughoutCond(condp, flp));
    }
    Sva_Trans_یال* guarded_یال(Sva_State_رأس* fromp, Sva_State_رأس* بالاترین, File_سطر* flp) {
        أرجع m_گراف.جمع_کن_Clocked_یال(fromp, بالاترین, throughoutCond(اشاره_تهی, flp));
    }

    Sva_State_رأس* جمع_کن_Delay_Chain(Sva_State_رأس* startp, بیعلامت اندازه, File_سطر* flp,
                                  منطقي isFixed = درست, Ast_گره_عبارت* clearCondp = اشاره_تهی,
                                  Ast_گره_عبارت* advanceCondp = اشاره_تهی) {
        إذا (isFixed && اندازه == 0) أرجع startp;
        UASSERT_OBJ(اندازه > 0, startp, "Delay chain needs at least one slot");
        إذا (isFixed && اندازه == 1 && !advanceCondp) {
            Sva_State_رأس* ثابت nextp = scoped_Create_رأس();
            guarded_یال(startp, nextp, flp);
            أرجع nextp;
        }
        Sva_State_رأس* ثابت ringVtxp = scoped_Create_رأس();
        ringVtxp->m_isFixedDelayRing = isFixed;
        ringVtxp->m_delay_Ring_اندازه = اندازه;
        إذا (clearCondp) {
            UASSERT_OBJ(!isFixed, startp, "Fixed delay cannot have a clear condition");
            ringVtxp->m_delayRingClearCondp = clearCondp->clone_درخت_خالص(خطأ);
        }
        ringVtxp->m_delayRingAdvanceCondp = advanceCondp;
        إذا (isFixed) {
            guarded_یال(startp, ringVtxp, flp);
        } وإلا {
            guardedLink(startp, ringVtxp, flp);
        }
        أرجع ringVtxp;
    }

    // Build NFA for an SExpr. finalCond = RHS (not yet added as a vertex).
    // isTopLevelStep: marks outermost required boolean check as rejectOnFail.
    // Apply a range delay `##[M:N]` to currentp. Returns true on success. On
    // failure, emits a diagnostic and returns false.
    منطقي applyRangeDelay(AstSExpr* sexprp, Sva_State_رأس*& currentp,
                         std::وکتور<Sva_State_رأس*>& midSources,
                         RangeDelayRejectInfo* rangeRejectInfop = اشاره_تهی) {
        File_سطر* ثابت flp = sexprp->fileline();
        AstDelay* ثابت delayp = VN_AS(sexprp->delayp(), Delay);
        Ast_گره_عبارت* ثابت rhsExprp = sexprp->exprp();
        ثابت بیعلامت minDelay = getConstUInt(delayp->lhsp());
        إذا (delayp->isUnbounded()) {
            // `##[M:$]`: wait M cycles, then retain every matured attempt in a
            // one-slot ring while also exposing the first eligible tick.
            currentp = جمع_کن_Delay_Chain(currentp, minDelay, flp);
            Sva_State_رأس* ثابت waitRingp = جمع_کن_Delay_Chain(currentp, 1, flp, خطأ);
            // The ring's live count is updated in NBA, so this link retains the previous
            // tick's live attempts alongside new arrivals for the next tick.
            guardedLink(waitRingp, waitRingp, flp);
            Sva_State_رأس* ثابت ادغام_Vtxp = scoped_Create_رأس();
            guardedLink(currentp, ادغام_Vtxp, flp);
            guardedLink(waitRingp, ادغام_Vtxp, flp);
            currentp = ادغام_Vtxp;
            currentp->m_isUnbounded = درست;  // Liveness, so no reject.
            m_درون_Unbounded_Scope = درست;
            أرجع درست;
        }
        ثابت بیعلامت maxDelay = getConstUInt(delayp->rhsp());
        إذا (minDelay == maxDelay) {
            currentp = جمع_کن_Delay_Chain(currentp, minDelay, flp);
            أرجع درست;
        }
        ثابت بیعلامت range = maxDelay - minDelay;
        currentp = جمع_کن_Delay_Chain(currentp, minDelay, flp);
        ثابت منطقي multi_چرخه_Rhs = rhsExprp->is_Multi_چرخه_Sva();
        ثابت Ast_گره_عبارت* ثابت preExprp = sexprp->preExprp();
        // Prefix and nested-sequence composition can lose endpoints or their multiplicity
        // required by cover sequence (IEEE 1800-2023 16.14.3), regardless of the range width.
        إذا (m_isCoverSeq
            && (multi_چرخه_Rhs || m_درون_Sequence_پیشوند
                || (preExprp && preExprp->is_Multi_چرخه_Sva()))) {
            warnEndpointUnsupported(flp, "this ranged cycle delay");
            أرجع خطأ;
        }
        إذا (m_درون_Sequence_پیشوند) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: Bounded ranged cycle delay in a sequence prefix");
            أرجع خطأ;
        }
        إذا (multi_چرخه_Rhs) {
            // Launch a candidate at every eligible tick. Candidate-local misses are not
            // assertion rejects while a later position can still match.
            إذا (rangeRejectInfop) {
                ثابت صحيح rhsLen = fixed_درازی(rhsExprp);
                إذا (rhsLen >= 0) *rangeRejectInfop = {currentp, range, rhsLen};
            }
            Sva_State_رأس* ثابت tailp = جمع_کن_Delay_Chain(currentp, range + 1U, flp, خطأ);
            Sva_State_رأس* ثابت ادغام_Vtxp = scoped_Create_رأس();
            ادغام_Vtxp->m_isUnbounded = درست;
            guardedLink(currentp, ادغام_Vtxp, flp);
            guardedLink(tailp, ادغام_Vtxp, flp);
            currentp = ادغام_Vtxp;
            m_درون_Unbounded_Scope = درست;
        } وإلا {
            // The first eligible tick is explicit. The ring retains attempts for the
            // remaining ticks and exposes the final tick for rejection. Only properties
            // clear on success, since cover sequence counts every endpoint.
            midSources.افزودن_پشت(currentp);
            currentp = جمع_کن_Delay_Chain(currentp, range + 1U, flp, خطأ,
                                     m_isCoverSeq ? اشاره_تهی : rhsExprp);
        }
        أرجع درست;
    }

    فراغ جمع_کن_Finite_Range_Reject(ثابت RangeDelayRejectInfo& info, ثابت Build_نتیجه& نتیجه,
                              File_سطر* flp) {
        إذا (!info.startp) أرجع;

        Sva_State_رأس* ثابت expiryVtxp
            = جمع_کن_Delay_Chain(info.startp, info.range + info.rhsLen, flp);
        Sva_State_رأس* ثابت expiryMatchp = scoped_Create_رأس();
        std::وکتور<Sva_State_رأس*> sources = نتیجه.midSources;
        sources.افزودن_پشت(نتیجه.جمله_Vertexp);
        لكل (Sva_State_رأس* ثابت srcp : sources) {
            Ast_گره_عبارت* ثابت condp
                = نتیجه.finalCondp ? sampled(نتیجه.finalCondp->clone_درخت_خالص(خطأ)) : اشاره_تهی;
            Sva_State_رأس* ثابت successNowp = scoped_Create_رأس();
            guardedLink(srcp, successNowp, condp, flp);
            // Retain a success through the latest deadline it can satisfy.
            Sva_State_رأس* ثابت historyp
                = جمع_کن_Delay_Chain(successNowp, info.range + 1U, flp, خطأ);
            guardedLink(successNowp, expiryMatchp, flp);
            guardedLink(historyp, expiryMatchp, flp);
        }

        Sva_State_رأس* ثابت sinkVtxp = m_گراف.create_State_رأس();
        sinkVtxp->m_isRejectSink = درست;
        Sva_Trans_یال* ثابت rejectp = m_گراف.جمع_کن_Link(expiryVtxp, sinkVtxp);
        rejectp->m_rejectOnFail = درست;
        rejectp->m_condVtxp = expiryMatchp;
    }

    Build_نتیجه buildSExpr(AstSExpr* sexprp, Sva_State_رأس* مدخل_Vtxp,
                           منطقي is_بالاترین_Level_گام = خطأ) {
        AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
        إذا (!delayp || !delayp->is_چرخه_Delay()) أرجع Build_نتیجه::fail();

        File_سطر* ثابت flp = sexprp->fileline();
        Ast_گره_عبارت* ثابت exprp = sexprp->exprp();

        // Handle LHS (preExpr)
        Sva_State_رأس* currentp = مدخل_Vtxp;
        إذا (Ast_گره_عبارت* ثابت preExprp = sexprp->preExprp()) {
            VL_RESTORER(m_درون_Sequence_پیشوند);
            m_درون_Sequence_پیشوند = درست;
            ثابت Build_نتیجه pre = build_عبارت(preExprp, currentp, is_بالاترین_Level_گام);
            إذا (!pre.valid()) أرجع Build_نتیجه::fail(pre.errorEmitted);  // LCOV_EXCL_LINE
            إذا (pre.finalCondp) {
                Sva_State_رأس* ثابت condVtxp = scoped_Create_رأس();
                Sva_Trans_یال* ثابت edgep = guardedLink(
                    pre.جمله_Vertexp, condVtxp, sampled(pre.finalCondp->clone_درخت_خالص(خطأ)), flp);
                إذا (is_بالاترین_Level_گام && !pre.جمله_Vertexp->m_isUnbounded && !m_درون_Unbounded_Scope) {
                    // Do not mark liveness sources: first boolean check is deferred.
                    edgep->m_rejectOnFail = درست;
                }
                freeUnlinkedCondp(pre.finalCondp);
                currentp = condVtxp;
            } وإلا {
                currentp = pre.جمله_Vertexp;
            }
        }

        // Handle delay
        std::وکتور<Sva_State_رأس*> rangeMidSources;
        RangeDelayRejectInfo rangeRejectInfo;
        ثابت منطقي جمع_کن_Range_Reject = is_بالاترین_Level_گام && !m_درون_Unbounded_Scope;
        إذا (delayp->isRangeDelay()) {
            إذا (!applyRangeDelay(sexprp, currentp, rangeMidSources,
                                 جمع_کن_Range_Reject ? &rangeRejectInfo : اشاره_تهی)) {
                أرجع Build_نتیجه::failWithError();
            }
        } وإلا {
            ثابت بیعلامت delay_چرخهها = getConstUInt(delayp->lhsp());
            currentp = جمع_کن_Delay_Chain(currentp, delay_چرخهها, flp);
        }

        // Multi-cycle RHS: recurse (only plain boolean is returned as finalCondp).
        إذا (exprp->is_Multi_چرخه_Sva()) {
            ثابت Build_نتیجه نتیجه = build_عبارت(exprp, currentp, is_بالاترین_Level_گام);
            إذا (نتیجه.valid()) جمع_کن_Finite_Range_Reject(rangeRejectInfo, نتیجه, flp);
            أرجع نتیجه;
        }
        أرجع {currentp, exprp, std::move(rangeMidSources)};
    }

    Build_نتیجه buildConsRep(AstSConsRep* repp, Sva_State_رأس* مدخل_Vtxp,
                             منطقي is_بالاترین_Level_گام = خطأ) {
        File_سطر* ثابت flp = repp->fileline();
        Ast_گره_عبارت* ثابت exprp = repp->exprp();
        // Multi-cycle expr in ConsRep not yet supported; bail to avoid invalid AST.
        إذا (exprp->is_Multi_چرخه_Sva()) {
            repp->v3warn(E_UNSUPPORTED, "Unsupported: multi-cycle sequence expression inside"
                                        " consecutive repetition (IEEE 1800-2023 16.9.2)");
            أرجع Build_نتیجه::failWithError();
        }
        ثابت بیعلامت minN = getConstUInt(repp->countp());

        // Sum sites across prefix + unbounded/range tail so one hoist covers
        // every check edge of this repetition.
        بیعلامت مجموع_Sites = minN;
        إذا (repp->unbounded()) {
            مجموع_Sites += 1;
        } وإلا إذا (repp->maxCountp()) {
            مجموع_Sites += getConstUInt(repp->maxCountp()) - minN;
        }
        AstVar* ثابت hoistVarp = tryHoistSampled(exprp, flp, مجموع_Sites);

        // Cover-sequence (IEEE 1800-2023 16.14.3): collect each end-of-match
        // position so they all fire the action, not just the merged terminal.
        std::وکتور<Sva_State_رأس*> consMidSources;

        Sva_State_رأس* currentp = مدخل_Vtxp;
        لكل (بیعلامت i = 0; i < minN; ++i) {
            // Keep the first repetition explicit, collapse all remaining checks into the ring.
            إذا (i == 1) {
                currentp = جمع_کن_Delay_Chain(currentp, minN - 1, flp);
                currentp->m_delayRingClearCondp
                    = جديد AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                currentp->m_replayAbortReject = درست;
                إذا (is_بالاترین_Level_گام) {
                    currentp->m_throughoutConds.افزودن_پشت(
                        sampledRefOrClone(hoistVarp, exprp, flp));
                }
                i = minN - 1;
            }
            // Every repetition in the minimum prefix is required.
            Sva_State_رأس* ثابت condVtxp = scoped_Create_رأس();
            Sva_Trans_یال* ثابت linkp
                = guardedLink(currentp, condVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            // Only an outermost required step rejects the property. Its first check is explicit,
            // later required checks reject through the ring's throughout guard.
            إذا (is_بالاترین_Level_گام && i == 0) linkp->m_rejectOnFail = درست;
            currentp = condVtxp;
        }
        // After minN: currentp is the first valid end-of-match position for [*m:n].
        إذا (m_isCoverSeq && (repp->unbounded() || repp->maxCountp())) {
            consMidSources.افزودن_پشت(currentp);
        }

        إذا (repp->unbounded()) {
            إذا (minN == 0) {
                Sva_State_رأس* ثابت waitVtxp = scoped_Create_رأس();
                guarded_یال(currentp, waitVtxp, flp);
                Sva_State_رأس* ثابت checkVtxp = scoped_Create_رأس();
                guardedLink(waitVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guarded_یال(checkVtxp, waitVtxp, flp);
                guardedLink(currentp, checkVtxp, flp);
                currentp = checkVtxp;
            } وإلا {
                Sva_State_رأس* ثابت loop_پشت_Vtxp = scoped_Create_رأس();
                guarded_یال(currentp, loop_پشت_Vtxp, flp);
                Sva_State_رأس* ثابت reCheckVtxp = scoped_Create_رأس();
                guardedLink(loop_پشت_Vtxp, reCheckVtxp, sampledRefOrClone(hoistVarp, exprp, flp),
                            flp);
                guarded_یال(reCheckVtxp, loop_پشت_Vtxp, flp);
                guardedLink(reCheckVtxp, currentp, flp);
            }
            currentp->m_isUnbounded = درست;
            m_درون_Unbounded_Scope = درست;
        } وإلا إذا (repp->maxCountp()) {
            ثابت بیعلامت maxN = getConstUInt(repp->maxCountp());
            Sva_State_رأس* ثابت ادغام_Vtxp = scoped_Create_رأس();
            guardedLink(currentp, ادغام_Vtxp, flp);
            بیعلامت دم_Min_N = minN;
            Sva_State_رأس* دم_Startp = currentp;
            إذا (minN == 0) {
                // Build the first optional iteration explicitly. Feeding the empty endpoint
                // directly into a range ring would incorrectly keep that match alive.
                Sva_State_رأس* ثابت بعدی_Vtxp = scoped_Create_رأس();
                guarded_یال(currentp, بعدی_Vtxp, flp);
                Sva_State_رأس* ثابت checkVtxp = scoped_Create_رأس();
                guardedLink(بعدی_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, ادغام_Vtxp, flp);
                إذا (m_isCoverSeq) consMidSources.افزودن_پشت(checkVtxp);
                دم_Startp = checkVtxp;
                دم_Min_N = 1;
            }
            إذا (maxN > دم_Min_N) {
                // Add tail-ring only if the tail is non-empty.
                Sva_State_رأس* ثابت بعدی_Vtxp
                    = جمع_کن_Delay_Chain(دم_Startp, maxN - دم_Min_N + 1, flp, خطأ);
                بعدی_Vtxp->m_delayRingClearCondp
                    = جديد AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                بعدی_Vtxp->m_replayAbortReject = درست;
                Sva_State_رأس* ثابت checkVtxp = scoped_Create_رأس();
                guardedLink(بعدی_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, ادغام_Vtxp, flp);
                إذا (m_isCoverSeq) consMidSources.افزودن_پشت(checkVtxp);
            }
            currentp = ادغام_Vtxp;
        }
        // finalCond = nullptr (already checked via Links)
        Build_نتیجه res;
        res.جمله_Vertexp = currentp;
        res.finalCondp = اشاره_تهی;
        res.midSources = std::move(consMidSources);
        // mergeVtxp is the OR of all the end-positions we already pushed to
        // midSources, so the main termVtxp -> matchVertex Link would duplicate.
        res.جمله_Is_Mid_ادغام = m_isCoverSeq && !res.midSources.خالی();
        أرجع res;
    }

    // always[lo:hi] / s_always[lo:hi] (IEEE 1800-2023 16.12.11).
    Build_نتیجه buildPropAlways(AstPropAlways* nodep, Sva_State_رأس* مدخل_Vtxp,
                                منطقي is_بالاترین_Level_گام = خطأ) {
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گره_عبارت* ثابت propp = nodep->propp();
        ثابت بیعلامت lo = getConstUInt(nodep->loBoundp());
        إذا (VN_IS(nodep->hiBoundp(), Unbounded)) {
            // Weak always [lo:$]: unbounded upper bound (IEEE 1800-2023 16.12.11).
            // p must hold at every clock tick at least lo cycles after the attempt
            // start; those ticks are not required to exist, so there is no
            // end-of-trace obligation (weak). The self-loop keeps the attempt live
            // every cycle; each observed cycle is a safety obligation, so a false p
            // rejects immediately.
            UASSERT_OBJ(!nodep->isStrong(), nodep, "Unbounded always must be weak (V3Width)");
            Sva_State_رأس* ثابت livep = جمع_کن_Delay_Chain(مدخل_Vtxp, lo, flp);
            livep->m_isUnbounded = درست;
            guarded_یال(livep, livep, flp);  // stay active every subsequent cycle
            Sva_State_رأس* ثابت sinkp = m_گراف.create_State_رأس();
            sinkp->m_isRejectSink = درست;
            Sva_Trans_یال* ثابت rejEdgep
                = guardedLink(livep, sinkp, sampled(propp->clone_درخت_خالص(خطأ)), flp);
            إذا (is_بالاترین_Level_گام) rejEdgep->m_rejectOnFail = درست;
            أرجع {livep, اشاره_تهی, {}};
        }
        ثابت بیعلامت hi = getConstUInt(nodep->hiBoundp());
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
        Sva_State_رأس* currentp = جمع_کن_Delay_Chain(مدخل_Vtxp, lo, flp);
        Sva_State_رأس* ثابت checkp = scoped_Create_رأس();
        Sva_Trans_یال* ثابت linkp
            = guardedLink(currentp, checkp, sampled(propp->clone_درخت_خالص(خطأ)), flp);
        إذا (is_بالاترین_Level_گام) linkp->m_rejectOnFail = درست;
        currentp = checkp;
        m_temporal_Guard_پشته.افزودن_پشت(propp);
        currentp = جمع_کن_Delay_Chain(currentp, hi - lo, flp);
        إذا (nodep->isStrong() && currentp->m_delay_Ring_اندازه) currentp->m_strongAlwaysRing = درست;
        m_temporal_Guard_پشته.برداشت_پشت();
        أرجع {currentp, propp, {}};
    }

    Build_نتیجه buildGotoRep(AstSGotoRep* repp, Sva_State_رأس* مدخل_Vtxp) {
        File_سطر* ثابت flp = repp->fileline();
        Ast_گره_عبارت* ثابت exprp = repp->exprp();
        ثابت بیعلامت minN = getConstUInt(repp->countp());
        إذا (minN == 0) أرجع Build_نتیجه::fail();
        ثابت منطقي hasMax = repp->maxCountp() != اشاره_تهی;
        ثابت بیعلامت maxN = hasMax ? getConstUInt(repp->maxCountp()) : minN;
        إذا (m_isCoverSeq) {
            // Several attempts may coalesce in the one-bit wait vertex before reaching the ring,
            // so a cover sequence action block could run too few times.
            warnEndpointUnsupported(flp, "a goto repetition");
            أرجع Build_نتیجه::failWithError();
        }

        AstVar* ثابت hoistVarp = tryHoistSampled(exprp, flp, 2);

        // The first guardedEdge is the ##1 before waiting for a match. In the wait state, false
        // takes the clocked self-loop, while true takes the zero-delay guardedLink on that tick.
        Sva_State_رأس* ثابت waitVtxp = scoped_Create_رأس();
        guarded_یال(مدخل_Vtxp, waitVtxp, flp);
        guarded_یال(waitVtxp, waitVtxp,
                    جديد AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)}, flp);
        Sva_State_رأس* currentp = scoped_Create_رأس();
        guardedLink(waitVtxp, currentp, sampledRefOrClone(hoistVarp, exprp, flp), flp);

        إذا (minN > 1) {
            Sva_State_رأس* ثابت ringVtxp = جمع_کن_Delay_Chain(
                currentp, minN - 1, flp, درست, اشاره_تهی, sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = درست;
            Sva_State_رأس* ثابت checkVtxp = scoped_Create_رأس();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            currentp = checkVtxp;
        }
        إذا (!hasMax) {
            currentp->m_isUnbounded = درست;  // [->N] waits unboundedly
            m_درون_Unbounded_Scope = درست;
            أرجع {currentp, اشاره_تهی, {}};
        }

        // [->M:N]: the range ring holds matches from M through N and advances
        // only on expr, preserving arbitrarily long gaps between occurrences.
        Sva_State_رأس* ثابت ادغام_Vtxp = scoped_Create_رأس();
        guardedLink(currentp, ادغام_Vtxp, flp);  // accept at match_M
        إذا (maxN > minN) {
            Sva_State_رأس* ثابت ringVtxp
                = جمع_کن_Delay_Chain(currentp, maxN - minN + 1, flp, خطأ, اشاره_تهی,
                                sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = درست;
            Sva_State_رأس* ثابت checkVtxp = scoped_Create_رأس();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            guardedLink(checkVtxp, ادغام_Vtxp, flp);
        }
        ادغام_Vtxp->m_isUnbounded = درست;  // [->M:N] still has unbounded waits between matches
        m_درون_Unbounded_Scope = درست;
        أرجع {ادغام_Vtxp, اشاره_تهی, {}};
    }

    // True if the sequence has a zero-minimum consecutive repetition, which may match empty
    ساكن منطقي hasZeroMinRep(ثابت Ast_گره_عبارت* seqp) {
        أرجع seqp->exists(
            [](ثابت AstSConsRep* repp) { أرجع getConstUInt(repp->countp()) == 0; });
    }

    // True if a leaf's condition has no implication or multi-cycle sequence in it
    // (a leaf can be e.g. 'not (x |-> y)')
    ساكن منطقي isPlainBoolean(ثابت Ast_گره_عبارت* condp) {
        أرجع !condp->exists(
            [](ثابت Ast_گره_عبارت* np) { أرجع np->is_Multi_چرخه_Sva() || VN_IS(np, Implication); });
    }

    // Build merge vertex for SOr / LogOr: both branches feed into one vertex.
    Build_نتیجه build_Or_ادغام(Ast_گره_عبارت* lhsp, Ast_گره_عبارت* rhsp, Sva_State_رأس* مدخل_Vtxp,
                             File_سطر* flp, منطقي want_Bool_برگ) {
        ثابت Build_نتیجه lhs = build_عبارت(lhsp, مدخل_Vtxp);
        ثابت Build_نتیجه rhs = build_عبارت(rhsp, مدخل_Vtxp);
        إذا (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع Build_نتیجه::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP
        // IEEE 1800-2023 16.14.3: a cover sequence counts every end-of-match. A
        // sequence operand of 'or' can end more than once, but only its final
        // end reaches the merge vertex below, so reject sequence operands rather
        // than under-count. Plain boolean disjunction has one end per cycle and
        // is handled by the OR-fold.
        ثابت منطقي booleanOnly = lhs.جمله_Vertexp == مدخل_Vtxp && lhs.finalCondp
                                 && rhs.جمله_Vertexp == مدخل_Vtxp && rhs.finalCondp;
        إذا (m_isCoverSeq && !booleanOnly) {
            warnEndpointUnsupported(flp, "a sequence operand of 'or'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع Build_نتیجه::failWithError();
        }
        إذا (booleanOnly && want_Bool_برگ) {
            إذا (isPlainBoolean(lhs.finalCondp) && isPlainBoolean(rhs.finalCondp)) {
                // As a |-> / |=> or top-level body, a disjunction of two plain booleans must
                // be able to reject: return it as a boolean finalCondp. The merge vertex
                // below drops the attempt silently when both operands are false.
                Ast_گره_عبارت* ثابت orCondp
                    = جديد AstLogOr{flp, lhs.finalCondp->clone_درخت_خالص(خطأ),
                                   rhs.finalCondp->clone_درخت_خالص(خطأ)};
                freeUnlinkedCondp(lhs.finalCondp);
                freeUnlinkedCondp(rhs.finalCondp);
                أرجع {مدخل_Vtxp, orCondp, {}};
            }
        }
        Sva_State_رأس* ثابت ادغام_Vtxp = scoped_Create_رأس();
        إذا (booleanOnly) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsp,
                        "Single-cycle SOr operands must have finalCondp");
            guardedLink(مدخل_Vtxp, ادغام_Vtxp,
                        جديد AstLogOr{flp, sampled(lhs.finalCondp->clone_درخت_خالص(خطأ)),
                                     sampled(rhs.finalCondp->clone_درخت_خالص(خطأ))},
                        flp);
        } وإلا {
            إذا (lhs.finalCondp) {
                guardedLink(lhs.جمله_Vertexp, ادغام_Vtxp,
                            sampled(lhs.finalCondp->clone_درخت_خالص(خطأ)), flp);
            } وإلا {
                guardedLink(lhs.جمله_Vertexp, ادغام_Vtxp, flp);
            }
            إذا (rhs.finalCondp) {
                guardedLink(rhs.جمله_Vertexp, ادغام_Vtxp,
                            sampled(rhs.finalCondp->clone_درخت_خالص(خطأ)), flp);
            } وإلا {
                guardedLink(rhs.جمله_Vertexp, ادغام_Vtxp, flp);
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        أرجع {ادغام_Vtxp, اشاره_تهی, {}};
    }

    // Build done-latch combiner for SAnd/SIntersect (IEEE 1800-2023 16.9.5).
    Build_نتیجه buildAndCombiner(Ast_گره_عبارت* lhsExprp, Ast_گره_عبارت* rhsExprp,
                                 Sva_State_رأس* مدخل_Vtxp, File_سطر* flp) {
        // Snapshot-restore scope so LHS liveness does not leak into RHS.
        ثابت منطقي savedScope = m_درون_Unbounded_Scope;
        ثابت Build_نتیجه lhs = build_عبارت(lhsExprp, مدخل_Vtxp);
        ثابت منطقي lhsScope = m_درون_Unbounded_Scope;
        m_درون_Unbounded_Scope = savedScope;
        ثابت Build_نتیجه rhs = build_عبارت(rhsExprp, مدخل_Vtxp);
        ثابت منطقي rhsScope = m_درون_Unbounded_Scope;
        m_درون_Unbounded_Scope = savedScope || lhsScope || rhsScope;
        إذا (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع Build_نتیجه::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP

        // Single-cycle operands: use boolean AND (done-latch would fire across cycles).
        // If both operands stayed at entry, they must be boolean leaves which
        // buildExpr returns with finalCondp=nodep (non-null).
        إذا (lhs.جمله_Vertexp == مدخل_Vtxp && rhs.جمله_Vertexp == مدخل_Vtxp) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsExprp,
                        "Single-cycle SAnd operands must have finalCondp");
            Ast_گره_عبارت* ثابت condp = جديد AstLogAnd{flp, lhs.finalCondp->clone_درخت_خالص(خطأ),
                                                     rhs.finalCondp->clone_درخت_خالص(خطأ)};
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع {مدخل_Vtxp, condp, {}};
        }
        // Range-delay mid-window sources in either sub-branch would need
        // to be folded into the latch's match-now signal, which the
        // current combiner does not support.
        إذا (!lhs.midSources.خالی() || !rhs.midSources.خالی()) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: ranged cycle delay in an operand of property 'and'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            أرجع Build_نتیجه::failWithError();
        }
        Sva_State_رأس* ثابت combVtxp = scoped_Create_رأس();
        combVtxp->m_isAndCombiner = درست;
        combVtxp->m_andLhsTermp = lhs.جمله_Vertexp;
        combVtxp->m_andRhsTermp = rhs.جمله_Vertexp;
        إذا (lhs.finalCondp) combVtxp->m_andLhsCondp = lhs.finalCondp->clone_درخت_خالص(خطأ);
        إذا (rhs.finalCondp) combVtxp->m_andRhsCondp = rhs.finalCondp->clone_درخت_خالص(خطأ);
        إذا (lhs.جمله_Vertexp->m_isUnbounded || rhs.جمله_Vertexp->m_isUnbounded) {
            combVtxp->m_isUnbounded = درست;
        }
        // Wire terminal-boolean rejects to a dedicated sink so each side can fail
        // the AND independently. Skip for liveness or single-cycle operands
        // (single-cycle termVertexp == entryVtxp would fire every cycle).
        إذا (!combVtxp->m_isUnbounded) {
            منطقي needSink = خطأ;
            ثابت منطقي lhs_Multi_چرخه = (lhs.جمله_Vertexp != مدخل_Vtxp);
            ثابت منطقي rhs_Multi_چرخه = (rhs.جمله_Vertexp != مدخل_Vtxp);
            إذا (lhs.finalCondp && lhs_Multi_چرخه && !lhs.جمله_Vertexp->m_isUnbounded) {
                needSink = درست;
            }
            إذا (rhs.finalCondp && rhs_Multi_چرخه && !rhs.جمله_Vertexp->m_isUnbounded) {
                needSink = درست;
            }
            إذا (needSink) {
                Sva_State_رأس* ثابت sinkVtxp = m_گراف.create_State_رأس();
                sinkVtxp->m_isRejectSink = درست;
                إذا (lhs.finalCondp && lhs_Multi_چرخه && !lhs.جمله_Vertexp->m_isUnbounded) {
                    Sva_Trans_یال* ثابت ep = m_گراف.جمع_کن_Link(
                        lhs.جمله_Vertexp, sinkVtxp, sampled(lhs.finalCondp->clone_درخت_خالص(خطأ)));
                    ep->m_rejectOnFail = درست;
                }
                إذا (rhs.finalCondp && rhs_Multi_چرخه && !rhs.جمله_Vertexp->m_isUnbounded) {
                    Sva_Trans_یال* ثابت ep = m_گراف.جمع_کن_Link(
                        rhs.جمله_Vertexp, sinkVtxp, sampled(rhs.finalCondp->clone_درخت_خالص(خطأ)));
                    ep->m_rejectOnFail = درست;
                }
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        أرجع {combVtxp, اشاره_تهی, {}};
    }

    // Lower `seq1 within seq2` (IEEE 1800-2023 16.9.10) as:
    //   (OR_{i in 0..slack} 1 ##i seq1 ##(slack-i) 1) intersect seq2
    // Both operands must have fixed length (no ranged cycle delays).
    // The OR lives inside a single SIntersect so one AndCombiner done-latch
    // serves all offsets; lifting it out would double-count matches where
    // multiple offsets accept on the same seq2 end cycle.
    Build_نتیجه buildSWithin(AstSWithin* nodep, Sva_State_رأس* مدخل_Vtxp,
                             منطقي is_بالاترین_Level_گام = خطأ) {
        ثابت صحيح innerLen = fixed_درازی(nodep->lhsp());
        ثابت صحيح outerLen = fixed_درازی(nodep->rhsp());
        إذا (innerLen < 0 || outerLen < 0) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: within with ranged cycle-delay operand");
            أرجع Build_نتیجه::failWithError();
        }
        إذا (innerLen > outerLen) {
            أرجع build_Never_تطابق_Intersect(
                nodep, مدخل_Vtxp, is_بالاترین_Level_گام,
                "the inner sequence is longer than the outer sequence");
        }
        File_سطر* ثابت flp = nodep->fileline();
        ثابت صحيح slack = outerLen - innerLen;
        Ast_گره_عبارت* innerOrp = اشاره_تهی;
        لكل (صحيح i = 0; i <= slack; ++i) {
            ثابت صحيح postPad = slack - i;
            Ast_گره_عبارت* branchp = nodep->lhsp()->clone_درخت_خالص(خطأ);
            إذا (i > 0) {
                AstConst* ثابت prePadp = جديد AstConst{flp, AstConst::بیت_True{}};
                AstDelay* ثابت delayp
                    = جديد AstDelay{flp, جديد AstConst{flp, static_cast<uint32_t>(i)}, درست};
                AstSExpr* ثابت wrapped = جديد AstSExpr{flp, prePadp, delayp, branchp};
                wrapped->dtype_مجموعه_بیت();
                branchp = wrapped;
            }
            إذا (postPad > 0) {
                AstConst* ثابت postTruep = جديد AstConst{flp, AstConst::بیت_True{}};
                AstDelay* ثابت delayp
                    = جديد AstDelay{flp, جديد AstConst{flp, static_cast<uint32_t>(postPad)}, درست};
                AstSExpr* ثابت wrapped = جديد AstSExpr{flp, branchp, delayp, postTruep};
                wrapped->dtype_مجموعه_بیت();
                branchp = wrapped;
            }
            innerOrp = innerOrp ? static_cast<Ast_گره_عبارت*>(جديد AstSOr{flp, innerOrp, branchp})
                                : branchp;
        }
        Ast_گره_عبارت* ثابت outerClonep = nodep->rhsp()->clone_درخت_خالص(خطأ);
        Ast_گره_عبارت* ثابت combinedp = جديد AstSIntersect{flp, innerOrp, outerClonep};
        Build_نتیجه نتیجه = build_عبارت(combinedp, مدخل_Vtxp, is_بالاترین_Level_گام);
        VL_DO_DANGLING(combinedp->delete_درخت(), combinedp);
        // When both operands are plain booleans, buildAndCombiner returns a
        // freshly-allocated AstAnd as finalCondp with no parent. Callers up
        // the chain clone-and-discard finalCondp, so leave it parent-less and
        // it leaks; anchor it in the graph now via an edge.
        إذا (نتیجه.valid() && نتیجه.finalCondp && !نتیجه.finalCondp->backp()) {
            Sva_State_رأس* ثابت wrapVtxp = scoped_Create_رأس();
            guardedLink(نتیجه.جمله_Vertexp, wrapVtxp, sampled(نتیجه.finalCondp), flp);
            نتیجه = {wrapVtxp, اشاره_تهی, نتیجه.midSources};
        }
        أرجع نتیجه;
    }

    // Collect the boolean leaf checks of a fixed-length sequence keyed by their
    // clock offset from the start. Returns false for anything other than nested
    // AstSExpr with fixed cycle delays over boolean leaves (e.g. throughout).
    ساكن منطقي flattenFixedSeq(Ast_گره_عبارت* nodep, صحيح پایه_جابه_جایی,
                                std::map<صحيح, std::وکتور<Ast_گره_عبارت*>>& بیرون) {
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
            إذا (!delayp || !delayp->is_چرخه_Delay() || delayp->isUnbounded()) أرجع خطأ;
            ثابت بیعلامت delay_چرخهها = getConstUInt(delayp->lhsp());
            إذا (delayp->isRangeDelay() && getConstUInt(delayp->rhsp()) != delay_چرخهها)
                أرجع خطأ;
            صحيح preLen = 0;
            إذا (Ast_گره_عبارت* ثابت prep = sexprp->preExprp()) {
                إذا (!flattenFixedSeq(prep, پایه_جابه_جایی, بیرون)) أرجع خطأ;
                preLen = fixed_درازی(prep);
                إذا (preLen < 0) أرجع خطأ;
            }
            أرجع flattenFixedSeq(sexprp->exprp(), پایه_جابه_جایی + preLen + delay_چرخهها, بیرون);
        }
        إذا (nodep->is_Multi_چرخه_Sva()) أرجع خطأ;
        بیرون[پایه_جابه_جایی].افزودن_پشت(nodep);
        أرجع درست;
    }

    // Conjoin two equal-length fixed sequences into one: at each clock offset
    // AND the boolean checks of both operands (IEEE 1800-2023 16.9.6 -- both
    // operands match the same window). Returns null if either operand is not a
    // plain fixed sequence of boolean leaves.
    ساكن Ast_گره_عبارت* conjoinFixedSeqs(Ast_گره_عبارت* lhsp, Ast_گره_عبارت* rhsp, File_سطر* flp) {
        std::map<صحيح, std::وکتور<Ast_گره_عبارت*>> checks;
        إذا (!flattenFixedSeq(lhsp, 0, checks) || !flattenFixedSeq(rhsp, 0, checks)) أرجع اشاره_تهی;
        إذا (checks.خالی()) أرجع اشاره_تهی;
        Ast_گره_عبارت* resultp = اشاره_تهی;
        صحيح prev_جابه_جایی = 0;
        لكل (ثابت خودکار& جابه_جایی_Checks : checks) {
            ثابت صحيح جابه_جایی = جابه_جایی_Checks.اولی;
            Ast_گره_عبارت* condp = اشاره_تهی;
            لكل (Ast_گره_عبارت* ثابت leafp : جابه_جایی_Checks.دومی) {
                Ast_گره_عبارت* ثابت clonep = leafp->clone_درخت_خالص(خطأ);
                إذا (!condp) {
                    condp = clonep;
                } وإلا {
                    condp = جديد AstLogAnd{flp, condp, clonep};
                    condp->dtype_مجموعه_بیت();
                }
            }
            إذا (!resultp) {
                إذا (جابه_جایی > 0) {
                    AstDelay* ثابت delayp = جديد AstDelay{
                        flp, جديد AstConst{flp, static_cast<uint32_t>(جابه_جایی)}, /*isCycle=*/درست};
                    resultp
                        = جديد AstSExpr{flp, جديد AstConst{flp, AstConst::بیت_True{}}, delayp, condp};
                    resultp->dtype_مجموعه_بیت();
                } وإلا {
                    resultp = condp;
                }
            } وإلا {
                AstDelay* ثابت delayp = جديد AstDelay{
                    flp, جديد AstConst{flp, static_cast<uint32_t>(جابه_جایی - prev_جابه_جایی)},
                    /*isCycle=*/درست};
                resultp = جديد AstSExpr{flp, resultp, delayp, condp};
                resultp->dtype_مجموعه_بیت();
            }
            prev_جابه_جایی = جابه_جایی;
        }
        أرجع resultp;
    }

    // `seq` is a simple ranged sequence `start ##[m:n] end` (start/end boolean,
    // start may be absent). Used to collapse a both-variable intersect to one
    // ranged delay.
    بنية SimpleRanged final {
        منطقي ok = خطأ;
        Ast_گره_عبارت* startp = اشاره_تهی;  // may be null (absent start)
        Ast_گره_عبارت* endp = اشاره_تهی;
    };
    ساكن SimpleRanged asSimpleRanged(Ast_گره_عبارت* nodep) {
        AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr);
        إذا (!sexprp) أرجع {};
        AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
        إذا (!delayp || !delayp->is_چرخه_Delay() || !delayp->isRangeDelay() || delayp->isUnbounded())
            أرجع {};
        إذا (getConstUInt(delayp->lhsp()) == getConstUInt(delayp->rhsp())) أرجع {};
        Ast_گره_عبارت* ثابت prep = sexprp->preExprp();
        إذا (prep && fixed_درازی(prep) != 0) أرجع {};
        إذا (fixed_درازی(sexprp->exprp()) != 0) أرجع {};
        أرجع {درست, prep, sexprp->exprp()};
    }

    // Build the NFA for a synthesized intersect lowering tree, then free it.
    // buildExpr returns the terminal condition (finalCondp) by reference into the
    // tree; detach a clone so the tree can be freed here. The graph already holds
    // clones/hoists of every edge condition, so nothing else dangles.
    Build_نتیجه build_From_Lowering_درخت(Ast_گره_عبارت* treep, Sva_State_رأس* مدخل_Vtxp,
                                      منطقي is_بالاترین_Level_گام) {
        Build_نتیجه نتیجه = build_عبارت(treep, مدخل_Vtxp, is_بالاترین_Level_گام);
        إذا (نتیجه.valid() && نتیجه.finalCondp) {
            نتیجه.finalCondp = نتیجه.finalCondp->clone_درخت_خالص(خطأ);
        }
        VL_DO_DANGLING(treep->delete_درخت(), treep);
        أرجع نتیجه;
    }

    // Empty common-length intersection -- unequal fixed lengths, or disjoint
    // ranged lengths. IEEE 1800-2023 16.9.6 requires both operands to match
    // over a window of the same length, so with no common length the intersect
    // simply never matches. This is legal (matching nothing), not an error, so
    // lower to a constant false rather than rejecting legal code.
    Build_نتیجه build_Never_تطابق_Intersect(Ast_گره_عبارت* nodep, Sva_State_رأس* مدخل_Vtxp,
                                         منطقي is_بالاترین_Level_گام, ثابت حرف* reason) {
        nodep->v3warn(NEVERMATCH, "Sequence can never match because " << reason << ".");
        Ast_گره_عبارت* ثابت falsep = جديد AstConst{nodep->fileline(), AstConst::بیت_False{}};
        أرجع build_From_Lowering_درخت(falsep, مدخل_Vtxp, is_بالاترین_Level_گام);
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
    Build_نتیجه buildVarLenIntersect(AstSIntersect* nodep, Sva_State_رأس* مدخل_Vtxp,
                                     منطقي is_بالاترین_Level_گام) {
        ثابت std::pair<صحيح, صحيح> lhsRange = درازی_Range(nodep->lhsp());
        ثابت std::pair<صحيح, صحيح> rhsRange = درازی_Range(nodep->rhsp());
        إذا (lhsRange.اولی < 0 || rhsRange.اولی < 0) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect with this variable-length operand");
            أرجع Build_نتیجه::failWithError();
        }
        ثابت صحيح lo = std::max(lhsRange.اولی, rhsRange.اولی);
        ثابت صحيح hi = std::min(lhsRange.دومی, rhsRange.دومی);
        إذا (lo > hi) {
            // Disjoint length ranges share no common length -> never matches.
            أرجع build_Never_تطابق_Intersect(nodep, مدخل_Vtxp, is_بالاترین_Level_گام,
                                            "intersect operands have no common length");
        }
        File_سطر* ثابت flp = nodep->fileline();
        إذا (lo == hi) {
            Ast_گره_عبارت* ثابت lp = realize_At_درازی(nodep->lhsp(), lo, lhsRange.اولی);
            Ast_گره_عبارت* ثابت rp = realize_At_درازی(nodep->rhsp(), lo, rhsRange.اولی);
            Ast_گره_عبارت* ثابت conjp = conjoinFixedSeqs(lp, rp, flp);
            VL_DO_DANGLING(lp->delete_درخت(), lp);
            VL_DO_DANGLING(rp->delete_درخت(), rp);
            إذا (!conjp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: intersect operand is not a plain boolean sequence");
                أرجع Build_نتیجه::failWithError();
            }
            أرجع build_From_Lowering_درخت(conjp, مدخل_Vtxp, is_بالاترین_Level_گام);
        }
        ثابت SimpleRanged sl = asSimpleRanged(nodep->lhsp());
        ثابت SimpleRanged sr = asSimpleRanged(nodep->rhsp());
        إذا (!sl.ok || !sr.ok) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect of two sequences that each vary in length over a"
                          " range with internal structure");
            أرجع Build_نتیجه::failWithError();
        }
        ثابت خودکار andBool = [&](Ast_گره_عبارت* ap, Ast_گره_عبارت* bp) -> Ast_گره_عبارت* {
            Ast_گره_عبارت* ثابت aClonep
                = ap ? ap->clone_درخت_خالص(خطأ) : جديد AstConst{flp, AstConst::بیت_True{}};
            Ast_گره_عبارت* ثابت bClonep
                = bp ? bp->clone_درخت_خالص(خطأ) : جديد AstConst{flp, AstConst::بیت_True{}};
            AstLogAnd* ثابت andp = جديد AstLogAnd{flp, aClonep, bClonep};
            andp->dtype_مجموعه_بیت();
            أرجع andp;
        };
        AstDelay* ثابت delayp = جديد AstDelay{flp, جديد AstConst{flp, static_cast<uint32_t>(lo)},
                                              /*isCycle=*/درست};
        delayp->rhsp(جديد AstConst{flp, static_cast<uint32_t>(hi)});
        AstSExpr* ثابت reducedp
            = جديد AstSExpr{flp, andBool(sl.startp, sr.startp), delayp, andBool(sl.endp, sr.endp)};
        reducedp->dtype_مجموعه_بیت();
        أرجع build_From_Lowering_درخت(reducedp, مدخل_Vtxp, is_بالاترین_Level_گام);
    }

    Build_نتیجه buildThroughout(AstSThroughout* nodep, Sva_State_رأس* مدخل_Vtxp,
                                منطقي is_بالاترین_Level_گام = خطأ) {
        // Mark entryVtxp so "cond false at tick 0" is detected as throughout-drop.
        مدخل_Vtxp->m_throughoutConds.افزودن_پشت(nodep->lhsp()->clone_درخت_خالص(خطأ));
        m_temporal_Guard_پشته.افزودن_پشت(nodep->lhsp());
        ثابت Build_نتیجه نتیجه = build_عبارت(nodep->rhsp(), مدخل_Vtxp, is_بالاترین_Level_گام);
        m_temporal_Guard_پشته.برداشت_پشت();
        أرجع نتیجه;
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
    Build_نتیجه buildUntil(AstUntil* nodep, Sva_State_رأس* مدخل_Vtxp, منطقي is_بالاترین_Level_گام) {
        File_سطر* ثابت flp = nodep->fileline();
        إذا (!is_بالاترین_Level_گام) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            أرجع Build_نتیجه::failWithError();
        }
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp();
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp();
        ثابت خودکار hasSeq = [](ثابت Ast_گره_عبارت* ep) {
            أرجع ep->exists([](ثابت Ast_گره_عبارت* np) { أرجع np->is_Multi_چرخه_Sva(); });
        };
        إذا (hasSeq(lhsp) || hasSeq(rhsp)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            أرجع Build_نتیجه::failWithError();
        }

        ثابت منطقي ov = nodep->isOverlapping();
        Ast_گره_عبارت* ثابت lhsBitp = nodep->lhsp();
        Ast_گره_عبارت* ثابت rhsBitp = nodep->rhsp();
        // p hoist count: continue, require (ov: 1 use; nov: 1 use). At least 2 uses.
        AstVar* ثابت pHoistp = tryHoistSampled(lhsBitp, flp, 2);
        // q hoist count: continue (1) + require nov (1) = 2; ov: continue only (1).
        AstVar* ثابت qHoistp = ov ? اشاره_تهی : tryHoistSampled(rhsBitp, flp, 2);

        Sva_State_رأس* ثابت waitCp = scoped_Create_رأس();
        Sva_State_رأس* ثابت waitRp = scoped_Create_رأس();
        waitCp->m_isUnbounded = درست;
        waitRp->m_strongPending = nodep->isStrong() && !m_isCover;

        // Entry and back-loop Links carry no condition; throughout-folding still applies.
        guardedLink(مدخل_Vtxp, waitCp, flp);
        guardedLink(waitRp, waitCp, flp);

        // Continue clocked edge: p && !q advances to next-cycle wait.
        Ast_گره_عبارت* ثابت contCondp
            = جديد AstLogAnd{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                            جديد AstLogNot{flp, sampledRefOrClone(qHoistp, rhsBitp, flp)}};
        guarded_یال(waitCp, waitRp, contCondp, flp);

        // Reject sink: fires when require-condition is false.
        Sva_State_رأس* ثابت sinkVtxp = m_گراف.create_State_رأس();
        sinkVtxp->m_isRejectSink = درست;
        Ast_گره_عبارت* requireCondp;
        إذا (ov) {
            requireCondp = sampledRefOrClone(pHoistp, lhsBitp, flp);
        } وإلا {
            requireCondp = جديد AstLogOr{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                                        sampledRefOrClone(qHoistp, rhsBitp, flp)};
        }
        Sva_Trans_یال* ثابت rejEdgep = m_گراف.جمع_کن_Link(waitCp, sinkVtxp, requireCondp);
        rejEdgep->m_rejectOnFail = درست;

        // Accept condition rides via finalCondp; assembleResult $sampled-wraps it.
        Ast_گره_عبارت* acceptCondp;
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
    Ast_گره_عبارت* abort_Fire_عبارت(Ast_گره_عبارت* condp, File_سطر* flp) {
        Ast_گره_عبارت* resultp = condp->clone_درخت_خالص(خطأ);
        لكل (Ast_گره_عبارت* ثابت op : m_outer_Abort_پشته)
            resultp = جديد AstLogAnd{flp, resultp, جديد AstLogNot{flp, op->clone_درخت_خالص(خطأ)}};
        أرجع resultp;
    }

    // True when unguarded ring-wide rejection or a same-tick Link chain already accounts the
    // attempt: a required-step Link covers both outcomes; followed-by pairs both edges.
    ساكن منطقي chain_Accounts_منبع(ثابت Sva_State_رأس* srcp,
                                    ثابت std::unordered_مجموعه<ثابت V3_گراف_یال*>& pre_یالها) {
        إذا (srcp->m_delay_Ring_اندازه && srcp->m_throughoutConds.خالی()) أرجع درست;
        لكل (ثابت V3_گراف_یال& edger : srcp->درون_یالها()) {
            إذا (pre_یالها.شمار(&edger)) استمر;
            ثابت Sva_Trans_یال& tedger = static_cast<ثابت Sva_Trans_یال&>(edger);
            إذا (tedger.m_consumes_چرخه) استمر;
            ثابت خودکار* ثابت fromp = static_cast<ثابت Sva_State_رأس*>(tedger.fromVtxp());
            إذا (fromp->m_abortRejectp) أرجع درست;
        }
        منطقي plainNonSink = خطأ;
        منطقي markedSink = خطأ;
        لكل (ثابت V3_گراف_یال& edger : srcp->بیرون_یالها()) {
            إذا (pre_یالها.شمار(&edger)) استمر;
            ثابت Sva_Trans_یال& tedger = static_cast<ثابت Sva_Trans_یال&>(edger);
            إذا (tedger.m_consumes_چرخه) استمر;
            ثابت منطقي sink = static_cast<ثابت Sva_State_رأس*>(tedger.مقصد_Vtxp())->m_isRejectSink;
            إذا (tedger.m_rejectOnFail) {
                إذا (!sink) أرجع درست;
                markedSink = درست;
            } وإلا إذا (!sink) {
                إذا (srcp->m_abortRejectp) أرجع درست;
                plainNonSink = درست;
            }
        }
        أرجع plainNonSink && markedSink;
    }

    // Reject edge: fires when the source is live and the abort samples true.
    فراغ جمع_کن_Abort_Reject_یال(Sva_State_رأس* srcp, Sva_State_رأس* sinkp, Ast_گره_عبارت* condp,
                            File_سطر* flp) {
        Ast_گره_عبارت* ثابت notFirep = جديد AstLogNot{flp, sampled(abort_Fire_عبارت(condp, flp))};
        m_گراف.جمع_کن_Link(srcp, sinkp, notFirep)->m_rejectOnFail = درست;
        أرجع;
    }

    // On the fire tick: kill body threads; accept kinds also forgive step misses.
    فراغ gate_Body_یالها_On_Abort(ثابت std::unordered_مجموعه<ثابت V3_گراف_یال*>& pre_یالها,
                              Ast_گره_عبارت* condp, VAbort_نوع نوع, File_سطر* flp) {
        لكل (V3_گراف_رأس& vtxr : m_گراف.m_گراف.vertices()) {
            لكل (V3_گراف_یال& edger : vtxr.بیرون_یالها()) {
                إذا (pre_یالها.شمار(&edger)) استمر;
                Sva_Trans_یال* ثابت tedgep = static_cast<Sva_Trans_یال*>(&edger);
                إذا (tedgep->m_rejectOnFail) {
                    إذا (!نوع.isAccept()) استمر;
                    Ast_گره_عبارت* ثابت firep = sampled(abort_Fire_عبارت(condp, flp));
                    tedgep->m_condp
                        = tedgep->m_condp ? جديد AstLogOr{flp, tedgep->m_condp, firep} : firep;
                } وإلا إذا (tedgep->m_consumes_چرخه) {
                    Ast_گره_عبارت* ثابت notFirep
                        = جديد AstLogNot{flp, sampled(abort_Fire_عبارت(condp, flp))};
                    tedgep->m_condp = tedgep->m_condp
                                          ? جديد AstLogAnd{flp, tedgep->m_condp, notFirep}
                                          : notFirep;
                }
            }
        }
    }

    Build_نتیجه buildAbortOn(Ast_گره_عبارت* condp, Ast_گره_عبارت* bodyp, Sva_State_رأس* مدخل_Vtxp,
                             VAbort_نوع نوع, File_سطر* flp, منطقي is_بالاترین_Level_گام) {
        // Snapshot pre-body vertices/edges so post-build diff yields the body's sub-NFA.
        std::unordered_مجموعه<ثابت V3_گراف_رأس*> preExisting;
        std::unordered_مجموعه<ثابت V3_گراف_یال*> pre_یالها;
        لكل (V3_گراف_رأس& vtxr : m_گراف.m_گراف.vertices()) {
            preExisting.درج(&vtxr);
            لكل (V3_گراف_یال& edger : vtxr.بیرون_یالها()) pre_یالها.درج(&edger);
        }

        m_outer_Abort_پشته.افزودن_پشت(condp);
        ثابت Build_نتیجه body_نتیجه = build_عبارت(bodyp, مدخل_Vtxp, is_بالاترین_Level_گام);
        m_outer_Abort_پشته.برداشت_پشت();
        إذا (!body_نتیجه.valid()) أرجع body_نتیجه;

        gate_Body_یالها_On_Abort(pre_یالها, condp, نوع, flp);

        // Live-thread sources for the abort edge: entry + new body vertices,
        // minus reject sinks (they carry reject fuel, not live-thread fuel).
        std::وکتور<Sva_State_رأس*> abortSources;
        abortSources.افزودن_پشت(مدخل_Vtxp);
        لكل (V3_گراف_رأس& vtxr : m_گراف.m_گراف.vertices()) {
            إذا (preExisting.شمار(&vtxr)) استمر;
            خودکار* ثابت sp = static_cast<Sva_State_رأس*>(&vtxr);
            إذا (sp->m_delay_Ring_اندازه || !sp->m_throughoutConds.خالی()) {
                Ast_گره_عبارت* ثابت firep = abort_Fire_عبارت(condp, flp);
                sp->m_abortClearp
                    = sp->m_abortClearp ? جديد AstLogOr{flp, sp->m_abortClearp, firep} : firep;
            }
            إذا (!نوع.isAccept()
                && ((sp->m_delay_Ring_اندازه && sp->m_throughoutConds.خالی())
                    || sp->m_replayAbortReject)) {
                Ast_گره_عبارت* ثابت firep = abort_Fire_عبارت(condp, flp);
                sp->m_abortRejectp
                    = sp->m_abortRejectp ? جديد AstLogOr{flp, sp->m_abortRejectp, firep} : firep;
            }
            إذا (sp->m_isRejectSink) استمر;
            abortSources.افزودن_پشت(sp);
        }

        خودکار sampledAbortFire = [&]() -> Ast_گره_عبارت* {
            Ast_گره_عبارت* ثابت عبارت = abort_Fire_عبارت(condp, flp);
            أرجع sampled(عبارت);
        };

        إذا (نوع.isAccept()) {
            // Match-only sink fed by $sampled(abort-fire) from every live source;
            // registered as midSource so it never contributes a reject.
            Sva_State_رأس* ثابت acceptSinkp = scoped_Create_رأس();
            لكل (Sva_State_رأس* ثابت srcp : abortSources)
                guardedLink(srcp, acceptSinkp, sampledAbortFire(), flp);
            std::وکتور<Sva_State_رأس*> midSources = body_نتیجه.midSources;
            midSources.افزودن_پشت(acceptSinkp);
            Ast_گره_عبارت* finalCondp = body_نتیجه.finalCondp;
            إذا (finalCondp) {
                إذا (finalCondp->backp()) finalCondp = finalCondp->clone_درخت_خالص(خطأ);
                finalCondp = جديد AstLogOr{flp, finalCondp, abort_Fire_عبارت(condp, flp)};
            }
            أرجع {body_نتیجه.جمله_Vertexp, finalCondp, std::move(midSources)};
        }

        // rejectOnFail treats m_condp as the success condition and fires on
        // !condp, so the edge carries !sampledAbortFire().
        Sva_State_رأس* ثابت rejectSinkp = m_گراف.create_State_رأس();
        rejectSinkp->m_isRejectSink = درست;
        لكل (Sva_State_رأس* ثابت srcp : abortSources)
            إذا (!chain_Accounts_منبع(srcp, pre_یالها))
                جمع_کن_Abort_Reject_یال(srcp, rejectSinkp, condp, flp);
        Ast_گره_عبارت* finalCondp = body_نتیجه.finalCondp;
        إذا (finalCondp) {
            إذا (finalCondp->backp()) finalCondp = finalCondp->clone_درخت_خالص(خطأ);
            finalCondp
                = جديد AstLogAnd{flp, finalCondp, جديد AstLogNot{flp, abort_Fire_عبارت(condp, flp)}};
        }
        أرجع {body_نتیجه.جمله_Vertexp, finalCondp, body_نتیجه.midSources};
    }

عام:
    // Free a dropped sub-result condition that is not linked into the AST
    // (abort folds synthesize unparented finalCondp trees).
    ساكن فراغ freeUnlinkedCondp(Ast_گره_عبارت* condp) {
        إذا (condp && !condp->backp()) VL_DO_DANGLING(condp->delete_درخت(), condp);
    }

    SvaNfaBuilder(Sva_گراف& گراف, Ast_گره_Module* modp, V3_Unique_نامها& prop_موقت_نامها,
                  منطقي isCoverSeq = خطأ, منطقي isSeqEvent = خطأ, منطقي isCover = خطأ)
        : m_گراف{گراف}
        , m_modp{modp}
        , m_prop_موقت_نامها{prop_موقت_نامها}
        , m_isCover{isCover}
        , m_isCoverSeq{isCoverSeq}
        , m_isSeqEvent{isSeqEvent} {}

    // Reset scope between antecedent and consequent: liveness must not leak.
    // m_outerAbortStack survives: an abort wrapping the implication covers the
    // consequent too (IEEE 1800-2023 16.12.14).
    فراغ resetScope() {
        m_درون_Unbounded_Scope = خطأ;
        m_temporal_Guard_پشته.clear();
    }

    Build_نتیجه build_عبارت(Ast_گره_عبارت* nodep, Sva_State_رأس* مدخل_Vtxp,
                          منطقي is_بالاترین_Level_گام = خطأ) {
        VL_RESTORER(m_want_Bool_برگ);
        ثابت منطقي want_Bool_برگ = m_want_Bool_برگ;
        m_want_Bool_برگ = خطأ;
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            أرجع buildSExpr(sexprp, مدخل_Vtxp, is_بالاترین_Level_گام);
        }
        إذا (AstSConsRep* ثابت repp = VN_CAST(nodep, SConsRep)) {
            أرجع buildConsRep(repp, مدخل_Vtxp, is_بالاترین_Level_گام);
        }
        إذا (AstPropAlways* ثابت alwaysp = VN_CAST(nodep, PropAlways)) {
            أرجع buildPropAlways(alwaysp, مدخل_Vtxp, is_بالاترین_Level_گام);
        }
        إذا (AstSGotoRep* ثابت repp = VN_CAST(nodep, SGotoRep)) {
            أرجع buildGotoRep(repp, مدخل_Vtxp);
        }
        إذا (AstSThroughout* ثابت throughoutp = VN_CAST(nodep, SThroughout)) {
            أرجع buildThroughout(throughoutp, مدخل_Vtxp, is_بالاترین_Level_گام);
        }
        إذا (AstSOr* ثابت orp = VN_CAST(nodep, SOr)) {
            أرجع build_Or_ادغام(orp->lhsp(), orp->rhsp(), مدخل_Vtxp, orp->fileline(),
                                want_Bool_برگ);
        }
        إذا (AstLogOr* ثابت orp = VN_CAST(nodep, LogOr)) {
            أرجع build_Or_ادغام(orp->lhsp(), orp->rhsp(), مدخل_Vtxp, orp->fileline(),
                                want_Bool_برگ);
        }
        إذا (AstSAnd* ثابت andp = VN_CAST(nodep, SAnd)) {
            أرجع buildAndCombiner(andp->lhsp(), andp->rhsp(), مدخل_Vtxp, andp->fileline());
        }
        إذا (AstSIntersect* ثابت intp = VN_CAST(nodep, SIntersect)) {
            // IEEE 1800-2023 16.9.6: both operands match over one window with
            // equal start and end (equal length). Lower to a single sequence
            // that conjoins both operands' per-cycle checks -- correct under
            // concurrent attempts, where the done-latch combiner conflates the
            // two operands' start times and over-accepts. The combiner remains
            // only as a fallback for operands that do not flatten.
            ثابت صحيح lhsLen = fixed_درازی(intp->lhsp());
            ثابت صحيح rhsLen = fixed_درازی(intp->rhsp());
            إذا (lhsLen >= 0 && rhsLen >= 0) {
                إذا (lhsLen != rhsLen) {
                    // Unequal fixed lengths share no common length -> never matches.
                    أرجع build_Never_تطابق_Intersect(intp, مدخل_Vtxp, is_بالاترین_Level_گام,
                                                    "intersect operands have no common length");
                }
                إذا (Ast_گره_عبارت* ثابت conjp
                    = conjoinFixedSeqs(intp->lhsp(), intp->rhsp(), intp->fileline())) {
                    أرجع build_From_Lowering_درخت(conjp, مدخل_Vtxp, is_بالاترین_Level_گام);
                }
                أرجع buildAndCombiner(intp->lhsp(), intp->rhsp(), مدخل_Vtxp, intp->fileline());
            }
            أرجع buildVarLenIntersect(intp, مدخل_Vtxp, is_بالاترین_Level_گام);
        }
        إذا (AstSWithin* ثابت withinp = VN_CAST(nodep, SWithin)) {
            أرجع buildSWithin(withinp, مدخل_Vtxp, is_بالاترین_Level_گام);
        }
        إذا (AstAbortOn* ثابت ap = VN_CAST(nodep, AbortOn)) {
            أرجع buildAbortOn(ap->condp(), ap->propp(), مدخل_Vtxp, ap->نوع(), ap->fileline(),
                                is_بالاترین_Level_گام);
        }
        إذا (VN_IS(nodep, SNonConsRep)) أرجع Build_نتیجه::fail();
        إذا (AstImplication* ثابت implp = VN_CAST(nodep, Implication)) {
            أرجع build_Implication_یالها(implp->lhsp(), implp->rhsp(), مدخل_Vtxp,
                                         implp->isOverlapped(), implp->isFollowedBy(),
                                         implp->lhsp(), implp->fileline());
        }
        إذا (AstUntil* ثابت untilp = VN_CAST(nodep, Until)) {
            أرجع buildUntil(untilp, مدخل_Vtxp, is_بالاترین_Level_گام);
        }
        // Boolean leaf (including LogAnd): return as finalCond
        أرجع {مدخل_Vtxp, nodep, {}};
    }

    // Wire an implication / followed-by from `entryVtxp`: builds the antecedent,
    // emits the match-link (and for followed-by the reject-sink edge), inserts a
    // delay vertex for non-overlapped forms, and builds the body. Used both for
    // nested AstImplication in pexpr position and for the top-level assertion
    // antecedent -- `errorNodep` anchors the "unsupported sequence antecedent"
    // error, which differs between the two call sites.
    Build_نتیجه build_Implication_یالها(Ast_گره_عبارت* antExprp, Ast_گره_عبارت* bodyExprp,
                                      Sva_State_رأس* مدخل_Vtxp, منطقي isOverlapped,
                                      منطقي isFollowedBy, Ast_گره* errorNodep, File_سطر* flp) {
        ثابت Build_نتیجه ant_نتیجه = build_عبارت(antExprp, مدخل_Vtxp);
        إذا (!ant_نتیجه.valid()) أرجع ant_نتیجه;

        // Followed-by requires pure-boolean antecedent for non-vacuous-fail at
        // the attempt-start cycle. IEEE 1800-2023 16.12.9 permits a multi-cycle
        // sequence LHS, so this is an implementation gap rather than illegal SV.
        إذا (isFollowedBy && ant_نتیجه.جمله_Vertexp != مدخل_Vtxp) {
            errorNodep->v3warn(E_UNSUPPORTED,
                               "Unsupported: sequence expression as antecedent of followed-by"
                               " (#-# / #=#) (IEEE 1800-2023 16.12.9)");
            أرجع Build_نتیجه::failWithError();
        }
        UASSERT_OBJ(!isFollowedBy || ant_نتیجه.finalCondp, errorNodep,
                    "followed-by antecedent terminal at entry must carry finalCondp");

        // Use raw createStateVertex() so trigVtxp starts without liveness --
        // reaching the antecedent terminal is a definitive event.
        Sva_State_رأس* ثابت trigVtxp = m_گراف.create_State_رأس();
        إذا (ant_نتیجه.finalCondp) {
            m_گراف.جمع_کن_Link(ant_نتیجه.جمله_Vertexp, trigVtxp,
                            sampled(ant_نتیجه.finalCondp->clone_درخت_خالص(خطأ)));
            // Followed-by non-vacuous fail: rejectOnFail fires when the attempt
            // is live (termVtx reachable) and sampled(antecedent) is false.
            إذا (isFollowedBy) {
                Sva_State_رأس* ثابت sinkVtxp = m_گراف.create_State_رأس();
                sinkVtxp->m_isRejectSink = درست;
                Sva_Trans_یال* ثابت ep
                    = m_گراف.جمع_کن_Link(ant_نتیجه.جمله_Vertexp, sinkVtxp,
                                      sampled(ant_نتیجه.finalCondp->clone_درخت_خالص(خطأ)));
                ep->m_rejectOnFail = درست;
            }
            // finalCondp is cloned into the Sampled nodes; if the original is
            // not parented anywhere in the AST anymore it must be freed here
            // or ASan flags it as a leak (e.g. t_sequence_bool_ops).
            إذا (!ant_نتیجه.finalCondp->backp()) {
                VL_DO_DANGLING(ant_نتیجه.finalCondp->delete_درخت(), ant_نتیجه.finalCondp);
            }
        } وإلا {
            m_گراف.جمع_کن_Link(ant_نتیجه.جمله_Vertexp, trigVtxp);
        }
        resetScope();

        Sva_State_رأس* bodyEntryp = trigVtxp;
        إذا (!isOverlapped) {
            Sva_State_رأس* ثابت delayVtxp = m_گراف.create_State_رأس();
            m_گراف.جمع_کن_Clocked_یال(trigVtxp, delayVtxp);
            bodyEntryp = delayVtxp;
        }
        // An empty antecedent match must not start the consequent, but a zero-minimum
        // repetition's empty match is not handled here; keep the merge vertex for those
        VL_RESTORER(m_want_Bool_برگ);
        m_want_Bool_برگ = !isFollowedBy && !hasZeroMinRep(antExprp);
        أرجع build_عبارت(bodyExprp, bodyEntryp, /*isTopLevelStep=*/درست);
    }

    Build_نتیجه build(Ast_گره_عبارت* exprp) {
        m_گراف.m_شروع_Vertexp = scoped_Create_رأس();
        VL_RESTORER(m_want_Bool_برگ);
        m_want_Bool_برگ = درست;
        أرجع build_عبارت(exprp, m_گراف.m_شروع_Vertexp, /*isTopLevelStep=*/درست);
    }
};

//######################################################################
// NFA Lowering (converts NFA graph to synthesizable AstAlways blocks)

صنف SvaNfaLowering final {
    Ast_گره_Module* ثابت m_modp;  // Module to add state vars and always blocks to
    Ast_گره_DType* ثابت m_u32DTypep;  // Shared unsigned counter dtype
    V3_Unique_نامها m_نامها{"__Vnfa"};
    اندازه_t m_stat_Delay_Ring_یال_Visits = 0;  // Delay-ring incoming edges visited

    // Per-lowering shared context (passed to phase sub-functions)
    // Per-vertex lowering state is stored in SvaVertexData and accessed via
    // V3GraphVertex::userp() (see SvaStateVertex::datap()).
    بنية پایینی_Ctx final {
        File_سطر* flp;  // Source location for generated AST
        Ast_Sen_درخت* senTreep;  // Clock sensitivity tree
        Ast_گره_عبارت* disableExprp;  // disable iff expression (may be nullptr)
        Ast_گره_عبارت* تطابق_Condp;  // Final boolean match condition (may be nullptr)
        VAssertType assertType;  // Assertion type for control tasks
        VAssertDirectiveType directiveType;  // Directive type for control tasks
        AstVar* killVarp;  // Last observed kill generation
        Sva_گراف& گراف;  // NFA graph
    };

    // Build a match-now expression: stateSig[i] && $sampled(condp)
    ساكن Ast_گره_عبارت* build_تطابق_Now(File_سطر* flp, Ast_گره_عبارت* stateExprp, Ast_گره_عبارت* condp) {
        Ast_گره_عبارت* ثابت statep = stateExprp->clone_درخت_خالص(خطأ);
        إذا (!condp) أرجع statep;
        أرجع جديد AstLogAnd{flp, statep, sampled(condp->clone_درخت_خالص(خطأ))};
    }
    ساكن Ast_گره_عبارت* andCond(File_سطر* flp, Ast_گره_عبارت* exprp, Ast_گره_عبارت* condp) {
        إذا (!condp) أرجع exprp;
        أرجع جديد AstLogAnd{flp, exprp, condp->clone_درخت_خالص(خطأ)};
    }
    // bp is always non-null; only ap can be null (serving as accumulator).
    ساكن Ast_گره_عبارت* orExprs(File_سطر* flp, Ast_گره_عبارت* ap, Ast_گره_عبارت* bp) {
        إذا (!ap) أرجع bp;
        أرجع جديد AstLogOr{flp, ap, bp};
    }
    ساكن Ast_گره_عبارت* جمع_کن_Countp(File_سطر* ثابت flp, Ast_گره_عبارت* ثابت مجموع_Countp,
                                  Ast_گره_عبارت* ثابت contributionp,
                                  Ast_گره_عبارت* ثابت enablep = اشاره_تهی) {
        // contribution = enable ? contribution : 0;
        Ast_گره_عبارت* ثابت enabledContributionp
            = enablep ? جديد AstCond{flp, enablep, contributionp,
                                    newTypedConstp(flp, contributionp->dtypep(), 0)}
                      : contributionp;
        إذا (!مجموع_Countp) أرجع enabledContributionp;
        أرجع جديد Ast_جمع_کن{flp, مجموع_Countp, enabledContributionp};
    }
    ساكن Ast_گره_عبارت* gateCheckedCountp(File_سطر* ثابت flp, Ast_گره_عبارت* ثابت countp,
                                          Ast_گره_عبارت*& overflowp, Ast_گره_عبارت* ثابت enablep) {
        إذا (overflowp) {
            overflowp = جديد AstLogAnd{flp, enablep->clone_درخت_خالص(خطأ), overflowp};
        }
        أرجع جمع_کن_Countp(flp, اشاره_تهی, countp, enablep);
    }
    ساكن Ast_گره_عبارت* جمع_کن_Checked_Countp(File_سطر* ثابت flp, Ast_گره_عبارت* ثابت مجموع_Countp,
                                         Ast_گره_عبارت* ثابت contributionp, Ast_گره_عبارت*& overflowp,
                                         Ast_گره_عبارت* ثابت contributionOverflowp = اشاره_تهی) {
        إذا (contributionOverflowp) { overflowp = orExprs(flp, overflowp, contributionOverflowp); }
        إذا (!مجموع_Countp) أرجع contributionp;
        Ast_گره_عبارت* ثابت oldTotalp = مجموع_Countp->clone_درخت_خالص(خطأ);
        Ast_گره_عبارت* ثابت resultp = جديد Ast_جمع_کن{flp, مجموع_Countp, contributionp};
        overflowp
            = orExprs(flp, overflowp, جديد AstLt{flp, resultp->clone_درخت_خالص(خطأ), oldTotalp});
        أرجع resultp;
    }
    AstIf* new_شمار_سرریز_Warning(File_سطر* ثابت flp, Ast_گره_عبارت* ثابت condp) ثابت {
        AstDisplay* ثابت dispp = جديد AstDisplay{
            flp, VDisplayType::DT_WARNING,
            "Cover sequence match count overflowed 32-bit storage; results may be incorrect.",
            اشاره_تهی, اشاره_تهی};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstIf* ثابت ifp = جديد AstIf{flp, condp, dispp};
        ifp->branchPred(VBranchPred::BP_UNLIKELY);
        أرجع ifp;
    }
    ساكن Ast_گره_عبارت* kill_فعال(پایینی_Ctx& c) {
        أرجع جديد AstNeq{c.flp, جديد AstVarRef{c.flp, c.killVarp, VAccess::READ},
                          assertKillGet(c.flp, c.assertType, c.directiveType)};
    }
    ساكن Ast_گره_عبارت* not_Kill_فعال(پایینی_Ctx& c) { أرجع جديد AstLogNot{c.flp, kill_فعال(c)}; }
    ساكن Ast_گره_عبارت* gateNotKill(پایینی_Ctx& c, Ast_گره_عبارت* exprp) {
        إذا (!exprp) أرجع اشاره_تهی;
        أرجع جديد AstLogAnd{c.flp, exprp, not_Kill_فعال(c)};
    }
    ساكن Ast_گره_عبارت* بعدی_Ring_شاخص(File_سطر* flp, Ast_گره_عبارت* idxExprp, uint32_t اندازه) {
        ثابت خودکار u32Const = [flp](uint32_t مقدار) {
            أرجع جديد AstConst{flp, AstConst::Widthed_مقدار{}, 32, مقدار};
        };
        UASSERT_OBJ(اندازه > 0, idxExprp, "Ring size must be positive");
        إذا (اندازه == 1) {
            VL_DO_DANGLING(idxExprp->delete_درخت(), idxExprp);
            أرجع u32Const(0);
        }
        // idx == size - 1 ? 0 : idx + 1
        Ast_جمع_کن* ثابت addp = جديد Ast_جمع_کن{flp, idxExprp->clone_درخت_خالص(خطأ), u32Const(1)};
        addp->dtypeFrom(idxExprp);
        AstCond* ثابت condp
            = جديد AstCond{flp, جديد AstEq{flp, idxExprp, u32Const(اندازه - 1)}, u32Const(0), addp};
        condp->dtypeFrom(idxExprp);
        أرجع condp;
    }
    ساكن Ast_گره_عبارت* delay_Ring_جایگاه(File_سطر* flp, Sva_State_رأس* vtxp, Ast_گره_عبارت* idxExprp,
                                      VAccess access = VAccess::READ) {
        // ring[idx]
        AstVar* ثابت ringp = vtxp->datap()->delayRingVarp;
        AstVarRef* ثابت refp = جديد AstVarRef{flp, ringp, access};
        إذا (vtxp->datap()->delayRingCounts) أرجع جديد AstArraySel{flp, refp, idxExprp};
        أرجع جديد AstSel{flp, refp, idxExprp, 1};
    }
    ساكن Ast_گره_عبارت* delay_Ring_سر_شمار(File_سطر* flp, Sva_State_رأس* vtxp) {
        Ast_گره_عبارت* ثابت headp
            = جديد AstVarRef{flp, vtxp->datap()->delay_Ring_سر_Varp, VAccess::READ};
        إذا (vtxp->datap()->delayRingCounts) أرجع headp;
        أرجع جديد AstExtend{flp, headp, vtxp->datap()->delay_Ring_زنده_شمار_Varp->dtypep()->عرض()};
    }
    ساكن Ast_گره_عبارت* delay_Ring_At_آخرین_شاخص(File_سطر* ثابت flp, AstVar* ثابت idxp,
                                             ثابت uint32_t اندازه) {
        أرجع جديد AstEq{flp, جديد AstVarRef{flp, idxp, VAccess::READ},
                         جديد AstConst{flp, AstConst::Widthed_مقدار{}, 32, اندازه - 1}};
    }
    ساكن Ast_گره_عبارت* delay_Ring_Output_شمار(File_سطر* ثابت flp, Sva_State_رأس* ثابت vtxp) {
        AstVar* ثابت idxp = vtxp->datap()->delayRingIdxVarp;
        ثابت uint32_t اندازه = vtxp->m_delay_Ring_اندازه;
        Ast_گره_عبارت* outgoingValidp
            = جديد AstVarRef{flp, vtxp->datap()->delayRingWrappedVarp, VAccess::READ};
        إذا (!vtxp->m_isFixedDelayRing) {
            outgoingValidp
                = جديد AstLogOr{flp, outgoingValidp, delay_Ring_At_آخرین_شاخص(flp, idxp, اندازه)};
        }
        AstVar* ثابت زنده_شمار_Varp = vtxp->datap()->delay_Ring_زنده_شمار_Varp;
        أرجع جديد AstCond{flp, outgoingValidp, delay_Ring_سر_شمار(flp, vtxp),
                           newTypedConstp(flp, زنده_شمار_Varp->dtypep(), 0)};
    }
    ساكن Ast_گره_عبارت* delay_Ring_Has_زنده_Threadsp(File_سطر* ثابت flp,
                                                 AstVar* ثابت زنده_شمار_Varp) {
        // active = live_count != 0;
        أرجع جديد AstNeq{flp, جديد AstVarRef{flp, زنده_شمار_Varp, VAccess::READ},
                          newTypedConstp(flp, زنده_شمار_Varp->dtypep(), 0)};
    }

    // Phase 3 output signals
    بنية Signal_مجموعه final {
        Ast_گره_عبارت* terminalActivep = اشاره_تهی;  // OR of all successful terminal matches
        Ast_گره_عبارت* تطابق_Countp = اشاره_تهی;  // NFA paths completing the sequence this tick
        Ast_گره_عبارت* تطابق_شمار_Overflowp = اشاره_تهی;  // Match count overflow this tick
        Ast_گره_عبارت* rejectBasep = اشاره_تهی;  // Reject when a terminal match fails
        Ast_گره_عبارت* required_گام_Rejectp = اشاره_تهی;  // Per-source reject from rejectOnFail Links
        Ast_گره_عبارت* throughoutRejectp = اشاره_تهی;  // Reject when a throughout guard drops
        Ast_گره_عبارت* threadFailCountp = اشاره_تهی;  // Number of threads rejected on this tick
    };

    // Disable condition also aborts attempts between clock events. A constant disable
    // never rises, and $sampled is unsupported in a sensitivity list.
    ساكن منطقي isAsyncDisable(ثابت Ast_گره_عبارت* ثابت disableExprp) {
        أرجع disableExprp && !VN_IS(disableExprp, Const)
               && !disableExprp->exists([](ثابت AstSampled*) { أرجع درست; });
    }

    // Emit the clocked update block of registers that hold pending attempts. The clearp
    // statements reset the same registers: a disable rising between clock events also
    // aborts pending attempts (IEEE 1800-2023 16.12), so clearp runs on that rising edge
    // in a separate block. It must clear unconditionally, as the disable may already be
    // false again when it runs, and the update block must not run on that edge, as it
    // would advance the attempts as if the clock had ticked. Blocks that hold no attempt
    // state, such as the $assertkill acknowledge in emitKillAckNba, do not go through here.
    // Callers pass a null clearp when the disable cannot rise between clock events.
    فراغ جمع_کن_State_Always(پایینی_Ctx& c, Ast_گره* bodyp, Ast_گره* clearp) {
        m_modp->جمع_کن_Stmtsp(
            جديد AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_درخت(خطأ), bodyp});
        إذا (!clearp) أرجع;
        m_modp->جمع_کن_Stmtsp(جديد AstAlways{
            c.flp, VAlwaysKwd::ALWAYS,
            جديد Ast_Sen_درخت{c.flp, جديد Ast_Sen_قلم{c.flp, VEdgeType::ET_POSEDGE,
                                                 c.disableExprp->clone_درخت_خالص(خطأ)}},
            clearp});
    }

    // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
    // Phase 2: State register NBA always block. Each clocked-edge target
    // latches the OR of its incoming contributions.
    فراغ emit_State_ثبات_Nba(پایینی_Ctx& c) {
        ثابت منطقي asyncDisable = isAsyncDisable(c.disableExprp);
        Ast_گره* bodyp = اشاره_تهی;
        Ast_گره* clearp = اشاره_تهی;
        لكل (ثابت V3_گراف_رأس& vtxr : c.گراف.m_گراف.vertices()) {
            ثابت Sva_State_رأس* ثابت vtxp = static_cast<ثابت Sva_State_رأس*>(&vtxr);
            إذا (!vtxp->datap()->stateVarp) استمر;

            Ast_گره_عبارت* بعدی_Statep = اشاره_تهی;
            لكل (ثابت V3_گراف_یال& edger : vtxp->درون_یالها()) {
                ثابت Sva_Trans_یال& tedger = static_cast<ثابت Sva_Trans_یال&>(edger);
                إذا (!tedger.m_consumes_چرخه) استمر;
                ثابت Sva_State_رأس* ثابت fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Clocked-edge source missing stateSig");

                Ast_گره_عبارت* srcSigp = fromVtxp->datap()->stateSigp->clone_درخت_خالص(خطأ);
                srcSigp = andCond(c.flp, srcSigp, tedger.m_condp);

                إذا (c.disableExprp) {
                    Ast_گره_عبارت* ثابت notDisp
                        = جديد AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(خطأ)};
                    srcSigp = جديد AstLogAnd{c.flp, srcSigp, notDisp};
                }
                بعدی_Statep = orExprs(c.flp, بعدی_Statep, srcSigp);
            }

            UASSERT_OBJ(بعدی_Statep, vtxp,
                        "Registered vertex has no clocked incoming contribution");
            بعدی_Statep = gateNotKill(c, بعدی_Statep);

            Ast_تخصیص_بده_Dly* ثابت assignp = جديد Ast_تخصیص_بده_Dly{
                c.flp, جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE}, بعدی_Statep};
            bodyp = Ast_گره::جمع_کن_بعدی_Null(bodyp, assignp);
            إذا (asyncDisable) {
                clearp = Ast_گره::جمع_کن_بعدی_Null(
                    clearp,
                    جديد Ast_تخصیص_بده_Dly{
                        c.flp, جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE},
                        جديد AstConst{c.flp, AstConst::بیت_False{}}});
            }
        }
        إذا (bodyp) جمع_کن_State_Always(c, bodyp, clearp);
    }

    // Phase 2b: In-place ring storage and registered ring metadata.
    فراغ emitDelayRingUpdates(پایینی_Ctx& c, ثابت منطقي شمار_Matches) {
        ثابت منطقي asyncDisable = isAsyncDisable(c.disableExprp);
        لكل (V3_گراف_رأس& vtxr : c.گراف.m_گراف.vertices()) {
            Sva_State_رأس* ثابت vtxp = static_cast<Sva_State_رأس*>(&vtxr);
            إذا (!vtxp->datap()->delayRingVarp) استمر;
            AstVar* ثابت idxp = vtxp->datap()->delayRingIdxVarp;
            AstVar* ثابت زنده_شمار_Varp = vtxp->datap()->delay_Ring_زنده_شمار_Varp;
            AstVar* ثابت wrappedp = vtxp->datap()->delayRingWrappedVarp;
            ثابت uint32_t اندازه = static_cast<uint32_t>(vtxp->m_delay_Ring_اندازه);

            Ast_گره_عبارت* incomingp = اشاره_تهی;
            Ast_گره_عبارت* incomingOverflowp = اشاره_تهی;
            لكل (ثابت V3_گراف_یال& edger : vtxp->درون_یالها()) {
                ++m_stat_Delay_Ring_یال_Visits;
                ثابت Sva_Trans_یال& tedger = static_cast<ثابت Sva_Trans_یال&>(edger);
                UASSERT_OBJ(tedger.m_consumes_چرخه == vtxp->m_isFixedDelayRing, vtxp,
                            "Delay-ring incoming edge kind mismatch");
                ثابت Sva_State_رأس* ثابت fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Delay-ring incoming source missing stateSig");
                UASSERT_OBJ(!شمار_Matches || fromVtxp->datap()->state_شمار_Sigp, fromVtxp,
                            "Delay-ring incoming source missing stateCountSig");
                // IEEE 1800-2023 16.14.1/16.14.3: assertion alternatives belong to one
                // evaluation; only cover sequence counts each end-of-match.
                Ast_گره_عبارت* contribp
                    = شمار_Matches
                          ? fromVtxp->datap()->state_شمار_Sigp->clone_درخت_خالص(خطأ)
                          : جديد AstExtend{c.flp,
                                          fromVtxp->datap()->stateSigp->clone_درخت_خالص(خطأ),
                                          m_u32DTypep->عرض()};
                Ast_گره_عبارت* contribOverflowp
                    = شمار_Matches && fromVtxp->datap()->state_شمار_سرریز_Sigp
                          ? fromVtxp->datap()->state_شمار_سرریز_Sigp->clone_درخت_خالص(خطأ)
                          : اشاره_تهی;
                إذا (tedger.m_condp) {
                    Ast_گره_عبارت* ثابت enablep = tedger.m_condp->clone_درخت_خالص(خطأ);
                    contribp = شمار_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, enablep)
                                   : جمع_کن_Countp(c.flp, اشاره_تهی, contribp, enablep);
                }
                إذا (c.disableExprp) {
                    Ast_گره_عبارت* ثابت notDisp
                        = جديد AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(خطأ)};
                    contribp = شمار_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, notDisp)
                                   : جمع_کن_Countp(c.flp, اشاره_تهی, contribp, notDisp);
                }
                incomingp = شمار_Matches ? جمع_کن_Checked_Countp(c.flp, incomingp, contribp,
                                                            incomingOverflowp, contribOverflowp)
                                         : جمع_کن_Countp(c.flp, incomingp, contribp);
            }
            UASSERT_OBJ(incomingp, vtxp, "Delay ring has no incoming edge");
            ثابت منطقي storesCounts = vtxp->datap()->delayRingCounts;
            Ast_گره_عبارت* ثابت incomingCountp
                = storesCounts
                      ? incomingp->clone_درخت_خالص(خطأ)
                      : جديد AstExtend{c.flp,
                                      جديد AstNeq{c.flp, incomingp->clone_درخت_خالص(خطأ),
                                                 newTypedConstp(c.flp, incomingp->dtypep(), 0)},
                                      m_u32DTypep->عرض()};
            Ast_گره_عبارت* ثابت storedIncomingp
                = storesCounts ? incomingp
                               : جديد AstNeq{c.flp, incomingp,
                                            newTypedConstp(c.flp, incomingp->dtypep(), 0)};
            // Only this block accesses the storage. Other processes read the cached head, so
            // the ring can be updated in place without an NBA shadow copy.
            Ast_تخصیص_بده* ثابت writeIncomingp = جديد Ast_تخصیص_بده{
                c.flp,
                delay_Ring_جایگاه(c.flp, vtxp, جديد AstVarRef{c.flp, idxp, VAccess::READ},
                              VAccess::WRITE),
                storedIncomingp};
            Ast_گره* updateBodyp = writeIncomingp;
            // live_count <= live_count - outgoing_count + incoming_count;
            Ast_گره_عبارت* ثابت outgoingp = delay_Ring_Output_شمار(c.flp, vtxp);
            Ast_گره_عبارت* ثابت afterOutgoingp
                = جديد AstSub{c.flp, جديد AstVarRef{c.flp, زنده_شمار_Varp, VAccess::READ}, outgoingp};
            Ast_گره_عبارت* ثابت oldAfterOutgoingp = afterOutgoingp->clone_درخت_خالص(خطأ);
            Ast_جمع_کن* ثابت بعدی_زنده_Countp = جديد Ast_جمع_کن{c.flp, afterOutgoingp, incomingCountp};
            إذا (storesCounts) {
                Ast_گره_عبارت* ثابت زنده_Overflowp
                    = جديد AstLt{c.flp, بعدی_زنده_Countp->clone_درخت_خالص(خطأ), oldAfterOutgoingp};
                Ast_گره_عبارت* ثابت overflowp = orExprs(c.flp, incomingOverflowp, زنده_Overflowp);
                updateBodyp = new_شمار_سرریز_Warning(c.flp, overflowp);
                updateBodyp->جمع_کن_بعدی(writeIncomingp);
            } وإلا {
                VL_DO_DANGLING(oldAfterOutgoingp->delete_درخت(), oldAfterOutgoingp);
            }
            // Cache the outgoing slot for the next index after the write. Reading after
            // writing also handles one-slot rings and two-slot range rings without alias cases.
            Ast_گره_عبارت* بعدی_سر_Idxp
                = بعدی_Ring_شاخص(c.flp, جديد AstVarRef{c.flp, idxp, VAccess::READ}, اندازه);
            إذا (!vtxp->m_isFixedDelayRing) {
                بعدی_سر_Idxp = بعدی_Ring_شاخص(c.flp, بعدی_سر_Idxp, اندازه);
            }
            updateBodyp->جمع_کن_بعدی(جديد Ast_تخصیص_بده_Dly{
                c.flp, جديد AstVarRef{c.flp, vtxp->datap()->delay_Ring_سر_Varp, VAccess::WRITE},
                delay_Ring_جایگاه(c.flp, vtxp, بعدی_سر_Idxp)});
            updateBodyp->جمع_کن_بعدی(جديد Ast_تخصیص_بده_Dly{
                c.flp, جديد AstVarRef{c.flp, زنده_شمار_Varp, VAccess::WRITE}, بعدی_زنده_Countp});
            // wrapped <= wrapped || idx == size - 1;
            updateBodyp->جمع_کن_بعدی(
                جديد Ast_تخصیص_بده_Dly{c.flp, جديد AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                 جديد AstLogOr{c.flp, جديد AstVarRef{c.flp, wrappedp, VAccess::READ},
                                              delay_Ring_At_آخرین_شاخص(c.flp, idxp, اندازه)}});
            // idx <= next_idx;
            updateBodyp->جمع_کن_بعدی(جديد Ast_تخصیص_بده_Dly{
                c.flp, جديد AstVarRef{c.flp, idxp, VAccess::WRITE},
                بعدی_Ring_شاخص(c.flp, جديد AstVarRef{c.flp, idxp, VAccess::READ}, اندازه)});
            إذا (vtxp->m_delayRingAdvanceCondp) {
                updateBodyp = جديد AstIf{
                    c.flp, sampled(vtxp->m_delayRingAdvanceCondp->clone_درخت_خالص(خطأ)),
                    updateBodyp};
            }

            Ast_گره_عبارت* clearCondp = kill_فعال(c);
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
            Ast_گره_عبارت* guardp = اشاره_تهی;
            لكل (Ast_گره_عبارت* ثابت cp : vtxp->m_throughoutConds) {
                Ast_گره_عبارت* ثابت sampledp = sampled(cp->clone_درخت_خالص(خطأ));
                guardp = guardp ? static_cast<Ast_گره_عبارت*>(جديد AstLogAnd{c.flp, guardp, sampledp})
                                : sampledp;
            }
            إذا (guardp) clearCondp = orExprs(c.flp, clearCondp, جديد AstLogNot{c.flp, guardp});
            // Logically clear the ring without touching its wide storage.
            Ast_تخصیص_بده_Dly* ثابت clearCountp
                = جديد Ast_تخصیص_بده_Dly{c.flp, جديد AstVarRef{c.flp, زنده_شمار_Varp, VAccess::WRITE},
                                   newTypedConstp(c.flp, زنده_شمار_Varp->dtypep(), 0)};
            clearCountp->جمع_کن_بعدی(جديد Ast_تخصیص_بده_Dly{c.flp,
                                                  جديد AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                                  جديد AstConst{c.flp, AstConst::بیت_False{}}});
            clearCountp->جمع_کن_بعدی(جديد Ast_تخصیص_بده_Dly{c.flp,
                                                  جديد AstVarRef{c.flp, idxp, VAccess::WRITE},
                                                  newTypedConstp(c.flp, idxp->dtypep(), 0)});
            updateBodyp = جديد AstIf{c.flp, clearCondp, clearCountp, updateBodyp};

            جمع_کن_State_Always(c, updateBodyp, asyncDisable ? clearCountp->clone_درخت(درست) : اشاره_تهی);
        }
    }

    // Phase 2c: SAnd combiner done-latch always block.
    // NBA semantics ensure doneL/doneR read pre-update values (IEEE 16.9.5).
    فراغ emit_And_Combiner_انجامشده_Latch_Nba(پایینی_Ctx& c) {
        ثابت منطقي asyncDisable = isAsyncDisable(c.disableExprp);
        لكل (ثابت V3_گراف_رأس& vtxr : c.گراف.m_گراف.vertices()) {
            ثابت Sva_State_رأس* ثابت avp = static_cast<ثابت Sva_State_رأس*>(&vtxr);
            إذا (!avp->datap()->انجامشده_LVarp) استمر;
            // doneLVars is non-null only for AndCombiner vertices, which always
            // have both m_andLhsTermp and m_andRhsTermp set at build time.
            UASSERT_OBJ(avp->m_andLhsTermp && avp->m_andRhsTermp, avp,
                        "AndCombiner vertex missing LHS/RHS terminal");
            ثابت Sva_State_رأس* ثابت lhsTermp = avp->m_andLhsTermp;
            ثابت Sva_State_رأس* ثابت rhsTermp = avp->m_andRhsTermp;
            // resolveLinks' 2*N+2 fixed-point pass is guaranteed to populate
            // stateSigp on every AndCombiner and its LHS/RHS terminals.
            UASSERT_OBJ(lhsTermp->datap()->stateSigp && rhsTermp->datap()->stateSigp
                            && avp->datap()->stateSigp,
                        avp, "AndCombiner stateSigp chain unresolved after resolveLinks");

            Ast_تخصیص_بده_Dly* ثابت clearLp = جديد Ast_تخصیص_بده_Dly{
                c.flp, جديد AstVarRef{c.flp, avp->datap()->انجامشده_LVarp, VAccess::WRITE},
                جديد AstConst{c.flp, AstConst::بیت_False{}}};
            Ast_تخصیص_بده_Dly* ثابت clearRp = جديد Ast_تخصیص_بده_Dly{
                c.flp, جديد AstVarRef{c.flp, avp->datap()->انجامشده_RVarp, VAccess::WRITE},
                جديد AstConst{c.flp, AstConst::بیت_False{}}};
            clearLp->جمع_کن_بعدی(clearRp);

            Ast_گره_عبارت* ثابت تطابق_LNowp
                = build_تطابق_Now(c.flp, lhsTermp->datap()->stateSigp, avp->m_andLhsCondp);
            Ast_گره_عبارت* ثابت تطابق_RNowp
                = build_تطابق_Now(c.flp, rhsTermp->datap()->stateSigp, avp->m_andRhsCondp);
            Ast_گره_عبارت* gateLp = تطابق_LNowp;
            Ast_گره_عبارت* gateRp = تطابق_RNowp;
            إذا (c.disableExprp) {
                Ast_گره_عبارت* ثابت notDisLp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(خطأ)};
                gateLp = جديد AstLogAnd{c.flp, gateLp, notDisLp};
                Ast_گره_عبارت* ثابت notDisRp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(خطأ)};
                gateRp = جديد AstLogAnd{c.flp, gateRp, notDisRp};
            }
            Ast_تخصیص_بده_Dly* ثابت مجموعه_Lp = جديد Ast_تخصیص_بده_Dly{
                c.flp, جديد AstVarRef{c.flp, avp->datap()->انجامشده_LVarp, VAccess::WRITE},
                جديد AstConst{c.flp, AstConst::بیت_True{}}};
            AstIf* ثابت مجموعه_LIfp = جديد AstIf{c.flp, gateLp, مجموعه_Lp, اشاره_تهی};
            Ast_تخصیص_بده_Dly* ثابت مجموعه_Rp = جديد Ast_تخصیص_بده_Dly{
                c.flp, جديد AstVarRef{c.flp, avp->datap()->انجامشده_RVarp, VAccess::WRITE},
                جديد AstConst{c.flp, AstConst::بیت_True{}}};
            AstIf* ثابت مجموعه_RIfp = جديد AstIf{c.flp, gateRp, مجموعه_Rp, اشاره_تهی};
            مجموعه_LIfp->جمع_کن_بعدی(مجموعه_RIfp);

            Ast_گره_عبارت* clearCondp = جديد AstLogOr{c.flp, kill_فعال(c),
                                                   avp->datap()->stateSigp->clone_درخت_خالص(خطأ)};
            إذا (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_درخت_خالص(خطأ));
            }
            AstIf* ثابت topp = جديد AstIf{c.flp, clearCondp, clearLp, مجموعه_LIfp};
            جمع_کن_State_Always(c, topp, asyncDisable ? clearLp->clone_درخت(درست) : اشاره_تهی);
        }
    }

    فراغ emitKillAckNba(پایینی_Ctx& c) {
        Ast_تخصیص_بده_Dly* ثابت ackp
            = جديد Ast_تخصیص_بده_Dly{c.flp, جديد AstVarRef{c.flp, c.killVarp, VAccess::WRITE},
                               assertKillGet(c.flp, c.assertType, c.directiveType)};
        m_modp->جمع_کن_Stmtsp(
            جديد AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_درخت(خطأ), ackp});
    }

    // Phase 3/3a/3b: Compute terminal match/reject signals, required-step reject,
    // throughout-drop reject; clean up intermediate state signals.
    // Phase 3: terminalActive and rejectBase from Links to matchVertex.
    // Builder only adds Links (non-clocked) to matchVertex via addLink in
    // wireMatchAndMidSources. For cover sequence, also count each end-of-match
    // so the action can be replayed without unrolling endpoints.
    فراغ compute_Terminal_تطابق_And_Reject(پایینی_Ctx& c, Signal_مجموعه& sigs, ثابت منطقي need_تطابق_شمار) {
        لكل (ثابت Sva_Trans_یال* ثابت tedgep : c.گراف.all_یالها()) {
            إذا (tedgep->مقصد_Vtxp() != c.گراف.m_تطابق_Vertexp) استمر;
            Sva_State_رأس* ثابت fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                        "Terminal-link source missing stateSig");

            Ast_گره_عبارت* srcSigp = fromVtxp->datap()->stateSigp->clone_درخت_خالص(خطأ);
            srcSigp = andCond(c.flp, srcSigp, tedgep->m_condp);
            إذا (need_تطابق_شمار) {
                UASSERT_OBJ(fromVtxp->datap()->state_شمار_Sigp, fromVtxp,
                            "Terminal-link source missing stateCountSig");
                Ast_گره_عبارت* contributionp
                    = fromVtxp->datap()->state_شمار_Sigp->clone_درخت_خالص(خطأ);
                Ast_گره_عبارت* contributionOverflowp
                    = fromVtxp->datap()->state_شمار_سرریز_Sigp
                          ? fromVtxp->datap()->state_شمار_سرریز_Sigp->clone_درخت_خالص(خطأ)
                          : اشاره_تهی;
                إذا (tedgep->m_condp) {
                    contributionp = gateCheckedCountp(c.flp, contributionp, contributionOverflowp,
                                                      tedgep->m_condp->clone_درخت_خالص(خطأ));
                }
                sigs.تطابق_Countp
                    = جمع_کن_Checked_Countp(c.flp, sigs.تطابق_Countp, contributionp,
                                       sigs.تطابق_شمار_Overflowp, contributionOverflowp);
            }

            إذا (fromVtxp->m_delay_Ring_اندازه && !fromVtxp->m_isFixedDelayRing) {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_درخت_خالص(خطأ));
                // reject |= (wrapped || idx == size - 1) && head && final_condition;
                Ast_گره_عبارت* expireContribp = delay_Ring_Output_شمار(c.flp, fromVtxp);
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
        UASSERT_OBJ(sigs.terminalActivep, c.گراف.m_تطابق_Vertexp,
                    "No terminal edge to match vertex");
    }

    Ast_گره_عبارت* newThroughoutThreadFailCountp(پایینی_Ctx& c, AstVar* ثابت delay_Ring_زنده_شمار_Varp,
                                               Ast_گره_عبارت* ثابت stateExprp,
                                               Ast_گره_عبارت* ثابت enablep) {
        Ast_گره_عبارت* ثابت فعال_Thread_Countp
            = delay_Ring_زنده_شمار_Varp
                  ? static_cast<Ast_گره_عبارت*>(
                        جديد AstVarRef{c.flp, delay_Ring_زنده_شمار_Varp, VAccess::READ})
                  : جديد AstExtend{c.flp, stateExprp->clone_درخت_خالص(خطأ), m_u32DTypep->عرض()};
        أرجع جمع_کن_Countp(c.flp, اشاره_تهی, فعال_Thread_Countp, enablep);
    }

    // Phase 3b: Throughout-drop and ring-wide abort rejection.
    فراغ computeThroughoutReject(پایینی_Ctx& c, Signal_مجموعه& sigs, ثابت منطقي need_Thread_Fail_شمار) {
        لكل (ثابت V3_گراف_رأس& vtxr : c.گراف.m_گراف.vertices()) {
            ثابت Sva_State_رأس* ثابت vtxp = static_cast<ثابت Sva_State_رأس*>(&vtxr);
            ثابت خودکار& conds = vtxp->m_throughoutConds;
            إذا (conds.خالی() && !vtxp->m_abortRejectp) استمر;
            إذا (vtxp->m_isAndCombiner) استمر;
            Ast_گره_عبارت* stateExprp = اشاره_تهی;
            إذا (vtxp->datap()->stateVarp) {
                stateExprp = جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
            } وإلا إذا (vtxp->datap()->delayRingVarp && vtxp->m_isFixedDelayRing) {
                stateExprp
                    = delay_Ring_Has_زنده_Threadsp(c.flp, vtxp->datap()->delay_Ring_زنده_شمار_Varp);
            } وإلا {
                UASSERT_OBJ(vtxp->datap()->stateSigp, vtxp,
                            "Throughout-conds vertex missing state representation");
                stateExprp = vtxp->datap()->stateSigp->clone_درخت_خالص(خطأ);
            }
            Ast_گره_عبارت* guardp = اشاره_تهی;
            لكل (Ast_گره_عبارت* ثابت cp : conds) {
                Ast_گره_عبارت* ثابت sp = sampled(cp->clone_درخت_خالص(خطأ));
                guardp = guardp ? static_cast<Ast_گره_عبارت*>(جديد AstLogAnd{c.flp, guardp, sp}) : sp;
            }
            إذا (vtxp->m_abortRejectp) {
                Ast_گره_عبارت* ثابت notAbortp
                    = جديد AstLogNot{c.flp, sampled(vtxp->m_abortRejectp->clone_درخت_خالص(خطأ))};
                guardp = guardp
                             ? static_cast<Ast_گره_عبارت*>(جديد AstLogAnd{c.flp, guardp, notAbortp})
                             : notAbortp;
            }
            Ast_گره_عبارت* rejectCondp = جديد AstLogNot{c.flp, guardp};
            إذا (vtxp->m_abortClearp) {
                // Any abort clears the ring. Accept aborts suppress a simultaneous guard failure;
                // reject aborts are restored below so they still force rejection.
                Ast_گره_عبارت* ثابت notAbortClearp
                    = جديد AstLogNot{c.flp, sampled(vtxp->m_abortClearp->clone_درخت_خالص(خطأ))};
                rejectCondp = جديد AstLogAnd{c.flp, rejectCondp, notAbortClearp};
                إذا (vtxp->m_abortRejectp) {
                    rejectCondp = جديد AstLogOr{
                        c.flp, rejectCondp, sampled(vtxp->m_abortRejectp->clone_درخت_خالص(خطأ))};
                }
            }
            إذا (need_Thread_Fail_شمار) {
                Ast_گره_عبارت* ثابت contributionp
                    = newThroughoutThreadFailCountp(c, vtxp->datap()->delay_Ring_زنده_شمار_Varp,
                                                    stateExprp, rejectCondp->clone_درخت_خالص(خطأ));
                sigs.threadFailCountp = جمع_کن_Countp(c.flp, sigs.threadFailCountp, contributionp);
                إذا (vtxp->m_abortRejectp && vtxp->m_delayRingAdvanceCondp) {
                    // An advancing ring thread also occupies its same-tick match vertex in the
                    // unrolled NFA, so abort rejection must replay both fail actions.
                    Ast_گره_عبارت* ثابت abortAndAdvancep = جديد AstLogAnd{
                        c.flp, sampled(vtxp->m_abortRejectp->clone_درخت_خالص(خطأ)),
                        sampled(vtxp->m_delayRingAdvanceCondp->clone_درخت_خالص(خطأ))};
                    Ast_گره_عبارت* ثابت تطابق_Contributionp = newThroughoutThreadFailCountp(
                        c, vtxp->datap()->delay_Ring_زنده_شمار_Varp, stateExprp, abortAndAdvancep);
                    sigs.threadFailCountp
                        = جمع_کن_Countp(c.flp, sigs.threadFailCountp, تطابق_Contributionp);
                }
            }
            sigs.throughoutRejectp = orExprs(c.flp, sigs.throughoutRejectp,
                                             جديد AstLogAnd{c.flp, stateExprp, rejectCondp});
        }
    }

    Signal_مجموعه compute_سیگنالها(پایینی_Ctx& c, ثابت منطقي need_Thread_Fail_شمار,
                             ثابت منطقي need_Throughout_Thread_Fail_شمار, ثابت منطقي need_تطابق_شمار) {
        Signal_مجموعه sigs;

        compute_Terminal_تطابق_And_Reject(c, sigs, need_تطابق_شمار);

        // Phase 3a: required-step rejection.
        // Builder only sets m_rejectOnFail on non-clocked Links with m_condp
        // or m_condVtxp, and the source always has a resolved stateSig.
        لكل (ثابت Sva_Trans_یال* ثابت tedgep : c.گراف.all_یالها()) {
            إذا (!tedgep->m_rejectOnFail) استمر;
            ثابت Sva_State_رأس* ثابت fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp && (tedgep->m_condp || tedgep->m_condVtxp),
                        fromVtxp,
                        "rejectOnFail Link must have condp/condVtxp and source stateSig");
            Ast_گره_عبارت* ثابت srcSigp = fromVtxp->datap()->stateSigp->clone_درخت_خالص(خطأ);
            Ast_گره_عبارت* condp = اشاره_تهی;
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
            Ast_گره_عبارت* ثابت notCondp = جديد AstLogNot{c.flp, condp};
            Ast_گره_عبارت* ثابت rawFailp = جديد AstLogAnd{c.flp, srcSigp, notCondp};
            إذا (need_Thread_Fail_شمار) {
                // thread_fail_count += fail;
                sigs.threadFailCountp = جمع_کن_Countp(
                    c.flp, sigs.threadFailCountp,
                    جديد AstExtend{c.flp, rawFailp->clone_درخت_خالص(خطأ), m_u32DTypep->عرض()});
            }
            Ast_گره_عبارت* ثابت failp = gateNotKill(c, rawFailp);
            sigs.required_گام_Rejectp = orExprs(c.flp, sigs.required_گام_Rejectp, failp);
        }

        computeThroughoutReject(c, sigs, need_Throughout_Thread_Fail_شمار);
        إذا (sigs.threadFailCountp) {
            sigs.threadFailCountp
                = جمع_کن_Countp(c.flp, اشاره_تهی, sigs.threadFailCountp, not_Kill_فعال(c));
        }
        إذا (sigs.تطابق_Countp) {
            إذا (c.تطابق_Condp) {
                sigs.تطابق_Countp
                    = gateCheckedCountp(c.flp, sigs.تطابق_Countp, sigs.تطابق_شمار_Overflowp,
                                        sampled(c.تطابق_Condp->clone_درخت_خالص(خطأ)));
            }
            sigs.تطابق_Countp = gateCheckedCountp(c.flp, sigs.تطابق_Countp, sigs.تطابق_شمار_Overflowp,
                                                 not_Kill_فعال(c));
        }
        sigs.terminalActivep = gateNotKill(c, sigs.terminalActivep);
        sigs.rejectBasep = gateNotKill(c, sigs.rejectBasep);
        sigs.throughoutRejectp = gateNotKill(c, sigs.throughoutRejectp);

        // Clean up intermediate state signals. These are orphan subtrees
        // (never linked into the enclosing AST); deleteTree() is immediate
        // which is what we want since stateSigp lifetime ends with this scope.
        لكل (V3_گراف_رأس& vtxr : c.گراف.m_گراف.vertices()) {
            Sva_State_رأس* ثابت vtxp = static_cast<Sva_State_رأس*>(&vtxr);
            Ast_گره_عبارت*& sigp = vtxp->datap()->stateSigp;
            إذا (sigp) VL_DO_DANGLING(sigp->delete_درخت(), sigp);
            Ast_گره_عبارت*& countp = vtxp->datap()->state_شمار_Sigp;
            إذا (countp) VL_DO_DANGLING(countp->delete_درخت(), countp);
            Ast_گره_عبارت*& overflowp = vtxp->datap()->state_شمار_سرریز_Sigp;
            إذا (overflowp) VL_DO_DANGLING(overflowp->delete_درخت(), overflowp);
        }
        // Disable iff gating (IEEE 1800-2023 16.12) on the current level value.
        إذا (c.disableExprp) {
            // terminalActivep is always set, so gate it unconditionally.
            Ast_گره_عبارت* ثابت notTermp
                = جديد AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(خطأ)};
            sigs.terminalActivep = جديد AstLogAnd{c.flp, sigs.terminalActivep, notTermp};
            إذا (sigs.rejectBasep) {
                Ast_گره_عبارت* ثابت notDisp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(خطأ)};
                sigs.rejectBasep = جديد AstLogAnd{c.flp, sigs.rejectBasep, notDisp};
            }
            إذا (sigs.throughoutRejectp) {
                Ast_گره_عبارت* ثابت notDisp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(خطأ)};
                sigs.throughoutRejectp = جديد AstLogAnd{c.flp, sigs.throughoutRejectp, notDisp};
            }
            إذا (sigs.required_گام_Rejectp) {
                Ast_گره_عبارت* ثابت notDisp
                    = جديد AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(خطأ)};
                sigs.required_گام_Rejectp = جديد AstLogAnd{c.flp, sigs.required_گام_Rejectp, notDisp};
            }
            إذا (sigs.تطابق_Countp) {
                sigs.تطابق_Countp = gateCheckedCountp(
                    c.flp, sigs.تطابق_Countp, sigs.تطابق_شمار_Overflowp,
                    جديد AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(خطأ)});
            }
        }

        إذا (sigs.تطابق_شمار_Overflowp) {
            m_modp->جمع_کن_Stmtsp(
                جديد AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_درخت(خطأ),
                              new_شمار_سرریز_Warning(c.flp, sigs.تطابق_شمار_Overflowp)});
            sigs.تطابق_شمار_Overflowp = اشاره_تهی;
        }

        أرجع sigs;
    }

    // Phase 1: Resolve combinational Links via fixed-point propagation.
    فراغ resolveLinks(پایینی_Ctx& c, Ast_گره_عبارت* triggerExprp, ثابت منطقي شمار_Matches) {
        // datap() was freshly allocated in lower() -- all state signals start null.
        c.گراف.m_شروع_Vertexp->datap()->stateSigp = triggerExprp->clone_درخت_خالص(خطأ);
        c.گراف.m_شروع_Vertexp->datap()->state_شمار_Sigp
            = جديد AstExtend{c.flp, triggerExprp->clone_درخت_خالص(خطأ), m_u32DTypep->عرض()};
        لكل (V3_گراف_رأس& vtxr : c.گراف.m_گراف.vertices()) {
            Sva_State_رأس* ثابت vtxp = static_cast<Sva_State_رأس*>(&vtxr);
            إذا (vtxp->datap()->stateVarp) {
                vtxp->datap()->stateSigp
                    = جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
                vtxp->datap()->state_شمار_Sigp = جديد AstExtend{
                    c.flp, جديد AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ},
                    m_u32DTypep->عرض()};
            } وإلا إذا (vtxp->datap()->delayRingVarp) {
                إذا (vtxp->m_isFixedDelayRing) {
                    // state_count = wrapped ? head : 0; state = state_count != 0;
                    Ast_گره_عبارت* ثابت countp = delay_Ring_Output_شمار(c.flp, vtxp);
                    vtxp->datap()->stateSigp = جديد AstNeq{c.flp, countp->clone_درخت_خالص(خطأ),
                                                          newTypedConstp(c.flp, m_u32DTypep, 0)};
                    vtxp->datap()->state_شمار_Sigp = countp;
                } وإلا {
                    vtxp->datap()->stateSigp
                        = delay_Ring_Has_زنده_Threadsp(c.flp, vtxp->datap()->delay_Ring_زنده_شمار_Varp);
                    vtxp->datap()->state_شمار_Sigp = جديد AstVarRef{
                        c.flp, vtxp->datap()->delay_Ring_زنده_شمار_Varp, VAccess::READ};
                }
            }
        }
        // Fixed-point propagation along zero-delay (Link) edges. Rebuild each
        // derived signal from its incoming edges on every pass; appending the same
        // contributions repeatedly makes merge expressions grow exponentially.
        ثابت اندازه_t passes = 2 * c.گراف.m_گراف.vertices().اندازه() + 2;
        لكل (اندازه_t pass = 0; pass < passes; ++pass) {
            // Rebuild SAnd combiners once both sub-NFA terminals are available.
            لكل (V3_گراف_رأس& vtxr : c.گراف.m_گراف.vertices()) {
                Sva_State_رأس* ثابت vtxp = static_cast<Sva_State_رأس*>(&vtxr);
                إذا (!vtxp->m_isAndCombiner) استمر;
                // AndCombiner vertices always have both terminal pointers set.
                UASSERT_OBJ(vtxp->m_andLhsTermp && vtxp->m_andRhsTermp, vtxp,
                            "AndCombiner vertex missing LHS/RHS terminal");
                ثابت Sva_State_رأس* ثابت lhsTermp = vtxp->m_andLhsTermp;
                ثابت Sva_State_رأس* ثابت rhsTermp = vtxp->m_andRhsTermp;
                إذا (!lhsTermp->datap()->stateSigp || !rhsTermp->datap()->stateSigp) استمر;
                Ast_گره_عبارت* ثابت تطابق_Lp
                    = build_تطابق_Now(c.flp, lhsTermp->datap()->stateSigp, vtxp->m_andLhsCondp);
                Ast_گره_عبارت* ثابت تطابق_Rp
                    = build_تطابق_Now(c.flp, rhsTermp->datap()->stateSigp, vtxp->m_andRhsCondp);
                Ast_گره_عبارت* ثابت انجامشده_LOrp = جديد AstLogOr{
                    c.flp, جديد AstVarRef{c.flp, vtxp->datap()->انجامشده_LVarp, VAccess::READ}, تطابق_Lp};
                Ast_گره_عبارت* ثابت انجامشده_ROrp = جديد AstLogOr{
                    c.flp, جديد AstVarRef{c.flp, vtxp->datap()->انجامشده_RVarp, VAccess::READ}, تطابق_Rp};
                Ast_گره_عبارت* ثابت bothp = جديد AstLogAnd{c.flp, انجامشده_LOrp, انجامشده_ROrp};
                Ast_گره_عبارت* ثابت oneNowp = جديد AstLogOr{c.flp, تطابق_Lp->clone_درخت_خالص(خطأ),
                                                          تطابق_Rp->clone_درخت_خالص(خطأ)};
                إذا (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_درخت(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = جديد AstLogAnd{c.flp, bothp, oneNowp};
                إذا (vtxp->datap()->state_شمار_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_شمار_Sigp->delete_درخت(),
                                   vtxp->datap()->state_شمار_Sigp);
                }
                vtxp->datap()->state_شمار_Sigp = جديد AstExtend{
                    c.flp, vtxp->datap()->stateSigp->clone_درخت_خالص(خطأ), m_u32DTypep->عرض()};
            }

            لكل (V3_گراف_رأس& vtxr : c.گراف.m_گراف.vertices()) {
                Sva_State_رأس* ثابت vtxp = static_cast<Sva_State_رأس*>(&vtxr);
                إذا (vtxp == c.گراف.m_شروع_Vertexp || vtxp->datap()->stateVarp
                    || vtxp->datap()->delayRingVarp || vtxp->m_isAndCombiner || vtxp->m_is_تطابق
                    || vtxp->m_isRejectSink) {
                    استمر;
                }
                Ast_گره_عبارت* بعدی_Statep = اشاره_تهی;
                Ast_گره_عبارت* بعدی_State_Countp = اشاره_تهی;
                Ast_گره_عبارت* بعدی_State_شمار_Overflowp = اشاره_تهی;
                لكل (ثابت V3_گراف_یال& er : vtxp->درون_یالها()) {
                    ثابت Sva_Trans_یال& te = static_cast<ثابت Sva_Trans_یال&>(er);
                    ثابت Sva_State_رأس* ثابت fromVtxp = te.fromVtxp();
                    إذا (!fromVtxp->datap()->stateSigp) استمر;
                    UASSERT_OBJ(fromVtxp->datap()->state_شمار_Sigp, fromVtxp,
                                "Link source missing stateCountSig");
                    Ast_گره_عبارت* ثابت contributionp = andCond(
                        c.flp, fromVtxp->datap()->stateSigp->clone_درخت_خالص(خطأ), te.m_condp);
                    بعدی_Statep = orExprs(c.flp, بعدی_Statep, contributionp);
                    Ast_گره_عبارت* شمار_Contributionp
                        = fromVtxp->datap()->state_شمار_Sigp->clone_درخت_خالص(خطأ);
                    Ast_گره_عبارت* شمار_Contribution_Overflowp
                        = شمار_Matches && fromVtxp->datap()->state_شمار_سرریز_Sigp
                              ? fromVtxp->datap()->state_شمار_سرریز_Sigp->clone_درخت_خالص(خطأ)
                              : اشاره_تهی;
                    إذا (te.m_condp) {
                        Ast_گره_عبارت* ثابت enablep = te.m_condp->clone_درخت_خالص(خطأ);
                        شمار_Contributionp
                            = شمار_Matches
                                  ? gateCheckedCountp(c.flp, شمار_Contributionp,
                                                      شمار_Contribution_Overflowp, enablep)
                                  : جمع_کن_Countp(c.flp, اشاره_تهی, شمار_Contributionp, enablep);
                    }
                    بعدی_State_Countp
                        = شمار_Matches
                              ? جمع_کن_Checked_Countp(c.flp, بعدی_State_Countp, شمار_Contributionp,
                                                 بعدی_State_شمار_Overflowp,
                                                 شمار_Contribution_Overflowp)
                              : جمع_کن_Countp(c.flp, بعدی_State_Countp, شمار_Contributionp);
                }
                إذا (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_درخت(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = بعدی_Statep;
                إذا (vtxp->datap()->state_شمار_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_شمار_Sigp->delete_درخت(),
                                   vtxp->datap()->state_شمار_Sigp);
                }
                vtxp->datap()->state_شمار_Sigp = بعدی_State_Countp;
                إذا (vtxp->datap()->state_شمار_سرریز_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_شمار_سرریز_Sigp->delete_درخت(),
                                   vtxp->datap()->state_شمار_سرریز_Sigp);
                }
                vtxp->datap()->state_شمار_سرریز_Sigp = بعدی_State_شمار_Overflowp;
            }
        }
    }

عام:
    // Combine terminal/reject signals into final output expression.
    ساكن Ast_گره_عبارت* assemble_نتیجه(Ast_گره_Cover_Or_Assert* ثابت assertp, ثابت منطقي negated,
                                       Ast_گره_عبارت* ثابت تطابق_Condp, ثابت Signal_مجموعه& sigs,
                                       Ast_گره_عبارت** ثابت بیرون_Matchpp) {
        File_سطر* ثابت flp = assertp->fileline();
        ثابت منطقي isCover = VN_IS(assertp, Cover);
        Ast_گره_عبارت* ثابت terminalActivep = sigs.terminalActivep;
        Ast_گره_عبارت* ثابت rejectBasep = sigs.rejectBasep;
        Ast_گره_عبارت* ثابت throughoutRejectp = sigs.throughoutRejectp;
        Ast_گره_عبارت* ثابت required_گام_Rejectp = sigs.required_گام_Rejectp;
        // Property negation (IEEE 1800-2023 16.12.1 `not`): invert match/reject.
        إذا (negated) {
            إذا (isCover) {
                إذا (terminalActivep)
                    VL_DO_DANGLING(terminalActivep->delete_درخت(), terminalActivep);
                Ast_گره_عبارت* negRejectp = اشاره_تهی;
                إذا (تطابق_Condp && rejectBasep) {
                    Ast_گره_عبارت* ثابت sampledCondp = sampled(تطابق_Condp->clone_درخت_خالص(خطأ));
                    Ast_گره_عبارت* ثابت notCondp = جديد AstLogNot{flp, sampledCondp};
                    negRejectp = جديد AstLogAnd{flp, rejectBasep, notCondp};
                } وإلا إذا (rejectBasep) {
                    VL_DO_DANGLING(rejectBasep->delete_درخت(), rejectBasep);
                }
                إذا (throughoutRejectp) negRejectp = orExprs(flp, negRejectp, throughoutRejectp);
                إذا (required_گام_Rejectp)
                    negRejectp = orExprs(flp, negRejectp, required_گام_Rejectp);
                أرجع negRejectp ? negRejectp : جديد AstConst{flp, AstConst::بیت_False{}};
            }
            // Negated assert/assume: output = !match.
            Ast_گره_عبارت* matchp = terminalActivep;
            إذا (تطابق_Condp) {
                Ast_گره_عبارت* ثابت sampledCondp = sampled(تطابق_Condp->clone_درخت_خالص(خطأ));
                matchp = جديد AstLogAnd{flp, matchp, sampledCondp};
            }
            إذا (بیرون_Matchpp) {
                Ast_گره_عبارت* notPMatchp = اشاره_تهی;
                إذا (تطابق_Condp && rejectBasep) {
                    Ast_گره_عبارت* ثابت sampledCondp = sampled(تطابق_Condp->clone_درخت_خالص(خطأ));
                    notPMatchp = جديد AstLogAnd{flp, rejectBasep->clone_درخت_خالص(خطأ),
                                               جديد AstLogNot{flp, sampledCondp}};
                } وإلا إذا (rejectBasep) {
                    notPMatchp = rejectBasep->clone_درخت_خالص(خطأ);
                }
                إذا (throughoutRejectp)
                    notPMatchp = orExprs(flp, notPMatchp, throughoutRejectp->clone_درخت_خالص(خطأ));
                إذا (required_گام_Rejectp)
                    notPMatchp
                        = orExprs(flp, notPMatchp, required_گام_Rejectp->clone_درخت_خالص(خطأ));
                *بیرون_Matchpp = notPMatchp;
            }
            إذا (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_درخت(), throughoutRejectp);
            إذا (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_درخت(), rejectBasep);
            إذا (required_گام_Rejectp)
                VL_DO_DANGLING(required_گام_Rejectp->delete_درخت(), required_گام_Rejectp);
            Ast_گره_عبارت* ثابت نتیجه_Exprp = جديد AstLogNot{flp, matchp};
            أرجع نتیجه_Exprp;
        }
        إذا (isCover) {
            إذا (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_درخت(), throughoutRejectp);
            إذا (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_درخت(), rejectBasep);
            إذا (required_گام_Rejectp)
                VL_DO_DANGLING(required_گام_Rejectp->delete_درخت(), required_گام_Rejectp);
            إذا (تطابق_Condp) {
                Ast_گره_عبارت* ثابت sampledCondp = sampled(تطابق_Condp->clone_درخت_خالص(خطأ));
                أرجع جديد AstLogAnd{flp, terminalActivep, sampledCondp};
            }
            أرجع terminalActivep;
        }
        // Assert/assume: output = !reject
        Ast_گره_عبارت* rejectp = اشاره_تهی;
        إذا (تطابق_Condp && rejectBasep) {
            Ast_گره_عبارت* ثابت sampledCondp = sampled(تطابق_Condp->clone_درخت_خالص(خطأ));
            rejectp = جديد AstLogAnd{flp, rejectBasep, جديد AstLogNot{flp, sampledCondp}};
        } وإلا إذا (rejectBasep) {
            VL_DO_DANGLING(rejectBasep->delete_درخت(), rejectBasep);
        }
        إذا (بیرون_Matchpp) {
            Ast_گره_عبارت* تطابق_Exprp = terminalActivep->clone_درخت_خالص(خطأ);
            إذا (تطابق_Condp) {
                Ast_گره_عبارت* ثابت sp = sampled(تطابق_Condp->clone_درخت_خالص(خطأ));
                تطابق_Exprp = جديد AstLogAnd{flp, تطابق_Exprp, sp};
            }
            *بیرون_Matchpp = تطابق_Exprp;
        }
        إذا (terminalActivep) VL_DO_DANGLING(terminalActivep->delete_درخت(), terminalActivep);
        إذا (throughoutRejectp) rejectp = orExprs(flp, rejectp, throughoutRejectp);
        إذا (required_گام_Rejectp) rejectp = orExprs(flp, rejectp, required_گام_Rejectp);
        إذا (!rejectp) أرجع جديد AstConst{flp, AstConst::بیت_True{}};
        Ast_گره_عبارت* ثابت نتیجه_Exprp = جديد AstLogNot{flp, rejectp};
        أرجع نتیجه_Exprp;
    }

    صریح SvaNfaLowering(Ast_گره_Module* modp)
        : m_modp{modp}
        , m_u32DTypep{modp->پیدا_کن_Basic_DType(VBasicDTypeKwd::UINT32)} {}
    ~SvaNfaLowering() {
        V3Stats::جمع_کن_Stat_جمع("Assertions, NFA delay ring edge visits", m_stat_Delay_Ring_یال_Visits);
    }

    // Lower NFA graph to synthesizable AstAlways blocks and raw result signals.
    // Links are combinational; Edges are registered (NBA).
    Signal_مجموعه پایینی(Ast_گره_Cover_Or_Assert* ثابت assertp, Sva_گراف& گراف,
                    Ast_Sen_درخت* ثابت senTreep, Ast_گره_عبارت* ثابت تطابق_Condp,
                    Ast_گره_عبارت* ثابت disableExprp, ثابت منطقي need_Thread_Fail_شمار,
                    ثابت منطقي need_Throughout_Thread_Fail_شمار) {
        File_سطر* ثابت flp = assertp->fileline();
        AstCover* ثابت coverp = VN_CAST(assertp, Cover);
        ثابت منطقي isSeqEvent = coverp && coverp->isSeqEvent();
        ثابت VAssertType assertType
            = isSeqEvent ? VAssertType{VAssertType::داخلی} : assertp->userType();
        ثابت VAssertDirectiveType directiveType
            = isSeqEvent ? VAssertDirectiveType{VAssertDirectiveType::داخلی}
                         : assertp->directive();
        ثابت std::string پایه_Name = m_نامها.get("");

        // Number vertices with sequential colors for array indexing.
        صحيح N = 0;
        لكل (V3_گراف_رأس& vtxr : گراف.m_گراف.vertices()) { vtxr.color(N++); }
        std::وکتور<Sva_State_رأس*> vtx(N, اشاره_تهی);
        لكل (V3_گراف_رأس& vtxr : گراف.m_گراف.vertices()) {
            vtx[vtxr.color()] = static_cast<Sva_State_رأس*>(&vtxr);
        }

        // Allocate per-vertex lowering data (stored via V3GraphVertex::userp()).
        std::وکتور<Sva_رأس_داده> رأس_داده(N);
        لكل (صحيح i = 0; i < N; ++i) vtx[i]->userp(&رأس_داده[i]);

        // Identify registered vertices (targets of clocked edges).
        لكل (صحيح i = 0; i < N; ++i) {
            لكل (ثابت V3_گراف_یال& edger : vtx[i]->بیرون_یالها()) {
                ثابت Sva_Trans_یال& tedger = static_cast<ثابت Sva_Trans_یال&>(edger);
                ثابت صحيح مقصد_Idx = tedger.مقصد_Vtxp()->color();
                إذا (tedger.m_consumes_چرخه && tedger.مقصد_Vtxp() != گراف.m_تطابق_Vertexp
                    && !tedger.مقصد_Vtxp()->m_isRejectSink) {
                    vtx[مقصد_Idx]->datap()->needsReg = درست;
                }
            }
        }

        // A packed range ring can emit several live endpoints without needing per-slot counts.
        // Widen only a ring whose input can already carry multiplicity.
        std::وکتور<منطقي> stateMayHaveMultiplicity(N);
        std::وکتور<منطقي> ringNeedsMultiplicity(N);
        std::وکتور<صحيح> multiplicityWork;
        ثابت منطقي شمار_Matches = coverp && coverp->isCoverSeq() && !isSeqEvent;
        إذا (شمار_Matches) {
            لكل (صحيح i = 0; i < N; ++i) {
                ثابت منطقي مضرب_Inputs = vtx[i]->درون_یالها().اندازه() > 1;
                ثابت منطقي isRing = vtx[i]->m_delay_Ring_اندازه;
                ringNeedsMultiplicity[i] = isRing && مضرب_Inputs;
                ثابت منطقي startsMultiplicity = isRing
                                                    ? !vtx[i]->m_isFixedDelayRing
                                                    : مضرب_Inputs && !vtx[i]->datap()->needsReg;
                إذا (startsMultiplicity) {
                    stateMayHaveMultiplicity[i] = درست;
                    multiplicityWork.افزودن_پشت(i);
                }
            }
            لكل (اندازه_t wi = 0; wi < multiplicityWork.اندازه(); ++wi) {
                لكل (ثابت V3_گراف_یال& edger : vtx[multiplicityWork[wi]]->بیرون_یالها()) {
                    ثابت Sva_Trans_یال& tedger = static_cast<ثابت Sva_Trans_یال&>(edger);
                    ثابت صحيح ti = tedger.مقصد_Vtxp()->color();
                    ثابت منطقي isRing = vtx[ti]->m_delay_Ring_اندازه;
                    إذا (isRing) ringNeedsMultiplicity[ti] = درست;
                    // Match and reject sinks have no outgoing edges, so they stop naturally.
                    ثابت منطقي stopsMultiplicity = !isRing && vtx[ti]->datap()->needsReg;
                    إذا (stateMayHaveMultiplicity[ti] || stopsMultiplicity) { استمر; }
                    stateMayHaveMultiplicity[ti] = درست;
                    multiplicityWork.افزودن_پشت(ti);
                }
            }
        }

        ثابت منطقي asyncDisable = isAsyncDisable(disableExprp);
        // Registers cleared on a rising disable are also written by their clocked block.
        File_سطر* ثابت regFlp = asyncDisable ? جديد File_سطر{flp} : flp;
        إذا (asyncDisable) regFlp->warnOff(V3_Error_کد::MULTIDRIVEN, درست);

        AstVar* ثابت killVarp
            = جديد AstVar{flp, VVarType::MODULETEMP, پایه_Name + "__kill", m_u32DTypep};
        killVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->جمع_کن_Stmtsp(killVarp);
        لكل (صحيح i = 0; i < N; ++i) {
            إذا (vtx[i]->m_isAndCombiner) {
                ثابت std::string پایه = پایه_Name + "__a" + std::مقصد_string(i);
                AstVar* ثابت lp = جديد AstVar{regFlp, VVarType::MODULETEMP, پایه + "_doneL",
                                              m_modp->پیدا_کن_بیت_DType()};
                lp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جمع_کن_Stmtsp(lp);
                vtx[i]->datap()->انجامشده_LVarp = lp;
                AstVar* ثابت rp = جديد AstVar{regFlp, VVarType::MODULETEMP, پایه + "_doneR",
                                              m_modp->پیدا_کن_بیت_DType()};
                rp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جمع_کن_Stmtsp(rp);
                vtx[i]->datap()->انجامشده_RVarp = rp;
                استمر;
            }
            إذا (vtx[i]->m_delay_Ring_اندازه) {
                ثابت std::string پایه = پایه_Name + "__d" + std::مقصد_string(i);
                ثابت منطقي storesCounts = شمار_Matches && ringNeedsMultiplicity[i];
                Ast_گره_DType* ringDTypep = اشاره_تهی;
                إذا (storesCounts) {
                    // int unsigned ring [size];
                    ringDTypep = جديد AstUnpackArrayDType{
                        flp, m_u32DTypep,
                        جديد AstRange{flp, static_cast<صحيح>(vtx[i]->m_delay_Ring_اندازه - 1), 0}};
                    v3Global.rootp()->typeTablep()->جمع_کن_Typesp(ringDTypep);
                } وإلا {
                    // bit [size-1:0] ring;
                    ringDTypep = m_modp->پیدا_کن_بیت_DType(vtx[i]->m_delay_Ring_اندازه,
                                                      vtx[i]->m_delay_Ring_اندازه, VSigning::UNSIGNED);
                }
                AstVar* ثابت ringp
                    = جديد AstVar{flp, VVarType::MODULETEMP, پایه + "_ring", ringDTypep};
                ringp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جمع_کن_Stmtsp(ringp);
                vtx[i]->datap()->delayRingVarp = ringp;
                vtx[i]->datap()->delayRingCounts = storesCounts;
                AstVar* ثابت headp
                    = جديد AstVar{flp, VVarType::MODULETEMP, پایه + "_head",
                                 storesCounts ? m_u32DTypep : m_modp->پیدا_کن_بیت_DType()};
                headp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جمع_کن_Stmtsp(headp);
                vtx[i]->datap()->delay_Ring_سر_Varp = headp;
                // int unsigned idx;
                AstVar* ثابت idxp
                    = جديد AstVar{regFlp, VVarType::MODULETEMP, پایه + "_idx", m_u32DTypep};
                idxp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جمع_کن_Stmtsp(idxp);
                vtx[i]->datap()->delayRingIdxVarp = idxp;
                // int unsigned live_count;
                AstVar* ثابت زنده_شمار_Varp
                    = جديد AstVar{regFlp, VVarType::MODULETEMP, پایه + "_liveCount", m_u32DTypep};
                زنده_شمار_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جمع_کن_Stmtsp(زنده_شمار_Varp);
                vtx[i]->datap()->delay_Ring_زنده_شمار_Varp = زنده_شمار_Varp;
                AstVar* ثابت wrappedp = جديد AstVar{regFlp, VVarType::MODULETEMP,
                                                    پایه + "_wrapped", m_modp->پیدا_کن_بیت_DType()};
                wrappedp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جمع_کن_Stmtsp(wrappedp);
                vtx[i]->datap()->delayRingWrappedVarp = wrappedp;
                استمر;
            }
            إذا (!vtx[i]->datap()->needsReg) استمر;
            إذا (vtx[i] == گراف.m_شروع_Vertexp || vtx[i]->m_is_تطابق) استمر;
            ثابت std::string varName = پایه_Name + "__s" + std::مقصد_string(i);
            AstVar* ثابت varp
                = جديد AstVar{regFlp, VVarType::MODULETEMP, varName, m_modp->پیدا_کن_بیت_DType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جمع_کن_Stmtsp(varp);
            vtx[i]->datap()->stateVarp = varp;
        }

        // Build lowering context for phase sub-functions.
        پایینی_Ctx c{flp,        senTreep,      disableExprp, تطابق_Condp,
                   assertType, directiveType, killVarp,     گراف};

        // Phase 1: Resolve combinational Links via fixed-point propagation.
        Ast_گره_عبارت* ثابت triggerExprp
            = isSeqEvent ? جديد AstConst{flp, AstConst::بیت_True{}}
                         : assertOnCond(flp, assertp->userType(), assertp->directive());
        resolveLinks(c, triggerExprp, شمار_Matches);
        VL_DO_DANGLING(triggerExprp->delete_درخت(), triggerExprp);

        // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
        emit_State_ثبات_Nba(c);
        emitDelayRingUpdates(c, شمار_Matches);
        emit_And_Combiner_انجامشده_Latch_Nba(c);
        emitKillAckNba(c);

        // Phase 3/3a/3b: Compute terminal match/reject signals (cleans up stateSig).
        ثابت Signal_مجموعه sigs
            = compute_سیگنالها(c, need_Thread_Fail_شمار, need_Throughout_Thread_Fail_شمار, شمار_Matches);

        // Strong s_always[m:n] end-of-simulation liveness: if any in-window state
        // is still set at $finish, the universal-quantifier window never completed
        // (IEEE 1800-2023 16.12.11 strong semantics). Fire the assertion failure
        // from a final block; V3Assert turns the DT_ERROR display into the standard
        // "Assertion failed in %m" message.
        // pending = |{strong state registers, strong delay live counts != 0};
        Ast_گره_عبارت* pendingp = اشاره_تهی;
        لكل (صحيح i = 0; i < N; ++i) {
            إذا (!vtx[i]->m_strongPending) استمر;
            Ast_گره_عبارت* pendingExprp = اشاره_تهی;
            إذا (vtx[i]->datap()->stateVarp) {
                pendingExprp = جديد AstVarRef{flp, vtx[i]->datap()->stateVarp, VAccess::READ};
            } وإلا إذا (vtx[i]->m_strongAlwaysRing) {
                pendingExprp
                    = delay_Ring_Has_زنده_Threadsp(flp, vtx[i]->datap()->delay_Ring_زنده_شمار_Varp);
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
            Ast_گره_عبارت* ثابت condp = جديد AstLogAnd{flp, assertOnp, pendingp};
            AstDisplay* ثابت dispp
                = جديد AstDisplay{flp, VDisplayType::DT_ERROR, "", اشاره_تهی, اشاره_تهی};
            dispp->fmtp()->timeunit(m_modp->timeunit());
            Ast_گره_Stmt* ثابت firep = dispp;
            إذا (v3Global.opt.stopFail()) firep->جمع_کن_بعدی(جديد AstStop{flp, خطأ});
            m_modp->جمع_کن_Stmtsp(جديد AstFinal{flp, جديد AstIf{flp, condp, firep}});
        }

        // Clear userp on every vertex before vertexData is destroyed.
        لكل (صحيح i = 0; i < N; ++i) vtx[i]->userp(اشاره_تهی);
        أرجع sigs;
    }
};

}  // namespace

//######################################################################
// Top-level visitor

صنف AssertNfaVisitor final : عام VNVisitor {
    // STATE
    Ast_گره_Module* m_modp = اشاره_تهی;  // Current module being processed
    AstClocking* m_defaultClockingp = اشاره_تهی;  // Default clocking
    AstDefaultDisable* m_defaultDisablep = اشاره_تهی;  // Default disable iff
    SvaNfaLowering* m_loweringp = اشاره_تهی;  // NFA-to-hardware lowering engine
    Ast_Sen_درخت* m_sampled_مقدار_Clockp = اشاره_تهی;  // Inherited clock during scoped attachment
    V3_Unique_نامها m_prop_Var_نامها{"__Vpropvar"};  // Property-local variable names
    V3_Unique_نامها m_prop_موقت_نامها{"__VnfaSampled"};  // Hoisted $sampled(propp) temps
    V3_Unique_نامها m_fail_شمار_نامها{"__VnfaRemainingFailCount"};  // Fail replay counter names
    std::مجموعه<ثابت AstProperty*> m_inliningProps;  // Recursion guard for inlineNamedProperty

    قالب <نام_نوع T_گره>
    فراغ بازدید_Sampled_مقدار(T_گره* ثابت nodep) {
        إذا (m_sampled_مقدار_Clockp && !nodep->sentreep()) {
            nodep->sentreep(m_sampled_مقدار_Clockp->clone_درخت(درست));
        }
        iterateChildren(nodep);
    }

    // Wire match vertex and mid-window sources for a successful NFA build.
    ساكن فراغ wire_تطابق_And_Mid_Sources(Sva_گراف& گراف, ثابت Build_نتیجه& نتیجه, File_سطر* flp) {
        گراف.create_تطابق_رأس();
        // Skip the main term Link when midSources already cover every
        // end-of-match (cover_sequence path); otherwise the per-mid extraction
        // double-counts via the merge vertex.
        إذا (!نتیجه.جمله_Is_Mid_ادغام) { گراف.جمع_کن_Link(نتیجه.جمله_Vertexp, گراف.m_تطابق_Vertexp); }
        لكل (Sva_State_رأس* srcVtxp : نتیجه.midSources) {
            Ast_گره_عبارت* condp = اشاره_تهی;
            لكل (Ast_گره_عبارت* ثابت tc : srcVtxp->m_throughoutConds) {
                Ast_گره_عبارت* ثابت tcClone = tc->clone_درخت_خالص(خطأ);
                condp = condp ? جديد AstLogAnd{flp, condp, tcClone} : tcClone;
            }
            گراف.جمع_کن_Link(srcVtxp, گراف.m_تطابق_Vertexp, condp);
            srcVtxp->m_isUnbounded = درست;
        }
    }

    ساكن Ast_گره_عبارت* getSequenceBodyExprp(ثابت AstSequence* seqp) {
        Ast_گره* bodyp = seqp->stmtsp();
        طالما (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        أرجع VN_CAST(bodyp, گره_عبارت);
    }

    ساكن AstPropSpec* getPropertySpecp(ثابت AstProperty* propp) {
        Ast_گره* stmtp = propp->stmtsp();
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
        إذا (!m_inliningProps.emplace(propyp).دومی) {
            funcrefp->v3error("Illegal recursive property reference");  // LCOV_EXCL_LINE
            أرجع;  // LCOV_EXCL_LINE
        }
        بنية Guard final {
            std::مجموعه<ثابت AstProperty*>& setr;
            ثابت AstProperty* keyp;
            ~Guard() { setr.erase(keyp); }
        } guard{m_inliningProps, propyp};
        AstPropSpec* propSpecp = getPropertySpecp(propyp);
        UASSERT_OBJ(propSpecp, funcrefp, "Property has no body PropSpec");
        propSpecp = propSpecp->clone_درخت(خطأ);

        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propyp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_گره_عبارت*> پورت_Map;
        لكل (ثابت خودکار& tconnect : tconnects) {
            پورت_Map[tconnect.اولی] = tconnect.دومی->exprp();
        }

        // Promote property-local variables to module-level temps (IEEE 16.10).
        std::unordered_map<ثابت AstVar*, AstVar*> localVarMap;
        لكل (Ast_گره* stmtp = propyp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (!varp->isIO()) {
                    ثابت string newName = m_prop_Var_نامها.get(varp);
                    AstVar* ثابت newVarp = جديد AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->جمع_کن_Stmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        propSpecp->foreach([&](AstVarRef* refp) {
            ثابت خودکار پورت_It = پورت_Map.پیدا_کن(refp->varp());
            إذا (پورت_It != پورت_Map.انتها()) {
                refp->replaceWith(پورت_It->دومی->clone_درخت(خطأ));
                VL_DO_DANGLING(افزودن_Deletep(refp), refp);
                أرجع;
            }
            ثابت خودکار localIt = localVarMap.پیدا_کن(refp->varp());
            إذا (localIt != localVarMap.انتها()) refp->varp(localIt->دومی);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        لكل (ثابت خودکار& tconnect : tconnects) {
            افزودن_Deletep(tconnect.دومی->exprp()->unlink_Fr_پشت());
        }
        V3AssertCommon::checkSvaInstanceDelays(propSpecp, funcrefp);

        // Merge disable iff (IEEE 1800-2023 16.12.1)
        إذا (outerSpecp->disablep() && propSpecp->disablep()) {
            outerSpecp->v3error("disable iff expression before property call "
                                "and in its body is not legal");
            افزودن_Deletep(propSpecp->disablep()->unlink_Fr_پشت());
        }
        إذا (outerSpecp->disablep()) {
            propSpecp->disablep(outerSpecp->disablep()->unlink_Fr_پشت());
        }

        إذا (outerSpecp->sensesp() && propSpecp->sensesp()) {
            outerSpecp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Clock event before property call and in its body");
            افزودن_Deletep(propSpecp->sensesp()->unlink_Fr_پشت());
        }
        إذا (outerSpecp->sensesp()) {
            Ast_Sen_قلم* ثابت sensesp = outerSpecp->sensesp();
            sensesp->unlink_Fr_پشت();
            propSpecp->sensesp(sensesp);
        }

        outerSpecp->replaceWith(propSpecp);
        VL_DO_DANGLING(افزودن_Deletep(outerSpecp), outerSpecp);
    }

    فراغ inlineSequenceRef(AstFuncRef* funcrefp, AstSequence* seqp) {
        Ast_گره_عبارت* ثابت bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        Ast_گره_عبارت* ثابت clonedp = bodyExprp->clone_درخت(خطأ);

        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_گره_عبارت*> پورت_Map;
        لكل (ثابت خودکار& tconnect : tconnects) {
            پورت_Map[tconnect.اولی] = tconnect.دومی->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ثابت خودکار it = پورت_Map.پیدا_کن(refp->varp());
            إذا (it != پورت_Map.انتها()) {
                refp->replaceWith(it->دومی->clone_درخت(خطأ));
                VL_DO_DANGLING(افزودن_Deletep(refp), refp);
            }
        });
        لكل (ثابت خودکار& tconnect : tconnects) {
            افزودن_Deletep(tconnect.دومی->exprp()->unlink_Fr_پشت());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(افزودن_Deletep(funcrefp), funcrefp);
        // Clear referenced flag so V3AssertPre cleanup does not emit
        // spurious UNSUPPORTED for sequences that were already inlined here.
        seqp->isReferenced(خطأ);
    }

    // Must run before hasMultiCycleExpr() so NFA sees sequence bodies.
    فراغ inlineAllSequenceRefs(Ast_گره* rootp) {
        منطقي تغییریافته = درست;
        طالما (تغییریافته) {
            تغییریافته = خطأ;
            rootp->foreach([&](AstFuncRef* funcrefp) {
                إذا (تغییریافته) أرجع;
                إذا (AstSequence* ثابت seqp = VN_CAST(funcrefp->taskp(), Sequence)) {
                    inlineSequenceRef(funcrefp, seqp);
                    تغییریافته = درست;
                }
            });
        }
    }

    ساكن منطقي has_Multi_چرخه_عبارت(ثابت Ast_گره* nodep) {
        أرجع nodep->exists([](ثابت Ast_گره* np) {
            إذا (ثابت خودکار* ثابت ep = VN_CAST(np, گره_عبارت)) أرجع ep->is_Multi_چرخه_Sva();
            أرجع خطأ;
        });
    }

    ساكن VPropStrength effectiveAssertPropStrength(ثابت AstPropSpec* ثابت propSpecp) {
        إذا (propSpecp->propStrength() != VPropStrength::DEFAULT) أرجع propSpecp->propStrength();
        أرجع propSpecp->fileline()->language() <= V3_Lang_کد::L1800_2005 ? VPropStrength::STRONG
                                                                           : VPropStrength::WEAK;
    }

    // Bare `assert property (p until q)` with boolean operands stays on
    // V3AssertPre's AstLoop lowering, which preserves per-attempt action-block
    // firings that this NFA's single-bit aggregated state cannot. Strong bare
    // forms are also lowered there. NFA still owns sequence operands and any
    // embedding inside a multi-cycle context (implication consequent, or/and
    // operands, etc.).
    ساكن منطقي is_Bare_بالاترین_Level_Until(Ast_گره* propp) {
        Ast_گره* p = propp;
        إذا (AstPropSpec* ثابت specp = VN_CAST(p, PropSpec)) p = specp->propp();
        طالما (AstLogNot* ثابت notp = VN_CAST(p, LogNot)) p = notp->lhsp();
        AstUntil* ثابت untilp = VN_CAST(p, Until);
        إذا (!untilp) أرجع خطأ;
        ثابت خودکار hasSeq = [](ثابت Ast_گره_عبارت* ep) {
            أرجع ep->exists([](ثابت Ast_گره_عبارت* np) { أرجع np->is_Multi_چرخه_Sva(); });
        };
        إذا (hasSeq(untilp->lhsp()) || hasSeq(untilp->rhsp())) أرجع خطأ;
        أرجع درست;
    }

    بنية PropertyParts final {
        Ast_گره_عبارت* triggerExprp = اشاره_تهی;
        Ast_گره_عبارت* seqExprp = اشاره_تهی;
        منطقي isOverlapped = درست;
        منطقي hasImplication = خطأ;
        منطقي isFollowedBy = خطأ;  // True for #-# / #=# (non-vacuous-fail on antecedent miss)
    };

    ساكن PropertyParts decomposeProperty(Ast_گره* propp) {
        PropertyParts parts;
        إذا (AstPropSpec* ثابت specp = VN_CAST(propp, PropSpec)) { propp = specp->propp(); }
        إذا (AstImplication* ثابت implp = VN_CAST(propp, Implication)) {
            parts.hasImplication = درست;
            parts.isOverlapped = implp->isOverlapped();
            parts.isFollowedBy = implp->isFollowedBy();
            parts.triggerExprp = implp->lhsp();
            parts.seqExprp = implp->rhsp();
        } وإلا إذا (Ast_گره_عبارت* ثابت exprp = VN_CAST(propp, گره_عبارت)) {
            parts.triggerExprp = اشاره_تهی;
            parts.seqExprp = exprp;
        }
        أرجع parts;
    }

    ساكن منطقي can_تقسیم_Implication_Pass_Actions(ثابت PropertyParts& parts) {
        UASSERT(parts.hasImplication,
                "Implication pass action split requested without implication");
        UASSERT(parts.triggerExprp, "Implication pass action split requested without trigger");
        // Direct vacuous/nonvacuous classification uses the antecedent value in the current
        // assertion attempt. Leave delayed antecedents on the existing NFA pass path.
        أرجع !has_Multi_چرخه_عبارت(parts.triggerExprp);
    }

    ساكن فراغ تقسیم_Implication_Pass_Actions(AstAssert* assertp, ثابت PropertyParts& parts,
                                            Ast_گره_عبارت* nonvacuousMatchp) {
        File_سطر* ثابت flp = assertp->fileline();

        Ast_گره* ثابت passsp = assertp->passsp()->unlink_Fr_پشت_With_بعدی();
        Ast_گره* splitsp = اشاره_تهی;

        إذا (!parts.isFollowedBy) {
            Ast_گره_عبارت* ثابت vacuousp
                = جديد AstLogNot{flp, sampled(parts.triggerExprp->clone_درخت_خالص(خطأ))};
            Ast_گره* ثابت vacuousBodyp = passsp->clone_درخت(خطأ);
            splitsp = newPassOnIf(flp, vacuousp, vacuousBodyp, assertp->userType(),
                                  assertp->directive(), /*vacuous=*/درست);
        }

        AstIf* ثابت nonvacuousp
            = newPassOnIf(flp, nonvacuousMatchp, passsp, assertp->userType(), assertp->directive(),
                          /*vacuous=*/خطأ);
        splitsp = splitsp ? Ast_گره::جمع_کن_بعدی<Ast_گره, Ast_گره>(splitsp, nonvacuousp)
                          : static_cast<Ast_گره*>(nonvacuousp);
        assertp->جمع_کن_Passsp(splitsp);
    }

    // On a PropSpec-wrapped assertion whose NFA build failed with a semantic
    // error (errorEmitted), replace the body with a BitFalse const so later
    // passes see a well-formed AST. Returns true if replaced.
    فراغ replaceBodyOnBuildError(File_سطر* flp, AstPropSpec* propSpecp, منطقي errorEmitted) {
        إذا (!errorEmitted) أرجع;
        Ast_گره* ثابت innerPropp = propSpecp->propp();
        innerPropp->replaceWith(جديد AstConst{flp, AstConst::بیت_False{}});
        VL_DO_DANGLING(افزودن_Deletep(innerPropp), innerPropp);
    }

    // Hoist a leading clocking event (IEEE 1800-2023 16.7):
    منطقي hoistClockedSeq(AstPropSpec* specp) {
        طالما (AstSClocked* ثابت clockedp = VN_CAST(specp->propp(), SClocked)) {
            إذا (specp->sensesp()) {
                clockedp->v3warn(E_UNSUPPORTED, "Unsupported: multiclocked sequence or property");
                replaceBodyOnBuildError(specp->fileline(), specp, درست);
                أرجع درست;
            }
            لكل (ثابت Ast_Sen_قلم* sp = clockedp->sensesp(); sp;
                 sp = VN_CAST(sp->nextp(), Sen_قلم)) {
                إذا (!sp->یال_Type().an_یال()) {
                    clockedp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: non-edge clocking event on a sequence; "
                                     "use an edge such as @(posedge clk)");
                    replaceBodyOnBuildError(specp->fileline(), specp, درست);
                    أرجع درست;
                }
            }
            specp->sensesp(clockedp->sensesp()->unlink_Fr_پشت_With_بعدی());
            Ast_گره_عبارت* ثابت bodyp = clockedp->exprp()->unlink_Fr_پشت();
            clockedp->replaceWith(bodyp);
            VL_DO_DANGLING(افزودن_Deletep(clockedp), clockedp);
        }
        // A clocking event anywhere else in the sequence is not supported.
        ثابت AstSClocked* nestedp = اشاره_تهی;
        specp->propp()->foreach([&](ثابت AstSClocked* p) {
            إذا (!nestedp) nestedp = p;
        });
        إذا (nestedp) {
            nestedp->v3warn(E_UNSUPPORTED,
                            "Unsupported: clocking event inside sequence expression");
            replaceBodyOnBuildError(specp->fileline(), specp, درست);
            أرجع درست;
        }
        أرجع خطأ;
    }

    // Build the NFA graph for a property body, handling both the antecedent
    // |-> consequent and simple sequence cases. Returns the consequent/body
    // BuildResult (invalid on parse/build failure).
    Build_نتیجه build_Assertion_گراف(SvaNfaBuilder& builder, Sva_گراف& گراف, Ast_گره_عبارت* seqBodyp,
                                    ثابت PropertyParts& parts, File_سطر* flp) {
        إذا (!parts.hasImplication) أرجع builder.build(seqBodyp);

        گراف.m_شروع_Vertexp = گراف.create_State_رأس();
        أرجع builder.build_Implication_یالها(parts.triggerExprp, seqBodyp, گراف.m_شروع_Vertexp,
                                             parts.isOverlapped, parts.isFollowedBy,
                                             parts.triggerExprp, flp);
    }

    // Install pass-action gating and replay simultaneous per-thread failures.
    فراغ attachActionHandlers(AstAssert* ثابت assertp, Ast_گره_عبارت* تطابق_Exprp,
                              Ast_Sen_درخت* ثابت threadFailReplaySenTreep,
                              Ast_گره_عبارت* ثابت threadFailCountp) {
        // Gate pass handler on match to prevent vacuous-pass firings.
        إذا (تطابق_Exprp) {
            // needMatch implies passsp() was non-null when evaluated above;
            // lowering does not mutate the assert's pass-action between the
            // two reads, so passsp() is still non-null here.
            Ast_گره* passsp = assertp->passsp();
            UASSERT_OBJ(passsp, assertp, "needMatch set but passsp is null");
            passsp->unlink_Fr_پشت_With_بعدی();
            File_سطر* ثابت flp = assertp->fileline();
            AstIf* ثابت ifp = جديد AstIf{flp, تطابق_Exprp, passsp, اشاره_تهی};
            assertp->جمع_کن_Passsp(ifp);
            // Fail-handler prefix for overlapping instances (IEEE 16.12):
            // fires when reject=1 && match=1 in the same cycle.
            إذا (assertp->failsp()) assertp->prependFailsp(ifp->clone_درخت(خطأ));
        }

        إذا (threadFailCountp) {
            UASSERT_OBJ(threadFailReplaySenTreep, assertp,
                        "Thread fail count missing sensitivity tree");
            Ast_گره* ثابت failsp = assertp->failsp();
            File_سطر* ثابت flp = assertp->fileline();
            // IEEE 1800-2023 16.12 requires one action-block evaluation per failed
            // thread. AstAssert handles the first, so replay the rest here.
            AstVar* ثابت مانده_Fail_شمار_Varp
                = جديد AstVar{flp, VVarType::MODULETEMP, m_fail_شمار_نامها.get(""),
                             m_modp->پیدا_کن_Basic_DType(VBasicDTypeKwd::UINT32)};
            مانده_Fail_شمار_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جمع_کن_Stmtsp(مانده_Fail_شمار_Varp);
            Ast_گره* ثابت replayStmtsp = جديد Ast_تخصیص_بده{
                flp, جديد AstVarRef{flp, مانده_Fail_شمار_Varp, VAccess::WRITE}, threadFailCountp};
            AstLoop* ثابت replayLoopp = جديد AstLoop{flp};
            replayLoopp->جمع_کن_Stmtsp(جديد AstLoopTest{
                flp, replayLoopp,
                جديد AstGt{flp, جديد AstVarRef{flp, مانده_Fail_شمار_Varp, VAccess::READ},
                          newTypedConstp(flp, مانده_Fail_شمار_Varp->dtypep(), 1)}});
            replayLoopp->جمع_کن_Stmtsp(newIfAssertFailOn(failsp->clone_درخت(درست), assertp->directive(),
                                                     assertp->userType()));
            AstSub* ثابت decrementedFailCountp
                = جديد AstSub{flp, جديد AstVarRef{flp, مانده_Fail_شمار_Varp, VAccess::READ},
                             newTypedConstp(flp, مانده_Fail_شمار_Varp->dtypep(), 1)};
            replayLoopp->جمع_کن_Stmtsp(
                جديد Ast_تخصیص_بده{flp, جديد AstVarRef{flp, مانده_Fail_شمار_Varp, VAccess::WRITE},
                              decrementedFailCountp});
            replayStmtsp->جمع_کن_بعدی(replayLoopp);
            m_modp->جمع_کن_Stmtsp(
                جديد AstAlways{flp, VAlwaysKwd::ALWAYS, threadFailReplaySenTreep, replayStmtsp});
        }
    }

    // Replace one VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). No-op if refp is not in matchMap.
    فراغ substitute_تطابق_قلم_Ref(AstVarRef* refp, بیعلامت K,
                                ثابت std::unordered_map<ثابت AstVar*, Ast_گره_عبارت*>& تطابق_Map) {
        ثابت خودکار it = تطابق_Map.پیدا_کن(refp->varp());
        إذا (it == تطابق_Map.انتها()) أرجع;
        Ast_گره_عبارت* newp = it->دومی->clone_درخت_خالص(خطأ);
        إذا (K > 0) {
            AstConst* ثابت ticksp = جديد AstConst{refp->fileline(), AstConst::Widthed_مقدار{}, 32,
                                                  static_cast<uint32_t>(K)};
            AstPast* ثابت pastp = جديد AstPast{refp->fileline(), newp, ticksp, اشاره_تهی,
                                               /* propertyTiming */ درست};
            pastp->dtypeFrom(newp);
            newp = pastp;
        }
        refp->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(refp), refp);
        أرجع;
    }

    // Recursively walk a consequent. Returns cycle length consumed and
    // substitutes each VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). Reports E_UNSUPPORTED on non-constant
    // delays or composite sequence operators and returns -1 on failure.
    صحيح
    walk_Substitute_تطابق_اقلام(Ast_گره_عبارت* nodep, بیعلامت K,
                             ثابت std::unordered_map<ثابت AstVar*, Ast_گره_عبارت*>& تطابق_اقلام) {
        إذا (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            // IEEE 1800-2023 16.9.2: cycle_delay's lhsp is a constant_expression
            // and the delay form in a sequence is always `##N`, folded by
            // V3Const + V3Param before V3AssertNfa. Range form `##[m:n]` is the
            // only user-visible reject here.
            AstDelay* ثابت delayp = VN_AS(sexprp->delayp(), Delay);
            UASSERT_OBJ(delayp->is_چرخه_Delay() && VN_IS(delayp->lhsp(), Const), sexprp,
                        "SVA cycle delay must have a constant lhsp");
            إذا (delayp->isRangeDelay()) {
                sexprp->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                              "non-constant cycle delay in consequent"
                                              " (IEEE 1800-2023 16.10)");
                أرجع -1;
            }
            ثابت بیعلامت delay_چرخهها = VN_AS(delayp->lhsp(), Const)->مقصد_UInt();
            صحيح preLen = 0;
            إذا (Ast_گره_عبارت* ثابت prep = sexprp->preExprp()) {
                preLen = walk_Substitute_تطابق_اقلام(prep, K, تطابق_اقلام);
                إذا (preLen < 0) أرجع -1;
            }
            ثابت صحيح bodyLen
                = walk_Substitute_تطابق_اقلام(sexprp->exprp(), K + preLen + delay_چرخهها, تطابق_اقلام);
            إذا (bodyLen < 0) أرجع -1;
            أرجع preLen + delay_چرخهها + bodyLen;
        }
        إذا (nodep->is_Multi_چرخه_Sva()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                         "composite sequence operator in consequent"
                                         " (IEEE 1800-2023 16.10)");
            أرجع -1;
        }
        std::وکتور<AstVarRef*> refs;
        nodep->foreach([&refs](AstVarRef* p) { refs.افزودن_پشت(p); });
        لكل (AstVarRef* ثابت refp : refs) substitute_تطابق_قلم_Ref(refp, K, تطابق_اقلام);
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
    منطقي lift_تطابق_قلم_Substitutions(PropertyParts& parts, Ast_گره_عبارت* seqBodyp) {
        إذا (!parts.hasImplication) أرجع خطأ;
        Ast_عبارت_Stmt* ثابت عبارت_Stmtp = VN_CAST(parts.triggerExprp, عبارت_Stmt);
        إذا (!عبارت_Stmtp) أرجع خطأ;
        // IEEE 1800-2023 16.10 BNF requires `(expr, match_item {, match_item})`
        // with at least one match item; V3LinkParse only emits ExprStmt for
        // this form and only emits AstAssign with VarRef LHS for each item.
        std::unordered_map<ثابت AstVar*, Ast_گره_عبارت*> تطابق_اقلام;
        لكل (Ast_گره* stmtp = عبارت_Stmtp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_تخصیص_بده* ثابت assignp = VN_AS(stmtp, تخصیص_بده);
            AstVarRef* ثابت lhsRefp = VN_AS(assignp->lhsp(), VarRef);
            تطابق_اقلام[lhsRefp->varp()] = assignp->rhsp();
        }
        ثابت بیعلامت شروع_K = parts.isOverlapped ? 0 : 1;
        ثابت صحيح درازی = walk_Substitute_تطابق_اقلام(seqBodyp, شروع_K, تطابق_اقلام);
        // Match-item substitution / strip mutates ancestor purity. Release
        // builds don't auto-clear caches on edits, so refresh here.
        VIsCached::clear_Cache_درخت();
        إذا (درازی < 0) أرجع درست;
        Ast_گره_عبارت* ثابت antBoolp = عبارت_Stmtp->resultp()->unlink_Fr_پشت();
        عبارت_Stmtp->replaceWith(antBoolp);
        VL_DO_DANGLING(افزودن_Deletep(عبارت_Stmtp), عبارت_Stmtp);
        parts.triggerExprp = antBoolp;
        أرجع خطأ;
    }

    // Outcome counts for a property if/case are wrong in an outcome-multiplying
    // context. Returns that context, or nullptr when the shape is supported.
    ساكن ثابت حرف* unsupportedPropertyControl(ثابت Ast_گره_Cover_Or_Assert* assertp,
                                                  ثابت Ast_گره_عبارت* seqBodyp, منطقي negated) {
        إذا (!hasPropertyControlConjunction(seqBodyp)) أرجع اشاره_تهی;
        إذا (negated) أرجع "negation";
        إذا (VN_IS(assertp, Cover)) أرجع "cover";
        إذا (VN_AS(assertp, Assert)->passsp()) أرجع "a pass action";
        أرجع اشاره_تهی;
    }

    // Inline property/sequence refs and reject unsupported shapes.
    // Returns the PropSpec to lower, or nullptr when fully handled here.
    AstPropSpec* prepareAssertionProp(Ast_گره_Cover_Or_Assert* assertp) {
        إذا (AstPropSpec* ثابت specp = VN_CAST(assertp->propp(), PropSpec)) {
            إذا (AstFuncRef* ثابت funcrefp = VN_CAST(specp->propp(), FuncRef)) {
                إذا (ثابت AstProperty* ثابت propyp = VN_CAST(funcrefp->taskp(), Property)) {
                    inlineNamedProperty(specp, funcrefp, propyp);
                }
            }
        }

        inlineAllSequenceRefs(assertp->propp());

        إذا (AstPropSpec* ثابت specp = VN_CAST(assertp->propp(), PropSpec)) {
            إذا (hoistClockedSeq(specp)) أرجع اشاره_تهی;
        }

        AstPropSpec* ثابت propp = VN_AS(assertp->propp(), PropSpec);
        إذا (!VN_IS(assertp, Cover)
            && effectiveAssertPropStrength(propp) == VPropStrength::STRONG) {
            propp->v3warn(E_UNSUPPORTED,
                          "Unsupported: strong property in " + assertp->verilogKwd() + ".");
            replaceBodyOnBuildError(assertp->fileline(), propp, /*errorEmitted=*/درست);
            أرجع اشاره_تهی;
        }

        إذا (!has_Multi_چرخه_عبارت(propp)) أرجع اشاره_تهی;
        // A nested property instance keeps its body behind the call; lowering would drop it.
        إذا (propp->exists([](ثابت AstFuncRef* refp) { أرجع VN_IS(refp->taskp(), Property); })) {
            assertp->v3warn(E_UNSUPPORTED,
                            "Unsupported: property instance inside a multi-cycle property "
                            "expression");
            VL_DO_DANGLING(افزودن_Deletep(assertp->unlink_Fr_پشت()), assertp);
            أرجع اشاره_تهی;
        }
        إذا (is_Bare_بالاترین_Level_Until(propp)) أرجع اشاره_تهی;
        أرجع propp;
    }

    فراغ processAssertion(Ast_گره_Cover_Or_Assert* assertp) {
        إذا (assertp->فوری()) أرجع;

        AstPropSpec* ثابت propp = prepareAssertionProp(assertp);
        إذا (!propp) أرجع;

        PropertyParts parts = decomposeProperty(propp);
        UASSERT_OBJ(parts.seqExprp, propp, "Property body must be an expression");

        // Unwrap `not` (IEEE 1800-2023 16.12.1); odd count -> negated semantics.
        Ast_گره_عبارت* seqBodyp = parts.seqExprp;
        منطقي negated = خطأ;
        طالما (AstLogNot* ثابت notp = VN_CAST(seqBodyp, LogNot)) {
            negated = !negated;
            seqBodyp = notp->lhsp();
        }

        ثابت حرف* ثابت propertyControlp
            = unsupportedPropertyControl(assertp, seqBodyp, negated);

        // Substitute property-local match-item refs in consequent with
        // $past(rhs, K) before NFA build (IEEE 1800-2023 16.10).
        إذا (lift_تطابق_قلم_Substitutions(parts, seqBodyp)) {
            AstPropSpec* ثابت psp = VN_CAST(assertp->propp(), PropSpec);
            UASSERT_OBJ(psp, assertp, "Concurrent assertion must have PropSpec");
            replaceBodyOnBuildError(assertp->fileline(), psp, /*errorEmitted=*/درست);
            أرجع;
        }

        AstCover* ثابت coverp = VN_CAST(assertp, Cover);
        ثابت منطقي isCoverSeq = coverp && coverp->isCoverSeq();
        // A sequence event control is not an assertion directive; no default
        // disable iff, no assertion control
        ثابت منطقي isSeqEvent = coverp && coverp->isSeqEvent();
        ثابت منطقي شمار_Matches = isCoverSeq && !isSeqEvent;
        // Inherit module defaults (IEEE 14.12, 16.15) when assertion has none.
        إذا (!propp->sensesp() && m_defaultClockingp) {
            propp->sensesp(m_defaultClockingp->sentreep()->sensesp()->clone_درخت(درست));
        }
        إذا (!propp->disablep() && m_defaultDisablep && !isSeqEvent) {
            propp->disablep(m_defaultDisablep->condp()->clone_درخت_خالص(درست));
        }
        إذا (!propp->sensesp()) أرجع;
        Ast_Sen_درخت* senTreep
            = جديد Ast_Sen_درخت{propp->fileline(), propp->sensesp()->clone_درخت(درست)};
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

        Sva_گراف گراف;
        SvaNfaBuilder builder{گراف,      m_modp,     m_prop_موقت_نامها,
                              isCoverSeq, isSeqEvent, coverp != اشاره_تهی};

        ثابت Build_نتیجه نتیجه = build_Assertion_گراف(builder, گراف, seqBodyp, parts, flp);
        إذا (نتیجه.valid()) wire_تطابق_And_Mid_Sources(گراف, نتیجه, flp);
        إذا (!نتیجه.valid()) {
            // Fall through to V3AssertPre for unsupported constructs; only
            // replace the body on real semantic errors. Any hoisted temps
            // from this attempt become orphan MODULETEMPs; V3Dead removes
            // them along with the dead always_comb driver.
            replaceBodyOnBuildError(flp, propp, نتیجه.errorEmitted);
            VL_DO_DANGLING(افزودن_Deletep(senTreep), senTreep);
            أرجع;
        }
        // After the build, so a construct the builder rejects reports itself.
        إذا (propertyControlp) {
            seqBodyp->v3warn(E_UNSUPPORTED,
                             "Unsupported: temporal property if/case with " << propertyControlp);
            replaceBodyOnBuildError(flp, propp, /*errorEmitted=*/درست);
            VL_DO_DANGLING(افزودن_Deletep(senTreep), senTreep);
            أرجع;
        }
        إذا (شمار_Matches && !v3Global.opt.coverageUser()) {
            SvaNfaBuilder::freeUnlinkedCondp(نتیجه.finalCondp);
            VL_DO_DANGLING(افزودن_Deletep(senTreep), senTreep);
            Ast_گره* ثابت innerPropp = propp->propp();
            innerPropp->replaceWith(جديد AstConst{flp, AstConst::بیت_False{}});
            VL_DO_DANGLING(افزودن_Deletep(innerPropp), innerPropp);
            أرجع;
        }

        AstAssert* ثابت assertAssertp = VN_CAST(assertp, Assert);
        ثابت منطقي تقسیم_Implication_Passsp = assertAssertp && assertAssertp->passsp()
                                            && parts.hasImplication
                                            && can_تقسیم_Implication_Pass_Actions(parts);
        ثابت منطقي need_تطابق = assertAssertp && assertAssertp->passsp()
                               && (!parts.hasImplication || تقسیم_Implication_Passsp);

        ثابت منطقي needsThreadFailReplay
            = assertAssertp && assertAssertp->failsp() && !parts.hasImplication;
        ثابت خودکار سیگنالها
            = m_loweringp->پایینی(assertp, گراف, senTreep, نتیجه.finalCondp, disableExprp,
                                 needsThreadFailReplay, needsThreadFailReplay && !negated);
        Ast_گره_عبارت* تطابق_Exprp = اشاره_تهی;
        Ast_گره_عبارت* outputExprp = m_loweringp->assemble_نتیجه(
            assertp, negated, نتیجه.finalCondp, سیگنالها, need_تطابق ? &تطابق_Exprp : اشاره_تهی);
        إذا (شمار_Matches) {
            UASSERT_OBJ(سیگنالها.تطابق_Countp, coverp, "Cover sequence missing match count");
            VL_DO_DANGLING(outputExprp->delete_درخت(), outputExprp);
            propp->تطابق_Countp(سیگنالها.تطابق_Countp);
            outputExprp = جديد AstNeq{flp, سیگنالها.تطابق_Countp->clone_درخت_خالص(خطأ),
                                     newTypedConstp(flp, سیگنالها.تطابق_Countp->dtypep(), 0)};
        }

        Ast_Sen_درخت* ثابت threadFailReplaySenTreep
            = سیگنالها.threadFailCountp ? senTreep->clone_درخت(خطأ) : اشاره_تهی;

        VL_DO_DANGLING(افزودن_Deletep(senTreep), senTreep);
        إذا (نتیجه.finalCondp && !نتیجه.finalCondp->backp()) افزودن_Deletep(نتیجه.finalCondp);

        إذا (تقسیم_Implication_Passsp) {
            تقسیم_Implication_Pass_Actions(assertAssertp, parts, تطابق_Exprp);
        } وإلا {
            attachActionHandlers(assertAssertp, تطابق_Exprp, threadFailReplaySenTreep,
                                 سیگنالها.threadFailCountp);
        }

        Ast_گره* ثابت innerPropp = propp->propp();
        innerPropp->replaceWith(outputExprp);
        VL_DO_DANGLING(افزودن_Deletep(innerPropp), innerPropp);

        UINFO(4, "NFA converted assertion at " << flp << پایان_سطر);

        إذا (dump_گراف_Level() >= 6) گراف.m_گراف.dumpDotFilePrefixed("assert-nfa");
    }

    // VISITORS
    فراغ بازدید(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_loweringp);
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultDisablep);
        m_modp = nodep;
        // A class nested in a module is in the scope of the module's default clocking
        إذا (!VN_IS(nodep, Class)) m_defaultClockingp = اشاره_تهی;
        m_defaultDisablep = nodep->defaultDisablep();
        SvaNfaLowering lowering{nodep};
        m_loweringp = &lowering;
        iterateChildren(nodep);
    }
    فراغ بازدید(AstClocking* nodep) override {
        إذا (nodep->isDefault() && !m_defaultClockingp) m_defaultClockingp = nodep;
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_Gen_بلوک* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    فراغ بازدید(AstDefaultDisable* nodep) override {}
    فراغ بازدید(AstFell* nodep) override { بازدید_Sampled_مقدار(nodep); }
    فراغ بازدید(AstPast* nodep) override { بازدید_Sampled_مقدار(nodep); }
    فراغ بازدید(AstRose* nodep) override { بازدید_Sampled_مقدار(nodep); }
    فراغ بازدید(AstStable* nodep) override { بازدید_Sampled_مقدار(nodep); }
    فراغ بازدید(AstAssert* nodep) override { processAssertion(nodep); }
    فراغ بازدید(AstCover* nodep) override { processAssertion(nodep); }
    فراغ بازدید(AstRestrict* nodep) override {
        // Restrict property is ignored by simulators (IEEE 1800-2023 16.12.2).
        // Remove here so temporal SExpr don't leak to V3AssertPre.
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
    }
    فراغ بازدید(AstAssertIntrinsic* nodep) override {}
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    صریح AssertNfaVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Top entry point

فراغ V3AssertNfa::assertNfaAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":" << پایان_سطر);
    { AssertNfaVisitor{nodep}; }
    V3Global::dump_Check_Global_درخت("assertnfa", 0, dump_درخت_Either_Level() >= 3);
}
