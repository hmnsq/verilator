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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Sched.h"

#समावेश "V3Const.h"
#समावेश "V3EmitCBase.h"
#समावेश "V3EmitV.h"
#समावेश "V3Order.h"
#समावेश "V3SenExprBuilder.h"
#समावेश "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

namespace V3Sched {

namespace {

//============================================================================
// Utility functions

std::vector<स्थिर Ast_Sen_वृक्ष*> getSenTreesUsedBy(स्थिर std::vector<स्थिर LogicByScope*>& lbsps) {
    स्थिर VNUser1InUse user1InUse;
    std::vector<स्थिर Ast_Sen_वृक्ष*> परिणाम;
    क्रम (स्थिर LogicByScope* स्थिर lbsp : lbsps) {
        क्रम (स्थिर स्वतः& entry : *lbsp) {
            AstActive* स्थिर activep = entry.m_activep;
            Ast_Sen_वृक्ष* स्थिर senTreep = activep->sentreep();
            अगर (senTreep->user1SetOnce()) जारी;
            अगर (senTreep->hasClocked() || senTreep->hasHybrid()) परिणाम.धकेलो_back(senTreep);
        }
    }
    वापस परिणाम;
}

व्योम addTriggerIndices(LogicByScope& lbs, स्थिर TriggerKit& trigKit, bool pre) {
    क्रम (LogicByScope::Entry& entry : lbs) {
        Ast_Sen_वृक्ष* स्थिर senTreep = entry.m_activep->sentreep();
        अगर (senTreep->hasCombo()) जारी;
        entry.m_triggers = trigKit.triggerIndices(senTreep, pre);
    }
}

// Add the VIF triggers that a given VarScope should be sensitive to.
// Case 2 (non-virtual interface read): sensitive to that specific VarScope's trigger only
// Case 3 (virtual interface read): sensitive to all member triggers of the same interface type
व्योम addTriggeredIface(स्थिर AstVarScope* vscp, स्थिर VirtIfaceTriggers::VscpTrigMap& vscpToTrig,
                       स्थिर VirtIfaceTriggers& virtIfaceTriggers, std::set<uint32_t>& triggers) {
    अगर (vscp->varp()->isVirtIface()) {
        // Virtual interface variable -- sensitive to all member triggers of this interface type
        स्थिर AstIface* स्थिर ifacep = VN_AS(vscp->varp()->dtypep(), IfaceRefDType)->ifacep();
        क्रम (स्थिर स्वतः& entry : virtIfaceTriggers.m_triggers) {
            अगर (entry.m_ifacep == ifacep) {
                स्थिर स्वतः it = vscpToTrig.find(entry.m_vscp);
                अगर (it != vscpToTrig.end()) triggers.जोड़ो(it->second);
            }
        }
    } अन्यथा {
        // Non-virtual interface member -- sensitive to this VarScope's trigger only
        स्थिर स्वतः it = vscpToTrig.find(vscp);
        अगर (it != vscpToTrig.end()) triggers.जोड़ो(it->second);
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
व्योम createEvalRegion(
    AstNetlist* netlistp,  //
    VEval eval,  // The entry point of the current region
    // Index of the region's 'first iteration' extra trigger, if it has one, otherwise ignored
    uint32_t firstIterTrigger,
    स्थिर TriggerKit& trigKit,  // The trigger kit
    AstVarScope* trigp,  // The trigger vector - may be nullptr if no triggers or using 'condp'
    Ast_गाँठ_व्यंजक* condp,  // Explicit condition that must be true to run 'phaseWorkp'
    Ast_गाँठ_Stmt* phasePrepp,  // Prep statements run before checking triggers
    Ast_गाँठ_Stmt* phaseWorkp,  // The work to do if anything triggered
    // Extra statements to run after the work, even if no triggers fired. This function is
    // passed a variable, which must be set to true if we must continue and loop again,
    // and must be unmodified otherwise.
    std::function<Ast_गाँठ_Stmt*(AstVarScope*)> phaseExtra = [](AstVarScope*) { वापस nullptr; }  //
) {
    UASSERT(!trigp || !condp, "Cannot use both 'trigp' and 'condp' in 'createEvalRegion'");
    UASSERT(!eval.firstIteration() || trigp,
            "Region without triggers cannot need a first iteration flag");

    // All work is under a trigger or condition, so with neither the region has
    // nothing to evaluate, and what we create below reduces to a no-op function.

    स्थिर std::string tag = eval.tag();
    Ast_शीर्ष_Scope* स्थिर शीर्ष_Scopep = netlistp->शीर्ष_Scopep();
    File_पंक्ति* स्थिर flp = netlistp->fileline();

    // Populate the trigger dump entry point function
    अगर (trigp) {
        netlistp->dumpTriggersFuncp(eval)->addStmtsp(trigKit.newDumpCall(trigp, tag, false));
    }

    AstCFunc* स्थिर funcp = netlistp->evalFuncp(eval);

    // A flag is passed from the run-time eval loop if this is the first iteration of the
    // current loop
    अगर (eval.firstIteration()) {
        AstVarScope* स्थिर firstIterArgp = util::newArgument(funcp, netlistp->findBitDType(),
                                                             "firstIteration", VDirection::निवेश);

        // Set the region's 'first iteration' trigger straight from the argument
        funcp->addStmtsp(trigKit.newExtraTriggerAssignment(firstIterArgp, firstIterTrigger));

        // Only 'stl' also needs a module level flag, for design logic that reads
        // it directly (see V3Timing). Those reads can be anywhere in the design,
        // hence module level. Always created, even if nothing reads it.
        // TODO: get rid of this special case
        अगर (eval == VEval::STL) {
            AstVarScope* स्थिर firstIterp = netlistp->stlFirstIterationp();
            firstIterp->varp()->noReset(true);
            firstIterp->varp()->isInternal(true);
            funcp->addStmtsp(नया AstAssign{flp, नया AstVarRef{flp, firstIterp, VAccess::WRITE},
                                           नया AstVarRef{flp, firstIterArgp, VAccess::READ}});
        }
    }

    {
        // Add the preparatory statements
        funcp->addStmtsp(phasePrepp);

        // The execute flag
        AstVarScope* स्थिर executeFlagp = शीर्ष_Scopep->create_अस्थायी("__V" + tag + "Execute", 1);
        executeFlagp->varp()->noReset(true);

        // If there is work in this region, execute it if any triggers fired
        अगर (phaseWorkp) {
            Ast_गाँठ_व्यंजक* स्थिर lhsp = नया AstVarRef{flp, executeFlagp, VAccess::WRITE};
            // If using explicit condition, that directly determines whether to execute,
            // otherwise check if any triggers are fired
            Ast_गाँठ_व्यंजक* स्थिर rhsp = condp ? condp : trigKit.newAnySetCall(trigp);
            funcp->addStmtsp(नया AstAssign{flp, lhsp, rhsp});

            // Add the work
            AstIf* स्थिर ifp = नया AstIf{flp, नया AstVarRef{flp, executeFlagp, VAccess::READ}};
            ifp->addThensp(phaseWorkp);
            funcp->addStmtsp(ifp);
        }

        // Construct the extra statements
        Ast_गाँठ_Stmt* स्थिर extraWorkp = phaseExtra(executeFlagp);
        अगर (extraWorkp) funcp->addStmtsp(extraWorkp);

        // The function returns true iff it did run work
        Ast_गाँठ_व्यंजक* स्थिर retp
            = phaseWorkp || extraWorkp
                  ? static_cast<Ast_गाँठ_व्यंजक*>(नया AstVarRef{flp, executeFlagp, VAccess::READ})
                  : static_cast<Ast_गाँठ_व्यंजक*>(नया AstConst{flp, AstConst::BitFalse{}});
        funcp->addStmtsp(नया AstCReturn{flp, retp});
    }
}

//============================================================================
// Collect and classify all logic in the design

LogicClasses gatherLogicClasses(AstNetlist* netlistp) {
    LogicClasses परिणाम;

    netlistp->foreach([&](AstScope* scopep) {
        scopep->foreach([&](AstActive* activep) {
            Ast_Sen_वृक्ष* स्थिर senTreep = activep->sentreep();
            अगर (senTreep->hasStatic()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "static initializer with additional sensitivities");
                परिणाम.m_static.add(scopep, activep);
            } अन्यथा अगर (senTreep->hasInitial()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'initial' logic with additional sensitivities");
                परिणाम.m_initial.add(scopep, activep);
            } अन्यथा अगर (senTreep->hasFinal()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'final' logic with additional sensitivities");
                परिणाम.m_final.add(scopep, activep);
            } अन्यथा अगर (senTreep->hasCombo()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "combinational logic with additional sensitivities");
                अगर (VN_IS(activep->stmtsp(), AlwaysPostponed)) {
                    परिणाम.m_postponed.add(scopep, activep);
                } अन्यथा {
                    परिणाम.m_comb.add(scopep, activep);
                }
            } अन्यथा {
                UASSERT_OBJ(senTreep->hasClocked(), activep, "What else could it be?");
                अगर (VN_IS(activep->stmtsp(), AlwaysObserved)) {
                    परिणाम.m_observed.add(scopep, activep);
                } अन्यथा अगर (VN_IS(activep->stmtsp(), AlwaysReactive)) {
                    परिणाम.m_reactive.add(scopep, activep);
                } अन्यथा {
                    परिणाम.m_clocked.add(scopep, activep);
                }
            }
        });
    });

    वापस परिणाम;
}

//============================================================================
// Simple ordering in source order

व्योम orderSequentially(AstCFunc* funcp, स्थिर LogicByScope& lbs) {
    // Create new subfunc for scope
    स्थिर स्वतः createNewSubFuncp = [&](AstScope* स्थिर scopep) {
        स्थिर string subName{funcp->name() + "__" + scopep->nameDotless()};
        AstCFunc* स्थिर subFuncp = नया AstCFunc{scopep->fileline(), subName, scopep};
        subFuncp->isLoose(true);
        subFuncp->isConst(false);
        subFuncp->declPrivate(true);
        subFuncp->slow(funcp->slow());
        scopep->addBlocksp(subFuncp);
        // Call it from the top function
        funcp->addStmtsp(util::callVoidFunc(subFuncp));
        वापस subFuncp;
    };
    स्थिर VNUser1InUse user1InUse;  // AstScope -> AstCFunc: the sub-function for the scope
    स्थिर VNUser2InUse user2InUse;  // AstScope -> int: sub-function counter used for names
    क्रम (स्थिर स्वतः& entry : lbs) {
        AstScope* स्थिर scopep = entry.m_scopep;
        AstActive* स्थिर activep = entry.m_activep;
        // Create a sub-function per scope so we can V3Combine them later
        अगर (!scopep->user1p()) scopep->user1p(createNewSubFuncp(scopep));
        // Add statements to sub-function
        क्रम (Ast_गाँठ *logicp = activep->stmtsp(), *nextp; logicp; logicp = nextp) {
            स्वतः* subFuncp = VN_AS(scopep->user1p(), CFunc);
            nextp = logicp->nextp();
            अगर (Ast_गाँठ_Procedure* स्थिर procp = VN_CAST(logicp, गाँठ_Procedure)) {
                अगर (Ast_गाँठ* bodyp = procp->stmtsp()) {
                    bodyp->unlink_Fr_Back_With_अग्र();
                    // If the process is suspendable, we need a separate function (a coroutine)
                    अगर (procp->isSuspendable()) {
                        funcp->slow(false);
                        subFuncp = createNewSubFuncp(scopep);
                        subFuncp->name(subFuncp->name() + "__Vtiming__"
                                       + cvtToStr(scopep->user2Inc()));
                        subFuncp->rtnType("VlCoroutine");
                        अगर (VN_IS(procp, Always)) {
                            subFuncp->slow(false);
                            File_पंक्ति* स्थिर flp = procp->fileline();
                            Ast_गाँठ_व्यंजक* स्थिर condp = नया AstCExpr{
                                flp, "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish())", 1};
                            AstLoop* स्थिर loopp = नया AstLoop{flp};
                            loopp->addStmtsp(नया AstLoopTest{flp, loopp, condp});
                            loopp->addStmtsp(bodyp);
                            bodyp = loopp;
                        }
                    }
                    subFuncp->addStmtsp(bodyp);
                    अगर (procp->needProcess()) subFuncp->setNeedProcess();
                    // A coroutine holds a single process, so it is complete here
                    अगर (procp->isSuspendable()) util::splitCheck(subFuncp);
                }
            } अन्यथा {
                logicp->unlinkFrBack();
                subFuncp->addStmtsp(logicp);
            }
        }
        अगर (activep->backp()) activep->unlinkFrBack();
        VL_DO_DANGLING(activep->delete_वृक्ष(), activep);
    }
    // Split sub-functions once complete
    क्रम (स्थिर स्वतः& entry : lbs) {
        AstScope* स्थिर scopep = entry.m_scopep;
        अगर (AstCFunc* स्थिर subFuncp = VN_AS(scopep->user1p(), CFunc)) {
            util::splitCheck(subFuncp);
            scopep->user1p(nullptr);
        }
    }
}

//============================================================================
// Create simply ordered functions

AstCFunc* createStatic(AstNetlist* netlistp, स्थिर LogicClasses& logicClasses) {
    AstCFunc* स्थिर funcp = netlistp->evalFuncp(VEval::STATIC);

    स्थिर LogicByScope& orig = logicClasses.m_static;
    अगर (orig.आकार() <= 1) {
        orderSequentially(funcp, orig);
        वापस funcp;
    }

    // Level-based module sorting can reorder packages so that an importing
    // package runs before the imported one.  Re-sort package entries by source
    // file position to restore compilation order (IEEE 1800-2023 26.3).
    std::vector<माप_प्रकार> indices(orig.आकार());
    क्रम (माप_प्रकार i = 0; i < orig.आकार(); ++i) indices[i] = i;
    std::stable_sort(indices.begin(), indices.end(), [&](माप_प्रकार a, माप_प्रकार b) {
        स्थिर Ast_गाँठ_Module* स्थिर modA = orig[a].m_scopep->modp();
        स्थिर Ast_गाँठ_Module* स्थिर modB = orig[b].m_scopep->modp();
        स्थिर bool isPkgA = VN_IS(modA, Package);
        स्थिर bool isPkgB = VN_IS(modB, Package);
        अगर (isPkgA != isPkgB) वापस isPkgA;  // Packages before non-packages
        अगर (isPkgA && isPkgB) {
            // Sort packages by source file position (compilation order)
            वापस modA->fileline()->operatorCompare(*modB->fileline()) < 0;
        }
        वापस false;  // Both non-package: preserve original order
    });
    LogicByScope sorted;
    क्रम (स्थिर माप_प्रकार i : indices) sorted.add(orig[i].m_scopep, orig[i].m_activep);

    orderSequentially(funcp, sorted);
    वापस funcp;  // Not splitting yet as it is not final
}

व्योम createInitial(AstNetlist* netlistp, स्थिर LogicClasses& logicClasses) {
    AstCFunc* स्थिर funcp = netlistp->evalFuncp(VEval::INITIAL);
    orderSequentially(funcp, logicClasses.m_initial);
    util::splitCheck(funcp);
}

व्योम createPostponed(AstNetlist* netlistp, स्थिर LogicClasses& logicClasses) {
    AstCFunc* स्थिर funcp = netlistp->evalFuncp(VEval::POSTPONED);
    orderSequentially(funcp, logicClasses.m_postponed);
    util::splitCheck(funcp);
}

व्योम createFinal(AstNetlist* netlistp, स्थिर LogicClasses& logicClasses) {
    AstCFunc* स्थिर funcp = netlistp->evalFuncp(VEval::FINAL);
    orderSequentially(funcp, logicClasses.m_final);
    util::splitCheck(funcp);
}

//============================================================================
// Helper that creates virtual interface value-change triggers

व्योम addVirtIfaceTriggerAssignments(AstNetlist* netlistp, AstCFunc* initFuncp,
                                    स्थिर VirtIfaceTriggers& virtIfaceTriggers,
                                    uint32_t first_सूचकांक, स्थिर TriggerKit& trigKit) {
    uint32_t सूचकांक_2 = first_सूचकांक;
    क्रम (स्थिर स्वतः& entry : virtIfaceTriggers.m_triggers) {
        trigKit.add_मूल्य_Change_Trigger_Assignment(netlistp, initFuncp, entry.m_vscp, सूचकांक_2);
        ++सूचकांक_2;
    }
}

// Order the combinational logic to create the 'stl' region
व्योम createSettle(AstNetlist* netlistp, AstCFunc* स्थिर initFuncp, Sen_व्यंजक_Builder& sen_व्यंजक_Bulider,
                  LogicClasses& logicClasses, स्थिर CovergroupRefBindings& cgRefBindings) {
    // Clone, because ordering is destructive, but we still need them for the other regions
    LogicByScope comb = logicClasses.m_comb.clone();
    LogicByScope hybrid = logicClasses.m_hybrid.clone();

    // We have an extra trigger denoting this is the first iteration of the settle loop
    TriggerKit::ExtraTriggers extraTriggers;
    स्थिर uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");

    // Gather the relevant sensitivity expressions and create the trigger kit
    स्थिर स्वतः& senTreeps = getSenTreesUsedBy({&comb, &hybrid});
    स्थिर TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_व्यंजक_Bulider, {},
                                                  senTreeps, "stl", extraTriggers, true, false);

