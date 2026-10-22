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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Sched.h"

#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3EmitCBase.h"
#അന്തര്ഭാവ "V3EmitV.h"
#അന്തര്ഭാവ "V3Order.h"
#അന്തര്ഭാവ "V3SenExprBuilder.h"
#അന്തര്ഭാവ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

നാമാകാശ V3Sched {

നാമാകാശ {

//============================================================================
// Utility functions

std::vector<സ്ഥിരം Ast_Sen_വൃക്ഷം*> getSenTreesUsedBy(സ്ഥിരം std::vector<സ്ഥിരം LogicByScope*>& lbsps) {
    സ്ഥിരം VNUser1InUse user1InUse;
    std::vector<സ്ഥിരം Ast_Sen_വൃക്ഷം*> result;
    വേണ്ടി (സ്ഥിരം LogicByScope* സ്ഥിരം lbsp : lbsps) {
        വേണ്ടി (സ്ഥിരം auto& entry : *lbsp) {
            AstActive* സ്ഥിരം activep = entry.m_activep;
            Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep = activep->sentreep();
            എങ്കിൽ (senTreep->user1SetOnce()) തുടരുക;
            എങ്കിൽ (senTreep->hasClocked() || senTreep->hasHybrid()) result.തള്ളുക_back(senTreep);
        }
    }
    തിരികെ result;
}

ശൂന്യം addTriggerIndices(LogicByScope& lbs, സ്ഥിരം TriggerKit& trigKit, തർക്കം pre) {
    വേണ്ടി (LogicByScope::Entry& entry : lbs) {
        Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep = entry.m_activep->sentreep();
        എങ്കിൽ (senTreep->hasCombo()) തുടരുക;
        entry.m_triggers = trigKit.triggerIndices(senTreep, pre);
    }
}

// Add the VIF triggers that a given VarScope should be sensitive to.
// Case 2 (non-virtual interface read): sensitive to that specific VarScope's trigger only
// Case 3 (virtual interface read): sensitive to all member triggers of the same interface type
ശൂന്യം addTriggeredIface(സ്ഥിരം AstVarScope* vscp, സ്ഥിരം VirtIfaceTriggers::VscpTrigMap& vscpToTrig,
                       സ്ഥിരം VirtIfaceTriggers& virtIfaceTriggers, std::set<uint32_t>& triggers) {
    എങ്കിൽ (vscp->varp()->isVirtIface()) {
        // Virtual interface variable -- sensitive to all member triggers of this interface type
        സ്ഥിരം AstIface* സ്ഥിരം ifacep = VN_AS(vscp->varp()->dtypep(), IfaceRefDType)->ifacep();
        വേണ്ടി (സ്ഥിരം auto& entry : virtIfaceTriggers.m_triggers) {
            എങ്കിൽ (entry.m_ifacep == ifacep) {
                സ്ഥിരം auto it = vscpToTrig.find(entry.m_vscp);
                എങ്കിൽ (it != vscpToTrig.end()) triggers.തിരുകുക(it->second);
            }
        }
    } അല്ലെങ്കിൽ {
        // Non-virtual interface member -- sensitive to this VarScope's trigger only
        സ്ഥിരം auto it = vscpToTrig.find(vscp);
        എങ്കിൽ (it != vscpToTrig.end()) triggers.തിരുകുക(it->second);
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
ശൂന്യം createEvalRegion(
    AstNetlist* netlistp,  //
    VEval eval,  // The entry point of the current region
    // Index of the region's 'first iteration' extra trigger, if it has one, otherwise ignored
    uint32_t firstIterTrigger,
    സ്ഥിരം TriggerKit& trigKit,  // The trigger kit
    AstVarScope* trigp,  // The trigger vector - may be nullptr if no triggers or using 'condp'
    Ast_കണ്ണി_വ്യഞ്ജകം* condp,  // Explicit condition that must be true to run 'phaseWorkp'
    Ast_കണ്ണി_Stmt* phasePrepp,  // Prep statements run before checking triggers
    Ast_കണ്ണി_Stmt* phaseWorkp,  // The work to do if anything triggered
    // Extra statements to run after the work, even if no triggers fired. This function is
    // passed a variable, which must be set to true if we must continue and loop again,
    // and must be unmodified otherwise.
    std::function<Ast_കണ്ണി_Stmt*(AstVarScope*)> phaseExtra = [](AstVarScope*) { തിരികെ nullptr; }  //
) {
    UASSERT(!trigp || !condp, "Cannot use both 'trigp' and 'condp' in 'createEvalRegion'");
    UASSERT(!eval.firstIteration() || trigp,
            "Region without triggers cannot need a first iteration flag");

    // All work is under a trigger or condition, so with neither the region has
    // nothing to evaluate, and what we create below reduces to a no-op function.

    സ്ഥിരം std::string tag = eval.tag();
    Ast_മുകൾ_Scope* സ്ഥിരം മുകൾ_Scopep = netlistp->മുകൾ_Scopep();
    File_വരി* സ്ഥിരം flp = netlistp->fileline();

    // Populate the trigger dump entry point function
    എങ്കിൽ (trigp) {
        netlistp->dumpTriggersFuncp(eval)->addStmtsp(trigKit.newDumpCall(trigp, tag, അസത്യം));
    }

    AstCFunc* സ്ഥിരം funcp = netlistp->evalFuncp(eval);

    // A flag is passed from the run-time eval loop if this is the first iteration of the
    // current loop
    എങ്കിൽ (eval.firstIteration()) {
        AstVarScope* സ്ഥിരം firstIterArgp = util::newArgument(funcp, netlistp->findBitDType(),
                                                             "firstIteration", VDirection::നിവേശം);

        // Set the region's 'first iteration' trigger straight from the argument
        funcp->addStmtsp(trigKit.newExtraTriggerAssignment(firstIterArgp, firstIterTrigger));

        // Only 'stl' also needs a module level flag, for design logic that reads
        // it directly (see V3Timing). Those reads can be anywhere in the design,
        // hence module level. Always created, even if nothing reads it.
        // TODO: get rid of this special case
        എങ്കിൽ (eval == VEval::STL) {
            AstVarScope* സ്ഥിരം firstIterp = netlistp->stlFirstIterationp();
            firstIterp->varp()->noReset(സത്യം);
            firstIterp->varp()->isInternal(സത്യം);
            funcp->addStmtsp(പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, firstIterp, VAccess::WRITE},
                                           പുതിയ AstVarRef{flp, firstIterArgp, VAccess::READ}});
        }
    }

    {
        // Add the preparatory statements
        funcp->addStmtsp(phasePrepp);

        // The execute flag
        AstVarScope* സ്ഥിരം executeFlagp = മുകൾ_Scopep->createTemp("__V" + tag + "Execute", 1);
        executeFlagp->varp()->noReset(സത്യം);

        // If there is work in this region, execute it if any triggers fired
        എങ്കിൽ (phaseWorkp) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = പുതിയ AstVarRef{flp, executeFlagp, VAccess::WRITE};
            // If using explicit condition, that directly determines whether to execute,
            // otherwise check if any triggers are fired
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = condp ? condp : trigKit.newAnySetCall(trigp);
            funcp->addStmtsp(പുതിയ AstAssign{flp, lhsp, rhsp});

            // Add the work
            AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, പുതിയ AstVarRef{flp, executeFlagp, VAccess::READ}};
            ifp->addThensp(phaseWorkp);
            funcp->addStmtsp(ifp);
        }

        // Construct the extra statements
        Ast_കണ്ണി_Stmt* സ്ഥിരം extraWorkp = phaseExtra(executeFlagp);
        എങ്കിൽ (extraWorkp) funcp->addStmtsp(extraWorkp);

        // The function returns true iff it did run work
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം retp
            = phaseWorkp || extraWorkp
                  ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstVarRef{flp, executeFlagp, VAccess::READ})
                  : static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstConst{flp, AstConst::BitFalse{}});
        funcp->addStmtsp(പുതിയ AstCReturn{flp, retp});
    }
}

