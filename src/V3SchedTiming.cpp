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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3AstUserAllocator.h"
#अन्तर्भाव "V3ClassGraph.h"
#अन्तर्भाव "V3EmitCBase.h"
#अन्तर्भाव "V3Sched.h"

#अन्तर्भाव <unordered_map>
#अन्तर्भाव <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

नामाकाश V3Sched {

//============================================================================
// Trigger indices of the external domains of each variable

std::map<ध्रुव AstVarScope*, std::vector<uint32_t>>
TimingKit::externalTriggerIndices(ध्रुव TriggerKit& trigKit) ध्रुव {
    std::map<ध्रुव AstVarScope*, std::vector<uint32_t>> result;
    प्रति (ध्रुव auto& vscpDomains : m_externalDomains) {
        std::vector<uint32_t>& indices = result[vscpDomains.first];
        प्रति (ध्रुव Ast_Sen_वृक्षः* ध्रुव domainp : vscpDomains.second) {
            प्रति (ध्रुव AstSenItem* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                indices.प्रक्षिप_back(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    प्रत्यागम result;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    जइ (!m_resumeFuncp) {
        जइ (m_lbs.empty()) प्रत्यागम nullptr;
        // Create global resume function
        AstNetlist* ध्रुव netlistp = v3Global.rootp();
        AstScope* ध्रुव scopeTopp = netlistp->शिखरम्_Scopep()->scopep();
        m_resumeFuncp = नूतन AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(सच्च);
        m_resumeFuncp->isLoose(सच्च);
        m_resumeFuncp->isConst(मिच्छ);
        m_resumeFuncp->declPrivate(सच्च);
        scopeTopp->addBlocksp(m_resumeFuncp);

        प्रति (ध्रुव auto& entry : m_lbs) {
            AstActive* ध्रुव activep = entry.m_activep;
            activep->foreach([एतत्](AstCMethodHard* ध्रुव exprp) {
                जइ (exprp->method() != VCMethod::SCHED_RESUME) प्रत्यागम;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = exprp->fromp();
                जइ (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    प्रत्यागम;
                }
                AstCMethodHard* ध्रुव moveToResumep = नूतन AstCMethodHard{
                    fromp->fileline(), fromp->clone_वृक्षः(मिच्छ),
                    VCMethod::SCHED_MOVE_TO_RESUME_QUEUE,
                    exprp->pinsp() ? exprp->pinsp()->clone_वृक्षः(सच्च) : nullptr};
                moveToResumep->dtypeSetVoid();
                m_resumeFuncp->addStmtsp(moveToResumep->makeStmt());
            });
        }

        // Put all the timing actives in the resume function
        AstIf* dlyShedIfp = nullptr;
        प्रति (ध्रुव auto& entry : m_lbs) {
            AstActive* ध्रुव activep = entry.m_activep;
            // Resume time delays last. For no particular reason other than
            // that's what we used to do prior to proper #0 support.
            AstVarRef* ध्रुव schedrefp = VN_AS(
                VN_AS(VN_AS(activep->stmtsp(), Stmt_व्यञ्जकम्)->exprp(), CMethodHard)->fromp(), VarRef);

            Ast_ग्रन्थिः* ध्रुव actionp = activep->stmtsp()->unlinkFrBackWithNext();
            जइ (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                File_पङ्क्तिः* ध्रुव flp = trigVscp->fileline();
                ध्रुव std::set<uint32_t>& triggers = entry.m_triggers;
                AstTriggered* ध्रुव condp
                    = नूतन AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
                dlyShedIfp = नूतन AstIf{flp, condp};
                dlyShedIfp->addThensp(actionp);
            } अण्णहा {
                m_resumeFuncp->addStmtsp(actionp);
            }
        }
        जइ (dlyShedIfp) m_resumeFuncp->addStmtsp(dlyShedIfp);

        // These are now spent, oispose of now empty AstActive instances
        m_lbs.deleteActives();
    }
    AstCCall* ध्रुव callp = नूतन AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
    callp->dtypeSetVoid();
    प्रत्यागम callp;
}

AstVarScope* TimingKit::getDelayScheduler(AstNetlist* ध्रुव /*netlistp*/) {
    प्रति (ध्रुव auto& entry : m_lbs) {
        AstActive* ध्रुव ap = entry.m_activep;
        // TODO: this triple VN_AS expression is ridiculous
        AstVarRef* ध्रुव schedrefp
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), Stmt_व्यञ्जकम्)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* ध्रुव vscp = schedrefp->varScopep();
        जइ (vscp->dtypep()->basicp()->isDelayScheduler()) प्रत्यागम vscp;
    }
    // None found. Design doesn't use any time delays
    प्रत्यागम nullptr;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    जइ (!m_readyFuncp) {
        प्रति (ध्रुव auto& entry : m_lbs) {
            AstActive* ध्रुव activep = entry.m_activep;
            auto* ध्रुव resumep = VN_AS(VN_AS(activep->stmtsp(), Stmt_व्यञ्जकम्)->exprp(), CMethodHard);
            UASSERT_OBJ(!resumep->nextp(), resumep, "Should be the only statement here");
            AstVarScope* ध्रुव schedulerp = VN_AS(resumep->fromp(), VarRef)->varScopep();
            UASSERT_OBJ(schedulerp->dtypep()->basicp()->isDelayScheduler()
                            || schedulerp->dtypep()->basicp()->isTriggerScheduler()
                            || schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler(),
                        schedulerp, "Unexpected type");
            जइ (!schedulerp->dtypep()->basicp()->isTriggerScheduler()) अनुवर्त;
            // Create the global ready function only if we have trigger schedulers
            जइ (!m_readyFuncp) {
                AstNetlist* ध्रुव netlistp = v3Global.rootp();
                AstScope* ध्रुव scopeTopp = netlistp->शिखरम्_Scopep()->scopep();
                m_readyFuncp = नूतन AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(सच्च);
                m_readyFuncp->isLoose(सच्च);
                m_readyFuncp->isConst(मिच्छ);
                m_readyFuncp->declPrivate(सच्च);
                scopeTopp->addBlocksp(m_readyFuncp);
            }

            File_पङ्क्तिः* ध्रुव flp = activep->sentreep()->fileline();

            // Create an 'AstIf' sensitive to the suspending triggers
            ध्रुव std::set<uint32_t>& triggers = entry.m_triggers;
            AstTriggered* ध्रुव condp
                = नूतन AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
            AstIf* ध्रुव ifp = नूतन AstIf{flp, condp};
            m_readyFuncp->addStmtsp(ifp);

            // Mark as ready the processes resumed on this sensitivity expression
            AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, schedulerp, VAccess::READWRITE};
            AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{flp, refp, VCMethod::SCHED_READY};
            callp->dtypeSetVoid();
            जइ (resumep->pinsp()) callp->addPinsp(resumep->pinsp()->clone_वृक्षः(मिच्छ));
            ifp->addThensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        जइ (!m_readyFuncp) प्रत्यागम nullptr;
    }
    AstCCall* ध्रुव callp = नूतन AstCCall{m_readyFuncp->fileline(), m_readyFuncp};
    callp->dtypeSetVoid();
    प्रत्यागम callp;
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

वर्ग AwaitVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  AstSenTree::user1()  -> bool.               Set true if the sentree has been visited.
    //  AstCFunc::user1()    -> AstUser1Allocator.  See alocator below
    ध्रुव VNUser1InUse m_inuser1;

    // STATE
    तार्किक m_inProcess = मिच्छ;  // Are we in a process?
    तार्किक m_gatherVars = मिच्छ;  // Should we gather vars in m_writtenBySuspendable?
    AstScope* ध्रुव m_scopeTopp;  // Scope at the top
    LogicByScope& m_lbs;  // Timing resume actives
    Ast_ग्रन्थिः_Stmt*& m_postUpdatesr;  // Post updates for the trigger eval function
    // Additional var sensitivities
    std::map<ध्रुव AstVarScope*, std::set<Ast_Sen_वृक्षः*>>& m_externalDomains;
    std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::set<Ast_Sen_वृक्षः*> m_processDomains;  // Sentrees from the current process
    // Variables written by suspendable processes
    std::set<AstVarScope*> m_writtenBySuspendable;
    संरचना CFuncCache final {
        std::set<Ast_Sen_वृक्षः*> m_processDomains;  // What shall be added to m_processDomains
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
        गणना State : uint8_t {
            UNINITIALIZED = 0,  // Not initialized members are empty
            VISITING,  // Visiting - needed for breaking recursion
            INITIALIZED,  // Members contains correct values
        } m_state  // Current state of Cache
            = UNINITIALIZED;
    };
    std::vector<ध्रुव AstCFunc*> m_call_राशिः;  // Current callstack of AstCFuncs
    // Caches how visiting the function with given value of m_gatherVars changes
    // m_processDomains and m_writtenBySuspendable - only accessed from visit(AstCFunc* nodep)
    AstUser1Allocator<AstCFunc, std::array<CFuncCache, 2>> m_cfuncsCache;
    // Count uses of not inlined writes to signals in suspendables
    VDouble0 m_notInlinedWritesInSuspendableUsage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    शून्य addResumePins(AstCMethodHard* ध्रुव resumep, Ast_ग्रन्थिः_व्यञ्जकम्* pinsp) {
        AstCExpr* ध्रुव exprp = VN_CAST(pinsp, CExpr);
        AstText* ध्रुव textp = VN_CAST(exprp->nodesp(), Text);
        जइ (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            जइ ((pinsp = VN_CAST(pinsp->nextp(), ग्रन्थिः_व्यञ्जकम्))) {
                resumep->addPinsp(pinsp->clone_वृक्षः(मिच्छ));
            }
        } अण्णहा {
            resumep->addPinsp(pinsp->clone_वृक्षः(मिच्छ));
        }
    }
    शून्य add_CFunc_Cached_मानानि(ध्रुव AstCFunc* ध्रुव cfuncp,
                              std::unordered_set<ध्रुव AstCFunc*>& visited) {
        जइ (!visited.निवेशय(cfuncp).second) प्रत्यागम;
        CFuncCache& मानम् = m_cfuncsCache(cfuncp)[static_cast<size_t>(m_gatherVars)];
        ध्रुव size_t initialSize = m_writtenBySuspendable.size();
        m_writtenBySuspendable.निवेशय(मानम्.m_writtenBySuspendable.begin(),
                                      मानम्.m_writtenBySuspendable.end());
        m_notInlinedWritesInSuspendableUsage += m_writtenBySuspendable.size() - initialSize;
        m_processDomains.निवेशय(मानम्.m_processDomains.begin(), मानम्.m_processDomains.end());
        प्रति (ध्रुव AstCFunc* ध्रुव includedp : मानम्.m_includes) {
            add_CFunc_Cached_मानानि(includedp, visited);
        }
    }
    // Create an active with a timing scheduler resume() call
    शून्य createResumeActive(AstCAwait* ध्रुव awaitp) {
        auto* ध्रुव methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* ध्रुव schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        Ast_Sen_वृक्षः* ध्रुव sentreep = awaitp->sentreep();
        File_पङ्क्तिः* ध्रुव flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        auto* ध्रुव resumep = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtypeSetVoid();
        जइ (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            जइ (Ast_ग्रन्थिः* ध्रुव dbginfop = methodp->pinsp()->nextp()) {
                जइ (methodp->pinsp()) addResumePins(resumep, static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(dbginfop));
            }
        } अण्णहा जइ (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            auto* ध्रुव postp = resumep->clone_वृक्षः(मिच्छ);
            postp->method(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = Ast_ग्रन्थिः::addNext(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        AstActive* ध्रुव activep = नूतन AstActive{flp, "_timing", sentreep};
        activep->addStmtsp(resumep->makeStmt());
        m_lbs.add(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    शून्य visitCalledCFunc(AstCFunc* ध्रुव nodep) {
        जइ (!m_inProcess) प्रत्यागम;  // Skip if not in process - nothing would be added to any set
        ध्रुव size_t cache_कुञ्चिका = static_cast<size_t>(m_gatherVars);
        CFuncCache& मानम् = m_cfuncsCache(nodep)[cache_कुञ्चिका];
        विकल्प (मानम्.m_state) {
        पक्ष CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            मानम्.m_state = CFuncCache::VISITING;
            m_call_राशिः.प्रक्षिप_back(nodep);
            iterateChildren(nodep);
            m_call_राशिः.निष्कासय_back();
            मानम्.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_processDomains, मानम्.m_processDomains);
            std::swap(m_writtenBySuspendable, मानम्.m_writtenBySuspendable);
        } भङ्ग;
        पक्ष CFuncCache::VISITING: {
            प्रति (size_t i = m_call_राशिः.size() - 1; m_call_राशिः.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_राशिः[i])[cache_कुञ्चिका].m_includes.निवेशय(nodep);
            }
            प्रत्यागम;  // Break recursion
        }
        पक्ष CFuncCache::INITIALIZED: भङ्ग;
        }
        std::unordered_set<ध्रुव AstCFunc*> visited;
        add_CFunc_Cached_मानानि(nodep, visited);
    }

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_Procedure* ध्रुव nodep) override {
        UASSERT_OBJ(!m_inProcess && !m_gatherVars && m_processDomains.empty()
                        && m_writtenBySuspendable.empty(),
                    nodep, "Process in process?");
        m_inProcess = सच्च;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        प्रति (AstVarScope* ध्रुव vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].निवेशय(m_processDomains.begin(), m_processDomains.end());
            vscp->varp()->setWrittenBySuspendable();
        }
        m_processDomains.clear();
        m_writtenBySuspendable.clear();
        m_inProcess = मिच्छ;
        m_gatherVars = मिच्छ;
    }
    शून्य visit(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        जइ (m_inProcess) m_gatherVars = सच्च;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    शून्य visit(AstCAwait* nodep) override {
        जइ (Ast_Sen_वृक्षः* ध्रुव sentreep = nodep->sentreep()) {
            जइ (!sentreep->user1SetOnce()) createResumeActive(nodep);
            जइ (m_inProcess) m_processDomains.निवेशय(sentreep);
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        जइ (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.निवेशय(nodep->varScopep());
        }
    }
    शून्य visit(Ast_ग्रन्थिः_CCall* ध्रुव nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ध्रुव auto& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        जइ (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } अण्णहा {
            प्रति (AstCFunc* ध्रुव cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    शून्य visit(AstCFunc* ध्रुव nodep) override {
        ध्रुव auto& मानम् = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        जइ (मानम्[0].m_state != CFuncCache::UNINITIALIZED
            || मानम्[1].m_state != CFuncCache::UNINITIALIZED) {
            प्रत्यागम;
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_व्यञ्जकम्_Stmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, Ast_ग्रन्थिः_Stmt*& postUpdatesr,
                          std::map<ध्रुव AstVarScope*, std::set<Ast_Sen_वृक्षः*>>& externalDomains)
        : m_scopeTopp{nodep->शिखरम्_Scopep()->scopep()}
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

TimingKit prepareTiming(AstNetlist* ध्रुव netlistp) {
    जइ (!v3Global.usesTiming()) प्रत्यागम {};
    LogicByScope lbs;
    Ast_ग्रन्थिः_Stmt* postUpdates = nullptr;
    std::map<ध्रुव AstVarScope*, std::set<Ast_Sen_वृक्षः*>> externalDomains;
    { AwaitVisitor{netlistp, lbs, postUpdates, externalDomains}; }
    प्रत्यागम {std::move(lbs), postUpdates, std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

// Transform all forked processes into functions
वर्ग TransformForksVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current fork.
    ध्रुव VNUser1InUse m_inuser1;

    // STATE
    तार्किक m_inClass = मिच्छ;  // Are we in a class?
    तार्किक m_beginHasAwaits = मिच्छ;  // Does the current begin have awaits?
    AstFork* m_forkp = nullptr;  // Current fork
    AstCFunc* m_funcp = nullptr;  // Current function

    // METHODS
    // Remap local vars referenced by the given fork function
    // TODO: We should only pass variables to the fork that are
    // live in the fork body, but for that we need a proper data
    // flow analysis framework which we don't have at the moment
    शून्य remapLocals(AstCFunc* ध्रुव funcp, AstCCall* ध्रुव callp) {
        ध्रुव VNUser2InUse user2InUse;  // AstVarScope -> AstVarScope: var to remap to
        funcp->foreach([&](Ast_ग्रन्थिः_Var_Ref* refp) {
            AstVar* ध्रुव varp = refp->varp();
            AstVarScope* ध्रुव vscp = refp->varScopep();
            AstBasicDType* ध्रुव dtypep = varp->dtypep()->basicp();
            // If not a fork..join, copy. All write refs should've been handled by V3Fork
            तार्किक pass_By_मानम् = !m_forkp->joinType().join();
            जइ (vscp->fork_By_मानम्()) {
                // Set just before the fork, for its processes to get by value
                pass_By_मानम् = सच्च;
            } अण्णहा जइ (!varp->isFuncLocal()) {
                // Not func local. Its lifetime is longer than the forked process. Skip
                प्रत्यागम;
            } अण्णहा जइ (!varp->user1()) {
                // Not declared before the fork. It cannot outlive the forked process
                प्रत्यागम;
            } अण्णहा जइ (dtypep && dtypep->isForkSync()) {
                // We can just pass it by value to the new function
                pass_By_मानम् = सच्च;
            }
            // Remap the reference
            जइ (!vscp->user2p()) {
                // Clone the var to the new function
                AstVar* ध्रुव newvarp
                    = नूतन AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                newvarp->funcLocal(सच्च);
                newvarp->direction(pass_By_मानम् ? VDirection::निवेशः : VDirection::REF);
                funcp->addArgsp(newvarp);
                AstVarScope* ध्रुव newvscp
                    = नूतन AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->addVarsp(newvscp);
                vscp->user2p(newvscp);
                callp->addArgsp(नूतन AstVarRef{refp->fileline(), vscp,
                                              pass_By_मानम् ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* ध्रुव newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_inClass);
        m_inClass = VN_IS(nodep, Class);
        iterateChildren(nodep);
    }
    शून्य visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        m_funcp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstVar* nodep) override {
        जइ (!m_forkp) nodep->user1(सच्च);
    }
    शून्य visit(AstFork* nodep) override {
        जइ (m_forkp) प्रत्यागम;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        Ast_ग्रन्थिः* resp = nullptr;
        जइ (Ast_ग्रन्थिः* ध्रुव declsp = nodep->declsp()) {
            resp = Ast_ग्रन्थिः::addNext(resp, declsp->unlinkFrBackWithNext());
        }
        जइ (Ast_ग्रन्थिः* ध्रुव stmtsp = nodep->stmtsp()) {
            resp = Ast_ग्रन्थिः::addNext(resp, stmtsp->unlinkFrBackWithNext());
        }
        जाव (AstBegin* ध्रुव beginp = nodep->forksp()) {
            जइ (Ast_ग्रन्थिः* ध्रुव declsp = beginp->declsp()) {
                resp = Ast_ग्रन्थिः::addNext(resp, declsp->unlinkFrBackWithNext());
            }
            जइ (Ast_ग्रन्थिः* ध्रुव stmtsp = beginp->stmtsp()) {
                resp = Ast_ग्रन्थिः::addNext(resp, stmtsp->unlinkFrBackWithNext());
            }
            VL_DO_DANGLING(प्रक्षिप_Deletep(beginp->unlinkFrBack()), beginp);
        }
        जइ (resp) {
            nodep->replaceWith(resp);
        } अण्णहा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstBegin* nodep) override {
        UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
        // Start with children, so later we only find awaits that are actually in this begin
        m_beginHasAwaits = मिच्छ;
        iterateChildrenConst(nodep);
        जइ (!nodep->stmtsp()) प्रत्यागम;
        जइ (!m_beginHasAwaits && !nodep->needProcess()) प्रत्यागम;

        UASSERT_OBJ(!nodep->name().empty(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        AstCFunc* ध्रुव newfuncp = नूतन AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->addNextHere(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->declPrivate(सच्च);
        // Create the call to the function
        AstCCall* ध्रुव callp = नूतन AstCCall{flp, newfuncp};
        callp->dtypeSetVoid();
        // If we're in a class, add a vlSymsp arg
        जइ (m_inClass) {
            newfuncp->addStmtsp(नूतन AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        जइ (Ast_ग्रन्थिः* ध्रुव declsp = nodep->declsp()) {
            newfuncp->addStmtsp(declsp->unlinkFrBackWithNext());
        }
        जइ (Ast_ग्रन्थिः* ध्रुव stmtsp = nodep->stmtsp()) {
            newfuncp->addStmtsp(stmtsp->unlinkFrBackWithNext());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->addStmtsp(callp->makeStmt());
        // Propagate if needs process
        जइ (nodep->needProcess()) newfuncp->setNeedProcess();
        remapLocals(newfuncp, callp);
    }
    शून्य visit(AstCAwait* nodep) override {
        m_beginHasAwaits = सच्च;
        iterateChildrenConst(nodep);
    }

    //--------------------
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = स्वभाव;
};

शून्य transformForks(AstNetlist* ध्रुव netlistp) {
    जइ (!v3Global.usesTiming()) प्रत्यागम;
    { TransformForksVisitor{netlistp}; }
    V3Global::dump_Check_Global_वृक्षः("transform_forks", 0, dump_वृक्षः_Either_Level() >= 3);
}

}  // namespace V3Sched
