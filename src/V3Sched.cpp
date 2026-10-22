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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Sched.h"

#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"
#ಅನ್ತರ್ಭಾವ "V3EmitV.h"
#ಅನ್ತರ್ಭಾವ "V3Order.h"
#ಅನ್ತರ್ಭಾವ "V3SenExprBuilder.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ನಾಮಾಕಾಶ V3Sched {

ನಾಮಾಕಾಶ {

//============================================================================
// Utility functions

std::vector<ಧ್ರುವ Ast_Sen_ವೃಕ್ಷ*> getSenTreesUsedBy(ಧ್ರುವ std::vector<ಧ್ರುವ LogicByScope*>& lbsps) {
    ಧ್ರುವ VNUser1InUse user1InUse;
    std::vector<ಧ್ರುವ Ast_Sen_ವೃಕ್ಷ*> result;
    ಪ್ರತಿ (ಧ್ರುವ LogicByScope* ಧ್ರುವ lbsp : lbsps) {
        ಪ್ರತಿ (ಧ್ರುವ auto& entry : *lbsp) {
            AstActive* ಧ್ರುವ activep = entry.m_activep;
            Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep = activep->sentreep();
            ಯದಿ (senTreep->user1SetOnce()) ಅನುವರ್ತ;
            ಯದಿ (senTreep->hasClocked() || senTreep->hasHybrid()) result.ತಳ್ಳು_back(senTreep);
        }
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

ಶೂನ್ಯ addTriggerIndices(LogicByScope& lbs, ಧ್ರುವ TriggerKit& trigKit, ತಾರ್ಕಿಕ pre) {
    ಪ್ರತಿ (LogicByScope::Entry& entry : lbs) {
        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep = entry.m_activep->sentreep();
        ಯದಿ (senTreep->hasCombo()) ಅನುವರ್ತ;
        entry.m_triggers = trigKit.triggerIndices(senTreep, pre);
    }
}

// Add the VIF triggers that a given VarScope should be sensitive to.
// Case 2 (non-virtual interface read): sensitive to that specific VarScope's trigger only
// Case 3 (virtual interface read): sensitive to all member triggers of the same interface type
ಶೂನ್ಯ addTriggeredIface(ಧ್ರುವ AstVarScope* vscp, ಧ್ರುವ VirtIfaceTriggers::VscpTrigMap& vscpToTrig,
                       ಧ್ರುವ VirtIfaceTriggers& virtIfaceTriggers, std::set<uint32_t>& triggers) {
    ಯದಿ (vscp->varp()->isVirtIface()) {
        // Virtual interface variable -- sensitive to all member triggers of this interface type
        ಧ್ರುವ AstIface* ಧ್ರುವ ifacep = VN_AS(vscp->varp()->dtypep(), IfaceRefDType)->ifacep();
        ಪ್ರತಿ (ಧ್ರುವ auto& entry : virtIfaceTriggers.m_triggers) {
            ಯದಿ (entry.m_ifacep == ifacep) {
                ಧ್ರುವ auto it = vscpToTrig.find(entry.m_vscp);
                ಯದಿ (it != vscpToTrig.end()) triggers.ಸೇರಿಸು(it->second);
            }
        }
    } ಅನ್ಯಥಾ {
        // Non-virtual interface member -- sensitive to this VarScope's trigger only
        ಧ್ರುವ auto it = vscpToTrig.find(vscp);
        ಯದಿ (it != vscpToTrig.end()) triggers.ಸೇರಿಸು(it->second);
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
ಶೂನ್ಯ createEvalRegion(
    AstNetlist* netlistp,  //
    VEval eval,  // The entry point of the current region
    // Index of the region's 'first iteration' extra trigger, if it has one, otherwise ignored
    uint32_t firstIterTrigger,
    ಧ್ರುವ TriggerKit& trigKit,  // The trigger kit
    AstVarScope* trigp,  // The trigger vector - may be nullptr if no triggers or using 'condp'
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp,  // Explicit condition that must be true to run 'phaseWorkp'
    Ast_ಗಂಟು_Stmt* phasePrepp,  // Prep statements run before checking triggers
    Ast_ಗಂಟು_Stmt* phaseWorkp,  // The work to do if anything triggered
    // Extra statements to run after the work, even if no triggers fired. This function is
    // passed a variable, which must be set to true if we must continue and loop again,
    // and must be unmodified otherwise.
    std::function<Ast_ಗಂಟು_Stmt*(AstVarScope*)> phaseExtra = [](AstVarScope*) { ಪ್ರತ್ಯಾಗಮ nullptr; }  //
) {
    UASSERT(!trigp || !condp, "Cannot use both 'trigp' and 'condp' in 'createEvalRegion'");
    UASSERT(!eval.firstIteration() || trigp,
            "Region without triggers cannot need a first iteration flag");

    // All work is under a trigger or condition, so with neither the region has
    // nothing to evaluate, and what we create below reduces to a no-op function.

    ಧ್ರುವ std::string tag = eval.tag();
    Ast_ತುದಿ_Scope* ಧ್ರುವ ತುದಿ_Scopep = netlistp->ತುದಿ_Scopep();
    File_ಸಾಲು* ಧ್ರುವ flp = netlistp->fileline();

    // Populate the trigger dump entry point function
    ಯದಿ (trigp) {
        netlistp->dumpTriggersFuncp(eval)->addStmtsp(trigKit.newDumpCall(trigp, tag, ಅಸತ್ಯ));
    }

    AstCFunc* ಧ್ರುವ funcp = netlistp->evalFuncp(eval);

    // A flag is passed from the run-time eval loop if this is the first iteration of the
    // current loop
    ಯದಿ (eval.firstIteration()) {
        AstVarScope* ಧ್ರುವ firstIterArgp = util::newArgument(funcp, netlistp->findBitDType(),
                                                             "firstIteration", VDirection::ಒಳಹರಿವು);

        // Set the region's 'first iteration' trigger straight from the argument
        funcp->addStmtsp(trigKit.newExtraTriggerAssignment(firstIterArgp, firstIterTrigger));

        // Only 'stl' also needs a module level flag, for design logic that reads
        // it directly (see V3Timing). Those reads can be anywhere in the design,
        // hence module level. Always created, even if nothing reads it.
        // TODO: get rid of this special case
        ಯದಿ (eval == VEval::STL) {
            AstVarScope* ಧ್ರುವ firstIterp = netlistp->stlFirstIterationp();
            firstIterp->varp()->noReset(ಸತ್ಯ);
            firstIterp->varp()->isInternal(ಸತ್ಯ);
            funcp->addStmtsp(ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, firstIterp, VAccess::WRITE},
                                           ನೂತನ AstVarRef{flp, firstIterArgp, VAccess::READ}});
        }
    }

    {
        // Add the preparatory statements
        funcp->addStmtsp(phasePrepp);

        // The execute flag
        AstVarScope* ಧ್ರುವ executeFlagp = ತುದಿ_Scopep->createTemp("__V" + tag + "Execute", 1);
        executeFlagp->varp()->noReset(ಸತ್ಯ);

        // If there is work in this region, execute it if any triggers fired
        ಯದಿ (phaseWorkp) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = ನೂತನ AstVarRef{flp, executeFlagp, VAccess::WRITE};
            // If using explicit condition, that directly determines whether to execute,
            // otherwise check if any triggers are fired
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = condp ? condp : trigKit.newAnySetCall(trigp);
            funcp->addStmtsp(ನೂತನ AstAssign{flp, lhsp, rhsp});

            // Add the work
            AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{flp, ನೂತನ AstVarRef{flp, executeFlagp, VAccess::READ}};
            ifp->addThensp(phaseWorkp);
            funcp->addStmtsp(ifp);
        }

        // Construct the extra statements
        Ast_ಗಂಟು_Stmt* ಧ್ರುವ extraWorkp = phaseExtra(executeFlagp);
        ಯದಿ (extraWorkp) funcp->addStmtsp(extraWorkp);

        // The function returns true iff it did run work
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ retp
            = phaseWorkp || extraWorkp
                  ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstVarRef{flp, executeFlagp, VAccess::READ})
                  : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstConst{flp, AstConst::BitFalse{}});
        funcp->addStmtsp(ನೂತನ AstCReturn{flp, retp});
    }
}