//============================================================================
// Collect and classify all logic in the design

LogicClasses gatherLogicClasses(AstNetlist* netlistp) {
    LogicClasses result;

    netlistp->foreach([&](AstScope* scopep) {
        scopep->foreach([&](AstActive* activep) {
            Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep = activep->sentreep();
            എങ്കിൽ (senTreep->hasStatic()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "static initializer with additional sensitivities");
                result.m_static.add(scopep, activep);
            } അല്ലെങ്കിൽ എങ്കിൽ (senTreep->hasInitial()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'initial' logic with additional sensitivities");
                result.m_initial.add(scopep, activep);
            } അല്ലെങ്കിൽ എങ്കിൽ (senTreep->hasFinal()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'final' logic with additional sensitivities");
                result.m_final.add(scopep, activep);
            } അല്ലെങ്കിൽ എങ്കിൽ (senTreep->hasCombo()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "combinational logic with additional sensitivities");
                എങ്കിൽ (VN_IS(activep->stmtsp(), AlwaysPostponed)) {
                    result.m_postponed.add(scopep, activep);
                } അല്ലെങ്കിൽ {
                    result.m_comb.add(scopep, activep);
                }
            } അല്ലെങ്കിൽ {
                UASSERT_OBJ(senTreep->hasClocked(), activep, "What else could it be?");
                എങ്കിൽ (VN_IS(activep->stmtsp(), AlwaysObserved)) {
                    result.m_observed.add(scopep, activep);
                } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(activep->stmtsp(), AlwaysReactive)) {
                    result.m_reactive.add(scopep, activep);
                } അല്ലെങ്കിൽ {
                    result.m_clocked.add(scopep, activep);
                }
            }
        });
    });

    തിരികെ result;
}

//============================================================================
// Simple ordering in source order

ശൂന്യം orderSequentially(AstCFunc* funcp, സ്ഥിരം LogicByScope& lbs) {
    // Create new subfunc for scope
    സ്ഥിരം auto createNewSubFuncp = [&](AstScope* സ്ഥിരം scopep) {
        സ്ഥിരം string subName{funcp->name() + "__" + scopep->nameDotless()};
        AstCFunc* സ്ഥിരം subFuncp = പുതിയ AstCFunc{scopep->fileline(), subName, scopep};
        subFuncp->isLoose(സത്യം);
        subFuncp->isConst(അസത്യം);
        subFuncp->declPrivate(സത്യം);
        subFuncp->slow(funcp->slow());
        scopep->addBlocksp(subFuncp);
        // Call it from the top function
        funcp->addStmtsp(util::callVoidFunc(subFuncp));
        തിരികെ subFuncp;
    };
    സ്ഥിരം VNUser1InUse user1InUse;  // AstScope -> AstCFunc: the sub-function for the scope
    സ്ഥിരം VNUser2InUse user2InUse;  // AstScope -> int: sub-function counter used for names
    വേണ്ടി (സ്ഥിരം auto& entry : lbs) {
        AstScope* സ്ഥിരം scopep = entry.m_scopep;
        AstActive* സ്ഥിരം activep = entry.m_activep;
        // Create a sub-function per scope so we can V3Combine them later
        എങ്കിൽ (!scopep->user1p()) scopep->user1p(createNewSubFuncp(scopep));
        // Add statements to sub-function
        വേണ്ടി (Ast_കണ്ണി *logicp = activep->stmtsp(), *nextp; logicp; logicp = nextp) {
            auto* subFuncp = VN_AS(scopep->user1p(), CFunc);
            nextp = logicp->nextp();
            എങ്കിൽ (Ast_കണ്ണി_Procedure* സ്ഥിരം procp = VN_CAST(logicp, കണ്ണി_Procedure)) {
                എങ്കിൽ (Ast_കണ്ണി* bodyp = procp->stmtsp()) {
                    bodyp->unlinkFrBackWithNext();
                    // If the process is suspendable, we need a separate function (a coroutine)
                    എങ്കിൽ (procp->isSuspendable()) {
                        funcp->slow(അസത്യം);
                        subFuncp = createNewSubFuncp(scopep);
                        subFuncp->name(subFuncp->name() + "__Vtiming__"
                                       + cvtToStr(scopep->user2Inc()));
                        subFuncp->rtnType("VlCoroutine");
                        എങ്കിൽ (VN_IS(procp, Always)) {
                            subFuncp->slow(അസത്യം);
                            File_വരി* സ്ഥിരം flp = procp->fileline();
                            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = പുതിയ AstCExpr{
                                flp, "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish())", 1};
                            AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{flp};
                            loopp->addStmtsp(പുതിയ AstLoopTest{flp, loopp, condp});
                            loopp->addStmtsp(bodyp);
                            bodyp = loopp;
                        }
                    }
                    subFuncp->addStmtsp(bodyp);
                    എങ്കിൽ (procp->needProcess()) subFuncp->setNeedProcess();
                    // A coroutine holds a single process, so it is complete here
                    എങ്കിൽ (procp->isSuspendable()) util::splitCheck(subFuncp);
                }
            } അല്ലെങ്കിൽ {
                logicp->unlinkFrBack();
                subFuncp->addStmtsp(logicp);
            }
        }
        എങ്കിൽ (activep->backp()) activep->unlinkFrBack();
        VL_DO_DANGLING(activep->delete_വൃക്ഷം(), activep);
    }
    // Split sub-functions once complete
    വേണ്ടി (സ്ഥിരം auto& entry : lbs) {
        AstScope* സ്ഥിരം scopep = entry.m_scopep;
        എങ്കിൽ (AstCFunc* സ്ഥിരം subFuncp = VN_AS(scopep->user1p(), CFunc)) {
            util::splitCheck(subFuncp);
            scopep->user1p(nullptr);
        }
    }
}

