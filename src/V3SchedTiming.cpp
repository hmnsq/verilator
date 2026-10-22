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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3AstUserAllocator.h"
#समावेश "V3ClassGraph.h"
#समावेश "V3EmitCBase.h"
#समावेश "V3Sched.h"

#समावेश <unordered_map>
#समावेश <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

namespace V3Sched {

//============================================================================
// Trigger indices of the external domains of each variable

std::map<स्थिर AstVarScope*, std::vector<uint32_t>>
TimingKit::externalTriggerIndices(स्थिर TriggerKit& trigKit) स्थिर {
    std::map<स्थिर AstVarScope*, std::vector<uint32_t>> परिणाम;
    क्रम (स्थिर स्वतः& vscpDomains : m_externalDomains) {
        std::vector<uint32_t>& indices = परिणाम[vscpDomains.first];
        क्रम (स्थिर Ast_Sen_वृक्ष* स्थिर domainp : vscpDomains.second) {
            क्रम (स्थिर AstSenItem* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                indices.धकेलो_back(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    वापस परिणाम;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    अगर (!m_resumeFuncp) {
        अगर (m_lbs.empty()) वापस nullptr;
        // Create global resume function
        AstNetlist* स्थिर netlistp = v3Global.rootp();
        AstScope* स्थिर scopeTopp = netlistp->शीर्ष_Scopep()->scopep();
        m_resumeFuncp = नया AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(true);
        m_resumeFuncp->isLoose(true);
        m_resumeFuncp->isConst(false);
        m_resumeFuncp->declPrivate(true);
        scopeTopp->addBlocksp(m_resumeFuncp);

        क्रम (स्थिर स्वतः& entry : m_lbs) {
            AstActive* स्थिर activep = entry.m_activep;
            activep->foreach([यह](AstCMethodHard* स्थिर exprp) {
                अगर (exprp->method() != VCMethod::SCHED_RESUME) वापस;
                Ast_गाँठ_व्यंजक* स्थिर fromp = exprp->fromp();
                अगर (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    वापस;
                }
                AstCMethodHard* स्थिर moveToResumep = नया AstCMethodHard{
                    fromp->fileline(), fromp->clone_वृक्ष(false),
                    VCMethod::SCHED_MOVE_TO_RESUME_QUEUE,
                    exprp->pinsp() ? exprp->pinsp()->clone_वृक्ष(true) : nullptr};
                moveToResumep->dtypeSetVoid();
                m_resumeFuncp->addStmtsp(moveToResumep->makeStmt());
            });
        }

        // Put all the timing actives in the resume function
        AstIf* dlyShedIfp = nullptr;
        क्रम (स्थिर स्वतः& entry : m_lbs) {
            AstActive* स्थिर activep = entry.m_activep;
            // Resume time delays last. For no particular reason other than
            // that's what we used to do prior to proper #0 support.
            AstVarRef* स्थिर schedrefp = VN_AS(
                VN_AS(VN_AS(activep->stmtsp(), Stmt_व्यंजक)->exprp(), CMethodHard)->fromp(), VarRef);

            Ast_गाँठ* स्थिर actionp = activep->stmtsp()->unlink_Fr_Back_With_अग्र();
            अगर (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                File_पंक्ति* स्थिर flp = trigVscp->fileline();
                स्थिर std::set<uint32_t>& triggers = entry.m_triggers;
                AstTriggered* स्थिर condp
                    = नया AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
                dlyShedIfp = नया AstIf{flp, condp};
                dlyShedIfp->addThensp(actionp);
            } अन्यथा {
                m_resumeFuncp->addStmtsp(actionp);
            }
        }
        अगर (dlyShedIfp) m_resumeFuncp->addStmtsp(dlyShedIfp);

        // These are now spent, oispose of now empty AstActive instances
        m_lbs.deleteActives();
    }
    AstCCall* स्थिर callp = नया AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
    callp->dtypeSetVoid();
    वापस callp;
}

AstVarScope* TimingKit::getDelayScheduler(AstNetlist* स्थिर /*netlistp*/) {
    क्रम (स्थिर स्वतः& entry : m_lbs) {
        AstActive* स्थिर ap = entry.m_activep;
        // TODO: this triple VN_AS expression is ridiculous
        AstVarRef* स्थिर schedrefp
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), Stmt_व्यंजक)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* स्थिर vscp = schedrefp->varScopep();
        अगर (vscp->dtypep()->basicp()->isDelayScheduler()) वापस vscp;
    }
    // None found. Design doesn't use any time delays
    वापस nullptr;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    अगर (!m_readyFuncp) {
        क्रम (स्थिर स्वतः& entry : m_lbs) {
            AstActive* स्थिर activep = entry.m_activep;
            स्वतः* स्थिर resumep = VN_AS(VN_AS(activep->stmtsp(), Stmt_व्यंजक)->exprp(), CMethodHard);
            UASSERT_OBJ(!resumep->nextp(), resumep, "Should be the only statement here");
            AstVarScope* स्थिर schedulerp = VN_AS(resumep->fromp(), VarRef)->varScopep();
            UASSERT_OBJ(schedulerp->dtypep()->basicp()->isDelayScheduler()
                            || schedulerp->dtypep()->basicp()->isTriggerScheduler()
                            || schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler(),
                        schedulerp, "Unexpected type");
            अगर (!schedulerp->dtypep()->basicp()->isTriggerScheduler()) जारी;
            // Create the global ready function only if we have trigger schedulers
            अगर (!m_readyFuncp) {
                AstNetlist* स्थिर netlistp = v3Global.rootp();
                AstScope* स्थिर scopeTopp = netlistp->शीर्ष_Scopep()->scopep();
                m_readyFuncp = नया AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(true);
                m_readyFuncp->isLoose(true);
                m_readyFuncp->isConst(false);
                m_readyFuncp->declPrivate(true);
                scopeTopp->addBlocksp(m_readyFuncp);
            }

            File_पंक्ति* स्थिर flp = activep->sentreep()->fileline();

            // Create an 'AstIf' sensitive to the suspending triggers
            स्थिर std::set<uint32_t>& triggers = entry.m_triggers;
            AstTriggered* स्थिर condp
                = नया AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
            AstIf* स्थिर ifp = नया AstIf{flp, condp};
            m_readyFuncp->addStmtsp(ifp);

            // Mark as ready the processes resumed on this sensitivity expression
            AstVarRef* स्थिर refp = नया AstVarRef{flp, schedulerp, VAccess::READWRITE};
            AstCMethodHard* स्थिर callp = नया AstCMethodHard{flp, refp, VCMethod::SCHED_READY};
            callp->dtypeSetVoid();
            अगर (resumep->pinsp()) callp->addPinsp(resumep->pinsp()->clone_वृक्ष(false));
            ifp->addThensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        अगर (!m_readyFuncp) वापस nullptr;
    }
    AstCCall* स्थिर callp = नया AstCCall{m_readyFuncp->fileline(), m_readyFuncp};
    callp->dtypeSetVoid();
    वापस callp;
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

श्रेणी AwaitVisitor final : खुला VNVisitor {
    // NODE STATE
    //  AstSenTree::user1()  -> bool.               Set true if the sentree has been visited.
    //  AstCFunc::user1()    -> AstUser1Allocator.  See alocator below
    स्थिर VNUser1InUse m_inuser1;

    // STATE
    bool m_inProcess = false;  // Are we in a process?
    bool m_gatherVars = false;  // Should we gather vars in m_writtenBySuspendable?
    AstScope* स्थिर m_scopeTopp;  // Scope at the top
    LogicByScope& m_lbs;  // Timing resume actives
    Ast_गाँठ_Stmt*& m_postUpdatesr;  // Post updates for the trigger eval function
    // Additional var sensitivities
    std::map<स्थिर AstVarScope*, std::set<Ast_Sen_वृक्ष*>>& m_externalDomains;
    std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::set<Ast_Sen_वृक्ष*> m_processDomains;  // Sentrees from the current process
    // Variables written by suspendable processes
    std::set<AstVarScope*> m_writtenBySuspendable;
    काष्ठा CFuncCache final {
        std::set<Ast_Sen_वृक्ष*> m_processDomains;  // What shall be added to m_processDomains
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
        क्रमागत State : uint8_t {
            UNINITIALIZED = 0,  // Not initialized members are empty
            VISITING,  // Visiting - needed for breaking recursion
            INITIALIZED,  // Members contains correct values
        } m_state  // Current state of Cache
            = UNINITIALIZED;
    };
    std::vector<स्थिर AstCFunc*> m_call_ढेर;  // Current callstack of AstCFuncs
    // Caches how visiting the function with given value of m_gatherVars changes
    // m_processDomains and m_writtenBySuspendable - only accessed from visit(AstCFunc* nodep)
    AstUser1Allocator<AstCFunc, std::array<CFuncCache, 2>> m_cfuncsCache;
    // Count uses of not inlined writes to signals in suspendables
    VDouble0 m_notInlinedWritesInSuspendableUsage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    व्योम addResumePins(AstCMethodHard* स्थिर resumep, Ast_गाँठ_व्यंजक* pinsp) {
        AstCExpr* स्थिर exprp = VN_CAST(pinsp, CExpr);
        AstText* स्थिर textp = VN_CAST(exprp->nodesp(), Text);
        अगर (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            अगर ((pinsp = VN_CAST(pinsp->nextp(), गाँठ_व्यंजक))) {
                resumep->addPinsp(pinsp->clone_वृक्ष(false));
            }
        } अन्यथा {
            resumep->addPinsp(pinsp->clone_वृक्ष(false));
        }
    }
    व्योम add_CFunc_Cached_मूल्यावली(स्थिर AstCFunc* स्थिर cfuncp,
                              std::unordered_set<स्थिर AstCFunc*>& visited) {
        अगर (!visited.जोड़ो(cfuncp).second) वापस;
        CFuncCache& मूल्य = m_cfuncsCache(cfuncp)[static_cast<माप_प्रकार>(m_gatherVars)];
        स्थिर माप_प्रकार initial_आकार = m_writtenBySuspendable.आकार();
        m_writtenBySuspendable.जोड़ो(मूल्य.m_writtenBySuspendable.begin(),
                                      मूल्य.m_writtenBySuspendable.end());
        m_notInlinedWritesInSuspendableUsage += m_writtenBySuspendable.आकार() - initial_आकार;
        m_processDomains.जोड़ो(मूल्य.m_processDomains.begin(), मूल्य.m_processDomains.end());
        क्रम (स्थिर AstCFunc* स्थिर includedp : मूल्य.m_includes) {
            add_CFunc_Cached_मूल्यावली(includedp, visited);
        }
    }
    // Create an active with a timing scheduler resume() call
    व्योम createResumeActive(AstCAwait* स्थिर awaitp) {
        स्वतः* स्थिर methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* स्थिर schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        Ast_Sen_वृक्ष* स्थिर sentreep = awaitp->sentreep();
        File_पंक्ति* स्थिर flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        स्वतः* स्थिर resumep = नया AstCMethodHard{
            flp, नया AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtypeSetVoid();
        अगर (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            अगर (Ast_गाँठ* स्थिर dbginfop = methodp->pinsp()->nextp()) {
                अगर (methodp->pinsp()) addResumePins(resumep, static_cast<Ast_गाँठ_व्यंजक*>(dbginfop));
            }
        } अन्यथा अगर (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            स्वतः* स्थिर postp = resumep->clone_वृक्ष(false);
            postp->method(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = Ast_गाँठ::add_अग्र(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        AstActive* स्थिर activep = नया AstActive{flp, "_timing", sentreep};
        activep->addStmtsp(resumep->makeStmt());
        m_lbs.add(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    व्योम visitCalledCFunc(AstCFunc* स्थिर nodep) {
        अगर (!m_inProcess) वापस;  // Skip if not in process - nothing would be added to any set
        स्थिर माप_प्रकार cache_कुंजी = static_cast<माप_प्रकार>(m_gatherVars);
        CFuncCache& मूल्य = m_cfuncsCache(nodep)[cache_कुंजी];
        चयन (मूल्य.m_state) {
        हाल CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            मूल्य.m_state = CFuncCache::VISITING;
            m_call_ढेर.धकेलो_back(nodep);
            iterateChildren(nodep);
            m_call_ढेर.निकालो_back();
            मूल्य.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_processDomains, मूल्य.m_processDomains);
            std::swap(m_writtenBySuspendable, मूल्य.m_writtenBySuspendable);
        } अवरोध;
        हाल CFuncCache::VISITING: {
            क्रम (माप_प्रकार i = m_call_ढेर.आकार() - 1; m_call_ढेर.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_ढेर[i])[cache_कुंजी].m_includes.जोड़ो(nodep);
            }
            वापस;  // Break recursion
        }
        हाल CFuncCache::INITIALIZED: अवरोध;
        }
        std::unordered_set<स्थिर AstCFunc*> visited;
        add_CFunc_Cached_मूल्यावली(nodep, visited);
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Procedure* स्थिर nodep) override {
        UASSERT_OBJ(!m_inProcess && !m_gatherVars && m_processDomains.empty()
                        && m_writtenBySuspendable.empty(),
                    nodep, "Process in process?");
        m_inProcess = true;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        क्रम (AstVarScope* स्थिर vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].जोड़ो(m_processDomains.begin(), m_processDomains.end());
            vscp->varp()->setWrittenBySuspendable();
        }
        m_processDomains.सफा();
        m_writtenBySuspendable.सफा();
        m_inProcess = false;
        m_gatherVars = false;
    }
    व्योम visit(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        अगर (m_inProcess) m_gatherVars = true;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    व्योम visit(AstCAwait* nodep) override {
        अगर (Ast_Sen_वृक्ष* स्थिर sentreep = nodep->sentreep()) {
            अगर (!sentreep->user1SetOnce()) createResumeActive(nodep);
            अगर (m_inProcess) m_processDomains.जोड़ो(sentreep);
        }
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_Var_Ref* nodep) override {
        अगर (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.जोड़ो(nodep->varScopep());
        }
    }
    व्योम visit(Ast_गाँठ_CCall* स्थिर nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        स्थिर स्वतः& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        अगर (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } अन्यथा {
            क्रम (AstCFunc* स्थिर cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    व्योम visit(AstCFunc* स्थिर nodep) override {
        स्थिर स्वतः& मूल्य = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        अगर (मूल्य[0].m_state != CFuncCache::UNINITIALIZED
            || मूल्य[1].m_state != CFuncCache::UNINITIALIZED) {
            वापस;
        }
        iterateChildren(nodep);
    }
    व्योम visit(Ast_व्यंजक_Stmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, Ast_गाँठ_Stmt*& postUpdatesr,
                          std::map<स्थिर AstVarScope*, std::set<Ast_Sen_वृक्ष*>>& externalDomains)
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

TimingKit prepareTiming(AstNetlist* स्थिर netlistp) {
    अगर (!v3Global.usesTiming()) वापस {};
    LogicByScope lbs;
    Ast_गाँठ_Stmt* postUpdates = nullptr;
    std::map<स्थिर AstVarScope*, std::set<Ast_Sen_वृक्ष*>> externalDomains;
    { AwaitVisitor{netlistp, lbs, postUpdates, externalDomains}; }
    वापस {std::move(lbs), postUpdates, std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

// Transform all forked processes into functions
श्रेणी TransformForksVisitor final : खुला VNVisitor {
    // NODE STATE
    //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current fork.
    स्थिर VNUser1InUse m_inuser1;

    // STATE
    bool m_inClass = false;  // Are we in a class?
    bool m_beginHasAwaits = false;  // Does the current begin have awaits?
    AstFork* m_forkp = nullptr;  // Current fork
    AstCFunc* m_funcp = nullptr;  // Current function

    // METHODS
    // Remap local vars referenced by the given fork function
    // TODO: We should only pass variables to the fork that are
    // live in the fork body, but for that we need a proper data
    // flow analysis framework which we don't have at the moment
    व्योम remapLocals(AstCFunc* स्थिर funcp, AstCCall* स्थिर callp) {
        स्थिर VNUser2InUse user2InUse;  // AstVarScope -> AstVarScope: var to remap to
        funcp->foreach([&](Ast_गाँठ_Var_Ref* refp) {
            AstVar* स्थिर varp = refp->varp();
            AstVarScope* स्थिर vscp = refp->varScopep();
            AstBasicDType* स्थिर dtypep = varp->dtypep()->basicp();
            // If not a fork..join, copy. All write refs should've been handled by V3Fork
            bool pass_By_मूल्य = !m_forkp->joinType().join();
            अगर (vscp->fork_By_मूल्य()) {
                // Set just before the fork, for its processes to get by value
                pass_By_मूल्य = true;
            } अन्यथा अगर (!varp->isFuncLocal()) {
                // Not func local. Its lifetime is longer than the forked process. Skip
                वापस;
            } अन्यथा अगर (!varp->user1()) {
                // Not declared before the fork. It cannot outlive the forked process
                वापस;
            } अन्यथा अगर (dtypep && dtypep->isForkSync()) {
                // We can just pass it by value to the new function
                pass_By_मूल्य = true;
            }
            // Remap the reference
            अगर (!vscp->user2p()) {
                // Clone the var to the new function
                AstVar* स्थिर newvarp
                    = नया AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                newvarp->funcLocal(true);
                newvarp->direction(pass_By_मूल्य ? VDirection::निवेश : VDirection::REF);
                funcp->addArgsp(newvarp);
                AstVarScope* स्थिर newvscp
                    = नया AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->addVarsp(newvscp);
                vscp->user2p(newvscp);
                callp->addArgsp(नया AstVarRef{refp->fileline(), vscp,
                                              pass_By_मूल्य ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* स्थिर newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        VL_RESTORER(m_inClass);
        m_inClass = VN_IS(nodep, Class);
        iterateChildren(nodep);
    }
    व्योम visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        m_funcp = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(AstVar* nodep) override {
        अगर (!m_forkp) nodep->user1(true);
    }
    व्योम visit(AstFork* nodep) override {
        अगर (m_forkp) वापस;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        Ast_गाँठ* resp = nullptr;
        अगर (Ast_गाँठ* स्थिर declsp = nodep->declsp()) {
            resp = Ast_गाँठ::add_अग्र(resp, declsp->unlink_Fr_Back_With_अग्र());
        }
        अगर (Ast_गाँठ* स्थिर stmtsp = nodep->stmtsp()) {
            resp = Ast_गाँठ::add_अग्र(resp, stmtsp->unlink_Fr_Back_With_अग्र());
        }
        जबतक (AstBegin* स्थिर beginp = nodep->forksp()) {
            अगर (Ast_गाँठ* स्थिर declsp = beginp->declsp()) {
                resp = Ast_गाँठ::add_अग्र(resp, declsp->unlink_Fr_Back_With_अग्र());
            }
            अगर (Ast_गाँठ* स्थिर stmtsp = beginp->stmtsp()) {
                resp = Ast_गाँठ::add_अग्र(resp, stmtsp->unlink_Fr_Back_With_अग्र());
            }
            VL_DO_DANGLING(धकेलो_Deletep(beginp->unlinkFrBack()), beginp);
        }
        अगर (resp) {
            nodep->replaceWith(resp);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstBegin* nodep) override {
        UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
        // Start with children, so later we only find awaits that are actually in this begin
        m_beginHasAwaits = false;
        iterateChildrenConst(nodep);
        अगर (!nodep->stmtsp()) वापस;
        अगर (!m_beginHasAwaits && !nodep->needProcess()) वापस;

        UASSERT_OBJ(!nodep->name().empty(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        File_पंक्ति* स्थिर flp = nodep->fileline();
        AstCFunc* स्थिर newfuncp = नया AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->add_अग्र_Here(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->declPrivate(true);
        // Create the call to the function
        AstCCall* स्थिर callp = नया AstCCall{flp, newfuncp};
        callp->dtypeSetVoid();
        // If we're in a class, add a vlSymsp arg
        अगर (m_inClass) {
            newfuncp->addStmtsp(नया AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        अगर (Ast_गाँठ* स्थिर declsp = nodep->declsp()) {
            newfuncp->addStmtsp(declsp->unlink_Fr_Back_With_अग्र());
        }
        अगर (Ast_गाँठ* स्थिर stmtsp = nodep->stmtsp()) {
            newfuncp->addStmtsp(stmtsp->unlink_Fr_Back_With_अग्र());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->addStmtsp(callp->makeStmt());
        // Propagate if needs process
        अगर (nodep->needProcess()) newfuncp->setNeedProcess();
        remapLocals(newfuncp, callp);
    }
    व्योम visit(AstCAwait* nodep) override {
        m_beginHasAwaits = true;
        iterateChildrenConst(nodep);
    }

    //--------------------
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = शेष;
};

व्योम transformForks(AstNetlist* स्थिर netlistp) {
    अगर (!v3Global.usesTiming()) वापस;
    { TransformForksVisitor{netlistp}; }
    V3Global::dump_Check_Global_वृक्ष("transform_forks", 0, dump_वृक्ष_Either_Level() >= 3);
}

}  // namespace V3Sched
