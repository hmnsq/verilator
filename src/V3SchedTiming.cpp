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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstUserAllocator.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3ClassGraph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCBase.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Sched.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

ਨਾਮਾਕਾਸ਼ V3Sched {

//============================================================================
// Trigger indices of the external domains of each variable

std::map<ਅਚਲ AstVarScope*, std::ਸਦਿਸ਼<uint32_t>>
TimingKit::externalTriggerIndices(ਅਚਲ TriggerKit& trigKit) ਅਚਲ {
    std::map<ਅਚਲ AstVarScope*, std::ਸਦਿਸ਼<uint32_t>> ਨਤੀਜਾ;
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& vscpDomains : m_externalDomains) {
        std::ਸਦਿਸ਼<uint32_t>& indices = ਨਤੀਜਾ[vscpDomains.ਪਹਿਲਾ];
        ਲਈ (ਅਚਲ Ast_Sen_ਰੁੱਖ* ਅਚਲ domainp : vscpDomains.ਦੂਜਾ) {
            ਲਈ (ਅਚਲ Ast_Sen_ਵਸਤੂ* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_ਵਸਤੂ)) {
                indices.ਧੱਕੋ_ਪਿੱਠ(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    ਜੇ (!m_resumeFuncp) {
        ਜੇ (m_lbs.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        // Create global resume function
        AstNetlist* ਅਚਲ netlistp = v3Global.rootp();
        AstScope* ਅਚਲ scopeTopp = netlistp->ਸਿਖਰ_Scopep()->scopep();
        m_resumeFuncp = ਨਵਾਂ AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(ਸੱਚ);
        m_resumeFuncp->isLoose(ਸੱਚ);
        m_resumeFuncp->isConst(ਝੂਠ);
        m_resumeFuncp->decl_ਨਿੱਜੀ(ਸੱਚ);
        scopeTopp->ਜੋੜੋ_Blocksp(m_resumeFuncp);

        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : m_lbs) {
            Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਇੰਦਰਾਜ.m_activep;
            activep->foreach([ਇਹ](AstCMethodHard* ਅਚਲ exprp) {
                ਜੇ (exprp->ਵਿਧੀ() != VCMethod::SCHED_RESUME) ਵਾਪਸ;
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = exprp->fromp();
                ਜੇ (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    ਵਾਪਸ;
                }
                AstCMethodHard* ਅਚਲ move_ਮੰਜ਼ਿਲ_Resumep = ਨਵਾਂ AstCMethodHard{
                    fromp->fileline(), fromp->clone_ਰੁੱਖ(ਝੂਠ),
                    VCMethod::SCHED_MOVE_ਮੰਜ਼ਿਲ_RESUME_ਕਤਾਰ,
                    exprp->pinsp() ? exprp->pinsp()->clone_ਰੁੱਖ(ਸੱਚ) : ਖਾਲੀ_ਸੂਚਕ};
                move_ਮੰਜ਼ਿਲ_Resumep->dtype_ਸੈੱਟ_Void();
                m_resumeFuncp->ਜੋੜੋ_Stmtsp(move_ਮੰਜ਼ਿਲ_Resumep->makeStmt());
            });
        }

        // Put all the timing actives in the resume function
        AstIf* dlyShedIfp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : m_lbs) {
            Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਇੰਦਰਾਜ.m_activep;
            // Resume time delays last. For no particular reason other than
            // that's what we used to do prior to proper #0 support.
            AstVarRef* ਅਚਲ schedrefp = VN_AS(
                VN_AS(VN_AS(activep->stmtsp(), Stmt_ਵਿਅੰਜਕ)->exprp(), CMethodHard)->fromp(), VarRef);

            Ast_ਗੰਢ* ਅਚਲ actionp = activep->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            ਜੇ (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                File_ਸਤਰ* ਅਚਲ flp = trigVscp->fileline();
                ਅਚਲ std::ਸੈੱਟ<uint32_t>& triggers = ਇੰਦਰਾਜ.m_triggers;
                AstTriggered* ਅਚਲ condp
                    = ਨਵਾਂ AstTriggered{flp, trigVscp, {triggers.begin(), triggers.ਸਿਰਾ()}};
                dlyShedIfp = ਨਵਾਂ AstIf{flp, condp};
                dlyShedIfp->ਜੋੜੋ_Thensp(actionp);
            } ਨਹੀਂਤਾਂ {
                m_resumeFuncp->ਜੋੜੋ_Stmtsp(actionp);
            }
        }
        ਜੇ (dlyShedIfp) m_resumeFuncp->ਜੋੜੋ_Stmtsp(dlyShedIfp);

        // These are now spent, oispose of now empty AstActive instances
        m_lbs.deleteActives();
    }
    AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
    callp->dtype_ਸੈੱਟ_Void();
    ਵਾਪਸ callp;
}

AstVarScope* TimingKit::getDelayScheduler(AstNetlist* ਅਚਲ /*netlistp*/) {
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : m_lbs) {
        Ast_ਸਰਗਰਮ* ਅਚਲ ap = ਇੰਦਰਾਜ.m_activep;
        // TODO: this triple VN_AS expression is ridiculous
        AstVarRef* ਅਚਲ schedrefp
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), Stmt_ਵਿਅੰਜਕ)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* ਅਚਲ vscp = schedrefp->varScopep();
        ਜੇ (vscp->dtypep()->basicp()->isDelayScheduler()) ਵਾਪਸ vscp;
    }
    // None found. Design doesn't use any time delays
    ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    ਜੇ (!m_readyFuncp) {
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : m_lbs) {
            Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਇੰਦਰਾਜ.m_activep;
            ਸਵੈਚਾਲਿਤ* ਅਚਲ resumep = VN_AS(VN_AS(activep->stmtsp(), Stmt_ਵਿਅੰਜਕ)->exprp(), CMethodHard);
            UASSERT_OBJ(!resumep->nextp(), resumep, "Should be the only statement here");
            AstVarScope* ਅਚਲ schedulerp = VN_AS(resumep->fromp(), VarRef)->varScopep();
            UASSERT_OBJ(schedulerp->dtypep()->basicp()->isDelayScheduler()
                            || schedulerp->dtypep()->basicp()->isTriggerScheduler()
                            || schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler(),
                        schedulerp, "Unexpected type");
            ਜੇ (!schedulerp->dtypep()->basicp()->isTriggerScheduler()) ਜਾਰੀ;
            // Create the global ready function only if we have trigger schedulers
            ਜੇ (!m_readyFuncp) {
                AstNetlist* ਅਚਲ netlistp = v3Global.rootp();
                AstScope* ਅਚਲ scopeTopp = netlistp->ਸਿਖਰ_Scopep()->scopep();
                m_readyFuncp = ਨਵਾਂ AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(ਸੱਚ);
                m_readyFuncp->isLoose(ਸੱਚ);
                m_readyFuncp->isConst(ਝੂਠ);
                m_readyFuncp->decl_ਨਿੱਜੀ(ਸੱਚ);
                scopeTopp->ਜੋੜੋ_Blocksp(m_readyFuncp);
            }

            File_ਸਤਰ* ਅਚਲ flp = activep->sentreep()->fileline();

            // Create an 'AstIf' sensitive to the suspending triggers
            ਅਚਲ std::ਸੈੱਟ<uint32_t>& triggers = ਇੰਦਰਾਜ.m_triggers;
            AstTriggered* ਅਚਲ condp
                = ਨਵਾਂ AstTriggered{flp, trigVscp, {triggers.begin(), triggers.ਸਿਰਾ()}};
            AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, condp};
            m_readyFuncp->ਜੋੜੋ_Stmtsp(ifp);

            // Mark as ready the processes resumed on this sensitivity expression
            AstVarRef* ਅਚਲ refp = ਨਵਾਂ AstVarRef{flp, schedulerp, VAccess::READWRITE};
            AstCMethodHard* ਅਚਲ callp = ਨਵਾਂ AstCMethodHard{flp, refp, VCMethod::SCHED_READY};
            callp->dtype_ਸੈੱਟ_Void();
            ਜੇ (resumep->pinsp()) callp->ਜੋੜੋ_Pinsp(resumep->pinsp()->clone_ਰੁੱਖ(ਝੂਠ));
            ifp->ਜੋੜੋ_Thensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        ਜੇ (!m_readyFuncp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }
    AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{m_readyFuncp->fileline(), m_readyFuncp};
    callp->dtype_ਸੈੱਟ_Void();
    ਵਾਪਸ callp;
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