//============================================================================
// Create simply ordered functions

AstCFunc* createStatic(AstNetlist* netlistp, സ്ഥിരം LogicClasses& logicClasses) {
    AstCFunc* സ്ഥിരം funcp = netlistp->evalFuncp(VEval::STATIC);

    സ്ഥിരം LogicByScope& orig = logicClasses.m_static;
    എങ്കിൽ (orig.size() <= 1) {
        orderSequentially(funcp, orig);
        തിരികെ funcp;
    }

    // Level-based module sorting can reorder packages so that an importing
    // package runs before the imported one.  Re-sort package entries by source
    // file position to restore compilation order (IEEE 1800-2023 26.3).
    std::vector<size_t> indices(orig.size());
    വേണ്ടി (size_t i = 0; i < orig.size(); ++i) indices[i] = i;
    std::stable_sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം modA = orig[a].m_scopep->modp();
        സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം modB = orig[b].m_scopep->modp();
        സ്ഥിരം തർക്കം isPkgA = VN_IS(modA, Package);
        സ്ഥിരം തർക്കം isPkgB = VN_IS(modB, Package);
        എങ്കിൽ (isPkgA != isPkgB) തിരികെ isPkgA;  // Packages before non-packages
        എങ്കിൽ (isPkgA && isPkgB) {
            // Sort packages by source file position (compilation order)
            തിരികെ modA->fileline()->operatorCompare(*modB->fileline()) < 0;
        }
        തിരികെ അസത്യം;  // Both non-package: preserve original order
    });
    LogicByScope sorted;
    വേണ്ടി (സ്ഥിരം size_t i : indices) sorted.add(orig[i].m_scopep, orig[i].m_activep);

    orderSequentially(funcp, sorted);
    തിരികെ funcp;  // Not splitting yet as it is not final
}

ശൂന്യം createInitial(AstNetlist* netlistp, സ്ഥിരം LogicClasses& logicClasses) {
    AstCFunc* സ്ഥിരം funcp = netlistp->evalFuncp(VEval::INITIAL);
    orderSequentially(funcp, logicClasses.m_initial);
    util::splitCheck(funcp);
}

ശൂന്യം createPostponed(AstNetlist* netlistp, സ്ഥിരം LogicClasses& logicClasses) {
    AstCFunc* സ്ഥിരം funcp = netlistp->evalFuncp(VEval::POSTPONED);
    orderSequentially(funcp, logicClasses.m_postponed);
    util::splitCheck(funcp);
}

ശൂന്യം createFinal(AstNetlist* netlistp, സ്ഥിരം LogicClasses& logicClasses) {
    AstCFunc* സ്ഥിരം funcp = netlistp->evalFuncp(VEval::FINAL);
    orderSequentially(funcp, logicClasses.m_final);
    util::splitCheck(funcp);
}

//============================================================================
// Helper that creates virtual interface value-change triggers

ശൂന്യം addVirtIfaceTriggerAssignments(AstNetlist* netlistp, AstCFunc* initFuncp,
                                    സ്ഥിരം VirtIfaceTriggers& virtIfaceTriggers,
                                    uint32_t firstIndex, സ്ഥിരം TriggerKit& trigKit) {
    uint32_t index = firstIndex;
    വേണ്ടി (സ്ഥിരം auto& entry : virtIfaceTriggers.m_triggers) {
        trigKit.add_മൂല്യം_Change_Trigger_Assignment(netlistp, initFuncp, entry.m_vscp, index);
        ++index;
    }
}

// Order the combinational logic to create the 'stl' region
ശൂന്യം createSettle(AstNetlist* netlistp, AstCFunc* സ്ഥിരം initFuncp, Sen_വ്യഞ്ജകം_Builder& sen_വ്യഞ്ജകം_Bulider,
                  LogicClasses& logicClasses, സ്ഥിരം CovergroupRefBindings& cgRefBindings) {
    // Clone, because ordering is destructive, but we still need them for the other regions
    LogicByScope comb = logicClasses.m_comb.clone();
    LogicByScope hybrid = logicClasses.m_hybrid.clone();

    // We have an extra trigger denoting this is the first iteration of the settle loop
    TriggerKit::ExtraTriggers extraTriggers;
    സ്ഥിരം uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");

    // Gather the relevant sensitivity expressions and create the trigger kit
    സ്ഥിരം auto& senTreeps = getSenTreesUsedBy({&comb, &hybrid});
    സ്ഥിരം TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_വ്യഞ്ജകം_Bulider, {},
                                                  senTreeps, "stl", extraTriggers, സത്യം, അസത്യം);

    // Add trigger indices (comb has none, so only do the hybrid)
    addTriggerIndices(hybrid, trigKit, അസത്യം);

    // First trigger is for pure combinational triggers (first iteration)
    സ്ഥിരം uint32_t നിവേശം_Changed = trigKit.extraTriggerIndex(firstIterationTrigger);

    // Create and the body function
    AstCFunc* സ്ഥിരം stlFuncp = V3Order::order(
        {&comb, &hybrid}, trigKit, trigKit.vscp(), cgRefBindings, "stl", അസത്യം, സത്യം,
        [=](സ്ഥിരം AstVarScope*, std::set<uint32_t>& triggers) { triggers.തിരുകുക(നിവേശം_Changed); });
    util::splitCheck(stlFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::STL, firstIterationTrigger, trigKit,
        // Use trigger
        trigKit.vscp(), nullptr,
        // Explicit condition
        // Prep statements: Compute the current 'stl' triggers
        [&trigKit] {
            Ast_കണ്ണി_Stmt* സ്ഥിരം stmtp = trigKit.newCompBaseCall();
            എങ്കിൽ (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), സത്യം));
            തിരികെ stmtp;
        }(),
        // Work statements: Invoke the 'stl' function
        util::callVoidFunc(stlFuncp));
}

