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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3AstUserAllocator.h"
#அந்தர்பாவ "V3ClassGraph.h"
#அந்தர்பாவ "V3EmitCBase.h"
#அந்தர்பாவ "V3Sched.h"

#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

நாமாகாஶ V3Sched {

//============================================================================
// Trigger indices of the external domains of each variable

std::map<மாறிலி AstVarScope*, std::vector<uint32_t>>
TimingKit::externalTriggerIndices(மாறிலி TriggerKit& trigKit) மாறிலி {
    std::map<மாறிலி AstVarScope*, std::vector<uint32_t>> result;
    க்கு (மாறிலி auto& vscpDomains : m_externalDomains) {
        std::vector<uint32_t>& indices = result[vscpDomains.first];
        க்கு (மாறிலி Ast_Sen_மரம்* மாறிலி domainp : vscpDomains.second) {
            க்கு (மாறிலி AstSenItem* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                indices.தள்ளு_back(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    திரும்பு result;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    எனில் (!m_resumeFuncp) {
        எனில் (m_lbs.empty()) திரும்பு nullptr;
        // Create global resume function
        AstNetlist* மாறிலி netlistp = v3Global.rootp();
        AstScope* மாறிலி scopeTopp = netlistp->உச்சி_Scopep()->scopep();
        m_resumeFuncp = புதிய AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(உண்மை);
        m_resumeFuncp->isLoose(உண்மை);
        m_resumeFuncp->isConst(பொய்);
        m_resumeFuncp->declPrivate(உண்மை);
        scopeTopp->addBlocksp(m_resumeFuncp);

        க்கு (மாறிலி auto& entry : m_lbs) {
            AstActive* மாறிலி activep = entry.m_activep;
            activep->foreach([இது](AstCMethodHard* மாறிலி exprp) {
                எனில் (exprp->method() != VCMethod::SCHED_RESUME) திரும்பு;
                Ast_முனை_கோவை* மாறிலி fromp = exprp->fromp();
                எனில் (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    திரும்பு;
                }
                AstCMethodHard* மாறிலி moveToResumep = புதிய AstCMethodHard{
                    fromp->fileline(), fromp->clone_மரம்(பொய்),
                    VCMethod::SCHED_MOVE_TO_RESUME_QUEUE,
                    exprp->pinsp() ? exprp->pinsp()->clone_மரம்(உண்மை) : nullptr};
                moveToResumep->dtypeSetVoid();
                m_resumeFuncp->addStmtsp(moveToResumep->makeStmt());
            });
        }

        // Put all the timing actives in the resume function
        AstIf* dlyShedIfp = nullptr;
        க்கு (மாறிலி auto& entry : m_lbs) {
            AstActive* மாறிலி activep = entry.m_activep;
            // Resume time delays last. For no particular reason other than
            // that's what we used to do prior to proper #0 support.
            AstVarRef* மாறிலி schedrefp = VN_AS(
                VN_AS(VN_AS(activep->stmtsp(), Stmt_கோவை)->exprp(), CMethodHard)->fromp(), VarRef);

            Ast_முனை* மாறிலி actionp = activep->stmtsp()->unlinkFrBackWithNext();
            எனில் (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                File_வரி* மாறிலி flp = trigVscp->fileline();
                மாறிலி std::set<uint32_t>& triggers = entry.m_triggers;
                AstTriggered* மாறிலி condp
                    = புதிய AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
                dlyShedIfp = புதிய AstIf{flp, condp};
                dlyShedIfp->addThensp(actionp);
            } இல்லையெனில் {
                m_resumeFuncp->addStmtsp(actionp);
            }
        }
        எனில் (dlyShedIfp) m_resumeFuncp->addStmtsp(dlyShedIfp);

        // These are now spent, oispose of now empty AstActive instances
        m_lbs.deleteActives();
    }
    AstCCall* மாறிலி callp = புதிய AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
    callp->dtypeSetVoid();
    திரும்பு callp;
}

AstVarScope* TimingKit::getDelayScheduler(AstNetlist* மாறிலி /*netlistp*/) {
    க்கு (மாறிலி auto& entry : m_lbs) {
        AstActive* மாறிலி ap = entry.m_activep;
        // TODO: this triple VN_AS expression is ridiculous
        AstVarRef* மாறிலி schedrefp
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), Stmt_கோவை)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* மாறிலி vscp = schedrefp->varScopep();
        எனில் (vscp->dtypep()->basicp()->isDelayScheduler()) திரும்பு vscp;
    }
    // None found. Design doesn't use any time delays
    திரும்பு nullptr;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    எனில் (!m_readyFuncp) {
        க்கு (மாறிலி auto& entry : m_lbs) {
            AstActive* மாறிலி activep = entry.m_activep;
            auto* மாறிலி resumep = VN_AS(VN_AS(activep->stmtsp(), Stmt_கோவை)->exprp(), CMethodHard);
            UASSERT_OBJ(!resumep->nextp(), resumep, "Should be the only statement here");
            AstVarScope* மாறிலி schedulerp = VN_AS(resumep->fromp(), VarRef)->varScopep();
            UASSERT_OBJ(schedulerp->dtypep()->basicp()->isDelayScheduler()
                            || schedulerp->dtypep()->basicp()->isTriggerScheduler()
                            || schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler(),
                        schedulerp, "Unexpected type");
            எனில் (!schedulerp->dtypep()->basicp()->isTriggerScheduler()) தொடர்;
            // Create the global ready function only if we have trigger schedulers
            எனில் (!m_readyFuncp) {
                AstNetlist* மாறிலி netlistp = v3Global.rootp();
                AstScope* மாறிலி scopeTopp = netlistp->உச்சி_Scopep()->scopep();
                m_readyFuncp = புதிய AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(உண்மை);
                m_readyFuncp->isLoose(உண்மை);
                m_readyFuncp->isConst(பொய்);
                m_readyFuncp->declPrivate(உண்மை);
                scopeTopp->addBlocksp(m_readyFuncp);
            }

            File_வரி* மாறிலி flp = activep->sentreep()->fileline();

            // Create an 'AstIf' sensitive to the suspending triggers
            மாறிலி std::set<uint32_t>& triggers = entry.m_triggers;
            AstTriggered* மாறிலி condp
                = புதிய AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
            AstIf* மாறிலி ifp = புதிய AstIf{flp, condp};
            m_readyFuncp->addStmtsp(ifp);

            // Mark as ready the processes resumed on this sensitivity expression
            AstVarRef* மாறிலி refp = புதிய AstVarRef{flp, schedulerp, VAccess::READWRITE};
            AstCMethodHard* மாறிலி callp = புதிய AstCMethodHard{flp, refp, VCMethod::SCHED_READY};
            callp->dtypeSetVoid();
            எனில் (resumep->pinsp()) callp->addPinsp(resumep->pinsp()->clone_மரம்(பொய்));
            ifp->addThensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        எனில் (!m_readyFuncp) திரும்பு nullptr;
    }
    AstCCall* மாறிலி callp = புதிய AstCCall{m_readyFuncp->fileline(), m_readyFuncp};
    callp->dtypeSetVoid();
    திரும்பு callp;
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

வகுப்பு AwaitVisitor final : பொது VNVisitor {
    // NODE STATE
    //  AstSenTree::user1()  -> bool.               Set true if the sentree has been visited.
    //  AstCFunc::user1()    -> AstUser1Allocator.  See alocator below
    மாறிலி VNUser1InUse m_inuser1;

    // STATE
    தருக்க m_inProcess = பொய்;  // Are we in a process?
    தருக்க m_gatherVars = பொய்;  // Should we gather vars in m_writtenBySuspendable?
    AstScope* மாறிலி m_scopeTopp;  // Scope at the top
    LogicByScope& m_lbs;  // Timing resume actives
    Ast_முனை_Stmt*& m_postUpdatesr;  // Post updates for the trigger eval function
    // Additional var sensitivities
    std::map<மாறிலி AstVarScope*, std::set<Ast_Sen_மரம்*>>& m_externalDomains;
    std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::set<Ast_Sen_மரம்*> m_processDomains;  // Sentrees from the current process
    // Variables written by suspendable processes
    std::set<AstVarScope*> m_writtenBySuspendable;
    கட்டமைப்பு CFuncCache final {
        std::set<Ast_Sen_மரம்*> m_processDomains;  // What shall be added to m_processDomains
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
        எண்ணிக்கை State : uint8_t {
            UNINITIALIZED = 0,  // Not initialized members are empty
            VISITING,  // Visiting - needed for breaking recursion
            INITIALIZED,  // Members contains correct values
        } m_state  // Current state of Cache
            = UNINITIALIZED;
    };
    std::vector<மாறிலி AstCFunc*> m_call_அடுக்கு;  // Current callstack of AstCFuncs
    // Caches how visiting the function with given value of m_gatherVars changes
    // m_processDomains and m_writtenBySuspendable - only accessed from visit(AstCFunc* nodep)
    AstUser1Allocator<AstCFunc, std::array<CFuncCache, 2>> m_cfuncsCache;
    // Count uses of not inlined writes to signals in suspendables
    VDouble0 m_notInlinedWritesInSuspendableUsage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    வெறுமை addResumePins(AstCMethodHard* மாறிலி resumep, Ast_முனை_கோவை* pinsp) {
        AstCExpr* மாறிலி exprp = VN_CAST(pinsp, CExpr);
        AstText* மாறிலி textp = VN_CAST(exprp->nodesp(), Text);
        எனில் (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            எனில் ((pinsp = VN_CAST(pinsp->nextp(), முனை_கோவை))) {
                resumep->addPinsp(pinsp->clone_மரம்(பொய்));
            }
        } இல்லையெனில் {
            resumep->addPinsp(pinsp->clone_மரம்(பொய்));
        }
    }
    வெறுமை add_CFunc_Cached_மதிப்புகள்(மாறிலி AstCFunc* மாறிலி cfuncp,
                              std::unordered_set<மாறிலி AstCFunc*>& visited) {
        எனில் (!visited.செருகு(cfuncp).second) திரும்பு;
        CFuncCache& மதிப்பு = m_cfuncsCache(cfuncp)[static_cast<size_t>(m_gatherVars)];
        மாறிலி size_t initialSize = m_writtenBySuspendable.size();
        m_writtenBySuspendable.செருகு(மதிப்பு.m_writtenBySuspendable.begin(),
                                      மதிப்பு.m_writtenBySuspendable.end());
        m_notInlinedWritesInSuspendableUsage += m_writtenBySuspendable.size() - initialSize;
        m_processDomains.செருகு(மதிப்பு.m_processDomains.begin(), மதிப்பு.m_processDomains.end());
        க்கு (மாறிலி AstCFunc* மாறிலி includedp : மதிப்பு.m_includes) {
            add_CFunc_Cached_மதிப்புகள்(includedp, visited);
        }
    }
    // Create an active with a timing scheduler resume() call
    வெறுமை createResumeActive(AstCAwait* மாறிலி awaitp) {
        auto* மாறிலி methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* மாறிலி schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        Ast_Sen_மரம்* மாறிலி sentreep = awaitp->sentreep();
        File_வரி* மாறிலி flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        auto* மாறிலி resumep = புதிய AstCMethodHard{
            flp, புதிய AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtypeSetVoid();
        எனில் (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            எனில் (Ast_முனை* மாறிலி dbginfop = methodp->pinsp()->nextp()) {
                எனில் (methodp->pinsp()) addResumePins(resumep, static_cast<Ast_முனை_கோவை*>(dbginfop));
            }
        } இல்லையெனில் எனில் (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            auto* மாறிலி postp = resumep->clone_மரம்(பொய்);
            postp->method(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = Ast_முனை::addNext(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        AstActive* மாறிலி activep = புதிய AstActive{flp, "_timing", sentreep};
        activep->addStmtsp(resumep->makeStmt());
        m_lbs.add(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    வெறுமை visitCalledCFunc(AstCFunc* மாறிலி nodep) {
        எனில் (!m_inProcess) திரும்பு;  // Skip if not in process - nothing would be added to any set
        மாறிலி size_t cache_சாவி = static_cast<size_t>(m_gatherVars);
        CFuncCache& மதிப்பு = m_cfuncsCache(nodep)[cache_சாவி];
        தேர்வு (மதிப்பு.m_state) {
        நிலை CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            மதிப்பு.m_state = CFuncCache::VISITING;
            m_call_அடுக்கு.தள்ளு_back(nodep);
            iterateChildren(nodep);
            m_call_அடுக்கு.எடு_back();
            மதிப்பு.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_processDomains, மதிப்பு.m_processDomains);
            std::swap(m_writtenBySuspendable, மதிப்பு.m_writtenBySuspendable);
        } நிறுத்து;
        நிலை CFuncCache::VISITING: {
            க்கு (size_t i = m_call_அடுக்கு.size() - 1; m_call_அடுக்கு.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_அடுக்கு[i])[cache_சாவி].m_includes.செருகு(nodep);
            }
            திரும்பு;  // Break recursion
        }
        நிலை CFuncCache::INITIALIZED: நிறுத்து;
        }
        std::unordered_set<மாறிலி AstCFunc*> visited;
        add_CFunc_Cached_மதிப்புகள்(nodep, visited);
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Procedure* மாறிலி nodep) override {
        UASSERT_OBJ(!m_inProcess && !m_gatherVars && m_processDomains.empty()
                        && m_writtenBySuspendable.empty(),
                    nodep, "Process in process?");
        m_inProcess = உண்மை;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        க்கு (AstVarScope* மாறிலி vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].செருகு(m_processDomains.begin(), m_processDomains.end());
            vscp->varp()->setWrittenBySuspendable();
        }
        m_processDomains.clear();
        m_writtenBySuspendable.clear();
        m_inProcess = பொய்;
        m_gatherVars = பொய்;
    }
    வெறுமை visit(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        எனில் (m_inProcess) m_gatherVars = உண்மை;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    வெறுமை visit(AstCAwait* nodep) override {
        எனில் (Ast_Sen_மரம்* மாறிலி sentreep = nodep->sentreep()) {
            எனில் (!sentreep->user1SetOnce()) createResumeActive(nodep);
            எனில் (m_inProcess) m_processDomains.செருகு(sentreep);
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_Var_Ref* nodep) override {
        எனில் (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.செருகு(nodep->varScopep());
        }
    }
    வெறுமை visit(Ast_முனை_CCall* மாறிலி nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        மாறிலி auto& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        எனில் (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } இல்லையெனில் {
            க்கு (AstCFunc* மாறிலி cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    வெறுமை visit(AstCFunc* மாறிலி nodep) override {
        மாறிலி auto& மதிப்பு = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        எனில் (மதிப்பு[0].m_state != CFuncCache::UNINITIALIZED
            || மதிப்பு[1].m_state != CFuncCache::UNINITIALIZED) {
            திரும்பு;
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_கோவை_Stmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, Ast_முனை_Stmt*& postUpdatesr,
                          std::map<மாறிலி AstVarScope*, std::set<Ast_Sen_மரம்*>>& externalDomains)
        : m_scopeTopp{nodep->உச்சி_Scopep()->scopep()}
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

TimingKit prepareTiming(AstNetlist* மாறிலி netlistp) {
    எனில் (!v3Global.usesTiming()) திரும்பு {};
    LogicByScope lbs;
    Ast_முனை_Stmt* postUpdates = nullptr;
    std::map<மாறிலி AstVarScope*, std::set<Ast_Sen_மரம்*>> externalDomains;
    { AwaitVisitor{netlistp, lbs, postUpdates, externalDomains}; }
    திரும்பு {std::move(lbs), postUpdates, std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

// Transform all forked processes into functions
வகுப்பு TransformForksVisitor final : பொது VNVisitor {
    // NODE STATE
    //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current fork.
    மாறிலி VNUser1InUse m_inuser1;

    // STATE
    தருக்க m_inClass = பொய்;  // Are we in a class?
    தருக்க m_beginHasAwaits = பொய்;  // Does the current begin have awaits?
    AstFork* m_forkp = nullptr;  // Current fork
    AstCFunc* m_funcp = nullptr;  // Current function

    // METHODS
    // Remap local vars referenced by the given fork function
    // TODO: We should only pass variables to the fork that are
    // live in the fork body, but for that we need a proper data
    // flow analysis framework which we don't have at the moment
    வெறுமை remapLocals(AstCFunc* மாறிலி funcp, AstCCall* மாறிலி callp) {
        மாறிலி VNUser2InUse user2InUse;  // AstVarScope -> AstVarScope: var to remap to
        funcp->foreach([&](Ast_முனை_Var_Ref* refp) {
            AstVar* மாறிலி varp = refp->varp();
            AstVarScope* மாறிலி vscp = refp->varScopep();
            AstBasicDType* மாறிலி dtypep = varp->dtypep()->basicp();
            // If not a fork..join, copy. All write refs should've been handled by V3Fork
            தருக்க pass_By_மதிப்பு = !m_forkp->joinType().join();
            எனில் (vscp->fork_By_மதிப்பு()) {
                // Set just before the fork, for its processes to get by value
                pass_By_மதிப்பு = உண்மை;
            } இல்லையெனில் எனில் (!varp->isFuncLocal()) {
                // Not func local. Its lifetime is longer than the forked process. Skip
                திரும்பு;
            } இல்லையெனில் எனில் (!varp->user1()) {
                // Not declared before the fork. It cannot outlive the forked process
                திரும்பு;
            } இல்லையெனில் எனில் (dtypep && dtypep->isForkSync()) {
                // We can just pass it by value to the new function
                pass_By_மதிப்பு = உண்மை;
            }
            // Remap the reference
            எனில் (!vscp->user2p()) {
                // Clone the var to the new function
                AstVar* மாறிலி newvarp
                    = புதிய AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                newvarp->funcLocal(உண்மை);
                newvarp->direction(pass_By_மதிப்பு ? VDirection::உள்ளீடு : VDirection::REF);
                funcp->addArgsp(newvarp);
                AstVarScope* மாறிலி newvscp
                    = புதிய AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->addVarsp(newvscp);
                vscp->user2p(newvscp);
                callp->addArgsp(புதிய AstVarRef{refp->fileline(), vscp,
                                              pass_By_மதிப்பு ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* மாறிலி newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        VL_RESTORER(m_inClass);
        m_inClass = VN_IS(nodep, Class);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        m_funcp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstVar* nodep) override {
        எனில் (!m_forkp) nodep->user1(உண்மை);
    }
    வெறுமை visit(AstFork* nodep) override {
        எனில் (m_forkp) திரும்பு;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        Ast_முனை* resp = nullptr;
        எனில் (Ast_முனை* மாறிலி declsp = nodep->declsp()) {
            resp = Ast_முனை::addNext(resp, declsp->unlinkFrBackWithNext());
        }
        எனில் (Ast_முனை* மாறிலி stmtsp = nodep->stmtsp()) {
            resp = Ast_முனை::addNext(resp, stmtsp->unlinkFrBackWithNext());
        }
        போது (AstBegin* மாறிலி beginp = nodep->forksp()) {
            எனில் (Ast_முனை* மாறிலி declsp = beginp->declsp()) {
                resp = Ast_முனை::addNext(resp, declsp->unlinkFrBackWithNext());
            }
            எனில் (Ast_முனை* மாறிலி stmtsp = beginp->stmtsp()) {
                resp = Ast_முனை::addNext(resp, stmtsp->unlinkFrBackWithNext());
            }
            VL_DO_DANGLING(தள்ளு_Deletep(beginp->unlinkFrBack()), beginp);
        }
        எனில் (resp) {
            nodep->replaceWith(resp);
        } இல்லையெனில் {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstBegin* nodep) override {
        UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
        // Start with children, so later we only find awaits that are actually in this begin
        m_beginHasAwaits = பொய்;
        iterateChildrenConst(nodep);
        எனில் (!nodep->stmtsp()) திரும்பு;
        எனில் (!m_beginHasAwaits && !nodep->needProcess()) திரும்பு;

        UASSERT_OBJ(!nodep->name().empty(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        File_வரி* மாறிலி flp = nodep->fileline();
        AstCFunc* மாறிலி newfuncp = புதிய AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->addNextHere(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->declPrivate(உண்மை);
        // Create the call to the function
        AstCCall* மாறிலி callp = புதிய AstCCall{flp, newfuncp};
        callp->dtypeSetVoid();
        // If we're in a class, add a vlSymsp arg
        எனில் (m_inClass) {
            newfuncp->addStmtsp(புதிய AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        எனில் (Ast_முனை* மாறிலி declsp = nodep->declsp()) {
            newfuncp->addStmtsp(declsp->unlinkFrBackWithNext());
        }
        எனில் (Ast_முனை* மாறிலி stmtsp = nodep->stmtsp()) {
            newfuncp->addStmtsp(stmtsp->unlinkFrBackWithNext());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->addStmtsp(callp->makeStmt());
        // Propagate if needs process
        எனில் (nodep->needProcess()) newfuncp->setNeedProcess();
        remapLocals(newfuncp, callp);
    }
    வெறுமை visit(AstCAwait* nodep) override {
        m_beginHasAwaits = உண்மை;
        iterateChildrenConst(nodep);
    }

    //--------------------
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = இயல்பு;
};

வெறுமை transformForks(AstNetlist* மாறிலி netlistp) {
    எனில் (!v3Global.usesTiming()) திரும்பு;
    { TransformForksVisitor{netlistp}; }
    V3Global::dump_Check_Global_மரம்("transform_forks", 0, dump_மரம்_Either_Level() >= 3);
}

}  // namespace V3Sched
