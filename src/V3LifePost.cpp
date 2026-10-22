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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3LifePost.h"

#அந்தர்பாவ "V3ExecGraph.h"
#அந்தர்பாவ "V3GraphPathChecker.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <memory>
#அந்தர்பாவ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LifePost delay elimination

வகுப்பு LifePostDlyVisitor final : பொது VNVisitorConst {
    // TYPES

    // Location of AstNode within the program
    template <typename T_முனை>
    வகுப்பு Location final {
        template <typename U_முனை>
        friend வகுப்பு Location;

        T_முனை* m_nodep;  // The AstNode being recorded
        மாறிலி AstExecGraph* m_egraphp;  // AstExecTraph location is under, if any
        மாறிலி ExecMTask* m_mtaskp;  // The ExecMTask location is under, if any
        uint32_t m_seq_இலக்கம்;  // Location counter

    பொது:
        Location(T_முனை* nodep, மாறிலி AstExecGraph* egp, மாறிலி ExecMTask* mtaskp, uint32_t seq_இலக்கம்)
            : m_nodep{nodep}
            , m_egraphp{egp}
            , m_mtaskp{mtaskp}
            , m_seq_இலக்கம்{seq_இலக்கம்} {}
        Location() = delete;

        T_முனை* மாறிலி& nodep() மாறிலி { திரும்பு m_nodep; }

        // "is before" - Note: Equality (concurrency) is possible iff they are independent mtasks!
        template <typename U_முனை>
        தருக்க operator<(மாறிலி Location<U_முனை>& that) மாறிலி {
            // If they are in different mtasks under the same graph, check for a path in the graph
            எனில் (m_egraphp && m_egraphp == that.m_egraphp && m_mtaskp != that.m_mtaskp) {
                GraphPathChecker* மாறிலி checkerp = m_egraphp->user1u().to<GraphPathChecker*>();
                திரும்பு checkerp->pathExistsFrom(m_mtaskp, that.m_mtaskp);
            }
            // Otherwise the sequence numbers work (one/both outside graph, or both in same mtask)
            திரும்பு m_seq_இலக்கம் < that.m_seq_இலக்கம்;
        }
    };

    // NODE STATE
    // AstVarScope::user1()    -> bool: referenced outside _eval__nba
    // AstVarScope::user4p()   -> AstVarScope*: Replacement variable
    // AstExecGraph::user1p()  -> GraphPathChecker*: path checker for this AstExecGraph
    மாறிலி VNUser1InUse m_inuser1;
    மாறிலி VNUser4InUse m_inuser4;

    // STATE
    uint32_t m_sequence = 0;  // Sequence number of assigns/varrefs,
    மாறிலி AstExecGraph* m_execGraphp = nullptr;  // Current AstExecGraph being processed (or null)
    மாறிலி ExecMTask* m_execMTaskp = nullptr;  // Current ExecMTask being processed (or null)
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_stat_சொற்கள்_Saved;  // Words removed from NBA shadow copies
    // Maps from Varscope to all their reads and writes
    using LocMap = std::unordered_map<மாறிலி AstVarScope*, std::vector<Location<AstVarRef>>>;
    LocMap m_reads;  // VarScope read locations
    LocMap m_writes;  // VarScope write locations
    std::vector<Location<Ast_முனை_Assign>> m_assigns;  // Assignments considered for removal
    std::vector<std::unique_ptr<GraphPathChecker>> m_checkers;  // Storage for exec graph checkers
    மாறிலி AstCFunc* மாறிலி m_nbaFuncp;  // The 'nba' region entry point
    தருக்க m_inEvalNba = பொய்;  // Traversing under the 'nba' region entry point

    // METHODS
    வெறுமை narrowCopies(Ast_முனை_Assign* postp, AstVarScope* dVscp,
                      மாறிலி std::vector<Location<AstVarRef>>& writes) {
        எனில் (!dVscp->isWide()) திரும்பு;
        Ast_முனை_Assign* மாறிலி prep = VN_AS(writes[0].nodep()->backp(), முனை_Assign);
        UASSERT_OBJ(VN_AS(prep->lhsp(), VarRef)->varScopep()
                            == VN_AS(postp->rhsp(), VarRef)->varScopep()
                        && VN_AS(prep->rhsp(), VarRef)->varScopep()
                               == VN_AS(postp->lhsp(), VarRef)->varScopep(),
                    prep, "NBA shadow pre/post assignments are not reverse copies");

        முழு lsb = dVscp->width();
        முழு end = 0;
        க்கு (size_t i = 1; i < writes.size(); ++i) {
            மாறிலி AstSel* மாறிலி selp = VN_CAST(writes[i].nodep()->backp(), Sel);
            எனில் (!selp || !VN_IS(selp->lsbp(), Const)) திரும்பு;
            மாறிலி முழு start = selp->lsbConst();
            எனில் (start > dVscp->width() - selp->width()) திரும்பு;
            lsb = std::min(lsb, start);
            end = std::max(end, start + selp->width());
        }
        // Copy one word-aligned range enclosing all writes, including any gaps.
        lsb = VL_BITWORD_E(lsb) * VL_EDATASIZE;
        end = std::min(VL_சொற்கள்_I(end) * VL_EDATASIZE, dVscp->width());
        மாறிலி முழு width = end - lsb;
        எனில் (width == dVscp->width()) திரும்பு;
        க்கு (Ast_முனை_Assign* மாறிலி assignp : {prep, postp}) {
            File_வரி* மாறிலி flp = assignp->fileline();
            assignp->lhsp(புதிய AstSel{flp, assignp->lhsp()->unlinkFrBack(), lsb, width});
            assignp->rhsp(புதிய AstSel{flp, assignp->rhsp()->unlinkFrBack(), lsb, width});
            assignp->dtypeFrom(assignp->lhsp());
        }
        m_stat_சொற்கள்_Saved += 2 * (dVscp->width_சொற்கள்() - VL_சொற்கள்_I(width));
    }

    வெறுமை squashAssignposts() {
        க்கு (மாறிலி Location<Ast_முனை_Assign>& assign : m_assigns) {
            AstVarScope* மாறிலி dVscp = VN_AS(assign.nodep()->rhsp(), VarRef)->varScopep();
            AstVarScope* மாறிலி qVscp = VN_AS(assign.nodep()->lhsp(), VarRef)->varScopep();

            // We are considering deleting 'y', don't do it if referenced external to _eval__nba
            எனில் (dVscp->user1()) தொடர்;

            மாறிலி std::vector<Location<AstVarRef>>& dWrites = m_writes[dVscp];
            UASSERT_OBJ(!dWrites.empty(), dVscp, "NBA shadow variable read but never written");

            // *** See file header for requirements ***

            // Proof (1) - Only read is on the RHS of this assignment
            எனில் (m_reads[dVscp].size() > 1) தொடர்;

            // Proof (2) - Only write is on the LHS of this assignment
            எனில் (m_writes[qVscp].size() > 1) தொடர்;

            // Proof (3) - Should always hold
            UASSERT_OBJ(VN_IS(dWrites.at(0).nodep()->backp(), முனை_Assign), dVscp,
                        "Partial first write to NBA shadow variable");

            // Proof (4)
            எனில் (dWrites.size() > 1) {
                // V3Order always serializes writes so they cannot be concurrent
                UASSERT_OBJ(dWrites[0] < dWrites[1], dVscp, "Concurrent writes");
                மாறிலி தருக்க qRdOK = [&]() {
                    க்கு (மாறிலி Location<AstVarRef>& qRead : m_reads[qVscp]) {
                        எனில் (assign < qRead) தொடர்;
                        // Check from 2nd write of 'd'
                        க்கு (size_t i = 1; i < dWrites.size(); ++i) {
                            எனில் (qRead < dWrites[i]) தொடர்;
                            திரும்பு பொய்;
                        }
                    }
                    திரும்பு உண்மை;
                }();
                எனில் (!qRdOK) {
                    narrowCopies(assign.nodep(), dVscp, dWrites);
                    தொடர்;
                }
            }

            // Mark variable for replacement
            dVscp->user4p(qVscp);
            // Delete assignment
            UINFO(4, "    DELETE " << assign.nodep());
            VL_DO_DANGLING(assign.nodep()->unlinkFrBack()->delete_மரம்(), assign.nodep());
            ++m_statAssnDel;
        }
    }

    // Trace code in the given function
    வெறுமை trace(AstCFunc* nodep) {
        VL_RESTORER(m_inEvalNba);
        எனில் (nodep == m_nbaFuncp) m_inEvalNba = உண்மை;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    வெறுமை visit(AstNetlist* nodep) override {
        // First, build maps of every location (mtask and sequence
        // within the mtask) where each varscope is read, and written.
        iterateChildrenConst(nodep);

        // We need to be able to pick up the first write of each variable.
        // V3Order serializes all writes, and we trace AstExecGraph in
        // dependency order, so the first one we encounter during tracing should
        // always be the one. It's somewhat expensive to assert so only with debugCheck().
        எனில் (v3Global.opt.debugCheck()) {
            க்கு (auto& pair : m_writes) {
                மாறிலி std::vector<Location<AstVarRef>>& writes = pair.second;
                மாறிலி Location<AstVarRef>& first = writes[0];
                க்கு (size_t i = 1; i < writes.size(); ++i) {
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
            மாறிலி AstVarScope* மாறிலி vscp = nodep->varScopep();
            AstVarScope* மாறிலி replacementp = VN_AS(vscp->user4p(), VarScope);
            எனில் (!replacementp) திரும்பு;
            UINFO(9, "  Replace " << nodep << " target " << vscp << " with " << replacementp);
            nodep->varScopep(replacementp);
            nodep->varp(replacementp->varp());
        });
    }

    வெறுமை visit(AstVarRef* nodep) override {
        // We only try to optimize NBA shadow variables
        எனில் (!nodep->varScopep()->optimizeLifePost()) திரும்பு;

        // Mark variables referenced outside the 'nba' region
        எனில் (!m_inEvalNba) {
            nodep->varScopep()->user1(உண்மை);
            திரும்பு;
        }

        // Consumption/generation of a variable,
        மாறிலி AstVarScope* மாறிலி vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");

        ++m_sequence;
        எனில் (nodep->access().isWriteOrRW()) {
            m_writes[vscp].emplace_back(nodep, m_execGraphp, m_execMTaskp, m_sequence);
        }
        எனில் (nodep->access().isReadOrRW()) {
            m_reads[vscp].emplace_back(nodep, m_execGraphp, m_execMTaskp, m_sequence);
        }
    }
    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        // Record RHS before assignment
        iterateConst(nodep->rhsp());
        // If a straight assignment between NBA variables, consider for removal
        எனில் (மாறிலி AstVarRef* மாறிலி lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            எனில் (மாறிலி AstVarRef* மாறிலி rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                எனில் (lhsp->varScopep()->optimizeLifePost()  //
                    && rhsp->varScopep()->optimizeLifePost()) {
                    m_assigns.emplace_back(nodep, m_execGraphp, m_execMTaskp, ++m_sequence);
                }
            }
        }
        // Record LHS after assignment
        iterateConst(nodep->lhsp());
    }
    வெறுமை visit(Ast_முனை_CCall* nodep) override {
        iterateChildrenConst(nodep);
        // Entry points are roots of the trace, no need to do it here
        எனில் (nodep->funcp()->entryPoint()) திரும்பு;
        // Trace cellee
        trace(nodep->funcp());
    }
    வெறுமை visit(AstExecGraph* nodep) override {
        UASSERT_OBJ(!m_execGraphp, nodep, "Nested AstExecGraph");
        VL_RESTORER(m_execGraphp);
        m_execGraphp = nodep;

        // Set up the path checker for this graph
        UASSERT_OBJ(!nodep->user1p(), nodep, "AstExecGraph visited twice");
        m_checkers.emplace_back(புதிய GraphPathChecker{nodep->depGraphp()});
        nodep->user1p(m_checkers.back().get());

        // Trace each mtask body. Note: the vertices are in topological order,
        // and we do not reset m_sequence, so a lower sequence number does
        // guarantee a node is not earlier than a higher sequence number, but
        // might still be concurrent.
        க்கு (V3GraphVertex& mtaskVtx : nodep->depGraphp()->vertices()) {
            மாறிலி ExecMTask* மாறிலி mtaskp = mtaskVtx.as<ExecMTask>();
            VL_RESTORER(m_execMTaskp);
            m_execMTaskp = mtaskp;
            trace(mtaskp->funcp());
        }
    }
    வெறுமை visit(AstCFunc* nodep) override {
        // Start a trace from each entry point
        எனில் (nodep->entryPoint()) trace(nodep);
    }
    //-----
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }

பொது:
    // CONSTRUCTORS
    explicit LifePostDlyVisitor(AstNetlist* netlistp)
        : m_nbaFuncp{netlistp->evalFuncp(VEval::NBA)} {
        iterateConst(netlistp);
    }
    ~LifePostDlyVisitor() override {
        V3Stats::addStat("Optimizations, Lifetime postassign deletions", m_statAssnDel);
        V3Stats::addStat("Optimizations, Lifetime NBA copy words removed", m_stat_சொற்கள்_Saved);
    }
};

//######################################################################
// LifePost class functions

வெறுமை V3LifePost::lifepostAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LifePostDlyVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("life_post", 0, dump_மரம்_Either_Level() >= 3);
}
