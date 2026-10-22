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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Assert.h"

#समावेश "V3AstUserAllocator.h"
#समावेश "V3Const.h"
#समावेश "V3Stats.h"
#समावेश "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

namespace {

श्रेणी DefaultDisableLocalVisitor final : खुला VNVisitor {
    // STATE
    Ast_गाँठ* m_scopep = nullptr;

    // VISITORS
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(nullptr);
        iterateChildren(nodep);
    }
    व्योम visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        nodep->defaultDisablep(nullptr);
        iterateChildren(nodep);
    }
    व्योम visit(AstDefaultDisable* nodep) override {
        UASSERT_OBJ(nodep, m_scopep,
                    "default disable iff must be inside a module or generate block");
        AstDefaultDisable* defaultp = nullptr;
        अगर (स्थिर Ast_गाँठ_Module* स्थिर modp = VN_CAST(m_scopep, गाँठ_Module)) {
            defaultp = modp->defaultDisablep();
        } अन्यथा {
            defaultp = VN_AS(m_scopep, GenBlock)->defaultDisablep();
        }
        अगर (VL_UNLIKELY(defaultp)) {
            nodep->v3error("Only one 'default disable iff' allowed per "
                           << (VN_IS(m_scopep, गाँठ_Module) ? "module" : "generate block")
                           << " (IEEE 1800-2023 16.15)");
        } अन्यथा अगर (Ast_गाँठ_Module* स्थिर modp = VN_CAST(m_scopep, गाँठ_Module)) {
            modp->defaultDisablep(nodep);
        } अन्यथा {
            VN_AS(m_scopep, GenBlock)->defaultDisablep(nodep);
        }
    }
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    explicit DefaultDisableLocalVisitor(AstNetlist* nodep) { iterate(nodep); }
};

श्रेणी DefaultDisablePropagateVisitor final : खुला VNVisitor {
    // STATE
    AstDefaultDisable* m_defaultDisablep = nullptr;

    // VISITORS
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    व्योम visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        अगर (!nodep->defaultDisablep()) nodep->defaultDisablep(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    explicit DefaultDisablePropagateVisitor(AstNetlist* nodep) { iterate(nodep); }
};

// Lower a sequence used as an event control ('@seq', IEEE 1800-2023 9.4.2.4) into a
// synthesized event fired by an internal 'cover sequence' on each end-of-match
श्रेणी SeqEventLowerVisitor final : खुला VNVisitor {
    // STATE
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    V3UniqueNames m_names{"__VseqEvent"};  // Synthesized event names

    // VISITORS
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(AstSenItem* nodep) override {
        AstFuncRef* स्थिर funcrefp = VN_CAST(nodep->sensp(), FuncRef);
        अगर (funcrefp && VN_IS(funcrefp->taskp(), Sequence)) {
            File_पंक्ति* स्थिर flp = nodep->fileline();
            AstVar* स्थिर eventp = नया AstVar{flp, VVarType::MODULETEMP, m_names.लो(nodep),
                                              m_modp->findBasicDType(VBasicDTypeKwd::EVENT)};
            eventp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(eventp);
            v3Global.setHasEvents();
            funcrefp->unlinkFrBack();
            nodep->sensp(नया AstVarRef{flp, eventp, VAccess::READ});
            स्थिर bool automaticActual = funcrefp->exists([](स्थिर Ast_गाँठ_Var_Ref* refp) {
                वापस refp->varp() && refp->varp()->lifetime().isAutomatic();
            });
            अगर (automaticActual) {
                nodep->v3error("Arguments to a sequence used as an event control must be"
                               " static (IEEE 1800-2023 9.4.2.4)");
                VN_AS(funcrefp->taskp(), Sequence)->isReferenced(false);
                VL_DO_DANGLING(धकेलो_Deletep(funcrefp), funcrefp);
                वापस;
            }
            AstFireEvent* स्थिर firep
                = नया AstFireEvent{flp, नया AstVarRef{flp, eventp, VAccess::WRITE}, false};
            AstCover* स्थिर coverp
                = नया AstCover{flp, नया AstPropSpec{flp, nullptr, nullptr, funcrefp}, firep,
                               VAssertType::CONCURRENT};
            coverp->isCoverSeq(true);
            coverp->isSeqEvent(true);
            m_modp->addStmtsp(coverp);
            वापस;
        }
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    explicit SeqEventLowerVisitor(AstNetlist* nodep) { iterate(nodep); }
};

}  // namespace

व्योम V3AssertCommon::collectDefaultDisable(AstNetlist* nodep) {
    { DefaultDisableLocalVisitor{nodep}; }
    { DefaultDisablePropagateVisitor{nodep}; }
}

व्योम V3AssertCommon::lowerSequenceEvents(AstNetlist* nodep) {
    { SeqEventLowerVisitor{nodep}; }
    V3Global::dump_Check_Global_वृक्ष("assertseqevent", 0, dump_वृक्ष_Either_Level() >= 3);
}

string V3AssertCommon::assertCtlGetCall(स्थिर अक्षर* query, VAssertType type,
                                        VAssertDirectiveType directiveType) {
    // Template arguments let the runtime build the type/directive mask at C++ compile time
    वापस "vlSymsp->_vm_contextp__->assertCtlGet<"s + std::to_string(type) + ", "s
           + std::to_string(directiveType) + ">(VerilatedAssertCtlQuery::"s + query + ")"s;
}

bool V3AssertCommon::check_Sva_मूल्य_Limit(Ast_गाँठ* nodep, स्थिर AstConst* constp, स्थिर अक्षर* what) {
    // Temporal values can materialize as O(N) AST or state in later passes.
    // Leave headroom for a range ring's extra slot and V3Number's signed
    // `(bits + 31) / 32` word count.
    अटल constexpr अचिन्हित SVA_मूल्य_HARD_LIMIT
        = static_cast<अचिन्हित>(std::numeric_limits<पूर्णांक>::max()) - 32U;
    स्थिर अचिन्हित configuredLimit = static_cast<अचिन्हित>(v3Global.opt.max_अंक_Width());
    स्थिर अचिन्हित limit = std::min(configuredLimit, SVA_मूल्य_HARD_LIMIT);
    अगर (constp->अंक().fitsInUInt() && constp->toUInt() <= limit) वापस true;
    nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << what << " exceeds implementation limit of "
                                                 << limit
                                                 << (configuredLimit <= SVA_मूल्य_HARD_LIMIT
                                                         ? " (--max-num-width)"
                                                         : " (host arithmetic limit)"));
    वापस false;
}

// The bound of a cycle delay in a sequence, if a non-negative elaboration-time constant
अटल स्थिर AstConst* checkSvaDelayBound(Ast_गाँठ* errp, Ast_गाँठ_व्यंजक* boundp, स्थिर अक्षर* what) {
    स्थिर AstConst* स्थिर constp = VN_CAST(boundp, Const);
    अगर (!constp || (constp->dtypep()->isSigned() && constp->अंक().isNegative())) {
        errp->v3error(what << " is not a non-negative elaboration-time constant"
                              " (IEEE 1800-2023 16.7)");
        वापस nullptr;
    }
    अगर (!V3AssertCommon::check_Sva_मूल्य_Limit(errp, constp, "SVA cycle delay")) वापस nullptr;
    वापस constp;
}

// Fold the bounds of a cycle delay in a sequence, and return if they are valid, else report it
// on errp
अटल bool svaCycleDelayValid(स्थिर AstDelay* nodep, Ast_गाँठ* errp) {
    V3Const::constifyParamsNoWarnEdit(nodep->lhsp());
    स्थिर AstConst* स्थिर minConstp = checkSvaDelayBound(
        errp, nodep->lhsp(), nodep->isRangeDelay() ? "Range delay minimum" : "Delay value");
    अगर (!nodep->rhsp() || nodep->isUnbounded()) वापस minConstp != nullptr;
    V3Const::constifyParamsNoWarnEdit(nodep->rhsp());
    स्थिर AstConst* स्थिर maxConstp
        = checkSvaDelayBound(errp, nodep->rhsp(), "Range delay maximum");
    अगर (!minConstp || !maxConstp) वापस false;
    अगर (maxConstp->toUInt() < minConstp->toUInt()) {
        errp->v3error("Range delay maximum must be >= minimum (IEEE 1800-2023 16.7)");
        वापस false;
    }
    वापस true;
}

