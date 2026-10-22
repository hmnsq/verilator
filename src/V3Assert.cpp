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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Assert.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstUserAllocator.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ਨਾਮਾਕਾਸ਼ {

ਵਰਗ DefaultDisableLocalVisitor final : ਜਨਤਕ VNVisitor {
    // STATE
    Ast_ਗੰਢ* m_scopep = ਖਾਲੀ_ਸੂਚਕ;

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(ਖਾਲੀ_ਸੂਚਕ);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Gen_ਖੰਡ* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(ਖਾਲੀ_ਸੂਚਕ);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstDefaultDisable* nodep) override {
        UASSERT_OBJ(nodep, m_scopep,
                    "default disable iff must be inside a module or generate block");
        AstDefaultDisable* defaultp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ modp = VN_CAST(m_scopep, ਗੰਢ_Module)) {
            defaultp = modp->defaultDisablep();
        } ਨਹੀਂਤਾਂ {
            defaultp = VN_AS(m_scopep, Gen_ਖੰਡ)->defaultDisablep();
        }
        ਜੇ (VL_UNLIKELY(defaultp)) {
            nodep->v3error("Only one 'default disable iff' allowed per "
                           << (VN_IS(m_scopep, ਗੰਢ_Module) ? "module" : "generate block")
                           << " (IEEE 1800-2023 16.15)");
        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_Module* ਅਚਲ modp = VN_CAST(m_scopep, ਗੰਢ_Module)) {
            modp->defaultDisablep(nodep);
        } ਨਹੀਂਤਾਂ {
            VN_AS(m_scopep, Gen_ਖੰਡ)->defaultDisablep(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    ਸਪਸ਼ਟ DefaultDisableLocalVisitor(AstNetlist* nodep) { iterate(nodep); }
};

ਵਰਗ DefaultDisablePropagateVisitor final : ਜਨਤਕ VNVisitor {
    // STATE
    AstDefaultDisable* m_defaultDisablep = ਖਾਲੀ_ਸੂਚਕ;

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Gen_ਖੰਡ* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        ਜੇ (!nodep->defaultDisablep()) nodep->defaultDisablep(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    ਸਪਸ਼ਟ DefaultDisablePropagateVisitor(AstNetlist* nodep) { iterate(nodep); }
};

// Lower a sequence used as an event control ('@seq', IEEE 1800-2023 9.4.2.4) into a
// synthesized event fired by an internal 'cover sequence' on each end-of-match
ਵਰਗ Seq_Event_ਹੇਠਲਾ_Visitor final : ਜਨਤਕ VNVisitor {
    // STATE
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    V3_Unique_ਨਾਂ m_ਨਾਂ{"__VseqEvent"};  // Synthesized event names

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Sen_ਵਸਤੂ* nodep) override {
        AstFuncRef* ਅਚਲ funcrefp = VN_CAST(nodep->sensp(), FuncRef);
        ਜੇ (funcrefp && VN_IS(funcrefp->taskp(), Sequence)) {
            File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
            AstVar* ਅਚਲ eventp = ਨਵਾਂ AstVar{flp, VVarType::MODULETEMP, m_ਨਾਂ.get(nodep),
                                              m_modp->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::EVENT)};
            eventp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->ਜੋੜੋ_Stmtsp(eventp);
            v3Global.ਸੈੱਟ_Has_Events();
            funcrefp->unlink_Fr_ਪਿੱਠ();
            nodep->sensp(ਨਵਾਂ AstVarRef{flp, eventp, VAccess::READ});
            ਅਚਲ ਤਾਰਕਿਕ automaticActual = funcrefp->exists([](ਅਚਲ Ast_ਗੰਢ_Var_Ref* refp) {
                ਵਾਪਸ refp->varp() && refp->varp()->lifetime().isAutomatic();
            });
            ਜੇ (automaticActual) {
                nodep->v3error("Arguments to a sequence used as an event control must be"
                               " static (IEEE 1800-2023 9.4.2.4)");
                VN_AS(funcrefp->taskp(), Sequence)->isReferenced(ਝੂਠ);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(funcrefp), funcrefp);
                ਵਾਪਸ;
            }
            AstFireEvent* ਅਚਲ firep
                = ਨਵਾਂ AstFireEvent{flp, ਨਵਾਂ AstVarRef{flp, eventp, VAccess::WRITE}, ਝੂਠ};
            AstCover* ਅਚਲ coverp
                = ਨਵਾਂ AstCover{flp, ਨਵਾਂ AstPropSpec{flp, ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ, funcrefp}, firep,
                               VAssertType::CONCURRENT};
            coverp->isCoverSeq(ਸੱਚ);
            coverp->isSeqEvent(ਸੱਚ);
            m_modp->ਜੋੜੋ_Stmtsp(coverp);
            ਵਾਪਸ;
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    ਸਪਸ਼ਟ Seq_Event_ਹੇਠਲਾ_Visitor(AstNetlist* nodep) { iterate(nodep); }
};

}  // namespace

ਖਾਲੀ V3AssertCommon::collectDefaultDisable(AstNetlist* nodep) {
    { DefaultDisableLocalVisitor{nodep}; }
    { DefaultDisablePropagateVisitor{nodep}; }
}

ਖਾਲੀ V3AssertCommon::ਹੇਠਲਾ_Sequence_Events(AstNetlist* nodep) {
    { Seq_Event_ਹੇਠਲਾ_Visitor{nodep}; }
    V3Global::dump_Check_Global_ਰੁੱਖ("assertseqevent", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}

string V3AssertCommon::assertCtlGetCall(ਅਚਲ ਅੱਖਰ* query, VAssertType type,
                                        VAssertDirectiveType directiveType) {
    // Template arguments let the runtime build the type/directive mask at C++ compile time
    ਵਾਪਸ "vlSymsp->_vm_contextp__->assertCtlGet<"s + std::ਮੰਜ਼ਿਲ_string(type) + ", "s
           + std::ਮੰਜ਼ਿਲ_string(directiveType) + ">(VerilatedAssertCtlQuery::"s + query + ")"s;
}

ਤਾਰਕਿਕ V3AssertCommon::check_Sva_ਮੁੱਲ_ਹੱਦਬੰਦੀ(Ast_ਗੰਢ* nodep, ਅਚਲ AstConst* constp, ਅਚਲ ਅੱਖਰ* what) {
    // Temporal values can materialize as O(N) AST or state in later passes.
    // Leave headroom for a range ring's extra slot and V3Number's signed
    // `(bits + 31) / 32` word count.
    ਸਥਿਰ constexpr ਬਿਨਾਨਿਸ਼ਾਨੀ SVA_ਮੁੱਲ_HARD_ਹੱਦਬੰਦੀ
        = static_cast<ਬਿਨਾਨਿਸ਼ਾਨੀ>(std::numeric_limits<ਪੂਰਨ>::max()) - 32U;
    ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ configured_ਹੱਦਬੰਦੀ = static_cast<ਬਿਨਾਨਿਸ਼ਾਨੀ>(v3Global.opt.max_ਅੰਕ_ਚੌੜਾਈ());
    ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ ਹੱਦਬੰਦੀ = std::min(configured_ਹੱਦਬੰਦੀ, SVA_ਮੁੱਲ_HARD_ਹੱਦਬੰਦੀ);
    ਜੇ (constp->ਅੰਕ().ਸਮਾਉਂਦਾ_ਵਿੱਚ_UInt() && constp->ਮੰਜ਼ਿਲ_UInt() <= ਹੱਦਬੰਦੀ) ਵਾਪਸ ਸੱਚ;
    nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << what << " exceeds implementation limit of "
                                                 << ਹੱਦਬੰਦੀ
                                                 << (configured_ਹੱਦਬੰਦੀ <= SVA_ਮੁੱਲ_HARD_ਹੱਦਬੰਦੀ
                                                         ? " (--max-num-width)"
                                                         : " (host arithmetic limit)"));
    ਵਾਪਸ ਝੂਠ;
}

// The bound of a cycle delay in a sequence, if a non-negative elaboration-time constant
ਸਥਿਰ ਅਚਲ AstConst* checkSvaDelayBound(Ast_ਗੰਢ* errp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* boundp, ਅਚਲ ਅੱਖਰ* what) {
    ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(boundp, Const);
    ਜੇ (!constp || (constp->dtypep()->isSigned() && constp->ਅੰਕ().isNegative())) {
        errp->v3error(what << " is not a non-negative elaboration-time constant"
                              " (IEEE 1800-2023 16.7)");
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }
    ਜੇ (!V3AssertCommon::check_Sva_ਮੁੱਲ_ਹੱਦਬੰਦੀ(errp, constp, "SVA cycle delay")) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    ਵਾਪਸ constp;
}

// Fold the bounds of a cycle delay in a sequence, and return if they are valid, else report it
// on errp
ਸਥਿਰ ਤਾਰਕਿਕ sva_ਚੱਕਰ_Delay_Valid(ਅਚਲ AstDelay* nodep, Ast_ਗੰਢ* errp) {
    V3Const::constifyParamsNoWarnEdit(nodep->lhsp());
    ਅਚਲ AstConst* ਅਚਲ minConstp = checkSvaDelayBound(
        errp, nodep->lhsp(), nodep->isRangeDelay() ? "Range delay minimum" : "Delay value");
    ਜੇ (!nodep->rhsp() || nodep->isUnbounded()) ਵਾਪਸ minConstp != ਖਾਲੀ_ਸੂਚਕ;
    V3Const::constifyParamsNoWarnEdit(nodep->rhsp());
    ਅਚਲ AstConst* ਅਚਲ maxConstp
        = checkSvaDelayBound(errp, nodep->rhsp(), "Range delay maximum");
    ਜੇ (!minConstp || !maxConstp) ਵਾਪਸ ਝੂਠ;
    ਜੇ (maxConstp->ਮੰਜ਼ਿਲ_UInt() < minConstp->ਮੰਜ਼ਿਲ_UInt()) {
        errp->v3error("Range delay maximum must be >= minimum (IEEE 1800-2023 16.7)");
        ਵਾਪਸ ਝੂਠ;
    }
    ਵਾਪਸ ਸੱਚ;
}

ਖਾਲੀ V3AssertCommon::check_Sva_ਚੱਕਰ_Delay(AstDelay* nodep, Ast_ਗੰਢ* errp) {
    ਜੇ (sva_ਚੱਕਰ_Delay_Valid(nodep, errp)) ਵਾਪਸ;
    // Make the delay valid, so later steps do not fail after the error
    File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ minp = nodep->lhsp();
    minp->replaceWith(ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring{}});
    VL_DO_DANGLING(minp->delete_ਰੁੱਖ(), minp);
    ਜੇ (nodep->rhsp() && !nodep->isUnbounded()) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ maxp = nodep->rhsp();
        maxp->replaceWith(ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring{}});
        VL_DO_DANGLING(maxp->delete_ਰੁੱਖ(), maxp);
    }
}

