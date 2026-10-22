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

std::ويڪٽر<ثابت Ast_Sen_وڻ*> get_Sen_Trees_استعمال_ٿيل_By(ثابت std::ويڪٽر<ثابت LogicByScope*>& lbsps) {
    ثابت VNUser1_اندريون_Use user1_اندريون_Use;
    std::ويڪٽر<ثابت Ast_Sen_وڻ*> نتيجو;
    لكل (ثابت LogicByScope* ثابت lbsp : lbsps) {
        لكل (ثابت خودڪار& داخلا_2 : *lbsp) {
            Ast_سرگرم* ثابت activep = داخلا_2.m_activep;
            Ast_Sen_وڻ* ثابت senTreep = activep->sentreep();
            إذا (senTreep->user1_سيٽ_Once()) استمر;
            إذا (senTreep->hasClocked() || senTreep->hasHybrid()) نتيجو.ڌڪو_پٺ(senTreep);
        }
    }
    أرجع نتيجو;
}

فراغ جوڙيو_Trigger_Indices(LogicByScope& lbs, ثابت TriggerKit& trigKit, منطقي pre) {
    لكل (LogicByScope::داخلا_3& داخلا_2 : lbs) {
        Ast_Sen_وڻ* ثابت senTreep = داخلا_2.m_activep->sentreep();
        إذا (senTreep->hasCombo()) استمر;
        داخلا_2.m_triggers = trigKit.triggerIndices(senTreep, pre);
    }
}

// Add the VIF triggers that a given VarScope should be sensitive to.
// Case 2 (non-virtual interface read): sensitive to that specific VarScope's trigger only
// Case 3 (virtual interface read): sensitive to all member triggers of the same interface type
فراغ جوڙيو_Triggered_Iface(ثابت AstVarScope* vscp, ثابت VirtIfaceTriggers::VscpTrigMap& vscp_منزل_Trig,
                       ثابت VirtIfaceTriggers& virtIfaceTriggers, std::سيٽ<uint32_t>& triggers) {
    إذا (vscp->varp()->isVirtIface()) {
        // Virtual interface variable -- sensitive to all member triggers of this interface type
        ثابت AstIface* ثابت ifacep = VN_AS(vscp->varp()->dtypep(), IfaceRefDType)->ifacep();
        لكل (ثابت خودڪار& داخلا_2 : virtIfaceTriggers.m_triggers) {
            إذا (داخلا_2.m_ifacep == ifacep) {
                ثابت خودڪار it = vscp_منزل_Trig.ڳوليو(داخلا_2.m_vscp);
                إذا (it != vscp_منزل_Trig.پڇاڙي()) triggers.وجهو(it->ٻيو);
            }
        }
    } وإلا {
        // Non-virtual interface member -- sensitive to this VarScope's trigger only
        ثابت خودڪار it = vscp_منزل_Trig.ڳوليو(vscp);
        إذا (it != vscp_منزل_Trig.پڇاڙي()) triggers.وجهو(it->ٻيو);
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
    uint32_t پهريون_Iter_Trigger,
    ثابت TriggerKit& trigKit,  // The trigger kit
    AstVarScope* trigp,  // The trigger vector - may be nullptr if no triggers or using 'condp'
    Ast_ڳنڍ_اظهار* condp,  // Explicit condition that must be true to run 'phaseWorkp'
    Ast_ڳنڍ_Stmt* phasePrepp,  // Prep statements run before checking triggers
    Ast_ڳنڍ_Stmt* phaseWorkp,  // The work to do if anything triggered
    // Extra statements to run after the work, even if no triggers fired. This function is
    // passed a variable, which must be set to true if we must continue and loop again,
    // and must be unmodified otherwise.
    std::function<Ast_ڳنڍ_Stmt*(AstVarScope*)> phaseExtra = [](AstVarScope*) { أرجع خالي_اشارو; }  //
) {
    UASSERT(!trigp || !condp, "Cannot use both 'trigp' and 'condp' in 'createEvalRegion'");
    UASSERT(!eval.پهريون_Iteration() || trigp,
            "Region without triggers cannot need a first iteration flag");

    // All work is under a trigger or condition, so with neither the region has
    // nothing to evaluate, and what we create below reduces to a no-op function.

    ثابت std::string ٽيگ = eval.ٽيگ();
    Ast_چوٽي_Scope* ثابت چوٽي_Scopep = netlistp->چوٽي_Scopep();
    File_سٽ* ثابت flp = netlistp->fileline();

    // Populate the trigger dump entry point function
    إذا (trigp) {
        netlistp->dumpTriggersFuncp(eval)->جوڙيو_Stmtsp(trigKit.newDumpCall(trigp, ٽيگ, خطأ));
    }

    AstCFunc* ثابت funcp = netlistp->evalFuncp(eval);

    // A flag is passed from the run-time eval loop if this is the first iteration of the
    // current loop
    إذا (eval.پهريون_Iteration()) {
        AstVarScope* ثابت پهريون_Iter_Argp = util::new_دليل(funcp, netlistp->ڳوليو_ٻٽو_انگ_DType(),
                                                             "firstIteration", VDirection::داخلا);

        // Set the region's 'first iteration' trigger straight from the argument
        funcp->جوڙيو_Stmtsp(trigKit.newExtraTriggerAssignment(پهريون_Iter_Argp, پهريون_Iter_Trigger));

        // Only 'stl' also needs a module level flag, for design logic that reads
        // it directly (see V3Timing). Those reads can be anywhere in the design,
        // hence module level. Always created, even if nothing reads it.
        // TODO: get rid of this special case
        إذا (eval == VEval::STL) {
            AstVarScope* ثابت پهريون_Iterp = netlistp->stl_پهريون_Iterationp();
            پهريون_Iterp->varp()->noReset(سچ);
            پهريون_Iterp->varp()->is_اندروني(سچ);
            funcp->جوڙيو_Stmtsp(جديد Ast_سونپيو{flp, جديد AstVarRef{flp, پهريون_Iterp, VAccess::WRITE},
                                           جديد AstVarRef{flp, پهريون_Iter_Argp, VAccess::READ}});
        }
    }

    {
        // Add the preparatory statements
        funcp->جوڙيو_Stmtsp(phasePrepp);

        // The execute flag
        AstVarScope* ثابت executeFlagp = چوٽي_Scopep->create_عارضي("__V" + ٽيگ + "Execute", 1);
        executeFlagp->varp()->noReset(سچ);

        // If there is work in this region, execute it if any triggers fired
        إذا (phaseWorkp) {
            Ast_ڳنڍ_اظهار* ثابت lhsp = جديد AstVarRef{flp, executeFlagp, VAccess::WRITE};
            // If using explicit condition, that directly determines whether to execute,
            // otherwise check if any triggers are fired
            Ast_ڳنڍ_اظهار* ثابت rhsp = condp ? condp : trigKit.new_Any_سيٽ_Call(trigp);
            funcp->جوڙيو_Stmtsp(جديد Ast_سونپيو{flp, lhsp, rhsp});

            // Add the work
            AstIf* ثابت ifp = جديد AstIf{flp, جديد AstVarRef{flp, executeFlagp, VAccess::READ}};
            ifp->جوڙيو_Thensp(phaseWorkp);
            funcp->جوڙيو_Stmtsp(ifp);
        }

        // Construct the extra statements
        Ast_ڳنڍ_Stmt* ثابت extraWorkp = phaseExtra(executeFlagp);
        إذا (extraWorkp) funcp->جوڙيو_Stmtsp(extraWorkp);

        // The function returns true iff it did run work
        Ast_ڳنڍ_اظهار* ثابت retp
            = phaseWorkp || extraWorkp
                  ? static_cast<Ast_ڳنڍ_اظهار*>(جديد AstVarRef{flp, executeFlagp, VAccess::READ})
                  : static_cast<Ast_ڳنڍ_اظهار*>(جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}});
        funcp->جوڙيو_Stmtsp(جديد AstCReturn{flp, retp});
    }
}

