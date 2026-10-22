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

std::vector<ধ্রুবক Ast_Sen_বৃক্ষ*> getSenTreesUsedBy(ধ্রুবক std::vector<ধ্রুবক LogicByScope*>& lbsps) {
    ধ্রুবক VNUser1InUse user1InUse;
    std::vector<ধ্রুবক Ast_Sen_বৃক্ষ*> ফলাফল;
    জন্য (ধ্রুবক LogicByScope* ধ্রুবক lbsp : lbsps) {
        জন্য (ধ্রুবক auto& entry : *lbsp) {
            AstActive* ধ্রুবক activep = entry.m_activep;
            Ast_Sen_বৃক্ষ* ধ্রুবক senTreep = activep->sentreep();
            যদি (senTreep->user1SetOnce()) চালু;
            যদি (senTreep->hasClocked() || senTreep->hasHybrid()) ফলাফল.ঠেলো_back(senTreep);
        }
    }
    ফেরত ফলাফল;
}

শূন্য addTriggerIndices(LogicByScope& lbs, ধ্রুবক TriggerKit& trigKit, যৌক্তিক pre) {
    জন্য (LogicByScope::Entry& entry : lbs) {
        Ast_Sen_বৃক্ষ* ধ্রুবক senTreep = entry.m_activep->sentreep();
        যদি (senTreep->hasCombo()) চালু;
        entry.m_triggers = trigKit.triggerIndices(senTreep, pre);
    }
}

// Add the VIF triggers that a given VarScope should be sensitive to.
// Case 2 (non-virtual interface read): sensitive to that specific VarScope's trigger only
// Case 3 (virtual interface read): sensitive to all member triggers of the same interface type
শূন্য addTriggeredIface(ধ্রুবক AstVarScope* vscp, ধ্রুবক VirtIfaceTriggers::VscpTrigMap& vscpToTrig,
                       ধ্রুবক VirtIfaceTriggers& virtIfaceTriggers, std::set<uint32_t>& triggers) {
    যদি (vscp->varp()->isVirtIface()) {
        // Virtual interface variable -- sensitive to all member triggers of this interface type
        ধ্রুবক AstIface* ধ্রুবক ifacep = VN_AS(vscp->varp()->dtypep(), IfaceRefDType)->ifacep();
        জন্য (ধ্রুবক auto& entry : virtIfaceTriggers.m_triggers) {
            যদি (entry.m_ifacep == ifacep) {
                ধ্রুবক auto it = vscpToTrig.find(entry.m_vscp);
                যদি (it != vscpToTrig.end()) triggers.ঢোকাও(it->second);
            }
        }
    } অন্যথা {
        // Non-virtual interface member -- sensitive to this VarScope's trigger only
        ধ্রুবক auto it = vscpToTrig.find(vscp);
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
    ধ্রুবক TriggerKit& trigKit,  // The trigger kit
    AstVarScope* trigp,  // The trigger vector - may be nullptr if no triggers or using 'condp'
    Ast_গ্রন্থি_রাশিমালা* condp,  // Explicit condition that must be true to run 'phaseWorkp'
    Ast_গ্রন্থি_Stmt* phasePrepp,  // Prep statements run before checking triggers
    Ast_গ্রন্থি_Stmt* phaseWorkp,  // The work to do if anything triggered
    // Extra statements to run after the work, even if no triggers fired. This function is
    // passed a variable, which must be set to true if we must continue and loop again,
    // and must be unmodified otherwise.
    std::function<Ast_গ্রন্থি_Stmt*(AstVarScope*)> phaseExtra = [](AstVarScope*) { ফেরত nullptr; }  //
) {
    UASSERT(!trigp || !condp, "Cannot use both 'trigp' and 'condp' in 'createEvalRegion'");
    UASSERT(!eval.firstIteration() || trigp,
            "Region without triggers cannot need a first iteration flag");

    // All work is under a trigger or condition, so with neither the region has
    // nothing to evaluate, and what we create below reduces to a no-op function.

    ধ্রুবক std::string tag = eval.tag();
    Ast_শীর্ষ_Scope* ধ্রুবক শীর্ষ_Scopep = netlistp->শীর্ষ_Scopep();
    File_সারি* ধ্রুবক flp = netlistp->fileline();

    // Populate the trigger dump entry point function
    যদি (trigp) {
        netlistp->dumpTriggersFuncp(eval)->addStmtsp(trigKit.newDumpCall(trigp, tag, মিথ্যা));
    }

    AstCFunc* ধ্রুবক funcp = netlistp->evalFuncp(eval);

    // A flag is passed from the run-time eval loop if this is the first iteration of the
    // current loop
    যদি (eval.firstIteration()) {
        AstVarScope* ধ্রুবক firstIterArgp = util::newArgument(funcp, netlistp->findBitDType(),
                                                             "firstIteration", VDirection::প্রবেশ);

        // Set the region's 'first iteration' trigger straight from the argument
        funcp->addStmtsp(trigKit.newExtraTriggerAssignment(firstIterArgp, firstIterTrigger));

        // Only 'stl' also needs a module level flag, for design logic that reads
        // it directly (see V3Timing). Those reads can be anywhere in the design,
        // hence module level. Always created, even if nothing reads it.
        // TODO: get rid of this special case
        যদি (eval == VEval::STL) {
            AstVarScope* ধ্রুবক firstIterp = netlistp->stlFirstIterationp();
            firstIterp->varp()->noReset(সত্য);
            firstIterp->varp()->isInternal(সত্য);
            funcp->addStmtsp(নতুন AstAssign{flp, নতুন AstVarRef{flp, firstIterp, VAccess::WRITE},
                                           নতুন AstVarRef{flp, firstIterArgp, VAccess::READ}});
        }
    }

    {
        // Add the preparatory statements
        funcp->addStmtsp(phasePrepp);

        // The execute flag
        AstVarScope* ধ্রুবক executeFlagp = শীর্ষ_Scopep->create_অস্থায়ী("__V" + tag + "Execute", 1);
        executeFlagp->varp()->noReset(সত্য);

        // If there is work in this region, execute it if any triggers fired
        যদি (phaseWorkp) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = নতুন AstVarRef{flp, executeFlagp, VAccess::WRITE};
            // If using explicit condition, that directly determines whether to execute,
            // otherwise check if any triggers are fired
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsp = condp ? condp : trigKit.newAnySetCall(trigp);
            funcp->addStmtsp(নতুন AstAssign{flp, lhsp, rhsp});

            // Add the work
            AstIf* ধ্রুবক ifp = নতুন AstIf{flp, নতুন AstVarRef{flp, executeFlagp, VAccess::READ}};
            ifp->addThensp(phaseWorkp);
            funcp->addStmtsp(ifp);
        }

        // Construct the extra statements
        Ast_গ্রন্থি_Stmt* ধ্রুবক extraWorkp = phaseExtra(executeFlagp);
        যদি (extraWorkp) funcp->addStmtsp(extraWorkp);

        // The function returns true iff it did run work
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক retp
            = phaseWorkp || extraWorkp
                  ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নতুন AstVarRef{flp, executeFlagp, VAccess::READ})
                  : static_cast<Ast_গ্রন্থি_রাশিমালা*>(নতুন AstConst{flp, AstConst::BitFalse{}});
        funcp->addStmtsp(নতুন AstCReturn{flp, retp});
    }
}

