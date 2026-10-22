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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Sched.h"

#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitCBase.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitV.h"
#ଅନ୍ତର୍ଭାଵ "V3Order.h"
#ଅନ୍ତର୍ଭାଵ "V3SenExprBuilder.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ନାମାକାଶ V3Sched {

ନାମାକାଶ {

//============================================================================
// Utility functions

std::vector<ଅଚଳ Ast_Sen_ବୃକ୍ଷ*> getSenTreesUsedBy(ଅଚଳ std::vector<ଅଚଳ LogicByScope*>& lbsps) {
    ଅଚଳ VNUser1InUse user1InUse;
    std::vector<ଅଚଳ Ast_Sen_ବୃକ୍ଷ*> result;
    ପାଇଁ (ଅଚଳ LogicByScope* ଅଚଳ lbsp : lbsps) {
        ପାଇଁ (ଅଚଳ auto& entry : *lbsp) {
            AstActive* ଅଚଳ activep = entry.m_activep;
            Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep = activep->sentreep();
            ଯଦି (senTreep->user1SetOnce()) ଜାରି;
            ଯଦି (senTreep->hasClocked() || senTreep->hasHybrid()) result.ଠେଲ_back(senTreep);
        }
    }
    ଫେରନ୍ତୁ result;
}

ଶୂନ୍ୟ addTriggerIndices(LogicByScope& lbs, ଅଚଳ TriggerKit& trigKit, ତାର୍କିକ pre) {
    ପାଇଁ (LogicByScope::Entry& entry : lbs) {
        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep = entry.m_activep->sentreep();
        ଯଦି (senTreep->hasCombo()) ଜାରି;
        entry.m_triggers = trigKit.triggerIndices(senTreep, pre);
    }
}

// Add the VIF triggers that a given VarScope should be sensitive to.
// Case 2 (non-virtual interface read): sensitive to that specific VarScope's trigger only
// Case 3 (virtual interface read): sensitive to all member triggers of the same interface type
ଶୂନ୍ୟ addTriggeredIface(ଅଚଳ AstVarScope* vscp, ଅଚଳ VirtIfaceTriggers::VscpTrigMap& vscpToTrig,
                       ଅଚଳ VirtIfaceTriggers& virtIfaceTriggers, std::set<uint32_t>& triggers) {
    ଯଦି (vscp->varp()->isVirtIface()) {
        // Virtual interface variable -- sensitive to all member triggers of this interface type
        ଅଚଳ AstIface* ଅଚଳ ifacep = VN_AS(vscp->varp()->dtypep(), IfaceRefDType)->ifacep();
        ପାଇଁ (ଅଚଳ auto& entry : virtIfaceTriggers.m_triggers) {
            ଯଦି (entry.m_ifacep == ifacep) {
                ଅଚଳ auto it = vscpToTrig.find(entry.m_vscp);
                ଯଦି (it != vscpToTrig.end()) triggers.ଭର୍ତ୍ତି(it->second);
            }
        }
    } ଅନ୍ୟଥା {
        // Non-virtual interface member -- sensitive to this VarScope's trigger only
        ଅଚଳ auto it = vscpToTrig.find(vscp);
        ଯଦି (it != vscpToTrig.end()) triggers.ଭର୍ତ୍ତି(it->second);
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
ଶୂନ୍ୟ createEvalRegion(
    AstNetlist* netlistp,  //
    VEval eval,  // The entry point of the current region
    // Index of the region's 'first iteration' extra trigger, if it has one, otherwise ignored
    uint32_t firstIterTrigger,
    ଅଚଳ TriggerKit& trigKit,  // The trigger kit
    AstVarScope* trigp,  // The trigger vector - may be nullptr if no triggers or using 'condp'
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp,  // Explicit condition that must be true to run 'phaseWorkp'
    Ast_ଗଣ୍ଠି_Stmt* phasePrepp,  // Prep statements run before checking triggers
    Ast_ଗଣ୍ଠି_Stmt* phaseWorkp,  // The work to do if anything triggered
    // Extra statements to run after the work, even if no triggers fired. This function is
    // passed a variable, which must be set to true if we must continue and loop again,
    // and must be unmodified otherwise.
    std::function<Ast_ଗଣ୍ଠି_Stmt*(AstVarScope*)> phaseExtra = [](AstVarScope*) { ଫେରନ୍ତୁ nullptr; }  //
) {
    UASSERT(!trigp || !condp, "Cannot use both 'trigp' and 'condp' in 'createEvalRegion'");
    UASSERT(!eval.firstIteration() || trigp,
            "Region without triggers cannot need a first iteration flag");

    // All work is under a trigger or condition, so with neither the region has
    // nothing to evaluate, and what we create below reduces to a no-op function.

    ଅଚଳ std::string tag = eval.tag();
    Ast_ଶୀର୍ଷ_Scope* ଅଚଳ ଶୀର୍ଷ_Scopep = netlistp->ଶୀର୍ଷ_Scopep();
    File_ଧାଡ଼ି* ଅଚଳ flp = netlistp->fileline();

    // Populate the trigger dump entry point function
    ଯଦି (trigp) {
        netlistp->dumpTriggersFuncp(eval)->addStmtsp(trigKit.newDumpCall(trigp, tag, ମିଥ୍ୟା));
    }

    AstCFunc* ଅଚଳ funcp = netlistp->evalFuncp(eval);

    // A flag is passed from the run-time eval loop if this is the first iteration of the
    // current loop
    ଯଦି (eval.firstIteration()) {
        AstVarScope* ଅଚଳ firstIterArgp = util::newArgument(funcp, netlistp->findBitDType(),
                                                             "firstIteration", VDirection::ନିବେଶ);

        // Set the region's 'first iteration' trigger straight from the argument
        funcp->addStmtsp(trigKit.newExtraTriggerAssignment(firstIterArgp, firstIterTrigger));

        // Only 'stl' also needs a module level flag, for design logic that reads
        // it directly (see V3Timing). Those reads can be anywhere in the design,
        // hence module level. Always created, even if nothing reads it.
        // TODO: get rid of this special case
        ଯଦି (eval == VEval::STL) {
            AstVarScope* ଅଚଳ firstIterp = netlistp->stlFirstIterationp();
            firstIterp->varp()->noReset(ସତ୍ୟ);
            firstIterp->varp()->isInternal(ସତ୍ୟ);
            funcp->addStmtsp(ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, firstIterp, VAccess::WRITE},
                                           ନୂତନ AstVarRef{flp, firstIterArgp, VAccess::READ}});
        }
    }

    {
        // Add the preparatory statements
        funcp->addStmtsp(phasePrepp);

        // The execute flag
        AstVarScope* ଅଚଳ executeFlagp = ଶୀର୍ଷ_Scopep->createTemp("__V" + tag + "Execute", 1);
        executeFlagp->varp()->noReset(ସତ୍ୟ);

        // If there is work in this region, execute it if any triggers fired
        ଯଦି (phaseWorkp) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = ନୂତନ AstVarRef{flp, executeFlagp, VAccess::WRITE};
            // If using explicit condition, that directly determines whether to execute,
            // otherwise check if any triggers are fired
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = condp ? condp : trigKit.newAnySetCall(trigp);
            funcp->addStmtsp(ନୂତନ AstAssign{flp, lhsp, rhsp});

            // Add the work
            AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, ନୂତନ AstVarRef{flp, executeFlagp, VAccess::READ}};
            ifp->addThensp(phaseWorkp);
            funcp->addStmtsp(ifp);
        }

        // Construct the extra statements
        Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ extraWorkp = phaseExtra(executeFlagp);
        ଯଦି (extraWorkp) funcp->addStmtsp(extraWorkp);

        // The function returns true iff it did run work
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ retp
            = phaseWorkp || extraWorkp
                  ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstVarRef{flp, executeFlagp, VAccess::READ})
                  : static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstConst{flp, AstConst::BitFalse{}});
        funcp->addStmtsp(ନୂତନ AstCReturn{flp, retp});
    }
}

