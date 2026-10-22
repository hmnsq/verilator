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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3LifePost.h"

#अन्तर्भाव "V3ExecGraph.h"
#अन्तर्भाव "V3GraphPathChecker.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <memory>
#अन्तर्भाव <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LifePost delay elimination

वर्ग LifePostDlyVisitor final : सार्वजनिक VNVisitorConst {
    // TYPES

    // Location of AstNode within the program
    ढाँचा <typename T_गाँठ>
    वर्ग Location final {
        ढाँचा <typename U_गाँठ>
        मित्र वर्ग Location;

        T_गाँठ* m_nodep;  // The AstNode being recorded
        ध्रुव AstExecGraph* m_egraphp;  // AstExecTraph location is under, if any
        ध्रुव ExecMTask* m_mtaskp;  // The ExecMTask location is under, if any
        uint32_t m_seq_अंक;  // Location counter

    सार्वजनिक:
        Location(T_गाँठ* nodep, ध्रुव AstExecGraph* egp, ध्रुव ExecMTask* mtaskp, uint32_t seq_अंक)
            : m_nodep{nodep}
            , m_egraphp{egp}
            , m_mtaskp{mtaskp}
            , m_seq_अंक{seq_अंक} {}
        Location() = delete;

        T_गाँठ* ध्रुव& nodep() ध्रुव { प्रत्यागम m_nodep; }

        // "is before" - Note: Equality (concurrency) is possible iff they are independent mtasks!
        ढाँचा <typename U_गाँठ>
        तार्किक चालक<(ध्रुव Location<U_गाँठ>& that) ध्रुव {
            // If they are in different mtasks under the same graph, check for a path in the graph
            यदि (m_egraphp && m_egraphp == that.m_egraphp && m_mtaskp != that.m_mtaskp) {
                GraphPathChecker* ध्रुव checkerp = m_egraphp->user1u().to<GraphPathChecker*>();
                प्रत्यागम checkerp->pathExistsFrom(m_mtaskp, that.m_mtaskp);
            }
            // Otherwise the sequence numbers work (one/both outside graph, or both in same mtask)
            प्रत्यागम m_seq_अंक < that.m_seq_अंक;
        }
    };

    // NODE STATE
    // AstVarScope::user1()    -> bool: referenced outside _eval__nba
    // AstVarScope::user4p()   -> AstVarScope*: Replacement variable
    // AstExecGraph::user1p()  -> GraphPathChecker*: path checker for this AstExecGraph
    ध्रुव VNUser1InUse m_inuser1;
    ध्रुव VNUser4InUse m_inuser4;

    // STATE
    uint32_t m_sequence = 0;  // Sequence number of assigns/varrefs,
    ध्रुव AstExecGraph* m_execGraphp = nullptr;  // Current AstExecGraph being processed (or null)
    ध्रुव ExecMTask* m_execMTaskp = nullptr;  // Current ExecMTask being processed (or null)
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_stat_शब्दगण_Saved;  // Words removed from NBA shadow copies
    // Maps from Varscope to all their reads and writes
    using LocMap = std::unordered_map<ध्रुव AstVarScope*, std::vector<Location<AstVarRef>>>;
    LocMap m_reads;  // VarScope read locations
    LocMap m_writes;  // VarScope write locations
    std::vector<Location<Ast_गाँठ_Assign>> m_assigns;  // Assignments considered for removal
    std::vector<std::unique_ptr<GraphPathChecker>> m_checkers;  // Storage for exec graph checkers
    ध्रुव AstCFunc* ध्रुव m_nbaFuncp;  // The 'nba' region entry point
    तार्किक m_inEvalNba = असत्य;  // Traversing under the 'nba' region entry point

    // METHODS
    शून्य narrowCopies(Ast_गाँठ_Assign* postp, AstVarScope* dVscp,
                      ध्रुव std::vector<Location<AstVarRef>>& writes) {
        यदि (!dVscp->isWide()) प्रत्यागम;
        Ast_गाँठ_Assign* ध्रुव prep = VN_AS(writes[0].nodep()->backp(), गाँठ_Assign);
        UASSERT_OBJ(VN_AS(prep->lhsp(), VarRef)->varScopep()
                            == VN_AS(postp->rhsp(), VarRef)->varScopep()
                        && VN_AS(prep->rhsp(), VarRef)->varScopep()
                               == VN_AS(postp->lhsp(), VarRef)->varScopep(),
                    prep, "NBA shadow pre/post assignments are not reverse copies");

        पूर्णाङ्क lsb = dVscp->width();
        पूर्णाङ्क end = 0;
        प्रति (माप_प्रकार i = 1; i < writes.size(); ++i) {
            ध्रुव AstSel* ध्रुव selp = VN_CAST(writes[i].nodep()->backp(), Sel);
            यदि (!selp || !VN_IS(selp->lsbp(), Const)) प्रत्यागम;
            ध्रुव पूर्णाङ्क start = selp->lsbConst();
            यदि (start > dVscp->width() - selp->width()) प्रत्यागम;
            lsb = std::min(lsb, start);
            end = std::max(end, start + selp->width());
        }
        // Copy one word-aligned range enclosing all writes, including any gaps.
        lsb = VL_BITWORD_E(lsb) * VL_EDATASIZE;
        end = std::min(VL_शब्दगण_I(end) * VL_EDATASIZE, dVscp->width());
        ध्रुव पूर्णाङ्क width = end - lsb;
        यदि (width == dVscp->width()) प्रत्यागम;
        प्रति (Ast_गाँठ_Assign* ध्रुव assignp : {prep, postp}) {
            File_पंक्ति* ध्रुव flp = assignp->fileline();
            assignp->lhsp(नूतन AstSel{flp, assignp->lhsp()->unlinkFrBack(), lsb, width});
            assignp->rhsp(नूतन AstSel{flp, assignp->rhsp()->unlinkFrBack(), lsb, width});
            assignp->dtypeFrom(assignp->lhsp());
        }
        m_stat_शब्दगण_Saved += 2 * (dVscp->width_शब्दगण() - VL_शब्दगण_I(width));
    }

    शून्य squashAssignposts() {
        प्रति (ध्रुव Location<Ast_गाँठ_Assign>& assign : m_assigns) {
            AstVarScope* ध्रुव dVscp = VN_AS(assign.nodep()->rhsp(), VarRef)->varScopep();
            AstVarScope* ध्रुव qVscp = VN_AS(assign.nodep()->lhsp(), VarRef)->varScopep();

            // We are considering deleting 'y', don't do it if referenced external to _eval__nba
            यदि (dVscp->user1()) अनुवर्त;

            ध्रुव std::vector<Location<AstVarRef>>& dWrites = m_writes[dVscp];
            UASSERT_OBJ(!dWrites.empty(), dVscp, "NBA shadow variable read but never written");

            // *** See file header for requirements ***

            // Proof (1) - Only read is on the RHS of this assignment
            यदि (m_reads[dVscp].size() > 1) अनुवर्त;

            // Proof (2) - Only write is on the LHS of this assignment
            यदि (m_writes[qVscp].size() > 1) अनुवर्त;

            // Proof (3) - Should always hold
            UASSERT_OBJ(VN_IS(dWrites.at(0).nodep()->backp(), गाँठ_Assign), dVscp,
                        "Partial first write to NBA shadow variable");

            // Proof (4)
            यदि (dWrites.size() > 1) {
                // V3Order always serializes writes so they cannot be concurrent
                UASSERT_OBJ(dWrites[0] < dWrites[1], dVscp, "Concurrent writes");
                ध्रुव तार्किक qRdOK = [&]() {
                    प्रति (ध्रुव Location<AstVarRef>& qRead : m_reads[qVscp]) {
                        यदि (assign < qRead) अनुवर्त;
                        // Check from 2nd write of 'd'
                        प्रति (माप_प्रकार i = 1; i < dWrites.size(); ++i) {
                            यदि (qRead < dWrites[i]) अनुवर्त;
                            प्रत्यागम असत्य;
                        }
                    }
                    प्रत्यागम सत्य;
                }();
                यदि (!qRdOK) {
                    narrowCopies(assign.nodep(), dVscp, dWrites);
                    अनुवर्त;
                }
            }

            // Mark variable for replacement
            dVscp->user4p(qVscp);
            // Delete assignment
            UINFO(4, "    DELETE " << assign.nodep());
            VL_DO_DANGLING(assign.nodep()->unlinkFrBack()->delete_वृक्ष(), assign.nodep());
            ++m_statAssnDel;
        }
    }

    // Trace code in the given function
    शून्य trace(AstCFunc* nodep) {
        VL_RESTORER(m_inEvalNba);
        यदि (nodep == m_nbaFuncp) m_inEvalNba = सत्य;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    शून्य visit(AstNetlist* nodep) override {
        // First, build maps of every location (mtask and sequence
        // within the mtask) where each varscope is read, and written.
        iterateChildrenConst(nodep);

        // We need to be able to pick up the first write of each variable.
        // V3Order serializes all writes, and we trace AstExecGraph in
        // dependency order, so the first one we encounter during tracing should
        // always be the one. It's somewhat expensive to assert so only with debugCheck().
        यदि (v3Global.opt.debugCheck()) {
            प्रति (स्वतः& pair : m_writes) {
                ध्रुव std::vector<Location<AstVarRef>>& writes = pair.second;
                ध्रुव Location<AstVarRef>& first = writes[0];
                प्रति (माप_प्रकार i = 1; i < writes.size(); ++i) {
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
            ध्रुव AstVarScope* ध्रुव vscp = nodep->varScopep();
            AstVarScope* ध्रुव replacementp = VN_AS(vscp->user4p(), VarScope);
            यदि (!replacementp) प्रत्यागम;
            UINFO(9, "  Replace " << nodep << " target " << vscp << " with " << replacementp);
            nodep->varScopep(replacementp);
            nodep->varp(replacementp->varp());
        });
    }

    शून्य visit(AstVarRef* nodep) override {
        // We only try to optimize NBA shadow variables
        यदि (!nodep->varScopep()->optimizeLifePost()) प्रत्यागम;

        // Mark variables referenced outside the 'nba' region
        यदि (!m_inEvalNba) {
            nodep->varScopep()->user1(सत्य);
            प्रत्यागम;
        }

        // Consumption/generation of a variable,
        ध्रुव AstVarScope* ध्रुव vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");

        ++m_sequence;
        यदि (nodep->access().isWriteOrRW()) {
            m_writes[vscp].emplace_back(nodep, m_execGraphp, m_execMTaskp, m_sequence);
        }
        यदि (nodep->access().isReadOrRW()) {
            m_reads[vscp].emplace_back(nodep, m_execGraphp, m_execMTaskp, m_sequence);
        }
    }
    शून्य visit(Ast_गाँठ_Assign* nodep) override {
        // Record RHS before assignment
        iterateConst(nodep->rhsp());
        // If a straight assignment between NBA variables, consider for removal
        यदि (ध्रुव AstVarRef* ध्रुव lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            यदि (ध्रुव AstVarRef* ध्रुव rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                यदि (lhsp->varScopep()->optimizeLifePost()  //
                    && rhsp->varScopep()->optimizeLifePost()) {
                    m_assigns.emplace_back(nodep, m_execGraphp, m_execMTaskp, ++m_sequence);
                }
            }
        }
        // Record LHS after assignment
        iterateConst(nodep->lhsp());
    }
    शून्य visit(Ast_गाँठ_CCall* nodep) override {
        iterateChildrenConst(nodep);
        // Entry points are roots of the trace, no need to do it here
        यदि (nodep->funcp()->entryPoint()) प्रत्यागम;
        // Trace cellee
        trace(nodep->funcp());
    }
    शून्य visit(AstExecGraph* nodep) override {
        UASSERT_OBJ(!m_execGraphp, nodep, "Nested AstExecGraph");
        VL_RESTORER(m_execGraphp);
        m_execGraphp = nodep;

        // Set up the path checker for this graph
        UASSERT_OBJ(!nodep->user1p(), nodep, "AstExecGraph visited twice");
        m_checkers.emplace_back(नूतन GraphPathChecker{nodep->depGraphp()});
        nodep->user1p(m_checkers.back().get());

        // Trace each mtask body. Note: the vertices are in topological order,
        // and we do not reset m_sequence, so a lower sequence number does
        // guarantee a node is not earlier than a higher sequence number, but
        // might still be concurrent.
        प्रति (V3GraphVertex& mtaskVtx : nodep->depGraphp()->vertices()) {
            ध्रुव ExecMTask* ध्रुव mtaskp = mtaskVtx.as<ExecMTask>();
            VL_RESTORER(m_execMTaskp);
            m_execMTaskp = mtaskp;
            trace(mtaskp->funcp());
        }
    }
    शून्य visit(AstCFunc* nodep) override {
        // Start a trace from each entry point
        यदि (nodep->entryPoint()) trace(nodep);
    }
    //-----
    शून्य visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit LifePostDlyVisitor(AstNetlist* netlistp)
        : m_nbaFuncp{netlistp->evalFuncp(VEval::NBA)} {
        iterateConst(netlistp);
    }
    ~LifePostDlyVisitor() override {
        V3Stats::addStat("Optimizations, Lifetime postassign deletions", m_statAssnDel);
        V3Stats::addStat("Optimizations, Lifetime NBA copy words removed", m_stat_शब्दगण_Saved);
    }
};

//######################################################################
// LifePost class functions

शून्य V3LifePost::lifepostAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LifePostDlyVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("life_post", 0, dump_वृक्ष_Either_Level() >= 3);
}
