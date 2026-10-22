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

std::map<ثابت AstVarScope*, std::ويڪٽر<uint32_t>>
TimingKit::externalTriggerIndices(ثابت TriggerKit& trigKit) ثابت {
    std::map<ثابت AstVarScope*, std::ويڪٽر<uint32_t>> نتيجو;
    لكل (ثابت خودڪار& vscpDomains : m_externalDomains) {
        std::ويڪٽر<uint32_t>& indices = نتيجو[vscpDomains.پهريون];
        لكل (ثابت Ast_Sen_وڻ* ثابت domainp : vscpDomains.ٻيو) {
            لكل (ثابت Ast_Sen_شي* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_شي)) {
                indices.ڌڪو_پٺ(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    أرجع نتيجو;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    إذا (!m_resumeFuncp) {
        إذا (m_lbs.خالي_جايون()) أرجع خالي_اشارو;
        // Create global resume function
        AstNetlist* ثابت netlistp = v3Global.rootp();
        AstScope* ثابت scopeTopp = netlistp->چوٽي_Scopep()->scopep();
        m_resumeFuncp = جديد AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(سچ);
        m_resumeFuncp->isLoose(سچ);
        m_resumeFuncp->isConst(خطأ);
        m_resumeFuncp->decl_ذاتي(سچ);
        scopeTopp->جوڙيو_Blocksp(m_resumeFuncp);

        لكل (ثابت خودڪار& داخلا_2 : m_lbs) {
            Ast_سرگرم* ثابت activep = داخلا_2.m_activep;
            activep->foreach([هذا](AstCMethodHard* ثابت exprp) {
                إذا (exprp->طريقو() != VCMethod::SCHED_RESUME) أرجع;
                Ast_ڳنڍ_اظهار* ثابت fromp = exprp->fromp();
                إذا (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    أرجع;
                }
                AstCMethodHard* ثابت move_منزل_Resumep = جديد AstCMethodHard{
                    fromp->fileline(), fromp->clone_وڻ(خطأ),
                    VCMethod::SCHED_MOVE_منزل_RESUME_قطار,
                    exprp->pinsp() ? exprp->pinsp()->clone_وڻ(سچ) : خالي_اشارو};
                move_منزل_Resumep->dtype_سيٽ_Void();
                m_resumeFuncp->جوڙيو_Stmtsp(move_منزل_Resumep->makeStmt());
            });
        }

        // Put all the timing actives in the resume function
        AstIf* dlyShedIfp = خالي_اشارو;
        لكل (ثابت خودڪار& داخلا_2 : m_lbs) {
            Ast_سرگرم* ثابت activep = داخلا_2.m_activep;
            // Resume time delays last. For no particular reason other than
            // that's what we used to do prior to proper #0 support.
            AstVarRef* ثابت schedrefp = VN_AS(
                VN_AS(VN_AS(activep->stmtsp(), Stmt_اظهار)->exprp(), CMethodHard)->fromp(), VarRef);

            Ast_ڳنڍ* ثابت actionp = activep->stmtsp()->unlink_Fr_پٺ_With_ايندڙ();
            إذا (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                File_سٽ* ثابت flp = trigVscp->fileline();
                ثابت std::سيٽ<uint32_t>& triggers = داخلا_2.m_triggers;
                AstTriggered* ثابت condp
                    = جديد AstTriggered{flp, trigVscp, {triggers.begin(), triggers.پڇاڙي()}};
                dlyShedIfp = جديد AstIf{flp, condp};
                dlyShedIfp->جوڙيو_Thensp(actionp);
            } وإلا {
                m_resumeFuncp->جوڙيو_Stmtsp(actionp);
            }
        }
        إذا (dlyShedIfp) m_resumeFuncp->جوڙيو_Stmtsp(dlyShedIfp);

        // These are now spent, oispose of now empty AstActive instances
        m_lbs.deleteActives();
    }
    AstCCall* ثابت callp = جديد AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
    callp->dtype_سيٽ_Void();
    أرجع callp;
}

AstVarScope* TimingKit::getDelayScheduler(AstNetlist* ثابت /*netlistp*/) {
    لكل (ثابت خودڪار& داخلا_2 : m_lbs) {
        Ast_سرگرم* ثابت ap = داخلا_2.m_activep;
        // TODO: this triple VN_AS expression is ridiculous
        AstVarRef* ثابت schedrefp
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), Stmt_اظهار)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* ثابت vscp = schedrefp->varScopep();
        إذا (vscp->dtypep()->basicp()->isDelayScheduler()) أرجع vscp;
    }
    // None found. Design doesn't use any time delays
    أرجع خالي_اشارو;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    إذا (!m_readyFuncp) {
        لكل (ثابت خودڪار& داخلا_2 : m_lbs) {
            Ast_سرگرم* ثابت activep = داخلا_2.m_activep;
            خودڪار* ثابت resumep = VN_AS(VN_AS(activep->stmtsp(), Stmt_اظهار)->exprp(), CMethodHard);
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
                AstScope* ثابت scopeTopp = netlistp->چوٽي_Scopep()->scopep();
                m_readyFuncp = جديد AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(سچ);
                m_readyFuncp->isLoose(سچ);
                m_readyFuncp->isConst(خطأ);
                m_readyFuncp->decl_ذاتي(سچ);
                scopeTopp->جوڙيو_Blocksp(m_readyFuncp);
            }

            File_سٽ* ثابت flp = activep->sentreep()->fileline();

            // Create an 'AstIf' sensitive to the suspending triggers
            ثابت std::سيٽ<uint32_t>& triggers = داخلا_2.m_triggers;
            AstTriggered* ثابت condp
                = جديد AstTriggered{flp, trigVscp, {triggers.begin(), triggers.پڇاڙي()}};
            AstIf* ثابت ifp = جديد AstIf{flp, condp};
            m_readyFuncp->جوڙيو_Stmtsp(ifp);

            // Mark as ready the processes resumed on this sensitivity expression
            AstVarRef* ثابت refp = جديد AstVarRef{flp, schedulerp, VAccess::READWRITE};
            AstCMethodHard* ثابت callp = جديد AstCMethodHard{flp, refp, VCMethod::SCHED_READY};
            callp->dtype_سيٽ_Void();
            إذا (resumep->pinsp()) callp->جوڙيو_Pinsp(resumep->pinsp()->clone_وڻ(خطأ));
            ifp->جوڙيو_Thensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        إذا (!m_readyFuncp) أرجع خالي_اشارو;
    }
    AstCCall* ثابت callp = جديد AstCCall{m_readyFuncp->fileline(), m_readyFuncp};
    callp->dtype_سيٽ_Void();
    أرجع callp;
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

صنف AwaitVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstSenTree::user1()  -> bool.               Set true if the sentree has been visited.
    //  AstCFunc::user1()    -> AstUser1Allocator.  See alocator below
    ثابت VNUser1_اندريون_Use m_inuser1;

    // STATE
    منطقي m_اندريون_Process = خطأ;  // Are we in a process?
    منطقي m_gatherVars = خطأ;  // Should we gather vars in m_writtenBySuspendable?
    AstScope* ثابت m_scopeTopp;  // Scope at the top
    LogicByScope& m_lbs;  // Timing resume actives
    Ast_ڳنڍ_Stmt*& m_postUpdatesr;  // Post updates for the trigger eval function
    // Additional var sensitivities
    std::map<ثابت AstVarScope*, std::سيٽ<Ast_Sen_وڻ*>>& m_externalDomains;
    std::unique_ptr<V3_Class_گراف>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::سيٽ<Ast_Sen_وڻ*> m_processDomains;  // Sentrees from the current process
    // Variables written by suspendable processes
    std::سيٽ<AstVarScope*> m_writtenBySuspendable;
    بنية CFuncCache final {
        std::سيٽ<Ast_Sen_وڻ*> m_processDomains;  // What shall be added to m_processDomains
        std::سيٽ<AstVarScope*>
            m_writtenBySuspendable;  // What shall be added to m_writtenBySuspendable
        std::سيٽ<AstCFunc*> m_includes;  // CFuncs whose CFuncCache shall be included into this
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
    std::ويڪٽر<ثابت AstCFunc*> m_call_ڍير;  // Current callstack of AstCFuncs
    // Caches how visiting the function with given value of m_gatherVars changes
    // m_processDomains and m_writtenBySuspendable - only accessed from visit(AstCFunc* nodep)
    AstUser1Allocator<AstCFunc, std::array<CFuncCache, 2>> m_cfuncsCache;
    // Count uses of not inlined writes to signals in suspendables
    VDouble0 m_not_Inlined_Writes_اندريون_Suspendable_Usage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    فراغ جوڙيو_Resume_Pins(AstCMethodHard* ثابت resumep, Ast_ڳنڍ_اظهار* pinsp) {
        AstCExpr* ثابت exprp = VN_CAST(pinsp, CExpr);
        Ast_متن* ثابت textp = VN_CAST(exprp->nodesp(), متن_2);
        إذا (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            إذا ((pinsp = VN_CAST(pinsp->nextp(), ڳنڍ_اظهار))) {
                resumep->جوڙيو_Pinsp(pinsp->clone_وڻ(خطأ));
            }
        } وإلا {
            resumep->جوڙيو_Pinsp(pinsp->clone_وڻ(خطأ));
        }
    }
    فراغ جوڙيو_CFunc_Cached_قدرون(ثابت AstCFunc* ثابت cfuncp,
                              std::unordered_سيٽ<ثابت AstCFunc*>& گهميل) {
        إذا (!گهميل.وجهو(cfuncp).ٻيو) أرجع;
        CFuncCache& قدر = m_cfuncsCache(cfuncp)[static_cast<سائيز_t>(m_gatherVars)];
        ثابت سائيز_t initial_سائيز = m_writtenBySuspendable.سائيز();
        m_writtenBySuspendable.وجهو(قدر.m_writtenBySuspendable.begin(),
                                      قدر.m_writtenBySuspendable.پڇاڙي());
        m_not_Inlined_Writes_اندريون_Suspendable_Usage += m_writtenBySuspendable.سائيز() - initial_سائيز;
        m_processDomains.وجهو(قدر.m_processDomains.begin(), قدر.m_processDomains.پڇاڙي());
        لكل (ثابت AstCFunc* ثابت includedp : قدر.m_includes) {
            جوڙيو_CFunc_Cached_قدرون(includedp, گهميل);
        }
    }
    // Create an active with a timing scheduler resume() call
    فراغ create_Resume_سرگرم(AstCAwait* ثابت awaitp) {
        خودڪار* ثابت methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* ثابت schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        Ast_Sen_وڻ* ثابت sentreep = awaitp->sentreep();
        File_سٽ* ثابت flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        خودڪار* ثابت resumep = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtype_سيٽ_Void();
        إذا (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            إذا (Ast_ڳنڍ* ثابت dbginfop = methodp->pinsp()->nextp()) {
                إذا (methodp->pinsp()) جوڙيو_Resume_Pins(resumep, static_cast<Ast_ڳنڍ_اظهار*>(dbginfop));
            }
        } وإلا إذا (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            خودڪار* ثابت postp = resumep->clone_وڻ(خطأ);
            postp->طريقو(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = Ast_ڳنڍ::جوڙيو_ايندڙ(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        Ast_سرگرم* ثابت activep = جديد Ast_سرگرم{flp, "_timing", sentreep};
        activep->جوڙيو_Stmtsp(resumep->makeStmt());
        m_lbs.جوڙيو(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    فراغ گهمو_Called_CFunc(AstCFunc* ثابت nodep) {
        إذا (!m_اندريون_Process) أرجع;  // Skip if not in process - nothing would be added to any set
        ثابت سائيز_t cache_ڪنجي = static_cast<سائيز_t>(m_gatherVars);
        CFuncCache& قدر = m_cfuncsCache(nodep)[cache_ڪنجي];
        بدل (قدر.m_state) {
        حالة CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            قدر.m_state = CFuncCache::VISITING;
            m_call_ڍير.ڌڪو_پٺ(nodep);
            iterateChildren(nodep);
            m_call_ڍير.ڪڍو_پٺ();
            قدر.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::مٽا_سٽا(m_processDomains, قدر.m_processDomains);
            std::مٽا_سٽا(m_writtenBySuspendable, قدر.m_writtenBySuspendable);
        } اكسر;
        حالة CFuncCache::VISITING: {
            لكل (سائيز_t i = m_call_ڍير.سائيز() - 1; m_call_ڍير.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_ڍير[i])[cache_ڪنجي].m_includes.وجهو(nodep);
            }
            أرجع;  // Break recursion
        }
        حالة CFuncCache::INITIALIZED: اكسر;
        }
        std::unordered_سيٽ<ثابت AstCFunc*> گهميل;
        جوڙيو_CFunc_Cached_قدرون(nodep, گهميل);
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Procedure* ثابت nodep) override {
        UASSERT_OBJ(!m_اندريون_Process && !m_gatherVars && m_processDomains.خالي_جايون()
                        && m_writtenBySuspendable.خالي_جايون(),
                    nodep, "Process in process?");
        m_اندريون_Process = سچ;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        لكل (AstVarScope* ثابت vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].وجهو(m_processDomains.begin(), m_processDomains.پڇاڙي());
            vscp->varp()->سيٽ_Written_By_Suspendable();
        }
        m_processDomains.clear();
        m_writtenBySuspendable.clear();
        m_اندريون_Process = خطأ;
        m_gatherVars = خطأ;
    }
    فراغ گهمو(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        إذا (m_اندريون_Process) m_gatherVars = سچ;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    فراغ گهمو(AstCAwait* nodep) override {
        إذا (Ast_Sen_وڻ* ثابت sentreep = nodep->sentreep()) {
            إذا (!sentreep->user1_سيٽ_Once()) create_Resume_سرگرم(nodep);
            إذا (m_اندريون_Process) m_processDomains.وجهو(sentreep);
        }
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Var_Ref* nodep) override {
        إذا (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.وجهو(nodep->varScopep());
        }
    }
    فراغ گهمو(Ast_ڳنڍ_CCall* ثابت nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ثابت خودڪار& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        إذا (cfuncps.خالي_جايون()) {
            گهمو_Called_CFunc(nodep->funcp());
        } وإلا {
            لكل (AstCFunc* ثابت cfuncp : cfuncps) گهمو_Called_CFunc(cfuncp);
        }
    }
    فراغ گهمو(AstCFunc* ثابت nodep) override {
        ثابت خودڪار& قدر = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        إذا (قدر[0].m_state != CFuncCache::UNINITIALIZED
            || قدر[1].m_state != CFuncCache::UNINITIALIZED) {
            أرجع;
        }
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_اظهار_Stmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, Ast_ڳنڍ_Stmt*& postUpdatesr,
                          std::map<ثابت AstVarScope*, std::سيٽ<Ast_Sen_وڻ*>>& externalDomains)
        : m_scopeTopp{nodep->چوٽي_Scopep()->scopep()}
        , m_lbs{lbs}
        , m_postUpdatesr{postUpdatesr}
        , m_externalDomains{externalDomains}
        , m_classGraphp{V3_Class_گراف::build(nodep)} {
        iterate(nodep);
    }
    ~AwaitVisitor() override {
        V3Stats::جوڙيو_Stat("Scheduling, count of non-inlined signal writes in suspendables",
                         m_not_Inlined_Writes_اندريون_Suspendable_Usage);
    }
};

TimingKit prepareTiming(AstNetlist* ثابت netlistp) {
    إذا (!v3Global.استعمال_ڪري_Timing()) أرجع {};
    LogicByScope lbs;
    Ast_ڳنڍ_Stmt* postUpdates = خالي_اشارو;
    std::map<ثابت AstVarScope*, std::سيٽ<Ast_Sen_وڻ*>> externalDomains;
    { AwaitVisitor{netlistp, lbs, postUpdates, externalDomains}; }
    أرجع {std::move(lbs), postUpdates, std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

// Transform all forked processes into functions
صنف TransformForksVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current fork.
    ثابت VNUser1_اندريون_Use m_inuser1;

    // STATE
    منطقي m_اندريون_Class = خطأ;  // Are we in a class?
    منطقي m_beginHasAwaits = خطأ;  // Does the current begin have awaits?
    AstFork* m_forkp = خالي_اشارو;  // Current fork
    AstCFunc* m_funcp = خالي_اشارو;  // Current function

    // METHODS
    // Remap local vars referenced by the given fork function
    // TODO: We should only pass variables to the fork that are
    // live in the fork body, but for that we need a proper data
    // flow analysis framework which we don't have at the moment
    فراغ remapLocals(AstCFunc* ثابت funcp, AstCCall* ثابت callp) {
        ثابت VNUser2_اندريون_Use user2_اندريون_Use;  // AstVarScope -> AstVarScope: var to remap to
        funcp->foreach([&](Ast_ڳنڍ_Var_Ref* refp) {
            AstVar* ثابت varp = refp->varp();
            AstVarScope* ثابت vscp = refp->varScopep();
            AstBasicDType* ثابت dtypep = varp->dtypep()->basicp();
            // If not a fork..join, copy. All write refs should've been handled by V3Fork
            منطقي pass_By_قدر = !m_forkp->joinType().join();
            إذا (vscp->fork_By_قدر()) {
                // Set just before the fork, for its processes to get by value
                pass_By_قدر = سچ;
            } وإلا إذا (!varp->isFuncLocal()) {
                // Not func local. Its lifetime is longer than the forked process. Skip
                أرجع;
            } وإلا إذا (!varp->user1()) {
                // Not declared before the fork. It cannot outlive the forked process
                أرجع;
            } وإلا إذا (dtypep && dtypep->isForkSync()) {
                // We can just pass it by value to the new function
                pass_By_قدر = سچ;
            }
            // Remap the reference
            إذا (!vscp->user2p()) {
                // Clone the var to the new function
                AstVar* ثابت newvarp
                    = جديد AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                newvarp->funcLocal(سچ);
                newvarp->direction(pass_By_قدر ? VDirection::داخلا : VDirection::REF);
                funcp->جوڙيو_Argsp(newvarp);
                AstVarScope* ثابت newvscp
                    = جديد AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->جوڙيو_Varsp(newvscp);
                vscp->user2p(newvscp);
                callp->جوڙيو_Argsp(جديد AstVarRef{refp->fileline(), vscp,
                                              pass_By_قدر ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* ثابت newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        VL_RESTORER(m_اندريون_Class);
        m_اندريون_Class = VN_IS(nodep, Class);
        iterateChildren(nodep);
    }
    فراغ گهمو(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        m_funcp = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstVar* nodep) override {
        إذا (!m_forkp) nodep->user1(سچ);
    }
    فراغ گهمو(AstFork* nodep) override {
        إذا (m_forkp) أرجع;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        Ast_ڳنڍ* resp = خالي_اشارو;
        إذا (Ast_ڳنڍ* ثابت declsp = nodep->declsp()) {
            resp = Ast_ڳنڍ::جوڙيو_ايندڙ(resp, declsp->unlink_Fr_پٺ_With_ايندڙ());
        }
        إذا (Ast_ڳنڍ* ثابت stmtsp = nodep->stmtsp()) {
            resp = Ast_ڳنڍ::جوڙيو_ايندڙ(resp, stmtsp->unlink_Fr_پٺ_With_ايندڙ());
        }
        طالما (AstBegin* ثابت beginp = nodep->forksp()) {
            إذا (Ast_ڳنڍ* ثابت declsp = beginp->declsp()) {
                resp = Ast_ڳنڍ::جوڙيو_ايندڙ(resp, declsp->unlink_Fr_پٺ_With_ايندڙ());
            }
            إذا (Ast_ڳنڍ* ثابت stmtsp = beginp->stmtsp()) {
                resp = Ast_ڳنڍ::جوڙيو_ايندڙ(resp, stmtsp->unlink_Fr_پٺ_With_ايندڙ());
            }
            VL_DO_DANGLING(ڌڪو_Deletep(beginp->unlink_Fr_پٺ()), beginp);
        }
        إذا (resp) {
            nodep->replaceWith(resp);
        } وإلا {
            nodep->unlink_Fr_پٺ();
        }
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(AstBegin* nodep) override {
        UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
        // Start with children, so later we only find awaits that are actually in this begin
        m_beginHasAwaits = خطأ;
        iterateChildrenConst(nodep);
        إذا (!nodep->stmtsp()) أرجع;
        إذا (!m_beginHasAwaits && !nodep->needProcess()) أرجع;

        UASSERT_OBJ(!nodep->name().خالي_جايون(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        File_سٽ* ثابت flp = nodep->fileline();
        AstCFunc* ثابت newfuncp = جديد AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->جوڙيو_ايندڙ_هتي(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->decl_ذاتي(سچ);
        // Create the call to the function
        AstCCall* ثابت callp = جديد AstCCall{flp, newfuncp};
        callp->dtype_سيٽ_Void();
        // If we're in a class, add a vlSymsp arg
        إذا (m_اندريون_Class) {
            newfuncp->جوڙيو_Stmtsp(جديد AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        إذا (Ast_ڳنڍ* ثابت declsp = nodep->declsp()) {
            newfuncp->جوڙيو_Stmtsp(declsp->unlink_Fr_پٺ_With_ايندڙ());
        }
        إذا (Ast_ڳنڍ* ثابت stmtsp = nodep->stmtsp()) {
            newfuncp->جوڙيو_Stmtsp(stmtsp->unlink_Fr_پٺ_With_ايندڙ());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->جوڙيو_Stmtsp(callp->makeStmt());
        // Propagate if needs process
        إذا (nodep->needProcess()) newfuncp->سيٽ_Need_Process();
        remapLocals(newfuncp, callp);
    }
    فراغ گهمو(AstCAwait* nodep) override {
        m_beginHasAwaits = سچ;
        iterateChildrenConst(nodep);
    }

    //--------------------
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = افتراضي;
};

فراغ transformForks(AstNetlist* ثابت netlistp) {
    إذا (!v3Global.استعمال_ڪري_Timing()) أرجع;
    { TransformForksVisitor{netlistp}; }
    V3Global::dump_Check_Global_وڻ("transform_forks", 0, dump_وڻ_Either_Level() >= 3);
}

}  // namespace V3Sched
