// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Estimate instruction count to run the logic
//                         we would generate for any given AST subtree.
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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3InstrCount.h"

#ضمن <iomanip>
#ضمن <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the instruction cost for executing all logic within and below
/// a given AST node. Note this estimates the number of instructions we'll
/// execute, not the number we'll generate. That is, for conditionals,
/// we'll count instructions from either the 'if' or the 'else' branch,
/// whichever is larger. We know we won't run both.

صنف Instr_شمېر_Visitor final : عام VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()        -> bool. Processed if assertNoDups
    //  AstNode::user2()        -> uint64_t.  Path cost + 1, 0 means don't dump
    ثابت VNUser2_دننه_Use m_inuser2;

    // MEMBERS
    uint32_t m_instr_شمېر = 0;  // Running count of instructions
    ثابت Ast_غوټه* ثابت m_پیل_Nodep;  // Start node of count
    منطقي m_tracingCall = خطأ;  // Iterating into a CCall to a CFunc
    منطقي m_دننه_CFunc = خطأ;  // Inside AstCFunc
    منطقي m_ignore_پاتې_شوی = خطأ;  // Ignore remaining statements in the block
    ثابت منطقي m_assertNoDups;  // Check for duplicates
    ثابت std::ostream* ثابت m_osp;  // Dump file
    std::unordered_map<ثابت AstCFunc*, uint32_t> m_funcCosts;  // Cost of each function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    صنف لیدنه_بنسټ final {
        // MEMBERS
        uint32_t m_saved_شمېر;  // Count before visit applied
        Ast_غوټه* ثابت m_nodep;  // Node helper is visiting
        Instr_شمېر_Visitor* ثابت m_visitor;  // Visitor whose start/endVisitBase are called

    عام:
        // CONSTRUCTORS
        لیدنه_بنسټ(Instr_شمېر_Visitor* visitor, Ast_غوټه* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_شمېر = m_visitor->پیل_لیدنه_بنسټ(nodep);
        }
        ~لیدنه_بنسټ() { m_visitor->پای_ټکی_لیدنه_بنسټ(m_saved_شمېر, m_nodep); }

    خاص:
        VL_UNCOPYABLE(لیدنه_بنسټ);
    };

عام:
    // CONSTRUCTORS
    Instr_شمېر_Visitor(Ast_غوټه* nodep, منطقي assertNoDups, std::ostream* osp)
        : m_پیل_Nodep{nodep}
        , m_assertNoDups{assertNoDups}
        , m_osp{osp} {
        iterateConstNull(nodep);
    }
    ~Instr_شمېر_Visitor() override = افتراضي;

    // METHODS
    uint32_t instr_شمېر() ثابت { أرجع m_instr_شمېر; }

