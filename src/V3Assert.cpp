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
    Ast_गाँठो* m_scopep = nullptr;

    // VISITORS
    शून्य visit(Ast_गाँठो_Module* nodep) override {
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
        यदि (अचर Ast_गाँठो_Module* अचर modp = VN_CAST(m_scopep, गाँठो_Module)) {
            defaultp = modp->defaultDisablep();
        } अन्यथा {
            defaultp = VN_AS(m_scopep, GenBlock)->defaultDisablep();
        }
        यदि (VL_UNLIKELY(defaultp)) {
            nodep->v3error("Only one 'default disable iff' allowed per "
                           << (VN_IS(m_scopep, गाँठो_Module) ? "module" : "generate block")
                           << " (IEEE 1800-2023 16.15)");
        } अन्यथा यदि (Ast_गाँठो_Module* अचर modp = VN_CAST(m_scopep, गाँठो_Module)) {
            modp->defaultDisablep(nodep);
        } अन्यथा {
            VN_AS(m_scopep, GenBlock)->defaultDisablep(nodep);
        }
    }
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    explicit DefaultDisableLocalVisitor(AstNetlist* nodep) { iterate(nodep); }
};

वर्ग DefaultDisablePropagateVisitor final : सार्वजनिक VNVisitor {
    // STATE
    AstDefaultDisable* m_defaultDisablep = nullptr;

    // VISITORS
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    शून्य visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        यदि (!nodep->defaultDisablep()) nodep->defaultDisablep(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    explicit DefaultDisablePropagateVisitor(AstNetlist* nodep) { iterate(nodep); }
};

// Lower a sequence used as an event control ('@seq', IEEE 1800-2023 9.4.2.4) into a
// synthesized event fired by an internal 'cover sequence' on each end-of-match
वर्ग SeqEventLowerVisitor final : सार्वजनिक VNVisitor {
    // STATE
    Ast_गाँठो_Module* m_modp = nullptr;  // Current module
    V3UniqueNames m_names{"__VseqEvent"};  // Synthesized event names

    // VISITORS
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstSenItem* nodep) override {
        AstFuncRef* अचर funcrefp = VN_CAST(nodep->sensp(), FuncRef);
        यदि (funcrefp && VN_IS(funcrefp->taskp(), Sequence)) {
            File_पङ्क्ति* अचर flp = nodep->fileline();
            AstVar* अचर eventp = नूतन AstVar{flp, VVarType::MODULETEMP, m_names.get(nodep),
                                              m_modp->findBasicDType(VBasicDTypeKwd::EVENT)};
            eventp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(eventp);
            v3Global.setHasEvents();
            funcrefp->unlinkFrBack();
            nodep->sensp(नूतन AstVarRef{flp, eventp, VAccess::READ});
            अचर तार्किक automaticActual = funcrefp->exists([](अचर Ast_गाँठो_Var_Ref* refp) {
                फर्काऊ refp->varp() && refp->varp()->lifetime().isAutomatic();
            });
            यदि (automaticActual) {
                nodep->v3error("Arguments to a sequence used as an event control must be"
                               " static (IEEE 1800-2023 9.4.2.4)");
                VN_AS(funcrefp->taskp(), Sequence)->isReferenced(असत्य);
                VL_DO_DANGLING(थप_Deletep(funcrefp), funcrefp);
                फर्काऊ;
            }
            AstFireEvent* अचर firep
                = नूतन AstFireEvent{flp, नूतन AstVarRef{flp, eventp, VAccess::WRITE}, असत्य};
            AstCover* अचर coverp
                = नूतन AstCover{flp, नूतन AstPropSpec{flp, nullptr, nullptr, funcrefp}, firep,
                               VAssertType::CONCURRENT};
            coverp->isCoverSeq(सत्य);
            coverp->isSeqEvent(सत्य);
            m_modp->addStmtsp(coverp);
            फर्काऊ;
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

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
    V3Global::dump_Check_Global_रूख("assertseqevent", 0, dump_रूख_Either_Level() >= 3);
}

string V3AssertCommon::assertCtlGetCall(अचर वर्ण* query, VAssertType type,
                                        VAssertDirectiveType directiveType) {
    // Template arguments let the runtime build the type/directive mask at C++ compile time
    फर्काऊ "vlSymsp->_vm_contextp__->assertCtlGet<"s + std::to_string(type) + ", "s
           + std::to_string(directiveType) + ">(VerilatedAssertCtlQuery::"s + query + ")"s;
}

तार्किक V3AssertCommon::check_Sva_मान_Limit(Ast_गाँठो* nodep, अचर AstConst* constp, अचर वर्ण* what) {
    // Temporal values can materialize as O(N) AST or state in later passes.
    // Leave headroom for a range ring's extra slot and V3Number's signed
    // `(bits + 31) / 32` word count.
    स्थिर constexpr अचिन्हित SVA_मान_HARD_LIMIT
        = static_cast<अचिन्हित>(std::numeric_limits<पूर्णाङ्क>::max()) - 32U;
    अचर अचिन्हित configuredLimit = static_cast<अचिन्हित>(v3Global.opt.max_अङ्क_Width());
    अचर अचिन्हित limit = std::min(configuredLimit, SVA_मान_HARD_LIMIT);
    यदि (constp->अङ्क().fitsInUInt() && constp->toUInt() <= limit) फर्काऊ सत्य;
    nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << what << " exceeds implementation limit of "
                                                 << limit
                                                 << (configuredLimit <= SVA_मान_HARD_LIMIT
                                                         ? " (--max-num-width)"
                                                         : " (host arithmetic limit)"));
    फर्काऊ असत्य;
}

// The bound of a cycle delay in a sequence, if a non-negative elaboration-time constant
स्थिर अचर AstConst* checkSvaDelayBound(Ast_गाँठो* errp, Ast_गाँठो_अभिव्यक्ति* boundp, अचर वर्ण* what) {
    अचर AstConst* अचर constp = VN_CAST(boundp, Const);
    यदि (!constp || (constp->dtypep()->isSigned() && constp->अङ्क().isNegative())) {
        errp->v3error(what << " is not a non-negative elaboration-time constant"
                              " (IEEE 1800-2023 16.7)");
        फर्काऊ nullptr;
    }
    यदि (!V3AssertCommon::check_Sva_मान_Limit(errp, constp, "SVA cycle delay")) फर्काऊ nullptr;
    फर्काऊ constp;
}

// Fold the bounds of a cycle delay in a sequence, and return if they are valid, else report it
// on errp
स्थिर तार्किक svaCycleDelayValid(अचर AstDelay* nodep, Ast_गाँठो* errp) {
    V3Const::constifyParamsNoWarnEdit(nodep->lhsp());
    अचर AstConst* अचर minConstp = checkSvaDelayBound(
        errp, nodep->lhsp(), nodep->isRangeDelay() ? "Range delay minimum" : "Delay value");
    यदि (!nodep->rhsp() || nodep->isUnbounded()) फर्काऊ minConstp != nullptr;
    V3Const::constifyParamsNoWarnEdit(nodep->rhsp());
    अचर AstConst* अचर maxConstp
        = checkSvaDelayBound(errp, nodep->rhsp(), "Range delay maximum");
    यदि (!minConstp || !maxConstp) फर्काऊ असत्य;
    यदि (maxConstp->toUInt() < minConstp->toUInt()) {
        errp->v3error("Range delay maximum must be >= minimum (IEEE 1800-2023 16.7)");
        फर्काऊ असत्य;
    }
    फर्काऊ सत्य;
}

शून्य V3AssertCommon::checkSvaCycleDelay(AstDelay* nodep, Ast_गाँठो* errp) {
    यदि (svaCycleDelayValid(nodep, errp)) फर्काऊ;
    // Make the delay valid, so later steps do not fail after the error
    File_पङ्क्ति* अचर flp = nodep->fileline();
    Ast_गाँठो_अभिव्यक्ति* अचर minp = nodep->lhsp();
    minp->replaceWith(नूतन AstConst{flp, AstConst::BitFalseErroring{}});
    VL_DO_DANGLING(minp->delete_रूख(), minp);
    यदि (nodep->rhsp() && !nodep->isUnbounded()) {
        Ast_गाँठो_अभिव्यक्ति* अचर maxp = nodep->rhsp();
        maxp->replaceWith(नूतन AstConst{flp, AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(maxp->delete_रूख(), maxp);
    }
}

तार्किक V3AssertCommon::refsSvaFormal(अचर Ast_गाँठो* nodep) {
    फर्काऊ nodep->exists([](अचर Ast_गाँठो_Var_Ref* refp) {
        अचर AstVar* अचर varp = refp->varp();
        यदि (!varp || !varp->isIO()) फर्काऊ असत्य;
        अचर Ast_गाँठो* अचर declp = varp->aboveLoopp();
        फर्काऊ VN_IS(declp, Sequence) || VN_IS(declp, Property);
    });
}

शून्य V3AssertCommon::checkSvaInstanceDelays(Ast_गाँठो* nodep, Ast_गाँठो* instp) {
    // A delay still referencing a formal argument of an enclosing sequence or property is
    // checked when that is instanced. Collect first, as checking edits the delays.
    std::vector<AstDelay*> delayps;
    nodep->foreach([&](AstDelay* delayp) {
        यदि (!delayp->isCycleDelay() || refsSvaFormal(delayp->lhsp())) फर्काऊ;
        यदि (delayp->rhsp() && refsSvaFormal(delayp->rhsp())) फर्काऊ;
        delayps.थप_back(delayp);
    });
    लागि (AstDelay* अचर delayp : delayps) checkSvaCycleDelay(delayp, instp);
}

//######################################################################
// AssertDeFutureVisitor
// If any AstFuture, then move all non-future varrefs to be one cycle behind,
// see IEEE 1800-2023 16.9.4.

वर्ग AssertDeFutureVisitor final : सार्वजनिक VNVisitor {
    // STATE - across all visitors
    Ast_गाँठो_Module* अचर m_modp;  // Module future is underneath
    अचर AstFuture* m_futurep;  // First AstFuture found
    अचर अचिन्हित m_past_अङ्क;  // Prefix unique number for this module
    std::map<AstVar*, AstVar*> m_delayedVars;  // Old to delayed variable mapping
    // STATE - for current visit position (use VL_RESTORER)
    तार्किक m_inFuture = असत्य;  // Inside a future
    तार्किक m_unsupported = असत्य;  // Printed unsupported

    // METHODS
    शून्य unsupported(Ast_गाँठो* nodep) {
        यदि (m_unsupported) फर्काऊ;
        m_unsupported = सत्य;
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported/illegal: Future value function used with expression with "
                          << nodep->prettyOperatorName());
    }
    // VISITORS
    शून्य visit(AstFuture* nodep) override {
        VL_RESTORER(m_inFuture);
        m_inFuture = सत्य;
        iterateChildren(nodep);
        // Done with the future, this subexpression is current-time
        nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(Ast_गाँठो_Var_Ref* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;
        यदि (m_inFuture || m_unsupported)
            फर्काऊ;  // Need user1 set above, don't process when Future is removed
        यदि (nodep->access().isWriteOrRW()) {
            unsupported(nodep);
            फर्काऊ;
        }
        स्वतः it = m_delayedVars.find(nodep->varp());
        AstVar* outvarp;
        यदि (it == m_delayedVars.end()) {
            Ast_Sen_रूख* अचर sentreep = m_futurep->sentreep();
            AstAlways* अचर alwaysp = नूतन AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS,
                                                     sentreep->clone_रूख(असत्य), nullptr};
            m_modp->addStmtsp(alwaysp);
            outvarp = नूतन AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                 "__Vnotfuture" + cvtToStr(m_past_अङ्क) + "_" + nodep->name(),
                                 nodep->dtypep()};
            m_modp->addStmtsp(outvarp);
            AstVarRef* varRefAWritep = नूतन AstVarRef{nodep->fileline(), outvarp, VAccess::WRITE};
            varRefAWritep->user1(सत्य);
            Ast_गाँठो_Var_Ref* varRefAReadp = nodep->clone_रूख(असत्य);
            varRefAReadp->user1(सत्य);
            Ast_गाँठो* अचर assp = नूतन AstAssignDly{nodep->fileline(), varRefAWritep, varRefAReadp};
            alwaysp->addStmtsp(assp);
            m_delayedVars.emplace(nodep->varp(), outvarp);
        } अन्यथा {
            outvarp = it->second;
        }
        AstVarRef* newp = नूतन AstVarRef{nodep->fileline(), outvarp, VAccess::READ};
        newp->user1(सत्य);
        UINFO(9, "DeFuture " << nodep << "  becomes " << newp);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(Ast_गाँठो_FTask_Ref* nodep) override { unsupported(nodep); }
    शून्य visit(AstMethodCall* nodep) override { unsupported(nodep); }
    शून्य visit(Ast_गाँठो* nodep) override {
        यदि (!nodep->isPure()) unsupported(nodep);
        iterateChildren(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit AssertDeFutureVisitor(Ast_गाँठो* nodep, Ast_गाँठो_Module* modp, अचिन्हित past_अङ्क)
        : m_modp{modp}
        , m_past_अङ्क{past_अङ्क} {
        // See if any Future before we process
        यदि (nodep->forall([&](अचर AstFuture* futurep) -> तार्किक {
                m_futurep = futurep;
                फर्काऊ असत्य;
            }))
            फर्काऊ;
        // UINFOTREE(9, nodep, "", "defuture-in");
        visit(nodep);  // Nodep may get deleted
        // UINFOTREE(9, nodep, "", "defuture-ou");
    }
    ~AssertDeFutureVisitor() = स्वभाव;
};

// Prepare one default sampled expression.
वर्ग Default_Sampled_मान_Visitor final : सार्वजनिक VNVisitor {
    // METHODS
    स्थिर AstConst* newTypeDefault(Ast_गाँठो_अभिव्यक्ति* nodep) {
        AstConst* अचर constp
            = नूतन AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()};
        यदि (nodep->dtypep()->isFourstate()) constp->अङ्क().setAllBitsX();
        constp->dtypeFrom(nodep);
        फर्काऊ constp;
    }

    // VISITORS
    शून्य visit(AstCMethodHard* nodep) override {
        यदि (nodep->method() == VCMethod::EVENT_IS_TRIGGERED) {
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
        } अन्यथा {
            iterateChildren(nodep);
        }
    }
    शून्य visit(Ast_गाँठो_Var_Ref* nodep) override {
        AstVar* अचर varp = nodep->varp();
        यदि (varp->isParam()
            || (!varp->isNet() && varp->lifetime().isStatic() && varp->hasUserInit())) {
            फर्काऊ;
        }
        nodep->replaceWith(newTypeDefault(nodep));
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    explicit Default_Sampled_मान_Visitor(Ast_गाँठो* nodep) { iterate(nodep); }
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
    अचर VNUser1InUse m_user1InUse;
    अचर VNUser2InUse m_user2InUse;
    AstUser2Allocator<AstAlways, std::vector<AstVar*>> m_delayed;

    // STATE
    Ast_गाँठो_Module* m_modp = nullptr;  // Last module
    अचर Ast_गाँठो* m_beginp = nullptr;  // Last AstBegin/AstGenBlock
    अचिन्हित m_monitor_अङ्क = 0;  // Global $monitor numbering (not per module)
    AstVar* m_monitor_अङ्क_Varp = nullptr;  // $monitor number variable
    AstVar* m_monitorOffVarp = nullptr;  // $monitoroff variable
    अचिन्हित m_mod_Past_अङ्क = 0;  // Module past numbering
    अचिन्हित m_mod_Strobe_अङ्क = 0;  // Module $strobe numbering
    Ast_गाँठो_Procedure* m_procedurep = nullptr;  // Current procedure
    VDouble0 m_statCover;  // Statistic tracking
    VDouble0 m_statAsNotImm;  // Statistic tracking
    VDouble0 m_statAsImm;  // Statistic tracking
    VDouble0 m_statAsFull;  // Statistic tracking
    VDouble0 m_statPastVars;  // Statistic tracking
    VDouble0 m_statAssertOnCombined;  // Statistic tracking
    VDouble0 m_statAssertOnHoisted;  // Statistic tracking
    तार्किक m_inSampled = असत्य;  // True inside a sampled expression
    तार्किक m_inRestrict = असत्य;  // True inside restrict assertion
    Ast_गाँठो* m_passsp = nullptr;  // Current pass statement
    Ast_गाँठो* m_failsp = nullptr;  // Current fail statement
    Ast_गाँठो_Cover_Or_Assert* m_assertp = nullptr;  // Current assertion
    AstFinal* m_finalp = nullptr;  // Current final block
    VDouble0 m_statLiftedCaseExprs;  // Count of purified case expressions
    Ast_गाँठो_FTask* m_ftaskp = nullptr;  // Current function/task
    V3UniqueNames m_caseTempNames{"__VCase"};
    V3UniqueNames m_match_गन्ती_Names{"__VnfaRemainingMatchCount"};  // Match replay counter names
    // Maps from (expression, senTree) to the AstAlways that computes its delayed values.
    std::unordered_map<VNRef<Ast_गाँठो_अभिव्यक्ति>, std::unordered_map<VNRef<Ast_Sen_रूख>, AstAlways*>>
        m_modExpr2Sen2DelayedAlwaysp;
    // Source sampled-value history is initialized, unlike internal property-lowering history.
    std::unordered_map<VNRef<Ast_गाँठो_अभिव्यक्ति>, std::unordered_map<VNRef<Ast_Sen_रूख>, AstAlways*>>
        m_modExpr2Sen2DefaultDelayedAlwaysp;
    // Map from delayed-value AstAlways to its last-sampling-tick time variable
    std::unordered_map<अचर AstAlways*, AstVar*> m_delayedAlways2TickTimep;

    // METHODS
    स्थिर अचर वर्ण* assertPassOnQuery(तार्किक vacuous) {
        स्थिर constexpr अचर वर्ण* queries[2]
            = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
        फर्काऊ queries[vacuous];
    }
    स्थिर Ast_गाँठो_अभिव्यक्ति* assertOnCond(File_पङ्क्ति* fl, VAssertType type,
                                     VAssertDirectiveType directiveType) {
        // cppcheck-suppress missingReturn
        विकल्प (directiveType) {
        पक्ष VAssertDirectiveType::INTRINSIC: फर्काऊ नूतन AstConst{fl, AstConst::BitTrue{}};
        पक्ष VAssertDirectiveType::VIOLATION_CASE: {
            यदि (v3Global.opt.assertCase()) {
                फर्काऊ नूतन AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            // If assertions are off, have constant propagation rip them out later
            // This allows syntax errors and such to be detected normally.
            फर्काऊ नूतन AstConst{fl, AstConst::BitFalse{}};
        }
        पक्ष VAssertDirectiveType::ASSERT:
        पक्ष VAssertDirectiveType::COVER:
        पक्ष VAssertDirectiveType::ASSUME: {
            यदि (v3Global.opt.assertOn()) {
                फर्काऊ नूतन AstCExpr{
                    fl, AstCExpr::Pure{},
                    V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
            }
            फर्काऊ नूतन AstConst{fl, AstConst::BitFalse{}};
        }
        पक्ष VAssertDirectiveType::INTERNAL:
        पक्ष VAssertDirectiveType::VIOLATION_IF:
        पक्ष VAssertDirectiveType::RESTRICT: {
            यदि (v3Global.opt.assertOn()) {
                फर्काऊ नूतन AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            फर्काऊ नूतन AstConst{fl, AstConst::BitFalse{}};
        }
        }
        VL_UNREACHABLE;
    }

    स्थिर तार्किक isControlled(VAssertDirectiveType directiveType) {
        फर्काऊ (static_cast<पूर्णाङ्क>(directiveType)
                & (static_cast<पूर्णाङ्क>(VAssertDirectiveType::ASSERT)
                   | static_cast<पूर्णाङ्क>(VAssertDirectiveType::COVER)
                   | static_cast<पूर्णाङ्क>(VAssertDirectiveType::ASSUME)));
    }
    स्थिर Ast_गाँठो_अभिव्यक्ति* assertPassOnCond(File_पङ्क्ति* fl, VAssertType type,
                                         VAssertDirectiveType directiveType, तार्किक vacuous) {
        यदि (!isControlled(directiveType)) फर्काऊ नूतन AstConst{fl, AstConst::BitTrue{}};
        यदि (!v3Global.opt.assertOn()) फर्काऊ नूतन AstConst{fl, AstConst::BitFalse{}};
        फर्काऊ नूतन AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType), 1};
    }
    स्थिर Ast_गाँठो_अभिव्यक्ति* assertFailOnCond(File_पङ्क्ति* fl, VAssertType type,
                                         VAssertDirectiveType directiveType) {
        यदि (!isControlled(directiveType)) फर्काऊ नूतन AstConst{fl, AstConst::BitTrue{}};
        यदि (!v3Global.opt.assertOn()) फर्काऊ नूतन AstConst{fl, AstConst::BitFalse{}};
        फर्काऊ नूतन AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType), 1};
    }
    string assertDisplayMessage(अचर Ast_गाँठो* nodep, अचर string& prefix, अचर string& message,
                                VDisplayType severity) {
        यदि (severity == VDisplayType::DT_ERROR || severity == VDisplayType::DT_FATAL) {
            फर्काऊ ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                    + cvtToStr(nodep->fileline()->lineno()) + ": Assertion failed in %m"
                    + ((message != "") ? ": " : "") + message + "\n");
        }
        फर्काऊ ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                + cvtToStr(nodep->fileline()->lineno()) + ": %m" + ((message != "") ? ": " : "")
                + message + "\n");
    }
    शून्य replaceDisplay(AstDisplay* nodep, अचर string& prefix) {
        nodep->fmtp()->text(
            assertDisplayMessage(nodep, prefix, nodep->fmtp()->text(), nodep->displayType()));
        nodep->displayType(VDisplayType::DT_WRITE);
        // cppcheck-suppress nullPointer
        Ast_गाँठो_अभिव्यक्ति* अचर timenewp = नूतन AstTime{nodep->fileline(), m_modp->timeunit()};
        nodep->fmtp()->prependExprsp(timenewp);
        यदि (!nodep->fmtp()->scopeNamep() && nodep->fmtp()->formatScopeTracking()) {
            nodep->fmtp()->scopeNamep(नूतन AstScopeName{nodep->fileline(), सत्य});
        }
    }
    AstSampled* new_Sampled_अभिव्यक्ति(Ast_गाँठो_अभिव्यक्ति* nodep) {
        फर्काऊ नूतन AstSampled{nodep->fileline(), nodep, nodep->dtypep(), सत्य};
    }
    AstVarRef* new_Monitor_अङ्क_Var_Refp(अचर Ast_गाँठो* nodep, VAccess access) {
        यदि (!m_monitor_अङ्क_Varp) {
            m_monitor_अङ्क_Varp = नूतन AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorNum",
                                          nodep->findUInt64DType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitor_अङ्क_Varp);
        }
        AstVarRef* अचर varrefp = नूतन AstVarRef{nodep->fileline(), m_monitor_अङ्क_Varp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        फर्काऊ varrefp;
    }
    AstVarRef* newMonitorOffVarRefp(अचर Ast_गाँठो* nodep, VAccess access) {
        यदि (!m_monitorOffVarp) {
            m_monitorOffVarp = नूतन AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorOff",
                                          nodep->findBitDType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitorOffVarp);
        }
        AstVarRef* अचर varrefp = नूतन AstVarRef{nodep->fileline(), m_monitorOffVarp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        फर्काऊ varrefp;
    }
    स्थिर AstIf* newIfAssertOn(Ast_गाँठो* bodyp, VAssertDirectiveType directiveType,
                                VAssertType type = VAssertType::INTERNAL) {
        // Add an internal if to check assertions are on.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_पङ्क्ति* अचर fl = bodyp->fileline();

        Ast_गाँठो_अभिव्यक्ति* अचर condp = assertOnCond(fl, type, directiveType);
        AstIf* अचर newp = नूतन AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(सत्य);  // To avoid LATCH warning
        newp->user2(सत्य);  // Mark as an assertOn() check
        फर्काऊ newp;
    }
    स्थिर Ast_गाँठो_Stmt* newIfAssertPassOn(Ast_गाँठो* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type, तार्किक vacuous) {
        // Add an internal if to check assertion passOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_पङ्क्ति* अचर fl = bodyp->fileline();
        Ast_गाँठो_अभिव्यक्ति* अचर condp = assertPassOnCond(fl, type, directiveType, vacuous);
        Ast_गाँठो_If* अचर newp = नूतन AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(सत्य);  // To avoid LATCH warning
        newp->user1(सत्य);  // Don't assert/cover this if
        newp->user2(सत्य);  // Mark as an assertOn() check
        फर्काऊ newp;
    }
    स्थिर Ast_गाँठो_Stmt* newIfAssertFailOn(Ast_गाँठो* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type) {
        // Add an internal if to check assertion failOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_पङ्क्ति* अचर fl = bodyp->fileline();
        Ast_गाँठो_अभिव्यक्ति* अचर condp = assertFailOnCond(fl, type, directiveType);
        Ast_गाँठो_If* अचर newp = नूतन AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(सत्य);  // To avoid LATCH warning
        newp->user1(सत्य);  // Don't assert/cover this if
        newp->user2(सत्य);  // Mark as an assertOn() check
        फर्काऊ newp;
    }

    स्थिर AstIf* assertCond(अचर Ast_गाँठो_Cover_Or_Assert* nodep, Ast_गाँठो_अभिव्यक्ति* propp,
                             Ast_गाँठो* passsp, Ast_गाँठो* failsp) {
        AstIf* अचर ifp = नूतन AstIf{nodep->fileline(), propp, passsp, failsp};
        // It's more LIKELY that we'll take the nullptr if clause
        // than the sim-killing else clause:
        ifp->branchPred(VBranchPred::BP_LIKELY);
        ifp->isBoundsCheck(सत्य);  // To avoid LATCH warning
        फर्काऊ ifp;
    }

    Ast_गाँठो_Stmt* assertBody(अचर Ast_गाँठो_Cover_Or_Assert* nodep, Ast_गाँठो* propp, Ast_गाँठो* passsp,
                            Ast_गाँठो* failsp) {
        यदि (AstPExpr* अचर pexprp = VN_CAST(propp, PExpr)) {
            यदि (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NOTIMING, "This property expression requires --timing");
                VL_DO_DANGLING(थप_Deletep(pexprp), pexprp);
                फर्काऊ नूतन AstBegin{nodep->fileline(), "", nullptr, असत्य};
            }
            AstFork* अचर forkp = नूतन AstFork{nodep->fileline(), VJoinType::JOIN_NONE};
            forkp->addForksp(pexprp->bodyp()->unlinkFrBack());
            यदि (Ast_गाँठो_Stmt* अचर finalp = pexprp->finalp()) {
                यदि (!m_finalp) {
                    m_finalp = नूतन AstFinal{m_modp->fileline(), finalp->unlinkFrBack()};
                    m_modp->addStmtsp(m_finalp);
                } अन्यथा {
                    m_finalp->addStmtsp(finalp->unlinkFrBack());
                }
            }
            VL_DO_DANGLING2(थप_Deletep(pexprp), pexprp, propp);
            फर्काऊ forkp;
        }

        फर्काऊ assertCond(nodep, VN_AS(propp, गाँठो_अभिव्यक्ति), passsp, failsp);
    }

    Ast_गाँठो_Stmt* newFireAssertUnchecked(अचर Ast_गाँठो_Stmt* nodep, अचर string& message,
                                        Ast_गाँठो_अभिव्यक्ति* exprsp = nullptr) {
        // Like newFireAssert() but omits the asserts-on check
        AstDisplay* अचर dispp
            = नूतन AstDisplay{nodep->fileline(), VDisplayType::DT_ERROR, message, nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        Ast_गाँठो_Stmt* अचर bodysp = dispp;
        replaceDisplay(dispp, "%%Error");  // Convert to standard DISPLAY format
        यदि (exprsp) dispp->fmtp()->exprsp()->addNext(exprsp);
        यदि (v3Global.opt.stopFail()) bodysp->addNext(नूतन AstStop{nodep->fileline(), असत्य});
        फर्काऊ bodysp;
    }

    Ast_गाँठो_Stmt* newFireAssert(अचर Ast_गाँठो_Stmt* nodep, VAssertDirectiveType directiveType,
                               VAssertType assertType, अचर string& message,
                               Ast_गाँठो_अभिव्यक्ति* exprsp = nullptr) {
        Ast_गाँठो_Stmt* bodysp = newFireAssertUnchecked(nodep, message, exprsp);
        bodysp = newIfAssertOn(bodysp, directiveType, assertType);
        फर्काऊ bodysp;
    }

    AstVar* createDelayedVar(अचर std::string& name, AstAlways* alwaysp, Ast_गाँठो_अभिव्यक्ति* exprp,
                             Ast_गाँठो_अभिव्यक्ति* initp) {
        File_पङ्क्ति* अचर flp = exprp->fileline();
        AstVar* अचर varp = नूतन AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        // TODO: this lifetime seems nonsene (can't have NBAs to automatics), but is as before
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->addStmtsp(varp);
        // Before enough clock ticks, $past returns the expression's default sampled value.
        यदि (initp) {
            m_modp->addStmtsp(नूतन AstInitialStatic{
                flp, नूतन AstAssign{flp, नूतन AstVarRef{flp, varp, VAccess::WRITE}, initp}});
        }
        ++m_statPastVars;
        // Actually set the delayed value
        Ast_गाँठो_अभिव्यक्ति* अचर lhsp = नूतन AstVarRef{flp, varp, VAccess::WRITE};
        AstAssignDly* अचर assignp = नूतन AstAssignDly{flp, lhsp, exprp};
        alwaysp->prependStmtsp(assignp);
        फर्काऊ varp;
    }

    AstAlways* getDelayedAlways(Ast_गाँठो_अभिव्यक्ति* exprp, Ast_गाँठो_अभिव्यक्ति* initp, Ast_Sen_रूख* senTreep) {
        स्वतः& expr2Sen2Alwaysr
            = initp ? m_modExpr2Sen2DefaultDelayedAlwaysp : m_modExpr2Sen2DelayedAlwaysp;
        AstAlways*& alwayspr = expr2Sen2Alwaysr[*exprp][*senTreep];
        यदि (!alwayspr) {
            File_पङ्क्ति* अचर flp = exprp->fileline();
            // Create the always block that computes the delayed values
            alwayspr = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, nullptr};
            m_modp->addStmtsp(alwayspr);
            // Create the once-delayed variable
            अचर std::string name = "_Vpast_" + cvtToStr(m_mod_Past_अङ्क++) + "_1";
            AstVar* अचर varp = createDelayedVar(name, alwayspr, exprp, initp);
            // Add it to delayed variable vector
            m_delayed(alwayspr).emplace_back(varp);
        } अन्यथा {
            // Reusing exiting, not needed
            VL_DO_DANGLING(थप_Deletep(exprp), exprp);
            यदि (initp) VL_DO_DANGLING(थप_Deletep(initp), initp);
            VL_DO_DANGLING(थप_Deletep(senTreep), senTreep);
        }
        फर्काऊ alwayspr;
    }

    Ast_गाँठो_अभिव्यक्ति* get_Past_मान(Ast_गाँठो_अभिव्यक्ति* exprp, Ast_गाँठो_अभिव्यक्ति* initp, Ast_Sen_रूख* senTreep,
                              uint32_t ticks) {
        UASSERT_OBJ(ticks > 0, exprp, "Delay must be > 0");
        File_पङ्क्ति* अचर flp = exprp->fileline();
        AstAlways* अचर alwaysp
            = getDelayedAlways(exprp, initp ? initp->clone_रूख_Pure(असत्य) : nullptr, senTreep);
        Ast_गाँठो_अभिव्यक्ति* अचर resultp = past_मान_Ref(alwaysp, flp, ticks, initp);
        यदि (initp) VL_DO_DANGLING(थप_Deletep(initp), initp);
        फर्काऊ resultp;
    }

    Ast_गाँठो_अभिव्यक्ति* past_मान_Ref(AstAlways* alwaysp, File_पङ्क्ति* flp, uint32_t ticks,
                              Ast_गाँठो_अभिव्यक्ति* initp) {
        std::vector<AstVar*>& delayedr = m_delayed(alwaysp);
        // Ensure the required delay exists
        जबसम्म (delayedr.size() < ticks) {
            AstVar* अचर firstp = delayedr.front();
            File_पङ्क्ति* अचर varFlp = firstp->fileline();
            // Create once more delayed value
            std::string name = firstp->name();
            name.resize(name.size() - 1);
            name += std::to_string(delayedr.size() + 1);
            Ast_गाँठो_अभिव्यक्ति* अचर prevp = नूतन AstVarRef{varFlp, delayedr.back(), VAccess::READ};
            AstVar* अचर varp = createDelayedVar(name, alwaysp, prevp,
                                                  initp ? initp->clone_रूख_Pure(असत्य) : nullptr);
            // Add it to delayed variable vector
            delayedr.emplace_back(varp);
        }
        // Return a reference to the appropriately delayed variable
        फर्काऊ नूतन AstVarRef{flp, delayedr.at(ticks - 1), VAccess::READ};
    }

    // Time of the pipeline's last sampling tick, for end-of-simulation readers
    AstVar* getPastTickTimeVar(AstAlways* alwaysp) {
        AstVar*& varpr = m_delayedAlways2TickTimep[alwaysp];
        यदि (!varpr) {
            File_पङ्क्ति* अचर flp = alwaysp->fileline();
            varpr = नूतन AstVar{flp, VVarType::MODULETEMP,
                               "_Vpast_" + cvtToStr(m_mod_Past_अङ्क++) + "_t",
                               m_modp->findUInt64DType()};
            m_modp->addStmtsp(varpr);
            Ast_गाँठो_अभिव्यक्ति* अचर timep = नूतन AstCExpr{flp, "vlSymsp->_vm_contextp__->time()", 64};
            alwaysp->addStmtsp(
                नूतन AstAssign{flp, नूतन AstVarRef{flp, varpr, VAccess::WRITE}, timep});
        }
        फर्काऊ varpr;
    }

    शून्य visitAssertionIterate(Ast_गाँठो_Cover_Or_Assert* nodep, Ast_गाँठो* failsp) {
        यदि (m_beginp && nodep->name() == "") nodep->name(m_beginp->name());

        { AssertDeFutureVisitor{nodep->propp(), m_modp, m_mod_Past_अङ्क++}; }

        iterateAndNextNull(nodep->sentreep());
        यदि (AstAssert* अचर assertp = VN_CAST(nodep, Assert)) {
            iterateAndNextNull(assertp->failsp());
        } अन्यथा यदि (AstAssertIntrinsic* अचर assertp = VN_CAST(nodep, AssertIntrinsic)) {
            iterateAndNextNull(assertp->failsp());
        } अन्यथा यदि (AstCover* अचर coverp = VN_CAST(nodep, Cover)) {
            iterateAndNextNull(coverp->coverincsp());
        } अन्यथा यदि (!VN_IS(nodep, Restrict)) {
            nodep->v3fatalSrc("Unhandled assert type");
        }
        iterateAndNextNull(nodep->passsp());
        Ast_Sen_रूख* sentreep = nodep->sentreep();
        यदि (nodep->immediate()) {
            UASSERT_OBJ(!sentreep, nodep, "Immediate assertions don't have sensitivity");
        } अन्यथा {
            UASSERT_OBJ(sentreep, nodep, "Concurrent assertions must have sensitivity");
            // Explicit inline clock differs from the enclosing always: hoist
            // and warn. To support this need queue of asserts to activate.
            यदि (m_procedurep && !nodep->senFromAlways()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Procedural concurrent assertion with"
                              " clocking event inside always (IEEE 1800-2023 16.14.6)");
                static_cast<Ast_गाँठो*>(m_procedurep)->addNext(nodep->unlinkFrBack());
                फर्काऊ;  // Later iterate will pick up
            }
            sentreep->unlinkFrBack();
            यदि (m_procedurep) { VL_DO_DANGLING(थप_Deletep(sentreep), sentreep); }
        }
        //
        अचर string& message = nodep->name();
        Ast_गाँठो* passsp = nodep->passsp();
        यदि (passsp) passsp->unlinkFrBackWithNext();
        यदि (AstAssert* अचर assertp = VN_CAST(nodep, Assert)) {
            failsp = assertp->failsp();
        } अन्यथा यदि (AstAssertIntrinsic* अचर assertp = VN_CAST(nodep, AssertIntrinsic)) {
            failsp = assertp->failsp();
        }
        यदि (failsp) failsp->unlinkFrBackWithNext();

        तार्किक selfDestruct = असत्य;
        तार्किक passspGated = असत्य;
        अचर AstCover* अचर coverp = VN_CAST(nodep, Cover);
        // A sequence event control is not an assertion directive; no assertion control
        अचर तार्किक seqEvent = coverp && coverp->isSeqEvent();
        यदि (coverp) {
            ++m_statCover;
            यदि (seqEvent) {
                // Keep the event-fire action, with no coverage bucket
            } अन्यथा यदि (!v3Global.opt.coverageUser()) {
                selfDestruct = सत्य;
            } अन्यथा {
                // V3Coverage assigned us a bucket to increment.
                AstCoverInc* अचर covincp = VN_AS(coverp->coverincsp(), CoverInc);
                UASSERT_OBJ(covincp, coverp, "Missing AstCoverInc under assertion");
                covincp->unlinkFrBackWithNext();  // next() might have  AstAssign for trace
                यदि (message != "") covincp->declp()->comment(message);
                यदि (passsp) {
                    passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                               /*vacuous=*/असत्य);
                    passspGated = सत्य;
                    passsp = Ast_गाँठो::addNext<Ast_गाँठो, Ast_गाँठो>(covincp, passsp);
                } अन्यथा {
                    passsp = covincp;
                }
            }
        } अन्यथा यदि (VN_IS(nodep, Assert) || VN_IS(nodep, AssertIntrinsic)) {
            यदि (nodep->immediate()) {
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

        Ast_गाँठो* propExprp;
        Ast_गाँठो_अभिव्यक्ति* disablep = nullptr;
        Ast_गाँठो_अभिव्यक्ति* matchCountp = nullptr;
        यदि (AstPropSpec* अचर specp = VN_CAST(nodep->propp(), PropSpec)) {
            propExprp = specp->propp()->unlinkFrBack();
            यदि (specp->disablep()) disablep = specp->disablep()->unlinkFrBack();
            matchCountp = specp->matchCountp();
        } अन्यथा {
            propExprp = nodep->propp()->unlinkFrBack();
        }
        File_पङ्क्ति* अचर flp = nodep->fileline();
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
        यदि (coverp && matchCountp && passsp) {
            // Convert the match count into a loop that decrements a temporary variable until it
            // reaches zero.
            matchCountp->unlinkFrBack();
            AstVar* अचर remainingp = नूतन AstVar{
                flp, VVarType::MODULETEMP, m_match_गन्ती_Names.get(""), matchCountp->dtypep()};
            remainingp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remainingp);
            Ast_गाँठो* अचर replaysp
                = नूतन AstAssign{flp, नूतन AstVarRef{flp, remainingp, VAccess::WRITE}, matchCountp};
            AstLoop* अचर loopp = नूतन AstLoop{flp};
            loopp->addStmtsp(
                नूतन AstLoopTest{flp, loopp, नूतन AstVarRef{flp, remainingp, VAccess::READ}});
            loopp->addStmtsp(passsp);
            loopp->addStmtsp(
                नूतन AstAssign{flp, नूतन AstVarRef{flp, remainingp, VAccess::WRITE},
                              नूतन AstSub{flp, नूतन AstVarRef{flp, remainingp, VAccess::READ},
                                         नूतन AstConst{flp, AstConst::Widthed_मान{},
                                                      remainingp->dtypep()->width(), 1}}});
            replaysp->addNext(loopp);
            passsp = replaysp;
        }
        Ast_गाँठो* bodysp = assertBody(nodep, propExprp, passsp, failsp);
        यदि (disablep) bodysp = नूतन AstIf{flp, नूतन AstLogNot{flp, disablep}, bodysp};
        // Add assertOn check last, for better combining
        यदि (!seqEvent) bodysp = newIfAssertOn(bodysp, nodep->directive(), nodep->userType());
        यदि (sentreep) bodysp = नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, sentreep, bodysp};

        यदि (passsp && !passsp->backp()) VL_DO_DANGLING(थप_Deletep(passsp), passsp);
        यदि (failsp && !failsp->backp()) VL_DO_DANGLING(थप_Deletep(failsp), failsp);

        // Install it
        यदि (selfDestruct) {
            // Delete it after making the tree.  This way we can tell the user
            // if it wasn't constructed nicely or has other errors without needing --coverage.
            VL_DO_DANGLING(bodysp->delete_रूख(), bodysp);
            nodep->unlinkFrBack();
        } अन्यथा {
            nodep->replaceWith(bodysp);
        }
        // Bye
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }

    तार्किक isEmptyStmt(Ast_गाँठो* nodep) {
        यदि (!nodep) फर्काऊ सत्य;
        यदि (AstBegin* अचर beginp = VN_CAST(nodep, Begin)) {
            यदि (beginp->declsp()) फर्काऊ असत्य;
            लागि (Ast_गाँठो* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                यदि (!isEmptyStmt(stmtp)) फर्काऊ असत्य;
            }
            फर्काऊ सत्य;
        }
        फर्काऊ असत्य;
    }

    // VISITORS
    शून्य visit(AstIf* nodep) override {
        यदि (!nodep->user1SetOnce()) {
            यदि (nodep->uniquePragma() || nodep->unique0Pragma()) {
                अचर Ast_गाँठो_If* ifp = nodep;
                Ast_गाँठो_अभिव्यक्ति* propp = nullptr;
                तार्किक hasDefaultElse = असत्य;
                गर {
                    // If this statement ends with 'else if', then nextIf will point to the
                    // nextIf statement.  Otherwise it will be null.
                    अचर Ast_गाँठो_If* अचर nextifp = dynamic_cast<Ast_गाँठो_If*>(ifp->elsesp());
                    iterateAndNextNull(ifp->condp());

                    // Recurse into the true case.
                    iterateAndNextNull(ifp->thensp());

                    // If the last else is not an else if, recurse into that too.
                    यदि (ifp->elsesp() && !nextifp) {  //
                        iterateAndNextNull(ifp->elsesp());
                    }

                    // Build a bitmask of the true predicates
                    Ast_गाँठो_अभिव्यक्ति* अचर predp = ifp->condp()->clone_रूख_Pure(असत्य);
                    यदि (propp) {
                        propp = नूतन AstConcat{nodep->fileline(), predp, propp};
                    } अन्यथा {
                        propp = predp;
                    }

                    // Record if this ends with an 'else' that does not have an if
                    यदि (ifp->elsesp() && !nextifp) hasDefaultElse = सत्य;

                    ifp = nextifp;
                } जबसम्म (ifp);

                अचर तार्किक allow_none = nodep->unique0Pragma();

                // Empty case means no property
                यदि (!propp) propp = नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}};

                // Note: if this ends with an 'else', then we don't need to validate that one of
                // the predicates evaluates to true.
                Ast_गाँठो_अभिव्यक्ति* अचर ohot
                    = ((allow_none || hasDefaultElse)
                           ? static_cast<Ast_गाँठो_अभिव्यक्ति*>(नूतन AstOneHot0{nodep->fileline(), propp})
                           : static_cast<Ast_गाँठो_अभिव्यक्ति*>(नूतन AstOneHot{nodep->fileline(), propp}));
                अचर VAssertType assertType
                    = nodep->uniquePragma() ? VAssertType::UNIQUE : VAssertType::UNIQUE0;
                // Move, not clone, as the branches might declare referenced variables
                VNRelinker relinker;
                nodep->unlinkFrBack(&relinker);
                AstIf* अचर checkifp
                    = नूतन AstIf{nodep->fileline(), नूतन AstLogNot{nodep->fileline(), ohot},
                                newFireAssert(nodep, VAssertDirectiveType::VIOLATION_IF,
                                              assertType, "'unique if' statement violated"),
                                nodep};
                checkifp->isBoundsCheck(सत्य);  // To avoid LATCH warning
                checkifp->branchPred(VBranchPred::BP_UNLIKELY);
                relinker.relink(checkifp);
                फर्काऊ;
            }
        }

        iterateChildren(nodep);

        यदि (nodep->user2()) {
            // Combine consecutive assertOn checks if possible
            यदि (AstIf* अचर backp = VN_CAST(nodep->backp(), If)) {
                यदि (backp->nextp() == nodep  //
                    && backp->user2()  //
                    && backp->condp()->same_रूख(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    backp->addThensp(nodep->thensp()->unlinkFrBackWithNext());
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(थप_Deletep(nodep), nodep);
                    फर्काऊ;
                }
            }
            // Combine nested assertOn checks if possible
            यदि (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* अचर checkp = VN_CAST(nodep->thensp(), If);
                यदि (checkp  //
                    && checkp->user2()  //
                    && checkp->condp()->same_रूख(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    VL_DO_DANGLING(थप_Deletep(checkp->unlinkFrBack()), checkp);
                    फर्काऊ;
                }
            }
            फर्काऊ;
        }

        // Swap assertOn check with single statement 'if' statement to bubble up for combining
        // Note we can't just swap the conditions as they two Ifs have different flags,
        // so swapping the Ifs themeselves then swapping back the bodies.
        यदि (nodep->condp()->isPure()) {
            यदि (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* अचर checkp = VN_CAST(nodep->thensp(), If);
                यदि (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addThensp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    फर्काऊ;
                }
            }
            यदि (nodep->elsesp() && !nodep->elsesp()->nextp() && isEmptyStmt(nodep->thensp())) {
                AstIf* अचर checkp = VN_CAST(nodep->elsesp(), If);
                यदि (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addElsesp(checkp->thensp()->unlinkFrBackWithNext());
                    checkp->addThensp(nodep);
                    फर्काऊ;
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
        यदि (!nodep->exprp()->isPure()) {
            ++m_statLiftedCaseExprs;
            File_पङ्क्ति* अचर fl = nodep->exprp()->fileline();
            AstVar* अचर varp = नूतन AstVar{fl, VVarType::BLOCKTEMP, m_caseTempNames.get(nodep),
                                            nodep->exprp()->dtypep()};
            Ast_गाँठो_अभिव्यक्ति* अचर origp = nodep->exprp()->unlinkFrBack();
            nodep->addHereThisAsNext(
                नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, origp});
            nodep->exprp(नूतन AstVarRef{fl, varp, VAccess::READ});
            यदि (m_ftaskp) {
                varp->funcLocal(सत्य);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } अन्यथा {
                m_modp->prependStmtsp(varp);
            }
            VIsCached::clear_Cache_रूख();
        }
        iterateChildren(nodep);
        यदि (!nodep->user1SetOnce()) {
            तार्किक has_default = असत्य;
            लागि (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                यदि (itemp->isDefault()) has_default = सत्य;
            }
            अचर Ast_गाँठो_DType* अभिव्यक्ति_Dtypep = nodep->exprp()->dtypep()->skipRefp();

            VAssertType assertType = VAssertType::INTERNAL;
            यदि (nodep->priorityPragma()) {
                assertType = VAssertType::PRIORITY;
            } अन्यथा यदि (nodep->uniquePragma()) {
                assertType = VAssertType::UNIQUE;
            } अन्यथा यदि (nodep->unique0Pragma()) {
                assertType = VAssertType::UNIQUE0;
            }

            string valFmt;
            यदि (अभिव्यक्ति_Dtypep->isIntegralOrPacked())
                valFmt = " for '" + cvtToStr(अभिव्यक्ति_Dtypep->widthMin()) + "'h%X'";
            यदि (nodep->fullPragma() || nodep->priorityPragma()) {
                // Need to add a default if there isn't one already
                ++m_statAsFull;
                यदि (!has_default) {
                    nodep->addItemsp(नूतन AstCaseItem{
                        nodep->fileline(), nullptr /*DEFAULT*/,
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      nodep->pragmaString() + ", but non-match found" + valFmt,
                                      valFmt.empty() ? nullptr
                                                     : nodep->exprp()->clone_रूख_Pure(असत्य))});
                }
            }
            यदि (nodep->parallelPragma() || nodep->uniquePragma() || nodep->unique0Pragma()) {
                // Need to check that one, and only one of the case items match at any moment
                // If there's a default, we allow none to match, else exactly one must match
                ++m_statAsFull;
                यदि (!has_default && !nodep->itemsp()) {
                    // Not parallel, but harmlessly so.
                } अन्यथा {
                    Ast_गाँठो_अभिव्यक्ति* propp = nullptr;
                    लागि (AstCaseItem* itemp = nodep->itemsp(); itemp;
                         itemp = VN_AS(itemp->nextp(), CaseItem)) {
                        Ast_गाँठो_अभिव्यक्ति* itembitp = nullptr;
                        लागि (Ast_गाँठो_अभिव्यक्ति* icondp = itemp->condsp(); icondp;
                             icondp = VN_AS(icondp->nextp(), गाँठो_अभिव्यक्ति)) {
                            Ast_गाँठो_अभिव्यक्ति* onep;
                            यदि (AstInsideRange* अचर rcondp = VN_CAST(icondp, InsideRange)) {
                                onep = rcondp->newAndFromInside(
                                    nodep->exprp()->clone_रूख_Pure(सत्य),
                                    rcondp->lhsp()->clone_रूख_Pure(सत्य),
                                    rcondp->rhsp()->clone_रूख_Pure(सत्य));
                            } अन्यथा यदि (nodep->casex() || nodep->casez() || nodep->caseInside()) {
                                onep = AstEqWild::newTyped(itemp->fileline(),
                                                           nodep->exprp()->clone_रूख_Pure(असत्य),
                                                           icondp->clone_रूख_Pure(असत्य));
                            } अन्यथा {
                                onep = AstEq::newTyped(icondp->fileline(),
                                                       nodep->exprp()->clone_रूख_Pure(असत्य),
                                                       icondp->clone_रूख_Pure(असत्य));
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
                    अचर तार्किक allow_none = has_default || nodep->unique0Pragma();
                    // The following assertion looks as below.
                    // if (!$onehot(propp)) begin
                    //     if (propp == '0) begin if (!allow_none) $error("none match"); end
                    //     else $error("multiple match");
                    // end
                    Ast_गाँठो_अभिव्यक्ति* अचर ohot = नूतन AstOneHot{nodep->fileline(), propp};
                    AstConst* अचर zero = नूतन AstConst{
                        nodep->fileline(), AstConst::Widthed_मान{}, propp->width(), 0};
                    AstIf* अचर ohotIfp
                        = नूतन AstIf{nodep->fileline(), नूतन AstLogNot{nodep->fileline(), ohot}};
                    AstIf* अचर zeroIfp = नूतन AstIf{
                        nodep->fileline(),
                        नूतन AstEq{nodep->fileline(), propp->clone_रूख_Pure(असत्य), zero}};
                    Ast_गाँठो_अभिव्यक्ति* अचर exprp = nodep->exprp();
                    अचर string pragmaStr = nodep->pragmaString();
                    यदि (!allow_none)
                        zeroIfp->addThensp(
                            newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                          pragmaStr + ", but none matched" + valFmt,
                                          valFmt.empty() ? nullptr : exprp->clone_रूख_Pure(असत्य)));
                    zeroIfp->addElsesp(
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      pragmaStr + ", but multiple matches found" + valFmt,
                                      valFmt.empty() ? nullptr : exprp->clone_रूख_Pure(असत्य)));
                    ohotIfp->addThensp(zeroIfp);
                    ohotIfp->isBoundsCheck(सत्य);  // To avoid LATCH warning
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
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }

    //========== Past
    शून्य visit(AstPast* nodep) override {
        यदि (!nodep->propertyTiming()) {
            nodep->initp(nodep->exprp()->clone_रूख_Pure(असत्य));
            { Default_Sampled_मान_Visitor{nodep->initp()}; }
        }
        Ast_गाँठो_अभिव्यक्ति* अचर initp = nodep->initp() ? nodep->initp()->unlinkFrBack() : nullptr;
        iterateChildren(nodep);
        uint32_t ticks = 1;
        यदि (nodep->ticksp()) {
            UASSERT_OBJ(VN_IS(nodep->ticksp(), Const), nodep,
                        "Expected constant ticks, checked in V3Width");
            ticks = VN_AS(nodep->ticksp(), Const)->toUInt();
        }
        UASSERT_OBJ(ticks >= 1, nodep, "0 tick should have been checked in V3Width");
        Ast_गाँठो_अभिव्यक्ति* अचर exprp = new_Sampled_अभिव्यक्ति(nodep->exprp()->unlinkFrBack());
        Ast_Sen_रूख* अचर senTreep = nodep->sentreep()->unlinkFrBack();
        Ast_गाँठो_अभिव्यक्ति* inp = nullptr;
        // Deliberately emitted for every ftask $past; calls outside final take the normal stage
        यदि (VN_IS(m_procedurep, Final) || m_ftaskp) {
            // A sampling-tick finish reads one stage deeper (IEEE 1800-2023 16.9.3).
            File_पङ्क्ति* अचर flp = nodep->fileline();
            AstAlways* अचर alwaysp
                = getDelayedAlways(exprp, initp ? initp->clone_रूख_Pure(असत्य) : nullptr, senTreep);
            Ast_गाँठो_अभिव्यक्ति* अचर betweenTicksp = past_मान_Ref(alwaysp, flp, ticks, initp);
            Ast_गाँठो_अभिव्यक्ति* अचर onTickp = past_मान_Ref(alwaysp, flp, ticks + 1, initp);
            यदि (initp) VL_DO_DANGLING(थप_Deletep(initp), initp);
            Ast_गाँठो_अभिव्यक्ति* अचर onTickCondp = नूतन AstLogAnd{
                flp, नूतन AstCExpr{flp, "vlSymsp->_vm_contextp__->executingFinal()", 1},
                नूतन AstEq{flp, नूतन AstVarRef{flp, getPastTickTimeVar(alwaysp), VAccess::READ},
                          नूतन AstCExpr{flp, "vlSymsp->_vm_contextp__->finishPendingTime()", 64}}};
            AstCond* अचर condp = नूतन AstCond{flp, onTickCondp, onTickp, betweenTicksp};
            condp->dtypeFrom(onTickp);
            inp = condp;
        } अन्यथा {
            inp = get_Past_मान(exprp, initp, senTreep, ticks);
        }
        nodep->replaceWith(inp);
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }

    //========== Move $sampled down to read-only variables
    शून्य visit(AstSampled* nodep) override {
        यदि (nodep->user1()) फर्काऊ;
        VL_RESTORER(m_inSampled);
        {
            m_inSampled = सत्य;
            iterateChildren(nodep);
        }
        यदि (nodep->exprp()) {
            nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(Ast_गाँठो_Var_Ref* nodep) override {
        यदि (m_inSampled && !nodep->user2() && !nodep->varp()->isTemp()) {
            यदि (!nodep->access().isReadOnly()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Write to variable in sampled expression");
            } अन्यथा {
                VNRelinker relinkHandle;
                nodep->unlinkFrBack(&relinkHandle);
                AstSampled* अचर newp = new_Sampled_अभिव्यक्ति(nodep);
                relinkHandle.relink(newp);
                newp->user1(1);
                v3Global.setHasSampled();
            }
        }
    }
    // Don't sample sensitivities
    शून्य visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSampled);
        m_inSampled = असत्य;
        iterateChildren(nodep);
    }
    शून्य visit(AstPExprClause* nodep) override {
        Ast_गाँठो* stmtsp = nullptr;
        यदि (nodep->pass() && m_passsp) {
            // Cover adds COVERINC by AstNode::addNext, thus need to clone next too.
            stmtsp = m_passsp->clone_रूख(सत्य);
            stmtsp = newIfAssertPassOn(stmtsp, m_assertp->directive(), m_assertp->userType(),
                                       nodep->vacuous());
        } अन्यथा यदि (!nodep->pass() && m_failsp) {
            stmtsp = m_failsp->clone_रूख(सत्य);
            stmtsp = newIfAssertFailOn(stmtsp, m_assertp->directive(), m_assertp->userType());
        }
        यदि (stmtsp) {
            stmtsp->foreachAndNext([](Ast_गाँठो_Var_Ref* अचर refp) {
                // References inside action blocks shouldn't be implicitly sampled
                // m_passsp/m_failsp have been already visited once and refs explicitly sampled
                // are handled already
                refp->user2(1);
            });
            nodep->replaceWith(stmtsp);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(AstPExpr* nodep) override {
        यदि (m_inRestrict) {
            VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
        } अन्यथा {
            iterateChildren(nodep);
        }
    }

    //========== Statements
    शून्य visit(AstDisplay* nodep) override {
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
            File_पङ्क्ति* अचर fl = nodep->fileline();

            AstSenItem* monSenItemsp = nullptr;
            यदि (Ast_गाँठो* अचर monExprsp = nodep->fmtp()->exprsp()) {
                monExprsp->foreachAndNext([&](AstVarRef* varrefp) {
                    AstSenItem* अचर senItemp
                        = नूतन AstSenItem{fl, VEdgeType::ET_CHANGED,
                                         // Clone so get VarRef or VarXRef as needed
                                         varrefp->clone_रूख(असत्य)};
                    monSenItemsp = Ast_गाँठो::addNextNull(monSenItemsp, senItemp);
                });
            }

            Ast_Sen_रूख* अचर mon_Sen_रूख = नूतन Ast_Sen_रूख{fl, monSenItemsp};
            अचर स्वतः mon_अङ्क = ++m_monitor_अङ्क;
            // Where $monitor was we do "__VmonitorNum = N;"
            AstAssign* अचर newsetp = नूतन AstAssign{
                fl, new_Monitor_अङ्क_Var_Refp(nodep, VAccess::WRITE), नूतन AstConst{fl, mon_अङ्क}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__VmonitorOn && __VmonitorNum==N) $display(...);"
            Ast_गाँठो* अचर stmtsp = nodep;
            AstIf* अचर ifp = नूतन AstIf{
                fl,
                नूतन AstLogAnd{fl, नूतन AstLogNot{fl, newMonitorOffVarRefp(nodep, VAccess::READ)},
                              नूतन AstEq{fl, नूतन AstConst{fl, mon_अङ्क},
                                        new_Monitor_अङ्क_Var_Refp(nodep, VAccess::READ)}},
                stmtsp};
            ifp->isBoundsCheck(सत्य);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_गाँठो* अचर newp = नूतन AstAlways{fl, VAlwaysKwd::ALWAYS, mon_Sen_रूख, ifp};
            m_modp->addStmtsp(newp);
        } अन्यथा यदि (nodep->displayType() == VDisplayType::DT_STROBE) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            // Need one-shot
            File_पङ्क्ति* अचर fl = nodep->fileline();
            AstVar* अचर varp
                = नूतन AstVar{fl, VVarType::MODULETEMP, "__Vstrobe" + cvtToStr(m_mod_Strobe_अङ्क++),
                             nodep->findBitDType()};
            m_modp->addStmtsp(varp);
            // Where $strobe was we do "__Vstrobe = '1;"
            AstAssign* अचर newsetp = नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                                     नूतन AstConst{fl, AstConst::BitTrue{}}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__Vstrobe) begin $display(...); __Vstrobe = '0; end"
            Ast_गाँठो* अचर stmtsp = nodep;
            AstIf* अचर ifp = नूतन AstIf{fl, नूतन AstVarRef{fl, varp, VAccess::READ}, stmtsp};
            ifp->isBoundsCheck(सत्य);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_गाँठो* अचर newp = नूतन AstAlwaysPostponed{fl, ifp};
            stmtsp->addNext(नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                          नूतन AstConst{fl, AstConst::BitFalse{}}});
            m_modp->addStmtsp(newp);
        }
    }
    शून्य visit(AstMonitorOff* nodep) override {
        AstAssign* अचर newp
            = नूतन AstAssign{nodep->fileline(), newMonitorOffVarRefp(nodep, VAccess::WRITE),
                            नूतन AstConst{nodep->fileline(), AstConst::BitTrue{}, nodep->off()}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(AstAssert* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    शून्य visit(AstAssertCtl* nodep) override {
        iterateChildren(nodep);

        तार्किक assertTypeConst = सत्य;
        यदि (!nodep->assertTypesp()) {
            nodep->assertTypes(VAssertType{ALL_ASSERT_TYPES});
        } अन्यथा यदि (अचर AstConst* अचर assertTypesp = VN_CAST(nodep->assertTypesp(), Const)) {
            nodep->assertTypes(VAssertType{assertTypesp->toSInt()});
        } अन्यथा {
            assertTypeConst = असत्य;
        }

        तार्किक controlTypeConst = असत्य;
        यदि (अचर AstConst* अचर constp = VN_CAST(nodep->controlTypep(), Const)) {
            nodep->ctlType(constp->toSInt());
            controlTypeConst = सत्य;
        }
        यदि (controlTypeConst
            && (nodep->ctlType() < VAssertCtlType::LOCK
                || nodep->ctlType() > VAssertCtlType::VACUOUS_OFF)) {
            nodep->unlinkFrBack();
            nodep->v3error("Bad $assertcontrol control_type '"
                           << cvtToStr(static_cast<पूर्णाङ्क>(nodep->ctlType()))
                           << "' (IEEE 1800-2023 Table 20-5)");
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
            फर्काऊ;
        }
        यदि (assertTypeConst && nodep->assertTypes() != ALL_ASSERT_TYPES
            && nodep->assertTypes().containsAny(VAssertType::UNIQUE | VAssertType::UNIQUE0
                                                | VAssertType::PRIORITY)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: assert control assertion_type");
            VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
            फर्काऊ;
        }

        तार्किक directiveTypeConst = सत्य;
        यदि (!nodep->directiveTypesp()) {
            nodep->directiveTypes(VAssertDirectiveType::ASSERT | VAssertDirectiveType::ASSUME
                                  | VAssertDirectiveType::COVER);
        } अन्यथा यदि (अचर AstConst* अचर directiveTypesp
                   = VN_CAST(nodep->directiveTypesp(), Const)) {
            nodep->directiveTypes(VAssertDirectiveType{directiveTypesp->toSInt()});
        } अन्यथा {
            directiveTypeConst = असत्य;
        }
        यदि (!directiveTypeConst) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: non-const assert directive type expression");
            VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
            फर्काऊ;
        }

        File_पङ्क्ति* अचर fl = nodep->fileline();
        UINFO(9, "Generating assertctl for a module: " << m_modp);
        AstCStmt* अचर newp = नूतन AstCStmt{fl};
        newp->add("vlSymsp->_vm_contextp__->assertCtl(");
        newp->add(nodep->controlTypep()->unlinkFrBack());
        newp->add(", ");
        यदि (nodep->assertTypesp()) {
            newp->add(nodep->assertTypesp()->unlinkFrBack());
        } अन्यथा {
            newp->add(std::to_string(ALL_ASSERT_TYPES));
        }
        newp->add(", " + std::to_string(nodep->directiveTypes()) + ");\n");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(AstAssertIntrinsic* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    शून्य visit(AstCover* nodep) override {  //
        visitAssertionIterate(nodep, nullptr);
    }
    शून्य visit(AstRestrict* nodep) override {
        VL_RESTORER(m_inRestrict);
        m_inRestrict = सत्य;
        iterateChildren(nodep);
        // IEEE says simulator ignores these
        VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
    }

    शून्य visit(Ast_गाँठो_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Past_अङ्क);
        VL_RESTORER(m_mod_Strobe_अङ्क);
        VL_RESTORER(m_finalp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DelayedAlwaysp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DefaultDelayedAlwaysp);
        VL_RESTORER_CLEAR(m_delayedAlways2TickTimep);
        m_modp = nodep;
        m_mod_Past_अङ्क = 0;
        m_mod_Strobe_अङ्क = 0;
        m_finalp = nullptr;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठो_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठो_FTask* nodep) override {
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
        यदि (!nodep->declsp()) {
            यदि (AstIf* अचर ifp = VN_CAST(nodep->stmtsp(), If)) {
                यदि (ifp->user2() && !ifp->nextp()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(ifp->unlinkFrBack());
                    nodep->addStmtsp(ifp->thensp()->unlinkFrBackWithNext());
                    ifp->addThensp(nodep);
                }
            }
        }
    }

    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

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
        VIsCached::clear_Cache_रूख();
    }
};

//######################################################################
// Top Assert class

शून्य V3Assert::assertAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_रूख("assert", 0, dump_रूख_Either_Level() >= 3);
}