//============================================================================
// Collect and classify all logic in the design

LogicClasses gatherLogicClasses(AstNetlist* netlistp) {
    LogicClasses نتيجو;

    netlistp->foreach([&](AstScope* scopep) {
        scopep->foreach([&](Ast_سرگرم* activep) {
            Ast_Sen_وڻ* ثابت senTreep = activep->sentreep();
            إذا (senTreep->hasStatic()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "static initializer with additional sensitivities");
                نتيجو.m_static.جوڙيو(scopep, activep);
            } وإلا إذا (senTreep->hasInitial()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'initial' logic with additional sensitivities");
                نتيجو.m_initial.جوڙيو(scopep, activep);
            } وإلا إذا (senTreep->hasFinal()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'final' logic with additional sensitivities");
                نتيجو.m_final.جوڙيو(scopep, activep);
            } وإلا إذا (senTreep->hasCombo()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "combinational logic with additional sensitivities");
                إذا (VN_IS(activep->stmtsp(), AlwaysPostponed)) {
                    نتيجو.m_postponed.جوڙيو(scopep, activep);
                } وإلا {
                    نتيجو.m_comb.جوڙيو(scopep, activep);
                }
            } وإلا {
                UASSERT_OBJ(senTreep->hasClocked(), activep, "What else could it be?");
                إذا (VN_IS(activep->stmtsp(), AlwaysObserved)) {
                    نتيجو.m_observed.جوڙيو(scopep, activep);
                } وإلا إذا (VN_IS(activep->stmtsp(), AlwaysReactive)) {
                    نتيجو.m_reactive.جوڙيو(scopep, activep);
                } وإلا {
                    نتيجو.m_clocked.جوڙيو(scopep, activep);
                }
            }
        });
    });

    أرجع نتيجو;
}

//============================================================================
// Simple ordering in source order

فراغ ترتيب_Sequentially(AstCFunc* funcp, ثابت LogicByScope& lbs) {
    // Create new subfunc for scope
    ثابت خودڪار createNewSubFuncp = [&](AstScope* ثابت scopep) {
        ثابت string subName{funcp->name() + "__" + scopep->nameDotless()};
        AstCFunc* ثابت subFuncp = جديد AstCFunc{scopep->fileline(), subName, scopep};
        subFuncp->isLoose(سچ);
        subFuncp->isConst(خطأ);
        subFuncp->decl_ذاتي(سچ);
        subFuncp->slow(funcp->slow());
        scopep->جوڙيو_Blocksp(subFuncp);
        // Call it from the top function
        funcp->جوڙيو_Stmtsp(util::callVoidFunc(subFuncp));
        أرجع subFuncp;
    };
    ثابت VNUser1_اندريون_Use user1_اندريون_Use;  // AstScope -> AstCFunc: the sub-function for the scope
    ثابت VNUser2_اندريون_Use user2_اندريون_Use;  // AstScope -> int: sub-function counter used for names
    لكل (ثابت خودڪار& داخلا_2 : lbs) {
        AstScope* ثابت scopep = داخلا_2.m_scopep;
        Ast_سرگرم* ثابت activep = داخلا_2.m_activep;
        // Create a sub-function per scope so we can V3Combine them later
        إذا (!scopep->user1p()) scopep->user1p(createNewSubFuncp(scopep));
        // Add statements to sub-function
        لكل (Ast_ڳنڍ *logicp = activep->stmtsp(), *nextp; logicp; logicp = nextp) {
            خودڪار* subFuncp = VN_AS(scopep->user1p(), CFunc);
            nextp = logicp->nextp();
            إذا (Ast_ڳنڍ_Procedure* ثابت procp = VN_CAST(logicp, ڳنڍ_Procedure)) {
                إذا (Ast_ڳنڍ* bodyp = procp->stmtsp()) {
                    bodyp->unlink_Fr_پٺ_With_ايندڙ();
                    // If the process is suspendable, we need a separate function (a coroutine)
                    إذا (procp->isSuspendable()) {
                        funcp->slow(خطأ);
                        subFuncp = createNewSubFuncp(scopep);
                        subFuncp->name(subFuncp->name() + "__Vtiming__"
                                       + cvt_منزل_Str(scopep->user2Inc()));
                        subFuncp->rtnType("VlCoroutine");
                        إذا (VN_IS(procp, Always)) {
                            subFuncp->slow(خطأ);
                            File_سٽ* ثابت flp = procp->fileline();
                            Ast_ڳنڍ_اظهار* ثابت condp = جديد AstCExpr{
                                flp, "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish())", 1};
                            AstLoop* ثابت loopp = جديد AstLoop{flp};
                            loopp->جوڙيو_Stmtsp(جديد AstLoopTest{flp, loopp, condp});
                            loopp->جوڙيو_Stmtsp(bodyp);
                            bodyp = loopp;
                        }
                    }
                    subFuncp->جوڙيو_Stmtsp(bodyp);
                    إذا (procp->needProcess()) subFuncp->سيٽ_Need_Process();
                    // A coroutine holds a single process, so it is complete here
                    إذا (procp->isSuspendable()) util::ورهايو_Check(subFuncp);
                }
            } وإلا {
                logicp->unlink_Fr_پٺ();
                subFuncp->جوڙيو_Stmtsp(logicp);
            }
        }
        إذا (activep->backp()) activep->unlink_Fr_پٺ();
        VL_DO_DANGLING(activep->delete_وڻ(), activep);
    }
    // Split sub-functions once complete
    لكل (ثابت خودڪار& داخلا_2 : lbs) {
        AstScope* ثابت scopep = داخلا_2.m_scopep;
        إذا (AstCFunc* ثابت subFuncp = VN_AS(scopep->user1p(), CFunc)) {
            util::ورهايو_Check(subFuncp);
            scopep->user1p(خالي_اشارو);
        }
    }
}

