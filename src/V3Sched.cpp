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

std::وکتور<ثابت Ast_Sen_درخت*> get_Sen_Trees_استفاده_شده_By(ثابت std::وکتور<ثابت LogicByScope*>& lbsps) {
    ثابت VNUser1_درون_Use user1_درون_Use;
    std::وکتور<ثابت Ast_Sen_درخت*> نتیجه;
    لكل (ثابت LogicByScope* ثابت lbsp : lbsps) {
        لكل (ثابت خودکار& مدخل : *lbsp) {
            Ast_فعال* ثابت activep = مدخل.m_activep;
            Ast_Sen_درخت* ثابت senTreep = activep->sentreep();
            إذا (senTreep->user1_مجموعه_Once()) استمر;
            إذا (senTreep->hasClocked() || senTreep->hasHybrid()) نتیجه.افزودن_پشت(senTreep);
        }
    }
    أرجع نتیجه;
}

فراغ جمع_کن_Trigger_Indices(LogicByScope& lbs, ثابت TriggerKit& trigKit, منطقي pre) {
    لكل (LogicByScope::مدخل_2& مدخل : lbs) {
        Ast_Sen_درخت* ثابت senTreep = مدخل.m_activep->sentreep();
        إذا (senTreep->hasCombo()) استمر;
        مدخل.m_triggers = trigKit.triggerIndices(senTreep, pre);
    }
}

// Add the VIF triggers that a given VarScope should be sensitive to.
// Case 2 (non-virtual interface read): sensitive to that specific VarScope's trigger only
// Case 3 (virtual interface read): sensitive to all member triggers of the same interface type
فراغ جمع_کن_Triggered_Iface(ثابت AstVarScope* vscp, ثابت VirtIfaceTriggers::VscpTrigMap& vscp_مقصد_Trig,
                       ثابت VirtIfaceTriggers& virtIfaceTriggers, std::مجموعه<uint32_t>& triggers) {
    إذا (vscp->varp()->isVirtIface()) {
        // Virtual interface variable -- sensitive to all member triggers of this interface type
        ثابت AstIface* ثابت ifacep = VN_AS(vscp->varp()->dtypep(), IfaceRefDType)->ifacep();
        لكل (ثابت خودکار& مدخل : virtIfaceTriggers.m_triggers) {
            إذا (مدخل.m_ifacep == ifacep) {
                ثابت خودکار it = vscp_مقصد_Trig.پیدا_کن(مدخل.m_vscp);
                إذا (it != vscp_مقصد_Trig.انتها()) triggers.درج(it->دومی);
            }
        }
    } وإلا {
        // Non-virtual interface member -- sensitive to this VarScope's trigger only
        ثابت خودکار it = vscp_مقصد_Trig.پیدا_کن(vscp);
        إذا (it != vscp_مقصد_Trig.انتها()) triggers.درج(it->دومی);
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
    uint32_t اولی_Iter_Trigger,
    ثابت TriggerKit& trigKit,  // The trigger kit
    AstVarScope* trigp,  // The trigger vector - may be nullptr if no triggers or using 'condp'
    Ast_گره_عبارت* condp,  // Explicit condition that must be true to run 'phaseWorkp'
    Ast_گره_Stmt* phasePrepp,  // Prep statements run before checking triggers
    Ast_گره_Stmt* phaseWorkp,  // The work to do if anything triggered
    // Extra statements to run after the work, even if no triggers fired. This function is
    // passed a variable, which must be set to true if we must continue and loop again,
    // and must be unmodified otherwise.
    std::function<Ast_گره_Stmt*(AstVarScope*)> phaseExtra = [](AstVarScope*) { أرجع اشاره_تهی; }  //
) {
    UASSERT(!trigp || !condp, "Cannot use both 'trigp' and 'condp' in 'createEvalRegion'");
    UASSERT(!eval.اولی_Iteration() || trigp,
            "Region without triggers cannot need a first iteration flag");

    // All work is under a trigger or condition, so with neither the region has
    // nothing to evaluate, and what we create below reduces to a no-op function.

    ثابت std::string برچسب = eval.برچسب();
    Ast_بالاترین_Scope* ثابت بالاترین_Scopep = netlistp->بالاترین_Scopep();
    File_سطر* ثابت flp = netlistp->fileline();

    // Populate the trigger dump entry point function
    إذا (trigp) {
        netlistp->dumpTriggersFuncp(eval)->جمع_کن_Stmtsp(trigKit.newDumpCall(trigp, برچسب, خطأ));
    }

    AstCFunc* ثابت funcp = netlistp->evalFuncp(eval);

    // A flag is passed from the run-time eval loop if this is the first iteration of the
    // current loop
    إذا (eval.اولی_Iteration()) {
        AstVarScope* ثابت اولی_Iter_Argp = util::new_آرگومان(funcp, netlistp->پیدا_کن_بیت_DType(),
                                                             "firstIteration", VDirection::درون_داد);

        // Set the region's 'first iteration' trigger straight from the argument
        funcp->جمع_کن_Stmtsp(trigKit.newExtraTriggerAssignment(اولی_Iter_Argp, اولی_Iter_Trigger));

        // Only 'stl' also needs a module level flag, for design logic that reads
        // it directly (see V3Timing). Those reads can be anywhere in the design,
        // hence module level. Always created, even if nothing reads it.
        // TODO: get rid of this special case
        إذا (eval == VEval::STL) {
            AstVarScope* ثابت اولی_Iterp = netlistp->stl_اولی_Iterationp();
            اولی_Iterp->varp()->noReset(درست);
            اولی_Iterp->varp()->is_داخلی(درست);
            funcp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{flp, جديد AstVarRef{flp, اولی_Iterp, VAccess::WRITE},
                                           جديد AstVarRef{flp, اولی_Iter_Argp, VAccess::READ}});
        }
    }

    {
        // Add the preparatory statements
        funcp->جمع_کن_Stmtsp(phasePrepp);

        // The execute flag
        AstVarScope* ثابت executeFlagp = بالاترین_Scopep->create_موقت("__V" + برچسب + "Execute", 1);
        executeFlagp->varp()->noReset(درست);

        // If there is work in this region, execute it if any triggers fired
        إذا (phaseWorkp) {
            Ast_گره_عبارت* ثابت lhsp = جديد AstVarRef{flp, executeFlagp, VAccess::WRITE};
            // If using explicit condition, that directly determines whether to execute,
            // otherwise check if any triggers are fired
            Ast_گره_عبارت* ثابت rhsp = condp ? condp : trigKit.new_Any_مجموعه_Call(trigp);
            funcp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{flp, lhsp, rhsp});

            // Add the work
            AstIf* ثابت ifp = جديد AstIf{flp, جديد AstVarRef{flp, executeFlagp, VAccess::READ}};
            ifp->جمع_کن_Thensp(phaseWorkp);
            funcp->جمع_کن_Stmtsp(ifp);
        }

        // Construct the extra statements
        Ast_گره_Stmt* ثابت extraWorkp = phaseExtra(executeFlagp);
        إذا (extraWorkp) funcp->جمع_کن_Stmtsp(extraWorkp);

        // The function returns true iff it did run work
        Ast_گره_عبارت* ثابت retp
            = phaseWorkp || extraWorkp
                  ? static_cast<Ast_گره_عبارت*>(جديد AstVarRef{flp, executeFlagp, VAccess::READ})
                  : static_cast<Ast_گره_عبارت*>(جديد AstConst{flp, AstConst::بیت_False{}});
        funcp->جمع_کن_Stmtsp(جديد AstCReturn{flp, retp});
    }
}

