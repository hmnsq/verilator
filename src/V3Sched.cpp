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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Sched.h"

#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3EmitCBase.h"
#અન્તર્ભાવ "V3EmitV.h"
#અન્તર્ભાવ "V3Order.h"
#અન્તર્ભાવ "V3SenExprBuilder.h"
#અન્તર્ભાવ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

નામાકાશ V3Sched {

નામાકાશ {

//============================================================================
// Utility functions

std::vector<અચલ Ast_Sen_વૃક્ષ*> getSenTreesUsedBy(અચલ std::vector<અચલ LogicByScope*>& lbsps) {
    અચલ VNUser1InUse user1InUse;
    std::vector<અચલ Ast_Sen_વૃક્ષ*> result;
    માટે (અચલ LogicByScope* અચલ lbsp : lbsps) {
        માટે (અચલ auto& entry : *lbsp) {
            AstActive* અચલ activep = entry.m_activep;
            Ast_Sen_વૃક્ષ* અચલ senTreep = activep->sentreep();
            જો (senTreep->user1SetOnce()) ચાલુ;
            જો (senTreep->hasClocked() || senTreep->hasHybrid()) result.ધકેલો_back(senTreep);
        }
    }
    પાછા result;
}

શૂન્ય addTriggerIndices(LogicByScope& lbs, અચલ TriggerKit& trigKit, તાર્કિક pre) {
    માટે (LogicByScope::Entry& entry : lbs) {
        Ast_Sen_વૃક્ષ* અચલ senTreep = entry.m_activep->sentreep();
        જો (senTreep->hasCombo()) ચાલુ;
        entry.m_triggers = trigKit.triggerIndices(senTreep, pre);
    }
}

// Add the VIF triggers that a given VarScope should be sensitive to.
// Case 2 (non-virtual interface read): sensitive to that specific VarScope's trigger only
// Case 3 (virtual interface read): sensitive to all member triggers of the same interface type
શૂન્ય addTriggeredIface(અચલ AstVarScope* vscp, અચલ VirtIfaceTriggers::VscpTrigMap& vscpToTrig,
                       અચલ VirtIfaceTriggers& virtIfaceTriggers, std::set<uint32_t>& triggers) {
    જો (vscp->varp()->isVirtIface()) {
        // Virtual interface variable -- sensitive to all member triggers of this interface type
        અચલ AstIface* અચલ ifacep = VN_AS(vscp->varp()->dtypep(), IfaceRefDType)->ifacep();
        માટે (અચલ auto& entry : virtIfaceTriggers.m_triggers) {
            જો (entry.m_ifacep == ifacep) {
                અચલ auto it = vscpToTrig.find(entry.m_vscp);
                જો (it != vscpToTrig.end()) triggers.ઉમેરો(it->second);
            }
        }
    } નહીંતર {
        // Non-virtual interface member -- sensitive to this VarScope's trigger only
        અચલ auto it = vscpToTrig.find(vscp);
        જો (it != vscpToTrig.end()) triggers.ઉમેરો(it->second);
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
શૂન્ય createEvalRegion(
    AstNetlist* netlistp,  //
    VEval eval,  // The entry point of the current region
    // Index of the region's 'first iteration' extra trigger, if it has one, otherwise ignored
    uint32_t firstIterTrigger,
    અચલ TriggerKit& trigKit,  // The trigger kit
    AstVarScope* trigp,  // The trigger vector - may be nullptr if no triggers or using 'condp'
    Ast_ગાંઠ_પદાવલિ* condp,  // Explicit condition that must be true to run 'phaseWorkp'
    Ast_ગાંઠ_Stmt* phasePrepp,  // Prep statements run before checking triggers
    Ast_ગાંઠ_Stmt* phaseWorkp,  // The work to do if anything triggered
    // Extra statements to run after the work, even if no triggers fired. This function is
    // passed a variable, which must be set to true if we must continue and loop again,
    // and must be unmodified otherwise.
    std::function<Ast_ગાંઠ_Stmt*(AstVarScope*)> phaseExtra = [](AstVarScope*) { પાછા nullptr; }  //
) {
    UASSERT(!trigp || !condp, "Cannot use both 'trigp' and 'condp' in 'createEvalRegion'");
    UASSERT(!eval.firstIteration() || trigp,
            "Region without triggers cannot need a first iteration flag");

    // All work is under a trigger or condition, so with neither the region has
    // nothing to evaluate, and what we create below reduces to a no-op function.

    અચલ std::string tag = eval.tag();
    Ast_ટોચ_Scope* અચલ ટોચ_Scopep = netlistp->ટોચ_Scopep();
    File_લીટી* અચલ flp = netlistp->fileline();

    // Populate the trigger dump entry point function
    જો (trigp) {
        netlistp->dumpTriggersFuncp(eval)->addStmtsp(trigKit.newDumpCall(trigp, tag, ખોટું));
    }

    AstCFunc* અચલ funcp = netlistp->evalFuncp(eval);

    // A flag is passed from the run-time eval loop if this is the first iteration of the
    // current loop
    જો (eval.firstIteration()) {
        AstVarScope* અચલ firstIterArgp = util::newArgument(funcp, netlistp->findBitDType(),
                                                             "firstIteration", VDirection::આગત);

        // Set the region's 'first iteration' trigger straight from the argument
        funcp->addStmtsp(trigKit.newExtraTriggerAssignment(firstIterArgp, firstIterTrigger));

        // Only 'stl' also needs a module level flag, for design logic that reads
        // it directly (see V3Timing). Those reads can be anywhere in the design,
        // hence module level. Always created, even if nothing reads it.
        // TODO: get rid of this special case
        જો (eval == VEval::STL) {
            AstVarScope* અચલ firstIterp = netlistp->stlFirstIterationp();
            firstIterp->varp()->noReset(સાચું);
            firstIterp->varp()->isInternal(સાચું);
            funcp->addStmtsp(નવું AstAssign{flp, નવું AstVarRef{flp, firstIterp, VAccess::WRITE},
                                           નવું AstVarRef{flp, firstIterArgp, VAccess::READ}});
        }
    }

    {
        // Add the preparatory statements
        funcp->addStmtsp(phasePrepp);

        // The execute flag
        AstVarScope* અચલ executeFlagp = ટોચ_Scopep->createTemp("__V" + tag + "Execute", 1);
        executeFlagp->varp()->noReset(સાચું);

        // If there is work in this region, execute it if any triggers fired
        જો (phaseWorkp) {
            Ast_ગાંઠ_પદાવલિ* અચલ lhsp = નવું AstVarRef{flp, executeFlagp, VAccess::WRITE};
            // If using explicit condition, that directly determines whether to execute,
            // otherwise check if any triggers are fired
            Ast_ગાંઠ_પદાવલિ* અચલ rhsp = condp ? condp : trigKit.newAnySetCall(trigp);
            funcp->addStmtsp(નવું AstAssign{flp, lhsp, rhsp});

            // Add the work
            AstIf* અચલ ifp = નવું AstIf{flp, નવું AstVarRef{flp, executeFlagp, VAccess::READ}};
            ifp->addThensp(phaseWorkp);
            funcp->addStmtsp(ifp);
        }

        // Construct the extra statements
        Ast_ગાંઠ_Stmt* અચલ extraWorkp = phaseExtra(executeFlagp);
        જો (extraWorkp) funcp->addStmtsp(extraWorkp);

        // The function returns true iff it did run work
        Ast_ગાંઠ_પદાવલિ* અચલ retp
            = phaseWorkp || extraWorkp
                  ? static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstVarRef{flp, executeFlagp, VAccess::READ})
                  : static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstConst{flp, AstConst::BitFalse{}});
        funcp->addStmtsp(નવું AstCReturn{flp, retp});
    }
}

//============================================================================
// Collect and classify all logic in the design

LogicClasses gatherLogicClasses(AstNetlist* netlistp) {
    LogicClasses result;

    netlistp->foreach([&](AstScope* scopep) {
        scopep->foreach([&](AstActive* activep) {
            Ast_Sen_વૃક્ષ* અચલ senTreep = activep->sentreep();
            જો (senTreep->hasStatic()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "static initializer with additional sensitivities");
                result.m_static.add(scopep, activep);
            } નહીંતર જો (senTreep->hasInitial()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'initial' logic with additional sensitivities");
                result.m_initial.add(scopep, activep);
            } નહીંતર જો (senTreep->hasFinal()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'final' logic with additional sensitivities");
                result.m_final.add(scopep, activep);
            } નહીંતર જો (senTreep->hasCombo()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "combinational logic with additional sensitivities");
                જો (VN_IS(activep->stmtsp(), AlwaysPostponed)) {
                    result.m_postponed.add(scopep, activep);
                } નહીંતર {
                    result.m_comb.add(scopep, activep);
                }
            } નહીંતર {
                UASSERT_OBJ(senTreep->hasClocked(), activep, "What else could it be?");
                જો (VN_IS(activep->stmtsp(), AlwaysObserved)) {
                    result.m_observed.add(scopep, activep);
                } નહીંતર જો (VN_IS(activep->stmtsp(), AlwaysReactive)) {
                    result.m_reactive.add(scopep, activep);
                } નહીંતર {
                    result.m_clocked.add(scopep, activep);
                }
            }
        });
    });

    પાછા result;
}

