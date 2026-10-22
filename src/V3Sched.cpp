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

std::vector<ثابت Ast_Sen_ڤوکوق*> getSenTreesUsedBy(ثابت std::vector<ثابت LogicByScope*>& lbsps) {
    ثابت VNUser1InUse user1InUse;
    std::vector<ثابت Ast_Sen_ڤوکوق*> result;
    لكل (ثابت LogicByScope* ثابت lbsp : lbsps) {
        لكل (ثابت auto& entry : *lbsp) {
            AstActive* ثابت activep = entry.m_activep;
            Ast_Sen_ڤوکوق* ثابت senTreep = activep->sentreep();
            إذا (senTreep->user1SetOnce()) استمر;
            إذا (senTreep->hasClocked() || senTreep->hasHybrid()) result.تولق_back(senTreep);
        }
    }
    أرجع result;
}

فراغ addTriggerIndices(LogicByScope& lbs, ثابت TriggerKit& trigKit, منطقي pre) {
    لكل (LogicByScope::Entry& entry : lbs) {
        Ast_Sen_ڤوکوق* ثابت senTreep = entry.m_activep->sentreep();
        إذا (senTreep->hasCombo()) استمر;
        entry.m_triggers = trigKit.triggerIndices(senTreep, pre);
    }
}

// Add the VIF triggers that a given VarScope should be sensitive to.
// Case 2 (non-virtual interface read): sensitive to that specific VarScope's trigger only
// Case 3 (virtual interface read): sensitive to all member triggers of the same interface type
فراغ addTriggeredIface(ثابت AstVarScope* vscp, ثابت VirtIfaceTriggers::VscpTrigMap& vscpToTrig,
                       ثابت VirtIfaceTriggers& virtIfaceTriggers, std::set<uint32_t>& triggers) {
    إذا (vscp->varp()->isVirtIface()) {
        // Virtual interface variable -- sensitive to all member triggers of this interface type
        ثابت AstIface* ثابت ifacep = VN_AS(vscp->varp()->dtypep(), IfaceRefDType)->ifacep();
        لكل (ثابت auto& entry : virtIfaceTriggers.m_triggers) {
            إذا (entry.m_ifacep == ifacep) {
                ثابت auto it = vscpToTrig.find(entry.m_vscp);
                إذا (it != vscpToTrig.end()) triggers.سيسيڤ(it->second);
            }
        }
    } وإلا {
        // Non-virtual interface member -- sensitive to this VarScope's trigger only
        ثابت auto it = vscpToTrig.find(vscp);
        إذا (it != vscpToTrig.end()) triggers.سيسيڤ(it->second);
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
    Ast_نود_اوڠکاڤن* condp,  // Explicit condition that must be true to run 'phaseWorkp'
    Ast_نود_Stmt* phasePrepp,  // Prep statements run before checking triggers
    Ast_نود_Stmt* phaseWorkp,  // The work to do if anything triggered
    // Extra statements to run after the work, even if no triggers fired. This function is
    // passed a variable, which must be set to true if we must continue and loop again,
    // and must be unmodified otherwise.
    std::function<Ast_نود_Stmt*(AstVarScope*)> phaseExtra = [](AstVarScope*) { أرجع nullptr; }  //
) {
    UASSERT(!trigp || !condp, "Cannot use both 'trigp' and 'condp' in 'createEvalRegion'");
    UASSERT(!eval.firstIteration() || trigp,
            "Region without triggers cannot need a first iteration flag");

    // All work is under a trigger or condition, so with neither the region has
    // nothing to evaluate, and what we create below reduces to a no-op function.

    ثابت std::string tag = eval.tag();
    Ast_اتس_Scope* ثابت اتس_Scopep = netlistp->اتس_Scopep();
    File_باريس* ثابت flp = netlistp->fileline();

    // Populate the trigger dump entry point function
    إذا (trigp) {
        netlistp->dumpTriggersFuncp(eval)->addStmtsp(trigKit.newDumpCall(trigp, tag, خطأ));
    }

    AstCFunc* ثابت funcp = netlistp->evalFuncp(eval);

    // A flag is passed from the run-time eval loop if this is the first iteration of the
    // current loop
    إذا (eval.firstIteration()) {
        AstVarScope* ثابت firstIterArgp = util::newArgument(funcp, netlistp->findBitDType(),
                                                             "firstIteration", VDirection::ماسوقن);

        // Set the region's 'first iteration' trigger straight from the argument
        funcp->addStmtsp(trigKit.newExtraTriggerAssignment(firstIterArgp, firstIterTrigger));

        // Only 'stl' also needs a module level flag, for design logic that reads
        // it directly (see V3Timing). Those reads can be anywhere in the design,
        // hence module level. Always created, even if nothing reads it.
        // TODO: get rid of this special case
        إذا (eval == VEval::STL) {
            AstVarScope* ثابت firstIterp = netlistp->stlFirstIterationp();
            firstIterp->varp()->noReset(true);
            firstIterp->varp()->isInternal(true);
            funcp->addStmtsp(جديد AstAssign{flp, جديد AstVarRef{flp, firstIterp, VAccess::WRITE},
                                           جديد AstVarRef{flp, firstIterArgp, VAccess::READ}});
        }
    }

    {
        // Add the preparatory statements
        funcp->addStmtsp(phasePrepp);

        // The execute flag
        AstVarScope* ثابت executeFlagp = اتس_Scopep->createTemp("__V" + tag + "Execute", 1);
        executeFlagp->varp()->noReset(true);

        // If there is work in this region, execute it if any triggers fired
        إذا (phaseWorkp) {
            Ast_نود_اوڠکاڤن* ثابت lhsp = جديد AstVarRef{flp, executeFlagp, VAccess::WRITE};
            // If using explicit condition, that directly determines whether to execute,
            // otherwise check if any triggers are fired
            Ast_نود_اوڠکاڤن* ثابت rhsp = condp ? condp : trigKit.newAnySetCall(trigp);
            funcp->addStmtsp(جديد AstAssign{flp, lhsp, rhsp});

            // Add the work
            AstIf* ثابت ifp = جديد AstIf{flp, جديد AstVarRef{flp, executeFlagp, VAccess::READ}};
            ifp->addThensp(phaseWorkp);
            funcp->addStmtsp(ifp);
        }

        // Construct the extra statements
        Ast_نود_Stmt* ثابت extraWorkp = phaseExtra(executeFlagp);
        إذا (extraWorkp) funcp->addStmtsp(extraWorkp);

        // The function returns true iff it did run work
        Ast_نود_اوڠکاڤن* ثابت retp
            = phaseWorkp || extraWorkp
                  ? static_cast<Ast_نود_اوڠکاڤن*>(جديد AstVarRef{flp, executeFlagp, VAccess::READ})
                  : static_cast<Ast_نود_اوڠکاڤن*>(جديد AstConst{flp, AstConst::BitFalse{}});
        funcp->addStmtsp(جديد AstCReturn{flp, retp});
    }
}

//============================================================================
// Collect and classify all logic in the design

LogicClasses gatherLogicClasses(AstNetlist* netlistp) {
    LogicClasses result;

    netlistp->foreach([&](AstScope* scopep) {
        scopep->foreach([&](AstActive* activep) {
            Ast_Sen_ڤوکوق* ثابت senTreep = activep->sentreep();
            إذا (senTreep->hasStatic()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "static initializer with additional sensitivities");
                result.m_static.add(scopep, activep);
            } وإلا إذا (senTreep->hasInitial()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'initial' logic with additional sensitivities");
                result.m_initial.add(scopep, activep);
            } وإلا إذا (senTreep->hasFinal()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'final' logic with additional sensitivities");
                result.m_final.add(scopep, activep);
            } وإلا إذا (senTreep->hasCombo()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "combinational logic with additional sensitivities");
                إذا (VN_IS(activep->stmtsp(), AlwaysPostponed)) {
                    result.m_postponed.add(scopep, activep);
                } وإلا {
                    result.m_comb.add(scopep, activep);
                }
            } وإلا {
                UASSERT_OBJ(senTreep->hasClocked(), activep, "What else could it be?");
                إذا (VN_IS(activep->stmtsp(), AlwaysObserved)) {
                    result.m_observed.add(scopep, activep);
                } وإلا إذا (VN_IS(activep->stmtsp(), AlwaysReactive)) {
                    result.m_reactive.add(scopep, activep);
                } وإلا {
                    result.m_clocked.add(scopep, activep);
                }
            }
        });
    });

    أرجع result;
}

