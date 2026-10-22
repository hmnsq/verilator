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

نامخلا V3Sched {

//============================================================================
// Trigger indices of the external domains of each variable

std::map<ثابت AstVarScope*, std::سمتیہ<uint32_t>>
TimingKit::externalTriggerIndices(ثابت TriggerKit& trigKit) ثابت {
    std::map<ثابت AstVarScope*, std::سمتیہ<uint32_t>> نتیجہ;
    برائے (ثابت خودکار& vscpDomains : m_externalDomains) {
        std::سمتیہ<uint32_t>& indices = نتیجہ[vscpDomains.پہلا];
        برائے (ثابت Ast_Sen_درخت* ثابت domainp : vscpDomains.دوسرا) {
            برائے (ثابت Ast_Sen_شے* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_شے)) {
                indices.ڈالو_عقب(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    واپس نتیجہ;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    اگر (!m_resumeFuncp) {
        اگر (m_lbs.خالی_خانے()) واپس خالی_اشاریہ;
        // Create global resume function
        AstNetlist* ثابت netlistp = v3Global.rootp();
        AstScope* ثابت scopeTopp = netlistp->چوٹی_Scopep()->scopep();
        m_resumeFuncp = نیا AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(سچ);
        m_resumeFuncp->isLoose(سچ);
        m_resumeFuncp->isConst(جھوٹ);
        m_resumeFuncp->decl_نجی(سچ);
        scopeTopp->جوڑو_Blocksp(m_resumeFuncp);

        برائے (ثابت خودکار& اندراج_2 : m_lbs) {
            Ast_فعال* ثابت activep = اندراج_2.m_activep;
            activep->foreach([یہ](AstCMethodHard* ثابت exprp) {
                اگر (exprp->طریقہ() != VCMethod::SCHED_RESUME) واپس;
                Ast_گرہ_اظہاریہ* ثابت fromp = exprp->fromp();
                اگر (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    واپس;
                }
                AstCMethodHard* ثابت move_مقصد_Resumep = نیا AstCMethodHard{
                    fromp->fileline(), fromp->clone_درخت(جھوٹ),
                    VCMethod::SCHED_MOVE_مقصد_RESUME_قطار,
                    exprp->pinsp() ? exprp->pinsp()->clone_درخت(سچ) : خالی_اشاریہ};
                move_مقصد_Resumep->dtype_سیٹ_Void();
                m_resumeFuncp->جوڑو_Stmtsp(move_مقصد_Resumep->makeStmt());
            });
        }

        // Put all the timing actives in the resume function
        AstIf* dlyShedIfp = خالی_اشاریہ;
        برائے (ثابت خودکار& اندراج_2 : m_lbs) {
            Ast_فعال* ثابت activep = اندراج_2.m_activep;
            // Resume time delays last. For no particular reason other than
            // that's what we used to do prior to proper #0 support.
            AstVarRef* ثابت schedrefp = VN_AS(
                VN_AS(VN_AS(activep->stmtsp(), Stmt_اظہاریہ)->exprp(), CMethodHard)->fromp(), VarRef);

            Ast_گرہ* ثابت actionp = activep->stmtsp()->unlink_Fr_عقب_With_اگلا();
            اگر (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                File_سطر* ثابت flp = trigVscp->fileline();
                ثابت std::سیٹ<uint32_t>& triggers = اندراج_2.m_triggers;
                AstTriggered* ثابت condp
                    = نیا AstTriggered{flp, trigVscp, {triggers.begin(), triggers.انتہا()}};
                dlyShedIfp = نیا AstIf{flp, condp};
                dlyShedIfp->جوڑو_Thensp(actionp);
            } ورنہ {
                m_resumeFuncp->جوڑو_Stmtsp(actionp);
            }
        }
        اگر (dlyShedIfp) m_resumeFuncp->جوڑو_Stmtsp(dlyShedIfp);

        // These are now spent, oispose of now empty AstActive instances
        m_lbs.deleteActives();
    }
    AstCCall* ثابت callp = نیا AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
    callp->dtype_سیٹ_Void();
    واپس callp;
}

AstVarScope* TimingKit::getDelayScheduler(AstNetlist* ثابت /*netlistp*/) {
    برائے (ثابت خودکار& اندراج_2 : m_lbs) {
        Ast_فعال* ثابت ap = اندراج_2.m_activep;
        // TODO: this triple VN_AS expression is ridiculous
        AstVarRef* ثابت schedrefp
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), Stmt_اظہاریہ)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* ثابت vscp = schedrefp->varScopep();
        اگر (vscp->dtypep()->basicp()->isDelayScheduler()) واپس vscp;
    }
    // None found. Design doesn't use any time delays
    واپس خالی_اشاریہ;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    اگر (!m_readyFuncp) {
        برائے (ثابت خودکار& اندراج_2 : m_lbs) {
            Ast_فعال* ثابت activep = اندراج_2.m_activep;
            خودکار* ثابت resumep = VN_AS(VN_AS(activep->stmtsp(), Stmt_اظہاریہ)->exprp(), CMethodHard);
            UASSERT_OBJ(!resumep->nextp(), resumep, "Should be the only statement here");
            AstVarScope* ثابت schedulerp = VN_AS(resumep->fromp(), VarRef)->varScopep();
            UASSERT_OBJ(schedulerp->dtypep()->basicp()->isDelayScheduler()
                            || schedulerp->dtypep()->basicp()->isTriggerScheduler()
                            || schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler(),
                        schedulerp, "Unexpected type");
            اگر (!schedulerp->dtypep()->basicp()->isTriggerScheduler()) جاری;
            // Create the global ready function only if we have trigger schedulers
            اگر (!m_readyFuncp) {
                AstNetlist* ثابت netlistp = v3Global.rootp();
                AstScope* ثابت scopeTopp = netlistp->چوٹی_Scopep()->scopep();
                m_readyFuncp = نیا AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(سچ);
                m_readyFuncp->isLoose(سچ);
                m_readyFuncp->isConst(جھوٹ);
                m_readyFuncp->decl_نجی(سچ);
                scopeTopp->جوڑو_Blocksp(m_readyFuncp);
            }

            File_سطر* ثابت flp = activep->sentreep()->fileline();

            // Create an 'AstIf' sensitive to the suspending triggers
            ثابت std::سیٹ<uint32_t>& triggers = اندراج_2.m_triggers;
            AstTriggered* ثابت condp
                = نیا AstTriggered{flp, trigVscp, {triggers.begin(), triggers.انتہا()}};
            AstIf* ثابت ifp = نیا AstIf{flp, condp};
            m_readyFuncp->جوڑو_Stmtsp(ifp);

            // Mark as ready the processes resumed on this sensitivity expression
            AstVarRef* ثابت refp = نیا AstVarRef{flp, schedulerp, VAccess::READWRITE};
            AstCMethodHard* ثابت callp = نیا AstCMethodHard{flp, refp, VCMethod::SCHED_READY};
            callp->dtype_سیٹ_Void();
            اگر (resumep->pinsp()) callp->جوڑو_Pinsp(resumep->pinsp()->clone_درخت(جھوٹ));
            ifp->جوڑو_Thensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        اگر (!m_readyFuncp) واپس خالی_اشاریہ;
    }
    AstCCall* ثابت callp = نیا AstCCall{m_readyFuncp->fileline(), m_readyFuncp};
    callp->dtype_سیٹ_Void();
    واپس callp;
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

جماعت AwaitVisitor final : عوامی VNVisitor {
    // NODE STATE
    //  AstSenTree::user1()  -> bool.               Set true if the sentree has been visited.
    //  AstCFunc::user1()    -> AstUser1Allocator.  See alocator below
    ثابت VNUser1_اندر_Use m_inuser1;

    // STATE
    منطقی m_اندر_Process = جھوٹ;  // Are we in a process?
    منطقی m_gatherVars = جھوٹ;  // Should we gather vars in m_writtenBySuspendable?
    AstScope* ثابت m_scopeTopp;  // Scope at the top
    LogicByScope& m_lbs;  // Timing resume actives
    Ast_گرہ_Stmt*& m_postUpdatesr;  // Post updates for the trigger eval function
    // Additional var sensitivities
    std::map<ثابت AstVarScope*, std::سیٹ<Ast_Sen_درخت*>>& m_externalDomains;
    std::unique_ptr<V3_Class_گراف>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::سیٹ<Ast_Sen_درخت*> m_processDomains;  // Sentrees from the current process
    // Variables written by suspendable processes
    std::سیٹ<AstVarScope*> m_writtenBySuspendable;
    ساخت CFuncCache final {
        std::سیٹ<Ast_Sen_درخت*> m_processDomains;  // What shall be added to m_processDomains
        std::سیٹ<AstVarScope*>
            m_writtenBySuspendable;  // What shall be added to m_writtenBySuspendable
        std::سیٹ<AstCFunc*> m_includes;  // CFuncs whose CFuncCache shall be included into this
        // m_includes does not need to keep bool with m_gatherVars to know which cache shall be
        // used since:
        //  Only possible transition in a graph is from `!m_gatherVars` to `m_gatherVars`
        //  (in visit(AstFork*) there is a only possible transition, visit(AstNodeProcedure*) is
        //  not considered since it does not occur during a graph traversal)
        //  therefore, if change of state occurs it is certain that the first node after transition
        //  won't be in visiting state
        //  therefore, every node in a m_includes shall take value of m_gatherVars under which
        //  current CFuncCache is
        شمار State : uint8_t {
            UNINITIALIZED = 0,  // Not initialized members are empty
            VISITING,  // Visiting - needed for breaking recursion
            INITIALIZED,  // Members contains correct values
        } m_state  // Current state of Cache
            = UNINITIALIZED;
    };
    std::سمتیہ<ثابت AstCFunc*> m_call_ڈھیر;  // Current callstack of AstCFuncs
    // Caches how visiting the function with given value of m_gatherVars changes
    // m_processDomains and m_writtenBySuspendable - only accessed from visit(AstCFunc* nodep)
    AstUser1Allocator<AstCFunc, std::array<CFuncCache, 2>> m_cfuncsCache;
    // Count uses of not inlined writes to signals in suspendables
    VDouble0 m_not_Inlined_Writes_اندر_Suspendable_Usage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    خالی جوڑو_Resume_Pins(AstCMethodHard* ثابت resumep, Ast_گرہ_اظہاریہ* pinsp) {
        AstCExpr* ثابت exprp = VN_CAST(pinsp, CExpr);
        Ast_متن* ثابت textp = VN_CAST(exprp->nodesp(), متن_2);
        اگر (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            اگر ((pinsp = VN_CAST(pinsp->nextp(), گرہ_اظہاریہ))) {
                resumep->جوڑو_Pinsp(pinsp->clone_درخت(جھوٹ));
            }
        } ورنہ {
            resumep->جوڑو_Pinsp(pinsp->clone_درخت(جھوٹ));
        }
    }
    خالی جوڑو_CFunc_Cached_اقدار(ثابت AstCFunc* ثابت cfuncp,
                              std::unordered_سیٹ<ثابت AstCFunc*>& دیکھا) {
        اگر (!دیکھا.داخل_کرو(cfuncp).دوسرا) واپس;
        CFuncCache& قدر = m_cfuncsCache(cfuncp)[static_cast<سائز_t>(m_gatherVars)];
        ثابت سائز_t initial_سائز = m_writtenBySuspendable.سائز();
        m_writtenBySuspendable.داخل_کرو(قدر.m_writtenBySuspendable.begin(),
                                      قدر.m_writtenBySuspendable.انتہا());
        m_not_Inlined_Writes_اندر_Suspendable_Usage += m_writtenBySuspendable.سائز() - initial_سائز;
        m_processDomains.داخل_کرو(قدر.m_processDomains.begin(), قدر.m_processDomains.انتہا());
        برائے (ثابت AstCFunc* ثابت includedp : قدر.m_includes) {
            جوڑو_CFunc_Cached_اقدار(includedp, دیکھا);
        }
    }
    // Create an active with a timing scheduler resume() call
    خالی create_Resume_فعال(AstCAwait* ثابت awaitp) {
        خودکار* ثابت methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* ثابت schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        Ast_Sen_درخت* ثابت sentreep = awaitp->sentreep();
        File_سطر* ثابت flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        خودکار* ثابت resumep = نیا AstCMethodHard{
            flp, نیا AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtype_سیٹ_Void();
        اگر (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            اگر (Ast_گرہ* ثابت dbginfop = methodp->pinsp()->nextp()) {
                اگر (methodp->pinsp()) جوڑو_Resume_Pins(resumep, static_cast<Ast_گرہ_اظہاریہ*>(dbginfop));
            }
        } ورنہ اگر (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            خودکار* ثابت postp = resumep->clone_درخت(جھوٹ);
            postp->طریقہ(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = Ast_گرہ::جوڑو_اگلا(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        Ast_فعال* ثابت activep = نیا Ast_فعال{flp, "_timing", sentreep};
        activep->جوڑو_Stmtsp(resumep->makeStmt());
        m_lbs.جوڑو(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    خالی دورہ_Called_CFunc(AstCFunc* ثابت nodep) {
        اگر (!m_اندر_Process) واپس;  // Skip if not in process - nothing would be added to any set
        ثابت سائز_t cache_کلید = static_cast<سائز_t>(m_gatherVars);
        CFuncCache& قدر = m_cfuncsCache(nodep)[cache_کلید];
        انتخاب (قدر.m_state) {
        حالت CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            قدر.m_state = CFuncCache::VISITING;
            m_call_ڈھیر.ڈالو_عقب(nodep);
            iterateChildren(nodep);
            m_call_ڈھیر.نکالو_عقب();
            قدر.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::تبادلہ(m_processDomains, قدر.m_processDomains);
            std::تبادلہ(m_writtenBySuspendable, قدر.m_writtenBySuspendable);
        } توڑو;
        حالت CFuncCache::VISITING: {
            برائے (سائز_t i = m_call_ڈھیر.سائز() - 1; m_call_ڈھیر.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_ڈھیر[i])[cache_کلید].m_includes.داخل_کرو(nodep);
            }
            واپس;  // Break recursion
        }
        حالت CFuncCache::INITIALIZED: توڑو;
        }
        std::unordered_سیٹ<ثابت AstCFunc*> دیکھا;
        جوڑو_CFunc_Cached_اقدار(nodep, دیکھا);
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_Procedure* ثابت nodep) override {
        UASSERT_OBJ(!m_اندر_Process && !m_gatherVars && m_processDomains.خالی_خانے()
                        && m_writtenBySuspendable.خالی_خانے(),
                    nodep, "Process in process?");
        m_اندر_Process = سچ;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        برائے (AstVarScope* ثابت vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].داخل_کرو(m_processDomains.begin(), m_processDomains.انتہا());
            vscp->varp()->سیٹ_Written_By_Suspendable();
        }
        m_processDomains.clear();
        m_writtenBySuspendable.clear();
        m_اندر_Process = جھوٹ;
        m_gatherVars = جھوٹ;
    }
    خالی دورہ(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        اگر (m_اندر_Process) m_gatherVars = سچ;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    خالی دورہ(AstCAwait* nodep) override {
        اگر (Ast_Sen_درخت* ثابت sentreep = nodep->sentreep()) {
            اگر (!sentreep->user1_سیٹ_Once()) create_Resume_فعال(nodep);
            اگر (m_اندر_Process) m_processDomains.داخل_کرو(sentreep);
        }
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_Var_Ref* nodep) override {
        اگر (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.داخل_کرو(nodep->varScopep());
        }
    }
    خالی دورہ(Ast_گرہ_CCall* ثابت nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ثابت خودکار& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        اگر (cfuncps.خالی_خانے()) {
            دورہ_Called_CFunc(nodep->funcp());
        } ورنہ {
            برائے (AstCFunc* ثابت cfuncp : cfuncps) دورہ_Called_CFunc(cfuncp);
        }
    }
    خالی دورہ(AstCFunc* ثابت nodep) override {
        ثابت خودکار& قدر = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        اگر (قدر[0].m_state != CFuncCache::UNINITIALIZED
            || قدر[1].m_state != CFuncCache::UNINITIALIZED) {
            واپس;
        }
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_اظہاریہ_Stmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, Ast_گرہ_Stmt*& postUpdatesr,
                          std::map<ثابت AstVarScope*, std::سیٹ<Ast_Sen_درخت*>>& externalDomains)
        : m_scopeTopp{nodep->چوٹی_Scopep()->scopep()}
        , m_lbs{lbs}
        , m_postUpdatesr{postUpdatesr}
        , m_externalDomains{externalDomains}
        , m_classGraphp{V3_Class_گراف::build(nodep)} {
        iterate(nodep);
    }
    ~AwaitVisitor() override {
        V3Stats::جوڑو_Stat("Scheduling, count of non-inlined signal writes in suspendables",
                         m_not_Inlined_Writes_اندر_Suspendable_Usage);
    }
};

TimingKit prepareTiming(AstNetlist* ثابت netlistp) {
    اگر (!v3Global.استعمال_کرتا_Timing()) واپس {};
    LogicByScope lbs;
    Ast_گرہ_Stmt* postUpdates = خالی_اشاریہ;
    std::map<ثابت AstVarScope*, std::سیٹ<Ast_Sen_درخت*>> externalDomains;
    { AwaitVisitor{netlistp, lbs, postUpdates, externalDomains}; }
    واپس {std::move(lbs), postUpdates, std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

// Transform all forked processes into functions
جماعت TransformForksVisitor final : عوامی VNVisitor {
    // NODE STATE
    //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current fork.
    ثابت VNUser1_اندر_Use m_inuser1;

    // STATE
    منطقی m_اندر_Class = جھوٹ;  // Are we in a class?
    منطقی m_beginHasAwaits = جھوٹ;  // Does the current begin have awaits?
    AstFork* m_forkp = خالی_اشاریہ;  // Current fork
    AstCFunc* m_funcp = خالی_اشاریہ;  // Current function

    // METHODS
    // Remap local vars referenced by the given fork function
    // TODO: We should only pass variables to the fork that are
    // live in the fork body, but for that we need a proper data
    // flow analysis framework which we don't have at the moment
    خالی remapLocals(AstCFunc* ثابت funcp, AstCCall* ثابت callp) {
        ثابت VNUser2_اندر_Use user2_اندر_Use;  // AstVarScope -> AstVarScope: var to remap to
        funcp->foreach([&](Ast_گرہ_Var_Ref* refp) {
            AstVar* ثابت varp = refp->varp();
            AstVarScope* ثابت vscp = refp->varScopep();
            AstBasicDType* ثابت dtypep = varp->dtypep()->basicp();
            // If not a fork..join, copy. All write refs should've been handled by V3Fork
            منطقی pass_By_قدر = !m_forkp->joinType().join();
            اگر (vscp->fork_By_قدر()) {
                // Set just before the fork, for its processes to get by value
                pass_By_قدر = سچ;
            } ورنہ اگر (!varp->isFuncLocal()) {
                // Not func local. Its lifetime is longer than the forked process. Skip
                واپس;
            } ورنہ اگر (!varp->user1()) {
                // Not declared before the fork. It cannot outlive the forked process
                واپس;
            } ورنہ اگر (dtypep && dtypep->isForkSync()) {
                // We can just pass it by value to the new function
                pass_By_قدر = سچ;
            }
            // Remap the reference
            اگر (!vscp->user2p()) {
                // Clone the var to the new function
                AstVar* ثابت newvarp
                    = نیا AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                newvarp->funcLocal(سچ);
                newvarp->direction(pass_By_قدر ? VDirection::داخلہ : VDirection::REF);
                funcp->جوڑو_Argsp(newvarp);
                AstVarScope* ثابت newvscp
                    = نیا AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->جوڑو_Varsp(newvscp);
                vscp->user2p(newvscp);
                callp->جوڑو_Argsp(نیا AstVarRef{refp->fileline(), vscp,
                                              pass_By_قدر ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* ثابت newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_اندر_Class);
        m_اندر_Class = VN_IS(nodep, Class);
        iterateChildren(nodep);
    }
    خالی دورہ(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        m_funcp = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(AstVar* nodep) override {
        اگر (!m_forkp) nodep->user1(سچ);
    }
    خالی دورہ(AstFork* nodep) override {
        اگر (m_forkp) واپس;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        Ast_گرہ* resp = خالی_اشاریہ;
        اگر (Ast_گرہ* ثابت declsp = nodep->declsp()) {
            resp = Ast_گرہ::جوڑو_اگلا(resp, declsp->unlink_Fr_عقب_With_اگلا());
        }
        اگر (Ast_گرہ* ثابت stmtsp = nodep->stmtsp()) {
            resp = Ast_گرہ::جوڑو_اگلا(resp, stmtsp->unlink_Fr_عقب_With_اگلا());
        }
        جب (AstBegin* ثابت beginp = nodep->forksp()) {
            اگر (Ast_گرہ* ثابت declsp = beginp->declsp()) {
                resp = Ast_گرہ::جوڑو_اگلا(resp, declsp->unlink_Fr_عقب_With_اگلا());
            }
            اگر (Ast_گرہ* ثابت stmtsp = beginp->stmtsp()) {
                resp = Ast_گرہ::جوڑو_اگلا(resp, stmtsp->unlink_Fr_عقب_With_اگلا());
            }
            VL_DO_DANGLING(ڈالو_Deletep(beginp->unlink_Fr_عقب()), beginp);
        }
        اگر (resp) {
            nodep->replaceWith(resp);
        } ورنہ {
            nodep->unlink_Fr_عقب();
        }
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstBegin* nodep) override {
        UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
        // Start with children, so later we only find awaits that are actually in this begin
        m_beginHasAwaits = جھوٹ;
        iterateChildrenConst(nodep);
        اگر (!nodep->stmtsp()) واپس;
        اگر (!m_beginHasAwaits && !nodep->needProcess()) واپس;

        UASSERT_OBJ(!nodep->name().خالی_خانے(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        File_سطر* ثابت flp = nodep->fileline();
        AstCFunc* ثابت newfuncp = نیا AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->جوڑو_اگلا_یہاں(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->decl_نجی(سچ);
        // Create the call to the function
        AstCCall* ثابت callp = نیا AstCCall{flp, newfuncp};
        callp->dtype_سیٹ_Void();
        // If we're in a class, add a vlSymsp arg
        اگر (m_اندر_Class) {
            newfuncp->جوڑو_Stmtsp(نیا AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        اگر (Ast_گرہ* ثابت declsp = nodep->declsp()) {
            newfuncp->جوڑو_Stmtsp(declsp->unlink_Fr_عقب_With_اگلا());
        }
        اگر (Ast_گرہ* ثابت stmtsp = nodep->stmtsp()) {
            newfuncp->جوڑو_Stmtsp(stmtsp->unlink_Fr_عقب_With_اگلا());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->جوڑو_Stmtsp(callp->makeStmt());
        // Propagate if needs process
        اگر (nodep->needProcess()) newfuncp->سیٹ_Need_Process();
        remapLocals(newfuncp, callp);
    }
    خالی دورہ(AstCAwait* nodep) override {
        m_beginHasAwaits = سچ;
        iterateChildrenConst(nodep);
    }

    //--------------------
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = طےشدہ;
};

خالی transformForks(AstNetlist* ثابت netlistp) {
    اگر (!v3Global.استعمال_کرتا_Timing()) واپس;
    { TransformForksVisitor{netlistp}; }
    V3Global::dump_Check_Global_درخت("transform_forks", 0, dump_درخت_Either_Level() >= 3);
}

}  // namespace V3Sched
