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
    Ast_غوټه* m_scopep = تش_اشاره;

    // VISITORS
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(تش_اشاره);
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_Gen_بلاک* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(تش_اشاره);
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstDefaultDisable* nodep) override {
        UASSERT_OBJ(nodep, m_scopep,
                    "default disable iff must be inside a module or generate block");
        AstDefaultDisable* defaultp = تش_اشاره;
        إذا (ثابت Ast_غوټه_Module* ثابت modp = VN_CAST(m_scopep, غوټه_Module)) {
            defaultp = modp->defaultDisablep();
        } وإلا {
            defaultp = VN_AS(m_scopep, Gen_بلاک)->defaultDisablep();
        }
        إذا (VL_UNLIKELY(defaultp)) {
            nodep->v3error("Only one 'default disable iff' allowed per "
                           << (VN_IS(m_scopep, غوټه_Module) ? "module" : "generate block")
                           << " (IEEE 1800-2023 16.15)");
        } وإلا إذا (Ast_غوټه_Module* ثابت modp = VN_CAST(m_scopep, غوټه_Module)) {
            modp->defaultDisablep(nodep);
        } وإلا {
            VN_AS(m_scopep, Gen_بلاک)->defaultDisablep(nodep);
        }
    }
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    څرګند DefaultDisableLocalVisitor(AstNetlist* nodep) { iterate(nodep); }
};

صنف DefaultDisablePropagateVisitor final : عام VNVisitor {
    // STATE
    AstDefaultDisable* m_defaultDisablep = تش_اشاره;

    // VISITORS
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_Gen_بلاک* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        إذا (!nodep->defaultDisablep()) nodep->defaultDisablep(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    څرګند DefaultDisablePropagateVisitor(AstNetlist* nodep) { iterate(nodep); }
};

// Lower a sequence used as an event control ('@seq', IEEE 1800-2023 9.4.2.4) into a
// synthesized event fired by an internal 'cover sequence' on each end-of-match
صنف Seq_Event_ښکتنی_Visitor final : عام VNVisitor {
    // STATE
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Current module
    V3_Unique_نومونه m_نومونه{"__VseqEvent"};  // Synthesized event names

    // VISITORS
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_Sen_توکی* nodep) override {
        AstFuncRef* ثابت funcrefp = VN_CAST(nodep->sensp(), FuncRef);
        إذا (funcrefp && VN_IS(funcrefp->taskp(), Sequence)) {
            File_کرښه* ثابت flp = nodep->fileline();
            AstVar* ثابت eventp = جديد AstVar{flp, VVarType::MODULETEMP, m_نومونه.get(nodep),
                                              m_modp->پیدا_کړه_Basic_DType(VBasicDTypeKwd::EVENT)};
            eventp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جمع_کړه_Stmtsp(eventp);
            v3Global.سېټ_Has_Events();
            funcrefp->unlink_Fr_شا();
            nodep->sensp(جديد AstVarRef{flp, eventp, VAccess::READ});
            ثابت منطقي automaticActual = funcrefp->exists([](ثابت Ast_غوټه_Var_Ref* refp) {
                أرجع refp->varp() && refp->varp()->lifetime().isAutomatic();
            });
            إذا (automaticActual) {
                nodep->v3error("Arguments to a sequence used as an event control must be"
                               " static (IEEE 1800-2023 9.4.2.4)");
                VN_AS(funcrefp->taskp(), Sequence)->isReferenced(خطأ);
                VL_DO_DANGLING(ټېل_Deletep(funcrefp), funcrefp);
                أرجع;
            }
            AstFireEvent* ثابت firep
                = جديد AstFireEvent{flp, جديد AstVarRef{flp, eventp, VAccess::WRITE}, خطأ};
            AstCover* ثابت coverp
                = جديد AstCover{flp, جديد AstPropSpec{flp, تش_اشاره, تش_اشاره, funcrefp}, firep,
                               VAssertType::CONCURRENT};
            coverp->isCoverSeq(ریښتیا);
            coverp->isSeqEvent(ریښتیا);
            m_modp->جمع_کړه_Stmtsp(coverp);
            أرجع;
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    څرګند Seq_Event_ښکتنی_Visitor(AstNetlist* nodep) { iterate(nodep); }
};

}  // namespace

فراغ V3AssertCommon::collectDefaultDisable(AstNetlist* nodep) {
    { DefaultDisableLocalVisitor{nodep}; }
    { DefaultDisablePropagateVisitor{nodep}; }
}

فراغ V3AssertCommon::ښکتنی_Sequence_Events(AstNetlist* nodep) {
    { Seq_Event_ښکتنی_Visitor{nodep}; }
    V3Global::dump_Check_Global_ونه("assertseqevent", 0, dump_ونه_Either_Level() >= 3);
}

string V3AssertCommon::assertCtlGetCall(ثابت حرف* query, VAssertType type,
                                        VAssertDirectiveType directiveType) {
    // Template arguments let the runtime build the type/directive mask at C++ compile time
    أرجع "vlSymsp->_vm_contextp__->assertCtlGet<"s + std::منزل_string(type) + ", "s
           + std::منزل_string(directiveType) + ">(VerilatedAssertCtlQuery::"s + query + ")"s;
}

منطقي V3AssertCommon::check_Sva_ارزښت_حد(Ast_غوټه* nodep, ثابت AstConst* constp, ثابت حرف* what) {
    // Temporal values can materialize as O(N) AST or state in later passes.
    // Leave headroom for a range ring's extra slot and V3Number's signed
    // `(bits + 31) / 32` word count.
    ساكن constexpr بینښه SVA_ارزښت_HARD_حد
        = static_cast<بینښه>(std::numeric_limits<صحيح>::max()) - 32U;
    ثابت بینښه configured_حد = static_cast<بینښه>(v3Global.opt.max_عدد_پلنوالی());
    ثابت بینښه حد = std::min(configured_حد, SVA_ارزښت_HARD_حد);
    إذا (constp->عدد().ځاییږي_دننه_UInt() && constp->منزل_UInt() <= حد) أرجع ریښتیا;
    nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << what << " exceeds implementation limit of "
                                                 << حد
                                                 << (configured_حد <= SVA_ارزښت_HARD_حد
                                                         ? " (--max-num-width)"
                                                         : " (host arithmetic limit)"));
    أرجع خطأ;
}

// The bound of a cycle delay in a sequence, if a non-negative elaboration-time constant
ساكن ثابت AstConst* checkSvaDelayBound(Ast_غوټه* errp, Ast_غوټه_بیان* boundp, ثابت حرف* what) {
    ثابت AstConst* ثابت constp = VN_CAST(boundp, Const);
    إذا (!constp || (constp->dtypep()->isSigned() && constp->عدد().isNegative())) {
        errp->v3error(what << " is not a non-negative elaboration-time constant"
                              " (IEEE 1800-2023 16.7)");
        أرجع تش_اشاره;
    }
    إذا (!V3AssertCommon::check_Sva_ارزښت_حد(errp, constp, "SVA cycle delay")) أرجع تش_اشاره;
    أرجع constp;
}

// Fold the bounds of a cycle delay in a sequence, and return if they are valid, else report it
// on errp
ساكن منطقي sva_دوره_Delay_Valid(ثابت AstDelay* nodep, Ast_غوټه* errp) {
    V3Const::constifyParamsNoWarnEdit(nodep->lhsp());
    ثابت AstConst* ثابت minConstp = checkSvaDelayBound(
        errp, nodep->lhsp(), nodep->isRangeDelay() ? "Range delay minimum" : "Delay value");
    إذا (!nodep->rhsp() || nodep->isUnbounded()) أرجع minConstp != تش_اشاره;
    V3Const::constifyParamsNoWarnEdit(nodep->rhsp());
    ثابت AstConst* ثابت maxConstp
        = checkSvaDelayBound(errp, nodep->rhsp(), "Range delay maximum");
    إذا (!minConstp || !maxConstp) أرجع خطأ;
    إذا (maxConstp->منزل_UInt() < minConstp->منزل_UInt()) {
        errp->v3error("Range delay maximum must be >= minimum (IEEE 1800-2023 16.7)");
        أرجع خطأ;
    }
    أرجع ریښتیا;
}