//============================================================================
// Order the replicated combinational logic to create the 'ico' region

ശൂന്യം createIcoRegion(AstNetlist* netlistp, AstCFunc* സ്ഥിരം initFuncp,
                     Sen_വ്യഞ്ജകം_Builder& sen_വ്യഞ്ജകം_Builder, LogicByScope& logic,
                     സ്ഥിരം VirtIfaceTriggers& virtIfaceTriggers,
                     സ്ഥിരം CovergroupRefBindings& cgRefBindings) {
    // SystemC only: Any top level inputs feeding a combinational logic must be marked,
    // so we can make them sc_sensitive
    എങ്കിൽ (v3Global.opt.systemC()) {
        logic.foreachLogic([](Ast_കണ്ണി* logicp) {
            logicp->foreach([](AstVarRef* refp) {
                എങ്കിൽ (refp->access().isWriteOnly()) തിരികെ;
                AstVarScope* സ്ഥിരം vscp = refp->varScopep();
                എങ്കിൽ (vscp->scopep()->is_മുകൾ() && vscp->varp()->isNonOutput()) {
                    vscp->varp()->scSensitive(സത്യം);
                }
            });
        });
    }

    // We have some extra trigger denoting external conditions
    AstVarScope* സ്ഥിരം dpiExportTriggerVscp = netlistp->dpiExportTriggerp();

    TriggerKit::ExtraTriggers extraTriggers;
    സ്ഥിരം uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");
    സ്ഥിരം uint32_t dpiExportTriggerIndex = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    സ്ഥിരം uint32_t firstVifTriggerIndex = extraTriggers.size();
    വേണ്ടി (സ്ഥിരം auto& entry : virtIfaceTriggers.m_triggers) {
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
    std::unordered_map<സ്ഥിരം AstVarScope*, Ast_Sen_വൃക്ഷം*> inp2changedp;
    std::vector<Ast_Sen_വൃക്ഷം*> icoChangeSenTreeps;
    എങ്കിൽ (v3Global.opt.fIcoChangeDetect().isTrue()) {
        File_വരി* സ്ഥിരം flp = netlistp->fileline();
        AstScope* സ്ഥിരം scopep = netlistp->മുകൾ_Scopep()->scopep();
        വേണ്ടി (AstVarScope* vscp = scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            // Only for top level ports, assume outputs don't change externally
            എങ്കിൽ (!vscp->varp()->isPrimaryInish()) തുടരുക;
            // Don't do if written by the design - wouldn't update the change detect 'prev' value
            എങ്കിൽ (vscp->varp()->icoMaybeWritten()) തുടരുക;
            // Don't do if forceable, as we can't see the actual value - this is belt and braces
            എങ്കിൽ (vscp->varp()->isForced()) തുടരുക;
            // Can't handle unpacked arrays (they have special types when primary input)
            എങ്കിൽ (VN_IS(vscp->dtypep()->skipRefp(), UnpackArrayDType)) തുടരുക;
            // Similarly to arrays, can't handle SystemC types
            എങ്കിൽ (vscp->varp()->isSc()) തുടരുക;
            // Create a sen tree triggered when this input changes
            Ast_Sen_വൃക്ഷം*& senTreepr = inp2changedp[vscp];
            UASSERT_OBJ(!senTreepr, vscp, "Duplicate input change detect trigger");
            AstVarRef* സ്ഥിരം refp = പുതിയ AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* സ്ഥിരം senItemp = പുതിയ AstSenItem{flp, VEdgeType::ET_CHANGED, refp};
            senTreepr = പുതിയ Ast_Sen_വൃക്ഷം{flp, senItemp};
            icoChangeSenTreeps.തള്ളുക_back(senTreepr);
        }
    }
    V3Stats::addStat("Scheduling, 'ico' change detect triggers", icoChangeSenTreeps.size());

    // Gather the relevant sensitivity expressions and create the trigger kit
    std::vector<സ്ഥിരം Ast_Sen_വൃക്ഷം*> senTreeps = getSenTreesUsedBy({&logic});
    senTreeps.തിരുകുക(senTreeps.end(), icoChangeSenTreeps.begin(), icoChangeSenTreeps.end());
    സ്ഥിരം TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_വ്യഞ്ജകം_Builder, {},
                                                  senTreeps, "ico", extraTriggers, അസത്യം, അസത്യം);
    std::ignore = sen_വ്യഞ്ജകം_Builder.getAndClearResults();

    എങ്കിൽ (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, initFuncp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);

    // Add trigger indices
    addTriggerIndices(logic, trigKit, അസത്യം);

    // The 'first iteration' trigger for top level inputs
    സ്ഥിരം uint32_t firstIterTrigger = trigKit.extraTriggerIndex(firstIterationTrigger);

    സ്ഥിരം auto& vifVscpToTrigIco
        = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);

    // Create and Order the body function
    AstCFunc* സ്ഥിരം icoFuncp = V3Order::order(
        {&logic}, trigKit, trigKit.vscp(), cgRefBindings, "ico", അസത്യം, അസത്യം,
        [&](സ്ഥിരം AstVarScope* vscp, std::set<uint32_t>& triggers) {
            AstVar* സ്ഥിരം varp = vscp->varp();
            // If it has an explicit change detect trigger, use that,
            // otherwise fall back to using the 'first iteration' trigger
            auto it = inp2changedp.find(vscp);
            എങ്കിൽ (it != inp2changedp.end()) {
                triggers.തിരുകുക(trigKit.senItem2TrigIdx(it->second->sensesp()));
            } അല്ലെങ്കിൽ എങ്കിൽ (varp->isPrimaryInish() || varp->isSigUserRWPublic() || varp->sampled()) {
                triggers.തിരുകുക(firstIterTrigger);
            }
            // Add other triggers
            എങ്കിൽ (varp->isWrittenByDpi())
                triggers.തിരുകുക(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
            എങ്കിൽ (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
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
            Ast_കണ്ണി_Stmt* സ്ഥിരം stmtp = trigKit.newCompBaseCall();
            എങ്കിൽ (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), സത്യം));
            തിരികെ stmtp;
        }(),
        // Work statements: Invoke the 'ico' function
        util::callVoidFunc(icoFuncp));

    // Release temporary input change detect SenTrees
    വേണ്ടി (Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep : icoChangeSenTreeps)
        VL_DO_DANGLING(senTreep->delete_വൃക്ഷം(), senTreep);
    icoChangeSenTreeps.clear();
}