//============================================================================
// Collect and classify all logic in the design

LogicClasses gatherLogicClasses(AstNetlist* netlistp) {
    LogicClasses نتیجه;

    netlistp->foreach([&](AstScope* scopep) {
        scopep->foreach([&](Ast_فعال* activep) {
            Ast_Sen_درخت* ثابت senTreep = activep->sentreep();
            إذا (senTreep->hasStatic()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "static initializer with additional sensitivities");
                نتیجه.m_static.جمع_کن(scopep, activep);
            } وإلا إذا (senTreep->hasInitial()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'initial' logic with additional sensitivities");
                نتیجه.m_initial.جمع_کن(scopep, activep);
            } وإلا إذا (senTreep->hasFinal()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "'final' logic with additional sensitivities");
                نتیجه.m_final.جمع_کن(scopep, activep);
            } وإلا إذا (senTreep->hasCombo()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), activep,
                            "combinational logic with additional sensitivities");
                إذا (VN_IS(activep->stmtsp(), AlwaysPostponed)) {
                    نتیجه.m_postponed.جمع_کن(scopep, activep);
                } وإلا {
                    نتیجه.m_comb.جمع_کن(scopep, activep);
                }
            } وإلا {
                UASSERT_OBJ(senTreep->hasClocked(), activep, "What else could it be?");
                إذا (VN_IS(activep->stmtsp(), AlwaysObserved)) {
                    نتیجه.m_observed.جمع_کن(scopep, activep);
                } وإلا إذا (VN_IS(activep->stmtsp(), AlwaysReactive)) {
                    نتیجه.m_reactive.جمع_کن(scopep, activep);
                } وإلا {
                    نتیجه.m_clocked.جمع_کن(scopep, activep);
                }
            }
        });
    });

    أرجع نتیجه;
}

//============================================================================
// Simple ordering in source order

فراغ ترتیب_Sequentially(AstCFunc* funcp, ثابت LogicByScope& lbs) {
    // Create new subfunc for scope
    ثابت خودکار createNewSubFuncp = [&](AstScope* ثابت scopep) {
        ثابت string subName{funcp->name() + "__" + scopep->nameDotless()};
        AstCFunc* ثابت subFuncp = جديد AstCFunc{scopep->fileline(), subName, scopep};
        subFuncp->isLoose(درست);
        subFuncp->isConst(خطأ);
        subFuncp->decl_خصوصی(درست);
        subFuncp->slow(funcp->slow());
        scopep->جمع_کن_Blocksp(subFuncp);
        // Call it from the top function
        funcp->جمع_کن_Stmtsp(util::callVoidFunc(subFuncp));
        أرجع subFuncp;
    };
    ثابت VNUser1_درون_Use user1_درون_Use;  // AstScope -> AstCFunc: the sub-function for the scope
    ثابت VNUser2_درون_Use user2_درون_Use;  // AstScope -> int: sub-function counter used for names
    لكل (ثابت خودکار& مدخل : lbs) {
        AstScope* ثابت scopep = مدخل.m_scopep;
        Ast_فعال* ثابت activep = مدخل.m_activep;
        // Create a sub-function per scope so we can V3Combine them later
        إذا (!scopep->user1p()) scopep->user1p(createNewSubFuncp(scopep));
        // Add statements to sub-function
        لكل (Ast_گره *logicp = activep->stmtsp(), *nextp; logicp; logicp = nextp) {
            خودکار* subFuncp = VN_AS(scopep->user1p(), CFunc);
            nextp = logicp->nextp();
            إذا (Ast_گره_Procedure* ثابت procp = VN_CAST(logicp, گره_Procedure)) {
                إذا (Ast_گره* bodyp = procp->stmtsp()) {
                    bodyp->unlink_Fr_پشت_With_بعدی();
                    // If the process is suspendable, we need a separate function (a coroutine)
                    إذا (procp->isSuspendable()) {
                        funcp->slow(خطأ);
                        subFuncp = createNewSubFuncp(scopep);
                        subFuncp->name(subFuncp->name() + "__Vtiming__"
                                       + cvt_مقصد_Str(scopep->user2Inc()));
                        subFuncp->rtnType("VlCoroutine");
                        إذا (VN_IS(procp, Always)) {
                            subFuncp->slow(خطأ);
                            File_سطر* ثابت flp = procp->fileline();
                            Ast_گره_عبارت* ثابت condp = جديد AstCExpr{
                                flp, "VL_LIKELY(!vlSymsp->_vm_contextp__->gotFinish())", 1};
                            AstLoop* ثابت loopp = جديد AstLoop{flp};
                            loopp->جمع_کن_Stmtsp(جديد AstLoopTest{flp, loopp, condp});
                            loopp->جمع_کن_Stmtsp(bodyp);
                            bodyp = loopp;
                        }
                    }
                    subFuncp->جمع_کن_Stmtsp(bodyp);
                    إذا (procp->needProcess()) subFuncp->مجموعه_Need_Process();
                    // A coroutine holds a single process, so it is complete here
                    إذا (procp->isSuspendable()) util::تقسیم_Check(subFuncp);
                }
            } وإلا {
                logicp->unlink_Fr_پشت();
                subFuncp->جمع_کن_Stmtsp(logicp);
            }
        }
        إذا (activep->backp()) activep->unlink_Fr_پشت();
        VL_DO_DANGLING(activep->delete_درخت(), activep);
    }
    // Split sub-functions once complete
    لكل (ثابت خودکار& مدخل : lbs) {
        AstScope* ثابت scopep = مدخل.m_scopep;
        إذا (AstCFunc* ثابت subFuncp = VN_AS(scopep->user1p(), CFunc)) {
            util::تقسیم_Check(subFuncp);
            scopep->user1p(اشاره_تهی);
        }
    }
}

