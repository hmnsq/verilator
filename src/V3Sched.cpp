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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Sched.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCBase.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitV.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Order.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3SenExprBuilder.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ਨਾਮਾਕਾਸ਼ V3Sched {

ਨਾਮਾਕਾਸ਼ {

//============================================================================
// Utility functions

std::ਸਦਿਸ਼<ਅਚਲ Ast_Sen_ਰੁੱਖ*> get_Sen_Trees_ਵਰਤਿਆ_By(ਅਚਲ std::ਸਦਿਸ਼<ਅਚਲ LogicByScope*>& lbsps) {
    ਅਚਲ VNUser1_ਵਿੱਚ_Use user1_ਵਿੱਚ_Use;
    std::ਸਦਿਸ਼<ਅਚਲ Ast_Sen_ਰੁੱਖ*> ਨਤੀਜਾ;
    ਲਈ (ਅਚਲ LogicByScope* ਅਚਲ lbsp : lbsps) {
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : *lbsp) {
            Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਇੰਦਰਾਜ.m_activep;
            Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep = activep->sentreep();
            ਜੇ (senTreep->user1_ਸੈੱਟ_Once()) ਜਾਰੀ;
            ਜੇ (senTreep->hasClocked() || senTreep->hasHybrid()) ਨਤੀਜਾ.ਧੱਕੋ_ਪਿੱਠ(senTreep);
        }
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਖਾਲੀ ਜੋੜੋ_Trigger_Indices(LogicByScope& lbs, ਅਚਲ TriggerKit& trigKit, ਤਾਰਕਿਕ pre) {
    ਲਈ (LogicByScope::ਇੰਦਰਾਜ_2& ਇੰਦਰਾਜ : lbs) {
        Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep = ਇੰਦਰਾਜ.m_activep->sentreep();
        ਜੇ (senTreep->hasCombo()) ਜਾਰੀ;
        ਇੰਦਰਾਜ.m_triggers = trigKit.triggerIndices(senTreep, pre);
    }
}

// Add the VIF triggers that a given VarScope should be sensitive to.
// Case 2 (non-virtual interface read): sensitive to that specific VarScope's trigger only
// Case 3 (virtual interface read): sensitive to all member triggers of the same interface type
ਖਾਲੀ ਜੋੜੋ_Triggered_Iface(ਅਚਲ AstVarScope* vscp, ਅਚਲ VirtIfaceTriggers::VscpTrigMap& vscp_ਮੰਜ਼ਿਲ_Trig,
                       ਅਚਲ VirtIfaceTriggers& virtIfaceTriggers, std::ਸੈੱਟ<uint32_t>& triggers) {
    ਜੇ (vscp->varp()->isVirtIface()) {
        // Virtual interface variable -- sensitive to all member triggers of this interface type
        ਅਚਲ AstIface* ਅਚਲ ifacep = VN_AS(vscp->varp()->dtypep(), IfaceRefDType)->ifacep();
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : virtIfaceTriggers.m_triggers) {
            ਜੇ (ਇੰਦਰਾਜ.m_ifacep == ifacep) {
                ਅਚਲ ਸਵੈਚਾਲਿਤ it = vscp_ਮੰਜ਼ਿਲ_Trig.ਲੱਭੋ(ਇੰਦਰਾਜ.m_vscp);
                ਜੇ (it != vscp_ਮੰਜ਼ਿਲ_Trig.ਸਿਰਾ()) triggers.ਪਾਓ(it->ਦੂਜਾ);
            }
        }
    } ਨਹੀਂਤਾਂ {
        // Non-virtual interface member -- sensitive to this VarScope's trigger only
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = vscp_ਮੰਜ਼ਿਲ_Trig.ਲੱਭੋ(vscp);
        ਜੇ (it != vscp_ਮੰਜ਼ਿਲ_Trig.ਸਿਰਾ()) triggers.ਪਾਓ(it->ਦੂਜਾ);
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
ਖਾਲੀ createEvalRegion(
    AstNetlist* netlistp,  //
    VEval eval,  // The entry point of the current region
    // Index of the region's 'first iteration' extra trigger, if it has one, otherwise ignored
    uint32_t ਪਹਿਲਾ_Iter_Trigger,
    ਅਚਲ TriggerKit& trigKit,  // The trigger kit
    AstVarScope* trigp,  // The trigger vector - may be nullptr if no triggers or using 'condp'
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp,  // Explicit condition that must be true to run 'phaseWorkp'
    Ast_ਗੰਢ_Stmt* phasePrepp,  // Prep statements run before checking triggers
    Ast_ਗੰਢ_Stmt* phaseWorkp,  // The work to do if anything triggered
    // Extra statements to run after the work, even if no triggers fired. This function is
    // passed a variable, which must be set to true if we must continue and loop again,
    // and must be unmodified otherwise.
    std::function<Ast_ਗੰਢ_Stmt*(AstVarScope*)> phaseExtra = [](AstVarScope*) { ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ; }  //
) {
    UASSERT(!trigp || !condp, "Cannot use both 'trigp' and 'condp' in 'createEvalRegion'");
    UASSERT(!eval.ਪਹਿਲਾ_Iteration() || trigp,
            "Region without triggers cannot need a first iteration flag");

    // All work is under a trigger or condition, so with neither the region has
    // nothing to evaluate, and what we create below reduces to a no-op function.

    ਅਚਲ std::string ਟੈਗ = eval.ਟੈਗ();
    Ast_ਸਿਖਰ_Scope* ਅਚਲ ਸਿਖਰ_Scopep = netlistp->ਸਿਖਰ_Scopep();
    File_ਸਤਰ* ਅਚਲ flp = netlistp->fileline();

    // Populate the trigger dump entry point function
    ਜੇ (trigp) {
        netlistp->dumpTriggersFuncp(eval)->ਜੋੜੋ_Stmtsp(trigKit.newDumpCall(trigp, ਟੈਗ, ਝੂਠ));
    }

    AstCFunc* ਅਚਲ funcp = netlistp->evalFuncp(eval);

    // A flag is passed from the run-time eval loop if this is the first iteration of the
    // current loop
    ਜੇ (eval.ਪਹਿਲਾ_Iteration()) {
        AstVarScope* ਅਚਲ ਪਹਿਲਾ_Iter_Argp = util::new_ਦਲੀਲ(funcp, netlistp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(),
                                                             "firstIteration", VDirection::ਨਿਵੇਸ਼);

        // Set the region's 'first iteration' trigger straight from the argument
        funcp->ਜੋੜੋ_Stmtsp(trigKit.newExtraTriggerAssignment(ਪਹਿਲਾ_Iter_Argp, ਪਹਿਲਾ_Iter_Trigger));

        // Only 'stl' also needs a module level flag, for design logic that reads
        // it directly (see V3Timing). Those reads can be anywhere in the design,
        // hence module level. Always created, even if nothing reads it.
        // TODO: get rid of this special case
        ਜੇ (eval == VEval::STL) {
            AstVarScope* ਅਚਲ ਪਹਿਲਾ_Iterp = netlistp->stl_ਪਹਿਲਾ_Iterationp();
            ਪਹਿਲਾ_Iterp->varp()->noReset(ਸੱਚ);
            ਪਹਿਲਾ_Iterp->varp()->is_ਅੰਦਰੂਨੀ(ਸੱਚ);
            funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, ਪਹਿਲਾ_Iterp, VAccess::WRITE},
                                           ਨਵਾਂ AstVarRef{flp, ਪਹਿਲਾ_Iter_Argp, VAccess::READ}});
        }
    }

    {
        // Add the preparatory statements
        funcp->ਜੋੜੋ_Stmtsp(phasePrepp);

        // The execute flag
        AstVarScope* ਅਚਲ executeFlagp = ਸਿਖਰ_Scopep->create_ਆਰਜ਼ੀ("__V" + ਟੈਗ + "Execute", 1);
        executeFlagp->varp()->noReset(ਸੱਚ);

        // If there is work in this region, execute it if any triggers fired
        ਜੇ (phaseWorkp) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = ਨਵਾਂ AstVarRef{flp, executeFlagp, VAccess::WRITE};
            // If using explicit condition, that directly determines whether to execute,
            // otherwise check if any triggers are fired
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = condp ? condp : trigKit.new_Any_ਸੈੱਟ_Call(trigp);
            funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, lhsp, rhsp});

            // Add the work
            AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, ਨਵਾਂ AstVarRef{flp, executeFlagp, VAccess::READ}};
            ifp->ਜੋੜੋ_Thensp(phaseWorkp);
            funcp->ਜੋੜੋ_Stmtsp(ifp);
        }

        // Construct the extra statements
        Ast_ਗੰਢ_Stmt* ਅਚਲ extraWorkp = phaseExtra(executeFlagp);
        ਜੇ (extraWorkp) funcp->ਜੋੜੋ_Stmtsp(extraWorkp);

        // The function returns true iff it did run work
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ retp
            = phaseWorkp || extraWorkp
                  ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstVarRef{flp, executeFlagp, VAccess::READ})
                  : static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
        funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCReturn{flp, retp});
    }
}

