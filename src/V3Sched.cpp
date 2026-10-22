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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Sched.h"

#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3EmitCBase.h"
#अन्तर्भाव "V3EmitV.h"
#अन्तर्भाव "V3Order.h"
#अन्तर्भाव "V3SenExprBuilder.h"
#अन्तर्भाव "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

नामाकाश V3Sched {

नामाकाश {

//============================================================================
// Utility functions

std::vector<अचर Ast_Sen_रूख*> getSenTreesUsedBy(अचर std::vector<अचर LogicByScope*>& lbsps) {
    अचर VNUser1InUse user1InUse;
    std::vector<अचर Ast_Sen_रूख*> result;
    लागि (अचर LogicByScope* अचर lbsp : lbsps) {
        लागि (अचर स्वतः& entry : *lbsp) {
            AstActive* अचर activep = entry.m_activep;
            Ast_Sen_रूख* अचर senTreep = activep->sentreep();
            यदि (senTreep->user1SetOnce()) जारी;
            यदि (senTreep->hasClocked() || senTreep->hasHybrid()) result.थप_back(senTreep);
        }
    }
    फर्काऊ result;
}

शून्य addTriggerIndices(LogicByScope& lbs, अचर TriggerKit& trigKit, तार्किक pre) {
    लागि (LogicByScope::Entry& entry : lbs) {
        Ast_Sen_रूख* अचर senTreep = entry.m_activep->sentreep();
        यदि (senTreep->hasCombo()) जारी;
        entry.m_triggers = trigKit.triggerIndices(senTreep, pre);
    }
}

// Add the VIF triggers that a given VarScope should be sensitive to.
// Case 2 (non-virtual interface read): sensitive to that specific VarScope's trigger only
// Case 3 (virtual interface read): sensitive to all member triggers of the same interface type
शून्य addTriggeredIface(अचर AstVarScope* vscp, अचर VirtIfaceTriggers::VscpTrigMap& vscpToTrig,
                       अचर VirtIfaceTriggers& virtIfaceTriggers, std::set<uint32_t>& triggers) {
    यदि (vscp->varp()->isVirtIface()) {
        // Virtual interface variable -- sensitive to all member triggers of this interface type
        अचर AstIface* अचर ifacep = VN_AS(vscp->varp()->dtypep(), IfaceRefDType)->ifacep();
        लागि (अचर स्वतः& entry : virtIfaceTriggers.m_triggers) {
            यदि (entry.m_ifacep == ifacep) {
                अचर स्वतः it = vscpToTrig.find(entry.m_vscp);
                यदि (it != vscpToTrig.end()) triggers.घुसाउ(it->second);
            }
        }
    } अन्यथा {
        // Non-virtual interface member -- sensitive to this VarScope's trigger only
        अचर स्वतः it = vscpToTrig.find(vscp);
        यदि (it != vscpToTrig.end()) triggers.घुसाउ(it->second);
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
शून्य createEvalRegion(
    AstNetlist* netlistp,  //
    VEval eval,  // The entry point of the current region
    // Index of the region's 'first iteration' extra trigger, if it has one, otherwise ignored
    uint32_t firstIterTrigger,
    अचर TriggerKit& trigKit,  // The trigger kit
    AstVarScope* trigp,  // The trigger vector - may be nullptr if no triggers or using 'condp'
    Ast_गाँठो_अभिव्यक्ति* condp,  // Explicit condition that must be true to run 'phaseWorkp'
    Ast_गाँठो_Stmt* phasePrepp,  // Prep statements run before checking triggers
    Ast_गाँठो_Stmt* phaseWorkp,  // The work to do if anything triggered
    // Extra statements to run after the work, even if no triggers fired. This function is
    // passed a variable, which must be set to true if we must continue and loop again,
    // and must be unmodified otherwise.
    std::function<Ast_गाँठो_Stmt*(AstVarScope*)> phaseExtra = [](AstVarScope*) { फर्काऊ nullptr; }  //
) {
    UASSERT(!trigp || !condp, "Cannot use both 'trigp' and 'condp' in 'createEvalRegion'");
    UASSERT(!eval.firstIteration() || trigp,
            "Region without triggers cannot need a first iteration flag");

    // All work is under a trigger or condition, so with neither the region has
    // nothing to evaluate, and what we create below reduces to a no-op function.

    अचर std::string tag = eval.tag();
    Ast_शीर्ष_Scope* अचर शीर्ष_Scopep = netlistp->शीर्ष_Scopep();
    File_पङ्क्ति* अचर flp = netlistp->fileline();

    // Populate the trigger dump entry point function
    यदि (trigp) {
        netlistp->dumpTriggersFuncp(eval)->addStmtsp(trigKit.newDumpCall(trigp, tag, असत्य));
    }

    AstCFunc* अचर funcp = netlistp->evalFuncp(eval);

    // A flag is passed from the run-time eval loop if this is the first iteration of the
    // current loop
    यदि (eval.firstIteration()) {
        AstVarScope* अचर firstIterArgp = util::newArgument(funcp, netlistp->findBitDType(),
                                                             "firstIteration", VDirection::आगत);

        // Set the region's 'first iteration' trigger straight from the argument
        funcp->addStmtsp(trigKit.newExtraTriggerAssignment(firstIterArgp, firstIterTrigger));

        // Only 'stl' also needs a module level flag, for design logic that reads
        // it directly (see V3Timing). Those reads can be anywhere in the design,
        // hence module level. Always created, even if nothing reads it.
        // TODO: get rid of this special case
        यदि (eval == VEval::STL) {
            AstVarScope* अचर firstIterp = netlistp->stlFirstIterationp();
            firstIterp->varp()->noReset(सत्य);
            firstIterp->varp()->isInternal(सत्य);
            funcp->addStmtsp(नूतन AstAssign{flp, नूतन AstVarRef{flp, firstIterp, VAccess::WRITE},
                                           नूतन AstVarRef{flp, firstIterArgp, VAccess::READ}});
        }
    }

    {
        // Add the preparatory statements
        funcp->addStmtsp(phasePrepp);

        // The execute flag
        AstVarScope* अचर executeFlagp = शीर्ष_Scopep->createTemp("__V" + tag + "Execute", 1);
        executeFlagp->varp()->noReset(सत्य);

        // If there is work in this region, execute it if any triggers fired
        यदि (phaseWorkp) {
            Ast_गाँठो_अभिव्यक्ति* अचर lhsp = नूतन AstVarRef{flp, executeFlagp, VAccess::WRITE};
            // If using explicit condition, that directly determines whether to execute,
            // otherwise check if any triggers are fired
            Ast_गाँठो_अभिव्यक्ति* अचर rhsp = condp ? condp : trigKit.newAnySetCall(trigp);
            funcp->addStmtsp(नूतन AstAssign{flp, lhsp, rhsp});

            // Add the work
            AstIf* अचर ifp = नूतन AstIf{flp, नूतन AstVarRef{flp, executeFlagp, VAccess::READ}};
            ifp->addThensp(phaseWorkp);
            funcp->addStmtsp(ifp);
        }

        // Construct the extra statements
        Ast_गाँठो_Stmt* अचर extraWorkp = phaseExtra(executeFlagp);
        यदि (extraWorkp) funcp->addStmtsp(extraWorkp);

        // The function returns true iff it did run work
        Ast_गाँठो_अभिव्यक्ति* अचर retp
            = phaseWorkp || extraWorkp
                  ? static_cast<Ast_गाँठो_अभिव्यक्ति*>(नूतन AstVarRef{flp, executeFlagp, VAccess::READ})
                  : static_cast<Ast_गाँठो_अभिव्यक्ति*>(नूतन AstConst{flp, AstConst::BitFalse{}});
        funcp->addStmtsp(नूतन AstCReturn{flp, retp});
    }
}

//============================================================================
// Collect and classify all logic in the design

LogicClasses gatherLogicClasses(AstNetlist* netlistp) {
    LogicClasses result;

    netlistp->foreach([&](AstScope* scopep) {
        scopep->foreach([&](AstActive* activep) {
            Ast_Sen_रूख* अचर senTreep = activep->sentreep();
            यदि (senTreep->hasStatic()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "static initializer with additional sensitivities");
                result.m_static.add(scopep, activep);
            } अन्यथा यदि (senTreep->hasInitial()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'initial' logic with additional sensitivities");
                result.m_initial.add(scopep, activep);
            } अन्यथा यदि (senTreep->hasFinal()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'final' logic with additional sensitivities");
                result.m_final.add(scopep, activep);
            } अन्यथा यदि (senTreep->hasCombo()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "combinational logic with additional sensitivities");
                यदि (VN_IS(activep->stmtsp(), AlwaysPostponed)) {
                    result.m_postponed.add(scopep, activep);
                } अन्यथा {
                    result.m_comb.add(scopep, activep);
                }
            } अन्यथा {
                UASSERT_OBJ(senTreep->hasClocked(), activep, "What else could it be?");
                यदि (VN_IS(activep->stmtsp(), AlwaysObserved)) {
                    result.m_observed.add(scopep, activep);
                } अन्यथा यदि (VN_IS(activep->stmtsp(), AlwaysReactive)) {
                    result.m_reactive.add(scopep, activep);
                } अन्यथा {
                    result.m_clocked.add(scopep, activep);
                }
            }
        });
    });

    फर्काऊ result;
}

//============================================================================
// Simple ordering in source order

शून्य orderSequentially(AstCFunc* funcp, अचर LogicByScope& lbs) {
    // Create new subfunc for scope
    अचर स्वतः createNewSubFuncp = [&](AstScope* अचर scopep) {
        अचर string subName{funcp->name() + "__" + scopep->nameDotless()};
        AstCFunc* अचर subFuncp = नूतन AstCFunc{scopep->fileline(), subName, scopep};
        subFuncp->isLoose(सत्य);
        subFuncp->isConst(असत्य);
        subFuncp->declPrivate(सत्य);
        subFuncp->slow(funcp->slow());
        scopep->addBlocksp(subFuncp);
        // Call it from the top function
        funcp->addStmtsp(util::callVoidFunc(subFuncp));
        फर्काऊ subFuncp;
    };
    अचर VNUser1InUse user1InUse;  // AstScope -> AstCFunc: the sub-function for the scope
    अचर VNUser2InUse user2InUse;  // AstScope -> int: sub-function counter used for names
    लागि (अचर स्वतः& entry : lbs) {
        AstScope* अचर scopep = entry.m_scopep;
        AstActive* अचर activep = entry.m_activep;
        // Create a sub-function per scope so we can V3Combine them later
        यदि (!scopep->user1p()) scopep->user1p(createNewSubFuncp(scopep));
        // Add statements to sub-function
        लागि (Ast_गाँठो *logicp = activep->stmtsp(), *nextp; logicp; logicp = nextp) {
            स्वतः* subFuncp = VN_AS(scopep->user1p(), CFunc);
            nextp = logicp->nextp();
            यदि (Ast_गाँठो_Procedure* अचर procp = VN_CAST(logicp, गाँठो_Procedure)) {
                यदि (Ast_गाँठो* bodyp = procp->stmtsp()) {
                    bodyp->unlinkFrBackWithNext();
                    // If the process is suspendable, we need a separate function (a coroutine)
                    यदि (procp->isSuspendable()) {
                        funcp->slow(असत्य);
                        subFuncp = createNewSubFuncp(scopep);
                        subFuncp->name(subFuncp->name() + "__Vtiming__"
                                       + cvtToStr(scopep->user2Inc()));
                        subFuncp->rtnType("VlCoroutine");
                        यदि (VN_IS(procp, Always)) {
                            subFuncp->slow(असत्य);
                            File_पङ्क्ति* अचर flp = procp->fileline();
                            Ast_गाँठो_अभिव्यक्ति* अचर condp = नूतन AstCExpr{
                                flp, "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish())", 1};
                            AstLoop* अचर loopp = नूतन AstLoop{flp};
                            loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, condp});
                            loopp->addStmtsp(bodyp);
                            bodyp = loopp;
                        }
                    }
                    subFuncp->addStmtsp(bodyp);
                    यदि (procp->needProcess()) subFuncp->setNeedProcess();
                    // A coroutine holds a single process, so it is complete here
                    यदि (procp->isSuspendable()) util::splitCheck(subFuncp);
                }
            } अन्यथा {
                logicp->unlinkFrBack();
                subFuncp->addStmtsp(logicp);
            }
        }
        यदि (activep->backp()) activep->unlinkFrBack();
        VL_DO_DANGLING(activep->delete_रूख(), activep);
    }
    // Split sub-functions once complete
    लागि (अचर स्वतः& entry : lbs) {
        AstScope* अचर scopep = entry.m_scopep;
        यदि (AstCFunc* अचर subFuncp = VN_AS(scopep->user1p(), CFunc)) {
            util::splitCheck(subFuncp);
            scopep->user1p(nullptr);
        }
    }
}

