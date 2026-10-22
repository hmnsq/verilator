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
    Ast_ग्रन्थिः* m_scopep = शून्यसूचकः;

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(शून्यसूचकः);
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_Gen_खण्डम्* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(शून्यसूचकः);
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstDefaultDisable* nodep) override {
        UASSERT_OBJ(nodep, m_scopep,
                    "default disable iff must be inside a module or generate block");
        AstDefaultDisable* defaultp = शून्यसूचकः;
        यदि (ध्रुव Ast_ग्रन्थिः_Module* ध्रुव modp = VN_CAST(m_scopep, ग्रन्थिः_Module)) {
            defaultp = modp->defaultDisablep();
        } अन्यथा {
            defaultp = VN_AS(m_scopep, Gen_खण्डम्)->defaultDisablep();
        }
        यदि (VL_UNLIKELY(defaultp)) {
            nodep->v3error("Only one 'default disable iff' allowed per "
                           << (VN_IS(m_scopep, ग्रन्थिः_Module) ? "module" : "generate block")
                           << " (IEEE 1800-2023 16.15)");
        } अन्यथा यदि (Ast_ग्रन्थिः_Module* ध्रुव modp = VN_CAST(m_scopep, ग्रन्थिः_Module)) {
            modp->defaultDisablep(nodep);
        } अन्यथा {
            VN_AS(m_scopep, Gen_खण्डम्)->defaultDisablep(nodep);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    स्पष्ट DefaultDisableLocalVisitor(AstNetlist* nodep) { iterate(nodep); }
};

वर्ग DefaultDisablePropagateVisitor final : सार्वजनिक VNVisitor {
    // STATE
    AstDefaultDisable* m_defaultDisablep = शून्यसूचकः;

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_Gen_खण्डम्* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        यदि (!nodep->defaultDisablep()) nodep->defaultDisablep(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    स्पष्ट DefaultDisablePropagateVisitor(AstNetlist* nodep) { iterate(nodep); }
};

// Lower a sequence used as an event control ('@seq', IEEE 1800-2023 9.4.2.4) into a
// synthesized event fired by an internal 'cover sequence' on each end-of-match
वर्ग Seq_Event_अधस्तनम्_Visitor final : सार्वजनिक VNVisitor {
    // STATE
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Current module
    V3_Unique_नामानि m_नामानि{"__VseqEvent"};  // Synthesized event names

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstSenItem* nodep) override {
        AstFuncRef* ध्रुव funcrefp = VN_CAST(nodep->sensp(), FuncRef);
        यदि (funcrefp && VN_IS(funcrefp->taskp(), Sequence)) {
            File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
            AstVar* ध्रुव eventp = नूतन AstVar{flp, VVarType::MODULETEMP, m_नामानि.get(nodep),
                                              m_modp->findBasicDType(VBasicDTypeKwd::EVENT)};
            eventp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->सङ्कलय_Stmtsp(eventp);
            v3Global.समुच्चयः_Has_Events();
            funcrefp->unlink_Fr_पृष्ठभागः();
            nodep->sensp(नूतन AstVarRef{flp, eventp, VAccess::READ});
            ध्रुव तार्किक automaticActual = funcrefp->exists([](ध्रुव Ast_ग्रन्थिः_Var_Ref* refp) {
                प्रत्यागम refp->varp() && refp->varp()->lifetime().isAutomatic();
            });
            यदि (automaticActual) {
                nodep->v3error("Arguments to a sequence used as an event control must be"
                               " static (IEEE 1800-2023 9.4.2.4)");
                VN_AS(funcrefp->taskp(), Sequence)->isReferenced(असत्य);
                VL_DO_DANGLING(प्रक्षिप_Deletep(funcrefp), funcrefp);
                प्रत्यागम;
            }
            AstFireEvent* ध्रुव firep
                = नूतन AstFireEvent{flp, नूतन AstVarRef{flp, eventp, VAccess::WRITE}, असत्य};
            AstCover* ध्रुव coverp
                = नूतन AstCover{flp, नूतन AstPropSpec{flp, शून्यसूचकः, शून्यसूचकः, funcrefp}, firep,
                               VAssertType::CONCURRENT};
            coverp->isCoverSeq(सत्य);
            coverp->isSeqEvent(सत्य);
            m_modp->सङ्कलय_Stmtsp(coverp);
            प्रत्यागम;
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    स्पष्ट Seq_Event_अधस्तनम्_Visitor(AstNetlist* nodep) { iterate(nodep); }
};

}  // namespace

शून्य V3AssertCommon::collectDefaultDisable(AstNetlist* nodep) {
    { DefaultDisableLocalVisitor{nodep}; }
    { DefaultDisablePropagateVisitor{nodep}; }
}

शून्य V3AssertCommon::अधस्तनम्_Sequence_Events(AstNetlist* nodep) {
    { Seq_Event_अधस्तनम्_Visitor{nodep}; }
    V3Global::dump_Check_Global_वृक्षः("assertseqevent", 0, dump_वृक्षः_Either_Level() >= 3);
}

string V3AssertCommon::assertCtlGetCall(ध्रुव वर्ण* query, VAssertType type,
                                        VAssertDirectiveType directiveType) {
    // Template arguments let the runtime build the type/directive mask at C++ compile time
    प्रत्यागम "vlSymsp->_vm_contextp__->assertCtlGet<"s + std::गन्तव्यम्_string(type) + ", "s
           + std::गन्तव्यम्_string(directiveType) + ">(VerilatedAssertCtlQuery::"s + query + ")"s;
}

तार्किक V3AssertCommon::check_Sva_मानम्_सीमा(Ast_ग्रन्थिः* nodep, ध्रुव AstConst* constp, ध्रुव वर्ण* what) {
    // Temporal values can materialize as O(N) AST or state in later passes.
    // Leave headroom for a range ring's extra slot and V3Number's signed
    // `(bits + 31) / 32` word count.
    स्थिर constexpr अचिह्नित SVA_मानम्_HARD_सीमा
        = static_cast<अचिह्नित>(std::numeric_limits<पूर्णाङ्क>::max()) - 32U;
    ध्रुव अचिह्नित configured_सीमा = static_cast<अचिह्नित>(v3Global.opt.max_अङ्कः_विस्तारः());
    ध्रुव अचिह्नित सीमा = std::min(configured_सीमा, SVA_मानम्_HARD_सीमा);
    यदि (constp->अङ्कः().माति_अन्तर्_UInt() && constp->गन्तव्यम्_UInt() <= सीमा) प्रत्यागम सत्य;
    nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << what << " exceeds implementation limit of "
                                                 << सीमा
                                                 << (configured_सीमा <= SVA_मानम्_HARD_सीमा
                                                         ? " (--max-num-width)"
                                                         : " (host arithmetic limit)"));
    प्रत्यागम असत्य;
}

// The bound of a cycle delay in a sequence, if a non-negative elaboration-time constant
स्थिर ध्रुव AstConst* checkSvaDelayBound(Ast_ग्रन्थिः* errp, Ast_ग्रन्थिः_व्यञ्जकम्* boundp, ध्रुव वर्ण* what) {
    ध्रुव AstConst* ध्रुव constp = VN_CAST(boundp, Const);
    यदि (!constp || (constp->dtypep()->isSigned() && constp->अङ्कः().isNegative())) {
        errp->v3error(what << " is not a non-negative elaboration-time constant"
                              " (IEEE 1800-2023 16.7)");
        प्रत्यागम शून्यसूचकः;
    }
    यदि (!V3AssertCommon::check_Sva_मानम्_सीमा(errp, constp, "SVA cycle delay")) प्रत्यागम शून्यसूचकः;
    प्रत्यागम constp;
}

// Fold the bounds of a cycle delay in a sequence, and return if they are valid, else report it
// on errp
स्थिर तार्किक sva_आवृत्तिः_Delay_Valid(ध्रुव AstDelay* nodep, Ast_ग्रन्थिः* errp) {
    V3Const::constifyParamsNoWarnEdit(nodep->lhsp());
    ध्रुव AstConst* ध्रुव minConstp = checkSvaDelayBound(
        errp, nodep->lhsp(), nodep->isRangeDelay() ? "Range delay minimum" : "Delay value");
    यदि (!nodep->rhsp() || nodep->isUnbounded()) प्रत्यागम minConstp != शून्यसूचकः;
    V3Const::constifyParamsNoWarnEdit(nodep->rhsp());
    ध्रुव AstConst* ध्रुव maxConstp
        = checkSvaDelayBound(errp, nodep->rhsp(), "Range delay maximum");
    यदि (!minConstp || !maxConstp) प्रत्यागम असत्य;
    यदि (maxConstp->गन्तव्यम्_UInt() < minConstp->गन्तव्यम्_UInt()) {
        errp->v3error("Range delay maximum must be >= minimum (IEEE 1800-2023 16.7)");
        प्रत्यागम असत्य;
    }
    प्रत्यागम सत्य;
}

शून्य V3AssertCommon::check_Sva_आवृत्तिः_Delay(AstDelay* nodep, Ast_ग्रन्थिः* errp) {
    यदि (sva_आवृत्तिः_Delay_Valid(nodep, errp)) प्रत्यागम;
    // Make the delay valid, so later steps do not fail after the error
    File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव minp = nodep->lhsp();
    minp->replaceWith(नूतन AstConst{flp, AstConst::BitFalseErroring{}});
    VL_DO_DANGLING(minp->delete_वृक्षः(), minp);
    यदि (nodep->rhsp() && !nodep->isUnbounded()) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव maxp = nodep->rhsp();
        maxp->replaceWith(नूतन AstConst{flp, AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(maxp->delete_वृक्षः(), maxp);
    }
}

तार्किक V3AssertCommon::refsSvaFormal(ध्रुव Ast_ग्रन्थिः* nodep) {
    प्रत्यागम nodep->exists([](ध्रुव Ast_ग्रन्थिः_Var_Ref* refp) {
        ध्रुव AstVar* ध्रुव varp = refp->varp();
        यदि (!varp || !varp->isIO()) प्रत्यागम असत्य;
        ध्रुव Ast_ग्रन्थिः* ध्रुव declp = varp->aboveLoopp();
        प्रत्यागम VN_IS(declp, Sequence) || VN_IS(declp, Property);
    });
}

शून्य V3AssertCommon::checkSvaInstanceDelays(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* instp) {
    // A delay still referencing a formal argument of an enclosing sequence or property is
    // checked when that is instanced. Collect first, as checking edits the delays.
    std::सदिशम्<AstDelay*> delayps;
    nodep->foreach([&](AstDelay* delayp) {
        यदि (!delayp->is_आवृत्तिः_Delay() || refsSvaFormal(delayp->lhsp())) प्रत्यागम;
        यदि (delayp->rhsp() && refsSvaFormal(delayp->rhsp())) प्रत्यागम;
        delayps.प्रक्षिप_पृष्ठभागः(delayp);
    });
    प्रति (AstDelay* ध्रुव delayp : delayps) check_Sva_आवृत्तिः_Delay(delayp, instp);
}

//######################################################################
// AssertDeFutureVisitor
// If any AstFuture, then move all non-future varrefs to be one cycle behind,
// see IEEE 1800-2023 16.9.4.

वर्ग AssertDeFutureVisitor final : सार्वजनिक VNVisitor {
    // STATE - across all visitors
    Ast_ग्रन्थिः_Module* ध्रुव m_modp;  // Module future is underneath
    ध्रुव AstFuture* m_futurep;  // First AstFuture found
    ध्रुव अचिह्नित m_past_अङ्कः;  // Prefix unique number for this module
    std::map<AstVar*, AstVar*> m_delayedVars;  // Old to delayed variable mapping
    // STATE - for current visit position (use VL_RESTORER)
    तार्किक m_अन्तर्_Future = असत्य;  // Inside a future
    तार्किक m_unsupported = असत्य;  // Printed unsupported

    // METHODS
    शून्य unsupported(Ast_ग्रन्थिः* nodep) {
        यदि (m_unsupported) प्रत्यागम;
        m_unsupported = सत्य;
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported/illegal: Future value function used with expression with "
                          << nodep->prettyOperatorName());
    }
    // VISITORS
    शून्य दर्शय(AstFuture* nodep) override {
        VL_RESTORER(m_अन्तर्_Future);
        m_अन्तर्_Future = सत्य;
        iterateChildren(nodep);
        // Done with the future, this subexpression is current-time
        nodep->replaceWith(nodep->exprp()->unlink_Fr_पृष्ठभागः());
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;
        यदि (m_अन्तर्_Future || m_unsupported)
            प्रत्यागम;  // Need user1 set above, don't process when Future is removed
        यदि (nodep->access().isWriteOrRW()) {
            unsupported(nodep);
            प्रत्यागम;
        }
        स्वचालित it = m_delayedVars.find(nodep->varp());
        AstVar* outvarp;
        यदि (it == m_delayedVars.अन्तः_2()) {
            Ast_Sen_वृक्षः* ध्रुव sentreep = m_futurep->sentreep();
            AstAlways* ध्रुव alwaysp = नूतन AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS,
                                                     sentreep->clone_वृक्षः(असत्य), शून्यसूचकः};
            m_modp->सङ्कलय_Stmtsp(alwaysp);
            outvarp = नूतन AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                 "__Vnotfuture" + cvt_गन्तव्यम्_Str(m_past_अङ्कः) + "_" + nodep->name(),
                                 nodep->dtypep()};
            m_modp->सङ्कलय_Stmtsp(outvarp);
            AstVarRef* varRefAWritep = नूतन AstVarRef{nodep->fileline(), outvarp, VAccess::WRITE};
            varRefAWritep->user1(सत्य);
            Ast_ग्रन्थिः_Var_Ref* varRefAReadp = nodep->clone_वृक्षः(असत्य);
            varRefAReadp->user1(सत्य);
            Ast_ग्रन्थिः* ध्रुव assp = नूतन Ast_नियोजय_Dly{nodep->fileline(), varRefAWritep, varRefAReadp};
            alwaysp->सङ्कलय_Stmtsp(assp);
            m_delayedVars.emplace(nodep->varp(), outvarp);
        } अन्यथा {
            outvarp = it->second;
        }
        AstVarRef* newp = नूतन AstVarRef{nodep->fileline(), outvarp, VAccess::READ};
        newp->user1(सत्य);
        UINFO(9, "DeFuture " << nodep << "  becomes " << newp);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override { unsupported(nodep); }
    शून्य दर्शय(Ast_विधानम्_Call* nodep) override { unsupported(nodep); }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        यदि (!nodep->is_विशुद्धम्()) unsupported(nodep);
        iterateChildren(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट AssertDeFutureVisitor(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_Module* modp, अचिह्नित past_अङ्कः)
        : m_modp{modp}
        , m_past_अङ्कः{past_अङ्कः} {
        // See if any Future before we process
        यदि (nodep->forall([&](ध्रुव AstFuture* futurep) -> तार्किक {
                m_futurep = futurep;
                प्रत्यागम असत्य;
            }))
            प्रत्यागम;
        // UINFOTREE(9, nodep, "", "defuture-in");
        दर्शय(nodep);  // Nodep may get deleted
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
        यदि (nodep->dtypep()->isFourstate()) constp->अङ्कः().समुच्चयः_All_द्वयङ्काः_X();
        constp->dtypeFrom(nodep);
        प्रत्यागम constp;
    }

    // VISITORS
    शून्य दर्शय(AstCMethodHard* nodep) override {
        यदि (nodep->विधानम्() == VCMethod::EVENT_IS_TRIGGERED) {
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        } अन्यथा {
            iterateChildren(nodep);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        AstVar* ध्रुव varp = nodep->varp();
        यदि (varp->isParam()
            || (!varp->isNet() && varp->lifetime().isStatic() && varp->hasUserInit())) {
            प्रत्यागम;
        }
        nodep->replaceWith(newTypeDefault(nodep));
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    स्पष्ट Default_Sampled_मानम्_Visitor(Ast_ग्रन्थिः* nodep) { iterate(nodep); }
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
    ध्रुव VNUser1_अन्तर्_Use m_user1_अन्तर्_Use;
    ध्रुव VNUser2_अन्तर्_Use m_user2_अन्तर्_Use;
    AstUser2Allocator<AstAlways, std::सदिशम्<AstVar*>> m_delayed;

    // STATE
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Last module
    ध्रुव Ast_ग्रन्थिः* m_beginp = शून्यसूचकः;  // Last AstBegin/AstGenBlock
    अचिह्नित m_monitor_अङ्कः = 0;  // Global $monitor numbering (not per module)
    AstVar* m_monitor_अङ्कः_Varp = शून्यसूचकः;  // $monitor number variable
    AstVar* m_monitorOffVarp = शून्यसूचकः;  // $monitoroff variable
    अचिह्नित m_mod_Past_अङ्कः = 0;  // Module past numbering
    अचिह्नित m_mod_Strobe_अङ्कः = 0;  // Module $strobe numbering
    Ast_ग्रन्थिः_Procedure* m_procedurep = शून्यसूचकः;  // Current procedure
    VDouble0 m_statCover;  // Statistic tracking
    VDouble0 m_statAsNotImm;  // Statistic tracking
    VDouble0 m_statAsImm;  // Statistic tracking
    VDouble0 m_stat_As_पूर्णम्;  // Statistic tracking
    VDouble0 m_statPastVars;  // Statistic tracking
    VDouble0 m_statAssertOnCombined;  // Statistic tracking
    VDouble0 m_statAssertOnHoisted;  // Statistic tracking
    तार्किक m_अन्तर्_Sampled = असत्य;  // True inside a sampled expression
    तार्किक m_अन्तर्_Restrict = असत्य;  // True inside restrict assertion
    Ast_ग्रन्थिः* m_passsp = शून्यसूचकः;  // Current pass statement
    Ast_ग्रन्थिः* m_failsp = शून्यसूचकः;  // Current fail statement
    Ast_ग्रन्थिः_Cover_Or_Assert* m_assertp = शून्यसूचकः;  // Current assertion
    AstFinal* m_finalp = शून्यसूचकः;  // Current final block
    VDouble0 m_statLiftedCaseExprs;  // Count of purified case expressions
    Ast_ग्रन्थिः_FTask* m_ftaskp = शून्यसूचकः;  // Current function/task
    V3_Unique_नामानि m_case_अस्थायी_नामानि{"__VCase"};
    V3_Unique_नामानि m_मेलनम्_गणनम्_नामानि{"__VnfaRemainingMatchCount"};  // Match replay counter names
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
            यदि (v3Global.opt.assertCase()) {
                प्रत्यागम नूतन AstCExpr{fl, AstCExpr::विशुद्धम्{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            // If assertions are off, have constant propagation rip them out later
            // This allows syntax errors and such to be detected normally.
            प्रत्यागम नूतन AstConst{fl, AstConst::BitFalse{}};
        }
        पक्ष VAssertDirectiveType::ASSERT:
        पक्ष VAssertDirectiveType::COVER:
        पक्ष VAssertDirectiveType::ASSUME: {
            यदि (v3Global.opt.assertOn()) {
                प्रत्यागम नूतन AstCExpr{
                    fl, AstCExpr::विशुद्धम्{},
                    V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
            }
            प्रत्यागम नूतन AstConst{fl, AstConst::BitFalse{}};
        }
        पक्ष VAssertDirectiveType::आन्तरिकम्:
        पक्ष VAssertDirectiveType::VIOLATION_IF:
        पक्ष VAssertDirectiveType::RESTRICT: {
            यदि (v3Global.opt.assertOn()) {
                प्रत्यागम नूतन AstCExpr{fl, AstCExpr::विशुद्धम्{}, "vlSymsp->_vm_contextp__->assertOn()",
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
        यदि (!isControlled(directiveType)) प्रत्यागम नूतन AstConst{fl, AstConst::BitTrue{}};
        यदि (!v3Global.opt.assertOn()) प्रत्यागम नूतन AstConst{fl, AstConst::BitFalse{}};
        प्रत्यागम नूतन AstCExpr{
            fl, AstCExpr::विशुद्धम्{},
            V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType), 1};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* assertFailOnCond(File_पङ्क्तिः* fl, VAssertType type,
                                         VAssertDirectiveType directiveType) {
        यदि (!isControlled(directiveType)) प्रत्यागम नूतन AstConst{fl, AstConst::BitTrue{}};
        यदि (!v3Global.opt.assertOn()) प्रत्यागम नूतन AstConst{fl, AstConst::BitFalse{}};
        प्रत्यागम नूतन AstCExpr{
            fl, AstCExpr::विशुद्धम्{},
            V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType), 1};
    }
    string assert_Display_सन्देशः(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव string& उपसर्गः, ध्रुव string& सन्देशः,
                                VDisplayType severity) {
        यदि (severity == VDisplayType::DT_ERROR || severity == VDisplayType::DT_FATAL) {
            प्रत्यागम ("[%0t] "s + उपसर्गः + ": " + nodep->fileline()->filebasename() + ":"
                    + cvt_गन्तव्यम्_Str(nodep->fileline()->lineno()) + ": Assertion failed in %m"
                    + ((सन्देशः != "") ? ": " : "") + सन्देशः + "\n");
        }
        प्रत्यागम ("[%0t] "s + उपसर्गः + ": " + nodep->fileline()->filebasename() + ":"
                + cvt_गन्तव्यम्_Str(nodep->fileline()->lineno()) + ": %m" + ((सन्देशः != "") ? ": " : "")
                + सन्देशः + "\n");
    }
    शून्य replaceDisplay(AstDisplay* nodep, ध्रुव string& उपसर्गः) {
        nodep->fmtp()->text(
            assert_Display_सन्देशः(nodep, उपसर्गः, nodep->fmtp()->text(), nodep->displayType()));
        nodep->displayType(VDisplayType::DT_WRITE);
        // cppcheck-suppress nullPointer
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव timenewp = नूतन AstTime{nodep->fileline(), m_modp->timeunit()};
        nodep->fmtp()->prependExprsp(timenewp);
        यदि (!nodep->fmtp()->scopeNamep() && nodep->fmtp()->formatScopeTracking()) {
            nodep->fmtp()->scopeNamep(नूतन AstScopeName{nodep->fileline(), सत्य});
        }
    }
    AstSampled* new_Sampled_व्यञ्जकम्(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        प्रत्यागम नूतन AstSampled{nodep->fileline(), nodep, nodep->dtypep(), सत्य};
    }
    AstVarRef* new_Monitor_अङ्कः_Var_Refp(ध्रुव Ast_ग्रन्थिः* nodep, VAccess access) {
        यदि (!m_monitor_अङ्कः_Varp) {
            m_monitor_अङ्कः_Varp = नूतन AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorNum",
                                          nodep->findUInt64DType()};
            v3Global.rootp()->dollarUnitPkgp()->सङ्कलय_Stmtsp(m_monitor_अङ्कः_Varp);
        }
        AstVarRef* ध्रुव varrefp = नूतन AstVarRef{nodep->fileline(), m_monitor_अङ्कः_Varp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        प्रत्यागम varrefp;
    }
    AstVarRef* newMonitorOffVarRefp(ध्रुव Ast_ग्रन्थिः* nodep, VAccess access) {
        यदि (!m_monitorOffVarp) {
            m_monitorOffVarp = नूतन AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorOff",
                                          nodep->findBitDType()};
            v3Global.rootp()->dollarUnitPkgp()->सङ्कलय_Stmtsp(m_monitorOffVarp);
        }
        AstVarRef* ध्रुव varrefp = नूतन AstVarRef{nodep->fileline(), m_monitorOffVarp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        प्रत्यागम varrefp;
    }
    स्थिर AstIf* newIfAssertOn(Ast_ग्रन्थिः* bodyp, VAssertDirectiveType directiveType,
                                VAssertType type = VAssertType::आन्तरिकम्) {
        // Add an internal if to check assertions are on.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_पङ्क्तिः* ध्रुव fl = bodyp->fileline();

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = assertOnCond(fl, type, directiveType);
        AstIf* ध्रुव newp = नूतन AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(सत्य);  // To avoid LATCH warning
        newp->user2(सत्य);  // Mark as an assertOn() check
        प्रत्यागम newp;
    }
    स्थिर Ast_ग्रन्थिः_Stmt* newIfAssertPassOn(Ast_ग्रन्थिः* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type, तार्किक vacuous) {
        // Add an internal if to check assertion passOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_पङ्क्तिः* ध्रुव fl = bodyp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = assertPassOnCond(fl, type, directiveType, vacuous);
        Ast_ग्रन्थिः_If* ध्रुव newp = नूतन AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(सत्य);  // To avoid LATCH warning
        newp->user1(सत्य);  // Don't assert/cover this if
        newp->user2(सत्य);  // Mark as an assertOn() check
        प्रत्यागम newp;
    }
    स्थिर Ast_ग्रन्थिः_Stmt* newIfAssertFailOn(Ast_ग्रन्थिः* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type) {
        // Add an internal if to check assertion failOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_पङ्क्तिः* ध्रुव fl = bodyp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = assertFailOnCond(fl, type, directiveType);
        Ast_ग्रन्थिः_If* ध्रुव newp = नूतन AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(सत्य);  // To avoid LATCH warning
        newp->user1(सत्य);  // Don't assert/cover this if
        newp->user2(सत्य);  // Mark as an assertOn() check
        प्रत्यागम newp;
    }

    स्थिर AstIf* assertCond(ध्रुव Ast_ग्रन्थिः_Cover_Or_Assert* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* propp,
                             Ast_ग्रन्थिः* passsp, Ast_ग्रन्थिः* failsp) {
        AstIf* ध्रुव ifp = नूतन AstIf{nodep->fileline(), propp, passsp, failsp};
        // It's more LIKELY that we'll take the nullptr if clause
        // than the sim-killing else clause:
        ifp->branchPred(VBranchPred::BP_LIKELY);
        ifp->isBoundsCheck(सत्य);  // To avoid LATCH warning
        प्रत्यागम ifp;
    }

    Ast_ग्रन्थिः_Stmt* assertBody(ध्रुव Ast_ग्रन्थिः_Cover_Or_Assert* nodep, Ast_ग्रन्थिः* propp, Ast_ग्रन्थिः* passsp,
                            Ast_ग्रन्थिः* failsp) {
        यदि (AstPExpr* ध्रुव pexprp = VN_CAST(propp, PExpr)) {
            यदि (!v3Global.opt.timing().is_समुच्चयः_True()) {
                nodep->v3warn(E_NOTIMING, "This property expression requires --timing");
                VL_DO_DANGLING(प्रक्षिप_Deletep(pexprp), pexprp);
                प्रत्यागम नूतन AstBegin{nodep->fileline(), "", शून्यसूचकः, असत्य};
            }
            AstFork* ध्रुव forkp = नूतन AstFork{nodep->fileline(), VJoinType::JOIN_NONE};
            forkp->सङ्कलय_Forksp(pexprp->bodyp()->unlink_Fr_पृष्ठभागः());
            यदि (Ast_ग्रन्थिः_Stmt* ध्रुव finalp = pexprp->finalp()) {
                यदि (!m_finalp) {
                    m_finalp = नूतन AstFinal{m_modp->fileline(), finalp->unlink_Fr_पृष्ठभागः()};
                    m_modp->सङ्कलय_Stmtsp(m_finalp);
                } अन्यथा {
                    m_finalp->सङ्कलय_Stmtsp(finalp->unlink_Fr_पृष्ठभागः());
                }
            }
            VL_DO_DANGLING2(प्रक्षिप_Deletep(pexprp), pexprp, propp);
            प्रत्यागम forkp;
        }

        प्रत्यागम assertCond(nodep, VN_AS(propp, ग्रन्थिः_व्यञ्जकम्), passsp, failsp);
    }

    Ast_ग्रन्थिः_Stmt* newFireAssertUnchecked(ध्रुव Ast_ग्रन्थिः_Stmt* nodep, ध्रुव string& सन्देशः,
                                        Ast_ग्रन्थिः_व्यञ्जकम्* exprsp = शून्यसूचकः) {
        // Like newFireAssert() but omits the asserts-on check
        AstDisplay* ध्रुव dispp
            = नूतन AstDisplay{nodep->fileline(), VDisplayType::DT_ERROR, सन्देशः, शून्यसूचकः, शून्यसूचकः};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        Ast_ग्रन्थिः_Stmt* ध्रुव bodysp = dispp;
        replaceDisplay(dispp, "%%Error");  // Convert to standard DISPLAY format
        यदि (exprsp) dispp->fmtp()->exprsp()->सङ्कलय_अग्र(exprsp);
        यदि (v3Global.opt.stopFail()) bodysp->सङ्कलय_अग्र(नूतन AstStop{nodep->fileline(), असत्य});
        प्रत्यागम bodysp;
    }

    Ast_ग्रन्थिः_Stmt* newFireAssert(ध्रुव Ast_ग्रन्थिः_Stmt* nodep, VAssertDirectiveType directiveType,
                               VAssertType assertType, ध्रुव string& सन्देशः,
                               Ast_ग्रन्थिः_व्यञ्जकम्* exprsp = शून्यसूचकः) {
        Ast_ग्रन्थिः_Stmt* bodysp = newFireAssertUnchecked(nodep, सन्देशः, exprsp);
        bodysp = newIfAssertOn(bodysp, directiveType, assertType);
        प्रत्यागम bodysp;
    }

    AstVar* createDelayedVar(ध्रुव std::string& name, AstAlways* alwaysp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp,
                             Ast_ग्रन्थिः_व्यञ्जकम्* initp) {
        File_पङ्क्तिः* ध्रुव flp = exprp->fileline();
        AstVar* ध्रुव varp = नूतन AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        // TODO: this lifetime seems nonsene (can't have NBAs to automatics), but is as before
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->सङ्कलय_Stmtsp(varp);
        // Before enough clock ticks, $past returns the expression's default sampled value.
        यदि (initp) {
            m_modp->सङ्कलय_Stmtsp(नूतन AstInitialStatic{
                flp, नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, varp, VAccess::WRITE}, initp}});
        }
        ++m_statPastVars;
        // Actually set the delayed value
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = नूतन AstVarRef{flp, varp, VAccess::WRITE};
        Ast_नियोजय_Dly* ध्रुव assignp = नूतन Ast_नियोजय_Dly{flp, lhsp, exprp};
        alwaysp->prependStmtsp(assignp);
        प्रत्यागम varp;
    }

    AstAlways* getDelayedAlways(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* initp, Ast_Sen_वृक्षः* senTreep) {
        स्वचालित& expr2Sen2Alwaysr
            = initp ? m_modExpr2Sen2DefaultDelayedAlwaysp : m_modExpr2Sen2DelayedAlwaysp;
        AstAlways*& alwayspr = expr2Sen2Alwaysr[*exprp][*senTreep];
        यदि (!alwayspr) {
            File_पङ्क्तिः* ध्रुव flp = exprp->fileline();
            // Create the always block that computes the delayed values
            alwayspr = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, शून्यसूचकः};
            m_modp->सङ्कलय_Stmtsp(alwayspr);
            // Create the once-delayed variable
            ध्रुव std::string name = "_Vpast_" + cvt_गन्तव्यम्_Str(m_mod_Past_अङ्कः++) + "_1";
            AstVar* ध्रुव varp = createDelayedVar(name, alwayspr, exprp, initp);
            // Add it to delayed variable vector
            m_delayed(alwayspr).emplace_पृष्ठभागः(varp);
        } अन्यथा {
            // Reusing exiting, not needed
            VL_DO_DANGLING(प्रक्षिप_Deletep(exprp), exprp);
            यदि (initp) VL_DO_DANGLING(प्रक्षिप_Deletep(initp), initp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(senTreep), senTreep);
        }
        प्रत्यागम alwayspr;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* get_Past_मानम्(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* initp, Ast_Sen_वृक्षः* senTreep,
                              uint32_t ticks) {
        UASSERT_OBJ(ticks > 0, exprp, "Delay must be > 0");
        File_पङ्क्तिः* ध्रुव flp = exprp->fileline();
        AstAlways* ध्रुव alwaysp
            = getDelayedAlways(exprp, initp ? initp->clone_वृक्षः_विशुद्धम्(असत्य) : शून्यसूचकः, senTreep);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव resultp = past_मानम्_Ref(alwaysp, flp, ticks, initp);
        यदि (initp) VL_DO_DANGLING(प्रक्षिप_Deletep(initp), initp);
        प्रत्यागम resultp;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* past_मानम्_Ref(AstAlways* alwaysp, File_पङ्क्तिः* flp, uint32_t ticks,
                              Ast_ग्रन्थिः_व्यञ्जकम्* initp) {
        std::सदिशम्<AstVar*>& delayedr = m_delayed(alwaysp);
        // Ensure the required delay exists
        यावत् (delayedr.आकार() < ticks) {
            AstVar* ध्रुव firstp = delayedr.front();
            File_पङ्क्तिः* ध्रुव varFlp = firstp->fileline();
            // Create once more delayed value
            std::string name = firstp->name();
            name.resize(name.आकार() - 1);
            name += std::गन्तव्यम्_string(delayedr.आकार() + 1);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव prevp = नूतन AstVarRef{varFlp, delayedr.पृष्ठभागः(), VAccess::READ};
            AstVar* ध्रुव varp = createDelayedVar(name, alwaysp, prevp,
                                                  initp ? initp->clone_वृक्षः_विशुद्धम्(असत्य) : शून्यसूचकः);
            // Add it to delayed variable vector
            delayedr.emplace_पृष्ठभागः(varp);
        }
        // Return a reference to the appropriately delayed variable
        प्रत्यागम नूतन AstVarRef{flp, delayedr.at(ticks - 1), VAccess::READ};
    }

    // Time of the pipeline's last sampling tick, for end-of-simulation readers
    AstVar* getPastTickTimeVar(AstAlways* alwaysp) {
        AstVar*& varpr = m_delayedAlways2TickTimep[alwaysp];
        यदि (!varpr) {
            File_पङ्क्तिः* ध्रुव flp = alwaysp->fileline();
            varpr = नूतन AstVar{flp, VVarType::MODULETEMP,
                               "_Vpast_" + cvt_गन्तव्यम्_Str(m_mod_Past_अङ्कः++) + "_t",
                               m_modp->findUInt64DType()};
            m_modp->सङ्कलय_Stmtsp(varpr);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव timep = नूतन AstCExpr{flp, "vlSymsp->_vm_contextp__->time()", 64};
            alwaysp->सङ्कलय_Stmtsp(
                नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, varpr, VAccess::WRITE}, timep});
        }
        प्रत्यागम varpr;
    }

    शून्य दर्शय_Assertion_Iterate(Ast_ग्रन्थिः_Cover_Or_Assert* nodep, Ast_ग्रन्थिः* failsp) {
        यदि (m_beginp && nodep->name() == "") nodep->name(m_beginp->name());

        { AssertDeFutureVisitor{nodep->propp(), m_modp, m_mod_Past_अङ्कः++}; }

        iterate_And_अग्र_Null(nodep->sentreep());
        यदि (AstAssert* ध्रुव assertp = VN_CAST(nodep, Assert)) {
            iterate_And_अग्र_Null(assertp->failsp());
        } अन्यथा यदि (AstAssertIntrinsic* ध्रुव assertp = VN_CAST(nodep, AssertIntrinsic)) {
            iterate_And_अग्र_Null(assertp->failsp());
        } अन्यथा यदि (AstCover* ध्रुव coverp = VN_CAST(nodep, Cover)) {
            iterate_And_अग्र_Null(coverp->coverincsp());
        } अन्यथा यदि (!VN_IS(nodep, Restrict)) {
            nodep->v3fatalSrc("Unhandled assert type");
        }
        iterate_And_अग्र_Null(nodep->passsp());
        Ast_Sen_वृक्षः* sentreep = nodep->sentreep();
        यदि (nodep->तात्कालिकम्()) {
            UASSERT_OBJ(!sentreep, nodep, "Immediate assertions don't have sensitivity");
        } अन्यथा {
            UASSERT_OBJ(sentreep, nodep, "Concurrent assertions must have sensitivity");
            // Explicit inline clock differs from the enclosing always: hoist
            // and warn. To support this need queue of asserts to activate.
            यदि (m_procedurep && !nodep->senFromAlways()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Procedural concurrent assertion with"
                              " clocking event inside always (IEEE 1800-2023 16.14.6)");
                static_cast<Ast_ग्रन्थिः*>(m_procedurep)->सङ्कलय_अग्र(nodep->unlink_Fr_पृष्ठभागः());
                प्रत्यागम;  // Later iterate will pick up
            }
            sentreep->unlink_Fr_पृष्ठभागः();
            यदि (m_procedurep) { VL_DO_DANGLING(प्रक्षिप_Deletep(sentreep), sentreep); }
        }
        //
        ध्रुव string& सन्देशः = nodep->name();
        Ast_ग्रन्थिः* passsp = nodep->passsp();
        यदि (passsp) passsp->unlink_Fr_पृष्ठभागः_With_अग्र();
        यदि (AstAssert* ध्रुव assertp = VN_CAST(nodep, Assert)) {
            failsp = assertp->failsp();
        } अन्यथा यदि (AstAssertIntrinsic* ध्रुव assertp = VN_CAST(nodep, AssertIntrinsic)) {
            failsp = assertp->failsp();
        }
        यदि (failsp) failsp->unlink_Fr_पृष्ठभागः_With_अग्र();

        तार्किक selfDestruct = असत्य;
        तार्किक passspGated = असत्य;
        ध्रुव AstCover* ध्रुव coverp = VN_CAST(nodep, Cover);
        // A sequence event control is not an assertion directive; no assertion control
        ध्रुव तार्किक seqEvent = coverp && coverp->isSeqEvent();
        यदि (coverp) {
            ++m_statCover;
            यदि (seqEvent) {
                // Keep the event-fire action, with no coverage bucket
            } अन्यथा यदि (!v3Global.opt.coverageUser()) {
                selfDestruct = सत्य;
            } अन्यथा {
                // V3Coverage assigned us a bucket to increment.
                AstCoverInc* ध्रुव covincp = VN_AS(coverp->coverincsp(), CoverInc);
                UASSERT_OBJ(covincp, coverp, "Missing AstCoverInc under assertion");
                covincp->unlink_Fr_पृष्ठभागः_With_अग्र();  // next() might have  AstAssign for trace
                यदि (सन्देशः != "") covincp->declp()->comment(सन्देशः);
                यदि (passsp) {
                    passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                               /*vacuous=*/असत्य);
                    passspGated = सत्य;
                    passsp = Ast_ग्रन्थिः::सङ्कलय_अग्र<Ast_ग्रन्थिः, Ast_ग्रन्थिः>(covincp, passsp);
                } अन्यथा {
                    passsp = covincp;
                }
            }
        } अन्यथा यदि (VN_IS(nodep, Assert) || VN_IS(nodep, AssertIntrinsic)) {
            यदि (nodep->तात्कालिकम्()) {
                ++m_statAsImm;
            } अन्यथा {
                ++m_statAsNotImm;
            }
            यदि (!passsp && !failsp)
                failsp = newFireAssertUnchecked(
                    nodep, VN_IS(nodep, AssertIntrinsic) ? "'$cast' failed." : "'assert' failed.");
        } अन्यथा {
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
        Ast_ग्रन्थिः_व्यञ्जकम्* disablep = शून्यसूचकः;
        Ast_ग्रन्थिः_व्यञ्जकम्* मेलनम्_Countp = शून्यसूचकः;
        यदि (AstPropSpec* ध्रुव specp = VN_CAST(nodep->propp(), PropSpec)) {
            propExprp = specp->propp()->unlink_Fr_पृष्ठभागः();
            यदि (specp->disablep()) disablep = specp->disablep()->unlink_Fr_पृष्ठभागः();
            मेलनम्_Countp = specp->मेलनम्_Countp();
        } अन्यथा {
            propExprp = nodep->propp()->unlink_Fr_पृष्ठभागः();
        }
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        तार्किक passspAlreadyGated = असत्य;
        यदि (passsp && VN_IS(passsp, If)) passspAlreadyGated = VN_AS(passsp, If)->user1();
        यदि (passsp && !passspGated && !passspAlreadyGated && !VN_IS(propExprp, PExpr)
            && !seqEvent) {
            passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                       /*vacuous=*/असत्य);
        }
        यदि (failsp && !VN_IS(propExprp, PExpr)) {
            failsp = newIfAssertFailOn(failsp, nodep->directive(), nodep->userType());
        }
        यदि (coverp && मेलनम्_Countp && passsp) {
            // Convert the match count into a loop that decrements a temporary variable until it
            // reaches zero.
            मेलनम्_Countp->unlink_Fr_पृष्ठभागः();
            AstVar* ध्रुव remainingp = नूतन AstVar{
                flp, VVarType::MODULETEMP, m_मेलनम्_गणनम्_नामानि.get(""), मेलनम्_Countp->dtypep()};
            remainingp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->सङ्कलय_Stmtsp(remainingp);
            Ast_ग्रन्थिः* ध्रुव replaysp
                = नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, remainingp, VAccess::WRITE}, मेलनम्_Countp};
            AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
            loopp->सङ्कलय_Stmtsp(
                नूतन AstLoopTest{flp, loopp, नूतन AstVarRef{flp, remainingp, VAccess::READ}});
            loopp->सङ्कलय_Stmtsp(passsp);
            loopp->सङ्कलय_Stmtsp(
                नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, remainingp, VAccess::WRITE},
                              नूतन AstSub{flp, नूतन AstVarRef{flp, remainingp, VAccess::READ},
                                         नूतन AstConst{flp, AstConst::Widthed_मानम्{},
                                                      remainingp->dtypep()->विस्तारः(), 1}}});
            replaysp->सङ्कलय_अग्र(loopp);
            passsp = replaysp;
        }
        Ast_ग्रन्थिः* bodysp = assertBody(nodep, propExprp, passsp, failsp);
        यदि (disablep) bodysp = नूतन AstIf{flp, नूतन AstLogNot{flp, disablep}, bodysp};
        // Add assertOn check last, for better combining
        यदि (!seqEvent) bodysp = newIfAssertOn(bodysp, nodep->directive(), nodep->userType());
        यदि (sentreep) bodysp = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, sentreep, bodysp};

        यदि (passsp && !passsp->backp()) VL_DO_DANGLING(प्रक्षिप_Deletep(passsp), passsp);
        यदि (failsp && !failsp->backp()) VL_DO_DANGLING(प्रक्षिप_Deletep(failsp), failsp);

        // Install it
        यदि (selfDestruct) {
            // Delete it after making the tree.  This way we can tell the user
            // if it wasn't constructed nicely or has other errors without needing --coverage.
            VL_DO_DANGLING(bodysp->delete_वृक्षः(), bodysp);
            nodep->unlink_Fr_पृष्ठभागः();
        } अन्यथा {
            nodep->replaceWith(bodysp);
        }
        // Bye
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    तार्किक is_रिक्तानि_Stmt(Ast_ग्रन्थिः* nodep) {
        यदि (!nodep) प्रत्यागम सत्य;
        यदि (AstBegin* ध्रुव beginp = VN_CAST(nodep, Begin)) {
            यदि (beginp->declsp()) प्रत्यागम असत्य;
            प्रति (Ast_ग्रन्थिः* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                यदि (!is_रिक्तानि_Stmt(stmtp)) प्रत्यागम असत्य;
            }
            प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    // VISITORS
    शून्य दर्शय(AstIf* nodep) override {
        यदि (!nodep->user1_समुच्चयः_Once()) {
            यदि (nodep->uniquePragma() || nodep->unique0Pragma()) {
                ध्रुव Ast_ग्रन्थिः_If* ifp = nodep;
                Ast_ग्रन्थिः_व्यञ्जकम्* propp = शून्यसूचकः;
                तार्किक hasDefaultElse = असत्य;
                कुरु {
                    // If this statement ends with 'else if', then nextIf will point to the
                    // nextIf statement.  Otherwise it will be null.
                    ध्रुव Ast_ग्रन्थिः_If* ध्रुव nextifp = dynamic_cast<Ast_ग्रन्थिः_If*>(ifp->elsesp());
                    iterate_And_अग्र_Null(ifp->condp());

                    // Recurse into the true case.
                    iterate_And_अग्र_Null(ifp->thensp());

                    // If the last else is not an else if, recurse into that too.
                    यदि (ifp->elsesp() && !nextifp) {  //
                        iterate_And_अग्र_Null(ifp->elsesp());
                    }

                    // Build a bitmask of the true predicates
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव predp = ifp->condp()->clone_वृक्षः_विशुद्धम्(असत्य);
                    यदि (propp) {
                        propp = नूतन AstConcat{nodep->fileline(), predp, propp};
                    } अन्यथा {
                        propp = predp;
                    }

                    // Record if this ends with an 'else' that does not have an if
                    यदि (ifp->elsesp() && !nextifp) hasDefaultElse = सत्य;

                    ifp = nextifp;
                } यावत् (ifp);

                ध्रुव तार्किक allow_none = nodep->unique0Pragma();

                // Empty case means no property
                यदि (!propp) propp = नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}};

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
                nodep->unlink_Fr_पृष्ठभागः(&relinker);
                AstIf* ध्रुव checkifp
                    = नूतन AstIf{nodep->fileline(), नूतन AstLogNot{nodep->fileline(), ohot},
                                newFireAssert(nodep, VAssertDirectiveType::VIOLATION_IF,
                                              assertType, "'unique if' statement violated"),
                                nodep};
                checkifp->isBoundsCheck(सत्य);  // To avoid LATCH warning
                checkifp->branchPred(VBranchPred::BP_UNLIKELY);
                relinker.relink(checkifp);
                प्रत्यागम;
            }
        }

        iterateChildren(nodep);

        यदि (nodep->user2()) {
            // Combine consecutive assertOn checks if possible
            यदि (AstIf* ध्रुव backp = VN_CAST(nodep->backp(), If)) {
                यदि (backp->nextp() == nodep  //
                    && backp->user2()  //
                    && backp->condp()->same_वृक्षः(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    backp->सङ्कलय_Thensp(nodep->thensp()->unlink_Fr_पृष्ठभागः_With_अग्र());
                    nodep->unlink_Fr_पृष्ठभागः();
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    प्रत्यागम;
                }
            }
            // Combine nested assertOn checks if possible
            यदि (nodep->thensp() && !nodep->thensp()->nextp() && is_रिक्तानि_Stmt(nodep->elsesp())) {
                AstIf* ध्रुव checkp = VN_CAST(nodep->thensp(), If);
                यदि (checkp  //
                    && checkp->user2()  //
                    && checkp->condp()->same_वृक्षः(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    nodep->सङ्कलय_Thensp(checkp->thensp()->unlink_Fr_पृष्ठभागः_With_अग्र());
                    VL_DO_DANGLING(प्रक्षिप_Deletep(checkp->unlink_Fr_पृष्ठभागः()), checkp);
                    प्रत्यागम;
                }
            }
            प्रत्यागम;
        }

        // Swap assertOn check with single statement 'if' statement to bubble up for combining
        // Note we can't just swap the conditions as they two Ifs have different flags,
        // so swapping the Ifs themeselves then swapping back the bodies.
        यदि (nodep->condp()->is_विशुद्धम्()) {
            यदि (nodep->thensp() && !nodep->thensp()->nextp() && is_रिक्तानि_Stmt(nodep->elsesp())) {
                AstIf* ध्रुव checkp = VN_CAST(nodep->thensp(), If);
                यदि (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlink_Fr_पृष्ठभागः());
                    nodep->सङ्कलय_Thensp(checkp->thensp()->unlink_Fr_पृष्ठभागः_With_अग्र());
                    checkp->सङ्कलय_Thensp(nodep);
                    प्रत्यागम;
                }
            }
            यदि (nodep->elsesp() && !nodep->elsesp()->nextp() && is_रिक्तानि_Stmt(nodep->thensp())) {
                AstIf* ध्रुव checkp = VN_CAST(nodep->elsesp(), If);
                यदि (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlink_Fr_पृष्ठभागः());
                    nodep->सङ्कलय_Elsesp(checkp->thensp()->unlink_Fr_पृष्ठभागः_With_अग्र());
                    checkp->सङ्कलय_Thensp(nodep);
                    प्रत्यागम;
                }
            }
        }
    }

    //========== Case assertions
    शून्य दर्शय(AstCase* nodep) override {
        // Introduce temporary variable for AstCase if needed - it is done here and not in V3Case
        // because this phase is before V3Scope and V3Case is not. Doing it before V3Scope ensures
        // that V3Scope will take care of a scope creation
        // We also need to do it before V3Begin, co that pragmas like `unique0` also work correctly
        यदि (!nodep->exprp()->is_विशुद्धम्()) {
            ++m_statLiftedCaseExprs;
            File_पङ्क्तिः* ध्रुव fl = nodep->exprp()->fileline();
            AstVar* ध्रुव varp = नूतन AstVar{fl, VVarType::BLOCKTEMP, m_case_अस्थायी_नामानि.get(nodep),
                                            nodep->exprp()->dtypep()};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव origp = nodep->exprp()->unlink_Fr_पृष्ठभागः();
            nodep->सङ्कलय_अत्र_This_As_अग्र(
                नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, origp});
            nodep->exprp(नूतन AstVarRef{fl, varp, VAccess::READ});
            यदि (m_ftaskp) {
                varp->funcLocal(सत्य);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } अन्यथा {
                m_modp->prependStmtsp(varp);
            }
            VIsCached::clear_Cache_वृक्षः();
        }
        iterateChildren(nodep);
        यदि (!nodep->user1_समुच्चयः_Once()) {
            तार्किक has_default = असत्य;
            प्रति (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                यदि (itemp->isDefault()) has_default = सत्य;
            }
            ध्रुव Ast_ग्रन्थिः_DType* व्यञ्जकम्_Dtypep = nodep->exprp()->dtypep()->skipRefp();

            VAssertType assertType = VAssertType::आन्तरिकम्;
            यदि (nodep->प्राथमिकता_Pragma()) {
                assertType = VAssertType::प्राथमिकता;
            } अन्यथा यदि (nodep->uniquePragma()) {
                assertType = VAssertType::UNIQUE;
            } अन्यथा यदि (nodep->unique0Pragma()) {
                assertType = VAssertType::UNIQUE0;
            }

            string valFmt;
            यदि (व्यञ्जकम्_Dtypep->isIntegralOrPacked())
                valFmt = " for '" + cvt_गन्तव्यम्_Str(व्यञ्जकम्_Dtypep->विस्तारः_Min()) + "'h%X'";
            यदि (nodep->पूर्णम्_Pragma() || nodep->प्राथमिकता_Pragma()) {
                // Need to add a default if there isn't one already
                ++m_stat_As_पूर्णम्;
                यदि (!has_default) {
                    nodep->सङ्कलय_Itemsp(नूतन AstCaseItem{
                        nodep->fileline(), शून्यसूचकः /*DEFAULT*/,
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      nodep->pragmaString() + ", but non-match found" + valFmt,
                                      valFmt.रिक्तानि() ? शून्यसूचकः
                                                     : nodep->exprp()->clone_वृक्षः_विशुद्धम्(असत्य))});
                }
            }
            यदि (nodep->parallelPragma() || nodep->uniquePragma() || nodep->unique0Pragma()) {
                // Need to check that one, and only one of the case items match at any moment
                // If there's a default, we allow none to match, else exactly one must match
                ++m_stat_As_पूर्णम्;
                यदि (!has_default && !nodep->itemsp()) {
                    // Not parallel, but harmlessly so.
                } अन्यथा {
                    Ast_ग्रन्थिः_व्यञ्जकम्* propp = शून्यसूचकः;
                    प्रति (AstCaseItem* itemp = nodep->itemsp(); itemp;
                         itemp = VN_AS(itemp->nextp(), CaseItem)) {
                        Ast_ग्रन्थिः_व्यञ्जकम्* itembitp = शून्यसूचकः;
                        प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* icondp = itemp->condsp(); icondp;
                             icondp = VN_AS(icondp->nextp(), ग्रन्थिः_व्यञ्जकम्)) {
                            Ast_ग्रन्थिः_व्यञ्जकम्* onep;
                            यदि (AstInsideRange* ध्रुव rcondp = VN_CAST(icondp, InsideRange)) {
                                onep = rcondp->newAndFromInside(
                                    nodep->exprp()->clone_वृक्षः_विशुद्धम्(सत्य),
                                    rcondp->lhsp()->clone_वृक्षः_विशुद्धम्(सत्य),
                                    rcondp->rhsp()->clone_वृक्षः_विशुद्धम्(सत्य));
                            } अन्यथा यदि (nodep->casex() || nodep->casez() || nodep->caseInside()) {
                                onep = AstEqWild::newTyped(itemp->fileline(),
                                                           nodep->exprp()->clone_वृक्षः_विशुद्धम्(असत्य),
                                                           icondp->clone_वृक्षः_विशुद्धम्(असत्य));
                            } अन्यथा {
                                onep = AstEq::newTyped(icondp->fileline(),
                                                       nodep->exprp()->clone_वृक्षः_विशुद्धम्(असत्य),
                                                       icondp->clone_वृक्षः_विशुद्धम्(असत्य));
                            }
                            // OR together all conditions within the same case item
                            यदि (onep) {
                                यदि (itembitp) {
                                    itembitp = नूतन AstOr{icondp->fileline(), onep, itembitp};
                                } अन्यथा {
                                    itembitp = onep;
                                }
                            }
                        }
                        यदि (itembitp) {
                            यदि (propp) {
                                propp = नूतन AstConcat{itemp->fileline(), itembitp, propp};
                            } अन्यथा {
                                propp = itembitp;
                            }
                        }
                    }
                    // Empty case means no property
                    यदि (!propp) propp = नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    ध्रुव तार्किक allow_none = has_default || nodep->unique0Pragma();
                    // The following assertion looks as below.
                    // if (!$onehot(propp)) begin
                    //     if (propp == '0) begin if (!allow_none) $error("none match"); end
                    //     else $error("multiple match");
                    //अन्तः
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव ohot = नूतन AstOneHot{nodep->fileline(), propp};
                    AstConst* ध्रुव zero = नूतन AstConst{
                        nodep->fileline(), AstConst::Widthed_मानम्{}, propp->विस्तारः(), 0};
                    AstIf* ध्रुव ohotIfp
                        = नूतन AstIf{nodep->fileline(), नूतन AstLogNot{nodep->fileline(), ohot}};
                    AstIf* ध्रुव zeroIfp = नूतन AstIf{
                        nodep->fileline(),
                        नूतन AstEq{nodep->fileline(), propp->clone_वृक्षः_विशुद्धम्(असत्य), zero}};
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = nodep->exprp();
                    ध्रुव string pragmaStr = nodep->pragmaString();
                    यदि (!allow_none)
                        zeroIfp->सङ्कलय_Thensp(
                            newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                          pragmaStr + ", but none matched" + valFmt,
                                          valFmt.रिक्तानि() ? शून्यसूचकः : exprp->clone_वृक्षः_विशुद्धम्(असत्य)));
                    zeroIfp->सङ्कलय_Elsesp(
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      pragmaStr + ", but multiple matches found" + valFmt,
                                      valFmt.रिक्तानि() ? शून्यसूचकः : exprp->clone_वृक्षः_विशुद्धम्(असत्य)));
                    ohotIfp->सङ्कलय_Thensp(zeroIfp);
                    ohotIfp->isBoundsCheck(सत्य);  // To avoid LATCH warning
                    ohotIfp->branchPred(VBranchPred::BP_UNLIKELY);
                    nodep->सङ्कलय_Not_Parallelp(ohotIfp);
                }
            }
        }
    }

    शून्य दर्शय(AstFuture* nodep) override {
        nodep->v3error("Future sampled value function called outside property or sequence "
                       "expression (IEEE 16.9.4)");
        nodep->replaceWith(नूतन AstConst{nodep->fileline(), 0});
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    //========== Past
    शून्य दर्शय(AstPast* nodep) override {
        यदि (!nodep->propertyTiming()) {
            nodep->initp(nodep->exprp()->clone_वृक्षः_विशुद्धम्(असत्य));
            { Default_Sampled_मानम्_Visitor{nodep->initp()}; }
        }
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव initp = nodep->initp() ? nodep->initp()->unlink_Fr_पृष्ठभागः() : शून्यसूचकः;
        iterateChildren(nodep);
        uint32_t ticks = 1;
        यदि (nodep->ticksp()) {
            UASSERT_OBJ(VN_IS(nodep->ticksp(), Const), nodep,
                        "Expected constant ticks, checked in V3Width");
            ticks = VN_AS(nodep->ticksp(), Const)->गन्तव्यम्_UInt();
        }
        UASSERT_OBJ(ticks >= 1, nodep, "0 tick should have been checked in V3Width");
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = new_Sampled_व्यञ्जकम्(nodep->exprp()->unlink_Fr_पृष्ठभागः());
        Ast_Sen_वृक्षः* ध्रुव senTreep = nodep->sentreep()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* inp = शून्यसूचकः;
        // Deliberately emitted for every ftask $past; calls outside final take the normal stage
        यदि (VN_IS(m_procedurep, Final) || m_ftaskp) {
            // A sampling-tick finish reads one stage deeper (IEEE 1800-2023 16.9.3).
            File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
            AstAlways* ध्रुव alwaysp
                = getDelayedAlways(exprp, initp ? initp->clone_वृक्षः_विशुद्धम्(असत्य) : शून्यसूचकः, senTreep);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव betweenTicksp = past_मानम्_Ref(alwaysp, flp, ticks, initp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव onTickp = past_मानम्_Ref(alwaysp, flp, ticks + 1, initp);
            यदि (initp) VL_DO_DANGLING(प्रक्षिप_Deletep(initp), initp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव onTickCondp = नूतन AstLogAnd{
                flp, नूतन AstCExpr{flp, "vlSymsp->_vm_contextp__->executingFinal()", 1},
                नूतन AstEq{flp, नूतन AstVarRef{flp, getPastTickTimeVar(alwaysp), VAccess::READ},
                          नूतन AstCExpr{flp, "vlSymsp->_vm_contextp__->finishPendingTime()", 64}}};
            AstCond* ध्रुव condp = नूतन AstCond{flp, onTickCondp, onTickp, betweenTicksp};
            condp->dtypeFrom(onTickp);
            inp = condp;
        } अन्यथा {
            inp = get_Past_मानम्(exprp, initp, senTreep, ticks);
        }
        nodep->replaceWith(inp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    //========== Move $sampled down to read-only variables
    शून्य दर्शय(AstSampled* nodep) override {
        यदि (nodep->user1()) प्रत्यागम;
        VL_RESTORER(m_अन्तर्_Sampled);
        {
            m_अन्तर्_Sampled = सत्य;
            iterateChildren(nodep);
        }
        यदि (nodep->exprp()) {
            nodep->replaceWith(nodep->exprp()->unlink_Fr_पृष्ठभागः());
        } अन्यथा {
            nodep->unlink_Fr_पृष्ठभागः();
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        यदि (m_अन्तर्_Sampled && !nodep->user2() && !nodep->varp()->is_अस्थायी()) {
            यदि (!nodep->access().isReadOnly()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Write to variable in sampled expression");
            } अन्यथा {
                VNRelinker relinkHandle;
                nodep->unlink_Fr_पृष्ठभागः(&relinkHandle);
                AstSampled* ध्रुव newp = new_Sampled_व्यञ्जकम्(nodep);
                relinkHandle.relink(newp);
                newp->user1(1);
                v3Global.समुच्चयः_Has_Sampled();
            }
        }
    }
    // Don't sample sensitivities
    शून्य दर्शय(AstSenItem* nodep) override {
        VL_RESTORER(m_अन्तर्_Sampled);
        m_अन्तर्_Sampled = असत्य;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstPExprClause* nodep) override {
        Ast_ग्रन्थिः* stmtsp = शून्यसूचकः;
        यदि (nodep->pass() && m_passsp) {
            // Cover adds COVERINC by AstNode::addNext, thus need to clone next too.
            stmtsp = m_passsp->clone_वृक्षः(सत्य);
            stmtsp = newIfAssertPassOn(stmtsp, m_assertp->directive(), m_assertp->userType(),
                                       nodep->vacuous());
        } अन्यथा यदि (!nodep->pass() && m_failsp) {
            stmtsp = m_failsp->clone_वृक्षः(सत्य);
            stmtsp = newIfAssertFailOn(stmtsp, m_assertp->directive(), m_assertp->userType());
        }
        यदि (stmtsp) {
            stmtsp->foreach_And_अग्र([](Ast_ग्रन्थिः_Var_Ref* ध्रुव refp) {
                // References inside action blocks shouldn't be implicitly sampled
                // m_passsp/m_failsp have been already visited once and refs explicitly sampled
                // are handled already
                refp->user2(1);
            });
            nodep->replaceWith(stmtsp);
        } अन्यथा {
            nodep->unlink_Fr_पृष्ठभागः();
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstPExpr* nodep) override {
        यदि (m_अन्तर्_Restrict) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
        } अन्यथा {
            iterateChildren(nodep);
        }
    }

    //========== Statements
    शून्य दर्शय(AstDisplay* nodep) override {
        iterateChildren(nodep);
        // Replace the special types with standard text
        यदि (nodep->displayType() == VDisplayType::DT_INFO) {
            replaceDisplay(nodep, "-Info");
        } अन्यथा यदि (nodep->displayType() == VDisplayType::DT_WARNING) {
            replaceDisplay(nodep, "%%Warning");
        } अन्यथा यदि (nodep->displayType() == VDisplayType::DT_ERROR) {
            replaceDisplay(nodep, "%%Error");
        } अन्यथा यदि (nodep->displayType() == VDisplayType::DT_FATAL) {
            replaceDisplay(nodep, "%%Fatal");
        } अन्यथा यदि (nodep->displayType() == VDisplayType::DT_MONITOR) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            File_पङ्क्तिः* ध्रुव fl = nodep->fileline();

            AstSenItem* monSenItemsp = शून्यसूचकः;
            यदि (Ast_ग्रन्थिः* ध्रुव monExprsp = nodep->fmtp()->exprsp()) {
                monExprsp->foreach_And_अग्र([&](AstVarRef* varrefp) {
                    AstSenItem* ध्रुव senItemp
                        = नूतन AstSenItem{fl, VEdgeType::ET_परिवर्तितम्,
                                         // Clone so get VarRef or VarXRef as needed
                                         varrefp->clone_वृक्षः(असत्य)};
                    monSenItemsp = Ast_ग्रन्थिः::सङ्कलय_अग्र_Null(monSenItemsp, senItemp);
                });
            }

            Ast_Sen_वृक्षः* ध्रुव mon_Sen_वृक्षः = नूतन Ast_Sen_वृक्षः{fl, monSenItemsp};
            ध्रुव स्वचालित mon_अङ्कः = ++m_monitor_अङ्कः;
            // Where $monitor was we do "__VmonitorNum = N;"
            Ast_नियोजय* ध्रुव newsetp = नूतन Ast_नियोजय{
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
            ifp->isBoundsCheck(सत्य);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_ग्रन्थिः* ध्रुव newp = नूतन AstAlways{fl, VAlwaysKwd::ALWAYS, mon_Sen_वृक्षः, ifp};
            m_modp->सङ्कलय_Stmtsp(newp);
        } अन्यथा यदि (nodep->displayType() == VDisplayType::DT_STROBE) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            // Need one-shot
            File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
            AstVar* ध्रुव varp
                = नूतन AstVar{fl, VVarType::MODULETEMP, "__Vstrobe" + cvt_गन्तव्यम्_Str(m_mod_Strobe_अङ्कः++),
                             nodep->findBitDType()};
            m_modp->सङ्कलय_Stmtsp(varp);
            // Where $strobe was we do "__Vstrobe = '1;"
            Ast_नियोजय* ध्रुव newsetp = नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                                     नूतन AstConst{fl, AstConst::BitTrue{}}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__Vstrobe) begin $display(...); __Vstrobe = '0; end"
            Ast_ग्रन्थिः* ध्रुव stmtsp = nodep;
            AstIf* ध्रुव ifp = नूतन AstIf{fl, नूतन AstVarRef{fl, varp, VAccess::READ}, stmtsp};
            ifp->isBoundsCheck(सत्य);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_ग्रन्थिः* ध्रुव newp = नूतन AstAlwaysPostponed{fl, ifp};
            stmtsp->सङ्कलय_अग्र(नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                          नूतन AstConst{fl, AstConst::BitFalse{}}});
            m_modp->सङ्कलय_Stmtsp(newp);
        }
    }
    शून्य दर्शय(AstMonitorOff* nodep) override {
        Ast_नियोजय* ध्रुव newp
            = नूतन Ast_नियोजय{nodep->fileline(), newMonitorOffVarRefp(nodep, VAccess::WRITE),
                            नूतन AstConst{nodep->fileline(), AstConst::BitTrue{}, nodep->off()}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstAssert* nodep) override {  //
        दर्शय_Assertion_Iterate(nodep, nodep->failsp());
    }
    शून्य दर्शय(AstAssertCtl* nodep) override {
        iterateChildren(nodep);

        तार्किक assertTypeConst = सत्य;
        यदि (!nodep->assertTypesp()) {
            nodep->assertTypes(VAssertType{ALL_ASSERT_TYPES});
        } अन्यथा यदि (ध्रुव AstConst* ध्रुव assertTypesp = VN_CAST(nodep->assertTypesp(), Const)) {
            nodep->assertTypes(VAssertType{assertTypesp->गन्तव्यम्_SInt()});
        } अन्यथा {
            assertTypeConst = असत्य;
        }

        तार्किक controlTypeConst = असत्य;
        यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->controlTypep(), Const)) {
            nodep->ctlType(constp->गन्तव्यम्_SInt());
            controlTypeConst = सत्य;
        }
        यदि (controlTypeConst
            && (nodep->ctlType() < VAssertCtlType::तालकम्
                || nodep->ctlType() > VAssertCtlType::VACUOUS_OFF)) {
            nodep->unlink_Fr_पृष्ठभागः();
            nodep->v3error("Bad $assertcontrol control_type '"
                           << cvt_गन्तव्यम्_Str(static_cast<पूर्णाङ्क>(nodep->ctlType()))
                           << "' (IEEE 1800-2023 Table 20-5)");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम;
        }
        यदि (assertTypeConst && nodep->assertTypes() != ALL_ASSERT_TYPES
            && nodep->assertTypes().अन्तर्भवति_Any(VAssertType::UNIQUE | VAssertType::UNIQUE0
                                                | VAssertType::प्राथमिकता)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: assert control assertion_type");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
            प्रत्यागम;
        }

        तार्किक directiveTypeConst = सत्य;
        यदि (!nodep->directiveTypesp()) {
            nodep->directiveTypes(VAssertDirectiveType::ASSERT | VAssertDirectiveType::ASSUME
                                  | VAssertDirectiveType::COVER);
        } अन्यथा यदि (ध्रुव AstConst* ध्रुव directiveTypesp
                   = VN_CAST(nodep->directiveTypesp(), Const)) {
            nodep->directiveTypes(VAssertDirectiveType{directiveTypesp->गन्तव्यम्_SInt()});
        } अन्यथा {
            directiveTypeConst = असत्य;
        }
        यदि (!directiveTypeConst) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: non-const assert directive type expression");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
            प्रत्यागम;
        }

        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        UINFO(9, "Generating assertctl for a module: " << m_modp);
        AstCStmt* ध्रुव newp = नूतन AstCStmt{fl};
        newp->सङ्कलय("vlSymsp->_vm_contextp__->assertCtl(");
        newp->सङ्कलय(nodep->controlTypep()->unlink_Fr_पृष्ठभागः());
        newp->सङ्कलय(", ");
        यदि (nodep->assertTypesp()) {
            newp->सङ्कलय(nodep->assertTypesp()->unlink_Fr_पृष्ठभागः());
        } अन्यथा {
            newp->सङ्कलय(std::गन्तव्यम्_string(ALL_ASSERT_TYPES));
        }
        newp->सङ्कलय(", " + std::गन्तव्यम्_string(nodep->directiveTypes()) + ");\n");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstAssertIntrinsic* nodep) override {  //
        दर्शय_Assertion_Iterate(nodep, nodep->failsp());
    }
    शून्य दर्शय(AstCover* nodep) override {  //
        दर्शय_Assertion_Iterate(nodep, शून्यसूचकः);
    }
    शून्य दर्शय(AstRestrict* nodep) override {
        VL_RESTORER(m_अन्तर्_Restrict);
        m_अन्तर्_Restrict = सत्य;
        iterateChildren(nodep);
        // IEEE says simulator ignores these
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
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
        m_finalp = शून्यसूचकः;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_Gen_खण्डम्* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstBegin* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
        // If the body is a single assertOn check, bubble it up for combining
        यदि (!nodep->declsp()) {
            यदि (AstIf* ध्रुव ifp = VN_CAST(nodep->stmtsp(), If)) {
                यदि (ifp->user2() && !ifp->nextp()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(ifp->unlink_Fr_पृष्ठभागः());
                    nodep->सङ्कलय_Stmtsp(ifp->thensp()->unlink_Fr_पृष्ठभागः_With_अग्र());
                    ifp->सङ्कलय_Thensp(nodep);
                }
            }
        }
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट AssertVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~AssertVisitor() override {
        V3Stats::सङ्कलय_Stat("Assertions, assert non-immediate statements", m_statAsNotImm);
        V3Stats::सङ्कलय_Stat("Assertions, assert immediate statements", m_statAsImm);
        V3Stats::सङ्कलय_Stat("Assertions, cover statements", m_statCover);
        V3Stats::सङ्कलय_Stat("Assertions, full/parallel case", m_stat_As_पूर्णम्);
        V3Stats::सङ्कलय_Stat("Assertions, $past variables", m_statPastVars);
        V3Stats::सङ्कलय_Stat("Assertions, assertOn checks combined", m_statAssertOnCombined);
        V3Stats::सङ्कलय_Stat("Assertions, assertOn checks hoisted", m_statAssertOnHoisted);
        V3Stats::सङ्कलय_Stat("Assertions, lifted impure case expressions", m_statLiftedCaseExprs);
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
