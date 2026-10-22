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

نامخلا {

جماعت Sva_State_راس;

// Per-vertex algorithm data, stored via V3GraphVertex::userp() during lowering
ساخت Sva_راس_ڈیٹا final {
    AstVar* stateVarp = خالی_اشاریہ;  // NBA state register for this vertex
    AstVar* delayRingVarp = خالی_اشاریہ;  // Packed occupancy bits or per-slot match counts
    AstVar* delay_Ring_سر_Varp
        = خالی_اشاریہ;  // Cached outgoing slot; isolates readers from in-place writes
    AstVar* delayRingIdxVarp = خالی_اشاریہ;  // Next slot written in delayRingVarp
    AstVar* delay_Ring_زندہ_گنتی_Varp = خالی_اشاریہ;  // Number of threads in delayRingVarp
    AstVar* delayRingWrappedVarp = خالی_اشاریہ;  // All slots written since the last clear
    AstVar* مکمل_LVarp = خالی_اشاریہ;  // SAnd LHS done-latch
    AstVar* مکمل_RVarp = خالی_اشاریہ;  // SAnd RHS done-latch
    Ast_گرہ_اظہاریہ* stateSigp = خالی_اشاریہ;  // Combinational state signal; OWNED during lowering
    Ast_گرہ_اظہاریہ* state_گنتی_Sigp = خالی_اشاریہ;  // Cover-sequence match count; OWNED during lowering
    Ast_گرہ_اظہاریہ* state_گنتی_حد_سے_تجاوز_Sigp = خالی_اشاریہ;  // Count overflow; OWNED during lowering
    منطقی delayRingCounts = جھوٹ;  // Ring slots hold cover-sequence match counts
    منطقی needsReg = جھوٹ;  // True if vertex has incoming clocked edge
};

// NFA state vertex -- one per NFA position in the sequence evaluation
جماعت Sva_State_راس final : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Sva_State_راس, V3_گراف_راس)
عوامی:
    // True if this is the sequence-match terminal vertex
    منطقی m_is_مطابقت = جھوٹ;
    // OWNED throughout-guard condition clones; IEEE 1800-2023 16.9.9
    std::سمتیہ<Ast_گرہ_اظہاریہ*> m_throughoutConds;
    // Nonzero for a ring-buffer vertex for ## delays.
    منطقی m_isFixedDelayRing = جھوٹ;
    غیرعلامتی m_delay_Ring_سائز = 0;  // Number of ring slots. Range: max-min+1.
    Ast_گرہ_اظہاریہ* m_delayRingClearCondp = خالی_اشاریہ;  // local RHS for pure-boolean range
    Ast_گرہ_اظہاریہ* m_delayRingAdvanceCondp = خالی_اشاریہ;  // Advance only when this condition holds
    منطقی m_replayAbortReject = جھوٹ;  // Compressed repetition needs per-thread abort replay
    // OWNED; enclosing-abort fire condition clearing state or suppressing guard rejection
    Ast_گرہ_اظہاریہ* m_abortClearp = خالی_اشاریہ;
    // OWNED; reject-abort fire condition rejecting all represented live threads
    Ast_گرہ_اظہاریہ* m_abortRejectp = خالی_اشاریہ;
    // Liveness terminal (IEEE weak semantics): reject must not fire from this source
    منطقی m_isUnbounded = جھوٹ;
    // Temporal sequence AND combiner; IEEE 1800-2023 16.9.5
    منطقی m_isAndCombiner = جھوٹ;
    Sva_State_راس* m_andLhsTermp = خالی_اشاریہ;  // LHS sub-NFA terminal vertex
    Sva_State_راس* m_andRhsTermp = خالی_اشاریہ;  // RHS sub-NFA terminal vertex
    Ast_گرہ_اظہاریہ* m_andLhsCondp = خالی_اشاریہ;  // OWNED; LHS final condition (may be nullptr)
    Ast_گرہ_اظہاریہ* m_andRhsCondp = خالی_اشاریہ;  // OWNED; RHS final condition (may be nullptr)
    // Reject sink for SAnd rejectOnFail wiring; not a state-signal source
    منطقی m_isRejectSink = جھوٹ;
    // In-window vertex of a strong s_always[m:n]: if its state is still set at
    // end-of-simulation the universal-quantifier window never completed, which is
    // a liveness failure (IEEE 1800-2023 16.12.11 strong semantics).
    منطقی m_strongPending = جھوٹ;
    // Ring introduced for the checked portion of a bounded strong s_always window.
    منطقی m_strongAlwaysRing = جھوٹ;

    // CONSTRUCTORS
    صریح Sva_State_راس(V3_گراف* graphp)
        : V3_گراف_راس{graphp} {}
    ~Sva_State_راس() override {
        برائے (Ast_گرہ_اظہاریہ* cp : m_throughoutConds) VL_DO_DANGLING(cp->delete_درخت(), cp);
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
        string name = "s" + cvt_مقصد_Str(color());
        اگر (m_delay_Ring_سائز) {
            name += "\\n";
            name += m_isFixedDelayRing ? "fixed chain " : "range chain ";
            name += cvt_مقصد_Str(m_delay_Ring_سائز) + " slots";
        }
        واپس name;
    }
    string dotColor() ثابت override {
        اگر (m_is_مطابقت) واپس "red";
        اگر (m_delay_Ring_سائز) واپس "blue";
        اگر (m_isAndCombiner) واپس "purple";
        واپس "black";
    }
    // LCOV_EXCL_STOP
    // Access per-vertex algorithm data (valid only during lowering phase)
    Sva_راس_ڈیٹا* datap() ثابت { واپس static_cast<Sva_راس_ڈیٹا*>(userp()); }
};

