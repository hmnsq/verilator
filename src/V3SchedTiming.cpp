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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3AstUserAllocator.h"
#അന്തര്ഭാവ "V3ClassGraph.h"
#അന്തര്ഭാവ "V3EmitCBase.h"
#അന്തര്ഭാവ "V3Sched.h"

#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

നാമാകാശ V3Sched {

//============================================================================
// Trigger indices of the external domains of each variable

std::map<സ്ഥിരം AstVarScope*, std::vector<uint32_t>>
TimingKit::externalTriggerIndices(സ്ഥിരം TriggerKit& trigKit) സ്ഥിരം {
    std::map<സ്ഥിരം AstVarScope*, std::vector<uint32_t>> result;
    വേണ്ടി (സ്ഥിരം auto& vscpDomains : m_externalDomains) {
        std::vector<uint32_t>& indices = result[vscpDomains.first];
        വേണ്ടി (സ്ഥിരം Ast_Sen_വൃക്ഷം* സ്ഥിരം domainp : vscpDomains.second) {
            വേണ്ടി (സ്ഥിരം AstSenItem* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                indices.തള്ളുക_back(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    തിരികെ result;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    എങ്കിൽ (!m_resumeFuncp) {
        എങ്കിൽ (m_lbs.empty()) തിരികെ nullptr;
        // Create global resume function
        AstNetlist* സ്ഥിരം netlistp = v3Global.rootp();
        AstScope* സ്ഥിരം scopeTopp = netlistp->മുകൾ_Scopep()->scopep();
        m_resumeFuncp = പുതിയ AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(സത്യം);
        m_resumeFuncp->isLoose(സത്യം);
        m_resumeFuncp->isConst(അസത്യം);
        m_resumeFuncp->declPrivate(സത്യം);
        scopeTopp->addBlocksp(m_resumeFuncp);

        വേണ്ടി (സ്ഥിരം auto& entry : m_lbs) {
            AstActive* സ്ഥിരം activep = entry.m_activep;
            activep->foreach([ഇത്](AstCMethodHard* സ്ഥിരം exprp) {
                എങ്കിൽ (exprp->method() != VCMethod::SCHED_RESUME) തിരികെ;
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = exprp->fromp();
                എങ്കിൽ (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    തിരികെ;
                }
                AstCMethodHard* സ്ഥിരം moveToResumep = പുതിയ AstCMethodHard{
                    fromp->fileline(), fromp->clone_വൃക്ഷം(അസത്യം),
                    VCMethod::SCHED_MOVE_TO_RESUME_QUEUE,
                    exprp->pinsp() ? exprp->pinsp()->clone_വൃക്ഷം(സത്യം) : nullptr};
                moveToResumep->dtypeSetVoid();
                m_resumeFuncp->addStmtsp(moveToResumep->makeStmt());
            });
        }

        // Put all the timing actives in the resume function
        AstIf* dlyShedIfp = nullptr;
        വേണ്ടി (സ്ഥിരം auto& entry : m_lbs) {
            AstActive* സ്ഥിരം activep = entry.m_activep;
            // Resume time delays last. For no particular reason other than
            // that's what we used to do prior to proper #0 support.
            AstVarRef* സ്ഥിരം schedrefp = VN_AS(
                VN_AS(VN_AS(activep->stmtsp(), Stmt_വ്യഞ്ജകം)->exprp(), CMethodHard)->fromp(), VarRef);

            Ast_കണ്ണി* സ്ഥിരം actionp = activep->stmtsp()->unlinkFrBackWithNext();
            എങ്കിൽ (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                File_വരി* സ്ഥിരം flp = trigVscp->fileline();
                സ്ഥിരം std::set<uint32_t>& triggers = entry.m_triggers;
                AstTriggered* സ്ഥിരം condp
                    = പുതിയ AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
                dlyShedIfp = പുതിയ AstIf{flp, condp};
                dlyShedIfp->addThensp(actionp);
            } അല്ലെങ്കിൽ {
                m_resumeFuncp->addStmtsp(actionp);
            }
        }
        എങ്കിൽ (dlyShedIfp) m_resumeFuncp->addStmtsp(dlyShedIfp);

        // These are now spent, oispose of now empty AstActive instances
        m_lbs.deleteActives();
    }
    AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
    callp->dtypeSetVoid();
    തിരികെ callp;
}

AstVarScope* TimingKit::getDelayScheduler(AstNetlist* സ്ഥിരം /*netlistp*/) {
    വേണ്ടി (സ്ഥിരം auto& entry : m_lbs) {
        AstActive* സ്ഥിരം ap = entry.m_activep;
        // TODO: this triple VN_AS expression is ridiculous
        AstVarRef* സ്ഥിരം schedrefp
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), Stmt_വ്യഞ്ജകം)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* സ്ഥിരം vscp = schedrefp->varScopep();
        എങ്കിൽ (vscp->dtypep()->basicp()->isDelayScheduler()) തിരികെ vscp;
    }
    // None found. Design doesn't use any time delays
    തിരികെ nullptr;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    എങ്കിൽ (!m_readyFuncp) {
        വേണ്ടി (സ്ഥിരം auto& entry : m_lbs) {
            AstActive* സ്ഥിരം activep = entry.m_activep;
            auto* സ്ഥിരം resumep = VN_AS(VN_AS(activep->stmtsp(), Stmt_വ്യഞ്ജകം)->exprp(), CMethodHard);
            UASSERT_OBJ(!resumep->nextp(), resumep, "Should be the only statement here");
            AstVarScope* സ്ഥിരം schedulerp = VN_AS(resumep->fromp(), VarRef)->varScopep();
            UASSERT_OBJ(schedulerp->dtypep()->basicp()->isDelayScheduler()
                            || schedulerp->dtypep()->basicp()->isTriggerScheduler()
                            || schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler(),
                        schedulerp, "Unexpected type");
            എങ്കിൽ (!schedulerp->dtypep()->basicp()->isTriggerScheduler()) തുടരുക;
            // Create the global ready function only if we have trigger schedulers
            എങ്കിൽ (!m_readyFuncp) {
                AstNetlist* സ്ഥിരം netlistp = v3Global.rootp();
                AstScope* സ്ഥിരം scopeTopp = netlistp->മുകൾ_Scopep()->scopep();
                m_readyFuncp = പുതിയ AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(സത്യം);
                m_readyFuncp->isLoose(സത്യം);
                m_readyFuncp->isConst(അസത്യം);
                m_readyFuncp->declPrivate(സത്യം);
                scopeTopp->addBlocksp(m_readyFuncp);
            }

            File_വരി* സ്ഥിരം flp = activep->sentreep()->fileline();

            // Create an 'AstIf' sensitive to the suspending triggers
            സ്ഥിരം std::set<uint32_t>& triggers = entry.m_triggers;
            AstTriggered* സ്ഥിരം condp
                = പുതിയ AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
            AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, condp};
            m_readyFuncp->addStmtsp(ifp);

            // Mark as ready the processes resumed on this sensitivity expression
            AstVarRef* സ്ഥിരം refp = പുതിയ AstVarRef{flp, schedulerp, VAccess::READWRITE};
            AstCMethodHard* സ്ഥിരം callp = പുതിയ AstCMethodHard{flp, refp, VCMethod::SCHED_READY};
            callp->dtypeSetVoid();
            എങ്കിൽ (resumep->pinsp()) callp->addPinsp(resumep->pinsp()->clone_വൃക്ഷം(അസത്യം));
            ifp->addThensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        എങ്കിൽ (!m_readyFuncp) തിരികെ nullptr;
    }
    AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{m_readyFuncp->fileline(), m_readyFuncp};
    callp->dtypeSetVoid();
    തിരികെ callp;
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

വർഗ്ഗം AwaitVisitor final : പൊതു VNVisitor {
    // NODE STATE
    //  AstSenTree::user1()  -> bool.               Set true if the sentree has been visited.
    //  AstCFunc::user1()    -> AstUser1Allocator.  See alocator below
    സ്ഥിരം VNUser1InUse m_inuser1;

    // STATE
    തർക്കം m_inProcess = അസത്യം;  // Are we in a process?
    തർക്കം m_gatherVars = അസത്യം;  // Should we gather vars in m_writtenBySuspendable?
    AstScope* സ്ഥിരം m_scopeTopp;  // Scope at the top
    LogicByScope& m_lbs;  // Timing resume actives
    Ast_കണ്ണി_Stmt*& m_postUpdatesr;  // Post updates for the trigger eval function
    // Additional var sensitivities
    std::map<സ്ഥിരം AstVarScope*, std::set<Ast_Sen_വൃക്ഷം*>>& m_externalDomains;
    std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::set<Ast_Sen_വൃക്ഷം*> m_processDomains;  // Sentrees from the current process
    // Variables written by suspendable processes
    std::set<AstVarScope*> m_writtenBySuspendable;
    ഘടന CFuncCache final {
        std::set<Ast_Sen_വൃക്ഷം*> m_processDomains;  // What shall be added to m_processDomains
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
        എണ്ണൽ State : uint8_t {
            UNINITIALIZED = 0,  // Not initialized members are empty
            VISITING,  // Visiting - needed for breaking recursion
            INITIALIZED,  // Members contains correct values
        } m_state  // Current state of Cache
            = UNINITIALIZED;
    };
    std::vector<സ്ഥിരം AstCFunc*> m_call_അട്ടി;  // Current callstack of AstCFuncs
    // Caches how visiting the function with given value of m_gatherVars changes
    // m_processDomains and m_writtenBySuspendable - only accessed from visit(AstCFunc* nodep)
    AstUser1Allocator<AstCFunc, std::array<CFuncCache, 2>> m_cfuncsCache;
    // Count uses of not inlined writes to signals in suspendables
    VDouble0 m_notInlinedWritesInSuspendableUsage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    ശൂന്യം addResumePins(AstCMethodHard* സ്ഥിരം resumep, Ast_കണ്ണി_വ്യഞ്ജകം* pinsp) {
        AstCExpr* സ്ഥിരം exprp = VN_CAST(pinsp, CExpr);
        AstText* സ്ഥിരം textp = VN_CAST(exprp->nodesp(), Text);
        എങ്കിൽ (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            എങ്കിൽ ((pinsp = VN_CAST(pinsp->nextp(), കണ്ണി_വ്യഞ്ജകം))) {
                resumep->addPinsp(pinsp->clone_വൃക്ഷം(അസത്യം));
            }
        } അല്ലെങ്കിൽ {
            resumep->addPinsp(pinsp->clone_വൃക്ഷം(അസത്യം));
        }
    }
    ശൂന്യം add_CFunc_Cached_മൂല്യങ്ങൾ(സ്ഥിരം AstCFunc* സ്ഥിരം cfuncp,
                              std::unordered_set<സ്ഥിരം AstCFunc*>& visited) {
        എങ്കിൽ (!visited.തിരുകുക(cfuncp).second) തിരികെ;
        CFuncCache& മൂല്യം = m_cfuncsCache(cfuncp)[static_cast<size_t>(m_gatherVars)];
        സ്ഥിരം size_t initialSize = m_writtenBySuspendable.size();
        m_writtenBySuspendable.തിരുകുക(മൂല്യം.m_writtenBySuspendable.begin(),
                                      മൂല്യം.m_writtenBySuspendable.end());
        m_notInlinedWritesInSuspendableUsage += m_writtenBySuspendable.size() - initialSize;
        m_processDomains.തിരുകുക(മൂല്യം.m_processDomains.begin(), മൂല്യം.m_processDomains.end());
        വേണ്ടി (സ്ഥിരം AstCFunc* സ്ഥിരം includedp : മൂല്യം.m_includes) {
            add_CFunc_Cached_മൂല്യങ്ങൾ(includedp, visited);
        }
    }
    // Create an active with a timing scheduler resume() call
    ശൂന്യം createResumeActive(AstCAwait* സ്ഥിരം awaitp) {
        auto* സ്ഥിരം methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* സ്ഥിരം schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        Ast_Sen_വൃക്ഷം* സ്ഥിരം sentreep = awaitp->sentreep();
        File_വരി* സ്ഥിരം flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        auto* സ്ഥിരം resumep = പുതിയ AstCMethodHard{
            flp, പുതിയ AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtypeSetVoid();
        എങ്കിൽ (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം dbginfop = methodp->pinsp()->nextp()) {
                എങ്കിൽ (methodp->pinsp()) addResumePins(resumep, static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(dbginfop));
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            auto* സ്ഥിരം postp = resumep->clone_വൃക്ഷം(അസത്യം);
            postp->method(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = Ast_കണ്ണി::addNext(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        AstActive* സ്ഥിരം activep = പുതിയ AstActive{flp, "_timing", sentreep};
        activep->addStmtsp(resumep->makeStmt());
        m_lbs.add(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    ശൂന്യം visitCalledCFunc(AstCFunc* സ്ഥിരം nodep) {
        എങ്കിൽ (!m_inProcess) തിരികെ;  // Skip if not in process - nothing would be added to any set
        സ്ഥിരം size_t cache_താക്കോൽ = static_cast<size_t>(m_gatherVars);
        CFuncCache& മൂല്യം = m_cfuncsCache(nodep)[cache_താക്കോൽ];
        തിരഞ്ഞെടുപ്പ് (മൂല്യം.m_state) {
        അവസ്ഥ CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            മൂല്യം.m_state = CFuncCache::VISITING;
            m_call_അട്ടി.തള്ളുക_back(nodep);
            iterateChildren(nodep);
            m_call_അട്ടി.എടുക്കുക_back();
            മൂല്യം.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_processDomains, മൂല്യം.m_processDomains);
            std::swap(m_writtenBySuspendable, മൂല്യം.m_writtenBySuspendable);
        } നിർത്തുക;
        അവസ്ഥ CFuncCache::VISITING: {
            വേണ്ടി (size_t i = m_call_അട്ടി.size() - 1; m_call_അട്ടി.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_അട്ടി[i])[cache_താക്കോൽ].m_includes.തിരുകുക(nodep);
            }
            തിരികെ;  // Break recursion
        }
        അവസ്ഥ CFuncCache::INITIALIZED: നിർത്തുക;
        }
        std::unordered_set<സ്ഥിരം AstCFunc*> visited;
        add_CFunc_Cached_മൂല്യങ്ങൾ(nodep, visited);
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Procedure* സ്ഥിരം nodep) override {
        UASSERT_OBJ(!m_inProcess && !m_gatherVars && m_processDomains.empty()
                        && m_writtenBySuspendable.empty(),
                    nodep, "Process in process?");
        m_inProcess = സത്യം;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        വേണ്ടി (AstVarScope* സ്ഥിരം vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].തിരുകുക(m_processDomains.begin(), m_processDomains.end());
            vscp->varp()->setWrittenBySuspendable();
        }
        m_processDomains.clear();
        m_writtenBySuspendable.clear();
        m_inProcess = അസത്യം;
        m_gatherVars = അസത്യം;
    }
    ശൂന്യം visit(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        എങ്കിൽ (m_inProcess) m_gatherVars = സത്യം;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstCAwait* nodep) override {
        എങ്കിൽ (Ast_Sen_വൃക്ഷം* സ്ഥിരം sentreep = nodep->sentreep()) {
            എങ്കിൽ (!sentreep->user1SetOnce()) createResumeActive(nodep);
            എങ്കിൽ (m_inProcess) m_processDomains.തിരുകുക(sentreep);
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        എങ്കിൽ (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.തിരുകുക(nodep->varScopep());
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_CCall* സ്ഥിരം nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        സ്ഥിരം auto& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        എങ്കിൽ (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } അല്ലെങ്കിൽ {
            വേണ്ടി (AstCFunc* സ്ഥിരം cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    ശൂന്യം visit(AstCFunc* സ്ഥിരം nodep) override {
        സ്ഥിരം auto& മൂല്യം = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        എങ്കിൽ (മൂല്യം[0].m_state != CFuncCache::UNINITIALIZED
            || മൂല്യം[1].m_state != CFuncCache::UNINITIALIZED) {
            തിരികെ;
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_വ്യഞ്ജകം_Stmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, Ast_കണ്ണി_Stmt*& postUpdatesr,
                          std::map<സ്ഥിരം AstVarScope*, std::set<Ast_Sen_വൃക്ഷം*>>& externalDomains)
        : m_scopeTopp{nodep->മുകൾ_Scopep()->scopep()}
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

TimingKit prepareTiming(AstNetlist* സ്ഥിരം netlistp) {
    എങ്കിൽ (!v3Global.usesTiming()) തിരികെ {};
    LogicByScope lbs;
    Ast_കണ്ണി_Stmt* postUpdates = nullptr;
    std::map<സ്ഥിരം AstVarScope*, std::set<Ast_Sen_വൃക്ഷം*>> externalDomains;
    { AwaitVisitor{netlistp, lbs, postUpdates, externalDomains}; }
    തിരികെ {std::move(lbs), postUpdates, std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

// Transform all forked processes into functions
വർഗ്ഗം TransformForksVisitor final : പൊതു VNVisitor {
    // NODE STATE
    //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current fork.
    സ്ഥിരം VNUser1InUse m_inuser1;

    // STATE
    തർക്കം m_inClass = അസത്യം;  // Are we in a class?
    തർക്കം m_beginHasAwaits = അസത്യം;  // Does the current begin have awaits?
    AstFork* m_forkp = nullptr;  // Current fork
    AstCFunc* m_funcp = nullptr;  // Current function

    // METHODS
    // Remap local vars referenced by the given fork function
    // TODO: We should only pass variables to the fork that are
    // live in the fork body, but for that we need a proper data
    // flow analysis framework which we don't have at the moment
    ശൂന്യം remapLocals(AstCFunc* സ്ഥിരം funcp, AstCCall* സ്ഥിരം callp) {
        സ്ഥിരം VNUser2InUse user2InUse;  // AstVarScope -> AstVarScope: var to remap to
        funcp->foreach([&](Ast_കണ്ണി_Var_Ref* refp) {
            AstVar* സ്ഥിരം varp = refp->varp();
            AstVarScope* സ്ഥിരം vscp = refp->varScopep();
            AstBasicDType* സ്ഥിരം dtypep = varp->dtypep()->basicp();
            // If not a fork..join, copy. All write refs should've been handled by V3Fork
            തർക്കം pass_By_മൂല്യം = !m_forkp->joinType().join();
            എങ്കിൽ (vscp->fork_By_മൂല്യം()) {
                // Set just before the fork, for its processes to get by value
                pass_By_മൂല്യം = സത്യം;
            } അല്ലെങ്കിൽ എങ്കിൽ (!varp->isFuncLocal()) {
                // Not func local. Its lifetime is longer than the forked process. Skip
                തിരികെ;
            } അല്ലെങ്കിൽ എങ്കിൽ (!varp->user1()) {
                // Not declared before the fork. It cannot outlive the forked process
                തിരികെ;
            } അല്ലെങ്കിൽ എങ്കിൽ (dtypep && dtypep->isForkSync()) {
                // We can just pass it by value to the new function
                pass_By_മൂല്യം = സത്യം;
            }
            // Remap the reference
            എങ്കിൽ (!vscp->user2p()) {
                // Clone the var to the new function
                AstVar* സ്ഥിരം newvarp
                    = പുതിയ AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                newvarp->funcLocal(സത്യം);
                newvarp->direction(pass_By_മൂല്യം ? VDirection::നിവേശം : VDirection::REF);
                funcp->addArgsp(newvarp);
                AstVarScope* സ്ഥിരം newvscp
                    = പുതിയ AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->addVarsp(newvscp);
                vscp->user2p(newvscp);
                callp->addArgsp(പുതിയ AstVarRef{refp->fileline(), vscp,
                                              pass_By_മൂല്യം ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* സ്ഥിരം newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_inClass);
        m_inClass = VN_IS(nodep, Class);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        m_funcp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstVar* nodep) override {
        എങ്കിൽ (!m_forkp) nodep->user1(സത്യം);
    }
    ശൂന്യം visit(AstFork* nodep) override {
        എങ്കിൽ (m_forkp) തിരികെ;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        Ast_കണ്ണി* resp = nullptr;
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം declsp = nodep->declsp()) {
            resp = Ast_കണ്ണി::addNext(resp, declsp->unlinkFrBackWithNext());
        }
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം stmtsp = nodep->stmtsp()) {
            resp = Ast_കണ്ണി::addNext(resp, stmtsp->unlinkFrBackWithNext());
        }
        ആകുമ്പോൾ (AstBegin* സ്ഥിരം beginp = nodep->forksp()) {
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം declsp = beginp->declsp()) {
                resp = Ast_കണ്ണി::addNext(resp, declsp->unlinkFrBackWithNext());
            }
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം stmtsp = beginp->stmtsp()) {
                resp = Ast_കണ്ണി::addNext(resp, stmtsp->unlinkFrBackWithNext());
            }
            VL_DO_DANGLING(തള്ളുക_Deletep(beginp->unlinkFrBack()), beginp);
        }
        എങ്കിൽ (resp) {
            nodep->replaceWith(resp);
        } അല്ലെങ്കിൽ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstBegin* nodep) override {
        UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
        // Start with children, so later we only find awaits that are actually in this begin
        m_beginHasAwaits = അസത്യം;
        iterateChildrenConst(nodep);
        എങ്കിൽ (!nodep->stmtsp()) തിരികെ;
        എങ്കിൽ (!m_beginHasAwaits && !nodep->needProcess()) തിരികെ;

        UASSERT_OBJ(!nodep->name().empty(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        AstCFunc* സ്ഥിരം newfuncp = പുതിയ AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->addNextHere(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->declPrivate(സത്യം);
        // Create the call to the function
        AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{flp, newfuncp};
        callp->dtypeSetVoid();
        // If we're in a class, add a vlSymsp arg
        എങ്കിൽ (m_inClass) {
            newfuncp->addStmtsp(പുതിയ AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം declsp = nodep->declsp()) {
            newfuncp->addStmtsp(declsp->unlinkFrBackWithNext());
        }
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം stmtsp = nodep->stmtsp()) {
            newfuncp->addStmtsp(stmtsp->unlinkFrBackWithNext());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->addStmtsp(callp->makeStmt());
        // Propagate if needs process
        എങ്കിൽ (nodep->needProcess()) newfuncp->setNeedProcess();
        remapLocals(newfuncp, callp);
    }
    ശൂന്യം visit(AstCAwait* nodep) override {
        m_beginHasAwaits = സത്യം;
        iterateChildrenConst(nodep);
    }

    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = സ്ഥിരസ്ഥിതി;
};

ശൂന്യം transformForks(AstNetlist* സ്ഥിരം netlistp) {
    എങ്കിൽ (!v3Global.usesTiming()) തിരികെ;
    { TransformForksVisitor{netlistp}; }
    V3Global::dump_Check_Global_വൃക്ഷം("transform_forks", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}

}  // namespace V3Sched
