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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Assert.h"

#অন্তর্ভাব "V3AstUserAllocator.h"
#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3Stats.h"
#অন্তর্ভাব "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ {

বর্গ DefaultDisableLocalVisitor final : সার্বজনিক VNVisitor {
    // STATE
    Ast_গ্রন্থি* m_scopep = nullptr;

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(nullptr);
        iterateChildren(nodep);
    }
    শূন্য visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(nullptr);
        iterateChildren(nodep);
    }
    শূন্য visit(AstDefaultDisable* nodep) override {
        UASSERT_OBJ(nodep, m_scopep,
                    "default disable iff must be inside a module or generate block");
        AstDefaultDisable* defaultp = nullptr;
        যদি (ধ্রুব Ast_গ্রন্থি_Module* ধ্রুব modp = VN_CAST(m_scopep, গ্রন্থি_Module)) {
            defaultp = modp->defaultDisablep();
        } অন্যথা {
            defaultp = VN_AS(m_scopep, GenBlock)->defaultDisablep();
        }
        যদি (VL_UNLIKELY(defaultp)) {
            nodep->v3error("Only one 'default disable iff' allowed per "
                           << (VN_IS(m_scopep, গ্রন্থি_Module) ? "module" : "generate block")
                           << " (IEEE 1800-2023 16.15)");
        } অন্যথা যদি (Ast_গ্রন্থি_Module* ধ্রুব modp = VN_CAST(m_scopep, গ্রন্থি_Module)) {
            modp->defaultDisablep(nodep);
        } অন্যথা {
            VN_AS(m_scopep, GenBlock)->defaultDisablep(nodep);
        }
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    explicit DefaultDisableLocalVisitor(AstNetlist* nodep) { iterate(nodep); }
};

বর্গ DefaultDisablePropagateVisitor final : সার্বজনিক VNVisitor {
    // STATE
    AstDefaultDisable* m_defaultDisablep = nullptr;

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    শূন্য visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        যদি (!nodep->defaultDisablep()) nodep->defaultDisablep(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    explicit DefaultDisablePropagateVisitor(AstNetlist* nodep) { iterate(nodep); }
};

// Lower a sequence used as an event control ('@seq', IEEE 1800-2023 9.4.2.4) into a
// synthesized event fired by an internal 'cover sequence' on each end-of-match
বর্গ SeqEventLowerVisitor final : সার্বজনিক VNVisitor {
    // STATE
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module
    V3UniqueNames m_names{"__VseqEvent"};  // Synthesized event names

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstSenItem* nodep) override {
        AstFuncRef* ধ্রুব funcrefp = VN_CAST(nodep->sensp(), FuncRef);
        যদি (funcrefp && VN_IS(funcrefp->taskp(), Sequence)) {
            File_সারি* ধ্রুব flp = nodep->fileline();
            AstVar* ধ্রুব eventp = নূতন AstVar{flp, VVarType::MODULETEMP, m_names.get(nodep),
                                              m_modp->findBasicDType(VBasicDTypeKwd::EVENT)};
            eventp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(eventp);
            v3Global.setHasEvents();
            funcrefp->unlinkFrBack();
            nodep->sensp(নূতন AstVarRef{flp, eventp, VAccess::READ});
            ধ্রুব তার্কিক automaticActual = funcrefp->exists([](ধ্রুব Ast_গ্রন্থি_Var_Ref* refp) {
                প্রত্যাগম refp->varp() && refp->varp()->lifetime().isAutomatic();
            });
            যদি (automaticActual) {
                nodep->v3error("Arguments to a sequence used as an event control must be"
                               " static (IEEE 1800-2023 9.4.2.4)");
                VN_AS(funcrefp->taskp(), Sequence)->isReferenced(অসত্য);
                VL_DO_DANGLING(ঠেলো_Deletep(funcrefp), funcrefp);
                প্রত্যাগম;
            }
            AstFireEvent* ধ্রুব firep
                = নূতন AstFireEvent{flp, নূতন AstVarRef{flp, eventp, VAccess::WRITE}, অসত্য};
            AstCover* ধ্রুব coverp
                = নূতন AstCover{flp, নূতন AstPropSpec{flp, nullptr, nullptr, funcrefp}, firep,
                               VAssertType::CONCURRENT};
            coverp->isCoverSeq(সত্য);
            coverp->isSeqEvent(সত্য);
            m_modp->addStmtsp(coverp);
            প্রত্যাগম;
        }
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    explicit SeqEventLowerVisitor(AstNetlist* nodep) { iterate(nodep); }
};

}  // namespace

শূন্য V3AssertCommon::collectDefaultDisable(AstNetlist* nodep) {
    { DefaultDisableLocalVisitor{nodep}; }
    { DefaultDisablePropagateVisitor{nodep}; }
}

শূন্য V3AssertCommon::lowerSequenceEvents(AstNetlist* nodep) {
    { SeqEventLowerVisitor{nodep}; }
    V3Global::dump_Check_Global_বৃক্ষ("assertseqevent", 0, dump_বৃক্ষ_Either_Level() >= 3);
}

string V3AssertCommon::assertCtlGetCall(ধ্রুব বর্ণ* query, VAssertType type,
                                        VAssertDirectiveType directiveType) {
    // Template arguments let the runtime build the type/directive mask at C++ compile time
    প্রত্যাগম "vlSymsp->_vm_contextp__->assertCtlGet<"s + std::to_string(type) + ", "s
           + std::to_string(directiveType) + ">(VerilatedAssertCtlQuery::"s + query + ")"s;
}

তার্কিক V3AssertCommon::check_Sva_মান_Limit(Ast_গ্রন্থি* nodep, ধ্রুব AstConst* constp, ধ্রুব বর্ণ* what) {
    // Temporal values can materialize as O(N) AST or state in later passes.
    // Leave headroom for a range ring's extra slot and V3Number's signed
    // `(bits + 31) / 32` word count.
    স্থির constexpr unsigned SVA_মান_HARD_LIMIT
        = static_cast<unsigned>(std::numeric_limits<পূর্ণাঙ্ক>::max()) - 32U;
    ধ্রুব unsigned configuredLimit = static_cast<unsigned>(v3Global.opt.max_অঙ্ক_Width());
    ধ্রুব unsigned limit = std::min(configuredLimit, SVA_মান_HARD_LIMIT);
    যদি (constp->অঙ্ক().fitsInUInt() && constp->toUInt() <= limit) প্রত্যাগম সত্য;
    nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << what << " exceeds implementation limit of "
                                                 << limit
                                                 << (configuredLimit <= SVA_মান_HARD_LIMIT
                                                         ? " (--max-num-width)"
                                                         : " (host arithmetic limit)"));
    প্রত্যাগম অসত্য;
}

// The bound of a cycle delay in a sequence, if a non-negative elaboration-time constant
স্থির ধ্রুব AstConst* checkSvaDelayBound(Ast_গ্রন্থি* errp, Ast_গ্রন্থি_রাশিমালা* boundp, ধ্রুব বর্ণ* what) {
    ধ্রুব AstConst* ধ্রুব constp = VN_CAST(boundp, Const);
    যদি (!constp || (constp->dtypep()->isSigned() && constp->অঙ্ক().isNegative())) {
        errp->v3error(what << " is not a non-negative elaboration-time constant"
                              " (IEEE 1800-2023 16.7)");
        প্রত্যাগম nullptr;
    }
    যদি (!V3AssertCommon::check_Sva_মান_Limit(errp, constp, "SVA cycle delay")) প্রত্যাগম nullptr;
    প্রত্যাগম constp;
}

// Fold the bounds of a cycle delay in a sequence, and return if they are valid, else report it
// on errp
স্থির তার্কিক svaCycleDelayValid(ধ্রুব AstDelay* nodep, Ast_গ্রন্থি* errp) {
    V3Const::constifyParamsNoWarnEdit(nodep->lhsp());
    ধ্রুব AstConst* ধ্রুব minConstp = checkSvaDelayBound(
        errp, nodep->lhsp(), nodep->isRangeDelay() ? "Range delay minimum" : "Delay value");
    যদি (!nodep->rhsp() || nodep->isUnbounded()) প্রত্যাগম minConstp != nullptr;
    V3Const::constifyParamsNoWarnEdit(nodep->rhsp());
    ধ্রুব AstConst* ধ্রুব maxConstp
        = checkSvaDelayBound(errp, nodep->rhsp(), "Range delay maximum");
    যদি (!minConstp || !maxConstp) প্রত্যাগম অসত্য;
    যদি (maxConstp->toUInt() < minConstp->toUInt()) {
        errp->v3error("Range delay maximum must be >= minimum (IEEE 1800-2023 16.7)");
        প্রত্যাগম অসত্য;
    }
    প্রত্যাগম সত্য;
}