//============================================================================
// Create simply ordered functions

AstCFunc* createStatic(AstNetlist* netlistp, ثابت LogicClasses& logicClasses) {
    AstCFunc* ثابت funcp = netlistp->evalFuncp(VEval::STATIC);

    ثابت LogicByScope& orig = logicClasses.m_static;
    إذا (orig.سائيز() <= 1) {
        ترتيب_Sequentially(funcp, orig);
        أرجع funcp;
    }

    // Level-based module sorting can reorder packages so that an importing
    // package runs before the imported one.  Re-sort package entries by source
    // file position to restore compilation order (IEEE 1800-2023 26.3).
    std::ويڪٽر<سائيز_t> indices(orig.سائيز());
    لكل (سائيز_t i = 0; i < orig.سائيز(); ++i) indices[i] = i;
    std::stable_ترتيب_ڏيو(indices.begin(), indices.پڇاڙي(), [&](سائيز_t a, سائيز_t b) {
        ثابت Ast_ڳنڍ_Module* ثابت modA = orig[a].m_scopep->modp();
        ثابت Ast_ڳنڍ_Module* ثابت modB = orig[b].m_scopep->modp();
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
    لكل (ثابت سائيز_t i : indices) sorted.جوڙيو(orig[i].m_scopep, orig[i].m_activep);

    ترتيب_Sequentially(funcp, sorted);
    أرجع funcp;  // Not splitting yet as it is not final
}

فراغ createInitial(AstNetlist* netlistp, ثابت LogicClasses& logicClasses) {
    AstCFunc* ثابت funcp = netlistp->evalFuncp(VEval::INITIAL);
    ترتيب_Sequentially(funcp, logicClasses.m_initial);
    util::ورهايو_Check(funcp);
}

فراغ createPostponed(AstNetlist* netlistp, ثابت LogicClasses& logicClasses) {
    AstCFunc* ثابت funcp = netlistp->evalFuncp(VEval::POSTPONED);
    ترتيب_Sequentially(funcp, logicClasses.m_postponed);
    util::ورهايو_Check(funcp);
}

فراغ createFinal(AstNetlist* netlistp, ثابت LogicClasses& logicClasses) {
    AstCFunc* ثابت funcp = netlistp->evalFuncp(VEval::FINAL);
    ترتيب_Sequentially(funcp, logicClasses.m_final);
    util::ورهايو_Check(funcp);
}

//============================================================================
// Helper that creates virtual interface value-change triggers

فراغ جوڙيو_Virt_Iface_Trigger_Assignments(AstNetlist* netlistp, AstCFunc* initFuncp,
                                    ثابت VirtIfaceTriggers& virtIfaceTriggers,
                                    uint32_t پهريون_اشاريو, ثابت TriggerKit& trigKit) {
    uint32_t اشاريو = پهريون_اشاريو;
    لكل (ثابت خودڪار& داخلا_2 : virtIfaceTriggers.m_triggers) {
        trigKit.جوڙيو_قدر_Change_Trigger_Assignment(netlistp, initFuncp, داخلا_2.m_vscp, اشاريو);
        ++اشاريو;
    }
}

// Order the combinational logic to create the 'stl' region
فراغ createSettle(AstNetlist* netlistp, AstCFunc* ثابت initFuncp, Sen_اظهار_Builder& sen_اظهار_Bulider,
                  LogicClasses& logicClasses, ثابت CovergroupRefBindings& cgRefBindings) {
    // Clone, because ordering is destructive, but we still need them for the other regions
    LogicByScope comb = logicClasses.m_comb.clone();
    LogicByScope hybrid = logicClasses.m_hybrid.clone();

    // We have an extra trigger denoting this is the first iteration of the settle loop
    TriggerKit::ExtraTriggers extraTriggers;
    ثابت uint32_t پهريون_Iteration_Trigger = extraTriggers.allocate("first iteration");

    // Gather the relevant sensitivity expressions and create the trigger kit
    ثابت خودڪار& senTreeps = get_Sen_Trees_استعمال_ٿيل_By({&comb, &hybrid});
    ثابت TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_اظهار_Bulider, {},
                                                  senTreeps, "stl", extraTriggers, سچ, خطأ);

    // Add trigger indices (comb has none, so only do the hybrid)
    جوڙيو_Trigger_Indices(hybrid, trigKit, خطأ);

    // First trigger is for pure combinational triggers (first iteration)
    ثابت uint32_t داخلا_بدليو = trigKit.extra_Trigger_اشاريو(پهريون_Iteration_Trigger);

    // Create and the body function
    AstCFunc* ثابت stlFuncp = V3_ترتيب::ترتيب(
        {&comb, &hybrid}, trigKit, trigKit.vscp(), cgRefBindings, "stl", خطأ, سچ,
        [=](ثابت AstVarScope*, std::سيٽ<uint32_t>& triggers) { triggers.وجهو(داخلا_بدليو); });
    util::ورهايو_Check(stlFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::STL, پهريون_Iteration_Trigger, trigKit,
        // Use trigger
        trigKit.vscp(), خالي_اشارو,
        // Explicit condition
        // Prep statements: Compute the current 'stl' triggers
        [&trigKit] {
            Ast_ڳنڍ_Stmt* ثابت stmtp = trigKit.new_Comp_بنياد_Call();
            إذا (stmtp) stmtp->جوڙيو_ايندڙ(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), سچ));
            أرجع stmtp;
        }(),
        // Work statements: Invoke the 'stl' function
        util::callVoidFunc(stlFuncp));
}

