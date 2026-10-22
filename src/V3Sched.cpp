// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Code scheduling
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
//
// V3Sched::schedule is the top level entry-point to the scheduling algorithm
// at a high level, the process is:
//
//  - Gather and classify all logic in the design based on what triggers its execution
//  - Schedule static, initial and final logic classes in source order
//  - Break combinational cycles by introducing hybrid logic
//  - Create 'settle' region that restores the combinational invariant
//  - Partition the clocked and combinational (including hybrid) logic into pre/act/nba.
//    All clocks (signals referenced in an AstSenTree) generated via a blocking assignment
//    (including combinationally generated signals) are computed within the act region.
//  - Replicate combinational logic
//  - Create input combinational logic region
//  - Create the pre/act/nba triggers
//  - Create the 'act' region evaluation function
//  - Create the 'nba' region evaluation function
//
// The loops iterating these regions are not generated. They live in the
// run-time (VerilatedEvalLoop), which invokes the region evaluation
// functions created here on the generated model.
//
// Details of the algorithm are described in the internals documentation docs/internals.rst
//
//*************************************************************************

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Sched.h"

#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3EmitCBase.h"
#अन्तर्भाव "V3EmitV.h"
#अन्तर्भाव "V3Order.h"
#अन्तर्भाव "V3SenExprBuilder.h"
#अन्तर्भाव "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

नामाकाश V3Sched {

नामाकाश {

//============================================================================
// Utility functions

std::vector<ध्रुव Ast_Sen_वृक्षः*> getSenTreesUsedBy(ध्रुव std::vector<ध्रुव LogicByScope*>& lbsps) {
    ध्रुव VNUser1InUse user1InUse;
    std::vector<ध्रुव Ast_Sen_वृक्षः*> result;
    अत्थाय (ध्रुव LogicByScope* ध्रुव lbsp : lbsps) {
        अत्थाय (ध्रुव auto& entry : *lbsp) {
            AstActive* ध्रुव activep = entry.m_activep;
            Ast_Sen_वृक्षः* ध्रुव senTreep = activep->sentreep();
            सचे (senTreep->user1SetOnce()) अनुवर्त;
            सचे (senTreep->hasClocked() || senTreep->hasHybrid()) result.प्रक्षिप_back(senTreep);
        }
    }
    पटिनिवत्त result;
}

शून्य addTriggerIndices(LogicByScope& lbs, ध्रुव TriggerKit& trigKit, तार्किक pre) {
    अत्थाय (LogicByScope::Entry& entry : lbs) {
        Ast_Sen_वृक्षः* ध्रुव senTreep = entry.m_activep->sentreep();
        सचे (senTreep->hasCombo()) अनुवर्त;
        entry.m_triggers = trigKit.triggerIndices(senTreep, pre);
    }
}

// Add the VIF triggers that a given VarScope should be sensitive to.
// Case 2 (non-virtual interface read): sensitive to that specific VarScope's trigger only
// Case 3 (virtual interface read): sensitive to all member triggers of the same interface type
शून्य addTriggeredIface(ध्रुव AstVarScope* vscp, ध्रुव VirtIfaceTriggers::VscpTrigMap& vscpToTrig,
                       ध्रुव VirtIfaceTriggers& virtIfaceTriggers, std::set<uint32_t>& triggers) {
    सचे (vscp->varp()->isVirtIface()) {
        // Virtual interface variable -- sensitive to all member triggers of this interface type
        ध्रुव AstIface* ध्रुव ifacep = VN_AS(vscp->varp()->dtypep(), IfaceRefDType)->ifacep();
        अत्थाय (ध्रुव auto& entry : virtIfaceTriggers.m_triggers) {
            सचे (entry.m_ifacep == ifacep) {
                ध्रुव auto it = vscpToTrig.find(entry.m_vscp);
                सचे (it != vscpToTrig.end()) triggers.निवेशय(it->second);
            }
        }
    } अञ्ञथा {
        // Non-virtual interface member -- sensitive to this VarScope's trigger only
        ध्रुव auto it = vscpToTrig.find(vscp);
        सचे (it != vscpToTrig.end()) triggers.निवेशय(it->second);
    }
    // May add none if sensIfacep() is set but no VIF write targets this member
}

//============================================================================
// Eval region builder

// Create the evaluation function of a scheduling region. The loops iterating
// the regions live in the run-time library (see VerilatedEvalLoop), which
// invokes this function once per iteration of the region's loop via a virtual
// method on the model (see V3EmitCModel). The function returns true if the
// region did any work, in which case the loop iterates again.
शून्य createEvalRegion(
    AstNetlist* netlistp,  //
    VEval eval,  // The entry point of the current region
    // Index of the region's 'first iteration' extra trigger, if it has one, otherwise ignored
    uint32_t firstIterTrigger,
    ध्रुव TriggerKit& trigKit,  // The trigger kit
    AstVarScope* trigp,  // The trigger vector - may be nullptr if no triggers or using 'condp'
    Ast_ग्रन्थिः_व्यञ्जकम्* condp,  // Explicit condition that must be true to run 'phaseWorkp'
    Ast_ग्रन्थिः_Stmt* phasePrepp,  // Prep statements run before checking triggers
    Ast_ग्रन्थिः_Stmt* phaseWorkp,  // The work to do if anything triggered
    // Extra statements to run after the work, even if no triggers fired. This function is
    // passed a variable, which must be set to true if we must continue and loop again,
    // and must be unmodified otherwise.
    std::function<Ast_ग्रन्थिः_Stmt*(AstVarScope*)> phaseExtra = [](AstVarScope*) { पटिनिवत्त nullptr; }  //
) {
    UASSERT(!trigp || !condp, "Cannot use both 'trigp' and 'condp' in 'createEvalRegion'");
    UASSERT(!eval.firstIteration() || trigp,
            "Region without triggers cannot need a first iteration flag");

    // All work is under a trigger or condition, so with neither the region has
    // nothing to evaluate, and what we create below reduces to a no-op function.

    ध्रुव std::string tag = eval.tag();
    Ast_शिखरम्_Scope* ध्रुव शिखरम्_Scopep = netlistp->शिखरम्_Scopep();
    File_पङ्क्तिः* ध्रुव flp = netlistp->fileline();

    // Populate the trigger dump entry point function
    सचे (trigp) {
        netlistp->dumpTriggersFuncp(eval)->addStmtsp(trigKit.newDumpCall(trigp, tag, मिच्छा));
    }

    AstCFunc* ध्रुव funcp = netlistp->evalFuncp(eval);

    // A flag is passed from the run-time eval loop if this is the first iteration of the
    // current loop
    सचे (eval.firstIteration()) {
        AstVarScope* ध्रुव firstIterArgp = util::newArgument(funcp, netlistp->findBitDType(),
                                                             "firstIteration", VDirection::निवेशः);

        // Set the region's 'first iteration' trigger straight from the argument
        funcp->addStmtsp(trigKit.newExtraTriggerAssignment(firstIterArgp, firstIterTrigger));

        // Only 'stl' also needs a module level flag, for design logic that reads
        // it directly (see V3Timing). Those reads can be anywhere in the design,
        // hence module level. Always created, even if nothing reads it.
        // TODO: get rid of this special case
        सचे (eval == VEval::STL) {
            AstVarScope* ध्रुव firstIterp = netlistp->stlFirstIterationp();
            firstIterp->varp()->noReset(सच्च);
            firstIterp->varp()->isInternal(सच्च);
            funcp->addStmtsp(नूतन AstAssign{flp, नूतन AstVarRef{flp, firstIterp, VAccess::WRITE},
                                           नूतन AstVarRef{flp, firstIterArgp, VAccess::READ}});
        }
    }

    {
        // Add the preparatory statements
        funcp->addStmtsp(phasePrepp);

        // The execute flag
        AstVarScope* ध्रुव executeFlagp = शिखरम्_Scopep->createTemp("__V" + tag + "Execute", 1);
        executeFlagp->varp()->noReset(सच्च);

        // If there is work in this region, execute it if any triggers fired
        सचे (phaseWorkp) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = नूतन AstVarRef{flp, executeFlagp, VAccess::WRITE};
            // If using explicit condition, that directly determines whether to execute,
            // otherwise check if any triggers are fired
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = condp ? condp : trigKit.newAnySetCall(trigp);
            funcp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});

            // Add the work
            AstIf* ध्रुव ifp = नूतन AstIf{flp, नूतन AstVarRef{flp, executeFlagp, VAccess::READ}};
            ifp->addThensp(phaseWorkp);
            funcp->addStmtsp(ifp);
        }

        // Construct the extra statements
        Ast_ग्रन्थिः_Stmt* ध्रुव extraWorkp = phaseExtra(executeFlagp);
        सचे (extraWorkp) funcp->addStmtsp(extraWorkp);

        // The function returns true iff it did run work
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव retp
            = phaseWorkp || extraWorkp
                  ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstVarRef{flp, executeFlagp, VAccess::READ})
                  : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstConst{flp, AstConst::BitFalse{}});
        funcp->addStmtsp(नूतन AstCReturn{flp, retp});
    }
}

