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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Sched.h"

#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"
#ಅನ್ತರ್ಭಾವ "V3EmitV.h"
#ಅನ್ತರ್ಭಾವ "V3Order.h"
#ಅನ್ತರ್ಭಾವ "V3SenExprBuilder.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ನಾಮಾಕಾಶ V3Sched {

ನಾಮಾಕಾಶ {

//============================================================================
// Utility functions

std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Ast_Sen_ವೃಕ್ಷ*> get_Sen_Trees_ಬಳಸಿದ_By(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ LogicByScope*>& lbsps) {
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use user1_ಒಳ_Use;
    std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Ast_Sen_ವೃಕ್ಷ*> ಫಲಿತಾಂಶ;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ LogicByScope* ಸ್ಥಿರಾಂಕ lbsp : lbsps) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : *lbsp) {
            Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ನಮೂದು.m_activep;
            Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep = activep->sentreep();
            ಆದರೆ (senTreep->user1_ಗಣ_Once()) ಮುಂದುವರಿ;
            ಆದರೆ (senTreep->hasClocked() || senTreep->hasHybrid()) ಫಲಿತಾಂಶ.ತಳ್ಳು_ಹಿಂಭಾಗ(senTreep);
        }
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಶೂನ್ಯ ಕೂಡಿಸು_Trigger_Indices(LogicByScope& lbs, ಸ್ಥಿರಾಂಕ TriggerKit& trigKit, ತಾರ್ಕಿಕ pre) {
    ಗಾಗಿ (LogicByScope::ನಮೂದು_2& ನಮೂದು : lbs) {
        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep = ನಮೂದು.m_activep->sentreep();
        ಆದರೆ (senTreep->hasCombo()) ಮುಂದುವರಿ;
        ನಮೂದು.m_triggers = trigKit.triggerIndices(senTreep, pre);
    }
}

// Add the VIF triggers that a given VarScope should be sensitive to.
// Case 2 (non-virtual interface read): sensitive to that specific VarScope's trigger only
// Case 3 (virtual interface read): sensitive to all member triggers of the same interface type
ಶೂನ್ಯ ಕೂಡಿಸು_Triggered_Iface(ಸ್ಥಿರಾಂಕ AstVarScope* vscp, ಸ್ಥಿರಾಂಕ VirtIfaceTriggers::VscpTrigMap& vscp_ಗಮ್ಯ_Trig,
                       ಸ್ಥಿರಾಂಕ VirtIfaceTriggers& virtIfaceTriggers, std::ಗಣ<uint32_t>& triggers) {
    ಆದರೆ (vscp->varp()->isVirtIface()) {
        // Virtual interface variable -- sensitive to all member triggers of this interface type
        ಸ್ಥಿರಾಂಕ AstIface* ಸ್ಥಿರಾಂಕ ifacep = VN_AS(vscp->varp()->dtypep(), IfaceRefDType)->ifacep();
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : virtIfaceTriggers.m_triggers) {
            ಆದರೆ (ನಮೂದು.m_ifacep == ifacep) {
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = vscp_ಗಮ್ಯ_Trig.ಕಂಡುಹಿಡಿ(ನಮೂದು.m_vscp);
                ಆದರೆ (it != vscp_ಗಮ್ಯ_Trig.ಕೊನೆ()) triggers.ಸೇರಿಸು(it->ಎರಡನೇ);
            }
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        // Non-virtual interface member -- sensitive to this VarScope's trigger only
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = vscp_ಗಮ್ಯ_Trig.ಕಂಡುಹಿಡಿ(vscp);
        ಆದರೆ (it != vscp_ಗಮ್ಯ_Trig.ಕೊನೆ()) triggers.ಸೇರಿಸು(it->ಎರಡನೇ);
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
ಶೂನ್ಯ createEvalRegion(
    AstNetlist* netlistp,  //
    VEval eval,  // The entry point of the current region
    // Index of the region's 'first iteration' extra trigger, if it has one, otherwise ignored
    uint32_t ಮೊದಲ_Iter_Trigger,
    ಸ್ಥಿರಾಂಕ TriggerKit& trigKit,  // The trigger kit
    AstVarScope* trigp,  // The trigger vector - may be nullptr if no triggers or using 'condp'
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp,  // Explicit condition that must be true to run 'phaseWorkp'
    Ast_ಗಂಟು_Stmt* phasePrepp,  // Prep statements run before checking triggers
    Ast_ಗಂಟು_Stmt* phaseWorkp,  // The work to do if anything triggered
    // Extra statements to run after the work, even if no triggers fired. This function is
    // passed a variable, which must be set to true if we must continue and loop again,
    // and must be unmodified otherwise.
    std::function<Ast_ಗಂಟು_Stmt*(AstVarScope*)> phaseExtra = [](AstVarScope*) { ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ; }  //
) {
    UASSERT(!trigp || !condp, "Cannot use both 'trigp' and 'condp' in 'createEvalRegion'");
    UASSERT(!eval.ಮೊದಲ_Iteration() || trigp,
            "Region without triggers cannot need a first iteration flag");

    // All work is under a trigger or condition, so with neither the region has
    // nothing to evaluate, and what we create below reduces to a no-op function.

    ಸ್ಥಿರಾಂಕ std::string ಟ್ಯಾಗ್ = eval.ಟ್ಯಾಗ್();
    Ast_ತುದಿ_Scope* ಸ್ಥಿರಾಂಕ ತುದಿ_Scopep = netlistp->ತುದಿ_Scopep();
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = netlistp->fileline();

    // Populate the trigger dump entry point function
    ಆದರೆ (trigp) {
        netlistp->dumpTriggersFuncp(eval)->ಕೂಡಿಸು_Stmtsp(trigKit.newDumpCall(trigp, ಟ್ಯಾಗ್, ಸುಳ್ಳು));
    }

    AstCFunc* ಸ್ಥಿರಾಂಕ funcp = netlistp->evalFuncp(eval);

    // A flag is passed from the run-time eval loop if this is the first iteration of the
    // current loop
    ಆದರೆ (eval.ಮೊದಲ_Iteration()) {
        AstVarScope* ಸ್ಥಿರಾಂಕ ಮೊದಲ_Iter_Argp = util::new_ವಾದ(funcp, netlistp->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(),
                                                             "firstIteration", VDirection::ಒಳಹರಿವು);

        // Set the region's 'first iteration' trigger straight from the argument
        funcp->ಕೂಡಿಸು_Stmtsp(trigKit.newExtraTriggerAssignment(ಮೊದಲ_Iter_Argp, ಮೊದಲ_Iter_Trigger));

        // Only 'stl' also needs a module level flag, for design logic that reads
        // it directly (see V3Timing). Those reads can be anywhere in the design,
        // hence module level. Always created, even if nothing reads it.
        // TODO: get rid of this special case
        ಆದರೆ (eval == VEval::STL) {
            AstVarScope* ಸ್ಥಿರಾಂಕ ಮೊದಲ_Iterp = netlistp->stl_ಮೊದಲ_Iterationp();
            ಮೊದಲ_Iterp->varp()->noReset(ನಿಜ);
            ಮೊದಲ_Iterp->varp()->is_ಆಂತರಿಕ(ನಿಜ);
            funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, ಮೊದಲ_Iterp, VAccess::WRITE},
                                           ಹೊಸ AstVarRef{flp, ಮೊದಲ_Iter_Argp, VAccess::READ}});
        }
    }

    {
        // Add the preparatory statements
        funcp->ಕೂಡಿಸು_Stmtsp(phasePrepp);

        // The execute flag
        AstVarScope* ಸ್ಥಿರಾಂಕ executeFlagp = ತುದಿ_Scopep->create_ತಾತ್ಕಾಲಿಕ("__V" + ಟ್ಯಾಗ್ + "Execute", 1);
        executeFlagp->varp()->noReset(ನಿಜ);

        // If there is work in this region, execute it if any triggers fired
        ಆದರೆ (phaseWorkp) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = ಹೊಸ AstVarRef{flp, executeFlagp, VAccess::WRITE};
            // If using explicit condition, that directly determines whether to execute,
            // otherwise check if any triggers are fired
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = condp ? condp : trigKit.new_Any_ಗಣ_Call(trigp);
            funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, lhsp, rhsp});

            // Add the work
            AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, ಹೊಸ AstVarRef{flp, executeFlagp, VAccess::READ}};
            ifp->ಕೂಡಿಸು_Thensp(phaseWorkp);
            funcp->ಕೂಡಿಸು_Stmtsp(ifp);
        }

        // Construct the extra statements
        Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ extraWorkp = phaseExtra(executeFlagp);
        ಆದರೆ (extraWorkp) funcp->ಕೂಡಿಸು_Stmtsp(extraWorkp);

        // The function returns true iff it did run work
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ retp
            = phaseWorkp || extraWorkp
                  ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstVarRef{flp, executeFlagp, VAccess::READ})
                  : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}});
        funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCReturn{flp, retp});
    }
}