//============================================================================
// Collect and classify all logic in the design

LogicClasses gatherLogicClasses(AstNetlist* netlistp) {
    LogicClasses result;

    netlistp->foreach([&](AstScope* scopep) {
        scopep->foreach([&](AstActive* activep) {
            Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep = activep->sentreep();
            ಯದಿ (senTreep->hasStatic()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "static initializer with additional sensitivities");
                result.m_static.add(scopep, activep);
            } ಅನ್ಯಥಾ ಯದಿ (senTreep->hasInitial()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'initial' logic with additional sensitivities");
                result.m_initial.add(scopep, activep);
            } ಅನ್ಯಥಾ ಯದಿ (senTreep->hasFinal()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'final' logic with additional sensitivities");
                result.m_final.add(scopep, activep);
            } ಅನ್ಯಥಾ ಯದಿ (senTreep->hasCombo()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "combinational logic with additional sensitivities");
                ಯದಿ (VN_IS(activep->stmtsp(), AlwaysPostponed)) {
                    result.m_postponed.add(scopep, activep);
                } ಅನ್ಯಥಾ {
                    result.m_comb.add(scopep, activep);
                }
            } ಅನ್ಯಥಾ {
                UASSERT_OBJ(senTreep->hasClocked(), activep, "What else could it be?");
                ಯದಿ (VN_IS(activep->stmtsp(), AlwaysObserved)) {
                    result.m_observed.add(scopep, activep);
                } ಅನ್ಯಥಾ ಯದಿ (VN_IS(activep->stmtsp(), AlwaysReactive)) {
                    result.m_reactive.add(scopep, activep);
                } ಅನ್ಯಥಾ {
                    result.m_clocked.add(scopep, activep);
                }
            }
        });
    });

    ಪ್ರತ್ಯಾಗಮ result;
}

//============================================================================
// Simple ordering in source order

ಶೂನ್ಯ orderSequentially(AstCFunc* funcp, ಧ್ರುವ LogicByScope& lbs) {
    // Create new subfunc for scope
    ಧ್ರುವ auto createNewSubFuncp = [&](AstScope* ಧ್ರುವ scopep) {
        ಧ್ರುವ string subName{funcp->name() + "__" + scopep->nameDotless()};
        AstCFunc* ಧ್ರುವ subFuncp = ನೂತನ AstCFunc{scopep->fileline(), subName, scopep};
        subFuncp->isLoose(ಸತ್ಯ);
        subFuncp->isConst(ಅಸತ್ಯ);
        subFuncp->declPrivate(ಸತ್ಯ);
        subFuncp->slow(funcp->slow());
        scopep->addBlocksp(subFuncp);
        // Call it from the top function
        funcp->addStmtsp(util::callVoidFunc(subFuncp));
        ಪ್ರತ್ಯಾಗಮ subFuncp;
    };
    ಧ್ರುವ VNUser1InUse user1InUse;  // AstScope -> AstCFunc: the sub-function for the scope
    ಧ್ರುವ VNUser2InUse user2InUse;  // AstScope -> int: sub-function counter used for names
    ಪ್ರತಿ (ಧ್ರುವ auto& entry : lbs) {
        AstScope* ಧ್ರುವ scopep = entry.m_scopep;
        AstActive* ಧ್ರುವ activep = entry.m_activep;
        // Create a sub-function per scope so we can V3Combine them later
        ಯದಿ (!scopep->user1p()) scopep->user1p(createNewSubFuncp(scopep));
        // Add statements to sub-function
        ಪ್ರತಿ (Ast_ಗಂಟು *logicp = activep->stmtsp(), *nextp; logicp; logicp = nextp) {
            auto* subFuncp = VN_AS(scopep->user1p(), CFunc);
            nextp = logicp->nextp();
            ಯದಿ (Ast_ಗಂಟು_Procedure* ಧ್ರುವ procp = VN_CAST(logicp, ಗಂಟು_Procedure)) {
                ಯದಿ (Ast_ಗಂಟು* bodyp = procp->stmtsp()) {
                    bodyp->unlinkFrBackWithNext();
                    // If the process is suspendable, we need a separate function (a coroutine)
                    ಯದಿ (procp->isSuspendable()) {
                        funcp->slow(ಅಸತ್ಯ);
                        subFuncp = createNewSubFuncp(scopep);
                        subFuncp->name(subFuncp->name() + "__Vtiming__"
                                       + cvtToStr(scopep->user2Inc()));
                        subFuncp->rtnType("VlCoroutine");
                        ಯದಿ (VN_IS(procp, Always)) {
                            subFuncp->slow(ಅಸತ್ಯ);
                            File_ಸಾಲು* ಧ್ರುವ flp = procp->fileline();
                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = ನೂತನ AstCExpr{
                                flp, "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish())", 1};
                            AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{flp};
                            loopp->addStmtsp(ನೂತನ AstLoopTest{flp, loopp, condp});
                            loopp->addStmtsp(bodyp);
                            bodyp = loopp;
                        }
                    }
                    subFuncp->addStmtsp(bodyp);
                    ಯದಿ (procp->needProcess()) subFuncp->setNeedProcess();
                    // A coroutine holds a single process, so it is complete here
                    ಯದಿ (procp->isSuspendable()) util::splitCheck(subFuncp);
                }
            } ಅನ್ಯಥಾ {
                logicp->unlinkFrBack();
                subFuncp->addStmtsp(logicp);
            }
        }
        ಯದಿ (activep->backp()) activep->unlinkFrBack();
        VL_DO_DANGLING(activep->delete_ವೃಕ್ಷ(), activep);
    }
    // Split sub-functions once complete
    ಪ್ರತಿ (ಧ್ರುವ auto& entry : lbs) {
        AstScope* ಧ್ರುವ scopep = entry.m_scopep;
        ಯದಿ (AstCFunc* ಧ್ರುವ subFuncp = VN_AS(scopep->user1p(), CFunc)) {
            util::splitCheck(subFuncp);
            scopep->user1p(nullptr);
        }
    }
}

