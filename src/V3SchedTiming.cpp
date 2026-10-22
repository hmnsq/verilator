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

std::map<ثابت AstVarScope*, std::وکتور<uint32_t>>
TimingKit::externalTriggerIndices(ثابت TriggerKit& trigKit) ثابت {
    std::map<ثابت AstVarScope*, std::وکتور<uint32_t>> نتیجه;
    لكل (ثابت خودکار& vscpDomains : m_externalDomains) {
        std::وکتور<uint32_t>& indices = نتیجه[vscpDomains.اولی];
        لكل (ثابت Ast_Sen_درخت* ثابت domainp : vscpDomains.دومی) {
            لكل (ثابت Ast_Sen_قلم* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_قلم)) {
                indices.افزودن_پشت(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    أرجع نتیجه;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    إذا (!m_resumeFuncp) {
        إذا (m_lbs.خالی()) أرجع اشاره_تهی;
        // Create global resume function
        AstNetlist* ثابت netlistp = v3Global.rootp();
        AstScope* ثابت scopeTopp = netlistp->بالاترین_Scopep()->scopep();
        m_resumeFuncp = جديد AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(درست);
        m_resumeFuncp->isLoose(درست);
        m_resumeFuncp->isConst(خطأ);
        m_resumeFuncp->decl_خصوصی(درست);
        scopeTopp->جمع_کن_Blocksp(m_resumeFuncp);

        لكل (ثابت خودکار& مدخل : m_lbs) {
            Ast_فعال* ثابت activep = مدخل.m_activep;
            activep->foreach([هذا](AstCMethodHard* ثابت exprp) {
                إذا (exprp->روش() != VCMethod::SCHED_RESUME) أرجع;
                Ast_گره_عبارت* ثابت fromp = exprp->fromp();
                إذا (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    أرجع;
                }
                AstCMethodHard* ثابت move_مقصد_Resumep = جديد AstCMethodHard{
                    fromp->fileline(), fromp->clone_درخت(خطأ),
                    VCMethod::SCHED_MOVE_مقصد_RESUME_صف,
                    exprp->pinsp() ? exprp->pinsp()->clone_درخت(درست) : اشاره_تهی};
                move_مقصد_Resumep->dtype_مجموعه_Void();
                m_resumeFuncp->جمع_کن_Stmtsp(move_مقصد_Resumep->makeStmt());
            });
        }

        // Put all the timing actives in the resume function
        AstIf* dlyShedIfp = اشاره_تهی;
        لكل (ثابت خودکار& مدخل : m_lbs) {
            Ast_فعال* ثابت activep = مدخل.m_activep;
            // Resume time delays last. For no particular reason other than
            // that's what we used to do prior to proper #0 support.
            AstVarRef* ثابت schedrefp = VN_AS(
                VN_AS(VN_AS(activep->stmtsp(), Stmt_عبارت)->exprp(), CMethodHard)->fromp(), VarRef);

            Ast_گره* ثابت actionp = activep->stmtsp()->unlink_Fr_پشت_With_بعدی();
            إذا (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                File_سطر* ثابت flp = trigVscp->fileline();
                ثابت std::مجموعه<uint32_t>& triggers = مدخل.m_triggers;
                AstTriggered* ثابت condp
                    = جديد AstTriggered{flp, trigVscp, {triggers.begin(), triggers.انتها()}};
                dlyShedIfp = جديد AstIf{flp, condp};
                dlyShedIfp->جمع_کن_Thensp(actionp);
            } وإلا {
                m_resumeFuncp->جمع_کن_Stmtsp(actionp);
            }
        }
        إذا (dlyShedIfp) m_resumeFuncp->جمع_کن_Stmtsp(dlyShedIfp);

        // These are now spent, oispose of now empty AstActive instances
        m_lbs.deleteActives();
    }
    AstCCall* ثابت callp = جديد AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
    callp->dtype_مجموعه_Void();
    أرجع callp;
}

AstVarScope* TimingKit::getDelayScheduler(AstNetlist* ثابت /*netlistp*/) {
    لكل (ثابت خودکار& مدخل : m_lbs) {
        Ast_فعال* ثابت ap = مدخل.m_activep;
        // TODO: this triple VN_AS expression is ridiculous
        AstVarRef* ثابت schedrefp
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), Stmt_عبارت)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* ثابت vscp = schedrefp->varScopep();
        إذا (vscp->dtypep()->basicp()->isDelayScheduler()) أرجع vscp;
    }
    // None found. Design doesn't use any time delays
    أرجع اشاره_تهی;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    إذا (!m_readyFuncp) {
        لكل (ثابت خودکار& مدخل : m_lbs) {
            Ast_فعال* ثابت activep = مدخل.m_activep;
            خودکار* ثابت resumep = VN_AS(VN_AS(activep->stmtsp(), Stmt_عبارت)->exprp(), CMethodHard);
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
                AstScope* ثابت scopeTopp = netlistp->بالاترین_Scopep()->scopep();
                m_readyFuncp = جديد AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(درست);
                m_readyFuncp->isLoose(درست);
                m_readyFuncp->isConst(خطأ);
                m_readyFuncp->decl_خصوصی(درست);
                scopeTopp->جمع_کن_Blocksp(m_readyFuncp);
            }

            File_سطر* ثابت flp = activep->sentreep()->fileline();

            // Create an 'AstIf' sensitive to the suspending triggers
            ثابت std::مجموعه<uint32_t>& triggers = مدخل.m_triggers;
            AstTriggered* ثابت condp
                = جديد AstTriggered{flp, trigVscp, {triggers.begin(), triggers.انتها()}};
            AstIf* ثابت ifp = جديد AstIf{flp, condp};
            m_readyFuncp->جمع_کن_Stmtsp(ifp);

            // Mark as ready the processes resumed on this sensitivity expression
            AstVarRef* ثابت refp = جديد AstVarRef{flp, schedulerp, VAccess::READWRITE};
            AstCMethodHard* ثابت callp = جديد AstCMethodHard{flp, refp, VCMethod::SCHED_READY};
            callp->dtype_مجموعه_Void();
            إذا (resumep->pinsp()) callp->جمع_کن_Pinsp(resumep->pinsp()->clone_درخت(خطأ));
            ifp->جمع_کن_Thensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        إذا (!m_readyFuncp) أرجع اشاره_تهی;
    }
    AstCCall* ثابت callp = جديد AstCCall{m_readyFuncp->fileline(), m_readyFuncp};
    callp->dtype_مجموعه_Void();
    أرجع callp;
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

صنف AwaitVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstSenTree::user1()  -> bool.               Set true if the sentree has been visited.
    //  AstCFunc::user1()    -> AstUser1Allocator.  See alocator below
    ثابت VNUser1_درون_Use m_inuser1;

    // STATE
    منطقي m_درون_Process = خطأ;  // Are we in a process?
    منطقي m_gatherVars = خطأ;  // Should we gather vars in m_writtenBySuspendable?
    AstScope* ثابت m_scopeTopp;  // Scope at the top
    LogicByScope& m_lbs;  // Timing resume actives
    Ast_گره_Stmt*& m_postUpdatesr;  // Post updates for the trigger eval function
    // Additional var sensitivities
    std::map<ثابت AstVarScope*, std::مجموعه<Ast_Sen_درخت*>>& m_externalDomains;
    std::unique_ptr<V3_Class_گراف>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::مجموعه<Ast_Sen_درخت*> m_processDomains;  // Sentrees from the current process
    // Variables written by suspendable processes
    std::مجموعه<AstVarScope*> m_writtenBySuspendable;
    بنية CFuncCache final {
        std::مجموعه<Ast_Sen_درخت*> m_processDomains;  // What shall be added to m_processDomains
        std::مجموعه<AstVarScope*>
            m_writtenBySuspendable;  // What shall be added to m_writtenBySuspendable
        std::مجموعه<AstCFunc*> m_includes;  // CFuncs whose CFuncCache shall be included into this
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
    std::وکتور<ثابت AstCFunc*> m_call_پشته;  // Current callstack of AstCFuncs
    // Caches how visiting the function with given value of m_gatherVars changes
    // m_processDomains and m_writtenBySuspendable - only accessed from visit(AstCFunc* nodep)
    AstUser1Allocator<AstCFunc, std::array<CFuncCache, 2>> m_cfuncsCache;
    // Count uses of not inlined writes to signals in suspendables
    VDouble0 m_not_Inlined_Writes_درون_Suspendable_Usage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    فراغ جمع_کن_Resume_Pins(AstCMethodHard* ثابت resumep, Ast_گره_عبارت* pinsp) {
        AstCExpr* ثابت exprp = VN_CAST(pinsp, CExpr);
        Ast_متن* ثابت textp = VN_CAST(exprp->nodesp(), متن_2);
        إذا (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            إذا ((pinsp = VN_CAST(pinsp->nextp(), گره_عبارت))) {
                resumep->جمع_کن_Pinsp(pinsp->clone_درخت(خطأ));
            }
        } وإلا {
            resumep->جمع_کن_Pinsp(pinsp->clone_درخت(خطأ));
        }
    }
    فراغ جمع_کن_CFunc_Cached_مقادیر(ثابت AstCFunc* ثابت cfuncp,
                              std::unordered_مجموعه<ثابت AstCFunc*>& دیدهشده) {
        إذا (!دیدهشده.درج(cfuncp).دومی) أرجع;
        CFuncCache& مقدار = m_cfuncsCache(cfuncp)[static_cast<اندازه_t>(m_gatherVars)];
        ثابت اندازه_t initial_اندازه = m_writtenBySuspendable.اندازه();
        m_writtenBySuspendable.درج(مقدار.m_writtenBySuspendable.begin(),
                                      مقدار.m_writtenBySuspendable.انتها());
        m_not_Inlined_Writes_درون_Suspendable_Usage += m_writtenBySuspendable.اندازه() - initial_اندازه;
        m_processDomains.درج(مقدار.m_processDomains.begin(), مقدار.m_processDomains.انتها());
        لكل (ثابت AstCFunc* ثابت includedp : مقدار.m_includes) {
            جمع_کن_CFunc_Cached_مقادیر(includedp, دیدهشده);
        }
    }
    // Create an active with a timing scheduler resume() call
    فراغ create_Resume_فعال(AstCAwait* ثابت awaitp) {
        خودکار* ثابت methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* ثابت schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        Ast_Sen_درخت* ثابت sentreep = awaitp->sentreep();
        File_سطر* ثابت flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        خودکار* ثابت resumep = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtype_مجموعه_Void();
        إذا (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            إذا (Ast_گره* ثابت dbginfop = methodp->pinsp()->nextp()) {
                إذا (methodp->pinsp()) جمع_کن_Resume_Pins(resumep, static_cast<Ast_گره_عبارت*>(dbginfop));
            }
        } وإلا إذا (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            خودکار* ثابت postp = resumep->clone_درخت(خطأ);
            postp->روش(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = Ast_گره::جمع_کن_بعدی(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        Ast_فعال* ثابت activep = جديد Ast_فعال{flp, "_timing", sentreep};
        activep->جمع_کن_Stmtsp(resumep->makeStmt());
        m_lbs.جمع_کن(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    فراغ بازدید_Called_CFunc(AstCFunc* ثابت nodep) {
        إذا (!m_درون_Process) أرجع;  // Skip if not in process - nothing would be added to any set
        ثابت اندازه_t cache_کلید = static_cast<اندازه_t>(m_gatherVars);
        CFuncCache& مقدار = m_cfuncsCache(nodep)[cache_کلید];
        بدل (مقدار.m_state) {
        حالة CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            مقدار.m_state = CFuncCache::VISITING;
            m_call_پشته.افزودن_پشت(nodep);
            iterateChildren(nodep);
            m_call_پشته.برداشت_پشت();
            مقدار.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::جابجا(m_processDomains, مقدار.m_processDomains);
            std::جابجا(m_writtenBySuspendable, مقدار.m_writtenBySuspendable);
        } اكسر;
        حالة CFuncCache::VISITING: {
            لكل (اندازه_t i = m_call_پشته.اندازه() - 1; m_call_پشته.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_پشته[i])[cache_کلید].m_includes.درج(nodep);
            }
            أرجع;  // Break recursion
        }
        حالة CFuncCache::INITIALIZED: اكسر;
        }
        std::unordered_مجموعه<ثابت AstCFunc*> دیدهشده;
        جمع_کن_CFunc_Cached_مقادیر(nodep, دیدهشده);
    }

    // VISITORS
    فراغ بازدید(Ast_گره_Procedure* ثابت nodep) override {
        UASSERT_OBJ(!m_درون_Process && !m_gatherVars && m_processDomains.خالی()
                        && m_writtenBySuspendable.خالی(),
                    nodep, "Process in process?");
        m_درون_Process = درست;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        لكل (AstVarScope* ثابت vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].درج(m_processDomains.begin(), m_processDomains.انتها());
            vscp->varp()->مجموعه_Written_By_Suspendable();
        }
        m_processDomains.clear();
        m_writtenBySuspendable.clear();
        m_درون_Process = خطأ;
        m_gatherVars = خطأ;
    }
    فراغ بازدید(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        إذا (m_درون_Process) m_gatherVars = درست;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    فراغ بازدید(AstCAwait* nodep) override {
        إذا (Ast_Sen_درخت* ثابت sentreep = nodep->sentreep()) {
            إذا (!sentreep->user1_مجموعه_Once()) create_Resume_فعال(nodep);
            إذا (m_درون_Process) m_processDomains.درج(sentreep);
        }
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_گره_Var_Ref* nodep) override {
        إذا (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.درج(nodep->varScopep());
        }
    }
    فراغ بازدید(Ast_گره_CCall* ثابت nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ثابت خودکار& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        إذا (cfuncps.خالی()) {
            بازدید_Called_CFunc(nodep->funcp());
        } وإلا {
            لكل (AstCFunc* ثابت cfuncp : cfuncps) بازدید_Called_CFunc(cfuncp);
        }
    }
    فراغ بازدید(AstCFunc* ثابت nodep) override {
        ثابت خودکار& مقدار = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        إذا (مقدار[0].m_state != CFuncCache::UNINITIALIZED
            || مقدار[1].m_state != CFuncCache::UNINITIALIZED) {
            أرجع;
        }
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_عبارت_Stmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, Ast_گره_Stmt*& postUpdatesr,
                          std::map<ثابت AstVarScope*, std::مجموعه<Ast_Sen_درخت*>>& externalDomains)
        : m_scopeTopp{nodep->بالاترین_Scopep()->scopep()}
        , m_lbs{lbs}
        , m_postUpdatesr{postUpdatesr}
        , m_externalDomains{externalDomains}
        , m_classGraphp{V3_Class_گراف::build(nodep)} {
        iterate(nodep);
    }
    ~AwaitVisitor() override {
        V3Stats::جمع_کن_Stat("Scheduling, count of non-inlined signal writes in suspendables",
                         m_not_Inlined_Writes_درون_Suspendable_Usage);
    }
};

TimingKit prepareTiming(AstNetlist* ثابت netlistp) {
    إذا (!v3Global.استفاده_میکند_Timing()) أرجع {};
    LogicByScope lbs;
    Ast_گره_Stmt* postUpdates = اشاره_تهی;
    std::map<ثابت AstVarScope*, std::مجموعه<Ast_Sen_درخت*>> externalDomains;
    { AwaitVisitor{netlistp, lbs, postUpdates, externalDomains}; }
    أرجع {std::move(lbs), postUpdates, std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

// Transform all forked processes into functions
صنف TransformForksVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current fork.
    ثابت VNUser1_درون_Use m_inuser1;

    // STATE
    منطقي m_درون_Class = خطأ;  // Are we in a class?
    منطقي m_beginHasAwaits = خطأ;  // Does the current begin have awaits?
    AstFork* m_forkp = اشاره_تهی;  // Current fork
    AstCFunc* m_funcp = اشاره_تهی;  // Current function

    // METHODS
    // Remap local vars referenced by the given fork function
    // TODO: We should only pass variables to the fork that are
    // live in the fork body, but for that we need a proper data
    // flow analysis framework which we don't have at the moment
    فراغ remapLocals(AstCFunc* ثابت funcp, AstCCall* ثابت callp) {
        ثابت VNUser2_درون_Use user2_درون_Use;  // AstVarScope -> AstVarScope: var to remap to
        funcp->foreach([&](Ast_گره_Var_Ref* refp) {
            AstVar* ثابت varp = refp->varp();
            AstVarScope* ثابت vscp = refp->varScopep();
            AstBasicDType* ثابت dtypep = varp->dtypep()->basicp();
            // If not a fork..join, copy. All write refs should've been handled by V3Fork
            منطقي pass_By_مقدار = !m_forkp->joinType().join();
            إذا (vscp->fork_By_مقدار()) {
                // Set just before the fork, for its processes to get by value
                pass_By_مقدار = درست;
            } وإلا إذا (!varp->isFuncLocal()) {
                // Not func local. Its lifetime is longer than the forked process. Skip
                أرجع;
            } وإلا إذا (!varp->user1()) {
                // Not declared before the fork. It cannot outlive the forked process
                أرجع;
            } وإلا إذا (dtypep && dtypep->isForkSync()) {
                // We can just pass it by value to the new function
                pass_By_مقدار = درست;
            }
            // Remap the reference
            إذا (!vscp->user2p()) {
                // Clone the var to the new function
                AstVar* ثابت newvarp
                    = جديد AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                newvarp->funcLocal(درست);
                newvarp->direction(pass_By_مقدار ? VDirection::درون_داد : VDirection::REF);
                funcp->جمع_کن_Argsp(newvarp);
                AstVarScope* ثابت newvscp
                    = جديد AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->جمع_کن_Varsp(newvscp);
                vscp->user2p(newvscp);
                callp->جمع_کن_Argsp(جديد AstVarRef{refp->fileline(), vscp,
                                              pass_By_مقدار ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* ثابت newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    فراغ بازدید(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_درون_Class);
        m_درون_Class = VN_IS(nodep, Class);
        iterateChildren(nodep);
    }
    فراغ بازدید(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        m_funcp = nodep;
        iterateChildren(nodep);
    }
    فراغ بازدید(AstVar* nodep) override {
        إذا (!m_forkp) nodep->user1(درست);
    }
    فراغ بازدید(AstFork* nodep) override {
        إذا (m_forkp) أرجع;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        Ast_گره* resp = اشاره_تهی;
        إذا (Ast_گره* ثابت declsp = nodep->declsp()) {
            resp = Ast_گره::جمع_کن_بعدی(resp, declsp->unlink_Fr_پشت_With_بعدی());
        }
        إذا (Ast_گره* ثابت stmtsp = nodep->stmtsp()) {
            resp = Ast_گره::جمع_کن_بعدی(resp, stmtsp->unlink_Fr_پشت_With_بعدی());
        }
        طالما (AstBegin* ثابت beginp = nodep->forksp()) {
            إذا (Ast_گره* ثابت declsp = beginp->declsp()) {
                resp = Ast_گره::جمع_کن_بعدی(resp, declsp->unlink_Fr_پشت_With_بعدی());
            }
            إذا (Ast_گره* ثابت stmtsp = beginp->stmtsp()) {
                resp = Ast_گره::جمع_کن_بعدی(resp, stmtsp->unlink_Fr_پشت_With_بعدی());
            }
            VL_DO_DANGLING(افزودن_Deletep(beginp->unlink_Fr_پشت()), beginp);
        }
        إذا (resp) {
            nodep->replaceWith(resp);
        } وإلا {
            nodep->unlink_Fr_پشت();
        }
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    فراغ بازدید(AstBegin* nodep) override {
        UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
        // Start with children, so later we only find awaits that are actually in this begin
        m_beginHasAwaits = خطأ;
        iterateChildrenConst(nodep);
        إذا (!nodep->stmtsp()) أرجع;
        إذا (!m_beginHasAwaits && !nodep->needProcess()) أرجع;

        UASSERT_OBJ(!nodep->name().خالی(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        File_سطر* ثابت flp = nodep->fileline();
        AstCFunc* ثابت newfuncp = جديد AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->جمع_کن_بعدی_اینجا(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->decl_خصوصی(درست);
        // Create the call to the function
        AstCCall* ثابت callp = جديد AstCCall{flp, newfuncp};
        callp->dtype_مجموعه_Void();
        // If we're in a class, add a vlSymsp arg
        إذا (m_درون_Class) {
            newfuncp->جمع_کن_Stmtsp(جديد AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        إذا (Ast_گره* ثابت declsp = nodep->declsp()) {
            newfuncp->جمع_کن_Stmtsp(declsp->unlink_Fr_پشت_With_بعدی());
        }
        إذا (Ast_گره* ثابت stmtsp = nodep->stmtsp()) {
            newfuncp->جمع_کن_Stmtsp(stmtsp->unlink_Fr_پشت_With_بعدی());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->جمع_کن_Stmtsp(callp->makeStmt());
        // Propagate if needs process
        إذا (nodep->needProcess()) newfuncp->مجموعه_Need_Process();
        remapLocals(newfuncp, callp);
    }
    فراغ بازدید(AstCAwait* nodep) override {
        m_beginHasAwaits = درست;
        iterateChildrenConst(nodep);
    }

    //--------------------
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = افتراضي;
};

فراغ transformForks(AstNetlist* ثابت netlistp) {
    إذا (!v3Global.استفاده_میکند_Timing()) أرجع;
    { TransformForksVisitor{netlistp}; }
    V3Global::dump_Check_Global_درخت("transform_forks", 0, dump_درخت_Either_Level() >= 3);
}

}  // namespace V3Sched