//============================================================================
// Collect and classify all logic in the design

LogicClasses gatherLogicClasses(AstNetlist* netlistp) {
    LogicClasses ಫಲಿತಾಂಶ;

    netlistp->foreach([&](AstScope* scopep) {
        scopep->foreach([&](Ast_ಸಕ್ರಿಯ* activep) {
            Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep = activep->sentreep();
            ಆದರೆ (senTreep->hasStatic()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "static initializer with additional sensitivities");
                ಫಲಿತಾಂಶ.m_static.ಕೂಡಿಸು(scopep, activep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (senTreep->hasInitial()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'initial' logic with additional sensitivities");
                ಫಲಿತಾಂಶ.m_initial.ಕೂಡಿಸು(scopep, activep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (senTreep->hasFinal()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'final' logic with additional sensitivities");
                ಫಲಿತಾಂಶ.m_final.ಕೂಡಿಸು(scopep, activep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (senTreep->hasCombo()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "combinational logic with additional sensitivities");
                ಆದರೆ (VN_IS(activep->stmtsp(), AlwaysPostponed)) {
                    ಫಲಿತಾಂಶ.m_postponed.ಕೂಡಿಸು(scopep, activep);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಫಲಿತಾಂಶ.m_comb.ಕೂಡಿಸು(scopep, activep);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                UASSERT_OBJ(senTreep->hasClocked(), activep, "What else could it be?");
                ಆದರೆ (VN_IS(activep->stmtsp(), AlwaysObserved)) {
                    ಫಲಿತಾಂಶ.m_observed.ಕೂಡಿಸು(scopep, activep);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(activep->stmtsp(), AlwaysReactive)) {
                    ಫಲಿತಾಂಶ.m_reactive.ಕೂಡಿಸು(scopep, activep);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಫಲಿತಾಂಶ.m_clocked.ಕೂಡಿಸು(scopep, activep);
                }
            }
        });
    });

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

//============================================================================
// Simple ordering in source order

ಶೂನ್ಯ ಕ್ರಮ_Sequentially(AstCFunc* funcp, ಸ್ಥಿರಾಂಕ LogicByScope& lbs) {
    // Create new subfunc for scope
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ createNewSubFuncp = [&](AstScope* ಸ್ಥಿರಾಂಕ scopep) {
        ಸ್ಥಿರಾಂಕ string subName{funcp->name() + "__" + scopep->nameDotless()};
        AstCFunc* ಸ್ಥಿರಾಂಕ subFuncp = ಹೊಸ AstCFunc{scopep->fileline(), subName, scopep};
        subFuncp->isLoose(ನಿಜ);
        subFuncp->isConst(ಸುಳ್ಳು);
        subFuncp->decl_ಖಾಸಗಿ(ನಿಜ);
        subFuncp->slow(funcp->slow());
        scopep->ಕೂಡಿಸು_Blocksp(subFuncp);
        // Call it from the top function
        funcp->ಕೂಡಿಸು_Stmtsp(util::callVoidFunc(subFuncp));
        ಹಿಂತಿರುಗಿ subFuncp;
    };
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use user1_ಒಳ_Use;  // AstScope -> AstCFunc: the sub-function for the scope
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use user2_ಒಳ_Use;  // AstScope -> int: sub-function counter used for names
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : lbs) {
        AstScope* ಸ್ಥಿರಾಂಕ scopep = ನಮೂದು.m_scopep;
        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ನಮೂದು.m_activep;
        // Create a sub-function per scope so we can V3Combine them later
        ಆದರೆ (!scopep->user1p()) scopep->user1p(createNewSubFuncp(scopep));
        // Add statements to sub-function
        ಗಾಗಿ (Ast_ಗಂಟು *logicp = activep->stmtsp(), *nextp; logicp; logicp = nextp) {
            ಸ್ವಯಂಚಾಲಿತ* subFuncp = VN_AS(scopep->user1p(), CFunc);
            nextp = logicp->nextp();
            ಆದರೆ (Ast_ಗಂಟು_Procedure* ಸ್ಥಿರಾಂಕ procp = VN_CAST(logicp, ಗಂಟು_Procedure)) {
                ಆದರೆ (Ast_ಗಂಟು* bodyp = procp->stmtsp()) {
                    bodyp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
                    // If the process is suspendable, we need a separate function (a coroutine)
                    ಆದರೆ (procp->isSuspendable()) {
                        funcp->slow(ಸುಳ್ಳು);
                        subFuncp = createNewSubFuncp(scopep);
                        subFuncp->name(subFuncp->name() + "__Vtiming__"
                                       + cvt_ಗಮ್ಯ_Str(scopep->user2Inc()));
                        subFuncp->rtnType("VlCoroutine");
                        ಆದರೆ (VN_IS(procp, Always)) {
                            subFuncp->slow(ಸುಳ್ಳು);
                            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = procp->fileline();
                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = ಹೊಸ AstCExpr{
                                flp, "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish())", 1};
                            AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{flp};
                            loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{flp, loopp, condp});
                            loopp->ಕೂಡಿಸು_Stmtsp(bodyp);
                            bodyp = loopp;
                        }
                    }
                    subFuncp->ಕೂಡಿಸು_Stmtsp(bodyp);
                    ಆದರೆ (procp->needProcess()) subFuncp->ಗಣ_Need_Process();
                    // A coroutine holds a single process, so it is complete here
                    ಆದರೆ (procp->isSuspendable()) util::ವಿಭಜಿಸು_Check(subFuncp);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                logicp->unlink_Fr_ಹಿಂಭಾಗ();
                subFuncp->ಕೂಡಿಸು_Stmtsp(logicp);
            }
        }
        ಆದರೆ (activep->backp()) activep->unlink_Fr_ಹಿಂಭಾಗ();
        VL_DO_DANGLING(activep->delete_ವೃಕ್ಷ(), activep);
    }
    // Split sub-functions once complete
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : lbs) {
        AstScope* ಸ್ಥಿರಾಂಕ scopep = ನಮೂದು.m_scopep;
        ಆದರೆ (AstCFunc* ಸ್ಥಿರಾಂಕ subFuncp = VN_AS(scopep->user1p(), CFunc)) {
            util::ವಿಭಜಿಸು_Check(subFuncp);
            scopep->user1p(ಶೂನ್ಯಸೂಚಕ);
        }
    }
}

//============================================================================
// Create simply ordered functions

AstCFunc* createStatic(AstNetlist* netlistp, ಸ್ಥಿರಾಂಕ LogicClasses& logicClasses) {
    AstCFunc* ಸ್ಥಿರಾಂಕ funcp = netlistp->evalFuncp(VEval::STATIC);

    ಸ್ಥಿರಾಂಕ LogicByScope& orig = logicClasses.m_static;
    ಆದರೆ (orig.ಗಾತ್ರ_2() <= 1) {
        ಕ್ರಮ_Sequentially(funcp, orig);
        ಹಿಂತಿರುಗಿ funcp;
    }

    // Level-based module sorting can reorder packages so that an importing
    // package runs before the imported one.  Re-sort package entries by source
    // file position to restore compilation order (IEEE 1800-2023 26.3).
    std::ಸದಿಶ<ಗಾತ್ರ_t> indices(orig.ಗಾತ್ರ_2());
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < orig.ಗಾತ್ರ_2(); ++i) indices[i] = i;
    std::stable_ವಿಂಗಡಿಸು(indices.begin(), indices.ಕೊನೆ(), [&](ಗಾತ್ರ_t a, ಗಾತ್ರ_t b) {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modA = orig[a].m_scopep->modp();
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modB = orig[b].m_scopep->modp();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isPkgA = VN_IS(modA, Package);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isPkgB = VN_IS(modB, Package);
        ಆದರೆ (isPkgA != isPkgB) ಹಿಂತಿರುಗಿ isPkgA;  // Packages before non-packages
        ಆದರೆ (isPkgA && isPkgB) {
            // Sort packages by source file position (compilation order)
            ಹಿಂತಿರುಗಿ modA->fileline()->operatorCompare(*modB->fileline()) < 0;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Both non-package: preserve original order
    });
    LogicByScope sorted;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t i : indices) sorted.ಕೂಡಿಸು(orig[i].m_scopep, orig[i].m_activep);

    ಕ್ರಮ_Sequentially(funcp, sorted);
    ಹಿಂತಿರುಗಿ funcp;  // Not splitting yet as it is not final
}

ಶೂನ್ಯ createInitial(AstNetlist* netlistp, ಸ್ಥಿರಾಂಕ LogicClasses& logicClasses) {
    AstCFunc* ಸ್ಥಿರಾಂಕ funcp = netlistp->evalFuncp(VEval::INITIAL);
    ಕ್ರಮ_Sequentially(funcp, logicClasses.m_initial);
    util::ವಿಭಜಿಸು_Check(funcp);
}

ಶೂನ್ಯ createPostponed(AstNetlist* netlistp, ಸ್ಥಿರಾಂಕ LogicClasses& logicClasses) {
    AstCFunc* ಸ್ಥಿರಾಂಕ funcp = netlistp->evalFuncp(VEval::POSTPONED);
    ಕ್ರಮ_Sequentially(funcp, logicClasses.m_postponed);
    util::ವಿಭಜಿಸು_Check(funcp);
}

ಶೂನ್ಯ createFinal(AstNetlist* netlistp, ಸ್ಥಿರಾಂಕ LogicClasses& logicClasses) {
    AstCFunc* ಸ್ಥಿರಾಂಕ funcp = netlistp->evalFuncp(VEval::FINAL);
    ಕ್ರಮ_Sequentially(funcp, logicClasses.m_final);
    util::ವಿಭಜಿಸು_Check(funcp);
}

//============================================================================
// Helper that creates virtual interface value-change triggers

ಶೂನ್ಯ ಕೂಡಿಸು_Virt_Iface_Trigger_Assignments(AstNetlist* netlistp, AstCFunc* initFuncp,
                                    ಸ್ಥಿರಾಂಕ VirtIfaceTriggers& virtIfaceTriggers,
                                    uint32_t ಮೊದಲ_ಸೂಚ್ಯಂಕ, ಸ್ಥಿರಾಂಕ TriggerKit& trigKit) {
    uint32_t ಸೂಚ್ಯಂಕ = ಮೊದಲ_ಸೂಚ್ಯಂಕ;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : virtIfaceTriggers.m_triggers) {
        trigKit.ಕೂಡಿಸು_ಮೌಲ್ಯ_Change_Trigger_Assignment(netlistp, initFuncp, ನಮೂದು.m_vscp, ಸೂಚ್ಯಂಕ);
        ++ಸೂಚ್ಯಂಕ;
    }
}

// Order the combinational logic to create the 'stl' region
ಶೂನ್ಯ createSettle(AstNetlist* netlistp, AstCFunc* ಸ್ಥಿರಾಂಕ initFuncp, Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder& sen_ಅಭಿವ್ಯಕ್ತಿ_Bulider,
                  LogicClasses& logicClasses, ಸ್ಥಿರಾಂಕ CovergroupRefBindings& cgRefBindings) {
    // Clone, because ordering is destructive, but we still need them for the other regions
    LogicByScope comb = logicClasses.m_comb.clone();
    LogicByScope hybrid = logicClasses.m_hybrid.clone();

    // We have an extra trigger denoting this is the first iteration of the settle loop
    TriggerKit::ExtraTriggers extraTriggers;
    ಸ್ಥಿರಾಂಕ uint32_t ಮೊದಲ_Iteration_Trigger = extraTriggers.allocate("first iteration");

    // Gather the relevant sensitivity expressions and create the trigger kit
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& senTreeps = get_Sen_Trees_ಬಳಸಿದ_By({&comb, &hybrid});
    ಸ್ಥಿರಾಂಕ TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_ಅಭಿವ್ಯಕ್ತಿ_Bulider, {},
                                                  senTreeps, "stl", extraTriggers, ನಿಜ, ಸುಳ್ಳು);

    // Add trigger indices (comb has none, so only do the hybrid)
    ಕೂಡಿಸು_Trigger_Indices(hybrid, trigKit, ಸುಳ್ಳು);

    // First trigger is for pure combinational triggers (first iteration)
    ಸ್ಥಿರಾಂಕ uint32_t ಒಳಹರಿವು_ಬದಲಾಯಿತು = trigKit.extra_Trigger_ಸೂಚ್ಯಂಕ(ಮೊದಲ_Iteration_Trigger);

    // Create and the body function
    AstCFunc* ಸ್ಥಿರಾಂಕ stlFuncp = V3_ಕ್ರಮ::ಕ್ರಮ(
        {&comb, &hybrid}, trigKit, trigKit.vscp(), cgRefBindings, "stl", ಸುಳ್ಳು, ನಿಜ,
        [=](ಸ್ಥಿರಾಂಕ AstVarScope*, std::ಗಣ<uint32_t>& triggers) { triggers.ಸೇರಿಸು(ಒಳಹರಿವು_ಬದಲಾಯಿತು); });
    util::ವಿಭಜಿಸು_Check(stlFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::STL, ಮೊದಲ_Iteration_Trigger, trigKit,
        // Use trigger
        trigKit.vscp(), ಶೂನ್ಯಸೂಚಕ,
        // Explicit condition
        // Prep statements: Compute the current 'stl' triggers
        [&trigKit] {
            Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ stmtp = trigKit.new_Comp_ತಳ_Call();
            ಆದರೆ (stmtp) stmtp->ಕೂಡಿಸು_ಮುಂದಿನ(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), ನಿಜ));
            ಹಿಂತಿರುಗಿ stmtp;
        }(),
        // Work statements: Invoke the 'stl' function
        util::callVoidFunc(stlFuncp));
}