শূন্য V3AssertCommon::checkSvaCycleDelay(AstDelay* nodep, Ast_গ্রন্থি* errp) {
    যদি (svaCycleDelayValid(nodep, errp)) প্রত্যাগম;
    // Make the delay valid, so later steps do not fail after the error
    File_সারি* ধ্রুব flp = nodep->fileline();
    Ast_গ্রন্থি_রাশিমালা* ধ্রুব minp = nodep->lhsp();
    minp->replaceWith(নূতন AstConst{flp, AstConst::BitFalseErroring{}});
    VL_DO_DANGLING(minp->delete_বৃক্ষ(), minp);
    যদি (nodep->rhsp() && !nodep->isUnbounded()) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব maxp = nodep->rhsp();
        maxp->replaceWith(নূতন AstConst{flp, AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(maxp->delete_বৃক্ষ(), maxp);
    }
}

তার্কিক V3AssertCommon::refsSvaFormal(ধ্রুব Ast_গ্রন্থি* nodep) {
    প্রত্যাগম nodep->exists([](ধ্রুব Ast_গ্রন্থি_Var_Ref* refp) {
        ধ্রুব AstVar* ধ্রুব varp = refp->varp();
        যদি (!varp || !varp->isIO()) প্রত্যাগম অসত্য;
        ধ্রুব Ast_গ্রন্থি* ধ্রুব declp = varp->aboveLoopp();
        প্রত্যাগম VN_IS(declp, Sequence) || VN_IS(declp, Property);
    });
}

শূন্য V3AssertCommon::checkSvaInstanceDelays(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি* instp) {
    // A delay still referencing a formal argument of an enclosing sequence or property is
    // checked when that is instanced. Collect first, as checking edits the delays.
    std::vector<AstDelay*> delayps;
    nodep->foreach([&](AstDelay* delayp) {
        যদি (!delayp->isCycleDelay() || refsSvaFormal(delayp->lhsp())) প্রত্যাগম;
        যদি (delayp->rhsp() && refsSvaFormal(delayp->rhsp())) প্রত্যাগম;
        delayps.ঠেলো_back(delayp);
    });
    প্রতি (AstDelay* ধ্রুব delayp : delayps) checkSvaCycleDelay(delayp, instp);
}

//######################################################################
// AssertDeFutureVisitor
// If any AstFuture, then move all non-future varrefs to be one cycle behind,
// see IEEE 1800-2023 16.9.4.

বর্গ AssertDeFutureVisitor final : সার্বজনিক VNVisitor {
    // STATE - across all visitors
    Ast_গ্রন্থি_Module* ধ্রুব m_modp;  // Module future is underneath
    ধ্রুব AstFuture* m_futurep;  // First AstFuture found
    ধ্রুব unsigned m_past_অঙ্ক;  // Prefix unique number for this module
    std::map<AstVar*, AstVar*> m_delayedVars;  // Old to delayed variable mapping
    // STATE - for current visit position (use VL_RESTORER)
    তার্কিক m_inFuture = অসত্য;  // Inside a future
    তার্কিক m_unsupported = অসত্য;  // Printed unsupported

    // METHODS
    শূন্য unsupported(Ast_গ্রন্থি* nodep) {
        যদি (m_unsupported) প্রত্যাগম;
        m_unsupported = সত্য;
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported/illegal: Future value function used with expression with "
                          << nodep->prettyOperatorName());
    }
    // VISITORS
    শূন্য visit(AstFuture* nodep) override {
        VL_RESTORER(m_inFuture);
        m_inFuture = সত্য;
        iterateChildren(nodep);
        // Done with the future, this subexpression is current-time
        nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;
        যদি (m_inFuture || m_unsupported)
            প্রত্যাগম;  // Need user1 set above, don't process when Future is removed
        যদি (nodep->access().isWriteOrRW()) {
            unsupported(nodep);
            প্রত্যাগম;
        }
        auto it = m_delayedVars.find(nodep->varp());
        AstVar* outvarp;
        যদি (it == m_delayedVars.end()) {
            Ast_Sen_বৃক্ষ* ধ্রুব sentreep = m_futurep->sentreep();
            AstAlways* ধ্রুব alwaysp = নূতন AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS,
                                                     sentreep->clone_বৃক্ষ(অসত্য), nullptr};
            m_modp->addStmtsp(alwaysp);
            outvarp = নূতন AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                 "__Vnotfuture" + cvtToStr(m_past_অঙ্ক) + "_" + nodep->name(),
                                 nodep->dtypep()};
            m_modp->addStmtsp(outvarp);
            AstVarRef* varRefAWritep = নূতন AstVarRef{nodep->fileline(), outvarp, VAccess::WRITE};
            varRefAWritep->user1(সত্য);
            Ast_গ্রন্থি_Var_Ref* varRefAReadp = nodep->clone_বৃক্ষ(অসত্য);
            varRefAReadp->user1(সত্য);
            Ast_গ্রন্থি* ধ্রুব assp = নূতন AstAssignDly{nodep->fileline(), varRefAWritep, varRefAReadp};
            alwaysp->addStmtsp(assp);
            m_delayedVars.emplace(nodep->varp(), outvarp);
        } অন্যথা {
            outvarp = it->second;
        }
        AstVarRef* newp = নূতন AstVarRef{nodep->fileline(), outvarp, VAccess::READ};
        newp->user1(সত্য);
        UINFO(9, "DeFuture " << nodep << "  becomes " << newp);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_FTask_Ref* nodep) override { unsupported(nodep); }
    শূন্য visit(AstMethodCall* nodep) override { unsupported(nodep); }
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যদি (!nodep->isPure()) unsupported(nodep);
        iterateChildren(nodep);
    }

সার্বজনিক:
    // CONSTRUCTORS
    explicit AssertDeFutureVisitor(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি_Module* modp, unsigned past_অঙ্ক)
        : m_modp{modp}
        , m_past_অঙ্ক{past_অঙ্ক} {
        // See if any Future before we process
        যদি (nodep->forall([&](ধ্রুব AstFuture* futurep) -> তার্কিক {
                m_futurep = futurep;
                প্রত্যাগম অসত্য;
            }))
            প্রত্যাগম;
        // UINFOTREE(9, nodep, "", "defuture-in");
        visit(nodep);  // Nodep may get deleted
        // UINFOTREE(9, nodep, "", "defuture-ou");
    }
    ~AssertDeFutureVisitor() = স্বভাব;
};

