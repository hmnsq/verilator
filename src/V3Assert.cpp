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
    Ast_عقدة* m_scopep = nullptr;

    // VISITORS
    فراغ visit(Ast_عقدة_Module* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(nullptr);
        iterateChildren(nodep);
    }
    فراغ visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(nullptr);
        iterateChildren(nodep);
    }
    فراغ visit(AstDefaultDisable* nodep) override {
        UASSERT_OBJ(nodep, m_scopep,
                    "default disable iff must be inside a module or generate block");
        AstDefaultDisable* defaultp = nullptr;
        إذا (ثابت Ast_عقدة_Module* ثابت modp = VN_CAST(m_scopep, عقدة_Module)) {
            defaultp = modp->defaultDisablep();
        } وإلا {
            defaultp = VN_AS(m_scopep, GenBlock)->defaultDisablep();
        }
        إذا (VL_UNLIKELY(defaultp)) {
            nodep->v3error("Only one 'default disable iff' allowed per "
                           << (VN_IS(m_scopep, عقدة_Module) ? "module" : "generate block")
                           << " (IEEE 1800-2023 16.15)");
        } وإلا إذا (Ast_عقدة_Module* ثابت modp = VN_CAST(m_scopep, عقدة_Module)) {
            modp->defaultDisablep(nodep);
        } وإلا {
            VN_AS(m_scopep, GenBlock)->defaultDisablep(nodep);
        }
    }
    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
    explicit DefaultDisableLocalVisitor(AstNetlist* nodep) { iterate(nodep); }
};

صنف DefaultDisablePropagateVisitor final : عام VNVisitor {
    // STATE
    AstDefaultDisable* m_defaultDisablep = nullptr;

    // VISITORS
    فراغ visit(Ast_عقدة_Module* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    فراغ visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        إذا (!nodep->defaultDisablep()) nodep->defaultDisablep(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
    explicit DefaultDisablePropagateVisitor(AstNetlist* nodep) { iterate(nodep); }
};

// Lower a sequence used as an event control ('@seq', IEEE 1800-2023 9.4.2.4) into a
// synthesized event fired by an internal 'cover sequence' on each end-of-match
صنف SeqEventLowerVisitor final : عام VNVisitor {
    // STATE
    Ast_عقدة_Module* m_modp = nullptr;  // Current module
    V3UniqueNames m_names{"__VseqEvent"};  // Synthesized event names

    // VISITORS
    فراغ visit(Ast_عقدة_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstSenItem* nodep) override {
        AstFuncRef* ثابت funcrefp = VN_CAST(nodep->sensp(), FuncRef);
        إذا (funcrefp && VN_IS(funcrefp->taskp(), Sequence)) {
            File_سطر* ثابت flp = nodep->fileline();
            AstVar* ثابت eventp = جديد AstVar{flp, VVarType::MODULETEMP, m_names.get(nodep),
                                              m_modp->findBasicDType(VBasicDTypeKwd::EVENT)};
            eventp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(eventp);
            v3Global.setHasEvents();
            funcrefp->unlinkFrBack();
            nodep->sensp(جديد AstVarRef{flp, eventp, VAccess::READ});
            ثابت منطقي automaticActual = funcrefp->exists([](ثابت Ast_عقدة_Var_Ref* refp) {
                إرجاع refp->varp() && refp->varp()->lifetime().isAutomatic();
            });
            إذا (automaticActual) {
                nodep->v3error("Arguments to a sequence used as an event control must be"
                               " static (IEEE 1800-2023 9.4.2.4)");
                VN_AS(funcrefp->taskp(), Sequence)->isReferenced(خطأ);
                VL_DO_DANGLING(ادفع_Deletep(funcrefp), funcrefp);
                إرجاع;
            }
            AstFireEvent* ثابت firep
                = جديد AstFireEvent{flp, جديد AstVarRef{flp, eventp, VAccess::WRITE}, خطأ};
            AstCover* ثابت coverp
                = جديد AstCover{flp, جديد AstPropSpec{flp, nullptr, nullptr, funcrefp}, firep,
                               VAssertType::CONCURRENT};
            coverp->isCoverSeq(true);
            coverp->isSeqEvent(true);
            m_modp->addStmtsp(coverp);
            إرجاع;
        }
        iterateChildren(nodep);
    }
    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
    explicit SeqEventLowerVisitor(AstNetlist* nodep) { iterate(nodep); }
};

}  // namespace

فراغ V3AssertCommon::collectDefaultDisable(AstNetlist* nodep) {
    { DefaultDisableLocalVisitor{nodep}; }
    { DefaultDisablePropagateVisitor{nodep}; }
}

فراغ V3AssertCommon::lowerSequenceEvents(AstNetlist* nodep) {
    { SeqEventLowerVisitor{nodep}; }
    V3Global::dump_Check_Global_شجرة("assertseqevent", 0, dump_شجرة_Either_Level() >= 3);
}

string V3AssertCommon::assertCtlGetCall(ثابت حرف* query, VAssertType type,
                                        VAssertDirectiveType directiveType) {
    // Template arguments let the runtime build the type/directive mask at C++ compile time
    إرجاع "vlSymsp->_vm_contextp__->assertCtlGet<"s + std::to_string(type) + ", "s
           + std::to_string(directiveType) + ">(VerilatedAssertCtlQuery::"s + query + ")"s;
}

منطقي V3AssertCommon::check_Sva_قيمة_Limit(Ast_عقدة* nodep, ثابت AstConst* constp, ثابت حرف* what) {
    // Temporal values can materialize as O(N) AST or state in later passes.
    // Leave headroom for a range ring's extra slot and V3Number's signed
    // `(bits + 31) / 32` word count.
    ساكن constexpr unsigned SVA_قيمة_HARD_LIMIT
        = static_cast<unsigned>(std::numeric_limits<صحيح>::max()) - 32U;
    ثابت unsigned configuredLimit = static_cast<unsigned>(v3Global.opt.max_رقم_Width());
    ثابت unsigned limit = std::min(configuredLimit, SVA_قيمة_HARD_LIMIT);
    إذا (constp->رقم().fitsInUInt() && constp->toUInt() <= limit) إرجاع true;
    nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << what << " exceeds implementation limit of "
                                                 << limit
                                                 << (configuredLimit <= SVA_قيمة_HARD_LIMIT
                                                         ? " (--max-num-width)"
                                                         : " (host arithmetic limit)"));
    إرجاع خطأ;
}

// The bound of a cycle delay in a sequence, if a non-negative elaboration-time constant
ساكن ثابت AstConst* checkSvaDelayBound(Ast_عقدة* errp, Ast_عقدة_تعبير* boundp, ثابت حرف* what) {
    ثابت AstConst* ثابت constp = VN_CAST(boundp, Const);
    إذا (!constp || (constp->dtypep()->isSigned() && constp->رقم().isNegative())) {
        errp->v3error(what << " is not a non-negative elaboration-time constant"
                              " (IEEE 1800-2023 16.7)");
        إرجاع nullptr;
    }
    إذا (!V3AssertCommon::check_Sva_قيمة_Limit(errp, constp, "SVA cycle delay")) إرجاع nullptr;
    إرجاع constp;
}

// Fold the bounds of a cycle delay in a sequence, and return if they are valid, else report it
// on errp
ساكن منطقي svaCycleDelayValid(ثابت AstDelay* nodep, Ast_عقدة* errp) {
    V3Const::constifyParamsNoWarnEdit(nodep->lhsp());
    ثابت AstConst* ثابت minConstp = checkSvaDelayBound(
        errp, nodep->lhsp(), nodep->isRangeDelay() ? "Range delay minimum" : "Delay value");
    إذا (!nodep->rhsp() || nodep->isUnbounded()) إرجاع minConstp != nullptr;
    V3Const::constifyParamsNoWarnEdit(nodep->rhsp());
    ثابت AstConst* ثابت maxConstp
        = checkSvaDelayBound(errp, nodep->rhsp(), "Range delay maximum");
    إذا (!minConstp || !maxConstp) إرجاع خطأ;
    إذا (maxConstp->toUInt() < minConstp->toUInt()) {
        errp->v3error("Range delay maximum must be >= minimum (IEEE 1800-2023 16.7)");
        إرجاع خطأ;
    }
    إرجاع true;
}