//============================================================================
// Collect and classify all logic in the design

LogicClasses gatherLogicClasses(AstNetlist* netlistp) {
    LogicClasses ਨਤੀਜਾ;

    netlistp->foreach([&](AstScope* scopep) {
        scopep->foreach([&](Ast_ਸਰਗਰਮ* activep) {
            Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep = activep->sentreep();
            ਜੇ (senTreep->hasStatic()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "static initializer with additional sensitivities");
                ਨਤੀਜਾ.m_static.ਜੋੜੋ(scopep, activep);
            } ਨਹੀਂਤਾਂ ਜੇ (senTreep->hasInitial()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'initial' logic with additional sensitivities");
                ਨਤੀਜਾ.m_initial.ਜੋੜੋ(scopep, activep);
            } ਨਹੀਂਤਾਂ ਜੇ (senTreep->hasFinal()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'final' logic with additional sensitivities");
                ਨਤੀਜਾ.m_final.ਜੋੜੋ(scopep, activep);
            } ਨਹੀਂਤਾਂ ਜੇ (senTreep->hasCombo()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "combinational logic with additional sensitivities");
                ਜੇ (VN_IS(activep->stmtsp(), AlwaysPostponed)) {
                    ਨਤੀਜਾ.m_postponed.ਜੋੜੋ(scopep, activep);
                } ਨਹੀਂਤਾਂ {
                    ਨਤੀਜਾ.m_comb.ਜੋੜੋ(scopep, activep);
                }
            } ਨਹੀਂਤਾਂ {
                UASSERT_OBJ(senTreep->hasClocked(), activep, "What else could it be?");
                ਜੇ (VN_IS(activep->stmtsp(), AlwaysObserved)) {
                    ਨਤੀਜਾ.m_observed.ਜੋੜੋ(scopep, activep);
                } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(activep->stmtsp(), AlwaysReactive)) {
                    ਨਤੀਜਾ.m_reactive.ਜੋੜੋ(scopep, activep);
                } ਨਹੀਂਤਾਂ {
                    ਨਤੀਜਾ.m_clocked.ਜੋੜੋ(scopep, activep);
                }
            }
        });
    });

    ਵਾਪਸ ਨਤੀਜਾ;
}

//============================================================================
// Simple ordering in source order

ਖਾਲੀ ਕ੍ਰਮ_Sequentially(AstCFunc* funcp, ਅਚਲ LogicByScope& lbs) {
    // Create new subfunc for scope
    ਅਚਲ ਸਵੈਚਾਲਿਤ createNewSubFuncp = [&](AstScope* ਅਚਲ scopep) {
        ਅਚਲ string subName{funcp->name() + "__" + scopep->nameDotless()};
        AstCFunc* ਅਚਲ subFuncp = ਨਵਾਂ AstCFunc{scopep->fileline(), subName, scopep};
        subFuncp->isLoose(ਸੱਚ);
        subFuncp->isConst(ਝੂਠ);
        subFuncp->decl_ਨਿੱਜੀ(ਸੱਚ);
        subFuncp->slow(funcp->slow());
        scopep->ਜੋੜੋ_Blocksp(subFuncp);
        // Call it from the top function
        funcp->ਜੋੜੋ_Stmtsp(util::callVoidFunc(subFuncp));
        ਵਾਪਸ subFuncp;
    };
    ਅਚਲ VNUser1_ਵਿੱਚ_Use user1_ਵਿੱਚ_Use;  // AstScope -> AstCFunc: the sub-function for the scope
    ਅਚਲ VNUser2_ਵਿੱਚ_Use user2_ਵਿੱਚ_Use;  // AstScope -> int: sub-function counter used for names
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : lbs) {
        AstScope* ਅਚਲ scopep = ਇੰਦਰਾਜ.m_scopep;
        Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਇੰਦਰਾਜ.m_activep;
        // Create a sub-function per scope so we can V3Combine them later
        ਜੇ (!scopep->user1p()) scopep->user1p(createNewSubFuncp(scopep));
        // Add statements to sub-function
        ਲਈ (Ast_ਗੰਢ *logicp = activep->stmtsp(), *nextp; logicp; logicp = nextp) {
            ਸਵੈਚਾਲਿਤ* subFuncp = VN_AS(scopep->user1p(), CFunc);
            nextp = logicp->nextp();
            ਜੇ (Ast_ਗੰਢ_Procedure* ਅਚਲ procp = VN_CAST(logicp, ਗੰਢ_Procedure)) {
                ਜੇ (Ast_ਗੰਢ* bodyp = procp->stmtsp()) {
                    bodyp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
                    // If the process is suspendable, we need a separate function (a coroutine)
                    ਜੇ (procp->isSuspendable()) {
                        funcp->slow(ਝੂਠ);
                        subFuncp = createNewSubFuncp(scopep);
                        subFuncp->name(subFuncp->name() + "__Vtiming__"
                                       + cvt_ਮੰਜ਼ਿਲ_Str(scopep->user2Inc()));
                        subFuncp->rtnType("VlCoroutine");
                        ਜੇ (VN_IS(procp, Always)) {
                            subFuncp->slow(ਝੂਠ);
                            File_ਸਤਰ* ਅਚਲ flp = procp->fileline();
                            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = ਨਵਾਂ AstCExpr{
                                flp, "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish())", 1};
                            AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{flp};
                            loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{flp, loopp, condp});
                            loopp->ਜੋੜੋ_Stmtsp(bodyp);
                            bodyp = loopp;
                        }
                    }
                    subFuncp->ਜੋੜੋ_Stmtsp(bodyp);
                    ਜੇ (procp->needProcess()) subFuncp->ਸੈੱਟ_Need_Process();
                    // A coroutine holds a single process, so it is complete here
                    ਜੇ (procp->isSuspendable()) util::ਵੰਡੋ_Check(subFuncp);
                }
            } ਨਹੀਂਤਾਂ {
                logicp->unlink_Fr_ਪਿੱਠ();
                subFuncp->ਜੋੜੋ_Stmtsp(logicp);
            }
        }
        ਜੇ (activep->backp()) activep->unlink_Fr_ਪਿੱਠ();
        VL_DO_DANGLING(activep->delete_ਰੁੱਖ(), activep);
    }
    // Split sub-functions once complete
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : lbs) {
        AstScope* ਅਚਲ scopep = ਇੰਦਰਾਜ.m_scopep;
        ਜੇ (AstCFunc* ਅਚਲ subFuncp = VN_AS(scopep->user1p(), CFunc)) {
            util::ਵੰਡੋ_Check(subFuncp);
            scopep->user1p(ਖਾਲੀ_ਸੂਚਕ);
        }
    }
}