// NFA transition edge -- clocked (##1) or combinational link (##0)
جماعت Sva_Trans_کنارہ final : عوامی V3_گراف_کنارہ {
    VL_RTTI_IMPL(Sva_Trans_کنارہ, V3_گراف_کنارہ)
عوامی:
    Ast_گرہ_اظہاریہ* m_condp;  // Transition condition; nullptr = unconditional; OWNED
    منطقی m_consumes_چکر;  // true = clocked edge (##1), false = link (##0/boolean)
    // Reject when source is active and condp is false; set only on
    // outermost required-step Link
    منطقی m_rejectOnFail = جھوٹ;
    // Optional dynamic condition vertex for m_rejectOnFail. Used when the
    // success condition is another NFA state rather than a static expression.
    Sva_State_راس* m_condVtxp = خالی_اشاریہ;

    // CONSTRUCTORS
    Sva_Trans_کنارہ(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی, Ast_گرہ_اظہاریہ* condp,
                 منطقی consumes_چکر)
        : V3_گراف_کنارہ{graphp, fromp, چوٹی, /*weight=*/1}
        , m_condp{condp}
        , m_consumes_چکر{consumes_چکر} {}
    ~Sva_Trans_کنارہ() override {
        اگر (m_condp) VL_DO_DANGLING(m_condp->delete_درخت(), m_condp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string dotLabel() ثابت override { واپس m_consumes_چکر ? "##1" : "link"; }
    string dotStyle() ثابت override { واپس m_consumes_چکر ? "" : "dashed"; }
    // LCOV_EXCL_STOP
    // Typed accessors for NFA vertices
    Sva_State_راس* fromVtxp() ثابت { واپس static_cast<Sva_State_راس*>(fromp()); }
    Sva_State_راس* مقصد_Vtxp() ثابت { واپس static_cast<Sva_State_راس*>(چوٹی()); }
};

// NFA graph container
جماعت Sva_گراف final {
عوامی:
    V3_گراف m_گراف;  // Owns all vertices and edges
    Sva_State_راس* m_آغاز_Vertexp = خالی_اشاریہ;  // Trigger/start vertex
    Sva_State_راس* m_مطابقت_Vertexp = خالی_اشاریہ;  // Sequence-match terminal vertex

    // Create a new state vertex
    Sva_State_راس* create_State_راس() { واپس نیا Sva_State_راس{&m_گراف}; }
    // Create the match terminal vertex
    Sva_State_راس* create_مطابقت_راس() {
        Sva_State_راس* ثابت vtxp = create_State_راس();
        vtxp->m_is_مطابقت = سچ;
        m_مطابقت_Vertexp = vtxp;
        واپس vtxp;
    }
    // Add a clocked transition edge (##1)
    Sva_Trans_کنارہ* جوڑو_Clocked_کنارہ(Sva_State_راس* fromp, Sva_State_راس* چوٹی,
                                 Ast_گرہ_اظہاریہ* condp = خالی_اشاریہ) {
        واپس نیا Sva_Trans_کنارہ{&m_گراف, fromp, چوٹی, condp, /*consumesCycle=*/سچ};
    }
    // Add a combinational link (##0 / boolean condition)
    Sva_Trans_کنارہ* جوڑو_Link(Sva_State_راس* fromp, Sva_State_راس* چوٹی,
                          Ast_گرہ_اظہاریہ* condp = خالی_اشاریہ) {
        واپس نیا Sva_Trans_کنارہ{&m_گراف, fromp, چوٹی, condp, /*consumesCycle=*/جھوٹ};
    }
    // Collect all edges into a flat vector for iteration.
    // Used by the lowering phase which needs global edge scans.
    std::سمتیہ<ثابت Sva_Trans_کنارہ*> all_کنارے() ثابت {
        std::سمتیہ<ثابت Sva_Trans_کنارہ*> نتیجہ;
        برائے (ثابت V3_گراف_راس& vtxr : m_گراف.vertices()) {
            برائے (ثابت V3_گراف_کنارہ& edger : vtxr.برآمد_کنارے()) {
                نتیجہ.ڈالو_عقب(static_cast<ثابت Sva_Trans_کنارہ*>(&edger));
            }
        }
        واپس نتیجہ;
    }
};

//######################################################################
// Builder result: terminal vertex + optional final condition (match Link condition).
ساخت Build_نتیجہ final {
    Sva_State_راس* رکن_Vertexp;  // Primary terminal; contributes to both match and reject
    Ast_گرہ_اظہاریہ* finalCondp;  // nullptr = unconditional
    // Mid-window sources for range delays (pure boolean RHS): match-only (isUnbounded)
    std::سمتیہ<Sva_State_راس*> midSources;
    منطقی errorEmitted = جھوٹ;  // Builder already emitted specific error; skip generic
    // For cover_sequence: when true, midSources already enumerate every
    // end-of-match, so wireMatchAndMidSources must NOT add the main
    // termVtxp -> matchVertex Link (would double-count via the merge vertex).
    منطقی رکن_Is_Mid_ملاؤ = جھوٹ;
    منطقی valid() ثابت { واپس رکن_Vertexp != خالی_اشاریہ; }
    ساکن Build_نتیجہ fail(منطقی errored = جھوٹ) { واپس {خالی_اشاریہ, خالی_اشاریہ, {}, errored}; }
    ساکن Build_نتیجہ failWithError() { واپس {خالی_اشاریہ, خالی_اشاریہ, {}, سچ}; }
};

// Parser-marked SAnd of overlapped implications: a property if/else/case.
ساکن منطقی hasPropertyControlConjunction(ثابت Ast_گرہ_اظہاریہ* nodep) {
    واپس nodep->exists([](ثابت AstSAnd* andp) { واپس andp->propertyControl(); });
}

ساکن AstConst* newTypedConstp(File_سطر* ثابت flp, ثابت Ast_گرہ_DType* ثابت dtypep,
                                ثابت uint32_t قدر) {
    AstConst* ثابت constp = نیا AstConst{flp, AstConst::DTyped{}, dtypep};
    constp->عدد().سیٹ_Long(قدر);
    واپس constp;
}

ساکن Ast_گرہ_اظہاریہ* sampled(Ast_گرہ_اظہاریہ* exprp) {
    واپس نیا AstSampled{exprp->fileline(), exprp, exprp->dtypep(), سچ};
}

ساکن ثابت حرف* assertPassOnQuery(منطقی vacuous) {
    ساکن constexpr ثابت حرف* queries[2]
        = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
    واپس queries[vacuous];
}

ساکن Ast_گرہ_اظہاریہ* assertOnCond(File_سطر* flp, VAssertType type,
                                 VAssertDirectiveType directiveType) {
    اگر (!v3Global.opt.assertOn()) { واپس نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}; }
    واپس نیا AstCExpr{flp, AstCExpr::خالص{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
}

ساکن Ast_گرہ_اظہاریہ* assertKillGet(File_سطر* flp, VAssertType type,
                                  VAssertDirectiveType directiveType) {
    واپس نیا AstCExpr{flp, AstCExpr::خالص{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_KILL", type, directiveType),
                        32};
}

ساکن string assert_Action_Control_سابقہ(VAssertDirectiveType directiveType) {
    ثابت صحیح controlled = !!(static_cast<صحیح>(directiveType)
                              & (static_cast<صحیح>(VAssertDirectiveType::ASSERT)
                                 | static_cast<صحیح>(VAssertDirectiveType::COVER)
                                 | static_cast<صحیح>(VAssertDirectiveType::ASSUME)));
    ثابت صحیح checkRuntime = controlled & static_cast<صحیح>(v3Global.opt.assertOn());
    واپس "("s + std::مقصد_string(controlled ^ 1) + " || ("s + std::مقصد_string(checkRuntime)
           + " && "s;
}

ساکن Ast_گرہ_اظہاریہ* assertPassOnCond(File_سطر* flp, VAssertType type,
                                     VAssertDirectiveType directiveType, منطقی vacuous) {
    واپس نیا AstCExpr{
        flp, AstCExpr::خالص{},
        assert_Action_Control_سابقہ(directiveType)
            + V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType)
            + "))"s,
        1};
}

ساکن Ast_گرہ_اظہاریہ* assertFailOnCond(File_سطر* flp, VAssertType type,
                                     VAssertDirectiveType directiveType) {
    واپس نیا AstCExpr{
        flp, AstCExpr::خالص{},
        assert_Action_Control_سابقہ(directiveType)
            + V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType) + "))"s,
        1};
}

ساکن AstIf* newPassOnIf(File_سطر* flp, Ast_گرہ_اظہاریہ* firep, Ast_گرہ* bodyp, VAssertType type,
                          VAssertDirectiveType directiveType, منطقی vacuous) {
    Ast_گرہ_اظہاریہ* ثابت condp
        = نیا AstLogAnd{flp, firep, assertPassOnCond(flp, type, directiveType, vacuous)};
    AstIf* ثابت ifp = نیا AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(سچ);
    ifp->user1(سچ);
    واپس ifp;
}

ساکن Ast_گرہ_Stmt* newIfAssertFailOn(Ast_گرہ* bodyp, VAssertDirectiveType directiveType,
                                      VAssertType type) {
    File_سطر* ثابت flp = bodyp->fileline();
    Ast_گرہ_اظہاریہ* ثابت condp = assertFailOnCond(flp, type, directiveType);
    AstIf* ثابت ifp = نیا AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(سچ);
    ifp->user1(سچ);
    واپس ifp;
}

//######################################################################
// NFA Builder

جماعت SvaNfaBuilder final {
    Sva_گراف& m_گراف;  // NFA graph being built
    Ast_گرہ_Module* ثابت m_modp;  // Module to receive hoisted sampled-prop temps
    V3_Unique_نام& m_prop_عارضی_نام;  // Module-shared temp-var name source
    std::سمتیہ<Ast_گرہ_اظہاریہ*> m_temporal_Guard_ڈھیر;  // Guards active across nested temporal states
    // Outer abort conditions, AND-ed as !cond into inner abort edges
    // (IEEE 1800-2023 16.12.14 outer-wraps-inner).
    std::سمتیہ<Ast_گرہ_اظہاریہ*> m_outer_Abort_ڈھیر;
    منطقی m_اندر_Unbounded_Scope = جھوٹ;  // Sticky: nodes created after inherit liveness
    منطقی m_markStrongPending = جھوٹ;  // Mark new vertices as strong s_always in-window
    منطقی m_isCover = جھوٹ;  // Cover directives do not fail at end-of-simulation
    // IEEE 1800-2023 16.14.3 cover sequence: each end-of-match fires the action,
    // not just the first. Builder builds parallel-branch (no first-match-wins)
    // topology when true. Default false preserves cover_property semantics.
    منطقی m_isCoverSeq = جھوٹ;
    // Unsupported endpoint topology must reject, not ignore, or the wait hangs
    منطقی m_isSeqEvent = جھوٹ;
    منطقی m_اندر_Sequence_سابقہ = جھوٹ;  // Prefix composition does not preserve every endpoint
    منطقی m_want_Bool_برگ = جھوٹ;  // Building a |-> / |=> or top-level body (see buildOrMerge)

    ساخت RangeDelayRejectInfo final {
        Sva_State_راس* startp = خالی_اشاریہ;
        غیرعلامتی range = 0;
        صحیح rhsLen = 0;
    };

    خالی warnEndpointUnsupported(File_سطر* flp, ثابت string& what) ثابت {
        اگر (m_isSeqEvent) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: sequence used as an event control with " << what);
        } ورنہ {
            flp->v3warn(COVERIGN, "Ignoring unsupported: cover sequence with " << what);
        }
    }

    Ast_گرہ_اظہاریہ* throughoutCond(Ast_گرہ_اظہاریہ* بنیاد_Condp, File_سطر* flp) {
        اگر (m_temporal_Guard_ڈھیر.خالی_خانے()) واپس بنیاد_Condp;
        // AND all active temporal guards (supports nesting)
        // Each must use $sampled values.
        Ast_گرہ_اظہاریہ* guardp = خالی_اشاریہ;
        برائے (Ast_گرہ_اظہاریہ* ثابت condp : m_temporal_Guard_ڈھیر) {
            Ast_گرہ_اظہاریہ* ثابت clonep = sampled(condp->clone_درخت_خالص(جھوٹ));
            اگر (!guardp) {
                guardp = clonep;
            } ورنہ {
                guardp = نیا AstLogAnd{flp, guardp, clonep};
            }
        }
        اگر (بنیاد_Condp) { guardp = نیا AstLogAnd{flp, بنیاد_Condp, guardp}; }
        واپس guardp;
    }

    ساکن غیرعلامتی getConstUInt(Ast_گرہ_اظہاریہ* exprp) {
        Ast_گرہ_اظہاریہ* ثابت constp = V3Const::constifyEdit(exprp->clone_درخت_خالص(جھوٹ));
        ثابت AstConst* ثابت cp = VN_CAST(constp, Const);
        ثابت غیرعلامتی val = cp ? cp->مقصد_UInt() : 0;
        VL_DO_DANGLING(constp->delete_درخت(), constp);
        واپس val;
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
    ساکن صحیح fixed_لمبائی(Ast_گرہ_اظہاریہ* nodep) {
        اگر (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
            اگر (!delayp || !delayp->is_چکر_Delay()) واپس -1;
            غیرعلامتی delay_ادوار;
            اگر (delayp->isRangeDelay()) {
                اگر (delayp->isUnbounded()) واپس -1;  // LCOV_EXCL_LINE
                ثابت غیرعلامتی minD = getConstUInt(delayp->lhsp());
                ثابت غیرعلامتی maxD = getConstUInt(delayp->rhsp());
                اگر (minD != maxD) واپس -1;
                delay_ادوار = minD;
            } ورنہ {
                delay_ادوار = getConstUInt(delayp->lhsp());
            }
            صحیح preLen = 0;
            اگر (Ast_گرہ_اظہاریہ* ثابت prep = sexprp->preExprp()) {
                preLen = fixed_لمبائی(prep);
                اگر (preLen < 0) واپس -1;  // LCOV_EXCL_LINE
            }
            ثابت صحیح bodyLen = fixed_لمبائی(sexprp->exprp());
            اگر (bodyLen < 0) واپس -1;  // LCOV_EXCL_LINE
            واپس preLen + delay_ادوار + bodyLen;
        }
        اگر (AstSThroughout* ثابت throughp = VN_CAST(nodep, SThroughout)) {
            واپس fixed_لمبائی(throughp->rhsp());
        }
        اگر (AstSOr* ثابت orp = VN_CAST(nodep, SOr)) {
            // Alternatives must share one end cycle; buildSWithin relies on
            // this to pair the OR with an SIntersect.
            ثابت صحیح lhsLen = fixed_لمبائی(orp->lhsp());
            ثابت صحیح rhsLen = fixed_لمبائی(orp->rhsp());
            اگر (lhsLen < 0 || rhsLen < 0 || lhsLen != rhsLen) واپس -1;
            واپس lhsLen;
        }
        اگر (AstSWithin* ثابت withinp = VN_CAST(nodep, SWithin)) {
            // `seq1 within seq2` ends at seq2's end cycle (IEEE 16.9.10).
            ثابت صحیح lhsLen = fixed_لمبائی(withinp->lhsp());
            ثابت صحیح rhsLen = fixed_لمبائی(withinp->rhsp());
            اگر (lhsLen < 0 || rhsLen < 0 || lhsLen > rhsLen) واپس -1;
            واپس rhsLen;
        }
        // LCOV_EXCL_START -- defensive: V3AssertPre rejects composite SVA ops
        // nested in an intersect arm before fixedLength runs (clock-context
        // resolution fails). Kept as a guard in case future parser relaxations
        // permit it.
        اگر (nodep->is_Multi_چکر_Sva()) واپس -1;
        // LCOV_EXCL_STOP
        // Plain boolean expression (no SVA constructs) -- 0 cycles.
        واپس 0;
    }

    // Contiguous match-length range [lo,hi] for an operand whose length varies
    // from AT MOST ONE ranged cycle delay; {-1,-1} otherwise. Drives the
    // variable-length `intersect` lowering (IEEE 1800-2023 16.9.6): more than
    // one ranged delay would make the per-length realization ambiguous, so it
    // is reported unsupported rather than mis-paired.
    ساکن std::pair<صحیح, صحیح> لمبائی_Range(Ast_گرہ_اظہاریہ* nodep) {
        اگر (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
            اگر (!delayp || !delayp->is_چکر_Delay()) واپس {-1, -1};
            std::pair<صحیح, صحیح> delayRange;
            اگر (delayp->isRangeDelay()) {
                اگر (delayp->isUnbounded()) واپس {-1, -1};
                ثابت غیرعلامتی minD = getConstUInt(delayp->lhsp());
                ثابت غیرعلامتی maxD = getConstUInt(delayp->rhsp());
                delayRange = {minD, maxD};
            } ورنہ {
                ثابت غیرعلامتی d = getConstUInt(delayp->lhsp());
                delayRange = {d, d};
            }
            std::pair<صحیح, صحیح> preRange{0, 0};
            اگر (Ast_گرہ_اظہاریہ* ثابت prep = sexprp->preExprp()) {
                preRange = لمبائی_Range(prep);
                اگر (preRange.پہلا < 0) واپس {-1, -1};
            }
            ثابت std::pair<صحیح, صحیح> bodyRange = لمبائی_Range(sexprp->exprp());
            اگر (bodyRange.پہلا < 0) واپس {-1, -1};
            ثابت صحیح variableParts = (preRange.پہلا != preRange.دوسرا)
                                      + (delayRange.پہلا != delayRange.دوسرا)
                                      + (bodyRange.پہلا != bodyRange.دوسرا);
            اگر (variableParts > 1) واپس {-1, -1};
            واپس {preRange.پہلا + delayRange.پہلا + bodyRange.پہلا,
                    preRange.دوسرا + delayRange.دوسرا + bodyRange.دوسرا};
        }
        اگر (AstSThroughout* ثابت throughp = VN_CAST(nodep, SThroughout)) {
            واپس لمبائی_Range(throughp->rhsp());
        }
        اگر (nodep->is_Multi_چکر_Sva()) واپس {-1, -1};
        واپس {0, 0};  // plain boolean -- 0 cycles
    }

    // Clone `operand` with its sole variable ranged cycle delay pinned so the
    // total match length is exactly `len`. `lo` is the operand's minimum length
    // (lengthRange().first). A fixed operand has no such delay and is returned
    // as a plain clone (callers only request its single achievable length).
    ساکن Ast_گرہ_اظہاریہ* realize_At_لمبائی(Ast_گرہ_اظہاریہ* عملیہ, صحیح len, صحیح lo) {
        Ast_گرہ_اظہاریہ* ثابت clonep = عملیہ->clone_درخت_خالص(جھوٹ);
        AstDelay* rangeDelayp = خالی_اشاریہ;
        clonep->foreach([&](AstDelay* dp) {
            اگر (!rangeDelayp && dp->isRangeDelay() && !dp->isUnbounded()
                && getConstUInt(dp->lhsp()) != getConstUInt(dp->rhsp())) {
                rangeDelayp = dp;
            }
        });
        اگر (rangeDelayp) {
            File_سطر* ثابت flp = rangeDelayp->fileline();
            ثابت غیرعلامتی pinned = getConstUInt(rangeDelayp->lhsp()) + (len - lo);
            Ast_گرہ_اظہاریہ* ثابت oldMinp = rangeDelayp->lhsp();
            oldMinp->replaceWith(نیا AstConst{flp, pinned});
            VL_DO_DANGLING(oldMinp->delete_درخت(), oldMinp);
            // Drop the max bound so it lowers as a fixed `##d`, not `##[d:d]`.
            Ast_گرہ* ثابت oldMaxp = rangeDelayp->rhsp()->unlink_Fr_عقب();
            VL_DO_DANGLING(oldMaxp->delete_درخت(), oldMaxp);
        }
        واپس clonep;
    }

    // Cuts AST size from O(N * sizeof(exprp)) to O(N) + O(sizeof(exprp)) by
    // sharing a single `VarRef` across N check edges. Hoist also matches the
    // IEEE 1800-2023 16.9.9 "single preponed-region snapshot" semantic for
    // any exprp -- even an impure one would now evaluate exactly once per
    // clock instead of N times. Orphan temps from failed builds are unused
    // MODULETEMPs and are removed by V3Dead.
    AstVar* tryHoistSampled(Ast_گرہ_اظہاریہ* exprp, File_سطر* flp, غیرعلامتی clone_گنتی) {
        constexpr غیرعلامتی k_Hoist_حد = 2;
        اگر (clone_گنتی < k_Hoist_حد) واپس خالی_اشاریہ;
        AstVar* ثابت عارضی_Varp
            = نیا AstVar{flp, VVarType::MODULETEMP, m_prop_عارضی_نام.get(exprp), exprp->dtypep()};
        m_modp->جوڑو_Stmtsp(عارضی_Varp);
        Ast_تفویض* ثابت assignp = نیا Ast_تفویض{flp, نیا AstVarRef{flp, عارضی_Varp, VAccess::WRITE},
                                                 sampled(exprp->clone_درخت_خالص(جھوٹ))};
        m_modp->جوڑو_Stmtsp(نیا AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, خالی_اشاریہ, assignp});
        واپس عارضی_Varp;
    }

    ساکن Ast_گرہ_اظہاریہ* sampledRefOrClone(AstVar* hoistVarp, Ast_گرہ_اظہاریہ* exprp, File_سطر* flp) {
        اگر (hoistVarp) واپس نیا AstVarRef{flp, hoistVarp, VAccess::READ};
        واپس sampled(exprp->clone_درخت_خالص(جھوٹ));
    }

    // Create vertex and inherit temporal guards from the current scope.
    Sva_State_راس* scoped_Create_راس() {
        Sva_State_راس* ثابت vtxp = m_گراف.create_State_راس();
        برائے (Ast_گرہ_اظہاریہ* ثابت cp : m_temporal_Guard_ڈھیر) {
            vtxp->m_throughoutConds.ڈالو_عقب(cp->clone_درخت_خالص(جھوٹ));
        }
        اگر (m_اندر_Unbounded_Scope) vtxp->m_isUnbounded = سچ;
        اگر (m_markStrongPending) vtxp->m_strongPending = سچ;
        واپس vtxp;
    }

    // AND current temporal guards into every edge/link.
    Sva_Trans_کنارہ* guardedLink(Sva_State_راس* fromp, Sva_State_راس* چوٹی, Ast_گرہ_اظہاریہ* condp,
                              File_سطر* flp) {
        واپس m_گراف.جوڑو_Link(fromp, چوٹی, throughoutCond(condp, flp));
    }
    Sva_Trans_کنارہ* guardedLink(Sva_State_راس* fromp, Sva_State_راس* چوٹی, File_سطر* flp) {
        واپس m_گراف.جوڑو_Link(fromp, چوٹی, throughoutCond(خالی_اشاریہ, flp));
    }
    Sva_Trans_کنارہ* guarded_کنارہ(Sva_State_راس* fromp, Sva_State_راس* چوٹی, Ast_گرہ_اظہاریہ* condp,
                              File_سطر* flp) {
        واپس m_گراف.جوڑو_Clocked_کنارہ(fromp, چوٹی, throughoutCond(condp, flp));
    }
    Sva_Trans_کنارہ* guarded_کنارہ(Sva_State_راس* fromp, Sva_State_راس* چوٹی, File_سطر* flp) {
        واپس m_گراف.جوڑو_Clocked_کنارہ(fromp, چوٹی, throughoutCond(خالی_اشاریہ, flp));
    }

    Sva_State_راس* جوڑو_Delay_Chain(Sva_State_راس* startp, غیرعلامتی سائز, File_سطر* flp,
                                  منطقی isFixed = سچ, Ast_گرہ_اظہاریہ* clearCondp = خالی_اشاریہ,
                                  Ast_گرہ_اظہاریہ* advanceCondp = خالی_اشاریہ) {
        اگر (isFixed && سائز == 0) واپس startp;
        UASSERT_OBJ(سائز > 0, startp, "Delay chain needs at least one slot");
        اگر (isFixed && سائز == 1 && !advanceCondp) {
            Sva_State_راس* ثابت nextp = scoped_Create_راس();
            guarded_کنارہ(startp, nextp, flp);
            واپس nextp;
        }
        Sva_State_راس* ثابت ringVtxp = scoped_Create_راس();
        ringVtxp->m_isFixedDelayRing = isFixed;
        ringVtxp->m_delay_Ring_سائز = سائز;
        اگر (clearCondp) {
            UASSERT_OBJ(!isFixed, startp, "Fixed delay cannot have a clear condition");
            ringVtxp->m_delayRingClearCondp = clearCondp->clone_درخت_خالص(جھوٹ);
        }
        ringVtxp->m_delayRingAdvanceCondp = advanceCondp;
        اگر (isFixed) {
            guarded_کنارہ(startp, ringVtxp, flp);
        } ورنہ {
            guardedLink(startp, ringVtxp, flp);
        }
        واپس ringVtxp;
    }

    // Build NFA for an SExpr. finalCond = RHS (not yet added as a vertex).
    // isTopLevelStep: marks outermost required boolean check as rejectOnFail.
    // Apply a range delay `##[M:N]` to currentp. Returns true on success. On
    // failure, emits a diagnostic and returns false.
    منطقی applyRangeDelay(AstSExpr* sexprp, Sva_State_راس*& currentp,
                         std::سمتیہ<Sva_State_راس*>& midSources,
                         RangeDelayRejectInfo* rangeRejectInfop = خالی_اشاریہ) {
        File_سطر* ثابت flp = sexprp->fileline();
        AstDelay* ثابت delayp = VN_AS(sexprp->delayp(), Delay);
        Ast_گرہ_اظہاریہ* ثابت rhsExprp = sexprp->exprp();
        ثابت غیرعلامتی minDelay = getConstUInt(delayp->lhsp());
        اگر (delayp->isUnbounded()) {
            // `##[M:$]`: wait M cycles, then retain every matured attempt in a
            // one-slot ring while also exposing the first eligible tick.
            currentp = جوڑو_Delay_Chain(currentp, minDelay, flp);
            Sva_State_راس* ثابت waitRingp = جوڑو_Delay_Chain(currentp, 1, flp, جھوٹ);
            // The ring's live count is updated in NBA, so this link retains the previous
            // tick's live attempts alongside new arrivals for the next tick.
            guardedLink(waitRingp, waitRingp, flp);
            Sva_State_راس* ثابت ملاؤ_Vtxp = scoped_Create_راس();
            guardedLink(currentp, ملاؤ_Vtxp, flp);
            guardedLink(waitRingp, ملاؤ_Vtxp, flp);
            currentp = ملاؤ_Vtxp;
            currentp->m_isUnbounded = سچ;  // Liveness, so no reject.
            m_اندر_Unbounded_Scope = سچ;
            واپس سچ;
        }
        ثابت غیرعلامتی maxDelay = getConstUInt(delayp->rhsp());
        اگر (minDelay == maxDelay) {
            currentp = جوڑو_Delay_Chain(currentp, minDelay, flp);
            واپس سچ;
        }
        ثابت غیرعلامتی range = maxDelay - minDelay;
        currentp = جوڑو_Delay_Chain(currentp, minDelay, flp);
        ثابت منطقی multi_چکر_Rhs = rhsExprp->is_Multi_چکر_Sva();
        ثابت Ast_گرہ_اظہاریہ* ثابت preExprp = sexprp->preExprp();
        // Prefix and nested-sequence composition can lose endpoints or their multiplicity
        // required by cover sequence (IEEE 1800-2023 16.14.3), regardless of the range width.
        اگر (m_isCoverSeq
            && (multi_چکر_Rhs || m_اندر_Sequence_سابقہ
                || (preExprp && preExprp->is_Multi_چکر_Sva()))) {
            warnEndpointUnsupported(flp, "this ranged cycle delay");
            واپس جھوٹ;
        }
        اگر (m_اندر_Sequence_سابقہ) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: Bounded ranged cycle delay in a sequence prefix");
            واپس جھوٹ;
        }
        اگر (multi_چکر_Rhs) {
            // Launch a candidate at every eligible tick. Candidate-local misses are not
            // assertion rejects while a later position can still match.
            اگر (rangeRejectInfop) {
                ثابت صحیح rhsLen = fixed_لمبائی(rhsExprp);
                اگر (rhsLen >= 0) *rangeRejectInfop = {currentp, range, rhsLen};
            }
            Sva_State_راس* ثابت tailp = جوڑو_Delay_Chain(currentp, range + 1U, flp, جھوٹ);
            Sva_State_راس* ثابت ملاؤ_Vtxp = scoped_Create_راس();
            ملاؤ_Vtxp->m_isUnbounded = سچ;
            guardedLink(currentp, ملاؤ_Vtxp, flp);
            guardedLink(tailp, ملاؤ_Vtxp, flp);
            currentp = ملاؤ_Vtxp;
            m_اندر_Unbounded_Scope = سچ;
        } ورنہ {
            // The first eligible tick is explicit. The ring retains attempts for the
            // remaining ticks and exposes the final tick for rejection. Only properties
            // clear on success, since cover sequence counts every endpoint.
            midSources.ڈالو_عقب(currentp);
            currentp = جوڑو_Delay_Chain(currentp, range + 1U, flp, جھوٹ,
                                     m_isCoverSeq ? خالی_اشاریہ : rhsExprp);
        }
        واپس سچ;
    }

    خالی جوڑو_Finite_Range_Reject(ثابت RangeDelayRejectInfo& info, ثابت Build_نتیجہ& نتیجہ,
                              File_سطر* flp) {
        اگر (!info.startp) واپس;

        Sva_State_راس* ثابت expiryVtxp
            = جوڑو_Delay_Chain(info.startp, info.range + info.rhsLen, flp);
        Sva_State_راس* ثابت expiryMatchp = scoped_Create_راس();
        std::سمتیہ<Sva_State_راس*> sources = نتیجہ.midSources;
        sources.ڈالو_عقب(نتیجہ.رکن_Vertexp);
        برائے (Sva_State_راس* ثابت srcp : sources) {
            Ast_گرہ_اظہاریہ* ثابت condp
                = نتیجہ.finalCondp ? sampled(نتیجہ.finalCondp->clone_درخت_خالص(جھوٹ)) : خالی_اشاریہ;
            Sva_State_راس* ثابت successNowp = scoped_Create_راس();
            guardedLink(srcp, successNowp, condp, flp);
            // Retain a success through the latest deadline it can satisfy.
            Sva_State_راس* ثابت historyp
                = جوڑو_Delay_Chain(successNowp, info.range + 1U, flp, جھوٹ);
            guardedLink(successNowp, expiryMatchp, flp);
            guardedLink(historyp, expiryMatchp, flp);
        }

        Sva_State_راس* ثابت sinkVtxp = m_گراف.create_State_راس();
        sinkVtxp->m_isRejectSink = سچ;
        Sva_Trans_کنارہ* ثابت rejectp = m_گراف.جوڑو_Link(expiryVtxp, sinkVtxp);
        rejectp->m_rejectOnFail = سچ;
        rejectp->m_condVtxp = expiryMatchp;
    }

    Build_نتیجہ buildSExpr(AstSExpr* sexprp, Sva_State_راس* اندراج_Vtxp,
                           منطقی is_چوٹی_Level_قدم = جھوٹ) {
        AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
        اگر (!delayp || !delayp->is_چکر_Delay()) واپس Build_نتیجہ::fail();

        File_سطر* ثابت flp = sexprp->fileline();
        Ast_گرہ_اظہاریہ* ثابت exprp = sexprp->exprp();

        // Handle LHS (preExpr)
        Sva_State_راس* currentp = اندراج_Vtxp;
        اگر (Ast_گرہ_اظہاریہ* ثابت preExprp = sexprp->preExprp()) {
            VL_RESTORER(m_اندر_Sequence_سابقہ);
            m_اندر_Sequence_سابقہ = سچ;
            ثابت Build_نتیجہ pre = build_اظہاریہ(preExprp, currentp, is_چوٹی_Level_قدم);
            اگر (!pre.valid()) واپس Build_نتیجہ::fail(pre.errorEmitted);  // LCOV_EXCL_LINE
            اگر (pre.finalCondp) {
                Sva_State_راس* ثابت condVtxp = scoped_Create_راس();
                Sva_Trans_کنارہ* ثابت edgep = guardedLink(
                    pre.رکن_Vertexp, condVtxp, sampled(pre.finalCondp->clone_درخت_خالص(جھوٹ)), flp);
                اگر (is_چوٹی_Level_قدم && !pre.رکن_Vertexp->m_isUnbounded && !m_اندر_Unbounded_Scope) {
                    // Do not mark liveness sources: first boolean check is deferred.
                    edgep->m_rejectOnFail = سچ;
                }
                freeUnlinkedCondp(pre.finalCondp);
                currentp = condVtxp;
            } ورنہ {
                currentp = pre.رکن_Vertexp;
            }
        }

        // Handle delay
        std::سمتیہ<Sva_State_راس*> rangeMidSources;
        RangeDelayRejectInfo rangeRejectInfo;
        ثابت منطقی جوڑو_Range_Reject = is_چوٹی_Level_قدم && !m_اندر_Unbounded_Scope;
        اگر (delayp->isRangeDelay()) {
            اگر (!applyRangeDelay(sexprp, currentp, rangeMidSources,
                                 جوڑو_Range_Reject ? &rangeRejectInfo : خالی_اشاریہ)) {
                واپس Build_نتیجہ::failWithError();
            }
        } ورنہ {
            ثابت غیرعلامتی delay_ادوار = getConstUInt(delayp->lhsp());
            currentp = جوڑو_Delay_Chain(currentp, delay_ادوار, flp);
        }

        // Multi-cycle RHS: recurse (only plain boolean is returned as finalCondp).
        اگر (exprp->is_Multi_چکر_Sva()) {
            ثابت Build_نتیجہ نتیجہ = build_اظہاریہ(exprp, currentp, is_چوٹی_Level_قدم);
            اگر (نتیجہ.valid()) جوڑو_Finite_Range_Reject(rangeRejectInfo, نتیجہ, flp);
            واپس نتیجہ;
        }
        واپس {currentp, exprp, std::move(rangeMidSources)};
    }

    Build_نتیجہ buildConsRep(AstSConsRep* repp, Sva_State_راس* اندراج_Vtxp,
                             منطقی is_چوٹی_Level_قدم = جھوٹ) {
        File_سطر* ثابت flp = repp->fileline();
        Ast_گرہ_اظہاریہ* ثابت exprp = repp->exprp();
        // Multi-cycle expr in ConsRep not yet supported; bail to avoid invalid AST.
        اگر (exprp->is_Multi_چکر_Sva()) {
            repp->v3warn(E_UNSUPPORTED, "Unsupported: multi-cycle sequence expression inside"
                                        " consecutive repetition (IEEE 1800-2023 16.9.2)");
            واپس Build_نتیجہ::failWithError();
        }
        ثابت غیرعلامتی minN = getConstUInt(repp->countp());

        // Sum sites across prefix + unbounded/range tail so one hoist covers
        // every check edge of this repetition.
        غیرعلامتی کل_Sites = minN;
        اگر (repp->unbounded()) {
            کل_Sites += 1;
        } ورنہ اگر (repp->maxCountp()) {
            کل_Sites += getConstUInt(repp->maxCountp()) - minN;
        }
        AstVar* ثابت hoistVarp = tryHoistSampled(exprp, flp, کل_Sites);

        // Cover-sequence (IEEE 1800-2023 16.14.3): collect each end-of-match
        // position so they all fire the action, not just the merged terminal.
        std::سمتیہ<Sva_State_راس*> consMidSources;

        Sva_State_راس* currentp = اندراج_Vtxp;
        برائے (غیرعلامتی i = 0; i < minN; ++i) {
            // Keep the first repetition explicit, collapse all remaining checks into the ring.
            اگر (i == 1) {
                currentp = جوڑو_Delay_Chain(currentp, minN - 1, flp);
                currentp->m_delayRingClearCondp
                    = نیا AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                currentp->m_replayAbortReject = سچ;
                اگر (is_چوٹی_Level_قدم) {
                    currentp->m_throughoutConds.ڈالو_عقب(
                        sampledRefOrClone(hoistVarp, exprp, flp));
                }
                i = minN - 1;
            }
            // Every repetition in the minimum prefix is required.
            Sva_State_راس* ثابت condVtxp = scoped_Create_راس();
            Sva_Trans_کنارہ* ثابت linkp
                = guardedLink(currentp, condVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            // Only an outermost required step rejects the property. Its first check is explicit,
            // later required checks reject through the ring's throughout guard.
            اگر (is_چوٹی_Level_قدم && i == 0) linkp->m_rejectOnFail = سچ;
            currentp = condVtxp;
        }
        // After minN: currentp is the first valid end-of-match position for [*m:n].
        اگر (m_isCoverSeq && (repp->unbounded() || repp->maxCountp())) {
            consMidSources.ڈالو_عقب(currentp);
        }

        اگر (repp->unbounded()) {
            اگر (minN == 0) {
                Sva_State_راس* ثابت waitVtxp = scoped_Create_راس();
                guarded_کنارہ(currentp, waitVtxp, flp);
                Sva_State_راس* ثابت checkVtxp = scoped_Create_راس();
                guardedLink(waitVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guarded_کنارہ(checkVtxp, waitVtxp, flp);
                guardedLink(currentp, checkVtxp, flp);
                currentp = checkVtxp;
            } ورنہ {
                Sva_State_راس* ثابت loop_عقب_Vtxp = scoped_Create_راس();
                guarded_کنارہ(currentp, loop_عقب_Vtxp, flp);
                Sva_State_راس* ثابت reCheckVtxp = scoped_Create_راس();
                guardedLink(loop_عقب_Vtxp, reCheckVtxp, sampledRefOrClone(hoistVarp, exprp, flp),
                            flp);
                guarded_کنارہ(reCheckVtxp, loop_عقب_Vtxp, flp);
                guardedLink(reCheckVtxp, currentp, flp);
            }
            currentp->m_isUnbounded = سچ;
            m_اندر_Unbounded_Scope = سچ;
        } ورنہ اگر (repp->maxCountp()) {
            ثابت غیرعلامتی maxN = getConstUInt(repp->maxCountp());
            Sva_State_راس* ثابت ملاؤ_Vtxp = scoped_Create_راس();
            guardedLink(currentp, ملاؤ_Vtxp, flp);
            غیرعلامتی دم_Min_N = minN;
            Sva_State_راس* دم_Startp = currentp;
            اگر (minN == 0) {
                // Build the first optional iteration explicitly. Feeding the empty endpoint
                // directly into a range ring would incorrectly keep that match alive.
                Sva_State_راس* ثابت اگلا_Vtxp = scoped_Create_راس();
                guarded_کنارہ(currentp, اگلا_Vtxp, flp);
                Sva_State_راس* ثابت checkVtxp = scoped_Create_راس();
                guardedLink(اگلا_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, ملاؤ_Vtxp, flp);
                اگر (m_isCoverSeq) consMidSources.ڈالو_عقب(checkVtxp);
                دم_Startp = checkVtxp;
                دم_Min_N = 1;
            }
            اگر (maxN > دم_Min_N) {
                // Add tail-ring only if the tail is non-empty.
                Sva_State_راس* ثابت اگلا_Vtxp
                    = جوڑو_Delay_Chain(دم_Startp, maxN - دم_Min_N + 1, flp, جھوٹ);
                اگلا_Vtxp->m_delayRingClearCondp
                    = نیا AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                اگلا_Vtxp->m_replayAbortReject = سچ;
                Sva_State_راس* ثابت checkVtxp = scoped_Create_راس();
                guardedLink(اگلا_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, ملاؤ_Vtxp, flp);
                اگر (m_isCoverSeq) consMidSources.ڈالو_عقب(checkVtxp);
            }
            currentp = ملاؤ_Vtxp;
        }
        // finalCond = nullptr (already checked via Links)
        Build_نتیجہ res;
        res.رکن_Vertexp = currentp;
        res.finalCondp = خالی_اشاریہ;
        res.midSources = std::move(consMidSources);
        // mergeVtxp is the OR of all the end-positions we already pushed to
        // midSources, so the main termVtxp -> matchVertex Link would duplicate.
        res.رکن_Is_Mid_ملاؤ = m_isCoverSeq && !res.midSources.خالی_خانے();
        واپس res;
    }

    // always[lo:hi] / s_always[lo:hi] (IEEE 1800-2023 16.12.11).
    Build_نتیجہ buildPropAlways(AstPropAlways* nodep, Sva_State_راس* اندراج_Vtxp,
                                منطقی is_چوٹی_Level_قدم = جھوٹ) {
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت propp = nodep->propp();
        ثابت غیرعلامتی lo = getConstUInt(nodep->loBoundp());
        اگر (VN_IS(nodep->hiBoundp(), Unbounded)) {
            // Weak always [lo:$]: unbounded upper bound (IEEE 1800-2023 16.12.11).
            // p must hold at every clock tick at least lo cycles after the attempt
            // start; those ticks are not required to exist, so there is no
            // end-of-trace obligation (weak). The self-loop keeps the attempt live
            // every cycle; each observed cycle is a safety obligation, so a false p
            // rejects immediately.
            UASSERT_OBJ(!nodep->isStrong(), nodep, "Unbounded always must be weak (V3Width)");
            Sva_State_راس* ثابت livep = جوڑو_Delay_Chain(اندراج_Vtxp, lo, flp);
            livep->m_isUnbounded = سچ;
            guarded_کنارہ(livep, livep, flp);  // stay active every subsequent cycle
            Sva_State_راس* ثابت sinkp = m_گراف.create_State_راس();
            sinkp->m_isRejectSink = سچ;
            Sva_Trans_کنارہ* ثابت rejEdgep
                = guardedLink(livep, sinkp, sampled(propp->clone_درخت_خالص(جھوٹ)), flp);
            اگر (is_چوٹی_Level_قدم) rejEdgep->m_rejectOnFail = سچ;
            واپس {livep, خالی_اشاریہ, {}};
        }
        ثابت غیرعلامتی hi = getConstUInt(nodep->hiBoundp());
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
        Sva_Trans_کنارہ* ثابت linkp
            = guardedLink(currentp, checkp, sampled(propp->clone_درخت_خالص(جھوٹ)), flp);
        اگر (is_چوٹی_Level_قدم) linkp->m_rejectOnFail = سچ;
        currentp = checkp;
        m_temporal_Guard_ڈھیر.ڈالو_عقب(propp);
        currentp = جوڑو_Delay_Chain(currentp, hi - lo, flp);
        اگر (nodep->isStrong() && currentp->m_delay_Ring_سائز) currentp->m_strongAlwaysRing = سچ;
        m_temporal_Guard_ڈھیر.نکالو_عقب();
        واپس {currentp, propp, {}};
    }

    Build_نتیجہ buildGotoRep(AstSGotoRep* repp, Sva_State_راس* اندراج_Vtxp) {
        File_سطر* ثابت flp = repp->fileline();
        Ast_گرہ_اظہاریہ* ثابت exprp = repp->exprp();
        ثابت غیرعلامتی minN = getConstUInt(repp->countp());
        اگر (minN == 0) واپس Build_نتیجہ::fail();
        ثابت منطقی hasMax = repp->maxCountp() != خالی_اشاریہ;
        ثابت غیرعلامتی maxN = hasMax ? getConstUInt(repp->maxCountp()) : minN;
        اگر (m_isCoverSeq) {
            // Several attempts may coalesce in the one-bit wait vertex before reaching the ring,
            // so a cover sequence action block could run too few times.
            warnEndpointUnsupported(flp, "a goto repetition");
            واپس Build_نتیجہ::failWithError();
        }

        AstVar* ثابت hoistVarp = tryHoistSampled(exprp, flp, 2);

        // The first guardedEdge is the ##1 before waiting for a match. In the wait state, false
        // takes the clocked self-loop, while true takes the zero-delay guardedLink on that tick.
        Sva_State_راس* ثابت waitVtxp = scoped_Create_راس();
        guarded_کنارہ(اندراج_Vtxp, waitVtxp, flp);
        guarded_کنارہ(waitVtxp, waitVtxp,
                    نیا AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)}, flp);
        Sva_State_راس* currentp = scoped_Create_راس();
        guardedLink(waitVtxp, currentp, sampledRefOrClone(hoistVarp, exprp, flp), flp);

        اگر (minN > 1) {
            Sva_State_راس* ثابت ringVtxp = جوڑو_Delay_Chain(
                currentp, minN - 1, flp, سچ, خالی_اشاریہ, sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = سچ;
            Sva_State_راس* ثابت checkVtxp = scoped_Create_راس();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            currentp = checkVtxp;
        }
        اگر (!hasMax) {
            currentp->m_isUnbounded = سچ;  // [->N] waits unboundedly
            m_اندر_Unbounded_Scope = سچ;
            واپس {currentp, خالی_اشاریہ, {}};
        }

        // [->M:N]: the range ring holds matches from M through N and advances
        // only on expr, preserving arbitrarily long gaps between occurrences.
        Sva_State_راس* ثابت ملاؤ_Vtxp = scoped_Create_راس();
        guardedLink(currentp, ملاؤ_Vtxp, flp);  // accept at match_M
        اگر (maxN > minN) {
            Sva_State_راس* ثابت ringVtxp
                = جوڑو_Delay_Chain(currentp, maxN - minN + 1, flp, جھوٹ, خالی_اشاریہ,
                                sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = سچ;
            Sva_State_راس* ثابت checkVtxp = scoped_Create_راس();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            guardedLink(checkVtxp, ملاؤ_Vtxp, flp);
        }
        ملاؤ_Vtxp->m_isUnbounded = سچ;  // [->M:N] still has unbounded waits between matches
        m_اندر_Unbounded_Scope = سچ;
        واپس {ملاؤ_Vtxp, خالی_اشاریہ, {}};
    }

    // True if the sequence has a zero-minimum consecutive repetition, which may match empty
    ساکن منطقی hasZeroMinRep(ثابت Ast_گرہ_اظہاریہ* seqp) {
        واپس seqp->exists(
            [](ثابت AstSConsRep* repp) { واپس getConstUInt(repp->countp()) == 0; });
    }

    // True if a leaf's condition has no implication or multi-cycle sequence in it
    // (a leaf can be e.g. 'not (x |-> y)')
    ساکن منطقی isPlainBoolean(ثابت Ast_گرہ_اظہاریہ* condp) {
        واپس !condp->exists(
            [](ثابت Ast_گرہ_اظہاریہ* np) { واپس np->is_Multi_چکر_Sva() || VN_IS(np, Implication); });
    }

    // Build merge vertex for SOr / LogOr: both branches feed into one vertex.
    Build_نتیجہ build_Or_ملاؤ(Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp, Sva_State_راس* اندراج_Vtxp,
                             File_سطر* flp, منطقی want_Bool_برگ) {
        ثابت Build_نتیجہ lhs = build_اظہاریہ(lhsp, اندراج_Vtxp);
        ثابت Build_نتیجہ rhs = build_اظہاریہ(rhsp, اندراج_Vtxp);
        اگر (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            واپس Build_نتیجہ::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP
        // IEEE 1800-2023 16.14.3: a cover sequence counts every end-of-match. A
        // sequence operand of 'or' can end more than once, but only its final
        // end reaches the merge vertex below, so reject sequence operands rather
        // than under-count. Plain boolean disjunction has one end per cycle and
        // is handled by the OR-fold.
        ثابت منطقی booleanOnly = lhs.رکن_Vertexp == اندراج_Vtxp && lhs.finalCondp
                                 && rhs.رکن_Vertexp == اندراج_Vtxp && rhs.finalCondp;
        اگر (m_isCoverSeq && !booleanOnly) {
            warnEndpointUnsupported(flp, "a sequence operand of 'or'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            واپس Build_نتیجہ::failWithError();
        }
        اگر (booleanOnly && want_Bool_برگ) {
            اگر (isPlainBoolean(lhs.finalCondp) && isPlainBoolean(rhs.finalCondp)) {
                // As a |-> / |=> or top-level body, a disjunction of two plain booleans must
                // be able to reject: return it as a boolean finalCondp. The merge vertex
                // below drops the attempt silently when both operands are false.
                Ast_گرہ_اظہاریہ* ثابت orCondp
                    = نیا AstLogOr{flp, lhs.finalCondp->clone_درخت_خالص(جھوٹ),
                                   rhs.finalCondp->clone_درخت_خالص(جھوٹ)};
                freeUnlinkedCondp(lhs.finalCondp);
                freeUnlinkedCondp(rhs.finalCondp);
                واپس {اندراج_Vtxp, orCondp, {}};
            }
        }
        Sva_State_راس* ثابت ملاؤ_Vtxp = scoped_Create_راس();
        اگر (booleanOnly) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsp,
                        "Single-cycle SOr operands must have finalCondp");
            guardedLink(اندراج_Vtxp, ملاؤ_Vtxp,
                        نیا AstLogOr{flp, sampled(lhs.finalCondp->clone_درخت_خالص(جھوٹ)),
                                     sampled(rhs.finalCondp->clone_درخت_خالص(جھوٹ))},
                        flp);
        } ورنہ {
            اگر (lhs.finalCondp) {
                guardedLink(lhs.رکن_Vertexp, ملاؤ_Vtxp,
                            sampled(lhs.finalCondp->clone_درخت_خالص(جھوٹ)), flp);
            } ورنہ {
                guardedLink(lhs.رکن_Vertexp, ملاؤ_Vtxp, flp);
            }
            اگر (rhs.finalCondp) {
                guardedLink(rhs.رکن_Vertexp, ملاؤ_Vtxp,
                            sampled(rhs.finalCondp->clone_درخت_خالص(جھوٹ)), flp);
            } ورنہ {
                guardedLink(rhs.رکن_Vertexp, ملاؤ_Vtxp, flp);
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        واپس {ملاؤ_Vtxp, خالی_اشاریہ, {}};
    }

    // Build done-latch combiner for SAnd/SIntersect (IEEE 1800-2023 16.9.5).
    Build_نتیجہ buildAndCombiner(Ast_گرہ_اظہاریہ* lhsExprp, Ast_گرہ_اظہاریہ* rhsExprp,
                                 Sva_State_راس* اندراج_Vtxp, File_سطر* flp) {
        // Snapshot-restore scope so LHS liveness does not leak into RHS.
        ثابت منطقی savedScope = m_اندر_Unbounded_Scope;
        ثابت Build_نتیجہ lhs = build_اظہاریہ(lhsExprp, اندراج_Vtxp);
        ثابت منطقی lhsScope = m_اندر_Unbounded_Scope;
        m_اندر_Unbounded_Scope = savedScope;
        ثابت Build_نتیجہ rhs = build_اظہاریہ(rhsExprp, اندراج_Vtxp);
        ثابت منطقی rhsScope = m_اندر_Unbounded_Scope;
        m_اندر_Unbounded_Scope = savedScope || lhsScope || rhsScope;
        اگر (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            واپس Build_نتیجہ::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP

        // Single-cycle operands: use boolean AND (done-latch would fire across cycles).
        // If both operands stayed at entry, they must be boolean leaves which
        // buildExpr returns with finalCondp=nodep (non-null).
        اگر (lhs.رکن_Vertexp == اندراج_Vtxp && rhs.رکن_Vertexp == اندراج_Vtxp) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsExprp,
                        "Single-cycle SAnd operands must have finalCondp");
            Ast_گرہ_اظہاریہ* ثابت condp = نیا AstLogAnd{flp, lhs.finalCondp->clone_درخت_خالص(جھوٹ),
                                                     rhs.finalCondp->clone_درخت_خالص(جھوٹ)};
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            واپس {اندراج_Vtxp, condp, {}};
        }
        // Range-delay mid-window sources in either sub-branch would need
        // to be folded into the latch's match-now signal, which the
        // current combiner does not support.
        اگر (!lhs.midSources.خالی_خانے() || !rhs.midSources.خالی_خانے()) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: ranged cycle delay in an operand of property 'and'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            واپس Build_نتیجہ::failWithError();
        }
        Sva_State_راس* ثابت combVtxp = scoped_Create_راس();
        combVtxp->m_isAndCombiner = سچ;
        combVtxp->m_andLhsTermp = lhs.رکن_Vertexp;
        combVtxp->m_andRhsTermp = rhs.رکن_Vertexp;
        اگر (lhs.finalCondp) combVtxp->m_andLhsCondp = lhs.finalCondp->clone_درخت_خالص(جھوٹ);
        اگر (rhs.finalCondp) combVtxp->m_andRhsCondp = rhs.finalCondp->clone_درخت_خالص(جھوٹ);
        اگر (lhs.رکن_Vertexp->m_isUnbounded || rhs.رکن_Vertexp->m_isUnbounded) {
            combVtxp->m_isUnbounded = سچ;
        }
        // Wire terminal-boolean rejects to a dedicated sink so each side can fail
        // the AND independently. Skip for liveness or single-cycle operands
        // (single-cycle termVertexp == entryVtxp would fire every cycle).
        اگر (!combVtxp->m_isUnbounded) {
            منطقی needSink = جھوٹ;
            ثابت منطقی lhs_Multi_چکر = (lhs.رکن_Vertexp != اندراج_Vtxp);
            ثابت منطقی rhs_Multi_چکر = (rhs.رکن_Vertexp != اندراج_Vtxp);
            اگر (lhs.finalCondp && lhs_Multi_چکر && !lhs.رکن_Vertexp->m_isUnbounded) {
                needSink = سچ;
            }
            اگر (rhs.finalCondp && rhs_Multi_چکر && !rhs.رکن_Vertexp->m_isUnbounded) {
                needSink = سچ;
            }
            اگر (needSink) {
                Sva_State_راس* ثابت sinkVtxp = m_گراف.create_State_راس();
                sinkVtxp->m_isRejectSink = سچ;
                اگر (lhs.finalCondp && lhs_Multi_چکر && !lhs.رکن_Vertexp->m_isUnbounded) {
                    Sva_Trans_کنارہ* ثابت ep = m_گراف.جوڑو_Link(
                        lhs.رکن_Vertexp, sinkVtxp, sampled(lhs.finalCondp->clone_درخت_خالص(جھوٹ)));
                    ep->m_rejectOnFail = سچ;
                }
                اگر (rhs.finalCondp && rhs_Multi_چکر && !rhs.رکن_Vertexp->m_isUnbounded) {
                    Sva_Trans_کنارہ* ثابت ep = m_گراف.جوڑو_Link(
                        rhs.رکن_Vertexp, sinkVtxp, sampled(rhs.finalCondp->clone_درخت_خالص(جھوٹ)));
                    ep->m_rejectOnFail = سچ;
                }
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        واپس {combVtxp, خالی_اشاریہ, {}};
    }

    // Lower `seq1 within seq2` (IEEE 1800-2023 16.9.10) as:
    //   (OR_{i in 0..slack} 1 ##i seq1 ##(slack-i) 1) intersect seq2
    // Both operands must have fixed length (no ranged cycle delays).
    // The OR lives inside a single SIntersect so one AndCombiner done-latch
    // serves all offsets; lifting it out would double-count matches where
    // multiple offsets accept on the same seq2 end cycle.
    Build_نتیجہ buildSWithin(AstSWithin* nodep, Sva_State_راس* اندراج_Vtxp,
                             منطقی is_چوٹی_Level_قدم = جھوٹ) {
        ثابت صحیح innerLen = fixed_لمبائی(nodep->lhsp());
        ثابت صحیح outerLen = fixed_لمبائی(nodep->rhsp());
        اگر (innerLen < 0 || outerLen < 0) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: within with ranged cycle-delay operand");
            واپس Build_نتیجہ::failWithError();
        }
        اگر (innerLen > outerLen) {
            واپس build_Never_مطابقت_Intersect(
                nodep, اندراج_Vtxp, is_چوٹی_Level_قدم,
                "the inner sequence is longer than the outer sequence");
        }
        File_سطر* ثابت flp = nodep->fileline();
        ثابت صحیح slack = outerLen - innerLen;
        Ast_گرہ_اظہاریہ* innerOrp = خالی_اشاریہ;
        برائے (صحیح i = 0; i <= slack; ++i) {
            ثابت صحیح postPad = slack - i;
            Ast_گرہ_اظہاریہ* branchp = nodep->lhsp()->clone_درخت_خالص(جھوٹ);
            اگر (i > 0) {
                AstConst* ثابت prePadp = نیا AstConst{flp, AstConst::ثنائی_ہندسہ_True{}};
                AstDelay* ثابت delayp
                    = نیا AstDelay{flp, نیا AstConst{flp, static_cast<uint32_t>(i)}, سچ};
                AstSExpr* ثابت wrapped = نیا AstSExpr{flp, prePadp, delayp, branchp};
                wrapped->dtype_سیٹ_ثنائی_ہندسہ();
                branchp = wrapped;
            }
            اگر (postPad > 0) {
                AstConst* ثابت postTruep = نیا AstConst{flp, AstConst::ثنائی_ہندسہ_True{}};
                AstDelay* ثابت delayp
                    = نیا AstDelay{flp, نیا AstConst{flp, static_cast<uint32_t>(postPad)}, سچ};
                AstSExpr* ثابت wrapped = نیا AstSExpr{flp, branchp, delayp, postTruep};
                wrapped->dtype_سیٹ_ثنائی_ہندسہ();
                branchp = wrapped;
            }
            innerOrp = innerOrp ? static_cast<Ast_گرہ_اظہاریہ*>(نیا AstSOr{flp, innerOrp, branchp})
                                : branchp;
        }
        Ast_گرہ_اظہاریہ* ثابت outerClonep = nodep->rhsp()->clone_درخت_خالص(جھوٹ);
        Ast_گرہ_اظہاریہ* ثابت combinedp = نیا AstSIntersect{flp, innerOrp, outerClonep};
        Build_نتیجہ نتیجہ = build_اظہاریہ(combinedp, اندراج_Vtxp, is_چوٹی_Level_قدم);
        VL_DO_DANGLING(combinedp->delete_درخت(), combinedp);
        // When both operands are plain booleans, buildAndCombiner returns a
        // freshly-allocated AstAnd as finalCondp with no parent. Callers up
        // the chain clone-and-discard finalCondp, so leave it parent-less and
        // it leaks; anchor it in the graph now via an edge.
        اگر (نتیجہ.valid() && نتیجہ.finalCondp && !نتیجہ.finalCondp->backp()) {
            Sva_State_راس* ثابت wrapVtxp = scoped_Create_راس();
            guardedLink(نتیجہ.رکن_Vertexp, wrapVtxp, sampled(نتیجہ.finalCondp), flp);
            نتیجہ = {wrapVtxp, خالی_اشاریہ, نتیجہ.midSources};
        }
        واپس نتیجہ;
    }

    // Collect the boolean leaf checks of a fixed-length sequence keyed by their
    // clock offset from the start. Returns false for anything other than nested
    // AstSExpr with fixed cycle delays over boolean leaves (e.g. throughout).
    ساکن منطقی flattenFixedSeq(Ast_گرہ_اظہاریہ* nodep, صحیح بنیاد_انحراف,
                                std::map<صحیح, std::سمتیہ<Ast_گرہ_اظہاریہ*>>& برآمد) {
        اگر (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
            اگر (!delayp || !delayp->is_چکر_Delay() || delayp->isUnbounded()) واپس جھوٹ;
            ثابت غیرعلامتی delay_ادوار = getConstUInt(delayp->lhsp());
            اگر (delayp->isRangeDelay() && getConstUInt(delayp->rhsp()) != delay_ادوار)
                واپس جھوٹ;
            صحیح preLen = 0;
            اگر (Ast_گرہ_اظہاریہ* ثابت prep = sexprp->preExprp()) {
                اگر (!flattenFixedSeq(prep, بنیاد_انحراف, برآمد)) واپس جھوٹ;
                preLen = fixed_لمبائی(prep);
                اگر (preLen < 0) واپس جھوٹ;
            }
            واپس flattenFixedSeq(sexprp->exprp(), بنیاد_انحراف + preLen + delay_ادوار, برآمد);
        }
        اگر (nodep->is_Multi_چکر_Sva()) واپس جھوٹ;
        برآمد[بنیاد_انحراف].ڈالو_عقب(nodep);
        واپس سچ;
    }

    // Conjoin two equal-length fixed sequences into one: at each clock offset
    // AND the boolean checks of both operands (IEEE 1800-2023 16.9.6 -- both
    // operands match the same window). Returns null if either operand is not a
    // plain fixed sequence of boolean leaves.
    ساکن Ast_گرہ_اظہاریہ* conjoinFixedSeqs(Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp, File_سطر* flp) {
        std::map<صحیح, std::سمتیہ<Ast_گرہ_اظہاریہ*>> checks;
        اگر (!flattenFixedSeq(lhsp, 0, checks) || !flattenFixedSeq(rhsp, 0, checks)) واپس خالی_اشاریہ;
        اگر (checks.خالی_خانے()) واپس خالی_اشاریہ;
        Ast_گرہ_اظہاریہ* resultp = خالی_اشاریہ;
        صحیح پچھلا_انحراف = 0;
        برائے (ثابت خودکار& انحراف_Checks : checks) {
            ثابت صحیح انحراف = انحراف_Checks.پہلا;
            Ast_گرہ_اظہاریہ* condp = خالی_اشاریہ;
            برائے (Ast_گرہ_اظہاریہ* ثابت leafp : انحراف_Checks.دوسرا) {
                Ast_گرہ_اظہاریہ* ثابت clonep = leafp->clone_درخت_خالص(جھوٹ);
                اگر (!condp) {
                    condp = clonep;
                } ورنہ {
                    condp = نیا AstLogAnd{flp, condp, clonep};
                    condp->dtype_سیٹ_ثنائی_ہندسہ();
                }
            }
            اگر (!resultp) {
                اگر (انحراف > 0) {
                    AstDelay* ثابت delayp = نیا AstDelay{
                        flp, نیا AstConst{flp, static_cast<uint32_t>(انحراف)}, /*isCycle=*/سچ};
                    resultp
                        = نیا AstSExpr{flp, نیا AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}, delayp, condp};
                    resultp->dtype_سیٹ_ثنائی_ہندسہ();
                } ورنہ {
                    resultp = condp;
                }
            } ورنہ {
                AstDelay* ثابت delayp = نیا AstDelay{
                    flp, نیا AstConst{flp, static_cast<uint32_t>(انحراف - پچھلا_انحراف)},
                    /*isCycle=*/سچ};
                resultp = نیا AstSExpr{flp, resultp, delayp, condp};
                resultp->dtype_سیٹ_ثنائی_ہندسہ();
            }
            پچھلا_انحراف = انحراف;
        }
        واپس resultp;
    }

    // `seq` is a simple ranged sequence `start ##[m:n] end` (start/end boolean,
    // start may be absent). Used to collapse a both-variable intersect to one
    // ranged delay.
    ساخت SimpleRanged final {
        منطقی ok = جھوٹ;
        Ast_گرہ_اظہاریہ* startp = خالی_اشاریہ;  // may be null (absent start)
        Ast_گرہ_اظہاریہ* endp = خالی_اشاریہ;
    };
    ساکن SimpleRanged asSimpleRanged(Ast_گرہ_اظہاریہ* nodep) {
        AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr);
        اگر (!sexprp) واپس {};
        AstDelay* ثابت delayp = VN_CAST(sexprp->delayp(), Delay);
        اگر (!delayp || !delayp->is_چکر_Delay() || !delayp->isRangeDelay() || delayp->isUnbounded())
            واپس {};
        اگر (getConstUInt(delayp->lhsp()) == getConstUInt(delayp->rhsp())) واپس {};
        Ast_گرہ_اظہاریہ* ثابت prep = sexprp->preExprp();
        اگر (prep && fixed_لمبائی(prep) != 0) واپس {};
        اگر (fixed_لمبائی(sexprp->exprp()) != 0) واپس {};
        واپس {سچ, prep, sexprp->exprp()};
    }

    // Build the NFA for a synthesized intersect lowering tree, then free it.
    // buildExpr returns the terminal condition (finalCondp) by reference into the
    // tree; detach a clone so the tree can be freed here. The graph already holds
    // clones/hoists of every edge condition, so nothing else dangles.
    Build_نتیجہ build_From_Lowering_درخت(Ast_گرہ_اظہاریہ* treep, Sva_State_راس* اندراج_Vtxp,
                                      منطقی is_چوٹی_Level_قدم) {
        Build_نتیجہ نتیجہ = build_اظہاریہ(treep, اندراج_Vtxp, is_چوٹی_Level_قدم);
        اگر (نتیجہ.valid() && نتیجہ.finalCondp) {
            نتیجہ.finalCondp = نتیجہ.finalCondp->clone_درخت_خالص(جھوٹ);
        }
        VL_DO_DANGLING(treep->delete_درخت(), treep);
        واپس نتیجہ;
    }

    // Empty common-length intersection -- unequal fixed lengths, or disjoint
    // ranged lengths. IEEE 1800-2023 16.9.6 requires both operands to match
    // over a window of the same length, so with no common length the intersect
    // simply never matches. This is legal (matching nothing), not an error, so
    // lower to a constant false rather than rejecting legal code.
    Build_نتیجہ build_Never_مطابقت_Intersect(Ast_گرہ_اظہاریہ* nodep, Sva_State_راس* اندراج_Vtxp,
                                         منطقی is_چوٹی_Level_قدم, ثابت حرف* reason) {
        nodep->v3warn(NEVERMATCH, "Sequence can never match because " << reason << ".");
        Ast_گرہ_اظہاریہ* ثابت falsep = نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}};
        واپس build_From_Lowering_درخت(falsep, اندراج_Vtxp, is_چوٹی_Level_قدم);
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
                                     منطقی is_چوٹی_Level_قدم) {
        ثابت std::pair<صحیح, صحیح> lhsRange = لمبائی_Range(nodep->lhsp());
        ثابت std::pair<صحیح, صحیح> rhsRange = لمبائی_Range(nodep->rhsp());
        اگر (lhsRange.پہلا < 0 || rhsRange.پہلا < 0) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect with this variable-length operand");
            واپس Build_نتیجہ::failWithError();
        }
        ثابت صحیح lo = std::max(lhsRange.پہلا, rhsRange.پہلا);
        ثابت صحیح hi = std::min(lhsRange.دوسرا, rhsRange.دوسرا);
        اگر (lo > hi) {
            // Disjoint length ranges share no common length -> never matches.
            واپس build_Never_مطابقت_Intersect(nodep, اندراج_Vtxp, is_چوٹی_Level_قدم,
                                            "intersect operands have no common length");
        }
        File_سطر* ثابت flp = nodep->fileline();
        اگر (lo == hi) {
            Ast_گرہ_اظہاریہ* ثابت lp = realize_At_لمبائی(nodep->lhsp(), lo, lhsRange.پہلا);
            Ast_گرہ_اظہاریہ* ثابت rp = realize_At_لمبائی(nodep->rhsp(), lo, rhsRange.پہلا);
            Ast_گرہ_اظہاریہ* ثابت conjp = conjoinFixedSeqs(lp, rp, flp);
            VL_DO_DANGLING(lp->delete_درخت(), lp);
            VL_DO_DANGLING(rp->delete_درخت(), rp);
            اگر (!conjp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: intersect operand is not a plain boolean sequence");
                واپس Build_نتیجہ::failWithError();
            }
            واپس build_From_Lowering_درخت(conjp, اندراج_Vtxp, is_چوٹی_Level_قدم);
        }
        ثابت SimpleRanged sl = asSimpleRanged(nodep->lhsp());
        ثابت SimpleRanged sr = asSimpleRanged(nodep->rhsp());
        اگر (!sl.ok || !sr.ok) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect of two sequences that each vary in length over a"
                          " range with internal structure");
            واپس Build_نتیجہ::failWithError();
        }
        ثابت خودکار andBool = [&](Ast_گرہ_اظہاریہ* ap, Ast_گرہ_اظہاریہ* bp) -> Ast_گرہ_اظہاریہ* {
            Ast_گرہ_اظہاریہ* ثابت aClonep
                = ap ? ap->clone_درخت_خالص(جھوٹ) : نیا AstConst{flp, AstConst::ثنائی_ہندسہ_True{}};
            Ast_گرہ_اظہاریہ* ثابت bClonep
                = bp ? bp->clone_درخت_خالص(جھوٹ) : نیا AstConst{flp, AstConst::ثنائی_ہندسہ_True{}};
            AstLogAnd* ثابت andp = نیا AstLogAnd{flp, aClonep, bClonep};
            andp->dtype_سیٹ_ثنائی_ہندسہ();
            واپس andp;
        };
        AstDelay* ثابت delayp = نیا AstDelay{flp, نیا AstConst{flp, static_cast<uint32_t>(lo)},
                                              /*isCycle=*/سچ};
        delayp->rhsp(نیا AstConst{flp, static_cast<uint32_t>(hi)});
        AstSExpr* ثابت reducedp
            = نیا AstSExpr{flp, andBool(sl.startp, sr.startp), delayp, andBool(sl.endp, sr.endp)};
        reducedp->dtype_سیٹ_ثنائی_ہندسہ();
        واپس build_From_Lowering_درخت(reducedp, اندراج_Vtxp, is_چوٹی_Level_قدم);
    }

    Build_نتیجہ buildThroughout(AstSThroughout* nodep, Sva_State_راس* اندراج_Vtxp,
                                منطقی is_چوٹی_Level_قدم = جھوٹ) {
        // Mark entryVtxp so "cond false at tick 0" is detected as throughout-drop.
        اندراج_Vtxp->m_throughoutConds.ڈالو_عقب(nodep->lhsp()->clone_درخت_خالص(جھوٹ));
        m_temporal_Guard_ڈھیر.ڈالو_عقب(nodep->lhsp());
        ثابت Build_نتیجہ نتیجہ = build_اظہاریہ(nodep->rhsp(), اندراج_Vtxp, is_چوٹی_Level_قدم);
        m_temporal_Guard_ڈھیر.نکالو_عقب();
        واپس نتیجہ;
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
    Build_نتیجہ buildUntil(AstUntil* nodep, Sva_State_راس* اندراج_Vtxp, منطقی is_چوٹی_Level_قدم) {
        File_سطر* ثابت flp = nodep->fileline();
        اگر (!is_چوٹی_Level_قدم) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            واپس Build_نتیجہ::failWithError();
        }
        Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp();
        Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp();
        ثابت خودکار hasSeq = [](ثابت Ast_گرہ_اظہاریہ* ep) {
            واپس ep->exists([](ثابت Ast_گرہ_اظہاریہ* np) { واپس np->is_Multi_چکر_Sva(); });
        };
        اگر (hasSeq(lhsp) || hasSeq(rhsp)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            واپس Build_نتیجہ::failWithError();
        }

        ثابت منطقی ov = nodep->isOverlapping();
        Ast_گرہ_اظہاریہ* ثابت lhsBitp = nodep->lhsp();
        Ast_گرہ_اظہاریہ* ثابت rhsBitp = nodep->rhsp();
        // p hoist count: continue, require (ov: 1 use; nov: 1 use). At least 2 uses.
        AstVar* ثابت pHoistp = tryHoistSampled(lhsBitp, flp, 2);
        // q hoist count: continue (1) + require nov (1) = 2; ov: continue only (1).
        AstVar* ثابت qHoistp = ov ? خالی_اشاریہ : tryHoistSampled(rhsBitp, flp, 2);

        Sva_State_راس* ثابت waitCp = scoped_Create_راس();
        Sva_State_راس* ثابت waitRp = scoped_Create_راس();
        waitCp->m_isUnbounded = سچ;
        waitRp->m_strongPending = nodep->isStrong() && !m_isCover;

        // Entry and back-loop Links carry no condition; throughout-folding still applies.
        guardedLink(اندراج_Vtxp, waitCp, flp);
        guardedLink(waitRp, waitCp, flp);

        // Continue clocked edge: p && !q advances to next-cycle wait.
        Ast_گرہ_اظہاریہ* ثابت contCondp
            = نیا AstLogAnd{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                            نیا AstLogNot{flp, sampledRefOrClone(qHoistp, rhsBitp, flp)}};
        guarded_کنارہ(waitCp, waitRp, contCondp, flp);

        // Reject sink: fires when require-condition is false.
        Sva_State_راس* ثابت sinkVtxp = m_گراف.create_State_راس();
        sinkVtxp->m_isRejectSink = سچ;
        Ast_گرہ_اظہاریہ* requireCondp;
        اگر (ov) {
            requireCondp = sampledRefOrClone(pHoistp, lhsBitp, flp);
        } ورنہ {
            requireCondp = نیا AstLogOr{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                                        sampledRefOrClone(qHoistp, rhsBitp, flp)};
        }
        Sva_Trans_کنارہ* ثابت rejEdgep = m_گراف.جوڑو_Link(waitCp, sinkVtxp, requireCondp);
        rejEdgep->m_rejectOnFail = سچ;

        // Accept condition rides via finalCondp; assembleResult $sampled-wraps it.
        Ast_گرہ_اظہاریہ* acceptCondp;
        اگر (ov) {
            acceptCondp
                = نیا AstLogAnd{flp, lhsBitp->clone_درخت_خالص(جھوٹ), rhsBitp->clone_درخت_خالص(جھوٹ)};
        } ورنہ {
            acceptCondp = rhsBitp->clone_درخت_خالص(جھوٹ);
        }
        واپس {waitCp, acceptCondp, {}};
    }

    // IEEE 1800-2023 16.12.14 property abort operators. Sync and async share
    // the same NFA encoding: AstSampled already gives matured values at every
    // maturing clocking event, and async firing "between clocks" is not
    // observable in a cycle-based model. VAbortKind selects accept vs reject
    // verdict (sync vs async only changes the user-visible spelling).

    // Build `condp && !outer_1 && !outer_2 ...` (unsampled).
    Ast_گرہ_اظہاریہ* abort_Fire_اظہاریہ(Ast_گرہ_اظہاریہ* condp, File_سطر* flp) {
        Ast_گرہ_اظہاریہ* resultp = condp->clone_درخت_خالص(جھوٹ);
        برائے (Ast_گرہ_اظہاریہ* ثابت op : m_outer_Abort_ڈھیر)
            resultp = نیا AstLogAnd{flp, resultp, نیا AstLogNot{flp, op->clone_درخت_خالص(جھوٹ)}};
        واپس resultp;
    }

    // True when unguarded ring-wide rejection or a same-tick Link chain already accounts the
    // attempt: a required-step Link covers both outcomes; followed-by pairs both edges.
    ساکن منطقی chain_Accounts_ماخذ(ثابت Sva_State_راس* srcp,
                                    ثابت std::unordered_سیٹ<ثابت V3_گراف_کنارہ*>& pre_کنارے) {
        اگر (srcp->m_delay_Ring_سائز && srcp->m_throughoutConds.خالی_خانے()) واپس سچ;
        برائے (ثابت V3_گراف_کنارہ& edger : srcp->اندر_کنارے()) {
            اگر (pre_کنارے.گنتی(&edger)) جاری;
            ثابت Sva_Trans_کنارہ& tedger = static_cast<ثابت Sva_Trans_کنارہ&>(edger);
            اگر (tedger.m_consumes_چکر) جاری;
            ثابت خودکار* ثابت fromp = static_cast<ثابت Sva_State_راس*>(tedger.fromVtxp());
            اگر (fromp->m_abortRejectp) واپس سچ;
        }
        منطقی plainNonSink = جھوٹ;
        منطقی markedSink = جھوٹ;
        برائے (ثابت V3_گراف_کنارہ& edger : srcp->برآمد_کنارے()) {
            اگر (pre_کنارے.گنتی(&edger)) جاری;
            ثابت Sva_Trans_کنارہ& tedger = static_cast<ثابت Sva_Trans_کنارہ&>(edger);
            اگر (tedger.m_consumes_چکر) جاری;
            ثابت منطقی sink = static_cast<ثابت Sva_State_راس*>(tedger.مقصد_Vtxp())->m_isRejectSink;
            اگر (tedger.m_rejectOnFail) {
                اگر (!sink) واپس سچ;
                markedSink = سچ;
            } ورنہ اگر (!sink) {
                اگر (srcp->m_abortRejectp) واپس سچ;
                plainNonSink = سچ;
            }
        }
        واپس plainNonSink && markedSink;
    }

    // Reject edge: fires when the source is live and the abort samples true.
    خالی جوڑو_Abort_Reject_کنارہ(Sva_State_راس* srcp, Sva_State_راس* sinkp, Ast_گرہ_اظہاریہ* condp,
                            File_سطر* flp) {
        Ast_گرہ_اظہاریہ* ثابت notFirep = نیا AstLogNot{flp, sampled(abort_Fire_اظہاریہ(condp, flp))};
        m_گراف.جوڑو_Link(srcp, sinkp, notFirep)->m_rejectOnFail = سچ;
        واپس;
    }

    // On the fire tick: kill body threads; accept kinds also forgive step misses.
    خالی gate_Body_کنارے_On_Abort(ثابت std::unordered_سیٹ<ثابت V3_گراف_کنارہ*>& pre_کنارے,
                              Ast_گرہ_اظہاریہ* condp, VAbort_قسم قسم, File_سطر* flp) {
        برائے (V3_گراف_راس& vtxr : m_گراف.m_گراف.vertices()) {
            برائے (V3_گراف_کنارہ& edger : vtxr.برآمد_کنارے()) {
                اگر (pre_کنارے.گنتی(&edger)) جاری;
                Sva_Trans_کنارہ* ثابت tedgep = static_cast<Sva_Trans_کنارہ*>(&edger);
                اگر (tedgep->m_rejectOnFail) {
                    اگر (!قسم.isAccept()) جاری;
                    Ast_گرہ_اظہاریہ* ثابت firep = sampled(abort_Fire_اظہاریہ(condp, flp));
                    tedgep->m_condp
                        = tedgep->m_condp ? نیا AstLogOr{flp, tedgep->m_condp, firep} : firep;
                } ورنہ اگر (tedgep->m_consumes_چکر) {
                    Ast_گرہ_اظہاریہ* ثابت notFirep
                        = نیا AstLogNot{flp, sampled(abort_Fire_اظہاریہ(condp, flp))};
                    tedgep->m_condp = tedgep->m_condp
                                          ? نیا AstLogAnd{flp, tedgep->m_condp, notFirep}
                                          : notFirep;
                }
            }
        }
    }

    Build_نتیجہ buildAbortOn(Ast_گرہ_اظہاریہ* condp, Ast_گرہ_اظہاریہ* bodyp, Sva_State_راس* اندراج_Vtxp,
                             VAbort_قسم قسم, File_سطر* flp, منطقی is_چوٹی_Level_قدم) {
        // Snapshot pre-body vertices/edges so post-build diff yields the body's sub-NFA.
        std::unordered_سیٹ<ثابت V3_گراف_راس*> preExisting;
        std::unordered_سیٹ<ثابت V3_گراف_کنارہ*> pre_کنارے;
        برائے (V3_گراف_راس& vtxr : m_گراف.m_گراف.vertices()) {
            preExisting.داخل_کرو(&vtxr);
            برائے (V3_گراف_کنارہ& edger : vtxr.برآمد_کنارے()) pre_کنارے.داخل_کرو(&edger);
        }

        m_outer_Abort_ڈھیر.ڈالو_عقب(condp);
        ثابت Build_نتیجہ body_نتیجہ = build_اظہاریہ(bodyp, اندراج_Vtxp, is_چوٹی_Level_قدم);
        m_outer_Abort_ڈھیر.نکالو_عقب();
        اگر (!body_نتیجہ.valid()) واپس body_نتیجہ;

        gate_Body_کنارے_On_Abort(pre_کنارے, condp, قسم, flp);

        // Live-thread sources for the abort edge: entry + new body vertices,
        // minus reject sinks (they carry reject fuel, not live-thread fuel).
        std::سمتیہ<Sva_State_راس*> abortSources;
        abortSources.ڈالو_عقب(اندراج_Vtxp);
        برائے (V3_گراف_راس& vtxr : m_گراف.m_گراف.vertices()) {
            اگر (preExisting.گنتی(&vtxr)) جاری;
            خودکار* ثابت sp = static_cast<Sva_State_راس*>(&vtxr);
            اگر (sp->m_delay_Ring_سائز || !sp->m_throughoutConds.خالی_خانے()) {
                Ast_گرہ_اظہاریہ* ثابت firep = abort_Fire_اظہاریہ(condp, flp);
                sp->m_abortClearp
                    = sp->m_abortClearp ? نیا AstLogOr{flp, sp->m_abortClearp, firep} : firep;
            }
            اگر (!قسم.isAccept()
                && ((sp->m_delay_Ring_سائز && sp->m_throughoutConds.خالی_خانے())
                    || sp->m_replayAbortReject)) {
                Ast_گرہ_اظہاریہ* ثابت firep = abort_Fire_اظہاریہ(condp, flp);
                sp->m_abortRejectp
                    = sp->m_abortRejectp ? نیا AstLogOr{flp, sp->m_abortRejectp, firep} : firep;
            }
            اگر (sp->m_isRejectSink) جاری;
            abortSources.ڈالو_عقب(sp);
        }

        خودکار sampledAbortFire = [&]() -> Ast_گرہ_اظہاریہ* {
            Ast_گرہ_اظہاریہ* ثابت اظہاریہ = abort_Fire_اظہاریہ(condp, flp);
            واپس sampled(اظہاریہ);
        };

        اگر (قسم.isAccept()) {
            // Match-only sink fed by $sampled(abort-fire) from every live source;
            // registered as midSource so it never contributes a reject.
            Sva_State_راس* ثابت acceptSinkp = scoped_Create_راس();
            برائے (Sva_State_راس* ثابت srcp : abortSources)
                guardedLink(srcp, acceptSinkp, sampledAbortFire(), flp);
            std::سمتیہ<Sva_State_راس*> midSources = body_نتیجہ.midSources;
            midSources.ڈالو_عقب(acceptSinkp);
            Ast_گرہ_اظہاریہ* finalCondp = body_نتیجہ.finalCondp;
            اگر (finalCondp) {
                اگر (finalCondp->backp()) finalCondp = finalCondp->clone_درخت_خالص(جھوٹ);
                finalCondp = نیا AstLogOr{flp, finalCondp, abort_Fire_اظہاریہ(condp, flp)};
            }
            واپس {body_نتیجہ.رکن_Vertexp, finalCondp, std::move(midSources)};
        }

        // rejectOnFail treats m_condp as the success condition and fires on
        // !condp, so the edge carries !sampledAbortFire().
        Sva_State_راس* ثابت rejectSinkp = m_گراف.create_State_راس();
        rejectSinkp->m_isRejectSink = سچ;
        برائے (Sva_State_راس* ثابت srcp : abortSources)
            اگر (!chain_Accounts_ماخذ(srcp, pre_کنارے))
                جوڑو_Abort_Reject_کنارہ(srcp, rejectSinkp, condp, flp);
        Ast_گرہ_اظہاریہ* finalCondp = body_نتیجہ.finalCondp;
        اگر (finalCondp) {
            اگر (finalCondp->backp()) finalCondp = finalCondp->clone_درخت_خالص(جھوٹ);
            finalCondp
                = نیا AstLogAnd{flp, finalCondp, نیا AstLogNot{flp, abort_Fire_اظہاریہ(condp, flp)}};
        }
        واپس {body_نتیجہ.رکن_Vertexp, finalCondp, body_نتیجہ.midSources};
    }