//============================================================================
// Collect and classify all logic in the design

LogicClasses gatherLogicClasses(AstNetlist* netlistp) {
    LogicClasses ফলাফল;

    netlistp->foreach([&](AstScope* scopep) {
        scopep->foreach([&](AstActive* activep) {
            Ast_Sen_বৃক্ষ* ধ্রুবক senTreep = activep->sentreep();
            যদি (senTreep->hasStatic()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "static initializer with additional sensitivities");
                ফলাফল.m_static.add(scopep, activep);
            } অন্যথা যদি (senTreep->hasInitial()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'initial' logic with additional sensitivities");
                ফলাফল.m_initial.add(scopep, activep);
            } অন্যথা যদি (senTreep->hasFinal()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'final' logic with additional sensitivities");
                ফলাফল.m_final.add(scopep, activep);
            } অন্যথা যদি (senTreep->hasCombo()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "combinational logic with additional sensitivities");
                যদি (VN_IS(activep->stmtsp(), AlwaysPostponed)) {
                    ফলাফল.m_postponed.add(scopep, activep);
                } অন্যথা {
                    ফলাফল.m_comb.add(scopep, activep);
                }
            } অন্যথা {
                UASSERT_OBJ(senTreep->hasClocked(), activep, "What else could it be?");
                যদি (VN_IS(activep->stmtsp(), AlwaysObserved)) {
                    ফলাফল.m_observed.add(scopep, activep);
                } অন্যথা যদি (VN_IS(activep->stmtsp(), AlwaysReactive)) {
                    ফলাফল.m_reactive.add(scopep, activep);
                } অন্যথা {
                    ফলাফল.m_clocked.add(scopep, activep);
                }
            }
        });
    });

    ফেরত ফলাফল;
}

//============================================================================
// Simple ordering in source order

