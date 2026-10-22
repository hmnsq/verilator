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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Sched.h"

#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3EmitCBase.h"
#அந்தர்பாவ "V3EmitV.h"
#அந்தர்பாவ "V3Order.h"
#அந்தர்பாவ "V3SenExprBuilder.h"
#அந்தர்பாவ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

நாமாகாஶ V3Sched {

நாமாகாஶ {

//============================================================================
// Utility functions

std::vector<மாறிலி Ast_Sen_மரம்*> getSenTreesUsedBy(மாறிலி std::vector<மாறிலி LogicByScope*>& lbsps) {
    மாறிலி VNUser1InUse user1InUse;
    std::vector<மாறிலி Ast_Sen_மரம்*> result;
    க்கு (மாறிலி LogicByScope* மாறிலி lbsp : lbsps) {
        க்கு (மாறிலி auto& entry : *lbsp) {
            AstActive* மாறிலி activep = entry.m_activep;
            Ast_Sen_மரம்* மாறிலி senTreep = activep->sentreep();
            எனில் (senTreep->user1SetOnce()) தொடர்;
            எனில் (senTreep->hasClocked() || senTreep->hasHybrid()) result.தள்ளு_back(senTreep);
        }
    }
    திரும்பு result;
}

வெறுமை addTriggerIndices(LogicByScope& lbs, மாறிலி TriggerKit& trigKit, தருக்க pre) {
    க்கு (LogicByScope::Entry& entry : lbs) {
        Ast_Sen_மரம்* மாறிலி senTreep = entry.m_activep->sentreep();
        எனில் (senTreep->hasCombo()) தொடர்;
        entry.m_triggers = trigKit.triggerIndices(senTreep, pre);
    }
}

// Add the VIF triggers that a given VarScope should be sensitive to.
// Case 2 (non-virtual interface read): sensitive to that specific VarScope's trigger only
// Case 3 (virtual interface read): sensitive to all member triggers of the same interface type
வெறுமை addTriggeredIface(மாறிலி AstVarScope* vscp, மாறிலி VirtIfaceTriggers::VscpTrigMap& vscpToTrig,
                       மாறிலி VirtIfaceTriggers& virtIfaceTriggers, std::set<uint32_t>& triggers) {
    எனில் (vscp->varp()->isVirtIface()) {
        // Virtual interface variable -- sensitive to all member triggers of this interface type
        மாறிலி AstIface* மாறிலி ifacep = VN_AS(vscp->varp()->dtypep(), IfaceRefDType)->ifacep();
        க்கு (மாறிலி auto& entry : virtIfaceTriggers.m_triggers) {
            எனில் (entry.m_ifacep == ifacep) {
                மாறிலி auto it = vscpToTrig.find(entry.m_vscp);
                எனில் (it != vscpToTrig.end()) triggers.செருகு(it->second);
            }
        }
    } இல்லையெனில் {
        // Non-virtual interface member -- sensitive to this VarScope's trigger only
        மாறிலி auto it = vscpToTrig.find(vscp);
        எனில் (it != vscpToTrig.end()) triggers.செருகு(it->second);
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
வெறுமை createEvalRegion(
    AstNetlist* netlistp,  //
    VEval eval,  // The entry point of the current region
    // Index of the region's 'first iteration' extra trigger, if it has one, otherwise ignored
    uint32_t firstIterTrigger,
    மாறிலி TriggerKit& trigKit,  // The trigger kit
    AstVarScope* trigp,  // The trigger vector - may be nullptr if no triggers or using 'condp'
    Ast_முனை_கோவை* condp,  // Explicit condition that must be true to run 'phaseWorkp'
    Ast_முனை_Stmt* phasePrepp,  // Prep statements run before checking triggers
    Ast_முனை_Stmt* phaseWorkp,  // The work to do if anything triggered
    // Extra statements to run after the work, even if no triggers fired. This function is
    // passed a variable, which must be set to true if we must continue and loop again,
    // and must be unmodified otherwise.
    std::function<Ast_முனை_Stmt*(AstVarScope*)> phaseExtra = [](AstVarScope*) { திரும்பு nullptr; }  //
) {
    UASSERT(!trigp || !condp, "Cannot use both 'trigp' and 'condp' in 'createEvalRegion'");
    UASSERT(!eval.firstIteration() || trigp,
            "Region without triggers cannot need a first iteration flag");

    // All work is under a trigger or condition, so with neither the region has
    // nothing to evaluate, and what we create below reduces to a no-op function.

    மாறிலி std::string tag = eval.tag();
    Ast_உச்சி_Scope* மாறிலி உச்சி_Scopep = netlistp->உச்சி_Scopep();
    File_வரி* மாறிலி flp = netlistp->fileline();

    // Populate the trigger dump entry point function
    எனில் (trigp) {
        netlistp->dumpTriggersFuncp(eval)->addStmtsp(trigKit.newDumpCall(trigp, tag, பொய்));
    }

    AstCFunc* மாறிலி funcp = netlistp->evalFuncp(eval);

    // A flag is passed from the run-time eval loop if this is the first iteration of the
    // current loop
    எனில் (eval.firstIteration()) {
        AstVarScope* மாறிலி firstIterArgp = util::newArgument(funcp, netlistp->findBitDType(),
                                                             "firstIteration", VDirection::உள்ளீடு);

        // Set the region's 'first iteration' trigger straight from the argument
        funcp->addStmtsp(trigKit.newExtraTriggerAssignment(firstIterArgp, firstIterTrigger));

        // Only 'stl' also needs a module level flag, for design logic that reads
        // it directly (see V3Timing). Those reads can be anywhere in the design,
        // hence module level. Always created, even if nothing reads it.
        // TODO: get rid of this special case
        எனில் (eval == VEval::STL) {
            AstVarScope* மாறிலி firstIterp = netlistp->stlFirstIterationp();
            firstIterp->varp()->noReset(உண்மை);
            firstIterp->varp()->isInternal(உண்மை);
            funcp->addStmtsp(புதிய AstAssign{flp, புதிய AstVarRef{flp, firstIterp, VAccess::WRITE},
                                           புதிய AstVarRef{flp, firstIterArgp, VAccess::READ}});
        }
    }

    {
        // Add the preparatory statements
        funcp->addStmtsp(phasePrepp);

        // The execute flag
        AstVarScope* மாறிலி executeFlagp = உச்சி_Scopep->createTemp("__V" + tag + "Execute", 1);
        executeFlagp->varp()->noReset(உண்மை);

        // If there is work in this region, execute it if any triggers fired
        எனில் (phaseWorkp) {
            Ast_முனை_கோவை* மாறிலி lhsp = புதிய AstVarRef{flp, executeFlagp, VAccess::WRITE};
            // If using explicit condition, that directly determines whether to execute,
            // otherwise check if any triggers are fired
            Ast_முனை_கோவை* மாறிலி rhsp = condp ? condp : trigKit.newAnySetCall(trigp);
            funcp->addStmtsp(புதிய AstAssign{flp, lhsp, rhsp});

            // Add the work
            AstIf* மாறிலி ifp = புதிய AstIf{flp, புதிய AstVarRef{flp, executeFlagp, VAccess::READ}};
            ifp->addThensp(phaseWorkp);
            funcp->addStmtsp(ifp);
        }

        // Construct the extra statements
        Ast_முனை_Stmt* மாறிலி extraWorkp = phaseExtra(executeFlagp);
        எனில் (extraWorkp) funcp->addStmtsp(extraWorkp);

        // The function returns true iff it did run work
        Ast_முனை_கோவை* மாறிலி retp
            = phaseWorkp || extraWorkp
                  ? static_cast<Ast_முனை_கோவை*>(புதிய AstVarRef{flp, executeFlagp, VAccess::READ})
                  : static_cast<Ast_முனை_கோவை*>(புதிய AstConst{flp, AstConst::BitFalse{}});
        funcp->addStmtsp(புதிய AstCReturn{flp, retp});
    }
}

//============================================================================
// Collect and classify all logic in the design

LogicClasses gatherLogicClasses(AstNetlist* netlistp) {
    LogicClasses result;

    netlistp->foreach([&](AstScope* scopep) {
        scopep->foreach([&](AstActive* activep) {
            Ast_Sen_மரம்* மாறிலி senTreep = activep->sentreep();
            எனில் (senTreep->hasStatic()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "static initializer with additional sensitivities");
                result.m_static.add(scopep, activep);
            } இல்லையெனில் எனில் (senTreep->hasInitial()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'initial' logic with additional sensitivities");
                result.m_initial.add(scopep, activep);
            } இல்லையெனில் எனில் (senTreep->hasFinal()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'final' logic with additional sensitivities");
                result.m_final.add(scopep, activep);
            } இல்லையெனில் எனில் (senTreep->hasCombo()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "combinational logic with additional sensitivities");
                எனில் (VN_IS(activep->stmtsp(), AlwaysPostponed)) {
                    result.m_postponed.add(scopep, activep);
                } இல்லையெனில் {
                    result.m_comb.add(scopep, activep);
                }
            } இல்லையெனில் {
                UASSERT_OBJ(senTreep->hasClocked(), activep, "What else could it be?");
                எனில் (VN_IS(activep->stmtsp(), AlwaysObserved)) {
                    result.m_observed.add(scopep, activep);
                } இல்லையெனில் எனில் (VN_IS(activep->stmtsp(), AlwaysReactive)) {
                    result.m_reactive.add(scopep, activep);
                } இல்லையெனில் {
                    result.m_clocked.add(scopep, activep);
                }
            }
        });
    });

    திரும்பு result;
}

//============================================================================
// Simple ordering in source order

வெறுமை orderSequentially(AstCFunc* funcp, மாறிலி LogicByScope& lbs) {
    // Create new subfunc for scope
    மாறிலி auto createNewSubFuncp = [&](AstScope* மாறிலி scopep) {
        மாறிலி string subName{funcp->name() + "__" + scopep->nameDotless()};
        AstCFunc* மாறிலி subFuncp = புதிய AstCFunc{scopep->fileline(), subName, scopep};
        subFuncp->isLoose(உண்மை);
        subFuncp->isConst(பொய்);
        subFuncp->declPrivate(உண்மை);
        subFuncp->slow(funcp->slow());
        scopep->addBlocksp(subFuncp);
        // Call it from the top function
        funcp->addStmtsp(util::callVoidFunc(subFuncp));
        திரும்பு subFuncp;
    };
    மாறிலி VNUser1InUse user1InUse;  // AstScope -> AstCFunc: the sub-function for the scope
    மாறிலி VNUser2InUse user2InUse;  // AstScope -> int: sub-function counter used for names
    க்கு (மாறிலி auto& entry : lbs) {
        AstScope* மாறிலி scopep = entry.m_scopep;
        AstActive* மாறிலி activep = entry.m_activep;
        // Create a sub-function per scope so we can V3Combine them later
        எனில் (!scopep->user1p()) scopep->user1p(createNewSubFuncp(scopep));
        // Add statements to sub-function
        க்கு (Ast_முனை *logicp = activep->stmtsp(), *nextp; logicp; logicp = nextp) {
            auto* subFuncp = VN_AS(scopep->user1p(), CFunc);
            nextp = logicp->nextp();
            எனில் (Ast_முனை_Procedure* மாறிலி procp = VN_CAST(logicp, முனை_Procedure)) {
                எனில் (Ast_முனை* bodyp = procp->stmtsp()) {
                    bodyp->unlinkFrBackWithNext();
                    // If the process is suspendable, we need a separate function (a coroutine)
                    எனில் (procp->isSuspendable()) {
                        funcp->slow(பொய்);
                        subFuncp = createNewSubFuncp(scopep);
                        subFuncp->name(subFuncp->name() + "__Vtiming__"
                                       + cvtToStr(scopep->user2Inc()));
                        subFuncp->rtnType("VlCoroutine");
                        எனில் (VN_IS(procp, Always)) {
                            subFuncp->slow(பொய்);
                            File_வரி* மாறிலி flp = procp->fileline();
                            Ast_முனை_கோவை* மாறிலி condp = புதிய AstCExpr{
                                flp, "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish())", 1};
                            AstLoop* மாறிலி loopp = புதிய AstLoop{flp};
                            loopp->addStmtsp(புதிய AstLoopTest{flp, loopp, condp});
                            loopp->addStmtsp(bodyp);
                            bodyp = loopp;
                        }
                    }
                    subFuncp->addStmtsp(bodyp);
                    எனில் (procp->needProcess()) subFuncp->setNeedProcess();
                    // A coroutine holds a single process, so it is complete here
                    எனில் (procp->isSuspendable()) util::splitCheck(subFuncp);
                }
            } இல்லையெனில் {
                logicp->unlinkFrBack();
                subFuncp->addStmtsp(logicp);
            }
        }
        எனில் (activep->backp()) activep->unlinkFrBack();
        VL_DO_DANGLING(activep->delete_மரம்(), activep);
    }
    // Split sub-functions once complete
    க்கு (மாறிலி auto& entry : lbs) {
        AstScope* மாறிலி scopep = entry.m_scopep;
        எனில் (AstCFunc* மாறிலி subFuncp = VN_AS(scopep->user1p(), CFunc)) {
            util::splitCheck(subFuncp);
            scopep->user1p(nullptr);
        }
    }
}

//============================================================================
// Create simply ordered functions

AstCFunc* createStatic(AstNetlist* netlistp, மாறிலி LogicClasses& logicClasses) {
    AstCFunc* மாறிலி funcp = netlistp->evalFuncp(VEval::STATIC);

    மாறிலி LogicByScope& orig = logicClasses.m_static;
    எனில் (orig.size() <= 1) {
        orderSequentially(funcp, orig);
        திரும்பு funcp;
    }

    // Level-based module sorting can reorder packages so that an importing
    // package runs before the imported one.  Re-sort package entries by source
    // file position to restore compilation order (IEEE 1800-2023 26.3).
    std::vector<size_t> indices(orig.size());
    க்கு (size_t i = 0; i < orig.size(); ++i) indices[i] = i;
    std::stable_sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        மாறிலி Ast_முனை_Module* மாறிலி modA = orig[a].m_scopep->modp();
        மாறிலி Ast_முனை_Module* மாறிலி modB = orig[b].m_scopep->modp();
        மாறிலி தருக்க isPkgA = VN_IS(modA, Package);
        மாறிலி தருக்க isPkgB = VN_IS(modB, Package);
        எனில் (isPkgA != isPkgB) திரும்பு isPkgA;  // Packages before non-packages
        எனில் (isPkgA && isPkgB) {
            // Sort packages by source file position (compilation order)
            திரும்பு modA->fileline()->operatorCompare(*modB->fileline()) < 0;
        }
        திரும்பு பொய்;  // Both non-package: preserve original order
    });
    LogicByScope sorted;
    க்கு (மாறிலி size_t i : indices) sorted.add(orig[i].m_scopep, orig[i].m_activep);

    orderSequentially(funcp, sorted);
    திரும்பு funcp;  // Not splitting yet as it is not final
}