//============================================================================
// Collect and classify all logic in the design

LogicClasses gatherLogicClasses(AstNetlist* netlistp) {
    LogicClasses result;

    netlistp->foreach([&](AstScope* scopep) {
        scopep->foreach([&](AstActive* activep) {
            Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep = activep->sentreep();
            ଯଦି (senTreep->hasStatic()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "static initializer with additional sensitivities");
                result.m_static.add(scopep, activep);
            } ଅନ୍ୟଥା ଯଦି (senTreep->hasInitial()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'initial' logic with additional sensitivities");
                result.m_initial.add(scopep, activep);
            } ଅନ୍ୟଥା ଯଦି (senTreep->hasFinal()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'final' logic with additional sensitivities");
                result.m_final.add(scopep, activep);
            } ଅନ୍ୟଥା ଯଦି (senTreep->hasCombo()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "combinational logic with additional sensitivities");
                ଯଦି (VN_IS(activep->stmtsp(), AlwaysPostponed)) {
                    result.m_postponed.add(scopep, activep);
                } ଅନ୍ୟଥା {
                    result.m_comb.add(scopep, activep);
                }
            } ଅନ୍ୟଥା {
                UASSERT_OBJ(senTreep->hasClocked(), activep, "What else could it be?");
                ଯଦି (VN_IS(activep->stmtsp(), AlwaysObserved)) {
                    result.m_observed.add(scopep, activep);
                } ଅନ୍ୟଥା ଯଦି (VN_IS(activep->stmtsp(), AlwaysReactive)) {
                    result.m_reactive.add(scopep, activep);
                } ଅନ୍ୟଥା {
                    result.m_clocked.add(scopep, activep);
                }
            }
        });
    });

    ଫେରନ୍ତୁ result;
}

//============================================================================
// Simple ordering in source order

ଶୂନ୍ୟ orderSequentially(AstCFunc* funcp, ଅଚଳ LogicByScope& lbs) {
    // Create new subfunc for scope
    ଅଚଳ auto createNewSubFuncp = [&](AstScope* ଅଚଳ scopep) {
        ଅଚଳ string subName{funcp->name() + "__" + scopep->nameDotless()};
        AstCFunc* ଅଚଳ subFuncp = ନୂତନ AstCFunc{scopep->fileline(), subName, scopep};
        subFuncp->isLoose(ସତ୍ୟ);
        subFuncp->isConst(ମିଥ୍ୟା);
        subFuncp->declPrivate(ସତ୍ୟ);
        subFuncp->slow(funcp->slow());
        scopep->addBlocksp(subFuncp);
        // Call it from the top function
        funcp->addStmtsp(util::callVoidFunc(subFuncp));
        ଫେରନ୍ତୁ subFuncp;
    };
    ଅଚଳ VNUser1InUse user1InUse;  // AstScope -> AstCFunc: the sub-function for the scope
    ଅଚଳ VNUser2InUse user2InUse;  // AstScope -> int: sub-function counter used for names
    ପାଇଁ (ଅଚଳ auto& entry : lbs) {
        AstScope* ଅଚଳ scopep = entry.m_scopep;
        AstActive* ଅଚଳ activep = entry.m_activep;
        // Create a sub-function per scope so we can V3Combine them later
        ଯଦି (!scopep->user1p()) scopep->user1p(createNewSubFuncp(scopep));
        // Add statements to sub-function
        ପାଇଁ (Ast_ଗଣ୍ଠି *logicp = activep->stmtsp(), *nextp; logicp; logicp = nextp) {
            auto* subFuncp = VN_AS(scopep->user1p(), CFunc);
            nextp = logicp->nextp();
            ଯଦି (Ast_ଗଣ୍ଠି_Procedure* ଅଚଳ procp = VN_CAST(logicp, ଗଣ୍ଠି_Procedure)) {
                ଯଦି (Ast_ଗଣ୍ଠି* bodyp = procp->stmtsp()) {
                    bodyp->unlinkFrBackWithNext();
                    // If the process is suspendable, we need a separate function (a coroutine)
                    ଯଦି (procp->isSuspendable()) {
                        funcp->slow(ମିଥ୍ୟା);
                        subFuncp = createNewSubFuncp(scopep);
                        subFuncp->name(subFuncp->name() + "__Vtiming__"
                                       + cvtToStr(scopep->user2Inc()));
                        subFuncp->rtnType("VlCoroutine");
                        ଯଦି (VN_IS(procp, Always)) {
                            subFuncp->slow(ମିଥ୍ୟା);
                            File_ଧାଡ଼ି* ଅଚଳ flp = procp->fileline();
                            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = ନୂତନ AstCExpr{
                                flp, "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish())", 1};
                            AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{flp};
                            loopp->addStmtsp(ନୂତନ AstLoopTest{flp, loopp, condp});
                            loopp->addStmtsp(bodyp);
                            bodyp = loopp;
                        }
                    }
                    subFuncp->addStmtsp(bodyp);
                    ଯଦି (procp->needProcess()) subFuncp->setNeedProcess();
                    // A coroutine holds a single process, so it is complete here
                    ଯଦି (procp->isSuspendable()) util::splitCheck(subFuncp);
                }
            } ଅନ୍ୟଥା {
                logicp->unlinkFrBack();
                subFuncp->addStmtsp(logicp);
            }
        }
        ଯଦି (activep->backp()) activep->unlinkFrBack();
        VL_DO_DANGLING(activep->delete_ବୃକ୍ଷ(), activep);
    }
    // Split sub-functions once complete
    ପାଇଁ (ଅଚଳ auto& entry : lbs) {
        AstScope* ଅଚଳ scopep = entry.m_scopep;
        ଯଦି (AstCFunc* ଅଚଳ subFuncp = VN_AS(scopep->user1p(), CFunc)) {
            util::splitCheck(subFuncp);
            scopep->user1p(nullptr);
        }
    }
}