व्योम V3AssertCommon::checkSvaCycleDelay(AstDelay* nodep, Ast_गाँठ* errp) {
    अगर (svaCycleDelayValid(nodep, errp)) वापस;
    // Make the delay valid, so later steps do not fail after the error
    File_पंक्ति* स्थिर flp = nodep->fileline();
    Ast_गाँठ_व्यंजक* स्थिर minp = nodep->lhsp();
    minp->replaceWith(नया AstConst{flp, AstConst::BitFalseErroring{}});
    VL_DO_DANGLING(minp->delete_वृक्ष(), minp);
    अगर (nodep->rhsp() && !nodep->isUnbounded()) {
        Ast_गाँठ_व्यंजक* स्थिर maxp = nodep->rhsp();
        maxp->replaceWith(नया AstConst{flp, AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(maxp->delete_वृक्ष(), maxp);
    }
}

bool V3AssertCommon::refsSvaFormal(स्थिर Ast_गाँठ* nodep) {
    वापस nodep->exists([](स्थिर Ast_गाँठ_Var_Ref* refp) {
        स्थिर AstVar* स्थिर varp = refp->varp();
        अगर (!varp || !varp->isIO()) वापस false;
        स्थिर Ast_गाँठ* स्थिर declp = varp->aboveLoopp();
        वापस VN_IS(declp, Sequence) || VN_IS(declp, Property);
    });
}

व्योम V3AssertCommon::checkSvaInstanceDelays(Ast_गाँठ* nodep, Ast_गाँठ* instp) {
    // A delay still referencing a formal argument of an enclosing sequence or property is
    // checked when that is instanced. Collect first, as checking edits the delays.
    std::vector<AstDelay*> delayps;
    nodep->foreach([&](AstDelay* delayp) {
        अगर (!delayp->isCycleDelay() || refsSvaFormal(delayp->lhsp())) वापस;
        अगर (delayp->rhsp() && refsSvaFormal(delayp->rhsp())) वापस;
        delayps.धकेलो_back(delayp);
    });
    क्रम (AstDelay* स्थिर delayp : delayps) checkSvaCycleDelay(delayp, instp);
}

//######################################################################
// AssertDeFutureVisitor
// If any AstFuture, then move all non-future varrefs to be one cycle behind,
// see IEEE 1800-2023 16.9.4.

श्रेणी AssertDeFutureVisitor final : खुला VNVisitor {
    // STATE - across all visitors
    Ast_गाँठ_Module* स्थिर m_modp;  // Module future is underneath
    स्थिर AstFuture* m_futurep;  // First AstFuture found
    स्थिर अचिन्हित m_past_अंक;  // Prefix unique number for this module
    std::map<AstVar*, AstVar*> m_delayedVars;  // Old to delayed variable mapping
    // STATE - for current visit position (use VL_RESTORER)
    bool m_inFuture = false;  // Inside a future
    bool m_unsupported = false;  // Printed unsupported

    // METHODS
    व्योम unsupported(Ast_गाँठ* nodep) {
        अगर (m_unsupported) वापस;
        m_unsupported = true;
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported/illegal: Future value function used with expression with "
                          << nodep->prettyOperatorName());
    }
    // VISITORS
    व्योम visit(AstFuture* nodep) override {
        VL_RESTORER(m_inFuture);
        m_inFuture = true;
        iterateChildren(nodep);
        // Done with the future, this subexpression is current-time
        nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(Ast_गाँठ_Var_Ref* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;
        अगर (m_inFuture || m_unsupported)
            वापस;  // Need user1 set above, don't process when Future is removed
        अगर (nodep->access().isWriteOrRW()) {
            unsupported(nodep);
            वापस;
        }
        स्वतः it = m_delayedVars.find(nodep->varp());
        AstVar* outvarp;
        अगर (it == m_delayedVars.end()) {
            Ast_Sen_वृक्ष* स्थिर sentreep = m_futurep->sentreep();
            AstAlways* स्थिर alwaysp = नया AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS,
                                                     sentreep->clone_वृक्ष(false), nullptr};
            m_modp->addStmtsp(alwaysp);
            outvarp = नया AstVar{nodep->fileline(), VVarType::MODULETEMP,
                                 "__Vnotfuture" + cvtToStr(m_past_अंक) + "_" + nodep->name(),
                                 nodep->dtypep()};
            m_modp->addStmtsp(outvarp);
            AstVarRef* varRefAWritep = नया AstVarRef{nodep->fileline(), outvarp, VAccess::WRITE};
            varRefAWritep->user1(true);
            Ast_गाँठ_Var_Ref* varRefAReadp = nodep->clone_वृक्ष(false);
            varRefAReadp->user1(true);
            Ast_गाँठ* स्थिर assp = नया AstAssignDly{nodep->fileline(), varRefAWritep, varRefAReadp};
            alwaysp->addStmtsp(assp);
            m_delayedVars.emplace(nodep->varp(), outvarp);
        } अन्यथा {
            outvarp = it->second;
        }
        AstVarRef* newp = नया AstVarRef{nodep->fileline(), outvarp, VAccess::READ};
        newp->user1(true);
        UINFO(9, "DeFuture " << nodep << "  becomes " << newp);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(Ast_गाँठ_FTask_Ref* nodep) override { unsupported(nodep); }
    व्योम visit(AstMethodCall* nodep) override { unsupported(nodep); }
    व्योम visit(Ast_गाँठ* nodep) override {
        अगर (!nodep->isPure()) unsupported(nodep);
        iterateChildren(nodep);
    }

खुला:
    // CONSTRUCTORS
    explicit AssertDeFutureVisitor(Ast_गाँठ* nodep, Ast_गाँठ_Module* modp, अचिन्हित past_अंक)
        : m_modp{modp}
        , m_past_अंक{past_अंक} {
        // See if any Future before we process
        अगर (nodep->forall([&](स्थिर AstFuture* futurep) -> bool {
                m_futurep = futurep;
                वापस false;
            }))
            वापस;
        // UINFOTREE(9, nodep, "", "defuture-in");
        visit(nodep);  // Nodep may get deleted
        // UINFOTREE(9, nodep, "", "defuture-ou");
    }
    ~AssertDeFutureVisitor() = शेष;
};

// Prepare one default sampled expression.
श्रेणी Default_Sampled_मूल्य_Visitor final : खुला VNVisitor {
    // METHODS
    अटल AstConst* newTypeDefault(Ast_गाँठ_व्यंजक* nodep) {
        AstConst* स्थिर constp
            = नया AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()};
        अगर (nodep->dtypep()->isFourstate()) constp->अंक().setAllBitsX();
        constp->dtypeFrom(nodep);
        वापस constp;
    }

    // VISITORS
    व्योम visit(AstCMethodHard* nodep) override {
        अगर (nodep->method() == VCMethod::EVENT_IS_TRIGGERED) {
            nodep->replaceWith(नया AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा {
            iterateChildren(nodep);
        }
    }
    व्योम visit(Ast_गाँठ_Var_Ref* nodep) override {
        AstVar* स्थिर varp = nodep->varp();
        अगर (varp->isParam()
            || (!varp->isNet() && varp->lifetime().isStatic() && varp->hasUserInit())) {
            वापस;
        }
        nodep->replaceWith(newTypeDefault(nodep));
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    explicit Default_Sampled_मूल्य_Visitor(Ast_गाँठ* nodep) { iterate(nodep); }
};

//######################################################################
// AssertVisitor

श्रेणी AssertVisitor final : खुला VNVisitor {
    // CONSTANTS
    अटल constexpr uint8_t ALL_ASSERT_TYPES
        = std::numeric_limits<std::underlying_type<VAssertType::en>::type>::max();

    // NODE STATE/TYPES
    // Cleared on netlist
    //  AstNode::user1()         -> bool.  True if processed
    //  AstAlways::user2p()      -> std::vector<AstVar*>. Delayed variables via 'm_delayed'
    //  AstNodeVarRef::user2()   -> bool.  True if shouldn't be sampled
    //  AstIf::user2()           -> bool.  True for assertOn() checks inserted in this pass
    स्थिर VNUser1InUse m_user1InUse;
    स्थिर VNUser2InUse m_user2InUse;
    AstUser2Allocator<AstAlways, std::vector<AstVar*>> m_delayed;

    // STATE
    Ast_गाँठ_Module* m_modp = nullptr;  // Last module
    स्थिर Ast_गाँठ* m_beginp = nullptr;  // Last AstBegin/AstGenBlock
    अचिन्हित m_monitor_अंक = 0;  // Global $monitor numbering (not per module)
    AstVar* m_monitor_अंक_Varp = nullptr;  // $monitor number variable
    AstVar* m_monitorOffVarp = nullptr;  // $monitoroff variable
    अचिन्हित m_mod_Past_अंक = 0;  // Module past numbering
    अचिन्हित m_mod_Strobe_अंक = 0;  // Module $strobe numbering
    Ast_गाँठ_Procedure* m_procedurep = nullptr;  // Current procedure
    VDouble0 m_statCover;  // Statistic tracking
    VDouble0 m_statAsNotImm;  // Statistic tracking
    VDouble0 m_statAsImm;  // Statistic tracking
    VDouble0 m_statAsFull;  // Statistic tracking
    VDouble0 m_statPastVars;  // Statistic tracking
    VDouble0 m_statAssertOnCombined;  // Statistic tracking
    VDouble0 m_statAssertOnHoisted;  // Statistic tracking
    bool m_inSampled = false;  // True inside a sampled expression
    bool m_inRestrict = false;  // True inside restrict assertion
    Ast_गाँठ* m_passsp = nullptr;  // Current pass statement
    Ast_गाँठ* m_failsp = nullptr;  // Current fail statement
    Ast_गाँठ_Cover_Or_Assert* m_assertp = nullptr;  // Current assertion
    AstFinal* m_finalp = nullptr;  // Current final block
    VDouble0 m_statLiftedCaseExprs;  // Count of purified case expressions
    Ast_गाँठ_FTask* m_ftaskp = nullptr;  // Current function/task
    V3UniqueNames m_case_अस्थायी_Names{"__VCase"};
    V3UniqueNames m_match_गिनती_Names{"__VnfaRemainingMatchCount"};  // Match replay counter names
    // Maps from (expression, senTree) to the AstAlways that computes its delayed values.
    std::unordered_map<VNRef<Ast_गाँठ_व्यंजक>, std::unordered_map<VNRef<Ast_Sen_वृक्ष>, AstAlways*>>
        m_modExpr2Sen2DelayedAlwaysp;
    // Source sampled-value history is initialized, unlike internal property-lowering history.
    std::unordered_map<VNRef<Ast_गाँठ_व्यंजक>, std::unordered_map<VNRef<Ast_Sen_वृक्ष>, AstAlways*>>
        m_modExpr2Sen2DefaultDelayedAlwaysp;
    // Map from delayed-value AstAlways to its last-sampling-tick time variable
    std::unordered_map<स्थिर AstAlways*, AstVar*> m_delayedAlways2TickTimep;

    // METHODS
    अटल स्थिर अक्षर* assertPassOnQuery(bool vacuous) {
        अटल constexpr स्थिर अक्षर* queries[2]
            = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
        वापस queries[vacuous];
    }
    अटल Ast_गाँठ_व्यंजक* assertOnCond(File_पंक्ति* fl, VAssertType type,
                                     VAssertDirectiveType directiveType) {
        // cppcheck-suppress missingReturn
        चयन (directiveType) {
        हाल VAssertDirectiveType::INTRINSIC: वापस नया AstConst{fl, AstConst::BitTrue{}};
        हाल VAssertDirectiveType::VIOLATION_CASE: {
            अगर (v3Global.opt.assertCase()) {
                वापस नया AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            // If assertions are off, have constant propagation rip them out later
            // This allows syntax errors and such to be detected normally.
            वापस नया AstConst{fl, AstConst::BitFalse{}};
        }
        हाल VAssertDirectiveType::ASSERT:
        हाल VAssertDirectiveType::COVER:
        हाल VAssertDirectiveType::ASSUME: {
            अगर (v3Global.opt.assertOn()) {
                वापस नया AstCExpr{
                    fl, AstCExpr::Pure{},
                    V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
            }
            वापस नया AstConst{fl, AstConst::BitFalse{}};
        }
        हाल VAssertDirectiveType::INTERNAL:
        हाल VAssertDirectiveType::VIOLATION_IF:
        हाल VAssertDirectiveType::RESTRICT: {
            अगर (v3Global.opt.assertOn()) {
                वापस नया AstCExpr{fl, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()",
                                    1};
            }
            वापस नया AstConst{fl, AstConst::BitFalse{}};
        }
        }
        VL_UNREACHABLE;
    }

    अटल bool isControlled(VAssertDirectiveType directiveType) {
        वापस (static_cast<पूर्णांक>(directiveType)
                & (static_cast<पूर्णांक>(VAssertDirectiveType::ASSERT)
                   | static_cast<पूर्णांक>(VAssertDirectiveType::COVER)
                   | static_cast<पूर्णांक>(VAssertDirectiveType::ASSUME)));
    }
    अटल Ast_गाँठ_व्यंजक* assertPassOnCond(File_पंक्ति* fl, VAssertType type,
                                         VAssertDirectiveType directiveType, bool vacuous) {
        अगर (!isControlled(directiveType)) वापस नया AstConst{fl, AstConst::BitTrue{}};
        अगर (!v3Global.opt.assertOn()) वापस नया AstConst{fl, AstConst::BitFalse{}};
        वापस नया AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType), 1};
    }
    अटल Ast_गाँठ_व्यंजक* assertFailOnCond(File_पंक्ति* fl, VAssertType type,
                                         VAssertDirectiveType directiveType) {
        अगर (!isControlled(directiveType)) वापस नया AstConst{fl, AstConst::BitTrue{}};
        अगर (!v3Global.opt.assertOn()) वापस नया AstConst{fl, AstConst::BitFalse{}};
        वापस नया AstCExpr{
            fl, AstCExpr::Pure{},
            V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType), 1};
    }
    string assertDisplayMessage(स्थिर Ast_गाँठ* nodep, स्थिर string& prefix, स्थिर string& message,
                                VDisplayType severity) {
        अगर (severity == VDisplayType::DT_ERROR || severity == VDisplayType::DT_FATAL) {
            वापस ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                    + cvtToStr(nodep->fileline()->lineno()) + ": Assertion failed in %m"
                    + ((message != "") ? ": " : "") + message + "\n");
        }
        वापस ("[%0t] "s + prefix + ": " + nodep->fileline()->filebasename() + ":"
                + cvtToStr(nodep->fileline()->lineno()) + ": %m" + ((message != "") ? ": " : "")
                + message + "\n");
    }
    व्योम replaceDisplay(AstDisplay* nodep, स्थिर string& prefix) {
        nodep->fmtp()->text(
            assertDisplayMessage(nodep, prefix, nodep->fmtp()->text(), nodep->displayType()));
        nodep->displayType(VDisplayType::DT_WRITE);
        // cppcheck-suppress nullPointer
        Ast_गाँठ_व्यंजक* स्थिर timenewp = नया AstTime{nodep->fileline(), m_modp->timeunit()};
        nodep->fmtp()->prependExprsp(timenewp);
        अगर (!nodep->fmtp()->scopeNamep() && nodep->fmtp()->formatScopeTracking()) {
            nodep->fmtp()->scopeNamep(नया AstScopeName{nodep->fileline(), true});
        }
    }
    AstSampled* new_Sampled_व्यंजक(Ast_गाँठ_व्यंजक* nodep) {
        वापस नया AstSampled{nodep->fileline(), nodep, nodep->dtypep(), true};
    }
    AstVarRef* new_Monitor_अंक_Var_Refp(स्थिर Ast_गाँठ* nodep, VAccess access) {
        अगर (!m_monitor_अंक_Varp) {
            m_monitor_अंक_Varp = नया AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorNum",
                                          nodep->findUInt64DType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitor_अंक_Varp);
        }
        AstVarRef* स्थिर varrefp = नया AstVarRef{nodep->fileline(), m_monitor_अंक_Varp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        वापस varrefp;
    }
    AstVarRef* newMonitorOffVarRefp(स्थिर Ast_गाँठ* nodep, VAccess access) {
        अगर (!m_monitorOffVarp) {
            m_monitorOffVarp = नया AstVar{nodep->fileline(), VVarType::MODULETEMP, "__VmonitorOff",
                                          nodep->findBitDType()};
            v3Global.rootp()->dollarUnitPkgp()->addStmtsp(m_monitorOffVarp);
        }
        AstVarRef* स्थिर varrefp = नया AstVarRef{nodep->fileline(), m_monitorOffVarp, access};
        varrefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgp());
        वापस varrefp;
    }
    अटल AstIf* newIfAssertOn(Ast_गाँठ* bodyp, VAssertDirectiveType directiveType,
                                VAssertType type = VAssertType::INTERNAL) {
        // Add an internal if to check assertions are on.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_पंक्ति* स्थिर fl = bodyp->fileline();

        Ast_गाँठ_व्यंजक* स्थिर condp = assertOnCond(fl, type, directiveType);
        AstIf* स्थिर newp = नया AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(true);  // To avoid LATCH warning
        newp->user2(true);  // Mark as an assertOn() check
        वापस newp;
    }
    अटल Ast_गाँठ_Stmt* newIfAssertPassOn(Ast_गाँठ* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type, bool vacuous) {
        // Add an internal if to check assertion passOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_पंक्ति* स्थिर fl = bodyp->fileline();
        Ast_गाँठ_व्यंजक* स्थिर condp = assertPassOnCond(fl, type, directiveType, vacuous);
        Ast_गाँठ_If* स्थिर newp = नया AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(true);  // To avoid LATCH warning
        newp->user1(true);  // Don't assert/cover this if
        newp->user2(true);  // Mark as an assertOn() check
        वापस newp;
    }
    अटल Ast_गाँठ_Stmt* newIfAssertFailOn(Ast_गाँठ* bodyp, VAssertDirectiveType directiveType,
                                          VAssertType type) {
        // Add an internal if to check assertion failOn is enabled.
        // Don't make this an AND term, as it's unlikely to need to test this.
        File_पंक्ति* स्थिर fl = bodyp->fileline();
        Ast_गाँठ_व्यंजक* स्थिर condp = assertFailOnCond(fl, type, directiveType);
        Ast_गाँठ_If* स्थिर newp = नया AstIf{fl, condp, bodyp};
        newp->isBoundsCheck(true);  // To avoid LATCH warning
        newp->user1(true);  // Don't assert/cover this if
        newp->user2(true);  // Mark as an assertOn() check
        वापस newp;
    }

    अटल AstIf* assertCond(स्थिर Ast_गाँठ_Cover_Or_Assert* nodep, Ast_गाँठ_व्यंजक* propp,
                             Ast_गाँठ* passsp, Ast_गाँठ* failsp) {
        AstIf* स्थिर ifp = नया AstIf{nodep->fileline(), propp, passsp, failsp};
        // It's more LIKELY that we'll take the nullptr if clause
        // than the sim-killing else clause:
        ifp->branchPred(VBranchPred::BP_LIKELY);
        ifp->isBoundsCheck(true);  // To avoid LATCH warning
        वापस ifp;
    }

    Ast_गाँठ_Stmt* assertBody(स्थिर Ast_गाँठ_Cover_Or_Assert* nodep, Ast_गाँठ* propp, Ast_गाँठ* passsp,
                            Ast_गाँठ* failsp) {
        अगर (AstPExpr* स्थिर pexprp = VN_CAST(propp, PExpr)) {
            अगर (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NOTIMING, "This property expression requires --timing");
                VL_DO_DANGLING(धकेलो_Deletep(pexprp), pexprp);
                वापस नया AstBegin{nodep->fileline(), "", nullptr, false};
            }
            AstFork* स्थिर forkp = नया AstFork{nodep->fileline(), VJoinType::JOIN_NONE};
            forkp->addForksp(pexprp->bodyp()->unlinkFrBack());
            अगर (Ast_गाँठ_Stmt* स्थिर finalp = pexprp->finalp()) {
                अगर (!m_finalp) {
                    m_finalp = नया AstFinal{m_modp->fileline(), finalp->unlinkFrBack()};
                    m_modp->addStmtsp(m_finalp);
                } अन्यथा {
                    m_finalp->addStmtsp(finalp->unlinkFrBack());
                }
            }
            VL_DO_DANGLING2(धकेलो_Deletep(pexprp), pexprp, propp);
            वापस forkp;
        }

        वापस assertCond(nodep, VN_AS(propp, गाँठ_व्यंजक), passsp, failsp);
    }

    Ast_गाँठ_Stmt* newFireAssertUnchecked(स्थिर Ast_गाँठ_Stmt* nodep, स्थिर string& message,
                                        Ast_गाँठ_व्यंजक* exprsp = nullptr) {
        // Like newFireAssert() but omits the asserts-on check
        AstDisplay* स्थिर dispp
            = नया AstDisplay{nodep->fileline(), VDisplayType::DT_ERROR, message, nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        Ast_गाँठ_Stmt* स्थिर bodysp = dispp;
        replaceDisplay(dispp, "%%Error");  // Convert to standard DISPLAY format
        अगर (exprsp) dispp->fmtp()->exprsp()->add_अग्र(exprsp);
        अगर (v3Global.opt.stopFail()) bodysp->add_अग्र(नया AstStop{nodep->fileline(), false});
        वापस bodysp;
    }

    Ast_गाँठ_Stmt* newFireAssert(स्थिर Ast_गाँठ_Stmt* nodep, VAssertDirectiveType directiveType,
                               VAssertType assertType, स्थिर string& message,
                               Ast_गाँठ_व्यंजक* exprsp = nullptr) {
        Ast_गाँठ_Stmt* bodysp = newFireAssertUnchecked(nodep, message, exprsp);
        bodysp = newIfAssertOn(bodysp, directiveType, assertType);
        वापस bodysp;
    }

    AstVar* createDelayedVar(स्थिर std::string& name, AstAlways* alwaysp, Ast_गाँठ_व्यंजक* exprp,
                             Ast_गाँठ_व्यंजक* initp) {
        File_पंक्ति* स्थिर flp = exprp->fileline();
        AstVar* स्थिर varp = नया AstVar{flp, VVarType::MODULETEMP, name, exprp->dtypep()};
        // TODO: this lifetime seems nonsene (can't have NBAs to automatics), but is as before
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->addStmtsp(varp);
        // Before enough clock ticks, $past returns the expression's default sampled value.
        अगर (initp) {
            m_modp->addStmtsp(नया AstInitialStatic{
                flp, नया AstAssign{flp, नया AstVarRef{flp, varp, VAccess::WRITE}, initp}});
        }
        ++m_statPastVars;
        // Actually set the delayed value
        Ast_गाँठ_व्यंजक* स्थिर lhsp = नया AstVarRef{flp, varp, VAccess::WRITE};
        AstAssignDly* स्थिर assignp = नया AstAssignDly{flp, lhsp, exprp};
        alwaysp->prependStmtsp(assignp);
        वापस varp;
    }

    AstAlways* getDelayedAlways(Ast_गाँठ_व्यंजक* exprp, Ast_गाँठ_व्यंजक* initp, Ast_Sen_वृक्ष* senTreep) {
        स्वतः& expr2Sen2Alwaysr
            = initp ? m_modExpr2Sen2DefaultDelayedAlwaysp : m_modExpr2Sen2DelayedAlwaysp;
        AstAlways*& alwayspr = expr2Sen2Alwaysr[*exprp][*senTreep];
        अगर (!alwayspr) {
            File_पंक्ति* स्थिर flp = exprp->fileline();
            // Create the always block that computes the delayed values
            alwayspr = नया AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, nullptr};
            m_modp->addStmtsp(alwayspr);
            // Create the once-delayed variable
            स्थिर std::string name = "_Vpast_" + cvtToStr(m_mod_Past_अंक++) + "_1";
            AstVar* स्थिर varp = createDelayedVar(name, alwayspr, exprp, initp);
            // Add it to delayed variable vector
            m_delayed(alwayspr).emplace_back(varp);
        } अन्यथा {
            // Reusing exiting, not needed
            VL_DO_DANGLING(धकेलो_Deletep(exprp), exprp);
            अगर (initp) VL_DO_DANGLING(धकेलो_Deletep(initp), initp);
            VL_DO_DANGLING(धकेलो_Deletep(senTreep), senTreep);
        }
        वापस alwayspr;
    }

    Ast_गाँठ_व्यंजक* get_Past_मूल्य(Ast_गाँठ_व्यंजक* exprp, Ast_गाँठ_व्यंजक* initp, Ast_Sen_वृक्ष* senTreep,
                              uint32_t ticks) {
        UASSERT_OBJ(ticks > 0, exprp, "Delay must be > 0");
        File_पंक्ति* स्थिर flp = exprp->fileline();
        AstAlways* स्थिर alwaysp
            = getDelayedAlways(exprp, initp ? initp->clone_वृक्ष_Pure(false) : nullptr, senTreep);
        Ast_गाँठ_व्यंजक* स्थिर resultp = past_मूल्य_Ref(alwaysp, flp, ticks, initp);
        अगर (initp) VL_DO_DANGLING(धकेलो_Deletep(initp), initp);
        वापस resultp;
    }

    Ast_गाँठ_व्यंजक* past_मूल्य_Ref(AstAlways* alwaysp, File_पंक्ति* flp, uint32_t ticks,
                              Ast_गाँठ_व्यंजक* initp) {
        std::vector<AstVar*>& delayedr = m_delayed(alwaysp);
        // Ensure the required delay exists
        जबतक (delayedr.आकार() < ticks) {
            AstVar* स्थिर firstp = delayedr.front();
            File_पंक्ति* स्थिर varFlp = firstp->fileline();
            // Create once more delayed value
            std::string name = firstp->name();
            name.resize(name.आकार() - 1);
            name += std::to_string(delayedr.आकार() + 1);
            Ast_गाँठ_व्यंजक* स्थिर prevp = नया AstVarRef{varFlp, delayedr.back(), VAccess::READ};
            AstVar* स्थिर varp = createDelayedVar(name, alwaysp, prevp,
                                                  initp ? initp->clone_वृक्ष_Pure(false) : nullptr);
            // Add it to delayed variable vector
            delayedr.emplace_back(varp);
        }
        // Return a reference to the appropriately delayed variable
        वापस नया AstVarRef{flp, delayedr.at(ticks - 1), VAccess::READ};
    }

    // Time of the pipeline's last sampling tick, for end-of-simulation readers
    AstVar* getPastTickTimeVar(AstAlways* alwaysp) {
        AstVar*& varpr = m_delayedAlways2TickTimep[alwaysp];
        अगर (!varpr) {
            File_पंक्ति* स्थिर flp = alwaysp->fileline();
            varpr = नया AstVar{flp, VVarType::MODULETEMP,
                               "_Vpast_" + cvtToStr(m_mod_Past_अंक++) + "_t",
                               m_modp->findUInt64DType()};
            m_modp->addStmtsp(varpr);
            Ast_गाँठ_व्यंजक* स्थिर timep = नया AstCExpr{flp, "vlSymsp->_vm_contextp__->time()", 64};
            alwaysp->addStmtsp(
                नया AstAssign{flp, नया AstVarRef{flp, varpr, VAccess::WRITE}, timep});
        }
        वापस varpr;
    }

    व्योम visitAssertionIterate(Ast_गाँठ_Cover_Or_Assert* nodep, Ast_गाँठ* failsp) {
        अगर (m_beginp && nodep->name() == "") nodep->name(m_beginp->name());

        { AssertDeFutureVisitor{nodep->propp(), m_modp, m_mod_Past_अंक++}; }

        iterate_And_अग्र_Null(nodep->sentreep());
        अगर (AstAssert* स्थिर assertp = VN_CAST(nodep, Assert)) {
            iterate_And_अग्र_Null(assertp->failsp());
        } अन्यथा अगर (AstAssertIntrinsic* स्थिर assertp = VN_CAST(nodep, AssertIntrinsic)) {
            iterate_And_अग्र_Null(assertp->failsp());
        } अन्यथा अगर (AstCover* स्थिर coverp = VN_CAST(nodep, Cover)) {
            iterate_And_अग्र_Null(coverp->coverincsp());
        } अन्यथा अगर (!VN_IS(nodep, Restrict)) {
            nodep->v3fatalSrc("Unhandled assert type");
        }
        iterate_And_अग्र_Null(nodep->passsp());
        Ast_Sen_वृक्ष* sentreep = nodep->sentreep();
        अगर (nodep->immediate()) {
            UASSERT_OBJ(!sentreep, nodep, "Immediate assertions don't have sensitivity");
        } अन्यथा {
            UASSERT_OBJ(sentreep, nodep, "Concurrent assertions must have sensitivity");
            // Explicit inline clock differs from the enclosing always: hoist
            // and warn. To support this need queue of asserts to activate.
            अगर (m_procedurep && !nodep->senFromAlways()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Procedural concurrent assertion with"
                              " clocking event inside always (IEEE 1800-2023 16.14.6)");
                static_cast<Ast_गाँठ*>(m_procedurep)->add_अग्र(nodep->unlinkFrBack());
                वापस;  // Later iterate will pick up
            }
            sentreep->unlinkFrBack();
            अगर (m_procedurep) { VL_DO_DANGLING(धकेलो_Deletep(sentreep), sentreep); }
        }
        //
        स्थिर string& message = nodep->name();
        Ast_गाँठ* passsp = nodep->passsp();
        अगर (passsp) passsp->unlink_Fr_Back_With_अग्र();
        अगर (AstAssert* स्थिर assertp = VN_CAST(nodep, Assert)) {
            failsp = assertp->failsp();
        } अन्यथा अगर (AstAssertIntrinsic* स्थिर assertp = VN_CAST(nodep, AssertIntrinsic)) {
            failsp = assertp->failsp();
        }
        अगर (failsp) failsp->unlink_Fr_Back_With_अग्र();

        bool selfDestruct = false;
        bool passspGated = false;
        स्थिर AstCover* स्थिर coverp = VN_CAST(nodep, Cover);
        // A sequence event control is not an assertion directive; no assertion control
        स्थिर bool seqEvent = coverp && coverp->isSeqEvent();
        अगर (coverp) {
            ++m_statCover;
            अगर (seqEvent) {
                // Keep the event-fire action, with no coverage bucket
            } अन्यथा अगर (!v3Global.opt.coverageUser()) {
                selfDestruct = true;
            } अन्यथा {
                // V3Coverage assigned us a bucket to increment.
                AstCoverInc* स्थिर covincp = VN_AS(coverp->coverincsp(), CoverInc);
                UASSERT_OBJ(covincp, coverp, "Missing AstCoverInc under assertion");
                covincp->unlink_Fr_Back_With_अग्र();  // next() might have  AstAssign for trace
                अगर (message != "") covincp->declp()->comment(message);
                अगर (passsp) {
                    passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                               /*vacuous=*/false);
                    passspGated = true;
                    passsp = Ast_गाँठ::add_अग्र<Ast_गाँठ, Ast_गाँठ>(covincp, passsp);
                } अन्यथा {
                    passsp = covincp;
                }
            }
        } अन्यथा अगर (VN_IS(nodep, Assert) || VN_IS(nodep, AssertIntrinsic)) {
            अगर (nodep->immediate()) {
                ++m_statAsImm;
            } अन्यथा {
                ++m_statAsNotImm;
            }
            अगर (!passsp && !failsp)
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

        Ast_गाँठ* propExprp;
        Ast_गाँठ_व्यंजक* disablep = nullptr;
        Ast_गाँठ_व्यंजक* matchCountp = nullptr;
        अगर (AstPropSpec* स्थिर specp = VN_CAST(nodep->propp(), PropSpec)) {
            propExprp = specp->propp()->unlinkFrBack();
            अगर (specp->disablep()) disablep = specp->disablep()->unlinkFrBack();
            matchCountp = specp->matchCountp();
        } अन्यथा {
            propExprp = nodep->propp()->unlinkFrBack();
        }
        File_पंक्ति* स्थिर flp = nodep->fileline();
        bool passspAlreadyGated = false;
        अगर (passsp && VN_IS(passsp, If)) passspAlreadyGated = VN_AS(passsp, If)->user1();
        अगर (passsp && !passspGated && !passspAlreadyGated && !VN_IS(propExprp, PExpr)
            && !seqEvent) {
            passsp = newIfAssertPassOn(passsp, nodep->directive(), nodep->userType(),
                                       /*vacuous=*/false);
        }
        अगर (failsp && !VN_IS(propExprp, PExpr)) {
            failsp = newIfAssertFailOn(failsp, nodep->directive(), nodep->userType());
        }
        अगर (coverp && matchCountp && passsp) {
            // Convert the match count into a loop that decrements a temporary variable until it
            // reaches zero.
            matchCountp->unlinkFrBack();
            AstVar* स्थिर remainingp = नया AstVar{
                flp, VVarType::MODULETEMP, m_match_गिनती_Names.लो(""), matchCountp->dtypep()};
            remainingp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remainingp);
            Ast_गाँठ* स्थिर replaysp
                = नया AstAssign{flp, नया AstVarRef{flp, remainingp, VAccess::WRITE}, matchCountp};
            AstLoop* स्थिर loopp = नया AstLoop{flp};
            loopp->addStmtsp(
                नया AstLoopTest{flp, loopp, नया AstVarRef{flp, remainingp, VAccess::READ}});
            loopp->addStmtsp(passsp);
            loopp->addStmtsp(
                नया AstAssign{flp, नया AstVarRef{flp, remainingp, VAccess::WRITE},
                              नया AstSub{flp, नया AstVarRef{flp, remainingp, VAccess::READ},
                                         नया AstConst{flp, AstConst::Widthed_मूल्य{},
                                                      remainingp->dtypep()->width(), 1}}});
            replaysp->add_अग्र(loopp);
            passsp = replaysp;
        }
        Ast_गाँठ* bodysp = assertBody(nodep, propExprp, passsp, failsp);
        अगर (disablep) bodysp = नया AstIf{flp, नया AstLogNot{flp, disablep}, bodysp};
        // Add assertOn check last, for better combining
        अगर (!seqEvent) bodysp = newIfAssertOn(bodysp, nodep->directive(), nodep->userType());
        अगर (sentreep) bodysp = नया AstAlways{flp, VAlwaysKwd::ALWAYS, sentreep, bodysp};

        अगर (passsp && !passsp->backp()) VL_DO_DANGLING(धकेलो_Deletep(passsp), passsp);
        अगर (failsp && !failsp->backp()) VL_DO_DANGLING(धकेलो_Deletep(failsp), failsp);

        // Install it
        अगर (selfDestruct) {
            // Delete it after making the tree.  This way we can tell the user
            // if it wasn't constructed nicely or has other errors without needing --coverage.
            VL_DO_DANGLING(bodysp->delete_वृक्ष(), bodysp);
            nodep->unlinkFrBack();
        } अन्यथा {
            nodep->replaceWith(bodysp);
        }
        // Bye
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

    bool isEmptyStmt(Ast_गाँठ* nodep) {
        अगर (!nodep) वापस true;
        अगर (AstBegin* स्थिर beginp = VN_CAST(nodep, Begin)) {
            अगर (beginp->declsp()) वापस false;
            क्रम (Ast_गाँठ* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                अगर (!isEmptyStmt(stmtp)) वापस false;
            }
            वापस true;
        }
        वापस false;
    }

    // VISITORS
    व्योम visit(AstIf* nodep) override {
        अगर (!nodep->user1SetOnce()) {
            अगर (nodep->uniquePragma() || nodep->unique0Pragma()) {
                स्थिर Ast_गाँठ_If* ifp = nodep;
                Ast_गाँठ_व्यंजक* propp = nullptr;
                bool hasDefaultElse = false;
                करो {
                    // If this statement ends with 'else if', then nextIf will point to the
                    // nextIf statement.  Otherwise it will be null.
                    स्थिर Ast_गाँठ_If* स्थिर nextifp = dynamic_cast<Ast_गाँठ_If*>(ifp->elsesp());
                    iterate_And_अग्र_Null(ifp->condp());

                    // Recurse into the true case.
                    iterate_And_अग्र_Null(ifp->thensp());

                    // If the last else is not an else if, recurse into that too.
                    अगर (ifp->elsesp() && !nextifp) {  //
                        iterate_And_अग्र_Null(ifp->elsesp());
                    }

                    // Build a bitmask of the true predicates
                    Ast_गाँठ_व्यंजक* स्थिर predp = ifp->condp()->clone_वृक्ष_Pure(false);
                    अगर (propp) {
                        propp = नया AstConcat{nodep->fileline(), predp, propp};
                    } अन्यथा {
                        propp = predp;
                    }

                    // Record if this ends with an 'else' that does not have an if
                    अगर (ifp->elsesp() && !nextifp) hasDefaultElse = true;

                    ifp = nextifp;
                } जबतक (ifp);

                स्थिर bool allow_none = nodep->unique0Pragma();

                // Empty case means no property
                अगर (!propp) propp = नया AstConst{nodep->fileline(), AstConst::BitFalse{}};

                // Note: if this ends with an 'else', then we don't need to validate that one of
                // the predicates evaluates to true.
                Ast_गाँठ_व्यंजक* स्थिर ohot
                    = ((allow_none || hasDefaultElse)
                           ? static_cast<Ast_गाँठ_व्यंजक*>(नया AstOneHot0{nodep->fileline(), propp})
                           : static_cast<Ast_गाँठ_व्यंजक*>(नया AstOneHot{nodep->fileline(), propp}));
                स्थिर VAssertType assertType
                    = nodep->uniquePragma() ? VAssertType::UNIQUE : VAssertType::UNIQUE0;
                // Move, not clone, as the branches might declare referenced variables
                VNRelinker relinker;
                nodep->unlinkFrBack(&relinker);
                AstIf* स्थिर checkifp
                    = नया AstIf{nodep->fileline(), नया AstLogNot{nodep->fileline(), ohot},
                                newFireAssert(nodep, VAssertDirectiveType::VIOLATION_IF,
                                              assertType, "'unique if' statement violated"),
                                nodep};
                checkifp->isBoundsCheck(true);  // To avoid LATCH warning
                checkifp->branchPred(VBranchPred::BP_UNLIKELY);
                relinker.relink(checkifp);
                वापस;
            }
        }

        iterateChildren(nodep);

        अगर (nodep->user2()) {
            // Combine consecutive assertOn checks if possible
            अगर (AstIf* स्थिर backp = VN_CAST(nodep->backp(), If)) {
                अगर (backp->nextp() == nodep  //
                    && backp->user2()  //
                    && backp->condp()->same_वृक्ष(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    backp->addThensp(nodep->thensp()->unlink_Fr_Back_With_अग्र());
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    वापस;
                }
            }
            // Combine nested assertOn checks if possible
            अगर (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* स्थिर checkp = VN_CAST(nodep->thensp(), If);
                अगर (checkp  //
                    && checkp->user2()  //
                    && checkp->condp()->same_वृक्ष(nodep->condp())) {
                    ++m_statAssertOnCombined;
                    nodep->addThensp(checkp->thensp()->unlink_Fr_Back_With_अग्र());
                    VL_DO_DANGLING(धकेलो_Deletep(checkp->unlinkFrBack()), checkp);
                    वापस;
                }
            }
            वापस;
        }

        // Swap assertOn check with single statement 'if' statement to bubble up for combining
        // Note we can't just swap the conditions as they two Ifs have different flags,
        // so swapping the Ifs themeselves then swapping back the bodies.
        अगर (nodep->condp()->isPure()) {
            अगर (nodep->thensp() && !nodep->thensp()->nextp() && isEmptyStmt(nodep->elsesp())) {
                AstIf* स्थिर checkp = VN_CAST(nodep->thensp(), If);
                अगर (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addThensp(checkp->thensp()->unlink_Fr_Back_With_अग्र());
                    checkp->addThensp(nodep);
                    वापस;
                }
            }
            अगर (nodep->elsesp() && !nodep->elsesp()->nextp() && isEmptyStmt(nodep->thensp())) {
                AstIf* स्थिर checkp = VN_CAST(nodep->elsesp(), If);
                अगर (checkp && checkp->user2()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(checkp->unlinkFrBack());
                    nodep->addElsesp(checkp->thensp()->unlink_Fr_Back_With_अग्र());
                    checkp->addThensp(nodep);
                    वापस;
                }
            }
        }
    }

    //========== Case assertions
    व्योम visit(AstCase* nodep) override {
        // Introduce temporary variable for AstCase if needed - it is done here and not in V3Case
        // because this phase is before V3Scope and V3Case is not. Doing it before V3Scope ensures
        // that V3Scope will take care of a scope creation
        // We also need to do it before V3Begin, co that pragmas like `unique0` also work correctly
        अगर (!nodep->exprp()->isPure()) {
            ++m_statLiftedCaseExprs;
            File_पंक्ति* स्थिर fl = nodep->exprp()->fileline();
            AstVar* स्थिर varp = नया AstVar{fl, VVarType::BLOCKTEMP, m_case_अस्थायी_Names.लो(nodep),
                                            nodep->exprp()->dtypep()};
            Ast_गाँठ_व्यंजक* स्थिर origp = nodep->exprp()->unlinkFrBack();
            nodep->add_Here_This_As_अग्र(
                नया AstAssign{fl, नया AstVarRef{fl, varp, VAccess::WRITE}, origp});
            nodep->exprp(नया AstVarRef{fl, varp, VAccess::READ});
            अगर (m_ftaskp) {
                varp->funcLocal(true);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } अन्यथा {
                m_modp->prependStmtsp(varp);
            }
            VIsCached::clear_Cache_वृक्ष();
        }
        iterateChildren(nodep);
        अगर (!nodep->user1SetOnce()) {
            bool has_default = false;
            क्रम (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                अगर (itemp->isDefault()) has_default = true;
            }
            स्थिर Ast_गाँठ_DType* व्यंजक_Dtypep = nodep->exprp()->dtypep()->skipRefp();

            VAssertType assertType = VAssertType::INTERNAL;
            अगर (nodep->priorityPragma()) {
                assertType = VAssertType::PRIORITY;
            } अन्यथा अगर (nodep->uniquePragma()) {
                assertType = VAssertType::UNIQUE;
            } अन्यथा अगर (nodep->unique0Pragma()) {
                assertType = VAssertType::UNIQUE0;
            }

            string valFmt;
            अगर (व्यंजक_Dtypep->isIntegralOrPacked())
                valFmt = " for '" + cvtToStr(व्यंजक_Dtypep->widthMin()) + "'h%X'";
            अगर (nodep->fullPragma() || nodep->priorityPragma()) {
                // Need to add a default if there isn't one already
                ++m_statAsFull;
                अगर (!has_default) {
                    nodep->addItemsp(नया AstCaseItem{
                        nodep->fileline(), nullptr /*DEFAULT*/,
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      nodep->pragmaString() + ", but non-match found" + valFmt,
                                      valFmt.empty() ? nullptr
                                                     : nodep->exprp()->clone_वृक्ष_Pure(false))});
                }
            }
            अगर (nodep->parallelPragma() || nodep->uniquePragma() || nodep->unique0Pragma()) {
                // Need to check that one, and only one of the case items match at any moment
                // If there's a default, we allow none to match, else exactly one must match
                ++m_statAsFull;
                अगर (!has_default && !nodep->itemsp()) {
                    // Not parallel, but harmlessly so.
                } अन्यथा {
                    Ast_गाँठ_व्यंजक* propp = nullptr;
                    क्रम (AstCaseItem* itemp = nodep->itemsp(); itemp;
                         itemp = VN_AS(itemp->nextp(), CaseItem)) {
                        Ast_गाँठ_व्यंजक* itembitp = nullptr;
                        क्रम (Ast_गाँठ_व्यंजक* icondp = itemp->condsp(); icondp;
                             icondp = VN_AS(icondp->nextp(), गाँठ_व्यंजक)) {
                            Ast_गाँठ_व्यंजक* onep;
                            अगर (AstInsideRange* स्थिर rcondp = VN_CAST(icondp, InsideRange)) {
                                onep = rcondp->newAndFromInside(
                                    nodep->exprp()->clone_वृक्ष_Pure(true),
                                    rcondp->lhsp()->clone_वृक्ष_Pure(true),
                                    rcondp->rhsp()->clone_वृक्ष_Pure(true));
                            } अन्यथा अगर (nodep->casex() || nodep->casez() || nodep->caseInside()) {
                                onep = AstEqWild::newTyped(itemp->fileline(),
                                                           nodep->exprp()->clone_वृक्ष_Pure(false),
                                                           icondp->clone_वृक्ष_Pure(false));
                            } अन्यथा {
                                onep = AstEq::newTyped(icondp->fileline(),
                                                       nodep->exprp()->clone_वृक्ष_Pure(false),
                                                       icondp->clone_वृक्ष_Pure(false));
                            }
                            // OR together all conditions within the same case item
                            अगर (onep) {
                                अगर (itembitp) {
                                    itembitp = नया AstOr{icondp->fileline(), onep, itembitp};
                                } अन्यथा {
                                    itembitp = onep;
                                }
                            }
                        }
                        अगर (itembitp) {
                            अगर (propp) {
                                propp = नया AstConcat{itemp->fileline(), itembitp, propp};
                            } अन्यथा {
                                propp = itembitp;
                            }
                        }
                    }
                    // Empty case means no property
                    अगर (!propp) propp = नया AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    स्थिर bool allow_none = has_default || nodep->unique0Pragma();
                    // The following assertion looks as below.
                    // if (!$onehot(propp)) begin
                    //     if (propp == '0) begin if (!allow_none) $error("none match"); end
                    //     else $error("multiple match");
                    // end
                    Ast_गाँठ_व्यंजक* स्थिर ohot = नया AstOneHot{nodep->fileline(), propp};
                    AstConst* स्थिर zero = नया AstConst{
                        nodep->fileline(), AstConst::Widthed_मूल्य{}, propp->width(), 0};
                    AstIf* स्थिर ohotIfp
                        = नया AstIf{nodep->fileline(), नया AstLogNot{nodep->fileline(), ohot}};
                    AstIf* स्थिर zeroIfp = नया AstIf{
                        nodep->fileline(),
                        नया AstEq{nodep->fileline(), propp->clone_वृक्ष_Pure(false), zero}};
                    Ast_गाँठ_व्यंजक* स्थिर exprp = nodep->exprp();
                    स्थिर string pragmaStr = nodep->pragmaString();
                    अगर (!allow_none)
                        zeroIfp->addThensp(
                            newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                          pragmaStr + ", but none matched" + valFmt,
                                          valFmt.empty() ? nullptr : exprp->clone_वृक्ष_Pure(false)));
                    zeroIfp->addElsesp(
                        newFireAssert(nodep, VAssertDirectiveType::VIOLATION_CASE, assertType,
                                      pragmaStr + ", but multiple matches found" + valFmt,
                                      valFmt.empty() ? nullptr : exprp->clone_वृक्ष_Pure(false)));
                    ohotIfp->addThensp(zeroIfp);
                    ohotIfp->isBoundsCheck(true);  // To avoid LATCH warning
                    ohotIfp->branchPred(VBranchPred::BP_UNLIKELY);
                    nodep->addNotParallelp(ohotIfp);
                }
            }
        }
    }

    व्योम visit(AstFuture* nodep) override {
        nodep->v3error("Future sampled value function called outside property or sequence "
                       "expression (IEEE 16.9.4)");
        nodep->replaceWith(नया AstConst{nodep->fileline(), 0});
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

    //========== Past
    व्योम visit(AstPast* nodep) override {
        अगर (!nodep->propertyTiming()) {
            nodep->initp(nodep->exprp()->clone_वृक्ष_Pure(false));
            { Default_Sampled_मूल्य_Visitor{nodep->initp()}; }
        }
        Ast_गाँठ_व्यंजक* स्थिर initp = nodep->initp() ? nodep->initp()->unlinkFrBack() : nullptr;
        iterateChildren(nodep);
        uint32_t ticks = 1;
        अगर (nodep->ticksp()) {
            UASSERT_OBJ(VN_IS(nodep->ticksp(), Const), nodep,
                        "Expected constant ticks, checked in V3Width");
            ticks = VN_AS(nodep->ticksp(), Const)->toUInt();
        }
        UASSERT_OBJ(ticks >= 1, nodep, "0 tick should have been checked in V3Width");
        Ast_गाँठ_व्यंजक* स्थिर exprp = new_Sampled_व्यंजक(nodep->exprp()->unlinkFrBack());
        Ast_Sen_वृक्ष* स्थिर senTreep = nodep->sentreep()->unlinkFrBack();
        Ast_गाँठ_व्यंजक* inp = nullptr;
        // Deliberately emitted for every ftask $past; calls outside final take the normal stage
        अगर (VN_IS(m_procedurep, Final) || m_ftaskp) {
            // A sampling-tick finish reads one stage deeper (IEEE 1800-2023 16.9.3).
            File_पंक्ति* स्थिर flp = nodep->fileline();
            AstAlways* स्थिर alwaysp
                = getDelayedAlways(exprp, initp ? initp->clone_वृक्ष_Pure(false) : nullptr, senTreep);
            Ast_गाँठ_व्यंजक* स्थिर betweenTicksp = past_मूल्य_Ref(alwaysp, flp, ticks, initp);
            Ast_गाँठ_व्यंजक* स्थिर onTickp = past_मूल्य_Ref(alwaysp, flp, ticks + 1, initp);
            अगर (initp) VL_DO_DANGLING(धकेलो_Deletep(initp), initp);
            Ast_गाँठ_व्यंजक* स्थिर onTickCondp = नया AstLogAnd{
                flp, नया AstCExpr{flp, "vlSymsp->_vm_contextp__->executingFinal()", 1},
                नया AstEq{flp, नया AstVarRef{flp, getPastTickTimeVar(alwaysp), VAccess::READ},
                          नया AstCExpr{flp, "vlSymsp->_vm_contextp__->finishPendingTime()", 64}}};
            AstCond* स्थिर condp = नया AstCond{flp, onTickCondp, onTickp, betweenTicksp};
            condp->dtypeFrom(onTickp);
            inp = condp;
        } अन्यथा {
            inp = get_Past_मूल्य(exprp, initp, senTreep, ticks);
        }
        nodep->replaceWith(inp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

    //========== Move $sampled down to read-only variables
    व्योम visit(AstSampled* nodep) override {
        अगर (nodep->user1()) वापस;
        VL_RESTORER(m_inSampled);
        {
            m_inSampled = true;
            iterateChildren(nodep);
        }
        अगर (nodep->exprp()) {
            nodep->replaceWith(nodep->exprp()->unlinkFrBack());
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(Ast_गाँठ_Var_Ref* nodep) override {
        अगर (m_inSampled && !nodep->user2() && !nodep->varp()->is_अस्थायी()) {
            अगर (!nodep->access().isReadOnly()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Write to variable in sampled expression");
            } अन्यथा {
                VNRelinker relinkHandle;
                nodep->unlinkFrBack(&relinkHandle);
                AstSampled* स्थिर newp = new_Sampled_व्यंजक(nodep);
                relinkHandle.relink(newp);
                newp->user1(1);
                v3Global.setHasSampled();
            }
        }
    }
    // Don't sample sensitivities
    व्योम visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSampled);
        m_inSampled = false;
        iterateChildren(nodep);
    }
    व्योम visit(AstPExprClause* nodep) override {
        Ast_गाँठ* stmtsp = nullptr;
        अगर (nodep->pass() && m_passsp) {
            // Cover adds COVERINC by AstNode::addNext, thus need to clone next too.
            stmtsp = m_passsp->clone_वृक्ष(true);
            stmtsp = newIfAssertPassOn(stmtsp, m_assertp->directive(), m_assertp->userType(),
                                       nodep->vacuous());
        } अन्यथा अगर (!nodep->pass() && m_failsp) {
            stmtsp = m_failsp->clone_वृक्ष(true);
            stmtsp = newIfAssertFailOn(stmtsp, m_assertp->directive(), m_assertp->userType());
        }
        अगर (stmtsp) {
            stmtsp->foreach_And_अग्र([](Ast_गाँठ_Var_Ref* स्थिर refp) {
                // References inside action blocks shouldn't be implicitly sampled
                // m_passsp/m_failsp have been already visited once and refs explicitly sampled
                // are handled already
                refp->user2(1);
            });
            nodep->replaceWith(stmtsp);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstPExpr* nodep) override {
        अगर (m_inRestrict) {
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
        } अन्यथा {
            iterateChildren(nodep);
        }
    }

    //========== Statements
    व्योम visit(AstDisplay* nodep) override {
        iterateChildren(nodep);
        // Replace the special types with standard text
        अगर (nodep->displayType() == VDisplayType::DT_INFO) {
            replaceDisplay(nodep, "-Info");
        } अन्यथा अगर (nodep->displayType() == VDisplayType::DT_WARNING) {
            replaceDisplay(nodep, "%%Warning");
        } अन्यथा अगर (nodep->displayType() == VDisplayType::DT_ERROR) {
            replaceDisplay(nodep, "%%Error");
        } अन्यथा अगर (nodep->displayType() == VDisplayType::DT_FATAL) {
            replaceDisplay(nodep, "%%Fatal");
        } अन्यथा अगर (nodep->displayType() == VDisplayType::DT_MONITOR) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            File_पंक्ति* स्थिर fl = nodep->fileline();

            AstSenItem* monSenItemsp = nullptr;
            अगर (Ast_गाँठ* स्थिर monExprsp = nodep->fmtp()->exprsp()) {
                monExprsp->foreach_And_अग्र([&](AstVarRef* varrefp) {
                    AstSenItem* स्थिर senItemp
                        = नया AstSenItem{fl, VEdgeType::ET_CHANGED,
                                         // Clone so get VarRef or VarXRef as needed
                                         varrefp->clone_वृक्ष(false)};
                    monSenItemsp = Ast_गाँठ::add_अग्र_Null(monSenItemsp, senItemp);
                });
            }

            Ast_Sen_वृक्ष* स्थिर mon_Sen_वृक्ष = नया Ast_Sen_वृक्ष{fl, monSenItemsp};
            स्थिर स्वतः mon_अंक = ++m_monitor_अंक;
            // Where $monitor was we do "__VmonitorNum = N;"
            AstAssign* स्थिर newsetp = नया AstAssign{
                fl, new_Monitor_अंक_Var_Refp(nodep, VAccess::WRITE), नया AstConst{fl, mon_अंक}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__VmonitorOn && __VmonitorNum==N) $display(...);"
            Ast_गाँठ* स्थिर stmtsp = nodep;
            AstIf* स्थिर ifp = नया AstIf{
                fl,
                नया AstLogAnd{fl, नया AstLogNot{fl, newMonitorOffVarRefp(nodep, VAccess::READ)},
                              नया AstEq{fl, नया AstConst{fl, mon_अंक},
                                        new_Monitor_अंक_Var_Refp(nodep, VAccess::READ)}},
                stmtsp};
            ifp->isBoundsCheck(true);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_गाँठ* स्थिर newp = नया AstAlways{fl, VAlwaysKwd::ALWAYS, mon_Sen_वृक्ष, ifp};
            m_modp->addStmtsp(newp);
        } अन्यथा अगर (nodep->displayType() == VDisplayType::DT_STROBE) {
            nodep->displayType(VDisplayType::DT_DISPLAY);
            // Need one-shot
            File_पंक्ति* स्थिर fl = nodep->fileline();
            AstVar* स्थिर varp
                = नया AstVar{fl, VVarType::MODULETEMP, "__Vstrobe" + cvtToStr(m_mod_Strobe_अंक++),
                             nodep->findBitDType()};
            m_modp->addStmtsp(varp);
            // Where $strobe was we do "__Vstrobe = '1;"
            AstAssign* स्थिर newsetp = नया AstAssign{fl, नया AstVarRef{fl, varp, VAccess::WRITE},
                                                     नया AstConst{fl, AstConst::BitTrue{}}};
            nodep->replaceWith(newsetp);
            // Add "always_comb if (__Vstrobe) begin $display(...); __Vstrobe = '0; end"
            Ast_गाँठ* स्थिर stmtsp = nodep;
            AstIf* स्थिर ifp = नया AstIf{fl, नया AstVarRef{fl, varp, VAccess::READ}, stmtsp};
            ifp->isBoundsCheck(true);  // To avoid LATCH warning
            ifp->branchPred(VBranchPred::BP_UNLIKELY);
            Ast_गाँठ* स्थिर newp = नया AstAlwaysPostponed{fl, ifp};
            stmtsp->add_अग्र(नया AstAssign{fl, नया AstVarRef{fl, varp, VAccess::WRITE},
                                          नया AstConst{fl, AstConst::BitFalse{}}});
            m_modp->addStmtsp(newp);
        }
    }
    व्योम visit(AstMonitorOff* nodep) override {
        AstAssign* स्थिर newp
            = नया AstAssign{nodep->fileline(), newMonitorOffVarRefp(nodep, VAccess::WRITE),
                            नया AstConst{nodep->fileline(), AstConst::BitTrue{}, nodep->off()}};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstAssert* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    व्योम visit(AstAssertCtl* nodep) override {
        iterateChildren(nodep);

        bool assertTypeConst = true;
        अगर (!nodep->assertTypesp()) {
            nodep->assertTypes(VAssertType{ALL_ASSERT_TYPES});
        } अन्यथा अगर (स्थिर AstConst* स्थिर assertTypesp = VN_CAST(nodep->assertTypesp(), Const)) {
            nodep->assertTypes(VAssertType{assertTypesp->toSInt()});
        } अन्यथा {
            assertTypeConst = false;
        }

        bool controlTypeConst = false;
        अगर (स्थिर AstConst* स्थिर constp = VN_CAST(nodep->controlTypep(), Const)) {
            nodep->ctlType(constp->toSInt());
            controlTypeConst = true;
        }
        अगर (controlTypeConst
            && (nodep->ctlType() < VAssertCtlType::LOCK
                || nodep->ctlType() > VAssertCtlType::VACUOUS_OFF)) {
            nodep->unlinkFrBack();
            nodep->v3error("Bad $assertcontrol control_type '"
                           << cvtToStr(static_cast<पूर्णांक>(nodep->ctlType()))
                           << "' (IEEE 1800-2023 Table 20-5)");
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        }
        अगर (assertTypeConst && nodep->assertTypes() != ALL_ASSERT_TYPES
            && nodep->assertTypes().containsAny(VAssertType::UNIQUE | VAssertType::UNIQUE0
                                                | VAssertType::PRIORITY)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: assert control assertion_type");
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
            वापस;
        }

        bool directiveTypeConst = true;
        अगर (!nodep->directiveTypesp()) {
            nodep->directiveTypes(VAssertDirectiveType::ASSERT | VAssertDirectiveType::ASSUME
                                  | VAssertDirectiveType::COVER);
        } अन्यथा अगर (स्थिर AstConst* स्थिर directiveTypesp
                   = VN_CAST(nodep->directiveTypesp(), Const)) {
            nodep->directiveTypes(VAssertDirectiveType{directiveTypesp->toSInt()});
        } अन्यथा {
            directiveTypeConst = false;
        }
        अगर (!directiveTypeConst) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: non-const assert directive type expression");
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
            वापस;
        }

        File_पंक्ति* स्थिर fl = nodep->fileline();
        UINFO(9, "Generating assertctl for a module: " << m_modp);
        AstCStmt* स्थिर newp = नया AstCStmt{fl};
        newp->add("vlSymsp->_vm_contextp__->assertCtl(");
        newp->add(nodep->controlTypep()->unlinkFrBack());
        newp->add(", ");
        अगर (nodep->assertTypesp()) {
            newp->add(nodep->assertTypesp()->unlinkFrBack());
        } अन्यथा {
            newp->add(std::to_string(ALL_ASSERT_TYPES));
        }
        newp->add(", " + std::to_string(nodep->directiveTypes()) + ");\n");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstAssertIntrinsic* nodep) override {  //
        visitAssertionIterate(nodep, nodep->failsp());
    }
    व्योम visit(AstCover* nodep) override {  //
        visitAssertionIterate(nodep, nullptr);
    }
    व्योम visit(AstRestrict* nodep) override {
        VL_RESTORER(m_inRestrict);
        m_inRestrict = true;
        iterateChildren(nodep);
        // IEEE says simulator ignores these
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }

    व्योम visit(Ast_गाँठ_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Past_अंक);
        VL_RESTORER(m_mod_Strobe_अंक);
        VL_RESTORER(m_finalp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DelayedAlwaysp);
        VL_RESTORER_CLEAR(m_modExpr2Sen2DefaultDelayedAlwaysp);
        VL_RESTORER_CLEAR(m_delayedAlways2TickTimep);
        m_modp = nodep;
        m_mod_Past_अंक = 0;
        m_mod_Strobe_अंक = 0;
        m_finalp = nullptr;
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(AstGenBlock* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(AstBegin* nodep) override {
        // This code is needed rather than a visitor in V3Begin,
        // because V3Assert is called before V3Begin
        VL_RESTORER(m_beginp);
        m_beginp = nodep;
        iterateChildren(nodep);
        // If the body is a single assertOn check, bubble it up for combining
        अगर (!nodep->declsp()) {
            अगर (AstIf* स्थिर ifp = VN_CAST(nodep->stmtsp(), If)) {
                अगर (ifp->user2() && !ifp->nextp()) {
                    ++m_statAssertOnHoisted;
                    nodep->replaceWith(ifp->unlinkFrBack());
                    nodep->addStmtsp(ifp->thensp()->unlink_Fr_Back_With_अग्र());
                    ifp->addThensp(nodep);
                }
            }
        }
    }

    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
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
        VIsCached::clear_Cache_वृक्ष();
    }
};

//######################################################################
// Top Assert class

व्योम V3Assert::assertAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("assert", 0, dump_वृक्ष_Either_Level() >= 3);
}
