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
    AstNode* m_scopep = nullptr;

    // VISITORS
    فراغ visit(AstNodeModule* nodep) override {
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
        𐡄𐡍 (ثابت AstNodeModule* ثابت modp = VN_CAST(m_scopep, NodeModule)) {
            defaultp = modp->defaultDisablep();
        } وإلا {
            defaultp = VN_AS(m_scopep, GenBlock)->defaultDisablep();
        }
        𐡄𐡍 (VL_UNLIKELY(defaultp)) {
            nodep->v3error("Only one 'default disable iff' allowed per "
                           << (VN_IS(m_scopep, NodeModule) ? "module" : "generate block")
                           << " (IEEE 1800-2023 16.15)");
        } وإلا 𐡄𐡍 (AstNodeModule* ثابت modp = VN_CAST(m_scopep, NodeModule)) {
            modp->defaultDisablep(nodep);
        } وإلا {
            VN_AS(m_scopep, GenBlock)->defaultDisablep(nodep);
        }
    }
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
    explicit DefaultDisableLocalVisitor(AstNetlist* nodep) { iterate(nodep); }
};

صنف DefaultDisablePropagateVisitor final : عام VNVisitor {
    // STATE
    AstDefaultDisable* m_defaultDisablep = nullptr;

    // VISITORS
    فراغ visit(AstNodeModule* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    فراغ visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        𐡄𐡍 (!nodep->defaultDisablep()) nodep->defaultDisablep(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
    explicit DefaultDisablePropagateVisitor(AstNetlist* nodep) { iterate(nodep); }
};

// Lower a sequence used as an event control ('@seq', IEEE 1800-2023 9.4.2.4) into a
// synthesized event fired by an internal 'cover sequence' on each end-of-match
صنف SeqEventLowerVisitor final : عام VNVisitor {
    // STATE
    AstNodeModule* m_modp = nullptr;  // Current module
    V3UniqueNames m_names{"__VseqEvent"};  // Synthesized event names

    // VISITORS
    فراغ visit(AstNodeModule* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstSenItem* nodep) override {
        AstFuncRef* ثابت funcrefp = VN_CAST(nodep->sensp(), FuncRef);
        𐡄𐡍 (funcrefp && VN_IS(funcrefp->taskp(), Sequence)) {
            FileLine* ثابت flp = nodep->fileline();
            AstVar* ثابت eventp = جديد AstVar{flp, VVarType::MODULETEMP, m_names.get(nodep),
                                              m_modp->findBasicDType(VBasicDTypeKwd::EVENT)};
            eventp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(eventp);
            v3Global.setHasEvents();
            funcrefp->unlinkFrBack();
            nodep->sensp(جديد AstVarRef{flp, eventp, VAccess::READ});
            ثابت منطقي automaticActual = funcrefp->exists([](ثابت AstNodeVarRef* refp) {
                أرجع refp->varp() && refp->varp()->lifetime().isAutomatic();
            });
            𐡄𐡍 (automaticActual) {
                nodep->v3error("Arguments to a sequence used as an event control must be"
                               " static (IEEE 1800-2023 9.4.2.4)");
                VN_AS(funcrefp->taskp(), Sequence)->isReferenced(خطأ);
                VL_DO_DANGLING(pushDeletep(funcrefp), funcrefp);
                أرجع;
            }
            AstFireEvent* ثابت firep
                = جديد AstFireEvent{flp, جديد AstVarRef{flp, eventp, VAccess::WRITE}, خطأ};
            AstCover* ثابت coverp
                = جديد AstCover{flp, جديد AstPropSpec{flp, nullptr, nullptr, funcrefp}, firep,
                               VAssertType::CONCURRENT};
            coverp->isCoverSeq(𐡒𐡔𐡈);
            coverp->isSeqEvent(𐡒𐡔𐡈);
            m_modp->addStmtsp(coverp);
            أرجع;
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

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
    V3Global::dumpCheckGlobalTree("assertseqevent", 0, dumpTreeEitherLevel() >= 3);
}

string V3AssertCommon::assertCtlGetCall(ثابت حرف* query, VAssertType type,
                                        VAssertDirectiveType directiveType) {
    // Template arguments let the runtime build the type/directive mask at C++ compile time
    أرجع "vlSymsp->_vm_contextp__->assertCtlGet<"s + std::to_string(type) + ", "s
           + std::to_string(directiveType) + ">(VerilatedAssertCtlQuery::"s + query + ")"s;
}

منطقي V3AssertCommon::checkSvaValueLimit(AstNode* nodep, ثابت AstConst* constp, ثابت حرف* what) {
    // Temporal values can materialize as O(N) AST or state in later passes.
    // Leave headroom for a range ring's extra slot and V3Number's signed
    // `(bits + 31) / 32` word count.
    ساكن constexpr unsigned SVA_VALUE_HARD_LIMIT
        = static_cast<unsigned>(std::numeric_limits<صحيح>::max()) - 32U;
    ثابت unsigned configuredLimit = static_cast<unsigned>(v3Global.opt.maxNumWidth());
    ثابت unsigned limit = std::min(configuredLimit, SVA_VALUE_HARD_LIMIT);
    𐡄𐡍 (constp->num().fitsInUInt() && constp->toUInt() <= limit) أرجع 𐡒𐡔𐡈;
    nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << what << " exceeds implementation limit of "
                                                 << limit
                                                 << (configuredLimit <= SVA_VALUE_HARD_LIMIT
                                                         ? " (--max-num-width)"
                                                         : " (host arithmetic limit)"));
    أرجع خطأ;
}

// The bound of a cycle delay in a sequence, if a non-negative elaboration-time constant
ساكن ثابت AstConst* checkSvaDelayBound(AstNode* errp, AstNodeExpr* boundp, ثابت حرف* what) {
    ثابت AstConst* ثابت constp = VN_CAST(boundp, Const);
    𐡄𐡍 (!constp || (constp->dtypep()->isSigned() && constp->num().isNegative())) {
        errp->v3error(what << " is not a non-negative elaboration-time constant"
                              " (IEEE 1800-2023 16.7)");
        أرجع nullptr;
    }
    𐡄𐡍 (!V3AssertCommon::checkSvaValueLimit(errp, constp, "SVA cycle delay")) أرجع nullptr;
    أرجع constp;
}

// Fold the bounds of a cycle delay in a sequence, and return if they are valid, else report it
// on errp
ساكن منطقي svaCycleDelayValid(ثابت AstDelay* nodep, AstNode* errp) {
    V3Const::constifyParamsNoWarnEdit(nodep->lhsp());
    ثابت AstConst* ثابت minConstp = checkSvaDelayBound(
        errp, nodep->lhsp(), nodep->isRangeDelay() ? "Range delay minimum" : "Delay value");
    𐡄𐡍 (!nodep->rhsp() || nodep->isUnbounded()) أرجع minConstp != nullptr;
    V3Const::constifyParamsNoWarnEdit(nodep->rhsp());
    ثابت AstConst* ثابت maxConstp
        = checkSvaDelayBound(errp, nodep->rhsp(), "Range delay maximum");
    𐡄𐡍 (!minConstp || !maxConstp) أرجع خطأ;
    𐡄𐡍 (maxConstp->toUInt() < minConstp->toUInt()) {
        errp->v3error("Range delay maximum must be >= minimum (IEEE 1800-2023 16.7)");
        أرجع خطأ;
    }
    أرجع 𐡒𐡔𐡈;
}

فراغ V3AssertCommon::checkSvaCycleDelay(AstDelay* nodep, AstNode* errp) {
    𐡄𐡍 (svaCycleDelayValid(nodep, errp)) أرجع;
    // Make the delay valid, so later steps do not fail after the error
    FileLine* ثابت flp = nodep->fileline();
    AstNodeExpr* ثابت minp = nodep->lhsp();
    minp->replaceWith(جديد AstConst{flp, AstConst::BitFalseErroring{}});
    VL_DO_DANGLING(minp->deleteTree(), minp);
    𐡄𐡍 (nodep->rhsp() && !nodep->isUnbounded()) {
        AstNodeExpr* ثابت maxp = nodep->rhsp();
        maxp->replaceWith(جديد AstConst{flp, AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(maxp->deleteTree(), maxp);
    }
}

منطقي V3AssertCommon::refsSvaFormal(ثابت AstNode* nodep) {
    أرجع nodep->exists([](ثابت AstNodeVarRef* refp) {
        ثابت AstVar* ثابت varp = refp->varp();
        𐡄𐡍 (!varp || !varp->isIO()) أرجع خطأ;
        ثابت AstNode* ثابت declp = varp->aboveLoopp();
        أرجع VN_IS(declp, Sequence) || VN_IS(declp, Property);
    });
}

فراغ V3AssertCommon::checkSvaInstanceDelays(AstNode* nodep, AstNode* instp) {
    // A delay still referencing a formal argument of an enclosing sequence or property is
    // checked when that is instanced. Collect first, as checking edits the delays.
    std::vector<AstDelay*> delayps;
    nodep->foreach([&](AstDelay* delayp) {
        𐡄𐡍 (!delayp->isCycleDelay() || refsSvaFormal(delayp->lhsp())) أرجع;
        𐡄𐡍 (delayp->rhsp() && refsSvaFormal(delayp->rhsp())) أرجع;
        delayps.push_back(delayp);
    });
    لكل (AstDelay* ثابت delayp : delayps) checkSvaCycleDelay(delayp, instp);
}

//######################################################################
// AssertDeFutureVisitor
// If any AstFuture, then move all non-future varrefs to be one cycle behind,
// see IEEE 1800-2023 16.9.4.

صنف AssertDeFutureVisitor final : عام VNVisitor {
    // STATE - across all visitors
    AstNodeModule* ثابت m_modp;  // Module future is underneath
    ثابت AstFuture* m_futurep;  // First AstFuture found
    ثابت unsigned m_pastNum;  // Prefix unique number for this module
    std::map<AstVar*, AstVar*> m_delayedVars;  // Old to delayed variable mapping
    // STATE - for current visit position (use VL_RESTORER)
    منطقي m_inFuture = خطأ;  // Inside a future
    منطقي m_unsupported = خطأ;  // Printed unsupported

    // METHODS
    فراغ unsupported(AstNode* nodep) {
        𐡄𐡍 (m_unsupported) أرجع;
        m_unsupported = 𐡒𐡔𐡈;
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported/illegal: Future value function used with expression with "
                          << nodep->prettyOperatorName());
    }
    // VISITORS
    فراغ visit(AstFuture* nodep) override {
        VL_RESTORER(m_inFuture);
        m_inFuture = 𐡒𐡔𐡈;
        iterateChildren(nodep);
        // Done with the future, this subexpression is current-time
        nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstNodeVarRef* nodep) override {
        𐡄𐡍 (nodep->user1SetOnce()) أرجع;
        𐡄𐡍 (m_inFuture || m_unsupported)
            أرجع;  // Need user1 set above, don't process when Future is removed
        𐡄𐡍 (nodep->access().isWriteOrRW()) {
            unsupported(nodep);
            أرجع;
        }
        auto it = m_delayedVars.find(nodep->varp());
        AstVar* outvarp;
        𐡄𐡍 (it == m_delayedVars.end()) {
            AstSenTree* ثابت sentreep = m_futurep->sentreep();
            AstAlways* ثابت alwaysp = جديد AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS,
                                                     sentreep->cloneTree(خطأ), nullptr};
            m_modp->addStmtsp(alwaysp);
            outvarp = جديد AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                 "__Vnotfuture" + cvtToStr(m_pastNum) + "_" + nodep->name(),
                                 nodep->dtypep()};
            m_modp->addStmtsp(outvarp);
            AstVarRef* varRefAWritep = جديد AstVarRef{nodep->fileline(), outvarp, VAccess::WRITE};
            varRefAWritep->user1(𐡒𐡔𐡈);
            AstNodeVarRef* varRefAReadp = nodep->cloneTree(خطأ);
            varRefAReadp->user1(𐡒𐡔𐡈);
            AstNode* ثابت assp = جديد AstAssignDly{nodep->fileline(), varRefAWritep, varRefAReadp};
            alwaysp->addStmtsp(assp);
            m_delayedVars.emplace(nodep->varp(), outvarp);
        } وإلا {
            outvarp = it->second;
        }
        AstVarRef* newp = جديد AstVarRef{nodep->fileline(), outvarp, VAccess::READ};
        newp->user1(𐡒𐡔𐡈);
        UINFO(9, "DeFuture " << nodep << "  becomes " << newp);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstNodeFTaskRef* nodep) override { unsupported(nodep); }
    فراغ visit(AstMethodCall* nodep) override { unsupported(nodep); }
    فراغ visit(AstNode* nodep) override {
        𐡄𐡍 (!nodep->isPure()) unsupported(nodep);
        iterateChildren(nodep);
    }

عام:
    // CONSTRUCTORS
    explicit AssertDeFutureVisitor(AstNode* nodep, AstNodeModule* modp, unsigned pastNum)
        : m_modp{modp}
        , m_pastNum{pastNum} {
        // See if any Future before we process
        𐡄𐡍 (nodep->forall([&](ثابت AstFuture* futurep) -> منطقي {
                m_futurep = futurep;
                أرجع خطأ;
            }))
            أرجع;
        // UINFOTREE(9, nodep, "", "defuture-in");
        visit(nodep);  // Nodep may get deleted
        // UINFOTREE(9, nodep, "", "defuture-ou");
    }
    ~AssertDeFutureVisitor() = افتراضي;
};

// Prepare one default sampled expression.
صنف DefaultSampledValueVisitor final : عام VNVisitor {
    // METHODS
    ساكن AstConst* newTypeDefault(AstNodeExpr* nodep) {
        AstConst* ثابت constp
            = جديد AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()};
        𐡄𐡍 (nodep->dtypep()->isFourstate()) constp->num().setAllBitsX();
        constp->dtypeFrom(nodep);
        أرجع constp;
    }

    // VISITORS
    فراغ visit(AstCMethodHard* nodep) override {
        𐡄𐡍 (nodep->method() == VCMethod::EVENT_IS_TRIGGERED) {
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } وإلا {
            iterateChildren(nodep);
        }
    }
    فراغ visit(AstNodeVarRef* nodep) override {
        AstVar* ثابت varp = nodep->varp();
        𐡄𐡍 (varp->isParam()
            || (!varp->isNet() && varp->lifetime().isStatic() && varp->hasUserInit())) {
            أرجع;
        }
        nodep->replaceWith(newTypeDefault(nodep));
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
    explicit DefaultSampledValueVisitor(AstNode* nodep) { iterate(nodep); }
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
    AstNodeModule* m_modp = nullptr;  // Last module
    ثابت AstNode* m_beginp = nullptr;  // Last AstBegin/AstGenBlock
    unsigned m_monitorNum = 0;  // Global $monitor numbering (not per module)
    AstVar* m_monitorNumVarp = nullptr;  // $monitor number variable
    AstVar* m_monitorOffVarp = nullptr;  // $monitoroff variable
    unsigned m_modPastNum = 0;  // Module past numbering
    unsigned m_modStrobeNum = 0;  // Module $strobe numbering
    AstNodeProcedure* m_procedurep = nullptr;  // Current procedure
    VDouble0 m_statCover;  // Statistic tracking
    VDouble0 m_statAsNotImm;  // Statistic tracking
    VDouble0 m_statAsImm;  // Statistic tracking
    VDouble0 m_statAsFull;  // Statistic tracking
    VDouble0 m_statPastVars;  // Statistic tracking
    VDouble0 m_statAssertOnCombined;  // Statistic tracking
    VDouble0 m_statAssertOnHoisted;  // Statistic tracking
    منطقي m_inSampled = خطأ;  // True inside a sampled expression
    منطقي m_inRestrict = خطأ;  // True inside restrict assertion
    AstNode* m_passsp = nullptr;  // Current pass statement
    AstNode* m_failsp = nullptr;  // Current fail statement
    AstNodeCoverOrAssert* m_assertp = nullptr;  // Current assertion
    AstFinal* m_finalp = nullptr;  // Current final block
    VDouble0 m_statLiftedCaseExprs;  // Count of purified case expressions
    AstNodeFTask* m_ftaskp = nullptr;  // Current function/task
    V3UniqueNames m_caseTempNames{"__VCase"};
    V3UniqueNames m_matchCountNames{"__VnfaRemainingMatchCount"};  // Match replay counter names
    // Maps from (expression, senTree) to the AstAlways that computes its delayed values.
    std::unordered_map<VNRef<AstNodeExpr>, std::unordered_map<VNRef<AstSenTree>, AstAlways*>>
        m_modExpr2Sen2DelayedAlwaysp;
    // Source sampled-value history is initialized, unlike internal property-lowering history.
    std::unordered_map<VNRef<AstNodeExpr>, std::unordered_map<VNRef<AstSenTree>, AstAlways*>>
        m_modExpr2Sen2DefaultDelayedAlwaysp;
    // Map from delayed-value AstAlways to its last-sampling-tick time variable
    std::unordered_map<ثابت AstAlways*, AstVar*> m_delayedAlways2TickTimep;

    // METHODS
    ساكن ثابت حرف* assertPassOnQuery(منطقي vacuous) {
        ساكن constexpr ثابت حرف* queries[2]
            = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
        أرجع queries[vacuous];
    }
    ساكن AstNodeExpr* assertOnCond(FileLine* fl, VAssertType type,
                                     VAssertDirectiveType directiveType) {
        // cppcheck-suppress missingReturn
        بدل (directiveType) {
        حالة VAssertDirectiveType::INTRINSIC: أرجع جديد AstConst{fl, AstConst::BitTrue{}};
        حالة VAssertDirectiveType::VIOLATION_CASE: {
            𐡄𐡍 (v3Global.opt.assertCase()) {
                أرجع جديد AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            // If assertions are off, have constant propagation rip them out later
            // This allows syntax errors and such to be detected normally.
            أرجع جديد AstConst{fl, AstConst::BitFalse{}};
        }
        حالة VAssertDirectiveType::ASSERT:
        حالة VAssertDirectiveType::COVER:
        حالة VAssertDirectiveType::ASSUME: {
            𐡄𐡍 (v3Global.opt.assertOn()) {
                أرجع جديد AstCExpr{
                    fl, AstCExpr::Pure{},
                    V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
            }
            أرجع جديد AstConst{fl, AstConst::BitFalse{}};
        }
        حالة VAssertDirectiveType::INTERNAL:
        حالة VAssertDirectiveType::VIOLATION_IF:
        حالة VAssertDirectiveType::RESTRICT: {
            𐡄𐡍 (v3Global.opt.assertOn()) {
                أرجع جديد AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            أرجع جديد AstConst{fl, AstConst::BitFalse{}};
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
    ساكن AstNodeExpr* assertPassOnCond(FileLine* fl, VAssertType type,
                                         VAssertDirectiveType directiveType, منطقي vacuous) {
        𐡄𐡍 (!isControlled(directiveType)) أرجع جديد AstConst{fl, AstConst::BitTrue{}};
        𐡄𐡍 (!v3Global.opt.assertOn()) أرجع جديد AstConst{fl, AstConst::BitFalse{}};
        أرجع جديد AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType), 1};
    }
    ساكن AstNodeExpr* assertFailOnCond(FileLine* fl, VAssertType type,
                                         VAssertDirectiveType directiveType) {
        𐡄𐡍 (!isControlled(directiveType)) أرجع جديد AstConst{fl, AstConst::BitTrue{}};
        𐡄𐡍 (!v3Global.opt.assertOn()) أرجع جديد AstConst{fl, AstConst::BitFalse{}};
        أرجع جديد AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType), 1};
    }
    string assertDisplayMessage(ثابت AstNode* nodep, ثابت string& prefix, ثابت string& message,
                                VDisplayType severity) {
        𐡄𐡍 (severity == VDisplayType::DT_ERROR || severity == VDisplayType::DT_FATAL) {
            أرجع ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                    + cvtToStr(nodep->fileline()->lineno()) + ": Assertion failed in %m"
                    + ((message != "") ? ": " : "") + message + "\n");
        }
        أرجع ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                + cvtToStr(nodep->fileline()->lineno()) + ": %m" + ((message != "") ? ": " : "")
                + message + "\n");
    }
    فراغ replaceDisplay(AstDisplay* nodep, ثابت string& prefix) {
        nodep->fmtp()->text(
            assertDisplayMessage(nodep, prefix, nodep->fmtp()->text(), nodep->displayType()));
        nodep->displayType(VDisplayType::DT_WRITE);
        // cppcheck-suppress nullPointer
        AstNodeExpr* ثابت timenewp = جديد AstTime{nodep->fileline(), m_modp->timeunit()};
        nodep->fmtp()->prependExprsp(timenewp);
        𐡄𐡍 (!nodep->fmtp()->scopeNamep() && nodep->fmtp()->formatScopeTracking()) {
            nodep->fmtp()->scopeNamep(جديد AstScopeName{nodep->fileline(), 𐡒𐡔𐡈});
        }
    }
    AstSampled* newSampledExpr(AstNodeExpr* nodep) {
        أرجع جديد AstSampled{nodep->fileline(), nodep, nodep->dtypep(), 𐡒𐡔𐡈};
    }
    AstVarRef* newMonitorNumVarRefp(ثابت AstNode* nodep, VAccess access) {
        𐡄𐡍 (!m_monitorNumVarp) {
            m_monitorNumVarp = جديد AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorNum",
                                          nodep->findUInt64DType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitorNumVarp);
        }
        AstVarRef* ثابت varrefp = جديد AstVarRef{nodep->fileline(), m_monitorNumVarp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        أرجع varrefp;
    }
    AstVarRef* newMonitorOffVarRefp(ثابت AstNode* nodep, VAccess access) {
        𐡄𐡍 (!m_monitorOffVarp) {
            m_monitorOffVarp = جديد AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorOff",
                                          nodep->findBitDType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitorOffVarp);
        }
        AstVarRef* ثابت varrefp = جديد AstVarRef{nodep->fileline(), m_monitorOffVarp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        أرجع varrefp;
    }
    ساكن AstIf* newIfAssertOn(AstNode* bodyp, VAssertDirectiveType directiveType,
                                VAssertType type = VAssertType::INTERNAL) {
        // Add an internal if to check assertions are on.
        // Don't make this an AND term, as it's unlikely to need to test this.
        FileLine* ثابت fl = bodyp->fileline();

        AstNodeExpr* ثابت condp = assertOnCond(fl, type, directiveType);
        AstIf* ثابت newp = جديد AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(𐡒𐡔𐡈);  // To avoid LATCH warning
        newp->user2(𐡒𐡔𐡈);  // Mark as an assertOn() check
        أرجع newp;
    }
    ساكن AstNodeStmt* newIfAssertPassOn(AstNode* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type, منطقي vacuous) {
        // Add an internal if to check assertion passOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        FileLine* ثابت fl = bodyp->fileline();
        AstNodeExpr* ثابت condp = assertPassOnCond(fl, type, directiveType, vacuous);
        AstNodeIf* ثابت newp = جديد AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(𐡒𐡔𐡈);  // To avoid LATCH warning
        newp->user1(𐡒𐡔𐡈);  // Don't assert/cover this if
        newp->user2(𐡒𐡔𐡈);  // Mark as an assertOn() check
        أرجع newp;
    }
    ساكن AstNodeStmt* newIfAssertFailOn(AstNode* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type) {
        // Add an internal if to check assertion failOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        FileLine* ثابت fl = bodyp->fileline();
        AstNodeExpr* ثابت condp = assertFailOnCond(fl, type, directiveType);
        AstNodeIf* ثابت newp = جديد AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(𐡒𐡔𐡈);  // To avoid LATCH warning
        newp->user1(𐡒𐡔𐡈);  // Don't assert/cover this if
        newp->user2(𐡒𐡔𐡈);  // Mark as an assertOn() check
        أرجع newp;
    }

    ساكن AstIf* assertCond(ثابت AstNodeCoverOrAssert* nodep, AstNodeExpr* propp,
                             AstNode* passsp, AstNode* failsp) {
        AstIf* ثابت ifp = جديد AstIf{nodep->fileline(), propp, passsp, failsp};
        // It's more LIKELY that we'll take the nullptr if clause
        // than the sim-killing else clause:
        ifp->branchPred(VBranchPred::BP_LIKELY);
        ifp->isBoundsCheck(𐡒𐡔𐡈);  // To avoid LATCH warning
        أرجع ifp;
    }

    AstNodeStmt* assertBody(ثابت AstNodeCoverOrAssert* nodep, AstNode* propp, AstNode* passsp,
                            AstNode* failsp) {
        𐡄𐡍 (AstPExpr* ثابت pexprp = VN_CAST(propp, PExpr)) {
            𐡄𐡍 (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NOTIMING, "This property expression requires --timing");
                VL_DO_DANGLING(pushDeletep(pexprp), pexprp);
                أرجع جديد AstBegin{nodep->fileline(), "", nullptr, خطأ};
            }
            AstFork* ثابت forkp = جديد AstFork{nodep->fileline(), VJoinType::JOIN_NONE};
            forkp->addForksp(pexprp->bodyp()->unlinkFrBack());
            𐡄𐡍 (AstNodeStmt* ثابت finalp = pexprp->finalp()) {
                𐡄𐡍 (!m_finalp) {
                    m_finalp = جديد AstFinal{m_modp->fileline(), finalp->unlinkFrBack()};
                    m_modp->addStmtsp(m_finalp);
                } وإلا {
                    m_finalp->addStmtsp(finalp->unlinkFrBack());
                }
            }
            VL_DO_DANGLING2(pushDeletep(pexprp), pexprp, propp);
            أرجع forkp;
        }

        أرجع assertCond(nodep, VN_AS(propp, NodeExpr), passsp, failsp);
    }

    AstNodeStmt* newFireAssertUnchecked(ثابت AstNodeStmt* nodep, ثابت string& message,
                                        AstNodeExpr* exprsp = nullptr) {
        // Like newFireAssert() but omits the asserts-on check
        AstDisplay* ثابت dispp
            = جديد AstDisplay{nodep->fileline(), VDisplayType::DT_ERROR, message, nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstNodeStmt* ثابت bodysp = dispp;
        replaceDisplay(dispp, "%%Error");  // Convert to standard DISPLAY format
        𐡄𐡍 (exprsp) dispp->fmtp()->exprsp()->addNext(exprsp);
        𐡄𐡍 (v3Global.opt.stopFail()) bodysp->addNext(جديد AstStop{nodep->fileline(), خطأ});
        أرجع bodysp;
    }

    AstNodeStmt* newFireAssert(ثابت AstNodeStmt* nodep, VAssertDirectiveType directiveType,
                               VAssertType assertType, ثابت string& message,
                               AstNodeExpr* exprsp = nullptr) {
        AstNodeStmt* bodysp = newFireAssertUnchecked(nodep, message, exprsp);
        bodysp = newIfAssertOn(bodysp, directiveType, assertType);
        أرجع bodysp;
    }

    AstVar* createDelayedVar(ثابت std::string& name, AstAlways* alwaysp, AstNodeExpr* exprp,
                             AstNodeExpr* initp) {
        FileLine* ثابت flp = exprp->fileline();
        AstVar* ثابت varp = جديد AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        // TODO: this lifetime seems nonsene (can't have NBAs to automatics), but is as before
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->addStmtsp(varp);
        // Before enough clock ticks, $past returns the expression's default sampled value.
        𐡄𐡍 (initp) {
            m_modp->addStmtsp(جديد AstInitialStatic{
                flp, جديد AstAssign{flp, جديد AstVarRef{flp, varp, VAccess::WRITE}, initp}});
        }
        ++m_statPastVars;
        // Actually set the delayed value
        AstNodeExpr* ثابت lhsp = جديد AstVarRef{flp, varp, VAccess::WRITE};
        AstAssignDly* ثابت assignp = جديد AstAssignDly{flp, lhsp, exprp};
        alwaysp->prependStmtsp(assignp);
        أرجع varp;
    }

    AstAlways* getDelayedAlways(AstNodeExpr* exprp, AstNodeExpr* initp, AstSenTree* senTreep) {
        auto& expr2Sen2Alwaysr
            = initp ? m_modExpr2Sen2DefaultDelayedAlwaysp : m_modExpr2Sen2DelayedAlwaysp;
        AstAlways*& alwayspr = expr2Sen2Alwaysr[*exprp][*senTreep];
        𐡄𐡍 (!alwayspr) {
            FileLine* ثابت flp = exprp->fileline();
            // Create the always block that computes the delayed values
            alwayspr = جديد AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, nullptr};
            m_modp->addStmtsp(alwayspr);
            // Create the once-delayed variable
            ثابت std::string name = "_Vpast_" + cvtToStr(m_modPastNum++) + "_1";
            AstVar* ثابت varp = createDelayedVar(name, alwayspr, exprp, initp);
            // Add it to delayed variable vector
            m_delayed(alwayspr).emplace_back(varp);
        } وإلا {
            // Reusing exiting, not needed
            VL_DO_DANGLING(pushDeletep(exprp), exprp);
            𐡄𐡍 (initp) VL_DO_DANGLING(pushDeletep(initp), initp);
            VL_DO_DANGLING(pushDeletep(senTreep), senTreep);
        }
        أرجع alwayspr;
    }

    AstNodeExpr* getPastValue(AstNodeExpr* exprp, AstNodeExpr* initp, AstSenTree* senTreep,
                              uint32_t ticks) {
        UASSERT_OBJ(ticks > 0, exprp, "Delay must be > 0");
        FileLine* ثابت flp = exprp->fileline();
        AstAlways* ثابت alwaysp
            = getDelayedAlways(exprp, initp ? initp->cloneTreePure(خطأ) : nullptr, senTreep);
        AstNodeExpr* ثابت resultp = pastValueRef(alwaysp, flp, ticks, initp);
        𐡄𐡍 (initp) VL_DO_DANGLING(pushDeletep(initp), initp);
        أرجع resultp;
    }

    AstNodeExpr* pastValueRef(AstAlways* alwaysp, FileLine* flp, uint32_t ticks,
                              AstNodeExpr* initp) {
        std::vector<AstVar*>& delayedr = m_delayed(alwaysp);
        // Ensure the required delay exists
        طالما (delayedr.size() < ticks) {
            AstVar* ثابت firstp = delayedr.front();
            FileLine* ثابت varFlp = firstp->fileline();
            // Create once more delayed value
            std::string name = firstp->name();
            name.resize(name.size() - 1);
            name += std::to_string(delayedr.size() + 1);
            AstNodeExpr* ثابت prevp = جديد AstVarRef{varFlp, delayedr.back(), VAccess::READ};
            AstVar* ثابت varp = createDelayedVar(name, alwaysp, prevp,
                                                  initp ? initp->cloneTreePure(خطأ) : nullptr);
            // Add it to delayed variable vector
            delayedr.emplace_back(varp);
        }
        // Return a reference to the appropriately delayed variable
        أرجع جديد AstVarRef{flp, delayedr.at(ticks - 1), VAccess::READ};
    }

    // Time of the pipeline's last sampling tick, for end-of-simulation readers
    AstVar* getPastTickTimeVar(AstAlways* alwaysp) {
        AstVar*& varpr = m_delayedAlways2TickTimep[alwaysp];
        𐡄𐡍 (!varpr) {
            FileLine* ثابت flp = alwaysp->fileline();
            varpr = جديد AstVar{flp, VVarType::MODULETEMP,
                               "_Vpast_" + cvtToStr(m_modPastNum++) + "_t",
                               m_modp->findUInt64DType()};
            m_modp->addStmtsp(varpr);
            AstNodeExpr* ثابت timep = جديد AstCExpr{flp, "vlSymsp->_vm_contextp__->time()", 64};
            alwaysp->addStmtsp(
                جديد AstAssign{flp, جديد AstVarRef{flp, varpr, VAccess::WRITE}, timep});
        }
        أرجع varpr;
    }

    فراغ visitAssertionIterate(AstNodeCoverOrAssert* nodep, AstNode* failsp) {
        𐡄𐡍 (m_beginp && nodep->name() == "") nodep->name(m_beginp->name());

        { AssertDeFutureVisitor{nodep->propp(), m_modp, m_modPastNum++}; }

        iterateAndNextNull(nodep->sentreep());
        𐡄𐡍 (AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            iterateAndNextNull(assertp->failsp());
        } وإلا 𐡄𐡍 (AstAssertIntrinsic* ثابت assertp = VN_CAST(nodep, AssertIntrinsic)) {
            iterateAndNextNull(assertp->failsp());
        } وإلا 𐡄𐡍 (AstCover* ثابت coverp = VN_CAST(nodep, Cover)) {
            iterateAndNextNull(coverp->coverincsp());
        } وإلا 𐡄𐡍 (!VN_IS(nodep, Restrict)) {
            nodep->v3fatalSrc("Unhandled assert type");
        }
        iterateAndNextNull(nodep->passsp());
        AstSenTree* sentreep = nodep->sentreep();
        𐡄𐡍 (nodep->immediate()) {
            UASSERT_OBJ(!sentreep, nodep, "Immediate assertions don't have sensitivity");
        } وإلا {
            UASSERT_OBJ(sentreep, nodep, "Concurrent assertions must have sensitivity");
            // Explicit inline clock differs from the enclosing always: hoist
            // and warn. To support this need queue of asserts to activate.
            𐡄𐡍 (m_procedurep && !nodep->senFromAlways()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Procedural concurrent assertion with"
                              " clocking event inside always (IEEE 1800-2023 16.14.6)");
                static_cast<AstNode*>(m_procedurep)->addNext(nodep->unlinkFrBack());
                أرجع;  // Later iterate will pick up
            }
            sentreep->unlinkFrBack();
            𐡄𐡍 (m_procedurep) { VL_DO_DANGLING(pushDeletep(sentreep), sentreep); }
        }
        //
        ثابت string& message = nodep->name();
        AstNode* passsp = nodep->passsp();
        𐡄𐡍 (passsp) passsp->unlinkFrBackWithNext();
        𐡄𐡍 (AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            failsp = assertp->failsp();
        } وإلا 𐡄𐡍 (AstAssertIntrinsic* ثابت assertp = VN_CAST(nodep, AssertIntrinsic)) {
            failsp = assertp->failsp();
        }
        𐡄𐡍 (failsp) failsp->unlinkFrBackWithNext();

        منطقي selfDestruct = خطأ;
        منطقي passspGated = خطأ;
        ثابت AstCover* ثابت coverp = VN_CAST(nodep, Cover);
        // A sequence event control is not an assertion directive; no assertion control
        ثابت منطقي seqEvent = coverp && coverp->isSeqEvent();
        𐡄𐡍 (coverp) {
            ++m_statCover;
            𐡄𐡍 (seqEvent) {
                // Keep the event-fire action, with no coverage bucket
            } وإلا 𐡄𐡍 (!v3Global.opt.coverageUser()) {
                selfDestruct = 𐡒𐡔𐡈;
            } وإلا {
                // V3Coverage assigned us a bucket to increment.
                AstCoverInc* ثابت covincp = VN_AS(coverp->coverincsp(), CoverInc);
                UASSERT_OBJ(covincp, coverp, "Missing AstCoverInc under assertion");
                covincp->unlinkFrBackWithNext();  // next() might have  AstAssign for trace
                𐡄𐡍 (message != "") covincp->declp()->comment(message);
                𐡄𐡍 (passsp) {
                    passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                               /*vacuous=*/خطأ);
                    passspGated = 𐡒𐡔𐡈;
                    passsp = AstNode::addNext<AstNode, AstNode>(covincp, passsp);
                } وإلا {
                    passsp = covincp;
                }
            }
        } وإلا 𐡄𐡍 (VN_IS(nodep, Assert) || VN_IS(nodep, AssertIntrinsic)) {
            𐡄𐡍 (nodep->immediate()) {
                ++m_statAsImm;
            } وإلا {
                ++m_statAsNotImm;
            }
            𐡄𐡍 (!passsp && !failsp)
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

        AstNode* propExprp;
        AstNodeExpr* disablep = nullptr;
        AstNodeExpr* matchCountp = nullptr;
        𐡄𐡍 (AstPropSpec* ثابت specp = VN_CAST(nodep->propp(), PropSpec)) {
            propExprp = specp->propp()->unlinkFrBack();
            𐡄𐡍 (specp->disablep()) disablep = specp->disablep()->unlinkFrBack();
            matchCountp = specp->matchCountp();
        } وإلا {
            propExprp = nodep->propp()->unlinkFrBack();
        }
        FileLine* ثابت flp = nodep->fileline();
        منطقي passspAlreadyGated = خطأ;
        𐡄𐡍 (passsp && VN_IS(passsp, If)) passspAlreadyGated = VN_AS(passsp, If)->user1();
        𐡄𐡍 (passsp && !passspGated && !passspAlreadyGated && !VN_IS(propExprp, PExpr)
            && !seqEvent) {
            passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                       /*vacuous=*/خطأ);
        }
        𐡄𐡍 (failsp && !VN_IS(propExprp, PExpr)) {
            failsp = newIfAssertFailOn(failsp, nodep->directive(), nodep->userType());
        }
        𐡄𐡍 (coverp && matchCountp && passsp) {
            // Convert the match count into a loop that decrements a temporary variable until it
            // reaches zero.
            matchCountp->unlinkFrBack();
            AstVar* ثابت remainingp = جديد AstVar{
                flp, VVarType::MODULETEMP, m_matchCountNames.get(""), matchCountp->dtypep()};
            remainingp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remainingp);
            AstNode* ثابت replaysp
                = جديد AstAssign{flp, جديد AstVarRef{flp, remainingp, VAccess::WRITE}, matchCountp};
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            loopp->addStmtsp(
                جديد AstLoopTest{flp, loopp, جديد AstVarRef{flp, remainingp, VAccess::READ}});
            loopp->addStmtsp(passsp);
            loopp->addStmtsp(
                جديد AstAssign{flp, جديد AstVarRef{flp, remainingp, VAccess::WRITE},
                              جديد AstSub{flp, جديد AstVarRef{flp, remainingp, VAccess::READ},
                                         جديد AstConst{flp, AstConst::WidthedValue{},
                                                      remainingp->dtypep()->width(), 1}}});
            replaysp->addNext(loopp);
            passsp = replaysp;
        }
        AstNode* bodysp = assertBody(nodep, propExprp, passsp, failsp);
        𐡄𐡍 (disablep) bodysp = جديد AstIf{flp, جديد AstLogNot{flp, disablep}, bodysp};
        // Add assertOn check last, for better combining
        𐡄𐡍 (!seqEvent) bodysp = newIfAssertOn(bodysp, nodep->directive(), nodep->userType());
        𐡄𐡍 (sentreep) bodysp = جديد AstAlways{flp, VAlwaysKwd::ALWAYS, sentreep, bodysp};

        𐡄𐡍 (passsp && !passsp->backp()) VL_DO_DANGLING(pushDeletep(passsp), passsp);
        𐡄𐡍 (failsp && !failsp->backp()) VL_DO_DANGLING(pushDeletep(failsp), failsp);

        // Install it
        𐡄𐡍 (selfDestruct) {
            // Delete it after making the tree.  This way we can tell the user
            // if it wasn't constructed nicely or has other errors without needing --coverage.
            VL_DO_DANGLING(bodysp->deleteTree(), bodysp);
            nodep->unlinkFrBack();
        } وإلا {
            nodep->replaceWith(bodysp);
        }
        // Bye
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    منطقي isEmptyStmt(AstNode* nodep) {
        𐡄𐡍 (!nodep) أرجع 𐡒𐡔𐡈;
        𐡄𐡍 (AstBegin* ثابت beginp = VN_CAST(nodep, Begin)) {
            𐡄𐡍 (beginp->declsp()) أرجع خطأ;
            لكل (AstNode* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                𐡄𐡍 (!isEmptyStmt(stmtp)) أرجع خطأ;
            }
            أرجع 𐡒𐡔𐡈;
        }
        أرجع خطأ;
    }

    // VISITORS
    فراغ visit(AstIf* nodep) override {
        𐡄𐡍 (!nodep->user1SetOnce()) {
            𐡄𐡍 (nodep->uniquePragma() || nodep->unique0Pragma()) {
                ثابت AstNodeIf* ifp = nodep;
                AstNodeExpr* propp = nullptr;
                منطقي hasDefaultElse = خطأ;
                𐡏𐡁𐡃 {
                    // If this statement ends with 'else if', then nextIf will point to the
                    // nextIf statement.  Otherwise it will be null.
                    ثابت AstNodeIf* ثابت nextifp = dynamic_cast<AstNodeIf*>(ifp->elsesp());
                    iterateAndNextNull(ifp->condp());

                    // Recurse into the true case.
                    iterateAndNextNull(ifp->thensp());

                    // If the last else is not an else if, recurse into that too.
                    𐡄𐡍 (ifp->elsesp() && !nextifp) {  //
                        iterateAndNextNull(ifp->elsesp());
                    }

                    // Build a bitmask of the true predicates
                    AstNodeExpr* ثابت predp = ifp->condp()->cloneTreePure(خطأ);
                    𐡄𐡍 (propp) {
                        propp = جديد AstConcat{nodep->fileline(), predp, propp};
                    } وإلا {
                        propp = predp;
                    }

                    // Record if this ends with an 'else' that does not have an if
                    𐡄𐡍 (ifp->elsesp() && !nextifp) hasDefaultElse = 𐡒𐡔𐡈;

                    ifp = nextifp;
                } طالما (ifp);

                ثابت منطقي allow_none = nodep->unique0Pragma();

                // Empty case means no property
                𐡄𐡍 (!propp) propp = جديد AstConst{nodep->fileline(), AstConst::BitFalse{}};

                // Note: if this ends with an 'else', then we don't need to validate that one of
                // the predicates evaluates to true.
                AstNodeExpr* ثابت ohot
                    = ((allow_none || hasDefaultElse)
                           ? static_cast<AstNodeExpr*>(جديد AstOneHot0{nodep->fileline(), propp})
                           : static_cast<AstNodeExpr*>(جديد AstOneHot{nodep->fileline(), propp}));
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
                checkifp->isBoundsCheck(𐡒𐡔𐡈);  // To avoid LATCH warning
                checkifp->branchPred(VBranchPred::BP_UNLIKELY);
                relinker.relink(checkifp);
                أرجع;
            }
        }

        iterateChildren(nodep);

        𐡄𐡍 (nodep->user2()) {
            // Combine consecutive assertOn checks if possible
            𐡄𐡍 (AstIf* ثابت backp = VN_CAST(nodep->backp(), If)) {
                𐡄𐡍 (backp->nextp() == nodep  //
                    && backp->user2()  //
                    && backp->condp()->sameTree(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    backp->addThensp(nodep->thensp()->unlinkFrBackWithNext());
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    أرجع;
                }
            }
            // Combine nested assertOn checks if possible
            𐡄𐡍 (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* ثابت checkp = VN_CAST(nodep->thensp(), If);
                𐡄𐡍 (checkp  //
                    && checkp->user2()  //
                    && checkp->condp()->sameTree(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    VL_DO_DANGLING(pushDeletep(checkp->unlinkFrBack()), checkp);
                    أرجع;
                }
            }
            أرجع;
        }

        // Swap assertOn check with single statement 'if' statement to bubble up for combining
        // Note we can't just swap the conditions as they two Ifs have different flags,
        // so swapping the Ifs themeselves then swapping back the bodies.
        𐡄𐡍 (nodep->condp()->isPure()) {
            𐡄𐡍 (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* ثابت checkp = VN_CAST(nodep->thensp(), If);
                𐡄𐡍 (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    أرجع;
                }
            }
            𐡄𐡍 (nodep->elsesp() && !nodep->elsesp()->nextp() && isEmptyStmt(nodep->thensp())) {
                AstIf* ثابت checkp = VN_CAST(nodep->elsesp(), If);
                𐡄𐡍 (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addElsesp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    أرجع;
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
        𐡄𐡍 (!nodep->exprp()->isPure()) {
            ++m_statLiftedCaseExprs;
            FileLine* ثابت fl = nodep->exprp()->fileline();
            AstVar* ثابت varp = جديد AstVar{fl, VVarType::BLOCKTEMP, m_caseTempNames.get(nodep),
                                            nodep->exprp()->dtypep()};
            AstNodeExpr* ثابت origp = nodep->exprp()->unlinkFrBack();
            nodep->addHereThisAsNext(
                جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, origp});
            nodep->exprp(جديد AstVarRef{fl, varp, VAccess::READ});
            𐡄𐡍 (m_ftaskp) {
                varp->funcLocal(𐡒𐡔𐡈);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } وإلا {
                m_modp->prependStmtsp(varp);
            }
            VIsCached::clearCacheTree();
        }
        iterateChildren(nodep);
        𐡄𐡍 (!nodep->user1SetOnce()) {
            منطقي has_default = خطأ;
            لكل (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                𐡄𐡍 (itemp->isDefault()) has_default = 𐡒𐡔𐡈;
            }
            ثابت AstNodeDType* exprDtypep = nodep->exprp()->dtypep()->skipRefp();

            VAssertType assertType = VAssertType::INTERNAL;
            𐡄𐡍 (nodep->priorityPragma()) {
                assertType = VAssertType::PRIORITY;
            } وإلا 𐡄𐡍 (nodep->uniquePragma()) {
                assertType = VAssertType::UNIQUE;
            } وإلا 𐡄𐡍 (nodep->unique0Pragma()) {
                assertType = VAssertType::UNIQUE0;
            }

            string valFmt;
            𐡄𐡍 (exprDtypep->isIntegralOrPacked())
                valFmt = " for '" + cvtToStr(exprDtypep->widthMin()) + "'h%X'";
            𐡄𐡍 (nodep->fullPragma() || nodep->priorityPragma()) {
                // Need to add a default if there isn't one already
                ++m_statAsFull;
                𐡄𐡍 (!has_default) {
                    nodep->addItemsp(جديد AstCaseItem{
                        nodep->fileline(), nullptr /*DEFAULT*/,
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      nodep->pragmaString() + ", but non-match found" + valFmt,
                                      valFmt.empty() ? nullptr
                                                     : nodep->exprp()->cloneTreePure(خطأ))});
                }
            }
            𐡄𐡍 (nodep->parallelPragma() || nodep->uniquePragma() || nodep->unique0Pragma()) {
                // Need to check that one, and only one of the case items match at any moment
                // If there's a default, we allow none to match, else exactly one must match
                ++m_statAsFull;
                𐡄𐡍 (!has_default && !nodep->itemsp()) {
                    // Not parallel, but harmlessly so.
                } وإلا {
                    AstNodeExpr* propp = nullptr;
                    لكل (AstCaseItem* itemp = nodep->itemsp(); itemp;
                         itemp = VN_AS(itemp->nextp(), CaseItem)) {
                        AstNodeExpr* itembitp = nullptr;
                        لكل (AstNodeExpr* icondp = itemp->condsp(); icondp;
                             icondp = VN_AS(icondp->nextp(), NodeExpr)) {
                            AstNodeExpr* onep;
                            𐡄𐡍 (AstInsideRange* ثابت rcondp = VN_CAST(icondp, InsideRange)) {
                                onep = rcondp->newAndFromInside(
                                    nodep->exprp()->cloneTreePure(𐡒𐡔𐡈),
                                    rcondp->lhsp()->cloneTreePure(𐡒𐡔𐡈),
                                    rcondp->rhsp()->cloneTreePure(𐡒𐡔𐡈));
                            } وإلا 𐡄𐡍 (nodep->casex() || nodep->casez() || nodep->caseInside()) {
                                onep = AstEqWild::newTyped(itemp->fileline(),
                                                           nodep->exprp()->cloneTreePure(خطأ),
                                                           icondp->cloneTreePure(خطأ));
                            } وإلا {
                                onep = AstEq::newTyped(icondp->fileline(),
                                                       nodep->exprp()->cloneTreePure(خطأ),
                                                       icondp->cloneTreePure(خطأ));
                            }
                            // OR together all conditions within the same case item
                            𐡄𐡍 (onep) {
                                𐡄𐡍 (itembitp) {
                                    itembitp = جديد AstOr{icondp->fileline(), onep, itembitp};
                                } وإلا {
                                    itembitp = onep;
                                }
                            }
                        }
                        𐡄𐡍 (itembitp) {
                            𐡄𐡍 (propp) {
                                propp = جديد AstConcat{itemp->fileline(), itembitp, propp};
                            } وإلا {
                                propp = itembitp;
                            }
                        }
                    }
                    // Empty case means no property
                    𐡄𐡍 (!propp) propp = جديد AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    ثابت منطقي allow_none = has_default || nodep->unique0Pragma();
                    // The following assertion looks as below.
                    // if (!$onehot(propp)) begin
                    //     if (propp == '0) begin if (!allow_none) $error("none match"); end
                    //     else $error("multiple match");
                    // end
                    AstNodeExpr* ثابت ohot = جديد AstOneHot{nodep->fileline(), propp};
                    AstConst* ثابت zero = جديد AstConst{
                        nodep->fileline(), AstConst::WidthedValue{}, propp->width(), 0};
                    AstIf* ثابت ohotIfp
                        = جديد AstIf{nodep->fileline(), جديد AstLogNot{nodep->fileline(), ohot}};
                    AstIf* ثابت zeroIfp = جديد AstIf{
                        nodep->fileline(),
                        جديد AstEq{nodep->fileline(), propp->cloneTreePure(خطأ), zero}};
                    AstNodeExpr* ثابت exprp = nodep->exprp();
                    ثابت string pragmaStr = nodep->pragmaString();
                    𐡄𐡍 (!allow_none)
                        zeroIfp->addThensp(
                            newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                          pragmaStr + ", but none matched" + valFmt,
                                          valFmt.empty() ? nullptr : exprp->cloneTreePure(خطأ)));
                    zeroIfp->addElsesp(
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      pragmaStr + ", but multiple matches found" + valFmt,
                                      valFmt.empty() ? nullptr : exprp->cloneTreePure(خطأ)));
                    ohotIfp->addThensp(zeroIfp);
                    ohotIfp->isBoundsCheck(𐡒𐡔𐡈);  // To avoid LATCH warning
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
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    //========== Past
    فراغ visit(AstPast* nodep) override {
        𐡄𐡍 (!nodep->propertyTiming()) {
            nodep->initp(nodep->exprp()->cloneTreePure(خطأ));
            { DefaultSampledValueVisitor{nodep->initp()}; }
        }
        AstNodeExpr* ثابت initp = nodep->initp() ? nodep->initp()->unlinkFrBack() : nullptr;
        iterateChildren(nodep);
        uint32_t ticks = 1;
        𐡄𐡍 (nodep->ticksp()) {
            UASSERT_OBJ(VN_IS(nodep->ticksp(), Const), nodep,
                        "Expected constant ticks, checked in V3Width");
            ticks = VN_AS(nodep->ticksp(), Const)->toUInt();
        }
        UASSERT_OBJ(ticks >= 1, nodep, "0 tick should have been checked in V3Width");
        AstNodeExpr* ثابت exprp = newSampledExpr(nodep->exprp()->unlinkFrBack());
        AstSenTree* ثابت senTreep = nodep->sentreep()->unlinkFrBack();
        AstNodeExpr* inp = nullptr;
        // Deliberately emitted for every ftask $past; calls outside final take the normal stage
        𐡄𐡍 (VN_IS(m_procedurep, Final) || m_ftaskp) {
            // A sampling-tick finish reads one stage deeper (IEEE 1800-2023 16.9.3).
            FileLine* ثابت flp = nodep->fileline();
            AstAlways* ثابت alwaysp
                = getDelayedAlways(exprp, initp ? initp->cloneTreePure(خطأ) : nullptr, senTreep);
            AstNodeExpr* ثابت betweenTicksp = pastValueRef(alwaysp, flp, ticks, initp);
            AstNodeExpr* ثابت onTickp = pastValueRef(alwaysp, flp, ticks + 1, initp);
            𐡄𐡍 (initp) VL_DO_DANGLING(pushDeletep(initp), initp);
            AstNodeExpr* ثابت onTickCondp = جديد AstLogAnd{
                flp, جديد AstCExpr{flp, "vlSymsp->_vm_contextp__->executingFinal()", 1},
                جديد AstEq{flp, جديد AstVarRef{flp, getPastTickTimeVar(alwaysp), VAccess::READ},
                          جديد AstCExpr{flp, "vlSymsp->_vm_contextp__->finishPendingTime()", 64}}};
            AstCond* ثابت condp = جديد AstCond{flp, onTickCondp, onTickp, betweenTicksp};
            condp->dtypeFrom(onTickp);
            inp = condp;
        } وإلا {
            inp = getPastValue(exprp, initp, senTreep, ticks);
        }
        nodep->replaceWith(inp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    //========== Move $sampled down to read-only variables
    فراغ visit(AstSampled* nodep) override {
        𐡄𐡍 (nodep->user1()) أرجع;
        VL_RESTORER(m_inSampled);
        {
            m_inSampled = 𐡒𐡔𐡈;
            iterateChildren(nodep);
        }
        𐡄𐡍 (nodep->exprp()) {
            nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        } وإلا {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstNodeVarRef* nodep) override {
        𐡄𐡍 (m_inSampled && !nodep->user2() && !nodep->varp()->isTemp()) {
            𐡄𐡍 (!nodep->access().isReadOnly()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Write to variable in sampled expression");
            } وإلا {
                VNRelinker relinkHandle;
                nodep->unlinkFrBack(&relinkHandle);
                AstSampled* ثابت newp = newSampledExpr(nodep);
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
        AstNode* stmtsp = nullptr;
        𐡄𐡍 (nodep->pass() && m_passsp) {
            // Cover adds COVERINC by AstNode::addNext, thus need to clone next too.
            stmtsp = m_passsp->cloneTree(𐡒𐡔𐡈);
            stmtsp = newIfAssertPassOn(stmtsp, m_assertp->directive(), m_assertp->userType(),
                                       nodep->vacuous());
        } وإلا 𐡄𐡍 (!nodep->pass() && m_failsp) {
            stmtsp = m_failsp->cloneTree(𐡒𐡔𐡈);
            stmtsp = newIfAssertFailOn(stmtsp, m_assertp->directive(), m_assertp->userType());
        }
        𐡄𐡍 (stmtsp) {
            stmtsp->foreachAndNext([](AstNodeVarRef* ثابت refp) {
                // References inside action blocks shouldn't be implicitly sampled
                // m_passsp/m_failsp have been already visited once and refs explicitly sampled
                // are handled already
                refp->user2(1);
            });
            nodep->replaceWith(stmtsp);
        } وإلا {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstPExpr* nodep) override {
        𐡄𐡍 (m_inRestrict) {
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
        } وإلا {
            iterateChildren(nodep);
        }
    }

    //========== Statements
    فراغ visit(AstDisplay* nodep) override {
        iterateChildren(nodep);
        // Replace the special types with standard text
        𐡄𐡍 (nodep->displayType() == VDisplayType::DT_INFO) {
            replaceDisplay(nodep, "-Info");
        } وإلا 𐡄𐡍 (nodep->displayType() == VDisplayType::DT_WARNING) {
            replaceDisplay(nodep, "%%Warning");
        } وإلا 𐡄𐡍 (nodep->displayType() == VDisplayType::DT_ERROR) {
            replaceDisplay(nodep, "%%Error");
        } وإلا 𐡄𐡍 (nodep->displayType() == VDisplayType::DT_FATAL) {
            replaceDisplay(nodep, "%%Fatal");
        } وإلا 𐡄𐡍 (nodep->displayType() == VDisplayType::DT_MONITOR) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            FileLine* ثابت fl = nodep->fileline();

            AstSenItem* monSenItemsp = nullptr;
            𐡄𐡍 (AstNode* ثابت monExprsp = nodep->fmtp()->exprsp()) {
                monExprsp->foreachAndNext([&](AstVarRef* varrefp) {
                    AstSenItem* ثابت senItemp
                        = جديد AstSenItem{fl, VEdgeType::ET_CHANGED,
                                         // Clone so get VarRef or VarXRef as needed
                                         varrefp->cloneTree(خطأ)};
                    monSenItemsp = AstNode::addNextNull(monSenItemsp, senItemp);
                });
            }

            AstSenTree* ثابت monSenTree = جديد AstSenTree{fl, monSenItemsp};
            ثابت auto monNum = ++m_monitorNum;
            // Where $monitor was we do "__VmonitorNum = N;"
            AstAssign* ثابت newsetp = جديد AstAssign{
                fl, newMonitorNumVarRefp(nodep, VAccess::WRITE), جديد AstConst{fl, monNum}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__VmonitorOn && __VmonitorNum==N) $display(...);"
            AstNode* ثابت stmtsp = nodep;
            AstIf* ثابت ifp = جديد AstIf{
                fl,
                جديد AstLogAnd{fl, جديد AstLogNot{fl, newMonitorOffVarRefp(nodep, VAccess::READ)},
                              جديد AstEq{fl, جديد AstConst{fl, monNum},
                                        newMonitorNumVarRefp(nodep, VAccess::READ)}},
                stmtsp};
            ifp->isBoundsCheck(𐡒𐡔𐡈);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            AstNode* ثابت newp = جديد AstAlways{fl, VAlwaysKwd::ALWAYS, monSenTree, ifp};
            m_modp->addStmtsp(newp);
        } وإلا 𐡄𐡍 (nodep->displayType() == VDisplayType::DT_STROBE) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            // Need one-shot
            FileLine* ثابت fl = nodep->fileline();
            AstVar* ثابت varp
                = جديد AstVar{fl, VVarType::MODULETEMP, "__Vstrobe" + cvtToStr(m_modStrobeNum++),
                             nodep->findBitDType()};
            m_modp->addStmtsp(varp);
            // Where $strobe was we do "__Vstrobe = '1;"
            AstAssign* ثابت newsetp = جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                                     جديد AstConst{fl, AstConst::BitTrue{}}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__Vstrobe) begin $display(...); __Vstrobe = '0; end"
            AstNode* ثابت stmtsp = nodep;
            AstIf* ثابت ifp = جديد AstIf{fl, جديد AstVarRef{fl, varp, VAccess::READ}, stmtsp};
            ifp->isBoundsCheck(𐡒𐡔𐡈);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            AstNode* ثابت newp = جديد AstAlwaysPostponed{fl, ifp};
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
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstAssert* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    فراغ visit(AstAssertCtl* nodep) override {
        iterateChildren(nodep);

        منطقي assertTypeConst = 𐡒𐡔𐡈;
        𐡄𐡍 (!nodep->assertTypesp()) {
            nodep->assertTypes(VAssertType{ALL_ASSERT_TYPES});
        } وإلا 𐡄𐡍 (ثابت AstConst* ثابت assertTypesp = VN_CAST(nodep->assertTypesp(), Const)) {
            nodep->assertTypes(VAssertType{assertTypesp->toSInt()});
        } وإلا {
            assertTypeConst = خطأ;
        }

        منطقي controlTypeConst = خطأ;
        𐡄𐡍 (ثابت AstConst* ثابت constp = VN_CAST(nodep->controlTypep(), Const)) {
            nodep->ctlType(constp->toSInt());
            controlTypeConst = 𐡒𐡔𐡈;
        }
        𐡄𐡍 (controlTypeConst
            && (nodep->ctlType() < VAssertCtlType::LOCK
                || nodep->ctlType() > VAssertCtlType::VACUOUS_OFF)) {
            nodep->unlinkFrBack();
            nodep->v3error("Bad $assertcontrol control_type '"
                           << cvtToStr(static_cast<صحيح>(nodep->ctlType()))
                           << "' (IEEE 1800-2023 Table 20-5)");
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            أرجع;
        }
        𐡄𐡍 (assertTypeConst && nodep->assertTypes() != ALL_ASSERT_TYPES
            && nodep->assertTypes().containsAny(VAssertType::UNIQUE | VAssertType::UNIQUE0
                                                | VAssertType::PRIORITY)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: assert control assertion_type");
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            أرجع;
        }

        منطقي directiveTypeConst = 𐡒𐡔𐡈;
        𐡄𐡍 (!nodep->directiveTypesp()) {
            nodep->directiveTypes(VAssertDirectiveType::ASSERT | VAssertDirectiveType::ASSUME
                                  | VAssertDirectiveType::COVER);
        } وإلا 𐡄𐡍 (ثابت AstConst* ثابت directiveTypesp
                   = VN_CAST(nodep->directiveTypesp(), Const)) {
            nodep->directiveTypes(VAssertDirectiveType{directiveTypesp->toSInt()});
        } وإلا {
            directiveTypeConst = خطأ;
        }
        𐡄𐡍 (!directiveTypeConst) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: non-const assert directive type expression");
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            أرجع;
        }

        FileLine* ثابت fl = nodep->fileline();
        UINFO(9, "Generating assertctl for a module: " << m_modp);
        AstCStmt* ثابت newp = جديد AstCStmt{fl};
        newp->add("vlSymsp->_vm_contextp__->assertCtl(");
        newp->add(nodep->controlTypep()->unlinkFrBack());
        newp->add(", ");
        𐡄𐡍 (nodep->assertTypesp()) {
            newp->add(nodep->assertTypesp()->unlinkFrBack());
        } وإلا {
            newp->add(std::to_string(ALL_ASSERT_TYPES));
        }
        newp->add(", " + std::to_string(nodep->directiveTypes()) + ");\n");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstAssertIntrinsic* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    فراغ visit(AstCover* nodep) override {  //
        visitAssertionIterate(nodep, nullptr);
    }
    فراغ visit(AstRestrict* nodep) override {
        VL_RESTORER(m_inRestrict);
        m_inRestrict = 𐡒𐡔𐡈;
        iterateChildren(nodep);
        // IEEE says simulator ignores these
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }

    فراغ visit(AstNodeModule* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_modPastNum);
        VL_RESTORER(m_modStrobeNum);
        VL_RESTORER(m_finalp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DelayedAlwaysp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DefaultDelayedAlwaysp);
        VL_RESTORER_CLEAR(m_delayedAlways2TickTimep);
        m_modp = nodep;
        m_modPastNum = 0;
        m_modStrobeNum = 0;
        m_finalp = nullptr;
        iterateChildren(nodep);
    }
    فراغ visit(AstNodeProcedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstNodeFTask* nodep) override {
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
        𐡄𐡍 (!nodep->declsp()) {
            𐡄𐡍 (AstIf* ثابت ifp = VN_CAST(nodep->stmtsp(), If)) {
                𐡄𐡍 (ifp->user2() && !ifp->nextp()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(ifp->unlinkFrBack());
                    nodep->addStmtsp(ifp->thensp()->unlinkFrBackWithNext());
                    ifp->addThensp(nodep);
                }
            }
        }
    }

    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

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
        VIsCached::clearCacheTree();
    }
};

//######################################################################
// Top Assert class

فراغ V3Assert::assertAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertVisitor{nodep}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("assert", 0, dumpTreeEitherLevel() >= 3);
}
