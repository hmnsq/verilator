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

std::map<ثابت AstVarScope*, std::ویکټور<uint32_t>>
TimingKit::externalTriggerIndices(ثابت TriggerKit& trigKit) ثابت {
    std::map<ثابت AstVarScope*, std::ویکټور<uint32_t>> پایله;
    لكل (ثابت خپلکاری& vscpDomains : m_externalDomains) {
        std::ویکټور<uint32_t>& indices = پایله[vscpDomains.لومړی];
        لكل (ثابت Ast_Sen_ونه* ثابت domainp : vscpDomains.دوهم) {
            لكل (ثابت Ast_Sen_توکی* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_توکی)) {
                indices.ټېل_شا(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    أرجع پایله;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    إذا (!m_resumeFuncp) {
        إذا (m_lbs.تشه()) أرجع تش_اشاره;
        // Create global resume function
        AstNetlist* ثابت netlistp = v3Global.rootp();
        AstScope* ثابت scopeTopp = netlistp->سر_Scopep()->scopep();
        m_resumeFuncp = جديد AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(ریښتیا);
        m_resumeFuncp->isLoose(ریښتیا);
        m_resumeFuncp->isConst(خطأ);
        m_resumeFuncp->decl_شخصي(ریښتیا);
        scopeTopp->جمع_کړه_Blocksp(m_resumeFuncp);

        لكل (ثابت خپلکاری& ثبت : m_lbs) {
            Ast_فعال* ثابت activep = ثبت.m_activep;
            activep->foreach([هذا](AstCMethodHard* ثابت exprp) {
                إذا (exprp->لاره() != VCMethod::SCHED_RESUME) أرجع;
                Ast_غوټه_بیان* ثابت fromp = exprp->fromp();
                إذا (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    أرجع;
                }
                AstCMethodHard* ثابت move_منزل_Resumep = جديد AstCMethodHard{
                    fromp->fileline(), fromp->clone_ونه(خطأ),
                    VCMethod::SCHED_MOVE_منزل_RESUME_کتار,
                    exprp->pinsp() ? exprp->pinsp()->clone_ونه(ریښتیا) : تش_اشاره};
                move_منزل_Resumep->dtype_سېټ_Void();
                m_resumeFuncp->جمع_کړه_Stmtsp(move_منزل_Resumep->makeStmt());
            });
        }

        // Put all the timing actives in the resume function
        AstIf* dlyShedIfp = تش_اشاره;
        لكل (ثابت خپلکاری& ثبت : m_lbs) {
            Ast_فعال* ثابت activep = ثبت.m_activep;
            // Resume time delays last. For no particular reason other than
            // that's what we used to do prior to proper #0 support.
            AstVarRef* ثابت schedrefp = VN_AS(
                VN_AS(VN_AS(activep->stmtsp(), Stmt_بیان)->exprp(), CMethodHard)->fromp(), VarRef);

            Ast_غوټه* ثابت actionp = activep->stmtsp()->unlink_Fr_شا_With_بل();
            إذا (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                File_کرښه* ثابت flp = trigVscp->fileline();
                ثابت std::سېټ<uint32_t>& triggers = ثبت.m_triggers;
                AstTriggered* ثابت condp
                    = جديد AstTriggered{flp, trigVscp, {triggers.begin(), triggers.پای_ټکی()}};
                dlyShedIfp = جديد AstIf{flp, condp};
                dlyShedIfp->جمع_کړه_Thensp(actionp);
            } وإلا {
                m_resumeFuncp->جمع_کړه_Stmtsp(actionp);
            }
        }
        إذا (dlyShedIfp) m_resumeFuncp->جمع_کړه_Stmtsp(dlyShedIfp);

        // These are now spent, oispose of now empty AstActive instances
        m_lbs.deleteActives();
    }
    AstCCall* ثابت callp = جديد AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
    callp->dtype_سېټ_Void();
    أرجع callp;
}

AstVarScope* TimingKit::getDelayScheduler(AstNetlist* ثابت /*netlistp*/) {
    لكل (ثابت خپلکاری& ثبت : m_lbs) {
        Ast_فعال* ثابت ap = ثبت.m_activep;
        // TODO: this triple VN_AS expression is ridiculous
        AstVarRef* ثابت schedrefp
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), Stmt_بیان)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* ثابت vscp = schedrefp->varScopep();
        إذا (vscp->dtypep()->basicp()->isDelayScheduler()) أرجع vscp;
    }
    // None found. Design doesn't use any time delays
    أرجع تش_اشاره;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    إذا (!m_readyFuncp) {
        لكل (ثابت خپلکاری& ثبت : m_lbs) {
            Ast_فعال* ثابت activep = ثبت.m_activep;
            خپلکاری* ثابت resumep = VN_AS(VN_AS(activep->stmtsp(), Stmt_بیان)->exprp(), CMethodHard);
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
                AstScope* ثابت scopeTopp = netlistp->سر_Scopep()->scopep();
                m_readyFuncp = جديد AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(ریښتیا);
                m_readyFuncp->isLoose(ریښتیا);
                m_readyFuncp->isConst(خطأ);
                m_readyFuncp->decl_شخصي(ریښتیا);
                scopeTopp->جمع_کړه_Blocksp(m_readyFuncp);
            }

            File_کرښه* ثابت flp = activep->sentreep()->fileline();

            // Create an 'AstIf' sensitive to the suspending triggers
            ثابت std::سېټ<uint32_t>& triggers = ثبت.m_triggers;
            AstTriggered* ثابت condp
                = جديد AstTriggered{flp, trigVscp, {triggers.begin(), triggers.پای_ټکی()}};
            AstIf* ثابت ifp = جديد AstIf{flp, condp};
            m_readyFuncp->جمع_کړه_Stmtsp(ifp);

            // Mark as ready the processes resumed on this sensitivity expression
            AstVarRef* ثابت refp = جديد AstVarRef{flp, schedulerp, VAccess::READWRITE};
            AstCMethodHard* ثابت callp = جديد AstCMethodHard{flp, refp, VCMethod::SCHED_READY};
            callp->dtype_سېټ_Void();
            إذا (resumep->pinsp()) callp->جمع_کړه_Pinsp(resumep->pinsp()->clone_ونه(خطأ));
            ifp->جمع_کړه_Thensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        إذا (!m_readyFuncp) أرجع تش_اشاره;
    }
    AstCCall* ثابت callp = جديد AstCCall{m_readyFuncp->fileline(), m_readyFuncp};
    callp->dtype_سېټ_Void();
    أرجع callp;
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

صنف AwaitVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstSenTree::user1()  -> bool.               Set true if the sentree has been visited.
    //  AstCFunc::user1()    -> AstUser1Allocator.  See alocator below
    ثابت VNUser1_دننه_Use m_inuser1;

    // STATE
    منطقي m_دننه_Process = خطأ;  // Are we in a process?
    منطقي m_gatherVars = خطأ;  // Should we gather vars in m_writtenBySuspendable?
    AstScope* ثابت m_scopeTopp;  // Scope at the top
    LogicByScope& m_lbs;  // Timing resume actives
    Ast_غوټه_Stmt*& m_postUpdatesr;  // Post updates for the trigger eval function
    // Additional var sensitivities
    std::map<ثابت AstVarScope*, std::سېټ<Ast_Sen_ونه*>>& m_externalDomains;
    std::unique_ptr<V3_Class_ګراف>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::سېټ<Ast_Sen_ونه*> m_processDomains;  // Sentrees from the current process
    // Variables written by suspendable processes
    std::سېټ<AstVarScope*> m_writtenBySuspendable;
    بنية CFuncCache final {
        std::سېټ<Ast_Sen_ونه*> m_processDomains;  // What shall be added to m_processDomains
        std::سېټ<AstVarScope*>
            m_writtenBySuspendable;  // What shall be added to m_writtenBySuspendable
        std::سېټ<AstCFunc*> m_includes;  // CFuncs whose CFuncCache shall be included into this
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
    std::ویکټور<ثابت AstCFunc*> m_call_ډېری;  // Current callstack of AstCFuncs
    // Caches how visiting the function with given value of m_gatherVars changes
    // m_processDomains and m_writtenBySuspendable - only accessed from visit(AstCFunc* nodep)
    AstUser1Allocator<AstCFunc, std::array<CFuncCache, 2>> m_cfuncsCache;
    // Count uses of not inlined writes to signals in suspendables
    VDouble0 m_not_Inlined_Writes_دننه_Suspendable_Usage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    فراغ جمع_کړه_Resume_Pins(AstCMethodHard* ثابت resumep, Ast_غوټه_بیان* pinsp) {
        AstCExpr* ثابت exprp = VN_CAST(pinsp, CExpr);
        Ast_متن* ثابت textp = VN_CAST(exprp->nodesp(), متن_2);
        إذا (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            إذا ((pinsp = VN_CAST(pinsp->nextp(), غوټه_بیان))) {
                resumep->جمع_کړه_Pinsp(pinsp->clone_ونه(خطأ));
            }
        } وإلا {
            resumep->جمع_کړه_Pinsp(pinsp->clone_ونه(خطأ));
        }
    }
    فراغ جمع_کړه_CFunc_Cached_ارزښتونه(ثابت AstCFunc* ثابت cfuncp,
                              std::unordered_سېټ<ثابت AstCFunc*>& لیدل_شوی) {
        إذا (!لیدل_شوی.ورننباسه(cfuncp).دوهم) أرجع;
        CFuncCache& ارزښت = m_cfuncsCache(cfuncp)[static_cast<اندازه_t>(m_gatherVars)];
        ثابت اندازه_t initial_اندازه = m_writtenBySuspendable.اندازه();
        m_writtenBySuspendable.ورننباسه(ارزښت.m_writtenBySuspendable.begin(),
                                      ارزښت.m_writtenBySuspendable.پای_ټکی());
        m_not_Inlined_Writes_دننه_Suspendable_Usage += m_writtenBySuspendable.اندازه() - initial_اندازه;
        m_processDomains.ورننباسه(ارزښت.m_processDomains.begin(), ارزښت.m_processDomains.پای_ټکی());
        لكل (ثابت AstCFunc* ثابت includedp : ارزښت.m_includes) {
            جمع_کړه_CFunc_Cached_ارزښتونه(includedp, لیدل_شوی);
        }
    }
    // Create an active with a timing scheduler resume() call
    فراغ create_Resume_فعال(AstCAwait* ثابت awaitp) {
        خپلکاری* ثابت methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* ثابت schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        Ast_Sen_ونه* ثابت sentreep = awaitp->sentreep();
        File_کرښه* ثابت flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        خپلکاری* ثابت resumep = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtype_سېټ_Void();
        إذا (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            إذا (Ast_غوټه* ثابت dbginfop = methodp->pinsp()->nextp()) {
                إذا (methodp->pinsp()) جمع_کړه_Resume_Pins(resumep, static_cast<Ast_غوټه_بیان*>(dbginfop));
            }
        } وإلا إذا (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            خپلکاری* ثابت postp = resumep->clone_ونه(خطأ);
            postp->لاره(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = Ast_غوټه::جمع_کړه_بل(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        Ast_فعال* ثابت activep = جديد Ast_فعال{flp, "_timing", sentreep};
        activep->جمع_کړه_Stmtsp(resumep->makeStmt());
        m_lbs.جمع_کړه(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    فراغ لیدنه_Called_CFunc(AstCFunc* ثابت nodep) {
        إذا (!m_دننه_Process) أرجع;  // Skip if not in process - nothing would be added to any set
        ثابت اندازه_t cache_کلی = static_cast<اندازه_t>(m_gatherVars);
        CFuncCache& ارزښت = m_cfuncsCache(nodep)[cache_کلی];
        بدل (ارزښت.m_state) {
        حالة CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            ارزښت.m_state = CFuncCache::VISITING;
            m_call_ډېری.ټېل_شا(nodep);
            iterateChildren(nodep);
            m_call_ډېری.راوباسه_شا();
            ارزښت.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::بدلون(m_processDomains, ارزښت.m_processDomains);
            std::بدلون(m_writtenBySuspendable, ارزښت.m_writtenBySuspendable);
        } اكسر;
        حالة CFuncCache::VISITING: {
            لكل (اندازه_t i = m_call_ډېری.اندازه() - 1; m_call_ډېری.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_ډېری[i])[cache_کلی].m_includes.ورننباسه(nodep);
            }
            أرجع;  // Break recursion
        }
        حالة CFuncCache::INITIALIZED: اكسر;
        }
        std::unordered_سېټ<ثابت AstCFunc*> لیدل_شوی;
        جمع_کړه_CFunc_Cached_ارزښتونه(nodep, لیدل_شوی);
    }

    // VISITORS
    فراغ لیدنه(Ast_غوټه_Procedure* ثابت nodep) override {
        UASSERT_OBJ(!m_دننه_Process && !m_gatherVars && m_processDomains.تشه()
                        && m_writtenBySuspendable.تشه(),
                    nodep, "Process in process?");
        m_دننه_Process = ریښتیا;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        لكل (AstVarScope* ثابت vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].ورننباسه(m_processDomains.begin(), m_processDomains.پای_ټکی());
            vscp->varp()->سېټ_Written_By_Suspendable();
        }
        m_processDomains.clear();
        m_writtenBySuspendable.clear();
        m_دننه_Process = خطأ;
        m_gatherVars = خطأ;
    }
    فراغ لیدنه(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        إذا (m_دننه_Process) m_gatherVars = ریښتیا;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstCAwait* nodep) override {
        إذا (Ast_Sen_ونه* ثابت sentreep = nodep->sentreep()) {
            إذا (!sentreep->user1_سېټ_Once()) create_Resume_فعال(nodep);
            إذا (m_دننه_Process) m_processDomains.ورننباسه(sentreep);
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Var_Ref* nodep) override {
        إذا (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.ورننباسه(nodep->varScopep());
        }
    }
    فراغ لیدنه(Ast_غوټه_CCall* ثابت nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ثابت خپلکاری& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        إذا (cfuncps.تشه()) {
            لیدنه_Called_CFunc(nodep->funcp());
        } وإلا {
            لكل (AstCFunc* ثابت cfuncp : cfuncps) لیدنه_Called_CFunc(cfuncp);
        }
    }
    فراغ لیدنه(AstCFunc* ثابت nodep) override {
        ثابت خپلکاری& ارزښت = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        إذا (ارزښت[0].m_state != CFuncCache::UNINITIALIZED
            || ارزښت[1].m_state != CFuncCache::UNINITIALIZED) {
            أرجع;
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_بیان_Stmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, Ast_غوټه_Stmt*& postUpdatesr,
                          std::map<ثابت AstVarScope*, std::سېټ<Ast_Sen_ونه*>>& externalDomains)
        : m_scopeTopp{nodep->سر_Scopep()->scopep()}
        , m_lbs{lbs}
        , m_postUpdatesr{postUpdatesr}
        , m_externalDomains{externalDomains}
        , m_classGraphp{V3_Class_ګراف::build(nodep)} {
        iterate(nodep);
    }
    ~AwaitVisitor() override {
        V3Stats::جمع_کړه_Stat("Scheduling, count of non-inlined signal writes in suspendables",
                         m_not_Inlined_Writes_دننه_Suspendable_Usage);
    }
};

TimingKit prepareTiming(AstNetlist* ثابت netlistp) {
    إذا (!v3Global.کاروي_Timing()) أرجع {};
    LogicByScope lbs;
    Ast_غوټه_Stmt* postUpdates = تش_اشاره;
    std::map<ثابت AstVarScope*, std::سېټ<Ast_Sen_ونه*>> externalDomains;
    { AwaitVisitor{netlistp, lbs, postUpdates, externalDomains}; }
    أرجع {std::move(lbs), postUpdates, std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

// Transform all forked processes into functions
صنف TransformForksVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current fork.
    ثابت VNUser1_دننه_Use m_inuser1;

    // STATE
    منطقي m_دننه_Class = خطأ;  // Are we in a class?
    منطقي m_beginHasAwaits = خطأ;  // Does the current begin have awaits?
    AstFork* m_forkp = تش_اشاره;  // Current fork
    AstCFunc* m_funcp = تش_اشاره;  // Current function

    // METHODS
    // Remap local vars referenced by the given fork function
    // TODO: We should only pass variables to the fork that are
    // live in the fork body, but for that we need a proper data
    // flow analysis framework which we don't have at the moment
    فراغ remapLocals(AstCFunc* ثابت funcp, AstCCall* ثابت callp) {
        ثابت VNUser2_دننه_Use user2_دننه_Use;  // AstVarScope -> AstVarScope: var to remap to
        funcp->foreach([&](Ast_غوټه_Var_Ref* refp) {
            AstVar* ثابت varp = refp->varp();
            AstVarScope* ثابت vscp = refp->varScopep();
            AstBasicDType* ثابت dtypep = varp->dtypep()->basicp();
            // If not a fork..join, copy. All write refs should've been handled by V3Fork
            منطقي pass_By_ارزښت = !m_forkp->joinType().join();
            إذا (vscp->fork_By_ارزښت()) {
                // Set just before the fork, for its processes to get by value
                pass_By_ارزښت = ریښتیا;
            } وإلا إذا (!varp->isFuncLocal()) {
                // Not func local. Its lifetime is longer than the forked process. Skip
                أرجع;
            } وإلا إذا (!varp->user1()) {
                // Not declared before the fork. It cannot outlive the forked process
                أرجع;
            } وإلا إذا (dtypep && dtypep->isForkSync()) {
                // We can just pass it by value to the new function
                pass_By_ارزښت = ریښتیا;
            }
            // Remap the reference
            إذا (!vscp->user2p()) {
                // Clone the var to the new function
                AstVar* ثابت newvarp
                    = جديد AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                newvarp->funcLocal(ریښتیا);
                newvarp->direction(pass_By_ارزښت ? VDirection::ننوت : VDirection::REF);
                funcp->جمع_کړه_Argsp(newvarp);
                AstVarScope* ثابت newvscp
                    = جديد AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->جمع_کړه_Varsp(newvscp);
                vscp->user2p(newvscp);
                callp->جمع_کړه_Argsp(جديد AstVarRef{refp->fileline(), vscp,
                                              pass_By_ارزښت ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* ثابت newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        VL_RESTORER(m_دننه_Class);
        m_دننه_Class = VN_IS(nodep, Class);
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        m_funcp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstVar* nodep) override {
        إذا (!m_forkp) nodep->user1(ریښتیا);
    }
    فراغ لیدنه(AstFork* nodep) override {
        إذا (m_forkp) أرجع;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        Ast_غوټه* resp = تش_اشاره;
        إذا (Ast_غوټه* ثابت declsp = nodep->declsp()) {
            resp = Ast_غوټه::جمع_کړه_بل(resp, declsp->unlink_Fr_شا_With_بل());
        }
        إذا (Ast_غوټه* ثابت stmtsp = nodep->stmtsp()) {
            resp = Ast_غوټه::جمع_کړه_بل(resp, stmtsp->unlink_Fr_شا_With_بل());
        }
        طالما (AstBegin* ثابت beginp = nodep->forksp()) {
            إذا (Ast_غوټه* ثابت declsp = beginp->declsp()) {
                resp = Ast_غوټه::جمع_کړه_بل(resp, declsp->unlink_Fr_شا_With_بل());
            }
            إذا (Ast_غوټه* ثابت stmtsp = beginp->stmtsp()) {
                resp = Ast_غوټه::جمع_کړه_بل(resp, stmtsp->unlink_Fr_شا_With_بل());
            }
            VL_DO_DANGLING(ټېل_Deletep(beginp->unlink_Fr_شا()), beginp);
        }
        إذا (resp) {
            nodep->replaceWith(resp);
        } وإلا {
            nodep->unlink_Fr_شا();
        }
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstBegin* nodep) override {
        UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
        // Start with children, so later we only find awaits that are actually in this begin
        m_beginHasAwaits = خطأ;
        iterateChildrenConst(nodep);
        إذا (!nodep->stmtsp()) أرجع;
        إذا (!m_beginHasAwaits && !nodep->needProcess()) أرجع;

        UASSERT_OBJ(!nodep->name().تشه(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        File_کرښه* ثابت flp = nodep->fileline();
        AstCFunc* ثابت newfuncp = جديد AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->جمع_کړه_بل_دلته(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->decl_شخصي(ریښتیا);
        // Create the call to the function
        AstCCall* ثابت callp = جديد AstCCall{flp, newfuncp};
        callp->dtype_سېټ_Void();
        // If we're in a class, add a vlSymsp arg
        إذا (m_دننه_Class) {
            newfuncp->جمع_کړه_Stmtsp(جديد AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        إذا (Ast_غوټه* ثابت declsp = nodep->declsp()) {
            newfuncp->جمع_کړه_Stmtsp(declsp->unlink_Fr_شا_With_بل());
        }
        إذا (Ast_غوټه* ثابت stmtsp = nodep->stmtsp()) {
            newfuncp->جمع_کړه_Stmtsp(stmtsp->unlink_Fr_شا_With_بل());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->جمع_کړه_Stmtsp(callp->makeStmt());
        // Propagate if needs process
        إذا (nodep->needProcess()) newfuncp->سېټ_Need_Process();
        remapLocals(newfuncp, callp);
    }
    فراغ لیدنه(AstCAwait* nodep) override {
        m_beginHasAwaits = ریښتیا;
        iterateChildrenConst(nodep);
    }

    //--------------------
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = افتراضي;
};

فراغ transformForks(AstNetlist* ثابت netlistp) {
    إذا (!v3Global.کاروي_Timing()) أرجع;
    { TransformForksVisitor{netlistp}; }
    V3Global::dump_Check_Global_ونه("transform_forks", 0, dump_ونه_Either_Level() >= 3);
}

}  // namespace V3Sched