শূন্য orderSequentially(AstCFunc* funcp, ধ্রুবক LogicByScope& lbs) {
    // Create new subfunc for scope
    ধ্রুবক auto createNewSubFuncp = [&](AstScope* ধ্রুবক scopep) {
        ধ্রুবক string subName{funcp->name() + "__" + scopep->nameDotless()};
        AstCFunc* ধ্রুবক subFuncp = নতুন AstCFunc{scopep->fileline(), subName, scopep};
        subFuncp->isLoose(সত্য);
        subFuncp->isConst(মিথ্যা);
        subFuncp->declPrivate(সত্য);
        subFuncp->slow(funcp->slow());
        scopep->addBlocksp(subFuncp);
        // Call it from the top function
        funcp->addStmtsp(util::callVoidFunc(subFuncp));
        ফেরত subFuncp;
    };
    ধ্রুবক VNUser1InUse user1InUse;  // AstScope -> AstCFunc: the sub-function for the scope
    ধ্রুবক VNUser2InUse user2InUse;  // AstScope -> int: sub-function counter used for names
    জন্য (ধ্রুবক auto& entry : lbs) {
        AstScope* ধ্রুবক scopep = entry.m_scopep;
        AstActive* ধ্রুবক activep = entry.m_activep;
        // Create a sub-function per scope so we can V3Combine them later
        যদি (!scopep->user1p()) scopep->user1p(createNewSubFuncp(scopep));
        // Add statements to sub-function
        জন্য (Ast_গ্রন্থি *logicp = activep->stmtsp(), *nextp; logicp; logicp = nextp) {
            auto* subFuncp = VN_AS(scopep->user1p(), CFunc);
            nextp = logicp->nextp();
            যদি (Ast_গ্রন্থি_Procedure* ধ্রুবক procp = VN_CAST(logicp, গ্রন্থি_Procedure)) {
                যদি (Ast_গ্রন্থি* bodyp = procp->stmtsp()) {
                    bodyp->unlink_Fr_Back_With_পরবর্তী();
                    // If the process is suspendable, we need a separate function (a coroutine)
                    যদি (procp->isSuspendable()) {
                        funcp->slow(মিথ্যা);
                        subFuncp = createNewSubFuncp(scopep);
                        subFuncp->name(subFuncp->name() + "__Vtiming__"
                                       + cvtToStr(scopep->user2Inc()));
                        subFuncp->rtnType("VlCoroutine");
                        যদি (VN_IS(procp, Always)) {
                            subFuncp->slow(মিথ্যা);
                            File_সারি* ধ্রুবক flp = procp->fileline();
                            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক condp = নতুন AstCExpr{
                                flp, "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish())", 1};
                            AstLoop* ধ্রুবক loopp = নতুন AstLoop{flp};
                            loopp->addStmtsp(নতুন AstLoopTest{flp, loopp, condp});
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
    জন্য (ধ্রুবক auto& entry : lbs) {
        AstScope* ধ্রুবক scopep = entry.m_scopep;
        যদি (AstCFunc* ধ্রুবক subFuncp = VN_AS(scopep->user1p(), CFunc)) {
            util::splitCheck(subFuncp);
            scopep->user1p(nullptr);
        }
    }
}

//============================================================================
// Create simply ordered functions

AstCFunc* createStatic(AstNetlist* netlistp, ধ্রুবক LogicClasses& logicClasses) {
    AstCFunc* ধ্রুবক funcp = netlistp->evalFuncp(VEval::STATIC);

    ধ্রুবক LogicByScope& orig = logicClasses.m_static;
    যদি (orig.আকার_2() <= 1) {
        orderSequentially(funcp, orig);
        ফেরত funcp;
    }

    // Level-based module sorting can reorder packages so that an importing
    // package runs before the imported one.  Re-sort package entries by source
    // file position to restore compilation order (IEEE 1800-2023 26.3).
    std::vector<আকার_t> indices(orig.আকার_2());
    জন্য (আকার_t i = 0; i < orig.আকার_2(); ++i) indices[i] = i;
    std::stable_sort(indices.begin(), indices.end(), [&](আকার_t a, আকার_t b) {
        ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক modA = orig[a].m_scopep->modp();
        ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক modB = orig[b].m_scopep->modp();
        ধ্রুবক যৌক্তিক isPkgA = VN_IS(modA, Package);
        ধ্রুবক যৌক্তিক isPkgB = VN_IS(modB, Package);
        যদি (isPkgA != isPkgB) ফেরত isPkgA;  // Packages before non-packages
        যদি (isPkgA && isPkgB) {
            // Sort packages by source file position (compilation order)
            ফেরত modA->fileline()->operatorCompare(*modB->fileline()) < 0;
        }
        ফেরত মিথ্যা;  // Both non-package: preserve original order
    });
    LogicByScope sorted;
    জন্য (ধ্রুবক আকার_t i : indices) sorted.add(orig[i].m_scopep, orig[i].m_activep);

    orderSequentially(funcp, sorted);
    ফেরত funcp;  // Not splitting yet as it is not final
}

শূন্য createInitial(AstNetlist* netlistp, ধ্রুবক LogicClasses& logicClasses) {
    AstCFunc* ধ্রুবক funcp = netlistp->evalFuncp(VEval::INITIAL);
    orderSequentially(funcp, logicClasses.m_initial);
    util::splitCheck(funcp);
}

শূন্য createPostponed(AstNetlist* netlistp, ধ্রুবক LogicClasses& logicClasses) {
    AstCFunc* ধ্রুবক funcp = netlistp->evalFuncp(VEval::POSTPONED);
    orderSequentially(funcp, logicClasses.m_postponed);
    util::splitCheck(funcp);
}

শূন্য createFinal(AstNetlist* netlistp, ধ্রুবক LogicClasses& logicClasses) {
    AstCFunc* ধ্রুবক funcp = netlistp->evalFuncp(VEval::FINAL);
    orderSequentially(funcp, logicClasses.m_final);
    util::splitCheck(funcp);
}

//============================================================================
// Helper that creates virtual interface value-change triggers

শূন্য addVirtIfaceTriggerAssignments(AstNetlist* netlistp, AstCFunc* initFuncp,
                                    ধ্রুবক VirtIfaceTriggers& virtIfaceTriggers,
                                    uint32_t first_সূচক, ধ্রুবক TriggerKit& trigKit) {
    uint32_t সূচক = first_সূচক;
    জন্য (ধ্রুবক auto& entry : virtIfaceTriggers.m_triggers) {
        trigKit.add_মান_Change_Trigger_Assignment(netlistp, initFuncp, entry.m_vscp, সূচক);
        ++সূচক;
    }
}

// Order the combinational logic to create the 'stl' region
শূন্য createSettle(AstNetlist* netlistp, AstCFunc* ধ্রুবক initFuncp, Sen_রাশিমালা_Builder& sen_রাশিমালা_Bulider,
                  LogicClasses& logicClasses, ধ্রুবক CovergroupRefBindings& cgRefBindings) {
    // Clone, because ordering is destructive, but we still need them for the other regions
    LogicByScope comb = logicClasses.m_comb.clone();
    LogicByScope hybrid = logicClasses.m_hybrid.clone();

    // We have an extra trigger denoting this is the first iteration of the settle loop
    TriggerKit::ExtraTriggers extraTriggers;
    ধ্রুবক uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");

    // Gather the relevant sensitivity expressions and create the trigger kit
    ধ্রুবক auto& senTreeps = getSenTreesUsedBy({&comb, &hybrid});
    ধ্রুবক TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_রাশিমালা_Bulider, {},
                                                  senTreeps, "stl", extraTriggers, সত্য, মিথ্যা);

    // Add trigger indices (comb has none, so only do the hybrid)
    addTriggerIndices(hybrid, trigKit, মিথ্যা);

    // First trigger is for pure combinational triggers (first iteration)
    ধ্রুবক uint32_t প্রবেশ_Changed = trigKit.extra_Trigger_সূচক(firstIterationTrigger);

    // Create and the body function
    AstCFunc* ধ্রুবক stlFuncp = V3Order::order(
        {&comb, &hybrid}, trigKit, trigKit.vscp(), cgRefBindings, "stl", মিথ্যা, সত্য,
        [=](ধ্রুবক AstVarScope*, std::set<uint32_t>& triggers) { triggers.ঢোকাও(প্রবেশ_Changed); });
    util::splitCheck(stlFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::STL, firstIterationTrigger, trigKit,
        // Use trigger
        trigKit.vscp(), nullptr,
        // Explicit condition
        // Prep statements: Compute the current 'stl' triggers
        [&trigKit] {
            Ast_গ্রন্থি_Stmt* ধ্রুবক stmtp = trigKit.newCompBaseCall();
            যদি (stmtp) stmtp->add_পরবর্তী(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), সত্য));
            ফেরত stmtp;
        }(),
        // Work statements: Invoke the 'stl' function
        util::callVoidFunc(stlFuncp));
}

//============================================================================
// Order the replicated combinational logic to create the 'ico' region

শূন্য createIcoRegion(AstNetlist* netlistp, AstCFunc* ধ্রুবক initFuncp,
                     Sen_রাশিমালা_Builder& sen_রাশিমালা_Builder, LogicByScope& logic,
                     ধ্রুবক VirtIfaceTriggers& virtIfaceTriggers,
                     ধ্রুবক CovergroupRefBindings& cgRefBindings) {
    // SystemC only: Any top level inputs feeding a combinational logic must be marked,
    // so we can make them sc_sensitive
    যদি (v3Global.opt.systemC()) {
        logic.foreachLogic([](Ast_গ্রন্থি* logicp) {
            logicp->foreach([](AstVarRef* refp) {
                যদি (refp->access().isWriteOnly()) ফেরত;
                AstVarScope* ধ্রুবক vscp = refp->varScopep();
                যদি (vscp->scopep()->is_শীর্ষ() && vscp->varp()->isNonOutput()) {
                    vscp->varp()->scSensitive(সত্য);
                }
            });
        });
    }

    // We have some extra trigger denoting external conditions
    AstVarScope* ধ্রুবক dpiExportTriggerVscp = netlistp->dpiExportTriggerp();

    TriggerKit::ExtraTriggers extraTriggers;
    ধ্রুবক uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");
    ধ্রুবক uint32_t dpi_Export_Trigger_সূচক = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    ধ্রুবক uint32_t first_Vif_Trigger_সূচক = extraTriggers.আকার_2();
    জন্য (ধ্রুবক auto& entry : virtIfaceTriggers.m_triggers) {
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
    std::unordered_map<ধ্রুবক AstVarScope*, Ast_Sen_বৃক্ষ*> inp2changedp;
    std::vector<Ast_Sen_বৃক্ষ*> icoChangeSenTreeps;
    যদি (v3Global.opt.fIcoChangeDetect().isTrue()) {
        File_সারি* ধ্রুবক flp = netlistp->fileline();
        AstScope* ধ্রুবক scopep = netlistp->শীর্ষ_Scopep()->scopep();
        জন্য (AstVarScope* vscp = scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            // Only for top level ports, assume outputs don't change externally
            যদি (!vscp->varp()->isPrimaryInish()) চালু;
            // Don't do if written by the design - wouldn't update the change detect 'prev' value
            যদি (vscp->varp()->icoMaybeWritten()) চালু;
            // Don't do if forceable, as we can't see the actual value - this is belt and braces
            যদি (vscp->varp()->isForced()) চালু;
            // Can't handle unpacked arrays (they have special types when primary input)
            যদি (VN_IS(vscp->dtypep()->skipRefp(), UnpackArrayDType)) চালু;
            // Similarly to arrays, can't handle SystemC types
            যদি (vscp->varp()->isSc()) চালু;
            // Create a sen tree triggered when this input changes
            Ast_Sen_বৃক্ষ*& senTreepr = inp2changedp[vscp];
            UASSERT_OBJ(!senTreepr, vscp, "Duplicate input change detect trigger");
            AstVarRef* ধ্রুবক refp = নতুন AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* ধ্রুবক senItemp = নতুন AstSenItem{flp, VEdgeType::ET_CHANGED, refp};
            senTreepr = নতুন Ast_Sen_বৃক্ষ{flp, senItemp};
            icoChangeSenTreeps.ঠেলো_back(senTreepr);
        }
    }
    V3Stats::addStat("Scheduling, 'ico' change detect triggers", icoChangeSenTreeps.আকার_2());

    // Gather the relevant sensitivity expressions and create the trigger kit
    std::vector<ধ্রুবক Ast_Sen_বৃক্ষ*> senTreeps = getSenTreesUsedBy({&logic});
    senTreeps.ঢোকাও(senTreeps.end(), icoChangeSenTreeps.begin(), icoChangeSenTreeps.end());
    ধ্রুবক TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_রাশিমালা_Builder, {},
                                                  senTreeps, "ico", extraTriggers, মিথ্যা, মিথ্যা);
    std::ignore = sen_রাশিমালা_Builder.getAndClearResults();

    যদি (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpi_Export_Trigger_সূচক);
    }
    addVirtIfaceTriggerAssignments(netlistp, initFuncp, virtIfaceTriggers, first_Vif_Trigger_সূচক,
                                   trigKit);

    // Add trigger indices
    addTriggerIndices(logic, trigKit, মিথ্যা);

    // The 'first iteration' trigger for top level inputs
    ধ্রুবক uint32_t firstIterTrigger = trigKit.extra_Trigger_সূচক(firstIterationTrigger);

    ধ্রুবক auto& vifVscpToTrigIco
        = virtIfaceTriggers.makeVscpToTrigMap(trigKit, first_Vif_Trigger_সূচক);

    // Create and Order the body function
    AstCFunc* ধ্রুবক icoFuncp = V3Order::order(
        {&logic}, trigKit, trigKit.vscp(), cgRefBindings, "ico", মিথ্যা, মিথ্যা,
        [&](ধ্রুবক AstVarScope* vscp, std::set<uint32_t>& triggers) {
            AstVar* ধ্রুবক varp = vscp->varp();
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
                triggers.ঢোকাও(trigKit.extra_Trigger_সূচক(dpi_Export_Trigger_সূচক));
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
            Ast_গ্রন্থি_Stmt* ধ্রুবক stmtp = trigKit.newCompBaseCall();
            যদি (stmtp) stmtp->add_পরবর্তী(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), সত্য));
            ফেরত stmtp;
        }(),
        // Work statements: Invoke the 'ico' function
        util::callVoidFunc(icoFuncp));

    // Release temporary input change detect SenTrees
    জন্য (Ast_Sen_বৃক্ষ* ধ্রুবক senTreep : icoChangeSenTreeps)
        VL_DO_DANGLING(senTreep->delete_বৃক্ষ(), senTreep);
    icoChangeSenTreeps.clear();
}

