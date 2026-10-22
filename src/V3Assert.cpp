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
    Ast_ಗಂಟು* m_scopep = ಶೂನ್ಯಸೂಚಕ;

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(ಶೂನ್ಯಸೂಚಕ);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Gen_ಖಂಡ* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(ಶೂನ್ಯಸೂಚಕ);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDefaultDisable* nodep) override {
        UASSERT_OBJ(nodep, m_scopep,
                    "default disable iff must be inside a module or generate block");
        AstDefaultDisable* defaultp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = VN_CAST(m_scopep, ಗಂಟು_Module)) {
            defaultp = modp->defaultDisablep();
        } ಇಲ್ಲದಿದ್ದರೆ {
            defaultp = VN_AS(m_scopep, Gen_ಖಂಡ)->defaultDisablep();
        }
        ಆದರೆ (VL_UNLIKELY(defaultp)) {
            nodep->v3error("Only one 'default disable iff' allowed per "
                           << (VN_IS(m_scopep, ಗಂಟು_Module) ? "module" : "generate block")
                           << " (IEEE 1800-2023 16.15)");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = VN_CAST(m_scopep, ಗಂಟು_Module)) {
            modp->defaultDisablep(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            VN_AS(m_scopep, Gen_ಖಂಡ)->defaultDisablep(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ DefaultDisableLocalVisitor(AstNetlist* nodep) { iterate(nodep); }
};

ವರ್ಗ DefaultDisablePropagateVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE
    AstDefaultDisable* m_defaultDisablep = ಶೂನ್ಯಸೂಚಕ;

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Gen_ಖಂಡ* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        ಆದರೆ (!nodep->defaultDisablep()) nodep->defaultDisablep(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ DefaultDisablePropagateVisitor(AstNetlist* nodep) { iterate(nodep); }
};

// Lower a sequence used as an event control ('@seq', IEEE 1800-2023 9.4.2.4) into a
// synthesized event fired by an internal 'cover sequence' on each end-of-match
ವರ್ಗ Seq_Event_ಅಧೋ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    V3_Unique_ಹೆಸರುಗಳು m_ಹೆಸರುಗಳು{"__VseqEvent"};  // Synthesized event names

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sen_ಅಂಶ* nodep) override {
        AstFuncRef* ಸ್ಥಿರಾಂಕ funcrefp = VN_CAST(nodep->sensp(), FuncRef);
        ಆದರೆ (funcrefp && VN_IS(funcrefp->taskp(), Sequence)) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
            AstVar* ಸ್ಥಿರಾಂಕ eventp = ಹೊಸ AstVar{flp, VVarType::MODULETEMP, m_ಹೆಸರುಗಳು.get(nodep),
                                              m_modp->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::EVENT)};
            eventp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->ಕೂಡಿಸು_Stmtsp(eventp);
            v3Global.ಗಣ_Has_Events();
            funcrefp->unlink_Fr_ಹಿಂಭಾಗ();
            nodep->sensp(ಹೊಸ AstVarRef{flp, eventp, VAccess::READ});
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ automaticActual = funcrefp->exists([](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* refp) {
                ಹಿಂತಿರುಗಿ refp->varp() && refp->varp()->lifetime().isAutomatic();
            });
            ಆದರೆ (automaticActual) {
                nodep->v3error("Arguments to a sequence used as an event control must be"
                               " static (IEEE 1800-2023 9.4.2.4)");
                VN_AS(funcrefp->taskp(), Sequence)->isReferenced(ಸುಳ್ಳು);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(funcrefp), funcrefp);
                ಹಿಂತಿರುಗಿ;
            }
            AstFireEvent* ಸ್ಥಿರಾಂಕ firep
                = ಹೊಸ AstFireEvent{flp, ಹೊಸ AstVarRef{flp, eventp, VAccess::WRITE}, ಸುಳ್ಳು};
            AstCover* ಸ್ಥಿರಾಂಕ coverp
                = ಹೊಸ AstCover{flp, ಹೊಸ AstPropSpec{flp, ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ, funcrefp}, firep,
                               VAssertType::CONCURRENT};
            coverp->isCoverSeq(ನಿಜ);
            coverp->isSeqEvent(ನಿಜ);
            m_modp->ಕೂಡಿಸು_Stmtsp(coverp);
            ಹಿಂತಿರುಗಿ;
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ Seq_Event_ಅಧೋ_Visitor(AstNetlist* nodep) { iterate(nodep); }
};

}  // namespace

ಶೂನ್ಯ V3AssertCommon::collectDefaultDisable(AstNetlist* nodep) {
    { DefaultDisableLocalVisitor{nodep}; }
    { DefaultDisablePropagateVisitor{nodep}; }
}

