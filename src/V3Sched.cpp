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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Sched.h"

#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3EmitCBase.h"
#অন্তর্ভাব "V3EmitV.h"
#অন্তর্ভাব "V3Order.h"
#অন্তর্ভাব "V3SenExprBuilder.h"
#অন্তর্ভাব "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ V3Sched {

নামাকাশ {

//============================================================================
// Utility functions

std::vector<ধ্রুব Ast_Sen_বৃক্ষ*> getSenTreesUsedBy(ধ্রুব std::vector<ধ্রুব LogicByScope*>& lbsps) {
    ধ্রুব VNUser1InUse user1InUse;
    std::vector<ধ্রুব Ast_Sen_বৃক্ষ*> result;
    প্রতি (ধ্রুব LogicByScope* ধ্রুব lbsp : lbsps) {
        প্রতি (ধ্রুব auto& entry : *lbsp) {
            AstActive* ধ্রুব activep = entry.m_activep;
            Ast_Sen_বৃক্ষ* ধ্রুব senTreep = activep->sentreep();
            যদি (senTreep->user1SetOnce()) অনুবর্ত;
            যদি (senTreep->hasClocked() || senTreep->hasHybrid()) result.ঠেলো_back(senTreep);
        }
    }
    প্রত্যাগম result;
}

শূন্য addTriggerIndices(LogicByScope& lbs, ধ্রুব TriggerKit& trigKit, তার্কিক pre) {
    প্রতি (LogicByScope::Entry& entry : lbs) {
        Ast_Sen_বৃক্ষ* ধ্রুব senTreep = entry.m_activep->sentreep();
        যদি (senTreep->hasCombo()) অনুবর্ত;
        entry.m_triggers = trigKit.triggerIndices(senTreep, pre);
    }
}

// Add the VIF triggers that a given VarScope should be sensitive to.
// Case 2 (non-virtual interface read): sensitive to that specific VarScope's trigger only
// Case 3 (virtual interface read): sensitive to all member triggers of the same interface type
শূন্য addTriggeredIface(ধ্রুব AstVarScope* vscp, ধ্রুব VirtIfaceTriggers::VscpTrigMap& vscpToTrig,
                       ধ্রুব VirtIfaceTriggers& virtIfaceTriggers, std::set<uint32_t>& triggers) {
    যদি (vscp->varp()->isVirtIface()) {
        // Virtual interface variable -- sensitive to all member triggers of this interface type
        ধ্রুব AstIface* ধ্রুব ifacep = VN_AS(vscp->varp()->dtypep(), IfaceRefDType)->ifacep();
        প্রতি (ধ্রুব auto& entry : virtIfaceTriggers.m_triggers) {
            যদি (entry.m_ifacep == ifacep) {
                ধ্রুব auto it = vscpToTrig.find(entry.m_vscp);
                যদি (it != vscpToTrig.end()) triggers.ঢোকাও(it->second);
            }
        }
    } অন্যথা {
        // Non-virtual interface member -- sensitive to this VarScope's trigger only
        ধ্রুব auto it = vscpToTrig.find(vscp);
        যদি (it != vscpToTrig.end()) triggers.ঢোকাও(it->second);
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
শূন্য createEvalRegion(
    AstNetlist* netlistp,  //
    VEval eval,  // The entry point of the current region
    // Index of the region's 'first iteration' extra trigger, if it has one, otherwise ignored
    uint32_t firstIterTrigger,
    ধ্রুব TriggerKit& trigKit,  // The trigger kit
    AstVarScope* trigp,  // The trigger vector - may be nullptr if no triggers or using 'condp'
    Ast_গ্রন্থি_রাশিমালা* condp,  // Explicit condition that must be true to run 'phaseWorkp'
    Ast_গ্রন্থি_Stmt* phasePrepp,  // Prep statements run before checking triggers
    Ast_গ্রন্থি_Stmt* phaseWorkp,  // The work to do if anything triggered
    // Extra statements to run after the work, even if no triggers fired. This function is
    // passed a variable, which must be set to true if we must continue and loop again,
    // and must be unmodified otherwise.
    std::function<Ast_গ্রন্থি_Stmt*(AstVarScope*)> phaseExtra = [](AstVarScope*) { প্রত্যাগম nullptr; }  //
) {
    UASSERT(!trigp || !condp, "Cannot use both 'trigp' and 'condp' in 'createEvalRegion'");
    UASSERT(!eval.firstIteration() || trigp,
            "Region without triggers cannot need a first iteration flag");

    // All work is under a trigger or condition, so with neither the region has
    // nothing to evaluate, and what we create below reduces to a no-op function.

    ধ্রুব std::string tag = eval.tag();
    Ast_শীর্ষ_Scope* ধ্রুব শীর্ষ_Scopep = netlistp->শীর্ষ_Scopep();
    File_সারি* ধ্রুব flp = netlistp->fileline();

    // Populate the trigger dump entry point function
    যদি (trigp) {
        netlistp->dumpTriggersFuncp(eval)->addStmtsp(trigKit.newDumpCall(trigp, tag, অসত্য));
    }

    AstCFunc* ধ্রুব funcp = netlistp->evalFuncp(eval);

    // A flag is passed from the run-time eval loop if this is the first iteration of the
    // current loop
    যদি (eval.firstIteration()) {
        AstVarScope* ধ্রুব firstIterArgp = util::newArgument(funcp, netlistp->findBitDType(),
                                                             "firstIteration", VDirection::প্রবেশ);

        // Set the region's 'first iteration' trigger straight from the argument
        funcp->addStmtsp(trigKit.newExtraTriggerAssignment(firstIterArgp, firstIterTrigger));

        // Only 'stl' also needs a module level flag, for design logic that reads
        // it directly (see V3Timing). Those reads can be anywhere in the design,
        // hence module level. Always created, even if nothing reads it.
        // TODO: get rid of this special case
        যদি (eval == VEval::STL) {
            AstVarScope* ধ্রুব firstIterp = netlistp->stlFirstIterationp();
            firstIterp->varp()->noReset(সত্য);
            firstIterp->varp()->isInternal(সত্য);
            funcp->addStmtsp(নূতন AstAssign{flp, নূতন AstVarRef{flp, firstIterp, VAccess::WRITE},
                                           নূতন AstVarRef{flp, firstIterArgp, VAccess::READ}});
        }
    }

    {
        // Add the preparatory statements
        funcp->addStmtsp(phasePrepp);

        // The execute flag
        AstVarScope* ধ্রুব executeFlagp = শীর্ষ_Scopep->createTemp("__V" + tag + "Execute", 1);
        executeFlagp->varp()->noReset(সত্য);

        // If there is work in this region, execute it if any triggers fired
        যদি (phaseWorkp) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব lhsp = নূতন AstVarRef{flp, executeFlagp, VAccess::WRITE};
            // If using explicit condition, that directly determines whether to execute,
            // otherwise check if any triggers are fired
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsp = condp ? condp : trigKit.newAnySetCall(trigp);
            funcp->addStmtsp(নূতন AstAssign{flp, lhsp, rhsp});

            // Add the work
            AstIf* ধ্রুব ifp = নূতন AstIf{flp, নূতন AstVarRef{flp, executeFlagp, VAccess::READ}};
            ifp->addThensp(phaseWorkp);
            funcp->addStmtsp(ifp);
        }

        // Construct the extra statements
        Ast_গ্রন্থি_Stmt* ধ্রুব extraWorkp = phaseExtra(executeFlagp);
        যদি (extraWorkp) funcp->addStmtsp(extraWorkp);

        // The function returns true iff it did run work
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব retp
            = phaseWorkp || extraWorkp
                  ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নূতন AstVarRef{flp, executeFlagp, VAccess::READ})
                  : static_cast<Ast_গ্রন্থি_রাশিমালা*>(নূতন AstConst{flp, AstConst::BitFalse{}});
        funcp->addStmtsp(নূতন AstCReturn{flp, retp});
    }
}

//============================================================================
// Collect and classify all logic in the design

LogicClasses gatherLogicClasses(AstNetlist* netlistp) {
    LogicClasses result;

    netlistp->foreach([&](AstScope* scopep) {
        scopep->foreach([&](AstActive* activep) {
            Ast_Sen_বৃক্ষ* ধ্রুব senTreep = activep->sentreep();
            যদি (senTreep->hasStatic()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "static initializer with additional sensitivities");
                result.m_static.add(scopep, activep);
            } অন্যথা যদি (senTreep->hasInitial()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'initial' logic with additional sensitivities");
                result.m_initial.add(scopep, activep);
            } অন্যথা যদি (senTreep->hasFinal()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'final' logic with additional sensitivities");
                result.m_final.add(scopep, activep);
            } অন্যথা যদি (senTreep->hasCombo()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "combinational logic with additional sensitivities");
                যদি (VN_IS(activep->stmtsp(), AlwaysPostponed)) {
                    result.m_postponed.add(scopep, activep);
                } অন্যথা {
                    result.m_comb.add(scopep, activep);
                }
            } অন্যথা {
                UASSERT_OBJ(senTreep->hasClocked(), activep, "What else could it be?");
                যদি (VN_IS(activep->stmtsp(), AlwaysObserved)) {
                    result.m_observed.add(scopep, activep);
                } অন্যথা যদি (VN_IS(activep->stmtsp(), AlwaysReactive)) {
                    result.m_reactive.add(scopep, activep);
                } অন্যথা {
                    result.m_clocked.add(scopep, activep);
                }
            }
        });
    });

    প্রত্যাগম result;
}