//============================================================================
// Order the replicated combinational logic to create the 'ico' region

فراغ createIcoRegion(AstNetlist* netlistp, AstCFunc* ثابت initFuncp,
                     Sen_اظهار_Builder& sen_اظهار_Builder, LogicByScope& logic,
                     ثابت VirtIfaceTriggers& virtIfaceTriggers,
                     ثابت CovergroupRefBindings& cgRefBindings) {
    // SystemC only: Any top level inputs feeding a combinational logic must be marked,
    // so we can make them sc_sensitive
    إذا (v3Global.opt.systemC()) {
        logic.foreachLogic([](Ast_ڳنڍ* logicp) {
            logicp->foreach([](AstVarRef* refp) {
                إذا (refp->access().isWriteOnly()) أرجع;
                AstVarScope* ثابت vscp = refp->varScopep();
                إذا (vscp->scopep()->is_چوٽي() && vscp->varp()->isNonOutput()) {
                    vscp->varp()->scSensitive(سچ);
                }
            });
        });
    }

    // We have some extra trigger denoting external conditions
    AstVarScope* ثابت dpiExportTriggerVscp = netlistp->dpiExportTriggerp();

    TriggerKit::ExtraTriggers extraTriggers;
    ثابت uint32_t پهريون_Iteration_Trigger = extraTriggers.allocate("first iteration");
    ثابت uint32_t dpi_Export_Trigger_اشاريو = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    ثابت uint32_t پهريون_Vif_Trigger_اشاريو = extraTriggers.سائيز();
    لكل (ثابت خودڪار& داخلا_2 : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + داخلا_2.m_ifacep->name() + "."
                               + داخلا_2.m_memberp->name());
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
    std::unordered_map<ثابت AstVarScope*, Ast_Sen_وڻ*> inp2changedp;
    std::ويڪٽر<Ast_Sen_وڻ*> icoChangeSenTreeps;
    إذا (v3Global.opt.fIcoChangeDetect().isTrue()) {
        File_سٽ* ثابت flp = netlistp->fileline();
        AstScope* ثابت scopep = netlistp->چوٽي_Scopep()->scopep();
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
            Ast_Sen_وڻ*& senTreepr = inp2changedp[vscp];
            UASSERT_OBJ(!senTreepr, vscp, "Duplicate input change detect trigger");
            AstVarRef* ثابت refp = جديد AstVarRef{flp, vscp, VAccess::READ};
            Ast_Sen_شي* ثابت senItemp = جديد Ast_Sen_شي{flp, VEdgeType::ET_بدليو, refp};
            senTreepr = جديد Ast_Sen_وڻ{flp, senItemp};
            icoChangeSenTreeps.ڌڪو_پٺ(senTreepr);
        }
    }
    V3Stats::جوڙيو_Stat("Scheduling, 'ico' change detect triggers", icoChangeSenTreeps.سائيز());

    // Gather the relevant sensitivity expressions and create the trigger kit
    std::ويڪٽر<ثابت Ast_Sen_وڻ*> senTreeps = get_Sen_Trees_استعمال_ٿيل_By({&logic});
    senTreeps.وجهو(senTreeps.پڇاڙي(), icoChangeSenTreeps.begin(), icoChangeSenTreeps.پڇاڙي());
    ثابت TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_اظهار_Builder, {},
                                                  senTreeps, "ico", extraTriggers, خطأ, خطأ);
    std::ignore = sen_اظهار_Builder.getAndClearResults();

    إذا (dpiExportTriggerVscp) {
        trigKit.جوڙيو_Extra_Trigger_Assignment(dpiExportTriggerVscp, dpi_Export_Trigger_اشاريو);
    }
    جوڙيو_Virt_Iface_Trigger_Assignments(netlistp, initFuncp, virtIfaceTriggers, پهريون_Vif_Trigger_اشاريو,
                                   trigKit);

    // Add trigger indices
    جوڙيو_Trigger_Indices(logic, trigKit, خطأ);

    // The 'first iteration' trigger for top level inputs
    ثابت uint32_t پهريون_Iter_Trigger = trigKit.extra_Trigger_اشاريو(پهريون_Iteration_Trigger);

    ثابت خودڪار& vif_Vscp_منزل_Trig_Ico
        = virtIfaceTriggers.make_Vscp_منزل_Trig_Map(trigKit, پهريون_Vif_Trigger_اشاريو);

    // Create and Order the body function
    AstCFunc* ثابت icoFuncp = V3_ترتيب::ترتيب(
        {&logic}, trigKit, trigKit.vscp(), cgRefBindings, "ico", خطأ, خطأ,
        [&](ثابت AstVarScope* vscp, std::سيٽ<uint32_t>& triggers) {
            AstVar* ثابت varp = vscp->varp();
            // If it has an explicit change detect trigger, use that,
            // otherwise fall back to using the 'first iteration' trigger
            خودڪار it = inp2changedp.ڳوليو(vscp);
            إذا (it != inp2changedp.پڇاڙي()) {
                triggers.وجهو(trigKit.senItem2TrigIdx(it->ٻيو->sensesp()));
            } وإلا إذا (varp->isPrimaryInish() || varp->isSigUserRWPublic() || varp->sampled()) {
                triggers.وجهو(پهريون_Iter_Trigger);
            }
            // Add other triggers
            إذا (varp->isWrittenByDpi())
                triggers.وجهو(trigKit.extra_Trigger_اشاريو(dpi_Export_Trigger_اشاريو));
            إذا (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                جوڙيو_Triggered_Iface(vscp, vif_Vscp_منزل_Trig_Ico, virtIfaceTriggers, triggers);
            }
        });
    util::ورهايو_Check(icoFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::ICO, پهريون_Iteration_Trigger, trigKit,
        // Use trigger
        trigKit.vscp(), خالي_اشارو,
        // Prep statements: Compute the current 'ico' triggers
        [&trigKit] {
            Ast_ڳنڍ_Stmt* ثابت stmtp = trigKit.new_Comp_بنياد_Call();
            إذا (stmtp) stmtp->جوڙيو_ايندڙ(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), سچ));
            أرجع stmtp;
        }(),
        // Work statements: Invoke the 'ico' function
        util::callVoidFunc(icoFuncp));

    // Release temporary input change detect SenTrees
    لكل (Ast_Sen_وڻ* ثابت senTreep : icoChangeSenTreeps)
        VL_DO_DANGLING(senTreep->delete_وڻ(), senTreep);
    icoChangeSenTreeps.clear();
}