//============================================================================
// Create simply ordered functions

AstCFunc* createStatic(AstNetlist* netlistp, ଅଚଳ LogicClasses& logicClasses) {
    AstCFunc* ଅଚଳ funcp = netlistp->evalFuncp(VEval::STATIC);

    ଅଚଳ LogicByScope& orig = logicClasses.m_static;
    ଯଦି (orig.size() <= 1) {
        orderSequentially(funcp, orig);
        ଫେରନ୍ତୁ funcp;
    }

    // Level-based module sorting can reorder packages so that an importing
    // package runs before the imported one.  Re-sort package entries by source
    // file position to restore compilation order (IEEE 1800-2023 26.3).
    std::vector<size_t> indices(orig.size());
    ପାଇଁ (size_t i = 0; i < orig.size(); ++i) indices[i] = i;
    std::stable_sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ modA = orig[a].m_scopep->modp();
        ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ modB = orig[b].m_scopep->modp();
        ଅଚଳ ତାର୍କିକ isPkgA = VN_IS(modA, Package);
        ଅଚଳ ତାର୍କିକ isPkgB = VN_IS(modB, Package);
        ଯଦି (isPkgA != isPkgB) ଫେରନ୍ତୁ isPkgA;  // Packages before non-packages
        ଯଦି (isPkgA && isPkgB) {
            // Sort packages by source file position (compilation order)
            ଫେରନ୍ତୁ modA->fileline()->operatorCompare(*modB->fileline()) < 0;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Both non-package: preserve original order
    });
    LogicByScope sorted;
    ପାଇଁ (ଅଚଳ size_t i : indices) sorted.add(orig[i].m_scopep, orig[i].m_activep);

    orderSequentially(funcp, sorted);
    ଫେରନ୍ତୁ funcp;  // Not splitting yet as it is not final
}

ଶୂନ୍ୟ createInitial(AstNetlist* netlistp, ଅଚଳ LogicClasses& logicClasses) {
    AstCFunc* ଅଚଳ funcp = netlistp->evalFuncp(VEval::INITIAL);
    orderSequentially(funcp, logicClasses.m_initial);
    util::splitCheck(funcp);
}

ଶୂନ୍ୟ createPostponed(AstNetlist* netlistp, ଅଚଳ LogicClasses& logicClasses) {
    AstCFunc* ଅଚଳ funcp = netlistp->evalFuncp(VEval::POSTPONED);
    orderSequentially(funcp, logicClasses.m_postponed);
    util::splitCheck(funcp);
}

ଶୂନ୍ୟ createFinal(AstNetlist* netlistp, ଅଚଳ LogicClasses& logicClasses) {
    AstCFunc* ଅଚଳ funcp = netlistp->evalFuncp(VEval::FINAL);
    orderSequentially(funcp, logicClasses.m_final);
    util::splitCheck(funcp);
}

//============================================================================
// Helper that creates virtual interface value-change triggers

ଶୂନ୍ୟ addVirtIfaceTriggerAssignments(AstNetlist* netlistp, AstCFunc* initFuncp,
                                    ଅଚଳ VirtIfaceTriggers& virtIfaceTriggers,
                                    uint32_t firstIndex, ଅଚଳ TriggerKit& trigKit) {
    uint32_t index = firstIndex;
    ପାଇଁ (ଅଚଳ auto& entry : virtIfaceTriggers.m_triggers) {
        trigKit.add_ମୂଲ୍ୟ_Change_Trigger_Assignment(netlistp, initFuncp, entry.m_vscp, index);
        ++index;
    }
}

// Order the combinational logic to create the 'stl' region
ଶୂନ୍ୟ createSettle(AstNetlist* netlistp, AstCFunc* ଅଚଳ initFuncp, Sen_ପଦସମଷ୍ଟି_Builder& sen_ପଦସମଷ୍ଟି_Bulider,
                  LogicClasses& logicClasses, ଅଚଳ CovergroupRefBindings& cgRefBindings) {
    // Clone, because ordering is destructive, but we still need them for the other regions
    LogicByScope comb = logicClasses.m_comb.clone();
    LogicByScope hybrid = logicClasses.m_hybrid.clone();

    // We have an extra trigger denoting this is the first iteration of the settle loop
    TriggerKit::ExtraTriggers extraTriggers;
    ଅଚଳ uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");

    // Gather the relevant sensitivity expressions and create the trigger kit
    ଅଚଳ auto& senTreeps = getSenTreesUsedBy({&comb, &hybrid});
    ଅଚଳ TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_ପଦସମଷ୍ଟି_Bulider, {},
                                                  senTreeps, "stl", extraTriggers, ସତ୍ୟ, ମିଥ୍ୟା);

    // Add trigger indices (comb has none, so only do the hybrid)
    addTriggerIndices(hybrid, trigKit, ମିଥ୍ୟା);

    // First trigger is for pure combinational triggers (first iteration)
    ଅଚଳ uint32_t ନିବେଶ_Changed = trigKit.extraTriggerIndex(firstIterationTrigger);

    // Create and the body function
    AstCFunc* ଅଚଳ stlFuncp = V3Order::order(
        {&comb, &hybrid}, trigKit, trigKit.vscp(), cgRefBindings, "stl", ମିଥ୍ୟା, ସତ୍ୟ,
        [=](ଅଚଳ AstVarScope*, std::set<uint32_t>& triggers) { triggers.ଭର୍ତ୍ତି(ନିବେଶ_Changed); });
    util::splitCheck(stlFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::STL, firstIterationTrigger, trigKit,
        // Use trigger
        trigKit.vscp(), nullptr,
        // Explicit condition
        // Prep statements: Compute the current 'stl' triggers
        [&trigKit] {
            Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ stmtp = trigKit.newCompBaseCall();
            ଯଦି (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), ସତ୍ୟ));
            ଫେରନ୍ତୁ stmtp;
        }(),
        // Work statements: Invoke the 'stl' function
        util::callVoidFunc(stlFuncp));
}

