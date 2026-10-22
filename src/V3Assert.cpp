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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Assert.h"

#അന്തര്ഭാവ "V3AstUserAllocator.h"
#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3Stats.h"
#അന്തര്ഭാവ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

നാമാകാശ {

വർഗ്ഗം DefaultDisableLocalVisitor final : പൊതു VNVisitor {
    // STATE
    Ast_കണ്ണി* m_scopep = nullptr;

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(nullptr);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(nullptr);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstDefaultDisable* nodep) override {
        UASSERT_OBJ(nodep, m_scopep,
                    "default disable iff must be inside a module or generate block");
        AstDefaultDisable* defaultp = nullptr;
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം modp = VN_CAST(m_scopep, കണ്ണി_Module)) {
            defaultp = modp->defaultDisablep();
        } അല്ലെങ്കിൽ {
            defaultp = VN_AS(m_scopep, GenBlock)->defaultDisablep();
        }
        എങ്കിൽ (VL_UNLIKELY(defaultp)) {
            nodep->v3error("Only one 'default disable iff' allowed per "
                           << (VN_IS(m_scopep, കണ്ണി_Module) ? "module" : "generate block")
                           << " (IEEE 1800-2023 16.15)");
        } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_Module* സ്ഥിരം modp = VN_CAST(m_scopep, കണ്ണി_Module)) {
            modp->defaultDisablep(nodep);
        } അല്ലെങ്കിൽ {
            VN_AS(m_scopep, GenBlock)->defaultDisablep(nodep);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    explicit DefaultDisableLocalVisitor(AstNetlist* nodep) { iterate(nodep); }
};

വർഗ്ഗം DefaultDisablePropagateVisitor final : പൊതു VNVisitor {
    // STATE
    AstDefaultDisable* m_defaultDisablep = nullptr;

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        എങ്കിൽ (!nodep->defaultDisablep()) nodep->defaultDisablep(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    explicit DefaultDisablePropagateVisitor(AstNetlist* nodep) { iterate(nodep); }
};

// Lower a sequence used as an event control ('@seq', IEEE 1800-2023 9.4.2.4) into a
// synthesized event fired by an internal 'cover sequence' on each end-of-match
വർഗ്ഗം SeqEventLowerVisitor final : പൊതു VNVisitor {
    // STATE
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    V3UniqueNames m_names{"__VseqEvent"};  // Synthesized event names

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstSenItem* nodep) override {
        AstFuncRef* സ്ഥിരം funcrefp = VN_CAST(nodep->sensp(), FuncRef);
        എങ്കിൽ (funcrefp && VN_IS(funcrefp->taskp(), Sequence)) {
            File_വരി* സ്ഥിരം flp = nodep->fileline();
            AstVar* സ്ഥിരം eventp = പുതിയ AstVar{flp, VVarType::MODULETEMP, m_names.get(nodep),
                                              m_modp->findBasicDType(VBasicDTypeKwd::EVENT)};
            eventp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(eventp);
            v3Global.setHasEvents();
            funcrefp->unlinkFrBack();
            nodep->sensp(പുതിയ AstVarRef{flp, eventp, VAccess::READ});
            സ്ഥിരം തർക്കം automaticActual = funcrefp->exists([](സ്ഥിരം Ast_കണ്ണി_Var_Ref* refp) {
                തിരികെ refp->varp() && refp->varp()->lifetime().isAutomatic();
            });
            എങ്കിൽ (automaticActual) {
                nodep->v3error("Arguments to a sequence used as an event control must be"
                               " static (IEEE 1800-2023 9.4.2.4)");
                VN_AS(funcrefp->taskp(), Sequence)->isReferenced(അസത്യം);
                VL_DO_DANGLING(തള്ളുക_Deletep(funcrefp), funcrefp);
                തിരികെ;
            }
            AstFireEvent* സ്ഥിരം firep
                = പുതിയ AstFireEvent{flp, പുതിയ AstVarRef{flp, eventp, VAccess::WRITE}, അസത്യം};
            AstCover* സ്ഥിരം coverp
                = പുതിയ AstCover{flp, പുതിയ AstPropSpec{flp, nullptr, nullptr, funcrefp}, firep,
                               VAssertType::CONCURRENT};
            coverp->isCoverSeq(സത്യം);
            coverp->isSeqEvent(സത്യം);
            m_modp->addStmtsp(coverp);
            തിരികെ;
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    explicit SeqEventLowerVisitor(AstNetlist* nodep) { iterate(nodep); }
};

}  // namespace

ശൂന്യം V3AssertCommon::collectDefaultDisable(AstNetlist* nodep) {
    { DefaultDisableLocalVisitor{nodep}; }
    { DefaultDisablePropagateVisitor{nodep}; }
}

ശൂന്യം V3AssertCommon::lowerSequenceEvents(AstNetlist* nodep) {
    { SeqEventLowerVisitor{nodep}; }
    V3Global::dump_Check_Global_വൃക്ഷം("assertseqevent", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}

string V3AssertCommon::assertCtlGetCall(സ്ഥിരം അക്ഷരം* query, VAssertType type,
                                        VAssertDirectiveType directiveType) {
    // Template arguments let the runtime build the type/directive mask at C++ compile time
    തിരികെ "vlSymsp->_vm_contextp__->assertCtlGet<"s + std::to_string(type) + ", "s
           + std::to_string(directiveType) + ">(VerilatedAssertCtlQuery::"s + query + ")"s;
}

തർക്കം V3AssertCommon::check_Sva_മൂല്യം_Limit(Ast_കണ്ണി* nodep, സ്ഥിരം AstConst* constp, സ്ഥിരം അക്ഷരം* what) {
    // Temporal values can materialize as O(N) AST or state in later passes.
    // Leave headroom for a range ring's extra slot and V3Number's signed
    // `(bits + 31) / 32` word count.
    സ്ഥിതം constexpr unsigned SVA_മൂല്യം_HARD_LIMIT
        = static_cast<unsigned>(std::numeric_limits<പൂർണ്ണസംഖ്യ>::max()) - 32U;
    സ്ഥിരം unsigned configuredLimit = static_cast<unsigned>(v3Global.opt.max_അക്കം_Width());
    സ്ഥിരം unsigned limit = std::min(configuredLimit, SVA_മൂല്യം_HARD_LIMIT);
    എങ്കിൽ (constp->അക്കം().fitsInUInt() && constp->toUInt() <= limit) തിരികെ സത്യം;
    nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << what << " exceeds implementation limit of "
                                                 << limit
                                                 << (configuredLimit <= SVA_മൂല്യം_HARD_LIMIT
                                                         ? " (--max-num-width)"
                                                         : " (host arithmetic limit)"));
    തിരികെ അസത്യം;
}

// The bound of a cycle delay in a sequence, if a non-negative elaboration-time constant
സ്ഥിതം സ്ഥിരം AstConst* checkSvaDelayBound(Ast_കണ്ണി* errp, Ast_കണ്ണി_വ്യഞ്ജകം* boundp, സ്ഥിരം അക്ഷരം* what) {
    സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(boundp, Const);
    എങ്കിൽ (!constp || (constp->dtypep()->isSigned() && constp->അക്കം().isNegative())) {
        errp->v3error(what << " is not a non-negative elaboration-time constant"
                              " (IEEE 1800-2023 16.7)");
        തിരികെ nullptr;
    }
    എങ്കിൽ (!V3AssertCommon::check_Sva_മൂല്യം_Limit(errp, constp, "SVA cycle delay")) തിരികെ nullptr;
    തിരികെ constp;
}

// Fold the bounds of a cycle delay in a sequence, and return if they are valid, else report it
// on errp
സ്ഥിതം തർക്കം svaCycleDelayValid(സ്ഥിരം AstDelay* nodep, Ast_കണ്ണി* errp) {
    V3Const::constifyParamsNoWarnEdit(nodep->lhsp());
    സ്ഥിരം AstConst* സ്ഥിരം minConstp = checkSvaDelayBound(
        errp, nodep->lhsp(), nodep->isRangeDelay() ? "Range delay minimum" : "Delay value");
    എങ്കിൽ (!nodep->rhsp() || nodep->isUnbounded()) തിരികെ minConstp != nullptr;
    V3Const::constifyParamsNoWarnEdit(nodep->rhsp());
    സ്ഥിരം AstConst* സ്ഥിരം maxConstp
        = checkSvaDelayBound(errp, nodep->rhsp(), "Range delay maximum");
    എങ്കിൽ (!minConstp || !maxConstp) തിരികെ അസത്യം;
    എങ്കിൽ (maxConstp->toUInt() < minConstp->toUInt()) {
        errp->v3error("Range delay maximum must be >= minimum (IEEE 1800-2023 16.7)");
        തിരികെ അസത്യം;
    }
    തിരികെ സത്യം;
}

ശൂന്യം V3AssertCommon::checkSvaCycleDelay(AstDelay* nodep, Ast_കണ്ണി* errp) {
    എങ്കിൽ (svaCycleDelayValid(nodep, errp)) തിരികെ;
    // Make the delay valid, so later steps do not fail after the error
    File_വരി* സ്ഥിരം flp = nodep->fileline();
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം minp = nodep->lhsp();
    minp->replaceWith(പുതിയ AstConst{flp, AstConst::BitFalseErroring{}});
    VL_DO_DANGLING(minp->delete_വൃക്ഷം(), minp);
    എങ്കിൽ (nodep->rhsp() && !nodep->isUnbounded()) {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം maxp = nodep->rhsp();
        maxp->replaceWith(പുതിയ AstConst{flp, AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(maxp->delete_വൃക്ഷം(), maxp);
    }
}

തർക്കം V3AssertCommon::refsSvaFormal(സ്ഥിരം Ast_കണ്ണി* nodep) {
    തിരികെ nodep->exists([](സ്ഥിരം Ast_കണ്ണി_Var_Ref* refp) {
        സ്ഥിരം AstVar* സ്ഥിരം varp = refp->varp();
        എങ്കിൽ (!varp || !varp->isIO()) തിരികെ അസത്യം;
        സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം declp = varp->aboveLoopp();
        തിരികെ VN_IS(declp, Sequence) || VN_IS(declp, Property);
    });
}

ശൂന്യം V3AssertCommon::checkSvaInstanceDelays(Ast_കണ്ണി* nodep, Ast_കണ്ണി* instp) {
    // A delay still referencing a formal argument of an enclosing sequence or property is
    // checked when that is instanced. Collect first, as checking edits the delays.
    std::vector<AstDelay*> delayps;
    nodep->foreach([&](AstDelay* delayp) {
        എങ്കിൽ (!delayp->isCycleDelay() || refsSvaFormal(delayp->lhsp())) തിരികെ;
        എങ്കിൽ (delayp->rhsp() && refsSvaFormal(delayp->rhsp())) തിരികെ;
        delayps.തള്ളുക_back(delayp);
    });
    വേണ്ടി (AstDelay* സ്ഥിരം delayp : delayps) checkSvaCycleDelay(delayp, instp);
}

//######################################################################
// AssertDeFutureVisitor
// If any AstFuture, then move all non-future varrefs to be one cycle behind,
// see IEEE 1800-2023 16.9.4.

വർഗ്ഗം AssertDeFutureVisitor final : പൊതു VNVisitor {
    // STATE - across all visitors
    Ast_കണ്ണി_Module* സ്ഥിരം m_modp;  // Module future is underneath
    സ്ഥിരം AstFuture* m_futurep;  // First AstFuture found
    സ്ഥിരം unsigned m_past_അക്കം;  // Prefix unique number for this module
    std::map<AstVar*, AstVar*> m_delayedVars;  // Old to delayed variable mapping
    // STATE - for current visit position (use VL_RESTORER)
    തർക്കം m_inFuture = അസത്യം;  // Inside a future
    തർക്കം m_unsupported = അസത്യം;  // Printed unsupported

    // METHODS
    ശൂന്യം unsupported(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (m_unsupported) തിരികെ;
        m_unsupported = സത്യം;
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported/illegal: Future value function used with expression with "
                          << nodep->prettyOperatorName());
    }
    // VISITORS
    ശൂന്യം visit(AstFuture* nodep) override {
        VL_RESTORER(m_inFuture);
        m_inFuture = സത്യം;
        iterateChildren(nodep);
        // Done with the future, this subexpression is current-time
        nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;
        എങ്കിൽ (m_inFuture || m_unsupported)
            തിരികെ;  // Need user1 set above, don't process when Future is removed
        എങ്കിൽ (nodep->access().isWriteOrRW()) {
            unsupported(nodep);
            തിരികെ;
        }
        auto it = m_delayedVars.find(nodep->varp());
        AstVar* outvarp;
        എങ്കിൽ (it == m_delayedVars.end()) {
            Ast_Sen_വൃക്ഷം* സ്ഥിരം sentreep = m_futurep->sentreep();
            AstAlways* സ്ഥിരം alwaysp = പുതിയ AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS,
                                                     sentreep->clone_വൃക്ഷം(അസത്യം), nullptr};
            m_modp->addStmtsp(alwaysp);
            outvarp = പുതിയ AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                 "__Vnotfuture" + cvtToStr(m_past_അക്കം) + "_" + nodep->name(),
                                 nodep->dtypep()};
            m_modp->addStmtsp(outvarp);
            AstVarRef* varRefAWritep = പുതിയ AstVarRef{nodep->fileline(), outvarp, VAccess::WRITE};
            varRefAWritep->user1(സത്യം);
            Ast_കണ്ണി_Var_Ref* varRefAReadp = nodep->clone_വൃക്ഷം(അസത്യം);
            varRefAReadp->user1(സത്യം);
            Ast_കണ്ണി* സ്ഥിരം assp = പുതിയ AstAssignDly{nodep->fileline(), varRefAWritep, varRefAReadp};
            alwaysp->addStmtsp(assp);
            m_delayedVars.emplace(nodep->varp(), outvarp);
        } അല്ലെങ്കിൽ {
            outvarp = it->second;
        }
        AstVarRef* newp = പുതിയ AstVarRef{nodep->fileline(), outvarp, VAccess::READ};
        newp->user1(സത്യം);
        UINFO(9, "DeFuture " << nodep << "  becomes " << newp);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override { unsupported(nodep); }
    ശൂന്യം visit(AstMethodCall* nodep) override { unsupported(nodep); }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        എങ്കിൽ (!nodep->isPure()) unsupported(nodep);
        iterateChildren(nodep);
    }

പൊതു:
    // CONSTRUCTORS
    explicit AssertDeFutureVisitor(Ast_കണ്ണി* nodep, Ast_കണ്ണി_Module* modp, unsigned past_അക്കം)
        : m_modp{modp}
        , m_past_അക്കം{past_അക്കം} {
        // See if any Future before we process
        എങ്കിൽ (nodep->forall([&](സ്ഥിരം AstFuture* futurep) -> തർക്കം {
                m_futurep = futurep;
                തിരികെ അസത്യം;
            }))
            തിരികെ;
        // UINFOTREE(9, nodep, "", "defuture-in");
        visit(nodep);  // Nodep may get deleted
        // UINFOTREE(9, nodep, "", "defuture-ou");
    }
    ~AssertDeFutureVisitor() = സ്ഥിരസ്ഥിതി;
};