வெறுமை createInitial(AstNetlist* netlistp, மாறிலி LogicClasses& logicClasses) {
    AstCFunc* மாறிலி funcp = netlistp->evalFuncp(VEval::INITIAL);
    orderSequentially(funcp, logicClasses.m_initial);
    util::splitCheck(funcp);
}

வெறுமை createPostponed(AstNetlist* netlistp, மாறிலி LogicClasses& logicClasses) {
    AstCFunc* மாறிலி funcp = netlistp->evalFuncp(VEval::POSTPONED);
    orderSequentially(funcp, logicClasses.m_postponed);
    util::splitCheck(funcp);
}

வெறுமை createFinal(AstNetlist* netlistp, மாறிலி LogicClasses& logicClasses) {
    AstCFunc* மாறிலி funcp = netlistp->evalFuncp(VEval::FINAL);
    orderSequentially(funcp, logicClasses.m_final);
    util::splitCheck(funcp);
}

//============================================================================
// Helper that creates virtual interface value-change triggers

வெறுமை addVirtIfaceTriggerAssignments(AstNetlist* netlistp, AstCFunc* initFuncp,
                                    மாறிலி VirtIfaceTriggers& virtIfaceTriggers,
                                    uint32_t firstIndex, மாறிலி TriggerKit& trigKit) {
    uint32_t index = firstIndex;
    க்கு (மாறிலி auto& entry : virtIfaceTriggers.m_triggers) {
        trigKit.add_மதிப்பு_Change_Trigger_Assignment(netlistp, initFuncp, entry.m_vscp, index);
        ++index;
    }
}