فراغ V3AssertCommon::check_Sva_دوره_Delay(AstDelay* nodep, Ast_غوټه* errp) {
    إذا (sva_دوره_Delay_Valid(nodep, errp)) أرجع;
    // Make the delay valid, so later steps do not fail after the error
    File_کرښه* ثابت flp = nodep->fileline();
    Ast_غوټه_بیان* ثابت minp = nodep->lhsp();
    minp->replaceWith(جديد AstConst{flp, AstConst::بټ_False_Erroring{}});
    VL_DO_DANGLING(minp->delete_ونه(), minp);
    إذا (nodep->rhsp() && !nodep->isUnbounded()) {
        Ast_غوټه_بیان* ثابت maxp = nodep->rhsp();
        maxp->replaceWith(جديد AstConst{flp, AstConst::بټ_False_Erroring{}});
        VL_DO_DANGLING(maxp->delete_ونه(), maxp);
    }
}

منطقي V3AssertCommon::refsSvaFormal(ثابت Ast_غوټه* nodep) {
    أرجع nodep->exists([](ثابت Ast_غوټه_Var_Ref* refp) {
        ثابت AstVar* ثابت varp = refp->varp();
        إذا (!varp || !varp->isIO()) أرجع خطأ;
        ثابت Ast_غوټه* ثابت declp = varp->aboveLoopp();
        أرجع VN_IS(declp, Sequence) || VN_IS(declp, Property);
    });
}

فراغ V3AssertCommon::checkSvaInstanceDelays(Ast_غوټه* nodep, Ast_غوټه* instp) {
    // A delay still referencing a formal argument of an enclosing sequence or property is
    // checked when that is instanced. Collect first, as checking edits the delays.
    std::ویکټور<AstDelay*> delayps;
    nodep->foreach([&](AstDelay* delayp) {
        إذا (!delayp->is_دوره_Delay() || refsSvaFormal(delayp->lhsp())) أرجع;
        إذا (delayp->rhsp() && refsSvaFormal(delayp->rhsp())) أرجع;
        delayps.ټېل_شا(delayp);
    });
    لكل (AstDelay* ثابت delayp : delayps) check_Sva_دوره_Delay(delayp, instp);
}

//######################################################################
// AssertDeFutureVisitor
// If any AstFuture, then move all non-future varrefs to be one cycle behind,
// see IEEE 1800-2023 16.9.4.

صنف AssertDeFutureVisitor final : عام VNVisitor {
    // STATE - across all visitors
    Ast_غوټه_Module* ثابت m_modp;  // Module future is underneath
    ثابت AstFuture* m_futurep;  // First AstFuture found
    ثابت بینښه m_past_عدد;  // Prefix unique number for this module
    std::map<AstVar*, AstVar*> m_delayedVars;  // Old to delayed variable mapping
    // STATE - for current visit position (use VL_RESTORER)
    منطقي m_دننه_Future = خطأ;  // Inside a future
    منطقي m_unsupported = خطأ;  // Printed unsupported

    // METHODS
    فراغ unsupported(Ast_غوټه* nodep) {
        إذا (m_unsupported) أرجع;
        m_unsupported = ریښتیا;
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported/illegal: Future value function used with expression with "
                          << nodep->prettyOperatorName());
    }
    // VISITORS
    فراغ لیدنه(AstFuture* nodep) override {
        VL_RESTORER(m_دننه_Future);
        m_دننه_Future = ریښتیا;
        iterateChildren(nodep);
        // Done with the future, this subexpression is current-time
        nodep->replaceWith(nodep->exprp()->unlink_Fr_شا());
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(Ast_غوټه_Var_Ref* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;
        إذا (m_دننه_Future || m_unsupported)
            أرجع;  // Need user1 set above, don't process when Future is removed
        إذا (nodep->access().isWriteOrRW()) {
            unsupported(nodep);
            أرجع;
        }
        خپلکاری it = m_delayedVars.پیدا_کړه(nodep->varp());
        AstVar* outvarp;
        إذا (it == m_delayedVars.پای_ټکی()) {
            Ast_Sen_ونه* ثابت sentreep = m_futurep->sentreep();
            AstAlways* ثابت alwaysp = جديد AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS,
                                                     sentreep->clone_ونه(خطأ), تش_اشاره};
            m_modp->جمع_کړه_Stmtsp(alwaysp);
            outvarp = جديد AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                 "__Vnotfuture" + cvt_منزل_Str(m_past_عدد) + "_" + nodep->name(),
                                 nodep->dtypep()};
            m_modp->جمع_کړه_Stmtsp(outvarp);
            AstVarRef* varRefAWritep = جديد AstVarRef{nodep->fileline(), outvarp, VAccess::WRITE};
            varRefAWritep->user1(ریښتیا);
            Ast_غوټه_Var_Ref* varRefAReadp = nodep->clone_ونه(خطأ);
            varRefAReadp->user1(ریښتیا);
            Ast_غوټه* ثابت assp = جديد Ast_ټاکل_Dly{nodep->fileline(), varRefAWritep, varRefAReadp};
            alwaysp->جمع_کړه_Stmtsp(assp);
            m_delayedVars.emplace(nodep->varp(), outvarp);
        } وإلا {
            outvarp = it->دوهم;
        }
        AstVarRef* newp = جديد AstVarRef{nodep->fileline(), outvarp, VAccess::READ};
        newp->user1(ریښتیا);
        UINFO(9, "DeFuture " << nodep << "  becomes " << newp);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(Ast_غوټه_FTask_Ref* nodep) override { unsupported(nodep); }
    فراغ لیدنه(Ast_لاره_Call* nodep) override { unsupported(nodep); }
    فراغ لیدنه(Ast_غوټه* nodep) override {
        إذا (!nodep->is_خالص()) unsupported(nodep);
        iterateChildren(nodep);
    }

عام:
    // CONSTRUCTORS
    څرګند AssertDeFutureVisitor(Ast_غوټه* nodep, Ast_غوټه_Module* modp, بینښه past_عدد)
        : m_modp{modp}
        , m_past_عدد{past_عدد} {
        // See if any Future before we process
        إذا (nodep->forall([&](ثابت AstFuture* futurep) -> منطقي {
                m_futurep = futurep;
                أرجع خطأ;
            }))
            أرجع;
        // UINFOTREE(9, nodep, "", "defuture-in");
        لیدنه(nodep);  // Nodep may get deleted
        // UINFOTREE(9, nodep, "", "defuture-ou");
    }
    ~AssertDeFutureVisitor() = افتراضي;
};

