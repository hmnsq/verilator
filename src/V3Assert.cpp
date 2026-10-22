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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Assert.h"

#अन्तर्भाव "V3AstUserAllocator.h"
#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

नामाकाश {

वर्ग DefaultDisableLocalVisitor final : सार्वजनिक VNVisitor {
    // STATE
    Ast_ग्रन्थिः* m_scopep = nullptr;

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(nullptr);
        iterateChildren(nodep);
    }
    शून्य visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(nullptr);
        iterateChildren(nodep);
    }
    शून्य visit(AstDefaultDisable* nodep) override {
        UASSERT_OBJ(nodep, m_scopep,
                    "default disable iff must be inside a module or generate block");
        AstDefaultDisable* defaultp = nullptr;
        जइ (ध्रुव Ast_ग्रन्थिः_Module* ध्रुव modp = VN_CAST(m_scopep, ग्रन्थिः_Module)) {
            defaultp = modp->defaultDisablep();
        } अण्णहा {
            defaultp = VN_AS(m_scopep, GenBlock)->defaultDisablep();
        }
        जइ (VL_UNLIKELY(defaultp)) {
            nodep->v3error("Only one 'default disable iff' allowed per "
                           << (VN_IS(m_scopep, ग्रन्थिः_Module) ? "module" : "generate block")
                           << " (IEEE 1800-2023 16.15)");
        } अण्णहा जइ (Ast_ग्रन्थिः_Module* ध्रुव modp = VN_CAST(m_scopep, ग्रन्थिः_Module)) {
            modp->defaultDisablep(nodep);
        } अण्णहा {
            VN_AS(m_scopep, GenBlock)->defaultDisablep(nodep);
        }
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    explicit DefaultDisableLocalVisitor(AstNetlist* nodep) { iterate(nodep); }
};

वर्ग DefaultDisablePropagateVisitor final : सार्वजनिक VNVisitor {
    // STATE
    AstDefaultDisable* m_defaultDisablep = nullptr;

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    शून्य visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        जइ (!nodep->defaultDisablep()) nodep->defaultDisablep(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    explicit DefaultDisablePropagateVisitor(AstNetlist* nodep) { iterate(nodep); }
};

// Lower a sequence used as an event control ('@seq', IEEE 1800-2023 9.4.2.4) into a
// synthesized event fired by an internal 'cover sequence' on each end-of-match
वर्ग SeqEventLowerVisitor final : सार्वजनिक VNVisitor {
    // STATE
    Ast_ग्रन्थिः_Module* m_modp = nullptr;  // Current module
    V3UniqueNames m_names{"__VseqEvent"};  // Synthesized event names

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstSenItem* nodep) override {
        AstFuncRef* ध्रुव funcrefp = VN_CAST(nodep->sensp(), FuncRef);
        जइ (funcrefp && VN_IS(funcrefp->taskp(), Sequence)) {
            File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
            AstVar* ध्रुव eventp = नूतन AstVar{flp, VVarType::MODULETEMP, m_names.get(nodep),
                                              m_modp->findBasicDType(VBasicDTypeKwd::EVENT)};
            eventp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(eventp);
            v3Global.setHasEvents();
            funcrefp->unlinkFrBack();
            nodep->sensp(नूतन AstVarRef{flp, eventp, VAccess::READ});
            ध्रुव तार्किक automaticActual = funcrefp->exists([](ध्रुव Ast_ग्रन्थिः_Var_Ref* refp) {
                प्रत्यागम refp->varp() && refp->varp()->lifetime().isAutomatic();
            });
            जइ (automaticActual) {
                nodep->v3error("Arguments to a sequence used as an event control must be"
                               " static (IEEE 1800-2023 9.4.2.4)");
                VN_AS(funcrefp->taskp(), Sequence)->isReferenced(मिच्छ);
                VL_DO_DANGLING(प्रक्षिप_Deletep(funcrefp), funcrefp);
                प्रत्यागम;
            }
            AstFireEvent* ध्रुव firep
                = नूतन AstFireEvent{flp, नूतन AstVarRef{flp, eventp, VAccess::WRITE}, मिच्छ};
            AstCover* ध्रुव coverp
                = नूतन AstCover{flp, नूतन AstPropSpec{flp, nullptr, nullptr, funcrefp}, firep,
                               VAssertType::CONCURRENT};
            coverp->isCoverSeq(सच्च);
            coverp->isSeqEvent(सच्च);
            m_modp->addStmtsp(coverp);
            प्रत्यागम;
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    explicit SeqEventLowerVisitor(AstNetlist* nodep) { iterate(nodep); }
};

}  // namespace

शून्य V3AssertCommon::collectDefaultDisable(AstNetlist* nodep) {
    { DefaultDisableLocalVisitor{nodep}; }
    { DefaultDisablePropagateVisitor{nodep}; }
}

शून्य V3AssertCommon::lowerSequenceEvents(AstNetlist* nodep) {
    { SeqEventLowerVisitor{nodep}; }
    V3Global::dump_Check_Global_वृक्षः("assertseqevent", 0, dump_वृक्षः_Either_Level() >= 3);
}

string V3AssertCommon::assertCtlGetCall(ध्रुव वर्ण* query, VAssertType type,
                                        VAssertDirectiveType directiveType) {
    // Template arguments let the runtime build the type/directive mask at C++ compile time
    प्रत्यागम "vlSymsp->_vm_contextp__->assertCtlGet<"s + std::to_string(type) + ", "s
           + std::to_string(directiveType) + ">(VerilatedAssertCtlQuery::"s + query + ")"s;
}

तार्किक V3AssertCommon::check_Sva_मानम्_Limit(Ast_ग्रन्थिः* nodep, ध्रुव AstConst* constp, ध्रुव वर्ण* what) {
    // Temporal values can materialize as O(N) AST or state in later passes.
    // Leave headroom for a range ring's extra slot and V3Number's signed
    // `(bits + 31) / 32` word count.
    स्थिर constexpr unsigned SVA_मानम्_HARD_LIMIT
        = static_cast<unsigned>(std::numeric_limits<पूर्णाङ्क>::max()) - 32U;
    ध्रुव unsigned configuredLimit = static_cast<unsigned>(v3Global.opt.max_अङ्कः_Width());
    ध्रुव unsigned limit = std::min(configuredLimit, SVA_मानम्_HARD_LIMIT);
    जइ (constp->अङ्कः().fitsInUInt() && constp->toUInt() <= limit) प्रत्यागम सच्च;
    nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << what << " exceeds implementation limit of "
                                                 << limit
                                                 << (configuredLimit <= SVA_मानम्_HARD_LIMIT
                                                         ? " (--max-num-width)"
                                                         : " (host arithmetic limit)"));
    प्रत्यागम मिच्छ;
}

// The bound of a cycle delay in a sequence, if a non-negative elaboration-time constant
स्थिर ध्रुव AstConst* checkSvaDelayBound(Ast_ग्रन्थिः* errp, Ast_ग्रन्थिः_व्यञ्जकम्* boundp, ध्रुव वर्ण* what) {
    ध्रुव AstConst* ध्रुव constp = VN_CAST(boundp, Const);
    जइ (!constp || (constp->dtypep()->isSigned() && constp->अङ्कः().isNegative())) {
        errp->v3error(what << " is not a non-negative elaboration-time constant"
                              " (IEEE 1800-2023 16.7)");
        प्रत्यागम nullptr;
    }
    जइ (!V3AssertCommon::check_Sva_मानम्_Limit(errp, constp, "SVA cycle delay")) प्रत्यागम nullptr;
    प्रत्यागम constp;
}

// Fold the bounds of a cycle delay in a sequence, and return if they are valid, else report it
// on errp
स्थिर तार्किक svaCycleDelayValid(ध्रुव AstDelay* nodep, Ast_ग्रन्थिः* errp) {
    V3Const::constifyParamsNoWarnEdit(nodep->lhsp());
    ध्रुव AstConst* ध्रुव minConstp = checkSvaDelayBound(
        errp, nodep->lhsp(), nodep->isRangeDelay() ? "Range delay minimum" : "Delay value");
    जइ (!nodep->rhsp() || nodep->isUnbounded()) प्रत्यागम minConstp != nullptr;
    V3Const::constifyParamsNoWarnEdit(nodep->rhsp());
    ध्रुव AstConst* ध्रुव maxConstp
        = checkSvaDelayBound(errp, nodep->rhsp(), "Range delay maximum");
    जइ (!minConstp || !maxConstp) प्रत्यागम मिच्छ;
    जइ (maxConstp->toUInt() < minConstp->toUInt()) {
        errp->v3error("Range delay maximum must be >= minimum (IEEE 1800-2023 16.7)");
        प्रत्यागम मिच्छ;
    }
    प्रत्यागम सच्च;
}

