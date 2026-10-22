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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3AstUserAllocator.h"
#ضمن "V3ClassGraph.h"
#ضمن "V3EmitCBase.h"
#ضمن "V3Sched.h"

#ضمن <unordered_map>
#ضمن <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

نطاق V3Sched {

//============================================================================
// Trigger indices of the external domains of each variable

std::map<ثابت AstVarScope*, std::vector<uint32_t>>
TimingKit::externalTriggerIndices(ثابت TriggerKit& trigKit) ثابت {
    std::map<ثابت AstVarScope*, std::vector<uint32_t>> result;
    لكل (ثابت auto& vscpDomains : m_externalDomains) {
        std::vector<uint32_t>& indices = result[vscpDomains.first];
        لكل (ثابت AstSenTree* ثابت domainp : vscpDomains.second) {
            لكل (ثابت AstSenItem* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                indices.push_back(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    أرجع result;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    𐡄𐡍 (!m_resumeFuncp) {
        𐡄𐡍 (m_lbs.empty()) أرجع nullptr;
        // Create global resume function
        AstNetlist* ثابت netlistp = v3Global.rootp();
        AstScope* ثابت scopeTopp = netlistp->topScopep()->scopep();
        m_resumeFuncp = جديد AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(𐡒𐡔𐡈);
        m_resumeFuncp->isLoose(𐡒𐡔𐡈);
        m_resumeFuncp->isConst(خطأ);
        m_resumeFuncp->declPrivate(𐡒𐡔𐡈);
        scopeTopp->addBlocksp(m_resumeFuncp);

        لكل (ثابت auto& entry : m_lbs) {
            AstActive* ثابت activep = entry.m_activep;
            activep->foreach([هذا](AstCMethodHard* ثابت exprp) {
                𐡄𐡍 (exprp->method() != VCMethod::SCHED_RESUME) أرجع;
                AstNodeExpr* ثابت fromp = exprp->fromp();
                𐡄𐡍 (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    أرجع;
                }
                AstCMethodHard* ثابت moveToResumep = جديد AstCMethodHard{
                    fromp->fileline(), fromp->cloneTree(خطأ),
                    VCMethod::SCHED_MOVE_TO_RESUME_QUEUE,
                    exprp->pinsp() ? exprp->pinsp()->cloneTree(𐡒𐡔𐡈) : nullptr};
                moveToResumep->dtypeSetVoid();
                m_resumeFuncp->addStmtsp(moveToResumep->makeStmt());
            });
        }

        // Put all the timing actives in the resume function
        AstIf* dlyShedIfp = nullptr;
        لكل (ثابت auto& entry : m_lbs) {
            AstActive* ثابت activep = entry.m_activep;
            // Resume time delays last. For no particular reason other than
            // that's what we used to do prior to proper #0 support.
            AstVarRef* ثابت schedrefp = VN_AS(
                VN_AS(VN_AS(activep->stmtsp(), StmtExpr)->exprp(), CMethodHard)->fromp(), VarRef);

            AstNode* ثابت actionp = activep->stmtsp()->unlinkFrBackWithNext();
            𐡄𐡍 (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                FileLine* ثابت flp = trigVscp->fileline();
                ثابت std::set<uint32_t>& triggers = entry.m_triggers;
                AstTriggered* ثابت condp
                    = جديد AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
                dlyShedIfp = جديد AstIf{flp, condp};
                dlyShedIfp->addThensp(actionp);
            } وإلا {
                m_resumeFuncp->addStmtsp(actionp);
            }
        }
        𐡄𐡍 (dlyShedIfp) m_resumeFuncp->addStmtsp(dlyShedIfp);

        // These are now spent, oispose of now empty AstActive instances
        m_lbs.deleteActives();
    }
    AstCCall* ثابت callp = جديد AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
    callp->dtypeSetVoid();
    أرجع callp;
}

AstVarScope* TimingKit::getDelayScheduler(AstNetlist* ثابت /*netlistp*/) {
    لكل (ثابت auto& entry : m_lbs) {
        AstActive* ثابت ap = entry.m_activep;
        // TODO: this triple VN_AS expression is ridiculous
        AstVarRef* ثابت schedrefp
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), StmtExpr)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* ثابت vscp = schedrefp->varScopep();
        𐡄𐡍 (vscp->dtypep()->basicp()->isDelayScheduler()) أرجع vscp;
    }
    // None found. Design doesn't use any time delays
    أرجع nullptr;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    𐡄𐡍 (!m_readyFuncp) {
        لكل (ثابت auto& entry : m_lbs) {
            AstActive* ثابت activep = entry.m_activep;
            auto* ثابت resumep = VN_AS(VN_AS(activep->stmtsp(), StmtExpr)->exprp(), CMethodHard);
            UASSERT_OBJ(!resumep->nextp(), resumep, "Should be the only statement here");
            AstVarScope* ثابت schedulerp = VN_AS(resumep->fromp(), VarRef)->varScopep();
            UASSERT_OBJ(schedulerp->dtypep()->basicp()->isDelayScheduler()
                            || schedulerp->dtypep()->basicp()->isTriggerScheduler()
                            || schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler(),
                        schedulerp, "Unexpected type");
            𐡄𐡍 (!schedulerp->dtypep()->basicp()->isTriggerScheduler()) استمر;
            // Create the global ready function only if we have trigger schedulers
            𐡄𐡍 (!m_readyFuncp) {
                AstNetlist* ثابت netlistp = v3Global.rootp();
                AstScope* ثابت scopeTopp = netlistp->topScopep()->scopep();
                m_readyFuncp = جديد AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(𐡒𐡔𐡈);
                m_readyFuncp->isLoose(𐡒𐡔𐡈);
                m_readyFuncp->isConst(خطأ);
                m_readyFuncp->declPrivate(𐡒𐡔𐡈);
                scopeTopp->addBlocksp(m_readyFuncp);
            }

            FileLine* ثابت flp = activep->sentreep()->fileline();

            // Create an 'AstIf' sensitive to the suspending triggers
            ثابت std::set<uint32_t>& triggers = entry.m_triggers;
            AstTriggered* ثابت condp
                = جديد AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
            AstIf* ثابت ifp = جديد AstIf{flp, condp};
            m_readyFuncp->addStmtsp(ifp);

            // Mark as ready the processes resumed on this sensitivity expression
            AstVarRef* ثابت refp = جديد AstVarRef{flp, schedulerp, VAccess::READWRITE};
            AstCMethodHard* ثابت callp = جديد AstCMethodHard{flp, refp, VCMethod::SCHED_READY};
            callp->dtypeSetVoid();
            𐡄𐡍 (resumep->pinsp()) callp->addPinsp(resumep->pinsp()->cloneTree(خطأ));
            ifp->addThensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        𐡄𐡍 (!m_readyFuncp) أرجع nullptr;
    }
    AstCCall* ثابت callp = جديد AstCCall{m_readyFuncp->fileline(), m_readyFuncp};
    callp->dtypeSetVoid();
    أرجع callp;
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

صنف AwaitVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstSenTree::user1()  -> bool.               Set true if the sentree has been visited.
    //  AstCFunc::user1()    -> AstUser1Allocator.  See alocator below
    ثابت VNUser1InUse m_inuser1;

    // STATE
    منطقي m_inProcess = خطأ;  // Are we in a process?
    منطقي m_gatherVars = خطأ;  // Should we gather vars in m_writtenBySuspendable?
    AstScope* ثابت m_scopeTopp;  // Scope at the top
    LogicByScope& m_lbs;  // Timing resume actives
    AstNodeStmt*& m_postUpdatesr;  // Post updates for the trigger eval function
    // Additional var sensitivities
    std::map<ثابت AstVarScope*, std::set<AstSenTree*>>& m_externalDomains;
    std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::set<AstSenTree*> m_processDomains;  // Sentrees from the current process
    // Variables written by suspendable processes
    std::set<AstVarScope*> m_writtenBySuspendable;
    بنية CFuncCache final {
        std::set<AstSenTree*> m_processDomains;  // What shall be added to m_processDomains
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
        تعداد State : uint8_t {
            UNINITIALIZED = 0,  // Not initialized members are empty
            VISITING,  // Visiting - needed for breaking recursion
            INITIALIZED,  // Members contains correct values
        } m_state  // Current state of Cache
            = UNINITIALIZED;
    };
    std::vector<ثابت AstCFunc*> m_callStack;  // Current callstack of AstCFuncs
    // Caches how visiting the function with given value of m_gatherVars changes
    // m_processDomains and m_writtenBySuspendable - only accessed from visit(AstCFunc* nodep)
    AstUser1Allocator<AstCFunc, std::array<CFuncCache, 2>> m_cfuncsCache;
    // Count uses of not inlined writes to signals in suspendables
    VDouble0 m_notInlinedWritesInSuspendableUsage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    فراغ addResumePins(AstCMethodHard* ثابت resumep, AstNodeExpr* pinsp) {
        AstCExpr* ثابت exprp = VN_CAST(pinsp, CExpr);
        AstText* ثابت textp = VN_CAST(exprp->nodesp(), Text);
        𐡄𐡍 (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            𐡄𐡍 ((pinsp = VN_CAST(pinsp->nextp(), NodeExpr))) {
                resumep->addPinsp(pinsp->cloneTree(خطأ));
            }
        } وإلا {
            resumep->addPinsp(pinsp->cloneTree(خطأ));
        }
    }
    فراغ addCFuncCachedValues(ثابت AstCFunc* ثابت cfuncp,
                              std::unordered_set<ثابت AstCFunc*>& visited) {
        𐡄𐡍 (!visited.insert(cfuncp).second) أرجع;
        CFuncCache& value = m_cfuncsCache(cfuncp)[static_cast<size_t>(m_gatherVars)];
        ثابت size_t initialSize = m_writtenBySuspendable.size();
        m_writtenBySuspendable.insert(value.m_writtenBySuspendable.begin(),
                                      value.m_writtenBySuspendable.end());
        m_notInlinedWritesInSuspendableUsage += m_writtenBySuspendable.size() - initialSize;
        m_processDomains.insert(value.m_processDomains.begin(), value.m_processDomains.end());
        لكل (ثابت AstCFunc* ثابت includedp : value.m_includes) {
            addCFuncCachedValues(includedp, visited);
        }
    }
    // Create an active with a timing scheduler resume() call
    فراغ createResumeActive(AstCAwait* ثابت awaitp) {
        auto* ثابت methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* ثابت schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        AstSenTree* ثابت sentreep = awaitp->sentreep();
        FileLine* ثابت flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        auto* ثابت resumep = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtypeSetVoid();
        𐡄𐡍 (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            𐡄𐡍 (AstNode* ثابت dbginfop = methodp->pinsp()->nextp()) {
                𐡄𐡍 (methodp->pinsp()) addResumePins(resumep, static_cast<AstNodeExpr*>(dbginfop));
            }
        } وإلا 𐡄𐡍 (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            auto* ثابت postp = resumep->cloneTree(خطأ);
            postp->method(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = AstNode::addNext(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        AstActive* ثابت activep = جديد AstActive{flp, "_timing", sentreep};
        activep->addStmtsp(resumep->makeStmt());
        m_lbs.add(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    فراغ visitCalledCFunc(AstCFunc* ثابت nodep) {
        𐡄𐡍 (!m_inProcess) أرجع;  // Skip if not in process - nothing would be added to any set
        ثابت size_t cacheKey = static_cast<size_t>(m_gatherVars);
        CFuncCache& value = m_cfuncsCache(nodep)[cacheKey];
        بدل (value.m_state) {
        حالة CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            value.m_state = CFuncCache::VISITING;
            m_callStack.push_back(nodep);
            iterateChildren(nodep);
            m_callStack.pop_back();
            value.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_processDomains, value.m_processDomains);
            std::swap(m_writtenBySuspendable, value.m_writtenBySuspendable);
        } اكسر;
        حالة CFuncCache::VISITING: {
            لكل (size_t i = m_callStack.size() - 1; m_callStack.at(i) != nodep; --i) {
                m_cfuncsCache(m_callStack[i])[cacheKey].m_includes.insert(nodep);
            }
            أرجع;  // Break recursion
        }
        حالة CFuncCache::INITIALIZED: اكسر;
        }
        std::unordered_set<ثابت AstCFunc*> visited;
        addCFuncCachedValues(nodep, visited);
    }

    // VISITORS
    فراغ visit(AstNodeProcedure* ثابت nodep) override {
        UASSERT_OBJ(!m_inProcess && !m_gatherVars && m_processDomains.empty()
                        && m_writtenBySuspendable.empty(),
                    nodep, "Process in process?");
        m_inProcess = 𐡒𐡔𐡈;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        لكل (AstVarScope* ثابت vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].insert(m_processDomains.begin(), m_processDomains.end());
            vscp->varp()->setWrittenBySuspendable();
        }
        m_processDomains.clear();
        m_writtenBySuspendable.clear();
        m_inProcess = خطأ;
        m_gatherVars = خطأ;
    }
    فراغ visit(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        𐡄𐡍 (m_inProcess) m_gatherVars = 𐡒𐡔𐡈;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    فراغ visit(AstCAwait* nodep) override {
        𐡄𐡍 (AstSenTree* ثابت sentreep = nodep->sentreep()) {
            𐡄𐡍 (!sentreep->user1SetOnce()) createResumeActive(nodep);
            𐡄𐡍 (m_inProcess) m_processDomains.insert(sentreep);
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstNodeVarRef* nodep) override {
        𐡄𐡍 (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.insert(nodep->varScopep());
        }
    }
    فراغ visit(AstNodeCCall* ثابت nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ثابت auto& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        𐡄𐡍 (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } وإلا {
            لكل (AstCFunc* ثابت cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    فراغ visit(AstCFunc* ثابت nodep) override {
        ثابت auto& value = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        𐡄𐡍 (value[0].m_state != CFuncCache::UNINITIALIZED
            || value[1].m_state != CFuncCache::UNINITIALIZED) {
            أرجع;
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstExprStmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, AstNodeStmt*& postUpdatesr,
                          std::map<ثابت AstVarScope*, std::set<AstSenTree*>>& externalDomains)
        : m_scopeTopp{nodep->topScopep()->scopep()}
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

TimingKit prepareTiming(AstNetlist* ثابت netlistp) {
    𐡄𐡍 (!v3Global.usesTiming()) أرجع {};
    LogicByScope lbs;
    AstNodeStmt* postUpdates = nullptr;
    std::map<ثابت AstVarScope*, std::set<AstSenTree*>> externalDomains;
    { AwaitVisitor{netlistp, lbs, postUpdates, externalDomains}; }
    أرجع {std::move(lbs), postUpdates, std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

// Transform all forked processes into functions
صنف TransformForksVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current fork.
    ثابت VNUser1InUse m_inuser1;

    // STATE
    منطقي m_inClass = خطأ;  // Are we in a class?
    منطقي m_beginHasAwaits = خطأ;  // Does the current begin have awaits?
    AstFork* m_forkp = nullptr;  // Current fork
    AstCFunc* m_funcp = nullptr;  // Current function

    // METHODS
    // Remap local vars referenced by the given fork function
    // TODO: We should only pass variables to the fork that are
    // live in the fork body, but for that we need a proper data
    // flow analysis framework which we don't have at the moment
    فراغ remapLocals(AstCFunc* ثابت funcp, AstCCall* ثابت callp) {
        ثابت VNUser2InUse user2InUse;  // AstVarScope -> AstVarScope: var to remap to
        funcp->foreach([&](AstNodeVarRef* refp) {
            AstVar* ثابت varp = refp->varp();
            AstVarScope* ثابت vscp = refp->varScopep();
            AstBasicDType* ثابت dtypep = varp->dtypep()->basicp();
            // If not a fork..join, copy. All write refs should've been handled by V3Fork
            منطقي passByValue = !m_forkp->joinType().join();
            𐡄𐡍 (vscp->forkByValue()) {
                // Set just before the fork, for its processes to get by value
                passByValue = 𐡒𐡔𐡈;
            } وإلا 𐡄𐡍 (!varp->isFuncLocal()) {
                // Not func local. Its lifetime is longer than the forked process. Skip
                أرجع;
            } وإلا 𐡄𐡍 (!varp->user1()) {
                // Not declared before the fork. It cannot outlive the forked process
                أرجع;
            } وإلا 𐡄𐡍 (dtypep && dtypep->isForkSync()) {
                // We can just pass it by value to the new function
                passByValue = 𐡒𐡔𐡈;
            }
            // Remap the reference
            𐡄𐡍 (!vscp->user2p()) {
                // Clone the var to the new function
                AstVar* ثابت newvarp
                    = جديد AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                newvarp->funcLocal(𐡒𐡔𐡈);
                newvarp->direction(passByValue ? VDirection::INPUT : VDirection::REF);
                funcp->addArgsp(newvarp);
                AstVarScope* ثابت newvscp
                    = جديد AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->addVarsp(newvscp);
                vscp->user2p(newvscp);
                callp->addArgsp(جديد AstVarRef{refp->fileline(), vscp,
                                              passByValue ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* ثابت newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    فراغ visit(AstNodeModule* nodep) override {
        VL_RESTORER(m_inClass);
        m_inClass = VN_IS(nodep, Class);
        iterateChildren(nodep);
    }
    فراغ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        m_funcp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstVar* nodep) override {
        𐡄𐡍 (!m_forkp) nodep->user1(𐡒𐡔𐡈);
    }
    فراغ visit(AstFork* nodep) override {
        𐡄𐡍 (m_forkp) أرجع;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        AstNode* resp = nullptr;
        𐡄𐡍 (AstNode* ثابت declsp = nodep->declsp()) {
            resp = AstNode::addNext(resp, declsp->unlinkFrBackWithNext());
        }
        𐡄𐡍 (AstNode* ثابت stmtsp = nodep->stmtsp()) {
            resp = AstNode::addNext(resp, stmtsp->unlinkFrBackWithNext());
        }
        طالما (AstBegin* ثابت beginp = nodep->forksp()) {
            𐡄𐡍 (AstNode* ثابت declsp = beginp->declsp()) {
                resp = AstNode::addNext(resp, declsp->unlinkFrBackWithNext());
            }
            𐡄𐡍 (AstNode* ثابت stmtsp = beginp->stmtsp()) {
                resp = AstNode::addNext(resp, stmtsp->unlinkFrBackWithNext());
            }
            VL_DO_DANGLING(pushDeletep(beginp->unlinkFrBack()), beginp);
        }
        𐡄𐡍 (resp) {
            nodep->replaceWith(resp);
        } وإلا {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstBegin* nodep) override {
        UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
        // Start with children, so later we only find awaits that are actually in this begin
        m_beginHasAwaits = خطأ;
        iterateChildrenConst(nodep);
        𐡄𐡍 (!nodep->stmtsp()) أرجع;
        𐡄𐡍 (!m_beginHasAwaits && !nodep->needProcess()) أرجع;

        UASSERT_OBJ(!nodep->name().empty(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        FileLine* ثابت flp = nodep->fileline();
        AstCFunc* ثابت newfuncp = جديد AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->addNextHere(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->declPrivate(𐡒𐡔𐡈);
        // Create the call to the function
        AstCCall* ثابت callp = جديد AstCCall{flp, newfuncp};
        callp->dtypeSetVoid();
        // If we're in a class, add a vlSymsp arg
        𐡄𐡍 (m_inClass) {
            newfuncp->addStmtsp(جديد AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        𐡄𐡍 (AstNode* ثابت declsp = nodep->declsp()) {
            newfuncp->addStmtsp(declsp->unlinkFrBackWithNext());
        }
        𐡄𐡍 (AstNode* ثابت stmtsp = nodep->stmtsp()) {
            newfuncp->addStmtsp(stmtsp->unlinkFrBackWithNext());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->addStmtsp(callp->makeStmt());
        // Propagate if needs process
        𐡄𐡍 (nodep->needProcess()) newfuncp->setNeedProcess();
        remapLocals(newfuncp, callp);
    }
    فراغ visit(AstCAwait* nodep) override {
        m_beginHasAwaits = 𐡒𐡔𐡈;
        iterateChildrenConst(nodep);
    }

    //--------------------
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = افتراضي;
};

فراغ transformForks(AstNetlist* ثابت netlistp) {
    𐡄𐡍 (!v3Global.usesTiming()) أرجع;
    { TransformForksVisitor{netlistp}; }
    V3Global::dumpCheckGlobalTree("transform_forks", 0, dumpTreeEitherLevel() >= 3);
}

}  // namespace V3Sched