//============================================================================
// Simple ordering in source order

فراغ orderSequentially(AstCFunc* funcp, ثابت LogicByScope& lbs) {
    // Create new subfunc for scope
    ثابت auto createNewSubFuncp = [&](AstScope* ثابت scopep) {
        ثابت string subName{funcp->name() + "__" + scopep->nameDotless()};
        AstCFunc* ثابت subFuncp = جديد AstCFunc{scopep->fileline(), subName, scopep};
        subFuncp->isLoose(true);
        subFuncp->isConst(خطأ);
        subFuncp->declPrivate(true);
        subFuncp->slow(funcp->slow());
        scopep->addBlocksp(subFuncp);
        // Call it from the top function
        funcp->addStmtsp(util::callVoidFunc(subFuncp));
        أرجع subFuncp;
    };
    ثابت VNUser1InUse user1InUse;  // AstScope -> AstCFunc: the sub-function for the scope
    ثابت VNUser2InUse user2InUse;  // AstScope -> int: sub-function counter used for names
    لكل (ثابت auto& entry : lbs) {
        AstScope* ثابت scopep = entry.m_scopep;
        AstActive* ثابت activep = entry.m_activep;
        // Create a sub-function per scope so we can V3Combine them later
        إذا (!scopep->user1p()) scopep->user1p(createNewSubFuncp(scopep));
        // Add statements to sub-function
        لكل (Ast_نود *logicp = activep->stmtsp(), *nextp; logicp; logicp = nextp) {
            auto* subFuncp = VN_AS(scopep->user1p(), CFunc);
            nextp = logicp->nextp();
            إذا (Ast_نود_Procedure* ثابت procp = VN_CAST(logicp, نود_Procedure)) {
                إذا (Ast_نود* bodyp = procp->stmtsp()) {
                    bodyp->unlinkFrBackWithNext();
                    // If the process is suspendable, we need a separate function (a coroutine)
                    إذا (procp->isSuspendable()) {
                        funcp->slow(خطأ);
                        subFuncp = createNewSubFuncp(scopep);
                        subFuncp->name(subFuncp->name() + "__Vtiming__"
                                       + cvtToStr(scopep->user2Inc()));
                        subFuncp->rtnType("VlCoroutine");
                        إذا (VN_IS(procp, Always)) {
                            subFuncp->slow(خطأ);
                            File_باريس* ثابت flp = procp->fileline();
                            Ast_نود_اوڠکاڤن* ثابت condp = جديد AstCExpr{
                                flp, "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish())", 1};
                            AstLoop* ثابت loopp = جديد AstLoop{flp};
                            loopp->addStmtsp(جديد AstLoopTest{flp, loopp, condp});
                            loopp->addStmtsp(bodyp);
                            bodyp = loopp;
                        }
                    }
                    subFuncp->addStmtsp(bodyp);
                    إذا (procp->needProcess()) subFuncp->setNeedProcess();
                    // A coroutine holds a single process, so it is complete here
                    إذا (procp->isSuspendable()) util::splitCheck(subFuncp);
                }
            } وإلا {
                logicp->unlinkFrBack();
                subFuncp->addStmtsp(logicp);
            }
        }
        إذا (activep->backp()) activep->unlinkFrBack();
        VL_DO_DANGLING(activep->delete_ڤوکوق(), activep);
    }
    // Split sub-functions once complete
    لكل (ثابت auto& entry : lbs) {
        AstScope* ثابت scopep = entry.m_scopep;
        إذا (AstCFunc* ثابت subFuncp = VN_AS(scopep->user1p(), CFunc)) {
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
    إذا (orig.size() <= 1) {
        orderSequentially(funcp, orig);
        أرجع funcp;
    }

    // Level-based module sorting can reorder packages so that an importing
    // package runs before the imported one.  Re-sort package entries by source
    // file position to restore compilation order (IEEE 1800-2023 26.3).
    std::vector<size_t> indices(orig.size());
    لكل (size_t i = 0; i < orig.size(); ++i) indices[i] = i;
    std::stable_sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        ثابت Ast_نود_Module* ثابت modA = orig[a].m_scopep->modp();
        ثابت Ast_نود_Module* ثابت modB = orig[b].m_scopep->modp();
        ثابت منطقي isPkgA = VN_IS(modA, Package);
        ثابت منطقي isPkgB = VN_IS(modB, Package);
        إذا (isPkgA != isPkgB) أرجع isPkgA;  // Packages before non-packages
        إذا (isPkgA && isPkgB) {
            // Sort packages by source file position (compilation order)
            أرجع modA->fileline()->operatorCompare(*modB->fileline()) < 0;
        }
        أرجع خطأ;  // Both non-package: preserve original order
    });
    LogicByScope sorted;
    لكل (ثابت size_t i : indices) sorted.add(orig[i].m_scopep, orig[i].m_activep);

    orderSequentially(funcp, sorted);
    أرجع funcp;  // Not splitting yet as it is not final
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
        trigKit.add_نيلاي_Change_Trigger_Assignment(netlistp, initFuncp, entry.m_vscp, index);
        ++index;
    }
}