//============================================================================
// Create simply ordered functions

AstCFunc* createStatic(AstNetlist* netlistp, ಧ್ರುವ LogicClasses& logicClasses) {
    AstCFunc* ಧ್ರುವ funcp = netlistp->evalFuncp(VEval::STATIC);

    ಧ್ರುವ LogicByScope& orig = logicClasses.m_static;
    ಯದಿ (orig.size() <= 1) {
        orderSequentially(funcp, orig);
        ಪ್ರತ್ಯಾಗಮ funcp;
    }

    // Level-based module sorting can reorder packages so that an importing
    // package runs before the imported one.  Re-sort package entries by source
    // file position to restore compilation order (IEEE 1800-2023 26.3).
    std::vector<size_t> indices(orig.size());
    ಪ್ರತಿ (size_t i = 0; i < orig.size(); ++i) indices[i] = i;
    std::stable_sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ modA = orig[a].m_scopep->modp();
        ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ modB = orig[b].m_scopep->modp();
        ಧ್ರುವ ತಾರ್ಕಿಕ isPkgA = VN_IS(modA, Package);
        ಧ್ರುವ ತಾರ್ಕಿಕ isPkgB = VN_IS(modB, Package);
        ಯದಿ (isPkgA != isPkgB) ಪ್ರತ್ಯಾಗಮ isPkgA;  // Packages before non-packages
        ಯದಿ (isPkgA && isPkgB) {
            // Sort packages by source file position (compilation order)
            ಪ್ರತ್ಯಾಗಮ modA->fileline()->operatorCompare(*modB->fileline()) < 0;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Both non-package: preserve original order
    });
    LogicByScope sorted;
    ಪ್ರತಿ (ಧ್ರುವ size_t i : indices) sorted.add(orig[i].m_scopep, orig[i].m_activep);

    orderSequentially(funcp, sorted);
    ಪ್ರತ್ಯಾಗಮ funcp;  // Not splitting yet as it is not final
}

ಶೂನ್ಯ createInitial(AstNetlist* netlistp, ಧ್ರುವ LogicClasses& logicClasses) {
    AstCFunc* ಧ್ರುವ funcp = netlistp->evalFuncp(VEval::INITIAL);
    orderSequentially(funcp, logicClasses.m_initial);
    util::splitCheck(funcp);
}

ಶೂನ್ಯ createPostponed(AstNetlist* netlistp, ಧ್ರುವ LogicClasses& logicClasses) {
    AstCFunc* ಧ್ರುವ funcp = netlistp->evalFuncp(VEval::POSTPONED);
    orderSequentially(funcp, logicClasses.m_postponed);
    util::splitCheck(funcp);
}

ಶೂನ್ಯ createFinal(AstNetlist* netlistp, ಧ್ರುವ LogicClasses& logicClasses) {
    AstCFunc* ಧ್ರುವ funcp = netlistp->evalFuncp(VEval::FINAL);
    orderSequentially(funcp, logicClasses.m_final);
    util::splitCheck(funcp);
}

//============================================================================
// Helper that creates virtual interface value-change triggers

ಶೂನ್ಯ addVirtIfaceTriggerAssignments(AstNetlist* netlistp, AstCFunc* initFuncp,
                                    ಧ್ರುವ VirtIfaceTriggers& virtIfaceTriggers,
                                    uint32_t firstIndex, ಧ್ರುವ TriggerKit& trigKit) {
    uint32_t index = firstIndex;
    ಪ್ರತಿ (ಧ್ರುವ auto& entry : virtIfaceTriggers.m_triggers) {
        trigKit.add_ಮೌಲ್ಯ_Change_Trigger_Assignment(netlistp, initFuncp, entry.m_vscp, index);
        ++index;
    }
}

// Order the combinational logic to create the 'stl' region
ಶೂನ್ಯ createSettle(AstNetlist* netlistp, AstCFunc* ಧ್ರುವ initFuncp, Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder& sen_ಅಭಿವ್ಯಕ್ತಿ_Bulider,
                  LogicClasses& logicClasses, ಧ್ರುವ CovergroupRefBindings& cgRefBindings) {
    // Clone, because ordering is destructive, but we still need them for the other regions
    LogicByScope comb = logicClasses.m_comb.clone();
    LogicByScope hybrid = logicClasses.m_hybrid.clone();

    // We have an extra trigger denoting this is the first iteration of the settle loop
    TriggerKit::ExtraTriggers extraTriggers;
    ಧ್ರುವ uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");

    // Gather the relevant sensitivity expressions and create the trigger kit
    ಧ್ರುವ auto& senTreeps = getSenTreesUsedBy({&comb, &hybrid});
    ಧ್ರುವ TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_ಅಭಿವ್ಯಕ್ತಿ_Bulider, {},
                                                  senTreeps, "stl", extraTriggers, ಸತ್ಯ, ಅಸತ್ಯ);

    // Add trigger indices (comb has none, so only do the hybrid)
    addTriggerIndices(hybrid, trigKit, ಅಸತ್ಯ);

    // First trigger is for pure combinational triggers (first iteration)
    ಧ್ರುವ uint32_t ಒಳಹರಿವು_Changed = trigKit.extraTriggerIndex(firstIterationTrigger);

    // Create and the body function
    AstCFunc* ಧ್ರುವ stlFuncp = V3Order::order(
        {&comb, &hybrid}, trigKit, trigKit.vscp(), cgRefBindings, "stl", ಅಸತ್ಯ, ಸತ್ಯ,
        [=](ಧ್ರುವ AstVarScope*, std::set<uint32_t>& triggers) { triggers.ಸೇರಿಸು(ಒಳಹರಿವು_Changed); });
    util::splitCheck(stlFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::STL, firstIterationTrigger, trigKit,
        // Use trigger
        trigKit.vscp(), nullptr,
        // Explicit condition
        // Prep statements: Compute the current 'stl' triggers
        [&trigKit] {
            Ast_ಗಂಟು_Stmt* ಧ್ರುವ stmtp = trigKit.newCompBaseCall();
            ಯದಿ (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), ಸತ್ಯ));
            ಪ್ರತ್ಯಾಗಮ stmtp;
        }(),
        // Work statements: Invoke the 'stl' function
        util::callVoidFunc(stlFuncp));
}