//============================================================================
// Create simply ordered functions

AstCFunc* createStatic(AstNetlist* netlistp, ثابت LogicClasses& logicClasses) {
    AstCFunc* ثابت funcp = netlistp->evalFuncp(VEval::STATIC);

    ثابت LogicByScope& orig = logicClasses.m_static;
    إذا (orig.اندازه() <= 1) {
        ترتیب_Sequentially(funcp, orig);
        أرجع funcp;
    }

    // Level-based module sorting can reorder packages so that an importing
    // package runs before the imported one.  Re-sort package entries by source
    // file position to restore compilation order (IEEE 1800-2023 26.3).
    std::وکتور<اندازه_t> indices(orig.اندازه());
    لكل (اندازه_t i = 0; i < orig.اندازه(); ++i) indices[i] = i;
    std::stable_مرتب_سازی(indices.begin(), indices.انتها(), [&](اندازه_t a, اندازه_t b) {
        ثابت Ast_گره_Module* ثابت modA = orig[a].m_scopep->modp();
        ثابت Ast_گره_Module* ثابت modB = orig[b].m_scopep->modp();
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
    لكل (ثابت اندازه_t i : indices) sorted.جمع_کن(orig[i].m_scopep, orig[i].m_activep);

    ترتیب_Sequentially(funcp, sorted);
    أرجع funcp;  // Not splitting yet as it is not final
}

فراغ createInitial(AstNetlist* netlistp, ثابت LogicClasses& logicClasses) {
    AstCFunc* ثابت funcp = netlistp->evalFuncp(VEval::INITIAL);
    ترتیب_Sequentially(funcp, logicClasses.m_initial);
    util::تقسیم_Check(funcp);
}

فراغ createPostponed(AstNetlist* netlistp, ثابت LogicClasses& logicClasses) {
    AstCFunc* ثابت funcp = netlistp->evalFuncp(VEval::POSTPONED);
    ترتیب_Sequentially(funcp, logicClasses.m_postponed);
    util::تقسیم_Check(funcp);
}

فراغ createFinal(AstNetlist* netlistp, ثابت LogicClasses& logicClasses) {
    AstCFunc* ثابت funcp = netlistp->evalFuncp(VEval::FINAL);
    ترتیب_Sequentially(funcp, logicClasses.m_final);
    util::تقسیم_Check(funcp);
}

//============================================================================
// Helper that creates virtual interface value-change triggers

فراغ جمع_کن_Virt_Iface_Trigger_Assignments(AstNetlist* netlistp, AstCFunc* initFuncp,
                                    ثابت VirtIfaceTriggers& virtIfaceTriggers,
                                    uint32_t اولی_شاخص, ثابت TriggerKit& trigKit) {
    uint32_t شاخص = اولی_شاخص;
    لكل (ثابت خودکار& مدخل : virtIfaceTriggers.m_triggers) {
        trigKit.جمع_کن_مقدار_Change_Trigger_Assignment(netlistp, initFuncp, مدخل.m_vscp, شاخص);
        ++شاخص;
    }
}

// Order the combinational logic to create the 'stl' region
فراغ createSettle(AstNetlist* netlistp, AstCFunc* ثابت initFuncp, Sen_عبارت_Builder& sen_عبارت_Bulider,
                  LogicClasses& logicClasses, ثابت CovergroupRefBindings& cgRefBindings) {
    // Clone, because ordering is destructive, but we still need them for the other regions
    LogicByScope comb = logicClasses.m_comb.clone();
    LogicByScope hybrid = logicClasses.m_hybrid.clone();

    // We have an extra trigger denoting this is the first iteration of the settle loop
    TriggerKit::ExtraTriggers extraTriggers;
    ثابت uint32_t اولی_Iteration_Trigger = extraTriggers.allocate("first iteration");

    // Gather the relevant sensitivity expressions and create the trigger kit
    ثابت خودکار& senTreeps = get_Sen_Trees_استفاده_شده_By({&comb, &hybrid});
    ثابت TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_عبارت_Bulider, {},
                                                  senTreeps, "stl", extraTriggers, درست, خطأ);

    // Add trigger indices (comb has none, so only do the hybrid)
    جمع_کن_Trigger_Indices(hybrid, trigKit, خطأ);

    // First trigger is for pure combinational triggers (first iteration)
    ثابت uint32_t درون_داد_تغییریافته = trigKit.extra_Trigger_شاخص(اولی_Iteration_Trigger);

    // Create and the body function
    AstCFunc* ثابت stlFuncp = V3_ترتیب::ترتیب(
        {&comb, &hybrid}, trigKit, trigKit.vscp(), cgRefBindings, "stl", خطأ, درست,
        [=](ثابت AstVarScope*, std::مجموعه<uint32_t>& triggers) { triggers.درج(درون_داد_تغییریافته); });
    util::تقسیم_Check(stlFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::STL, اولی_Iteration_Trigger, trigKit,
        // Use trigger
        trigKit.vscp(), اشاره_تهی,
        // Explicit condition
        // Prep statements: Compute the current 'stl' triggers
        [&trigKit] {
            Ast_گره_Stmt* ثابت stmtp = trigKit.new_Comp_پایه_Call();
            إذا (stmtp) stmtp->جمع_کن_بعدی(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), درست));
            أرجع stmtp;
        }(),
        // Work statements: Invoke the 'stl' function
        util::callVoidFunc(stlFuncp));
}

//============================================================================
// Order the replicated combinational logic to create the 'ico' region

