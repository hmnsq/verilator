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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3AstUserAllocator.h"
#ଅନ୍ତର୍ଭାଵ "V3ClassGraph.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitCBase.h"
#ଅନ୍ତର୍ଭାଵ "V3Sched.h"

#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

ନାମାକାଶ V3Sched {

//============================================================================
// Trigger indices of the external domains of each variable

std::map<ଅଚଳ AstVarScope*, std::vector<uint32_t>>
TimingKit::externalTriggerIndices(ଅଚଳ TriggerKit& trigKit) ଅଚଳ {
    std::map<ଅଚଳ AstVarScope*, std::vector<uint32_t>> result;
    ପାଇଁ (ଅଚଳ auto& vscpDomains : m_externalDomains) {
        std::vector<uint32_t>& indices = result[vscpDomains.first];
        ପାଇଁ (ଅଚଳ Ast_Sen_ବୃକ୍ଷ* ଅଚଳ domainp : vscpDomains.second) {
            ପାଇଁ (ଅଚଳ AstSenItem* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                indices.ଠେଲ_back(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    ଫେରନ୍ତୁ result;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    ଯଦି (!m_resumeFuncp) {
        ଯଦି (m_lbs.empty()) ଫେରନ୍ତୁ nullptr;
        // Create global resume function
        AstNetlist* ଅଚଳ netlistp = v3Global.rootp();
        AstScope* ଅଚଳ scopeTopp = netlistp->ଶୀର୍ଷ_Scopep()->scopep();
        m_resumeFuncp = ନୂତନ AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(ସତ୍ୟ);
        m_resumeFuncp->isLoose(ସତ୍ୟ);
        m_resumeFuncp->isConst(ମିଥ୍ୟା);
        m_resumeFuncp->declPrivate(ସତ୍ୟ);
        scopeTopp->addBlocksp(m_resumeFuncp);

        ପାଇଁ (ଅଚଳ auto& entry : m_lbs) {
            AstActive* ଅଚଳ activep = entry.m_activep;
            activep->foreach([ଏହି](AstCMethodHard* ଅଚଳ exprp) {
                ଯଦି (exprp->method() != VCMethod::SCHED_RESUME) ଫେରନ୍ତୁ;
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = exprp->fromp();
                ଯଦି (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    ଫେରନ୍ତୁ;
                }
                AstCMethodHard* ଅଚଳ moveToResumep = ନୂତନ AstCMethodHard{
                    fromp->fileline(), fromp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା),
                    VCMethod::SCHED_MOVE_TO_RESUME_QUEUE,
                    exprp->pinsp() ? exprp->pinsp()->clone_ବୃକ୍ଷ(ସତ୍ୟ) : nullptr};
                moveToResumep->dtypeSetVoid();
                m_resumeFuncp->addStmtsp(moveToResumep->makeStmt());
            });
        }

        // Put all the timing actives in the resume function
        AstIf* dlyShedIfp = nullptr;
        ପାଇଁ (ଅଚଳ auto& entry : m_lbs) {
            AstActive* ଅଚଳ activep = entry.m_activep;
            // Resume time delays last. For no particular reason other than
            // that's what we used to do prior to proper #0 support.
            AstVarRef* ଅଚଳ schedrefp = VN_AS(
                VN_AS(VN_AS(activep->stmtsp(), Stmt_ପଦସମଷ୍ଟି)->exprp(), CMethodHard)->fromp(), VarRef);

            Ast_ଗଣ୍ଠି* ଅଚଳ actionp = activep->stmtsp()->unlinkFrBackWithNext();
            ଯଦି (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                File_ଧାଡ଼ି* ଅଚଳ flp = trigVscp->fileline();
                ଅଚଳ std::set<uint32_t>& triggers = entry.m_triggers;
                AstTriggered* ଅଚଳ condp
                    = ନୂତନ AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
                dlyShedIfp = ନୂତନ AstIf{flp, condp};
                dlyShedIfp->addThensp(actionp);
            } ଅନ୍ୟଥା {
                m_resumeFuncp->addStmtsp(actionp);
            }
        }
        ଯଦି (dlyShedIfp) m_resumeFuncp->addStmtsp(dlyShedIfp);

        // These are now spent, oispose of now empty AstActive instances
        m_lbs.deleteActives();
    }
    AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
    callp->dtypeSetVoid();
    ଫେରନ୍ତୁ callp;
}

AstVarScope* TimingKit::getDelayScheduler(AstNetlist* ଅଚଳ /*netlistp*/) {
    ପାଇଁ (ଅଚଳ auto& entry : m_lbs) {
        AstActive* ଅଚଳ ap = entry.m_activep;
        // TODO: this triple VN_AS expression is ridiculous
        AstVarRef* ଅଚଳ schedrefp
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), Stmt_ପଦସମଷ୍ଟି)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* ଅଚଳ vscp = schedrefp->varScopep();
        ଯଦି (vscp->dtypep()->basicp()->isDelayScheduler()) ଫେରନ୍ତୁ vscp;
    }
    // None found. Design doesn't use any time delays
    ଫେରନ୍ତୁ nullptr;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    ଯଦି (!m_readyFuncp) {
        ପାଇଁ (ଅଚଳ auto& entry : m_lbs) {
            AstActive* ଅଚଳ activep = entry.m_activep;
            auto* ଅଚଳ resumep = VN_AS(VN_AS(activep->stmtsp(), Stmt_ପଦସମଷ୍ଟି)->exprp(), CMethodHard);
            UASSERT_OBJ(!resumep->nextp(), resumep, "Should be the only statement here");
            AstVarScope* ଅଚଳ schedulerp = VN_AS(resumep->fromp(), VarRef)->varScopep();
            UASSERT_OBJ(schedulerp->dtypep()->basicp()->isDelayScheduler()
                            || schedulerp->dtypep()->basicp()->isTriggerScheduler()
                            || schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler(),
                        schedulerp, "Unexpected type");
            ଯଦି (!schedulerp->dtypep()->basicp()->isTriggerScheduler()) ଜାରି;
            // Create the global ready function only if we have trigger schedulers
            ଯଦି (!m_readyFuncp) {
                AstNetlist* ଅଚଳ netlistp = v3Global.rootp();
                AstScope* ଅଚଳ scopeTopp = netlistp->ଶୀର୍ଷ_Scopep()->scopep();
                m_readyFuncp = ନୂତନ AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(ସତ୍ୟ);
                m_readyFuncp->isLoose(ସତ୍ୟ);
                m_readyFuncp->isConst(ମିଥ୍ୟା);
                m_readyFuncp->declPrivate(ସତ୍ୟ);
                scopeTopp->addBlocksp(m_readyFuncp);
            }

            File_ଧାଡ଼ି* ଅଚଳ flp = activep->sentreep()->fileline();

            // Create an 'AstIf' sensitive to the suspending triggers
            ଅଚଳ std::set<uint32_t>& triggers = entry.m_triggers;
            AstTriggered* ଅଚଳ condp
                = ନୂତନ AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
            AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, condp};
            m_readyFuncp->addStmtsp(ifp);

            // Mark as ready the processes resumed on this sensitivity expression
            AstVarRef* ଅଚଳ refp = ନୂତନ AstVarRef{flp, schedulerp, VAccess::READWRITE};
            AstCMethodHard* ଅଚଳ callp = ନୂତନ AstCMethodHard{flp, refp, VCMethod::SCHED_READY};
            callp->dtypeSetVoid();
            ଯଦି (resumep->pinsp()) callp->addPinsp(resumep->pinsp()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
            ifp->addThensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        ଯଦି (!m_readyFuncp) ଫେରନ୍ତୁ nullptr;
    }
    AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{m_readyFuncp->fileline(), m_readyFuncp};
    callp->dtypeSetVoid();
    ଫେରନ୍ତୁ callp;
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

ଶ୍ରେଣୀ AwaitVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    //  AstSenTree::user1()  -> bool.               Set true if the sentree has been visited.
    //  AstCFunc::user1()    -> AstUser1Allocator.  See alocator below
    ଅଚଳ VNUser1InUse m_inuser1;

    // STATE
    ତାର୍କିକ m_inProcess = ମିଥ୍ୟା;  // Are we in a process?
    ତାର୍କିକ m_gatherVars = ମିଥ୍ୟା;  // Should we gather vars in m_writtenBySuspendable?
    AstScope* ଅଚଳ m_scopeTopp;  // Scope at the top
    LogicByScope& m_lbs;  // Timing resume actives
    Ast_ଗଣ୍ଠି_Stmt*& m_postUpdatesr;  // Post updates for the trigger eval function
    // Additional var sensitivities
    std::map<ଅଚଳ AstVarScope*, std::set<Ast_Sen_ବୃକ୍ଷ*>>& m_externalDomains;
    std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::set<Ast_Sen_ବୃକ୍ଷ*> m_processDomains;  // Sentrees from the current process
    // Variables written by suspendable processes
    std::set<AstVarScope*> m_writtenBySuspendable;
    ସଂରଚନା CFuncCache final {
        std::set<Ast_Sen_ବୃକ୍ଷ*> m_processDomains;  // What shall be added to m_processDomains
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
        ଗଣନା State : uint8_t {
            UNINITIALIZED = 0,  // Not initialized members are empty
            VISITING,  // Visiting - needed for breaking recursion
            INITIALIZED,  // Members contains correct values
        } m_state  // Current state of Cache
            = UNINITIALIZED;
    };
    std::vector<ଅଚଳ AstCFunc*> m_call_ଗଦା;  // Current callstack of AstCFuncs
    // Caches how visiting the function with given value of m_gatherVars changes
    // m_processDomains and m_writtenBySuspendable - only accessed from visit(AstCFunc* nodep)
    AstUser1Allocator<AstCFunc, std::array<CFuncCache, 2>> m_cfuncsCache;
    // Count uses of not inlined writes to signals in suspendables
    VDouble0 m_notInlinedWritesInSuspendableUsage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    ଶୂନ୍ୟ addResumePins(AstCMethodHard* ଅଚଳ resumep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* pinsp) {
        AstCExpr* ଅଚଳ exprp = VN_CAST(pinsp, CExpr);
        AstText* ଅଚଳ textp = VN_CAST(exprp->nodesp(), Text);
        ଯଦି (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            ଯଦି ((pinsp = VN_CAST(pinsp->nextp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି))) {
                resumep->addPinsp(pinsp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
            }
        } ଅନ୍ୟଥା {
            resumep->addPinsp(pinsp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
        }
    }
    ଶୂନ୍ୟ add_CFunc_Cached_ମୂଲ୍ୟଗୁଡ଼ିକ(ଅଚଳ AstCFunc* ଅଚଳ cfuncp,
                              std::unordered_set<ଅଚଳ AstCFunc*>& visited) {
        ଯଦି (!visited.ଭର୍ତ୍ତି(cfuncp).second) ଫେରନ୍ତୁ;
        CFuncCache& ମୂଲ୍ୟ = m_cfuncsCache(cfuncp)[static_cast<size_t>(m_gatherVars)];
        ଅଚଳ size_t initialSize = m_writtenBySuspendable.size();
        m_writtenBySuspendable.ଭର୍ତ୍ତି(ମୂଲ୍ୟ.m_writtenBySuspendable.begin(),
                                      ମୂଲ୍ୟ.m_writtenBySuspendable.end());
        m_notInlinedWritesInSuspendableUsage += m_writtenBySuspendable.size() - initialSize;
        m_processDomains.ଭର୍ତ୍ତି(ମୂଲ୍ୟ.m_processDomains.begin(), ମୂଲ୍ୟ.m_processDomains.end());
        ପାଇଁ (ଅଚଳ AstCFunc* ଅଚଳ includedp : ମୂଲ୍ୟ.m_includes) {
            add_CFunc_Cached_ମୂଲ୍ୟଗୁଡ଼ିକ(includedp, visited);
        }
    }
    // Create an active with a timing scheduler resume() call
    ଶୂନ୍ୟ createResumeActive(AstCAwait* ଅଚଳ awaitp) {
        auto* ଅଚଳ methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* ଅଚଳ schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ sentreep = awaitp->sentreep();
        File_ଧାଡ଼ି* ଅଚଳ flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        auto* ଅଚଳ resumep = ନୂତନ AstCMethodHard{
            flp, ନୂତନ AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtypeSetVoid();
        ଯଦି (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ dbginfop = methodp->pinsp()->nextp()) {
                ଯଦି (methodp->pinsp()) addResumePins(resumep, static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(dbginfop));
            }
        } ଅନ୍ୟଥା ଯଦି (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            auto* ଅଚଳ postp = resumep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            postp->method(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = Ast_ଗଣ୍ଠି::addNext(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        AstActive* ଅଚଳ activep = ନୂତନ AstActive{flp, "_timing", sentreep};
        activep->addStmtsp(resumep->makeStmt());
        m_lbs.add(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    ଶୂନ୍ୟ visitCalledCFunc(AstCFunc* ଅଚଳ nodep) {
        ଯଦି (!m_inProcess) ଫେରନ୍ତୁ;  // Skip if not in process - nothing would be added to any set
        ଅଚଳ size_t cache_ଚାବି = static_cast<size_t>(m_gatherVars);
        CFuncCache& ମୂଲ୍ୟ = m_cfuncsCache(nodep)[cache_ଚାବି];
        ବାଛ (ମୂଲ୍ୟ.m_state) {
        ସ୍ଥିତି CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            ମୂଲ୍ୟ.m_state = CFuncCache::VISITING;
            m_call_ଗଦା.ଠେଲ_back(nodep);
            iterateChildren(nodep);
            m_call_ଗଦା.କାଢ଼_back();
            ମୂଲ୍ୟ.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_processDomains, ମୂଲ୍ୟ.m_processDomains);
            std::swap(m_writtenBySuspendable, ମୂଲ୍ୟ.m_writtenBySuspendable);
        } ଭାଙ୍ଗ;
        ସ୍ଥିତି CFuncCache::VISITING: {
            ପାଇଁ (size_t i = m_call_ଗଦା.size() - 1; m_call_ଗଦା.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_ଗଦା[i])[cache_ଚାବି].m_includes.ଭର୍ତ୍ତି(nodep);
            }
            ଫେରନ୍ତୁ;  // Break recursion
        }
        ସ୍ଥିତି CFuncCache::INITIALIZED: ଭାଙ୍ଗ;
        }
        std::unordered_set<ଅଚଳ AstCFunc*> visited;
        add_CFunc_Cached_ମୂଲ୍ୟଗୁଡ଼ିକ(nodep, visited);
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Procedure* ଅଚଳ nodep) override {
        UASSERT_OBJ(!m_inProcess && !m_gatherVars && m_processDomains.empty()
                        && m_writtenBySuspendable.empty(),
                    nodep, "Process in process?");
        m_inProcess = ସତ୍ୟ;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        ପାଇଁ (AstVarScope* ଅଚଳ vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].ଭର୍ତ୍ତି(m_processDomains.begin(), m_processDomains.end());
            vscp->varp()->setWrittenBySuspendable();
        }
        m_processDomains.clear();
        m_writtenBySuspendable.clear();
        m_inProcess = ମିଥ୍ୟା;
        m_gatherVars = ମିଥ୍ୟା;
    }
    ଶୂନ୍ୟ visit(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        ଯଦି (m_inProcess) m_gatherVars = ସତ୍ୟ;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstCAwait* nodep) override {
        ଯଦି (Ast_Sen_ବୃକ୍ଷ* ଅଚଳ sentreep = nodep->sentreep()) {
            ଯଦି (!sentreep->user1SetOnce()) createResumeActive(nodep);
            ଯଦି (m_inProcess) m_processDomains.ଭର୍ତ୍ତି(sentreep);
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        ଯଦି (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.ଭର୍ତ୍ତି(nodep->varScopep());
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_CCall* ଅଚଳ nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ଅଚଳ auto& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        ଯଦି (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } ଅନ୍ୟଥା {
            ପାଇଁ (AstCFunc* ଅଚଳ cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    ଶୂନ୍ୟ visit(AstCFunc* ଅଚଳ nodep) override {
        ଅଚଳ auto& ମୂଲ୍ୟ = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        ଯଦି (ମୂଲ୍ୟ[0].m_state != CFuncCache::UNINITIALIZED
            || ମୂଲ୍ୟ[1].m_state != CFuncCache::UNINITIALIZED) {
            ଫେରନ୍ତୁ;
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ପଦସମଷ୍ଟି_Stmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, Ast_ଗଣ୍ଠି_Stmt*& postUpdatesr,
                          std::map<ଅଚଳ AstVarScope*, std::set<Ast_Sen_ବୃକ୍ଷ*>>& externalDomains)
        : m_scopeTopp{nodep->ଶୀର୍ଷ_Scopep()->scopep()}
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

TimingKit prepareTiming(AstNetlist* ଅଚଳ netlistp) {
    ଯଦି (!v3Global.usesTiming()) ଫେରନ୍ତୁ {};
    LogicByScope lbs;
    Ast_ଗଣ୍ଠି_Stmt* postUpdates = nullptr;
    std::map<ଅଚଳ AstVarScope*, std::set<Ast_Sen_ବୃକ୍ଷ*>> externalDomains;
    { AwaitVisitor{netlistp, lbs, postUpdates, externalDomains}; }
    ଫେରନ୍ତୁ {std::move(lbs), postUpdates, std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

// Transform all forked processes into functions
ଶ୍ରେଣୀ TransformForksVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current fork.
    ଅଚଳ VNUser1InUse m_inuser1;

    // STATE
    ତାର୍କିକ m_inClass = ମିଥ୍ୟା;  // Are we in a class?
    ତାର୍କିକ m_beginHasAwaits = ମିଥ୍ୟା;  // Does the current begin have awaits?
    AstFork* m_forkp = nullptr;  // Current fork
    AstCFunc* m_funcp = nullptr;  // Current function

    // METHODS
    // Remap local vars referenced by the given fork function
    // TODO: We should only pass variables to the fork that are
    // live in the fork body, but for that we need a proper data
    // flow analysis framework which we don't have at the moment
    ଶୂନ୍ୟ remapLocals(AstCFunc* ଅଚଳ funcp, AstCCall* ଅଚଳ callp) {
        ଅଚଳ VNUser2InUse user2InUse;  // AstVarScope -> AstVarScope: var to remap to
        funcp->foreach([&](Ast_ଗଣ୍ଠି_Var_Ref* refp) {
            AstVar* ଅଚଳ varp = refp->varp();
            AstVarScope* ଅଚଳ vscp = refp->varScopep();
            AstBasicDType* ଅଚଳ dtypep = varp->dtypep()->basicp();
            // If not a fork..join, copy. All write refs should've been handled by V3Fork
            ତାର୍କିକ pass_By_ମୂଲ୍ୟ = !m_forkp->joinType().join();
            ଯଦି (vscp->fork_By_ମୂଲ୍ୟ()) {
                // Set just before the fork, for its processes to get by value
                pass_By_ମୂଲ୍ୟ = ସତ୍ୟ;
            } ଅନ୍ୟଥା ଯଦି (!varp->isFuncLocal()) {
                // Not func local. Its lifetime is longer than the forked process. Skip
                ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା ଯଦି (!varp->user1()) {
                // Not declared before the fork. It cannot outlive the forked process
                ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା ଯଦି (dtypep && dtypep->isForkSync()) {
                // We can just pass it by value to the new function
                pass_By_ମୂଲ୍ୟ = ସତ୍ୟ;
            }
            // Remap the reference
            ଯଦି (!vscp->user2p()) {
                // Clone the var to the new function
                AstVar* ଅଚଳ newvarp
                    = ନୂତନ AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                newvarp->funcLocal(ସତ୍ୟ);
                newvarp->direction(pass_By_ମୂଲ୍ୟ ? VDirection::ନିବେଶ : VDirection::REF);
                funcp->addArgsp(newvarp);
                AstVarScope* ଅଚଳ newvscp
                    = ନୂତନ AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->addVarsp(newvscp);
                vscp->user2p(newvscp);
                callp->addArgsp(ନୂତନ AstVarRef{refp->fileline(), vscp,
                                              pass_By_ମୂଲ୍ୟ ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* ଅଚଳ newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_inClass);
        m_inClass = VN_IS(nodep, Class);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        m_funcp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        ଯଦି (!m_forkp) nodep->user1(ସତ୍ୟ);
    }
    ଶୂନ୍ୟ visit(AstFork* nodep) override {
        ଯଦି (m_forkp) ଫେରନ୍ତୁ;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        Ast_ଗଣ୍ଠି* resp = nullptr;
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ declsp = nodep->declsp()) {
            resp = Ast_ଗଣ୍ଠି::addNext(resp, declsp->unlinkFrBackWithNext());
        }
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = nodep->stmtsp()) {
            resp = Ast_ଗଣ୍ଠି::addNext(resp, stmtsp->unlinkFrBackWithNext());
        }
        ଯେତେବେଳେ (AstBegin* ଅଚଳ beginp = nodep->forksp()) {
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ declsp = beginp->declsp()) {
                resp = Ast_ଗଣ୍ଠି::addNext(resp, declsp->unlinkFrBackWithNext());
            }
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = beginp->stmtsp()) {
                resp = Ast_ଗଣ୍ଠି::addNext(resp, stmtsp->unlinkFrBackWithNext());
            }
            VL_DO_DANGLING(ଠେଲ_Deletep(beginp->unlinkFrBack()), beginp);
        }
        ଯଦି (resp) {
            nodep->replaceWith(resp);
        } ଅନ୍ୟଥା {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstBegin* nodep) override {
        UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
        // Start with children, so later we only find awaits that are actually in this begin
        m_beginHasAwaits = ମିଥ୍ୟା;
        iterateChildrenConst(nodep);
        ଯଦି (!nodep->stmtsp()) ଫେରନ୍ତୁ;
        ଯଦି (!m_beginHasAwaits && !nodep->needProcess()) ଫେରନ୍ତୁ;

        UASSERT_OBJ(!nodep->name().empty(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        AstCFunc* ଅଚଳ newfuncp = ନୂତନ AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->addNextHere(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->declPrivate(ସତ୍ୟ);
        // Create the call to the function
        AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{flp, newfuncp};
        callp->dtypeSetVoid();
        // If we're in a class, add a vlSymsp arg
        ଯଦି (m_inClass) {
            newfuncp->addStmtsp(ନୂତନ AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ declsp = nodep->declsp()) {
            newfuncp->addStmtsp(declsp->unlinkFrBackWithNext());
        }
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = nodep->stmtsp()) {
            newfuncp->addStmtsp(stmtsp->unlinkFrBackWithNext());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->addStmtsp(callp->makeStmt());
        // Propagate if needs process
        ଯଦି (nodep->needProcess()) newfuncp->setNeedProcess();
        remapLocals(newfuncp, callp);
    }
    ଶୂନ୍ୟ visit(AstCAwait* nodep) override {
        m_beginHasAwaits = ସତ୍ୟ;
        iterateChildrenConst(nodep);
    }

    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

ଶୂନ୍ୟ transformForks(AstNetlist* ଅଚଳ netlistp) {
    ଯଦି (!v3Global.usesTiming()) ଫେରନ୍ତୁ;
    { TransformForksVisitor{netlistp}; }
    V3Global::dump_Check_Global_ବୃକ୍ଷ("transform_forks", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}

}  // namespace V3Sched
