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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3LifePost.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3ExecGraph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3GraphPathChecker.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LifePost delay elimination

ਵਰਗ LifePostDlyVisitor final : ਜਨਤਕ VNVisitorConst {
    // TYPES

    // Location of AstNode within the program
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_ਗੰਢ>
    ਵਰਗ Location final {
        ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ U_ਗੰਢ>
        ਮਿੱਤਰ ਵਰਗ Location;

        T_ਗੰਢ* m_nodep;  // The AstNode being recorded
        ਅਚਲ Ast_Exec_ਗ੍ਰਾਫ* m_egraphp;  // AstExecTraph location is under, if any
        ਅਚਲ ExecMTask* m_mtaskp;  // The ExecMTask location is under, if any
        uint32_t m_seq_ਅੰਕ;  // Location counter

    ਜਨਤਕ:
        Location(T_ਗੰਢ* nodep, ਅਚਲ Ast_Exec_ਗ੍ਰਾਫ* egp, ਅਚਲ ExecMTask* mtaskp, uint32_t seq_ਅੰਕ)
            : m_nodep{nodep}
            , m_egraphp{egp}
            , m_mtaskp{mtaskp}
            , m_seq_ਅੰਕ{seq_ਅੰਕ} {}
        Location() = ਮਿਟਾਓ;

        T_ਗੰਢ* ਅਚਲ& nodep() ਅਚਲ { ਵਾਪਸ m_nodep; }

        // "is before" - Note: Equality (concurrency) is possible iff they are independent mtasks!
        ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ U_ਗੰਢ>
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ<(ਅਚਲ Location<U_ਗੰਢ>& that) ਅਚਲ {
            // If they are in different mtasks under the same graph, check for a path in the graph
            ਜੇ (m_egraphp && m_egraphp == that.m_egraphp && m_mtaskp != that.m_mtaskp) {
                ਗ੍ਰਾਫ_ਰਸਤਾ_Checker* ਅਚਲ checkerp = m_egraphp->user1u().ਮੰਜ਼ਿਲ<ਗ੍ਰਾਫ_ਰਸਤਾ_Checker*>();
                ਵਾਪਸ checkerp->ਰਸਤਾ_Exists_From(m_mtaskp, that.m_mtaskp);
            }
            // Otherwise the sequence numbers work (one/both outside graph, or both in same mtask)
            ਵਾਪਸ m_seq_ਅੰਕ < that.m_seq_ਅੰਕ;
        }
    };

    // NODE STATE
    // AstVarScope::user1()    -> bool: referenced outside _eval__nba
    // AstVarScope::user4p()   -> AstVarScope*: Replacement variable
    // AstExecGraph::user1p()  -> GraphPathChecker*: path checker for this AstExecGraph
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
    ਅਚਲ VNUser4_ਵਿੱਚ_Use m_inuser4;

    // STATE
    uint32_t m_sequence = 0;  // Sequence number of assigns/varrefs,
    ਅਚਲ Ast_Exec_ਗ੍ਰਾਫ* m_execGraphp = ਖਾਲੀ_ਸੂਚਕ;  // Current AstExecGraph being processed (or null)
    ਅਚਲ ExecMTask* m_execMTaskp = ਖਾਲੀ_ਸੂਚਕ;  // Current ExecMTask being processed (or null)
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_stat_ਸ਼ਬਦ_Saved;  // Words removed from NBA shadow copies
    // Maps from Varscope to all their reads and writes
    ਵਰਤੋਂ LocMap = std::unordered_map<ਅਚਲ AstVarScope*, std::ਸਦਿਸ਼<Location<AstVarRef>>>;
    LocMap m_reads;  // VarScope read locations
    LocMap m_writes;  // VarScope write locations
    std::ਸਦਿਸ਼<Location<Ast_ਗੰਢ_ਸੌਂਪੋ>> m_assigns;  // Assignments considered for removal
    std::ਸਦਿਸ਼<std::unique_ptr<ਗ੍ਰਾਫ_ਰਸਤਾ_Checker>> m_checkers;  // Storage for exec graph checkers
    ਅਚਲ AstCFunc* ਅਚਲ m_nbaFuncp;  // The 'nba' region entry point
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Eval_Nba = ਝੂਠ;  // Traversing under the 'nba' region entry point

    // METHODS
    ਖਾਲੀ narrowCopies(Ast_ਗੰਢ_ਸੌਂਪੋ* postp, AstVarScope* dVscp,
                      ਅਚਲ std::ਸਦਿਸ਼<Location<AstVarRef>>& writes) {
        ਜੇ (!dVscp->isWide()) ਵਾਪਸ;
        Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ prep = VN_AS(writes[0].nodep()->backp(), ਗੰਢ_ਸੌਂਪੋ);
        UASSERT_OBJ(VN_AS(prep->lhsp(), VarRef)->varScopep()
                            == VN_AS(postp->rhsp(), VarRef)->varScopep()
                        && VN_AS(prep->rhsp(), VarRef)->varScopep()
                               == VN_AS(postp->lhsp(), VarRef)->varScopep(),
                    prep, "NBA shadow pre/post assignments are not reverse copies");

        ਪੂਰਨ lsb = dVscp->ਚੌੜਾਈ();
        ਪੂਰਨ ਸਿਰਾ = 0;
        ਲਈ (ਮਾਪ_t i = 1; i < writes.ਮਾਪ(); ++i) {
            ਅਚਲ AstSel* ਅਚਲ selp = VN_CAST(writes[i].nodep()->backp(), Sel);
            ਜੇ (!selp || !VN_IS(selp->lsbp(), Const)) ਵਾਪਸ;
            ਅਚਲ ਪੂਰਨ ਸ਼ੁਰੂ = selp->lsbConst();
            ਜੇ (ਸ਼ੁਰੂ > dVscp->ਚੌੜਾਈ() - selp->ਚੌੜਾਈ()) ਵਾਪਸ;
            lsb = std::min(lsb, ਸ਼ੁਰੂ);
            ਸਿਰਾ = std::max(ਸਿਰਾ, ਸ਼ੁਰੂ + selp->ਚੌੜਾਈ());
        }
        // Copy one word-aligned range enclosing all writes, including any gaps.
        lsb = VL_BITWORD_E(lsb) * VL_EDATASIZE;
        ਸਿਰਾ = std::min(VL_ਸ਼ਬਦ_I(ਸਿਰਾ) * VL_EDATASIZE, dVscp->ਚੌੜਾਈ());
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = ਸਿਰਾ - lsb;
        ਜੇ (ਚੌੜਾਈ == dVscp->ਚੌੜਾਈ()) ਵਾਪਸ;
        ਲਈ (Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assignp : {prep, postp}) {
            File_ਸਤਰ* ਅਚਲ flp = assignp->fileline();
            assignp->lhsp(ਨਵਾਂ AstSel{flp, assignp->lhsp()->unlink_Fr_ਪਿੱਠ(), lsb, ਚੌੜਾਈ});
            assignp->rhsp(ਨਵਾਂ AstSel{flp, assignp->rhsp()->unlink_Fr_ਪਿੱਠ(), lsb, ਚੌੜਾਈ});
            assignp->dtypeFrom(assignp->lhsp());
        }
        m_stat_ਸ਼ਬਦ_Saved += 2 * (dVscp->ਚੌੜਾਈ_ਸ਼ਬਦ() - VL_ਸ਼ਬਦ_I(ਚੌੜਾਈ));
    }

    ਖਾਲੀ squashAssignposts() {
        ਲਈ (ਅਚਲ Location<Ast_ਗੰਢ_ਸੌਂਪੋ>& ਸੌਂਪੋ_2 : m_assigns) {
            AstVarScope* ਅਚਲ dVscp = VN_AS(ਸੌਂਪੋ_2.nodep()->rhsp(), VarRef)->varScopep();
            AstVarScope* ਅਚਲ qVscp = VN_AS(ਸੌਂਪੋ_2.nodep()->lhsp(), VarRef)->varScopep();

            // We are considering deleting 'y', don't do it if referenced external to _eval__nba
            ਜੇ (dVscp->user1()) ਜਾਰੀ;

            ਅਚਲ std::ਸਦਿਸ਼<Location<AstVarRef>>& dWrites = m_writes[dVscp];
            UASSERT_OBJ(!dWrites.ਖਾਲੀ_ਥਾਵਾਂ(), dVscp, "NBA shadow variable read but never written");

            // *** See file header for requirements ***

            // Proof (1) - Only read is on the RHS of this assignment
            ਜੇ (m_reads[dVscp].ਮਾਪ() > 1) ਜਾਰੀ;

            // Proof (2) - Only write is on the LHS of this assignment
            ਜੇ (m_writes[qVscp].ਮਾਪ() > 1) ਜਾਰੀ;

            // Proof (3) - Should always hold
            UASSERT_OBJ(VN_IS(dWrites.at(0).nodep()->backp(), ਗੰਢ_ਸੌਂਪੋ), dVscp,
                        "Partial first write to NBA shadow variable");

            // Proof (4)
            ਜੇ (dWrites.ਮਾਪ() > 1) {
                // V3Order always serializes writes so they cannot be concurrent
                UASSERT_OBJ(dWrites[0] < dWrites[1], dVscp, "Concurrent writes");
                ਅਚਲ ਤਾਰਕਿਕ qRdOK = [&]() {
                    ਲਈ (ਅਚਲ Location<AstVarRef>& qRead : m_reads[qVscp]) {
                        ਜੇ (ਸੌਂਪੋ_2 < qRead) ਜਾਰੀ;
                        // Check from 2nd write of 'd'
                        ਲਈ (ਮਾਪ_t i = 1; i < dWrites.ਮਾਪ(); ++i) {
                            ਜੇ (qRead < dWrites[i]) ਜਾਰੀ;
                            ਵਾਪਸ ਝੂਠ;
                        }
                    }
                    ਵਾਪਸ ਸੱਚ;
                }();
                ਜੇ (!qRdOK) {
                    narrowCopies(ਸੌਂਪੋ_2.nodep(), dVscp, dWrites);
                    ਜਾਰੀ;
                }
            }

            // Mark variable for replacement
            dVscp->user4p(qVscp);
            // Delete assignment
            UINFO(4, "    DELETE " << ਸੌਂਪੋ_2.nodep());
            VL_DO_DANGLING(ਸੌਂਪੋ_2.nodep()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), ਸੌਂਪੋ_2.nodep());
            ++m_statAssnDel;
        }
    }

    // Trace code in the given function
    ਖਾਲੀ trace(AstCFunc* nodep) {
        VL_RESTORER(m_ਵਿੱਚ_Eval_Nba);
        ਜੇ (nodep == m_nbaFuncp) m_ਵਿੱਚ_Eval_Nba = ਸੱਚ;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstNetlist* nodep) override {
        // First, build maps of every location (mtask and sequence
        // within the mtask) where each varscope is read, and written.
        iterateChildrenConst(nodep);

        // We need to be able to pick up the first write of each variable.
        // V3Order serializes all writes, and we trace AstExecGraph in
        // dependency order, so the first one we encounter during tracing should
        // always be the one. It's somewhat expensive to assert so only with debugCheck().
        ਜੇ (v3Global.opt.debugCheck()) {
            ਲਈ (ਸਵੈਚਾਲਿਤ& pair : m_writes) {
                ਅਚਲ std::ਸਦਿਸ਼<Location<AstVarRef>>& writes = pair.ਦੂਜਾ;
                ਅਚਲ Location<AstVarRef>& ਪਹਿਲਾ = writes[0];
                ਲਈ (ਮਾਪ_t i = 1; i < writes.ਮਾਪ(); ++i) {
                    UASSERT_OBJ(ਪਹਿਲਾ < writes[i], pair.ਪਹਿਲਾ, "First write is not the first");
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
            ਅਚਲ AstVarScope* ਅਚਲ vscp = nodep->varScopep();
            AstVarScope* ਅਚਲ replacementp = VN_AS(vscp->user4p(), VarScope);
            ਜੇ (!replacementp) ਵਾਪਸ;
            UINFO(9, "  Replace " << nodep << " target " << vscp << " with " << replacementp);
            nodep->varScopep(replacementp);
            nodep->varp(replacementp->varp());
        });
    }

    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        // We only try to optimize NBA shadow variables
        ਜੇ (!nodep->varScopep()->optimizeLifePost()) ਵਾਪਸ;

        // Mark variables referenced outside the 'nba' region
        ਜੇ (!m_ਵਿੱਚ_Eval_Nba) {
            nodep->varScopep()->user1(ਸੱਚ);
            ਵਾਪਸ;
        }

        // Consumption/generation of a variable,
        ਅਚਲ AstVarScope* ਅਚਲ vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");

        ++m_sequence;
        ਜੇ (nodep->access().isWriteOrRW()) {
            m_writes[vscp].emplace_ਪਿੱਠ(nodep, m_execGraphp, m_execMTaskp, m_sequence);
        }
        ਜੇ (nodep->access().isReadOrRW()) {
            m_reads[vscp].emplace_ਪਿੱਠ(nodep, m_execGraphp, m_execMTaskp, m_sequence);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        // Record RHS before assignment
        iterateConst(nodep->rhsp());
        // If a straight assignment between NBA variables, consider for removal
        ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                ਜੇ (lhsp->varScopep()->optimizeLifePost()  //
                    && rhsp->varScopep()->optimizeLifePost()) {
                    m_assigns.emplace_ਪਿੱਠ(nodep, m_execGraphp, m_execMTaskp, ++m_sequence);
                }
            }
        }
        // Record LHS after assignment
        iterateConst(nodep->lhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_CCall* nodep) override {
        iterateChildrenConst(nodep);
        // Entry points are roots of the trace, no need to do it here
        ਜੇ (nodep->funcp()->ਇੰਦਰਾਜ_ਬਿੰਦੂ()) ਵਾਪਸ;
        // Trace cellee
        trace(nodep->funcp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Exec_ਗ੍ਰਾਫ* nodep) override {
        UASSERT_OBJ(!m_execGraphp, nodep, "Nested AstExecGraph");
        VL_RESTORER(m_execGraphp);
        m_execGraphp = nodep;

        // Set up the path checker for this graph
        UASSERT_OBJ(!nodep->user1p(), nodep, "AstExecGraph visited twice");
        m_checkers.emplace_ਪਿੱਠ(ਨਵਾਂ ਗ੍ਰਾਫ_ਰਸਤਾ_Checker{nodep->depGraphp()});
        nodep->user1p(m_checkers.ਪਿੱਠ().get());

        // Trace each mtask body. Note: the vertices are in topological order,
        // and we do not reset m_sequence, so a lower sequence number does
        // guarantee a node is not earlier than a higher sequence number, but
        // might still be concurrent.
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& mtaskVtx : nodep->depGraphp()->vertices()) {
            ਅਚਲ ExecMTask* ਅਚਲ mtaskp = mtaskVtx.as<ExecMTask>();
            VL_RESTORER(m_execMTaskp);
            m_execMTaskp = mtaskp;
            trace(mtaskp->funcp());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        // Start a trace from each entry point
        ਜੇ (nodep->ਇੰਦਰਾਜ_ਬਿੰਦੂ()) trace(nodep);
    }
    //-----
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ LifePostDlyVisitor(AstNetlist* netlistp)
        : m_nbaFuncp{netlistp->evalFuncp(VEval::NBA)} {
        iterateConst(netlistp);
    }
    ~LifePostDlyVisitor() override {
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Lifetime postassign deletions", m_statAssnDel);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Lifetime NBA copy words removed", m_stat_ਸ਼ਬਦ_Saved);
    }
};

//######################################################################
// LifePost class functions

ਖਾਲੀ V3LifePost::lifepostAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LifePostDlyVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("life_post", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
