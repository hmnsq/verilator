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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Sched.h"

#شامل "V3Const.h"
#شامل "V3EmitCBase.h"
#شامل "V3EmitV.h"
#شامل "V3Order.h"
#شامل "V3SenExprBuilder.h"
#شامل "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

فضاینام V3Sched {

فضاینام {

//============================================================================
// Utility functions

std::vector<ثابت Ast_Sen_درخت*> getSenTreesUsedBy(ثابت std::vector<ثابت LogicByScope*>& lbsps) {
    ثابت VNUser1InUse user1InUse;
    std::vector<ثابت Ast_Sen_درخت*> result;
    برای (ثابت LogicByScope* ثابت lbsp : lbsps) {
        برای (ثابت auto& entry : *lbsp) {
            AstActive* ثابت activep = entry.m_activep;
            Ast_Sen_درخت* ثابت senTreep = activep->sentreep();
            اگر (senTreep->user1SetOnce()) ادامه;
            اگر (senTreep->hasClocked() || senTreep->hasHybrid()) result.افزودن_back(senTreep);
        }
    }
    بازگشت result;
}

تهی addTriggerIndices(LogicByScope& lbs, ثابت TriggerKit& trigKit, منطقی pre) {
    برای (LogicByScope::Entry& entry : lbs) {
        Ast_Sen_درخت* ثابت senTreep = entry.m_activep->sentreep();
        اگر (senTreep->hasCombo()) ادامه;
        entry.m_triggers = trigKit.triggerIndices(senTreep, pre);
    }
}

// Add the VIF triggers that a given VarScope should be sensitive to.
// Case 2 (non-virtual interface read): sensitive to that specific VarScope's trigger only
// Case 3 (virtual interface read): sensitive to all member triggers of the same interface type
تهی addTriggeredIface(ثابت AstVarScope* vscp, ثابت VirtIfaceTriggers::VscpTrigMap& vscpToTrig,
                       ثابت VirtIfaceTriggers& virtIfaceTriggers, std::set<uint32_t>& triggers) {
    اگر (vscp->varp()->isVirtIface()) {
        // Virtual interface variable -- sensitive to all member triggers of this interface type
        ثابت AstIface* ثابت ifacep = VN_AS(vscp->varp()->dtypep(), IfaceRefDType)->ifacep();
        برای (ثابت auto& entry : virtIfaceTriggers.m_triggers) {
            اگر (entry.m_ifacep == ifacep) {
                ثابت auto it = vscpToTrig.find(entry.m_vscp);
                اگر (it != vscpToTrig.end()) triggers.درج(it->second);
            }
        }
    } وگرنه {
        // Non-virtual interface member -- sensitive to this VarScope's trigger only
        ثابت auto it = vscpToTrig.find(vscp);
        اگر (it != vscpToTrig.end()) triggers.درج(it->second);
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
تهی createEvalRegion(
    AstNetlist* netlistp,  //
    VEval eval,  // The entry point of the current region
    // Index of the region's 'first iteration' extra trigger, if it has one, otherwise ignored
    uint32_t firstIterTrigger,
    ثابت TriggerKit& trigKit,  // The trigger kit
    AstVarScope* trigp,  // The trigger vector - may be nullptr if no triggers or using 'condp'
    Ast_گره_عبارت* condp,  // Explicit condition that must be true to run 'phaseWorkp'
    Ast_گره_Stmt* phasePrepp,  // Prep statements run before checking triggers
    Ast_گره_Stmt* phaseWorkp,  // The work to do if anything triggered
    // Extra statements to run after the work, even if no triggers fired. This function is
    // passed a variable, which must be set to true if we must continue and loop again,
    // and must be unmodified otherwise.
    std::function<Ast_گره_Stmt*(AstVarScope*)> phaseExtra = [](AstVarScope*) { بازگشت nullptr; }  //
) {
    UASSERT(!trigp || !condp, "Cannot use both 'trigp' and 'condp' in 'createEvalRegion'");
    UASSERT(!eval.firstIteration() || trigp,
            "Region without triggers cannot need a first iteration flag");

    // All work is under a trigger or condition, so with neither the region has
    // nothing to evaluate, and what we create below reduces to a no-op function.

    ثابت std::string tag = eval.tag();
    Ast_بالاترین_Scope* ثابت بالاترین_Scopep = netlistp->بالاترین_Scopep();
    File_سطر* ثابت flp = netlistp->fileline();

    // Populate the trigger dump entry point function
    اگر (trigp) {
        netlistp->dumpTriggersFuncp(eval)->addStmtsp(trigKit.newDumpCall(trigp, tag, نادرست));
    }

    AstCFunc* ثابت funcp = netlistp->evalFuncp(eval);

    // A flag is passed from the run-time eval loop if this is the first iteration of the
    // current loop
    اگر (eval.firstIteration()) {
        AstVarScope* ثابت firstIterArgp = util::newArgument(funcp, netlistp->findBitDType(),
                                                             "firstIteration", VDirection::درون_داد);

        // Set the region's 'first iteration' trigger straight from the argument
        funcp->addStmtsp(trigKit.newExtraTriggerAssignment(firstIterArgp, firstIterTrigger));

        // Only 'stl' also needs a module level flag, for design logic that reads
        // it directly (see V3Timing). Those reads can be anywhere in the design,
        // hence module level. Always created, even if nothing reads it.
        // TODO: get rid of this special case
        اگر (eval == VEval::STL) {
            AstVarScope* ثابت firstIterp = netlistp->stlFirstIterationp();
            firstIterp->varp()->noReset(درست);
            firstIterp->varp()->isInternal(درست);
            funcp->addStmtsp(جدید AstAssign{flp, جدید AstVarRef{flp, firstIterp, VAccess::WRITE},
                                           جدید AstVarRef{flp, firstIterArgp, VAccess::READ}});
        }
    }

    {
        // Add the preparatory statements
        funcp->addStmtsp(phasePrepp);

        // The execute flag
        AstVarScope* ثابت executeFlagp = بالاترین_Scopep->createTemp("__V" + tag + "Execute", 1);
        executeFlagp->varp()->noReset(درست);

        // If there is work in this region, execute it if any triggers fired
        اگر (phaseWorkp) {
            Ast_گره_عبارت* ثابت lhsp = جدید AstVarRef{flp, executeFlagp, VAccess::WRITE};
            // If using explicit condition, that directly determines whether to execute,
            // otherwise check if any triggers are fired
            Ast_گره_عبارت* ثابت rhsp = condp ? condp : trigKit.newAnySetCall(trigp);
            funcp->addStmtsp(جدید AstAssign{flp, lhsp, rhsp});

            // Add the work
            AstIf* ثابت ifp = جدید AstIf{flp, جدید AstVarRef{flp, executeFlagp, VAccess::READ}};
            ifp->addThensp(phaseWorkp);
            funcp->addStmtsp(ifp);
        }

        // Construct the extra statements
        Ast_گره_Stmt* ثابت extraWorkp = phaseExtra(executeFlagp);
        اگر (extraWorkp) funcp->addStmtsp(extraWorkp);

        // The function returns true iff it did run work
        Ast_گره_عبارت* ثابت retp
            = phaseWorkp || extraWorkp
                  ? static_cast<Ast_گره_عبارت*>(جدید AstVarRef{flp, executeFlagp, VAccess::READ})
                  : static_cast<Ast_گره_عبارت*>(جدید AstConst{flp, AstConst::BitFalse{}});
        funcp->addStmtsp(جدید AstCReturn{flp, retp});
    }
}

//============================================================================
// Collect and classify all logic in the design

LogicClasses gatherLogicClasses(AstNetlist* netlistp) {
    LogicClasses result;

    netlistp->foreach([&](AstScope* scopep) {
        scopep->foreach([&](AstActive* activep) {
            Ast_Sen_درخت* ثابت senTreep = activep->sentreep();
            اگر (senTreep->hasStatic()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "static initializer with additional sensitivities");
                result.m_static.add(scopep, activep);
            } وگرنه اگر (senTreep->hasInitial()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'initial' logic with additional sensitivities");
                result.m_initial.add(scopep, activep);
            } وگرنه اگر (senTreep->hasFinal()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'final' logic with additional sensitivities");
                result.m_final.add(scopep, activep);
            } وگرنه اگر (senTreep->hasCombo()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "combinational logic with additional sensitivities");
                اگر (VN_IS(activep->stmtsp(), AlwaysPostponed)) {
                    result.m_postponed.add(scopep, activep);
                } وگرنه {
                    result.m_comb.add(scopep, activep);
                }
            } وگرنه {
                UASSERT_OBJ(senTreep->hasClocked(), activep, "What else could it be?");
                اگر (VN_IS(activep->stmtsp(), AlwaysObserved)) {
                    result.m_observed.add(scopep, activep);
                } وگرنه اگر (VN_IS(activep->stmtsp(), AlwaysReactive)) {
                    result.m_reactive.add(scopep, activep);
                } وگرنه {
                    result.m_clocked.add(scopep, activep);
                }
            }
        });
    });

    بازگشت result;
}

//============================================================================
// Simple ordering in source order

تهی orderSequentially(AstCFunc* funcp, ثابت LogicByScope& lbs) {
    // Create new subfunc for scope
    ثابت auto createNewSubFuncp = [&](AstScope* ثابت scopep) {
        ثابت string subName{funcp->name() + "__" + scopep->nameDotless()};
        AstCFunc* ثابت subFuncp = جدید AstCFunc{scopep->fileline(), subName, scopep};
        subFuncp->isLoose(درست);
        subFuncp->isConst(نادرست);
        subFuncp->declPrivate(درست);
        subFuncp->slow(funcp->slow());
        scopep->addBlocksp(subFuncp);
        // Call it from the top function
        funcp->addStmtsp(util::callVoidFunc(subFuncp));
        بازگشت subFuncp;
    };
    ثابت VNUser1InUse user1InUse;  // AstScope -> AstCFunc: the sub-function for the scope
    ثابت VNUser2InUse user2InUse;  // AstScope -> int: sub-function counter used for names
    برای (ثابت auto& entry : lbs) {
        AstScope* ثابت scopep = entry.m_scopep;
        AstActive* ثابت activep = entry.m_activep;
        // Create a sub-function per scope so we can V3Combine them later
        اگر (!scopep->user1p()) scopep->user1p(createNewSubFuncp(scopep));
        // Add statements to sub-function
        برای (Ast_گره *logicp = activep->stmtsp(), *nextp; logicp; logicp = nextp) {
            auto* subFuncp = VN_AS(scopep->user1p(), CFunc);
            nextp = logicp->nextp();
            اگر (Ast_گره_Procedure* ثابت procp = VN_CAST(logicp, گره_Procedure)) {
                اگر (Ast_گره* bodyp = procp->stmtsp()) {
                    bodyp->unlinkFrBackWithNext();
                    // If the process is suspendable, we need a separate function (a coroutine)
                    اگر (procp->isSuspendable()) {
                        funcp->slow(نادرست);
                        subFuncp = createNewSubFuncp(scopep);
                        subFuncp->name(subFuncp->name() + "__Vtiming__"
                                       + cvtToStr(scopep->user2Inc()));
                        subFuncp->rtnType("VlCoroutine");
                        اگر (VN_IS(procp, Always)) {
                            subFuncp->slow(نادرست);
                            File_سطر* ثابت flp = procp->fileline();
                            Ast_گره_عبارت* ثابت condp = جدید AstCExpr{
                                flp, "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish())", 1};
                            AstLoop* ثابت loopp = جدید AstLoop{flp};
                            loopp->addStmtsp(جدید AstLoopTest{flp, loopp, condp});
                            loopp->addStmtsp(bodyp);
                            bodyp = loopp;
                        }
                    }
                    subFuncp->addStmtsp(bodyp);
                    اگر (procp->needProcess()) subFuncp->setNeedProcess();
                    // A coroutine holds a single process, so it is complete here
                    اگر (procp->isSuspendable()) util::splitCheck(subFuncp);
                }
            } وگرنه {
                logicp->unlinkFrBack();
                subFuncp->addStmtsp(logicp);
            }
        }
        اگر (activep->backp()) activep->unlinkFrBack();
        VL_DO_DANGLING(activep->delete_درخت(), activep);
    }
    // Split sub-functions once complete
    برای (ثابت auto& entry : lbs) {
        AstScope* ثابت scopep = entry.m_scopep;
        اگر (AstCFunc* ثابت subFuncp = VN_AS(scopep->user1p(), CFunc)) {
            util::splitCheck(subFuncp);
            scopep->user1p(nullptr);
        }
    }
}

//============================================================================
// Create simply ordered functions

AstCFunc* createStatic(AstNetlist* netlistp, ثابت LogicClasses& logicClasses) {
    AstCFunc* ثابت funcp = netlistp->evalFuncp(VEval::STATIC);

    ثابت LogicByScope& orig = logicClasses.m_static;
    اگر (orig.size() <= 1) {
        orderSequentially(funcp, orig);
        بازگشت funcp;
    }

    // Level-based module sorting can reorder packages so that an importing
    // package runs before the imported one.  Re-sort package entries by source
    // file position to restore compilation order (IEEE 1800-2023 26.3).
    std::vector<size_t> indices(orig.size());
    برای (size_t i = 0; i < orig.size(); ++i) indices[i] = i;
    std::stable_sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        ثابت Ast_گره_Module* ثابت modA = orig[a].m_scopep->modp();
        ثابت Ast_گره_Module* ثابت modB = orig[b].m_scopep->modp();
        ثابت منطقی isPkgA = VN_IS(modA, Package);
        ثابت منطقی isPkgB = VN_IS(modB, Package);
        اگر (isPkgA != isPkgB) بازگشت isPkgA;  // Packages before non-packages
        اگر (isPkgA && isPkgB) {
            // Sort packages by source file position (compilation order)
            بازگشت modA->fileline()->operatorCompare(*modB->fileline()) < 0;
        }
        بازگشت نادرست;  // Both non-package: preserve original order
    });
    LogicByScope sorted;
    برای (ثابت size_t i : indices) sorted.add(orig[i].m_scopep, orig[i].m_activep);

    orderSequentially(funcp, sorted);
    بازگشت funcp;  // Not splitting yet as it is not final
}