//============================================================================
// Simple ordering in source order

શૂન્ય orderSequentially(AstCFunc* funcp, અચલ LogicByScope& lbs) {
    // Create new subfunc for scope
    અચલ auto createNewSubFuncp = [&](AstScope* અચલ scopep) {
        અચલ string subName{funcp->name() + "__" + scopep->nameDotless()};
        AstCFunc* અચલ subFuncp = નવું AstCFunc{scopep->fileline(), subName, scopep};
        subFuncp->isLoose(સાચું);
        subFuncp->isConst(ખોટું);
        subFuncp->declPrivate(સાચું);
        subFuncp->slow(funcp->slow());
        scopep->addBlocksp(subFuncp);
        // Call it from the top function
        funcp->addStmtsp(util::callVoidFunc(subFuncp));
        પાછા subFuncp;
    };
    અચલ VNUser1InUse user1InUse;  // AstScope -> AstCFunc: the sub-function for the scope
    અચલ VNUser2InUse user2InUse;  // AstScope -> int: sub-function counter used for names
    માટે (અચલ auto& entry : lbs) {
        AstScope* અચલ scopep = entry.m_scopep;
        AstActive* અચલ activep = entry.m_activep;
        // Create a sub-function per scope so we can V3Combine them later
        જો (!scopep->user1p()) scopep->user1p(createNewSubFuncp(scopep));
        // Add statements to sub-function
        માટે (Ast_ગાંઠ *logicp = activep->stmtsp(), *nextp; logicp; logicp = nextp) {
            auto* subFuncp = VN_AS(scopep->user1p(), CFunc);
            nextp = logicp->nextp();
            જો (Ast_ગાંઠ_Procedure* અચલ procp = VN_CAST(logicp, ગાંઠ_Procedure)) {
                જો (Ast_ગાંઠ* bodyp = procp->stmtsp()) {
                    bodyp->unlinkFrBackWithNext();
                    // If the process is suspendable, we need a separate function (a coroutine)
                    જો (procp->isSuspendable()) {
                        funcp->slow(ખોટું);
                        subFuncp = createNewSubFuncp(scopep);
                        subFuncp->name(subFuncp->name() + "__Vtiming__"
                                       + cvtToStr(scopep->user2Inc()));
                        subFuncp->rtnType("VlCoroutine");
                        જો (VN_IS(procp, Always)) {
                            subFuncp->slow(ખોટું);
                            File_લીટી* અચલ flp = procp->fileline();
                            Ast_ગાંઠ_પદાવલિ* અચલ condp = નવું AstCExpr{
                                flp, "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish())", 1};
                            AstLoop* અચલ loopp = નવું AstLoop{flp};
                            loopp->addStmtsp(નવું AstLoopTest{flp, loopp, condp});
                            loopp->addStmtsp(bodyp);
                            bodyp = loopp;
                        }
                    }
                    subFuncp->addStmtsp(bodyp);
                    જો (procp->needProcess()) subFuncp->setNeedProcess();
                    // A coroutine holds a single process, so it is complete here
                    જો (procp->isSuspendable()) util::splitCheck(subFuncp);
                }
            } નહીંતર {
                logicp->unlinkFrBack();
                subFuncp->addStmtsp(logicp);
            }
        }
        જો (activep->backp()) activep->unlinkFrBack();
        VL_DO_DANGLING(activep->delete_વૃક્ષ(), activep);
    }
    // Split sub-functions once complete
    માટે (અચલ auto& entry : lbs) {
        AstScope* અચલ scopep = entry.m_scopep;
        જો (AstCFunc* અચલ subFuncp = VN_AS(scopep->user1p(), CFunc)) {
            util::splitCheck(subFuncp);
            scopep->user1p(nullptr);
        }
    }
}

