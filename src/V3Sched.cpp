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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Sched.h"

#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3EmitCBase.h"
#అన్తర్భావ "V3EmitV.h"
#అన్తర్భావ "V3Order.h"
#అన్తర్భావ "V3SenExprBuilder.h"
#అన్తర్భావ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

నామాకాశ V3Sched {

నామాకాశ {

//============================================================================
// Utility functions

std::సదిశ<స్థిరరాశి Ast_Sen_వృక్షం*> get_Sen_Trees_వాడిన_By(స్థిరరాశి std::సదిశ<స్థిరరాశి LogicByScope*>& lbsps) {
    స్థిరరాశి VNUser1_లోపల_Use user1_లోపల_Use;
    std::సదిశ<స్థిరరాశి Ast_Sen_వృక్షం*> ఫలితం;
    కొరకు (స్థిరరాశి LogicByScope* స్థిరరాశి lbsp : lbsps) {
        కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : *lbsp) {
            Ast_చురుకైన* స్థిరరాశి activep = నమోదు_2.m_activep;
            Ast_Sen_వృక్షం* స్థిరరాశి senTreep = activep->sentreep();
            ఒకవేళ (senTreep->user1_సమితి_Once()) కొనసాగు;
            ఒకవేళ (senTreep->hasClocked() || senTreep->hasHybrid()) ఫలితం.నెట్టు_వెనుక(senTreep);
        }
    }
    తిరిగి ఫలితం;
}

శూన్య కూడు_Trigger_Indices(LogicByScope& lbs, స్థిరరాశి TriggerKit& trigKit, తార్కిక pre) {
    కొరకు (LogicByScope::నమోదు_3& నమోదు_2 : lbs) {
        Ast_Sen_వృక్షం* స్థిరరాశి senTreep = నమోదు_2.m_activep->sentreep();
        ఒకవేళ (senTreep->hasCombo()) కొనసాగు;
        నమోదు_2.m_triggers = trigKit.triggerIndices(senTreep, pre);
    }
}

// Add the VIF triggers that a given VarScope should be sensitive to.
// Case 2 (non-virtual interface read): sensitive to that specific VarScope's trigger only
// Case 3 (virtual interface read): sensitive to all member triggers of the same interface type
శూన్య కూడు_Triggered_Iface(స్థిరరాశి AstVarScope* vscp, స్థిరరాశి VirtIfaceTriggers::VscpTrigMap& vscp_గమ్యం_Trig,
                       స్థిరరాశి VirtIfaceTriggers& virtIfaceTriggers, std::సమితి<uint32_t>& triggers) {
    ఒకవేళ (vscp->varp()->isVirtIface()) {
        // Virtual interface variable -- sensitive to all member triggers of this interface type
        స్థిరరాశి AstIface* స్థిరరాశి ifacep = VN_AS(vscp->varp()->dtypep(), IfaceRefDType)->ifacep();
        కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : virtIfaceTriggers.m_triggers) {
            ఒకవేళ (నమోదు_2.m_ifacep == ifacep) {
                స్థిరరాశి స్వయంచాలక it = vscp_గమ్యం_Trig.కనుగొను(నమోదు_2.m_vscp);
                ఒకవేళ (it != vscp_గమ్యం_Trig.చివర()) triggers.చొప్పించు(it->రెండవ);
            }
        }
    } లేకపోతే {
        // Non-virtual interface member -- sensitive to this VarScope's trigger only
        స్థిరరాశి స్వయంచాలక it = vscp_గమ్యం_Trig.కనుగొను(vscp);
        ఒకవేళ (it != vscp_గమ్యం_Trig.చివర()) triggers.చొప్పించు(it->రెండవ);
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
శూన్య createEvalRegion(
    AstNetlist* netlistp,  //
    VEval eval,  // The entry point of the current region
    // Index of the region's 'first iteration' extra trigger, if it has one, otherwise ignored
    uint32_t మొదటి_Iter_Trigger,
    స్థిరరాశి TriggerKit& trigKit,  // The trigger kit
    AstVarScope* trigp,  // The trigger vector - may be nullptr if no triggers or using 'condp'
    Ast_కణుపు_సమాసం* condp,  // Explicit condition that must be true to run 'phaseWorkp'
    Ast_కణుపు_Stmt* phasePrepp,  // Prep statements run before checking triggers
    Ast_కణుపు_Stmt* phaseWorkp,  // The work to do if anything triggered
    // Extra statements to run after the work, even if no triggers fired. This function is
    // passed a variable, which must be set to true if we must continue and loop again,
    // and must be unmodified otherwise.
    std::function<Ast_కణుపు_Stmt*(AstVarScope*)> phaseExtra = [](AstVarScope*) { తిరిగి శూన్యసూచిక; }  //
) {
    UASSERT(!trigp || !condp, "Cannot use both 'trigp' and 'condp' in 'createEvalRegion'");
    UASSERT(!eval.మొదటి_Iteration() || trigp,
            "Region without triggers cannot need a first iteration flag");

    // All work is under a trigger or condition, so with neither the region has
    // nothing to evaluate, and what we create below reduces to a no-op function.

    స్థిరరాశి std::string ట్యాగ్ = eval.ట్యాగ్();
    Ast_శిఖరం_Scope* స్థిరరాశి శిఖరం_Scopep = netlistp->శిఖరం_Scopep();
    File_పంక్తి* స్థిరరాశి flp = netlistp->fileline();

    // Populate the trigger dump entry point function
    ఒకవేళ (trigp) {
        netlistp->dumpTriggersFuncp(eval)->కూడు_Stmtsp(trigKit.newDumpCall(trigp, ట్యాగ్, అబద్ధం));
    }

    AstCFunc* స్థిరరాశి funcp = netlistp->evalFuncp(eval);

    // A flag is passed from the run-time eval loop if this is the first iteration of the
    // current loop
    ఒకవేళ (eval.మొదటి_Iteration()) {
        AstVarScope* స్థిరరాశి మొదటి_Iter_Argp = util::new_వాదం(funcp, netlistp->కనుగొను_బిట్టు_DType(),
                                                             "firstIteration", VDirection::ఆదానం);

        // Set the region's 'first iteration' trigger straight from the argument
        funcp->కూడు_Stmtsp(trigKit.newExtraTriggerAssignment(మొదటి_Iter_Argp, మొదటి_Iter_Trigger));

        // Only 'stl' also needs a module level flag, for design logic that reads
        // it directly (see V3Timing). Those reads can be anywhere in the design,
        // hence module level. Always created, even if nothing reads it.
        // TODO: get rid of this special case
        ఒకవేళ (eval == VEval::STL) {
            AstVarScope* స్థిరరాశి మొదటి_Iterp = netlistp->stl_మొదటి_Iterationp();
            మొదటి_Iterp->varp()->noReset(నిజం);
            మొదటి_Iterp->varp()->is_అంతర్గత(నిజం);
            funcp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, మొదటి_Iterp, VAccess::WRITE},
                                           కొత్త AstVarRef{flp, మొదటి_Iter_Argp, VAccess::READ}});
        }
    }

    {
        // Add the preparatory statements
        funcp->కూడు_Stmtsp(phasePrepp);

        // The execute flag
        AstVarScope* స్థిరరాశి executeFlagp = శిఖరం_Scopep->create_తాత్కాలిక("__V" + ట్యాగ్ + "Execute", 1);
        executeFlagp->varp()->noReset(నిజం);

        // If there is work in this region, execute it if any triggers fired
        ఒకవేళ (phaseWorkp) {
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = కొత్త AstVarRef{flp, executeFlagp, VAccess::WRITE};
            // If using explicit condition, that directly determines whether to execute,
            // otherwise check if any triggers are fired
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = condp ? condp : trigKit.new_Any_సమితి_Call(trigp);
            funcp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, lhsp, rhsp});

            // Add the work
            AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, కొత్త AstVarRef{flp, executeFlagp, VAccess::READ}};
            ifp->కూడు_Thensp(phaseWorkp);
            funcp->కూడు_Stmtsp(ifp);
        }

        // Construct the extra statements
        Ast_కణుపు_Stmt* స్థిరరాశి extraWorkp = phaseExtra(executeFlagp);
        ఒకవేళ (extraWorkp) funcp->కూడు_Stmtsp(extraWorkp);

        // The function returns true iff it did run work
        Ast_కణుపు_సమాసం* స్థిరరాశి retp
            = phaseWorkp || extraWorkp
                  ? static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstVarRef{flp, executeFlagp, VAccess::READ})
                  : static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstConst{flp, AstConst::బిట్టు_False{}});
        funcp->కూడు_Stmtsp(కొత్త AstCReturn{flp, retp});
    }
}

