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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Sched.h"

#ضمن "V3Const.h"
#ضمن "V3EmitCBase.h"
#ضمن "V3EmitV.h"
#ضمن "V3Order.h"
#ضمن "V3SenExprBuilder.h"
#ضمن "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

نطاق V3Sched {

نطاق {

//============================================================================
// Utility functions

std::vector<ثابت AstSenTree*> getSenTreesUsedBy(ثابت std::vector<ثابت LogicByScope*>& lbsps) {
    ثابت VNUser1InUse user1InUse;
    std::vector<ثابت AstSenTree*> result;
    لكل (ثابت LogicByScope* ثابت lbsp : lbsps) {
        لكل (ثابت auto& entry : *lbsp) {
            AstActive* ثابت activep = entry.m_activep;
            AstSenTree* ثابت senTreep = activep->sentreep();
            ܐܢ (senTreep->user1SetOnce()) استمر;
            ܐܢ (senTreep->hasClocked() || senTreep->hasHybrid()) result.push_back(senTreep);
        }
    }
    ܦܢܐ result;
}

فراغ addTriggerIndices(LogicByScope& lbs, ثابت TriggerKit& trigKit, منطقي pre) {
    لكل (LogicByScope::Entry& entry : lbs) {
        AstSenTree* ثابت senTreep = entry.m_activep->sentreep();
        ܐܢ (senTreep->hasCombo()) استمر;
        entry.m_triggers = trigKit.triggerIndices(senTreep, pre);
    }
}

// Add the VIF triggers that a given VarScope should be sensitive to.
// Case 2 (non-virtual interface read): sensitive to that specific VarScope's trigger only
// Case 3 (virtual interface read): sensitive to all member triggers of the same interface type
فراغ addTriggeredIface(ثابت AstVarScope* vscp, ثابت VirtIfaceTriggers::VscpTrigMap& vscpToTrig,
                       ثابت VirtIfaceTriggers& virtIfaceTriggers, std::set<uint32_t>& triggers) {
    ܐܢ (vscp->varp()->isVirtIface()) {
        // Virtual interface variable -- sensitive to all member triggers of this interface type
        ثابت AstIface* ثابت ifacep = VN_AS(vscp->varp()->dtypep(), IfaceRefDType)->ifacep();
        لكل (ثابت auto& entry : virtIfaceTriggers.m_triggers) {
            ܐܢ (entry.m_ifacep == ifacep) {
                ثابت auto it = vscpToTrig.find(entry.m_vscp);
                ܐܢ (it != vscpToTrig.end()) triggers.insert(it->second);
            }
        }
    } ܐܚܪܢܐ {
        // Non-virtual interface member -- sensitive to this VarScope's trigger only
        ثابت auto it = vscpToTrig.find(vscp);
        ܐܢ (it != vscpToTrig.end()) triggers.insert(it->second);
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
فراغ createEvalRegion(
    AstNetlist* netlistp,  //
    VEval eval,  // The entry point of the current region
    // Index of the region's 'first iteration' extra trigger, if it has one, otherwise ignored
    uint32_t firstIterTrigger,
    ثابت TriggerKit& trigKit,  // The trigger kit
    AstVarScope* trigp,  // The trigger vector - may be nullptr if no triggers or using 'condp'
    AstNodeExpr* condp,  // Explicit condition that must be true to run 'phaseWorkp'
    AstNodeStmt* phasePrepp,  // Prep statements run before checking triggers
    AstNodeStmt* phaseWorkp,  // The work to do if anything triggered
    // Extra statements to run after the work, even if no triggers fired. This function is
    // passed a variable, which must be set to true if we must continue and loop again,
    // and must be unmodified otherwise.
    std::function<AstNodeStmt*(AstVarScope*)> phaseExtra = [](AstVarScope*) { ܦܢܐ nullptr; }  //
) {
    UASSERT(!trigp || !condp, "Cannot use both 'trigp' and 'condp' in 'createEvalRegion'");
    UASSERT(!eval.firstIteration() || trigp,
            "Region without triggers cannot need a first iteration flag");

    // All work is under a trigger or condition, so with neither the region has
    // nothing to evaluate, and what we create below reduces to a no-op function.

    ثابت std::string tag = eval.tag();
    AstTopScope* ثابت topScopep = netlistp->topScopep();
    FileLine* ثابت flp = netlistp->fileline();

    // Populate the trigger dump entry point function
    ܐܢ (trigp) {
        netlistp->dumpTriggersFuncp(eval)->addStmtsp(trigKit.newDumpCall(trigp, tag, ܕܓܠܐ));
    }

    AstCFunc* ثابت funcp = netlistp->evalFuncp(eval);

    // A flag is passed from the run-time eval loop if this is the first iteration of the
    // current loop
    ܐܢ (eval.firstIteration()) {
        AstVarScope* ثابت firstIterArgp = util::newArgument(funcp, netlistp->findBitDType(),
                                                             "firstIteration", VDirection::INPUT);

        // Set the region's 'first iteration' trigger straight from the argument
        funcp->addStmtsp(trigKit.newExtraTriggerAssignment(firstIterArgp, firstIterTrigger));

        // Only 'stl' also needs a module level flag, for design logic that reads
        // it directly (see V3Timing). Those reads can be anywhere in the design,
        // hence module level. Always created, even if nothing reads it.
        // TODO: get rid of this special case
        ܐܢ (eval == VEval::STL) {
            AstVarScope* ثابت firstIterp = netlistp->stlFirstIterationp();
            firstIterp->varp()->noReset(ܫܪܪܐ);
            firstIterp->varp()->isInternal(ܫܪܪܐ);
            funcp->addStmtsp(جديد AstAssign{flp, جديد AstVarRef{flp, firstIterp, VAccess::WRITE},
                                           جديد AstVarRef{flp, firstIterArgp, VAccess::READ}});
        }
    }

    {
        // Add the preparatory statements
        funcp->addStmtsp(phasePrepp);

        // The execute flag
        AstVarScope* ثابت executeFlagp = topScopep->createTemp("__V" + tag + "Execute", 1);
        executeFlagp->varp()->noReset(ܫܪܪܐ);

        // If there is work in this region, execute it if any triggers fired
        ܐܢ (phaseWorkp) {
            AstNodeExpr* ثابت lhsp = جديد AstVarRef{flp, executeFlagp, VAccess::WRITE};
            // If using explicit condition, that directly determines whether to execute,
            // otherwise check if any triggers are fired
            AstNodeExpr* ثابت rhsp = condp ? condp : trigKit.newAnySetCall(trigp);
            funcp->addStmtsp(جديد AstAssign{flp, lhsp, rhsp});

            // Add the work
            AstIf* ثابت ifp = جديد AstIf{flp, جديد AstVarRef{flp, executeFlagp, VAccess::READ}};
            ifp->addThensp(phaseWorkp);
            funcp->addStmtsp(ifp);
        }

        // Construct the extra statements
        AstNodeStmt* ثابت extraWorkp = phaseExtra(executeFlagp);
        ܐܢ (extraWorkp) funcp->addStmtsp(extraWorkp);

        // The function returns true iff it did run work
        AstNodeExpr* ثابت retp
            = phaseWorkp || extraWorkp
                  ? static_cast<AstNodeExpr*>(جديد AstVarRef{flp, executeFlagp, VAccess::READ})
                  : static_cast<AstNodeExpr*>(جديد AstConst{flp, AstConst::BitFalse{}});
        funcp->addStmtsp(جديد AstCReturn{flp, retp});
    }
}

//============================================================================
// Collect and classify all logic in the design

LogicClasses gatherLogicClasses(AstNetlist* netlistp) {
    LogicClasses result;

    netlistp->foreach([&](AstScope* scopep) {
        scopep->foreach([&](AstActive* activep) {
            AstSenTree* ثابت senTreep = activep->sentreep();
            ܐܢ (senTreep->hasStatic()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "static initializer with additional sensitivities");
                result.m_static.add(scopep, activep);
            } ܐܚܪܢܐ ܐܢ (senTreep->hasInitial()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'initial' logic with additional sensitivities");
                result.m_initial.add(scopep, activep);
            } ܐܚܪܢܐ ܐܢ (senTreep->hasFinal()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'final' logic with additional sensitivities");
                result.m_final.add(scopep, activep);
            } ܐܚܪܢܐ ܐܢ (senTreep->hasCombo()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "combinational logic with additional sensitivities");
                ܐܢ (VN_IS(activep->stmtsp(), AlwaysPostponed)) {
                    result.m_postponed.add(scopep, activep);
                } ܐܚܪܢܐ {
                    result.m_comb.add(scopep, activep);
                }
            } ܐܚܪܢܐ {
                UASSERT_OBJ(senTreep->hasClocked(), activep, "What else could it be?");
                ܐܢ (VN_IS(activep->stmtsp(), AlwaysObserved)) {
                    result.m_observed.add(scopep, activep);
                } ܐܚܪܢܐ ܐܢ (VN_IS(activep->stmtsp(), AlwaysReactive)) {
                    result.m_reactive.add(scopep, activep);
                } ܐܚܪܢܐ {
                    result.m_clocked.add(scopep, activep);
                }
            }
        });
    });

    ܦܢܐ result;
}

//============================================================================
// Simple ordering in source order

فراغ orderSequentially(AstCFunc* funcp, ثابت LogicByScope& lbs) {
    // Create new subfunc for scope
    ثابت auto createNewSubFuncp = [&](AstScope* ثابت scopep) {
        ثابت string subName{funcp->name() + "__" + scopep->nameDotless()};
        AstCFunc* ثابت subFuncp = جديد AstCFunc{scopep->fileline(), subName, scopep};
        subFuncp->isLoose(ܫܪܪܐ);
        subFuncp->isConst(ܕܓܠܐ);
        subFuncp->declPrivate(ܫܪܪܐ);
        subFuncp->slow(funcp->slow());
        scopep->addBlocksp(subFuncp);
        // Call it from the top function
        funcp->addStmtsp(util::callVoidFunc(subFuncp));
        ܦܢܐ subFuncp;
    };
    ثابت VNUser1InUse user1InUse;  // AstScope -> AstCFunc: the sub-function for the scope
    ثابت VNUser2InUse user2InUse;  // AstScope -> int: sub-function counter used for names
    لكل (ثابت auto& entry : lbs) {
        AstScope* ثابت scopep = entry.m_scopep;
        AstActive* ثابت activep = entry.m_activep;
        // Create a sub-function per scope so we can V3Combine them later
        ܐܢ (!scopep->user1p()) scopep->user1p(createNewSubFuncp(scopep));
        // Add statements to sub-function
        لكل (AstNode *logicp = activep->stmtsp(), *nextp; logicp; logicp = nextp) {
            auto* subFuncp = VN_AS(scopep->user1p(), CFunc);
            nextp = logicp->nextp();
            ܐܢ (AstNodeProcedure* ثابت procp = VN_CAST(logicp, NodeProcedure)) {
                ܐܢ (AstNode* bodyp = procp->stmtsp()) {
                    bodyp->unlinkFrBackWithNext();
                    // If the process is suspendable, we need a separate function (a coroutine)
                    ܐܢ (procp->isSuspendable()) {
                        funcp->slow(ܕܓܠܐ);
                        subFuncp = createNewSubFuncp(scopep);
                        subFuncp->name(subFuncp->name() + "__Vtiming__"
                                       + cvtToStr(scopep->user2Inc()));
                        subFuncp->rtnType("VlCoroutine");
                        ܐܢ (VN_IS(procp, Always)) {
                            subFuncp->slow(ܕܓܠܐ);
                            FileLine* ثابت flp = procp->fileline();
                            AstNodeExpr* ثابت condp = جديد AstCExpr{
                                flp, "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish())", 1};
                            AstLoop* ثابت loopp = جديد AstLoop{flp};
                            loopp->addStmtsp(جديد AstLoopTest{flp, loopp, condp});
                            loopp->addStmtsp(bodyp);
                            bodyp = loopp;
                        }
                    }
                    subFuncp->addStmtsp(bodyp);
                    ܐܢ (procp->needProcess()) subFuncp->setNeedProcess();
                    // A coroutine holds a single process, so it is complete here
                    ܐܢ (procp->isSuspendable()) util::splitCheck(subFuncp);
                }
            } ܐܚܪܢܐ {
                logicp->unlinkFrBack();
                subFuncp->addStmtsp(logicp);
            }
        }
        ܐܢ (activep->backp()) activep->unlinkFrBack();
        VL_DO_DANGLING(activep->deleteTree(), activep);
    }
    // Split sub-functions once complete
    لكل (ثابت auto& entry : lbs) {
        AstScope* ثابت scopep = entry.m_scopep;
        ܐܢ (AstCFunc* ثابت subFuncp = VN_AS(scopep->user1p(), CFunc)) {
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
    ܐܢ (orig.size() <= 1) {
        orderSequentially(funcp, orig);
        ܦܢܐ funcp;
    }

    // Level-based module sorting can reorder packages so that an importing
    // package runs before the imported one.  Re-sort package entries by source
    // file position to restore compilation order (IEEE 1800-2023 26.3).
    std::vector<size_t> indices(orig.size());
    لكل (size_t i = 0; i < orig.size(); ++i) indices[i] = i;
    std::stable_sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        ثابت AstNodeModule* ثابت modA = orig[a].m_scopep->modp();
        ثابت AstNodeModule* ثابت modB = orig[b].m_scopep->modp();
        ثابت منطقي isPkgA = VN_IS(modA, Package);
        ثابت منطقي isPkgB = VN_IS(modB, Package);
        ܐܢ (isPkgA != isPkgB) ܦܢܐ isPkgA;  // Packages before non-packages
        ܐܢ (isPkgA && isPkgB) {
            // Sort packages by source file position (compilation order)
            ܦܢܐ modA->fileline()->operatorCompare(*modB->fileline()) < 0;
        }
        ܦܢܐ ܕܓܠܐ;  // Both non-package: preserve original order
    });
    LogicByScope sorted;
    لكل (ثابت size_t i : indices) sorted.add(orig[i].m_scopep, orig[i].m_activep);

    orderSequentially(funcp, sorted);
    ܦܢܐ funcp;  // Not splitting yet as it is not final
}

فراغ createInitial(AstNetlist* netlistp, ثابت LogicClasses& logicClasses) {
    AstCFunc* ثابت funcp = netlistp->evalFuncp(VEval::INITIAL);
    orderSequentially(funcp, logicClasses.m_initial);
    util::splitCheck(funcp);
}

فراغ createPostponed(AstNetlist* netlistp, ثابت LogicClasses& logicClasses) {
    AstCFunc* ثابت funcp = netlistp->evalFuncp(VEval::POSTPONED);
    orderSequentially(funcp, logicClasses.m_postponed);
    util::splitCheck(funcp);
}

فراغ createFinal(AstNetlist* netlistp, ثابت LogicClasses& logicClasses) {
    AstCFunc* ثابت funcp = netlistp->evalFuncp(VEval::FINAL);
    orderSequentially(funcp, logicClasses.m_final);
    util::splitCheck(funcp);
}

//============================================================================
// Helper that creates virtual interface value-change triggers

فراغ addVirtIfaceTriggerAssignments(AstNetlist* netlistp, AstCFunc* initFuncp,
                                    ثابت VirtIfaceTriggers& virtIfaceTriggers,
                                    uint32_t firstIndex, ثابت TriggerKit& trigKit) {
    uint32_t index = firstIndex;
    لكل (ثابت auto& entry : virtIfaceTriggers.m_triggers) {
        trigKit.addValueChangeTriggerAssignment(netlistp, initFuncp, entry.m_vscp, index);
        ++index;
    }
}

// Order the combinational logic to create the 'stl' region
فراغ createSettle(AstNetlist* netlistp, AstCFunc* ثابت initFuncp, SenExprBuilder& senExprBulider,
                  LogicClasses& logicClasses, ثابت CovergroupRefBindings& cgRefBindings) {
    // Clone, because ordering is destructive, but we still need them for the other regions
    LogicByScope comb = logicClasses.m_comb.clone();
    LogicByScope hybrid = logicClasses.m_hybrid.clone();

    // We have an extra trigger denoting this is the first iteration of the settle loop
    TriggerKit::ExtraTriggers extraTriggers;
    ثابت uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");

    // Gather the relevant sensitivity expressions and create the trigger kit
    ثابت auto& senTreeps = getSenTreesUsedBy({&comb, &hybrid});
    ثابت TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, senExprBulider, {},
                                                  senTreeps, "stl", extraTriggers, ܫܪܪܐ, ܕܓܠܐ);

    // Add trigger indices (comb has none, so only do the hybrid)
    addTriggerIndices(hybrid, trigKit, ܕܓܠܐ);

    // First trigger is for pure combinational triggers (first iteration)
    ثابت uint32_t inputChanged = trigKit.extraTriggerIndex(firstIterationTrigger);

    // Create and the body function
    AstCFunc* ثابت stlFuncp = V3Order::order(
        {&comb, &hybrid}, trigKit, trigKit.vscp(), cgRefBindings, "stl", ܕܓܠܐ, ܫܪܪܐ,
        [=](ثابت AstVarScope*, std::set<uint32_t>& triggers) { triggers.insert(inputChanged); });
    util::splitCheck(stlFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::STL, firstIterationTrigger, trigKit,
        // Use trigger
        trigKit.vscp(), nullptr,
        // Explicit condition
        // Prep statements: Compute the current 'stl' triggers
        [&trigKit] {
            AstNodeStmt* ثابت stmtp = trigKit.newCompBaseCall();
            ܐܢ (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), ܫܪܪܐ));
            ܦܢܐ stmtp;
        }(),
        // Work statements: Invoke the 'stl' function
        util::callVoidFunc(stlFuncp));
}