//============================================================================
// Simple ordering in source order

শূন্য orderSequentially(AstCFunc* funcp, ধ্রুব LogicByScope& lbs) {
    // Create new subfunc for scope
    ধ্রুব auto createNewSubFuncp = [&](AstScope* ধ্রুব scopep) {
        ধ্রুব string subName{funcp->name() + "__" + scopep->nameDotless()};
        AstCFunc* ধ্রুব subFuncp = নূতন AstCFunc{scopep->fileline(), subName, scopep};
        subFuncp->isLoose(সত্য);
        subFuncp->isConst(অসত্য);
        subFuncp->declPrivate(সত্য);
        subFuncp->slow(funcp->slow());
        scopep->addBlocksp(subFuncp);
        // Call it from the top function
        funcp->addStmtsp(util::callVoidFunc(subFuncp));
        প্রত্যাগম subFuncp;
    };
    ধ্রুব VNUser1InUse user1InUse;  // AstScope -> AstCFunc: the sub-function for the scope
    ধ্রুব VNUser2InUse user2InUse;  // AstScope -> int: sub-function counter used for names
    প্রতি (ধ্রুব auto& entry : lbs) {
        AstScope* ধ্রুব scopep = entry.m_scopep;
        AstActive* ধ্রুব activep = entry.m_activep;
        // Create a sub-function per scope so we can V3Combine them later
        যদি (!scopep->user1p()) scopep->user1p(createNewSubFuncp(scopep));
        // Add statements to sub-function
        প্রতি (Ast_গ্রন্থি *logicp = activep->stmtsp(), *nextp; logicp; logicp = nextp) {
            auto* subFuncp = VN_AS(scopep->user1p(), CFunc);
            nextp = logicp->nextp();
            যদি (Ast_গ্রন্থি_Procedure* ধ্রুব procp = VN_CAST(logicp, গ্রন্থি_Procedure)) {
                যদি (Ast_গ্রন্থি* bodyp = procp->stmtsp()) {
                    bodyp->unlinkFrBackWithNext();
                    // If the process is suspendable, we need a separate function (a coroutine)
                    যদি (procp->isSuspendable()) {
                        funcp->slow(অসত্য);
                        subFuncp = createNewSubFuncp(scopep);
                        subFuncp->name(subFuncp->name() + "__Vtiming__"
                                       + cvtToStr(scopep->user2Inc()));
                        subFuncp->rtnType("VlCoroutine");
                        যদি (VN_IS(procp, Always)) {
                            subFuncp->slow(অসত্য);
                            File_সারি* ধ্রুব flp = procp->fileline();
                            Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp = নূতন AstCExpr{
                                flp, "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish())", 1};
                            AstLoop* ধ্রুব loopp = নূতন AstLoop{flp};
                            loopp->addStmtsp(নূতন AstLoopTest{flp, loopp, condp});
                            loopp->addStmtsp(bodyp);
                            bodyp = loopp;
                        }
                    }
                    subFuncp->addStmtsp(bodyp);
                    যদি (procp->needProcess()) subFuncp->setNeedProcess();
                    // A coroutine holds a single process, so it is complete here
                    যদি (procp->isSuspendable()) util::splitCheck(subFuncp);
                }
            } অন্যথা {
                logicp->unlinkFrBack();
                subFuncp->addStmtsp(logicp);
            }
        }
        যদি (activep->backp()) activep->unlinkFrBack();
        VL_DO_DANGLING(activep->delete_বৃক্ষ(), activep);
    }
    // Split sub-functions once complete
    প্রতি (ধ্রুব auto& entry : lbs) {
        AstScope* ধ্রুব scopep = entry.m_scopep;
        যদি (AstCFunc* ধ্রুব subFuncp = VN_AS(scopep->user1p(), CFunc)) {
            util::splitCheck(subFuncp);
            scopep->user1p(nullptr);
        }
    }
}