//============================================================================
// Collect and classify all logic in the design

LogicClasses gatherLogicClasses(AstNetlist* netlistp) {
    LogicClasses ఫలితం;

    netlistp->foreach([&](AstScope* scopep) {
        scopep->foreach([&](Ast_చురుకైన* activep) {
            Ast_Sen_వృక్షం* స్థిరరాశి senTreep = activep->sentreep();
            ఒకవేళ (senTreep->hasStatic()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "static initializer with additional sensitivities");
                ఫలితం.m_static.కూడు(scopep, activep);
            } లేకపోతే ఒకవేళ (senTreep->hasInitial()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'initial' logic with additional sensitivities");
                ఫలితం.m_initial.కూడు(scopep, activep);
            } లేకపోతే ఒకవేళ (senTreep->hasFinal()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'final' logic with additional sensitivities");
                ఫలితం.m_final.కూడు(scopep, activep);
            } లేకపోతే ఒకవేళ (senTreep->hasCombo()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "combinational logic with additional sensitivities");
                ఒకవేళ (VN_IS(activep->stmtsp(), AlwaysPostponed)) {
                    ఫలితం.m_postponed.కూడు(scopep, activep);
                } లేకపోతే {
                    ఫలితం.m_comb.కూడు(scopep, activep);
                }
            } లేకపోతే {
                UASSERT_OBJ(senTreep->hasClocked(), activep, "What else could it be?");
                ఒకవేళ (VN_IS(activep->stmtsp(), AlwaysObserved)) {
                    ఫలితం.m_observed.కూడు(scopep, activep);
                } లేకపోతే ఒకవేళ (VN_IS(activep->stmtsp(), AlwaysReactive)) {
                    ఫలితం.m_reactive.కూడు(scopep, activep);
                } లేకపోతే {
                    ఫలితం.m_clocked.కూడు(scopep, activep);
                }
            }
        });
    });

    తిరిగి ఫలితం;
}

//============================================================================
// Simple ordering in source order

శూన్య క్రమం_Sequentially(AstCFunc* funcp, స్థిరరాశి LogicByScope& lbs) {
    // Create new subfunc for scope
    స్థిరరాశి స్వయంచాలక createNewSubFuncp = [&](AstScope* స్థిరరాశి scopep) {
        స్థిరరాశి string subName{funcp->name() + "__" + scopep->nameDotless()};
        AstCFunc* స్థిరరాశి subFuncp = కొత్త AstCFunc{scopep->fileline(), subName, scopep};
        subFuncp->isLoose(నిజం);
        subFuncp->isConst(అబద్ధం);
        subFuncp->decl_వ్యక్తిగత(నిజం);
        subFuncp->slow(funcp->slow());
        scopep->కూడు_Blocksp(subFuncp);
        // Call it from the top function
        funcp->కూడు_Stmtsp(util::callVoidFunc(subFuncp));
        తిరిగి subFuncp;
    };
    స్థిరరాశి VNUser1_లోపల_Use user1_లోపల_Use;  // AstScope -> AstCFunc: the sub-function for the scope
    స్థిరరాశి VNUser2_లోపల_Use user2_లోపల_Use;  // AstScope -> int: sub-function counter used for names
    కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : lbs) {
        AstScope* స్థిరరాశి scopep = నమోదు_2.m_scopep;
        Ast_చురుకైన* స్థిరరాశి activep = నమోదు_2.m_activep;
        // Create a sub-function per scope so we can V3Combine them later
        ఒకవేళ (!scopep->user1p()) scopep->user1p(createNewSubFuncp(scopep));
        // Add statements to sub-function
        కొరకు (Ast_కణుపు *logicp = activep->stmtsp(), *nextp; logicp; logicp = nextp) {
            స్వయంచాలక* subFuncp = VN_AS(scopep->user1p(), CFunc);
            nextp = logicp->nextp();
            ఒకవేళ (Ast_కణుపు_Procedure* స్థిరరాశి procp = VN_CAST(logicp, కణుపు_Procedure)) {
                ఒకవేళ (Ast_కణుపు* bodyp = procp->stmtsp()) {
                    bodyp->unlink_Fr_వెనుక_With_తదుపరి();
                    // If the process is suspendable, we need a separate function (a coroutine)
                    ఒకవేళ (procp->isSuspendable()) {
                        funcp->slow(అబద్ధం);
                        subFuncp = createNewSubFuncp(scopep);
                        subFuncp->name(subFuncp->name() + "__Vtiming__"
                                       + cvt_గమ్యం_Str(scopep->user2Inc()));
                        subFuncp->rtnType("VlCoroutine");
                        ఒకవేళ (VN_IS(procp, Always)) {
                            subFuncp->slow(అబద్ధం);
                            File_పంక్తి* స్థిరరాశి flp = procp->fileline();
                            Ast_కణుపు_సమాసం* స్థిరరాశి condp = కొత్త AstCExpr{
                                flp, "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish())", 1};
                            AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{flp};
                            loopp->కూడు_Stmtsp(కొత్త AstLoopTest{flp, loopp, condp});
                            loopp->కూడు_Stmtsp(bodyp);
                            bodyp = loopp;
                        }
                    }
                    subFuncp->కూడు_Stmtsp(bodyp);
                    ఒకవేళ (procp->needProcess()) subFuncp->సమితి_Need_Process();
                    // A coroutine holds a single process, so it is complete here
                    ఒకవేళ (procp->isSuspendable()) util::విభజించు_Check(subFuncp);
                }
            } లేకపోతే {
                logicp->unlink_Fr_వెనుక();
                subFuncp->కూడు_Stmtsp(logicp);
            }
        }
        ఒకవేళ (activep->backp()) activep->unlink_Fr_వెనుక();
        VL_DO_DANGLING(activep->delete_వృక్షం(), activep);
    }
    // Split sub-functions once complete
    కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : lbs) {
        AstScope* స్థిరరాశి scopep = నమోదు_2.m_scopep;
        ఒకవేళ (AstCFunc* స్థిరరాశి subFuncp = VN_AS(scopep->user1p(), CFunc)) {
            util::విభజించు_Check(subFuncp);
            scopep->user1p(శూన్యసూచిక);
        }
    }
}

