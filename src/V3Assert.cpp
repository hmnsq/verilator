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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Assert.h"

#அந்தர்பாவ "V3AstUserAllocator.h"
#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3Stats.h"
#அந்தர்பாவ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

நாமாகாஶ {

வகுப்பு DefaultDisableLocalVisitor final : பொது VNVisitor {
    // STATE
    Ast_முனை* m_scopep = nullptr;

    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(nullptr);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(nullptr);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstDefaultDisable* nodep) override {
        UASSERT_OBJ(nodep, m_scopep,
                    "default disable iff must be inside a module or generate block");
        AstDefaultDisable* defaultp = nullptr;
        எனில் (மாறிலி Ast_முனை_Module* மாறிலி modp = VN_CAST(m_scopep, முனை_Module)) {
            defaultp = modp->defaultDisablep();
        } இல்லையெனில் {
            defaultp = VN_AS(m_scopep, GenBlock)->defaultDisablep();
        }
        எனில் (VL_UNLIKELY(defaultp)) {
            nodep->v3error("Only one 'default disable iff' allowed per "
                           << (VN_IS(m_scopep, முனை_Module) ? "module" : "generate block")
                           << " (IEEE 1800-2023 16.15)");
        } இல்லையெனில் எனில் (Ast_முனை_Module* மாறிலி modp = VN_CAST(m_scopep, முனை_Module)) {
            modp->defaultDisablep(nodep);
        } இல்லையெனில் {
            VN_AS(m_scopep, GenBlock)->defaultDisablep(nodep);
        }
    }
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    explicit DefaultDisableLocalVisitor(AstNetlist* nodep) { iterate(nodep); }
};

வகுப்பு DefaultDisablePropagateVisitor final : பொது VNVisitor {
    // STATE
    AstDefaultDisable* m_defaultDisablep = nullptr;

    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    வெறுமை visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        எனில் (!nodep->defaultDisablep()) nodep->defaultDisablep(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    explicit DefaultDisablePropagateVisitor(AstNetlist* nodep) { iterate(nodep); }
};

// Lower a sequence used as an event control ('@seq', IEEE 1800-2023 9.4.2.4) into a
// synthesized event fired by an internal 'cover sequence' on each end-of-match
வகுப்பு SeqEventLowerVisitor final : பொது VNVisitor {
    // STATE
    Ast_முனை_Module* m_modp = nullptr;  // Current module
    V3UniqueNames m_names{"__VseqEvent"};  // Synthesized event names

    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstSenItem* nodep) override {
        AstFuncRef* மாறிலி funcrefp = VN_CAST(nodep->sensp(), FuncRef);
        எனில் (funcrefp && VN_IS(funcrefp->taskp(), Sequence)) {
            File_வரி* மாறிலி flp = nodep->fileline();
            AstVar* மாறிலி eventp = புதிய AstVar{flp, VVarType::MODULETEMP, m_names.get(nodep),
                                              m_modp->findBasicDType(VBasicDTypeKwd::EVENT)};
            eventp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(eventp);
            v3Global.setHasEvents();
            funcrefp->unlinkFrBack();
            nodep->sensp(புதிய AstVarRef{flp, eventp, VAccess::READ});
            மாறிலி தருக்க automaticActual = funcrefp->exists([](மாறிலி Ast_முனை_Var_Ref* refp) {
                திரும்பு refp->varp() && refp->varp()->lifetime().isAutomatic();
            });
            எனில் (automaticActual) {
                nodep->v3error("Arguments to a sequence used as an event control must be"
                               " static (IEEE 1800-2023 9.4.2.4)");
                VN_AS(funcrefp->taskp(), Sequence)->isReferenced(பொய்);
                VL_DO_DANGLING(தள்ளு_Deletep(funcrefp), funcrefp);
                திரும்பு;
            }
            AstFireEvent* மாறிலி firep
                = புதிய AstFireEvent{flp, புதிய AstVarRef{flp, eventp, VAccess::WRITE}, பொய்};
            AstCover* மாறிலி coverp
                = புதிய AstCover{flp, புதிய AstPropSpec{flp, nullptr, nullptr, funcrefp}, firep,
                               VAssertType::CONCURRENT};
            coverp->isCoverSeq(உண்மை);
            coverp->isSeqEvent(உண்மை);
            m_modp->addStmtsp(coverp);
            திரும்பு;
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    explicit SeqEventLowerVisitor(AstNetlist* nodep) { iterate(nodep); }
};

}  // namespace

வெறுமை V3AssertCommon::collectDefaultDisable(AstNetlist* nodep) {
    { DefaultDisableLocalVisitor{nodep}; }
    { DefaultDisablePropagateVisitor{nodep}; }
}

வெறுமை V3AssertCommon::lowerSequenceEvents(AstNetlist* nodep) {
    { SeqEventLowerVisitor{nodep}; }
    V3Global::dump_Check_Global_மரம்("assertseqevent", 0, dump_மரம்_Either_Level() >= 3);
}

string V3AssertCommon::assertCtlGetCall(மாறிலி எழுத்து* query, VAssertType type,
                                        VAssertDirectiveType directiveType) {
    // Template arguments let the runtime build the type/directive mask at C++ compile time
    திரும்பு "vlSymsp->_vm_contextp__->assertCtlGet<"s + std::to_string(type) + ", "s
           + std::to_string(directiveType) + ">(VerilatedAssertCtlQuery::"s + query + ")"s;
}

தருக்க V3AssertCommon::check_Sva_மதிப்பு_Limit(Ast_முனை* nodep, மாறிலி AstConst* constp, மாறிலி எழுத்து* what) {
    // Temporal values can materialize as O(N) AST or state in later passes.
    // Leave headroom for a range ring's extra slot and V3Number's signed
    // `(bits + 31) / 32` word count.
    நிலையான constexpr unsigned SVA_மதிப்பு_HARD_LIMIT
        = static_cast<unsigned>(std::numeric_limits<முழு>::max()) - 32U;
    மாறிலி unsigned configuredLimit = static_cast<unsigned>(v3Global.opt.max_இலக்கம்_Width());
    மாறிலி unsigned limit = std::min(configuredLimit, SVA_மதிப்பு_HARD_LIMIT);
    எனில் (constp->இலக்கம்().fitsInUInt() && constp->toUInt() <= limit) திரும்பு உண்மை;
    nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << what << " exceeds implementation limit of "
                                                 << limit
                                                 << (configuredLimit <= SVA_மதிப்பு_HARD_LIMIT
                                                         ? " (--max-num-width)"
                                                         : " (host arithmetic limit)"));
    திரும்பு பொய்;
}

// The bound of a cycle delay in a sequence, if a non-negative elaboration-time constant
நிலையான மாறிலி AstConst* checkSvaDelayBound(Ast_முனை* errp, Ast_முனை_கோவை* boundp, மாறிலி எழுத்து* what) {
    மாறிலி AstConst* மாறிலி constp = VN_CAST(boundp, Const);
    எனில் (!constp || (constp->dtypep()->isSigned() && constp->இலக்கம்().isNegative())) {
        errp->v3error(what << " is not a non-negative elaboration-time constant"
                              " (IEEE 1800-2023 16.7)");
        திரும்பு nullptr;
    }
    எனில் (!V3AssertCommon::check_Sva_மதிப்பு_Limit(errp, constp, "SVA cycle delay")) திரும்பு nullptr;
    திரும்பு constp;
}

// Fold the bounds of a cycle delay in a sequence, and return if they are valid, else report it
// on errp
நிலையான தருக்க svaCycleDelayValid(மாறிலி AstDelay* nodep, Ast_முனை* errp) {
    V3Const::constifyParamsNoWarnEdit(nodep->lhsp());
    மாறிலி AstConst* மாறிலி minConstp = checkSvaDelayBound(
        errp, nodep->lhsp(), nodep->isRangeDelay() ? "Range delay minimum" : "Delay value");
    எனில் (!nodep->rhsp() || nodep->isUnbounded()) திரும்பு minConstp != nullptr;
    V3Const::constifyParamsNoWarnEdit(nodep->rhsp());
    மாறிலி AstConst* மாறிலி maxConstp
        = checkSvaDelayBound(errp, nodep->rhsp(), "Range delay maximum");
    எனில் (!minConstp || !maxConstp) திரும்பு பொய்;
    எனில் (maxConstp->toUInt() < minConstp->toUInt()) {
        errp->v3error("Range delay maximum must be >= minimum (IEEE 1800-2023 16.7)");
        திரும்பு பொய்;
    }
    திரும்பு உண்மை;
}