    // Add trigger indices (comb has none, so only do the hybrid)
    addTriggerIndices(hybrid, trigKit, false);

    // First trigger is for pure combinational triggers (first iteration)
    स्थिर uint32_t निवेश_Changed = trigKit.extra_Trigger_सूचकांक(firstIterationTrigger);

    // Create and the body function
    AstCFunc* स्थिर stlFuncp = V3Order::order(
        {&comb, &hybrid}, trigKit, trigKit.vscp(), cgRefBindings, "stl", false, true,
        [=](स्थिर AstVarScope*, std::set<uint32_t>& triggers) { triggers.जोड़ो(निवेश_Changed); });
    util::splitCheck(stlFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::STL, firstIterationTrigger, trigKit,
        // Use trigger
        trigKit.vscp(), nullptr,
        // Explicit condition
        // Prep statements: Compute the current 'stl' triggers
        [&trigKit] {
            Ast_गाँठ_Stmt* स्थिर stmtp = trigKit.newCompBaseCall();
            अगर (stmtp) stmtp->add_अग्र(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), true));
            वापस stmtp;
        }(),
        // Work statements: Invoke the 'stl' function
        util::callVoidFunc(stlFuncp));
}

//============================================================================
// Order the replicated combinational logic to create the 'ico' region

व्योम createIcoRegion(AstNetlist* netlistp, AstCFunc* स्थिर initFuncp,
                     Sen_व्यंजक_Builder& sen_व्यंजक_Builder, LogicByScope& logic,
                     स्थिर VirtIfaceTriggers& virtIfaceTriggers,
                     स्थिर CovergroupRefBindings& cgRefBindings) {
    // SystemC only: Any top level inputs feeding a combinational logic must be marked,
    // so we can make them sc_sensitive
    अगर (v3Global.opt.systemC()) {
        logic.foreachLogic([](Ast_गाँठ* logicp) {
            logicp->foreach([](AstVarRef* refp) {
                अगर (refp->access().isWriteOnly()) वापस;
                AstVarScope* स्थिर vscp = refp->varScopep();
                अगर (vscp->scopep()->is_शीर्ष() && vscp->varp()->isNonOutput()) {
                    vscp->varp()->scSensitive(true);
                }
            });
        });
    }

    // We have some extra trigger denoting external conditions
    AstVarScope* स्थिर dpiExportTriggerVscp = netlistp->dpiExportTriggerp();

    TriggerKit::ExtraTriggers extraTriggers;
    स्थिर uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");
    स्थिर uint32_t dpi_Export_Trigger_सूचकांक = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    स्थिर uint32_t first_Vif_Trigger_सूचकांक = extraTriggers.आकार();
    क्रम (स्थिर स्वतः& entry : virtIfaceTriggers.m_triggers) {
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
    std::unordered_map<स्थिर AstVarScope*, Ast_Sen_वृक्ष*> inp2changedp;
    std::vector<Ast_Sen_वृक्ष*> icoChangeSenTreeps;
    अगर (v3Global.opt.fIcoChangeDetect().isTrue()) {
        File_पंक्ति* स्थिर flp = netlistp->fileline();
        AstScope* स्थिर scopep = netlistp->शीर्ष_Scopep()->scopep();
        क्रम (AstVarScope* vscp = scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            // Only for top level ports, assume outputs don't change externally
            अगर (!vscp->varp()->isPrimaryInish()) जारी;
            // Don't do if written by the design - wouldn't update the change detect 'prev' value
            अगर (vscp->varp()->icoMaybeWritten()) जारी;
            // Don't do if forceable, as we can't see the actual value - this is belt and braces
            अगर (vscp->varp()->isForced()) जारी;
            // Can't handle unpacked arrays (they have special types when primary input)
            अगर (VN_IS(vscp->dtypep()->skipRefp(), UnpackArrayDType)) जारी;
            // Similarly to arrays, can't handle SystemC types
            अगर (vscp->varp()->isSc()) जारी;
            // Create a sen tree triggered when this input changes
            Ast_Sen_वृक्ष*& senTreepr = inp2changedp[vscp];
            UASSERT_OBJ(!senTreepr, vscp, "Duplicate input change detect trigger");
            AstVarRef* स्थिर refp = नया AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* स्थिर senItemp = नया AstSenItem{flp, VEdgeType::ET_CHANGED, refp};
            senTreepr = नया Ast_Sen_वृक्ष{flp, senItemp};
            icoChangeSenTreeps.धकेलो_back(senTreepr);
        }
    }
    V3Stats::addStat("Scheduling, 'ico' change detect triggers", icoChangeSenTreeps.आकार());

    // Gather the relevant sensitivity expressions and create the trigger kit
    std::vector<स्थिर Ast_Sen_वृक्ष*> senTreeps = getSenTreesUsedBy({&logic});
    senTreeps.जोड़ो(senTreeps.end(), icoChangeSenTreeps.begin(), icoChangeSenTreeps.end());
    स्थिर TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_व्यंजक_Builder, {},
                                                  senTreeps, "ico", extraTriggers, false, false);
    std::ignore = sen_व्यंजक_Builder.getAndClearResults();

    अगर (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpi_Export_Trigger_सूचकांक);
    }
    addVirtIfaceTriggerAssignments(netlistp, initFuncp, virtIfaceTriggers, first_Vif_Trigger_सूचकांक,
                                   trigKit);

    // Add trigger indices
    addTriggerIndices(logic, trigKit, false);

    // The 'first iteration' trigger for top level inputs
    स्थिर uint32_t firstIterTrigger = trigKit.extra_Trigger_सूचकांक(firstIterationTrigger);

    स्थिर स्वतः& vifVscpToTrigIco
        = virtIfaceTriggers.makeVscpToTrigMap(trigKit, first_Vif_Trigger_सूचकांक);

    // Create and Order the body function
    AstCFunc* स्थिर icoFuncp = V3Order::order(
        {&logic}, trigKit, trigKit.vscp(), cgRefBindings, "ico", false, false,
        [&](स्थिर AstVarScope* vscp, std::set<uint32_t>& triggers) {
            AstVar* स्थिर varp = vscp->varp();
            // If it has an explicit change detect trigger, use that,
            // otherwise fall back to using the 'first iteration' trigger
            स्वतः it = inp2changedp.find(vscp);
            अगर (it != inp2changedp.end()) {
                triggers.जोड़ो(trigKit.senItem2TrigIdx(it->second->sensesp()));
            } अन्यथा अगर (varp->isPrimaryInish() || varp->isSigUserRWPublic() || varp->sampled()) {
                triggers.जोड़ो(firstIterTrigger);
            }
            // Add other triggers
            अगर (varp->isWrittenByDpi())
                triggers.जोड़ो(trigKit.extra_Trigger_सूचकांक(dpi_Export_Trigger_सूचकांक));
            अगर (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
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
            Ast_गाँठ_Stmt* स्थिर stmtp = trigKit.newCompBaseCall();
            अगर (stmtp) stmtp->add_अग्र(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), true));
            वापस stmtp;
        }(),
        // Work statements: Invoke the 'ico' function
        util::callVoidFunc(icoFuncp));

    // Release temporary input change detect SenTrees
    क्रम (Ast_Sen_वृक्ष* स्थिर senTreep : icoChangeSenTreeps)
        VL_DO_DANGLING(senTreep->delete_वृक्ष(), senTreep);
    icoChangeSenTreeps.सफा();
}