عوامی:
    // Free a dropped sub-result condition that is not linked into the AST
    // (abort folds synthesize unparented finalCondp trees).
    ساکن خالی freeUnlinkedCondp(Ast_گرہ_اظہاریہ* condp) {
        اگر (condp && !condp->backp()) VL_DO_DANGLING(condp->delete_درخت(), condp);
    }

    SvaNfaBuilder(Sva_گراف& گراف, Ast_گرہ_Module* modp, V3_Unique_نام& prop_عارضی_نام,
                  منطقی isCoverSeq = جھوٹ, منطقی isSeqEvent = جھوٹ, منطقی isCover = جھوٹ)
        : m_گراف{گراف}
        , m_modp{modp}
        , m_prop_عارضی_نام{prop_عارضی_نام}
        , m_isCover{isCover}
        , m_isCoverSeq{isCoverSeq}
        , m_isSeqEvent{isSeqEvent} {}

    // Reset scope between antecedent and consequent: liveness must not leak.
    // m_outerAbortStack survives: an abort wrapping the implication covers the
    // consequent too (IEEE 1800-2023 16.12.14).
    خالی resetScope() {
        m_اندر_Unbounded_Scope = جھوٹ;
        m_temporal_Guard_ڈھیر.clear();
    }

    Build_نتیجہ build_اظہاریہ(Ast_گرہ_اظہاریہ* nodep, Sva_State_راس* اندراج_Vtxp,
                          منطقی is_چوٹی_Level_قدم = جھوٹ) {
        VL_RESTORER(m_want_Bool_برگ);
        ثابت منطقی want_Bool_برگ = m_want_Bool_برگ;
        m_want_Bool_برگ = جھوٹ;
        اگر (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            واپس buildSExpr(sexprp, اندراج_Vtxp, is_چوٹی_Level_قدم);
        }
        اگر (AstSConsRep* ثابت repp = VN_CAST(nodep, SConsRep)) {
            واپس buildConsRep(repp, اندراج_Vtxp, is_چوٹی_Level_قدم);
        }
        اگر (AstPropAlways* ثابت alwaysp = VN_CAST(nodep, PropAlways)) {
            واپس buildPropAlways(alwaysp, اندراج_Vtxp, is_چوٹی_Level_قدم);
        }
        اگر (AstSGotoRep* ثابت repp = VN_CAST(nodep, SGotoRep)) {
            واپس buildGotoRep(repp, اندراج_Vtxp);
        }
        اگر (AstSThroughout* ثابت throughoutp = VN_CAST(nodep, SThroughout)) {
            واپس buildThroughout(throughoutp, اندراج_Vtxp, is_چوٹی_Level_قدم);
        }
        اگر (AstSOr* ثابت orp = VN_CAST(nodep, SOr)) {
            واپس build_Or_ملاؤ(orp->lhsp(), orp->rhsp(), اندراج_Vtxp, orp->fileline(),
                                want_Bool_برگ);
        }
        اگر (AstLogOr* ثابت orp = VN_CAST(nodep, LogOr)) {
            واپس build_Or_ملاؤ(orp->lhsp(), orp->rhsp(), اندراج_Vtxp, orp->fileline(),
                                want_Bool_برگ);
        }
        اگر (AstSAnd* ثابت andp = VN_CAST(nodep, SAnd)) {
            واپس buildAndCombiner(andp->lhsp(), andp->rhsp(), اندراج_Vtxp, andp->fileline());
        }
        اگر (AstSIntersect* ثابت intp = VN_CAST(nodep, SIntersect)) {
            // IEEE 1800-2023 16.9.6: both operands match over one window with
            // equal start and end (equal length). Lower to a single sequence
            // that conjoins both operands' per-cycle checks -- correct under
            // concurrent attempts, where the done-latch combiner conflates the
            // two operands' start times and over-accepts. The combiner remains
            // only as a fallback for operands that do not flatten.
            ثابت صحیح lhsLen = fixed_لمبائی(intp->lhsp());
            ثابت صحیح rhsLen = fixed_لمبائی(intp->rhsp());
            اگر (lhsLen >= 0 && rhsLen >= 0) {
                اگر (lhsLen != rhsLen) {
                    // Unequal fixed lengths share no common length -> never matches.
                    واپس build_Never_مطابقت_Intersect(intp, اندراج_Vtxp, is_چوٹی_Level_قدم,
                                                    "intersect operands have no common length");
                }
                اگر (Ast_گرہ_اظہاریہ* ثابت conjp
                    = conjoinFixedSeqs(intp->lhsp(), intp->rhsp(), intp->fileline())) {
                    واپس build_From_Lowering_درخت(conjp, اندراج_Vtxp, is_چوٹی_Level_قدم);
                }
                واپس buildAndCombiner(intp->lhsp(), intp->rhsp(), اندراج_Vtxp, intp->fileline());
            }
            واپس buildVarLenIntersect(intp, اندراج_Vtxp, is_چوٹی_Level_قدم);
        }
        اگر (AstSWithin* ثابت withinp = VN_CAST(nodep, SWithin)) {
            واپس buildSWithin(withinp, اندراج_Vtxp, is_چوٹی_Level_قدم);
        }
        اگر (AstAbortOn* ثابت ap = VN_CAST(nodep, AbortOn)) {
            واپس buildAbortOn(ap->condp(), ap->propp(), اندراج_Vtxp, ap->قسم(), ap->fileline(),
                                is_چوٹی_Level_قدم);
        }
        اگر (VN_IS(nodep, SNonConsRep)) واپس Build_نتیجہ::fail();
        اگر (AstImplication* ثابت implp = VN_CAST(nodep, Implication)) {
            واپس build_Implication_کنارے(implp->lhsp(), implp->rhsp(), اندراج_Vtxp,
                                         implp->isOverlapped(), implp->isFollowedBy(),
                                         implp->lhsp(), implp->fileline());
        }
        اگر (AstUntil* ثابت untilp = VN_CAST(nodep, Until)) {
            واپس buildUntil(untilp, اندراج_Vtxp, is_چوٹی_Level_قدم);
        }
        // Boolean leaf (including LogAnd): return as finalCond
        واپس {اندراج_Vtxp, nodep, {}};
    }

    // Wire an implication / followed-by from `entryVtxp`: builds the antecedent,
    // emits the match-link (and for followed-by the reject-sink edge), inserts a
    // delay vertex for non-overlapped forms, and builds the body. Used both for
    // nested AstImplication in pexpr position and for the top-level assertion
    // antecedent -- `errorNodep` anchors the "unsupported sequence antecedent"
    // error, which differs between the two call sites.
    Build_نتیجہ build_Implication_کنارے(Ast_گرہ_اظہاریہ* antExprp, Ast_گرہ_اظہاریہ* bodyExprp,
                                      Sva_State_راس* اندراج_Vtxp, منطقی isOverlapped,
                                      منطقی isFollowedBy, Ast_گرہ* errorNodep, File_سطر* flp) {
        ثابت Build_نتیجہ ant_نتیجہ = build_اظہاریہ(antExprp, اندراج_Vtxp);
        اگر (!ant_نتیجہ.valid()) واپس ant_نتیجہ;

        // Followed-by requires pure-boolean antecedent for non-vacuous-fail at
        // the attempt-start cycle. IEEE 1800-2023 16.12.9 permits a multi-cycle
        // sequence LHS, so this is an implementation gap rather than illegal SV.
        اگر (isFollowedBy && ant_نتیجہ.رکن_Vertexp != اندراج_Vtxp) {
            errorNodep->v3warn(E_UNSUPPORTED,
                               "Unsupported: sequence expression as antecedent of followed-by"
                               " (#-# / #=#) (IEEE 1800-2023 16.12.9)");
            واپس Build_نتیجہ::failWithError();
        }
        UASSERT_OBJ(!isFollowedBy || ant_نتیجہ.finalCondp, errorNodep,
                    "followed-by antecedent terminal at entry must carry finalCondp");

        // Use raw createStateVertex() so trigVtxp starts without liveness --
        // reaching the antecedent terminal is a definitive event.
        Sva_State_راس* ثابت trigVtxp = m_گراف.create_State_راس();
        اگر (ant_نتیجہ.finalCondp) {
            m_گراف.جوڑو_Link(ant_نتیجہ.رکن_Vertexp, trigVtxp,
                            sampled(ant_نتیجہ.finalCondp->clone_درخت_خالص(جھوٹ)));
            // Followed-by non-vacuous fail: rejectOnFail fires when the attempt
            // is live (termVtx reachable) and sampled(antecedent) is false.
            اگر (isFollowedBy) {
                Sva_State_راس* ثابت sinkVtxp = m_گراف.create_State_راس();
                sinkVtxp->m_isRejectSink = سچ;
                Sva_Trans_کنارہ* ثابت ep
                    = m_گراف.جوڑو_Link(ant_نتیجہ.رکن_Vertexp, sinkVtxp,
                                      sampled(ant_نتیجہ.finalCondp->clone_درخت_خالص(جھوٹ)));
                ep->m_rejectOnFail = سچ;
            }
            // finalCondp is cloned into the Sampled nodes; if the original is
            // not parented anywhere in the AST anymore it must be freed here
            // or ASan flags it as a leak (e.g. t_sequence_bool_ops).
            اگر (!ant_نتیجہ.finalCondp->backp()) {
                VL_DO_DANGLING(ant_نتیجہ.finalCondp->delete_درخت(), ant_نتیجہ.finalCondp);
            }
        } ورنہ {
            m_گراف.جوڑو_Link(ant_نتیجہ.رکن_Vertexp, trigVtxp);
        }
        resetScope();

        Sva_State_راس* bodyEntryp = trigVtxp;
        اگر (!isOverlapped) {
            Sva_State_راس* ثابت delayVtxp = m_گراف.create_State_راس();
            m_گراف.جوڑو_Clocked_کنارہ(trigVtxp, delayVtxp);
            bodyEntryp = delayVtxp;
        }
        // An empty antecedent match must not start the consequent, but a zero-minimum
        // repetition's empty match is not handled here; keep the merge vertex for those
        VL_RESTORER(m_want_Bool_برگ);
        m_want_Bool_برگ = !isFollowedBy && !hasZeroMinRep(antExprp);
        واپس build_اظہاریہ(bodyExprp, bodyEntryp, /*isTopLevelStep=*/سچ);
    }

    Build_نتیجہ build(Ast_گرہ_اظہاریہ* exprp) {
        m_گراف.m_آغاز_Vertexp = scoped_Create_راس();
        VL_RESTORER(m_want_Bool_برگ);
        m_want_Bool_برگ = سچ;
        واپس build_اظہاریہ(exprp, m_گراف.m_آغاز_Vertexp, /*isTopLevelStep=*/سچ);
    }
};