//============================================================================
// Create simply ordered functions

AstCFunc* createStatic(AstNetlist* netlistp, अचर LogicClasses& logicClasses) {
    AstCFunc* अचर funcp = netlistp->evalFuncp(VEval::STATIC);

    अचर LogicByScope& orig = logicClasses.m_static;
    यदि (orig.size() <= 1) {
        orderSequentially(funcp, orig);
        फर्काऊ funcp;
    }

    // Level-based module sorting can reorder packages so that an importing
    // package runs before the imported one.  Re-sort package entries by source
    // file position to restore compilation order (IEEE 1800-2023 26.3).
    std::vector<माप_प्रकार> indices(orig.size());
    लागि (माप_प्रकार i = 0; i < orig.size(); ++i) indices[i] = i;
    std::stable_sort(indices.begin(), indices.end(), [&](माप_प्रकार a, माप_प्रकार b) {
        अचर Ast_गाँठो_Module* अचर modA = orig[a].m_scopep->modp();
        अचर Ast_गाँठो_Module* अचर modB = orig[b].m_scopep->modp();
        अचर तार्किक isPkgA = VN_IS(modA, Package);
        अचर तार्किक isPkgB = VN_IS(modB, Package);
        यदि (isPkgA != isPkgB) फर्काऊ isPkgA;  // Packages before non-packages
        यदि (isPkgA && isPkgB) {
            // Sort packages by source file position (compilation order)
            फर्काऊ modA->fileline()->operatorCompare(*modB->fileline()) < 0;
        }
        फर्काऊ असत्य;  // Both non-package: preserve original order
    });
    LogicByScope sorted;
    लागि (अचर माप_प्रकार i : indices) sorted.add(orig[i].m_scopep, orig[i].m_activep);

    orderSequentially(funcp, sorted);
    फर्काऊ funcp;  // Not splitting yet as it is not final
}

