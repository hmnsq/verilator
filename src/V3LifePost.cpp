// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: NBA shadow variable assignment elimination
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
// Given a pair of variables 'd' and 'q', where 'd' is the shadow variable
// created by V3Delayed using the ShadowVar scheme. Attemp to turn this code:
//
//      ... reads of 'q' ok here
//  X1:  d = q; // First and complete write of 'd' (the pre-scheduled NBA initial assignment)
//      ... reads of 'q' ok here
//       d = foo; // Second assignment to 'd'
//      ... no reads of 'q' here
//  X2:  q = d; // Only and complete write of 'q', only read of 'd' (the post-scheduled NBA commit)
//      ... reads of 'q' ok here
//
// into:
//
//  X1:  q = q;
//       ...
//       q = foo;
//       ...
//  X2:  q = q;
//
// by replacing 'd' with 'q'. This then allows deletion of 'd' and the two assignments.
//
// More formally, with the non-sequential mtasks graph, we must prove all of these:
//  1) No reads of 'd' anywhere except for the ASSIGNPOST itself
//  2) No write of 'q' anywhere except for the ASSIGNPOST itself
//  3) The first write of 'd' is complete (writes all bits)
//  4) Every read  of 'q' either falls before the second write of 'd', or after only read of 'd'
//
// Notes:
//
// While these rules could be applied to any variables, not just the NBA
// shadow variables. **Proving** that no reads of 'q' happen after the second
// assignment of 'd' is difficult due to the presence of loops (the whole
// eval_nba is inside a loop), virtual methods and other dynamic executions.
// For the NBA shadow variables, we can compute this safely as their use
// is understood as we schedule their first and last assignments specially.
//
// Constraint 2 could be relaxed to "no write of 'q' before the only read of
// 'd', however we only have one write of 'q', created in V3Delayed, so
// trying harder would just be a code coverage hole today.
//
// Constraint 3 should always hold with V3Delayed, will check assert it.
//
// If old-value reads prevent elimination, constant partial writes can still narrow both
// copies to the words containing those writes. Other shadow bits are never read. The
// assignments keep their original positions, so evaluation order and scheduling are unchanged.
//
//*************************************************************************

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3LifePost.h"

#అన్తర్భావ "V3ExecGraph.h"
#అన్తర్భావ "V3GraphPathChecker.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <memory>
#అన్తర్భావ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LifePost delay elimination