تهی createInitial(AstNetlist* netlistp, ثابت LogicClasses& logicClasses) {
    AstCFunc* ثابت funcp = netlistp->evalFuncp(VEval::INITIAL);
    orderSequentially(funcp, logicClasses.m_initial);
    util::splitCheck(funcp);
}

تهی createPostponed(AstNetlist* netlistp, ثابت LogicClasses& logicClasses) {
    AstCFunc* ثابت funcp = netlistp->evalFuncp(VEval::POSTPONED);
    orderSequentially(funcp, logicClasses.m_postponed);
    util::splitCheck(funcp);
}

تهی createFinal(AstNetlist* netlistp, ثابت LogicClasses& logicClasses) {
    AstCFunc* ثابت funcp = netlistp->evalFuncp(VEval::FINAL);
    orderSequentially(funcp, logicClasses.m_final);
    util::splitCheck(funcp);
}

//============================================================================
// Helper that creates virtual interface value-change triggers

تهی addVirtIfaceTriggerAssignments(AstNetlist* netlistp, AstCFunc* initFuncp,
                                    ثابت VirtIfaceTriggers& virtIfaceTriggers,
                                    uint32_t firstIndex, ثابت TriggerKit& trigKit) {
    uint32_t index = firstIndex;
    برای (ثابت auto& entry : virtIfaceTriggers.m_triggers) {
        trigKit.add_مقدار_Change_Trigger_Assignment(netlistp, initFuncp, entry.m_vscp, index);
        ++index;
    }
}