शून्य createInitial(AstNetlist* netlistp, अचर LogicClasses& logicClasses) {
    AstCFunc* अचर funcp = netlistp->evalFuncp(VEval::INITIAL);
    orderSequentially(funcp, logicClasses.m_initial);
    util::splitCheck(funcp);
}

शून्य createPostponed(AstNetlist* netlistp, अचर LogicClasses& logicClasses) {
    AstCFunc* अचर funcp = netlistp->evalFuncp(VEval::POSTPONED);
    orderSequentially(funcp, logicClasses.m_postponed);
    util::splitCheck(funcp);
}

शून्य createFinal(AstNetlist* netlistp, अचर LogicClasses& logicClasses) {
    AstCFunc* अचर funcp = netlistp->evalFuncp(VEval::FINAL);
    orderSequentially(funcp, logicClasses.m_final);
    util::splitCheck(funcp);
}

//============================================================================
// Helper that creates virtual interface value-change triggers

शून्य addVirtIfaceTriggerAssignments(AstNetlist* netlistp, AstCFunc* initFuncp,
                                    अचर VirtIfaceTriggers& virtIfaceTriggers,
                                    uint32_t firstIndex, अचर TriggerKit& trigKit) {
    uint32_t index = firstIndex;
    लागि (अचर स्वतः& entry : virtIfaceTriggers.m_triggers) {
        trigKit.add_मान_Change_Trigger_Assignment(netlistp, initFuncp, entry.m_vscp, index);
        ++index;
    }
}

// Order the combinational logic to create the 'stl' region
शून्य createSettle(AstNetlist* netlistp, AstCFunc* अचर initFuncp, Sen_अभिव्यक्ति_Builder& sen_अभिव्यक्ति_Bulider,
                  LogicClasses& logicClasses, अचर CovergroupRefBindings& cgRefBindings) {
    // Clone, because ordering is destructive, but we still need them for the other regions
    LogicByScope comb = logicClasses.m_comb.clone();
    LogicByScope hybrid = logicClasses.m_hybrid.clone();

    // We have an extra trigger denoting this is the first iteration of the settle loop
    TriggerKit::ExtraTriggers extraTriggers;
    अचर uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");

    // Gather the relevant sensitivity expressions and create the trigger kit
    अचर स्वतः& senTreeps = getSenTreesUsedBy({&comb, &hybrid});
    अचर TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_अभिव्यक्ति_Bulider, {},
                                                  senTreeps, "stl", extraTriggers, सत्य, असत्य);

    // Add trigger indices (comb has none, so only do the hybrid)
    addTriggerIndices(hybrid, trigKit, असत्य);

    // First trigger is for pure combinational triggers (first iteration)
    अचर uint32_t आगत_Changed = trigKit.extraTriggerIndex(firstIterationTrigger);

    // Create and the body function
    AstCFunc* अचर stlFuncp = V3Order::order(
        {&comb, &hybrid}, trigKit, trigKit.vscp(), cgRefBindings, "stl", असत्य, सत्य,
        [=](अचर AstVarScope*, std::set<uint32_t>& triggers) { triggers.घुसाउ(आगत_Changed); });
    util::splitCheck(stlFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::STL, firstIterationTrigger, trigKit,
        // Use trigger
        trigKit.vscp(), nullptr,
        // Explicit condition
        // Prep statements: Compute the current 'stl' triggers
        [&trigKit] {
            Ast_गाँठो_Stmt* अचर stmtp = trigKit.newCompBaseCall();
            यदि (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), सत्य));
            फर्काऊ stmtp;
        }(),
        // Work statements: Invoke the 'stl' function
        util::callVoidFunc(stlFuncp));
}