//============================================================================
// Order the replicated combinational logic to create the 'ico' region

ଶୂନ୍ୟ createIcoRegion(AstNetlist* netlistp, AstCFunc* ଅଚଳ initFuncp,
                     Sen_ପଦସମଷ୍ଟି_Builder& sen_ପଦସମଷ୍ଟି_Builder, LogicByScope& logic,
                     ଅଚଳ VirtIfaceTriggers& virtIfaceTriggers,
                     ଅଚଳ CovergroupRefBindings& cgRefBindings) {
    // SystemC only: Any top level inputs feeding a combinational logic must be marked,
    // so we can make them sc_sensitive
    ଯଦି (v3Global.opt.systemC()) {
        logic.foreachLogic([](Ast_ଗଣ୍ଠି* logicp) {
            logicp->foreach([](AstVarRef* refp) {
                ଯଦି (refp->access().isWriteOnly()) ଫେରନ୍ତୁ;
                AstVarScope* ଅଚଳ vscp = refp->varScopep();
                ଯଦି (vscp->scopep()->is_ଶୀର୍ଷ() && vscp->varp()->isNonOutput()) {
                    vscp->varp()->scSensitive(ସତ୍ୟ);
                }
            });
        });
    }

    // We have some extra trigger denoting external conditions
    AstVarScope* ଅଚଳ dpiExportTriggerVscp = netlistp->dpiExportTriggerp();

    TriggerKit::ExtraTriggers extraTriggers;
    ଅଚଳ uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");
    ଅଚଳ uint32_t dpiExportTriggerIndex = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    ଅଚଳ uint32_t firstVifTriggerIndex = extraTriggers.size();
    ପାଇଁ (ଅଚଳ auto& entry : virtIfaceTriggers.m_triggers) {
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
    std::unordered_map<ଅଚଳ AstVarScope*, Ast_Sen_ବୃକ୍ଷ*> inp2changedp;
    std::vector<Ast_Sen_ବୃକ୍ଷ*> icoChangeSenTreeps;
    ଯଦି (v3Global.opt.fIcoChangeDetect().isTrue()) {
        File_ଧାଡ଼ି* ଅଚଳ flp = netlistp->fileline();
        AstScope* ଅଚଳ scopep = netlistp->ଶୀର୍ଷ_Scopep()->scopep();
        ପାଇଁ (AstVarScope* vscp = scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            // Only for top level ports, assume outputs don't change externally
            ଯଦି (!vscp->varp()->isPrimaryInish()) ଜାରି;
            // Don't do if written by the design - wouldn't update the change detect 'prev' value
            ଯଦି (vscp->varp()->icoMaybeWritten()) ଜାରି;
            // Don't do if forceable, as we can't see the actual value - this is belt and braces
            ଯଦି (vscp->varp()->isForced()) ଜାରି;
            // Can't handle unpacked arrays (they have special types when primary input)
            ଯଦି (VN_IS(vscp->dtypep()->skipRefp(), UnpackArrayDType)) ଜାରି;
            // Similarly to arrays, can't handle SystemC types
            ଯଦି (vscp->varp()->isSc()) ଜାରି;
            // Create a sen tree triggered when this input changes
            Ast_Sen_ବୃକ୍ଷ*& senTreepr = inp2changedp[vscp];
            UASSERT_OBJ(!senTreepr, vscp, "Duplicate input change detect trigger");
            AstVarRef* ଅଚଳ refp = ନୂତନ AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* ଅଚଳ senItemp = ନୂତନ AstSenItem{flp, VEdgeType::ET_CHANGED, refp};
            senTreepr = ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, senItemp};
            icoChangeSenTreeps.ଠେଲ_back(senTreepr);
        }
    }
    V3Stats::addStat("Scheduling, 'ico' change detect triggers", icoChangeSenTreeps.size());

    // Gather the relevant sensitivity expressions and create the trigger kit
    std::vector<ଅଚଳ Ast_Sen_ବୃକ୍ଷ*> senTreeps = getSenTreesUsedBy({&logic});
    senTreeps.ଭର୍ତ୍ତି(senTreeps.end(), icoChangeSenTreeps.begin(), icoChangeSenTreeps.end());
    ଅଚଳ TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_ପଦସମଷ୍ଟି_Builder, {},
                                                  senTreeps, "ico", extraTriggers, ମିଥ୍ୟା, ମିଥ୍ୟା);
    std::ignore = sen_ପଦସମଷ୍ଟି_Builder.getAndClearResults();

    ଯଦି (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, initFuncp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);

    // Add trigger indices
    addTriggerIndices(logic, trigKit, ମିଥ୍ୟା);

    // The 'first iteration' trigger for top level inputs
    ଅଚଳ uint32_t firstIterTrigger = trigKit.extraTriggerIndex(firstIterationTrigger);

    ଅଚଳ auto& vifVscpToTrigIco
        = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);

    // Create and Order the body function
    AstCFunc* ଅଚଳ icoFuncp = V3Order::order(
        {&logic}, trigKit, trigKit.vscp(), cgRefBindings, "ico", ମିଥ୍ୟା, ମିଥ୍ୟା,
        [&](ଅଚଳ AstVarScope* vscp, std::set<uint32_t>& triggers) {
            AstVar* ଅଚଳ varp = vscp->varp();
            // If it has an explicit change detect trigger, use that,
            // otherwise fall back to using the 'first iteration' trigger
            auto it = inp2changedp.find(vscp);
            ଯଦି (it != inp2changedp.end()) {
                triggers.ଭର୍ତ୍ତି(trigKit.senItem2TrigIdx(it->second->sensesp()));
            } ଅନ୍ୟଥା ଯଦି (varp->isPrimaryInish() || varp->isSigUserRWPublic() || varp->sampled()) {
                triggers.ଭର୍ତ୍ତି(firstIterTrigger);
            }
            // Add other triggers
            ଯଦି (varp->isWrittenByDpi())
                triggers.ଭର୍ତ୍ତି(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
            ଯଦି (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
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
            Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ stmtp = trigKit.newCompBaseCall();
            ଯଦି (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), ସତ୍ୟ));
            ଫେରନ୍ତୁ stmtp;
        }(),
        // Work statements: Invoke the 'ico' function
        util::callVoidFunc(icoFuncp));

    // Release temporary input change detect SenTrees
    ପାଇଁ (Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep : icoChangeSenTreeps)
        VL_DO_DANGLING(senTreep->delete_ବୃକ୍ଷ(), senTreep);
    icoChangeSenTreeps.clear();
}

