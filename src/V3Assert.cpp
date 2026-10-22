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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Assert.h"

#شامل "V3AstUserAllocator.h"
#شامل "V3Const.h"
#شامل "V3Stats.h"
#شامل "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

فضاینام {

رده DefaultDisableLocalVisitor final : عمومی VNVisitor {
    // STATE
    Ast_گره* m_scopep = nullptr;

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(nullptr);
        iterateChildren(nodep);
    }
    تهی visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(nullptr);
        iterateChildren(nodep);
    }
    تهی visit(AstDefaultDisable* nodep) override {
        UASSERT_OBJ(nodep, m_scopep,
                    "default disable iff must be inside a module or generate block");
        AstDefaultDisable* defaultp = nullptr;
        اگر (ثابت Ast_گره_Module* ثابت modp = VN_CAST(m_scopep, گره_Module)) {
            defaultp = modp->defaultDisablep();
        } وگرنه {
            defaultp = VN_AS(m_scopep, GenBlock)->defaultDisablep();
        }
        اگر (VL_UNLIKELY(defaultp)) {
            nodep->v3error("Only one 'default disable iff' allowed per "
                           << (VN_IS(m_scopep, گره_Module) ? "module" : "generate block")
                           << " (IEEE 1800-2023 16.15)");
        } وگرنه اگر (Ast_گره_Module* ثابت modp = VN_CAST(m_scopep, گره_Module)) {
            modp->defaultDisablep(nodep);
        } وگرنه {
            VN_AS(m_scopep, GenBlock)->defaultDisablep(nodep);
        }
    }
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    explicit DefaultDisableLocalVisitor(AstNetlist* nodep) { iterate(nodep); }
};

رده DefaultDisablePropagateVisitor final : عمومی VNVisitor {
    // STATE
    AstDefaultDisable* m_defaultDisablep = nullptr;

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    تهی visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        اگر (!nodep->defaultDisablep()) nodep->defaultDisablep(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    explicit DefaultDisablePropagateVisitor(AstNetlist* nodep) { iterate(nodep); }
};

// Lower a sequence used as an event control ('@seq', IEEE 1800-2023 9.4.2.4) into a
// synthesized event fired by an internal 'cover sequence' on each end-of-match
رده SeqEventLowerVisitor final : عمومی VNVisitor {
    // STATE
    Ast_گره_Module* m_modp = nullptr;  // Current module
    V3UniqueNames m_names{"__VseqEvent"};  // Synthesized event names

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstSenItem* nodep) override {
        AstFuncRef* ثابت funcrefp = VN_CAST(nodep->sensp(), FuncRef);
        اگر (funcrefp && VN_IS(funcrefp->taskp(), Sequence)) {
            File_سطر* ثابت flp = nodep->fileline();
            AstVar* ثابت eventp = جدید AstVar{flp, VVarType::MODULETEMP, m_names.get(nodep),
                                              m_modp->findBasicDType(VBasicDTypeKwd::EVENT)};
            eventp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(eventp);
            v3Global.setHasEvents();
            funcrefp->unlinkFrBack();
            nodep->sensp(جدید AstVarRef{flp, eventp, VAccess::READ});
            ثابت منطقی automaticActual = funcrefp->exists([](ثابت Ast_گره_Var_Ref* refp) {
                بازگشت refp->varp() && refp->varp()->lifetime().isAutomatic();
            });
            اگر (automaticActual) {
                nodep->v3error("Arguments to a sequence used as an event control must be"
                               " static (IEEE 1800-2023 9.4.2.4)");
                VN_AS(funcrefp->taskp(), Sequence)->isReferenced(نادرست);
                VL_DO_DANGLING(افزودن_Deletep(funcrefp), funcrefp);
                بازگشت;
            }
            AstFireEvent* ثابت firep
                = جدید AstFireEvent{flp, جدید AstVarRef{flp, eventp, VAccess::WRITE}, نادرست};
            AstCover* ثابت coverp
                = جدید AstCover{flp, جدید AstPropSpec{flp, nullptr, nullptr, funcrefp}, firep,
                               VAssertType::CONCURRENT};
            coverp->isCoverSeq(درست);
            coverp->isSeqEvent(درست);
            m_modp->addStmtsp(coverp);
            بازگشت;
        }
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    explicit SeqEventLowerVisitor(AstNetlist* nodep) { iterate(nodep); }
};

}  // namespace

تهی V3AssertCommon::collectDefaultDisable(AstNetlist* nodep) {
    { DefaultDisableLocalVisitor{nodep}; }
    { DefaultDisablePropagateVisitor{nodep}; }
}

تهی V3AssertCommon::lowerSequenceEvents(AstNetlist* nodep) {
    { SeqEventLowerVisitor{nodep}; }
    V3Global::dump_Check_Global_درخت("assertseqevent", 0, dump_درخت_Either_Level() >= 3);
}

string V3AssertCommon::assertCtlGetCall(ثابت نویسه* query, VAssertType type,
                                        VAssertDirectiveType directiveType) {
    // Template arguments let the runtime build the type/directive mask at C++ compile time
    بازگشت "vlSymsp->_vm_contextp__->assertCtlGet<"s + std::to_string(type) + ", "s
           + std::to_string(directiveType) + ">(VerilatedAssertCtlQuery::"s + query + ")"s;
}

منطقی V3AssertCommon::check_Sva_مقدار_Limit(Ast_گره* nodep, ثابت AstConst* constp, ثابت نویسه* what) {
    // Temporal values can materialize as O(N) AST or state in later passes.
    // Leave headroom for a range ring's extra slot and V3Number's signed
    // `(bits + 31) / 32` word count.
    ایستا constexpr unsigned SVA_مقدار_HARD_LIMIT
        = static_cast<unsigned>(std::numeric_limits<صحیح>::max()) - 32U;
    ثابت unsigned configuredLimit = static_cast<unsigned>(v3Global.opt.max_رقم_Width());
    ثابت unsigned limit = std::min(configuredLimit, SVA_مقدار_HARD_LIMIT);
    اگر (constp->رقم().fitsInUInt() && constp->toUInt() <= limit) بازگشت درست;
    nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << what << " exceeds implementation limit of "
                                                 << limit
                                                 << (configuredLimit <= SVA_مقدار_HARD_LIMIT
                                                         ? " (--max-num-width)"
                                                         : " (host arithmetic limit)"));
    بازگشت نادرست;
}

// The bound of a cycle delay in a sequence, if a non-negative elaboration-time constant
ایستا ثابت AstConst* checkSvaDelayBound(Ast_گره* errp, Ast_گره_عبارت* boundp, ثابت نویسه* what) {
    ثابت AstConst* ثابت constp = VN_CAST(boundp, Const);
    اگر (!constp || (constp->dtypep()->isSigned() && constp->رقم().isNegative())) {
        errp->v3error(what << " is not a non-negative elaboration-time constant"
                              " (IEEE 1800-2023 16.7)");
        بازگشت nullptr;
    }
    اگر (!V3AssertCommon::check_Sva_مقدار_Limit(errp, constp, "SVA cycle delay")) بازگشت nullptr;
    بازگشت constp;
}

// Fold the bounds of a cycle delay in a sequence, and return if they are valid, else report it
// on errp
ایستا منطقی svaCycleDelayValid(ثابت AstDelay* nodep, Ast_گره* errp) {
    V3Const::constifyParamsNoWarnEdit(nodep->lhsp());
    ثابت AstConst* ثابت minConstp = checkSvaDelayBound(
        errp, nodep->lhsp(), nodep->isRangeDelay() ? "Range delay minimum" : "Delay value");
    اگر (!nodep->rhsp() || nodep->isUnbounded()) بازگشت minConstp != nullptr;
    V3Const::constifyParamsNoWarnEdit(nodep->rhsp());
    ثابت AstConst* ثابت maxConstp
        = checkSvaDelayBound(errp, nodep->rhsp(), "Range delay maximum");
    اگر (!minConstp || !maxConstp) بازگشت نادرست;
    اگر (maxConstp->toUInt() < minConstp->toUInt()) {
        errp->v3error("Range delay maximum must be >= minimum (IEEE 1800-2023 16.7)");
        بازگشت نادرست;
    }
    بازگشت درست;
}