வெறுமை V3AssertCommon::checkSvaCycleDelay(AstDelay* nodep, Ast_முனை* errp) {
    எனில் (svaCycleDelayValid(nodep, errp)) திரும்பு;
    // Make the delay valid, so later steps do not fail after the error
    File_வரி* மாறிலி flp = nodep->fileline();
    Ast_முனை_கோவை* மாறிலி minp = nodep->lhsp();
    minp->replaceWith(புதிய AstConst{flp, AstConst::BitFalseErroring{}});
    VL_DO_DANGLING(minp->delete_மரம்(), minp);
    எனில் (nodep->rhsp() && !nodep->isUnbounded()) {
        Ast_முனை_கோவை* மாறிலி maxp = nodep->rhsp();
        maxp->replaceWith(புதிய AstConst{flp, AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(maxp->delete_மரம்(), maxp);
    }
}

தருக்க V3AssertCommon::refsSvaFormal(மாறிலி Ast_முனை* nodep) {
    திரும்பு nodep->exists([](மாறிலி Ast_முனை_Var_Ref* refp) {
        மாறிலி AstVar* மாறிலி varp = refp->varp();
        எனில் (!varp || !varp->isIO()) திரும்பு பொய்;
        மாறிலி Ast_முனை* மாறிலி declp = varp->aboveLoopp();
        திரும்பு VN_IS(declp, Sequence) || VN_IS(declp, Property);
    });
}

வெறுமை V3AssertCommon::checkSvaInstanceDelays(Ast_முனை* nodep, Ast_முனை* instp) {
    // A delay still referencing a formal argument of an enclosing sequence or property is
    // checked when that is instanced. Collect first, as checking edits the delays.
    std::vector<AstDelay*> delayps;
    nodep->foreach([&](AstDelay* delayp) {
        எனில் (!delayp->isCycleDelay() || refsSvaFormal(delayp->lhsp())) திரும்பு;
        எனில் (delayp->rhsp() && refsSvaFormal(delayp->rhsp())) திரும்பு;
        delayps.தள்ளு_back(delayp);
    });
    க்கு (AstDelay* மாறிலி delayp : delayps) checkSvaCycleDelay(delayp, instp);
}

//######################################################################
// AssertDeFutureVisitor
// If any AstFuture, then move all non-future varrefs to be one cycle behind,
// see IEEE 1800-2023 16.9.4.

வகுப்பு AssertDeFutureVisitor final : பொது VNVisitor {
    // STATE - across all visitors
    Ast_முனை_Module* மாறிலி m_modp;  // Module future is underneath
    மாறிலி AstFuture* m_futurep;  // First AstFuture found
    மாறிலி unsigned m_past_இலக்கம்;  // Prefix unique number for this module
    std::map<AstVar*, AstVar*> m_delayedVars;  // Old to delayed variable mapping
    // STATE - for current visit position (use VL_RESTORER)
    தருக்க m_inFuture = பொய்;  // Inside a future
    தருக்க m_unsupported = பொய்;  // Printed unsupported

    // METHODS
    வெறுமை unsupported(Ast_முனை* nodep) {
        எனில் (m_unsupported) திரும்பு;
        m_unsupported = உண்மை;
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported/illegal: Future value function used with expression with "
                          << nodep->prettyOperatorName());
    }
    // VISITORS
    வெறுமை visit(AstFuture* nodep) override {
        VL_RESTORER(m_inFuture);
        m_inFuture = உண்மை;
        iterateChildren(nodep);
        // Done with the future, this subexpression is current-time
        nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(Ast_முனை_Var_Ref* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;
        எனில் (m_inFuture || m_unsupported)
            திரும்பு;  // Need user1 set above, don't process when Future is removed
        எனில் (nodep->access().isWriteOrRW()) {
            unsupported(nodep);
            திரும்பு;
        }
        auto it = m_delayedVars.find(nodep->varp());
        AstVar* outvarp;
        எனில் (it == m_delayedVars.end()) {
            Ast_Sen_மரம்* மாறிலி sentreep = m_futurep->sentreep();
            AstAlways* மாறிலி alwaysp = புதிய AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS,
                                                     sentreep->clone_மரம்(பொய்), nullptr};
            m_modp->addStmtsp(alwaysp);
            outvarp = புதிய AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                 "__Vnotfuture" + cvtToStr(m_past_இலக்கம்) + "_" + nodep->name(),
                                 nodep->dtypep()};
            m_modp->addStmtsp(outvarp);
            AstVarRef* varRefAWritep = புதிய AstVarRef{nodep->fileline(), outvarp, VAccess::WRITE};
            varRefAWritep->user1(உண்மை);
            Ast_முனை_Var_Ref* varRefAReadp = nodep->clone_மரம்(பொய்);
            varRefAReadp->user1(உண்மை);
            Ast_முனை* மாறிலி assp = புதிய AstAssignDly{nodep->fileline(), varRefAWritep, varRefAReadp};
            alwaysp->addStmtsp(assp);
            m_delayedVars.emplace(nodep->varp(), outvarp);
        } இல்லையெனில் {
            outvarp = it->second;
        }
        AstVarRef* newp = புதிய AstVarRef{nodep->fileline(), outvarp, VAccess::READ};
        newp->user1(உண்மை);
        UINFO(9, "DeFuture " << nodep << "  becomes " << newp);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(Ast_முனை_FTask_Ref* nodep) override { unsupported(nodep); }
    வெறுமை visit(AstMethodCall* nodep) override { unsupported(nodep); }
    வெறுமை visit(Ast_முனை* nodep) override {
        எனில் (!nodep->isPure()) unsupported(nodep);
        iterateChildren(nodep);
    }

பொது:
    // CONSTRUCTORS
    explicit AssertDeFutureVisitor(Ast_முனை* nodep, Ast_முனை_Module* modp, unsigned past_இலக்கம்)
        : m_modp{modp}
        , m_past_இலக்கம்{past_இலக்கம்} {
        // See if any Future before we process
        எனில் (nodep->forall([&](மாறிலி AstFuture* futurep) -> தருக்க {
                m_futurep = futurep;
                திரும்பு பொய்;
            }))
            திரும்பு;
        // UINFOTREE(9, nodep, "", "defuture-in");
        visit(nodep);  // Nodep may get deleted
        // UINFOTREE(9, nodep, "", "defuture-ou");
    }
    ~AssertDeFutureVisitor() = இயல்பு;
};

// Prepare one default sampled expression.
வகுப்பு Default_Sampled_மதிப்பு_Visitor final : பொது VNVisitor {
    // METHODS
    நிலையான AstConst* newTypeDefault(Ast_முனை_கோவை* nodep) {
        AstConst* மாறிலி constp
            = புதிய AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()};
        எனில் (nodep->dtypep()->isFourstate()) constp->இலக்கம்().setAllBitsX();
        constp->dtypeFrom(nodep);
        திரும்பு constp;
    }

    // VISITORS
    வெறுமை visit(AstCMethodHard* nodep) override {
        எனில் (nodep->method() == VCMethod::EVENT_IS_TRIGGERED) {
            nodep->replaceWith(புதிய AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் {
            iterateChildren(nodep);
        }
    }
    வெறுமை visit(Ast_முனை_Var_Ref* nodep) override {
        AstVar* மாறிலி varp = nodep->varp();
        எனில் (varp->isParam()
            || (!varp->isNet() && varp->lifetime().isStatic() && varp->hasUserInit())) {
            திரும்பு;
        }
        nodep->replaceWith(newTypeDefault(nodep));
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    explicit Default_Sampled_மதிப்பு_Visitor(Ast_முனை* nodep) { iterate(nodep); }
};

//######################################################################
// AssertVisitor

வகுப்பு AssertVisitor final : பொது VNVisitor {
    // CONSTANTS
    நிலையான constexpr uint8_t ALL_ASSERT_TYPES
        = std::numeric_limits<std::underlying_type<VAssertType::en>::type>::max();

    // NODE STATE/TYPES
    // Cleared on netlist
    //  AstNode::user1()         -> bool.  True if processed
    //  AstAlways::user2p()      -> std::vector<AstVar*>. Delayed variables via 'm_delayed'
    //  AstNodeVarRef::user2()   -> bool.  True if shouldn't be sampled
    //  AstIf::user2()           -> bool.  True for assertOn() checks inserted in this pass
    மாறிலி VNUser1InUse m_user1InUse;
    மாறிலி VNUser2InUse m_user2InUse;
    AstUser2Allocator<AstAlways, std::vector<AstVar*>> m_delayed;

    // STATE
    Ast_முனை_Module* m_modp = nullptr;  // Last module
    மாறிலி Ast_முனை* m_beginp = nullptr;  // Last AstBegin/AstGenBlock
    unsigned m_monitor_இலக்கம் = 0;  // Global $monitor numbering (not per module)
    AstVar* m_monitor_இலக்கம்_Varp = nullptr;  // $monitor number variable
    AstVar* m_monitorOffVarp = nullptr;  // $monitoroff variable
    unsigned m_mod_Past_இலக்கம் = 0;  // Module past numbering
    unsigned m_mod_Strobe_இலக்கம் = 0;  // Module $strobe numbering
    Ast_முனை_Procedure* m_procedurep = nullptr;  // Current procedure
    VDouble0 m_statCover;  // Statistic tracking
    VDouble0 m_statAsNotImm;  // Statistic tracking
    VDouble0 m_statAsImm;  // Statistic tracking
    VDouble0 m_statAsFull;  // Statistic tracking
    VDouble0 m_statPastVars;  // Statistic tracking
    VDouble0 m_statAssertOnCombined;  // Statistic tracking
    VDouble0 m_statAssertOnHoisted;  // Statistic tracking
    தருக்க m_inSampled = பொய்;  // True inside a sampled expression
    தருக்க m_inRestrict = பொய்;  // True inside restrict assertion
    Ast_முனை* m_passsp = nullptr;  // Current pass statement
    Ast_முனை* m_failsp = nullptr;  // Current fail statement
    Ast_முனை_Cover_Or_Assert* m_assertp = nullptr;  // Current assertion
    AstFinal* m_finalp = nullptr;  // Current final block
    VDouble0 m_statLiftedCaseExprs;  // Count of purified case expressions
    Ast_முனை_FTask* m_ftaskp = nullptr;  // Current function/task
    V3UniqueNames m_caseTempNames{"__VCase"};
    V3UniqueNames m_match_மொத்தம்_Names{"__VnfaRemainingMatchCount"};  // Match replay counter names
    // Maps from (expression, senTree) to the AstAlways that computes its delayed values.
    std::unordered_map<VNRef<Ast_முனை_கோவை>, std::unordered_map<VNRef<Ast_Sen_மரம்>, AstAlways*>>
        m_modExpr2Sen2DelayedAlwaysp;
    // Source sampled-value history is initialized, unlike internal property-lowering history.
    std::unordered_map<VNRef<Ast_முனை_கோவை>, std::unordered_map<VNRef<Ast_Sen_மரம்>, AstAlways*>>
        m_modExpr2Sen2DefaultDelayedAlwaysp;
    // Map from delayed-value AstAlways to its last-sampling-tick time variable
    std::unordered_map<மாறிலி AstAlways*, AstVar*> m_delayedAlways2TickTimep;

    // METHODS
    நிலையான மாறிலி எழுத்து* assertPassOnQuery(தருக்க vacuous) {
        நிலையான constexpr மாறிலி எழுத்து* queries[2]
            = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
        திரும்பு queries[vacuous];
    }
    நிலையான Ast_முனை_கோவை* assertOnCond(File_வரி* fl, VAssertType type,
                                     VAssertDirectiveType directiveType) {
        // cppcheck-suppress missingReturn
        தேர்வு (directiveType) {
        நிலை VAssertDirectiveType::INTRINSIC: திரும்பு புதிய AstConst{fl, AstConst::BitTrue{}};
        நிலை VAssertDirectiveType::VIOLATION_CASE: {
            எனில் (v3Global.opt.assertCase()) {
                திரும்பு புதிய AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            // If assertions are off, have constant propagation rip them out later
            // This allows syntax errors and such to be detected normally.
            திரும்பு புதிய AstConst{fl, AstConst::BitFalse{}};
        }
        நிலை VAssertDirectiveType::ASSERT:
        நிலை VAssertDirectiveType::COVER:
        நிலை VAssertDirectiveType::ASSUME: {
            எனில் (v3Global.opt.assertOn()) {
                திரும்பு புதிய AstCExpr{
                    fl, AstCExpr::Pure{},
                    V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
            }
            திரும்பு புதிய AstConst{fl, AstConst::BitFalse{}};
        }
        நிலை VAssertDirectiveType::INTERNAL:
        நிலை VAssertDirectiveType::VIOLATION_IF:
        நிலை VAssertDirectiveType::RESTRICT: {
            எனில் (v3Global.opt.assertOn()) {
                திரும்பு புதிய AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            திரும்பு புதிய AstConst{fl, AstConst::BitFalse{}};
        }
        }
        VL_UNREACHABLE;
    }

    நிலையான தருக்க isControlled(VAssertDirectiveType directiveType) {
        திரும்பு (static_cast<முழு>(directiveType)
                & (static_cast<முழு>(VAssertDirectiveType::ASSERT)
                   | static_cast<முழு>(VAssertDirectiveType::COVER)
                   | static_cast<முழு>(VAssertDirectiveType::ASSUME)));
    }
    நிலையான Ast_முனை_கோவை* assertPassOnCond(File_வரி* fl, VAssertType type,
                                         VAssertDirectiveType directiveType, தருக்க vacuous) {
        எனில் (!isControlled(directiveType)) திரும்பு புதிய AstConst{fl, AstConst::BitTrue{}};
        எனில் (!v3Global.opt.assertOn()) திரும்பு புதிய AstConst{fl, AstConst::BitFalse{}};
        திரும்பு புதிய AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType), 1};
    }
    நிலையான Ast_முனை_கோவை* assertFailOnCond(File_வரி* fl, VAssertType type,
                                         VAssertDirectiveType directiveType) {
        எனில் (!isControlled(directiveType)) திரும்பு புதிய AstConst{fl, AstConst::BitTrue{}};
        எனில் (!v3Global.opt.assertOn()) திரும்பு புதிய AstConst{fl, AstConst::BitFalse{}};
        திரும்பு புதிய AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType), 1};
    }
    string assertDisplayMessage(மாறிலி Ast_முனை* nodep, மாறிலி string& prefix, மாறிலி string& message,
                                VDisplayType severity) {
        எனில் (severity == VDisplayType::DT_ERROR || severity == VDisplayType::DT_FATAL) {
            திரும்பு ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                    + cvtToStr(nodep->fileline()->lineno()) + ": Assertion failed in %m"
                    + ((message != "") ? ": " : "") + message + "\n");
        }
        திரும்பு ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                + cvtToStr(nodep->fileline()->lineno()) + ": %m" + ((message != "") ? ": " : "")
                + message + "\n");
    }
    வெறுமை replaceDisplay(AstDisplay* nodep, மாறிலி string& prefix) {
        nodep->fmtp()->text(
            assertDisplayMessage(nodep, prefix, nodep->fmtp()->text(), nodep->displayType()));
        nodep->displayType(VDisplayType::DT_WRITE);
        // cppcheck-suppress nullPointer
        Ast_முனை_கோவை* மாறிலி timenewp = புதிய AstTime{nodep->fileline(), m_modp->timeunit()};
        nodep->fmtp()->prependExprsp(timenewp);
        எனில் (!nodep->fmtp()->scopeNamep() && nodep->fmtp()->formatScopeTracking()) {
            nodep->fmtp()->scopeNamep(புதிய AstScopeName{nodep->fileline(), உண்மை});
        }
    }
    AstSampled* new_Sampled_கோவை(Ast_முனை_கோவை* nodep) {
        திரும்பு புதிய AstSampled{nodep->fileline(), nodep, nodep->dtypep(), உண்மை};
    }
    AstVarRef* new_Monitor_இலக்கம்_Var_Refp(மாறிலி Ast_முனை* nodep, VAccess access) {
        எனில் (!m_monitor_இலக்கம்_Varp) {
            m_monitor_இலக்கம்_Varp = புதிய AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorNum",
                                          nodep->findUInt64DType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitor_இலக்கம்_Varp);
        }
        AstVarRef* மாறிலி varrefp = புதிய AstVarRef{nodep->fileline(), m_monitor_இலக்கம்_Varp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        திரும்பு varrefp;
    }
    AstVarRef* newMonitorOffVarRefp(மாறிலி Ast_முனை* nodep, VAccess access) {
        எனில் (!m_monitorOffVarp) {
            m_monitorOffVarp = புதிய AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorOff",
                                          nodep->findBitDType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitorOffVarp);
        }
        AstVarRef* மாறிலி varrefp = புதிய AstVarRef{nodep->fileline(), m_monitorOffVarp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        திரும்பு varrefp;
    }
    நிலையான AstIf* newIfAssertOn(Ast_முனை* bodyp, VAssertDirectiveType directiveType,
                                VAssertType type = VAssertType::INTERNAL) {
        // Add an internal if to check assertions are on.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_வரி* மாறிலி fl = bodyp->fileline();

        Ast_முனை_கோவை* மாறிலி condp = assertOnCond(fl, type, directiveType);
        AstIf* மாறிலி newp = புதிய AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(உண்மை);  // To avoid LATCH warning
        newp->user2(உண்மை);  // Mark as an assertOn() check
        திரும்பு newp;
    }
    நிலையான Ast_முனை_Stmt* newIfAssertPassOn(Ast_முனை* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type, தருக்க vacuous) {
        // Add an internal if to check assertion passOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_வரி* மாறிலி fl = bodyp->fileline();
        Ast_முனை_கோவை* மாறிலி condp = assertPassOnCond(fl, type, directiveType, vacuous);
        Ast_முனை_If* மாறிலி newp = புதிய AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(உண்மை);  // To avoid LATCH warning
        newp->user1(உண்மை);  // Don't assert/cover this if
        newp->user2(உண்மை);  // Mark as an assertOn() check
        திரும்பு newp;
    }
    நிலையான Ast_முனை_Stmt* newIfAssertFailOn(Ast_முனை* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type) {
        // Add an internal if to check assertion failOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_வரி* மாறிலி fl = bodyp->fileline();
        Ast_முனை_கோவை* மாறிலி condp = assertFailOnCond(fl, type, directiveType);
        Ast_முனை_If* மாறிலி newp = புதிய AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(உண்மை);  // To avoid LATCH warning
        newp->user1(உண்மை);  // Don't assert/cover this if
        newp->user2(உண்மை);  // Mark as an assertOn() check
        திரும்பு newp;
    }

    நிலையான AstIf* assertCond(மாறிலி Ast_முனை_Cover_Or_Assert* nodep, Ast_முனை_கோவை* propp,
                             Ast_முனை* passsp, Ast_முனை* failsp) {
        AstIf* மாறிலி ifp = புதிய AstIf{nodep->fileline(), propp, passsp, failsp};
        // It's more LIKELY that we'll take the nullptr if clause
        // than the sim-killing else clause:
        ifp->branchPred(VBranchPred::BP_LIKELY);
        ifp->isBoundsCheck(உண்மை);  // To avoid LATCH warning
        திரும்பு ifp;
    }

    Ast_முனை_Stmt* assertBody(மாறிலி Ast_முனை_Cover_Or_Assert* nodep, Ast_முனை* propp, Ast_முனை* passsp,
                            Ast_முனை* failsp) {
        எனில் (AstPExpr* மாறிலி pexprp = VN_CAST(propp, PExpr)) {
            எனில் (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NOTIMING, "This property expression requires --timing");
                VL_DO_DANGLING(தள்ளு_Deletep(pexprp), pexprp);
                திரும்பு புதிய AstBegin{nodep->fileline(), "", nullptr, பொய்};
            }
            AstFork* மாறிலி forkp = புதிய AstFork{nodep->fileline(), VJoinType::JOIN_NONE};
            forkp->addForksp(pexprp->bodyp()->unlinkFrBack());
            எனில் (Ast_முனை_Stmt* மாறிலி finalp = pexprp->finalp()) {
                எனில் (!m_finalp) {
                    m_finalp = புதிய AstFinal{m_modp->fileline(), finalp->unlinkFrBack()};
                    m_modp->addStmtsp(m_finalp);
                } இல்லையெனில் {
                    m_finalp->addStmtsp(finalp->unlinkFrBack());
                }
            }
            VL_DO_DANGLING2(தள்ளு_Deletep(pexprp), pexprp, propp);
            திரும்பு forkp;
        }

        திரும்பு assertCond(nodep, VN_AS(propp, முனை_கோவை), passsp, failsp);
    }

    Ast_முனை_Stmt* newFireAssertUnchecked(மாறிலி Ast_முனை_Stmt* nodep, மாறிலி string& message,
                                        Ast_முனை_கோவை* exprsp = nullptr) {
        // Like newFireAssert() but omits the asserts-on check
        AstDisplay* மாறிலி dispp
            = புதிய AstDisplay{nodep->fileline(), VDisplayType::DT_ERROR, message, nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        Ast_முனை_Stmt* மாறிலி bodysp = dispp;
        replaceDisplay(dispp, "%%Error");  // Convert to standard DISPLAY format
        எனில் (exprsp) dispp->fmtp()->exprsp()->addNext(exprsp);
        எனில் (v3Global.opt.stopFail()) bodysp->addNext(புதிய AstStop{nodep->fileline(), பொய்});
        திரும்பு bodysp;
    }

    Ast_முனை_Stmt* newFireAssert(மாறிலி Ast_முனை_Stmt* nodep, VAssertDirectiveType directiveType,
                               VAssertType assertType, மாறிலி string& message,
                               Ast_முனை_கோவை* exprsp = nullptr) {
        Ast_முனை_Stmt* bodysp = newFireAssertUnchecked(nodep, message, exprsp);
        bodysp = newIfAssertOn(bodysp, directiveType, assertType);
        திரும்பு bodysp;
    }

    AstVar* createDelayedVar(மாறிலி std::string& name, AstAlways* alwaysp, Ast_முனை_கோவை* exprp,
                             Ast_முனை_கோவை* initp) {
        File_வரி* மாறிலி flp = exprp->fileline();
        AstVar* மாறிலி varp = புதிய AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        // TODO: this lifetime seems nonsene (can't have NBAs to automatics), but is as before
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->addStmtsp(varp);
        // Before enough clock ticks, $past returns the expression's default sampled value.
        எனில் (initp) {
            m_modp->addStmtsp(புதிய AstInitialStatic{
                flp, புதிய AstAssign{flp, புதிய AstVarRef{flp, varp, VAccess::WRITE}, initp}});
        }
        ++m_statPastVars;
        // Actually set the delayed value
        Ast_முனை_கோவை* மாறிலி lhsp = புதிய AstVarRef{flp, varp, VAccess::WRITE};
        AstAssignDly* மாறிலி assignp = புதிய AstAssignDly{flp, lhsp, exprp};
        alwaysp->prependStmtsp(assignp);
        திரும்பு varp;
    }

    AstAlways* getDelayedAlways(Ast_முனை_கோவை* exprp, Ast_முனை_கோவை* initp, Ast_Sen_மரம்* senTreep) {
        auto& expr2Sen2Alwaysr
            = initp ? m_modExpr2Sen2DefaultDelayedAlwaysp : m_modExpr2Sen2DelayedAlwaysp;
        AstAlways*& alwayspr = expr2Sen2Alwaysr[*exprp][*senTreep];
        எனில் (!alwayspr) {
            File_வரி* மாறிலி flp = exprp->fileline();
            // Create the always block that computes the delayed values
            alwayspr = புதிய AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, nullptr};
            m_modp->addStmtsp(alwayspr);
            // Create the once-delayed variable
            மாறிலி std::string name = "_Vpast_" + cvtToStr(m_mod_Past_இலக்கம்++) + "_1";
            AstVar* மாறிலி varp = createDelayedVar(name, alwayspr, exprp, initp);
            // Add it to delayed variable vector
            m_delayed(alwayspr).emplace_back(varp);
        } இல்லையெனில் {
            // Reusing exiting, not needed
            VL_DO_DANGLING(தள்ளு_Deletep(exprp), exprp);
            எனில் (initp) VL_DO_DANGLING(தள்ளு_Deletep(initp), initp);
            VL_DO_DANGLING(தள்ளு_Deletep(senTreep), senTreep);
        }
        திரும்பு alwayspr;
    }

    Ast_முனை_கோவை* get_Past_மதிப்பு(Ast_முனை_கோவை* exprp, Ast_முனை_கோவை* initp, Ast_Sen_மரம்* senTreep,
                              uint32_t ticks) {
        UASSERT_OBJ(ticks > 0, exprp, "Delay must be > 0");
        File_வரி* மாறிலி flp = exprp->fileline();
        AstAlways* மாறிலி alwaysp
            = getDelayedAlways(exprp, initp ? initp->clone_மரம்_Pure(பொய்) : nullptr, senTreep);
        Ast_முனை_கோவை* மாறிலி resultp = past_மதிப்பு_Ref(alwaysp, flp, ticks, initp);
        எனில் (initp) VL_DO_DANGLING(தள்ளு_Deletep(initp), initp);
        திரும்பு resultp;
    }

    Ast_முனை_கோவை* past_மதிப்பு_Ref(AstAlways* alwaysp, File_வரி* flp, uint32_t ticks,
                              Ast_முனை_கோவை* initp) {
        std::vector<AstVar*>& delayedr = m_delayed(alwaysp);
        // Ensure the required delay exists
        போது (delayedr.size() < ticks) {
            AstVar* மாறிலி firstp = delayedr.front();
            File_வரி* மாறிலி varFlp = firstp->fileline();
            // Create once more delayed value
            std::string name = firstp->name();
            name.resize(name.size() - 1);
            name += std::to_string(delayedr.size() + 1);
            Ast_முனை_கோவை* மாறிலி prevp = புதிய AstVarRef{varFlp, delayedr.back(), VAccess::READ};
            AstVar* மாறிலி varp = createDelayedVar(name, alwaysp, prevp,
                                                  initp ? initp->clone_மரம்_Pure(பொய்) : nullptr);
            // Add it to delayed variable vector
            delayedr.emplace_back(varp);
        }
        // Return a reference to the appropriately delayed variable
        திரும்பு புதிய AstVarRef{flp, delayedr.at(ticks - 1), VAccess::READ};
    }

    // Time of the pipeline's last sampling tick, for end-of-simulation readers
    AstVar* getPastTickTimeVar(AstAlways* alwaysp) {
        AstVar*& varpr = m_delayedAlways2TickTimep[alwaysp];
        எனில் (!varpr) {
            File_வரி* மாறிலி flp = alwaysp->fileline();
            varpr = புதிய AstVar{flp, VVarType::MODULETEMP,
                               "_Vpast_" + cvtToStr(m_mod_Past_இலக்கம்++) + "_t",
                               m_modp->findUInt64DType()};
            m_modp->addStmtsp(varpr);
            Ast_முனை_கோவை* மாறிலி timep = புதிய AstCExpr{flp, "vlSymsp->_vm_contextp__->time()", 64};
            alwaysp->addStmtsp(
                புதிய AstAssign{flp, புதிய AstVarRef{flp, varpr, VAccess::WRITE}, timep});
        }
        திரும்பு varpr;
    }

    வெறுமை visitAssertionIterate(Ast_முனை_Cover_Or_Assert* nodep, Ast_முனை* failsp) {
        எனில் (m_beginp && nodep->name() == "") nodep->name(m_beginp->name());

        { AssertDeFutureVisitor{nodep->propp(), m_modp, m_mod_Past_இலக்கம்++}; }

        iterateAndNextNull(nodep->sentreep());
        எனில் (AstAssert* மாறிலி assertp = VN_CAST(nodep, Assert)) {
            iterateAndNextNull(assertp->failsp());
        } இல்லையெனில் எனில் (AstAssertIntrinsic* மாறிலி assertp = VN_CAST(nodep, AssertIntrinsic)) {
            iterateAndNextNull(assertp->failsp());
        } இல்லையெனில் எனில் (AstCover* மாறிலி coverp = VN_CAST(nodep, Cover)) {
            iterateAndNextNull(coverp->coverincsp());
        } இல்லையெனில் எனில் (!VN_IS(nodep, Restrict)) {
            nodep->v3fatalSrc("Unhandled assert type");
        }
        iterateAndNextNull(nodep->passsp());
        Ast_Sen_மரம்* sentreep = nodep->sentreep();
        எனில் (nodep->immediate()) {
            UASSERT_OBJ(!sentreep, nodep, "Immediate assertions don't have sensitivity");
        } இல்லையெனில் {
            UASSERT_OBJ(sentreep, nodep, "Concurrent assertions must have sensitivity");
            // Explicit inline clock differs from the enclosing always: hoist
            // and warn. To support this need queue of asserts to activate.
            எனில் (m_procedurep && !nodep->senFromAlways()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Procedural concurrent assertion with"
                              " clocking event inside always (IEEE 1800-2023 16.14.6)");
                static_cast<Ast_முனை*>(m_procedurep)->addNext(nodep->unlinkFrBack());
                திரும்பு;  // Later iterate will pick up
            }
            sentreep->unlinkFrBack();
            எனில் (m_procedurep) { VL_DO_DANGLING(தள்ளு_Deletep(sentreep), sentreep); }
        }
        //
        மாறிலி string& message = nodep->name();
        Ast_முனை* passsp = nodep->passsp();
        எனில் (passsp) passsp->unlinkFrBackWithNext();
        எனில் (AstAssert* மாறிலி assertp = VN_CAST(nodep, Assert)) {
            failsp = assertp->failsp();
        } இல்லையெனில் எனில் (AstAssertIntrinsic* மாறிலி assertp = VN_CAST(nodep, AssertIntrinsic)) {
            failsp = assertp->failsp();
        }
        எனில் (failsp) failsp->unlinkFrBackWithNext();

        தருக்க selfDestruct = பொய்;
        தருக்க passspGated = பொய்;
        மாறிலி AstCover* மாறிலி coverp = VN_CAST(nodep, Cover);
        // A sequence event control is not an assertion directive; no assertion control
        மாறிலி தருக்க seqEvent = coverp && coverp->isSeqEvent();
        எனில் (coverp) {
            ++m_statCover;
            எனில் (seqEvent) {
                // Keep the event-fire action, with no coverage bucket
            } இல்லையெனில் எனில் (!v3Global.opt.coverageUser()) {
                selfDestruct = உண்மை;
            } இல்லையெனில் {
                // V3Coverage assigned us a bucket to increment.
                AstCoverInc* மாறிலி covincp = VN_AS(coverp->coverincsp(), CoverInc);
                UASSERT_OBJ(covincp, coverp, "Missing AstCoverInc under assertion");
                covincp->unlinkFrBackWithNext();  // next() might have  AstAssign for trace
                எனில் (message != "") covincp->declp()->comment(message);
                எனில் (passsp) {
                    passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                               /*vacuous=*/பொய்);
                    passspGated = உண்மை;
                    passsp = Ast_முனை::addNext<Ast_முனை, Ast_முனை>(covincp, passsp);
                } இல்லையெனில் {
                    passsp = covincp;
                }
            }
        } இல்லையெனில் எனில் (VN_IS(nodep, Assert) || VN_IS(nodep, AssertIntrinsic)) {
            எனில் (nodep->immediate()) {
                ++m_statAsImm;
            } இல்லையெனில் {
                ++m_statAsNotImm;
            }
            எனில் (!passsp && !failsp)
                failsp = newFireAssertUnchecked(
                    nodep, VN_IS(nodep, AssertIntrinsic) ? "'$cast' failed." : "'assert' failed.");
        } இல்லையெனில் {
            nodep->v3fatalSrc("Unknown node type");
        }

        VL_RESTORER(m_passsp);
        VL_RESTORER(m_failsp);
        VL_RESTORER(m_assertp);
        m_passsp = passsp;
        m_failsp = failsp;
        m_assertp = nodep;
        iterate(nodep->propp());

        Ast_முனை* propExprp;
        Ast_முனை_கோவை* disablep = nullptr;
        Ast_முனை_கோவை* matchCountp = nullptr;
        எனில் (AstPropSpec* மாறிலி specp = VN_CAST(nodep->propp(), PropSpec)) {
            propExprp = specp->propp()->unlinkFrBack();
            எனில் (specp->disablep()) disablep = specp->disablep()->unlinkFrBack();
            matchCountp = specp->matchCountp();
        } இல்லையெனில் {
            propExprp = nodep->propp()->unlinkFrBack();
        }
        File_வரி* மாறிலி flp = nodep->fileline();
        தருக்க passspAlreadyGated = பொய்;
        எனில் (passsp && VN_IS(passsp, If)) passspAlreadyGated = VN_AS(passsp, If)->user1();
        எனில் (passsp && !passspGated && !passspAlreadyGated && !VN_IS(propExprp, PExpr)
            && !seqEvent) {
            passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                       /*vacuous=*/பொய்);
        }
        எனில் (failsp && !VN_IS(propExprp, PExpr)) {
            failsp = newIfAssertFailOn(failsp, nodep->directive(), nodep->userType());
        }
        எனில் (coverp && matchCountp && passsp) {
            // Convert the match count into a loop that decrements a temporary variable until it
            // reaches zero.
            matchCountp->unlinkFrBack();
            AstVar* மாறிலி remainingp = புதிய AstVar{
                flp, VVarType::MODULETEMP, m_match_மொத்தம்_Names.get(""), matchCountp->dtypep()};
            remainingp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remainingp);
            Ast_முனை* மாறிலி replaysp
                = புதிய AstAssign{flp, புதிய AstVarRef{flp, remainingp, VAccess::WRITE}, matchCountp};
            AstLoop* மாறிலி loopp = புதிய AstLoop{flp};
            loopp->addStmtsp(
                புதிய AstLoopTest{flp, loopp, புதிய AstVarRef{flp, remainingp, VAccess::READ}});
            loopp->addStmtsp(passsp);
            loopp->addStmtsp(
                புதிய AstAssign{flp, புதிய AstVarRef{flp, remainingp, VAccess::WRITE},
                              புதிய AstSub{flp, புதிய AstVarRef{flp, remainingp, VAccess::READ},
                                         புதிய AstConst{flp, AstConst::Widthed_மதிப்பு{},
                                                      remainingp->dtypep()->width(), 1}}});
            replaysp->addNext(loopp);
            passsp = replaysp;
        }
        Ast_முனை* bodysp = assertBody(nodep, propExprp, passsp, failsp);
        எனில் (disablep) bodysp = புதிய AstIf{flp, புதிய AstLogNot{flp, disablep}, bodysp};
        // Add assertOn check last, for better combining
        எனில் (!seqEvent) bodysp = newIfAssertOn(bodysp, nodep->directive(), nodep->userType());
        எனில் (sentreep) bodysp = புதிய AstAlways{flp, VAlwaysKwd::ALWAYS, sentreep, bodysp};

        எனில் (passsp && !passsp->backp()) VL_DO_DANGLING(தள்ளு_Deletep(passsp), passsp);
        எனில் (failsp && !failsp->backp()) VL_DO_DANGLING(தள்ளு_Deletep(failsp), failsp);

        // Install it
        எனில் (selfDestruct) {
            // Delete it after making the tree.  This way we can tell the user
            // if it wasn't constructed nicely or has other errors without needing --coverage.
            VL_DO_DANGLING(bodysp->delete_மரம்(), bodysp);
            nodep->unlinkFrBack();
        } இல்லையெனில் {
            nodep->replaceWith(bodysp);
        }
        // Bye
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    தருக்க isEmptyStmt(Ast_முனை* nodep) {
        எனில் (!nodep) திரும்பு உண்மை;
        எனில் (AstBegin* மாறிலி beginp = VN_CAST(nodep, Begin)) {
            எனில் (beginp->declsp()) திரும்பு பொய்;
            க்கு (Ast_முனை* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                எனில் (!isEmptyStmt(stmtp)) திரும்பு பொய்;
            }
            திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }

    // VISITORS
    வெறுமை visit(AstIf* nodep) override {
        எனில் (!nodep->user1SetOnce()) {
            எனில் (nodep->uniquePragma() || nodep->unique0Pragma()) {
                மாறிலி Ast_முனை_If* ifp = nodep;
                Ast_முனை_கோவை* propp = nullptr;
                தருக்க hasDefaultElse = பொய்;
                செய் {
                    // If this statement ends with 'else if', then nextIf will point to the
                    // nextIf statement.  Otherwise it will be null.
                    மாறிலி Ast_முனை_If* மாறிலி nextifp = dynamic_cast<Ast_முனை_If*>(ifp->elsesp());
                    iterateAndNextNull(ifp->condp());

                    // Recurse into the true case.
                    iterateAndNextNull(ifp->thensp());

                    // If the last else is not an else if, recurse into that too.
                    எனில் (ifp->elsesp() && !nextifp) {  //
                        iterateAndNextNull(ifp->elsesp());
                    }

                    // Build a bitmask of the true predicates
                    Ast_முனை_கோவை* மாறிலி predp = ifp->condp()->clone_மரம்_Pure(பொய்);
                    எனில் (propp) {
                        propp = புதிய AstConcat{nodep->fileline(), predp, propp};
                    } இல்லையெனில் {
                        propp = predp;
                    }

                    // Record if this ends with an 'else' that does not have an if
                    எனில் (ifp->elsesp() && !nextifp) hasDefaultElse = உண்மை;

                    ifp = nextifp;
                } போது (ifp);

                மாறிலி தருக்க allow_none = nodep->unique0Pragma();

                // Empty case means no property
                எனில் (!propp) propp = புதிய AstConst{nodep->fileline(), AstConst::BitFalse{}};

                // Note: if this ends with an 'else', then we don't need to validate that one of
                // the predicates evaluates to true.
                Ast_முனை_கோவை* மாறிலி ohot
                    = ((allow_none || hasDefaultElse)
                           ? static_cast<Ast_முனை_கோவை*>(புதிய AstOneHot0{nodep->fileline(), propp})
                           : static_cast<Ast_முனை_கோவை*>(புதிய AstOneHot{nodep->fileline(), propp}));
                மாறிலி VAssertType assertType
                    = nodep->uniquePragma() ? VAssertType::UNIQUE : VAssertType::UNIQUE0;
                // Move, not clone, as the branches might declare referenced variables
                VNRelinker relinker;
                nodep->unlinkFrBack(&relinker);
                AstIf* மாறிலி checkifp
                    = புதிய AstIf{nodep->fileline(), புதிய AstLogNot{nodep->fileline(), ohot},
                                newFireAssert(nodep, VAssertDirectiveType::VIOLATION_IF,
                                              assertType, "'unique if' statement violated"),
                                nodep};
                checkifp->isBoundsCheck(உண்மை);  // To avoid LATCH warning
                checkifp->branchPred(VBranchPred::BP_UNLIKELY);
                relinker.relink(checkifp);
                திரும்பு;
            }
        }

        iterateChildren(nodep);

        எனில் (nodep->user2()) {
            // Combine consecutive assertOn checks if possible
            எனில் (AstIf* மாறிலி backp = VN_CAST(nodep->backp(), If)) {
                எனில் (backp->nextp() == nodep  //
                    && backp->user2()  //
                    && backp->condp()->same_மரம்(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    backp->addThensp(nodep->thensp()->unlinkFrBackWithNext());
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    திரும்பு;
                }
            }
            // Combine nested assertOn checks if possible
            எனில் (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* மாறிலி checkp = VN_CAST(nodep->thensp(), If);
                எனில் (checkp  //
                    && checkp->user2()  //
                    && checkp->condp()->same_மரம்(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    VL_DO_DANGLING(தள்ளு_Deletep(checkp->unlinkFrBack()), checkp);
                    திரும்பு;
                }
            }
            திரும்பு;
        }

        // Swap assertOn check with single statement 'if' statement to bubble up for combining
        // Note we can't just swap the conditions as they two Ifs have different flags,
        // so swapping the Ifs themeselves then swapping back the bodies.
        எனில் (nodep->condp()->isPure()) {
            எனில் (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* மாறிலி checkp = VN_CAST(nodep->thensp(), If);
                எனில் (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    திரும்பு;
                }
            }
            எனில் (nodep->elsesp() && !nodep->elsesp()->nextp() && isEmptyStmt(nodep->thensp())) {
                AstIf* மாறிலி checkp = VN_CAST(nodep->elsesp(), If);
                எனில் (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addElsesp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    திரும்பு;
                }
            }
        }
    }

    //========== Case assertions
    வெறுமை visit(AstCase* nodep) override {
        // Introduce temporary variable for AstCase if needed - it is done here and not in V3Case
        // because this phase is before V3Scope and V3Case is not. Doing it before V3Scope ensures
        // that V3Scope will take care of a scope creation
        // We also need to do it before V3Begin, co that pragmas like `unique0` also work correctly
        எனில் (!nodep->exprp()->isPure()) {
            ++m_statLiftedCaseExprs;
            File_வரி* மாறிலி fl = nodep->exprp()->fileline();
            AstVar* மாறிலி varp = புதிய AstVar{fl, VVarType::BLOCKTEMP, m_caseTempNames.get(nodep),
                                            nodep->exprp()->dtypep()};
            Ast_முனை_கோவை* மாறிலி origp = nodep->exprp()->unlinkFrBack();
            nodep->addHereThisAsNext(
                புதிய AstAssign{fl, புதிய AstVarRef{fl, varp, VAccess::WRITE}, origp});
            nodep->exprp(புதிய AstVarRef{fl, varp, VAccess::READ});
            எனில் (m_ftaskp) {
                varp->funcLocal(உண்மை);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } இல்லையெனில் {
                m_modp->prependStmtsp(varp);
            }
            VIsCached::clear_Cache_மரம்();
        }
        iterateChildren(nodep);
        எனில் (!nodep->user1SetOnce()) {
            தருக்க has_default = பொய்;
            க்கு (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                எனில் (itemp->isDefault()) has_default = உண்மை;
            }
            மாறிலி Ast_முனை_DType* கோவை_Dtypep = nodep->exprp()->dtypep()->skipRefp();

            VAssertType assertType = VAssertType::INTERNAL;
            எனில் (nodep->priorityPragma()) {
                assertType = VAssertType::PRIORITY;
            } இல்லையெனில் எனில் (nodep->uniquePragma()) {
                assertType = VAssertType::UNIQUE;
            } இல்லையெனில் எனில் (nodep->unique0Pragma()) {
                assertType = VAssertType::UNIQUE0;
            }

            string valFmt;
            எனில் (கோவை_Dtypep->isIntegralOrPacked())
                valFmt = " for '" + cvtToStr(கோவை_Dtypep->widthMin()) + "'h%X'";
            எனில் (nodep->fullPragma() || nodep->priorityPragma()) {
                // Need to add a default if there isn't one already
                ++m_statAsFull;
                எனில் (!has_default) {
                    nodep->addItemsp(புதிய AstCaseItem{
                        nodep->fileline(), nullptr /*DEFAULT*/,
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      nodep->pragmaString() + ", but non-match found" + valFmt,
                                      valFmt.empty() ? nullptr
                                                     : nodep->exprp()->clone_மரம்_Pure(பொய்))});
                }
            }
            எனில் (nodep->parallelPragma() || nodep->uniquePragma() || nodep->unique0Pragma()) {
                // Need to check that one, and only one of the case items match at any moment
                // If there's a default, we allow none to match, else exactly one must match
                ++m_statAsFull;
                எனில் (!has_default && !nodep->itemsp()) {
                    // Not parallel, but harmlessly so.
                } இல்லையெனில் {
                    Ast_முனை_கோவை* propp = nullptr;
                    க்கு (AstCaseItem* itemp = nodep->itemsp(); itemp;
                         itemp = VN_AS(itemp->nextp(), CaseItem)) {
                        Ast_முனை_கோவை* itembitp = nullptr;
                        க்கு (Ast_முனை_கோவை* icondp = itemp->condsp(); icondp;
                             icondp = VN_AS(icondp->nextp(), முனை_கோவை)) {
                            Ast_முனை_கோவை* onep;
                            எனில் (AstInsideRange* மாறிலி rcondp = VN_CAST(icondp, InsideRange)) {
                                onep = rcondp->newAndFromInside(
                                    nodep->exprp()->clone_மரம்_Pure(உண்மை),
                                    rcondp->lhsp()->clone_மரம்_Pure(உண்மை),
                                    rcondp->rhsp()->clone_மரம்_Pure(உண்மை));
                            } இல்லையெனில் எனில் (nodep->casex() || nodep->casez() || nodep->caseInside()) {
                                onep = AstEqWild::newTyped(itemp->fileline(),
                                                           nodep->exprp()->clone_மரம்_Pure(பொய்),
                                                           icondp->clone_மரம்_Pure(பொய்));
                            } இல்லையெனில் {
                                onep = AstEq::newTyped(icondp->fileline(),
                                                       nodep->exprp()->clone_மரம்_Pure(பொய்),
                                                       icondp->clone_மரம்_Pure(பொய்));
                            }
                            // OR together all conditions within the same case item
                            எனில் (onep) {
                                எனில் (itembitp) {
                                    itembitp = புதிய AstOr{icondp->fileline(), onep, itembitp};
                                } இல்லையெனில் {
                                    itembitp = onep;
                                }
                            }
                        }
                        எனில் (itembitp) {
                            எனில் (propp) {
                                propp = புதிய AstConcat{itemp->fileline(), itembitp, propp};
                            } இல்லையெனில் {
                                propp = itembitp;
                            }
                        }
                    }
                    // Empty case means no property
                    எனில் (!propp) propp = புதிய AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    மாறிலி தருக்க allow_none = has_default || nodep->unique0Pragma();
                    // The following assertion looks as below.
                    // if (!$onehot(propp)) begin
                    //     if (propp == '0) begin if (!allow_none) $error("none match"); end
                    //     else $error("multiple match");
                    // end
                    Ast_முனை_கோவை* மாறிலி ohot = புதிய AstOneHot{nodep->fileline(), propp};
                    AstConst* மாறிலி zero = புதிய AstConst{
                        nodep->fileline(), AstConst::Widthed_மதிப்பு{}, propp->width(), 0};
                    AstIf* மாறிலி ohotIfp
                        = புதிய AstIf{nodep->fileline(), புதிய AstLogNot{nodep->fileline(), ohot}};
                    AstIf* மாறிலி zeroIfp = புதிய AstIf{
                        nodep->fileline(),
                        புதிய AstEq{nodep->fileline(), propp->clone_மரம்_Pure(பொய்), zero}};
                    Ast_முனை_கோவை* மாறிலி exprp = nodep->exprp();
                    மாறிலி string pragmaStr = nodep->pragmaString();
                    எனில் (!allow_none)
                        zeroIfp->addThensp(
                            newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                          pragmaStr + ", but none matched" + valFmt,
                                          valFmt.empty() ? nullptr : exprp->clone_மரம்_Pure(பொய்)));
                    zeroIfp->addElsesp(
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      pragmaStr + ", but multiple matches found" + valFmt,
                                      valFmt.empty() ? nullptr : exprp->clone_மரம்_Pure(பொய்)));
                    ohotIfp->addThensp(zeroIfp);
                    ohotIfp->isBoundsCheck(உண்மை);  // To avoid LATCH warning
                    ohotIfp->branchPred(VBranchPred::BP_UNLIKELY);
                    nodep->addNotParallelp(ohotIfp);
                }
            }
        }
    }

    வெறுமை visit(AstFuture* nodep) override {
        nodep->v3error("Future sampled value function called outside property or sequence "
                       "expression (IEEE 16.9.4)");
        nodep->replaceWith(புதிய AstConst{nodep->fileline(), 0});
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    //========== Past
    வெறுமை visit(AstPast* nodep) override {
        எனில் (!nodep->propertyTiming()) {
            nodep->initp(nodep->exprp()->clone_மரம்_Pure(பொய்));
            { Default_Sampled_மதிப்பு_Visitor{nodep->initp()}; }
        }
        Ast_முனை_கோவை* மாறிலி initp = nodep->initp() ? nodep->initp()->unlinkFrBack() : nullptr;
        iterateChildren(nodep);
        uint32_t ticks = 1;
        எனில் (nodep->ticksp()) {
            UASSERT_OBJ(VN_IS(nodep->ticksp(), Const), nodep,
                        "Expected constant ticks, checked in V3Width");
            ticks = VN_AS(nodep->ticksp(), Const)->toUInt();
        }
        UASSERT_OBJ(ticks >= 1, nodep, "0 tick should have been checked in V3Width");
        Ast_முனை_கோவை* மாறிலி exprp = new_Sampled_கோவை(nodep->exprp()->unlinkFrBack());
        Ast_Sen_மரம்* மாறிலி senTreep = nodep->sentreep()->unlinkFrBack();
        Ast_முனை_கோவை* inp = nullptr;
        // Deliberately emitted for every ftask $past; calls outside final take the normal stage
        எனில் (VN_IS(m_procedurep, Final) || m_ftaskp) {
            // A sampling-tick finish reads one stage deeper (IEEE 1800-2023 16.9.3).
            File_வரி* மாறிலி flp = nodep->fileline();
            AstAlways* மாறிலி alwaysp
                = getDelayedAlways(exprp, initp ? initp->clone_மரம்_Pure(பொய்) : nullptr, senTreep);
            Ast_முனை_கோவை* மாறிலி betweenTicksp = past_மதிப்பு_Ref(alwaysp, flp, ticks, initp);
            Ast_முனை_கோவை* மாறிலி onTickp = past_மதிப்பு_Ref(alwaysp, flp, ticks + 1, initp);
            எனில் (initp) VL_DO_DANGLING(தள்ளு_Deletep(initp), initp);
            Ast_முனை_கோவை* மாறிலி onTickCondp = புதிய AstLogAnd{
                flp, புதிய AstCExpr{flp, "vlSymsp->_vm_contextp__->executingFinal()", 1},
                புதிய AstEq{flp, புதிய AstVarRef{flp, getPastTickTimeVar(alwaysp), VAccess::READ},
                          புதிய AstCExpr{flp, "vlSymsp->_vm_contextp__->finishPendingTime()", 64}}};
            AstCond* மாறிலி condp = புதிய AstCond{flp, onTickCondp, onTickp, betweenTicksp};
            condp->dtypeFrom(onTickp);
            inp = condp;
        } இல்லையெனில் {
            inp = get_Past_மதிப்பு(exprp, initp, senTreep, ticks);
        }
        nodep->replaceWith(inp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    //========== Move $sampled down to read-only variables
    வெறுமை visit(AstSampled* nodep) override {
        எனில் (nodep->user1()) திரும்பு;
        VL_RESTORER(m_inSampled);
        {
            m_inSampled = உண்மை;
            iterateChildren(nodep);
        }
        எனில் (nodep->exprp()) {
            nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        } இல்லையெனில் {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(Ast_முனை_Var_Ref* nodep) override {
        எனில் (m_inSampled && !nodep->user2() && !nodep->varp()->isTemp()) {
            எனில் (!nodep->access().isReadOnly()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Write to variable in sampled expression");
            } இல்லையெனில் {
                VNRelinker relinkHandle;
                nodep->unlinkFrBack(&relinkHandle);
                AstSampled* மாறிலி newp = new_Sampled_கோவை(nodep);
                relinkHandle.relink(newp);
                newp->user1(1);
                v3Global.setHasSampled();
            }
        }
    }
    // Don't sample sensitivities
    வெறுமை visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSampled);
        m_inSampled = பொய்;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstPExprClause* nodep) override {
        Ast_முனை* stmtsp = nullptr;
        எனில் (nodep->pass() && m_passsp) {
            // Cover adds COVERINC by AstNode::addNext, thus need to clone next too.
            stmtsp = m_passsp->clone_மரம்(உண்மை);
            stmtsp = newIfAssertPassOn(stmtsp, m_assertp->directive(), m_assertp->userType(),
                                       nodep->vacuous());
        } இல்லையெனில் எனில் (!nodep->pass() && m_failsp) {
            stmtsp = m_failsp->clone_மரம்(உண்மை);
            stmtsp = newIfAssertFailOn(stmtsp, m_assertp->directive(), m_assertp->userType());
        }
        எனில் (stmtsp) {
            stmtsp->foreachAndNext([](Ast_முனை_Var_Ref* மாறிலி refp) {
                // References inside action blocks shouldn't be implicitly sampled
                // m_passsp/m_failsp have been already visited once and refs explicitly sampled
                // are handled already
                refp->user2(1);
            });
            nodep->replaceWith(stmtsp);
        } இல்லையெனில் {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstPExpr* nodep) override {
        எனில் (m_inRestrict) {
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
        } இல்லையெனில் {
            iterateChildren(nodep);
        }
    }

    //========== Statements
    வெறுமை visit(AstDisplay* nodep) override {
        iterateChildren(nodep);
        // Replace the special types with standard text
        எனில் (nodep->displayType() == VDisplayType::DT_INFO) {
            replaceDisplay(nodep, "-Info");
        } இல்லையெனில் எனில் (nodep->displayType() == VDisplayType::DT_WARNING) {
            replaceDisplay(nodep, "%%Warning");
        } இல்லையெனில் எனில் (nodep->displayType() == VDisplayType::DT_ERROR) {
            replaceDisplay(nodep, "%%Error");
        } இல்லையெனில் எனில் (nodep->displayType() == VDisplayType::DT_FATAL) {
            replaceDisplay(nodep, "%%Fatal");
        } இல்லையெனில் எனில் (nodep->displayType() == VDisplayType::DT_MONITOR) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            File_வரி* மாறிலி fl = nodep->fileline();

            AstSenItem* monSenItemsp = nullptr;
            எனில் (Ast_முனை* மாறிலி monExprsp = nodep->fmtp()->exprsp()) {
                monExprsp->foreachAndNext([&](AstVarRef* varrefp) {
                    AstSenItem* மாறிலி senItemp
                        = புதிய AstSenItem{fl, VEdgeType::ET_CHANGED,
                                         // Clone so get VarRef or VarXRef as needed
                                         varrefp->clone_மரம்(பொய்)};
                    monSenItemsp = Ast_முனை::addNextNull(monSenItemsp, senItemp);
                });
            }

            Ast_Sen_மரம்* மாறிலி mon_Sen_மரம் = புதிய Ast_Sen_மரம்{fl, monSenItemsp};
            மாறிலி auto mon_இலக்கம் = ++m_monitor_இலக்கம்;
            // Where $monitor was we do "__VmonitorNum = N;"
            AstAssign* மாறிலி newsetp = புதிய AstAssign{
                fl, new_Monitor_இலக்கம்_Var_Refp(nodep, VAccess::WRITE), புதிய AstConst{fl, mon_இலக்கம்}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__VmonitorOn && __VmonitorNum==N) $display(...);"
            Ast_முனை* மாறிலி stmtsp = nodep;
            AstIf* மாறிலி ifp = புதிய AstIf{
                fl,
                புதிய AstLogAnd{fl, புதிய AstLogNot{fl, newMonitorOffVarRefp(nodep, VAccess::READ)},
                              புதிய AstEq{fl, புதிய AstConst{fl, mon_இலக்கம்},
                                        new_Monitor_இலக்கம்_Var_Refp(nodep, VAccess::READ)}},
                stmtsp};
            ifp->isBoundsCheck(உண்மை);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_முனை* மாறிலி newp = புதிய AstAlways{fl, VAlwaysKwd::ALWAYS, mon_Sen_மரம், ifp};
            m_modp->addStmtsp(newp);
        } இல்லையெனில் எனில் (nodep->displayType() == VDisplayType::DT_STROBE) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            // Need one-shot
            File_வரி* மாறிலி fl = nodep->fileline();
            AstVar* மாறிலி varp
                = புதிய AstVar{fl, VVarType::MODULETEMP, "__Vstrobe" + cvtToStr(m_mod_Strobe_இலக்கம்++),
                             nodep->findBitDType()};
            m_modp->addStmtsp(varp);
            // Where $strobe was we do "__Vstrobe = '1;"
            AstAssign* மாறிலி newsetp = புதிய AstAssign{fl, புதிய AstVarRef{fl, varp, VAccess::WRITE},
                                                     புதிய AstConst{fl, AstConst::BitTrue{}}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__Vstrobe) begin $display(...); __Vstrobe = '0; end"
            Ast_முனை* மாறிலி stmtsp = nodep;
            AstIf* மாறிலி ifp = புதிய AstIf{fl, புதிய AstVarRef{fl, varp, VAccess::READ}, stmtsp};
            ifp->isBoundsCheck(உண்மை);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_முனை* மாறிலி newp = புதிய AstAlwaysPostponed{fl, ifp};
            stmtsp->addNext(புதிய AstAssign{fl, புதிய AstVarRef{fl, varp, VAccess::WRITE},
                                          புதிய AstConst{fl, AstConst::BitFalse{}}});
            m_modp->addStmtsp(newp);
        }
    }
    வெறுமை visit(AstMonitorOff* nodep) override {
        AstAssign* மாறிலி newp
            = புதிய AstAssign{nodep->fileline(), newMonitorOffVarRefp(nodep, VAccess::WRITE),
                            புதிய AstConst{nodep->fileline(), AstConst::BitTrue{}, nodep->off()}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstAssert* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    வெறுமை visit(AstAssertCtl* nodep) override {
        iterateChildren(nodep);

        தருக்க assertTypeConst = உண்மை;
        எனில் (!nodep->assertTypesp()) {
            nodep->assertTypes(VAssertType{ALL_ASSERT_TYPES});
        } இல்லையெனில் எனில் (மாறிலி AstConst* மாறிலி assertTypesp = VN_CAST(nodep->assertTypesp(), Const)) {
            nodep->assertTypes(VAssertType{assertTypesp->toSInt()});
        } இல்லையெனில் {
            assertTypeConst = பொய்;
        }

        தருக்க controlTypeConst = பொய்;
        எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep->controlTypep(), Const)) {
            nodep->ctlType(constp->toSInt());
            controlTypeConst = உண்மை;
        }
        எனில் (controlTypeConst
            && (nodep->ctlType() < VAssertCtlType::LOCK
                || nodep->ctlType() > VAssertCtlType::VACUOUS_OFF)) {
            nodep->unlinkFrBack();
            nodep->v3error("Bad $assertcontrol control_type '"
                           << cvtToStr(static_cast<முழு>(nodep->ctlType()))
                           << "' (IEEE 1800-2023 Table 20-5)");
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        }
        எனில் (assertTypeConst && nodep->assertTypes() != ALL_ASSERT_TYPES
            && nodep->assertTypes().containsAny(VAssertType::UNIQUE | VAssertType::UNIQUE0
                                                | VAssertType::PRIORITY)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: assert control assertion_type");
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            திரும்பு;
        }

        தருக்க directiveTypeConst = உண்மை;
        எனில் (!nodep->directiveTypesp()) {
            nodep->directiveTypes(VAssertDirectiveType::ASSERT | VAssertDirectiveType::ASSUME
                                  | VAssertDirectiveType::COVER);
        } இல்லையெனில் எனில் (மாறிலி AstConst* மாறிலி directiveTypesp
                   = VN_CAST(nodep->directiveTypesp(), Const)) {
            nodep->directiveTypes(VAssertDirectiveType{directiveTypesp->toSInt()});
        } இல்லையெனில் {
            directiveTypeConst = பொய்;
        }
        எனில் (!directiveTypeConst) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: non-const assert directive type expression");
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            திரும்பு;
        }

        File_வரி* மாறிலி fl = nodep->fileline();
        UINFO(9, "Generating assertctl for a module: " << m_modp);
        AstCStmt* மாறிலி newp = புதிய AstCStmt{fl};
        newp->add("vlSymsp->_vm_contextp__->assertCtl(");
        newp->add(nodep->controlTypep()->unlinkFrBack());
        newp->add(", ");
        எனில் (nodep->assertTypesp()) {
            newp->add(nodep->assertTypesp()->unlinkFrBack());
        } இல்லையெனில் {
            newp->add(std::to_string(ALL_ASSERT_TYPES));
        }
        newp->add(", " + std::to_string(nodep->directiveTypes()) + ");\n");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstAssertIntrinsic* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    வெறுமை visit(AstCover* nodep) override {  //
        visitAssertionIterate(nodep, nullptr);
    }
    வெறுமை visit(AstRestrict* nodep) override {
        VL_RESTORER(m_inRestrict);
        m_inRestrict = உண்மை;
        iterateChildren(nodep);
        // IEEE says simulator ignores these
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }

    வெறுமை visit(Ast_முனை_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Past_இலக்கம்);
        VL_RESTORER(m_mod_Strobe_இலக்கம்);
        VL_RESTORER(m_finalp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DelayedAlwaysp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DefaultDelayedAlwaysp);
        VL_RESTORER_CLEAR(m_delayedAlways2TickTimep);
        m_modp = nodep;
        m_mod_Past_இலக்கம் = 0;
        m_mod_Strobe_இலக்கம் = 0;
        m_finalp = nullptr;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstGenBlock* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstBegin* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
        // If the body is a single assertOn check, bubble it up for combining
        எனில் (!nodep->declsp()) {
            எனில் (AstIf* மாறிலி ifp = VN_CAST(nodep->stmtsp(), If)) {
                எனில் (ifp->user2() && !ifp->nextp()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(ifp->unlinkFrBack());
                    nodep->addStmtsp(ifp->thensp()->unlinkFrBackWithNext());
                    ifp->addThensp(nodep);
                }
            }
        }
    }

    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
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
        VIsCached::clear_Cache_மரம்();
    }
};

//######################################################################
// Top Assert class

வெறுமை V3Assert::assertAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("assert", 0, dump_மரம்_Either_Level() >= 3);
}