//============================================================================
// EvalKit groups items that have to be passed to createEval() for a given eval region

ସଂରଚନା EvalKit final {
    // The AstVarScope representing the region's trigger vector
    AstVarScope* ଅଚଳ m_vscp = nullptr;
    // The AstCFunc that evaluates the region's logic
    AstCFunc* ଅଚଳ m_funcp = nullptr;
    // Is this kit used/required?
    ତାର୍କିକ empty() ଅଚଳ { ଫେରନ୍ତୁ !m_funcp; }
};

//============================================================================
// Create the evaluation function of each region of a time step

ଶୂନ୍ୟ createEval(AstNetlist* netlistp,  //
                ଅଚଳ TriggerKit& trigKit,  //
                ଅଚଳ EvalKit& actKit,  //
                ଅଚଳ EvalKit& nbaKit,  //
                ଅଚଳ EvalKit& obsKit,  //
                ଅଚଳ EvalKit& reactKit,  //
                TimingKit& timingKit  //
) {
    File_ଧାଡ଼ି* ଅଚଳ flp = netlistp->fileline();

    // Grab the delay scheduler variable, if any
    AstVarScope* ଅଚଳ delaySchedVscp = timingKit.getDelayScheduler(netlistp);

    // 'createResume' consumes the contents that 'createReady' needs, so do the right order
    AstCCall* ଅଚଳ timingReadyp = timingKit.createReady(trigKit.vscp());
    AstCCall* ଅଚଳ timingResumep = timingKit.createResume(trigKit.vscp());

    // Create the 'act' region
    createEvalRegion(  //
        netlistp, VEval::ACT, 0, trigKit,
        // Use trigger
        actKit.m_vscp, nullptr,
        // Prep statements
        [&]() {
            // Compute the current 'act' triggers - the NBA triggers are the latched value
            Ast_ଗଣ୍ଠି_Stmt* stmtsp = trigKit.newCompBaseCall();
            Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ dumpp
                = stmtsp ? trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), ସତ୍ୟ) : nullptr;
            // Mark as ready for triggered awaits
            ଯଦି (timingReadyp) stmtsp = Ast_ଗଣ୍ଠି::addNext(stmtsp, timingReadyp->makeStmt());
            ଯଦି (AstVarScope* ଅଚଳ vscAccp = trigKit.vscAccp()) {
                stmtsp = Ast_ଗଣ୍ଠି::addNext(stmtsp, trigKit.newOrIntoCall(actKit.m_vscp, vscAccp));
            }
            stmtsp = Ast_ଗଣ୍ଠି::addNext(stmtsp, trigKit.newCompExtCall(nbaKit.m_vscp));
            stmtsp = Ast_ଗଣ୍ଠି::addNext(stmtsp, dumpp);
            // Latch the 'act' triggers under the 'nba' triggers
            stmtsp = Ast_ଗଣ୍ଠି::addNext(stmtsp, trigKit.newOrIntoCall(nbaKit.m_vscp, actKit.m_vscp));
            //
            ଫେରନ୍ତୁ stmtsp;
        }(),
        // Work statements
        [&]() {
            Ast_ଗଣ୍ଠି_Stmt* workp = nullptr;
            ଯଦି (AstVarScope* ଅଚଳ actAccp = trigKit.vscAccp()) {
                AstCMethodHard* ଅଚଳ cCallp = ନୂତନ AstCMethodHard{
                    flp, ନୂତନ AstVarRef{flp, actAccp, VAccess::WRITE}, VCMethod::UNPACKED_FILL,
                    ନୂତନ AstConst{flp, AstConst::Unsized64{}, 0}};
                cCallp->dtypeSetVoid();
                workp = Ast_ଗଣ୍ଠି::addNext(workp, cCallp->makeStmt());
            }
            // Resume triggered timing schedulers
            ଯଦି (timingResumep) workp = Ast_ଗଣ୍ଠି::addNext(workp, timingResumep->makeStmt());
            // Invoke the 'act' function
            workp = Ast_ଗଣ୍ଠି::addNext(workp, util::callVoidFunc(actKit.m_funcp));
            //
            ଫେରନ୍ତୁ workp;
        }());

    // Create the 'inact' region
    createEvalRegion(  //
        netlistp, VEval::INACT, 0, trigKit,
        // Use explicit condition
        nullptr,
        [&]() -> Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* {
            ଯଦି (!delaySchedVscp) ଫେରନ୍ତୁ nullptr;
            // Run if any zero delays are pending
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ callp
                = ନୂତନ AstCMethodHard{flp, ନୂତନ AstVarRef{flp, delaySchedVscp, VAccess::READ},
                                     VCMethod::SCHED_AWAITING_ZERO_DELAY};
            callp->dtypeSetBit();
            ଫେରନ୍ତୁ callp;
        }(),
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_ଗଣ୍ଠି_Stmt* {
            ଯଦି (!delaySchedVscp) {
                // Nothing to do if there are no delays at all in the design
                ଫେରନ୍ତୁ nullptr;
            } ଅନ୍ୟଥା ଯଦି (v3Global.usesZeroDelay()) {
                // Resume processes watiting for #0 delay
                AstCMethodHard* ଅଚଳ callp = ନୂତନ AstCMethodHard{
                    flp, ନୂତନ AstVarRef{flp, delaySchedVscp, VAccess::READWRITE},
                    VCMethod::SCHED_RESUME_ZERO_DELAY};
                callp->dtypeSetVoid();
                ଫେରନ୍ତୁ callp->makeStmt();
            } ଅନ୍ୟଥା {
                // Assumption was that the design doesn't use #0 delays.
                // Die at run-time if it does.
                AstCStmt* ଅଚଳ stmtp = ନୂତନ AstCStmt{flp};
                ଅଚଳ File_ଧାଡ଼ି* ଅଚଳ locp = netlistp->ଶୀର୍ଷ_Modulep()->fileline();
                ଅଚଳ std::string& file = VIdProtect::protect(locp->filename());
                ଅଚଳ std::string& ଧାଡ଼ି = std::to_string(locp->lineno());
                stmtp->add("VL_FATAL_MT(\"" + V3OutFormatter::quoteNameControls(file) + "\", "
                           + ଧାଡ଼ି
                           + ", \"\", \"ZERODLY: Design Verilated with '--no-sched-zero-delay', "
                           + "but #0 delay executed at runtime\");");
                ଫେରନ୍ତୁ stmtp;
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
            Ast_ଗଣ୍ଠି_Stmt* workp = nullptr;
            // Latch the 'nba' trigger flags under the following region's trigger flags
            ଯଦି (!obsKit.empty()) {
                workp = trigKit.newOrIntoCall(obsKit.m_vscp, nbaKit.m_vscp);
            } ଅନ୍ୟଥା ଯଦି (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, nbaKit.m_vscp);
            }
            // Invoke the 'nba' function
            workp = Ast_ଗଣ୍ଠି::addNext(workp, util::callVoidFunc(nbaKit.m_funcp));
            // Clear the 'nba' triggers
            workp = Ast_ଗଣ୍ଠି::addNext(workp, trigKit.newClearCall(nbaKit.m_vscp));
            //
            ଫେରନ୍ତୁ workp;
        }(),
        // Extra work (not conditional on having had a fired trigger)
        [&](AstVarScope* continuep) -> Ast_ଗଣ୍ଠି_Stmt* {
            // Check if any dynamic NBAs are pending, if there are any in the design
            ଯଦି (!netlistp->nbaEventp()) ଫେରନ୍ତୁ nullptr;
            AstVarScope* ଅଚଳ nbaEventp = netlistp->nbaEventp();
            AstVarScope* ଅଚଳ nbaEventTriggerp = netlistp->nbaEventTriggerp();
            UASSERT(nbaEventTriggerp, "NBA event trigger var should exist");
            netlistp->nbaEventp(nullptr);
            netlistp->nbaEventTriggerp(nullptr);

            // If a dynamic NBA is pending, clear the pending flag and fire the ready event
            AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, ନୂତନ AstVarRef{flp, nbaEventTriggerp, VAccess::READ}};
            ifp->addThensp(util::setVar(continuep, 1));
            ifp->addThensp(util::setVar(nbaEventTriggerp, 0));
            AstCMethodHard* ଅଚଳ firep = ନୂତନ AstCMethodHard{
                flp, ନୂତନ AstVarRef{flp, nbaEventp, VAccess::WRITE}, VCMethod::EVENT_FIRE};
            firep->dtypeSetVoid();
            ifp->addThensp(firep->makeStmt());
            ଫେରନ୍ତୁ ifp;
        });

    // Create the 'obs' region
    createEvalRegion(  //
        netlistp, VEval::OBS, 0, trigKit,
        // Use trigger
        obsKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_ଗଣ୍ଠି_Stmt* {
            ଯଦି (obsKit.empty()) ଫେରନ୍ତୁ nullptr;
            Ast_ଗଣ୍ଠି_Stmt* workp = nullptr;
            // Latch the Observed trigger flags under the Reactive trigger flags
            ଯଦି (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, obsKit.m_vscp);
            }
            // Invoke the 'obs' function
            workp = Ast_ଗଣ୍ଠି::addNext(workp, util::callVoidFunc(obsKit.m_funcp));
            // Clear the 'obs' triggers
            workp = Ast_ଗଣ୍ଠି::addNext(workp, trigKit.newClearCall(obsKit.m_vscp));
            //
            ଫେରନ୍ତୁ workp;
        }());

    // Create the 'react' region
    createEvalRegion(  //
        netlistp, VEval::REACT, 0, trigKit,
        // Use trigger
        reactKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_ଗଣ୍ଠି_Stmt* {
            ଯଦି (reactKit.empty()) ଫେରନ୍ତୁ nullptr;
            // Invoke the 'react' function
            Ast_ଗଣ୍ଠି_Stmt* workp = util::callVoidFunc(reactKit.m_funcp);
            // Clear the 'react' triggers
            workp = Ast_ଗଣ୍ଠି::addNext(workp, trigKit.newClearCall(reactKit.m_vscp));
            ଫେରନ୍ତୁ workp;
        }());
}

}  // namespace