//============================================================================
// EvalKit groups items that have to be passed to createEval() for a given eval region

काष्ठा EvalKit final {
    // The AstVarScope representing the region's trigger vector
    AstVarScope* स्थिर m_vscp = nullptr;
    // The AstCFunc that evaluates the region's logic
    AstCFunc* स्थिर m_funcp = nullptr;
    // Is this kit used/required?
    bool empty() स्थिर { वापस !m_funcp; }
};

//============================================================================
// Create the evaluation function of each region of a time step

व्योम createEval(AstNetlist* netlistp,  //
                स्थिर TriggerKit& trigKit,  //
                स्थिर EvalKit& actKit,  //
                स्थिर EvalKit& nbaKit,  //
                स्थिर EvalKit& obsKit,  //
                स्थिर EvalKit& reactKit,  //
                TimingKit& timingKit  //
) {
    File_पंक्ति* स्थिर flp = netlistp->fileline();

    // Grab the delay scheduler variable, if any
    AstVarScope* स्थिर delaySchedVscp = timingKit.getDelayScheduler(netlistp);

    // 'createResume' consumes the contents that 'createReady' needs, so do the right order
    AstCCall* स्थिर timingReadyp = timingKit.createReady(trigKit.vscp());
    AstCCall* स्थिर timingResumep = timingKit.createResume(trigKit.vscp());

    // Create the 'act' region
    createEvalRegion(  //
        netlistp, VEval::ACT, 0, trigKit,
        // Use trigger
        actKit.m_vscp, nullptr,
        // Prep statements
        [&]() {
            // Compute the current 'act' triggers - the NBA triggers are the latched value
            Ast_गाँठ_Stmt* stmtsp = trigKit.newCompBaseCall();
            Ast_गाँठ_Stmt* स्थिर dumpp
                = stmtsp ? trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), true) : nullptr;
            // Mark as ready for triggered awaits
            अगर (timingReadyp) stmtsp = Ast_गाँठ::add_अग्र(stmtsp, timingReadyp->makeStmt());
            अगर (AstVarScope* स्थिर vscAccp = trigKit.vscAccp()) {
                stmtsp = Ast_गाँठ::add_अग्र(stmtsp, trigKit.newOrIntoCall(actKit.m_vscp, vscAccp));
            }
            stmtsp = Ast_गाँठ::add_अग्र(stmtsp, trigKit.newCompExtCall(nbaKit.m_vscp));
            stmtsp = Ast_गाँठ::add_अग्र(stmtsp, dumpp);
            // Latch the 'act' triggers under the 'nba' triggers
            stmtsp = Ast_गाँठ::add_अग्र(stmtsp, trigKit.newOrIntoCall(nbaKit.m_vscp, actKit.m_vscp));
            //
            वापस stmtsp;
        }(),
        // Work statements
        [&]() {
            Ast_गाँठ_Stmt* workp = nullptr;
            अगर (AstVarScope* स्थिर actAccp = trigKit.vscAccp()) {
                AstCMethodHard* स्थिर cCallp = नया AstCMethodHard{
                    flp, नया AstVarRef{flp, actAccp, VAccess::WRITE}, VCMethod::UNPACKED_FILL,
                    नया AstConst{flp, AstConst::Unsized64{}, 0}};
                cCallp->dtypeSetVoid();
                workp = Ast_गाँठ::add_अग्र(workp, cCallp->makeStmt());
            }
            // Resume triggered timing schedulers
            अगर (timingResumep) workp = Ast_गाँठ::add_अग्र(workp, timingResumep->makeStmt());
            // Invoke the 'act' function
            workp = Ast_गाँठ::add_अग्र(workp, util::callVoidFunc(actKit.m_funcp));
            //
            वापस workp;
        }());

    // Create the 'inact' region
    createEvalRegion(  //
        netlistp, VEval::INACT, 0, trigKit,
        // Use explicit condition
        nullptr,
        [&]() -> Ast_गाँठ_व्यंजक* {
            अगर (!delaySchedVscp) वापस nullptr;
            // Run if any zero delays are pending
            Ast_गाँठ_व्यंजक* स्थिर callp
                = नया AstCMethodHard{flp, नया AstVarRef{flp, delaySchedVscp, VAccess::READ},
                                     VCMethod::SCHED_AWAITING_ZERO_DELAY};
            callp->dtypeSetBit();
            वापस callp;
        }(),
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_गाँठ_Stmt* {
            अगर (!delaySchedVscp) {
                // Nothing to do if there are no delays at all in the design
                वापस nullptr;
            } अन्यथा अगर (v3Global.usesZeroDelay()) {
                // Resume processes watiting for #0 delay
                AstCMethodHard* स्थिर callp = नया AstCMethodHard{
                    flp, नया AstVarRef{flp, delaySchedVscp, VAccess::READWRITE},
                    VCMethod::SCHED_RESUME_ZERO_DELAY};
                callp->dtypeSetVoid();
                वापस callp->makeStmt();
            } अन्यथा {
                // Assumption was that the design doesn't use #0 delays.
                // Die at run-time if it does.
                AstCStmt* स्थिर stmtp = नया AstCStmt{flp};
                स्थिर File_पंक्ति* स्थिर locp = netlistp->शीर्ष_Modulep()->fileline();
                स्थिर std::string& file = VIdProtect::protect(locp->filename());
                स्थिर std::string& पंक्ति = std::to_string(locp->lineno());
                stmtp->add("VL_FATAL_MT(\"" + V3OutFormatter::quoteNameControls(file) + "\", "
                           + पंक्ति
                           + ", \"\", \"ZERODLY: Design Verilated with '--no-sched-zero-delay', "
                           + "but #0 delay executed at runtime\");");
                वापस stmtp;
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
            Ast_गाँठ_Stmt* workp = nullptr;
            // Latch the 'nba' trigger flags under the following region's trigger flags
            अगर (!obsKit.empty()) {
                workp = trigKit.newOrIntoCall(obsKit.m_vscp, nbaKit.m_vscp);
            } अन्यथा अगर (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, nbaKit.m_vscp);
            }
            // Invoke the 'nba' function
            workp = Ast_गाँठ::add_अग्र(workp, util::callVoidFunc(nbaKit.m_funcp));
            // Clear the 'nba' triggers
            workp = Ast_गाँठ::add_अग्र(workp, trigKit.newClearCall(nbaKit.m_vscp));
            //
            वापस workp;
        }(),
        // Extra work (not conditional on having had a fired trigger)
        [&](AstVarScope* continuep) -> Ast_गाँठ_Stmt* {
            // Check if any dynamic NBAs are pending, if there are any in the design
            अगर (!netlistp->nbaEventp()) वापस nullptr;
            AstVarScope* स्थिर nbaEventp = netlistp->nbaEventp();
            AstVarScope* स्थिर nbaEventTriggerp = netlistp->nbaEventTriggerp();
            UASSERT(nbaEventTriggerp, "NBA event trigger var should exist");
            netlistp->nbaEventp(nullptr);
            netlistp->nbaEventTriggerp(nullptr);

            // If a dynamic NBA is pending, clear the pending flag and fire the ready event
            AstIf* स्थिर ifp = नया AstIf{flp, नया AstVarRef{flp, nbaEventTriggerp, VAccess::READ}};
            ifp->addThensp(util::setVar(continuep, 1));
            ifp->addThensp(util::setVar(nbaEventTriggerp, 0));
            AstCMethodHard* स्थिर firep = नया AstCMethodHard{
                flp, नया AstVarRef{flp, nbaEventp, VAccess::WRITE}, VCMethod::EVENT_FIRE};
            firep->dtypeSetVoid();
            ifp->addThensp(firep->makeStmt());
            वापस ifp;
        });

    // Create the 'obs' region
    createEvalRegion(  //
        netlistp, VEval::OBS, 0, trigKit,
        // Use trigger
        obsKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_गाँठ_Stmt* {
            अगर (obsKit.empty()) वापस nullptr;
            Ast_गाँठ_Stmt* workp = nullptr;
            // Latch the Observed trigger flags under the Reactive trigger flags
            अगर (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, obsKit.m_vscp);
            }
            // Invoke the 'obs' function
            workp = Ast_गाँठ::add_अग्र(workp, util::callVoidFunc(obsKit.m_funcp));
            // Clear the 'obs' triggers
            workp = Ast_गाँठ::add_अग्र(workp, trigKit.newClearCall(obsKit.m_vscp));
            //
            वापस workp;
        }());

    // Create the 'react' region
    createEvalRegion(  //
        netlistp, VEval::REACT, 0, trigKit,
        // Use trigger
        reactKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_गाँठ_Stmt* {
            अगर (reactKit.empty()) वापस nullptr;
            // Invoke the 'react' function
            Ast_गाँठ_Stmt* workp = util::callVoidFunc(reactKit.m_funcp);
            // Clear the 'react' triggers
            workp = Ast_गाँठ::add_अग्र(workp, trigKit.newClearCall(reactKit.m_vscp));
            वापस workp;
        }());
}

}  // namespace