//============================================================================
// Create simply ordered functions

AstCFunc* createStatic(AstNetlist* netlistp, ਅਚਲ LogicClasses& logicClasses) {
    AstCFunc* ਅਚਲ funcp = netlistp->evalFuncp(VEval::STATIC);

    ਅਚਲ LogicByScope& orig = logicClasses.m_static;
    ਜੇ (orig.ਮਾਪ() <= 1) {
        ਕ੍ਰਮ_Sequentially(funcp, orig);
        ਵਾਪਸ funcp;
    }

    // Level-based module sorting can reorder packages so that an importing
    // package runs before the imported one.  Re-sort package entries by source
    // file position to restore compilation order (IEEE 1800-2023 26.3).
    std::ਸਦਿਸ਼<ਮਾਪ_t> indices(orig.ਮਾਪ());
    ਲਈ (ਮਾਪ_t i = 0; i < orig.ਮਾਪ(); ++i) indices[i] = i;
    std::stable_ਛਾਂਟੋ(indices.begin(), indices.ਸਿਰਾ(), [&](ਮਾਪ_t a, ਮਾਪ_t b) {
        ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ modA = orig[a].m_scopep->modp();
        ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ modB = orig[b].m_scopep->modp();
        ਅਚਲ ਤਾਰਕਿਕ isPkgA = VN_IS(modA, Package);
        ਅਚਲ ਤਾਰਕਿਕ isPkgB = VN_IS(modB, Package);
        ਜੇ (isPkgA != isPkgB) ਵਾਪਸ isPkgA;  // Packages before non-packages
        ਜੇ (isPkgA && isPkgB) {
            // Sort packages by source file position (compilation order)
            ਵਾਪਸ modA->fileline()->operatorCompare(*modB->fileline()) < 0;
        }
        ਵਾਪਸ ਝੂਠ;  // Both non-package: preserve original order
    });
    LogicByScope sorted;
    ਲਈ (ਅਚਲ ਮਾਪ_t i : indices) sorted.ਜੋੜੋ(orig[i].m_scopep, orig[i].m_activep);

    ਕ੍ਰਮ_Sequentially(funcp, sorted);
    ਵਾਪਸ funcp;  // Not splitting yet as it is not final
}

ਖਾਲੀ createInitial(AstNetlist* netlistp, ਅਚਲ LogicClasses& logicClasses) {
    AstCFunc* ਅਚਲ funcp = netlistp->evalFuncp(VEval::INITIAL);
    ਕ੍ਰਮ_Sequentially(funcp, logicClasses.m_initial);
    util::ਵੰਡੋ_Check(funcp);
}

ਖਾਲੀ createPostponed(AstNetlist* netlistp, ਅਚਲ LogicClasses& logicClasses) {
    AstCFunc* ਅਚਲ funcp = netlistp->evalFuncp(VEval::POSTPONED);
    ਕ੍ਰਮ_Sequentially(funcp, logicClasses.m_postponed);
    util::ਵੰਡੋ_Check(funcp);
}

ਖਾਲੀ createFinal(AstNetlist* netlistp, ਅਚਲ LogicClasses& logicClasses) {
    AstCFunc* ਅਚਲ funcp = netlistp->evalFuncp(VEval::FINAL);
    ਕ੍ਰਮ_Sequentially(funcp, logicClasses.m_final);
    util::ਵੰਡੋ_Check(funcp);
}

//============================================================================
// Helper that creates virtual interface value-change triggers

ਖਾਲੀ ਜੋੜੋ_Virt_Iface_Trigger_Assignments(AstNetlist* netlistp, AstCFunc* initFuncp,
                                    ਅਚਲ VirtIfaceTriggers& virtIfaceTriggers,
                                    uint32_t ਪਹਿਲਾ_ਸੂਚਕਾਂਕ, ਅਚਲ TriggerKit& trigKit) {
    uint32_t ਸੂਚਕਾਂਕ = ਪਹਿਲਾ_ਸੂਚਕਾਂਕ;
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : virtIfaceTriggers.m_triggers) {
        trigKit.ਜੋੜੋ_ਮੁੱਲ_Change_Trigger_Assignment(netlistp, initFuncp, ਇੰਦਰਾਜ.m_vscp, ਸੂਚਕਾਂਕ);
        ++ਸੂਚਕਾਂਕ;
    }
}

// Order the combinational logic to create the 'stl' region
ਖਾਲੀ createSettle(AstNetlist* netlistp, AstCFunc* ਅਚਲ initFuncp, Sen_ਵਿਅੰਜਕ_Builder& sen_ਵਿਅੰਜਕ_Bulider,
                  LogicClasses& logicClasses, ਅਚਲ CovergroupRefBindings& cgRefBindings) {
    // Clone, because ordering is destructive, but we still need them for the other regions
    LogicByScope comb = logicClasses.m_comb.clone();
    LogicByScope hybrid = logicClasses.m_hybrid.clone();

    // We have an extra trigger denoting this is the first iteration of the settle loop
    TriggerKit::ExtraTriggers extraTriggers;
    ਅਚਲ uint32_t ਪਹਿਲਾ_Iteration_Trigger = extraTriggers.allocate("first iteration");

    // Gather the relevant sensitivity expressions and create the trigger kit
    ਅਚਲ ਸਵੈਚਾਲਿਤ& senTreeps = get_Sen_Trees_ਵਰਤਿਆ_By({&comb, &hybrid});
    ਅਚਲ TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_ਵਿਅੰਜਕ_Bulider, {},
                                                  senTreeps, "stl", extraTriggers, ਸੱਚ, ਝੂਠ);

    // Add trigger indices (comb has none, so only do the hybrid)
    ਜੋੜੋ_Trigger_Indices(hybrid, trigKit, ਝੂਠ);

    // First trigger is for pure combinational triggers (first iteration)
    ਅਚਲ uint32_t ਨਿਵੇਸ਼_ਬਦਲਿਆ = trigKit.extra_Trigger_ਸੂਚਕਾਂਕ(ਪਹਿਲਾ_Iteration_Trigger);

    // Create and the body function
    AstCFunc* ਅਚਲ stlFuncp = V3_ਕ੍ਰਮ::ਕ੍ਰਮ(
        {&comb, &hybrid}, trigKit, trigKit.vscp(), cgRefBindings, "stl", ਝੂਠ, ਸੱਚ,
        [=](ਅਚਲ AstVarScope*, std::ਸੈੱਟ<uint32_t>& triggers) { triggers.ਪਾਓ(ਨਿਵੇਸ਼_ਬਦਲਿਆ); });
    util::ਵੰਡੋ_Check(stlFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::STL, ਪਹਿਲਾ_Iteration_Trigger, trigKit,
        // Use trigger
        trigKit.vscp(), ਖਾਲੀ_ਸੂਚਕ,
        // Explicit condition
        // Prep statements: Compute the current 'stl' triggers
        [&trigKit] {
            Ast_ਗੰਢ_Stmt* ਅਚਲ stmtp = trigKit.new_Comp_ਅਧਾਰ_Call();
            ਜੇ (stmtp) stmtp->ਜੋੜੋ_ਅਗਲਾ(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), ਸੱਚ));
            ਵਾਪਸ stmtp;
        }(),
        // Work statements: Invoke the 'stl' function
        util::callVoidFunc(stlFuncp));
}

//============================================================================
// Order the replicated combinational logic to create the 'ico' region

