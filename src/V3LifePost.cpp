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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3LifePost.h"

#অন্তর্ভাব "V3ExecGraph.h"
#অন্তর্ভাব "V3GraphPathChecker.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <memory>
#অন্তর্ভাব <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LifePost delay elimination

বর্গ LifePostDlyVisitor final : সার্বজনিক VNVisitorConst {
    // TYPES

    // Location of AstNode within the program
    template <typename T_গ্রন্থি>
    বর্গ Location final {
        template <typename U_গ্রন্থি>
        friend বর্গ Location;

        T_গ্রন্থি* m_nodep;  // The AstNode being recorded
        ধ্রুব AstExecGraph* m_egraphp;  // AstExecTraph location is under, if any
        ধ্রুব ExecMTask* m_mtaskp;  // The ExecMTask location is under, if any
        uint32_t m_seq_অঙ্ক;  // Location counter

    সার্বজনিক:
        Location(T_গ্রন্থি* nodep, ধ্রুব AstExecGraph* egp, ধ্রুব ExecMTask* mtaskp, uint32_t seq_অঙ্ক)
            : m_nodep{nodep}
            , m_egraphp{egp}
            , m_mtaskp{mtaskp}
            , m_seq_অঙ্ক{seq_অঙ্ক} {}
        Location() = delete;

        T_গ্রন্থি* ধ্রুব& nodep() ধ্রুব { প্রত্যাগম m_nodep; }

        // "is before" - Note: Equality (concurrency) is possible iff they are independent mtasks!
        template <typename U_গ্রন্থি>
        তার্কিক operator<(ধ্রুব Location<U_গ্রন্থি>& that) ধ্রুব {
            // If they are in different mtasks under the same graph, check for a path in the graph
            যদি (m_egraphp && m_egraphp == that.m_egraphp && m_mtaskp != that.m_mtaskp) {
                GraphPathChecker* ধ্রুব checkerp = m_egraphp->user1u().to<GraphPathChecker*>();
                প্রত্যাগম checkerp->pathExistsFrom(m_mtaskp, that.m_mtaskp);
            }
            // Otherwise the sequence numbers work (one/both outside graph, or both in same mtask)
            প্রত্যাগম m_seq_অঙ্ক < that.m_seq_অঙ্ক;
        }
    };

    // NODE STATE
    // AstVarScope::user1()    -> bool: referenced outside _eval__nba
    // AstVarScope::user4p()   -> AstVarScope*: Replacement variable
    // AstExecGraph::user1p()  -> GraphPathChecker*: path checker for this AstExecGraph
    ধ্রুব VNUser1InUse m_inuser1;
    ধ্রুব VNUser4InUse m_inuser4;

    // STATE
    uint32_t m_sequence = 0;  // Sequence number of assigns/varrefs,
    ধ্রুব AstExecGraph* m_execGraphp = nullptr;  // Current AstExecGraph being processed (or null)
    ধ্রুব ExecMTask* m_execMTaskp = nullptr;  // Current ExecMTask being processed (or null)
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_stat_শব্দগুলি_Saved;  // Words removed from NBA shadow copies
    // Maps from Varscope to all their reads and writes
    using LocMap = std::unordered_map<ধ্রুব AstVarScope*, std::vector<Location<AstVarRef>>>;
    LocMap m_reads;  // VarScope read locations
    LocMap m_writes;  // VarScope write locations
    std::vector<Location<Ast_গ্রন্থি_Assign>> m_assigns;  // Assignments considered for removal
    std::vector<std::unique_ptr<GraphPathChecker>> m_checkers;  // Storage for exec graph checkers
    ধ্রুব AstCFunc* ধ্রুব m_nbaFuncp;  // The 'nba' region entry point
    তার্কিক m_inEvalNba = অসত্য;  // Traversing under the 'nba' region entry point

    // METHODS
    শূন্য narrowCopies(Ast_গ্রন্থি_Assign* postp, AstVarScope* dVscp,
                      ধ্রুব std::vector<Location<AstVarRef>>& writes) {
        যদি (!dVscp->isWide()) প্রত্যাগম;
        Ast_গ্রন্থি_Assign* ধ্রুব prep = VN_AS(writes[0].nodep()->backp(), গ্রন্থি_Assign);
        UASSERT_OBJ(VN_AS(prep->lhsp(), VarRef)->varScopep()
                            == VN_AS(postp->rhsp(), VarRef)->varScopep()
                        && VN_AS(prep->rhsp(), VarRef)->varScopep()
                               == VN_AS(postp->lhsp(), VarRef)->varScopep(),
                    prep, "NBA shadow pre/post assignments are not reverse copies");

        পূর্ণাঙ্ক lsb = dVscp->width();
        পূর্ণাঙ্ক end = 0;
        প্রতি (size_t i = 1; i < writes.size(); ++i) {
            ধ্রুব AstSel* ধ্রুব selp = VN_CAST(writes[i].nodep()->backp(), Sel);
            যদি (!selp || !VN_IS(selp->lsbp(), Const)) প্রত্যাগম;
            ধ্রুব পূর্ণাঙ্ক start = selp->lsbConst();
            যদি (start > dVscp->width() - selp->width()) প্রত্যাগম;
            lsb = std::min(lsb, start);
            end = std::max(end, start + selp->width());
        }
        // Copy one word-aligned range enclosing all writes, including any gaps.
        lsb = VL_BITWORD_E(lsb) * VL_EDATASIZE;
        end = std::min(VL_শব্দগুলি_I(end) * VL_EDATASIZE, dVscp->width());
        ধ্রুব পূর্ণাঙ্ক width = end - lsb;
        যদি (width == dVscp->width()) প্রত্যাগম;
        প্রতি (Ast_গ্রন্থি_Assign* ধ্রুব assignp : {prep, postp}) {
            File_সারি* ধ্রুব flp = assignp->fileline();
            assignp->lhsp(নূতন AstSel{flp, assignp->lhsp()->unlinkFrBack(), lsb, width});
            assignp->rhsp(নূতন AstSel{flp, assignp->rhsp()->unlinkFrBack(), lsb, width});
            assignp->dtypeFrom(assignp->lhsp());
        }
        m_stat_শব্দগুলি_Saved += 2 * (dVscp->width_শব্দগুলি() - VL_শব্দগুলি_I(width));
    }

    শূন্য squashAssignposts() {
        প্রতি (ধ্রুব Location<Ast_গ্রন্থি_Assign>& assign : m_assigns) {
            AstVarScope* ধ্রুব dVscp = VN_AS(assign.nodep()->rhsp(), VarRef)->varScopep();
            AstVarScope* ধ্রুব qVscp = VN_AS(assign.nodep()->lhsp(), VarRef)->varScopep();

            // We are considering deleting 'y', don't do it if referenced external to _eval__nba
            যদি (dVscp->user1()) অনুবর্ত;

            ধ্রুব std::vector<Location<AstVarRef>>& dWrites = m_writes[dVscp];
            UASSERT_OBJ(!dWrites.empty(), dVscp, "NBA shadow variable read but never written");

            // *** See file header for requirements ***

            // Proof (1) - Only read is on the RHS of this assignment
            যদি (m_reads[dVscp].size() > 1) অনুবর্ত;

            // Proof (2) - Only write is on the LHS of this assignment
            যদি (m_writes[qVscp].size() > 1) অনুবর্ত;

            // Proof (3) - Should always hold
            UASSERT_OBJ(VN_IS(dWrites.at(0).nodep()->backp(), গ্রন্থি_Assign), dVscp,
                        "Partial first write to NBA shadow variable");

            // Proof (4)
            যদি (dWrites.size() > 1) {
                // V3Order always serializes writes so they cannot be concurrent
                UASSERT_OBJ(dWrites[0] < dWrites[1], dVscp, "Concurrent writes");
                ধ্রুব তার্কিক qRdOK = [&]() {
                    প্রতি (ধ্রুব Location<AstVarRef>& qRead : m_reads[qVscp]) {
                        যদি (assign < qRead) অনুবর্ত;
                        // Check from 2nd write of 'd'
                        প্রতি (size_t i = 1; i < dWrites.size(); ++i) {
                            যদি (qRead < dWrites[i]) অনুবর্ত;
                            প্রত্যাগম অসত্য;
                        }
                    }
                    প্রত্যাগম সত্য;
                }();
                যদি (!qRdOK) {
                    narrowCopies(assign.nodep(), dVscp, dWrites);
                    অনুবর্ত;
                }
            }

            // Mark variable for replacement
            dVscp->user4p(qVscp);
            // Delete assignment
            UINFO(4, "    DELETE " << assign.nodep());
            VL_DO_DANGLING(assign.nodep()->unlinkFrBack()->delete_বৃক্ষ(), assign.nodep());
            ++m_statAssnDel;
        }
    }

    // Trace code in the given function
    শূন্য trace(AstCFunc* nodep) {
        VL_RESTORER(m_inEvalNba);
        যদি (nodep == m_nbaFuncp) m_inEvalNba = সত্য;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    শূন্য visit(AstNetlist* nodep) override {
        // First, build maps of every location (mtask and sequence
        // within the mtask) where each varscope is read, and written.
        iterateChildrenConst(nodep);

        // We need to be able to pick up the first write of each variable.
        // V3Order serializes all writes, and we trace AstExecGraph in
        // dependency order, so the first one we encounter during tracing should
        // always be the one. It's somewhat expensive to assert so only with debugCheck().
        যদি (v3Global.opt.debugCheck()) {
            প্রতি (auto& pair : m_writes) {
                ধ্রুব std::vector<Location<AstVarRef>>& writes = pair.second;
                ধ্রুব Location<AstVarRef>& first = writes[0];
                প্রতি (size_t i = 1; i < writes.size(); ++i) {
                    UASSERT_OBJ(first < writes[i], pair.first, "First write is not the first");
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
            ধ্রুব AstVarScope* ধ্রুব vscp = nodep->varScopep();
            AstVarScope* ধ্রুব replacementp = VN_AS(vscp->user4p(), VarScope);
            যদি (!replacementp) প্রত্যাগম;
            UINFO(9, "  Replace " << nodep << " target " << vscp << " with " << replacementp);
            nodep->varScopep(replacementp);
            nodep->varp(replacementp->varp());
        });
    }

    শূন্য visit(AstVarRef* nodep) override {
        // We only try to optimize NBA shadow variables
        যদি (!nodep->varScopep()->optimizeLifePost()) প্রত্যাগম;

        // Mark variables referenced outside the 'nba' region
        যদি (!m_inEvalNba) {
            nodep->varScopep()->user1(সত্য);
            প্রত্যাগম;
        }

        // Consumption/generation of a variable,
        ধ্রুব AstVarScope* ধ্রুব vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");

        ++m_sequence;
        যদি (nodep->access().isWriteOrRW()) {
            m_writes[vscp].emplace_back(nodep, m_execGraphp, m_execMTaskp, m_sequence);
        }
        যদি (nodep->access().isReadOrRW()) {
            m_reads[vscp].emplace_back(nodep, m_execGraphp, m_execMTaskp, m_sequence);
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        // Record RHS before assignment
        iterateConst(nodep->rhsp());
        // If a straight assignment between NBA variables, consider for removal
        যদি (ধ্রুব AstVarRef* ধ্রুব lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            যদি (ধ্রুব AstVarRef* ধ্রুব rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                যদি (lhsp->varScopep()->optimizeLifePost()  //
                    && rhsp->varScopep()->optimizeLifePost()) {
                    m_assigns.emplace_back(nodep, m_execGraphp, m_execMTaskp, ++m_sequence);
                }
            }
        }
        // Record LHS after assignment
        iterateConst(nodep->lhsp());
    }
    শূন্য visit(Ast_গ্রন্থি_CCall* nodep) override {
        iterateChildrenConst(nodep);
        // Entry points are roots of the trace, no need to do it here
        যদি (nodep->funcp()->entryPoint()) প্রত্যাগম;
        // Trace cellee
        trace(nodep->funcp());
    }
    শূন্য visit(AstExecGraph* nodep) override {
        UASSERT_OBJ(!m_execGraphp, nodep, "Nested AstExecGraph");
        VL_RESTORER(m_execGraphp);
        m_execGraphp = nodep;

        // Set up the path checker for this graph
        UASSERT_OBJ(!nodep->user1p(), nodep, "AstExecGraph visited twice");
        m_checkers.emplace_back(নূতন GraphPathChecker{nodep->depGraphp()});
        nodep->user1p(m_checkers.back().get());

        // Trace each mtask body. Note: the vertices are in topological order,
        // and we do not reset m_sequence, so a lower sequence number does
        // guarantee a node is not earlier than a higher sequence number, but
        // might still be concurrent.
        প্রতি (V3GraphVertex& mtaskVtx : nodep->depGraphp()->vertices()) {
            ধ্রুব ExecMTask* ধ্রুব mtaskp = mtaskVtx.as<ExecMTask>();
            VL_RESTORER(m_execMTaskp);
            m_execMTaskp = mtaskp;
            trace(mtaskp->funcp());
        }
    }
    শূন্য visit(AstCFunc* nodep) override {
        // Start a trace from each entry point
        যদি (nodep->entryPoint()) trace(nodep);
    }
    //-----
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    explicit LifePostDlyVisitor(AstNetlist* netlistp)
        : m_nbaFuncp{netlistp->evalFuncp(VEval::NBA)} {
        iterateConst(netlistp);
    }
    ~LifePostDlyVisitor() override {
        V3Stats::addStat("Optimizations, Lifetime postassign deletions", m_statAssnDel);
        V3Stats::addStat("Optimizations, Lifetime NBA copy words removed", m_stat_শব্দগুলি_Saved);
    }
};

//######################################################################
// LifePost class functions

শূন্য V3LifePost::lifepostAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LifePostDlyVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("life_post", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