//============================================================================
// Create simply ordered functions

AstCFunc* createStatic(AstNetlist* netlistp, અચલ LogicClasses& logicClasses) {
    AstCFunc* અચલ funcp = netlistp->evalFuncp(VEval::STATIC);

    અચલ LogicByScope& orig = logicClasses.m_static;
    જો (orig.size() <= 1) {
        orderSequentially(funcp, orig);
        પાછા funcp;
    }

    // Level-based module sorting can reorder packages so that an importing
    // package runs before the imported one.  Re-sort package entries by source
    // file position to restore compilation order (IEEE 1800-2023 26.3).
    std::vector<size_t> indices(orig.size());
    માટે (size_t i = 0; i < orig.size(); ++i) indices[i] = i;
    std::stable_sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        અચલ Ast_ગાંઠ_Module* અચલ modA = orig[a].m_scopep->modp();
        અચલ Ast_ગાંઠ_Module* અચલ modB = orig[b].m_scopep->modp();
        અચલ તાર્કિક isPkgA = VN_IS(modA, Package);
        અચલ તાર્કિક isPkgB = VN_IS(modB, Package);
        જો (isPkgA != isPkgB) પાછા isPkgA;  // Packages before non-packages
        જો (isPkgA && isPkgB) {
            // Sort packages by source file position (compilation order)
            પાછા modA->fileline()->operatorCompare(*modB->fileline()) < 0;
        }
        પાછા ખોટું;  // Both non-package: preserve original order
    });
    LogicByScope sorted;
    માટે (અચલ size_t i : indices) sorted.add(orig[i].m_scopep, orig[i].m_activep);

    orderSequentially(funcp, sorted);
    પાછા funcp;  // Not splitting yet as it is not final
}

શૂન્ય createInitial(AstNetlist* netlistp, અચલ LogicClasses& logicClasses) {
    AstCFunc* અચલ funcp = netlistp->evalFuncp(VEval::INITIAL);
    orderSequentially(funcp, logicClasses.m_initial);
    util::splitCheck(funcp);
}

શૂન્ય createPostponed(AstNetlist* netlistp, અચલ LogicClasses& logicClasses) {
    AstCFunc* અચલ funcp = netlistp->evalFuncp(VEval::POSTPONED);
    orderSequentially(funcp, logicClasses.m_postponed);
    util::splitCheck(funcp);
}

શૂન્ય createFinal(AstNetlist* netlistp, અચલ LogicClasses& logicClasses) {
    AstCFunc* અચલ funcp = netlistp->evalFuncp(VEval::FINAL);
    orderSequentially(funcp, logicClasses.m_final);
    util::splitCheck(funcp);
}

//============================================================================
// Helper that creates virtual interface value-change triggers

શૂન્ય addVirtIfaceTriggerAssignments(AstNetlist* netlistp, AstCFunc* initFuncp,
                                    અચલ VirtIfaceTriggers& virtIfaceTriggers,
                                    uint32_t firstIndex, અચલ TriggerKit& trigKit) {
    uint32_t index = firstIndex;
    માટે (અચલ auto& entry : virtIfaceTriggers.m_triggers) {
        trigKit.add_કિંમત_Change_Trigger_Assignment(netlistp, initFuncp, entry.m_vscp, index);
        ++index;
    }
}

// Order the combinational logic to create the 'stl' region
શૂન્ય createSettle(AstNetlist* netlistp, AstCFunc* અચલ initFuncp, Sen_પદાવલિ_Builder& sen_પદાવલિ_Bulider,
                  LogicClasses& logicClasses, અચલ CovergroupRefBindings& cgRefBindings) {
    // Clone, because ordering is destructive, but we still need them for the other regions
    LogicByScope comb = logicClasses.m_comb.clone();
    LogicByScope hybrid = logicClasses.m_hybrid.clone();

    // We have an extra trigger denoting this is the first iteration of the settle loop
    TriggerKit::ExtraTriggers extraTriggers;
    અચલ uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");

    // Gather the relevant sensitivity expressions and create the trigger kit
    અચલ auto& senTreeps = getSenTreesUsedBy({&comb, &hybrid});
    અચલ TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_પદાવલિ_Bulider, {},
                                                  senTreeps, "stl", extraTriggers, સાચું, ખોટું);

    // Add trigger indices (comb has none, so only do the hybrid)
    addTriggerIndices(hybrid, trigKit, ખોટું);

    // First trigger is for pure combinational triggers (first iteration)
    અચલ uint32_t આગત_Changed = trigKit.extraTriggerIndex(firstIterationTrigger);

    // Create and the body function
    AstCFunc* અચલ stlFuncp = V3Order::order(
        {&comb, &hybrid}, trigKit, trigKit.vscp(), cgRefBindings, "stl", ખોટું, સાચું,
        [=](અચલ AstVarScope*, std::set<uint32_t>& triggers) { triggers.ઉમેરો(આગત_Changed); });
    util::splitCheck(stlFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::STL, firstIterationTrigger, trigKit,
        // Use trigger
        trigKit.vscp(), nullptr,
        // Explicit condition
        // Prep statements: Compute the current 'stl' triggers
        [&trigKit] {
            Ast_ગાંઠ_Stmt* અચલ stmtp = trigKit.newCompBaseCall();
            જો (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), સાચું));
            પાછા stmtp;
        }(),
        // Work statements: Invoke the 'stl' function
        util::callVoidFunc(stlFuncp));
}