//============================================================================
// Order the replicated combinational logic to create the 'ico' region

शून्य createIcoRegion(AstNetlist* netlistp, AstCFunc* अचर initFuncp,
                     Sen_अभिव्यक्ति_Builder& sen_अभिव्यक्ति_Builder, LogicByScope& logic,
                     अचर VirtIfaceTriggers& virtIfaceTriggers,
                     अचर CovergroupRefBindings& cgRefBindings) {
    // SystemC only: Any top level inputs feeding a combinational logic must be marked,
    // so we can make them sc_sensitive
    यदि (v3Global.opt.systemC()) {
        logic.foreachLogic([](Ast_गाँठो* logicp) {
            logicp->foreach([](AstVarRef* refp) {
                यदि (refp->access().isWriteOnly()) फर्काऊ;
                AstVarScope* अचर vscp = refp->varScopep();
                यदि (vscp->scopep()->is_शीर्ष() && vscp->varp()->isNonOutput()) {
                    vscp->varp()->scSensitive(सत्य);
                }
            });
        });
    }

    // We have some extra trigger denoting external conditions
    AstVarScope* अचर dpiExportTriggerVscp = netlistp->dpiExportTriggerp();

    TriggerKit::ExtraTriggers extraTriggers;
    अचर uint32_t firstIterationTrigger = extraTriggers.allocate("first iteration");
    अचर uint32_t dpiExportTriggerIndex = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    अचर uint32_t firstVifTriggerIndex = extraTriggers.size();
    लागि (अचर स्वतः& entry : virtIfaceTriggers.m_triggers) {
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
    std::unordered_map<अचर AstVarScope*, Ast_Sen_रूख*> inp2changedp;
    std::vector<Ast_Sen_रूख*> icoChangeSenTreeps;
    यदि (v3Global.opt.fIcoChangeDetect().isTrue()) {
        File_पङ्क्ति* अचर flp = netlistp->fileline();
        AstScope* अचर scopep = netlistp->शीर्ष_Scopep()->scopep();
        लागि (AstVarScope* vscp = scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            // Only for top level ports, assume outputs don't change externally
            यदि (!vscp->varp()->isPrimaryInish()) जारी;
            // Don't do if written by the design - wouldn't update the change detect 'prev' value
            यदि (vscp->varp()->icoMaybeWritten()) जारी;
            // Don't do if forceable, as we can't see the actual value - this is belt and braces
            यदि (vscp->varp()->isForced()) जारी;
            // Can't handle unpacked arrays (they have special types when primary input)
            यदि (VN_IS(vscp->dtypep()->skipRefp(), UnpackArrayDType)) जारी;
            // Similarly to arrays, can't handle SystemC types
            यदि (vscp->varp()->isSc()) जारी;
            // Create a sen tree triggered when this input changes
            Ast_Sen_रूख*& senTreepr = inp2changedp[vscp];
            UASSERT_OBJ(!senTreepr, vscp, "Duplicate input change detect trigger");
            AstVarRef* अचर refp = नूतन AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* अचर senItemp = नूतन AstSenItem{flp, VEdgeType::ET_CHANGED, refp};
            senTreepr = नूतन Ast_Sen_रूख{flp, senItemp};
            icoChangeSenTreeps.थप_back(senTreepr);
        }
    }
    V3Stats::addStat("Scheduling, 'ico' change detect triggers", icoChangeSenTreeps.size());

    // Gather the relevant sensitivity expressions and create the trigger kit
    std::vector<अचर Ast_Sen_रूख*> senTreeps = getSenTreesUsedBy({&logic});
    senTreeps.घुसाउ(senTreeps.end(), icoChangeSenTreeps.begin(), icoChangeSenTreeps.end());
    अचर TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_अभिव्यक्ति_Builder, {},
                                                  senTreeps, "ico", extraTriggers, असत्य, असत्य);
    std::ignore = sen_अभिव्यक्ति_Builder.getAndClearResults();

    यदि (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, initFuncp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);

    // Add trigger indices
    addTriggerIndices(logic, trigKit, असत्य);

    // The 'first iteration' trigger for top level inputs
    अचर uint32_t firstIterTrigger = trigKit.extraTriggerIndex(firstIterationTrigger);

    अचर स्वतः& vifVscpToTrigIco
        = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);

    // Create and Order the body function
    AstCFunc* अचर icoFuncp = V3Order::order(
        {&logic}, trigKit, trigKit.vscp(), cgRefBindings, "ico", असत्य, असत्य,
        [&](अचर AstVarScope* vscp, std::set<uint32_t>& triggers) {
            AstVar* अचर varp = vscp->varp();
            // If it has an explicit change detect trigger, use that,
            // otherwise fall back to using the 'first iteration' trigger
            स्वतः it = inp2changedp.find(vscp);
            यदि (it != inp2changedp.end()) {
                triggers.घुसाउ(trigKit.senItem2TrigIdx(it->second->sensesp()));
            } अन्यथा यदि (varp->isPrimaryInish() || varp->isSigUserRWPublic() || varp->sampled()) {
                triggers.घुसाउ(firstIterTrigger);
            }
            // Add other triggers
            यदि (varp->isWrittenByDpi())
                triggers.घुसाउ(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
            यदि (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
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
            Ast_गाँठो_Stmt* अचर stmtp = trigKit.newCompBaseCall();
            यदि (stmtp) stmtp->addNext(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), सत्य));
            फर्काऊ stmtp;
        }(),
        // Work statements: Invoke the 'ico' function
        util::callVoidFunc(icoFuncp));

    // Release temporary input change detect SenTrees
    लागि (Ast_Sen_रूख* अचर senTreep : icoChangeSenTreeps)
        VL_DO_DANGLING(senTreep->delete_रूख(), senTreep);
    icoChangeSenTreeps.clear();
}