ਵਰਗ AwaitVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    //  AstSenTree::user1()  -> bool.               Set true if the sentree has been visited.
    //  AstCFunc::user1()    -> AstUser1Allocator.  See alocator below
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;

    // STATE
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Process = ਝੂਠ;  // Are we in a process?
    ਤਾਰਕਿਕ m_gatherVars = ਝੂਠ;  // Should we gather vars in m_writtenBySuspendable?
    AstScope* ਅਚਲ m_scopeTopp;  // Scope at the top
    LogicByScope& m_lbs;  // Timing resume actives
    Ast_ਗੰਢ_Stmt*& m_postUpdatesr;  // Post updates for the trigger eval function
    // Additional var sensitivities
    std::map<ਅਚਲ AstVarScope*, std::ਸੈੱਟ<Ast_Sen_ਰੁੱਖ*>>& m_externalDomains;
    std::unique_ptr<V3_Class_ਗ੍ਰਾਫ>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::ਸੈੱਟ<Ast_Sen_ਰੁੱਖ*> m_processDomains;  // Sentrees from the current process
    // Variables written by suspendable processes
    std::ਸੈੱਟ<AstVarScope*> m_writtenBySuspendable;
    ਬਣਤਰ CFuncCache final {
        std::ਸੈੱਟ<Ast_Sen_ਰੁੱਖ*> m_processDomains;  // What shall be added to m_processDomains
        std::ਸੈੱਟ<AstVarScope*>
            m_writtenBySuspendable;  // What shall be added to m_writtenBySuspendable
        std::ਸੈੱਟ<AstCFunc*> m_includes;  // CFuncs whose CFuncCache shall be included into this
        // m_includes does not need to keep bool with m_gatherVars to know which cache shall be
        // used since:
        //  Only possible transition in a graph is from `!m_gatherVars` to `m_gatherVars`
        //  (in visit(AstFork*) there is a only possible transition, visit(AstNodeProcedure*) is
        //  not considered since it does not occur during a graph traversal)
        //  therefore, if change of state occurs it is certain that the first node after transition
        //  won't be in visiting state
        //  therefore, every node in a m_includes shall take value of m_gatherVars under which
        //  current CFuncCache is
        ਗਿਣਤੀ State : uint8_t {
            UNINITIALIZED = 0,  // Not initialized members are empty
            VISITING,  // Visiting - needed for breaking recursion
            INITIALIZED,  // Members contains correct values
        } m_state  // Current state of Cache
            = UNINITIALIZED;
    };
    std::ਸਦਿਸ਼<ਅਚਲ AstCFunc*> m_call_ਢੇਰ;  // Current callstack of AstCFuncs
    // Caches how visiting the function with given value of m_gatherVars changes
    // m_processDomains and m_writtenBySuspendable - only accessed from visit(AstCFunc* nodep)
    AstUser1Allocator<AstCFunc, std::array<CFuncCache, 2>> m_cfuncsCache;
    // Count uses of not inlined writes to signals in suspendables
    VDouble0 m_not_Inlined_Writes_ਵਿੱਚ_Suspendable_Usage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    ਖਾਲੀ ਜੋੜੋ_Resume_Pins(AstCMethodHard* ਅਚਲ resumep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* pinsp) {
        AstCExpr* ਅਚਲ exprp = VN_CAST(pinsp, CExpr);
        Ast_ਪਾਠ* ਅਚਲ textp = VN_CAST(exprp->nodesp(), ਪਾਠ_2);
        ਜੇ (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            ਜੇ ((pinsp = VN_CAST(pinsp->nextp(), ਗੰਢ_ਵਿਅੰਜਕ))) {
                resumep->ਜੋੜੋ_Pinsp(pinsp->clone_ਰੁੱਖ(ਝੂਠ));
            }
        } ਨਹੀਂਤਾਂ {
            resumep->ਜੋੜੋ_Pinsp(pinsp->clone_ਰੁੱਖ(ਝੂਠ));
        }
    }
    ਖਾਲੀ ਜੋੜੋ_CFunc_Cached_ਮੁੱਲਾਂ(ਅਚਲ AstCFunc* ਅਚਲ cfuncp,
                              std::unordered_ਸੈੱਟ<ਅਚਲ AstCFunc*>& ਦੇਖਿਆ) {
        ਜੇ (!ਦੇਖਿਆ.ਪਾਓ(cfuncp).ਦੂਜਾ) ਵਾਪਸ;
        CFuncCache& ਮੁੱਲ = m_cfuncsCache(cfuncp)[static_cast<ਮਾਪ_t>(m_gatherVars)];
        ਅਚਲ ਮਾਪ_t initial_ਮਾਪ = m_writtenBySuspendable.ਮਾਪ();
        m_writtenBySuspendable.ਪਾਓ(ਮੁੱਲ.m_writtenBySuspendable.begin(),
                                      ਮੁੱਲ.m_writtenBySuspendable.ਸਿਰਾ());
        m_not_Inlined_Writes_ਵਿੱਚ_Suspendable_Usage += m_writtenBySuspendable.ਮਾਪ() - initial_ਮਾਪ;
        m_processDomains.ਪਾਓ(ਮੁੱਲ.m_processDomains.begin(), ਮੁੱਲ.m_processDomains.ਸਿਰਾ());
        ਲਈ (ਅਚਲ AstCFunc* ਅਚਲ includedp : ਮੁੱਲ.m_includes) {
            ਜੋੜੋ_CFunc_Cached_ਮੁੱਲਾਂ(includedp, ਦੇਖਿਆ);
        }
    }
    // Create an active with a timing scheduler resume() call
    ਖਾਲੀ create_Resume_ਸਰਗਰਮ(AstCAwait* ਅਚਲ awaitp) {
        ਸਵੈਚਾਲਿਤ* ਅਚਲ methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* ਅਚਲ schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        Ast_Sen_ਰੁੱਖ* ਅਚਲ sentreep = awaitp->sentreep();
        File_ਸਤਰ* ਅਚਲ flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        ਸਵੈਚਾਲਿਤ* ਅਚਲ resumep = ਨਵਾਂ AstCMethodHard{
            flp, ਨਵਾਂ AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtype_ਸੈੱਟ_Void();
        ਜੇ (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ dbginfop = methodp->pinsp()->nextp()) {
                ਜੇ (methodp->pinsp()) ਜੋੜੋ_Resume_Pins(resumep, static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(dbginfop));
            }
        } ਨਹੀਂਤਾਂ ਜੇ (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            ਸਵੈਚਾਲਿਤ* ਅਚਲ postp = resumep->clone_ਰੁੱਖ(ਝੂਠ);
            postp->ਵਿਧੀ(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਨਵਾਂ Ast_ਸਰਗਰਮ{flp, "_timing", sentreep};
        activep->ਜੋੜੋ_Stmtsp(resumep->makeStmt());
        m_lbs.ਜੋੜੋ(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    ਖਾਲੀ ਦੇਖੋ_Called_CFunc(AstCFunc* ਅਚਲ nodep) {
        ਜੇ (!m_ਵਿੱਚ_Process) ਵਾਪਸ;  // Skip if not in process - nothing would be added to any set
        ਅਚਲ ਮਾਪ_t cache_ਕੁੰਜੀ = static_cast<ਮਾਪ_t>(m_gatherVars);
        CFuncCache& ਮੁੱਲ = m_cfuncsCache(nodep)[cache_ਕੁੰਜੀ];
        ਚੋਣ (ਮੁੱਲ.m_state) {
        ਹਾਲਤ CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            ਮੁੱਲ.m_state = CFuncCache::VISITING;
            m_call_ਢੇਰ.ਧੱਕੋ_ਪਿੱਠ(nodep);
            iterateChildren(nodep);
            m_call_ਢੇਰ.ਕੱਢੋ_ਪਿੱਠ();
            ਮੁੱਲ.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::ਅਦਲਾ_ਬਦਲੀ(m_processDomains, ਮੁੱਲ.m_processDomains);
            std::ਅਦਲਾ_ਬਦਲੀ(m_writtenBySuspendable, ਮੁੱਲ.m_writtenBySuspendable);
        } ਰੋਕੋ;
        ਹਾਲਤ CFuncCache::VISITING: {
            ਲਈ (ਮਾਪ_t i = m_call_ਢੇਰ.ਮਾਪ() - 1; m_call_ਢੇਰ.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_ਢੇਰ[i])[cache_ਕੁੰਜੀ].m_includes.ਪਾਓ(nodep);
            }
            ਵਾਪਸ;  // Break recursion
        }
        ਹਾਲਤ CFuncCache::INITIALIZED: ਰੋਕੋ;
        }
        std::unordered_ਸੈੱਟ<ਅਚਲ AstCFunc*> ਦੇਖਿਆ;
        ਜੋੜੋ_CFunc_Cached_ਮੁੱਲਾਂ(nodep, ਦੇਖਿਆ);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Procedure* ਅਚਲ nodep) override {
        UASSERT_OBJ(!m_ਵਿੱਚ_Process && !m_gatherVars && m_processDomains.ਖਾਲੀ_ਥਾਵਾਂ()
                        && m_writtenBySuspendable.ਖਾਲੀ_ਥਾਵਾਂ(),
                    nodep, "Process in process?");
        m_ਵਿੱਚ_Process = ਸੱਚ;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        ਲਈ (AstVarScope* ਅਚਲ vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].ਪਾਓ(m_processDomains.begin(), m_processDomains.ਸਿਰਾ());
            vscp->varp()->ਸੈੱਟ_Written_By_Suspendable();
        }
        m_processDomains.clear();
        m_writtenBySuspendable.clear();
        m_ਵਿੱਚ_Process = ਝੂਠ;
        m_gatherVars = ਝੂਠ;
    }
    ਖਾਲੀ ਦੇਖੋ(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        ਜੇ (m_ਵਿੱਚ_Process) m_gatherVars = ਸੱਚ;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCAwait* nodep) override {
        ਜੇ (Ast_Sen_ਰੁੱਖ* ਅਚਲ sentreep = nodep->sentreep()) {
            ਜੇ (!sentreep->user1_ਸੈੱਟ_Once()) create_Resume_ਸਰਗਰਮ(nodep);
            ਜੇ (m_ਵਿੱਚ_Process) m_processDomains.ਪਾਓ(sentreep);
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        ਜੇ (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.ਪਾਓ(nodep->varScopep());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_CCall* ਅਚਲ nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ਅਚਲ ਸਵੈਚਾਲਿਤ& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        ਜੇ (cfuncps.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਦੇਖੋ_Called_CFunc(nodep->funcp());
        } ਨਹੀਂਤਾਂ {
            ਲਈ (AstCFunc* ਅਚਲ cfuncp : cfuncps) ਦੇਖੋ_Called_CFunc(cfuncp);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* ਅਚਲ nodep) override {
        ਅਚਲ ਸਵੈਚਾਲਿਤ& ਮੁੱਲ = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        ਜੇ (ਮੁੱਲ[0].m_state != CFuncCache::UNINITIALIZED
            || ਮੁੱਲ[1].m_state != CFuncCache::UNINITIALIZED) {
            ਵਾਪਸ;
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਵਿਅੰਜਕ_Stmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, Ast_ਗੰਢ_Stmt*& postUpdatesr,
                          std::map<ਅਚਲ AstVarScope*, std::ਸੈੱਟ<Ast_Sen_ਰੁੱਖ*>>& externalDomains)
        : m_scopeTopp{nodep->ਸਿਖਰ_Scopep()->scopep()}
        , m_lbs{lbs}
        , m_postUpdatesr{postUpdatesr}
        , m_externalDomains{externalDomains}
        , m_classGraphp{V3_Class_ਗ੍ਰਾਫ::build(nodep)} {
        iterate(nodep);
    }
    ~AwaitVisitor() override {
        V3Stats::ਜੋੜੋ_Stat("Scheduling, count of non-inlined signal writes in suspendables",
                         m_not_Inlined_Writes_ਵਿੱਚ_Suspendable_Usage);
    }
};

TimingKit prepareTiming(AstNetlist* ਅਚਲ netlistp) {
    ਜੇ (!v3Global.ਵਰਤਦਾ_Timing()) ਵਾਪਸ {};
    LogicByScope lbs;
    Ast_ਗੰਢ_Stmt* postUpdates = ਖਾਲੀ_ਸੂਚਕ;
    std::map<ਅਚਲ AstVarScope*, std::ਸੈੱਟ<Ast_Sen_ਰੁੱਖ*>> externalDomains;
    { AwaitVisitor{netlistp, lbs, postUpdates, externalDomains}; }
    ਵਾਪਸ {std::move(lbs), postUpdates, std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

// Transform all forked processes into functions
ਵਰਗ TransformForksVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current fork.
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;

    // STATE
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Class = ਝੂਠ;  // Are we in a class?
    ਤਾਰਕਿਕ m_beginHasAwaits = ਝੂਠ;  // Does the current begin have awaits?
    AstFork* m_forkp = ਖਾਲੀ_ਸੂਚਕ;  // Current fork
    AstCFunc* m_funcp = ਖਾਲੀ_ਸੂਚਕ;  // Current function

    // METHODS
    // Remap local vars referenced by the given fork function
    // TODO: We should only pass variables to the fork that are
    // live in the fork body, but for that we need a proper data
    // flow analysis framework which we don't have at the moment
    ਖਾਲੀ remapLocals(AstCFunc* ਅਚਲ funcp, AstCCall* ਅਚਲ callp) {
        ਅਚਲ VNUser2_ਵਿੱਚ_Use user2_ਵਿੱਚ_Use;  // AstVarScope -> AstVarScope: var to remap to
        funcp->foreach([&](Ast_ਗੰਢ_Var_Ref* refp) {
            AstVar* ਅਚਲ varp = refp->varp();
            AstVarScope* ਅਚਲ vscp = refp->varScopep();
            AstBasicDType* ਅਚਲ dtypep = varp->dtypep()->basicp();
            // If not a fork..join, copy. All write refs should've been handled by V3Fork
            ਤਾਰਕਿਕ pass_By_ਮੁੱਲ = !m_forkp->joinType().join();
            ਜੇ (vscp->fork_By_ਮੁੱਲ()) {
                // Set just before the fork, for its processes to get by value
                pass_By_ਮੁੱਲ = ਸੱਚ;
            } ਨਹੀਂਤਾਂ ਜੇ (!varp->isFuncLocal()) {
                // Not func local. Its lifetime is longer than the forked process. Skip
                ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ ਜੇ (!varp->user1()) {
                // Not declared before the fork. It cannot outlive the forked process
                ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ ਜੇ (dtypep && dtypep->isForkSync()) {
                // We can just pass it by value to the new function
                pass_By_ਮੁੱਲ = ਸੱਚ;
            }
            // Remap the reference
            ਜੇ (!vscp->user2p()) {
                // Clone the var to the new function
                AstVar* ਅਚਲ newvarp
                    = ਨਵਾਂ AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                newvarp->funcLocal(ਸੱਚ);
                newvarp->direction(pass_By_ਮੁੱਲ ? VDirection::ਨਿਵੇਸ਼ : VDirection::REF);
                funcp->ਜੋੜੋ_Argsp(newvarp);
                AstVarScope* ਅਚਲ newvscp
                    = ਨਵਾਂ AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->ਜੋੜੋ_Varsp(newvscp);
                vscp->user2p(newvscp);
                callp->ਜੋੜੋ_Argsp(ਨਵਾਂ AstVarRef{refp->fileline(), vscp,
                                              pass_By_ਮੁੱਲ ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* ਅਚਲ newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Class);
        m_ਵਿੱਚ_Class = VN_IS(nodep, Class);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        m_funcp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        ਜੇ (!m_forkp) nodep->user1(ਸੱਚ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFork* nodep) override {
        ਜੇ (m_forkp) ਵਾਪਸ;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        Ast_ਗੰਢ* resp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ declsp = nodep->declsp()) {
            resp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(resp, declsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
        }
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ stmtsp = nodep->stmtsp()) {
            resp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(resp, stmtsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
        }
        ਜਦੋਂ (AstBegin* ਅਚਲ beginp = nodep->forksp()) {
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ declsp = beginp->declsp()) {
                resp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(resp, declsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
            }
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ stmtsp = beginp->stmtsp()) {
                resp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(resp, stmtsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
            }
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(beginp->unlink_Fr_ਪਿੱਠ()), beginp);
        }
        ਜੇ (resp) {
            nodep->replaceWith(resp);
        } ਨਹੀਂਤਾਂ {
            nodep->unlink_Fr_ਪਿੱਠ();
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstBegin* nodep) override {
        UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
        // Start with children, so later we only find awaits that are actually in this begin
        m_beginHasAwaits = ਝੂਠ;
        iterateChildrenConst(nodep);
        ਜੇ (!nodep->stmtsp()) ਵਾਪਸ;
        ਜੇ (!m_beginHasAwaits && !nodep->needProcess()) ਵਾਪਸ;

        UASSERT_OBJ(!nodep->name().ਖਾਲੀ_ਥਾਵਾਂ(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        AstCFunc* ਅਚਲ newfuncp = ਨਵਾਂ AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->decl_ਨਿੱਜੀ(ਸੱਚ);
        // Create the call to the function
        AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{flp, newfuncp};
        callp->dtype_ਸੈੱਟ_Void();
        // If we're in a class, add a vlSymsp arg
        ਜੇ (m_ਵਿੱਚ_Class) {
            newfuncp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ declsp = nodep->declsp()) {
            newfuncp->ਜੋੜੋ_Stmtsp(declsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
        }
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ stmtsp = nodep->stmtsp()) {
            newfuncp->ਜੋੜੋ_Stmtsp(stmtsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->ਜੋੜੋ_Stmtsp(callp->makeStmt());
        // Propagate if needs process
        ਜੇ (nodep->needProcess()) newfuncp->ਸੈੱਟ_Need_Process();
        remapLocals(newfuncp, callp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCAwait* nodep) override {
        m_beginHasAwaits = ਸੱਚ;
        iterateChildrenConst(nodep);
    }

    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = ਮੂਲ;
};

ਖਾਲੀ transformForks(AstNetlist* ਅਚਲ netlistp) {
    ਜੇ (!v3Global.ਵਰਤਦਾ_Timing()) ਵਾਪਸ;
    { TransformForksVisitor{netlistp}; }
    V3Global::dump_Check_Global_ਰੁੱਖ("transform_forks", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}

}  // namespace V3Sched