//============================================================================
// Order the replicated combinational logic to create the 'ico' region

ಶೂನ್ಯ createIcoRegion(AstNetlist* netlistp, AstCFunc* ಧ್ರುವ initFuncp,
                     Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder& sen_ಅಭಿವ್ಯಕ್ತಿ_Builder, LogicByScope& logic,
                     ಧ್ರುವ VirtIfaceTriggers& virtIfaceTriggers,
                     ಧ್ರುವ CovergroupRefBindings& cgRefBindings) {
    // SystemC only: Any top level inputs feeding a combinational logic must be marked,
    // so we can make them sc_sensitive
    ಯದಿ (v3Global.opt.systemC()) {
        logic.foreachLogic([](Ast_ಗಂಟು* logicp) {
            logicp->foreach([](AstVarRef* refp) {
                ಯದಿ (refp->access().isWriteOnly()) ಪ್ರತ್ಯಾಗಮ;
                AstVarScope* ಧ್ರುವ vscp = refp->varScopep();
                ಯದಿ (vscp->scopep()->is_ತುದಿ() && vscp->varp()->isNonOutput()) {
                    vscp->varp()->scSensitive(ಸತ್ಯ);
                }
            });
        });
    }

    // We have some extra trigger denoting external conditions
    AstVarScope* ಧ್ರುವ dpiExportTriggerVscp = netlistp->dpiExportTriggerp();

    TriggerKit::ExtraTriggers extraTriggers;
    ಧ್ರುವ uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");
    ಧ್ರುವ uint32_t dpiExportTriggerIndex = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    ಧ್ರುವ uint32_t firstVifTriggerIndex = extraTriggers.size();
    ಪ್ರತಿ (ಧ್ರುವ auto& entry : virtIfaceTriggers.m_triggers) {
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
    std::unordered_map<ಧ್ರುವ AstVarScope*, Ast_Sen_ವೃಕ್ಷ*> inp2changedp;
    std::vector<Ast_Sen_ವೃಕ್ಷ*> icoChangeSenTreeps;
    ಯದಿ (v3Global.opt.fIcoChangeDetect().isTrue()) {
        File_ಸಾಲು* ಧ್ರುವ flp = netlistp->fileline();
        AstScope* ಧ್ರುವ scopep = netlistp->ತುದಿ_Scopep()->scopep();
        ಪ್ರತಿ (AstVarScope* vscp = scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            // Only for top level ports, assume outputs don't change externally
            ಯದಿ (!vscp->varp()->isPrimaryInish()) ಅನುವರ್ತ;
            // Don't do if written by the design - wouldn't update the change detect 'prev' value
            ಯದಿ (vscp->varp()->icoMaybeWritten()) ಅನುವರ್ತ;
            // Don't do if forceable, as we can't see the actual value - this is belt and braces
            ಯದಿ (vscp->varp()->isForced()) ಅನುವರ್ತ;
            // Can't handle unpacked arrays (they have special types when primary input)
            ಯದಿ (VN_IS(vscp->dtypep()->skipRefp(), UnpackArrayDType)) ಅನುವರ್ತ;
            // Similarly to arrays, can't handle SystemC types
            ಯದಿ (vscp->varp()->isSc()) ಅನುವರ್ತ;
            // Create a sen tree triggered when this input changes
            Ast_Sen_ವೃಕ್ಷ*& senTreepr = inp2changedp[vscp];
            UASSERT_OBJ(!senTreepr, vscp, "Duplicate input change detect trigger");
            AstVarRef* ಧ್ರುವ refp = ನೂತನ AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* ಧ್ರುವ senItemp = ನೂತನ AstSenItem{flp, VEdgeType::ET_CHANGED, refp};
            senTreepr = ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, senItemp};
            icoChangeSenTreeps.ತಳ್ಳು_back(senTreepr);
        }
    }
    V3Stats::addStat("Scheduling, 'ico' change detect triggers", icoChangeSenTreeps.size());

    // Gather the relevant sensitivity expressions and create the trigger kit
    std::vector<ಧ್ರುವ Ast_Sen_ವೃಕ್ಷ*> senTreeps = getSenTreesUsedBy({&logic});
    senTreeps.ಸೇರಿಸು(senTreeps.end(), icoChangeSenTreeps.begin(), icoChangeSenTreeps.end());
    ಧ್ರುವ TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_ಅಭಿವ್ಯಕ್ತಿ_Builder, {},
                                                  senTreeps, "ico", extraTriggers, ಅಸತ್ಯ, ಅಸತ್ಯ);
    std::ignore = sen_ಅಭಿವ್ಯಕ್ತಿ_Builder.getAndClearResults();

    ಯದಿ (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, initFuncp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);

    // Add trigger indices
    addTriggerIndices(logic, trigKit, ಅಸತ್ಯ);

    // The 'first iteration' trigger for top level inputs
    ಧ್ರುವ uint32_t firstIterTrigger = trigKit.extraTriggerIndex(firstIterationTrigger);

    ಧ್ರುವ auto& vifVscpToTrigIco
        = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);

    // Create and Order the body function
    AstCFunc* ಧ್ರುವ icoFuncp = V3Order::order(
        {&logic}, trigKit, trigKit.vscp(), cgRefBindings, "ico", ಅಸತ್ಯ, ಅಸತ್ಯ,
        [&](ಧ್ರುವ AstVarScope* vscp, std::set<uint32_t>& triggers) {
            AstVar* ಧ್ರುವ varp = vscp->varp();
            // If it has an explicit change detect trigger, use that,
            // otherwise fall back to using the 'first iteration' trigger
            auto it = inp2changedp.find(vscp);
            ಯದಿ (it != inp2changedp.end()) {
                triggers.ಸೇರಿಸು(trigKit.senItem2TrigIdx(it->second->sensesp()));
            } ಅನ್ಯಥಾ ಯದಿ (varp->isPrimaryInish() || varp->isSigUserRWPublic() || varp->sampled()) {
                triggers.ಸೇರಿಸು(firstIterTrigger);
            }
            // Add other triggers
            ಯದಿ (varp->isWrittenByDpi())
                triggers.ಸೇರಿಸು(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
            ಯದಿ (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
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
            Ast_ಗಂಟು_Stmt* ಧ್ರುವ stmtp = trigKit.newCompBaseCall();
            ಯದಿ (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), ಸತ್ಯ));
            ಪ್ರತ್ಯಾಗಮ stmtp;
        }(),
        // Work statements: Invoke the 'ico' function
        util::callVoidFunc(icoFuncp));

    // Release temporary input change detect SenTrees
    ಪ್ರತಿ (Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep : icoChangeSenTreeps)
        VL_DO_DANGLING(senTreep->delete_ವೃಕ್ಷ(), senTreep);
    icoChangeSenTreeps.clear();
}