//============================================================================
// EvalKit groups items that have to be passed to createEval() for a given eval region

ഘടന EvalKit final {
    // The AstVarScope representing the region's trigger vector
    AstVarScope* സ്ഥിരം m_vscp = nullptr;
    // The AstCFunc that evaluates the region's logic
    AstCFunc* സ്ഥിരം m_funcp = nullptr;
    // Is this kit used/required?
    തർക്കം empty() സ്ഥിരം { തിരികെ !m_funcp; }
};

//============================================================================
// Create the evaluation function of each region of a time step

ശൂന്യം createEval(AstNetlist* netlistp,  //
                സ്ഥിരം TriggerKit& trigKit,  //
                സ്ഥിരം EvalKit& actKit,  //
                സ്ഥിരം EvalKit& nbaKit,  //
                സ്ഥിരം EvalKit& obsKit,  //
                സ്ഥിരം EvalKit& reactKit,  //
                TimingKit& timingKit  //
) {
    File_വരി* സ്ഥിരം flp = netlistp->fileline();

    // Grab the delay scheduler variable, if any
    AstVarScope* സ്ഥിരം delaySchedVscp = timingKit.getDelayScheduler(netlistp);

    // 'createResume' consumes the contents that 'createReady' needs, so do the right order
    AstCCall* സ്ഥിരം timingReadyp = timingKit.createReady(trigKit.vscp());
    AstCCall* സ്ഥിരം timingResumep = timingKit.createResume(trigKit.vscp());

    // Create the 'act' region
    createEvalRegion(  //
        netlistp, VEval::ACT, 0, trigKit,
        // Use trigger
        actKit.m_vscp, nullptr,
        // Prep statements
        [&]() {
            // Compute the current 'act' triggers - the NBA triggers are the latched value
            Ast_കണ്ണി_Stmt* stmtsp = trigKit.newCompBaseCall();
            Ast_കണ്ണി_Stmt* സ്ഥിരം dumpp
                = stmtsp ? trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), സത്യം) : nullptr;
            // Mark as ready for triggered awaits
            എങ്കിൽ (timingReadyp) stmtsp = Ast_കണ്ണി::addNext(stmtsp, timingReadyp->makeStmt());
            എങ്കിൽ (AstVarScope* സ്ഥിരം vscAccp = trigKit.vscAccp()) {
                stmtsp = Ast_കണ്ണി::addNext(stmtsp, trigKit.newOrIntoCall(actKit.m_vscp, vscAccp));
            }
            stmtsp = Ast_കണ്ണി::addNext(stmtsp, trigKit.newCompExtCall(nbaKit.m_vscp));
            stmtsp = Ast_കണ്ണി::addNext(stmtsp, dumpp);
            // Latch the 'act' triggers under the 'nba' triggers
            stmtsp = Ast_കണ്ണി::addNext(stmtsp, trigKit.newOrIntoCall(nbaKit.m_vscp, actKit.m_vscp));
            //
            തിരികെ stmtsp;
        }(),
        // Work statements
        [&]() {
            Ast_കണ്ണി_Stmt* workp = nullptr;
            എങ്കിൽ (AstVarScope* സ്ഥിരം actAccp = trigKit.vscAccp()) {
                AstCMethodHard* സ്ഥിരം cCallp = പുതിയ AstCMethodHard{
                    flp, പുതിയ AstVarRef{flp, actAccp, VAccess::WRITE}, VCMethod::UNPACKED_FILL,
                    പുതിയ AstConst{flp, AstConst::Unsized64{}, 0}};
                cCallp->dtypeSetVoid();
                workp = Ast_കണ്ണി::addNext(workp, cCallp->makeStmt());
            }
            // Resume triggered timing schedulers
            എങ്കിൽ (timingResumep) workp = Ast_കണ്ണി::addNext(workp, timingResumep->makeStmt());
            // Invoke the 'act' function
            workp = Ast_കണ്ണി::addNext(workp, util::callVoidFunc(actKit.m_funcp));
            //
            തിരികെ workp;
        }());

    // Create the 'inact' region
    createEvalRegion(  //
        netlistp, VEval::INACT, 0, trigKit,
        // Use explicit condition
        nullptr,
        [&]() -> Ast_കണ്ണി_വ്യഞ്ജകം* {
            എങ്കിൽ (!delaySchedVscp) തിരികെ nullptr;
            // Run if any zero delays are pending
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം callp
                = പുതിയ AstCMethodHard{flp, പുതിയ AstVarRef{flp, delaySchedVscp, VAccess::READ},
                                     VCMethod::SCHED_AWAITING_ZERO_DELAY};
            callp->dtypeSetBit();
            തിരികെ callp;
        }(),
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_കണ്ണി_Stmt* {
            എങ്കിൽ (!delaySchedVscp) {
                // Nothing to do if there are no delays at all in the design
                തിരികെ nullptr;
            } അല്ലെങ്കിൽ എങ്കിൽ (v3Global.usesZeroDelay()) {
                // Resume processes watiting for #0 delay
                AstCMethodHard* സ്ഥിരം callp = പുതിയ AstCMethodHard{
                    flp, പുതിയ AstVarRef{flp, delaySchedVscp, VAccess::READWRITE},
                    VCMethod::SCHED_RESUME_ZERO_DELAY};
                callp->dtypeSetVoid();
                തിരികെ callp->makeStmt();
            } അല്ലെങ്കിൽ {
                // Assumption was that the design doesn't use #0 delays.
                // Die at run-time if it does.
                AstCStmt* സ്ഥിരം stmtp = പുതിയ AstCStmt{flp};
                സ്ഥിരം File_വരി* സ്ഥിരം locp = netlistp->മുകൾ_Modulep()->fileline();
                സ്ഥിരം std::string& file = VIdProtect::protect(locp->filename());
                സ്ഥിരം std::string& വരി = std::to_string(locp->lineno());
                stmtp->add("VL_FATAL_MT(\"" + V3OutFormatter::quoteNameControls(file) + "\", "
                           + വരി
                           + ", \"\", \"ZERODLY: Design Verilated with '--no-sched-zero-delay', "
                           + "but #0 delay executed at runtime\");");
                തിരികെ stmtp;
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
            Ast_കണ്ണി_Stmt* workp = nullptr;
            // Latch the 'nba' trigger flags under the following region's trigger flags
            എങ്കിൽ (!obsKit.empty()) {
                workp = trigKit.newOrIntoCall(obsKit.m_vscp, nbaKit.m_vscp);
            } അല്ലെങ്കിൽ എങ്കിൽ (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, nbaKit.m_vscp);
            }
            // Invoke the 'nba' function
            workp = Ast_കണ്ണി::addNext(workp, util::callVoidFunc(nbaKit.m_funcp));
            // Clear the 'nba' triggers
            workp = Ast_കണ്ണി::addNext(workp, trigKit.newClearCall(nbaKit.m_vscp));
            //
            തിരികെ workp;
        }(),
        // Extra work (not conditional on having had a fired trigger)
        [&](AstVarScope* continuep) -> Ast_കണ്ണി_Stmt* {
            // Check if any dynamic NBAs are pending, if there are any in the design
            എങ്കിൽ (!netlistp->nbaEventp()) തിരികെ nullptr;
            AstVarScope* സ്ഥിരം nbaEventp = netlistp->nbaEventp();
            AstVarScope* സ്ഥിരം nbaEventTriggerp = netlistp->nbaEventTriggerp();
            UASSERT(nbaEventTriggerp, "NBA event trigger var should exist");
            netlistp->nbaEventp(nullptr);
            netlistp->nbaEventTriggerp(nullptr);

            // If a dynamic NBA is pending, clear the pending flag and fire the ready event
            AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, പുതിയ AstVarRef{flp, nbaEventTriggerp, VAccess::READ}};
            ifp->addThensp(util::setVar(continuep, 1));
            ifp->addThensp(util::setVar(nbaEventTriggerp, 0));
            AstCMethodHard* സ്ഥിരം firep = പുതിയ AstCMethodHard{
                flp, പുതിയ AstVarRef{flp, nbaEventp, VAccess::WRITE}, VCMethod::EVENT_FIRE};
            firep->dtypeSetVoid();
            ifp->addThensp(firep->makeStmt());
            തിരികെ ifp;
        });

    // Create the 'obs' region
    createEvalRegion(  //
        netlistp, VEval::OBS, 0, trigKit,
        // Use trigger
        obsKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_കണ്ണി_Stmt* {
            എങ്കിൽ (obsKit.empty()) തിരികെ nullptr;
            Ast_കണ്ണി_Stmt* workp = nullptr;
            // Latch the Observed trigger flags under the Reactive trigger flags
            എങ്കിൽ (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, obsKit.m_vscp);
            }
            // Invoke the 'obs' function
            workp = Ast_കണ്ണി::addNext(workp, util::callVoidFunc(obsKit.m_funcp));
            // Clear the 'obs' triggers
            workp = Ast_കണ്ണി::addNext(workp, trigKit.newClearCall(obsKit.m_vscp));
            //
            തിരികെ workp;
        }());

    // Create the 'react' region
    createEvalRegion(  //
        netlistp, VEval::REACT, 0, trigKit,
        // Use trigger
        reactKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_കണ്ണി_Stmt* {
            എങ്കിൽ (reactKit.empty()) തിരികെ nullptr;
            // Invoke the 'react' function
            Ast_കണ്ണി_Stmt* workp = util::callVoidFunc(reactKit.m_funcp);
            // Clear the 'react' triggers
            workp = Ast_കണ്ണി::addNext(workp, trigKit.newClearCall(reactKit.m_vscp));
            തിരികെ workp;
        }());
}

}  // namespace