فراغ createIcoRegion(AstNetlist* netlistp, AstCFunc* ثابت initFuncp,
                     Sen_عبارت_Builder& sen_عبارت_Builder, LogicByScope& logic,
                     ثابت VirtIfaceTriggers& virtIfaceTriggers,
                     ثابت CovergroupRefBindings& cgRefBindings) {
    // SystemC only: Any top level inputs feeding a combinational logic must be marked,
    // so we can make them sc_sensitive
    إذا (v3Global.opt.systemC()) {
        logic.foreachLogic([](Ast_گره* logicp) {
            logicp->foreach([](AstVarRef* refp) {
                إذا (refp->access().isWriteOnly()) أرجع;
                AstVarScope* ثابت vscp = refp->varScopep();
                إذا (vscp->scopep()->is_بالاترین() && vscp->varp()->isNonOutput()) {
                    vscp->varp()->scSensitive(درست);
                }
            });
        });
    }

    // We have some extra trigger denoting external conditions
    AstVarScope* ثابت dpiExportTriggerVscp = netlistp->dpiExportTriggerp();

    TriggerKit::ExtraTriggers extraTriggers;
    ثابت uint32_t اولی_Iteration_Trigger = extraTriggers.allocate("first iteration");
    ثابت uint32_t dpi_Export_Trigger_شاخص = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    ثابت uint32_t اولی_Vif_Trigger_شاخص = extraTriggers.اندازه();
    لكل (ثابت خودکار& مدخل : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + مدخل.m_ifacep->name() + "."
                               + مدخل.m_memberp->name());
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
    std::unordered_map<ثابت AstVarScope*, Ast_Sen_درخت*> inp2changedp;
    std::وکتور<Ast_Sen_درخت*> icoChangeSenTreeps;
    إذا (v3Global.opt.fIcoChangeDetect().isTrue()) {
        File_سطر* ثابت flp = netlistp->fileline();
        AstScope* ثابت scopep = netlistp->بالاترین_Scopep()->scopep();
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
            Ast_Sen_درخت*& senTreepr = inp2changedp[vscp];
            UASSERT_OBJ(!senTreepr, vscp, "Duplicate input change detect trigger");
            AstVarRef* ثابت refp = جديد AstVarRef{flp, vscp, VAccess::READ};
            Ast_Sen_قلم* ثابت senItemp = جديد Ast_Sen_قلم{flp, VEdgeType::ET_تغییریافته, refp};
            senTreepr = جديد Ast_Sen_درخت{flp, senItemp};
            icoChangeSenTreeps.افزودن_پشت(senTreepr);
        }
    }
    V3Stats::جمع_کن_Stat("Scheduling, 'ico' change detect triggers", icoChangeSenTreeps.اندازه());

    // Gather the relevant sensitivity expressions and create the trigger kit
    std::وکتور<ثابت Ast_Sen_درخت*> senTreeps = get_Sen_Trees_استفاده_شده_By({&logic});
    senTreeps.درج(senTreeps.انتها(), icoChangeSenTreeps.begin(), icoChangeSenTreeps.انتها());
    ثابت TriggerKit trigKit = TriggerKit::create(netlistp, initFuncp, sen_عبارت_Builder, {},
                                                  senTreeps, "ico", extraTriggers, خطأ, خطأ);
    std::ignore = sen_عبارت_Builder.getAndClearResults();

    إذا (dpiExportTriggerVscp) {
        trigKit.جمع_کن_Extra_Trigger_Assignment(dpiExportTriggerVscp, dpi_Export_Trigger_شاخص);
    }
    جمع_کن_Virt_Iface_Trigger_Assignments(netlistp, initFuncp, virtIfaceTriggers, اولی_Vif_Trigger_شاخص,
                                   trigKit);

    // Add trigger indices
    جمع_کن_Trigger_Indices(logic, trigKit, خطأ);

    // The 'first iteration' trigger for top level inputs
    ثابت uint32_t اولی_Iter_Trigger = trigKit.extra_Trigger_شاخص(اولی_Iteration_Trigger);

    ثابت خودکار& vif_Vscp_مقصد_Trig_Ico
        = virtIfaceTriggers.make_Vscp_مقصد_Trig_Map(trigKit, اولی_Vif_Trigger_شاخص);

    // Create and Order the body function
    AstCFunc* ثابت icoFuncp = V3_ترتیب::ترتیب(
        {&logic}, trigKit, trigKit.vscp(), cgRefBindings, "ico", خطأ, خطأ,
        [&](ثابت AstVarScope* vscp, std::مجموعه<uint32_t>& triggers) {
            AstVar* ثابت varp = vscp->varp();
            // If it has an explicit change detect trigger, use that,
            // otherwise fall back to using the 'first iteration' trigger
            خودکار it = inp2changedp.پیدا_کن(vscp);
            إذا (it != inp2changedp.انتها()) {
                triggers.درج(trigKit.senItem2TrigIdx(it->دومی->sensesp()));
            } وإلا إذا (varp->isPrimaryInish() || varp->isSigUserRWPublic() || varp->sampled()) {
                triggers.درج(اولی_Iter_Trigger);
            }
            // Add other triggers
            إذا (varp->isWrittenByDpi())
                triggers.درج(trigKit.extra_Trigger_شاخص(dpi_Export_Trigger_شاخص));
            إذا (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                جمع_کن_Triggered_Iface(vscp, vif_Vscp_مقصد_Trig_Ico, virtIfaceTriggers, triggers);
            }
        });
    util::تقسیم_Check(icoFuncp);

    // Create the region evaluation function
    createEvalRegion(  //
        netlistp, VEval::ICO, اولی_Iteration_Trigger, trigKit,
        // Use trigger
        trigKit.vscp(), اشاره_تهی,
        // Prep statements: Compute the current 'ico' triggers
        [&trigKit] {
            Ast_گره_Stmt* ثابت stmtp = trigKit.new_Comp_پایه_Call();
            إذا (stmtp) stmtp->جمع_کن_بعدی(trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), درست));
            أرجع stmtp;
        }(),
        // Work statements: Invoke the 'ico' function
        util::callVoidFunc(icoFuncp));

    // Release temporary input change detect SenTrees
    لكل (Ast_Sen_درخت* ثابت senTreep : icoChangeSenTreeps)
        VL_DO_DANGLING(senTreep->delete_درخت(), senTreep);
    icoChangeSenTreeps.clear();
}