// Order the combinational logic to create the 'stl' region
تهی createSettle(AstNetlist* netlistp, AstCFunc* ثابت initFuncp, Sen_عبارت_Builder& sen_عبارت_Bulider,
                  LogicClasses& logicClasses, ثابت CovergroupRefBindings& cgRefBindings) {
    // Clone, because ordering is destructive, but we still need them for the other regions
    LogicByScope comb = logicClasses.m_comb.clone();
    LogicByScope hybrid = logicClasses.m_hybrid.clone();

    // We have an extra trigger denoting this is the first iteration of the settle loop
    TriggerKit::ExtraTriggers extraTriggers;
    ثابت uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");

    // Gather the relevant sensitivity expressions and create the trigger kit
    ثابت auto& senTreeps = getSenTreesUsedBy({&comb, &hybrid});
    ثابت TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_عبارت_Bulider, {},
                                                  senTreeps, "stl", extraTriggers, درست, نادرست);

    // Add trigger indices (comb has none, so only do the hybrid)
    addTriggerIndices(hybrid, trigKit, نادرست);

    // First trigger is for pure combinational triggers (first iteration)
    ثابت uint32_t درون_داد_Changed = trigKit.extraTriggerIndex(firstIterationTrigger);

    // Create and the body function
    AstCFunc* ثابت stlFuncp = V3Order::order(
        {&comb, &hybrid}, trigKit, trigKit.vscp(), cgRefBindings, "stl", نادرست, درست,
        [=](ثابت AstVarScope*, std::set<uint32_t>& triggers) { triggers.درج(درون_داد_Changed); });
    util::splitCheck(stlFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::STL, firstIterationTrigger, trigKit,
        // Use trigger
        trigKit.vscp(), nullptr,
        // Explicit condition
        // Prep statements: Compute the current 'stl' triggers
        [&trigKit] {
            Ast_گره_Stmt* ثابت stmtp = trigKit.newCompBaseCall();
            اگر (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), درست));
            بازگشت stmtp;
        }(),
        // Work statements: Invoke the 'stl' function
        util::callVoidFunc(stlFuncp));
}

