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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3AstUserAllocator.h"
#অন্তর্ভাব "V3ClassGraph.h"
#অন্তর্ভাব "V3EmitCBase.h"
#অন্তর্ভাব "V3Sched.h"

#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ V3Sched {

//============================================================================
// Trigger indices of the external domains of each variable

std::map<ধ্রুব AstVarScope*, std::vector<uint32_t>>
TimingKit::externalTriggerIndices(ধ্রুব TriggerKit& trigKit) ধ্রুব {
    std::map<ধ্রুব AstVarScope*, std::vector<uint32_t>> result;
    প্রতি (ধ্রুব auto& vscpDomains : m_externalDomains) {
        std::vector<uint32_t>& indices = result[vscpDomains.first];
        প্রতি (ধ্রুব Ast_Sen_বৃক্ষ* ধ্রুব domainp : vscpDomains.second) {
            প্রতি (ধ্রুব AstSenItem* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                indices.ঠেলো_back(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    প্রত্যাগম result;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    যদি (!m_resumeFuncp) {
        যদি (m_lbs.empty()) প্রত্যাগম nullptr;
        // Create global resume function
        AstNetlist* ধ্রুব netlistp = v3Global.rootp();
        AstScope* ধ্রুব scopeTopp = netlistp->শীর্ষ_Scopep()->scopep();
        m_resumeFuncp = নূতন AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(সত্য);
        m_resumeFuncp->isLoose(সত্য);
        m_resumeFuncp->isConst(অসত্য);
        m_resumeFuncp->declPrivate(সত্য);
        scopeTopp->addBlocksp(m_resumeFuncp);

        প্রতি (ধ্রুব auto& entry : m_lbs) {
            AstActive* ধ্রুব activep = entry.m_activep;
            activep->foreach([এতত্](AstCMethodHard* ধ্রুব exprp) {
                যদি (exprp->method() != VCMethod::SCHED_RESUME) প্রত্যাগম;
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব fromp = exprp->fromp();
                যদি (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    প্রত্যাগম;
                }
                AstCMethodHard* ধ্রুব moveToResumep = নূতন AstCMethodHard{
                    fromp->fileline(), fromp->clone_বৃক্ষ(অসত্য),
                    VCMethod::SCHED_MOVE_TO_RESUME_QUEUE,
                    exprp->pinsp() ? exprp->pinsp()->clone_বৃক্ষ(সত্য) : nullptr};
                moveToResumep->dtypeSetVoid();
                m_resumeFuncp->addStmtsp(moveToResumep->makeStmt());
            });
        }

        // Put all the timing actives in the resume function
        AstIf* dlyShedIfp = nullptr;
        প্রতি (ধ্রুব auto& entry : m_lbs) {
            AstActive* ধ্রুব activep = entry.m_activep;
            // Resume time delays last. For no particular reason other than
            // that's what we used to do prior to proper #0 support.
            AstVarRef* ধ্রুব schedrefp = VN_AS(
                VN_AS(VN_AS(activep->stmtsp(), Stmt_রাশিমালা)->exprp(), CMethodHard)->fromp(), VarRef);

            Ast_গ্রন্থি* ধ্রুব actionp = activep->stmtsp()->unlinkFrBackWithNext();
            যদি (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                File_সারি* ধ্রুব flp = trigVscp->fileline();
                ধ্রুব std::set<uint32_t>& triggers = entry.m_triggers;
                AstTriggered* ধ্রুব condp
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
    AstCCall* ধ্রুব callp = নূতন AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
    callp->dtypeSetVoid();
    প্রত্যাগম callp;
}

AstVarScope* TimingKit::getDelayScheduler(AstNetlist* ধ্রুব /*netlistp*/) {
    প্রতি (ধ্রুব auto& entry : m_lbs) {
        AstActive* ধ্রুব ap = entry.m_activep;
        // TODO: this triple VN_AS expression is ridiculous
        AstVarRef* ধ্রুব schedrefp
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), Stmt_রাশিমালা)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* ধ্রুব vscp = schedrefp->varScopep();
        যদি (vscp->dtypep()->basicp()->isDelayScheduler()) প্রত্যাগম vscp;
    }
    // None found. Design doesn't use any time delays
    প্রত্যাগম nullptr;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    যদি (!m_readyFuncp) {
        প্রতি (ধ্রুব auto& entry : m_lbs) {
            AstActive* ধ্রুব activep = entry.m_activep;
            auto* ধ্রুব resumep = VN_AS(VN_AS(activep->stmtsp(), Stmt_রাশিমালা)->exprp(), CMethodHard);
            UASSERT_OBJ(!resumep->nextp(), resumep, "Should be the only statement here");
            AstVarScope* ধ্রুব schedulerp = VN_AS(resumep->fromp(), VarRef)->varScopep();
            UASSERT_OBJ(schedulerp->dtypep()->basicp()->isDelayScheduler()
                            || schedulerp->dtypep()->basicp()->isTriggerScheduler()
                            || schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler(),
                        schedulerp, "Unexpected type");
            যদি (!schedulerp->dtypep()->basicp()->isTriggerScheduler()) অনুবর্ত;
            // Create the global ready function only if we have trigger schedulers
            যদি (!m_readyFuncp) {
                AstNetlist* ধ্রুব netlistp = v3Global.rootp();
                AstScope* ধ্রুব scopeTopp = netlistp->শীর্ষ_Scopep()->scopep();
                m_readyFuncp = নূতন AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(সত্য);
                m_readyFuncp->isLoose(সত্য);
                m_readyFuncp->isConst(অসত্য);
                m_readyFuncp->declPrivate(সত্য);
                scopeTopp->addBlocksp(m_readyFuncp);
            }

            File_সারি* ধ্রুব flp = activep->sentreep()->fileline();

            // Create an 'AstIf' sensitive to the suspending triggers
            ধ্রুব std::set<uint32_t>& triggers = entry.m_triggers;
            AstTriggered* ধ্রুব condp
                = নূতন AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
            AstIf* ধ্রুব ifp = নূতন AstIf{flp, condp};
            m_readyFuncp->addStmtsp(ifp);

            // Mark as ready the processes resumed on this sensitivity expression
            AstVarRef* ধ্রুব refp = নূতন AstVarRef{flp, schedulerp, VAccess::READWRITE};
            AstCMethodHard* ধ্রুব callp = নূতন AstCMethodHard{flp, refp, VCMethod::SCHED_READY};
            callp->dtypeSetVoid();
            যদি (resumep->pinsp()) callp->addPinsp(resumep->pinsp()->clone_বৃক্ষ(অসত্য));
            ifp->addThensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        যদি (!m_readyFuncp) প্রত্যাগম nullptr;
    }
    AstCCall* ধ্রুব callp = নূতন AstCCall{m_readyFuncp->fileline(), m_readyFuncp};
    callp->dtypeSetVoid();
    প্রত্যাগম callp;
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

বর্গ AwaitVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    //  AstSenTree::user1()  -> bool.               Set true if the sentree has been visited.
    //  AstCFunc::user1()    -> AstUser1Allocator.  See alocator below
    ধ্রুব VNUser1InUse m_inuser1;

    // STATE
    তার্কিক m_inProcess = অসত্য;  // Are we in a process?
    তার্কিক m_gatherVars = অসত্য;  // Should we gather vars in m_writtenBySuspendable?
    AstScope* ধ্রুব m_scopeTopp;  // Scope at the top
    LogicByScope& m_lbs;  // Timing resume actives
    Ast_গ্রন্থি_Stmt*& m_postUpdatesr;  // Post updates for the trigger eval function
    // Additional var sensitivities
    std::map<ধ্রুব AstVarScope*, std::set<Ast_Sen_বৃক্ষ*>>& m_externalDomains;
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
    std::vector<ধ্রুব AstCFunc*> m_call_স্তূপ;  // Current callstack of AstCFuncs
    // Caches how visiting the function with given value of m_gatherVars changes
    // m_processDomains and m_writtenBySuspendable - only accessed from visit(AstCFunc* nodep)
    AstUser1Allocator<AstCFunc, std::array<CFuncCache, 2>> m_cfuncsCache;
    // Count uses of not inlined writes to signals in suspendables
    VDouble0 m_notInlinedWritesInSuspendableUsage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    শূন্য addResumePins(AstCMethodHard* ধ্রুব resumep, Ast_গ্রন্থি_রাশিমালা* pinsp) {
        AstCExpr* ধ্রুব exprp = VN_CAST(pinsp, CExpr);
        AstText* ধ্রুব textp = VN_CAST(exprp->nodesp(), Text);
        যদি (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            যদি ((pinsp = VN_CAST(pinsp->nextp(), গ্রন্থি_রাশিমালা))) {
                resumep->addPinsp(pinsp->clone_বৃক্ষ(অসত্য));
            }
        } অন্যথা {
            resumep->addPinsp(pinsp->clone_বৃক্ষ(অসত্য));
        }
    }
    শূন্য add_CFunc_Cached_মানসমূহ(ধ্রুব AstCFunc* ধ্রুব cfuncp,
                              std::unordered_set<ধ্রুব AstCFunc*>& visited) {
        যদি (!visited.ঢোকাও(cfuncp).second) প্রত্যাগম;
        CFuncCache& মান = m_cfuncsCache(cfuncp)[static_cast<size_t>(m_gatherVars)];
        ধ্রুব size_t initialSize = m_writtenBySuspendable.size();
        m_writtenBySuspendable.ঢোকাও(মান.m_writtenBySuspendable.begin(),
                                      মান.m_writtenBySuspendable.end());
        m_notInlinedWritesInSuspendableUsage += m_writtenBySuspendable.size() - initialSize;
        m_processDomains.ঢোকাও(মান.m_processDomains.begin(), মান.m_processDomains.end());
        প্রতি (ধ্রুব AstCFunc* ধ্রুব includedp : মান.m_includes) {
            add_CFunc_Cached_মানসমূহ(includedp, visited);
        }
    }
    // Create an active with a timing scheduler resume() call
    শূন্য createResumeActive(AstCAwait* ধ্রুব awaitp) {
        auto* ধ্রুব methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* ধ্রুব schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        Ast_Sen_বৃক্ষ* ধ্রুব sentreep = awaitp->sentreep();
        File_সারি* ধ্রুব flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        auto* ধ্রুব resumep = নূতন AstCMethodHard{
            flp, নূতন AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtypeSetVoid();
        যদি (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            যদি (Ast_গ্রন্থি* ধ্রুব dbginfop = methodp->pinsp()->nextp()) {
                যদি (methodp->pinsp()) addResumePins(resumep, static_cast<Ast_গ্রন্থি_রাশিমালা*>(dbginfop));
            }
        } অন্যথা যদি (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            auto* ধ্রুব postp = resumep->clone_বৃক্ষ(অসত্য);
            postp->method(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = Ast_গ্রন্থি::addNext(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        AstActive* ধ্রুব activep = নূতন AstActive{flp, "_timing", sentreep};
        activep->addStmtsp(resumep->makeStmt());
        m_lbs.add(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    শূন্য visitCalledCFunc(AstCFunc* ধ্রুব nodep) {
        যদি (!m_inProcess) প্রত্যাগম;  // Skip if not in process - nothing would be added to any set
        ধ্রুব size_t cache_চাবি = static_cast<size_t>(m_gatherVars);
        CFuncCache& মান = m_cfuncsCache(nodep)[cache_চাবি];
        বিকল্প (মান.m_state) {
        পক্ষ CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            মান.m_state = CFuncCache::VISITING;
            m_call_স্তূপ.ঠেলো_back(nodep);
            iterateChildren(nodep);
            m_call_স্তূপ.বের_করো_back();
            মান.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_processDomains, মান.m_processDomains);
            std::swap(m_writtenBySuspendable, মান.m_writtenBySuspendable);
        } ভঙ্গ;
        পক্ষ CFuncCache::VISITING: {
            প্রতি (size_t i = m_call_স্তূপ.size() - 1; m_call_স্তূপ.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_স্তূপ[i])[cache_চাবি].m_includes.ঢোকাও(nodep);
            }
            প্রত্যাগম;  // Break recursion
        }
        পক্ষ CFuncCache::INITIALIZED: ভঙ্গ;
        }
        std::unordered_set<ধ্রুব AstCFunc*> visited;
        add_CFunc_Cached_মানসমূহ(nodep, visited);
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Procedure* ধ্রুব nodep) override {
        UASSERT_OBJ(!m_inProcess && !m_gatherVars && m_processDomains.empty()
                        && m_writtenBySuspendable.empty(),
                    nodep, "Process in process?");
        m_inProcess = সত্য;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        প্রতি (AstVarScope* ধ্রুব vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].ঢোকাও(m_processDomains.begin(), m_processDomains.end());
            vscp->varp()->setWrittenBySuspendable();
        }
        m_processDomains.clear();
        m_writtenBySuspendable.clear();
        m_inProcess = অসত্য;
        m_gatherVars = অসত্য;
    }
    শূন্য visit(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        যদি (m_inProcess) m_gatherVars = সত্য;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    শূন্য visit(AstCAwait* nodep) override {
        যদি (Ast_Sen_বৃক্ষ* ধ্রুব sentreep = nodep->sentreep()) {
            যদি (!sentreep->user1SetOnce()) createResumeActive(nodep);
            যদি (m_inProcess) m_processDomains.ঢোকাও(sentreep);
        }
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        যদি (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.ঢোকাও(nodep->varScopep());
        }
    }
    শূন্য visit(Ast_গ্রন্থি_CCall* ধ্রুব nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ধ্রুব auto& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        যদি (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } অন্যথা {
            প্রতি (AstCFunc* ধ্রুব cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    শূন্য visit(AstCFunc* ধ্রুব nodep) override {
        ধ্রুব auto& মান = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        যদি (মান[0].m_state != CFuncCache::UNINITIALIZED
            || মান[1].m_state != CFuncCache::UNINITIALIZED) {
            প্রত্যাগম;
        }
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_রাশিমালা_Stmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    explicit AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, Ast_গ্রন্থি_Stmt*& postUpdatesr,
                          std::map<ধ্রুব AstVarScope*, std::set<Ast_Sen_বৃক্ষ*>>& externalDomains)
        : m_scopeTopp{nodep->শীর্ষ_Scopep()->scopep()}
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

TimingKit prepareTiming(AstNetlist* ধ্রুব netlistp) {
    যদি (!v3Global.usesTiming()) প্রত্যাগম {};
    LogicByScope lbs;
    Ast_গ্রন্থি_Stmt* postUpdates = nullptr;
    std::map<ধ্রুব AstVarScope*, std::set<Ast_Sen_বৃক্ষ*>> externalDomains;
    { AwaitVisitor{netlistp, lbs, postUpdates, externalDomains}; }
    প্রত্যাগম {std::move(lbs), postUpdates, std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

// Transform all forked processes into functions
বর্গ TransformForksVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current fork.
    ধ্রুব VNUser1InUse m_inuser1;

    // STATE
    তার্কিক m_inClass = অসত্য;  // Are we in a class?
    তার্কিক m_beginHasAwaits = অসত্য;  // Does the current begin have awaits?
    AstFork* m_forkp = nullptr;  // Current fork
    AstCFunc* m_funcp = nullptr;  // Current function

    // METHODS
    // Remap local vars referenced by the given fork function
    // TODO: We should only pass variables to the fork that are
    // live in the fork body, but for that we need a proper data
    // flow analysis framework which we don't have at the moment
    শূন্য remapLocals(AstCFunc* ধ্রুব funcp, AstCCall* ধ্রুব callp) {
        ধ্রুব VNUser2InUse user2InUse;  // AstVarScope -> AstVarScope: var to remap to
        funcp->foreach([&](Ast_গ্রন্থি_Var_Ref* refp) {
            AstVar* ধ্রুব varp = refp->varp();
            AstVarScope* ধ্রুব vscp = refp->varScopep();
            AstBasicDType* ধ্রুব dtypep = varp->dtypep()->basicp();
            // If not a fork..join, copy. All write refs should've been handled by V3Fork
            তার্কিক pass_By_মান = !m_forkp->joinType().join();
            যদি (vscp->fork_By_মান()) {
                // Set just before the fork, for its processes to get by value
                pass_By_মান = সত্য;
            } অন্যথা যদি (!varp->isFuncLocal()) {
                // Not func local. Its lifetime is longer than the forked process. Skip
                প্রত্যাগম;
            } অন্যথা যদি (!varp->user1()) {
                // Not declared before the fork. It cannot outlive the forked process
                প্রত্যাগম;
            } অন্যথা যদি (dtypep && dtypep->isForkSync()) {
                // We can just pass it by value to the new function
                pass_By_মান = সত্য;
            }
            // Remap the reference
            যদি (!vscp->user2p()) {
                // Clone the var to the new function
                AstVar* ধ্রুব newvarp
                    = নূতন AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                newvarp->funcLocal(সত্য);
                newvarp->direction(pass_By_মান ? VDirection::প্রবেশ : VDirection::REF);
                funcp->addArgsp(newvarp);
                AstVarScope* ধ্রুব newvscp
                    = নূতন AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->addVarsp(newvscp);
                vscp->user2p(newvscp);
                callp->addArgsp(নূতন AstVarRef{refp->fileline(), vscp,
                                              pass_By_মান ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* ধ্রুব newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
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
        যদি (!m_forkp) nodep->user1(সত্য);
    }
    শূন্য visit(AstFork* nodep) override {
        যদি (m_forkp) প্রত্যাগম;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        Ast_গ্রন্থি* resp = nullptr;
        যদি (Ast_গ্রন্থি* ধ্রুব declsp = nodep->declsp()) {
            resp = Ast_গ্রন্থি::addNext(resp, declsp->unlinkFrBackWithNext());
        }
        যদি (Ast_গ্রন্থি* ধ্রুব stmtsp = nodep->stmtsp()) {
            resp = Ast_গ্রন্থি::addNext(resp, stmtsp->unlinkFrBackWithNext());
        }
        যাবত্ (AstBegin* ধ্রুব beginp = nodep->forksp()) {
            যদি (Ast_গ্রন্থি* ধ্রুব declsp = beginp->declsp()) {
                resp = Ast_গ্রন্থি::addNext(resp, declsp->unlinkFrBackWithNext());
            }
            যদি (Ast_গ্রন্থি* ধ্রুব stmtsp = beginp->stmtsp()) {
                resp = Ast_গ্রন্থি::addNext(resp, stmtsp->unlinkFrBackWithNext());
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
        m_beginHasAwaits = অসত্য;
        iterateChildrenConst(nodep);
        যদি (!nodep->stmtsp()) প্রত্যাগম;
        যদি (!m_beginHasAwaits && !nodep->needProcess()) প্রত্যাগম;

        UASSERT_OBJ(!nodep->name().empty(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        File_সারি* ধ্রুব flp = nodep->fileline();
        AstCFunc* ধ্রুব newfuncp = নূতন AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->addNextHere(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->declPrivate(সত্য);
        // Create the call to the function
        AstCCall* ধ্রুব callp = নূতন AstCCall{flp, newfuncp};
        callp->dtypeSetVoid();
        // If we're in a class, add a vlSymsp arg
        যদি (m_inClass) {
            newfuncp->addStmtsp(নূতন AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        যদি (Ast_গ্রন্থি* ধ্রুব declsp = nodep->declsp()) {
            newfuncp->addStmtsp(declsp->unlinkFrBackWithNext());
        }
        যদি (Ast_গ্রন্থি* ধ্রুব stmtsp = nodep->stmtsp()) {
            newfuncp->addStmtsp(stmtsp->unlinkFrBackWithNext());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->addStmtsp(callp->makeStmt());
        // Propagate if needs process
        যদি (nodep->needProcess()) newfuncp->setNeedProcess();
        remapLocals(newfuncp, callp);
    }
    শূন্য visit(AstCAwait* nodep) override {
        m_beginHasAwaits = সত্য;
        iterateChildrenConst(nodep);
    }

    //--------------------
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    explicit TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = স্বভাব;
};

শূন্য transformForks(AstNetlist* ধ্রুব netlistp) {
    যদি (!v3Global.usesTiming()) প্রত্যাগম;
    { TransformForksVisitor{netlistp}; }
    V3Global::dump_Check_Global_বৃক্ষ("transform_forks", 0, dump_বৃক্ষ_Either_Level() >= 3);
}

}  // namespace V3Sched