//============================================================================
// Collect and classify all logic in the design

LogicClasses gatherLogicClasses(AstNetlist* netlistp) {
    LogicClasses result;

    netlistp->foreach([&](AstScope* scopep) {
        scopep->foreach([&](AstActive* activep) {
            Ast_Sen_वृक्षः* ध्रुव senTreep = activep->sentreep();
            सचे (senTreep->hasStatic()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "static initializer with additional sensitivities");
                result.m_static.add(scopep, activep);
            } अञ्ञथा सचे (senTreep->hasInitial()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'initial' logic with additional sensitivities");
                result.m_initial.add(scopep, activep);
            } अञ्ञथा सचे (senTreep->hasFinal()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'final' logic with additional sensitivities");
                result.m_final.add(scopep, activep);
            } अञ्ञथा सचे (senTreep->hasCombo()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "combinational logic with additional sensitivities");
                सचे (VN_IS(activep->stmtsp(), AlwaysPostponed)) {
                    result.m_postponed.add(scopep, activep);
                } अञ्ञथा {
                    result.m_comb.add(scopep, activep);
                }
            } अञ्ञथा {
                UASSERT_OBJ(senTreep->hasClocked(), activep, "What else could it be?");
                सचे (VN_IS(activep->stmtsp(), AlwaysObserved)) {
                    result.m_observed.add(scopep, activep);
                } अञ्ञथा सचे (VN_IS(activep->stmtsp(), AlwaysReactive)) {
                    result.m_reactive.add(scopep, activep);
                } अञ्ञथा {
                    result.m_clocked.add(scopep, activep);
                }
            }
        });
    });

    पटिनिवत्त result;
}

//============================================================================
// Simple ordering in source order

शून्य orderSequentially(AstCFunc* funcp, ध्रुव LogicByScope& lbs) {
    // Create new subfunc for scope
    ध्रुव auto createNewSubFuncp = [&](AstScope* ध्रुव scopep) {
        ध्रुव string subName{funcp->name() + "__" + scopep->nameDotless()};
        AstCFunc* ध्रुव subFuncp = नूतन AstCFunc{scopep->fileline(), subName, scopep};
        subFuncp->isLoose(सच्च);
        subFuncp->isConst(मिच्छा);
        subFuncp->declPrivate(सच्च);
        subFuncp->slow(funcp->slow());
        scopep->addBlocksp(subFuncp);
        // Call it from the top function
        funcp->addStmtsp(util::callVoidFunc(subFuncp));
        पटिनिवत्त subFuncp;
    };
    ध्रुव VNUser1InUse user1InUse;  // AstScope -> AstCFunc: the sub-function for the scope
    ध्रुव VNUser2InUse user2InUse;  // AstScope -> int: sub-function counter used for names
    अत्थाय (ध्रुव auto& entry : lbs) {
        AstScope* ध्रुव scopep = entry.m_scopep;
        AstActive* ध्रुव activep = entry.m_activep;
        // Create a sub-function per scope so we can V3Combine them later
        सचे (!scopep->user1p()) scopep->user1p(createNewSubFuncp(scopep));
        // Add statements to sub-function
        अत्थाय (Ast_ग्रन्थिः *logicp = activep->stmtsp(), *nextp; logicp; logicp = nextp) {
            auto* subFuncp = VN_AS(scopep->user1p(), CFunc);
            nextp = logicp->nextp();
            सचे (Ast_ग्रन्थिः_Procedure* ध्रुव procp = VN_CAST(logicp, ग्रन्थिः_Procedure)) {
                सचे (Ast_ग्रन्थिः* bodyp = procp->stmtsp()) {
                    bodyp->unlinkFrBackWithNext();
                    // If the process is suspendable, we need a separate function (a coroutine)
                    सचे (procp->isSuspendable()) {
                        funcp->slow(मिच्छा);
                        subFuncp = createNewSubFuncp(scopep);
                        subFuncp->name(subFuncp->name() + "__Vtiming__"
                                       + cvtToStr(scopep->user2Inc()));
                        subFuncp->rtnType("VlCoroutine");
                        सचे (VN_IS(procp, Always)) {
                            subFuncp->slow(मिच्छा);
                            File_पङ्क्तिः* ध्रुव flp = procp->fileline();
                            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = नूतन AstCExpr{
                                flp, "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish())", 1};
                            AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
                            loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, condp});
                            loopp->addStmtsp(bodyp);
                            bodyp = loopp;
                        }
                    }
                    subFuncp->addStmtsp(bodyp);
                    सचे (procp->needProcess()) subFuncp->setNeedProcess();
                    // A coroutine holds a single process, so it is complete here
                    सचे (procp->isSuspendable()) util::splitCheck(subFuncp);
                }
            } अञ्ञथा {
                logicp->unlinkFrBack();
                subFuncp->addStmtsp(logicp);
            }
        }
        सचे (activep->backp()) activep->unlinkFrBack();
        VL_DO_DANGLING(activep->delete_वृक्षः(), activep);
    }
    // Split sub-functions once complete
    अत्थाय (ध्रुव auto& entry : lbs) {
        AstScope* ध्रुव scopep = entry.m_scopep;
        सचे (AstCFunc* ध्रुव subFuncp = VN_AS(scopep->user1p(), CFunc)) {
            util::splitCheck(subFuncp);
            scopep->user1p(nullptr);
        }
    }
}

