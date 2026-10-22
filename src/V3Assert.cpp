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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Assert.h"

#ಅನ್ತರ್ಭಾವ "V3AstUserAllocator.h"
#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ನಾಮಾಕಾಶ {

ವರ್ಗ DefaultDisableLocalVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE
    Ast_ಗಂಟು* m_scopep = nullptr;

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(nullptr);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(nullptr);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstDefaultDisable* nodep) override {
        UASSERT_OBJ(nodep, m_scopep,
                    "default disable iff must be inside a module or generate block");
        AstDefaultDisable* defaultp = nullptr;
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ modp = VN_CAST(m_scopep, ಗಂಟು_Module)) {
            defaultp = modp->defaultDisablep();
        } ಅನ್ಯಥಾ {
            defaultp = VN_AS(m_scopep, GenBlock)->defaultDisablep();
        }
        ಯದಿ (VL_UNLIKELY(defaultp)) {
            nodep->v3error("Only one 'default disable iff' allowed per "
                           << (VN_IS(m_scopep, ಗಂಟು_Module) ? "module" : "generate block")
                           << " (IEEE 1800-2023 16.15)");
        } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_Module* ಧ್ರುವ modp = VN_CAST(m_scopep, ಗಂಟು_Module)) {
            modp->defaultDisablep(nodep);
        } ಅನ್ಯಥಾ {
            VN_AS(m_scopep, GenBlock)->defaultDisablep(nodep);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    explicit DefaultDisableLocalVisitor(AstNetlist* nodep) { iterate(nodep); }
};

ವರ್ಗ DefaultDisablePropagateVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE
    AstDefaultDisable* m_defaultDisablep = nullptr;

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        ಯದಿ (!nodep->defaultDisablep()) nodep->defaultDisablep(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    explicit DefaultDisablePropagateVisitor(AstNetlist* nodep) { iterate(nodep); }
};

// Lower a sequence used as an event control ('@seq', IEEE 1800-2023 9.4.2.4) into a
// synthesized event fired by an internal 'cover sequence' on each end-of-match
ವರ್ಗ SeqEventLowerVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
    V3UniqueNames m_names{"__VseqEvent"};  // Synthesized event names

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstSenItem* nodep) override {
        AstFuncRef* ಧ್ರುವ funcrefp = VN_CAST(nodep->sensp(), FuncRef);
        ಯದಿ (funcrefp && VN_IS(funcrefp->taskp(), Sequence)) {
            File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
            AstVar* ಧ್ರುವ eventp = ನೂತನ AstVar{flp, VVarType::MODULETEMP, m_names.get(nodep),
                                              m_modp->findBasicDType(VBasicDTypeKwd::EVENT)};
            eventp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(eventp);
            v3Global.setHasEvents();
            funcrefp->unlinkFrBack();
            nodep->sensp(ನೂತನ AstVarRef{flp, eventp, VAccess::READ});
            ಧ್ರುವ ತಾರ್ಕಿಕ automaticActual = funcrefp->exists([](ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* refp) {
                ಪ್ರತ್ಯಾಗಮ refp->varp() && refp->varp()->lifetime().isAutomatic();
            });
            ಯದಿ (automaticActual) {
                nodep->v3error("Arguments to a sequence used as an event control must be"
                               " static (IEEE 1800-2023 9.4.2.4)");
                VN_AS(funcrefp->taskp(), Sequence)->isReferenced(ಅಸತ್ಯ);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(funcrefp), funcrefp);
                ಪ್ರತ್ಯಾಗಮ;
            }
            AstFireEvent* ಧ್ರುವ firep
                = ನೂತನ AstFireEvent{flp, ನೂತನ AstVarRef{flp, eventp, VAccess::WRITE}, ಅಸತ್ಯ};
            AstCover* ಧ್ರುವ coverp
                = ನೂತನ AstCover{flp, ನೂತನ AstPropSpec{flp, nullptr, nullptr, funcrefp}, firep,
                               VAssertType::CONCURRENT};
            coverp->isCoverSeq(ಸತ್ಯ);
            coverp->isSeqEvent(ಸತ್ಯ);
            m_modp->addStmtsp(coverp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    explicit SeqEventLowerVisitor(AstNetlist* nodep) { iterate(nodep); }
};

}  // namespace

ಶೂನ್ಯ V3AssertCommon::collectDefaultDisable(AstNetlist* nodep) {
    { DefaultDisableLocalVisitor{nodep}; }
    { DefaultDisablePropagateVisitor{nodep}; }
}

