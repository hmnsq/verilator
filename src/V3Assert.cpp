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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Assert.h"

#ଅନ୍ତର୍ଭାଵ "V3AstUserAllocator.h"
#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ନାମାକାଶ {

ଶ୍ରେଣୀ DefaultDisableLocalVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // STATE
    Ast_ଗଣ୍ଠି* m_scopep = nullptr;

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(nullptr);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(nullptr);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstDefaultDisable* nodep) override {
        UASSERT_OBJ(nodep, m_scopep,
                    "default disable iff must be inside a module or generate block");
        AstDefaultDisable* defaultp = nullptr;
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = VN_CAST(m_scopep, ଗଣ୍ଠି_Module)) {
            defaultp = modp->defaultDisablep();
        } ଅନ୍ୟଥା {
            defaultp = VN_AS(m_scopep, GenBlock)->defaultDisablep();
        }
        ଯଦି (VL_UNLIKELY(defaultp)) {
            nodep->v3error("Only one 'default disable iff' allowed per "
                           << (VN_IS(m_scopep, ଗଣ୍ଠି_Module) ? "module" : "generate block")
                           << " (IEEE 1800-2023 16.15)");
        } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = VN_CAST(m_scopep, ଗଣ୍ଠି_Module)) {
            modp->defaultDisablep(nodep);
        } ଅନ୍ୟଥା {
            VN_AS(m_scopep, GenBlock)->defaultDisablep(nodep);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    explicit DefaultDisableLocalVisitor(AstNetlist* nodep) { iterate(nodep); }
};

ଶ୍ରେଣୀ DefaultDisablePropagateVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // STATE
    AstDefaultDisable* m_defaultDisablep = nullptr;

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        ଯଦି (!nodep->defaultDisablep()) nodep->defaultDisablep(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    explicit DefaultDisablePropagateVisitor(AstNetlist* nodep) { iterate(nodep); }
};

// Lower a sequence used as an event control ('@seq', IEEE 1800-2023 9.4.2.4) into a
// synthesized event fired by an internal 'cover sequence' on each end-of-match
ଶ୍ରେଣୀ SeqEventLowerVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // STATE
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    V3UniqueNames m_names{"__VseqEvent"};  // Synthesized event names

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstSenItem* nodep) override {
        AstFuncRef* ଅଚଳ funcrefp = VN_CAST(nodep->sensp(), FuncRef);
        ଯଦି (funcrefp && VN_IS(funcrefp->taskp(), Sequence)) {
            File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
            AstVar* ଅଚଳ eventp = ନୂତନ AstVar{flp, VVarType::MODULETEMP, m_names.get(nodep),
                                              m_modp->findBasicDType(VBasicDTypeKwd::EVENT)};
            eventp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(eventp);
            v3Global.setHasEvents();
            funcrefp->unlinkFrBack();
            nodep->sensp(ନୂତନ AstVarRef{flp, eventp, VAccess::READ});
            ଅଚଳ ତାର୍କିକ automaticActual = funcrefp->exists([](ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* refp) {
                ଫେରନ୍ତୁ refp->varp() && refp->varp()->lifetime().isAutomatic();
            });
            ଯଦି (automaticActual) {
                nodep->v3error("Arguments to a sequence used as an event control must be"
                               " static (IEEE 1800-2023 9.4.2.4)");
                VN_AS(funcrefp->taskp(), Sequence)->isReferenced(ମିଥ୍ୟା);
                VL_DO_DANGLING(ଠେଲ_Deletep(funcrefp), funcrefp);
                ଫେରନ୍ତୁ;
            }
            AstFireEvent* ଅଚଳ firep
                = ନୂତନ AstFireEvent{flp, ନୂତନ AstVarRef{flp, eventp, VAccess::WRITE}, ମିଥ୍ୟା};
            AstCover* ଅଚଳ coverp
                = ନୂତନ AstCover{flp, ନୂତନ AstPropSpec{flp, nullptr, nullptr, funcrefp}, firep,
                               VAssertType::CONCURRENT};
            coverp->isCoverSeq(ସତ୍ୟ);
            coverp->isSeqEvent(ସତ୍ୟ);
            m_modp->addStmtsp(coverp);
            ଫେରନ୍ତୁ;
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    explicit SeqEventLowerVisitor(AstNetlist* nodep) { iterate(nodep); }
};

}  // namespace

ଶୂନ୍ୟ V3AssertCommon::collectDefaultDisable(AstNetlist* nodep) {
    { DefaultDisableLocalVisitor{nodep}; }
    { DefaultDisablePropagateVisitor{nodep}; }
}

ଶୂନ୍ୟ V3AssertCommon::lowerSequenceEvents(AstNetlist* nodep) {
    { SeqEventLowerVisitor{nodep}; }
    V3Global::dump_Check_Global_ବୃକ୍ଷ("assertseqevent", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}

string V3AssertCommon::assertCtlGetCall(ଅଚଳ ଅକ୍ଷର* query, VAssertType type,
                                        VAssertDirectiveType directiveType) {
    // Template arguments let the runtime build the type/directive mask at C++ compile time
    ଫେରନ୍ତୁ "vlSymsp->_vm_contextp__->assertCtlGet<"s + std::to_string(type) + ", "s
           + std::to_string(directiveType) + ">(VerilatedAssertCtlQuery::"s + query + ")"s;
}

ତାର୍କିକ V3AssertCommon::check_Sva_ମୂଲ୍ୟ_Limit(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ AstConst* constp, ଅଚଳ ଅକ୍ଷର* what) {
    // Temporal values can materialize as O(N) AST or state in later passes.
    // Leave headroom for a range ring's extra slot and V3Number's signed
    // `(bits + 31) / 32` word count.
    ସ୍ଥିର constexpr unsigned SVA_ମୂଲ୍ୟ_HARD_LIMIT
        = static_cast<unsigned>(std::numeric_limits<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>::max()) - 32U;
    ଅଚଳ unsigned configuredLimit = static_cast<unsigned>(v3Global.opt.max_ଅଙ୍କ_Width());
    ଅଚଳ unsigned limit = std::min(configuredLimit, SVA_ମୂଲ୍ୟ_HARD_LIMIT);
    ଯଦି (constp->ଅଙ୍କ().fitsInUInt() && constp->toUInt() <= limit) ଫେରନ୍ତୁ ସତ୍ୟ;
    nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << what << " exceeds implementation limit of "
                                                 << limit
                                                 << (configuredLimit <= SVA_ମୂଲ୍ୟ_HARD_LIMIT
                                                         ? " (--max-num-width)"
                                                         : " (host arithmetic limit)"));
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
}

// The bound of a cycle delay in a sequence, if a non-negative elaboration-time constant
ସ୍ଥିର ଅଚଳ AstConst* checkSvaDelayBound(Ast_ଗଣ୍ଠି* errp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* boundp, ଅଚଳ ଅକ୍ଷର* what) {
    ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(boundp, Const);
    ଯଦି (!constp || (constp->dtypep()->isSigned() && constp->ଅଙ୍କ().isNegative())) {
        errp->v3error(what << " is not a non-negative elaboration-time constant"
                              " (IEEE 1800-2023 16.7)");
        ଫେରନ୍ତୁ nullptr;
    }
    ଯଦି (!V3AssertCommon::check_Sva_ମୂଲ୍ୟ_Limit(errp, constp, "SVA cycle delay")) ଫେରନ୍ତୁ nullptr;
    ଫେରନ୍ତୁ constp;
}