//============================================================================
// Order the replicated combinational logic to create the 'ico' region

تهی createIcoRegion(AstNetlist* netlistp, AstCFunc* ثابت initFuncp,
                     Sen_عبارت_Builder& sen_عبارت_Builder, LogicByScope& logic,
                     ثابت VirtIfaceTriggers& virtIfaceTriggers,
                     ثابت CovergroupRefBindings& cgRefBindings) {
    // SystemC only: Any top level inputs feeding a combinational logic must be marked,
    // so we can make them sc_sensitive
    اگر (v3Global.opt.systemC()) {
        logic.foreachLogic([](Ast_گره* logicp) {
            logicp->foreach([](AstVarRef* refp) {
                اگر (refp->access().isWriteOnly()) بازگشت;
                AstVarScope* ثابت vscp = refp->varScopep();
                اگر (vscp->scopep()->is_بالاترین() && vscp->varp()->isNonOutput()) {
                    vscp->varp()->scSensitive(درست);
                }
            });
        });
    }

    // We have some extra trigger denoting external conditions
    AstVarScope* ثابت dpiExportTriggerVscp = netlistp->dpiExportTriggerp();

    TriggerKit::ExtraTriggers extraTriggers;
    ثابت uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");
    ثابت uint32_t dpiExportTriggerIndex = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    ثابت uint32_t firstVifTriggerIndex = extraTriggers.size();
    برای (ثابت auto& entry : virtIfaceTriggers.m_triggers) {
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
    std::unordered_map<ثابت AstVarScope*, Ast_Sen_درخت*> inp2changedp;
    std::vector<Ast_Sen_درخت*> icoChangeSenTreeps;
    اگر (v3Global.opt.fIcoChangeDetect().isTrue()) {
        File_سطر* ثابت flp = netlistp->fileline();
        AstScope* ثابت scopep = netlistp->بالاترین_Scopep()->scopep();
        برای (AstVarScope* vscp = scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            // Only for top level ports, assume outputs don't change externally
            اگر (!vscp->varp()->isPrimaryInish()) ادامه;
            // Don't do if written by the design - wouldn't update the change detect 'prev' value
            اگر (vscp->varp()->icoMaybeWritten()) ادامه;
            // Don't do if forceable, as we can't see the actual value - this is belt and braces
            اگر (vscp->varp()->isForced()) ادامه;
            // Can't handle unpacked arrays (they have special types when primary input)
            اگر (VN_IS(vscp->dtypep()->skipRefp(), UnpackArrayDType)) ادامه;
            // Similarly to arrays, can't handle SystemC types
            اگر (vscp->varp()->isSc()) ادامه;
            // Create a sen tree triggered when this input changes
            Ast_Sen_درخت*& senTreepr = inp2changedp[vscp];
            UASSERT_OBJ(!senTreepr, vscp, "Duplicate input change detect trigger");
            AstVarRef* ثابت refp = جدید AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* ثابت senItemp = جدید AstSenItem{flp, VEdgeType::ET_CHANGED, refp};
            senTreepr = جدید Ast_Sen_درخت{flp, senItemp};
            icoChangeSenTreeps.افزودن_back(senTreepr);
        }
    }
    V3Stats::addStat("Scheduling, 'ico' change detect triggers", icoChangeSenTreeps.size());

    // Gather the relevant sensitivity expressions and create the trigger kit
    std::vector<ثابت Ast_Sen_درخت*> senTreeps = getSenTreesUsedBy({&logic});
    senTreeps.درج(senTreeps.end(), icoChangeSenTreeps.begin(), icoChangeSenTreeps.end());
    ثابت TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_عبارت_Builder, {},
                                                  senTreeps, "ico", extraTriggers, نادرست, نادرست);
    std::ignore = sen_عبارت_Builder.getAndClearResults();

    اگر (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, initFuncp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);

    // Add trigger indices
    addTriggerIndices(logic, trigKit, نادرست);

    // The 'first iteration' trigger for top level inputs
    ثابت uint32_t firstIterTrigger = trigKit.extraTriggerIndex(firstIterationTrigger);

    ثابت auto& vifVscpToTrigIco
        = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);

    // Create and Order the body function
    AstCFunc* ثابت icoFuncp = V3Order::order(
        {&logic}, trigKit, trigKit.vscp(), cgRefBindings, "ico", نادرست, نادرست,
        [&](ثابت AstVarScope* vscp, std::set<uint32_t>& triggers) {
            AstVar* ثابت varp = vscp->varp();
            // If it has an explicit change detect trigger, use that,
            // otherwise fall back to using the 'first iteration' trigger
            auto it = inp2changedp.find(vscp);
            اگر (it != inp2changedp.end()) {
                triggers.درج(trigKit.senItem2TrigIdx(it->second->sensesp()));
            } وگرنه اگر (varp->isPrimaryInish() || varp->isSigUserRWPublic() || varp->sampled()) {
                triggers.درج(firstIterTrigger);
            }
            // Add other triggers
            اگر (varp->isWrittenByDpi())
                triggers.درج(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
            اگر (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
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
            Ast_گره_Stmt* ثابت stmtp = trigKit.newCompBaseCall();
            اگر (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), درست));
            بازگشت stmtp;
        }(),
        // Work statements: Invoke the 'ico' function
        util::callVoidFunc(icoFuncp));

    // Release temporary input change detect SenTrees
    برای (Ast_Sen_درخت* ثابت senTreep : icoChangeSenTreeps)
        VL_DO_DANGLING(senTreep->delete_درخت(), senTreep);
    icoChangeSenTreeps.clear();
}