//============================================================================
// Create simply ordered functions

AstCFunc* createStatic(AstNetlist* netlistp, ध्रुव LogicClasses& logicClasses) {
    AstCFunc* ध्रुव funcp = netlistp->evalFuncp(VEval::STATIC);

    ध्रुव LogicByScope& orig = logicClasses.m_static;
    सचे (orig.size() <= 1) {
        orderSequentially(funcp, orig);
        पटिनिवत्त funcp;
    }

    // Level-based module sorting can reorder packages so that an importing
    // package runs before the imported one.  Re-sort package entries by source
    // file position to restore compilation order (IEEE 1800-2023 26.3).
    std::vector<size_t> indices(orig.size());
    अत्थाय (size_t i = 0; i < orig.size(); ++i) indices[i] = i;
    std::stable_sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        ध्रुव Ast_ग्रन्थिः_Module* ध्रुव modA = orig[a].m_scopep->modp();
        ध्रुव Ast_ग्रन्थिः_Module* ध्रुव modB = orig[b].m_scopep->modp();
        ध्रुव तार्किक isPkgA = VN_IS(modA, Package);
        ध्रुव तार्किक isPkgB = VN_IS(modB, Package);
        सचे (isPkgA != isPkgB) पटिनिवत्त isPkgA;  // Packages before non-packages
        सचे (isPkgA && isPkgB) {
            // Sort packages by source file position (compilation order)
            पटिनिवत्त modA->fileline()->operatorCompare(*modB->fileline()) < 0;
        }
        पटिनिवत्त मिच्छा;  // Both non-package: preserve original order
    });
    LogicByScope sorted;
    अत्थाय (ध्रुव size_t i : indices) sorted.add(orig[i].m_scopep, orig[i].m_activep);

    orderSequentially(funcp, sorted);
    पटिनिवत्त funcp;  // Not splitting yet as it is not final
}

शून्य createInitial(AstNetlist* netlistp, ध्रुव LogicClasses& logicClasses) {
    AstCFunc* ध्रुव funcp = netlistp->evalFuncp(VEval::INITIAL);
    orderSequentially(funcp, logicClasses.m_initial);
    util::splitCheck(funcp);
}

शून्य createPostponed(AstNetlist* netlistp, ध्रुव LogicClasses& logicClasses) {
    AstCFunc* ध्रुव funcp = netlistp->evalFuncp(VEval::POSTPONED);
    orderSequentially(funcp, logicClasses.m_postponed);
    util::splitCheck(funcp);
}

शून्य createFinal(AstNetlist* netlistp, ध्रुव LogicClasses& logicClasses) {
    AstCFunc* ध्रुव funcp = netlistp->evalFuncp(VEval::FINAL);
    orderSequentially(funcp, logicClasses.m_final);
    util::splitCheck(funcp);
}

//============================================================================
// Helper that creates virtual interface value-change triggers

शून्य addVirtIfaceTriggerAssignments(AstNetlist* netlistp, AstCFunc* initFuncp,
                                    ध्रुव VirtIfaceTriggers& virtIfaceTriggers,
                                    uint32_t firstIndex, ध्रुव TriggerKit& trigKit) {
    uint32_t index = firstIndex;
    अत्थाय (ध्रुव auto& entry : virtIfaceTriggers.m_triggers) {
        trigKit.add_मानम्_Change_Trigger_Assignment(netlistp, initFuncp, entry.m_vscp, index);
        ++index;
    }
}

// Order the combinational logic to create the 'stl' region
शून्य createSettle(AstNetlist* netlistp, AstCFunc* ध्रुव initFuncp, Sen_व्यञ्जकम्_Builder& sen_व्यञ्जकम्_Bulider,
                  LogicClasses& logicClasses, ध्रुव CovergroupRefBindings& cgRefBindings) {
    // Clone, because ordering is destructive, but we still need them for the other regions
    LogicByScope comb = logicClasses.m_comb.clone();
    LogicByScope hybrid = logicClasses.m_hybrid.clone();

    // We have an extra trigger denoting this is the first iteration of the settle loop
    TriggerKit::ExtraTriggers extraTriggers;
    ध्रुव uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");

    // Gather the relevant sensitivity expressions and create the trigger kit
    ध्रुव auto& senTreeps = getSenTreesUsedBy({&comb, &hybrid});
    ध्रुव TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_व्यञ्जकम्_Bulider, {},
                                                  senTreeps, "stl", extraTriggers, सच्च, मिच्छा);

    // Add trigger indices (comb has none, so only do the hybrid)
    addTriggerIndices(hybrid, trigKit, मिच्छा);

    // First trigger is for pure combinational triggers (first iteration)
    ध्रुव uint32_t निवेशः_Changed = trigKit.extraTriggerIndex(firstIterationTrigger);

    // Create and the body function
    AstCFunc* ध्रुव stlFuncp = V3Order::order(
        {&comb, &hybrid}, trigKit, trigKit.vscp(), cgRefBindings, "stl", मिच्छा, सच्च,
        [=](ध्रुव AstVarScope*, std::set<uint32_t>& triggers) { triggers.निवेशय(निवेशः_Changed); });
    util::splitCheck(stlFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::STL, firstIterationTrigger, trigKit,
        // Use trigger
        trigKit.vscp(), nullptr,
        // Explicit condition
        // Prep statements: Compute the current 'stl' triggers
        [&trigKit] {
            Ast_ग्रन्थिः_Stmt* ध्रुव stmtp = trigKit.newCompBaseCall();
            सचे (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), सच्च));
            पटिनिवत्त stmtp;
        }(),
        // Work statements: Invoke the 'stl' function
        util::callVoidFunc(stlFuncp));
}