// Order the combinational logic to create the 'stl' region
வெறுமை createSettle(AstNetlist* netlistp, AstCFunc* மாறிலி initFuncp, Sen_கோவை_Builder& sen_கோவை_Bulider,
                  LogicClasses& logicClasses, மாறிலி CovergroupRefBindings& cgRefBindings) {
    // Clone, because ordering is destructive, but we still need them for the other regions
    LogicByScope comb = logicClasses.m_comb.clone();
    LogicByScope hybrid = logicClasses.m_hybrid.clone();

    // We have an extra trigger denoting this is the first iteration of the settle loop
    TriggerKit::ExtraTriggers extraTriggers;
    மாறிலி uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");

    // Gather the relevant sensitivity expressions and create the trigger kit
    மாறிலி auto& senTreeps = getSenTreesUsedBy({&comb, &hybrid});
    மாறிலி TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_கோவை_Bulider, {},
                                                  senTreeps, "stl", extraTriggers, உண்மை, பொய்);

    // Add trigger indices (comb has none, so only do the hybrid)
    addTriggerIndices(hybrid, trigKit, பொய்);

    // First trigger is for pure combinational triggers (first iteration)
    மாறிலி uint32_t உள்ளீடு_Changed = trigKit.extraTriggerIndex(firstIterationTrigger);

    // Create and the body function
    AstCFunc* மாறிலி stlFuncp = V3Order::order(
        {&comb, &hybrid}, trigKit, trigKit.vscp(), cgRefBindings, "stl", பொய், உண்மை,
        [=](மாறிலி AstVarScope*, std::set<uint32_t>& triggers) { triggers.செருகு(உள்ளீடு_Changed); });
    util::splitCheck(stlFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::STL, firstIterationTrigger, trigKit,
        // Use trigger
        trigKit.vscp(), nullptr,
        // Explicit condition
        // Prep statements: Compute the current 'stl' triggers
        [&trigKit] {
            Ast_முனை_Stmt* மாறிலி stmtp = trigKit.newCompBaseCall();
            எனில் (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), உண்மை));
            திரும்பு stmtp;
        }(),
        // Work statements: Invoke the 'stl' function
        util::callVoidFunc(stlFuncp));
}