//============================================================================
// Create simply ordered functions

AstCFunc* createStatic(AstNetlist* netlistp, స్థిరరాశి LogicClasses& logicClasses) {
    AstCFunc* స్థిరరాశి funcp = netlistp->evalFuncp(VEval::STATIC);

    స్థిరరాశి LogicByScope& orig = logicClasses.m_static;
    ఒకవేళ (orig.కొలత() <= 1) {
        క్రమం_Sequentially(funcp, orig);
        తిరిగి funcp;
    }

    // Level-based module sorting can reorder packages so that an importing
    // package runs before the imported one.  Re-sort package entries by source
    // file position to restore compilation order (IEEE 1800-2023 26.3).
    std::సదిశ<కొలత_t> indices(orig.కొలత());
    కొరకు (కొలత_t i = 0; i < orig.కొలత(); ++i) indices[i] = i;
    std::stable_క్రమీకరించు(indices.begin(), indices.చివర(), [&](కొలత_t a, కొలత_t b) {
        స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి modA = orig[a].m_scopep->modp();
        స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి modB = orig[b].m_scopep->modp();
        స్థిరరాశి తార్కిక isPkgA = VN_IS(modA, Package);
        స్థిరరాశి తార్కిక isPkgB = VN_IS(modB, Package);
        ఒకవేళ (isPkgA != isPkgB) తిరిగి isPkgA;  // Packages before non-packages
        ఒకవేళ (isPkgA && isPkgB) {
            // Sort packages by source file position (compilation order)
            తిరిగి modA->fileline()->operatorCompare(*modB->fileline()) < 0;
        }
        తిరిగి అబద్ధం;  // Both non-package: preserve original order
    });
    LogicByScope sorted;
    కొరకు (స్థిరరాశి కొలత_t i : indices) sorted.కూడు(orig[i].m_scopep, orig[i].m_activep);

    క్రమం_Sequentially(funcp, sorted);
    తిరిగి funcp;  // Not splitting yet as it is not final
}

శూన్య createInitial(AstNetlist* netlistp, స్థిరరాశి LogicClasses& logicClasses) {
    AstCFunc* స్థిరరాశి funcp = netlistp->evalFuncp(VEval::INITIAL);
    క్రమం_Sequentially(funcp, logicClasses.m_initial);
    util::విభజించు_Check(funcp);
}

శూన్య createPostponed(AstNetlist* netlistp, స్థిరరాశి LogicClasses& logicClasses) {
    AstCFunc* స్థిరరాశి funcp = netlistp->evalFuncp(VEval::POSTPONED);
    క్రమం_Sequentially(funcp, logicClasses.m_postponed);
    util::విభజించు_Check(funcp);
}

శూన్య createFinal(AstNetlist* netlistp, స్థిరరాశి LogicClasses& logicClasses) {
    AstCFunc* స్థిరరాశి funcp = netlistp->evalFuncp(VEval::FINAL);
    క్రమం_Sequentially(funcp, logicClasses.m_final);
    util::విభజించు_Check(funcp);
}

//============================================================================
// Helper that creates virtual interface value-change triggers

శూన్య కూడు_Virt_Iface_Trigger_Assignments(AstNetlist* netlistp, AstCFunc* initFuncp,
                                    స్థిరరాశి VirtIfaceTriggers& virtIfaceTriggers,
                                    uint32_t మొదటి_సూచీ, స్థిరరాశి TriggerKit& trigKit) {
    uint32_t సూచీ = మొదటి_సూచీ;
    కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : virtIfaceTriggers.m_triggers) {
        trigKit.కూడు_విలువ_Change_Trigger_Assignment(netlistp, initFuncp, నమోదు_2.m_vscp, సూచీ);
        ++సూచీ;
    }
}

// Order the combinational logic to create the 'stl' region
శూన్య createSettle(AstNetlist* netlistp, AstCFunc* స్థిరరాశి initFuncp, Sen_సమాసం_Builder& sen_సమాసం_Bulider,
                  LogicClasses& logicClasses, స్థిరరాశి CovergroupRefBindings& cgRefBindings) {
    // Clone, because ordering is destructive, but we still need them for the other regions
    LogicByScope comb = logicClasses.m_comb.clone();
    LogicByScope hybrid = logicClasses.m_hybrid.clone();

    // We have an extra trigger denoting this is the first iteration of the settle loop
    TriggerKit::ExtraTriggers extraTriggers;
    స్థిరరాశి uint32_t మొదటి_Iteration_Trigger = extraTriggers.allocate("first iteration");

    // Gather the relevant sensitivity expressions and create the trigger kit
    స్థిరరాశి స్వయంచాలక& senTreeps = get_Sen_Trees_వాడిన_By({&comb, &hybrid});
    స్థిరరాశి TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_సమాసం_Bulider, {},
                                                  senTreeps, "stl", extraTriggers, నిజం, అబద్ధం);

    // Add trigger indices (comb has none, so only do the hybrid)
    కూడు_Trigger_Indices(hybrid, trigKit, అబద్ధం);

    // First trigger is for pure combinational triggers (first iteration)
    స్థిరరాశి uint32_t ఆదానం_మారింది = trigKit.extra_Trigger_సూచీ(మొదటి_Iteration_Trigger);

    // Create and the body function
    AstCFunc* స్థిరరాశి stlFuncp = V3_క్రమం::క్రమం(
        {&comb, &hybrid}, trigKit, trigKit.vscp(), cgRefBindings, "stl", అబద్ధం, నిజం,
        [=](స్థిరరాశి AstVarScope*, std::సమితి<uint32_t>& triggers) { triggers.చొప్పించు(ఆదానం_మారింది); });
    util::విభజించు_Check(stlFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::STL, మొదటి_Iteration_Trigger, trigKit,
        // Use trigger
        trigKit.vscp(), శూన్యసూచిక,
        // Explicit condition
        // Prep statements: Compute the current 'stl' triggers
        [&trigKit] {
            Ast_కణుపు_Stmt* స్థిరరాశి stmtp = trigKit.new_Comp_పునాది_Call();
            ఒకవేళ (stmtp) stmtp->కూడు_తదుపరి(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), నిజం));
            తిరిగి stmtp;
        }(),
        // Work statements: Invoke the 'stl' function
        util::callVoidFunc(stlFuncp));
}

//============================================================================
// Order the replicated combinational logic to create the 'ico' region

