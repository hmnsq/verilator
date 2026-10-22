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

std::map<ध्रुव AstVarScope*, std::सदिशम्<uint32_t>>
TimingKit::externalTriggerIndices(ध्रुव TriggerKit& trigKit) ध्रुव {
    std::map<ध्रुव AstVarScope*, std::सदिशम्<uint32_t>> परिणाम;
    प्रति (ध्रुव स्वचालित& vscpDomains : m_externalDomains) {
        std::सदिशम्<uint32_t>& indices = परिणाम[vscpDomains.first];
        प्रति (ध्रुव Ast_Sen_वृक्षः* ध्रुव domainp : vscpDomains.second) {
            प्रति (ध्रुव AstSenItem* itemp = domainp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                indices.प्रक्षिप_पृष्ठभागः(trigKit.senItem2TrigIdx(itemp));
            }
        }
    }
    प्रत्यागम परिणाम;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstVarScope* trigVscp) {
    यदि (!m_resumeFuncp) {
        यदि (m_lbs.रिक्तानि()) प्रत्यागम शून्यसूचकः;
        // Create global resume function
        AstNetlist* ध्रुव netlistp = v3Global.rootp();
        AstScope* ध्रुव scopeTopp = netlistp->शिखरम्_Scopep()->scopep();
        m_resumeFuncp = नूतन AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(सत्य);
        m_resumeFuncp->isLoose(सत्य);
        m_resumeFuncp->isConst(असत्य);
        m_resumeFuncp->decl_वैयक्तिकम्(सत्य);
        scopeTopp->सङ्कलय_Blocksp(m_resumeFuncp);

        प्रति (ध्रुव स्वचालित& प्रविष्टिः : m_lbs) {
            Ast_सक्रिय* ध्रुव activep = प्रविष्टिः.m_activep;
            activep->foreach([एतत्](AstCMethodHard* ध्रुव exprp) {
                यदि (exprp->विधानम्() != VCMethod::SCHED_RESUME) प्रत्यागम;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = exprp->fromp();
                यदि (VN_AS(fromp->dtypep(), BasicDType)->keyword()
                    != VBasicDTypeKwd::TRIGGER_SCHEDULER) {
                    प्रत्यागम;
                }
                AstCMethodHard* ध्रुव move_गन्तव्यम्_Resumep = नूतन AstCMethodHard{
                    fromp->fileline(), fromp->clone_वृक्षः(असत्य),
                    VCMethod::SCHED_MOVE_गन्तव्यम्_RESUME_QUEUE,
                    exprp->pinsp() ? exprp->pinsp()->clone_वृक्षः(सत्य) : शून्यसूचकः};
                move_गन्तव्यम्_Resumep->dtype_समुच्चयः_Void();
                m_resumeFuncp->सङ्कलय_Stmtsp(move_गन्तव्यम्_Resumep->makeStmt());
            });
        }

        // Put all the timing actives in the resume function
        AstIf* dlyShedIfp = शून्यसूचकः;
        प्रति (ध्रुव स्वचालित& प्रविष्टिः : m_lbs) {
            Ast_सक्रिय* ध्रुव activep = प्रविष्टिः.m_activep;
            // Resume time delays last. For no particular reason other than
            // that's what we used to do prior to proper #0 support.
            AstVarRef* ध्रुव schedrefp = VN_AS(
                VN_AS(VN_AS(activep->stmtsp(), Stmt_व्यञ्जकम्)->exprp(), CMethodHard)->fromp(), VarRef);

            Ast_ग्रन्थिः* ध्रुव actionp = activep->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र();
            यदि (schedrefp->varScopep()->dtypep()->basicp()->isDelayScheduler()) {
                File_पङ्क्तिः* ध्रुव flp = trigVscp->fileline();
                ध्रुव std::समुच्चयः<uint32_t>& triggers = प्रविष्टिः.m_triggers;
                AstTriggered* ध्रुव condp
                    = नूतन AstTriggered{flp, trigVscp, {triggers.begin(), triggers.अन्तः_2()}};
                dlyShedIfp = नूतन AstIf{flp, condp};
                dlyShedIfp->सङ्कलय_Thensp(actionp);
            } अन्यथा {
                m_resumeFuncp->सङ्कलय_Stmtsp(actionp);
            }
        }
        यदि (dlyShedIfp) m_resumeFuncp->सङ्कलय_Stmtsp(dlyShedIfp);

        // These are now spent, oispose of now empty AstActive instances
        m_lbs.deleteActives();
    }
    AstCCall* ध्रुव callp = नूतन AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
    callp->dtype_समुच्चयः_Void();
    प्रत्यागम callp;
}