//============================================================================
// Order the replicated combinational logic to create the 'ico' region

શૂન્ય createIcoRegion(AstNetlist* netlistp, AstCFunc* અચલ initFuncp,
                     Sen_પદાવલિ_Builder& sen_પદાવલિ_Builder, LogicByScope& logic,
                     અચલ VirtIfaceTriggers& virtIfaceTriggers,
                     અચલ CovergroupRefBindings& cgRefBindings) {
    // SystemC only: Any top level inputs feeding a combinational logic must be marked,
    // so we can make them sc_sensitive
    જો (v3Global.opt.systemC()) {
        logic.foreachLogic([](Ast_ગાંઠ* logicp) {
            logicp->foreach([](AstVarRef* refp) {
                જો (refp->access().isWriteOnly()) પાછા;
                AstVarScope* અચલ vscp = refp->varScopep();
                જો (vscp->scopep()->is_ટોચ() && vscp->varp()->isNonOutput()) {
                    vscp->varp()->scSensitive(સાચું);
                }
            });
        });
    }

    // We have some extra trigger denoting external conditions
    AstVarScope* અચલ dpiExportTriggerVscp = netlistp->dpiExportTriggerp();

    TriggerKit::ExtraTriggers extraTriggers;
    અચલ uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");
    અચલ uint32_t dpiExportTriggerIndex = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    અચલ uint32_t firstVifTriggerIndex = extraTriggers.size();
    માટે (અચલ auto& entry : virtIfaceTriggers.m_triggers) {
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
    std::unordered_map<અચલ AstVarScope*, Ast_Sen_વૃક્ષ*> inp2changedp;
    std::vector<Ast_Sen_વૃક્ષ*> icoChangeSenTreeps;
    જો (v3Global.opt.fIcoChangeDetect().isTrue()) {
        File_લીટી* અચલ flp = netlistp->fileline();
        AstScope* અચલ scopep = netlistp->ટોચ_Scopep()->scopep();
        માટે (AstVarScope* vscp = scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            // Only for top level ports, assume outputs don't change externally
            જો (!vscp->varp()->isPrimaryInish()) ચાલુ;
            // Don't do if written by the design - wouldn't update the change detect 'prev' value
            જો (vscp->varp()->icoMaybeWritten()) ચાલુ;
            // Don't do if forceable, as we can't see the actual value - this is belt and braces
            જો (vscp->varp()->isForced()) ચાલુ;
            // Can't handle unpacked arrays (they have special types when primary input)
            જો (VN_IS(vscp->dtypep()->skipRefp(), UnpackArrayDType)) ચાલુ;
            // Similarly to arrays, can't handle SystemC types
            જો (vscp->varp()->isSc()) ચાલુ;
            // Create a sen tree triggered when this input changes
            Ast_Sen_વૃક્ષ*& senTreepr = inp2changedp[vscp];
            UASSERT_OBJ(!senTreepr, vscp, "Duplicate input change detect trigger");
            AstVarRef* અચલ refp = નવું AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* અચલ senItemp = નવું AstSenItem{flp, VEdgeType::ET_CHANGED, refp};
            senTreepr = નવું Ast_Sen_વૃક્ષ{flp, senItemp};
            icoChangeSenTreeps.ધકેલો_back(senTreepr);
        }
    }
    V3Stats::addStat("Scheduling, 'ico' change detect triggers", icoChangeSenTreeps.size());

    // Gather the relevant sensitivity expressions and create the trigger kit
    std::vector<અચલ Ast_Sen_વૃક્ષ*> senTreeps = getSenTreesUsedBy({&logic});
    senTreeps.ઉમેરો(senTreeps.end(), icoChangeSenTreeps.begin(), icoChangeSenTreeps.end());
    અચલ TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_પદાવલિ_Builder, {},
                                                  senTreeps, "ico", extraTriggers, ખોટું, ખોટું);
    std::ignore = sen_પદાવલિ_Builder.getAndClearResults();

    જો (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, initFuncp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);

    // Add trigger indices
    addTriggerIndices(logic, trigKit, ખોટું);

    // The 'first iteration' trigger for top level inputs
    અચલ uint32_t firstIterTrigger = trigKit.extraTriggerIndex(firstIterationTrigger);

    અચલ auto& vifVscpToTrigIco
        = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);

    // Create and Order the body function
    AstCFunc* અચલ icoFuncp = V3Order::order(
        {&logic}, trigKit, trigKit.vscp(), cgRefBindings, "ico", ખોટું, ખોટું,
        [&](અચલ AstVarScope* vscp, std::set<uint32_t>& triggers) {
            AstVar* અચલ varp = vscp->varp();
            // If it has an explicit change detect trigger, use that,
            // otherwise fall back to using the 'first iteration' trigger
            auto it = inp2changedp.find(vscp);
            જો (it != inp2changedp.end()) {
                triggers.ઉમેરો(trigKit.senItem2TrigIdx(it->second->sensesp()));
            } નહીંતર જો (varp->isPrimaryInish() || varp->isSigUserRWPublic() || varp->sampled()) {
                triggers.ઉમેરો(firstIterTrigger);
            }
            // Add other triggers
            જો (varp->isWrittenByDpi())
                triggers.ઉમેરો(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
            જો (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
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
            Ast_ગાંઠ_Stmt* અચલ stmtp = trigKit.newCompBaseCall();
            જો (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), સાચું));
            પાછા stmtp;
        }(),
        // Work statements: Invoke the 'ico' function
        util::callVoidFunc(icoFuncp));

    // Release temporary input change detect SenTrees
    માટે (Ast_Sen_વૃક્ષ* અચલ senTreep : icoChangeSenTreeps)
        VL_DO_DANGLING(senTreep->delete_વૃક્ષ(), senTreep);
    icoChangeSenTreeps.clear();
}