//============================================================================
// Order the replicated combinational logic to create the 'ico' region

வெறுமை createIcoRegion(AstNetlist* netlistp, AstCFunc* மாறிலி initFuncp,
                     Sen_கோவை_Builder& sen_கோவை_Builder, LogicByScope& logic,
                     மாறிலி VirtIfaceTriggers& virtIfaceTriggers,
                     மாறிலி CovergroupRefBindings& cgRefBindings) {
    // SystemC only: Any top level inputs feeding a combinational logic must be marked,
    // so we can make them sc_sensitive
    எனில் (v3Global.opt.systemC()) {
        logic.foreachLogic([](Ast_முனை* logicp) {
            logicp->foreach([](AstVarRef* refp) {
                எனில் (refp->access().isWriteOnly()) திரும்பு;
                AstVarScope* மாறிலி vscp = refp->varScopep();
                எனில் (vscp->scopep()->is_உச்சி() && vscp->varp()->isNonOutput()) {
                    vscp->varp()->scSensitive(உண்மை);
                }
            });
        });
    }

    // We have some extra trigger denoting external conditions
    AstVarScope* மாறிலி dpiExportTriggerVscp = netlistp->dpiExportTriggerp();

    TriggerKit::ExtraTriggers extraTriggers;
    மாறிலி uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");
    மாறிலி uint32_t dpiExportTriggerIndex = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    மாறிலி uint32_t firstVifTriggerIndex = extraTriggers.size();
    க்கு (மாறிலி auto& entry : virtIfaceTriggers.m_triggers) {
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
    std::unordered_map<மாறிலி AstVarScope*, Ast_Sen_மரம்*> inp2changedp;
    std::vector<Ast_Sen_மரம்*> icoChangeSenTreeps;
    எனில் (v3Global.opt.fIcoChangeDetect().isTrue()) {
        File_வரி* மாறிலி flp = netlistp->fileline();
        AstScope* மாறிலி scopep = netlistp->உச்சி_Scopep()->scopep();
        க்கு (AstVarScope* vscp = scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            // Only for top level ports, assume outputs don't change externally
            எனில் (!vscp->varp()->isPrimaryInish()) தொடர்;
            // Don't do if written by the design - wouldn't update the change detect 'prev' value
            எனில் (vscp->varp()->icoMaybeWritten()) தொடர்;
            // Don't do if forceable, as we can't see the actual value - this is belt and braces
            எனில் (vscp->varp()->isForced()) தொடர்;
            // Can't handle unpacked arrays (they have special types when primary input)
            எனில் (VN_IS(vscp->dtypep()->skipRefp(), UnpackArrayDType)) தொடர்;
            // Similarly to arrays, can't handle SystemC types
            எனில் (vscp->varp()->isSc()) தொடர்;
            // Create a sen tree triggered when this input changes
            Ast_Sen_மரம்*& senTreepr = inp2changedp[vscp];
            UASSERT_OBJ(!senTreepr, vscp, "Duplicate input change detect trigger");
            AstVarRef* மாறிலி refp = புதிய AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* மாறிலி senItemp = புதிய AstSenItem{flp, VEdgeType::ET_CHANGED, refp};
            senTreepr = புதிய Ast_Sen_மரம்{flp, senItemp};
            icoChangeSenTreeps.தள்ளு_back(senTreepr);
        }
    }
    V3Stats::addStat("Scheduling, 'ico' change detect triggers", icoChangeSenTreeps.size());

    // Gather the relevant sensitivity expressions and create the trigger kit
    std::vector<மாறிலி Ast_Sen_மரம்*> senTreeps = getSenTreesUsedBy({&logic});
    senTreeps.செருகு(senTreeps.end(), icoChangeSenTreeps.begin(), icoChangeSenTreeps.end());
    மாறிலி TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_கோவை_Builder, {},
                                                  senTreeps, "ico", extraTriggers, பொய், பொய்);
    std::ignore = sen_கோவை_Builder.getAndClearResults();

    எனில் (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, initFuncp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);

    // Add trigger indices
    addTriggerIndices(logic, trigKit, பொய்);

    // The 'first iteration' trigger for top level inputs
    மாறிலி uint32_t firstIterTrigger = trigKit.extraTriggerIndex(firstIterationTrigger);

    மாறிலி auto& vifVscpToTrigIco
        = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);

    // Create and Order the body function
    AstCFunc* மாறிலி icoFuncp = V3Order::order(
        {&logic}, trigKit, trigKit.vscp(), cgRefBindings, "ico", பொய், பொய்,
        [&](மாறிலி AstVarScope* vscp, std::set<uint32_t>& triggers) {
            AstVar* மாறிலி varp = vscp->varp();
            // If it has an explicit change detect trigger, use that,
            // otherwise fall back to using the 'first iteration' trigger
            auto it = inp2changedp.find(vscp);
            எனில் (it != inp2changedp.end()) {
                triggers.செருகு(trigKit.senItem2TrigIdx(it->second->sensesp()));
            } இல்லையெனில் எனில் (varp->isPrimaryInish() || varp->isSigUserRWPublic() || varp->sampled()) {
                triggers.செருகு(firstIterTrigger);
            }
            // Add other triggers
            எனில் (varp->isWrittenByDpi())
                triggers.செருகு(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
            எனில் (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
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
            Ast_முனை_Stmt* மாறிலி stmtp = trigKit.newCompBaseCall();
            எனில் (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), உண்மை));
            திரும்பு stmtp;
        }(),
        // Work statements: Invoke the 'ico' function
        util::callVoidFunc(icoFuncp));

    // Release temporary input change detect SenTrees
    க்கு (Ast_Sen_மரம்* மாறிலி senTreep : icoChangeSenTreeps)
        VL_DO_DANGLING(senTreep->delete_மரம்(), senTreep);
    icoChangeSenTreeps.clear();
}