शून्य V3AssertCommon::checkSvaCycleDelay(AstDelay* nodep, Ast_ग्रन्थिः* errp) {
    जइ (svaCycleDelayValid(nodep, errp)) प्रत्यागम;
    // Make the delay valid, so later steps do not fail after the error
    File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव minp = nodep->lhsp();
    minp->replaceWith(नूतन AstConst{flp, AstConst::BitFalseErroring{}});
    VL_DO_DANGLING(minp->delete_वृक्षः(), minp);
    जइ (nodep->rhsp() && !nodep->isUnbounded()) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव maxp = nodep->rhsp();
        maxp->replaceWith(नूतन AstConst{flp, AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(maxp->delete_वृक्षः(), maxp);
    }
}

तार्किक V3AssertCommon::refsSvaFormal(ध्रुव Ast_ग्रन्थिः* nodep) {
    प्रत्यागम nodep->exists([](ध्रुव Ast_ग्रन्थिः_Var_Ref* refp) {
        ध्रुव AstVar* ध्रुव varp = refp->varp();
        जइ (!varp || !varp->isIO()) प्रत्यागम मिच्छ;
        ध्रुव Ast_ग्रन्थिः* ध्रुव declp = varp->aboveLoopp();
        प्रत्यागम VN_IS(declp, Sequence) || VN_IS(declp, Property);
    });
}

शून्य V3AssertCommon::checkSvaInstanceDelays(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* instp) {
    // A delay still referencing a formal argument of an enclosing sequence or property is
    // checked when that is instanced. Collect first, as checking edits the delays.
    std::vector<AstDelay*> delayps;
    nodep->foreach([&](AstDelay* delayp) {
        जइ (!delayp->isCycleDelay() || refsSvaFormal(delayp->lhsp())) प्रत्यागम;
        जइ (delayp->rhsp() && refsSvaFormal(delayp->rhsp())) प्रत्यागम;
        delayps.प्रक्षिप_back(delayp);
    });
    प्रति (AstDelay* ध्रुव delayp : delayps) checkSvaCycleDelay(delayp, instp);
}

//######################################################################
// AssertDeFutureVisitor
// If any AstFuture, then move all non-future varrefs to be one cycle behind,
// see IEEE 1800-2023 16.9.4.

वर्ग AssertDeFutureVisitor final : सार्वजनिक VNVisitor {
    // STATE - across all visitors
    Ast_ग्रन्थिः_Module* ध्रुव m_modp;  // Module future is underneath
    ध्रुव AstFuture* m_futurep;  // First AstFuture found
    ध्रुव unsigned m_past_अङ्कः;  // Prefix unique number for this module
    std::map<AstVar*, AstVar*> m_delayedVars;  // Old to delayed variable mapping
    // STATE - for current visit position (use VL_RESTORER)
    तार्किक m_inFuture = मिच्छ;  // Inside a future
    तार्किक m_unsupported = मिच्छ;  // Printed unsupported

    // METHODS
    शून्य unsupported(Ast_ग्रन्थिः* nodep) {
        जइ (m_unsupported) प्रत्यागम;
        m_unsupported = सच्च;
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported/illegal: Future value function used with expression with "
                          << nodep->prettyOperatorName());
    }
    // VISITORS
    शून्य visit(AstFuture* nodep) override {
        VL_RESTORER(m_inFuture);
        m_inFuture = सच्च;
        iterateChildren(nodep);
        // Done with the future, this subexpression is current-time
        nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        जइ (nodep->user1SetOnce()) प्रत्यागम;
        जइ (m_inFuture || m_unsupported)
            प्रत्यागम;  // Need user1 set above, don't process when Future is removed
        जइ (nodep->access().isWriteOrRW()) {
            unsupported(nodep);
            प्रत्यागम;
        }
        auto it = m_delayedVars.find(nodep->varp());
        AstVar* outvarp;
        जइ (it == m_delayedVars.end()) {
            Ast_Sen_वृक्षः* ध्रुव sentreep = m_futurep->sentreep();
            AstAlways* ध्रुव alwaysp = नूतन AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS,
                                                     sentreep->clone_वृक्षः(मिच्छ), nullptr};
            m_modp->addStmtsp(alwaysp);
            outvarp = नूतन AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                 "__Vnotfuture" + cvtToStr(m_past_अङ्कः) + "_" + nodep->name(),
                                 nodep->dtypep()};
            m_modp->addStmtsp(outvarp);
            AstVarRef* varRefAWritep = नूतन AstVarRef{nodep->fileline(), outvarp, VAccess::WRITE};
            varRefAWritep->user1(सच्च);
            Ast_ग्रन्थिः_Var_Ref* varRefAReadp = nodep->clone_वृक्षः(मिच्छ);
            varRefAReadp->user1(सच्च);
            Ast_ग्रन्थिः* ध्रुव assp = नूतन AstAssignDly{nodep->fileline(), varRefAWritep, varRefAReadp};
            alwaysp->addStmtsp(assp);
            m_delayedVars.emplace(nodep->varp(), outvarp);
        } अण्णहा {
            outvarp = it->second;
        }
        AstVarRef* newp = नूतन AstVarRef{nodep->fileline(), outvarp, VAccess::READ};
        newp->user1(सच्च);
        UINFO(9, "DeFuture " << nodep << "  becomes " << newp);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_FTask_Ref* nodep) override { unsupported(nodep); }
    शून्य visit(AstMethodCall* nodep) override { unsupported(nodep); }
    शून्य visit(Ast_ग्रन्थिः* nodep) override {
        जइ (!nodep->isPure()) unsupported(nodep);
        iterateChildren(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit AssertDeFutureVisitor(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_Module* modp, unsigned past_अङ्कः)
        : m_modp{modp}
        , m_past_अङ्कः{past_अङ्कः} {
        // See if any Future before we process
        जइ (nodep->forall([&](ध्रुव AstFuture* futurep) -> तार्किक {
                m_futurep = futurep;
                प्रत्यागम मिच्छ;
            }))
            प्रत्यागम;
        // UINFOTREE(9, nodep, "", "defuture-in");
        visit(nodep);  // Nodep may get deleted
        // UINFOTREE(9, nodep, "", "defuture-ou");
    }
    ~AssertDeFutureVisitor() = स्वभाव;
};

// Prepare one default sampled expression.
वर्ग Default_Sampled_मानम्_Visitor final : सार्वजनिक VNVisitor {
    // METHODS
    स्थिर AstConst* newTypeDefault(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        AstConst* ध्रुव constp
            = नूतन AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()};
        जइ (nodep->dtypep()->isFourstate()) constp->अङ्कः().setAllBitsX();
        constp->dtypeFrom(nodep);
        प्रत्यागम constp;
    }

    // VISITORS
    शून्य visit(AstCMethodHard* nodep) override {
        जइ (nodep->method() == VCMethod::EVENT_IS_TRIGGERED) {
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        } अण्णहा {
            iterateChildren(nodep);
        }
    }
    शून्य visit(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        AstVar* ध्रुव varp = nodep->varp();
        जइ (varp->isParam()
            || (!varp->isNet() && varp->lifetime().isStatic() && varp->hasUserInit())) {
            प्रत्यागम;
        }
        nodep->replaceWith(newTypeDefault(nodep));
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    explicit Default_Sampled_मानम्_Visitor(Ast_ग्रन्थिः* nodep) { iterate(nodep); }
};

//######################################################################
// AssertVisitor

वर्ग AssertVisitor final : सार्वजनिक VNVisitor {
    // CONSTANTS
    स्थिर constexpr uint8_t ALL_ASSERT_TYPES
        = std::numeric_limits<std::underlying_type<VAssertType::en>::type>::max();

    // NODE STATE/TYPES
    // Cleared on netlist
    //  AstNode::user1()         -> bool.  True if processed
    //  AstAlways::user2p()      -> std::vector<AstVar*>. Delayed variables via 'm_delayed'
    //  AstNodeVarRef::user2()   -> bool.  True if shouldn't be sampled
    //  AstIf::user2()           -> bool.  True for assertOn() checks inserted in this pass
    ध्रुव VNUser1InUse m_user1InUse;
    ध्रुव VNUser2InUse m_user2InUse;
    AstUser2Allocator<AstAlways, std::vector<AstVar*>> m_delayed;

    // STATE
    Ast_ग्रन्थिः_Module* m_modp = nullptr;  // Last module
    ध्रुव Ast_ग्रन्थिः* m_beginp = nullptr;  // Last AstBegin/AstGenBlock
    unsigned m_monitor_अङ्कः = 0;  // Global $monitor numbering (not per module)
    AstVar* m_monitor_अङ्कः_Varp = nullptr;  // $monitor number variable
    AstVar* m_monitorOffVarp = nullptr;  // $monitoroff variable
    unsigned m_mod_Past_अङ्कः = 0;  // Module past numbering
    unsigned m_mod_Strobe_अङ्कः = 0;  // Module $strobe numbering
    Ast_ग्रन्थिः_Procedure* m_procedurep = nullptr;  // Current procedure
    VDouble0 m_statCover;  // Statistic tracking
    VDouble0 m_statAsNotImm;  // Statistic tracking
    VDouble0 m_statAsImm;  // Statistic tracking
    VDouble0 m_statAsFull;  // Statistic tracking
    VDouble0 m_statPastVars;  // Statistic tracking
    VDouble0 m_statAssertOnCombined;  // Statistic tracking
    VDouble0 m_statAssertOnHoisted;  // Statistic tracking
    तार्किक m_inSampled = मिच्छ;  // True inside a sampled expression
    तार्किक m_inRestrict = मिच्छ;  // True inside restrict assertion
    Ast_ग्रन्थिः* m_passsp = nullptr;  // Current pass statement
    Ast_ग्रन्थिः* m_failsp = nullptr;  // Current fail statement
    Ast_ग्रन्थिः_Cover_Or_Assert* m_assertp = nullptr;  // Current assertion
    AstFinal* m_finalp = nullptr;  // Current final block
    VDouble0 m_statLiftedCaseExprs;  // Count of purified case expressions
    Ast_ग्रन्थिः_FTask* m_ftaskp = nullptr;  // Current function/task
    V3UniqueNames m_caseTempNames{"__VCase"};
    V3UniqueNames m_match_गणनम्_Names{"__VnfaRemainingMatchCount"};  // Match replay counter names
    // Maps from (expression, senTree) to the AstAlways that computes its delayed values.
    std::unordered_map<VNRef<Ast_ग्रन्थिः_व्यञ्जकम्>, std::unordered_map<VNRef<Ast_Sen_वृक्षः>, AstAlways*>>
        m_modExpr2Sen2DelayedAlwaysp;
    // Source sampled-value history is initialized, unlike internal property-lowering history.
    std::unordered_map<VNRef<Ast_ग्रन्थिः_व्यञ्जकम्>, std::unordered_map<VNRef<Ast_Sen_वृक्षः>, AstAlways*>>
        m_modExpr2Sen2DefaultDelayedAlwaysp;
    // Map from delayed-value AstAlways to its last-sampling-tick time variable
    std::unordered_map<ध्रुव AstAlways*, AstVar*> m_delayedAlways2TickTimep;

    // METHODS
    स्थिर ध्रुव वर्ण* assertPassOnQuery(तार्किक vacuous) {
        स्थिर constexpr ध्रुव वर्ण* queries[2]
            = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
        प्रत्यागम queries[vacuous];
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* assertOnCond(File_पङ्क्तिः* fl, VAssertType type,
                                     VAssertDirectiveType directiveType) {
        // cppcheck-suppress missingReturn
        विकल्प (directiveType) {
        पक्ष VAssertDirectiveType::INTRINSIC: प्रत्यागम नूतन AstConst{fl, AstConst::BitTrue{}};
        पक्ष VAssertDirectiveType::VIOLATION_CASE: {
            जइ (v3Global.opt.assertCase()) {
                प्रत्यागम नूतन AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            // If assertions are off, have constant propagation rip them out later
            // This allows syntax errors and such to be detected normally.
            प्रत्यागम नूतन AstConst{fl, AstConst::BitFalse{}};
        }
        पक्ष VAssertDirectiveType::ASSERT:
        पक्ष VAssertDirectiveType::COVER:
        पक्ष VAssertDirectiveType::ASSUME: {
            जइ (v3Global.opt.assertOn()) {
                प्रत्यागम नूतन AstCExpr{
                    fl, AstCExpr::Pure{},
                    V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
            }
            प्रत्यागम नूतन AstConst{fl, AstConst::BitFalse{}};
        }
        पक्ष VAssertDirectiveType::INTERNAL:
        पक्ष VAssertDirectiveType::VIOLATION_IF:
        पक्ष VAssertDirectiveType::RESTRICT: {
            जइ (v3Global.opt.assertOn()) {
                प्रत्यागम नूतन AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            प्रत्यागम नूतन AstConst{fl, AstConst::BitFalse{}};
        }
        }
        VL_UNREACHABLE;
    }

    स्थिर तार्किक isControlled(VAssertDirectiveType directiveType) {
        प्रत्यागम (static_cast<पूर्णाङ्क>(directiveType)
                & (static_cast<पूर्णाङ्क>(VAssertDirectiveType::ASSERT)
                   | static_cast<पूर्णाङ्क>(VAssertDirectiveType::COVER)
                   | static_cast<पूर्णाङ्क>(VAssertDirectiveType::ASSUME)));
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* assertPassOnCond(File_पङ्क्तिः* fl, VAssertType type,
                                         VAssertDirectiveType directiveType, तार्किक vacuous) {
        जइ (!isControlled(directiveType)) प्रत्यागम नूतन AstConst{fl, AstConst::BitTrue{}};
        जइ (!v3Global.opt.assertOn()) प्रत्यागम नूतन AstConst{fl, AstConst::BitFalse{}};
        प्रत्यागम नूतन AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType), 1};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* assertFailOnCond(File_पङ्क्तिः* fl, VAssertType type,
                                         VAssertDirectiveType directiveType) {
        जइ (!isControlled(directiveType)) प्रत्यागम नूतन AstConst{fl, AstConst::BitTrue{}};
        जइ (!v3Global.opt.assertOn()) प्रत्यागम नूतन AstConst{fl, AstConst::BitFalse{}};
        प्रत्यागम नूतन AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType), 1};
    }
    string assertDisplayMessage(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव string& prefix, ध्रुव string& message,
                                VDisplayType severity) {
        जइ (severity == VDisplayType::DT_ERROR || severity == VDisplayType::DT_FATAL) {
            प्रत्यागम ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                    + cvtToStr(nodep->fileline()->lineno()) + ": Assertion failed in %m"
                    + ((message != "") ? ": " : "") + message + "\n");
        }
        प्रत्यागम ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                + cvtToStr(nodep->fileline()->lineno()) + ": %m" + ((message != "") ? ": " : "")
                + message + "\n");
    }
    शून्य replaceDisplay(AstDisplay* nodep, ध्रुव string& prefix) {
        nodep->fmtp()->text(
            assertDisplayMessage(nodep, prefix, nodep->fmtp()->text(), nodep->displayType()));
        nodep->displayType(VDisplayType::DT_WRITE);
        // cppcheck-suppress nullPointer
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव timenewp = नूतन AstTime{nodep->fileline(), m_modp->timeunit()};
        nodep->fmtp()->prependExprsp(timenewp);
        जइ (!nodep->fmtp()->scopeNamep() && nodep->fmtp()->formatScopeTracking()) {
            nodep->fmtp()->scopeNamep(नूतन AstScopeName{nodep->fileline(), सच्च});
        }
    }
    AstSampled* new_Sampled_व्यञ्जकम्(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        प्रत्यागम नूतन AstSampled{nodep->fileline(), nodep, nodep->dtypep(), सच्च};
    }
    AstVarRef* new_Monitor_अङ्कः_Var_Refp(ध्रुव Ast_ग्रन्थिः* nodep, VAccess access) {
        जइ (!m_monitor_अङ्कः_Varp) {
            m_monitor_अङ्कः_Varp = नूतन AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorNum",
                                          nodep->findUInt64DType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitor_अङ्कः_Varp);
        }
        AstVarRef* ध्रुव varrefp = नूतन AstVarRef{nodep->fileline(), m_monitor_अङ्कः_Varp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        प्रत्यागम varrefp;
    }
    AstVarRef* newMonitorOffVarRefp(ध्रुव Ast_ग्रन्थिः* nodep, VAccess access) {
        जइ (!m_monitorOffVarp) {
            m_monitorOffVarp = नूतन AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorOff",
                                          nodep->findBitDType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitorOffVarp);
        }
        AstVarRef* ध्रुव varrefp = नूतन AstVarRef{nodep->fileline(), m_monitorOffVarp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        प्रत्यागम varrefp;
    }
    स्थिर AstIf* newIfAssertOn(Ast_ग्रन्थिः* bodyp, VAssertDirectiveType directiveType,
                                VAssertType type = VAssertType::INTERNAL) {
        // Add an internal if to check assertions are on.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_पङ्क्तिः* ध्रुव fl = bodyp->fileline();

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = assertOnCond(fl, type, directiveType);
        AstIf* ध्रुव newp = नूतन AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(सच्च);  // To avoid LATCH warning
        newp->user2(सच्च);  // Mark as an assertOn() check
        प्रत्यागम newp;
    }
    स्थिर Ast_ग्रन्थिः_Stmt* newIfAssertPassOn(Ast_ग्रन्थिः* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type, तार्किक vacuous) {
        // Add an internal if to check assertion passOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_पङ्क्तिः* ध्रुव fl = bodyp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = assertPassOnCond(fl, type, directiveType, vacuous);
        Ast_ग्रन्थिः_If* ध्रुव newp = नूतन AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(सच्च);  // To avoid LATCH warning
        newp->user1(सच्च);  // Don't assert/cover this if
        newp->user2(सच्च);  // Mark as an assertOn() check
        प्रत्यागम newp;
    }
    स्थिर Ast_ग्रन्थिः_Stmt* newIfAssertFailOn(Ast_ग्रन्थिः* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type) {
        // Add an internal if to check assertion failOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_पङ्क्तिः* ध्रुव fl = bodyp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = assertFailOnCond(fl, type, directiveType);
        Ast_ग्रन्थिः_If* ध्रुव newp = नूतन AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(सच्च);  // To avoid LATCH warning
        newp->user1(सच्च);  // Don't assert/cover this if
        newp->user2(सच्च);  // Mark as an assertOn() check
        प्रत्यागम newp;
    }

    स्थिर AstIf* assertCond(ध्रुव Ast_ग्रन्थिः_Cover_Or_Assert* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* propp,
                             Ast_ग्रन्थिः* passsp, Ast_ग्रन्थिः* failsp) {
        AstIf* ध्रुव ifp = नूतन AstIf{nodep->fileline(), propp, passsp, failsp};
        // It's more LIKELY that we'll take the nullptr if clause
        // than the sim-killing else clause:
        ifp->branchPred(VBranchPred::BP_LIKELY);
        ifp->isBoundsCheck(सच्च);  // To avoid LATCH warning
        प्रत्यागम ifp;
    }

    Ast_ग्रन्थिः_Stmt* assertBody(ध्रुव Ast_ग्रन्थिः_Cover_Or_Assert* nodep, Ast_ग्रन्थिः* propp, Ast_ग्रन्थिः* passsp,
                            Ast_ग्रन्थिः* failsp) {
        जइ (AstPExpr* ध्रुव pexprp = VN_CAST(propp, PExpr)) {
            जइ (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NOTIMING, "This property expression requires --timing");
                VL_DO_DANGLING(प्रक्षिप_Deletep(pexprp), pexprp);
                प्रत्यागम नूतन AstBegin{nodep->fileline(), "", nullptr, मिच्छ};
            }
            AstFork* ध्रुव forkp = नूतन AstFork{nodep->fileline(), VJoinType::JOIN_NONE};
            forkp->addForksp(pexprp->bodyp()->unlinkFrBack());
            जइ (Ast_ग्रन्थिः_Stmt* ध्रुव finalp = pexprp->finalp()) {
                जइ (!m_finalp) {
                    m_finalp = नूतन AstFinal{m_modp->fileline(), finalp->unlinkFrBack()};
                    m_modp->addStmtsp(m_finalp);
                } अण्णहा {
                    m_finalp->addStmtsp(finalp->unlinkFrBack());
                }
            }
            VL_DO_DANGLING2(प्रक्षिप_Deletep(pexprp), pexprp, propp);
            प्रत्यागम forkp;
        }

        प्रत्यागम assertCond(nodep, VN_AS(propp, ग्रन्थिः_व्यञ्जकम्), passsp, failsp);
    }

    Ast_ग्रन्थिः_Stmt* newFireAssertUnchecked(ध्रुव Ast_ग्रन्थिः_Stmt* nodep, ध्रुव string& message,
                                        Ast_ग्रन्थिः_व्यञ्जकम्* exprsp = nullptr) {
        // Like newFireAssert() but omits the asserts-on check
        AstDisplay* ध्रुव dispp
            = नूतन AstDisplay{nodep->fileline(), VDisplayType::DT_ERROR, message, nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        Ast_ग्रन्थिः_Stmt* ध्रुव bodysp = dispp;
        replaceDisplay(dispp, "%%Error");  // Convert to standard DISPLAY format
        जइ (exprsp) dispp->fmtp()->exprsp()->addNext(exprsp);
        जइ (v3Global.opt.stopFail()) bodysp->addNext(नूतन AstStop{nodep->fileline(), मिच्छ});
        प्रत्यागम bodysp;
    }

    Ast_ग्रन्थिः_Stmt* newFireAssert(ध्रुव Ast_ग्रन्थिः_Stmt* nodep, VAssertDirectiveType directiveType,
                               VAssertType assertType, ध्रुव string& message,
                               Ast_ग्रन्थिः_व्यञ्जकम्* exprsp = nullptr) {
        Ast_ग्रन्थिः_Stmt* bodysp = newFireAssertUnchecked(nodep, message, exprsp);
        bodysp = newIfAssertOn(bodysp, directiveType, assertType);
        प्रत्यागम bodysp;
    }

    AstVar* createDelayedVar(ध्रुव std::string& name, AstAlways* alwaysp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp,
                             Ast_ग्रन्थिः_व्यञ्जकम्* initp) {
        File_पङ्क्तिः* ध्रुव flp = exprp->fileline();
        AstVar* ध्रुव varp = नूतन AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        // TODO: this lifetime seems nonsene (can't have NBAs to automatics), but is as before
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->addStmtsp(varp);
        // Before enough clock ticks, $past returns the expression's default sampled value.
        जइ (initp) {
            m_modp->addStmtsp(नूतन AstInitialStatic{
                flp, नूतन AstAssign{flp, नूतन AstVarRef{flp, varp, VAccess::WRITE}, initp}});
        }
        ++m_statPastVars;
        // Actually set the delayed value
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = नूतन AstVarRef{flp, varp, VAccess::WRITE};
        AstAssignDly* ध्रुव assignp = नूतन AstAssignDly{flp, lhsp, exprp};
        alwaysp->prependStmtsp(assignp);
        प्रत्यागम varp;
    }

    AstAlways* getDelayedAlways(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* initp, Ast_Sen_वृक्षः* senTreep) {
        auto& expr2Sen2Alwaysr
            = initp ? m_modExpr2Sen2DefaultDelayedAlwaysp : m_modExpr2Sen2DelayedAlwaysp;
        AstAlways*& alwayspr = expr2Sen2Alwaysr[*exprp][*senTreep];
        जइ (!alwayspr) {
            File_पङ्क्तिः* ध्रुव flp = exprp->fileline();
            // Create the always block that computes the delayed values
            alwayspr = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, nullptr};
            m_modp->addStmtsp(alwayspr);
            // Create the once-delayed variable
            ध्रुव std::string name = "_Vpast_" + cvtToStr(m_mod_Past_अङ्कः++) + "_1";
            AstVar* ध्रुव varp = createDelayedVar(name, alwayspr, exprp, initp);
            // Add it to delayed variable vector
            m_delayed(alwayspr).emplace_back(varp);
        } अण्णहा {
            // Reusing exiting, not needed
            VL_DO_DANGLING(प्रक्षिप_Deletep(exprp), exprp);
            जइ (initp) VL_DO_DANGLING(प्रक्षिप_Deletep(initp), initp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(senTreep), senTreep);
        }
        प्रत्यागम alwayspr;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* get_Past_मानम्(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* initp, Ast_Sen_वृक्षः* senTreep,
                              uint32_t ticks) {
        UASSERT_OBJ(ticks > 0, exprp, "Delay must be > 0");
        File_पङ्क्तिः* ध्रुव flp = exprp->fileline();
        AstAlways* ध्रुव alwaysp
            = getDelayedAlways(exprp, initp ? initp->clone_वृक्षः_Pure(मिच्छ) : nullptr, senTreep);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव resultp = past_मानम्_Ref(alwaysp, flp, ticks, initp);
        जइ (initp) VL_DO_DANGLING(प्रक्षिप_Deletep(initp), initp);
        प्रत्यागम resultp;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* past_मानम्_Ref(AstAlways* alwaysp, File_पङ्क्तिः* flp, uint32_t ticks,
                              Ast_ग्रन्थिः_व्यञ्जकम्* initp) {
        std::vector<AstVar*>& delayedr = m_delayed(alwaysp);
        // Ensure the required delay exists
        जाव (delayedr.size() < ticks) {
            AstVar* ध्रुव firstp = delayedr.front();
            File_पङ्क्तिः* ध्रुव varFlp = firstp->fileline();
            // Create once more delayed value
            std::string name = firstp->name();
            name.resize(name.size() - 1);
            name += std::to_string(delayedr.size() + 1);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव prevp = नूतन AstVarRef{varFlp, delayedr.back(), VAccess::READ};
            AstVar* ध्रुव varp = createDelayedVar(name, alwaysp, prevp,
                                                  initp ? initp->clone_वृक्षः_Pure(मिच्छ) : nullptr);
            // Add it to delayed variable vector
            delayedr.emplace_back(varp);
        }
        // Return a reference to the appropriately delayed variable
        प्रत्यागम नूतन AstVarRef{flp, delayedr.at(ticks - 1), VAccess::READ};
    }

    // Time of the pipeline's last sampling tick, for end-of-simulation readers
    AstVar* getPastTickTimeVar(AstAlways* alwaysp) {
        AstVar*& varpr = m_delayedAlways2TickTimep[alwaysp];
        जइ (!varpr) {
            File_पङ्क्तिः* ध्रुव flp = alwaysp->fileline();
            varpr = नूतन AstVar{flp, VVarType::MODULETEMP,
                               "_Vpast_" + cvtToStr(m_mod_Past_अङ्कः++) + "_t",
                               m_modp->findUInt64DType()};
            m_modp->addStmtsp(varpr);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव timep = नूतन AstCExpr{flp, "vlSymsp->_vm_contextp__->time()", 64};
            alwaysp->addStmtsp(
                नूतन AstAssign{flp, नूतन AstVarRef{flp, varpr, VAccess::WRITE}, timep});
        }
        प्रत्यागम varpr;
    }

    शून्य visitAssertionIterate(Ast_ग्रन्थिः_Cover_Or_Assert* nodep, Ast_ग्रन्थिः* failsp) {
        जइ (m_beginp && nodep->name() == "") nodep->name(m_beginp->name());

        { AssertDeFutureVisitor{nodep->propp(), m_modp, m_mod_Past_अङ्कः++}; }

        iterateAndNextNull(nodep->sentreep());
        जइ (AstAssert* ध्रुव assertp = VN_CAST(nodep, Assert)) {
            iterateAndNextNull(assertp->failsp());
        } अण्णहा जइ (AstAssertIntrinsic* ध्रुव assertp = VN_CAST(nodep, AssertIntrinsic)) {
            iterateAndNextNull(assertp->failsp());
        } अण्णहा जइ (AstCover* ध्रुव coverp = VN_CAST(nodep, Cover)) {
            iterateAndNextNull(coverp->coverincsp());
        } अण्णहा जइ (!VN_IS(nodep, Restrict)) {
            nodep->v3fatalSrc("Unhandled assert type");
        }
        iterateAndNextNull(nodep->passsp());
        Ast_Sen_वृक्षः* sentreep = nodep->sentreep();
        जइ (nodep->immediate()) {
            UASSERT_OBJ(!sentreep, nodep, "Immediate assertions don't have sensitivity");
        } अण्णहा {
            UASSERT_OBJ(sentreep, nodep, "Concurrent assertions must have sensitivity");
            // Explicit inline clock differs from the enclosing always: hoist
            // and warn. To support this need queue of asserts to activate.
            जइ (m_procedurep && !nodep->senFromAlways()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Procedural concurrent assertion with"
                              " clocking event inside always (IEEE 1800-2023 16.14.6)");
                static_cast<Ast_ग्रन्थिः*>(m_procedurep)->addNext(nodep->unlinkFrBack());
                प्रत्यागम;  // Later iterate will pick up
            }
            sentreep->unlinkFrBack();
            जइ (m_procedurep) { VL_DO_DANGLING(प्रक्षिप_Deletep(sentreep), sentreep); }
        }
        //
        ध्रुव string& message = nodep->name();
        Ast_ग्रन्थिः* passsp = nodep->passsp();
        जइ (passsp) passsp->unlinkFrBackWithNext();
        जइ (AstAssert* ध्रुव assertp = VN_CAST(nodep, Assert)) {
            failsp = assertp->failsp();
        } अण्णहा जइ (AstAssertIntrinsic* ध्रुव assertp = VN_CAST(nodep, AssertIntrinsic)) {
            failsp = assertp->failsp();
        }
        जइ (failsp) failsp->unlinkFrBackWithNext();

        तार्किक selfDestruct = मिच्छ;
        तार्किक passspGated = मिच्छ;
        ध्रुव AstCover* ध्रुव coverp = VN_CAST(nodep, Cover);
        // A sequence event control is not an assertion directive; no assertion control
        ध्रुव तार्किक seqEvent = coverp && coverp->isSeqEvent();
        जइ (coverp) {
            ++m_statCover;
            जइ (seqEvent) {
                // Keep the event-fire action, with no coverage bucket
            } अण्णहा जइ (!v3Global.opt.coverageUser()) {
                selfDestruct = सच्च;
            } अण्णहा {
                // V3Coverage assigned us a bucket to increment.
                AstCoverInc* ध्रुव covincp = VN_AS(coverp->coverincsp(), CoverInc);
                UASSERT_OBJ(covincp, coverp, "Missing AstCoverInc under assertion");
                covincp->unlinkFrBackWithNext();  // next() might have  AstAssign for trace
                जइ (message != "") covincp->declp()->comment(message);
                जइ (passsp) {
                    passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                               /*vacuous=*/मिच्छ);
                    passspGated = सच्च;
                    passsp = Ast_ग्रन्थिः::addNext<Ast_ग्रन्थिः, Ast_ग्रन्थिः>(covincp, passsp);
                } अण्णहा {
                    passsp = covincp;
                }
            }
        } अण्णहा जइ (VN_IS(nodep, Assert) || VN_IS(nodep, AssertIntrinsic)) {
            जइ (nodep->immediate()) {
                ++m_statAsImm;
            } अण्णहा {
                ++m_statAsNotImm;
            }
            जइ (!passsp && !failsp)
                failsp = newFireAssertUnchecked(
                    nodep, VN_IS(nodep, AssertIntrinsic) ? "'$cast' failed." : "'assert' failed.");
        } अण्णहा {
            nodep->v3fatalSrc("Unknown node type");
        }

        VL_RESTORER(m_passsp);
        VL_RESTORER(m_failsp);
        VL_RESTORER(m_assertp);
        m_passsp = passsp;
        m_failsp = failsp;
        m_assertp = nodep;
        iterate(nodep->propp());

        Ast_ग्रन्थिः* propExprp;
        Ast_ग्रन्थिः_व्यञ्जकम्* disablep = nullptr;
        Ast_ग्रन्थिः_व्यञ्जकम्* matchCountp = nullptr;
        जइ (AstPropSpec* ध्रुव specp = VN_CAST(nodep->propp(), PropSpec)) {
            propExprp = specp->propp()->unlinkFrBack();
            जइ (specp->disablep()) disablep = specp->disablep()->unlinkFrBack();
            matchCountp = specp->matchCountp();
        } अण्णहा {
            propExprp = nodep->propp()->unlinkFrBack();
        }
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        तार्किक passspAlreadyGated = मिच्छ;
        जइ (passsp && VN_IS(passsp, If)) passspAlreadyGated = VN_AS(passsp, If)->user1();
        जइ (passsp && !passspGated && !passspAlreadyGated && !VN_IS(propExprp, PExpr)
            && !seqEvent) {
            passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                       /*vacuous=*/मिच्छ);
        }
        जइ (failsp && !VN_IS(propExprp, PExpr)) {
            failsp = newIfAssertFailOn(failsp, nodep->directive(), nodep->userType());
        }
        जइ (coverp && matchCountp && passsp) {
            // Convert the match count into a loop that decrements a temporary variable until it
            // reaches zero.
            matchCountp->unlinkFrBack();
            AstVar* ध्रुव remainingp = नूतन AstVar{
                flp, VVarType::MODULETEMP, m_match_गणनम्_Names.get(""), matchCountp->dtypep()};
            remainingp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remainingp);
            Ast_ग्रन्थिः* ध्रुव replaysp
                = नूतन AstAssign{flp, नूतन AstVarRef{flp, remainingp, VAccess::WRITE}, matchCountp};
            AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
            loopp->addStmtsp(
                नूतन AstLoopTest{flp, loopp, नूतन AstVarRef{flp, remainingp, VAccess::READ}});
            loopp->addStmtsp(passsp);
            loopp->addStmtsp(
                नूतन AstAssign{flp, नूतन AstVarRef{flp, remainingp, VAccess::WRITE},
                              नूतन AstSub{flp, नूतन AstVarRef{flp, remainingp, VAccess::READ},
                                         नूतन AstConst{flp, AstConst::Widthed_मानम्{},
                                                      remainingp->dtypep()->width(), 1}}});
            replaysp->addNext(loopp);
            passsp = replaysp;
        }
        Ast_ग्रन्थिः* bodysp = assertBody(nodep, propExprp, passsp, failsp);
        जइ (disablep) bodysp = नूतन AstIf{flp, नूतन AstLogNot{flp, disablep}, bodysp};
        // Add assertOn check last, for better combining
        जइ (!seqEvent) bodysp = newIfAssertOn(bodysp, nodep->directive(), nodep->userType());
        जइ (sentreep) bodysp = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, sentreep, bodysp};

        जइ (passsp && !passsp->backp()) VL_DO_DANGLING(प्रक्षिप_Deletep(passsp), passsp);
        जइ (failsp && !failsp->backp()) VL_DO_DANGLING(प्रक्षिप_Deletep(failsp), failsp);

        // Install it
        जइ (selfDestruct) {
            // Delete it after making the tree.  This way we can tell the user
            // if it wasn't constructed nicely or has other errors without needing --coverage.
            VL_DO_DANGLING(bodysp->delete_वृक्षः(), bodysp);
            nodep->unlinkFrBack();
        } अण्णहा {
            nodep->replaceWith(bodysp);
        }
        // Bye
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    तार्किक isEmptyStmt(Ast_ग्रन्थिः* nodep) {
        जइ (!nodep) प्रत्यागम सच्च;
        जइ (AstBegin* ध्रुव beginp = VN_CAST(nodep, Begin)) {
            जइ (beginp->declsp()) प्रत्यागम मिच्छ;
            प्रति (Ast_ग्रन्थिः* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                जइ (!isEmptyStmt(stmtp)) प्रत्यागम मिच्छ;
            }
            प्रत्यागम सच्च;
        }
        प्रत्यागम मिच्छ;
    }

    // VISITORS
    शून्य visit(AstIf* nodep) override {
        जइ (!nodep->user1SetOnce()) {
            जइ (nodep->uniquePragma() || nodep->unique0Pragma()) {
                ध्रुव Ast_ग्रन्थिः_If* ifp = nodep;
                Ast_ग्रन्थिः_व्यञ्जकम्* propp = nullptr;
                तार्किक hasDefaultElse = मिच्छ;
                करेहि {
                    // If this statement ends with 'else if', then nextIf will point to the
                    // nextIf statement.  Otherwise it will be null.
                    ध्रुव Ast_ग्रन्थिः_If* ध्रुव nextifp = dynamic_cast<Ast_ग्रन्थिः_If*>(ifp->elsesp());
                    iterateAndNextNull(ifp->condp());

                    // Recurse into the true case.
                    iterateAndNextNull(ifp->thensp());

                    // If the last else is not an else if, recurse into that too.
                    जइ (ifp->elsesp() && !nextifp) {  //
                        iterateAndNextNull(ifp->elsesp());
                    }

                    // Build a bitmask of the true predicates
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव predp = ifp->condp()->clone_वृक्षः_Pure(मिच्छ);
                    जइ (propp) {
                        propp = नूतन AstConcat{nodep->fileline(), predp, propp};
                    } अण्णहा {
                        propp = predp;
                    }

                    // Record if this ends with an 'else' that does not have an if
                    जइ (ifp->elsesp() && !nextifp) hasDefaultElse = सच्च;

                    ifp = nextifp;
                } जाव (ifp);

                ध्रुव तार्किक allow_none = nodep->unique0Pragma();

                // Empty case means no property
                जइ (!propp) propp = नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}};

                // Note: if this ends with an 'else', then we don't need to validate that one of
                // the predicates evaluates to true.
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव ohot
                    = ((allow_none || hasDefaultElse)
                           ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstOneHot0{nodep->fileline(), propp})
                           : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstOneHot{nodep->fileline(), propp}));
                ध्रुव VAssertType assertType
                    = nodep->uniquePragma() ? VAssertType::UNIQUE : VAssertType::UNIQUE0;
                // Move, not clone, as the branches might declare referenced variables
                VNRelinker relinker;
                nodep->unlinkFrBack(&relinker);
                AstIf* ध्रुव checkifp
                    = नूतन AstIf{nodep->fileline(), नूतन AstLogNot{nodep->fileline(), ohot},
                                newFireAssert(nodep, VAssertDirectiveType::VIOLATION_IF,
                                              assertType, "'unique if' statement violated"),
                                nodep};
                checkifp->isBoundsCheck(सच्च);  // To avoid LATCH warning
                checkifp->branchPred(VBranchPred::BP_UNLIKELY);
                relinker.relink(checkifp);
                प्रत्यागम;
            }
        }

        iterateChildren(nodep);

        जइ (nodep->user2()) {
            // Combine consecutive assertOn checks if possible
            जइ (AstIf* ध्रुव backp = VN_CAST(nodep->backp(), If)) {
                जइ (backp->nextp() == nodep  //
                    && backp->user2()  //
                    && backp->condp()->same_वृक्षः(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    backp->addThensp(nodep->thensp()->unlinkFrBackWithNext());
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    प्रत्यागम;
                }
            }
            // Combine nested assertOn checks if possible
            जइ (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* ध्रुव checkp = VN_CAST(nodep->thensp(), If);
                जइ (checkp  //
                    && checkp->user2()  //
                    && checkp->condp()->same_वृक्षः(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    VL_DO_DANGLING(प्रक्षिप_Deletep(checkp->unlinkFrBack()), checkp);
                    प्रत्यागम;
                }
            }
            प्रत्यागम;
        }

        // Swap assertOn check with single statement 'if' statement to bubble up for combining
        // Note we can't just swap the conditions as they two Ifs have different flags,
        // so swapping the Ifs themeselves then swapping back the bodies.
        जइ (nodep->condp()->isPure()) {
            जइ (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* ध्रुव checkp = VN_CAST(nodep->thensp(), If);
                जइ (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    प्रत्यागम;
                }
            }
            जइ (nodep->elsesp() && !nodep->elsesp()->nextp() && isEmptyStmt(nodep->thensp())) {
                AstIf* ध्रुव checkp = VN_CAST(nodep->elsesp(), If);
                जइ (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addElsesp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    प्रत्यागम;
                }
            }
        }
    }

    //========== Case assertions
    शून्य visit(AstCase* nodep) override {
        // Introduce temporary variable for AstCase if needed - it is done here and not in V3Case
        // because this phase is before V3Scope and V3Case is not. Doing it before V3Scope ensures
        // that V3Scope will take care of a scope creation
        // We also need to do it before V3Begin, co that pragmas like `unique0` also work correctly
        जइ (!nodep->exprp()->isPure()) {
            ++m_statLiftedCaseExprs;
            File_पङ्क्तिः* ध्रुव fl = nodep->exprp()->fileline();
            AstVar* ध्रुव varp = नूतन AstVar{fl, VVarType::BLOCKTEMP, m_caseTempNames.get(nodep),
                                            nodep->exprp()->dtypep()};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव origp = nodep->exprp()->unlinkFrBack();
            nodep->addHereThisAsNext(
                नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, origp});
            nodep->exprp(नूतन AstVarRef{fl, varp, VAccess::READ});
            जइ (m_ftaskp) {
                varp->funcLocal(सच्च);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } अण्णहा {
                m_modp->prependStmtsp(varp);
            }
            VIsCached::clear_Cache_वृक्षः();
        }
        iterateChildren(nodep);
        जइ (!nodep->user1SetOnce()) {
            तार्किक has_default = मिच्छ;
            प्रति (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                जइ (itemp->isDefault()) has_default = सच्च;
            }
            ध्रुव Ast_ग्रन्थिः_DType* व्यञ्जकम्_Dtypep = nodep->exprp()->dtypep()->skipRefp();

            VAssertType assertType = VAssertType::INTERNAL;
            जइ (nodep->priorityPragma()) {
                assertType = VAssertType::PRIORITY;
            } अण्णहा जइ (nodep->uniquePragma()) {
                assertType = VAssertType::UNIQUE;
            } अण्णहा जइ (nodep->unique0Pragma()) {
                assertType = VAssertType::UNIQUE0;
            }

            string valFmt;
            जइ (व्यञ्जकम्_Dtypep->isIntegralOrPacked())
                valFmt = " for '" + cvtToStr(व्यञ्जकम्_Dtypep->widthMin()) + "'h%X'";
            जइ (nodep->fullPragma() || nodep->priorityPragma()) {
                // Need to add a default if there isn't one already
                ++m_statAsFull;
                जइ (!has_default) {
                    nodep->addItemsp(नूतन AstCaseItem{
                        nodep->fileline(), nullptr /*DEFAULT*/,
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      nodep->pragmaString() + ", but non-match found" + valFmt,
                                      valFmt.empty() ? nullptr
                                                     : nodep->exprp()->clone_वृक्षः_Pure(मिच्छ))});
                }
            }
            जइ (nodep->parallelPragma() || nodep->uniquePragma() || nodep->unique0Pragma()) {
                // Need to check that one, and only one of the case items match at any moment
                // If there's a default, we allow none to match, else exactly one must match
                ++m_statAsFull;
                जइ (!has_default && !nodep->itemsp()) {
                    // Not parallel, but harmlessly so.
                } अण्णहा {
                    Ast_ग्रन्थिः_व्यञ्जकम्* propp = nullptr;
                    प्रति (AstCaseItem* itemp = nodep->itemsp(); itemp;
                         itemp = VN_AS(itemp->nextp(), CaseItem)) {
                        Ast_ग्रन्थिः_व्यञ्जकम्* itembitp = nullptr;
                        प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* icondp = itemp->condsp(); icondp;
                             icondp = VN_AS(icondp->nextp(), ग्रन्थिः_व्यञ्जकम्)) {
                            Ast_ग्रन्थिः_व्यञ्जकम्* onep;
                            जइ (AstInsideRange* ध्रुव rcondp = VN_CAST(icondp, InsideRange)) {
                                onep = rcondp->newAndFromInside(
                                    nodep->exprp()->clone_वृक्षः_Pure(सच्च),
                                    rcondp->lhsp()->clone_वृक्षः_Pure(सच्च),
                                    rcondp->rhsp()->clone_वृक्षः_Pure(सच्च));
                            } अण्णहा जइ (nodep->casex() || nodep->casez() || nodep->caseInside()) {
                                onep = AstEqWild::newTyped(itemp->fileline(),
                                                           nodep->exprp()->clone_वृक्षः_Pure(मिच्छ),
                                                           icondp->clone_वृक्षः_Pure(मिच्छ));
                            } अण्णहा {
                                onep = AstEq::newTyped(icondp->fileline(),
                                                       nodep->exprp()->clone_वृक्षः_Pure(मिच्छ),
                                                       icondp->clone_वृक्षः_Pure(मिच्छ));
                            }
                            // OR together all conditions within the same case item
                            जइ (onep) {
                                जइ (itembitp) {
                                    itembitp = नूतन AstOr{icondp->fileline(), onep, itembitp};
                                } अण्णहा {
                                    itembitp = onep;
                                }
                            }
                        }
                        जइ (itembitp) {
                            जइ (propp) {
                                propp = नूतन AstConcat{itemp->fileline(), itembitp, propp};
                            } अण्णहा {
                                propp = itembitp;
                            }
                        }
                    }
                    // Empty case means no property
                    जइ (!propp) propp = नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    ध्रुव तार्किक allow_none = has_default || nodep->unique0Pragma();
                    // The following assertion looks as below.
                    // if (!$onehot(propp)) begin
                    //     if (propp == '0) begin if (!allow_none) $error("none match"); end
                    //     else $error("multiple match");
                    // end
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव ohot = नूतन AstOneHot{nodep->fileline(), propp};
                    AstConst* ध्रुव zero = नूतन AstConst{
                        nodep->fileline(), AstConst::Widthed_मानम्{}, propp->width(), 0};
                    AstIf* ध्रुव ohotIfp
                        = नूतन AstIf{nodep->fileline(), नूतन AstLogNot{nodep->fileline(), ohot}};
                    AstIf* ध्रुव zeroIfp = नूतन AstIf{
                        nodep->fileline(),
                        नूतन AstEq{nodep->fileline(), propp->clone_वृक्षः_Pure(मिच्छ), zero}};
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = nodep->exprp();
                    ध्रुव string pragmaStr = nodep->pragmaString();
                    जइ (!allow_none)
                        zeroIfp->addThensp(
                            newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                          pragmaStr + ", but none matched" + valFmt,
                                          valFmt.empty() ? nullptr : exprp->clone_वृक्षः_Pure(मिच्छ)));
                    zeroIfp->addElsesp(
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      pragmaStr + ", but multiple matches found" + valFmt,
                                      valFmt.empty() ? nullptr : exprp->clone_वृक्षः_Pure(मिच्छ)));
                    ohotIfp->addThensp(zeroIfp);
                    ohotIfp->isBoundsCheck(सच्च);  // To avoid LATCH warning
                    ohotIfp->branchPred(VBranchPred::BP_UNLIKELY);
                    nodep->addNotParallelp(ohotIfp);
                }
            }
        }
    }

    शून्य visit(AstFuture* nodep) override {
        nodep->v3error("Future sampled value function called outside property or sequence "
                       "expression (IEEE 16.9.4)");
        nodep->replaceWith(नूतन AstConst{nodep->fileline(), 0});
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    //========== Past
    शून्य visit(AstPast* nodep) override {
        जइ (!nodep->propertyTiming()) {
            nodep->initp(nodep->exprp()->clone_वृक्षः_Pure(मिच्छ));
            { Default_Sampled_मानम्_Visitor{nodep->initp()}; }
        }
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव initp = nodep->initp() ? nodep->initp()->unlinkFrBack() : nullptr;
        iterateChildren(nodep);
        uint32_t ticks = 1;
        जइ (nodep->ticksp()) {
            UASSERT_OBJ(VN_IS(nodep->ticksp(), Const), nodep,
                        "Expected constant ticks, checked in V3Width");
            ticks = VN_AS(nodep->ticksp(), Const)->toUInt();
        }
        UASSERT_OBJ(ticks >= 1, nodep, "0 tick should have been checked in V3Width");
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = new_Sampled_व्यञ्जकम्(nodep->exprp()->unlinkFrBack());
        Ast_Sen_वृक्षः* ध्रुव senTreep = nodep->sentreep()->unlinkFrBack();
        Ast_ग्रन्थिः_व्यञ्जकम्* inp = nullptr;
        // Deliberately emitted for every ftask $past; calls outside final take the normal stage
        जइ (VN_IS(m_procedurep, Final) || m_ftaskp) {
            // A sampling-tick finish reads one stage deeper (IEEE 1800-2023 16.9.3).
            File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
            AstAlways* ध्रुव alwaysp
                = getDelayedAlways(exprp, initp ? initp->clone_वृक्षः_Pure(मिच्छ) : nullptr, senTreep);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव betweenTicksp = past_मानम्_Ref(alwaysp, flp, ticks, initp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव onTickp = past_मानम्_Ref(alwaysp, flp, ticks + 1, initp);
            जइ (initp) VL_DO_DANGLING(प्रक्षिप_Deletep(initp), initp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव onTickCondp = नूतन AstLogAnd{
                flp, नूतन AstCExpr{flp, "vlSymsp->_vm_contextp__->executingFinal()", 1},
                नूतन AstEq{flp, नूतन AstVarRef{flp, getPastTickTimeVar(alwaysp), VAccess::READ},
                          नूतन AstCExpr{flp, "vlSymsp->_vm_contextp__->finishPendingTime()", 64}}};
            AstCond* ध्रुव condp = नूतन AstCond{flp, onTickCondp, onTickp, betweenTicksp};
            condp->dtypeFrom(onTickp);
            inp = condp;
        } अण्णहा {
            inp = get_Past_मानम्(exprp, initp, senTreep, ticks);
        }
        nodep->replaceWith(inp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    //========== Move $sampled down to read-only variables
    शून्य visit(AstSampled* nodep) override {
        जइ (nodep->user1()) प्रत्यागम;
        VL_RESTORER(m_inSampled);
        {
            m_inSampled = सच्च;
            iterateChildren(nodep);
        }
        जइ (nodep->exprp()) {
            nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        } अण्णहा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        जइ (m_inSampled && !nodep->user2() && !nodep->varp()->isTemp()) {
            जइ (!nodep->access().isReadOnly()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Write to variable in sampled expression");
            } अण्णहा {
                VNRelinker relinkHandle;
                nodep->unlinkFrBack(&relinkHandle);
                AstSampled* ध्रुव newp = new_Sampled_व्यञ्जकम्(nodep);
                relinkHandle.relink(newp);
                newp->user1(1);
                v3Global.setHasSampled();
            }
        }
    }
    // Don't sample sensitivities
    शून्य visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSampled);
        m_inSampled = मिच्छ;
        iterateChildren(nodep);
    }
    शून्य visit(AstPExprClause* nodep) override {
        Ast_ग्रन्थिः* stmtsp = nullptr;
        जइ (nodep->pass() && m_passsp) {
            // Cover adds COVERINC by AstNode::addNext, thus need to clone next too.
            stmtsp = m_passsp->clone_वृक्षः(सच्च);
            stmtsp = newIfAssertPassOn(stmtsp, m_assertp->directive(), m_assertp->userType(),
                                       nodep->vacuous());
        } अण्णहा जइ (!nodep->pass() && m_failsp) {
            stmtsp = m_failsp->clone_वृक्षः(सच्च);
            stmtsp = newIfAssertFailOn(stmtsp, m_assertp->directive(), m_assertp->userType());
        }
        जइ (stmtsp) {
            stmtsp->foreachAndNext([](Ast_ग्रन्थिः_Var_Ref* ध्रुव refp) {
                // References inside action blocks shouldn't be implicitly sampled
                // m_passsp/m_failsp have been already visited once and refs explicitly sampled
                // are handled already
                refp->user2(1);
            });
            nodep->replaceWith(stmtsp);
        } अण्णहा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstPExpr* nodep) override {
        जइ (m_inRestrict) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
        } अण्णहा {
            iterateChildren(nodep);
        }
    }

    //========== Statements
    शून्य visit(AstDisplay* nodep) override {
        iterateChildren(nodep);
        // Replace the special types with standard text
        जइ (nodep->displayType() == VDisplayType::DT_INFO) {
            replaceDisplay(nodep, "-Info");
        } अण्णहा जइ (nodep->displayType() == VDisplayType::DT_WARNING) {
            replaceDisplay(nodep, "%%Warning");
        } अण्णहा जइ (nodep->displayType() == VDisplayType::DT_ERROR) {
            replaceDisplay(nodep, "%%Error");
        } अण्णहा जइ (nodep->displayType() == VDisplayType::DT_FATAL) {
            replaceDisplay(nodep, "%%Fatal");
        } अण्णहा जइ (nodep->displayType() == VDisplayType::DT_MONITOR) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            File_पङ्क्तिः* ध्रुव fl = nodep->fileline();

            AstSenItem* monSenItemsp = nullptr;
            जइ (Ast_ग्रन्थिः* ध्रुव monExprsp = nodep->fmtp()->exprsp()) {
                monExprsp->foreachAndNext([&](AstVarRef* varrefp) {
                    AstSenItem* ध्रुव senItemp
                        = नूतन AstSenItem{fl, VEdgeType::ET_CHANGED,
                                         // Clone so get VarRef or VarXRef as needed
                                         varrefp->clone_वृक्षः(मिच्छ)};
                    monSenItemsp = Ast_ग्रन्थिः::addNextNull(monSenItemsp, senItemp);
                });
            }

            Ast_Sen_वृक्षः* ध्रुव mon_Sen_वृक्षः = नूतन Ast_Sen_वृक्षः{fl, monSenItemsp};
            ध्रुव auto mon_अङ्कः = ++m_monitor_अङ्कः;
            // Where $monitor was we do "__VmonitorNum = N;"
            AstAssign* ध्रुव newsetp = नूतन AstAssign{
                fl, new_Monitor_अङ्कः_Var_Refp(nodep, VAccess::WRITE), नूतन AstConst{fl, mon_अङ्कः}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__VmonitorOn && __VmonitorNum==N) $display(...);"
            Ast_ग्रन्थिः* ध्रुव stmtsp = nodep;
            AstIf* ध्रुव ifp = नूतन AstIf{
                fl,
                नूतन AstLogAnd{fl, नूतन AstLogNot{fl, newMonitorOffVarRefp(nodep, VAccess::READ)},
                              नूतन AstEq{fl, नूतन AstConst{fl, mon_अङ्कः},
                                        new_Monitor_अङ्कः_Var_Refp(nodep, VAccess::READ)}},
                stmtsp};
            ifp->isBoundsCheck(सच्च);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_ग्रन्थिः* ध्रुव newp = नूतन AstAlways{fl, VAlwaysKwd::ALWAYS, mon_Sen_वृक्षः, ifp};
            m_modp->addStmtsp(newp);
        } अण्णहा जइ (nodep->displayType() == VDisplayType::DT_STROBE) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            // Need one-shot
            File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
            AstVar* ध्रुव varp
                = नूतन AstVar{fl, VVarType::MODULETEMP, "__Vstrobe" + cvtToStr(m_mod_Strobe_अङ्कः++),
                             nodep->findBitDType()};
            m_modp->addStmtsp(varp);
            // Where $strobe was we do "__Vstrobe = '1;"
            AstAssign* ध्रुव newsetp = नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                                     नूतन AstConst{fl, AstConst::BitTrue{}}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__Vstrobe) begin $display(...); __Vstrobe = '0; end"
            Ast_ग्रन्थिः* ध्रुव stmtsp = nodep;
            AstIf* ध्रुव ifp = नूतन AstIf{fl, नूतन AstVarRef{fl, varp, VAccess::READ}, stmtsp};
            ifp->isBoundsCheck(सच्च);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_ग्रन्थिः* ध्रुव newp = नूतन AstAlwaysPostponed{fl, ifp};
            stmtsp->addNext(नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                          नूतन AstConst{fl, AstConst::BitFalse{}}});
            m_modp->addStmtsp(newp);
        }
    }
    शून्य visit(AstMonitorOff* nodep) override {
        AstAssign* ध्रुव newp
            = नूतन AstAssign{nodep->fileline(), newMonitorOffVarRefp(nodep, VAccess::WRITE),
                            नूतन AstConst{nodep->fileline(), AstConst::BitTrue{}, nodep->off()}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstAssert* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    शून्य visit(AstAssertCtl* nodep) override {
        iterateChildren(nodep);

        तार्किक assertTypeConst = सच्च;
        जइ (!nodep->assertTypesp()) {
            nodep->assertTypes(VAssertType{ALL_ASSERT_TYPES});
        } अण्णहा जइ (ध्रुव AstConst* ध्रुव assertTypesp = VN_CAST(nodep->assertTypesp(), Const)) {
            nodep->assertTypes(VAssertType{assertTypesp->toSInt()});
        } अण्णहा {
            assertTypeConst = मिच्छ;
        }

        तार्किक controlTypeConst = मिच्छ;
        जइ (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->controlTypep(), Const)) {
            nodep->ctlType(constp->toSInt());
            controlTypeConst = सच्च;
        }
        जइ (controlTypeConst
            && (nodep->ctlType() < VAssertCtlType::LOCK
                || nodep->ctlType() > VAssertCtlType::VACUOUS_OFF)) {
            nodep->unlinkFrBack();
            nodep->v3error("Bad $assertcontrol control_type '"
                           << cvtToStr(static_cast<पूर्णाङ्क>(nodep->ctlType()))
                           << "' (IEEE 1800-2023 Table 20-5)");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम;
        }
        जइ (assertTypeConst && nodep->assertTypes() != ALL_ASSERT_TYPES
            && nodep->assertTypes().containsAny(VAssertType::UNIQUE | VAssertType::UNIQUE0
                                                | VAssertType::PRIORITY)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: assert control assertion_type");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
            प्रत्यागम;
        }

        तार्किक directiveTypeConst = सच्च;
        जइ (!nodep->directiveTypesp()) {
            nodep->directiveTypes(VAssertDirectiveType::ASSERT | VAssertDirectiveType::ASSUME
                                  | VAssertDirectiveType::COVER);
        } अण्णहा जइ (ध्रुव AstConst* ध्रुव directiveTypesp
                   = VN_CAST(nodep->directiveTypesp(), Const)) {
            nodep->directiveTypes(VAssertDirectiveType{directiveTypesp->toSInt()});
        } अण्णहा {
            directiveTypeConst = मिच्छ;
        }
        जइ (!directiveTypeConst) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: non-const assert directive type expression");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
            प्रत्यागम;
        }

        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        UINFO(9, "Generating assertctl for a module: " << m_modp);
        AstCStmt* ध्रुव newp = नूतन AstCStmt{fl};
        newp->add("vlSymsp->_vm_contextp__->assertCtl(");
        newp->add(nodep->controlTypep()->unlinkFrBack());
        newp->add(", ");
        जइ (nodep->assertTypesp()) {
            newp->add(nodep->assertTypesp()->unlinkFrBack());
        } अण्णहा {
            newp->add(std::to_string(ALL_ASSERT_TYPES));
        }
        newp->add(", " + std::to_string(nodep->directiveTypes()) + ");\n");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstAssertIntrinsic* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    शून्य visit(AstCover* nodep) override {  //
        visitAssertionIterate(nodep, nullptr);
    }
    शून्य visit(AstRestrict* nodep) override {
        VL_RESTORER(m_inRestrict);
        m_inRestrict = सच्च;
        iterateChildren(nodep);
        // IEEE says simulator ignores these
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
    }

    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Past_अङ्कः);
        VL_RESTORER(m_mod_Strobe_अङ्कः);
        VL_RESTORER(m_finalp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DelayedAlwaysp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DefaultDelayedAlwaysp);
        VL_RESTORER_CLEAR(m_delayedAlways2TickTimep);
        m_modp = nodep;
        m_mod_Past_अङ्कः = 0;
        m_mod_Strobe_अङ्कः = 0;
        m_finalp = nullptr;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstGenBlock* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstBegin* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
        // If the body is a single assertOn check, bubble it up for combining
        जइ (!nodep->declsp()) {
            जइ (AstIf* ध्रुव ifp = VN_CAST(nodep->stmtsp(), If)) {
                जइ (ifp->user2() && !ifp->nextp()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(ifp->unlinkFrBack());
                    nodep->addStmtsp(ifp->thensp()->unlinkFrBackWithNext());
                    ifp->addThensp(nodep);
                }
            }
        }
    }

    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
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
        VIsCached::clear_Cache_वृक्षः();
    }
};

//######################################################################
// Top Assert class

शून्य V3Assert::assertAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("assert", 0, dump_वृक्षः_Either_Level() >= 3);
}