శూన్య createIcoRegion(AstNetlist* netlistp, AstCFunc* స్థిరరాశి initFuncp,
                     Sen_సమాసం_Builder& sen_సమాసం_Builder, LogicByScope& logic,
                     స్థిరరాశి VirtIfaceTriggers& virtIfaceTriggers,
                     స్థిరరాశి CovergroupRefBindings& cgRefBindings) {
    // SystemC only: Any top level inputs feeding a combinational logic must be marked,
    // so we can make them sc_sensitive
    ఒకవేళ (v3Global.opt.systemC()) {
        logic.foreachLogic([](Ast_కణుపు* logicp) {
            logicp->foreach([](AstVarRef* refp) {
                ఒకవేళ (refp->access().isWriteOnly()) తిరిగి;
                AstVarScope* స్థిరరాశి vscp = refp->varScopep();
                ఒకవేళ (vscp->scopep()->is_శిఖరం() && vscp->varp()->isNonOutput()) {
                    vscp->varp()->scSensitive(నిజం);
                }
            });
        });
    }

    // We have some extra trigger denoting external conditions
    AstVarScope* స్థిరరాశి dpiExportTriggerVscp = netlistp->dpiExportTriggerp();

    TriggerKit::ExtraTriggers extraTriggers;
    స్థిరరాశి uint32_t మొదటి_Iteration_Trigger = extraTriggers.allocate("first iteration");
    స్థిరరాశి uint32_t dpi_Export_Trigger_సూచీ = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    స్థిరరాశి uint32_t మొదటి_Vif_Trigger_సూచీ = extraTriggers.కొలత();
    కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + నమోదు_2.m_ifacep->name() + "."
                               + నమోదు_2.m_memberp->name());
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
    std::unordered_map<స్థిరరాశి AstVarScope*, Ast_Sen_వృక్షం*> inp2changedp;
    std::సదిశ<Ast_Sen_వృక్షం*> icoChangeSenTreeps;
    ఒకవేళ (v3Global.opt.fIcoChangeDetect().isTrue()) {
        File_పంక్తి* స్థిరరాశి flp = netlistp->fileline();
        AstScope* స్థిరరాశి scopep = netlistp->శిఖరం_Scopep()->scopep();
        కొరకు (AstVarScope* vscp = scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            // Only for top level ports, assume outputs don't change externally
            ఒకవేళ (!vscp->varp()->isPrimaryInish()) కొనసాగు;
            // Don't do if written by the design - wouldn't update the change detect 'prev' value
            ఒకవేళ (vscp->varp()->icoMaybeWritten()) కొనసాగు;
            // Don't do if forceable, as we can't see the actual value - this is belt and braces
            ఒకవేళ (vscp->varp()->isForced()) కొనసాగు;
            // Can't handle unpacked arrays (they have special types when primary input)
            ఒకవేళ (VN_IS(vscp->dtypep()->skipRefp(), UnpackArrayDType)) కొనసాగు;
            // Similarly to arrays, can't handle SystemC types
            ఒకవేళ (vscp->varp()->isSc()) కొనసాగు;
            // Create a sen tree triggered when this input changes
            Ast_Sen_వృక్షం*& senTreepr = inp2changedp[vscp];
            UASSERT_OBJ(!senTreepr, vscp, "Duplicate input change detect trigger");
            AstVarRef* స్థిరరాశి refp = కొత్త AstVarRef{flp, vscp, VAccess::READ};
            Ast_Sen_అంశం* స్థిరరాశి senItemp = కొత్త Ast_Sen_అంశం{flp, VEdgeType::ET_మారింది, refp};
            senTreepr = కొత్త Ast_Sen_వృక్షం{flp, senItemp};
            icoChangeSenTreeps.నెట్టు_వెనుక(senTreepr);
        }
    }
    V3Stats::కూడు_Stat("Scheduling, 'ico' change detect triggers", icoChangeSenTreeps.కొలత());

    // Gather the relevant sensitivity expressions and create the trigger kit
    std::సదిశ<స్థిరరాశి Ast_Sen_వృక్షం*> senTreeps = get_Sen_Trees_వాడిన_By({&logic});
    senTreeps.చొప్పించు(senTreeps.చివర(), icoChangeSenTreeps.begin(), icoChangeSenTreeps.చివర());
    స్థిరరాశి TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_సమాసం_Builder, {},
                                                  senTreeps, "ico", extraTriggers, అబద్ధం, అబద్ధం);
    std::ignore = sen_సమాసం_Builder.getAndClearResults();

    ఒకవేళ (dpiExportTriggerVscp) {
        trigKit.కూడు_Extra_Trigger_Assignment(dpiExportTriggerVscp, dpi_Export_Trigger_సూచీ);
    }
    కూడు_Virt_Iface_Trigger_Assignments(netlistp, initFuncp, virtIfaceTriggers, మొదటి_Vif_Trigger_సూచీ,
                                   trigKit);

    // Add trigger indices
    కూడు_Trigger_Indices(logic, trigKit, అబద్ధం);

    // The 'first iteration' trigger for top level inputs
    స్థిరరాశి uint32_t మొదటి_Iter_Trigger = trigKit.extra_Trigger_సూచీ(మొదటి_Iteration_Trigger);

    స్థిరరాశి స్వయంచాలక& vif_Vscp_గమ్యం_Trig_Ico
        = virtIfaceTriggers.make_Vscp_గమ్యం_Trig_Map(trigKit, మొదటి_Vif_Trigger_సూచీ);

    // Create and Order the body function
    AstCFunc* స్థిరరాశి icoFuncp = V3_క్రమం::క్రమం(
        {&logic}, trigKit, trigKit.vscp(), cgRefBindings, "ico", అబద్ధం, అబద్ధం,
        [&](స్థిరరాశి AstVarScope* vscp, std::సమితి<uint32_t>& triggers) {
            AstVar* స్థిరరాశి varp = vscp->varp();
            // If it has an explicit change detect trigger, use that,
            // otherwise fall back to using the 'first iteration' trigger
            స్వయంచాలక it = inp2changedp.కనుగొను(vscp);
            ఒకవేళ (it != inp2changedp.చివర()) {
                triggers.చొప్పించు(trigKit.senItem2TrigIdx(it->రెండవ->sensesp()));
            } లేకపోతే ఒకవేళ (varp->isPrimaryInish() || varp->isSigUserRWPublic() || varp->sampled()) {
                triggers.చొప్పించు(మొదటి_Iter_Trigger);
            }
            // Add other triggers
            ఒకవేళ (varp->isWrittenByDpi())
                triggers.చొప్పించు(trigKit.extra_Trigger_సూచీ(dpi_Export_Trigger_సూచీ));
            ఒకవేళ (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                కూడు_Triggered_Iface(vscp, vif_Vscp_గమ్యం_Trig_Ico, virtIfaceTriggers, triggers);
            }
        });
    util::విభజించు_Check(icoFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::ICO, మొదటి_Iteration_Trigger, trigKit,
        // Use trigger
        trigKit.vscp(), శూన్యసూచిక,
        // Prep statements: Compute the current 'ico' triggers
        [&trigKit] {
            Ast_కణుపు_Stmt* స్థిరరాశి stmtp = trigKit.new_Comp_పునాది_Call();
            ఒకవేళ (stmtp) stmtp->కూడు_తదుపరి(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), నిజం));
            తిరిగి stmtp;
        }(),
        // Work statements: Invoke the 'ico' function
        util::callVoidFunc(icoFuncp));

    // Release temporary input change detect SenTrees
    కొరకు (Ast_Sen_వృక్షం* స్థిరరాశి senTreep : icoChangeSenTreeps)
        VL_DO_DANGLING(senTreep->delete_వృక్షం(), senTreep);
    icoChangeSenTreeps.clear();
}