خاص:
    فراغ reset() {
        m_instr_شمېر = 0;
        m_ignore_پاتې_شوی = خطأ;
    }
    uint32_t پیل_لیدنه_بنسټ(Ast_غوټه* nodep) {
        UASSERT_OBJ(!m_ignore_پاتې_شوی, nodep, "Should not reach here if ignoring");
        إذا (m_assertNoDups && !m_دننه_CFunc) {
            // Ensure we don't count the same node twice
            //
            // We only enable this assert for the initial LogicMTask counts
            // in V3Order. We can't enable it for the 2nd pass in V3EmitC,
            // as we expect mtasks to contain common logic after V3Combine,
            // so this would fail.
            //
            // Also, we expect some collisions within calls to CFuncs
            // (which at the V3Order stage represent verilog tasks, not to
            // the CFuncs that V3Order will generate.) So don't check for
            // collisions in CFuncs.
            UASSERT_OBJ(!nodep->user1p(), nodep,
                        "Node originally inserted below logic vertex "
                            << static_cast<Ast_غوټه*>(nodep->user1p()));
            nodep->user1p(const_cast<فراغ*>(reinterpret_cast<ثابت فراغ*>(m_پیل_Nodep)));
        }

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ثابت uint32_t saved_شمېر = m_instr_شمېر;
        m_instr_شمېر = nodep->instr_شمېر();
        أرجع saved_شمېر;
    }
    فراغ پای_ټکی_لیدنه_بنسټ(uint32_t saved_شمېر, Ast_غوټه* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::کیڼ << m_instr_شمېر << "  " << nodep);
        mark_لګښت(nodep);
        إذا (!m_ignore_پاتې_شوی) m_instr_شمېر += saved_شمېر;
    }
    فراغ mark_لګښت(Ast_غوټه* nodep) {
        إذا (m_osp) nodep->user2(m_instr_شمېر + 1);  // Else don't mark to avoid writeback
    }

    // VISITORS
    فراغ لیدنه(Ast_غوټه_Sel* nodep) override {
        إذا (m_ignore_پاتې_شوی) أرجع;
        // This covers both AstArraySel and AstWordSel
        //
        // If some vector is a bazillion dwords long, and we're selecting 1
        // dword to read or write from it, our cost should be small.
        //
        // Hence, exclude the child of the AstWordSel from the computation,
        // whose cost scales with the size of the entire (maybe large) vector.
        ثابت لیدنه_بنسټ vb{هذا, nodep};
        iterate_And_بل_Const_Null(nodep->bitp());
    }
    فراغ لیدنه(AstSel* nodep) override {
        إذا (m_ignore_پاتې_شوی) أرجع;
        // Similar to AstNodeSel above, a small select into a large vector
        // is not expensive. Count the cost of the AstSel itself (scales with
        // its width) and the cost of the lsbp() and widthp() nodes, but not
        // the fromp() node which could be disproportionately large.
        ثابت لیدنه_بنسټ vb{هذا, nodep};
        iterate_And_بل_Const_Null(nodep->lsbp());
    }
    فراغ لیدنه(AstConcat* nodep) override {
        إذا (m_ignore_پاتې_شوی) أرجع;
        // Ignore the cost of the concat node itself. The problem with
        // counting concat is that when we have many things concatted
        // together, it's not a single operation, but this:
        //
        //  concat(a, concat(b, concat(c, concat(d, ... ))))
        //
        // Then if we account a cost to each 'concat' that scales with its
        // width, this whole operation ends up with a cost accounting that
        // scales with N^2. Of course, the real operation isn't that
        // expensive: we won't copy each element over and over, we'll just
        // copy it once from its origin into its destination, so the actual
        // cost is linear with the size of the data. We don't need to count
        // the concat at all to reflect a linear cost; it's already there
        // in the width of the destination (which we count) and the sum of
        // the cost of the operands.
        iterateChildrenConst(nodep);
        mark_لګښت(nodep);
    }
    فراغ لیدنه(Ast_غوټه_If* nodep) override {
        إذا (m_ignore_پاتې_شوی) أرجع;
        ثابت لیدنه_بنسټ vb{هذا, nodep};
        iterate_And_بل_Const_Null(nodep->condp());
        ثابت uint32_t saved_شمېر = m_instr_شمېر;

        UINFO(8, "thensp:");
        reset();
        iterate_And_بل_Const_Null(nodep->thensp());
        uint32_t if_شمېر = m_instr_شمېر;
        إذا (nodep->branchPred().unlikely()) if_شمېر = 0;

        UINFO(8, "elsesp:");
        reset();
        iterate_And_بل_Const_Null(nodep->elsesp());
        uint32_t else_شمېر = m_instr_شمېر;
        إذا (nodep->branchPred().likely()) else_شمېر = 0;

        reset();
        إذا (if_شمېر >= else_شمېر) {
            m_instr_شمېر = saved_شمېر + if_شمېر;
            إذا (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } وإلا {
            m_instr_شمېر = saved_شمېر + else_شمېر;
            إذا (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    فراغ لیدنه(AstCond* nodep) override {
        إذا (m_ignore_پاتې_شوی) أرجع;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ثابت لیدنه_بنسټ vb{هذا, nodep};
        iterate_And_بل_Const_Null(nodep->condp());
        ثابت uint32_t saved_شمېر = m_instr_شمېر;

        UINFO(8, "?");
        reset();
        iterate_And_بل_Const_Null(nodep->thenp());
        ثابت uint32_t if_شمېر = m_instr_شمېر;

        UINFO(8, ":");
        reset();
        iterate_And_بل_Const_Null(nodep->elsep());
        ثابت uint32_t else_شمېر = m_instr_شمېر;

        reset();
        إذا (if_شمېر >= else_شمېر) {
            m_instr_شمېر = saved_شمېر + if_شمېر;
            إذا (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } وإلا {
            m_instr_شمېر = saved_شمېر + else_شمېر;
            إذا (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    فراغ لیدنه(AstCAwait* nodep) override {
        إذا (m_ignore_پاتې_شوی) أرجع;
        iterateChildrenConst(nodep);
        // Anything past a co_await is irrelevant
        m_ignore_پاتې_شوی = ریښتیا;
    }
    فراغ لیدنه(AstFork* nodep) override {
        إذا (m_ignore_پاتې_شوی) أرجع;
        ثابت لیدنه_بنسټ vb{هذا, nodep};
        iterate_And_بل_Const_Null(nodep->stmtsp());
        uint32_t ټول_شمېر = m_instr_شمېر;
        // Sum counts in each statement until the first await
        لكل (Ast_غوټه* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            reset();
            iterateConst(stmtp);
            ټول_شمېر += m_instr_شمېر;
        }
        m_instr_شمېر = ټول_شمېر;
        m_ignore_پاتې_شوی = خطأ;
    }
    فراغ لیدنه(Ast_فعال* nodep) override {
        // You'd think that the OrderLogicVertex's would be disjoint trees
        // of stuff in the AST, but it isn't so: V3Order makes an
        // OrderLogicVertex for each ACTIVE, and then also makes an
        // OrderLogicVertex for each statement within the ACTIVE.
        //
        // To avoid double-counting costs, stop recursing and short-circuit
        // the computation for each ACTIVE.
        //
        // Our intent is that this only stops at the root node of the
        // search; there should be no actives beneath the root, as there
        // are no actives-under-actives.  In any case, check that we're at
        // root:
        mark_لګښت(nodep);
        UASSERT_OBJ(nodep == m_پیل_Nodep, nodep, "Multiple actives, or not start node");
    }
    فراغ لیدنه(Ast_غوټه_CCall* nodep) override {
        إذا (m_ignore_پاتې_شوی) أرجع;
        ثابت لیدنه_بنسټ vb{هذا, nodep};
        iterateChildrenConst(nodep);
        ثابت خپلکاری it = m_funcCosts.پیدا_کړه(nodep->funcp());
        إذا (it == m_funcCosts.پای_ټکی()) {
            m_tracingCall = ریښتیا;
            iterateConst(nodep->funcp());
            UASSERT_OBJ(!m_tracingCall, nodep,
                        "visit(AstCFunc) should have cleared m_tracingCall.");
        } وإلا {
            m_instr_شمېر += it->دوهم;  // This guards and adds zero cost on recursive calls
        }
    }
    فراغ لیدنه(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        UASSERT_OBJ(m_tracingCall || nodep == m_پیل_Nodep, nodep,
                    "AstCFunc not under AstCCall, or not start node");
        UASSERT_OBJ(!m_ignore_پاتې_شوی, nodep, "Should not be ignoring at the start of a CFunc");
        ثابت خپلکاری emplacePair = m_funcCosts.emplace(nodep, 0);
        UASSERT_OBJ(emplacePair.دوهم, nodep, "Should visit every CFunc at most once");
        uint32_t& funcCostr = emplacePair.لومړی->دوهم;
        m_tracingCall = خطأ;

        ثابت uint32_t saved_شمېر = m_instr_شمېر;
        reset();
        {
            VL_RESTORER(m_دننه_CFunc);
            m_دننه_CFunc = ریښتیا;
            ثابت لیدنه_بنسټ vb{هذا, nodep};
            iterateChildrenConst(nodep);
        }
        funcCostr = m_instr_شمېر;
        m_instr_شمېر += saved_شمېر;

        m_ignore_پاتې_شوی = خطأ;
    }
    فراغ لیدنه(Ast_غوټه_Cons* nodep) override {
        إذا (m_ignore_پاتې_شوی) أرجع;
        ثابت لیدنه_بنسټ vb{هذا, nodep};
        // A constant value is not built at run time, so its parts cost nothing
        إذا (!nodep->is_Constant_ونه()) iterateChildrenConst(nodep);
    }
    فراغ لیدنه(Ast_غوټه* nodep) override {
        إذا (m_ignore_پاتې_شوی) أرجع;
        ثابت لیدنه_بنسټ vb{هذا, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(Instr_شمېر_Visitor);
};

// Iterate the graph printing the critical path marked by previous visitation
صنف Instr_شمېر_Dump_Visitor final : عام VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> int.  Path cost, 0 means don't dump

    // MEMBERS
    std::ostream* ثابت m_osp;  // Dump file
    بینښه m_ژوروالی = 0;  // Current tree depth for printing indent

عام:
    // CONSTRUCTORS
    Instr_شمېر_Dump_Visitor(Ast_غوټه* nodep, std::ostream* osp)
        : m_osp{osp} {
        // No check for nullptr output, so...
        UASSERT_OBJ(osp, nodep, "Don't call if not dumping");
        إذا (nodep) iterateConst(nodep);
    }
    ~Instr_شمېر_Dump_Visitor() override = افتراضي;

خاص:
    // METHODS
    string indent() ثابت { أرجع string(m_ژوروالی, ':') + " "; }
    فراغ لیدنه(Ast_غوټه* nodep) override {
        ++m_ژوروالی;
        إذا (ثابت بینښه لګښت_Plus1 = nodep->user2()) {
            *m_osp << "  " << indent() << "cost " << std::setw(6) << std::کیڼ << (لګښت_Plus1 - 1)
                   << "  " << nodep << '\n';
            iterateChildrenConst(nodep);
        }
        --m_ژوروالی;
    }

    VL_UNCOPYABLE(Instr_شمېر_Dump_Visitor);
};

uint32_t V3_Instr_شمېر::شمېر(Ast_غوټه* nodep, منطقي assertNoDups, std::ostream* osp) {
    ثابت Instr_شمېر_Visitor visitor{nodep, assertNoDups, osp};
    إذا (osp) ثابت Instr_شمېر_Dump_Visitor dumper{nodep, osp};
    أرجع visitor.instr_شمېر();
}