//============================================================================
// Order the replicated combinational logic to create the 'ico' region

فراغ createIcoRegion(AstNetlist* netlistp, AstCFunc* ثابت initFuncp,
                     SenExprBuilder& senExprBuilder, LogicByScope& logic,
                     ثابت VirtIfaceTriggers& virtIfaceTriggers,
                     ثابت CovergroupRefBindings& cgRefBindings) {
    // SystemC only: Any top level inputs feeding a combinational logic must be marked,
    // so we can make them sc_sensitive
    ܐܢ (v3Global.opt.systemC()) {
        logic.foreachLogic([](AstNode* logicp) {
            logicp->foreach([](AstVarRef* refp) {
                ܐܢ (refp->access().isWriteOnly()) ܦܢܐ;
                AstVarScope* ثابت vscp = refp->varScopep();
                ܐܢ (vscp->scopep()->isTop() && vscp->varp()->isNonOutput()) {
                    vscp->varp()->scSensitive(ܫܪܪܐ);
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
    لكل (ثابت auto& entry : virtIfaceTriggers.m_triggers) {
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
    std::unordered_map<ثابت AstVarScope*, AstSenTree*> inp2changedp;
    std::vector<AstSenTree*> icoChangeSenTreeps;
    ܐܢ (v3Global.opt.fIcoChangeDetect().isTrue()) {
        FileLine* ثابت flp = netlistp->fileline();
        AstScope* ثابت scopep = netlistp->topScopep()->scopep();
        لكل (AstVarScope* vscp = scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            // Only for top level ports, assume outputs don't change externally
            ܐܢ (!vscp->varp()->isPrimaryInish()) استمر;
            // Don't do if written by the design - wouldn't update the change detect 'prev' value
            ܐܢ (vscp->varp()->icoMaybeWritten()) استمر;
            // Don't do if forceable, as we can't see the actual value - this is belt and braces
            ܐܢ (vscp->varp()->isForced()) استمر;
            // Can't handle unpacked arrays (they have special types when primary input)
            ܐܢ (VN_IS(vscp->dtypep()->skipRefp(), UnpackArrayDType)) استمر;
            // Similarly to arrays, can't handle SystemC types
            ܐܢ (vscp->varp()->isSc()) استمر;
            // Create a sen tree triggered when this input changes
            AstSenTree*& senTreepr = inp2changedp[vscp];
            UASSERT_OBJ(!senTreepr, vscp, "Duplicate input change detect trigger");
            AstVarRef* ثابت refp = جديد AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* ثابت senItemp = جديد AstSenItem{flp, VEdgeType::ET_CHANGED, refp};
            senTreepr = جديد AstSenTree{flp, senItemp};
            icoChangeSenTreeps.push_back(senTreepr);
        }
    }
    V3Stats::addStat("Scheduling, 'ico' change detect triggers", icoChangeSenTreeps.size());

    // Gather the relevant sensitivity expressions and create the trigger kit
    std::vector<ثابت AstSenTree*> senTreeps = getSenTreesUsedBy({&logic});
    senTreeps.insert(senTreeps.end(), icoChangeSenTreeps.begin(), icoChangeSenTreeps.end());
    ثابت TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, senExprBuilder, {},
                                                  senTreeps, "ico", extraTriggers, ܕܓܠܐ, ܕܓܠܐ);
    std::ignore = senExprBuilder.getAndClearResults();

    ܐܢ (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, initFuncp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);

    // Add trigger indices
    addTriggerIndices(logic, trigKit, ܕܓܠܐ);

    // The 'first iteration' trigger for top level inputs
    ثابت uint32_t firstIterTrigger = trigKit.extraTriggerIndex(firstIterationTrigger);

    ثابت auto& vifVscpToTrigIco
        = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);

    // Create and Order the body function
    AstCFunc* ثابت icoFuncp = V3Order::order(
        {&logic}, trigKit, trigKit.vscp(), cgRefBindings, "ico", ܕܓܠܐ, ܕܓܠܐ,
        [&](ثابت AstVarScope* vscp, std::set<uint32_t>& triggers) {
            AstVar* ثابت varp = vscp->varp();
            // If it has an explicit change detect trigger, use that,
            // otherwise fall back to using the 'first iteration' trigger
            auto it = inp2changedp.find(vscp);
            ܐܢ (it != inp2changedp.end()) {
                triggers.insert(trigKit.senItem2TrigIdx(it->second->sensesp()));
            } ܐܚܪܢܐ ܐܢ (varp->isPrimaryInish() || varp->isSigUserRWPublic() || varp->sampled()) {
                triggers.insert(firstIterTrigger);
            }
            // Add other triggers
            ܐܢ (varp->isWrittenByDpi())
                triggers.insert(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
            ܐܢ (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
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
            AstNodeStmt* ثابت stmtp = trigKit.newCompBaseCall();
            ܐܢ (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), ܫܪܪܐ));
            ܦܢܐ stmtp;
        }(),
        // Work statements: Invoke the 'ico' function
        util::callVoidFunc(icoFuncp));

    // Release temporary input change detect SenTrees
    لكل (AstSenTree* ثابت senTreep : icoChangeSenTreeps)
        VL_DO_DANGLING(senTreep->deleteTree(), senTreep);
    icoChangeSenTreeps.clear();
}

//============================================================================
// EvalKit groups items that have to be passed to createEval() for a given eval region

بنية EvalKit final {
    // The AstVarScope representing the region's trigger vector
    AstVarScope* ثابت m_vscp = nullptr;
    // The AstCFunc that evaluates the region's logic
    AstCFunc* ثابت m_funcp = nullptr;
    // Is this kit used/required?
    منطقي empty() ثابت { ܦܢܐ !m_funcp; }
};

//============================================================================
// Create the evaluation function of each region of a time step

فراغ createEval(AstNetlist* netlistp,  //
                ثابت TriggerKit& trigKit,  //
                ثابت EvalKit& actKit,  //
                ثابت EvalKit& nbaKit,  //
                ثابت EvalKit& obsKit,  //
                ثابت EvalKit& reactKit,  //
                TimingKit& timingKit  //
) {
    FileLine* ثابت flp = netlistp->fileline();

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
            AstNodeStmt* stmtsp = trigKit.newCompBaseCall();
            AstNodeStmt* ثابت dumpp
                = stmtsp ? trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), ܫܪܪܐ) : nullptr;
            // Mark as ready for triggered awaits
            ܐܢ (timingReadyp) stmtsp = AstNode::addNext(stmtsp, timingReadyp->makeStmt());
            ܐܢ (AstVarScope* ثابت vscAccp = trigKit.vscAccp()) {
                stmtsp = AstNode::addNext(stmtsp, trigKit.newOrIntoCall(actKit.m_vscp, vscAccp));
            }
            stmtsp = AstNode::addNext(stmtsp, trigKit.newCompExtCall(nbaKit.m_vscp));
            stmtsp = AstNode::addNext(stmtsp, dumpp);
            // Latch the 'act' triggers under the 'nba' triggers
            stmtsp = AstNode::addNext(stmtsp, trigKit.newOrIntoCall(nbaKit.m_vscp, actKit.m_vscp));
            //
            ܦܢܐ stmtsp;
        }(),
        // Work statements
        [&]() {
            AstNodeStmt* workp = nullptr;
            ܐܢ (AstVarScope* ثابت actAccp = trigKit.vscAccp()) {
                AstCMethodHard* ثابت cCallp = جديد AstCMethodHard{
                    flp, جديد AstVarRef{flp, actAccp, VAccess::WRITE}, VCMethod::UNPACKED_FILL,
                    جديد AstConst{flp, AstConst::Unsized64{}, 0}};
                cCallp->dtypeSetVoid();
                workp = AstNode::addNext(workp, cCallp->makeStmt());
            }
            // Resume triggered timing schedulers
            ܐܢ (timingResumep) workp = AstNode::addNext(workp, timingResumep->makeStmt());
            // Invoke the 'act' function
            workp = AstNode::addNext(workp, util::callVoidFunc(actKit.m_funcp));
            //
            ܦܢܐ workp;
        }());

    // Create the 'inact' region
    createEvalRegion(  //
        netlistp, VEval::INACT, 0, trigKit,
        // Use explicit condition
        nullptr,
        [&]() -> AstNodeExpr* {
            ܐܢ (!delaySchedVscp) ܦܢܐ nullptr;
            // Run if any zero delays are pending
            AstNodeExpr* ثابت callp
                = جديد AstCMethodHard{flp, جديد AstVarRef{flp, delaySchedVscp, VAccess::READ},
                                     VCMethod::SCHED_AWAITING_ZERO_DELAY};
            callp->dtypeSetBit();
            ܦܢܐ callp;
        }(),
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> AstNodeStmt* {
            ܐܢ (!delaySchedVscp) {
                // Nothing to do if there are no delays at all in the design
                ܦܢܐ nullptr;
            } ܐܚܪܢܐ ܐܢ (v3Global.usesZeroDelay()) {
                // Resume processes watiting for #0 delay
                AstCMethodHard* ثابت callp = جديد AstCMethodHard{
                    flp, جديد AstVarRef{flp, delaySchedVscp, VAccess::READWRITE},
                    VCMethod::SCHED_RESUME_ZERO_DELAY};
                callp->dtypeSetVoid();
                ܦܢܐ callp->makeStmt();
            } ܐܚܪܢܐ {
                // Assumption was that the design doesn't use #0 delays.
                // Die at run-time if it does.
                AstCStmt* ثابت stmtp = جديد AstCStmt{flp};
                ثابت FileLine* ثابت locp = netlistp->topModulep()->fileline();
                ثابت std::string& file = VIdProtect::protect(locp->filename());
                ثابت std::string& line = std::to_string(locp->lineno());
                stmtp->add("VL_FATAL_MT(\"" + V3OutFormatter::quoteNameControls(file) + "\", "
                           + line
                           + ", \"\", \"ZERODLY: Design Verilated with '--no-sched-zero-delay', "
                           + "but #0 delay executed at runtime\");");
                ܦܢܐ stmtp;
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
            AstNodeStmt* workp = nullptr;
            // Latch the 'nba' trigger flags under the following region's trigger flags
            ܐܢ (!obsKit.empty()) {
                workp = trigKit.newOrIntoCall(obsKit.m_vscp, nbaKit.m_vscp);
            } ܐܚܪܢܐ ܐܢ (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, nbaKit.m_vscp);
            }
            // Invoke the 'nba' function
            workp = AstNode::addNext(workp, util::callVoidFunc(nbaKit.m_funcp));
            // Clear the 'nba' triggers
            workp = AstNode::addNext(workp, trigKit.newClearCall(nbaKit.m_vscp));
            //
            ܦܢܐ workp;
        }(),
        // Extra work (not conditional on having had a fired trigger)
        [&](AstVarScope* continuep) -> AstNodeStmt* {
            // Check if any dynamic NBAs are pending, if there are any in the design
            ܐܢ (!netlistp->nbaEventp()) ܦܢܐ nullptr;
            AstVarScope* ثابت nbaEventp = netlistp->nbaEventp();
            AstVarScope* ثابت nbaEventTriggerp = netlistp->nbaEventTriggerp();
            UASSERT(nbaEventTriggerp, "NBA event trigger var should exist");
            netlistp->nbaEventp(nullptr);
            netlistp->nbaEventTriggerp(nullptr);

            // If a dynamic NBA is pending, clear the pending flag and fire the ready event
            AstIf* ثابت ifp = جديد AstIf{flp, جديد AstVarRef{flp, nbaEventTriggerp, VAccess::READ}};
            ifp->addThensp(util::setVar(continuep, 1));
            ifp->addThensp(util::setVar(nbaEventTriggerp, 0));
            AstCMethodHard* ثابت firep = جديد AstCMethodHard{
                flp, جديد AstVarRef{flp, nbaEventp, VAccess::WRITE}, VCMethod::EVENT_FIRE};
            firep->dtypeSetVoid();
            ifp->addThensp(firep->makeStmt());
            ܦܢܐ ifp;
        });

    // Create the 'obs' region
    createEvalRegion(  //
        netlistp, VEval::OBS, 0, trigKit,
        // Use trigger
        obsKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> AstNodeStmt* {
            ܐܢ (obsKit.empty()) ܦܢܐ nullptr;
            AstNodeStmt* workp = nullptr;
            // Latch the Observed trigger flags under the Reactive trigger flags
            ܐܢ (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, obsKit.m_vscp);
            }
            // Invoke the 'obs' function
            workp = AstNode::addNext(workp, util::callVoidFunc(obsKit.m_funcp));
            // Clear the 'obs' triggers
            workp = AstNode::addNext(workp, trigKit.newClearCall(obsKit.m_vscp));
            //
            ܦܢܐ workp;
        }());

    // Create the 'react' region
    createEvalRegion(  //
        netlistp, VEval::REACT, 0, trigKit,
        // Use trigger
        reactKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> AstNodeStmt* {
            ܐܢ (reactKit.empty()) ܦܢܐ nullptr;
            // Invoke the 'react' function
            AstNodeStmt* workp = util::callVoidFunc(reactKit.m_funcp);
            // Clear the 'react' triggers
            workp = AstNode::addNext(workp, trigKit.newClearCall(reactKit.m_vscp));
            ܦܢܐ workp;
        }());
}

}  // namespace

