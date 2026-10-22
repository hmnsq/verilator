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

صنف Instr_شمار_Visitor final : عام VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()        -> bool. Processed if assertNoDups
    //  AstNode::user2()        -> uint64_t.  Path cost + 1, 0 means don't dump
    ثابت VNUser2_درون_Use m_inuser2;

    // MEMBERS
    uint32_t m_instr_شمار = 0;  // Running count of instructions
    ثابت Ast_گره* ثابت m_شروع_Nodep;  // Start node of count
    منطقي m_tracingCall = خطأ;  // Iterating into a CCall to a CFunc
    منطقي m_درون_CFunc = خطأ;  // Inside AstCFunc
    منطقي m_ignore_مانده = خطأ;  // Ignore remaining statements in the block
    ثابت منطقي m_assertNoDups;  // Check for duplicates
    ثابت std::ostream* ثابت m_osp;  // Dump file
    std::unordered_map<ثابت AstCFunc*, uint32_t> m_funcCosts;  // Cost of each function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    صنف بازدید_پایه final {
        // MEMBERS
        uint32_t m_saved_شمار;  // Count before visit applied
        Ast_گره* ثابت m_nodep;  // Node helper is visiting
        Instr_شمار_Visitor* ثابت m_visitor;  // Visitor whose start/endVisitBase are called

    عام:
        // CONSTRUCTORS
        بازدید_پایه(Instr_شمار_Visitor* visitor, Ast_گره* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_شمار = m_visitor->شروع_بازدید_پایه(nodep);
        }
        ~بازدید_پایه() { m_visitor->انتها_بازدید_پایه(m_saved_شمار, m_nodep); }

    خاص:
        VL_UNCOPYABLE(بازدید_پایه);
    };

عام:
    // CONSTRUCTORS
    Instr_شمار_Visitor(Ast_گره* nodep, منطقي assertNoDups, std::ostream* osp)
        : m_شروع_Nodep{nodep}
        , m_assertNoDups{assertNoDups}
        , m_osp{osp} {
        iterateConstNull(nodep);
    }
    ~Instr_شمار_Visitor() override = افتراضي;

    // METHODS
    uint32_t instr_شمار() ثابت { أرجع m_instr_شمار; }