//============================================================================
// Create simply ordered functions

AstCFunc* createStatic(AstNetlist* netlistp, ধ্রুব LogicClasses& logicClasses) {
    AstCFunc* ধ্রুব funcp = netlistp->evalFuncp(VEval::STATIC);

    ধ্রুব LogicByScope& orig = logicClasses.m_static;
    যদি (orig.size() <= 1) {
        orderSequentially(funcp, orig);
        প্রত্যাগম funcp;
    }

    // Level-based module sorting can reorder packages so that an importing
    // package runs before the imported one.  Re-sort package entries by source
    // file position to restore compilation order (IEEE 1800-2023 26.3).
    std::vector<size_t> indices(orig.size());
    প্রতি (size_t i = 0; i < orig.size(); ++i) indices[i] = i;
    std::stable_sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        ধ্রুব Ast_গ্রন্থি_Module* ধ্রুব modA = orig[a].m_scopep->modp();
        ধ্রুব Ast_গ্রন্থি_Module* ধ্রুব modB = orig[b].m_scopep->modp();
        ধ্রুব তার্কিক isPkgA = VN_IS(modA, Package);
        ধ্রুব তার্কিক isPkgB = VN_IS(modB, Package);
        যদি (isPkgA != isPkgB) প্রত্যাগম isPkgA;  // Packages before non-packages
        যদি (isPkgA && isPkgB) {
            // Sort packages by source file position (compilation order)
            প্রত্যাগম modA->fileline()->operatorCompare(*modB->fileline()) < 0;
        }
        প্রত্যাগম অসত্য;  // Both non-package: preserve original order
    });
    LogicByScope sorted;
    প্রতি (ধ্রুব size_t i : indices) sorted.add(orig[i].m_scopep, orig[i].m_activep);

    orderSequentially(funcp, sorted);
    প্রত্যাগম funcp;  // Not splitting yet as it is not final
}

শূন্য createInitial(AstNetlist* netlistp, ধ্রুব LogicClasses& logicClasses) {
    AstCFunc* ধ্রুব funcp = netlistp->evalFuncp(VEval::INITIAL);
    orderSequentially(funcp, logicClasses.m_initial);
    util::splitCheck(funcp);
}

শূন্য createPostponed(AstNetlist* netlistp, ধ্রুব LogicClasses& logicClasses) {
    AstCFunc* ধ্রুব funcp = netlistp->evalFuncp(VEval::POSTPONED);
    orderSequentially(funcp, logicClasses.m_postponed);
    util::splitCheck(funcp);
}

শূন্য createFinal(AstNetlist* netlistp, ধ্রুব LogicClasses& logicClasses) {
    AstCFunc* ধ্রুব funcp = netlistp->evalFuncp(VEval::FINAL);
    orderSequentially(funcp, logicClasses.m_final);
    util::splitCheck(funcp);
}

//============================================================================
// Helper that creates virtual interface value-change triggers

শূন্য addVirtIfaceTriggerAssignments(AstNetlist* netlistp, AstCFunc* initFuncp,
                                    ধ্রুব VirtIfaceTriggers& virtIfaceTriggers,
                                    uint32_t firstIndex, ধ্রুব TriggerKit& trigKit) {
    uint32_t index = firstIndex;
    প্রতি (ধ্রুব auto& entry : virtIfaceTriggers.m_triggers) {
        trigKit.add_মান_Change_Trigger_Assignment(netlistp, initFuncp, entry.m_vscp, index);
        ++index;
    }
}

// Order the combinational logic to create the 'stl' region
শূন্য createSettle(AstNetlist* netlistp, AstCFunc* ধ্রুব initFuncp, Sen_রাশিমালা_Builder& sen_রাশিমালা_Bulider,
                  LogicClasses& logicClasses, ধ্রুব CovergroupRefBindings& cgRefBindings) {
    // Clone, because ordering is destructive, but we still need them for the other regions
    LogicByScope comb = logicClasses.m_comb.clone();
    LogicByScope hybrid = logicClasses.m_hybrid.clone();

    // We have an extra trigger denoting this is the first iteration of the settle loop
    TriggerKit::ExtraTriggers extraTriggers;
    ধ্রুব uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");

    // Gather the relevant sensitivity expressions and create the trigger kit
    ধ্রুব auto& senTreeps = getSenTreesUsedBy({&comb, &hybrid});
    ধ্রুব TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_রাশিমালা_Bulider, {},
                                                  senTreeps, "stl", extraTriggers, সত্য, অসত্য);

    // Add trigger indices (comb has none, so only do the hybrid)
    addTriggerIndices(hybrid, trigKit, অসত্য);

    // First trigger is for pure combinational triggers (first iteration)
    ধ্রুব uint32_t প্রবেশ_Changed = trigKit.extraTriggerIndex(firstIterationTrigger);

    // Create and the body function
    AstCFunc* ধ্রুব stlFuncp = V3Order::order(
        {&comb, &hybrid}, trigKit, trigKit.vscp(), cgRefBindings, "stl", অসত্য, সত্য,
        [=](ধ্রুব AstVarScope*, std::set<uint32_t>& triggers) { triggers.ঢোকাও(প্রবেশ_Changed); });
    util::splitCheck(stlFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::STL, firstIterationTrigger, trigKit,
        // Use trigger
        trigKit.vscp(), nullptr,
        // Explicit condition
        // Prep statements: Compute the current 'stl' triggers
        [&trigKit] {
            Ast_গ্রন্থি_Stmt* ধ্রুব stmtp = trigKit.newCompBaseCall();
            যদি (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), সত্য));
            প্রত্যাগম stmtp;
        }(),
        // Work statements: Invoke the 'stl' function
        util::callVoidFunc(stlFuncp));
}