AstVarScope* TimingKit::getDelayScheduler(AstNetlist* ध्रुव /*netlistp*/) {
    प्रति (ध्रुव स्वचालित& प्रविष्टिः : m_lbs) {
        Ast_सक्रिय* ध्रुव ap = प्रविष्टिः.m_activep;
        // TODO: this triple VN_AS expression is ridiculous
        AstVarRef* ध्रुव schedrefp
            = VN_AS(VN_AS(VN_AS(ap->stmtsp(), Stmt_व्यञ्जकम्)->exprp(), CMethodHard)->fromp(), VarRef);
        AstVarScope* ध्रुव vscp = schedrefp->varScopep();
        यदि (vscp->dtypep()->basicp()->isDelayScheduler()) प्रत्यागम vscp;
    }
    // None found. Design doesn't use any time delays
    प्रत्यागम शून्यसूचकः;
}

//============================================================================
// Creates a timing ready call (if needed, else returns null)

AstCCall* TimingKit::createReady(AstVarScope* trigVscp) {
    यदि (!m_readyFuncp) {
        प्रति (ध्रुव स्वचालित& प्रविष्टिः : m_lbs) {
            Ast_सक्रिय* ध्रुव activep = प्रविष्टिः.m_activep;
            स्वचालित* ध्रुव resumep = VN_AS(VN_AS(activep->stmtsp(), Stmt_व्यञ्जकम्)->exprp(), CMethodHard);
            UASSERT_OBJ(!resumep->nextp(), resumep, "Should be the only statement here");
            AstVarScope* ध्रुव schedulerp = VN_AS(resumep->fromp(), VarRef)->varScopep();
            UASSERT_OBJ(schedulerp->dtypep()->basicp()->isDelayScheduler()
                            || schedulerp->dtypep()->basicp()->isTriggerScheduler()
                            || schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler(),
                        schedulerp, "Unexpected type");
            यदि (!schedulerp->dtypep()->basicp()->isTriggerScheduler()) अनुवर्त;
            // Create the global ready function only if we have trigger schedulers
            यदि (!m_readyFuncp) {
                AstNetlist* ध्रुव netlistp = v3Global.rootp();
                AstScope* ध्रुव scopeTopp = netlistp->शिखरम्_Scopep()->scopep();
                m_readyFuncp = नूतन AstCFunc{netlistp->fileline(), "_timing_ready", scopeTopp, ""};
                m_readyFuncp->dontCombine(सत्य);
                m_readyFuncp->isLoose(सत्य);
                m_readyFuncp->isConst(असत्य);
                m_readyFuncp->decl_वैयक्तिकम्(सत्य);
                scopeTopp->सङ्कलय_Blocksp(m_readyFuncp);
            }

            File_पङ्क्तिः* ध्रुव flp = activep->sentreep()->fileline();

            // Create an 'AstIf' sensitive to the suspending triggers
            ध्रुव std::समुच्चयः<uint32_t>& triggers = प्रविष्टिः.m_triggers;
            AstTriggered* ध्रुव condp
                = नूतन AstTriggered{flp, trigVscp, {triggers.begin(), triggers.अन्तः_2()}};
            AstIf* ध्रुव ifp = नूतन AstIf{flp, condp};
            m_readyFuncp->सङ्कलय_Stmtsp(ifp);

            // Mark as ready the processes resumed on this sensitivity expression
            AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, schedulerp, VAccess::READWRITE};
            AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{flp, refp, VCMethod::SCHED_READY};
            callp->dtype_समुच्चयः_Void();
            यदि (resumep->pinsp()) callp->सङ्कलय_Pinsp(resumep->pinsp()->clone_वृक्षः(असत्य));
            ifp->सङ्कलय_Thensp(callp->makeStmt());
        }
        // We still haven't created a ready function (no trigger schedulers), return null
        यदि (!m_readyFuncp) प्रत्यागम शून्यसूचकः;
    }
    AstCCall* ध्रुव callp = नूतन AstCCall{m_readyFuncp->fileline(), m_readyFuncp};
    callp->dtype_समुच्चयः_Void();
    प्रत्यागम callp;
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

