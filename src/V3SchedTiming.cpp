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

std::map<ثابت AstVarScope*, std::سمتیہ<uint32_t>>
TimingKit::externalTriggerIndices(ثابت TriggerKit& trigKit) ثابت {
    std::map<ثابت AstVarScope*, std::سمتیہ<uint32_t>> نتیجہ;
    لكل (ثابت خودکار& vscpDomains : m_externalDomains) {
        std::سمتیہ<uint32_t>& indices = نتیجہ[vscpDomains.پہلا];
        لكل (ثابت Ast_Sen_درخت* ثابت domainp : vscpDomains.دوجا) {
            لكل (ثابت Ast_Sen_شے* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_شے)) {
                indices.ڈالو_پٹھ(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    أرجع نتیجہ;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    إذا (!m_resumeFuncp) {
        إذا (m_lbs.خالی_تھاواں()) أرجع خالی_سوچک;
        // Create global resume function
        AstNetlist* ثابت netlistp = v3Global.rootp();
        AstScope* ثابت scopeTopp = netlistp->چوٹی_Scopep()->scopep();
        m_resumeFuncp = جديد AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(سچ);
        m_resumeFuncp->isLoose(سچ);
        m_resumeFuncp->isConst(خطأ);
        m_resumeFuncp->decl_نجی(سچ);
        scopeTopp->جوڑو_Blocksp(m_resumeFuncp);

        لكل (ثابت خودکار& اندراج : m_lbs) {
            Ast_سرگرم* ثابت activep = اندراج.m_activep;
            activep->foreach([هذا](AstCMethodHard* ثابت exprp) {
                إذا (exprp->طریقہ() != VCMethod::SCHED_RESUME) أرجع;
                Ast_گرہ_اظہاریہ* ثابت fromp = exprp->fromp();
                إذا (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    أرجع;
                }
                AstCMethodHard* ثابت move_منزل_Resumep = جديد AstCMethodHard{
                    fromp->fileline(), fromp->clone_درخت(خطأ),
                    VCMethod::SCHED_MOVE_منزل_RESUME_قطار,
                    exprp->pinsp() ? exprp->pinsp()->clone_درخت(سچ) : خالی_سوچک};
                move_منزل_Resumep->dtype_سیٹ_Void();
                m_resumeFuncp->جوڑو_Stmtsp(move_منزل_Resumep->makeStmt());
            });
        }

        // Put all the timing actives in the resume function
        AstIf* dlyShedIfp = خالی_سوچک;
        لكل (ثابت خودکار& اندراج : m_lbs) {
            Ast_سرگرم* ثابت activep = اندراج.m_activep;
            // Resume time delays last. For no particular reason other than
            // that's what we used to do prior to proper #0 support.
            AstVarRef* ثابت schedrefp = VN_AS(
                VN_AS(VN_AS(activep->stmtsp(), Stmt_اظہاریہ)->exprp(), CMethodHard)->fromp(), VarRef);

            Ast_گرہ* ثابت actionp = activep->stmtsp()->unlink_Fr_پٹھ_With_اگلا();
            إذا (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                File_سطر* ثابت flp = trigVscp->fileline();
                ثابت std::سیٹ<uint32_t>& triggers = اندراج.m_triggers;
                AstTriggered* ثابت condp
                    = جديد AstTriggered{flp, trigVscp, {triggers.begin(), triggers.سرا()}};
                dlyShedIfp = جديد AstIf{flp, condp};
                dlyShedIfp->جوڑو_Thensp(actionp);
            } وإلا {
                m_resumeFuncp->جوڑو_Stmtsp(actionp);
            }
        }
        إذا (dlyShedIfp) m_resumeFuncp->جوڑو_Stmtsp(dlyShedIfp);

        // These are now spent, oispose of now empty AstActive instances
        m_lbs.deleteActives();
    }
    AstCCall* ثابت callp = جديد AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
    callp->dtype_سیٹ_Void();
    أرجع callp;
}

AstVarScope* TimingKit::getDelayScheduler(AstNetlist* ثابت /*netlistp*/) {
    لكل (ثابت خودکار& اندراج : m_lbs) {
        Ast_سرگرم* ثابت ap = اندراج.m_activep;
        // TODO: this triple VN_AS expression is ridiculous
        AstVarRef* ثابت schedrefp
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), Stmt_اظہاریہ)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* ثابت vscp = schedrefp->varScopep();
        إذا (vscp->dtypep()->basicp()->isDelayScheduler()) أرجع vscp;
    }
    // None found. Design doesn't use any time delays
    أرجع خالی_سوچک;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    إذا (!m_readyFuncp) {
        لكل (ثابت خودکار& اندراج : m_lbs) {
            Ast_سرگرم* ثابت activep = اندراج.m_activep;
            خودکار* ثابت resumep = VN_AS(VN_AS(activep->stmtsp(), Stmt_اظہاریہ)->exprp(), CMethodHard);
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
                AstScope* ثابت scopeTopp = netlistp->چوٹی_Scopep()->scopep();
                m_readyFuncp = جديد AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(سچ);
                m_readyFuncp->isLoose(سچ);
                m_readyFuncp->isConst(خطأ);
                m_readyFuncp->decl_نجی(سچ);
                scopeTopp->جوڑو_Blocksp(m_readyFuncp);
            }

            File_سطر* ثابت flp = activep->sentreep()->fileline();

            // Create an 'AstIf' sensitive to the suspending triggers
            ثابت std::سیٹ<uint32_t>& triggers = اندراج.m_triggers;
            AstTriggered* ثابت condp
                = جديد AstTriggered{flp, trigVscp, {triggers.begin(), triggers.سرا()}};
            AstIf* ثابت ifp = جديد AstIf{flp, condp};
            m_readyFuncp->جوڑو_Stmtsp(ifp);

            // Mark as ready the processes resumed on this sensitivity expression
            AstVarRef* ثابت refp = جديد AstVarRef{flp, schedulerp, VAccess::READWRITE};
            AstCMethodHard* ثابت callp = جديد AstCMethodHard{flp, refp, VCMethod::SCHED_READY};
            callp->dtype_سیٹ_Void();
            إذا (resumep->pinsp()) callp->جوڑو_Pinsp(resumep->pinsp()->clone_درخت(خطأ));
            ifp->جوڑو_Thensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        إذا (!m_readyFuncp) أرجع خالی_سوچک;
    }
    AstCCall* ثابت callp = جديد AstCCall{m_readyFuncp->fileline(), m_readyFuncp};
    callp->dtype_سیٹ_Void();
    أرجع callp;
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

صنف AwaitVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstSenTree::user1()  -> bool.               Set true if the sentree has been visited.
    //  AstCFunc::user1()    -> AstUser1Allocator.  See alocator below
    ثابت VNUser1_وچ_Use m_inuser1;

    // STATE
    منطقي m_وچ_Process = خطأ;  // Are we in a process?
    منطقي m_gatherVars = خطأ;  // Should we gather vars in m_writtenBySuspendable?
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
    بنية CFuncCache final {
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
        تعداد State : uint8_t {
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
    VDouble0 m_not_Inlined_Writes_وچ_Suspendable_Usage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    فراغ جوڑو_Resume_Pins(AstCMethodHard* ثابت resumep, Ast_گرہ_اظہاریہ* pinsp) {
        AstCExpr* ثابت exprp = VN_CAST(pinsp, CExpr);
        Ast_متن* ثابت textp = VN_CAST(exprp->nodesp(), متن_2);
        إذا (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            إذا ((pinsp = VN_CAST(pinsp->nextp(), گرہ_اظہاریہ))) {
                resumep->جوڑو_Pinsp(pinsp->clone_درخت(خطأ));
            }
        } وإلا {
            resumep->جوڑو_Pinsp(pinsp->clone_درخت(خطأ));
        }
    }
    فراغ جوڑو_CFunc_Cached_اقدار(ثابت AstCFunc* ثابت cfuncp,
                              std::unordered_سیٹ<ثابت AstCFunc*>& ویکھیا) {
        إذا (!ویکھیا.داخل_کرو(cfuncp).دوجا) أرجع;
        CFuncCache& قدر = m_cfuncsCache(cfuncp)[static_cast<ناپ_t>(m_gatherVars)];
        ثابت ناپ_t initial_ناپ = m_writtenBySuspendable.ناپ();
        m_writtenBySuspendable.داخل_کرو(قدر.m_writtenBySuspendable.begin(),
                                      قدر.m_writtenBySuspendable.سرا());
        m_not_Inlined_Writes_وچ_Suspendable_Usage += m_writtenBySuspendable.ناپ() - initial_ناپ;
        m_processDomains.داخل_کرو(قدر.m_processDomains.begin(), قدر.m_processDomains.سرا());
        لكل (ثابت AstCFunc* ثابت includedp : قدر.m_includes) {
            جوڑو_CFunc_Cached_اقدار(includedp, ویکھیا);
        }
    }
    // Create an active with a timing scheduler resume() call
    فراغ create_Resume_سرگرم(AstCAwait* ثابت awaitp) {
        خودکار* ثابت methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* ثابت schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        Ast_Sen_درخت* ثابت sentreep = awaitp->sentreep();
        File_سطر* ثابت flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        خودکار* ثابت resumep = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtype_سیٹ_Void();
        إذا (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            إذا (Ast_گرہ* ثابت dbginfop = methodp->pinsp()->nextp()) {
                إذا (methodp->pinsp()) جوڑو_Resume_Pins(resumep, static_cast<Ast_گرہ_اظہاریہ*>(dbginfop));
            }
        } وإلا إذا (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            خودکار* ثابت postp = resumep->clone_درخت(خطأ);
            postp->طریقہ(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = Ast_گرہ::جوڑو_اگلا(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        Ast_سرگرم* ثابت activep = جديد Ast_سرگرم{flp, "_timing", sentreep};
        activep->جوڑو_Stmtsp(resumep->makeStmt());
        m_lbs.جوڑو(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    فراغ ویکھو_Called_CFunc(AstCFunc* ثابت nodep) {
        إذا (!m_وچ_Process) أرجع;  // Skip if not in process - nothing would be added to any set
        ثابت ناپ_t cache_کلید = static_cast<ناپ_t>(m_gatherVars);
        CFuncCache& قدر = m_cfuncsCache(nodep)[cache_کلید];
        بدل (قدر.m_state) {
        حالة CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            قدر.m_state = CFuncCache::VISITING;
            m_call_ڈھیر.ڈالو_پٹھ(nodep);
            iterateChildren(nodep);
            m_call_ڈھیر.نکالو_پٹھ();
            قدر.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::وٹاندرا(m_processDomains, قدر.m_processDomains);
            std::وٹاندرا(m_writtenBySuspendable, قدر.m_writtenBySuspendable);
        } اكسر;
        حالة CFuncCache::VISITING: {
            لكل (ناپ_t i = m_call_ڈھیر.ناپ() - 1; m_call_ڈھیر.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_ڈھیر[i])[cache_کلید].m_includes.داخل_کرو(nodep);
            }
            أرجع;  // Break recursion
        }
        حالة CFuncCache::INITIALIZED: اكسر;
        }
        std::unordered_سیٹ<ثابت AstCFunc*> ویکھیا;
        جوڑو_CFunc_Cached_اقدار(nodep, ویکھیا);
    }

    // VISITORS
    فراغ ویکھو(Ast_گرہ_Procedure* ثابت nodep) override {
        UASSERT_OBJ(!m_وچ_Process && !m_gatherVars && m_processDomains.خالی_تھاواں()
                        && m_writtenBySuspendable.خالی_تھاواں(),
                    nodep, "Process in process?");
        m_وچ_Process = سچ;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        لكل (AstVarScope* ثابت vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].داخل_کرو(m_processDomains.begin(), m_processDomains.سرا());
            vscp->varp()->سیٹ_Written_By_Suspendable();
        }
        m_processDomains.clear();
        m_writtenBySuspendable.clear();
        m_وچ_Process = خطأ;
        m_gatherVars = خطأ;
    }
    فراغ ویکھو(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        إذا (m_وچ_Process) m_gatherVars = سچ;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstCAwait* nodep) override {
        إذا (Ast_Sen_درخت* ثابت sentreep = nodep->sentreep()) {
            إذا (!sentreep->user1_سیٹ_Once()) create_Resume_سرگرم(nodep);
            إذا (m_وچ_Process) m_processDomains.داخل_کرو(sentreep);
        }
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_گرہ_Var_Ref* nodep) override {
        إذا (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.داخل_کرو(nodep->varScopep());
        }
    }
    فراغ ویکھو(Ast_گرہ_CCall* ثابت nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ثابت خودکار& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        إذا (cfuncps.خالی_تھاواں()) {
            ویکھو_Called_CFunc(nodep->funcp());
        } وإلا {
            لكل (AstCFunc* ثابت cfuncp : cfuncps) ویکھو_Called_CFunc(cfuncp);
        }
    }
    فراغ ویکھو(AstCFunc* ثابت nodep) override {
        ثابت خودکار& قدر = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        إذا (قدر[0].m_state != CFuncCache::UNINITIALIZED
            || قدر[1].m_state != CFuncCache::UNINITIALIZED) {
            أرجع;
        }
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_اظہاریہ_Stmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, Ast_گرہ_Stmt*& postUpdatesr,
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
                         m_not_Inlined_Writes_وچ_Suspendable_Usage);
    }
};

TimingKit prepareTiming(AstNetlist* ثابت netlistp) {
    إذا (!v3Global.ورتدا_Timing()) أرجع {};
    LogicByScope lbs;
    Ast_گرہ_Stmt* postUpdates = خالی_سوچک;
    std::map<ثابت AstVarScope*, std::سیٹ<Ast_Sen_درخت*>> externalDomains;
    { AwaitVisitor{netlistp, lbs, postUpdates, externalDomains}; }
    أرجع {std::move(lbs), postUpdates, std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

// Transform all forked processes into functions
صنف TransformForksVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current fork.
    ثابت VNUser1_وچ_Use m_inuser1;

    // STATE
    منطقي m_وچ_Class = خطأ;  // Are we in a class?
    منطقي m_beginHasAwaits = خطأ;  // Does the current begin have awaits?
    AstFork* m_forkp = خالی_سوچک;  // Current fork
    AstCFunc* m_funcp = خالی_سوچک;  // Current function

    // METHODS
    // Remap local vars referenced by the given fork function
    // TODO: We should only pass variables to the fork that are
    // live in the fork body, but for that we need a proper data
    // flow analysis framework which we don't have at the moment
    فراغ remapLocals(AstCFunc* ثابت funcp, AstCCall* ثابت callp) {
        ثابت VNUser2_وچ_Use user2_وچ_Use;  // AstVarScope -> AstVarScope: var to remap to
        funcp->foreach([&](Ast_گرہ_Var_Ref* refp) {
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
                newvarp->direction(pass_By_قدر ? VDirection::داخلہ : VDirection::REF);
                funcp->جوڑو_Argsp(newvarp);
                AstVarScope* ثابت newvscp
                    = جديد AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->جوڑو_Varsp(newvscp);
                vscp->user2p(newvscp);
                callp->جوڑو_Argsp(جديد AstVarRef{refp->fileline(), vscp,
                                              pass_By_قدر ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* ثابت newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_وچ_Class);
        m_وچ_Class = VN_IS(nodep, Class);
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        m_funcp = nodep;
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstVar* nodep) override {
        إذا (!m_forkp) nodep->user1(سچ);
    }
    فراغ ویکھو(AstFork* nodep) override {
        إذا (m_forkp) أرجع;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        Ast_گرہ* resp = خالی_سوچک;
        إذا (Ast_گرہ* ثابت declsp = nodep->declsp()) {
            resp = Ast_گرہ::جوڑو_اگلا(resp, declsp->unlink_Fr_پٹھ_With_اگلا());
        }
        إذا (Ast_گرہ* ثابت stmtsp = nodep->stmtsp()) {
            resp = Ast_گرہ::جوڑو_اگلا(resp, stmtsp->unlink_Fr_پٹھ_With_اگلا());
        }
        طالما (AstBegin* ثابت beginp = nodep->forksp()) {
            إذا (Ast_گرہ* ثابت declsp = beginp->declsp()) {
                resp = Ast_گرہ::جوڑو_اگلا(resp, declsp->unlink_Fr_پٹھ_With_اگلا());
            }
            إذا (Ast_گرہ* ثابت stmtsp = beginp->stmtsp()) {
                resp = Ast_گرہ::جوڑو_اگلا(resp, stmtsp->unlink_Fr_پٹھ_With_اگلا());
            }
            VL_DO_DANGLING(ڈالو_Deletep(beginp->unlink_Fr_پٹھ()), beginp);
        }
        إذا (resp) {
            nodep->replaceWith(resp);
        } وإلا {
            nodep->unlink_Fr_پٹھ();
        }
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ ویکھو(AstBegin* nodep) override {
        UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
        // Start with children, so later we only find awaits that are actually in this begin
        m_beginHasAwaits = خطأ;
        iterateChildrenConst(nodep);
        إذا (!nodep->stmtsp()) أرجع;
        إذا (!m_beginHasAwaits && !nodep->needProcess()) أرجع;

        UASSERT_OBJ(!nodep->name().خالی_تھاواں(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        File_سطر* ثابت flp = nodep->fileline();
        AstCFunc* ثابت newfuncp = جديد AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->جوڑو_اگلا_ایتھے(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->decl_نجی(سچ);
        // Create the call to the function
        AstCCall* ثابت callp = جديد AstCCall{flp, newfuncp};
        callp->dtype_سیٹ_Void();
        // If we're in a class, add a vlSymsp arg
        إذا (m_وچ_Class) {
            newfuncp->جوڑو_Stmtsp(جديد AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        إذا (Ast_گرہ* ثابت declsp = nodep->declsp()) {
            newfuncp->جوڑو_Stmtsp(declsp->unlink_Fr_پٹھ_With_اگلا());
        }
        إذا (Ast_گرہ* ثابت stmtsp = nodep->stmtsp()) {
            newfuncp->جوڑو_Stmtsp(stmtsp->unlink_Fr_پٹھ_With_اگلا());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->جوڑو_Stmtsp(callp->makeStmt());
        // Propagate if needs process
        إذا (nodep->needProcess()) newfuncp->سیٹ_Need_Process();
        remapLocals(newfuncp, callp);
    }
    فراغ ویکھو(AstCAwait* nodep) override {
        m_beginHasAwaits = سچ;
        iterateChildrenConst(nodep);
    }

    //--------------------
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = افتراضي;
};

فراغ transformForks(AstNetlist* ثابت netlistp) {
    إذا (!v3Global.ورتدا_Timing()) أرجع;
    { TransformForksVisitor{netlistp}; }
    V3Global::dump_Check_Global_درخت("transform_forks", 0, dump_درخت_Either_Level() >= 3);
}

}  // namespace V3Sched