//============================================================================
// EvalKit groups items that have to be passed to createEval() for a given eval region

संरचना EvalKit final {
    // The AstVarScope representing the region's trigger vector
    AstVarScope* अचर m_vscp = nullptr;
    // The AstCFunc that evaluates the region's logic
    AstCFunc* अचर m_funcp = nullptr;
    // Is this kit used/required?
    तार्किक empty() अचर { फर्काऊ !m_funcp; }
};

//============================================================================
// Create the evaluation function of each region of a time step

शून्य createEval(AstNetlist* netlistp,  //
                अचर TriggerKit& trigKit,  //
                अचर EvalKit& actKit,  //
                अचर EvalKit& nbaKit,  //
                अचर EvalKit& obsKit,  //
                अचर EvalKit& reactKit,  //
                TimingKit& timingKit  //
) {
    File_पङ्क्ति* अचर flp = netlistp->fileline();

    // Grab the delay scheduler variable, if any
    AstVarScope* अचर delaySchedVscp = timingKit.getDelayScheduler(netlistp);

    // 'createResume' consumes the contents that 'createReady' needs, so do the right order
    AstCCall* अचर timingReadyp = timingKit.createReady(trigKit.vscp());
    AstCCall* अचर timingResumep = timingKit.createResume(trigKit.vscp());

    // Create the 'act' region
    createEvalRegion(  //
        netlistp, VEval::ACT, 0, trigKit,
        // Use trigger
        actKit.m_vscp, nullptr,
        // Prep statements
        [&]() {
            // Compute the current 'act' triggers - the NBA triggers are the latched value
            Ast_गाँठो_Stmt* stmtsp = trigKit.newCompBaseCall();
            Ast_गाँठो_Stmt* अचर dumpp
                = stmtsp ? trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), सत्य) : nullptr;
            // Mark as ready for triggered awaits
            यदि (timingReadyp) stmtsp = Ast_गाँठो::addNext(stmtsp, timingReadyp->makeStmt());
            यदि (AstVarScope* अचर vscAccp = trigKit.vscAccp()) {
                stmtsp = Ast_गाँठो::addNext(stmtsp, trigKit.newOrIntoCall(actKit.m_vscp, vscAccp));
            }
            stmtsp = Ast_गाँठो::addNext(stmtsp, trigKit.newCompExtCall(nbaKit.m_vscp));
            stmtsp = Ast_गाँठो::addNext(stmtsp, dumpp);
            // Latch the 'act' triggers under the 'nba' triggers
            stmtsp = Ast_गाँठो::addNext(stmtsp, trigKit.newOrIntoCall(nbaKit.m_vscp, actKit.m_vscp));
            //
            फर्काऊ stmtsp;
        }(),
        // Work statements
        [&]() {
            Ast_गाँठो_Stmt* workp = nullptr;
            यदि (AstVarScope* अचर actAccp = trigKit.vscAccp()) {
                AstCMethodHard* अचर cCallp = नूतन AstCMethodHard{
                    flp, नूतन AstVarRef{flp, actAccp, VAccess::WRITE}, VCMethod::UNPACKED_FILL,
                    नूतन AstConst{flp, AstConst::Unsized64{}, 0}};
                cCallp->dtypeSetVoid();
                workp = Ast_गाँठो::addNext(workp, cCallp->makeStmt());
            }
            // Resume triggered timing schedulers
            यदि (timingResumep) workp = Ast_गाँठो::addNext(workp, timingResumep->makeStmt());
            // Invoke the 'act' function
            workp = Ast_गाँठो::addNext(workp, util::callVoidFunc(actKit.m_funcp));
            //
            फर्काऊ workp;
        }());

    // Create the 'inact' region
    createEvalRegion(  //
        netlistp, VEval::INACT, 0, trigKit,
        // Use explicit condition
        nullptr,
        [&]() -> Ast_गाँठो_अभिव्यक्ति* {
            यदि (!delaySchedVscp) फर्काऊ nullptr;
            // Run if any zero delays are pending
            Ast_गाँठो_अभिव्यक्ति* अचर callp
                = नूतन AstCMethodHard{flp, नूतन AstVarRef{flp, delaySchedVscp, VAccess::READ},
                                     VCMethod::SCHED_AWAITING_ZERO_DELAY};
            callp->dtypeSetBit();
            फर्काऊ callp;
        }(),
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_गाँठो_Stmt* {
            यदि (!delaySchedVscp) {
                // Nothing to do if there are no delays at all in the design
                फर्काऊ nullptr;
            } अन्यथा यदि (v3Global.usesZeroDelay()) {
                // Resume processes watiting for #0 delay
                AstCMethodHard* अचर callp = नूतन AstCMethodHard{
                    flp, नूतन AstVarRef{flp, delaySchedVscp, VAccess::READWRITE},
                    VCMethod::SCHED_RESUME_ZERO_DELAY};
                callp->dtypeSetVoid();
                फर्काऊ callp->makeStmt();
            } अन्यथा {
                // Assumption was that the design doesn't use #0 delays.
                // Die at run-time if it does.
                AstCStmt* अचर stmtp = नूतन AstCStmt{flp};
                अचर File_पङ्क्ति* अचर locp = netlistp->शीर्ष_Modulep()->fileline();
                अचर std::string& file = VIdProtect::protect(locp->filename());
                अचर std::string& पङ्क्ति = std::to_string(locp->lineno());
                stmtp->add("VL_FATAL_MT(\"" + V3OutFormatter::quoteNameControls(file) + "\", "
                           + पङ्क्ति
                           + ", \"\", \"ZERODLY: Design Verilated with '--no-sched-zero-delay', "
                           + "but #0 delay executed at runtime\");");
                फर्काऊ stmtp;
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
            Ast_गाँठो_Stmt* workp = nullptr;
            // Latch the 'nba' trigger flags under the following region's trigger flags
            यदि (!obsKit.empty()) {
                workp = trigKit.newOrIntoCall(obsKit.m_vscp, nbaKit.m_vscp);
            } अन्यथा यदि (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, nbaKit.m_vscp);
            }
            // Invoke the 'nba' function
            workp = Ast_गाँठो::addNext(workp, util::callVoidFunc(nbaKit.m_funcp));
            // Clear the 'nba' triggers
            workp = Ast_गाँठो::addNext(workp, trigKit.newClearCall(nbaKit.m_vscp));
            //
            फर्काऊ workp;
        }(),
        // Extra work (not conditional on having had a fired trigger)
        [&](AstVarScope* continuep) -> Ast_गाँठो_Stmt* {
            // Check if any dynamic NBAs are pending, if there are any in the design
            यदि (!netlistp->nbaEventp()) फर्काऊ nullptr;
            AstVarScope* अचर nbaEventp = netlistp->nbaEventp();
            AstVarScope* अचर nbaEventTriggerp = netlistp->nbaEventTriggerp();
            UASSERT(nbaEventTriggerp, "NBA event trigger var should exist");
            netlistp->nbaEventp(nullptr);
            netlistp->nbaEventTriggerp(nullptr);

            // If a dynamic NBA is pending, clear the pending flag and fire the ready event
            AstIf* अचर ifp = नूतन AstIf{flp, नूतन AstVarRef{flp, nbaEventTriggerp, VAccess::READ}};
            ifp->addThensp(util::setVar(continuep, 1));
            ifp->addThensp(util::setVar(nbaEventTriggerp, 0));
            AstCMethodHard* अचर firep = नूतन AstCMethodHard{
                flp, नूतन AstVarRef{flp, nbaEventp, VAccess::WRITE}, VCMethod::EVENT_FIRE};
            firep->dtypeSetVoid();
            ifp->addThensp(firep->makeStmt());
            फर्काऊ ifp;
        });

    // Create the 'obs' region
    createEvalRegion(  //
        netlistp, VEval::OBS, 0, trigKit,
        // Use trigger
        obsKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_गाँठो_Stmt* {
            यदि (obsKit.empty()) फर्काऊ nullptr;
            Ast_गाँठो_Stmt* workp = nullptr;
            // Latch the Observed trigger flags under the Reactive trigger flags
            यदि (!reactKit.empty()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, obsKit.m_vscp);
            }
            // Invoke the 'obs' function
            workp = Ast_गाँठो::addNext(workp, util::callVoidFunc(obsKit.m_funcp));
            // Clear the 'obs' triggers
            workp = Ast_गाँठो::addNext(workp, trigKit.newClearCall(obsKit.m_vscp));
            //
            फर्काऊ workp;
        }());

    // Create the 'react' region
    createEvalRegion(  //
        netlistp, VEval::REACT, 0, trigKit,
        // Use trigger
        reactKit.m_vscp, nullptr,
        // Prep statements
        nullptr,
        // Work statements
        [&]() -> Ast_गाँठो_Stmt* {
            यदि (reactKit.empty()) फर्काऊ nullptr;
            // Invoke the 'react' function
            Ast_गाँठो_Stmt* workp = util::callVoidFunc(reactKit.m_funcp);
            // Clear the 'react' triggers
            workp = Ast_गाँठो::addNext(workp, trigKit.newClearCall(reactKit.m_vscp));
            फर्काऊ workp;
        }());
}

}  // namespace