ਤਾਰਕਿਕ V3AssertCommon::refsSvaFormal(ਅਚਲ Ast_ਗੰਢ* nodep) {
    ਵਾਪਸ nodep->exists([](ਅਚਲ Ast_ਗੰਢ_Var_Ref* refp) {
        ਅਚਲ AstVar* ਅਚਲ varp = refp->varp();
        ਜੇ (!varp || !varp->isIO()) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ Ast_ਗੰਢ* ਅਚਲ declp = varp->aboveLoopp();
        ਵਾਪਸ VN_IS(declp, Sequence) || VN_IS(declp, Property);
    });
}

ਖਾਲੀ V3AssertCommon::checkSvaInstanceDelays(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* instp) {
    // A delay still referencing a formal argument of an enclosing sequence or property is
    // checked when that is instanced. Collect first, as checking edits the delays.
    std::ਸਦਿਸ਼<AstDelay*> delayps;
    nodep->foreach([&](AstDelay* delayp) {
        ਜੇ (!delayp->is_ਚੱਕਰ_Delay() || refsSvaFormal(delayp->lhsp())) ਵਾਪਸ;
        ਜੇ (delayp->rhsp() && refsSvaFormal(delayp->rhsp())) ਵਾਪਸ;
        delayps.ਧੱਕੋ_ਪਿੱਠ(delayp);
    });
    ਲਈ (AstDelay* ਅਚਲ delayp : delayps) check_Sva_ਚੱਕਰ_Delay(delayp, instp);
}

//######################################################################
// AssertDeFutureVisitor
// If any AstFuture, then move all non-future varrefs to be one cycle behind,
// see IEEE 1800-2023 16.9.4.

