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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3AstUserAllocator.h"
#అన్తర్భావ "V3ClassGraph.h"
#అన్తర్భావ "V3EmitCBase.h"
#అన్తర్భావ "V3Sched.h"

#అన్తర్భావ <unordered_map>
#అన్తర్భావ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

నామాకాశ V3Sched {

//============================================================================
// Trigger indices of the external domains of each variable

std::map<స్థిరరాశి AstVarScope*, std::సదిశ<uint32_t>>
TimingKit::externalTriggerIndices(స్థిరరాశి TriggerKit& trigKit) స్థిరరాశి {
    std::map<స్థిరరాశి AstVarScope*, std::సదిశ<uint32_t>> ఫలితం;
    కొరకు (స్థిరరాశి స్వయంచాలక& vscpDomains : m_externalDomains) {
        std::సదిశ<uint32_t>& indices = ఫలితం[vscpDomains.మొదటి];
        కొరకు (స్థిరరాశి Ast_Sen_వృక్షం* స్థిరరాశి domainp : vscpDomains.రెండవ) {
            కొరకు (స్థిరరాశి Ast_Sen_అంశం* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Sen_అంశం)) {
                indices.నెట్టు_వెనుక(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    తిరిగి ఫలితం;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    ఒకవేళ (!m_resumeFuncp) {
        ఒకవేళ (m_lbs.ఖాళీలు()) తిరిగి శూన్యసూచిక;
        // Create global resume function
        AstNetlist* స్థిరరాశి netlistp = v3Global.rootp();
        AstScope* స్థిరరాశి scopeTopp = netlistp->శిఖరం_Scopep()->scopep();
        m_resumeFuncp = కొత్త AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(నిజం);
        m_resumeFuncp->isLoose(నిజం);
        m_resumeFuncp->isConst(అబద్ధం);
        m_resumeFuncp->decl_వ్యక్తిగత(నిజం);
        scopeTopp->కూడు_Blocksp(m_resumeFuncp);

        కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : m_lbs) {
            Ast_చురుకైన* స్థిరరాశి activep = నమోదు_2.m_activep;
            activep->foreach([ఇది](AstCMethodHard* స్థిరరాశి exprp) {
                ఒకవేళ (exprp->పద్ధతి() != VCMethod::SCHED_RESUME) తిరిగి;
                Ast_కణుపు_సమాసం* స్థిరరాశి fromp = exprp->fromp();
                ఒకవేళ (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    తిరిగి;
                }
                AstCMethodHard* స్థిరరాశి move_గమ్యం_Resumep = కొత్త AstCMethodHard{
                    fromp->fileline(), fromp->clone_వృక్షం(అబద్ధం),
                    VCMethod::SCHED_MOVE_గమ్యం_RESUME_వరుస,
                    exprp->pinsp() ? exprp->pinsp()->clone_వృక్షం(నిజం) : శూన్యసూచిక};
                move_గమ్యం_Resumep->dtype_సమితి_Void();
                m_resumeFuncp->కూడు_Stmtsp(move_గమ్యం_Resumep->makeStmt());
            });
        }

        // Put all the timing actives in the resume function
        AstIf* dlyShedIfp = శూన్యసూచిక;
        కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : m_lbs) {
            Ast_చురుకైన* స్థిరరాశి activep = నమోదు_2.m_activep;
            // Resume time delays last. For no particular reason other than
            // that's what we used to do prior to proper #0 support.
            AstVarRef* స్థిరరాశి schedrefp = VN_AS(
                VN_AS(VN_AS(activep->stmtsp(), Stmt_సమాసం)->exprp(), CMethodHard)->fromp(), VarRef);

            Ast_కణుపు* స్థిరరాశి actionp = activep->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి();
            ఒకవేళ (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                File_పంక్తి* స్థిరరాశి flp = trigVscp->fileline();
                స్థిరరాశి std::సమితి<uint32_t>& triggers = నమోదు_2.m_triggers;
                AstTriggered* స్థిరరాశి condp
                    = కొత్త AstTriggered{flp, trigVscp, {triggers.begin(), triggers.చివర()}};
                dlyShedIfp = కొత్త AstIf{flp, condp};
                dlyShedIfp->కూడు_Thensp(actionp);
            } లేకపోతే {
                m_resumeFuncp->కూడు_Stmtsp(actionp);
            }
        }
        ఒకవేళ (dlyShedIfp) m_resumeFuncp->కూడు_Stmtsp(dlyShedIfp);

        // These are now spent, oispose of now empty AstActive instances
        m_lbs.deleteActives();
    }
    AstCCall* స్థిరరాశి callp = కొత్త AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
    callp->dtype_సమితి_Void();
    తిరిగి callp;
}

AstVarScope* TimingKit::getDelayScheduler(AstNetlist* స్థిరరాశి /*netlistp*/) {
    కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : m_lbs) {
        Ast_చురుకైన* స్థిరరాశి ap = నమోదు_2.m_activep;
        // TODO: this triple VN_AS expression is ridiculous
        AstVarRef* స్థిరరాశి schedrefp
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), Stmt_సమాసం)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* స్థిరరాశి vscp = schedrefp->varScopep();
        ఒకవేళ (vscp->dtypep()->basicp()->isDelayScheduler()) తిరిగి vscp;
    }
    // None found. Design doesn't use any time delays
    తిరిగి శూన్యసూచిక;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    ఒకవేళ (!m_readyFuncp) {
        కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : m_lbs) {
            Ast_చురుకైన* స్థిరరాశి activep = నమోదు_2.m_activep;
            స్వయంచాలక* స్థిరరాశి resumep = VN_AS(VN_AS(activep->stmtsp(), Stmt_సమాసం)->exprp(), CMethodHard);
            UASSERT_OBJ(!resumep->nextp(), resumep, "Should be the only statement here");
            AstVarScope* స్థిరరాశి schedulerp = VN_AS(resumep->fromp(), VarRef)->varScopep();
            UASSERT_OBJ(schedulerp->dtypep()->basicp()->isDelayScheduler()
                            || schedulerp->dtypep()->basicp()->isTriggerScheduler()
                            || schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler(),
                        schedulerp, "Unexpected type");
            ఒకవేళ (!schedulerp->dtypep()->basicp()->isTriggerScheduler()) కొనసాగు;
            // Create the global ready function only if we have trigger schedulers
            ఒకవేళ (!m_readyFuncp) {
                AstNetlist* స్థిరరాశి netlistp = v3Global.rootp();
                AstScope* స్థిరరాశి scopeTopp = netlistp->శిఖరం_Scopep()->scopep();
                m_readyFuncp = కొత్త AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(నిజం);
                m_readyFuncp->isLoose(నిజం);
                m_readyFuncp->isConst(అబద్ధం);
                m_readyFuncp->decl_వ్యక్తిగత(నిజం);
                scopeTopp->కూడు_Blocksp(m_readyFuncp);
            }

            File_పంక్తి* స్థిరరాశి flp = activep->sentreep()->fileline();

            // Create an 'AstIf' sensitive to the suspending triggers
            స్థిరరాశి std::సమితి<uint32_t>& triggers = నమోదు_2.m_triggers;
            AstTriggered* స్థిరరాశి condp
                = కొత్త AstTriggered{flp, trigVscp, {triggers.begin(), triggers.చివర()}};
            AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, condp};
            m_readyFuncp->కూడు_Stmtsp(ifp);

            // Mark as ready the processes resumed on this sensitivity expression
            AstVarRef* స్థిరరాశి refp = కొత్త AstVarRef{flp, schedulerp, VAccess::READWRITE};
            AstCMethodHard* స్థిరరాశి callp = కొత్త AstCMethodHard{flp, refp, VCMethod::SCHED_READY};
            callp->dtype_సమితి_Void();
            ఒకవేళ (resumep->pinsp()) callp->కూడు_Pinsp(resumep->pinsp()->clone_వృక్షం(అబద్ధం));
            ifp->కూడు_Thensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        ఒకవేళ (!m_readyFuncp) తిరిగి శూన్యసూచిక;
    }
    AstCCall* స్థిరరాశి callp = కొత్త AstCCall{m_readyFuncp->fileline(), m_readyFuncp};
    callp->dtype_సమితి_Void();
    తిరిగి callp;
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