// Prepare one default sampled expression.
صنف Default_Sampled_ارزښت_Visitor final : عام VNVisitor {
    // METHODS
    ساكن AstConst* newTypeDefault(Ast_غوټه_بیان* nodep) {
        AstConst* ثابت constp
            = جديد AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()};
        إذا (nodep->dtypep()->isFourstate()) constp->عدد().سېټ_All_بټونه_X();
        constp->dtypeFrom(nodep);
        أرجع constp;
    }

    // VISITORS
    فراغ لیدنه(AstCMethodHard* nodep) override {
        إذا (nodep->لاره() == VCMethod::EVENT_IS_TRIGGERED) {
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بټ_False{}});
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        } وإلا {
            iterateChildren(nodep);
        }
    }
    فراغ لیدنه(Ast_غوټه_Var_Ref* nodep) override {
        AstVar* ثابت varp = nodep->varp();
        إذا (varp->isParam()
            || (!varp->isNet() && varp->lifetime().isStatic() && varp->hasUserInit())) {
            أرجع;
        }
        nodep->replaceWith(newTypeDefault(nodep));
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    څرګند Default_Sampled_ارزښت_Visitor(Ast_غوټه* nodep) { iterate(nodep); }
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
    ثابت VNUser1_دننه_Use m_user1_دننه_Use;
    ثابت VNUser2_دننه_Use m_user2_دننه_Use;
    AstUser2Allocator<AstAlways, std::ویکټور<AstVar*>> m_delayed;

    // STATE
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Last module
    ثابت Ast_غوټه* m_beginp = تش_اشاره;  // Last AstBegin/AstGenBlock
    بینښه m_monitor_عدد = 0;  // Global $monitor numbering (not per module)
    AstVar* m_monitor_عدد_Varp = تش_اشاره;  // $monitor number variable
    AstVar* m_monitorOffVarp = تش_اشاره;  // $monitoroff variable
    بینښه m_mod_Past_عدد = 0;  // Module past numbering
    بینښه m_mod_Strobe_عدد = 0;  // Module $strobe numbering
    Ast_غوټه_Procedure* m_procedurep = تش_اشاره;  // Current procedure
    VDouble0 m_statCover;  // Statistic tracking
    VDouble0 m_statAsNotImm;  // Statistic tracking
    VDouble0 m_statAsImm;  // Statistic tracking
    VDouble0 m_stat_As_ډک;  // Statistic tracking
    VDouble0 m_statPastVars;  // Statistic tracking
    VDouble0 m_statAssertOnCombined;  // Statistic tracking
    VDouble0 m_statAssertOnHoisted;  // Statistic tracking
    منطقي m_دننه_Sampled = خطأ;  // True inside a sampled expression
    منطقي m_دننه_Restrict = خطأ;  // True inside restrict assertion
    Ast_غوټه* m_passsp = تش_اشاره;  // Current pass statement
    Ast_غوټه* m_failsp = تش_اشاره;  // Current fail statement
    Ast_غوټه_Cover_Or_Assert* m_assertp = تش_اشاره;  // Current assertion
    AstFinal* m_finalp = تش_اشاره;  // Current final block
    VDouble0 m_statLiftedCaseExprs;  // Count of purified case expressions
    Ast_غوټه_FTask* m_ftaskp = تش_اشاره;  // Current function/task
    V3_Unique_نومونه m_case_لنډمهاله_نومونه{"__VCase"};
    V3_Unique_نومونه m_سمون_شمېر_نومونه{"__VnfaRemainingMatchCount"};  // Match replay counter names
    // Maps from (expression, senTree) to the AstAlways that computes its delayed values.
    std::unordered_map<VNRef<Ast_غوټه_بیان>, std::unordered_map<VNRef<Ast_Sen_ونه>, AstAlways*>>
        m_modExpr2Sen2DelayedAlwaysp;
    // Source sampled-value history is initialized, unlike internal property-lowering history.
    std::unordered_map<VNRef<Ast_غوټه_بیان>, std::unordered_map<VNRef<Ast_Sen_ونه>, AstAlways*>>
        m_modExpr2Sen2DefaultDelayedAlwaysp;
    // Map from delayed-value AstAlways to its last-sampling-tick time variable
    std::unordered_map<ثابت AstAlways*, AstVar*> m_delayedAlways2TickTimep;

    // METHODS
    ساكن ثابت حرف* assertPassOnQuery(منطقي vacuous) {
        ساكن constexpr ثابت حرف* queries[2]
            = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
        أرجع queries[vacuous];
    }
    ساكن Ast_غوټه_بیان* assertOnCond(File_کرښه* fl, VAssertType type,
                                     VAssertDirectiveType directiveType) {
        // cppcheck-suppress missingReturn
        بدل (directiveType) {
        حالة VAssertDirectiveType::INTRINSIC: أرجع جديد AstConst{fl, AstConst::بټ_True{}};
        حالة VAssertDirectiveType::VIOLATION_CASE: {
            إذا (v3Global.opt.assertCase()) {
                أرجع جديد AstCExpr{fl, AstCExpr::خالص{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            // If assertions are off, have constant propagation rip them out later
            // This allows syntax errors and such to be detected normally.
            أرجع جديد AstConst{fl, AstConst::بټ_False{}};
        }
        حالة VAssertDirectiveType::ASSERT:
        حالة VAssertDirectiveType::COVER:
        حالة VAssertDirectiveType::ASSUME: {
            إذا (v3Global.opt.assertOn()) {
                أرجع جديد AstCExpr{
                    fl, AstCExpr::خالص{},
                    V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
            }
            أرجع جديد AstConst{fl, AstConst::بټ_False{}};
        }
        حالة VAssertDirectiveType::دننی:
        حالة VAssertDirectiveType::VIOLATION_IF:
        حالة VAssertDirectiveType::RESTRICT: {
            إذا (v3Global.opt.assertOn()) {
                أرجع جديد AstCExpr{fl, AstCExpr::خالص{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            أرجع جديد AstConst{fl, AstConst::بټ_False{}};
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
    ساكن Ast_غوټه_بیان* assertPassOnCond(File_کرښه* fl, VAssertType type,
                                         VAssertDirectiveType directiveType, منطقي vacuous) {
        إذا (!isControlled(directiveType)) أرجع جديد AstConst{fl, AstConst::بټ_True{}};
        إذا (!v3Global.opt.assertOn()) أرجع جديد AstConst{fl, AstConst::بټ_False{}};
        أرجع جديد AstCExpr{
            fl, AstCExpr::خالص{},
            V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType), 1};
    }
    ساكن Ast_غوټه_بیان* assertFailOnCond(File_کرښه* fl, VAssertType type,
                                         VAssertDirectiveType directiveType) {
        إذا (!isControlled(directiveType)) أرجع جديد AstConst{fl, AstConst::بټ_True{}};
        إذا (!v3Global.opt.assertOn()) أرجع جديد AstConst{fl, AstConst::بټ_False{}};
        أرجع جديد AstCExpr{
            fl, AstCExpr::خالص{},
            V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType), 1};
    }
    string assert_Display_پیغام(ثابت Ast_غوټه* nodep, ثابت string& مختاړی, ثابت string& پیغام,
                                VDisplayType severity) {
        إذا (severity == VDisplayType::DT_ERROR || severity == VDisplayType::DT_FATAL) {
            أرجع ("[%0t] "s + مختاړی + ": " + nodep->fileline()->filebasename() + ":"
                    + cvt_منزل_Str(nodep->fileline()->lineno()) + ": Assertion failed in %m"
                    + ((پیغام != "") ? ": " : "") + پیغام + "\n");
        }
        أرجع ("[%0t] "s + مختاړی + ": " + nodep->fileline()->filebasename() + ":"
                + cvt_منزل_Str(nodep->fileline()->lineno()) + ": %m" + ((پیغام != "") ? ": " : "")
                + پیغام + "\n");
    }
    فراغ replaceDisplay(AstDisplay* nodep, ثابت string& مختاړی) {
        nodep->fmtp()->متن(
            assert_Display_پیغام(nodep, مختاړی, nodep->fmtp()->متن(), nodep->displayType()));
        nodep->displayType(VDisplayType::DT_WRITE);
        // cppcheck-suppress nullPointer
        Ast_غوټه_بیان* ثابت timenewp = جديد Ast_وخت{nodep->fileline(), m_modp->timeunit()};
        nodep->fmtp()->prependExprsp(timenewp);
        إذا (!nodep->fmtp()->scopeNamep() && nodep->fmtp()->formatScopeTracking()) {
            nodep->fmtp()->scopeNamep(جديد AstScopeName{nodep->fileline(), ریښتیا});
        }
    }
    AstSampled* new_Sampled_بیان(Ast_غوټه_بیان* nodep) {
        أرجع جديد AstSampled{nodep->fileline(), nodep, nodep->dtypep(), ریښتیا};
    }
    AstVarRef* new_Monitor_عدد_Var_Refp(ثابت Ast_غوټه* nodep, VAccess access) {
        إذا (!m_monitor_عدد_Varp) {
            m_monitor_عدد_Varp = جديد AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorNum",
                                          nodep->پیدا_کړه_UInt64_DType()};
            v3Global.rootp()->dollarUnitPkgp()->جمع_کړه_Stmtsp(m_monitor_عدد_Varp);
        }
        AstVarRef* ثابت varrefp = جديد AstVarRef{nodep->fileline(), m_monitor_عدد_Varp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        أرجع varrefp;
    }
    AstVarRef* newMonitorOffVarRefp(ثابت Ast_غوټه* nodep, VAccess access) {
        إذا (!m_monitorOffVarp) {
            m_monitorOffVarp = جديد AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorOff",
                                          nodep->پیدا_کړه_بټ_DType()};
            v3Global.rootp()->dollarUnitPkgp()->جمع_کړه_Stmtsp(m_monitorOffVarp);
        }
        AstVarRef* ثابت varrefp = جديد AstVarRef{nodep->fileline(), m_monitorOffVarp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        أرجع varrefp;
    }
    ساكن AstIf* newIfAssertOn(Ast_غوټه* bodyp, VAssertDirectiveType directiveType,
                                VAssertType type = VAssertType::دننی) {
        // Add an internal if to check assertions are on.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_کرښه* ثابت fl = bodyp->fileline();

        Ast_غوټه_بیان* ثابت condp = assertOnCond(fl, type, directiveType);
        AstIf* ثابت newp = جديد AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(ریښتیا);  // To avoid LATCH warning
        newp->user2(ریښتیا);  // Mark as an assertOn() check
        أرجع newp;
    }
    ساكن Ast_غوټه_Stmt* newIfAssertPassOn(Ast_غوټه* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type, منطقي vacuous) {
        // Add an internal if to check assertion passOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_کرښه* ثابت fl = bodyp->fileline();
        Ast_غوټه_بیان* ثابت condp = assertPassOnCond(fl, type, directiveType, vacuous);
        Ast_غوټه_If* ثابت newp = جديد AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(ریښتیا);  // To avoid LATCH warning
        newp->user1(ریښتیا);  // Don't assert/cover this if
        newp->user2(ریښتیا);  // Mark as an assertOn() check
        أرجع newp;
    }
    ساكن Ast_غوټه_Stmt* newIfAssertFailOn(Ast_غوټه* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type) {
        // Add an internal if to check assertion failOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_کرښه* ثابت fl = bodyp->fileline();
        Ast_غوټه_بیان* ثابت condp = assertFailOnCond(fl, type, directiveType);
        Ast_غوټه_If* ثابت newp = جديد AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(ریښتیا);  // To avoid LATCH warning
        newp->user1(ریښتیا);  // Don't assert/cover this if
        newp->user2(ریښتیا);  // Mark as an assertOn() check
        أرجع newp;
    }

    ساكن AstIf* assertCond(ثابت Ast_غوټه_Cover_Or_Assert* nodep, Ast_غوټه_بیان* propp,
                             Ast_غوټه* passsp, Ast_غوټه* failsp) {
        AstIf* ثابت ifp = جديد AstIf{nodep->fileline(), propp, passsp, failsp};
        // It's more LIKELY that we'll take the nullptr if clause
        // than the sim-killing else clause:
        ifp->branchPred(VBranchPred::BP_LIKELY);
        ifp->isBoundsCheck(ریښتیا);  // To avoid LATCH warning
        أرجع ifp;
    }

    Ast_غوټه_Stmt* assertBody(ثابت Ast_غوټه_Cover_Or_Assert* nodep, Ast_غوټه* propp, Ast_غوټه* passsp,
                            Ast_غوټه* failsp) {
        إذا (AstPExpr* ثابت pexprp = VN_CAST(propp, PExpr)) {
            إذا (!v3Global.opt.timing().is_سېټ_True()) {
                nodep->v3warn(E_NOTIMING, "This property expression requires --timing");
                VL_DO_DANGLING(ټېل_Deletep(pexprp), pexprp);
                أرجع جديد AstBegin{nodep->fileline(), "", تش_اشاره, خطأ};
            }
            AstFork* ثابت forkp = جديد AstFork{nodep->fileline(), VJoinType::JOIN_NONE};
            forkp->جمع_کړه_Forksp(pexprp->bodyp()->unlink_Fr_شا());
            إذا (Ast_غوټه_Stmt* ثابت finalp = pexprp->finalp()) {
                إذا (!m_finalp) {
                    m_finalp = جديد AstFinal{m_modp->fileline(), finalp->unlink_Fr_شا()};
                    m_modp->جمع_کړه_Stmtsp(m_finalp);
                } وإلا {
                    m_finalp->جمع_کړه_Stmtsp(finalp->unlink_Fr_شا());
                }
            }
            VL_DO_DANGLING2(ټېل_Deletep(pexprp), pexprp, propp);
            أرجع forkp;
        }

        أرجع assertCond(nodep, VN_AS(propp, غوټه_بیان), passsp, failsp);
    }

    Ast_غوټه_Stmt* newFireAssertUnchecked(ثابت Ast_غوټه_Stmt* nodep, ثابت string& پیغام,
                                        Ast_غوټه_بیان* exprsp = تش_اشاره) {
        // Like newFireAssert() but omits the asserts-on check
        AstDisplay* ثابت dispp
            = جديد AstDisplay{nodep->fileline(), VDisplayType::DT_ERROR, پیغام, تش_اشاره, تش_اشاره};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        Ast_غوټه_Stmt* ثابت bodysp = dispp;
        replaceDisplay(dispp, "%%Error");  // Convert to standard DISPLAY format
        إذا (exprsp) dispp->fmtp()->exprsp()->جمع_کړه_بل(exprsp);
        إذا (v3Global.opt.stopFail()) bodysp->جمع_کړه_بل(جديد AstStop{nodep->fileline(), خطأ});
        أرجع bodysp;
    }

    Ast_غوټه_Stmt* newFireAssert(ثابت Ast_غوټه_Stmt* nodep, VAssertDirectiveType directiveType,
                               VAssertType assertType, ثابت string& پیغام,
                               Ast_غوټه_بیان* exprsp = تش_اشاره) {
        Ast_غوټه_Stmt* bodysp = newFireAssertUnchecked(nodep, پیغام, exprsp);
        bodysp = newIfAssertOn(bodysp, directiveType, assertType);
        أرجع bodysp;
    }

    AstVar* createDelayedVar(ثابت std::string& name, AstAlways* alwaysp, Ast_غوټه_بیان* exprp,
                             Ast_غوټه_بیان* initp) {
        File_کرښه* ثابت flp = exprp->fileline();
        AstVar* ثابت varp = جديد AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        // TODO: this lifetime seems nonsene (can't have NBAs to automatics), but is as before
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->جمع_کړه_Stmtsp(varp);
        // Before enough clock ticks, $past returns the expression's default sampled value.
        إذا (initp) {
            m_modp->جمع_کړه_Stmtsp(جديد AstInitialStatic{
                flp, جديد Ast_ټاکل{flp, جديد AstVarRef{flp, varp, VAccess::WRITE}, initp}});
        }
        ++m_statPastVars;
        // Actually set the delayed value
        Ast_غوټه_بیان* ثابت lhsp = جديد AstVarRef{flp, varp, VAccess::WRITE};
        Ast_ټاکل_Dly* ثابت assignp = جديد Ast_ټاکل_Dly{flp, lhsp, exprp};
        alwaysp->prependStmtsp(assignp);
        أرجع varp;
    }

    AstAlways* getDelayedAlways(Ast_غوټه_بیان* exprp, Ast_غوټه_بیان* initp, Ast_Sen_ونه* senTreep) {
        خپلکاری& expr2Sen2Alwaysr
            = initp ? m_modExpr2Sen2DefaultDelayedAlwaysp : m_modExpr2Sen2DelayedAlwaysp;
        AstAlways*& alwayspr = expr2Sen2Alwaysr[*exprp][*senTreep];
        إذا (!alwayspr) {
            File_کرښه* ثابت flp = exprp->fileline();
            // Create the always block that computes the delayed values
            alwayspr = جديد AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, تش_اشاره};
            m_modp->جمع_کړه_Stmtsp(alwayspr);
            // Create the once-delayed variable
            ثابت std::string name = "_Vpast_" + cvt_منزل_Str(m_mod_Past_عدد++) + "_1";
            AstVar* ثابت varp = createDelayedVar(name, alwayspr, exprp, initp);
            // Add it to delayed variable vector
            m_delayed(alwayspr).emplace_شا(varp);
        } وإلا {
            // Reusing exiting, not needed
            VL_DO_DANGLING(ټېل_Deletep(exprp), exprp);
            إذا (initp) VL_DO_DANGLING(ټېل_Deletep(initp), initp);
            VL_DO_DANGLING(ټېل_Deletep(senTreep), senTreep);
        }
        أرجع alwayspr;
    }

    Ast_غوټه_بیان* get_Past_ارزښت(Ast_غوټه_بیان* exprp, Ast_غوټه_بیان* initp, Ast_Sen_ونه* senTreep,
                              uint32_t ticks) {
        UASSERT_OBJ(ticks > 0, exprp, "Delay must be > 0");
        File_کرښه* ثابت flp = exprp->fileline();
        AstAlways* ثابت alwaysp
            = getDelayedAlways(exprp, initp ? initp->clone_ونه_خالص(خطأ) : تش_اشاره, senTreep);
        Ast_غوټه_بیان* ثابت resultp = past_ارزښت_Ref(alwaysp, flp, ticks, initp);
        إذا (initp) VL_DO_DANGLING(ټېل_Deletep(initp), initp);
        أرجع resultp;
    }

    Ast_غوټه_بیان* past_ارزښت_Ref(AstAlways* alwaysp, File_کرښه* flp, uint32_t ticks,
                              Ast_غوټه_بیان* initp) {
        std::ویکټور<AstVar*>& delayedr = m_delayed(alwaysp);
        // Ensure the required delay exists
        طالما (delayedr.اندازه() < ticks) {
            AstVar* ثابت firstp = delayedr.front();
            File_کرښه* ثابت varFlp = firstp->fileline();
            // Create once more delayed value
            std::string name = firstp->name();
            name.resize(name.اندازه() - 1);
            name += std::منزل_string(delayedr.اندازه() + 1);
            Ast_غوټه_بیان* ثابت prevp = جديد AstVarRef{varFlp, delayedr.شا(), VAccess::READ};
            AstVar* ثابت varp = createDelayedVar(name, alwaysp, prevp,
                                                  initp ? initp->clone_ونه_خالص(خطأ) : تش_اشاره);
            // Add it to delayed variable vector
            delayedr.emplace_شا(varp);
        }
        // Return a reference to the appropriately delayed variable
        أرجع جديد AstVarRef{flp, delayedr.at(ticks - 1), VAccess::READ};
    }

    // Time of the pipeline's last sampling tick, for end-of-simulation readers
    AstVar* get_Past_Tick_وخت_Var(AstAlways* alwaysp) {
        AstVar*& varpr = m_delayedAlways2TickTimep[alwaysp];
        إذا (!varpr) {
            File_کرښه* ثابت flp = alwaysp->fileline();
            varpr = جديد AstVar{flp, VVarType::MODULETEMP,
                               "_Vpast_" + cvt_منزل_Str(m_mod_Past_عدد++) + "_t",
                               m_modp->پیدا_کړه_UInt64_DType()};
            m_modp->جمع_کړه_Stmtsp(varpr);
            Ast_غوټه_بیان* ثابت timep = جديد AstCExpr{flp, "vlSymsp->_vm_contextp__->time()", 64};
            alwaysp->جمع_کړه_Stmtsp(
                جديد Ast_ټاکل{flp, جديد AstVarRef{flp, varpr, VAccess::WRITE}, timep});
        }
        أرجع varpr;
    }

    فراغ لیدنه_Assertion_Iterate(Ast_غوټه_Cover_Or_Assert* nodep, Ast_غوټه* failsp) {
        إذا (m_beginp && nodep->name() == "") nodep->name(m_beginp->name());

        { AssertDeFutureVisitor{nodep->propp(), m_modp, m_mod_Past_عدد++}; }

        iterate_And_بل_Null(nodep->sentreep());
        إذا (AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            iterate_And_بل_Null(assertp->failsp());
        } وإلا إذا (AstAssertIntrinsic* ثابت assertp = VN_CAST(nodep, AssertIntrinsic)) {
            iterate_And_بل_Null(assertp->failsp());
        } وإلا إذا (AstCover* ثابت coverp = VN_CAST(nodep, Cover)) {
            iterate_And_بل_Null(coverp->coverincsp());
        } وإلا إذا (!VN_IS(nodep, Restrict)) {
            nodep->v3fatalSrc("Unhandled assert type");
        }
        iterate_And_بل_Null(nodep->passsp());
        Ast_Sen_ونه* sentreep = nodep->sentreep();
        إذا (nodep->سمدستي()) {
            UASSERT_OBJ(!sentreep, nodep, "Immediate assertions don't have sensitivity");
        } وإلا {
            UASSERT_OBJ(sentreep, nodep, "Concurrent assertions must have sensitivity");
            // Explicit inline clock differs from the enclosing always: hoist
            // and warn. To support this need queue of asserts to activate.
            إذا (m_procedurep && !nodep->senFromAlways()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Procedural concurrent assertion with"
                              " clocking event inside always (IEEE 1800-2023 16.14.6)");
                static_cast<Ast_غوټه*>(m_procedurep)->جمع_کړه_بل(nodep->unlink_Fr_شا());
                أرجع;  // Later iterate will pick up
            }
            sentreep->unlink_Fr_شا();
            إذا (m_procedurep) { VL_DO_DANGLING(ټېل_Deletep(sentreep), sentreep); }
        }
        //
        ثابت string& پیغام = nodep->name();
        Ast_غوټه* passsp = nodep->passsp();
        إذا (passsp) passsp->unlink_Fr_شا_With_بل();
        إذا (AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            failsp = assertp->failsp();
        } وإلا إذا (AstAssertIntrinsic* ثابت assertp = VN_CAST(nodep, AssertIntrinsic)) {
            failsp = assertp->failsp();
        }
        إذا (failsp) failsp->unlink_Fr_شا_With_بل();

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
                selfDestruct = ریښتیا;
            } وإلا {
                // V3Coverage assigned us a bucket to increment.
                AstCoverInc* ثابت covincp = VN_AS(coverp->coverincsp(), CoverInc);
                UASSERT_OBJ(covincp, coverp, "Missing AstCoverInc under assertion");
                covincp->unlink_Fr_شا_With_بل();  // next() might have  AstAssign for trace
                إذا (پیغام != "") covincp->declp()->comment(پیغام);
                إذا (passsp) {
                    passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                               /*vacuous=*/خطأ);
                    passspGated = ریښتیا;
                    passsp = Ast_غوټه::جمع_کړه_بل<Ast_غوټه, Ast_غوټه>(covincp, passsp);
                } وإلا {
                    passsp = covincp;
                }
            }
        } وإلا إذا (VN_IS(nodep, Assert) || VN_IS(nodep, AssertIntrinsic)) {
            إذا (nodep->سمدستي()) {
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

        Ast_غوټه* propExprp;
        Ast_غوټه_بیان* disablep = تش_اشاره;
        Ast_غوټه_بیان* سمون_Countp = تش_اشاره;
        إذا (AstPropSpec* ثابت specp = VN_CAST(nodep->propp(), PropSpec)) {
            propExprp = specp->propp()->unlink_Fr_شا();
            إذا (specp->disablep()) disablep = specp->disablep()->unlink_Fr_شا();
            سمون_Countp = specp->سمون_Countp();
        } وإلا {
            propExprp = nodep->propp()->unlink_Fr_شا();
        }
        File_کرښه* ثابت flp = nodep->fileline();
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
        إذا (coverp && سمون_Countp && passsp) {
            // Convert the match count into a loop that decrements a temporary variable until it
            // reaches zero.
            سمون_Countp->unlink_Fr_شا();
            AstVar* ثابت remainingp = جديد AstVar{
                flp, VVarType::MODULETEMP, m_سمون_شمېر_نومونه.get(""), سمون_Countp->dtypep()};
            remainingp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->جمع_کړه_Stmtsp(remainingp);
            Ast_غوټه* ثابت replaysp
                = جديد Ast_ټاکل{flp, جديد AstVarRef{flp, remainingp, VAccess::WRITE}, سمون_Countp};
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            loopp->جمع_کړه_Stmtsp(
                جديد AstLoopTest{flp, loopp, جديد AstVarRef{flp, remainingp, VAccess::READ}});
            loopp->جمع_کړه_Stmtsp(passsp);
            loopp->جمع_کړه_Stmtsp(
                جديد Ast_ټاکل{flp, جديد AstVarRef{flp, remainingp, VAccess::WRITE},
                              جديد AstSub{flp, جديد AstVarRef{flp, remainingp, VAccess::READ},
                                         جديد AstConst{flp, AstConst::Widthed_ارزښت{},
                                                      remainingp->dtypep()->پلنوالی(), 1}}});
            replaysp->جمع_کړه_بل(loopp);
            passsp = replaysp;
        }
        Ast_غوټه* bodysp = assertBody(nodep, propExprp, passsp, failsp);
        إذا (disablep) bodysp = جديد AstIf{flp, جديد AstLogNot{flp, disablep}, bodysp};
        // Add assertOn check last, for better combining
        إذا (!seqEvent) bodysp = newIfAssertOn(bodysp, nodep->directive(), nodep->userType());
        إذا (sentreep) bodysp = جديد AstAlways{flp, VAlwaysKwd::ALWAYS, sentreep, bodysp};

        إذا (passsp && !passsp->backp()) VL_DO_DANGLING(ټېل_Deletep(passsp), passsp);
        إذا (failsp && !failsp->backp()) VL_DO_DANGLING(ټېل_Deletep(failsp), failsp);

        // Install it
        إذا (selfDestruct) {
            // Delete it after making the tree.  This way we can tell the user
            // if it wasn't constructed nicely or has other errors without needing --coverage.
            VL_DO_DANGLING(bodysp->delete_ونه(), bodysp);
            nodep->unlink_Fr_شا();
        } وإلا {
            nodep->replaceWith(bodysp);
        }
        // Bye
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    منطقي is_تشه_Stmt(Ast_غوټه* nodep) {
        إذا (!nodep) أرجع ریښتیا;
        إذا (AstBegin* ثابت beginp = VN_CAST(nodep, Begin)) {
            إذا (beginp->declsp()) أرجع خطأ;
            لكل (Ast_غوټه* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (!is_تشه_Stmt(stmtp)) أرجع خطأ;
            }
            أرجع ریښتیا;
        }
        أرجع خطأ;
    }

    // VISITORS
    فراغ لیدنه(AstIf* nodep) override {
        إذا (!nodep->user1_سېټ_Once()) {
            إذا (nodep->uniquePragma() || nodep->unique0Pragma()) {
                ثابت Ast_غوټه_If* ifp = nodep;
                Ast_غوټه_بیان* propp = تش_اشاره;
                منطقي hasDefaultElse = خطأ;
                افعل {
                    // If this statement ends with 'else if', then nextIf will point to the
                    // nextIf statement.  Otherwise it will be null.
                    ثابت Ast_غوټه_If* ثابت nextifp = dynamic_cast<Ast_غوټه_If*>(ifp->elsesp());
                    iterate_And_بل_Null(ifp->condp());

                    // Recurse into the true case.
                    iterate_And_بل_Null(ifp->thensp());

                    // If the last else is not an else if, recurse into that too.
                    إذا (ifp->elsesp() && !nextifp) {  //
                        iterate_And_بل_Null(ifp->elsesp());
                    }

                    // Build a bitmask of the true predicates
                    Ast_غوټه_بیان* ثابت predp = ifp->condp()->clone_ونه_خالص(خطأ);
                    إذا (propp) {
                        propp = جديد AstConcat{nodep->fileline(), predp, propp};
                    } وإلا {
                        propp = predp;
                    }

                    // Record if this ends with an 'else' that does not have an if
                    إذا (ifp->elsesp() && !nextifp) hasDefaultElse = ریښتیا;

                    ifp = nextifp;
                } طالما (ifp);

                ثابت منطقي allow_none = nodep->unique0Pragma();

                // Empty case means no property
                إذا (!propp) propp = جديد AstConst{nodep->fileline(), AstConst::بټ_False{}};

                // Note: if this ends with an 'else', then we don't need to validate that one of
                // the predicates evaluates to true.
                Ast_غوټه_بیان* ثابت ohot
                    = ((allow_none || hasDefaultElse)
                           ? static_cast<Ast_غوټه_بیان*>(جديد AstOneHot0{nodep->fileline(), propp})
                           : static_cast<Ast_غوټه_بیان*>(جديد AstOneHot{nodep->fileline(), propp}));
                ثابت VAssertType assertType
                    = nodep->uniquePragma() ? VAssertType::UNIQUE : VAssertType::UNIQUE0;
                // Move, not clone, as the branches might declare referenced variables
                VNRelinker relinker;
                nodep->unlink_Fr_شا(&relinker);
                AstIf* ثابت checkifp
                    = جديد AstIf{nodep->fileline(), جديد AstLogNot{nodep->fileline(), ohot},
                                newFireAssert(nodep, VAssertDirectiveType::VIOLATION_IF,
                                              assertType, "'unique if' statement violated"),
                                nodep};
                checkifp->isBoundsCheck(ریښتیا);  // To avoid LATCH warning
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
                    && backp->condp()->same_ونه(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    backp->جمع_کړه_Thensp(nodep->thensp()->unlink_Fr_شا_With_بل());
                    nodep->unlink_Fr_شا();
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    أرجع;
                }
            }
            // Combine nested assertOn checks if possible
            إذا (nodep->thensp() && !nodep->thensp()->nextp() && is_تشه_Stmt(nodep->elsesp())) {
                AstIf* ثابت checkp = VN_CAST(nodep->thensp(), If);
                إذا (checkp  //
                    && checkp->user2()  //
                    && checkp->condp()->same_ونه(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    nodep->جمع_کړه_Thensp(checkp->thensp()->unlink_Fr_شا_With_بل());
                    VL_DO_DANGLING(ټېل_Deletep(checkp->unlink_Fr_شا()), checkp);
                    أرجع;
                }
            }
            أرجع;
        }

        // Swap assertOn check with single statement 'if' statement to bubble up for combining
        // Note we can't just swap the conditions as they two Ifs have different flags,
        // so swapping the Ifs themeselves then swapping back the bodies.
        إذا (nodep->condp()->is_خالص()) {
            إذا (nodep->thensp() && !nodep->thensp()->nextp() && is_تشه_Stmt(nodep->elsesp())) {
                AstIf* ثابت checkp = VN_CAST(nodep->thensp(), If);
                إذا (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlink_Fr_شا());
                    nodep->جمع_کړه_Thensp(checkp->thensp()->unlink_Fr_شا_With_بل());
                    checkp->جمع_کړه_Thensp(nodep);
                    أرجع;
                }
            }
            إذا (nodep->elsesp() && !nodep->elsesp()->nextp() && is_تشه_Stmt(nodep->thensp())) {
                AstIf* ثابت checkp = VN_CAST(nodep->elsesp(), If);
                إذا (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlink_Fr_شا());
                    nodep->جمع_کړه_Elsesp(checkp->thensp()->unlink_Fr_شا_With_بل());
                    checkp->جمع_کړه_Thensp(nodep);
                    أرجع;
                }
            }
        }
    }

    //========== Case assertions
    فراغ لیدنه(AstCase* nodep) override {
        // Introduce temporary variable for AstCase if needed - it is done here and not in V3Case
        // because this phase is before V3Scope and V3Case is not. Doing it before V3Scope ensures
        // that V3Scope will take care of a scope creation
        // We also need to do it before V3Begin, co that pragmas like `unique0` also work correctly
        إذا (!nodep->exprp()->is_خالص()) {
            ++m_statLiftedCaseExprs;
            File_کرښه* ثابت fl = nodep->exprp()->fileline();
            AstVar* ثابت varp = جديد AstVar{fl, VVarType::BLOCKTEMP, m_case_لنډمهاله_نومونه.get(nodep),
                                            nodep->exprp()->dtypep()};
            Ast_غوټه_بیان* ثابت origp = nodep->exprp()->unlink_Fr_شا();
            nodep->جمع_کړه_دلته_This_As_بل(
                جديد Ast_ټاکل{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, origp});
            nodep->exprp(جديد AstVarRef{fl, varp, VAccess::READ});
            إذا (m_ftaskp) {
                varp->funcLocal(ریښتیا);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } وإلا {
                m_modp->prependStmtsp(varp);
            }
            VIsCached::clear_Cache_ونه();
        }
        iterateChildren(nodep);
        إذا (!nodep->user1_سېټ_Once()) {
            منطقي has_default = خطأ;
            لكل (Ast_Case_توکی* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Case_توکی)) {
                إذا (itemp->isDefault()) has_default = ریښتیا;
            }
            ثابت Ast_غوټه_DType* بیان_Dtypep = nodep->exprp()->dtypep()->skipRefp();

            VAssertType assertType = VAssertType::دننی;
            إذا (nodep->لومړیتوب_Pragma()) {
                assertType = VAssertType::لومړیتوب;
            } وإلا إذا (nodep->uniquePragma()) {
                assertType = VAssertType::UNIQUE;
            } وإلا إذا (nodep->unique0Pragma()) {
                assertType = VAssertType::UNIQUE0;
            }

            string valFmt;
            إذا (بیان_Dtypep->isIntegralOrPacked())
                valFmt = " for '" + cvt_منزل_Str(بیان_Dtypep->پلنوالی_Min()) + "'h%X'";
            إذا (nodep->ډک_Pragma() || nodep->لومړیتوب_Pragma()) {
                // Need to add a default if there isn't one already
                ++m_stat_As_ډک;
                إذا (!has_default) {
                    nodep->جمع_کړه_Itemsp(جديد Ast_Case_توکی{
                        nodep->fileline(), تش_اشاره /*DEFAULT*/,
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      nodep->pragmaString() + ", but non-match found" + valFmt,
                                      valFmt.تشه() ? تش_اشاره
                                                     : nodep->exprp()->clone_ونه_خالص(خطأ))});
                }
            }
            إذا (nodep->parallelPragma() || nodep->uniquePragma() || nodep->unique0Pragma()) {
                // Need to check that one, and only one of the case items match at any moment
                // If there's a default, we allow none to match, else exactly one must match
                ++m_stat_As_ډک;
                إذا (!has_default && !nodep->itemsp()) {
                    // Not parallel, but harmlessly so.
                } وإلا {
                    Ast_غوټه_بیان* propp = تش_اشاره;
                    لكل (Ast_Case_توکی* itemp = nodep->itemsp(); itemp;
                         itemp = VN_AS(itemp->nextp(), Case_توکی)) {
                        Ast_غوټه_بیان* itembitp = تش_اشاره;
                        لكل (Ast_غوټه_بیان* icondp = itemp->condsp(); icondp;
                             icondp = VN_AS(icondp->nextp(), غوټه_بیان)) {
                            Ast_غوټه_بیان* onep;
                            إذا (AstInsideRange* ثابت rcondp = VN_CAST(icondp, InsideRange)) {
                                onep = rcondp->newAndFromInside(
                                    nodep->exprp()->clone_ونه_خالص(ریښتیا),
                                    rcondp->lhsp()->clone_ونه_خالص(ریښتیا),
                                    rcondp->rhsp()->clone_ونه_خالص(ریښتیا));
                            } وإلا إذا (nodep->casex() || nodep->casez() || nodep->caseInside()) {
                                onep = AstEqWild::newTyped(itemp->fileline(),
                                                           nodep->exprp()->clone_ونه_خالص(خطأ),
                                                           icondp->clone_ونه_خالص(خطأ));
                            } وإلا {
                                onep = AstEq::newTyped(icondp->fileline(),
                                                       nodep->exprp()->clone_ونه_خالص(خطأ),
                                                       icondp->clone_ونه_خالص(خطأ));
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
                    إذا (!propp) propp = جديد AstConst{nodep->fileline(), AstConst::بټ_False{}};
                    ثابت منطقي allow_none = has_default || nodep->unique0Pragma();
                    // The following assertion looks as below.
                    // if (!$onehot(propp)) begin
                    //     if (propp == '0) begin if (!allow_none) $error("none match"); end
                    //     else $error("multiple match");
                    //پای_ټکی
                    Ast_غوټه_بیان* ثابت ohot = جديد AstOneHot{nodep->fileline(), propp};
                    AstConst* ثابت zero = جديد AstConst{
                        nodep->fileline(), AstConst::Widthed_ارزښت{}, propp->پلنوالی(), 0};
                    AstIf* ثابت ohotIfp
                        = جديد AstIf{nodep->fileline(), جديد AstLogNot{nodep->fileline(), ohot}};
                    AstIf* ثابت zeroIfp = جديد AstIf{
                        nodep->fileline(),
                        جديد AstEq{nodep->fileline(), propp->clone_ونه_خالص(خطأ), zero}};
                    Ast_غوټه_بیان* ثابت exprp = nodep->exprp();
                    ثابت string pragmaStr = nodep->pragmaString();
                    إذا (!allow_none)
                        zeroIfp->جمع_کړه_Thensp(
                            newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                          pragmaStr + ", but none matched" + valFmt,
                                          valFmt.تشه() ? تش_اشاره : exprp->clone_ونه_خالص(خطأ)));
                    zeroIfp->جمع_کړه_Elsesp(
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      pragmaStr + ", but multiple matches found" + valFmt,
                                      valFmt.تشه() ? تش_اشاره : exprp->clone_ونه_خالص(خطأ)));
                    ohotIfp->جمع_کړه_Thensp(zeroIfp);
                    ohotIfp->isBoundsCheck(ریښتیا);  // To avoid LATCH warning
                    ohotIfp->branchPred(VBranchPred::BP_UNLIKELY);
                    nodep->جمع_کړه_Not_Parallelp(ohotIfp);
                }
            }
        }
    }

    فراغ لیدنه(AstFuture* nodep) override {
        nodep->v3error("Future sampled value function called outside property or sequence "
                       "expression (IEEE 16.9.4)");
        nodep->replaceWith(جديد AstConst{nodep->fileline(), 0});
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    //========== Past
    فراغ لیدنه(AstPast* nodep) override {
        إذا (!nodep->propertyTiming()) {
            nodep->initp(nodep->exprp()->clone_ونه_خالص(خطأ));
            { Default_Sampled_ارزښت_Visitor{nodep->initp()}; }
        }
        Ast_غوټه_بیان* ثابت initp = nodep->initp() ? nodep->initp()->unlink_Fr_شا() : تش_اشاره;
        iterateChildren(nodep);
        uint32_t ticks = 1;
        إذا (nodep->ticksp()) {
            UASSERT_OBJ(VN_IS(nodep->ticksp(), Const), nodep,
                        "Expected constant ticks, checked in V3Width");
            ticks = VN_AS(nodep->ticksp(), Const)->منزل_UInt();
        }
        UASSERT_OBJ(ticks >= 1, nodep, "0 tick should have been checked in V3Width");
        Ast_غوټه_بیان* ثابت exprp = new_Sampled_بیان(nodep->exprp()->unlink_Fr_شا());
        Ast_Sen_ونه* ثابت senTreep = nodep->sentreep()->unlink_Fr_شا();
        Ast_غوټه_بیان* inp = تش_اشاره;
        // Deliberately emitted for every ftask $past; calls outside final take the normal stage
        إذا (VN_IS(m_procedurep, Final) || m_ftaskp) {
            // A sampling-tick finish reads one stage deeper (IEEE 1800-2023 16.9.3).
            File_کرښه* ثابت flp = nodep->fileline();
            AstAlways* ثابت alwaysp
                = getDelayedAlways(exprp, initp ? initp->clone_ونه_خالص(خطأ) : تش_اشاره, senTreep);
            Ast_غوټه_بیان* ثابت betweenTicksp = past_ارزښت_Ref(alwaysp, flp, ticks, initp);
            Ast_غوټه_بیان* ثابت onTickp = past_ارزښت_Ref(alwaysp, flp, ticks + 1, initp);
            إذا (initp) VL_DO_DANGLING(ټېل_Deletep(initp), initp);
            Ast_غوټه_بیان* ثابت onTickCondp = جديد AstLogAnd{
                flp, جديد AstCExpr{flp, "vlSymsp->_vm_contextp__->executingFinal()", 1},
                جديد AstEq{flp, جديد AstVarRef{flp, get_Past_Tick_وخت_Var(alwaysp), VAccess::READ},
                          جديد AstCExpr{flp, "vlSymsp->_vm_contextp__->finishPendingTime()", 64}}};
            AstCond* ثابت condp = جديد AstCond{flp, onTickCondp, onTickp, betweenTicksp};
            condp->dtypeFrom(onTickp);
            inp = condp;
        } وإلا {
            inp = get_Past_ارزښت(exprp, initp, senTreep, ticks);
        }
        nodep->replaceWith(inp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    //========== Move $sampled down to read-only variables
    فراغ لیدنه(AstSampled* nodep) override {
        إذا (nodep->user1()) أرجع;
        VL_RESTORER(m_دننه_Sampled);
        {
            m_دننه_Sampled = ریښتیا;
            iterateChildren(nodep);
        }
        إذا (nodep->exprp()) {
            nodep->replaceWith(nodep->exprp()->unlink_Fr_شا());
        } وإلا {
            nodep->unlink_Fr_شا();
        }
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(Ast_غوټه_Var_Ref* nodep) override {
        إذا (m_دننه_Sampled && !nodep->user2() && !nodep->varp()->is_لنډمهاله()) {
            إذا (!nodep->access().isReadOnly()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Write to variable in sampled expression");
            } وإلا {
                VNRelinker relinkHandle;
                nodep->unlink_Fr_شا(&relinkHandle);
                AstSampled* ثابت newp = new_Sampled_بیان(nodep);
                relinkHandle.relink(newp);
                newp->user1(1);
                v3Global.سېټ_Has_Sampled();
            }
        }
    }
    // Don't sample sensitivities
    فراغ لیدنه(Ast_Sen_توکی* nodep) override {
        VL_RESTORER(m_دننه_Sampled);
        m_دننه_Sampled = خطأ;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstPExprClause* nodep) override {
        Ast_غوټه* stmtsp = تش_اشاره;
        إذا (nodep->pass() && m_passsp) {
            // Cover adds COVERINC by AstNode::addNext, thus need to clone next too.
            stmtsp = m_passsp->clone_ونه(ریښتیا);
            stmtsp = newIfAssertPassOn(stmtsp, m_assertp->directive(), m_assertp->userType(),
                                       nodep->vacuous());
        } وإلا إذا (!nodep->pass() && m_failsp) {
            stmtsp = m_failsp->clone_ونه(ریښتیا);
            stmtsp = newIfAssertFailOn(stmtsp, m_assertp->directive(), m_assertp->userType());
        }
        إذا (stmtsp) {
            stmtsp->foreach_And_بل([](Ast_غوټه_Var_Ref* ثابت refp) {
                // References inside action blocks shouldn't be implicitly sampled
                // m_passsp/m_failsp have been already visited once and refs explicitly sampled
                // are handled already
                refp->user2(1);
            });
            nodep->replaceWith(stmtsp);
        } وإلا {
            nodep->unlink_Fr_شا();
        }
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstPExpr* nodep) override {
        إذا (m_دننه_Restrict) {
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
        } وإلا {
            iterateChildren(nodep);
        }
    }

    //========== Statements
    فراغ لیدنه(AstDisplay* nodep) override {
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
            File_کرښه* ثابت fl = nodep->fileline();

            Ast_Sen_توکی* monSenItemsp = تش_اشاره;
            إذا (Ast_غوټه* ثابت monExprsp = nodep->fmtp()->exprsp()) {
                monExprsp->foreach_And_بل([&](AstVarRef* varrefp) {
                    Ast_Sen_توکی* ثابت senItemp
                        = جديد Ast_Sen_توکی{fl, VEdgeType::ET_بدل_شوی,
                                         // Clone so get VarRef or VarXRef as needed
                                         varrefp->clone_ونه(خطأ)};
                    monSenItemsp = Ast_غوټه::جمع_کړه_بل_Null(monSenItemsp, senItemp);
                });
            }

            Ast_Sen_ونه* ثابت mon_Sen_ونه = جديد Ast_Sen_ونه{fl, monSenItemsp};
            ثابت خپلکاری mon_عدد = ++m_monitor_عدد;
            // Where $monitor was we do "__VmonitorNum = N;"
            Ast_ټاکل* ثابت newsetp = جديد Ast_ټاکل{
                fl, new_Monitor_عدد_Var_Refp(nodep, VAccess::WRITE), جديد AstConst{fl, mon_عدد}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__VmonitorOn && __VmonitorNum==N) $display(...);"
            Ast_غوټه* ثابت stmtsp = nodep;
            AstIf* ثابت ifp = جديد AstIf{
                fl,
                جديد AstLogAnd{fl, جديد AstLogNot{fl, newMonitorOffVarRefp(nodep, VAccess::READ)},
                              جديد AstEq{fl, جديد AstConst{fl, mon_عدد},
                                        new_Monitor_عدد_Var_Refp(nodep, VAccess::READ)}},
                stmtsp};
            ifp->isBoundsCheck(ریښتیا);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_غوټه* ثابت newp = جديد AstAlways{fl, VAlwaysKwd::ALWAYS, mon_Sen_ونه, ifp};
            m_modp->جمع_کړه_Stmtsp(newp);
        } وإلا إذا (nodep->displayType() == VDisplayType::DT_STROBE) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            // Need one-shot
            File_کرښه* ثابت fl = nodep->fileline();
            AstVar* ثابت varp
                = جديد AstVar{fl, VVarType::MODULETEMP, "__Vstrobe" + cvt_منزل_Str(m_mod_Strobe_عدد++),
                             nodep->پیدا_کړه_بټ_DType()};
            m_modp->جمع_کړه_Stmtsp(varp);
            // Where $strobe was we do "__Vstrobe = '1;"
            Ast_ټاکل* ثابت newsetp = جديد Ast_ټاکل{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                                     جديد AstConst{fl, AstConst::بټ_True{}}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__Vstrobe) begin $display(...); __Vstrobe = '0; end"
            Ast_غوټه* ثابت stmtsp = nodep;
            AstIf* ثابت ifp = جديد AstIf{fl, جديد AstVarRef{fl, varp, VAccess::READ}, stmtsp};
            ifp->isBoundsCheck(ریښتیا);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_غوټه* ثابت newp = جديد AstAlwaysPostponed{fl, ifp};
            stmtsp->جمع_کړه_بل(جديد Ast_ټاکل{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                          جديد AstConst{fl, AstConst::بټ_False{}}});
            m_modp->جمع_کړه_Stmtsp(newp);
        }
    }
    فراغ لیدنه(AstMonitorOff* nodep) override {
        Ast_ټاکل* ثابت newp
            = جديد Ast_ټاکل{nodep->fileline(), newMonitorOffVarRefp(nodep, VAccess::WRITE),
                            جديد AstConst{nodep->fileline(), AstConst::بټ_True{}, nodep->off()}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstAssert* nodep) override {  //
        لیدنه_Assertion_Iterate(nodep, nodep->failsp());
    }
    فراغ لیدنه(AstAssertCtl* nodep) override {
        iterateChildren(nodep);

        منطقي assertTypeConst = ریښتیا;
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
            controlTypeConst = ریښتیا;
        }
        إذا (controlTypeConst
            && (nodep->ctlType() < VAssertCtlType::قلف
                || nodep->ctlType() > VAssertCtlType::VACUOUS_OFF)) {
            nodep->unlink_Fr_شا();
            nodep->v3error("Bad $assertcontrol control_type '"
                           << cvt_منزل_Str(static_cast<صحيح>(nodep->ctlType()))
                           << "' (IEEE 1800-2023 Table 20-5)");
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        }
        إذا (assertTypeConst && nodep->assertTypes() != ALL_ASSERT_TYPES
            && nodep->assertTypes().لري_Any(VAssertType::UNIQUE | VAssertType::UNIQUE0
                                                | VAssertType::لومړیتوب)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: assert control assertion_type");
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            أرجع;
        }

        منطقي directiveTypeConst = ریښتیا;
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
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            أرجع;
        }

        File_کرښه* ثابت fl = nodep->fileline();
        UINFO(9, "Generating assertctl for a module: " << m_modp);
        AstCStmt* ثابت newp = جديد AstCStmt{fl};
        newp->جمع_کړه("vlSymsp->_vm_contextp__->assertCtl(");
        newp->جمع_کړه(nodep->controlTypep()->unlink_Fr_شا());
        newp->جمع_کړه(", ");
        إذا (nodep->assertTypesp()) {
            newp->جمع_کړه(nodep->assertTypesp()->unlink_Fr_شا());
        } وإلا {
            newp->جمع_کړه(std::منزل_string(ALL_ASSERT_TYPES));
        }
        newp->جمع_کړه(", " + std::منزل_string(nodep->directiveTypes()) + ");\n");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstAssertIntrinsic* nodep) override {  //
        لیدنه_Assertion_Iterate(nodep, nodep->failsp());
    }
    فراغ لیدنه(AstCover* nodep) override {  //
        لیدنه_Assertion_Iterate(nodep, تش_اشاره);
    }
    فراغ لیدنه(AstRestrict* nodep) override {
        VL_RESTORER(m_دننه_Restrict);
        m_دننه_Restrict = ریښتیا;
        iterateChildren(nodep);
        // IEEE says simulator ignores these
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }

    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Past_عدد);
        VL_RESTORER(m_mod_Strobe_عدد);
        VL_RESTORER(m_finalp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DelayedAlwaysp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DefaultDelayedAlwaysp);
        VL_RESTORER_CLEAR(m_delayedAlways2TickTimep);
        m_modp = nodep;
        m_mod_Past_عدد = 0;
        m_mod_Strobe_عدد = 0;
        m_finalp = تش_اشاره;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_Gen_بلاک* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstBegin* nodep) override {
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
                    nodep->replaceWith(ifp->unlink_Fr_شا());
                    nodep->جمع_کړه_Stmtsp(ifp->thensp()->unlink_Fr_شا_With_بل());
                    ifp->جمع_کړه_Thensp(nodep);
                }
            }
        }
    }

    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند AssertVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~AssertVisitor() override {
        V3Stats::جمع_کړه_Stat("Assertions, assert non-immediate statements", m_statAsNotImm);
        V3Stats::جمع_کړه_Stat("Assertions, assert immediate statements", m_statAsImm);
        V3Stats::جمع_کړه_Stat("Assertions, cover statements", m_statCover);
        V3Stats::جمع_کړه_Stat("Assertions, full/parallel case", m_stat_As_ډک);
        V3Stats::جمع_کړه_Stat("Assertions, $past variables", m_statPastVars);
        V3Stats::جمع_کړه_Stat("Assertions, assertOn checks combined", m_statAssertOnCombined);
        V3Stats::جمع_کړه_Stat("Assertions, assertOn checks hoisted", m_statAssertOnHoisted);
        V3Stats::جمع_کړه_Stat("Assertions, lifted impure case expressions", m_statLiftedCaseExprs);
        // Rewrites can change purity, e.g. by compiling out assertion statements with --no-assert
        VIsCached::clear_Cache_ونه();
    }
};

//######################################################################
// Top Assert class

فراغ V3Assert::assertAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("assert", 0, dump_ونه_Either_Level() >= 3);
}
