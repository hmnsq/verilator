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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3AstUserAllocator.h"
#شامل "V3ClassGraph.h"
#شامل "V3EmitCBase.h"
#شامل "V3Sched.h"

#شامل <unordered_map>
#شامل <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

فضاینام V3Sched {

//============================================================================
// Trigger indices of the external domains of each variable

std::map<ثابت AstVarScope*, std::vector<uint32_t>>
TimingKit::externalTriggerIndices(ثابت TriggerKit& trigKit) ثابت {
    std::map<ثابت AstVarScope*, std::vector<uint32_t>> result;
    برای (ثابت auto& vscpDomains : m_externalDomains) {
        std::vector<uint32_t>& indices = result[vscpDomains.first];
        برای (ثابت Ast_Sen_درخت* ثابت domainp : vscpDomains.second) {
            برای (ثابت AstSenItem* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                indices.افزودن_back(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    بازگشت result;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    اگر (!m_resumeFuncp) {
        اگر (m_lbs.empty()) بازگشت nullptr;
        // Create global resume function
        AstNetlist* ثابت netlistp = v3Global.rootp();
        AstScope* ثابت scopeTopp = netlistp->بالاترین_Scopep()->scopep();
        m_resumeFuncp = جدید AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(درست);
        m_resumeFuncp->isLoose(درست);
        m_resumeFuncp->isConst(نادرست);
        m_resumeFuncp->declPrivate(درست);
        scopeTopp->addBlocksp(m_resumeFuncp);

        برای (ثابت auto& entry : m_lbs) {
            AstActive* ثابت activep = entry.m_activep;
            activep->foreach([این](AstCMethodHard* ثابت exprp) {
                اگر (exprp->method() != VCMethod::SCHED_RESUME) بازگشت;
                Ast_گره_عبارت* ثابت fromp = exprp->fromp();
                اگر (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    بازگشت;
                }
                AstCMethodHard* ثابت moveToResumep = جدید AstCMethodHard{
                    fromp->fileline(), fromp->clone_درخت(نادرست),
                    VCMethod::SCHED_MOVE_TO_RESUME_QUEUE,
                    exprp->pinsp() ? exprp->pinsp()->clone_درخت(درست) : nullptr};
                moveToResumep->dtypeSetVoid();
                m_resumeFuncp->addStmtsp(moveToResumep->makeStmt());
            });
        }

        // Put all the timing actives in the resume function
        AstIf* dlyShedIfp = nullptr;
        برای (ثابت auto& entry : m_lbs) {
            AstActive* ثابت activep = entry.m_activep;
            // Resume time delays last. For no particular reason other than
            // that's what we used to do prior to proper #0 support.
            AstVarRef* ثابت schedrefp = VN_AS(
                VN_AS(VN_AS(activep->stmtsp(), Stmt_عبارت)->exprp(), CMethodHard)->fromp(), VarRef);

            Ast_گره* ثابت actionp = activep->stmtsp()->unlinkFrBackWithNext();
            اگر (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                File_سطر* ثابت flp = trigVscp->fileline();
                ثابت std::set<uint32_t>& triggers = entry.m_triggers;
                AstTriggered* ثابت condp
                    = جدید AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
                dlyShedIfp = جدید AstIf{flp, condp};
                dlyShedIfp->addThensp(actionp);
            } وگرنه {
                m_resumeFuncp->addStmtsp(actionp);
            }
        }
        اگر (dlyShedIfp) m_resumeFuncp->addStmtsp(dlyShedIfp);

        // These are now spent, oispose of now empty AstActive instances
        m_lbs.deleteActives();
    }
    AstCCall* ثابت callp = جدید AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
    callp->dtypeSetVoid();
    بازگشت callp;
}

AstVarScope* TimingKit::getDelayScheduler(AstNetlist* ثابت /*netlistp*/) {
    برای (ثابت auto& entry : m_lbs) {
        AstActive* ثابت ap = entry.m_activep;
        // TODO: this triple VN_AS expression is ridiculous
        AstVarRef* ثابت schedrefp
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), Stmt_عبارت)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* ثابت vscp = schedrefp->varScopep();
        اگر (vscp->dtypep()->basicp()->isDelayScheduler()) بازگشت vscp;
    }
    // None found. Design doesn't use any time delays
    بازگشت nullptr;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    اگر (!m_readyFuncp) {
        برای (ثابت auto& entry : m_lbs) {
            AstActive* ثابت activep = entry.m_activep;
            auto* ثابت resumep = VN_AS(VN_AS(activep->stmtsp(), Stmt_عبارت)->exprp(), CMethodHard);
            UASSERT_OBJ(!resumep->nextp(), resumep, "Should be the only statement here");
            AstVarScope* ثابت schedulerp = VN_AS(resumep->fromp(), VarRef)->varScopep();
            UASSERT_OBJ(schedulerp->dtypep()->basicp()->isDelayScheduler()
                            || schedulerp->dtypep()->basicp()->isTriggerScheduler()
                            || schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler(),
                        schedulerp, "Unexpected type");
            اگر (!schedulerp->dtypep()->basicp()->isTriggerScheduler()) ادامه;
            // Create the global ready function only if we have trigger schedulers
            اگر (!m_readyFuncp) {
                AstNetlist* ثابت netlistp = v3Global.rootp();
                AstScope* ثابت scopeTopp = netlistp->بالاترین_Scopep()->scopep();
                m_readyFuncp = جدید AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(درست);
                m_readyFuncp->isLoose(درست);
                m_readyFuncp->isConst(نادرست);
                m_readyFuncp->declPrivate(درست);
                scopeTopp->addBlocksp(m_readyFuncp);
            }

            File_سطر* ثابت flp = activep->sentreep()->fileline();

            // Create an 'AstIf' sensitive to the suspending triggers
            ثابت std::set<uint32_t>& triggers = entry.m_triggers;
            AstTriggered* ثابت condp
                = جدید AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
            AstIf* ثابت ifp = جدید AstIf{flp, condp};
            m_readyFuncp->addStmtsp(ifp);

            // Mark as ready the processes resumed on this sensitivity expression
            AstVarRef* ثابت refp = جدید AstVarRef{flp, schedulerp, VAccess::READWRITE};
            AstCMethodHard* ثابت callp = جدید AstCMethodHard{flp, refp, VCMethod::SCHED_READY};
            callp->dtypeSetVoid();
            اگر (resumep->pinsp()) callp->addPinsp(resumep->pinsp()->clone_درخت(نادرست));
            ifp->addThensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        اگر (!m_readyFuncp) بازگشت nullptr;
    }
    AstCCall* ثابت callp = جدید AstCCall{m_readyFuncp->fileline(), m_readyFuncp};
    callp->dtypeSetVoid();
    بازگشت callp;
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

رده AwaitVisitor final : عمومی VNVisitor {
    // NODE STATE
    //  AstSenTree::user1()  -> bool.               Set true if the sentree has been visited.
    //  AstCFunc::user1()    -> AstUser1Allocator.  See alocator below
    ثابت VNUser1InUse m_inuser1;

    // STATE
    منطقی m_inProcess = نادرست;  // Are we in a process?
    منطقی m_gatherVars = نادرست;  // Should we gather vars in m_writtenBySuspendable?
    AstScope* ثابت m_scopeTopp;  // Scope at the top
    LogicByScope& m_lbs;  // Timing resume actives
    Ast_گره_Stmt*& m_postUpdatesr;  // Post updates for the trigger eval function
    // Additional var sensitivities
    std::map<ثابت AstVarScope*, std::set<Ast_Sen_درخت*>>& m_externalDomains;
    std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::set<Ast_Sen_درخت*> m_processDomains;  // Sentrees from the current process
    // Variables written by suspendable processes
    std::set<AstVarScope*> m_writtenBySuspendable;
    ساختار CFuncCache final {
        std::set<Ast_Sen_درخت*> m_processDomains;  // What shall be added to m_processDomains
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
        شمارش State : uint8_t {
            UNINITIALIZED = 0,  // Not initialized members are empty
            VISITING,  // Visiting - needed for breaking recursion
            INITIALIZED,  // Members contains correct values
        } m_state  // Current state of Cache
            = UNINITIALIZED;
    };
    std::vector<ثابت AstCFunc*> m_call_پشته;  // Current callstack of AstCFuncs
    // Caches how visiting the function with given value of m_gatherVars changes
    // m_processDomains and m_writtenBySuspendable - only accessed from visit(AstCFunc* nodep)
    AstUser1Allocator<AstCFunc, std::array<CFuncCache, 2>> m_cfuncsCache;
    // Count uses of not inlined writes to signals in suspendables
    VDouble0 m_notInlinedWritesInSuspendableUsage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    تهی addResumePins(AstCMethodHard* ثابت resumep, Ast_گره_عبارت* pinsp) {
        AstCExpr* ثابت exprp = VN_CAST(pinsp, CExpr);
        AstText* ثابت textp = VN_CAST(exprp->nodesp(), Text);
        اگر (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            اگر ((pinsp = VN_CAST(pinsp->nextp(), گره_عبارت))) {
                resumep->addPinsp(pinsp->clone_درخت(نادرست));
            }
        } وگرنه {
            resumep->addPinsp(pinsp->clone_درخت(نادرست));
        }
    }
    تهی add_CFunc_Cached_مقادیر(ثابت AstCFunc* ثابت cfuncp,
                              std::unordered_set<ثابت AstCFunc*>& visited) {
        اگر (!visited.درج(cfuncp).second) بازگشت;
        CFuncCache& مقدار = m_cfuncsCache(cfuncp)[static_cast<size_t>(m_gatherVars)];
        ثابت size_t initialSize = m_writtenBySuspendable.size();
        m_writtenBySuspendable.درج(مقدار.m_writtenBySuspendable.begin(),
                                      مقدار.m_writtenBySuspendable.end());
        m_notInlinedWritesInSuspendableUsage += m_writtenBySuspendable.size() - initialSize;
        m_processDomains.درج(مقدار.m_processDomains.begin(), مقدار.m_processDomains.end());
        برای (ثابت AstCFunc* ثابت includedp : مقدار.m_includes) {
            add_CFunc_Cached_مقادیر(includedp, visited);
        }
    }
    // Create an active with a timing scheduler resume() call
    تهی createResumeActive(AstCAwait* ثابت awaitp) {
        auto* ثابت methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* ثابت schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        Ast_Sen_درخت* ثابت sentreep = awaitp->sentreep();
        File_سطر* ثابت flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        auto* ثابت resumep = جدید AstCMethodHard{
            flp, جدید AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtypeSetVoid();
        اگر (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            اگر (Ast_گره* ثابت dbginfop = methodp->pinsp()->nextp()) {
                اگر (methodp->pinsp()) addResumePins(resumep, static_cast<Ast_گره_عبارت*>(dbginfop));
            }
        } وگرنه اگر (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            auto* ثابت postp = resumep->clone_درخت(نادرست);
            postp->method(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = Ast_گره::addNext(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        AstActive* ثابت activep = جدید AstActive{flp, "_timing", sentreep};
        activep->addStmtsp(resumep->makeStmt());
        m_lbs.add(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    تهی visitCalledCFunc(AstCFunc* ثابت nodep) {
        اگر (!m_inProcess) بازگشت;  // Skip if not in process - nothing would be added to any set
        ثابت size_t cache_کلید = static_cast<size_t>(m_gatherVars);
        CFuncCache& مقدار = m_cfuncsCache(nodep)[cache_کلید];
        انتخاب (مقدار.m_state) {
        حالت CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            مقدار.m_state = CFuncCache::VISITING;
            m_call_پشته.افزودن_back(nodep);
            iterateChildren(nodep);
            m_call_پشته.برداشت_back();
            مقدار.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_processDomains, مقدار.m_processDomains);
            std::swap(m_writtenBySuspendable, مقدار.m_writtenBySuspendable);
        } بشکن;
        حالت CFuncCache::VISITING: {
            برای (size_t i = m_call_پشته.size() - 1; m_call_پشته.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_پشته[i])[cache_کلید].m_includes.درج(nodep);
            }
            بازگشت;  // Break recursion
        }
        حالت CFuncCache::INITIALIZED: بشکن;
        }
        std::unordered_set<ثابت AstCFunc*> visited;
        add_CFunc_Cached_مقادیر(nodep, visited);
    }

    // VISITORS
    تهی visit(Ast_گره_Procedure* ثابت nodep) override {
        UASSERT_OBJ(!m_inProcess && !m_gatherVars && m_processDomains.empty()
                        && m_writtenBySuspendable.empty(),
                    nodep, "Process in process?");
        m_inProcess = درست;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        برای (AstVarScope* ثابت vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].درج(m_processDomains.begin(), m_processDomains.end());
            vscp->varp()->setWrittenBySuspendable();
        }
        m_processDomains.clear();
        m_writtenBySuspendable.clear();
        m_inProcess = نادرست;
        m_gatherVars = نادرست;
    }
    تهی visit(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        اگر (m_inProcess) m_gatherVars = درست;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    تهی visit(AstCAwait* nodep) override {
        اگر (Ast_Sen_درخت* ثابت sentreep = nodep->sentreep()) {
            اگر (!sentreep->user1SetOnce()) createResumeActive(nodep);
            اگر (m_inProcess) m_processDomains.درج(sentreep);
        }
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_Var_Ref* nodep) override {
        اگر (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.درج(nodep->varScopep());
        }
    }
    تهی visit(Ast_گره_CCall* ثابت nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ثابت auto& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        اگر (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } وگرنه {
            برای (AstCFunc* ثابت cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    تهی visit(AstCFunc* ثابت nodep) override {
        ثابت auto& مقدار = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        اگر (مقدار[0].m_state != CFuncCache::UNINITIALIZED
            || مقدار[1].m_state != CFuncCache::UNINITIALIZED) {
            بازگشت;
        }
        iterateChildren(nodep);
    }
    تهی visit(Ast_عبارت_Stmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, Ast_گره_Stmt*& postUpdatesr,
                          std::map<ثابت AstVarScope*, std::set<Ast_Sen_درخت*>>& externalDomains)
        : m_scopeTopp{nodep->بالاترین_Scopep()->scopep()}
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
    اگر (!v3Global.usesTiming()) بازگشت {};
    LogicByScope lbs;
    Ast_گره_Stmt* postUpdates = nullptr;
    std::map<ثابت AstVarScope*, std::set<Ast_Sen_درخت*>> externalDomains;
    { AwaitVisitor{netlistp, lbs, postUpdates, externalDomains}; }
    بازگشت {std::move(lbs), postUpdates, std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

// Transform all forked processes into functions
رده TransformForksVisitor final : عمومی VNVisitor {
    // NODE STATE
    //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current fork.
    ثابت VNUser1InUse m_inuser1;

    // STATE
    منطقی m_inClass = نادرست;  // Are we in a class?
    منطقی m_beginHasAwaits = نادرست;  // Does the current begin have awaits?
    AstFork* m_forkp = nullptr;  // Current fork
    AstCFunc* m_funcp = nullptr;  // Current function

    // METHODS
    // Remap local vars referenced by the given fork function
    // TODO: We should only pass variables to the fork that are
    // live in the fork body, but for that we need a proper data
    // flow analysis framework which we don't have at the moment
    تهی remapLocals(AstCFunc* ثابت funcp, AstCCall* ثابت callp) {
        ثابت VNUser2InUse user2InUse;  // AstVarScope -> AstVarScope: var to remap to
        funcp->foreach([&](Ast_گره_Var_Ref* refp) {
            AstVar* ثابت varp = refp->varp();
            AstVarScope* ثابت vscp = refp->varScopep();
            AstBasicDType* ثابت dtypep = varp->dtypep()->basicp();
            // If not a fork..join, copy. All write refs should've been handled by V3Fork
            منطقی pass_By_مقدار = !m_forkp->joinType().join();
            اگر (vscp->fork_By_مقدار()) {
                // Set just before the fork, for its processes to get by value
                pass_By_مقدار = درست;
            } وگرنه اگر (!varp->isFuncLocal()) {
                // Not func local. Its lifetime is longer than the forked process. Skip
                بازگشت;
            } وگرنه اگر (!varp->user1()) {
                // Not declared before the fork. It cannot outlive the forked process
                بازگشت;
            } وگرنه اگر (dtypep && dtypep->isForkSync()) {
                // We can just pass it by value to the new function
                pass_By_مقدار = درست;
            }
            // Remap the reference
            اگر (!vscp->user2p()) {
                // Clone the var to the new function
                AstVar* ثابت newvarp
                    = جدید AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                newvarp->funcLocal(درست);
                newvarp->direction(pass_By_مقدار ? VDirection::درون_داد : VDirection::REF);
                funcp->addArgsp(newvarp);
                AstVarScope* ثابت newvscp
                    = جدید AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->addVarsp(newvscp);
                vscp->user2p(newvscp);
                callp->addArgsp(جدید AstVarRef{refp->fileline(), vscp,
                                              pass_By_مقدار ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* ثابت newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_inClass);
        m_inClass = VN_IS(nodep, Class);
        iterateChildren(nodep);
    }
    تهی visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        m_funcp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstVar* nodep) override {
        اگر (!m_forkp) nodep->user1(درست);
    }
    تهی visit(AstFork* nodep) override {
        اگر (m_forkp) بازگشت;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        Ast_گره* resp = nullptr;
        اگر (Ast_گره* ثابت declsp = nodep->declsp()) {
            resp = Ast_گره::addNext(resp, declsp->unlinkFrBackWithNext());
        }
        اگر (Ast_گره* ثابت stmtsp = nodep->stmtsp()) {
            resp = Ast_گره::addNext(resp, stmtsp->unlinkFrBackWithNext());
        }
        وقتی (AstBegin* ثابت beginp = nodep->forksp()) {
            اگر (Ast_گره* ثابت declsp = beginp->declsp()) {
                resp = Ast_گره::addNext(resp, declsp->unlinkFrBackWithNext());
            }
            اگر (Ast_گره* ثابت stmtsp = beginp->stmtsp()) {
                resp = Ast_گره::addNext(resp, stmtsp->unlinkFrBackWithNext());
            }
            VL_DO_DANGLING(افزودن_Deletep(beginp->unlinkFrBack()), beginp);
        }
        اگر (resp) {
            nodep->replaceWith(resp);
        } وگرنه {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstBegin* nodep) override {
        UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
        // Start with children, so later we only find awaits that are actually in this begin
        m_beginHasAwaits = نادرست;
        iterateChildrenConst(nodep);
        اگر (!nodep->stmtsp()) بازگشت;
        اگر (!m_beginHasAwaits && !nodep->needProcess()) بازگشت;

        UASSERT_OBJ(!nodep->name().empty(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        File_سطر* ثابت flp = nodep->fileline();
        AstCFunc* ثابت newfuncp = جدید AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->addNextHere(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->declPrivate(درست);
        // Create the call to the function
        AstCCall* ثابت callp = جدید AstCCall{flp, newfuncp};
        callp->dtypeSetVoid();
        // If we're in a class, add a vlSymsp arg
        اگر (m_inClass) {
            newfuncp->addStmtsp(جدید AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        اگر (Ast_گره* ثابت declsp = nodep->declsp()) {
            newfuncp->addStmtsp(declsp->unlinkFrBackWithNext());
        }
        اگر (Ast_گره* ثابت stmtsp = nodep->stmtsp()) {
            newfuncp->addStmtsp(stmtsp->unlinkFrBackWithNext());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->addStmtsp(callp->makeStmt());
        // Propagate if needs process
        اگر (nodep->needProcess()) newfuncp->setNeedProcess();
        remapLocals(newfuncp, callp);
    }
    تهی visit(AstCAwait* nodep) override {
        m_beginHasAwaits = درست;
        iterateChildrenConst(nodep);
    }

    //--------------------
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = پیشفرض;
};

تهی transformForks(AstNetlist* ثابت netlistp) {
    اگر (!v3Global.usesTiming()) بازگشت;
    { TransformForksVisitor{netlistp}; }
    V3Global::dump_Check_Global_درخت("transform_forks", 0, dump_درخت_Either_Level() >= 3);
}

}  // namespace V3Sched