//============================================================================
// Helper that builds virtual interface trigger sentrees

VirtIfaceTriggers::VscpTrigMap VirtIfaceTriggers::makeVscpToTrigMap(अचर TriggerKit& trigKit,
                                                                    uint32_t firstIndex) अचर {
    VscpTrigMap map;
    uint32_t index = firstIndex;
    लागि (अचर स्वतः& entry : m_triggers) {
        map.emplace(entry.m_vscp, trigKit.extraTriggerIndex(index));
        ++index;
    }
    फर्काऊ map;
}

//============================================================================
// Top level entry-point to scheduling

शून्य schedule(AstNetlist* netlistp) {
    अचर स्वतः addSizeStat = [](अचर string& name, अचर LogicByScope& lbs) {
        uint64_t size = 0;
        lbs.foreachLogic([&](Ast_गाँठो* nodep) { size += nodep->गाँठो_गन्ती(); });
        V3Stats::addStat("Scheduling, " + name, size);
    };

    // Step 2: Prepare external domains for timing and virtual interfaces
    // Create extra triggers for virtual interfaces
    अचर स्वतः& virtIfaceTriggers = makeVirtIfaceTriggers(netlistp);
    // Resolve what covergroup reference formal arguments are bound to, which is visible here
    // but not from V3Order, where the reads through them must be modeled
    अचर CovergroupRefBindings& cgRefBindings = makeCovergroupRefBindings(netlistp);
    // Prepare timing-related logic and external domains
    TimingKit timingKit = prepareTiming(netlistp);

    // Step 3: Gather and classify all logic in the design
    LogicClasses logicClasses = gatherLogicClasses(netlistp);

    यदि (v3Global.opt.stats()) {
        V3Stats::statsStage("sched-gather");
        addSizeStat("size of class: static", logicClasses.m_static);
        addSizeStat("size of class: initial", logicClasses.m_initial);
        addSizeStat("size of class: final", logicClasses.m_final);
    }

    // Step 4: Schedule static, initial and final logic classes in source order
    AstCFunc* अचर staticp = createStatic(netlistp, logicClasses);
    यदि (v3Global.opt.stats()) V3Stats::statsStage("sched-static");

    createInitial(netlistp, logicClasses);
    यदि (v3Global.opt.stats()) V3Stats::statsStage("sched-initial");

    createFinal(netlistp, logicClasses);
    यदि (v3Global.opt.stats()) V3Stats::statsStage("sched-final");

    // Step 5: Break combinational cycles by introducing hybrid logic
    // Note: breakCycles also removes corresponding logic from logicClasses.m_comb;
    logicClasses.m_hybrid = breakCycles(netlistp, logicClasses.m_comb);
    यदि (v3Global.opt.stats()) {
        addSizeStat("size of class: clocked", logicClasses.m_clocked);
        addSizeStat("size of class: combinational", logicClasses.m_comb);
        addSizeStat("size of class: hybrid", logicClasses.m_hybrid);
        V3Stats::statsStage("sched-break-cycles");
    }

    // We pass around a single SenExprBuilder instance, as we only need one set of 'prev' variables
    // for edge/change detection in sensitivity expressions, which this keeps track of.
    Ast_शीर्ष_Scope* अचर शीर्ष_Scopep = netlistp->शीर्ष_Scopep();
    AstScope* अचर scopeTopp = शीर्ष_Scopep->scopep();
    Sen_अभिव्यक्ति_Builder sen_अभिव्यक्ति_Builder{scopeTopp};

    // Step 6: Create 'settle' region that restores the combinational invariant
    createSettle(netlistp, staticp, sen_अभिव्यक्ति_Builder, logicClasses, cgRefBindings);
    यदि (v3Global.opt.stats()) V3Stats::statsStage("sched-settle");

    // Step 7: Partition the clocked and combinational (including hybrid) logic into pre/act/nba.
    // All clocks (signals referenced in an AstSenTree) generated via a blocking assignment
    // (including combinationally generated signals) are computed within the act region.
    LogicRegions logicRegions
        = partition(logicClasses.m_clocked, logicClasses.m_comb, logicClasses.m_hybrid);
    logicRegions.m_obs = logicClasses.m_observed;
    logicRegions.m_react = logicClasses.m_reactive;
    यदि (v3Global.opt.stats()) {
        addSizeStat("size of region: Active Pre", logicRegions.m_pre);
        addSizeStat("size of region: Active", logicRegions.m_act);
        addSizeStat("size of region: NBA", logicRegions.m_nba);
        addSizeStat("size of region: Observed", logicRegions.m_obs);
        addSizeStat("size of region: Reactive", logicRegions.m_react);
        V3Stats::statsStage("sched-partition");
    }

    // Step 8: Replicate combinational logic
    LogicReplicas logicReplicas = replicateLogic(logicRegions);
    यदि (v3Global.opt.stats()) {
        addSizeStat("size of replicated logic: Input", logicReplicas.m_ico);
        addSizeStat("size of replicated logic: Active", logicReplicas.m_act);
        addSizeStat("size of replicated logic: NBA", logicReplicas.m_nba);
        addSizeStat("size of replicated logic: Observed", logicReplicas.m_obs);
        addSizeStat("size of replicated logic: Reactive", logicReplicas.m_react);
        V3Stats::statsStage("sched-replicate");
    }

    // Step 9: Create the input combinational logic
    createIcoRegion(netlistp, staticp, sen_अभिव्यक्ति_Builder, logicReplicas.m_ico, virtIfaceTriggers,
                    cgRefBindings);
    यदि (v3Global.opt.stats()) V3Stats::statsStage("sched-create-ico");

    // Step 10: Create the triggers
    AstVarScope* अचर dpiExportTriggerVscp = netlistp->dpiExportTriggerp();
    netlistp->dpiExportTriggerp(nullptr);  // Finished with this here

    // We may have an extra trigger for variable updated in DPI exports
    TriggerKit::ExtraTriggers extraTriggers;
    अचर uint32_t dpiExportTriggerIndex = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    अचर uint32_t firstVifTriggerIndex = extraTriggers.size();
    लागि (अचर स्वतः& entry : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + entry.m_ifacep->name() + "."
                               + entry.m_memberp->name());
    }

    अचर स्वतः& preTreeps = getSenTreesUsedBy({&logicRegions.m_pre});
    अचर स्वतः& senTreeps = getSenTreesUsedBy({&logicRegions.m_act,  //
                                               &logicRegions.m_nba,  //
                                               &logicRegions.m_obs,  //
                                               &logicRegions.m_react,  //
                                               &timingKit.m_lbs});
    अचर TriggerKit trigKit
        = TriggerKit::create(netlistp, staticp, sen_अभिव्यक्ति_Builder, preTreeps, senTreeps, "act",
                             extraTriggers, असत्य, v3Global.usesTiming());

    // Add post updates from the timing kit
    यदि (timingKit.m_postUpdates) trigKit.compBasep()->addStmtsp(timingKit.m_postUpdates);

    यदि (dpiExportTriggerVscp) {
        trigKit.addExtraTriggerAssignment(dpiExportTriggerVscp, dpiExportTriggerIndex);
    }
    addVirtIfaceTriggerAssignments(netlistp, staticp, virtIfaceTriggers, firstVifTriggerIndex,
                                   trigKit);
    यदि (v3Global.opt.stats()) V3Stats::statsStage("sched-create-triggers");

    // Note: Experiments so far show that running the Act (or Ico) regions on
    // multiple threads is always a net loss, so only use multi-threading for
    // NBA for now. This can be revised if evidence is available that it would
    // be beneficial

    // Step 11: Create the 'act' region evaluation function

    // Add the trigger indices of the input logic
    addTriggerIndices(logicRegions.m_pre, trigKit, सत्य);
    addTriggerIndices(logicRegions.m_act, trigKit, असत्य);
    addTriggerIndices(logicReplicas.m_act, trigKit, असत्य);
    addTriggerIndices(timingKit.m_lbs, trigKit, असत्य);

    // External triggers of variables. Trigger indices are the same in all regions, so share.
    अचर std::map<अचर AstVarScope*, std::vector<uint32_t>> timingTriggers
        = timingKit.externalTriggerIndices(trigKit);
    अचर स्वतः& vifVscpToTrig = virtIfaceTriggers.makeVscpToTrigMap(trigKit, firstVifTriggerIndex);
    अचर V3Order::ExternalTriggersProvider externalTriggers
        = [&](अचर AstVarScope* vscp, std::set<uint32_t>& triggers) {
              स्वतः it = timingTriggers.find(vscp);
              यदि (it != timingTriggers.end())
                  triggers.घुसाउ(it->second.begin(), it->second.end());
              यदि (vscp->varp()->isWrittenByDpi()) {
                  triggers.घुसाउ(trigKit.extraTriggerIndex(dpiExportTriggerIndex));
              }
              यदि (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                  addTriggeredIface(vscp, vifVscpToTrig, virtIfaceTriggers, triggers);
              }
          };

    AstCFunc* अचर actFuncp
        = V3Order::order({&logicRegions.m_pre, &logicRegions.m_act, &logicReplicas.m_act}, trigKit,
                         trigKit.vscp(), cgRefBindings, "act", असत्य, असत्य, externalTriggers);
    util::splitCheck(actFuncp);
    यदि (v3Global.opt.stats()) V3Stats::statsStage("sched-create-act");

    अचर EvalKit actKit{trigKit.vscp(), actFuncp};

    // Orders a region's logic and creates the region eval function
    अचर स्वतः order = [&](अचर std::string& name,
                           अचर std::vector<V3Sched::LogicByScope*>& logic) -> EvalKit {
        UINFO(2, "Scheduling " << name << " #logic = " << logic.size());
        AstVarScope* अचर trigVscp = trigKit.newTrigVec(name);
        // Add the trigger indices of the input logic
        लागि (LogicByScope* lbs : logic) addTriggerIndices(*lbs, trigKit, असत्य);

        AstCFunc* अचर funcp
            = V3Order::order(logic, trigKit, trigVscp, cgRefBindings, name,
                             name == "nba" && v3Global.opt.mtasks(), असत्य, externalTriggers);

        फर्काऊ {trigVscp, funcp};
    };

    // Step 12: Create the 'nba' region evaluation function
    अचर EvalKit nbaKit = order("nba", {&logicRegions.m_nba, &logicReplicas.m_nba});
    util::splitCheck(nbaKit.m_funcp);
    यदि (v3Global.opt.stats()) V3Stats::statsStage("sched-create-nba");

    // Orders a region's logic and creates the region eval function (only if there is any logic in
    // the region)
    अचर स्वतः orderIfNonEmpty
        = [&](अचर std::string& name, अचर std::vector<LogicByScope*>& logic) -> EvalKit {
        यदि (logic[0]->empty())
            फर्काऊ {};  // if region is empty, replica is supposed to be empty as well
        अचर स्वतः& kit = order(name, logic);
        यदि (v3Global.opt.stats()) V3Stats::statsStage("sched-create-" + name);
        फर्काऊ kit;
    };

    // Step 13: Create the 'obs' region evaluation function
    अचर EvalKit obsKit = orderIfNonEmpty("obs", {&logicRegions.m_obs, &logicReplicas.m_obs});

    // Step 14: Create the 'react' region evaluation function
    अचर EvalKit reactKit
        = orderIfNonEmpty("react", {&logicRegions.m_react, &logicReplicas.m_react});

    // Step 15: Create the 'postponed' region evaluation function
    createPostponed(netlistp, logicClasses);

    // Step 16: Populate the eval entry point function of each region of a time step
    createEval(netlistp, trigKit, actKit, nbaKit, obsKit, reactKit, timingKit);

    // Step 17: Add neccessary evaluation before awaits
    यदि (AstCCall* अचर readyp = timingKit.createReady(trigKit.vscp())) {
        staticp->addStmtsp(readyp->makeStmt());
        beforeTrigVisitor(netlistp, sen_अभिव्यक्ति_Builder, trigKit);
    } अन्यथा {
        // beforeTrigVisitor clears Sentree pointers in AstCAwaits (as these sentrees will get
        // deleted later) if there was no need to call it, SenTrees have to be cleaned manually
        netlistp->foreach([](AstCAwait* अचर cAwaitp) { cAwaitp->clearSentreep(); });
    }
    यदि (AstVarScope* अचर trigAccp = trigKit.vscAccp()) {
        // Copy trigger vector to accumulator at the end of static initialziation so,
        // triggers fired during initialization persist to the first resume.
        अचर AstUnpackArrayDType* अचर trigAccDTypep
            = VN_AS(trigAccp->dtypep(), UnpackArrayDType);
        UASSERT_OBJ(
            trigAccDTypep->दायाँ() == 0, trigAccp,
            "Expected that trigger vector and accumulator start elements enumeration from 0");
        UASSERT_OBJ(trigAccDTypep->बायाँ() >= 0, trigAccp,
                    "Expected that trigger vector and accumulator has no negative indexes");
        File_पङ्क्ति* अचर flp = trigAccp->fileline();
        AstVarScope* अचर vscp = netlistp->शीर्ष_Scopep()->createTemp("__Vi", 32);
        AstLoop* अचर loopp = नूतन AstLoop{flp};
        loopp->addStmtsp(
            नूतन AstAssign{flp,
                          नूतन AstArraySel{flp, नूतन AstVarRef{flp, trigAccp, VAccess::WRITE},
                                          नूतन AstVarRef{flp, vscp, VAccess::READ}},
                          नूतन AstArraySel{flp, नूतन AstVarRef{flp, actKit.m_vscp, VAccess::READ},
                                          नूतन AstVarRef{flp, vscp, VAccess::READ}}});
        loopp->addStmtsp(util::incrementVar(vscp));
        loopp->addStmtsp(नूतन AstLoopTest{
            flp, loopp,
            नूतन AstLte{flp, नूतन AstVarRef{flp, vscp, VAccess::READ},
                       नूतन AstConst{flp, AstConst::Widthed_मान{}, 32,
                                    static_cast<uint32_t>(trigAccDTypep->बायाँ())}}});
        staticp->addStmtsp(loopp);
    }

    // Step 18: Clean up
    netlistp->clearStlFirstIterationp();

    यदि (v3Global.opt.stats()) {
        // A sample() call resolved to the union over its covergroup's constructions reads more
        // than it can, which orders it against more logic than necessary
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per instance",
                         cgRefBindings.अङ्क_Exact_Calls());
        V3Stats::addStat("Scheduling, covergroup ref sample calls, per type",
                         cgRefBindings.अङ्क_Union_Calls());
    }

    // Haven't split static initializer yet
    util::splitCheck(staticp);

    // Record eval stats
    netlistp->addEvalStats("sched");

    // Dump
    V3Global::dump_Check_Global_रूख("sched", 0, dump_रूख_Either_Level() >= 3);
}

}  // namespace V3Sched