//============================================================================
// Order the replicated combinational logic to create the 'ico' region

ಶೂನ್ಯ createIcoRegion(AstNetlist* netlistp, AstCFunc* ಸ್ಥಿರಾಂಕ initFuncp,
                     Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder& sen_ಅಭಿವ್ಯಕ್ತಿ_Builder, LogicByScope& logic,
                     ಸ್ಥಿರಾಂಕ VirtIfaceTriggers& virtIfaceTriggers,
                     ಸ್ಥಿರಾಂಕ CovergroupRefBindings& cgRefBindings) {
    // SystemC only: Any top level inputs feeding a combinational logic must be marked,
    // so we can make them sc_sensitive
    ಆದರೆ (v3Global.opt.systemC()) {
        logic.foreachLogic([](Ast_ಗಂಟು* logicp) {
            logicp->foreach([](AstVarRef* refp) {
                ಆದರೆ (refp->access().isWriteOnly()) ಹಿಂತಿರುಗಿ;
                AstVarScope* ಸ್ಥಿರಾಂಕ vscp = refp->varScopep();
                ಆದರೆ (vscp->scopep()->is_ತುದಿ() && vscp->varp()->isNonOutput()) {
                    vscp->varp()->scSensitive(ನಿಜ);
                }
            });
        });
    }

    // We have some extra trigger denoting external conditions
    AstVarScope* ಸ್ಥಿರಾಂಕ dpiExportTriggerVscp = netlistp->dpiExportTriggerp();

    TriggerKit::ExtraTriggers extraTriggers;
    ಸ್ಥಿರಾಂಕ uint32_t ಮೊದಲ_Iteration_Trigger = extraTriggers.allocate("first iteration");
    ಸ್ಥಿರಾಂಕ uint32_t dpi_Export_Trigger_ಸೂಚ್ಯಂಕ = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    ಸ್ಥಿರಾಂಕ uint32_t ಮೊದಲ_Vif_Trigger_ಸೂಚ್ಯಂಕ = extraTriggers.ಗಾತ್ರ_2();
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + ನಮೂದು.m_ifacep->name() + "."
                               + ನಮೂದು.m_memberp->name());
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
    std::unordered_map<ಸ್ಥಿರಾಂಕ AstVarScope*, Ast_Sen_ವೃಕ್ಷ*> inp2changedp;
    std::ಸದಿಶ<Ast_Sen_ವೃಕ್ಷ*> icoChangeSenTreeps;
    ಆದರೆ (v3Global.opt.fIcoChangeDetect().isTrue()) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = netlistp->fileline();
        AstScope* ಸ್ಥಿರಾಂಕ scopep = netlistp->ತುದಿ_Scopep()->scopep();
        ಗಾಗಿ (AstVarScope* vscp = scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            // Only for top level ports, assume outputs don't change externally
            ಆದರೆ (!vscp->varp()->isPrimaryInish()) ಮುಂದುವರಿ;
            // Don't do if written by the design - wouldn't update the change detect 'prev' value
            ಆದರೆ (vscp->varp()->icoMaybeWritten()) ಮುಂದುವರಿ;
            // Don't do if forceable, as we can't see the actual value - this is belt and braces
            ಆದರೆ (vscp->varp()->isForced()) ಮುಂದುವರಿ;
            // Can't handle unpacked arrays (they have special types when primary input)
            ಆದರೆ (VN_IS(vscp->dtypep()->skipRefp(), UnpackArrayDType)) ಮುಂದುವರಿ;
            // Similarly to arrays, can't handle SystemC types
            ಆದರೆ (vscp->varp()->isSc()) ಮುಂದುವರಿ;
            // Create a sen tree triggered when this input changes
            Ast_Sen_ವೃಕ್ಷ*& senTreepr = inp2changedp[vscp];
            UASSERT_OBJ(!senTreepr, vscp, "Duplicate input change detect trigger");
            AstVarRef* ಸ್ಥಿರಾಂಕ refp = ಹೊಸ AstVarRef{flp, vscp, VAccess::READ};
            Ast_Sen_ಅಂಶ* ಸ್ಥಿರಾಂಕ senItemp = ಹೊಸ Ast_Sen_ಅಂಶ{flp, VEdgeType::ET_ಬದಲಾಯಿತು, refp};
            senTreepr = ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, senItemp};
            icoChangeSenTreeps.ತಳ್ಳು_ಹಿಂಭಾಗ(senTreepr);
        }
    }
    V3Stats::ಕೂಡಿಸು_Stat("Scheduling, 'ico' change detect triggers", icoChangeSenTreeps.ಗಾತ್ರ_2());

    // Gather the relevant sensitivity expressions and create the trigger kit
    std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Ast_Sen_ವೃಕ್ಷ*> senTreeps = get_Sen_Trees_ಬಳಸಿದ_By({&logic});
    senTreeps.ಸೇರಿಸು(senTreeps.ಕೊನೆ(), icoChangeSenTreeps.begin(), icoChangeSenTreeps.ಕೊನೆ());
    ಸ್ಥಿರಾಂಕ TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_ಅಭಿವ್ಯಕ್ತಿ_Builder, {},
                                                  senTreeps, "ico", extraTriggers, ಸುಳ್ಳು, ಸುಳ್ಳು);
    std::ignore = sen_ಅಭಿವ್ಯಕ್ತಿ_Builder.getAndClearResults();

    ಆದರೆ (dpiExportTriggerVscp) {
        trigKit.ಕೂಡಿಸು_Extra_Trigger_Assignment(dpiExportTriggerVscp, dpi_Export_Trigger_ಸೂಚ್ಯಂಕ);
    }
    ಕೂಡಿಸು_Virt_Iface_Trigger_Assignments(netlistp, initFuncp, virtIfaceTriggers, ಮೊದಲ_Vif_Trigger_ಸೂಚ್ಯಂಕ,
                                   trigKit);

    // Add trigger indices
    ಕೂಡಿಸು_Trigger_Indices(logic, trigKit, ಸುಳ್ಳು);

    // The 'first iteration' trigger for top level inputs
    ಸ್ಥಿರಾಂಕ uint32_t ಮೊದಲ_Iter_Trigger = trigKit.extra_Trigger_ಸೂಚ್ಯಂಕ(ಮೊದಲ_Iteration_Trigger);

    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& vif_Vscp_ಗಮ್ಯ_Trig_Ico
        = virtIfaceTriggers.make_Vscp_ಗಮ್ಯ_Trig_Map(trigKit, ಮೊದಲ_Vif_Trigger_ಸೂಚ್ಯಂಕ);

    // Create and Order the body function
    AstCFunc* ಸ್ಥಿರಾಂಕ icoFuncp = V3_ಕ್ರಮ::ಕ್ರಮ(
        {&logic}, trigKit, trigKit.vscp(), cgRefBindings, "ico", ಸುಳ್ಳು, ಸುಳ್ಳು,
        [&](ಸ್ಥಿರಾಂಕ AstVarScope* vscp, std::ಗಣ<uint32_t>& triggers) {
            AstVar* ಸ್ಥಿರಾಂಕ varp = vscp->varp();
            // If it has an explicit change detect trigger, use that,
            // otherwise fall back to using the 'first iteration' trigger
            ಸ್ವಯಂಚಾಲಿತ it = inp2changedp.ಕಂಡುಹಿಡಿ(vscp);
            ಆದರೆ (it != inp2changedp.ಕೊನೆ()) {
                triggers.ಸೇರಿಸು(trigKit.senItem2TrigIdx(it->ಎರಡನೇ->sensesp()));
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (varp->isPrimaryInish() || varp->isSigUserRWPublic() || varp->sampled()) {
                triggers.ಸೇರಿಸು(ಮೊದಲ_Iter_Trigger);
            }
            // Add other triggers
            ಆದರೆ (varp->isWrittenByDpi())
                triggers.ಸೇರಿಸು(trigKit.extra_Trigger_ಸೂಚ್ಯಂಕ(dpi_Export_Trigger_ಸೂಚ್ಯಂಕ));
            ಆದರೆ (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                ಕೂಡಿಸು_Triggered_Iface(vscp, vif_Vscp_ಗಮ್ಯ_Trig_Ico, virtIfaceTriggers, triggers);
            }
        });
    util::ವಿಭಜಿಸು_Check(icoFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::ICO, ಮೊದಲ_Iteration_Trigger, trigKit,
        // Use trigger
        trigKit.vscp(), ಶೂನ್ಯಸೂಚಕ,
        // Prep statements: Compute the current 'ico' triggers
        [&trigKit] {
            Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ stmtp = trigKit.new_Comp_ತಳ_Call();
            ಆದರೆ (stmtp) stmtp->ಕೂಡಿಸು_ಮುಂದಿನ(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), ನಿಜ));
            ಹಿಂತಿರುಗಿ stmtp;
        }(),
        // Work statements: Invoke the 'ico' function
        util::callVoidFunc(icoFuncp));

    // Release temporary input change detect SenTrees
    ಗಾಗಿ (Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep : icoChangeSenTreeps)
        VL_DO_DANGLING(senTreep->delete_ವೃಕ್ಷ(), senTreep);
    icoChangeSenTreeps.clear();
}