ਵਰਗ AssertDeFutureVisitor final : ਜਨਤਕ VNVisitor {
    // STATE - across all visitors
    Ast_ਗੰਢ_Module* ਅਚਲ m_modp;  // Module future is underneath
    ਅਚਲ AstFuture* m_futurep;  // First AstFuture found
    ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ m_past_ਅੰਕ;  // Prefix unique number for this module
    std::map<AstVar*, AstVar*> m_delayedVars;  // Old to delayed variable mapping
    // STATE - for current visit position (use VL_RESTORER)
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Future = ਝੂਠ;  // Inside a future
    ਤਾਰਕਿਕ m_unsupported = ਝੂਠ;  // Printed unsupported

    // METHODS
    ਖਾਲੀ unsupported(Ast_ਗੰਢ* nodep) {
        ਜੇ (m_unsupported) ਵਾਪਸ;
        m_unsupported = ਸੱਚ;
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported/illegal: Future value function used with expression with "
                          << nodep->prettyOperatorName());
    }
    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstFuture* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Future);
        m_ਵਿੱਚ_Future = ਸੱਚ;
        iterateChildren(nodep);
        // Done with the future, this subexpression is current-time
        nodep->replaceWith(nodep->exprp()->unlink_Fr_ਪਿੱਠ());
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;
        ਜੇ (m_ਵਿੱਚ_Future || m_unsupported)
            ਵਾਪਸ;  // Need user1 set above, don't process when Future is removed
        ਜੇ (nodep->access().isWriteOrRW()) {
            unsupported(nodep);
            ਵਾਪਸ;
        }
        ਸਵੈਚਾਲਿਤ it = m_delayedVars.ਲੱਭੋ(nodep->varp());
        AstVar* outvarp;
        ਜੇ (it == m_delayedVars.ਸਿਰਾ()) {
            Ast_Sen_ਰੁੱਖ* ਅਚਲ sentreep = m_futurep->sentreep();
            AstAlways* ਅਚਲ alwaysp = ਨਵਾਂ AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS,
                                                     sentreep->clone_ਰੁੱਖ(ਝੂਠ), ਖਾਲੀ_ਸੂਚਕ};
            m_modp->ਜੋੜੋ_Stmtsp(alwaysp);
            outvarp = ਨਵਾਂ AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                 "__Vnotfuture" + cvt_ਮੰਜ਼ਿਲ_Str(m_past_ਅੰਕ) + "_" + nodep->name(),
                                 nodep->dtypep()};
            m_modp->ਜੋੜੋ_Stmtsp(outvarp);
            AstVarRef* varRefAWritep = ਨਵਾਂ AstVarRef{nodep->fileline(), outvarp, VAccess::WRITE};
            varRefAWritep->user1(ਸੱਚ);
            Ast_ਗੰਢ_Var_Ref* varRefAReadp = nodep->clone_ਰੁੱਖ(ਝੂਠ);
            varRefAReadp->user1(ਸੱਚ);
            Ast_ਗੰਢ* ਅਚਲ assp = ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{nodep->fileline(), varRefAWritep, varRefAReadp};
            alwaysp->ਜੋੜੋ_Stmtsp(assp);
            m_delayedVars.emplace(nodep->varp(), outvarp);
        } ਨਹੀਂਤਾਂ {
            outvarp = it->ਦੂਜਾ;
        }
        AstVarRef* newp = ਨਵਾਂ AstVarRef{nodep->fileline(), outvarp, VAccess::READ};
        newp->user1(ਸੱਚ);
        UINFO(9, "DeFuture " << nodep << "  becomes " << newp);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override { unsupported(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਵਿਧੀ_Call* nodep) override { unsupported(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        ਜੇ (!nodep->is_ਸ਼ੁੱਧ()) unsupported(nodep);
        iterateChildren(nodep);
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ AssertDeFutureVisitor(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ_Module* modp, ਬਿਨਾਨਿਸ਼ਾਨੀ past_ਅੰਕ)
        : m_modp{modp}
        , m_past_ਅੰਕ{past_ਅੰਕ} {
        // See if any Future before we process
        ਜੇ (nodep->forall([&](ਅਚਲ AstFuture* futurep) -> ਤਾਰਕਿਕ {
                m_futurep = futurep;
                ਵਾਪਸ ਝੂਠ;
            }))
            ਵਾਪਸ;
        // UINFOTREE(9, nodep, "", "defuture-in");
        ਦੇਖੋ(nodep);  // Nodep may get deleted
        // UINFOTREE(9, nodep, "", "defuture-ou");
    }
    ~AssertDeFutureVisitor() = ਮੂਲ;
};

// Prepare one default sampled expression.
ਵਰਗ Default_Sampled_ਮੁੱਲ_Visitor final : ਜਨਤਕ VNVisitor {
    // METHODS
    ਸਥਿਰ AstConst* newTypeDefault(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        AstConst* ਅਚਲ constp
            = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()};
        ਜੇ (nodep->dtypep()->isFourstate()) constp->ਅੰਕ().ਸੈੱਟ_All_ਬਿੱਟ_X();
        constp->dtypeFrom(nodep);
        ਵਾਪਸ constp;
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstCMethodHard* nodep) override {
        ਜੇ (nodep->ਵਿਧੀ() == VCMethod::EVENT_IS_TRIGGERED) {
            nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ {
            iterateChildren(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        AstVar* ਅਚਲ varp = nodep->varp();
        ਜੇ (varp->isParam()
            || (!varp->isNet() && varp->lifetime().isStatic() && varp->hasUserInit())) {
            ਵਾਪਸ;
        }
        nodep->replaceWith(newTypeDefault(nodep));
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    ਸਪਸ਼ਟ Default_Sampled_ਮੁੱਲ_Visitor(Ast_ਗੰਢ* nodep) { iterate(nodep); }
};

//######################################################################
// AssertVisitor

ਵਰਗ AssertVisitor final : ਜਨਤਕ VNVisitor {
    // CONSTANTS
    ਸਥਿਰ constexpr uint8_t ALL_ASSERT_TYPES
        = std::numeric_limits<std::underlying_type<VAssertType::en>::type>::max();

    // NODE STATE/TYPES
    // Cleared on netlist
    //  AstNode::user1()         -> bool.  True if processed
    //  AstAlways::user2p()      -> std::vector<AstVar*>. Delayed variables via 'm_delayed'
    //  AstNodeVarRef::user2()   -> bool.  True if shouldn't be sampled
    //  AstIf::user2()           -> bool.  True for assertOn() checks inserted in this pass
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_user1_ਵਿੱਚ_Use;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_user2_ਵਿੱਚ_Use;
    AstUser2Allocator<AstAlways, std::ਸਦਿਸ਼<AstVar*>> m_delayed;

    // STATE
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Last module
    ਅਚਲ Ast_ਗੰਢ* m_beginp = ਖਾਲੀ_ਸੂਚਕ;  // Last AstBegin/AstGenBlock
    ਬਿਨਾਨਿਸ਼ਾਨੀ m_monitor_ਅੰਕ = 0;  // Global $monitor numbering (not per module)
    AstVar* m_monitor_ਅੰਕ_Varp = ਖਾਲੀ_ਸੂਚਕ;  // $monitor number variable
    AstVar* m_monitorOffVarp = ਖਾਲੀ_ਸੂਚਕ;  // $monitoroff variable
    ਬਿਨਾਨਿਸ਼ਾਨੀ m_mod_Past_ਅੰਕ = 0;  // Module past numbering
    ਬਿਨਾਨਿਸ਼ਾਨੀ m_mod_Strobe_ਅੰਕ = 0;  // Module $strobe numbering
    Ast_ਗੰਢ_Procedure* m_procedurep = ਖਾਲੀ_ਸੂਚਕ;  // Current procedure
    VDouble0 m_statCover;  // Statistic tracking
    VDouble0 m_statAsNotImm;  // Statistic tracking
    VDouble0 m_statAsImm;  // Statistic tracking
    VDouble0 m_stat_As_ਭਰਿਆ;  // Statistic tracking
    VDouble0 m_statPastVars;  // Statistic tracking
    VDouble0 m_statAssertOnCombined;  // Statistic tracking
    VDouble0 m_statAssertOnHoisted;  // Statistic tracking
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Sampled = ਝੂਠ;  // True inside a sampled expression
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Restrict = ਝੂਠ;  // True inside restrict assertion
    Ast_ਗੰਢ* m_passsp = ਖਾਲੀ_ਸੂਚਕ;  // Current pass statement
    Ast_ਗੰਢ* m_failsp = ਖਾਲੀ_ਸੂਚਕ;  // Current fail statement
    Ast_ਗੰਢ_Cover_Or_Assert* m_assertp = ਖਾਲੀ_ਸੂਚਕ;  // Current assertion
    AstFinal* m_finalp = ਖਾਲੀ_ਸੂਚਕ;  // Current final block
    VDouble0 m_statLiftedCaseExprs;  // Count of purified case expressions
    Ast_ਗੰਢ_FTask* m_ftaskp = ਖਾਲੀ_ਸੂਚਕ;  // Current function/task
    V3_Unique_ਨਾਂ m_case_ਆਰਜ਼ੀ_ਨਾਂ{"__VCase"};
    V3_Unique_ਨਾਂ m_ਮੇਲ_ਗਿਣਤੀ_ਨਾਂ{"__VnfaRemainingMatchCount"};  // Match replay counter names
    // Maps from (expression, senTree) to the AstAlways that computes its delayed values.
    std::unordered_map<VNRef<Ast_ਗੰਢ_ਵਿਅੰਜਕ>, std::unordered_map<VNRef<Ast_Sen_ਰੁੱਖ>, AstAlways*>>
        m_modExpr2Sen2DelayedAlwaysp;
    // Source sampled-value history is initialized, unlike internal property-lowering history.
    std::unordered_map<VNRef<Ast_ਗੰਢ_ਵਿਅੰਜਕ>, std::unordered_map<VNRef<Ast_Sen_ਰੁੱਖ>, AstAlways*>>
        m_modExpr2Sen2DefaultDelayedAlwaysp;
    // Map from delayed-value AstAlways to its last-sampling-tick time variable
    std::unordered_map<ਅਚਲ AstAlways*, AstVar*> m_delayedAlways2TickTimep;

    // METHODS
    ਸਥਿਰ ਅਚਲ ਅੱਖਰ* assertPassOnQuery(ਤਾਰਕਿਕ vacuous) {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* queries[2]
            = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
        ਵਾਪਸ queries[vacuous];
    }
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* assertOnCond(File_ਸਤਰ* fl, VAssertType type,
                                     VAssertDirectiveType directiveType) {
        // cppcheck-suppress missingReturn
        ਚੋਣ (directiveType) {
        ਹਾਲਤ VAssertDirectiveType::INTRINSIC: ਵਾਪਸ ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}};
        ਹਾਲਤ VAssertDirectiveType::VIOLATION_CASE: {
            ਜੇ (v3Global.opt.assertCase()) {
                ਵਾਪਸ ਨਵਾਂ AstCExpr{fl, AstCExpr::ਸ਼ੁੱਧ{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            // If assertions are off, have constant propagation rip them out later
            // This allows syntax errors and such to be detected normally.
            ਵਾਪਸ ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
        }
        ਹਾਲਤ VAssertDirectiveType::ASSERT:
        ਹਾਲਤ VAssertDirectiveType::COVER:
        ਹਾਲਤ VAssertDirectiveType::ASSUME: {
            ਜੇ (v3Global.opt.assertOn()) {
                ਵਾਪਸ ਨਵਾਂ AstCExpr{
                    fl, AstCExpr::ਸ਼ੁੱਧ{},
                    V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
            }
            ਵਾਪਸ ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
        }
        ਹਾਲਤ VAssertDirectiveType::ਅੰਦਰੂਨੀ:
        ਹਾਲਤ VAssertDirectiveType::VIOLATION_IF:
        ਹਾਲਤ VAssertDirectiveType::RESTRICT: {
            ਜੇ (v3Global.opt.assertOn()) {
                ਵਾਪਸ ਨਵਾਂ AstCExpr{fl, AstCExpr::ਸ਼ੁੱਧ{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            ਵਾਪਸ ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
        }
        }
        VL_UNREACHABLE;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ isControlled(VAssertDirectiveType directiveType) {
        ਵਾਪਸ (static_cast<ਪੂਰਨ>(directiveType)
                & (static_cast<ਪੂਰਨ>(VAssertDirectiveType::ASSERT)
                   | static_cast<ਪੂਰਨ>(VAssertDirectiveType::COVER)
                   | static_cast<ਪੂਰਨ>(VAssertDirectiveType::ASSUME)));
    }
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* assertPassOnCond(File_ਸਤਰ* fl, VAssertType type,
                                         VAssertDirectiveType directiveType, ਤਾਰਕਿਕ vacuous) {
        ਜੇ (!isControlled(directiveType)) ਵਾਪਸ ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}};
        ਜੇ (!v3Global.opt.assertOn()) ਵਾਪਸ ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
        ਵਾਪਸ ਨਵਾਂ AstCExpr{
            fl, AstCExpr::ਸ਼ੁੱਧ{},
            V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType), 1};
    }
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* assertFailOnCond(File_ਸਤਰ* fl, VAssertType type,
                                         VAssertDirectiveType directiveType) {
        ਜੇ (!isControlled(directiveType)) ਵਾਪਸ ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}};
        ਜੇ (!v3Global.opt.assertOn()) ਵਾਪਸ ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
        ਵਾਪਸ ਨਵਾਂ AstCExpr{
            fl, AstCExpr::ਸ਼ੁੱਧ{},
            V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType), 1};
    }
    string assert_Display_ਸੁਨੇਹਾ(ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ string& ਅਗੇਤਰ, ਅਚਲ string& ਸੁਨੇਹਾ,
                                VDisplayType severity) {
        ਜੇ (severity == VDisplayType::DT_ERROR || severity == VDisplayType::DT_FATAL) {
            ਵਾਪਸ ("[%0t] "s + ਅਗੇਤਰ + ": " + nodep->fileline()->filebasename() + ":"
                    + cvt_ਮੰਜ਼ਿਲ_Str(nodep->fileline()->lineno()) + ": Assertion failed in %m"
                    + ((ਸੁਨੇਹਾ != "") ? ": " : "") + ਸੁਨੇਹਾ + "\n");
        }
        ਵਾਪਸ ("[%0t] "s + ਅਗੇਤਰ + ": " + nodep->fileline()->filebasename() + ":"
                + cvt_ਮੰਜ਼ਿਲ_Str(nodep->fileline()->lineno()) + ": %m" + ((ਸੁਨੇਹਾ != "") ? ": " : "")
                + ਸੁਨੇਹਾ + "\n");
    }
    ਖਾਲੀ replaceDisplay(AstDisplay* nodep, ਅਚਲ string& ਅਗੇਤਰ) {
        nodep->fmtp()->ਪਾਠ(
            assert_Display_ਸੁਨੇਹਾ(nodep, ਅਗੇਤਰ, nodep->fmtp()->ਪਾਠ(), nodep->displayType()));
        nodep->displayType(VDisplayType::DT_WRITE);
        // cppcheck-suppress nullPointer
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ timenewp = ਨਵਾਂ Ast_ਸਮਾਂ{nodep->fileline(), m_modp->timeunit()};
        nodep->fmtp()->prependExprsp(timenewp);
        ਜੇ (!nodep->fmtp()->scopeNamep() && nodep->fmtp()->formatScopeTracking()) {
            nodep->fmtp()->scopeNamep(ਨਵਾਂ AstScopeName{nodep->fileline(), ਸੱਚ});
        }
    }
    AstSampled* new_Sampled_ਵਿਅੰਜਕ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਵਾਪਸ ਨਵਾਂ AstSampled{nodep->fileline(), nodep, nodep->dtypep(), ਸੱਚ};
    }
    AstVarRef* new_Monitor_ਅੰਕ_Var_Refp(ਅਚਲ Ast_ਗੰਢ* nodep, VAccess access) {
        ਜੇ (!m_monitor_ਅੰਕ_Varp) {
            m_monitor_ਅੰਕ_Varp = ਨਵਾਂ AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorNum",
                                          nodep->ਲੱਭੋ_UInt64_DType()};
            v3Global.rootp()->dollarUnitPkgp()->ਜੋੜੋ_Stmtsp(m_monitor_ਅੰਕ_Varp);
        }
        AstVarRef* ਅਚਲ varrefp = ਨਵਾਂ AstVarRef{nodep->fileline(), m_monitor_ਅੰਕ_Varp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        ਵਾਪਸ varrefp;
    }
    AstVarRef* newMonitorOffVarRefp(ਅਚਲ Ast_ਗੰਢ* nodep, VAccess access) {
        ਜੇ (!m_monitorOffVarp) {
            m_monitorOffVarp = ਨਵਾਂ AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorOff",
                                          nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType()};
            v3Global.rootp()->dollarUnitPkgp()->ਜੋੜੋ_Stmtsp(m_monitorOffVarp);
        }
        AstVarRef* ਅਚਲ varrefp = ਨਵਾਂ AstVarRef{nodep->fileline(), m_monitorOffVarp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        ਵਾਪਸ varrefp;
    }
    ਸਥਿਰ AstIf* newIfAssertOn(Ast_ਗੰਢ* bodyp, VAssertDirectiveType directiveType,
                                VAssertType type = VAssertType::ਅੰਦਰੂਨੀ) {
        // Add an internal if to check assertions are on.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_ਸਤਰ* ਅਚਲ fl = bodyp->fileline();

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = assertOnCond(fl, type, directiveType);
        AstIf* ਅਚਲ newp = ਨਵਾਂ AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(ਸੱਚ);  // To avoid LATCH warning
        newp->user2(ਸੱਚ);  // Mark as an assertOn() check
        ਵਾਪਸ newp;
    }
    ਸਥਿਰ Ast_ਗੰਢ_Stmt* newIfAssertPassOn(Ast_ਗੰਢ* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type, ਤਾਰਕਿਕ vacuous) {
        // Add an internal if to check assertion passOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_ਸਤਰ* ਅਚਲ fl = bodyp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = assertPassOnCond(fl, type, directiveType, vacuous);
        Ast_ਗੰਢ_If* ਅਚਲ newp = ਨਵਾਂ AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(ਸੱਚ);  // To avoid LATCH warning
        newp->user1(ਸੱਚ);  // Don't assert/cover this if
        newp->user2(ਸੱਚ);  // Mark as an assertOn() check
        ਵਾਪਸ newp;
    }
    ਸਥਿਰ Ast_ਗੰਢ_Stmt* newIfAssertFailOn(Ast_ਗੰਢ* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type) {
        // Add an internal if to check assertion failOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_ਸਤਰ* ਅਚਲ fl = bodyp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = assertFailOnCond(fl, type, directiveType);
        Ast_ਗੰਢ_If* ਅਚਲ newp = ਨਵਾਂ AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(ਸੱਚ);  // To avoid LATCH warning
        newp->user1(ਸੱਚ);  // Don't assert/cover this if
        newp->user2(ਸੱਚ);  // Mark as an assertOn() check
        ਵਾਪਸ newp;
    }

    ਸਥਿਰ AstIf* assertCond(ਅਚਲ Ast_ਗੰਢ_Cover_Or_Assert* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* propp,
                             Ast_ਗੰਢ* passsp, Ast_ਗੰਢ* failsp) {
        AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{nodep->fileline(), propp, passsp, failsp};
        // It's more LIKELY that we'll take the nullptr if clause
        // than the sim-killing else clause:
        ifp->branchPred(VBranchPred::BP_LIKELY);
        ifp->isBoundsCheck(ਸੱਚ);  // To avoid LATCH warning
        ਵਾਪਸ ifp;
    }

    Ast_ਗੰਢ_Stmt* assertBody(ਅਚਲ Ast_ਗੰਢ_Cover_Or_Assert* nodep, Ast_ਗੰਢ* propp, Ast_ਗੰਢ* passsp,
                            Ast_ਗੰਢ* failsp) {
        ਜੇ (AstPExpr* ਅਚਲ pexprp = VN_CAST(propp, PExpr)) {
            ਜੇ (!v3Global.opt.timing().is_ਸੈੱਟ_True()) {
                nodep->v3warn(E_NOTIMING, "This property expression requires --timing");
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(pexprp), pexprp);
                ਵਾਪਸ ਨਵਾਂ AstBegin{nodep->fileline(), "", ਖਾਲੀ_ਸੂਚਕ, ਝੂਠ};
            }
            AstFork* ਅਚਲ forkp = ਨਵਾਂ AstFork{nodep->fileline(), VJoinType::JOIN_NONE};
            forkp->ਜੋੜੋ_Forksp(pexprp->bodyp()->unlink_Fr_ਪਿੱਠ());
            ਜੇ (Ast_ਗੰਢ_Stmt* ਅਚਲ finalp = pexprp->finalp()) {
                ਜੇ (!m_finalp) {
                    m_finalp = ਨਵਾਂ AstFinal{m_modp->fileline(), finalp->unlink_Fr_ਪਿੱਠ()};
                    m_modp->ਜੋੜੋ_Stmtsp(m_finalp);
                } ਨਹੀਂਤਾਂ {
                    m_finalp->ਜੋੜੋ_Stmtsp(finalp->unlink_Fr_ਪਿੱਠ());
                }
            }
            VL_DO_DANGLING2(ਧੱਕੋ_Deletep(pexprp), pexprp, propp);
            ਵਾਪਸ forkp;
        }

        ਵਾਪਸ assertCond(nodep, VN_AS(propp, ਗੰਢ_ਵਿਅੰਜਕ), passsp, failsp);
    }

    Ast_ਗੰਢ_Stmt* newFireAssertUnchecked(ਅਚਲ Ast_ਗੰਢ_Stmt* nodep, ਅਚਲ string& ਸੁਨੇਹਾ,
                                        Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprsp = ਖਾਲੀ_ਸੂਚਕ) {
        // Like newFireAssert() but omits the asserts-on check
        AstDisplay* ਅਚਲ dispp
            = ਨਵਾਂ AstDisplay{nodep->fileline(), VDisplayType::DT_ERROR, ਸੁਨੇਹਾ, ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        Ast_ਗੰਢ_Stmt* ਅਚਲ bodysp = dispp;
        replaceDisplay(dispp, "%%Error");  // Convert to standard DISPLAY format
        ਜੇ (exprsp) dispp->fmtp()->exprsp()->ਜੋੜੋ_ਅਗਲਾ(exprsp);
        ਜੇ (v3Global.opt.stopFail()) bodysp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ AstStop{nodep->fileline(), ਝੂਠ});
        ਵਾਪਸ bodysp;
    }

    Ast_ਗੰਢ_Stmt* newFireAssert(ਅਚਲ Ast_ਗੰਢ_Stmt* nodep, VAssertDirectiveType directiveType,
                               VAssertType assertType, ਅਚਲ string& ਸੁਨੇਹਾ,
                               Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprsp = ਖਾਲੀ_ਸੂਚਕ) {
        Ast_ਗੰਢ_Stmt* bodysp = newFireAssertUnchecked(nodep, ਸੁਨੇਹਾ, exprsp);
        bodysp = newIfAssertOn(bodysp, directiveType, assertType);
        ਵਾਪਸ bodysp;
    }

    AstVar* createDelayedVar(ਅਚਲ std::string& name, AstAlways* alwaysp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp,
                             Ast_ਗੰਢ_ਵਿਅੰਜਕ* initp) {
        File_ਸਤਰ* ਅਚਲ flp = exprp->fileline();
        AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        // TODO: this lifetime seems nonsene (can't have NBAs to automatics), but is as before
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->ਜੋੜੋ_Stmtsp(varp);
        // Before enough clock ticks, $past returns the expression's default sampled value.
        ਜੇ (initp) {
            m_modp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstInitialStatic{
                flp, ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, varp, VAccess::WRITE}, initp}});
        }
        ++m_statPastVars;
        // Actually set the delayed value
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = ਨਵਾਂ AstVarRef{flp, varp, VAccess::WRITE};
        Ast_ਸੌਂਪੋ_Dly* ਅਚਲ assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{flp, lhsp, exprp};
        alwaysp->prependStmtsp(assignp);
        ਵਾਪਸ varp;
    }

    AstAlways* getDelayedAlways(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* initp, Ast_Sen_ਰੁੱਖ* senTreep) {
        ਸਵੈਚਾਲਿਤ& expr2Sen2Alwaysr
            = initp ? m_modExpr2Sen2DefaultDelayedAlwaysp : m_modExpr2Sen2DelayedAlwaysp;
        AstAlways*& alwayspr = expr2Sen2Alwaysr[*exprp][*senTreep];
        ਜੇ (!alwayspr) {
            File_ਸਤਰ* ਅਚਲ flp = exprp->fileline();
            // Create the always block that computes the delayed values
            alwayspr = ਨਵਾਂ AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ਖਾਲੀ_ਸੂਚਕ};
            m_modp->ਜੋੜੋ_Stmtsp(alwayspr);
            // Create the once-delayed variable
            ਅਚਲ std::string name = "_Vpast_" + cvt_ਮੰਜ਼ਿਲ_Str(m_mod_Past_ਅੰਕ++) + "_1";
            AstVar* ਅਚਲ varp = createDelayedVar(name, alwayspr, exprp, initp);
            // Add it to delayed variable vector
            m_delayed(alwayspr).emplace_ਪਿੱਠ(varp);
        } ਨਹੀਂਤਾਂ {
            // Reusing exiting, not needed
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(exprp), exprp);
            ਜੇ (initp) VL_DO_DANGLING(ਧੱਕੋ_Deletep(initp), initp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(senTreep), senTreep);
        }
        ਵਾਪਸ alwayspr;
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* get_Past_ਮੁੱਲ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* initp, Ast_Sen_ਰੁੱਖ* senTreep,
                              uint32_t ticks) {
        UASSERT_OBJ(ticks > 0, exprp, "Delay must be > 0");
        File_ਸਤਰ* ਅਚਲ flp = exprp->fileline();
        AstAlways* ਅਚਲ alwaysp
            = getDelayedAlways(exprp, initp ? initp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ) : ਖਾਲੀ_ਸੂਚਕ, senTreep);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ resultp = past_ਮੁੱਲ_Ref(alwaysp, flp, ticks, initp);
        ਜੇ (initp) VL_DO_DANGLING(ਧੱਕੋ_Deletep(initp), initp);
        ਵਾਪਸ resultp;
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* past_ਮੁੱਲ_Ref(AstAlways* alwaysp, File_ਸਤਰ* flp, uint32_t ticks,
                              Ast_ਗੰਢ_ਵਿਅੰਜਕ* initp) {
        std::ਸਦਿਸ਼<AstVar*>& delayedr = m_delayed(alwaysp);
        // Ensure the required delay exists
        ਜਦੋਂ (delayedr.ਮਾਪ() < ticks) {
            AstVar* ਅਚਲ firstp = delayedr.front();
            File_ਸਤਰ* ਅਚਲ varFlp = firstp->fileline();
            // Create once more delayed value
            std::string name = firstp->name();
            name.resize(name.ਮਾਪ() - 1);
            name += std::ਮੰਜ਼ਿਲ_string(delayedr.ਮਾਪ() + 1);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ prevp = ਨਵਾਂ AstVarRef{varFlp, delayedr.ਪਿੱਠ(), VAccess::READ};
            AstVar* ਅਚਲ varp = createDelayedVar(name, alwaysp, prevp,
                                                  initp ? initp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ) : ਖਾਲੀ_ਸੂਚਕ);
            // Add it to delayed variable vector
            delayedr.emplace_ਪਿੱਠ(varp);
        }
        // Return a reference to the appropriately delayed variable
        ਵਾਪਸ ਨਵਾਂ AstVarRef{flp, delayedr.at(ticks - 1), VAccess::READ};
    }

    // Time of the pipeline's last sampling tick, for end-of-simulation readers
    AstVar* get_Past_Tick_ਸਮਾਂ_Var(AstAlways* alwaysp) {
        AstVar*& varpr = m_delayedAlways2TickTimep[alwaysp];
        ਜੇ (!varpr) {
            File_ਸਤਰ* ਅਚਲ flp = alwaysp->fileline();
            varpr = ਨਵਾਂ AstVar{flp, VVarType::MODULETEMP,
                               "_Vpast_" + cvt_ਮੰਜ਼ਿਲ_Str(m_mod_Past_ਅੰਕ++) + "_t",
                               m_modp->ਲੱਭੋ_UInt64_DType()};
            m_modp->ਜੋੜੋ_Stmtsp(varpr);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ timep = ਨਵਾਂ AstCExpr{flp, "vlSymsp->_vm_contextp__->time()", 64};
            alwaysp->ਜੋੜੋ_Stmtsp(
                ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, varpr, VAccess::WRITE}, timep});
        }
        ਵਾਪਸ varpr;
    }

    ਖਾਲੀ ਦੇਖੋ_Assertion_Iterate(Ast_ਗੰਢ_Cover_Or_Assert* nodep, Ast_ਗੰਢ* failsp) {
        ਜੇ (m_beginp && nodep->name() == "") nodep->name(m_beginp->name());

        { AssertDeFutureVisitor{nodep->propp(), m_modp, m_mod_Past_ਅੰਕ++}; }

        iterate_And_ਅਗਲਾ_Null(nodep->sentreep());
        ਜੇ (AstAssert* ਅਚਲ assertp = VN_CAST(nodep, Assert)) {
            iterate_And_ਅਗਲਾ_Null(assertp->failsp());
        } ਨਹੀਂਤਾਂ ਜੇ (AstAssertIntrinsic* ਅਚਲ assertp = VN_CAST(nodep, AssertIntrinsic)) {
            iterate_And_ਅਗਲਾ_Null(assertp->failsp());
        } ਨਹੀਂਤਾਂ ਜੇ (AstCover* ਅਚਲ coverp = VN_CAST(nodep, Cover)) {
            iterate_And_ਅਗਲਾ_Null(coverp->coverincsp());
        } ਨਹੀਂਤਾਂ ਜੇ (!VN_IS(nodep, Restrict)) {
            nodep->v3fatalSrc("Unhandled assert type");
        }
        iterate_And_ਅਗਲਾ_Null(nodep->passsp());
        Ast_Sen_ਰੁੱਖ* sentreep = nodep->sentreep();
        ਜੇ (nodep->ਤੁਰੰਤ()) {
            UASSERT_OBJ(!sentreep, nodep, "Immediate assertions don't have sensitivity");
        } ਨਹੀਂਤਾਂ {
            UASSERT_OBJ(sentreep, nodep, "Concurrent assertions must have sensitivity");
            // Explicit inline clock differs from the enclosing always: hoist
            // and warn. To support this need queue of asserts to activate.
            ਜੇ (m_procedurep && !nodep->senFromAlways()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Procedural concurrent assertion with"
                              " clocking event inside always (IEEE 1800-2023 16.14.6)");
                static_cast<Ast_ਗੰਢ*>(m_procedurep)->ਜੋੜੋ_ਅਗਲਾ(nodep->unlink_Fr_ਪਿੱਠ());
                ਵਾਪਸ;  // Later iterate will pick up
            }
            sentreep->unlink_Fr_ਪਿੱਠ();
            ਜੇ (m_procedurep) { VL_DO_DANGLING(ਧੱਕੋ_Deletep(sentreep), sentreep); }
        }
        //
        ਅਚਲ string& ਸੁਨੇਹਾ = nodep->name();
        Ast_ਗੰਢ* passsp = nodep->passsp();
        ਜੇ (passsp) passsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
        ਜੇ (AstAssert* ਅਚਲ assertp = VN_CAST(nodep, Assert)) {
            failsp = assertp->failsp();
        } ਨਹੀਂਤਾਂ ਜੇ (AstAssertIntrinsic* ਅਚਲ assertp = VN_CAST(nodep, AssertIntrinsic)) {
            failsp = assertp->failsp();
        }
        ਜੇ (failsp) failsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();

        ਤਾਰਕਿਕ selfDestruct = ਝੂਠ;
        ਤਾਰਕਿਕ passspGated = ਝੂਠ;
        ਅਚਲ AstCover* ਅਚਲ coverp = VN_CAST(nodep, Cover);
        // A sequence event control is not an assertion directive; no assertion control
        ਅਚਲ ਤਾਰਕਿਕ seqEvent = coverp && coverp->isSeqEvent();
        ਜੇ (coverp) {
            ++m_statCover;
            ਜੇ (seqEvent) {
                // Keep the event-fire action, with no coverage bucket
            } ਨਹੀਂਤਾਂ ਜੇ (!v3Global.opt.coverageUser()) {
                selfDestruct = ਸੱਚ;
            } ਨਹੀਂਤਾਂ {
                // V3Coverage assigned us a bucket to increment.
                AstCoverInc* ਅਚਲ covincp = VN_AS(coverp->coverincsp(), CoverInc);
                UASSERT_OBJ(covincp, coverp, "Missing AstCoverInc under assertion");
                covincp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();  // next() might have  AstAssign for trace
                ਜੇ (ਸੁਨੇਹਾ != "") covincp->declp()->comment(ਸੁਨੇਹਾ);
                ਜੇ (passsp) {
                    passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                               /*vacuous=*/ਝੂਠ);
                    passspGated = ਸੱਚ;
                    passsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ<Ast_ਗੰਢ, Ast_ਗੰਢ>(covincp, passsp);
                } ਨਹੀਂਤਾਂ {
                    passsp = covincp;
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, Assert) || VN_IS(nodep, AssertIntrinsic)) {
            ਜੇ (nodep->ਤੁਰੰਤ()) {
                ++m_statAsImm;
            } ਨਹੀਂਤਾਂ {
                ++m_statAsNotImm;
            }
            ਜੇ (!passsp && !failsp)
                failsp = newFireAssertUnchecked(
                    nodep, VN_IS(nodep, AssertIntrinsic) ? "'$cast' failed." : "'assert' failed.");
        } ਨਹੀਂਤਾਂ {
            nodep->v3fatalSrc("Unknown node type");
        }

        VL_RESTORER(m_passsp);
        VL_RESTORER(m_failsp);
        VL_RESTORER(m_assertp);
        m_passsp = passsp;
        m_failsp = failsp;
        m_assertp = nodep;
        iterate(nodep->propp());

        Ast_ਗੰਢ* propExprp;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* disablep = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਮੇਲ_Countp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (AstPropSpec* ਅਚਲ specp = VN_CAST(nodep->propp(), PropSpec)) {
            propExprp = specp->propp()->unlink_Fr_ਪਿੱਠ();
            ਜੇ (specp->disablep()) disablep = specp->disablep()->unlink_Fr_ਪਿੱਠ();
            ਮੇਲ_Countp = specp->ਮੇਲ_Countp();
        } ਨਹੀਂਤਾਂ {
            propExprp = nodep->propp()->unlink_Fr_ਪਿੱਠ();
        }
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        ਤਾਰਕਿਕ passspAlreadyGated = ਝੂਠ;
        ਜੇ (passsp && VN_IS(passsp, If)) passspAlreadyGated = VN_AS(passsp, If)->user1();
        ਜੇ (passsp && !passspGated && !passspAlreadyGated && !VN_IS(propExprp, PExpr)
            && !seqEvent) {
            passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                       /*vacuous=*/ਝੂਠ);
        }
        ਜੇ (failsp && !VN_IS(propExprp, PExpr)) {
            failsp = newIfAssertFailOn(failsp, nodep->directive(), nodep->userType());
        }
        ਜੇ (coverp && ਮੇਲ_Countp && passsp) {
            // Convert the match count into a loop that decrements a temporary variable until it
            // reaches zero.
            ਮੇਲ_Countp->unlink_Fr_ਪਿੱਠ();
            AstVar* ਅਚਲ remainingp = ਨਵਾਂ AstVar{
                flp, VVarType::MODULETEMP, m_ਮੇਲ_ਗਿਣਤੀ_ਨਾਂ.get(""), ਮੇਲ_Countp->dtypep()};
            remainingp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->ਜੋੜੋ_Stmtsp(remainingp);
            Ast_ਗੰਢ* ਅਚਲ replaysp
                = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, remainingp, VAccess::WRITE}, ਮੇਲ_Countp};
            AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{flp};
            loopp->ਜੋੜੋ_Stmtsp(
                ਨਵਾਂ AstLoopTest{flp, loopp, ਨਵਾਂ AstVarRef{flp, remainingp, VAccess::READ}});
            loopp->ਜੋੜੋ_Stmtsp(passsp);
            loopp->ਜੋੜੋ_Stmtsp(
                ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, remainingp, VAccess::WRITE},
                              ਨਵਾਂ AstSub{flp, ਨਵਾਂ AstVarRef{flp, remainingp, VAccess::READ},
                                         ਨਵਾਂ AstConst{flp, AstConst::Widthed_ਮੁੱਲ{},
                                                      remainingp->dtypep()->ਚੌੜਾਈ(), 1}}});
            replaysp->ਜੋੜੋ_ਅਗਲਾ(loopp);
            passsp = replaysp;
        }
        Ast_ਗੰਢ* bodysp = assertBody(nodep, propExprp, passsp, failsp);
        ਜੇ (disablep) bodysp = ਨਵਾਂ AstIf{flp, ਨਵਾਂ AstLogNot{flp, disablep}, bodysp};
        // Add assertOn check last, for better combining
        ਜੇ (!seqEvent) bodysp = newIfAssertOn(bodysp, nodep->directive(), nodep->userType());
        ਜੇ (sentreep) bodysp = ਨਵਾਂ AstAlways{flp, VAlwaysKwd::ALWAYS, sentreep, bodysp};

        ਜੇ (passsp && !passsp->backp()) VL_DO_DANGLING(ਧੱਕੋ_Deletep(passsp), passsp);
        ਜੇ (failsp && !failsp->backp()) VL_DO_DANGLING(ਧੱਕੋ_Deletep(failsp), failsp);

        // Install it
        ਜੇ (selfDestruct) {
            // Delete it after making the tree.  This way we can tell the user
            // if it wasn't constructed nicely or has other errors without needing --coverage.
            VL_DO_DANGLING(bodysp->delete_ਰੁੱਖ(), bodysp);
            nodep->unlink_Fr_ਪਿੱਠ();
        } ਨਹੀਂਤਾਂ {
            nodep->replaceWith(bodysp);
        }
        // Bye
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    ਤਾਰਕਿਕ is_ਖਾਲੀ_ਥਾਵਾਂ_Stmt(Ast_ਗੰਢ* nodep) {
        ਜੇ (!nodep) ਵਾਪਸ ਸੱਚ;
        ਜੇ (AstBegin* ਅਚਲ beginp = VN_CAST(nodep, Begin)) {
            ਜੇ (beginp->declsp()) ਵਾਪਸ ਝੂਠ;
            ਲਈ (Ast_ਗੰਢ* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ਜੇ (!is_ਖਾਲੀ_ਥਾਵਾਂ_Stmt(stmtp)) ਵਾਪਸ ਝੂਠ;
            }
            ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstIf* nodep) override {
        ਜੇ (!nodep->user1_ਸੈੱਟ_Once()) {
            ਜੇ (nodep->uniquePragma() || nodep->unique0Pragma()) {
                ਅਚਲ Ast_ਗੰਢ_If* ifp = nodep;
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* propp = ਖਾਲੀ_ਸੂਚਕ;
                ਤਾਰਕਿਕ hasDefaultElse = ਝੂਠ;
                ਕਰੋ {
                    // If this statement ends with 'else if', then nextIf will point to the
                    // nextIf statement.  Otherwise it will be null.
                    ਅਚਲ Ast_ਗੰਢ_If* ਅਚਲ nextifp = dynamic_cast<Ast_ਗੰਢ_If*>(ifp->elsesp());
                    iterate_And_ਅਗਲਾ_Null(ifp->condp());

                    // Recurse into the true case.
                    iterate_And_ਅਗਲਾ_Null(ifp->thensp());

                    // If the last else is not an else if, recurse into that too.
                    ਜੇ (ifp->elsesp() && !nextifp) {  //
                        iterate_And_ਅਗਲਾ_Null(ifp->elsesp());
                    }

                    // Build a bitmask of the true predicates
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ predp = ifp->condp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                    ਜੇ (propp) {
                        propp = ਨਵਾਂ AstConcat{nodep->fileline(), predp, propp};
                    } ਨਹੀਂਤਾਂ {
                        propp = predp;
                    }

                    // Record if this ends with an 'else' that does not have an if
                    ਜੇ (ifp->elsesp() && !nextifp) hasDefaultElse = ਸੱਚ;

                    ifp = nextifp;
                } ਜਦੋਂ (ifp);

                ਅਚਲ ਤਾਰਕਿਕ allow_none = nodep->unique0Pragma();

                // Empty case means no property
                ਜੇ (!propp) propp = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};

                // Note: if this ends with an 'else', then we don't need to validate that one of
                // the predicates evaluates to true.
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ohot
                    = ((allow_none || hasDefaultElse)
                           ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstOneHot0{nodep->fileline(), propp})
                           : static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstOneHot{nodep->fileline(), propp}));
                ਅਚਲ VAssertType assertType
                    = nodep->uniquePragma() ? VAssertType::UNIQUE : VAssertType::UNIQUE0;
                // Move, not clone, as the branches might declare referenced variables
                VNRelinker relinker;
                nodep->unlink_Fr_ਪਿੱਠ(&relinker);
                AstIf* ਅਚਲ checkifp
                    = ਨਵਾਂ AstIf{nodep->fileline(), ਨਵਾਂ AstLogNot{nodep->fileline(), ohot},
                                newFireAssert(nodep, VAssertDirectiveType::VIOLATION_IF,
                                              assertType, "'unique if' statement violated"),
                                nodep};
                checkifp->isBoundsCheck(ਸੱਚ);  // To avoid LATCH warning
                checkifp->branchPred(VBranchPred::BP_UNLIKELY);
                relinker.relink(checkifp);
                ਵਾਪਸ;
            }
        }

        iterateChildren(nodep);

        ਜੇ (nodep->user2()) {
            // Combine consecutive assertOn checks if possible
            ਜੇ (AstIf* ਅਚਲ backp = VN_CAST(nodep->backp(), If)) {
                ਜੇ (backp->nextp() == nodep  //
                    && backp->user2()  //
                    && backp->condp()->same_ਰੁੱਖ(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    backp->ਜੋੜੋ_Thensp(nodep->thensp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
                    nodep->unlink_Fr_ਪਿੱਠ();
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਵਾਪਸ;
                }
            }
            // Combine nested assertOn checks if possible
            ਜੇ (nodep->thensp() && !nodep->thensp()->nextp() && is_ਖਾਲੀ_ਥਾਵਾਂ_Stmt(nodep->elsesp())) {
                AstIf* ਅਚਲ checkp = VN_CAST(nodep->thensp(), If);
                ਜੇ (checkp  //
                    && checkp->user2()  //
                    && checkp->condp()->same_ਰੁੱਖ(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    nodep->ਜੋੜੋ_Thensp(checkp->thensp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(checkp->unlink_Fr_ਪਿੱਠ()), checkp);
                    ਵਾਪਸ;
                }
            }
            ਵਾਪਸ;
        }

        // Swap assertOn check with single statement 'if' statement to bubble up for combining
        // Note we can't just swap the conditions as they two Ifs have different flags,
        // so swapping the Ifs themeselves then swapping back the bodies.
        ਜੇ (nodep->condp()->is_ਸ਼ੁੱਧ()) {
            ਜੇ (nodep->thensp() && !nodep->thensp()->nextp() && is_ਖਾਲੀ_ਥਾਵਾਂ_Stmt(nodep->elsesp())) {
                AstIf* ਅਚਲ checkp = VN_CAST(nodep->thensp(), If);
                ਜੇ (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlink_Fr_ਪਿੱਠ());
                    nodep->ਜੋੜੋ_Thensp(checkp->thensp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
                    checkp->ਜੋੜੋ_Thensp(nodep);
                    ਵਾਪਸ;
                }
            }
            ਜੇ (nodep->elsesp() && !nodep->elsesp()->nextp() && is_ਖਾਲੀ_ਥਾਵਾਂ_Stmt(nodep->thensp())) {
                AstIf* ਅਚਲ checkp = VN_CAST(nodep->elsesp(), If);
                ਜੇ (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlink_Fr_ਪਿੱਠ());
                    nodep->ਜੋੜੋ_Elsesp(checkp->thensp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
                    checkp->ਜੋੜੋ_Thensp(nodep);
                    ਵਾਪਸ;
                }
            }
        }
    }

    //========== Case assertions
    ਖਾਲੀ ਦੇਖੋ(AstCase* nodep) override {
        // Introduce temporary variable for AstCase if needed - it is done here and not in V3Case
        // because this phase is before V3Scope and V3Case is not. Doing it before V3Scope ensures
        // that V3Scope will take care of a scope creation
        // We also need to do it before V3Begin, co that pragmas like `unique0` also work correctly
        ਜੇ (!nodep->exprp()->is_ਸ਼ੁੱਧ()) {
            ++m_statLiftedCaseExprs;
            File_ਸਤਰ* ਅਚਲ fl = nodep->exprp()->fileline();
            AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{fl, VVarType::BLOCKTEMP, m_case_ਆਰਜ਼ੀ_ਨਾਂ.get(nodep),
                                            nodep->exprp()->dtypep()};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ origp = nodep->exprp()->unlink_Fr_ਪਿੱਠ();
            nodep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(
                ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE}, origp});
            nodep->exprp(ਨਵਾਂ AstVarRef{fl, varp, VAccess::READ});
            ਜੇ (m_ftaskp) {
                varp->funcLocal(ਸੱਚ);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } ਨਹੀਂਤਾਂ {
                m_modp->prependStmtsp(varp);
            }
            VIsCached::clear_Cache_ਰੁੱਖ();
        }
        iterateChildren(nodep);
        ਜੇ (!nodep->user1_ਸੈੱਟ_Once()) {
            ਤਾਰਕਿਕ has_default = ਝੂਠ;
            ਲਈ (Ast_Case_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Case_ਵਸਤੂ)) {
                ਜੇ (itemp->isDefault()) has_default = ਸੱਚ;
            }
            ਅਚਲ Ast_ਗੰਢ_DType* ਵਿਅੰਜਕ_Dtypep = nodep->exprp()->dtypep()->skipRefp();

            VAssertType assertType = VAssertType::ਅੰਦਰੂਨੀ;
            ਜੇ (nodep->ਤਰਜੀਹ_Pragma()) {
                assertType = VAssertType::ਤਰਜੀਹ;
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->uniquePragma()) {
                assertType = VAssertType::UNIQUE;
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->unique0Pragma()) {
                assertType = VAssertType::UNIQUE0;
            }

            string valFmt;
            ਜੇ (ਵਿਅੰਜਕ_Dtypep->isIntegralOrPacked())
                valFmt = " for '" + cvt_ਮੰਜ਼ਿਲ_Str(ਵਿਅੰਜਕ_Dtypep->ਚੌੜਾਈ_Min()) + "'h%X'";
            ਜੇ (nodep->ਭਰਿਆ_Pragma() || nodep->ਤਰਜੀਹ_Pragma()) {
                // Need to add a default if there isn't one already
                ++m_stat_As_ਭਰਿਆ;
                ਜੇ (!has_default) {
                    nodep->ਜੋੜੋ_Itemsp(ਨਵਾਂ Ast_Case_ਵਸਤੂ{
                        nodep->fileline(), ਖਾਲੀ_ਸੂਚਕ /*DEFAULT*/,
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      nodep->pragmaString() + ", but non-match found" + valFmt,
                                      valFmt.ਖਾਲੀ_ਥਾਵਾਂ() ? ਖਾਲੀ_ਸੂਚਕ
                                                     : nodep->exprp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ))});
                }
            }
            ਜੇ (nodep->parallelPragma() || nodep->uniquePragma() || nodep->unique0Pragma()) {
                // Need to check that one, and only one of the case items match at any moment
                // If there's a default, we allow none to match, else exactly one must match
                ++m_stat_As_ਭਰਿਆ;
                ਜੇ (!has_default && !nodep->itemsp()) {
                    // Not parallel, but harmlessly so.
                } ਨਹੀਂਤਾਂ {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* propp = ਖਾਲੀ_ਸੂਚਕ;
                    ਲਈ (Ast_Case_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
                         itemp = VN_AS(itemp->nextp(), Case_ਵਸਤੂ)) {
                        Ast_ਗੰਢ_ਵਿਅੰਜਕ* itembitp = ਖਾਲੀ_ਸੂਚਕ;
                        ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* icondp = itemp->condsp(); icondp;
                             icondp = VN_AS(icondp->nextp(), ਗੰਢ_ਵਿਅੰਜਕ)) {
                            Ast_ਗੰਢ_ਵਿਅੰਜਕ* onep;
                            ਜੇ (AstInsideRange* ਅਚਲ rcondp = VN_CAST(icondp, InsideRange)) {
                                onep = rcondp->newAndFromInside(
                                    nodep->exprp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ),
                                    rcondp->lhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ),
                                    rcondp->rhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ));
                            } ਨਹੀਂਤਾਂ ਜੇ (nodep->casex() || nodep->casez() || nodep->caseInside()) {
                                onep = AstEqWild::newTyped(itemp->fileline(),
                                                           nodep->exprp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                                                           icondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
                            } ਨਹੀਂਤਾਂ {
                                onep = AstEq::newTyped(icondp->fileline(),
                                                       nodep->exprp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                                                       icondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
                            }
                            // OR together all conditions within the same case item
                            ਜੇ (onep) {
                                ਜੇ (itembitp) {
                                    itembitp = ਨਵਾਂ AstOr{icondp->fileline(), onep, itembitp};
                                } ਨਹੀਂਤਾਂ {
                                    itembitp = onep;
                                }
                            }
                        }
                        ਜੇ (itembitp) {
                            ਜੇ (propp) {
                                propp = ਨਵਾਂ AstConcat{itemp->fileline(), itembitp, propp};
                            } ਨਹੀਂਤਾਂ {
                                propp = itembitp;
                            }
                        }
                    }
                    // Empty case means no property
                    ਜੇ (!propp) propp = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
                    ਅਚਲ ਤਾਰਕਿਕ allow_none = has_default || nodep->unique0Pragma();
                    // The following assertion looks as below.
                    // if (!$onehot(propp)) begin
                    //     if (propp == '0) begin if (!allow_none) $error("none match"); end
                    //     else $error("multiple match");
                    //ਸਿਰਾ
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ohot = ਨਵਾਂ AstOneHot{nodep->fileline(), propp};
                    AstConst* ਅਚਲ zero = ਨਵਾਂ AstConst{
                        nodep->fileline(), AstConst::Widthed_ਮੁੱਲ{}, propp->ਚੌੜਾਈ(), 0};
                    AstIf* ਅਚਲ ohotIfp
                        = ਨਵਾਂ AstIf{nodep->fileline(), ਨਵਾਂ AstLogNot{nodep->fileline(), ohot}};
                    AstIf* ਅਚਲ zeroIfp = ਨਵਾਂ AstIf{
                        nodep->fileline(),
                        ਨਵਾਂ AstEq{nodep->fileline(), propp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), zero}};
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = nodep->exprp();
                    ਅਚਲ string pragmaStr = nodep->pragmaString();
                    ਜੇ (!allow_none)
                        zeroIfp->ਜੋੜੋ_Thensp(
                            newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                          pragmaStr + ", but none matched" + valFmt,
                                          valFmt.ਖਾਲੀ_ਥਾਵਾਂ() ? ਖਾਲੀ_ਸੂਚਕ : exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)));
                    zeroIfp->ਜੋੜੋ_Elsesp(
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      pragmaStr + ", but multiple matches found" + valFmt,
                                      valFmt.ਖਾਲੀ_ਥਾਵਾਂ() ? ਖਾਲੀ_ਸੂਚਕ : exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)));
                    ohotIfp->ਜੋੜੋ_Thensp(zeroIfp);
                    ohotIfp->isBoundsCheck(ਸੱਚ);  // To avoid LATCH warning
                    ohotIfp->branchPred(VBranchPred::BP_UNLIKELY);
                    nodep->ਜੋੜੋ_Not_Parallelp(ohotIfp);
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstFuture* nodep) override {
        nodep->v3error("Future sampled value function called outside property or sequence "
                       "expression (IEEE 16.9.4)");
        nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), 0});
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    //========== Past
    ਖਾਲੀ ਦੇਖੋ(AstPast* nodep) override {
        ਜੇ (!nodep->propertyTiming()) {
            nodep->initp(nodep->exprp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
            { Default_Sampled_ਮੁੱਲ_Visitor{nodep->initp()}; }
        }
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ initp = nodep->initp() ? nodep->initp()->unlink_Fr_ਪਿੱਠ() : ਖਾਲੀ_ਸੂਚਕ;
        iterateChildren(nodep);
        uint32_t ticks = 1;
        ਜੇ (nodep->ticksp()) {
            UASSERT_OBJ(VN_IS(nodep->ticksp(), Const), nodep,
                        "Expected constant ticks, checked in V3Width");
            ticks = VN_AS(nodep->ticksp(), Const)->ਮੰਜ਼ਿਲ_UInt();
        }
        UASSERT_OBJ(ticks >= 1, nodep, "0 tick should have been checked in V3Width");
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = new_Sampled_ਵਿਅੰਜਕ(nodep->exprp()->unlink_Fr_ਪਿੱਠ());
        Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep = nodep->sentreep()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* inp = ਖਾਲੀ_ਸੂਚਕ;
        // Deliberately emitted for every ftask $past; calls outside final take the normal stage
        ਜੇ (VN_IS(m_procedurep, Final) || m_ftaskp) {
            // A sampling-tick finish reads one stage deeper (IEEE 1800-2023 16.9.3).
            File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
            AstAlways* ਅਚਲ alwaysp
                = getDelayedAlways(exprp, initp ? initp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ) : ਖਾਲੀ_ਸੂਚਕ, senTreep);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ betweenTicksp = past_ਮੁੱਲ_Ref(alwaysp, flp, ticks, initp);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ onTickp = past_ਮੁੱਲ_Ref(alwaysp, flp, ticks + 1, initp);
            ਜੇ (initp) VL_DO_DANGLING(ਧੱਕੋ_Deletep(initp), initp);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ onTickCondp = ਨਵਾਂ AstLogAnd{
                flp, ਨਵਾਂ AstCExpr{flp, "vlSymsp->_vm_contextp__->executingFinal()", 1},
                ਨਵਾਂ AstEq{flp, ਨਵਾਂ AstVarRef{flp, get_Past_Tick_ਸਮਾਂ_Var(alwaysp), VAccess::READ},
                          ਨਵਾਂ AstCExpr{flp, "vlSymsp->_vm_contextp__->finishPendingTime()", 64}}};
            AstCond* ਅਚਲ condp = ਨਵਾਂ AstCond{flp, onTickCondp, onTickp, betweenTicksp};
            condp->dtypeFrom(onTickp);
            inp = condp;
        } ਨਹੀਂਤਾਂ {
            inp = get_Past_ਮੁੱਲ(exprp, initp, senTreep, ticks);
        }
        nodep->replaceWith(inp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    //========== Move $sampled down to read-only variables
    ਖਾਲੀ ਦੇਖੋ(AstSampled* nodep) override {
        ਜੇ (nodep->user1()) ਵਾਪਸ;
        VL_RESTORER(m_ਵਿੱਚ_Sampled);
        {
            m_ਵਿੱਚ_Sampled = ਸੱਚ;
            iterateChildren(nodep);
        }
        ਜੇ (nodep->exprp()) {
            nodep->replaceWith(nodep->exprp()->unlink_Fr_ਪਿੱਠ());
        } ਨਹੀਂਤਾਂ {
            nodep->unlink_Fr_ਪਿੱਠ();
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        ਜੇ (m_ਵਿੱਚ_Sampled && !nodep->user2() && !nodep->varp()->is_ਆਰਜ਼ੀ()) {
            ਜੇ (!nodep->access().isReadOnly()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Write to variable in sampled expression");
            } ਨਹੀਂਤਾਂ {
                VNRelinker relinkHandle;
                nodep->unlink_Fr_ਪਿੱਠ(&relinkHandle);
                AstSampled* ਅਚਲ newp = new_Sampled_ਵਿਅੰਜਕ(nodep);
                relinkHandle.relink(newp);
                newp->user1(1);
                v3Global.ਸੈੱਟ_Has_Sampled();
            }
        }
    }
    // Don't sample sensitivities
    ਖਾਲੀ ਦੇਖੋ(Ast_Sen_ਵਸਤੂ* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Sampled);
        m_ਵਿੱਚ_Sampled = ਝੂਠ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstPExprClause* nodep) override {
        Ast_ਗੰਢ* stmtsp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (nodep->pass() && m_passsp) {
            // Cover adds COVERINC by AstNode::addNext, thus need to clone next too.
            stmtsp = m_passsp->clone_ਰੁੱਖ(ਸੱਚ);
            stmtsp = newIfAssertPassOn(stmtsp, m_assertp->directive(), m_assertp->userType(),
                                       nodep->vacuous());
        } ਨਹੀਂਤਾਂ ਜੇ (!nodep->pass() && m_failsp) {
            stmtsp = m_failsp->clone_ਰੁੱਖ(ਸੱਚ);
            stmtsp = newIfAssertFailOn(stmtsp, m_assertp->directive(), m_assertp->userType());
        }
        ਜੇ (stmtsp) {
            stmtsp->foreach_And_ਅਗਲਾ([](Ast_ਗੰਢ_Var_Ref* ਅਚਲ refp) {
                // References inside action blocks shouldn't be implicitly sampled
                // m_passsp/m_failsp have been already visited once and refs explicitly sampled
                // are handled already
                refp->user2(1);
            });
            nodep->replaceWith(stmtsp);
        } ਨਹੀਂਤਾਂ {
            nodep->unlink_Fr_ਪਿੱਠ();
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstPExpr* nodep) override {
        ਜੇ (m_ਵਿੱਚ_Restrict) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
        } ਨਹੀਂਤਾਂ {
            iterateChildren(nodep);
        }
    }

    //========== Statements
    ਖਾਲੀ ਦੇਖੋ(AstDisplay* nodep) override {
        iterateChildren(nodep);
        // Replace the special types with standard text
        ਜੇ (nodep->displayType() == VDisplayType::DT_INFO) {
            replaceDisplay(nodep, "-Info");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->displayType() == VDisplayType::DT_WARNING) {
            replaceDisplay(nodep, "%%Warning");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->displayType() == VDisplayType::DT_ERROR) {
            replaceDisplay(nodep, "%%Error");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->displayType() == VDisplayType::DT_FATAL) {
            replaceDisplay(nodep, "%%Fatal");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->displayType() == VDisplayType::DT_MONITOR) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();

            Ast_Sen_ਵਸਤੂ* monSenItemsp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ monExprsp = nodep->fmtp()->exprsp()) {
                monExprsp->foreach_And_ਅਗਲਾ([&](AstVarRef* varrefp) {
                    Ast_Sen_ਵਸਤੂ* ਅਚਲ senItemp
                        = ਨਵਾਂ Ast_Sen_ਵਸਤੂ{fl, VEdgeType::ET_ਬਦਲਿਆ,
                                         // Clone so get VarRef or VarXRef as needed
                                         varrefp->clone_ਰੁੱਖ(ਝੂਠ)};
                    monSenItemsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(monSenItemsp, senItemp);
                });
            }

            Ast_Sen_ਰੁੱਖ* ਅਚਲ mon_Sen_ਰੁੱਖ = ਨਵਾਂ Ast_Sen_ਰੁੱਖ{fl, monSenItemsp};
            ਅਚਲ ਸਵੈਚਾਲਿਤ mon_ਅੰਕ = ++m_monitor_ਅੰਕ;
            // Where $monitor was we do "__VmonitorNum = N;"
            Ast_ਸੌਂਪੋ* ਅਚਲ newsetp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                fl, new_Monitor_ਅੰਕ_Var_Refp(nodep, VAccess::WRITE), ਨਵਾਂ AstConst{fl, mon_ਅੰਕ}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__VmonitorOn && __VmonitorNum==N) $display(...);"
            Ast_ਗੰਢ* ਅਚਲ stmtsp = nodep;
            AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{
                fl,
                ਨਵਾਂ AstLogAnd{fl, ਨਵਾਂ AstLogNot{fl, newMonitorOffVarRefp(nodep, VAccess::READ)},
                              ਨਵਾਂ AstEq{fl, ਨਵਾਂ AstConst{fl, mon_ਅੰਕ},
                                        new_Monitor_ਅੰਕ_Var_Refp(nodep, VAccess::READ)}},
                stmtsp};
            ifp->isBoundsCheck(ਸੱਚ);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ AstAlways{fl, VAlwaysKwd::ALWAYS, mon_Sen_ਰੁੱਖ, ifp};
            m_modp->ਜੋੜੋ_Stmtsp(newp);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->displayType() == VDisplayType::DT_STROBE) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            // Need one-shot
            File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
            AstVar* ਅਚਲ varp
                = ਨਵਾਂ AstVar{fl, VVarType::MODULETEMP, "__Vstrobe" + cvt_ਮੰਜ਼ਿਲ_Str(m_mod_Strobe_ਅੰਕ++),
                             nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType()};
            m_modp->ਜੋੜੋ_Stmtsp(varp);
            // Where $strobe was we do "__Vstrobe = '1;"
            Ast_ਸੌਂਪੋ* ਅਚਲ newsetp = ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE},
                                                     ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__Vstrobe) begin $display(...); __Vstrobe = '0; end"
            Ast_ਗੰਢ* ਅਚਲ stmtsp = nodep;
            AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::READ}, stmtsp};
            ifp->isBoundsCheck(ਸੱਚ);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ AstAlwaysPostponed{fl, ifp};
            stmtsp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE},
                                          ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}});
            m_modp->ਜੋੜੋ_Stmtsp(newp);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstMonitorOff* nodep) override {
        Ast_ਸੌਂਪੋ* ਅਚਲ newp
            = ਨਵਾਂ Ast_ਸੌਂਪੋ{nodep->fileline(), newMonitorOffVarRefp(nodep, VAccess::WRITE),
                            ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}, nodep->off()}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstAssert* nodep) override {  //
        ਦੇਖੋ_Assertion_Iterate(nodep, nodep->failsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstAssertCtl* nodep) override {
        iterateChildren(nodep);

        ਤਾਰਕਿਕ assertTypeConst = ਸੱਚ;
        ਜੇ (!nodep->assertTypesp()) {
            nodep->assertTypes(VAssertType{ALL_ASSERT_TYPES});
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstConst* ਅਚਲ assertTypesp = VN_CAST(nodep->assertTypesp(), Const)) {
            nodep->assertTypes(VAssertType{assertTypesp->ਮੰਜ਼ਿਲ_SInt()});
        } ਨਹੀਂਤਾਂ {
            assertTypeConst = ਝੂਠ;
        }

        ਤਾਰਕਿਕ controlTypeConst = ਝੂਠ;
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep->controlTypep(), Const)) {
            nodep->ctlType(constp->ਮੰਜ਼ਿਲ_SInt());
            controlTypeConst = ਸੱਚ;
        }
        ਜੇ (controlTypeConst
            && (nodep->ctlType() < VAssertCtlType::ਤਾਲਾ
                || nodep->ctlType() > VAssertCtlType::VACUOUS_OFF)) {
            nodep->unlink_Fr_ਪਿੱਠ();
            nodep->v3error("Bad $assertcontrol control_type '"
                           << cvt_ਮੰਜ਼ਿਲ_Str(static_cast<ਪੂਰਨ>(nodep->ctlType()))
                           << "' (IEEE 1800-2023 Table 20-5)");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        }
        ਜੇ (assertTypeConst && nodep->assertTypes() != ALL_ASSERT_TYPES
            && nodep->assertTypes().ਸ਼ਾਮਲ_ਹੈ_Any(VAssertType::UNIQUE | VAssertType::UNIQUE0
                                                | VAssertType::ਤਰਜੀਹ)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: assert control assertion_type");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        }

        ਤਾਰਕਿਕ directiveTypeConst = ਸੱਚ;
        ਜੇ (!nodep->directiveTypesp()) {
            nodep->directiveTypes(VAssertDirectiveType::ASSERT | VAssertDirectiveType::ASSUME
                                  | VAssertDirectiveType::COVER);
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstConst* ਅਚਲ directiveTypesp
                   = VN_CAST(nodep->directiveTypesp(), Const)) {
            nodep->directiveTypes(VAssertDirectiveType{directiveTypesp->ਮੰਜ਼ਿਲ_SInt()});
        } ਨਹੀਂਤਾਂ {
            directiveTypeConst = ਝੂਠ;
        }
        ਜੇ (!directiveTypeConst) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: non-const assert directive type expression");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        }

        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        UINFO(9, "Generating assertctl for a module: " << m_modp);
        AstCStmt* ਅਚਲ newp = ਨਵਾਂ AstCStmt{fl};
        newp->ਜੋੜੋ("vlSymsp->_vm_contextp__->assertCtl(");
        newp->ਜੋੜੋ(nodep->controlTypep()->unlink_Fr_ਪਿੱਠ());
        newp->ਜੋੜੋ(", ");
        ਜੇ (nodep->assertTypesp()) {
            newp->ਜੋੜੋ(nodep->assertTypesp()->unlink_Fr_ਪਿੱਠ());
        } ਨਹੀਂਤਾਂ {
            newp->ਜੋੜੋ(std::ਮੰਜ਼ਿਲ_string(ALL_ASSERT_TYPES));
        }
        newp->ਜੋੜੋ(", " + std::ਮੰਜ਼ਿਲ_string(nodep->directiveTypes()) + ");\n");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstAssertIntrinsic* nodep) override {  //
        ਦੇਖੋ_Assertion_Iterate(nodep, nodep->failsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCover* nodep) override {  //
        ਦੇਖੋ_Assertion_Iterate(nodep, ਖਾਲੀ_ਸੂਚਕ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRestrict* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Restrict);
        m_ਵਿੱਚ_Restrict = ਸੱਚ;
        iterateChildren(nodep);
        // IEEE says simulator ignores these
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Past_ਅੰਕ);
        VL_RESTORER(m_mod_Strobe_ਅੰਕ);
        VL_RESTORER(m_finalp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DelayedAlwaysp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DefaultDelayedAlwaysp);
        VL_RESTORER_CLEAR(m_delayedAlways2TickTimep);
        m_modp = nodep;
        m_mod_Past_ਅੰਕ = 0;
        m_mod_Strobe_ਅੰਕ = 0;
        m_finalp = ਖਾਲੀ_ਸੂਚਕ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Gen_ਖੰਡ* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstBegin* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
        // If the body is a single assertOn check, bubble it up for combining
        ਜੇ (!nodep->declsp()) {
            ਜੇ (AstIf* ਅਚਲ ifp = VN_CAST(nodep->stmtsp(), If)) {
                ਜੇ (ifp->user2() && !ifp->nextp()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(ifp->unlink_Fr_ਪਿੱਠ());
                    nodep->ਜੋੜੋ_Stmtsp(ifp->thensp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
                    ifp->ਜੋੜੋ_Thensp(nodep);
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ AssertVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~AssertVisitor() override {
        V3Stats::ਜੋੜੋ_Stat("Assertions, assert non-immediate statements", m_statAsNotImm);
        V3Stats::ਜੋੜੋ_Stat("Assertions, assert immediate statements", m_statAsImm);
        V3Stats::ਜੋੜੋ_Stat("Assertions, cover statements", m_statCover);
        V3Stats::ਜੋੜੋ_Stat("Assertions, full/parallel case", m_stat_As_ਭਰਿਆ);
        V3Stats::ਜੋੜੋ_Stat("Assertions, $past variables", m_statPastVars);
        V3Stats::ਜੋੜੋ_Stat("Assertions, assertOn checks combined", m_statAssertOnCombined);
        V3Stats::ਜੋੜੋ_Stat("Assertions, assertOn checks hoisted", m_statAssertOnHoisted);
        V3Stats::ਜੋੜੋ_Stat("Assertions, lifted impure case expressions", m_statLiftedCaseExprs);
        // Rewrites can change purity, e.g. by compiling out assertion statements with --no-assert
        VIsCached::clear_Cache_ਰੁੱਖ();
    }
};

//######################################################################
// Top Assert class

ਖਾਲੀ V3Assert::assertAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("assert", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