خاص:
    فراغ reset() {
        m_instr_شمار = 0;
        m_ignore_مانده = خطأ;
    }
    uint32_t شروع_بازدید_پایه(Ast_گره* nodep) {
        UASSERT_OBJ(!m_ignore_مانده, nodep, "Should not reach here if ignoring");
        إذا (m_assertNoDups && !m_درون_CFunc) {
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
                            << static_cast<Ast_گره*>(nodep->user1p()));
            nodep->user1p(const_cast<فراغ*>(reinterpret_cast<ثابت فراغ*>(m_شروع_Nodep)));
        }

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ثابت uint32_t saved_شمار = m_instr_شمار;
        m_instr_شمار = nodep->instr_شمار();
        أرجع saved_شمار;
    }
    فراغ انتها_بازدید_پایه(uint32_t saved_شمار, Ast_گره* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::چپ << m_instr_شمار << "  " << nodep);
        mark_هزینه(nodep);
        إذا (!m_ignore_مانده) m_instr_شمار += saved_شمار;
    }
    فراغ mark_هزینه(Ast_گره* nodep) {
        إذا (m_osp) nodep->user2(m_instr_شمار + 1);  // Else don't mark to avoid writeback
    }

    // VISITORS
    فراغ بازدید(Ast_گره_Sel* nodep) override {
        إذا (m_ignore_مانده) أرجع;
        // This covers both AstArraySel and AstWordSel
        //
        // If some vector is a bazillion dwords long, and we're selecting 1
        // dword to read or write from it, our cost should be small.
        //
        // Hence, exclude the child of the AstWordSel from the computation,
        // whose cost scales with the size of the entire (maybe large) vector.
        ثابت بازدید_پایه vb{هذا, nodep};
        iterate_And_بعدی_Const_Null(nodep->bitp());
    }
    فراغ بازدید(AstSel* nodep) override {
        إذا (m_ignore_مانده) أرجع;
        // Similar to AstNodeSel above, a small select into a large vector
        // is not expensive. Count the cost of the AstSel itself (scales with
        // its width) and the cost of the lsbp() and widthp() nodes, but not
        // the fromp() node which could be disproportionately large.
        ثابت بازدید_پایه vb{هذا, nodep};
        iterate_And_بعدی_Const_Null(nodep->lsbp());
    }
    فراغ بازدید(AstConcat* nodep) override {
        إذا (m_ignore_مانده) أرجع;
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
        mark_هزینه(nodep);
    }
    فراغ بازدید(Ast_گره_If* nodep) override {
        إذا (m_ignore_مانده) أرجع;
        ثابت بازدید_پایه vb{هذا, nodep};
        iterate_And_بعدی_Const_Null(nodep->condp());
        ثابت uint32_t saved_شمار = m_instr_شمار;

        UINFO(8, "thensp:");
        reset();
        iterate_And_بعدی_Const_Null(nodep->thensp());
        uint32_t if_شمار = m_instr_شمار;
        إذا (nodep->branchPred().unlikely()) if_شمار = 0;

        UINFO(8, "elsesp:");
        reset();
        iterate_And_بعدی_Const_Null(nodep->elsesp());
        uint32_t else_شمار = m_instr_شمار;
        إذا (nodep->branchPred().likely()) else_شمار = 0;

        reset();
        إذا (if_شمار >= else_شمار) {
            m_instr_شمار = saved_شمار + if_شمار;
            إذا (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } وإلا {
            m_instr_شمار = saved_شمار + else_شمار;
            إذا (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    فراغ بازدید(AstCond* nodep) override {
        إذا (m_ignore_مانده) أرجع;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ثابت بازدید_پایه vb{هذا, nodep};
        iterate_And_بعدی_Const_Null(nodep->condp());
        ثابت uint32_t saved_شمار = m_instr_شمار;

        UINFO(8, "?");
        reset();
        iterate_And_بعدی_Const_Null(nodep->thenp());
        ثابت uint32_t if_شمار = m_instr_شمار;

        UINFO(8, ":");
        reset();
        iterate_And_بعدی_Const_Null(nodep->elsep());
        ثابت uint32_t else_شمار = m_instr_شمار;

        reset();
        إذا (if_شمار >= else_شمار) {
            m_instr_شمار = saved_شمار + if_شمار;
            إذا (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } وإلا {
            m_instr_شمار = saved_شمار + else_شمار;
            إذا (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    فراغ بازدید(AstCAwait* nodep) override {
        إذا (m_ignore_مانده) أرجع;
        iterateChildrenConst(nodep);
        // Anything past a co_await is irrelevant
        m_ignore_مانده = درست;
    }
    فراغ بازدید(AstFork* nodep) override {
        إذا (m_ignore_مانده) أرجع;
        ثابت بازدید_پایه vb{هذا, nodep};
        iterate_And_بعدی_Const_Null(nodep->stmtsp());
        uint32_t مجموع_شمار = m_instr_شمار;
        // Sum counts in each statement until the first await
        لكل (Ast_گره* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            reset();
            iterateConst(stmtp);
            مجموع_شمار += m_instr_شمار;
        }
        m_instr_شمار = مجموع_شمار;
        m_ignore_مانده = خطأ;
    }
    فراغ بازدید(Ast_فعال* nodep) override {
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
        mark_هزینه(nodep);
        UASSERT_OBJ(nodep == m_شروع_Nodep, nodep, "Multiple actives, or not start node");
    }
    فراغ بازدید(Ast_گره_CCall* nodep) override {
        إذا (m_ignore_مانده) أرجع;
        ثابت بازدید_پایه vb{هذا, nodep};
        iterateChildrenConst(nodep);
        ثابت خودکار it = m_funcCosts.پیدا_کن(nodep->funcp());
        إذا (it == m_funcCosts.انتها()) {
            m_tracingCall = درست;
            iterateConst(nodep->funcp());
            UASSERT_OBJ(!m_tracingCall, nodep,
                        "visit(AstCFunc) should have cleared m_tracingCall.");
        } وإلا {
            m_instr_شمار += it->دومی;  // This guards and adds zero cost on recursive calls
        }
    }
    فراغ بازدید(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        UASSERT_OBJ(m_tracingCall || nodep == m_شروع_Nodep, nodep,
                    "AstCFunc not under AstCCall, or not start node");
        UASSERT_OBJ(!m_ignore_مانده, nodep, "Should not be ignoring at the start of a CFunc");
        ثابت خودکار emplacePair = m_funcCosts.emplace(nodep, 0);
        UASSERT_OBJ(emplacePair.دومی, nodep, "Should visit every CFunc at most once");
        uint32_t& funcCostr = emplacePair.اولی->دومی;
        m_tracingCall = خطأ;

        ثابت uint32_t saved_شمار = m_instr_شمار;
        reset();
        {
            VL_RESTORER(m_درون_CFunc);
            m_درون_CFunc = درست;
            ثابت بازدید_پایه vb{هذا, nodep};
            iterateChildrenConst(nodep);
        }
        funcCostr = m_instr_شمار;
        m_instr_شمار += saved_شمار;

        m_ignore_مانده = خطأ;
    }
    فراغ بازدید(Ast_گره_Cons* nodep) override {
        إذا (m_ignore_مانده) أرجع;
        ثابت بازدید_پایه vb{هذا, nodep};
        // A constant value is not built at run time, so its parts cost nothing
        إذا (!nodep->is_Constant_درخت()) iterateChildrenConst(nodep);
    }
    فراغ بازدید(Ast_گره* nodep) override {
        إذا (m_ignore_مانده) أرجع;
        ثابت بازدید_پایه vb{هذا, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(Instr_شمار_Visitor);
};

// Iterate the graph printing the critical path marked by previous visitation
صنف Instr_شمار_Dump_Visitor final : عام VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> int.  Path cost, 0 means don't dump

    // MEMBERS
    std::ostream* ثابت m_osp;  // Dump file
    بیعلامت m_عمق = 0;  // Current tree depth for printing indent

عام:
    // CONSTRUCTORS
    Instr_شمار_Dump_Visitor(Ast_گره* nodep, std::ostream* osp)
        : m_osp{osp} {
        // No check for nullptr output, so...
        UASSERT_OBJ(osp, nodep, "Don't call if not dumping");
        إذا (nodep) iterateConst(nodep);
    }
    ~Instr_شمار_Dump_Visitor() override = افتراضي;

خاص:
    // METHODS
    string indent() ثابت { أرجع string(m_عمق, ':') + " "; }
    فراغ بازدید(Ast_گره* nodep) override {
        ++m_عمق;
        إذا (ثابت بیعلامت هزینه_Plus1 = nodep->user2()) {
            *m_osp << "  " << indent() << "cost " << std::setw(6) << std::چپ << (هزینه_Plus1 - 1)
                   << "  " << nodep << '\n';
            iterateChildrenConst(nodep);
        }
        --m_عمق;
    }

    VL_UNCOPYABLE(Instr_شمار_Dump_Visitor);
};

uint32_t V3_Instr_شمار::شمار(Ast_گره* nodep, منطقي assertNoDups, std::ostream* osp) {
    ثابت Instr_شمار_Visitor visitor{nodep, assertNoDups, osp};
    إذا (osp) ثابت Instr_شمار_Dump_Visitor dumper{nodep, osp};
    أرجع visitor.instr_شمار();
}