//============================================================================
// EvalKit groups items that have to be passed to createEval() for a given eval region

கட்டமைப்பு EvalKit final {
    // The AstVarScope representing the region's trigger vector
    AstVarScope* மாறிலி m_vscp = nullptr;
    // The AstCFunc that evaluates the region's logic
    AstCFunc* மாறிலி m_funcp = nullptr;
    // Is this kit used/required?
    தருக்க empty() மாறிலி { திரும்பு !m_funcp; }
};

//============================================================================
// Create the evaluation function of each region of a time step

வெறுமை createEval(AstNetlist* netlistp,  //
                மாறிலி TriggerKit& trigKit,  //
                மாறிலி EvalKit& actKit,  //
                மாறிலி EvalKit& nbaKit,  //
                மாறிலி EvalKit& obsKit,  //
                மாறிலி EvalKit& reactKit,  //
                TimingKit& timingKit  //
) {
    File_வரி* மாறிலி flp = netlistp->fileline();

    // Grab the delay scheduler variable, if any
    AstVarScope* மாறிலி delaySchedVscp = timingKit.getDelayScheduler(netlistp);

    // 'createResume' consumes the contents that 'createReady' needs, so do the right order
    AstCCall* மாறிலி timingReadyp = timingKit.createReady(trigKit.vscp());
    AstCCall* மாறிலி timingResumep = timingKit.createResume(trigKit.vscp());

    // Create the 'act' region
    createEvalRegion(  //
        netlistp, VEval::ACT, 0, trigKit,
        // Use trigger
        actKit.m_vscp, nullptr,
        // Prep statements
        [&]() {
            // Compute the current 'act' triggers - the NBA triggers are the latched value
            Ast_முனை_Stmt* stmtsp = trigKit.newCompBaseCall();
            Ast_முனை_Stmt* மாறிலி dumpp
                = stmtsp ? trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), உண்மை) : nullptr;
            // Mark as ready for triggered awaits
            எனில் (timingReadyp) stmtsp = Ast_முனை::addNext(stmtsp, timingReadyp->makeStmt());
            எனில் (AstVarScope* மாறிலி vscAccp = trigKit.vscAccp()) {
                stmtsp = Ast_முனை::addNext(stmtsp, trigKit.newOrIntoCall(actKit.m_vscp, vscAccp));
            }
            stmtsp = Ast_முனை::addNext(stmtsp, trigKit.newCompExtCall(nbaKit.m_vscp));
            stmtsp = Ast_முனை::addNext(stmtsp, dumpp);
            // Latch the 'act' triggers under the 'nba' triggers
            stmtsp = Ast_முனை::addNext(stmtsp, trigKit.newOrIntoCall(nbaKit.m_vscp, actKit.m_vscp));
            //
            திரும்பு stmtsp;
        }(),
        // Work statements
        [&]() {
            Ast_முனை_Stmt* workp = nullptr;
            எனில் (AstVarScope* மாறிலி actAccp = trigKit.vscAccp()) {
                AstCMethodHard* மாறிலி cCallp = புதிய AstCMethodHard{
                    flp, புதிய AstVarRef{flp, actAccp, VAccess::WRITE}, VCMethod::UNPACKED_FILL,
                    புதிய AstConst{flp, AstConst::Unsized64{}, 0}};
                cCallp->dtypeSetVoid();
                workp = Ast_முனை::addNext(workp, cCallp->makeStmt());
            }
            // Resume triggered timing schedulers
            எனில் (timingResumep) workp = Ast_முனை::addNext(workp, timingResumep->makeStmt());
            // Invoke the 'act' function
            workp = Ast_முனை::addNext(workp, util::callVoidFunc(actKit.m_funcp));
            //
            திரும்பு workp;
        }());

    // Create the 'inact' region
    createEvalRegion(  //
        netlistp, VEval::INACT, 0, trigKit,
        // Use explicit condition
        nullptr,
        [&]() -> Ast_முனை_கோவை* {
            எனில் (!delaySchedVscp) திரும்பு nullptr;
            // Run if any zero delays are pending
            Ast_முனை_கோவை* மாறிலி callp
                = புதிய AstCMethodHard{flp, புதிய AstVarRef{flp, delaySchedVscp, VAccess::READ},
                                     VCMethod::SCHED_AWAITING_ZERO_DELAY};
            callp->dtypeSetBit();
            திரும்பு callp;
        }(),
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_முனை_Stmt* {
            எனில் (!delaySchedVscp) {
                // Nothing to do if there are no delays at all in the design
                திரும்பு nullptr;
            } இல்லையெனில் எனில் (v3Global.usesZeroDelay()) {
                // Resume processes watiting for #0 delay
                AstCMethodHard* மாறிலி callp = புதிய AstCMethodHard{
                    flp, புதிய AstVarRef{flp, delaySchedVscp, VAccess::READWRITE},
                    VCMethod::SCHED_RESUME_ZERO_DELAY};
                callp->dtypeSetVoid();
                திரும்பு callp->makeStmt();
            } இல்லையெனில் {
                // Assumption was that the design doesn't use #0 delays.
                // Die at run-time if it does.
                AstCStmt* மாறிலி stmtp = புதிய AstCStmt{flp};
                மாறிலி File_வரி* மாறிலி locp = netlistp->உச்சி_Modulep()->fileline();
                மாறிலி std::string& file = VIdProtect::protect(locp->filename());
                மாறிலி std::string& வரி = std::to_string(locp->lineno());
                stmtp->add("VL_FATAL_MT(\"" + V3OutFormatter::quoteNameControls(file) + "\", "
                           + வரி
                           + ", \"\", \"ZERODLY: Design Verilated with '--no-sched-zero-delay', "
                           + "but #0 delay executed at runtime\");");
                திரும்பு stmtp;
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
            Ast_முனை_Stmt* workp = nullptr;
            // Latch the 'nba' trigger flags under the following region's trigger flags
            எனில் (!obsKit.empty()) {
                workp = trigKit.newOrIntoCall(obsKit.m_vscp, nbaKit.m_vscp);
            } இல்லையெனில் எனில் (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, nbaKit.m_vscp);
            }
            // Invoke the 'nba' function
            workp = Ast_முனை::addNext(workp, util::callVoidFunc(nbaKit.m_funcp));
            // Clear the 'nba' triggers
            workp = Ast_முனை::addNext(workp, trigKit.newClearCall(nbaKit.m_vscp));
            //
            திரும்பு workp;
        }(),
        // Extra work (not conditional on having had a fired trigger)
        [&](AstVarScope* continuep) -> Ast_முனை_Stmt* {
            // Check if any dynamic NBAs are pending, if there are any in the design
            எனில் (!netlistp->nbaEventp()) திரும்பு nullptr;
            AstVarScope* மாறிலி nbaEventp = netlistp->nbaEventp();
            AstVarScope* மாறிலி nbaEventTriggerp = netlistp->nbaEventTriggerp();
            UASSERT(nbaEventTriggerp, "NBA event trigger var should exist");
            netlistp->nbaEventp(nullptr);
            netlistp->nbaEventTriggerp(nullptr);

            // If a dynamic NBA is pending, clear the pending flag and fire the ready event
            AstIf* மாறிலி ifp = புதிய AstIf{flp, புதிய AstVarRef{flp, nbaEventTriggerp, VAccess::READ}};
            ifp->addThensp(util::setVar(continuep, 1));
            ifp->addThensp(util::setVar(nbaEventTriggerp, 0));
            AstCMethodHard* மாறிலி firep = புதிய AstCMethodHard{
                flp, புதிய AstVarRef{flp, nbaEventp, VAccess::WRITE}, VCMethod::EVENT_FIRE};
            firep->dtypeSetVoid();
            ifp->addThensp(firep->makeStmt());
            திரும்பு ifp;
        });

    // Create the 'obs' region
    createEvalRegion(  //
        netlistp, VEval::OBS, 0, trigKit,
        // Use trigger
        obsKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_முனை_Stmt* {
            எனில் (obsKit.empty()) திரும்பு nullptr;
            Ast_முனை_Stmt* workp = nullptr;
            // Latch the Observed trigger flags under the Reactive trigger flags
            எனில் (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, obsKit.m_vscp);
            }
            // Invoke the 'obs' function
            workp = Ast_முனை::addNext(workp, util::callVoidFunc(obsKit.m_funcp));
            // Clear the 'obs' triggers
            workp = Ast_முனை::addNext(workp, trigKit.newClearCall(obsKit.m_vscp));
            //
            திரும்பு workp;
        }());

    // Create the 'react' region
    createEvalRegion(  //
        netlistp, VEval::REACT, 0, trigKit,
        // Use trigger
        reactKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_முனை_Stmt* {
            எனில் (reactKit.empty()) திரும்பு nullptr;
            // Invoke the 'react' function
            Ast_முனை_Stmt* workp = util::callVoidFunc(reactKit.m_funcp);
            // Clear the 'react' triggers
            workp = Ast_முனை::addNext(workp, trigKit.newClearCall(reactKit.m_vscp));
            திரும்பு workp;
        }());
}

}  // namespace