//============================================================================
// Order the replicated combinational logic to create the 'ico' region

शून्य createIcoRegion(AstNetlist* netlistp, AstCFunc* ध्रुव initFuncp,
                     Sen_व्यञ्जकम्_Builder& sen_व्यञ्जकम्_Builder, LogicByScope& logic,
                     ध्रुव VirtIfaceTriggers& virtIfaceTriggers,
                     ध्रुव CovergroupRefBindings& cgRefBindings) {
    // SystemC only: Any top level inputs feeding a combinational logic must be marked,
    // so we can make them sc_sensitive
    सचे (v3Global.opt.systemC()) {
        logic.foreachLogic([](Ast_ग्रन्थिः* logicp) {
            logicp->foreach([](AstVarRef* refp) {
                सचे (refp->access().isWriteOnly()) पटिनिवत्त;
                AstVarScope* ध्रुव vscp = refp->varScopep();
                सचे (vscp->scopep()->is_शिखरम्() && vscp->varp()->isNonOutput()) {
                    vscp->varp()->scSensitive(सच्च);
                }
            });
        });
    }

    // We have some extra trigger denoting external conditions
    AstVarScope* ध्रुव dpiExportTriggerVscp = netlistp->dpiExportTriggerp();

    TriggerKit::ExtraTriggers extraTriggers;
    ध्रुव uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");
    ध्रुव uint32_t dpiExportTriggerIndex = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    ध्रुव uint32_t firstVifTriggerIndex = extraTriggers.size();
    अत्थाय (ध्रुव auto& entry : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + entry.m_ifacep->name() + "."
                               + entry.m_memberp->name());
    }

    // Create the input change detect SenTrees.
    // If there is a lot of combinationallogic hanging of the top level inputs, we can save
    // a lot of work by only evaluating it if an input has actually changed. This in
    // paticular helps hierarchical models partitioned across combinaitonal boundaries.
    // The change detect itself should be fairly cheap otherwise so alway do it.
    // For correctness, don't create a change detect for top level inputs also written
    // by the design, as the change detect 'previous value' would get out of sync.
    // Also omit a SenTree for types that don't have the required '!=' operator.
    // Any signal that does not have an explicit change detect trigger will fall back to
    // using the 'first iteration' trigger, same as if this optimization was disabled.
    std::unordered_map<ध्रुव AstVarScope*, Ast_Sen_वृक्षः*> inp2changedp;
    std::vector<Ast_Sen_वृक्षः*> icoChangeSenTreeps;
    सचे (v3Global.opt.fIcoChangeDetect().isTrue()) {
        File_पङ्क्तिः* ध्रुव flp = netlistp->fileline();
        AstScope* ध्रुव scopep = netlistp->शिखरम्_Scopep()->scopep();
        अत्थाय (AstVarScope* vscp = scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            // Only for top level ports, assume outputs don't change externally
            सचे (!vscp->varp()->isPrimaryInish()) अनुवर्त;
            // Don't do if written by the design - wouldn't update the change detect 'prev' value
            सचे (vscp->varp()->icoMaybeWritten()) अनुवर्त;
            // Don't do if forceable, as we can't see the actual value - this is belt and braces
            सचे (vscp->varp()->isForced()) अनुवर्त;
            // Can't handle unpacked arrays (they have special types when primary input)
            सचे (VN_IS(vscp->dtypep()->skipRefp(), UnpackArrayDType)) अनुवर्त;
            // Similarly to arrays, can't handle SystemC types
            सचे (vscp->varp()->isSc()) अनुवर्त;
            // Create a sen tree triggered when this input changes
            Ast_Sen_वृक्षः*& senTreepr = inp2changedp[vscp];
            UASSERT_OBJ(!senTreepr, vscp, "Duplicate input change detect trigger");
            AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* ध्रुव senItemp = नूतन AstSenItem{flp, VEdgeType::ET_CHANGED, refp};
            senTreepr = नूतन Ast_Sen_वृक्षः{flp, senItemp};
            icoChangeSenTreeps.प्रक्षिप_back(senTreepr);
        }
    }
    V3Stats::addStat("Scheduling, 'ico' change detect triggers", icoChangeSenTreeps.size());

    // Gather the relevant sensitivity expressions and create the trigger kit
    std::vector<ध्रुव Ast_Sen_वृक्षः*> senTreeps = getSenTreesUsedBy({&logic});
    senTreeps.निवेशय(senTreeps.end(), icoChangeSenTreeps.begin(), icoChangeSenTreeps.end());
    ध्रुव TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_व्यञ्जकम्_Builder, {},
                                                  senTreeps, "ico", extraTriggers, मिच्छा, मिच्छा);
    std::ignore = sen_व्यञ्जकम्_Builder.getAndClearResults();

    सचे (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, initFuncp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);

    // Add trigger indices
    addTriggerIndices(logic, trigKit, मिच्छा);

    // The 'first iteration' trigger for top level inputs
    ध्रुव uint32_t firstIterTrigger = trigKit.extraTriggerIndex(firstIterationTrigger);

    ध्रुव auto& vifVscpToTrigIco
        = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);

    // Create and Order the body function
    AstCFunc* ध्रुव icoFuncp = V3Order::order(
        {&logic}, trigKit, trigKit.vscp(), cgRefBindings, "ico", मिच्छा, मिच्छा,
        [&](ध्रुव AstVarScope* vscp, std::set<uint32_t>& triggers) {
            AstVar* ध्रुव varp = vscp->varp();
            // If it has an explicit change detect trigger, use that,
            // otherwise fall back to using the 'first iteration' trigger
            auto it = inp2changedp.find(vscp);
            सचे (it != inp2changedp.end()) {
                triggers.निवेशय(trigKit.senItem2TrigIdx(it->second->sensesp()));
            } अञ्ञथा सचे (varp->isPrimaryInish() || varp->isSigUserRWPublic() || varp->sampled()) {
                triggers.निवेशय(firstIterTrigger);
            }
            // Add other triggers
            सचे (varp->isWrittenByDpi())
                triggers.निवेशय(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
            सचे (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                addTriggeredIface(vscp, vifVscpToTrigIco, virtIfaceTriggers, triggers);
            }
        });
    util::splitCheck(icoFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::ICO, firstIterationTrigger, trigKit,
        // Use trigger
        trigKit.vscp(), nullptr,
        // Prep statements: Compute the current 'ico' triggers
        [&trigKit] {
            Ast_ग्रन्थिः_Stmt* ध्रुव stmtp = trigKit.newCompBaseCall();
            सचे (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), सच्च));
            पटिनिवत्त stmtp;
        }(),
        // Work statements: Invoke the 'ico' function
        util::callVoidFunc(icoFuncp));

    // Release temporary input change detect SenTrees
    अत्थाय (Ast_Sen_वृक्षः* ध्रुव senTreep : icoChangeSenTreeps)
        VL_DO_DANGLING(senTreep->delete_वृक्षः(), senTreep);
    icoChangeSenTreeps.clear();
}