فراغ V3AssertCommon::checkSvaCycleDelay(AstDelay* nodep, Ast_عقدة* errp) {
    إذا (svaCycleDelayValid(nodep, errp)) إرجاع;
    // Make the delay valid, so later steps do not fail after the error
    File_سطر* ثابت flp = nodep->fileline();
    Ast_عقدة_تعبير* ثابت minp = nodep->lhsp();
    minp->replaceWith(جديد AstConst{flp, AstConst::BitFalseErroring{}});
    VL_DO_DANGLING(minp->delete_شجرة(), minp);
    إذا (nodep->rhsp() && !nodep->isUnbounded()) {
        Ast_عقدة_تعبير* ثابت maxp = nodep->rhsp();
        maxp->replaceWith(جديد AstConst{flp, AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(maxp->delete_شجرة(), maxp);
    }
}

منطقي V3AssertCommon::refsSvaFormal(ثابت Ast_عقدة* nodep) {
    إرجاع nodep->exists([](ثابت Ast_عقدة_Var_Ref* refp) {
        ثابت AstVar* ثابت varp = refp->varp();
        إذا (!varp || !varp->isIO()) إرجاع خطأ;
        ثابت Ast_عقدة* ثابت declp = varp->aboveLoopp();
        إرجاع VN_IS(declp, Sequence) || VN_IS(declp, Property);
    });
}

فراغ V3AssertCommon::checkSvaInstanceDelays(Ast_عقدة* nodep, Ast_عقدة* instp) {
    // A delay still referencing a formal argument of an enclosing sequence or property is
    // checked when that is instanced. Collect first, as checking edits the delays.
    std::vector<AstDelay*> delayps;
    nodep->foreach([&](AstDelay* delayp) {
        إذا (!delayp->isCycleDelay() || refsSvaFormal(delayp->lhsp())) إرجاع;
        إذا (delayp->rhsp() && refsSvaFormal(delayp->rhsp())) إرجاع;
        delayps.ادفع_back(delayp);
    });
    لكل (AstDelay* ثابت delayp : delayps) checkSvaCycleDelay(delayp, instp);
}

//######################################################################
// AssertDeFutureVisitor
// If any AstFuture, then move all non-future varrefs to be one cycle behind,
// see IEEE 1800-2023 16.9.4.

صنف AssertDeFutureVisitor final : عام VNVisitor {
    // STATE - across all visitors
    Ast_عقدة_Module* ثابت m_modp;  // Module future is underneath
    ثابت AstFuture* m_futurep;  // First AstFuture found
    ثابت unsigned m_past_رقم;  // Prefix unique number for this module
    std::map<AstVar*, AstVar*> m_delayedVars;  // Old to delayed variable mapping
    // STATE - for current visit position (use VL_RESTORER)
    منطقي m_inFuture = خطأ;  // Inside a future
    منطقي m_unsupported = خطأ;  // Printed unsupported

    // METHODS
    فراغ unsupported(Ast_عقدة* nodep) {
        إذا (m_unsupported) إرجاع;
        m_unsupported = true;
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported/illegal: Future value function used with expression with "
                          << nodep->prettyOperatorName());
    }
    // VISITORS
    فراغ visit(AstFuture* nodep) override {
        VL_RESTORER(m_inFuture);
        m_inFuture = true;
        iterateChildren(nodep);
        // Done with the future, this subexpression is current-time
        nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }
    فراغ visit(Ast_عقدة_Var_Ref* nodep) override {
        إذا (nodep->user1SetOnce()) إرجاع;
        إذا (m_inFuture || m_unsupported)
            إرجاع;  // Need user1 set above, don't process when Future is removed
        إذا (nodep->access().isWriteOrRW()) {
            unsupported(nodep);
            إرجاع;
        }
        auto it = m_delayedVars.find(nodep->varp());
        AstVar* outvarp;
        إذا (it == m_delayedVars.end()) {
            Ast_Sen_شجرة* ثابت sentreep = m_futurep->sentreep();
            AstAlways* ثابت alwaysp = جديد AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS,
                                                     sentreep->clone_شجرة(خطأ), nullptr};
            m_modp->addStmtsp(alwaysp);
            outvarp = جديد AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                 "__Vnotfuture" + cvtToStr(m_past_رقم) + "_" + nodep->name(),
                                 nodep->dtypep()};
            m_modp->addStmtsp(outvarp);
            AstVarRef* varRefAWritep = جديد AstVarRef{nodep->fileline(), outvarp, VAccess::WRITE};
            varRefAWritep->user1(true);
            Ast_عقدة_Var_Ref* varRefAReadp = nodep->clone_شجرة(خطأ);
            varRefAReadp->user1(true);
            Ast_عقدة* ثابت assp = جديد AstAssignDly{nodep->fileline(), varRefAWritep, varRefAReadp};
            alwaysp->addStmtsp(assp);
            m_delayedVars.emplace(nodep->varp(), outvarp);
        } وإلا {
            outvarp = it->second;
        }
        AstVarRef* newp = جديد AstVarRef{nodep->fileline(), outvarp, VAccess::READ};
        newp->user1(true);
        UINFO(9, "DeFuture " << nodep << "  becomes " << newp);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }
    فراغ visit(Ast_عقدة_FTask_Ref* nodep) override { unsupported(nodep); }
    فراغ visit(AstMethodCall* nodep) override { unsupported(nodep); }
    فراغ visit(Ast_عقدة* nodep) override {
        إذا (!nodep->isPure()) unsupported(nodep);
        iterateChildren(nodep);
    }

عام:
    // CONSTRUCTORS
    explicit AssertDeFutureVisitor(Ast_عقدة* nodep, Ast_عقدة_Module* modp, unsigned past_رقم)
        : m_modp{modp}
        , m_past_رقم{past_رقم} {
        // See if any Future before we process
        إذا (nodep->forall([&](ثابت AstFuture* futurep) -> منطقي {
                m_futurep = futurep;
                إرجاع خطأ;
            }))
            إرجاع;
        // UINFOTREE(9, nodep, "", "defuture-in");
        visit(nodep);  // Nodep may get deleted
        // UINFOTREE(9, nodep, "", "defuture-ou");
    }
    ~AssertDeFutureVisitor() = افتراضي;
};