//============================================================================
// EvalKit groups items that have to be passed to createEval() for a given eval region

સંરચના EvalKit final {
    // The AstVarScope representing the region's trigger vector
    AstVarScope* અચલ m_vscp = nullptr;
    // The AstCFunc that evaluates the region's logic
    AstCFunc* અચલ m_funcp = nullptr;
    // Is this kit used/required?
    તાર્કિક empty() અચલ { પાછા !m_funcp; }
};

//============================================================================
// Create the evaluation function of each region of a time step

શૂન્ય createEval(AstNetlist* netlistp,  //
                અચલ TriggerKit& trigKit,  //
                અચલ EvalKit& actKit,  //
                અચલ EvalKit& nbaKit,  //
                અચલ EvalKit& obsKit,  //
                અચલ EvalKit& reactKit,  //
                TimingKit& timingKit  //
) {
    File_લીટી* અચલ flp = netlistp->fileline();

    // Grab the delay scheduler variable, if any
    AstVarScope* અચલ delaySchedVscp = timingKit.getDelayScheduler(netlistp);

    // 'createResume' consumes the contents that 'createReady' needs, so do the right order
    AstCCall* અચલ timingReadyp = timingKit.createReady(trigKit.vscp());
    AstCCall* અચલ timingResumep = timingKit.createResume(trigKit.vscp());

    // Create the 'act' region
    createEvalRegion(  //
        netlistp, VEval::ACT, 0, trigKit,
        // Use trigger
        actKit.m_vscp, nullptr,
        // Prep statements
        [&]() {
            // Compute the current 'act' triggers - the NBA triggers are the latched value
            Ast_ગાંઠ_Stmt* stmtsp = trigKit.newCompBaseCall();
            Ast_ગાંઠ_Stmt* અચલ dumpp
                = stmtsp ? trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), સાચું) : nullptr;
            // Mark as ready for triggered awaits
            જો (timingReadyp) stmtsp = Ast_ગાંઠ::addNext(stmtsp, timingReadyp->makeStmt());
            જો (AstVarScope* અચલ vscAccp = trigKit.vscAccp()) {
                stmtsp = Ast_ગાંઠ::addNext(stmtsp, trigKit.newOrIntoCall(actKit.m_vscp, vscAccp));
            }
            stmtsp = Ast_ગાંઠ::addNext(stmtsp, trigKit.newCompExtCall(nbaKit.m_vscp));
            stmtsp = Ast_ગાંઠ::addNext(stmtsp, dumpp);
            // Latch the 'act' triggers under the 'nba' triggers
            stmtsp = Ast_ગાંઠ::addNext(stmtsp, trigKit.newOrIntoCall(nbaKit.m_vscp, actKit.m_vscp));
            //
            પાછા stmtsp;
        }(),
        // Work statements
        [&]() {
            Ast_ગાંઠ_Stmt* workp = nullptr;
            જો (AstVarScope* અચલ actAccp = trigKit.vscAccp()) {
                AstCMethodHard* અચલ cCallp = નવું AstCMethodHard{
                    flp, નવું AstVarRef{flp, actAccp, VAccess::WRITE}, VCMethod::UNPACKED_FILL,
                    નવું AstConst{flp, AstConst::Unsized64{}, 0}};
                cCallp->dtypeSetVoid();
                workp = Ast_ગાંઠ::addNext(workp, cCallp->makeStmt());
            }
            // Resume triggered timing schedulers
            જો (timingResumep) workp = Ast_ગાંઠ::addNext(workp, timingResumep->makeStmt());
            // Invoke the 'act' function
            workp = Ast_ગાંઠ::addNext(workp, util::callVoidFunc(actKit.m_funcp));
            //
            પાછા workp;
        }());

    // Create the 'inact' region
    createEvalRegion(  //
        netlistp, VEval::INACT, 0, trigKit,
        // Use explicit condition
        nullptr,
        [&]() -> Ast_ગાંઠ_પદાવલિ* {
            જો (!delaySchedVscp) પાછા nullptr;
            // Run if any zero delays are pending
            Ast_ગાંઠ_પદાવલિ* અચલ callp
                = નવું AstCMethodHard{flp, નવું AstVarRef{flp, delaySchedVscp, VAccess::READ},
                                     VCMethod::SCHED_AWAITING_ZERO_DELAY};
            callp->dtypeSetBit();
            પાછા callp;
        }(),
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_ગાંઠ_Stmt* {
            જો (!delaySchedVscp) {
                // Nothing to do if there are no delays at all in the design
                પાછા nullptr;
            } નહીંતર જો (v3Global.usesZeroDelay()) {
                // Resume processes watiting for #0 delay
                AstCMethodHard* અચલ callp = નવું AstCMethodHard{
                    flp, નવું AstVarRef{flp, delaySchedVscp, VAccess::READWRITE},
                    VCMethod::SCHED_RESUME_ZERO_DELAY};
                callp->dtypeSetVoid();
                પાછા callp->makeStmt();
            } નહીંતર {
                // Assumption was that the design doesn't use #0 delays.
                // Die at run-time if it does.
                AstCStmt* અચલ stmtp = નવું AstCStmt{flp};
                અચલ File_લીટી* અચલ locp = netlistp->ટોચ_Modulep()->fileline();
                અચલ std::string& file = VIdProtect::protect(locp->filename());
                અચલ std::string& લીટી = std::to_string(locp->lineno());
                stmtp->add("VL_FATAL_MT(\"" + V3OutFormatter::quoteNameControls(file) + "\", "
                           + લીટી
                           + ", \"\", \"ZERODLY: Design Verilated with '--no-sched-zero-delay', "
                           + "but #0 delay executed at runtime\");");
                પાછા stmtp;
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
            Ast_ગાંઠ_Stmt* workp = nullptr;
            // Latch the 'nba' trigger flags under the following region's trigger flags
            જો (!obsKit.empty()) {
                workp = trigKit.newOrIntoCall(obsKit.m_vscp, nbaKit.m_vscp);
            } નહીંતર જો (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, nbaKit.m_vscp);
            }
            // Invoke the 'nba' function
            workp = Ast_ગાંઠ::addNext(workp, util::callVoidFunc(nbaKit.m_funcp));
            // Clear the 'nba' triggers
            workp = Ast_ગાંઠ::addNext(workp, trigKit.newClearCall(nbaKit.m_vscp));
            //
            પાછા workp;
        }(),
        // Extra work (not conditional on having had a fired trigger)
        [&](AstVarScope* continuep) -> Ast_ગાંઠ_Stmt* {
            // Check if any dynamic NBAs are pending, if there are any in the design
            જો (!netlistp->nbaEventp()) પાછા nullptr;
            AstVarScope* અચલ nbaEventp = netlistp->nbaEventp();
            AstVarScope* અચલ nbaEventTriggerp = netlistp->nbaEventTriggerp();
            UASSERT(nbaEventTriggerp, "NBA event trigger var should exist");
            netlistp->nbaEventp(nullptr);
            netlistp->nbaEventTriggerp(nullptr);

            // If a dynamic NBA is pending, clear the pending flag and fire the ready event
            AstIf* અચલ ifp = નવું AstIf{flp, નવું AstVarRef{flp, nbaEventTriggerp, VAccess::READ}};
            ifp->addThensp(util::setVar(continuep, 1));
            ifp->addThensp(util::setVar(nbaEventTriggerp, 0));
            AstCMethodHard* અચલ firep = નવું AstCMethodHard{
                flp, નવું AstVarRef{flp, nbaEventp, VAccess::WRITE}, VCMethod::EVENT_FIRE};
            firep->dtypeSetVoid();
            ifp->addThensp(firep->makeStmt());
            પાછા ifp;
        });

    // Create the 'obs' region
    createEvalRegion(  //
        netlistp, VEval::OBS, 0, trigKit,
        // Use trigger
        obsKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_ગાંઠ_Stmt* {
            જો (obsKit.empty()) પાછા nullptr;
            Ast_ગાંઠ_Stmt* workp = nullptr;
            // Latch the Observed trigger flags under the Reactive trigger flags
            જો (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, obsKit.m_vscp);
            }
            // Invoke the 'obs' function
            workp = Ast_ગાંઠ::addNext(workp, util::callVoidFunc(obsKit.m_funcp));
            // Clear the 'obs' triggers
            workp = Ast_ગાંઠ::addNext(workp, trigKit.newClearCall(obsKit.m_vscp));
            //
            પાછા workp;
        }());

    // Create the 'react' region
    createEvalRegion(  //
        netlistp, VEval::REACT, 0, trigKit,
        // Use trigger
        reactKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_ગાંઠ_Stmt* {
            જો (reactKit.empty()) પાછા nullptr;
            // Invoke the 'react' function
            Ast_ગાંઠ_Stmt* workp = util::callVoidFunc(reactKit.m_funcp);
            // Clear the 'react' triggers
            workp = Ast_ગાંઠ::addNext(workp, trigKit.newClearCall(reactKit.m_vscp));
            પાછા workp;
        }());
}

}  // namespace