//============================================================================
// EvalKit groups items that have to be passed to createEval() for a given eval region

ಸಂರಚನಾ EvalKit final {
    // The AstVarScope representing the region's trigger vector
    AstVarScope* ಧ್ರುವ m_vscp = nullptr;
    // The AstCFunc that evaluates the region's logic
    AstCFunc* ಧ್ರುವ m_funcp = nullptr;
    // Is this kit used/required?
    ತಾರ್ಕಿಕ empty() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ !m_funcp; }
};

//============================================================================
// Create the evaluation function of each region of a time step

ಶೂನ್ಯ createEval(AstNetlist* netlistp,  //
                ಧ್ರುವ TriggerKit& trigKit,  //
                ಧ್ರುವ EvalKit& actKit,  //
                ಧ್ರುವ EvalKit& nbaKit,  //
                ಧ್ರುವ EvalKit& obsKit,  //
                ಧ್ರುವ EvalKit& reactKit,  //
                TimingKit& timingKit  //
) {
    File_ಸಾಲು* ಧ್ರುವ flp = netlistp->fileline();

    // Grab the delay scheduler variable, if any
    AstVarScope* ಧ್ರುವ delaySchedVscp = timingKit.getDelayScheduler(netlistp);

    // 'createResume' consumes the contents that 'createReady' needs, so do the right order
    AstCCall* ಧ್ರುವ timingReadyp = timingKit.createReady(trigKit.vscp());
    AstCCall* ಧ್ರುವ timingResumep = timingKit.createResume(trigKit.vscp());

    // Create the 'act' region
    createEvalRegion(  //
        netlistp, VEval::ACT, 0, trigKit,
        // Use trigger
        actKit.m_vscp, nullptr,
        // Prep statements
        [&]() {
            // Compute the current 'act' triggers - the NBA triggers are the latched value
            Ast_ಗಂಟು_Stmt* stmtsp = trigKit.newCompBaseCall();
            Ast_ಗಂಟು_Stmt* ಧ್ರುವ dumpp
                = stmtsp ? trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), ಸತ್ಯ) : nullptr;
            // Mark as ready for triggered awaits
            ಯದಿ (timingReadyp) stmtsp = Ast_ಗಂಟು::addNext(stmtsp, timingReadyp->makeStmt());
            ಯದಿ (AstVarScope* ಧ್ರುವ vscAccp = trigKit.vscAccp()) {
                stmtsp = Ast_ಗಂಟು::addNext(stmtsp, trigKit.newOrIntoCall(actKit.m_vscp, vscAccp));
            }
            stmtsp = Ast_ಗಂಟು::addNext(stmtsp, trigKit.newCompExtCall(nbaKit.m_vscp));
            stmtsp = Ast_ಗಂಟು::addNext(stmtsp, dumpp);
            // Latch the 'act' triggers under the 'nba' triggers
            stmtsp = Ast_ಗಂಟು::addNext(stmtsp, trigKit.newOrIntoCall(nbaKit.m_vscp, actKit.m_vscp));
            //
            ಪ್ರತ್ಯಾಗಮ stmtsp;
        }(),
        // Work statements
        [&]() {
            Ast_ಗಂಟು_Stmt* workp = nullptr;
            ಯದಿ (AstVarScope* ಧ್ರುವ actAccp = trigKit.vscAccp()) {
                AstCMethodHard* ಧ್ರುವ cCallp = ನೂತನ AstCMethodHard{
                    flp, ನೂತನ AstVarRef{flp, actAccp, VAccess::WRITE}, VCMethod::UNPACKED_FILL,
                    ನೂತನ AstConst{flp, AstConst::Unsized64{}, 0}};
                cCallp->dtypeSetVoid();
                workp = Ast_ಗಂಟು::addNext(workp, cCallp->makeStmt());
            }
            // Resume triggered timing schedulers
            ಯದಿ (timingResumep) workp = Ast_ಗಂಟು::addNext(workp, timingResumep->makeStmt());
            // Invoke the 'act' function
            workp = Ast_ಗಂಟು::addNext(workp, util::callVoidFunc(actKit.m_funcp));
            //
            ಪ್ರತ್ಯಾಗಮ workp;
        }());

    // Create the 'inact' region
    createEvalRegion(  //
        netlistp, VEval::INACT, 0, trigKit,
        // Use explicit condition
        nullptr,
        [&]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
            ಯದಿ (!delaySchedVscp) ಪ್ರತ್ಯಾಗಮ nullptr;
            // Run if any zero delays are pending
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ callp
                = ನೂತನ AstCMethodHard{flp, ನೂತನ AstVarRef{flp, delaySchedVscp, VAccess::READ},
                                     VCMethod::SCHED_AWAITING_ZERO_DELAY};
            callp->dtypeSetBit();
            ಪ್ರತ್ಯಾಗಮ callp;
        }(),
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_ಗಂಟು_Stmt* {
            ಯದಿ (!delaySchedVscp) {
                // Nothing to do if there are no delays at all in the design
                ಪ್ರತ್ಯಾಗಮ nullptr;
            } ಅನ್ಯಥಾ ಯದಿ (v3Global.usesZeroDelay()) {
                // Resume processes watiting for #0 delay
                AstCMethodHard* ಧ್ರುವ callp = ನೂತನ AstCMethodHard{
                    flp, ನೂತನ AstVarRef{flp, delaySchedVscp, VAccess::READWRITE},
                    VCMethod::SCHED_RESUME_ZERO_DELAY};
                callp->dtypeSetVoid();
                ಪ್ರತ್ಯಾಗಮ callp->makeStmt();
            } ಅನ್ಯಥಾ {
                // Assumption was that the design doesn't use #0 delays.
                // Die at run-time if it does.
                AstCStmt* ಧ್ರುವ stmtp = ನೂತನ AstCStmt{flp};
                ಧ್ರುವ File_ಸಾಲು* ಧ್ರುವ locp = netlistp->ತುದಿ_Modulep()->fileline();
                ಧ್ರುವ std::string& file = VIdProtect::protect(locp->filename());
                ಧ್ರುವ std::string& ಸಾಲು = std::to_string(locp->lineno());
                stmtp->add("VL_FATAL_MT(\"" + V3OutFormatter::quoteNameControls(file) + "\", "
                           + ಸಾಲು
                           + ", \"\", \"ZERODLY: Design Verilated with '--no-sched-zero-delay', "
                           + "but #0 delay executed at runtime\");");
                ಪ್ರತ್ಯಾಗಮ stmtp;
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
            Ast_ಗಂಟು_Stmt* workp = nullptr;
            // Latch the 'nba' trigger flags under the following region's trigger flags
            ಯದಿ (!obsKit.empty()) {
                workp = trigKit.newOrIntoCall(obsKit.m_vscp, nbaKit.m_vscp);
            } ಅನ್ಯಥಾ ಯದಿ (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, nbaKit.m_vscp);
            }
            // Invoke the 'nba' function
            workp = Ast_ಗಂಟು::addNext(workp, util::callVoidFunc(nbaKit.m_funcp));
            // Clear the 'nba' triggers
            workp = Ast_ಗಂಟು::addNext(workp, trigKit.newClearCall(nbaKit.m_vscp));
            //
            ಪ್ರತ್ಯಾಗಮ workp;
        }(),
        // Extra work (not conditional on having had a fired trigger)
        [&](AstVarScope* continuep) -> Ast_ಗಂಟು_Stmt* {
            // Check if any dynamic NBAs are pending, if there are any in the design
            ಯದಿ (!netlistp->nbaEventp()) ಪ್ರತ್ಯಾಗಮ nullptr;
            AstVarScope* ಧ್ರುವ nbaEventp = netlistp->nbaEventp();
            AstVarScope* ಧ್ರುವ nbaEventTriggerp = netlistp->nbaEventTriggerp();
            UASSERT(nbaEventTriggerp, "NBA event trigger var should exist");
            netlistp->nbaEventp(nullptr);
            netlistp->nbaEventTriggerp(nullptr);

            // If a dynamic NBA is pending, clear the pending flag and fire the ready event
            AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{flp, ನೂತನ AstVarRef{flp, nbaEventTriggerp, VAccess::READ}};
            ifp->addThensp(util::setVar(continuep, 1));
            ifp->addThensp(util::setVar(nbaEventTriggerp, 0));
            AstCMethodHard* ಧ್ರುವ firep = ನೂತನ AstCMethodHard{
                flp, ನೂತನ AstVarRef{flp, nbaEventp, VAccess::WRITE}, VCMethod::EVENT_FIRE};
            firep->dtypeSetVoid();
            ifp->addThensp(firep->makeStmt());
            ಪ್ರತ್ಯಾಗಮ ifp;
        });

    // Create the 'obs' region
    createEvalRegion(  //
        netlistp, VEval::OBS, 0, trigKit,
        // Use trigger
        obsKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_ಗಂಟು_Stmt* {
            ಯದಿ (obsKit.empty()) ಪ್ರತ್ಯಾಗಮ nullptr;
            Ast_ಗಂಟು_Stmt* workp = nullptr;
            // Latch the Observed trigger flags under the Reactive trigger flags
            ಯದಿ (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, obsKit.m_vscp);
            }
            // Invoke the 'obs' function
            workp = Ast_ಗಂಟು::addNext(workp, util::callVoidFunc(obsKit.m_funcp));
            // Clear the 'obs' triggers
            workp = Ast_ಗಂಟು::addNext(workp, trigKit.newClearCall(obsKit.m_vscp));
            //
            ಪ್ರತ್ಯಾಗಮ workp;
        }());

    // Create the 'react' region
    createEvalRegion(  //
        netlistp, VEval::REACT, 0, trigKit,
        // Use trigger
        reactKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_ಗಂಟು_Stmt* {
            ಯದಿ (reactKit.empty()) ಪ್ರತ್ಯಾಗಮ nullptr;
            // Invoke the 'react' function
            Ast_ಗಂಟು_Stmt* workp = util::callVoidFunc(reactKit.m_funcp);
            // Clear the 'react' triggers
            workp = Ast_ಗಂಟು::addNext(workp, trigKit.newClearCall(reactKit.m_vscp));
            ಪ್ರತ್ಯಾಗಮ workp;
        }());
}

}  // namespace