ਖਾਲੀ createIcoRegion(AstNetlist* netlistp, AstCFunc* ਅਚਲ initFuncp,
                     Sen_ਵਿਅੰਜਕ_Builder& sen_ਵਿਅੰਜਕ_Builder, LogicByScope& logic,
                     ਅਚਲ VirtIfaceTriggers& virtIfaceTriggers,
                     ਅਚਲ CovergroupRefBindings& cgRefBindings) {
    // SystemC only: Any top level inputs feeding a combinational logic must be marked,
    // so we can make them sc_sensitive
    ਜੇ (v3Global.opt.systemC()) {
        logic.foreachLogic([](Ast_ਗੰਢ* logicp) {
            logicp->foreach([](AstVarRef* refp) {
                ਜੇ (refp->access().isWriteOnly()) ਵਾਪਸ;
                AstVarScope* ਅਚਲ vscp = refp->varScopep();
                ਜੇ (vscp->scopep()->is_ਸਿਖਰ() && vscp->varp()->isNonOutput()) {
                    vscp->varp()->scSensitive(ਸੱਚ);
                }
            });
        });
    }

    // We have some extra trigger denoting external conditions
    AstVarScope* ਅਚਲ dpiExportTriggerVscp = netlistp->dpiExportTriggerp();

    TriggerKit::ExtraTriggers extraTriggers;
    ਅਚਲ uint32_t ਪਹਿਲਾ_Iteration_Trigger = extraTriggers.allocate("first iteration");
    ਅਚਲ uint32_t dpi_Export_Trigger_ਸੂਚਕਾਂਕ = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    ਅਚਲ uint32_t ਪਹਿਲਾ_Vif_Trigger_ਸੂਚਕਾਂਕ = extraTriggers.ਮਾਪ();
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + ਇੰਦਰਾਜ.m_ifacep->name() + "."
                               + ਇੰਦਰਾਜ.m_memberp->name());
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
    std::unordered_map<ਅਚਲ AstVarScope*, Ast_Sen_ਰੁੱਖ*> inp2changedp;
    std::ਸਦਿਸ਼<Ast_Sen_ਰੁੱਖ*> icoChangeSenTreeps;
    ਜੇ (v3Global.opt.fIcoChangeDetect().isTrue()) {
        File_ਸਤਰ* ਅਚਲ flp = netlistp->fileline();
        AstScope* ਅਚਲ scopep = netlistp->ਸਿਖਰ_Scopep()->scopep();
        ਲਈ (AstVarScope* vscp = scopep->varsp(); vscp; vscp = VN_AS(vscp->nextp(), VarScope)) {
            // Only for top level ports, assume outputs don't change externally
            ਜੇ (!vscp->varp()->isPrimaryInish()) ਜਾਰੀ;
            // Don't do if written by the design - wouldn't update the change detect 'prev' value
            ਜੇ (vscp->varp()->icoMaybeWritten()) ਜਾਰੀ;
            // Don't do if forceable, as we can't see the actual value - this is belt and braces
            ਜੇ (vscp->varp()->isForced()) ਜਾਰੀ;
            // Can't handle unpacked arrays (they have special types when primary input)
            ਜੇ (VN_IS(vscp->dtypep()->skipRefp(), UnpackArrayDType)) ਜਾਰੀ;
            // Similarly to arrays, can't handle SystemC types
            ਜੇ (vscp->varp()->isSc()) ਜਾਰੀ;
            // Create a sen tree triggered when this input changes
            Ast_Sen_ਰੁੱਖ*& senTreepr = inp2changedp[vscp];
            UASSERT_OBJ(!senTreepr, vscp, "Duplicate input change detect trigger");
            AstVarRef* ਅਚਲ refp = ਨਵਾਂ AstVarRef{flp, vscp, VAccess::READ};
            Ast_Sen_ਵਸਤੂ* ਅਚਲ senItemp = ਨਵਾਂ Ast_Sen_ਵਸਤੂ{flp, VEdgeType::ET_ਬਦਲਿਆ, refp};
            senTreepr = ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, senItemp};
            icoChangeSenTreeps.ਧੱਕੋ_ਪਿੱਠ(senTreepr);
        }
    }
    V3Stats::ਜੋੜੋ_Stat("Scheduling, 'ico' change detect triggers", icoChangeSenTreeps.ਮਾਪ());

    // Gather the relevant sensitivity expressions and create the trigger kit
    std::ਸਦਿਸ਼<ਅਚਲ Ast_Sen_ਰੁੱਖ*> senTreeps = get_Sen_Trees_ਵਰਤਿਆ_By({&logic});
    senTreeps.ਪਾਓ(senTreeps.ਸਿਰਾ(), icoChangeSenTreeps.begin(), icoChangeSenTreeps.ਸਿਰਾ());
    ਅਚਲ TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_ਵਿਅੰਜਕ_Builder, {},
                                                  senTreeps, "ico", extraTriggers, ਝੂਠ, ਝੂਠ);
    std::ignore = sen_ਵਿਅੰਜਕ_Builder.getAndClearResults();

    ਜੇ (dpiExportTriggerVscp) {
        trigKit.ਜੋੜੋ_Extra_Trigger_Assignment(dpiExportTriggerVscp, dpi_Export_Trigger_ਸੂਚਕਾਂਕ);
    }
    ਜੋੜੋ_Virt_Iface_Trigger_Assignments(netlistp, initFuncp, virtIfaceTriggers, ਪਹਿਲਾ_Vif_Trigger_ਸੂਚਕਾਂਕ,
                                   trigKit);

    // Add trigger indices
    ਜੋੜੋ_Trigger_Indices(logic, trigKit, ਝੂਠ);

    // The 'first iteration' trigger for top level inputs
    ਅਚਲ uint32_t ਪਹਿਲਾ_Iter_Trigger = trigKit.extra_Trigger_ਸੂਚਕਾਂਕ(ਪਹਿਲਾ_Iteration_Trigger);

    ਅਚਲ ਸਵੈਚਾਲਿਤ& vif_Vscp_ਮੰਜ਼ਿਲ_Trig_Ico
        = virtIfaceTriggers.make_Vscp_ਮੰਜ਼ਿਲ_Trig_Map(trigKit, ਪਹਿਲਾ_Vif_Trigger_ਸੂਚਕਾਂਕ);

    // Create and Order the body function
    AstCFunc* ਅਚਲ icoFuncp = V3_ਕ੍ਰਮ::ਕ੍ਰਮ(
        {&logic}, trigKit, trigKit.vscp(), cgRefBindings, "ico", ਝੂਠ, ਝੂਠ,
        [&](ਅਚਲ AstVarScope* vscp, std::ਸੈੱਟ<uint32_t>& triggers) {
            AstVar* ਅਚਲ varp = vscp->varp();
            // If it has an explicit change detect trigger, use that,
            // otherwise fall back to using the 'first iteration' trigger
            ਸਵੈਚਾਲਿਤ it = inp2changedp.ਲੱਭੋ(vscp);
            ਜੇ (it != inp2changedp.ਸਿਰਾ()) {
                triggers.ਪਾਓ(trigKit.senItem2TrigIdx(it->ਦੂਜਾ->sensesp()));
            } ਨਹੀਂਤਾਂ ਜੇ (varp->isPrimaryInish() || varp->isSigUserRWPublic() || varp->sampled()) {
                triggers.ਪਾਓ(ਪਹਿਲਾ_Iter_Trigger);
            }
            // Add other triggers
            ਜੇ (varp->isWrittenByDpi())
                triggers.ਪਾਓ(trigKit.extra_Trigger_ਸੂਚਕਾਂਕ(dpi_Export_Trigger_ਸੂਚਕਾਂਕ));
            ਜੇ (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                ਜੋੜੋ_Triggered_Iface(vscp, vif_Vscp_ਮੰਜ਼ਿਲ_Trig_Ico, virtIfaceTriggers, triggers);
            }
        });
    util::ਵੰਡੋ_Check(icoFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::ICO, ਪਹਿਲਾ_Iteration_Trigger, trigKit,
        // Use trigger
        trigKit.vscp(), ਖਾਲੀ_ਸੂਚਕ,
        // Prep statements: Compute the current 'ico' triggers
        [&trigKit] {
            Ast_ਗੰਢ_Stmt* ਅਚਲ stmtp = trigKit.new_Comp_ਅਧਾਰ_Call();
            ਜੇ (stmtp) stmtp->ਜੋੜੋ_ਅਗਲਾ(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), ਸੱਚ));
            ਵਾਪਸ stmtp;
        }(),
        // Work statements: Invoke the 'ico' function
        util::callVoidFunc(icoFuncp));

    // Release temporary input change detect SenTrees
    ਲਈ (Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep : icoChangeSenTreeps)
        VL_DO_DANGLING(senTreep->delete_ਰੁੱਖ(), senTreep);
    icoChangeSenTreeps.clear();
}

