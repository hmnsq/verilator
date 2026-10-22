// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Collect and print statistics
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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Assert.h"

#ضمن "V3AstUserAllocator.h"
#ضمن "V3Const.h"
#ضمن "V3Stats.h"
#ضمن "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

نطاق {

صنف DefaultDisableLocalVisitor final : عام VNVisitor {
    // STATE
    Ast_ڳنڍ* m_scopep = خالي_اشارو;

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(خالي_اشارو);
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_Gen_بلاڪ* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(خالي_اشارو);
        iterateChildren(nodep);
    }
    فراغ گهمو(AstDefaultDisable* nodep) override {
        UASSERT_OBJ(nodep, m_scopep,
                    "default disable iff must be inside a module or generate block");
        AstDefaultDisable* defaultp = خالي_اشارو;
        إذا (ثابت Ast_ڳنڍ_Module* ثابت modp = VN_CAST(m_scopep, ڳنڍ_Module)) {
            defaultp = modp->defaultDisablep();
        } وإلا {
            defaultp = VN_AS(m_scopep, Gen_بلاڪ)->defaultDisablep();
        }
        إذا (VL_UNLIKELY(defaultp)) {
            nodep->v3error("Only one 'default disable iff' allowed per "
                           << (VN_IS(m_scopep, ڳنڍ_Module) ? "module" : "generate block")
                           << " (IEEE 1800-2023 16.15)");
        } وإلا إذا (Ast_ڳنڍ_Module* ثابت modp = VN_CAST(m_scopep, ڳنڍ_Module)) {
            modp->defaultDisablep(nodep);
        } وإلا {
            VN_AS(m_scopep, Gen_بلاڪ)->defaultDisablep(nodep);
        }
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    واضح DefaultDisableLocalVisitor(AstNetlist* nodep) { iterate(nodep); }
};

صنف DefaultDisablePropagateVisitor final : عام VNVisitor {
    // STATE
    AstDefaultDisable* m_defaultDisablep = خالي_اشارو;

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_Gen_بلاڪ* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        إذا (!nodep->defaultDisablep()) nodep->defaultDisablep(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    واضح DefaultDisablePropagateVisitor(AstNetlist* nodep) { iterate(nodep); }
};

// Lower a sequence used as an event control ('@seq', IEEE 1800-2023 9.4.2.4) into a
// synthesized event fired by an internal 'cover sequence' on each end-of-match
صنف Seq_Event_هيٺاهون_Visitor final : عام VNVisitor {
    // STATE
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Current module
    V3_Unique_نالا m_نالا{"__VseqEvent"};  // Synthesized event names

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_Sen_شي* nodep) override {
        AstFuncRef* ثابت funcrefp = VN_CAST(nodep->sensp(), FuncRef);
        إذا (funcrefp && VN_IS(funcrefp->taskp(), Sequence)) {
            File_سٽ* ثابت flp = nodep->fileline();
            AstVar* ثابت eventp = جديد AstVar{flp, VVarType::MODULETEMP, m_نالا.get(nodep),
                                              m_modp->ڳوليو_Basic_DType(VBasicDTypeKwd::EVENT)};
            eventp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جوڙيو_Stmtsp(eventp);
            v3Global.سيٽ_Has_Events();
            funcrefp->unlink_Fr_پٺ();
            nodep->sensp(جديد AstVarRef{flp, eventp, VAccess::READ});
            ثابت منطقي automaticActual = funcrefp->exists([](ثابت Ast_ڳنڍ_Var_Ref* refp) {
                أرجع refp->varp() && refp->varp()->lifetime().isAutomatic();
            });
            إذا (automaticActual) {
                nodep->v3error("Arguments to a sequence used as an event control must be"
                               " static (IEEE 1800-2023 9.4.2.4)");
                VN_AS(funcrefp->taskp(), Sequence)->isReferenced(خطأ);
                VL_DO_DANGLING(ڌڪو_Deletep(funcrefp), funcrefp);
                أرجع;
            }
            AstFireEvent* ثابت firep
                = جديد AstFireEvent{flp, جديد AstVarRef{flp, eventp, VAccess::WRITE}, خطأ};
            AstCover* ثابت coverp
                = جديد AstCover{flp, جديد AstPropSpec{flp, خالي_اشارو, خالي_اشارو, funcrefp}, firep,
                               VAssertType::CONCURRENT};
            coverp->isCoverSeq(سچ);
            coverp->isSeqEvent(سچ);
            m_modp->جوڙيو_Stmtsp(coverp);
            أرجع;
        }
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    واضح Seq_Event_هيٺاهون_Visitor(AstNetlist* nodep) { iterate(nodep); }
};

}  // namespace

فراغ V3AssertCommon::collectDefaultDisable(AstNetlist* nodep) {
    { DefaultDisableLocalVisitor{nodep}; }
    { DefaultDisablePropagateVisitor{nodep}; }
}

فراغ V3AssertCommon::هيٺاهون_Sequence_Events(AstNetlist* nodep) {
    { Seq_Event_هيٺاهون_Visitor{nodep}; }
    V3Global::dump_Check_Global_وڻ("assertseqevent", 0, dump_وڻ_Either_Level() >= 3);
}

string V3AssertCommon::assertCtlGetCall(ثابت حرف* query, VAssertType type,
                                        VAssertDirectiveType directiveType) {
    // Template arguments let the runtime build the type/directive mask at C++ compile time
    أرجع "vlSymsp->_vm_contextp__->assertCtlGet<"s + std::منزل_string(type) + ", "s
           + std::منزل_string(directiveType) + ">(VerilatedAssertCtlQuery::"s + query + ")"s;
}

منطقي V3AssertCommon::check_Sva_قدر_حد_بندي(Ast_ڳنڍ* nodep, ثابت AstConst* constp, ثابت حرف* what) {
    // Temporal values can materialize as O(N) AST or state in later passes.
    // Leave headroom for a range ring's extra slot and V3Number's signed
    // `(bits + 31) / 32` word count.
    ساكن constexpr بنانشان SVA_قدر_HARD_حد_بندي
        = static_cast<بنانشان>(std::numeric_limits<صحيح>::max()) - 32U;
    ثابت بنانشان configured_حد_بندي = static_cast<بنانشان>(v3Global.opt.max_انگ_ويڪر());
    ثابت بنانشان حد_بندي = std::min(configured_حد_بندي, SVA_قدر_HARD_حد_بندي);
    إذا (constp->انگ().سمائي_اندريون_UInt() && constp->منزل_UInt() <= حد_بندي) أرجع سچ;
    nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << what << " exceeds implementation limit of "
                                                 << حد_بندي
                                                 << (configured_حد_بندي <= SVA_قدر_HARD_حد_بندي
                                                         ? " (--max-num-width)"
                                                         : " (host arithmetic limit)"));
    أرجع خطأ;
}

// The bound of a cycle delay in a sequence, if a non-negative elaboration-time constant
ساكن ثابت AstConst* checkSvaDelayBound(Ast_ڳنڍ* errp, Ast_ڳنڍ_اظهار* boundp, ثابت حرف* what) {
    ثابت AstConst* ثابت constp = VN_CAST(boundp, Const);
    إذا (!constp || (constp->dtypep()->isSigned() && constp->انگ().isNegative())) {
        errp->v3error(what << " is not a non-negative elaboration-time constant"
                              " (IEEE 1800-2023 16.7)");
        أرجع خالي_اشارو;
    }
    إذا (!V3AssertCommon::check_Sva_قدر_حد_بندي(errp, constp, "SVA cycle delay")) أرجع خالي_اشارو;
    أرجع constp;
}

// Fold the bounds of a cycle delay in a sequence, and return if they are valid, else report it
// on errp
ساكن منطقي sva_چڪر_Delay_Valid(ثابت AstDelay* nodep, Ast_ڳنڍ* errp) {
    V3Const::constifyParamsNoWarnEdit(nodep->lhsp());
    ثابت AstConst* ثابت minConstp = checkSvaDelayBound(
        errp, nodep->lhsp(), nodep->isRangeDelay() ? "Range delay minimum" : "Delay value");
    إذا (!nodep->rhsp() || nodep->isUnbounded()) أرجع minConstp != خالي_اشارو;
    V3Const::constifyParamsNoWarnEdit(nodep->rhsp());
    ثابت AstConst* ثابت maxConstp
        = checkSvaDelayBound(errp, nodep->rhsp(), "Range delay maximum");
    إذا (!minConstp || !maxConstp) أرجع خطأ;
    إذا (maxConstp->منزل_UInt() < minConstp->منزل_UInt()) {
        errp->v3error("Range delay maximum must be >= minimum (IEEE 1800-2023 16.7)");
        أرجع خطأ;
    }
    أرجع سچ;
}

فراغ V3AssertCommon::check_Sva_چڪر_Delay(AstDelay* nodep, Ast_ڳنڍ* errp) {
    إذا (sva_چڪر_Delay_Valid(nodep, errp)) أرجع;
    // Make the delay valid, so later steps do not fail after the error
    File_سٽ* ثابت flp = nodep->fileline();
    Ast_ڳنڍ_اظهار* ثابت minp = nodep->lhsp();
    minp->replaceWith(جديد AstConst{flp, AstConst::ٻٽو_انگ_False_Erroring{}});
    VL_DO_DANGLING(minp->delete_وڻ(), minp);
    إذا (nodep->rhsp() && !nodep->isUnbounded()) {
        Ast_ڳنڍ_اظهار* ثابت maxp = nodep->rhsp();
        maxp->replaceWith(جديد AstConst{flp, AstConst::ٻٽو_انگ_False_Erroring{}});
        VL_DO_DANGLING(maxp->delete_وڻ(), maxp);
    }
}