//============================================================================
// EvalKit groups items that have to be passed to createEval() for a given eval region

بنية EvalKit final {
    // The AstVarScope representing the region's trigger vector
    AstVarScope* ثابت m_vscp = اشاره_تهی;
    // The AstCFunc that evaluates the region's logic
    AstCFunc* ثابت m_funcp = اشاره_تهی;
    // Is this kit used/required?
    منطقي خالی() ثابت { أرجع !m_funcp; }
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
    File_سطر* ثابت flp = netlistp->fileline();

    // Grab the delay scheduler variable, if any
    AstVarScope* ثابت delaySchedVscp = timingKit.getDelayScheduler(netlistp);

    // 'createResume' consumes the contents that 'createReady' needs, so do the right order
    AstCCall* ثابت timingReadyp = timingKit.createReady(trigKit.vscp());
    AstCCall* ثابت timingResumep = timingKit.createResume(trigKit.vscp());

    // Create the 'act' region
    createEvalRegion(  //
        netlistp, VEval::ACT, 0, trigKit,
        // Use trigger
        actKit.m_vscp, اشاره_تهی,
        // Prep statements
        [&]() {
            // Compute the current 'act' triggers - the NBA triggers are the latched value
            Ast_گره_Stmt* stmtsp = trigKit.new_Comp_پایه_Call();
            Ast_گره_Stmt* ثابت dumpp
                = stmtsp ? trigKit.newDumpCall(trigKit.vscp(), trigKit.name(), درست) : اشاره_تهی;
            // Mark as ready for triggered awaits
            إذا (timingReadyp) stmtsp = Ast_گره::جمع_کن_بعدی(stmtsp, timingReadyp->makeStmt());
            إذا (AstVarScope* ثابت vscAccp = trigKit.vscAccp()) {
                stmtsp = Ast_گره::جمع_کن_بعدی(stmtsp, trigKit.newOrIntoCall(actKit.m_vscp, vscAccp));
            }
            stmtsp = Ast_گره::جمع_کن_بعدی(stmtsp, trigKit.newCompExtCall(nbaKit.m_vscp));
            stmtsp = Ast_گره::جمع_کن_بعدی(stmtsp, dumpp);
            // Latch the 'act' triggers under the 'nba' triggers
            stmtsp = Ast_گره::جمع_کن_بعدی(stmtsp, trigKit.newOrIntoCall(nbaKit.m_vscp, actKit.m_vscp));
            //
            أرجع stmtsp;
        }(),
        // Work statements
        [&]() {
            Ast_گره_Stmt* workp = اشاره_تهی;
            إذا (AstVarScope* ثابت actAccp = trigKit.vscAccp()) {
                AstCMethodHard* ثابت cCallp = جديد AstCMethodHard{
                    flp, جديد AstVarRef{flp, actAccp, VAccess::WRITE}, VCMethod::UNPACKED_FILL,
                    جديد AstConst{flp, AstConst::Unsized64{}, 0}};
                cCallp->dtype_مجموعه_Void();
                workp = Ast_گره::جمع_کن_بعدی(workp, cCallp->makeStmt());
            }
            // Resume triggered timing schedulers
            إذا (timingResumep) workp = Ast_گره::جمع_کن_بعدی(workp, timingResumep->makeStmt());
            // Invoke the 'act' function
            workp = Ast_گره::جمع_کن_بعدی(workp, util::callVoidFunc(actKit.m_funcp));
            //
            أرجع workp;
        }());

    // Create the 'inact' region
    createEvalRegion(  //
        netlistp, VEval::INACT, 0, trigKit,
        // Use explicit condition
        اشاره_تهی,
        [&]() -> Ast_گره_عبارت* {
            إذا (!delaySchedVscp) أرجع اشاره_تهی;
            // Run if any zero delays are pending
            Ast_گره_عبارت* ثابت callp
                = جديد AstCMethodHard{flp, جديد AstVarRef{flp, delaySchedVscp, VAccess::READ},
                                     VCMethod::SCHED_AWAITING_ZERO_DELAY};
            callp->dtype_مجموعه_بیت();
            أرجع callp;
        }(),
        // Prep statements
        اشاره_تهی,
        // Work statements
        [&]() -> Ast_گره_Stmt* {
            إذا (!delaySchedVscp) {
                // Nothing to do if there are no delays at all in the design
                أرجع اشاره_تهی;
            } وإلا إذا (v3Global.استفاده_میکند_Zero_Delay()) {
                // Resume processes watiting for #0 delay
                AstCMethodHard* ثابت callp = جديد AstCMethodHard{
                    flp, جديد AstVarRef{flp, delaySchedVscp, VAccess::READWRITE},
                    VCMethod::SCHED_RESUME_ZERO_DELAY};
                callp->dtype_مجموعه_Void();
                أرجع callp->makeStmt();
            } وإلا {
                // Assumption was that the design doesn't use #0 delays.
                // Die at run-time if it does.
                AstCStmt* ثابت stmtp = جديد AstCStmt{flp};
                ثابت File_سطر* ثابت locp = netlistp->بالاترین_Modulep()->fileline();
                ثابت std::string& file = VIdProtect::protect(locp->filename());
                ثابت std::string& سطر = std::مقصد_string(locp->lineno());
                stmtp->جمع_کن("VL_FATAL_MT(\"" + V3_بیرون_Formatter::quoteNameControls(file) + "\", "
                           + سطر
                           + ", \"\", \"ZERODLY: Design Verilated with '--no-sched-zero-delay', "
                           + "but #0 delay executed at runtime\");");
                أرجع stmtp;
            }
        }());

    // Create the 'nba' region
    createEvalRegion(  //
        netlistp, VEval::NBA, 0, trigKit,
        // Use trigger
        nbaKit.m_vscp, اشاره_تهی,
        // Prep statements
        اشاره_تهی,
        // Work statements
        [&]() {
            Ast_گره_Stmt* workp = اشاره_تهی;
            // Latch the 'nba' trigger flags under the following region's trigger flags
            إذا (!obsKit.خالی()) {
                workp = trigKit.newOrIntoCall(obsKit.m_vscp, nbaKit.m_vscp);
            } وإلا إذا (!reactKit.خالی()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, nbaKit.m_vscp);
            }
            // Invoke the 'nba' function
            workp = Ast_گره::جمع_کن_بعدی(workp, util::callVoidFunc(nbaKit.m_funcp));
            // Clear the 'nba' triggers
            workp = Ast_گره::جمع_کن_بعدی(workp, trigKit.newClearCall(nbaKit.m_vscp));
            //
            أرجع workp;
        }(),
        // Extra work (not conditional on having had a fired trigger)
        [&](AstVarScope* continuep) -> Ast_گره_Stmt* {
            // Check if any dynamic NBAs are pending, if there are any in the design
            إذا (!netlistp->nbaEventp()) أرجع اشاره_تهی;
            AstVarScope* ثابت nbaEventp = netlistp->nbaEventp();
            AstVarScope* ثابت nbaEventTriggerp = netlistp->nbaEventTriggerp();
            UASSERT(nbaEventTriggerp, "NBA event trigger var should exist");
            netlistp->nbaEventp(اشاره_تهی);
            netlistp->nbaEventTriggerp(اشاره_تهی);

            // If a dynamic NBA is pending, clear the pending flag and fire the ready event
            AstIf* ثابت ifp = جديد AstIf{flp, جديد AstVarRef{flp, nbaEventTriggerp, VAccess::READ}};
            ifp->جمع_کن_Thensp(util::مجموعه_Var(continuep, 1));
            ifp->جمع_کن_Thensp(util::مجموعه_Var(nbaEventTriggerp, 0));
            AstCMethodHard* ثابت firep = جديد AstCMethodHard{
                flp, جديد AstVarRef{flp, nbaEventp, VAccess::WRITE}, VCMethod::EVENT_FIRE};
            firep->dtype_مجموعه_Void();
            ifp->جمع_کن_Thensp(firep->makeStmt());
            أرجع ifp;
        });

    // Create the 'obs' region
    createEvalRegion(  //
        netlistp, VEval::OBS, 0, trigKit,
        // Use trigger
        obsKit.m_vscp, اشاره_تهی,
        // Prep statements
        اشاره_تهی,
        // Work statements
        [&]() -> Ast_گره_Stmt* {
            إذا (obsKit.خالی()) أرجع اشاره_تهی;
            Ast_گره_Stmt* workp = اشاره_تهی;
            // Latch the Observed trigger flags under the Reactive trigger flags
            إذا (!reactKit.خالی()) {
                workp = trigKit.newOrIntoCall(reactKit.m_vscp, obsKit.m_vscp);
            }
            // Invoke the 'obs' function
            workp = Ast_گره::جمع_کن_بعدی(workp, util::callVoidFunc(obsKit.m_funcp));
            // Clear the 'obs' triggers
            workp = Ast_گره::جمع_کن_بعدی(workp, trigKit.newClearCall(obsKit.m_vscp));
            //
            أرجع workp;
        }());

    // Create the 'react' region
    createEvalRegion(  //
        netlistp, VEval::REACT, 0, trigKit,
        // Use trigger
        reactKit.m_vscp, اشاره_تهی,
        // Prep statements
        اشاره_تهی,
        // Work statements
        [&]() -> Ast_گره_Stmt* {
            إذا (reactKit.خالی()) أرجع اشاره_تهی;
            // Invoke the 'react' function
            Ast_گره_Stmt* workp = util::callVoidFunc(reactKit.m_funcp);
            // Clear the 'react' triggers
            workp = Ast_گره::جمع_کن_بعدی(workp, trigKit.newClearCall(reactKit.m_vscp));
            أرجع workp;
        }());
}

}  // namespace

