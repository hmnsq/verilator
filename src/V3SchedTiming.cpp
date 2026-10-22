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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3AstUserAllocator.h"
#অন্তর্ভাৱ "V3ClassGraph.h"
#অন্তর্ভাৱ "V3EmitCBase.h"
#অন্তর্ভাৱ "V3Sched.h"

#অন্তর্ভাৱ <unordered_map>
#অন্তর্ভাৱ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ V3Sched {

//============================================================================
// Trigger indices of the external domains of each variable

std::map<ধ্রুৱ AstVarScope*, std::vector<uint32_t>>
TimingKit::externalTriggerIndices(ধ্রুৱ TriggerKit& trigKit) ধ্রুৱ {
    std::map<ধ্রুৱ AstVarScope*, std::vector<uint32_t>> result;
    বাবে (ধ্রুৱ auto& vscpDomains : m_externalDomains) {
        std::vector<uint32_t>& indices = result[vscpDomains.first];
        বাবে (ধ্রুৱ Ast_Sen_বৃক্ষ* ধ্রুৱ domainp : vscpDomains.second) {
            বাবে (ধ্রুৱ AstSenItem* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                indices.ঠেলো_back(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    ঘূৰাই result;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    যদি (!m_resumeFuncp) {
        যদি (m_lbs.empty()) ঘূৰাই nullptr;
        // Create global resume function
        AstNetlist* ধ্রুৱ netlistp = v3Global.rootp();
        AstScope* ধ্রুৱ scopeTopp = netlistp->শীৰ্ষ_Scopep()->scopep();
        m_resumeFuncp = নূতন AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(সঁচা);
        m_resumeFuncp->isLoose(সঁচা);
        m_resumeFuncp->isConst(মিছা);
        m_resumeFuncp->declPrivate(সঁচা);
        scopeTopp->addBlocksp(m_resumeFuncp);

        বাবে (ধ্রুৱ auto& entry : m_lbs) {
            AstActive* ধ্রুৱ activep = entry.m_activep;
            activep->foreach([এতত্](AstCMethodHard* ধ্রুৱ exprp) {
                যদি (exprp->method() != VCMethod::SCHED_RESUME) ঘূৰাই;
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = exprp->fromp();
                যদি (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    ঘূৰাই;
                }
                AstCMethodHard* ধ্রুৱ moveToResumep = নূতন AstCMethodHard{
                    fromp->fileline(), fromp->clone_বৃক্ষ(মিছা),
                    VCMethod::SCHED_MOVE_TO_RESUME_QUEUE,
                    exprp->pinsp() ? exprp->pinsp()->clone_বৃক্ষ(সঁচা) : nullptr};
                moveToResumep->dtypeSetVoid();
                m_resumeFuncp->addStmtsp(moveToResumep->makeStmt());
            });
        }

        // Put all the timing actives in the resume function
        AstIf* dlyShedIfp = nullptr;
        বাবে (ধ্রুৱ auto& entry : m_lbs) {
            AstActive* ধ্রুৱ activep = entry.m_activep;
            // Resume time delays last. For no particular reason other than
            // that's what we used to do prior to proper #0 support.
            AstVarRef* ধ্রুৱ schedrefp = VN_AS(
                VN_AS(VN_AS(activep->stmtsp(), Stmt_ৰাশিমালা)->exprp(), CMethodHard)->fromp(), VarRef);

            Ast_গ্ৰন্থি* ধ্রুৱ actionp = activep->stmtsp()->unlinkFrBackWithNext();
            যদি (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                File_সাৰি* ধ্রুৱ flp = trigVscp->fileline();
                ধ্রুৱ std::set<uint32_t>& triggers = entry.m_triggers;
                AstTriggered* ধ্রুৱ condp
                    = নূতন AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
                dlyShedIfp = নূতন AstIf{flp, condp};
                dlyShedIfp->addThensp(actionp);
            } অন্যথা {
                m_resumeFuncp->addStmtsp(actionp);
            }
        }
        যদি (dlyShedIfp) m_resumeFuncp->addStmtsp(dlyShedIfp);

        // These are now spent, oispose of now empty AstActive instances
        m_lbs.deleteActives();
    }
    AstCCall* ধ্রুৱ callp = নূতন AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
    callp->dtypeSetVoid();
    ঘূৰাই callp;
}

AstVarScope* TimingKit::getDelayScheduler(AstNetlist* ধ্রুৱ /*netlistp*/) {
    বাবে (ধ্রুৱ auto& entry : m_lbs) {
        AstActive* ধ্রুৱ ap = entry.m_activep;
        // TODO: this triple VN_AS expression is ridiculous
        AstVarRef* ধ্রুৱ schedrefp
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), Stmt_ৰাশিমালা)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* ধ্রুৱ vscp = schedrefp->varScopep();
        যদি (vscp->dtypep()->basicp()->isDelayScheduler()) ঘূৰাই vscp;
    }
    // None found. Design doesn't use any time delays
    ঘূৰাই nullptr;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    যদি (!m_readyFuncp) {
        বাবে (ধ্রুৱ auto& entry : m_lbs) {
            AstActive* ধ্রুৱ activep = entry.m_activep;
            auto* ধ্রুৱ resumep = VN_AS(VN_AS(activep->stmtsp(), Stmt_ৰাশিমালা)->exprp(), CMethodHard);
            UASSERT_OBJ(!resumep->nextp(), resumep, "Should be the only statement here");
            AstVarScope* ধ্রুৱ schedulerp = VN_AS(resumep->fromp(), VarRef)->varScopep();
            UASSERT_OBJ(schedulerp->dtypep()->basicp()->isDelayScheduler()
                            || schedulerp->dtypep()->basicp()->isTriggerScheduler()
                            || schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler(),
                        schedulerp, "Unexpected type");
            যদি (!schedulerp->dtypep()->basicp()->isTriggerScheduler()) অনুৱর্ত;
            // Create the global ready function only if we have trigger schedulers
            যদি (!m_readyFuncp) {
                AstNetlist* ধ্রুৱ netlistp = v3Global.rootp();
                AstScope* ধ্রুৱ scopeTopp = netlistp->শীৰ্ষ_Scopep()->scopep();
                m_readyFuncp = নূতন AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(সঁচা);
                m_readyFuncp->isLoose(সঁচা);
                m_readyFuncp->isConst(মিছা);
                m_readyFuncp->declPrivate(সঁচা);
                scopeTopp->addBlocksp(m_readyFuncp);
            }

            File_সাৰি* ধ্রুৱ flp = activep->sentreep()->fileline();

            // Create an 'AstIf' sensitive to the suspending triggers
            ধ্রুৱ std::set<uint32_t>& triggers = entry.m_triggers;
            AstTriggered* ধ্রুৱ condp
                = নূতন AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
            AstIf* ধ্রুৱ ifp = নূতন AstIf{flp, condp};
            m_readyFuncp->addStmtsp(ifp);

            // Mark as ready the processes resumed on this sensitivity expression
            AstVarRef* ধ্রুৱ refp = নূতন AstVarRef{flp, schedulerp, VAccess::READWRITE};
            AstCMethodHard* ধ্রুৱ callp = নূতন AstCMethodHard{flp, refp, VCMethod::SCHED_READY};
            callp->dtypeSetVoid();
            যদি (resumep->pinsp()) callp->addPinsp(resumep->pinsp()->clone_বৃক্ষ(মিছা));
            ifp->addThensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        যদি (!m_readyFuncp) ঘূৰাই nullptr;
    }
    AstCCall* ধ্রুৱ callp = নূতন AstCCall{m_readyFuncp->fileline(), m_readyFuncp};
    callp->dtypeSetVoid();
    ঘূৰাই callp;
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

শ্ৰেণী AwaitVisitor final : সার্ৱজনিক VNVisitor {
    // NODE STATE
    //  AstSenTree::user1()  -> bool.               Set true if the sentree has been visited.
    //  AstCFunc::user1()    -> AstUser1Allocator.  See alocator below
    ধ্রুৱ VNUser1InUse m_inuser1;

    // STATE
    তার্কিক m_inProcess = মিছা;  // Are we in a process?
    তার্কিক m_gatherVars = মিছা;  // Should we gather vars in m_writtenBySuspendable?
    AstScope* ধ্রুৱ m_scopeTopp;  // Scope at the top
    LogicByScope& m_lbs;  // Timing resume actives
    Ast_গ্ৰন্থি_Stmt*& m_postUpdatesr;  // Post updates for the trigger eval function
    // Additional var sensitivities
    std::map<ধ্রুৱ AstVarScope*, std::set<Ast_Sen_বৃক্ষ*>>& m_externalDomains;
    std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::set<Ast_Sen_বৃক্ষ*> m_processDomains;  // Sentrees from the current process
    // Variables written by suspendable processes
    std::set<AstVarScope*> m_writtenBySuspendable;
    সংরচনা CFuncCache final {
        std::set<Ast_Sen_বৃক্ষ*> m_processDomains;  // What shall be added to m_processDomains
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
        গণনা State : uint8_t {
            UNINITIALIZED = 0,  // Not initialized members are empty
            VISITING,  // Visiting - needed for breaking recursion
            INITIALIZED,  // Members contains correct values
        } m_state  // Current state of Cache
            = UNINITIALIZED;
    };
    std::vector<ধ্রুৱ AstCFunc*> m_call_স্তূপ;  // Current callstack of AstCFuncs
    // Caches how visiting the function with given value of m_gatherVars changes
    // m_processDomains and m_writtenBySuspendable - only accessed from visit(AstCFunc* nodep)
    AstUser1Allocator<AstCFunc, std::array<CFuncCache, 2>> m_cfuncsCache;
    // Count uses of not inlined writes to signals in suspendables
    VDouble0 m_notInlinedWritesInSuspendableUsage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    শূন্য addResumePins(AstCMethodHard* ধ্রুৱ resumep, Ast_গ্ৰন্থি_ৰাশিমালা* pinsp) {
        AstCExpr* ধ্রুৱ exprp = VN_CAST(pinsp, CExpr);
        AstText* ধ্রুৱ textp = VN_CAST(exprp->nodesp(), Text);
        যদি (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            যদি ((pinsp = VN_CAST(pinsp->nextp(), গ্ৰন্থি_ৰাশিমালা))) {
                resumep->addPinsp(pinsp->clone_বৃক্ষ(মিছা));
            }
        } অন্যথা {
            resumep->addPinsp(pinsp->clone_বৃক্ষ(মিছা));
        }
    }
    শূন্য add_CFunc_Cached_মানসমূহ(ধ্রুৱ AstCFunc* ধ্রুৱ cfuncp,
                              std::unordered_set<ধ্রুৱ AstCFunc*>& visited) {
        যদি (!visited.ঢোকাও(cfuncp).second) ঘূৰাই;
        CFuncCache& মান = m_cfuncsCache(cfuncp)[static_cast<size_t>(m_gatherVars)];
        ধ্রুৱ size_t initialSize = m_writtenBySuspendable.size();
        m_writtenBySuspendable.ঢোকাও(মান.m_writtenBySuspendable.begin(),
                                      মান.m_writtenBySuspendable.end());
        m_notInlinedWritesInSuspendableUsage += m_writtenBySuspendable.size() - initialSize;
        m_processDomains.ঢোকাও(মান.m_processDomains.begin(), মান.m_processDomains.end());
        বাবে (ধ্রুৱ AstCFunc* ধ্রুৱ includedp : মান.m_includes) {
            add_CFunc_Cached_মানসমূহ(includedp, visited);
        }
    }
    // Create an active with a timing scheduler resume() call
    শূন্য createResumeActive(AstCAwait* ধ্রুৱ awaitp) {
        auto* ধ্রুৱ methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* ধ্রুৱ schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        Ast_Sen_বৃক্ষ* ধ্রুৱ sentreep = awaitp->sentreep();
        File_সাৰি* ধ্রুৱ flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        auto* ধ্রুৱ resumep = নূতন AstCMethodHard{
            flp, নূতন AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtypeSetVoid();
        যদি (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            যদি (Ast_গ্ৰন্থি* ধ্রুৱ dbginfop = methodp->pinsp()->nextp()) {
                যদি (methodp->pinsp()) addResumePins(resumep, static_cast<Ast_গ্ৰন্থি_ৰাশিমালা*>(dbginfop));
            }
        } অন্যথা যদি (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            auto* ধ্রুৱ postp = resumep->clone_বৃক্ষ(মিছা);
            postp->method(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = Ast_গ্ৰন্থি::addNext(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        AstActive* ধ্রুৱ activep = নূতন AstActive{flp, "_timing", sentreep};
        activep->addStmtsp(resumep->makeStmt());
        m_lbs.add(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    শূন্য visitCalledCFunc(AstCFunc* ধ্রুৱ nodep) {
        যদি (!m_inProcess) ঘূৰাই;  // Skip if not in process - nothing would be added to any set
        ধ্রুৱ size_t cache_চাবি = static_cast<size_t>(m_gatherVars);
        CFuncCache& মান = m_cfuncsCache(nodep)[cache_চাবি];
        ৱিকল্প (মান.m_state) {
        পক্ষ CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            মান.m_state = CFuncCache::VISITING;
            m_call_স্তূপ.ঠেলো_back(nodep);
            iterateChildren(nodep);
            m_call_স্তূপ.বেৰ_কৰো_back();
            মান.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_processDomains, মান.m_processDomains);
            std::swap(m_writtenBySuspendable, মান.m_writtenBySuspendable);
        } ভঙ্গ;
        পক্ষ CFuncCache::VISITING: {
            বাবে (size_t i = m_call_স্তূপ.size() - 1; m_call_স্তূপ.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_স্তূপ[i])[cache_চাবি].m_includes.ঢোকাও(nodep);
            }
            ঘূৰাই;  // Break recursion
        }
        পক্ষ CFuncCache::INITIALIZED: ভঙ্গ;
        }
        std::unordered_set<ধ্রুৱ AstCFunc*> visited;
        add_CFunc_Cached_মানসমূহ(nodep, visited);
    }

    // VISITORS
    শূন্য visit(Ast_গ্ৰন্থি_Procedure* ধ্রুৱ nodep) override {
        UASSERT_OBJ(!m_inProcess && !m_gatherVars && m_processDomains.empty()
                        && m_writtenBySuspendable.empty(),
                    nodep, "Process in process?");
        m_inProcess = সঁচা;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        বাবে (AstVarScope* ধ্রুৱ vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].ঢোকাও(m_processDomains.begin(), m_processDomains.end());
            vscp->varp()->setWrittenBySuspendable();
        }
        m_processDomains.clear();
        m_writtenBySuspendable.clear();
        m_inProcess = মিছা;
        m_gatherVars = মিছা;
    }
    শূন্য visit(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        যদি (m_inProcess) m_gatherVars = সঁচা;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    শূন্য visit(AstCAwait* nodep) override {
        যদি (Ast_Sen_বৃক্ষ* ধ্রুৱ sentreep = nodep->sentreep()) {
            যদি (!sentreep->user1SetOnce()) createResumeActive(nodep);
            যদি (m_inProcess) m_processDomains.ঢোকাও(sentreep);
        }
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_Var_Ref* nodep) override {
        যদি (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.ঢোকাও(nodep->varScopep());
        }
    }
    শূন্য visit(Ast_গ্ৰন্থি_CCall* ধ্রুৱ nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ধ্রুৱ auto& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        যদি (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } অন্যথা {
            বাবে (AstCFunc* ধ্রুৱ cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    শূন্য visit(AstCFunc* ধ্রুৱ nodep) override {
        ধ্রুৱ auto& মান = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        যদি (মান[0].m_state != CFuncCache::UNINITIALIZED
            || মান[1].m_state != CFuncCache::UNINITIALIZED) {
            ঘূৰাই;
        }
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_ৰাশিমালা_Stmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, Ast_গ্ৰন্থি_Stmt*& postUpdatesr,
                          std::map<ধ্রুৱ AstVarScope*, std::set<Ast_Sen_বৃক্ষ*>>& externalDomains)
        : m_scopeTopp{nodep->শীৰ্ষ_Scopep()->scopep()}
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

TimingKit prepareTiming(AstNetlist* ধ্রুৱ netlistp) {
    যদি (!v3Global.usesTiming()) ঘূৰাই {};
    LogicByScope lbs;
    Ast_গ্ৰন্থি_Stmt* postUpdates = nullptr;
    std::map<ধ্রুৱ AstVarScope*, std::set<Ast_Sen_বৃক্ষ*>> externalDomains;
    { AwaitVisitor{netlistp, lbs, postUpdates, externalDomains}; }
    ঘূৰাই {std::move(lbs), postUpdates, std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

// Transform all forked processes into functions
শ্ৰেণী TransformForksVisitor final : সার্ৱজনিক VNVisitor {
    // NODE STATE
    //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current fork.
    ধ্রুৱ VNUser1InUse m_inuser1;

    // STATE
    তার্কিক m_inClass = মিছা;  // Are we in a class?
    তার্কিক m_beginHasAwaits = মিছা;  // Does the current begin have awaits?
    AstFork* m_forkp = nullptr;  // Current fork
    AstCFunc* m_funcp = nullptr;  // Current function

    // METHODS
    // Remap local vars referenced by the given fork function
    // TODO: We should only pass variables to the fork that are
    // live in the fork body, but for that we need a proper data
    // flow analysis framework which we don't have at the moment
    শূন্য remapLocals(AstCFunc* ধ্রুৱ funcp, AstCCall* ধ্রুৱ callp) {
        ধ্রুৱ VNUser2InUse user2InUse;  // AstVarScope -> AstVarScope: var to remap to
        funcp->foreach([&](Ast_গ্ৰন্থি_Var_Ref* refp) {
            AstVar* ধ্রুৱ varp = refp->varp();
            AstVarScope* ধ্রুৱ vscp = refp->varScopep();
            AstBasicDType* ধ্রুৱ dtypep = varp->dtypep()->basicp();
            // If not a fork..join, copy. All write refs should've been handled by V3Fork
            তার্কিক pass_By_মান = !m_forkp->joinType().join();
            যদি (vscp->fork_By_মান()) {
                // Set just before the fork, for its processes to get by value
                pass_By_মান = সঁচা;
            } অন্যথা যদি (!varp->isFuncLocal()) {
                // Not func local. Its lifetime is longer than the forked process. Skip
                ঘূৰাই;
            } অন্যথা যদি (!varp->user1()) {
                // Not declared before the fork. It cannot outlive the forked process
                ঘূৰাই;
            } অন্যথা যদি (dtypep && dtypep->isForkSync()) {
                // We can just pass it by value to the new function
                pass_By_মান = সঁচা;
            }
            // Remap the reference
            যদি (!vscp->user2p()) {
                // Clone the var to the new function
                AstVar* ধ্রুৱ newvarp
                    = নূতন AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                newvarp->funcLocal(সঁচা);
                newvarp->direction(pass_By_মান ? VDirection::প্ৰবেশ : VDirection::REF);
                funcp->addArgsp(newvarp);
                AstVarScope* ধ্রুৱ newvscp
                    = নূতন AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->addVarsp(newvscp);
                vscp->user2p(newvscp);
                callp->addArgsp(নূতন AstVarRef{refp->fileline(), vscp,
                                              pass_By_মান ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* ধ্রুৱ newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    শূন্য visit(Ast_গ্ৰন্থি_Module* nodep) override {
        VL_RESTORER(m_inClass);
        m_inClass = VN_IS(nodep, Class);
        iterateChildren(nodep);
    }
    শূন্য visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        m_funcp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstVar* nodep) override {
        যদি (!m_forkp) nodep->user1(সঁচা);
    }
    শূন্য visit(AstFork* nodep) override {
        যদি (m_forkp) ঘূৰাই;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        Ast_গ্ৰন্থি* resp = nullptr;
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ declsp = nodep->declsp()) {
            resp = Ast_গ্ৰন্থি::addNext(resp, declsp->unlinkFrBackWithNext());
        }
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ stmtsp = nodep->stmtsp()) {
            resp = Ast_গ্ৰন্থি::addNext(resp, stmtsp->unlinkFrBackWithNext());
        }
        যেতিয়ালৈ (AstBegin* ধ্রুৱ beginp = nodep->forksp()) {
            যদি (Ast_গ্ৰন্থি* ধ্রুৱ declsp = beginp->declsp()) {
                resp = Ast_গ্ৰন্থি::addNext(resp, declsp->unlinkFrBackWithNext());
            }
            যদি (Ast_গ্ৰন্থি* ধ্রুৱ stmtsp = beginp->stmtsp()) {
                resp = Ast_গ্ৰন্থি::addNext(resp, stmtsp->unlinkFrBackWithNext());
            }
            VL_DO_DANGLING(ঠেলো_Deletep(beginp->unlinkFrBack()), beginp);
        }
        যদি (resp) {
            nodep->replaceWith(resp);
        } অন্যথা {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstBegin* nodep) override {
        UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
        // Start with children, so later we only find awaits that are actually in this begin
        m_beginHasAwaits = মিছা;
        iterateChildrenConst(nodep);
        যদি (!nodep->stmtsp()) ঘূৰাই;
        যদি (!m_beginHasAwaits && !nodep->needProcess()) ঘূৰাই;

        UASSERT_OBJ(!nodep->name().empty(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        File_সাৰি* ধ্রুৱ flp = nodep->fileline();
        AstCFunc* ধ্রুৱ newfuncp = নূতন AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->addNextHere(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->declPrivate(সঁচা);
        // Create the call to the function
        AstCCall* ধ্রুৱ callp = নূতন AstCCall{flp, newfuncp};
        callp->dtypeSetVoid();
        // If we're in a class, add a vlSymsp arg
        যদি (m_inClass) {
            newfuncp->addStmtsp(নূতন AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ declsp = nodep->declsp()) {
            newfuncp->addStmtsp(declsp->unlinkFrBackWithNext());
        }
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ stmtsp = nodep->stmtsp()) {
            newfuncp->addStmtsp(stmtsp->unlinkFrBackWithNext());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->addStmtsp(callp->makeStmt());
        // Propagate if needs process
        যদি (nodep->needProcess()) newfuncp->setNeedProcess();
        remapLocals(newfuncp, callp);
    }
    শূন্য visit(AstCAwait* nodep) override {
        m_beginHasAwaits = সঁচা;
        iterateChildrenConst(nodep);
    }

    //--------------------
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = স্ৱভাৱ;
};

শূন্য transformForks(AstNetlist* ধ্রুৱ netlistp) {
    যদি (!v3Global.usesTiming()) ঘূৰাই;
    { TransformForksVisitor{netlistp}; }
    V3Global::dump_Check_Global_বৃক্ষ("transform_forks", 0, dump_বৃক্ষ_Either_Level() >= 3);
}

}  // namespace V3Sched