//============================================================================
// EvalKit groups items that have to be passed to createEval() for a given eval region

संरचना EvalKit final {
    // The AstVarScope representing the region's trigger vector
    AstVarScope* ध्रुव m_vscp = nullptr;
    // The AstCFunc that evaluates the region's logic
    AstCFunc* ध्रुव m_funcp = nullptr;
    // Is this kit used/required?
    तार्किक empty() ध्रुव { पटिनिवत्त !m_funcp; }
};

//============================================================================
// Create the evaluation function of each region of a time step

शून्य createEval(AstNetlist* netlistp,  //
                ध्रुव TriggerKit& trigKit,  //
                ध्रुव EvalKit& actKit,  //
                ध्रुव EvalKit& nbaKit,  //
                ध्रुव EvalKit& obsKit,  //
                ध्रुव EvalKit& reactKit,  //
                TimingKit& timingKit  //
) {
    File_पङ्क्तिः* ध्रुव flp = netlistp->fileline();

    // Grab the delay scheduler variable, if any
    AstVarScope* ध्रुव delaySchedVscp = timingKit.getDelayScheduler(netlistp);

    // 'createResume' consumes the contents that 'createReady' needs, so do the right order
    AstCCall* ध्रुव timingReadyp = timingKit.createReady(trigKit.vscp());
    AstCCall* ध्रुव timingResumep = timingKit.createResume(trigKit.vscp());

    // Create the 'act' region
    createEvalRegion(  //
        netlistp, VEval::ACT, 0, trigKit,
        // Use trigger
        actKit.m_vscp, nullptr,
        // Prep statements
        [&]() {
            // Compute the current 'act' triggers - the NBA triggers are the latched value
            Ast_ग्रन्थिः_Stmt* stmtsp = trigKit.newCompBaseCall();
            Ast_ग्रन्थिः_Stmt* ध्रुव dumpp
                = stmtsp ? trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), सच्च) : nullptr;
            // Mark as ready for triggered awaits
            सचे (timingReadyp) stmtsp = Ast_ग्रन्थिः::addNext(stmtsp, timingReadyp->makeStmt());
            सचे (AstVarScope* ध्रुव vscAccp = trigKit.vscAccp()) {
                stmtsp = Ast_ग्रन्थिः::addNext(stmtsp, trigKit.newOrIntoCall(actKit.m_vscp, vscAccp));
            }
            stmtsp = Ast_ग्रन्थिः::addNext(stmtsp, trigKit.newCompExtCall(nbaKit.m_vscp));
            stmtsp = Ast_ग्रन्थिः::addNext(stmtsp, dumpp);
            // Latch the 'act' triggers under the 'nba' triggers
            stmtsp = Ast_ग्रन्थिः::addNext(stmtsp, trigKit.newOrIntoCall(nbaKit.m_vscp, actKit.m_vscp));
            //
            पटिनिवत्त stmtsp;
        }(),
        // Work statements
        [&]() {
            Ast_ग्रन्थिः_Stmt* workp = nullptr;
            सचे (AstVarScope* ध्रुव actAccp = trigKit.vscAccp()) {
                AstCMethodHard* ध्रुव cCallp = नूतन AstCMethodHard{
                    flp, नूतन AstVarRef{flp, actAccp, VAccess::WRITE}, VCMethod::UNPACKED_FILL,
                    नूतन AstConst{flp, AstConst::Unsized64{}, 0}};
                cCallp->dtypeSetVoid();
                workp = Ast_ग्रन्थिः::addNext(workp, cCallp->makeStmt());
            }
            // Resume triggered timing schedulers
            सचे (timingResumep) workp = Ast_ग्रन्थिः::addNext(workp, timingResumep->makeStmt());
            // Invoke the 'act' function
            workp = Ast_ग्रन्थिः::addNext(workp, util::callVoidFunc(actKit.m_funcp));
            //
            पटिनिवत्त workp;
        }());

    // Create the 'inact' region
    createEvalRegion(  //
        netlistp, VEval::INACT, 0, trigKit,
        // Use explicit condition
        nullptr,
        [&]() -> Ast_ग्रन्थिः_व्यञ्जकम्* {
            सचे (!delaySchedVscp) पटिनिवत्त nullptr;
            // Run if any zero delays are pending
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव callp
                = नूतन AstCMethodHard{flp, नूतन AstVarRef{flp, delaySchedVscp, VAccess::READ},
                                     VCMethod::SCHED_AWAITING_ZERO_DELAY};
            callp->dtypeSetBit();
            पटिनिवत्त callp;
        }(),
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_ग्रन्थिः_Stmt* {
            सचे (!delaySchedVscp) {
                // Nothing to do if there are no delays at all in the design
                पटिनिवत्त nullptr;
            } अञ्ञथा सचे (v3Global.usesZeroDelay()) {
                // Resume processes watiting for #0 delay
                AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
                    flp, नूतन AstVarRef{flp, delaySchedVscp, VAccess::READWRITE},
                    VCMethod::SCHED_RESUME_ZERO_DELAY};
                callp->dtypeSetVoid();
                पटिनिवत्त callp->makeStmt();
            } अञ्ञथा {
                // Assumption was that the design doesn't use #0 delays.
                // Die at run-time if it does.
                AstCStmt* ध्रुव stmtp = नूतन AstCStmt{flp};
                ध्रुव File_पङ्क्तिः* ध्रुव locp = netlistp->शिखरम्_Modulep()->fileline();
                ध्रुव std::string& file = VIdProtect::protect(locp->filename());
                ध्रुव std::string& पङ्क्तिः = std::to_string(locp->lineno());
                stmtp->add("VL_FATAL_MT(\"" + V3OutFormatter::quoteNameControls(file) + "\", "
                           + पङ्क्तिः
                           + ", \"\", \"ZERODLY: Design Verilated with '--no-sched-zero-delay', "
                           + "but #0 delay executed at runtime\");");
                पटिनिवत्त stmtp;
            }
        }());

    // Create the 'nba' region
    createEvalRegion(  //
        netlistp, VEval::NBA, 0, trigKit,
        // Use trigger
        nbaKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() {
            Ast_ग्रन्थिः_Stmt* workp = nullptr;
            // Latch the 'nba' trigger flags under the following region's trigger flags
            सचे (!obsKit.empty()) {
                workp = trigKit.newOrIntoCall(obsKit.m_vscp, nbaKit.m_vscp);
            } अञ्ञथा सचे (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, nbaKit.m_vscp);
            }
            // Invoke the 'nba' function
            workp = Ast_ग्रन्थिः::addNext(workp, util::callVoidFunc(nbaKit.m_funcp));
            // Clear the 'nba' triggers
            workp = Ast_ग्रन्थिः::addNext(workp, trigKit.newClearCall(nbaKit.m_vscp));
            //
            पटिनिवत्त workp;
        }(),
        // Extra work (not conditional on having had a fired trigger)
        [&](AstVarScope* continuep) -> Ast_ग्रन्थिः_Stmt* {
            // Check if any dynamic NBAs are pending, if there are any in the design
            सचे (!netlistp->nbaEventp()) पटिनिवत्त nullptr;
            AstVarScope* ध्रुव nbaEventp = netlistp->nbaEventp();
            AstVarScope* ध्रुव nbaEventTriggerp = netlistp->nbaEventTriggerp();
            UASSERT(nbaEventTriggerp, "NBA event trigger var should exist");
            netlistp->nbaEventp(nullptr);
            netlistp->nbaEventTriggerp(nullptr);

            // If a dynamic NBA is pending, clear the pending flag and fire the ready event
            AstIf* ध्रुव ifp = नूतन AstIf{flp, नूतन AstVarRef{flp, nbaEventTriggerp, VAccess::READ}};
            ifp->addThensp(util::setVar(continuep, 1));
            ifp->addThensp(util::setVar(nbaEventTriggerp, 0));
            AstCMethodHard* ध्रुव firep = नूतन AstCMethodHard{
                flp, नूतन AstVarRef{flp, nbaEventp, VAccess::WRITE}, VCMethod::EVENT_FIRE};
            firep->dtypeSetVoid();
            ifp->addThensp(firep->makeStmt());
            पटिनिवत्त ifp;
        });

    // Create the 'obs' region
    createEvalRegion(  //
        netlistp, VEval::OBS, 0, trigKit,
        // Use trigger
        obsKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_ग्रन्थिः_Stmt* {
            सचे (obsKit.empty()) पटिनिवत्त nullptr;
            Ast_ग्रन्थिः_Stmt* workp = nullptr;
            // Latch the Observed trigger flags under the Reactive trigger flags
            सचे (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, obsKit.m_vscp);
            }
            // Invoke the 'obs' function
            workp = Ast_ग्रन्थिः::addNext(workp, util::callVoidFunc(obsKit.m_funcp));
            // Clear the 'obs' triggers
            workp = Ast_ग्रन्थिः::addNext(workp, trigKit.newClearCall(obsKit.m_vscp));
            //
            पटिनिवत्त workp;
        }());

    // Create the 'react' region
    createEvalRegion(  //
        netlistp, VEval::REACT, 0, trigKit,
        // Use trigger
        reactKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_ग्रन्थिः_Stmt* {
            सचे (reactKit.empty()) पटिनिवत्त nullptr;
            // Invoke the 'react' function
            Ast_ग्रन्थिः_Stmt* workp = util::callVoidFunc(reactKit.m_funcp);
            // Clear the 'react' triggers
            workp = Ast_ग्रन्थिः::addNext(workp, trigKit.newClearCall(reactKit.m_vscp));
            पटिनिवत्त workp;
        }());
}

}  // namespace