//######################################################################
// NFA Lowering (converts NFA graph to synthesizable AstAlways blocks)

جماعت SvaNfaLowering final {
    Ast_گرہ_Module* ثابت m_modp;  // Module to add state vars and always blocks to
    Ast_گرہ_DType* ثابت m_u32DTypep;  // Shared unsigned counter dtype
    V3_Unique_نام m_نام{"__Vnfa"};
    سائز_t m_stat_Delay_Ring_کنارہ_Visits = 0;  // Delay-ring incoming edges visited

    // Per-lowering shared context (passed to phase sub-functions)
    // Per-vertex lowering state is stored in SvaVertexData and accessed via
    // V3GraphVertex::userp() (see SvaStateVertex::datap()).
    ساخت زیریں_Ctx final {
        File_سطر* flp;  // Source location for generated AST
        Ast_Sen_درخت* senTreep;  // Clock sensitivity tree
        Ast_گرہ_اظہاریہ* disableExprp;  // disable iff expression (may be nullptr)
        Ast_گرہ_اظہاریہ* مطابقت_Condp;  // Final boolean match condition (may be nullptr)
        VAssertType assertType;  // Assertion type for control tasks
        VAssertDirectiveType directiveType;  // Directive type for control tasks
        AstVar* killVarp;  // Last observed kill generation
        Sva_گراف& گراف;  // NFA graph
    };

    // Build a match-now expression: stateSig[i] && $sampled(condp)
    ساکن Ast_گرہ_اظہاریہ* build_مطابقت_Now(File_سطر* flp, Ast_گرہ_اظہاریہ* stateExprp, Ast_گرہ_اظہاریہ* condp) {
        Ast_گرہ_اظہاریہ* ثابت statep = stateExprp->clone_درخت_خالص(جھوٹ);
        اگر (!condp) واپس statep;
        واپس نیا AstLogAnd{flp, statep, sampled(condp->clone_درخت_خالص(جھوٹ))};
    }
    ساکن Ast_گرہ_اظہاریہ* andCond(File_سطر* flp, Ast_گرہ_اظہاریہ* exprp, Ast_گرہ_اظہاریہ* condp) {
        اگر (!condp) واپس exprp;
        واپس نیا AstLogAnd{flp, exprp, condp->clone_درخت_خالص(جھوٹ)};
    }
    // bp is always non-null; only ap can be null (serving as accumulator).
    ساکن Ast_گرہ_اظہاریہ* orExprs(File_سطر* flp, Ast_گرہ_اظہاریہ* ap, Ast_گرہ_اظہاریہ* bp) {
        اگر (!ap) واپس bp;
        واپس نیا AstLogOr{flp, ap, bp};
    }
    ساکن Ast_گرہ_اظہاریہ* جوڑو_Countp(File_سطر* ثابت flp, Ast_گرہ_اظہاریہ* ثابت کل_Countp,
                                  Ast_گرہ_اظہاریہ* ثابت contributionp,
                                  Ast_گرہ_اظہاریہ* ثابت enablep = خالی_اشاریہ) {
        // contribution = enable ? contribution : 0;
        Ast_گرہ_اظہاریہ* ثابت enabledContributionp
            = enablep ? نیا AstCond{flp, enablep, contributionp,
                                    newTypedConstp(flp, contributionp->dtypep(), 0)}
                      : contributionp;
        اگر (!کل_Countp) واپس enabledContributionp;
        واپس نیا Ast_جوڑو{flp, کل_Countp, enabledContributionp};
    }
    ساکن Ast_گرہ_اظہاریہ* gateCheckedCountp(File_سطر* ثابت flp, Ast_گرہ_اظہاریہ* ثابت countp,
                                          Ast_گرہ_اظہاریہ*& overflowp, Ast_گرہ_اظہاریہ* ثابت enablep) {
        اگر (overflowp) {
            overflowp = نیا AstLogAnd{flp, enablep->clone_درخت_خالص(جھوٹ), overflowp};
        }
        واپس جوڑو_Countp(flp, خالی_اشاریہ, countp, enablep);
    }
    ساکن Ast_گرہ_اظہاریہ* جوڑو_Checked_Countp(File_سطر* ثابت flp, Ast_گرہ_اظہاریہ* ثابت کل_Countp,
                                         Ast_گرہ_اظہاریہ* ثابت contributionp, Ast_گرہ_اظہاریہ*& overflowp,
                                         Ast_گرہ_اظہاریہ* ثابت contributionOverflowp = خالی_اشاریہ) {
        اگر (contributionOverflowp) { overflowp = orExprs(flp, overflowp, contributionOverflowp); }
        اگر (!کل_Countp) واپس contributionp;
        Ast_گرہ_اظہاریہ* ثابت oldTotalp = کل_Countp->clone_درخت_خالص(جھوٹ);
        Ast_گرہ_اظہاریہ* ثابت resultp = نیا Ast_جوڑو{flp, کل_Countp, contributionp};
        overflowp
            = orExprs(flp, overflowp, نیا AstLt{flp, resultp->clone_درخت_خالص(جھوٹ), oldTotalp});
        واپس resultp;
    }
    AstIf* new_گنتی_حد_سے_تجاوز_Warning(File_سطر* ثابت flp, Ast_گرہ_اظہاریہ* ثابت condp) ثابت {
        AstDisplay* ثابت dispp = نیا AstDisplay{
            flp, VDisplayType::DT_WARNING,
            "Cover sequence match count overflowed 32-bit storage; results may be incorrect.",
            خالی_اشاریہ, خالی_اشاریہ};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstIf* ثابت ifp = نیا AstIf{flp, condp, dispp};
        ifp->branchPred(VBranchPred::BP_UNLIKELY);
        واپس ifp;
    }
    ساکن Ast_گرہ_اظہاریہ* kill_فعال(زیریں_Ctx& c) {
        واپس نیا AstNeq{c.flp, نیا AstVarRef{c.flp, c.killVarp, VAccess::READ},
                          assertKillGet(c.flp, c.assertType, c.directiveType)};
    }
    ساکن Ast_گرہ_اظہاریہ* not_Kill_فعال(زیریں_Ctx& c) { واپس نیا AstLogNot{c.flp, kill_فعال(c)}; }
    ساکن Ast_گرہ_اظہاریہ* gateNotKill(زیریں_Ctx& c, Ast_گرہ_اظہاریہ* exprp) {
        اگر (!exprp) واپس خالی_اشاریہ;
        واپس نیا AstLogAnd{c.flp, exprp, not_Kill_فعال(c)};
    }
    ساکن Ast_گرہ_اظہاریہ* اگلا_Ring_اشاریہ(File_سطر* flp, Ast_گرہ_اظہاریہ* idxExprp, uint32_t سائز) {
        ثابت خودکار u32Const = [flp](uint32_t قدر) {
            واپس نیا AstConst{flp, AstConst::Widthed_قدر{}, 32, قدر};
        };
        UASSERT_OBJ(سائز > 0, idxExprp, "Ring size must be positive");
        اگر (سائز == 1) {
            VL_DO_DANGLING(idxExprp->delete_درخت(), idxExprp);
            واپس u32Const(0);
        }
        // idx == size - 1 ? 0 : idx + 1
        Ast_جوڑو* ثابت addp = نیا Ast_جوڑو{flp, idxExprp->clone_درخت_خالص(جھوٹ), u32Const(1)};
        addp->dtypeFrom(idxExprp);
        AstCond* ثابت condp
            = نیا AstCond{flp, نیا AstEq{flp, idxExprp, u32Const(سائز - 1)}, u32Const(0), addp};
        condp->dtypeFrom(idxExprp);
        واپس condp;
    }
    ساکن Ast_گرہ_اظہاریہ* delay_Ring_خانہ(File_سطر* flp, Sva_State_راس* vtxp, Ast_گرہ_اظہاریہ* idxExprp,
                                      VAccess access = VAccess::READ) {
        // ring[idx]
        AstVar* ثابت ringp = vtxp->datap()->delayRingVarp;
        AstVarRef* ثابت refp = نیا AstVarRef{flp, ringp, access};
        اگر (vtxp->datap()->delayRingCounts) واپس نیا AstArraySel{flp, refp, idxExprp};
        واپس نیا AstSel{flp, refp, idxExprp, 1};
    }
    ساکن Ast_گرہ_اظہاریہ* delay_Ring_سر_گنتی(File_سطر* flp, Sva_State_راس* vtxp) {
        Ast_گرہ_اظہاریہ* ثابت headp
            = نیا AstVarRef{flp, vtxp->datap()->delay_Ring_سر_Varp, VAccess::READ};
        اگر (vtxp->datap()->delayRingCounts) واپس headp;
        واپس نیا AstExtend{flp, headp, vtxp->datap()->delay_Ring_زندہ_گنتی_Varp->dtypep()->چوڑائی()};
    }
    ساکن Ast_گرہ_اظہاریہ* delay_Ring_At_آخری_اشاریہ(File_سطر* ثابت flp, AstVar* ثابت idxp,
                                             ثابت uint32_t سائز) {
        واپس نیا AstEq{flp, نیا AstVarRef{flp, idxp, VAccess::READ},
                         نیا AstConst{flp, AstConst::Widthed_قدر{}, 32, سائز - 1}};
    }
    ساکن Ast_گرہ_اظہاریہ* delay_Ring_Output_گنتی(File_سطر* ثابت flp, Sva_State_راس* ثابت vtxp) {
        AstVar* ثابت idxp = vtxp->datap()->delayRingIdxVarp;
        ثابت uint32_t سائز = vtxp->m_delay_Ring_سائز;
        Ast_گرہ_اظہاریہ* outgoingValidp
            = نیا AstVarRef{flp, vtxp->datap()->delayRingWrappedVarp, VAccess::READ};
        اگر (!vtxp->m_isFixedDelayRing) {
            outgoingValidp
                = نیا AstLogOr{flp, outgoingValidp, delay_Ring_At_آخری_اشاریہ(flp, idxp, سائز)};
        }
        AstVar* ثابت زندہ_گنتی_Varp = vtxp->datap()->delay_Ring_زندہ_گنتی_Varp;
        واپس نیا AstCond{flp, outgoingValidp, delay_Ring_سر_گنتی(flp, vtxp),
                           newTypedConstp(flp, زندہ_گنتی_Varp->dtypep(), 0)};
    }
    ساکن Ast_گرہ_اظہاریہ* delay_Ring_Has_زندہ_Threadsp(File_سطر* ثابت flp,
                                                 AstVar* ثابت زندہ_گنتی_Varp) {
        // active = live_count != 0;
        واپس نیا AstNeq{flp, نیا AstVarRef{flp, زندہ_گنتی_Varp, VAccess::READ},
                          newTypedConstp(flp, زندہ_گنتی_Varp->dtypep(), 0)};
    }

    // Phase 3 output signals
    ساخت Signal_سیٹ final {
        Ast_گرہ_اظہاریہ* terminalActivep = خالی_اشاریہ;  // OR of all successful terminal matches
        Ast_گرہ_اظہاریہ* مطابقت_Countp = خالی_اشاریہ;  // NFA paths completing the sequence this tick
        Ast_گرہ_اظہاریہ* مطابقت_گنتی_Overflowp = خالی_اشاریہ;  // Match count overflow this tick
        Ast_گرہ_اظہاریہ* rejectBasep = خالی_اشاریہ;  // Reject when a terminal match fails
        Ast_گرہ_اظہاریہ* required_قدم_Rejectp = خالی_اشاریہ;  // Per-source reject from rejectOnFail Links
        Ast_گرہ_اظہاریہ* throughoutRejectp = خالی_اشاریہ;  // Reject when a throughout guard drops
        Ast_گرہ_اظہاریہ* threadFailCountp = خالی_اشاریہ;  // Number of threads rejected on this tick
    };

    // Disable condition also aborts attempts between clock events. A constant disable
    // never rises, and $sampled is unsupported in a sensitivity list.
    ساکن منطقی isAsyncDisable(ثابت Ast_گرہ_اظہاریہ* ثابت disableExprp) {
        واپس disableExprp && !VN_IS(disableExprp, Const)
               && !disableExprp->exists([](ثابت AstSampled*) { واپس سچ; });
    }

    // Emit the clocked update block of registers that hold pending attempts. The clearp
    // statements reset the same registers: a disable rising between clock events also
    // aborts pending attempts (IEEE 1800-2023 16.12), so clearp runs on that rising edge
    // in a separate block. It must clear unconditionally, as the disable may already be
    // false again when it runs, and the update block must not run on that edge, as it
    // would advance the attempts as if the clock had ticked. Blocks that hold no attempt
    // state, such as the $assertkill acknowledge in emitKillAckNba, do not go through here.
    // Callers pass a null clearp when the disable cannot rise between clock events.
    خالی جوڑو_State_Always(زیریں_Ctx& c, Ast_گرہ* bodyp, Ast_گرہ* clearp) {
        m_modp->جوڑو_Stmtsp(
            نیا AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_درخت(جھوٹ), bodyp});
        اگر (!clearp) واپس;
        m_modp->جوڑو_Stmtsp(نیا AstAlways{
            c.flp, VAlwaysKwd::ALWAYS,
            نیا Ast_Sen_درخت{c.flp, نیا Ast_Sen_شے{c.flp, VEdgeType::ET_POSEDGE,
                                                 c.disableExprp->clone_درخت_خالص(جھوٹ)}},
            clearp});
    }

    // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
    // Phase 2: State register NBA always block. Each clocked-edge target
    // latches the OR of its incoming contributions.
    خالی emit_State_رجسٹر_Nba(زیریں_Ctx& c) {
        ثابت منطقی asyncDisable = isAsyncDisable(c.disableExprp);
        Ast_گرہ* bodyp = خالی_اشاریہ;
        Ast_گرہ* clearp = خالی_اشاریہ;
        برائے (ثابت V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
            ثابت Sva_State_راس* ثابت vtxp = static_cast<ثابت Sva_State_راس*>(&vtxr);
            اگر (!vtxp->datap()->stateVarp) جاری;

            Ast_گرہ_اظہاریہ* اگلا_Statep = خالی_اشاریہ;
            برائے (ثابت V3_گراف_کنارہ& edger : vtxp->اندر_کنارے()) {
                ثابت Sva_Trans_کنارہ& tedger = static_cast<ثابت Sva_Trans_کنارہ&>(edger);
                اگر (!tedger.m_consumes_چکر) جاری;
                ثابت Sva_State_راس* ثابت fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Clocked-edge source missing stateSig");

                Ast_گرہ_اظہاریہ* srcSigp = fromVtxp->datap()->stateSigp->clone_درخت_خالص(جھوٹ);
                srcSigp = andCond(c.flp, srcSigp, tedger.m_condp);

                اگر (c.disableExprp) {
                    Ast_گرہ_اظہاریہ* ثابت notDisp
                        = نیا AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(جھوٹ)};
                    srcSigp = نیا AstLogAnd{c.flp, srcSigp, notDisp};
                }
                اگلا_Statep = orExprs(c.flp, اگلا_Statep, srcSigp);
            }

            UASSERT_OBJ(اگلا_Statep, vtxp,
                        "Registered vertex has no clocked incoming contribution");
            اگلا_Statep = gateNotKill(c, اگلا_Statep);

            Ast_تفویض_Dly* ثابت assignp = نیا Ast_تفویض_Dly{
                c.flp, نیا AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE}, اگلا_Statep};
            bodyp = Ast_گرہ::جوڑو_اگلا_Null(bodyp, assignp);
            اگر (asyncDisable) {
                clearp = Ast_گرہ::جوڑو_اگلا_Null(
                    clearp,
                    نیا Ast_تفویض_Dly{
                        c.flp, نیا AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE},
                        نیا AstConst{c.flp, AstConst::ثنائی_ہندسہ_False{}}});
            }
        }
        اگر (bodyp) جوڑو_State_Always(c, bodyp, clearp);
    }

    // Phase 2b: In-place ring storage and registered ring metadata.
    خالی emitDelayRingUpdates(زیریں_Ctx& c, ثابت منطقی گنتی_Matches) {
        ثابت منطقی asyncDisable = isAsyncDisable(c.disableExprp);
        برائے (V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
            Sva_State_راس* ثابت vtxp = static_cast<Sva_State_راس*>(&vtxr);
            اگر (!vtxp->datap()->delayRingVarp) جاری;
            AstVar* ثابت idxp = vtxp->datap()->delayRingIdxVarp;
            AstVar* ثابت زندہ_گنتی_Varp = vtxp->datap()->delay_Ring_زندہ_گنتی_Varp;
            AstVar* ثابت wrappedp = vtxp->datap()->delayRingWrappedVarp;
            ثابت uint32_t سائز = static_cast<uint32_t>(vtxp->m_delay_Ring_سائز);

            Ast_گرہ_اظہاریہ* incomingp = خالی_اشاریہ;
            Ast_گرہ_اظہاریہ* incomingOverflowp = خالی_اشاریہ;
            برائے (ثابت V3_گراف_کنارہ& edger : vtxp->اندر_کنارے()) {
                ++m_stat_Delay_Ring_کنارہ_Visits;
                ثابت Sva_Trans_کنارہ& tedger = static_cast<ثابت Sva_Trans_کنارہ&>(edger);
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
                          ? fromVtxp->datap()->state_گنتی_Sigp->clone_درخت_خالص(جھوٹ)
                          : نیا AstExtend{c.flp,
                                          fromVtxp->datap()->stateSigp->clone_درخت_خالص(جھوٹ),
                                          m_u32DTypep->چوڑائی()};
                Ast_گرہ_اظہاریہ* contribOverflowp
                    = گنتی_Matches && fromVtxp->datap()->state_گنتی_حد_سے_تجاوز_Sigp
                          ? fromVtxp->datap()->state_گنتی_حد_سے_تجاوز_Sigp->clone_درخت_خالص(جھوٹ)
                          : خالی_اشاریہ;
                اگر (tedger.m_condp) {
                    Ast_گرہ_اظہاریہ* ثابت enablep = tedger.m_condp->clone_درخت_خالص(جھوٹ);
                    contribp = گنتی_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, enablep)
                                   : جوڑو_Countp(c.flp, خالی_اشاریہ, contribp, enablep);
                }
                اگر (c.disableExprp) {
                    Ast_گرہ_اظہاریہ* ثابت notDisp
                        = نیا AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(جھوٹ)};
                    contribp = گنتی_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, notDisp)
                                   : جوڑو_Countp(c.flp, خالی_اشاریہ, contribp, notDisp);
                }
                incomingp = گنتی_Matches ? جوڑو_Checked_Countp(c.flp, incomingp, contribp,
                                                            incomingOverflowp, contribOverflowp)
                                         : جوڑو_Countp(c.flp, incomingp, contribp);
            }
            UASSERT_OBJ(incomingp, vtxp, "Delay ring has no incoming edge");
            ثابت منطقی storesCounts = vtxp->datap()->delayRingCounts;
            Ast_گرہ_اظہاریہ* ثابت incomingCountp
                = storesCounts
                      ? incomingp->clone_درخت_خالص(جھوٹ)
                      : نیا AstExtend{c.flp,
                                      نیا AstNeq{c.flp, incomingp->clone_درخت_خالص(جھوٹ),
                                                 newTypedConstp(c.flp, incomingp->dtypep(), 0)},
                                      m_u32DTypep->چوڑائی()};
            Ast_گرہ_اظہاریہ* ثابت storedIncomingp
                = storesCounts ? incomingp
                               : نیا AstNeq{c.flp, incomingp,
                                            newTypedConstp(c.flp, incomingp->dtypep(), 0)};
            // Only this block accesses the storage. Other processes read the cached head, so
            // the ring can be updated in place without an NBA shadow copy.
            Ast_تفویض* ثابت writeIncomingp = نیا Ast_تفویض{
                c.flp,
                delay_Ring_خانہ(c.flp, vtxp, نیا AstVarRef{c.flp, idxp, VAccess::READ},
                              VAccess::WRITE),
                storedIncomingp};
            Ast_گرہ* updateBodyp = writeIncomingp;
            // live_count <= live_count - outgoing_count + incoming_count;
            Ast_گرہ_اظہاریہ* ثابت outgoingp = delay_Ring_Output_گنتی(c.flp, vtxp);
            Ast_گرہ_اظہاریہ* ثابت afterOutgoingp
                = نیا AstSub{c.flp, نیا AstVarRef{c.flp, زندہ_گنتی_Varp, VAccess::READ}, outgoingp};
            Ast_گرہ_اظہاریہ* ثابت oldAfterOutgoingp = afterOutgoingp->clone_درخت_خالص(جھوٹ);
            Ast_جوڑو* ثابت اگلا_زندہ_Countp = نیا Ast_جوڑو{c.flp, afterOutgoingp, incomingCountp};
            اگر (storesCounts) {
                Ast_گرہ_اظہاریہ* ثابت زندہ_Overflowp
                    = نیا AstLt{c.flp, اگلا_زندہ_Countp->clone_درخت_خالص(جھوٹ), oldAfterOutgoingp};
                Ast_گرہ_اظہاریہ* ثابت overflowp = orExprs(c.flp, incomingOverflowp, زندہ_Overflowp);
                updateBodyp = new_گنتی_حد_سے_تجاوز_Warning(c.flp, overflowp);
                updateBodyp->جوڑو_اگلا(writeIncomingp);
            } ورنہ {
                VL_DO_DANGLING(oldAfterOutgoingp->delete_درخت(), oldAfterOutgoingp);
            }
            // Cache the outgoing slot for the next index after the write. Reading after
            // writing also handles one-slot rings and two-slot range rings without alias cases.
            Ast_گرہ_اظہاریہ* اگلا_سر_Idxp
                = اگلا_Ring_اشاریہ(c.flp, نیا AstVarRef{c.flp, idxp, VAccess::READ}, سائز);
            اگر (!vtxp->m_isFixedDelayRing) {
                اگلا_سر_Idxp = اگلا_Ring_اشاریہ(c.flp, اگلا_سر_Idxp, سائز);
            }
            updateBodyp->جوڑو_اگلا(نیا Ast_تفویض_Dly{
                c.flp, نیا AstVarRef{c.flp, vtxp->datap()->delay_Ring_سر_Varp, VAccess::WRITE},
                delay_Ring_خانہ(c.flp, vtxp, اگلا_سر_Idxp)});
            updateBodyp->جوڑو_اگلا(نیا Ast_تفویض_Dly{
                c.flp, نیا AstVarRef{c.flp, زندہ_گنتی_Varp, VAccess::WRITE}, اگلا_زندہ_Countp});
            // wrapped <= wrapped || idx == size - 1;
            updateBodyp->جوڑو_اگلا(
                نیا Ast_تفویض_Dly{c.flp, نیا AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                 نیا AstLogOr{c.flp, نیا AstVarRef{c.flp, wrappedp, VAccess::READ},
                                              delay_Ring_At_آخری_اشاریہ(c.flp, idxp, سائز)}});
            // idx <= next_idx;
            updateBodyp->جوڑو_اگلا(نیا Ast_تفویض_Dly{
                c.flp, نیا AstVarRef{c.flp, idxp, VAccess::WRITE},
                اگلا_Ring_اشاریہ(c.flp, نیا AstVarRef{c.flp, idxp, VAccess::READ}, سائز)});
            اگر (vtxp->m_delayRingAdvanceCondp) {
                updateBodyp = نیا AstIf{
                    c.flp, sampled(vtxp->m_delayRingAdvanceCondp->clone_درخت_خالص(جھوٹ)),
                    updateBodyp};
            }

            Ast_گرہ_اظہاریہ* clearCondp = kill_فعال(c);
            اگر (vtxp->m_delayRingClearCondp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_delayRingClearCondp->clone_درخت_خالص(جھوٹ)));
            }
            اگر (vtxp->m_abortClearp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_abortClearp->clone_درخت_خالص(جھوٹ)));
            }
            اگر (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_درخت_خالص(جھوٹ));
            }
            Ast_گرہ_اظہاریہ* guardp = خالی_اشاریہ;
            برائے (Ast_گرہ_اظہاریہ* ثابت cp : vtxp->m_throughoutConds) {
                Ast_گرہ_اظہاریہ* ثابت sampledp = sampled(cp->clone_درخت_خالص(جھوٹ));
                guardp = guardp ? static_cast<Ast_گرہ_اظہاریہ*>(نیا AstLogAnd{c.flp, guardp, sampledp})
                                : sampledp;
            }
            اگر (guardp) clearCondp = orExprs(c.flp, clearCondp, نیا AstLogNot{c.flp, guardp});
            // Logically clear the ring without touching its wide storage.
            Ast_تفویض_Dly* ثابت clearCountp
                = نیا Ast_تفویض_Dly{c.flp, نیا AstVarRef{c.flp, زندہ_گنتی_Varp, VAccess::WRITE},
                                   newTypedConstp(c.flp, زندہ_گنتی_Varp->dtypep(), 0)};
            clearCountp->جوڑو_اگلا(نیا Ast_تفویض_Dly{c.flp,
                                                  نیا AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                                  نیا AstConst{c.flp, AstConst::ثنائی_ہندسہ_False{}}});
            clearCountp->جوڑو_اگلا(نیا Ast_تفویض_Dly{c.flp,
                                                  نیا AstVarRef{c.flp, idxp, VAccess::WRITE},
                                                  newTypedConstp(c.flp, idxp->dtypep(), 0)});
            updateBodyp = نیا AstIf{c.flp, clearCondp, clearCountp, updateBodyp};

            جوڑو_State_Always(c, updateBodyp, asyncDisable ? clearCountp->clone_درخت(سچ) : خالی_اشاریہ);
        }
    }

    // Phase 2c: SAnd combiner done-latch always block.
    // NBA semantics ensure doneL/doneR read pre-update values (IEEE 16.9.5).
    خالی emit_And_Combiner_مکمل_Latch_Nba(زیریں_Ctx& c) {
        ثابت منطقی asyncDisable = isAsyncDisable(c.disableExprp);
        برائے (ثابت V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
            ثابت Sva_State_راس* ثابت avp = static_cast<ثابت Sva_State_راس*>(&vtxr);
            اگر (!avp->datap()->مکمل_LVarp) جاری;
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

            Ast_تفویض_Dly* ثابت clearLp = نیا Ast_تفویض_Dly{
                c.flp, نیا AstVarRef{c.flp, avp->datap()->مکمل_LVarp, VAccess::WRITE},
                نیا AstConst{c.flp, AstConst::ثنائی_ہندسہ_False{}}};
            Ast_تفویض_Dly* ثابت clearRp = نیا Ast_تفویض_Dly{
                c.flp, نیا AstVarRef{c.flp, avp->datap()->مکمل_RVarp, VAccess::WRITE},
                نیا AstConst{c.flp, AstConst::ثنائی_ہندسہ_False{}}};
            clearLp->جوڑو_اگلا(clearRp);

            Ast_گرہ_اظہاریہ* ثابت مطابقت_LNowp
                = build_مطابقت_Now(c.flp, lhsTermp->datap()->stateSigp, avp->m_andLhsCondp);
            Ast_گرہ_اظہاریہ* ثابت مطابقت_RNowp
                = build_مطابقت_Now(c.flp, rhsTermp->datap()->stateSigp, avp->m_andRhsCondp);
            Ast_گرہ_اظہاریہ* gateLp = مطابقت_LNowp;
            Ast_گرہ_اظہاریہ* gateRp = مطابقت_RNowp;
            اگر (c.disableExprp) {
                Ast_گرہ_اظہاریہ* ثابت notDisLp
                    = نیا AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(جھوٹ)};
                gateLp = نیا AstLogAnd{c.flp, gateLp, notDisLp};
                Ast_گرہ_اظہاریہ* ثابت notDisRp
                    = نیا AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(جھوٹ)};
                gateRp = نیا AstLogAnd{c.flp, gateRp, notDisRp};
            }
            Ast_تفویض_Dly* ثابت سیٹ_Lp = نیا Ast_تفویض_Dly{
                c.flp, نیا AstVarRef{c.flp, avp->datap()->مکمل_LVarp, VAccess::WRITE},
                نیا AstConst{c.flp, AstConst::ثنائی_ہندسہ_True{}}};
            AstIf* ثابت سیٹ_LIfp = نیا AstIf{c.flp, gateLp, سیٹ_Lp, خالی_اشاریہ};
            Ast_تفویض_Dly* ثابت سیٹ_Rp = نیا Ast_تفویض_Dly{
                c.flp, نیا AstVarRef{c.flp, avp->datap()->مکمل_RVarp, VAccess::WRITE},
                نیا AstConst{c.flp, AstConst::ثنائی_ہندسہ_True{}}};
            AstIf* ثابت سیٹ_RIfp = نیا AstIf{c.flp, gateRp, سیٹ_Rp, خالی_اشاریہ};
            سیٹ_LIfp->جوڑو_اگلا(سیٹ_RIfp);

            Ast_گرہ_اظہاریہ* clearCondp = نیا AstLogOr{c.flp, kill_فعال(c),
                                                   avp->datap()->stateSigp->clone_درخت_خالص(جھوٹ)};
            اگر (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_درخت_خالص(جھوٹ));
            }
            AstIf* ثابت topp = نیا AstIf{c.flp, clearCondp, clearLp, سیٹ_LIfp};
            جوڑو_State_Always(c, topp, asyncDisable ? clearLp->clone_درخت(سچ) : خالی_اشاریہ);
        }
    }

    خالی emitKillAckNba(زیریں_Ctx& c) {
        Ast_تفویض_Dly* ثابت ackp
            = نیا Ast_تفویض_Dly{c.flp, نیا AstVarRef{c.flp, c.killVarp, VAccess::WRITE},
                               assertKillGet(c.flp, c.assertType, c.directiveType)};
        m_modp->جوڑو_Stmtsp(
            نیا AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_درخت(جھوٹ), ackp});
    }

    // Phase 3/3a/3b: Compute terminal match/reject signals, required-step reject,
    // throughout-drop reject; clean up intermediate state signals.
    // Phase 3: terminalActive and rejectBase from Links to matchVertex.
    // Builder only adds Links (non-clocked) to matchVertex via addLink in
    // wireMatchAndMidSources. For cover sequence, also count each end-of-match
    // so the action can be replayed without unrolling endpoints.
    خالی compute_Terminal_مطابقت_And_Reject(زیریں_Ctx& c, Signal_سیٹ& sigs, ثابت منطقی need_مطابقت_گنتی) {
        برائے (ثابت Sva_Trans_کنارہ* ثابت tedgep : c.گراف.all_کنارے()) {
            اگر (tedgep->مقصد_Vtxp() != c.گراف.m_مطابقت_Vertexp) جاری;
            Sva_State_راس* ثابت fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                        "Terminal-link source missing stateSig");

            Ast_گرہ_اظہاریہ* srcSigp = fromVtxp->datap()->stateSigp->clone_درخت_خالص(جھوٹ);
            srcSigp = andCond(c.flp, srcSigp, tedgep->m_condp);
            اگر (need_مطابقت_گنتی) {
                UASSERT_OBJ(fromVtxp->datap()->state_گنتی_Sigp, fromVtxp,
                            "Terminal-link source missing stateCountSig");
                Ast_گرہ_اظہاریہ* contributionp
                    = fromVtxp->datap()->state_گنتی_Sigp->clone_درخت_خالص(جھوٹ);
                Ast_گرہ_اظہاریہ* contributionOverflowp
                    = fromVtxp->datap()->state_گنتی_حد_سے_تجاوز_Sigp
                          ? fromVtxp->datap()->state_گنتی_حد_سے_تجاوز_Sigp->clone_درخت_خالص(جھوٹ)
                          : خالی_اشاریہ;
                اگر (tedgep->m_condp) {
                    contributionp = gateCheckedCountp(c.flp, contributionp, contributionOverflowp,
                                                      tedgep->m_condp->clone_درخت_خالص(جھوٹ));
                }
                sigs.مطابقت_Countp
                    = جوڑو_Checked_Countp(c.flp, sigs.مطابقت_Countp, contributionp,
                                       sigs.مطابقت_گنتی_Overflowp, contributionOverflowp);
            }

            اگر (fromVtxp->m_delay_Ring_سائز && !fromVtxp->m_isFixedDelayRing) {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_درخت_خالص(جھوٹ));
                // reject |= (wrapped || idx == size - 1) && head && final_condition;
                Ast_گرہ_اظہاریہ* expireContribp = delay_Ring_Output_گنتی(c.flp, fromVtxp);
                expireContribp = andCond(c.flp, expireContribp, tedgep->m_condp);
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, expireContribp);
                VL_DO_DANGLING(srcSigp->delete_درخت(), srcSigp);
            } ورنہ اگر (fromVtxp->m_isUnbounded || fromVtxp->m_isAndCombiner) {
                sigs.terminalActivep = orExprs(c.flp, sigs.terminalActivep, srcSigp);
            } ورنہ {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_درخت_خالص(جھوٹ));
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, srcSigp);
            }
        }
        // wireMatchAndMidSources always adds a Link from result.termVertexp
        // to m_matchVertexp, so the loop above always sets terminalActivep.
        UASSERT_OBJ(sigs.terminalActivep, c.گراف.m_مطابقت_Vertexp,
                    "No terminal edge to match vertex");
    }

    Ast_گرہ_اظہاریہ* newThroughoutThreadFailCountp(زیریں_Ctx& c, AstVar* ثابت delay_Ring_زندہ_گنتی_Varp,
                                               Ast_گرہ_اظہاریہ* ثابت stateExprp,
                                               Ast_گرہ_اظہاریہ* ثابت enablep) {
        Ast_گرہ_اظہاریہ* ثابت فعال_Thread_Countp
            = delay_Ring_زندہ_گنتی_Varp
                  ? static_cast<Ast_گرہ_اظہاریہ*>(
                        نیا AstVarRef{c.flp, delay_Ring_زندہ_گنتی_Varp, VAccess::READ})
                  : نیا AstExtend{c.flp, stateExprp->clone_درخت_خالص(جھوٹ), m_u32DTypep->چوڑائی()};
        واپس جوڑو_Countp(c.flp, خالی_اشاریہ, فعال_Thread_Countp, enablep);
    }

    // Phase 3b: Throughout-drop and ring-wide abort rejection.
    خالی computeThroughoutReject(زیریں_Ctx& c, Signal_سیٹ& sigs, ثابت منطقی need_Thread_Fail_گنتی) {
        برائے (ثابت V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
            ثابت Sva_State_راس* ثابت vtxp = static_cast<ثابت Sva_State_راس*>(&vtxr);
            ثابت خودکار& conds = vtxp->m_throughoutConds;
            اگر (conds.خالی_خانے() && !vtxp->m_abortRejectp) جاری;
            اگر (vtxp->m_isAndCombiner) جاری;
            Ast_گرہ_اظہاریہ* stateExprp = خالی_اشاریہ;
            اگر (vtxp->datap()->stateVarp) {
                stateExprp = نیا AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
            } ورنہ اگر (vtxp->datap()->delayRingVarp && vtxp->m_isFixedDelayRing) {
                stateExprp
                    = delay_Ring_Has_زندہ_Threadsp(c.flp, vtxp->datap()->delay_Ring_زندہ_گنتی_Varp);
            } ورنہ {
                UASSERT_OBJ(vtxp->datap()->stateSigp, vtxp,
                            "Throughout-conds vertex missing state representation");
                stateExprp = vtxp->datap()->stateSigp->clone_درخت_خالص(جھوٹ);
            }
            Ast_گرہ_اظہاریہ* guardp = خالی_اشاریہ;
            برائے (Ast_گرہ_اظہاریہ* ثابت cp : conds) {
                Ast_گرہ_اظہاریہ* ثابت sp = sampled(cp->clone_درخت_خالص(جھوٹ));
                guardp = guardp ? static_cast<Ast_گرہ_اظہاریہ*>(نیا AstLogAnd{c.flp, guardp, sp}) : sp;
            }
            اگر (vtxp->m_abortRejectp) {
                Ast_گرہ_اظہاریہ* ثابت notAbortp
                    = نیا AstLogNot{c.flp, sampled(vtxp->m_abortRejectp->clone_درخت_خالص(جھوٹ))};
                guardp = guardp
                             ? static_cast<Ast_گرہ_اظہاریہ*>(نیا AstLogAnd{c.flp, guardp, notAbortp})
                             : notAbortp;
            }
            Ast_گرہ_اظہاریہ* rejectCondp = نیا AstLogNot{c.flp, guardp};
            اگر (vtxp->m_abortClearp) {
                // Any abort clears the ring. Accept aborts suppress a simultaneous guard failure;
                // reject aborts are restored below so they still force rejection.
                Ast_گرہ_اظہاریہ* ثابت notAbortClearp
                    = نیا AstLogNot{c.flp, sampled(vtxp->m_abortClearp->clone_درخت_خالص(جھوٹ))};
                rejectCondp = نیا AstLogAnd{c.flp, rejectCondp, notAbortClearp};
                اگر (vtxp->m_abortRejectp) {
                    rejectCondp = نیا AstLogOr{
                        c.flp, rejectCondp, sampled(vtxp->m_abortRejectp->clone_درخت_خالص(جھوٹ))};
                }
            }
            اگر (need_Thread_Fail_گنتی) {
                Ast_گرہ_اظہاریہ* ثابت contributionp
                    = newThroughoutThreadFailCountp(c, vtxp->datap()->delay_Ring_زندہ_گنتی_Varp,
                                                    stateExprp, rejectCondp->clone_درخت_خالص(جھوٹ));
                sigs.threadFailCountp = جوڑو_Countp(c.flp, sigs.threadFailCountp, contributionp);
                اگر (vtxp->m_abortRejectp && vtxp->m_delayRingAdvanceCondp) {
                    // An advancing ring thread also occupies its same-tick match vertex in the
                    // unrolled NFA, so abort rejection must replay both fail actions.
                    Ast_گرہ_اظہاریہ* ثابت abortAndAdvancep = نیا AstLogAnd{
                        c.flp, sampled(vtxp->m_abortRejectp->clone_درخت_خالص(جھوٹ)),
                        sampled(vtxp->m_delayRingAdvanceCondp->clone_درخت_خالص(جھوٹ))};
                    Ast_گرہ_اظہاریہ* ثابت مطابقت_Contributionp = newThroughoutThreadFailCountp(
                        c, vtxp->datap()->delay_Ring_زندہ_گنتی_Varp, stateExprp, abortAndAdvancep);
                    sigs.threadFailCountp
                        = جوڑو_Countp(c.flp, sigs.threadFailCountp, مطابقت_Contributionp);
                }
            }
            sigs.throughoutRejectp = orExprs(c.flp, sigs.throughoutRejectp,
                                             نیا AstLogAnd{c.flp, stateExprp, rejectCondp});
        }
    }

    Signal_سیٹ compute_اشارات(زیریں_Ctx& c, ثابت منطقی need_Thread_Fail_گنتی,
                             ثابت منطقی need_Throughout_Thread_Fail_گنتی, ثابت منطقی need_مطابقت_گنتی) {
        Signal_سیٹ sigs;

        compute_Terminal_مطابقت_And_Reject(c, sigs, need_مطابقت_گنتی);

        // Phase 3a: required-step rejection.
        // Builder only sets m_rejectOnFail on non-clocked Links with m_condp
        // or m_condVtxp, and the source always has a resolved stateSig.
        برائے (ثابت Sva_Trans_کنارہ* ثابت tedgep : c.گراف.all_کنارے()) {
            اگر (!tedgep->m_rejectOnFail) جاری;
            ثابت Sva_State_راس* ثابت fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp && (tedgep->m_condp || tedgep->m_condVtxp),
                        fromVtxp,
                        "rejectOnFail Link must have condp/condVtxp and source stateSig");
            Ast_گرہ_اظہاریہ* ثابت srcSigp = fromVtxp->datap()->stateSigp->clone_درخت_خالص(جھوٹ);
            Ast_گرہ_اظہاریہ* condp = خالی_اشاریہ;
            اگر (tedgep->m_condVtxp) {
                UASSERT_OBJ(tedgep->m_condVtxp->datap()->stateSigp, tedgep->m_condVtxp,
                            "rejectOnFail condVtxp missing stateSig");
                condp = tedgep->m_condVtxp->datap()->stateSigp->clone_درخت_خالص(جھوٹ);
                اگر (tedgep->m_condp) {
                    condp = نیا AstLogOr{c.flp, condp, tedgep->m_condp->clone_درخت_خالص(جھوٹ)};
                }
            } ورنہ {
                condp = tedgep->m_condp->clone_درخت_خالص(جھوٹ);
            }
            Ast_گرہ_اظہاریہ* ثابت notCondp = نیا AstLogNot{c.flp, condp};
            Ast_گرہ_اظہاریہ* ثابت rawFailp = نیا AstLogAnd{c.flp, srcSigp, notCondp};
            اگر (need_Thread_Fail_گنتی) {
                // thread_fail_count += fail;
                sigs.threadFailCountp = جوڑو_Countp(
                    c.flp, sigs.threadFailCountp,
                    نیا AstExtend{c.flp, rawFailp->clone_درخت_خالص(جھوٹ), m_u32DTypep->چوڑائی()});
            }
            Ast_گرہ_اظہاریہ* ثابت failp = gateNotKill(c, rawFailp);
            sigs.required_قدم_Rejectp = orExprs(c.flp, sigs.required_قدم_Rejectp, failp);
        }

        computeThroughoutReject(c, sigs, need_Throughout_Thread_Fail_گنتی);
        اگر (sigs.threadFailCountp) {
            sigs.threadFailCountp
                = جوڑو_Countp(c.flp, خالی_اشاریہ, sigs.threadFailCountp, not_Kill_فعال(c));
        }
        اگر (sigs.مطابقت_Countp) {
            اگر (c.مطابقت_Condp) {
                sigs.مطابقت_Countp
                    = gateCheckedCountp(c.flp, sigs.مطابقت_Countp, sigs.مطابقت_گنتی_Overflowp,
                                        sampled(c.مطابقت_Condp->clone_درخت_خالص(جھوٹ)));
            }
            sigs.مطابقت_Countp = gateCheckedCountp(c.flp, sigs.مطابقت_Countp, sigs.مطابقت_گنتی_Overflowp,
                                                 not_Kill_فعال(c));
        }
        sigs.terminalActivep = gateNotKill(c, sigs.terminalActivep);
        sigs.rejectBasep = gateNotKill(c, sigs.rejectBasep);
        sigs.throughoutRejectp = gateNotKill(c, sigs.throughoutRejectp);

        // Clean up intermediate state signals. These are orphan subtrees
        // (never linked into the enclosing AST); deleteTree() is immediate
        // which is what we want since stateSigp lifetime ends with this scope.
        برائے (V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
            Sva_State_راس* ثابت vtxp = static_cast<Sva_State_راس*>(&vtxr);
            Ast_گرہ_اظہاریہ*& sigp = vtxp->datap()->stateSigp;
            اگر (sigp) VL_DO_DANGLING(sigp->delete_درخت(), sigp);
            Ast_گرہ_اظہاریہ*& countp = vtxp->datap()->state_گنتی_Sigp;
            اگر (countp) VL_DO_DANGLING(countp->delete_درخت(), countp);
            Ast_گرہ_اظہاریہ*& overflowp = vtxp->datap()->state_گنتی_حد_سے_تجاوز_Sigp;
            اگر (overflowp) VL_DO_DANGLING(overflowp->delete_درخت(), overflowp);
        }
        // Disable iff gating (IEEE 1800-2023 16.12) on the current level value.
        اگر (c.disableExprp) {
            // terminalActivep is always set, so gate it unconditionally.
            Ast_گرہ_اظہاریہ* ثابت notTermp
                = نیا AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(جھوٹ)};
            sigs.terminalActivep = نیا AstLogAnd{c.flp, sigs.terminalActivep, notTermp};
            اگر (sigs.rejectBasep) {
                Ast_گرہ_اظہاریہ* ثابت notDisp
                    = نیا AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(جھوٹ)};
                sigs.rejectBasep = نیا AstLogAnd{c.flp, sigs.rejectBasep, notDisp};
            }
            اگر (sigs.throughoutRejectp) {
                Ast_گرہ_اظہاریہ* ثابت notDisp
                    = نیا AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(جھوٹ)};
                sigs.throughoutRejectp = نیا AstLogAnd{c.flp, sigs.throughoutRejectp, notDisp};
            }
            اگر (sigs.required_قدم_Rejectp) {
                Ast_گرہ_اظہاریہ* ثابت notDisp
                    = نیا AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(جھوٹ)};
                sigs.required_قدم_Rejectp = نیا AstLogAnd{c.flp, sigs.required_قدم_Rejectp, notDisp};
            }
            اگر (sigs.مطابقت_Countp) {
                sigs.مطابقت_Countp = gateCheckedCountp(
                    c.flp, sigs.مطابقت_Countp, sigs.مطابقت_گنتی_Overflowp,
                    نیا AstLogNot{c.flp, c.disableExprp->clone_درخت_خالص(جھوٹ)});
            }
        }

        اگر (sigs.مطابقت_گنتی_Overflowp) {
            m_modp->جوڑو_Stmtsp(
                نیا AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_درخت(جھوٹ),
                              new_گنتی_حد_سے_تجاوز_Warning(c.flp, sigs.مطابقت_گنتی_Overflowp)});
            sigs.مطابقت_گنتی_Overflowp = خالی_اشاریہ;
        }

        واپس sigs;
    }

    // Phase 1: Resolve combinational Links via fixed-point propagation.
    خالی resolveLinks(زیریں_Ctx& c, Ast_گرہ_اظہاریہ* triggerExprp, ثابت منطقی گنتی_Matches) {
        // datap() was freshly allocated in lower() -- all state signals start null.
        c.گراف.m_آغاز_Vertexp->datap()->stateSigp = triggerExprp->clone_درخت_خالص(جھوٹ);
        c.گراف.m_آغاز_Vertexp->datap()->state_گنتی_Sigp
            = نیا AstExtend{c.flp, triggerExprp->clone_درخت_خالص(جھوٹ), m_u32DTypep->چوڑائی()};
        برائے (V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
            Sva_State_راس* ثابت vtxp = static_cast<Sva_State_راس*>(&vtxr);
            اگر (vtxp->datap()->stateVarp) {
                vtxp->datap()->stateSigp
                    = نیا AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
                vtxp->datap()->state_گنتی_Sigp = نیا AstExtend{
                    c.flp, نیا AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ},
                    m_u32DTypep->چوڑائی()};
            } ورنہ اگر (vtxp->datap()->delayRingVarp) {
                اگر (vtxp->m_isFixedDelayRing) {
                    // state_count = wrapped ? head : 0; state = state_count != 0;
                    Ast_گرہ_اظہاریہ* ثابت countp = delay_Ring_Output_گنتی(c.flp, vtxp);
                    vtxp->datap()->stateSigp = نیا AstNeq{c.flp, countp->clone_درخت_خالص(جھوٹ),
                                                          newTypedConstp(c.flp, m_u32DTypep, 0)};
                    vtxp->datap()->state_گنتی_Sigp = countp;
                } ورنہ {
                    vtxp->datap()->stateSigp
                        = delay_Ring_Has_زندہ_Threadsp(c.flp, vtxp->datap()->delay_Ring_زندہ_گنتی_Varp);
                    vtxp->datap()->state_گنتی_Sigp = نیا AstVarRef{
                        c.flp, vtxp->datap()->delay_Ring_زندہ_گنتی_Varp, VAccess::READ};
                }
            }
        }
        // Fixed-point propagation along zero-delay (Link) edges. Rebuild each
        // derived signal from its incoming edges on every pass; appending the same
        // contributions repeatedly makes merge expressions grow exponentially.
        ثابت سائز_t passes = 2 * c.گراف.m_گراف.vertices().سائز() + 2;
        برائے (سائز_t pass = 0; pass < passes; ++pass) {
            // Rebuild SAnd combiners once both sub-NFA terminals are available.
            برائے (V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
                Sva_State_راس* ثابت vtxp = static_cast<Sva_State_راس*>(&vtxr);
                اگر (!vtxp->m_isAndCombiner) جاری;
                // AndCombiner vertices always have both terminal pointers set.
                UASSERT_OBJ(vtxp->m_andLhsTermp && vtxp->m_andRhsTermp, vtxp,
                            "AndCombiner vertex missing LHS/RHS terminal");
                ثابت Sva_State_راس* ثابت lhsTermp = vtxp->m_andLhsTermp;
                ثابت Sva_State_راس* ثابت rhsTermp = vtxp->m_andRhsTermp;
                اگر (!lhsTermp->datap()->stateSigp || !rhsTermp->datap()->stateSigp) جاری;
                Ast_گرہ_اظہاریہ* ثابت مطابقت_Lp
                    = build_مطابقت_Now(c.flp, lhsTermp->datap()->stateSigp, vtxp->m_andLhsCondp);
                Ast_گرہ_اظہاریہ* ثابت مطابقت_Rp
                    = build_مطابقت_Now(c.flp, rhsTermp->datap()->stateSigp, vtxp->m_andRhsCondp);
                Ast_گرہ_اظہاریہ* ثابت مکمل_LOrp = نیا AstLogOr{
                    c.flp, نیا AstVarRef{c.flp, vtxp->datap()->مکمل_LVarp, VAccess::READ}, مطابقت_Lp};
                Ast_گرہ_اظہاریہ* ثابت مکمل_ROrp = نیا AstLogOr{
                    c.flp, نیا AstVarRef{c.flp, vtxp->datap()->مکمل_RVarp, VAccess::READ}, مطابقت_Rp};
                Ast_گرہ_اظہاریہ* ثابت bothp = نیا AstLogAnd{c.flp, مکمل_LOrp, مکمل_ROrp};
                Ast_گرہ_اظہاریہ* ثابت oneNowp = نیا AstLogOr{c.flp, مطابقت_Lp->clone_درخت_خالص(جھوٹ),
                                                          مطابقت_Rp->clone_درخت_خالص(جھوٹ)};
                اگر (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_درخت(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = نیا AstLogAnd{c.flp, bothp, oneNowp};
                اگر (vtxp->datap()->state_گنتی_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_گنتی_Sigp->delete_درخت(),
                                   vtxp->datap()->state_گنتی_Sigp);
                }
                vtxp->datap()->state_گنتی_Sigp = نیا AstExtend{
                    c.flp, vtxp->datap()->stateSigp->clone_درخت_خالص(جھوٹ), m_u32DTypep->چوڑائی()};
            }

            برائے (V3_گراف_راس& vtxr : c.گراف.m_گراف.vertices()) {
                Sva_State_راس* ثابت vtxp = static_cast<Sva_State_راس*>(&vtxr);
                اگر (vtxp == c.گراف.m_آغاز_Vertexp || vtxp->datap()->stateVarp
                    || vtxp->datap()->delayRingVarp || vtxp->m_isAndCombiner || vtxp->m_is_مطابقت
                    || vtxp->m_isRejectSink) {
                    جاری;
                }
                Ast_گرہ_اظہاریہ* اگلا_Statep = خالی_اشاریہ;
                Ast_گرہ_اظہاریہ* اگلا_State_Countp = خالی_اشاریہ;
                Ast_گرہ_اظہاریہ* اگلا_State_گنتی_Overflowp = خالی_اشاریہ;
                برائے (ثابت V3_گراف_کنارہ& er : vtxp->اندر_کنارے()) {
                    ثابت Sva_Trans_کنارہ& te = static_cast<ثابت Sva_Trans_کنارہ&>(er);
                    ثابت Sva_State_راس* ثابت fromVtxp = te.fromVtxp();
                    اگر (!fromVtxp->datap()->stateSigp) جاری;
                    UASSERT_OBJ(fromVtxp->datap()->state_گنتی_Sigp, fromVtxp,
                                "Link source missing stateCountSig");
                    Ast_گرہ_اظہاریہ* ثابت contributionp = andCond(
                        c.flp, fromVtxp->datap()->stateSigp->clone_درخت_خالص(جھوٹ), te.m_condp);
                    اگلا_Statep = orExprs(c.flp, اگلا_Statep, contributionp);
                    Ast_گرہ_اظہاریہ* گنتی_Contributionp
                        = fromVtxp->datap()->state_گنتی_Sigp->clone_درخت_خالص(جھوٹ);
                    Ast_گرہ_اظہاریہ* گنتی_Contribution_Overflowp
                        = گنتی_Matches && fromVtxp->datap()->state_گنتی_حد_سے_تجاوز_Sigp
                              ? fromVtxp->datap()->state_گنتی_حد_سے_تجاوز_Sigp->clone_درخت_خالص(جھوٹ)
                              : خالی_اشاریہ;
                    اگر (te.m_condp) {
                        Ast_گرہ_اظہاریہ* ثابت enablep = te.m_condp->clone_درخت_خالص(جھوٹ);
                        گنتی_Contributionp
                            = گنتی_Matches
                                  ? gateCheckedCountp(c.flp, گنتی_Contributionp,
                                                      گنتی_Contribution_Overflowp, enablep)
                                  : جوڑو_Countp(c.flp, خالی_اشاریہ, گنتی_Contributionp, enablep);
                    }
                    اگلا_State_Countp
                        = گنتی_Matches
                              ? جوڑو_Checked_Countp(c.flp, اگلا_State_Countp, گنتی_Contributionp,
                                                 اگلا_State_گنتی_Overflowp,
                                                 گنتی_Contribution_Overflowp)
                              : جوڑو_Countp(c.flp, اگلا_State_Countp, گنتی_Contributionp);
                }
                اگر (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_درخت(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = اگلا_Statep;
                اگر (vtxp->datap()->state_گنتی_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_گنتی_Sigp->delete_درخت(),
                                   vtxp->datap()->state_گنتی_Sigp);
                }
                vtxp->datap()->state_گنتی_Sigp = اگلا_State_Countp;
                اگر (vtxp->datap()->state_گنتی_حد_سے_تجاوز_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_گنتی_حد_سے_تجاوز_Sigp->delete_درخت(),
                                   vtxp->datap()->state_گنتی_حد_سے_تجاوز_Sigp);
                }
                vtxp->datap()->state_گنتی_حد_سے_تجاوز_Sigp = اگلا_State_گنتی_Overflowp;
            }
        }
    }