//============================================================================
// EvalKit groups items that have to be passed to createEval() for a given eval region

ರಚನೆ EvalKit final {
    // The AstVarScope representing the region's trigger vector
    AstVarScope* ಸ್ಥಿರಾಂಕ m_vscp = ಶೂನ್ಯಸೂಚಕ;
    // The AstCFunc that evaluates the region's logic
    AstCFunc* ಸ್ಥಿರಾಂಕ m_funcp = ಶೂನ್ಯಸೂಚಕ;
    // Is this kit used/required?
    ತಾರ್ಕಿಕ ಖಾಲಿಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ !m_funcp; }
};

//============================================================================
// Create the evaluation function of each region of a time step

ಶೂನ್ಯ createEval(AstNetlist* netlistp,  //
                ಸ್ಥಿರಾಂಕ TriggerKit& trigKit,  //
                ಸ್ಥಿರಾಂಕ EvalKit& actKit,  //
                ಸ್ಥಿರಾಂಕ EvalKit& nbaKit,  //
                ಸ್ಥಿರಾಂಕ EvalKit& obsKit,  //
                ಸ್ಥಿರಾಂಕ EvalKit& reactKit,  //
                TimingKit& timingKit  //
) {
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = netlistp->fileline();

    // Grab the delay scheduler variable, if any
    AstVarScope* ಸ್ಥಿರಾಂಕ delaySchedVscp = timingKit.getDelayScheduler(netlistp);

    // 'createResume' consumes the contents that 'createReady' needs, so do the right order
    AstCCall* ಸ್ಥಿರಾಂಕ timingReadyp = timingKit.createReady(trigKit.vscp());
    AstCCall* ಸ್ಥಿರಾಂಕ timingResumep = timingKit.createResume(trigKit.vscp());

    // Create the 'act' region
    createEvalRegion(  //
        netlistp, VEval::ACT, 0, trigKit,
        // Use trigger
        actKit.m_vscp, ಶೂನ್ಯಸೂಚಕ,
        // Prep statements
        [&]() {
            // Compute the current 'act' triggers - the NBA triggers are the latched value
            Ast_ಗಂಟು_Stmt* stmtsp = trigKit.new_Comp_ತಳ_Call();
            Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ dumpp
                = stmtsp ? trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), ನಿಜ) : ಶೂನ್ಯಸೂಚಕ;
            // Mark as ready for triggered awaits
            ಆದರೆ (timingReadyp) stmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(stmtsp, timingReadyp->makeStmt());
            ಆದರೆ (AstVarScope* ಸ್ಥಿರಾಂಕ vscAccp = trigKit.vscAccp()) {
                stmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(stmtsp, trigKit.newOrIntoCall(actKit.m_vscp, vscAccp));
            }
            stmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(stmtsp, trigKit.newCompExtCall(nbaKit.m_vscp));
            stmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(stmtsp, dumpp);
            // Latch the 'act' triggers under the 'nba' triggers
            stmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(stmtsp, trigKit.newOrIntoCall(nbaKit.m_vscp, actKit.m_vscp));
            //
            ಹಿಂತಿರುಗಿ stmtsp;
        }(),
        // Work statements
        [&]() {
            Ast_ಗಂಟು_Stmt* workp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (AstVarScope* ಸ್ಥಿರಾಂಕ actAccp = trigKit.vscAccp()) {
                AstCMethodHard* ಸ್ಥಿರಾಂಕ cCallp = ಹೊಸ AstCMethodHard{
                    flp, ಹೊಸ AstVarRef{flp, actAccp, VAccess::WRITE}, VCMethod::UNPACKED_FILL,
                    ಹೊಸ AstConst{flp, AstConst::Unsized64{}, 0}};
                cCallp->dtype_ಗಣ_Void();
                workp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(workp, cCallp->makeStmt());
            }
            // Resume triggered timing schedulers
            ಆದರೆ (timingResumep) workp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(workp, timingResumep->makeStmt());
            // Invoke the 'act' function
            workp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(workp, util::callVoidFunc(actKit.m_funcp));
            //
            ಹಿಂತಿರುಗಿ workp;
        }());

    // Create the 'inact' region
    createEvalRegion(  //
        netlistp, VEval::INACT, 0, trigKit,
        // Use explicit condition
        ಶೂನ್ಯಸೂಚಕ,
        [&]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
            ಆದರೆ (!delaySchedVscp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            // Run if any zero delays are pending
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ callp
                = ಹೊಸ AstCMethodHard{flp, ಹೊಸ AstVarRef{flp, delaySchedVscp, VAccess::READ},
                                     VCMethod::SCHED_AWAITING_ZERO_DELAY};
            callp->dtype_ಗಣ_ಬಿಟ್();
            ಹಿಂತಿರುಗಿ callp;
        }(),
        // Prep statements
        ಶೂನ್ಯಸೂಚಕ,
        // Work statements
        [&]() -> Ast_ಗಂಟು_Stmt* {
            ಆದರೆ (!delaySchedVscp) {
                // Nothing to do if there are no delays at all in the design
                ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (v3Global.ಬಳಸುತ್ತದೆ_Zero_Delay()) {
                // Resume processes watiting for #0 delay
                AstCMethodHard* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCMethodHard{
                    flp, ಹೊಸ AstVarRef{flp, delaySchedVscp, VAccess::READWRITE},
                    VCMethod::SCHED_RESUME_ZERO_DELAY};
                callp->dtype_ಗಣ_Void();
                ಹಿಂತಿರುಗಿ callp->makeStmt();
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Assumption was that the design doesn't use #0 delays.
                // Die at run-time if it does.
                AstCStmt* ಸ್ಥಿರಾಂಕ stmtp = ಹೊಸ AstCStmt{flp};
                ಸ್ಥಿರಾಂಕ File_ಸಾಲು* ಸ್ಥಿರಾಂಕ locp = netlistp->ತುದಿ_Modulep()->fileline();
                ಸ್ಥಿರಾಂಕ std::string& file = VIdProtect::protect(locp->filename());
                ಸ್ಥಿರಾಂಕ std::string& ಸಾಲು = std::ಗಮ್ಯ_string(locp->lineno());
                stmtp->ಕೂಡಿಸು("VL_FATAL_MT(\"" + V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(file) + "\", "
                           + ಸಾಲು
                           + ", \"\", \"ZERODLY: Design Verilated with '--no-sched-zero-delay', "
                           + "but #0 delay executed at runtime\");");
                ಹಿಂತಿರುಗಿ stmtp;
            }
        }());

    // Create the 'nba' region
    createEvalRegion(  //
        netlistp, VEval::NBA, 0, trigKit,
        // Use trigger
        nbaKit.m_vscp, ಶೂನ್ಯಸೂಚಕ,
        // Prep statements
        ಶೂನ್ಯಸೂಚಕ,
        // Work statements
        [&]() {
            Ast_ಗಂಟು_Stmt* workp = ಶೂನ್ಯಸೂಚಕ;
            // Latch the 'nba' trigger flags under the following region's trigger flags
            ಆದರೆ (!obsKit.ಖಾಲಿಗಳು()) {
                workp = trigKit.newOrIntoCall(obsKit.m_vscp, nbaKit.m_vscp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!reactKit.ಖಾಲಿಗಳು()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, nbaKit.m_vscp);
            }
            // Invoke the 'nba' function
            workp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(workp, util::callVoidFunc(nbaKit.m_funcp));
            // Clear the 'nba' triggers
            workp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(workp, trigKit.newClearCall(nbaKit.m_vscp));
            //
            ಹಿಂತಿರುಗಿ workp;
        }(),
        // Extra work (not conditional on having had a fired trigger)
        [&](AstVarScope* continuep) -> Ast_ಗಂಟು_Stmt* {
            // Check if any dynamic NBAs are pending, if there are any in the design
            ಆದರೆ (!netlistp->nbaEventp()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            AstVarScope* ಸ್ಥಿರಾಂಕ nbaEventp = netlistp->nbaEventp();
            AstVarScope* ಸ್ಥಿರಾಂಕ nbaEventTriggerp = netlistp->nbaEventTriggerp();
            UASSERT(nbaEventTriggerp, "NBA event trigger var should exist");
            netlistp->nbaEventp(ಶೂನ್ಯಸೂಚಕ);
            netlistp->nbaEventTriggerp(ಶೂನ್ಯಸೂಚಕ);

            // If a dynamic NBA is pending, clear the pending flag and fire the ready event
            AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, ಹೊಸ AstVarRef{flp, nbaEventTriggerp, VAccess::READ}};
            ifp->ಕೂಡಿಸು_Thensp(util::ಗಣ_Var(continuep, 1));
            ifp->ಕೂಡಿಸು_Thensp(util::ಗಣ_Var(nbaEventTriggerp, 0));
            AstCMethodHard* ಸ್ಥಿರಾಂಕ firep = ಹೊಸ AstCMethodHard{
                flp, ಹೊಸ AstVarRef{flp, nbaEventp, VAccess::WRITE}, VCMethod::EVENT_FIRE};
            firep->dtype_ಗಣ_Void();
            ifp->ಕೂಡಿಸು_Thensp(firep->makeStmt());
            ಹಿಂತಿರುಗಿ ifp;
        });

    // Create the 'obs' region
    createEvalRegion(  //
        netlistp, VEval::OBS, 0, trigKit,
        // Use trigger
        obsKit.m_vscp, ಶೂನ್ಯಸೂಚಕ,
        // Prep statements
        ಶೂನ್ಯಸೂಚಕ,
        // Work statements
        [&]() -> Ast_ಗಂಟು_Stmt* {
            ಆದರೆ (obsKit.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            Ast_ಗಂಟು_Stmt* workp = ಶೂನ್ಯಸೂಚಕ;
            // Latch the Observed trigger flags under the Reactive trigger flags
            ಆದರೆ (!reactKit.ಖಾಲಿಗಳು()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, obsKit.m_vscp);
            }
            // Invoke the 'obs' function
            workp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(workp, util::callVoidFunc(obsKit.m_funcp));
            // Clear the 'obs' triggers
            workp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(workp, trigKit.newClearCall(obsKit.m_vscp));
            //
            ಹಿಂತಿರುಗಿ workp;
        }());

    // Create the 'react' region
    createEvalRegion(  //
        netlistp, VEval::REACT, 0, trigKit,
        // Use trigger
        reactKit.m_vscp, ಶೂನ್ಯಸೂಚಕ,
        // Prep statements
        ಶೂನ್ಯಸೂಚಕ,
        // Work statements
        [&]() -> Ast_ಗಂಟು_Stmt* {
            ಆದರೆ (reactKit.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            // Invoke the 'react' function
            Ast_ಗಂಟು_Stmt* workp = util::callVoidFunc(reactKit.m_funcp);
            // Clear the 'react' triggers
            workp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(workp, trigKit.newClearCall(reactKit.m_vscp));
            ಹಿಂತಿರುಗಿ workp;
        }());
}

}  // namespace