//============================================================================
// EvalKit groups items that have to be passed to createEval() for a given eval region

నిర్మాణం EvalKit final {
    // The AstVarScope representing the region's trigger vector
    AstVarScope* స్థిరరాశి m_vscp = శూన్యసూచిక;
    // The AstCFunc that evaluates the region's logic
    AstCFunc* స్థిరరాశి m_funcp = శూన్యసూచిక;
    // Is this kit used/required?
    తార్కిక ఖాళీలు() స్థిరరాశి { తిరిగి !m_funcp; }
};

//============================================================================
// Create the evaluation function of each region of a time step

శూన్య createEval(AstNetlist* netlistp,  //
                స్థిరరాశి TriggerKit& trigKit,  //
                స్థిరరాశి EvalKit& actKit,  //
                స్థిరరాశి EvalKit& nbaKit,  //
                స్థిరరాశి EvalKit& obsKit,  //
                స్థిరరాశి EvalKit& reactKit,  //
                TimingKit& timingKit  //
) {
    File_పంక్తి* స్థిరరాశి flp = netlistp->fileline();

    // Grab the delay scheduler variable, if any
    AstVarScope* స్థిరరాశి delaySchedVscp = timingKit.getDelayScheduler(netlistp);

    // 'createResume' consumes the contents that 'createReady' needs, so do the right order
    AstCCall* స్థిరరాశి timingReadyp = timingKit.createReady(trigKit.vscp());
    AstCCall* స్థిరరాశి timingResumep = timingKit.createResume(trigKit.vscp());

    // Create the 'act' region
    createEvalRegion(  //
        netlistp, VEval::ACT, 0, trigKit,
        // Use trigger
        actKit.m_vscp, శూన్యసూచిక,
        // Prep statements
        [&]() {
            // Compute the current 'act' triggers - the NBA triggers are the latched value
            Ast_కణుపు_Stmt* stmtsp = trigKit.new_Comp_పునాది_Call();
            Ast_కణుపు_Stmt* స్థిరరాశి dumpp
                = stmtsp ? trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), నిజం) : శూన్యసూచిక;
            // Mark as ready for triggered awaits
            ఒకవేళ (timingReadyp) stmtsp = Ast_కణుపు::కూడు_తదుపరి(stmtsp, timingReadyp->makeStmt());
            ఒకవేళ (AstVarScope* స్థిరరాశి vscAccp = trigKit.vscAccp()) {
                stmtsp = Ast_కణుపు::కూడు_తదుపరి(stmtsp, trigKit.newOrIntoCall(actKit.m_vscp, vscAccp));
            }
            stmtsp = Ast_కణుపు::కూడు_తదుపరి(stmtsp, trigKit.newCompExtCall(nbaKit.m_vscp));
            stmtsp = Ast_కణుపు::కూడు_తదుపరి(stmtsp, dumpp);
            // Latch the 'act' triggers under the 'nba' triggers
            stmtsp = Ast_కణుపు::కూడు_తదుపరి(stmtsp, trigKit.newOrIntoCall(nbaKit.m_vscp, actKit.m_vscp));
            //
            తిరిగి stmtsp;
        }(),
        // Work statements
        [&]() {
            Ast_కణుపు_Stmt* workp = శూన్యసూచిక;
            ఒకవేళ (AstVarScope* స్థిరరాశి actAccp = trigKit.vscAccp()) {
                AstCMethodHard* స్థిరరాశి cCallp = కొత్త AstCMethodHard{
                    flp, కొత్త AstVarRef{flp, actAccp, VAccess::WRITE}, VCMethod::UNPACKED_FILL,
                    కొత్త AstConst{flp, AstConst::Unsized64{}, 0}};
                cCallp->dtype_సమితి_Void();
                workp = Ast_కణుపు::కూడు_తదుపరి(workp, cCallp->makeStmt());
            }
            // Resume triggered timing schedulers
            ఒకవేళ (timingResumep) workp = Ast_కణుపు::కూడు_తదుపరి(workp, timingResumep->makeStmt());
            // Invoke the 'act' function
            workp = Ast_కణుపు::కూడు_తదుపరి(workp, util::callVoidFunc(actKit.m_funcp));
            //
            తిరిగి workp;
        }());

    // Create the 'inact' region
    createEvalRegion(  //
        netlistp, VEval::INACT, 0, trigKit,
        // Use explicit condition
        శూన్యసూచిక,
        [&]() -> Ast_కణుపు_సమాసం* {
            ఒకవేళ (!delaySchedVscp) తిరిగి శూన్యసూచిక;
            // Run if any zero delays are pending
            Ast_కణుపు_సమాసం* స్థిరరాశి callp
                = కొత్త AstCMethodHard{flp, కొత్త AstVarRef{flp, delaySchedVscp, VAccess::READ},
                                     VCMethod::SCHED_AWAITING_ZERO_DELAY};
            callp->dtype_సమితి_బిట్టు();
            తిరిగి callp;
        }(),
        // Prep statements
        శూన్యసూచిక,
        // Work statements
        [&]() -> Ast_కణుపు_Stmt* {
            ఒకవేళ (!delaySchedVscp) {
                // Nothing to do if there are no delays at all in the design
                తిరిగి శూన్యసూచిక;
            } లేకపోతే ఒకవేళ (v3Global.ఉపయోగిస్తుంది_Zero_Delay()) {
                // Resume processes watiting for #0 delay
                AstCMethodHard* స్థిరరాశి callp = కొత్త AstCMethodHard{
                    flp, కొత్త AstVarRef{flp, delaySchedVscp, VAccess::READWRITE},
                    VCMethod::SCHED_RESUME_ZERO_DELAY};
                callp->dtype_సమితి_Void();
                తిరిగి callp->makeStmt();
            } లేకపోతే {
                // Assumption was that the design doesn't use #0 delays.
                // Die at run-time if it does.
                AstCStmt* స్థిరరాశి stmtp = కొత్త AstCStmt{flp};
                స్థిరరాశి File_పంక్తి* స్థిరరాశి locp = netlistp->శిఖరం_Modulep()->fileline();
                స్థిరరాశి std::string& file = VIdProtect::protect(locp->filename());
                స్థిరరాశి std::string& పంక్తి = std::గమ్యం_string(locp->lineno());
                stmtp->కూడు("VL_FATAL_MT(\"" + V3_బహిర్గతం_Formatter::quoteNameControls(file) + "\", "
                           + పంక్తి
                           + ", \"\", \"ZERODLY: Design Verilated with '--no-sched-zero-delay', "
                           + "but #0 delay executed at runtime\");");
                తిరిగి stmtp;
            }
        }());

    // Create the 'nba' region
    createEvalRegion(  //
        netlistp, VEval::NBA, 0, trigKit,
        // Use trigger
        nbaKit.m_vscp, శూన్యసూచిక,
        // Prep statements
        శూన్యసూచిక,
        // Work statements
        [&]() {
            Ast_కణుపు_Stmt* workp = శూన్యసూచిక;
            // Latch the 'nba' trigger flags under the following region's trigger flags
            ఒకవేళ (!obsKit.ఖాళీలు()) {
                workp = trigKit.newOrIntoCall(obsKit.m_vscp, nbaKit.m_vscp);
            } లేకపోతే ఒకవేళ (!reactKit.ఖాళీలు()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, nbaKit.m_vscp);
            }
            // Invoke the 'nba' function
            workp = Ast_కణుపు::కూడు_తదుపరి(workp, util::callVoidFunc(nbaKit.m_funcp));
            // Clear the 'nba' triggers
            workp = Ast_కణుపు::కూడు_తదుపరి(workp, trigKit.newClearCall(nbaKit.m_vscp));
            //
            తిరిగి workp;
        }(),
        // Extra work (not conditional on having had a fired trigger)
        [&](AstVarScope* continuep) -> Ast_కణుపు_Stmt* {
            // Check if any dynamic NBAs are pending, if there are any in the design
            ఒకవేళ (!netlistp->nbaEventp()) తిరిగి శూన్యసూచిక;
            AstVarScope* స్థిరరాశి nbaEventp = netlistp->nbaEventp();
            AstVarScope* స్థిరరాశి nbaEventTriggerp = netlistp->nbaEventTriggerp();
            UASSERT(nbaEventTriggerp, "NBA event trigger var should exist");
            netlistp->nbaEventp(శూన్యసూచిక);
            netlistp->nbaEventTriggerp(శూన్యసూచిక);

            // If a dynamic NBA is pending, clear the pending flag and fire the ready event
            AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, కొత్త AstVarRef{flp, nbaEventTriggerp, VAccess::READ}};
            ifp->కూడు_Thensp(util::సమితి_Var(continuep, 1));
            ifp->కూడు_Thensp(util::సమితి_Var(nbaEventTriggerp, 0));
            AstCMethodHard* స్థిరరాశి firep = కొత్త AstCMethodHard{
                flp, కొత్త AstVarRef{flp, nbaEventp, VAccess::WRITE}, VCMethod::EVENT_FIRE};
            firep->dtype_సమితి_Void();
            ifp->కూడు_Thensp(firep->makeStmt());
            తిరిగి ifp;
        });

    // Create the 'obs' region
    createEvalRegion(  //
        netlistp, VEval::OBS, 0, trigKit,
        // Use trigger
        obsKit.m_vscp, శూన్యసూచిక,
        // Prep statements
        శూన్యసూచిక,
        // Work statements
        [&]() -> Ast_కణుపు_Stmt* {
            ఒకవేళ (obsKit.ఖాళీలు()) తిరిగి శూన్యసూచిక;
            Ast_కణుపు_Stmt* workp = శూన్యసూచిక;
            // Latch the Observed trigger flags under the Reactive trigger flags
            ఒకవేళ (!reactKit.ఖాళీలు()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, obsKit.m_vscp);
            }
            // Invoke the 'obs' function
            workp = Ast_కణుపు::కూడు_తదుపరి(workp, util::callVoidFunc(obsKit.m_funcp));
            // Clear the 'obs' triggers
            workp = Ast_కణుపు::కూడు_తదుపరి(workp, trigKit.newClearCall(obsKit.m_vscp));
            //
            తిరిగి workp;
        }());

    // Create the 'react' region
    createEvalRegion(  //
        netlistp, VEval::REACT, 0, trigKit,
        // Use trigger
        reactKit.m_vscp, శూన్యసూచిక,
        // Prep statements
        శూన్యసూచిక,
        // Work statements
        [&]() -> Ast_కణుపు_Stmt* {
            ఒకవేళ (reactKit.ఖాళీలు()) తిరిగి శూన్యసూచిక;
            // Invoke the 'react' function
            Ast_కణుపు_Stmt* workp = util::callVoidFunc(reactKit.m_funcp);
            // Clear the 'react' triggers
            workp = Ast_కణుపు::కూడు_తదుపరి(workp, trigKit.newClearCall(reactKit.m_vscp));
            తిరిగి workp;
        }());
}

}  // namespace