// Order the combinational logic to create the 'stl' region
فراغ createSettle(AstNetlist* netlistp, AstCFunc* ثابت initFuncp, Sen_اوڠکاڤن_Builder& sen_اوڠکاڤن_Bulider,
                  LogicClasses& logicClasses, ثابت CovergroupRefBindings& cgRefBindings) {
    // Clone, because ordering is destructive, but we still need them for the other regions
    LogicByScope comb = logicClasses.m_comb.clone();
    LogicByScope hybrid = logicClasses.m_hybrid.clone();

    // We have an extra trigger denoting this is the first iteration of the settle loop
    TriggerKit::ExtraTriggers extraTriggers;
    ثابت uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");

    // Gather the relevant sensitivity expressions and create the trigger kit
    ثابت auto& senTreeps = getSenTreesUsedBy({&comb, &hybrid});
    ثابت TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_اوڠکاڤن_Bulider, {},
                                                  senTreeps, "stl", extraTriggers, true, خطأ);

    // Add trigger indices (comb has none, so only do the hybrid)
    addTriggerIndices(hybrid, trigKit, خطأ);

    // First trigger is for pure combinational triggers (first iteration)
    ثابت uint32_t ماسوقن_Changed = trigKit.extraTriggerIndex(firstIterationTrigger);

    // Create and the body function
    AstCFunc* ثابت stlFuncp = V3Order::order(
        {&comb, &hybrid}, trigKit, trigKit.vscp(), cgRefBindings, "stl", خطأ, true,
        [=](ثابت AstVarScope*, std::set<uint32_t>& triggers) { triggers.سيسيڤ(ماسوقن_Changed); });
    util::splitCheck(stlFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::STL, firstIterationTrigger, trigKit,
        // Use trigger
        trigKit.vscp(), nullptr,
        // Explicit condition
        // Prep statements: Compute the current 'stl' triggers
        [&trigKit] {
            Ast_نود_Stmt* ثابت stmtp = trigKit.newCompBaseCall();
            إذا (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), true));
            أرجع stmtp;
        }(),
        // Work statements: Invoke the 'stl' function
        util::callVoidFunc(stlFuncp));
}

