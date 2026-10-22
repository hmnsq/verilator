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
// Functions defined in this file are used by V3Sched.cpp to properly integrate
// static scheduling with timing features. They create external domains for
// variables, remap them to trigger vectors, and create timing resume/ready
// calls for the global eval loop. There is also a function that transforms
// forks into emittable constructs.
//
// See the internals documentation docs/internals.rst for more details.
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3AstUserAllocator.h"
#ಅನ್ತರ್ಭಾವ "V3ClassGraph.h"
#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"
#ಅನ್ತರ್ಭಾವ "V3Sched.h"

#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

ನಾಮಾಕಾಶ V3Sched {

//============================================================================
// Trigger indices of the external domains of each variable

std::map<ಸ್ಥಿರಾಂಕ AstVarScope*, std::ಸದಿಶ<uint32_t>>
TimingKit::externalTriggerIndices(ಸ್ಥಿರಾಂಕ TriggerKit& trigKit) ಸ್ಥಿರಾಂಕ {
    std::map<ಸ್ಥಿರಾಂಕ AstVarScope*, std::ಸದಿಶ<uint32_t>> ಫಲಿತಾಂಶ;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& vscpDomains : m_externalDomains) {
        std::ಸದಿಶ<uint32_t>& indices = ಫಲಿತಾಂಶ[vscpDomains.ಮೊದಲ];
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ domainp : vscpDomains.ಎರಡನೇ) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_ಅಂಶ)) {
                indices.ತಳ್ಳು_ಹಿಂಭಾಗ(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    ಆದರೆ (!m_resumeFuncp) {
        ಆದರೆ (m_lbs.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        // Create global resume function
        AstNetlist* ಸ್ಥಿರಾಂಕ netlistp = v3Global.rootp();
        AstScope* ಸ್ಥಿರಾಂಕ scopeTopp = netlistp->ತುದಿ_Scopep()->scopep();
        m_resumeFuncp = ಹೊಸ AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(ನಿಜ);
        m_resumeFuncp->isLoose(ನಿಜ);
        m_resumeFuncp->isConst(ಸುಳ್ಳು);
        m_resumeFuncp->decl_ಖಾಸಗಿ(ನಿಜ);
        scopeTopp->ಕೂಡಿಸು_Blocksp(m_resumeFuncp);

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : m_lbs) {
            Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ನಮೂದು.m_activep;
            activep->foreach([ಇದು](AstCMethodHard* ಸ್ಥಿರಾಂಕ exprp) {
                ಆದರೆ (exprp->ವಿಧಾನ() != VCMethod::SCHED_RESUME) ಹಿಂತಿರುಗಿ;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = exprp->fromp();
                ಆದರೆ (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    ಹಿಂತಿರುಗಿ;
                }
                AstCMethodHard* ಸ್ಥಿರಾಂಕ move_ಗಮ್ಯ_Resumep = ಹೊಸ AstCMethodHard{
                    fromp->fileline(), fromp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು),
                    VCMethod::SCHED_MOVE_ಗಮ್ಯ_RESUME_ಸರತಿ,
                    exprp->pinsp() ? exprp->pinsp()->clone_ವೃಕ್ಷ(ನಿಜ) : ಶೂನ್ಯಸೂಚಕ};
                move_ಗಮ್ಯ_Resumep->dtype_ಗಣ_Void();
                m_resumeFuncp->ಕೂಡಿಸು_Stmtsp(move_ಗಮ್ಯ_Resumep->makeStmt());
            });
        }

        // Put all the timing actives in the resume function
        AstIf* dlyShedIfp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : m_lbs) {
            Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ನಮೂದು.m_activep;
            // Resume time delays last. For no particular reason other than
            // that's what we used to do prior to proper #0 support.
            AstVarRef* ಸ್ಥಿರಾಂಕ schedrefp = VN_AS(
                VN_AS(VN_AS(activep->stmtsp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)->exprp(), CMethodHard)->fromp(), VarRef);

            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ actionp = activep->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            ಆದರೆ (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = trigVscp->fileline();
                ಸ್ಥಿರಾಂಕ std::ಗಣ<uint32_t>& triggers = ನಮೂದು.m_triggers;
                AstTriggered* ಸ್ಥಿರಾಂಕ condp
                    = ಹೊಸ AstTriggered{flp, trigVscp, {triggers.begin(), triggers.ಕೊನೆ()}};
                dlyShedIfp = ಹೊಸ AstIf{flp, condp};
                dlyShedIfp->ಕೂಡಿಸು_Thensp(actionp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_resumeFuncp->ಕೂಡಿಸು_Stmtsp(actionp);
            }
        }
        ಆದರೆ (dlyShedIfp) m_resumeFuncp->ಕೂಡಿಸು_Stmtsp(dlyShedIfp);

        // These are now spent, oispose of now empty AstActive instances
        m_lbs.deleteActives();
    }
    AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
    callp->dtype_ಗಣ_Void();
    ಹಿಂತಿರುಗಿ callp;
}