//============================================================================
// Helper that builds virtual interface trigger sentrees

VirtIfaceTriggers::VscpTrigMap VirtIfaceTriggers::make_Vscp_గమ్యం_Trig_Map(స్థిరరాశి TriggerKit& trigKit,
                                                                    uint32_t మొదటి_సూచీ) స్థిరరాశి {
    VscpTrigMap map;
    uint32_t సూచీ = మొదటి_సూచీ;
    కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : m_triggers) {
        map.emplace(నమోదు_2.m_vscp, trigKit.extra_Trigger_సూచీ(సూచీ));
        ++సూచీ;
    }
    తిరిగి map;
}

//============================================================================
// Top level entry-point to scheduling

శూన్య కాలపట్టిక(AstNetlist* netlistp) {
    స్థిరరాశి స్వయంచాలక కూడు_కొలత_Stat = [](స్థిరరాశి string& name, స్థిరరాశి LogicByScope& lbs) {
        uint64_t కొలత = 0;
        lbs.foreachLogic([&](Ast_కణుపు* nodep) { కొలత += nodep->కణుపు_లెక్క(); });
        V3Stats::కూడు_Stat("Scheduling, " + name, కొలత);
    };

    // Step 2: Prepare external domains for timing and virtual interfaces
    // Create extra triggers for virtual interfaces
    స్థిరరాశి స్వయంచాలక& virtIfaceTriggers = makeVirtIfaceTriggers(netlistp);
    // Resolve what covergroup reference formal arguments are bound to, which is visible here
    // but not from V3Order, where the reads through them must be modeled
    స్థిరరాశి CovergroupRefBindings& cgRefBindings = makeCovergroupRefBindings(netlistp);
    // Prepare timing-related logic and external domains
    TimingKit timingKit = prepareTiming(netlistp);

    // Step 3: Gather and classify all logic in the design
    LogicClasses logicClasses = gatherLogicClasses(netlistp);

    ఒకవేళ (v3Global.opt.stats()) {
        V3Stats::statsStage("sched-gather");
        కూడు_కొలత_Stat("size of class: static", logicClasses.m_static);
        కూడు_కొలత_Stat("size of class: initial", logicClasses.m_initial);
        కూడు_కొలత_Stat("size of class: final", logicClasses.m_final);
    }

    // Step 4: Schedule static, initial and final logic classes in source order
    AstCFunc* స్థిరరాశి staticp = createStatic(netlistp, logicClasses);
    ఒకవేళ (v3Global.opt.stats()) V3Stats::statsStage("sched-static");

    createInitial(netlistp, logicClasses);
    ఒకవేళ (v3Global.opt.stats()) V3Stats::statsStage("sched-initial");

    createFinal(netlistp, logicClasses);
    ఒకవేళ (v3Global.opt.stats()) V3Stats::statsStage("sched-final");

    // Step 5: Break combinational cycles by introducing hybrid logic
    // Note: breakCycles also removes corresponding logic from logicClasses.m_comb;
    logicClasses.m_hybrid = break_చక్రాలు(netlistp, logicClasses.m_comb);
    ఒకవేళ (v3Global.opt.stats()) {
        కూడు_కొలత_Stat("size of class: clocked", logicClasses.m_clocked);
        కూడు_కొలత_Stat("size of class: combinational", logicClasses.m_comb);
        కూడు_కొలత_Stat("size of class: hybrid", logicClasses.m_hybrid);
        V3Stats::statsStage("sched-break-cycles");
    }

    // We pass around a single SenExprBuilder instance, as we only need one set of 'prev' variables
    // for edge/change detection in sensitivity expressions, which this keeps track of.
    Ast_శిఖరం_Scope* స్థిరరాశి శిఖరం_Scopep = netlistp->శిఖరం_Scopep();
    AstScope* స్థిరరాశి scopeTopp = శిఖరం_Scopep->scopep();
    Sen_సమాసం_Builder sen_సమాసం_Builder{scopeTopp};

    // Step 6: Create 'settle' region that restores the combinational invariant
    createSettle(netlistp, staticp, sen_సమాసం_Builder, logicClasses, cgRefBindings);
    ఒకవేళ (v3Global.opt.stats()) V3Stats::statsStage("sched-settle");

    // Step 7: Partition the clocked and combinational (including hybrid) logic into pre/act/nba.
    // All clocks (signals referenced in an AstSenTree) generated via a blocking assignment
    // (including combinationally generated signals) are computed within the act region.
    LogicRegions logicRegions
        = విభజన(logicClasses.m_clocked, logicClasses.m_comb, logicClasses.m_hybrid);
    logicRegions.m_obs = logicClasses.m_observed;
    logicRegions.m_react = logicClasses.m_reactive;
    ఒకవేళ (v3Global.opt.stats()) {
        కూడు_కొలత_Stat("size of region: Active Pre", logicRegions.m_pre);
        కూడు_కొలత_Stat("size of region: Active", logicRegions.m_act);
        కూడు_కొలత_Stat("size of region: NBA", logicRegions.m_nba);
        కూడు_కొలత_Stat("size of region: Observed", logicRegions.m_obs);
        కూడు_కొలత_Stat("size of region: Reactive", logicRegions.m_react);
        V3Stats::statsStage("sched-partition");
    }

    // Step 8: Replicate combinational logic
    LogicReplicas logicReplicas = replicateLogic(logicRegions);
    ఒకవేళ (v3Global.opt.stats()) {
        కూడు_కొలత_Stat("size of replicated logic: Input", logicReplicas.m_ico);
        కూడు_కొలత_Stat("size of replicated logic: Active", logicReplicas.m_act);
        కూడు_కొలత_Stat("size of replicated logic: NBA", logicReplicas.m_nba);
        కూడు_కొలత_Stat("size of replicated logic: Observed", logicReplicas.m_obs);
        కూడు_కొలత_Stat("size of replicated logic: Reactive", logicReplicas.m_react);
        V3Stats::statsStage("sched-replicate");
    }

    // Step 9: Create the input combinational logic
    createIcoRegion(netlistp, staticp, sen_సమాసం_Builder, logicReplicas.m_ico, virtIfaceTriggers,
                    cgRefBindings);
    ఒకవేళ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-ico");

    // Step 10: Create the triggers
    AstVarScope* స్థిరరాశి dpiExportTriggerVscp = netlistp->dpiExportTriggerp();
    netlistp->dpiExportTriggerp(శూన్యసూచిక);  // Finished with this here

    // We may have an extra trigger for variable updated in DPI exports
    TriggerKit::ExtraTriggers extraTriggers;
    స్థిరరాశి uint32_t dpi_Export_Trigger_సూచీ = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    స్థిరరాశి uint32_t మొదటి_Vif_Trigger_సూచీ = extraTriggers.కొలత();
    కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + నమోదు_2.m_ifacep->name() + "."
                               + నమోదు_2.m_memberp->name());
    }

    స్థిరరాశి స్వయంచాలక& preTreeps = get_Sen_Trees_వాడిన_By({&logicRegions.m_pre});
    స్థిరరాశి స్వయంచాలక& senTreeps = get_Sen_Trees_వాడిన_By({&logicRegions.m_act,  //
                                               &logicRegions.m_nba,  //
                                               &logicRegions.m_obs,  //
                                               &logicRegions.m_react,  //
                                               &timingKit.m_lbs});
    స్థిరరాశి TriggerKit trigKit
        = TriggerKit::create(netlistp, staticp, sen_సమాసం_Builder, preTreeps, senTreeps, "act",
                             extraTriggers, అబద్ధం, v3Global.ఉపయోగిస్తుంది_Timing());

    // Add post updates from the timing kit
    ఒకవేళ (timingKit.m_postUpdates) trigKit.compBasep()->కూడు_Stmtsp(timingKit.m_postUpdates);

    ఒకవేళ (dpiExportTriggerVscp) {
        trigKit.కూడు_Extra_Trigger_Assignment(dpiExportTriggerVscp, dpi_Export_Trigger_సూచీ);
    }
    కూడు_Virt_Iface_Trigger_Assignments(netlistp, staticp, virtIfaceTriggers, మొదటి_Vif_Trigger_సూచీ,
                                   trigKit);
    ఒకవేళ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-triggers");

    // Note: Experiments so far show that running the Act (or Ico) regions on
    // multiple threads is always a net loss, so only use multi-threading for
    // NBA for now. This can be revised if evidence is available that it would
    // be beneficial

    // Step 11: Create the 'act' region evaluation function

    // Add the trigger indices of the input logic
    కూడు_Trigger_Indices(logicRegions.m_pre, trigKit, నిజం);
    కూడు_Trigger_Indices(logicRegions.m_act, trigKit, అబద్ధం);
    కూడు_Trigger_Indices(logicReplicas.m_act, trigKit, అబద్ధం);
    కూడు_Trigger_Indices(timingKit.m_lbs, trigKit, అబద్ధం);

    // External triggers of variables. Trigger indices are the same in all regions, so share.
    స్థిరరాశి std::map<స్థిరరాశి AstVarScope*, std::సదిశ<uint32_t>> timingTriggers
        = timingKit.externalTriggerIndices(trigKit);
    స్థిరరాశి స్వయంచాలక& vif_Vscp_గమ్యం_Trig = virtIfaceTriggers.make_Vscp_గమ్యం_Trig_Map(trigKit, మొదటి_Vif_Trigger_సూచీ);
    స్థిరరాశి V3_క్రమం::ExternalTriggersProvider externalTriggers
        = [&](స్థిరరాశి AstVarScope* vscp, std::సమితి<uint32_t>& triggers) {
              స్వయంచాలక it = timingTriggers.కనుగొను(vscp);
              ఒకవేళ (it != timingTriggers.చివర())
                  triggers.చొప్పించు(it->రెండవ.begin(), it->రెండవ.చివర());
              ఒకవేళ (vscp->varp()->isWrittenByDpi()) {
                  triggers.చొప్పించు(trigKit.extra_Trigger_సూచీ(dpi_Export_Trigger_సూచీ));
              }
              ఒకవేళ (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                  కూడు_Triggered_Iface(vscp, vif_Vscp_గమ్యం_Trig, virtIfaceTriggers, triggers);
              }
          };

    AstCFunc* స్థిరరాశి actFuncp
        = V3_క్రమం::క్రమం({&logicRegions.m_pre, &logicRegions.m_act, &logicReplicas.m_act}, trigKit,
                         trigKit.vscp(), cgRefBindings, "act", అబద్ధం, అబద్ధం, externalTriggers);
    util::విభజించు_Check(actFuncp);
    ఒకవేళ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-act");

    స్థిరరాశి EvalKit actKit{trigKit.vscp(), actFuncp};

    // Orders a region's logic and creates the region eval function
    స్థిరరాశి స్వయంచాలక క్రమం = [&](స్థిరరాశి std::string& name,
                           స్థిరరాశి std::సదిశ<V3Sched::LogicByScope*>& logic) -> EvalKit {
        UINFO(2, "Scheduling " << name << " #logic = " << logic.కొలత());
        AstVarScope* స్థిరరాశి trigVscp = trigKit.newTrigVec(name);
        // Add the trigger indices of the input logic
        కొరకు (LogicByScope* lbs : logic) కూడు_Trigger_Indices(*lbs, trigKit, అబద్ధం);

        AstCFunc* స్థిరరాశి funcp
            = V3_క్రమం::క్రమం(logic, trigKit, trigVscp, cgRefBindings, name,
                             name == "nba" && v3Global.opt.mtasks(), అబద్ధం, externalTriggers);

        తిరిగి {trigVscp, funcp};
    };

    // Step 12: Create the 'nba' region evaluation function
    స్థిరరాశి EvalKit nbaKit = క్రమం("nba", {&logicRegions.m_nba, &logicReplicas.m_nba});
    util::విభజించు_Check(nbaKit.m_funcp);
    ఒకవేళ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-nba");

    // Orders a region's logic and creates the region eval function (only if there is any logic in
    // the region)
    స్థిరరాశి స్వయంచాలక క్రమం_If_Non_ఖాళీలు
        = [&](స్థిరరాశి std::string& name, స్థిరరాశి std::సదిశ<LogicByScope*>& logic) -> EvalKit {
        ఒకవేళ (logic[0]->ఖాళీలు())
            తిరిగి {};  // if region is empty, replica is supposed to be empty as well
        స్థిరరాశి స్వయంచాలక& kit = క్రమం(name, logic);
        ఒకవేళ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-" + name);
        తిరిగి kit;
    };

    // Step 13: Create the 'obs' region evaluation function
    స్థిరరాశి EvalKit obsKit = క్రమం_If_Non_ఖాళీలు("obs", {&logicRegions.m_obs, &logicReplicas.m_obs});

    // Step 14: Create the 'react' region evaluation function
    స్థిరరాశి EvalKit reactKit
        = క్రమం_If_Non_ఖాళీలు("react", {&logicRegions.m_react, &logicReplicas.m_react});

    // Step 15: Create the 'postponed' region evaluation function
    createPostponed(netlistp, logicClasses);

    // Step 16: Populate the eval entry point function of each region of a time step
    createEval(netlistp, trigKit, actKit, nbaKit, obsKit, reactKit, timingKit);

    // Step 17: Add neccessary evaluation before awaits
    ఒకవేళ (AstCCall* స్థిరరాశి readyp = timingKit.createReady(trigKit.vscp())) {
        staticp->కూడు_Stmtsp(readyp->makeStmt());
        beforeTrigVisitor(netlistp, sen_సమాసం_Builder, trigKit);
    } లేకపోతే {
        // beforeTrigVisitor clears Sentree pointers in AstCAwaits (as these sentrees will get
        // deleted later) if there was no need to call it, SenTrees have to be cleaned manually
        netlistp->foreach([](AstCAwait* స్థిరరాశి cAwaitp) { cAwaitp->clearSentreep(); });
    }
    ఒకవేళ (AstVarScope* స్థిరరాశి trigAccp = trigKit.vscAccp()) {
        // Copy trigger vector to accumulator at the end of static initialziation so,
        // triggers fired during initialization persist to the first resume.
        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి trigAccDTypep
            = VN_AS(trigAccp->dtypep(), UnpackArrayDType);
        UASSERT_OBJ(
            trigAccDTypep->కుడి() == 0, trigAccp,
            "Expected that trigger vector and accumulator start elements enumeration from 0");
        UASSERT_OBJ(trigAccDTypep->ఎడమ() >= 0, trigAccp,
                    "Expected that trigger vector and accumulator has no negative indexes");
        File_పంక్తి* స్థిరరాశి flp = trigAccp->fileline();
        AstVarScope* స్థిరరాశి vscp = netlistp->శిఖరం_Scopep()->create_తాత్కాలిక("__Vi", 32);
        AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{flp};
        loopp->కూడు_Stmtsp(
            కొత్త Ast_కేటాయించు{flp,
                          కొత్త AstArraySel{flp, కొత్త AstVarRef{flp, trigAccp, VAccess::WRITE},
                                          కొత్త AstVarRef{flp, vscp, VAccess::READ}},
                          కొత్త AstArraySel{flp, కొత్త AstVarRef{flp, actKit.m_vscp, VAccess::READ},
                                          కొత్త AstVarRef{flp, vscp, VAccess::READ}}});
        loopp->కూడు_Stmtsp(util::incrementVar(vscp));
        loopp->కూడు_Stmtsp(కొత్త AstLoopTest{
            flp, loopp,
            కొత్త AstLte{flp, కొత్త AstVarRef{flp, vscp, VAccess::READ},
                       కొత్త AstConst{flp, AstConst::Widthed_విలువ{}, 32,
                                    static_cast<uint32_t>(trigAccDTypep->ఎడమ())}}});
        staticp->కూడు_Stmtsp(loopp);
    }

    // Step 18: Clean up
    netlistp->clear_Stl_మొదటి_Iterationp();

    ఒకవేళ (v3Global.opt.stats()) {
        // A sample() call resolved to the union over its covergroup's constructions reads more
        // than it can, which orders it against more logic than necessary
        V3Stats::కూడు_Stat("Scheduling, covergroup ref sample calls, per instance",
                         cgRefBindings.అంకె_Exact_Calls());
        V3Stats::కూడు_Stat("Scheduling, covergroup ref sample calls, per type",
                         cgRefBindings.అంకె_Union_Calls());
    }

    // Haven't split static initializer yet
    util::విభజించు_Check(staticp);

    // Record eval stats
    netlistp->కూడు_Eval_Stats("sched");

    // Dump
    V3Global::dump_Check_Global_వృక్షం("sched", 0, dump_వృక్షం_Either_Level() >= 3);
}

}  // namespace V3Sched