//============================================================================
// Helper that builds virtual interface trigger sentrees

VirtIfaceTriggers::VscpTrigMap VirtIfaceTriggers::makeVscpToTrigMap(અચલ TriggerKit& trigKit,
                                                                    uint32_t firstIndex) અચલ {
    VscpTrigMap map;
    uint32_t index = firstIndex;
    માટે (અચલ auto& entry : m_triggers) {
        map.emplace(entry.m_vscp, trigKit.extraTriggerIndex(index));
        ++index;
    }
    પાછા map;
}

//============================================================================
// Top level entry-point to scheduling

શૂન્ય schedule(AstNetlist* netlistp) {
    અચલ auto addSizeStat = [](અચલ string& name, અચલ LogicByScope& lbs) {
        uint64_t size = 0;
        lbs.foreachLogic([&](Ast_ગાંઠ* nodep) { size += nodep->ગાંઠ_ગણતરી(); });
        V3Stats::addStat("Scheduling, " + name, size);
    };

    // Step 2: Prepare external domains for timing and virtual interfaces
    // Create extra triggers for virtual interfaces
    અચલ auto& virtIfaceTriggers = makeVirtIfaceTriggers(netlistp);
    // Resolve what covergroup reference formal arguments are bound to, which is visible here
    // but not from V3Order, where the reads through them must be modeled
    અચલ CovergroupRefBindings& cgRefBindings = makeCovergroupRefBindings(netlistp);
    // Prepare timing-related logic and external domains
    TimingKit timingKit = prepareTiming(netlistp);

    // Step 3: Gather and classify all logic in the design
    LogicClasses logicClasses = gatherLogicClasses(netlistp);

    જો (v3Global.opt.stats()) {
        V3Stats::statsStage("sched-gather");
        addSizeStat("size of class: static", logicClasses.m_static);
        addSizeStat("size of class: initial", logicClasses.m_initial);
        addSizeStat("size of class: final", logicClasses.m_final);
    }

    // Step 4: Schedule static, initial and final logic classes in source order
    AstCFunc* અચલ staticp = createStatic(netlistp, logicClasses);
    જો (v3Global.opt.stats()) V3Stats::statsStage("sched-static");

    createInitial(netlistp, logicClasses);
    જો (v3Global.opt.stats()) V3Stats::statsStage("sched-initial");

    createFinal(netlistp, logicClasses);
    જો (v3Global.opt.stats()) V3Stats::statsStage("sched-final");

    // Step 5: Break combinational cycles by introducing hybrid logic
    // Note: breakCycles also removes corresponding logic from logicClasses.m_comb;
    logicClasses.m_hybrid = breakCycles(netlistp, logicClasses.m_comb);
    જો (v3Global.opt.stats()) {
        addSizeStat("size of class: clocked", logicClasses.m_clocked);
        addSizeStat("size of class: combinational", logicClasses.m_comb);
        addSizeStat("size of class: hybrid", logicClasses.m_hybrid);
        V3Stats::statsStage("sched-break-cycles");
    }

    // We pass around a single SenExprBuilder instance, as we only need one set of 'prev' variables
    // for edge/change detection in sensitivity expressions, which this keeps track of.
    Ast_ટોચ_Scope* અચલ ટોચ_Scopep = netlistp->ટોચ_Scopep();
    AstScope* અચલ scopeTopp = ટોચ_Scopep->scopep();
    Sen_પદાવલિ_Builder sen_પદાવલિ_Builder{scopeTopp};

    // Step 6: Create 'settle' region that restores the combinational invariant
    createSettle(netlistp, staticp, sen_પદાવલિ_Builder, logicClasses, cgRefBindings);
    જો (v3Global.opt.stats()) V3Stats::statsStage("sched-settle");

    // Step 7: Partition the clocked and combinational (including hybrid) logic into pre/act/nba.
    // All clocks (signals referenced in an AstSenTree) generated via a blocking assignment
    // (including combinationally generated signals) are computed within the act region.
    LogicRegions logicRegions
        = partition(logicClasses.m_clocked, logicClasses.m_comb, logicClasses.m_hybrid);
    logicRegions.m_obs = logicClasses.m_observed;
    logicRegions.m_react = logicClasses.m_reactive;
    જો (v3Global.opt.stats()) {
        addSizeStat("size of region: Active Pre", logicRegions.m_pre);
        addSizeStat("size of region: Active", logicRegions.m_act);
        addSizeStat("size of region: NBA", logicRegions.m_nba);
        addSizeStat("size of region: Observed", logicRegions.m_obs);
        addSizeStat("size of region: Reactive", logicRegions.m_react);
        V3Stats::statsStage("sched-partition");
    }

    // Step 8: Replicate combinational logic
    LogicReplicas logicReplicas = replicateLogic(logicRegions);
    જો (v3Global.opt.stats()) {
        addSizeStat("size of replicated logic: Input", logicReplicas.m_ico);
        addSizeStat("size of replicated logic: Active", logicReplicas.m_act);
        addSizeStat("size of replicated logic: NBA", logicReplicas.m_nba);
        addSizeStat("size of replicated logic: Observed", logicReplicas.m_obs);
        addSizeStat("size of replicated logic: Reactive", logicReplicas.m_react);
        V3Stats::statsStage("sched-replicate");
    }

    // Step 9: Create the input combinational logic
    createIcoRegion(netlistp, staticp, sen_પદાવલિ_Builder, logicReplicas.m_ico, virtIfaceTriggers,
                    cgRefBindings);
    જો (v3Global.opt.stats()) V3Stats::statsStage("sched-create-ico");

    // Step 10: Create the triggers
    AstVarScope* અચલ dpiExportTriggerVscp = netlistp->dpiExportTriggerp();
    netlistp->dpiExportTriggerp(nullptr);  // Finished with this here

    // We may have an extra trigger for variable updated in DPI exports
    TriggerKit::ExtraTriggers extraTriggers;
    અચલ uint32_t dpiExportTriggerIndex = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    અચલ uint32_t firstVifTriggerIndex = extraTriggers.size();
    માટે (અચલ auto& entry : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + entry.m_ifacep->name() + "."
                               + entry.m_memberp->name());
    }

    અચલ auto& preTreeps = getSenTreesUsedBy({&logicRegions.m_pre});
    અચલ auto& senTreeps = getSenTreesUsedBy({&logicRegions.m_act,  //
                                               &logicRegions.m_nba,  //
                                               &logicRegions.m_obs,  //
                                               &logicRegions.m_react,  //
                                               &timingKit.m_lbs});
    અચલ TriggerKit trigKit
        = TriggerKit::create(netlistp, staticp, sen_પદાવલિ_Builder, preTreeps, senTreeps, "act",
                             extraTriggers, ખોટું, v3Global.usesTiming());

    // Add post updates from the timing kit
    જો (timingKit.m_postUpdates) trigKit.compBasep()->addStmtsp(timingKit.m_postUpdates);

    જો (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, staticp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);
    જો (v3Global.opt.stats()) V3Stats::statsStage("sched-create-triggers");

    // Note: Experiments so far show that running the Act (or Ico) regions on
    // multiple threads is always a net loss, so only use multi-threading for
    // NBA for now. This can be revised if evidence is available that it would
    // be beneficial

    // Step 11: Create the 'act' region evaluation function

    // Add the trigger indices of the input logic
    addTriggerIndices(logicRegions.m_pre, trigKit, સાચું);
    addTriggerIndices(logicRegions.m_act, trigKit, ખોટું);
    addTriggerIndices(logicReplicas.m_act, trigKit, ખોટું);
    addTriggerIndices(timingKit.m_lbs, trigKit, ખોટું);

    // External triggers of variables. Trigger indices are the same in all regions, so share.
    અચલ std::map<અચલ AstVarScope*, std::vector<uint32_t>> timingTriggers
        = timingKit.externalTriggerIndices(trigKit);
    અચલ auto& vifVscpToTrig = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);
    અચલ V3Order::ExternalTriggersProvider externalTriggers
        = [&](અચલ AstVarScope* vscp, std::set<uint32_t>& triggers) {
              auto it = timingTriggers.find(vscp);
              જો (it != timingTriggers.end())
                  triggers.ઉમેરો(it->second.begin(), it->second.end());
              જો (vscp->varp()->isWrittenByDpi()) {
                  triggers.ઉમેરો(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
              }
              જો (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                  addTriggeredIface(vscp, vifVscpToTrig, virtIfaceTriggers, triggers);
              }
          };

    AstCFunc* અચલ actFuncp
        = V3Order::order({&logicRegions.m_pre, &logicRegions.m_act, &logicReplicas.m_act}, trigKit,
                         trigKit.vscp(), cgRefBindings, "act", ખોટું, ખોટું, externalTriggers);
    util::splitCheck(actFuncp);
    જો (v3Global.opt.stats()) V3Stats::statsStage("sched-create-act");

    અચલ EvalKit actKit{trigKit.vscp(), actFuncp};

    // Orders a region's logic and creates the region eval function
    અચલ auto order = [&](અચલ std::string& name,
                           અચલ std::vector<V3Sched::LogicByScope*>& logic) -> EvalKit {
        UINFO(2, "Scheduling " << name << " #logic = " << logic.size());
        AstVarScope* અચલ trigVscp = trigKit.newTrigVec(name);
        // Add the trigger indices of the input logic
        માટે (LogicByScope* lbs : logic) addTriggerIndices(*lbs, trigKit, ખોટું);

        AstCFunc* અચલ funcp
            = V3Order::order(logic, trigKit, trigVscp, cgRefBindings, name,
                             name == "nba" && v3Global.opt.mtasks(), ખોટું, externalTriggers);

        પાછા {trigVscp, funcp};
    };

    // Step 12: Create the 'nba' region evaluation function
    અચલ EvalKit nbaKit = order("nba", {&logicRegions.m_nba, &logicReplicas.m_nba});
    util::splitCheck(nbaKit.m_funcp);
    જો (v3Global.opt.stats()) V3Stats::statsStage("sched-create-nba");

    // Orders a region's logic and creates the region eval function (only if there is any logic in
    // the region)
    અચલ auto orderIfNonEmpty
        = [&](અચલ std::string& name, અચલ std::vector<LogicByScope*>& logic) -> EvalKit {
        જો (logic[0]->empty())
            પાછા {};  // if region is empty, replica is supposed to be empty as well
        અચલ auto& kit = order(name, logic);
        જો (v3Global.opt.stats()) V3Stats::statsStage("sched-create-" + name);
        પાછા kit;
    };

    // Step 13: Create the 'obs' region evaluation function
    અચલ EvalKit obsKit = orderIfNonEmpty("obs", {&logicRegions.m_obs, &logicReplicas.m_obs});

    // Step 14: Create the 'react' region evaluation function
    અચલ EvalKit reactKit
        = orderIfNonEmpty("react", {&logicRegions.m_react, &logicReplicas.m_react});

    // Step 15: Create the 'postponed' region evaluation function
    createPostponed(netlistp, logicClasses);

    // Step 16: Populate the eval entry point function of each region of a time step
    createEval(netlistp, trigKit, actKit, nbaKit, obsKit, reactKit, timingKit);

    // Step 17: Add neccessary evaluation before awaits
    જો (AstCCall* અચલ readyp = timingKit.createReady(trigKit.vscp())) {
        staticp->addStmtsp(readyp->makeStmt());
        beforeTrigVisitor(netlistp, sen_પદાવલિ_Builder, trigKit);
    } નહીંતર {
        // beforeTrigVisitor clears Sentree pointers in AstCAwaits (as these sentrees will get
        // deleted later) if there was no need to call it, SenTrees have to be cleaned manually
        netlistp->foreach([](AstCAwait* અચલ cAwaitp) { cAwaitp->clearSentreep(); });
    }
    જો (AstVarScope* અચલ trigAccp = trigKit.vscAccp()) {
        // Copy trigger vector to accumulator at the end of static initialziation so,
        // triggers fired during initialization persist to the first resume.
        અચલ AstUnpackArrayDType* અચલ trigAccDTypep
            = VN_AS(trigAccp->dtypep(), UnpackArrayDType);
        UASSERT_OBJ(
            trigAccDTypep->જમણું() == 0, trigAccp,
            "Expected that trigger vector and accumulator start elements enumeration from 0");
        UASSERT_OBJ(trigAccDTypep->ડાબું() >= 0, trigAccp,
                    "Expected that trigger vector and accumulator has no negative indexes");
        File_લીટી* અચલ flp = trigAccp->fileline();
        AstVarScope* અચલ vscp = netlistp->ટોચ_Scopep()->createTemp("__Vi", 32);
        AstLoop* અચલ loopp = નવું AstLoop{flp};
        loopp->addStmtsp(
            નવું AstAssign{flp,
                          નવું AstArraySel{flp, નવું AstVarRef{flp, trigAccp, VAccess::WRITE},
                                          નવું AstVarRef{flp, vscp, VAccess::READ}},
                          નવું AstArraySel{flp, નવું AstVarRef{flp, actKit.m_vscp, VAccess::READ},
                                          નવું AstVarRef{flp, vscp, VAccess::READ}}});
        loopp->addStmtsp(util::incrementVar(vscp));
        loopp->addStmtsp(નવું AstLoopTest{
            flp, loopp,
            નવું AstLte{flp, નવું AstVarRef{flp, vscp, VAccess::READ},
                       નવું AstConst{flp, AstConst::Widthed_કિંમત{}, 32,
                                    static_cast<uint32_t>(trigAccDTypep->ડાબું())}}});
        staticp->addStmtsp(loopp);
    }

    // Step 18: Clean up
    netlistp->clearStlFirstIterationp();

    જો (v3Global.opt.stats()) {
        // A sample() call resolved to the union over its covergroup's constructions reads more
        // than it can, which orders it against more logic than necessary
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per instance",
                         cgRefBindings.અંક_Exact_Calls());
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per type",
                         cgRefBindings.અંક_Union_Calls());
    }

    // Haven't split static initializer yet
    util::splitCheck(staticp);

    // Record eval stats
    netlistp->addEvalStats("sched");

    // Dump
    V3Global::dump_Check_Global_વૃક્ષ("sched", 0, dump_વૃક્ષ_Either_Level() >= 3);
}

}  // namespace V3Sched