// Prepare one default sampled expression.
വർഗ്ഗം Default_Sampled_മൂല്യം_Visitor final : പൊതു VNVisitor {
    // METHODS
    സ്ഥിതം AstConst* newTypeDefault(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        AstConst* സ്ഥിരം constp
            = പുതിയ AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()};
        എങ്കിൽ (nodep->dtypep()->isFourstate()) constp->അക്കം().setAllBitsX();
        constp->dtypeFrom(nodep);
        തിരികെ constp;
    }

    // VISITORS
    ശൂന്യം visit(AstCMethodHard* nodep) override {
        എങ്കിൽ (nodep->method() == VCMethod::EVENT_IS_TRIGGERED) {
            nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ {
            iterateChildren(nodep);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        AstVar* സ്ഥിരം varp = nodep->varp();
        എങ്കിൽ (varp->isParam()
            || (!varp->isNet() && varp->lifetime().isStatic() && varp->hasUserInit())) {
            തിരികെ;
        }
        nodep->replaceWith(newTypeDefault(nodep));
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    explicit Default_Sampled_മൂല്യം_Visitor(Ast_കണ്ണി* nodep) { iterate(nodep); }
};

//######################################################################
// AssertVisitor

വർഗ്ഗം AssertVisitor final : പൊതു VNVisitor {
    // CONSTANTS
    സ്ഥിതം constexpr uint8_t ALL_ASSERT_TYPES
        = std::numeric_limits<std::underlying_type<VAssertType::en>::type>::max();

    // NODE STATE/TYPES
    // Cleared on netlist
    //  AstNode::user1()         -> bool.  True if processed
    //  AstAlways::user2p()      -> std::vector<AstVar*>. Delayed variables via 'm_delayed'
    //  AstNodeVarRef::user2()   -> bool.  True if shouldn't be sampled
    //  AstIf::user2()           -> bool.  True for assertOn() checks inserted in this pass
    സ്ഥിരം VNUser1InUse m_user1InUse;
    സ്ഥിരം VNUser2InUse m_user2InUse;
    AstUser2Allocator<AstAlways, std::vector<AstVar*>> m_delayed;

    // STATE
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Last module
    സ്ഥിരം Ast_കണ്ണി* m_beginp = nullptr;  // Last AstBegin/AstGenBlock
    unsigned m_monitor_അക്കം = 0;  // Global $monitor numbering (not per module)
    AstVar* m_monitor_അക്കം_Varp = nullptr;  // $monitor number variable
    AstVar* m_monitorOffVarp = nullptr;  // $monitoroff variable
    unsigned m_mod_Past_അക്കം = 0;  // Module past numbering
    unsigned m_mod_Strobe_അക്കം = 0;  // Module $strobe numbering
    Ast_കണ്ണി_Procedure* m_procedurep = nullptr;  // Current procedure
    VDouble0 m_statCover;  // Statistic tracking
    VDouble0 m_statAsNotImm;  // Statistic tracking
    VDouble0 m_statAsImm;  // Statistic tracking
    VDouble0 m_statAsFull;  // Statistic tracking
    VDouble0 m_statPastVars;  // Statistic tracking
    VDouble0 m_statAssertOnCombined;  // Statistic tracking
    VDouble0 m_statAssertOnHoisted;  // Statistic tracking
    തർക്കം m_inSampled = അസത്യം;  // True inside a sampled expression
    തർക്കം m_inRestrict = അസത്യം;  // True inside restrict assertion
    Ast_കണ്ണി* m_passsp = nullptr;  // Current pass statement
    Ast_കണ്ണി* m_failsp = nullptr;  // Current fail statement
    Ast_കണ്ണി_Cover_Or_Assert* m_assertp = nullptr;  // Current assertion
    AstFinal* m_finalp = nullptr;  // Current final block
    VDouble0 m_statLiftedCaseExprs;  // Count of purified case expressions
    Ast_കണ്ണി_FTask* m_ftaskp = nullptr;  // Current function/task
    V3UniqueNames m_caseTempNames{"__VCase"};
    V3UniqueNames m_match_എണ്ണം_Names{"__VnfaRemainingMatchCount"};  // Match replay counter names
    // Maps from (expression, senTree) to the AstAlways that computes its delayed values.
    std::unordered_map<VNRef<Ast_കണ്ണി_വ്യഞ്ജകം>, std::unordered_map<VNRef<Ast_Sen_വൃക്ഷം>, AstAlways*>>
        m_modExpr2Sen2DelayedAlwaysp;
    // Source sampled-value history is initialized, unlike internal property-lowering history.
    std::unordered_map<VNRef<Ast_കണ്ണി_വ്യഞ്ജകം>, std::unordered_map<VNRef<Ast_Sen_വൃക്ഷം>, AstAlways*>>
        m_modExpr2Sen2DefaultDelayedAlwaysp;
    // Map from delayed-value AstAlways to its last-sampling-tick time variable
    std::unordered_map<സ്ഥിരം AstAlways*, AstVar*> m_delayedAlways2TickTimep;

    // METHODS
    സ്ഥിതം സ്ഥിരം അക്ഷരം* assertPassOnQuery(തർക്കം vacuous) {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* queries[2]
            = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
        തിരികെ queries[vacuous];
    }
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* assertOnCond(File_വരി* fl, VAssertType type,
                                     VAssertDirectiveType directiveType) {
        // cppcheck-suppress missingReturn
        തിരഞ്ഞെടുപ്പ് (directiveType) {
        അവസ്ഥ VAssertDirectiveType::INTRINSIC: തിരികെ പുതിയ AstConst{fl, AstConst::BitTrue{}};
        അവസ്ഥ VAssertDirectiveType::VIOLATION_CASE: {
            എങ്കിൽ (v3Global.opt.assertCase()) {
                തിരികെ പുതിയ AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            // If assertions are off, have constant propagation rip them out later
            // This allows syntax errors and such to be detected normally.
            തിരികെ പുതിയ AstConst{fl, AstConst::BitFalse{}};
        }
        അവസ്ഥ VAssertDirectiveType::ASSERT:
        അവസ്ഥ VAssertDirectiveType::COVER:
        അവസ്ഥ VAssertDirectiveType::ASSUME: {
            എങ്കിൽ (v3Global.opt.assertOn()) {
                തിരികെ പുതിയ AstCExpr{
                    fl, AstCExpr::Pure{},
                    V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
            }
            തിരികെ പുതിയ AstConst{fl, AstConst::BitFalse{}};
        }
        അവസ്ഥ VAssertDirectiveType::INTERNAL:
        അവസ്ഥ VAssertDirectiveType::VIOLATION_IF:
        അവസ്ഥ VAssertDirectiveType::RESTRICT: {
            എങ്കിൽ (v3Global.opt.assertOn()) {
                തിരികെ പുതിയ AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            തിരികെ പുതിയ AstConst{fl, AstConst::BitFalse{}};
        }
        }
        VL_UNREACHABLE;
    }

    സ്ഥിതം തർക്കം isControlled(VAssertDirectiveType directiveType) {
        തിരികെ (static_cast<പൂർണ്ണസംഖ്യ>(directiveType)
                & (static_cast<പൂർണ്ണസംഖ്യ>(VAssertDirectiveType::ASSERT)
                   | static_cast<പൂർണ്ണസംഖ്യ>(VAssertDirectiveType::COVER)
                   | static_cast<പൂർണ്ണസംഖ്യ>(VAssertDirectiveType::ASSUME)));
    }
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* assertPassOnCond(File_വരി* fl, VAssertType type,
                                         VAssertDirectiveType directiveType, തർക്കം vacuous) {
        എങ്കിൽ (!isControlled(directiveType)) തിരികെ പുതിയ AstConst{fl, AstConst::BitTrue{}};
        എങ്കിൽ (!v3Global.opt.assertOn()) തിരികെ പുതിയ AstConst{fl, AstConst::BitFalse{}};
        തിരികെ പുതിയ AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType), 1};
    }
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* assertFailOnCond(File_വരി* fl, VAssertType type,
                                         VAssertDirectiveType directiveType) {
        എങ്കിൽ (!isControlled(directiveType)) തിരികെ പുതിയ AstConst{fl, AstConst::BitTrue{}};
        എങ്കിൽ (!v3Global.opt.assertOn()) തിരികെ പുതിയ AstConst{fl, AstConst::BitFalse{}};
        തിരികെ പുതിയ AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType), 1};
    }
    string assertDisplayMessage(സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം string& prefix, സ്ഥിരം string& message,
                                VDisplayType severity) {
        എങ്കിൽ (severity == VDisplayType::DT_ERROR || severity == VDisplayType::DT_FATAL) {
            തിരികെ ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                    + cvtToStr(nodep->fileline()->lineno()) + ": Assertion failed in %m"
                    + ((message != "") ? ": " : "") + message + "\n");
        }
        തിരികെ ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                + cvtToStr(nodep->fileline()->lineno()) + ": %m" + ((message != "") ? ": " : "")
                + message + "\n");
    }
    ശൂന്യം replaceDisplay(AstDisplay* nodep, സ്ഥിരം string& prefix) {
        nodep->fmtp()->text(
            assertDisplayMessage(nodep, prefix, nodep->fmtp()->text(), nodep->displayType()));
        nodep->displayType(VDisplayType::DT_WRITE);
        // cppcheck-suppress nullPointer
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം timenewp = പുതിയ AstTime{nodep->fileline(), m_modp->timeunit()};
        nodep->fmtp()->prependExprsp(timenewp);
        എങ്കിൽ (!nodep->fmtp()->scopeNamep() && nodep->fmtp()->formatScopeTracking()) {
            nodep->fmtp()->scopeNamep(പുതിയ AstScopeName{nodep->fileline(), സത്യം});
        }
    }
    AstSampled* new_Sampled_വ്യഞ്ജകം(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        തിരികെ പുതിയ AstSampled{nodep->fileline(), nodep, nodep->dtypep(), സത്യം};
    }
    AstVarRef* new_Monitor_അക്കം_Var_Refp(സ്ഥിരം Ast_കണ്ണി* nodep, VAccess access) {
        എങ്കിൽ (!m_monitor_അക്കം_Varp) {
            m_monitor_അക്കം_Varp = പുതിയ AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorNum",
                                          nodep->findUInt64DType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitor_അക്കം_Varp);
        }
        AstVarRef* സ്ഥിരം varrefp = പുതിയ AstVarRef{nodep->fileline(), m_monitor_അക്കം_Varp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        തിരികെ varrefp;
    }
    AstVarRef* newMonitorOffVarRefp(സ്ഥിരം Ast_കണ്ണി* nodep, VAccess access) {
        എങ്കിൽ (!m_monitorOffVarp) {
            m_monitorOffVarp = പുതിയ AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorOff",
                                          nodep->findBitDType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitorOffVarp);
        }
        AstVarRef* സ്ഥിരം varrefp = പുതിയ AstVarRef{nodep->fileline(), m_monitorOffVarp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        തിരികെ varrefp;
    }
    സ്ഥിതം AstIf* newIfAssertOn(Ast_കണ്ണി* bodyp, VAssertDirectiveType directiveType,
                                VAssertType type = VAssertType::INTERNAL) {
        // Add an internal if to check assertions are on.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_വരി* സ്ഥിരം fl = bodyp->fileline();

        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = assertOnCond(fl, type, directiveType);
        AstIf* സ്ഥിരം newp = പുതിയ AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(സത്യം);  // To avoid LATCH warning
        newp->user2(സത്യം);  // Mark as an assertOn() check
        തിരികെ newp;
    }
    സ്ഥിതം Ast_കണ്ണി_Stmt* newIfAssertPassOn(Ast_കണ്ണി* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type, തർക്കം vacuous) {
        // Add an internal if to check assertion passOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_വരി* സ്ഥിരം fl = bodyp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = assertPassOnCond(fl, type, directiveType, vacuous);
        Ast_കണ്ണി_If* സ്ഥിരം newp = പുതിയ AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(സത്യം);  // To avoid LATCH warning
        newp->user1(സത്യം);  // Don't assert/cover this if
        newp->user2(സത്യം);  // Mark as an assertOn() check
        തിരികെ newp;
    }
    സ്ഥിതം Ast_കണ്ണി_Stmt* newIfAssertFailOn(Ast_കണ്ണി* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type) {
        // Add an internal if to check assertion failOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_വരി* സ്ഥിരം fl = bodyp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = assertFailOnCond(fl, type, directiveType);
        Ast_കണ്ണി_If* സ്ഥിരം newp = പുതിയ AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(സത്യം);  // To avoid LATCH warning
        newp->user1(സത്യം);  // Don't assert/cover this if
        newp->user2(സത്യം);  // Mark as an assertOn() check
        തിരികെ newp;
    }

    സ്ഥിതം AstIf* assertCond(സ്ഥിരം Ast_കണ്ണി_Cover_Or_Assert* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* propp,
                             Ast_കണ്ണി* passsp, Ast_കണ്ണി* failsp) {
        AstIf* സ്ഥിരം ifp = പുതിയ AstIf{nodep->fileline(), propp, passsp, failsp};
        // It's more LIKELY that we'll take the nullptr if clause
        // than the sim-killing else clause:
        ifp->branchPred(VBranchPred::BP_LIKELY);
        ifp->isBoundsCheck(സത്യം);  // To avoid LATCH warning
        തിരികെ ifp;
    }

    Ast_കണ്ണി_Stmt* assertBody(സ്ഥിരം Ast_കണ്ണി_Cover_Or_Assert* nodep, Ast_കണ്ണി* propp, Ast_കണ്ണി* passsp,
                            Ast_കണ്ണി* failsp) {
        എങ്കിൽ (AstPExpr* സ്ഥിരം pexprp = VN_CAST(propp, PExpr)) {
            എങ്കിൽ (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NOTIMING, "This property expression requires --timing");
                VL_DO_DANGLING(തള്ളുക_Deletep(pexprp), pexprp);
                തിരികെ പുതിയ AstBegin{nodep->fileline(), "", nullptr, അസത്യം};
            }
            AstFork* സ്ഥിരം forkp = പുതിയ AstFork{nodep->fileline(), VJoinType::JOIN_NONE};
            forkp->addForksp(pexprp->bodyp()->unlinkFrBack());
            എങ്കിൽ (Ast_കണ്ണി_Stmt* സ്ഥിരം finalp = pexprp->finalp()) {
                എങ്കിൽ (!m_finalp) {
                    m_finalp = പുതിയ AstFinal{m_modp->fileline(), finalp->unlinkFrBack()};
                    m_modp->addStmtsp(m_finalp);
                } അല്ലെങ്കിൽ {
                    m_finalp->addStmtsp(finalp->unlinkFrBack());
                }
            }
            VL_DO_DANGLING2(തള്ളുക_Deletep(pexprp), pexprp, propp);
            തിരികെ forkp;
        }

        തിരികെ assertCond(nodep, VN_AS(propp, കണ്ണി_വ്യഞ്ജകം), passsp, failsp);
    }

    Ast_കണ്ണി_Stmt* newFireAssertUnchecked(സ്ഥിരം Ast_കണ്ണി_Stmt* nodep, സ്ഥിരം string& message,
                                        Ast_കണ്ണി_വ്യഞ്ജകം* exprsp = nullptr) {
        // Like newFireAssert() but omits the asserts-on check
        AstDisplay* സ്ഥിരം dispp
            = പുതിയ AstDisplay{nodep->fileline(), VDisplayType::DT_ERROR, message, nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        Ast_കണ്ണി_Stmt* സ്ഥിരം bodysp = dispp;
        replaceDisplay(dispp, "%%Error");  // Convert to standard DISPLAY format
        എങ്കിൽ (exprsp) dispp->fmtp()->exprsp()->addNext(exprsp);
        എങ്കിൽ (v3Global.opt.stopFail()) bodysp->addNext(പുതിയ AstStop{nodep->fileline(), അസത്യം});
        തിരികെ bodysp;
    }

    Ast_കണ്ണി_Stmt* newFireAssert(സ്ഥിരം Ast_കണ്ണി_Stmt* nodep, VAssertDirectiveType directiveType,
                               VAssertType assertType, സ്ഥിരം string& message,
                               Ast_കണ്ണി_വ്യഞ്ജകം* exprsp = nullptr) {
        Ast_കണ്ണി_Stmt* bodysp = newFireAssertUnchecked(nodep, message, exprsp);
        bodysp = newIfAssertOn(bodysp, directiveType, assertType);
        തിരികെ bodysp;
    }

    AstVar* createDelayedVar(സ്ഥിരം std::string& name, AstAlways* alwaysp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp,
                             Ast_കണ്ണി_വ്യഞ്ജകം* initp) {
        File_വരി* സ്ഥിരം flp = exprp->fileline();
        AstVar* സ്ഥിരം varp = പുതിയ AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        // TODO: this lifetime seems nonsene (can't have NBAs to automatics), but is as before
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->addStmtsp(varp);
        // Before enough clock ticks, $past returns the expression's default sampled value.
        എങ്കിൽ (initp) {
            m_modp->addStmtsp(പുതിയ AstInitialStatic{
                flp, പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, varp, VAccess::WRITE}, initp}});
        }
        ++m_statPastVars;
        // Actually set the delayed value
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = പുതിയ AstVarRef{flp, varp, VAccess::WRITE};
        AstAssignDly* സ്ഥിരം assignp = പുതിയ AstAssignDly{flp, lhsp, exprp};
        alwaysp->prependStmtsp(assignp);
        തിരികെ varp;
    }

    AstAlways* getDelayedAlways(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_കണ്ണി_വ്യഞ്ജകം* initp, Ast_Sen_വൃക്ഷം* senTreep) {
        auto& expr2Sen2Alwaysr
            = initp ? m_modExpr2Sen2DefaultDelayedAlwaysp : m_modExpr2Sen2DelayedAlwaysp;
        AstAlways*& alwayspr = expr2Sen2Alwaysr[*exprp][*senTreep];
        എങ്കിൽ (!alwayspr) {
            File_വരി* സ്ഥിരം flp = exprp->fileline();
            // Create the always block that computes the delayed values
            alwayspr = പുതിയ AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, nullptr};
            m_modp->addStmtsp(alwayspr);
            // Create the once-delayed variable
            സ്ഥിരം std::string name = "_Vpast_" + cvtToStr(m_mod_Past_അക്കം++) + "_1";
            AstVar* സ്ഥിരം varp = createDelayedVar(name, alwayspr, exprp, initp);
            // Add it to delayed variable vector
            m_delayed(alwayspr).emplace_back(varp);
        } അല്ലെങ്കിൽ {
            // Reusing exiting, not needed
            VL_DO_DANGLING(തള്ളുക_Deletep(exprp), exprp);
            എങ്കിൽ (initp) VL_DO_DANGLING(തള്ളുക_Deletep(initp), initp);
            VL_DO_DANGLING(തള്ളുക_Deletep(senTreep), senTreep);
        }
        തിരികെ alwayspr;
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* get_Past_മൂല്യം(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_കണ്ണി_വ്യഞ്ജകം* initp, Ast_Sen_വൃക്ഷം* senTreep,
                              uint32_t ticks) {
        UASSERT_OBJ(ticks > 0, exprp, "Delay must be > 0");
        File_വരി* സ്ഥിരം flp = exprp->fileline();
        AstAlways* സ്ഥിരം alwaysp
            = getDelayedAlways(exprp, initp ? initp->clone_വൃക്ഷം_Pure(അസത്യം) : nullptr, senTreep);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം resultp = past_മൂല്യം_Ref(alwaysp, flp, ticks, initp);
        എങ്കിൽ (initp) VL_DO_DANGLING(തള്ളുക_Deletep(initp), initp);
        തിരികെ resultp;
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* past_മൂല്യം_Ref(AstAlways* alwaysp, File_വരി* flp, uint32_t ticks,
                              Ast_കണ്ണി_വ്യഞ്ജകം* initp) {
        std::vector<AstVar*>& delayedr = m_delayed(alwaysp);
        // Ensure the required delay exists
        ആകുമ്പോൾ (delayedr.size() < ticks) {
            AstVar* സ്ഥിരം firstp = delayedr.front();
            File_വരി* സ്ഥിരം varFlp = firstp->fileline();
            // Create once more delayed value
            std::string name = firstp->name();
            name.resize(name.size() - 1);
            name += std::to_string(delayedr.size() + 1);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം prevp = പുതിയ AstVarRef{varFlp, delayedr.back(), VAccess::READ};
            AstVar* സ്ഥിരം varp = createDelayedVar(name, alwaysp, prevp,
                                                  initp ? initp->clone_വൃക്ഷം_Pure(അസത്യം) : nullptr);
            // Add it to delayed variable vector
            delayedr.emplace_back(varp);
        }
        // Return a reference to the appropriately delayed variable
        തിരികെ പുതിയ AstVarRef{flp, delayedr.at(ticks - 1), VAccess::READ};
    }

    // Time of the pipeline's last sampling tick, for end-of-simulation readers
    AstVar* getPastTickTimeVar(AstAlways* alwaysp) {
        AstVar*& varpr = m_delayedAlways2TickTimep[alwaysp];
        എങ്കിൽ (!varpr) {
            File_വരി* സ്ഥിരം flp = alwaysp->fileline();
            varpr = പുതിയ AstVar{flp, VVarType::MODULETEMP,
                               "_Vpast_" + cvtToStr(m_mod_Past_അക്കം++) + "_t",
                               m_modp->findUInt64DType()};
            m_modp->addStmtsp(varpr);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം timep = പുതിയ AstCExpr{flp, "vlSymsp->_vm_contextp__->time()", 64};
            alwaysp->addStmtsp(
                പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, varpr, VAccess::WRITE}, timep});
        }
        തിരികെ varpr;
    }

    ശൂന്യം visitAssertionIterate(Ast_കണ്ണി_Cover_Or_Assert* nodep, Ast_കണ്ണി* failsp) {
        എങ്കിൽ (m_beginp && nodep->name() == "") nodep->name(m_beginp->name());

        { AssertDeFutureVisitor{nodep->propp(), m_modp, m_mod_Past_അക്കം++}; }

        iterateAndNextNull(nodep->sentreep());
        എങ്കിൽ (AstAssert* സ്ഥിരം assertp = VN_CAST(nodep, Assert)) {
            iterateAndNextNull(assertp->failsp());
        } അല്ലെങ്കിൽ എങ്കിൽ (AstAssertIntrinsic* സ്ഥിരം assertp = VN_CAST(nodep, AssertIntrinsic)) {
            iterateAndNextNull(assertp->failsp());
        } അല്ലെങ്കിൽ എങ്കിൽ (AstCover* സ്ഥിരം coverp = VN_CAST(nodep, Cover)) {
            iterateAndNextNull(coverp->coverincsp());
        } അല്ലെങ്കിൽ എങ്കിൽ (!VN_IS(nodep, Restrict)) {
            nodep->v3fatalSrc("Unhandled assert type");
        }
        iterateAndNextNull(nodep->passsp());
        Ast_Sen_വൃക്ഷം* sentreep = nodep->sentreep();
        എങ്കിൽ (nodep->immediate()) {
            UASSERT_OBJ(!sentreep, nodep, "Immediate assertions don't have sensitivity");
        } അല്ലെങ്കിൽ {
            UASSERT_OBJ(sentreep, nodep, "Concurrent assertions must have sensitivity");
            // Explicit inline clock differs from the enclosing always: hoist
            // and warn. To support this need queue of asserts to activate.
            എങ്കിൽ (m_procedurep && !nodep->senFromAlways()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Procedural concurrent assertion with"
                              " clocking event inside always (IEEE 1800-2023 16.14.6)");
                static_cast<Ast_കണ്ണി*>(m_procedurep)->addNext(nodep->unlinkFrBack());
                തിരികെ;  // Later iterate will pick up
            }
            sentreep->unlinkFrBack();
            എങ്കിൽ (m_procedurep) { VL_DO_DANGLING(തള്ളുക_Deletep(sentreep), sentreep); }
        }
        //
        സ്ഥിരം string& message = nodep->name();
        Ast_കണ്ണി* passsp = nodep->passsp();
        എങ്കിൽ (passsp) passsp->unlinkFrBackWithNext();
        എങ്കിൽ (AstAssert* സ്ഥിരം assertp = VN_CAST(nodep, Assert)) {
            failsp = assertp->failsp();
        } അല്ലെങ്കിൽ എങ്കിൽ (AstAssertIntrinsic* സ്ഥിരം assertp = VN_CAST(nodep, AssertIntrinsic)) {
            failsp = assertp->failsp();
        }
        എങ്കിൽ (failsp) failsp->unlinkFrBackWithNext();

        തർക്കം selfDestruct = അസത്യം;
        തർക്കം passspGated = അസത്യം;
        സ്ഥിരം AstCover* സ്ഥിരം coverp = VN_CAST(nodep, Cover);
        // A sequence event control is not an assertion directive; no assertion control
        സ്ഥിരം തർക്കം seqEvent = coverp && coverp->isSeqEvent();
        എങ്കിൽ (coverp) {
            ++m_statCover;
            എങ്കിൽ (seqEvent) {
                // Keep the event-fire action, with no coverage bucket
            } അല്ലെങ്കിൽ എങ്കിൽ (!v3Global.opt.coverageUser()) {
                selfDestruct = സത്യം;
            } അല്ലെങ്കിൽ {
                // V3Coverage assigned us a bucket to increment.
                AstCoverInc* സ്ഥിരം covincp = VN_AS(coverp->coverincsp(), CoverInc);
                UASSERT_OBJ(covincp, coverp, "Missing AstCoverInc under assertion");
                covincp->unlinkFrBackWithNext();  // next() might have  AstAssign for trace
                എങ്കിൽ (message != "") covincp->declp()->comment(message);
                എങ്കിൽ (passsp) {
                    passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                               /*vacuous=*/അസത്യം);
                    passspGated = സത്യം;
                    passsp = Ast_കണ്ണി::addNext<Ast_കണ്ണി, Ast_കണ്ണി>(covincp, passsp);
                } അല്ലെങ്കിൽ {
                    passsp = covincp;
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, Assert) || VN_IS(nodep, AssertIntrinsic)) {
            എങ്കിൽ (nodep->immediate()) {
                ++m_statAsImm;
            } അല്ലെങ്കിൽ {
                ++m_statAsNotImm;
            }
            എങ്കിൽ (!passsp && !failsp)
                failsp = newFireAssertUnchecked(
                    nodep, VN_IS(nodep, AssertIntrinsic) ? "'$cast' failed." : "'assert' failed.");
        } അല്ലെങ്കിൽ {
            nodep->v3fatalSrc("Unknown node type");
        }

        VL_RESTORER(m_passsp);
        VL_RESTORER(m_failsp);
        VL_RESTORER(m_assertp);
        m_passsp = passsp;
        m_failsp = failsp;
        m_assertp = nodep;
        iterate(nodep->propp());

        Ast_കണ്ണി* propExprp;
        Ast_കണ്ണി_വ്യഞ്ജകം* disablep = nullptr;
        Ast_കണ്ണി_വ്യഞ്ജകം* matchCountp = nullptr;
        എങ്കിൽ (AstPropSpec* സ്ഥിരം specp = VN_CAST(nodep->propp(), PropSpec)) {
            propExprp = specp->propp()->unlinkFrBack();
            എങ്കിൽ (specp->disablep()) disablep = specp->disablep()->unlinkFrBack();
            matchCountp = specp->matchCountp();
        } അല്ലെങ്കിൽ {
            propExprp = nodep->propp()->unlinkFrBack();
        }
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        തർക്കം passspAlreadyGated = അസത്യം;
        എങ്കിൽ (passsp && VN_IS(passsp, If)) passspAlreadyGated = VN_AS(passsp, If)->user1();
        എങ്കിൽ (passsp && !passspGated && !passspAlreadyGated && !VN_IS(propExprp, PExpr)
            && !seqEvent) {
            passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                       /*vacuous=*/അസത്യം);
        }
        എങ്കിൽ (failsp && !VN_IS(propExprp, PExpr)) {
            failsp = newIfAssertFailOn(failsp, nodep->directive(), nodep->userType());
        }
        എങ്കിൽ (coverp && matchCountp && passsp) {
            // Convert the match count into a loop that decrements a temporary variable until it
            // reaches zero.
            matchCountp->unlinkFrBack();
            AstVar* സ്ഥിരം remainingp = പുതിയ AstVar{
                flp, VVarType::MODULETEMP, m_match_എണ്ണം_Names.get(""), matchCountp->dtypep()};
            remainingp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remainingp);
            Ast_കണ്ണി* സ്ഥിരം replaysp
                = പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, remainingp, VAccess::WRITE}, matchCountp};
            AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{flp};
            loopp->addStmtsp(
                പുതിയ AstLoopTest{flp, loopp, പുതിയ AstVarRef{flp, remainingp, VAccess::READ}});
            loopp->addStmtsp(passsp);
            loopp->addStmtsp(
                പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, remainingp, VAccess::WRITE},
                              പുതിയ AstSub{flp, പുതിയ AstVarRef{flp, remainingp, VAccess::READ},
                                         പുതിയ AstConst{flp, AstConst::Widthed_മൂല്യം{},
                                                      remainingp->dtypep()->width(), 1}}});
            replaysp->addNext(loopp);
            passsp = replaysp;
        }
        Ast_കണ്ണി* bodysp = assertBody(nodep, propExprp, passsp, failsp);
        എങ്കിൽ (disablep) bodysp = പുതിയ AstIf{flp, പുതിയ AstLogNot{flp, disablep}, bodysp};
        // Add assertOn check last, for better combining
        എങ്കിൽ (!seqEvent) bodysp = newIfAssertOn(bodysp, nodep->directive(), nodep->userType());
        എങ്കിൽ (sentreep) bodysp = പുതിയ AstAlways{flp, VAlwaysKwd::ALWAYS, sentreep, bodysp};

        എങ്കിൽ (passsp && !passsp->backp()) VL_DO_DANGLING(തള്ളുക_Deletep(passsp), passsp);
        എങ്കിൽ (failsp && !failsp->backp()) VL_DO_DANGLING(തള്ളുക_Deletep(failsp), failsp);

        // Install it
        എങ്കിൽ (selfDestruct) {
            // Delete it after making the tree.  This way we can tell the user
            // if it wasn't constructed nicely or has other errors without needing --coverage.
            VL_DO_DANGLING(bodysp->delete_വൃക്ഷം(), bodysp);
            nodep->unlinkFrBack();
        } അല്ലെങ്കിൽ {
            nodep->replaceWith(bodysp);
        }
        // Bye
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    തർക്കം isEmptyStmt(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (!nodep) തിരികെ സത്യം;
        എങ്കിൽ (AstBegin* സ്ഥിരം beginp = VN_CAST(nodep, Begin)) {
            എങ്കിൽ (beginp->declsp()) തിരികെ അസത്യം;
            വേണ്ടി (Ast_കണ്ണി* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                എങ്കിൽ (!isEmptyStmt(stmtp)) തിരികെ അസത്യം;
            }
            തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }

    // VISITORS
    ശൂന്യം visit(AstIf* nodep) override {
        എങ്കിൽ (!nodep->user1SetOnce()) {
            എങ്കിൽ (nodep->uniquePragma() || nodep->unique0Pragma()) {
                സ്ഥിരം Ast_കണ്ണി_If* ifp = nodep;
                Ast_കണ്ണി_വ്യഞ്ജകം* propp = nullptr;
                തർക്കം hasDefaultElse = അസത്യം;
                ചെയ്യുക {
                    // If this statement ends with 'else if', then nextIf will point to the
                    // nextIf statement.  Otherwise it will be null.
                    സ്ഥിരം Ast_കണ്ണി_If* സ്ഥിരം nextifp = dynamic_cast<Ast_കണ്ണി_If*>(ifp->elsesp());
                    iterateAndNextNull(ifp->condp());

                    // Recurse into the true case.
                    iterateAndNextNull(ifp->thensp());

                    // If the last else is not an else if, recurse into that too.
                    എങ്കിൽ (ifp->elsesp() && !nextifp) {  //
                        iterateAndNextNull(ifp->elsesp());
                    }

                    // Build a bitmask of the true predicates
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം predp = ifp->condp()->clone_വൃക്ഷം_Pure(അസത്യം);
                    എങ്കിൽ (propp) {
                        propp = പുതിയ AstConcat{nodep->fileline(), predp, propp};
                    } അല്ലെങ്കിൽ {
                        propp = predp;
                    }

                    // Record if this ends with an 'else' that does not have an if
                    എങ്കിൽ (ifp->elsesp() && !nextifp) hasDefaultElse = സത്യം;

                    ifp = nextifp;
                } ആകുമ്പോൾ (ifp);

                സ്ഥിരം തർക്കം allow_none = nodep->unique0Pragma();

                // Empty case means no property
                എങ്കിൽ (!propp) propp = പുതിയ AstConst{nodep->fileline(), AstConst::BitFalse{}};

                // Note: if this ends with an 'else', then we don't need to validate that one of
                // the predicates evaluates to true.
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം ohot
                    = ((allow_none || hasDefaultElse)
                           ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstOneHot0{nodep->fileline(), propp})
                           : static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstOneHot{nodep->fileline(), propp}));
                സ്ഥിരം VAssertType assertType
                    = nodep->uniquePragma() ? VAssertType::UNIQUE : VAssertType::UNIQUE0;
                // Move, not clone, as the branches might declare referenced variables
                VNRelinker relinker;
                nodep->unlinkFrBack(&relinker);
                AstIf* സ്ഥിരം checkifp
                    = പുതിയ AstIf{nodep->fileline(), പുതിയ AstLogNot{nodep->fileline(), ohot},
                                newFireAssert(nodep, VAssertDirectiveType::VIOLATION_IF,
                                              assertType, "'unique if' statement violated"),
                                nodep};
                checkifp->isBoundsCheck(സത്യം);  // To avoid LATCH warning
                checkifp->branchPred(VBranchPred::BP_UNLIKELY);
                relinker.relink(checkifp);
                തിരികെ;
            }
        }

        iterateChildren(nodep);

        എങ്കിൽ (nodep->user2()) {
            // Combine consecutive assertOn checks if possible
            എങ്കിൽ (AstIf* സ്ഥിരം backp = VN_CAST(nodep->backp(), If)) {
                എങ്കിൽ (backp->nextp() == nodep  //
                    && backp->user2()  //
                    && backp->condp()->same_വൃക്ഷം(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    backp->addThensp(nodep->thensp()->unlinkFrBackWithNext());
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    തിരികെ;
                }
            }
            // Combine nested assertOn checks if possible
            എങ്കിൽ (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* സ്ഥിരം checkp = VN_CAST(nodep->thensp(), If);
                എങ്കിൽ (checkp  //
                    && checkp->user2()  //
                    && checkp->condp()->same_വൃക്ഷം(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    VL_DO_DANGLING(തള്ളുക_Deletep(checkp->unlinkFrBack()), checkp);
                    തിരികെ;
                }
            }
            തിരികെ;
        }

        // Swap assertOn check with single statement 'if' statement to bubble up for combining
        // Note we can't just swap the conditions as they two Ifs have different flags,
        // so swapping the Ifs themeselves then swapping back the bodies.
        എങ്കിൽ (nodep->condp()->isPure()) {
            എങ്കിൽ (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* സ്ഥിരം checkp = VN_CAST(nodep->thensp(), If);
                എങ്കിൽ (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    തിരികെ;
                }
            }
            എങ്കിൽ (nodep->elsesp() && !nodep->elsesp()->nextp() && isEmptyStmt(nodep->thensp())) {
                AstIf* സ്ഥിരം checkp = VN_CAST(nodep->elsesp(), If);
                എങ്കിൽ (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addElsesp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    തിരികെ;
                }
            }
        }
    }

    //========== Case assertions
    ശൂന്യം visit(AstCase* nodep) override {
        // Introduce temporary variable for AstCase if needed - it is done here and not in V3Case
        // because this phase is before V3Scope and V3Case is not. Doing it before V3Scope ensures
        // that V3Scope will take care of a scope creation
        // We also need to do it before V3Begin, co that pragmas like `unique0` also work correctly
        എങ്കിൽ (!nodep->exprp()->isPure()) {
            ++m_statLiftedCaseExprs;
            File_വരി* സ്ഥിരം fl = nodep->exprp()->fileline();
            AstVar* സ്ഥിരം varp = പുതിയ AstVar{fl, VVarType::BLOCKTEMP, m_caseTempNames.get(nodep),
                                            nodep->exprp()->dtypep()};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം origp = nodep->exprp()->unlinkFrBack();
            nodep->addHereThisAsNext(
                പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, varp, VAccess::WRITE}, origp});
            nodep->exprp(പുതിയ AstVarRef{fl, varp, VAccess::READ});
            എങ്കിൽ (m_ftaskp) {
                varp->funcLocal(സത്യം);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } അല്ലെങ്കിൽ {
                m_modp->prependStmtsp(varp);
            }
            VIsCached::clear_Cache_വൃക്ഷം();
        }
        iterateChildren(nodep);
        എങ്കിൽ (!nodep->user1SetOnce()) {
            തർക്കം has_default = അസത്യം;
            വേണ്ടി (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                എങ്കിൽ (itemp->isDefault()) has_default = സത്യം;
            }
            സ്ഥിരം Ast_കണ്ണി_DType* വ്യഞ്ജകം_Dtypep = nodep->exprp()->dtypep()->skipRefp();

            VAssertType assertType = VAssertType::INTERNAL;
            എങ്കിൽ (nodep->priorityPragma()) {
                assertType = VAssertType::PRIORITY;
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->uniquePragma()) {
                assertType = VAssertType::UNIQUE;
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->unique0Pragma()) {
                assertType = VAssertType::UNIQUE0;
            }

            string valFmt;
            എങ്കിൽ (വ്യഞ്ജകം_Dtypep->isIntegralOrPacked())
                valFmt = " for '" + cvtToStr(വ്യഞ്ജകം_Dtypep->widthMin()) + "'h%X'";
            എങ്കിൽ (nodep->fullPragma() || nodep->priorityPragma()) {
                // Need to add a default if there isn't one already
                ++m_statAsFull;
                എങ്കിൽ (!has_default) {
                    nodep->addItemsp(പുതിയ AstCaseItem{
                        nodep->fileline(), nullptr /*DEFAULT*/,
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      nodep->pragmaString() + ", but non-match found" + valFmt,
                                      valFmt.empty() ? nullptr
                                                     : nodep->exprp()->clone_വൃക്ഷം_Pure(അസത്യം))});
                }
            }
            എങ്കിൽ (nodep->parallelPragma() || nodep->uniquePragma() || nodep->unique0Pragma()) {
                // Need to check that one, and only one of the case items match at any moment
                // If there's a default, we allow none to match, else exactly one must match
                ++m_statAsFull;
                എങ്കിൽ (!has_default && !nodep->itemsp()) {
                    // Not parallel, but harmlessly so.
                } അല്ലെങ്കിൽ {
                    Ast_കണ്ണി_വ്യഞ്ജകം* propp = nullptr;
                    വേണ്ടി (AstCaseItem* itemp = nodep->itemsp(); itemp;
                         itemp = VN_AS(itemp->nextp(), CaseItem)) {
                        Ast_കണ്ണി_വ്യഞ്ജകം* itembitp = nullptr;
                        വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* icondp = itemp->condsp(); icondp;
                             icondp = VN_AS(icondp->nextp(), കണ്ണി_വ്യഞ്ജകം)) {
                            Ast_കണ്ണി_വ്യഞ്ജകം* onep;
                            എങ്കിൽ (AstInsideRange* സ്ഥിരം rcondp = VN_CAST(icondp, InsideRange)) {
                                onep = rcondp->newAndFromInside(
                                    nodep->exprp()->clone_വൃക്ഷം_Pure(സത്യം),
                                    rcondp->lhsp()->clone_വൃക്ഷം_Pure(സത്യം),
                                    rcondp->rhsp()->clone_വൃക്ഷം_Pure(സത്യം));
                            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->casex() || nodep->casez() || nodep->caseInside()) {
                                onep = AstEqWild::newTyped(itemp->fileline(),
                                                           nodep->exprp()->clone_വൃക്ഷം_Pure(അസത്യം),
                                                           icondp->clone_വൃക്ഷം_Pure(അസത്യം));
                            } അല്ലെങ്കിൽ {
                                onep = AstEq::newTyped(icondp->fileline(),
                                                       nodep->exprp()->clone_വൃക്ഷം_Pure(അസത്യം),
                                                       icondp->clone_വൃക്ഷം_Pure(അസത്യം));
                            }
                            // OR together all conditions within the same case item
                            എങ്കിൽ (onep) {
                                എങ്കിൽ (itembitp) {
                                    itembitp = പുതിയ AstOr{icondp->fileline(), onep, itembitp};
                                } അല്ലെങ്കിൽ {
                                    itembitp = onep;
                                }
                            }
                        }
                        എങ്കിൽ (itembitp) {
                            എങ്കിൽ (propp) {
                                propp = പുതിയ AstConcat{itemp->fileline(), itembitp, propp};
                            } അല്ലെങ്കിൽ {
                                propp = itembitp;
                            }
                        }
                    }
                    // Empty case means no property
                    എങ്കിൽ (!propp) propp = പുതിയ AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    സ്ഥിരം തർക്കം allow_none = has_default || nodep->unique0Pragma();
                    // The following assertion looks as below.
                    // if (!$onehot(propp)) begin
                    //     if (propp == '0) begin if (!allow_none) $error("none match"); end
                    //     else $error("multiple match");
                    // end
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം ohot = പുതിയ AstOneHot{nodep->fileline(), propp};
                    AstConst* സ്ഥിരം zero = പുതിയ AstConst{
                        nodep->fileline(), AstConst::Widthed_മൂല്യം{}, propp->width(), 0};
                    AstIf* സ്ഥിരം ohotIfp
                        = പുതിയ AstIf{nodep->fileline(), പുതിയ AstLogNot{nodep->fileline(), ohot}};
                    AstIf* സ്ഥിരം zeroIfp = പുതിയ AstIf{
                        nodep->fileline(),
                        പുതിയ AstEq{nodep->fileline(), propp->clone_വൃക്ഷം_Pure(അസത്യം), zero}};
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = nodep->exprp();
                    സ്ഥിരം string pragmaStr = nodep->pragmaString();
                    എങ്കിൽ (!allow_none)
                        zeroIfp->addThensp(
                            newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                          pragmaStr + ", but none matched" + valFmt,
                                          valFmt.empty() ? nullptr : exprp->clone_വൃക്ഷം_Pure(അസത്യം)));
                    zeroIfp->addElsesp(
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      pragmaStr + ", but multiple matches found" + valFmt,
                                      valFmt.empty() ? nullptr : exprp->clone_വൃക്ഷം_Pure(അസത്യം)));
                    ohotIfp->addThensp(zeroIfp);
                    ohotIfp->isBoundsCheck(സത്യം);  // To avoid LATCH warning
                    ohotIfp->branchPred(VBranchPred::BP_UNLIKELY);
                    nodep->addNotParallelp(ohotIfp);
                }
            }
        }
    }

    ശൂന്യം visit(AstFuture* nodep) override {
        nodep->v3error("Future sampled value function called outside property or sequence "
                       "expression (IEEE 16.9.4)");
        nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), 0});
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    //========== Past
    ശൂന്യം visit(AstPast* nodep) override {
        എങ്കിൽ (!nodep->propertyTiming()) {
            nodep->initp(nodep->exprp()->clone_വൃക്ഷം_Pure(അസത്യം));
            { Default_Sampled_മൂല്യം_Visitor{nodep->initp()}; }
        }
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം initp = nodep->initp() ? nodep->initp()->unlinkFrBack() : nullptr;
        iterateChildren(nodep);
        uint32_t ticks = 1;
        എങ്കിൽ (nodep->ticksp()) {
            UASSERT_OBJ(VN_IS(nodep->ticksp(), Const), nodep,
                        "Expected constant ticks, checked in V3Width");
            ticks = VN_AS(nodep->ticksp(), Const)->toUInt();
        }
        UASSERT_OBJ(ticks >= 1, nodep, "0 tick should have been checked in V3Width");
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = new_Sampled_വ്യഞ്ജകം(nodep->exprp()->unlinkFrBack());
        Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep = nodep->sentreep()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* inp = nullptr;
        // Deliberately emitted for every ftask $past; calls outside final take the normal stage
        എങ്കിൽ (VN_IS(m_procedurep, Final) || m_ftaskp) {
            // A sampling-tick finish reads one stage deeper (IEEE 1800-2023 16.9.3).
            File_വരി* സ്ഥിരം flp = nodep->fileline();
            AstAlways* സ്ഥിരം alwaysp
                = getDelayedAlways(exprp, initp ? initp->clone_വൃക്ഷം_Pure(അസത്യം) : nullptr, senTreep);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം betweenTicksp = past_മൂല്യം_Ref(alwaysp, flp, ticks, initp);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം onTickp = past_മൂല്യം_Ref(alwaysp, flp, ticks + 1, initp);
            എങ്കിൽ (initp) VL_DO_DANGLING(തള്ളുക_Deletep(initp), initp);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം onTickCondp = പുതിയ AstLogAnd{
                flp, പുതിയ AstCExpr{flp, "vlSymsp->_vm_contextp__->executingFinal()", 1},
                പുതിയ AstEq{flp, പുതിയ AstVarRef{flp, getPastTickTimeVar(alwaysp), VAccess::READ},
                          പുതിയ AstCExpr{flp, "vlSymsp->_vm_contextp__->finishPendingTime()", 64}}};
            AstCond* സ്ഥിരം condp = പുതിയ AstCond{flp, onTickCondp, onTickp, betweenTicksp};
            condp->dtypeFrom(onTickp);
            inp = condp;
        } അല്ലെങ്കിൽ {
            inp = get_Past_മൂല്യം(exprp, initp, senTreep, ticks);
        }
        nodep->replaceWith(inp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    //========== Move $sampled down to read-only variables
    ശൂന്യം visit(AstSampled* nodep) override {
        എങ്കിൽ (nodep->user1()) തിരികെ;
        VL_RESTORER(m_inSampled);
        {
            m_inSampled = സത്യം;
            iterateChildren(nodep);
        }
        എങ്കിൽ (nodep->exprp()) {
            nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        } അല്ലെങ്കിൽ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        എങ്കിൽ (m_inSampled && !nodep->user2() && !nodep->varp()->isTemp()) {
            എങ്കിൽ (!nodep->access().isReadOnly()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Write to variable in sampled expression");
            } അല്ലെങ്കിൽ {
                VNRelinker relinkHandle;
                nodep->unlinkFrBack(&relinkHandle);
                AstSampled* സ്ഥിരം newp = new_Sampled_വ്യഞ്ജകം(nodep);
                relinkHandle.relink(newp);
                newp->user1(1);
                v3Global.setHasSampled();
            }
        }
    }
    // Don't sample sensitivities
    ശൂന്യം visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSampled);
        m_inSampled = അസത്യം;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstPExprClause* nodep) override {
        Ast_കണ്ണി* stmtsp = nullptr;
        എങ്കിൽ (nodep->pass() && m_passsp) {
            // Cover adds COVERINC by AstNode::addNext, thus need to clone next too.
            stmtsp = m_passsp->clone_വൃക്ഷം(സത്യം);
            stmtsp = newIfAssertPassOn(stmtsp, m_assertp->directive(), m_assertp->userType(),
                                       nodep->vacuous());
        } അല്ലെങ്കിൽ എങ്കിൽ (!nodep->pass() && m_failsp) {
            stmtsp = m_failsp->clone_വൃക്ഷം(സത്യം);
            stmtsp = newIfAssertFailOn(stmtsp, m_assertp->directive(), m_assertp->userType());
        }
        എങ്കിൽ (stmtsp) {
            stmtsp->foreachAndNext([](Ast_കണ്ണി_Var_Ref* സ്ഥിരം refp) {
                // References inside action blocks shouldn't be implicitly sampled
                // m_passsp/m_failsp have been already visited once and refs explicitly sampled
                // are handled already
                refp->user2(1);
            });
            nodep->replaceWith(stmtsp);
        } അല്ലെങ്കിൽ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstPExpr* nodep) override {
        എങ്കിൽ (m_inRestrict) {
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
        } അല്ലെങ്കിൽ {
            iterateChildren(nodep);
        }
    }

    //========== Statements
    ശൂന്യം visit(AstDisplay* nodep) override {
        iterateChildren(nodep);
        // Replace the special types with standard text
        എങ്കിൽ (nodep->displayType() == VDisplayType::DT_INFO) {
            replaceDisplay(nodep, "-Info");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->displayType() == VDisplayType::DT_WARNING) {
            replaceDisplay(nodep, "%%Warning");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->displayType() == VDisplayType::DT_ERROR) {
            replaceDisplay(nodep, "%%Error");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->displayType() == VDisplayType::DT_FATAL) {
            replaceDisplay(nodep, "%%Fatal");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->displayType() == VDisplayType::DT_MONITOR) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            File_വരി* സ്ഥിരം fl = nodep->fileline();

            AstSenItem* monSenItemsp = nullptr;
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം monExprsp = nodep->fmtp()->exprsp()) {
                monExprsp->foreachAndNext([&](AstVarRef* varrefp) {
                    AstSenItem* സ്ഥിരം senItemp
                        = പുതിയ AstSenItem{fl, VEdgeType::ET_CHANGED,
                                         // Clone so get VarRef or VarXRef as needed
                                         varrefp->clone_വൃക്ഷം(അസത്യം)};
                    monSenItemsp = Ast_കണ്ണി::addNextNull(monSenItemsp, senItemp);
                });
            }

            Ast_Sen_വൃക്ഷം* സ്ഥിരം mon_Sen_വൃക്ഷം = പുതിയ Ast_Sen_വൃക്ഷം{fl, monSenItemsp};
            സ്ഥിരം auto mon_അക്കം = ++m_monitor_അക്കം;
            // Where $monitor was we do "__VmonitorNum = N;"
            AstAssign* സ്ഥിരം newsetp = പുതിയ AstAssign{
                fl, new_Monitor_അക്കം_Var_Refp(nodep, VAccess::WRITE), പുതിയ AstConst{fl, mon_അക്കം}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__VmonitorOn && __VmonitorNum==N) $display(...);"
            Ast_കണ്ണി* സ്ഥിരം stmtsp = nodep;
            AstIf* സ്ഥിരം ifp = പുതിയ AstIf{
                fl,
                പുതിയ AstLogAnd{fl, പുതിയ AstLogNot{fl, newMonitorOffVarRefp(nodep, VAccess::READ)},
                              പുതിയ AstEq{fl, പുതിയ AstConst{fl, mon_അക്കം},
                                        new_Monitor_അക്കം_Var_Refp(nodep, VAccess::READ)}},
                stmtsp};
            ifp->isBoundsCheck(സത്യം);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstAlways{fl, VAlwaysKwd::ALWAYS, mon_Sen_വൃക്ഷം, ifp};
            m_modp->addStmtsp(newp);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->displayType() == VDisplayType::DT_STROBE) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            // Need one-shot
            File_വരി* സ്ഥിരം fl = nodep->fileline();
            AstVar* സ്ഥിരം varp
                = പുതിയ AstVar{fl, VVarType::MODULETEMP, "__Vstrobe" + cvtToStr(m_mod_Strobe_അക്കം++),
                             nodep->findBitDType()};
            m_modp->addStmtsp(varp);
            // Where $strobe was we do "__Vstrobe = '1;"
            AstAssign* സ്ഥിരം newsetp = പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, varp, VAccess::WRITE},
                                                     പുതിയ AstConst{fl, AstConst::BitTrue{}}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__Vstrobe) begin $display(...); __Vstrobe = '0; end"
            Ast_കണ്ണി* സ്ഥിരം stmtsp = nodep;
            AstIf* സ്ഥിരം ifp = പുതിയ AstIf{fl, പുതിയ AstVarRef{fl, varp, VAccess::READ}, stmtsp};
            ifp->isBoundsCheck(സത്യം);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstAlwaysPostponed{fl, ifp};
            stmtsp->addNext(പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, varp, VAccess::WRITE},
                                          പുതിയ AstConst{fl, AstConst::BitFalse{}}});
            m_modp->addStmtsp(newp);
        }
    }
    ശൂന്യം visit(AstMonitorOff* nodep) override {
        AstAssign* സ്ഥിരം newp
            = പുതിയ AstAssign{nodep->fileline(), newMonitorOffVarRefp(nodep, VAccess::WRITE),
                            പുതിയ AstConst{nodep->fileline(), AstConst::BitTrue{}, nodep->off()}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstAssert* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    ശൂന്യം visit(AstAssertCtl* nodep) override {
        iterateChildren(nodep);

        തർക്കം assertTypeConst = സത്യം;
        എങ്കിൽ (!nodep->assertTypesp()) {
            nodep->assertTypes(VAssertType{ALL_ASSERT_TYPES});
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം assertTypesp = VN_CAST(nodep->assertTypesp(), Const)) {
            nodep->assertTypes(VAssertType{assertTypesp->toSInt()});
        } അല്ലെങ്കിൽ {
            assertTypeConst = അസത്യം;
        }

        തർക്കം controlTypeConst = അസത്യം;
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep->controlTypep(), Const)) {
            nodep->ctlType(constp->toSInt());
            controlTypeConst = സത്യം;
        }
        എങ്കിൽ (controlTypeConst
            && (nodep->ctlType() < VAssertCtlType::LOCK
                || nodep->ctlType() > VAssertCtlType::VACUOUS_OFF)) {
            nodep->unlinkFrBack();
            nodep->v3error("Bad $assertcontrol control_type '"
                           << cvtToStr(static_cast<പൂർണ്ണസംഖ്യ>(nodep->ctlType()))
                           << "' (IEEE 1800-2023 Table 20-5)");
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        }
        എങ്കിൽ (assertTypeConst && nodep->assertTypes() != ALL_ASSERT_TYPES
            && nodep->assertTypes().containsAny(VAssertType::UNIQUE | VAssertType::UNIQUE0
                                                | VAssertType::PRIORITY)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: assert control assertion_type");
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
        }

        തർക്കം directiveTypeConst = സത്യം;
        എങ്കിൽ (!nodep->directiveTypesp()) {
            nodep->directiveTypes(VAssertDirectiveType::ASSERT | VAssertDirectiveType::ASSUME
                                  | VAssertDirectiveType::COVER);
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം directiveTypesp
                   = VN_CAST(nodep->directiveTypesp(), Const)) {
            nodep->directiveTypes(VAssertDirectiveType{directiveTypesp->toSInt()});
        } അല്ലെങ്കിൽ {
            directiveTypeConst = അസത്യം;
        }
        എങ്കിൽ (!directiveTypeConst) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: non-const assert directive type expression");
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
        }

        File_വരി* സ്ഥിരം fl = nodep->fileline();
        UINFO(9, "Generating assertctl for a module: " << m_modp);
        AstCStmt* സ്ഥിരം newp = പുതിയ AstCStmt{fl};
        newp->add("vlSymsp->_vm_contextp__->assertCtl(");
        newp->add(nodep->controlTypep()->unlinkFrBack());
        newp->add(", ");
        എങ്കിൽ (nodep->assertTypesp()) {
            newp->add(nodep->assertTypesp()->unlinkFrBack());
        } അല്ലെങ്കിൽ {
            newp->add(std::to_string(ALL_ASSERT_TYPES));
        }
        newp->add(", " + std::to_string(nodep->directiveTypes()) + ");\n");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstAssertIntrinsic* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    ശൂന്യം visit(AstCover* nodep) override {  //
        visitAssertionIterate(nodep, nullptr);
    }
    ശൂന്യം visit(AstRestrict* nodep) override {
        VL_RESTORER(m_inRestrict);
        m_inRestrict = സത്യം;
        iterateChildren(nodep);
        // IEEE says simulator ignores these
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Past_അക്കം);
        VL_RESTORER(m_mod_Strobe_അക്കം);
        VL_RESTORER(m_finalp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DelayedAlwaysp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DefaultDelayedAlwaysp);
        VL_RESTORER_CLEAR(m_delayedAlways2TickTimep);
        m_modp = nodep;
        m_mod_Past_അക്കം = 0;
        m_mod_Strobe_അക്കം = 0;
        m_finalp = nullptr;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstGenBlock* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstBegin* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
        // If the body is a single assertOn check, bubble it up for combining
        എങ്കിൽ (!nodep->declsp()) {
            എങ്കിൽ (AstIf* സ്ഥിരം ifp = VN_CAST(nodep->stmtsp(), If)) {
                എങ്കിൽ (ifp->user2() && !ifp->nextp()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(ifp->unlinkFrBack());
                    nodep->addStmtsp(ifp->thensp()->unlinkFrBackWithNext());
                    ifp->addThensp(nodep);
                }
            }
        }
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
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
        VIsCached::clear_Cache_വൃക്ഷം();
    }
};

//######################################################################
// Top Assert class

ശൂന്യം V3Assert::assertAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("assert", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