// Fold the bounds of a cycle delay in a sequence, and return if they are valid, else report it
// on errp
ସ୍ଥିର ତାର୍କିକ svaCycleDelayValid(ଅଚଳ AstDelay* nodep, Ast_ଗଣ୍ଠି* errp) {
    V3Const::constifyParamsNoWarnEdit(nodep->lhsp());
    ଅଚଳ AstConst* ଅଚଳ minConstp = checkSvaDelayBound(
        errp, nodep->lhsp(), nodep->isRangeDelay() ? "Range delay minimum" : "Delay value");
    ଯଦି (!nodep->rhsp() || nodep->isUnbounded()) ଫେରନ୍ତୁ minConstp != nullptr;
    V3Const::constifyParamsNoWarnEdit(nodep->rhsp());
    ଅଚଳ AstConst* ଅଚଳ maxConstp
        = checkSvaDelayBound(errp, nodep->rhsp(), "Range delay maximum");
    ଯଦି (!minConstp || !maxConstp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଯଦି (maxConstp->toUInt() < minConstp->toUInt()) {
        errp->v3error("Range delay maximum must be >= minimum (IEEE 1800-2023 16.7)");
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ଫେରନ୍ତୁ ସତ୍ୟ;
}

ଶୂନ୍ୟ V3AssertCommon::checkSvaCycleDelay(AstDelay* nodep, Ast_ଗଣ୍ଠି* errp) {
    ଯଦି (svaCycleDelayValid(nodep, errp)) ଫେରନ୍ତୁ;
    // Make the delay valid, so later steps do not fail after the error
    File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ minp = nodep->lhsp();
    minp->replaceWith(ନୂତନ AstConst{flp, AstConst::BitFalseErroring{}});
    VL_DO_DANGLING(minp->delete_ବୃକ୍ଷ(), minp);
    ଯଦି (nodep->rhsp() && !nodep->isUnbounded()) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ maxp = nodep->rhsp();
        maxp->replaceWith(ନୂତନ AstConst{flp, AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(maxp->delete_ବୃକ୍ଷ(), maxp);
    }
}

ତାର୍କିକ V3AssertCommon::refsSvaFormal(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
    ଫେରନ୍ତୁ nodep->exists([](ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* refp) {
        ଅଚଳ AstVar* ଅଚଳ varp = refp->varp();
        ଯଦି (!varp || !varp->isIO()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ declp = varp->aboveLoopp();
        ଫେରନ୍ତୁ VN_IS(declp, Sequence) || VN_IS(declp, Property);
    });
}

ଶୂନ୍ୟ V3AssertCommon::checkSvaInstanceDelays(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି* instp) {
    // A delay still referencing a formal argument of an enclosing sequence or property is
    // checked when that is instanced. Collect first, as checking edits the delays.
    std::vector<AstDelay*> delayps;
    nodep->foreach([&](AstDelay* delayp) {
        ଯଦି (!delayp->isCycleDelay() || refsSvaFormal(delayp->lhsp())) ଫେରନ୍ତୁ;
        ଯଦି (delayp->rhsp() && refsSvaFormal(delayp->rhsp())) ଫେରନ୍ତୁ;
        delayps.ଠେଲ_back(delayp);
    });
    ପାଇଁ (AstDelay* ଅଚଳ delayp : delayps) checkSvaCycleDelay(delayp, instp);
}

//######################################################################
// AssertDeFutureVisitor
// If any AstFuture, then move all non-future varrefs to be one cycle behind,
// see IEEE 1800-2023 16.9.4.

ଶ୍ରେଣୀ AssertDeFutureVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // STATE - across all visitors
    Ast_ଗଣ୍ଠି_Module* ଅଚଳ m_modp;  // Module future is underneath
    ଅଚଳ AstFuture* m_futurep;  // First AstFuture found
    ଅଚଳ unsigned m_past_ଅଙ୍କ;  // Prefix unique number for this module
    std::map<AstVar*, AstVar*> m_delayedVars;  // Old to delayed variable mapping
    // STATE - for current visit position (use VL_RESTORER)
    ତାର୍କିକ m_inFuture = ମିଥ୍ୟା;  // Inside a future
    ତାର୍କିକ m_unsupported = ମିଥ୍ୟା;  // Printed unsupported

    // METHODS
    ଶୂନ୍ୟ unsupported(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (m_unsupported) ଫେରନ୍ତୁ;
        m_unsupported = ସତ୍ୟ;
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported/illegal: Future value function used with expression with "
                          << nodep->prettyOperatorName());
    }
    // VISITORS
    ଶୂନ୍ୟ visit(AstFuture* nodep) override {
        VL_RESTORER(m_inFuture);
        m_inFuture = ସତ୍ୟ;
        iterateChildren(nodep);
        // Done with the future, this subexpression is current-time
        nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;
        ଯଦି (m_inFuture || m_unsupported)
            ଫେରନ୍ତୁ;  // Need user1 set above, don't process when Future is removed
        ଯଦି (nodep->access().isWriteOrRW()) {
            unsupported(nodep);
            ଫେରନ୍ତୁ;
        }
        auto it = m_delayedVars.find(nodep->varp());
        AstVar* outvarp;
        ଯଦି (it == m_delayedVars.end()) {
            Ast_Sen_ବୃକ୍ଷ* ଅଚଳ sentreep = m_futurep->sentreep();
            AstAlways* ଅଚଳ alwaysp = ନୂତନ AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS,
                                                     sentreep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), nullptr};
            m_modp->addStmtsp(alwaysp);
            outvarp = ନୂତନ AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                 "__Vnotfuture" + cvtToStr(m_past_ଅଙ୍କ) + "_" + nodep->name(),
                                 nodep->dtypep()};
            m_modp->addStmtsp(outvarp);
            AstVarRef* varRefAWritep = ନୂତନ AstVarRef{nodep->fileline(), outvarp, VAccess::WRITE};
            varRefAWritep->user1(ସତ୍ୟ);
            Ast_ଗଣ୍ଠି_Var_Ref* varRefAReadp = nodep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            varRefAReadp->user1(ସତ୍ୟ);
            Ast_ଗଣ୍ଠି* ଅଚଳ assp = ନୂତନ AstAssignDly{nodep->fileline(), varRefAWritep, varRefAReadp};
            alwaysp->addStmtsp(assp);
            m_delayedVars.emplace(nodep->varp(), outvarp);
        } ଅନ୍ୟଥା {
            outvarp = it->second;
        }
        AstVarRef* newp = ନୂତନ AstVarRef{nodep->fileline(), outvarp, VAccess::READ};
        newp->user1(ସତ୍ୟ);
        UINFO(9, "DeFuture " << nodep << "  becomes " << newp);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override { unsupported(nodep); }
    ଶୂନ୍ୟ visit(AstMethodCall* nodep) override { unsupported(nodep); }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        ଯଦି (!nodep->isPure()) unsupported(nodep);
        iterateChildren(nodep);
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit AssertDeFutureVisitor(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି_Module* modp, unsigned past_ଅଙ୍କ)
        : m_modp{modp}
        , m_past_ଅଙ୍କ{past_ଅଙ୍କ} {
        // See if any Future before we process
        ଯଦି (nodep->forall([&](ଅଚଳ AstFuture* futurep) -> ତାର୍କିକ {
                m_futurep = futurep;
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }))
            ଫେରନ୍ତୁ;
        // UINFOTREE(9, nodep, "", "defuture-in");
        visit(nodep);  // Nodep may get deleted
        // UINFOTREE(9, nodep, "", "defuture-ou");
    }
    ~AssertDeFutureVisitor() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

// Prepare one default sampled expression.
ଶ୍ରେଣୀ Default_Sampled_ମୂଲ୍ୟ_Visitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // METHODS
    ସ୍ଥିର AstConst* newTypeDefault(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        AstConst* ଅଚଳ constp
            = ନୂତନ AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()};
        ଯଦି (nodep->dtypep()->isFourstate()) constp->ଅଙ୍କ().setAllBitsX();
        constp->dtypeFrom(nodep);
        ଫେରନ୍ତୁ constp;
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstCMethodHard* nodep) override {
        ଯଦି (nodep->method() == VCMethod::EVENT_IS_TRIGGERED) {
            nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା {
            iterateChildren(nodep);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        AstVar* ଅଚଳ varp = nodep->varp();
        ଯଦି (varp->isParam()
            || (!varp->isNet() && varp->lifetime().isStatic() && varp->hasUserInit())) {
            ଫେରନ୍ତୁ;
        }
        nodep->replaceWith(newTypeDefault(nodep));
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    explicit Default_Sampled_ମୂଲ୍ୟ_Visitor(Ast_ଗଣ୍ଠି* nodep) { iterate(nodep); }
};

//######################################################################
// AssertVisitor

ଶ୍ରେଣୀ AssertVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // CONSTANTS
    ସ୍ଥିର constexpr uint8_t ALL_ASSERT_TYPES
        = std::numeric_limits<std::underlying_type<VAssertType::en>::type>::max();

    // NODE STATE/TYPES
    // Cleared on netlist
    //  AstNode::user1()         -> bool.  True if processed
    //  AstAlways::user2p()      -> std::vector<AstVar*>. Delayed variables via 'm_delayed'
    //  AstNodeVarRef::user2()   -> bool.  True if shouldn't be sampled
    //  AstIf::user2()           -> bool.  True for assertOn() checks inserted in this pass
    ଅଚଳ VNUser1InUse m_user1InUse;
    ଅଚଳ VNUser2InUse m_user2InUse;
    AstUser2Allocator<AstAlways, std::vector<AstVar*>> m_delayed;

    // STATE
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Last module
    ଅଚଳ Ast_ଗଣ୍ଠି* m_beginp = nullptr;  // Last AstBegin/AstGenBlock
    unsigned m_monitor_ଅଙ୍କ = 0;  // Global $monitor numbering (not per module)
    AstVar* m_monitor_ଅଙ୍କ_Varp = nullptr;  // $monitor number variable
    AstVar* m_monitorOffVarp = nullptr;  // $monitoroff variable
    unsigned m_mod_Past_ଅଙ୍କ = 0;  // Module past numbering
    unsigned m_mod_Strobe_ଅଙ୍କ = 0;  // Module $strobe numbering
    Ast_ଗଣ୍ଠି_Procedure* m_procedurep = nullptr;  // Current procedure
    VDouble0 m_statCover;  // Statistic tracking
    VDouble0 m_statAsNotImm;  // Statistic tracking
    VDouble0 m_statAsImm;  // Statistic tracking
    VDouble0 m_statAsFull;  // Statistic tracking
    VDouble0 m_statPastVars;  // Statistic tracking
    VDouble0 m_statAssertOnCombined;  // Statistic tracking
    VDouble0 m_statAssertOnHoisted;  // Statistic tracking
    ତାର୍କିକ m_inSampled = ମିଥ୍ୟା;  // True inside a sampled expression
    ତାର୍କିକ m_inRestrict = ମିଥ୍ୟା;  // True inside restrict assertion
    Ast_ଗଣ୍ଠି* m_passsp = nullptr;  // Current pass statement
    Ast_ଗଣ୍ଠି* m_failsp = nullptr;  // Current fail statement
    Ast_ଗଣ୍ଠି_Cover_Or_Assert* m_assertp = nullptr;  // Current assertion
    AstFinal* m_finalp = nullptr;  // Current final block
    VDouble0 m_statLiftedCaseExprs;  // Count of purified case expressions
    Ast_ଗଣ୍ଠି_FTask* m_ftaskp = nullptr;  // Current function/task
    V3UniqueNames m_caseTempNames{"__VCase"};
    V3UniqueNames m_match_ଗଣନା_Names{"__VnfaRemainingMatchCount"};  // Match replay counter names
    // Maps from (expression, senTree) to the AstAlways that computes its delayed values.
    std::unordered_map<VNRef<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି>, std::unordered_map<VNRef<Ast_Sen_ବୃକ୍ଷ>, AstAlways*>>
        m_modExpr2Sen2DelayedAlwaysp;
    // Source sampled-value history is initialized, unlike internal property-lowering history.
    std::unordered_map<VNRef<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି>, std::unordered_map<VNRef<Ast_Sen_ବୃକ୍ଷ>, AstAlways*>>
        m_modExpr2Sen2DefaultDelayedAlwaysp;
    // Map from delayed-value AstAlways to its last-sampling-tick time variable
    std::unordered_map<ଅଚଳ AstAlways*, AstVar*> m_delayedAlways2TickTimep;

    // METHODS
    ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* assertPassOnQuery(ତାର୍କିକ vacuous) {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* queries[2]
            = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
        ଫେରନ୍ତୁ queries[vacuous];
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* assertOnCond(File_ଧାଡ଼ି* fl, VAssertType type,
                                     VAssertDirectiveType directiveType) {
        // cppcheck-suppress missingReturn
        ବାଛ (directiveType) {
        ସ୍ଥିତି VAssertDirectiveType::INTRINSIC: ଫେରନ୍ତୁ ନୂତନ AstConst{fl, AstConst::BitTrue{}};
        ସ୍ଥିତି VAssertDirectiveType::VIOLATION_CASE: {
            ଯଦି (v3Global.opt.assertCase()) {
                ଫେରନ୍ତୁ ନୂତନ AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            // If assertions are off, have constant propagation rip them out later
            // This allows syntax errors and such to be detected normally.
            ଫେରନ୍ତୁ ନୂତନ AstConst{fl, AstConst::BitFalse{}};
        }
        ସ୍ଥିତି VAssertDirectiveType::ASSERT:
        ସ୍ଥିତି VAssertDirectiveType::COVER:
        ସ୍ଥିତି VAssertDirectiveType::ASSUME: {
            ଯଦି (v3Global.opt.assertOn()) {
                ଫେରନ୍ତୁ ନୂତନ AstCExpr{
                    fl, AstCExpr::Pure{},
                    V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
            }
            ଫେରନ୍ତୁ ନୂତନ AstConst{fl, AstConst::BitFalse{}};
        }
        ସ୍ଥିତି VAssertDirectiveType::INTERNAL:
        ସ୍ଥିତି VAssertDirectiveType::VIOLATION_IF:
        ସ୍ଥିତି VAssertDirectiveType::RESTRICT: {
            ଯଦି (v3Global.opt.assertOn()) {
                ଫେରନ୍ତୁ ନୂତନ AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            ଫେରନ୍ତୁ ନୂତନ AstConst{fl, AstConst::BitFalse{}};
        }
        }
        VL_UNREACHABLE;
    }

    ସ୍ଥିର ତାର୍କିକ isControlled(VAssertDirectiveType directiveType) {
        ଫେରନ୍ତୁ (static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(directiveType)
                & (static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(VAssertDirectiveType::ASSERT)
                   | static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(VAssertDirectiveType::COVER)
                   | static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(VAssertDirectiveType::ASSUME)));
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* assertPassOnCond(File_ଧାଡ଼ି* fl, VAssertType type,
                                         VAssertDirectiveType directiveType, ତାର୍କିକ vacuous) {
        ଯଦି (!isControlled(directiveType)) ଫେରନ୍ତୁ ନୂତନ AstConst{fl, AstConst::BitTrue{}};
        ଯଦି (!v3Global.opt.assertOn()) ଫେରନ୍ତୁ ନୂତନ AstConst{fl, AstConst::BitFalse{}};
        ଫେରନ୍ତୁ ନୂତନ AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType), 1};
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* assertFailOnCond(File_ଧାଡ଼ି* fl, VAssertType type,
                                         VAssertDirectiveType directiveType) {
        ଯଦି (!isControlled(directiveType)) ଫେରନ୍ତୁ ନୂତନ AstConst{fl, AstConst::BitTrue{}};
        ଯଦି (!v3Global.opt.assertOn()) ଫେରନ୍ତୁ ନୂତନ AstConst{fl, AstConst::BitFalse{}};
        ଫେରନ୍ତୁ ନୂତନ AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType), 1};
    }
    string assertDisplayMessage(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& prefix, ଅଚଳ string& message,
                                VDisplayType severity) {
        ଯଦି (severity == VDisplayType::DT_ERROR || severity == VDisplayType::DT_FATAL) {
            ଫେରନ୍ତୁ ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                    + cvtToStr(nodep->fileline()->lineno()) + ": Assertion failed in %m"
                    + ((message != "") ? ": " : "") + message + "\n");
        }
        ଫେରନ୍ତୁ ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                + cvtToStr(nodep->fileline()->lineno()) + ": %m" + ((message != "") ? ": " : "")
                + message + "\n");
    }
    ଶୂନ୍ୟ replaceDisplay(AstDisplay* nodep, ଅଚଳ string& prefix) {
        nodep->fmtp()->text(
            assertDisplayMessage(nodep, prefix, nodep->fmtp()->text(), nodep->displayType()));
        nodep->displayType(VDisplayType::DT_WRITE);
        // cppcheck-suppress nullPointer
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ timenewp = ନୂତନ AstTime{nodep->fileline(), m_modp->timeunit()};
        nodep->fmtp()->prependExprsp(timenewp);
        ଯଦି (!nodep->fmtp()->scopeNamep() && nodep->fmtp()->formatScopeTracking()) {
            nodep->fmtp()->scopeNamep(ନୂତନ AstScopeName{nodep->fileline(), ସତ୍ୟ});
        }
    }
    AstSampled* new_Sampled_ପଦସମଷ୍ଟି(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଫେରନ୍ତୁ ନୂତନ AstSampled{nodep->fileline(), nodep, nodep->dtypep(), ସତ୍ୟ};
    }
    AstVarRef* new_Monitor_ଅଙ୍କ_Var_Refp(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, VAccess access) {
        ଯଦି (!m_monitor_ଅଙ୍କ_Varp) {
            m_monitor_ଅଙ୍କ_Varp = ନୂତନ AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorNum",
                                          nodep->findUInt64DType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitor_ଅଙ୍କ_Varp);
        }
        AstVarRef* ଅଚଳ varrefp = ନୂତନ AstVarRef{nodep->fileline(), m_monitor_ଅଙ୍କ_Varp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        ଫେରନ୍ତୁ varrefp;
    }
    AstVarRef* newMonitorOffVarRefp(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, VAccess access) {
        ଯଦି (!m_monitorOffVarp) {
            m_monitorOffVarp = ନୂତନ AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorOff",
                                          nodep->findBitDType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitorOffVarp);
        }
        AstVarRef* ଅଚଳ varrefp = ନୂତନ AstVarRef{nodep->fileline(), m_monitorOffVarp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        ଫେରନ୍ତୁ varrefp;
    }
    ସ୍ଥିର AstIf* newIfAssertOn(Ast_ଗଣ୍ଠି* bodyp, VAssertDirectiveType directiveType,
                                VAssertType type = VAssertType::INTERNAL) {
        // Add an internal if to check assertions are on.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_ଧାଡ଼ି* ଅଚଳ fl = bodyp->fileline();

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = assertOnCond(fl, type, directiveType);
        AstIf* ଅଚଳ newp = ନୂତନ AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(ସତ୍ୟ);  // To avoid LATCH warning
        newp->user2(ସତ୍ୟ);  // Mark as an assertOn() check
        ଫେରନ୍ତୁ newp;
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_Stmt* newIfAssertPassOn(Ast_ଗଣ୍ଠି* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type, ତାର୍କିକ vacuous) {
        // Add an internal if to check assertion passOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_ଧାଡ଼ି* ଅଚଳ fl = bodyp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = assertPassOnCond(fl, type, directiveType, vacuous);
        Ast_ଗଣ୍ଠି_If* ଅଚଳ newp = ନୂତନ AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(ସତ୍ୟ);  // To avoid LATCH warning
        newp->user1(ସତ୍ୟ);  // Don't assert/cover this if
        newp->user2(ସତ୍ୟ);  // Mark as an assertOn() check
        ଫେରନ୍ତୁ newp;
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_Stmt* newIfAssertFailOn(Ast_ଗଣ୍ଠି* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type) {
        // Add an internal if to check assertion failOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_ଧାଡ଼ି* ଅଚଳ fl = bodyp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = assertFailOnCond(fl, type, directiveType);
        Ast_ଗଣ୍ଠି_If* ଅଚଳ newp = ନୂତନ AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(ସତ୍ୟ);  // To avoid LATCH warning
        newp->user1(ସତ୍ୟ);  // Don't assert/cover this if
        newp->user2(ସତ୍ୟ);  // Mark as an assertOn() check
        ଫେରନ୍ତୁ newp;
    }

    ସ୍ଥିର AstIf* assertCond(ଅଚଳ Ast_ଗଣ୍ଠି_Cover_Or_Assert* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* propp,
                             Ast_ଗଣ୍ଠି* passsp, Ast_ଗଣ୍ଠି* failsp) {
        AstIf* ଅଚଳ ifp = ନୂତନ AstIf{nodep->fileline(), propp, passsp, failsp};
        // It's more LIKELY that we'll take the nullptr if clause
        // than the sim-killing else clause:
        ifp->branchPred(VBranchPred::BP_LIKELY);
        ifp->isBoundsCheck(ସତ୍ୟ);  // To avoid LATCH warning
        ଫେରନ୍ତୁ ifp;
    }

    Ast_ଗଣ୍ଠି_Stmt* assertBody(ଅଚଳ Ast_ଗଣ୍ଠି_Cover_Or_Assert* nodep, Ast_ଗଣ୍ଠି* propp, Ast_ଗଣ୍ଠି* passsp,
                            Ast_ଗଣ୍ଠି* failsp) {
        ଯଦି (AstPExpr* ଅଚଳ pexprp = VN_CAST(propp, PExpr)) {
            ଯଦି (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NOTIMING, "This property expression requires --timing");
                VL_DO_DANGLING(ଠେଲ_Deletep(pexprp), pexprp);
                ଫେରନ୍ତୁ ନୂତନ AstBegin{nodep->fileline(), "", nullptr, ମିଥ୍ୟା};
            }
            AstFork* ଅଚଳ forkp = ନୂତନ AstFork{nodep->fileline(), VJoinType::JOIN_NONE};
            forkp->addForksp(pexprp->bodyp()->unlinkFrBack());
            ଯଦି (Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ finalp = pexprp->finalp()) {
                ଯଦି (!m_finalp) {
                    m_finalp = ନୂତନ AstFinal{m_modp->fileline(), finalp->unlinkFrBack()};
                    m_modp->addStmtsp(m_finalp);
                } ଅନ୍ୟଥା {
                    m_finalp->addStmtsp(finalp->unlinkFrBack());
                }
            }
            VL_DO_DANGLING2(ଠେଲ_Deletep(pexprp), pexprp, propp);
            ଫେରନ୍ତୁ forkp;
        }

        ଫେରନ୍ତୁ assertCond(nodep, VN_AS(propp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି), passsp, failsp);
    }

    Ast_ଗଣ୍ଠି_Stmt* newFireAssertUnchecked(ଅଚଳ Ast_ଗଣ୍ଠି_Stmt* nodep, ଅଚଳ string& message,
                                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprsp = nullptr) {
        // Like newFireAssert() but omits the asserts-on check
        AstDisplay* ଅଚଳ dispp
            = ନୂତନ AstDisplay{nodep->fileline(), VDisplayType::DT_ERROR, message, nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ bodysp = dispp;
        replaceDisplay(dispp, "%%Error");  // Convert to standard DISPLAY format
        ଯଦି (exprsp) dispp->fmtp()->exprsp()->addNext(exprsp);
        ଯଦି (v3Global.opt.stopFail()) bodysp->addNext(ନୂତନ AstStop{nodep->fileline(), ମିଥ୍ୟା});
        ଫେରନ୍ତୁ bodysp;
    }

    Ast_ଗଣ୍ଠି_Stmt* newFireAssert(ଅଚଳ Ast_ଗଣ୍ଠି_Stmt* nodep, VAssertDirectiveType directiveType,
                               VAssertType assertType, ଅଚଳ string& message,
                               Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprsp = nullptr) {
        Ast_ଗଣ୍ଠି_Stmt* bodysp = newFireAssertUnchecked(nodep, message, exprsp);
        bodysp = newIfAssertOn(bodysp, directiveType, assertType);
        ଫେରନ୍ତୁ bodysp;
    }

    AstVar* createDelayedVar(ଅଚଳ std::string& name, AstAlways* alwaysp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp,
                             Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* initp) {
        File_ଧାଡ଼ି* ଅଚଳ flp = exprp->fileline();
        AstVar* ଅଚଳ varp = ନୂତନ AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        // TODO: this lifetime seems nonsene (can't have NBAs to automatics), but is as before
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->addStmtsp(varp);
        // Before enough clock ticks, $past returns the expression's default sampled value.
        ଯଦି (initp) {
            m_modp->addStmtsp(ନୂତନ AstInitialStatic{
                flp, ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, varp, VAccess::WRITE}, initp}});
        }
        ++m_statPastVars;
        // Actually set the delayed value
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = ନୂତନ AstVarRef{flp, varp, VAccess::WRITE};
        AstAssignDly* ଅଚଳ assignp = ନୂତନ AstAssignDly{flp, lhsp, exprp};
        alwaysp->prependStmtsp(assignp);
        ଫେରନ୍ତୁ varp;
    }

    AstAlways* getDelayedAlways(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* initp, Ast_Sen_ବୃକ୍ଷ* senTreep) {
        auto& expr2Sen2Alwaysr
            = initp ? m_modExpr2Sen2DefaultDelayedAlwaysp : m_modExpr2Sen2DelayedAlwaysp;
        AstAlways*& alwayspr = expr2Sen2Alwaysr[*exprp][*senTreep];
        ଯଦି (!alwayspr) {
            File_ଧାଡ଼ି* ଅଚଳ flp = exprp->fileline();
            // Create the always block that computes the delayed values
            alwayspr = ନୂତନ AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, nullptr};
            m_modp->addStmtsp(alwayspr);
            // Create the once-delayed variable
            ଅଚଳ std::string name = "_Vpast_" + cvtToStr(m_mod_Past_ଅଙ୍କ++) + "_1";
            AstVar* ଅଚଳ varp = createDelayedVar(name, alwayspr, exprp, initp);
            // Add it to delayed variable vector
            m_delayed(alwayspr).emplace_back(varp);
        } ଅନ୍ୟଥା {
            // Reusing exiting, not needed
            VL_DO_DANGLING(ଠେଲ_Deletep(exprp), exprp);
            ଯଦି (initp) VL_DO_DANGLING(ଠେଲ_Deletep(initp), initp);
            VL_DO_DANGLING(ଠେଲ_Deletep(senTreep), senTreep);
        }
        ଫେରନ୍ତୁ alwayspr;
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* get_Past_ମୂଲ୍ୟ(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* initp, Ast_Sen_ବୃକ୍ଷ* senTreep,
                              uint32_t ticks) {
        UASSERT_OBJ(ticks > 0, exprp, "Delay must be > 0");
        File_ଧାଡ଼ି* ଅଚଳ flp = exprp->fileline();
        AstAlways* ଅଚଳ alwaysp
            = getDelayedAlways(exprp, initp ? initp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା) : nullptr, senTreep);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ resultp = past_ମୂଲ୍ୟ_Ref(alwaysp, flp, ticks, initp);
        ଯଦି (initp) VL_DO_DANGLING(ଠେଲ_Deletep(initp), initp);
        ଫେରନ୍ତୁ resultp;
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* past_ମୂଲ୍ୟ_Ref(AstAlways* alwaysp, File_ଧାଡ଼ି* flp, uint32_t ticks,
                              Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* initp) {
        std::vector<AstVar*>& delayedr = m_delayed(alwaysp);
        // Ensure the required delay exists
        ଯେତେବେଳେ (delayedr.size() < ticks) {
            AstVar* ଅଚଳ firstp = delayedr.front();
            File_ଧାଡ଼ି* ଅଚଳ varFlp = firstp->fileline();
            // Create once more delayed value
            std::string name = firstp->name();
            name.resize(name.size() - 1);
            name += std::to_string(delayedr.size() + 1);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ prevp = ନୂତନ AstVarRef{varFlp, delayedr.back(), VAccess::READ};
            AstVar* ଅଚଳ varp = createDelayedVar(name, alwaysp, prevp,
                                                  initp ? initp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା) : nullptr);
            // Add it to delayed variable vector
            delayedr.emplace_back(varp);
        }
        // Return a reference to the appropriately delayed variable
        ଫେରନ୍ତୁ ନୂତନ AstVarRef{flp, delayedr.at(ticks - 1), VAccess::READ};
    }

    // Time of the pipeline's last sampling tick, for end-of-simulation readers
    AstVar* getPastTickTimeVar(AstAlways* alwaysp) {
        AstVar*& varpr = m_delayedAlways2TickTimep[alwaysp];
        ଯଦି (!varpr) {
            File_ଧାଡ଼ି* ଅଚଳ flp = alwaysp->fileline();
            varpr = ନୂତନ AstVar{flp, VVarType::MODULETEMP,
                               "_Vpast_" + cvtToStr(m_mod_Past_ଅଙ୍କ++) + "_t",
                               m_modp->findUInt64DType()};
            m_modp->addStmtsp(varpr);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ timep = ନୂତନ AstCExpr{flp, "vlSymsp->_vm_contextp__->time()", 64};
            alwaysp->addStmtsp(
                ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, varpr, VAccess::WRITE}, timep});
        }
        ଫେରନ୍ତୁ varpr;
    }

    ଶୂନ୍ୟ visitAssertionIterate(Ast_ଗଣ୍ଠି_Cover_Or_Assert* nodep, Ast_ଗଣ୍ଠି* failsp) {
        ଯଦି (m_beginp && nodep->name() == "") nodep->name(m_beginp->name());

        { AssertDeFutureVisitor{nodep->propp(), m_modp, m_mod_Past_ଅଙ୍କ++}; }

        iterateAndNextNull(nodep->sentreep());
        ଯଦି (AstAssert* ଅଚଳ assertp = VN_CAST(nodep, Assert)) {
            iterateAndNextNull(assertp->failsp());
        } ଅନ୍ୟଥା ଯଦି (AstAssertIntrinsic* ଅଚଳ assertp = VN_CAST(nodep, AssertIntrinsic)) {
            iterateAndNextNull(assertp->failsp());
        } ଅନ୍ୟଥା ଯଦି (AstCover* ଅଚଳ coverp = VN_CAST(nodep, Cover)) {
            iterateAndNextNull(coverp->coverincsp());
        } ଅନ୍ୟଥା ଯଦି (!VN_IS(nodep, Restrict)) {
            nodep->v3fatalSrc("Unhandled assert type");
        }
        iterateAndNextNull(nodep->passsp());
        Ast_Sen_ବୃକ୍ଷ* sentreep = nodep->sentreep();
        ଯଦି (nodep->immediate()) {
            UASSERT_OBJ(!sentreep, nodep, "Immediate assertions don't have sensitivity");
        } ଅନ୍ୟଥା {
            UASSERT_OBJ(sentreep, nodep, "Concurrent assertions must have sensitivity");
            // Explicit inline clock differs from the enclosing always: hoist
            // and warn. To support this need queue of asserts to activate.
            ଯଦି (m_procedurep && !nodep->senFromAlways()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Procedural concurrent assertion with"
                              " clocking event inside always (IEEE 1800-2023 16.14.6)");
                static_cast<Ast_ଗଣ୍ଠି*>(m_procedurep)->addNext(nodep->unlinkFrBack());
                ଫେରନ୍ତୁ;  // Later iterate will pick up
            }
            sentreep->unlinkFrBack();
            ଯଦି (m_procedurep) { VL_DO_DANGLING(ଠେଲ_Deletep(sentreep), sentreep); }
        }
        //
        ଅଚଳ string& message = nodep->name();
        Ast_ଗଣ୍ଠି* passsp = nodep->passsp();
        ଯଦି (passsp) passsp->unlinkFrBackWithNext();
        ଯଦି (AstAssert* ଅଚଳ assertp = VN_CAST(nodep, Assert)) {
            failsp = assertp->failsp();
        } ଅନ୍ୟଥା ଯଦି (AstAssertIntrinsic* ଅଚଳ assertp = VN_CAST(nodep, AssertIntrinsic)) {
            failsp = assertp->failsp();
        }
        ଯଦି (failsp) failsp->unlinkFrBackWithNext();

        ତାର୍କିକ selfDestruct = ମିଥ୍ୟା;
        ତାର୍କିକ passspGated = ମିଥ୍ୟା;
        ଅଚଳ AstCover* ଅଚଳ coverp = VN_CAST(nodep, Cover);
        // A sequence event control is not an assertion directive; no assertion control
        ଅଚଳ ତାର୍କିକ seqEvent = coverp && coverp->isSeqEvent();
        ଯଦି (coverp) {
            ++m_statCover;
            ଯଦି (seqEvent) {
                // Keep the event-fire action, with no coverage bucket
            } ଅନ୍ୟଥା ଯଦି (!v3Global.opt.coverageUser()) {
                selfDestruct = ସତ୍ୟ;
            } ଅନ୍ୟଥା {
                // V3Coverage assigned us a bucket to increment.
                AstCoverInc* ଅଚଳ covincp = VN_AS(coverp->coverincsp(), CoverInc);
                UASSERT_OBJ(covincp, coverp, "Missing AstCoverInc under assertion");
                covincp->unlinkFrBackWithNext();  // next() might have  AstAssign for trace
                ଯଦି (message != "") covincp->declp()->comment(message);
                ଯଦି (passsp) {
                    passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                               /*vacuous=*/ମିଥ୍ୟା);
                    passspGated = ସତ୍ୟ;
                    passsp = Ast_ଗଣ୍ଠି::addNext<Ast_ଗଣ୍ଠି, Ast_ଗଣ୍ଠି>(covincp, passsp);
                } ଅନ୍ୟଥା {
                    passsp = covincp;
                }
            }
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, Assert) || VN_IS(nodep, AssertIntrinsic)) {
            ଯଦି (nodep->immediate()) {
                ++m_statAsImm;
            } ଅନ୍ୟଥା {
                ++m_statAsNotImm;
            }
            ଯଦି (!passsp && !failsp)
                failsp = newFireAssertUnchecked(
                    nodep, VN_IS(nodep, AssertIntrinsic) ? "'$cast' failed." : "'assert' failed.");
        } ଅନ୍ୟଥା {
            nodep->v3fatalSrc("Unknown node type");
        }

        VL_RESTORER(m_passsp);
        VL_RESTORER(m_failsp);
        VL_RESTORER(m_assertp);
        m_passsp = passsp;
        m_failsp = failsp;
        m_assertp = nodep;
        iterate(nodep->propp());

        Ast_ଗଣ୍ଠି* propExprp;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* disablep = nullptr;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* matchCountp = nullptr;
        ଯଦି (AstPropSpec* ଅଚଳ specp = VN_CAST(nodep->propp(), PropSpec)) {
            propExprp = specp->propp()->unlinkFrBack();
            ଯଦି (specp->disablep()) disablep = specp->disablep()->unlinkFrBack();
            matchCountp = specp->matchCountp();
        } ଅନ୍ୟଥା {
            propExprp = nodep->propp()->unlinkFrBack();
        }
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        ତାର୍କିକ passspAlreadyGated = ମିଥ୍ୟା;
        ଯଦି (passsp && VN_IS(passsp, If)) passspAlreadyGated = VN_AS(passsp, If)->user1();
        ଯଦି (passsp && !passspGated && !passspAlreadyGated && !VN_IS(propExprp, PExpr)
            && !seqEvent) {
            passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                       /*vacuous=*/ମିଥ୍ୟା);
        }
        ଯଦି (failsp && !VN_IS(propExprp, PExpr)) {
            failsp = newIfAssertFailOn(failsp, nodep->directive(), nodep->userType());
        }
        ଯଦି (coverp && matchCountp && passsp) {
            // Convert the match count into a loop that decrements a temporary variable until it
            // reaches zero.
            matchCountp->unlinkFrBack();
            AstVar* ଅଚଳ remainingp = ନୂତନ AstVar{
                flp, VVarType::MODULETEMP, m_match_ଗଣନା_Names.get(""), matchCountp->dtypep()};
            remainingp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remainingp);
            Ast_ଗଣ୍ଠି* ଅଚଳ replaysp
                = ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, remainingp, VAccess::WRITE}, matchCountp};
            AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{flp};
            loopp->addStmtsp(
                ନୂତନ AstLoopTest{flp, loopp, ନୂତନ AstVarRef{flp, remainingp, VAccess::READ}});
            loopp->addStmtsp(passsp);
            loopp->addStmtsp(
                ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, remainingp, VAccess::WRITE},
                              ନୂତନ AstSub{flp, ନୂତନ AstVarRef{flp, remainingp, VAccess::READ},
                                         ନୂତନ AstConst{flp, AstConst::Widthed_ମୂଲ୍ୟ{},
                                                      remainingp->dtypep()->width(), 1}}});
            replaysp->addNext(loopp);
            passsp = replaysp;
        }
        Ast_ଗଣ୍ଠି* bodysp = assertBody(nodep, propExprp, passsp, failsp);
        ଯଦି (disablep) bodysp = ନୂତନ AstIf{flp, ନୂତନ AstLogNot{flp, disablep}, bodysp};
        // Add assertOn check last, for better combining
        ଯଦି (!seqEvent) bodysp = newIfAssertOn(bodysp, nodep->directive(), nodep->userType());
        ଯଦି (sentreep) bodysp = ନୂତନ AstAlways{flp, VAlwaysKwd::ALWAYS, sentreep, bodysp};

        ଯଦି (passsp && !passsp->backp()) VL_DO_DANGLING(ଠେଲ_Deletep(passsp), passsp);
        ଯଦି (failsp && !failsp->backp()) VL_DO_DANGLING(ଠେଲ_Deletep(failsp), failsp);

        // Install it
        ଯଦି (selfDestruct) {
            // Delete it after making the tree.  This way we can tell the user
            // if it wasn't constructed nicely or has other errors without needing --coverage.
            VL_DO_DANGLING(bodysp->delete_ବୃକ୍ଷ(), bodysp);
            nodep->unlinkFrBack();
        } ଅନ୍ୟଥା {
            nodep->replaceWith(bodysp);
        }
        // Bye
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    ତାର୍କିକ isEmptyStmt(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (!nodep) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (AstBegin* ଅଚଳ beginp = VN_CAST(nodep, Begin)) {
            ଯଦି (beginp->declsp()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ଯଦି (!isEmptyStmt(stmtp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstIf* nodep) override {
        ଯଦି (!nodep->user1SetOnce()) {
            ଯଦି (nodep->uniquePragma() || nodep->unique0Pragma()) {
                ଅଚଳ Ast_ଗଣ୍ଠି_If* ifp = nodep;
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* propp = nullptr;
                ତାର୍କିକ hasDefaultElse = ମିଥ୍ୟା;
                କର {
                    // If this statement ends with 'else if', then nextIf will point to the
                    // nextIf statement.  Otherwise it will be null.
                    ଅଚଳ Ast_ଗଣ୍ଠି_If* ଅଚଳ nextifp = dynamic_cast<Ast_ଗଣ୍ଠି_If*>(ifp->elsesp());
                    iterateAndNextNull(ifp->condp());

                    // Recurse into the true case.
                    iterateAndNextNull(ifp->thensp());

                    // If the last else is not an else if, recurse into that too.
                    ଯଦି (ifp->elsesp() && !nextifp) {  //
                        iterateAndNextNull(ifp->elsesp());
                    }

                    // Build a bitmask of the true predicates
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ predp = ifp->condp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                    ଯଦି (propp) {
                        propp = ନୂତନ AstConcat{nodep->fileline(), predp, propp};
                    } ଅନ୍ୟଥା {
                        propp = predp;
                    }

                    // Record if this ends with an 'else' that does not have an if
                    ଯଦି (ifp->elsesp() && !nextifp) hasDefaultElse = ସତ୍ୟ;

                    ifp = nextifp;
                } ଯେତେବେଳେ (ifp);

                ଅଚଳ ତାର୍କିକ allow_none = nodep->unique0Pragma();

                // Empty case means no property
                ଯଦି (!propp) propp = ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalse{}};

                // Note: if this ends with an 'else', then we don't need to validate that one of
                // the predicates evaluates to true.
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ ohot
                    = ((allow_none || hasDefaultElse)
                           ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstOneHot0{nodep->fileline(), propp})
                           : static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstOneHot{nodep->fileline(), propp}));
                ଅଚଳ VAssertType assertType
                    = nodep->uniquePragma() ? VAssertType::UNIQUE : VAssertType::UNIQUE0;
                // Move, not clone, as the branches might declare referenced variables
                VNRelinker relinker;
                nodep->unlinkFrBack(&relinker);
                AstIf* ଅଚଳ checkifp
                    = ନୂତନ AstIf{nodep->fileline(), ନୂତନ AstLogNot{nodep->fileline(), ohot},
                                newFireAssert(nodep, VAssertDirectiveType::VIOLATION_IF,
                                              assertType, "'unique if' statement violated"),
                                nodep};
                checkifp->isBoundsCheck(ସତ୍ୟ);  // To avoid LATCH warning
                checkifp->branchPred(VBranchPred::BP_UNLIKELY);
                relinker.relink(checkifp);
                ଫେରନ୍ତୁ;
            }
        }

        iterateChildren(nodep);

        ଯଦି (nodep->user2()) {
            // Combine consecutive assertOn checks if possible
            ଯଦି (AstIf* ଅଚଳ backp = VN_CAST(nodep->backp(), If)) {
                ଯଦି (backp->nextp() == nodep  //
                    && backp->user2()  //
                    && backp->condp()->same_ବୃକ୍ଷ(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    backp->addThensp(nodep->thensp()->unlinkFrBackWithNext());
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଫେରନ୍ତୁ;
                }
            }
            // Combine nested assertOn checks if possible
            ଯଦି (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* ଅଚଳ checkp = VN_CAST(nodep->thensp(), If);
                ଯଦି (checkp  //
                    && checkp->user2()  //
                    && checkp->condp()->same_ବୃକ୍ଷ(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    VL_DO_DANGLING(ଠେଲ_Deletep(checkp->unlinkFrBack()), checkp);
                    ଫେରନ୍ତୁ;
                }
            }
            ଫେରନ୍ତୁ;
        }

        // Swap assertOn check with single statement 'if' statement to bubble up for combining
        // Note we can't just swap the conditions as they two Ifs have different flags,
        // so swapping the Ifs themeselves then swapping back the bodies.
        ଯଦି (nodep->condp()->isPure()) {
            ଯଦି (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* ଅଚଳ checkp = VN_CAST(nodep->thensp(), If);
                ଯଦି (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    ଫେରନ୍ତୁ;
                }
            }
            ଯଦି (nodep->elsesp() && !nodep->elsesp()->nextp() && isEmptyStmt(nodep->thensp())) {
                AstIf* ଅଚଳ checkp = VN_CAST(nodep->elsesp(), If);
                ଯଦି (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addElsesp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    ଫେରନ୍ତୁ;
                }
            }
        }
    }

    //========== Case assertions
    ଶୂନ୍ୟ visit(AstCase* nodep) override {
        // Introduce temporary variable for AstCase if needed - it is done here and not in V3Case
        // because this phase is before V3Scope and V3Case is not. Doing it before V3Scope ensures
        // that V3Scope will take care of a scope creation
        // We also need to do it before V3Begin, co that pragmas like `unique0` also work correctly
        ଯଦି (!nodep->exprp()->isPure()) {
            ++m_statLiftedCaseExprs;
            File_ଧାଡ଼ି* ଅଚଳ fl = nodep->exprp()->fileline();
            AstVar* ଅଚଳ varp = ନୂତନ AstVar{fl, VVarType::BLOCKTEMP, m_caseTempNames.get(nodep),
                                            nodep->exprp()->dtypep()};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ origp = nodep->exprp()->unlinkFrBack();
            nodep->addHereThisAsNext(
                ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, varp, VAccess::WRITE}, origp});
            nodep->exprp(ନୂତନ AstVarRef{fl, varp, VAccess::READ});
            ଯଦି (m_ftaskp) {
                varp->funcLocal(ସତ୍ୟ);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } ଅନ୍ୟଥା {
                m_modp->prependStmtsp(varp);
            }
            VIsCached::clear_Cache_ବୃକ୍ଷ();
        }
        iterateChildren(nodep);
        ଯଦି (!nodep->user1SetOnce()) {
            ତାର୍କିକ has_default = ମିଥ୍ୟା;
            ପାଇଁ (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                ଯଦି (itemp->isDefault()) has_default = ସତ୍ୟ;
            }
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ପଦସମଷ୍ଟି_Dtypep = nodep->exprp()->dtypep()->skipRefp();

            VAssertType assertType = VAssertType::INTERNAL;
            ଯଦି (nodep->priorityPragma()) {
                assertType = VAssertType::PRIORITY;
            } ଅନ୍ୟଥା ଯଦି (nodep->uniquePragma()) {
                assertType = VAssertType::UNIQUE;
            } ଅନ୍ୟଥା ଯଦି (nodep->unique0Pragma()) {
                assertType = VAssertType::UNIQUE0;
            }

            string valFmt;
            ଯଦି (ପଦସମଷ୍ଟି_Dtypep->isIntegralOrPacked())
                valFmt = " for '" + cvtToStr(ପଦସମଷ୍ଟି_Dtypep->widthMin()) + "'h%X'";
            ଯଦି (nodep->fullPragma() || nodep->priorityPragma()) {
                // Need to add a default if there isn't one already
                ++m_statAsFull;
                ଯଦି (!has_default) {
                    nodep->addItemsp(ନୂତନ AstCaseItem{
                        nodep->fileline(), nullptr /*DEFAULT*/,
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      nodep->pragmaString() + ", but non-match found" + valFmt,
                                      valFmt.empty() ? nullptr
                                                     : nodep->exprp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା))});
                }
            }
            ଯଦି (nodep->parallelPragma() || nodep->uniquePragma() || nodep->unique0Pragma()) {
                // Need to check that one, and only one of the case items match at any moment
                // If there's a default, we allow none to match, else exactly one must match
                ++m_statAsFull;
                ଯଦି (!has_default && !nodep->itemsp()) {
                    // Not parallel, but harmlessly so.
                } ଅନ୍ୟଥା {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* propp = nullptr;
                    ପାଇଁ (AstCaseItem* itemp = nodep->itemsp(); itemp;
                         itemp = VN_AS(itemp->nextp(), CaseItem)) {
                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* itembitp = nullptr;
                        ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* icondp = itemp->condsp(); icondp;
                             icondp = VN_AS(icondp->nextp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) {
                            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* onep;
                            ଯଦି (AstInsideRange* ଅଚଳ rcondp = VN_CAST(icondp, InsideRange)) {
                                onep = rcondp->newAndFromInside(
                                    nodep->exprp()->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ),
                                    rcondp->lhsp()->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ),
                                    rcondp->rhsp()->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ));
                            } ଅନ୍ୟଥା ଯଦି (nodep->casex() || nodep->casez() || nodep->caseInside()) {
                                onep = AstEqWild::newTyped(itemp->fileline(),
                                                           nodep->exprp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                                                           icondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
                            } ଅନ୍ୟଥା {
                                onep = AstEq::newTyped(icondp->fileline(),
                                                       nodep->exprp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                                                       icondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
                            }
                            // OR together all conditions within the same case item
                            ଯଦି (onep) {
                                ଯଦି (itembitp) {
                                    itembitp = ନୂତନ AstOr{icondp->fileline(), onep, itembitp};
                                } ଅନ୍ୟଥା {
                                    itembitp = onep;
                                }
                            }
                        }
                        ଯଦି (itembitp) {
                            ଯଦି (propp) {
                                propp = ନୂତନ AstConcat{itemp->fileline(), itembitp, propp};
                            } ଅନ୍ୟଥା {
                                propp = itembitp;
                            }
                        }
                    }
                    // Empty case means no property
                    ଯଦି (!propp) propp = ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    ଅଚଳ ତାର୍କିକ allow_none = has_default || nodep->unique0Pragma();
                    // The following assertion looks as below.
                    // if (!$onehot(propp)) begin
                    //     if (propp == '0) begin if (!allow_none) $error("none match"); end
                    //     else $error("multiple match");
                    // end
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ ohot = ନୂତନ AstOneHot{nodep->fileline(), propp};
                    AstConst* ଅଚଳ zero = ନୂତନ AstConst{
                        nodep->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{}, propp->width(), 0};
                    AstIf* ଅଚଳ ohotIfp
                        = ନୂତନ AstIf{nodep->fileline(), ନୂତନ AstLogNot{nodep->fileline(), ohot}};
                    AstIf* ଅଚଳ zeroIfp = ନୂତନ AstIf{
                        nodep->fileline(),
                        ନୂତନ AstEq{nodep->fileline(), propp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), zero}};
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = nodep->exprp();
                    ଅଚଳ string pragmaStr = nodep->pragmaString();
                    ଯଦି (!allow_none)
                        zeroIfp->addThensp(
                            newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                          pragmaStr + ", but none matched" + valFmt,
                                          valFmt.empty() ? nullptr : exprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)));
                    zeroIfp->addElsesp(
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      pragmaStr + ", but multiple matches found" + valFmt,
                                      valFmt.empty() ? nullptr : exprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)));
                    ohotIfp->addThensp(zeroIfp);
                    ohotIfp->isBoundsCheck(ସତ୍ୟ);  // To avoid LATCH warning
                    ohotIfp->branchPred(VBranchPred::BP_UNLIKELY);
                    nodep->addNotParallelp(ohotIfp);
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(AstFuture* nodep) override {
        nodep->v3error("Future sampled value function called outside property or sequence "
                       "expression (IEEE 16.9.4)");
        nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), 0});
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    //========== Past
    ଶୂନ୍ୟ visit(AstPast* nodep) override {
        ଯଦି (!nodep->propertyTiming()) {
            nodep->initp(nodep->exprp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
            { Default_Sampled_ମୂଲ୍ୟ_Visitor{nodep->initp()}; }
        }
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ initp = nodep->initp() ? nodep->initp()->unlinkFrBack() : nullptr;
        iterateChildren(nodep);
        uint32_t ticks = 1;
        ଯଦି (nodep->ticksp()) {
            UASSERT_OBJ(VN_IS(nodep->ticksp(), Const), nodep,
                        "Expected constant ticks, checked in V3Width");
            ticks = VN_AS(nodep->ticksp(), Const)->toUInt();
        }
        UASSERT_OBJ(ticks >= 1, nodep, "0 tick should have been checked in V3Width");
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = new_Sampled_ପଦସମଷ୍ଟି(nodep->exprp()->unlinkFrBack());
        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep = nodep->sentreep()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* inp = nullptr;
        // Deliberately emitted for every ftask $past; calls outside final take the normal stage
        ଯଦି (VN_IS(m_procedurep, Final) || m_ftaskp) {
            // A sampling-tick finish reads one stage deeper (IEEE 1800-2023 16.9.3).
            File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
            AstAlways* ଅଚଳ alwaysp
                = getDelayedAlways(exprp, initp ? initp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା) : nullptr, senTreep);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ betweenTicksp = past_ମୂଲ୍ୟ_Ref(alwaysp, flp, ticks, initp);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ onTickp = past_ମୂଲ୍ୟ_Ref(alwaysp, flp, ticks + 1, initp);
            ଯଦି (initp) VL_DO_DANGLING(ଠେଲ_Deletep(initp), initp);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ onTickCondp = ନୂତନ AstLogAnd{
                flp, ନୂତନ AstCExpr{flp, "vlSymsp->_vm_contextp__->executingFinal()", 1},
                ନୂତନ AstEq{flp, ନୂତନ AstVarRef{flp, getPastTickTimeVar(alwaysp), VAccess::READ},
                          ନୂତନ AstCExpr{flp, "vlSymsp->_vm_contextp__->finishPendingTime()", 64}}};
            AstCond* ଅଚଳ condp = ନୂତନ AstCond{flp, onTickCondp, onTickp, betweenTicksp};
            condp->dtypeFrom(onTickp);
            inp = condp;
        } ଅନ୍ୟଥା {
            inp = get_Past_ମୂଲ୍ୟ(exprp, initp, senTreep, ticks);
        }
        nodep->replaceWith(inp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    //========== Move $sampled down to read-only variables
    ଶୂନ୍ୟ visit(AstSampled* nodep) override {
        ଯଦି (nodep->user1()) ଫେରନ୍ତୁ;
        VL_RESTORER(m_inSampled);
        {
            m_inSampled = ସତ୍ୟ;
            iterateChildren(nodep);
        }
        ଯଦି (nodep->exprp()) {
            nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        } ଅନ୍ୟଥା {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        ଯଦି (m_inSampled && !nodep->user2() && !nodep->varp()->isTemp()) {
            ଯଦି (!nodep->access().isReadOnly()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Write to variable in sampled expression");
            } ଅନ୍ୟଥା {
                VNRelinker relinkHandle;
                nodep->unlinkFrBack(&relinkHandle);
                AstSampled* ଅଚଳ newp = new_Sampled_ପଦସମଷ୍ଟି(nodep);
                relinkHandle.relink(newp);
                newp->user1(1);
                v3Global.setHasSampled();
            }
        }
    }
    // Don't sample sensitivities
    ଶୂନ୍ୟ visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSampled);
        m_inSampled = ମିଥ୍ୟା;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstPExprClause* nodep) override {
        Ast_ଗଣ୍ଠି* stmtsp = nullptr;
        ଯଦି (nodep->pass() && m_passsp) {
            // Cover adds COVERINC by AstNode::addNext, thus need to clone next too.
            stmtsp = m_passsp->clone_ବୃକ୍ଷ(ସତ୍ୟ);
            stmtsp = newIfAssertPassOn(stmtsp, m_assertp->directive(), m_assertp->userType(),
                                       nodep->vacuous());
        } ଅନ୍ୟଥା ଯଦି (!nodep->pass() && m_failsp) {
            stmtsp = m_failsp->clone_ବୃକ୍ଷ(ସତ୍ୟ);
            stmtsp = newIfAssertFailOn(stmtsp, m_assertp->directive(), m_assertp->userType());
        }
        ଯଦି (stmtsp) {
            stmtsp->foreachAndNext([](Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ refp) {
                // References inside action blocks shouldn't be implicitly sampled
                // m_passsp/m_failsp have been already visited once and refs explicitly sampled
                // are handled already
                refp->user2(1);
            });
            nodep->replaceWith(stmtsp);
        } ଅନ୍ୟଥା {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstPExpr* nodep) override {
        ଯଦି (m_inRestrict) {
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
        } ଅନ୍ୟଥା {
            iterateChildren(nodep);
        }
    }

    //========== Statements
    ଶୂନ୍ୟ visit(AstDisplay* nodep) override {
        iterateChildren(nodep);
        // Replace the special types with standard text
        ଯଦି (nodep->displayType() == VDisplayType::DT_INFO) {
            replaceDisplay(nodep, "-Info");
        } ଅନ୍ୟଥା ଯଦି (nodep->displayType() == VDisplayType::DT_WARNING) {
            replaceDisplay(nodep, "%%Warning");
        } ଅନ୍ୟଥା ଯଦି (nodep->displayType() == VDisplayType::DT_ERROR) {
            replaceDisplay(nodep, "%%Error");
        } ଅନ୍ୟଥା ଯଦି (nodep->displayType() == VDisplayType::DT_FATAL) {
            replaceDisplay(nodep, "%%Fatal");
        } ଅନ୍ୟଥା ଯଦି (nodep->displayType() == VDisplayType::DT_MONITOR) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();

            AstSenItem* monSenItemsp = nullptr;
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ monExprsp = nodep->fmtp()->exprsp()) {
                monExprsp->foreachAndNext([&](AstVarRef* varrefp) {
                    AstSenItem* ଅଚଳ senItemp
                        = ନୂତନ AstSenItem{fl, VEdgeType::ET_CHANGED,
                                         // Clone so get VarRef or VarXRef as needed
                                         varrefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)};
                    monSenItemsp = Ast_ଗଣ୍ଠି::addNextNull(monSenItemsp, senItemp);
                });
            }

            Ast_Sen_ବୃକ୍ଷ* ଅଚଳ mon_Sen_ବୃକ୍ଷ = ନୂତନ Ast_Sen_ବୃକ୍ଷ{fl, monSenItemsp};
            ଅଚଳ auto mon_ଅଙ୍କ = ++m_monitor_ଅଙ୍କ;
            // Where $monitor was we do "__VmonitorNum = N;"
            AstAssign* ଅଚଳ newsetp = ନୂତନ AstAssign{
                fl, new_Monitor_ଅଙ୍କ_Var_Refp(nodep, VAccess::WRITE), ନୂତନ AstConst{fl, mon_ଅଙ୍କ}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__VmonitorOn && __VmonitorNum==N) $display(...);"
            Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = nodep;
            AstIf* ଅଚଳ ifp = ନୂତନ AstIf{
                fl,
                ନୂତନ AstLogAnd{fl, ନୂତନ AstLogNot{fl, newMonitorOffVarRefp(nodep, VAccess::READ)},
                              ନୂତନ AstEq{fl, ନୂତନ AstConst{fl, mon_ଅଙ୍କ},
                                        new_Monitor_ଅଙ୍କ_Var_Refp(nodep, VAccess::READ)}},
                stmtsp};
            ifp->isBoundsCheck(ସତ୍ୟ);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstAlways{fl, VAlwaysKwd::ALWAYS, mon_Sen_ବୃକ୍ଷ, ifp};
            m_modp->addStmtsp(newp);
        } ଅନ୍ୟଥା ଯଦି (nodep->displayType() == VDisplayType::DT_STROBE) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            // Need one-shot
            File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
            AstVar* ଅଚଳ varp
                = ନୂତନ AstVar{fl, VVarType::MODULETEMP, "__Vstrobe" + cvtToStr(m_mod_Strobe_ଅଙ୍କ++),
                             nodep->findBitDType()};
            m_modp->addStmtsp(varp);
            // Where $strobe was we do "__Vstrobe = '1;"
            AstAssign* ଅଚଳ newsetp = ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, varp, VAccess::WRITE},
                                                     ନୂତନ AstConst{fl, AstConst::BitTrue{}}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__Vstrobe) begin $display(...); __Vstrobe = '0; end"
            Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = nodep;
            AstIf* ଅଚଳ ifp = ନୂତନ AstIf{fl, ନୂତନ AstVarRef{fl, varp, VAccess::READ}, stmtsp};
            ifp->isBoundsCheck(ସତ୍ୟ);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstAlwaysPostponed{fl, ifp};
            stmtsp->addNext(ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, varp, VAccess::WRITE},
                                          ନୂତନ AstConst{fl, AstConst::BitFalse{}}});
            m_modp->addStmtsp(newp);
        }
    }
    ଶୂନ୍ୟ visit(AstMonitorOff* nodep) override {
        AstAssign* ଅଚଳ newp
            = ନୂତନ AstAssign{nodep->fileline(), newMonitorOffVarRefp(nodep, VAccess::WRITE),
                            ନୂତନ AstConst{nodep->fileline(), AstConst::BitTrue{}, nodep->off()}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstAssert* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    ଶୂନ୍ୟ visit(AstAssertCtl* nodep) override {
        iterateChildren(nodep);

        ତାର୍କିକ assertTypeConst = ସତ୍ୟ;
        ଯଦି (!nodep->assertTypesp()) {
            nodep->assertTypes(VAssertType{ALL_ASSERT_TYPES});
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstConst* ଅଚଳ assertTypesp = VN_CAST(nodep->assertTypesp(), Const)) {
            nodep->assertTypes(VAssertType{assertTypesp->toSInt()});
        } ଅନ୍ୟଥା {
            assertTypeConst = ମିଥ୍ୟା;
        }

        ତାର୍କିକ controlTypeConst = ମିଥ୍ୟା;
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep->controlTypep(), Const)) {
            nodep->ctlType(constp->toSInt());
            controlTypeConst = ସତ୍ୟ;
        }
        ଯଦି (controlTypeConst
            && (nodep->ctlType() < VAssertCtlType::LOCK
                || nodep->ctlType() > VAssertCtlType::VACUOUS_OFF)) {
            nodep->unlinkFrBack();
            nodep->v3error("Bad $assertcontrol control_type '"
                           << cvtToStr(static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(nodep->ctlType()))
                           << "' (IEEE 1800-2023 Table 20-5)");
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (assertTypeConst && nodep->assertTypes() != ALL_ASSERT_TYPES
            && nodep->assertTypes().containsAny(VAssertType::UNIQUE | VAssertType::UNIQUE0
                                                | VAssertType::PRIORITY)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: assert control assertion_type");
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
        }

        ତାର୍କିକ directiveTypeConst = ସତ୍ୟ;
        ଯଦି (!nodep->directiveTypesp()) {
            nodep->directiveTypes(VAssertDirectiveType::ASSERT | VAssertDirectiveType::ASSUME
                                  | VAssertDirectiveType::COVER);
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstConst* ଅଚଳ directiveTypesp
                   = VN_CAST(nodep->directiveTypesp(), Const)) {
            nodep->directiveTypes(VAssertDirectiveType{directiveTypesp->toSInt()});
        } ଅନ୍ୟଥା {
            directiveTypeConst = ମିଥ୍ୟା;
        }
        ଯଦି (!directiveTypeConst) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: non-const assert directive type expression");
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
        }

        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        UINFO(9, "Generating assertctl for a module: " << m_modp);
        AstCStmt* ଅଚଳ newp = ନୂତନ AstCStmt{fl};
        newp->add("vlSymsp->_vm_contextp__->assertCtl(");
        newp->add(nodep->controlTypep()->unlinkFrBack());
        newp->add(", ");
        ଯଦି (nodep->assertTypesp()) {
            newp->add(nodep->assertTypesp()->unlinkFrBack());
        } ଅନ୍ୟଥା {
            newp->add(std::to_string(ALL_ASSERT_TYPES));
        }
        newp->add(", " + std::to_string(nodep->directiveTypes()) + ");\n");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstAssertIntrinsic* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    ଶୂନ୍ୟ visit(AstCover* nodep) override {  //
        visitAssertionIterate(nodep, nullptr);
    }
    ଶୂନ୍ୟ visit(AstRestrict* nodep) override {
        VL_RESTORER(m_inRestrict);
        m_inRestrict = ସତ୍ୟ;
        iterateChildren(nodep);
        // IEEE says simulator ignores these
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Past_ଅଙ୍କ);
        VL_RESTORER(m_mod_Strobe_ଅଙ୍କ);
        VL_RESTORER(m_finalp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DelayedAlwaysp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DefaultDelayedAlwaysp);
        VL_RESTORER_CLEAR(m_delayedAlways2TickTimep);
        m_modp = nodep;
        m_mod_Past_ଅଙ୍କ = 0;
        m_mod_Strobe_ଅଙ୍କ = 0;
        m_finalp = nullptr;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstGenBlock* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstBegin* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
        // If the body is a single assertOn check, bubble it up for combining
        ଯଦି (!nodep->declsp()) {
            ଯଦି (AstIf* ଅଚଳ ifp = VN_CAST(nodep->stmtsp(), If)) {
                ଯଦି (ifp->user2() && !ifp->nextp()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(ifp->unlinkFrBack());
                    nodep->addStmtsp(ifp->thensp()->unlinkFrBackWithNext());
                    ifp->addThensp(nodep);
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
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
        VIsCached::clear_Cache_ବୃକ୍ଷ();
    }
};

//######################################################################
// Top Assert class

ଶୂନ୍ୟ V3Assert::assertAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("assert", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