తరగతి AwaitVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    //  AstSenTree::user1()  -> bool.               Set true if the sentree has been visited.
    //  AstCFunc::user1()    -> AstUser1Allocator.  See alocator below
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;

    // STATE
    తార్కిక m_లోపల_Process = అబద్ధం;  // Are we in a process?
    తార్కిక m_gatherVars = అబద్ధం;  // Should we gather vars in m_writtenBySuspendable?
    AstScope* స్థిరరాశి m_scopeTopp;  // Scope at the top
    LogicByScope& m_lbs;  // Timing resume actives
    Ast_కణుపు_Stmt*& m_postUpdatesr;  // Post updates for the trigger eval function
    // Additional var sensitivities
    std::map<స్థిరరాశి AstVarScope*, std::సమితి<Ast_Sen_వృక్షం*>>& m_externalDomains;
    std::unique_ptr<V3_Class_లేఖాచిత్రం>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::సమితి<Ast_Sen_వృక్షం*> m_processDomains;  // Sentrees from the current process
    // Variables written by suspendable processes
    std::సమితి<AstVarScope*> m_writtenBySuspendable;
    నిర్మాణం CFuncCache final {
        std::సమితి<Ast_Sen_వృక్షం*> m_processDomains;  // What shall be added to m_processDomains
        std::సమితి<AstVarScope*>
            m_writtenBySuspendable;  // What shall be added to m_writtenBySuspendable
        std::సమితి<AstCFunc*> m_includes;  // CFuncs whose CFuncCache shall be included into this
        // m_includes does not need to keep bool with m_gatherVars to know which cache shall be
        // used since:
        //  Only possible transition in a graph is from `!m_gatherVars` to `m_gatherVars`
        //  (in visit(AstFork*) there is a only possible transition, visit(AstNodeProcedure*) is
        //  not considered since it does not occur during a graph traversal)
        //  therefore, if change of state occurs it is certain that the first node after transition
        //  won't be in visiting state
        //  therefore, every node in a m_includes shall take value of m_gatherVars under which
        //  current CFuncCache is
        గణన State : uint8_t {
            UNINITIALIZED = 0,  // Not initialized members are empty
            VISITING,  // Visiting - needed for breaking recursion
            INITIALIZED,  // Members contains correct values
        } m_state  // Current state of Cache
            = UNINITIALIZED;
    };
    std::సదిశ<స్థిరరాశి AstCFunc*> m_call_దొంతర;  // Current callstack of AstCFuncs
    // Caches how visiting the function with given value of m_gatherVars changes
    // m_processDomains and m_writtenBySuspendable - only accessed from visit(AstCFunc* nodep)
    AstUser1Allocator<AstCFunc, std::array<CFuncCache, 2>> m_cfuncsCache;
    // Count uses of not inlined writes to signals in suspendables
    VDouble0 m_not_Inlined_Writes_లోపల_Suspendable_Usage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    శూన్య కూడు_Resume_Pins(AstCMethodHard* స్థిరరాశి resumep, Ast_కణుపు_సమాసం* pinsp) {
        AstCExpr* స్థిరరాశి exprp = VN_CAST(pinsp, CExpr);
        Ast_పాఠ్యం* స్థిరరాశి textp = VN_CAST(exprp->nodesp(), పాఠ్యం_2);
        ఒకవేళ (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            ఒకవేళ ((pinsp = VN_CAST(pinsp->nextp(), కణుపు_సమాసం))) {
                resumep->కూడు_Pinsp(pinsp->clone_వృక్షం(అబద్ధం));
            }
        } లేకపోతే {
            resumep->కూడు_Pinsp(pinsp->clone_వృక్షం(అబద్ధం));
        }
    }
    శూన్య కూడు_CFunc_Cached_విలువలు(స్థిరరాశి AstCFunc* స్థిరరాశి cfuncp,
                              std::unordered_సమితి<స్థిరరాశి AstCFunc*>& సందర్శించిన) {
        ఒకవేళ (!సందర్శించిన.చొప్పించు(cfuncp).రెండవ) తిరిగి;
        CFuncCache& విలువ = m_cfuncsCache(cfuncp)[static_cast<కొలత_t>(m_gatherVars)];
        స్థిరరాశి కొలత_t initial_కొలత = m_writtenBySuspendable.కొలత();
        m_writtenBySuspendable.చొప్పించు(విలువ.m_writtenBySuspendable.begin(),
                                      విలువ.m_writtenBySuspendable.చివర());
        m_not_Inlined_Writes_లోపల_Suspendable_Usage += m_writtenBySuspendable.కొలత() - initial_కొలత;
        m_processDomains.చొప్పించు(విలువ.m_processDomains.begin(), విలువ.m_processDomains.చివర());
        కొరకు (స్థిరరాశి AstCFunc* స్థిరరాశి includedp : విలువ.m_includes) {
            కూడు_CFunc_Cached_విలువలు(includedp, సందర్శించిన);
        }
    }
    // Create an active with a timing scheduler resume() call
    శూన్య create_Resume_చురుకైన(AstCAwait* స్థిరరాశి awaitp) {
        స్వయంచాలక* స్థిరరాశి methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* స్థిరరాశి schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        Ast_Sen_వృక్షం* స్థిరరాశి sentreep = awaitp->sentreep();
        File_పంక్తి* స్థిరరాశి flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        స్వయంచాలక* స్థిరరాశి resumep = కొత్త AstCMethodHard{
            flp, కొత్త AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtype_సమితి_Void();
        ఒకవేళ (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి dbginfop = methodp->pinsp()->nextp()) {
                ఒకవేళ (methodp->pinsp()) కూడు_Resume_Pins(resumep, static_cast<Ast_కణుపు_సమాసం*>(dbginfop));
            }
        } లేకపోతే ఒకవేళ (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            స్వయంచాలక* స్థిరరాశి postp = resumep->clone_వృక్షం(అబద్ధం);
            postp->పద్ధతి(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = Ast_కణుపు::కూడు_తదుపరి(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        Ast_చురుకైన* స్థిరరాశి activep = కొత్త Ast_చురుకైన{flp, "_timing", sentreep};
        activep->కూడు_Stmtsp(resumep->makeStmt());
        m_lbs.కూడు(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    శూన్య సందర్శించు_Called_CFunc(AstCFunc* స్థిరరాశి nodep) {
        ఒకవేళ (!m_లోపల_Process) తిరిగి;  // Skip if not in process - nothing would be added to any set
        స్థిరరాశి కొలత_t cache_కీలకం = static_cast<కొలత_t>(m_gatherVars);
        CFuncCache& విలువ = m_cfuncsCache(nodep)[cache_కీలకం];
        ఎంపిక (విలువ.m_state) {
        స్థితి CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            విలువ.m_state = CFuncCache::VISITING;
            m_call_దొంతర.నెట్టు_వెనుక(nodep);
            iterateChildren(nodep);
            m_call_దొంతర.తీయి_వెనుక();
            విలువ.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::మార్పిడి(m_processDomains, విలువ.m_processDomains);
            std::మార్పిడి(m_writtenBySuspendable, విలువ.m_writtenBySuspendable);
        } ఆపు;
        స్థితి CFuncCache::VISITING: {
            కొరకు (కొలత_t i = m_call_దొంతర.కొలత() - 1; m_call_దొంతర.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_దొంతర[i])[cache_కీలకం].m_includes.చొప్పించు(nodep);
            }
            తిరిగి;  // Break recursion
        }
        స్థితి CFuncCache::INITIALIZED: ఆపు;
        }
        std::unordered_సమితి<స్థిరరాశి AstCFunc*> సందర్శించిన;
        కూడు_CFunc_Cached_విలువలు(nodep, సందర్శించిన);
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Procedure* స్థిరరాశి nodep) override {
        UASSERT_OBJ(!m_లోపల_Process && !m_gatherVars && m_processDomains.ఖాళీలు()
                        && m_writtenBySuspendable.ఖాళీలు(),
                    nodep, "Process in process?");
        m_లోపల_Process = నిజం;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        కొరకు (AstVarScope* స్థిరరాశి vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].చొప్పించు(m_processDomains.begin(), m_processDomains.చివర());
            vscp->varp()->సమితి_Written_By_Suspendable();
        }
        m_processDomains.clear();
        m_writtenBySuspendable.clear();
        m_లోపల_Process = అబద్ధం;
        m_gatherVars = అబద్ధం;
    }
    శూన్య సందర్శించు(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        ఒకవేళ (m_లోపల_Process) m_gatherVars = నిజం;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstCAwait* nodep) override {
        ఒకవేళ (Ast_Sen_వృక్షం* స్థిరరాశి sentreep = nodep->sentreep()) {
            ఒకవేళ (!sentreep->user1_సమితి_Once()) create_Resume_చురుకైన(nodep);
            ఒకవేళ (m_లోపల_Process) m_processDomains.చొప్పించు(sentreep);
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        ఒకవేళ (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.చొప్పించు(nodep->varScopep());
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_CCall* స్థిరరాశి nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        స్థిరరాశి స్వయంచాలక& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        ఒకవేళ (cfuncps.ఖాళీలు()) {
            సందర్శించు_Called_CFunc(nodep->funcp());
        } లేకపోతే {
            కొరకు (AstCFunc* స్థిరరాశి cfuncp : cfuncps) సందర్శించు_Called_CFunc(cfuncp);
        }
    }
    శూన్య సందర్శించు(AstCFunc* స్థిరరాశి nodep) override {
        స్థిరరాశి స్వయంచాలక& విలువ = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        ఒకవేళ (విలువ[0].m_state != CFuncCache::UNINITIALIZED
            || విలువ[1].m_state != CFuncCache::UNINITIALIZED) {
            తిరిగి;
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_సమాసం_Stmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, Ast_కణుపు_Stmt*& postUpdatesr,
                          std::map<స్థిరరాశి AstVarScope*, std::సమితి<Ast_Sen_వృక్షం*>>& externalDomains)
        : m_scopeTopp{nodep->శిఖరం_Scopep()->scopep()}
        , m_lbs{lbs}
        , m_postUpdatesr{postUpdatesr}
        , m_externalDomains{externalDomains}
        , m_classGraphp{V3_Class_లేఖాచిత్రం::build(nodep)} {
        iterate(nodep);
    }
    ~AwaitVisitor() override {
        V3Stats::కూడు_Stat("Scheduling, count of non-inlined signal writes in suspendables",
                         m_not_Inlined_Writes_లోపల_Suspendable_Usage);
    }
};

TimingKit prepareTiming(AstNetlist* స్థిరరాశి netlistp) {
    ఒకవేళ (!v3Global.ఉపయోగిస్తుంది_Timing()) తిరిగి {};
    LogicByScope lbs;
    Ast_కణుపు_Stmt* postUpdates = శూన్యసూచిక;
    std::map<స్థిరరాశి AstVarScope*, std::సమితి<Ast_Sen_వృక్షం*>> externalDomains;
    { AwaitVisitor{netlistp, lbs, postUpdates, externalDomains}; }
    తిరిగి {std::move(lbs), postUpdates, std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

// Transform all forked processes into functions
తరగతి TransformForksVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current fork.
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;

    // STATE
    తార్కిక m_లోపల_Class = అబద్ధం;  // Are we in a class?
    తార్కిక m_beginHasAwaits = అబద్ధం;  // Does the current begin have awaits?
    AstFork* m_forkp = శూన్యసూచిక;  // Current fork
    AstCFunc* m_funcp = శూన్యసూచిక;  // Current function

    // METHODS
    // Remap local vars referenced by the given fork function
    // TODO: We should only pass variables to the fork that are
    // live in the fork body, but for that we need a proper data
    // flow analysis framework which we don't have at the moment
    శూన్య remapLocals(AstCFunc* స్థిరరాశి funcp, AstCCall* స్థిరరాశి callp) {
        స్థిరరాశి VNUser2_లోపల_Use user2_లోపల_Use;  // AstVarScope -> AstVarScope: var to remap to
        funcp->foreach([&](Ast_కణుపు_Var_Ref* refp) {
            AstVar* స్థిరరాశి varp = refp->varp();
            AstVarScope* స్థిరరాశి vscp = refp->varScopep();
            AstBasicDType* స్థిరరాశి dtypep = varp->dtypep()->basicp();
            // If not a fork..join, copy. All write refs should've been handled by V3Fork
            తార్కిక pass_By_విలువ = !m_forkp->joinType().join();
            ఒకవేళ (vscp->fork_By_విలువ()) {
                // Set just before the fork, for its processes to get by value
                pass_By_విలువ = నిజం;
            } లేకపోతే ఒకవేళ (!varp->isFuncLocal()) {
                // Not func local. Its lifetime is longer than the forked process. Skip
                తిరిగి;
            } లేకపోతే ఒకవేళ (!varp->user1()) {
                // Not declared before the fork. It cannot outlive the forked process
                తిరిగి;
            } లేకపోతే ఒకవేళ (dtypep && dtypep->isForkSync()) {
                // We can just pass it by value to the new function
                pass_By_విలువ = నిజం;
            }
            // Remap the reference
            ఒకవేళ (!vscp->user2p()) {
                // Clone the var to the new function
                AstVar* స్థిరరాశి newvarp
                    = కొత్త AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                newvarp->funcLocal(నిజం);
                newvarp->direction(pass_By_విలువ ? VDirection::ఆదానం : VDirection::REF);
                funcp->కూడు_Argsp(newvarp);
                AstVarScope* స్థిరరాశి newvscp
                    = కొత్త AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->కూడు_Varsp(newvscp);
                vscp->user2p(newvscp);
                callp->కూడు_Argsp(కొత్త AstVarRef{refp->fileline(), vscp,
                                              pass_By_విలువ ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* స్థిరరాశి newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        VL_RESTORER(m_లోపల_Class);
        m_లోపల_Class = VN_IS(nodep, Class);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        m_funcp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstVar* nodep) override {
        ఒకవేళ (!m_forkp) nodep->user1(నిజం);
    }
    శూన్య సందర్శించు(AstFork* nodep) override {
        ఒకవేళ (m_forkp) తిరిగి;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        Ast_కణుపు* resp = శూన్యసూచిక;
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి declsp = nodep->declsp()) {
            resp = Ast_కణుపు::కూడు_తదుపరి(resp, declsp->unlink_Fr_వెనుక_With_తదుపరి());
        }
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి stmtsp = nodep->stmtsp()) {
            resp = Ast_కణుపు::కూడు_తదుపరి(resp, stmtsp->unlink_Fr_వెనుక_With_తదుపరి());
        }
        అయితే (AstBegin* స్థిరరాశి beginp = nodep->forksp()) {
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి declsp = beginp->declsp()) {
                resp = Ast_కణుపు::కూడు_తదుపరి(resp, declsp->unlink_Fr_వెనుక_With_తదుపరి());
            }
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి stmtsp = beginp->stmtsp()) {
                resp = Ast_కణుపు::కూడు_తదుపరి(resp, stmtsp->unlink_Fr_వెనుక_With_తదుపరి());
            }
            VL_DO_DANGLING(నెట్టు_Deletep(beginp->unlink_Fr_వెనుక()), beginp);
        }
        ఒకవేళ (resp) {
            nodep->replaceWith(resp);
        } లేకపోతే {
            nodep->unlink_Fr_వెనుక();
        }
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstBegin* nodep) override {
        UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
        // Start with children, so later we only find awaits that are actually in this begin
        m_beginHasAwaits = అబద్ధం;
        iterateChildrenConst(nodep);
        ఒకవేళ (!nodep->stmtsp()) తిరిగి;
        ఒకవేళ (!m_beginHasAwaits && !nodep->needProcess()) తిరిగి;

        UASSERT_OBJ(!nodep->name().ఖాళీలు(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        AstCFunc* స్థిరరాశి newfuncp = కొత్త AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->కూడు_తదుపరి_ఇక్కడ(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->decl_వ్యక్తిగత(నిజం);
        // Create the call to the function
        AstCCall* స్థిరరాశి callp = కొత్త AstCCall{flp, newfuncp};
        callp->dtype_సమితి_Void();
        // If we're in a class, add a vlSymsp arg
        ఒకవేళ (m_లోపల_Class) {
            newfuncp->కూడు_Stmtsp(కొత్త AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి declsp = nodep->declsp()) {
            newfuncp->కూడు_Stmtsp(declsp->unlink_Fr_వెనుక_With_తదుపరి());
        }
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి stmtsp = nodep->stmtsp()) {
            newfuncp->కూడు_Stmtsp(stmtsp->unlink_Fr_వెనుక_With_తదుపరి());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->కూడు_Stmtsp(callp->makeStmt());
        // Propagate if needs process
        ఒకవేళ (nodep->needProcess()) newfuncp->సమితి_Need_Process();
        remapLocals(newfuncp, callp);
    }
    శూన్య సందర్శించు(AstCAwait* nodep) override {
        m_beginHasAwaits = నిజం;
        iterateChildrenConst(nodep);
    }

    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = అప్రమేయ;
};

శూన్య transformForks(AstNetlist* స్థిరరాశి netlistp) {
    ఒకవేళ (!v3Global.ఉపయోగిస్తుంది_Timing()) తిరిగి;
    { TransformForksVisitor{netlistp}; }
    V3Global::dump_Check_Global_వృక్షం("transform_forks", 0, dump_వృక్షం_Either_Level() >= 3);
}

}  // namespace V3Sched