//============================================================================
// EvalKit groups items that have to be passed to createEval() for a given eval region

ساختار EvalKit final {
    // The AstVarScope representing the region's trigger vector
    AstVarScope* ثابت m_vscp = nullptr;
    // The AstCFunc that evaluates the region's logic
    AstCFunc* ثابت m_funcp = nullptr;
    // Is this kit used/required?
    منطقی empty() ثابت { بازگشت !m_funcp; }
};

//============================================================================
// Create the evaluation function of each region of a time step

تهی createEval(AstNetlist* netlistp,  //
                ثابت TriggerKit& trigKit,  //
                ثابت EvalKit& actKit,  //
                ثابت EvalKit& nbaKit,  //
                ثابت EvalKit& obsKit,  //
                ثابت EvalKit& reactKit,  //
                TimingKit& timingKit  //
) {
    File_سطر* ثابت flp = netlistp->fileline();

    // Grab the delay scheduler variable, if any
    AstVarScope* ثابت delaySchedVscp = timingKit.getDelayScheduler(netlistp);

    // 'createResume' consumes the contents that 'createReady' needs, so do the right order
    AstCCall* ثابت timingReadyp = timingKit.createReady(trigKit.vscp());
    AstCCall* ثابت timingResumep = timingKit.createResume(trigKit.vscp());

    // Create the 'act' region
    createEvalRegion(  //
        netlistp, VEval::ACT, 0, trigKit,
        // Use trigger
        actKit.m_vscp, nullptr,
        // Prep statements
        [&]() {
            // Compute the current 'act' triggers - the NBA triggers are the latched value
            Ast_گره_Stmt* stmtsp = trigKit.newCompBaseCall();
            Ast_گره_Stmt* ثابت dumpp
                = stmtsp ? trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), درست) : nullptr;
            // Mark as ready for triggered awaits
            اگر (timingReadyp) stmtsp = Ast_گره::addNext(stmtsp, timingReadyp->makeStmt());
            اگر (AstVarScope* ثابت vscAccp = trigKit.vscAccp()) {
                stmtsp = Ast_گره::addNext(stmtsp, trigKit.newOrIntoCall(actKit.m_vscp, vscAccp));
            }
            stmtsp = Ast_گره::addNext(stmtsp, trigKit.newCompExtCall(nbaKit.m_vscp));
            stmtsp = Ast_گره::addNext(stmtsp, dumpp);
            // Latch the 'act' triggers under the 'nba' triggers
            stmtsp = Ast_گره::addNext(stmtsp, trigKit.newOrIntoCall(nbaKit.m_vscp, actKit.m_vscp));
            //
            بازگشت stmtsp;
        }(),
        // Work statements
        [&]() {
            Ast_گره_Stmt* workp = nullptr;
            اگر (AstVarScope* ثابت actAccp = trigKit.vscAccp()) {
                AstCMethodHard* ثابت cCallp = جدید AstCMethodHard{
                    flp, جدید AstVarRef{flp, actAccp, VAccess::WRITE}, VCMethod::UNPACKED_FILL,
                    جدید AstConst{flp, AstConst::Unsized64{}, 0}};
                cCallp->dtypeSetVoid();
                workp = Ast_گره::addNext(workp, cCallp->makeStmt());
            }
            // Resume triggered timing schedulers
            اگر (timingResumep) workp = Ast_گره::addNext(workp, timingResumep->makeStmt());
            // Invoke the 'act' function
            workp = Ast_گره::addNext(workp, util::callVoidFunc(actKit.m_funcp));
            //
            بازگشت workp;
        }());

    // Create the 'inact' region
    createEvalRegion(  //
        netlistp, VEval::INACT, 0, trigKit,
        // Use explicit condition
        nullptr,
        [&]() -> Ast_گره_عبارت* {
            اگر (!delaySchedVscp) بازگشت nullptr;
            // Run if any zero delays are pending
            Ast_گره_عبارت* ثابت callp
                = جدید AstCMethodHard{flp, جدید AstVarRef{flp, delaySchedVscp, VAccess::READ},
                                     VCMethod::SCHED_AWAITING_ZERO_DELAY};
            callp->dtypeSetBit();
            بازگشت callp;
        }(),
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_گره_Stmt* {
            اگر (!delaySchedVscp) {
                // Nothing to do if there are no delays at all in the design
                بازگشت nullptr;
            } وگرنه اگر (v3Global.usesZeroDelay()) {
                // Resume processes watiting for #0 delay
                AstCMethodHard* ثابت callp = جدید AstCMethodHard{
                    flp, جدید AstVarRef{flp, delaySchedVscp, VAccess::READWRITE},
                    VCMethod::SCHED_RESUME_ZERO_DELAY};
                callp->dtypeSetVoid();
                بازگشت callp->makeStmt();
            } وگرنه {
                // Assumption was that the design doesn't use #0 delays.
                // Die at run-time if it does.
                AstCStmt* ثابت stmtp = جدید AstCStmt{flp};
                ثابت File_سطر* ثابت locp = netlistp->بالاترین_Modulep()->fileline();
                ثابت std::string& file = VIdProtect::protect(locp->filename());
                ثابت std::string& سطر = std::to_string(locp->lineno());
                stmtp->add("VL_FATAL_MT(\"" + V3OutFormatter::quoteNameControls(file) + "\", "
                           + سطر
                           + ", \"\", \"ZERODLY: Design Verilated with '--no-sched-zero-delay', "
                           + "but #0 delay executed at runtime\");");
                بازگشت stmtp;
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
            Ast_گره_Stmt* workp = nullptr;
            // Latch the 'nba' trigger flags under the following region's trigger flags
            اگر (!obsKit.empty()) {
                workp = trigKit.newOrIntoCall(obsKit.m_vscp, nbaKit.m_vscp);
            } وگرنه اگر (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, nbaKit.m_vscp);
            }
            // Invoke the 'nba' function
            workp = Ast_گره::addNext(workp, util::callVoidFunc(nbaKit.m_funcp));
            // Clear the 'nba' triggers
            workp = Ast_گره::addNext(workp, trigKit.newClearCall(nbaKit.m_vscp));
            //
            بازگشت workp;
        }(),
        // Extra work (not conditional on having had a fired trigger)
        [&](AstVarScope* continuep) -> Ast_گره_Stmt* {
            // Check if any dynamic NBAs are pending, if there are any in the design
            اگر (!netlistp->nbaEventp()) بازگشت nullptr;
            AstVarScope* ثابت nbaEventp = netlistp->nbaEventp();
            AstVarScope* ثابت nbaEventTriggerp = netlistp->nbaEventTriggerp();
            UASSERT(nbaEventTriggerp, "NBA event trigger var should exist");
            netlistp->nbaEventp(nullptr);
            netlistp->nbaEventTriggerp(nullptr);

            // If a dynamic NBA is pending, clear the pending flag and fire the ready event
            AstIf* ثابت ifp = جدید AstIf{flp, جدید AstVarRef{flp, nbaEventTriggerp, VAccess::READ}};
            ifp->addThensp(util::setVar(continuep, 1));
            ifp->addThensp(util::setVar(nbaEventTriggerp, 0));
            AstCMethodHard* ثابت firep = جدید AstCMethodHard{
                flp, جدید AstVarRef{flp, nbaEventp, VAccess::WRITE}, VCMethod::EVENT_FIRE};
            firep->dtypeSetVoid();
            ifp->addThensp(firep->makeStmt());
            بازگشت ifp;
        });

    // Create the 'obs' region
    createEvalRegion(  //
        netlistp, VEval::OBS, 0, trigKit,
        // Use trigger
        obsKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_گره_Stmt* {
            اگر (obsKit.empty()) بازگشت nullptr;
            Ast_گره_Stmt* workp = nullptr;
            // Latch the Observed trigger flags under the Reactive trigger flags
            اگر (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, obsKit.m_vscp);
            }
            // Invoke the 'obs' function
            workp = Ast_گره::addNext(workp, util::callVoidFunc(obsKit.m_funcp));
            // Clear the 'obs' triggers
            workp = Ast_گره::addNext(workp, trigKit.newClearCall(obsKit.m_vscp));
            //
            بازگشت workp;
        }());

    // Create the 'react' region
    createEvalRegion(  //
        netlistp, VEval::REACT, 0, trigKit,
        // Use trigger
        reactKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_گره_Stmt* {
            اگر (reactKit.empty()) بازگشت nullptr;
            // Invoke the 'react' function
            Ast_گره_Stmt* workp = util::callVoidFunc(reactKit.m_funcp);
            // Clear the 'react' triggers
            workp = Ast_گره::addNext(workp, trigKit.newClearCall(reactKit.m_vscp));
            بازگشت workp;
        }());
}

}  // namespace