//============================================================================
// EvalKit groups items that have to be passed to createEval() for a given eval region

ਬਣਤਰ EvalKit final {
    // The AstVarScope representing the region's trigger vector
    AstVarScope* ਅਚਲ m_vscp = ਖਾਲੀ_ਸੂਚਕ;
    // The AstCFunc that evaluates the region's logic
    AstCFunc* ਅਚਲ m_funcp = ਖਾਲੀ_ਸੂਚਕ;
    // Is this kit used/required?
    ਤਾਰਕਿਕ ਖਾਲੀ_ਥਾਵਾਂ() ਅਚਲ { ਵਾਪਸ !m_funcp; }
};

//============================================================================
// Create the evaluation function of each region of a time step

ਖਾਲੀ createEval(AstNetlist* netlistp,  //
                ਅਚਲ TriggerKit& trigKit,  //
                ਅਚਲ EvalKit& actKit,  //
                ਅਚਲ EvalKit& nbaKit,  //
                ਅਚਲ EvalKit& obsKit,  //
                ਅਚਲ EvalKit& reactKit,  //
                TimingKit& timingKit  //
) {
    File_ਸਤਰ* ਅਚਲ flp = netlistp->fileline();

    // Grab the delay scheduler variable, if any
    AstVarScope* ਅਚਲ delaySchedVscp = timingKit.getDelayScheduler(netlistp);

    // 'createResume' consumes the contents that 'createReady' needs, so do the right order
    AstCCall* ਅਚਲ timingReadyp = timingKit.createReady(trigKit.vscp());
    AstCCall* ਅਚਲ timingResumep = timingKit.createResume(trigKit.vscp());

    // Create the 'act' region
    createEvalRegion(  //
        netlistp, VEval::ACT, 0, trigKit,
        // Use trigger
        actKit.m_vscp, ਖਾਲੀ_ਸੂਚਕ,
        // Prep statements
        [&]() {
            // Compute the current 'act' triggers - the NBA triggers are the latched value
            Ast_ਗੰਢ_Stmt* stmtsp = trigKit.new_Comp_ਅਧਾਰ_Call();
            Ast_ਗੰਢ_Stmt* ਅਚਲ dumpp
                = stmtsp ? trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), ਸੱਚ) : ਖਾਲੀ_ਸੂਚਕ;
            // Mark as ready for triggered awaits
            ਜੇ (timingReadyp) stmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(stmtsp, timingReadyp->makeStmt());
            ਜੇ (AstVarScope* ਅਚਲ vscAccp = trigKit.vscAccp()) {
                stmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(stmtsp, trigKit.newOrIntoCall(actKit.m_vscp, vscAccp));
            }
            stmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(stmtsp, trigKit.newCompExtCall(nbaKit.m_vscp));
            stmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(stmtsp, dumpp);
            // Latch the 'act' triggers under the 'nba' triggers
            stmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(stmtsp, trigKit.newOrIntoCall(nbaKit.m_vscp, actKit.m_vscp));
            //
            ਵਾਪਸ stmtsp;
        }(),
        // Work statements
        [&]() {
            Ast_ਗੰਢ_Stmt* workp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (AstVarScope* ਅਚਲ actAccp = trigKit.vscAccp()) {
                AstCMethodHard* ਅਚਲ cCallp = ਨਵਾਂ AstCMethodHard{
                    flp, ਨਵਾਂ AstVarRef{flp, actAccp, VAccess::WRITE}, VCMethod::UNPACKED_FILL,
                    ਨਵਾਂ AstConst{flp, AstConst::Unsized64{}, 0}};
                cCallp->dtype_ਸੈੱਟ_Void();
                workp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(workp, cCallp->makeStmt());
            }
            // Resume triggered timing schedulers
            ਜੇ (timingResumep) workp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(workp, timingResumep->makeStmt());
            // Invoke the 'act' function
            workp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(workp, util::callVoidFunc(actKit.m_funcp));
            //
            ਵਾਪਸ workp;
        }());

    // Create the 'inact' region
    createEvalRegion(  //
        netlistp, VEval::INACT, 0, trigKit,
        // Use explicit condition
        ਖਾਲੀ_ਸੂਚਕ,
        [&]() -> Ast_ਗੰਢ_ਵਿਅੰਜਕ* {
            ਜੇ (!delaySchedVscp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            // Run if any zero delays are pending
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ callp
                = ਨਵਾਂ AstCMethodHard{flp, ਨਵਾਂ AstVarRef{flp, delaySchedVscp, VAccess::READ},
                                     VCMethod::SCHED_AWAITING_ZERO_DELAY};
            callp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
            ਵਾਪਸ callp;
        }(),
        // Prep statements
        ਖਾਲੀ_ਸੂਚਕ,
        // Work statements
        [&]() -> Ast_ਗੰਢ_Stmt* {
            ਜੇ (!delaySchedVscp) {
                // Nothing to do if there are no delays at all in the design
                ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            } ਨਹੀਂਤਾਂ ਜੇ (v3Global.ਵਰਤਦਾ_Zero_Delay()) {
                // Resume processes watiting for #0 delay
                AstCMethodHard* ਅਚਲ callp = ਨਵਾਂ AstCMethodHard{
                    flp, ਨਵਾਂ AstVarRef{flp, delaySchedVscp, VAccess::READWRITE},
                    VCMethod::SCHED_RESUME_ZERO_DELAY};
                callp->dtype_ਸੈੱਟ_Void();
                ਵਾਪਸ callp->makeStmt();
            } ਨਹੀਂਤਾਂ {
                // Assumption was that the design doesn't use #0 delays.
                // Die at run-time if it does.
                AstCStmt* ਅਚਲ stmtp = ਨਵਾਂ AstCStmt{flp};
                ਅਚਲ File_ਸਤਰ* ਅਚਲ locp = netlistp->ਸਿਖਰ_Modulep()->fileline();
                ਅਚਲ std::string& file = VIdProtect::protect(locp->filename());
                ਅਚਲ std::string& ਸਤਰ = std::ਮੰਜ਼ਿਲ_string(locp->lineno());
                stmtp->ਜੋੜੋ("VL_FATAL_MT(\"" + V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(file) + "\", "
                           + ਸਤਰ
                           + ", \"\", \"ZERODLY: Design Verilated with '--no-sched-zero-delay', "
                           + "but #0 delay executed at runtime\");");
                ਵਾਪਸ stmtp;
            }
        }());

    // Create the 'nba' region
    createEvalRegion(  //
        netlistp, VEval::NBA, 0, trigKit,
        // Use trigger
        nbaKit.m_vscp, ਖਾਲੀ_ਸੂਚਕ,
        // Prep statements
        ਖਾਲੀ_ਸੂਚਕ,
        // Work statements
        [&]() {
            Ast_ਗੰਢ_Stmt* workp = ਖਾਲੀ_ਸੂਚਕ;
            // Latch the 'nba' trigger flags under the following region's trigger flags
            ਜੇ (!obsKit.ਖਾਲੀ_ਥਾਵਾਂ()) {
                workp = trigKit.newOrIntoCall(obsKit.m_vscp, nbaKit.m_vscp);
            } ਨਹੀਂਤਾਂ ਜੇ (!reactKit.ਖਾਲੀ_ਥਾਵਾਂ()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, nbaKit.m_vscp);
            }
            // Invoke the 'nba' function
            workp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(workp, util::callVoidFunc(nbaKit.m_funcp));
            // Clear the 'nba' triggers
            workp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(workp, trigKit.newClearCall(nbaKit.m_vscp));
            //
            ਵਾਪਸ workp;
        }(),
        // Extra work (not conditional on having had a fired trigger)
        [&](AstVarScope* continuep) -> Ast_ਗੰਢ_Stmt* {
            // Check if any dynamic NBAs are pending, if there are any in the design
            ਜੇ (!netlistp->nbaEventp()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            AstVarScope* ਅਚਲ nbaEventp = netlistp->nbaEventp();
            AstVarScope* ਅਚਲ nbaEventTriggerp = netlistp->nbaEventTriggerp();
            UASSERT(nbaEventTriggerp, "NBA event trigger var should exist");
            netlistp->nbaEventp(ਖਾਲੀ_ਸੂਚਕ);
            netlistp->nbaEventTriggerp(ਖਾਲੀ_ਸੂਚਕ);

            // If a dynamic NBA is pending, clear the pending flag and fire the ready event
            AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, ਨਵਾਂ AstVarRef{flp, nbaEventTriggerp, VAccess::READ}};
            ifp->ਜੋੜੋ_Thensp(util::ਸੈੱਟ_Var(continuep, 1));
            ifp->ਜੋੜੋ_Thensp(util::ਸੈੱਟ_Var(nbaEventTriggerp, 0));
            AstCMethodHard* ਅਚਲ firep = ਨਵਾਂ AstCMethodHard{
                flp, ਨਵਾਂ AstVarRef{flp, nbaEventp, VAccess::WRITE}, VCMethod::EVENT_FIRE};
            firep->dtype_ਸੈੱਟ_Void();
            ifp->ਜੋੜੋ_Thensp(firep->makeStmt());
            ਵਾਪਸ ifp;
        });

    // Create the 'obs' region
    createEvalRegion(  //
        netlistp, VEval::OBS, 0, trigKit,
        // Use trigger
        obsKit.m_vscp, ਖਾਲੀ_ਸੂਚਕ,
        // Prep statements
        ਖਾਲੀ_ਸੂਚਕ,
        // Work statements
        [&]() -> Ast_ਗੰਢ_Stmt* {
            ਜੇ (obsKit.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            Ast_ਗੰਢ_Stmt* workp = ਖਾਲੀ_ਸੂਚਕ;
            // Latch the Observed trigger flags under the Reactive trigger flags
            ਜੇ (!reactKit.ਖਾਲੀ_ਥਾਵਾਂ()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, obsKit.m_vscp);
            }
            // Invoke the 'obs' function
            workp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(workp, util::callVoidFunc(obsKit.m_funcp));
            // Clear the 'obs' triggers
            workp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(workp, trigKit.newClearCall(obsKit.m_vscp));
            //
            ਵਾਪਸ workp;
        }());

    // Create the 'react' region
    createEvalRegion(  //
        netlistp, VEval::REACT, 0, trigKit,
        // Use trigger
        reactKit.m_vscp, ਖਾਲੀ_ਸੂਚਕ,
        // Prep statements
        ਖਾਲੀ_ਸੂਚਕ,
        // Work statements
        [&]() -> Ast_ਗੰਢ_Stmt* {
            ਜੇ (reactKit.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            // Invoke the 'react' function
            Ast_ਗੰਢ_Stmt* workp = util::callVoidFunc(reactKit.m_funcp);
            // Clear the 'react' triggers
            workp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(workp, trigKit.newClearCall(reactKit.m_vscp));
            ਵਾਪਸ workp;
        }());
}

}  // namespace

