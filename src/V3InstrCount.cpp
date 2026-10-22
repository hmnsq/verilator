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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3InstrCount.h"

#অন্তর্ভাব <iomanip>
#অন্তর্ভাব <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the instruction cost for executing all logic within and below
/// a given AST node. Note this estimates the number of instructions we'll
/// execute, not the number we'll generate. That is, for conditionals,
/// we'll count instructions from either the 'if' or the 'else' branch,
/// whichever is larger. We know we won't run both.

শ্রেণী Instr_গুনতি_Visitor final : সর্বজনীন VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()        -> bool. Processed if assertNoDups
    //  AstNode::user2()        -> uint64_t.  Path cost + 1, 0 means don't dump
    ধ্রুবক VNUser2InUse m_inuser2;

    // MEMBERS
    uint32_t m_instr_গুনতি = 0;  // Running count of instructions
    ধ্রুবক Ast_গ্রন্থি* ধ্রুবক m_startNodep;  // Start node of count
    যৌক্তিক m_tracingCall = মিথ্যা;  // Iterating into a CCall to a CFunc
    যৌক্তিক m_inCFunc = মিথ্যা;  // Inside AstCFunc
    যৌক্তিক m_ignoreRemaining = মিথ্যা;  // Ignore remaining statements in the block
    ধ্রুবক যৌক্তিক m_assertNoDups;  // Check for duplicates
    ধ্রুবক std::ostream* ধ্রুবক m_osp;  // Dump file
    std::unordered_map<ধ্রুবক AstCFunc*, uint32_t> m_funcCosts;  // Cost of each function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    শ্রেণী VisitBase final {
        // MEMBERS
        uint32_t m_saved_গুনতি;  // Count before visit applied
        Ast_গ্রন্থি* ধ্রুবক m_nodep;  // Node helper is visiting
        Instr_গুনতি_Visitor* ধ্রুবক m_visitor;  // Visitor whose start/endVisitBase are called

    সর্বজনীন:
        // CONSTRUCTORS
        VisitBase(Instr_গুনতি_Visitor* visitor, Ast_গ্রন্থি* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_গুনতি = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_গুনতি, m_nodep); }

    ব্যক্তিগত:
        VL_UNCOPYABLE(VisitBase);
    };

সর্বজনীন:
    // CONSTRUCTORS
    Instr_গুনতি_Visitor(Ast_গ্রন্থি* nodep, যৌক্তিক assertNoDups, std::ostream* osp)
        : m_startNodep{nodep}
        , m_assertNoDups{assertNoDups}
        , m_osp{osp} {
        iterateConstNull(nodep);
    }
    ~Instr_গুনতি_Visitor() override = পূর্বনির্ধারিত;

    // METHODS
    uint32_t instr_গুনতি() ধ্রুবক { ফেরত m_instr_গুনতি; }