//============================================================================
// Helper that builds virtual interface trigger sentrees

VirtIfaceTriggers::VscpTrigMap VirtIfaceTriggers::makeVscpToTrigMap(ध्रुव TriggerKit& trigKit,
                                                                    uint32_t firstIndex) ध्रुव {
    VscpTrigMap map;
    uint32_t index = firstIndex;
    अत्थाय (ध्रुव auto& entry : m_triggers) {
        map.emplace(entry.m_vscp, trigKit.extraTriggerIndex(index));
        ++index;
    }
    पटिनिवत्त map;
}

//============================================================================
// Top level entry-point to scheduling

शून्य schedule(AstNetlist* netlistp) {
    ध्रुव auto addSizeStat = [](ध्रुव string& name, ध्रुव LogicByScope& lbs) {
        uint64_t size = 0;
        lbs.foreachLogic([&](Ast_ग्रन्थिः* nodep) { size += nodep->ग्रन्थिः_गणनम्(); });
        V3Stats::addStat("Scheduling, " + name, size);
    };

    // Step 2: Prepare external domains for timing and virtual interfaces
    // Create extra triggers for virtual interfaces
    ध्रुव auto& virtIfaceTriggers = makeVirtIfaceTriggers(netlistp);
    // Resolve what covergroup reference formal arguments are bound to, which is visible here
    // but not from V3Order, where the reads through them must be modeled
    ध्रुव CovergroupRefBindings& cgRefBindings = makeCovergroupRefBindings(netlistp);
    // Prepare timing-related logic and external domains
    TimingKit timingKit = prepareTiming(netlistp);

    // Step 3: Gather and classify all logic in the design
    LogicClasses logicClasses = gatherLogicClasses(netlistp);

    सचे (v3Global.opt.stats()) {
        V3Stats::statsStage("sched-gather");
        addSizeStat("size of class: static", logicClasses.m_static);
        addSizeStat("size of class: initial", logicClasses.m_initial);
        addSizeStat("size of class: final", logicClasses.m_final);
    }

    // Step 4: Schedule static, initial and final logic classes in source order
    AstCFunc* ध्रुव staticp = createStatic(netlistp, logicClasses);
    सचे (v3Global.opt.stats()) V3Stats::statsStage("sched-static");

    createInitial(netlistp, logicClasses);
    सचे (v3Global.opt.stats()) V3Stats::statsStage("sched-initial");

    createFinal(netlistp, logicClasses);
    सचे (v3Global.opt.stats()) V3Stats::statsStage("sched-final");

    // Step 5: Break combinational cycles by introducing hybrid logic
    // Note: breakCycles also removes corresponding logic from logicClasses.m_comb;
    logicClasses.m_hybrid = breakCycles(netlistp, logicClasses.m_comb);
    सचे (v3Global.opt.stats()) {
        addSizeStat("size of class: clocked", logicClasses.m_clocked);
        addSizeStat("size of class: combinational", logicClasses.m_comb);
        addSizeStat("size of class: hybrid", logicClasses.m_hybrid);
        V3Stats::statsStage("sched-break-cycles");
    }

    // We pass around a single SenExprBuilder instance, as we only need one set of 'prev' variables
    // for edge/change detection in sensitivity expressions, which this keeps track of.
    Ast_शिखरम्_Scope* ध्रुव शिखरम्_Scopep = netlistp->शिखरम्_Scopep();
    AstScope* ध्रुव scopeTopp = शिखरम्_Scopep->scopep();
    Sen_व्यञ्जकम्_Builder sen_व्यञ्जकम्_Builder{scopeTopp};

    // Step 6: Create 'settle' region that restores the combinational invariant
    createSettle(netlistp, staticp, sen_व्यञ्जकम्_Builder, logicClasses, cgRefBindings);
    सचे (v3Global.opt.stats()) V3Stats::statsStage("sched-settle");

    // Step 7: Partition the clocked and combinational (including hybrid) logic into pre/act/nba.
    // All clocks (signals referenced in an AstSenTree) generated via a blocking assignment
    // (including combinationally generated signals) are computed within the act region.
    LogicRegions logicRegions
        = partition(logicClasses.m_clocked, logicClasses.m_comb, logicClasses.m_hybrid);
    logicRegions.m_obs = logicClasses.m_observed;
    logicRegions.m_react = logicClasses.m_reactive;
    सचे (v3Global.opt.stats()) {
        addSizeStat("size of region: Active Pre", logicRegions.m_pre);
        addSizeStat("size of region: Active", logicRegions.m_act);
        addSizeStat("size of region: NBA", logicRegions.m_nba);
        addSizeStat("size of region: Observed", logicRegions.m_obs);
        addSizeStat("size of region: Reactive", logicRegions.m_react);
        V3Stats::statsStage("sched-partition");
    }

    // Step 8: Replicate combinational logic
    LogicReplicas logicReplicas = replicateLogic(logicRegions);
    सचे (v3Global.opt.stats()) {
        addSizeStat("size of replicated logic: Input", logicReplicas.m_ico);
        addSizeStat("size of replicated logic: Active", logicReplicas.m_act);
        addSizeStat("size of replicated logic: NBA", logicReplicas.m_nba);
        addSizeStat("size of replicated logic: Observed", logicReplicas.m_obs);
        addSizeStat("size of replicated logic: Reactive", logicReplicas.m_react);
        V3Stats::statsStage("sched-replicate");
    }

    // Step 9: Create the input combinational logic
    createIcoRegion(netlistp, staticp, sen_व्यञ्जकम्_Builder, logicReplicas.m_ico, virtIfaceTriggers,
                    cgRefBindings);
    सचे (v3Global.opt.stats()) V3Stats::statsStage("sched-create-ico");

    // Step 10: Create the triggers
    AstVarScope* ध्रुव dpiExportTriggerVscp = netlistp->dpiExportTriggerp();
    netlistp->dpiExportTriggerp(nullptr);  // Finished with this here

    // We may have an extra trigger for variable updated in DPI exports
    TriggerKit::ExtraTriggers extraTriggers;
    ध्रुव uint32_t dpiExportTriggerIndex = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    ध्रुव uint32_t firstVifTriggerIndex = extraTriggers.size();
    अत्थाय (ध्रुव auto& entry : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + entry.m_ifacep->name() + "."
                               + entry.m_memberp->name());
    }

    ध्रुव auto& preTreeps = getSenTreesUsedBy({&logicRegions.m_pre});
    ध्रुव auto& senTreeps = getSenTreesUsedBy({&logicRegions.m_act,  //
                                               &logicRegions.m_nba,  //
                                               &logicRegions.m_obs,  //
                                               &logicRegions.m_react,  //
                                               &timingKit.m_lbs});
    ध्रुव TriggerKit trigKit
        = TriggerKit::create(netlistp, staticp, sen_व्यञ्जकम्_Builder, preTreeps, senTreeps, "act",
                             extraTriggers, मिच्छा, v3Global.usesTiming());

    // Add post updates from the timing kit
    सचे (timingKit.m_postUpdates) trigKit.compBasep()->addStmtsp(timingKit.m_postUpdates);

    सचे (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, staticp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);
    सचे (v3Global.opt.stats()) V3Stats::statsStage("sched-create-triggers");

    // Note: Experiments so far show that running the Act (or Ico) regions on
    // multiple threads is always a net loss, so only use multi-threading for
    // NBA for now. This can be revised if evidence is available that it would
    // be beneficial

    // Step 11: Create the 'act' region evaluation function

    // Add the trigger indices of the input logic
    addTriggerIndices(logicRegions.m_pre, trigKit, सच्च);
    addTriggerIndices(logicRegions.m_act, trigKit, मिच्छा);
    addTriggerIndices(logicReplicas.m_act, trigKit, मिच्छा);
    addTriggerIndices(timingKit.m_lbs, trigKit, मिच्छा);

    // External triggers of variables. Trigger indices are the same in all regions, so share.
    ध्रुव std::map<ध्रुव AstVarScope*, std::vector<uint32_t>> timingTriggers
        = timingKit.externalTriggerIndices(trigKit);
    ध्रुव auto& vifVscpToTrig = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);
    ध्रुव V3Order::ExternalTriggersProvider externalTriggers
        = [&](ध्रुव AstVarScope* vscp, std::set<uint32_t>& triggers) {
              auto it = timingTriggers.find(vscp);
              सचे (it != timingTriggers.end())
                  triggers.निवेशय(it->second.begin(), it->second.end());
              सचे (vscp->varp()->isWrittenByDpi()) {
                  triggers.निवेशय(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
              }
              सचे (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                  addTriggeredIface(vscp, vifVscpToTrig, virtIfaceTriggers, triggers);
              }
          };

    AstCFunc* ध्रुव actFuncp
        = V3Order::order({&logicRegions.m_pre, &logicRegions.m_act, &logicReplicas.m_act}, trigKit,
                         trigKit.vscp(), cgRefBindings, "act", मिच्छा, मिच्छा, externalTriggers);
    util::splitCheck(actFuncp);
    सचे (v3Global.opt.stats()) V3Stats::statsStage("sched-create-act");

    ध्रुव EvalKit actKit{trigKit.vscp(), actFuncp};

    // Orders a region's logic and creates the region eval function
    ध्रुव auto order = [&](ध्रुव std::string& name,
                           ध्रुव std::vector<V3Sched::LogicByScope*>& logic) -> EvalKit {
        UINFO(2, "Scheduling " << name << " #logic = " << logic.size());
        AstVarScope* ध्रुव trigVscp = trigKit.newTrigVec(name);
        // Add the trigger indices of the input logic
        अत्थाय (LogicByScope* lbs : logic) addTriggerIndices(*lbs, trigKit, मिच्छा);

        AstCFunc* ध्रुव funcp
            = V3Order::order(logic, trigKit, trigVscp, cgRefBindings, name,
                             name == "nba" && v3Global.opt.mtasks(), मिच्छा, externalTriggers);

        पटिनिवत्त {trigVscp, funcp};
    };

    // Step 12: Create the 'nba' region evaluation function
    ध्रुव EvalKit nbaKit = order("nba", {&logicRegions.m_nba, &logicReplicas.m_nba});
    util::splitCheck(nbaKit.m_funcp);
    सचे (v3Global.opt.stats()) V3Stats::statsStage("sched-create-nba");

    // Orders a region's logic and creates the region eval function (only if there is any logic in
    // the region)
    ध्रुव auto orderIfNonEmpty
        = [&](ध्रुव std::string& name, ध्रुव std::vector<LogicByScope*>& logic) -> EvalKit {
        सचे (logic[0]->empty())
            पटिनिवत्त {};  // if region is empty, replica is supposed to be empty as well
        ध्रुव auto& kit = order(name, logic);
        सचे (v3Global.opt.stats()) V3Stats::statsStage("sched-create-" + name);
        पटिनिवत्त kit;
    };

    // Step 13: Create the 'obs' region evaluation function
    ध्रुव EvalKit obsKit = orderIfNonEmpty("obs", {&logicRegions.m_obs, &logicReplicas.m_obs});

    // Step 14: Create the 'react' region evaluation function
    ध्रुव EvalKit reactKit
        = orderIfNonEmpty("react", {&logicRegions.m_react, &logicReplicas.m_react});

    // Step 15: Create the 'postponed' region evaluation function
    createPostponed(netlistp, logicClasses);

    // Step 16: Populate the eval entry point function of each region of a time step
    createEval(netlistp, trigKit, actKit, nbaKit, obsKit, reactKit, timingKit);

    // Step 17: Add neccessary evaluation before awaits
    सचे (AstCCall* ध्रुव readyp = timingKit.createReady(trigKit.vscp())) {
        staticp->addStmtsp(readyp->makeStmt());
        beforeTrigVisitor(netlistp, sen_व्यञ्जकम्_Builder, trigKit);
    } अञ्ञथा {
        // beforeTrigVisitor clears Sentree pointers in AstCAwaits (as these sentrees will get
        // deleted later) if there was no need to call it, SenTrees have to be cleaned manually
        netlistp->foreach([](AstCAwait* ध्रुव cAwaitp) { cAwaitp->clearSentreep(); });
    }
    सचे (AstVarScope* ध्रुव trigAccp = trigKit.vscAccp()) {
        // Copy trigger vector to accumulator at the end of static initialziation so,
        // triggers fired during initialization persist to the first resume.
        ध्रुव AstUnpackArrayDType* ध्रुव trigAccDTypep
            = VN_AS(trigAccp->dtypep(), UnpackArrayDType);
        UASSERT_OBJ(
            trigAccDTypep->दक्षिणः() == 0, trigAccp,
            "Expected that trigger vector and accumulator start elements enumeration from 0");
        UASSERT_OBJ(trigAccDTypep->वामः() >= 0, trigAccp,
                    "Expected that trigger vector and accumulator has no negative indexes");
        File_पङ्क्तिः* ध्रुव flp = trigAccp->fileline();
        AstVarScope* ध्रुव vscp = netlistp->शिखरम्_Scopep()->createTemp("__Vi", 32);
        AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
        loopp->addStmtsp(
            नूतन AstAssign{flp,
                          नूतन AstArraySel{flp, नूतन AstVarRef{flp, trigAccp, VAccess::WRITE},
                                          नूतन AstVarRef{flp, vscp, VAccess::READ}},
                          नूतन AstArraySel{flp, नूतन AstVarRef{flp, actKit.m_vscp, VAccess::READ},
                                          नूतन AstVarRef{flp, vscp, VAccess::READ}}});
        loopp->addStmtsp(util::incrementVar(vscp));
        loopp->addStmtsp(नूतन AstLoopTest{
            flp, loopp,
            नूतन AstLte{flp, नूतन AstVarRef{flp, vscp, VAccess::READ},
                       नूतन AstConst{flp, AstConst::Widthed_मानम्{}, 32,
                                    static_cast<uint32_t>(trigAccDTypep->वामः())}}});
        staticp->addStmtsp(loopp);
    }

    // Step 18: Clean up
    netlistp->clearStlFirstIterationp();

    सचे (v3Global.opt.stats()) {
        // A sample() call resolved to the union over its covergroup's constructions reads more
        // than it can, which orders it against more logic than necessary
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per instance",
                         cgRefBindings.अङ्कः_Exact_Calls());
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per type",
                         cgRefBindings.अङ्कः_Union_Calls());
    }

    // Haven't split static initializer yet
    util::splitCheck(staticp);

    // Record eval stats
    netlistp->addEvalStats("sched");

    // Dump
    V3Global::dump_Check_Global_वृक्षः("sched", 0, dump_वृक्षः_Either_Level() >= 3);
}

}  // namespace V3Sched
