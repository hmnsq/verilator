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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3LifePost.h"

#ضمن "V3ExecGraph.h"
#ضمن "V3GraphPathChecker.h"
#ضمن "V3Stats.h"

#ضمن <algorithm>
#ضمن <memory>
#ضمن <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LifePost delay elimination

صنف LifePostDlyVisitor final : عام VNVisitorConst {
    // TYPES

    // Location of AstNode within the program
    قالب <نام_نوع T_گره>
    صنف Location final {
        قالب <نام_نوع U_گره>
        دوست صنف Location;

        T_گره* m_nodep;  // The AstNode being recorded
        ثابت Ast_Exec_گراف* m_egraphp;  // AstExecTraph location is under, if any
        ثابت ExecMTask* m_mtaskp;  // The ExecMTask location is under, if any
        uint32_t m_seq_رقم;  // Location counter

    عام:
        Location(T_گره* nodep, ثابت Ast_Exec_گراف* egp, ثابت ExecMTask* mtaskp, uint32_t seq_رقم)
            : m_nodep{nodep}
            , m_egraphp{egp}
            , m_mtaskp{mtaskp}
            , m_seq_رقم{seq_رقم} {}
        Location() = حذف;

        T_گره* ثابت& nodep() ثابت { أرجع m_nodep; }

        // "is before" - Note: Equality (concurrency) is possible iff they are independent mtasks!
        قالب <نام_نوع U_گره>
        منطقي عملگر<(ثابت Location<U_گره>& that) ثابت {
            // If they are in different mtasks under the same graph, check for a path in the graph
            إذا (m_egraphp && m_egraphp == that.m_egraphp && m_mtaskp != that.m_mtaskp) {
                گراف_مسیر_Checker* ثابت checkerp = m_egraphp->user1u().مقصد<گراف_مسیر_Checker*>();
                أرجع checkerp->مسیر_Exists_From(m_mtaskp, that.m_mtaskp);
            }
            // Otherwise the sequence numbers work (one/both outside graph, or both in same mtask)
            أرجع m_seq_رقم < that.m_seq_رقم;
        }
    };

    // NODE STATE
    // AstVarScope::user1()    -> bool: referenced outside _eval__nba
    // AstVarScope::user4p()   -> AstVarScope*: Replacement variable
    // AstExecGraph::user1p()  -> GraphPathChecker*: path checker for this AstExecGraph
    ثابت VNUser1_درون_Use m_inuser1;
    ثابت VNUser4_درون_Use m_inuser4;

    // STATE
    uint32_t m_sequence = 0;  // Sequence number of assigns/varrefs,
    ثابت Ast_Exec_گراف* m_execGraphp = اشاره_تهی;  // Current AstExecGraph being processed (or null)
    ثابت ExecMTask* m_execMTaskp = اشاره_تهی;  // Current ExecMTask being processed (or null)
    VDouble0 m_statAssnDel;  // Statistic tracking
    VDouble0 m_stat_واژگان_Saved;  // Words removed from NBA shadow copies
    // Maps from Varscope to all their reads and writes
    استفاده LocMap = std::unordered_map<ثابت AstVarScope*, std::وکتور<Location<AstVarRef>>>;
    LocMap m_reads;  // VarScope read locations
    LocMap m_writes;  // VarScope write locations
    std::وکتور<Location<Ast_گره_تخصیص_بده>> m_assigns;  // Assignments considered for removal
    std::وکتور<std::unique_ptr<گراف_مسیر_Checker>> m_checkers;  // Storage for exec graph checkers
    ثابت AstCFunc* ثابت m_nbaFuncp;  // The 'nba' region entry point
    منطقي m_درون_Eval_Nba = خطأ;  // Traversing under the 'nba' region entry point

    // METHODS
    فراغ narrowCopies(Ast_گره_تخصیص_بده* postp, AstVarScope* dVscp,
                      ثابت std::وکتور<Location<AstVarRef>>& writes) {
        إذا (!dVscp->isWide()) أرجع;
        Ast_گره_تخصیص_بده* ثابت prep = VN_AS(writes[0].nodep()->backp(), گره_تخصیص_بده);
        UASSERT_OBJ(VN_AS(prep->lhsp(), VarRef)->varScopep()
                            == VN_AS(postp->rhsp(), VarRef)->varScopep()
                        && VN_AS(prep->rhsp(), VarRef)->varScopep()
                               == VN_AS(postp->lhsp(), VarRef)->varScopep(),
                    prep, "NBA shadow pre/post assignments are not reverse copies");

        صحيح lsb = dVscp->عرض();
        صحيح انتها = 0;
        لكل (اندازه_t i = 1; i < writes.اندازه(); ++i) {
            ثابت AstSel* ثابت selp = VN_CAST(writes[i].nodep()->backp(), Sel);
            إذا (!selp || !VN_IS(selp->lsbp(), Const)) أرجع;
            ثابت صحيح شروع = selp->lsbConst();
            إذا (شروع > dVscp->عرض() - selp->عرض()) أرجع;
            lsb = std::min(lsb, شروع);
            انتها = std::max(انتها, شروع + selp->عرض());
        }
        // Copy one word-aligned range enclosing all writes, including any gaps.
        lsb = VL_BITWORD_E(lsb) * VL_EDATASIZE;
        انتها = std::min(VL_واژگان_I(انتها) * VL_EDATASIZE, dVscp->عرض());
        ثابت صحيح عرض = انتها - lsb;
        إذا (عرض == dVscp->عرض()) أرجع;
        لكل (Ast_گره_تخصیص_بده* ثابت assignp : {prep, postp}) {
            File_سطر* ثابت flp = assignp->fileline();
            assignp->lhsp(جديد AstSel{flp, assignp->lhsp()->unlink_Fr_پشت(), lsb, عرض});
            assignp->rhsp(جديد AstSel{flp, assignp->rhsp()->unlink_Fr_پشت(), lsb, عرض});
            assignp->dtypeFrom(assignp->lhsp());
        }
        m_stat_واژگان_Saved += 2 * (dVscp->عرض_واژگان() - VL_واژگان_I(عرض));
    }

    فراغ squashAssignposts() {
        لكل (ثابت Location<Ast_گره_تخصیص_بده>& تخصیص_بده_2 : m_assigns) {
            AstVarScope* ثابت dVscp = VN_AS(تخصیص_بده_2.nodep()->rhsp(), VarRef)->varScopep();
            AstVarScope* ثابت qVscp = VN_AS(تخصیص_بده_2.nodep()->lhsp(), VarRef)->varScopep();

            // We are considering deleting 'y', don't do it if referenced external to _eval__nba
            إذا (dVscp->user1()) استمر;

            ثابت std::وکتور<Location<AstVarRef>>& dWrites = m_writes[dVscp];
            UASSERT_OBJ(!dWrites.خالی(), dVscp, "NBA shadow variable read but never written");

            // *** See file header for requirements ***

            // Proof (1) - Only read is on the RHS of this assignment
            إذا (m_reads[dVscp].اندازه() > 1) استمر;

            // Proof (2) - Only write is on the LHS of this assignment
            إذا (m_writes[qVscp].اندازه() > 1) استمر;

            // Proof (3) - Should always hold
            UASSERT_OBJ(VN_IS(dWrites.at(0).nodep()->backp(), گره_تخصیص_بده), dVscp,
                        "Partial first write to NBA shadow variable");

            // Proof (4)
            إذا (dWrites.اندازه() > 1) {
                // V3Order always serializes writes so they cannot be concurrent
                UASSERT_OBJ(dWrites[0] < dWrites[1], dVscp, "Concurrent writes");
                ثابت منطقي qRdOK = [&]() {
                    لكل (ثابت Location<AstVarRef>& qRead : m_reads[qVscp]) {
                        إذا (تخصیص_بده_2 < qRead) استمر;
                        // Check from 2nd write of 'd'
                        لكل (اندازه_t i = 1; i < dWrites.اندازه(); ++i) {
                            إذا (qRead < dWrites[i]) استمر;
                            أرجع خطأ;
                        }
                    }
                    أرجع درست;
                }();
                إذا (!qRdOK) {
                    narrowCopies(تخصیص_بده_2.nodep(), dVscp, dWrites);
                    استمر;
                }
            }

            // Mark variable for replacement
            dVscp->user4p(qVscp);
            // Delete assignment
            UINFO(4, "    DELETE " << تخصیص_بده_2.nodep());
            VL_DO_DANGLING(تخصیص_بده_2.nodep()->unlink_Fr_پشت()->delete_درخت(), تخصیص_بده_2.nodep());
            ++m_statAssnDel;
        }
    }

    // Trace code in the given function
    فراغ trace(AstCFunc* nodep) {
        VL_RESTORER(m_درون_Eval_Nba);
        إذا (nodep == m_nbaFuncp) m_درون_Eval_Nba = درست;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    فراغ بازدید(AstNetlist* nodep) override {
        // First, build maps of every location (mtask and sequence
        // within the mtask) where each varscope is read, and written.
        iterateChildrenConst(nodep);

        // We need to be able to pick up the first write of each variable.
        // V3Order serializes all writes, and we trace AstExecGraph in
        // dependency order, so the first one we encounter during tracing should
        // always be the one. It's somewhat expensive to assert so only with debugCheck().
        إذا (v3Global.opt.debugCheck()) {
            لكل (خودکار& pair : m_writes) {
                ثابت std::وکتور<Location<AstVarRef>>& writes = pair.دومی;
                ثابت Location<AstVarRef>& اولی = writes[0];
                لكل (اندازه_t i = 1; i < writes.اندازه(); ++i) {
                    UASSERT_OBJ(اولی < writes[i], pair.اولی, "First write is not the first");
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
            ثابت AstVarScope* ثابت vscp = nodep->varScopep();
            AstVarScope* ثابت replacementp = VN_AS(vscp->user4p(), VarScope);
            إذا (!replacementp) أرجع;
            UINFO(9, "  Replace " << nodep << " target " << vscp << " with " << replacementp);
            nodep->varScopep(replacementp);
            nodep->varp(replacementp->varp());
        });
    }

    فراغ بازدید(AstVarRef* nodep) override {
        // We only try to optimize NBA shadow variables
        إذا (!nodep->varScopep()->optimizeLifePost()) أرجع;

        // Mark variables referenced outside the 'nba' region
        إذا (!m_درون_Eval_Nba) {
            nodep->varScopep()->user1(درست);
            أرجع;
        }

        // Consumption/generation of a variable,
        ثابت AstVarScope* ثابت vscp = nodep->varScopep();
        UASSERT_OBJ(vscp, nodep, "Scope not assigned");

        ++m_sequence;
        إذا (nodep->access().isWriteOrRW()) {
            m_writes[vscp].emplace_پشت(nodep, m_execGraphp, m_execMTaskp, m_sequence);
        }
        إذا (nodep->access().isReadOrRW()) {
            m_reads[vscp].emplace_پشت(nodep, m_execGraphp, m_execMTaskp, m_sequence);
        }
    }
    فراغ بازدید(Ast_گره_تخصیص_بده* nodep) override {
        // Record RHS before assignment
        iterateConst(nodep->rhsp());
        // If a straight assignment between NBA variables, consider for removal
        إذا (ثابت AstVarRef* ثابت lhsp = VN_CAST(nodep->lhsp(), VarRef)) {
            إذا (ثابت AstVarRef* ثابت rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                إذا (lhsp->varScopep()->optimizeLifePost()  //
                    && rhsp->varScopep()->optimizeLifePost()) {
                    m_assigns.emplace_پشت(nodep, m_execGraphp, m_execMTaskp, ++m_sequence);
                }
            }
        }
        // Record LHS after assignment
        iterateConst(nodep->lhsp());
    }
    فراغ بازدید(Ast_گره_CCall* nodep) override {
        iterateChildrenConst(nodep);
        // Entry points are roots of the trace, no need to do it here
        إذا (nodep->funcp()->مدخل_نقطه()) أرجع;
        // Trace cellee
        trace(nodep->funcp());
    }
    فراغ بازدید(Ast_Exec_گراف* nodep) override {
        UASSERT_OBJ(!m_execGraphp, nodep, "Nested AstExecGraph");
        VL_RESTORER(m_execGraphp);
        m_execGraphp = nodep;

        // Set up the path checker for this graph
        UASSERT_OBJ(!nodep->user1p(), nodep, "AstExecGraph visited twice");
        m_checkers.emplace_پشت(جديد گراف_مسیر_Checker{nodep->depGraphp()});
        nodep->user1p(m_checkers.پشت().get());

        // Trace each mtask body. Note: the vertices are in topological order,
        // and we do not reset m_sequence, so a lower sequence number does
        // guarantee a node is not earlier than a higher sequence number, but
        // might still be concurrent.
        لكل (V3_گراف_رأس& mtaskVtx : nodep->depGraphp()->vertices()) {
            ثابت ExecMTask* ثابت mtaskp = mtaskVtx.as<ExecMTask>();
            VL_RESTORER(m_execMTaskp);
            m_execMTaskp = mtaskp;
            trace(mtaskp->funcp());
        }
    }
    فراغ بازدید(AstCFunc* nodep) override {
        // Start a trace from each entry point
        إذا (nodep->مدخل_نقطه()) trace(nodep);
    }
    //-----
    فراغ بازدید(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    صریح LifePostDlyVisitor(AstNetlist* netlistp)
        : m_nbaFuncp{netlistp->evalFuncp(VEval::NBA)} {
        iterateConst(netlistp);
    }
    ~LifePostDlyVisitor() override {
        V3Stats::جمع_کن_Stat("Optimizations, Lifetime postassign deletions", m_statAssnDel);
        V3Stats::جمع_کن_Stat("Optimizations, Lifetime NBA copy words removed", m_stat_واژگان_Saved);
    }
};

//######################################################################
// LifePost class functions

فراغ V3LifePost::lifepostAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LifePostDlyVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("life_post", 0, dump_درخت_Either_Level() >= 3);
}