//============================================================================
// Helper that builds virtual interface trigger sentrees

VirtIfaceTriggers::VscpTrigMap VirtIfaceTriggers::make_Vscp_ਮੰਜ਼ਿਲ_Trig_Map(ਅਚਲ TriggerKit& trigKit,
                                                                    uint32_t ਪਹਿਲਾ_ਸੂਚਕਾਂਕ) ਅਚਲ {
    VscpTrigMap map;
    uint32_t ਸੂਚਕਾਂਕ = ਪਹਿਲਾ_ਸੂਚਕਾਂਕ;
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : m_triggers) {
        map.emplace(ਇੰਦਰਾਜ.m_vscp, trigKit.extra_Trigger_ਸੂਚਕਾਂਕ(ਸੂਚਕਾਂਕ));
        ++ਸੂਚਕਾਂਕ;
    }
    ਵਾਪਸ map;
}

//============================================================================
// Top level entry-point to scheduling

ਖਾਲੀ ਸਮਾਂ_ਸਾਰਣੀ(AstNetlist* netlistp) {
    ਅਚਲ ਸਵੈਚਾਲਿਤ ਜੋੜੋ_ਮਾਪ_Stat = [](ਅਚਲ string& name, ਅਚਲ LogicByScope& lbs) {
        uint64_t ਮਾਪ = 0;
        lbs.foreachLogic([&](Ast_ਗੰਢ* nodep) { ਮਾਪ += nodep->ਗੰਢ_ਗਿਣਤੀ(); });
        V3Stats::ਜੋੜੋ_Stat("Scheduling, " + name, ਮਾਪ);
    };

    // Step 2: Prepare external domains for timing and virtual interfaces
    // Create extra triggers for virtual interfaces
    ਅਚਲ ਸਵੈਚਾਲਿਤ& virtIfaceTriggers = makeVirtIfaceTriggers(netlistp);
    // Resolve what covergroup reference formal arguments are bound to, which is visible here
    // but not from V3Order, where the reads through them must be modeled
    ਅਚਲ CovergroupRefBindings& cgRefBindings = makeCovergroupRefBindings(netlistp);
    // Prepare timing-related logic and external domains
    TimingKit timingKit = prepareTiming(netlistp);

    // Step 3: Gather and classify all logic in the design
    LogicClasses logicClasses = gatherLogicClasses(netlistp);

    ਜੇ (v3Global.opt.stats()) {
        V3Stats::statsStage("sched-gather");
        ਜੋੜੋ_ਮਾਪ_Stat("size of class: static", logicClasses.m_static);
        ਜੋੜੋ_ਮਾਪ_Stat("size of class: initial", logicClasses.m_initial);
        ਜੋੜੋ_ਮਾਪ_Stat("size of class: final", logicClasses.m_final);
    }

    // Step 4: Schedule static, initial and final logic classes in source order
    AstCFunc* ਅਚਲ staticp = createStatic(netlistp, logicClasses);
    ਜੇ (v3Global.opt.stats()) V3Stats::statsStage("sched-static");

    createInitial(netlistp, logicClasses);
    ਜੇ (v3Global.opt.stats()) V3Stats::statsStage("sched-initial");

    createFinal(netlistp, logicClasses);
    ਜੇ (v3Global.opt.stats()) V3Stats::statsStage("sched-final");

    // Step 5: Break combinational cycles by introducing hybrid logic
    // Note: breakCycles also removes corresponding logic from logicClasses.m_comb;
    logicClasses.m_hybrid = break_ਚੱਕਰਾਂ(netlistp, logicClasses.m_comb);
    ਜੇ (v3Global.opt.stats()) {
        ਜੋੜੋ_ਮਾਪ_Stat("size of class: clocked", logicClasses.m_clocked);
        ਜੋੜੋ_ਮਾਪ_Stat("size of class: combinational", logicClasses.m_comb);
        ਜੋੜੋ_ਮਾਪ_Stat("size of class: hybrid", logicClasses.m_hybrid);
        V3Stats::statsStage("sched-break-cycles");
    }

    // We pass around a single SenExprBuilder instance, as we only need one set of 'prev' variables
    // for edge/change detection in sensitivity expressions, which this keeps track of.
    Ast_ਸਿਖਰ_Scope* ਅਚਲ ਸਿਖਰ_Scopep = netlistp->ਸਿਖਰ_Scopep();
    AstScope* ਅਚਲ scopeTopp = ਸਿਖਰ_Scopep->scopep();
    Sen_ਵਿਅੰਜਕ_Builder sen_ਵਿਅੰਜਕ_Builder{scopeTopp};

    // Step 6: Create 'settle' region that restores the combinational invariant
    createSettle(netlistp, staticp, sen_ਵਿਅੰਜਕ_Builder, logicClasses, cgRefBindings);
    ਜੇ (v3Global.opt.stats()) V3Stats::statsStage("sched-settle");

    // Step 7: Partition the clocked and combinational (including hybrid) logic into pre/act/nba.
    // All clocks (signals referenced in an AstSenTree) generated via a blocking assignment
    // (including combinationally generated signals) are computed within the act region.
    LogicRegions logicRegions
        = ਹਿੱਸੇ_ਕਰੋ(logicClasses.m_clocked, logicClasses.m_comb, logicClasses.m_hybrid);
    logicRegions.m_obs = logicClasses.m_observed;
    logicRegions.m_react = logicClasses.m_reactive;
    ਜੇ (v3Global.opt.stats()) {
        ਜੋੜੋ_ਮਾਪ_Stat("size of region: Active Pre", logicRegions.m_pre);
        ਜੋੜੋ_ਮਾਪ_Stat("size of region: Active", logicRegions.m_act);
        ਜੋੜੋ_ਮਾਪ_Stat("size of region: NBA", logicRegions.m_nba);
        ਜੋੜੋ_ਮਾਪ_Stat("size of region: Observed", logicRegions.m_obs);
        ਜੋੜੋ_ਮਾਪ_Stat("size of region: Reactive", logicRegions.m_react);
        V3Stats::statsStage("sched-partition");
    }

    // Step 8: Replicate combinational logic
    LogicReplicas logicReplicas = replicateLogic(logicRegions);
    ਜੇ (v3Global.opt.stats()) {
        ਜੋੜੋ_ਮਾਪ_Stat("size of replicated logic: Input", logicReplicas.m_ico);
        ਜੋੜੋ_ਮਾਪ_Stat("size of replicated logic: Active", logicReplicas.m_act);
        ਜੋੜੋ_ਮਾਪ_Stat("size of replicated logic: NBA", logicReplicas.m_nba);
        ਜੋੜੋ_ਮਾਪ_Stat("size of replicated logic: Observed", logicReplicas.m_obs);
        ਜੋੜੋ_ਮਾਪ_Stat("size of replicated logic: Reactive", logicReplicas.m_react);
        V3Stats::statsStage("sched-replicate");
    }

    // Step 9: Create the input combinational logic
    createIcoRegion(netlistp, staticp, sen_ਵਿਅੰਜਕ_Builder, logicReplicas.m_ico, virtIfaceTriggers,
                    cgRefBindings);
    ਜੇ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-ico");

    // Step 10: Create the triggers
    AstVarScope* ਅਚਲ dpiExportTriggerVscp = netlistp->dpiExportTriggerp();
    netlistp->dpiExportTriggerp(ਖਾਲੀ_ਸੂਚਕ);  // Finished with this here

    // We may have an extra trigger for variable updated in DPI exports
    TriggerKit::ExtraTriggers extraTriggers;
    ਅਚਲ uint32_t dpi_Export_Trigger_ਸੂਚਕਾਂਕ = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    ਅਚਲ uint32_t ਪਹਿਲਾ_Vif_Trigger_ਸੂਚਕਾਂਕ = extraTriggers.ਮਾਪ();
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + ਇੰਦਰਾਜ.m_ifacep->name() + "."
                               + ਇੰਦਰਾਜ.m_memberp->name());
    }

    ਅਚਲ ਸਵੈਚਾਲਿਤ& preTreeps = get_Sen_Trees_ਵਰਤਿਆ_By({&logicRegions.m_pre});
    ਅਚਲ ਸਵੈਚਾਲਿਤ& senTreeps = get_Sen_Trees_ਵਰਤਿਆ_By({&logicRegions.m_act,  //
                                               &logicRegions.m_nba,  //
                                               &logicRegions.m_obs,  //
                                               &logicRegions.m_react,  //
                                               &timingKit.m_lbs});
    ਅਚਲ TriggerKit trigKit
        = TriggerKit::create(netlistp, staticp, sen_ਵਿਅੰਜਕ_Builder, preTreeps, senTreeps, "act",
                             extraTriggers, ਝੂਠ, v3Global.ਵਰਤਦਾ_Timing());

    // Add post updates from the timing kit
    ਜੇ (timingKit.m_postUpdates) trigKit.compBasep()->ਜੋੜੋ_Stmtsp(timingKit.m_postUpdates);

    ਜੇ (dpiExportTriggerVscp) {
        trigKit.ਜੋੜੋ_Extra_Trigger_Assignment(dpiExportTriggerVscp, dpi_Export_Trigger_ਸੂਚਕਾਂਕ);
    }
    ਜੋੜੋ_Virt_Iface_Trigger_Assignments(netlistp, staticp, virtIfaceTriggers, ਪਹਿਲਾ_Vif_Trigger_ਸੂਚਕਾਂਕ,
                                   trigKit);
    ਜੇ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-triggers");

    // Note: Experiments so far show that running the Act (or Ico) regions on
    // multiple threads is always a net loss, so only use multi-threading for
    // NBA for now. This can be revised if evidence is available that it would
    // be beneficial

    // Step 11: Create the 'act' region evaluation function

    // Add the trigger indices of the input logic
    ਜੋੜੋ_Trigger_Indices(logicRegions.m_pre, trigKit, ਸੱਚ);
    ਜੋੜੋ_Trigger_Indices(logicRegions.m_act, trigKit, ਝੂਠ);
    ਜੋੜੋ_Trigger_Indices(logicReplicas.m_act, trigKit, ਝੂਠ);
    ਜੋੜੋ_Trigger_Indices(timingKit.m_lbs, trigKit, ਝੂਠ);

    // External triggers of variables. Trigger indices are the same in all regions, so share.
    ਅਚਲ std::map<ਅਚਲ AstVarScope*, std::ਸਦਿਸ਼<uint32_t>> timingTriggers
        = timingKit.externalTriggerIndices(trigKit);
    ਅਚਲ ਸਵੈਚਾਲਿਤ& vif_Vscp_ਮੰਜ਼ਿਲ_Trig = virtIfaceTriggers.make_Vscp_ਮੰਜ਼ਿਲ_Trig_Map(trigKit, ਪਹਿਲਾ_Vif_Trigger_ਸੂਚਕਾਂਕ);
    ਅਚਲ V3_ਕ੍ਰਮ::ExternalTriggersProvider externalTriggers
        = [&](ਅਚਲ AstVarScope* vscp, std::ਸੈੱਟ<uint32_t>& triggers) {
              ਸਵੈਚਾਲਿਤ it = timingTriggers.ਲੱਭੋ(vscp);
              ਜੇ (it != timingTriggers.ਸਿਰਾ())
                  triggers.ਪਾਓ(it->ਦੂਜਾ.begin(), it->ਦੂਜਾ.ਸਿਰਾ());
              ਜੇ (vscp->varp()->isWrittenByDpi()) {
                  triggers.ਪਾਓ(trigKit.extra_Trigger_ਸੂਚਕਾਂਕ(dpi_Export_Trigger_ਸੂਚਕਾਂਕ));
              }
              ਜੇ (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                  ਜੋੜੋ_Triggered_Iface(vscp, vif_Vscp_ਮੰਜ਼ਿਲ_Trig, virtIfaceTriggers, triggers);
              }
          };

    AstCFunc* ਅਚਲ actFuncp
        = V3_ਕ੍ਰਮ::ਕ੍ਰਮ({&logicRegions.m_pre, &logicRegions.m_act, &logicReplicas.m_act}, trigKit,
                         trigKit.vscp(), cgRefBindings, "act", ਝੂਠ, ਝੂਠ, externalTriggers);
    util::ਵੰਡੋ_Check(actFuncp);
    ਜੇ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-act");

    ਅਚਲ EvalKit actKit{trigKit.vscp(), actFuncp};

    // Orders a region's logic and creates the region eval function
    ਅਚਲ ਸਵੈਚਾਲਿਤ ਕ੍ਰਮ = [&](ਅਚਲ std::string& name,
                           ਅਚਲ std::ਸਦਿਸ਼<V3Sched::LogicByScope*>& logic) -> EvalKit {
        UINFO(2, "Scheduling " << name << " #logic = " << logic.ਮਾਪ());
        AstVarScope* ਅਚਲ trigVscp = trigKit.newTrigVec(name);
        // Add the trigger indices of the input logic
        ਲਈ (LogicByScope* lbs : logic) ਜੋੜੋ_Trigger_Indices(*lbs, trigKit, ਝੂਠ);

        AstCFunc* ਅਚਲ funcp
            = V3_ਕ੍ਰਮ::ਕ੍ਰਮ(logic, trigKit, trigVscp, cgRefBindings, name,
                             name == "nba" && v3Global.opt.mtasks(), ਝੂਠ, externalTriggers);

        ਵਾਪਸ {trigVscp, funcp};
    };

    // Step 12: Create the 'nba' region evaluation function
    ਅਚਲ EvalKit nbaKit = ਕ੍ਰਮ("nba", {&logicRegions.m_nba, &logicReplicas.m_nba});
    util::ਵੰਡੋ_Check(nbaKit.m_funcp);
    ਜੇ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-nba");

    // Orders a region's logic and creates the region eval function (only if there is any logic in
    // the region)
    ਅਚਲ ਸਵੈਚਾਲਿਤ ਕ੍ਰਮ_If_Non_ਖਾਲੀ_ਥਾਵਾਂ
        = [&](ਅਚਲ std::string& name, ਅਚਲ std::ਸਦਿਸ਼<LogicByScope*>& logic) -> EvalKit {
        ਜੇ (logic[0]->ਖਾਲੀ_ਥਾਵਾਂ())
            ਵਾਪਸ {};  // if region is empty, replica is supposed to be empty as well
        ਅਚਲ ਸਵੈਚਾਲਿਤ& kit = ਕ੍ਰਮ(name, logic);
        ਜੇ (v3Global.opt.stats()) V3Stats::statsStage("sched-create-" + name);
        ਵਾਪਸ kit;
    };

    // Step 13: Create the 'obs' region evaluation function
    ਅਚਲ EvalKit obsKit = ਕ੍ਰਮ_If_Non_ਖਾਲੀ_ਥਾਵਾਂ("obs", {&logicRegions.m_obs, &logicReplicas.m_obs});

    // Step 14: Create the 'react' region evaluation function
    ਅਚਲ EvalKit reactKit
        = ਕ੍ਰਮ_If_Non_ਖਾਲੀ_ਥਾਵਾਂ("react", {&logicRegions.m_react, &logicReplicas.m_react});

    // Step 15: Create the 'postponed' region evaluation function
    createPostponed(netlistp, logicClasses);

    // Step 16: Populate the eval entry point function of each region of a time step
    createEval(netlistp, trigKit, actKit, nbaKit, obsKit, reactKit, timingKit);

    // Step 17: Add neccessary evaluation before awaits
    ਜੇ (AstCCall* ਅਚਲ readyp = timingKit.createReady(trigKit.vscp())) {
        staticp->ਜੋੜੋ_Stmtsp(readyp->makeStmt());
        beforeTrigVisitor(netlistp, sen_ਵਿਅੰਜਕ_Builder, trigKit);
    } ਨਹੀਂਤਾਂ {
        // beforeTrigVisitor clears Sentree pointers in AstCAwaits (as these sentrees will get
        // deleted later) if there was no need to call it, SenTrees have to be cleaned manually
        netlistp->foreach([](AstCAwait* ਅਚਲ cAwaitp) { cAwaitp->clearSentreep(); });
    }
    ਜੇ (AstVarScope* ਅਚਲ trigAccp = trigKit.vscAccp()) {
        // Copy trigger vector to accumulator at the end of static initialziation so,
        // triggers fired during initialization persist to the first resume.
        ਅਚਲ AstUnpackArrayDType* ਅਚਲ trigAccDTypep
            = VN_AS(trigAccp->dtypep(), UnpackArrayDType);
        UASSERT_OBJ(
            trigAccDTypep->ਸੱਜੇ() == 0, trigAccp,
            "Expected that trigger vector and accumulator start elements enumeration from 0");
        UASSERT_OBJ(trigAccDTypep->ਖੱਬੇ() >= 0, trigAccp,
                    "Expected that trigger vector and accumulator has no negative indexes");
        File_ਸਤਰ* ਅਚਲ flp = trigAccp->fileline();
        AstVarScope* ਅਚਲ vscp = netlistp->ਸਿਖਰ_Scopep()->create_ਆਰਜ਼ੀ("__Vi", 32);
        AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{flp};
        loopp->ਜੋੜੋ_Stmtsp(
            ਨਵਾਂ Ast_ਸੌਂਪੋ{flp,
                          ਨਵਾਂ AstArraySel{flp, ਨਵਾਂ AstVarRef{flp, trigAccp, VAccess::WRITE},
                                          ਨਵਾਂ AstVarRef{flp, vscp, VAccess::READ}},
                          ਨਵਾਂ AstArraySel{flp, ਨਵਾਂ AstVarRef{flp, actKit.m_vscp, VAccess::READ},
                                          ਨਵਾਂ AstVarRef{flp, vscp, VAccess::READ}}});
        loopp->ਜੋੜੋ_Stmtsp(util::incrementVar(vscp));
        loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{
            flp, loopp,
            ਨਵਾਂ AstLte{flp, ਨਵਾਂ AstVarRef{flp, vscp, VAccess::READ},
                       ਨਵਾਂ AstConst{flp, AstConst::Widthed_ਮੁੱਲ{}, 32,
                                    static_cast<uint32_t>(trigAccDTypep->ਖੱਬੇ())}}});
        staticp->ਜੋੜੋ_Stmtsp(loopp);
    }

    // Step 18: Clean up
    netlistp->clear_Stl_ਪਹਿਲਾ_Iterationp();

    ਜੇ (v3Global.opt.stats()) {
        // A sample() call resolved to the union over its covergroup's constructions reads more
        // than it can, which orders it against more logic than necessary
        V3Stats::ਜੋੜੋ_Stat("Scheduling, covergroup ref sample calls, per instance",
                         cgRefBindings.ਅੰਕ_Exact_Calls());
        V3Stats::ਜੋੜੋ_Stat("Scheduling, covergroup ref sample calls, per type",
                         cgRefBindings.ਅੰਕ_Union_Calls());
    }

    // Haven't split static initializer yet
    util::ਵੰਡੋ_Check(staticp);

    // Record eval stats
    netlistp->ਜੋੜੋ_Eval_Stats("sched");

    // Dump
    V3Global::dump_Check_Global_ਰੁੱਖ("sched", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}

}  // namespace V3Sched