AstVarScope* TimingKit::getDelayScheduler(AstNetlist* ಸ್ಥಿರಾಂಕ /*netlistp*/) {
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : m_lbs) {
        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ ap = ನಮೂದು.m_activep;
        // TODO: this triple VN_AS expression is ridiculous
        AstVarRef* ಸ್ಥಿರಾಂಕ schedrefp
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = schedrefp->varScopep();
        ಆದರೆ (vscp->dtypep()->basicp()->isDelayScheduler()) ಹಿಂತಿರುಗಿ vscp;
    }
    // None found. Design doesn't use any time delays
    ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    ಆದರೆ (!m_readyFuncp) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : m_lbs) {
            Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ನಮೂದು.m_activep;
            ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ resumep = VN_AS(VN_AS(activep->stmtsp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)->exprp(), CMethodHard);
            UASSERT_OBJ(!resumep->nextp(), resumep, "Should be the only statement here");
            AstVarScope* ಸ್ಥಿರಾಂಕ schedulerp = VN_AS(resumep->fromp(), VarRef)->varScopep();
            UASSERT_OBJ(schedulerp->dtypep()->basicp()->isDelayScheduler()
                            || schedulerp->dtypep()->basicp()->isTriggerScheduler()
                            || schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler(),
                        schedulerp, "Unexpected type");
            ಆದರೆ (!schedulerp->dtypep()->basicp()->isTriggerScheduler()) ಮುಂದುವರಿ;
            // Create the global ready function only if we have trigger schedulers
            ಆದರೆ (!m_readyFuncp) {
                AstNetlist* ಸ್ಥಿರಾಂಕ netlistp = v3Global.rootp();
                AstScope* ಸ್ಥಿರಾಂಕ scopeTopp = netlistp->ತುದಿ_Scopep()->scopep();
                m_readyFuncp = ಹೊಸ AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(ನಿಜ);
                m_readyFuncp->isLoose(ನಿಜ);
                m_readyFuncp->isConst(ಸುಳ್ಳು);
                m_readyFuncp->decl_ಖಾಸಗಿ(ನಿಜ);
                scopeTopp->ಕೂಡಿಸು_Blocksp(m_readyFuncp);
            }

            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = activep->sentreep()->fileline();

            // Create an 'AstIf' sensitive to the suspending triggers
            ಸ್ಥಿರಾಂಕ std::ಗಣ<uint32_t>& triggers = ನಮೂದು.m_triggers;
            AstTriggered* ಸ್ಥಿರಾಂಕ condp
                = ಹೊಸ AstTriggered{flp, trigVscp, {triggers.begin(), triggers.ಕೊನೆ()}};
            AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, condp};
            m_readyFuncp->ಕೂಡಿಸು_Stmtsp(ifp);

            // Mark as ready the processes resumed on this sensitivity expression
            AstVarRef* ಸ್ಥಿರಾಂಕ refp = ಹೊಸ AstVarRef{flp, schedulerp, VAccess::READWRITE};
            AstCMethodHard* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCMethodHard{flp, refp, VCMethod::SCHED_READY};
            callp->dtype_ಗಣ_Void();
            ಆದರೆ (resumep->pinsp()) callp->ಕೂಡಿಸು_Pinsp(resumep->pinsp()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
            ifp->ಕೂಡಿಸು_Thensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        ಆದರೆ (!m_readyFuncp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }
    AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{m_readyFuncp->fileline(), m_readyFuncp};
    callp->dtype_ಗಣ_Void();
    ಹಿಂತಿರುಗಿ callp;
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

ವರ್ಗ AwaitVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  AstSenTree::user1()  -> bool.               Set true if the sentree has been visited.
    //  AstCFunc::user1()    -> AstUser1Allocator.  See alocator below
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;

    // STATE
    ತಾರ್ಕಿಕ m_ಒಳ_Process = ಸುಳ್ಳು;  // Are we in a process?
    ತಾರ್ಕಿಕ m_gatherVars = ಸುಳ್ಳು;  // Should we gather vars in m_writtenBySuspendable?
    AstScope* ಸ್ಥಿರಾಂಕ m_scopeTopp;  // Scope at the top
    LogicByScope& m_lbs;  // Timing resume actives
    Ast_ಗಂಟು_Stmt*& m_postUpdatesr;  // Post updates for the trigger eval function
    // Additional var sensitivities
    std::map<ಸ್ಥಿರಾಂಕ AstVarScope*, std::ಗಣ<Ast_Sen_ವೃಕ್ಷ*>>& m_externalDomains;
    std::unique_ptr<V3_Class_ನಕ್ಷೆ>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::ಗಣ<Ast_Sen_ವೃಕ್ಷ*> m_processDomains;  // Sentrees from the current process
    // Variables written by suspendable processes
    std::ಗಣ<AstVarScope*> m_writtenBySuspendable;
    ರಚನೆ CFuncCache final {
        std::ಗಣ<Ast_Sen_ವೃಕ್ಷ*> m_processDomains;  // What shall be added to m_processDomains
        std::ಗಣ<AstVarScope*>
            m_writtenBySuspendable;  // What shall be added to m_writtenBySuspendable
        std::ಗಣ<AstCFunc*> m_includes;  // CFuncs whose CFuncCache shall be included into this
        // m_includes does not need to keep bool with m_gatherVars to know which cache shall be
        // used since:
        //  Only possible transition in a graph is from `!m_gatherVars` to `m_gatherVars`
        //  (in visit(AstFork*) there is a only possible transition, visit(AstNodeProcedure*) is
        //  not considered since it does not occur during a graph traversal)
        //  therefore, if change of state occurs it is certain that the first node after transition
        //  won't be in visiting state
        //  therefore, every node in a m_includes shall take value of m_gatherVars under which
        //  current CFuncCache is
        ಎಣಿಕೆ State : uint8_t {
            UNINITIALIZED = 0,  // Not initialized members are empty
            VISITING,  // Visiting - needed for breaking recursion
            INITIALIZED,  // Members contains correct values
        } m_state  // Current state of Cache
            = UNINITIALIZED;
    };
    std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ AstCFunc*> m_call_ರಾಶಿ;  // Current callstack of AstCFuncs
    // Caches how visiting the function with given value of m_gatherVars changes
    // m_processDomains and m_writtenBySuspendable - only accessed from visit(AstCFunc* nodep)
    AstUser1Allocator<AstCFunc, std::array<CFuncCache, 2>> m_cfuncsCache;
    // Count uses of not inlined writes to signals in suspendables
    VDouble0 m_not_Inlined_Writes_ಒಳ_Suspendable_Usage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    ಶೂನ್ಯ ಕೂಡಿಸು_Resume_Pins(AstCMethodHard* ಸ್ಥಿರಾಂಕ resumep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pinsp) {
        AstCExpr* ಸ್ಥಿರಾಂಕ exprp = VN_CAST(pinsp, CExpr);
        Ast_ಪಠ್ಯ* ಸ್ಥಿರಾಂಕ textp = VN_CAST(exprp->nodesp(), ಪಠ್ಯ_2);
        ಆದರೆ (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            ಆದರೆ ((pinsp = VN_CAST(pinsp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ))) {
                resumep->ಕೂಡಿಸು_Pinsp(pinsp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            resumep->ಕೂಡಿಸು_Pinsp(pinsp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
        }
    }
    ಶೂನ್ಯ ಕೂಡಿಸು_CFunc_Cached_ಮೌಲ್ಯಗಳು(ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ cfuncp,
                              std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ AstCFunc*>& ಭೇಟಿಯಾದ) {
        ಆದರೆ (!ಭೇಟಿಯಾದ.ಸೇರಿಸು(cfuncp).ಎರಡನೇ) ಹಿಂತಿರುಗಿ;
        CFuncCache& ಮೌಲ್ಯ = m_cfuncsCache(cfuncp)[static_cast<ಗಾತ್ರ_t>(m_gatherVars)];
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t initial_ಗಾತ್ರ = m_writtenBySuspendable.ಗಾತ್ರ_2();
        m_writtenBySuspendable.ಸೇರಿಸು(ಮೌಲ್ಯ.m_writtenBySuspendable.begin(),
                                      ಮೌಲ್ಯ.m_writtenBySuspendable.ಕೊನೆ());
        m_not_Inlined_Writes_ಒಳ_Suspendable_Usage += m_writtenBySuspendable.ಗಾತ್ರ_2() - initial_ಗಾತ್ರ;
        m_processDomains.ಸೇರಿಸು(ಮೌಲ್ಯ.m_processDomains.begin(), ಮೌಲ್ಯ.m_processDomains.ಕೊನೆ());
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ includedp : ಮೌಲ್ಯ.m_includes) {
            ಕೂಡಿಸು_CFunc_Cached_ಮೌಲ್ಯಗಳು(includedp, ಭೇಟಿಯಾದ);
        }
    }
    // Create an active with a timing scheduler resume() call
    ಶೂನ್ಯ create_Resume_ಸಕ್ರಿಯ(AstCAwait* ಸ್ಥಿರಾಂಕ awaitp) {
        ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* ಸ್ಥಿರಾಂಕ schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ sentreep = awaitp->sentreep();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ resumep = ಹೊಸ AstCMethodHard{
            flp, ಹೊಸ AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtype_ಗಣ_Void();
        ಆದರೆ (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ dbginfop = methodp->pinsp()->nextp()) {
                ಆದರೆ (methodp->pinsp()) ಕೂಡಿಸು_Resume_Pins(resumep, static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(dbginfop));
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ postp = resumep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            postp->ವಿಧಾನ(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ಹೊಸ Ast_ಸಕ್ರಿಯ{flp, "_timing", sentreep};
        activep->ಕೂಡಿಸು_Stmtsp(resumep->makeStmt());
        m_lbs.ಕೂಡಿಸು(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    ಶೂನ್ಯ ಭೇಟಿ_Called_CFunc(AstCFunc* ಸ್ಥಿರಾಂಕ nodep) {
        ಆದರೆ (!m_ಒಳ_Process) ಹಿಂತಿರುಗಿ;  // Skip if not in process - nothing would be added to any set
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t cache_ಕೀಲಿ = static_cast<ಗಾತ್ರ_t>(m_gatherVars);
        CFuncCache& ಮೌಲ್ಯ = m_cfuncsCache(nodep)[cache_ಕೀಲಿ];
        ಆಯ್ಕೆ (ಮೌಲ್ಯ.m_state) {
        ಸ್ಥಿತಿ CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            ಮೌಲ್ಯ.m_state = CFuncCache::VISITING;
            m_call_ರಾಶಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
            iterateChildren(nodep);
            m_call_ರಾಶಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
            ಮೌಲ್ಯ.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::ಅದಲುಬದಲು(m_processDomains, ಮೌಲ್ಯ.m_processDomains);
            std::ಅದಲುಬದಲು(m_writtenBySuspendable, ಮೌಲ್ಯ.m_writtenBySuspendable);
        } ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ CFuncCache::VISITING: {
            ಗಾಗಿ (ಗಾತ್ರ_t i = m_call_ರಾಶಿ.ಗಾತ್ರ_2() - 1; m_call_ರಾಶಿ.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_ರಾಶಿ[i])[cache_ಕೀಲಿ].m_includes.ಸೇರಿಸು(nodep);
            }
            ಹಿಂತಿರುಗಿ;  // Break recursion
        }
        ಸ್ಥಿತಿ CFuncCache::INITIALIZED: ನಿಲ್ಲಿಸು;
        }
        std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ AstCFunc*> ಭೇಟಿಯಾದ;
        ಕೂಡಿಸು_CFunc_Cached_ಮೌಲ್ಯಗಳು(nodep, ಭೇಟಿಯಾದ);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Procedure* ಸ್ಥಿರಾಂಕ nodep) override {
        UASSERT_OBJ(!m_ಒಳ_Process && !m_gatherVars && m_processDomains.ಖಾಲಿಗಳು()
                        && m_writtenBySuspendable.ಖಾಲಿಗಳು(),
                    nodep, "Process in process?");
        m_ಒಳ_Process = ನಿಜ;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        ಗಾಗಿ (AstVarScope* ಸ್ಥಿರಾಂಕ vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].ಸೇರಿಸು(m_processDomains.begin(), m_processDomains.ಕೊನೆ());
            vscp->varp()->ಗಣ_Written_By_Suspendable();
        }
        m_processDomains.clear();
        m_writtenBySuspendable.clear();
        m_ಒಳ_Process = ಸುಳ್ಳು;
        m_gatherVars = ಸುಳ್ಳು;
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        ಆದರೆ (m_ಒಳ_Process) m_gatherVars = ನಿಜ;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCAwait* nodep) override {
        ಆದರೆ (Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ sentreep = nodep->sentreep()) {
            ಆದರೆ (!sentreep->user1_ಗಣ_Once()) create_Resume_ಸಕ್ರಿಯ(nodep);
            ಆದರೆ (m_ಒಳ_Process) m_processDomains.ಸೇರಿಸು(sentreep);
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಆದರೆ (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.ಸೇರಿಸು(nodep->varScopep());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_CCall* ಸ್ಥಿರಾಂಕ nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        ಆದರೆ (cfuncps.ಖಾಲಿಗಳು()) {
            ಭೇಟಿ_Called_CFunc(nodep->funcp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಗಾಗಿ (AstCFunc* ಸ್ಥಿರಾಂಕ cfuncp : cfuncps) ಭೇಟಿ_Called_CFunc(cfuncp);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* ಸ್ಥಿರಾಂಕ nodep) override {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಮೌಲ್ಯ = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        ಆದರೆ (ಮೌಲ್ಯ[0].m_state != CFuncCache::UNINITIALIZED
            || ಮೌಲ್ಯ[1].m_state != CFuncCache::UNINITIALIZED) {
            ಹಿಂತಿರುಗಿ;
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, Ast_ಗಂಟು_Stmt*& postUpdatesr,
                          std::map<ಸ್ಥಿರಾಂಕ AstVarScope*, std::ಗಣ<Ast_Sen_ವೃಕ್ಷ*>>& externalDomains)
        : m_scopeTopp{nodep->ತುದಿ_Scopep()->scopep()}
        , m_lbs{lbs}
        , m_postUpdatesr{postUpdatesr}
        , m_externalDomains{externalDomains}
        , m_classGraphp{V3_Class_ನಕ್ಷೆ::build(nodep)} {
        iterate(nodep);
    }
    ~AwaitVisitor() override {
        V3Stats::ಕೂಡಿಸು_Stat("Scheduling, count of non-inlined signal writes in suspendables",
                         m_not_Inlined_Writes_ಒಳ_Suspendable_Usage);
    }
};

TimingKit prepareTiming(AstNetlist* ಸ್ಥಿರಾಂಕ netlistp) {
    ಆದರೆ (!v3Global.ಬಳಸುತ್ತದೆ_Timing()) ಹಿಂತಿರುಗಿ {};
    LogicByScope lbs;
    Ast_ಗಂಟು_Stmt* postUpdates = ಶೂನ್ಯಸೂಚಕ;
    std::map<ಸ್ಥಿರಾಂಕ AstVarScope*, std::ಗಣ<Ast_Sen_ವೃಕ್ಷ*>> externalDomains;
    { AwaitVisitor{netlistp, lbs, postUpdates, externalDomains}; }
    ಹಿಂತಿರುಗಿ {std::move(lbs), postUpdates, std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

// Transform all forked processes into functions
ವರ್ಗ TransformForksVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current fork.
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;

    // STATE
    ತಾರ್ಕಿಕ m_ಒಳ_Class = ಸುಳ್ಳು;  // Are we in a class?
    ತಾರ್ಕಿಕ m_beginHasAwaits = ಸುಳ್ಳು;  // Does the current begin have awaits?
    AstFork* m_forkp = ಶೂನ್ಯಸೂಚಕ;  // Current fork
    AstCFunc* m_funcp = ಶೂನ್ಯಸೂಚಕ;  // Current function

    // METHODS
    // Remap local vars referenced by the given fork function
    // TODO: We should only pass variables to the fork that are
    // live in the fork body, but for that we need a proper data
    // flow analysis framework which we don't have at the moment
    ಶೂನ್ಯ remapLocals(AstCFunc* ಸ್ಥಿರಾಂಕ funcp, AstCCall* ಸ್ಥಿರಾಂಕ callp) {
        ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use user2_ಒಳ_Use;  // AstVarScope -> AstVarScope: var to remap to
        funcp->foreach([&](Ast_ಗಂಟು_Var_Ref* refp) {
            AstVar* ಸ್ಥಿರಾಂಕ varp = refp->varp();
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = refp->varScopep();
            AstBasicDType* ಸ್ಥಿರಾಂಕ dtypep = varp->dtypep()->basicp();
            // If not a fork..join, copy. All write refs should've been handled by V3Fork
            ತಾರ್ಕಿಕ pass_By_ಮೌಲ್ಯ = !m_forkp->joinType().join();
            ಆದರೆ (vscp->fork_By_ಮೌಲ್ಯ()) {
                // Set just before the fork, for its processes to get by value
                pass_By_ಮೌಲ್ಯ = ನಿಜ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!varp->isFuncLocal()) {
                // Not func local. Its lifetime is longer than the forked process. Skip
                ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!varp->user1()) {
                // Not declared before the fork. It cannot outlive the forked process
                ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (dtypep && dtypep->isForkSync()) {
                // We can just pass it by value to the new function
                pass_By_ಮೌಲ್ಯ = ನಿಜ;
            }
            // Remap the reference
            ಆದರೆ (!vscp->user2p()) {
                // Clone the var to the new function
                AstVar* ಸ್ಥಿರಾಂಕ newvarp
                    = ಹೊಸ AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                newvarp->funcLocal(ನಿಜ);
                newvarp->direction(pass_By_ಮೌಲ್ಯ ? VDirection::ಒಳಹರಿವು : VDirection::REF);
                funcp->ಕೂಡಿಸು_Argsp(newvarp);
                AstVarScope* ಸ್ಥಿರಾಂಕ newvscp
                    = ಹೊಸ AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->ಕೂಡಿಸು_Varsp(newvscp);
                vscp->user2p(newvscp);
                callp->ಕೂಡಿಸು_Argsp(ಹೊಸ AstVarRef{refp->fileline(), vscp,
                                              pass_By_ಮೌಲ್ಯ ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* ಸ್ಥಿರಾಂಕ newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_ಒಳ_Class);
        m_ಒಳ_Class = VN_IS(nodep, Class);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        m_funcp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        ಆದರೆ (!m_forkp) nodep->user1(ನಿಜ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFork* nodep) override {
        ಆದರೆ (m_forkp) ಹಿಂತಿರುಗಿ;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        Ast_ಗಂಟು* resp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ declsp = nodep->declsp()) {
            resp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(resp, declsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
        }
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = nodep->stmtsp()) {
            resp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(resp, stmtsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
        }
        ಆಗ (AstBegin* ಸ್ಥಿರಾಂಕ beginp = nodep->forksp()) {
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ declsp = beginp->declsp()) {
                resp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(resp, declsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
            }
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = beginp->stmtsp()) {
                resp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(resp, stmtsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(beginp->unlink_Fr_ಹಿಂಭಾಗ()), beginp);
        }
        ಆದರೆ (resp) {
            nodep->replaceWith(resp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstBegin* nodep) override {
        UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
        // Start with children, so later we only find awaits that are actually in this begin
        m_beginHasAwaits = ಸುಳ್ಳು;
        iterateChildrenConst(nodep);
        ಆದರೆ (!nodep->stmtsp()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!m_beginHasAwaits && !nodep->needProcess()) ಹಿಂತಿರುಗಿ;

        UASSERT_OBJ(!nodep->name().ಖಾಲಿಗಳು(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        AstCFunc* ಸ್ಥಿರಾಂಕ newfuncp = ಹೊಸ AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->decl_ಖಾಸಗಿ(ನಿಜ);
        // Create the call to the function
        AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{flp, newfuncp};
        callp->dtype_ಗಣ_Void();
        // If we're in a class, add a vlSymsp arg
        ಆದರೆ (m_ಒಳ_Class) {
            newfuncp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ declsp = nodep->declsp()) {
            newfuncp->ಕೂಡಿಸು_Stmtsp(declsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
        }
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = nodep->stmtsp()) {
            newfuncp->ಕೂಡಿಸು_Stmtsp(stmtsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->ಕೂಡಿಸು_Stmtsp(callp->makeStmt());
        // Propagate if needs process
        ಆದರೆ (nodep->needProcess()) newfuncp->ಗಣ_Need_Process();
        remapLocals(newfuncp, callp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCAwait* nodep) override {
        m_beginHasAwaits = ನಿಜ;
        iterateChildrenConst(nodep);
    }

    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

ಶೂನ್ಯ transformForks(AstNetlist* ಸ್ಥಿರಾಂಕ netlistp) {
    ಆದರೆ (!v3Global.ಬಳಸುತ್ತದೆ_Timing()) ಹಿಂತಿರುಗಿ;
    { TransformForksVisitor{netlistp}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("transform_forks", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

}  // namespace V3Sched