//============================================================================
// EvalKit groups items that have to be passed to createEval() for a given eval region

কাঠামো EvalKit final {
    // The AstVarScope representing the region's trigger vector
    AstVarScope* ধ্রুবক m_vscp = nullptr;
    // The AstCFunc that evaluates the region's logic
    AstCFunc* ধ্রুবক m_funcp = nullptr;
    // Is this kit used/required?
    যৌক্তিক empty() ধ্রুবক { ফেরত !m_funcp; }
};

//============================================================================
// Create the evaluation function of each region of a time step

শূন্য createEval(AstNetlist* netlistp,  //
                ধ্রুবক TriggerKit& trigKit,  //
                ধ্রুবক EvalKit& actKit,  //
                ধ্রুবক EvalKit& nbaKit,  //
                ধ্রুবক EvalKit& obsKit,  //
                ধ্রুবক EvalKit& reactKit,  //
                TimingKit& timingKit  //
) {
    File_সারি* ধ্রুবক flp = netlistp->fileline();

    // Grab the delay scheduler variable, if any
    AstVarScope* ধ্রুবক delaySchedVscp = timingKit.getDelayScheduler(netlistp);

    // 'createResume' consumes the contents that 'createReady' needs, so do the right order
    AstCCall* ধ্রুবক timingReadyp = timingKit.createReady(trigKit.vscp());
    AstCCall* ধ্রুবক timingResumep = timingKit.createResume(trigKit.vscp());

    // Create the 'act' region
    createEvalRegion(  //
        netlistp, VEval::ACT, 0, trigKit,
        // Use trigger
        actKit.m_vscp, nullptr,
        // Prep statements
        [&]() {
            // Compute the current 'act' triggers - the NBA triggers are the latched value
            Ast_গ্রন্থি_Stmt* stmtsp = trigKit.newCompBaseCall();
            Ast_গ্রন্থি_Stmt* ধ্রুবক dumpp
                = stmtsp ? trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), সত্য) : nullptr;
            // Mark as ready for triggered awaits
            যদি (timingReadyp) stmtsp = Ast_গ্রন্থি::add_পরবর্তী(stmtsp, timingReadyp->makeStmt());
            যদি (AstVarScope* ধ্রুবক vscAccp = trigKit.vscAccp()) {
                stmtsp = Ast_গ্রন্থি::add_পরবর্তী(stmtsp, trigKit.newOrIntoCall(actKit.m_vscp, vscAccp));
            }
            stmtsp = Ast_গ্রন্থি::add_পরবর্তী(stmtsp, trigKit.newCompExtCall(nbaKit.m_vscp));
            stmtsp = Ast_গ্রন্থি::add_পরবর্তী(stmtsp, dumpp);
            // Latch the 'act' triggers under the 'nba' triggers
            stmtsp = Ast_গ্রন্থি::add_পরবর্তী(stmtsp, trigKit.newOrIntoCall(nbaKit.m_vscp, actKit.m_vscp));
            //
            ফেরত stmtsp;
        }(),
        // Work statements
        [&]() {
            Ast_গ্রন্থি_Stmt* workp = nullptr;
            যদি (AstVarScope* ধ্রুবক actAccp = trigKit.vscAccp()) {
                AstCMethodHard* ধ্রুবক cCallp = নতুন AstCMethodHard{
                    flp, নতুন AstVarRef{flp, actAccp, VAccess::WRITE}, VCMethod::UNPACKED_FILL,
                    নতুন AstConst{flp, AstConst::Unsized64{}, 0}};
                cCallp->dtypeSetVoid();
                workp = Ast_গ্রন্থি::add_পরবর্তী(workp, cCallp->makeStmt());
            }
            // Resume triggered timing schedulers
            যদি (timingResumep) workp = Ast_গ্রন্থি::add_পরবর্তী(workp, timingResumep->makeStmt());
            // Invoke the 'act' function
            workp = Ast_গ্রন্থি::add_পরবর্তী(workp, util::callVoidFunc(actKit.m_funcp));
            //
            ফেরত workp;
        }());

    // Create the 'inact' region
    createEvalRegion(  //
        netlistp, VEval::INACT, 0, trigKit,
        // Use explicit condition
        nullptr,
        [&]() -> Ast_গ্রন্থি_রাশিমালা* {
            যদি (!delaySchedVscp) ফেরত nullptr;
            // Run if any zero delays are pending
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক callp
                = নতুন AstCMethodHard{flp, নতুন AstVarRef{flp, delaySchedVscp, VAccess::READ},
                                     VCMethod::SCHED_AWAITING_ZERO_DELAY};
            callp->dtypeSetBit();
            ফেরত callp;
        }(),
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_গ্রন্থি_Stmt* {
            যদি (!delaySchedVscp) {
                // Nothing to do if there are no delays at all in the design
                ফেরত nullptr;
            } অন্যথা যদি (v3Global.usesZeroDelay()) {
                // Resume processes watiting for #0 delay
                AstCMethodHard* ধ্রুবক callp = নতুন AstCMethodHard{
                    flp, নতুন AstVarRef{flp, delaySchedVscp, VAccess::READWRITE},
                    VCMethod::SCHED_RESUME_ZERO_DELAY};
                callp->dtypeSetVoid();
                ফেরত callp->makeStmt();
            } অন্যথা {
                // Assumption was that the design doesn't use #0 delays.
                // Die at run-time if it does.
                AstCStmt* ধ্রুবক stmtp = নতুন AstCStmt{flp};
                ধ্রুবক File_সারি* ধ্রুবক locp = netlistp->শীর্ষ_Modulep()->fileline();
                ধ্রুবক std::string& file = VIdProtect::protect(locp->filename());
                ধ্রুবক std::string& সারি = std::to_string(locp->lineno());
                stmtp->add("VL_FATAL_MT(\"" + V3OutFormatter::quoteNameControls(file) + "\", "
                           + সারি
                           + ", \"\", \"ZERODLY: Design Verilated with '--no-sched-zero-delay', "
                           + "but #0 delay executed at runtime\");");
                ফেরত stmtp;
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
            workp = Ast_গ্রন্থি::add_পরবর্তী(workp, util::callVoidFunc(nbaKit.m_funcp));
            // Clear the 'nba' triggers
            workp = Ast_গ্রন্থি::add_পরবর্তী(workp, trigKit.newClearCall(nbaKit.m_vscp));
            //
            ফেরত workp;
        }(),
        // Extra work (not conditional on having had a fired trigger)
        [&](AstVarScope* continuep) -> Ast_গ্রন্থি_Stmt* {
            // Check if any dynamic NBAs are pending, if there are any in the design
            যদি (!netlistp->nbaEventp()) ফেরত nullptr;
            AstVarScope* ধ্রুবক nbaEventp = netlistp->nbaEventp();
            AstVarScope* ধ্রুবক nbaEventTriggerp = netlistp->nbaEventTriggerp();
            UASSERT(nbaEventTriggerp, "NBA event trigger var should exist");
            netlistp->nbaEventp(nullptr);
            netlistp->nbaEventTriggerp(nullptr);

            // If a dynamic NBA is pending, clear the pending flag and fire the ready event
            AstIf* ধ্রুবক ifp = নতুন AstIf{flp, নতুন AstVarRef{flp, nbaEventTriggerp, VAccess::READ}};
            ifp->addThensp(util::setVar(continuep, 1));
            ifp->addThensp(util::setVar(nbaEventTriggerp, 0));
            AstCMethodHard* ধ্রুবক firep = নতুন AstCMethodHard{
                flp, নতুন AstVarRef{flp, nbaEventp, VAccess::WRITE}, VCMethod::EVENT_FIRE};
            firep->dtypeSetVoid();
            ifp->addThensp(firep->makeStmt());
            ফেরত ifp;
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
            যদি (obsKit.empty()) ফেরত nullptr;
            Ast_গ্রন্থি_Stmt* workp = nullptr;
            // Latch the Observed trigger flags under the Reactive trigger flags
            যদি (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, obsKit.m_vscp);
            }
            // Invoke the 'obs' function
            workp = Ast_গ্রন্থি::add_পরবর্তী(workp, util::callVoidFunc(obsKit.m_funcp));
            // Clear the 'obs' triggers
            workp = Ast_গ্রন্থি::add_পরবর্তী(workp, trigKit.newClearCall(obsKit.m_vscp));
            //
            ফেরত workp;
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
            যদি (reactKit.empty()) ফেরত nullptr;
            // Invoke the 'react' function
            Ast_গ্রন্থি_Stmt* workp = util::callVoidFunc(reactKit.m_funcp);
            // Clear the 'react' triggers
            workp = Ast_গ্রন্থি::add_পরবর্তী(workp, trigKit.newClearCall(reactKit.m_vscp));
            ফেরত workp;
        }());
}

}  // namespace