//============================================================================
// Helper that builds virtual interface trigger sentrees

VirtIfaceTriggers::VscpTrigMap VirtIfaceTriggers::makeVscpToTrigMap(ଅଚଳ TriggerKit& trigKit,
                                                                    uint32_t firstIndex) ଅଚଳ {
    VscpTrigMap map;
    uint32_t index = firstIndex;
    ପାଇଁ (ଅଚଳ auto& entry : m_triggers) {
        map.emplace(entry.m_vscp, trigKit.extraTriggerIndex(index));
        ++index;
    }
    ଫେରନ୍ତୁ map;
}

//============================================================================
// Top level entry-point to scheduling

ଶୂନ୍ୟ schedule(AstNetlist* netlistp) {
    ଅଚଳ auto addSizeStat = [](ଅଚଳ string& name, ଅଚଳ LogicByScope& lbs) {
        uint64_t size = 0;
        lbs.foreachLogic([&](Ast_ଗଣ୍ଠି* nodep) { size += nodep->ଗଣ୍ଠି_ଗଣନା(); });
        V3Stats::addStat("Scheduling, " + name, size);
    };

    // Step 2: Prepare external domains for timing and virtual interfaces
    // Create extra triggers for virtual interfaces
    ଅଚଳ auto& virtIfaceTriggers = makeVirtIfaceTriggers(netlistp);
    // Resolve what covergroup reference formal arguments are bound to, which is visible here
    // but not from V3Order, where the reads through them must be modeled
    ଅଚଳ CovergroupRefBindings& cgRefBindings = makeCovergroupRefBindings(netlistp);
    // Prepare timing-related logic and external domains
    TimingKit timingKit = prepareTiming(netlistp);

    // Step 3: Gather and classify all logic in the design
    LogicClasses logicClasses = gatherLogicClasses(netlistp);

    ଯଦି (v3Global.opt.stats()) {
        V3Stats::statsStage("sched-gather");
        addSizeStat("size of class: static", logicClasses.m_static);
        addSizeStat("size of class: initial", logicClasses.m_initial);
        addSizeStat("size of class: final", logicClasses.m_final);
    }

    // Step 4: Schedule static, initial and final logic classes in source order
    AstCFunc* ଅଚଳ staticp = createStatic(netlistp, logicClasses);
    ଯଦି (v3Global.opt.stats()) V3Stats::statsStage("sched-static");

    createInitial(netlistp, logicClasses);
    ଯଦି (v3Global.opt.stats()) V3Stats::statsStage("sched-initial");

    createFinal(netlistp, logicClasses);
    ଯଦି (v3Global.opt.stats()) V3Stats::statsStage("sched-final");

    // Step 5: Break combinational cycles by introducing hybrid logic
    // Note: breakCycles also removes corresponding logic from logicClasses.m_comb;
    logicClasses.m_hybrid = breakCycles(netlistp, logicClasses.m_comb);
    ଯଦି (v3Global.opt.stats()) {
        addSizeStat("size of class: clocked", logicClasses.m_clocked);
        addSizeStat("size of class: combinational", logicClasses.m_comb);
        addSizeStat("size of class: hybrid", logicClasses.m_hybrid);
        V3Stats::statsStage("sched-break-cycles");
    }

    // We pass around a single SenExprBuilder instance, as we only need one set of 'prev' variables
    // for edge/change detection in sensitivity expressions, which this keeps track of.
    Ast_ଶୀର୍ଷ_Scope* ଅଚଳ ଶୀର୍ଷ_Scopep = netlistp->ଶୀର୍ଷ_Scopep();
    AstScope* ଅଚଳ scopeTopp = ଶୀର୍ଷ_Scopep->scopep();
    Sen_ପଦସମଷ୍ଟି_Builder sen_ପଦସମଷ୍ଟି_Builder{scopeTopp};

    // Step 6: Create 'settle' region that restores the combinational invariant
    createSettle(netlistp, staticp, sen_ପଦସମଷ୍ଟି_Builder, logicClasses, cgRefBindings);
    ଯଦି (v3Global.opt.stats()) V3Stats::statsStage("sched-settle");

    // Step 7: Partition the clocked and combinational (including hybrid) logic into pre/act/nba.
    // All clocks (signals referenced in an AstSenTree) generated via a blocking assignment
    // (including combinationally generated signals) are computed within the act region.
    LogicRegions logicRegions
        = partition(logicClasses.m_clocked, logicClasses.m_comb, logicClasses.m_hybrid);
    logicRegions.m_obs = logicClasses.m_observed;
    logicRegions.m_react = logicClasses.m_reactive;
    ଯଦି (v3Global.opt.stats()) {
        addSizeStat("size of region: Active Pre", logicRegions.m_pre);
        addSizeStat("size of region: Active", logicRegions.m_act);
        addSizeStat("size of region: NBA", logicRegions.m_nba);
        addSizeStat("size of region: Observed", logicRegions.m_obs);
        addSizeStat("size of region: Reactive", logicRegions.m_react);
        V3Stats::statsStage("sched-partition");
    }

    // Step 8: Replicate combinational logic
    LogicReplicas logicReplicas = replicateLogic(logicRegions);
    ଯଦି (v3Global.opt.stats()) {
        addSizeStat("size of replicated logic: Input", logicReplicas.m_ico);
        addSizeStat("size of replicated logic: Active", logicReplicas.m_act);
        addSizeStat("size of replicated logic: NBA", logicReplicas.m_nba);
        addSizeStat("size of replicated logic: Observed", logicReplicas.m_obs);
        addSizeStat("size of replicated logic: Reactive", logicReplicas.m_react);
        V3Stats::statsStage("sched-replicate");
    }

    // Step 9: Create the input combinational logic
    createIcoRegion(netlistp, staticp, sen_ପଦସମଷ୍ଟି_Builder, logicReplicas.m_ico, virtIfaceTriggers,
                    cgRefBindings);
    ଯଦି (v3Global.opt.stats()) V3Stats::statsStage("sched-create-ico");

    // Step 10: Create the triggers
    AstVarScope* ଅଚଳ dpiExportTriggerVscp = netlistp->dpiExportTriggerp();
    netlistp->dpiExportTriggerp(nullptr);  // Finished with this here

    // We may have an extra trigger for variable updated in DPI exports
    TriggerKit::ExtraTriggers extraTriggers;
    ଅଚଳ uint32_t dpiExportTriggerIndex = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    ଅଚଳ uint32_t firstVifTriggerIndex = extraTriggers.size();
    ପାଇଁ (ଅଚଳ auto& entry : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + entry.m_ifacep->name() + "."
                               + entry.m_memberp->name());
    }

    ଅଚଳ auto& preTreeps = getSenTreesUsedBy({&logicRegions.m_pre});
    ଅଚଳ auto& senTreeps = getSenTreesUsedBy({&logicRegions.m_act,  //
                                               &logicRegions.m_nba,  //
                                               &logicRegions.m_obs,  //
                                               &logicRegions.m_react,  //
                                               &timingKit.m_lbs});
    ଅଚଳ TriggerKit trigKit
        = TriggerKit::create(netlistp, staticp, sen_ପଦସମଷ୍ଟି_Builder, preTreeps, senTreeps, "act",
                             extraTriggers, ମିଥ୍ୟା, v3Global.usesTiming());

    // Add post updates from the timing kit
    ଯଦି (timingKit.m_postUpdates) trigKit.compBasep()->addStmtsp(timingKit.m_postUpdates);

    ଯଦି (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, staticp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);
    ଯଦି (v3Global.opt.stats()) V3Stats::statsStage("sched-create-triggers");

    // Note: Experiments so far show that running the Act (or Ico) regions on
    // multiple threads is always a net loss, so only use multi-threading for
    // NBA for now. This can be revised if evidence is available that it would
    // be beneficial

    // Step 11: Create the 'act' region evaluation function

    // Add the trigger indices of the input logic
    addTriggerIndices(logicRegions.m_pre, trigKit, ସତ୍ୟ);
    addTriggerIndices(logicRegions.m_act, trigKit, ମିଥ୍ୟା);
    addTriggerIndices(logicReplicas.m_act, trigKit, ମିଥ୍ୟା);
    addTriggerIndices(timingKit.m_lbs, trigKit, ମିଥ୍ୟା);

    // External triggers of variables. Trigger indices are the same in all regions, so share.
    ଅଚଳ std::map<ଅଚଳ AstVarScope*, std::vector<uint32_t>> timingTriggers
        = timingKit.externalTriggerIndices(trigKit);
    ଅଚଳ auto& vifVscpToTrig = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);
    ଅଚଳ V3Order::ExternalTriggersProvider externalTriggers
        = [&](ଅଚଳ AstVarScope* vscp, std::set<uint32_t>& triggers) {
              auto it = timingTriggers.find(vscp);
              ଯଦି (it != timingTriggers.end())
                  triggers.ଭର୍ତ୍ତି(it->second.begin(), it->second.end());
              ଯଦି (vscp->varp()->isWrittenByDpi()) {
                  triggers.ଭର୍ତ୍ତି(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
              }
              ଯଦି (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                  addTriggeredIface(vscp, vifVscpToTrig, virtIfaceTriggers, triggers);
              }
          };

    AstCFunc* ଅଚଳ actFuncp
        = V3Order::order({&logicRegions.m_pre, &logicRegions.m_act, &logicReplicas.m_act}, trigKit,
                         trigKit.vscp(), cgRefBindings, "act", ମିଥ୍ୟା, ମିଥ୍ୟା, externalTriggers);
    util::splitCheck(actFuncp);
    ଯଦି (v3Global.opt.stats()) V3Stats::statsStage("sched-create-act");

    ଅଚଳ EvalKit actKit{trigKit.vscp(), actFuncp};

    // Orders a region's logic and creates the region eval function
    ଅଚଳ auto order = [&](ଅଚଳ std::string& name,
                           ଅଚଳ std::vector<V3Sched::LogicByScope*>& logic) -> EvalKit {
        UINFO(2, "Scheduling " << name << " #logic = " << logic.size());
        AstVarScope* ଅଚଳ trigVscp = trigKit.newTrigVec(name);
        // Add the trigger indices of the input logic
        ପାଇଁ (LogicByScope* lbs : logic) addTriggerIndices(*lbs, trigKit, ମିଥ୍ୟା);

        AstCFunc* ଅଚଳ funcp
            = V3Order::order(logic, trigKit, trigVscp, cgRefBindings, name,
                             name == "nba" && v3Global.opt.mtasks(), ମିଥ୍ୟା, externalTriggers);

        ଫେରନ୍ତୁ {trigVscp, funcp};
    };

    // Step 12: Create the 'nba' region evaluation function
    ଅଚଳ EvalKit nbaKit = order("nba", {&logicRegions.m_nba, &logicReplicas.m_nba});
    util::splitCheck(nbaKit.m_funcp);
    ଯଦି (v3Global.opt.stats()) V3Stats::statsStage("sched-create-nba");

    // Orders a region's logic and creates the region eval function (only if there is any logic in
    // the region)
    ଅଚଳ auto orderIfNonEmpty
        = [&](ଅଚଳ std::string& name, ଅଚଳ std::vector<LogicByScope*>& logic) -> EvalKit {
        ଯଦି (logic[0]->empty())
            ଫେରନ୍ତୁ {};  // if region is empty, replica is supposed to be empty as well
        ଅଚଳ auto& kit = order(name, logic);
        ଯଦି (v3Global.opt.stats()) V3Stats::statsStage("sched-create-" + name);
        ଫେରନ୍ତୁ kit;
    };

    // Step 13: Create the 'obs' region evaluation function
    ଅଚଳ EvalKit obsKit = orderIfNonEmpty("obs", {&logicRegions.m_obs, &logicReplicas.m_obs});

    // Step 14: Create the 'react' region evaluation function
    ଅଚଳ EvalKit reactKit
        = orderIfNonEmpty("react", {&logicRegions.m_react, &logicReplicas.m_react});

    // Step 15: Create the 'postponed' region evaluation function
    createPostponed(netlistp, logicClasses);

    // Step 16: Populate the eval entry point function of each region of a time step
    createEval(netlistp, trigKit, actKit, nbaKit, obsKit, reactKit, timingKit);

    // Step 17: Add neccessary evaluation before awaits
    ଯଦି (AstCCall* ଅଚଳ readyp = timingKit.createReady(trigKit.vscp())) {
        staticp->addStmtsp(readyp->makeStmt());
        beforeTrigVisitor(netlistp, sen_ପଦସମଷ୍ଟି_Builder, trigKit);
    } ଅନ୍ୟଥା {
        // beforeTrigVisitor clears Sentree pointers in AstCAwaits (as these sentrees will get
        // deleted later) if there was no need to call it, SenTrees have to be cleaned manually
        netlistp->foreach([](AstCAwait* ଅଚଳ cAwaitp) { cAwaitp->clearSentreep(); });
    }
    ଯଦି (AstVarScope* ଅଚଳ trigAccp = trigKit.vscAccp()) {
        // Copy trigger vector to accumulator at the end of static initialziation so,
        // triggers fired during initialization persist to the first resume.
        ଅଚଳ AstUnpackArrayDType* ଅଚଳ trigAccDTypep
            = VN_AS(trigAccp->dtypep(), UnpackArrayDType);
        UASSERT_OBJ(
            trigAccDTypep->ଡାହାଣ() == 0, trigAccp,
            "Expected that trigger vector and accumulator start elements enumeration from 0");
        UASSERT_OBJ(trigAccDTypep->ବାମ() >= 0, trigAccp,
                    "Expected that trigger vector and accumulator has no negative indexes");
        File_ଧାଡ଼ି* ଅଚଳ flp = trigAccp->fileline();
        AstVarScope* ଅଚଳ vscp = netlistp->ଶୀର୍ଷ_Scopep()->createTemp("__Vi", 32);
        AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{flp};
        loopp->addStmtsp(
            ନୂତନ AstAssign{flp,
                          ନୂତନ AstArraySel{flp, ନୂତନ AstVarRef{flp, trigAccp, VAccess::WRITE},
                                          ନୂତନ AstVarRef{flp, vscp, VAccess::READ}},
                          ନୂତନ AstArraySel{flp, ନୂତନ AstVarRef{flp, actKit.m_vscp, VAccess::READ},
                                          ନୂତନ AstVarRef{flp, vscp, VAccess::READ}}});
        loopp->addStmtsp(util::incrementVar(vscp));
        loopp->addStmtsp(ନୂତନ AstLoopTest{
            flp, loopp,
            ନୂତନ AstLte{flp, ନୂତନ AstVarRef{flp, vscp, VAccess::READ},
                       ନୂତନ AstConst{flp, AstConst::Widthed_ମୂଲ୍ୟ{}, 32,
                                    static_cast<uint32_t>(trigAccDTypep->ବାମ())}}});
        staticp->addStmtsp(loopp);
    }

    // Step 18: Clean up
    netlistp->clearStlFirstIterationp();

    ଯଦି (v3Global.opt.stats()) {
        // A sample() call resolved to the union over its covergroup's constructions reads more
        // than it can, which orders it against more logic than necessary
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per instance",
                         cgRefBindings.ଅଙ୍କ_Exact_Calls());
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per type",
                         cgRefBindings.ଅଙ୍କ_Union_Calls());
    }

    // Haven't split static initializer yet
    util::splitCheck(staticp);

    // Record eval stats
    netlistp->addEvalStats("sched");

    // Dump
    V3Global::dump_Check_Global_ବୃକ୍ଷ("sched", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}

}  // namespace V3Sched