//============================================================================
// Helper that builds virtual interface trigger sentrees

VirtIfaceTriggers::VscpTrigMap VirtIfaceTriggers::make_Vscp_ಗಮ್ಯ_Trig_Map(ಸ್ಥಿರಾಂಕ TriggerKit& trigKit,
                                                                    uint32_t ಮೊದಲ_ಸೂಚ್ಯಂಕ) ಸ್ಥಿರಾಂಕ {
    VscpTrigMap map;
    uint32_t ಸೂಚ್ಯಂಕ = ಮೊದಲ_ಸೂಚ್ಯಂಕ;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : m_triggers) {
        map.emplace(ನಮೂದು.m_vscp, trigKit.extra_Trigger_ಸೂಚ್ಯಂಕ(ಸೂಚ್ಯಂಕ));
        ++ಸೂಚ್ಯಂಕ;
    }
    ಹಿಂತಿರುಗಿ map;
}

//============================================================================
// Top level entry-point to scheduling

ಶೂನ್ಯ ವೇಳಾಪಟ್ಟಿ(AstNetlist* netlistp) {
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕೂಡಿಸು_ಗಾತ್ರ_Stat = [](ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ LogicByScope& lbs) {
        uint64_t ಗಾತ್ರ_2 = 0;
        lbs.foreachLogic([&](Ast_ಗಂಟು* nodep) { ಗಾತ್ರ_2 += nodep->ಗಂಟು_ಎಣಿಕೆ(); });
        V3Stats::ಕೂಡಿಸು_Stat("Scheduling, " + name, ಗಾತ್ರ_2);
    };

    // Step 2: Prepare external domains for timing and virtual interfaces
    // Create extra triggers for virtual interfaces
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& virtIfaceTriggers = makeVirtIfaceTriggers(netlistp);
    // Resolve what covergroup reference formal arguments are bound to, which is visible here
    // but not from V3Order, where the reads through them must be modeled
    ಸ್ಥಿರಾಂಕ CovergroupRefBindings& cgRefBindings = makeCovergroupRefBindings(netlistp);
    // Prepare timing-related logic and external domains
    TimingKit timingKit = prepareTiming(netlistp);

    // Step 3: Gather and classify all logic in the design
    LogicClasses logicClasses = gatherLogicClasses(netlistp);

    ಆದರೆ (v3Global.opt.stats()) {
        V3Stats::statsStage("sched-gather");
        ಕೂಡಿಸು_ಗಾತ್ರ_Stat("size of class: static", logicClasses.m_static);
        ಕೂಡಿಸು_ಗಾತ್ರ_Stat("size of class: initial", logicClasses.m_initial);
        ಕೂಡಿಸು_ಗಾತ್ರ_Stat("size of class: final", logicClasses.m_final);
    }

    // Step 4: Schedule static, initial and final logic classes in source order
    AstCFunc* ಸ್ಥಿರಾಂಕ staticp = createStatic(netlistp, logicClasses);
    ಆದರೆ (v3Global.opt.stats()) V3Stats::statsStage("sched-static");

    createInitial(netlistp, logicClasses);
    ಆದರೆ (v3Global.opt.stats()) V3Stats::statsStage("sched-initial");

    createFinal(netlistp, logicClasses);
    ಆದರೆ (v3Global.opt.stats()) V3Stats::statsStage("sched-final");

    // Step 5: Break combinational cycles by introducing hybrid logic
    // Note: breakCycles also removes corresponding logic from logicClasses.m_comb;
    logicClasses.m_hybrid = break_ಚಕ್ರಗಳು(netlistp, logicClasses.m_comb);
    ಆದರೆ (v3Global.opt.stats()) {
        ಕೂಡಿಸು_ಗಾತ್ರ_Stat("size of class: clocked", logicClasses.m_clocked);
        ಕೂಡಿಸು_ಗಾತ್ರ_Stat("size of class: combinational", logicClasses.m_comb);
        ಕೂಡಿಸು_ಗಾತ್ರ_Stat("size of class: hybrid", logicClasses.m_hybrid);
        V3Stats::statsStage("sched-break-cycles");
    }

    // We pass around a single SenExprBuilder instance, as we only need one set of 'prev' variables
    // for edge/change detection in sensitivity expressions, which this keeps track of.
    Ast_ತುದಿ_Scope* ಸ್ಥಿರಾಂಕ ತುದಿ_Scopep = netlistp->ತುದಿ_Scopep();
    AstScope* ಸ್ಥಿರಾಂಕ scopeTopp = ತುದಿ_Scopep->scopep();
    Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder sen_ಅಭಿವ್ಯಕ್ತಿ_Builder{scopeTopp};

    // Step 6: Create 'settle' region that restores the combinational invariant
    createSettle(netlistp, staticp, sen_ಅಭಿವ್ಯಕ್ತಿ_Builder, logicClasses, cgRefBindings);
    ಆದರೆ (v3Global.opt.stats()) V3Stats::statsStage("sched-settle");

    // Step 7: Partition the clocked and combinational (including hybrid) logic into pre/act/nba.
    // All clocks (signals referenced in an AstSenTree) generated via a blocking assignment
    // (including combinationally generated signals) are computed within the act region.
    LogicRegions logicRegions
        = ವಿಭಜನೆ(logicClasses.m_clocked, logicClasses.m_comb, logicClasses.m_hybrid);
    logicRegions.m_obs = logicClasses.m_observed;
    logicRegions.m_react = logicClasses.m_reactive;
    ಆದರೆ (v3Global.opt.stats()) {
        ಕೂಡಿಸು_ಗಾತ್ರ_Stat("size of region: Active Pre", logicRegions.m_pre);
        ಕೂಡಿಸು_ಗಾತ್ರ_Stat("size of region: Active", logicRegions.m_act);
        ಕೂಡಿಸು_ಗಾತ್ರ_Stat("size of region: NBA", logicRegions.m_nba);
        ಕೂಡಿಸು_ಗಾತ್ರ_Stat("size of region: Observed", logicRegions.m_obs);
        ಕೂಡಿಸು_ಗಾತ್ರ_Stat("size of region: Reactive", logicRegions.m_react);
        V3Stats::statsStage("sched-partition");
    }

    // Step 8: Replicate combinational logic
    LogicReplicas logicReplicas = replicateLogic(logicRegions);
    ಆದರೆ (v3Global.opt.stats()) {
        ಕೂಡಿಸು_ಗಾತ್ರ_Stat("size of replicated logic: Input", logicReplicas.m_ico);
        ಕೂಡಿಸು_ಗಾತ್ರ_Stat("size of replicated logic: Active", logicReplicas.m_act);
        ಕೂಡಿಸು_ಗಾತ್ರ_Stat("size of replicated logic: NBA", logicReplicas.m_nba);
        ಕೂಡಿಸು_ಗಾತ್ರ_Stat("size of replicated logic: Observed", logicReplicas.m_obs);
        ಕೂಡಿಸು_ಗಾತ್ರ_Stat("size of replicated logic: Reactive", logicReplicas.m_react);
        V3Stats::statsStage("sched-replicate");
    }

    // Step 9: Create the input combinational logic
    createIcoRegion(netlistp, staticp, sen_ಅಭಿವ್ಯಕ್ತಿ_Builder, logicReplicas.m_ico, virtIfaceTriggers,
                    cgRefBindings);
    ಆದರೆ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-ico");

    // Step 10: Create the triggers
    AstVarScope* ಸ್ಥಿರಾಂಕ dpiExportTriggerVscp = netlistp->dpiExportTriggerp();
    netlistp->dpiExportTriggerp(ಶೂನ್ಯಸೂಚಕ);  // Finished with this here

    // We may have an extra trigger for variable updated in DPI exports
    TriggerKit::ExtraTriggers extraTriggers;
    ಸ್ಥಿರಾಂಕ uint32_t dpi_Export_Trigger_ಸೂಚ್ಯಂಕ = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    ಸ್ಥಿರಾಂಕ uint32_t ಮೊದಲ_Vif_Trigger_ಸೂಚ್ಯಂಕ = extraTriggers.ಗಾತ್ರ_2();
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + ನಮೂದು.m_ifacep->name() + "."
                               + ನಮೂದು.m_memberp->name());
    }

    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& preTreeps = get_Sen_Trees_ಬಳಸಿದ_By({&logicRegions.m_pre});
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& senTreeps = get_Sen_Trees_ಬಳಸಿದ_By({&logicRegions.m_act,  //
                                               &logicRegions.m_nba,  //
                                               &logicRegions.m_obs,  //
                                               &logicRegions.m_react,  //
                                               &timingKit.m_lbs});
    ಸ್ಥಿರಾಂಕ TriggerKit trigKit
        = TriggerKit::create(netlistp, staticp, sen_ಅಭಿವ್ಯಕ್ತಿ_Builder, preTreeps, senTreeps, "act",
                             extraTriggers, ಸುಳ್ಳು, v3Global.ಬಳಸುತ್ತದೆ_Timing());

    // Add post updates from the timing kit
    ಆದರೆ (timingKit.m_postUpdates) trigKit.compBasep()->ಕೂಡಿಸು_Stmtsp(timingKit.m_postUpdates);

    ಆದರೆ (dpiExportTriggerVscp) {
        trigKit.ಕೂಡಿಸು_Extra_Trigger_Assignment(dpiExportTriggerVscp, dpi_Export_Trigger_ಸೂಚ್ಯಂಕ);
    }
    ಕೂಡಿಸು_Virt_Iface_Trigger_Assignments(netlistp, staticp, virtIfaceTriggers, ಮೊದಲ_Vif_Trigger_ಸೂಚ್ಯಂಕ,
                                   trigKit);
    ಆದರೆ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-triggers");

    // Note: Experiments so far show that running the Act (or Ico) regions on
    // multiple threads is always a net loss, so only use multi-threading for
    // NBA for now. This can be revised if evidence is available that it would
    // be beneficial

    // Step 11: Create the 'act' region evaluation function

    // Add the trigger indices of the input logic
    ಕೂಡಿಸು_Trigger_Indices(logicRegions.m_pre, trigKit, ನಿಜ);
    ಕೂಡಿಸು_Trigger_Indices(logicRegions.m_act, trigKit, ಸುಳ್ಳು);
    ಕೂಡಿಸು_Trigger_Indices(logicReplicas.m_act, trigKit, ಸುಳ್ಳು);
    ಕೂಡಿಸು_Trigger_Indices(timingKit.m_lbs, trigKit, ಸುಳ್ಳು);

    // External triggers of variables. Trigger indices are the same in all regions, so share.
    ಸ್ಥಿರಾಂಕ std::map<ಸ್ಥಿರಾಂಕ AstVarScope*, std::ಸದಿಶ<uint32_t>> timingTriggers
        = timingKit.externalTriggerIndices(trigKit);
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& vif_Vscp_ಗಮ್ಯ_Trig = virtIfaceTriggers.make_Vscp_ಗಮ್ಯ_Trig_Map(trigKit, ಮೊದಲ_Vif_Trigger_ಸೂಚ್ಯಂಕ);
    ಸ್ಥಿರಾಂಕ V3_ಕ್ರಮ::ExternalTriggersProvider externalTriggers
        = [&](ಸ್ಥಿರಾಂಕ AstVarScope* vscp, std::ಗಣ<uint32_t>& triggers) {
              ಸ್ವಯಂಚಾಲಿತ it = timingTriggers.ಕಂಡುಹಿಡಿ(vscp);
              ಆದರೆ (it != timingTriggers.ಕೊನೆ())
                  triggers.ಸೇರಿಸು(it->ಎರಡನೇ.begin(), it->ಎರಡನೇ.ಕೊನೆ());
              ಆದರೆ (vscp->varp()->isWrittenByDpi()) {
                  triggers.ಸೇರಿಸು(trigKit.extra_Trigger_ಸೂಚ್ಯಂಕ(dpi_Export_Trigger_ಸೂಚ್ಯಂಕ));
              }
              ಆದರೆ (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                  ಕೂಡಿಸು_Triggered_Iface(vscp, vif_Vscp_ಗಮ್ಯ_Trig, virtIfaceTriggers, triggers);
              }
          };

    AstCFunc* ಸ್ಥಿರಾಂಕ actFuncp
        = V3_ಕ್ರಮ::ಕ್ರಮ({&logicRegions.m_pre, &logicRegions.m_act, &logicReplicas.m_act}, trigKit,
                         trigKit.vscp(), cgRefBindings, "act", ಸುಳ್ಳು, ಸುಳ್ಳು, externalTriggers);
    util::ವಿಭಜಿಸು_Check(actFuncp);
    ಆದರೆ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-act");

    ಸ್ಥಿರಾಂಕ EvalKit actKit{trigKit.vscp(), actFuncp};

    // Orders a region's logic and creates the region eval function
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕ್ರಮ = [&](ಸ್ಥಿರಾಂಕ std::string& name,
                           ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<V3Sched::LogicByScope*>& logic) -> EvalKit {
        UINFO(2, "Scheduling " << name << " #logic = " << logic.ಗಾತ್ರ_2());
        AstVarScope* ಸ್ಥಿರಾಂಕ trigVscp = trigKit.newTrigVec(name);
        // Add the trigger indices of the input logic
        ಗಾಗಿ (LogicByScope* lbs : logic) ಕೂಡಿಸು_Trigger_Indices(*lbs, trigKit, ಸುಳ್ಳು);

        AstCFunc* ಸ್ಥಿರಾಂಕ funcp
            = V3_ಕ್ರಮ::ಕ್ರಮ(logic, trigKit, trigVscp, cgRefBindings, name,
                             name == "nba" && v3Global.opt.mtasks(), ಸುಳ್ಳು, externalTriggers);

        ಹಿಂತಿರುಗಿ {trigVscp, funcp};
    };

    // Step 12: Create the 'nba' region evaluation function
    ಸ್ಥಿರಾಂಕ EvalKit nbaKit = ಕ್ರಮ("nba", {&logicRegions.m_nba, &logicReplicas.m_nba});
    util::ವಿಭಜಿಸು_Check(nbaKit.m_funcp);
    ಆದರೆ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-nba");

    // Orders a region's logic and creates the region eval function (only if there is any logic in
    // the region)
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕ್ರಮ_If_Non_ಖಾಲಿಗಳು
        = [&](ಸ್ಥಿರಾಂಕ std::string& name, ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<LogicByScope*>& logic) -> EvalKit {
        ಆದರೆ (logic[0]->ಖಾಲಿಗಳು())
            ಹಿಂತಿರುಗಿ {};  // if region is empty, replica is supposed to be empty as well
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& kit = ಕ್ರಮ(name, logic);
        ಆದರೆ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-" + name);
        ಹಿಂತಿರುಗಿ kit;
    };

    // Step 13: Create the 'obs' region evaluation function
    ಸ್ಥಿರಾಂಕ EvalKit obsKit = ಕ್ರಮ_If_Non_ಖಾಲಿಗಳು("obs", {&logicRegions.m_obs, &logicReplicas.m_obs});

    // Step 14: Create the 'react' region evaluation function
    ಸ್ಥಿರಾಂಕ EvalKit reactKit
        = ಕ್ರಮ_If_Non_ಖಾಲಿಗಳು("react", {&logicRegions.m_react, &logicReplicas.m_react});

    // Step 15: Create the 'postponed' region evaluation function
    createPostponed(netlistp, logicClasses);

    // Step 16: Populate the eval entry point function of each region of a time step
    createEval(netlistp, trigKit, actKit, nbaKit, obsKit, reactKit, timingKit);

    // Step 17: Add neccessary evaluation before awaits
    ಆದರೆ (AstCCall* ಸ್ಥಿರಾಂಕ readyp = timingKit.createReady(trigKit.vscp())) {
        staticp->ಕೂಡಿಸು_Stmtsp(readyp->makeStmt());
        beforeTrigVisitor(netlistp, sen_ಅಭಿವ್ಯಕ್ತಿ_Builder, trigKit);
    } ಇಲ್ಲದಿದ್ದರೆ {
        // beforeTrigVisitor clears Sentree pointers in AstCAwaits (as these sentrees will get
        // deleted later) if there was no need to call it, SenTrees have to be cleaned manually
        netlistp->foreach([](AstCAwait* ಸ್ಥಿರಾಂಕ cAwaitp) { cAwaitp->clearSentreep(); });
    }
    ಆದರೆ (AstVarScope* ಸ್ಥಿರಾಂಕ trigAccp = trigKit.vscAccp()) {
        // Copy trigger vector to accumulator at the end of static initialziation so,
        // triggers fired during initialization persist to the first resume.
        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ trigAccDTypep
            = VN_AS(trigAccp->dtypep(), UnpackArrayDType);
        UASSERT_OBJ(
            trigAccDTypep->ಬಲ() == 0, trigAccp,
            "Expected that trigger vector and accumulator start elements enumeration from 0");
        UASSERT_OBJ(trigAccDTypep->ಎಡ() >= 0, trigAccp,
                    "Expected that trigger vector and accumulator has no negative indexes");
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = trigAccp->fileline();
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = netlistp->ತುದಿ_Scopep()->create_ತಾತ್ಕಾಲಿಕ("__Vi", 32);
        AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{flp};
        loopp->ಕೂಡಿಸು_Stmtsp(
            ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp,
                          ಹೊಸ AstArraySel{flp, ಹೊಸ AstVarRef{flp, trigAccp, VAccess::WRITE},
                                          ಹೊಸ AstVarRef{flp, vscp, VAccess::READ}},
                          ಹೊಸ AstArraySel{flp, ಹೊಸ AstVarRef{flp, actKit.m_vscp, VAccess::READ},
                                          ಹೊಸ AstVarRef{flp, vscp, VAccess::READ}}});
        loopp->ಕೂಡಿಸು_Stmtsp(util::incrementVar(vscp));
        loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{
            flp, loopp,
            ಹೊಸ AstLte{flp, ಹೊಸ AstVarRef{flp, vscp, VAccess::READ},
                       ಹೊಸ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, 32,
                                    static_cast<uint32_t>(trigAccDTypep->ಎಡ())}}});
        staticp->ಕೂಡಿಸು_Stmtsp(loopp);
    }

    // Step 18: Clean up
    netlistp->clear_Stl_ಮೊದಲ_Iterationp();

    ಆದರೆ (v3Global.opt.stats()) {
        // A sample() call resolved to the union over its covergroup's constructions reads more
        // than it can, which orders it against more logic than necessary
        V3Stats::ಕೂಡಿಸು_Stat("Scheduling, covergroup ref sample calls, per instance",
                         cgRefBindings.ಅಂಕೆ_Exact_Calls());
        V3Stats::ಕೂಡಿಸು_Stat("Scheduling, covergroup ref sample calls, per type",
                         cgRefBindings.ಅಂಕೆ_Union_Calls());
    }

    // Haven't split static initializer yet
    util::ವಿಭಜಿಸು_Check(staticp);

    // Record eval stats
    netlistp->ಕೂಡಿಸು_Eval_Stats("sched");

    // Dump
    V3Global::dump_Check_Global_ವೃಕ್ಷ("sched", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

}  // namespace V3Sched
