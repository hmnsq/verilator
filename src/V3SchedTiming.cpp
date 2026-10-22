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
        لكل (ثابت Ast_Sen_بشيا* ثابت domainp : vscpDomains.second) {
            لكل (ثابت AstSenItem* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                indices.تور_back(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    أرجع result;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    إذا (!m_resumeFuncp) {
        إذا (m_lbs.empty()) أرجع nullptr;
        // Create global resume function
        AstNetlist* ثابت netlistp = v3Global.rootp();
        AstScope* ثابت scopeTopp = netlistp->كولي_Scopep()->scopep();
        m_resumeFuncp = جديد AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(true);
        m_resumeFuncp->isLoose(true);
        m_resumeFuncp->isConst(خطأ);
        m_resumeFuncp->declPrivate(true);
        scopeTopp->addBlocksp(m_resumeFuncp);

        لكل (ثابت auto& entry : m_lbs) {
            AstActive* ثابت activep = entry.m_activep;
            activep->foreach([هذا](AstCMethodHard* ثابت exprp) {
                إذا (exprp->method() != VCMethod::SCHED_RESUME) أرجع;
                Ast_كلي_فرچ* ثابت fromp = exprp->fromp();
                إذا (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    أرجع;
                }
                AstCMethodHard* ثابت moveToResumep = جديد AstCMethodHard{
                    fromp->fileline(), fromp->clone_بشيا(خطأ),
                    VCMethod::SCHED_MOVE_TO_RESUME_QUEUE,
                    exprp->pinsp() ? exprp->pinsp()->clone_بشيا(true) : nullptr};
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
                VN_AS(VN_AS(activep->stmtsp(), Stmt_فرچ)->exprp(), CMethodHard)->fromp(), VarRef);

            Ast_كلي* ثابت actionp = activep->stmtsp()->unlinkFrBackWithNext();
            إذا (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                File_لي* ثابت flp = trigVscp->fileline();
                ثابت std::set<uint32_t>& triggers = entry.m_triggers;
                AstTriggered* ثابت condp
                    = جديد AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
                dlyShedIfp = جديد AstIf{flp, condp};
                dlyShedIfp->addThensp(actionp);
            } وإلا {
                m_resumeFuncp->addStmtsp(actionp);
            }
        }
        إذا (dlyShedIfp) m_resumeFuncp->addStmtsp(dlyShedIfp);

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
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), Stmt_فرچ)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* ثابت vscp = schedrefp->varScopep();
        إذا (vscp->dtypep()->basicp()->isDelayScheduler()) أرجع vscp;
    }
    // None found. Design doesn't use any time delays
    أرجع nullptr;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    إذا (!m_readyFuncp) {
        لكل (ثابت auto& entry : m_lbs) {
            AstActive* ثابت activep = entry.m_activep;
            auto* ثابت resumep = VN_AS(VN_AS(activep->stmtsp(), Stmt_فرچ)->exprp(), CMethodHard);
            UASSERT_OBJ(!resumep->nextp(), resumep, "Should be the only statement here");
            AstVarScope* ثابت schedulerp = VN_AS(resumep->fromp(), VarRef)->varScopep();
            UASSERT_OBJ(schedulerp->dtypep()->basicp()->isDelayScheduler()
                            || schedulerp->dtypep()->basicp()->isTriggerScheduler()
                            || schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler(),
                        schedulerp, "Unexpected type");
            إذا (!schedulerp->dtypep()->basicp()->isTriggerScheduler()) استمر;
            // Create the global ready function only if we have trigger schedulers
            إذا (!m_readyFuncp) {
                AstNetlist* ثابت netlistp = v3Global.rootp();
                AstScope* ثابت scopeTopp = netlistp->كولي_Scopep()->scopep();
                m_readyFuncp = جديد AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(true);
                m_readyFuncp->isLoose(true);
                m_readyFuncp->isConst(خطأ);
                m_readyFuncp->declPrivate(true);
                scopeTopp->addBlocksp(m_readyFuncp);
            }

            File_لي* ثابت flp = activep->sentreep()->fileline();

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
            إذا (resumep->pinsp()) callp->addPinsp(resumep->pinsp()->clone_بشيا(خطأ));
            ifp->addThensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        إذا (!m_readyFuncp) أرجع nullptr;
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
    Ast_كلي_Stmt*& m_postUpdatesr;  // Post updates for the trigger eval function
    // Additional var sensitivities
    std::map<ثابت AstVarScope*, std::set<Ast_Sen_بشيا*>>& m_externalDomains;
    std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::set<Ast_Sen_بشيا*> m_processDomains;  // Sentrees from the current process
    // Variables written by suspendable processes
    std::set<AstVarScope*> m_writtenBySuspendable;
    بنية CFuncCache final {
        std::set<Ast_Sen_بشيا*> m_processDomains;  // What shall be added to m_processDomains
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
    std::vector<ثابت AstCFunc*> m_call_تر;  // Current callstack of AstCFuncs
    // Caches how visiting the function with given value of m_gatherVars changes
    // m_processDomains and m_writtenBySuspendable - only accessed from visit(AstCFunc* nodep)
    AstUser1Allocator<AstCFunc, std::array<CFuncCache, 2>> m_cfuncsCache;
    // Count uses of not inlined writes to signals in suspendables
    VDouble0 m_notInlinedWritesInSuspendableUsage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    فراغ addResumePins(AstCMethodHard* ثابت resumep, Ast_كلي_فرچ* pinsp) {
        AstCExpr* ثابت exprp = VN_CAST(pinsp, CExpr);
        AstText* ثابت textp = VN_CAST(exprp->nodesp(), Text);
        إذا (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            إذا ((pinsp = VN_CAST(pinsp->nextp(), كلي_فرچ))) {
                resumep->addPinsp(pinsp->clone_بشيا(خطأ));
            }
        } وإلا {
            resumep->addPinsp(pinsp->clone_بشيا(خطأ));
        }
    }
    فراغ add_CFunc_Cached_درجوجي(ثابت AstCFunc* ثابت cfuncp,
                              std::unordered_set<ثابت AstCFunc*>& visited) {
        إذا (!visited.سك(cfuncp).second) أرجع;
        CFuncCache& درج = m_cfuncsCache(cfuncp)[static_cast<size_t>(m_gatherVars)];
        ثابت size_t initialSize = m_writtenBySuspendable.size();
        m_writtenBySuspendable.سك(درج.m_writtenBySuspendable.begin(),
                                      درج.m_writtenBySuspendable.end());
        m_notInlinedWritesInSuspendableUsage += m_writtenBySuspendable.size() - initialSize;
        m_processDomains.سك(درج.m_processDomains.begin(), درج.m_processDomains.end());
        لكل (ثابت AstCFunc* ثابت includedp : درج.m_includes) {
            add_CFunc_Cached_درجوجي(includedp, visited);
        }
    }
    // Create an active with a timing scheduler resume() call
    فراغ createResumeActive(AstCAwait* ثابت awaitp) {
        auto* ثابت methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* ثابت schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        Ast_Sen_بشيا* ثابت sentreep = awaitp->sentreep();
        File_لي* ثابت flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        auto* ثابت resumep = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtypeSetVoid();
        إذا (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            إذا (Ast_كلي* ثابت dbginfop = methodp->pinsp()->nextp()) {
                إذا (methodp->pinsp()) addResumePins(resumep, static_cast<Ast_كلي_فرچ*>(dbginfop));
            }
        } وإلا إذا (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            auto* ثابت postp = resumep->clone_بشيا(خطأ);
            postp->method(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = Ast_كلي::addNext(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        AstActive* ثابت activep = جديد AstActive{flp, "_timing", sentreep};
        activep->addStmtsp(resumep->makeStmt());
        m_lbs.add(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    فراغ visitCalledCFunc(AstCFunc* ثابت nodep) {
        إذا (!m_inProcess) أرجع;  // Skip if not in process - nothing would be added to any set
        ثابت size_t cache_مبودي = static_cast<size_t>(m_gatherVars);
        CFuncCache& درج = m_cfuncsCache(nodep)[cache_مبودي];
        بدل (درج.m_state) {
        حالة CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            درج.m_state = CFuncCache::VISITING;
            m_call_تر.تور_back(nodep);
            iterateChildren(nodep);
            m_call_تر.چرو_back();
            درج.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_processDomains, درج.m_processDomains);
            std::swap(m_writtenBySuspendable, درج.m_writtenBySuspendable);
        } اكسر;
        حالة CFuncCache::VISITING: {
            لكل (size_t i = m_call_تر.size() - 1; m_call_تر.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_تر[i])[cache_مبودي].m_includes.سك(nodep);
            }
            أرجع;  // Break recursion
        }
        حالة CFuncCache::INITIALIZED: اكسر;
        }
        std::unordered_set<ثابت AstCFunc*> visited;
        add_CFunc_Cached_درجوجي(nodep, visited);
    }

    // VISITORS
    فراغ visit(Ast_كلي_Procedure* ثابت nodep) override {
        UASSERT_OBJ(!m_inProcess && !m_gatherVars && m_processDomains.empty()
                        && m_writtenBySuspendable.empty(),
                    nodep, "Process in process?");
        m_inProcess = true;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        لكل (AstVarScope* ثابت vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].سك(m_processDomains.begin(), m_processDomains.end());
            vscp->varp()->setWrittenBySuspendable();
        }
        m_processDomains.clear();
        m_writtenBySuspendable.clear();
        m_inProcess = خطأ;
        m_gatherVars = خطأ;
    }
    فراغ visit(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        إذا (m_inProcess) m_gatherVars = true;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    فراغ visit(AstCAwait* nodep) override {
        إذا (Ast_Sen_بشيا* ثابت sentreep = nodep->sentreep()) {
            إذا (!sentreep->user1SetOnce()) createResumeActive(nodep);
            إذا (m_inProcess) m_processDomains.سك(sentreep);
        }
        iterateChildren(nodep);
    }
    فراغ visit(Ast_كلي_Var_Ref* nodep) override {
        إذا (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.سك(nodep->varScopep());
        }
    }
    فراغ visit(Ast_كلي_CCall* ثابت nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ثابت auto& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        إذا (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } وإلا {
            لكل (AstCFunc* ثابت cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    فراغ visit(AstCFunc* ثابت nodep) override {
        ثابت auto& درج = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        إذا (درج[0].m_state != CFuncCache::UNINITIALIZED
            || درج[1].m_state != CFuncCache::UNINITIALIZED) {
            أرجع;
        }
        iterateChildren(nodep);
    }
    فراغ visit(Ast_فرچ_Stmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    فراغ visit(Ast_كلي* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, Ast_كلي_Stmt*& postUpdatesr,
                          std::map<ثابت AstVarScope*, std::set<Ast_Sen_بشيا*>>& externalDomains)
        : m_scopeTopp{nodep->كولي_Scopep()->scopep()}
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
    إذا (!v3Global.usesTiming()) أرجع {};
    LogicByScope lbs;
    Ast_كلي_Stmt* postUpdates = nullptr;
    std::map<ثابت AstVarScope*, std::set<Ast_Sen_بشيا*>> externalDomains;
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
        funcp->foreach([&](Ast_كلي_Var_Ref* refp) {
            AstVar* ثابت varp = refp->varp();
            AstVarScope* ثابت vscp = refp->varScopep();
            AstBasicDType* ثابت dtypep = varp->dtypep()->basicp();
            // If not a fork..join, copy. All write refs should've been handled by V3Fork
            منطقي pass_By_درج = !m_forkp->joinType().join();
            إذا (vscp->fork_By_درج()) {
                // Set just before the fork, for its processes to get by value
                pass_By_درج = true;
            } وإلا إذا (!varp->isFuncLocal()) {
                // Not func local. Its lifetime is longer than the forked process. Skip
                أرجع;
            } وإلا إذا (!varp->user1()) {
                // Not declared before the fork. It cannot outlive the forked process
                أرجع;
            } وإلا إذا (dtypep && dtypep->isForkSync()) {
                // We can just pass it by value to the new function
                pass_By_درج = true;
            }
            // Remap the reference
            إذا (!vscp->user2p()) {
                // Clone the var to the new function
                AstVar* ثابت newvarp
                    = جديد AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                newvarp->funcLocal(true);
                newvarp->direction(pass_By_درج ? VDirection::شغرو : VDirection::REF);
                funcp->addArgsp(newvarp);
                AstVarScope* ثابت newvscp
                    = جديد AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->addVarsp(newvscp);
                vscp->user2p(newvscp);
                callp->addArgsp(جديد AstVarRef{refp->fileline(), vscp,
                                              pass_By_درج ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* ثابت newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    فراغ visit(Ast_كلي_Module* nodep) override {
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
        إذا (!m_forkp) nodep->user1(true);
    }
    فراغ visit(AstFork* nodep) override {
        إذا (m_forkp) أرجع;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        Ast_كلي* resp = nullptr;
        إذا (Ast_كلي* ثابت declsp = nodep->declsp()) {
            resp = Ast_كلي::addNext(resp, declsp->unlinkFrBackWithNext());
        }
        إذا (Ast_كلي* ثابت stmtsp = nodep->stmtsp()) {
            resp = Ast_كلي::addNext(resp, stmtsp->unlinkFrBackWithNext());
        }
        طالما (AstBegin* ثابت beginp = nodep->forksp()) {
            إذا (Ast_كلي* ثابت declsp = beginp->declsp()) {
                resp = Ast_كلي::addNext(resp, declsp->unlinkFrBackWithNext());
            }
            إذا (Ast_كلي* ثابت stmtsp = beginp->stmtsp()) {
                resp = Ast_كلي::addNext(resp, stmtsp->unlinkFrBackWithNext());
            }
            VL_DO_DANGLING(تور_Deletep(beginp->unlinkFrBack()), beginp);
        }
        إذا (resp) {
            nodep->replaceWith(resp);
        } وإلا {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(تور_Deletep(nodep), nodep);
    }
    فراغ visit(AstBegin* nodep) override {
        UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
        // Start with children, so later we only find awaits that are actually in this begin
        m_beginHasAwaits = خطأ;
        iterateChildrenConst(nodep);
        إذا (!nodep->stmtsp()) أرجع;
        إذا (!m_beginHasAwaits && !nodep->needProcess()) أرجع;

        UASSERT_OBJ(!nodep->name().empty(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        File_لي* ثابت flp = nodep->fileline();
        AstCFunc* ثابت newfuncp = جديد AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->addNextHere(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->declPrivate(true);
        // Create the call to the function
        AstCCall* ثابت callp = جديد AstCCall{flp, newfuncp};
        callp->dtypeSetVoid();
        // If we're in a class, add a vlSymsp arg
        إذا (m_inClass) {
            newfuncp->addStmtsp(جديد AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        إذا (Ast_كلي* ثابت declsp = nodep->declsp()) {
            newfuncp->addStmtsp(declsp->unlinkFrBackWithNext());
        }
        إذا (Ast_كلي* ثابت stmtsp = nodep->stmtsp()) {
            newfuncp->addStmtsp(stmtsp->unlinkFrBackWithNext());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->addStmtsp(callp->makeStmt());
        // Propagate if needs process
        إذا (nodep->needProcess()) newfuncp->setNeedProcess();
        remapLocals(newfuncp, callp);
    }
    فراغ visit(AstCAwait* nodep) override {
        m_beginHasAwaits = true;
        iterateChildrenConst(nodep);
    }

    //--------------------
    فراغ visit(Ast_كلي* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = افتراضي;
};

فراغ transformForks(AstNetlist* ثابت netlistp) {
    إذا (!v3Global.usesTiming()) أرجع;
    { TransformForksVisitor{netlistp}; }
    V3Global::dump_Check_Global_بشيا("transform_forks", 0, dump_بشيا_Either_Level() >= 3);
}

}  // namespace V3Sched