عوامی:
    // Combine terminal/reject signals into final output expression.
    ساکن Ast_گرہ_اظہاریہ* assemble_نتیجہ(Ast_گرہ_Cover_Or_Assert* ثابت assertp, ثابت منطقی negated,
                                       Ast_گرہ_اظہاریہ* ثابت مطابقت_Condp, ثابت Signal_سیٹ& sigs,
                                       Ast_گرہ_اظہاریہ** ثابت برآمد_Matchpp) {
        File_سطر* ثابت flp = assertp->fileline();
        ثابت منطقی isCover = VN_IS(assertp, Cover);
        Ast_گرہ_اظہاریہ* ثابت terminalActivep = sigs.terminalActivep;
        Ast_گرہ_اظہاریہ* ثابت rejectBasep = sigs.rejectBasep;
        Ast_گرہ_اظہاریہ* ثابت throughoutRejectp = sigs.throughoutRejectp;
        Ast_گرہ_اظہاریہ* ثابت required_قدم_Rejectp = sigs.required_قدم_Rejectp;
        // Property negation (IEEE 1800-2023 16.12.1 `not`): invert match/reject.
        اگر (negated) {
            اگر (isCover) {
                اگر (terminalActivep)
                    VL_DO_DANGLING(terminalActivep->delete_درخت(), terminalActivep);
                Ast_گرہ_اظہاریہ* negRejectp = خالی_اشاریہ;
                اگر (مطابقت_Condp && rejectBasep) {
                    Ast_گرہ_اظہاریہ* ثابت sampledCondp = sampled(مطابقت_Condp->clone_درخت_خالص(جھوٹ));
                    Ast_گرہ_اظہاریہ* ثابت notCondp = نیا AstLogNot{flp, sampledCondp};
                    negRejectp = نیا AstLogAnd{flp, rejectBasep, notCondp};
                } ورنہ اگر (rejectBasep) {
                    VL_DO_DANGLING(rejectBasep->delete_درخت(), rejectBasep);
                }
                اگر (throughoutRejectp) negRejectp = orExprs(flp, negRejectp, throughoutRejectp);
                اگر (required_قدم_Rejectp)
                    negRejectp = orExprs(flp, negRejectp, required_قدم_Rejectp);
                واپس negRejectp ? negRejectp : نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}};
            }
            // Negated assert/assume: output = !match.
            Ast_گرہ_اظہاریہ* matchp = terminalActivep;
            اگر (مطابقت_Condp) {
                Ast_گرہ_اظہاریہ* ثابت sampledCondp = sampled(مطابقت_Condp->clone_درخت_خالص(جھوٹ));
                matchp = نیا AstLogAnd{flp, matchp, sampledCondp};
            }
            اگر (برآمد_Matchpp) {
                Ast_گرہ_اظہاریہ* notPMatchp = خالی_اشاریہ;
                اگر (مطابقت_Condp && rejectBasep) {
                    Ast_گرہ_اظہاریہ* ثابت sampledCondp = sampled(مطابقت_Condp->clone_درخت_خالص(جھوٹ));
                    notPMatchp = نیا AstLogAnd{flp, rejectBasep->clone_درخت_خالص(جھوٹ),
                                               نیا AstLogNot{flp, sampledCondp}};
                } ورنہ اگر (rejectBasep) {
                    notPMatchp = rejectBasep->clone_درخت_خالص(جھوٹ);
                }
                اگر (throughoutRejectp)
                    notPMatchp = orExprs(flp, notPMatchp, throughoutRejectp->clone_درخت_خالص(جھوٹ));
                اگر (required_قدم_Rejectp)
                    notPMatchp
                        = orExprs(flp, notPMatchp, required_قدم_Rejectp->clone_درخت_خالص(جھوٹ));
                *برآمد_Matchpp = notPMatchp;
            }
            اگر (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_درخت(), throughoutRejectp);
            اگر (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_درخت(), rejectBasep);
            اگر (required_قدم_Rejectp)
                VL_DO_DANGLING(required_قدم_Rejectp->delete_درخت(), required_قدم_Rejectp);
            Ast_گرہ_اظہاریہ* ثابت نتیجہ_Exprp = نیا AstLogNot{flp, matchp};
            واپس نتیجہ_Exprp;
        }
        اگر (isCover) {
            اگر (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_درخت(), throughoutRejectp);
            اگر (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_درخت(), rejectBasep);
            اگر (required_قدم_Rejectp)
                VL_DO_DANGLING(required_قدم_Rejectp->delete_درخت(), required_قدم_Rejectp);
            اگر (مطابقت_Condp) {
                Ast_گرہ_اظہاریہ* ثابت sampledCondp = sampled(مطابقت_Condp->clone_درخت_خالص(جھوٹ));
                واپس نیا AstLogAnd{flp, terminalActivep, sampledCondp};
            }
            واپس terminalActivep;
        }
        // Assert/assume: output = !reject
        Ast_گرہ_اظہاریہ* rejectp = خالی_اشاریہ;
        اگر (مطابقت_Condp && rejectBasep) {
            Ast_گرہ_اظہاریہ* ثابت sampledCondp = sampled(مطابقت_Condp->clone_درخت_خالص(جھوٹ));
            rejectp = نیا AstLogAnd{flp, rejectBasep, نیا AstLogNot{flp, sampledCondp}};
        } ورنہ اگر (rejectBasep) {
            VL_DO_DANGLING(rejectBasep->delete_درخت(), rejectBasep);
        }
        اگر (برآمد_Matchpp) {
            Ast_گرہ_اظہاریہ* مطابقت_Exprp = terminalActivep->clone_درخت_خالص(جھوٹ);
            اگر (مطابقت_Condp) {
                Ast_گرہ_اظہاریہ* ثابت sp = sampled(مطابقت_Condp->clone_درخت_خالص(جھوٹ));
                مطابقت_Exprp = نیا AstLogAnd{flp, مطابقت_Exprp, sp};
            }
            *برآمد_Matchpp = مطابقت_Exprp;
        }
        اگر (terminalActivep) VL_DO_DANGLING(terminalActivep->delete_درخت(), terminalActivep);
        اگر (throughoutRejectp) rejectp = orExprs(flp, rejectp, throughoutRejectp);
        اگر (required_قدم_Rejectp) rejectp = orExprs(flp, rejectp, required_قدم_Rejectp);
        اگر (!rejectp) واپس نیا AstConst{flp, AstConst::ثنائی_ہندسہ_True{}};
        Ast_گرہ_اظہاریہ* ثابت نتیجہ_Exprp = نیا AstLogNot{flp, rejectp};
        واپس نتیجہ_Exprp;
    }

    صریح SvaNfaLowering(Ast_گرہ_Module* modp)
        : m_modp{modp}
        , m_u32DTypep{modp->ڈھونڈو_Basic_DType(VBasicDTypeKwd::UINT32)} {}
    ~SvaNfaLowering() {
        V3Stats::جوڑو_Stat_مجموع("Assertions, NFA delay ring edge visits", m_stat_Delay_Ring_کنارہ_Visits);
    }

    // Lower NFA graph to synthesizable AstAlways blocks and raw result signals.
    // Links are combinational; Edges are registered (NBA).
    Signal_سیٹ زیریں(Ast_گرہ_Cover_Or_Assert* ثابت assertp, Sva_گراف& گراف,
                    Ast_Sen_درخت* ثابت senTreep, Ast_گرہ_اظہاریہ* ثابت مطابقت_Condp,
                    Ast_گرہ_اظہاریہ* ثابت disableExprp, ثابت منطقی need_Thread_Fail_گنتی,
                    ثابت منطقی need_Throughout_Thread_Fail_گنتی) {
        File_سطر* ثابت flp = assertp->fileline();
        AstCover* ثابت coverp = VN_CAST(assertp, Cover);
        ثابت منطقی isSeqEvent = coverp && coverp->isSeqEvent();
        ثابت VAssertType assertType
            = isSeqEvent ? VAssertType{VAssertType::اندرونی_2} : assertp->userType();
        ثابت VAssertDirectiveType directiveType
            = isSeqEvent ? VAssertDirectiveType{VAssertDirectiveType::اندرونی_2}
                         : assertp->directive();
        ثابت std::string بنیاد_Name = m_نام.get("");

        // Number vertices with sequential colors for array indexing.
        صحیح N = 0;
        برائے (V3_گراف_راس& vtxr : گراف.m_گراف.vertices()) { vtxr.color(N++); }
        std::سمتیہ<Sva_State_راس*> vtx(N, خالی_اشاریہ);
        برائے (V3_گراف_راس& vtxr : گراف.m_گراف.vertices()) {
            vtx[vtxr.color()] = static_cast<Sva_State_راس*>(&vtxr);
        }

        // Allocate per-vertex lowering data (stored via V3GraphVertex::userp()).
        std::سمتیہ<Sva_راس_ڈیٹا> راس_ڈیٹا(N);
        برائے (صحیح i = 0; i < N; ++i) vtx[i]->userp(&راس_ڈیٹا[i]);

        // Identify registered vertices (targets of clocked edges).
        برائے (صحیح i = 0; i < N; ++i) {
            برائے (ثابت V3_گراف_کنارہ& edger : vtx[i]->برآمد_کنارے()) {
                ثابت Sva_Trans_کنارہ& tedger = static_cast<ثابت Sva_Trans_کنارہ&>(edger);
                ثابت صحیح مقصد_Idx = tedger.مقصد_Vtxp()->color();
                اگر (tedger.m_consumes_چکر && tedger.مقصد_Vtxp() != گراف.m_مطابقت_Vertexp
                    && !tedger.مقصد_Vtxp()->m_isRejectSink) {
                    vtx[مقصد_Idx]->datap()->needsReg = سچ;
                }
            }
        }

        // A packed range ring can emit several live endpoints without needing per-slot counts.
        // Widen only a ring whose input can already carry multiplicity.
        std::سمتیہ<منطقی> stateMayHaveMultiplicity(N);
        std::سمتیہ<منطقی> ringNeedsMultiplicity(N);
        std::سمتیہ<صحیح> multiplicityWork;
        ثابت منطقی گنتی_Matches = coverp && coverp->isCoverSeq() && !isSeqEvent;
        اگر (گنتی_Matches) {
            برائے (صحیح i = 0; i < N; ++i) {
                ثابت منطقی مضرب_Inputs = vtx[i]->اندر_کنارے().سائز() > 1;
                ثابت منطقی isRing = vtx[i]->m_delay_Ring_سائز;
                ringNeedsMultiplicity[i] = isRing && مضرب_Inputs;
                ثابت منطقی startsMultiplicity = isRing
                                                    ? !vtx[i]->m_isFixedDelayRing
                                                    : مضرب_Inputs && !vtx[i]->datap()->needsReg;
                اگر (startsMultiplicity) {
                    stateMayHaveMultiplicity[i] = سچ;
                    multiplicityWork.ڈالو_عقب(i);
                }
            }
            برائے (سائز_t wi = 0; wi < multiplicityWork.سائز(); ++wi) {
                برائے (ثابت V3_گراف_کنارہ& edger : vtx[multiplicityWork[wi]]->برآمد_کنارے()) {
                    ثابت Sva_Trans_کنارہ& tedger = static_cast<ثابت Sva_Trans_کنارہ&>(edger);
                    ثابت صحیح ti = tedger.مقصد_Vtxp()->color();
                    ثابت منطقی isRing = vtx[ti]->m_delay_Ring_سائز;
                    اگر (isRing) ringNeedsMultiplicity[ti] = سچ;
                    // Match and reject sinks have no outgoing edges, so they stop naturally.
                    ثابت منطقی stopsMultiplicity = !isRing && vtx[ti]->datap()->needsReg;
                    اگر (stateMayHaveMultiplicity[ti] || stopsMultiplicity) { جاری; }
                    stateMayHaveMultiplicity[ti] = سچ;
                    multiplicityWork.ڈالو_عقب(ti);
                }
            }
        }

        ثابت منطقی asyncDisable = isAsyncDisable(disableExprp);
        // Registers cleared on a rising disable are also written by their clocked block.
        File_سطر* ثابت regFlp = asyncDisable ? نیا File_سطر{flp} : flp;
        اگر (asyncDisable) regFlp->warnOff(V3_Error_رمز::MULTIDRIVEN, سچ);

        AstVar* ثابت killVarp
            = نیا AstVar{flp, VVarType::MODULETEMP, بنیاد_Name + "__kill", m_u32DTypep};
        killVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->جوڑو_Stmtsp(killVarp);
        برائے (صحیح i = 0; i < N; ++i) {
            اگر (vtx[i]->m_isAndCombiner) {
                ثابت std::string بنیاد = بنیاد_Name + "__a" + std::مقصد_string(i);
                AstVar* ثابت lp = نیا AstVar{regFlp, VVarType::MODULETEMP, بنیاد + "_doneL",
                                              m_modp->ڈھونڈو_ثنائی_ہندسہ_DType()};
                lp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڑو_Stmtsp(lp);
                vtx[i]->datap()->مکمل_LVarp = lp;
                AstVar* ثابت rp = نیا AstVar{regFlp, VVarType::MODULETEMP, بنیاد + "_doneR",
                                              m_modp->ڈھونڈو_ثنائی_ہندسہ_DType()};
                rp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڑو_Stmtsp(rp);
                vtx[i]->datap()->مکمل_RVarp = rp;
                جاری;
            }
            اگر (vtx[i]->m_delay_Ring_سائز) {
                ثابت std::string بنیاد = بنیاد_Name + "__d" + std::مقصد_string(i);
                ثابت منطقی storesCounts = گنتی_Matches && ringNeedsMultiplicity[i];
                Ast_گرہ_DType* ringDTypep = خالی_اشاریہ;
                اگر (storesCounts) {
                    // int unsigned ring [size];
                    ringDTypep = نیا AstUnpackArrayDType{
                        flp, m_u32DTypep,
                        نیا AstRange{flp, static_cast<صحیح>(vtx[i]->m_delay_Ring_سائز - 1), 0}};
                    v3Global.rootp()->typeTablep()->جوڑو_Typesp(ringDTypep);
                } ورنہ {
                    // bit [size-1:0] ring;
                    ringDTypep = m_modp->ڈھونڈو_ثنائی_ہندسہ_DType(vtx[i]->m_delay_Ring_سائز,
                                                      vtx[i]->m_delay_Ring_سائز, VSigning::UNSIGNED);
                }
                AstVar* ثابت ringp
                    = نیا AstVar{flp, VVarType::MODULETEMP, بنیاد + "_ring", ringDTypep};
                ringp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڑو_Stmtsp(ringp);
                vtx[i]->datap()->delayRingVarp = ringp;
                vtx[i]->datap()->delayRingCounts = storesCounts;
                AstVar* ثابت headp
                    = نیا AstVar{flp, VVarType::MODULETEMP, بنیاد + "_head",
                                 storesCounts ? m_u32DTypep : m_modp->ڈھونڈو_ثنائی_ہندسہ_DType()};
                headp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڑو_Stmtsp(headp);
                vtx[i]->datap()->delay_Ring_سر_Varp = headp;
                // int unsigned idx;
                AstVar* ثابت idxp
                    = نیا AstVar{regFlp, VVarType::MODULETEMP, بنیاد + "_idx", m_u32DTypep};
                idxp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڑو_Stmtsp(idxp);
                vtx[i]->datap()->delayRingIdxVarp = idxp;
                // int unsigned live_count;
                AstVar* ثابت زندہ_گنتی_Varp
                    = نیا AstVar{regFlp, VVarType::MODULETEMP, بنیاد + "_liveCount", m_u32DTypep};
                زندہ_گنتی_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڑو_Stmtsp(زندہ_گنتی_Varp);
                vtx[i]->datap()->delay_Ring_زندہ_گنتی_Varp = زندہ_گنتی_Varp;
                AstVar* ثابت wrappedp = نیا AstVar{regFlp, VVarType::MODULETEMP,
                                                    بنیاد + "_wrapped", m_modp->ڈھونڈو_ثنائی_ہندسہ_DType()};
                wrappedp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->جوڑو_Stmtsp(wrappedp);
                vtx[i]->datap()->delayRingWrappedVarp = wrappedp;
                جاری;
            }
            اگر (!vtx[i]->datap()->needsReg) جاری;
            اگر (vtx[i] == گراف.m_آغاز_Vertexp || vtx[i]->m_is_مطابقت) جاری;
            ثابت std::string varName = بنیاد_Name + "__s" + std::مقصد_string(i);
            AstVar* ثابت varp
                = نیا AstVar{regFlp, VVarType::MODULETEMP, varName, m_modp->ڈھونڈو_ثنائی_ہندسہ_DType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جوڑو_Stmtsp(varp);
            vtx[i]->datap()->stateVarp = varp;
        }

        // Build lowering context for phase sub-functions.
        زیریں_Ctx c{flp,        senTreep,      disableExprp, مطابقت_Condp,
                   assertType, directiveType, killVarp,     گراف};

        // Phase 1: Resolve combinational Links via fixed-point propagation.
        Ast_گرہ_اظہاریہ* ثابت triggerExprp
            = isSeqEvent ? نیا AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}
                         : assertOnCond(flp, assertp->userType(), assertp->directive());
        resolveLinks(c, triggerExprp, گنتی_Matches);
        VL_DO_DANGLING(triggerExprp->delete_درخت(), triggerExprp);

        // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
        emit_State_رجسٹر_Nba(c);
        emitDelayRingUpdates(c, گنتی_Matches);
        emit_And_Combiner_مکمل_Latch_Nba(c);
        emitKillAckNba(c);

        // Phase 3/3a/3b: Compute terminal match/reject signals (cleans up stateSig).
        ثابت Signal_سیٹ sigs
            = compute_اشارات(c, need_Thread_Fail_گنتی, need_Throughout_Thread_Fail_گنتی, گنتی_Matches);

        // Strong s_always[m:n] end-of-simulation liveness: if any in-window state
        // is still set at $finish, the universal-quantifier window never completed
        // (IEEE 1800-2023 16.12.11 strong semantics). Fire the assertion failure
        // from a final block; V3Assert turns the DT_ERROR display into the standard
        // "Assertion failed in %m" message.
        // pending = |{strong state registers, strong delay live counts != 0};
        Ast_گرہ_اظہاریہ* pendingp = خالی_اشاریہ;
        برائے (صحیح i = 0; i < N; ++i) {
            اگر (!vtx[i]->m_strongPending) جاری;
            Ast_گرہ_اظہاریہ* pendingExprp = خالی_اشاریہ;
            اگر (vtx[i]->datap()->stateVarp) {
                pendingExprp = نیا AstVarRef{flp, vtx[i]->datap()->stateVarp, VAccess::READ};
            } ورنہ اگر (vtx[i]->m_strongAlwaysRing) {
                pendingExprp
                    = delay_Ring_Has_زندہ_Threadsp(flp, vtx[i]->datap()->delay_Ring_زندہ_گنتی_Varp);
            } ورنہ {
                جاری;
            }
            اگر (!pendingp) {
                pendingp = pendingExprp;
            } ورنہ {
                pendingp = نیا AstLogOr{flp, pendingp, pendingExprp};
            }
        }
        اگر (pendingp) {
            AstCExpr* ثابت assertOnp
                = نیا AstCExpr{flp, AstCExpr::خالص{}, "vlSymsp->_vm_contextp__->assertOn()", 1};
            Ast_گرہ_اظہاریہ* ثابت condp = نیا AstLogAnd{flp, assertOnp, pendingp};
            AstDisplay* ثابت dispp
                = نیا AstDisplay{flp, VDisplayType::DT_ERROR, "", خالی_اشاریہ, خالی_اشاریہ};
            dispp->fmtp()->timeunit(m_modp->timeunit());
            Ast_گرہ_Stmt* ثابت firep = dispp;
            اگر (v3Global.opt.stopFail()) firep->جوڑو_اگلا(نیا AstStop{flp, جھوٹ});
            m_modp->جوڑو_Stmtsp(نیا AstFinal{flp, نیا AstIf{flp, condp, firep}});
        }

        // Clear userp on every vertex before vertexData is destroyed.
        برائے (صحیح i = 0; i < N; ++i) vtx[i]->userp(خالی_اشاریہ);
        واپس sigs;
    }
};

}  // namespace