تهی V3AssertCommon::checkSvaCycleDelay(AstDelay* nodep, Ast_گره* errp) {
    اگر (svaCycleDelayValid(nodep, errp)) بازگشت;
    // Make the delay valid, so later steps do not fail after the error
    File_سطر* ثابت flp = nodep->fileline();
    Ast_گره_عبارت* ثابت minp = nodep->lhsp();
    minp->replaceWith(جدید AstConst{flp, AstConst::BitFalseErroring{}});
    VL_DO_DANGLING(minp->delete_درخت(), minp);
    اگر (nodep->rhsp() && !nodep->isUnbounded()) {
        Ast_گره_عبارت* ثابت maxp = nodep->rhsp();
        maxp->replaceWith(جدید AstConst{flp, AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(maxp->delete_درخت(), maxp);
    }
}

منطقی V3AssertCommon::refsSvaFormal(ثابت Ast_گره* nodep) {
    بازگشت nodep->exists([](ثابت Ast_گره_Var_Ref* refp) {
        ثابت AstVar* ثابت varp = refp->varp();
        اگر (!varp || !varp->isIO()) بازگشت نادرست;
        ثابت Ast_گره* ثابت declp = varp->aboveLoopp();
        بازگشت VN_IS(declp, Sequence) || VN_IS(declp, Property);
    });
}

تهی V3AssertCommon::checkSvaInstanceDelays(Ast_گره* nodep, Ast_گره* instp) {
    // A delay still referencing a formal argument of an enclosing sequence or property is
    // checked when that is instanced. Collect first, as checking edits the delays.
    std::vector<AstDelay*> delayps;
    nodep->foreach([&](AstDelay* delayp) {
        اگر (!delayp->isCycleDelay() || refsSvaFormal(delayp->lhsp())) بازگشت;
        اگر (delayp->rhsp() && refsSvaFormal(delayp->rhsp())) بازگشت;
        delayps.افزودن_back(delayp);
    });
    برای (AstDelay* ثابت delayp : delayps) checkSvaCycleDelay(delayp, instp);
}

//######################################################################
// AssertDeFutureVisitor
// If any AstFuture, then move all non-future varrefs to be one cycle behind,
// see IEEE 1800-2023 16.9.4.

رده AssertDeFutureVisitor final : عمومی VNVisitor {
    // STATE - across all visitors
    Ast_گره_Module* ثابت m_modp;  // Module future is underneath
    ثابت AstFuture* m_futurep;  // First AstFuture found
    ثابت unsigned m_past_رقم;  // Prefix unique number for this module
    std::map<AstVar*, AstVar*> m_delayedVars;  // Old to delayed variable mapping
    // STATE - for current visit position (use VL_RESTORER)
    منطقی m_inFuture = نادرست;  // Inside a future
    منطقی m_unsupported = نادرست;  // Printed unsupported

    // METHODS
    تهی unsupported(Ast_گره* nodep) {
        اگر (m_unsupported) بازگشت;
        m_unsupported = درست;
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported/illegal: Future value function used with expression with "
                          << nodep->prettyOperatorName());
    }
    // VISITORS
    تهی visit(AstFuture* nodep) override {
        VL_RESTORER(m_inFuture);
        m_inFuture = درست;
        iterateChildren(nodep);
        // Done with the future, this subexpression is current-time
        nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(Ast_گره_Var_Ref* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;
        اگر (m_inFuture || m_unsupported)
            بازگشت;  // Need user1 set above, don't process when Future is removed
        اگر (nodep->access().isWriteOrRW()) {
            unsupported(nodep);
            بازگشت;
        }
        auto it = m_delayedVars.find(nodep->varp());
        AstVar* outvarp;
        اگر (it == m_delayedVars.end()) {
            Ast_Sen_درخت* ثابت sentreep = m_futurep->sentreep();
            AstAlways* ثابت alwaysp = جدید AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS,
                                                     sentreep->clone_درخت(نادرست), nullptr};
            m_modp->addStmtsp(alwaysp);
            outvarp = جدید AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                 "__Vnotfuture" + cvtToStr(m_past_رقم) + "_" + nodep->name(),
                                 nodep->dtypep()};
            m_modp->addStmtsp(outvarp);
            AstVarRef* varRefAWritep = جدید AstVarRef{nodep->fileline(), outvarp, VAccess::WRITE};
            varRefAWritep->user1(درست);
            Ast_گره_Var_Ref* varRefAReadp = nodep->clone_درخت(نادرست);
            varRefAReadp->user1(درست);
            Ast_گره* ثابت assp = جدید AstAssignDly{nodep->fileline(), varRefAWritep, varRefAReadp};
            alwaysp->addStmtsp(assp);
            m_delayedVars.emplace(nodep->varp(), outvarp);
        } وگرنه {
            outvarp = it->second;
        }
        AstVarRef* newp = جدید AstVarRef{nodep->fileline(), outvarp, VAccess::READ};
        newp->user1(درست);
        UINFO(9, "DeFuture " << nodep << "  becomes " << newp);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(Ast_گره_FTask_Ref* nodep) override { unsupported(nodep); }
    تهی visit(AstMethodCall* nodep) override { unsupported(nodep); }
    تهی visit(Ast_گره* nodep) override {
        اگر (!nodep->isPure()) unsupported(nodep);
        iterateChildren(nodep);
    }

عمومی:
    // CONSTRUCTORS
    explicit AssertDeFutureVisitor(Ast_گره* nodep, Ast_گره_Module* modp, unsigned past_رقم)
        : m_modp{modp}
        , m_past_رقم{past_رقم} {
        // See if any Future before we process
        اگر (nodep->forall([&](ثابت AstFuture* futurep) -> منطقی {
                m_futurep = futurep;
                بازگشت نادرست;
            }))
            بازگشت;
        // UINFOTREE(9, nodep, "", "defuture-in");
        visit(nodep);  // Nodep may get deleted
        // UINFOTREE(9, nodep, "", "defuture-ou");
    }
    ~AssertDeFutureVisitor() = پیشفرض;
};

// Prepare one default sampled expression.
رده Default_Sampled_مقدار_Visitor final : عمومی VNVisitor {
    // METHODS
    ایستا AstConst* newTypeDefault(Ast_گره_عبارت* nodep) {
        AstConst* ثابت constp
            = جدید AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()};
        اگر (nodep->dtypep()->isFourstate()) constp->رقم().setAllBitsX();
        constp->dtypeFrom(nodep);
        بازگشت constp;
    }

    // VISITORS
    تهی visit(AstCMethodHard* nodep) override {
        اگر (nodep->method() == VCMethod::EVENT_IS_TRIGGERED) {
            nodep->replaceWith(جدید AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وگرنه {
            iterateChildren(nodep);
        }
    }
    تهی visit(Ast_گره_Var_Ref* nodep) override {
        AstVar* ثابت varp = nodep->varp();
        اگر (varp->isParam()
            || (!varp->isNet() && varp->lifetime().isStatic() && varp->hasUserInit())) {
            بازگشت;
        }
        nodep->replaceWith(newTypeDefault(nodep));
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    explicit Default_Sampled_مقدار_Visitor(Ast_گره* nodep) { iterate(nodep); }
};

//######################################################################
// AssertVisitor

رده AssertVisitor final : عمومی VNVisitor {
    // CONSTANTS
    ایستا constexpr uint8_t ALL_ASSERT_TYPES
        = std::numeric_limits<std::underlying_type<VAssertType::en>::type>::max();

    // NODE STATE/TYPES
    // Cleared on netlist
    //  AstNode::user1()         -> bool.  True if processed
    //  AstAlways::user2p()      -> std::vector<AstVar*>. Delayed variables via 'm_delayed'
    //  AstNodeVarRef::user2()   -> bool.  True if shouldn't be sampled
    //  AstIf::user2()           -> bool.  True for assertOn() checks inserted in this pass
    ثابت VNUser1InUse m_user1InUse;
    ثابت VNUser2InUse m_user2InUse;
    AstUser2Allocator<AstAlways, std::vector<AstVar*>> m_delayed;

    // STATE
    Ast_گره_Module* m_modp = nullptr;  // Last module
    ثابت Ast_گره* m_beginp = nullptr;  // Last AstBegin/AstGenBlock
    unsigned m_monitor_رقم = 0;  // Global $monitor numbering (not per module)
    AstVar* m_monitor_رقم_Varp = nullptr;  // $monitor number variable
    AstVar* m_monitorOffVarp = nullptr;  // $monitoroff variable
    unsigned m_mod_Past_رقم = 0;  // Module past numbering
    unsigned m_mod_Strobe_رقم = 0;  // Module $strobe numbering
    Ast_گره_Procedure* m_procedurep = nullptr;  // Current procedure
    VDouble0 m_statCover;  // Statistic tracking
    VDouble0 m_statAsNotImm;  // Statistic tracking
    VDouble0 m_statAsImm;  // Statistic tracking
    VDouble0 m_statAsFull;  // Statistic tracking
    VDouble0 m_statPastVars;  // Statistic tracking
    VDouble0 m_statAssertOnCombined;  // Statistic tracking
    VDouble0 m_statAssertOnHoisted;  // Statistic tracking
    منطقی m_inSampled = نادرست;  // True inside a sampled expression
    منطقی m_inRestrict = نادرست;  // True inside restrict assertion
    Ast_گره* m_passsp = nullptr;  // Current pass statement
    Ast_گره* m_failsp = nullptr;  // Current fail statement
    Ast_گره_Cover_Or_Assert* m_assertp = nullptr;  // Current assertion
    AstFinal* m_finalp = nullptr;  // Current final block
    VDouble0 m_statLiftedCaseExprs;  // Count of purified case expressions
    Ast_گره_FTask* m_ftaskp = nullptr;  // Current function/task
    V3UniqueNames m_caseTempNames{"__VCase"};
    V3UniqueNames m_match_شمار_Names{"__VnfaRemainingMatchCount"};  // Match replay counter names
    // Maps from (expression, senTree) to the AstAlways that computes its delayed values.
    std::unordered_map<VNRef<Ast_گره_عبارت>, std::unordered_map<VNRef<Ast_Sen_درخت>, AstAlways*>>
        m_modExpr2Sen2DelayedAlwaysp;
    // Source sampled-value history is initialized, unlike internal property-lowering history.
    std::unordered_map<VNRef<Ast_گره_عبارت>, std::unordered_map<VNRef<Ast_Sen_درخت>, AstAlways*>>
        m_modExpr2Sen2DefaultDelayedAlwaysp;
    // Map from delayed-value AstAlways to its last-sampling-tick time variable
    std::unordered_map<ثابت AstAlways*, AstVar*> m_delayedAlways2TickTimep;

    // METHODS
    ایستا ثابت نویسه* assertPassOnQuery(منطقی vacuous) {
        ایستا constexpr ثابت نویسه* queries[2]
            = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
        بازگشت queries[vacuous];
    }
    ایستا Ast_گره_عبارت* assertOnCond(File_سطر* fl, VAssertType type,
                                     VAssertDirectiveType directiveType) {
        // cppcheck-suppress missingReturn
        انتخاب (directiveType) {
        حالت VAssertDirectiveType::INTRINSIC: بازگشت جدید AstConst{fl, AstConst::BitTrue{}};
        حالت VAssertDirectiveType::VIOLATION_CASE: {
            اگر (v3Global.opt.assertCase()) {
                بازگشت جدید AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            // If assertions are off, have constant propagation rip them out later
            // This allows syntax errors and such to be detected normally.
            بازگشت جدید AstConst{fl, AstConst::BitFalse{}};
        }
        حالت VAssertDirectiveType::ASSERT:
        حالت VAssertDirectiveType::COVER:
        حالت VAssertDirectiveType::ASSUME: {
            اگر (v3Global.opt.assertOn()) {
                بازگشت جدید AstCExpr{
                    fl, AstCExpr::Pure{},
                    V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
            }
            بازگشت جدید AstConst{fl, AstConst::BitFalse{}};
        }
        حالت VAssertDirectiveType::INTERNAL:
        حالت VAssertDirectiveType::VIOLATION_IF:
        حالت VAssertDirectiveType::RESTRICT: {
            اگر (v3Global.opt.assertOn()) {
                بازگشت جدید AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            بازگشت جدید AstConst{fl, AstConst::BitFalse{}};
        }
        }
        VL_UNREACHABLE;
    }

    ایستا منطقی isControlled(VAssertDirectiveType directiveType) {
        بازگشت (static_cast<صحیح>(directiveType)
                & (static_cast<صحیح>(VAssertDirectiveType::ASSERT)
                   | static_cast<صحیح>(VAssertDirectiveType::COVER)
                   | static_cast<صحیح>(VAssertDirectiveType::ASSUME)));
    }
    ایستا Ast_گره_عبارت* assertPassOnCond(File_سطر* fl, VAssertType type,
                                         VAssertDirectiveType directiveType, منطقی vacuous) {
        اگر (!isControlled(directiveType)) بازگشت جدید AstConst{fl, AstConst::BitTrue{}};
        اگر (!v3Global.opt.assertOn()) بازگشت جدید AstConst{fl, AstConst::BitFalse{}};
        بازگشت جدید AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType), 1};
    }
    ایستا Ast_گره_عبارت* assertFailOnCond(File_سطر* fl, VAssertType type,
                                         VAssertDirectiveType directiveType) {
        اگر (!isControlled(directiveType)) بازگشت جدید AstConst{fl, AstConst::BitTrue{}};
        اگر (!v3Global.opt.assertOn()) بازگشت جدید AstConst{fl, AstConst::BitFalse{}};
        بازگشت جدید AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType), 1};
    }
    string assertDisplayMessage(ثابت Ast_گره* nodep, ثابت string& prefix, ثابت string& message,
                                VDisplayType severity) {
        اگر (severity == VDisplayType::DT_ERROR || severity == VDisplayType::DT_FATAL) {
            بازگشت ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                    + cvtToStr(nodep->fileline()->lineno()) + ": Assertion failed in %m"
                    + ((message != "") ? ": " : "") + message + "\n");
        }
        بازگشت ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                + cvtToStr(nodep->fileline()->lineno()) + ": %m" + ((message != "") ? ": " : "")
                + message + "\n");
    }
    تهی replaceDisplay(AstDisplay* nodep, ثابت string& prefix) {
        nodep->fmtp()->text(
            assertDisplayMessage(nodep, prefix, nodep->fmtp()->text(), nodep->displayType()));
        nodep->displayType(VDisplayType::DT_WRITE);
        // cppcheck-suppress nullPointer
        Ast_گره_عبارت* ثابت timenewp = جدید AstTime{nodep->fileline(), m_modp->timeunit()};
        nodep->fmtp()->prependExprsp(timenewp);
        اگر (!nodep->fmtp()->scopeNamep() && nodep->fmtp()->formatScopeTracking()) {
            nodep->fmtp()->scopeNamep(جدید AstScopeName{nodep->fileline(), درست});
        }
    }
    AstSampled* new_Sampled_عبارت(Ast_گره_عبارت* nodep) {
        بازگشت جدید AstSampled{nodep->fileline(), nodep, nodep->dtypep(), درست};
    }
    AstVarRef* new_Monitor_رقم_Var_Refp(ثابت Ast_گره* nodep, VAccess access) {
        اگر (!m_monitor_رقم_Varp) {
            m_monitor_رقم_Varp = جدید AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorNum",
                                          nodep->findUInt64DType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitor_رقم_Varp);
        }
        AstVarRef* ثابت varrefp = جدید AstVarRef{nodep->fileline(), m_monitor_رقم_Varp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        بازگشت varrefp;
    }
    AstVarRef* newMonitorOffVarRefp(ثابت Ast_گره* nodep, VAccess access) {
        اگر (!m_monitorOffVarp) {
            m_monitorOffVarp = جدید AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorOff",
                                          nodep->findBitDType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitorOffVarp);
        }
        AstVarRef* ثابت varrefp = جدید AstVarRef{nodep->fileline(), m_monitorOffVarp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        بازگشت varrefp;
    }
    ایستا AstIf* newIfAssertOn(Ast_گره* bodyp, VAssertDirectiveType directiveType,
                                VAssertType type = VAssertType::INTERNAL) {
        // Add an internal if to check assertions are on.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_سطر* ثابت fl = bodyp->fileline();

        Ast_گره_عبارت* ثابت condp = assertOnCond(fl, type, directiveType);
        AstIf* ثابت newp = جدید AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(درست);  // To avoid LATCH warning
        newp->user2(درست);  // Mark as an assertOn() check
        بازگشت newp;
    }
    ایستا Ast_گره_Stmt* newIfAssertPassOn(Ast_گره* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type, منطقی vacuous) {
        // Add an internal if to check assertion passOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_سطر* ثابت fl = bodyp->fileline();
        Ast_گره_عبارت* ثابت condp = assertPassOnCond(fl, type, directiveType, vacuous);
        Ast_گره_If* ثابت newp = جدید AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(درست);  // To avoid LATCH warning
        newp->user1(درست);  // Don't assert/cover this if
        newp->user2(درست);  // Mark as an assertOn() check
        بازگشت newp;
    }
    ایستا Ast_گره_Stmt* newIfAssertFailOn(Ast_گره* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type) {
        // Add an internal if to check assertion failOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_سطر* ثابت fl = bodyp->fileline();
        Ast_گره_عبارت* ثابت condp = assertFailOnCond(fl, type, directiveType);
        Ast_گره_If* ثابت newp = جدید AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(درست);  // To avoid LATCH warning
        newp->user1(درست);  // Don't assert/cover this if
        newp->user2(درست);  // Mark as an assertOn() check
        بازگشت newp;
    }

    ایستا AstIf* assertCond(ثابت Ast_گره_Cover_Or_Assert* nodep, Ast_گره_عبارت* propp,
                             Ast_گره* passsp, Ast_گره* failsp) {
        AstIf* ثابت ifp = جدید AstIf{nodep->fileline(), propp, passsp, failsp};
        // It's more LIKELY that we'll take the nullptr if clause
        // than the sim-killing else clause:
        ifp->branchPred(VBranchPred::BP_LIKELY);
        ifp->isBoundsCheck(درست);  // To avoid LATCH warning
        بازگشت ifp;
    }

    Ast_گره_Stmt* assertBody(ثابت Ast_گره_Cover_Or_Assert* nodep, Ast_گره* propp, Ast_گره* passsp,
                            Ast_گره* failsp) {
        اگر (AstPExpr* ثابت pexprp = VN_CAST(propp, PExpr)) {
            اگر (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NOTIMING, "This property expression requires --timing");
                VL_DO_DANGLING(افزودن_Deletep(pexprp), pexprp);
                بازگشت جدید AstBegin{nodep->fileline(), "", nullptr, نادرست};
            }
            AstFork* ثابت forkp = جدید AstFork{nodep->fileline(), VJoinType::JOIN_NONE};
            forkp->addForksp(pexprp->bodyp()->unlinkFrBack());
            اگر (Ast_گره_Stmt* ثابت finalp = pexprp->finalp()) {
                اگر (!m_finalp) {
                    m_finalp = جدید AstFinal{m_modp->fileline(), finalp->unlinkFrBack()};
                    m_modp->addStmtsp(m_finalp);
                } وگرنه {
                    m_finalp->addStmtsp(finalp->unlinkFrBack());
                }
            }
            VL_DO_DANGLING2(افزودن_Deletep(pexprp), pexprp, propp);
            بازگشت forkp;
        }

        بازگشت assertCond(nodep, VN_AS(propp, گره_عبارت), passsp, failsp);
    }

    Ast_گره_Stmt* newFireAssertUnchecked(ثابت Ast_گره_Stmt* nodep, ثابت string& message,
                                        Ast_گره_عبارت* exprsp = nullptr) {
        // Like newFireAssert() but omits the asserts-on check
        AstDisplay* ثابت dispp
            = جدید AstDisplay{nodep->fileline(), VDisplayType::DT_ERROR, message, nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        Ast_گره_Stmt* ثابت bodysp = dispp;
        replaceDisplay(dispp, "%%Error");  // Convert to standard DISPLAY format
        اگر (exprsp) dispp->fmtp()->exprsp()->addNext(exprsp);
        اگر (v3Global.opt.stopFail()) bodysp->addNext(جدید AstStop{nodep->fileline(), نادرست});
        بازگشت bodysp;
    }

    Ast_گره_Stmt* newFireAssert(ثابت Ast_گره_Stmt* nodep, VAssertDirectiveType directiveType,
                               VAssertType assertType, ثابت string& message,
                               Ast_گره_عبارت* exprsp = nullptr) {
        Ast_گره_Stmt* bodysp = newFireAssertUnchecked(nodep, message, exprsp);
        bodysp = newIfAssertOn(bodysp, directiveType, assertType);
        بازگشت bodysp;
    }

    AstVar* createDelayedVar(ثابت std::string& name, AstAlways* alwaysp, Ast_گره_عبارت* exprp,
                             Ast_گره_عبارت* initp) {
        File_سطر* ثابت flp = exprp->fileline();
        AstVar* ثابت varp = جدید AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        // TODO: this lifetime seems nonsene (can't have NBAs to automatics), but is as before
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->addStmtsp(varp);
        // Before enough clock ticks, $past returns the expression's default sampled value.
        اگر (initp) {
            m_modp->addStmtsp(جدید AstInitialStatic{
                flp, جدید AstAssign{flp, جدید AstVarRef{flp, varp, VAccess::WRITE}, initp}});
        }
        ++m_statPastVars;
        // Actually set the delayed value
        Ast_گره_عبارت* ثابت lhsp = جدید AstVarRef{flp, varp, VAccess::WRITE};
        AstAssignDly* ثابت assignp = جدید AstAssignDly{flp, lhsp, exprp};
        alwaysp->prependStmtsp(assignp);
        بازگشت varp;
    }

    AstAlways* getDelayedAlways(Ast_گره_عبارت* exprp, Ast_گره_عبارت* initp, Ast_Sen_درخت* senTreep) {
        auto& expr2Sen2Alwaysr
            = initp ? m_modExpr2Sen2DefaultDelayedAlwaysp : m_modExpr2Sen2DelayedAlwaysp;
        AstAlways*& alwayspr = expr2Sen2Alwaysr[*exprp][*senTreep];
        اگر (!alwayspr) {
            File_سطر* ثابت flp = exprp->fileline();
            // Create the always block that computes the delayed values
            alwayspr = جدید AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, nullptr};
            m_modp->addStmtsp(alwayspr);
            // Create the once-delayed variable
            ثابت std::string name = "_Vpast_" + cvtToStr(m_mod_Past_رقم++) + "_1";
            AstVar* ثابت varp = createDelayedVar(name, alwayspr, exprp, initp);
            // Add it to delayed variable vector
            m_delayed(alwayspr).emplace_back(varp);
        } وگرنه {
            // Reusing exiting, not needed
            VL_DO_DANGLING(افزودن_Deletep(exprp), exprp);
            اگر (initp) VL_DO_DANGLING(افزودن_Deletep(initp), initp);
            VL_DO_DANGLING(افزودن_Deletep(senTreep), senTreep);
        }
        بازگشت alwayspr;
    }

    Ast_گره_عبارت* get_Past_مقدار(Ast_گره_عبارت* exprp, Ast_گره_عبارت* initp, Ast_Sen_درخت* senTreep,
                              uint32_t ticks) {
        UASSERT_OBJ(ticks > 0, exprp, "Delay must be > 0");
        File_سطر* ثابت flp = exprp->fileline();
        AstAlways* ثابت alwaysp
            = getDelayedAlways(exprp, initp ? initp->clone_درخت_Pure(نادرست) : nullptr, senTreep);
        Ast_گره_عبارت* ثابت resultp = past_مقدار_Ref(alwaysp, flp, ticks, initp);
        اگر (initp) VL_DO_DANGLING(افزودن_Deletep(initp), initp);
        بازگشت resultp;
    }

    Ast_گره_عبارت* past_مقدار_Ref(AstAlways* alwaysp, File_سطر* flp, uint32_t ticks,
                              Ast_گره_عبارت* initp) {
        std::vector<AstVar*>& delayedr = m_delayed(alwaysp);
        // Ensure the required delay exists
        وقتی (delayedr.size() < ticks) {
            AstVar* ثابت firstp = delayedr.front();
            File_سطر* ثابت varFlp = firstp->fileline();
            // Create once more delayed value
            std::string name = firstp->name();
            name.resize(name.size() - 1);
            name += std::to_string(delayedr.size() + 1);
            Ast_گره_عبارت* ثابت prevp = جدید AstVarRef{varFlp, delayedr.back(), VAccess::READ};
            AstVar* ثابت varp = createDelayedVar(name, alwaysp, prevp,
                                                  initp ? initp->clone_درخت_Pure(نادرست) : nullptr);
            // Add it to delayed variable vector
            delayedr.emplace_back(varp);
        }
        // Return a reference to the appropriately delayed variable
        بازگشت جدید AstVarRef{flp, delayedr.at(ticks - 1), VAccess::READ};
    }

    // Time of the pipeline's last sampling tick, for end-of-simulation readers
    AstVar* getPastTickTimeVar(AstAlways* alwaysp) {
        AstVar*& varpr = m_delayedAlways2TickTimep[alwaysp];
        اگر (!varpr) {
            File_سطر* ثابت flp = alwaysp->fileline();
            varpr = جدید AstVar{flp, VVarType::MODULETEMP,
                               "_Vpast_" + cvtToStr(m_mod_Past_رقم++) + "_t",
                               m_modp->findUInt64DType()};
            m_modp->addStmtsp(varpr);
            Ast_گره_عبارت* ثابت timep = جدید AstCExpr{flp, "vlSymsp->_vm_contextp__->time()", 64};
            alwaysp->addStmtsp(
                جدید AstAssign{flp, جدید AstVarRef{flp, varpr, VAccess::WRITE}, timep});
        }
        بازگشت varpr;
    }

    تهی visitAssertionIterate(Ast_گره_Cover_Or_Assert* nodep, Ast_گره* failsp) {
        اگر (m_beginp && nodep->name() == "") nodep->name(m_beginp->name());

        { AssertDeFutureVisitor{nodep->propp(), m_modp, m_mod_Past_رقم++}; }

        iterateAndNextNull(nodep->sentreep());
        اگر (AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            iterateAndNextNull(assertp->failsp());
        } وگرنه اگر (AstAssertIntrinsic* ثابت assertp = VN_CAST(nodep, AssertIntrinsic)) {
            iterateAndNextNull(assertp->failsp());
        } وگرنه اگر (AstCover* ثابت coverp = VN_CAST(nodep, Cover)) {
            iterateAndNextNull(coverp->coverincsp());
        } وگرنه اگر (!VN_IS(nodep, Restrict)) {
            nodep->v3fatalSrc("Unhandled assert type");
        }
        iterateAndNextNull(nodep->passsp());
        Ast_Sen_درخت* sentreep = nodep->sentreep();
        اگر (nodep->immediate()) {
            UASSERT_OBJ(!sentreep, nodep, "Immediate assertions don't have sensitivity");
        } وگرنه {
            UASSERT_OBJ(sentreep, nodep, "Concurrent assertions must have sensitivity");
            // Explicit inline clock differs from the enclosing always: hoist
            // and warn. To support this need queue of asserts to activate.
            اگر (m_procedurep && !nodep->senFromAlways()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Procedural concurrent assertion with"
                              " clocking event inside always (IEEE 1800-2023 16.14.6)");
                static_cast<Ast_گره*>(m_procedurep)->addNext(nodep->unlinkFrBack());
                بازگشت;  // Later iterate will pick up
            }
            sentreep->unlinkFrBack();
            اگر (m_procedurep) { VL_DO_DANGLING(افزودن_Deletep(sentreep), sentreep); }
        }
        //
        ثابت string& message = nodep->name();
        Ast_گره* passsp = nodep->passsp();
        اگر (passsp) passsp->unlinkFrBackWithNext();
        اگر (AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            failsp = assertp->failsp();
        } وگرنه اگر (AstAssertIntrinsic* ثابت assertp = VN_CAST(nodep, AssertIntrinsic)) {
            failsp = assertp->failsp();
        }
        اگر (failsp) failsp->unlinkFrBackWithNext();

        منطقی selfDestruct = نادرست;
        منطقی passspGated = نادرست;
        ثابت AstCover* ثابت coverp = VN_CAST(nodep, Cover);
        // A sequence event control is not an assertion directive; no assertion control
        ثابت منطقی seqEvent = coverp && coverp->isSeqEvent();
        اگر (coverp) {
            ++m_statCover;
            اگر (seqEvent) {
                // Keep the event-fire action, with no coverage bucket
            } وگرنه اگر (!v3Global.opt.coverageUser()) {
                selfDestruct = درست;
            } وگرنه {
                // V3Coverage assigned us a bucket to increment.
                AstCoverInc* ثابت covincp = VN_AS(coverp->coverincsp(), CoverInc);
                UASSERT_OBJ(covincp, coverp, "Missing AstCoverInc under assertion");
                covincp->unlinkFrBackWithNext();  // next() might have  AstAssign for trace
                اگر (message != "") covincp->declp()->comment(message);
                اگر (passsp) {
                    passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                               /*vacuous=*/نادرست);
                    passspGated = درست;
                    passsp = Ast_گره::addNext<Ast_گره, Ast_گره>(covincp, passsp);
                } وگرنه {
                    passsp = covincp;
                }
            }
        } وگرنه اگر (VN_IS(nodep, Assert) || VN_IS(nodep, AssertIntrinsic)) {
            اگر (nodep->immediate()) {
                ++m_statAsImm;
            } وگرنه {
                ++m_statAsNotImm;
            }
            اگر (!passsp && !failsp)
                failsp = newFireAssertUnchecked(
                    nodep, VN_IS(nodep, AssertIntrinsic) ? "'$cast' failed." : "'assert' failed.");
        } وگرنه {
            nodep->v3fatalSrc("Unknown node type");
        }

        VL_RESTORER(m_passsp);
        VL_RESTORER(m_failsp);
        VL_RESTORER(m_assertp);
        m_passsp = passsp;
        m_failsp = failsp;
        m_assertp = nodep;
        iterate(nodep->propp());

        Ast_گره* propExprp;
        Ast_گره_عبارت* disablep = nullptr;
        Ast_گره_عبارت* matchCountp = nullptr;
        اگر (AstPropSpec* ثابت specp = VN_CAST(nodep->propp(), PropSpec)) {
            propExprp = specp->propp()->unlinkFrBack();
            اگر (specp->disablep()) disablep = specp->disablep()->unlinkFrBack();
            matchCountp = specp->matchCountp();
        } وگرنه {
            propExprp = nodep->propp()->unlinkFrBack();
        }
        File_سطر* ثابت flp = nodep->fileline();
        منطقی passspAlreadyGated = نادرست;
        اگر (passsp && VN_IS(passsp, If)) passspAlreadyGated = VN_AS(passsp, If)->user1();
        اگر (passsp && !passspGated && !passspAlreadyGated && !VN_IS(propExprp, PExpr)
            && !seqEvent) {
            passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                       /*vacuous=*/نادرست);
        }
        اگر (failsp && !VN_IS(propExprp, PExpr)) {
            failsp = newIfAssertFailOn(failsp, nodep->directive(), nodep->userType());
        }
        اگر (coverp && matchCountp && passsp) {
            // Convert the match count into a loop that decrements a temporary variable until it
            // reaches zero.
            matchCountp->unlinkFrBack();
            AstVar* ثابت remainingp = جدید AstVar{
                flp, VVarType::MODULETEMP, m_match_شمار_Names.get(""), matchCountp->dtypep()};
            remainingp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remainingp);
            Ast_گره* ثابت replaysp
                = جدید AstAssign{flp, جدید AstVarRef{flp, remainingp, VAccess::WRITE}, matchCountp};
            AstLoop* ثابت loopp = جدید AstLoop{flp};
            loopp->addStmtsp(
                جدید AstLoopTest{flp, loopp, جدید AstVarRef{flp, remainingp, VAccess::READ}});
            loopp->addStmtsp(passsp);
            loopp->addStmtsp(
                جدید AstAssign{flp, جدید AstVarRef{flp, remainingp, VAccess::WRITE},
                              جدید AstSub{flp, جدید AstVarRef{flp, remainingp, VAccess::READ},
                                         جدید AstConst{flp, AstConst::Widthed_مقدار{},
                                                      remainingp->dtypep()->width(), 1}}});
            replaysp->addNext(loopp);
            passsp = replaysp;
        }
        Ast_گره* bodysp = assertBody(nodep, propExprp, passsp, failsp);
        اگر (disablep) bodysp = جدید AstIf{flp, جدید AstLogNot{flp, disablep}, bodysp};
        // Add assertOn check last, for better combining
        اگر (!seqEvent) bodysp = newIfAssertOn(bodysp, nodep->directive(), nodep->userType());
        اگر (sentreep) bodysp = جدید AstAlways{flp, VAlwaysKwd::ALWAYS, sentreep, bodysp};

        اگر (passsp && !passsp->backp()) VL_DO_DANGLING(افزودن_Deletep(passsp), passsp);
        اگر (failsp && !failsp->backp()) VL_DO_DANGLING(افزودن_Deletep(failsp), failsp);

        // Install it
        اگر (selfDestruct) {
            // Delete it after making the tree.  This way we can tell the user
            // if it wasn't constructed nicely or has other errors without needing --coverage.
            VL_DO_DANGLING(bodysp->delete_درخت(), bodysp);
            nodep->unlinkFrBack();
        } وگرنه {
            nodep->replaceWith(bodysp);
        }
        // Bye
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    منطقی isEmptyStmt(Ast_گره* nodep) {
        اگر (!nodep) بازگشت درست;
        اگر (AstBegin* ثابت beginp = VN_CAST(nodep, Begin)) {
            اگر (beginp->declsp()) بازگشت نادرست;
            برای (Ast_گره* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                اگر (!isEmptyStmt(stmtp)) بازگشت نادرست;
            }
            بازگشت درست;
        }
        بازگشت نادرست;
    }

    // VISITORS
    تهی visit(AstIf* nodep) override {
        اگر (!nodep->user1SetOnce()) {
            اگر (nodep->uniquePragma() || nodep->unique0Pragma()) {
                ثابت Ast_گره_If* ifp = nodep;
                Ast_گره_عبارت* propp = nullptr;
                منطقی hasDefaultElse = نادرست;
                انجامبده {
                    // If this statement ends with 'else if', then nextIf will point to the
                    // nextIf statement.  Otherwise it will be null.
                    ثابت Ast_گره_If* ثابت nextifp = dynamic_cast<Ast_گره_If*>(ifp->elsesp());
                    iterateAndNextNull(ifp->condp());

                    // Recurse into the true case.
                    iterateAndNextNull(ifp->thensp());

                    // If the last else is not an else if, recurse into that too.
                    اگر (ifp->elsesp() && !nextifp) {  //
                        iterateAndNextNull(ifp->elsesp());
                    }

                    // Build a bitmask of the true predicates
                    Ast_گره_عبارت* ثابت predp = ifp->condp()->clone_درخت_Pure(نادرست);
                    اگر (propp) {
                        propp = جدید AstConcat{nodep->fileline(), predp, propp};
                    } وگرنه {
                        propp = predp;
                    }

                    // Record if this ends with an 'else' that does not have an if
                    اگر (ifp->elsesp() && !nextifp) hasDefaultElse = درست;

                    ifp = nextifp;
                } وقتی (ifp);

                ثابت منطقی allow_none = nodep->unique0Pragma();

                // Empty case means no property
                اگر (!propp) propp = جدید AstConst{nodep->fileline(), AstConst::BitFalse{}};

                // Note: if this ends with an 'else', then we don't need to validate that one of
                // the predicates evaluates to true.
                Ast_گره_عبارت* ثابت ohot
                    = ((allow_none || hasDefaultElse)
                           ? static_cast<Ast_گره_عبارت*>(جدید AstOneHot0{nodep->fileline(), propp})
                           : static_cast<Ast_گره_عبارت*>(جدید AstOneHot{nodep->fileline(), propp}));
                ثابت VAssertType assertType
                    = nodep->uniquePragma() ? VAssertType::UNIQUE : VAssertType::UNIQUE0;
                // Move, not clone, as the branches might declare referenced variables
                VNRelinker relinker;
                nodep->unlinkFrBack(&relinker);
                AstIf* ثابت checkifp
                    = جدید AstIf{nodep->fileline(), جدید AstLogNot{nodep->fileline(), ohot},
                                newFireAssert(nodep, VAssertDirectiveType::VIOLATION_IF,
                                              assertType, "'unique if' statement violated"),
                                nodep};
                checkifp->isBoundsCheck(درست);  // To avoid LATCH warning
                checkifp->branchPred(VBranchPred::BP_UNLIKELY);
                relinker.relink(checkifp);
                بازگشت;
            }
        }

        iterateChildren(nodep);

        اگر (nodep->user2()) {
            // Combine consecutive assertOn checks if possible
            اگر (AstIf* ثابت backp = VN_CAST(nodep->backp(), If)) {
                اگر (backp->nextp() == nodep  //
                    && backp->user2()  //
                    && backp->condp()->same_درخت(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    backp->addThensp(nodep->thensp()->unlinkFrBackWithNext());
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    بازگشت;
                }
            }
            // Combine nested assertOn checks if possible
            اگر (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* ثابت checkp = VN_CAST(nodep->thensp(), If);
                اگر (checkp  //
                    && checkp->user2()  //
                    && checkp->condp()->same_درخت(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    VL_DO_DANGLING(افزودن_Deletep(checkp->unlinkFrBack()), checkp);
                    بازگشت;
                }
            }
            بازگشت;
        }

        // Swap assertOn check with single statement 'if' statement to bubble up for combining
        // Note we can't just swap the conditions as they two Ifs have different flags,
        // so swapping the Ifs themeselves then swapping back the bodies.
        اگر (nodep->condp()->isPure()) {
            اگر (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* ثابت checkp = VN_CAST(nodep->thensp(), If);
                اگر (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    بازگشت;
                }
            }
            اگر (nodep->elsesp() && !nodep->elsesp()->nextp() && isEmptyStmt(nodep->thensp())) {
                AstIf* ثابت checkp = VN_CAST(nodep->elsesp(), If);
                اگر (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addElsesp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    بازگشت;
                }
            }
        }
    }

    //========== Case assertions
    تهی visit(AstCase* nodep) override {
        // Introduce temporary variable for AstCase if needed - it is done here and not in V3Case
        // because this phase is before V3Scope and V3Case is not. Doing it before V3Scope ensures
        // that V3Scope will take care of a scope creation
        // We also need to do it before V3Begin, co that pragmas like `unique0` also work correctly
        اگر (!nodep->exprp()->isPure()) {
            ++m_statLiftedCaseExprs;
            File_سطر* ثابت fl = nodep->exprp()->fileline();
            AstVar* ثابت varp = جدید AstVar{fl, VVarType::BLOCKTEMP, m_caseTempNames.get(nodep),
                                            nodep->exprp()->dtypep()};
            Ast_گره_عبارت* ثابت origp = nodep->exprp()->unlinkFrBack();
            nodep->addHereThisAsNext(
                جدید AstAssign{fl, جدید AstVarRef{fl, varp, VAccess::WRITE}, origp});
            nodep->exprp(جدید AstVarRef{fl, varp, VAccess::READ});
            اگر (m_ftaskp) {
                varp->funcLocal(درست);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } وگرنه {
                m_modp->prependStmtsp(varp);
            }
            VIsCached::clear_Cache_درخت();
        }
        iterateChildren(nodep);
        اگر (!nodep->user1SetOnce()) {
            منطقی has_default = نادرست;
            برای (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                اگر (itemp->isDefault()) has_default = درست;
            }
            ثابت Ast_گره_DType* عبارت_Dtypep = nodep->exprp()->dtypep()->skipRefp();

            VAssertType assertType = VAssertType::INTERNAL;
            اگر (nodep->priorityPragma()) {
                assertType = VAssertType::PRIORITY;
            } وگرنه اگر (nodep->uniquePragma()) {
                assertType = VAssertType::UNIQUE;
            } وگرنه اگر (nodep->unique0Pragma()) {
                assertType = VAssertType::UNIQUE0;
            }

            string valFmt;
            اگر (عبارت_Dtypep->isIntegralOrPacked())
                valFmt = " for '" + cvtToStr(عبارت_Dtypep->widthMin()) + "'h%X'";
            اگر (nodep->fullPragma() || nodep->priorityPragma()) {
                // Need to add a default if there isn't one already
                ++m_statAsFull;
                اگر (!has_default) {
                    nodep->addItemsp(جدید AstCaseItem{
                        nodep->fileline(), nullptr /*DEFAULT*/,
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      nodep->pragmaString() + ", but non-match found" + valFmt,
                                      valFmt.empty() ? nullptr
                                                     : nodep->exprp()->clone_درخت_Pure(نادرست))});
                }
            }
            اگر (nodep->parallelPragma() || nodep->uniquePragma() || nodep->unique0Pragma()) {
                // Need to check that one, and only one of the case items match at any moment
                // If there's a default, we allow none to match, else exactly one must match
                ++m_statAsFull;
                اگر (!has_default && !nodep->itemsp()) {
                    // Not parallel, but harmlessly so.
                } وگرنه {
                    Ast_گره_عبارت* propp = nullptr;
                    برای (AstCaseItem* itemp = nodep->itemsp(); itemp;
                         itemp = VN_AS(itemp->nextp(), CaseItem)) {
                        Ast_گره_عبارت* itembitp = nullptr;
                        برای (Ast_گره_عبارت* icondp = itemp->condsp(); icondp;
                             icondp = VN_AS(icondp->nextp(), گره_عبارت)) {
                            Ast_گره_عبارت* onep;
                            اگر (AstInsideRange* ثابت rcondp = VN_CAST(icondp, InsideRange)) {
                                onep = rcondp->newAndFromInside(
                                    nodep->exprp()->clone_درخت_Pure(درست),
                                    rcondp->lhsp()->clone_درخت_Pure(درست),
                                    rcondp->rhsp()->clone_درخت_Pure(درست));
                            } وگرنه اگر (nodep->casex() || nodep->casez() || nodep->caseInside()) {
                                onep = AstEqWild::newTyped(itemp->fileline(),
                                                           nodep->exprp()->clone_درخت_Pure(نادرست),
                                                           icondp->clone_درخت_Pure(نادرست));
                            } وگرنه {
                                onep = AstEq::newTyped(icondp->fileline(),
                                                       nodep->exprp()->clone_درخت_Pure(نادرست),
                                                       icondp->clone_درخت_Pure(نادرست));
                            }
                            // OR together all conditions within the same case item
                            اگر (onep) {
                                اگر (itembitp) {
                                    itembitp = جدید AstOr{icondp->fileline(), onep, itembitp};
                                } وگرنه {
                                    itembitp = onep;
                                }
                            }
                        }
                        اگر (itembitp) {
                            اگر (propp) {
                                propp = جدید AstConcat{itemp->fileline(), itembitp, propp};
                            } وگرنه {
                                propp = itembitp;
                            }
                        }
                    }
                    // Empty case means no property
                    اگر (!propp) propp = جدید AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    ثابت منطقی allow_none = has_default || nodep->unique0Pragma();
                    // The following assertion looks as below.
                    // if (!$onehot(propp)) begin
                    //     if (propp == '0) begin if (!allow_none) $error("none match"); end
                    //     else $error("multiple match");
                    // end
                    Ast_گره_عبارت* ثابت ohot = جدید AstOneHot{nodep->fileline(), propp};
                    AstConst* ثابت zero = جدید AstConst{
                        nodep->fileline(), AstConst::Widthed_مقدار{}, propp->width(), 0};
                    AstIf* ثابت ohotIfp
                        = جدید AstIf{nodep->fileline(), جدید AstLogNot{nodep->fileline(), ohot}};
                    AstIf* ثابت zeroIfp = جدید AstIf{
                        nodep->fileline(),
                        جدید AstEq{nodep->fileline(), propp->clone_درخت_Pure(نادرست), zero}};
                    Ast_گره_عبارت* ثابت exprp = nodep->exprp();
                    ثابت string pragmaStr = nodep->pragmaString();
                    اگر (!allow_none)
                        zeroIfp->addThensp(
                            newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                          pragmaStr + ", but none matched" + valFmt,
                                          valFmt.empty() ? nullptr : exprp->clone_درخت_Pure(نادرست)));
                    zeroIfp->addElsesp(
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      pragmaStr + ", but multiple matches found" + valFmt,
                                      valFmt.empty() ? nullptr : exprp->clone_درخت_Pure(نادرست)));
                    ohotIfp->addThensp(zeroIfp);
                    ohotIfp->isBoundsCheck(درست);  // To avoid LATCH warning
                    ohotIfp->branchPred(VBranchPred::BP_UNLIKELY);
                    nodep->addNotParallelp(ohotIfp);
                }
            }
        }
    }

    تهی visit(AstFuture* nodep) override {
        nodep->v3error("Future sampled value function called outside property or sequence "
                       "expression (IEEE 16.9.4)");
        nodep->replaceWith(جدید AstConst{nodep->fileline(), 0});
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    //========== Past
    تهی visit(AstPast* nodep) override {
        اگر (!nodep->propertyTiming()) {
            nodep->initp(nodep->exprp()->clone_درخت_Pure(نادرست));
            { Default_Sampled_مقدار_Visitor{nodep->initp()}; }
        }
        Ast_گره_عبارت* ثابت initp = nodep->initp() ? nodep->initp()->unlinkFrBack() : nullptr;
        iterateChildren(nodep);
        uint32_t ticks = 1;
        اگر (nodep->ticksp()) {
            UASSERT_OBJ(VN_IS(nodep->ticksp(), Const), nodep,
                        "Expected constant ticks, checked in V3Width");
            ticks = VN_AS(nodep->ticksp(), Const)->toUInt();
        }
        UASSERT_OBJ(ticks >= 1, nodep, "0 tick should have been checked in V3Width");
        Ast_گره_عبارت* ثابت exprp = new_Sampled_عبارت(nodep->exprp()->unlinkFrBack());
        Ast_Sen_درخت* ثابت senTreep = nodep->sentreep()->unlinkFrBack();
        Ast_گره_عبارت* inp = nullptr;
        // Deliberately emitted for every ftask $past; calls outside final take the normal stage
        اگر (VN_IS(m_procedurep, Final) || m_ftaskp) {
            // A sampling-tick finish reads one stage deeper (IEEE 1800-2023 16.9.3).
            File_سطر* ثابت flp = nodep->fileline();
            AstAlways* ثابت alwaysp
                = getDelayedAlways(exprp, initp ? initp->clone_درخت_Pure(نادرست) : nullptr, senTreep);
            Ast_گره_عبارت* ثابت betweenTicksp = past_مقدار_Ref(alwaysp, flp, ticks, initp);
            Ast_گره_عبارت* ثابت onTickp = past_مقدار_Ref(alwaysp, flp, ticks + 1, initp);
            اگر (initp) VL_DO_DANGLING(افزودن_Deletep(initp), initp);
            Ast_گره_عبارت* ثابت onTickCondp = جدید AstLogAnd{
                flp, جدید AstCExpr{flp, "vlSymsp->_vm_contextp__->executingFinal()", 1},
                جدید AstEq{flp, جدید AstVarRef{flp, getPastTickTimeVar(alwaysp), VAccess::READ},
                          جدید AstCExpr{flp, "vlSymsp->_vm_contextp__->finishPendingTime()", 64}}};
            AstCond* ثابت condp = جدید AstCond{flp, onTickCondp, onTickp, betweenTicksp};
            condp->dtypeFrom(onTickp);
            inp = condp;
        } وگرنه {
            inp = get_Past_مقدار(exprp, initp, senTreep, ticks);
        }
        nodep->replaceWith(inp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    //========== Move $sampled down to read-only variables
    تهی visit(AstSampled* nodep) override {
        اگر (nodep->user1()) بازگشت;
        VL_RESTORER(m_inSampled);
        {
            m_inSampled = درست;
            iterateChildren(nodep);
        }
        اگر (nodep->exprp()) {
            nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        } وگرنه {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(Ast_گره_Var_Ref* nodep) override {
        اگر (m_inSampled && !nodep->user2() && !nodep->varp()->isTemp()) {
            اگر (!nodep->access().isReadOnly()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Write to variable in sampled expression");
            } وگرنه {
                VNRelinker relinkHandle;
                nodep->unlinkFrBack(&relinkHandle);
                AstSampled* ثابت newp = new_Sampled_عبارت(nodep);
                relinkHandle.relink(newp);
                newp->user1(1);
                v3Global.setHasSampled();
            }
        }
    }
    // Don't sample sensitivities
    تهی visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSampled);
        m_inSampled = نادرست;
        iterateChildren(nodep);
    }
    تهی visit(AstPExprClause* nodep) override {
        Ast_گره* stmtsp = nullptr;
        اگر (nodep->pass() && m_passsp) {
            // Cover adds COVERINC by AstNode::addNext, thus need to clone next too.
            stmtsp = m_passsp->clone_درخت(درست);
            stmtsp = newIfAssertPassOn(stmtsp, m_assertp->directive(), m_assertp->userType(),
                                       nodep->vacuous());
        } وگرنه اگر (!nodep->pass() && m_failsp) {
            stmtsp = m_failsp->clone_درخت(درست);
            stmtsp = newIfAssertFailOn(stmtsp, m_assertp->directive(), m_assertp->userType());
        }
        اگر (stmtsp) {
            stmtsp->foreachAndNext([](Ast_گره_Var_Ref* ثابت refp) {
                // References inside action blocks shouldn't be implicitly sampled
                // m_passsp/m_failsp have been already visited once and refs explicitly sampled
                // are handled already
                refp->user2(1);
            });
            nodep->replaceWith(stmtsp);
        } وگرنه {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstPExpr* nodep) override {
        اگر (m_inRestrict) {
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
        } وگرنه {
            iterateChildren(nodep);
        }
    }

    //========== Statements
    تهی visit(AstDisplay* nodep) override {
        iterateChildren(nodep);
        // Replace the special types with standard text
        اگر (nodep->displayType() == VDisplayType::DT_INFO) {
            replaceDisplay(nodep, "-Info");
        } وگرنه اگر (nodep->displayType() == VDisplayType::DT_WARNING) {
            replaceDisplay(nodep, "%%Warning");
        } وگرنه اگر (nodep->displayType() == VDisplayType::DT_ERROR) {
            replaceDisplay(nodep, "%%Error");
        } وگرنه اگر (nodep->displayType() == VDisplayType::DT_FATAL) {
            replaceDisplay(nodep, "%%Fatal");
        } وگرنه اگر (nodep->displayType() == VDisplayType::DT_MONITOR) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            File_سطر* ثابت fl = nodep->fileline();

            AstSenItem* monSenItemsp = nullptr;
            اگر (Ast_گره* ثابت monExprsp = nodep->fmtp()->exprsp()) {
                monExprsp->foreachAndNext([&](AstVarRef* varrefp) {
                    AstSenItem* ثابت senItemp
                        = جدید AstSenItem{fl, VEdgeType::ET_CHANGED,
                                         // Clone so get VarRef or VarXRef as needed
                                         varrefp->clone_درخت(نادرست)};
                    monSenItemsp = Ast_گره::addNextNull(monSenItemsp, senItemp);
                });
            }

            Ast_Sen_درخت* ثابت mon_Sen_درخت = جدید Ast_Sen_درخت{fl, monSenItemsp};
            ثابت auto mon_رقم = ++m_monitor_رقم;
            // Where $monitor was we do "__VmonitorNum = N;"
            AstAssign* ثابت newsetp = جدید AstAssign{
                fl, new_Monitor_رقم_Var_Refp(nodep, VAccess::WRITE), جدید AstConst{fl, mon_رقم}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__VmonitorOn && __VmonitorNum==N) $display(...);"
            Ast_گره* ثابت stmtsp = nodep;
            AstIf* ثابت ifp = جدید AstIf{
                fl,
                جدید AstLogAnd{fl, جدید AstLogNot{fl, newMonitorOffVarRefp(nodep, VAccess::READ)},
                              جدید AstEq{fl, جدید AstConst{fl, mon_رقم},
                                        new_Monitor_رقم_Var_Refp(nodep, VAccess::READ)}},
                stmtsp};
            ifp->isBoundsCheck(درست);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_گره* ثابت newp = جدید AstAlways{fl, VAlwaysKwd::ALWAYS, mon_Sen_درخت, ifp};
            m_modp->addStmtsp(newp);
        } وگرنه اگر (nodep->displayType() == VDisplayType::DT_STROBE) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            // Need one-shot
            File_سطر* ثابت fl = nodep->fileline();
            AstVar* ثابت varp
                = جدید AstVar{fl, VVarType::MODULETEMP, "__Vstrobe" + cvtToStr(m_mod_Strobe_رقم++),
                             nodep->findBitDType()};
            m_modp->addStmtsp(varp);
            // Where $strobe was we do "__Vstrobe = '1;"
            AstAssign* ثابت newsetp = جدید AstAssign{fl, جدید AstVarRef{fl, varp, VAccess::WRITE},
                                                     جدید AstConst{fl, AstConst::BitTrue{}}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__Vstrobe) begin $display(...); __Vstrobe = '0; end"
            Ast_گره* ثابت stmtsp = nodep;
            AstIf* ثابت ifp = جدید AstIf{fl, جدید AstVarRef{fl, varp, VAccess::READ}, stmtsp};
            ifp->isBoundsCheck(درست);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_گره* ثابت newp = جدید AstAlwaysPostponed{fl, ifp};
            stmtsp->addNext(جدید AstAssign{fl, جدید AstVarRef{fl, varp, VAccess::WRITE},
                                          جدید AstConst{fl, AstConst::BitFalse{}}});
            m_modp->addStmtsp(newp);
        }
    }
    تهی visit(AstMonitorOff* nodep) override {
        AstAssign* ثابت newp
            = جدید AstAssign{nodep->fileline(), newMonitorOffVarRefp(nodep, VAccess::WRITE),
                            جدید AstConst{nodep->fileline(), AstConst::BitTrue{}, nodep->off()}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstAssert* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    تهی visit(AstAssertCtl* nodep) override {
        iterateChildren(nodep);

        منطقی assertTypeConst = درست;
        اگر (!nodep->assertTypesp()) {
            nodep->assertTypes(VAssertType{ALL_ASSERT_TYPES});
        } وگرنه اگر (ثابت AstConst* ثابت assertTypesp = VN_CAST(nodep->assertTypesp(), Const)) {
            nodep->assertTypes(VAssertType{assertTypesp->toSInt()});
        } وگرنه {
            assertTypeConst = نادرست;
        }

        منطقی controlTypeConst = نادرست;
        اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep->controlTypep(), Const)) {
            nodep->ctlType(constp->toSInt());
            controlTypeConst = درست;
        }
        اگر (controlTypeConst
            && (nodep->ctlType() < VAssertCtlType::LOCK
                || nodep->ctlType() > VAssertCtlType::VACUOUS_OFF)) {
            nodep->unlinkFrBack();
            nodep->v3error("Bad $assertcontrol control_type '"
                           << cvtToStr(static_cast<صحیح>(nodep->ctlType()))
                           << "' (IEEE 1800-2023 Table 20-5)");
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        }
        اگر (assertTypeConst && nodep->assertTypes() != ALL_ASSERT_TYPES
            && nodep->assertTypes().containsAny(VAssertType::UNIQUE | VAssertType::UNIQUE0
                                                | VAssertType::PRIORITY)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: assert control assertion_type");
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        }

        منطقی directiveTypeConst = درست;
        اگر (!nodep->directiveTypesp()) {
            nodep->directiveTypes(VAssertDirectiveType::ASSERT | VAssertDirectiveType::ASSUME
                                  | VAssertDirectiveType::COVER);
        } وگرنه اگر (ثابت AstConst* ثابت directiveTypesp
                   = VN_CAST(nodep->directiveTypesp(), Const)) {
            nodep->directiveTypes(VAssertDirectiveType{directiveTypesp->toSInt()});
        } وگرنه {
            directiveTypeConst = نادرست;
        }
        اگر (!directiveTypeConst) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: non-const assert directive type expression");
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        }

        File_سطر* ثابت fl = nodep->fileline();
        UINFO(9, "Generating assertctl for a module: " << m_modp);
        AstCStmt* ثابت newp = جدید AstCStmt{fl};
        newp->add("vlSymsp->_vm_contextp__->assertCtl(");
        newp->add(nodep->controlTypep()->unlinkFrBack());
        newp->add(", ");
        اگر (nodep->assertTypesp()) {
            newp->add(nodep->assertTypesp()->unlinkFrBack());
        } وگرنه {
            newp->add(std::to_string(ALL_ASSERT_TYPES));
        }
        newp->add(", " + std::to_string(nodep->directiveTypes()) + ");\n");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstAssertIntrinsic* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    تهی visit(AstCover* nodep) override {  //
        visitAssertionIterate(nodep, nullptr);
    }
    تهی visit(AstRestrict* nodep) override {
        VL_RESTORER(m_inRestrict);
        m_inRestrict = درست;
        iterateChildren(nodep);
        // IEEE says simulator ignores these
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }

    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Past_رقم);
        VL_RESTORER(m_mod_Strobe_رقم);
        VL_RESTORER(m_finalp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DelayedAlwaysp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DefaultDelayedAlwaysp);
        VL_RESTORER_CLEAR(m_delayedAlways2TickTimep);
        m_modp = nodep;
        m_mod_Past_رقم = 0;
        m_mod_Strobe_رقم = 0;
        m_finalp = nullptr;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstGenBlock* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstBegin* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
        // If the body is a single assertOn check, bubble it up for combining
        اگر (!nodep->declsp()) {
            اگر (AstIf* ثابت ifp = VN_CAST(nodep->stmtsp(), If)) {
                اگر (ifp->user2() && !ifp->nextp()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(ifp->unlinkFrBack());
                    nodep->addStmtsp(ifp->thensp()->unlinkFrBackWithNext());
                    ifp->addThensp(nodep);
                }
            }
        }
    }

    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit AssertVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~AssertVisitor() override {
        V3Stats::addStat("Assertions, assert non-immediate statements", m_statAsNotImm);
        V3Stats::addStat("Assertions, assert immediate statements", m_statAsImm);
        V3Stats::addStat("Assertions, cover statements", m_statCover);
        V3Stats::addStat("Assertions, full/parallel case", m_statAsFull);
        V3Stats::addStat("Assertions, $past variables", m_statPastVars);
        V3Stats::addStat("Assertions, assertOn checks combined", m_statAssertOnCombined);
        V3Stats::addStat("Assertions, assertOn checks hoisted", m_statAssertOnHoisted);
        V3Stats::addStat("Assertions, lifted impure case expressions", m_statLiftedCaseExprs);
        // Rewrites can change purity, e.g. by compiling out assertion statements with --no-assert
        VIsCached::clear_Cache_درخت();
    }
};

//######################################################################
// Top Assert class

تهی V3Assert::assertAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("assert", 0, dump_درخت_Either_Level() >= 3);
}