ব্যক্তিগত:
    শূন্য reset() {
        m_instr_গুনতি = 0;
        m_ignoreRemaining = মিথ্যা;
    }
    uint32_t startVisitBase(Ast_গ্রন্থি* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");
        যদি (m_assertNoDups && !m_inCFunc) {
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
                            << static_cast<Ast_গ্রন্থি*>(nodep->user1p()));
            nodep->user1p(const_cast<শূন্য*>(reinterpret_cast<ধ্রুবক শূন্য*>(m_startNodep)));
        }

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ধ্রুবক uint32_t saved_গুনতি = m_instr_গুনতি;
        m_instr_গুনতি = nodep->instr_গুনতি();
        ফেরত saved_গুনতি;
    }
    শূন্য endVisitBase(uint32_t saved_গুনতি, Ast_গ্রন্থি* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::বাম << m_instr_গুনতি << "  " << nodep);
        markCost(nodep);
        যদি (!m_ignoreRemaining) m_instr_গুনতি += saved_গুনতি;
    }
    শূন্য markCost(Ast_গ্রন্থি* nodep) {
        যদি (m_osp) nodep->user2(m_instr_গুনতি + 1);  // Else don't mark to avoid writeback
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Sel* nodep) override {
        যদি (m_ignoreRemaining) ফেরত;
        // This covers both AstArraySel and AstWordSel
        //
        // If some vector is a bazillion dwords long, and we're selecting 1
        // dword to read or write from it, our cost should be small.
        //
        // Hence, exclude the child of the AstWordSel from the computation,
        // whose cost scales with the size of the entire (maybe large) vector.
        ধ্রুবক VisitBase vb{এই, nodep};
        iterate_And_পরবর্তী_Const_Null(nodep->bitp());
    }
    শূন্য visit(AstSel* nodep) override {
        যদি (m_ignoreRemaining) ফেরত;
        // Similar to AstNodeSel above, a small select into a large vector
        // is not expensive. Count the cost of the AstSel itself (scales with
        // its width) and the cost of the lsbp() and widthp() nodes, but not
        // the fromp() node which could be disproportionately large.
        ধ্রুবক VisitBase vb{এই, nodep};
        iterate_And_পরবর্তী_Const_Null(nodep->lsbp());
    }
    শূন্য visit(AstConcat* nodep) override {
        যদি (m_ignoreRemaining) ফেরত;
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
        markCost(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_If* nodep) override {
        যদি (m_ignoreRemaining) ফেরত;
        ধ্রুবক VisitBase vb{এই, nodep};
        iterate_And_পরবর্তী_Const_Null(nodep->condp());
        ধ্রুবক uint32_t saved_গুনতি = m_instr_গুনতি;

        UINFO(8, "thensp:");
        reset();
        iterate_And_পরবর্তী_Const_Null(nodep->thensp());
        uint32_t if_গুনতি = m_instr_গুনতি;
        যদি (nodep->branchPred().unlikely()) if_গুনতি = 0;

        UINFO(8, "elsesp:");
        reset();
        iterate_And_পরবর্তী_Const_Null(nodep->elsesp());
        uint32_t else_গুনতি = m_instr_গুনতি;
        যদি (nodep->branchPred().likely()) else_গুনতি = 0;

        reset();
        যদি (if_গুনতি >= else_গুনতি) {
            m_instr_গুনতি = saved_গুনতি + if_গুনতি;
            যদি (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } অন্যথা {
            m_instr_গুনতি = saved_গুনতি + else_গুনতি;
            যদি (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    শূন্য visit(AstCond* nodep) override {
        যদি (m_ignoreRemaining) ফেরত;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ধ্রুবক VisitBase vb{এই, nodep};
        iterate_And_পরবর্তী_Const_Null(nodep->condp());
        ধ্রুবক uint32_t saved_গুনতি = m_instr_গুনতি;

        UINFO(8, "?");
        reset();
        iterate_And_পরবর্তী_Const_Null(nodep->thenp());
        ধ্রুবক uint32_t if_গুনতি = m_instr_গুনতি;

        UINFO(8, ":");
        reset();
        iterate_And_পরবর্তী_Const_Null(nodep->elsep());
        ধ্রুবক uint32_t else_গুনতি = m_instr_গুনতি;

        reset();
        যদি (if_গুনতি >= else_গুনতি) {
            m_instr_গুনতি = saved_গুনতি + if_গুনতি;
            যদি (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } অন্যথা {
            m_instr_গুনতি = saved_গুনতি + else_গুনতি;
            যদি (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    শূন্য visit(AstCAwait* nodep) override {
        যদি (m_ignoreRemaining) ফেরত;
        iterateChildrenConst(nodep);
        // Anything past a co_await is irrelevant
        m_ignoreRemaining = সত্য;
    }
    শূন্য visit(AstFork* nodep) override {
        যদি (m_ignoreRemaining) ফেরত;
        ধ্রুবক VisitBase vb{এই, nodep};
        iterate_And_পরবর্তী_Const_Null(nodep->stmtsp());
        uint32_t যোগ_গুনতি = m_instr_গুনতি;
        // Sum counts in each statement until the first await
        জন্য (Ast_গ্রন্থি* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            reset();
            iterateConst(stmtp);
            যোগ_গুনতি += m_instr_গুনতি;
        }
        m_instr_গুনতি = যোগ_গুনতি;
        m_ignoreRemaining = মিথ্যা;
    }
    শূন্য visit(AstActive* nodep) override {
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
        markCost(nodep);
        UASSERT_OBJ(nodep == m_startNodep, nodep, "Multiple actives, or not start node");
    }
    শূন্য visit(Ast_গ্রন্থি_CCall* nodep) override {
        যদি (m_ignoreRemaining) ফেরত;
        ধ্রুবক VisitBase vb{এই, nodep};
        iterateChildrenConst(nodep);
        ধ্রুবক auto it = m_funcCosts.find(nodep->funcp());
        যদি (it == m_funcCosts.end()) {
            m_tracingCall = সত্য;
            iterateConst(nodep->funcp());
            UASSERT_OBJ(!m_tracingCall, nodep,
                        "visit(AstCFunc) should have cleared m_tracingCall.");
        } অন্যথা {
            m_instr_গুনতি += it->second;  // This guards and adds zero cost on recursive calls
        }
    }
    শূন্য visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        UASSERT_OBJ(m_tracingCall || nodep == m_startNodep, nodep,
                    "AstCFunc not under AstCCall, or not start node");
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not be ignoring at the start of a CFunc");
        ধ্রুবক auto emplacePair = m_funcCosts.emplace(nodep, 0);
        UASSERT_OBJ(emplacePair.second, nodep, "Should visit every CFunc at most once");
        uint32_t& funcCostr = emplacePair.first->second;
        m_tracingCall = মিথ্যা;

        ধ্রুবক uint32_t saved_গুনতি = m_instr_গুনতি;
        reset();
        {
            VL_RESTORER(m_inCFunc);
            m_inCFunc = সত্য;
            ধ্রুবক VisitBase vb{এই, nodep};
            iterateChildrenConst(nodep);
        }
        funcCostr = m_instr_গুনতি;
        m_instr_গুনতি += saved_গুনতি;

        m_ignoreRemaining = মিথ্যা;
    }
    শূন্য visit(Ast_গ্রন্থি_Cons* nodep) override {
        যদি (m_ignoreRemaining) ফেরত;
        ধ্রুবক VisitBase vb{এই, nodep};
        // A constant value is not built at run time, so its parts cost nothing
        যদি (!nodep->is_Constant_বৃক্ষ()) iterateChildrenConst(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যদি (m_ignoreRemaining) ফেরত;
        ধ্রুবক VisitBase vb{এই, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(Instr_গুনতি_Visitor);
};

// Iterate the graph printing the critical path marked by previous visitation
শ্রেণী Instr_গুনতি_Dump_Visitor final : সর্বজনীন VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> int.  Path cost, 0 means don't dump

    // MEMBERS
    std::ostream* ধ্রুবক m_osp;  // Dump file
    unsigned m_depth = 0;  // Current tree depth for printing indent

সর্বজনীন:
    // CONSTRUCTORS
    Instr_গুনতি_Dump_Visitor(Ast_গ্রন্থি* nodep, std::ostream* osp)
        : m_osp{osp} {
        // No check for nullptr output, so...
        UASSERT_OBJ(osp, nodep, "Don't call if not dumping");
        যদি (nodep) iterateConst(nodep);
    }
    ~Instr_গুনতি_Dump_Visitor() override = পূর্বনির্ধারিত;

ব্যক্তিগত:
    // METHODS
    string indent() ধ্রুবক { ফেরত string(m_depth, ':') + " "; }
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        ++m_depth;
        যদি (ধ্রুবক unsigned costPlus1 = nodep->user2()) {
            *m_osp << "  " << indent() << "cost " << std::setw(6) << std::বাম << (costPlus1 - 1)
                   << "  " << nodep << '\n';
            iterateChildrenConst(nodep);
        }
        --m_depth;
    }

    VL_UNCOPYABLE(Instr_গুনতি_Dump_Visitor);
};

uint32_t V3_Instr_গুনতি::গুনতি(Ast_গ্রন্থি* nodep, যৌক্তিক assertNoDups, std::ostream* osp) {
    ধ্রুবক Instr_গুনতি_Visitor visitor{nodep, assertNoDups, osp};
    যদি (osp) ধ্রুবক Instr_গুনতি_Dump_Visitor dumper{nodep, osp};
    ফেরত visitor.instr_গুনতি();
}