वर्ग AwaitVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  AstSenTree::user1()  -> bool.               Set true if the sentree has been visited.
    //  AstCFunc::user1()    -> AstUser1Allocator.  See alocator below
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;

    // STATE
    तार्किक m_अन्तर्_Process = असत्य;  // Are we in a process?
    तार्किक m_gatherVars = असत्य;  // Should we gather vars in m_writtenBySuspendable?
    AstScope* ध्रुव m_scopeTopp;  // Scope at the top
    LogicByScope& m_lbs;  // Timing resume actives
    Ast_ग्रन्थिः_Stmt*& m_postUpdatesr;  // Post updates for the trigger eval function
    // Additional var sensitivities
    std::map<ध्रुव AstVarScope*, std::समुच्चयः<Ast_Sen_वृक्षः*>>& m_externalDomains;
    std::unique_ptr<V3ClassGraph>
        m_classGraphp;  // class graph to get possibly called functions from a virtual call
    std::समुच्चयः<Ast_Sen_वृक्षः*> m_processDomains;  // Sentrees from the current process
    // Variables written by suspendable processes
    std::समुच्चयः<AstVarScope*> m_writtenBySuspendable;
    संरचना CFuncCache final {
        std::समुच्चयः<Ast_Sen_वृक्षः*> m_processDomains;  // What shall be added to m_processDomains
        std::समुच्चयः<AstVarScope*>
            m_writtenBySuspendable;  // What shall be added to m_writtenBySuspendable
        std::समुच्चयः<AstCFunc*> m_includes;  // CFuncs whose CFuncCache shall be included into this
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
    std::सदिशम्<ध्रुव AstCFunc*> m_call_राशिः;  // Current callstack of AstCFuncs
    // Caches how visiting the function with given value of m_gatherVars changes
    // m_processDomains and m_writtenBySuspendable - only accessed from visit(AstCFunc* nodep)
    AstUser1Allocator<AstCFunc, std::array<CFuncCache, 2>> m_cfuncsCache;
    // Count uses of not inlined writes to signals in suspendables
    VDouble0 m_not_Inlined_Writes_अन्तर्_Suspendable_Usage;

    // METHODS
    // Add arguments to a resume() call based on arguments in the suspending call
    शून्य सङ्कलय_Resume_Pins(AstCMethodHard* ध्रुव resumep, Ast_ग्रन्थिः_व्यञ्जकम्* pinsp) {
        AstCExpr* ध्रुव exprp = VN_CAST(pinsp, CExpr);
        AstText* ध्रुव textp = VN_CAST(exprp->nodesp(), Text);
        यदि (textp) {
            // The first argument, vlProcess, isn't used by any of resume() methods, skip it
            यदि ((pinsp = VN_CAST(pinsp->nextp(), ग्रन्थिः_व्यञ्जकम्))) {
                resumep->सङ्कलय_Pinsp(pinsp->clone_वृक्षः(असत्य));
            }
        } अन्यथा {
            resumep->सङ्कलय_Pinsp(pinsp->clone_वृक्षः(असत्य));
        }
    }
    शून्य सङ्कलय_CFunc_Cached_मानानि(ध्रुव AstCFunc* ध्रुव cfuncp,
                              std::unordered_समुच्चयः<ध्रुव AstCFunc*>& visited) {
        यदि (!visited.निवेशय(cfuncp).second) प्रत्यागम;
        CFuncCache& मानम् = m_cfuncsCache(cfuncp)[static_cast<आकार_t>(m_gatherVars)];
        ध्रुव आकार_t initial_आकार = m_writtenBySuspendable.आकार();
        m_writtenBySuspendable.निवेशय(मानम्.m_writtenBySuspendable.begin(),
                                      मानम्.m_writtenBySuspendable.अन्तः_2());
        m_not_Inlined_Writes_अन्तर्_Suspendable_Usage += m_writtenBySuspendable.आकार() - initial_आकार;
        m_processDomains.निवेशय(मानम्.m_processDomains.begin(), मानम्.m_processDomains.अन्तः_2());
        प्रति (ध्रुव AstCFunc* ध्रुव includedp : मानम्.m_includes) {
            सङ्कलय_CFunc_Cached_मानानि(includedp, visited);
        }
    }
    // Create an active with a timing scheduler resume() call
    शून्य create_Resume_सक्रिय(AstCAwait* ध्रुव awaitp) {
        स्वचालित* ध्रुव methodp = VN_AS(awaitp->exprp(), CMethodHard);
        AstVarScope* ध्रुव schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
        Ast_Sen_वृक्षः* ध्रुव sentreep = awaitp->sentreep();
        File_पङ्क्तिः* ध्रुव flp = sentreep->fileline();
        // Create a resume() call on the timing scheduler
        स्वचालित* ध्रुव resumep = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, schedulerp, VAccess::READWRITE}, VCMethod::SCHED_RESUME};
        resumep->dtype_समुच्चयः_Void();
        यदि (schedulerp->dtypep()->basicp()->isTriggerScheduler()) {
            UASSERT_OBJ(methodp->pinsp(), methodp,
                        "Trigger method should have pins from V3Timing");
            // The first pin is the ready boolean, the rest (if any) should be debug info
            // See V3Timing for details
            यदि (Ast_ग्रन्थिः* ध्रुव dbginfop = methodp->pinsp()->nextp()) {
                यदि (methodp->pinsp()) सङ्कलय_Resume_Pins(resumep, static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(dbginfop));
            }
        } अन्यथा यदि (schedulerp->dtypep()->basicp()->isDynamicTriggerScheduler()) {
            स्वचालित* ध्रुव postp = resumep->clone_वृक्षः(असत्य);
            postp->विधानम्(VCMethod::SCHED_DO_POST_UPDATES);
            m_postUpdatesr = Ast_ग्रन्थिः::सङ्कलय_अग्र(m_postUpdatesr, postp->makeStmt());
        }
        // Put it in an active
        Ast_सक्रिय* ध्रुव activep = नूतन Ast_सक्रिय{flp, "_timing", sentreep};
        activep->सङ्कलय_Stmtsp(resumep->makeStmt());
        m_lbs.सङ्कलय(m_scopeTopp, activep);
    }
    // Visit AstCFunc from a AstNodeCCall - this function is rather expensive to call therefore, it
    // only shall be called when necessary
    शून्य दर्शय_Called_CFunc(AstCFunc* ध्रुव nodep) {
        यदि (!m_अन्तर्_Process) प्रत्यागम;  // Skip if not in process - nothing would be added to any set
        ध्रुव आकार_t cache_कुञ्चिका = static_cast<आकार_t>(m_gatherVars);
        CFuncCache& मानम् = m_cfuncsCache(nodep)[cache_कुञ्चिका];
        विकल्प (मानम्.m_state) {
        पक्ष CFuncCache::UNINITIALIZED: {
            // Save current state
            VL_RESTORER_CLEAR(m_processDomains);
            VL_RESTORER_CLEAR(m_writtenBySuspendable);

            // Visit
            मानम्.m_state = CFuncCache::VISITING;
            m_call_राशिः.प्रक्षिप_पृष्ठभागः(nodep);
            iterateChildren(nodep);
            m_call_राशिः.निष्कासय_पृष्ठभागः();
            मानम्.m_state = CFuncCache::INITIALIZED;

            // Save a cache
            std::swap(m_processDomains, मानम्.m_processDomains);
            std::swap(m_writtenBySuspendable, मानम्.m_writtenBySuspendable);
        } भङ्ग;
        पक्ष CFuncCache::VISITING: {
            प्रति (आकार_t i = m_call_राशिः.आकार() - 1; m_call_राशिः.at(i) != nodep; --i) {
                m_cfuncsCache(m_call_राशिः[i])[cache_कुञ्चिका].m_includes.निवेशय(nodep);
            }
            प्रत्यागम;  // Break recursion
        }
        पक्ष CFuncCache::INITIALIZED: भङ्ग;
        }
        std::unordered_समुच्चयः<ध्रुव AstCFunc*> visited;
        सङ्कलय_CFunc_Cached_मानानि(nodep, visited);
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_Procedure* ध्रुव nodep) override {
        UASSERT_OBJ(!m_अन्तर्_Process && !m_gatherVars && m_processDomains.रिक्तानि()
                        && m_writtenBySuspendable.रिक्तानि(),
                    nodep, "Process in process?");
        m_अन्तर्_Process = सत्य;
        m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
        iterateChildren(nodep);
        प्रति (AstVarScope* ध्रुव vscp : m_writtenBySuspendable) {
            m_externalDomains[vscp].निवेशय(m_processDomains.begin(), m_processDomains.अन्तः_2());
            vscp->varp()->समुच्चयः_Written_By_Suspendable();
        }
        m_processDomains.clear();
        m_writtenBySuspendable.clear();
        m_अन्तर्_Process = असत्य;
        m_gatherVars = असत्य;
    }
    शून्य दर्शय(AstFork* nodep) override {
        VL_RESTORER(m_gatherVars);
        यदि (m_अन्तर्_Process) m_gatherVars = सत्य;
        // If not in a process, we don't need to gather variables or domains
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstCAwait* nodep) override {
        यदि (Ast_Sen_वृक्षः* ध्रुव sentreep = nodep->sentreep()) {
            यदि (!sentreep->user1_समुच्चयः_Once()) create_Resume_सक्रिय(nodep);
            यदि (m_अन्तर्_Process) m_processDomains.निवेशय(sentreep);
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        यदि (m_gatherVars && nodep->access().isWriteOrRW() && !nodep->varp()->ignoreSchedWrite()) {
            m_writtenBySuspendable.निवेशय(nodep->varScopep());
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_CCall* ध्रुव nodep) override {
        iterateChildren(nodep);
        // We need to visit bodies of non-inlined functions
        ध्रुव स्वचालित& cfuncps = m_classGraphp->getCallPossibleCFuncs(nodep);
        यदि (cfuncps.रिक्तानि()) {
            दर्शय_Called_CFunc(nodep->funcp());
        } अन्यथा {
            प्रति (AstCFunc* ध्रुव cfuncp : cfuncps) दर्शय_Called_CFunc(cfuncp);
        }
    }
    शून्य दर्शय(AstCFunc* ध्रुव nodep) override {
        ध्रुव स्वचालित& मानम् = m_cfuncsCache(nodep);
        // Check whether it was already visited by visitCalledCFunc()
        यदि (मानम्[0].m_state != CFuncCache::UNINITIALIZED
            || मानम्[1].m_state != CFuncCache::UNINITIALIZED) {
            प्रत्यागम;
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_व्यञ्जकम्_Stmt* nodep) override { iterateChildren(nodep); }

    //--------------------
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, Ast_ग्रन्थिः_Stmt*& postUpdatesr,
                          std::map<ध्रुव AstVarScope*, std::समुच्चयः<Ast_Sen_वृक्षः*>>& externalDomains)
        : m_scopeTopp{nodep->शिखरम्_Scopep()->scopep()}
        , m_lbs{lbs}
        , m_postUpdatesr{postUpdatesr}
        , m_externalDomains{externalDomains}
        , m_classGraphp{V3ClassGraph::build(nodep)} {
        iterate(nodep);
    }
    ~AwaitVisitor() override {
        V3Stats::सङ्कलय_Stat("Scheduling, count of non-inlined signal writes in suspendables",
                         m_not_Inlined_Writes_अन्तर्_Suspendable_Usage);
    }
};

TimingKit prepareTiming(AstNetlist* ध्रुव netlistp) {
    यदि (!v3Global.उपयुङ्क्ते_Timing()) प्रत्यागम {};
    LogicByScope lbs;
    Ast_ग्रन्थिः_Stmt* postUpdates = शून्यसूचकः;
    std::map<ध्रुव AstVarScope*, std::समुच्चयः<Ast_Sen_वृक्षः*>> externalDomains;
    { AwaitVisitor{netlistp, lbs, postUpdates, externalDomains}; }
    प्रत्यागम {std::move(lbs), postUpdates, std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

// Transform all forked processes into functions
वर्ग TransformForksVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current fork.
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;

    // STATE
    तार्किक m_अन्तर्_Class = असत्य;  // Are we in a class?
    तार्किक m_beginHasAwaits = असत्य;  // Does the current begin have awaits?
    AstFork* m_forkp = शून्यसूचकः;  // Current fork
    AstCFunc* m_funcp = शून्यसूचकः;  // Current function

    // METHODS
    // Remap local vars referenced by the given fork function
    // TODO: We should only pass variables to the fork that are
    // live in the fork body, but for that we need a proper data
    // flow analysis framework which we don't have at the moment
    शून्य remapLocals(AstCFunc* ध्रुव funcp, AstCCall* ध्रुव callp) {
        ध्रुव VNUser2_अन्तर्_Use user2_अन्तर्_Use;  // AstVarScope -> AstVarScope: var to remap to
        funcp->foreach([&](Ast_ग्रन्थिः_Var_Ref* refp) {
            AstVar* ध्रुव varp = refp->varp();
            AstVarScope* ध्रुव vscp = refp->varScopep();
            AstBasicDType* ध्रुव dtypep = varp->dtypep()->basicp();
            // If not a fork..join, copy. All write refs should've been handled by V3Fork
            तार्किक pass_By_मानम् = !m_forkp->joinType().join();
            यदि (vscp->fork_By_मानम्()) {
                // Set just before the fork, for its processes to get by value
                pass_By_मानम् = सत्य;
            } अन्यथा यदि (!varp->isFuncLocal()) {
                // Not func local. Its lifetime is longer than the forked process. Skip
                प्रत्यागम;
            } अन्यथा यदि (!varp->user1()) {
                // Not declared before the fork. It cannot outlive the forked process
                प्रत्यागम;
            } अन्यथा यदि (dtypep && dtypep->isForkSync()) {
                // We can just pass it by value to the new function
                pass_By_मानम् = सत्य;
            }
            // Remap the reference
            यदि (!vscp->user2p()) {
                // Clone the var to the new function
                AstVar* ध्रुव newvarp
                    = नूतन AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                newvarp->funcLocal(सत्य);
                newvarp->direction(pass_By_मानम् ? VDirection::निवेशः : VDirection::REF);
                funcp->सङ्कलय_Argsp(newvarp);
                AstVarScope* ध्रुव newvscp
                    = नूतन AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                funcp->scopep()->सङ्कलय_Varsp(newvscp);
                vscp->user2p(newvscp);
                callp->सङ्कलय_Argsp(नूतन AstVarRef{refp->fileline(), vscp,
                                              pass_By_मानम् ? VAccess::READ : VAccess::READWRITE});
            }
            AstVarScope* ध्रुव newvscp = VN_AS(vscp->user2p(), VarScope);
            refp->varScopep(newvscp);
            refp->varp(newvscp->varp());
        });
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_अन्तर्_Class);
        m_अन्तर्_Class = VN_IS(nodep, Class);
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        m_funcp = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstVar* nodep) override {
        यदि (!m_forkp) nodep->user1(सत्य);
    }
    शून्य दर्शय(AstFork* nodep) override {
        यदि (m_forkp) प्रत्यागम;  // Handle forks in forks after moving them to new functions
        VL_RESTORER(m_forkp);
        m_forkp = nodep;
        iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
        // Replace self with the function calls (no co_await, as we don't want the main
        // process to suspend whenever any of the children do)
        // V3Dead could have removed all statements from the fork, so guard against it
        // Inline begins now that they are not needed
        Ast_ग्रन्थिः* resp = शून्यसूचकः;
        यदि (Ast_ग्रन्थिः* ध्रुव declsp = nodep->declsp()) {
            resp = Ast_ग्रन्थिः::सङ्कलय_अग्र(resp, declsp->unlink_Fr_पृष्ठभागः_With_अग्र());
        }
        यदि (Ast_ग्रन्थिः* ध्रुव stmtsp = nodep->stmtsp()) {
            resp = Ast_ग्रन्थिः::सङ्कलय_अग्र(resp, stmtsp->unlink_Fr_पृष्ठभागः_With_अग्र());
        }
        यावत् (AstBegin* ध्रुव beginp = nodep->forksp()) {
            यदि (Ast_ग्रन्थिः* ध्रुव declsp = beginp->declsp()) {
                resp = Ast_ग्रन्थिः::सङ्कलय_अग्र(resp, declsp->unlink_Fr_पृष्ठभागः_With_अग्र());
            }
            यदि (Ast_ग्रन्थिः* ध्रुव stmtsp = beginp->stmtsp()) {
                resp = Ast_ग्रन्थिः::सङ्कलय_अग्र(resp, stmtsp->unlink_Fr_पृष्ठभागः_With_अग्र());
            }
            VL_DO_DANGLING(प्रक्षिप_Deletep(beginp->unlink_Fr_पृष्ठभागः()), beginp);
        }
        यदि (resp) {
            nodep->replaceWith(resp);
        } अन्यथा {
            nodep->unlink_Fr_पृष्ठभागः();
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstBegin* nodep) override {
        UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
        // Start with children, so later we only find awaits that are actually in this begin
        m_beginHasAwaits = असत्य;
        iterateChildrenConst(nodep);
        यदि (!nodep->stmtsp()) प्रत्यागम;
        यदि (!m_beginHasAwaits && !nodep->needProcess()) प्रत्यागम;

        UASSERT_OBJ(!nodep->name().रिक्तानि(), nodep, "Begin needs a name");
        // Create a function to put this begin's statements in
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        AstCFunc* ध्रुव newfuncp = नूतन AstCFunc{flp, m_funcp->name() + "__" + nodep->name(),
                                                m_funcp->scopep(), "VlCoroutine"};

        m_funcp->सङ्कलय_अग्र_अत्र(newfuncp);
        newfuncp->isLoose(m_funcp->isLoose());
        newfuncp->slow(m_funcp->slow());
        newfuncp->isConst(m_funcp->isConst());
        newfuncp->decl_वैयक्तिकम्(सत्य);
        // Create the call to the function
        AstCCall* ध्रुव callp = नूतन AstCCall{flp, newfuncp};
        callp->dtype_समुच्चयः_Void();
        // If we're in a class, add a vlSymsp arg
        यदि (m_अन्तर्_Class) {
            newfuncp->सङ्कलय_Stmtsp(नूतन AstCStmt{flp, "VL_KEEP_THIS;"});
            newfuncp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        // Put the begin's statements in the function
        यदि (Ast_ग्रन्थिः* ध्रुव declsp = nodep->declsp()) {
            newfuncp->सङ्कलय_Stmtsp(declsp->unlink_Fr_पृष्ठभागः_With_अग्र());
        }
        यदि (Ast_ग्रन्थिः* ध्रुव stmtsp = nodep->stmtsp()) {
            newfuncp->सङ्कलय_Stmtsp(stmtsp->unlink_Fr_पृष्ठभागः_With_अग्र());
        }
        // Replace the body of the begin with a call to the newly created function
        nodep->सङ्कलय_Stmtsp(callp->makeStmt());
        // Propagate if needs process
        यदि (nodep->needProcess()) newfuncp->समुच्चयः_Need_Process();
        remapLocals(newfuncp, callp);
    }
    शून्य दर्शय(AstCAwait* nodep) override {
        m_beginHasAwaits = सत्य;
        iterateChildrenConst(nodep);
    }

    //--------------------
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट TransformForksVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~TransformForksVisitor() override = स्वभाव;
};

शून्य transformForks(AstNetlist* ध्रुव netlistp) {
    यदि (!v3Global.उपयुङ्क्ते_Timing()) प्रत्यागम;
    { TransformForksVisitor{netlistp}; }
    V3Global::dump_Check_Global_वृक्षः("transform_forks", 0, dump_वृक्षः_Either_Level() >= 3);
}

}  // namespace V3Sched