منطقي V3AssertCommon::refsSvaFormal(ثابت Ast_ڳنڍ* nodep) {
    أرجع nodep->exists([](ثابت Ast_ڳنڍ_Var_Ref* refp) {
        ثابت AstVar* ثابت varp = refp->varp();
        إذا (!varp || !varp->isIO()) أرجع خطأ;
        ثابت Ast_ڳنڍ* ثابت declp = varp->aboveLoopp();
        أرجع VN_IS(declp, Sequence) || VN_IS(declp, Property);
    });
}

فراغ V3AssertCommon::checkSvaInstanceDelays(Ast_ڳنڍ* nodep, Ast_ڳنڍ* instp) {
    // A delay still referencing a formal argument of an enclosing sequence or property is
    // checked when that is instanced. Collect first, as checking edits the delays.
    std::ويڪٽر<AstDelay*> delayps;
    nodep->foreach([&](AstDelay* delayp) {
        إذا (!delayp->is_چڪر_Delay() || refsSvaFormal(delayp->lhsp())) أرجع;
        إذا (delayp->rhsp() && refsSvaFormal(delayp->rhsp())) أرجع;
        delayps.ڌڪو_پٺ(delayp);
    });
    لكل (AstDelay* ثابت delayp : delayps) check_Sva_چڪر_Delay(delayp, instp);
}

//######################################################################
// AssertDeFutureVisitor
// If any AstFuture, then move all non-future varrefs to be one cycle behind,
// see IEEE 1800-2023 16.9.4.

صنف AssertDeFutureVisitor final : عام VNVisitor {
    // STATE - across all visitors
    Ast_ڳنڍ_Module* ثابت m_modp;  // Module future is underneath
    ثابت AstFuture* m_futurep;  // First AstFuture found
    ثابت بنانشان m_past_انگ;  // Prefix unique number for this module
    std::map<AstVar*, AstVar*> m_delayedVars;  // Old to delayed variable mapping
    // STATE - for current visit position (use VL_RESTORER)
    منطقي m_اندريون_Future = خطأ;  // Inside a future
    منطقي m_unsupported = خطأ;  // Printed unsupported

    // METHODS
    فراغ unsupported(Ast_ڳنڍ* nodep) {
        إذا (m_unsupported) أرجع;
        m_unsupported = سچ;
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported/illegal: Future value function used with expression with "
                          << nodep->prettyOperatorName());
    }
    // VISITORS
    فراغ گهمو(AstFuture* nodep) override {
        VL_RESTORER(m_اندريون_Future);
        m_اندريون_Future = سچ;
        iterateChildren(nodep);
        // Done with the future, this subexpression is current-time
        nodep->replaceWith(nodep->exprp()->unlink_Fr_پٺ());
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Var_Ref* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;
        إذا (m_اندريون_Future || m_unsupported)
            أرجع;  // Need user1 set above, don't process when Future is removed
        إذا (nodep->access().isWriteOrRW()) {
            unsupported(nodep);
            أرجع;
        }
        خودڪار it = m_delayedVars.ڳوليو(nodep->varp());
        AstVar* outvarp;
        إذا (it == m_delayedVars.پڇاڙي()) {
            Ast_Sen_وڻ* ثابت sentreep = m_futurep->sentreep();
            AstAlways* ثابت alwaysp = جديد AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS,
                                                     sentreep->clone_وڻ(خطأ), خالي_اشارو};
            m_modp->جوڙيو_Stmtsp(alwaysp);
            outvarp = جديد AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                 "__Vnotfuture" + cvt_منزل_Str(m_past_انگ) + "_" + nodep->name(),
                                 nodep->dtypep()};
            m_modp->جوڙيو_Stmtsp(outvarp);
            AstVarRef* varRefAWritep = جديد AstVarRef{nodep->fileline(), outvarp, VAccess::WRITE};
            varRefAWritep->user1(سچ);
            Ast_ڳنڍ_Var_Ref* varRefAReadp = nodep->clone_وڻ(خطأ);
            varRefAReadp->user1(سچ);
            Ast_ڳنڍ* ثابت assp = جديد Ast_سونپيو_Dly{nodep->fileline(), varRefAWritep, varRefAReadp};
            alwaysp->جوڙيو_Stmtsp(assp);
            m_delayedVars.emplace(nodep->varp(), outvarp);
        } وإلا {
            outvarp = it->ٻيو;
        }
        AstVarRef* newp = جديد AstVarRef{nodep->fileline(), outvarp, VAccess::READ};
        newp->user1(سچ);
        UINFO(9, "DeFuture " << nodep << "  becomes " << newp);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_FTask_Ref* nodep) override { unsupported(nodep); }
    فراغ گهمو(Ast_طريقو_Call* nodep) override { unsupported(nodep); }
    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        إذا (!nodep->is_خالص()) unsupported(nodep);
        iterateChildren(nodep);
    }

عام:
    // CONSTRUCTORS
    واضح AssertDeFutureVisitor(Ast_ڳنڍ* nodep, Ast_ڳنڍ_Module* modp, بنانشان past_انگ)
        : m_modp{modp}
        , m_past_انگ{past_انگ} {
        // See if any Future before we process
        إذا (nodep->forall([&](ثابت AstFuture* futurep) -> منطقي {
                m_futurep = futurep;
                أرجع خطأ;
            }))
            أرجع;
        // UINFOTREE(9, nodep, "", "defuture-in");
        گهمو(nodep);  // Nodep may get deleted
        // UINFOTREE(9, nodep, "", "defuture-ou");
    }
    ~AssertDeFutureVisitor() = افتراضي;
};

// Prepare one default sampled expression.
صنف Default_Sampled_قدر_Visitor final : عام VNVisitor {
    // METHODS
    ساكن AstConst* newTypeDefault(Ast_ڳنڍ_اظهار* nodep) {
        AstConst* ثابت constp
            = جديد AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()};
        إذا (nodep->dtypep()->isFourstate()) constp->انگ().سيٽ_All_بٽ_X();
        constp->dtypeFrom(nodep);
        أرجع constp;
    }

    // VISITORS
    فراغ گهمو(AstCMethodHard* nodep) override {
        إذا (nodep->طريقو() == VCMethod::EVENT_IS_TRIGGERED) {
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::ٻٽو_انگ_False{}});
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        } وإلا {
            iterateChildren(nodep);
        }
    }
    فراغ گهمو(Ast_ڳنڍ_Var_Ref* nodep) override {
        AstVar* ثابت varp = nodep->varp();
        إذا (varp->isParam()
            || (!varp->isNet() && varp->lifetime().isStatic() && varp->hasUserInit())) {
            أرجع;
        }
        nodep->replaceWith(newTypeDefault(nodep));
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    واضح Default_Sampled_قدر_Visitor(Ast_ڳنڍ* nodep) { iterate(nodep); }
};

//######################################################################
// AssertVisitor