//============================================================================
// EvalKit groups items that have to be passed to createEval() for a given eval region

بنية EvalKit final {
    // The AstVarScope representing the region's trigger vector
    AstVarScope* ثابت m_vscp = خالي_اشارو;
    // The AstCFunc that evaluates the region's logic
    AstCFunc* ثابت m_funcp = خالي_اشارو;
    // Is this kit used/required?
    منطقي خالي_جايون() ثابت { أرجع !m_funcp; }
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
    File_سٽ* ثابت flp = netlistp->fileline();

    // Grab the delay scheduler variable, if any
    AstVarScope* ثابت delaySchedVscp = timingKit.getDelayScheduler(netlistp);

    // 'createResume' consumes the contents that 'createReady' needs, so do the right order
    AstCCall* ثابت timingReadyp = timingKit.createReady(trigKit.vscp());
    AstCCall* ثابت timingResumep = timingKit.createResume(trigKit.vscp());

    // Create the 'act' region
    createEvalRegion(  //
        netlistp, VEval::ACT, 0, trigKit,
        // Use trigger
        actKit.m_vscp, خالي_اشارو,
        // Prep statements
        [&]() {
            // Compute the current 'act' triggers - the NBA triggers are the latched value
            Ast_ڳنڍ_Stmt* stmtsp = trigKit.new_Comp_بنياد_Call();
            Ast_ڳنڍ_Stmt* ثابت dumpp
                = stmtsp ? trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), سچ) : خالي_اشارو;
            // Mark as ready for triggered awaits
            إذا (timingReadyp) stmtsp = Ast_ڳنڍ::جوڙيو_ايندڙ(stmtsp, timingReadyp->makeStmt());
            إذا (AstVarScope* ثابت vscAccp = trigKit.vscAccp()) {
                stmtsp = Ast_ڳنڍ::جوڙيو_ايندڙ(stmtsp, trigKit.newOrIntoCall(actKit.m_vscp, vscAccp));
            }
            stmtsp = Ast_ڳنڍ::جوڙيو_ايندڙ(stmtsp, trigKit.newCompExtCall(nbaKit.m_vscp));
            stmtsp = Ast_ڳنڍ::جوڙيو_ايندڙ(stmtsp, dumpp);
            // Latch the 'act' triggers under the 'nba' triggers
            stmtsp = Ast_ڳنڍ::جوڙيو_ايندڙ(stmtsp, trigKit.newOrIntoCall(nbaKit.m_vscp, actKit.m_vscp));
            //
            أرجع stmtsp;
        }(),
        // Work statements
        [&]() {
            Ast_ڳنڍ_Stmt* workp = خالي_اشارو;
            إذا (AstVarScope* ثابت actAccp = trigKit.vscAccp()) {
                AstCMethodHard* ثابت cCallp = جديد AstCMethodHard{
                    flp, جديد AstVarRef{flp, actAccp, VAccess::WRITE}, VCMethod::UNPACKED_FILL,
                    جديد AstConst{flp, AstConst::Unsized64{}, 0}};
                cCallp->dtype_سيٽ_Void();
                workp = Ast_ڳنڍ::جوڙيو_ايندڙ(workp, cCallp->makeStmt());
            }
            // Resume triggered timing schedulers
            إذا (timingResumep) workp = Ast_ڳنڍ::جوڙيو_ايندڙ(workp, timingResumep->makeStmt());
            // Invoke the 'act' function
            workp = Ast_ڳنڍ::جوڙيو_ايندڙ(workp, util::callVoidFunc(actKit.m_funcp));
            //
            أرجع workp;
        }());

    // Create the 'inact' region
    createEvalRegion(  //
        netlistp, VEval::INACT, 0, trigKit,
        // Use explicit condition
        خالي_اشارو,
        [&]() -> Ast_ڳنڍ_اظهار* {
            إذا (!delaySchedVscp) أرجع خالي_اشارو;
            // Run if any zero delays are pending
            Ast_ڳنڍ_اظهار* ثابت callp
                = جديد AstCMethodHard{flp, جديد AstVarRef{flp, delaySchedVscp, VAccess::READ},
                                     VCMethod::SCHED_AWAITING_ZERO_DELAY};
            callp->dtype_سيٽ_ٻٽو_انگ();
            أرجع callp;
        }(),
        // Prep statements
        خالي_اشارو,
        // Work statements
        [&]() -> Ast_ڳنڍ_Stmt* {
            إذا (!delaySchedVscp) {
                // Nothing to do if there are no delays at all in the design
                أرجع خالي_اشارو;
            } وإلا إذا (v3Global.استعمال_ڪري_Zero_Delay()) {
                // Resume processes watiting for #0 delay
                AstCMethodHard* ثابت callp = جديد AstCMethodHard{
                    flp, جديد AstVarRef{flp, delaySchedVscp, VAccess::READWRITE},
                    VCMethod::SCHED_RESUME_ZERO_DELAY};
                callp->dtype_سيٽ_Void();
                أرجع callp->makeStmt();
            } وإلا {
                // Assumption was that the design doesn't use #0 delays.
                // Die at run-time if it does.
                AstCStmt* ثابت stmtp = جديد AstCStmt{flp};
                ثابت File_سٽ* ثابت locp = netlistp->چوٽي_Modulep()->fileline();
                ثابت std::string& file = VIdProtect::protect(locp->filename());
                ثابت std::string& سٽ = std::منزل_string(locp->lineno());
                stmtp->جوڙيو("VL_FATAL_MT(\"" + V3_ٻاهريون_Formatter::quoteNameControls(file) + "\", "
                           + سٽ
                           + ", \"\", \"ZERODLY: Design Verilated with '--no-sched-zero-delay', "
                           + "but #0 delay executed at runtime\");");
                أرجع stmtp;
            }
        }());

    // Create the 'nba' region
    createEvalRegion(  //
        netlistp, VEval::NBA, 0, trigKit,
        // Use trigger
        nbaKit.m_vscp, خالي_اشارو,
        // Prep statements
        خالي_اشارو,
        // Work statements
        [&]() {
            Ast_ڳنڍ_Stmt* workp = خالي_اشارو;
            // Latch the 'nba' trigger flags under the following region's trigger flags
            إذا (!obsKit.خالي_جايون()) {
                workp = trigKit.newOrIntoCall(obsKit.m_vscp, nbaKit.m_vscp);
            } وإلا إذا (!reactKit.خالي_جايون()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, nbaKit.m_vscp);
            }
            // Invoke the 'nba' function
            workp = Ast_ڳنڍ::جوڙيو_ايندڙ(workp, util::callVoidFunc(nbaKit.m_funcp));
            // Clear the 'nba' triggers
            workp = Ast_ڳنڍ::جوڙيو_ايندڙ(workp, trigKit.newClearCall(nbaKit.m_vscp));
            //
            أرجع workp;
        }(),
        // Extra work (not conditional on having had a fired trigger)
        [&](AstVarScope* continuep) -> Ast_ڳنڍ_Stmt* {
            // Check if any dynamic NBAs are pending, if there are any in the design
            إذا (!netlistp->nbaEventp()) أرجع خالي_اشارو;
            AstVarScope* ثابت nbaEventp = netlistp->nbaEventp();
            AstVarScope* ثابت nbaEventTriggerp = netlistp->nbaEventTriggerp();
            UASSERT(nbaEventTriggerp, "NBA event trigger var should exist");
            netlistp->nbaEventp(خالي_اشارو);
            netlistp->nbaEventTriggerp(خالي_اشارو);

            // If a dynamic NBA is pending, clear the pending flag and fire the ready event
            AstIf* ثابت ifp = جديد AstIf{flp, جديد AstVarRef{flp, nbaEventTriggerp, VAccess::READ}};
            ifp->جوڙيو_Thensp(util::سيٽ_Var(continuep, 1));
            ifp->جوڙيو_Thensp(util::سيٽ_Var(nbaEventTriggerp, 0));
            AstCMethodHard* ثابت firep = جديد AstCMethodHard{
                flp, جديد AstVarRef{flp, nbaEventp, VAccess::WRITE}, VCMethod::EVENT_FIRE};
            firep->dtype_سيٽ_Void();
            ifp->جوڙيو_Thensp(firep->makeStmt());
            أرجع ifp;
        });

    // Create the 'obs' region
    createEvalRegion(  //
        netlistp, VEval::OBS, 0, trigKit,
        // Use trigger
        obsKit.m_vscp, خالي_اشارو,
        // Prep statements
        خالي_اشارو,
        // Work statements
        [&]() -> Ast_ڳنڍ_Stmt* {
            إذا (obsKit.خالي_جايون()) أرجع خالي_اشارو;
            Ast_ڳنڍ_Stmt* workp = خالي_اشارو;
            // Latch the Observed trigger flags under the Reactive trigger flags
            إذا (!reactKit.خالي_جايون()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, obsKit.m_vscp);
            }
            // Invoke the 'obs' function
            workp = Ast_ڳنڍ::جوڙيو_ايندڙ(workp, util::callVoidFunc(obsKit.m_funcp));
            // Clear the 'obs' triggers
            workp = Ast_ڳنڍ::جوڙيو_ايندڙ(workp, trigKit.newClearCall(obsKit.m_vscp));
            //
            أرجع workp;
        }());

    // Create the 'react' region
    createEvalRegion(  //
        netlistp, VEval::REACT, 0, trigKit,
        // Use trigger
        reactKit.m_vscp, خالي_اشارو,
        // Prep statements
        خالي_اشارو,
        // Work statements
        [&]() -> Ast_ڳنڍ_Stmt* {
            إذا (reactKit.خالي_جايون()) أرجع خالي_اشارو;
            // Invoke the 'react' function
            Ast_ڳنڍ_Stmt* workp = util::callVoidFunc(reactKit.m_funcp);
            // Clear the 'react' triggers
            workp = Ast_ڳنڍ::جوڙيو_ايندڙ(workp, trigKit.newClearCall(reactKit.m_vscp));
            أرجع workp;
        }());
}

}  // namespace