//============================================================================
// Helper that builds virtual interface trigger sentrees

VirtIfaceTriggers::VscpTrigMap VirtIfaceTriggers::makeVscpToTrigMap(ಧ್ರುವ TriggerKit& trigKit,
                                                                    uint32_t firstIndex) ಧ್ರುವ {
    VscpTrigMap map;
    uint32_t index = firstIndex;
    ಪ್ರತಿ (ಧ್ರುವ auto& entry : m_triggers) {
        map.emplace(entry.m_vscp, trigKit.extraTriggerIndex(index));
        ++index;
    }
    ಪ್ರತ್ಯಾಗಮ map;
}

//============================================================================
// Top level entry-point to scheduling

ಶೂನ್ಯ schedule(AstNetlist* netlistp) {
    ಧ್ರುವ auto addSizeStat = [](ಧ್ರುವ string& name, ಧ್ರುವ LogicByScope& lbs) {
        uint64_t size = 0;
        lbs.foreachLogic([&](Ast_ಗಂಟು* nodep) { size += nodep->ಗಂಟು_ಎಣಿಕೆ(); });
        V3Stats::addStat("Scheduling, " + name, size);
    };

    // Step 2: Prepare external domains for timing and virtual interfaces
    // Create extra triggers for virtual interfaces
    ಧ್ರುವ auto& virtIfaceTriggers = makeVirtIfaceTriggers(netlistp);
    // Resolve what covergroup reference formal arguments are bound to, which is visible here
    // but not from V3Order, where the reads through them must be modeled
    ಧ್ರುವ CovergroupRefBindings& cgRefBindings = makeCovergroupRefBindings(netlistp);
    // Prepare timing-related logic and external domains
    TimingKit timingKit = prepareTiming(netlistp);

    // Step 3: Gather and classify all logic in the design
    LogicClasses logicClasses = gatherLogicClasses(netlistp);

    ಯದಿ (v3Global.opt.stats()) {
        V3Stats::statsStage("sched-gather");
        addSizeStat("size of class: static", logicClasses.m_static);
        addSizeStat("size of class: initial", logicClasses.m_initial);
        addSizeStat("size of class: final", logicClasses.m_final);
    }

    // Step 4: Schedule static, initial and final logic classes in source order
    AstCFunc* ಧ್ರುವ staticp = createStatic(netlistp, logicClasses);
    ಯದಿ (v3Global.opt.stats()) V3Stats::statsStage("sched-static");

    createInitial(netlistp, logicClasses);
    ಯದಿ (v3Global.opt.stats()) V3Stats::statsStage("sched-initial");

    createFinal(netlistp, logicClasses);
    ಯದಿ (v3Global.opt.stats()) V3Stats::statsStage("sched-final");

    // Step 5: Break combinational cycles by introducing hybrid logic
    // Note: breakCycles also removes corresponding logic from logicClasses.m_comb;
    logicClasses.m_hybrid = breakCycles(netlistp, logicClasses.m_comb);
    ಯದಿ (v3Global.opt.stats()) {
        addSizeStat("size of class: clocked", logicClasses.m_clocked);
        addSizeStat("size of class: combinational", logicClasses.m_comb);
        addSizeStat("size of class: hybrid", logicClasses.m_hybrid);
        V3Stats::statsStage("sched-break-cycles");
    }

    // We pass around a single SenExprBuilder instance, as we only need one set of 'prev' variables
    // for edge/change detection in sensitivity expressions, which this keeps track of.
    Ast_ತುದಿ_Scope* ಧ್ರುವ ತುದಿ_Scopep = netlistp->ತುದಿ_Scopep();
    AstScope* ಧ್ರುವ scopeTopp = ತುದಿ_Scopep->scopep();
    Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder sen_ಅಭಿವ್ಯಕ್ತಿ_Builder{scopeTopp};

    // Step 6: Create 'settle' region that restores the combinational invariant
    createSettle(netlistp, staticp, sen_ಅಭಿವ್ಯಕ್ತಿ_Builder, logicClasses, cgRefBindings);
    ಯದಿ (v3Global.opt.stats()) V3Stats::statsStage("sched-settle");

    // Step 7: Partition the clocked and combinational (including hybrid) logic into pre/act/nba.
    // All clocks (signals referenced in an AstSenTree) generated via a blocking assignment
    // (including combinationally generated signals) are computed within the act region.
    LogicRegions logicRegions
        = partition(logicClasses.m_clocked, logicClasses.m_comb, logicClasses.m_hybrid);
    logicRegions.m_obs = logicClasses.m_observed;
    logicRegions.m_react = logicClasses.m_reactive;
    ಯದಿ (v3Global.opt.stats()) {
        addSizeStat("size of region: Active Pre", logicRegions.m_pre);
        addSizeStat("size of region: Active", logicRegions.m_act);
        addSizeStat("size of region: NBA", logicRegions.m_nba);
        addSizeStat("size of region: Observed", logicRegions.m_obs);
        addSizeStat("size of region: Reactive", logicRegions.m_react);
        V3Stats::statsStage("sched-partition");
    }

    // Step 8: Replicate combinational logic
    LogicReplicas logicReplicas = replicateLogic(logicRegions);
    ಯದಿ (v3Global.opt.stats()) {
        addSizeStat("size of replicated logic: Input", logicReplicas.m_ico);
        addSizeStat("size of replicated logic: Active", logicReplicas.m_act);
        addSizeStat("size of replicated logic: NBA", logicReplicas.m_nba);
        addSizeStat("size of replicated logic: Observed", logicReplicas.m_obs);
        addSizeStat("size of replicated logic: Reactive", logicReplicas.m_react);
        V3Stats::statsStage("sched-replicate");
    }

    // Step 9: Create the input combinational logic
    createIcoRegion(netlistp, staticp, sen_ಅಭಿವ್ಯಕ್ತಿ_Builder, logicReplicas.m_ico, virtIfaceTriggers,
                    cgRefBindings);
    ಯದಿ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-ico");

    // Step 10: Create the triggers
    AstVarScope* ಧ್ರುವ dpiExportTriggerVscp = netlistp->dpiExportTriggerp();
    netlistp->dpiExportTriggerp(nullptr);  // Finished with this here

    // We may have an extra trigger for variable updated in DPI exports
    TriggerKit::ExtraTriggers extraTriggers;
    ಧ್ರುವ uint32_t dpiExportTriggerIndex = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    ಧ್ರುವ uint32_t firstVifTriggerIndex = extraTriggers.size();
    ಪ್ರತಿ (ಧ್ರುವ auto& entry : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + entry.m_ifacep->name() + "."
                               + entry.m_memberp->name());
    }

    ಧ್ರುವ auto& preTreeps = getSenTreesUsedBy({&logicRegions.m_pre});
    ಧ್ರುವ auto& senTreeps = getSenTreesUsedBy({&logicRegions.m_act,  //
                                               &logicRegions.m_nba,  //
                                               &logicRegions.m_obs,  //
                                               &logicRegions.m_react,  //
                                               &timingKit.m_lbs});
    ಧ್ರುವ TriggerKit trigKit
        = TriggerKit::create(netlistp, staticp, sen_ಅಭಿವ್ಯಕ್ತಿ_Builder, preTreeps, senTreeps, "act",
                             extraTriggers, ಅಸತ್ಯ, v3Global.usesTiming());

    // Add post updates from the timing kit
    ಯದಿ (timingKit.m_postUpdates) trigKit.compBasep()->addStmtsp(timingKit.m_postUpdates);

    ಯದಿ (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, staticp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);
    ಯದಿ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-triggers");

    // Note: Experiments so far show that running the Act (or Ico) regions on
    // multiple threads is always a net loss, so only use multi-threading for
    // NBA for now. This can be revised if evidence is available that it would
    // be beneficial

    // Step 11: Create the 'act' region evaluation function

    // Add the trigger indices of the input logic
    addTriggerIndices(logicRegions.m_pre, trigKit, ಸತ್ಯ);
    addTriggerIndices(logicRegions.m_act, trigKit, ಅಸತ್ಯ);
    addTriggerIndices(logicReplicas.m_act, trigKit, ಅಸತ್ಯ);
    addTriggerIndices(timingKit.m_lbs, trigKit, ಅಸತ್ಯ);

    // External triggers of variables. Trigger indices are the same in all regions, so share.
    ಧ್ರುವ std::map<ಧ್ರುವ AstVarScope*, std::vector<uint32_t>> timingTriggers
        = timingKit.externalTriggerIndices(trigKit);
    ಧ್ರುವ auto& vifVscpToTrig = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);
    ಧ್ರುವ V3Order::ExternalTriggersProvider externalTriggers
        = [&](ಧ್ರುವ AstVarScope* vscp, std::set<uint32_t>& triggers) {
              auto it = timingTriggers.find(vscp);
              ಯದಿ (it != timingTriggers.end())
                  triggers.ಸೇರಿಸು(it->second.begin(), it->second.end());
              ಯದಿ (vscp->varp()->isWrittenByDpi()) {
                  triggers.ಸೇರಿಸು(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
              }
              ಯದಿ (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                  addTriggeredIface(vscp, vifVscpToTrig, virtIfaceTriggers, triggers);
              }
          };

    AstCFunc* ಧ್ರುವ actFuncp
        = V3Order::order({&logicRegions.m_pre, &logicRegions.m_act, &logicReplicas.m_act}, trigKit,
                         trigKit.vscp(), cgRefBindings, "act", ಅಸತ್ಯ, ಅಸತ್ಯ, externalTriggers);
    util::splitCheck(actFuncp);
    ಯದಿ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-act");

    ಧ್ರುವ EvalKit actKit{trigKit.vscp(), actFuncp};

    // Orders a region's logic and creates the region eval function
    ಧ್ರುವ auto order = [&](ಧ್ರುವ std::string& name,
                           ಧ್ರುವ std::vector<V3Sched::LogicByScope*>& logic) -> EvalKit {
        UINFO(2, "Scheduling " << name << " #logic = " << logic.size());
        AstVarScope* ಧ್ರುವ trigVscp = trigKit.newTrigVec(name);
        // Add the trigger indices of the input logic
        ಪ್ರತಿ (LogicByScope* lbs : logic) addTriggerIndices(*lbs, trigKit, ಅಸತ್ಯ);

        AstCFunc* ಧ್ರುವ funcp
            = V3Order::order(logic, trigKit, trigVscp, cgRefBindings, name,
                             name == "nba" && v3Global.opt.mtasks(), ಅಸತ್ಯ, externalTriggers);

        ಪ್ರತ್ಯಾಗಮ {trigVscp, funcp};
    };

    // Step 12: Create the 'nba' region evaluation function
    ಧ್ರುವ EvalKit nbaKit = order("nba", {&logicRegions.m_nba, &logicReplicas.m_nba});
    util::splitCheck(nbaKit.m_funcp);
    ಯದಿ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-nba");

    // Orders a region's logic and creates the region eval function (only if there is any logic in
    // the region)
    ಧ್ರುವ auto orderIfNonEmpty
        = [&](ಧ್ರುವ std::string& name, ಧ್ರುವ std::vector<LogicByScope*>& logic) -> EvalKit {
        ಯದಿ (logic[0]->empty())
            ಪ್ರತ್ಯಾಗಮ {};  // if region is empty, replica is supposed to be empty as well
        ಧ್ರುವ auto& kit = order(name, logic);
        ಯದಿ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-" + name);
        ಪ್ರತ್ಯಾಗಮ kit;
    };

    // Step 13: Create the 'obs' region evaluation function
    ಧ್ರುವ EvalKit obsKit = orderIfNonEmpty("obs", {&logicRegions.m_obs, &logicReplicas.m_obs});

    // Step 14: Create the 'react' region evaluation function
    ಧ್ರುವ EvalKit reactKit
        = orderIfNonEmpty("react", {&logicRegions.m_react, &logicReplicas.m_react});

    // Step 15: Create the 'postponed' region evaluation function
    createPostponed(netlistp, logicClasses);

    // Step 16: Populate the eval entry point function of each region of a time step
    createEval(netlistp, trigKit, actKit, nbaKit, obsKit, reactKit, timingKit);

    // Step 17: Add neccessary evaluation before awaits
    ಯದಿ (AstCCall* ಧ್ರುವ readyp = timingKit.createReady(trigKit.vscp())) {
        staticp->addStmtsp(readyp->makeStmt());
        beforeTrigVisitor(netlistp, sen_ಅಭಿವ್ಯಕ್ತಿ_Builder, trigKit);
    } ಅನ್ಯಥಾ {
        // beforeTrigVisitor clears Sentree pointers in AstCAwaits (as these sentrees will get
        // deleted later) if there was no need to call it, SenTrees have to be cleaned manually
        netlistp->foreach([](AstCAwait* ಧ್ರುವ cAwaitp) { cAwaitp->clearSentreep(); });
    }
    ಯದಿ (AstVarScope* ಧ್ರುವ trigAccp = trigKit.vscAccp()) {
        // Copy trigger vector to accumulator at the end of static initialziation so,
        // triggers fired during initialization persist to the first resume.
        ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ trigAccDTypep
            = VN_AS(trigAccp->dtypep(), UnpackArrayDType);
        UASSERT_OBJ(
            trigAccDTypep->ಬಲ() == 0, trigAccp,
            "Expected that trigger vector and accumulator start elements enumeration from 0");
        UASSERT_OBJ(trigAccDTypep->ಎಡ() >= 0, trigAccp,
                    "Expected that trigger vector and accumulator has no negative indexes");
        File_ಸಾಲು* ಧ್ರುವ flp = trigAccp->fileline();
        AstVarScope* ಧ್ರುವ vscp = netlistp->ತುದಿ_Scopep()->createTemp("__Vi", 32);
        AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{flp};
        loopp->addStmtsp(
            ನೂತನ AstAssign{flp,
                          ನೂತನ AstArraySel{flp, ನೂತನ AstVarRef{flp, trigAccp, VAccess::WRITE},
                                          ನೂತನ AstVarRef{flp, vscp, VAccess::READ}},
                          ನೂತನ AstArraySel{flp, ನೂತನ AstVarRef{flp, actKit.m_vscp, VAccess::READ},
                                          ನೂತನ AstVarRef{flp, vscp, VAccess::READ}}});
        loopp->addStmtsp(util::incrementVar(vscp));
        loopp->addStmtsp(ನೂತನ AstLoopTest{
            flp, loopp,
            ನೂತನ AstLte{flp, ನೂತನ AstVarRef{flp, vscp, VAccess::READ},
                       ನೂತನ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, 32,
                                    static_cast<uint32_t>(trigAccDTypep->ಎಡ())}}});
        staticp->addStmtsp(loopp);
    }

    // Step 18: Clean up
    netlistp->clearStlFirstIterationp();

    ಯದಿ (v3Global.opt.stats()) {
        // A sample() call resolved to the union over its covergroup's constructions reads more
        // than it can, which orders it against more logic than necessary
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per instance",
                         cgRefBindings.ಅಂಕೆ_Exact_Calls());
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per type",
                         cgRefBindings.ಅಂಕೆ_Union_Calls());
    }

    // Haven't split static initializer yet
    util::splitCheck(staticp);

    // Record eval stats
    netlistp->addEvalStats("sched");

    // Dump
    V3Global::dump_Check_Global_ವೃಕ್ಷ("sched", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

}  // namespace V3Sched
