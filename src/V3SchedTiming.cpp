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

std::map<अचर AstVarScope*, std::vector<uint32_t>>
TimingKit::externalTriggerIndices(अचर TriggerKit& trigKit) अचर {
    std::map<अचर AstVarScope*, std::vector<uint32_t>> result;
    लागि (अचर स्वतः& vscpDomains : m_externalDomains) {
        std::vector<uint32_t>& indices = result[vscpDomains.first];
        लागि (अचर Ast_Sen_रूख* अचर domainp : vscpDomains.second) {
            लागि (अचर AstSenItem* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                indices.थप_back(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    फर्काऊ result;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    यदि (!m_resumeFuncp) {
        यदि (m_lbs.empty()) फर्काऊ nullptr;
        // Create global resume function
        AstNetlist* अचर netlistp = v3Global.rootp();
        AstScope* अचर scopeTopp = netlistp->शीर्ष_Scopep()->scopep();
        m_resumeFuncp = नूतन AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(सत्य);
        m_resumeFuncp->isLoose(सत्य);
        m_resumeFuncp->isConst(असत्य);
        m_resumeFuncp->declPrivate(सत्य);
        scopeTopp->addBlocksp(m_resumeFuncp);

        लागि (अचर स्वतः& entry : m_lbs) {
            AstActive* अचर activep = entry.m_activep;
            activep->foreach([एतत्](AstCMethodHard* अचर exprp) {
                यदि (exprp->method() != VCMethod::SCHED_RESUME) फर्काऊ;
                Ast_गाँठो_अभिव्यक्ति* अचर fromp = exprp->fromp();
                यदि (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    फर्काऊ;
                }
                AstCMethodHard* अचर moveToResumep = नूतन AstCMethodHard{
                    fromp->fileline(), fromp->clone_रूख(असत्य),
                    VCMethod::SCHED_MOVE_TO_RESUME_QUEUE,
                    exprp->pinsp() ? exprp->pinsp()->clone_रूख(सत्य) : nullptr};
                moveToResumep->dtypeSetVoid();
                m_resumeFuncp->addStmtsp(moveToResumep->makeStmt());
            });
        }

        // Put all the timing actives in the resume function
        AstIf* dlyShedIfp = nullptr;
        लागि (अचर स्वतः& entry : m_lbs) {
            AstActive* अचर activep = entry.m_activep;
            // Resume time delays last. For no particular reason other than
            // that's what we used to do prior to proper #0 support.
            AstVarRef* अचर schedrefp = VN_AS(
                VN_AS(VN_AS(activep->stmtsp(), Stmt_अभिव्यक्ति)->exprp(), CMethodHard)->fromp(), VarRef);

            Ast_गाँठो* अचर actionp = activep->stmtsp()->unlinkFrBackWithNext();
            यदि (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                File_पङ्क्ति* अचर flp = trigVscp->fileline();
                अचर std::set<uint32_t>& triggers = entry.m_triggers;
                AstTriggered* अचर condp
                    = नूतन AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
                dlyShedIfp = नूतन AstIf{flp, condp};
                dlyShedIfp->addThensp(actionp);
            } अन्यथा {
                m_resumeFuncp->addStmtsp(actionp);
            }
        }
        यदि (dlyShedIfp) m_resumeFuncp->addStmtsp(dlyShedIfp);

        // These are now spent, oispose of now empty AstActive instances
        m_lbs.deleteActives();
    }
    AstCCall* अचर callp = नूतन AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
    callp->dtypeSetVoid();
    फर्काऊ callp;
}

AstVarScope* TimingKit::getDelayScheduler(AstNetlist* अचर /*netlistp*/) {
    लागि (अचर स्वतः& entry : m_lbs) {
        AstActive* अचर ap = entry.m_activep;
        // TODO: this triple VN_AS expression is ridiculous
        AstVarRef* अचर schedrefp
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), Stmt_अभिव्यक्ति)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* अचर vscp = schedrefp->varScopep();
        यदि (vscp->dtypep()->basicp()->isDelayScheduler()) फर्काऊ vscp;
    }
    // None found. Design doesn't use any time delays
    फर्काऊ nullptr;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    यदि (!m_readyFuncp) {
        लागि (अचर स्वतः& entry : m_lbs) {
            AstActive* अचर activep = entry.m_activep;
            स्वतः* अचर resumep = VN_AS(VN_AS(activep->stmtsp(), Stmt_अभिव्यक्ति)->exprp(), CMethodHard);
            UASSERT_OBJ(!resumep->nextp(), resumep, "Should be the only statement here");
            AstVarScope* अचर schedulerp = VN_AS(resumep->fromp(), VarRef)->varScopep();
            UASSERT_OBJ(schedulerp->dtypep()->basicp()->isDelayScheduler()
                            || schedulerp->dtypep()->basicp()->isTriggerScheduler()
                            || schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler(),
                        schedulerp, "Unexpected type");
            यदि (!schedulerp->dtypep()->basicp()->isTriggerScheduler()) जारी;
            // Create the global ready function only if we have trigger schedulers
            यदि (!m_readyFuncp) {
                AstNetlist* अचर netlistp = v3Global.rootp();
                AstScope* अचर scopeTopp = netlistp->शीर्ष_Scopep()->scopep();
                m_readyFuncp = नूतन AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(सत्य);
                m_readyFuncp->isLoose(सत्य);
                m_readyFuncp->isConst(असत्य);
                m_readyFuncp->declPrivate(सत्य);
                scopeTopp->addBlocksp(m_readyFuncp);
            }

            File_पङ्क्ति* अचर flp = activep->sentreep()->fileline();

            // Create an 'AstIf' sensitive to the suspending triggers
            अचर std::set<uint32_t>& triggers = entry.m_triggers;
            AstTriggered* अचर condp
                = नूतन AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
            AstIf* अचर ifp = नूतन AstIf{flp, condp};
            m_readyFuncp->addStmtsp(ifp);

            // Mark as ready the processes resumed on this sensitivity expression
            AstVarRef* अचर refp = नूतन AstVarRef{flp, schedulerp, VAccess::READWRITE};
            AstCMethodHard* अचर callp = नूतन AstCMethodHard{flp, refp, VCMethod::SCHED_READY};
            callp->dtypeSetVoid();
            यदि (resumep->pinsp()) callp->addPinsp(resumep->pinsp()->clone_रूख(असत्य));
            ifp->addThensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        यदि (!m_readyFuncp) फर्काऊ nullptr;
    }
    AstCCall* अचर callp = नूतन AstCCall{m_readyFuncp->fileline(), m_readyFuncp};
    callp->dtypeSetVoid();
    फर्काऊ callp;
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

वर्ग AwaitVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  AstSenTree::user1()  -> bool.               Set true if the sentree has been visited.
    //  AstCFunc::user1()    -> AstUser1Allocator.  See alocator below
    अचर VNUser1InUse m_inuser1;

    // STATE
    तार्किक m_inProcess = असत्य;  // Are we in a process?
    तार्किक m_gatherVars = असत्य;  // Should we gather vars in m_writtenBySuspendable?
    AstScope* अचर m_scopeTopp;  // Scope at the top
    LogicByScope& m_lbs;  // Timing resume actives
    Ast_गाँठो_Stmt*& m_postUpdatesr;  // Post updates for the trigger eval function
    // Additional var sensitivities
    std::map<अचर AstVarScope*, std::set<Ast_Sen_रूख*>>& m_externalDomains;
    std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::set<Ast_Sen_रूख*> m_processDomains;  // Sentrees from the current process
    // Variables written by suspendable processes
    std::set<AstVarScope*> m_writtenBySuspendable;
    संरचना CFuncCache final {
        std::set<Ast_Sen_रूख*> m_processDomains;  // What shall be added to m_processDomains
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
    std::vector<अचर AstCFunc*> m_call_थाक;  // Current callstack of AstCFuncs
    // Caches how visiting the function with given value of m_gatherVars changes
    // m_processDomains and m_writtenBySuspendable - only accessed from visit(AstCFunc* nodep)
    AstUser1Allocator<AstCFunc, std::array<CFuncCache, 2>> m_cfuncsCache;
    // Count uses of not inlined writes to signals in suspendables
    VDouble0 m_notInlinedWritesInSuspendableUsage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    शून्य addResumePins(AstCMethodHard* अचर resumep, Ast_गाँठो_अभिव्यक्ति* pinsp) {
        AstCExpr* अचर exprp = VN_CAST(pinsp, CExpr);
        AstText* अचर textp = VN_CAST(exprp->nodesp(), Text);
        यदि (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            यदि ((pinsp = VN_CAST(pinsp->nextp(), गाँठो_अभिव्यक्ति))) {
                resumep->addPinsp(pinsp->clone_रूख(असत्य));
            }
        } अन्यथा {
            resumep->addPinsp(pinsp->clone_रूख(असत्य));
        }
    }
    शून्य add_CFunc_Cached_मानहरू(अचर AstCFunc* अचर cfuncp,
                              std::unordered_set<अचर AstCFunc*>& visited) {
        यदि (!visited.घुसाउ(cfuncp).second) फर्काऊ;
        CFuncCache& मान = m_cfuncsCache(cfuncp)[static_cast<माप_प्रकार>(m_gatherVars)];
        अचर माप_प्रकार initialSize = m_writtenBySuspendable.size();
        m_writtenBySuspendable.घुसाउ(मान.m_writtenBySuspendable.begin(),
                                      मान.m_writtenBySuspendable.end());
        m_notInlinedWritesInSuspendableUsage += m_writtenBySuspendable.size() - initialSize;
        m_processDomains.घुसाउ(मान.m_processDomains.begin(), मान.m_processDomains.end());
        लागि (अचर AstCFunc* अचर includedp : मान.m_includes) {
            add_CFunc_Cached_मानहरू(includedp, visited);
        }
    }
    // Create an active with a timing scheduler resume() call
    शून्य createResumeActive(AstCAwait* अचर awaitp) {
        स्वतः* अचर methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* अचर schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        Ast_Sen_रूख* अचर sentreep = awaitp->sentreep();
        File_पङ्क्ति* अचर flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        स्वतः* अचर resumep = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtypeSetVoid();
        यदि (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            यदि (Ast_गाँठो* अचर dbginfop = methodp->pinsp()->nextp()) {
                यदि (methodp->pinsp()) addResumePins(resumep, static_cast<Ast_गाँठो_अभिव्यक्ति*>(dbginfop));
            }
        } अन्यथा यदि (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            स्वतः* अचर postp = resumep->clone_रूख(असत्य);
            postp->method(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = Ast_गाँठो::addNext(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        AstActive* अचर activep = नूतन AstActive{flp, "_timing", sentreep};
        activep->addStmtsp(resumep->makeStmt());
        m_lbs.add(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    शून्य visitCalledCFunc(AstCFunc* अचर nodep) {
        यदि (!m_inProcess) फर्काऊ;  // Skip if not in process - nothing would be added to any set
        अचर माप_प्रकार cache_साँचो = static_cast<माप_प्रकार>(m_gatherVars);
        CFuncCache& मान = m_cfuncsCache(nodep)[cache_साँचो];
        विकल्प (मान.m_state) {
        पक्ष CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            मान.m_state = CFuncCache::VISITING;
            m_call_थाक.थप_back(nodep);
            iterateChildren(nodep);
            m_call_थाक.झिक_back();
            मान.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_processDomains, मान.m_processDomains);
            std::swap(m_writtenBySuspendable, मान.m_writtenBySuspendable);
        } रोक;
        पक्ष CFuncCache::VISITING: {
            लागि (माप_प्रकार i = m_call_थाक.size() - 1; m_call_थाक.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_थाक[i])[cache_साँचो].m_includes.घुसाउ(nodep);
            }
            फर्काऊ;  // Break recursion
        }
        पक्ष CFuncCache::INITIALIZED: रोक;
        }
        std::unordered_set<अचर AstCFunc*> visited;
        add_CFunc_Cached_मानहरू(nodep, visited);
    }

    // VISITORS
    शून्य visit(Ast_गाँठो_Procedure* अचर nodep) override {
        UASSERT_OBJ(!m_inProcess && !m_gatherVars && m_processDomains.empty()
                        && m_writtenBySuspendable.empty(),
                    nodep, "Process in process?");
        m_inProcess = सत्य;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        लागि (AstVarScope* अचर vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].घुसाउ(m_processDomains.begin(), m_processDomains.end());
            vscp->varp()->setWrittenBySuspendable();
        }
        m_processDomains.clear();
        m_writtenBySuspendable.clear();
        m_inProcess = असत्य;
        m_gatherVars = असत्य;
    }
    शून्य visit(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        यदि (m_inProcess) m_gatherVars = सत्य;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    शून्य visit(AstCAwait* nodep) override {
        यदि (Ast_Sen_रूख* अचर sentreep = nodep->sentreep()) {
            यदि (!sentreep->user1SetOnce()) createResumeActive(nodep);
            यदि (m_inProcess) m_processDomains.घुसाउ(sentreep);
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठो_Var_Ref* nodep) override {
        यदि (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.घुसाउ(nodep->varScopep());
        }
    }
    शून्य visit(Ast_गाँठो_CCall* अचर nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        अचर स्वतः& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        यदि (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } अन्यथा {
            लागि (AstCFunc* अचर cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    शून्य visit(AstCFunc* अचर nodep) override {
        अचर स्वतः& मान = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        यदि (मान[0].m_state != CFuncCache::UNINITIALIZED
            || मान[1].m_state != CFuncCache::UNINITIALIZED) {
            फर्काऊ;
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_अभिव्यक्ति_Stmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, Ast_गाँठो_Stmt*& postUpdatesr,
                          std::map<अचर AstVarScope*, std::set<Ast_Sen_रूख*>>& externalDomains)
        : m_scopeTopp{nodep->शीर्ष_Scopep()->scopep()}
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

TimingKit prepareTiming(AstNetlist* अचर netlistp) {
    यदि (!v3Global.usesTiming()) फर्काऊ {};
    LogicByScope lbs;
    Ast_गाँठो_Stmt* postUpdates = nullptr;
    std::map<अचर AstVarScope*, std::set<Ast_Sen_रूख*>> externalDomains;
    { AwaitVisitor{netlistp, lbs, postUpdates, externalDomains}; }
    फर्काऊ {std::move(lbs), postUpdates, std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

// Transform all forked processes into functions
वर्ग TransformForksVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current fork.
    अचर VNUser1InUse m_inuser1;

    // STATE
    तार्किक m_inClass = असत्य;  // Are we in a class?
    तार्किक m_beginHasAwaits = असत्य;  // Does the current begin have awaits?
    AstFork* m_forkp = nullptr;  // Current fork
    AstCFunc* m_funcp = nullptr;  // Current function

    // METHODS
    // Remap local vars referenced by the given fork function
    // TODO: We should only pass variables to the fork that are
    // live in the fork body, but for that we need a proper data
    // flow analysis framework which we don't have at the moment
    शून्य remapLocals(AstCFunc* अचर funcp, AstCCall* अचर callp) {
        अचर VNUser2InUse user2InUse;  // AstVarScope -> AstVarScope: var to remap to
        funcp->foreach([&](Ast_गाँठो_Var_Ref* refp) {
            AstVar* अचर varp = refp->varp();
            AstVarScope* अचर vscp = refp->varScopep();
            AstBasicDType* अचर dtypep = varp->dtypep()->basicp();
            // If not a fork..join, copy. All write refs should've been handled by V3Fork
            तार्किक pass_By_मान = !m_forkp->joinType().join();
            यदि (vscp->fork_By_मान()) {
                // Set just before the fork, for its processes to get by value
                pass_By_मान = सत्य;
            } अन्यथा यदि (!varp->isFuncLocal()) {
                // Not func local. Its lifetime is longer than the forked process. Skip
                फर्काऊ;
            } अन्यथा यदि (!varp->user1()) {
                // Not declared before the fork. It cannot outlive the forked process
                फर्काऊ;
            } अन्यथा यदि (dtypep && dtypep->isForkSync()) {
                // We can just pass it by value to the new function
                pass_By_मान = सत्य;
            }
            // Remap the reference
            यदि (!vscp->user2p()) {
                // Clone the var to the new function
                AstVar* अचर newvarp
                    = नूतन AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                newvarp->funcLocal(सत्य);
                newvarp->direction(pass_By_मान ? VDirection::आगत : VDirection::REF);
                funcp->addArgsp(newvarp);
                AstVarScope* अचर newvscp
                    = नूतन AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->addVarsp(newvscp);
                vscp->user2p(newvscp);
                callp->addArgsp(नूतन AstVarRef{refp->fileline(), vscp,
                                              pass_By_मान ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* अचर newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    शून्य visit(Ast_गाँठो_Module* nodep) override {
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
        यदि (!m_forkp) nodep->user1(सत्य);
    }
    शून्य visit(AstFork* nodep) override {
        यदि (m_forkp) फर्काऊ;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        Ast_गाँठो* resp = nullptr;
        यदि (Ast_गाँठो* अचर declsp = nodep->declsp()) {
            resp = Ast_गाँठो::addNext(resp, declsp->unlinkFrBackWithNext());
        }
        यदि (Ast_गाँठो* अचर stmtsp = nodep->stmtsp()) {
            resp = Ast_गाँठो::addNext(resp, stmtsp->unlinkFrBackWithNext());
        }
        जबसम्म (AstBegin* अचर beginp = nodep->forksp()) {
            यदि (Ast_गाँठो* अचर declsp = beginp->declsp()) {
                resp = Ast_गाँठो::addNext(resp, declsp->unlinkFrBackWithNext());
            }
            यदि (Ast_गाँठो* अचर stmtsp = beginp->stmtsp()) {
                resp = Ast_गाँठो::addNext(resp, stmtsp->unlinkFrBackWithNext());
            }
            VL_DO_DANGLING(थप_Deletep(beginp->unlinkFrBack()), beginp);
        }
        यदि (resp) {
            nodep->replaceWith(resp);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(AstBegin* nodep) override {
        UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
        // Start with children, so later we only find awaits that are actually in this begin
        m_beginHasAwaits = असत्य;
        iterateChildrenConst(nodep);
        यदि (!nodep->stmtsp()) फर्काऊ;
        यदि (!m_beginHasAwaits && !nodep->needProcess()) फर्काऊ;

        UASSERT_OBJ(!nodep->name().empty(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        File_पङ्क्ति* अचर flp = nodep->fileline();
        AstCFunc* अचर newfuncp = नूतन AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->addNextHere(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->declPrivate(सत्य);
        // Create the call to the function
        AstCCall* अचर callp = नूतन AstCCall{flp, newfuncp};
        callp->dtypeSetVoid();
        // If we're in a class, add a vlSymsp arg
        यदि (m_inClass) {
            newfuncp->addStmtsp(नूतन AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        यदि (Ast_गाँठो* अचर declsp = nodep->declsp()) {
            newfuncp->addStmtsp(declsp->unlinkFrBackWithNext());
        }
        यदि (Ast_गाँठो* अचर stmtsp = nodep->stmtsp()) {
            newfuncp->addStmtsp(stmtsp->unlinkFrBackWithNext());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->addStmtsp(callp->makeStmt());
        // Propagate if needs process
        यदि (nodep->needProcess()) newfuncp->setNeedProcess();
        remapLocals(newfuncp, callp);
    }
    शून्य visit(AstCAwait* nodep) override {
        m_beginHasAwaits = सत्य;
        iterateChildrenConst(nodep);
    }

    //--------------------
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = स्वभाव;
};

शून्य transformForks(AstNetlist* अचर netlistp) {
    यदि (!v3Global.usesTiming()) फर्काऊ;
    { TransformForksVisitor{netlistp}; }
    V3Global::dump_Check_Global_रूख("transform_forks", 0, dump_रूख_Either_Level() >= 3);
}

}  // namespace V3Sched