ಶೂನ್ಯ V3AssertCommon::lowerSequenceEvents(AstNetlist* nodep) {
    { SeqEventLowerVisitor{nodep}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("assertseqevent", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

string V3AssertCommon::assertCtlGetCall(ಧ್ರುವ ವರ್ಣ* query, VAssertType type,
                                        VAssertDirectiveType directiveType) {
    // Template arguments let the runtime build the type/directive mask at C++ compile time
    ಪ್ರತ್ಯಾಗಮ "vlSymsp->_vm_contextp__->assertCtlGet<"s + std::to_string(type) + ", "s
           + std::to_string(directiveType) + ">(VerilatedAssertCtlQuery::"s + query + ")"s;
}

ತಾರ್ಕಿಕ V3AssertCommon::check_Sva_ಮೌಲ್ಯ_Limit(Ast_ಗಂಟು* nodep, ಧ್ರುವ AstConst* constp, ಧ್ರುವ ವರ್ಣ* what) {
    // Temporal values can materialize as O(N) AST or state in later passes.
    // Leave headroom for a range ring's extra slot and V3Number's signed
    // `(bits + 31) / 32` word count.
    ಸ್ಥಿರ constexpr unsigned SVA_ಮೌಲ್ಯ_HARD_LIMIT
        = static_cast<unsigned>(std::numeric_limits<ಪೂರ್ಣಾಙ್ಕ>::max()) - 32U;
    ಧ್ರುವ unsigned configuredLimit = static_cast<unsigned>(v3Global.opt.max_ಅಂಕೆ_Width());
    ಧ್ರುವ unsigned limit = std::min(configuredLimit, SVA_ಮೌಲ್ಯ_HARD_LIMIT);
    ಯದಿ (constp->ಅಂಕೆ().fitsInUInt() && constp->toUInt() <= limit) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << what << " exceeds implementation limit of "
                                                 << limit
                                                 << (configuredLimit <= SVA_ಮೌಲ್ಯ_HARD_LIMIT
                                                         ? " (--max-num-width)"
                                                         : " (host arithmetic limit)"));
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
}

// The bound of a cycle delay in a sequence, if a non-negative elaboration-time constant
ಸ್ಥಿರ ಧ್ರುವ AstConst* checkSvaDelayBound(Ast_ಗಂಟು* errp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* boundp, ಧ್ರುವ ವರ್ಣ* what) {
    ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(boundp, Const);
    ಯದಿ (!constp || (constp->dtypep()->isSigned() && constp->ಅಂಕೆ().isNegative())) {
        errp->v3error(what << " is not a non-negative elaboration-time constant"
                              " (IEEE 1800-2023 16.7)");
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    ಯದಿ (!V3AssertCommon::check_Sva_ಮೌಲ್ಯ_Limit(errp, constp, "SVA cycle delay")) ಪ್ರತ್ಯಾಗಮ nullptr;
    ಪ್ರತ್ಯಾಗಮ constp;
}

// Fold the bounds of a cycle delay in a sequence, and return if they are valid, else report it
// on errp
ಸ್ಥಿರ ತಾರ್ಕಿಕ svaCycleDelayValid(ಧ್ರುವ AstDelay* nodep, Ast_ಗಂಟು* errp) {
    V3Const::constifyParamsNoWarnEdit(nodep->lhsp());
    ಧ್ರುವ AstConst* ಧ್ರುವ minConstp = checkSvaDelayBound(
        errp, nodep->lhsp(), nodep->isRangeDelay() ? "Range delay minimum" : "Delay value");
    ಯದಿ (!nodep->rhsp() || nodep->isUnbounded()) ಪ್ರತ್ಯಾಗಮ minConstp != nullptr;
    V3Const::constifyParamsNoWarnEdit(nodep->rhsp());
    ಧ್ರುವ AstConst* ಧ್ರುವ maxConstp
        = checkSvaDelayBound(errp, nodep->rhsp(), "Range delay maximum");
    ಯದಿ (!minConstp || !maxConstp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಯದಿ (maxConstp->toUInt() < minConstp->toUInt()) {
        errp->v3error("Range delay maximum must be >= minimum (IEEE 1800-2023 16.7)");
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
}

ಶೂನ್ಯ V3AssertCommon::checkSvaCycleDelay(AstDelay* nodep, Ast_ಗಂಟು* errp) {
    ಯದಿ (svaCycleDelayValid(nodep, errp)) ಪ್ರತ್ಯಾಗಮ;
    // Make the delay valid, so later steps do not fail after the error
    File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ minp = nodep->lhsp();
    minp->replaceWith(ನೂತನ AstConst{flp, AstConst::BitFalseErroring{}});
    VL_DO_DANGLING(minp->delete_ವೃಕ್ಷ(), minp);
    ಯದಿ (nodep->rhsp() && !nodep->isUnbounded()) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ maxp = nodep->rhsp();
        maxp->replaceWith(ನೂತನ AstConst{flp, AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(maxp->delete_ವೃಕ್ಷ(), maxp);
    }
}

ತಾರ್ಕಿಕ V3AssertCommon::refsSvaFormal(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
    ಪ್ರತ್ಯಾಗಮ nodep->exists([](ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* refp) {
        ಧ್ರುವ AstVar* ಧ್ರುವ varp = refp->varp();
        ಯದಿ (!varp || !varp->isIO()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ declp = varp->aboveLoopp();
        ಪ್ರತ್ಯಾಗಮ VN_IS(declp, Sequence) || VN_IS(declp, Property);
    });
}

ಶೂನ್ಯ V3AssertCommon::checkSvaInstanceDelays(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* instp) {
    // A delay still referencing a formal argument of an enclosing sequence or property is
    // checked when that is instanced. Collect first, as checking edits the delays.
    std::vector<AstDelay*> delayps;
    nodep->foreach([&](AstDelay* delayp) {
        ಯದಿ (!delayp->isCycleDelay() || refsSvaFormal(delayp->lhsp())) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (delayp->rhsp() && refsSvaFormal(delayp->rhsp())) ಪ್ರತ್ಯಾಗಮ;
        delayps.ತಳ್ಳು_back(delayp);
    });
    ಪ್ರತಿ (AstDelay* ಧ್ರುವ delayp : delayps) checkSvaCycleDelay(delayp, instp);
}

//######################################################################
// AssertDeFutureVisitor
// If any AstFuture, then move all non-future varrefs to be one cycle behind,
// see IEEE 1800-2023 16.9.4.

ವರ್ಗ AssertDeFutureVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE - across all visitors
    Ast_ಗಂಟು_Module* ಧ್ರುವ m_modp;  // Module future is underneath
    ಧ್ರುವ AstFuture* m_futurep;  // First AstFuture found
    ಧ್ರುವ unsigned m_past_ಅಂಕೆ;  // Prefix unique number for this module
    std::map<AstVar*, AstVar*> m_delayedVars;  // Old to delayed variable mapping
    // STATE - for current visit position (use VL_RESTORER)
    ತಾರ್ಕಿಕ m_inFuture = ಅಸತ್ಯ;  // Inside a future
    ತಾರ್ಕಿಕ m_unsupported = ಅಸತ್ಯ;  // Printed unsupported

    // METHODS
    ಶೂನ್ಯ unsupported(Ast_ಗಂಟು* nodep) {
        ಯದಿ (m_unsupported) ಪ್ರತ್ಯಾಗಮ;
        m_unsupported = ಸತ್ಯ;
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported/illegal: Future value function used with expression with "
                          << nodep->prettyOperatorName());
    }
    // VISITORS
    ಶೂನ್ಯ visit(AstFuture* nodep) override {
        VL_RESTORER(m_inFuture);
        m_inFuture = ಸತ್ಯ;
        iterateChildren(nodep);
        // Done with the future, this subexpression is current-time
        nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (m_inFuture || m_unsupported)
            ಪ್ರತ್ಯಾಗಮ;  // Need user1 set above, don't process when Future is removed
        ಯದಿ (nodep->access().isWriteOrRW()) {
            unsupported(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        auto it = m_delayedVars.find(nodep->varp());
        AstVar* outvarp;
        ಯದಿ (it == m_delayedVars.end()) {
            Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ sentreep = m_futurep->sentreep();
            AstAlways* ಧ್ರುವ alwaysp = ನೂತನ AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS,
                                                     sentreep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), nullptr};
            m_modp->addStmtsp(alwaysp);
            outvarp = ನೂತನ AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                 "__Vnotfuture" + cvtToStr(m_past_ಅಂಕೆ) + "_" + nodep->name(),
                                 nodep->dtypep()};
            m_modp->addStmtsp(outvarp);
            AstVarRef* varRefAWritep = ನೂತನ AstVarRef{nodep->fileline(), outvarp, VAccess::WRITE};
            varRefAWritep->user1(ಸತ್ಯ);
            Ast_ಗಂಟು_Var_Ref* varRefAReadp = nodep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            varRefAReadp->user1(ಸತ್ಯ);
            Ast_ಗಂಟು* ಧ್ರುವ assp = ನೂತನ AstAssignDly{nodep->fileline(), varRefAWritep, varRefAReadp};
            alwaysp->addStmtsp(assp);
            m_delayedVars.emplace(nodep->varp(), outvarp);
        } ಅನ್ಯಥಾ {
            outvarp = it->second;
        }
        AstVarRef* newp = ನೂತನ AstVarRef{nodep->fileline(), outvarp, VAccess::READ};
        newp->user1(ಸತ್ಯ);
        UINFO(9, "DeFuture " << nodep << "  becomes " << newp);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override { unsupported(nodep); }
    ಶೂನ್ಯ visit(AstMethodCall* nodep) override { unsupported(nodep); }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        ಯದಿ (!nodep->isPure()) unsupported(nodep);
        iterateChildren(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit AssertDeFutureVisitor(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_Module* modp, unsigned past_ಅಂಕೆ)
        : m_modp{modp}
        , m_past_ಅಂಕೆ{past_ಅಂಕೆ} {
        // See if any Future before we process
        ಯದಿ (nodep->forall([&](ಧ್ರುವ AstFuture* futurep) -> ತಾರ್ಕಿಕ {
                m_futurep = futurep;
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }))
            ಪ್ರತ್ಯಾಗಮ;
        // UINFOTREE(9, nodep, "", "defuture-in");
        visit(nodep);  // Nodep may get deleted
        // UINFOTREE(9, nodep, "", "defuture-ou");
    }
    ~AssertDeFutureVisitor() = ಸ್ವಭಾವ;
};

// Prepare one default sampled expression.
ವರ್ಗ Default_Sampled_ಮೌಲ್ಯ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // METHODS
    ಸ್ಥಿರ AstConst* newTypeDefault(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        AstConst* ಧ್ರುವ constp
            = ನೂತನ AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()};
        ಯದಿ (nodep->dtypep()->isFourstate()) constp->ಅಂಕೆ().setAllBitsX();
        constp->dtypeFrom(nodep);
        ಪ್ರತ್ಯಾಗಮ constp;
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstCMethodHard* nodep) override {
        ಯದಿ (nodep->method() == VCMethod::EVENT_IS_TRIGGERED) {
            nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ {
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        AstVar* ಧ್ರುವ varp = nodep->varp();
        ಯದಿ (varp->isParam()
            || (!varp->isNet() && varp->lifetime().isStatic() && varp->hasUserInit())) {
            ಪ್ರತ್ಯಾಗಮ;
        }
        nodep->replaceWith(newTypeDefault(nodep));
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    explicit Default_Sampled_ಮೌಲ್ಯ_Visitor(Ast_ಗಂಟು* nodep) { iterate(nodep); }
};

//######################################################################
// AssertVisitor

ವರ್ಗ AssertVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // CONSTANTS
    ಸ್ಥಿರ constexpr uint8_t ALL_ASSERT_TYPES
        = std::numeric_limits<std::underlying_type<VAssertType::en>::type>::max();

    // NODE STATE/TYPES
    // Cleared on netlist
    //  AstNode::user1()         -> bool.  True if processed
    //  AstAlways::user2p()      -> std::vector<AstVar*>. Delayed variables via 'm_delayed'
    //  AstNodeVarRef::user2()   -> bool.  True if shouldn't be sampled
    //  AstIf::user2()           -> bool.  True for assertOn() checks inserted in this pass
    ಧ್ರುವ VNUser1InUse m_user1InUse;
    ಧ್ರುವ VNUser2InUse m_user2InUse;
    AstUser2Allocator<AstAlways, std::vector<AstVar*>> m_delayed;

    // STATE
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Last module
    ಧ್ರುವ Ast_ಗಂಟು* m_beginp = nullptr;  // Last AstBegin/AstGenBlock
    unsigned m_monitor_ಅಂಕೆ = 0;  // Global $monitor numbering (not per module)
    AstVar* m_monitor_ಅಂಕೆ_Varp = nullptr;  // $monitor number variable
    AstVar* m_monitorOffVarp = nullptr;  // $monitoroff variable
    unsigned m_mod_Past_ಅಂಕೆ = 0;  // Module past numbering
    unsigned m_mod_Strobe_ಅಂಕೆ = 0;  // Module $strobe numbering
    Ast_ಗಂಟು_Procedure* m_procedurep = nullptr;  // Current procedure
    VDouble0 m_statCover;  // Statistic tracking
    VDouble0 m_statAsNotImm;  // Statistic tracking
    VDouble0 m_statAsImm;  // Statistic tracking
    VDouble0 m_statAsFull;  // Statistic tracking
    VDouble0 m_statPastVars;  // Statistic tracking
    VDouble0 m_statAssertOnCombined;  // Statistic tracking
    VDouble0 m_statAssertOnHoisted;  // Statistic tracking
    ತಾರ್ಕಿಕ m_inSampled = ಅಸತ್ಯ;  // True inside a sampled expression
    ತಾರ್ಕಿಕ m_inRestrict = ಅಸತ್ಯ;  // True inside restrict assertion
    Ast_ಗಂಟು* m_passsp = nullptr;  // Current pass statement
    Ast_ಗಂಟು* m_failsp = nullptr;  // Current fail statement
    Ast_ಗಂಟು_Cover_Or_Assert* m_assertp = nullptr;  // Current assertion
    AstFinal* m_finalp = nullptr;  // Current final block
    VDouble0 m_statLiftedCaseExprs;  // Count of purified case expressions
    Ast_ಗಂಟು_FTask* m_ftaskp = nullptr;  // Current function/task
    V3UniqueNames m_caseTempNames{"__VCase"};
    V3UniqueNames m_match_ಎಣಿಕೆ_Names{"__VnfaRemainingMatchCount"};  // Match replay counter names
    // Maps from (expression, senTree) to the AstAlways that computes its delayed values.
    std::unordered_map<VNRef<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ>, std::unordered_map<VNRef<Ast_Sen_ವೃಕ್ಷ>, AstAlways*>>
        m_modExpr2Sen2DelayedAlwaysp;
    // Source sampled-value history is initialized, unlike internal property-lowering history.
    std::unordered_map<VNRef<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ>, std::unordered_map<VNRef<Ast_Sen_ವೃಕ್ಷ>, AstAlways*>>
        m_modExpr2Sen2DefaultDelayedAlwaysp;
    // Map from delayed-value AstAlways to its last-sampling-tick time variable
    std::unordered_map<ಧ್ರುವ AstAlways*, AstVar*> m_delayedAlways2TickTimep;

    // METHODS
    ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* assertPassOnQuery(ತಾರ್ಕಿಕ vacuous) {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* queries[2]
            = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
        ಪ್ರತ್ಯಾಗಮ queries[vacuous];
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* assertOnCond(File_ಸಾಲು* fl, VAssertType type,
                                     VAssertDirectiveType directiveType) {
        // cppcheck-suppress missingReturn
        ವಿಕಲ್ಪ (directiveType) {
        ಪಕ್ಷ VAssertDirectiveType::INTRINSIC: ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{fl, AstConst::BitTrue{}};
        ಪಕ್ಷ VAssertDirectiveType::VIOLATION_CASE: {
            ಯದಿ (v3Global.opt.assertCase()) {
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            // If assertions are off, have constant propagation rip them out later
            // This allows syntax errors and such to be detected normally.
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{fl, AstConst::BitFalse{}};
        }
        ಪಕ್ಷ VAssertDirectiveType::ASSERT:
        ಪಕ್ಷ VAssertDirectiveType::COVER:
        ಪಕ್ಷ VAssertDirectiveType::ASSUME: {
            ಯದಿ (v3Global.opt.assertOn()) {
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCExpr{
                    fl, AstCExpr::Pure{},
                    V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
            }
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{fl, AstConst::BitFalse{}};
        }
        ಪಕ್ಷ VAssertDirectiveType::INTERNAL:
        ಪಕ್ಷ VAssertDirectiveType::VIOLATION_IF:
        ಪಕ್ಷ VAssertDirectiveType::RESTRICT: {
            ಯದಿ (v3Global.opt.assertOn()) {
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{fl, AstConst::BitFalse{}};
        }
        }
        VL_UNREACHABLE;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isControlled(VAssertDirectiveType directiveType) {
        ಪ್ರತ್ಯಾಗಮ (static_cast<ಪೂರ್ಣಾಙ್ಕ>(directiveType)
                & (static_cast<ಪೂರ್ಣಾಙ್ಕ>(VAssertDirectiveType::ASSERT)
                   | static_cast<ಪೂರ್ಣಾಙ್ಕ>(VAssertDirectiveType::COVER)
                   | static_cast<ಪೂರ್ಣಾಙ್ಕ>(VAssertDirectiveType::ASSUME)));
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* assertPassOnCond(File_ಸಾಲು* fl, VAssertType type,
                                         VAssertDirectiveType directiveType, ತಾರ್ಕಿಕ vacuous) {
        ಯದಿ (!isControlled(directiveType)) ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{fl, AstConst::BitTrue{}};
        ಯದಿ (!v3Global.opt.assertOn()) ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{fl, AstConst::BitFalse{}};
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType), 1};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* assertFailOnCond(File_ಸಾಲು* fl, VAssertType type,
                                         VAssertDirectiveType directiveType) {
        ಯದಿ (!isControlled(directiveType)) ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{fl, AstConst::BitTrue{}};
        ಯದಿ (!v3Global.opt.assertOn()) ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{fl, AstConst::BitFalse{}};
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType), 1};
    }
    string assertDisplayMessage(ಧ್ರುವ Ast_ಗಂಟು* nodep, ಧ್ರುವ string& prefix, ಧ್ರುವ string& message,
                                VDisplayType severity) {
        ಯದಿ (severity == VDisplayType::DT_ERROR || severity == VDisplayType::DT_FATAL) {
            ಪ್ರತ್ಯಾಗಮ ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                    + cvtToStr(nodep->fileline()->lineno()) + ": Assertion failed in %m"
                    + ((message != "") ? ": " : "") + message + "\n");
        }
        ಪ್ರತ್ಯಾಗಮ ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                + cvtToStr(nodep->fileline()->lineno()) + ": %m" + ((message != "") ? ": " : "")
                + message + "\n");
    }
    ಶೂನ್ಯ replaceDisplay(AstDisplay* nodep, ಧ್ರುವ string& prefix) {
        nodep->fmtp()->text(
            assertDisplayMessage(nodep, prefix, nodep->fmtp()->text(), nodep->displayType()));
        nodep->displayType(VDisplayType::DT_WRITE);
        // cppcheck-suppress nullPointer
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ timenewp = ನೂತನ AstTime{nodep->fileline(), m_modp->timeunit()};
        nodep->fmtp()->prependExprsp(timenewp);
        ಯದಿ (!nodep->fmtp()->scopeNamep() && nodep->fmtp()->formatScopeTracking()) {
            nodep->fmtp()->scopeNamep(ನೂತನ AstScopeName{nodep->fileline(), ಸತ್ಯ});
        }
    }
    AstSampled* new_Sampled_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstSampled{nodep->fileline(), nodep, nodep->dtypep(), ಸತ್ಯ};
    }
    AstVarRef* new_Monitor_ಅಂಕೆ_Var_Refp(ಧ್ರುವ Ast_ಗಂಟು* nodep, VAccess access) {
        ಯದಿ (!m_monitor_ಅಂಕೆ_Varp) {
            m_monitor_ಅಂಕೆ_Varp = ನೂತನ AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorNum",
                                          nodep->findUInt64DType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitor_ಅಂಕೆ_Varp);
        }
        AstVarRef* ಧ್ರುವ varrefp = ನೂತನ AstVarRef{nodep->fileline(), m_monitor_ಅಂಕೆ_Varp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        ಪ್ರತ್ಯಾಗಮ varrefp;
    }
    AstVarRef* newMonitorOffVarRefp(ಧ್ರುವ Ast_ಗಂಟು* nodep, VAccess access) {
        ಯದಿ (!m_monitorOffVarp) {
            m_monitorOffVarp = ನೂತನ AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorOff",
                                          nodep->findBitDType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitorOffVarp);
        }
        AstVarRef* ಧ್ರುವ varrefp = ನೂತನ AstVarRef{nodep->fileline(), m_monitorOffVarp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        ಪ್ರತ್ಯಾಗಮ varrefp;
    }
    ಸ್ಥಿರ AstIf* newIfAssertOn(Ast_ಗಂಟು* bodyp, VAssertDirectiveType directiveType,
                                VAssertType type = VAssertType::INTERNAL) {
        // Add an internal if to check assertions are on.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_ಸಾಲು* ಧ್ರುವ fl = bodyp->fileline();

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = assertOnCond(fl, type, directiveType);
        AstIf* ಧ್ರುವ newp = ನೂತನ AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(ಸತ್ಯ);  // To avoid LATCH warning
        newp->user2(ಸತ್ಯ);  // Mark as an assertOn() check
        ಪ್ರತ್ಯಾಗಮ newp;
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_Stmt* newIfAssertPassOn(Ast_ಗಂಟು* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type, ತಾರ್ಕಿಕ vacuous) {
        // Add an internal if to check assertion passOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_ಸಾಲು* ಧ್ರುವ fl = bodyp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = assertPassOnCond(fl, type, directiveType, vacuous);
        Ast_ಗಂಟು_If* ಧ್ರುವ newp = ನೂತನ AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(ಸತ್ಯ);  // To avoid LATCH warning
        newp->user1(ಸತ್ಯ);  // Don't assert/cover this if
        newp->user2(ಸತ್ಯ);  // Mark as an assertOn() check
        ಪ್ರತ್ಯಾಗಮ newp;
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_Stmt* newIfAssertFailOn(Ast_ಗಂಟು* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type) {
        // Add an internal if to check assertion failOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_ಸಾಲು* ಧ್ರುವ fl = bodyp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = assertFailOnCond(fl, type, directiveType);
        Ast_ಗಂಟು_If* ಧ್ರುವ newp = ನೂತನ AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(ಸತ್ಯ);  // To avoid LATCH warning
        newp->user1(ಸತ್ಯ);  // Don't assert/cover this if
        newp->user2(ಸತ್ಯ);  // Mark as an assertOn() check
        ಪ್ರತ್ಯಾಗಮ newp;
    }

    ಸ್ಥಿರ AstIf* assertCond(ಧ್ರುವ Ast_ಗಂಟು_Cover_Or_Assert* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* propp,
                             Ast_ಗಂಟು* passsp, Ast_ಗಂಟು* failsp) {
        AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{nodep->fileline(), propp, passsp, failsp};
        // It's more LIKELY that we'll take the nullptr if clause
        // than the sim-killing else clause:
        ifp->branchPred(VBranchPred::BP_LIKELY);
        ifp->isBoundsCheck(ಸತ್ಯ);  // To avoid LATCH warning
        ಪ್ರತ್ಯಾಗಮ ifp;
    }

    Ast_ಗಂಟು_Stmt* assertBody(ಧ್ರುವ Ast_ಗಂಟು_Cover_Or_Assert* nodep, Ast_ಗಂಟು* propp, Ast_ಗಂಟು* passsp,
                            Ast_ಗಂಟು* failsp) {
        ಯದಿ (AstPExpr* ಧ್ರುವ pexprp = VN_CAST(propp, PExpr)) {
            ಯದಿ (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NOTIMING, "This property expression requires --timing");
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(pexprp), pexprp);
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstBegin{nodep->fileline(), "", nullptr, ಅಸತ್ಯ};
            }
            AstFork* ಧ್ರುವ forkp = ನೂತನ AstFork{nodep->fileline(), VJoinType::JOIN_NONE};
            forkp->addForksp(pexprp->bodyp()->unlinkFrBack());
            ಯದಿ (Ast_ಗಂಟು_Stmt* ಧ್ರುವ finalp = pexprp->finalp()) {
                ಯದಿ (!m_finalp) {
                    m_finalp = ನೂತನ AstFinal{m_modp->fileline(), finalp->unlinkFrBack()};
                    m_modp->addStmtsp(m_finalp);
                } ಅನ್ಯಥಾ {
                    m_finalp->addStmtsp(finalp->unlinkFrBack());
                }
            }
            VL_DO_DANGLING2(ತಳ್ಳು_Deletep(pexprp), pexprp, propp);
            ಪ್ರತ್ಯಾಗಮ forkp;
        }

        ಪ್ರತ್ಯಾಗಮ assertCond(nodep, VN_AS(propp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ), passsp, failsp);
    }

    Ast_ಗಂಟು_Stmt* newFireAssertUnchecked(ಧ್ರುವ Ast_ಗಂಟು_Stmt* nodep, ಧ್ರುವ string& message,
                                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp = nullptr) {
        // Like newFireAssert() but omits the asserts-on check
        AstDisplay* ಧ್ರುವ dispp
            = ನೂತನ AstDisplay{nodep->fileline(), VDisplayType::DT_ERROR, message, nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        Ast_ಗಂಟು_Stmt* ಧ್ರುವ bodysp = dispp;
        replaceDisplay(dispp, "%%Error");  // Convert to standard DISPLAY format
        ಯದಿ (exprsp) dispp->fmtp()->exprsp()->addNext(exprsp);
        ಯದಿ (v3Global.opt.stopFail()) bodysp->addNext(ನೂತನ AstStop{nodep->fileline(), ಅಸತ್ಯ});
        ಪ್ರತ್ಯಾಗಮ bodysp;
    }

    Ast_ಗಂಟು_Stmt* newFireAssert(ಧ್ರುವ Ast_ಗಂಟು_Stmt* nodep, VAssertDirectiveType directiveType,
                               VAssertType assertType, ಧ್ರುವ string& message,
                               Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp = nullptr) {
        Ast_ಗಂಟು_Stmt* bodysp = newFireAssertUnchecked(nodep, message, exprsp);
        bodysp = newIfAssertOn(bodysp, directiveType, assertType);
        ಪ್ರತ್ಯಾಗಮ bodysp;
    }

    AstVar* createDelayedVar(ಧ್ರುವ std::string& name, AstAlways* alwaysp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp,
                             Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* initp) {
        File_ಸಾಲು* ಧ್ರುವ flp = exprp->fileline();
        AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        // TODO: this lifetime seems nonsene (can't have NBAs to automatics), but is as before
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->addStmtsp(varp);
        // Before enough clock ticks, $past returns the expression's default sampled value.
        ಯದಿ (initp) {
            m_modp->addStmtsp(ನೂತನ AstInitialStatic{
                flp, ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, varp, VAccess::WRITE}, initp}});
        }
        ++m_statPastVars;
        // Actually set the delayed value
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = ನೂತನ AstVarRef{flp, varp, VAccess::WRITE};
        AstAssignDly* ಧ್ರುವ assignp = ನೂತನ AstAssignDly{flp, lhsp, exprp};
        alwaysp->prependStmtsp(assignp);
        ಪ್ರತ್ಯಾಗಮ varp;
    }

    AstAlways* getDelayedAlways(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* initp, Ast_Sen_ವೃಕ್ಷ* senTreep) {
        auto& expr2Sen2Alwaysr
            = initp ? m_modExpr2Sen2DefaultDelayedAlwaysp : m_modExpr2Sen2DelayedAlwaysp;
        AstAlways*& alwayspr = expr2Sen2Alwaysr[*exprp][*senTreep];
        ಯದಿ (!alwayspr) {
            File_ಸಾಲು* ಧ್ರುವ flp = exprp->fileline();
            // Create the always block that computes the delayed values
            alwayspr = ನೂತನ AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, nullptr};
            m_modp->addStmtsp(alwayspr);
            // Create the once-delayed variable
            ಧ್ರುವ std::string name = "_Vpast_" + cvtToStr(m_mod_Past_ಅಂಕೆ++) + "_1";
            AstVar* ಧ್ರುವ varp = createDelayedVar(name, alwayspr, exprp, initp);
            // Add it to delayed variable vector
            m_delayed(alwayspr).emplace_back(varp);
        } ಅನ್ಯಥಾ {
            // Reusing exiting, not needed
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(exprp), exprp);
            ಯದಿ (initp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(initp), initp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(senTreep), senTreep);
        }
        ಪ್ರತ್ಯಾಗಮ alwayspr;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* get_Past_ಮೌಲ್ಯ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* initp, Ast_Sen_ವೃಕ್ಷ* senTreep,
                              uint32_t ticks) {
        UASSERT_OBJ(ticks > 0, exprp, "Delay must be > 0");
        File_ಸಾಲು* ಧ್ರುವ flp = exprp->fileline();
        AstAlways* ಧ್ರುವ alwaysp
            = getDelayedAlways(exprp, initp ? initp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ) : nullptr, senTreep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ resultp = past_ಮೌಲ್ಯ_Ref(alwaysp, flp, ticks, initp);
        ಯದಿ (initp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(initp), initp);
        ಪ್ರತ್ಯಾಗಮ resultp;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* past_ಮೌಲ್ಯ_Ref(AstAlways* alwaysp, File_ಸಾಲು* flp, uint32_t ticks,
                              Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* initp) {
        std::vector<AstVar*>& delayedr = m_delayed(alwaysp);
        // Ensure the required delay exists
        ಯಾವತ್ (delayedr.size() < ticks) {
            AstVar* ಧ್ರುವ firstp = delayedr.front();
            File_ಸಾಲು* ಧ್ರುವ varFlp = firstp->fileline();
            // Create once more delayed value
            std::string name = firstp->name();
            name.resize(name.size() - 1);
            name += std::to_string(delayedr.size() + 1);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ prevp = ನೂತನ AstVarRef{varFlp, delayedr.back(), VAccess::READ};
            AstVar* ಧ್ರುವ varp = createDelayedVar(name, alwaysp, prevp,
                                                  initp ? initp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ) : nullptr);
            // Add it to delayed variable vector
            delayedr.emplace_back(varp);
        }
        // Return a reference to the appropriately delayed variable
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{flp, delayedr.at(ticks - 1), VAccess::READ};
    }

    // Time of the pipeline's last sampling tick, for end-of-simulation readers
    AstVar* getPastTickTimeVar(AstAlways* alwaysp) {
        AstVar*& varpr = m_delayedAlways2TickTimep[alwaysp];
        ಯದಿ (!varpr) {
            File_ಸಾಲು* ಧ್ರುವ flp = alwaysp->fileline();
            varpr = ನೂತನ AstVar{flp, VVarType::MODULETEMP,
                               "_Vpast_" + cvtToStr(m_mod_Past_ಅಂಕೆ++) + "_t",
                               m_modp->findUInt64DType()};
            m_modp->addStmtsp(varpr);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ timep = ನೂತನ AstCExpr{flp, "vlSymsp->_vm_contextp__->time()", 64};
            alwaysp->addStmtsp(
                ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, varpr, VAccess::WRITE}, timep});
        }
        ಪ್ರತ್ಯಾಗಮ varpr;
    }

    ಶೂನ್ಯ visitAssertionIterate(Ast_ಗಂಟು_Cover_Or_Assert* nodep, Ast_ಗಂಟು* failsp) {
        ಯದಿ (m_beginp && nodep->name() == "") nodep->name(m_beginp->name());

        { AssertDeFutureVisitor{nodep->propp(), m_modp, m_mod_Past_ಅಂಕೆ++}; }

        iterateAndNextNull(nodep->sentreep());
        ಯದಿ (AstAssert* ಧ್ರುವ assertp = VN_CAST(nodep, Assert)) {
            iterateAndNextNull(assertp->failsp());
        } ಅನ್ಯಥಾ ಯದಿ (AstAssertIntrinsic* ಧ್ರುವ assertp = VN_CAST(nodep, AssertIntrinsic)) {
            iterateAndNextNull(assertp->failsp());
        } ಅನ್ಯಥಾ ಯದಿ (AstCover* ಧ್ರುವ coverp = VN_CAST(nodep, Cover)) {
            iterateAndNextNull(coverp->coverincsp());
        } ಅನ್ಯಥಾ ಯದಿ (!VN_IS(nodep, Restrict)) {
            nodep->v3fatalSrc("Unhandled assert type");
        }
        iterateAndNextNull(nodep->passsp());
        Ast_Sen_ವೃಕ್ಷ* sentreep = nodep->sentreep();
        ಯದಿ (nodep->immediate()) {
            UASSERT_OBJ(!sentreep, nodep, "Immediate assertions don't have sensitivity");
        } ಅನ್ಯಥಾ {
            UASSERT_OBJ(sentreep, nodep, "Concurrent assertions must have sensitivity");
            // Explicit inline clock differs from the enclosing always: hoist
            // and warn. To support this need queue of asserts to activate.
            ಯದಿ (m_procedurep && !nodep->senFromAlways()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Procedural concurrent assertion with"
                              " clocking event inside always (IEEE 1800-2023 16.14.6)");
                static_cast<Ast_ಗಂಟು*>(m_procedurep)->addNext(nodep->unlinkFrBack());
                ಪ್ರತ್ಯಾಗಮ;  // Later iterate will pick up
            }
            sentreep->unlinkFrBack();
            ಯದಿ (m_procedurep) { VL_DO_DANGLING(ತಳ್ಳು_Deletep(sentreep), sentreep); }
        }
        //
        ಧ್ರುವ string& message = nodep->name();
        Ast_ಗಂಟು* passsp = nodep->passsp();
        ಯದಿ (passsp) passsp->unlinkFrBackWithNext();
        ಯದಿ (AstAssert* ಧ್ರುವ assertp = VN_CAST(nodep, Assert)) {
            failsp = assertp->failsp();
        } ಅನ್ಯಥಾ ಯದಿ (AstAssertIntrinsic* ಧ್ರುವ assertp = VN_CAST(nodep, AssertIntrinsic)) {
            failsp = assertp->failsp();
        }
        ಯದಿ (failsp) failsp->unlinkFrBackWithNext();

        ತಾರ್ಕಿಕ selfDestruct = ಅಸತ್ಯ;
        ತಾರ್ಕಿಕ passspGated = ಅಸತ್ಯ;
        ಧ್ರುವ AstCover* ಧ್ರುವ coverp = VN_CAST(nodep, Cover);
        // A sequence event control is not an assertion directive; no assertion control
        ಧ್ರುವ ತಾರ್ಕಿಕ seqEvent = coverp && coverp->isSeqEvent();
        ಯದಿ (coverp) {
            ++m_statCover;
            ಯದಿ (seqEvent) {
                // Keep the event-fire action, with no coverage bucket
            } ಅನ್ಯಥಾ ಯದಿ (!v3Global.opt.coverageUser()) {
                selfDestruct = ಸತ್ಯ;
            } ಅನ್ಯಥಾ {
                // V3Coverage assigned us a bucket to increment.
                AstCoverInc* ಧ್ರುವ covincp = VN_AS(coverp->coverincsp(), CoverInc);
                UASSERT_OBJ(covincp, coverp, "Missing AstCoverInc under assertion");
                covincp->unlinkFrBackWithNext();  // next() might have  AstAssign for trace
                ಯದಿ (message != "") covincp->declp()->comment(message);
                ಯದಿ (passsp) {
                    passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                               /*vacuous=*/ಅಸತ್ಯ);
                    passspGated = ಸತ್ಯ;
                    passsp = Ast_ಗಂಟು::addNext<Ast_ಗಂಟು, Ast_ಗಂಟು>(covincp, passsp);
                } ಅನ್ಯಥಾ {
                    passsp = covincp;
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, Assert) || VN_IS(nodep, AssertIntrinsic)) {
            ಯದಿ (nodep->immediate()) {
                ++m_statAsImm;
            } ಅನ್ಯಥಾ {
                ++m_statAsNotImm;
            }
            ಯದಿ (!passsp && !failsp)
                failsp = newFireAssertUnchecked(
                    nodep, VN_IS(nodep, AssertIntrinsic) ? "'$cast' failed." : "'assert' failed.");
        } ಅನ್ಯಥಾ {
            nodep->v3fatalSrc("Unknown node type");
        }

        VL_RESTORER(m_passsp);
        VL_RESTORER(m_failsp);
        VL_RESTORER(m_assertp);
        m_passsp = passsp;
        m_failsp = failsp;
        m_assertp = nodep;
        iterate(nodep->propp());

        Ast_ಗಂಟು* propExprp;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* disablep = nullptr;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* matchCountp = nullptr;
        ಯದಿ (AstPropSpec* ಧ್ರುವ specp = VN_CAST(nodep->propp(), PropSpec)) {
            propExprp = specp->propp()->unlinkFrBack();
            ಯದಿ (specp->disablep()) disablep = specp->disablep()->unlinkFrBack();
            matchCountp = specp->matchCountp();
        } ಅನ್ಯಥಾ {
            propExprp = nodep->propp()->unlinkFrBack();
        }
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        ತಾರ್ಕಿಕ passspAlreadyGated = ಅಸತ್ಯ;
        ಯದಿ (passsp && VN_IS(passsp, If)) passspAlreadyGated = VN_AS(passsp, If)->user1();
        ಯದಿ (passsp && !passspGated && !passspAlreadyGated && !VN_IS(propExprp, PExpr)
            && !seqEvent) {
            passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                       /*vacuous=*/ಅಸತ್ಯ);
        }
        ಯದಿ (failsp && !VN_IS(propExprp, PExpr)) {
            failsp = newIfAssertFailOn(failsp, nodep->directive(), nodep->userType());
        }
        ಯದಿ (coverp && matchCountp && passsp) {
            // Convert the match count into a loop that decrements a temporary variable until it
            // reaches zero.
            matchCountp->unlinkFrBack();
            AstVar* ಧ್ರುವ remainingp = ನೂತನ AstVar{
                flp, VVarType::MODULETEMP, m_match_ಎಣಿಕೆ_Names.get(""), matchCountp->dtypep()};
            remainingp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remainingp);
            Ast_ಗಂಟು* ಧ್ರುವ replaysp
                = ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, remainingp, VAccess::WRITE}, matchCountp};
            AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{flp};
            loopp->addStmtsp(
                ನೂತನ AstLoopTest{flp, loopp, ನೂತನ AstVarRef{flp, remainingp, VAccess::READ}});
            loopp->addStmtsp(passsp);
            loopp->addStmtsp(
                ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, remainingp, VAccess::WRITE},
                              ನೂತನ AstSub{flp, ನೂತನ AstVarRef{flp, remainingp, VAccess::READ},
                                         ನೂತನ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{},
                                                      remainingp->dtypep()->width(), 1}}});
            replaysp->addNext(loopp);
            passsp = replaysp;
        }
        Ast_ಗಂಟು* bodysp = assertBody(nodep, propExprp, passsp, failsp);
        ಯದಿ (disablep) bodysp = ನೂತನ AstIf{flp, ನೂತನ AstLogNot{flp, disablep}, bodysp};
        // Add assertOn check last, for better combining
        ಯದಿ (!seqEvent) bodysp = newIfAssertOn(bodysp, nodep->directive(), nodep->userType());
        ಯದಿ (sentreep) bodysp = ನೂತನ AstAlways{flp, VAlwaysKwd::ALWAYS, sentreep, bodysp};

        ಯದಿ (passsp && !passsp->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(passsp), passsp);
        ಯದಿ (failsp && !failsp->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(failsp), failsp);

        // Install it
        ಯದಿ (selfDestruct) {
            // Delete it after making the tree.  This way we can tell the user
            // if it wasn't constructed nicely or has other errors without needing --coverage.
            VL_DO_DANGLING(bodysp->delete_ವೃಕ್ಷ(), bodysp);
            nodep->unlinkFrBack();
        } ಅನ್ಯಥಾ {
            nodep->replaceWith(bodysp);
        }
        // Bye
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ತಾರ್ಕಿಕ isEmptyStmt(Ast_ಗಂಟು* nodep) {
        ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (AstBegin* ಧ್ರುವ beginp = VN_CAST(nodep, Begin)) {
            ಯದಿ (beginp->declsp()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ಯದಿ (!isEmptyStmt(stmtp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstIf* nodep) override {
        ಯದಿ (!nodep->user1SetOnce()) {
            ಯದಿ (nodep->uniquePragma() || nodep->unique0Pragma()) {
                ಧ್ರುವ Ast_ಗಂಟು_If* ifp = nodep;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* propp = nullptr;
                ತಾರ್ಕಿಕ hasDefaultElse = ಅಸತ್ಯ;
                ಕುರು {
                    // If this statement ends with 'else if', then nextIf will point to the
                    // nextIf statement.  Otherwise it will be null.
                    ಧ್ರುವ Ast_ಗಂಟು_If* ಧ್ರುವ nextifp = dynamic_cast<Ast_ಗಂಟು_If*>(ifp->elsesp());
                    iterateAndNextNull(ifp->condp());

                    // Recurse into the true case.
                    iterateAndNextNull(ifp->thensp());

                    // If the last else is not an else if, recurse into that too.
                    ಯದಿ (ifp->elsesp() && !nextifp) {  //
                        iterateAndNextNull(ifp->elsesp());
                    }

                    // Build a bitmask of the true predicates
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ predp = ifp->condp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                    ಯದಿ (propp) {
                        propp = ನೂತನ AstConcat{nodep->fileline(), predp, propp};
                    } ಅನ್ಯಥಾ {
                        propp = predp;
                    }

                    // Record if this ends with an 'else' that does not have an if
                    ಯದಿ (ifp->elsesp() && !nextifp) hasDefaultElse = ಸತ್ಯ;

                    ifp = nextifp;
                } ಯಾವತ್ (ifp);

                ಧ್ರುವ ತಾರ್ಕಿಕ allow_none = nodep->unique0Pragma();

                // Empty case means no property
                ಯದಿ (!propp) propp = ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalse{}};

                // Note: if this ends with an 'else', then we don't need to validate that one of
                // the predicates evaluates to true.
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ohot
                    = ((allow_none || hasDefaultElse)
                           ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstOneHot0{nodep->fileline(), propp})
                           : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstOneHot{nodep->fileline(), propp}));
                ಧ್ರುವ VAssertType assertType
                    = nodep->uniquePragma() ? VAssertType::UNIQUE : VAssertType::UNIQUE0;
                // Move, not clone, as the branches might declare referenced variables
                VNRelinker relinker;
                nodep->unlinkFrBack(&relinker);
                AstIf* ಧ್ರುವ checkifp
                    = ನೂತನ AstIf{nodep->fileline(), ನೂತನ AstLogNot{nodep->fileline(), ohot},
                                newFireAssert(nodep, VAssertDirectiveType::VIOLATION_IF,
                                              assertType, "'unique if' statement violated"),
                                nodep};
                checkifp->isBoundsCheck(ಸತ್ಯ);  // To avoid LATCH warning
                checkifp->branchPred(VBranchPred::BP_UNLIKELY);
                relinker.relink(checkifp);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        iterateChildren(nodep);

        ಯದಿ (nodep->user2()) {
            // Combine consecutive assertOn checks if possible
            ಯದಿ (AstIf* ಧ್ರುವ backp = VN_CAST(nodep->backp(), If)) {
                ಯದಿ (backp->nextp() == nodep  //
                    && backp->user2()  //
                    && backp->condp()->same_ವೃಕ್ಷ(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    backp->addThensp(nodep->thensp()->unlinkFrBackWithNext());
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            // Combine nested assertOn checks if possible
            ಯದಿ (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* ಧ್ರುವ checkp = VN_CAST(nodep->thensp(), If);
                ಯದಿ (checkp  //
                    && checkp->user2()  //
                    && checkp->condp()->same_ವೃಕ್ಷ(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(checkp->unlinkFrBack()), checkp);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Swap assertOn check with single statement 'if' statement to bubble up for combining
        // Note we can't just swap the conditions as they two Ifs have different flags,
        // so swapping the Ifs themeselves then swapping back the bodies.
        ಯದಿ (nodep->condp()->isPure()) {
            ಯದಿ (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* ಧ್ರುವ checkp = VN_CAST(nodep->thensp(), If);
                ಯದಿ (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            ಯದಿ (nodep->elsesp() && !nodep->elsesp()->nextp() && isEmptyStmt(nodep->thensp())) {
                AstIf* ಧ್ರುವ checkp = VN_CAST(nodep->elsesp(), If);
                ಯದಿ (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addElsesp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }
    }

    //========== Case assertions
    ಶೂನ್ಯ visit(AstCase* nodep) override {
        // Introduce temporary variable for AstCase if needed - it is done here and not in V3Case
        // because this phase is before V3Scope and V3Case is not. Doing it before V3Scope ensures
        // that V3Scope will take care of a scope creation
        // We also need to do it before V3Begin, co that pragmas like `unique0` also work correctly
        ಯದಿ (!nodep->exprp()->isPure()) {
            ++m_statLiftedCaseExprs;
            File_ಸಾಲು* ಧ್ರುವ fl = nodep->exprp()->fileline();
            AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{fl, VVarType::BLOCKTEMP, m_caseTempNames.get(nodep),
                                            nodep->exprp()->dtypep()};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ origp = nodep->exprp()->unlinkFrBack();
            nodep->addHereThisAsNext(
                ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, varp, VAccess::WRITE}, origp});
            nodep->exprp(ನೂತನ AstVarRef{fl, varp, VAccess::READ});
            ಯದಿ (m_ftaskp) {
                varp->funcLocal(ಸತ್ಯ);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } ಅನ್ಯಥಾ {
                m_modp->prependStmtsp(varp);
            }
            VIsCached::clear_Cache_ವೃಕ್ಷ();
        }
        iterateChildren(nodep);
        ಯದಿ (!nodep->user1SetOnce()) {
            ತಾರ್ಕಿಕ has_default = ಅಸತ್ಯ;
            ಪ್ರತಿ (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                ಯದಿ (itemp->isDefault()) has_default = ಸತ್ಯ;
            }
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಅಭಿವ್ಯಕ್ತಿ_Dtypep = nodep->exprp()->dtypep()->skipRefp();

            VAssertType assertType = VAssertType::INTERNAL;
            ಯದಿ (nodep->priorityPragma()) {
                assertType = VAssertType::PRIORITY;
            } ಅನ್ಯಥಾ ಯದಿ (nodep->uniquePragma()) {
                assertType = VAssertType::UNIQUE;
            } ಅನ್ಯಥಾ ಯದಿ (nodep->unique0Pragma()) {
                assertType = VAssertType::UNIQUE0;
            }

            string valFmt;
            ಯದಿ (ಅಭಿವ್ಯಕ್ತಿ_Dtypep->isIntegralOrPacked())
                valFmt = " for '" + cvtToStr(ಅಭಿವ್ಯಕ್ತಿ_Dtypep->widthMin()) + "'h%X'";
            ಯದಿ (nodep->fullPragma() || nodep->priorityPragma()) {
                // Need to add a default if there isn't one already
                ++m_statAsFull;
                ಯದಿ (!has_default) {
                    nodep->addItemsp(ನೂತನ AstCaseItem{
                        nodep->fileline(), nullptr /*DEFAULT*/,
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      nodep->pragmaString() + ", but non-match found" + valFmt,
                                      valFmt.empty() ? nullptr
                                                     : nodep->exprp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ))});
                }
            }
            ಯದಿ (nodep->parallelPragma() || nodep->uniquePragma() || nodep->unique0Pragma()) {
                // Need to check that one, and only one of the case items match at any moment
                // If there's a default, we allow none to match, else exactly one must match
                ++m_statAsFull;
                ಯದಿ (!has_default && !nodep->itemsp()) {
                    // Not parallel, but harmlessly so.
                } ಅನ್ಯಥಾ {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* propp = nullptr;
                    ಪ್ರತಿ (AstCaseItem* itemp = nodep->itemsp(); itemp;
                         itemp = VN_AS(itemp->nextp(), CaseItem)) {
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* itembitp = nullptr;
                        ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* icondp = itemp->condsp(); icondp;
                             icondp = VN_AS(icondp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* onep;
                            ಯದಿ (AstInsideRange* ಧ್ರುವ rcondp = VN_CAST(icondp, InsideRange)) {
                                onep = rcondp->newAndFromInside(
                                    nodep->exprp()->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ),
                                    rcondp->lhsp()->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ),
                                    rcondp->rhsp()->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ));
                            } ಅನ್ಯಥಾ ಯದಿ (nodep->casex() || nodep->casez() || nodep->caseInside()) {
                                onep = AstEqWild::newTyped(itemp->fileline(),
                                                           nodep->exprp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                                                           icondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
                            } ಅನ್ಯಥಾ {
                                onep = AstEq::newTyped(icondp->fileline(),
                                                       nodep->exprp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                                                       icondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
                            }
                            // OR together all conditions within the same case item
                            ಯದಿ (onep) {
                                ಯದಿ (itembitp) {
                                    itembitp = ನೂತನ AstOr{icondp->fileline(), onep, itembitp};
                                } ಅನ್ಯಥಾ {
                                    itembitp = onep;
                                }
                            }
                        }
                        ಯದಿ (itembitp) {
                            ಯದಿ (propp) {
                                propp = ನೂತನ AstConcat{itemp->fileline(), itembitp, propp};
                            } ಅನ್ಯಥಾ {
                                propp = itembitp;
                            }
                        }
                    }
                    // Empty case means no property
                    ಯದಿ (!propp) propp = ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    ಧ್ರುವ ತಾರ್ಕಿಕ allow_none = has_default || nodep->unique0Pragma();
                    // The following assertion looks as below.
                    // if (!$onehot(propp)) begin
                    //     if (propp == '0) begin if (!allow_none) $error("none match"); end
                    //     else $error("multiple match");
                    // end
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ohot = ನೂತನ AstOneHot{nodep->fileline(), propp};
                    AstConst* ಧ್ರುವ zero = ನೂತನ AstConst{
                        nodep->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, propp->width(), 0};
                    AstIf* ಧ್ರುವ ohotIfp
                        = ನೂತನ AstIf{nodep->fileline(), ನೂತನ AstLogNot{nodep->fileline(), ohot}};
                    AstIf* ಧ್ರುವ zeroIfp = ನೂತನ AstIf{
                        nodep->fileline(),
                        ನೂತನ AstEq{nodep->fileline(), propp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), zero}};
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = nodep->exprp();
                    ಧ್ರುವ string pragmaStr = nodep->pragmaString();
                    ಯದಿ (!allow_none)
                        zeroIfp->addThensp(
                            newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                          pragmaStr + ", but none matched" + valFmt,
                                          valFmt.empty() ? nullptr : exprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)));
                    zeroIfp->addElsesp(
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      pragmaStr + ", but multiple matches found" + valFmt,
                                      valFmt.empty() ? nullptr : exprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)));
                    ohotIfp->addThensp(zeroIfp);
                    ohotIfp->isBoundsCheck(ಸತ್ಯ);  // To avoid LATCH warning
                    ohotIfp->branchPred(VBranchPred::BP_UNLIKELY);
                    nodep->addNotParallelp(ohotIfp);
                }
            }
        }
    }

    ಶೂನ್ಯ visit(AstFuture* nodep) override {
        nodep->v3error("Future sampled value function called outside property or sequence "
                       "expression (IEEE 16.9.4)");
        nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), 0});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    //========== Past
    ಶೂನ್ಯ visit(AstPast* nodep) override {
        ಯದಿ (!nodep->propertyTiming()) {
            nodep->initp(nodep->exprp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
            { Default_Sampled_ಮೌಲ್ಯ_Visitor{nodep->initp()}; }
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ initp = nodep->initp() ? nodep->initp()->unlinkFrBack() : nullptr;
        iterateChildren(nodep);
        uint32_t ticks = 1;
        ಯದಿ (nodep->ticksp()) {
            UASSERT_OBJ(VN_IS(nodep->ticksp(), Const), nodep,
                        "Expected constant ticks, checked in V3Width");
            ticks = VN_AS(nodep->ticksp(), Const)->toUInt();
        }
        UASSERT_OBJ(ticks >= 1, nodep, "0 tick should have been checked in V3Width");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = new_Sampled_ಅಭಿವ್ಯಕ್ತಿ(nodep->exprp()->unlinkFrBack());
        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep = nodep->sentreep()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* inp = nullptr;
        // Deliberately emitted for every ftask $past; calls outside final take the normal stage
        ಯದಿ (VN_IS(m_procedurep, Final) || m_ftaskp) {
            // A sampling-tick finish reads one stage deeper (IEEE 1800-2023 16.9.3).
            File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
            AstAlways* ಧ್ರುವ alwaysp
                = getDelayedAlways(exprp, initp ? initp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ) : nullptr, senTreep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ betweenTicksp = past_ಮೌಲ್ಯ_Ref(alwaysp, flp, ticks, initp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ onTickp = past_ಮೌಲ್ಯ_Ref(alwaysp, flp, ticks + 1, initp);
            ಯದಿ (initp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(initp), initp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ onTickCondp = ನೂತನ AstLogAnd{
                flp, ನೂತನ AstCExpr{flp, "vlSymsp->_vm_contextp__->executingFinal()", 1},
                ನೂತನ AstEq{flp, ನೂತನ AstVarRef{flp, getPastTickTimeVar(alwaysp), VAccess::READ},
                          ನೂತನ AstCExpr{flp, "vlSymsp->_vm_contextp__->finishPendingTime()", 64}}};
            AstCond* ಧ್ರುವ condp = ನೂತನ AstCond{flp, onTickCondp, onTickp, betweenTicksp};
            condp->dtypeFrom(onTickp);
            inp = condp;
        } ಅನ್ಯಥಾ {
            inp = get_Past_ಮೌಲ್ಯ(exprp, initp, senTreep, ticks);
        }
        nodep->replaceWith(inp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    //========== Move $sampled down to read-only variables
    ಶೂನ್ಯ visit(AstSampled* nodep) override {
        ಯದಿ (nodep->user1()) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_inSampled);
        {
            m_inSampled = ಸತ್ಯ;
            iterateChildren(nodep);
        }
        ಯದಿ (nodep->exprp()) {
            nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        } ಅನ್ಯಥಾ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಯದಿ (m_inSampled && !nodep->user2() && !nodep->varp()->isTemp()) {
            ಯದಿ (!nodep->access().isReadOnly()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Write to variable in sampled expression");
            } ಅನ್ಯಥಾ {
                VNRelinker relinkHandle;
                nodep->unlinkFrBack(&relinkHandle);
                AstSampled* ಧ್ರುವ newp = new_Sampled_ಅಭಿವ್ಯಕ್ತಿ(nodep);
                relinkHandle.relink(newp);
                newp->user1(1);
                v3Global.setHasSampled();
            }
        }
    }
    // Don't sample sensitivities
    ಶೂನ್ಯ visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSampled);
        m_inSampled = ಅಸತ್ಯ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstPExprClause* nodep) override {
        Ast_ಗಂಟು* stmtsp = nullptr;
        ಯದಿ (nodep->pass() && m_passsp) {
            // Cover adds COVERINC by AstNode::addNext, thus need to clone next too.
            stmtsp = m_passsp->clone_ವೃಕ್ಷ(ಸತ್ಯ);
            stmtsp = newIfAssertPassOn(stmtsp, m_assertp->directive(), m_assertp->userType(),
                                       nodep->vacuous());
        } ಅನ್ಯಥಾ ಯದಿ (!nodep->pass() && m_failsp) {
            stmtsp = m_failsp->clone_ವೃಕ್ಷ(ಸತ್ಯ);
            stmtsp = newIfAssertFailOn(stmtsp, m_assertp->directive(), m_assertp->userType());
        }
        ಯದಿ (stmtsp) {
            stmtsp->foreachAndNext([](Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ refp) {
                // References inside action blocks shouldn't be implicitly sampled
                // m_passsp/m_failsp have been already visited once and refs explicitly sampled
                // are handled already
                refp->user2(1);
            });
            nodep->replaceWith(stmtsp);
        } ಅನ್ಯಥಾ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstPExpr* nodep) override {
        ಯದಿ (m_inRestrict) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
        } ಅನ್ಯಥಾ {
            iterateChildren(nodep);
        }
    }

    //========== Statements
    ಶೂನ್ಯ visit(AstDisplay* nodep) override {
        iterateChildren(nodep);
        // Replace the special types with standard text
        ಯದಿ (nodep->displayType() == VDisplayType::DT_INFO) {
            replaceDisplay(nodep, "-Info");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->displayType() == VDisplayType::DT_WARNING) {
            replaceDisplay(nodep, "%%Warning");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->displayType() == VDisplayType::DT_ERROR) {
            replaceDisplay(nodep, "%%Error");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->displayType() == VDisplayType::DT_FATAL) {
            replaceDisplay(nodep, "%%Fatal");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->displayType() == VDisplayType::DT_MONITOR) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();

            AstSenItem* monSenItemsp = nullptr;
            ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ monExprsp = nodep->fmtp()->exprsp()) {
                monExprsp->foreachAndNext([&](AstVarRef* varrefp) {
                    AstSenItem* ಧ್ರುವ senItemp
                        = ನೂತನ AstSenItem{fl, VEdgeType::ET_CHANGED,
                                         // Clone so get VarRef or VarXRef as needed
                                         varrefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)};
                    monSenItemsp = Ast_ಗಂಟು::addNextNull(monSenItemsp, senItemp);
                });
            }

            Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ mon_Sen_ವೃಕ್ಷ = ನೂತನ Ast_Sen_ವೃಕ್ಷ{fl, monSenItemsp};
            ಧ್ರುವ auto mon_ಅಂಕೆ = ++m_monitor_ಅಂಕೆ;
            // Where $monitor was we do "__VmonitorNum = N;"
            AstAssign* ಧ್ರುವ newsetp = ನೂತನ AstAssign{
                fl, new_Monitor_ಅಂಕೆ_Var_Refp(nodep, VAccess::WRITE), ನೂತನ AstConst{fl, mon_ಅಂಕೆ}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__VmonitorOn && __VmonitorNum==N) $display(...);"
            Ast_ಗಂಟು* ಧ್ರುವ stmtsp = nodep;
            AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{
                fl,
                ನೂತನ AstLogAnd{fl, ನೂತನ AstLogNot{fl, newMonitorOffVarRefp(nodep, VAccess::READ)},
                              ನೂತನ AstEq{fl, ನೂತನ AstConst{fl, mon_ಅಂಕೆ},
                                        new_Monitor_ಅಂಕೆ_Var_Refp(nodep, VAccess::READ)}},
                stmtsp};
            ifp->isBoundsCheck(ಸತ್ಯ);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstAlways{fl, VAlwaysKwd::ALWAYS, mon_Sen_ವೃಕ್ಷ, ifp};
            m_modp->addStmtsp(newp);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->displayType() == VDisplayType::DT_STROBE) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            // Need one-shot
            File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
            AstVar* ಧ್ರುವ varp
                = ನೂತನ AstVar{fl, VVarType::MODULETEMP, "__Vstrobe" + cvtToStr(m_mod_Strobe_ಅಂಕೆ++),
                             nodep->findBitDType()};
            m_modp->addStmtsp(varp);
            // Where $strobe was we do "__Vstrobe = '1;"
            AstAssign* ಧ್ರುವ newsetp = ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, varp, VAccess::WRITE},
                                                     ನೂತನ AstConst{fl, AstConst::BitTrue{}}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__Vstrobe) begin $display(...); __Vstrobe = '0; end"
            Ast_ಗಂಟು* ಧ್ರುವ stmtsp = nodep;
            AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{fl, ನೂತನ AstVarRef{fl, varp, VAccess::READ}, stmtsp};
            ifp->isBoundsCheck(ಸತ್ಯ);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstAlwaysPostponed{fl, ifp};
            stmtsp->addNext(ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, varp, VAccess::WRITE},
                                          ನೂತನ AstConst{fl, AstConst::BitFalse{}}});
            m_modp->addStmtsp(newp);
        }
    }
    ಶೂನ್ಯ visit(AstMonitorOff* nodep) override {
        AstAssign* ಧ್ರುವ newp
            = ನೂತನ AstAssign{nodep->fileline(), newMonitorOffVarRefp(nodep, VAccess::WRITE),
                            ನೂತನ AstConst{nodep->fileline(), AstConst::BitTrue{}, nodep->off()}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstAssert* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    ಶೂನ್ಯ visit(AstAssertCtl* nodep) override {
        iterateChildren(nodep);

        ತಾರ್ಕಿಕ assertTypeConst = ಸತ್ಯ;
        ಯದಿ (!nodep->assertTypesp()) {
            nodep->assertTypes(VAssertType{ALL_ASSERT_TYPES});
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ assertTypesp = VN_CAST(nodep->assertTypesp(), Const)) {
            nodep->assertTypes(VAssertType{assertTypesp->toSInt()});
        } ಅನ್ಯಥಾ {
            assertTypeConst = ಅಸತ್ಯ;
        }

        ತಾರ್ಕಿಕ controlTypeConst = ಅಸತ್ಯ;
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep->controlTypep(), Const)) {
            nodep->ctlType(constp->toSInt());
            controlTypeConst = ಸತ್ಯ;
        }
        ಯದಿ (controlTypeConst
            && (nodep->ctlType() < VAssertCtlType::LOCK
                || nodep->ctlType() > VAssertCtlType::VACUOUS_OFF)) {
            nodep->unlinkFrBack();
            nodep->v3error("Bad $assertcontrol control_type '"
                           << cvtToStr(static_cast<ಪೂರ್ಣಾಙ್ಕ>(nodep->ctlType()))
                           << "' (IEEE 1800-2023 Table 20-5)");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (assertTypeConst && nodep->assertTypes() != ALL_ASSERT_TYPES
            && nodep->assertTypes().containsAny(VAssertType::UNIQUE | VAssertType::UNIQUE0
                                                | VAssertType::PRIORITY)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: assert control assertion_type");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        ತಾರ್ಕಿಕ directiveTypeConst = ಸತ್ಯ;
        ಯದಿ (!nodep->directiveTypesp()) {
            nodep->directiveTypes(VAssertDirectiveType::ASSERT | VAssertDirectiveType::ASSUME
                                  | VAssertDirectiveType::COVER);
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ directiveTypesp
                   = VN_CAST(nodep->directiveTypesp(), Const)) {
            nodep->directiveTypes(VAssertDirectiveType{directiveTypesp->toSInt()});
        } ಅನ್ಯಥಾ {
            directiveTypeConst = ಅಸತ್ಯ;
        }
        ಯದಿ (!directiveTypeConst) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: non-const assert directive type expression");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        UINFO(9, "Generating assertctl for a module: " << m_modp);
        AstCStmt* ಧ್ರುವ newp = ನೂತನ AstCStmt{fl};
        newp->add("vlSymsp->_vm_contextp__->assertCtl(");
        newp->add(nodep->controlTypep()->unlinkFrBack());
        newp->add(", ");
        ಯದಿ (nodep->assertTypesp()) {
            newp->add(nodep->assertTypesp()->unlinkFrBack());
        } ಅನ್ಯಥಾ {
            newp->add(std::to_string(ALL_ASSERT_TYPES));
        }
        newp->add(", " + std::to_string(nodep->directiveTypes()) + ");\n");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstAssertIntrinsic* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    ಶೂನ್ಯ visit(AstCover* nodep) override {  //
        visitAssertionIterate(nodep, nullptr);
    }
    ಶೂನ್ಯ visit(AstRestrict* nodep) override {
        VL_RESTORER(m_inRestrict);
        m_inRestrict = ಸತ್ಯ;
        iterateChildren(nodep);
        // IEEE says simulator ignores these
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Past_ಅಂಕೆ);
        VL_RESTORER(m_mod_Strobe_ಅಂಕೆ);
        VL_RESTORER(m_finalp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DelayedAlwaysp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DefaultDelayedAlwaysp);
        VL_RESTORER_CLEAR(m_delayedAlways2TickTimep);
        m_modp = nodep;
        m_mod_Past_ಅಂಕೆ = 0;
        m_mod_Strobe_ಅಂಕೆ = 0;
        m_finalp = nullptr;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstGenBlock* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstBegin* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
        // If the body is a single assertOn check, bubble it up for combining
        ಯದಿ (!nodep->declsp()) {
            ಯದಿ (AstIf* ಧ್ರುವ ifp = VN_CAST(nodep->stmtsp(), If)) {
                ಯದಿ (ifp->user2() && !ifp->nextp()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(ifp->unlinkFrBack());
                    nodep->addStmtsp(ifp->thensp()->unlinkFrBackWithNext());
                    ifp->addThensp(nodep);
                }
            }
        }
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
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
        VIsCached::clear_Cache_ವೃಕ್ಷ();
    }
};

//######################################################################
// Top Assert class

ಶೂನ್ಯ V3Assert::assertAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("assert", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