// Prepare one default sampled expression.
صنف Default_Sampled_قيمة_Visitor final : عام VNVisitor {
    // METHODS
    ساكن AstConst* newTypeDefault(Ast_عقدة_تعبير* nodep) {
        AstConst* ثابت constp
            = جديد AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()};
        إذا (nodep->dtypep()->isFourstate()) constp->رقم().setAllBitsX();
        constp->dtypeFrom(nodep);
        إرجاع constp;
    }

    // VISITORS
    فراغ visit(AstCMethodHard* nodep) override {
        إذا (nodep->method() == VCMethod::EVENT_IS_TRIGGERED) {
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        } وإلا {
            iterateChildren(nodep);
        }
    }
    فراغ visit(Ast_عقدة_Var_Ref* nodep) override {
        AstVar* ثابت varp = nodep->varp();
        إذا (varp->isParam()
            || (!varp->isNet() && varp->lifetime().isStatic() && varp->hasUserInit())) {
            إرجاع;
        }
        nodep->replaceWith(newTypeDefault(nodep));
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }
    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
    explicit Default_Sampled_قيمة_Visitor(Ast_عقدة* nodep) { iterate(nodep); }
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
    ثابت VNUser1InUse m_user1InUse;
    ثابت VNUser2InUse m_user2InUse;
    AstUser2Allocator<AstAlways, std::vector<AstVar*>> m_delayed;

    // STATE
    Ast_عقدة_Module* m_modp = nullptr;  // Last module
    ثابت Ast_عقدة* m_beginp = nullptr;  // Last AstBegin/AstGenBlock
    unsigned m_monitor_رقم = 0;  // Global $monitor numbering (not per module)
    AstVar* m_monitor_رقم_Varp = nullptr;  // $monitor number variable
    AstVar* m_monitorOffVarp = nullptr;  // $monitoroff variable
    unsigned m_mod_Past_رقم = 0;  // Module past numbering
    unsigned m_mod_Strobe_رقم = 0;  // Module $strobe numbering
    Ast_عقدة_Procedure* m_procedurep = nullptr;  // Current procedure
    VDouble0 m_statCover;  // Statistic tracking
    VDouble0 m_statAsNotImm;  // Statistic tracking
    VDouble0 m_statAsImm;  // Statistic tracking
    VDouble0 m_statAsFull;  // Statistic tracking
    VDouble0 m_statPastVars;  // Statistic tracking
    VDouble0 m_statAssertOnCombined;  // Statistic tracking
    VDouble0 m_statAssertOnHoisted;  // Statistic tracking
    منطقي m_inSampled = خطأ;  // True inside a sampled expression
    منطقي m_inRestrict = خطأ;  // True inside restrict assertion
    Ast_عقدة* m_passsp = nullptr;  // Current pass statement
    Ast_عقدة* m_failsp = nullptr;  // Current fail statement
    Ast_عقدة_Cover_Or_Assert* m_assertp = nullptr;  // Current assertion
    AstFinal* m_finalp = nullptr;  // Current final block
    VDouble0 m_statLiftedCaseExprs;  // Count of purified case expressions
    Ast_عقدة_FTask* m_ftaskp = nullptr;  // Current function/task
    V3UniqueNames m_caseTempNames{"__VCase"};
    V3UniqueNames m_match_تعداد_Names{"__VnfaRemainingMatchCount"};  // Match replay counter names
    // Maps from (expression, senTree) to the AstAlways that computes its delayed values.
    std::unordered_map<VNRef<Ast_عقدة_تعبير>, std::unordered_map<VNRef<Ast_Sen_شجرة>, AstAlways*>>
        m_modExpr2Sen2DelayedAlwaysp;
    // Source sampled-value history is initialized, unlike internal property-lowering history.
    std::unordered_map<VNRef<Ast_عقدة_تعبير>, std::unordered_map<VNRef<Ast_Sen_شجرة>, AstAlways*>>
        m_modExpr2Sen2DefaultDelayedAlwaysp;
    // Map from delayed-value AstAlways to its last-sampling-tick time variable
    std::unordered_map<ثابت AstAlways*, AstVar*> m_delayedAlways2TickTimep;

    // METHODS
    ساكن ثابت حرف* assertPassOnQuery(منطقي vacuous) {
        ساكن constexpr ثابت حرف* queries[2]
            = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
        إرجاع queries[vacuous];
    }
    ساكن Ast_عقدة_تعبير* assertOnCond(File_سطر* fl, VAssertType type,
                                     VAssertDirectiveType directiveType) {
        // cppcheck-suppress missingReturn
        بدل (directiveType) {
        حالة VAssertDirectiveType::INTRINSIC: إرجاع جديد AstConst{fl, AstConst::BitTrue{}};
        حالة VAssertDirectiveType::VIOLATION_CASE: {
            إذا (v3Global.opt.assertCase()) {
                إرجاع جديد AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            // If assertions are off, have constant propagation rip them out later
            // This allows syntax errors and such to be detected normally.
            إرجاع جديد AstConst{fl, AstConst::BitFalse{}};
        }
        حالة VAssertDirectiveType::ASSERT:
        حالة VAssertDirectiveType::COVER:
        حالة VAssertDirectiveType::ASSUME: {
            إذا (v3Global.opt.assertOn()) {
                إرجاع جديد AstCExpr{
                    fl, AstCExpr::Pure{},
                    V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
            }
            إرجاع جديد AstConst{fl, AstConst::BitFalse{}};
        }
        حالة VAssertDirectiveType::INTERNAL:
        حالة VAssertDirectiveType::VIOLATION_IF:
        حالة VAssertDirectiveType::RESTRICT: {
            إذا (v3Global.opt.assertOn()) {
                إرجاع جديد AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            إرجاع جديد AstConst{fl, AstConst::BitFalse{}};
        }
        }
        VL_UNREACHABLE;
    }

    ساكن منطقي isControlled(VAssertDirectiveType directiveType) {
        إرجاع (static_cast<صحيح>(directiveType)
                & (static_cast<صحيح>(VAssertDirectiveType::ASSERT)
                   | static_cast<صحيح>(VAssertDirectiveType::COVER)
                   | static_cast<صحيح>(VAssertDirectiveType::ASSUME)));
    }
    ساكن Ast_عقدة_تعبير* assertPassOnCond(File_سطر* fl, VAssertType type,
                                         VAssertDirectiveType directiveType, منطقي vacuous) {
        إذا (!isControlled(directiveType)) إرجاع جديد AstConst{fl, AstConst::BitTrue{}};
        إذا (!v3Global.opt.assertOn()) إرجاع جديد AstConst{fl, AstConst::BitFalse{}};
        إرجاع جديد AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType), 1};
    }
    ساكن Ast_عقدة_تعبير* assertFailOnCond(File_سطر* fl, VAssertType type,
                                         VAssertDirectiveType directiveType) {
        إذا (!isControlled(directiveType)) إرجاع جديد AstConst{fl, AstConst::BitTrue{}};
        إذا (!v3Global.opt.assertOn()) إرجاع جديد AstConst{fl, AstConst::BitFalse{}};
        إرجاع جديد AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType), 1};
    }
    string assertDisplayMessage(ثابت Ast_عقدة* nodep, ثابت string& prefix, ثابت string& message,
                                VDisplayType severity) {
        إذا (severity == VDisplayType::DT_ERROR || severity == VDisplayType::DT_FATAL) {
            إرجاع ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                    + cvtToStr(nodep->fileline()->lineno()) + ": Assertion failed in %m"
                    + ((message != "") ? ": " : "") + message + "\n");
        }
        إرجاع ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                + cvtToStr(nodep->fileline()->lineno()) + ": %m" + ((message != "") ? ": " : "")
                + message + "\n");
    }
    فراغ replaceDisplay(AstDisplay* nodep, ثابت string& prefix) {
        nodep->fmtp()->text(
            assertDisplayMessage(nodep, prefix, nodep->fmtp()->text(), nodep->displayType()));
        nodep->displayType(VDisplayType::DT_WRITE);
        // cppcheck-suppress nullPointer
        Ast_عقدة_تعبير* ثابت timenewp = جديد AstTime{nodep->fileline(), m_modp->timeunit()};
        nodep->fmtp()->prependExprsp(timenewp);
        إذا (!nodep->fmtp()->scopeNamep() && nodep->fmtp()->formatScopeTracking()) {
            nodep->fmtp()->scopeNamep(جديد AstScopeName{nodep->fileline(), true});
        }
    }
    AstSampled* new_Sampled_تعبير(Ast_عقدة_تعبير* nodep) {
        إرجاع جديد AstSampled{nodep->fileline(), nodep, nodep->dtypep(), true};
    }
    AstVarRef* new_Monitor_رقم_Var_Refp(ثابت Ast_عقدة* nodep, VAccess access) {
        إذا (!m_monitor_رقم_Varp) {
            m_monitor_رقم_Varp = جديد AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorNum",
                                          nodep->findUInt64DType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitor_رقم_Varp);
        }
        AstVarRef* ثابت varrefp = جديد AstVarRef{nodep->fileline(), m_monitor_رقم_Varp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        إرجاع varrefp;
    }
    AstVarRef* newMonitorOffVarRefp(ثابت Ast_عقدة* nodep, VAccess access) {
        إذا (!m_monitorOffVarp) {
            m_monitorOffVarp = جديد AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorOff",
                                          nodep->findBitDType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitorOffVarp);
        }
        AstVarRef* ثابت varrefp = جديد AstVarRef{nodep->fileline(), m_monitorOffVarp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        إرجاع varrefp;
    }
    ساكن AstIf* newIfAssertOn(Ast_عقدة* bodyp, VAssertDirectiveType directiveType,
                                VAssertType type = VAssertType::INTERNAL) {
        // Add an internal if to check assertions are on.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_سطر* ثابت fl = bodyp->fileline();

        Ast_عقدة_تعبير* ثابت condp = assertOnCond(fl, type, directiveType);
        AstIf* ثابت newp = جديد AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(true);  // To avoid LATCH warning
        newp->user2(true);  // Mark as an assertOn() check
        إرجاع newp;
    }
    ساكن Ast_عقدة_Stmt* newIfAssertPassOn(Ast_عقدة* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type, منطقي vacuous) {
        // Add an internal if to check assertion passOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_سطر* ثابت fl = bodyp->fileline();
        Ast_عقدة_تعبير* ثابت condp = assertPassOnCond(fl, type, directiveType, vacuous);
        Ast_عقدة_If* ثابت newp = جديد AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(true);  // To avoid LATCH warning
        newp->user1(true);  // Don't assert/cover this if
        newp->user2(true);  // Mark as an assertOn() check
        إرجاع newp;
    }
    ساكن Ast_عقدة_Stmt* newIfAssertFailOn(Ast_عقدة* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type) {
        // Add an internal if to check assertion failOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_سطر* ثابت fl = bodyp->fileline();
        Ast_عقدة_تعبير* ثابت condp = assertFailOnCond(fl, type, directiveType);
        Ast_عقدة_If* ثابت newp = جديد AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(true);  // To avoid LATCH warning
        newp->user1(true);  // Don't assert/cover this if
        newp->user2(true);  // Mark as an assertOn() check
        إرجاع newp;
    }

    ساكن AstIf* assertCond(ثابت Ast_عقدة_Cover_Or_Assert* nodep, Ast_عقدة_تعبير* propp,
                             Ast_عقدة* passsp, Ast_عقدة* failsp) {
        AstIf* ثابت ifp = جديد AstIf{nodep->fileline(), propp, passsp, failsp};
        // It's more LIKELY that we'll take the nullptr if clause
        // than the sim-killing else clause:
        ifp->branchPred(VBranchPred::BP_LIKELY);
        ifp->isBoundsCheck(true);  // To avoid LATCH warning
        إرجاع ifp;
    }

    Ast_عقدة_Stmt* assertBody(ثابت Ast_عقدة_Cover_Or_Assert* nodep, Ast_عقدة* propp, Ast_عقدة* passsp,
                            Ast_عقدة* failsp) {
        إذا (AstPExpr* ثابت pexprp = VN_CAST(propp, PExpr)) {
            إذا (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NOTIMING, "This property expression requires --timing");
                VL_DO_DANGLING(ادفع_Deletep(pexprp), pexprp);
                إرجاع جديد AstBegin{nodep->fileline(), "", nullptr, خطأ};
            }
            AstFork* ثابت forkp = جديد AstFork{nodep->fileline(), VJoinType::JOIN_NONE};
            forkp->addForksp(pexprp->bodyp()->unlinkFrBack());
            إذا (Ast_عقدة_Stmt* ثابت finalp = pexprp->finalp()) {
                إذا (!m_finalp) {
                    m_finalp = جديد AstFinal{m_modp->fileline(), finalp->unlinkFrBack()};
                    m_modp->addStmtsp(m_finalp);
                } وإلا {
                    m_finalp->addStmtsp(finalp->unlinkFrBack());
                }
            }
            VL_DO_DANGLING2(ادفع_Deletep(pexprp), pexprp, propp);
            إرجاع forkp;
        }

        إرجاع assertCond(nodep, VN_AS(propp, عقدة_تعبير), passsp, failsp);
    }

    Ast_عقدة_Stmt* newFireAssertUnchecked(ثابت Ast_عقدة_Stmt* nodep, ثابت string& message,
                                        Ast_عقدة_تعبير* exprsp = nullptr) {
        // Like newFireAssert() but omits the asserts-on check
        AstDisplay* ثابت dispp
            = جديد AstDisplay{nodep->fileline(), VDisplayType::DT_ERROR, message, nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        Ast_عقدة_Stmt* ثابت bodysp = dispp;
        replaceDisplay(dispp, "%%Error");  // Convert to standard DISPLAY format
        إذا (exprsp) dispp->fmtp()->exprsp()->addNext(exprsp);
        إذا (v3Global.opt.stopFail()) bodysp->addNext(جديد AstStop{nodep->fileline(), خطأ});
        إرجاع bodysp;
    }

    Ast_عقدة_Stmt* newFireAssert(ثابت Ast_عقدة_Stmt* nodep, VAssertDirectiveType directiveType,
                               VAssertType assertType, ثابت string& message,
                               Ast_عقدة_تعبير* exprsp = nullptr) {
        Ast_عقدة_Stmt* bodysp = newFireAssertUnchecked(nodep, message, exprsp);
        bodysp = newIfAssertOn(bodysp, directiveType, assertType);
        إرجاع bodysp;
    }

    AstVar* createDelayedVar(ثابت std::string& name, AstAlways* alwaysp, Ast_عقدة_تعبير* exprp,
                             Ast_عقدة_تعبير* initp) {
        File_سطر* ثابت flp = exprp->fileline();
        AstVar* ثابت varp = جديد AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        // TODO: this lifetime seems nonsene (can't have NBAs to automatics), but is as before
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->addStmtsp(varp);
        // Before enough clock ticks, $past returns the expression's default sampled value.
        إذا (initp) {
            m_modp->addStmtsp(جديد AstInitialStatic{
                flp, جديد AstAssign{flp, جديد AstVarRef{flp, varp, VAccess::WRITE}, initp}});
        }
        ++m_statPastVars;
        // Actually set the delayed value
        Ast_عقدة_تعبير* ثابت lhsp = جديد AstVarRef{flp, varp, VAccess::WRITE};
        AstAssignDly* ثابت assignp = جديد AstAssignDly{flp, lhsp, exprp};
        alwaysp->prependStmtsp(assignp);
        إرجاع varp;
    }

    AstAlways* getDelayedAlways(Ast_عقدة_تعبير* exprp, Ast_عقدة_تعبير* initp, Ast_Sen_شجرة* senTreep) {
        auto& expr2Sen2Alwaysr
            = initp ? m_modExpr2Sen2DefaultDelayedAlwaysp : m_modExpr2Sen2DelayedAlwaysp;
        AstAlways*& alwayspr = expr2Sen2Alwaysr[*exprp][*senTreep];
        إذا (!alwayspr) {
            File_سطر* ثابت flp = exprp->fileline();
            // Create the always block that computes the delayed values
            alwayspr = جديد AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, nullptr};
            m_modp->addStmtsp(alwayspr);
            // Create the once-delayed variable
            ثابت std::string name = "_Vpast_" + cvtToStr(m_mod_Past_رقم++) + "_1";
            AstVar* ثابت varp = createDelayedVar(name, alwayspr, exprp, initp);
            // Add it to delayed variable vector
            m_delayed(alwayspr).emplace_back(varp);
        } وإلا {
            // Reusing exiting, not needed
            VL_DO_DANGLING(ادفع_Deletep(exprp), exprp);
            إذا (initp) VL_DO_DANGLING(ادفع_Deletep(initp), initp);
            VL_DO_DANGLING(ادفع_Deletep(senTreep), senTreep);
        }
        إرجاع alwayspr;
    }

    Ast_عقدة_تعبير* get_Past_قيمة(Ast_عقدة_تعبير* exprp, Ast_عقدة_تعبير* initp, Ast_Sen_شجرة* senTreep,
                              uint32_t ticks) {
        UASSERT_OBJ(ticks > 0, exprp, "Delay must be > 0");
        File_سطر* ثابت flp = exprp->fileline();
        AstAlways* ثابت alwaysp
            = getDelayedAlways(exprp, initp ? initp->clone_شجرة_Pure(خطأ) : nullptr, senTreep);
        Ast_عقدة_تعبير* ثابت resultp = past_قيمة_Ref(alwaysp, flp, ticks, initp);
        إذا (initp) VL_DO_DANGLING(ادفع_Deletep(initp), initp);
        إرجاع resultp;
    }

    Ast_عقدة_تعبير* past_قيمة_Ref(AstAlways* alwaysp, File_سطر* flp, uint32_t ticks,
                              Ast_عقدة_تعبير* initp) {
        std::vector<AstVar*>& delayedr = m_delayed(alwaysp);
        // Ensure the required delay exists
        بينما (delayedr.size() < ticks) {
            AstVar* ثابت firstp = delayedr.front();
            File_سطر* ثابت varFlp = firstp->fileline();
            // Create once more delayed value
            std::string name = firstp->name();
            name.resize(name.size() - 1);
            name += std::to_string(delayedr.size() + 1);
            Ast_عقدة_تعبير* ثابت prevp = جديد AstVarRef{varFlp, delayedr.back(), VAccess::READ};
            AstVar* ثابت varp = createDelayedVar(name, alwaysp, prevp,
                                                  initp ? initp->clone_شجرة_Pure(خطأ) : nullptr);
            // Add it to delayed variable vector
            delayedr.emplace_back(varp);
        }
        // Return a reference to the appropriately delayed variable
        إرجاع جديد AstVarRef{flp, delayedr.at(ticks - 1), VAccess::READ};
    }

    // Time of the pipeline's last sampling tick, for end-of-simulation readers
    AstVar* getPastTickTimeVar(AstAlways* alwaysp) {
        AstVar*& varpr = m_delayedAlways2TickTimep[alwaysp];
        إذا (!varpr) {
            File_سطر* ثابت flp = alwaysp->fileline();
            varpr = جديد AstVar{flp, VVarType::MODULETEMP,
                               "_Vpast_" + cvtToStr(m_mod_Past_رقم++) + "_t",
                               m_modp->findUInt64DType()};
            m_modp->addStmtsp(varpr);
            Ast_عقدة_تعبير* ثابت timep = جديد AstCExpr{flp, "vlSymsp->_vm_contextp__->time()", 64};
            alwaysp->addStmtsp(
                جديد AstAssign{flp, جديد AstVarRef{flp, varpr, VAccess::WRITE}, timep});
        }
        إرجاع varpr;
    }

    فراغ visitAssertionIterate(Ast_عقدة_Cover_Or_Assert* nodep, Ast_عقدة* failsp) {
        إذا (m_beginp && nodep->name() == "") nodep->name(m_beginp->name());

        { AssertDeFutureVisitor{nodep->propp(), m_modp, m_mod_Past_رقم++}; }

        iterateAndNextNull(nodep->sentreep());
        إذا (AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            iterateAndNextNull(assertp->failsp());
        } وإلا إذا (AstAssertIntrinsic* ثابت assertp = VN_CAST(nodep, AssertIntrinsic)) {
            iterateAndNextNull(assertp->failsp());
        } وإلا إذا (AstCover* ثابت coverp = VN_CAST(nodep, Cover)) {
            iterateAndNextNull(coverp->coverincsp());
        } وإلا إذا (!VN_IS(nodep, Restrict)) {
            nodep->v3fatalSrc("Unhandled assert type");
        }
        iterateAndNextNull(nodep->passsp());
        Ast_Sen_شجرة* sentreep = nodep->sentreep();
        إذا (nodep->immediate()) {
            UASSERT_OBJ(!sentreep, nodep, "Immediate assertions don't have sensitivity");
        } وإلا {
            UASSERT_OBJ(sentreep, nodep, "Concurrent assertions must have sensitivity");
            // Explicit inline clock differs from the enclosing always: hoist
            // and warn. To support this need queue of asserts to activate.
            إذا (m_procedurep && !nodep->senFromAlways()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Procedural concurrent assertion with"
                              " clocking event inside always (IEEE 1800-2023 16.14.6)");
                static_cast<Ast_عقدة*>(m_procedurep)->addNext(nodep->unlinkFrBack());
                إرجاع;  // Later iterate will pick up
            }
            sentreep->unlinkFrBack();
            إذا (m_procedurep) { VL_DO_DANGLING(ادفع_Deletep(sentreep), sentreep); }
        }
        //
        ثابت string& message = nodep->name();
        Ast_عقدة* passsp = nodep->passsp();
        إذا (passsp) passsp->unlinkFrBackWithNext();
        إذا (AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            failsp = assertp->failsp();
        } وإلا إذا (AstAssertIntrinsic* ثابت assertp = VN_CAST(nodep, AssertIntrinsic)) {
            failsp = assertp->failsp();
        }
        إذا (failsp) failsp->unlinkFrBackWithNext();

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
                selfDestruct = true;
            } وإلا {
                // V3Coverage assigned us a bucket to increment.
                AstCoverInc* ثابت covincp = VN_AS(coverp->coverincsp(), CoverInc);
                UASSERT_OBJ(covincp, coverp, "Missing AstCoverInc under assertion");
                covincp->unlinkFrBackWithNext();  // next() might have  AstAssign for trace
                إذا (message != "") covincp->declp()->comment(message);
                إذا (passsp) {
                    passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                               /*vacuous=*/خطأ);
                    passspGated = true;
                    passsp = Ast_عقدة::addNext<Ast_عقدة, Ast_عقدة>(covincp, passsp);
                } وإلا {
                    passsp = covincp;
                }
            }
        } وإلا إذا (VN_IS(nodep, Assert) || VN_IS(nodep, AssertIntrinsic)) {
            إذا (nodep->immediate()) {
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

        Ast_عقدة* propExprp;
        Ast_عقدة_تعبير* disablep = nullptr;
        Ast_عقدة_تعبير* matchCountp = nullptr;
        إذا (AstPropSpec* ثابت specp = VN_CAST(nodep->propp(), PropSpec)) {
            propExprp = specp->propp()->unlinkFrBack();
            إذا (specp->disablep()) disablep = specp->disablep()->unlinkFrBack();
            matchCountp = specp->matchCountp();
        } وإلا {
            propExprp = nodep->propp()->unlinkFrBack();
        }
        File_سطر* ثابت flp = nodep->fileline();
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
        إذا (coverp && matchCountp && passsp) {
            // Convert the match count into a loop that decrements a temporary variable until it
            // reaches zero.
            matchCountp->unlinkFrBack();
            AstVar* ثابت remainingp = جديد AstVar{
                flp, VVarType::MODULETEMP, m_match_تعداد_Names.get(""), matchCountp->dtypep()};
            remainingp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remainingp);
            Ast_عقدة* ثابت replaysp
                = جديد AstAssign{flp, جديد AstVarRef{flp, remainingp, VAccess::WRITE}, matchCountp};
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            loopp->addStmtsp(
                جديد AstLoopTest{flp, loopp, جديد AstVarRef{flp, remainingp, VAccess::READ}});
            loopp->addStmtsp(passsp);
            loopp->addStmtsp(
                جديد AstAssign{flp, جديد AstVarRef{flp, remainingp, VAccess::WRITE},
                              جديد AstSub{flp, جديد AstVarRef{flp, remainingp, VAccess::READ},
                                         جديد AstConst{flp, AstConst::Widthed_قيمة{},
                                                      remainingp->dtypep()->width(), 1}}});
            replaysp->addNext(loopp);
            passsp = replaysp;
        }
        Ast_عقدة* bodysp = assertBody(nodep, propExprp, passsp, failsp);
        إذا (disablep) bodysp = جديد AstIf{flp, جديد AstLogNot{flp, disablep}, bodysp};
        // Add assertOn check last, for better combining
        إذا (!seqEvent) bodysp = newIfAssertOn(bodysp, nodep->directive(), nodep->userType());
        إذا (sentreep) bodysp = جديد AstAlways{flp, VAlwaysKwd::ALWAYS, sentreep, bodysp};

        إذا (passsp && !passsp->backp()) VL_DO_DANGLING(ادفع_Deletep(passsp), passsp);
        إذا (failsp && !failsp->backp()) VL_DO_DANGLING(ادفع_Deletep(failsp), failsp);

        // Install it
        إذا (selfDestruct) {
            // Delete it after making the tree.  This way we can tell the user
            // if it wasn't constructed nicely or has other errors without needing --coverage.
            VL_DO_DANGLING(bodysp->delete_شجرة(), bodysp);
            nodep->unlinkFrBack();
        } وإلا {
            nodep->replaceWith(bodysp);
        }
        // Bye
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }

    منطقي isEmptyStmt(Ast_عقدة* nodep) {
        إذا (!nodep) إرجاع true;
        إذا (AstBegin* ثابت beginp = VN_CAST(nodep, Begin)) {
            إذا (beginp->declsp()) إرجاع خطأ;
            لكل (Ast_عقدة* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (!isEmptyStmt(stmtp)) إرجاع خطأ;
            }
            إرجاع true;
        }
        إرجاع خطأ;
    }

    // VISITORS
    فراغ visit(AstIf* nodep) override {
        إذا (!nodep->user1SetOnce()) {
            إذا (nodep->uniquePragma() || nodep->unique0Pragma()) {
                ثابت Ast_عقدة_If* ifp = nodep;
                Ast_عقدة_تعبير* propp = nullptr;
                منطقي hasDefaultElse = خطأ;
                افعل {
                    // If this statement ends with 'else if', then nextIf will point to the
                    // nextIf statement.  Otherwise it will be null.
                    ثابت Ast_عقدة_If* ثابت nextifp = dynamic_cast<Ast_عقدة_If*>(ifp->elsesp());
                    iterateAndNextNull(ifp->condp());

                    // Recurse into the true case.
                    iterateAndNextNull(ifp->thensp());

                    // If the last else is not an else if, recurse into that too.
                    إذا (ifp->elsesp() && !nextifp) {  //
                        iterateAndNextNull(ifp->elsesp());
                    }

                    // Build a bitmask of the true predicates
                    Ast_عقدة_تعبير* ثابت predp = ifp->condp()->clone_شجرة_Pure(خطأ);
                    إذا (propp) {
                        propp = جديد AstConcat{nodep->fileline(), predp, propp};
                    } وإلا {
                        propp = predp;
                    }

                    // Record if this ends with an 'else' that does not have an if
                    إذا (ifp->elsesp() && !nextifp) hasDefaultElse = true;

                    ifp = nextifp;
                } بينما (ifp);

                ثابت منطقي allow_none = nodep->unique0Pragma();

                // Empty case means no property
                إذا (!propp) propp = جديد AstConst{nodep->fileline(), AstConst::BitFalse{}};

                // Note: if this ends with an 'else', then we don't need to validate that one of
                // the predicates evaluates to true.
                Ast_عقدة_تعبير* ثابت ohot
                    = ((allow_none || hasDefaultElse)
                           ? static_cast<Ast_عقدة_تعبير*>(جديد AstOneHot0{nodep->fileline(), propp})
                           : static_cast<Ast_عقدة_تعبير*>(جديد AstOneHot{nodep->fileline(), propp}));
                ثابت VAssertType assertType
                    = nodep->uniquePragma() ? VAssertType::UNIQUE : VAssertType::UNIQUE0;
                // Move, not clone, as the branches might declare referenced variables
                VNRelinker relinker;
                nodep->unlinkFrBack(&relinker);
                AstIf* ثابت checkifp
                    = جديد AstIf{nodep->fileline(), جديد AstLogNot{nodep->fileline(), ohot},
                                newFireAssert(nodep, VAssertDirectiveType::VIOLATION_IF,
                                              assertType, "'unique if' statement violated"),
                                nodep};
                checkifp->isBoundsCheck(true);  // To avoid LATCH warning
                checkifp->branchPred(VBranchPred::BP_UNLIKELY);
                relinker.relink(checkifp);
                إرجاع;
            }
        }

        iterateChildren(nodep);

        إذا (nodep->user2()) {
            // Combine consecutive assertOn checks if possible
            إذا (AstIf* ثابت backp = VN_CAST(nodep->backp(), If)) {
                إذا (backp->nextp() == nodep  //
                    && backp->user2()  //
                    && backp->condp()->same_شجرة(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    backp->addThensp(nodep->thensp()->unlinkFrBackWithNext());
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
                    إرجاع;
                }
            }
            // Combine nested assertOn checks if possible
            إذا (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* ثابت checkp = VN_CAST(nodep->thensp(), If);
                إذا (checkp  //
                    && checkp->user2()  //
                    && checkp->condp()->same_شجرة(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    VL_DO_DANGLING(ادفع_Deletep(checkp->unlinkFrBack()), checkp);
                    إرجاع;
                }
            }
            إرجاع;
        }

        // Swap assertOn check with single statement 'if' statement to bubble up for combining
        // Note we can't just swap the conditions as they two Ifs have different flags,
        // so swapping the Ifs themeselves then swapping back the bodies.
        إذا (nodep->condp()->isPure()) {
            إذا (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* ثابت checkp = VN_CAST(nodep->thensp(), If);
                إذا (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    إرجاع;
                }
            }
            إذا (nodep->elsesp() && !nodep->elsesp()->nextp() && isEmptyStmt(nodep->thensp())) {
                AstIf* ثابت checkp = VN_CAST(nodep->elsesp(), If);
                إذا (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addElsesp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    إرجاع;
                }
            }
        }
    }

    //========== Case assertions
    فراغ visit(AstCase* nodep) override {
        // Introduce temporary variable for AstCase if needed - it is done here and not in V3Case
        // because this phase is before V3Scope and V3Case is not. Doing it before V3Scope ensures
        // that V3Scope will take care of a scope creation
        // We also need to do it before V3Begin, co that pragmas like `unique0` also work correctly
        إذا (!nodep->exprp()->isPure()) {
            ++m_statLiftedCaseExprs;
            File_سطر* ثابت fl = nodep->exprp()->fileline();
            AstVar* ثابت varp = جديد AstVar{fl, VVarType::BLOCKTEMP, m_caseTempNames.get(nodep),
                                            nodep->exprp()->dtypep()};
            Ast_عقدة_تعبير* ثابت origp = nodep->exprp()->unlinkFrBack();
            nodep->addHereThisAsNext(
                جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, origp});
            nodep->exprp(جديد AstVarRef{fl, varp, VAccess::READ});
            إذا (m_ftaskp) {
                varp->funcLocal(true);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } وإلا {
                m_modp->prependStmtsp(varp);
            }
            VIsCached::clear_Cache_شجرة();
        }
        iterateChildren(nodep);
        إذا (!nodep->user1SetOnce()) {
            منطقي has_default = خطأ;
            لكل (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                إذا (itemp->isDefault()) has_default = true;
            }
            ثابت Ast_عقدة_DType* تعبير_Dtypep = nodep->exprp()->dtypep()->skipRefp();

            VAssertType assertType = VAssertType::INTERNAL;
            إذا (nodep->priorityPragma()) {
                assertType = VAssertType::PRIORITY;
            } وإلا إذا (nodep->uniquePragma()) {
                assertType = VAssertType::UNIQUE;
            } وإلا إذا (nodep->unique0Pragma()) {
                assertType = VAssertType::UNIQUE0;
            }

            string valFmt;
            إذا (تعبير_Dtypep->isIntegralOrPacked())
                valFmt = " for '" + cvtToStr(تعبير_Dtypep->widthMin()) + "'h%X'";
            إذا (nodep->fullPragma() || nodep->priorityPragma()) {
                // Need to add a default if there isn't one already
                ++m_statAsFull;
                إذا (!has_default) {
                    nodep->addItemsp(جديد AstCaseItem{
                        nodep->fileline(), nullptr /*DEFAULT*/,
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      nodep->pragmaString() + ", but non-match found" + valFmt,
                                      valFmt.empty() ? nullptr
                                                     : nodep->exprp()->clone_شجرة_Pure(خطأ))});
                }
            }
            إذا (nodep->parallelPragma() || nodep->uniquePragma() || nodep->unique0Pragma()) {
                // Need to check that one, and only one of the case items match at any moment
                // If there's a default, we allow none to match, else exactly one must match
                ++m_statAsFull;
                إذا (!has_default && !nodep->itemsp()) {
                    // Not parallel, but harmlessly so.
                } وإلا {
                    Ast_عقدة_تعبير* propp = nullptr;
                    لكل (AstCaseItem* itemp = nodep->itemsp(); itemp;
                         itemp = VN_AS(itemp->nextp(), CaseItem)) {
                        Ast_عقدة_تعبير* itembitp = nullptr;
                        لكل (Ast_عقدة_تعبير* icondp = itemp->condsp(); icondp;
                             icondp = VN_AS(icondp->nextp(), عقدة_تعبير)) {
                            Ast_عقدة_تعبير* onep;
                            إذا (AstInsideRange* ثابت rcondp = VN_CAST(icondp, InsideRange)) {
                                onep = rcondp->newAndFromInside(
                                    nodep->exprp()->clone_شجرة_Pure(true),
                                    rcondp->lhsp()->clone_شجرة_Pure(true),
                                    rcondp->rhsp()->clone_شجرة_Pure(true));
                            } وإلا إذا (nodep->casex() || nodep->casez() || nodep->caseInside()) {
                                onep = AstEqWild::newTyped(itemp->fileline(),
                                                           nodep->exprp()->clone_شجرة_Pure(خطأ),
                                                           icondp->clone_شجرة_Pure(خطأ));
                            } وإلا {
                                onep = AstEq::newTyped(icondp->fileline(),
                                                       nodep->exprp()->clone_شجرة_Pure(خطأ),
                                                       icondp->clone_شجرة_Pure(خطأ));
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
                    إذا (!propp) propp = جديد AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    ثابت منطقي allow_none = has_default || nodep->unique0Pragma();
                    // The following assertion looks as below.
                    // if (!$onehot(propp)) begin
                    //     if (propp == '0) begin if (!allow_none) $error("none match"); end
                    //     else $error("multiple match");
                    // end
                    Ast_عقدة_تعبير* ثابت ohot = جديد AstOneHot{nodep->fileline(), propp};
                    AstConst* ثابت zero = جديد AstConst{
                        nodep->fileline(), AstConst::Widthed_قيمة{}, propp->width(), 0};
                    AstIf* ثابت ohotIfp
                        = جديد AstIf{nodep->fileline(), جديد AstLogNot{nodep->fileline(), ohot}};
                    AstIf* ثابت zeroIfp = جديد AstIf{
                        nodep->fileline(),
                        جديد AstEq{nodep->fileline(), propp->clone_شجرة_Pure(خطأ), zero}};
                    Ast_عقدة_تعبير* ثابت exprp = nodep->exprp();
                    ثابت string pragmaStr = nodep->pragmaString();
                    إذا (!allow_none)
                        zeroIfp->addThensp(
                            newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                          pragmaStr + ", but none matched" + valFmt,
                                          valFmt.empty() ? nullptr : exprp->clone_شجرة_Pure(خطأ)));
                    zeroIfp->addElsesp(
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      pragmaStr + ", but multiple matches found" + valFmt,
                                      valFmt.empty() ? nullptr : exprp->clone_شجرة_Pure(خطأ)));
                    ohotIfp->addThensp(zeroIfp);
                    ohotIfp->isBoundsCheck(true);  // To avoid LATCH warning
                    ohotIfp->branchPred(VBranchPred::BP_UNLIKELY);
                    nodep->addNotParallelp(ohotIfp);
                }
            }
        }
    }

    فراغ visit(AstFuture* nodep) override {
        nodep->v3error("Future sampled value function called outside property or sequence "
                       "expression (IEEE 16.9.4)");
        nodep->replaceWith(جديد AstConst{nodep->fileline(), 0});
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }

    //========== Past
    فراغ visit(AstPast* nodep) override {
        إذا (!nodep->propertyTiming()) {
            nodep->initp(nodep->exprp()->clone_شجرة_Pure(خطأ));
            { Default_Sampled_قيمة_Visitor{nodep->initp()}; }
        }
        Ast_عقدة_تعبير* ثابت initp = nodep->initp() ? nodep->initp()->unlinkFrBack() : nullptr;
        iterateChildren(nodep);
        uint32_t ticks = 1;
        إذا (nodep->ticksp()) {
            UASSERT_OBJ(VN_IS(nodep->ticksp(), Const), nodep,
                        "Expected constant ticks, checked in V3Width");
            ticks = VN_AS(nodep->ticksp(), Const)->toUInt();
        }
        UASSERT_OBJ(ticks >= 1, nodep, "0 tick should have been checked in V3Width");
        Ast_عقدة_تعبير* ثابت exprp = new_Sampled_تعبير(nodep->exprp()->unlinkFrBack());
        Ast_Sen_شجرة* ثابت senTreep = nodep->sentreep()->unlinkFrBack();
        Ast_عقدة_تعبير* inp = nullptr;
        // Deliberately emitted for every ftask $past; calls outside final take the normal stage
        إذا (VN_IS(m_procedurep, Final) || m_ftaskp) {
            // A sampling-tick finish reads one stage deeper (IEEE 1800-2023 16.9.3).
            File_سطر* ثابت flp = nodep->fileline();
            AstAlways* ثابت alwaysp
                = getDelayedAlways(exprp, initp ? initp->clone_شجرة_Pure(خطأ) : nullptr, senTreep);
            Ast_عقدة_تعبير* ثابت betweenTicksp = past_قيمة_Ref(alwaysp, flp, ticks, initp);
            Ast_عقدة_تعبير* ثابت onTickp = past_قيمة_Ref(alwaysp, flp, ticks + 1, initp);
            إذا (initp) VL_DO_DANGLING(ادفع_Deletep(initp), initp);
            Ast_عقدة_تعبير* ثابت onTickCondp = جديد AstLogAnd{
                flp, جديد AstCExpr{flp, "vlSymsp->_vm_contextp__->executingFinal()", 1},
                جديد AstEq{flp, جديد AstVarRef{flp, getPastTickTimeVar(alwaysp), VAccess::READ},
                          جديد AstCExpr{flp, "vlSymsp->_vm_contextp__->finishPendingTime()", 64}}};
            AstCond* ثابت condp = جديد AstCond{flp, onTickCondp, onTickp, betweenTicksp};
            condp->dtypeFrom(onTickp);
            inp = condp;
        } وإلا {
            inp = get_Past_قيمة(exprp, initp, senTreep, ticks);
        }
        nodep->replaceWith(inp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }

    //========== Move $sampled down to read-only variables
    فراغ visit(AstSampled* nodep) override {
        إذا (nodep->user1()) إرجاع;
        VL_RESTORER(m_inSampled);
        {
            m_inSampled = true;
            iterateChildren(nodep);
        }
        إذا (nodep->exprp()) {
            nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        } وإلا {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }
    فراغ visit(Ast_عقدة_Var_Ref* nodep) override {
        إذا (m_inSampled && !nodep->user2() && !nodep->varp()->isTemp()) {
            إذا (!nodep->access().isReadOnly()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Write to variable in sampled expression");
            } وإلا {
                VNRelinker relinkHandle;
                nodep->unlinkFrBack(&relinkHandle);
                AstSampled* ثابت newp = new_Sampled_تعبير(nodep);
                relinkHandle.relink(newp);
                newp->user1(1);
                v3Global.setHasSampled();
            }
        }
    }
    // Don't sample sensitivities
    فراغ visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSampled);
        m_inSampled = خطأ;
        iterateChildren(nodep);
    }
    فراغ visit(AstPExprClause* nodep) override {
        Ast_عقدة* stmtsp = nullptr;
        إذا (nodep->pass() && m_passsp) {
            // Cover adds COVERINC by AstNode::addNext, thus need to clone next too.
            stmtsp = m_passsp->clone_شجرة(true);
            stmtsp = newIfAssertPassOn(stmtsp, m_assertp->directive(), m_assertp->userType(),
                                       nodep->vacuous());
        } وإلا إذا (!nodep->pass() && m_failsp) {
            stmtsp = m_failsp->clone_شجرة(true);
            stmtsp = newIfAssertFailOn(stmtsp, m_assertp->directive(), m_assertp->userType());
        }
        إذا (stmtsp) {
            stmtsp->foreachAndNext([](Ast_عقدة_Var_Ref* ثابت refp) {
                // References inside action blocks shouldn't be implicitly sampled
                // m_passsp/m_failsp have been already visited once and refs explicitly sampled
                // are handled already
                refp->user2(1);
            });
            nodep->replaceWith(stmtsp);
        } وإلا {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }
    فراغ visit(AstPExpr* nodep) override {
        إذا (m_inRestrict) {
            VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
        } وإلا {
            iterateChildren(nodep);
        }
    }

    //========== Statements
    فراغ visit(AstDisplay* nodep) override {
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
            File_سطر* ثابت fl = nodep->fileline();

            AstSenItem* monSenItemsp = nullptr;
            إذا (Ast_عقدة* ثابت monExprsp = nodep->fmtp()->exprsp()) {
                monExprsp->foreachAndNext([&](AstVarRef* varrefp) {
                    AstSenItem* ثابت senItemp
                        = جديد AstSenItem{fl, VEdgeType::ET_CHANGED,
                                         // Clone so get VarRef or VarXRef as needed
                                         varrefp->clone_شجرة(خطأ)};
                    monSenItemsp = Ast_عقدة::addNextNull(monSenItemsp, senItemp);
                });
            }

            Ast_Sen_شجرة* ثابت mon_Sen_شجرة = جديد Ast_Sen_شجرة{fl, monSenItemsp};
            ثابت auto mon_رقم = ++m_monitor_رقم;
            // Where $monitor was we do "__VmonitorNum = N;"
            AstAssign* ثابت newsetp = جديد AstAssign{
                fl, new_Monitor_رقم_Var_Refp(nodep, VAccess::WRITE), جديد AstConst{fl, mon_رقم}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__VmonitorOn && __VmonitorNum==N) $display(...);"
            Ast_عقدة* ثابت stmtsp = nodep;
            AstIf* ثابت ifp = جديد AstIf{
                fl,
                جديد AstLogAnd{fl, جديد AstLogNot{fl, newMonitorOffVarRefp(nodep, VAccess::READ)},
                              جديد AstEq{fl, جديد AstConst{fl, mon_رقم},
                                        new_Monitor_رقم_Var_Refp(nodep, VAccess::READ)}},
                stmtsp};
            ifp->isBoundsCheck(true);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_عقدة* ثابت newp = جديد AstAlways{fl, VAlwaysKwd::ALWAYS, mon_Sen_شجرة, ifp};
            m_modp->addStmtsp(newp);
        } وإلا إذا (nodep->displayType() == VDisplayType::DT_STROBE) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            // Need one-shot
            File_سطر* ثابت fl = nodep->fileline();
            AstVar* ثابت varp
                = جديد AstVar{fl, VVarType::MODULETEMP, "__Vstrobe" + cvtToStr(m_mod_Strobe_رقم++),
                             nodep->findBitDType()};
            m_modp->addStmtsp(varp);
            // Where $strobe was we do "__Vstrobe = '1;"
            AstAssign* ثابت newsetp = جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                                     جديد AstConst{fl, AstConst::BitTrue{}}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__Vstrobe) begin $display(...); __Vstrobe = '0; end"
            Ast_عقدة* ثابت stmtsp = nodep;
            AstIf* ثابت ifp = جديد AstIf{fl, جديد AstVarRef{fl, varp, VAccess::READ}, stmtsp};
            ifp->isBoundsCheck(true);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_عقدة* ثابت newp = جديد AstAlwaysPostponed{fl, ifp};
            stmtsp->addNext(جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                          جديد AstConst{fl, AstConst::BitFalse{}}});
            m_modp->addStmtsp(newp);
        }
    }
    فراغ visit(AstMonitorOff* nodep) override {
        AstAssign* ثابت newp
            = جديد AstAssign{nodep->fileline(), newMonitorOffVarRefp(nodep, VAccess::WRITE),
                            جديد AstConst{nodep->fileline(), AstConst::BitTrue{}, nodep->off()}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }
    فراغ visit(AstAssert* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    فراغ visit(AstAssertCtl* nodep) override {
        iterateChildren(nodep);

        منطقي assertTypeConst = true;
        إذا (!nodep->assertTypesp()) {
            nodep->assertTypes(VAssertType{ALL_ASSERT_TYPES});
        } وإلا إذا (ثابت AstConst* ثابت assertTypesp = VN_CAST(nodep->assertTypesp(), Const)) {
            nodep->assertTypes(VAssertType{assertTypesp->toSInt()});
        } وإلا {
            assertTypeConst = خطأ;
        }

        منطقي controlTypeConst = خطأ;
        إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep->controlTypep(), Const)) {
            nodep->ctlType(constp->toSInt());
            controlTypeConst = true;
        }
        إذا (controlTypeConst
            && (nodep->ctlType() < VAssertCtlType::LOCK
                || nodep->ctlType() > VAssertCtlType::VACUOUS_OFF)) {
            nodep->unlinkFrBack();
            nodep->v3error("Bad $assertcontrol control_type '"
                           << cvtToStr(static_cast<صحيح>(nodep->ctlType()))
                           << "' (IEEE 1800-2023 Table 20-5)");
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
            إرجاع;
        }
        إذا (assertTypeConst && nodep->assertTypes() != ALL_ASSERT_TYPES
            && nodep->assertTypes().containsAny(VAssertType::UNIQUE | VAssertType::UNIQUE0
                                                | VAssertType::PRIORITY)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: assert control assertion_type");
            VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
            إرجاع;
        }

        منطقي directiveTypeConst = true;
        إذا (!nodep->directiveTypesp()) {
            nodep->directiveTypes(VAssertDirectiveType::ASSERT | VAssertDirectiveType::ASSUME
                                  | VAssertDirectiveType::COVER);
        } وإلا إذا (ثابت AstConst* ثابت directiveTypesp
                   = VN_CAST(nodep->directiveTypesp(), Const)) {
            nodep->directiveTypes(VAssertDirectiveType{directiveTypesp->toSInt()});
        } وإلا {
            directiveTypeConst = خطأ;
        }
        إذا (!directiveTypeConst) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: non-const assert directive type expression");
            VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
            إرجاع;
        }

        File_سطر* ثابت fl = nodep->fileline();
        UINFO(9, "Generating assertctl for a module: " << m_modp);
        AstCStmt* ثابت newp = جديد AstCStmt{fl};
        newp->add("vlSymsp->_vm_contextp__->assertCtl(");
        newp->add(nodep->controlTypep()->unlinkFrBack());
        newp->add(", ");
        إذا (nodep->assertTypesp()) {
            newp->add(nodep->assertTypesp()->unlinkFrBack());
        } وإلا {
            newp->add(std::to_string(ALL_ASSERT_TYPES));
        }
        newp->add(", " + std::to_string(nodep->directiveTypes()) + ");\n");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
    }
    فراغ visit(AstAssertIntrinsic* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    فراغ visit(AstCover* nodep) override {  //
        visitAssertionIterate(nodep, nullptr);
    }
    فراغ visit(AstRestrict* nodep) override {
        VL_RESTORER(m_inRestrict);
        m_inRestrict = true;
        iterateChildren(nodep);
        // IEEE says simulator ignores these
        VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);
    }

    فراغ visit(Ast_عقدة_Module* nodep) override {
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
    فراغ visit(Ast_عقدة_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(Ast_عقدة_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstGenBlock* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstBegin* nodep) override {
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
                    nodep->replaceWith(ifp->unlinkFrBack());
                    nodep->addStmtsp(ifp->thensp()->unlinkFrBackWithNext());
                    ifp->addThensp(nodep);
                }
            }
        }
    }

    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
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
        VIsCached::clear_Cache_شجرة();
    }
};

//######################################################################
// Top Assert class

فراغ V3Assert::assertAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_شجرة("assert", 0, dump_شجرة_Either_Level() >= 3);
}