//============================================================================
// Helper that builds virtual interface trigger sentrees

VirtIfaceTriggers::VscpTrigMap VirtIfaceTriggers::makeVscpToTrigMap(സ്ഥിരം TriggerKit& trigKit,
                                                                    uint32_t firstIndex) സ്ഥിരം {
    VscpTrigMap map;
    uint32_t index = firstIndex;
    വേണ്ടി (സ്ഥിരം auto& entry : m_triggers) {
        map.emplace(entry.m_vscp, trigKit.extraTriggerIndex(index));
        ++index;
    }
    തിരികെ map;
}

//============================================================================
// Top level entry-point to scheduling

ശൂന്യം schedule(AstNetlist* netlistp) {
    സ്ഥിരം auto addSizeStat = [](സ്ഥിരം string& name, സ്ഥിരം LogicByScope& lbs) {
        uint64_t size = 0;
        lbs.foreachLogic([&](Ast_കണ്ണി* nodep) { size += nodep->കണ്ണി_എണ്ണം(); });
        V3Stats::addStat("Scheduling, " + name, size);
    };

    // Step 2: Prepare external domains for timing and virtual interfaces
    // Create extra triggers for virtual interfaces
    സ്ഥിരം auto& virtIfaceTriggers = makeVirtIfaceTriggers(netlistp);
    // Resolve what covergroup reference formal arguments are bound to, which is visible here
    // but not from V3Order, where the reads through them must be modeled
    സ്ഥിരം CovergroupRefBindings& cgRefBindings = makeCovergroupRefBindings(netlistp);
    // Prepare timing-related logic and external domains
    TimingKit timingKit = prepareTiming(netlistp);

    // Step 3: Gather and classify all logic in the design
    LogicClasses logicClasses = gatherLogicClasses(netlistp);

    എങ്കിൽ (v3Global.opt.stats()) {
        V3Stats::statsStage("sched-gather");
        addSizeStat("size of class: static", logicClasses.m_static);
        addSizeStat("size of class: initial", logicClasses.m_initial);
        addSizeStat("size of class: final", logicClasses.m_final);
    }

    // Step 4: Schedule static, initial and final logic classes in source order
    AstCFunc* സ്ഥിരം staticp = createStatic(netlistp, logicClasses);
    എങ്കിൽ (v3Global.opt.stats()) V3Stats::statsStage("sched-static");

    createInitial(netlistp, logicClasses);
    എങ്കിൽ (v3Global.opt.stats()) V3Stats::statsStage("sched-initial");

    createFinal(netlistp, logicClasses);
    എങ്കിൽ (v3Global.opt.stats()) V3Stats::statsStage("sched-final");

    // Step 5: Break combinational cycles by introducing hybrid logic
    // Note: breakCycles also removes corresponding logic from logicClasses.m_comb;
    logicClasses.m_hybrid = breakCycles(netlistp, logicClasses.m_comb);
    എങ്കിൽ (v3Global.opt.stats()) {
        addSizeStat("size of class: clocked", logicClasses.m_clocked);
        addSizeStat("size of class: combinational", logicClasses.m_comb);
        addSizeStat("size of class: hybrid", logicClasses.m_hybrid);
        V3Stats::statsStage("sched-break-cycles");
    }

    // We pass around a single SenExprBuilder instance, as we only need one set of 'prev' variables
    // for edge/change detection in sensitivity expressions, which this keeps track of.
    Ast_മുകൾ_Scope* സ്ഥിരം മുകൾ_Scopep = netlistp->മുകൾ_Scopep();
    AstScope* സ്ഥിരം scopeTopp = മുകൾ_Scopep->scopep();
    Sen_വ്യഞ്ജകം_Builder sen_വ്യഞ്ജകം_Builder{scopeTopp};

    // Step 6: Create 'settle' region that restores the combinational invariant
    createSettle(netlistp, staticp, sen_വ്യഞ്ജകം_Builder, logicClasses, cgRefBindings);
    എങ്കിൽ (v3Global.opt.stats()) V3Stats::statsStage("sched-settle");

    // Step 7: Partition the clocked and combinational (including hybrid) logic into pre/act/nba.
    // All clocks (signals referenced in an AstSenTree) generated via a blocking assignment
    // (including combinationally generated signals) are computed within the act region.
    LogicRegions logicRegions
        = partition(logicClasses.m_clocked, logicClasses.m_comb, logicClasses.m_hybrid);
    logicRegions.m_obs = logicClasses.m_observed;
    logicRegions.m_react = logicClasses.m_reactive;
    എങ്കിൽ (v3Global.opt.stats()) {
        addSizeStat("size of region: Active Pre", logicRegions.m_pre);
        addSizeStat("size of region: Active", logicRegions.m_act);
        addSizeStat("size of region: NBA", logicRegions.m_nba);
        addSizeStat("size of region: Observed", logicRegions.m_obs);
        addSizeStat("size of region: Reactive", logicRegions.m_react);
        V3Stats::statsStage("sched-partition");
    }

    // Step 8: Replicate combinational logic
    LogicReplicas logicReplicas = replicateLogic(logicRegions);
    എങ്കിൽ (v3Global.opt.stats()) {
        addSizeStat("size of replicated logic: Input", logicReplicas.m_ico);
        addSizeStat("size of replicated logic: Active", logicReplicas.m_act);
        addSizeStat("size of replicated logic: NBA", logicReplicas.m_nba);
        addSizeStat("size of replicated logic: Observed", logicReplicas.m_obs);
        addSizeStat("size of replicated logic: Reactive", logicReplicas.m_react);
        V3Stats::statsStage("sched-replicate");
    }

    // Step 9: Create the input combinational logic
    createIcoRegion(netlistp, staticp, sen_വ്യഞ്ജകം_Builder, logicReplicas.m_ico, virtIfaceTriggers,
                    cgRefBindings);
    എങ്കിൽ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-ico");

    // Step 10: Create the triggers
    AstVarScope* സ്ഥിരം dpiExportTriggerVscp = netlistp->dpiExportTriggerp();
    netlistp->dpiExportTriggerp(nullptr);  // Finished with this here

    // We may have an extra trigger for variable updated in DPI exports
    TriggerKit::ExtraTriggers extraTriggers;
    സ്ഥിരം uint32_t dpiExportTriggerIndex = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    സ്ഥിരം uint32_t firstVifTriggerIndex = extraTriggers.size();
    വേണ്ടി (സ്ഥിരം auto& entry : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + entry.m_ifacep->name() + "."
                               + entry.m_memberp->name());
    }

    സ്ഥിരം auto& preTreeps = getSenTreesUsedBy({&logicRegions.m_pre});
    സ്ഥിരം auto& senTreeps = getSenTreesUsedBy({&logicRegions.m_act,  //
                                               &logicRegions.m_nba,  //
                                               &logicRegions.m_obs,  //
                                               &logicRegions.m_react,  //
                                               &timingKit.m_lbs});
    സ്ഥിരം TriggerKit trigKit
        = TriggerKit::create(netlistp, staticp, sen_വ്യഞ്ജകം_Builder, preTreeps, senTreeps, "act",
                             extraTriggers, അസത്യം, v3Global.usesTiming());

    // Add post updates from the timing kit
    എങ്കിൽ (timingKit.m_postUpdates) trigKit.compBasep()->addStmtsp(timingKit.m_postUpdates);

    എങ്കിൽ (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, staticp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);
    എങ്കിൽ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-triggers");

    // Note: Experiments so far show that running the Act (or Ico) regions on
    // multiple threads is always a net loss, so only use multi-threading for
    // NBA for now. This can be revised if evidence is available that it would
    // be beneficial

    // Step 11: Create the 'act' region evaluation function

    // Add the trigger indices of the input logic
    addTriggerIndices(logicRegions.m_pre, trigKit, സത്യം);
    addTriggerIndices(logicRegions.m_act, trigKit, അസത്യം);
    addTriggerIndices(logicReplicas.m_act, trigKit, അസത്യം);
    addTriggerIndices(timingKit.m_lbs, trigKit, അസത്യം);

    // External triggers of variables. Trigger indices are the same in all regions, so share.
    സ്ഥിരം std::map<സ്ഥിരം AstVarScope*, std::vector<uint32_t>> timingTriggers
        = timingKit.externalTriggerIndices(trigKit);
    സ്ഥിരം auto& vifVscpToTrig = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);
    സ്ഥിരം V3Order::ExternalTriggersProvider externalTriggers
        = [&](സ്ഥിരം AstVarScope* vscp, std::set<uint32_t>& triggers) {
              auto it = timingTriggers.find(vscp);
              എങ്കിൽ (it != timingTriggers.end())
                  triggers.തിരുകുക(it->second.begin(), it->second.end());
              എങ്കിൽ (vscp->varp()->isWrittenByDpi()) {
                  triggers.തിരുകുക(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
              }
              എങ്കിൽ (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                  addTriggeredIface(vscp, vifVscpToTrig, virtIfaceTriggers, triggers);
              }
          };

    AstCFunc* സ്ഥിരം actFuncp
        = V3Order::order({&logicRegions.m_pre, &logicRegions.m_act, &logicReplicas.m_act}, trigKit,
                         trigKit.vscp(), cgRefBindings, "act", അസത്യം, അസത്യം, externalTriggers);
    util::splitCheck(actFuncp);
    എങ്കിൽ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-act");

    സ്ഥിരം EvalKit actKit{trigKit.vscp(), actFuncp};

    // Orders a region's logic and creates the region eval function
    സ്ഥിരം auto order = [&](സ്ഥിരം std::string& name,
                           സ്ഥിരം std::vector<V3Sched::LogicByScope*>& logic) -> EvalKit {
        UINFO(2, "Scheduling " << name << " #logic = " << logic.size());
        AstVarScope* സ്ഥിരം trigVscp = trigKit.newTrigVec(name);
        // Add the trigger indices of the input logic
        വേണ്ടി (LogicByScope* lbs : logic) addTriggerIndices(*lbs, trigKit, അസത്യം);

        AstCFunc* സ്ഥിരം funcp
            = V3Order::order(logic, trigKit, trigVscp, cgRefBindings, name,
                             name == "nba" && v3Global.opt.mtasks(), അസത്യം, externalTriggers);

        തിരികെ {trigVscp, funcp};
    };

    // Step 12: Create the 'nba' region evaluation function
    സ്ഥിരം EvalKit nbaKit = order("nba", {&logicRegions.m_nba, &logicReplicas.m_nba});
    util::splitCheck(nbaKit.m_funcp);
    എങ്കിൽ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-nba");

    // Orders a region's logic and creates the region eval function (only if there is any logic in
    // the region)
    സ്ഥിരം auto orderIfNonEmpty
        = [&](സ്ഥിരം std::string& name, സ്ഥിരം std::vector<LogicByScope*>& logic) -> EvalKit {
        എങ്കിൽ (logic[0]->empty())
            തിരികെ {};  // if region is empty, replica is supposed to be empty as well
        സ്ഥിരം auto& kit = order(name, logic);
        എങ്കിൽ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-" + name);
        തിരികെ kit;
    };

    // Step 13: Create the 'obs' region evaluation function
    സ്ഥിരം EvalKit obsKit = orderIfNonEmpty("obs", {&logicRegions.m_obs, &logicReplicas.m_obs});

    // Step 14: Create the 'react' region evaluation function
    സ്ഥിരം EvalKit reactKit
        = orderIfNonEmpty("react", {&logicRegions.m_react, &logicReplicas.m_react});

    // Step 15: Create the 'postponed' region evaluation function
    createPostponed(netlistp, logicClasses);

    // Step 16: Populate the eval entry point function of each region of a time step
    createEval(netlistp, trigKit, actKit, nbaKit, obsKit, reactKit, timingKit);

    // Step 17: Add neccessary evaluation before awaits
    എങ്കിൽ (AstCCall* സ്ഥിരം readyp = timingKit.createReady(trigKit.vscp())) {
        staticp->addStmtsp(readyp->makeStmt());
        beforeTrigVisitor(netlistp, sen_വ്യഞ്ജകം_Builder, trigKit);
    } അല്ലെങ്കിൽ {
        // beforeTrigVisitor clears Sentree pointers in AstCAwaits (as these sentrees will get
        // deleted later) if there was no need to call it, SenTrees have to be cleaned manually
        netlistp->foreach([](AstCAwait* സ്ഥിരം cAwaitp) { cAwaitp->clearSentreep(); });
    }
    എങ്കിൽ (AstVarScope* സ്ഥിരം trigAccp = trigKit.vscAccp()) {
        // Copy trigger vector to accumulator at the end of static initialziation so,
        // triggers fired during initialization persist to the first resume.
        സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം trigAccDTypep
            = VN_AS(trigAccp->dtypep(), UnpackArrayDType);
        UASSERT_OBJ(
            trigAccDTypep->വലത്() == 0, trigAccp,
            "Expected that trigger vector and accumulator start elements enumeration from 0");
        UASSERT_OBJ(trigAccDTypep->ഇടത്() >= 0, trigAccp,
                    "Expected that trigger vector and accumulator has no negative indexes");
        File_വരി* സ്ഥിരം flp = trigAccp->fileline();
        AstVarScope* സ്ഥിരം vscp = netlistp->മുകൾ_Scopep()->createTemp("__Vi", 32);
        AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{flp};
        loopp->addStmtsp(
            പുതിയ AstAssign{flp,
                          പുതിയ AstArraySel{flp, പുതിയ AstVarRef{flp, trigAccp, VAccess::WRITE},
                                          പുതിയ AstVarRef{flp, vscp, VAccess::READ}},
                          പുതിയ AstArraySel{flp, പുതിയ AstVarRef{flp, actKit.m_vscp, VAccess::READ},
                                          പുതിയ AstVarRef{flp, vscp, VAccess::READ}}});
        loopp->addStmtsp(util::incrementVar(vscp));
        loopp->addStmtsp(പുതിയ AstLoopTest{
            flp, loopp,
            പുതിയ AstLte{flp, പുതിയ AstVarRef{flp, vscp, VAccess::READ},
                       പുതിയ AstConst{flp, AstConst::Widthed_മൂല്യം{}, 32,
                                    static_cast<uint32_t>(trigAccDTypep->ഇടത്())}}});
        staticp->addStmtsp(loopp);
    }

    // Step 18: Clean up
    netlistp->clearStlFirstIterationp();

    എങ്കിൽ (v3Global.opt.stats()) {
        // A sample() call resolved to the union over its covergroup's constructions reads more
        // than it can, which orders it against more logic than necessary
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per instance",
                         cgRefBindings.അക്കം_Exact_Calls());
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per type",
                         cgRefBindings.അക്കം_Union_Calls());
    }

    // Haven't split static initializer yet
    util::splitCheck(staticp);

    // Record eval stats
    netlistp->addEvalStats("sched");

    // Dump
    V3Global::dump_Check_Global_വൃക്ഷം("sched", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}

}  // namespace V3Sched