//============================================================================
// Order the replicated combinational logic to create the 'ico' region

শূন্য createIcoRegion(AstNetlist* netlistp, AstCFunc* ধ্রুব initFuncp,
                     Sen_রাশিমালা_Builder& sen_রাশিমালা_Builder, LogicByScope& logic,
                     ধ্রুব VirtIfaceTriggers& virtIfaceTriggers,
                     ধ্রুব CovergroupRefBindings& cgRefBindings) {
    // SystemC only: Any top level inputs feeding a combinational logic must be marked,
    // so we can make them sc_sensitive
    যদি (v3Global.opt.systemC()) {
        logic.foreachLogic([](Ast_গ্রন্থি* logicp) {
            logicp->foreach([](AstVarRef* refp) {
                যদি (refp->access().isWriteOnly()) প্রত্যাগম;
                AstVarScope* ধ্রুব vscp = refp->varScopep();
                যদি (vscp->scopep()->is_শীর্ষ() && vscp->varp()->isNonOutput()) {
                    vscp->varp()->scSensitive(সত্য);
                }
            });
        });
    }

    // We have some extra trigger denoting external conditions
    AstVarScope* ধ্রুব dpiExportTriggerVscp = netlistp->dpiExportTriggerp();

    TriggerKit::ExtraTriggers extraTriggers;
    ধ্রুব uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");
    ধ্রুব uint32_t dpiExportTriggerIndex = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    ধ্রুব uint32_t firstVifTriggerIndex = extraTriggers.size();
    প্রতি (ধ্রুব auto& entry : virtIfaceTriggers.m_triggers) {
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
    std::unordered_map<ধ্রুব AstVarScope*, Ast_Sen_বৃক্ষ*> inp2changedp;
    std::vector<Ast_Sen_বৃক্ষ*> icoChangeSenTreeps;
    যদি (v3Global.opt.fIcoChangeDetect().isTrue()) {
        File_সারি* ধ্রুব flp = netlistp->fileline();
        AstScope* ধ্রুব scopep = netlistp->শীর্ষ_Scopep()->scopep();
        প্রতি (AstVarScope* vscp = scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            // Only for top level ports, assume outputs don't change externally
            যদি (!vscp->varp()->isPrimaryInish()) অনুবর্ত;
            // Don't do if written by the design - wouldn't update the change detect 'prev' value
            যদি (vscp->varp()->icoMaybeWritten()) অনুবর্ত;
            // Don't do if forceable, as we can't see the actual value - this is belt and braces
            যদি (vscp->varp()->isForced()) অনুবর্ত;
            // Can't handle unpacked arrays (they have special types when primary input)
            যদি (VN_IS(vscp->dtypep()->skipRefp(), UnpackArrayDType)) অনুবর্ত;
            // Similarly to arrays, can't handle SystemC types
            যদি (vscp->varp()->isSc()) অনুবর্ত;
            // Create a sen tree triggered when this input changes
            Ast_Sen_বৃক্ষ*& senTreepr = inp2changedp[vscp];
            UASSERT_OBJ(!senTreepr, vscp, "Duplicate input change detect trigger");
            AstVarRef* ধ্রুব refp = নূতন AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* ধ্রুব senItemp = নূতন AstSenItem{flp, VEdgeType::ET_CHANGED, refp};
            senTreepr = নূতন Ast_Sen_বৃক্ষ{flp, senItemp};
            icoChangeSenTreeps.ঠেলো_back(senTreepr);
        }
    }
    V3Stats::addStat("Scheduling, 'ico' change detect triggers", icoChangeSenTreeps.size());

    // Gather the relevant sensitivity expressions and create the trigger kit
    std::vector<ধ্রুব Ast_Sen_বৃক্ষ*> senTreeps = getSenTreesUsedBy({&logic});
    senTreeps.ঢোকাও(senTreeps.end(), icoChangeSenTreeps.begin(), icoChangeSenTreeps.end());
    ধ্রুব TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_রাশিমালা_Builder, {},
                                                  senTreeps, "ico", extraTriggers, অসত্য, অসত্য);
    std::ignore = sen_রাশিমালা_Builder.getAndClearResults();

    যদি (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, initFuncp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);

    // Add trigger indices
    addTriggerIndices(logic, trigKit, অসত্য);

    // The 'first iteration' trigger for top level inputs
    ধ্রুব uint32_t firstIterTrigger = trigKit.extraTriggerIndex(firstIterationTrigger);

    ধ্রুব auto& vifVscpToTrigIco
        = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);

    // Create and Order the body function
    AstCFunc* ধ্রুব icoFuncp = V3Order::order(
        {&logic}, trigKit, trigKit.vscp(), cgRefBindings, "ico", অসত্য, অসত্য,
        [&](ধ্রুব AstVarScope* vscp, std::set<uint32_t>& triggers) {
            AstVar* ধ্রুব varp = vscp->varp();
            // If it has an explicit change detect trigger, use that,
            // otherwise fall back to using the 'first iteration' trigger
            auto it = inp2changedp.find(vscp);
            যদি (it != inp2changedp.end()) {
                triggers.ঢোকাও(trigKit.senItem2TrigIdx(it->second->sensesp()));
            } অন্যথা যদি (varp->isPrimaryInish() || varp->isSigUserRWPublic() || varp->sampled()) {
                triggers.ঢোকাও(firstIterTrigger);
            }
            // Add other triggers
            যদি (varp->isWrittenByDpi())
                triggers.ঢোকাও(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
            যদি (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
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
            Ast_গ্রন্থি_Stmt* ধ্রুব stmtp = trigKit.newCompBaseCall();
            যদি (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), সত্য));
            প্রত্যাগম stmtp;
        }(),
        // Work statements: Invoke the 'ico' function
        util::callVoidFunc(icoFuncp));

    // Release temporary input change detect SenTrees
    প্রতি (Ast_Sen_বৃক্ষ* ধ্রুব senTreep : icoChangeSenTreeps)
        VL_DO_DANGLING(senTreep->delete_বৃক্ষ(), senTreep);
    icoChangeSenTreeps.clear();
}