ಶೂನ್ಯ V3AssertCommon::ಅಧೋ_Sequence_Events(AstNetlist* nodep) {
    { Seq_Event_ಅಧೋ_Visitor{nodep}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("assertseqevent", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

string V3AssertCommon::assertCtlGetCall(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* query, VAssertType type,
                                        VAssertDirectiveType directiveType) {
    // Template arguments let the runtime build the type/directive mask at C++ compile time
    ಹಿಂತಿರುಗಿ "vlSymsp->_vm_contextp__->assertCtlGet<"s + std::ಗಮ್ಯ_string(type) + ", "s
           + std::ಗಮ್ಯ_string(directiveType) + ">(VerilatedAssertCtlQuery::"s + query + ")"s;
}

ತಾರ್ಕಿಕ V3AssertCommon::check_Sva_ಮೌಲ್ಯ_ಎಲ್ಲೆ(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ AstConst* constp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* what) {
    // Temporal values can materialize as O(N) AST or state in later passes.
    // Leave headroom for a range ring's extra slot and V3Number's signed
    // `(bits + 31) / 32` word count.
    ಸ್ಥಿರ constexpr ಚಿಹ್ನೆರಹಿತ SVA_ಮೌಲ್ಯ_HARD_ಎಲ್ಲೆ
        = static_cast<ಚಿಹ್ನೆರಹಿತ>(std::numeric_limits<ಪೂರ್ಣಾಂಕ>::max()) - 32U;
    ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ configured_ಎಲ್ಲೆ = static_cast<ಚಿಹ್ನೆರಹಿತ>(v3Global.opt.max_ಅಂಕೆ_ಅಗಲ());
    ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ ಎಲ್ಲೆ = std::min(configured_ಎಲ್ಲೆ, SVA_ಮೌಲ್ಯ_HARD_ಎಲ್ಲೆ);
    ಆದರೆ (constp->ಅಂಕೆ().ಹೊಂದುತ್ತದೆ_ಒಳ_UInt() && constp->ಗಮ್ಯ_UInt() <= ಎಲ್ಲೆ) ಹಿಂತಿರುಗಿ ನಿಜ;
    nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << what << " exceeds implementation limit of "
                                                 << ಎಲ್ಲೆ
                                                 << (configured_ಎಲ್ಲೆ <= SVA_ಮೌಲ್ಯ_HARD_ಎಲ್ಲೆ
                                                         ? " (--max-num-width)"
                                                         : " (host arithmetic limit)"));
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
}

// The bound of a cycle delay in a sequence, if a non-negative elaboration-time constant
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ AstConst* checkSvaDelayBound(Ast_ಗಂಟು* errp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* boundp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* what) {
    ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(boundp, Const);
    ಆದರೆ (!constp || (constp->dtypep()->isSigned() && constp->ಅಂಕೆ().isNegative())) {
        errp->v3error(what << " is not a non-negative elaboration-time constant"
                              " (IEEE 1800-2023 16.7)");
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }
    ಆದರೆ (!V3AssertCommon::check_Sva_ಮೌಲ್ಯ_ಎಲ್ಲೆ(errp, constp, "SVA cycle delay")) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    ಹಿಂತಿರುಗಿ constp;
}

// Fold the bounds of a cycle delay in a sequence, and return if they are valid, else report it
// on errp
ಸ್ಥಿರ ತಾರ್ಕಿಕ sva_ಚಕ್ರ_Delay_Valid(ಸ್ಥಿರಾಂಕ AstDelay* nodep, Ast_ಗಂಟು* errp) {
    V3Const::constifyParamsNoWarnEdit(nodep->lhsp());
    ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ minConstp = checkSvaDelayBound(
        errp, nodep->lhsp(), nodep->isRangeDelay() ? "Range delay minimum" : "Delay value");
    ಆದರೆ (!nodep->rhsp() || nodep->isUnbounded()) ಹಿಂತಿರುಗಿ minConstp != ಶೂನ್ಯಸೂಚಕ;
    V3Const::constifyParamsNoWarnEdit(nodep->rhsp());
    ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ maxConstp
        = checkSvaDelayBound(errp, nodep->rhsp(), "Range delay maximum");
    ಆದರೆ (!minConstp || !maxConstp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಆದರೆ (maxConstp->ಗಮ್ಯ_UInt() < minConstp->ಗಮ್ಯ_UInt()) {
        errp->v3error("Range delay maximum must be >= minimum (IEEE 1800-2023 16.7)");
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಹಿಂತಿರುಗಿ ನಿಜ;
}

ಶೂನ್ಯ V3AssertCommon::check_Sva_ಚಕ್ರ_Delay(AstDelay* nodep, Ast_ಗಂಟು* errp) {
    ಆದರೆ (sva_ಚಕ್ರ_Delay_Valid(nodep, errp)) ಹಿಂತಿರುಗಿ;
    // Make the delay valid, so later steps do not fail after the error
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ minp = nodep->lhsp();
    minp->replaceWith(ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False_Erroring{}});
    VL_DO_DANGLING(minp->delete_ವೃಕ್ಷ(), minp);
    ಆದರೆ (nodep->rhsp() && !nodep->isUnbounded()) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ maxp = nodep->rhsp();
        maxp->replaceWith(ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False_Erroring{}});
        VL_DO_DANGLING(maxp->delete_ವೃಕ್ಷ(), maxp);
    }
}

ತಾರ್ಕಿಕ V3AssertCommon::refsSvaFormal(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
    ಹಿಂತಿರುಗಿ nodep->exists([](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* refp) {
        ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = refp->varp();
        ಆದರೆ (!varp || !varp->isIO()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ declp = varp->aboveLoopp();
        ಹಿಂತಿರುಗಿ VN_IS(declp, Sequence) || VN_IS(declp, Property);
    });
}

ಶೂನ್ಯ V3AssertCommon::checkSvaInstanceDelays(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* instp) {
    // A delay still referencing a formal argument of an enclosing sequence or property is
    // checked when that is instanced. Collect first, as checking edits the delays.
    std::ಸದಿಶ<AstDelay*> delayps;
    nodep->foreach([&](AstDelay* delayp) {
        ಆದರೆ (!delayp->is_ಚಕ್ರ_Delay() || refsSvaFormal(delayp->lhsp())) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (delayp->rhsp() && refsSvaFormal(delayp->rhsp())) ಹಿಂತಿರುಗಿ;
        delayps.ತಳ್ಳು_ಹಿಂಭಾಗ(delayp);
    });
    ಗಾಗಿ (AstDelay* ಸ್ಥಿರಾಂಕ delayp : delayps) check_Sva_ಚಕ್ರ_Delay(delayp, instp);
}

//######################################################################
// AssertDeFutureVisitor
// If any AstFuture, then move all non-future varrefs to be one cycle behind,
// see IEEE 1800-2023 16.9.4.

ವರ್ಗ AssertDeFutureVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE - across all visitors
    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ m_modp;  // Module future is underneath
    ಸ್ಥಿರಾಂಕ AstFuture* m_futurep;  // First AstFuture found
    ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ m_past_ಅಂಕೆ;  // Prefix unique number for this module
    std::map<AstVar*, AstVar*> m_delayedVars;  // Old to delayed variable mapping
    // STATE - for current visit position (use VL_RESTORER)
    ತಾರ್ಕಿಕ m_ಒಳ_Future = ಸುಳ್ಳು;  // Inside a future
    ತಾರ್ಕಿಕ m_unsupported = ಸುಳ್ಳು;  // Printed unsupported

    // METHODS
    ಶೂನ್ಯ unsupported(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (m_unsupported) ಹಿಂತಿರುಗಿ;
        m_unsupported = ನಿಜ;
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported/illegal: Future value function used with expression with "
                          << nodep->prettyOperatorName());
    }
    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstFuture* nodep) override {
        VL_RESTORER(m_ಒಳ_Future);
        m_ಒಳ_Future = ನಿಜ;
        iterateChildren(nodep);
        // Done with the future, this subexpression is current-time
        nodep->replaceWith(nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ());
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (m_ಒಳ_Future || m_unsupported)
            ಹಿಂತಿರುಗಿ;  // Need user1 set above, don't process when Future is removed
        ಆದರೆ (nodep->access().isWriteOrRW()) {
            unsupported(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಸ್ವಯಂಚಾಲಿತ it = m_delayedVars.ಕಂಡುಹಿಡಿ(nodep->varp());
        AstVar* outvarp;
        ಆದರೆ (it == m_delayedVars.ಕೊನೆ()) {
            Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ sentreep = m_futurep->sentreep();
            AstAlways* ಸ್ಥಿರಾಂಕ alwaysp = ಹೊಸ AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS,
                                                     sentreep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), ಶೂನ್ಯಸೂಚಕ};
            m_modp->ಕೂಡಿಸು_Stmtsp(alwaysp);
            outvarp = ಹೊಸ AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                 "__Vnotfuture" + cvt_ಗಮ್ಯ_Str(m_past_ಅಂಕೆ) + "_" + nodep->name(),
                                 nodep->dtypep()};
            m_modp->ಕೂಡಿಸು_Stmtsp(outvarp);
            AstVarRef* varRefAWritep = ಹೊಸ AstVarRef{nodep->fileline(), outvarp, VAccess::WRITE};
            varRefAWritep->user1(ನಿಜ);
            Ast_ಗಂಟು_Var_Ref* varRefAReadp = nodep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            varRefAReadp->user1(ನಿಜ);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ assp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{nodep->fileline(), varRefAWritep, varRefAReadp};
            alwaysp->ಕೂಡಿಸು_Stmtsp(assp);
            m_delayedVars.emplace(nodep->varp(), outvarp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            outvarp = it->ಎರಡನೇ;
        }
        AstVarRef* newp = ಹೊಸ AstVarRef{nodep->fileline(), outvarp, VAccess::READ};
        newp->user1(ನಿಜ);
        UINFO(9, "DeFuture " << nodep << "  becomes " << newp);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override { unsupported(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ವಿಧಾನ_Call* nodep) override { unsupported(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        ಆದರೆ (!nodep->is_ಶುದ್ಧ()) unsupported(nodep);
        iterateChildren(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ AssertDeFutureVisitor(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_Module* modp, ಚಿಹ್ನೆರಹಿತ past_ಅಂಕೆ)
        : m_modp{modp}
        , m_past_ಅಂಕೆ{past_ಅಂಕೆ} {
        // See if any Future before we process
        ಆದರೆ (nodep->forall([&](ಸ್ಥಿರಾಂಕ AstFuture* futurep) -> ತಾರ್ಕಿಕ {
                m_futurep = futurep;
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }))
            ಹಿಂತಿರುಗಿ;
        // UINFOTREE(9, nodep, "", "defuture-in");
        ಭೇಟಿ(nodep);  // Nodep may get deleted
        // UINFOTREE(9, nodep, "", "defuture-ou");
    }
    ~AssertDeFutureVisitor() = ಪೂರ್ವನಿಯೋಜಿತ;
};

// Prepare one default sampled expression.
ವರ್ಗ Default_Sampled_ಮೌಲ್ಯ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // METHODS
    ಸ್ಥಿರ AstConst* newTypeDefault(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        AstConst* ಸ್ಥಿರಾಂಕ constp
            = ಹೊಸ AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()};
        ಆದರೆ (nodep->dtypep()->isFourstate()) constp->ಅಂಕೆ().ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
        constp->dtypeFrom(nodep);
        ಹಿಂತಿರುಗಿ constp;
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstCMethodHard* nodep) override {
        ಆದರೆ (nodep->ವಿಧಾನ() == VCMethod::EVENT_IS_TRIGGERED) {
            nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
        ಆದರೆ (varp->isParam()
            || (!varp->isNet() && varp->lifetime().isStatic() && varp->hasUserInit())) {
            ಹಿಂತಿರುಗಿ;
        }
        nodep->replaceWith(newTypeDefault(nodep));
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ Default_Sampled_ಮೌಲ್ಯ_Visitor(Ast_ಗಂಟು* nodep) { iterate(nodep); }
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
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_user1_ಒಳ_Use;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_user2_ಒಳ_Use;
    AstUser2Allocator<AstAlways, std::ಸದಿಶ<AstVar*>> m_delayed;

    // STATE
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Last module
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* m_beginp = ಶೂನ್ಯಸೂಚಕ;  // Last AstBegin/AstGenBlock
    ಚಿಹ್ನೆರಹಿತ m_monitor_ಅಂಕೆ = 0;  // Global $monitor numbering (not per module)
    AstVar* m_monitor_ಅಂಕೆ_Varp = ಶೂನ್ಯಸೂಚಕ;  // $monitor number variable
    AstVar* m_monitorOffVarp = ಶೂನ್ಯಸೂಚಕ;  // $monitoroff variable
    ಚಿಹ್ನೆರಹಿತ m_mod_Past_ಅಂಕೆ = 0;  // Module past numbering
    ಚಿಹ್ನೆರಹಿತ m_mod_Strobe_ಅಂಕೆ = 0;  // Module $strobe numbering
    Ast_ಗಂಟು_Procedure* m_procedurep = ಶೂನ್ಯಸೂಚಕ;  // Current procedure
    VDouble0 m_statCover;  // Statistic tracking
    VDouble0 m_statAsNotImm;  // Statistic tracking
    VDouble0 m_statAsImm;  // Statistic tracking
    VDouble0 m_stat_As_ತುಂಬಿದ;  // Statistic tracking
    VDouble0 m_statPastVars;  // Statistic tracking
    VDouble0 m_statAssertOnCombined;  // Statistic tracking
    VDouble0 m_statAssertOnHoisted;  // Statistic tracking
    ತಾರ್ಕಿಕ m_ಒಳ_Sampled = ಸುಳ್ಳು;  // True inside a sampled expression
    ತಾರ್ಕಿಕ m_ಒಳ_Restrict = ಸುಳ್ಳು;  // True inside restrict assertion
    Ast_ಗಂಟು* m_passsp = ಶೂನ್ಯಸೂಚಕ;  // Current pass statement
    Ast_ಗಂಟು* m_failsp = ಶೂನ್ಯಸೂಚಕ;  // Current fail statement
    Ast_ಗಂಟು_Cover_Or_Assert* m_assertp = ಶೂನ್ಯಸೂಚಕ;  // Current assertion
    AstFinal* m_finalp = ಶೂನ್ಯಸೂಚಕ;  // Current final block
    VDouble0 m_statLiftedCaseExprs;  // Count of purified case expressions
    Ast_ಗಂಟು_FTask* m_ftaskp = ಶೂನ್ಯಸೂಚಕ;  // Current function/task
    V3_Unique_ಹೆಸರುಗಳು m_case_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು{"__VCase"};
    V3_Unique_ಹೆಸರುಗಳು m_ಹೊಂದಿಕೆ_ಎಣಿಕೆ_ಹೆಸರುಗಳು{"__VnfaRemainingMatchCount"};  // Match replay counter names
    // Maps from (expression, senTree) to the AstAlways that computes its delayed values.
    std::unordered_map<VNRef<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ>, std::unordered_map<VNRef<Ast_Sen_ವೃಕ್ಷ>, AstAlways*>>
        m_modExpr2Sen2DelayedAlwaysp;
    // Source sampled-value history is initialized, unlike internal property-lowering history.
    std::unordered_map<VNRef<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ>, std::unordered_map<VNRef<Ast_Sen_ವೃಕ್ಷ>, AstAlways*>>
        m_modExpr2Sen2DefaultDelayedAlwaysp;
    // Map from delayed-value AstAlways to its last-sampling-tick time variable
    std::unordered_map<ಸ್ಥಿರಾಂಕ AstAlways*, AstVar*> m_delayedAlways2TickTimep;

    // METHODS
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* assertPassOnQuery(ತಾರ್ಕಿಕ vacuous) {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* queries[2]
            = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
        ಹಿಂತಿರುಗಿ queries[vacuous];
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* assertOnCond(File_ಸಾಲು* fl, VAssertType type,
                                     VAssertDirectiveType directiveType) {
        // cppcheck-suppress missingReturn
        ಆಯ್ಕೆ (directiveType) {
        ಸ್ಥಿತಿ VAssertDirectiveType::INTRINSIC: ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}};
        ಸ್ಥಿತಿ VAssertDirectiveType::VIOLATION_CASE: {
            ಆದರೆ (v3Global.opt.assertCase()) {
                ಹಿಂತಿರುಗಿ ಹೊಸ AstCExpr{fl, AstCExpr::ಶುದ್ಧ{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            // If assertions are off, have constant propagation rip them out later
            // This allows syntax errors and such to be detected normally.
            ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_False{}};
        }
        ಸ್ಥಿತಿ VAssertDirectiveType::ASSERT:
        ಸ್ಥಿತಿ VAssertDirectiveType::COVER:
        ಸ್ಥಿತಿ VAssertDirectiveType::ASSUME: {
            ಆದರೆ (v3Global.opt.assertOn()) {
                ಹಿಂತಿರುಗಿ ಹೊಸ AstCExpr{
                    fl, AstCExpr::ಶುದ್ಧ{},
                    V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
            }
            ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_False{}};
        }
        ಸ್ಥಿತಿ VAssertDirectiveType::ಆಂತರಿಕ:
        ಸ್ಥಿತಿ VAssertDirectiveType::VIOLATION_IF:
        ಸ್ಥಿತಿ VAssertDirectiveType::RESTRICT: {
            ಆದರೆ (v3Global.opt.assertOn()) {
                ಹಿಂತಿರುಗಿ ಹೊಸ AstCExpr{fl, AstCExpr::ಶುದ್ಧ{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_False{}};
        }
        }
        VL_UNREACHABLE;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isControlled(VAssertDirectiveType directiveType) {
        ಹಿಂತಿರುಗಿ (static_cast<ಪೂರ್ಣಾಂಕ>(directiveType)
                & (static_cast<ಪೂರ್ಣಾಂಕ>(VAssertDirectiveType::ASSERT)
                   | static_cast<ಪೂರ್ಣಾಂಕ>(VAssertDirectiveType::COVER)
                   | static_cast<ಪೂರ್ಣಾಂಕ>(VAssertDirectiveType::ASSUME)));
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* assertPassOnCond(File_ಸಾಲು* fl, VAssertType type,
                                         VAssertDirectiveType directiveType, ತಾರ್ಕಿಕ vacuous) {
        ಆದರೆ (!isControlled(directiveType)) ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}};
        ಆದರೆ (!v3Global.opt.assertOn()) ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_False{}};
        ಹಿಂತಿರುಗಿ ಹೊಸ AstCExpr{
            fl, AstCExpr::ಶುದ್ಧ{},
            V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType), 1};
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* assertFailOnCond(File_ಸಾಲು* fl, VAssertType type,
                                         VAssertDirectiveType directiveType) {
        ಆದರೆ (!isControlled(directiveType)) ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}};
        ಆದರೆ (!v3Global.opt.assertOn()) ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_False{}};
        ಹಿಂತಿರುಗಿ ಹೊಸ AstCExpr{
            fl, AstCExpr::ಶುದ್ಧ{},
            V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType), 1};
    }
    string assert_Display_ಸಂದೇಶ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& ಪೂರ್ವಪ್ರತ್ಯಯ, ಸ್ಥಿರಾಂಕ string& ಸಂದೇಶ,
                                VDisplayType severity) {
        ಆದರೆ (severity == VDisplayType::DT_ERROR || severity == VDisplayType::DT_FATAL) {
            ಹಿಂತಿರುಗಿ ("[%0t] "s + ಪೂರ್ವಪ್ರತ್ಯಯ + ": " + nodep->fileline()->filebasename() + ":"
                    + cvt_ಗಮ್ಯ_Str(nodep->fileline()->lineno()) + ": Assertion failed in %m"
                    + ((ಸಂದೇಶ != "") ? ": " : "") + ಸಂದೇಶ + "\n");
        }
        ಹಿಂತಿರುಗಿ ("[%0t] "s + ಪೂರ್ವಪ್ರತ್ಯಯ + ": " + nodep->fileline()->filebasename() + ":"
                + cvt_ಗಮ್ಯ_Str(nodep->fileline()->lineno()) + ": %m" + ((ಸಂದೇಶ != "") ? ": " : "")
                + ಸಂದೇಶ + "\n");
    }
    ಶೂನ್ಯ replaceDisplay(AstDisplay* nodep, ಸ್ಥಿರಾಂಕ string& ಪೂರ್ವಪ್ರತ್ಯಯ) {
        nodep->fmtp()->ಪಠ್ಯ(
            assert_Display_ಸಂದೇಶ(nodep, ಪೂರ್ವಪ್ರತ್ಯಯ, nodep->fmtp()->ಪಠ್ಯ(), nodep->displayType()));
        nodep->displayType(VDisplayType::DT_WRITE);
        // cppcheck-suppress nullPointer
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ timenewp = ಹೊಸ Ast_ಸಮಯ{nodep->fileline(), m_modp->timeunit()};
        nodep->fmtp()->prependExprsp(timenewp);
        ಆದರೆ (!nodep->fmtp()->scopeNamep() && nodep->fmtp()->formatScopeTracking()) {
            nodep->fmtp()->scopeNamep(ಹೊಸ AstScopeName{nodep->fileline(), ನಿಜ});
        }
    }
    AstSampled* new_Sampled_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಹಿಂತಿರುಗಿ ಹೊಸ AstSampled{nodep->fileline(), nodep, nodep->dtypep(), ನಿಜ};
    }
    AstVarRef* new_Monitor_ಅಂಕೆ_Var_Refp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, VAccess access) {
        ಆದರೆ (!m_monitor_ಅಂಕೆ_Varp) {
            m_monitor_ಅಂಕೆ_Varp = ಹೊಸ AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorNum",
                                          nodep->ಕಂಡುಹಿಡಿ_UInt64_DType()};
            v3Global.rootp()->dollarUnitPkgp()->ಕೂಡಿಸು_Stmtsp(m_monitor_ಅಂಕೆ_Varp);
        }
        AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = ಹೊಸ AstVarRef{nodep->fileline(), m_monitor_ಅಂಕೆ_Varp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        ಹಿಂತಿರುಗಿ varrefp;
    }
    AstVarRef* newMonitorOffVarRefp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, VAccess access) {
        ಆದರೆ (!m_monitorOffVarp) {
            m_monitorOffVarp = ಹೊಸ AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorOff",
                                          nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType()};
            v3Global.rootp()->dollarUnitPkgp()->ಕೂಡಿಸು_Stmtsp(m_monitorOffVarp);
        }
        AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = ಹೊಸ AstVarRef{nodep->fileline(), m_monitorOffVarp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        ಹಿಂತಿರುಗಿ varrefp;
    }
    ಸ್ಥಿರ AstIf* newIfAssertOn(Ast_ಗಂಟು* bodyp, VAssertDirectiveType directiveType,
                                VAssertType type = VAssertType::ಆಂತರಿಕ) {
        // Add an internal if to check assertions are on.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = bodyp->fileline();

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = assertOnCond(fl, type, directiveType);
        AstIf* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(ನಿಜ);  // To avoid LATCH warning
        newp->user2(ನಿಜ);  // Mark as an assertOn() check
        ಹಿಂತಿರುಗಿ newp;
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_Stmt* newIfAssertPassOn(Ast_ಗಂಟು* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type, ತಾರ್ಕಿಕ vacuous) {
        // Add an internal if to check assertion passOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = bodyp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = assertPassOnCond(fl, type, directiveType, vacuous);
        Ast_ಗಂಟು_If* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(ನಿಜ);  // To avoid LATCH warning
        newp->user1(ನಿಜ);  // Don't assert/cover this if
        newp->user2(ನಿಜ);  // Mark as an assertOn() check
        ಹಿಂತಿರುಗಿ newp;
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_Stmt* newIfAssertFailOn(Ast_ಗಂಟು* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type) {
        // Add an internal if to check assertion failOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = bodyp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = assertFailOnCond(fl, type, directiveType);
        Ast_ಗಂಟು_If* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(ನಿಜ);  // To avoid LATCH warning
        newp->user1(ನಿಜ);  // Don't assert/cover this if
        newp->user2(ನಿಜ);  // Mark as an assertOn() check
        ಹಿಂತಿರುಗಿ newp;
    }

    ಸ್ಥಿರ AstIf* assertCond(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Cover_Or_Assert* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* propp,
                             Ast_ಗಂಟು* passsp, Ast_ಗಂಟು* failsp) {
        AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{nodep->fileline(), propp, passsp, failsp};
        // It's more LIKELY that we'll take the nullptr if clause
        // than the sim-killing else clause:
        ifp->branchPred(VBranchPred::BP_LIKELY);
        ifp->isBoundsCheck(ನಿಜ);  // To avoid LATCH warning
        ಹಿಂತಿರುಗಿ ifp;
    }

    Ast_ಗಂಟು_Stmt* assertBody(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Cover_Or_Assert* nodep, Ast_ಗಂಟು* propp, Ast_ಗಂಟು* passsp,
                            Ast_ಗಂಟು* failsp) {
        ಆದರೆ (AstPExpr* ಸ್ಥಿರಾಂಕ pexprp = VN_CAST(propp, PExpr)) {
            ಆದರೆ (!v3Global.opt.timing().is_ಗಣ_True()) {
                nodep->v3warn(E_NOTIMING, "This property expression requires --timing");
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(pexprp), pexprp);
                ಹಿಂತಿರುಗಿ ಹೊಸ AstBegin{nodep->fileline(), "", ಶೂನ್ಯಸೂಚಕ, ಸುಳ್ಳು};
            }
            AstFork* ಸ್ಥಿರಾಂಕ forkp = ಹೊಸ AstFork{nodep->fileline(), VJoinType::JOIN_NONE};
            forkp->ಕೂಡಿಸು_Forksp(pexprp->bodyp()->unlink_Fr_ಹಿಂಭಾಗ());
            ಆದರೆ (Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ finalp = pexprp->finalp()) {
                ಆದರೆ (!m_finalp) {
                    m_finalp = ಹೊಸ AstFinal{m_modp->fileline(), finalp->unlink_Fr_ಹಿಂಭಾಗ()};
                    m_modp->ಕೂಡಿಸು_Stmtsp(m_finalp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    m_finalp->ಕೂಡಿಸು_Stmtsp(finalp->unlink_Fr_ಹಿಂಭಾಗ());
                }
            }
            VL_DO_DANGLING2(ತಳ್ಳು_Deletep(pexprp), pexprp, propp);
            ಹಿಂತಿರುಗಿ forkp;
        }

        ಹಿಂತಿರುಗಿ assertCond(nodep, VN_AS(propp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ), passsp, failsp);
    }

    Ast_ಗಂಟು_Stmt* newFireAssertUnchecked(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Stmt* nodep, ಸ್ಥಿರಾಂಕ string& ಸಂದೇಶ,
                                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp = ಶೂನ್ಯಸೂಚಕ) {
        // Like newFireAssert() but omits the asserts-on check
        AstDisplay* ಸ್ಥಿರಾಂಕ dispp
            = ಹೊಸ AstDisplay{nodep->fileline(), VDisplayType::DT_ERROR, ಸಂದೇಶ, ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ bodysp = dispp;
        replaceDisplay(dispp, "%%Error");  // Convert to standard DISPLAY format
        ಆದರೆ (exprsp) dispp->fmtp()->exprsp()->ಕೂಡಿಸು_ಮುಂದಿನ(exprsp);
        ಆದರೆ (v3Global.opt.stopFail()) bodysp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ AstStop{nodep->fileline(), ಸುಳ್ಳು});
        ಹಿಂತಿರುಗಿ bodysp;
    }

    Ast_ಗಂಟು_Stmt* newFireAssert(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Stmt* nodep, VAssertDirectiveType directiveType,
                               VAssertType assertType, ಸ್ಥಿರಾಂಕ string& ಸಂದೇಶ,
                               Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp = ಶೂನ್ಯಸೂಚಕ) {
        Ast_ಗಂಟು_Stmt* bodysp = newFireAssertUnchecked(nodep, ಸಂದೇಶ, exprsp);
        bodysp = newIfAssertOn(bodysp, directiveType, assertType);
        ಹಿಂತಿರುಗಿ bodysp;
    }

    AstVar* createDelayedVar(ಸ್ಥಿರಾಂಕ std::string& name, AstAlways* alwaysp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp,
                             Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* initp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = exprp->fileline();
        AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        // TODO: this lifetime seems nonsene (can't have NBAs to automatics), but is as before
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->ಕೂಡಿಸು_Stmtsp(varp);
        // Before enough clock ticks, $past returns the expression's default sampled value.
        ಆದರೆ (initp) {
            m_modp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstInitialStatic{
                flp, ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, varp, VAccess::WRITE}, initp}});
        }
        ++m_statPastVars;
        // Actually set the delayed value
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = ಹೊಸ AstVarRef{flp, varp, VAccess::WRITE};
        Ast_ನಿಯೋಜಿಸು_Dly* ಸ್ಥಿರಾಂಕ assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{flp, lhsp, exprp};
        alwaysp->prependStmtsp(assignp);
        ಹಿಂತಿರುಗಿ varp;
    }

    AstAlways* getDelayedAlways(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* initp, Ast_Sen_ವೃಕ್ಷ* senTreep) {
        ಸ್ವಯಂಚಾಲಿತ& expr2Sen2Alwaysr
            = initp ? m_modExpr2Sen2DefaultDelayedAlwaysp : m_modExpr2Sen2DelayedAlwaysp;
        AstAlways*& alwayspr = expr2Sen2Alwaysr[*exprp][*senTreep];
        ಆದರೆ (!alwayspr) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = exprp->fileline();
            // Create the always block that computes the delayed values
            alwayspr = ಹೊಸ AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ಶೂನ್ಯಸೂಚಕ};
            m_modp->ಕೂಡಿಸು_Stmtsp(alwayspr);
            // Create the once-delayed variable
            ಸ್ಥಿರಾಂಕ std::string name = "_Vpast_" + cvt_ಗಮ್ಯ_Str(m_mod_Past_ಅಂಕೆ++) + "_1";
            AstVar* ಸ್ಥಿರಾಂಕ varp = createDelayedVar(name, alwayspr, exprp, initp);
            // Add it to delayed variable vector
            m_delayed(alwayspr).emplace_ಹಿಂಭಾಗ(varp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Reusing exiting, not needed
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(exprp), exprp);
            ಆದರೆ (initp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(initp), initp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(senTreep), senTreep);
        }
        ಹಿಂತಿರುಗಿ alwayspr;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* get_Past_ಮೌಲ್ಯ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* initp, Ast_Sen_ವೃಕ್ಷ* senTreep,
                              uint32_t ticks) {
        UASSERT_OBJ(ticks > 0, exprp, "Delay must be > 0");
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = exprp->fileline();
        AstAlways* ಸ್ಥಿರಾಂಕ alwaysp
            = getDelayedAlways(exprp, initp ? initp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು) : ಶೂನ್ಯಸೂಚಕ, senTreep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ resultp = past_ಮೌಲ್ಯ_Ref(alwaysp, flp, ticks, initp);
        ಆದರೆ (initp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(initp), initp);
        ಹಿಂತಿರುಗಿ resultp;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* past_ಮೌಲ್ಯ_Ref(AstAlways* alwaysp, File_ಸಾಲು* flp, uint32_t ticks,
                              Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* initp) {
        std::ಸದಿಶ<AstVar*>& delayedr = m_delayed(alwaysp);
        // Ensure the required delay exists
        ಆಗ (delayedr.ಗಾತ್ರ_2() < ticks) {
            AstVar* ಸ್ಥಿರಾಂಕ firstp = delayedr.front();
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ varFlp = firstp->fileline();
            // Create once more delayed value
            std::string name = firstp->name();
            name.resize(name.ಗಾತ್ರ_2() - 1);
            name += std::ಗಮ್ಯ_string(delayedr.ಗಾತ್ರ_2() + 1);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ prevp = ಹೊಸ AstVarRef{varFlp, delayedr.ಹಿಂಭಾಗ(), VAccess::READ};
            AstVar* ಸ್ಥಿರಾಂಕ varp = createDelayedVar(name, alwaysp, prevp,
                                                  initp ? initp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು) : ಶೂನ್ಯಸೂಚಕ);
            // Add it to delayed variable vector
            delayedr.emplace_ಹಿಂಭಾಗ(varp);
        }
        // Return a reference to the appropriately delayed variable
        ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{flp, delayedr.at(ticks - 1), VAccess::READ};
    }

    // Time of the pipeline's last sampling tick, for end-of-simulation readers
    AstVar* get_Past_Tick_ಸಮಯ_Var(AstAlways* alwaysp) {
        AstVar*& varpr = m_delayedAlways2TickTimep[alwaysp];
        ಆದರೆ (!varpr) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = alwaysp->fileline();
            varpr = ಹೊಸ AstVar{flp, VVarType::MODULETEMP,
                               "_Vpast_" + cvt_ಗಮ್ಯ_Str(m_mod_Past_ಅಂಕೆ++) + "_t",
                               m_modp->ಕಂಡುಹಿಡಿ_UInt64_DType()};
            m_modp->ಕೂಡಿಸು_Stmtsp(varpr);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ timep = ಹೊಸ AstCExpr{flp, "vlSymsp->_vm_contextp__->time()", 64};
            alwaysp->ಕೂಡಿಸು_Stmtsp(
                ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, varpr, VAccess::WRITE}, timep});
        }
        ಹಿಂತಿರುಗಿ varpr;
    }

    ಶೂನ್ಯ ಭೇಟಿ_Assertion_Iterate(Ast_ಗಂಟು_Cover_Or_Assert* nodep, Ast_ಗಂಟು* failsp) {
        ಆದರೆ (m_beginp && nodep->name() == "") nodep->name(m_beginp->name());

        { AssertDeFutureVisitor{nodep->propp(), m_modp, m_mod_Past_ಅಂಕೆ++}; }

        iterate_And_ಮುಂದಿನ_Null(nodep->sentreep());
        ಆದರೆ (AstAssert* ಸ್ಥಿರಾಂಕ assertp = VN_CAST(nodep, Assert)) {
            iterate_And_ಮುಂದಿನ_Null(assertp->failsp());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstAssertIntrinsic* ಸ್ಥಿರಾಂಕ assertp = VN_CAST(nodep, AssertIntrinsic)) {
            iterate_And_ಮುಂದಿನ_Null(assertp->failsp());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstCover* ಸ್ಥಿರಾಂಕ coverp = VN_CAST(nodep, Cover)) {
            iterate_And_ಮುಂದಿನ_Null(coverp->coverincsp());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!VN_IS(nodep, Restrict)) {
            nodep->v3fatalSrc("Unhandled assert type");
        }
        iterate_And_ಮುಂದಿನ_Null(nodep->passsp());
        Ast_Sen_ವೃಕ್ಷ* sentreep = nodep->sentreep();
        ಆದರೆ (nodep->ತಕ್ಷಣ()) {
            UASSERT_OBJ(!sentreep, nodep, "Immediate assertions don't have sensitivity");
        } ಇಲ್ಲದಿದ್ದರೆ {
            UASSERT_OBJ(sentreep, nodep, "Concurrent assertions must have sensitivity");
            // Explicit inline clock differs from the enclosing always: hoist
            // and warn. To support this need queue of asserts to activate.
            ಆದರೆ (m_procedurep && !nodep->senFromAlways()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Procedural concurrent assertion with"
                              " clocking event inside always (IEEE 1800-2023 16.14.6)");
                static_cast<Ast_ಗಂಟು*>(m_procedurep)->ಕೂಡಿಸು_ಮುಂದಿನ(nodep->unlink_Fr_ಹಿಂಭಾಗ());
                ಹಿಂತಿರುಗಿ;  // Later iterate will pick up
            }
            sentreep->unlink_Fr_ಹಿಂಭಾಗ();
            ಆದರೆ (m_procedurep) { VL_DO_DANGLING(ತಳ್ಳು_Deletep(sentreep), sentreep); }
        }
        //
        ಸ್ಥಿರಾಂಕ string& ಸಂದೇಶ = nodep->name();
        Ast_ಗಂಟು* passsp = nodep->passsp();
        ಆದರೆ (passsp) passsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
        ಆದರೆ (AstAssert* ಸ್ಥಿರಾಂಕ assertp = VN_CAST(nodep, Assert)) {
            failsp = assertp->failsp();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstAssertIntrinsic* ಸ್ಥಿರಾಂಕ assertp = VN_CAST(nodep, AssertIntrinsic)) {
            failsp = assertp->failsp();
        }
        ಆದರೆ (failsp) failsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();

        ತಾರ್ಕಿಕ selfDestruct = ಸುಳ್ಳು;
        ತಾರ್ಕಿಕ passspGated = ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstCover* ಸ್ಥಿರಾಂಕ coverp = VN_CAST(nodep, Cover);
        // A sequence event control is not an assertion directive; no assertion control
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ seqEvent = coverp && coverp->isSeqEvent();
        ಆದರೆ (coverp) {
            ++m_statCover;
            ಆದರೆ (seqEvent) {
                // Keep the event-fire action, with no coverage bucket
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!v3Global.opt.coverageUser()) {
                selfDestruct = ನಿಜ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                // V3Coverage assigned us a bucket to increment.
                AstCoverInc* ಸ್ಥಿರಾಂಕ covincp = VN_AS(coverp->coverincsp(), CoverInc);
                UASSERT_OBJ(covincp, coverp, "Missing AstCoverInc under assertion");
                covincp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();  // next() might have  AstAssign for trace
                ಆದರೆ (ಸಂದೇಶ != "") covincp->declp()->comment(ಸಂದೇಶ);
                ಆದರೆ (passsp) {
                    passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                               /*vacuous=*/ಸುಳ್ಳು);
                    passspGated = ನಿಜ;
                    passsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ<Ast_ಗಂಟು, Ast_ಗಂಟು>(covincp, passsp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    passsp = covincp;
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, Assert) || VN_IS(nodep, AssertIntrinsic)) {
            ಆದರೆ (nodep->ತಕ್ಷಣ()) {
                ++m_statAsImm;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ++m_statAsNotImm;
            }
            ಆದರೆ (!passsp && !failsp)
                failsp = newFireAssertUnchecked(
                    nodep, VN_IS(nodep, AssertIntrinsic) ? "'$cast' failed." : "'assert' failed.");
        } ಇಲ್ಲದಿದ್ದರೆ {
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
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* disablep = ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಹೊಂದಿಕೆ_Countp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (AstPropSpec* ಸ್ಥಿರಾಂಕ specp = VN_CAST(nodep->propp(), PropSpec)) {
            propExprp = specp->propp()->unlink_Fr_ಹಿಂಭಾಗ();
            ಆದರೆ (specp->disablep()) disablep = specp->disablep()->unlink_Fr_ಹಿಂಭಾಗ();
            ಹೊಂದಿಕೆ_Countp = specp->ಹೊಂದಿಕೆ_Countp();
        } ಇಲ್ಲದಿದ್ದರೆ {
            propExprp = nodep->propp()->unlink_Fr_ಹಿಂಭಾಗ();
        }
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        ತಾರ್ಕಿಕ passspAlreadyGated = ಸುಳ್ಳು;
        ಆದರೆ (passsp && VN_IS(passsp, If)) passspAlreadyGated = VN_AS(passsp, If)->user1();
        ಆದರೆ (passsp && !passspGated && !passspAlreadyGated && !VN_IS(propExprp, PExpr)
            && !seqEvent) {
            passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                       /*vacuous=*/ಸುಳ್ಳು);
        }
        ಆದರೆ (failsp && !VN_IS(propExprp, PExpr)) {
            failsp = newIfAssertFailOn(failsp, nodep->directive(), nodep->userType());
        }
        ಆದರೆ (coverp && ಹೊಂದಿಕೆ_Countp && passsp) {
            // Convert the match count into a loop that decrements a temporary variable until it
            // reaches zero.
            ಹೊಂದಿಕೆ_Countp->unlink_Fr_ಹಿಂಭಾಗ();
            AstVar* ಸ್ಥಿರಾಂಕ remainingp = ಹೊಸ AstVar{
                flp, VVarType::MODULETEMP, m_ಹೊಂದಿಕೆ_ಎಣಿಕೆ_ಹೆಸರುಗಳು.get(""), ಹೊಂದಿಕೆ_Countp->dtypep()};
            remainingp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->ಕೂಡಿಸು_Stmtsp(remainingp);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ replaysp
                = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, remainingp, VAccess::WRITE}, ಹೊಂದಿಕೆ_Countp};
            AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{flp};
            loopp->ಕೂಡಿಸು_Stmtsp(
                ಹೊಸ AstLoopTest{flp, loopp, ಹೊಸ AstVarRef{flp, remainingp, VAccess::READ}});
            loopp->ಕೂಡಿಸು_Stmtsp(passsp);
            loopp->ಕೂಡಿಸು_Stmtsp(
                ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, remainingp, VAccess::WRITE},
                              ಹೊಸ AstSub{flp, ಹೊಸ AstVarRef{flp, remainingp, VAccess::READ},
                                         ಹೊಸ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{},
                                                      remainingp->dtypep()->ಅಗಲ(), 1}}});
            replaysp->ಕೂಡಿಸು_ಮುಂದಿನ(loopp);
            passsp = replaysp;
        }
        Ast_ಗಂಟು* bodysp = assertBody(nodep, propExprp, passsp, failsp);
        ಆದರೆ (disablep) bodysp = ಹೊಸ AstIf{flp, ಹೊಸ AstLogNot{flp, disablep}, bodysp};
        // Add assertOn check last, for better combining
        ಆದರೆ (!seqEvent) bodysp = newIfAssertOn(bodysp, nodep->directive(), nodep->userType());
        ಆದರೆ (sentreep) bodysp = ಹೊಸ AstAlways{flp, VAlwaysKwd::ALWAYS, sentreep, bodysp};

        ಆದರೆ (passsp && !passsp->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(passsp), passsp);
        ಆದರೆ (failsp && !failsp->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(failsp), failsp);

        // Install it
        ಆದರೆ (selfDestruct) {
            // Delete it after making the tree.  This way we can tell the user
            // if it wasn't constructed nicely or has other errors without needing --coverage.
            VL_DO_DANGLING(bodysp->delete_ವೃಕ್ಷ(), bodysp);
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->replaceWith(bodysp);
        }
        // Bye
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ತಾರ್ಕಿಕ is_ಖಾಲಿಗಳು_Stmt(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (AstBegin* ಸ್ಥಿರಾಂಕ beginp = VN_CAST(nodep, Begin)) {
            ಆದರೆ (beginp->declsp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಗಾಗಿ (Ast_ಗಂಟು* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ಆದರೆ (!is_ಖಾಲಿಗಳು_Stmt(stmtp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstIf* nodep) override {
        ಆದರೆ (!nodep->user1_ಗಣ_Once()) {
            ಆದರೆ (nodep->uniquePragma() || nodep->unique0Pragma()) {
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_If* ifp = nodep;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* propp = ಶೂನ್ಯಸೂಚಕ;
                ತಾರ್ಕಿಕ hasDefaultElse = ಸುಳ್ಳು;
                ಮಾಡು {
                    // If this statement ends with 'else if', then nextIf will point to the
                    // nextIf statement.  Otherwise it will be null.
                    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_If* ಸ್ಥಿರಾಂಕ nextifp = dynamic_cast<Ast_ಗಂಟು_If*>(ifp->elsesp());
                    iterate_And_ಮುಂದಿನ_Null(ifp->condp());

                    // Recurse into the true case.
                    iterate_And_ಮುಂದಿನ_Null(ifp->thensp());

                    // If the last else is not an else if, recurse into that too.
                    ಆದರೆ (ifp->elsesp() && !nextifp) {  //
                        iterate_And_ಮುಂದಿನ_Null(ifp->elsesp());
                    }

                    // Build a bitmask of the true predicates
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ predp = ifp->condp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                    ಆದರೆ (propp) {
                        propp = ಹೊಸ AstConcat{nodep->fileline(), predp, propp};
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        propp = predp;
                    }

                    // Record if this ends with an 'else' that does not have an if
                    ಆದರೆ (ifp->elsesp() && !nextifp) hasDefaultElse = ನಿಜ;

                    ifp = nextifp;
                } ಆಗ (ifp);

                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ allow_none = nodep->unique0Pragma();

                // Empty case means no property
                ಆದರೆ (!propp) propp = ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False{}};

                // Note: if this ends with an 'else', then we don't need to validate that one of
                // the predicates evaluates to true.
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ohot
                    = ((allow_none || hasDefaultElse)
                           ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstOneHot0{nodep->fileline(), propp})
                           : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstOneHot{nodep->fileline(), propp}));
                ಸ್ಥಿರಾಂಕ VAssertType assertType
                    = nodep->uniquePragma() ? VAssertType::UNIQUE : VAssertType::UNIQUE0;
                // Move, not clone, as the branches might declare referenced variables
                VNRelinker relinker;
                nodep->unlink_Fr_ಹಿಂಭಾಗ(&relinker);
                AstIf* ಸ್ಥಿರಾಂಕ checkifp
                    = ಹೊಸ AstIf{nodep->fileline(), ಹೊಸ AstLogNot{nodep->fileline(), ohot},
                                newFireAssert(nodep, VAssertDirectiveType::VIOLATION_IF,
                                              assertType, "'unique if' statement violated"),
                                nodep};
                checkifp->isBoundsCheck(ನಿಜ);  // To avoid LATCH warning
                checkifp->branchPred(VBranchPred::BP_UNLIKELY);
                relinker.relink(checkifp);
                ಹಿಂತಿರುಗಿ;
            }
        }

        iterateChildren(nodep);

        ಆದರೆ (nodep->user2()) {
            // Combine consecutive assertOn checks if possible
            ಆದರೆ (AstIf* ಸ್ಥಿರಾಂಕ backp = VN_CAST(nodep->backp(), If)) {
                ಆದರೆ (backp->nextp() == nodep  //
                    && backp->user2()  //
                    && backp->condp()->same_ವೃಕ್ಷ(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    backp->ಕೂಡಿಸು_Thensp(nodep->thensp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
                    nodep->unlink_Fr_ಹಿಂಭಾಗ();
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಹಿಂತಿರುಗಿ;
                }
            }
            // Combine nested assertOn checks if possible
            ಆದರೆ (nodep->thensp() && !nodep->thensp()->nextp() && is_ಖಾಲಿಗಳು_Stmt(nodep->elsesp())) {
                AstIf* ಸ್ಥಿರಾಂಕ checkp = VN_CAST(nodep->thensp(), If);
                ಆದರೆ (checkp  //
                    && checkp->user2()  //
                    && checkp->condp()->same_ವೃಕ್ಷ(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    nodep->ಕೂಡಿಸು_Thensp(checkp->thensp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(checkp->unlink_Fr_ಹಿಂಭಾಗ()), checkp);
                    ಹಿಂತಿರುಗಿ;
                }
            }
            ಹಿಂತಿರುಗಿ;
        }

        // Swap assertOn check with single statement 'if' statement to bubble up for combining
        // Note we can't just swap the conditions as they two Ifs have different flags,
        // so swapping the Ifs themeselves then swapping back the bodies.
        ಆದರೆ (nodep->condp()->is_ಶುದ್ಧ()) {
            ಆದರೆ (nodep->thensp() && !nodep->thensp()->nextp() && is_ಖಾಲಿಗಳು_Stmt(nodep->elsesp())) {
                AstIf* ಸ್ಥಿರಾಂಕ checkp = VN_CAST(nodep->thensp(), If);
                ಆದರೆ (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlink_Fr_ಹಿಂಭಾಗ());
                    nodep->ಕೂಡಿಸು_Thensp(checkp->thensp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
                    checkp->ಕೂಡಿಸು_Thensp(nodep);
                    ಹಿಂತಿರುಗಿ;
                }
            }
            ಆದರೆ (nodep->elsesp() && !nodep->elsesp()->nextp() && is_ಖಾಲಿಗಳು_Stmt(nodep->thensp())) {
                AstIf* ಸ್ಥಿರಾಂಕ checkp = VN_CAST(nodep->elsesp(), If);
                ಆದರೆ (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlink_Fr_ಹಿಂಭಾಗ());
                    nodep->ಕೂಡಿಸು_Elsesp(checkp->thensp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
                    checkp->ಕೂಡಿಸು_Thensp(nodep);
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }
    }

    //========== Case assertions
    ಶೂನ್ಯ ಭೇಟಿ(AstCase* nodep) override {
        // Introduce temporary variable for AstCase if needed - it is done here and not in V3Case
        // because this phase is before V3Scope and V3Case is not. Doing it before V3Scope ensures
        // that V3Scope will take care of a scope creation
        // We also need to do it before V3Begin, co that pragmas like `unique0` also work correctly
        ಆದರೆ (!nodep->exprp()->is_ಶುದ್ಧ()) {
            ++m_statLiftedCaseExprs;
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->exprp()->fileline();
            AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{fl, VVarType::BLOCKTEMP, m_case_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು.get(nodep),
                                            nodep->exprp()->dtypep()};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ origp = nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ();
            nodep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(
                ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE}, origp});
            nodep->exprp(ಹೊಸ AstVarRef{fl, varp, VAccess::READ});
            ಆದರೆ (m_ftaskp) {
                varp->funcLocal(ನಿಜ);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_modp->prependStmtsp(varp);
            }
            VIsCached::clear_Cache_ವೃಕ್ಷ();
        }
        iterateChildren(nodep);
        ಆದರೆ (!nodep->user1_ಗಣ_Once()) {
            ತಾರ್ಕಿಕ has_default = ಸುಳ್ಳು;
            ಗಾಗಿ (Ast_Case_ಅಂಶ* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Case_ಅಂಶ)) {
                ಆದರೆ (itemp->isDefault()) has_default = ನಿಜ;
            }
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಅಭಿವ್ಯಕ್ತಿ_Dtypep = nodep->exprp()->dtypep()->skipRefp();

            VAssertType assertType = VAssertType::ಆಂತರಿಕ;
            ಆದರೆ (nodep->ಆದ್ಯತೆ_Pragma()) {
                assertType = VAssertType::ಆದ್ಯತೆ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->uniquePragma()) {
                assertType = VAssertType::UNIQUE;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->unique0Pragma()) {
                assertType = VAssertType::UNIQUE0;
            }

            string valFmt;
            ಆದರೆ (ಅಭಿವ್ಯಕ್ತಿ_Dtypep->isIntegralOrPacked())
                valFmt = " for '" + cvt_ಗಮ್ಯ_Str(ಅಭಿವ್ಯಕ್ತಿ_Dtypep->ಅಗಲ_Min()) + "'h%X'";
            ಆದರೆ (nodep->ತುಂಬಿದ_Pragma() || nodep->ಆದ್ಯತೆ_Pragma()) {
                // Need to add a default if there isn't one already
                ++m_stat_As_ತುಂಬಿದ;
                ಆದರೆ (!has_default) {
                    nodep->ಕೂಡಿಸು_Itemsp(ಹೊಸ Ast_Case_ಅಂಶ{
                        nodep->fileline(), ಶೂನ್ಯಸೂಚಕ /*DEFAULT*/,
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      nodep->pragmaString() + ", but non-match found" + valFmt,
                                      valFmt.ಖಾಲಿಗಳು() ? ಶೂನ್ಯಸೂಚಕ
                                                     : nodep->exprp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು))});
                }
            }
            ಆದರೆ (nodep->parallelPragma() || nodep->uniquePragma() || nodep->unique0Pragma()) {
                // Need to check that one, and only one of the case items match at any moment
                // If there's a default, we allow none to match, else exactly one must match
                ++m_stat_As_ತುಂಬಿದ;
                ಆದರೆ (!has_default && !nodep->itemsp()) {
                    // Not parallel, but harmlessly so.
                } ಇಲ್ಲದಿದ್ದರೆ {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* propp = ಶೂನ್ಯಸೂಚಕ;
                    ಗಾಗಿ (Ast_Case_ಅಂಶ* itemp = nodep->itemsp(); itemp;
                         itemp = VN_AS(itemp->nextp(), Case_ಅಂಶ)) {
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* itembitp = ಶೂನ್ಯಸೂಚಕ;
                        ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* icondp = itemp->condsp(); icondp;
                             icondp = VN_AS(icondp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* onep;
                            ಆದರೆ (AstInsideRange* ಸ್ಥಿರಾಂಕ rcondp = VN_CAST(icondp, InsideRange)) {
                                onep = rcondp->newAndFromInside(
                                    nodep->exprp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ),
                                    rcondp->lhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ),
                                    rcondp->rhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ));
                            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->casex() || nodep->casez() || nodep->caseInside()) {
                                onep = AstEqWild::newTyped(itemp->fileline(),
                                                           nodep->exprp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                                                           icondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
                            } ಇಲ್ಲದಿದ್ದರೆ {
                                onep = AstEq::newTyped(icondp->fileline(),
                                                       nodep->exprp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                                                       icondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
                            }
                            // OR together all conditions within the same case item
                            ಆದರೆ (onep) {
                                ಆದರೆ (itembitp) {
                                    itembitp = ಹೊಸ AstOr{icondp->fileline(), onep, itembitp};
                                } ಇಲ್ಲದಿದ್ದರೆ {
                                    itembitp = onep;
                                }
                            }
                        }
                        ಆದರೆ (itembitp) {
                            ಆದರೆ (propp) {
                                propp = ಹೊಸ AstConcat{itemp->fileline(), itembitp, propp};
                            } ಇಲ್ಲದಿದ್ದರೆ {
                                propp = itembitp;
                            }
                        }
                    }
                    // Empty case means no property
                    ಆದರೆ (!propp) propp = ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False{}};
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ allow_none = has_default || nodep->unique0Pragma();
                    // The following assertion looks as below.
                    // if (!$onehot(propp)) begin
                    //     if (propp == '0) begin if (!allow_none) $error("none match"); end
                    //     else $error("multiple match");
                    //ಕೊನೆ
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ohot = ಹೊಸ AstOneHot{nodep->fileline(), propp};
                    AstConst* ಸ್ಥಿರಾಂಕ zero = ಹೊಸ AstConst{
                        nodep->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, propp->ಅಗಲ(), 0};
                    AstIf* ಸ್ಥಿರಾಂಕ ohotIfp
                        = ಹೊಸ AstIf{nodep->fileline(), ಹೊಸ AstLogNot{nodep->fileline(), ohot}};
                    AstIf* ಸ್ಥಿರಾಂಕ zeroIfp = ಹೊಸ AstIf{
                        nodep->fileline(),
                        ಹೊಸ AstEq{nodep->fileline(), propp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), zero}};
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = nodep->exprp();
                    ಸ್ಥಿರಾಂಕ string pragmaStr = nodep->pragmaString();
                    ಆದರೆ (!allow_none)
                        zeroIfp->ಕೂಡಿಸು_Thensp(
                            newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                          pragmaStr + ", but none matched" + valFmt,
                                          valFmt.ಖಾಲಿಗಳು() ? ಶೂನ್ಯಸೂಚಕ : exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)));
                    zeroIfp->ಕೂಡಿಸು_Elsesp(
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      pragmaStr + ", but multiple matches found" + valFmt,
                                      valFmt.ಖಾಲಿಗಳು() ? ಶೂನ್ಯಸೂಚಕ : exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)));
                    ohotIfp->ಕೂಡಿಸು_Thensp(zeroIfp);
                    ohotIfp->isBoundsCheck(ನಿಜ);  // To avoid LATCH warning
                    ohotIfp->branchPred(VBranchPred::BP_UNLIKELY);
                    nodep->ಕೂಡಿಸು_Not_Parallelp(ohotIfp);
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstFuture* nodep) override {
        nodep->v3error("Future sampled value function called outside property or sequence "
                       "expression (IEEE 16.9.4)");
        nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), 0});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    //========== Past
    ಶೂನ್ಯ ಭೇಟಿ(AstPast* nodep) override {
        ಆದರೆ (!nodep->propertyTiming()) {
            nodep->initp(nodep->exprp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
            { Default_Sampled_ಮೌಲ್ಯ_Visitor{nodep->initp()}; }
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ initp = nodep->initp() ? nodep->initp()->unlink_Fr_ಹಿಂಭಾಗ() : ಶೂನ್ಯಸೂಚಕ;
        iterateChildren(nodep);
        uint32_t ticks = 1;
        ಆದರೆ (nodep->ticksp()) {
            UASSERT_OBJ(VN_IS(nodep->ticksp(), Const), nodep,
                        "Expected constant ticks, checked in V3Width");
            ticks = VN_AS(nodep->ticksp(), Const)->ಗಮ್ಯ_UInt();
        }
        UASSERT_OBJ(ticks >= 1, nodep, "0 tick should have been checked in V3Width");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = new_Sampled_ಅಭಿವ್ಯಕ್ತಿ(nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ());
        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep = nodep->sentreep()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* inp = ಶೂನ್ಯಸೂಚಕ;
        // Deliberately emitted for every ftask $past; calls outside final take the normal stage
        ಆದರೆ (VN_IS(m_procedurep, Final) || m_ftaskp) {
            // A sampling-tick finish reads one stage deeper (IEEE 1800-2023 16.9.3).
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
            AstAlways* ಸ್ಥಿರಾಂಕ alwaysp
                = getDelayedAlways(exprp, initp ? initp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು) : ಶೂನ್ಯಸೂಚಕ, senTreep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ betweenTicksp = past_ಮೌಲ್ಯ_Ref(alwaysp, flp, ticks, initp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ onTickp = past_ಮೌಲ್ಯ_Ref(alwaysp, flp, ticks + 1, initp);
            ಆದರೆ (initp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(initp), initp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ onTickCondp = ಹೊಸ AstLogAnd{
                flp, ಹೊಸ AstCExpr{flp, "vlSymsp->_vm_contextp__->executingFinal()", 1},
                ಹೊಸ AstEq{flp, ಹೊಸ AstVarRef{flp, get_Past_Tick_ಸಮಯ_Var(alwaysp), VAccess::READ},
                          ಹೊಸ AstCExpr{flp, "vlSymsp->_vm_contextp__->finishPendingTime()", 64}}};
            AstCond* ಸ್ಥಿರಾಂಕ condp = ಹೊಸ AstCond{flp, onTickCondp, onTickp, betweenTicksp};
            condp->dtypeFrom(onTickp);
            inp = condp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            inp = get_Past_ಮೌಲ್ಯ(exprp, initp, senTreep, ticks);
        }
        nodep->replaceWith(inp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    //========== Move $sampled down to read-only variables
    ಶೂನ್ಯ ಭೇಟಿ(AstSampled* nodep) override {
        ಆದರೆ (nodep->user1()) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_ಒಳ_Sampled);
        {
            m_ಒಳ_Sampled = ನಿಜ;
            iterateChildren(nodep);
        }
        ಆದರೆ (nodep->exprp()) {
            nodep->replaceWith(nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ());
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಆದರೆ (m_ಒಳ_Sampled && !nodep->user2() && !nodep->varp()->is_ತಾತ್ಕಾಲಿಕ()) {
            ಆದರೆ (!nodep->access().isReadOnly()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Write to variable in sampled expression");
            } ಇಲ್ಲದಿದ್ದರೆ {
                VNRelinker relinkHandle;
                nodep->unlink_Fr_ಹಿಂಭಾಗ(&relinkHandle);
                AstSampled* ಸ್ಥಿರಾಂಕ newp = new_Sampled_ಅಭಿವ್ಯಕ್ತಿ(nodep);
                relinkHandle.relink(newp);
                newp->user1(1);
                v3Global.ಗಣ_Has_Sampled();
            }
        }
    }
    // Don't sample sensitivities
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sen_ಅಂಶ* nodep) override {
        VL_RESTORER(m_ಒಳ_Sampled);
        m_ಒಳ_Sampled = ಸುಳ್ಳು;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPExprClause* nodep) override {
        Ast_ಗಂಟು* stmtsp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (nodep->pass() && m_passsp) {
            // Cover adds COVERINC by AstNode::addNext, thus need to clone next too.
            stmtsp = m_passsp->clone_ವೃಕ್ಷ(ನಿಜ);
            stmtsp = newIfAssertPassOn(stmtsp, m_assertp->directive(), m_assertp->userType(),
                                       nodep->vacuous());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!nodep->pass() && m_failsp) {
            stmtsp = m_failsp->clone_ವೃಕ್ಷ(ನಿಜ);
            stmtsp = newIfAssertFailOn(stmtsp, m_assertp->directive(), m_assertp->userType());
        }
        ಆದರೆ (stmtsp) {
            stmtsp->foreach_And_ಮುಂದಿನ([](Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ refp) {
                // References inside action blocks shouldn't be implicitly sampled
                // m_passsp/m_failsp have been already visited once and refs explicitly sampled
                // are handled already
                refp->user2(1);
            });
            nodep->replaceWith(stmtsp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPExpr* nodep) override {
        ಆದರೆ (m_ಒಳ_Restrict) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterateChildren(nodep);
        }
    }

    //========== Statements
    ಶೂನ್ಯ ಭೇಟಿ(AstDisplay* nodep) override {
        iterateChildren(nodep);
        // Replace the special types with standard text
        ಆದರೆ (nodep->displayType() == VDisplayType::DT_INFO) {
            replaceDisplay(nodep, "-Info");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->displayType() == VDisplayType::DT_WARNING) {
            replaceDisplay(nodep, "%%Warning");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->displayType() == VDisplayType::DT_ERROR) {
            replaceDisplay(nodep, "%%Error");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->displayType() == VDisplayType::DT_FATAL) {
            replaceDisplay(nodep, "%%Fatal");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->displayType() == VDisplayType::DT_MONITOR) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();

            Ast_Sen_ಅಂಶ* monSenItemsp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ monExprsp = nodep->fmtp()->exprsp()) {
                monExprsp->foreach_And_ಮುಂದಿನ([&](AstVarRef* varrefp) {
                    Ast_Sen_ಅಂಶ* ಸ್ಥಿರಾಂಕ senItemp
                        = ಹೊಸ Ast_Sen_ಅಂಶ{fl, VEdgeType::ET_ಬದಲಾಯಿತು,
                                         // Clone so get VarRef or VarXRef as needed
                                         varrefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)};
                    monSenItemsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(monSenItemsp, senItemp);
                });
            }

            Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ mon_Sen_ವೃಕ್ಷ = ಹೊಸ Ast_Sen_ವೃಕ್ಷ{fl, monSenItemsp};
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ mon_ಅಂಕೆ = ++m_monitor_ಅಂಕೆ;
            // Where $monitor was we do "__VmonitorNum = N;"
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ newsetp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                fl, new_Monitor_ಅಂಕೆ_Var_Refp(nodep, VAccess::WRITE), ಹೊಸ AstConst{fl, mon_ಅಂಕೆ}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__VmonitorOn && __VmonitorNum==N) $display(...);"
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = nodep;
            AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{
                fl,
                ಹೊಸ AstLogAnd{fl, ಹೊಸ AstLogNot{fl, newMonitorOffVarRefp(nodep, VAccess::READ)},
                              ಹೊಸ AstEq{fl, ಹೊಸ AstConst{fl, mon_ಅಂಕೆ},
                                        new_Monitor_ಅಂಕೆ_Var_Refp(nodep, VAccess::READ)}},
                stmtsp};
            ifp->isBoundsCheck(ನಿಜ);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstAlways{fl, VAlwaysKwd::ALWAYS, mon_Sen_ವೃಕ್ಷ, ifp};
            m_modp->ಕೂಡಿಸು_Stmtsp(newp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->displayType() == VDisplayType::DT_STROBE) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            // Need one-shot
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
            AstVar* ಸ್ಥಿರಾಂಕ varp
                = ಹೊಸ AstVar{fl, VVarType::MODULETEMP, "__Vstrobe" + cvt_ಗಮ್ಯ_Str(m_mod_Strobe_ಅಂಕೆ++),
                             nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType()};
            m_modp->ಕೂಡಿಸು_Stmtsp(varp);
            // Where $strobe was we do "__Vstrobe = '1;"
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ newsetp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE},
                                                     ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__Vstrobe) begin $display(...); __Vstrobe = '0; end"
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = nodep;
            AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::READ}, stmtsp};
            ifp->isBoundsCheck(ನಿಜ);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstAlwaysPostponed{fl, ifp};
            stmtsp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE},
                                          ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_False{}}});
            m_modp->ಕೂಡಿಸು_Stmtsp(newp);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMonitorOff* nodep) override {
        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ newp
            = ಹೊಸ Ast_ನಿಯೋಜಿಸು{nodep->fileline(), newMonitorOffVarRefp(nodep, VAccess::WRITE),
                            ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_True{}, nodep->off()}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAssert* nodep) override {  //
        ಭೇಟಿ_Assertion_Iterate(nodep, nodep->failsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAssertCtl* nodep) override {
        iterateChildren(nodep);

        ತಾರ್ಕಿಕ assertTypeConst = ನಿಜ;
        ಆದರೆ (!nodep->assertTypesp()) {
            nodep->assertTypes(VAssertType{ALL_ASSERT_TYPES});
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ assertTypesp = VN_CAST(nodep->assertTypesp(), Const)) {
            nodep->assertTypes(VAssertType{assertTypesp->ಗಮ್ಯ_SInt()});
        } ಇಲ್ಲದಿದ್ದರೆ {
            assertTypeConst = ಸುಳ್ಳು;
        }

        ತಾರ್ಕಿಕ controlTypeConst = ಸುಳ್ಳು;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep->controlTypep(), Const)) {
            nodep->ctlType(constp->ಗಮ್ಯ_SInt());
            controlTypeConst = ನಿಜ;
        }
        ಆದರೆ (controlTypeConst
            && (nodep->ctlType() < VAssertCtlType::ಬೀಗ
                || nodep->ctlType() > VAssertCtlType::VACUOUS_OFF)) {
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
            nodep->v3error("Bad $assertcontrol control_type '"
                           << cvt_ಗಮ್ಯ_Str(static_cast<ಪೂರ್ಣಾಂಕ>(nodep->ctlType()))
                           << "' (IEEE 1800-2023 Table 20-5)");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (assertTypeConst && nodep->assertTypes() != ALL_ASSERT_TYPES
            && nodep->assertTypes().ಒಳಗೊಂಡಿದೆ_Any(VAssertType::UNIQUE | VAssertType::UNIQUE0
                                                | VAssertType::ಆದ್ಯತೆ)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: assert control assertion_type");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        }

        ತಾರ್ಕಿಕ directiveTypeConst = ನಿಜ;
        ಆದರೆ (!nodep->directiveTypesp()) {
            nodep->directiveTypes(VAssertDirectiveType::ASSERT | VAssertDirectiveType::ASSUME
                                  | VAssertDirectiveType::COVER);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ directiveTypesp
                   = VN_CAST(nodep->directiveTypesp(), Const)) {
            nodep->directiveTypes(VAssertDirectiveType{directiveTypesp->ಗಮ್ಯ_SInt()});
        } ಇಲ್ಲದಿದ್ದರೆ {
            directiveTypeConst = ಸುಳ್ಳು;
        }
        ಆದರೆ (!directiveTypeConst) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: non-const assert directive type expression");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        }

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        UINFO(9, "Generating assertctl for a module: " << m_modp);
        AstCStmt* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstCStmt{fl};
        newp->ಕೂಡಿಸು("vlSymsp->_vm_contextp__->assertCtl(");
        newp->ಕೂಡಿಸು(nodep->controlTypep()->unlink_Fr_ಹಿಂಭಾಗ());
        newp->ಕೂಡಿಸು(", ");
        ಆದರೆ (nodep->assertTypesp()) {
            newp->ಕೂಡಿಸು(nodep->assertTypesp()->unlink_Fr_ಹಿಂಭಾಗ());
        } ಇಲ್ಲದಿದ್ದರೆ {
            newp->ಕೂಡಿಸು(std::ಗಮ್ಯ_string(ALL_ASSERT_TYPES));
        }
        newp->ಕೂಡಿಸು(", " + std::ಗಮ್ಯ_string(nodep->directiveTypes()) + ");\n");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAssertIntrinsic* nodep) override {  //
        ಭೇಟಿ_Assertion_Iterate(nodep, nodep->failsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCover* nodep) override {  //
        ಭೇಟಿ_Assertion_Iterate(nodep, ಶೂನ್ಯಸೂಚಕ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRestrict* nodep) override {
        VL_RESTORER(m_ಒಳ_Restrict);
        m_ಒಳ_Restrict = ನಿಜ;
        iterateChildren(nodep);
        // IEEE says simulator ignores these
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
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
        m_finalp = ಶೂನ್ಯಸೂಚಕ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Gen_ಖಂಡ* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstBegin* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
        // If the body is a single assertOn check, bubble it up for combining
        ಆದರೆ (!nodep->declsp()) {
            ಆದರೆ (AstIf* ಸ್ಥಿರಾಂಕ ifp = VN_CAST(nodep->stmtsp(), If)) {
                ಆದರೆ (ifp->user2() && !ifp->nextp()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(ifp->unlink_Fr_ಹಿಂಭಾಗ());
                    nodep->ಕೂಡಿಸು_Stmtsp(ifp->thensp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
                    ifp->ಕೂಡಿಸು_Thensp(nodep);
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ AssertVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~AssertVisitor() override {
        V3Stats::ಕೂಡಿಸು_Stat("Assertions, assert non-immediate statements", m_statAsNotImm);
        V3Stats::ಕೂಡಿಸು_Stat("Assertions, assert immediate statements", m_statAsImm);
        V3Stats::ಕೂಡಿಸು_Stat("Assertions, cover statements", m_statCover);
        V3Stats::ಕೂಡಿಸು_Stat("Assertions, full/parallel case", m_stat_As_ತುಂಬಿದ);
        V3Stats::ಕೂಡಿಸು_Stat("Assertions, $past variables", m_statPastVars);
        V3Stats::ಕೂಡಿಸು_Stat("Assertions, assertOn checks combined", m_statAssertOnCombined);
        V3Stats::ಕೂಡಿಸು_Stat("Assertions, assertOn checks hoisted", m_statAssertOnHoisted);
        V3Stats::ಕೂಡಿಸು_Stat("Assertions, lifted impure case expressions", m_statLiftedCaseExprs);
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