// Prepare one default sampled expression.
বর্গ Default_Sampled_মান_Visitor final : সার্বজনিক VNVisitor {
    // METHODS
    স্থির AstConst* newTypeDefault(Ast_গ্রন্থি_রাশিমালা* nodep) {
        AstConst* ধ্রুব constp
            = নূতন AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()};
        যদি (nodep->dtypep()->isFourstate()) constp->অঙ্ক().setAllBitsX();
        constp->dtypeFrom(nodep);
        প্রত্যাগম constp;
    }

    // VISITORS
    শূন্য visit(AstCMethodHard* nodep) override {
        যদি (nodep->method() == VCMethod::EVENT_IS_TRIGGERED) {
            nodep->replaceWith(নূতন AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা {
            iterateChildren(nodep);
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        AstVar* ধ্রুব varp = nodep->varp();
        যদি (varp->isParam()
            || (!varp->isNet() && varp->lifetime().isStatic() && varp->hasUserInit())) {
            প্রত্যাগম;
        }
        nodep->replaceWith(newTypeDefault(nodep));
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    explicit Default_Sampled_মান_Visitor(Ast_গ্রন্থি* nodep) { iterate(nodep); }
};

//######################################################################
// AssertVisitor

বর্গ AssertVisitor final : সার্বজনিক VNVisitor {
    // CONSTANTS
    স্থির constexpr uint8_t ALL_ASSERT_TYPES
        = std::numeric_limits<std::underlying_type<VAssertType::en>::type>::max();

    // NODE STATE/TYPES
    // Cleared on netlist
    //  AstNode::user1()         -> bool.  True if processed
    //  AstAlways::user2p()      -> std::vector<AstVar*>. Delayed variables via 'm_delayed'
    //  AstNodeVarRef::user2()   -> bool.  True if shouldn't be sampled
    //  AstIf::user2()           -> bool.  True for assertOn() checks inserted in this pass
    ধ্রুব VNUser1InUse m_user1InUse;
    ধ্রুব VNUser2InUse m_user2InUse;
    AstUser2Allocator<AstAlways, std::vector<AstVar*>> m_delayed;

    // STATE
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Last module
    ধ্রুব Ast_গ্রন্থি* m_beginp = nullptr;  // Last AstBegin/AstGenBlock
    unsigned m_monitor_অঙ্ক = 0;  // Global $monitor numbering (not per module)
    AstVar* m_monitor_অঙ্ক_Varp = nullptr;  // $monitor number variable
    AstVar* m_monitorOffVarp = nullptr;  // $monitoroff variable
    unsigned m_mod_Past_অঙ্ক = 0;  // Module past numbering
    unsigned m_mod_Strobe_অঙ্ক = 0;  // Module $strobe numbering
    Ast_গ্রন্থি_Procedure* m_procedurep = nullptr;  // Current procedure
    VDouble0 m_statCover;  // Statistic tracking
    VDouble0 m_statAsNotImm;  // Statistic tracking
    VDouble0 m_statAsImm;  // Statistic tracking
    VDouble0 m_statAsFull;  // Statistic tracking
    VDouble0 m_statPastVars;  // Statistic tracking
    VDouble0 m_statAssertOnCombined;  // Statistic tracking
    VDouble0 m_statAssertOnHoisted;  // Statistic tracking
    তার্কিক m_inSampled = অসত্য;  // True inside a sampled expression
    তার্কিক m_inRestrict = অসত্য;  // True inside restrict assertion
    Ast_গ্রন্থি* m_passsp = nullptr;  // Current pass statement
    Ast_গ্রন্থি* m_failsp = nullptr;  // Current fail statement
    Ast_গ্রন্থি_Cover_Or_Assert* m_assertp = nullptr;  // Current assertion
    AstFinal* m_finalp = nullptr;  // Current final block
    VDouble0 m_statLiftedCaseExprs;  // Count of purified case expressions
    Ast_গ্রন্থি_FTask* m_ftaskp = nullptr;  // Current function/task
    V3UniqueNames m_caseTempNames{"__VCase"};
    V3UniqueNames m_match_গুনতি_Names{"__VnfaRemainingMatchCount"};  // Match replay counter names
    // Maps from (expression, senTree) to the AstAlways that computes its delayed values.
    std::unordered_map<VNRef<Ast_গ্রন্থি_রাশিমালা>, std::unordered_map<VNRef<Ast_Sen_বৃক্ষ>, AstAlways*>>
        m_modExpr2Sen2DelayedAlwaysp;
    // Source sampled-value history is initialized, unlike internal property-lowering history.
    std::unordered_map<VNRef<Ast_গ্রন্থি_রাশিমালা>, std::unordered_map<VNRef<Ast_Sen_বৃক্ষ>, AstAlways*>>
        m_modExpr2Sen2DefaultDelayedAlwaysp;
    // Map from delayed-value AstAlways to its last-sampling-tick time variable
    std::unordered_map<ধ্রুব AstAlways*, AstVar*> m_delayedAlways2TickTimep;

    // METHODS
    স্থির ধ্রুব বর্ণ* assertPassOnQuery(তার্কিক vacuous) {
        স্থির constexpr ধ্রুব বর্ণ* queries[2]
            = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
        প্রত্যাগম queries[vacuous];
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* assertOnCond(File_সারি* fl, VAssertType type,
                                     VAssertDirectiveType directiveType) {
        // cppcheck-suppress missingReturn
        বিকল্প (directiveType) {
        পক্ষ VAssertDirectiveType::INTRINSIC: প্রত্যাগম নূতন AstConst{fl, AstConst::BitTrue{}};
        পক্ষ VAssertDirectiveType::VIOLATION_CASE: {
            যদি (v3Global.opt.assertCase()) {
                প্রত্যাগম নূতন AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            // If assertions are off, have constant propagation rip them out later
            // This allows syntax errors and such to be detected normally.
            প্রত্যাগম নূতন AstConst{fl, AstConst::BitFalse{}};
        }
        পক্ষ VAssertDirectiveType::ASSERT:
        পক্ষ VAssertDirectiveType::COVER:
        পক্ষ VAssertDirectiveType::ASSUME: {
            যদি (v3Global.opt.assertOn()) {
                প্রত্যাগম নূতন AstCExpr{
                    fl, AstCExpr::Pure{},
                    V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
            }
            প্রত্যাগম নূতন AstConst{fl, AstConst::BitFalse{}};
        }
        পক্ষ VAssertDirectiveType::INTERNAL:
        পক্ষ VAssertDirectiveType::VIOLATION_IF:
        পক্ষ VAssertDirectiveType::RESTRICT: {
            যদি (v3Global.opt.assertOn()) {
                প্রত্যাগম নূতন AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            প্রত্যাগম নূতন AstConst{fl, AstConst::BitFalse{}};
        }
        }
        VL_UNREACHABLE;
    }

    স্থির তার্কিক isControlled(VAssertDirectiveType directiveType) {
        প্রত্যাগম (static_cast<পূর্ণাঙ্ক>(directiveType)
                & (static_cast<পূর্ণাঙ্ক>(VAssertDirectiveType::ASSERT)
                   | static_cast<পূর্ণাঙ্ক>(VAssertDirectiveType::COVER)
                   | static_cast<পূর্ণাঙ্ক>(VAssertDirectiveType::ASSUME)));
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* assertPassOnCond(File_সারি* fl, VAssertType type,
                                         VAssertDirectiveType directiveType, তার্কিক vacuous) {
        যদি (!isControlled(directiveType)) প্রত্যাগম নূতন AstConst{fl, AstConst::BitTrue{}};
        যদি (!v3Global.opt.assertOn()) প্রত্যাগম নূতন AstConst{fl, AstConst::BitFalse{}};
        প্রত্যাগম নূতন AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType), 1};
    }
    স্থির Ast_গ্রন্থি_রাশিমালা* assertFailOnCond(File_সারি* fl, VAssertType type,
                                         VAssertDirectiveType directiveType) {
        যদি (!isControlled(directiveType)) প্রত্যাগম নূতন AstConst{fl, AstConst::BitTrue{}};
        যদি (!v3Global.opt.assertOn()) প্রত্যাগম নূতন AstConst{fl, AstConst::BitFalse{}};
        প্রত্যাগম নূতন AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType), 1};
    }
    string assertDisplayMessage(ধ্রুব Ast_গ্রন্থি* nodep, ধ্রুব string& prefix, ধ্রুব string& message,
                                VDisplayType severity) {
        যদি (severity == VDisplayType::DT_ERROR || severity == VDisplayType::DT_FATAL) {
            প্রত্যাগম ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                    + cvtToStr(nodep->fileline()->lineno()) + ": Assertion failed in %m"
                    + ((message != "") ? ": " : "") + message + "\n");
        }
        প্রত্যাগম ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                + cvtToStr(nodep->fileline()->lineno()) + ": %m" + ((message != "") ? ": " : "")
                + message + "\n");
    }
    শূন্য replaceDisplay(AstDisplay* nodep, ধ্রুব string& prefix) {
        nodep->fmtp()->text(
            assertDisplayMessage(nodep, prefix, nodep->fmtp()->text(), nodep->displayType()));
        nodep->displayType(VDisplayType::DT_WRITE);
        // cppcheck-suppress nullPointer
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব timenewp = নূতন AstTime{nodep->fileline(), m_modp->timeunit()};
        nodep->fmtp()->prependExprsp(timenewp);
        যদি (!nodep->fmtp()->scopeNamep() && nodep->fmtp()->formatScopeTracking()) {
            nodep->fmtp()->scopeNamep(নূতন AstScopeName{nodep->fileline(), সত্য});
        }
    }
    AstSampled* new_Sampled_রাশিমালা(Ast_গ্রন্থি_রাশিমালা* nodep) {
        প্রত্যাগম নূতন AstSampled{nodep->fileline(), nodep, nodep->dtypep(), সত্য};
    }
    AstVarRef* new_Monitor_অঙ্ক_Var_Refp(ধ্রুব Ast_গ্রন্থি* nodep, VAccess access) {
        যদি (!m_monitor_অঙ্ক_Varp) {
            m_monitor_অঙ্ক_Varp = নূতন AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorNum",
                                          nodep->findUInt64DType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitor_অঙ্ক_Varp);
        }
        AstVarRef* ধ্রুব varrefp = নূতন AstVarRef{nodep->fileline(), m_monitor_অঙ্ক_Varp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        প্রত্যাগম varrefp;
    }
    AstVarRef* newMonitorOffVarRefp(ধ্রুব Ast_গ্রন্থি* nodep, VAccess access) {
        যদি (!m_monitorOffVarp) {
            m_monitorOffVarp = নূতন AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorOff",
                                          nodep->findBitDType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitorOffVarp);
        }
        AstVarRef* ধ্রুব varrefp = নূতন AstVarRef{nodep->fileline(), m_monitorOffVarp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        প্রত্যাগম varrefp;
    }
    স্থির AstIf* newIfAssertOn(Ast_গ্রন্থি* bodyp, VAssertDirectiveType directiveType,
                                VAssertType type = VAssertType::INTERNAL) {
        // Add an internal if to check assertions are on.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_সারি* ধ্রুব fl = bodyp->fileline();

        Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp = assertOnCond(fl, type, directiveType);
        AstIf* ধ্রুব newp = নূতন AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(সত্য);  // To avoid LATCH warning
        newp->user2(সত্য);  // Mark as an assertOn() check
        প্রত্যাগম newp;
    }
    স্থির Ast_গ্রন্থি_Stmt* newIfAssertPassOn(Ast_গ্রন্থি* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type, তার্কিক vacuous) {
        // Add an internal if to check assertion passOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_সারি* ধ্রুব fl = bodyp->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp = assertPassOnCond(fl, type, directiveType, vacuous);
        Ast_গ্রন্থি_If* ধ্রুব newp = নূতন AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(সত্য);  // To avoid LATCH warning
        newp->user1(সত্য);  // Don't assert/cover this if
        newp->user2(সত্য);  // Mark as an assertOn() check
        প্রত্যাগম newp;
    }
    স্থির Ast_গ্রন্থি_Stmt* newIfAssertFailOn(Ast_গ্রন্থি* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type) {
        // Add an internal if to check assertion failOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_সারি* ধ্রুব fl = bodyp->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp = assertFailOnCond(fl, type, directiveType);
        Ast_গ্রন্থি_If* ধ্রুব newp = নূতন AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(সত্য);  // To avoid LATCH warning
        newp->user1(সত্য);  // Don't assert/cover this if
        newp->user2(সত্য);  // Mark as an assertOn() check
        প্রত্যাগম newp;
    }

    স্থির AstIf* assertCond(ধ্রুব Ast_গ্রন্থি_Cover_Or_Assert* nodep, Ast_গ্রন্থি_রাশিমালা* propp,
                             Ast_গ্রন্থি* passsp, Ast_গ্রন্থি* failsp) {
        AstIf* ধ্রুব ifp = নূতন AstIf{nodep->fileline(), propp, passsp, failsp};
        // It's more LIKELY that we'll take the nullptr if clause
        // than the sim-killing else clause:
        ifp->branchPred(VBranchPred::BP_LIKELY);
        ifp->isBoundsCheck(সত্য);  // To avoid LATCH warning
        প্রত্যাগম ifp;
    }

    Ast_গ্রন্থি_Stmt* assertBody(ধ্রুব Ast_গ্রন্থি_Cover_Or_Assert* nodep, Ast_গ্রন্থি* propp, Ast_গ্রন্থি* passsp,
                            Ast_গ্রন্থি* failsp) {
        যদি (AstPExpr* ধ্রুব pexprp = VN_CAST(propp, PExpr)) {
            যদি (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NOTIMING, "This property expression requires --timing");
                VL_DO_DANGLING(ঠেলো_Deletep(pexprp), pexprp);
                প্রত্যাগম নূতন AstBegin{nodep->fileline(), "", nullptr, অসত্য};
            }
            AstFork* ধ্রুব forkp = নূতন AstFork{nodep->fileline(), VJoinType::JOIN_NONE};
            forkp->addForksp(pexprp->bodyp()->unlinkFrBack());
            যদি (Ast_গ্রন্থি_Stmt* ধ্রুব finalp = pexprp->finalp()) {
                যদি (!m_finalp) {
                    m_finalp = নূতন AstFinal{m_modp->fileline(), finalp->unlinkFrBack()};
                    m_modp->addStmtsp(m_finalp);
                } অন্যথা {
                    m_finalp->addStmtsp(finalp->unlinkFrBack());
                }
            }
            VL_DO_DANGLING2(ঠেলো_Deletep(pexprp), pexprp, propp);
            প্রত্যাগম forkp;
        }

        প্রত্যাগম assertCond(nodep, VN_AS(propp, গ্রন্থি_রাশিমালা), passsp, failsp);
    }

    Ast_গ্রন্থি_Stmt* newFireAssertUnchecked(ধ্রুব Ast_গ্রন্থি_Stmt* nodep, ধ্রুব string& message,
                                        Ast_গ্রন্থি_রাশিমালা* exprsp = nullptr) {
        // Like newFireAssert() but omits the asserts-on check
        AstDisplay* ধ্রুব dispp
            = নূতন AstDisplay{nodep->fileline(), VDisplayType::DT_ERROR, message, nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        Ast_গ্রন্থি_Stmt* ধ্রুব bodysp = dispp;
        replaceDisplay(dispp, "%%Error");  // Convert to standard DISPLAY format
        যদি (exprsp) dispp->fmtp()->exprsp()->addNext(exprsp);
        যদি (v3Global.opt.stopFail()) bodysp->addNext(নূতন AstStop{nodep->fileline(), অসত্য});
        প্রত্যাগম bodysp;
    }

    Ast_গ্রন্থি_Stmt* newFireAssert(ধ্রুব Ast_গ্রন্থি_Stmt* nodep, VAssertDirectiveType directiveType,
                               VAssertType assertType, ধ্রুব string& message,
                               Ast_গ্রন্থি_রাশিমালা* exprsp = nullptr) {
        Ast_গ্রন্থি_Stmt* bodysp = newFireAssertUnchecked(nodep, message, exprsp);
        bodysp = newIfAssertOn(bodysp, directiveType, assertType);
        প্রত্যাগম bodysp;
    }

    AstVar* createDelayedVar(ধ্রুব std::string& name, AstAlways* alwaysp, Ast_গ্রন্থি_রাশিমালা* exprp,
                             Ast_গ্রন্থি_রাশিমালা* initp) {
        File_সারি* ধ্রুব flp = exprp->fileline();
        AstVar* ধ্রুব varp = নূতন AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        // TODO: this lifetime seems nonsene (can't have NBAs to automatics), but is as before
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->addStmtsp(varp);
        // Before enough clock ticks, $past returns the expression's default sampled value.
        যদি (initp) {
            m_modp->addStmtsp(নূতন AstInitialStatic{
                flp, নূতন AstAssign{flp, নূতন AstVarRef{flp, varp, VAccess::WRITE}, initp}});
        }
        ++m_statPastVars;
        // Actually set the delayed value
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব lhsp = নূতন AstVarRef{flp, varp, VAccess::WRITE};
        AstAssignDly* ধ্রুব assignp = নূতন AstAssignDly{flp, lhsp, exprp};
        alwaysp->prependStmtsp(assignp);
        প্রত্যাগম varp;
    }

    AstAlways* getDelayedAlways(Ast_গ্রন্থি_রাশিমালা* exprp, Ast_গ্রন্থি_রাশিমালা* initp, Ast_Sen_বৃক্ষ* senTreep) {
        auto& expr2Sen2Alwaysr
            = initp ? m_modExpr2Sen2DefaultDelayedAlwaysp : m_modExpr2Sen2DelayedAlwaysp;
        AstAlways*& alwayspr = expr2Sen2Alwaysr[*exprp][*senTreep];
        যদি (!alwayspr) {
            File_সারি* ধ্রুব flp = exprp->fileline();
            // Create the always block that computes the delayed values
            alwayspr = নূতন AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, nullptr};
            m_modp->addStmtsp(alwayspr);
            // Create the once-delayed variable
            ধ্রুব std::string name = "_Vpast_" + cvtToStr(m_mod_Past_অঙ্ক++) + "_1";
            AstVar* ধ্রুব varp = createDelayedVar(name, alwayspr, exprp, initp);
            // Add it to delayed variable vector
            m_delayed(alwayspr).emplace_back(varp);
        } অন্যথা {
            // Reusing exiting, not needed
            VL_DO_DANGLING(ঠেলো_Deletep(exprp), exprp);
            যদি (initp) VL_DO_DANGLING(ঠেলো_Deletep(initp), initp);
            VL_DO_DANGLING(ঠেলো_Deletep(senTreep), senTreep);
        }
        প্রত্যাগম alwayspr;
    }

    Ast_গ্রন্থি_রাশিমালা* get_Past_মান(Ast_গ্রন্থি_রাশিমালা* exprp, Ast_গ্রন্থি_রাশিমালা* initp, Ast_Sen_বৃক্ষ* senTreep,
                              uint32_t ticks) {
        UASSERT_OBJ(ticks > 0, exprp, "Delay must be > 0");
        File_সারি* ধ্রুব flp = exprp->fileline();
        AstAlways* ধ্রুব alwaysp
            = getDelayedAlways(exprp, initp ? initp->clone_বৃক্ষ_Pure(অসত্য) : nullptr, senTreep);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব resultp = past_মান_Ref(alwaysp, flp, ticks, initp);
        যদি (initp) VL_DO_DANGLING(ঠেলো_Deletep(initp), initp);
        প্রত্যাগম resultp;
    }

    Ast_গ্রন্থি_রাশিমালা* past_মান_Ref(AstAlways* alwaysp, File_সারি* flp, uint32_t ticks,
                              Ast_গ্রন্থি_রাশিমালা* initp) {
        std::vector<AstVar*>& delayedr = m_delayed(alwaysp);
        // Ensure the required delay exists
        যাবত্ (delayedr.size() < ticks) {
            AstVar* ধ্রুব firstp = delayedr.front();
            File_সারি* ধ্রুব varFlp = firstp->fileline();
            // Create once more delayed value
            std::string name = firstp->name();
            name.resize(name.size() - 1);
            name += std::to_string(delayedr.size() + 1);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব prevp = নূতন AstVarRef{varFlp, delayedr.back(), VAccess::READ};
            AstVar* ধ্রুব varp = createDelayedVar(name, alwaysp, prevp,
                                                  initp ? initp->clone_বৃক্ষ_Pure(অসত্য) : nullptr);
            // Add it to delayed variable vector
            delayedr.emplace_back(varp);
        }
        // Return a reference to the appropriately delayed variable
        প্রত্যাগম নূতন AstVarRef{flp, delayedr.at(ticks - 1), VAccess::READ};
    }

    // Time of the pipeline's last sampling tick, for end-of-simulation readers
    AstVar* getPastTickTimeVar(AstAlways* alwaysp) {
        AstVar*& varpr = m_delayedAlways2TickTimep[alwaysp];
        যদি (!varpr) {
            File_সারি* ধ্রুব flp = alwaysp->fileline();
            varpr = নূতন AstVar{flp, VVarType::MODULETEMP,
                               "_Vpast_" + cvtToStr(m_mod_Past_অঙ্ক++) + "_t",
                               m_modp->findUInt64DType()};
            m_modp->addStmtsp(varpr);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব timep = নূতন AstCExpr{flp, "vlSymsp->_vm_contextp__->time()", 64};
            alwaysp->addStmtsp(
                নূতন AstAssign{flp, নূতন AstVarRef{flp, varpr, VAccess::WRITE}, timep});
        }
        প্রত্যাগম varpr;
    }

    শূন্য visitAssertionIterate(Ast_গ্রন্থি_Cover_Or_Assert* nodep, Ast_গ্রন্থি* failsp) {
        যদি (m_beginp && nodep->name() == "") nodep->name(m_beginp->name());

        { AssertDeFutureVisitor{nodep->propp(), m_modp, m_mod_Past_অঙ্ক++}; }

        iterateAndNextNull(nodep->sentreep());
        যদি (AstAssert* ধ্রুব assertp = VN_CAST(nodep, Assert)) {
            iterateAndNextNull(assertp->failsp());
        } অন্যথা যদি (AstAssertIntrinsic* ধ্রুব assertp = VN_CAST(nodep, AssertIntrinsic)) {
            iterateAndNextNull(assertp->failsp());
        } অন্যথা যদি (AstCover* ধ্রুব coverp = VN_CAST(nodep, Cover)) {
            iterateAndNextNull(coverp->coverincsp());
        } অন্যথা যদি (!VN_IS(nodep, Restrict)) {
            nodep->v3fatalSrc("Unhandled assert type");
        }
        iterateAndNextNull(nodep->passsp());
        Ast_Sen_বৃক্ষ* sentreep = nodep->sentreep();
        যদি (nodep->immediate()) {
            UASSERT_OBJ(!sentreep, nodep, "Immediate assertions don't have sensitivity");
        } অন্যথা {
            UASSERT_OBJ(sentreep, nodep, "Concurrent assertions must have sensitivity");
            // Explicit inline clock differs from the enclosing always: hoist
            // and warn. To support this need queue of asserts to activate.
            যদি (m_procedurep && !nodep->senFromAlways()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Procedural concurrent assertion with"
                              " clocking event inside always (IEEE 1800-2023 16.14.6)");
                static_cast<Ast_গ্রন্থি*>(m_procedurep)->addNext(nodep->unlinkFrBack());
                প্রত্যাগম;  // Later iterate will pick up
            }
            sentreep->unlinkFrBack();
            যদি (m_procedurep) { VL_DO_DANGLING(ঠেলো_Deletep(sentreep), sentreep); }
        }
        //
        ধ্রুব string& message = nodep->name();
        Ast_গ্রন্থি* passsp = nodep->passsp();
        যদি (passsp) passsp->unlinkFrBackWithNext();
        যদি (AstAssert* ধ্রুব assertp = VN_CAST(nodep, Assert)) {
            failsp = assertp->failsp();
        } অন্যথা যদি (AstAssertIntrinsic* ধ্রুব assertp = VN_CAST(nodep, AssertIntrinsic)) {
            failsp = assertp->failsp();
        }
        যদি (failsp) failsp->unlinkFrBackWithNext();

        তার্কিক selfDestruct = অসত্য;
        তার্কিক passspGated = অসত্য;
        ধ্রুব AstCover* ধ্রুব coverp = VN_CAST(nodep, Cover);
        // A sequence event control is not an assertion directive; no assertion control
        ধ্রুব তার্কিক seqEvent = coverp && coverp->isSeqEvent();
        যদি (coverp) {
            ++m_statCover;
            যদি (seqEvent) {
                // Keep the event-fire action, with no coverage bucket
            } অন্যথা যদি (!v3Global.opt.coverageUser()) {
                selfDestruct = সত্য;
            } অন্যথা {
                // V3Coverage assigned us a bucket to increment.
                AstCoverInc* ধ্রুব covincp = VN_AS(coverp->coverincsp(), CoverInc);
                UASSERT_OBJ(covincp, coverp, "Missing AstCoverInc under assertion");
                covincp->unlinkFrBackWithNext();  // next() might have  AstAssign for trace
                যদি (message != "") covincp->declp()->comment(message);
                যদি (passsp) {
                    passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                               /*vacuous=*/অসত্য);
                    passspGated = সত্য;
                    passsp = Ast_গ্রন্থি::addNext<Ast_গ্রন্থি, Ast_গ্রন্থি>(covincp, passsp);
                } অন্যথা {
                    passsp = covincp;
                }
            }
        } অন্যথা যদি (VN_IS(nodep, Assert) || VN_IS(nodep, AssertIntrinsic)) {
            যদি (nodep->immediate()) {
                ++m_statAsImm;
            } অন্যথা {
                ++m_statAsNotImm;
            }
            যদি (!passsp && !failsp)
                failsp = newFireAssertUnchecked(
                    nodep, VN_IS(nodep, AssertIntrinsic) ? "'$cast' failed." : "'assert' failed.");
        } অন্যথা {
            nodep->v3fatalSrc("Unknown node type");
        }

        VL_RESTORER(m_passsp);
        VL_RESTORER(m_failsp);
        VL_RESTORER(m_assertp);
        m_passsp = passsp;
        m_failsp = failsp;
        m_assertp = nodep;
        iterate(nodep->propp());

        Ast_গ্রন্থি* propExprp;
        Ast_গ্রন্থি_রাশিমালা* disablep = nullptr;
        Ast_গ্রন্থি_রাশিমালা* matchCountp = nullptr;
        যদি (AstPropSpec* ধ্রুব specp = VN_CAST(nodep->propp(), PropSpec)) {
            propExprp = specp->propp()->unlinkFrBack();
            যদি (specp->disablep()) disablep = specp->disablep()->unlinkFrBack();
            matchCountp = specp->matchCountp();
        } অন্যথা {
            propExprp = nodep->propp()->unlinkFrBack();
        }
        File_সারি* ধ্রুব flp = nodep->fileline();
        তার্কিক passspAlreadyGated = অসত্য;
        যদি (passsp && VN_IS(passsp, If)) passspAlreadyGated = VN_AS(passsp, If)->user1();
        যদি (passsp && !passspGated && !passspAlreadyGated && !VN_IS(propExprp, PExpr)
            && !seqEvent) {
            passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                       /*vacuous=*/অসত্য);
        }
        যদি (failsp && !VN_IS(propExprp, PExpr)) {
            failsp = newIfAssertFailOn(failsp, nodep->directive(), nodep->userType());
        }
        যদি (coverp && matchCountp && passsp) {
            // Convert the match count into a loop that decrements a temporary variable until it
            // reaches zero.
            matchCountp->unlinkFrBack();
            AstVar* ধ্রুব remainingp = নূতন AstVar{
                flp, VVarType::MODULETEMP, m_match_গুনতি_Names.get(""), matchCountp->dtypep()};
            remainingp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remainingp);
            Ast_গ্রন্থি* ধ্রুব replaysp
                = নূতন AstAssign{flp, নূতন AstVarRef{flp, remainingp, VAccess::WRITE}, matchCountp};
            AstLoop* ধ্রুব loopp = নূতন AstLoop{flp};
            loopp->addStmtsp(
                নূতন AstLoopTest{flp, loopp, নূতন AstVarRef{flp, remainingp, VAccess::READ}});
            loopp->addStmtsp(passsp);
            loopp->addStmtsp(
                নূতন AstAssign{flp, নূতন AstVarRef{flp, remainingp, VAccess::WRITE},
                              নূতন AstSub{flp, নূতন AstVarRef{flp, remainingp, VAccess::READ},
                                         নূতন AstConst{flp, AstConst::Widthed_মান{},
                                                      remainingp->dtypep()->width(), 1}}});
            replaysp->addNext(loopp);
            passsp = replaysp;
        }
        Ast_গ্রন্থি* bodysp = assertBody(nodep, propExprp, passsp, failsp);
        যদি (disablep) bodysp = নূতন AstIf{flp, নূতন AstLogNot{flp, disablep}, bodysp};
        // Add assertOn check last, for better combining
        যদি (!seqEvent) bodysp = newIfAssertOn(bodysp, nodep->directive(), nodep->userType());
        যদি (sentreep) bodysp = নূতন AstAlways{flp, VAlwaysKwd::ALWAYS, sentreep, bodysp};

        যদি (passsp && !passsp->backp()) VL_DO_DANGLING(ঠেলো_Deletep(passsp), passsp);
        যদি (failsp && !failsp->backp()) VL_DO_DANGLING(ঠেলো_Deletep(failsp), failsp);

        // Install it
        যদি (selfDestruct) {
            // Delete it after making the tree.  This way we can tell the user
            // if it wasn't constructed nicely or has other errors without needing --coverage.
            VL_DO_DANGLING(bodysp->delete_বৃক্ষ(), bodysp);
            nodep->unlinkFrBack();
        } অন্যথা {
            nodep->replaceWith(bodysp);
        }
        // Bye
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    তার্কিক isEmptyStmt(Ast_গ্রন্থি* nodep) {
        যদি (!nodep) প্রত্যাগম সত্য;
        যদি (AstBegin* ধ্রুব beginp = VN_CAST(nodep, Begin)) {
            যদি (beginp->declsp()) প্রত্যাগম অসত্য;
            প্রতি (Ast_গ্রন্থি* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                যদি (!isEmptyStmt(stmtp)) প্রত্যাগম অসত্য;
            }
            প্রত্যাগম সত্য;
        }
        প্রত্যাগম অসত্য;
    }

    // VISITORS
    শূন্য visit(AstIf* nodep) override {
        যদি (!nodep->user1SetOnce()) {
            যদি (nodep->uniquePragma() || nodep->unique0Pragma()) {
                ধ্রুব Ast_গ্রন্থি_If* ifp = nodep;
                Ast_গ্রন্থি_রাশিমালা* propp = nullptr;
                তার্কিক hasDefaultElse = অসত্য;
                কুরু {
                    // If this statement ends with 'else if', then nextIf will point to the
                    // nextIf statement.  Otherwise it will be null.
                    ধ্রুব Ast_গ্রন্থি_If* ধ্রুব nextifp = dynamic_cast<Ast_গ্রন্থি_If*>(ifp->elsesp());
                    iterateAndNextNull(ifp->condp());

                    // Recurse into the true case.
                    iterateAndNextNull(ifp->thensp());

                    // If the last else is not an else if, recurse into that too.
                    যদি (ifp->elsesp() && !nextifp) {  //
                        iterateAndNextNull(ifp->elsesp());
                    }

                    // Build a bitmask of the true predicates
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব predp = ifp->condp()->clone_বৃক্ষ_Pure(অসত্য);
                    যদি (propp) {
                        propp = নূতন AstConcat{nodep->fileline(), predp, propp};
                    } অন্যথা {
                        propp = predp;
                    }

                    // Record if this ends with an 'else' that does not have an if
                    যদি (ifp->elsesp() && !nextifp) hasDefaultElse = সত্য;

                    ifp = nextifp;
                } যাবত্ (ifp);

                ধ্রুব তার্কিক allow_none = nodep->unique0Pragma();

                // Empty case means no property
                যদি (!propp) propp = নূতন AstConst{nodep->fileline(), AstConst::BitFalse{}};

                // Note: if this ends with an 'else', then we don't need to validate that one of
                // the predicates evaluates to true.
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব ohot
                    = ((allow_none || hasDefaultElse)
                           ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নূতন AstOneHot0{nodep->fileline(), propp})
                           : static_cast<Ast_গ্রন্থি_রাশিমালা*>(নূতন AstOneHot{nodep->fileline(), propp}));
                ধ্রুব VAssertType assertType
                    = nodep->uniquePragma() ? VAssertType::UNIQUE : VAssertType::UNIQUE0;
                // Move, not clone, as the branches might declare referenced variables
                VNRelinker relinker;
                nodep->unlinkFrBack(&relinker);
                AstIf* ধ্রুব checkifp
                    = নূতন AstIf{nodep->fileline(), নূতন AstLogNot{nodep->fileline(), ohot},
                                newFireAssert(nodep, VAssertDirectiveType::VIOLATION_IF,
                                              assertType, "'unique if' statement violated"),
                                nodep};
                checkifp->isBoundsCheck(সত্য);  // To avoid LATCH warning
                checkifp->branchPred(VBranchPred::BP_UNLIKELY);
                relinker.relink(checkifp);
                প্রত্যাগম;
            }
        }

        iterateChildren(nodep);

        যদি (nodep->user2()) {
            // Combine consecutive assertOn checks if possible
            যদি (AstIf* ধ্রুব backp = VN_CAST(nodep->backp(), If)) {
                যদি (backp->nextp() == nodep  //
                    && backp->user2()  //
                    && backp->condp()->same_বৃক্ষ(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    backp->addThensp(nodep->thensp()->unlinkFrBackWithNext());
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    প্রত্যাগম;
                }
            }
            // Combine nested assertOn checks if possible
            যদি (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* ধ্রুব checkp = VN_CAST(nodep->thensp(), If);
                যদি (checkp  //
                    && checkp->user2()  //
                    && checkp->condp()->same_বৃক্ষ(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    VL_DO_DANGLING(ঠেলো_Deletep(checkp->unlinkFrBack()), checkp);
                    প্রত্যাগম;
                }
            }
            প্রত্যাগম;
        }

        // Swap assertOn check with single statement 'if' statement to bubble up for combining
        // Note we can't just swap the conditions as they two Ifs have different flags,
        // so swapping the Ifs themeselves then swapping back the bodies.
        যদি (nodep->condp()->isPure()) {
            যদি (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* ধ্রুব checkp = VN_CAST(nodep->thensp(), If);
                যদি (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    প্রত্যাগম;
                }
            }
            যদি (nodep->elsesp() && !nodep->elsesp()->nextp() && isEmptyStmt(nodep->thensp())) {
                AstIf* ধ্রুব checkp = VN_CAST(nodep->elsesp(), If);
                যদি (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addElsesp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    প্রত্যাগম;
                }
            }
        }
    }

    //========== Case assertions
    শূন্য visit(AstCase* nodep) override {
        // Introduce temporary variable for AstCase if needed - it is done here and not in V3Case
        // because this phase is before V3Scope and V3Case is not. Doing it before V3Scope ensures
        // that V3Scope will take care of a scope creation
        // We also need to do it before V3Begin, co that pragmas like `unique0` also work correctly
        যদি (!nodep->exprp()->isPure()) {
            ++m_statLiftedCaseExprs;
            File_সারি* ধ্রুব fl = nodep->exprp()->fileline();
            AstVar* ধ্রুব varp = নূতন AstVar{fl, VVarType::BLOCKTEMP, m_caseTempNames.get(nodep),
                                            nodep->exprp()->dtypep()};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব origp = nodep->exprp()->unlinkFrBack();
            nodep->addHereThisAsNext(
                নূতন AstAssign{fl, নূতন AstVarRef{fl, varp, VAccess::WRITE}, origp});
            nodep->exprp(নূতন AstVarRef{fl, varp, VAccess::READ});
            যদি (m_ftaskp) {
                varp->funcLocal(সত্য);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } অন্যথা {
                m_modp->prependStmtsp(varp);
            }
            VIsCached::clear_Cache_বৃক্ষ();
        }
        iterateChildren(nodep);
        যদি (!nodep->user1SetOnce()) {
            তার্কিক has_default = অসত্য;
            প্রতি (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                যদি (itemp->isDefault()) has_default = সত্য;
            }
            ধ্রুব Ast_গ্রন্থি_DType* রাশিমালা_Dtypep = nodep->exprp()->dtypep()->skipRefp();

            VAssertType assertType = VAssertType::INTERNAL;
            যদি (nodep->priorityPragma()) {
                assertType = VAssertType::PRIORITY;
            } অন্যথা যদি (nodep->uniquePragma()) {
                assertType = VAssertType::UNIQUE;
            } অন্যথা যদি (nodep->unique0Pragma()) {
                assertType = VAssertType::UNIQUE0;
            }

            string valFmt;
            যদি (রাশিমালা_Dtypep->isIntegralOrPacked())
                valFmt = " for '" + cvtToStr(রাশিমালা_Dtypep->widthMin()) + "'h%X'";
            যদি (nodep->fullPragma() || nodep->priorityPragma()) {
                // Need to add a default if there isn't one already
                ++m_statAsFull;
                যদি (!has_default) {
                    nodep->addItemsp(নূতন AstCaseItem{
                        nodep->fileline(), nullptr /*DEFAULT*/,
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      nodep->pragmaString() + ", but non-match found" + valFmt,
                                      valFmt.empty() ? nullptr
                                                     : nodep->exprp()->clone_বৃক্ষ_Pure(অসত্য))});
                }
            }
            যদি (nodep->parallelPragma() || nodep->uniquePragma() || nodep->unique0Pragma()) {
                // Need to check that one, and only one of the case items match at any moment
                // If there's a default, we allow none to match, else exactly one must match
                ++m_statAsFull;
                যদি (!has_default && !nodep->itemsp()) {
                    // Not parallel, but harmlessly so.
                } অন্যথা {
                    Ast_গ্রন্থি_রাশিমালা* propp = nullptr;
                    প্রতি (AstCaseItem* itemp = nodep->itemsp(); itemp;
                         itemp = VN_AS(itemp->nextp(), CaseItem)) {
                        Ast_গ্রন্থি_রাশিমালা* itembitp = nullptr;
                        প্রতি (Ast_গ্রন্থি_রাশিমালা* icondp = itemp->condsp(); icondp;
                             icondp = VN_AS(icondp->nextp(), গ্রন্থি_রাশিমালা)) {
                            Ast_গ্রন্থি_রাশিমালা* onep;
                            যদি (AstInsideRange* ধ্রুব rcondp = VN_CAST(icondp, InsideRange)) {
                                onep = rcondp->newAndFromInside(
                                    nodep->exprp()->clone_বৃক্ষ_Pure(সত্য),
                                    rcondp->lhsp()->clone_বৃক্ষ_Pure(সত্য),
                                    rcondp->rhsp()->clone_বৃক্ষ_Pure(সত্য));
                            } অন্যথা যদি (nodep->casex() || nodep->casez() || nodep->caseInside()) {
                                onep = AstEqWild::newTyped(itemp->fileline(),
                                                           nodep->exprp()->clone_বৃক্ষ_Pure(অসত্য),
                                                           icondp->clone_বৃক্ষ_Pure(অসত্য));
                            } অন্যথা {
                                onep = AstEq::newTyped(icondp->fileline(),
                                                       nodep->exprp()->clone_বৃক্ষ_Pure(অসত্য),
                                                       icondp->clone_বৃক্ষ_Pure(অসত্য));
                            }
                            // OR together all conditions within the same case item
                            যদি (onep) {
                                যদি (itembitp) {
                                    itembitp = নূতন AstOr{icondp->fileline(), onep, itembitp};
                                } অন্যথা {
                                    itembitp = onep;
                                }
                            }
                        }
                        যদি (itembitp) {
                            যদি (propp) {
                                propp = নূতন AstConcat{itemp->fileline(), itembitp, propp};
                            } অন্যথা {
                                propp = itembitp;
                            }
                        }
                    }
                    // Empty case means no property
                    যদি (!propp) propp = নূতন AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    ধ্রুব তার্কিক allow_none = has_default || nodep->unique0Pragma();
                    // The following assertion looks as below.
                    // if (!$onehot(propp)) begin
                    //     if (propp == '0) begin if (!allow_none) $error("none match"); end
                    //     else $error("multiple match");
                    // end
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব ohot = নূতন AstOneHot{nodep->fileline(), propp};
                    AstConst* ধ্রুব zero = নূতন AstConst{
                        nodep->fileline(), AstConst::Widthed_মান{}, propp->width(), 0};
                    AstIf* ধ্রুব ohotIfp
                        = নূতন AstIf{nodep->fileline(), নূতন AstLogNot{nodep->fileline(), ohot}};
                    AstIf* ধ্রুব zeroIfp = নূতন AstIf{
                        nodep->fileline(),
                        নূতন AstEq{nodep->fileline(), propp->clone_বৃক্ষ_Pure(অসত্য), zero}};
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = nodep->exprp();
                    ধ্রুব string pragmaStr = nodep->pragmaString();
                    যদি (!allow_none)
                        zeroIfp->addThensp(
                            newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                          pragmaStr + ", but none matched" + valFmt,
                                          valFmt.empty() ? nullptr : exprp->clone_বৃক্ষ_Pure(অসত্য)));
                    zeroIfp->addElsesp(
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      pragmaStr + ", but multiple matches found" + valFmt,
                                      valFmt.empty() ? nullptr : exprp->clone_বৃক্ষ_Pure(অসত্য)));
                    ohotIfp->addThensp(zeroIfp);
                    ohotIfp->isBoundsCheck(সত্য);  // To avoid LATCH warning
                    ohotIfp->branchPred(VBranchPred::BP_UNLIKELY);
                    nodep->addNotParallelp(ohotIfp);
                }
            }
        }
    }

    শূন্য visit(AstFuture* nodep) override {
        nodep->v3error("Future sampled value function called outside property or sequence "
                       "expression (IEEE 16.9.4)");
        nodep->replaceWith(নূতন AstConst{nodep->fileline(), 0});
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    //========== Past
    শূন্য visit(AstPast* nodep) override {
        যদি (!nodep->propertyTiming()) {
            nodep->initp(nodep->exprp()->clone_বৃক্ষ_Pure(অসত্য));
            { Default_Sampled_মান_Visitor{nodep->initp()}; }
        }
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব initp = nodep->initp() ? nodep->initp()->unlinkFrBack() : nullptr;
        iterateChildren(nodep);
        uint32_t ticks = 1;
        যদি (nodep->ticksp()) {
            UASSERT_OBJ(VN_IS(nodep->ticksp(), Const), nodep,
                        "Expected constant ticks, checked in V3Width");
            ticks = VN_AS(nodep->ticksp(), Const)->toUInt();
        }
        UASSERT_OBJ(ticks >= 1, nodep, "0 tick should have been checked in V3Width");
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = new_Sampled_রাশিমালা(nodep->exprp()->unlinkFrBack());
        Ast_Sen_বৃক্ষ* ধ্রুব senTreep = nodep->sentreep()->unlinkFrBack();
        Ast_গ্রন্থি_রাশিমালা* inp = nullptr;
        // Deliberately emitted for every ftask $past; calls outside final take the normal stage
        যদি (VN_IS(m_procedurep, Final) || m_ftaskp) {
            // A sampling-tick finish reads one stage deeper (IEEE 1800-2023 16.9.3).
            File_সারি* ধ্রুব flp = nodep->fileline();
            AstAlways* ধ্রুব alwaysp
                = getDelayedAlways(exprp, initp ? initp->clone_বৃক্ষ_Pure(অসত্য) : nullptr, senTreep);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব betweenTicksp = past_মান_Ref(alwaysp, flp, ticks, initp);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব onTickp = past_মান_Ref(alwaysp, flp, ticks + 1, initp);
            যদি (initp) VL_DO_DANGLING(ঠেলো_Deletep(initp), initp);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব onTickCondp = নূতন AstLogAnd{
                flp, নূতন AstCExpr{flp, "vlSymsp->_vm_contextp__->executingFinal()", 1},
                নূতন AstEq{flp, নূতন AstVarRef{flp, getPastTickTimeVar(alwaysp), VAccess::READ},
                          নূতন AstCExpr{flp, "vlSymsp->_vm_contextp__->finishPendingTime()", 64}}};
            AstCond* ধ্রুব condp = নূতন AstCond{flp, onTickCondp, onTickp, betweenTicksp};
            condp->dtypeFrom(onTickp);
            inp = condp;
        } অন্যথা {
            inp = get_Past_মান(exprp, initp, senTreep, ticks);
        }
        nodep->replaceWith(inp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    //========== Move $sampled down to read-only variables
    শূন্য visit(AstSampled* nodep) override {
        যদি (nodep->user1()) প্রত্যাগম;
        VL_RESTORER(m_inSampled);
        {
            m_inSampled = সত্য;
            iterateChildren(nodep);
        }
        যদি (nodep->exprp()) {
            nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        } অন্যথা {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        যদি (m_inSampled && !nodep->user2() && !nodep->varp()->isTemp()) {
            যদি (!nodep->access().isReadOnly()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Write to variable in sampled expression");
            } অন্যথা {
                VNRelinker relinkHandle;
                nodep->unlinkFrBack(&relinkHandle);
                AstSampled* ধ্রুব newp = new_Sampled_রাশিমালা(nodep);
                relinkHandle.relink(newp);
                newp->user1(1);
                v3Global.setHasSampled();
            }
        }
    }
    // Don't sample sensitivities
    শূন্য visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSampled);
        m_inSampled = অসত্য;
        iterateChildren(nodep);
    }
    শূন্য visit(AstPExprClause* nodep) override {
        Ast_গ্রন্থি* stmtsp = nullptr;
        যদি (nodep->pass() && m_passsp) {
            // Cover adds COVERINC by AstNode::addNext, thus need to clone next too.
            stmtsp = m_passsp->clone_বৃক্ষ(সত্য);
            stmtsp = newIfAssertPassOn(stmtsp, m_assertp->directive(), m_assertp->userType(),
                                       nodep->vacuous());
        } অন্যথা যদি (!nodep->pass() && m_failsp) {
            stmtsp = m_failsp->clone_বৃক্ষ(সত্য);
            stmtsp = newIfAssertFailOn(stmtsp, m_assertp->directive(), m_assertp->userType());
        }
        যদি (stmtsp) {
            stmtsp->foreachAndNext([](Ast_গ্রন্থি_Var_Ref* ধ্রুব refp) {
                // References inside action blocks shouldn't be implicitly sampled
                // m_passsp/m_failsp have been already visited once and refs explicitly sampled
                // are handled already
                refp->user2(1);
            });
            nodep->replaceWith(stmtsp);
        } অন্যথা {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstPExpr* nodep) override {
        যদি (m_inRestrict) {
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
        } অন্যথা {
            iterateChildren(nodep);
        }
    }

    //========== Statements
    শূন্য visit(AstDisplay* nodep) override {
        iterateChildren(nodep);
        // Replace the special types with standard text
        যদি (nodep->displayType() == VDisplayType::DT_INFO) {
            replaceDisplay(nodep, "-Info");
        } অন্যথা যদি (nodep->displayType() == VDisplayType::DT_WARNING) {
            replaceDisplay(nodep, "%%Warning");
        } অন্যথা যদি (nodep->displayType() == VDisplayType::DT_ERROR) {
            replaceDisplay(nodep, "%%Error");
        } অন্যথা যদি (nodep->displayType() == VDisplayType::DT_FATAL) {
            replaceDisplay(nodep, "%%Fatal");
        } অন্যথা যদি (nodep->displayType() == VDisplayType::DT_MONITOR) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            File_সারি* ধ্রুব fl = nodep->fileline();

            AstSenItem* monSenItemsp = nullptr;
            যদি (Ast_গ্রন্থি* ধ্রুব monExprsp = nodep->fmtp()->exprsp()) {
                monExprsp->foreachAndNext([&](AstVarRef* varrefp) {
                    AstSenItem* ধ্রুব senItemp
                        = নূতন AstSenItem{fl, VEdgeType::ET_CHANGED,
                                         // Clone so get VarRef or VarXRef as needed
                                         varrefp->clone_বৃক্ষ(অসত্য)};
                    monSenItemsp = Ast_গ্রন্থি::addNextNull(monSenItemsp, senItemp);
                });
            }

            Ast_Sen_বৃক্ষ* ধ্রুব mon_Sen_বৃক্ষ = নূতন Ast_Sen_বৃক্ষ{fl, monSenItemsp};
            ধ্রুব auto mon_অঙ্ক = ++m_monitor_অঙ্ক;
            // Where $monitor was we do "__VmonitorNum = N;"
            AstAssign* ধ্রুব newsetp = নূতন AstAssign{
                fl, new_Monitor_অঙ্ক_Var_Refp(nodep, VAccess::WRITE), নূতন AstConst{fl, mon_অঙ্ক}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__VmonitorOn && __VmonitorNum==N) $display(...);"
            Ast_গ্রন্থি* ধ্রুব stmtsp = nodep;
            AstIf* ধ্রুব ifp = নূতন AstIf{
                fl,
                নূতন AstLogAnd{fl, নূতন AstLogNot{fl, newMonitorOffVarRefp(nodep, VAccess::READ)},
                              নূতন AstEq{fl, নূতন AstConst{fl, mon_অঙ্ক},
                                        new_Monitor_অঙ্ক_Var_Refp(nodep, VAccess::READ)}},
                stmtsp};
            ifp->isBoundsCheck(সত্য);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_গ্রন্থি* ধ্রুব newp = নূতন AstAlways{fl, VAlwaysKwd::ALWAYS, mon_Sen_বৃক্ষ, ifp};
            m_modp->addStmtsp(newp);
        } অন্যথা যদি (nodep->displayType() == VDisplayType::DT_STROBE) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            // Need one-shot
            File_সারি* ধ্রুব fl = nodep->fileline();
            AstVar* ধ্রুব varp
                = নূতন AstVar{fl, VVarType::MODULETEMP, "__Vstrobe" + cvtToStr(m_mod_Strobe_অঙ্ক++),
                             nodep->findBitDType()};
            m_modp->addStmtsp(varp);
            // Where $strobe was we do "__Vstrobe = '1;"
            AstAssign* ধ্রুব newsetp = নূতন AstAssign{fl, নূতন AstVarRef{fl, varp, VAccess::WRITE},
                                                     নূতন AstConst{fl, AstConst::BitTrue{}}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__Vstrobe) begin $display(...); __Vstrobe = '0; end"
            Ast_গ্রন্থি* ধ্রুব stmtsp = nodep;
            AstIf* ধ্রুব ifp = নূতন AstIf{fl, নূতন AstVarRef{fl, varp, VAccess::READ}, stmtsp};
            ifp->isBoundsCheck(সত্য);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_গ্রন্থি* ধ্রুব newp = নূতন AstAlwaysPostponed{fl, ifp};
            stmtsp->addNext(নূতন AstAssign{fl, নূতন AstVarRef{fl, varp, VAccess::WRITE},
                                          নূতন AstConst{fl, AstConst::BitFalse{}}});
            m_modp->addStmtsp(newp);
        }
    }
    শূন্য visit(AstMonitorOff* nodep) override {
        AstAssign* ধ্রুব newp
            = নূতন AstAssign{nodep->fileline(), newMonitorOffVarRefp(nodep, VAccess::WRITE),
                            নূতন AstConst{nodep->fileline(), AstConst::BitTrue{}, nodep->off()}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstAssert* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    শূন্য visit(AstAssertCtl* nodep) override {
        iterateChildren(nodep);

        তার্কিক assertTypeConst = সত্য;
        যদি (!nodep->assertTypesp()) {
            nodep->assertTypes(VAssertType{ALL_ASSERT_TYPES});
        } অন্যথা যদি (ধ্রুব AstConst* ধ্রুব assertTypesp = VN_CAST(nodep->assertTypesp(), Const)) {
            nodep->assertTypes(VAssertType{assertTypesp->toSInt()});
        } অন্যথা {
            assertTypeConst = অসত্য;
        }

        তার্কিক controlTypeConst = অসত্য;
        যদি (ধ্রুব AstConst* ধ্রুব constp = VN_CAST(nodep->controlTypep(), Const)) {
            nodep->ctlType(constp->toSInt());
            controlTypeConst = সত্য;
        }
        যদি (controlTypeConst
            && (nodep->ctlType() < VAssertCtlType::LOCK
                || nodep->ctlType() > VAssertCtlType::VACUOUS_OFF)) {
            nodep->unlinkFrBack();
            nodep->v3error("Bad $assertcontrol control_type '"
                           << cvtToStr(static_cast<পূর্ণাঙ্ক>(nodep->ctlType()))
                           << "' (IEEE 1800-2023 Table 20-5)");
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম;
        }
        যদি (assertTypeConst && nodep->assertTypes() != ALL_ASSERT_TYPES
            && nodep->assertTypes().containsAny(VAssertType::UNIQUE | VAssertType::UNIQUE0
                                                | VAssertType::PRIORITY)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: assert control assertion_type");
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            প্রত্যাগম;
        }

        তার্কিক directiveTypeConst = সত্য;
        যদি (!nodep->directiveTypesp()) {
            nodep->directiveTypes(VAssertDirectiveType::ASSERT | VAssertDirectiveType::ASSUME
                                  | VAssertDirectiveType::COVER);
        } অন্যথা যদি (ধ্রুব AstConst* ধ্রুব directiveTypesp
                   = VN_CAST(nodep->directiveTypesp(), Const)) {
            nodep->directiveTypes(VAssertDirectiveType{directiveTypesp->toSInt()});
        } অন্যথা {
            directiveTypeConst = অসত্য;
        }
        যদি (!directiveTypeConst) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: non-const assert directive type expression");
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            প্রত্যাগম;
        }

        File_সারি* ধ্রুব fl = nodep->fileline();
        UINFO(9, "Generating assertctl for a module: " << m_modp);
        AstCStmt* ধ্রুব newp = নূতন AstCStmt{fl};
        newp->add("vlSymsp->_vm_contextp__->assertCtl(");
        newp->add(nodep->controlTypep()->unlinkFrBack());
        newp->add(", ");
        যদি (nodep->assertTypesp()) {
            newp->add(nodep->assertTypesp()->unlinkFrBack());
        } অন্যথা {
            newp->add(std::to_string(ALL_ASSERT_TYPES));
        }
        newp->add(", " + std::to_string(nodep->directiveTypes()) + ");\n");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstAssertIntrinsic* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    শূন্য visit(AstCover* nodep) override {  //
        visitAssertionIterate(nodep, nullptr);
    }
    শূন্য visit(AstRestrict* nodep) override {
        VL_RESTORER(m_inRestrict);
        m_inRestrict = সত্য;
        iterateChildren(nodep);
        // IEEE says simulator ignores these
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }

    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Past_অঙ্ক);
        VL_RESTORER(m_mod_Strobe_অঙ্ক);
        VL_RESTORER(m_finalp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DelayedAlwaysp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DefaultDelayedAlwaysp);
        VL_RESTORER_CLEAR(m_delayedAlways2TickTimep);
        m_modp = nodep;
        m_mod_Past_অঙ্ক = 0;
        m_mod_Strobe_অঙ্ক = 0;
        m_finalp = nullptr;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstGenBlock* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstBegin* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
        // If the body is a single assertOn check, bubble it up for combining
        যদি (!nodep->declsp()) {
            যদি (AstIf* ধ্রুব ifp = VN_CAST(nodep->stmtsp(), If)) {
                যদি (ifp->user2() && !ifp->nextp()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(ifp->unlinkFrBack());
                    nodep->addStmtsp(ifp->thensp()->unlinkFrBackWithNext());
                    ifp->addThensp(nodep);
                }
            }
        }
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
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
        VIsCached::clear_Cache_বৃক্ষ();
    }
};

//######################################################################
// Top Assert class

শূন্য V3Assert::assertAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("assert", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