//============================================================================
// Helper that builds virtual interface trigger sentrees

VirtIfaceTriggers::VscpTrigMap VirtIfaceTriggers::make_Vscp_منزل_Trig_Map(ثابت TriggerKit& trigKit,
                                                                    uint32_t پهريون_اشاريو) ثابت {
    VscpTrigMap map;
    uint32_t اشاريو = پهريون_اشاريو;
    لكل (ثابت خودڪار& داخلا_2 : m_triggers) {
        map.emplace(داخلا_2.m_vscp, trigKit.extra_Trigger_اشاريو(اشاريو));
        ++اشاريو;
    }
    أرجع map;
}

//============================================================================
// Top level entry-point to scheduling

فراغ وقت_جدول(AstNetlist* netlistp) {
    ثابت خودڪار جوڙيو_سائيز_Stat = [](ثابت string& name, ثابت LogicByScope& lbs) {
        uint64_t سائيز = 0;
        lbs.foreachLogic([&](Ast_ڳنڍ* nodep) { سائيز += nodep->ڳنڍ_ڳڻپ(); });
        V3Stats::جوڙيو_Stat("Scheduling, " + name, سائيز);
    };

    // Step 2: Prepare external domains for timing and virtual interfaces
    // Create extra triggers for virtual interfaces
    ثابت خودڪار& virtIfaceTriggers = makeVirtIfaceTriggers(netlistp);
    // Resolve what covergroup reference formal arguments are bound to, which is visible here
    // but not from V3Order, where the reads through them must be modeled
    ثابت CovergroupRefBindings& cgRefBindings = makeCovergroupRefBindings(netlistp);
    // Prepare timing-related logic and external domains
    TimingKit timingKit = prepareTiming(netlistp);

    // Step 3: Gather and classify all logic in the design
    LogicClasses logicClasses = gatherLogicClasses(netlistp);

    إذا (v3Global.opt.stats()) {
        V3Stats::statsStage("sched-gather");
        جوڙيو_سائيز_Stat("size of class: static", logicClasses.m_static);
        جوڙيو_سائيز_Stat("size of class: initial", logicClasses.m_initial);
        جوڙيو_سائيز_Stat("size of class: final", logicClasses.m_final);
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
    logicClasses.m_hybrid = break_چڪرن(netlistp, logicClasses.m_comb);
    إذا (v3Global.opt.stats()) {
        جوڙيو_سائيز_Stat("size of class: clocked", logicClasses.m_clocked);
        جوڙيو_سائيز_Stat("size of class: combinational", logicClasses.m_comb);
        جوڙيو_سائيز_Stat("size of class: hybrid", logicClasses.m_hybrid);
        V3Stats::statsStage("sched-break-cycles");
    }

    // We pass around a single SenExprBuilder instance, as we only need one set of 'prev' variables
    // for edge/change detection in sensitivity expressions, which this keeps track of.
    Ast_چوٽي_Scope* ثابت چوٽي_Scopep = netlistp->چوٽي_Scopep();
    AstScope* ثابت scopeTopp = چوٽي_Scopep->scopep();
    Sen_اظهار_Builder sen_اظهار_Builder{scopeTopp};

    // Step 6: Create 'settle' region that restores the combinational invariant
    createSettle(netlistp, staticp, sen_اظهار_Builder, logicClasses, cgRefBindings);
    إذا (v3Global.opt.stats()) V3Stats::statsStage("sched-settle");

    // Step 7: Partition the clocked and combinational (including hybrid) logic into pre/act/nba.
    // All clocks (signals referenced in an AstSenTree) generated via a blocking assignment
    // (including combinationally generated signals) are computed within the act region.
    LogicRegions logicRegions
        = حصا_ڪريو(logicClasses.m_clocked, logicClasses.m_comb, logicClasses.m_hybrid);
    logicRegions.m_obs = logicClasses.m_observed;
    logicRegions.m_react = logicClasses.m_reactive;
    إذا (v3Global.opt.stats()) {
        جوڙيو_سائيز_Stat("size of region: Active Pre", logicRegions.m_pre);
        جوڙيو_سائيز_Stat("size of region: Active", logicRegions.m_act);
        جوڙيو_سائيز_Stat("size of region: NBA", logicRegions.m_nba);
        جوڙيو_سائيز_Stat("size of region: Observed", logicRegions.m_obs);
        جوڙيو_سائيز_Stat("size of region: Reactive", logicRegions.m_react);
        V3Stats::statsStage("sched-partition");
    }

    // Step 8: Replicate combinational logic
    LogicReplicas logicReplicas = replicateLogic(logicRegions);
    إذا (v3Global.opt.stats()) {
        جوڙيو_سائيز_Stat("size of replicated logic: Input", logicReplicas.m_ico);
        جوڙيو_سائيز_Stat("size of replicated logic: Active", logicReplicas.m_act);
        جوڙيو_سائيز_Stat("size of replicated logic: NBA", logicReplicas.m_nba);
        جوڙيو_سائيز_Stat("size of replicated logic: Observed", logicReplicas.m_obs);
        جوڙيو_سائيز_Stat("size of replicated logic: Reactive", logicReplicas.m_react);
        V3Stats::statsStage("sched-replicate");
    }

    // Step 9: Create the input combinational logic
    createIcoRegion(netlistp, staticp, sen_اظهار_Builder, logicReplicas.m_ico, virtIfaceTriggers,
                    cgRefBindings);
    إذا (v3Global.opt.stats()) V3Stats::statsStage("sched-create-ico");

    // Step 10: Create the triggers
    AstVarScope* ثابت dpiExportTriggerVscp = netlistp->dpiExportTriggerp();
    netlistp->dpiExportTriggerp(خالي_اشارو);  // Finished with this here

    // We may have an extra trigger for variable updated in DPI exports
    TriggerKit::ExtraTriggers extraTriggers;
    ثابت uint32_t dpi_Export_Trigger_اشاريو = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    ثابت uint32_t پهريون_Vif_Trigger_اشاريو = extraTriggers.سائيز();
    لكل (ثابت خودڪار& داخلا_2 : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + داخلا_2.m_ifacep->name() + "."
                               + داخلا_2.m_memberp->name());
    }

    ثابت خودڪار& preTreeps = get_Sen_Trees_استعمال_ٿيل_By({&logicRegions.m_pre});
    ثابت خودڪار& senTreeps = get_Sen_Trees_استعمال_ٿيل_By({&logicRegions.m_act,  //
                                               &logicRegions.m_nba,  //
                                               &logicRegions.m_obs,  //
                                               &logicRegions.m_react,  //
                                               &timingKit.m_lbs});
    ثابت TriggerKit trigKit
        = TriggerKit::create(netlistp, staticp, sen_اظهار_Builder, preTreeps, senTreeps, "act",
                             extraTriggers, خطأ, v3Global.استعمال_ڪري_Timing());

    // Add post updates from the timing kit
    إذا (timingKit.m_postUpdates) trigKit.compBasep()->جوڙيو_Stmtsp(timingKit.m_postUpdates);

    إذا (dpiExportTriggerVscp) {
        trigKit.جوڙيو_Extra_Trigger_Assignment(dpiExportTriggerVscp, dpi_Export_Trigger_اشاريو);
    }
    جوڙيو_Virt_Iface_Trigger_Assignments(netlistp, staticp, virtIfaceTriggers, پهريون_Vif_Trigger_اشاريو,
                                   trigKit);
    إذا (v3Global.opt.stats()) V3Stats::statsStage("sched-create-triggers");

    // Note: Experiments so far show that running the Act (or Ico) regions on
    // multiple threads is always a net loss, so only use multi-threading for
    // NBA for now. This can be revised if evidence is available that it would
    // be beneficial

    // Step 11: Create the 'act' region evaluation function

    // Add the trigger indices of the input logic
    جوڙيو_Trigger_Indices(logicRegions.m_pre, trigKit, سچ);
    جوڙيو_Trigger_Indices(logicRegions.m_act, trigKit, خطأ);
    جوڙيو_Trigger_Indices(logicReplicas.m_act, trigKit, خطأ);
    جوڙيو_Trigger_Indices(timingKit.m_lbs, trigKit, خطأ);

    // External triggers of variables. Trigger indices are the same in all regions, so share.
    ثابت std::map<ثابت AstVarScope*, std::ويڪٽر<uint32_t>> timingTriggers
        = timingKit.externalTriggerIndices(trigKit);
    ثابت خودڪار& vif_Vscp_منزل_Trig = virtIfaceTriggers.make_Vscp_منزل_Trig_Map(trigKit, پهريون_Vif_Trigger_اشاريو);
    ثابت V3_ترتيب::ExternalTriggersProvider externalTriggers
        = [&](ثابت AstVarScope* vscp, std::سيٽ<uint32_t>& triggers) {
              خودڪار it = timingTriggers.ڳوليو(vscp);
              إذا (it != timingTriggers.پڇاڙي())
                  triggers.وجهو(it->ٻيو.begin(), it->ٻيو.پڇاڙي());
              إذا (vscp->varp()->isWrittenByDpi()) {
                  triggers.وجهو(trigKit.extra_Trigger_اشاريو(dpi_Export_Trigger_اشاريو));
              }
              إذا (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                  جوڙيو_Triggered_Iface(vscp, vif_Vscp_منزل_Trig, virtIfaceTriggers, triggers);
              }
          };

    AstCFunc* ثابت actFuncp
        = V3_ترتيب::ترتيب({&logicRegions.m_pre, &logicRegions.m_act, &logicReplicas.m_act}, trigKit,
                         trigKit.vscp(), cgRefBindings, "act", خطأ, خطأ, externalTriggers);
    util::ورهايو_Check(actFuncp);
    إذا (v3Global.opt.stats()) V3Stats::statsStage("sched-create-act");

    ثابت EvalKit actKit{trigKit.vscp(), actFuncp};

    // Orders a region's logic and creates the region eval function
    ثابت خودڪار ترتيب = [&](ثابت std::string& name,
                           ثابت std::ويڪٽر<V3Sched::LogicByScope*>& logic) -> EvalKit {
        UINFO(2, "Scheduling " << name << " #logic = " << logic.سائيز());
        AstVarScope* ثابت trigVscp = trigKit.newTrigVec(name);
        // Add the trigger indices of the input logic
        لكل (LogicByScope* lbs : logic) جوڙيو_Trigger_Indices(*lbs, trigKit, خطأ);

        AstCFunc* ثابت funcp
            = V3_ترتيب::ترتيب(logic, trigKit, trigVscp, cgRefBindings, name,
                             name == "nba" && v3Global.opt.mtasks(), خطأ, externalTriggers);

        أرجع {trigVscp, funcp};
    };

    // Step 12: Create the 'nba' region evaluation function
    ثابت EvalKit nbaKit = ترتيب("nba", {&logicRegions.m_nba, &logicReplicas.m_nba});
    util::ورهايو_Check(nbaKit.m_funcp);
    إذا (v3Global.opt.stats()) V3Stats::statsStage("sched-create-nba");

    // Orders a region's logic and creates the region eval function (only if there is any logic in
    // the region)
    ثابت خودڪار ترتيب_If_Non_خالي_جايون
        = [&](ثابت std::string& name, ثابت std::ويڪٽر<LogicByScope*>& logic) -> EvalKit {
        إذا (logic[0]->خالي_جايون())
            أرجع {};  // if region is empty, replica is supposed to be empty as well
        ثابت خودڪار& kit = ترتيب(name, logic);
        إذا (v3Global.opt.stats()) V3Stats::statsStage("sched-create-" + name);
        أرجع kit;
    };

    // Step 13: Create the 'obs' region evaluation function
    ثابت EvalKit obsKit = ترتيب_If_Non_خالي_جايون("obs", {&logicRegions.m_obs, &logicReplicas.m_obs});

    // Step 14: Create the 'react' region evaluation function
    ثابت EvalKit reactKit
        = ترتيب_If_Non_خالي_جايون("react", {&logicRegions.m_react, &logicReplicas.m_react});

    // Step 15: Create the 'postponed' region evaluation function
    createPostponed(netlistp, logicClasses);

    // Step 16: Populate the eval entry point function of each region of a time step
    createEval(netlistp, trigKit, actKit, nbaKit, obsKit, reactKit, timingKit);

    // Step 17: Add neccessary evaluation before awaits
    إذا (AstCCall* ثابت readyp = timingKit.createReady(trigKit.vscp())) {
        staticp->جوڙيو_Stmtsp(readyp->makeStmt());
        beforeTrigVisitor(netlistp, sen_اظهار_Builder, trigKit);
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
            trigAccDTypep->ساڄي() == 0, trigAccp,
            "Expected that trigger vector and accumulator start elements enumeration from 0");
        UASSERT_OBJ(trigAccDTypep->کاٻي() >= 0, trigAccp,
                    "Expected that trigger vector and accumulator has no negative indexes");
        File_سٽ* ثابت flp = trigAccp->fileline();
        AstVarScope* ثابت vscp = netlistp->چوٽي_Scopep()->create_عارضي("__Vi", 32);
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        loopp->جوڙيو_Stmtsp(
            جديد Ast_سونپيو{flp,
                          جديد AstArraySel{flp, جديد AstVarRef{flp, trigAccp, VAccess::WRITE},
                                          جديد AstVarRef{flp, vscp, VAccess::READ}},
                          جديد AstArraySel{flp, جديد AstVarRef{flp, actKit.m_vscp, VAccess::READ},
                                          جديد AstVarRef{flp, vscp, VAccess::READ}}});
        loopp->جوڙيو_Stmtsp(util::incrementVar(vscp));
        loopp->جوڙيو_Stmtsp(جديد AstLoopTest{
            flp, loopp,
            جديد AstLte{flp, جديد AstVarRef{flp, vscp, VAccess::READ},
                       جديد AstConst{flp, AstConst::Widthed_قدر{}, 32,
                                    static_cast<uint32_t>(trigAccDTypep->کاٻي())}}});
        staticp->جوڙيو_Stmtsp(loopp);
    }

    // Step 18: Clean up
    netlistp->clear_Stl_پهريون_Iterationp();

    إذا (v3Global.opt.stats()) {
        // A sample() call resolved to the union over its covergroup's constructions reads more
        // than it can, which orders it against more logic than necessary
        V3Stats::جوڙيو_Stat("Scheduling, covergroup ref sample calls, per instance",
                         cgRefBindings.انگ_Exact_Calls());
        V3Stats::جوڙيو_Stat("Scheduling, covergroup ref sample calls, per type",
                         cgRefBindings.انگ_Union_Calls());
    }

    // Haven't split static initializer yet
    util::ورهايو_Check(staticp);

    // Record eval stats
    netlistp->جوڙيو_Eval_Stats("sched");

    // Dump
    V3Global::dump_Check_Global_وڻ("sched", 0, dump_وڻ_Either_Level() >= 3);
}

}  // namespace V3Sched