//============================================================================
// Helper that builds virtual interface trigger sentrees

VirtIfaceTriggers::VscpTrigMap VirtIfaceTriggers::makeVscpToTrigMap(ثابت TriggerKit& trigKit,
                                                                    uint32_t firstIndex) ثابت {
    VscpTrigMap map;
    uint32_t index = firstIndex;
    برای (ثابت auto& entry : m_triggers) {
        map.emplace(entry.m_vscp, trigKit.extraTriggerIndex(index));
        ++index;
    }
    بازگشت map;
}

//============================================================================
// Top level entry-point to scheduling

تهی schedule(AstNetlist* netlistp) {
    ثابت auto addSizeStat = [](ثابت string& name, ثابت LogicByScope& lbs) {
        uint64_t size = 0;
        lbs.foreachLogic([&](Ast_گره* nodep) { size += nodep->گره_شمار(); });
        V3Stats::addStat("Scheduling, " + name, size);
    };

    // Step 2: Prepare external domains for timing and virtual interfaces
    // Create extra triggers for virtual interfaces
    ثابت auto& virtIfaceTriggers = makeVirtIfaceTriggers(netlistp);
    // Resolve what covergroup reference formal arguments are bound to, which is visible here
    // but not from V3Order, where the reads through them must be modeled
    ثابت CovergroupRefBindings& cgRefBindings = makeCovergroupRefBindings(netlistp);
    // Prepare timing-related logic and external domains
    TimingKit timingKit = prepareTiming(netlistp);

    // Step 3: Gather and classify all logic in the design
    LogicClasses logicClasses = gatherLogicClasses(netlistp);

    اگر (v3Global.opt.stats()) {
        V3Stats::statsStage("sched-gather");
        addSizeStat("size of class: static", logicClasses.m_static);
        addSizeStat("size of class: initial", logicClasses.m_initial);
        addSizeStat("size of class: final", logicClasses.m_final);
    }

    // Step 4: Schedule static, initial and final logic classes in source order
    AstCFunc* ثابت staticp = createStatic(netlistp, logicClasses);
    اگر (v3Global.opt.stats()) V3Stats::statsStage("sched-static");

    createInitial(netlistp, logicClasses);
    اگر (v3Global.opt.stats()) V3Stats::statsStage("sched-initial");

    createFinal(netlistp, logicClasses);
    اگر (v3Global.opt.stats()) V3Stats::statsStage("sched-final");

    // Step 5: Break combinational cycles by introducing hybrid logic
    // Note: breakCycles also removes corresponding logic from logicClasses.m_comb;
    logicClasses.m_hybrid = breakCycles(netlistp, logicClasses.m_comb);
    اگر (v3Global.opt.stats()) {
        addSizeStat("size of class: clocked", logicClasses.m_clocked);
        addSizeStat("size of class: combinational", logicClasses.m_comb);
        addSizeStat("size of class: hybrid", logicClasses.m_hybrid);
        V3Stats::statsStage("sched-break-cycles");
    }

    // We pass around a single SenExprBuilder instance, as we only need one set of 'prev' variables
    // for edge/change detection in sensitivity expressions, which this keeps track of.
    Ast_بالاترین_Scope* ثابت بالاترین_Scopep = netlistp->بالاترین_Scopep();
    AstScope* ثابت scopeTopp = بالاترین_Scopep->scopep();
    Sen_عبارت_Builder sen_عبارت_Builder{scopeTopp};

    // Step 6: Create 'settle' region that restores the combinational invariant
    createSettle(netlistp, staticp, sen_عبارت_Builder, logicClasses, cgRefBindings);
    اگر (v3Global.opt.stats()) V3Stats::statsStage("sched-settle");

    // Step 7: Partition the clocked and combinational (including hybrid) logic into pre/act/nba.
    // All clocks (signals referenced in an AstSenTree) generated via a blocking assignment
    // (including combinationally generated signals) are computed within the act region.
    LogicRegions logicRegions
        = partition(logicClasses.m_clocked, logicClasses.m_comb, logicClasses.m_hybrid);
    logicRegions.m_obs = logicClasses.m_observed;
    logicRegions.m_react = logicClasses.m_reactive;
    اگر (v3Global.opt.stats()) {
        addSizeStat("size of region: Active Pre", logicRegions.m_pre);
        addSizeStat("size of region: Active", logicRegions.m_act);
        addSizeStat("size of region: NBA", logicRegions.m_nba);
        addSizeStat("size of region: Observed", logicRegions.m_obs);
        addSizeStat("size of region: Reactive", logicRegions.m_react);
        V3Stats::statsStage("sched-partition");
    }

    // Step 8: Replicate combinational logic
    LogicReplicas logicReplicas = replicateLogic(logicRegions);
    اگر (v3Global.opt.stats()) {
        addSizeStat("size of replicated logic: Input", logicReplicas.m_ico);
        addSizeStat("size of replicated logic: Active", logicReplicas.m_act);
        addSizeStat("size of replicated logic: NBA", logicReplicas.m_nba);
        addSizeStat("size of replicated logic: Observed", logicReplicas.m_obs);
        addSizeStat("size of replicated logic: Reactive", logicReplicas.m_react);
        V3Stats::statsStage("sched-replicate");
    }

    // Step 9: Create the input combinational logic
    createIcoRegion(netlistp, staticp, sen_عبارت_Builder, logicReplicas.m_ico, virtIfaceTriggers,
                    cgRefBindings);
    اگر (v3Global.opt.stats()) V3Stats::statsStage("sched-create-ico");

    // Step 10: Create the triggers
    AstVarScope* ثابت dpiExportTriggerVscp = netlistp->dpiExportTriggerp();
    netlistp->dpiExportTriggerp(nullptr);  // Finished with this here

    // We may have an extra trigger for variable updated in DPI exports
    TriggerKit::ExtraTriggers extraTriggers;
    ثابت uint32_t dpiExportTriggerIndex = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    ثابت uint32_t firstVifTriggerIndex = extraTriggers.size();
    برای (ثابت auto& entry : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + entry.m_ifacep->name() + "."
                               + entry.m_memberp->name());
    }

    ثابت auto& preTreeps = getSenTreesUsedBy({&logicRegions.m_pre});
    ثابت auto& senTreeps = getSenTreesUsedBy({&logicRegions.m_act,  //
                                               &logicRegions.m_nba,  //
                                               &logicRegions.m_obs,  //
                                               &logicRegions.m_react,  //
                                               &timingKit.m_lbs});
    ثابت TriggerKit trigKit
        = TriggerKit::create(netlistp, staticp, sen_عبارت_Builder, preTreeps, senTreeps, "act",
                             extraTriggers, نادرست, v3Global.usesTiming());

    // Add post updates from the timing kit
    اگر (timingKit.m_postUpdates) trigKit.compBasep()->addStmtsp(timingKit.m_postUpdates);

    اگر (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, staticp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);
    اگر (v3Global.opt.stats()) V3Stats::statsStage("sched-create-triggers");

    // Note: Experiments so far show that running the Act (or Ico) regions on
    // multiple threads is always a net loss, so only use multi-threading for
    // NBA for now. This can be revised if evidence is available that it would
    // be beneficial

    // Step 11: Create the 'act' region evaluation function

    // Add the trigger indices of the input logic
    addTriggerIndices(logicRegions.m_pre, trigKit, درست);
    addTriggerIndices(logicRegions.m_act, trigKit, نادرست);
    addTriggerIndices(logicReplicas.m_act, trigKit, نادرست);
    addTriggerIndices(timingKit.m_lbs, trigKit, نادرست);

    // External triggers of variables. Trigger indices are the same in all regions, so share.
    ثابت std::map<ثابت AstVarScope*, std::vector<uint32_t>> timingTriggers
        = timingKit.externalTriggerIndices(trigKit);
    ثابت auto& vifVscpToTrig = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);
    ثابت V3Order::ExternalTriggersProvider externalTriggers
        = [&](ثابت AstVarScope* vscp, std::set<uint32_t>& triggers) {
              auto it = timingTriggers.find(vscp);
              اگر (it != timingTriggers.end())
                  triggers.درج(it->second.begin(), it->second.end());
              اگر (vscp->varp()->isWrittenByDpi()) {
                  triggers.درج(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
              }
              اگر (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                  addTriggeredIface(vscp, vifVscpToTrig, virtIfaceTriggers, triggers);
              }
          };

    AstCFunc* ثابت actFuncp
        = V3Order::order({&logicRegions.m_pre, &logicRegions.m_act, &logicReplicas.m_act}, trigKit,
                         trigKit.vscp(), cgRefBindings, "act", نادرست, نادرست, externalTriggers);
    util::splitCheck(actFuncp);
    اگر (v3Global.opt.stats()) V3Stats::statsStage("sched-create-act");

    ثابت EvalKit actKit{trigKit.vscp(), actFuncp};

    // Orders a region's logic and creates the region eval function
    ثابت auto order = [&](ثابت std::string& name,
                           ثابت std::vector<V3Sched::LogicByScope*>& logic) -> EvalKit {
        UINFO(2, "Scheduling " << name << " #logic = " << logic.size());
        AstVarScope* ثابت trigVscp = trigKit.newTrigVec(name);
        // Add the trigger indices of the input logic
        برای (LogicByScope* lbs : logic) addTriggerIndices(*lbs, trigKit, نادرست);

        AstCFunc* ثابت funcp
            = V3Order::order(logic, trigKit, trigVscp, cgRefBindings, name,
                             name == "nba" && v3Global.opt.mtasks(), نادرست, externalTriggers);

        بازگشت {trigVscp, funcp};
    };

    // Step 12: Create the 'nba' region evaluation function
    ثابت EvalKit nbaKit = order("nba", {&logicRegions.m_nba, &logicReplicas.m_nba});
    util::splitCheck(nbaKit.m_funcp);
    اگر (v3Global.opt.stats()) V3Stats::statsStage("sched-create-nba");

    // Orders a region's logic and creates the region eval function (only if there is any logic in
    // the region)
    ثابت auto orderIfNonEmpty
        = [&](ثابت std::string& name, ثابت std::vector<LogicByScope*>& logic) -> EvalKit {
        اگر (logic[0]->empty())
            بازگشت {};  // if region is empty, replica is supposed to be empty as well
        ثابت auto& kit = order(name, logic);
        اگر (v3Global.opt.stats()) V3Stats::statsStage("sched-create-" + name);
        بازگشت kit;
    };

    // Step 13: Create the 'obs' region evaluation function
    ثابت EvalKit obsKit = orderIfNonEmpty("obs", {&logicRegions.m_obs, &logicReplicas.m_obs});

    // Step 14: Create the 'react' region evaluation function
    ثابت EvalKit reactKit
        = orderIfNonEmpty("react", {&logicRegions.m_react, &logicReplicas.m_react});

    // Step 15: Create the 'postponed' region evaluation function
    createPostponed(netlistp, logicClasses);

    // Step 16: Populate the eval entry point function of each region of a time step
    createEval(netlistp, trigKit, actKit, nbaKit, obsKit, reactKit, timingKit);

    // Step 17: Add neccessary evaluation before awaits
    اگر (AstCCall* ثابت readyp = timingKit.createReady(trigKit.vscp())) {
        staticp->addStmtsp(readyp->makeStmt());
        beforeTrigVisitor(netlistp, sen_عبارت_Builder, trigKit);
    } وگرنه {
        // beforeTrigVisitor clears Sentree pointers in AstCAwaits (as these sentrees will get
        // deleted later) if there was no need to call it, SenTrees have to be cleaned manually
        netlistp->foreach([](AstCAwait* ثابت cAwaitp) { cAwaitp->clearSentreep(); });
    }
    اگر (AstVarScope* ثابت trigAccp = trigKit.vscAccp()) {
        // Copy trigger vector to accumulator at the end of static initialziation so,
        // triggers fired during initialization persist to the first resume.
        ثابت AstUnpackArrayDType* ثابت trigAccDTypep
            = VN_AS(trigAccp->dtypep(), UnpackArrayDType);
        UASSERT_OBJ(
            trigAccDTypep->راست() == 0, trigAccp,
            "Expected that trigger vector and accumulator start elements enumeration from 0");
        UASSERT_OBJ(trigAccDTypep->چپ() >= 0, trigAccp,
                    "Expected that trigger vector and accumulator has no negative indexes");
        File_سطر* ثابت flp = trigAccp->fileline();
        AstVarScope* ثابت vscp = netlistp->بالاترین_Scopep()->createTemp("__Vi", 32);
        AstLoop* ثابت loopp = جدید AstLoop{flp};
        loopp->addStmtsp(
            جدید AstAssign{flp,
                          جدید AstArraySel{flp, جدید AstVarRef{flp, trigAccp, VAccess::WRITE},
                                          جدید AstVarRef{flp, vscp, VAccess::READ}},
                          جدید AstArraySel{flp, جدید AstVarRef{flp, actKit.m_vscp, VAccess::READ},
                                          جدید AstVarRef{flp, vscp, VAccess::READ}}});
        loopp->addStmtsp(util::incrementVar(vscp));
        loopp->addStmtsp(جدید AstLoopTest{
            flp, loopp,
            جدید AstLte{flp, جدید AstVarRef{flp, vscp, VAccess::READ},
                       جدید AstConst{flp, AstConst::Widthed_مقدار{}, 32,
                                    static_cast<uint32_t>(trigAccDTypep->چپ())}}});
        staticp->addStmtsp(loopp);
    }

    // Step 18: Clean up
    netlistp->clearStlFirstIterationp();

    اگر (v3Global.opt.stats()) {
        // A sample() call resolved to the union over its covergroup's constructions reads more
        // than it can, which orders it against more logic than necessary
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per instance",
                         cgRefBindings.رقم_Exact_Calls());
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per type",
                         cgRefBindings.رقم_Union_Calls());
    }

    // Haven't split static initializer yet
    util::splitCheck(staticp);

    // Record eval stats
    netlistp->addEvalStats("sched");

    // Dump
    V3Global::dump_Check_Global_درخت("sched", 0, dump_درخت_Either_Level() >= 3);
}

}  // namespace V3Sched
