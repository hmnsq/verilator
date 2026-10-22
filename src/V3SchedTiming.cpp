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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3AstUserAllocator.h"
#અન્તર્ભાવ "V3ClassGraph.h"
#અન્તર્ભાવ "V3EmitCBase.h"
#અન્તર્ભાવ "V3Sched.h"

#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

નામાકાશ V3Sched {

//============================================================================
// Trigger indices of the external domains of each variable

std::map<અચલ AstVarScope*, std::vector<uint32_t>>
TimingKit::externalTriggerIndices(અચલ TriggerKit& trigKit) અચલ {
    std::map<અચલ AstVarScope*, std::vector<uint32_t>> result;
    માટે (અચલ auto& vscpDomains : m_externalDomains) {
        std::vector<uint32_t>& indices = result[vscpDomains.first];
        માટે (અચલ Ast_Sen_વૃક્ષ* અચલ domainp : vscpDomains.second) {
            માટે (અચલ AstSenItem* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                indices.ધકેલો_back(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    પાછા result;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    જો (!m_resumeFuncp) {
        જો (m_lbs.empty()) પાછા nullptr;
        // Create global resume function
        AstNetlist* અચલ netlistp = v3Global.rootp();
        AstScope* અચલ scopeTopp = netlistp->ટોચ_Scopep()->scopep();
        m_resumeFuncp = નવું AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(સાચું);
        m_resumeFuncp->isLoose(સાચું);
        m_resumeFuncp->isConst(ખોટું);
        m_resumeFuncp->declPrivate(સાચું);
        scopeTopp->addBlocksp(m_resumeFuncp);

        માટે (અચલ auto& entry : m_lbs) {
            AstActive* અચલ activep = entry.m_activep;
            activep->foreach([આ](AstCMethodHard* અચલ exprp) {
                જો (exprp->method() != VCMethod::SCHED_RESUME) પાછા;
                Ast_ગાંઠ_પદાવલિ* અચલ fromp = exprp->fromp();
                જો (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    પાછા;
                }
                AstCMethodHard* અચલ moveToResumep = નવું AstCMethodHard{
                    fromp->fileline(), fromp->clone_વૃક્ષ(ખોટું),
                    VCMethod::SCHED_MOVE_TO_RESUME_QUEUE,
                    exprp->pinsp() ? exprp->pinsp()->clone_વૃક્ષ(સાચું) : nullptr};
                moveToResumep->dtypeSetVoid();
                m_resumeFuncp->addStmtsp(moveToResumep->makeStmt());
            });
        }

        // Put all the timing actives in the resume function
        AstIf* dlyShedIfp = nullptr;
        માટે (અચલ auto& entry : m_lbs) {
            AstActive* અચલ activep = entry.m_activep;
            // Resume time delays last. For no particular reason other than
            // that's what we used to do prior to proper #0 support.
            AstVarRef* અચલ schedrefp = VN_AS(
                VN_AS(VN_AS(activep->stmtsp(), Stmt_પદાવલિ)->exprp(), CMethodHard)->fromp(), VarRef);

            Ast_ગાંઠ* અચલ actionp = activep->stmtsp()->unlinkFrBackWithNext();
            જો (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                File_લીટી* અચલ flp = trigVscp->fileline();
                અચલ std::set<uint32_t>& triggers = entry.m_triggers;
                AstTriggered* અચલ condp
                    = નવું AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
                dlyShedIfp = નવું AstIf{flp, condp};
                dlyShedIfp->addThensp(actionp);
            } નહીંતર {
                m_resumeFuncp->addStmtsp(actionp);
            }
        }
        જો (dlyShedIfp) m_resumeFuncp->addStmtsp(dlyShedIfp);

        // These are now spent, oispose of now empty AstActive instances
        m_lbs.deleteActives();
    }
    AstCCall* અચલ callp = નવું AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
    callp->dtypeSetVoid();
    પાછા callp;
}

AstVarScope* TimingKit::getDelayScheduler(AstNetlist* અચલ /*netlistp*/) {
    માટે (અચલ auto& entry : m_lbs) {
        AstActive* અચલ ap = entry.m_activep;
        // TODO: this triple VN_AS expression is ridiculous
        AstVarRef* અચલ schedrefp
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), Stmt_પદાવલિ)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* અચલ vscp = schedrefp->varScopep();
        જો (vscp->dtypep()->basicp()->isDelayScheduler()) પાછા vscp;
    }
    // None found. Design doesn't use any time delays
    પાછા nullptr;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    જો (!m_readyFuncp) {
        માટે (અચલ auto& entry : m_lbs) {
            AstActive* અચલ activep = entry.m_activep;
            auto* અચલ resumep = VN_AS(VN_AS(activep->stmtsp(), Stmt_પદાવલિ)->exprp(), CMethodHard);
            UASSERT_OBJ(!resumep->nextp(), resumep, "Should be the only statement here");
            AstVarScope* અચલ schedulerp = VN_AS(resumep->fromp(), VarRef)->varScopep();
            UASSERT_OBJ(schedulerp->dtypep()->basicp()->isDelayScheduler()
                            || schedulerp->dtypep()->basicp()->isTriggerScheduler()
                            || schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler(),
                        schedulerp, "Unexpected type");
            જો (!schedulerp->dtypep()->basicp()->isTriggerScheduler()) ચાલુ;
            // Create the global ready function only if we have trigger schedulers
            જો (!m_readyFuncp) {
                AstNetlist* અચલ netlistp = v3Global.rootp();
                AstScope* અચલ scopeTopp = netlistp->ટોચ_Scopep()->scopep();
                m_readyFuncp = નવું AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(સાચું);
                m_readyFuncp->isLoose(સાચું);
                m_readyFuncp->isConst(ખોટું);
                m_readyFuncp->declPrivate(સાચું);
                scopeTopp->addBlocksp(m_readyFuncp);
            }

            File_લીટી* અચલ flp = activep->sentreep()->fileline();

            // Create an 'AstIf' sensitive to the suspending triggers
            અચલ std::set<uint32_t>& triggers = entry.m_triggers;
            AstTriggered* અચલ condp
                = નવું AstTriggered{flp, trigVscp, {triggers.begin(), triggers.end()}};
            AstIf* અચલ ifp = નવું AstIf{flp, condp};
            m_readyFuncp->addStmtsp(ifp);

            // Mark as ready the processes resumed on this sensitivity expression
            AstVarRef* અચલ refp = નવું AstVarRef{flp, schedulerp, VAccess::READWRITE};
            AstCMethodHard* અચલ callp = નવું AstCMethodHard{flp, refp, VCMethod::SCHED_READY};
            callp->dtypeSetVoid();
            જો (resumep->pinsp()) callp->addPinsp(resumep->pinsp()->clone_વૃક્ષ(ખોટું));
            ifp->addThensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        જો (!m_readyFuncp) પાછા nullptr;
    }
    AstCCall* અચલ callp = નવું AstCCall{m_readyFuncp->fileline(), m_readyFuncp};
    callp->dtypeSetVoid();
    પાછા callp;
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

વર્ગ AwaitVisitor final : જાહેર VNVisitor {
    // NODE STATE
    //  AstSenTree::user1()  -> bool.               Set true if the sentree has been visited.
    //  AstCFunc::user1()    -> AstUser1Allocator.  See alocator below
    અચલ VNUser1InUse m_inuser1;

    // STATE
    તાર્કિક m_inProcess = ખોટું;  // Are we in a process?
    તાર્કિક m_gatherVars = ખોટું;  // Should we gather vars in m_writtenBySuspendable?
    AstScope* અચલ m_scopeTopp;  // Scope at the top
    LogicByScope& m_lbs;  // Timing resume actives
    Ast_ગાંઠ_Stmt*& m_postUpdatesr;  // Post updates for the trigger eval function
    // Additional var sensitivities
    std::map<અચલ AstVarScope*, std::set<Ast_Sen_વૃક્ષ*>>& m_externalDomains;
    std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::set<Ast_Sen_વૃક્ષ*> m_processDomains;  // Sentrees from the current process
    // Variables written by suspendable processes
    std::set<AstVarScope*> m_writtenBySuspendable;
    સંરચના CFuncCache final {
        std::set<Ast_Sen_વૃક્ષ*> m_processDomains;  // What shall be added to m_processDomains
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
        ગણના State : uint8_t {
            UNINITIALIZED = 0,  // Not initialized members are empty
            VISITING,  // Visiting - needed for breaking recursion
            INITIALIZED,  // Members contains correct values
        } m_state  // Current state of Cache
            = UNINITIALIZED;
    };
    std::vector<અચલ AstCFunc*> m_call_ઢગલો;  // Current callstack of AstCFuncs
    // Caches how visiting the function with given value of m_gatherVars changes
    // m_processDomains and m_writtenBySuspendable - only accessed from visit(AstCFunc* nodep)
    AstUser1Allocator<AstCFunc, std::array<CFuncCache, 2>> m_cfuncsCache;
    // Count uses of not inlined writes to signals in suspendables
    VDouble0 m_notInlinedWritesInSuspendableUsage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    શૂન્ય addResumePins(AstCMethodHard* અચલ resumep, Ast_ગાંઠ_પદાવલિ* pinsp) {
        AstCExpr* અચલ exprp = VN_CAST(pinsp, CExpr);
        AstText* અચલ textp = VN_CAST(exprp->nodesp(), Text);
        જો (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            જો ((pinsp = VN_CAST(pinsp->nextp(), ગાંઠ_પદાવલિ))) {
                resumep->addPinsp(pinsp->clone_વૃક્ષ(ખોટું));
            }
        } નહીંતર {
            resumep->addPinsp(pinsp->clone_વૃક્ષ(ખોટું));
        }
    }
    શૂન્ય add_CFunc_Cached_કિંમતો(અચલ AstCFunc* અચલ cfuncp,
                              std::unordered_set<અચલ AstCFunc*>& visited) {
        જો (!visited.ઉમેરો(cfuncp).second) પાછા;
        CFuncCache& કિંમત = m_cfuncsCache(cfuncp)[static_cast<size_t>(m_gatherVars)];
        અચલ size_t initialSize = m_writtenBySuspendable.size();
        m_writtenBySuspendable.ઉમેરો(કિંમત.m_writtenBySuspendable.begin(),
                                      કિંમત.m_writtenBySuspendable.end());
        m_notInlinedWritesInSuspendableUsage += m_writtenBySuspendable.size() - initialSize;
        m_processDomains.ઉમેરો(કિંમત.m_processDomains.begin(), કિંમત.m_processDomains.end());
        માટે (અચલ AstCFunc* અચલ includedp : કિંમત.m_includes) {
            add_CFunc_Cached_કિંમતો(includedp, visited);
        }
    }
    // Create an active with a timing scheduler resume() call
    શૂન્ય createResumeActive(AstCAwait* અચલ awaitp) {
        auto* અચલ methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* અચલ schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        Ast_Sen_વૃક્ષ* અચલ sentreep = awaitp->sentreep();
        File_લીટી* અચલ flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        auto* અચલ resumep = નવું AstCMethodHard{
            flp, નવું AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtypeSetVoid();
        જો (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            જો (Ast_ગાંઠ* અચલ dbginfop = methodp->pinsp()->nextp()) {
                જો (methodp->pinsp()) addResumePins(resumep, static_cast<Ast_ગાંઠ_પદાવલિ*>(dbginfop));
            }
        } નહીંતર જો (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            auto* અચલ postp = resumep->clone_વૃક્ષ(ખોટું);
            postp->method(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = Ast_ગાંઠ::addNext(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        AstActive* અચલ activep = નવું AstActive{flp, "_timing", sentreep};
        activep->addStmtsp(resumep->makeStmt());
        m_lbs.add(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    શૂન્ય visitCalledCFunc(AstCFunc* અચલ nodep) {
        જો (!m_inProcess) પાછા;  // Skip if not in process - nothing would be added to any set
        અચલ size_t cache_ચાવી = static_cast<size_t>(m_gatherVars);
        CFuncCache& કિંમત = m_cfuncsCache(nodep)[cache_ચાવી];
        પસંદગી (કિંમત.m_state) {
        સ્થિતિ CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            કિંમત.m_state = CFuncCache::VISITING;
            m_call_ઢગલો.ધકેલો_back(nodep);
            iterateChildren(nodep);
            m_call_ઢગલો.કાઢો_back();
            કિંમત.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_processDomains, કિંમત.m_processDomains);
            std::swap(m_writtenBySuspendable, કિંમત.m_writtenBySuspendable);
        } તોડો;
        સ્થિતિ CFuncCache::VISITING: {
            માટે (size_t i = m_call_ઢગલો.size() - 1; m_call_ઢગલો.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_ઢગલો[i])[cache_ચાવી].m_includes.ઉમેરો(nodep);
            }
            પાછા;  // Break recursion
        }
        સ્થિતિ CFuncCache::INITIALIZED: તોડો;
        }
        std::unordered_set<અચલ AstCFunc*> visited;
        add_CFunc_Cached_કિંમતો(nodep, visited);
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Procedure* અચલ nodep) override {
        UASSERT_OBJ(!m_inProcess && !m_gatherVars && m_processDomains.empty()
                        && m_writtenBySuspendable.empty(),
                    nodep, "Process in process?");
        m_inProcess = સાચું;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        માટે (AstVarScope* અચલ vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].ઉમેરો(m_processDomains.begin(), m_processDomains.end());
            vscp->varp()->setWrittenBySuspendable();
        }
        m_processDomains.clear();
        m_writtenBySuspendable.clear();
        m_inProcess = ખોટું;
        m_gatherVars = ખોટું;
    }
    શૂન્ય visit(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        જો (m_inProcess) m_gatherVars = સાચું;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstCAwait* nodep) override {
        જો (Ast_Sen_વૃક્ષ* અચલ sentreep = nodep->sentreep()) {
            જો (!sentreep->user1SetOnce()) createResumeActive(nodep);
            જો (m_inProcess) m_processDomains.ઉમેરો(sentreep);
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        જો (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.ઉમેરો(nodep->varScopep());
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_CCall* અચલ nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        અચલ auto& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        જો (cfuncps.empty()) {
            visitCalledCFunc(nodep->funcp());
        } નહીંતર {
            માટે (AstCFunc* અચલ cfuncp : cfuncps) visitCalledCFunc(cfuncp);
        }
    }
    શૂન્ય visit(AstCFunc* અચલ nodep) override {
        અચલ auto& કિંમત = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        જો (કિંમત[0].m_state != CFuncCache::UNINITIALIZED
            || કિંમત[1].m_state != CFuncCache::UNINITIALIZED) {
            પાછા;
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_પદાવલિ_Stmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, Ast_ગાંઠ_Stmt*& postUpdatesr,
                          std::map<અચલ AstVarScope*, std::set<Ast_Sen_વૃક્ષ*>>& externalDomains)
        : m_scopeTopp{nodep->ટોચ_Scopep()->scopep()}
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

TimingKit prepareTiming(AstNetlist* અચલ netlistp) {
    જો (!v3Global.usesTiming()) પાછા {};
    LogicByScope lbs;
    Ast_ગાંઠ_Stmt* postUpdates = nullptr;
    std::map<અચલ AstVarScope*, std::set<Ast_Sen_વૃક્ષ*>> externalDomains;
    { AwaitVisitor{netlistp, lbs, postUpdates, externalDomains}; }
    પાછા {std::move(lbs), postUpdates, std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

// Transform all forked processes into functions
વર્ગ TransformForksVisitor final : જાહેર VNVisitor {
    // NODE STATE
    //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current fork.
    અચલ VNUser1InUse m_inuser1;

    // STATE
    તાર્કિક m_inClass = ખોટું;  // Are we in a class?
    તાર્કિક m_beginHasAwaits = ખોટું;  // Does the current begin have awaits?
    AstFork* m_forkp = nullptr;  // Current fork
    AstCFunc* m_funcp = nullptr;  // Current function

    // METHODS
    // Remap local vars referenced by the given fork function
    // TODO: We should only pass variables to the fork that are
    // live in the fork body, but for that we need a proper data
    // flow analysis framework which we don't have at the moment
    શૂન્ય remapLocals(AstCFunc* અચલ funcp, AstCCall* અચલ callp) {
        અચલ VNUser2InUse user2InUse;  // AstVarScope -> AstVarScope: var to remap to
        funcp->foreach([&](Ast_ગાંઠ_Var_Ref* refp) {
            AstVar* અચલ varp = refp->varp();
            AstVarScope* અચલ vscp = refp->varScopep();
            AstBasicDType* અચલ dtypep = varp->dtypep()->basicp();
            // If not a fork..join, copy. All write refs should've been handled by V3Fork
            તાર્કિક pass_By_કિંમત = !m_forkp->joinType().join();
            જો (vscp->fork_By_કિંમત()) {
                // Set just before the fork, for its processes to get by value
                pass_By_કિંમત = સાચું;
            } નહીંતર જો (!varp->isFuncLocal()) {
                // Not func local. Its lifetime is longer than the forked process. Skip
                પાછા;
            } નહીંતર જો (!varp->user1()) {
                // Not declared before the fork. It cannot outlive the forked process
                પાછા;
            } નહીંતર જો (dtypep && dtypep->isForkSync()) {
                // We can just pass it by value to the new function
                pass_By_કિંમત = સાચું;
            }
            // Remap the reference
            જો (!vscp->user2p()) {
                // Clone the var to the new function
                AstVar* અચલ newvarp
                    = નવું AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                newvarp->funcLocal(સાચું);
                newvarp->direction(pass_By_કિંમત ? VDirection::આગત : VDirection::REF);
                funcp->addArgsp(newvarp);
                AstVarScope* અચલ newvscp
                    = નવું AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->addVarsp(newvscp);
                vscp->user2p(newvscp);
                callp->addArgsp(નવું AstVarRef{refp->fileline(), vscp,
                                              pass_By_કિંમત ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* અચલ newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER(m_inClass);
        m_inClass = VN_IS(nodep, Class);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        m_funcp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstVar* nodep) override {
        જો (!m_forkp) nodep->user1(સાચું);
    }
    શૂન્ય visit(AstFork* nodep) override {
        જો (m_forkp) પાછા;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        Ast_ગાંઠ* resp = nullptr;
        જો (Ast_ગાંઠ* અચલ declsp = nodep->declsp()) {
            resp = Ast_ગાંઠ::addNext(resp, declsp->unlinkFrBackWithNext());
        }
        જો (Ast_ગાંઠ* અચલ stmtsp = nodep->stmtsp()) {
            resp = Ast_ગાંઠ::addNext(resp, stmtsp->unlinkFrBackWithNext());
        }
        જ્યારે (AstBegin* અચલ beginp = nodep->forksp()) {
            જો (Ast_ગાંઠ* અચલ declsp = beginp->declsp()) {
                resp = Ast_ગાંઠ::addNext(resp, declsp->unlinkFrBackWithNext());
            }
            જો (Ast_ગાંઠ* અચલ stmtsp = beginp->stmtsp()) {
                resp = Ast_ગાંઠ::addNext(resp, stmtsp->unlinkFrBackWithNext());
            }
            VL_DO_DANGLING(ધકેલો_Deletep(beginp->unlinkFrBack()), beginp);
        }
        જો (resp) {
            nodep->replaceWith(resp);
        } નહીંતર {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstBegin* nodep) override {
        UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
        // Start with children, so later we only find awaits that are actually in this begin
        m_beginHasAwaits = ખોટું;
        iterateChildrenConst(nodep);
        જો (!nodep->stmtsp()) પાછા;
        જો (!m_beginHasAwaits && !nodep->needProcess()) પાછા;

        UASSERT_OBJ(!nodep->name().empty(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        File_લીટી* અચલ flp = nodep->fileline();
        AstCFunc* અચલ newfuncp = નવું AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->addNextHere(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->declPrivate(સાચું);
        // Create the call to the function
        AstCCall* અચલ callp = નવું AstCCall{flp, newfuncp};
        callp->dtypeSetVoid();
        // If we're in a class, add a vlSymsp arg
        જો (m_inClass) {
            newfuncp->addStmtsp(નવું AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        જો (Ast_ગાંઠ* અચલ declsp = nodep->declsp()) {
            newfuncp->addStmtsp(declsp->unlinkFrBackWithNext());
        }
        જો (Ast_ગાંઠ* અચલ stmtsp = nodep->stmtsp()) {
            newfuncp->addStmtsp(stmtsp->unlinkFrBackWithNext());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->addStmtsp(callp->makeStmt());
        // Propagate if needs process
        જો (nodep->needProcess()) newfuncp->setNeedProcess();
        remapLocals(newfuncp, callp);
    }
    શૂન્ય visit(AstCAwait* nodep) override {
        m_beginHasAwaits = સાચું;
        iterateChildrenConst(nodep);
    }

    //--------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = મૂળભૂત;
};

શૂન્ય transformForks(AstNetlist* અચલ netlistp) {
    જો (!v3Global.usesTiming()) પાછા;
    { TransformForksVisitor{netlistp}; }
    V3Global::dump_Check_Global_વૃક્ષ("transform_forks", 0, dump_વૃક્ષ_Either_Level() >= 3);
}

}  // namespace V3Sched