صنف AssertVisitor final : عام VNVisitor {
    // CONSTANTS
    ساكن constexpr uint8_t ALL_ASSERT_TYPES
        = std::numeric_limits<std::underlying_type<VAssertType::en>::type>::max();

    // NODE STATE/TYPES
    // Cleared on netlist
    //  AstNode::user1()         -> bool.  True if processed
    //  AstAlways::user2p()      -> std::vector<AstVar*>. Delayed variables via 'm_delayed'
    //  AstNodeVarRef::user2()   -> bool.  True if shouldn't be sampled
    //  AstIf::user2()           -> bool.  True for assertOn() checks inserted in this pass
    ثابت VNUser1_اندريون_Use m_user1_اندريون_Use;
    ثابت VNUser2_اندريون_Use m_user2_اندريون_Use;
    AstUser2Allocator<AstAlways, std::ويڪٽر<AstVar*>> m_delayed;

    // STATE
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Last module
    ثابت Ast_ڳنڍ* m_beginp = خالي_اشارو;  // Last AstBegin/AstGenBlock
    بنانشان m_monitor_انگ = 0;  // Global $monitor numbering (not per module)
    AstVar* m_monitor_انگ_Varp = خالي_اشارو;  // $monitor number variable
    AstVar* m_monitorOffVarp = خالي_اشارو;  // $monitoroff variable
    بنانشان m_mod_Past_انگ = 0;  // Module past numbering
    بنانشان m_mod_Strobe_انگ = 0;  // Module $strobe numbering
    Ast_ڳنڍ_Procedure* m_procedurep = خالي_اشارو;  // Current procedure
    VDouble0 m_statCover;  // Statistic tracking
    VDouble0 m_statAsNotImm;  // Statistic tracking
    VDouble0 m_statAsImm;  // Statistic tracking
    VDouble0 m_stat_As_ڀريل;  // Statistic tracking
    VDouble0 m_statPastVars;  // Statistic tracking
    VDouble0 m_statAssertOnCombined;  // Statistic tracking
    VDouble0 m_statAssertOnHoisted;  // Statistic tracking
    منطقي m_اندريون_Sampled = خطأ;  // True inside a sampled expression
    منطقي m_اندريون_Restrict = خطأ;  // True inside restrict assertion
    Ast_ڳنڍ* m_passsp = خالي_اشارو;  // Current pass statement
    Ast_ڳنڍ* m_failsp = خالي_اشارو;  // Current fail statement
    Ast_ڳنڍ_Cover_Or_Assert* m_assertp = خالي_اشارو;  // Current assertion
    AstFinal* m_finalp = خالي_اشارو;  // Current final block
    VDouble0 m_statLiftedCaseExprs;  // Count of purified case expressions
    Ast_ڳنڍ_FTask* m_ftaskp = خالي_اشارو;  // Current function/task
    V3_Unique_نالا m_case_عارضي_نالا{"__VCase"};
    V3_Unique_نالا m_ميلاپ_ڳڻپ_نالا{"__VnfaRemainingMatchCount"};  // Match replay counter names
    // Maps from (expression, senTree) to the AstAlways that computes its delayed values.
    std::unordered_map<VNRef<Ast_ڳنڍ_اظهار>, std::unordered_map<VNRef<Ast_Sen_وڻ>, AstAlways*>>
        m_modExpr2Sen2DelayedAlwaysp;
    // Source sampled-value history is initialized, unlike internal property-lowering history.
    std::unordered_map<VNRef<Ast_ڳنڍ_اظهار>, std::unordered_map<VNRef<Ast_Sen_وڻ>, AstAlways*>>
        m_modExpr2Sen2DefaultDelayedAlwaysp;
    // Map from delayed-value AstAlways to its last-sampling-tick time variable
    std::unordered_map<ثابت AstAlways*, AstVar*> m_delayedAlways2TickTimep;

    // METHODS
    ساكن ثابت حرف* assertPassOnQuery(منطقي vacuous) {
        ساكن constexpr ثابت حرف* queries[2]
            = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
        أرجع queries[vacuous];
    }
    ساكن Ast_ڳنڍ_اظهار* assertOnCond(File_سٽ* fl, VAssertType type,
                                     VAssertDirectiveType directiveType) {
        // cppcheck-suppress missingReturn
        بدل (directiveType) {
        حالة VAssertDirectiveType::INTRINSIC: أرجع جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}};
        حالة VAssertDirectiveType::VIOLATION_CASE: {
            إذا (v3Global.opt.assertCase()) {
                أرجع جديد AstCExpr{fl, AstCExpr::خالص{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            // If assertions are off, have constant propagation rip them out later
            // This allows syntax errors and such to be detected normally.
            أرجع جديد AstConst{fl, AstConst::ٻٽو_انگ_False{}};
        }
        حالة VAssertDirectiveType::ASSERT:
        حالة VAssertDirectiveType::COVER:
        حالة VAssertDirectiveType::ASSUME: {
            إذا (v3Global.opt.assertOn()) {
                أرجع جديد AstCExpr{
                    fl, AstCExpr::خالص{},
                    V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
            }
            أرجع جديد AstConst{fl, AstConst::ٻٽو_انگ_False{}};
        }
        حالة VAssertDirectiveType::اندروني:
        حالة VAssertDirectiveType::VIOLATION_IF:
        حالة VAssertDirectiveType::RESTRICT: {
            إذا (v3Global.opt.assertOn()) {
                أرجع جديد AstCExpr{fl, AstCExpr::خالص{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            أرجع جديد AstConst{fl, AstConst::ٻٽو_انگ_False{}};
        }
        }
        VL_UNREACHABLE;
    }

    ساكن منطقي isControlled(VAssertDirectiveType directiveType) {
        أرجع (static_cast<صحيح>(directiveType)
                & (static_cast<صحيح>(VAssertDirectiveType::ASSERT)
                   | static_cast<صحيح>(VAssertDirectiveType::COVER)
                   | static_cast<صحيح>(VAssertDirectiveType::ASSUME)));
    }
    ساكن Ast_ڳنڍ_اظهار* assertPassOnCond(File_سٽ* fl, VAssertType type,
                                         VAssertDirectiveType directiveType, منطقي vacuous) {
        إذا (!isControlled(directiveType)) أرجع جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}};
        إذا (!v3Global.opt.assertOn()) أرجع جديد AstConst{fl, AstConst::ٻٽو_انگ_False{}};
        أرجع جديد AstCExpr{
            fl, AstCExpr::خالص{},
            V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType), 1};
    }
    ساكن Ast_ڳنڍ_اظهار* assertFailOnCond(File_سٽ* fl, VAssertType type,
                                         VAssertDirectiveType directiveType) {
        إذا (!isControlled(directiveType)) أرجع جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}};
        إذا (!v3Global.opt.assertOn()) أرجع جديد AstConst{fl, AstConst::ٻٽو_انگ_False{}};
        أرجع جديد AstCExpr{
            fl, AstCExpr::خالص{},
            V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType), 1};
    }
    string assert_Display_پيغام(ثابت Ast_ڳنڍ* nodep, ثابت string& اڳياڙي, ثابت string& پيغام,
                                VDisplayType severity) {
        إذا (severity == VDisplayType::DT_ERROR || severity == VDisplayType::DT_FATAL) {
            أرجع ("[%0t] "s + اڳياڙي + ": " + nodep->fileline()->filebasename() + ":"
                    + cvt_منزل_Str(nodep->fileline()->lineno()) + ": Assertion failed in %m"
                    + ((پيغام != "") ? ": " : "") + پيغام + "\n");
        }
        أرجع ("[%0t] "s + اڳياڙي + ": " + nodep->fileline()->filebasename() + ":"
                + cvt_منزل_Str(nodep->fileline()->lineno()) + ": %m" + ((پيغام != "") ? ": " : "")
                + پيغام + "\n");
    }
    فراغ replaceDisplay(AstDisplay* nodep, ثابت string& اڳياڙي) {
        nodep->fmtp()->متن(
            assert_Display_پيغام(nodep, اڳياڙي, nodep->fmtp()->متن(), nodep->displayType()));
        nodep->displayType(VDisplayType::DT_WRITE);
        // cppcheck-suppress nullPointer
        Ast_ڳنڍ_اظهار* ثابت timenewp = جديد Ast_وقت{nodep->fileline(), m_modp->timeunit()};
        nodep->fmtp()->prependExprsp(timenewp);
        إذا (!nodep->fmtp()->scopeNamep() && nodep->fmtp()->formatScopeTracking()) {
            nodep->fmtp()->scopeNamep(جديد AstScopeName{nodep->fileline(), سچ});
        }
    }
    AstSampled* new_Sampled_اظهار(Ast_ڳنڍ_اظهار* nodep) {
        أرجع جديد AstSampled{nodep->fileline(), nodep, nodep->dtypep(), سچ};
    }
    AstVarRef* new_Monitor_انگ_Var_Refp(ثابت Ast_ڳنڍ* nodep, VAccess access) {
        إذا (!m_monitor_انگ_Varp) {
            m_monitor_انگ_Varp = جديد AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorNum",
                                          nodep->ڳوليو_UInt64_DType()};
            v3Global.rootp()->dollarUnitPkgp()->جوڙيو_Stmtsp(m_monitor_انگ_Varp);
        }
        AstVarRef* ثابت varrefp = جديد AstVarRef{nodep->fileline(), m_monitor_انگ_Varp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        أرجع varrefp;
    }
    AstVarRef* newMonitorOffVarRefp(ثابت Ast_ڳنڍ* nodep, VAccess access) {
        إذا (!m_monitorOffVarp) {
            m_monitorOffVarp = جديد AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorOff",
                                          nodep->ڳوليو_ٻٽو_انگ_DType()};
            v3Global.rootp()->dollarUnitPkgp()->جوڙيو_Stmtsp(m_monitorOffVarp);
        }
        AstVarRef* ثابت varrefp = جديد AstVarRef{nodep->fileline(), m_monitorOffVarp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        أرجع varrefp;
    }
    ساكن AstIf* newIfAssertOn(Ast_ڳنڍ* bodyp, VAssertDirectiveType directiveType,
                                VAssertType type = VAssertType::اندروني) {
        // Add an internal if to check assertions are on.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_سٽ* ثابت fl = bodyp->fileline();

        Ast_ڳنڍ_اظهار* ثابت condp = assertOnCond(fl, type, directiveType);
        AstIf* ثابت newp = جديد AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(سچ);  // To avoid LATCH warning
        newp->user2(سچ);  // Mark as an assertOn() check
        أرجع newp;
    }
    ساكن Ast_ڳنڍ_Stmt* newIfAssertPassOn(Ast_ڳنڍ* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type, منطقي vacuous) {
        // Add an internal if to check assertion passOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_سٽ* ثابت fl = bodyp->fileline();
        Ast_ڳنڍ_اظهار* ثابت condp = assertPassOnCond(fl, type, directiveType, vacuous);
        Ast_ڳنڍ_If* ثابت newp = جديد AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(سچ);  // To avoid LATCH warning
        newp->user1(سچ);  // Don't assert/cover this if
        newp->user2(سچ);  // Mark as an assertOn() check
        أرجع newp;
    }
    ساكن Ast_ڳنڍ_Stmt* newIfAssertFailOn(Ast_ڳنڍ* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type) {
        // Add an internal if to check assertion failOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_سٽ* ثابت fl = bodyp->fileline();
        Ast_ڳنڍ_اظهار* ثابت condp = assertFailOnCond(fl, type, directiveType);
        Ast_ڳنڍ_If* ثابت newp = جديد AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(سچ);  // To avoid LATCH warning
        newp->user1(سچ);  // Don't assert/cover this if
        newp->user2(سچ);  // Mark as an assertOn() check
        أرجع newp;
    }

    ساكن AstIf* assertCond(ثابت Ast_ڳنڍ_Cover_Or_Assert* nodep, Ast_ڳنڍ_اظهار* propp,
                             Ast_ڳنڍ* passsp, Ast_ڳنڍ* failsp) {
        AstIf* ثابت ifp = جديد AstIf{nodep->fileline(), propp, passsp, failsp};
        // It's more LIKELY that we'll take the nullptr if clause
        // than the sim-killing else clause:
        ifp->branchPred(VBranchPred::BP_LIKELY);
        ifp->isBoundsCheck(سچ);  // To avoid LATCH warning
        أرجع ifp;
    }

    Ast_ڳنڍ_Stmt* assertBody(ثابت Ast_ڳنڍ_Cover_Or_Assert* nodep, Ast_ڳنڍ* propp, Ast_ڳنڍ* passsp,
                            Ast_ڳنڍ* failsp) {
        إذا (AstPExpr* ثابت pexprp = VN_CAST(propp, PExpr)) {
            إذا (!v3Global.opt.timing().is_سيٽ_True()) {
                nodep->v3warn(E_NOTIMING, "This property expression requires --timing");
                VL_DO_DANGLING(ڌڪو_Deletep(pexprp), pexprp);
                أرجع جديد AstBegin{nodep->fileline(), "", خالي_اشارو, خطأ};
            }
            AstFork* ثابت forkp = جديد AstFork{nodep->fileline(), VJoinType::JOIN_NONE};
            forkp->جوڙيو_Forksp(pexprp->bodyp()->unlink_Fr_پٺ());
            إذا (Ast_ڳنڍ_Stmt* ثابت finalp = pexprp->finalp()) {
                إذا (!m_finalp) {
                    m_finalp = جديد AstFinal{m_modp->fileline(), finalp->unlink_Fr_پٺ()};
                    m_modp->جوڙيو_Stmtsp(m_finalp);
                } وإلا {
                    m_finalp->جوڙيو_Stmtsp(finalp->unlink_Fr_پٺ());
                }
            }
            VL_DO_DANGLING2(ڌڪو_Deletep(pexprp), pexprp, propp);
            أرجع forkp;
        }

        أرجع assertCond(nodep, VN_AS(propp, ڳنڍ_اظهار), passsp, failsp);
    }

    Ast_ڳنڍ_Stmt* newFireAssertUnchecked(ثابت Ast_ڳنڍ_Stmt* nodep, ثابت string& پيغام,
                                        Ast_ڳنڍ_اظهار* exprsp = خالي_اشارو) {
        // Like newFireAssert() but omits the asserts-on check
        AstDisplay* ثابت dispp
            = جديد AstDisplay{nodep->fileline(), VDisplayType::DT_ERROR, پيغام, خالي_اشارو, خالي_اشارو};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        Ast_ڳنڍ_Stmt* ثابت bodysp = dispp;
        replaceDisplay(dispp, "%%Error");  // Convert to standard DISPLAY format
        إذا (exprsp) dispp->fmtp()->exprsp()->جوڙيو_ايندڙ(exprsp);
        إذا (v3Global.opt.stopFail()) bodysp->جوڙيو_ايندڙ(جديد AstStop{nodep->fileline(), خطأ});
        أرجع bodysp;
    }

    Ast_ڳنڍ_Stmt* newFireAssert(ثابت Ast_ڳنڍ_Stmt* nodep, VAssertDirectiveType directiveType,
                               VAssertType assertType, ثابت string& پيغام,
                               Ast_ڳنڍ_اظهار* exprsp = خالي_اشارو) {
        Ast_ڳنڍ_Stmt* bodysp = newFireAssertUnchecked(nodep, پيغام, exprsp);
        bodysp = newIfAssertOn(bodysp, directiveType, assertType);
        أرجع bodysp;
    }

    AstVar* createDelayedVar(ثابت std::string& name, AstAlways* alwaysp, Ast_ڳنڍ_اظهار* exprp,
                             Ast_ڳنڍ_اظهار* initp) {
        File_سٽ* ثابت flp = exprp->fileline();
        AstVar* ثابت varp = جديد AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        // TODO: this lifetime seems nonsene (can't have NBAs to automatics), but is as before
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->جوڙيو_Stmtsp(varp);
        // Before enough clock ticks, $past returns the expression's default sampled value.
        إذا (initp) {
            m_modp->جوڙيو_Stmtsp(جديد AstInitialStatic{
                flp, جديد Ast_سونپيو{flp, جديد AstVarRef{flp, varp, VAccess::WRITE}, initp}});
        }
        ++m_statPastVars;
        // Actually set the delayed value
        Ast_ڳنڍ_اظهار* ثابت lhsp = جديد AstVarRef{flp, varp, VAccess::WRITE};
        Ast_سونپيو_Dly* ثابت assignp = جديد Ast_سونپيو_Dly{flp, lhsp, exprp};
        alwaysp->prependStmtsp(assignp);
        أرجع varp;
    }

    AstAlways* getDelayedAlways(Ast_ڳنڍ_اظهار* exprp, Ast_ڳنڍ_اظهار* initp, Ast_Sen_وڻ* senTreep) {
        خودڪار& expr2Sen2Alwaysr
            = initp ? m_modExpr2Sen2DefaultDelayedAlwaysp : m_modExpr2Sen2DelayedAlwaysp;
        AstAlways*& alwayspr = expr2Sen2Alwaysr[*exprp][*senTreep];
        إذا (!alwayspr) {
            File_سٽ* ثابت flp = exprp->fileline();
            // Create the always block that computes the delayed values
            alwayspr = جديد AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, خالي_اشارو};
            m_modp->جوڙيو_Stmtsp(alwayspr);
            // Create the once-delayed variable
            ثابت std::string name = "_Vpast_" + cvt_منزل_Str(m_mod_Past_انگ++) + "_1";
            AstVar* ثابت varp = createDelayedVar(name, alwayspr, exprp, initp);
            // Add it to delayed variable vector
            m_delayed(alwayspr).emplace_پٺ(varp);
        } وإلا {
            // Reusing exiting, not needed
            VL_DO_DANGLING(ڌڪو_Deletep(exprp), exprp);
            إذا (initp) VL_DO_DANGLING(ڌڪو_Deletep(initp), initp);
            VL_DO_DANGLING(ڌڪو_Deletep(senTreep), senTreep);
        }
        أرجع alwayspr;
    }

    Ast_ڳنڍ_اظهار* get_Past_قدر(Ast_ڳنڍ_اظهار* exprp, Ast_ڳنڍ_اظهار* initp, Ast_Sen_وڻ* senTreep,
                              uint32_t ticks) {
        UASSERT_OBJ(ticks > 0, exprp, "Delay must be > 0");
        File_سٽ* ثابت flp = exprp->fileline();
        AstAlways* ثابت alwaysp
            = getDelayedAlways(exprp, initp ? initp->clone_وڻ_خالص(خطأ) : خالي_اشارو, senTreep);
        Ast_ڳنڍ_اظهار* ثابت resultp = past_قدر_Ref(alwaysp, flp, ticks, initp);
        إذا (initp) VL_DO_DANGLING(ڌڪو_Deletep(initp), initp);
        أرجع resultp;
    }

    Ast_ڳنڍ_اظهار* past_قدر_Ref(AstAlways* alwaysp, File_سٽ* flp, uint32_t ticks,
                              Ast_ڳنڍ_اظهار* initp) {
        std::ويڪٽر<AstVar*>& delayedr = m_delayed(alwaysp);
        // Ensure the required delay exists
        طالما (delayedr.سائيز() < ticks) {
            AstVar* ثابت firstp = delayedr.front();
            File_سٽ* ثابت varFlp = firstp->fileline();
            // Create once more delayed value
            std::string name = firstp->name();
            name.resize(name.سائيز() - 1);
            name += std::منزل_string(delayedr.سائيز() + 1);
            Ast_ڳنڍ_اظهار* ثابت prevp = جديد AstVarRef{varFlp, delayedr.پٺ(), VAccess::READ};
            AstVar* ثابت varp = createDelayedVar(name, alwaysp, prevp,
                                                  initp ? initp->clone_وڻ_خالص(خطأ) : خالي_اشارو);
            // Add it to delayed variable vector
            delayedr.emplace_پٺ(varp);
        }
        // Return a reference to the appropriately delayed variable
        أرجع جديد AstVarRef{flp, delayedr.at(ticks - 1), VAccess::READ};
    }

    // Time of the pipeline's last sampling tick, for end-of-simulation readers
    AstVar* get_Past_Tick_وقت_Var(AstAlways* alwaysp) {
        AstVar*& varpr = m_delayedAlways2TickTimep[alwaysp];
        إذا (!varpr) {
            File_سٽ* ثابت flp = alwaysp->fileline();
            varpr = جديد AstVar{flp, VVarType::MODULETEMP,
                               "_Vpast_" + cvt_منزل_Str(m_mod_Past_انگ++) + "_t",
                               m_modp->ڳوليو_UInt64_DType()};
            m_modp->جوڙيو_Stmtsp(varpr);
            Ast_ڳنڍ_اظهار* ثابت timep = جديد AstCExpr{flp, "vlSymsp->_vm_contextp__->time()", 64};
            alwaysp->جوڙيو_Stmtsp(
                جديد Ast_سونپيو{flp, جديد AstVarRef{flp, varpr, VAccess::WRITE}, timep});
        }
        أرجع varpr;
    }

    فراغ گهمو_Assertion_Iterate(Ast_ڳنڍ_Cover_Or_Assert* nodep, Ast_ڳنڍ* failsp) {
        إذا (m_beginp && nodep->name() == "") nodep->name(m_beginp->name());

        { AssertDeFutureVisitor{nodep->propp(), m_modp, m_mod_Past_انگ++}; }

        iterate_And_ايندڙ_Null(nodep->sentreep());
        إذا (AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            iterate_And_ايندڙ_Null(assertp->failsp());
        } وإلا إذا (AstAssertIntrinsic* ثابت assertp = VN_CAST(nodep, AssertIntrinsic)) {
            iterate_And_ايندڙ_Null(assertp->failsp());
        } وإلا إذا (AstCover* ثابت coverp = VN_CAST(nodep, Cover)) {
            iterate_And_ايندڙ_Null(coverp->coverincsp());
        } وإلا إذا (!VN_IS(nodep, Restrict)) {
            nodep->v3fatalSrc("Unhandled assert type");
        }
        iterate_And_ايندڙ_Null(nodep->passsp());
        Ast_Sen_وڻ* sentreep = nodep->sentreep();
        إذا (nodep->فوري()) {
            UASSERT_OBJ(!sentreep, nodep, "Immediate assertions don't have sensitivity");
        } وإلا {
            UASSERT_OBJ(sentreep, nodep, "Concurrent assertions must have sensitivity");
            // Explicit inline clock differs from the enclosing always: hoist
            // and warn. To support this need queue of asserts to activate.
            إذا (m_procedurep && !nodep->senFromAlways()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Procedural concurrent assertion with"
                              " clocking event inside always (IEEE 1800-2023 16.14.6)");
                static_cast<Ast_ڳنڍ*>(m_procedurep)->جوڙيو_ايندڙ(nodep->unlink_Fr_پٺ());
                أرجع;  // Later iterate will pick up
            }
            sentreep->unlink_Fr_پٺ();
            إذا (m_procedurep) { VL_DO_DANGLING(ڌڪو_Deletep(sentreep), sentreep); }
        }
        //
        ثابت string& پيغام = nodep->name();
        Ast_ڳنڍ* passsp = nodep->passsp();
        إذا (passsp) passsp->unlink_Fr_پٺ_With_ايندڙ();
        إذا (AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            failsp = assertp->failsp();
        } وإلا إذا (AstAssertIntrinsic* ثابت assertp = VN_CAST(nodep, AssertIntrinsic)) {
            failsp = assertp->failsp();
        }
        إذا (failsp) failsp->unlink_Fr_پٺ_With_ايندڙ();

        منطقي selfDestruct = خطأ;
        منطقي passspGated = خطأ;
        ثابت AstCover* ثابت coverp = VN_CAST(nodep, Cover);
        // A sequence event control is not an assertion directive; no assertion control
        ثابت منطقي seqEvent = coverp && coverp->isSeqEvent();
        إذا (coverp) {
            ++m_statCover;
            إذا (seqEvent) {
                // Keep the event-fire action, with no coverage bucket
            } وإلا إذا (!v3Global.opt.coverageUser()) {
                selfDestruct = سچ;
            } وإلا {
                // V3Coverage assigned us a bucket to increment.
                AstCoverInc* ثابت covincp = VN_AS(coverp->coverincsp(), CoverInc);
                UASSERT_OBJ(covincp, coverp, "Missing AstCoverInc under assertion");
                covincp->unlink_Fr_پٺ_With_ايندڙ();  // next() might have  AstAssign for trace
                إذا (پيغام != "") covincp->declp()->comment(پيغام);
                إذا (passsp) {
                    passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                               /*vacuous=*/خطأ);
                    passspGated = سچ;
                    passsp = Ast_ڳنڍ::جوڙيو_ايندڙ<Ast_ڳنڍ, Ast_ڳنڍ>(covincp, passsp);
                } وإلا {
                    passsp = covincp;
                }
            }
        } وإلا إذا (VN_IS(nodep, Assert) || VN_IS(nodep, AssertIntrinsic)) {
            إذا (nodep->فوري()) {
                ++m_statAsImm;
            } وإلا {
                ++m_statAsNotImm;
            }
            إذا (!passsp && !failsp)
                failsp = newFireAssertUnchecked(
                    nodep, VN_IS(nodep, AssertIntrinsic) ? "'$cast' failed." : "'assert' failed.");
        } وإلا {
            nodep->v3fatalSrc("Unknown node type");
        }

        VL_RESTORER(m_passsp);
        VL_RESTORER(m_failsp);
        VL_RESTORER(m_assertp);
        m_passsp = passsp;
        m_failsp = failsp;
        m_assertp = nodep;
        iterate(nodep->propp());

        Ast_ڳنڍ* propExprp;
        Ast_ڳنڍ_اظهار* disablep = خالي_اشارو;
        Ast_ڳنڍ_اظهار* ميلاپ_Countp = خالي_اشارو;
        إذا (AstPropSpec* ثابت specp = VN_CAST(nodep->propp(), PropSpec)) {
            propExprp = specp->propp()->unlink_Fr_پٺ();
            إذا (specp->disablep()) disablep = specp->disablep()->unlink_Fr_پٺ();
            ميلاپ_Countp = specp->ميلاپ_Countp();
        } وإلا {
            propExprp = nodep->propp()->unlink_Fr_پٺ();
        }
        File_سٽ* ثابت flp = nodep->fileline();
        منطقي passspAlreadyGated = خطأ;
        إذا (passsp && VN_IS(passsp, If)) passspAlreadyGated = VN_AS(passsp, If)->user1();
        إذا (passsp && !passspGated && !passspAlreadyGated && !VN_IS(propExprp, PExpr)
            && !seqEvent) {
            passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                       /*vacuous=*/خطأ);
        }
        إذا (failsp && !VN_IS(propExprp, PExpr)) {
            failsp = newIfAssertFailOn(failsp, nodep->directive(), nodep->userType());
        }
        إذا (coverp && ميلاپ_Countp && passsp) {
            // Convert the match count into a loop that decrements a temporary variable until it
            // reaches zero.
            ميلاپ_Countp->unlink_Fr_پٺ();
            AstVar* ثابت remainingp = جديد AstVar{
                flp, VVarType::MODULETEMP, m_ميلاپ_ڳڻپ_نالا.get(""), ميلاپ_Countp->dtypep()};
            remainingp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جوڙيو_Stmtsp(remainingp);
            Ast_ڳنڍ* ثابت replaysp
                = جديد Ast_سونپيو{flp, جديد AstVarRef{flp, remainingp, VAccess::WRITE}, ميلاپ_Countp};
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            loopp->جوڙيو_Stmtsp(
                جديد AstLoopTest{flp, loopp, جديد AstVarRef{flp, remainingp, VAccess::READ}});
            loopp->جوڙيو_Stmtsp(passsp);
            loopp->جوڙيو_Stmtsp(
                جديد Ast_سونپيو{flp, جديد AstVarRef{flp, remainingp, VAccess::WRITE},
                              جديد AstSub{flp, جديد AstVarRef{flp, remainingp, VAccess::READ},
                                         جديد AstConst{flp, AstConst::Widthed_قدر{},
                                                      remainingp->dtypep()->ويڪر(), 1}}});
            replaysp->جوڙيو_ايندڙ(loopp);
            passsp = replaysp;
        }
        Ast_ڳنڍ* bodysp = assertBody(nodep, propExprp, passsp, failsp);
        إذا (disablep) bodysp = جديد AstIf{flp, جديد AstLogNot{flp, disablep}, bodysp};
        // Add assertOn check last, for better combining
        إذا (!seqEvent) bodysp = newIfAssertOn(bodysp, nodep->directive(), nodep->userType());
        إذا (sentreep) bodysp = جديد AstAlways{flp, VAlwaysKwd::ALWAYS, sentreep, bodysp};

        إذا (passsp && !passsp->backp()) VL_DO_DANGLING(ڌڪو_Deletep(passsp), passsp);
        إذا (failsp && !failsp->backp()) VL_DO_DANGLING(ڌڪو_Deletep(failsp), failsp);

        // Install it
        إذا (selfDestruct) {
            // Delete it after making the tree.  This way we can tell the user
            // if it wasn't constructed nicely or has other errors without needing --coverage.
            VL_DO_DANGLING(bodysp->delete_وڻ(), bodysp);
            nodep->unlink_Fr_پٺ();
        } وإلا {
            nodep->replaceWith(bodysp);
        }
        // Bye
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }

    منطقي is_خالي_جايون_Stmt(Ast_ڳنڍ* nodep) {
        إذا (!nodep) أرجع سچ;
        إذا (AstBegin* ثابت beginp = VN_CAST(nodep, Begin)) {
            إذا (beginp->declsp()) أرجع خطأ;
            لكل (Ast_ڳنڍ* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (!is_خالي_جايون_Stmt(stmtp)) أرجع خطأ;
            }
            أرجع سچ;
        }
        أرجع خطأ;
    }

    // VISITORS
    فراغ گهمو(AstIf* nodep) override {
        إذا (!nodep->user1_سيٽ_Once()) {
            إذا (nodep->uniquePragma() || nodep->unique0Pragma()) {
                ثابت Ast_ڳنڍ_If* ifp = nodep;
                Ast_ڳنڍ_اظهار* propp = خالي_اشارو;
                منطقي hasDefaultElse = خطأ;
                افعل {
                    // If this statement ends with 'else if', then nextIf will point to the
                    // nextIf statement.  Otherwise it will be null.
                    ثابت Ast_ڳنڍ_If* ثابت nextifp = dynamic_cast<Ast_ڳنڍ_If*>(ifp->elsesp());
                    iterate_And_ايندڙ_Null(ifp->condp());

                    // Recurse into the true case.
                    iterate_And_ايندڙ_Null(ifp->thensp());

                    // If the last else is not an else if, recurse into that too.
                    إذا (ifp->elsesp() && !nextifp) {  //
                        iterate_And_ايندڙ_Null(ifp->elsesp());
                    }

                    // Build a bitmask of the true predicates
                    Ast_ڳنڍ_اظهار* ثابت predp = ifp->condp()->clone_وڻ_خالص(خطأ);
                    إذا (propp) {
                        propp = جديد AstConcat{nodep->fileline(), predp, propp};
                    } وإلا {
                        propp = predp;
                    }

                    // Record if this ends with an 'else' that does not have an if
                    إذا (ifp->elsesp() && !nextifp) hasDefaultElse = سچ;

                    ifp = nextifp;
                } طالما (ifp);

                ثابت منطقي allow_none = nodep->unique0Pragma();

                // Empty case means no property
                إذا (!propp) propp = جديد AstConst{nodep->fileline(), AstConst::ٻٽو_انگ_False{}};

                // Note: if this ends with an 'else', then we don't need to validate that one of
                // the predicates evaluates to true.
                Ast_ڳنڍ_اظهار* ثابت ohot
                    = ((allow_none || hasDefaultElse)
                           ? static_cast<Ast_ڳنڍ_اظهار*>(جديد AstOneHot0{nodep->fileline(), propp})
                           : static_cast<Ast_ڳنڍ_اظهار*>(جديد AstOneHot{nodep->fileline(), propp}));
                ثابت VAssertType assertType
                    = nodep->uniquePragma() ? VAssertType::UNIQUE : VAssertType::UNIQUE0;
                // Move, not clone, as the branches might declare referenced variables
                VNRelinker relinker;
                nodep->unlink_Fr_پٺ(&relinker);
                AstIf* ثابت checkifp
                    = جديد AstIf{nodep->fileline(), جديد AstLogNot{nodep->fileline(), ohot},
                                newFireAssert(nodep, VAssertDirectiveType::VIOLATION_IF,
                                              assertType, "'unique if' statement violated"),
                                nodep};
                checkifp->isBoundsCheck(سچ);  // To avoid LATCH warning
                checkifp->branchPred(VBranchPred::BP_UNLIKELY);
                relinker.relink(checkifp);
                أرجع;
            }
        }

        iterateChildren(nodep);

        إذا (nodep->user2()) {
            // Combine consecutive assertOn checks if possible
            إذا (AstIf* ثابت backp = VN_CAST(nodep->backp(), If)) {
                إذا (backp->nextp() == nodep  //
                    && backp->user2()  //
                    && backp->condp()->same_وڻ(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    backp->جوڙيو_Thensp(nodep->thensp()->unlink_Fr_پٺ_With_ايندڙ());
                    nodep->unlink_Fr_پٺ();
                    VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                    أرجع;
                }
            }
            // Combine nested assertOn checks if possible
            إذا (nodep->thensp() && !nodep->thensp()->nextp() && is_خالي_جايون_Stmt(nodep->elsesp())) {
                AstIf* ثابت checkp = VN_CAST(nodep->thensp(), If);
                إذا (checkp  //
                    && checkp->user2()  //
                    && checkp->condp()->same_وڻ(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    nodep->جوڙيو_Thensp(checkp->thensp()->unlink_Fr_پٺ_With_ايندڙ());
                    VL_DO_DANGLING(ڌڪو_Deletep(checkp->unlink_Fr_پٺ()), checkp);
                    أرجع;
                }
            }
            أرجع;
        }

        // Swap assertOn check with single statement 'if' statement to bubble up for combining
        // Note we can't just swap the conditions as they two Ifs have different flags,
        // so swapping the Ifs themeselves then swapping back the bodies.
        إذا (nodep->condp()->is_خالص()) {
            إذا (nodep->thensp() && !nodep->thensp()->nextp() && is_خالي_جايون_Stmt(nodep->elsesp())) {
                AstIf* ثابت checkp = VN_CAST(nodep->thensp(), If);
                إذا (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlink_Fr_پٺ());
                    nodep->جوڙيو_Thensp(checkp->thensp()->unlink_Fr_پٺ_With_ايندڙ());
                    checkp->جوڙيو_Thensp(nodep);
                    أرجع;
                }
            }
            إذا (nodep->elsesp() && !nodep->elsesp()->nextp() && is_خالي_جايون_Stmt(nodep->thensp())) {
                AstIf* ثابت checkp = VN_CAST(nodep->elsesp(), If);
                إذا (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlink_Fr_پٺ());
                    nodep->جوڙيو_Elsesp(checkp->thensp()->unlink_Fr_پٺ_With_ايندڙ());
                    checkp->جوڙيو_Thensp(nodep);
                    أرجع;
                }
            }
        }
    }

    //========== Case assertions
    فراغ گهمو(AstCase* nodep) override {
        // Introduce temporary variable for AstCase if needed - it is done here and not in V3Case
        // because this phase is before V3Scope and V3Case is not. Doing it before V3Scope ensures
        // that V3Scope will take care of a scope creation
        // We also need to do it before V3Begin, co that pragmas like `unique0` also work correctly
        إذا (!nodep->exprp()->is_خالص()) {
            ++m_statLiftedCaseExprs;
            File_سٽ* ثابت fl = nodep->exprp()->fileline();
            AstVar* ثابت varp = جديد AstVar{fl, VVarType::BLOCKTEMP, m_case_عارضي_نالا.get(nodep),
                                            nodep->exprp()->dtypep()};
            Ast_ڳنڍ_اظهار* ثابت origp = nodep->exprp()->unlink_Fr_پٺ();
            nodep->جوڙيو_هتي_This_As_ايندڙ(
                جديد Ast_سونپيو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, origp});
            nodep->exprp(جديد AstVarRef{fl, varp, VAccess::READ});
            إذا (m_ftaskp) {
                varp->funcLocal(سچ);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } وإلا {
                m_modp->prependStmtsp(varp);
            }
            VIsCached::clear_Cache_وڻ();
        }
        iterateChildren(nodep);
        إذا (!nodep->user1_سيٽ_Once()) {
            منطقي has_default = خطأ;
            لكل (Ast_Case_شي* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Case_شي)) {
                إذا (itemp->isDefault()) has_default = سچ;
            }
            ثابت Ast_ڳنڍ_DType* اظهار_Dtypep = nodep->exprp()->dtypep()->skipRefp();

            VAssertType assertType = VAssertType::اندروني;
            إذا (nodep->ترجيح_Pragma()) {
                assertType = VAssertType::ترجيح;
            } وإلا إذا (nodep->uniquePragma()) {
                assertType = VAssertType::UNIQUE;
            } وإلا إذا (nodep->unique0Pragma()) {
                assertType = VAssertType::UNIQUE0;
            }

            string valFmt;
            إذا (اظهار_Dtypep->isIntegralOrPacked())
                valFmt = " for '" + cvt_منزل_Str(اظهار_Dtypep->ويڪر_Min()) + "'h%X'";
            إذا (nodep->ڀريل_Pragma() || nodep->ترجيح_Pragma()) {
                // Need to add a default if there isn't one already
                ++m_stat_As_ڀريل;
                إذا (!has_default) {
                    nodep->جوڙيو_Itemsp(جديد Ast_Case_شي{
                        nodep->fileline(), خالي_اشارو /*DEFAULT*/,
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      nodep->pragmaString() + ", but non-match found" + valFmt,
                                      valFmt.خالي_جايون() ? خالي_اشارو
                                                     : nodep->exprp()->clone_وڻ_خالص(خطأ))});
                }
            }
            إذا (nodep->parallelPragma() || nodep->uniquePragma() || nodep->unique0Pragma()) {
                // Need to check that one, and only one of the case items match at any moment
                // If there's a default, we allow none to match, else exactly one must match
                ++m_stat_As_ڀريل;
                إذا (!has_default && !nodep->itemsp()) {
                    // Not parallel, but harmlessly so.
                } وإلا {
                    Ast_ڳنڍ_اظهار* propp = خالي_اشارو;
                    لكل (Ast_Case_شي* itemp = nodep->itemsp(); itemp;
                         itemp = VN_AS(itemp->nextp(), Case_شي)) {
                        Ast_ڳنڍ_اظهار* itembitp = خالي_اشارو;
                        لكل (Ast_ڳنڍ_اظهار* icondp = itemp->condsp(); icondp;
                             icondp = VN_AS(icondp->nextp(), ڳنڍ_اظهار)) {
                            Ast_ڳنڍ_اظهار* onep;
                            إذا (AstInsideRange* ثابت rcondp = VN_CAST(icondp, InsideRange)) {
                                onep = rcondp->newAndFromInside(
                                    nodep->exprp()->clone_وڻ_خالص(سچ),
                                    rcondp->lhsp()->clone_وڻ_خالص(سچ),
                                    rcondp->rhsp()->clone_وڻ_خالص(سچ));
                            } وإلا إذا (nodep->casex() || nodep->casez() || nodep->caseInside()) {
                                onep = AstEqWild::newTyped(itemp->fileline(),
                                                           nodep->exprp()->clone_وڻ_خالص(خطأ),
                                                           icondp->clone_وڻ_خالص(خطأ));
                            } وإلا {
                                onep = AstEq::newTyped(icondp->fileline(),
                                                       nodep->exprp()->clone_وڻ_خالص(خطأ),
                                                       icondp->clone_وڻ_خالص(خطأ));
                            }
                            // OR together all conditions within the same case item
                            إذا (onep) {
                                إذا (itembitp) {
                                    itembitp = جديد AstOr{icondp->fileline(), onep, itembitp};
                                } وإلا {
                                    itembitp = onep;
                                }
                            }
                        }
                        إذا (itembitp) {
                            إذا (propp) {
                                propp = جديد AstConcat{itemp->fileline(), itembitp, propp};
                            } وإلا {
                                propp = itembitp;
                            }
                        }
                    }
                    // Empty case means no property
                    إذا (!propp) propp = جديد AstConst{nodep->fileline(), AstConst::ٻٽو_انگ_False{}};
                    ثابت منطقي allow_none = has_default || nodep->unique0Pragma();
                    // The following assertion looks as below.
                    // if (!$onehot(propp)) begin
                    //     if (propp == '0) begin if (!allow_none) $error("none match"); end
                    //     else $error("multiple match");
                    //پڇاڙي
                    Ast_ڳنڍ_اظهار* ثابت ohot = جديد AstOneHot{nodep->fileline(), propp};
                    AstConst* ثابت zero = جديد AstConst{
                        nodep->fileline(), AstConst::Widthed_قدر{}, propp->ويڪر(), 0};
                    AstIf* ثابت ohotIfp
                        = جديد AstIf{nodep->fileline(), جديد AstLogNot{nodep->fileline(), ohot}};
                    AstIf* ثابت zeroIfp = جديد AstIf{
                        nodep->fileline(),
                        جديد AstEq{nodep->fileline(), propp->clone_وڻ_خالص(خطأ), zero}};
                    Ast_ڳنڍ_اظهار* ثابت exprp = nodep->exprp();
                    ثابت string pragmaStr = nodep->pragmaString();
                    إذا (!allow_none)
                        zeroIfp->جوڙيو_Thensp(
                            newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                          pragmaStr + ", but none matched" + valFmt,
                                          valFmt.خالي_جايون() ? خالي_اشارو : exprp->clone_وڻ_خالص(خطأ)));
                    zeroIfp->جوڙيو_Elsesp(
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      pragmaStr + ", but multiple matches found" + valFmt,
                                      valFmt.خالي_جايون() ? خالي_اشارو : exprp->clone_وڻ_خالص(خطأ)));
                    ohotIfp->جوڙيو_Thensp(zeroIfp);
                    ohotIfp->isBoundsCheck(سچ);  // To avoid LATCH warning
                    ohotIfp->branchPred(VBranchPred::BP_UNLIKELY);
                    nodep->جوڙيو_Not_Parallelp(ohotIfp);
                }
            }
        }
    }

    فراغ گهمو(AstFuture* nodep) override {
        nodep->v3error("Future sampled value function called outside property or sequence "
                       "expression (IEEE 16.9.4)");
        nodep->replaceWith(جديد AstConst{nodep->fileline(), 0});
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }

    //========== Past
    فراغ گهمو(AstPast* nodep) override {
        إذا (!nodep->propertyTiming()) {
            nodep->initp(nodep->exprp()->clone_وڻ_خالص(خطأ));
            { Default_Sampled_قدر_Visitor{nodep->initp()}; }
        }
        Ast_ڳنڍ_اظهار* ثابت initp = nodep->initp() ? nodep->initp()->unlink_Fr_پٺ() : خالي_اشارو;
        iterateChildren(nodep);
        uint32_t ticks = 1;
        إذا (nodep->ticksp()) {
            UASSERT_OBJ(VN_IS(nodep->ticksp(), Const), nodep,
                        "Expected constant ticks, checked in V3Width");
            ticks = VN_AS(nodep->ticksp(), Const)->منزل_UInt();
        }
        UASSERT_OBJ(ticks >= 1, nodep, "0 tick should have been checked in V3Width");
        Ast_ڳنڍ_اظهار* ثابت exprp = new_Sampled_اظهار(nodep->exprp()->unlink_Fr_پٺ());
        Ast_Sen_وڻ* ثابت senTreep = nodep->sentreep()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* inp = خالي_اشارو;
        // Deliberately emitted for every ftask $past; calls outside final take the normal stage
        إذا (VN_IS(m_procedurep, Final) || m_ftaskp) {
            // A sampling-tick finish reads one stage deeper (IEEE 1800-2023 16.9.3).
            File_سٽ* ثابت flp = nodep->fileline();
            AstAlways* ثابت alwaysp
                = getDelayedAlways(exprp, initp ? initp->clone_وڻ_خالص(خطأ) : خالي_اشارو, senTreep);
            Ast_ڳنڍ_اظهار* ثابت betweenTicksp = past_قدر_Ref(alwaysp, flp, ticks, initp);
            Ast_ڳنڍ_اظهار* ثابت onTickp = past_قدر_Ref(alwaysp, flp, ticks + 1, initp);
            إذا (initp) VL_DO_DANGLING(ڌڪو_Deletep(initp), initp);
            Ast_ڳنڍ_اظهار* ثابت onTickCondp = جديد AstLogAnd{
                flp, جديد AstCExpr{flp, "vlSymsp->_vm_contextp__->executingFinal()", 1},
                جديد AstEq{flp, جديد AstVarRef{flp, get_Past_Tick_وقت_Var(alwaysp), VAccess::READ},
                          جديد AstCExpr{flp, "vlSymsp->_vm_contextp__->finishPendingTime()", 64}}};
            AstCond* ثابت condp = جديد AstCond{flp, onTickCondp, onTickp, betweenTicksp};
            condp->dtypeFrom(onTickp);
            inp = condp;
        } وإلا {
            inp = get_Past_قدر(exprp, initp, senTreep, ticks);
        }
        nodep->replaceWith(inp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }

    //========== Move $sampled down to read-only variables
    فراغ گهمو(AstSampled* nodep) override {
        إذا (nodep->user1()) أرجع;
        VL_RESTORER(m_اندريون_Sampled);
        {
            m_اندريون_Sampled = سچ;
            iterateChildren(nodep);
        }
        إذا (nodep->exprp()) {
            nodep->replaceWith(nodep->exprp()->unlink_Fr_پٺ());
        } وإلا {
            nodep->unlink_Fr_پٺ();
        }
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Var_Ref* nodep) override {
        إذا (m_اندريون_Sampled && !nodep->user2() && !nodep->varp()->is_عارضي()) {
            إذا (!nodep->access().isReadOnly()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Write to variable in sampled expression");
            } وإلا {
                VNRelinker relinkHandle;
                nodep->unlink_Fr_پٺ(&relinkHandle);
                AstSampled* ثابت newp = new_Sampled_اظهار(nodep);
                relinkHandle.relink(newp);
                newp->user1(1);
                v3Global.سيٽ_Has_Sampled();
            }
        }
    }
    // Don't sample sensitivities
    فراغ گهمو(Ast_Sen_شي* nodep) override {
        VL_RESTORER(m_اندريون_Sampled);
        m_اندريون_Sampled = خطأ;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstPExprClause* nodep) override {
        Ast_ڳنڍ* stmtsp = خالي_اشارو;
        إذا (nodep->pass() && m_passsp) {
            // Cover adds COVERINC by AstNode::addNext, thus need to clone next too.
            stmtsp = m_passsp->clone_وڻ(سچ);
            stmtsp = newIfAssertPassOn(stmtsp, m_assertp->directive(), m_assertp->userType(),
                                       nodep->vacuous());
        } وإلا إذا (!nodep->pass() && m_failsp) {
            stmtsp = m_failsp->clone_وڻ(سچ);
            stmtsp = newIfAssertFailOn(stmtsp, m_assertp->directive(), m_assertp->userType());
        }
        إذا (stmtsp) {
            stmtsp->foreach_And_ايندڙ([](Ast_ڳنڍ_Var_Ref* ثابت refp) {
                // References inside action blocks shouldn't be implicitly sampled
                // m_passsp/m_failsp have been already visited once and refs explicitly sampled
                // are handled already
                refp->user2(1);
            });
            nodep->replaceWith(stmtsp);
        } وإلا {
            nodep->unlink_Fr_پٺ();
        }
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(AstPExpr* nodep) override {
        إذا (m_اندريون_Restrict) {
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
        } وإلا {
            iterateChildren(nodep);
        }
    }

    //========== Statements
    فراغ گهمو(AstDisplay* nodep) override {
        iterateChildren(nodep);
        // Replace the special types with standard text
        إذا (nodep->displayType() == VDisplayType::DT_INFO) {
            replaceDisplay(nodep, "-Info");
        } وإلا إذا (nodep->displayType() == VDisplayType::DT_WARNING) {
            replaceDisplay(nodep, "%%Warning");
        } وإلا إذا (nodep->displayType() == VDisplayType::DT_ERROR) {
            replaceDisplay(nodep, "%%Error");
        } وإلا إذا (nodep->displayType() == VDisplayType::DT_FATAL) {
            replaceDisplay(nodep, "%%Fatal");
        } وإلا إذا (nodep->displayType() == VDisplayType::DT_MONITOR) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            File_سٽ* ثابت fl = nodep->fileline();

            Ast_Sen_شي* monSenItemsp = خالي_اشارو;
            إذا (Ast_ڳنڍ* ثابت monExprsp = nodep->fmtp()->exprsp()) {
                monExprsp->foreach_And_ايندڙ([&](AstVarRef* varrefp) {
                    Ast_Sen_شي* ثابت senItemp
                        = جديد Ast_Sen_شي{fl, VEdgeType::ET_بدليو,
                                         // Clone so get VarRef or VarXRef as needed
                                         varrefp->clone_وڻ(خطأ)};
                    monSenItemsp = Ast_ڳنڍ::جوڙيو_ايندڙ_Null(monSenItemsp, senItemp);
                });
            }

            Ast_Sen_وڻ* ثابت mon_Sen_وڻ = جديد Ast_Sen_وڻ{fl, monSenItemsp};
            ثابت خودڪار mon_انگ = ++m_monitor_انگ;
            // Where $monitor was we do "__VmonitorNum = N;"
            Ast_سونپيو* ثابت newsetp = جديد Ast_سونپيو{
                fl, new_Monitor_انگ_Var_Refp(nodep, VAccess::WRITE), جديد AstConst{fl, mon_انگ}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__VmonitorOn && __VmonitorNum==N) $display(...);"
            Ast_ڳنڍ* ثابت stmtsp = nodep;
            AstIf* ثابت ifp = جديد AstIf{
                fl,
                جديد AstLogAnd{fl, جديد AstLogNot{fl, newMonitorOffVarRefp(nodep, VAccess::READ)},
                              جديد AstEq{fl, جديد AstConst{fl, mon_انگ},
                                        new_Monitor_انگ_Var_Refp(nodep, VAccess::READ)}},
                stmtsp};
            ifp->isBoundsCheck(سچ);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_ڳنڍ* ثابت newp = جديد AstAlways{fl, VAlwaysKwd::ALWAYS, mon_Sen_وڻ, ifp};
            m_modp->جوڙيو_Stmtsp(newp);
        } وإلا إذا (nodep->displayType() == VDisplayType::DT_STROBE) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            // Need one-shot
            File_سٽ* ثابت fl = nodep->fileline();
            AstVar* ثابت varp
                = جديد AstVar{fl, VVarType::MODULETEMP, "__Vstrobe" + cvt_منزل_Str(m_mod_Strobe_انگ++),
                             nodep->ڳوليو_ٻٽو_انگ_DType()};
            m_modp->جوڙيو_Stmtsp(varp);
            // Where $strobe was we do "__Vstrobe = '1;"
            Ast_سونپيو* ثابت newsetp = جديد Ast_سونپيو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                                     جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__Vstrobe) begin $display(...); __Vstrobe = '0; end"
            Ast_ڳنڍ* ثابت stmtsp = nodep;
            AstIf* ثابت ifp = جديد AstIf{fl, جديد AstVarRef{fl, varp, VAccess::READ}, stmtsp};
            ifp->isBoundsCheck(سچ);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_ڳنڍ* ثابت newp = جديد AstAlwaysPostponed{fl, ifp};
            stmtsp->جوڙيو_ايندڙ(جديد Ast_سونپيو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                          جديد AstConst{fl, AstConst::ٻٽو_انگ_False{}}});
            m_modp->جوڙيو_Stmtsp(newp);
        }
    }
    فراغ گهمو(AstMonitorOff* nodep) override {
        Ast_سونپيو* ثابت newp
            = جديد Ast_سونپيو{nodep->fileline(), newMonitorOffVarRefp(nodep, VAccess::WRITE),
                            جديد AstConst{nodep->fileline(), AstConst::ٻٽو_انگ_True{}, nodep->off()}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(AstAssert* nodep) override {  //
        گهمو_Assertion_Iterate(nodep, nodep->failsp());
    }
    فراغ گهمو(AstAssertCtl* nodep) override {
        iterateChildren(nodep);

        منطقي assertTypeConst = سچ;
        إذا (!nodep->assertTypesp()) {
            nodep->assertTypes(VAssertType{ALL_ASSERT_TYPES});
        } وإلا إذا (ثابت AstConst* ثابت assertTypesp = VN_CAST(nodep->assertTypesp(), Const)) {
            nodep->assertTypes(VAssertType{assertTypesp->منزل_SInt()});
        } وإلا {
            assertTypeConst = خطأ;
        }

        منطقي controlTypeConst = خطأ;
        إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep->controlTypep(), Const)) {
            nodep->ctlType(constp->منزل_SInt());
            controlTypeConst = سچ;
        }
        إذا (controlTypeConst
            && (nodep->ctlType() < VAssertCtlType::تالو
                || nodep->ctlType() > VAssertCtlType::VACUOUS_OFF)) {
            nodep->unlink_Fr_پٺ();
            nodep->v3error("Bad $assertcontrol control_type '"
                           << cvt_منزل_Str(static_cast<صحيح>(nodep->ctlType()))
                           << "' (IEEE 1800-2023 Table 20-5)");
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع;
        }
        إذا (assertTypeConst && nodep->assertTypes() != ALL_ASSERT_TYPES
            && nodep->assertTypes().شامل_آهي_Any(VAssertType::UNIQUE | VAssertType::UNIQUE0
                                                | VAssertType::ترجيح)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: assert control assertion_type");
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
            أرجع;
        }

        منطقي directiveTypeConst = سچ;
        إذا (!nodep->directiveTypesp()) {
            nodep->directiveTypes(VAssertDirectiveType::ASSERT | VAssertDirectiveType::ASSUME
                                  | VAssertDirectiveType::COVER);
        } وإلا إذا (ثابت AstConst* ثابت directiveTypesp
                   = VN_CAST(nodep->directiveTypesp(), Const)) {
            nodep->directiveTypes(VAssertDirectiveType{directiveTypesp->منزل_SInt()});
        } وإلا {
            directiveTypeConst = خطأ;
        }
        إذا (!directiveTypeConst) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: non-const assert directive type expression");
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
            أرجع;
        }

        File_سٽ* ثابت fl = nodep->fileline();
        UINFO(9, "Generating assertctl for a module: " << m_modp);
        AstCStmt* ثابت newp = جديد AstCStmt{fl};
        newp->جوڙيو("vlSymsp->_vm_contextp__->assertCtl(");
        newp->جوڙيو(nodep->controlTypep()->unlink_Fr_پٺ());
        newp->جوڙيو(", ");
        إذا (nodep->assertTypesp()) {
            newp->جوڙيو(nodep->assertTypesp()->unlink_Fr_پٺ());
        } وإلا {
            newp->جوڙيو(std::منزل_string(ALL_ASSERT_TYPES));
        }
        newp->جوڙيو(", " + std::منزل_string(nodep->directiveTypes()) + ");\n");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(AstAssertIntrinsic* nodep) override {  //
        گهمو_Assertion_Iterate(nodep, nodep->failsp());
    }
    فراغ گهمو(AstCover* nodep) override {  //
        گهمو_Assertion_Iterate(nodep, خالي_اشارو);
    }
    فراغ گهمو(AstRestrict* nodep) override {
        VL_RESTORER(m_اندريون_Restrict);
        m_اندريون_Restrict = سچ;
        iterateChildren(nodep);
        // IEEE says simulator ignores these
        VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
    }

    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Past_انگ);
        VL_RESTORER(m_mod_Strobe_انگ);
        VL_RESTORER(m_finalp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DelayedAlwaysp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DefaultDelayedAlwaysp);
        VL_RESTORER_CLEAR(m_delayedAlways2TickTimep);
        m_modp = nodep;
        m_mod_Past_انگ = 0;
        m_mod_Strobe_انگ = 0;
        m_finalp = خالي_اشارو;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_Gen_بلاڪ* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstBegin* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
        // If the body is a single assertOn check, bubble it up for combining
        إذا (!nodep->declsp()) {
            إذا (AstIf* ثابت ifp = VN_CAST(nodep->stmtsp(), If)) {
                إذا (ifp->user2() && !ifp->nextp()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(ifp->unlink_Fr_پٺ());
                    nodep->جوڙيو_Stmtsp(ifp->thensp()->unlink_Fr_پٺ_With_ايندڙ());
                    ifp->جوڙيو_Thensp(nodep);
                }
            }
        }
    }

    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح AssertVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~AssertVisitor() override {
        V3Stats::جوڙيو_Stat("Assertions, assert non-immediate statements", m_statAsNotImm);
        V3Stats::جوڙيو_Stat("Assertions, assert immediate statements", m_statAsImm);
        V3Stats::جوڙيو_Stat("Assertions, cover statements", m_statCover);
        V3Stats::جوڙيو_Stat("Assertions, full/parallel case", m_stat_As_ڀريل);
        V3Stats::جوڙيو_Stat("Assertions, $past variables", m_statPastVars);
        V3Stats::جوڙيو_Stat("Assertions, assertOn checks combined", m_statAssertOnCombined);
        V3Stats::جوڙيو_Stat("Assertions, assertOn checks hoisted", m_statAssertOnHoisted);
        V3Stats::جوڙيو_Stat("Assertions, lifted impure case expressions", m_statLiftedCaseExprs);
        // Rewrites can change purity, e.g. by compiling out assertion statements with --no-assert
        VIsCached::clear_Cache_وڻ();
    }
};

//######################################################################
// Top Assert class

فراغ V3Assert::assertAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("assert", 0, dump_وڻ_Either_Level() >= 3);
}