//============================================================================
// Helper that builds virtual interface trigger sentrees

VirtIfaceTriggers::VscpTrigMap VirtIfaceTriggers::makeVscpToTrigMap(स्थिर TriggerKit& trigKit,
                                                                    uint32_t first_सूचकांक) स्थिर {
    VscpTrigMap map;
    uint32_t सूचकांक_2 = first_सूचकांक;
    क्रम (स्थिर स्वतः& entry : m_triggers) {
        map.emplace(entry.m_vscp, trigKit.extra_Trigger_सूचकांक(सूचकांक_2));
        ++सूचकांक_2;
    }
    वापस map;
}

//============================================================================
// Top level entry-point to scheduling

व्योम schedule(AstNetlist* netlistp) {
    स्थिर स्वतः add_आकार_Stat = [](स्थिर string& name, स्थिर LogicByScope& lbs) {
        uint64_t आकार = 0;
        lbs.foreachLogic([&](Ast_गाँठ* nodep) { आकार += nodep->गाँठ_गिनती(); });
        V3Stats::addStat("Scheduling, " + name, आकार);
    };

    // Step 2: Prepare external domains for timing and virtual interfaces
    // Create extra triggers for virtual interfaces
    स्थिर स्वतः& virtIfaceTriggers = makeVirtIfaceTriggers(netlistp);
    // Resolve what covergroup reference formal arguments are bound to, which is visible here
    // but not from V3Order, where the reads through them must be modeled
    स्थिर CovergroupRefBindings& cgRefBindings = makeCovergroupRefBindings(netlistp);
    // Prepare timing-related logic and external domains
    TimingKit timingKit = prepareTiming(netlistp);

    // Step 3: Gather and classify all logic in the design
    LogicClasses logicClasses = gatherLogicClasses(netlistp);

    अगर (v3Global.opt.stats()) {
        V3Stats::statsStage("sched-gather");
        add_आकार_Stat("size of class: static", logicClasses.m_static);
        add_आकार_Stat("size of class: initial", logicClasses.m_initial);
        add_आकार_Stat("size of class: final", logicClasses.m_final);
    }

    // Step 4: Schedule static, initial and final logic classes in source order
    AstCFunc* स्थिर staticp = createStatic(netlistp, logicClasses);
    अगर (v3Global.opt.stats()) V3Stats::statsStage("sched-static");

    createInitial(netlistp, logicClasses);
    अगर (v3Global.opt.stats()) V3Stats::statsStage("sched-initial");

    createFinal(netlistp, logicClasses);
    अगर (v3Global.opt.stats()) V3Stats::statsStage("sched-final");

    // Step 5: Break combinational cycles by introducing hybrid logic
    // Note: breakCycles also removes corresponding logic from logicClasses.m_comb;
    logicClasses.m_hybrid = breakCycles(netlistp, logicClasses.m_comb);
    अगर (v3Global.opt.stats()) {
        add_आकार_Stat("size of class: clocked", logicClasses.m_clocked);
        add_आकार_Stat("size of class: combinational", logicClasses.m_comb);
        add_आकार_Stat("size of class: hybrid", logicClasses.m_hybrid);
        V3Stats::statsStage("sched-break-cycles");
    }

    // We pass around a single SenExprBuilder instance, as we only need one set of 'prev' variables
    // for edge/change detection in sensitivity expressions, which this keeps track of.
    Ast_शीर्ष_Scope* स्थिर शीर्ष_Scopep = netlistp->शीर्ष_Scopep();
    AstScope* स्थिर scopeTopp = शीर्ष_Scopep->scopep();
    Sen_व्यंजक_Builder sen_व्यंजक_Builder{scopeTopp};

    // Step 6: Create 'settle' region that restores the combinational invariant
    createSettle(netlistp, staticp, sen_व्यंजक_Builder, logicClasses, cgRefBindings);
    अगर (v3Global.opt.stats()) V3Stats::statsStage("sched-settle");

    // Step 7: Partition the clocked and combinational (including hybrid) logic into pre/act/nba.
    // All clocks (signals referenced in an AstSenTree) generated via a blocking assignment
    // (including combinationally generated signals) are computed within the act region.
    LogicRegions logicRegions
        = partition(logicClasses.m_clocked, logicClasses.m_comb, logicClasses.m_hybrid);
    logicRegions.m_obs = logicClasses.m_observed;
    logicRegions.m_react = logicClasses.m_reactive;
    अगर (v3Global.opt.stats()) {
        add_आकार_Stat("size of region: Active Pre", logicRegions.m_pre);
        add_आकार_Stat("size of region: Active", logicRegions.m_act);
        add_आकार_Stat("size of region: NBA", logicRegions.m_nba);
        add_आकार_Stat("size of region: Observed", logicRegions.m_obs);
        add_आकार_Stat("size of region: Reactive", logicRegions.m_react);
        V3Stats::statsStage("sched-partition");
    }

    // Step 8: Replicate combinational logic
    LogicReplicas logicReplicas = replicateLogic(logicRegions);
    अगर (v3Global.opt.stats()) {
        add_आकार_Stat("size of replicated logic: Input", logicReplicas.m_ico);
        add_आकार_Stat("size of replicated logic: Active", logicReplicas.m_act);
        add_आकार_Stat("size of replicated logic: NBA", logicReplicas.m_nba);
        add_आकार_Stat("size of replicated logic: Observed", logicReplicas.m_obs);
        add_आकार_Stat("size of replicated logic: Reactive", logicReplicas.m_react);
        V3Stats::statsStage("sched-replicate");
    }

    // Step 9: Create the input combinational logic
    createIcoRegion(netlistp, staticp, sen_व्यंजक_Builder, logicReplicas.m_ico, virtIfaceTriggers,
                    cgRefBindings);
    अगर (v3Global.opt.stats()) V3Stats::statsStage("sched-create-ico");

    // Step 10: Create the triggers
    AstVarScope* स्थिर dpiExportTriggerVscp = netlistp->dpiExportTriggerp();
    netlistp->dpiExportTriggerp(nullptr);  // Finished with this here

    // We may have an extra trigger for variable updated in DPI exports
    TriggerKit::ExtraTriggers extraTriggers;
    स्थिर uint32_t dpi_Export_Trigger_सूचकांक = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    स्थिर uint32_t first_Vif_Trigger_सूचकांक = extraTriggers.आकार();
    क्रम (स्थिर स्वतः& entry : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + entry.m_ifacep->name() + "."
                               + entry.m_memberp->name());
    }

    स्थिर स्वतः& preTreeps = getSenTreesUsedBy({&logicRegions.m_pre});
    स्थिर स्वतः& senTreeps = getSenTreesUsedBy({&logicRegions.m_act,  //
                                               &logicRegions.m_nba,  //
                                               &logicRegions.m_obs,  //
                                               &logicRegions.m_react,  //
                                               &timingKit.m_lbs});
    स्थिर TriggerKit trigKit
        = TriggerKit::create(netlistp, staticp, sen_व्यंजक_Builder, preTreeps, senTreeps, "act",
                             extraTriggers, false, v3Global.usesTiming());

    // Add post updates from the timing kit
    अगर (timingKit.m_postUpdates) trigKit.compBasep()->addStmtsp(timingKit.m_postUpdates);

    अगर (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpi_Export_Trigger_सूचकांक);
    }
    addVirtIfaceTriggerAssignments(netlistp, staticp, virtIfaceTriggers, first_Vif_Trigger_सूचकांक,
                                   trigKit);
    अगर (v3Global.opt.stats()) V3Stats::statsStage("sched-create-triggers");

    // Note: Experiments so far show that running the Act (or Ico) regions on
    // multiple threads is always a net loss, so only use multi-threading for
    // NBA for now. This can be revised if evidence is available that it would
    // be beneficial

    // Step 11: Create the 'act' region evaluation function

    // Add the trigger indices of the input logic
    addTriggerIndices(logicRegions.m_pre, trigKit, true);
    addTriggerIndices(logicRegions.m_act, trigKit, false);
    addTriggerIndices(logicReplicas.m_act, trigKit, false);
    addTriggerIndices(timingKit.m_lbs, trigKit, false);

    // External triggers of variables. Trigger indices are the same in all regions, so share.
    स्थिर std::map<स्थिर AstVarScope*, std::vector<uint32_t>> timingTriggers
        = timingKit.externalTriggerIndices(trigKit);
    स्थिर स्वतः& vifVscpToTrig = virtIfaceTriggers.makeVscpToTrigMap(trigKit, first_Vif_Trigger_सूचकांक);
    स्थिर V3Order::ExternalTriggersProvider externalTriggers
        = [&](स्थिर AstVarScope* vscp, std::set<uint32_t>& triggers) {
              स्वतः it = timingTriggers.find(vscp);
              अगर (it != timingTriggers.end())
                  triggers.जोड़ो(it->second.begin(), it->second.end());
              अगर (vscp->varp()->isWrittenByDpi()) {
                  triggers.जोड़ो(trigKit.extra_Trigger_सूचकांक(dpi_Export_Trigger_सूचकांक));
              }
              अगर (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                  addTriggeredIface(vscp, vifVscpToTrig, virtIfaceTriggers, triggers);
              }
          };

    AstCFunc* स्थिर actFuncp
        = V3Order::order({&logicRegions.m_pre, &logicRegions.m_act, &logicReplicas.m_act}, trigKit,
                         trigKit.vscp(), cgRefBindings, "act", false, false, externalTriggers);
    util::splitCheck(actFuncp);
    अगर (v3Global.opt.stats()) V3Stats::statsStage("sched-create-act");

    स्थिर EvalKit actKit{trigKit.vscp(), actFuncp};

    // Orders a region's logic and creates the region eval function
    स्थिर स्वतः order = [&](स्थिर std::string& name,
                           स्थिर std::vector<V3Sched::LogicByScope*>& logic) -> EvalKit {
        UINFO(2, "Scheduling " << name << " #logic = " << logic.आकार());
        AstVarScope* स्थिर trigVscp = trigKit.newTrigVec(name);
        // Add the trigger indices of the input logic
        क्रम (LogicByScope* lbs : logic) addTriggerIndices(*lbs, trigKit, false);

        AstCFunc* स्थिर funcp
            = V3Order::order(logic, trigKit, trigVscp, cgRefBindings, name,
                             name == "nba" && v3Global.opt.mtasks(), false, externalTriggers);

        वापस {trigVscp, funcp};
    };

    // Step 12: Create the 'nba' region evaluation function
    स्थिर EvalKit nbaKit = order("nba", {&logicRegions.m_nba, &logicReplicas.m_nba});
    util::splitCheck(nbaKit.m_funcp);
    अगर (v3Global.opt.stats()) V3Stats::statsStage("sched-create-nba");

    // Orders a region's logic and creates the region eval function (only if there is any logic in
    // the region)
    स्थिर स्वतः orderIfNonEmpty
        = [&](स्थिर std::string& name, स्थिर std::vector<LogicByScope*>& logic) -> EvalKit {
        अगर (logic[0]->empty())
            वापस {};  // if region is empty, replica is supposed to be empty as well
        स्थिर स्वतः& kit = order(name, logic);
        अगर (v3Global.opt.stats()) V3Stats::statsStage("sched-create-" + name);
        वापस kit;
    };

    // Step 13: Create the 'obs' region evaluation function
    स्थिर EvalKit obsKit = orderIfNonEmpty("obs", {&logicRegions.m_obs, &logicReplicas.m_obs});

    // Step 14: Create the 'react' region evaluation function
    स्थिर EvalKit reactKit
        = orderIfNonEmpty("react", {&logicRegions.m_react, &logicReplicas.m_react});

    // Step 15: Create the 'postponed' region evaluation function
    createPostponed(netlistp, logicClasses);

    // Step 16: Populate the eval entry point function of each region of a time step
    createEval(netlistp, trigKit, actKit, nbaKit, obsKit, reactKit, timingKit);

    // Step 17: Add neccessary evaluation before awaits
    अगर (AstCCall* स्थिर readyp = timingKit.createReady(trigKit.vscp())) {
        staticp->addStmtsp(readyp->makeStmt());
        beforeTrigVisitor(netlistp, sen_व्यंजक_Builder, trigKit);
    } अन्यथा {
        // beforeTrigVisitor clears Sentree pointers in AstCAwaits (as these sentrees will get
        // deleted later) if there was no need to call it, SenTrees have to be cleaned manually
        netlistp->foreach([](AstCAwait* स्थिर cAwaitp) { cAwaitp->clearSentreep(); });
    }
    अगर (AstVarScope* स्थिर trigAccp = trigKit.vscAccp()) {
        // Copy trigger vector to accumulator at the end of static initialziation so,
        // triggers fired during initialization persist to the first resume.
        स्थिर AstUnpackArrayDType* स्थिर trigAccDTypep
            = VN_AS(trigAccp->dtypep(), UnpackArrayDType);
        UASSERT_OBJ(
            trigAccDTypep->दाँए() == 0, trigAccp,
            "Expected that trigger vector and accumulator start elements enumeration from 0");
        UASSERT_OBJ(trigAccDTypep->बाँए() >= 0, trigAccp,
                    "Expected that trigger vector and accumulator has no negative indexes");
        File_पंक्ति* स्थिर flp = trigAccp->fileline();
        AstVarScope* स्थिर vscp = netlistp->शीर्ष_Scopep()->create_अस्थायी("__Vi", 32);
        AstLoop* स्थिर loopp = नया AstLoop{flp};
        loopp->addStmtsp(
            नया AstAssign{flp,
                          नया AstArraySel{flp, नया AstVarRef{flp, trigAccp, VAccess::WRITE},
                                          नया AstVarRef{flp, vscp, VAccess::READ}},
                          नया AstArraySel{flp, नया AstVarRef{flp, actKit.m_vscp, VAccess::READ},
                                          नया AstVarRef{flp, vscp, VAccess::READ}}});
        loopp->addStmtsp(util::incrementVar(vscp));
        loopp->addStmtsp(नया AstLoopTest{
            flp, loopp,
            नया AstLte{flp, नया AstVarRef{flp, vscp, VAccess::READ},
                       नया AstConst{flp, AstConst::Widthed_मूल्य{}, 32,
                                    static_cast<uint32_t>(trigAccDTypep->बाँए())}}});
        staticp->addStmtsp(loopp);
    }

    // Step 18: Clean up
    netlistp->clearStlFirstIterationp();

    अगर (v3Global.opt.stats()) {
        // A sample() call resolved to the union over its covergroup's constructions reads more
        // than it can, which orders it against more logic than necessary
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per instance",
                         cgRefBindings.अंक_Exact_Calls());
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per type",
                         cgRefBindings.अंक_Union_Calls());
    }

    // Haven't split static initializer yet
    util::splitCheck(staticp);

    // Record eval stats
    netlistp->addEvalStats("sched");

    // Dump
    V3Global::dump_Check_Global_वृक्ष("sched", 0, dump_वृक्ष_Either_Level() >= 3);
}

}  // namespace V3Sched
