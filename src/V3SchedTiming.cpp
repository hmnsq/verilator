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

std::map<ಧ್ರುವ AstVarScope*, std::vector<uint32_t>>
TimingKit::externalTriggerIndices(ಧ್ರುವ TriggerKit& trigKit) ಧ್ರುವ {
    std::map<ಧ್ರುವ AstVarScope*, std::vector<uint32_t>> result;
    ಪ್ರತಿ (ಧ್ರುವ auto& vscpDomains : m_externalDomains) {
        std::vector<uint32_t>& indices = result[vscpDomains.first];
        ಪ್ರತಿ (ಧ್ರುವ Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ domainp : vscpDomains.second) {
            ಪ್ರತಿ (ಧ್ರುವ AstSenItem* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                indices.ತಳ್ಳು_back(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    ಯದಿ (!m_resumeFuncp) {
        ಯದಿ (m_lbs.empty()) ಪ್ರತ್ಯಾಗಮ nullptr;
        // Create global resume function
        AstNetlist* ಧ್ರುವ netlistp = v3Global.rootp();
        AstScope* ಧ್ರುವ scopeTopp = netlistp->ತುದಿ_Scopep()->scopep();
        m_resumeFuncp = ನೂತನ AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(ಸತ್ಯ);
        m_resumeFuncp->isLoose(ಸತ್ಯ);
        m_resumeFuncp->isConst(ಅಸತ್ಯ);
        m_resumeFuncp->declPrivate(ಸತ್ಯ);
        scopeTopp->addBlocksp(m_resumeFuncp);

        ಪ್ರತಿ (ಧ್ರುವ auto& entry : m_lbs) {
            AstActive* ಧ್ರುವ activep = entry.m_activep;
            activep->foreach([ಏತತ್](AstCMethodHard* ಧ್ರುವ exprp) {
                ಯದಿ (exprp->method() != VCMethod::SCHED_RESUME) ಪ್ರತ್ಯಾಗಮ;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = exprp->fromp();
                ಯದಿ (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    ಪ್ರತ್ಯಾಗಮ;
                }
                AstCMethodHard* ಧ್ರುವ moveToResumep = ನೂತನ AstCMethodHard{
                    fromp->fileline(), fromp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ),
                    VCMethod::SCHED_MOVE_TO_RESUME_QUEUE,
                    exprp->pinsp() ? exprp->pinsp()->clone_ವೃಕ್ಷ(ಸತ್ಯ) : nullptr};
                moveToResumep->dtypeSetVoid();
                m_resumeFuncp->addStmtsp(moveToResumep->makeStmt());
            });
        }

        // Put all the timing actives in the resume function
        AstIf* dlyShedIfp = nullptr;
        ಪ್ರತಿ (ಧ್ರುವ auto& entry : m_lbs) {
            AstActive* ಧ್ರುವ activep = entry.m_activep;
            // Resume time delays last. For no particular reason other than
            // that's what we used to do prior to proper #0 support.
            AstVarRef* ಧ್ರುವ schedrefp = VN_AS(
                VN_AS(VN_AS(activep->stmtsp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)->exprp(), CMethodHard)->fromp(), VarRef);

            Ast_ಗಂಟು* ಧ್ರುವ actionp = activep->stmtsp()->unlinkFrBackWithNext();
            ಯದಿ (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                File_ಸಾಲು* ಧ್ರುವ flp = trigVscp->fileline();
                ಧ್ರುವ std::set<uint32_t>& triggers = entry.m_triggers;
                AstTriggered* ಧ್ರುವ condp
                    = ನೂತನ AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
                dlyShedIfp = ನೂತನ AstIf{flp, condp};
                dlyShedIfp->addThensp(actionp);
            } ಅನ್ಯಥಾ {
                m_resumeFuncp->addStmtsp(actionp);
            }
        }
        ಯದಿ (dlyShedIfp) m_resumeFuncp->addStmtsp(dlyShedIfp);

        // These are now spent, oispose of now empty AstActive instances
        m_lbs.deleteActives();
    }
    AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
    callp->dtypeSetVoid();
    ಪ್ರತ್ಯಾಗಮ callp;
}

AstVarScope* TimingKit::getDelayScheduler(AstNetlist* ಧ್ರುವ /*netlistp*/) {
    ಪ್ರತಿ (ಧ್ರುವ auto& entry : m_lbs) {
        AstActive* ಧ್ರುವ ap = entry.m_activep;
        // TODO: this triple VN_AS expression is ridiculous
        AstVarRef* ಧ್ರುವ schedrefp
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* ಧ್ರುವ vscp = schedrefp->varScopep();
        ಯದಿ (vscp->dtypep()->basicp()->isDelayScheduler()) ಪ್ರತ್ಯಾಗಮ vscp;
    }
    // None found. Design doesn't use any time delays
    ಪ್ರತ್ಯಾಗಮ nullptr;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    ಯದಿ (!m_readyFuncp) {
        ಪ್ರತಿ (ಧ್ರುವ auto& entry : m_lbs) {
            AstActive* ಧ್ರುವ activep = entry.m_activep;
            auto* ಧ್ರುವ resumep = VN_AS(VN_AS(activep->stmtsp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)->exprp(), CMethodHard);
            UASSERT_OBJ(!resumep->nextp(), resumep, "Should be the only statement here");
            AstVarScope* ಧ್ರುವ schedulerp = VN_AS(resumep->fromp(), VarRef)->varScopep();
            UASSERT_OBJ(schedulerp->dtypep()->basicp()->isDelayScheduler()
                            || schedulerp->dtypep()->basicp()->isTriggerScheduler()
                            || schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler(),
                        schedulerp, "Unexpected type");
            ಯದಿ (!schedulerp->dtypep()->basicp()->isTriggerScheduler()) ಅನುವರ್ತ;
            // Create the global ready function only if we have trigger schedulers
            ಯದಿ (!m_readyFuncp) {
                AstNetlist* ಧ್ರುವ netlistp = v3Global.rootp();
                AstScope* ಧ್ರುವ scopeTopp = netlistp->ತುದಿ_Scopep()->scopep();
                m_readyFuncp = ನೂತನ AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(ಸತ್ಯ);
                m_readyFuncp->isLoose(ಸತ್ಯ);
                m_readyFuncp->isConst(ಅಸತ್ಯ);
                m_readyFuncp->declPrivate(ಸತ್ಯ);
                scopeTopp->addBlocksp(m_readyFuncp);
            }

            File_ಸಾಲು* ಧ್ರುವ flp = activep->sentreep()->fileline();

            // Create an 'AstIf' sensitive to the suspending triggers
            ಧ್ರುವ std::set<uint32_t>& triggers = entry.m_triggers;
            AstTriggered* ಧ್ರುವ condp
                = ನೂತನ AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
            AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{flp, condp};
            m_readyFuncp->addStmtsp(ifp);

            // Mark as ready the processes resumed on this sensitivity expression
            AstVarRef* ಧ್ರುವ refp = ನೂತನ AstVarRef{flp, schedulerp, VAccess::READWRITE};
            AstCMethodHard* ಧ್ರುವ callp = ನೂತನ AstCMethodHard{flp, refp, VCMethod::SCHED_READY};
            callp->dtypeSetVoid();
            ಯದಿ (resumep->pinsp()) callp->addPinsp(resumep->pinsp()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
            ifp->addThensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        ಯದಿ (!m_readyFuncp) ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{m_readyFuncp->fileline(), m_readyFuncp};
    callp->dtypeSetVoid();
    ಪ್ರತ್ಯಾಗಮ callp;
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

ವರ್ಗ AwaitVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  AstSenTree::user1()  -> bool.               Set true if the sentree has been visited.
    //  AstCFunc::user1()    -> AstUser1Allocator.  See alocator below
    ಧ್ರುವ VNUser1InUse m_inuser1;

    // STATE
    ತಾರ್ಕಿಕ m_inProcess = ಅಸತ್ಯ;  // Are we in a process?
    ತಾರ್ಕಿಕ m_gatherVars = ಅಸತ್ಯ;  // Should we gather vars in m_writtenBySuspendable?
    AstScope* ಧ್ರುವ m_scopeTopp;  // Scope at the top
    LogicByScope& m_lbs;  // Timing resume actives
    Ast_ಗಂಟು_Stmt*& m_postUpdatesr;  // Post updates for the trigger eval function
    // Additional var sensitivities
    std::map<ಧ್ರುವ AstVarScope*, std::set<Ast_Sen_ವೃಕ್ಷ*>>& m_externalDomains;
    std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::set<Ast_Sen_ವೃಕ್ಷ*> m_processDomains;  // Sentrees from the current process
    // Variables written by suspendable processes
    std::set<AstVarScope*> m_writtenBySuspendable;
    ಸಂರಚನಾ CFuncCache final {
        std::set<Ast_Sen_ವೃಕ್ಷ*> m_processDomains;  // What shall be added to m_processDomains
        std::set<AstVarScope*>
            m_writtenBySuspendable;  // What shall be added to m_writtenBySuspendable
        std::set<AstCFunc*> m_includes;  // CFuncs whose CFuncCache shall be included into this
        // m_includes does not need to keep bool with m_gatherVars to know which cache shall be
        // used since:
        //  Only possible transition in a graph is from `!m_gatherVars` to `m_gatherVars`
        //  (in visit(AstFork*) there is a only possible transition, visit(AstNodeProcedure*) is
        //  not considered since it does not occur during a graph traversal)
        //  therefore, if change of state occurs it is certain that the first node after transition
        //  won't be in visiting state
        //  therefore, every node in a m_includes shall take value of m_gatherVars under which
        //  current CFuncCache is
        ಗಣನಾ State : uint8_t {
            UNINITIALIZED = 0,  // Not initialized members are empty
            VISITING,  // Visiting - needed for breaking recursion
            INITIALIZED,  // Members contains correct values
        } m_state  // Current state of Cache
            = UNINITIALIZED;
    };
    std::vector<ಧ್ರುವ AstCFunc*> m_call_ರಾಶಿ;  // Current callstack of AstCFuncs
    // Caches how visiting the function with given value of m_gatherVars changes
    // m_processDomains and m_writtenBySuspendable - only accessed from visit(AstCFunc* nodep)
    AstUser1Allocator<AstCFunc, std::array<CFuncCache, 2>> m_cfuncsCache;
    // Count uses of not inlined writes to signals in suspendables
    VDouble0 m_notInlinedWritesInSuspendableUsage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    ಶೂನ್ಯ addResumePins(AstCMethodHard* ಧ್ರುವ resumep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pinsp) {
        AstCExpr* ಧ್ರುವ exprp = VN_CAST(pinsp, CExpr);
        AstText* ಧ್ರುವ textp = VN_CAST(exprp->nodesp(), Text);
        ಯದಿ (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            ಯದಿ ((pinsp = VN_CAST(pinsp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ))) {
                resumep->addPinsp(pinsp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
            }
        } ಅನ್ಯಥಾ {
            resumep->addPinsp(pinsp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
        }
    }
    ಶೂನ್ಯ add_CFunc_Cached_ಮೌಲ್ಯಗಳು(ಧ್ರುವ AstCFunc* ಧ್ರುವ cfuncp,
                              std::unordered_set<ಧ್ರುವ AstCFunc*>& visited) {
        ಯದಿ (!visited.ಸೇರಿಸು(cfuncp).second) ಪ್ರತ್ಯಾಗಮ;
        CFuncCache& ಮೌಲ್ಯ = m_cfuncsCache(cfuncp)[static_cast<size_t>(m_gatherVars)];
        ಧ್ರುವ size_t initialSize = m_writtenBySuspendable.size();
        m_writtenBySuspendable.ಸೇರಿಸು(ಮೌಲ್ಯ.m_writtenBySuspendable.begin(),
                                      ಮೌಲ್ಯ.m_writtenBySuspendable.end());
        m_notInlinedWritesInSuspendableUsage += m_writtenBySuspendable.size() - initialSize;
        m_processDomains.ಸೇರಿಸು(ಮೌಲ್ಯ.m_processDomains.begin(), ಮೌಲ್ಯ.m_processDomains.end());
        ಪ್ರತಿ (ಧ್ರುವ AstCFunc* ಧ್ರುವ includedp : ಮೌಲ್ಯ.m_includes) {
            add_CFunc_Cached_ಮೌಲ್ಯಗಳು(includedp, visited);
        }
    }
    // Create an active with a timing scheduler resume() call
    ಶೂನ್ಯ createResumeActive(AstCAwait* ಧ್ರುವ awaitp) {
        auto* ಧ್ರುವ methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* ಧ್ರುವ schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ sentreep = awaitp->sentreep();
        File_ಸಾಲು* ಧ್ರುವ flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        auto* ಧ್ರುವ resumep = ನೂತನ AstCMethodHard{
            flp, ನೂತನ AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtypeSetVoid();
        ಯದಿ (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ dbginfop = methodp->pinsp()->nextp()) {
                ಯದಿ (methodp->pinsp()) addResumePins(resumep, static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(dbginfop));
            }
        } ಅನ್ಯಥಾ ಯದಿ (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            auto* ಧ್ರುವ postp = resumep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            postp->method(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = Ast_ಗಂಟು::addNext(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        AstActive* ಧ್ರುವ activep = ನೂತನ AstActive{flp, "_timing", sentreep};
        activep->addStmtsp(resumep->makeStmt());
        m_lbs.add(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    ಶೂನ್ಯ visitCalledCFunc(AstCFunc* ಧ್ರುವ nodep) {
        ಯದಿ (!m_inProcess) ಪ್ರತ್ಯಾಗಮ;  // Skip if not in process - nothing would be added to any set
        ಧ್ರುವ size_t cache_ಕೀಲಿ = static_cast<size_t>(m_gatherVars);
        CFuncCache& ಮೌಲ್ಯ = m_cfuncsCache(nodep)[cache_ಕೀಲಿ];
        ವಿಕಲ್ಪ (ಮೌಲ್ಯ.m_state) {
        ಪಕ್ಷ CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            ಮೌಲ್ಯ.m_state = CFuncCache::VISITING;
            m_call_ರಾಶಿ.ತಳ್ಳು_back(nodep);
            iterateChildren(nodep);
            m_call_ರಾಶಿ.ತೆಗೆ_back();
            ಮೌಲ್ಯ.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_processDomains, ಮೌಲ್ಯ.m_processDomains);
            std::swap(m_writtenBySuspendable, ಮೌಲ್ಯ.m_writtenBySuspendable);
        } ಭಙ್ಗ;
        ಪಕ್ಷ CFuncCache::VISITING: {
            ಪ್ರತಿ (size_t i = m_call_ರಾಶಿ.size() - 1; m_call_ರಾಶಿ.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_ರಾಶಿ[i])[cache_ಕೀಲಿ].m_includes.ಸೇರಿಸು(nodep);
            }
            ಪ್ರತ್ಯಾಗಮ;  // Break recursion
        }
        ಪಕ್ಷ CFuncCache::INITIALIZED: ಭಙ್ಗ;
        }
        std::unordered_set<ಧ್ರುವ AstCFunc*> visited;
        add_CFunc_Cached_ಮೌಲ್ಯಗಳು(nodep, visited);
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Procedure* ಧ್ರುವ nodep) override {
        UASSERT_OBJ(!m_inProcess && !m_gatherVars && m_processDomains.empty()
                        && m_writtenBySuspendable.empty(),
                    nodep, "Process in process?");
        m_inProcess = ಸತ್ಯ;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        ಪ್ರತಿ (AstVarScope* ಧ್ರುವ vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].ಸೇರಿಸು(m_processDomains.begin(), m_processDomains.end());
            vscp->varp()->setWrittenBySuspendable();
        }
        m_processDomains.clear();
        m_writtenBySuspendable.clear();
        m_inProcess = ಅಸತ್ಯ;
        m_gatherVars = ಅಸತ್ಯ;
    }
    ಶೂನ್ಯ visit(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        ಯದಿ (m_inProcess) m_gatherVars = ಸತ್ಯ;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstCAwait* nodep) override {
        ಯದಿ (Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ sentreep = nodep->sentreep()) {
            ಯದಿ (!sentreep->user1SetOnce()) createResumeActive(nodep);
            ಯದಿ (m_inProcess) m_processDomains.ಸೇರಿಸು(sentreep);
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಯದಿ (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.ಸೇರಿಸು(nodep->varScopep());
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_CCall* ಧ್ರುವ nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ಧ್ರುವ auto& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        ಯದಿ (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } ಅನ್ಯಥಾ {
            ಪ್ರತಿ (AstCFunc* ಧ್ರುವ cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    ಶೂನ್ಯ visit(AstCFunc* ಧ್ರುವ nodep) override {
        ಧ್ರುವ auto& ಮೌಲ್ಯ = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        ಯದಿ (ಮೌಲ್ಯ[0].m_state != CFuncCache::UNINITIALIZED
            || ಮೌಲ್ಯ[1].m_state != CFuncCache::UNINITIALIZED) {
            ಪ್ರತ್ಯಾಗಮ;
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, Ast_ಗಂಟು_Stmt*& postUpdatesr,
                          std::map<ಧ್ರುವ AstVarScope*, std::set<Ast_Sen_ವೃಕ್ಷ*>>& externalDomains)
        : m_scopeTopp{nodep->ತುದಿ_Scopep()->scopep()}
        , m_lbs{lbs}
        , m_postUpdatesr{postUpdatesr}
        , m_externalDomains{externalDomains}
        , m_classGraphp{V3ClassGraph::build(nodep)} {
        iterate(nodep);
    }
    ~AwaitVisitor() override {
        V3Stats::addStat("Scheduling, count of non-inlined signal writes in suspendables",
                         m_notInlinedWritesInSuspendableUsage);
    }
};

TimingKit prepareTiming(AstNetlist* ಧ್ರುವ netlistp) {
    ಯದಿ (!v3Global.usesTiming()) ಪ್ರತ್ಯಾಗಮ {};
    LogicByScope lbs;
    Ast_ಗಂಟು_Stmt* postUpdates = nullptr;
    std::map<ಧ್ರುವ AstVarScope*, std::set<Ast_Sen_ವೃಕ್ಷ*>> externalDomains;
    { AwaitVisitor{netlistp, lbs, postUpdates, externalDomains}; }
    ಪ್ರತ್ಯಾಗಮ {std::move(lbs), postUpdates, std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

// Transform all forked processes into functions
ವರ್ಗ TransformForksVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current fork.
    ಧ್ರುವ VNUser1InUse m_inuser1;

    // STATE
    ತಾರ್ಕಿಕ m_inClass = ಅಸತ್ಯ;  // Are we in a class?
    ತಾರ್ಕಿಕ m_beginHasAwaits = ಅಸತ್ಯ;  // Does the current begin have awaits?
    AstFork* m_forkp = nullptr;  // Current fork
    AstCFunc* m_funcp = nullptr;  // Current function

    // METHODS
    // Remap local vars referenced by the given fork function
    // TODO: We should only pass variables to the fork that are
    // live in the fork body, but for that we need a proper data
    // flow analysis framework which we don't have at the moment
    ಶೂನ್ಯ remapLocals(AstCFunc* ಧ್ರುವ funcp, AstCCall* ಧ್ರುವ callp) {
        ಧ್ರುವ VNUser2InUse user2InUse;  // AstVarScope -> AstVarScope: var to remap to
        funcp->foreach([&](Ast_ಗಂಟು_Var_Ref* refp) {
            AstVar* ಧ್ರುವ varp = refp->varp();
            AstVarScope* ಧ್ರುವ vscp = refp->varScopep();
            AstBasicDType* ಧ್ರುವ dtypep = varp->dtypep()->basicp();
            // If not a fork..join, copy. All write refs should've been handled by V3Fork
            ತಾರ್ಕಿಕ pass_By_ಮೌಲ್ಯ = !m_forkp->joinType().join();
            ಯದಿ (vscp->fork_By_ಮೌಲ್ಯ()) {
                // Set just before the fork, for its processes to get by value
                pass_By_ಮೌಲ್ಯ = ಸತ್ಯ;
            } ಅನ್ಯಥಾ ಯದಿ (!varp->isFuncLocal()) {
                // Not func local. Its lifetime is longer than the forked process. Skip
                ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ ಯದಿ (!varp->user1()) {
                // Not declared before the fork. It cannot outlive the forked process
                ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ ಯದಿ (dtypep && dtypep->isForkSync()) {
                // We can just pass it by value to the new function
                pass_By_ಮೌಲ್ಯ = ಸತ್ಯ;
            }
            // Remap the reference
            ಯದಿ (!vscp->user2p()) {
                // Clone the var to the new function
                AstVar* ಧ್ರುವ newvarp
                    = ನೂತನ AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                newvarp->funcLocal(ಸತ್ಯ);
                newvarp->direction(pass_By_ಮೌಲ್ಯ ? VDirection::ಒಳಹರಿವು : VDirection::REF);
                funcp->addArgsp(newvarp);
                AstVarScope* ಧ್ರುವ newvscp
                    = ನೂತನ AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->addVarsp(newvscp);
                vscp->user2p(newvscp);
                callp->addArgsp(ನೂತನ AstVarRef{refp->fileline(), vscp,
                                              pass_By_ಮೌಲ್ಯ ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* ಧ್ರುವ newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_inClass);
        m_inClass = VN_IS(nodep, Class);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        m_funcp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        ಯದಿ (!m_forkp) nodep->user1(ಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstFork* nodep) override {
        ಯದಿ (m_forkp) ಪ್ರತ್ಯಾಗಮ;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        Ast_ಗಂಟು* resp = nullptr;
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ declsp = nodep->declsp()) {
            resp = Ast_ಗಂಟು::addNext(resp, declsp->unlinkFrBackWithNext());
        }
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ stmtsp = nodep->stmtsp()) {
            resp = Ast_ಗಂಟು::addNext(resp, stmtsp->unlinkFrBackWithNext());
        }
        ಯಾವತ್ (AstBegin* ಧ್ರುವ beginp = nodep->forksp()) {
            ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ declsp = beginp->declsp()) {
                resp = Ast_ಗಂಟು::addNext(resp, declsp->unlinkFrBackWithNext());
            }
            ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ stmtsp = beginp->stmtsp()) {
                resp = Ast_ಗಂಟು::addNext(resp, stmtsp->unlinkFrBackWithNext());
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(beginp->unlinkFrBack()), beginp);
        }
        ಯದಿ (resp) {
            nodep->replaceWith(resp);
        } ಅನ್ಯಥಾ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstBegin* nodep) override {
        UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
        // Start with children, so later we only find awaits that are actually in this begin
        m_beginHasAwaits = ಅಸತ್ಯ;
        iterateChildrenConst(nodep);
        ಯದಿ (!nodep->stmtsp()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!m_beginHasAwaits && !nodep->needProcess()) ಪ್ರತ್ಯಾಗಮ;

        UASSERT_OBJ(!nodep->name().empty(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        AstCFunc* ಧ್ರುವ newfuncp = ನೂತನ AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->addNextHere(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->declPrivate(ಸತ್ಯ);
        // Create the call to the function
        AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{flp, newfuncp};
        callp->dtypeSetVoid();
        // If we're in a class, add a vlSymsp arg
        ಯದಿ (m_inClass) {
            newfuncp->addStmtsp(ನೂತನ AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ declsp = nodep->declsp()) {
            newfuncp->addStmtsp(declsp->unlinkFrBackWithNext());
        }
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ stmtsp = nodep->stmtsp()) {
            newfuncp->addStmtsp(stmtsp->unlinkFrBackWithNext());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->addStmtsp(callp->makeStmt());
        // Propagate if needs process
        ಯದಿ (nodep->needProcess()) newfuncp->setNeedProcess();
        remapLocals(newfuncp, callp);
    }
    ಶೂನ್ಯ visit(AstCAwait* nodep) override {
        m_beginHasAwaits = ಸತ್ಯ;
        iterateChildrenConst(nodep);
    }

    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = ಸ್ವಭಾವ;
};

ಶೂನ್ಯ transformForks(AstNetlist* ಧ್ರುವ netlistp) {
    ಯದಿ (!v3Global.usesTiming()) ಪ್ರತ್ಯಾಗಮ;
    { TransformForksVisitor{netlistp}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("transform_forks", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

}  // namespace V3Sched