//============================================================================
// EvalKit groups items that have to be passed to createEval() for a given eval region

সংরচনা EvalKit final {
    // The AstVarScope representing the region's trigger vector
    AstVarScope* ধ্রুব m_vscp = nullptr;
    // The AstCFunc that evaluates the region's logic
    AstCFunc* ধ্রুব m_funcp = nullptr;
    // Is this kit used/required?
    তার্কিক empty() ধ্রুব { প্রত্যাগম !m_funcp; }
};

//============================================================================
// Create the evaluation function of each region of a time step

শূন্য createEval(AstNetlist* netlistp,  //
                ধ্রুব TriggerKit& trigKit,  //
                ধ্রুব EvalKit& actKit,  //
                ধ্রুব EvalKit& nbaKit,  //
                ধ্রুব EvalKit& obsKit,  //
                ধ্রুব EvalKit& reactKit,  //
                TimingKit& timingKit  //
) {
    File_সারি* ধ্রুব flp = netlistp->fileline();

    // Grab the delay scheduler variable, if any
    AstVarScope* ধ্রুব delaySchedVscp = timingKit.getDelayScheduler(netlistp);

    // 'createResume' consumes the contents that 'createReady' needs, so do the right order
    AstCCall* ধ্রুব timingReadyp = timingKit.createReady(trigKit.vscp());
    AstCCall* ধ্রুব timingResumep = timingKit.createResume(trigKit.vscp());

    // Create the 'act' region
    createEvalRegion(  //
        netlistp, VEval::ACT, 0, trigKit,
        // Use trigger
        actKit.m_vscp, nullptr,
        // Prep statements
        [&]() {
            // Compute the current 'act' triggers - the NBA triggers are the latched value
            Ast_গ্রন্থি_Stmt* stmtsp = trigKit.newCompBaseCall();
            Ast_গ্রন্থি_Stmt* ধ্রুব dumpp
                = stmtsp ? trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), সত্য) : nullptr;
            // Mark as ready for triggered awaits
            যদি (timingReadyp) stmtsp = Ast_গ্রন্থি::addNext(stmtsp, timingReadyp->makeStmt());
            যদি (AstVarScope* ধ্রুব vscAccp = trigKit.vscAccp()) {
                stmtsp = Ast_গ্রন্থি::addNext(stmtsp, trigKit.newOrIntoCall(actKit.m_vscp, vscAccp));
            }
            stmtsp = Ast_গ্রন্থি::addNext(stmtsp, trigKit.newCompExtCall(nbaKit.m_vscp));
            stmtsp = Ast_গ্রন্থি::addNext(stmtsp, dumpp);
            // Latch the 'act' triggers under the 'nba' triggers
            stmtsp = Ast_গ্রন্থি::addNext(stmtsp, trigKit.newOrIntoCall(nbaKit.m_vscp, actKit.m_vscp));
            //
            প্রত্যাগম stmtsp;
        }(),
        // Work statements
        [&]() {
            Ast_গ্রন্থি_Stmt* workp = nullptr;
            যদি (AstVarScope* ধ্রুব actAccp = trigKit.vscAccp()) {
                AstCMethodHard* ধ্রুব cCallp = নূতন AstCMethodHard{
                    flp, নূতন AstVarRef{flp, actAccp, VAccess::WRITE}, VCMethod::UNPACKED_FILL,
                    নূতন AstConst{flp, AstConst::Unsized64{}, 0}};
                cCallp->dtypeSetVoid();
                workp = Ast_গ্রন্থি::addNext(workp, cCallp->makeStmt());
            }
            // Resume triggered timing schedulers
            যদি (timingResumep) workp = Ast_গ্রন্থি::addNext(workp, timingResumep->makeStmt());
            // Invoke the 'act' function
            workp = Ast_গ্রন্থি::addNext(workp, util::callVoidFunc(actKit.m_funcp));
            //
            প্রত্যাগম workp;
        }());

    // Create the 'inact' region
    createEvalRegion(  //
        netlistp, VEval::INACT, 0, trigKit,
        // Use explicit condition
        nullptr,
        [&]() -> Ast_গ্রন্থি_রাশিমালা* {
            যদি (!delaySchedVscp) প্রত্যাগম nullptr;
            // Run if any zero delays are pending
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব callp
                = নূতন AstCMethodHard{flp, নূতন AstVarRef{flp, delaySchedVscp, VAccess::READ},
                                     VCMethod::SCHED_AWAITING_ZERO_DELAY};
            callp->dtypeSetBit();
            প্রত্যাগম callp;
        }(),
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_গ্রন্থি_Stmt* {
            যদি (!delaySchedVscp) {
                // Nothing to do if there are no delays at all in the design
                প্রত্যাগম nullptr;
            } অন্যথা যদি (v3Global.usesZeroDelay()) {
                // Resume processes watiting for #0 delay
                AstCMethodHard* ধ্রুব callp = নূতন AstCMethodHard{
                    flp, নূতন AstVarRef{flp, delaySchedVscp, VAccess::READWRITE},
                    VCMethod::SCHED_RESUME_ZERO_DELAY};
                callp->dtypeSetVoid();
                প্রত্যাগম callp->makeStmt();
            } অন্যথা {
                // Assumption was that the design doesn't use #0 delays.
                // Die at run-time if it does.
                AstCStmt* ধ্রুব stmtp = নূতন AstCStmt{flp};
                ধ্রুব File_সারি* ধ্রুব locp = netlistp->শীর্ষ_Modulep()->fileline();
                ধ্রুব std::string& file = VIdProtect::protect(locp->filename());
                ধ্রুব std::string& সারি = std::to_string(locp->lineno());
                stmtp->add("VL_FATAL_MT(\"" + V3OutFormatter::quoteNameControls(file) + "\", "
                           + সারি
                           + ", \"\", \"ZERODLY: Design Verilated with '--no-sched-zero-delay', "
                           + "but #0 delay executed at runtime\");");
                প্রত্যাগম stmtp;
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
            Ast_গ্রন্থি_Stmt* workp = nullptr;
            // Latch the 'nba' trigger flags under the following region's trigger flags
            যদি (!obsKit.empty()) {
                workp = trigKit.newOrIntoCall(obsKit.m_vscp, nbaKit.m_vscp);
            } অন্যথা যদি (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, nbaKit.m_vscp);
            }
            // Invoke the 'nba' function
            workp = Ast_গ্রন্থি::addNext(workp, util::callVoidFunc(nbaKit.m_funcp));
            // Clear the 'nba' triggers
            workp = Ast_গ্রন্থি::addNext(workp, trigKit.newClearCall(nbaKit.m_vscp));
            //
            প্রত্যাগম workp;
        }(),
        // Extra work (not conditional on having had a fired trigger)
        [&](AstVarScope* continuep) -> Ast_গ্রন্থি_Stmt* {
            // Check if any dynamic NBAs are pending, if there are any in the design
            যদি (!netlistp->nbaEventp()) প্রত্যাগম nullptr;
            AstVarScope* ধ্রুব nbaEventp = netlistp->nbaEventp();
            AstVarScope* ধ্রুব nbaEventTriggerp = netlistp->nbaEventTriggerp();
            UASSERT(nbaEventTriggerp, "NBA event trigger var should exist");
            netlistp->nbaEventp(nullptr);
            netlistp->nbaEventTriggerp(nullptr);

            // If a dynamic NBA is pending, clear the pending flag and fire the ready event
            AstIf* ধ্রুব ifp = নূতন AstIf{flp, নূতন AstVarRef{flp, nbaEventTriggerp, VAccess::READ}};
            ifp->addThensp(util::setVar(continuep, 1));
            ifp->addThensp(util::setVar(nbaEventTriggerp, 0));
            AstCMethodHard* ধ্রুব firep = নূতন AstCMethodHard{
                flp, নূতন AstVarRef{flp, nbaEventp, VAccess::WRITE}, VCMethod::EVENT_FIRE};
            firep->dtypeSetVoid();
            ifp->addThensp(firep->makeStmt());
            প্রত্যাগম ifp;
        });

    // Create the 'obs' region
    createEvalRegion(  //
        netlistp, VEval::OBS, 0, trigKit,
        // Use trigger
        obsKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_গ্রন্থি_Stmt* {
            যদি (obsKit.empty()) প্রত্যাগম nullptr;
            Ast_গ্রন্থি_Stmt* workp = nullptr;
            // Latch the Observed trigger flags under the Reactive trigger flags
            যদি (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, obsKit.m_vscp);
            }
            // Invoke the 'obs' function
            workp = Ast_গ্রন্থি::addNext(workp, util::callVoidFunc(obsKit.m_funcp));
            // Clear the 'obs' triggers
            workp = Ast_গ্রন্থি::addNext(workp, trigKit.newClearCall(obsKit.m_vscp));
            //
            প্রত্যাগম workp;
        }());

    // Create the 'react' region
    createEvalRegion(  //
        netlistp, VEval::REACT, 0, trigKit,
        // Use trigger
        reactKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_গ্রন্থি_Stmt* {
            যদি (reactKit.empty()) প্রত্যাগম nullptr;
            // Invoke the 'react' function
            Ast_গ্রন্থি_Stmt* workp = util::callVoidFunc(reactKit.m_funcp);
            // Clear the 'react' triggers
            workp = Ast_গ্রন্থি::addNext(workp, trigKit.newClearCall(reactKit.m_vscp));
            প্রত্যাগম workp;
        }());
}

}  // namespace

