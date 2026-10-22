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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3LifePost.h"

#ಅನ್ತರ್ಭಾವ "V3ExecGraph.h"
#ಅನ್ತರ್ಭಾವ "V3GraphPathChecker.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LifePost delay elimination

ವರ್ಗ LifePostDlyVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // TYPES

    // Location of AstNode within the program
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_ಗಂಟು>
    ವರ್ಗ Location final {
        ಮಾದರಿ <ಬಗೆ_ಹೆಸರು U_ಗಂಟು>
        ಮಿತ್ರ ವರ್ಗ Location;

        T_ಗಂಟು* m_nodep;  // The AstNode being recorded
        ಸ್ಥಿರಾಂಕ Ast_Exec_ನಕ್ಷೆ* m_egraphp;  // AstExecTraph location is under, if any
        ಸ್ಥಿರಾಂಕ ExecMTask* m_mtaskp;  // The ExecMTask location is under, if any
        uint32_t m_seq_ಅಂಕೆ;  // Location counter

    ಸಾರ್ವಜನಿಕ:
        Location(T_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ Ast_Exec_ನಕ್ಷೆ* egp, ಸ್ಥಿರಾಂಕ ExecMTask* mtaskp, uint32_t seq_ಅಂಕೆ)
            : m_nodep{nodep}
            , m_egraphp{egp}
            , m_mtaskp{mtaskp}
            , m_seq_ಅಂಕೆ{seq_ಅಂಕೆ} {}
        Location() = ಅಳಿಸು;

        T_ಗಂಟು* ಸ್ಥಿರಾಂಕ& nodep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_nodep; }

        // "is before" - Note: Equality (concurrency) is possible iff they are independent mtasks!
        ಮಾದರಿ <ಬಗೆ_ಹೆಸರು U_ಗಂಟು>
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ<(ಸ್ಥಿರಾಂಕ Location<U_ಗಂಟು>& that) ಸ್ಥಿರಾಂಕ {
            // If they are in different mtasks under the same graph, check for a path in the graph
            ಆದರೆ (m_egraphp && m_egraphp == that.m_egraphp && m_mtaskp != that.m_mtaskp) {
                ನಕ್ಷೆ_ಮಾರ್ಗ_Checker* ಸ್ಥಿರಾಂಕ checkerp = m_egraphp->user1u().ಗಮ್ಯ<ನಕ್ಷೆ_ಮಾರ್ಗ_Checker*>();
                ಹಿಂತಿರುಗಿ checkerp->ಮಾರ್ಗ_Exists_From(m_mtaskp, that.m_mtaskp);
            }
            // Otherwise the sequence numbers work (one/both outside graph, or both in same mtask)
            ಹಿಂತಿರುಗಿ m_seq_ಅಂಕೆ < that.m_seq_ಅಂಕೆ;
        }
    };

    // NODE STATE
    // AstVarScope::user1()    -> bool: referenced outside _eval__nba
    // AstVarScope::user4p()   -> AstVarScope*: Replacement variable
    // AstExecGraph::user1p()  -> GraphPathChecker*: path checker for this AstExecGraph
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;
    ಸ್ಥಿರಾಂಕ VNUser4_ಒಳ_Use m_inuser4;

    // STATE
    uint32_t m_sequence = 0;  // Sequence number of assigns/varrefs,
    ಸ್ಥಿರಾಂಕ Ast_Exec_ನಕ್ಷೆ* m_execGraphp = ಶೂನ್ಯಸೂಚಕ;  // Current AstExecGraph being processed (or null)
    ಸ್ಥಿರಾಂಕ ExecMTask* m_execMTaskp = ಶೂನ್ಯಸೂಚಕ;  // Current ExecMTask being processed (or null)
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_stat_ಪದಗಳು_Saved;  // Words removed from NBA shadow copies
    // Maps from Varscope to all their reads and writes
    ಬಳಸಿ LocMap = std::unordered_map<ಸ್ಥಿರಾಂಕ AstVarScope*, std::ಸದಿಶ<Location<AstVarRef>>>;
    LocMap m_reads;  // VarScope read locations
    LocMap m_writes;  // VarScope write locations
    std::ಸದಿಶ<Location<Ast_ಗಂಟು_ನಿಯೋಜಿಸು>> m_assigns;  // Assignments considered for removal
    std::ಸದಿಶ<std::unique_ptr<ನಕ್ಷೆ_ಮಾರ್ಗ_Checker>> m_checkers;  // Storage for exec graph checkers
    ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ m_nbaFuncp;  // The 'nba' region entry point
    ತಾರ್ಕಿಕ m_ಒಳ_Eval_Nba = ಸುಳ್ಳು;  // Traversing under the 'nba' region entry point

    // METHODS
    ಶೂನ್ಯ narrowCopies(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* postp, AstVarScope* dVscp,
                      ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Location<AstVarRef>>& writes) {
        ಆದರೆ (!dVscp->isWide()) ಹಿಂತಿರುಗಿ;
        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ prep = VN_AS(writes[0].nodep()->backp(), ಗಂಟು_ನಿಯೋಜಿಸು);
        UASSERT_OBJ(VN_AS(prep->lhsp(), VarRef)->varScopep()
                            == VN_AS(postp->rhsp(), VarRef)->varScopep()
                        && VN_AS(prep->rhsp(), VarRef)->varScopep()
                               == VN_AS(postp->lhsp(), VarRef)->varScopep(),
                    prep, "NBA shadow pre/post assignments are not reverse copies");

        ಪೂರ್ಣಾಂಕ lsb = dVscp->ಅಗಲ();
        ಪೂರ್ಣಾಂಕ ಕೊನೆ = 0;
        ಗಾಗಿ (ಗಾತ್ರ_t i = 1; i < writes.ಗಾತ್ರ_2(); ++i) {
            ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(writes[i].nodep()->backp(), Sel);
            ಆದರೆ (!selp || !VN_IS(selp->lsbp(), Const)) ಹಿಂತಿರುಗಿ;
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಆರಂಭ = selp->lsbConst();
            ಆದರೆ (ಆರಂಭ > dVscp->ಅಗಲ() - selp->ಅಗಲ()) ಹಿಂತಿರುಗಿ;
            lsb = std::min(lsb, ಆರಂಭ);
            ಕೊನೆ = std::max(ಕೊನೆ, ಆರಂಭ + selp->ಅಗಲ());
        }
        // Copy one word-aligned range enclosing all writes, including any gaps.
        lsb = VL_BITWORD_E(lsb) * VL_EDATASIZE;
        ಕೊನೆ = std::min(VL_ಪದಗಳು_I(ಕೊನೆ) * VL_EDATASIZE, dVscp->ಅಗಲ());
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = ಕೊನೆ - lsb;
        ಆದರೆ (ಅಗಲ == dVscp->ಅಗಲ()) ಹಿಂತಿರುಗಿ;
        ಗಾಗಿ (Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp : {prep, postp}) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = assignp->fileline();
            assignp->lhsp(ಹೊಸ AstSel{flp, assignp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(), lsb, ಅಗಲ});
            assignp->rhsp(ಹೊಸ AstSel{flp, assignp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ(), lsb, ಅಗಲ});
            assignp->dtypeFrom(assignp->lhsp());
        }
        m_stat_ಪದಗಳು_Saved += 2 * (dVscp->ಅಗಲ_ಪದಗಳು() - VL_ಪದಗಳು_I(ಅಗಲ));
    }

    ಶೂನ್ಯ squashAssignposts() {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Location<Ast_ಗಂಟು_ನಿಯೋಜಿಸು>& ನಿಯೋಜಿಸು_2 : m_assigns) {
            AstVarScope* ಸ್ಥಿರಾಂಕ dVscp = VN_AS(ನಿಯೋಜಿಸು_2.nodep()->rhsp(), VarRef)->varScopep();
            AstVarScope* ಸ್ಥಿರಾಂಕ qVscp = VN_AS(ನಿಯೋಜಿಸು_2.nodep()->lhsp(), VarRef)->varScopep();

            // We are considering deleting 'y', don't do it if referenced external to _eval__nba
            ಆದರೆ (dVscp->user1()) ಮುಂದುವರಿ;

            ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Location<AstVarRef>>& dWrites = m_writes[dVscp];
            UASSERT_OBJ(!dWrites.ಖಾಲಿಗಳು(), dVscp, "NBA shadow variable read but never written");

            // *** See file header for requirements ***

            // Proof (1) - Only read is on the RHS of this assignment
            ಆದರೆ (m_reads[dVscp].ಗಾತ್ರ_2() > 1) ಮುಂದುವರಿ;

            // Proof (2) - Only write is on the LHS of this assignment
            ಆದರೆ (m_writes[qVscp].ಗಾತ್ರ_2() > 1) ಮುಂದುವರಿ;

            // Proof (3) - Should always hold
            UASSERT_OBJ(VN_IS(dWrites.at(0).nodep()->backp(), ಗಂಟು_ನಿಯೋಜಿಸು), dVscp,
                        "Partial first write to NBA shadow variable");

            // Proof (4)
            ಆದರೆ (dWrites.ಗಾತ್ರ_2() > 1) {
                // V3Order always serializes writes so they cannot be concurrent
                UASSERT_OBJ(dWrites[0] < dWrites[1], dVscp, "Concurrent writes");
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ qRdOK = [&]() {
                    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Location<AstVarRef>& qRead : m_reads[qVscp]) {
                        ಆದರೆ (ನಿಯೋಜಿಸು_2 < qRead) ಮುಂದುವರಿ;
                        // Check from 2nd write of 'd'
                        ಗಾಗಿ (ಗಾತ್ರ_t i = 1; i < dWrites.ಗಾತ್ರ_2(); ++i) {
                            ಆದರೆ (qRead < dWrites[i]) ಮುಂದುವರಿ;
                            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                        }
                    }
                    ಹಿಂತಿರುಗಿ ನಿಜ;
                }();
                ಆದರೆ (!qRdOK) {
                    narrowCopies(ನಿಯೋಜಿಸು_2.nodep(), dVscp, dWrites);
                    ಮುಂದುವರಿ;
                }
            }

            // Mark variable for replacement
            dVscp->user4p(qVscp);
            // Delete assignment
            UINFO(4, "    DELETE " << ನಿಯೋಜಿಸು_2.nodep());
            VL_DO_DANGLING(ನಿಯೋಜಿಸು_2.nodep()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), ನಿಯೋಜಿಸು_2.nodep());
            ++m_statAssnDel;
        }
    }

    // Trace code in the given function
    ಶೂನ್ಯ trace(AstCFunc* nodep) {
        VL_RESTORER(m_ಒಳ_Eval_Nba);
        ಆದರೆ (nodep == m_nbaFuncp) m_ಒಳ_Eval_Nba = ನಿಜ;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstNetlist* nodep) override {
        // First, build maps of every location (mtask and sequence
        // within the mtask) where each varscope is read, and written.
        iterateChildrenConst(nodep);

        // We need to be able to pick up the first write of each variable.
        // V3Order serializes all writes, and we trace AstExecGraph in
        // dependency order, so the first one we encounter during tracing should
        // always be the one. It's somewhat expensive to assert so only with debugCheck().
        ಆದರೆ (v3Global.opt.debugCheck()) {
            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& pair : m_writes) {
                ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Location<AstVarRef>>& writes = pair.ಎರಡನೇ;
                ಸ್ಥಿರಾಂಕ Location<AstVarRef>& ಮೊದಲ = writes[0];
                ಗಾಗಿ (ಗಾತ್ರ_t i = 1; i < writes.ಗಾತ್ರ_2(); ++i) {
                    UASSERT_OBJ(ಮೊದಲ < writes[i], pair.ಮೊದಲ, "First write is not the first");
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
            ಸ್ಥಿರಾಂಕ AstVarScope* ಸ್ಥಿರಾಂಕ vscp = nodep->varScopep();
            AstVarScope* ಸ್ಥಿರಾಂಕ replacementp = VN_AS(vscp->user4p(), VarScope);
            ಆದರೆ (!replacementp) ಹಿಂತಿರುಗಿ;
            UINFO(9, "  Replace " << nodep << " target " << vscp << " with " << replacementp);
            nodep->varScopep(replacementp);
            nodep->varp(replacementp->varp());
        });
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        // We only try to optimize NBA shadow variables
        ಆದರೆ (!nodep->varScopep()->optimizeLifePost()) ಹಿಂತಿರುಗಿ;

        // Mark variables referenced outside the 'nba' region
        ಆದರೆ (!m_ಒಳ_Eval_Nba) {
            nodep->varScopep()->user1(ನಿಜ);
            ಹಿಂತಿರುಗಿ;
        }

        // Consumption/generation of a variable,
        ಸ್ಥಿರಾಂಕ AstVarScope* ಸ್ಥಿರಾಂಕ vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");

        ++m_sequence;
        ಆದರೆ (nodep->access().isWriteOrRW()) {
            m_writes[vscp].emplace_ಹಿಂಭಾಗ(nodep, m_execGraphp, m_execMTaskp, m_sequence);
        }
        ಆದರೆ (nodep->access().isReadOrRW()) {
            m_reads[vscp].emplace_ಹಿಂಭಾಗ(nodep, m_execGraphp, m_execMTaskp, m_sequence);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        // Record RHS before assignment
        iterateConst(nodep->rhsp());
        // If a straight assignment between NBA variables, consider for removal
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                ಆದರೆ (lhsp->varScopep()->optimizeLifePost()  //
                    && rhsp->varScopep()->optimizeLifePost()) {
                    m_assigns.emplace_ಹಿಂಭಾಗ(nodep, m_execGraphp, m_execMTaskp, ++m_sequence);
                }
            }
        }
        // Record LHS after assignment
        iterateConst(nodep->lhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_CCall* nodep) override {
        iterateChildrenConst(nodep);
        // Entry points are roots of the trace, no need to do it here
        ಆದರೆ (nodep->funcp()->ನಮೂದು_ಬಿಂದು()) ಹಿಂತಿರುಗಿ;
        // Trace cellee
        trace(nodep->funcp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Exec_ನಕ್ಷೆ* nodep) override {
        UASSERT_OBJ(!m_execGraphp, nodep, "Nested AstExecGraph");
        VL_RESTORER(m_execGraphp);
        m_execGraphp = nodep;

        // Set up the path checker for this graph
        UASSERT_OBJ(!nodep->user1p(), nodep, "AstExecGraph visited twice");
        m_checkers.emplace_ಹಿಂಭಾಗ(ಹೊಸ ನಕ್ಷೆ_ಮಾರ್ಗ_Checker{nodep->depGraphp()});
        nodep->user1p(m_checkers.ಹಿಂಭಾಗ().get());

        // Trace each mtask body. Note: the vertices are in topological order,
        // and we do not reset m_sequence, so a lower sequence number does
        // guarantee a node is not earlier than a higher sequence number, but
        // might still be concurrent.
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& mtaskVtx : nodep->depGraphp()->vertices()) {
            ಸ್ಥಿರಾಂಕ ExecMTask* ಸ್ಥಿರಾಂಕ mtaskp = mtaskVtx.as<ExecMTask>();
            VL_RESTORER(m_execMTaskp);
            m_execMTaskp = mtaskp;
            trace(mtaskp->funcp());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        // Start a trace from each entry point
        ಆದರೆ (nodep->ನಮೂದು_ಬಿಂದು()) trace(nodep);
    }
    //-----
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ LifePostDlyVisitor(AstNetlist* netlistp)
        : m_nbaFuncp{netlistp->evalFuncp(VEval::NBA)} {
        iterateConst(netlistp);
    }
    ~LifePostDlyVisitor() override {
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Lifetime postassign deletions", m_statAssnDel);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Lifetime NBA copy words removed", m_stat_ಪದಗಳು_Saved);
    }
};

//######################################################################
// LifePost class functions

ಶೂನ್ಯ V3LifePost::lifepostAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LifePostDlyVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("life_post", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