//######################################################################
// Top-level visitor

جماعت AssertNfaVisitor final : عوامی VNVisitor {
    // STATE
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module being processed
    AstClocking* m_defaultClockingp = خالی_اشاریہ;  // Default clocking
    AstDefaultDisable* m_defaultDisablep = خالی_اشاریہ;  // Default disable iff
    SvaNfaLowering* m_loweringp = خالی_اشاریہ;  // NFA-to-hardware lowering engine
    Ast_Sen_درخت* m_sampled_قدر_Clockp = خالی_اشاریہ;  // Inherited clock during scoped attachment
    V3_Unique_نام m_prop_Var_نام{"__Vpropvar"};  // Property-local variable names
    V3_Unique_نام m_prop_عارضی_نام{"__VnfaSampled"};  // Hoisted $sampled(propp) temps
    V3_Unique_نام m_fail_گنتی_نام{"__VnfaRemainingFailCount"};  // Fail replay counter names
    std::سیٹ<ثابت AstProperty*> m_inliningProps;  // Recursion guard for inlineNamedProperty

    سانچہ <قسم_نام T_گرہ>
    خالی دورہ_Sampled_قدر(T_گرہ* ثابت nodep) {
        اگر (m_sampled_قدر_Clockp && !nodep->sentreep()) {
            nodep->sentreep(m_sampled_قدر_Clockp->clone_درخت(سچ));
        }
        iterateChildren(nodep);
    }

    // Wire match vertex and mid-window sources for a successful NFA build.
    ساکن خالی wire_مطابقت_And_Mid_Sources(Sva_گراف& گراف, ثابت Build_نتیجہ& نتیجہ, File_سطر* flp) {
        گراف.create_مطابقت_راس();
        // Skip the main term Link when midSources already cover every
        // end-of-match (cover_sequence path); otherwise the per-mid extraction
        // double-counts via the merge vertex.
        اگر (!نتیجہ.رکن_Is_Mid_ملاؤ) { گراف.جوڑو_Link(نتیجہ.رکن_Vertexp, گراف.m_مطابقت_Vertexp); }
        برائے (Sva_State_راس* srcVtxp : نتیجہ.midSources) {
            Ast_گرہ_اظہاریہ* condp = خالی_اشاریہ;
            برائے (Ast_گرہ_اظہاریہ* ثابت tc : srcVtxp->m_throughoutConds) {
                Ast_گرہ_اظہاریہ* ثابت tcClone = tc->clone_درخت_خالص(جھوٹ);
                condp = condp ? نیا AstLogAnd{flp, condp, tcClone} : tcClone;
            }
            گراف.جوڑو_Link(srcVtxp, گراف.m_مطابقت_Vertexp, condp);
            srcVtxp->m_isUnbounded = سچ;
        }
    }

    ساکن Ast_گرہ_اظہاریہ* getSequenceBodyExprp(ثابت AstSequence* seqp) {
        Ast_گرہ* bodyp = seqp->stmtsp();
        جب (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        واپس VN_CAST(bodyp, گرہ_اظہاریہ);
    }

    ساکن AstPropSpec* getPropertySpecp(ثابت AstProperty* propp) {
        Ast_گرہ* stmtp = propp->stmtsp();
        // V3LinkParse emits InitialStaticStmt for property-local variable
        // initialisers; the InitialAutomaticStmt variant only appears for
        // task/function-scope automatic lifetime, not properties.
        جب (stmtp
               && (VN_IS(stmtp, Var) || VN_IS(stmtp, InitialStaticStmt)
                   || VN_IS(stmtp, InitialAutomaticStmt))) {  // LCOV_EXCL_LINE
            stmtp = stmtp->nextp();
        }
        واپس VN_CAST(stmtp, PropSpec);
    }

    خالی inlineNamedProperty(AstPropSpec* outerSpecp, AstFuncRef* funcrefp,
                             ثابت AstProperty* propyp) {
        // Recursion guard: IEEE 1800-2023 16.12.1 forbids recursive properties.
        // V3Width emits "Recursive property call" for direct recursion before this
        // pass runs; this catches any nested-inlining cycle that slips past.
        اگر (!m_inliningProps.emplace(propyp).دوسرا) {
            funcrefp->v3error("Illegal recursive property reference");  // LCOV_EXCL_LINE
            واپس;  // LCOV_EXCL_LINE
        }
        ساخت Guard final {
            std::سیٹ<ثابت AstProperty*>& setr;
            ثابت AstProperty* keyp;
            ~Guard() { setr.erase(keyp); }
        } guard{m_inliningProps, propyp};
        AstPropSpec* propSpecp = getPropertySpecp(propyp);
        UASSERT_OBJ(propSpecp, funcrefp, "Property has no body PropSpec");
        propSpecp = propSpecp->clone_درخت(جھوٹ);

        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propyp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_گرہ_اظہاریہ*> پورٹ_Map;
        برائے (ثابت خودکار& tconnect : tconnects) {
            پورٹ_Map[tconnect.پہلا] = tconnect.دوسرا->exprp();
        }

        // Promote property-local variables to module-level temps (IEEE 16.10).
        std::unordered_map<ثابت AstVar*, AstVar*> localVarMap;
        برائے (Ast_گرہ* stmtp = propyp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                اگر (!varp->isIO()) {
                    ثابت string newName = m_prop_Var_نام.get(varp);
                    AstVar* ثابت newVarp = نیا AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->جوڑو_Stmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        propSpecp->foreach([&](AstVarRef* refp) {
            ثابت خودکار پورٹ_It = پورٹ_Map.ڈھونڈو(refp->varp());
            اگر (پورٹ_It != پورٹ_Map.انتہا()) {
                refp->replaceWith(پورٹ_It->دوسرا->clone_درخت(جھوٹ));
                VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
                واپس;
            }
            ثابت خودکار localIt = localVarMap.ڈھونڈو(refp->varp());
            اگر (localIt != localVarMap.انتہا()) refp->varp(localIt->دوسرا);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        برائے (ثابت خودکار& tconnect : tconnects) {
            ڈالو_Deletep(tconnect.دوسرا->exprp()->unlink_Fr_عقب());
        }
        V3AssertCommon::checkSvaInstanceDelays(propSpecp, funcrefp);

        // Merge disable iff (IEEE 1800-2023 16.12.1)
        اگر (outerSpecp->disablep() && propSpecp->disablep()) {
            outerSpecp->v3error("disable iff expression before property call "
                                "and in its body is not legal");
            ڈالو_Deletep(propSpecp->disablep()->unlink_Fr_عقب());
        }
        اگر (outerSpecp->disablep()) {
            propSpecp->disablep(outerSpecp->disablep()->unlink_Fr_عقب());
        }

        اگر (outerSpecp->sensesp() && propSpecp->sensesp()) {
            outerSpecp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Clock event before property call and in its body");
            ڈالو_Deletep(propSpecp->sensesp()->unlink_Fr_عقب());
        }
        اگر (outerSpecp->sensesp()) {
            Ast_Sen_شے* ثابت sensesp = outerSpecp->sensesp();
            sensesp->unlink_Fr_عقب();
            propSpecp->sensesp(sensesp);
        }

        outerSpecp->replaceWith(propSpecp);
        VL_DO_DANGLING(ڈالو_Deletep(outerSpecp), outerSpecp);
    }

    خالی inlineSequenceRef(AstFuncRef* funcrefp, AstSequence* seqp) {
        Ast_گرہ_اظہاریہ* ثابت bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        Ast_گرہ_اظہاریہ* ثابت clonedp = bodyExprp->clone_درخت(جھوٹ);

        ثابت V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ثابت AstVar*, Ast_گرہ_اظہاریہ*> پورٹ_Map;
        برائے (ثابت خودکار& tconnect : tconnects) {
            پورٹ_Map[tconnect.پہلا] = tconnect.دوسرا->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ثابت خودکار it = پورٹ_Map.ڈھونڈو(refp->varp());
            اگر (it != پورٹ_Map.انتہا()) {
                refp->replaceWith(it->دوسرا->clone_درخت(جھوٹ));
                VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
            }
        });
        برائے (ثابت خودکار& tconnect : tconnects) {
            ڈالو_Deletep(tconnect.دوسرا->exprp()->unlink_Fr_عقب());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ڈالو_Deletep(funcrefp), funcrefp);
        // Clear referenced flag so V3AssertPre cleanup does not emit
        // spurious UNSUPPORTED for sequences that were already inlined here.
        seqp->isReferenced(جھوٹ);
    }

    // Must run before hasMultiCycleExpr() so NFA sees sequence bodies.
    خالی inlineAllSequenceRefs(Ast_گرہ* rootp) {
        منطقی بدلا = سچ;
        جب (بدلا) {
            بدلا = جھوٹ;
            rootp->foreach([&](AstFuncRef* funcrefp) {
                اگر (بدلا) واپس;
                اگر (AstSequence* ثابت seqp = VN_CAST(funcrefp->taskp(), Sequence)) {
                    inlineSequenceRef(funcrefp, seqp);
                    بدلا = سچ;
                }
            });
        }
    }

    ساکن منطقی has_Multi_چکر_اظہاریہ(ثابت Ast_گرہ* nodep) {
        واپس nodep->exists([](ثابت Ast_گرہ* np) {
            اگر (ثابت خودکار* ثابت ep = VN_CAST(np, گرہ_اظہاریہ)) واپس ep->is_Multi_چکر_Sva();
            واپس جھوٹ;
        });
    }

    ساکن VPropStrength effectiveAssertPropStrength(ثابت AstPropSpec* ثابت propSpecp) {
        اگر (propSpecp->propStrength() != VPropStrength::DEFAULT) واپس propSpecp->propStrength();
        واپس propSpecp->fileline()->language() <= V3_Lang_رمز::L1800_2005 ? VPropStrength::STRONG
                                                                           : VPropStrength::WEAK;
    }

    // Bare `assert property (p until q)` with boolean operands stays on
    // V3AssertPre's AstLoop lowering, which preserves per-attempt action-block
    // firings that this NFA's single-bit aggregated state cannot. Strong bare
    // forms are also lowered there. NFA still owns sequence operands and any
    // embedding inside a multi-cycle context (implication consequent, or/and
    // operands, etc.).
    ساکن منطقی is_Bare_چوٹی_Level_Until(Ast_گرہ* propp) {
        Ast_گرہ* p = propp;
        اگر (AstPropSpec* ثابت specp = VN_CAST(p, PropSpec)) p = specp->propp();
        جب (AstLogNot* ثابت notp = VN_CAST(p, LogNot)) p = notp->lhsp();
        AstUntil* ثابت untilp = VN_CAST(p, Until);
        اگر (!untilp) واپس جھوٹ;
        ثابت خودکار hasSeq = [](ثابت Ast_گرہ_اظہاریہ* ep) {
            واپس ep->exists([](ثابت Ast_گرہ_اظہاریہ* np) { واپس np->is_Multi_چکر_Sva(); });
        };
        اگر (hasSeq(untilp->lhsp()) || hasSeq(untilp->rhsp())) واپس جھوٹ;
        واپس سچ;
    }

    ساخت PropertyParts final {
        Ast_گرہ_اظہاریہ* triggerExprp = خالی_اشاریہ;
        Ast_گرہ_اظہاریہ* seqExprp = خالی_اشاریہ;
        منطقی isOverlapped = سچ;
        منطقی hasImplication = جھوٹ;
        منطقی isFollowedBy = جھوٹ;  // True for #-# / #=# (non-vacuous-fail on antecedent miss)
    };

    ساکن PropertyParts decomposeProperty(Ast_گرہ* propp) {
        PropertyParts parts;
        اگر (AstPropSpec* ثابت specp = VN_CAST(propp, PropSpec)) { propp = specp->propp(); }
        اگر (AstImplication* ثابت implp = VN_CAST(propp, Implication)) {
            parts.hasImplication = سچ;
            parts.isOverlapped = implp->isOverlapped();
            parts.isFollowedBy = implp->isFollowedBy();
            parts.triggerExprp = implp->lhsp();
            parts.seqExprp = implp->rhsp();
        } ورنہ اگر (Ast_گرہ_اظہاریہ* ثابت exprp = VN_CAST(propp, گرہ_اظہاریہ)) {
            parts.triggerExprp = خالی_اشاریہ;
            parts.seqExprp = exprp;
        }
        واپس parts;
    }

    ساکن منطقی can_بانٹو_Implication_Pass_Actions(ثابت PropertyParts& parts) {
        UASSERT(parts.hasImplication,
                "Implication pass action split requested without implication");
        UASSERT(parts.triggerExprp, "Implication pass action split requested without trigger");
        // Direct vacuous/nonvacuous classification uses the antecedent value in the current
        // assertion attempt. Leave delayed antecedents on the existing NFA pass path.
        واپس !has_Multi_چکر_اظہاریہ(parts.triggerExprp);
    }

    ساکن خالی بانٹو_Implication_Pass_Actions(AstAssert* assertp, ثابت PropertyParts& parts,
                                            Ast_گرہ_اظہاریہ* nonvacuousMatchp) {
        File_سطر* ثابت flp = assertp->fileline();

        Ast_گرہ* ثابت passsp = assertp->passsp()->unlink_Fr_عقب_With_اگلا();
        Ast_گرہ* splitsp = خالی_اشاریہ;

        اگر (!parts.isFollowedBy) {
            Ast_گرہ_اظہاریہ* ثابت vacuousp
                = نیا AstLogNot{flp, sampled(parts.triggerExprp->clone_درخت_خالص(جھوٹ))};
            Ast_گرہ* ثابت vacuousBodyp = passsp->clone_درخت(جھوٹ);
            splitsp = newPassOnIf(flp, vacuousp, vacuousBodyp, assertp->userType(),
                                  assertp->directive(), /*vacuous=*/سچ);
        }

        AstIf* ثابت nonvacuousp
            = newPassOnIf(flp, nonvacuousMatchp, passsp, assertp->userType(), assertp->directive(),
                          /*vacuous=*/جھوٹ);
        splitsp = splitsp ? Ast_گرہ::جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(splitsp, nonvacuousp)
                          : static_cast<Ast_گرہ*>(nonvacuousp);
        assertp->جوڑو_Passsp(splitsp);
    }

    // On a PropSpec-wrapped assertion whose NFA build failed with a semantic
    // error (errorEmitted), replace the body with a BitFalse const so later
    // passes see a well-formed AST. Returns true if replaced.
    خالی replaceBodyOnBuildError(File_سطر* flp, AstPropSpec* propSpecp, منطقی errorEmitted) {
        اگر (!errorEmitted) واپس;
        Ast_گرہ* ثابت innerPropp = propSpecp->propp();
        innerPropp->replaceWith(نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}});
        VL_DO_DANGLING(ڈالو_Deletep(innerPropp), innerPropp);
    }

    // Hoist a leading clocking event (IEEE 1800-2023 16.7):
    منطقی hoistClockedSeq(AstPropSpec* specp) {
        جب (AstSClocked* ثابت clockedp = VN_CAST(specp->propp(), SClocked)) {
            اگر (specp->sensesp()) {
                clockedp->v3warn(E_UNSUPPORTED, "Unsupported: multiclocked sequence or property");
                replaceBodyOnBuildError(specp->fileline(), specp, سچ);
                واپس سچ;
            }
            برائے (ثابت Ast_Sen_شے* sp = clockedp->sensesp(); sp;
                 sp = VN_CAST(sp->nextp(), Sen_شے)) {
                اگر (!sp->کنارہ_Type().an_کنارہ()) {
                    clockedp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: non-edge clocking event on a sequence; "
                                     "use an edge such as @(posedge clk)");
                    replaceBodyOnBuildError(specp->fileline(), specp, سچ);
                    واپس سچ;
                }
            }
            specp->sensesp(clockedp->sensesp()->unlink_Fr_عقب_With_اگلا());
            Ast_گرہ_اظہاریہ* ثابت bodyp = clockedp->exprp()->unlink_Fr_عقب();
            clockedp->replaceWith(bodyp);
            VL_DO_DANGLING(ڈالو_Deletep(clockedp), clockedp);
        }
        // A clocking event anywhere else in the sequence is not supported.
        ثابت AstSClocked* nestedp = خالی_اشاریہ;
        specp->propp()->foreach([&](ثابت AstSClocked* p) {
            اگر (!nestedp) nestedp = p;
        });
        اگر (nestedp) {
            nestedp->v3warn(E_UNSUPPORTED,
                            "Unsupported: clocking event inside sequence expression");
            replaceBodyOnBuildError(specp->fileline(), specp, سچ);
            واپس سچ;
        }
        واپس جھوٹ;
    }

    // Build the NFA graph for a property body, handling both the antecedent
    // |-> consequent and simple sequence cases. Returns the consequent/body
    // BuildResult (invalid on parse/build failure).
    Build_نتیجہ build_Assertion_گراف(SvaNfaBuilder& builder, Sva_گراف& گراف, Ast_گرہ_اظہاریہ* seqBodyp,
                                    ثابت PropertyParts& parts, File_سطر* flp) {
        اگر (!parts.hasImplication) واپس builder.build(seqBodyp);

        گراف.m_آغاز_Vertexp = گراف.create_State_راس();
        واپس builder.build_Implication_کنارے(parts.triggerExprp, seqBodyp, گراف.m_آغاز_Vertexp,
                                             parts.isOverlapped, parts.isFollowedBy,
                                             parts.triggerExprp, flp);
    }

    // Install pass-action gating and replay simultaneous per-thread failures.
    خالی attachActionHandlers(AstAssert* ثابت assertp, Ast_گرہ_اظہاریہ* مطابقت_Exprp,
                              Ast_Sen_درخت* ثابت threadFailReplaySenTreep,
                              Ast_گرہ_اظہاریہ* ثابت threadFailCountp) {
        // Gate pass handler on match to prevent vacuous-pass firings.
        اگر (مطابقت_Exprp) {
            // needMatch implies passsp() was non-null when evaluated above;
            // lowering does not mutate the assert's pass-action between the
            // two reads, so passsp() is still non-null here.
            Ast_گرہ* passsp = assertp->passsp();
            UASSERT_OBJ(passsp, assertp, "needMatch set but passsp is null");
            passsp->unlink_Fr_عقب_With_اگلا();
            File_سطر* ثابت flp = assertp->fileline();
            AstIf* ثابت ifp = نیا AstIf{flp, مطابقت_Exprp, passsp, خالی_اشاریہ};
            assertp->جوڑو_Passsp(ifp);
            // Fail-handler prefix for overlapping instances (IEEE 16.12):
            // fires when reject=1 && match=1 in the same cycle.
            اگر (assertp->failsp()) assertp->prependFailsp(ifp->clone_درخت(جھوٹ));
        }

        اگر (threadFailCountp) {
            UASSERT_OBJ(threadFailReplaySenTreep, assertp,
                        "Thread fail count missing sensitivity tree");
            Ast_گرہ* ثابت failsp = assertp->failsp();
            File_سطر* ثابت flp = assertp->fileline();
            // IEEE 1800-2023 16.12 requires one action-block evaluation per failed
            // thread. AstAssert handles the first, so replay the rest here.
            AstVar* ثابت بقایا_Fail_گنتی_Varp
                = نیا AstVar{flp, VVarType::MODULETEMP, m_fail_گنتی_نام.get(""),
                             m_modp->ڈھونڈو_Basic_DType(VBasicDTypeKwd::UINT32)};
            بقایا_Fail_گنتی_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جوڑو_Stmtsp(بقایا_Fail_گنتی_Varp);
            Ast_گرہ* ثابت replayStmtsp = نیا Ast_تفویض{
                flp, نیا AstVarRef{flp, بقایا_Fail_گنتی_Varp, VAccess::WRITE}, threadFailCountp};
            AstLoop* ثابت replayLoopp = نیا AstLoop{flp};
            replayLoopp->جوڑو_Stmtsp(نیا AstLoopTest{
                flp, replayLoopp,
                نیا AstGt{flp, نیا AstVarRef{flp, بقایا_Fail_گنتی_Varp, VAccess::READ},
                          newTypedConstp(flp, بقایا_Fail_گنتی_Varp->dtypep(), 1)}});
            replayLoopp->جوڑو_Stmtsp(newIfAssertFailOn(failsp->clone_درخت(سچ), assertp->directive(),
                                                     assertp->userType()));
            AstSub* ثابت decrementedFailCountp
                = نیا AstSub{flp, نیا AstVarRef{flp, بقایا_Fail_گنتی_Varp, VAccess::READ},
                             newTypedConstp(flp, بقایا_Fail_گنتی_Varp->dtypep(), 1)};
            replayLoopp->جوڑو_Stmtsp(
                نیا Ast_تفویض{flp, نیا AstVarRef{flp, بقایا_Fail_گنتی_Varp, VAccess::WRITE},
                              decrementedFailCountp});
            replayStmtsp->جوڑو_اگلا(replayLoopp);
            m_modp->جوڑو_Stmtsp(
                نیا AstAlways{flp, VAlwaysKwd::ALWAYS, threadFailReplaySenTreep, replayStmtsp});
        }
    }

    // Replace one VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). No-op if refp is not in matchMap.
    خالی substitute_مطابقت_شے_Ref(AstVarRef* refp, غیرعلامتی K,
                                ثابت std::unordered_map<ثابت AstVar*, Ast_گرہ_اظہاریہ*>& مطابقت_Map) {
        ثابت خودکار it = مطابقت_Map.ڈھونڈو(refp->varp());
        اگر (it == مطابقت_Map.انتہا()) واپس;
        Ast_گرہ_اظہاریہ* newp = it->دوسرا->clone_درخت_خالص(جھوٹ);
        اگر (K > 0) {
            AstConst* ثابت ticksp = نیا AstConst{refp->fileline(), AstConst::Widthed_قدر{}, 32,
                                                  static_cast<uint32_t>(K)};
            AstPast* ثابت pastp = نیا AstPast{refp->fileline(), newp, ticksp, خالی_اشاریہ,
                                               /* propertyTiming */ سچ};
            pastp->dtypeFrom(newp);
            newp = pastp;
        }
        refp->replaceWith(newp);
        VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
        واپس;
    }

    // Recursively walk a consequent. Returns cycle length consumed and
    // substitutes each VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). Reports E_UNSUPPORTED on non-constant
    // delays or composite sequence operators and returns -1 on failure.
    صحیح
    walk_Substitute_مطابقت_اشیاء(Ast_گرہ_اظہاریہ* nodep, غیرعلامتی K,
                             ثابت std::unordered_map<ثابت AstVar*, Ast_گرہ_اظہاریہ*>& مطابقت_اشیاء) {
        اگر (AstSExpr* ثابت sexprp = VN_CAST(nodep, SExpr)) {
            // IEEE 1800-2023 16.9.2: cycle_delay's lhsp is a constant_expression
            // and the delay form in a sequence is always `##N`, folded by
            // V3Const + V3Param before V3AssertNfa. Range form `##[m:n]` is the
            // only user-visible reject here.
            AstDelay* ثابت delayp = VN_AS(sexprp->delayp(), Delay);
            UASSERT_OBJ(delayp->is_چکر_Delay() && VN_IS(delayp->lhsp(), Const), sexprp,
                        "SVA cycle delay must have a constant lhsp");
            اگر (delayp->isRangeDelay()) {
                sexprp->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                              "non-constant cycle delay in consequent"
                                              " (IEEE 1800-2023 16.10)");
                واپس -1;
            }
            ثابت غیرعلامتی delay_ادوار = VN_AS(delayp->lhsp(), Const)->مقصد_UInt();
            صحیح preLen = 0;
            اگر (Ast_گرہ_اظہاریہ* ثابت prep = sexprp->preExprp()) {
                preLen = walk_Substitute_مطابقت_اشیاء(prep, K, مطابقت_اشیاء);
                اگر (preLen < 0) واپس -1;
            }
            ثابت صحیح bodyLen
                = walk_Substitute_مطابقت_اشیاء(sexprp->exprp(), K + preLen + delay_ادوار, مطابقت_اشیاء);
            اگر (bodyLen < 0) واپس -1;
            واپس preLen + delay_ادوار + bodyLen;
        }
        اگر (nodep->is_Multi_چکر_Sva()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                         "composite sequence operator in consequent"
                                         " (IEEE 1800-2023 16.10)");
            واپس -1;
        }
        std::سمتیہ<AstVarRef*> refs;
        nodep->foreach([&refs](AstVarRef* p) { refs.ڈالو_عقب(p); });
        برائے (AstVarRef* ثابت refp : refs) substitute_مطابقت_شے_Ref(refp, K, مطابقت_اشیاء);
        واپس 0;
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
    منطقی lift_مطابقت_شے_Substitutions(PropertyParts& parts, Ast_گرہ_اظہاریہ* seqBodyp) {
        اگر (!parts.hasImplication) واپس جھوٹ;
        Ast_اظہاریہ_Stmt* ثابت اظہاریہ_Stmtp = VN_CAST(parts.triggerExprp, اظہاریہ_Stmt);
        اگر (!اظہاریہ_Stmtp) واپس جھوٹ;
        // IEEE 1800-2023 16.10 BNF requires `(expr, match_item {, match_item})`
        // with at least one match item; V3LinkParse only emits ExprStmt for
        // this form and only emits AstAssign with VarRef LHS for each item.
        std::unordered_map<ثابت AstVar*, Ast_گرہ_اظہاریہ*> مطابقت_اشیاء;
        برائے (Ast_گرہ* stmtp = اظہاریہ_Stmtp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_تفویض* ثابت assignp = VN_AS(stmtp, تفویض);
            AstVarRef* ثابت lhsRefp = VN_AS(assignp->lhsp(), VarRef);
            مطابقت_اشیاء[lhsRefp->varp()] = assignp->rhsp();
        }
        ثابت غیرعلامتی آغاز_K = parts.isOverlapped ? 0 : 1;
        ثابت صحیح لمبائی_2 = walk_Substitute_مطابقت_اشیاء(seqBodyp, آغاز_K, مطابقت_اشیاء);
        // Match-item substitution / strip mutates ancestor purity. Release
        // builds don't auto-clear caches on edits, so refresh here.
        VIsCached::clear_Cache_درخت();
        اگر (لمبائی_2 < 0) واپس سچ;
        Ast_گرہ_اظہاریہ* ثابت antBoolp = اظہاریہ_Stmtp->resultp()->unlink_Fr_عقب();
        اظہاریہ_Stmtp->replaceWith(antBoolp);
        VL_DO_DANGLING(ڈالو_Deletep(اظہاریہ_Stmtp), اظہاریہ_Stmtp);
        parts.triggerExprp = antBoolp;
        واپس جھوٹ;
    }

    // Outcome counts for a property if/case are wrong in an outcome-multiplying
    // context. Returns that context, or nullptr when the shape is supported.
    ساکن ثابت حرف* unsupportedPropertyControl(ثابت Ast_گرہ_Cover_Or_Assert* assertp,
                                                  ثابت Ast_گرہ_اظہاریہ* seqBodyp, منطقی negated) {
        اگر (!hasPropertyControlConjunction(seqBodyp)) واپس خالی_اشاریہ;
        اگر (negated) واپس "negation";
        اگر (VN_IS(assertp, Cover)) واپس "cover";
        اگر (VN_AS(assertp, Assert)->passsp()) واپس "a pass action";
        واپس خالی_اشاریہ;
    }

    // Inline property/sequence refs and reject unsupported shapes.
    // Returns the PropSpec to lower, or nullptr when fully handled here.
    AstPropSpec* prepareAssertionProp(Ast_گرہ_Cover_Or_Assert* assertp) {
        اگر (AstPropSpec* ثابت specp = VN_CAST(assertp->propp(), PropSpec)) {
            اگر (AstFuncRef* ثابت funcrefp = VN_CAST(specp->propp(), FuncRef)) {
                اگر (ثابت AstProperty* ثابت propyp = VN_CAST(funcrefp->taskp(), Property)) {
                    inlineNamedProperty(specp, funcrefp, propyp);
                }
            }
        }

        inlineAllSequenceRefs(assertp->propp());

        اگر (AstPropSpec* ثابت specp = VN_CAST(assertp->propp(), PropSpec)) {
            اگر (hoistClockedSeq(specp)) واپس خالی_اشاریہ;
        }

        AstPropSpec* ثابت propp = VN_AS(assertp->propp(), PropSpec);
        اگر (!VN_IS(assertp, Cover)
            && effectiveAssertPropStrength(propp) == VPropStrength::STRONG) {
            propp->v3warn(E_UNSUPPORTED,
                          "Unsupported: strong property in " + assertp->verilogKwd() + ".");
            replaceBodyOnBuildError(assertp->fileline(), propp, /*errorEmitted=*/سچ);
            واپس خالی_اشاریہ;
        }

        اگر (!has_Multi_چکر_اظہاریہ(propp)) واپس خالی_اشاریہ;
        // A nested property instance keeps its body behind the call; lowering would drop it.
        اگر (propp->exists([](ثابت AstFuncRef* refp) { واپس VN_IS(refp->taskp(), Property); })) {
            assertp->v3warn(E_UNSUPPORTED,
                            "Unsupported: property instance inside a multi-cycle property "
                            "expression");
            VL_DO_DANGLING(ڈالو_Deletep(assertp->unlink_Fr_عقب()), assertp);
            واپس خالی_اشاریہ;
        }
        اگر (is_Bare_چوٹی_Level_Until(propp)) واپس خالی_اشاریہ;
        واپس propp;
    }

    خالی processAssertion(Ast_گرہ_Cover_Or_Assert* assertp) {
        اگر (assertp->فوری()) واپس;

        AstPropSpec* ثابت propp = prepareAssertionProp(assertp);
        اگر (!propp) واپس;

        PropertyParts parts = decomposeProperty(propp);
        UASSERT_OBJ(parts.seqExprp, propp, "Property body must be an expression");

        // Unwrap `not` (IEEE 1800-2023 16.12.1); odd count -> negated semantics.
        Ast_گرہ_اظہاریہ* seqBodyp = parts.seqExprp;
        منطقی negated = جھوٹ;
        جب (AstLogNot* ثابت notp = VN_CAST(seqBodyp, LogNot)) {
            negated = !negated;
            seqBodyp = notp->lhsp();
        }

        ثابت حرف* ثابت propertyControlp
            = unsupportedPropertyControl(assertp, seqBodyp, negated);

        // Substitute property-local match-item refs in consequent with
        // $past(rhs, K) before NFA build (IEEE 1800-2023 16.10).
        اگر (lift_مطابقت_شے_Substitutions(parts, seqBodyp)) {
            AstPropSpec* ثابت psp = VN_CAST(assertp->propp(), PropSpec);
            UASSERT_OBJ(psp, assertp, "Concurrent assertion must have PropSpec");
            replaceBodyOnBuildError(assertp->fileline(), psp, /*errorEmitted=*/سچ);
            واپس;
        }

        AstCover* ثابت coverp = VN_CAST(assertp, Cover);
        ثابت منطقی isCoverSeq = coverp && coverp->isCoverSeq();
        // A sequence event control is not an assertion directive; no default
        // disable iff, no assertion control
        ثابت منطقی isSeqEvent = coverp && coverp->isSeqEvent();
        ثابت منطقی گنتی_Matches = isCoverSeq && !isSeqEvent;
        // Inherit module defaults (IEEE 14.12, 16.15) when assertion has none.
        اگر (!propp->sensesp() && m_defaultClockingp) {
            propp->sensesp(m_defaultClockingp->sentreep()->sensesp()->clone_درخت(سچ));
        }
        اگر (!propp->disablep() && m_defaultDisablep && !isSeqEvent) {
            propp->disablep(m_defaultDisablep->condp()->clone_درخت_خالص(سچ));
        }
        اگر (!propp->sensesp()) واپس;
        Ast_Sen_درخت* senTreep
            = نیا Ast_Sen_درخت{propp->fileline(), propp->sensesp()->clone_درخت(سچ)};
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
        SvaNfaBuilder builder{گراف,      m_modp,     m_prop_عارضی_نام,
                              isCoverSeq, isSeqEvent, coverp != خالی_اشاریہ};

        ثابت Build_نتیجہ نتیجہ = build_Assertion_گراف(builder, گراف, seqBodyp, parts, flp);
        اگر (نتیجہ.valid()) wire_مطابقت_And_Mid_Sources(گراف, نتیجہ, flp);
        اگر (!نتیجہ.valid()) {
            // Fall through to V3AssertPre for unsupported constructs; only
            // replace the body on real semantic errors. Any hoisted temps
            // from this attempt become orphan MODULETEMPs; V3Dead removes
            // them along with the dead always_comb driver.
            replaceBodyOnBuildError(flp, propp, نتیجہ.errorEmitted);
            VL_DO_DANGLING(ڈالو_Deletep(senTreep), senTreep);
            واپس;
        }
        // After the build, so a construct the builder rejects reports itself.
        اگر (propertyControlp) {
            seqBodyp->v3warn(E_UNSUPPORTED,
                             "Unsupported: temporal property if/case with " << propertyControlp);
            replaceBodyOnBuildError(flp, propp, /*errorEmitted=*/سچ);
            VL_DO_DANGLING(ڈالو_Deletep(senTreep), senTreep);
            واپس;
        }
        اگر (گنتی_Matches && !v3Global.opt.coverageUser()) {
            SvaNfaBuilder::freeUnlinkedCondp(نتیجہ.finalCondp);
            VL_DO_DANGLING(ڈالو_Deletep(senTreep), senTreep);
            Ast_گرہ* ثابت innerPropp = propp->propp();
            innerPropp->replaceWith(نیا AstConst{flp, AstConst::ثنائی_ہندسہ_False{}});
            VL_DO_DANGLING(ڈالو_Deletep(innerPropp), innerPropp);
            واپس;
        }

        AstAssert* ثابت assertAssertp = VN_CAST(assertp, Assert);
        ثابت منطقی بانٹو_Implication_Passsp = assertAssertp && assertAssertp->passsp()
                                            && parts.hasImplication
                                            && can_بانٹو_Implication_Pass_Actions(parts);
        ثابت منطقی need_مطابقت = assertAssertp && assertAssertp->passsp()
                               && (!parts.hasImplication || بانٹو_Implication_Passsp);

        ثابت منطقی needsThreadFailReplay
            = assertAssertp && assertAssertp->failsp() && !parts.hasImplication;
        ثابت خودکار اشارات
            = m_loweringp->زیریں(assertp, گراف, senTreep, نتیجہ.finalCondp, disableExprp,
                                 needsThreadFailReplay, needsThreadFailReplay && !negated);
        Ast_گرہ_اظہاریہ* مطابقت_Exprp = خالی_اشاریہ;
        Ast_گرہ_اظہاریہ* outputExprp = m_loweringp->assemble_نتیجہ(
            assertp, negated, نتیجہ.finalCondp, اشارات, need_مطابقت ? &مطابقت_Exprp : خالی_اشاریہ);
        اگر (گنتی_Matches) {
            UASSERT_OBJ(اشارات.مطابقت_Countp, coverp, "Cover sequence missing match count");
            VL_DO_DANGLING(outputExprp->delete_درخت(), outputExprp);
            propp->مطابقت_Countp(اشارات.مطابقت_Countp);
            outputExprp = نیا AstNeq{flp, اشارات.مطابقت_Countp->clone_درخت_خالص(جھوٹ),
                                     newTypedConstp(flp, اشارات.مطابقت_Countp->dtypep(), 0)};
        }

        Ast_Sen_درخت* ثابت threadFailReplaySenTreep
            = اشارات.threadFailCountp ? senTreep->clone_درخت(جھوٹ) : خالی_اشاریہ;

        VL_DO_DANGLING(ڈالو_Deletep(senTreep), senTreep);
        اگر (نتیجہ.finalCondp && !نتیجہ.finalCondp->backp()) ڈالو_Deletep(نتیجہ.finalCondp);

        اگر (بانٹو_Implication_Passsp) {
            بانٹو_Implication_Pass_Actions(assertAssertp, parts, مطابقت_Exprp);
        } ورنہ {
            attachActionHandlers(assertAssertp, مطابقت_Exprp, threadFailReplaySenTreep,
                                 اشارات.threadFailCountp);
        }

        Ast_گرہ* ثابت innerPropp = propp->propp();
        innerPropp->replaceWith(outputExprp);
        VL_DO_DANGLING(ڈالو_Deletep(innerPropp), innerPropp);

        UINFO(4, "NFA converted assertion at " << flp << سطر_ختم);

        اگر (dump_گراف_Level() >= 6) گراف.m_گراف.dumpDotFilePrefixed("assert-nfa");
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_loweringp);
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultDisablep);
        m_modp = nodep;
        // A class nested in a module is in the scope of the module's default clocking
        اگر (!VN_IS(nodep, Class)) m_defaultClockingp = خالی_اشاریہ;
        m_defaultDisablep = nodep->defaultDisablep();
        SvaNfaLowering lowering{nodep};
        m_loweringp = &lowering;
        iterateChildren(nodep);
    }
    خالی دورہ(AstClocking* nodep) override {
        اگر (nodep->isDefault() && !m_defaultClockingp) m_defaultClockingp = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_Gen_بلاک* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    خالی دورہ(AstDefaultDisable* nodep) override {}
    خالی دورہ(AstFell* nodep) override { دورہ_Sampled_قدر(nodep); }
    خالی دورہ(AstPast* nodep) override { دورہ_Sampled_قدر(nodep); }
    خالی دورہ(AstRose* nodep) override { دورہ_Sampled_قدر(nodep); }
    خالی دورہ(AstStable* nodep) override { دورہ_Sampled_قدر(nodep); }
    خالی دورہ(AstAssert* nodep) override { processAssertion(nodep); }
    خالی دورہ(AstCover* nodep) override { processAssertion(nodep); }
    خالی دورہ(AstRestrict* nodep) override {
        // Restrict property is ignored by simulators (IEEE 1800-2023 16.12.2).
        // Remove here so temporal SExpr don't leak to V3AssertPre.
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }
    خالی دورہ(AstAssertIntrinsic* nodep) override {}
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    صریح AssertNfaVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Top entry point

خالی V3AssertNfa::assertNfaAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":" << سطر_ختم);
    { AssertNfaVisitor{nodep}; }
    V3Global::dump_Check_Global_درخت("assertnfa", 0, dump_درخت_Either_Level() >= 3);
}