//============================================================================
// Helper that builds virtual interface trigger sentrees

VirtIfaceTriggers::VscpTrigMap VirtIfaceTriggers::makeVscpToTrigMap(ধ্রুব TriggerKit& trigKit,
                                                                    uint32_t firstIndex) ধ্রুব {
    VscpTrigMap map;
    uint32_t index = firstIndex;
    প্রতি (ধ্রুব auto& entry : m_triggers) {
        map.emplace(entry.m_vscp, trigKit.extraTriggerIndex(index));
        ++index;
    }
    প্রত্যাগম map;
}

//============================================================================
// Top level entry-point to scheduling

শূন্য schedule(AstNetlist* netlistp) {
    ধ্রুব auto addSizeStat = [](ধ্রুব string& name, ধ্রুব LogicByScope& lbs) {
        uint64_t size = 0;
        lbs.foreachLogic([&](Ast_গ্রন্থি* nodep) { size += nodep->গ্রন্থি_গুনতি(); });
        V3Stats::addStat("Scheduling, " + name, size);
    };

    // Step 2: Prepare external domains for timing and virtual interfaces
    // Create extra triggers for virtual interfaces
    ধ্রুব auto& virtIfaceTriggers = makeVirtIfaceTriggers(netlistp);
    // Resolve what covergroup reference formal arguments are bound to, which is visible here
    // but not from V3Order, where the reads through them must be modeled
    ধ্রুব CovergroupRefBindings& cgRefBindings = makeCovergroupRefBindings(netlistp);
    // Prepare timing-related logic and external domains
    TimingKit timingKit = prepareTiming(netlistp);

    // Step 3: Gather and classify all logic in the design
    LogicClasses logicClasses = gatherLogicClasses(netlistp);

    যদি (v3Global.opt.stats()) {
        V3Stats::statsStage("sched-gather");
        addSizeStat("size of class: static", logicClasses.m_static);
        addSizeStat("size of class: initial", logicClasses.m_initial);
        addSizeStat("size of class: final", logicClasses.m_final);
    }

    // Step 4: Schedule static, initial and final logic classes in source order
    AstCFunc* ধ্রুব staticp = createStatic(netlistp, logicClasses);
    যদি (v3Global.opt.stats()) V3Stats::statsStage("sched-static");

    createInitial(netlistp, logicClasses);
    যদি (v3Global.opt.stats()) V3Stats::statsStage("sched-initial");

    createFinal(netlistp, logicClasses);
    যদি (v3Global.opt.stats()) V3Stats::statsStage("sched-final");

    // Step 5: Break combinational cycles by introducing hybrid logic
    // Note: breakCycles also removes corresponding logic from logicClasses.m_comb;
    logicClasses.m_hybrid = breakCycles(netlistp, logicClasses.m_comb);
    যদি (v3Global.opt.stats()) {
        addSizeStat("size of class: clocked", logicClasses.m_clocked);
        addSizeStat("size of class: combinational", logicClasses.m_comb);
        addSizeStat("size of class: hybrid", logicClasses.m_hybrid);
        V3Stats::statsStage("sched-break-cycles");
    }

    // We pass around a single SenExprBuilder instance, as we only need one set of 'prev' variables
    // for edge/change detection in sensitivity expressions, which this keeps track of.
    Ast_শীর্ষ_Scope* ধ্রুব শীর্ষ_Scopep = netlistp->শীর্ষ_Scopep();
    AstScope* ধ্রুব scopeTopp = শীর্ষ_Scopep->scopep();
    Sen_রাশিমালা_Builder sen_রাশিমালা_Builder{scopeTopp};

    // Step 6: Create 'settle' region that restores the combinational invariant
    createSettle(netlistp, staticp, sen_রাশিমালা_Builder, logicClasses, cgRefBindings);
    যদি (v3Global.opt.stats()) V3Stats::statsStage("sched-settle");

    // Step 7: Partition the clocked and combinational (including hybrid) logic into pre/act/nba.
    // All clocks (signals referenced in an AstSenTree) generated via a blocking assignment
    // (including combinationally generated signals) are computed within the act region.
    LogicRegions logicRegions
        = partition(logicClasses.m_clocked, logicClasses.m_comb, logicClasses.m_hybrid);
    logicRegions.m_obs = logicClasses.m_observed;
    logicRegions.m_react = logicClasses.m_reactive;
    যদি (v3Global.opt.stats()) {
        addSizeStat("size of region: Active Pre", logicRegions.m_pre);
        addSizeStat("size of region: Active", logicRegions.m_act);
        addSizeStat("size of region: NBA", logicRegions.m_nba);
        addSizeStat("size of region: Observed", logicRegions.m_obs);
        addSizeStat("size of region: Reactive", logicRegions.m_react);
        V3Stats::statsStage("sched-partition");
    }

    // Step 8: Replicate combinational logic
    LogicReplicas logicReplicas = replicateLogic(logicRegions);
    যদি (v3Global.opt.stats()) {
        addSizeStat("size of replicated logic: Input", logicReplicas.m_ico);
        addSizeStat("size of replicated logic: Active", logicReplicas.m_act);
        addSizeStat("size of replicated logic: NBA", logicReplicas.m_nba);
        addSizeStat("size of replicated logic: Observed", logicReplicas.m_obs);
        addSizeStat("size of replicated logic: Reactive", logicReplicas.m_react);
        V3Stats::statsStage("sched-replicate");
    }

    // Step 9: Create the input combinational logic
    createIcoRegion(netlistp, staticp, sen_রাশিমালা_Builder, logicReplicas.m_ico, virtIfaceTriggers,
                    cgRefBindings);
    যদি (v3Global.opt.stats()) V3Stats::statsStage("sched-create-ico");

    // Step 10: Create the triggers
    AstVarScope* ধ্রুব dpiExportTriggerVscp = netlistp->dpiExportTriggerp();
    netlistp->dpiExportTriggerp(nullptr);  // Finished with this here

    // We may have an extra trigger for variable updated in DPI exports
    TriggerKit::ExtraTriggers extraTriggers;
    ধ্রুব uint32_t dpiExportTriggerIndex = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    ধ্রুব uint32_t firstVifTriggerIndex = extraTriggers.size();
    প্রতি (ধ্রুব auto& entry : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + entry.m_ifacep->name() + "."
                               + entry.m_memberp->name());
    }

    ধ্রুব auto& preTreeps = getSenTreesUsedBy({&logicRegions.m_pre});
    ধ্রুব auto& senTreeps = getSenTreesUsedBy({&logicRegions.m_act,  //
                                               &logicRegions.m_nba,  //
                                               &logicRegions.m_obs,  //
                                               &logicRegions.m_react,  //
                                               &timingKit.m_lbs});
    ধ্রুব TriggerKit trigKit
        = TriggerKit::create(netlistp, staticp, sen_রাশিমালা_Builder, preTreeps, senTreeps, "act",
                             extraTriggers, অসত্য, v3Global.usesTiming());

    // Add post updates from the timing kit
    যদি (timingKit.m_postUpdates) trigKit.compBasep()->addStmtsp(timingKit.m_postUpdates);

    যদি (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, staticp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);
    যদি (v3Global.opt.stats()) V3Stats::statsStage("sched-create-triggers");

    // Note: Experiments so far show that running the Act (or Ico) regions on
    // multiple threads is always a net loss, so only use multi-threading for
    // NBA for now. This can be revised if evidence is available that it would
    // be beneficial

    // Step 11: Create the 'act' region evaluation function

    // Add the trigger indices of the input logic
    addTriggerIndices(logicRegions.m_pre, trigKit, সত্য);
    addTriggerIndices(logicRegions.m_act, trigKit, অসত্য);
    addTriggerIndices(logicReplicas.m_act, trigKit, অসত্য);
    addTriggerIndices(timingKit.m_lbs, trigKit, অসত্য);

    // External triggers of variables. Trigger indices are the same in all regions, so share.
    ধ্রুব std::map<ধ্রুব AstVarScope*, std::vector<uint32_t>> timingTriggers
        = timingKit.externalTriggerIndices(trigKit);
    ধ্রুব auto& vifVscpToTrig = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);
    ধ্রুব V3Order::ExternalTriggersProvider externalTriggers
        = [&](ধ্রুব AstVarScope* vscp, std::set<uint32_t>& triggers) {
              auto it = timingTriggers.find(vscp);
              যদি (it != timingTriggers.end())
                  triggers.ঢোকাও(it->second.begin(), it->second.end());
              যদি (vscp->varp()->isWrittenByDpi()) {
                  triggers.ঢোকাও(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
              }
              যদি (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                  addTriggeredIface(vscp, vifVscpToTrig, virtIfaceTriggers, triggers);
              }
          };

    AstCFunc* ধ্রুব actFuncp
        = V3Order::order({&logicRegions.m_pre, &logicRegions.m_act, &logicReplicas.m_act}, trigKit,
                         trigKit.vscp(), cgRefBindings, "act", অসত্য, অসত্য, externalTriggers);
    util::splitCheck(actFuncp);
    যদি (v3Global.opt.stats()) V3Stats::statsStage("sched-create-act");

    ধ্রুব EvalKit actKit{trigKit.vscp(), actFuncp};

    // Orders a region's logic and creates the region eval function
    ধ্রুব auto order = [&](ধ্রুব std::string& name,
                           ধ্রুব std::vector<V3Sched::LogicByScope*>& logic) -> EvalKit {
        UINFO(2, "Scheduling " << name << " #logic = " << logic.size());
        AstVarScope* ধ্রুব trigVscp = trigKit.newTrigVec(name);
        // Add the trigger indices of the input logic
        প্রতি (LogicByScope* lbs : logic) addTriggerIndices(*lbs, trigKit, অসত্য);

        AstCFunc* ধ্রুব funcp
            = V3Order::order(logic, trigKit, trigVscp, cgRefBindings, name,
                             name == "nba" && v3Global.opt.mtasks(), অসত্য, externalTriggers);

        প্রত্যাগম {trigVscp, funcp};
    };

    // Step 12: Create the 'nba' region evaluation function
    ধ্রুব EvalKit nbaKit = order("nba", {&logicRegions.m_nba, &logicReplicas.m_nba});
    util::splitCheck(nbaKit.m_funcp);
    যদি (v3Global.opt.stats()) V3Stats::statsStage("sched-create-nba");

    // Orders a region's logic and creates the region eval function (only if there is any logic in
    // the region)
    ধ্রুব auto orderIfNonEmpty
        = [&](ধ্রুব std::string& name, ধ্রুব std::vector<LogicByScope*>& logic) -> EvalKit {
        যদি (logic[0]->empty())
            প্রত্যাগম {};  // if region is empty, replica is supposed to be empty as well
        ধ্রুব auto& kit = order(name, logic);
        যদি (v3Global.opt.stats()) V3Stats::statsStage("sched-create-" + name);
        প্রত্যাগম kit;
    };

    // Step 13: Create the 'obs' region evaluation function
    ধ্রুব EvalKit obsKit = orderIfNonEmpty("obs", {&logicRegions.m_obs, &logicReplicas.m_obs});

    // Step 14: Create the 'react' region evaluation function
    ধ্রুব EvalKit reactKit
        = orderIfNonEmpty("react", {&logicRegions.m_react, &logicReplicas.m_react});

    // Step 15: Create the 'postponed' region evaluation function
    createPostponed(netlistp, logicClasses);

    // Step 16: Populate the eval entry point function of each region of a time step
    createEval(netlistp, trigKit, actKit, nbaKit, obsKit, reactKit, timingKit);

    // Step 17: Add neccessary evaluation before awaits
    যদি (AstCCall* ধ্রুব readyp = timingKit.createReady(trigKit.vscp())) {
        staticp->addStmtsp(readyp->makeStmt());
        beforeTrigVisitor(netlistp, sen_রাশিমালা_Builder, trigKit);
    } অন্যথা {
        // beforeTrigVisitor clears Sentree pointers in AstCAwaits (as these sentrees will get
        // deleted later) if there was no need to call it, SenTrees have to be cleaned manually
        netlistp->foreach([](AstCAwait* ধ্রুব cAwaitp) { cAwaitp->clearSentreep(); });
    }
    যদি (AstVarScope* ধ্রুব trigAccp = trigKit.vscAccp()) {
        // Copy trigger vector to accumulator at the end of static initialziation so,
        // triggers fired during initialization persist to the first resume.
        ধ্রুব AstUnpackArrayDType* ধ্রুব trigAccDTypep
            = VN_AS(trigAccp->dtypep(), UnpackArrayDType);
        UASSERT_OBJ(
            trigAccDTypep->ডান() == 0, trigAccp,
            "Expected that trigger vector and accumulator start elements enumeration from 0");
        UASSERT_OBJ(trigAccDTypep->বাম() >= 0, trigAccp,
                    "Expected that trigger vector and accumulator has no negative indexes");
        File_সারি* ধ্রুব flp = trigAccp->fileline();
        AstVarScope* ধ্রুব vscp = netlistp->শীর্ষ_Scopep()->createTemp("__Vi", 32);
        AstLoop* ধ্রুব loopp = নূতন AstLoop{flp};
        loopp->addStmtsp(
            নূতন AstAssign{flp,
                          নূতন AstArraySel{flp, নূতন AstVarRef{flp, trigAccp, VAccess::WRITE},
                                          নূতন AstVarRef{flp, vscp, VAccess::READ}},
                          নূতন AstArraySel{flp, নূতন AstVarRef{flp, actKit.m_vscp, VAccess::READ},
                                          নূতন AstVarRef{flp, vscp, VAccess::READ}}});
        loopp->addStmtsp(util::incrementVar(vscp));
        loopp->addStmtsp(নূতন AstLoopTest{
            flp, loopp,
            নূতন AstLte{flp, নূতন AstVarRef{flp, vscp, VAccess::READ},
                       নূতন AstConst{flp, AstConst::Widthed_মান{}, 32,
                                    static_cast<uint32_t>(trigAccDTypep->বাম())}}});
        staticp->addStmtsp(loopp);
    }

    // Step 18: Clean up
    netlistp->clearStlFirstIterationp();

    যদি (v3Global.opt.stats()) {
        // A sample() call resolved to the union over its covergroup's constructions reads more
        // than it can, which orders it against more logic than necessary
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per instance",
                         cgRefBindings.অঙ্ক_Exact_Calls());
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per type",
                         cgRefBindings.অঙ্ক_Union_Calls());
    }

    // Haven't split static initializer yet
    util::splitCheck(staticp);

    // Record eval stats
    netlistp->addEvalStats("sched");

    // Dump
    V3Global::dump_Check_Global_বৃক্ষ("sched", 0, dump_বৃক্ষ_Either_Level() >= 3);
}

}  // namespace V3Sched
