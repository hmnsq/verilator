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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Assert.h"

#અન્તર્ભાવ "V3AstUserAllocator.h"
#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3Stats.h"
#અન્તર્ભાવ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

નામાકાશ {

વર્ગ DefaultDisableLocalVisitor final : જાહેર VNVisitor {
    // STATE
    Ast_ગાંઠ* m_scopep = nullptr;

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(nullptr);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(nullptr);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstDefaultDisable* nodep) override {
        UASSERT_OBJ(nodep, m_scopep,
                    "default disable iff must be inside a module or generate block");
        AstDefaultDisable* defaultp = nullptr;
        જો (અચલ Ast_ગાંઠ_Module* અચલ modp = VN_CAST(m_scopep, ગાંઠ_Module)) {
            defaultp = modp->defaultDisablep();
        } નહીંતર {
            defaultp = VN_AS(m_scopep, GenBlock)->defaultDisablep();
        }
        જો (VL_UNLIKELY(defaultp)) {
            nodep->v3error("Only one 'default disable iff' allowed per "
                           << (VN_IS(m_scopep, ગાંઠ_Module) ? "module" : "generate block")
                           << " (IEEE 1800-2023 16.15)");
        } નહીંતર જો (Ast_ગાંઠ_Module* અચલ modp = VN_CAST(m_scopep, ગાંઠ_Module)) {
            modp->defaultDisablep(nodep);
        } નહીંતર {
            VN_AS(m_scopep, GenBlock)->defaultDisablep(nodep);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    explicit DefaultDisableLocalVisitor(AstNetlist* nodep) { iterate(nodep); }
};

વર્ગ DefaultDisablePropagateVisitor final : જાહેર VNVisitor {
    // STATE
    AstDefaultDisable* m_defaultDisablep = nullptr;

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        જો (!nodep->defaultDisablep()) nodep->defaultDisablep(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    explicit DefaultDisablePropagateVisitor(AstNetlist* nodep) { iterate(nodep); }
};

// Lower a sequence used as an event control ('@seq', IEEE 1800-2023 9.4.2.4) into a
// synthesized event fired by an internal 'cover sequence' on each end-of-match
વર્ગ SeqEventLowerVisitor final : જાહેર VNVisitor {
    // STATE
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    V3UniqueNames m_names{"__VseqEvent"};  // Synthesized event names

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstSenItem* nodep) override {
        AstFuncRef* અચલ funcrefp = VN_CAST(nodep->sensp(), FuncRef);
        જો (funcrefp && VN_IS(funcrefp->taskp(), Sequence)) {
            File_લીટી* અચલ flp = nodep->fileline();
            AstVar* અચલ eventp = નવું AstVar{flp, VVarType::MODULETEMP, m_names.get(nodep),
                                              m_modp->findBasicDType(VBasicDTypeKwd::EVENT)};
            eventp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(eventp);
            v3Global.setHasEvents();
            funcrefp->unlinkFrBack();
            nodep->sensp(નવું AstVarRef{flp, eventp, VAccess::READ});
            અચલ તાર્કિક automaticActual = funcrefp->exists([](અચલ Ast_ગાંઠ_Var_Ref* refp) {
                પાછા refp->varp() && refp->varp()->lifetime().isAutomatic();
            });
            જો (automaticActual) {
                nodep->v3error("Arguments to a sequence used as an event control must be"
                               " static (IEEE 1800-2023 9.4.2.4)");
                VN_AS(funcrefp->taskp(), Sequence)->isReferenced(ખોટું);
                VL_DO_DANGLING(ધકેલો_Deletep(funcrefp), funcrefp);
                પાછા;
            }
            AstFireEvent* અચલ firep
                = નવું AstFireEvent{flp, નવું AstVarRef{flp, eventp, VAccess::WRITE}, ખોટું};
            AstCover* અચલ coverp
                = નવું AstCover{flp, નવું AstPropSpec{flp, nullptr, nullptr, funcrefp}, firep,
                               VAssertType::CONCURRENT};
            coverp->isCoverSeq(સાચું);
            coverp->isSeqEvent(સાચું);
            m_modp->addStmtsp(coverp);
            પાછા;
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    explicit SeqEventLowerVisitor(AstNetlist* nodep) { iterate(nodep); }
};

}  // namespace

શૂન્ય V3AssertCommon::collectDefaultDisable(AstNetlist* nodep) {
    { DefaultDisableLocalVisitor{nodep}; }
    { DefaultDisablePropagateVisitor{nodep}; }
}

શૂન્ય V3AssertCommon::lowerSequenceEvents(AstNetlist* nodep) {
    { SeqEventLowerVisitor{nodep}; }
    V3Global::dump_Check_Global_વૃક્ષ("assertseqevent", 0, dump_વૃક્ષ_Either_Level() >= 3);
}

string V3AssertCommon::assertCtlGetCall(અચલ અક્ષર* query, VAssertType type,
                                        VAssertDirectiveType directiveType) {
    // Template arguments let the runtime build the type/directive mask at C++ compile time
    પાછા "vlSymsp->_vm_contextp__->assertCtlGet<"s + std::to_string(type) + ", "s
           + std::to_string(directiveType) + ">(VerilatedAssertCtlQuery::"s + query + ")"s;
}

તાર્કિક V3AssertCommon::check_Sva_કિંમત_Limit(Ast_ગાંઠ* nodep, અચલ AstConst* constp, અચલ અક્ષર* what) {
    // Temporal values can materialize as O(N) AST or state in later passes.
    // Leave headroom for a range ring's extra slot and V3Number's signed
    // `(bits + 31) / 32` word count.
    સ્થિર constexpr unsigned SVA_કિંમત_HARD_LIMIT
        = static_cast<unsigned>(std::numeric_limits<પૂર્ણાંક>::max()) - 32U;
    અચલ unsigned configuredLimit = static_cast<unsigned>(v3Global.opt.max_અંક_Width());
    અચલ unsigned limit = std::min(configuredLimit, SVA_કિંમત_HARD_LIMIT);
    જો (constp->અંક().fitsInUInt() && constp->toUInt() <= limit) પાછા સાચું;
    nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << what << " exceeds implementation limit of "
                                                 << limit
                                                 << (configuredLimit <= SVA_કિંમત_HARD_LIMIT
                                                         ? " (--max-num-width)"
                                                         : " (host arithmetic limit)"));
    પાછા ખોટું;
}

// The bound of a cycle delay in a sequence, if a non-negative elaboration-time constant
સ્થિર અચલ AstConst* checkSvaDelayBound(Ast_ગાંઠ* errp, Ast_ગાંઠ_પદાવલિ* boundp, અચલ અક્ષર* what) {
    અચલ AstConst* અચલ constp = VN_CAST(boundp, Const);
    જો (!constp || (constp->dtypep()->isSigned() && constp->અંક().isNegative())) {
        errp->v3error(what << " is not a non-negative elaboration-time constant"
                              " (IEEE 1800-2023 16.7)");
        પાછા nullptr;
    }
    જો (!V3AssertCommon::check_Sva_કિંમત_Limit(errp, constp, "SVA cycle delay")) પાછા nullptr;
    પાછા constp;
}

// Fold the bounds of a cycle delay in a sequence, and return if they are valid, else report it
// on errp
સ્થિર તાર્કિક svaCycleDelayValid(અચલ AstDelay* nodep, Ast_ગાંઠ* errp) {
    V3Const::constifyParamsNoWarnEdit(nodep->lhsp());
    અચલ AstConst* અચલ minConstp = checkSvaDelayBound(
        errp, nodep->lhsp(), nodep->isRangeDelay() ? "Range delay minimum" : "Delay value");
    જો (!nodep->rhsp() || nodep->isUnbounded()) પાછા minConstp != nullptr;
    V3Const::constifyParamsNoWarnEdit(nodep->rhsp());
    અચલ AstConst* અચલ maxConstp
        = checkSvaDelayBound(errp, nodep->rhsp(), "Range delay maximum");
    જો (!minConstp || !maxConstp) પાછા ખોટું;
    જો (maxConstp->toUInt() < minConstp->toUInt()) {
        errp->v3error("Range delay maximum must be >= minimum (IEEE 1800-2023 16.7)");
        પાછા ખોટું;
    }
    પાછા સાચું;
}

શૂન્ય V3AssertCommon::checkSvaCycleDelay(AstDelay* nodep, Ast_ગાંઠ* errp) {
    જો (svaCycleDelayValid(nodep, errp)) પાછા;
    // Make the delay valid, so later steps do not fail after the error
    File_લીટી* અચલ flp = nodep->fileline();
    Ast_ગાંઠ_પદાવલિ* અચલ minp = nodep->lhsp();
    minp->replaceWith(નવું AstConst{flp, AstConst::BitFalseErroring{}});
    VL_DO_DANGLING(minp->delete_વૃક્ષ(), minp);
    જો (nodep->rhsp() && !nodep->isUnbounded()) {
        Ast_ગાંઠ_પદાવલિ* અચલ maxp = nodep->rhsp();
        maxp->replaceWith(નવું AstConst{flp, AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(maxp->delete_વૃક્ષ(), maxp);
    }
}

તાર્કિક V3AssertCommon::refsSvaFormal(અચલ Ast_ગાંઠ* nodep) {
    પાછા nodep->exists([](અચલ Ast_ગાંઠ_Var_Ref* refp) {
        અચલ AstVar* અચલ varp = refp->varp();
        જો (!varp || !varp->isIO()) પાછા ખોટું;
        અચલ Ast_ગાંઠ* અચલ declp = varp->aboveLoopp();
        પાછા VN_IS(declp, Sequence) || VN_IS(declp, Property);
    });
}

શૂન્ય V3AssertCommon::checkSvaInstanceDelays(Ast_ગાંઠ* nodep, Ast_ગાંઠ* instp) {
    // A delay still referencing a formal argument of an enclosing sequence or property is
    // checked when that is instanced. Collect first, as checking edits the delays.
    std::vector<AstDelay*> delayps;
    nodep->foreach([&](AstDelay* delayp) {
        જો (!delayp->isCycleDelay() || refsSvaFormal(delayp->lhsp())) પાછા;
        જો (delayp->rhsp() && refsSvaFormal(delayp->rhsp())) પાછા;
        delayps.ધકેલો_back(delayp);
    });
    માટે (AstDelay* અચલ delayp : delayps) checkSvaCycleDelay(delayp, instp);
}

//######################################################################
// AssertDeFutureVisitor
// If any AstFuture, then move all non-future varrefs to be one cycle behind,
// see IEEE 1800-2023 16.9.4.

વર્ગ AssertDeFutureVisitor final : જાહેર VNVisitor {
    // STATE - across all visitors
    Ast_ગાંઠ_Module* અચલ m_modp;  // Module future is underneath
    અચલ AstFuture* m_futurep;  // First AstFuture found
    અચલ unsigned m_past_અંક;  // Prefix unique number for this module
    std::map<AstVar*, AstVar*> m_delayedVars;  // Old to delayed variable mapping
    // STATE - for current visit position (use VL_RESTORER)
    તાર્કિક m_inFuture = ખોટું;  // Inside a future
    તાર્કિક m_unsupported = ખોટું;  // Printed unsupported

    // METHODS
    શૂન્ય unsupported(Ast_ગાંઠ* nodep) {
        જો (m_unsupported) પાછા;
        m_unsupported = સાચું;
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported/illegal: Future value function used with expression with "
                          << nodep->prettyOperatorName());
    }
    // VISITORS
    શૂન્ય visit(AstFuture* nodep) override {
        VL_RESTORER(m_inFuture);
        m_inFuture = સાચું;
        iterateChildren(nodep);
        // Done with the future, this subexpression is current-time
        nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;
        જો (m_inFuture || m_unsupported)
            પાછા;  // Need user1 set above, don't process when Future is removed
        જો (nodep->access().isWriteOrRW()) {
            unsupported(nodep);
            પાછા;
        }
        auto it = m_delayedVars.find(nodep->varp());
        AstVar* outvarp;
        જો (it == m_delayedVars.end()) {
            Ast_Sen_વૃક્ષ* અચલ sentreep = m_futurep->sentreep();
            AstAlways* અચલ alwaysp = નવું AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS,
                                                     sentreep->clone_વૃક્ષ(ખોટું), nullptr};
            m_modp->addStmtsp(alwaysp);
            outvarp = નવું AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                 "__Vnotfuture" + cvtToStr(m_past_અંક) + "_" + nodep->name(),
                                 nodep->dtypep()};
            m_modp->addStmtsp(outvarp);
            AstVarRef* varRefAWritep = નવું AstVarRef{nodep->fileline(), outvarp, VAccess::WRITE};
            varRefAWritep->user1(સાચું);
            Ast_ગાંઠ_Var_Ref* varRefAReadp = nodep->clone_વૃક્ષ(ખોટું);
            varRefAReadp->user1(સાચું);
            Ast_ગાંઠ* અચલ assp = નવું AstAssignDly{nodep->fileline(), varRefAWritep, varRefAReadp};
            alwaysp->addStmtsp(assp);
            m_delayedVars.emplace(nodep->varp(), outvarp);
        } નહીંતર {
            outvarp = it->second;
        }
        AstVarRef* newp = નવું AstVarRef{nodep->fileline(), outvarp, VAccess::READ};
        newp->user1(સાચું);
        UINFO(9, "DeFuture " << nodep << "  becomes " << newp);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override { unsupported(nodep); }
    શૂન્ય visit(AstMethodCall* nodep) override { unsupported(nodep); }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        જો (!nodep->isPure()) unsupported(nodep);
        iterateChildren(nodep);
    }

જાહેર:
    // CONSTRUCTORS
    explicit AssertDeFutureVisitor(Ast_ગાંઠ* nodep, Ast_ગાંઠ_Module* modp, unsigned past_અંક)
        : m_modp{modp}
        , m_past_અંક{past_અંક} {
        // See if any Future before we process
        જો (nodep->forall([&](અચલ AstFuture* futurep) -> તાર્કિક {
                m_futurep = futurep;
                પાછા ખોટું;
            }))
            પાછા;
        // UINFOTREE(9, nodep, "", "defuture-in");
        visit(nodep);  // Nodep may get deleted
        // UINFOTREE(9, nodep, "", "defuture-ou");
    }
    ~AssertDeFutureVisitor() = મૂળભૂત;
};

// Prepare one default sampled expression.
વર્ગ Default_Sampled_કિંમત_Visitor final : જાહેર VNVisitor {
    // METHODS
    સ્થિર AstConst* newTypeDefault(Ast_ગાંઠ_પદાવલિ* nodep) {
        AstConst* અચલ constp
            = નવું AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()};
        જો (nodep->dtypep()->isFourstate()) constp->અંક().setAllBitsX();
        constp->dtypeFrom(nodep);
        પાછા constp;
    }

    // VISITORS
    શૂન્ય visit(AstCMethodHard* nodep) override {
        જો (nodep->method() == VCMethod::EVENT_IS_TRIGGERED) {
            nodep->replaceWith(નવું AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        } નહીંતર {
            iterateChildren(nodep);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        AstVar* અચલ varp = nodep->varp();
        જો (varp->isParam()
            || (!varp->isNet() && varp->lifetime().isStatic() && varp->hasUserInit())) {
            પાછા;
        }
        nodep->replaceWith(newTypeDefault(nodep));
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    explicit Default_Sampled_કિંમત_Visitor(Ast_ગાંઠ* nodep) { iterate(nodep); }
};

//######################################################################
// AssertVisitor

વર્ગ AssertVisitor final : જાહેર VNVisitor {
    // CONSTANTS
    સ્થિર constexpr uint8_t ALL_ASSERT_TYPES
        = std::numeric_limits<std::underlying_type<VAssertType::en>::type>::max();

    // NODE STATE/TYPES
    // Cleared on netlist
    //  AstNode::user1()         -> bool.  True if processed
    //  AstAlways::user2p()      -> std::vector<AstVar*>. Delayed variables via 'm_delayed'
    //  AstNodeVarRef::user2()   -> bool.  True if shouldn't be sampled
    //  AstIf::user2()           -> bool.  True for assertOn() checks inserted in this pass
    અચલ VNUser1InUse m_user1InUse;
    અચલ VNUser2InUse m_user2InUse;
    AstUser2Allocator<AstAlways, std::vector<AstVar*>> m_delayed;

    // STATE
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Last module
    અચલ Ast_ગાંઠ* m_beginp = nullptr;  // Last AstBegin/AstGenBlock
    unsigned m_monitor_અંક = 0;  // Global $monitor numbering (not per module)
    AstVar* m_monitor_અંક_Varp = nullptr;  // $monitor number variable
    AstVar* m_monitorOffVarp = nullptr;  // $monitoroff variable
    unsigned m_mod_Past_અંક = 0;  // Module past numbering
    unsigned m_mod_Strobe_અંક = 0;  // Module $strobe numbering
    Ast_ગાંઠ_Procedure* m_procedurep = nullptr;  // Current procedure
    VDouble0 m_statCover;  // Statistic tracking
    VDouble0 m_statAsNotImm;  // Statistic tracking
    VDouble0 m_statAsImm;  // Statistic tracking
    VDouble0 m_statAsFull;  // Statistic tracking
    VDouble0 m_statPastVars;  // Statistic tracking
    VDouble0 m_statAssertOnCombined;  // Statistic tracking
    VDouble0 m_statAssertOnHoisted;  // Statistic tracking
    તાર્કિક m_inSampled = ખોટું;  // True inside a sampled expression
    તાર્કિક m_inRestrict = ખોટું;  // True inside restrict assertion
    Ast_ગાંઠ* m_passsp = nullptr;  // Current pass statement
    Ast_ગાંઠ* m_failsp = nullptr;  // Current fail statement
    Ast_ગાંઠ_Cover_Or_Assert* m_assertp = nullptr;  // Current assertion
    AstFinal* m_finalp = nullptr;  // Current final block
    VDouble0 m_statLiftedCaseExprs;  // Count of purified case expressions
    Ast_ગાંઠ_FTask* m_ftaskp = nullptr;  // Current function/task
    V3UniqueNames m_caseTempNames{"__VCase"};
    V3UniqueNames m_match_ગણતરી_Names{"__VnfaRemainingMatchCount"};  // Match replay counter names
    // Maps from (expression, senTree) to the AstAlways that computes its delayed values.
    std::unordered_map<VNRef<Ast_ગાંઠ_પદાવલિ>, std::unordered_map<VNRef<Ast_Sen_વૃક્ષ>, AstAlways*>>
        m_modExpr2Sen2DelayedAlwaysp;
    // Source sampled-value history is initialized, unlike internal property-lowering history.
    std::unordered_map<VNRef<Ast_ગાંઠ_પદાવલિ>, std::unordered_map<VNRef<Ast_Sen_વૃક્ષ>, AstAlways*>>
        m_modExpr2Sen2DefaultDelayedAlwaysp;
    // Map from delayed-value AstAlways to its last-sampling-tick time variable
    std::unordered_map<અચલ AstAlways*, AstVar*> m_delayedAlways2TickTimep;

    // METHODS
    સ્થિર અચલ અક્ષર* assertPassOnQuery(તાર્કિક vacuous) {
        સ્થિર constexpr અચલ અક્ષર* queries[2]
            = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
        પાછા queries[vacuous];
    }
    સ્થિર Ast_ગાંઠ_પદાવલિ* assertOnCond(File_લીટી* fl, VAssertType type,
                                     VAssertDirectiveType directiveType) {
        // cppcheck-suppress missingReturn
        પસંદગી (directiveType) {
        સ્થિતિ VAssertDirectiveType::INTRINSIC: પાછા નવું AstConst{fl, AstConst::BitTrue{}};
        સ્થિતિ VAssertDirectiveType::VIOLATION_CASE: {
            જો (v3Global.opt.assertCase()) {
                પાછા નવું AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            // If assertions are off, have constant propagation rip them out later
            // This allows syntax errors and such to be detected normally.
            પાછા નવું AstConst{fl, AstConst::BitFalse{}};
        }
        સ્થિતિ VAssertDirectiveType::ASSERT:
        સ્થિતિ VAssertDirectiveType::COVER:
        સ્થિતિ VAssertDirectiveType::ASSUME: {
            જો (v3Global.opt.assertOn()) {
                પાછા નવું AstCExpr{
                    fl, AstCExpr::Pure{},
                    V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
            }
            પાછા નવું AstConst{fl, AstConst::BitFalse{}};
        }
        સ્થિતિ VAssertDirectiveType::INTERNAL:
        સ્થિતિ VAssertDirectiveType::VIOLATION_IF:
        સ્થિતિ VAssertDirectiveType::RESTRICT: {
            જો (v3Global.opt.assertOn()) {
                પાછા નવું AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            પાછા નવું AstConst{fl, AstConst::BitFalse{}};
        }
        }
        VL_UNREACHABLE;
    }

    સ્થિર તાર્કિક isControlled(VAssertDirectiveType directiveType) {
        પાછા (static_cast<પૂર્ણાંક>(directiveType)
                & (static_cast<પૂર્ણાંક>(VAssertDirectiveType::ASSERT)
                   | static_cast<પૂર્ણાંક>(VAssertDirectiveType::COVER)
                   | static_cast<પૂર્ણાંક>(VAssertDirectiveType::ASSUME)));
    }
    સ્થિર Ast_ગાંઠ_પદાવલિ* assertPassOnCond(File_લીટી* fl, VAssertType type,
                                         VAssertDirectiveType directiveType, તાર્કિક vacuous) {
        જો (!isControlled(directiveType)) પાછા નવું AstConst{fl, AstConst::BitTrue{}};
        જો (!v3Global.opt.assertOn()) પાછા નવું AstConst{fl, AstConst::BitFalse{}};
        પાછા નવું AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType), 1};
    }
    સ્થિર Ast_ગાંઠ_પદાવલિ* assertFailOnCond(File_લીટી* fl, VAssertType type,
                                         VAssertDirectiveType directiveType) {
        જો (!isControlled(directiveType)) પાછા નવું AstConst{fl, AstConst::BitTrue{}};
        જો (!v3Global.opt.assertOn()) પાછા નવું AstConst{fl, AstConst::BitFalse{}};
        પાછા નવું AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType), 1};
    }
    string assertDisplayMessage(અચલ Ast_ગાંઠ* nodep, અચલ string& prefix, અચલ string& message,
                                VDisplayType severity) {
        જો (severity == VDisplayType::DT_ERROR || severity == VDisplayType::DT_FATAL) {
            પાછા ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                    + cvtToStr(nodep->fileline()->lineno()) + ": Assertion failed in %m"
                    + ((message != "") ? ": " : "") + message + "\n");
        }
        પાછા ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                + cvtToStr(nodep->fileline()->lineno()) + ": %m" + ((message != "") ? ": " : "")
                + message + "\n");
    }
    શૂન્ય replaceDisplay(AstDisplay* nodep, અચલ string& prefix) {
        nodep->fmtp()->text(
            assertDisplayMessage(nodep, prefix, nodep->fmtp()->text(), nodep->displayType()));
        nodep->displayType(VDisplayType::DT_WRITE);
        // cppcheck-suppress nullPointer
        Ast_ગાંઠ_પદાવલિ* અચલ timenewp = નવું AstTime{nodep->fileline(), m_modp->timeunit()};
        nodep->fmtp()->prependExprsp(timenewp);
        જો (!nodep->fmtp()->scopeNamep() && nodep->fmtp()->formatScopeTracking()) {
            nodep->fmtp()->scopeNamep(નવું AstScopeName{nodep->fileline(), સાચું});
        }
    }
    AstSampled* new_Sampled_પદાવલિ(Ast_ગાંઠ_પદાવલિ* nodep) {
        પાછા નવું AstSampled{nodep->fileline(), nodep, nodep->dtypep(), સાચું};
    }
    AstVarRef* new_Monitor_અંક_Var_Refp(અચલ Ast_ગાંઠ* nodep, VAccess access) {
        જો (!m_monitor_અંક_Varp) {
            m_monitor_અંક_Varp = નવું AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorNum",
                                          nodep->findUInt64DType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitor_અંક_Varp);
        }
        AstVarRef* અચલ varrefp = નવું AstVarRef{nodep->fileline(), m_monitor_અંક_Varp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        પાછા varrefp;
    }
    AstVarRef* newMonitorOffVarRefp(અચલ Ast_ગાંઠ* nodep, VAccess access) {
        જો (!m_monitorOffVarp) {
            m_monitorOffVarp = નવું AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorOff",
                                          nodep->findBitDType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitorOffVarp);
        }
        AstVarRef* અચલ varrefp = નવું AstVarRef{nodep->fileline(), m_monitorOffVarp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        પાછા varrefp;
    }
    સ્થિર AstIf* newIfAssertOn(Ast_ગાંઠ* bodyp, VAssertDirectiveType directiveType,
                                VAssertType type = VAssertType::INTERNAL) {
        // Add an internal if to check assertions are on.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_લીટી* અચલ fl = bodyp->fileline();

        Ast_ગાંઠ_પદાવલિ* અચલ condp = assertOnCond(fl, type, directiveType);
        AstIf* અચલ newp = નવું AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(સાચું);  // To avoid LATCH warning
        newp->user2(સાચું);  // Mark as an assertOn() check
        પાછા newp;
    }
    સ્થિર Ast_ગાંઠ_Stmt* newIfAssertPassOn(Ast_ગાંઠ* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type, તાર્કિક vacuous) {
        // Add an internal if to check assertion passOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_લીટી* અચલ fl = bodyp->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ condp = assertPassOnCond(fl, type, directiveType, vacuous);
        Ast_ગાંઠ_If* અચલ newp = નવું AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(સાચું);  // To avoid LATCH warning
        newp->user1(સાચું);  // Don't assert/cover this if
        newp->user2(સાચું);  // Mark as an assertOn() check
        પાછા newp;
    }
    સ્થિર Ast_ગાંઠ_Stmt* newIfAssertFailOn(Ast_ગાંઠ* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type) {
        // Add an internal if to check assertion failOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_લીટી* અચલ fl = bodyp->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ condp = assertFailOnCond(fl, type, directiveType);
        Ast_ગાંઠ_If* અચલ newp = નવું AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(સાચું);  // To avoid LATCH warning
        newp->user1(સાચું);  // Don't assert/cover this if
        newp->user2(સાચું);  // Mark as an assertOn() check
        પાછા newp;
    }

    સ્થિર AstIf* assertCond(અચલ Ast_ગાંઠ_Cover_Or_Assert* nodep, Ast_ગાંઠ_પદાવલિ* propp,
                             Ast_ગાંઠ* passsp, Ast_ગાંઠ* failsp) {
        AstIf* અચલ ifp = નવું AstIf{nodep->fileline(), propp, passsp, failsp};
        // It's more LIKELY that we'll take the nullptr if clause
        // than the sim-killing else clause:
        ifp->branchPred(VBranchPred::BP_LIKELY);
        ifp->isBoundsCheck(સાચું);  // To avoid LATCH warning
        પાછા ifp;
    }

    Ast_ગાંઠ_Stmt* assertBody(અચલ Ast_ગાંઠ_Cover_Or_Assert* nodep, Ast_ગાંઠ* propp, Ast_ગાંઠ* passsp,
                            Ast_ગાંઠ* failsp) {
        જો (AstPExpr* અચલ pexprp = VN_CAST(propp, PExpr)) {
            જો (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NOTIMING, "This property expression requires --timing");
                VL_DO_DANGLING(ધકેલો_Deletep(pexprp), pexprp);
                પાછા નવું AstBegin{nodep->fileline(), "", nullptr, ખોટું};
            }
            AstFork* અચલ forkp = નવું AstFork{nodep->fileline(), VJoinType::JOIN_NONE};
            forkp->addForksp(pexprp->bodyp()->unlinkFrBack());
            જો (Ast_ગાંઠ_Stmt* અચલ finalp = pexprp->finalp()) {
                જો (!m_finalp) {
                    m_finalp = નવું AstFinal{m_modp->fileline(), finalp->unlinkFrBack()};
                    m_modp->addStmtsp(m_finalp);
                } નહીંતર {
                    m_finalp->addStmtsp(finalp->unlinkFrBack());
                }
            }
            VL_DO_DANGLING2(ધકેલો_Deletep(pexprp), pexprp, propp);
            પાછા forkp;
        }

        પાછા assertCond(nodep, VN_AS(propp, ગાંઠ_પદાવલિ), passsp, failsp);
    }

    Ast_ગાંઠ_Stmt* newFireAssertUnchecked(અચલ Ast_ગાંઠ_Stmt* nodep, અચલ string& message,
                                        Ast_ગાંઠ_પદાવલિ* exprsp = nullptr) {
        // Like newFireAssert() but omits the asserts-on check
        AstDisplay* અચલ dispp
            = નવું AstDisplay{nodep->fileline(), VDisplayType::DT_ERROR, message, nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        Ast_ગાંઠ_Stmt* અચલ bodysp = dispp;
        replaceDisplay(dispp, "%%Error");  // Convert to standard DISPLAY format
        જો (exprsp) dispp->fmtp()->exprsp()->addNext(exprsp);
        જો (v3Global.opt.stopFail()) bodysp->addNext(નવું AstStop{nodep->fileline(), ખોટું});
        પાછા bodysp;
    }

    Ast_ગાંઠ_Stmt* newFireAssert(અચલ Ast_ગાંઠ_Stmt* nodep, VAssertDirectiveType directiveType,
                               VAssertType assertType, અચલ string& message,
                               Ast_ગાંઠ_પદાવલિ* exprsp = nullptr) {
        Ast_ગાંઠ_Stmt* bodysp = newFireAssertUnchecked(nodep, message, exprsp);
        bodysp = newIfAssertOn(bodysp, directiveType, assertType);
        પાછા bodysp;
    }

    AstVar* createDelayedVar(અચલ std::string& name, AstAlways* alwaysp, Ast_ગાંઠ_પદાવલિ* exprp,
                             Ast_ગાંઠ_પદાવલિ* initp) {
        File_લીટી* અચલ flp = exprp->fileline();
        AstVar* અચલ varp = નવું AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        // TODO: this lifetime seems nonsene (can't have NBAs to automatics), but is as before
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->addStmtsp(varp);
        // Before enough clock ticks, $past returns the expression's default sampled value.
        જો (initp) {
            m_modp->addStmtsp(નવું AstInitialStatic{
                flp, નવું AstAssign{flp, નવું AstVarRef{flp, varp, VAccess::WRITE}, initp}});
        }
        ++m_statPastVars;
        // Actually set the delayed value
        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = નવું AstVarRef{flp, varp, VAccess::WRITE};
        AstAssignDly* અચલ assignp = નવું AstAssignDly{flp, lhsp, exprp};
        alwaysp->prependStmtsp(assignp);
        પાછા varp;
    }

    AstAlways* getDelayedAlways(Ast_ગાંઠ_પદાવલિ* exprp, Ast_ગાંઠ_પદાવલિ* initp, Ast_Sen_વૃક્ષ* senTreep) {
        auto& expr2Sen2Alwaysr
            = initp ? m_modExpr2Sen2DefaultDelayedAlwaysp : m_modExpr2Sen2DelayedAlwaysp;
        AstAlways*& alwayspr = expr2Sen2Alwaysr[*exprp][*senTreep];
        જો (!alwayspr) {
            File_લીટી* અચલ flp = exprp->fileline();
            // Create the always block that computes the delayed values
            alwayspr = નવું AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, nullptr};
            m_modp->addStmtsp(alwayspr);
            // Create the once-delayed variable
            અચલ std::string name = "_Vpast_" + cvtToStr(m_mod_Past_અંક++) + "_1";
            AstVar* અચલ varp = createDelayedVar(name, alwayspr, exprp, initp);
            // Add it to delayed variable vector
            m_delayed(alwayspr).emplace_back(varp);
        } નહીંતર {
            // Reusing exiting, not needed
            VL_DO_DANGLING(ધકેલો_Deletep(exprp), exprp);
            જો (initp) VL_DO_DANGLING(ધકેલો_Deletep(initp), initp);
            VL_DO_DANGLING(ધકેલો_Deletep(senTreep), senTreep);
        }
        પાછા alwayspr;
    }

    Ast_ગાંઠ_પદાવલિ* get_Past_કિંમત(Ast_ગાંઠ_પદાવલિ* exprp, Ast_ગાંઠ_પદાવલિ* initp, Ast_Sen_વૃક્ષ* senTreep,
                              uint32_t ticks) {
        UASSERT_OBJ(ticks > 0, exprp, "Delay must be > 0");
        File_લીટી* અચલ flp = exprp->fileline();
        AstAlways* અચલ alwaysp
            = getDelayedAlways(exprp, initp ? initp->clone_વૃક્ષ_Pure(ખોટું) : nullptr, senTreep);
        Ast_ગાંઠ_પદાવલિ* અચલ resultp = past_કિંમત_Ref(alwaysp, flp, ticks, initp);
        જો (initp) VL_DO_DANGLING(ધકેલો_Deletep(initp), initp);
        પાછા resultp;
    }

    Ast_ગાંઠ_પદાવલિ* past_કિંમત_Ref(AstAlways* alwaysp, File_લીટી* flp, uint32_t ticks,
                              Ast_ગાંઠ_પદાવલિ* initp) {
        std::vector<AstVar*>& delayedr = m_delayed(alwaysp);
        // Ensure the required delay exists
        જ્યારે (delayedr.size() < ticks) {
            AstVar* અચલ firstp = delayedr.front();
            File_લીટી* અચલ varFlp = firstp->fileline();
            // Create once more delayed value
            std::string name = firstp->name();
            name.resize(name.size() - 1);
            name += std::to_string(delayedr.size() + 1);
            Ast_ગાંઠ_પદાવલિ* અચલ prevp = નવું AstVarRef{varFlp, delayedr.back(), VAccess::READ};
            AstVar* અચલ varp = createDelayedVar(name, alwaysp, prevp,
                                                  initp ? initp->clone_વૃક્ષ_Pure(ખોટું) : nullptr);
            // Add it to delayed variable vector
            delayedr.emplace_back(varp);
        }
        // Return a reference to the appropriately delayed variable
        પાછા નવું AstVarRef{flp, delayedr.at(ticks - 1), VAccess::READ};
    }

    // Time of the pipeline's last sampling tick, for end-of-simulation readers
    AstVar* getPastTickTimeVar(AstAlways* alwaysp) {
        AstVar*& varpr = m_delayedAlways2TickTimep[alwaysp];
        જો (!varpr) {
            File_લીટી* અચલ flp = alwaysp->fileline();
            varpr = નવું AstVar{flp, VVarType::MODULETEMP,
                               "_Vpast_" + cvtToStr(m_mod_Past_અંક++) + "_t",
                               m_modp->findUInt64DType()};
            m_modp->addStmtsp(varpr);
            Ast_ગાંઠ_પદાવલિ* અચલ timep = નવું AstCExpr{flp, "vlSymsp->_vm_contextp__->time()", 64};
            alwaysp->addStmtsp(
                નવું AstAssign{flp, નવું AstVarRef{flp, varpr, VAccess::WRITE}, timep});
        }
        પાછા varpr;
    }

    શૂન્ય visitAssertionIterate(Ast_ગાંઠ_Cover_Or_Assert* nodep, Ast_ગાંઠ* failsp) {
        જો (m_beginp && nodep->name() == "") nodep->name(m_beginp->name());

        { AssertDeFutureVisitor{nodep->propp(), m_modp, m_mod_Past_અંક++}; }

        iterateAndNextNull(nodep->sentreep());
        જો (AstAssert* અચલ assertp = VN_CAST(nodep, Assert)) {
            iterateAndNextNull(assertp->failsp());
        } નહીંતર જો (AstAssertIntrinsic* અચલ assertp = VN_CAST(nodep, AssertIntrinsic)) {
            iterateAndNextNull(assertp->failsp());
        } નહીંતર જો (AstCover* અચલ coverp = VN_CAST(nodep, Cover)) {
            iterateAndNextNull(coverp->coverincsp());
        } નહીંતર જો (!VN_IS(nodep, Restrict)) {
            nodep->v3fatalSrc("Unhandled assert type");
        }
        iterateAndNextNull(nodep->passsp());
        Ast_Sen_વૃક્ષ* sentreep = nodep->sentreep();
        જો (nodep->immediate()) {
            UASSERT_OBJ(!sentreep, nodep, "Immediate assertions don't have sensitivity");
        } નહીંતર {
            UASSERT_OBJ(sentreep, nodep, "Concurrent assertions must have sensitivity");
            // Explicit inline clock differs from the enclosing always: hoist
            // and warn. To support this need queue of asserts to activate.
            જો (m_procedurep && !nodep->senFromAlways()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Procedural concurrent assertion with"
                              " clocking event inside always (IEEE 1800-2023 16.14.6)");
                static_cast<Ast_ગાંઠ*>(m_procedurep)->addNext(nodep->unlinkFrBack());
                પાછા;  // Later iterate will pick up
            }
            sentreep->unlinkFrBack();
            જો (m_procedurep) { VL_DO_DANGLING(ધકેલો_Deletep(sentreep), sentreep); }
        }
        //
        અચલ string& message = nodep->name();
        Ast_ગાંઠ* passsp = nodep->passsp();
        જો (passsp) passsp->unlinkFrBackWithNext();
        જો (AstAssert* અચલ assertp = VN_CAST(nodep, Assert)) {
            failsp = assertp->failsp();
        } નહીંતર જો (AstAssertIntrinsic* અચલ assertp = VN_CAST(nodep, AssertIntrinsic)) {
            failsp = assertp->failsp();
        }
        જો (failsp) failsp->unlinkFrBackWithNext();

        તાર્કિક selfDestruct = ખોટું;
        તાર્કિક passspGated = ખોટું;
        અચલ AstCover* અચલ coverp = VN_CAST(nodep, Cover);
        // A sequence event control is not an assertion directive; no assertion control
        અચલ તાર્કિક seqEvent = coverp && coverp->isSeqEvent();
        જો (coverp) {
            ++m_statCover;
            જો (seqEvent) {
                // Keep the event-fire action, with no coverage bucket
            } નહીંતર જો (!v3Global.opt.coverageUser()) {
                selfDestruct = સાચું;
            } નહીંતર {
                // V3Coverage assigned us a bucket to increment.
                AstCoverInc* અચલ covincp = VN_AS(coverp->coverincsp(), CoverInc);
                UASSERT_OBJ(covincp, coverp, "Missing AstCoverInc under assertion");
                covincp->unlinkFrBackWithNext();  // next() might have  AstAssign for trace
                જો (message != "") covincp->declp()->comment(message);
                જો (passsp) {
                    passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                               /*vacuous=*/ખોટું);
                    passspGated = સાચું;
                    passsp = Ast_ગાંઠ::addNext<Ast_ગાંઠ, Ast_ગાંઠ>(covincp, passsp);
                } નહીંતર {
                    passsp = covincp;
                }
            }
        } નહીંતર જો (VN_IS(nodep, Assert) || VN_IS(nodep, AssertIntrinsic)) {
            જો (nodep->immediate()) {
                ++m_statAsImm;
            } નહીંતર {
                ++m_statAsNotImm;
            }
            જો (!passsp && !failsp)
                failsp = newFireAssertUnchecked(
                    nodep, VN_IS(nodep, AssertIntrinsic) ? "'$cast' failed." : "'assert' failed.");
        } નહીંતર {
            nodep->v3fatalSrc("Unknown node type");
        }

        VL_RESTORER(m_passsp);
        VL_RESTORER(m_failsp);
        VL_RESTORER(m_assertp);
        m_passsp = passsp;
        m_failsp = failsp;
        m_assertp = nodep;
        iterate(nodep->propp());

        Ast_ગાંઠ* propExprp;
        Ast_ગાંઠ_પદાવલિ* disablep = nullptr;
        Ast_ગાંઠ_પદાવલિ* matchCountp = nullptr;
        જો (AstPropSpec* અચલ specp = VN_CAST(nodep->propp(), PropSpec)) {
            propExprp = specp->propp()->unlinkFrBack();
            જો (specp->disablep()) disablep = specp->disablep()->unlinkFrBack();
            matchCountp = specp->matchCountp();
        } નહીંતર {
            propExprp = nodep->propp()->unlinkFrBack();
        }
        File_લીટી* અચલ flp = nodep->fileline();
        તાર્કિક passspAlreadyGated = ખોટું;
        જો (passsp && VN_IS(passsp, If)) passspAlreadyGated = VN_AS(passsp, If)->user1();
        જો (passsp && !passspGated && !passspAlreadyGated && !VN_IS(propExprp, PExpr)
            && !seqEvent) {
            passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                       /*vacuous=*/ખોટું);
        }
        જો (failsp && !VN_IS(propExprp, PExpr)) {
            failsp = newIfAssertFailOn(failsp, nodep->directive(), nodep->userType());
        }
        જો (coverp && matchCountp && passsp) {
            // Convert the match count into a loop that decrements a temporary variable until it
            // reaches zero.
            matchCountp->unlinkFrBack();
            AstVar* અચલ remainingp = નવું AstVar{
                flp, VVarType::MODULETEMP, m_match_ગણતરી_Names.get(""), matchCountp->dtypep()};
            remainingp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remainingp);
            Ast_ગાંઠ* અચલ replaysp
                = નવું AstAssign{flp, નવું AstVarRef{flp, remainingp, VAccess::WRITE}, matchCountp};
            AstLoop* અચલ loopp = નવું AstLoop{flp};
            loopp->addStmtsp(
                નવું AstLoopTest{flp, loopp, નવું AstVarRef{flp, remainingp, VAccess::READ}});
            loopp->addStmtsp(passsp);
            loopp->addStmtsp(
                નવું AstAssign{flp, નવું AstVarRef{flp, remainingp, VAccess::WRITE},
                              નવું AstSub{flp, નવું AstVarRef{flp, remainingp, VAccess::READ},
                                         નવું AstConst{flp, AstConst::Widthed_કિંમત{},
                                                      remainingp->dtypep()->width(), 1}}});
            replaysp->addNext(loopp);
            passsp = replaysp;
        }
        Ast_ગાંઠ* bodysp = assertBody(nodep, propExprp, passsp, failsp);
        જો (disablep) bodysp = નવું AstIf{flp, નવું AstLogNot{flp, disablep}, bodysp};
        // Add assertOn check last, for better combining
        જો (!seqEvent) bodysp = newIfAssertOn(bodysp, nodep->directive(), nodep->userType());
        જો (sentreep) bodysp = નવું AstAlways{flp, VAlwaysKwd::ALWAYS, sentreep, bodysp};

        જો (passsp && !passsp->backp()) VL_DO_DANGLING(ધકેલો_Deletep(passsp), passsp);
        જો (failsp && !failsp->backp()) VL_DO_DANGLING(ધકેલો_Deletep(failsp), failsp);

        // Install it
        જો (selfDestruct) {
            // Delete it after making the tree.  This way we can tell the user
            // if it wasn't constructed nicely or has other errors without needing --coverage.
            VL_DO_DANGLING(bodysp->delete_વૃક્ષ(), bodysp);
            nodep->unlinkFrBack();
        } નહીંતર {
            nodep->replaceWith(bodysp);
        }
        // Bye
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    તાર્કિક isEmptyStmt(Ast_ગાંઠ* nodep) {
        જો (!nodep) પાછા સાચું;
        જો (AstBegin* અચલ beginp = VN_CAST(nodep, Begin)) {
            જો (beginp->declsp()) પાછા ખોટું;
            માટે (Ast_ગાંઠ* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                જો (!isEmptyStmt(stmtp)) પાછા ખોટું;
            }
            પાછા સાચું;
        }
        પાછા ખોટું;
    }

    // VISITORS
    શૂન્ય visit(AstIf* nodep) override {
        જો (!nodep->user1SetOnce()) {
            જો (nodep->uniquePragma() || nodep->unique0Pragma()) {
                અચલ Ast_ગાંઠ_If* ifp = nodep;
                Ast_ગાંઠ_પદાવલિ* propp = nullptr;
                તાર્કિક hasDefaultElse = ખોટું;
                કરો {
                    // If this statement ends with 'else if', then nextIf will point to the
                    // nextIf statement.  Otherwise it will be null.
                    અચલ Ast_ગાંઠ_If* અચલ nextifp = dynamic_cast<Ast_ગાંઠ_If*>(ifp->elsesp());
                    iterateAndNextNull(ifp->condp());

                    // Recurse into the true case.
                    iterateAndNextNull(ifp->thensp());

                    // If the last else is not an else if, recurse into that too.
                    જો (ifp->elsesp() && !nextifp) {  //
                        iterateAndNextNull(ifp->elsesp());
                    }

                    // Build a bitmask of the true predicates
                    Ast_ગાંઠ_પદાવલિ* અચલ predp = ifp->condp()->clone_વૃક્ષ_Pure(ખોટું);
                    જો (propp) {
                        propp = નવું AstConcat{nodep->fileline(), predp, propp};
                    } નહીંતર {
                        propp = predp;
                    }

                    // Record if this ends with an 'else' that does not have an if
                    જો (ifp->elsesp() && !nextifp) hasDefaultElse = સાચું;

                    ifp = nextifp;
                } જ્યારે (ifp);

                અચલ તાર્કિક allow_none = nodep->unique0Pragma();

                // Empty case means no property
                જો (!propp) propp = નવું AstConst{nodep->fileline(), AstConst::BitFalse{}};

                // Note: if this ends with an 'else', then we don't need to validate that one of
                // the predicates evaluates to true.
                Ast_ગાંઠ_પદાવલિ* અચલ ohot
                    = ((allow_none || hasDefaultElse)
                           ? static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstOneHot0{nodep->fileline(), propp})
                           : static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstOneHot{nodep->fileline(), propp}));
                અચલ VAssertType assertType
                    = nodep->uniquePragma() ? VAssertType::UNIQUE : VAssertType::UNIQUE0;
                // Move, not clone, as the branches might declare referenced variables
                VNRelinker relinker;
                nodep->unlinkFrBack(&relinker);
                AstIf* અચલ checkifp
                    = નવું AstIf{nodep->fileline(), નવું AstLogNot{nodep->fileline(), ohot},
                                newFireAssert(nodep, VAssertDirectiveType::VIOLATION_IF,
                                              assertType, "'unique if' statement violated"),
                                nodep};
                checkifp->isBoundsCheck(સાચું);  // To avoid LATCH warning
                checkifp->branchPred(VBranchPred::BP_UNLIKELY);
                relinker.relink(checkifp);
                પાછા;
            }
        }

        iterateChildren(nodep);

        જો (nodep->user2()) {
            // Combine consecutive assertOn checks if possible
            જો (AstIf* અચલ backp = VN_CAST(nodep->backp(), If)) {
                જો (backp->nextp() == nodep  //
                    && backp->user2()  //
                    && backp->condp()->same_વૃક્ષ(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    backp->addThensp(nodep->thensp()->unlinkFrBackWithNext());
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    પાછા;
                }
            }
            // Combine nested assertOn checks if possible
            જો (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* અચલ checkp = VN_CAST(nodep->thensp(), If);
                જો (checkp  //
                    && checkp->user2()  //
                    && checkp->condp()->same_વૃક્ષ(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    VL_DO_DANGLING(ધકેલો_Deletep(checkp->unlinkFrBack()), checkp);
                    પાછા;
                }
            }
            પાછા;
        }

        // Swap assertOn check with single statement 'if' statement to bubble up for combining
        // Note we can't just swap the conditions as they two Ifs have different flags,
        // so swapping the Ifs themeselves then swapping back the bodies.
        જો (nodep->condp()->isPure()) {
            જો (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* અચલ checkp = VN_CAST(nodep->thensp(), If);
                જો (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    પાછા;
                }
            }
            જો (nodep->elsesp() && !nodep->elsesp()->nextp() && isEmptyStmt(nodep->thensp())) {
                AstIf* અચલ checkp = VN_CAST(nodep->elsesp(), If);
                જો (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addElsesp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    પાછા;
                }
            }
        }
    }

    //========== Case assertions
    શૂન્ય visit(AstCase* nodep) override {
        // Introduce temporary variable for AstCase if needed - it is done here and not in V3Case
        // because this phase is before V3Scope and V3Case is not. Doing it before V3Scope ensures
        // that V3Scope will take care of a scope creation
        // We also need to do it before V3Begin, co that pragmas like `unique0` also work correctly
        જો (!nodep->exprp()->isPure()) {
            ++m_statLiftedCaseExprs;
            File_લીટી* અચલ fl = nodep->exprp()->fileline();
            AstVar* અચલ varp = નવું AstVar{fl, VVarType::BLOCKTEMP, m_caseTempNames.get(nodep),
                                            nodep->exprp()->dtypep()};
            Ast_ગાંઠ_પદાવલિ* અચલ origp = nodep->exprp()->unlinkFrBack();
            nodep->addHereThisAsNext(
                નવું AstAssign{fl, નવું AstVarRef{fl, varp, VAccess::WRITE}, origp});
            nodep->exprp(નવું AstVarRef{fl, varp, VAccess::READ});
            જો (m_ftaskp) {
                varp->funcLocal(સાચું);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } નહીંતર {
                m_modp->prependStmtsp(varp);
            }
            VIsCached::clear_Cache_વૃક્ષ();
        }
        iterateChildren(nodep);
        જો (!nodep->user1SetOnce()) {
            તાર્કિક has_default = ખોટું;
            માટે (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                જો (itemp->isDefault()) has_default = સાચું;
            }
            અચલ Ast_ગાંઠ_DType* પદાવલિ_Dtypep = nodep->exprp()->dtypep()->skipRefp();

            VAssertType assertType = VAssertType::INTERNAL;
            જો (nodep->priorityPragma()) {
                assertType = VAssertType::PRIORITY;
            } નહીંતર જો (nodep->uniquePragma()) {
                assertType = VAssertType::UNIQUE;
            } નહીંતર જો (nodep->unique0Pragma()) {
                assertType = VAssertType::UNIQUE0;
            }

            string valFmt;
            જો (પદાવલિ_Dtypep->isIntegralOrPacked())
                valFmt = " for '" + cvtToStr(પદાવલિ_Dtypep->widthMin()) + "'h%X'";
            જો (nodep->fullPragma() || nodep->priorityPragma()) {
                // Need to add a default if there isn't one already
                ++m_statAsFull;
                જો (!has_default) {
                    nodep->addItemsp(નવું AstCaseItem{
                        nodep->fileline(), nullptr /*DEFAULT*/,
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      nodep->pragmaString() + ", but non-match found" + valFmt,
                                      valFmt.empty() ? nullptr
                                                     : nodep->exprp()->clone_વૃક્ષ_Pure(ખોટું))});
                }
            }
            જો (nodep->parallelPragma() || nodep->uniquePragma() || nodep->unique0Pragma()) {
                // Need to check that one, and only one of the case items match at any moment
                // If there's a default, we allow none to match, else exactly one must match
                ++m_statAsFull;
                જો (!has_default && !nodep->itemsp()) {
                    // Not parallel, but harmlessly so.
                } નહીંતર {
                    Ast_ગાંઠ_પદાવલિ* propp = nullptr;
                    માટે (AstCaseItem* itemp = nodep->itemsp(); itemp;
                         itemp = VN_AS(itemp->nextp(), CaseItem)) {
                        Ast_ગાંઠ_પદાવલિ* itembitp = nullptr;
                        માટે (Ast_ગાંઠ_પદાવલિ* icondp = itemp->condsp(); icondp;
                             icondp = VN_AS(icondp->nextp(), ગાંઠ_પદાવલિ)) {
                            Ast_ગાંઠ_પદાવલિ* onep;
                            જો (AstInsideRange* અચલ rcondp = VN_CAST(icondp, InsideRange)) {
                                onep = rcondp->newAndFromInside(
                                    nodep->exprp()->clone_વૃક્ષ_Pure(સાચું),
                                    rcondp->lhsp()->clone_વૃક્ષ_Pure(સાચું),
                                    rcondp->rhsp()->clone_વૃક્ષ_Pure(સાચું));
                            } નહીંતર જો (nodep->casex() || nodep->casez() || nodep->caseInside()) {
                                onep = AstEqWild::newTyped(itemp->fileline(),
                                                           nodep->exprp()->clone_વૃક્ષ_Pure(ખોટું),
                                                           icondp->clone_વૃક્ષ_Pure(ખોટું));
                            } નહીંતર {
                                onep = AstEq::newTyped(icondp->fileline(),
                                                       nodep->exprp()->clone_વૃક્ષ_Pure(ખોટું),
                                                       icondp->clone_વૃક્ષ_Pure(ખોટું));
                            }
                            // OR together all conditions within the same case item
                            જો (onep) {
                                જો (itembitp) {
                                    itembitp = નવું AstOr{icondp->fileline(), onep, itembitp};
                                } નહીંતર {
                                    itembitp = onep;
                                }
                            }
                        }
                        જો (itembitp) {
                            જો (propp) {
                                propp = નવું AstConcat{itemp->fileline(), itembitp, propp};
                            } નહીંતર {
                                propp = itembitp;
                            }
                        }
                    }
                    // Empty case means no property
                    જો (!propp) propp = નવું AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    અચલ તાર્કિક allow_none = has_default || nodep->unique0Pragma();
                    // The following assertion looks as below.
                    // if (!$onehot(propp)) begin
                    //     if (propp == '0) begin if (!allow_none) $error("none match"); end
                    //     else $error("multiple match");
                    // end
                    Ast_ગાંઠ_પદાવલિ* અચલ ohot = નવું AstOneHot{nodep->fileline(), propp};
                    AstConst* અચલ zero = નવું AstConst{
                        nodep->fileline(), AstConst::Widthed_કિંમત{}, propp->width(), 0};
                    AstIf* અચલ ohotIfp
                        = નવું AstIf{nodep->fileline(), નવું AstLogNot{nodep->fileline(), ohot}};
                    AstIf* અચલ zeroIfp = નવું AstIf{
                        nodep->fileline(),
                        નવું AstEq{nodep->fileline(), propp->clone_વૃક્ષ_Pure(ખોટું), zero}};
                    Ast_ગાંઠ_પદાવલિ* અચલ exprp = nodep->exprp();
                    અચલ string pragmaStr = nodep->pragmaString();
                    જો (!allow_none)
                        zeroIfp->addThensp(
                            newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                          pragmaStr + ", but none matched" + valFmt,
                                          valFmt.empty() ? nullptr : exprp->clone_વૃક્ષ_Pure(ખોટું)));
                    zeroIfp->addElsesp(
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      pragmaStr + ", but multiple matches found" + valFmt,
                                      valFmt.empty() ? nullptr : exprp->clone_વૃક્ષ_Pure(ખોટું)));
                    ohotIfp->addThensp(zeroIfp);
                    ohotIfp->isBoundsCheck(સાચું);  // To avoid LATCH warning
                    ohotIfp->branchPred(VBranchPred::BP_UNLIKELY);
                    nodep->addNotParallelp(ohotIfp);
                }
            }
        }
    }

    શૂન્ય visit(AstFuture* nodep) override {
        nodep->v3error("Future sampled value function called outside property or sequence "
                       "expression (IEEE 16.9.4)");
        nodep->replaceWith(નવું AstConst{nodep->fileline(), 0});
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    //========== Past
    શૂન્ય visit(AstPast* nodep) override {
        જો (!nodep->propertyTiming()) {
            nodep->initp(nodep->exprp()->clone_વૃક્ષ_Pure(ખોટું));
            { Default_Sampled_કિંમત_Visitor{nodep->initp()}; }
        }
        Ast_ગાંઠ_પદાવલિ* અચલ initp = nodep->initp() ? nodep->initp()->unlinkFrBack() : nullptr;
        iterateChildren(nodep);
        uint32_t ticks = 1;
        જો (nodep->ticksp()) {
            UASSERT_OBJ(VN_IS(nodep->ticksp(), Const), nodep,
                        "Expected constant ticks, checked in V3Width");
            ticks = VN_AS(nodep->ticksp(), Const)->toUInt();
        }
        UASSERT_OBJ(ticks >= 1, nodep, "0 tick should have been checked in V3Width");
        Ast_ગાંઠ_પદાવલિ* અચલ exprp = new_Sampled_પદાવલિ(nodep->exprp()->unlinkFrBack());
        Ast_Sen_વૃક્ષ* અચલ senTreep = nodep->sentreep()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* inp = nullptr;
        // Deliberately emitted for every ftask $past; calls outside final take the normal stage
        જો (VN_IS(m_procedurep, Final) || m_ftaskp) {
            // A sampling-tick finish reads one stage deeper (IEEE 1800-2023 16.9.3).
            File_લીટી* અચલ flp = nodep->fileline();
            AstAlways* અચલ alwaysp
                = getDelayedAlways(exprp, initp ? initp->clone_વૃક્ષ_Pure(ખોટું) : nullptr, senTreep);
            Ast_ગાંઠ_પદાવલિ* અચલ betweenTicksp = past_કિંમત_Ref(alwaysp, flp, ticks, initp);
            Ast_ગાંઠ_પદાવલિ* અચલ onTickp = past_કિંમત_Ref(alwaysp, flp, ticks + 1, initp);
            જો (initp) VL_DO_DANGLING(ધકેલો_Deletep(initp), initp);
            Ast_ગાંઠ_પદાવલિ* અચલ onTickCondp = નવું AstLogAnd{
                flp, નવું AstCExpr{flp, "vlSymsp->_vm_contextp__->executingFinal()", 1},
                નવું AstEq{flp, નવું AstVarRef{flp, getPastTickTimeVar(alwaysp), VAccess::READ},
                          નવું AstCExpr{flp, "vlSymsp->_vm_contextp__->finishPendingTime()", 64}}};
            AstCond* અચલ condp = નવું AstCond{flp, onTickCondp, onTickp, betweenTicksp};
            condp->dtypeFrom(onTickp);
            inp = condp;
        } નહીંતર {
            inp = get_Past_કિંમત(exprp, initp, senTreep, ticks);
        }
        nodep->replaceWith(inp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    //========== Move $sampled down to read-only variables
    શૂન્ય visit(AstSampled* nodep) override {
        જો (nodep->user1()) પાછા;
        VL_RESTORER(m_inSampled);
        {
            m_inSampled = સાચું;
            iterateChildren(nodep);
        }
        જો (nodep->exprp()) {
            nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        } નહીંતર {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        જો (m_inSampled && !nodep->user2() && !nodep->varp()->isTemp()) {
            જો (!nodep->access().isReadOnly()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Write to variable in sampled expression");
            } નહીંતર {
                VNRelinker relinkHandle;
                nodep->unlinkFrBack(&relinkHandle);
                AstSampled* અચલ newp = new_Sampled_પદાવલિ(nodep);
                relinkHandle.relink(newp);
                newp->user1(1);
                v3Global.setHasSampled();
            }
        }
    }
    // Don't sample sensitivities
    શૂન્ય visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSampled);
        m_inSampled = ખોટું;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstPExprClause* nodep) override {
        Ast_ગાંઠ* stmtsp = nullptr;
        જો (nodep->pass() && m_passsp) {
            // Cover adds COVERINC by AstNode::addNext, thus need to clone next too.
            stmtsp = m_passsp->clone_વૃક્ષ(સાચું);
            stmtsp = newIfAssertPassOn(stmtsp, m_assertp->directive(), m_assertp->userType(),
                                       nodep->vacuous());
        } નહીંતર જો (!nodep->pass() && m_failsp) {
            stmtsp = m_failsp->clone_વૃક્ષ(સાચું);
            stmtsp = newIfAssertFailOn(stmtsp, m_assertp->directive(), m_assertp->userType());
        }
        જો (stmtsp) {
            stmtsp->foreachAndNext([](Ast_ગાંઠ_Var_Ref* અચલ refp) {
                // References inside action blocks shouldn't be implicitly sampled
                // m_passsp/m_failsp have been already visited once and refs explicitly sampled
                // are handled already
                refp->user2(1);
            });
            nodep->replaceWith(stmtsp);
        } નહીંતર {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstPExpr* nodep) override {
        જો (m_inRestrict) {
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
        } નહીંતર {
            iterateChildren(nodep);
        }
    }

    //========== Statements
    શૂન્ય visit(AstDisplay* nodep) override {
        iterateChildren(nodep);
        // Replace the special types with standard text
        જો (nodep->displayType() == VDisplayType::DT_INFO) {
            replaceDisplay(nodep, "-Info");
        } નહીંતર જો (nodep->displayType() == VDisplayType::DT_WARNING) {
            replaceDisplay(nodep, "%%Warning");
        } નહીંતર જો (nodep->displayType() == VDisplayType::DT_ERROR) {
            replaceDisplay(nodep, "%%Error");
        } નહીંતર જો (nodep->displayType() == VDisplayType::DT_FATAL) {
            replaceDisplay(nodep, "%%Fatal");
        } નહીંતર જો (nodep->displayType() == VDisplayType::DT_MONITOR) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            File_લીટી* અચલ fl = nodep->fileline();

            AstSenItem* monSenItemsp = nullptr;
            જો (Ast_ગાંઠ* અચલ monExprsp = nodep->fmtp()->exprsp()) {
                monExprsp->foreachAndNext([&](AstVarRef* varrefp) {
                    AstSenItem* અચલ senItemp
                        = નવું AstSenItem{fl, VEdgeType::ET_CHANGED,
                                         // Clone so get VarRef or VarXRef as needed
                                         varrefp->clone_વૃક્ષ(ખોટું)};
                    monSenItemsp = Ast_ગાંઠ::addNextNull(monSenItemsp, senItemp);
                });
            }

            Ast_Sen_વૃક્ષ* અચલ mon_Sen_વૃક્ષ = નવું Ast_Sen_વૃક્ષ{fl, monSenItemsp};
            અચલ auto mon_અંક = ++m_monitor_અંક;
            // Where $monitor was we do "__VmonitorNum = N;"
            AstAssign* અચલ newsetp = નવું AstAssign{
                fl, new_Monitor_અંક_Var_Refp(nodep, VAccess::WRITE), નવું AstConst{fl, mon_અંક}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__VmonitorOn && __VmonitorNum==N) $display(...);"
            Ast_ગાંઠ* અચલ stmtsp = nodep;
            AstIf* અચલ ifp = નવું AstIf{
                fl,
                નવું AstLogAnd{fl, નવું AstLogNot{fl, newMonitorOffVarRefp(nodep, VAccess::READ)},
                              નવું AstEq{fl, નવું AstConst{fl, mon_અંક},
                                        new_Monitor_અંક_Var_Refp(nodep, VAccess::READ)}},
                stmtsp};
            ifp->isBoundsCheck(સાચું);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_ગાંઠ* અચલ newp = નવું AstAlways{fl, VAlwaysKwd::ALWAYS, mon_Sen_વૃક્ષ, ifp};
            m_modp->addStmtsp(newp);
        } નહીંતર જો (nodep->displayType() == VDisplayType::DT_STROBE) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            // Need one-shot
            File_લીટી* અચલ fl = nodep->fileline();
            AstVar* અચલ varp
                = નવું AstVar{fl, VVarType::MODULETEMP, "__Vstrobe" + cvtToStr(m_mod_Strobe_અંક++),
                             nodep->findBitDType()};
            m_modp->addStmtsp(varp);
            // Where $strobe was we do "__Vstrobe = '1;"
            AstAssign* અચલ newsetp = નવું AstAssign{fl, નવું AstVarRef{fl, varp, VAccess::WRITE},
                                                     નવું AstConst{fl, AstConst::BitTrue{}}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__Vstrobe) begin $display(...); __Vstrobe = '0; end"
            Ast_ગાંઠ* અચલ stmtsp = nodep;
            AstIf* અચલ ifp = નવું AstIf{fl, નવું AstVarRef{fl, varp, VAccess::READ}, stmtsp};
            ifp->isBoundsCheck(સાચું);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_ગાંઠ* અચલ newp = નવું AstAlwaysPostponed{fl, ifp};
            stmtsp->addNext(નવું AstAssign{fl, નવું AstVarRef{fl, varp, VAccess::WRITE},
                                          નવું AstConst{fl, AstConst::BitFalse{}}});
            m_modp->addStmtsp(newp);
        }
    }
    શૂન્ય visit(AstMonitorOff* nodep) override {
        AstAssign* અચલ newp
            = નવું AstAssign{nodep->fileline(), newMonitorOffVarRefp(nodep, VAccess::WRITE),
                            નવું AstConst{nodep->fileline(), AstConst::BitTrue{}, nodep->off()}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstAssert* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    શૂન્ય visit(AstAssertCtl* nodep) override {
        iterateChildren(nodep);

        તાર્કિક assertTypeConst = સાચું;
        જો (!nodep->assertTypesp()) {
            nodep->assertTypes(VAssertType{ALL_ASSERT_TYPES});
        } નહીંતર જો (અચલ AstConst* અચલ assertTypesp = VN_CAST(nodep->assertTypesp(), Const)) {
            nodep->assertTypes(VAssertType{assertTypesp->toSInt()});
        } નહીંતર {
            assertTypeConst = ખોટું;
        }

        તાર્કિક controlTypeConst = ખોટું;
        જો (અચલ AstConst* અચલ constp = VN_CAST(nodep->controlTypep(), Const)) {
            nodep->ctlType(constp->toSInt());
            controlTypeConst = સાચું;
        }
        જો (controlTypeConst
            && (nodep->ctlType() < VAssertCtlType::LOCK
                || nodep->ctlType() > VAssertCtlType::VACUOUS_OFF)) {
            nodep->unlinkFrBack();
            nodep->v3error("Bad $assertcontrol control_type '"
                           << cvtToStr(static_cast<પૂર્ણાંક>(nodep->ctlType()))
                           << "' (IEEE 1800-2023 Table 20-5)");
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        }
        જો (assertTypeConst && nodep->assertTypes() != ALL_ASSERT_TYPES
            && nodep->assertTypes().containsAny(VAssertType::UNIQUE | VAssertType::UNIQUE0
                                                | VAssertType::PRIORITY)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: assert control assertion_type");
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            પાછા;
        }

        તાર્કિક directiveTypeConst = સાચું;
        જો (!nodep->directiveTypesp()) {
            nodep->directiveTypes(VAssertDirectiveType::ASSERT | VAssertDirectiveType::ASSUME
                                  | VAssertDirectiveType::COVER);
        } નહીંતર જો (અચલ AstConst* અચલ directiveTypesp
                   = VN_CAST(nodep->directiveTypesp(), Const)) {
            nodep->directiveTypes(VAssertDirectiveType{directiveTypesp->toSInt()});
        } નહીંતર {
            directiveTypeConst = ખોટું;
        }
        જો (!directiveTypeConst) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: non-const assert directive type expression");
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            પાછા;
        }

        File_લીટી* અચલ fl = nodep->fileline();
        UINFO(9, "Generating assertctl for a module: " << m_modp);
        AstCStmt* અચલ newp = નવું AstCStmt{fl};
        newp->add("vlSymsp->_vm_contextp__->assertCtl(");
        newp->add(nodep->controlTypep()->unlinkFrBack());
        newp->add(", ");
        જો (nodep->assertTypesp()) {
            newp->add(nodep->assertTypesp()->unlinkFrBack());
        } નહીંતર {
            newp->add(std::to_string(ALL_ASSERT_TYPES));
        }
        newp->add(", " + std::to_string(nodep->directiveTypes()) + ");\n");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstAssertIntrinsic* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    શૂન્ય visit(AstCover* nodep) override {  //
        visitAssertionIterate(nodep, nullptr);
    }
    શૂન્ય visit(AstRestrict* nodep) override {
        VL_RESTORER(m_inRestrict);
        m_inRestrict = સાચું;
        iterateChildren(nodep);
        // IEEE says simulator ignores these
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Past_અંક);
        VL_RESTORER(m_mod_Strobe_અંક);
        VL_RESTORER(m_finalp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DelayedAlwaysp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DefaultDelayedAlwaysp);
        VL_RESTORER_CLEAR(m_delayedAlways2TickTimep);
        m_modp = nodep;
        m_mod_Past_અંક = 0;
        m_mod_Strobe_અંક = 0;
        m_finalp = nullptr;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstGenBlock* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstBegin* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
        // If the body is a single assertOn check, bubble it up for combining
        જો (!nodep->declsp()) {
            જો (AstIf* અચલ ifp = VN_CAST(nodep->stmtsp(), If)) {
                જો (ifp->user2() && !ifp->nextp()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(ifp->unlinkFrBack());
                    nodep->addStmtsp(ifp->thensp()->unlinkFrBackWithNext());
                    ifp->addThensp(nodep);
                }
            }
        }
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
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
        VIsCached::clear_Cache_વૃક્ષ();
    }
};

//######################################################################
// Top Assert class

શૂન્ય V3Assert::assertAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("assert", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