//============================================================================
// Order the replicated combinational logic to create the 'ico' region

فراغ createIcoRegion(AstNetlist* netlistp, AstCFunc* ثابت initFuncp,
                     Sen_اوڠکاڤن_Builder& sen_اوڠکاڤن_Builder, LogicByScope& logic,
                     ثابت VirtIfaceTriggers& virtIfaceTriggers,
                     ثابت CovergroupRefBindings& cgRefBindings) {
    // SystemC only: Any top level inputs feeding a combinational logic must be marked,
    // so we can make them sc_sensitive
    إذا (v3Global.opt.systemC()) {
        logic.foreachLogic([](Ast_نود* logicp) {
            logicp->foreach([](AstVarRef* refp) {
                إذا (refp->access().isWriteOnly()) أرجع;
                AstVarScope* ثابت vscp = refp->varScopep();
                إذا (vscp->scopep()->is_اتس() && vscp->varp()->isNonOutput()) {
                    vscp->varp()->scSensitive(true);
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
    std::unordered_map<ثابت AstVarScope*, Ast_Sen_ڤوکوق*> inp2changedp;
    std::vector<Ast_Sen_ڤوکوق*> icoChangeSenTreeps;
    إذا (v3Global.opt.fIcoChangeDetect().isTrue()) {
        File_باريس* ثابت flp = netlistp->fileline();
        AstScope* ثابت scopep = netlistp->اتس_Scopep()->scopep();
        لكل (AstVarScope* vscp = scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            // Only for top level ports, assume outputs don't change externally
            إذا (!vscp->varp()->isPrimaryInish()) استمر;
            // Don't do if written by the design - wouldn't update the change detect 'prev' value
            إذا (vscp->varp()->icoMaybeWritten()) استمر;
            // Don't do if forceable, as we can't see the actual value - this is belt and braces
            إذا (vscp->varp()->isForced()) استمر;
            // Can't handle unpacked arrays (they have special types when primary input)
            إذا (VN_IS(vscp->dtypep()->skipRefp(), UnpackArrayDType)) استمر;
            // Similarly to arrays, can't handle SystemC types
            إذا (vscp->varp()->isSc()) استمر;
            // Create a sen tree triggered when this input changes
            Ast_Sen_ڤوکوق*& senTreepr = inp2changedp[vscp];
            UASSERT_OBJ(!senTreepr, vscp, "Duplicate input change detect trigger");
            AstVarRef* ثابت refp = جديد AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* ثابت senItemp = جديد AstSenItem{flp, VEdgeType::ET_CHANGED, refp};
            senTreepr = جديد Ast_Sen_ڤوکوق{flp, senItemp};
            icoChangeSenTreeps.تولق_back(senTreepr);
        }
    }
    V3Stats::addStat("Scheduling, 'ico' change detect triggers", icoChangeSenTreeps.size());

    // Gather the relevant sensitivity expressions and create the trigger kit
    std::vector<ثابت Ast_Sen_ڤوکوق*> senTreeps = getSenTreesUsedBy({&logic});
    senTreeps.سيسيڤ(senTreeps.end(), icoChangeSenTreeps.begin(), icoChangeSenTreeps.end());
    ثابت TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_اوڠکاڤن_Builder, {},
                                                  senTreeps, "ico", extraTriggers, خطأ, خطأ);
    std::ignore = sen_اوڠکاڤن_Builder.getAndClearResults();

    إذا (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, initFuncp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);

    // Add trigger indices
    addTriggerIndices(logic, trigKit, خطأ);

    // The 'first iteration' trigger for top level inputs
    ثابت uint32_t firstIterTrigger = trigKit.extraTriggerIndex(firstIterationTrigger);

    ثابت auto& vifVscpToTrigIco
        = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);

    // Create and Order the body function
    AstCFunc* ثابت icoFuncp = V3Order::order(
        {&logic}, trigKit, trigKit.vscp(), cgRefBindings, "ico", خطأ, خطأ,
        [&](ثابت AstVarScope* vscp, std::set<uint32_t>& triggers) {
            AstVar* ثابت varp = vscp->varp();
            // If it has an explicit change detect trigger, use that,
            // otherwise fall back to using the 'first iteration' trigger
            auto it = inp2changedp.find(vscp);
            إذا (it != inp2changedp.end()) {
                triggers.سيسيڤ(trigKit.senItem2TrigIdx(it->second->sensesp()));
            } وإلا إذا (varp->isPrimaryInish() || varp->isSigUserRWPublic() || varp->sampled()) {
                triggers.سيسيڤ(firstIterTrigger);
            }
            // Add other triggers
            إذا (varp->isWrittenByDpi())
                triggers.سيسيڤ(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
            إذا (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
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
            Ast_نود_Stmt* ثابت stmtp = trigKit.newCompBaseCall();
            إذا (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), true));
            أرجع stmtp;
        }(),
        // Work statements: Invoke the 'ico' function
        util::callVoidFunc(icoFuncp));

    // Release temporary input change detect SenTrees
    لكل (Ast_Sen_ڤوکوق* ثابت senTreep : icoChangeSenTreeps)
        VL_DO_DANGLING(senTreep->delete_ڤوکوق(), senTreep);
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
    منطقي empty() ثابت { أرجع !m_funcp; }
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
    File_باريس* ثابت flp = netlistp->fileline();

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
            Ast_نود_Stmt* stmtsp = trigKit.newCompBaseCall();
            Ast_نود_Stmt* ثابت dumpp
                = stmtsp ? trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), true) : nullptr;
            // Mark as ready for triggered awaits
            إذا (timingReadyp) stmtsp = Ast_نود::addNext(stmtsp, timingReadyp->makeStmt());
            إذا (AstVarScope* ثابت vscAccp = trigKit.vscAccp()) {
                stmtsp = Ast_نود::addNext(stmtsp, trigKit.newOrIntoCall(actKit.m_vscp, vscAccp));
            }
            stmtsp = Ast_نود::addNext(stmtsp, trigKit.newCompExtCall(nbaKit.m_vscp));
            stmtsp = Ast_نود::addNext(stmtsp, dumpp);
            // Latch the 'act' triggers under the 'nba' triggers
            stmtsp = Ast_نود::addNext(stmtsp, trigKit.newOrIntoCall(nbaKit.m_vscp, actKit.m_vscp));
            //
            أرجع stmtsp;
        }(),
        // Work statements
        [&]() {
            Ast_نود_Stmt* workp = nullptr;
            إذا (AstVarScope* ثابت actAccp = trigKit.vscAccp()) {
                AstCMethodHard* ثابت cCallp = جديد AstCMethodHard{
                    flp, جديد AstVarRef{flp, actAccp, VAccess::WRITE}, VCMethod::UNPACKED_FILL,
                    جديد AstConst{flp, AstConst::Unsized64{}, 0}};
                cCallp->dtypeSetVoid();
                workp = Ast_نود::addNext(workp, cCallp->makeStmt());
            }
            // Resume triggered timing schedulers
            إذا (timingResumep) workp = Ast_نود::addNext(workp, timingResumep->makeStmt());
            // Invoke the 'act' function
            workp = Ast_نود::addNext(workp, util::callVoidFunc(actKit.m_funcp));
            //
            أرجع workp;
        }());

    // Create the 'inact' region
    createEvalRegion(  //
        netlistp, VEval::INACT, 0, trigKit,
        // Use explicit condition
        nullptr,
        [&]() -> Ast_نود_اوڠکاڤن* {
            إذا (!delaySchedVscp) أرجع nullptr;
            // Run if any zero delays are pending
            Ast_نود_اوڠکاڤن* ثابت callp
                = جديد AstCMethodHard{flp, جديد AstVarRef{flp, delaySchedVscp, VAccess::READ},
                                     VCMethod::SCHED_AWAITING_ZERO_DELAY};
            callp->dtypeSetBit();
            أرجع callp;
        }(),
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_نود_Stmt* {
            إذا (!delaySchedVscp) {
                // Nothing to do if there are no delays at all in the design
                أرجع nullptr;
            } وإلا إذا (v3Global.usesZeroDelay()) {
                // Resume processes watiting for #0 delay
                AstCMethodHard* ثابت callp = جديد AstCMethodHard{
                    flp, جديد AstVarRef{flp, delaySchedVscp, VAccess::READWRITE},
                    VCMethod::SCHED_RESUME_ZERO_DELAY};
                callp->dtypeSetVoid();
                أرجع callp->makeStmt();
            } وإلا {
                // Assumption was that the design doesn't use #0 delays.
                // Die at run-time if it does.
                AstCStmt* ثابت stmtp = جديد AstCStmt{flp};
                ثابت File_باريس* ثابت locp = netlistp->اتس_Modulep()->fileline();
                ثابت std::string& file = VIdProtect::protect(locp->filename());
                ثابت std::string& باريس = std::to_string(locp->lineno());
                stmtp->add("VL_FATAL_MT(\"" + V3OutFormatter::quoteNameControls(file) + "\", "
                           + باريس
                           + ", \"\", \"ZERODLY: Design Verilated with '--no-sched-zero-delay', "
                           + "but #0 delay executed at runtime\");");
                أرجع stmtp;
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
            Ast_نود_Stmt* workp = nullptr;
            // Latch the 'nba' trigger flags under the following region's trigger flags
            إذا (!obsKit.empty()) {
                workp = trigKit.newOrIntoCall(obsKit.m_vscp, nbaKit.m_vscp);
            } وإلا إذا (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, nbaKit.m_vscp);
            }
            // Invoke the 'nba' function
            workp = Ast_نود::addNext(workp, util::callVoidFunc(nbaKit.m_funcp));
            // Clear the 'nba' triggers
            workp = Ast_نود::addNext(workp, trigKit.newClearCall(nbaKit.m_vscp));
            //
            أرجع workp;
        }(),
        // Extra work (not conditional on having had a fired trigger)
        [&](AstVarScope* continuep) -> Ast_نود_Stmt* {
            // Check if any dynamic NBAs are pending, if there are any in the design
            إذا (!netlistp->nbaEventp()) أرجع nullptr;
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
            أرجع ifp;
        });

    // Create the 'obs' region
    createEvalRegion(  //
        netlistp, VEval::OBS, 0, trigKit,
        // Use trigger
        obsKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_نود_Stmt* {
            إذا (obsKit.empty()) أرجع nullptr;
            Ast_نود_Stmt* workp = nullptr;
            // Latch the Observed trigger flags under the Reactive trigger flags
            إذا (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, obsKit.m_vscp);
            }
            // Invoke the 'obs' function
            workp = Ast_نود::addNext(workp, util::callVoidFunc(obsKit.m_funcp));
            // Clear the 'obs' triggers
            workp = Ast_نود::addNext(workp, trigKit.newClearCall(obsKit.m_vscp));
            //
            أرجع workp;
        }());

    // Create the 'react' region
    createEvalRegion(  //
        netlistp, VEval::REACT, 0, trigKit,
        // Use trigger
        reactKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_نود_Stmt* {
            إذا (reactKit.empty()) أرجع nullptr;
            // Invoke the 'react' function
            Ast_نود_Stmt* workp = util::callVoidFunc(reactKit.m_funcp);
            // Clear the 'react' triggers
            workp = Ast_نود::addNext(workp, trigKit.newClearCall(reactKit.m_vscp));
            أرجع workp;
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
    أرجع map;
}

//============================================================================
// Top level entry-point to scheduling

فراغ schedule(AstNetlist* netlistp) {
    ثابت auto addSizeStat = [](ثابت string& name, ثابت LogicByScope& lbs) {
        uint64_t size = 0;
        lbs.foreachLogic([&](Ast_نود* nodep) { size += nodep->نود_کيراءن(); });
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

    إذا (v3Global.opt.stats()) {
        V3Stats::statsStage("sched-gather");
        addSizeStat("size of class: static", logicClasses.m_static);
        addSizeStat("size of class: initial", logicClasses.m_initial);
        addSizeStat("size of class: final", logicClasses.m_final);
    }

    // Step 4: Schedule static, initial and final logic classes in source order
    AstCFunc* ثابت staticp = createStatic(netlistp, logicClasses);
    إذا (v3Global.opt.stats()) V3Stats::statsStage("sched-static");

    createInitial(netlistp, logicClasses);
    إذا (v3Global.opt.stats()) V3Stats::statsStage("sched-initial");

    createFinal(netlistp, logicClasses);
    إذا (v3Global.opt.stats()) V3Stats::statsStage("sched-final");

    // Step 5: Break combinational cycles by introducing hybrid logic
    // Note: breakCycles also removes corresponding logic from logicClasses.m_comb;
    logicClasses.m_hybrid = breakCycles(netlistp, logicClasses.m_comb);
    إذا (v3Global.opt.stats()) {
        addSizeStat("size of class: clocked", logicClasses.m_clocked);
        addSizeStat("size of class: combinational", logicClasses.m_comb);
        addSizeStat("size of class: hybrid", logicClasses.m_hybrid);
        V3Stats::statsStage("sched-break-cycles");
    }

    // We pass around a single SenExprBuilder instance, as we only need one set of 'prev' variables
    // for edge/change detection in sensitivity expressions, which this keeps track of.
    Ast_اتس_Scope* ثابت اتس_Scopep = netlistp->اتس_Scopep();
    AstScope* ثابت scopeTopp = اتس_Scopep->scopep();
    Sen_اوڠکاڤن_Builder sen_اوڠکاڤن_Builder{scopeTopp};

    // Step 6: Create 'settle' region that restores the combinational invariant
    createSettle(netlistp, staticp, sen_اوڠکاڤن_Builder, logicClasses, cgRefBindings);
    إذا (v3Global.opt.stats()) V3Stats::statsStage("sched-settle");

    // Step 7: Partition the clocked and combinational (including hybrid) logic into pre/act/nba.
    // All clocks (signals referenced in an AstSenTree) generated via a blocking assignment
    // (including combinationally generated signals) are computed within the act region.
    LogicRegions logicRegions
        = partition(logicClasses.m_clocked, logicClasses.m_comb, logicClasses.m_hybrid);
    logicRegions.m_obs = logicClasses.m_observed;
    logicRegions.m_react = logicClasses.m_reactive;
    إذا (v3Global.opt.stats()) {
        addSizeStat("size of region: Active Pre", logicRegions.m_pre);
        addSizeStat("size of region: Active", logicRegions.m_act);
        addSizeStat("size of region: NBA", logicRegions.m_nba);
        addSizeStat("size of region: Observed", logicRegions.m_obs);
        addSizeStat("size of region: Reactive", logicRegions.m_react);
        V3Stats::statsStage("sched-partition");
    }

    // Step 8: Replicate combinational logic
    LogicReplicas logicReplicas = replicateLogic(logicRegions);
    إذا (v3Global.opt.stats()) {
        addSizeStat("size of replicated logic: Input", logicReplicas.m_ico);
        addSizeStat("size of replicated logic: Active", logicReplicas.m_act);
        addSizeStat("size of replicated logic: NBA", logicReplicas.m_nba);
        addSizeStat("size of replicated logic: Observed", logicReplicas.m_obs);
        addSizeStat("size of replicated logic: Reactive", logicReplicas.m_react);
        V3Stats::statsStage("sched-replicate");
    }

    // Step 9: Create the input combinational logic
    createIcoRegion(netlistp, staticp, sen_اوڠکاڤن_Builder, logicReplicas.m_ico, virtIfaceTriggers,
                    cgRefBindings);
    إذا (v3Global.opt.stats()) V3Stats::statsStage("sched-create-ico");

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
        = TriggerKit::create(netlistp, staticp, sen_اوڠکاڤن_Builder, preTreeps, senTreeps, "act",
                             extraTriggers, خطأ, v3Global.usesTiming());

    // Add post updates from the timing kit
    إذا (timingKit.m_postUpdates) trigKit.compBasep()->addStmtsp(timingKit.m_postUpdates);

    إذا (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, staticp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);
    إذا (v3Global.opt.stats()) V3Stats::statsStage("sched-create-triggers");

    // Note: Experiments so far show that running the Act (or Ico) regions on
    // multiple threads is always a net loss, so only use multi-threading for
    // NBA for now. This can be revised if evidence is available that it would
    // be beneficial

    // Step 11: Create the 'act' region evaluation function

    // Add the trigger indices of the input logic
    addTriggerIndices(logicRegions.m_pre, trigKit, true);
    addTriggerIndices(logicRegions.m_act, trigKit, خطأ);
    addTriggerIndices(logicReplicas.m_act, trigKit, خطأ);
    addTriggerIndices(timingKit.m_lbs, trigKit, خطأ);

    // External triggers of variables. Trigger indices are the same in all regions, so share.
    ثابت std::map<ثابت AstVarScope*, std::vector<uint32_t>> timingTriggers
        = timingKit.externalTriggerIndices(trigKit);
    ثابت auto& vifVscpToTrig = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);
    ثابت V3Order::ExternalTriggersProvider externalTriggers
        = [&](ثابت AstVarScope* vscp, std::set<uint32_t>& triggers) {
              auto it = timingTriggers.find(vscp);
              إذا (it != timingTriggers.end())
                  triggers.سيسيڤ(it->second.begin(), it->second.end());
              إذا (vscp->varp()->isWrittenByDpi()) {
                  triggers.سيسيڤ(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
              }
              إذا (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                  addTriggeredIface(vscp, vifVscpToTrig, virtIfaceTriggers, triggers);
              }
          };

    AstCFunc* ثابت actFuncp
        = V3Order::order({&logicRegions.m_pre, &logicRegions.m_act, &logicReplicas.m_act}, trigKit,
                         trigKit.vscp(), cgRefBindings, "act", خطأ, خطأ, externalTriggers);
    util::splitCheck(actFuncp);
    إذا (v3Global.opt.stats()) V3Stats::statsStage("sched-create-act");

    ثابت EvalKit actKit{trigKit.vscp(), actFuncp};

    // Orders a region's logic and creates the region eval function
    ثابت auto order = [&](ثابت std::string& name,
                           ثابت std::vector<V3Sched::LogicByScope*>& logic) -> EvalKit {
        UINFO(2, "Scheduling " << name << " #logic = " << logic.size());
        AstVarScope* ثابت trigVscp = trigKit.newTrigVec(name);
        // Add the trigger indices of the input logic
        لكل (LogicByScope* lbs : logic) addTriggerIndices(*lbs, trigKit, خطأ);

        AstCFunc* ثابت funcp
            = V3Order::order(logic, trigKit, trigVscp, cgRefBindings, name,
                             name == "nba" && v3Global.opt.mtasks(), خطأ, externalTriggers);

        أرجع {trigVscp, funcp};
    };

    // Step 12: Create the 'nba' region evaluation function
    ثابت EvalKit nbaKit = order("nba", {&logicRegions.m_nba, &logicReplicas.m_nba});
    util::splitCheck(nbaKit.m_funcp);
    إذا (v3Global.opt.stats()) V3Stats::statsStage("sched-create-nba");

    // Orders a region's logic and creates the region eval function (only if there is any logic in
    // the region)
    ثابت auto orderIfNonEmpty
        = [&](ثابت std::string& name, ثابت std::vector<LogicByScope*>& logic) -> EvalKit {
        إذا (logic[0]->empty())
            أرجع {};  // if region is empty, replica is supposed to be empty as well
        ثابت auto& kit = order(name, logic);
        إذا (v3Global.opt.stats()) V3Stats::statsStage("sched-create-" + name);
        أرجع kit;
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
    إذا (AstCCall* ثابت readyp = timingKit.createReady(trigKit.vscp())) {
        staticp->addStmtsp(readyp->makeStmt());
        beforeTrigVisitor(netlistp, sen_اوڠکاڤن_Builder, trigKit);
    } وإلا {
        // beforeTrigVisitor clears Sentree pointers in AstCAwaits (as these sentrees will get
        // deleted later) if there was no need to call it, SenTrees have to be cleaned manually
        netlistp->foreach([](AstCAwait* ثابت cAwaitp) { cAwaitp->clearSentreep(); });
    }
    إذا (AstVarScope* ثابت trigAccp = trigKit.vscAccp()) {
        // Copy trigger vector to accumulator at the end of static initialziation so,
        // triggers fired during initialization persist to the first resume.
        ثابت AstUnpackArrayDType* ثابت trigAccDTypep
            = VN_AS(trigAccp->dtypep(), UnpackArrayDType);
        UASSERT_OBJ(
            trigAccDTypep->کانن() == 0, trigAccp,
            "Expected that trigger vector and accumulator start elements enumeration from 0");
        UASSERT_OBJ(trigAccDTypep->کيري() >= 0, trigAccp,
                    "Expected that trigger vector and accumulator has no negative indexes");
        File_باريس* ثابت flp = trigAccp->fileline();
        AstVarScope* ثابت vscp = netlistp->اتس_Scopep()->createTemp("__Vi", 32);
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
                       جديد AstConst{flp, AstConst::Widthed_نيلاي{}, 32,
                                    static_cast<uint32_t>(trigAccDTypep->کيري())}}});
        staticp->addStmtsp(loopp);
    }

    // Step 18: Clean up
    netlistp->clearStlFirstIterationp();

    إذا (v3Global.opt.stats()) {
        // A sample() call resolved to the union over its covergroup's constructions reads more
        // than it can, which orders it against more logic than necessary
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per instance",
                         cgRefBindings.اڠک_Exact_Calls());
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per type",
                         cgRefBindings.اڠک_Union_Calls());
    }

    // Haven't split static initializer yet
    util::splitCheck(staticp);

    // Record eval stats
    netlistp->addEvalStats("sched");

    // Dump
    V3Global::dump_Check_Global_ڤوکوق("sched", 0, dump_ڤوکوق_Either_Level() >= 3);
}

}  // namespace V3Sched