//============================================================================
// Helper that builds virtual interface trigger sentrees

VirtIfaceTriggers::VscpTrigMap VirtIfaceTriggers::make_Vscp_مقصد_Trig_Map(ثابت TriggerKit& trigKit,
                                                                    uint32_t اولی_شاخص) ثابت {
    VscpTrigMap map;
    uint32_t شاخص = اولی_شاخص;
    لكل (ثابت خودکار& مدخل : m_triggers) {
        map.emplace(مدخل.m_vscp, trigKit.extra_Trigger_شاخص(شاخص));
        ++شاخص;
    }
    أرجع map;
}

//============================================================================
// Top level entry-point to scheduling

فراغ زمانبندی(AstNetlist* netlistp) {
    ثابت خودکار جمع_کن_اندازه_Stat = [](ثابت string& name, ثابت LogicByScope& lbs) {
        uint64_t اندازه = 0;
        lbs.foreachLogic([&](Ast_گره* nodep) { اندازه += nodep->گره_شمار(); });
        V3Stats::جمع_کن_Stat("Scheduling, " + name, اندازه);
    };

    // Step 2: Prepare external domains for timing and virtual interfaces
    // Create extra triggers for virtual interfaces
    ثابت خودکار& virtIfaceTriggers = makeVirtIfaceTriggers(netlistp);
    // Resolve what covergroup reference formal arguments are bound to, which is visible here
    // but not from V3Order, where the reads through them must be modeled
    ثابت CovergroupRefBindings& cgRefBindings = makeCovergroupRefBindings(netlistp);
    // Prepare timing-related logic and external domains
    TimingKit timingKit = prepareTiming(netlistp);

    // Step 3: Gather and classify all logic in the design
    LogicClasses logicClasses = gatherLogicClasses(netlistp);

    إذا (v3Global.opt.stats()) {
        V3Stats::statsStage("sched-gather");
        جمع_کن_اندازه_Stat("size of class: static", logicClasses.m_static);
        جمع_کن_اندازه_Stat("size of class: initial", logicClasses.m_initial);
        جمع_کن_اندازه_Stat("size of class: final", logicClasses.m_final);
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
    logicClasses.m_hybrid = break_چرخهها(netlistp, logicClasses.m_comb);
    إذا (v3Global.opt.stats()) {
        جمع_کن_اندازه_Stat("size of class: clocked", logicClasses.m_clocked);
        جمع_کن_اندازه_Stat("size of class: combinational", logicClasses.m_comb);
        جمع_کن_اندازه_Stat("size of class: hybrid", logicClasses.m_hybrid);
        V3Stats::statsStage("sched-break-cycles");
    }

    // We pass around a single SenExprBuilder instance, as we only need one set of 'prev' variables
    // for edge/change detection in sensitivity expressions, which this keeps track of.
    Ast_بالاترین_Scope* ثابت بالاترین_Scopep = netlistp->بالاترین_Scopep();
    AstScope* ثابت scopeTopp = بالاترین_Scopep->scopep();
    Sen_عبارت_Builder sen_عبارت_Builder{scopeTopp};

    // Step 6: Create 'settle' region that restores the combinational invariant
    createSettle(netlistp, staticp, sen_عبارت_Builder, logicClasses, cgRefBindings);
    إذا (v3Global.opt.stats()) V3Stats::statsStage("sched-settle");

    // Step 7: Partition the clocked and combinational (including hybrid) logic into pre/act/nba.
    // All clocks (signals referenced in an AstSenTree) generated via a blocking assignment
    // (including combinationally generated signals) are computed within the act region.
    LogicRegions logicRegions
        = افراز(logicClasses.m_clocked, logicClasses.m_comb, logicClasses.m_hybrid);
    logicRegions.m_obs = logicClasses.m_observed;
    logicRegions.m_react = logicClasses.m_reactive;
    إذا (v3Global.opt.stats()) {
        جمع_کن_اندازه_Stat("size of region: Active Pre", logicRegions.m_pre);
        جمع_کن_اندازه_Stat("size of region: Active", logicRegions.m_act);
        جمع_کن_اندازه_Stat("size of region: NBA", logicRegions.m_nba);
        جمع_کن_اندازه_Stat("size of region: Observed", logicRegions.m_obs);
        جمع_کن_اندازه_Stat("size of region: Reactive", logicRegions.m_react);
        V3Stats::statsStage("sched-partition");
    }

    // Step 8: Replicate combinational logic
    LogicReplicas logicReplicas = replicateLogic(logicRegions);
    إذا (v3Global.opt.stats()) {
        جمع_کن_اندازه_Stat("size of replicated logic: Input", logicReplicas.m_ico);
        جمع_کن_اندازه_Stat("size of replicated logic: Active", logicReplicas.m_act);
        جمع_کن_اندازه_Stat("size of replicated logic: NBA", logicReplicas.m_nba);
        جمع_کن_اندازه_Stat("size of replicated logic: Observed", logicReplicas.m_obs);
        جمع_کن_اندازه_Stat("size of replicated logic: Reactive", logicReplicas.m_react);
        V3Stats::statsStage("sched-replicate");
    }

    // Step 9: Create the input combinational logic
    createIcoRegion(netlistp, staticp, sen_عبارت_Builder, logicReplicas.m_ico, virtIfaceTriggers,
                    cgRefBindings);
    إذا (v3Global.opt.stats()) V3Stats::statsStage("sched-create-ico");

    // Step 10: Create the triggers
    AstVarScope* ثابت dpiExportTriggerVscp = netlistp->dpiExportTriggerp();
    netlistp->dpiExportTriggerp(اشاره_تهی);  // Finished with this here

    // We may have an extra trigger for variable updated in DPI exports
    TriggerKit::ExtraTriggers extraTriggers;
    ثابت uint32_t dpi_Export_Trigger_شاخص = dpiExportTriggerVscp
                                               ? extraTriggers.allocate("DPI export trigger")
                                               : std::numeric_limits<uint32_t>::max();
    ثابت uint32_t اولی_Vif_Trigger_شاخص = extraTriggers.اندازه();
    لكل (ثابت خودکار& مدخل : virtIfaceTriggers.m_triggers) {
        extraTriggers.allocate("virtual interface member: " + مدخل.m_ifacep->name() + "."
                               + مدخل.m_memberp->name());
    }

    ثابت خودکار& preTreeps = get_Sen_Trees_استفاده_شده_By({&logicRegions.m_pre});
    ثابت خودکار& senTreeps = get_Sen_Trees_استفاده_شده_By({&logicRegions.m_act,  //
                                               &logicRegions.m_nba,  //
                                               &logicRegions.m_obs,  //
                                               &logicRegions.m_react,  //
                                               &timingKit.m_lbs});
    ثابت TriggerKit trigKit
        = TriggerKit::create(netlistp, staticp, sen_عبارت_Builder, preTreeps, senTreeps, "act",
                             extraTriggers, خطأ, v3Global.استفاده_میکند_Timing());

    // Add post updates from the timing kit
    إذا (timingKit.m_postUpdates) trigKit.compBasep()->جمع_کن_Stmtsp(timingKit.m_postUpdates);

    إذا (dpiExportTriggerVscp) {
        trigKit.جمع_کن_Extra_Trigger_Assignment(dpiExportTriggerVscp, dpi_Export_Trigger_شاخص);
    }
    جمع_کن_Virt_Iface_Trigger_Assignments(netlistp, staticp, virtIfaceTriggers, اولی_Vif_Trigger_شاخص,
                                   trigKit);
    إذا (v3Global.opt.stats()) V3Stats::statsStage("sched-create-triggers");

    // Note: Experiments so far show that running the Act (or Ico) regions on
    // multiple threads is always a net loss, so only use multi-threading for
    // NBA for now. This can be revised if evidence is available that it would
    // be beneficial

    // Step 11: Create the 'act' region evaluation function

    // Add the trigger indices of the input logic
    جمع_کن_Trigger_Indices(logicRegions.m_pre, trigKit, درست);
    جمع_کن_Trigger_Indices(logicRegions.m_act, trigKit, خطأ);
    جمع_کن_Trigger_Indices(logicReplicas.m_act, trigKit, خطأ);
    جمع_کن_Trigger_Indices(timingKit.m_lbs, trigKit, خطأ);

    // External triggers of variables. Trigger indices are the same in all regions, so share.
    ثابت std::map<ثابت AstVarScope*, std::وکتور<uint32_t>> timingTriggers
        = timingKit.externalTriggerIndices(trigKit);
    ثابت خودکار& vif_Vscp_مقصد_Trig = virtIfaceTriggers.make_Vscp_مقصد_Trig_Map(trigKit, اولی_Vif_Trigger_شاخص);
    ثابت V3_ترتیب::ExternalTriggersProvider externalTriggers
        = [&](ثابت AstVarScope* vscp, std::مجموعه<uint32_t>& triggers) {
              خودکار it = timingTriggers.پیدا_کن(vscp);
              إذا (it != timingTriggers.انتها())
                  triggers.درج(it->دومی.begin(), it->دومی.انتها());
              إذا (vscp->varp()->isWrittenByDpi()) {
                  triggers.درج(trigKit.extra_Trigger_شاخص(dpi_Export_Trigger_شاخص));
              }
              إذا (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                  جمع_کن_Triggered_Iface(vscp, vif_Vscp_مقصد_Trig, virtIfaceTriggers, triggers);
              }
          };

    AstCFunc* ثابت actFuncp
        = V3_ترتیب::ترتیب({&logicRegions.m_pre, &logicRegions.m_act, &logicReplicas.m_act}, trigKit,
                         trigKit.vscp(), cgRefBindings, "act", خطأ, خطأ, externalTriggers);
    util::تقسیم_Check(actFuncp);
    إذا (v3Global.opt.stats()) V3Stats::statsStage("sched-create-act");

    ثابت EvalKit actKit{trigKit.vscp(), actFuncp};

    // Orders a region's logic and creates the region eval function
    ثابت خودکار ترتیب = [&](ثابت std::string& name,
                           ثابت std::وکتور<V3Sched::LogicByScope*>& logic) -> EvalKit {
        UINFO(2, "Scheduling " << name << " #logic = " << logic.اندازه());
        AstVarScope* ثابت trigVscp = trigKit.newTrigVec(name);
        // Add the trigger indices of the input logic
        لكل (LogicByScope* lbs : logic) جمع_کن_Trigger_Indices(*lbs, trigKit, خطأ);

        AstCFunc* ثابت funcp
            = V3_ترتیب::ترتیب(logic, trigKit, trigVscp, cgRefBindings, name,
                             name == "nba" && v3Global.opt.mtasks(), خطأ, externalTriggers);

        أرجع {trigVscp, funcp};
    };

    // Step 12: Create the 'nba' region evaluation function
    ثابت EvalKit nbaKit = ترتیب("nba", {&logicRegions.m_nba, &logicReplicas.m_nba});
    util::تقسیم_Check(nbaKit.m_funcp);
    إذا (v3Global.opt.stats()) V3Stats::statsStage("sched-create-nba");

    // Orders a region's logic and creates the region eval function (only if there is any logic in
    // the region)
    ثابت خودکار ترتیب_If_Non_خالی
        = [&](ثابت std::string& name, ثابت std::وکتور<LogicByScope*>& logic) -> EvalKit {
        إذا (logic[0]->خالی())
            أرجع {};  // if region is empty, replica is supposed to be empty as well
        ثابت خودکار& kit = ترتیب(name, logic);
        إذا (v3Global.opt.stats()) V3Stats::statsStage("sched-create-" + name);
        أرجع kit;
    };

    // Step 13: Create the 'obs' region evaluation function
    ثابت EvalKit obsKit = ترتیب_If_Non_خالی("obs", {&logicRegions.m_obs, &logicReplicas.m_obs});

    // Step 14: Create the 'react' region evaluation function
    ثابت EvalKit reactKit
        = ترتیب_If_Non_خالی("react", {&logicRegions.m_react, &logicReplicas.m_react});

    // Step 15: Create the 'postponed' region evaluation function
    createPostponed(netlistp, logicClasses);

    // Step 16: Populate the eval entry point function of each region of a time step
    createEval(netlistp, trigKit, actKit, nbaKit, obsKit, reactKit, timingKit);

    // Step 17: Add neccessary evaluation before awaits
    إذا (AstCCall* ثابت readyp = timingKit.createReady(trigKit.vscp())) {
        staticp->جمع_کن_Stmtsp(readyp->makeStmt());
        beforeTrigVisitor(netlistp, sen_عبارت_Builder, trigKit);
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
            trigAccDTypep->راست() == 0, trigAccp,
            "Expected that trigger vector and accumulator start elements enumeration from 0");
        UASSERT_OBJ(trigAccDTypep->چپ() >= 0, trigAccp,
                    "Expected that trigger vector and accumulator has no negative indexes");
        File_سطر* ثابت flp = trigAccp->fileline();
        AstVarScope* ثابت vscp = netlistp->بالاترین_Scopep()->create_موقت("__Vi", 32);
        AstLoop* ثابت loopp = جديد AstLoop{flp};
        loopp->جمع_کن_Stmtsp(
            جديد Ast_تخصیص_بده{flp,
                          جديد AstArraySel{flp, جديد AstVarRef{flp, trigAccp, VAccess::WRITE},
                                          جديد AstVarRef{flp, vscp, VAccess::READ}},
                          جديد AstArraySel{flp, جديد AstVarRef{flp, actKit.m_vscp, VAccess::READ},
                                          جديد AstVarRef{flp, vscp, VAccess::READ}}});
        loopp->جمع_کن_Stmtsp(util::incrementVar(vscp));
        loopp->جمع_کن_Stmtsp(جديد AstLoopTest{
            flp, loopp,
            جديد AstLte{flp, جديد AstVarRef{flp, vscp, VAccess::READ},
                       جديد AstConst{flp, AstConst::Widthed_مقدار{}, 32,
                                    static_cast<uint32_t>(trigAccDTypep->چپ())}}});
        staticp->جمع_کن_Stmtsp(loopp);
    }

    // Step 18: Clean up
    netlistp->clear_Stl_اولی_Iterationp();

    إذا (v3Global.opt.stats()) {
        // A sample() call resolved to the union over its covergroup's constructions reads more
        // than it can, which orders it against more logic than necessary
        V3Stats::جمع_کن_Stat("Scheduling, covergroup ref sample calls, per instance",
                         cgRefBindings.رقم_Exact_Calls());
        V3Stats::جمع_کن_Stat("Scheduling, covergroup ref sample calls, per type",
                         cgRefBindings.رقم_Union_Calls());
    }

    // Haven't split static initializer yet
    util::تقسیم_Check(staticp);

    // Record eval stats
    netlistp->جمع_کن_Eval_Stats("sched");

    // Dump
    V3Global::dump_Check_Global_درخت("sched", 0, dump_درخت_Either_Level() >= 3);
}

}  // namespace V3Sched