//============================================================================
// Helper that builds virtual interface trigger sentrees

VirtIfaceTriggers::VscpTrigMap VirtIfaceTriggers::makeVscpToTrigMap(மாறிலி TriggerKit& trigKit,
                                                                    uint32_t firstIndex) மாறிலி {
    VscpTrigMap map;
    uint32_t index = firstIndex;
    க்கு (மாறிலி auto& entry : m_triggers) {
        map.emplace(entry.m_vscp, trigKit.extraTriggerIndex(index));
        ++index;
    }
    திரும்பு map;
}

//============================================================================
// Top level entry-point to scheduling

வெறுமை schedule(AstNetlist* netlistp) {
    மாறிலி auto addSizeStat = [](மாறிலி string& name, மாறிலி LogicByScope& lbs) {
        uint64_t size = 0;
        lbs.foreachLogic([&](Ast_முனை* nodep) { size += nodep->முனை_மொத்தம்(); });
        V3Stats::addStat("Scheduling, " + name, size);
    };

    // Step 2: Prepare external domains for timing and virtual interfaces
    // Create extra triggers for virtual interfaces
    மாறிலி auto& virtIfaceTriggers = makeVirtIfaceTriggers(netlistp);
    // Resolve what covergroup reference formal arguments are bound to, which is visible here
    // but not from V3Order, where the reads through them must be modeled
    மாறிலி CovergroupRefBindings& cgRefBindings = makeCovergroupRefBindings(netlistp);
    // Prepare timing-related logic and external domains
    TimingKit timingKit = prepareTiming(netlistp);

    // Step 3: Gather and classify all logic in the design
    LogicClasses logicClasses = gatherLogicClasses(netlistp);

    எனில் (v3Global.opt.stats()) {
        V3Stats::statsStage("sched-gather");
        addSizeStat("size of class: static", logicClasses.m_static);
        addSizeStat("size of class: initial", logicClasses.m_initial);
        addSizeStat("size of class: final", logicClasses.m_final);
    }

    // Step 4: Schedule static, initial and final logic classes in source order
    AstCFunc* மாறிலி staticp = createStatic(netlistp, logicClasses);
    எனில் (v3Global.opt.stats()) V3Stats::statsStage("sched-static");

    createInitial(netlistp, logicClasses);
    எனில் (v3Global.opt.stats()) V3Stats::statsStage("sched-initial");

    createFinal(netlistp, logicClasses);
    எனில் (v3Global.opt.stats()) V3Stats::statsStage("sched-final");

    // Step 5: Break combinational cycles by introducing hybrid logic
    // Note: breakCycles also removes corresponding logic from logicClasses.m_comb;
    logicClasses.m_hybrid = breakCycles(netlistp, logicClasses.m_comb);
    எனில் (v3Global.opt.stats()) {
        addSizeStat("size of class: clocked", logicClasses.m_clocked);
        addSizeStat("size of class: combinational", logicClasses.m_comb);
        addSizeStat("size of class: hybrid", logicClasses.m_hybrid);
        V3Stats::statsStage("sched-break-cycles");
    }

    // We pass around a single SenExprBuilder instance, as we only need one set of 'prev' variables
    // for edge/change detection in sensitivity expressions, which this keeps track of.
    Ast_உச்சி_Scope* மாறிலி உச்சி_Scopep = netlistp->உச்சி_Scopep();
    AstScope* மாறிலி scopeTopp = உச்சி_Scopep->scopep();
    Sen_கோவை_Builder sen_கோவை_Builder{scopeTopp};

    // Step 6: Create 'settle' region that restores the combinational invariant
    createSettle(netlistp, staticp, sen_கோவை_Builder, logicClasses, cgRefBindings);
    எனில் (v3Global.opt.stats()) V3Stats::statsStage("sched-settle");

    // Step 7: Partition the clocked and combinational (including hybrid) logic into pre/act/nba.
    // All clocks (signals referenced in an AstSenTree) generated via a blocking assignment
    // (including combinationally generated signals) are computed within the act region.
    LogicRegions logicRegions
        = partition(logicClasses.m_clocked, logicClasses.m_comb, logicClasses.m_hybrid);
    logicRegions.m_obs = logicClasses.m_observed;
    logicRegions.m_react = logicClasses.m_reactive;
    எனில் (v3Global.opt.stats()) {
        addSizeStat("size of region: Active Pre", logicRegions.m_pre);
        addSizeStat("size of region: Active", logicRegions.m_act);
        addSizeStat("size of region: NBA", logicRegions.m_nba);
        addSizeStat("size of region: Observed", logicRegions.m_obs);
        addSizeStat("size of region: Reactive", logicRegions.m_react);
        V3Stats::statsStage("sched-partition");
    }

    // Step 8: Replicate combinational logic
    LogicReplicas logicReplicas = replicateLogic(logicRegions);
    எனில் (v3Global.opt.stats()) {
        addSizeStat("size of replicated logic: Input", logicReplicas.m_ico);
        addSizeStat("size of replicated logic: Active", logicReplicas.m_act);
        addSizeStat("size of replicated logic: NBA", logicReplicas.m_nba);
        addSizeStat("size of replicated logic: Observed", logicReplicas.m_obs);
        addSizeStat("size of replicated logic: Reactive", logicReplicas.m_react);
        V3Stats::statsStage("sched-replicate");
    }

    // Step 9: Create the input combinational logic
    createIcoRegion(netlistp, staticp, sen_கோவை_Builder, logicReplicas.m_ico, virtIfaceTriggers,
                    cgRefBindings);
    எனில் (v3Global.opt.stats()) V3Stats::statsStage("sched-create-ico");

    // Step 10: Create the triggers
    AstVarScope* மாறிலி dpiExportTriggerVscp = netlistp->dpiExportTriggerp();
    netlistp->dpiExportTriggerp(nullptr);  // Finished with this here

    // We may have an extra trigger for variable updated in DPI exports
    TriggerKit::ExtraTriggers extraTriggers;
    மாறிலி uint32_t dpiExportTriggerIndex = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    மாறிலி uint32_t firstVifTriggerIndex = extraTriggers.size();
    க்கு (மாறிலி auto& entry : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + entry.m_ifacep->name() + "."
                               + entry.m_memberp->name());
    }

    மாறிலி auto& preTreeps = getSenTreesUsedBy({&logicRegions.m_pre});
    மாறிலி auto& senTreeps = getSenTreesUsedBy({&logicRegions.m_act,  //
                                               &logicRegions.m_nba,  //
                                               &logicRegions.m_obs,  //
                                               &logicRegions.m_react,  //
                                               &timingKit.m_lbs});
    மாறிலி TriggerKit trigKit
        = TriggerKit::create(netlistp, staticp, sen_கோவை_Builder, preTreeps, senTreeps, "act",
                             extraTriggers, பொய், v3Global.usesTiming());

    // Add post updates from the timing kit
    எனில் (timingKit.m_postUpdates) trigKit.compBasep()->addStmtsp(timingKit.m_postUpdates);

    எனில் (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, staticp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);
    எனில் (v3Global.opt.stats()) V3Stats::statsStage("sched-create-triggers");

    // Note: Experiments so far show that running the Act (or Ico) regions on
    // multiple threads is always a net loss, so only use multi-threading for
    // NBA for now. This can be revised if evidence is available that it would
    // be beneficial

    // Step 11: Create the 'act' region evaluation function

    // Add the trigger indices of the input logic
    addTriggerIndices(logicRegions.m_pre, trigKit, உண்மை);
    addTriggerIndices(logicRegions.m_act, trigKit, பொய்);
    addTriggerIndices(logicReplicas.m_act, trigKit, பொய்);
    addTriggerIndices(timingKit.m_lbs, trigKit, பொய்);

    // External triggers of variables. Trigger indices are the same in all regions, so share.
    மாறிலி std::map<மாறிலி AstVarScope*, std::vector<uint32_t>> timingTriggers
        = timingKit.externalTriggerIndices(trigKit);
    மாறிலி auto& vifVscpToTrig = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);
    மாறிலி V3Order::ExternalTriggersProvider externalTriggers
        = [&](மாறிலி AstVarScope* vscp, std::set<uint32_t>& triggers) {
              auto it = timingTriggers.find(vscp);
              எனில் (it != timingTriggers.end())
                  triggers.செருகு(it->second.begin(), it->second.end());
              எனில் (vscp->varp()->isWrittenByDpi()) {
                  triggers.செருகு(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
              }
              எனில் (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                  addTriggeredIface(vscp, vifVscpToTrig, virtIfaceTriggers, triggers);
              }
          };

    AstCFunc* மாறிலி actFuncp
        = V3Order::order({&logicRegions.m_pre, &logicRegions.m_act, &logicReplicas.m_act}, trigKit,
                         trigKit.vscp(), cgRefBindings, "act", பொய், பொய், externalTriggers);
    util::splitCheck(actFuncp);
    எனில் (v3Global.opt.stats()) V3Stats::statsStage("sched-create-act");

    மாறிலி EvalKit actKit{trigKit.vscp(), actFuncp};

    // Orders a region's logic and creates the region eval function
    மாறிலி auto order = [&](மாறிலி std::string& name,
                           மாறிலி std::vector<V3Sched::LogicByScope*>& logic) -> EvalKit {
        UINFO(2, "Scheduling " << name << " #logic = " << logic.size());
        AstVarScope* மாறிலி trigVscp = trigKit.newTrigVec(name);
        // Add the trigger indices of the input logic
        க்கு (LogicByScope* lbs : logic) addTriggerIndices(*lbs, trigKit, பொய்);

        AstCFunc* மாறிலி funcp
            = V3Order::order(logic, trigKit, trigVscp, cgRefBindings, name,
                             name == "nba" && v3Global.opt.mtasks(), பொய், externalTriggers);

        திரும்பு {trigVscp, funcp};
    };

    // Step 12: Create the 'nba' region evaluation function
    மாறிலி EvalKit nbaKit = order("nba", {&logicRegions.m_nba, &logicReplicas.m_nba});
    util::splitCheck(nbaKit.m_funcp);
    எனில் (v3Global.opt.stats()) V3Stats::statsStage("sched-create-nba");

    // Orders a region's logic and creates the region eval function (only if there is any logic in
    // the region)
    மாறிலி auto orderIfNonEmpty
        = [&](மாறிலி std::string& name, மாறிலி std::vector<LogicByScope*>& logic) -> EvalKit {
        எனில் (logic[0]->empty())
            திரும்பு {};  // if region is empty, replica is supposed to be empty as well
        மாறிலி auto& kit = order(name, logic);
        எனில் (v3Global.opt.stats()) V3Stats::statsStage("sched-create-" + name);
        திரும்பு kit;
    };

    // Step 13: Create the 'obs' region evaluation function
    மாறிலி EvalKit obsKit = orderIfNonEmpty("obs", {&logicRegions.m_obs, &logicReplicas.m_obs});

    // Step 14: Create the 'react' region evaluation function
    மாறிலி EvalKit reactKit
        = orderIfNonEmpty("react", {&logicRegions.m_react, &logicReplicas.m_react});

    // Step 15: Create the 'postponed' region evaluation function
    createPostponed(netlistp, logicClasses);

    // Step 16: Populate the eval entry point function of each region of a time step
    createEval(netlistp, trigKit, actKit, nbaKit, obsKit, reactKit, timingKit);

    // Step 17: Add neccessary evaluation before awaits
    எனில் (AstCCall* மாறிலி readyp = timingKit.createReady(trigKit.vscp())) {
        staticp->addStmtsp(readyp->makeStmt());
        beforeTrigVisitor(netlistp, sen_கோவை_Builder, trigKit);
    } இல்லையெனில் {
        // beforeTrigVisitor clears Sentree pointers in AstCAwaits (as these sentrees will get
        // deleted later) if there was no need to call it, SenTrees have to be cleaned manually
        netlistp->foreach([](AstCAwait* மாறிலி cAwaitp) { cAwaitp->clearSentreep(); });
    }
    எனில் (AstVarScope* மாறிலி trigAccp = trigKit.vscAccp()) {
        // Copy trigger vector to accumulator at the end of static initialziation so,
        // triggers fired during initialization persist to the first resume.
        மாறிலி AstUnpackArrayDType* மாறிலி trigAccDTypep
            = VN_AS(trigAccp->dtypep(), UnpackArrayDType);
        UASSERT_OBJ(
            trigAccDTypep->வலது() == 0, trigAccp,
            "Expected that trigger vector and accumulator start elements enumeration from 0");
        UASSERT_OBJ(trigAccDTypep->இடது() >= 0, trigAccp,
                    "Expected that trigger vector and accumulator has no negative indexes");
        File_வரி* மாறிலி flp = trigAccp->fileline();
        AstVarScope* மாறிலி vscp = netlistp->உச்சி_Scopep()->createTemp("__Vi", 32);
        AstLoop* மாறிலி loopp = புதிய AstLoop{flp};
        loopp->addStmtsp(
            புதிய AstAssign{flp,
                          புதிய AstArraySel{flp, புதிய AstVarRef{flp, trigAccp, VAccess::WRITE},
                                          புதிய AstVarRef{flp, vscp, VAccess::READ}},
                          புதிய AstArraySel{flp, புதிய AstVarRef{flp, actKit.m_vscp, VAccess::READ},
                                          புதிய AstVarRef{flp, vscp, VAccess::READ}}});
        loopp->addStmtsp(util::incrementVar(vscp));
        loopp->addStmtsp(புதிய AstLoopTest{
            flp, loopp,
            புதிய AstLte{flp, புதிய AstVarRef{flp, vscp, VAccess::READ},
                       புதிய AstConst{flp, AstConst::Widthed_மதிப்பு{}, 32,
                                    static_cast<uint32_t>(trigAccDTypep->இடது())}}});
        staticp->addStmtsp(loopp);
    }

    // Step 18: Clean up
    netlistp->clearStlFirstIterationp();

    எனில் (v3Global.opt.stats()) {
        // A sample() call resolved to the union over its covergroup's constructions reads more
        // than it can, which orders it against more logic than necessary
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per instance",
                         cgRefBindings.இலக்கம்_Exact_Calls());
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per type",
                         cgRefBindings.இலக்கம்_Union_Calls());
    }

    // Haven't split static initializer yet
    util::splitCheck(staticp);

    // Record eval stats
    netlistp->addEvalStats("sched");

    // Dump
    V3Global::dump_Check_Global_மரம்("sched", 0, dump_மரம்_Either_Level() >= 3);
}

}  // namespace V3Sched