//============================================================================
// Helper that builds virtual interface trigger sentrees

VirtIfaceTriggers::VscpTrigMap VirtIfaceTriggers::makeVscpToTrigMap(ধ্রুবক TriggerKit& trigKit,
                                                                    uint32_t first_সূচক) ধ্রুবক {
    VscpTrigMap map;
    uint32_t সূচক = first_সূচক;
    জন্য (ধ্রুবক auto& entry : m_triggers) {
        map.emplace(entry.m_vscp, trigKit.extra_Trigger_সূচক(সূচক));
        ++সূচক;
    }
    ফেরত map;
}

//============================================================================
// Top level entry-point to scheduling

শূন্য schedule(AstNetlist* netlistp) {
    ধ্রুবক auto add_আকার_Stat = [](ধ্রুবক string& name, ধ্রুবক LogicByScope& lbs) {
        uint64_t আকার_2 = 0;
        lbs.foreachLogic([&](Ast_গ্রন্থি* nodep) { আকার_2 += nodep->গ্রন্থি_গুনতি(); });
        V3Stats::addStat("Scheduling, " + name, আকার_2);
    };

    // Step 2: Prepare external domains for timing and virtual interfaces
    // Create extra triggers for virtual interfaces
    ধ্রুবক auto& virtIfaceTriggers = makeVirtIfaceTriggers(netlistp);
    // Resolve what covergroup reference formal arguments are bound to, which is visible here
    // but not from V3Order, where the reads through them must be modeled
    ধ্রুবক CovergroupRefBindings& cgRefBindings = makeCovergroupRefBindings(netlistp);
    // Prepare timing-related logic and external domains
    TimingKit timingKit = prepareTiming(netlistp);

    // Step 3: Gather and classify all logic in the design
    LogicClasses logicClasses = gatherLogicClasses(netlistp);

    যদি (v3Global.opt.stats()) {
        V3Stats::statsStage("sched-gather");
        add_আকার_Stat("size of class: static", logicClasses.m_static);
        add_আকার_Stat("size of class: initial", logicClasses.m_initial);
        add_আকার_Stat("size of class: final", logicClasses.m_final);
    }

    // Step 4: Schedule static, initial and final logic classes in source order
    AstCFunc* ধ্রুবক staticp = createStatic(netlistp, logicClasses);
    যদি (v3Global.opt.stats()) V3Stats::statsStage("sched-static");

    createInitial(netlistp, logicClasses);
    যদি (v3Global.opt.stats()) V3Stats::statsStage("sched-initial");

    createFinal(netlistp, logicClasses);
    যদি (v3Global.opt.stats()) V3Stats::statsStage("sched-final");

    // Step 5: Break combinational cycles by introducing hybrid logic
    // Note: breakCycles also removes corresponding logic from logicClasses.m_comb;
    logicClasses.m_hybrid = breakCycles(netlistp, logicClasses.m_comb);
    যদি (v3Global.opt.stats()) {
        add_আকার_Stat("size of class: clocked", logicClasses.m_clocked);
        add_আকার_Stat("size of class: combinational", logicClasses.m_comb);
        add_আকার_Stat("size of class: hybrid", logicClasses.m_hybrid);
        V3Stats::statsStage("sched-break-cycles");
    }

    // We pass around a single SenExprBuilder instance, as we only need one set of 'prev' variables
    // for edge/change detection in sensitivity expressions, which this keeps track of.
    Ast_শীর্ষ_Scope* ধ্রুবক শীর্ষ_Scopep = netlistp->শীর্ষ_Scopep();
    AstScope* ধ্রুবক scopeTopp = শীর্ষ_Scopep->scopep();
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
        add_আকার_Stat("size of region: Active Pre", logicRegions.m_pre);
        add_আকার_Stat("size of region: Active", logicRegions.m_act);
        add_আকার_Stat("size of region: NBA", logicRegions.m_nba);
        add_আকার_Stat("size of region: Observed", logicRegions.m_obs);
        add_আকার_Stat("size of region: Reactive", logicRegions.m_react);
        V3Stats::statsStage("sched-partition");
    }

    // Step 8: Replicate combinational logic
    LogicReplicas logicReplicas = replicateLogic(logicRegions);
    যদি (v3Global.opt.stats()) {
        add_আকার_Stat("size of replicated logic: Input", logicReplicas.m_ico);
        add_আকার_Stat("size of replicated logic: Active", logicReplicas.m_act);
        add_আকার_Stat("size of replicated logic: NBA", logicReplicas.m_nba);
        add_আকার_Stat("size of replicated logic: Observed", logicReplicas.m_obs);
        add_আকার_Stat("size of replicated logic: Reactive", logicReplicas.m_react);
        V3Stats::statsStage("sched-replicate");
    }

    // Step 9: Create the input combinational logic
    createIcoRegion(netlistp, staticp, sen_রাশিমালা_Builder, logicReplicas.m_ico, virtIfaceTriggers,
                    cgRefBindings);
    যদি (v3Global.opt.stats()) V3Stats::statsStage("sched-create-ico");

    // Step 10: Create the triggers
    AstVarScope* ধ্রুবক dpiExportTriggerVscp = netlistp->dpiExportTriggerp();
    netlistp->dpiExportTriggerp(nullptr);  // Finished with this here

    // We may have an extra trigger for variable updated in DPI exports
    TriggerKit::ExtraTriggers extraTriggers;
    ধ্রুবক uint32_t dpi_Export_Trigger_সূচক = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    ধ্রুবক uint32_t first_Vif_Trigger_সূচক = extraTriggers.আকার_2();
    জন্য (ধ্রুবক auto& entry : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + entry.m_ifacep->name() + "."
                               + entry.m_memberp->name());
    }

    ধ্রুবক auto& preTreeps = getSenTreesUsedBy({&logicRegions.m_pre});
    ধ্রুবক auto& senTreeps = getSenTreesUsedBy({&logicRegions.m_act,  //
                                               &logicRegions.m_nba,  //
                                               &logicRegions.m_obs,  //
                                               &logicRegions.m_react,  //
                                               &timingKit.m_lbs});
    ধ্রুবক TriggerKit trigKit
        = TriggerKit::create(netlistp, staticp, sen_রাশিমালা_Builder, preTreeps, senTreeps, "act",
                             extraTriggers, মিথ্যা, v3Global.usesTiming());

    // Add post updates from the timing kit
    যদি (timingKit.m_postUpdates) trigKit.compBasep()->addStmtsp(timingKit.m_postUpdates);

    যদি (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpi_Export_Trigger_সূচক);
    }
    addVirtIfaceTriggerAssignments(netlistp, staticp, virtIfaceTriggers, first_Vif_Trigger_সূচক,
                                   trigKit);
    যদি (v3Global.opt.stats()) V3Stats::statsStage("sched-create-triggers");

    // Note: Experiments so far show that running the Act (or Ico) regions on
    // multiple threads is always a net loss, so only use multi-threading for
    // NBA for now. This can be revised if evidence is available that it would
    // be beneficial

    // Step 11: Create the 'act' region evaluation function

    // Add the trigger indices of the input logic
    addTriggerIndices(logicRegions.m_pre, trigKit, সত্য);
    addTriggerIndices(logicRegions.m_act, trigKit, মিথ্যা);
    addTriggerIndices(logicReplicas.m_act, trigKit, মিথ্যা);
    addTriggerIndices(timingKit.m_lbs, trigKit, মিথ্যা);

    // External triggers of variables. Trigger indices are the same in all regions, so share.
    ধ্রুবক std::map<ধ্রুবক AstVarScope*, std::vector<uint32_t>> timingTriggers
        = timingKit.externalTriggerIndices(trigKit);
    ধ্রুবক auto& vifVscpToTrig = virtIfaceTriggers.makeVscpToTrigMap(trigKit, first_Vif_Trigger_সূচক);
    ধ্রুবক V3Order::ExternalTriggersProvider externalTriggers
        = [&](ধ্রুবক AstVarScope* vscp, std::set<uint32_t>& triggers) {
              auto it = timingTriggers.find(vscp);
              যদি (it != timingTriggers.end())
                  triggers.ঢোকাও(it->second.begin(), it->second.end());
              যদি (vscp->varp()->isWrittenByDpi()) {
                  triggers.ঢোকাও(trigKit.extra_Trigger_সূচক(dpi_Export_Trigger_সূচক));
              }
              যদি (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                  addTriggeredIface(vscp, vifVscpToTrig, virtIfaceTriggers, triggers);
              }
          };

    AstCFunc* ধ্রুবক actFuncp
        = V3Order::order({&logicRegions.m_pre, &logicRegions.m_act, &logicReplicas.m_act}, trigKit,
                         trigKit.vscp(), cgRefBindings, "act", মিথ্যা, মিথ্যা, externalTriggers);
    util::splitCheck(actFuncp);
    যদি (v3Global.opt.stats()) V3Stats::statsStage("sched-create-act");

    ধ্রুবক EvalKit actKit{trigKit.vscp(), actFuncp};

    // Orders a region's logic and creates the region eval function
    ধ্রুবক auto order = [&](ধ্রুবক std::string& name,
                           ধ্রুবক std::vector<V3Sched::LogicByScope*>& logic) -> EvalKit {
        UINFO(2, "Scheduling " << name << " #logic = " << logic.আকার_2());
        AstVarScope* ধ্রুবক trigVscp = trigKit.newTrigVec(name);
        // Add the trigger indices of the input logic
        জন্য (LogicByScope* lbs : logic) addTriggerIndices(*lbs, trigKit, মিথ্যা);

        AstCFunc* ধ্রুবক funcp
            = V3Order::order(logic, trigKit, trigVscp, cgRefBindings, name,
                             name == "nba" && v3Global.opt.mtasks(), মিথ্যা, externalTriggers);

        ফেরত {trigVscp, funcp};
    };

    // Step 12: Create the 'nba' region evaluation function
    ধ্রুবক EvalKit nbaKit = order("nba", {&logicRegions.m_nba, &logicReplicas.m_nba});
    util::splitCheck(nbaKit.m_funcp);
    যদি (v3Global.opt.stats()) V3Stats::statsStage("sched-create-nba");

    // Orders a region's logic and creates the region eval function (only if there is any logic in
    // the region)
    ধ্রুবক auto orderIfNonEmpty
        = [&](ধ্রুবক std::string& name, ধ্রুবক std::vector<LogicByScope*>& logic) -> EvalKit {
        যদি (logic[0]->empty())
            ফেরত {};  // if region is empty, replica is supposed to be empty as well
        ধ্রুবক auto& kit = order(name, logic);
        যদি (v3Global.opt.stats()) V3Stats::statsStage("sched-create-" + name);
        ফেরত kit;
    };

    // Step 13: Create the 'obs' region evaluation function
    ধ্রুবক EvalKit obsKit = orderIfNonEmpty("obs", {&logicRegions.m_obs, &logicReplicas.m_obs});

    // Step 14: Create the 'react' region evaluation function
    ধ্রুবক EvalKit reactKit
        = orderIfNonEmpty("react", {&logicRegions.m_react, &logicReplicas.m_react});

    // Step 15: Create the 'postponed' region evaluation function
    createPostponed(netlistp, logicClasses);

    // Step 16: Populate the eval entry point function of each region of a time step
    createEval(netlistp, trigKit, actKit, nbaKit, obsKit, reactKit, timingKit);

    // Step 17: Add neccessary evaluation before awaits
    যদি (AstCCall* ধ্রুবক readyp = timingKit.createReady(trigKit.vscp())) {
        staticp->addStmtsp(readyp->makeStmt());
        beforeTrigVisitor(netlistp, sen_রাশিমালা_Builder, trigKit);
    } অন্যথা {
        // beforeTrigVisitor clears Sentree pointers in AstCAwaits (as these sentrees will get
        // deleted later) if there was no need to call it, SenTrees have to be cleaned manually
        netlistp->foreach([](AstCAwait* ধ্রুবক cAwaitp) { cAwaitp->clearSentreep(); });
    }
    যদি (AstVarScope* ধ্রুবক trigAccp = trigKit.vscAccp()) {
        // Copy trigger vector to accumulator at the end of static initialziation so,
        // triggers fired during initialization persist to the first resume.
        ধ্রুবক AstUnpackArrayDType* ধ্রুবক trigAccDTypep
            = VN_AS(trigAccp->dtypep(), UnpackArrayDType);
        UASSERT_OBJ(
            trigAccDTypep->ডান() == 0, trigAccp,
            "Expected that trigger vector and accumulator start elements enumeration from 0");
        UASSERT_OBJ(trigAccDTypep->বাম() >= 0, trigAccp,
                    "Expected that trigger vector and accumulator has no negative indexes");
        File_সারি* ধ্রুবক flp = trigAccp->fileline();
        AstVarScope* ধ্রুবক vscp = netlistp->শীর্ষ_Scopep()->create_অস্থায়ী("__Vi", 32);
        AstLoop* ধ্রুবক loopp = নতুন AstLoop{flp};
        loopp->addStmtsp(
            নতুন AstAssign{flp,
                          নতুন AstArraySel{flp, নতুন AstVarRef{flp, trigAccp, VAccess::WRITE},
                                          নতুন AstVarRef{flp, vscp, VAccess::READ}},
                          নতুন AstArraySel{flp, নতুন AstVarRef{flp, actKit.m_vscp, VAccess::READ},
                                          নতুন AstVarRef{flp, vscp, VAccess::READ}}});
        loopp->addStmtsp(util::incrementVar(vscp));
        loopp->addStmtsp(নতুন AstLoopTest{
            flp, loopp,
            নতুন AstLte{flp, নতুন AstVarRef{flp, vscp, VAccess::READ},
                       নতুন AstConst{flp, AstConst::Widthed_মান{}, 32,
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