//============================================================================
// Helper that builds virtual interface trigger sentrees

VirtIfaceTriggers::VscpTrigMap VirtIfaceTriggers::makeVscpToTrigMap(ثابت TriggerKit& trigKit,
                                                                    uint32_t firstIndex) ثابت {
    VscpTrigMap map;
    uint32_t index = firstIndex;
    لكل (ثابت auto& entry : m_triggers) {
        map.emplace(entry.m_vscp, trigKit.extraTriggerIndex(index));
        ++index;
    }
    ܦܢܐ map;
}

//============================================================================
// Top level entry-point to scheduling

فراغ schedule(AstNetlist* netlistp) {
    ثابت auto addSizeStat = [](ثابت string& name, ثابت LogicByScope& lbs) {
        uint64_t size = 0;
        lbs.foreachLogic([&](AstNode* nodep) { size += nodep->nodeCount(); });
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

    ܐܢ (v3Global.opt.stats()) {
        V3Stats::statsStage("sched-gather");
        addSizeStat("size of class: static", logicClasses.m_static);
        addSizeStat("size of class: initial", logicClasses.m_initial);
        addSizeStat("size of class: final", logicClasses.m_final);
    }

    // Step 4: Schedule static, initial and final logic classes in source order
    AstCFunc* ثابت staticp = createStatic(netlistp, logicClasses);
    ܐܢ (v3Global.opt.stats()) V3Stats::statsStage("sched-static");

    createInitial(netlistp, logicClasses);
    ܐܢ (v3Global.opt.stats()) V3Stats::statsStage("sched-initial");

    createFinal(netlistp, logicClasses);
    ܐܢ (v3Global.opt.stats()) V3Stats::statsStage("sched-final");

    // Step 5: Break combinational cycles by introducing hybrid logic
    // Note: breakCycles also removes corresponding logic from logicClasses.m_comb;
    logicClasses.m_hybrid = breakCycles(netlistp, logicClasses.m_comb);
    ܐܢ (v3Global.opt.stats()) {
        addSizeStat("size of class: clocked", logicClasses.m_clocked);
        addSizeStat("size of class: combinational", logicClasses.m_comb);
        addSizeStat("size of class: hybrid", logicClasses.m_hybrid);
        V3Stats::statsStage("sched-break-cycles");
    }

    // We pass around a single SenExprBuilder instance, as we only need one set of 'prev' variables
    // for edge/change detection in sensitivity expressions, which this keeps track of.
    AstTopScope* ثابت topScopep = netlistp->topScopep();
    AstScope* ثابت scopeTopp = topScopep->scopep();
    SenExprBuilder senExprBuilder{scopeTopp};

    // Step 6: Create 'settle' region that restores the combinational invariant
    createSettle(netlistp, staticp, senExprBuilder, logicClasses, cgRefBindings);
    ܐܢ (v3Global.opt.stats()) V3Stats::statsStage("sched-settle");

    // Step 7: Partition the clocked and combinational (including hybrid) logic into pre/act/nba.
    // All clocks (signals referenced in an AstSenTree) generated via a blocking assignment
    // (including combinationally generated signals) are computed within the act region.
    LogicRegions logicRegions
        = partition(logicClasses.m_clocked, logicClasses.m_comb, logicClasses.m_hybrid);
    logicRegions.m_obs = logicClasses.m_observed;
    logicRegions.m_react = logicClasses.m_reactive;
    ܐܢ (v3Global.opt.stats()) {
        addSizeStat("size of region: Active Pre", logicRegions.m_pre);
        addSizeStat("size of region: Active", logicRegions.m_act);
        addSizeStat("size of region: NBA", logicRegions.m_nba);
        addSizeStat("size of region: Observed", logicRegions.m_obs);
        addSizeStat("size of region: Reactive", logicRegions.m_react);
        V3Stats::statsStage("sched-partition");
    }

    // Step 8: Replicate combinational logic
    LogicReplicas logicReplicas = replicateLogic(logicRegions);
    ܐܢ (v3Global.opt.stats()) {
        addSizeStat("size of replicated logic: Input", logicReplicas.m_ico);
        addSizeStat("size of replicated logic: Active", logicReplicas.m_act);
        addSizeStat("size of replicated logic: NBA", logicReplicas.m_nba);
        addSizeStat("size of replicated logic: Observed", logicReplicas.m_obs);
        addSizeStat("size of replicated logic: Reactive", logicReplicas.m_react);
        V3Stats::statsStage("sched-replicate");
    }

    // Step 9: Create the input combinational logic
    createIcoRegion(netlistp, staticp, senExprBuilder, logicReplicas.m_ico, virtIfaceTriggers,
                    cgRefBindings);
    ܐܢ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-ico");

    // Step 10: Create the triggers
    AstVarScope* ثابت dpiExportTriggerVscp = netlistp->dpiExportTriggerp();
    netlistp->dpiExportTriggerp(nullptr);  // Finished with this here

    // We may have an extra trigger for variable updated in DPI exports
    TriggerKit::ExtraTriggers extraTriggers;
    ثابت uint32_t dpiExportTriggerIndex = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    ثابت uint32_t firstVifTriggerIndex = extraTriggers.size();
    لكل (ثابت auto& entry : virtIfaceTriggers.m_triggers) {
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
        = TriggerKit::create(netlistp, staticp, senExprBuilder, preTreeps, senTreeps, "act",
                             extraTriggers, ܕܓܠܐ, v3Global.usesTiming());

    // Add post updates from the timing kit
    ܐܢ (timingKit.m_postUpdates) trigKit.compBasep()->addStmtsp(timingKit.m_postUpdates);

    ܐܢ (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, staticp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);
    ܐܢ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-triggers");

    // Note: Experiments so far show that running the Act (or Ico) regions on
    // multiple threads is always a net loss, so only use multi-threading for
    // NBA for now. This can be revised if evidence is available that it would
    // be beneficial

    // Step 11: Create the 'act' region evaluation function

    // Add the trigger indices of the input logic
    addTriggerIndices(logicRegions.m_pre, trigKit, ܫܪܪܐ);
    addTriggerIndices(logicRegions.m_act, trigKit, ܕܓܠܐ);
    addTriggerIndices(logicReplicas.m_act, trigKit, ܕܓܠܐ);
    addTriggerIndices(timingKit.m_lbs, trigKit, ܕܓܠܐ);

    // External triggers of variables. Trigger indices are the same in all regions, so share.
    ثابت std::map<ثابت AstVarScope*, std::vector<uint32_t>> timingTriggers
        = timingKit.externalTriggerIndices(trigKit);
    ثابت auto& vifVscpToTrig = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);
    ثابت V3Order::ExternalTriggersProvider externalTriggers
        = [&](ثابت AstVarScope* vscp, std::set<uint32_t>& triggers) {
              auto it = timingTriggers.find(vscp);
              ܐܢ (it != timingTriggers.end())
                  triggers.insert(it->second.begin(), it->second.end());
              ܐܢ (vscp->varp()->isWrittenByDpi()) {
                  triggers.insert(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
              }
              ܐܢ (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                  addTriggeredIface(vscp, vifVscpToTrig, virtIfaceTriggers, triggers);
              }
          };

    AstCFunc* ثابت actFuncp
        = V3Order::order({&logicRegions.m_pre, &logicRegions.m_act, &logicReplicas.m_act}, trigKit,
                         trigKit.vscp(), cgRefBindings, "act", ܕܓܠܐ, ܕܓܠܐ, externalTriggers);
    util::splitCheck(actFuncp);
    ܐܢ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-act");

    ثابت EvalKit actKit{trigKit.vscp(), actFuncp};

    // Orders a region's logic and creates the region eval function
    ثابت auto order = [&](ثابت std::string& name,
                           ثابت std::vector<V3Sched::LogicByScope*>& logic) -> EvalKit {
        UINFO(2, "Scheduling " << name << " #logic = " << logic.size());
        AstVarScope* ثابت trigVscp = trigKit.newTrigVec(name);
        // Add the trigger indices of the input logic
        لكل (LogicByScope* lbs : logic) addTriggerIndices(*lbs, trigKit, ܕܓܠܐ);

        AstCFunc* ثابت funcp
            = V3Order::order(logic, trigKit, trigVscp, cgRefBindings, name,
                             name == "nba" && v3Global.opt.mtasks(), ܕܓܠܐ, externalTriggers);

        ܦܢܐ {trigVscp, funcp};
    };

    // Step 12: Create the 'nba' region evaluation function
    ثابت EvalKit nbaKit = order("nba", {&logicRegions.m_nba, &logicReplicas.m_nba});
    util::splitCheck(nbaKit.m_funcp);
    ܐܢ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-nba");

    // Orders a region's logic and creates the region eval function (only if there is any logic in
    // the region)
    ثابت auto orderIfNonEmpty
        = [&](ثابت std::string& name, ثابت std::vector<LogicByScope*>& logic) -> EvalKit {
        ܐܢ (logic[0]->empty())
            ܦܢܐ {};  // if region is empty, replica is supposed to be empty as well
        ثابت auto& kit = order(name, logic);
        ܐܢ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-" + name);
        ܦܢܐ kit;
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
    ܐܢ (AstCCall* ثابت readyp = timingKit.createReady(trigKit.vscp())) {
        staticp->addStmtsp(readyp->makeStmt());
        beforeTrigVisitor(netlistp, senExprBuilder, trigKit);
    } ܐܚܪܢܐ {
        // beforeTrigVisitor clears Sentree pointers in AstCAwaits (as these sentrees will get
        // deleted later) if there was no need to call it, SenTrees have to be cleaned manually
        netlistp->foreach([](AstCAwait* ثابت cAwaitp) { cAwaitp->clearSentreep(); });
    }
    ܐܢ (AstVarScope* ثابت trigAccp = trigKit.vscAccp()) {
        // Copy trigger vector to accumulator at the end of static initialziation so,
        // triggers fired during initialization persist to the first resume.
        ثابت AstUnpackArrayDType* ثابت trigAccDTypep
            = VN_AS(trigAccp->dtypep(), UnpackArrayDType);
        UASSERT_OBJ(
            trigAccDTypep->right() == 0, trigAccp,
            "Expected that trigger vector and accumulator start elements enumeration from 0");
        UASSERT_OBJ(trigAccDTypep->left() >= 0, trigAccp,
                    "Expected that trigger vector and accumulator has no negative indexes");
        FileLine* ثابت flp = trigAccp->fileline();
        AstVarScope* ثابت vscp = netlistp->topScopep()->createTemp("__Vi", 32);
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        loopp->addStmtsp(
            جديد AstAssign{flp,
                          جديد AstArraySel{flp, جديد AstVarRef{flp, trigAccp, VAccess::WRITE},
                                          جديد AstVarRef{flp, vscp, VAccess::READ}},
                          جديد AstArraySel{flp, جديد AstVarRef{flp, actKit.m_vscp, VAccess::READ},
                                          جديد AstVarRef{flp, vscp, VAccess::READ}}});
        loopp->addStmtsp(util::incrementVar(vscp));
        loopp->addStmtsp(جديد AstLoopTest{
            flp, loopp,
            جديد AstLte{flp, جديد AstVarRef{flp, vscp, VAccess::READ},
                       جديد AstConst{flp, AstConst::WidthedValue{}, 32,
                                    static_cast<uint32_t>(trigAccDTypep->left())}}});
        staticp->addStmtsp(loopp);
    }

    // Step 18: Clean up
    netlistp->clearStlFirstIterationp();

    ܐܢ (v3Global.opt.stats()) {
        // A sample() call resolved to the union over its covergroup's constructions reads more
        // than it can, which orders it against more logic than necessary
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per instance",
                         cgRefBindings.numExactCalls());
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per type",
                         cgRefBindings.numUnionCalls());
    }

    // Haven't split static initializer yet
    util::splitCheck(staticp);

    // Record eval stats
    netlistp->addEvalStats("sched");

    // Dump
    V3Global::dumpCheckGlobalTree("sched", 0, dumpTreeEitherLevel() >= 3);
}

}  // namespace V3Sched