తరగతి LifePostDlyVisitor final : బహిరంగ VNVisitorConst {
    // TYPES

    // Location of AstNode within the program
    మూస <రకం_పేరు T_కణుపు>
    తరగతి Location final {
        మూస <రకం_పేరు U_కణుపు>
        మిత్ర తరగతి Location;

        T_కణుపు* m_nodep;  // The AstNode being recorded
        స్థిరరాశి Ast_Exec_లేఖాచిత్రం* m_egraphp;  // AstExecTraph location is under, if any
        స్థిరరాశి ExecMTask* m_mtaskp;  // The ExecMTask location is under, if any
        uint32_t m_seq_అంకె;  // Location counter

    బహిరంగ:
        Location(T_కణుపు* nodep, స్థిరరాశి Ast_Exec_లేఖాచిత్రం* egp, స్థిరరాశి ExecMTask* mtaskp, uint32_t seq_అంకె)
            : m_nodep{nodep}
            , m_egraphp{egp}
            , m_mtaskp{mtaskp}
            , m_seq_అంకె{seq_అంకె} {}
        Location() = తొలగించు;

        T_కణుపు* స్థిరరాశి& nodep() స్థిరరాశి { తిరిగి m_nodep; }

        // "is before" - Note: Equality (concurrency) is possible iff they are independent mtasks!
        మూస <రకం_పేరు U_కణుపు>
        తార్కిక క్రియాకారకం<(స్థిరరాశి Location<U_కణుపు>& that) స్థిరరాశి {
            // If they are in different mtasks under the same graph, check for a path in the graph
            ఒకవేళ (m_egraphp && m_egraphp == that.m_egraphp && m_mtaskp != that.m_mtaskp) {
                లేఖాచిత్రం_మార్గం_Checker* స్థిరరాశి checkerp = m_egraphp->user1u().గమ్యం<లేఖాచిత్రం_మార్గం_Checker*>();
                తిరిగి checkerp->మార్గం_Exists_From(m_mtaskp, that.m_mtaskp);
            }
            // Otherwise the sequence numbers work (one/both outside graph, or both in same mtask)
            తిరిగి m_seq_అంకె < that.m_seq_అంకె;
        }
    };

    // NODE STATE
    // AstVarScope::user1()    -> bool: referenced outside _eval__nba
    // AstVarScope::user4p()   -> AstVarScope*: Replacement variable
    // AstExecGraph::user1p()  -> GraphPathChecker*: path checker for this AstExecGraph
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
    స్థిరరాశి VNUser4_లోపల_Use m_inuser4;

    // STATE
    uint32_t m_sequence = 0;  // Sequence number of assigns/varrefs,
    స్థిరరాశి Ast_Exec_లేఖాచిత్రం* m_execGraphp = శూన్యసూచిక;  // Current AstExecGraph being processed (or null)
    స్థిరరాశి ExecMTask* m_execMTaskp = శూన్యసూచిక;  // Current ExecMTask being processed (or null)
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_stat_పదాలు_Saved;  // Words removed from NBA shadow copies
    // Maps from Varscope to all their reads and writes
    ఉపయోగించి LocMap = std::unordered_map<స్థిరరాశి AstVarScope*, std::సదిశ<Location<AstVarRef>>>;
    LocMap m_reads;  // VarScope read locations
    LocMap m_writes;  // VarScope write locations
    std::సదిశ<Location<Ast_కణుపు_కేటాయించు>> m_assigns;  // Assignments considered for removal
    std::సదిశ<std::unique_ptr<లేఖాచిత్రం_మార్గం_Checker>> m_checkers;  // Storage for exec graph checkers
    స్థిరరాశి AstCFunc* స్థిరరాశి m_nbaFuncp;  // The 'nba' region entry point
    తార్కిక m_లోపల_Eval_Nba = అబద్ధం;  // Traversing under the 'nba' region entry point

    // METHODS
    శూన్య narrowCopies(Ast_కణుపు_కేటాయించు* postp, AstVarScope* dVscp,
                      స్థిరరాశి std::సదిశ<Location<AstVarRef>>& writes) {
        ఒకవేళ (!dVscp->isWide()) తిరిగి;
        Ast_కణుపు_కేటాయించు* స్థిరరాశి prep = VN_AS(writes[0].nodep()->backp(), కణుపు_కేటాయించు);
        UASSERT_OBJ(VN_AS(prep->lhsp(), VarRef)->varScopep()
                            == VN_AS(postp->rhsp(), VarRef)->varScopep()
                        && VN_AS(prep->rhsp(), VarRef)->varScopep()
                               == VN_AS(postp->lhsp(), VarRef)->varScopep(),
                    prep, "NBA shadow pre/post assignments are not reverse copies");

        పూర్ణాంకం lsb = dVscp->వెడల్పు();
        పూర్ణాంకం చివర = 0;
        కొరకు (కొలత_t i = 1; i < writes.కొలత(); ++i) {
            స్థిరరాశి AstSel* స్థిరరాశి selp = VN_CAST(writes[i].nodep()->backp(), Sel);
            ఒకవేళ (!selp || !VN_IS(selp->lsbp(), Const)) తిరిగి;
            స్థిరరాశి పూర్ణాంకం ప్రారంభం = selp->lsbConst();
            ఒకవేళ (ప్రారంభం > dVscp->వెడల్పు() - selp->వెడల్పు()) తిరిగి;
            lsb = std::min(lsb, ప్రారంభం);
            చివర = std::max(చివర, ప్రారంభం + selp->వెడల్పు());
        }
        // Copy one word-aligned range enclosing all writes, including any gaps.
        lsb = VL_BITWORD_E(lsb) * VL_EDATASIZE;
        చివర = std::min(VL_పదాలు_I(చివర) * VL_EDATASIZE, dVscp->వెడల్పు());
        స్థిరరాశి పూర్ణాంకం వెడల్పు = చివర - lsb;
        ఒకవేళ (వెడల్పు == dVscp->వెడల్పు()) తిరిగి;
        కొరకు (Ast_కణుపు_కేటాయించు* స్థిరరాశి assignp : {prep, postp}) {
            File_పంక్తి* స్థిరరాశి flp = assignp->fileline();
            assignp->lhsp(కొత్త AstSel{flp, assignp->lhsp()->unlink_Fr_వెనుక(), lsb, వెడల్పు});
            assignp->rhsp(కొత్త AstSel{flp, assignp->rhsp()->unlink_Fr_వెనుక(), lsb, వెడల్పు});
            assignp->dtypeFrom(assignp->lhsp());
        }
        m_stat_పదాలు_Saved += 2 * (dVscp->వెడల్పు_పదాలు() - VL_పదాలు_I(వెడల్పు));
    }

    శూన్య squashAssignposts() {
        కొరకు (స్థిరరాశి Location<Ast_కణుపు_కేటాయించు>& కేటాయించు_2 : m_assigns) {
            AstVarScope* స్థిరరాశి dVscp = VN_AS(కేటాయించు_2.nodep()->rhsp(), VarRef)->varScopep();
            AstVarScope* స్థిరరాశి qVscp = VN_AS(కేటాయించు_2.nodep()->lhsp(), VarRef)->varScopep();

            // We are considering deleting 'y', don't do it if referenced external to _eval__nba
            ఒకవేళ (dVscp->user1()) కొనసాగు;

            స్థిరరాశి std::సదిశ<Location<AstVarRef>>& dWrites = m_writes[dVscp];
            UASSERT_OBJ(!dWrites.ఖాళీలు(), dVscp, "NBA shadow variable read but never written");

            // *** See file header for requirements ***

            // Proof (1) - Only read is on the RHS of this assignment
            ఒకవేళ (m_reads[dVscp].కొలత() > 1) కొనసాగు;

            // Proof (2) - Only write is on the LHS of this assignment
            ఒకవేళ (m_writes[qVscp].కొలత() > 1) కొనసాగు;

            // Proof (3) - Should always hold
            UASSERT_OBJ(VN_IS(dWrites.at(0).nodep()->backp(), కణుపు_కేటాయించు), dVscp,
                        "Partial first write to NBA shadow variable");

            // Proof (4)
            ఒకవేళ (dWrites.కొలత() > 1) {
                // V3Order always serializes writes so they cannot be concurrent
                UASSERT_OBJ(dWrites[0] < dWrites[1], dVscp, "Concurrent writes");
                స్థిరరాశి తార్కిక qRdOK = [&]() {
                    కొరకు (స్థిరరాశి Location<AstVarRef>& qRead : m_reads[qVscp]) {
                        ఒకవేళ (కేటాయించు_2 < qRead) కొనసాగు;
                        // Check from 2nd write of 'd'
                        కొరకు (కొలత_t i = 1; i < dWrites.కొలత(); ++i) {
                            ఒకవేళ (qRead < dWrites[i]) కొనసాగు;
                            తిరిగి అబద్ధం;
                        }
                    }
                    తిరిగి నిజం;
                }();
                ఒకవేళ (!qRdOK) {
                    narrowCopies(కేటాయించు_2.nodep(), dVscp, dWrites);
                    కొనసాగు;
                }
            }

            // Mark variable for replacement
            dVscp->user4p(qVscp);
            // Delete assignment
            UINFO(4, "    DELETE " << కేటాయించు_2.nodep());
            VL_DO_DANGLING(కేటాయించు_2.nodep()->unlink_Fr_వెనుక()->delete_వృక్షం(), కేటాయించు_2.nodep());
            ++m_statAssnDel;
        }
    }

    // Trace code in the given function
    శూన్య trace(AstCFunc* nodep) {
        VL_RESTORER(m_లోపల_Eval_Nba);
        ఒకవేళ (nodep == m_nbaFuncp) m_లోపల_Eval_Nba = నిజం;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    శూన్య సందర్శించు(AstNetlist* nodep) override {
        // First, build maps of every location (mtask and sequence
        // within the mtask) where each varscope is read, and written.
        iterateChildrenConst(nodep);

        // We need to be able to pick up the first write of each variable.
        // V3Order serializes all writes, and we trace AstExecGraph in
        // dependency order, so the first one we encounter during tracing should
        // always be the one. It's somewhat expensive to assert so only with debugCheck().
        ఒకవేళ (v3Global.opt.debugCheck()) {
            కొరకు (స్వయంచాలక& pair : m_writes) {
                స్థిరరాశి std::సదిశ<Location<AstVarRef>>& writes = pair.రెండవ;
                స్థిరరాశి Location<AstVarRef>& మొదటి = writes[0];
                కొరకు (కొలత_t i = 1; i < writes.కొలత(); ++i) {
                    UASSERT_OBJ(మొదటి < writes[i], pair.మొదటి, "First write is not the first");
                }
            }
        }

        // Find all assignposts. Determine which ones can be
        // eliminated. Remove those, and mark their dly vars' user4 field
        // to indicate we should replace these dly vars with their original
        // variables.
        squashAssignposts();

        // Apply replacements
        nodep->foreach([](AstVarRef* nodep) {
            స్థిరరాశి AstVarScope* స్థిరరాశి vscp = nodep->varScopep();
            AstVarScope* స్థిరరాశి replacementp = VN_AS(vscp->user4p(), VarScope);
            ఒకవేళ (!replacementp) తిరిగి;
            UINFO(9, "  Replace " << nodep << " target " << vscp << " with " << replacementp);
            nodep->varScopep(replacementp);
            nodep->varp(replacementp->varp());
        });
    }

    శూన్య సందర్శించు(AstVarRef* nodep) override {
        // We only try to optimize NBA shadow variables
        ఒకవేళ (!nodep->varScopep()->optimizeLifePost()) తిరిగి;

        // Mark variables referenced outside the 'nba' region
        ఒకవేళ (!m_లోపల_Eval_Nba) {
            nodep->varScopep()->user1(నిజం);
            తిరిగి;
        }

        // Consumption/generation of a variable,
        స్థిరరాశి AstVarScope* స్థిరరాశి vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");

        ++m_sequence;
        ఒకవేళ (nodep->access().isWriteOrRW()) {
            m_writes[vscp].emplace_వెనుక(nodep, m_execGraphp, m_execMTaskp, m_sequence);
        }
        ఒకవేళ (nodep->access().isReadOrRW()) {
            m_reads[vscp].emplace_వెనుక(nodep, m_execGraphp, m_execMTaskp, m_sequence);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        // Record RHS before assignment
        iterateConst(nodep->rhsp());
        // If a straight assignment between NBA variables, consider for removal
        ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                ఒకవేళ (lhsp->varScopep()->optimizeLifePost()  //
                    && rhsp->varScopep()->optimizeLifePost()) {
                    m_assigns.emplace_వెనుక(nodep, m_execGraphp, m_execMTaskp, ++m_sequence);
                }
            }
        }
        // Record LHS after assignment
        iterateConst(nodep->lhsp());
    }
    శూన్య సందర్శించు(Ast_కణుపు_CCall* nodep) override {
        iterateChildrenConst(nodep);
        // Entry points are roots of the trace, no need to do it here
        ఒకవేళ (nodep->funcp()->నమోదు_బిందువు()) తిరిగి;
        // Trace cellee
        trace(nodep->funcp());
    }
    శూన్య సందర్శించు(Ast_Exec_లేఖాచిత్రం* nodep) override {
        UASSERT_OBJ(!m_execGraphp, nodep, "Nested AstExecGraph");
        VL_RESTORER(m_execGraphp);
        m_execGraphp = nodep;

        // Set up the path checker for this graph
        UASSERT_OBJ(!nodep->user1p(), nodep, "AstExecGraph visited twice");
        m_checkers.emplace_వెనుక(కొత్త లేఖాచిత్రం_మార్గం_Checker{nodep->depGraphp()});
        nodep->user1p(m_checkers.వెనుక().get());

        // Trace each mtask body. Note: the vertices are in topological order,
        // and we do not reset m_sequence, so a lower sequence number does
        // guarantee a node is not earlier than a higher sequence number, but
        // might still be concurrent.
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& mtaskVtx : nodep->depGraphp()->vertices()) {
            స్థిరరాశి ExecMTask* స్థిరరాశి mtaskp = mtaskVtx.as<ExecMTask>();
            VL_RESTORER(m_execMTaskp);
            m_execMTaskp = mtaskp;
            trace(mtaskp->funcp());
        }
    }
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        // Start a trace from each entry point
        ఒకవేళ (nodep->నమోదు_బిందువు()) trace(nodep);
    }
    //-----
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన LifePostDlyVisitor(AstNetlist* netlistp)
        : m_nbaFuncp{netlistp->evalFuncp(VEval::NBA)} {
        iterateConst(netlistp);
    }
    ~LifePostDlyVisitor() override {
        V3Stats::కూడు_Stat("Optimizations, Lifetime postassign deletions", m_statAssnDel);
        V3Stats::కూడు_Stat("Optimizations, Lifetime NBA copy words removed", m_stat_పదాలు_Saved);
    }
};

//######################################################################
// LifePost class functions

శూన్య V3LifePost::lifepostAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LifePostDlyVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("life_post", 0, dump_వృక్షం_Either_Level() >= 3);
}
