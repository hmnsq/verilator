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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3InstrCount.h"

#شامل <iomanip>
#شامل <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the instruction cost for executing all logic within and below
/// a given AST node. Note this estimates the number of instructions we'll
/// execute, not the number we'll generate. That is, for conditionals,
/// we'll count instructions from either the 'if' or the 'else' branch,
/// whichever is larger. We know we won't run both.

رده Instr_شمار_Visitor final : عمومی VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()        -> bool. Processed if assertNoDups
    //  AstNode::user2()        -> uint64_t.  Path cost + 1, 0 means don't dump
    ثابت VNUser2InUse m_inuser2;

    // MEMBERS
    uint32_t m_instr_شمار = 0;  // Running count of instructions
    ثابت Ast_گره* ثابت m_startNodep;  // Start node of count
    منطقی m_tracingCall = نادرست;  // Iterating into a CCall to a CFunc
    منطقی m_inCFunc = نادرست;  // Inside AstCFunc
    منطقی m_ignoreRemaining = نادرست;  // Ignore remaining statements in the block
    ثابت منطقی m_assertNoDups;  // Check for duplicates
    ثابت std::ostream* ثابت m_osp;  // Dump file
    std::unordered_map<ثابت AstCFunc*, uint32_t> m_funcCosts;  // Cost of each function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    رده VisitBase final {
        // MEMBERS
        uint32_t m_saved_شمار;  // Count before visit applied
        Ast_گره* ثابت m_nodep;  // Node helper is visiting
        Instr_شمار_Visitor* ثابت m_visitor;  // Visitor whose start/endVisitBase are called

    عمومی:
        // CONSTRUCTORS
        VisitBase(Instr_شمار_Visitor* visitor, Ast_گره* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_شمار = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_شمار, m_nodep); }

    خصوصی:
        VL_UNCOPYABLE(VisitBase);
    };

عمومی:
    // CONSTRUCTORS
    Instr_شمار_Visitor(Ast_گره* nodep, منطقی assertNoDups, std::ostream* osp)
        : m_startNodep{nodep}
        , m_assertNoDups{assertNoDups}
        , m_osp{osp} {
        iterateConstNull(nodep);
    }
    ~Instr_شمار_Visitor() override = پیشفرض;

    // METHODS
    uint32_t instr_شمار() ثابت { بازگشت m_instr_شمار; }

خصوصی:
    تهی reset() {
        m_instr_شمار = 0;
        m_ignoreRemaining = نادرست;
    }
    uint32_t startVisitBase(Ast_گره* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");
        اگر (m_assertNoDups && !m_inCFunc) {
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
            nodep->user1p(const_cast<تهی*>(reinterpret_cast<ثابت تهی*>(m_startNodep)));
        }

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ثابت uint32_t saved_شمار = m_instr_شمار;
        m_instr_شمار = nodep->instr_شمار();
        بازگشت saved_شمار;
    }
    تهی endVisitBase(uint32_t saved_شمار, Ast_گره* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::چپ << m_instr_شمار << "  " << nodep);
        markCost(nodep);
        اگر (!m_ignoreRemaining) m_instr_شمار += saved_شمار;
    }
    تهی markCost(Ast_گره* nodep) {
        اگر (m_osp) nodep->user2(m_instr_شمار + 1);  // Else don't mark to avoid writeback
    }

    // VISITORS
    تهی visit(Ast_گره_Sel* nodep) override {
        اگر (m_ignoreRemaining) بازگشت;
        // This covers both AstArraySel and AstWordSel
        //
        // If some vector is a bazillion dwords long, and we're selecting 1
        // dword to read or write from it, our cost should be small.
        //
        // Hence, exclude the child of the AstWordSel from the computation,
        // whose cost scales with the size of the entire (maybe large) vector.
        ثابت VisitBase vb{این, nodep};
        iterateAndNextConstNull(nodep->bitp());
    }
    تهی visit(AstSel* nodep) override {
        اگر (m_ignoreRemaining) بازگشت;
        // Similar to AstNodeSel above, a small select into a large vector
        // is not expensive. Count the cost of the AstSel itself (scales with
        // its width) and the cost of the lsbp() and widthp() nodes, but not
        // the fromp() node which could be disproportionately large.
        ثابت VisitBase vb{این, nodep};
        iterateAndNextConstNull(nodep->lsbp());
    }
    تهی visit(AstConcat* nodep) override {
        اگر (m_ignoreRemaining) بازگشت;
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
    تهی visit(Ast_گره_If* nodep) override {
        اگر (m_ignoreRemaining) بازگشت;
        ثابت VisitBase vb{این, nodep};
        iterateAndNextConstNull(nodep->condp());
        ثابت uint32_t saved_شمار = m_instr_شمار;

        UINFO(8, "thensp:");
        reset();
        iterateAndNextConstNull(nodep->thensp());
        uint32_t if_شمار = m_instr_شمار;
        اگر (nodep->branchPred().unlikely()) if_شمار = 0;

        UINFO(8, "elsesp:");
        reset();
        iterateAndNextConstNull(nodep->elsesp());
        uint32_t else_شمار = m_instr_شمار;
        اگر (nodep->branchPred().likely()) else_شمار = 0;

        reset();
        اگر (if_شمار >= else_شمار) {
            m_instr_شمار = saved_شمار + if_شمار;
            اگر (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } وگرنه {
            m_instr_شمار = saved_شمار + else_شمار;
            اگر (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    تهی visit(AstCond* nodep) override {
        اگر (m_ignoreRemaining) بازگشت;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ثابت VisitBase vb{این, nodep};
        iterateAndNextConstNull(nodep->condp());
        ثابت uint32_t saved_شمار = m_instr_شمار;

        UINFO(8, "?");
        reset();
        iterateAndNextConstNull(nodep->thenp());
        ثابت uint32_t if_شمار = m_instr_شمار;

        UINFO(8, ":");
        reset();
        iterateAndNextConstNull(nodep->elsep());
        ثابت uint32_t else_شمار = m_instr_شمار;

        reset();
        اگر (if_شمار >= else_شمار) {
            m_instr_شمار = saved_شمار + if_شمار;
            اگر (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } وگرنه {
            m_instr_شمار = saved_شمار + else_شمار;
            اگر (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    تهی visit(AstCAwait* nodep) override {
        اگر (m_ignoreRemaining) بازگشت;
        iterateChildrenConst(nodep);
        // Anything past a co_await is irrelevant
        m_ignoreRemaining = درست;
    }
    تهی visit(AstFork* nodep) override {
        اگر (m_ignoreRemaining) بازگشت;
        ثابت VisitBase vb{این, nodep};
        iterateAndNextConstNull(nodep->stmtsp());
        uint32_t total_شمار = m_instr_شمار;
        // Sum counts in each statement until the first await
        برای (Ast_گره* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            reset();
            iterateConst(stmtp);
            total_شمار += m_instr_شمار;
        }
        m_instr_شمار = total_شمار;
        m_ignoreRemaining = نادرست;
    }
    تهی visit(AstActive* nodep) override {
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
    تهی visit(Ast_گره_CCall* nodep) override {
        اگر (m_ignoreRemaining) بازگشت;
        ثابت VisitBase vb{این, nodep};
        iterateChildrenConst(nodep);
        ثابت auto it = m_funcCosts.find(nodep->funcp());
        اگر (it == m_funcCosts.end()) {
            m_tracingCall = درست;
            iterateConst(nodep->funcp());
            UASSERT_OBJ(!m_tracingCall, nodep,
                        "visit(AstCFunc) should have cleared m_tracingCall.");
        } وگرنه {
            m_instr_شمار += it->second;  // This guards and adds zero cost on recursive calls
        }
    }
    تهی visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        UASSERT_OBJ(m_tracingCall || nodep == m_startNodep, nodep,
                    "AstCFunc not under AstCCall, or not start node");
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not be ignoring at the start of a CFunc");
        ثابت auto emplacePair = m_funcCosts.emplace(nodep, 0);
        UASSERT_OBJ(emplacePair.second, nodep, "Should visit every CFunc at most once");
        uint32_t& funcCostr = emplacePair.first->second;
        m_tracingCall = نادرست;

        ثابت uint32_t saved_شمار = m_instr_شمار;
        reset();
        {
            VL_RESTORER(m_inCFunc);
            m_inCFunc = درست;
            ثابت VisitBase vb{این, nodep};
            iterateChildrenConst(nodep);
        }
        funcCostr = m_instr_شمار;
        m_instr_شمار += saved_شمار;

        m_ignoreRemaining = نادرست;
    }
    تهی visit(Ast_گره_Cons* nodep) override {
        اگر (m_ignoreRemaining) بازگشت;
        ثابت VisitBase vb{این, nodep};
        // A constant value is not built at run time, so its parts cost nothing
        اگر (!nodep->is_Constant_درخت()) iterateChildrenConst(nodep);
    }
    تهی visit(Ast_گره* nodep) override {
        اگر (m_ignoreRemaining) بازگشت;
        ثابت VisitBase vb{این, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(Instr_شمار_Visitor);
};

// Iterate the graph printing the critical path marked by previous visitation
رده Instr_شمار_Dump_Visitor final : عمومی VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> int.  Path cost, 0 means don't dump

    // MEMBERS
    std::ostream* ثابت m_osp;  // Dump file
    unsigned m_depth = 0;  // Current tree depth for printing indent

عمومی:
    // CONSTRUCTORS
    Instr_شمار_Dump_Visitor(Ast_گره* nodep, std::ostream* osp)
        : m_osp{osp} {
        // No check for nullptr output, so...
        UASSERT_OBJ(osp, nodep, "Don't call if not dumping");
        اگر (nodep) iterateConst(nodep);
    }
    ~Instr_شمار_Dump_Visitor() override = پیشفرض;

خصوصی:
    // METHODS
    string indent() ثابت { بازگشت string(m_depth, ':') + " "; }
    تهی visit(Ast_گره* nodep) override {
        ++m_depth;
        اگر (ثابت unsigned costPlus1 = nodep->user2()) {
            *m_osp << "  " << indent() << "cost " << std::setw(6) << std::چپ << (costPlus1 - 1)
                   << "  " << nodep << '\n';
            iterateChildrenConst(nodep);
        }
        --m_depth;
    }

    VL_UNCOPYABLE(Instr_شمار_Dump_Visitor);
};

uint32_t V3_Instr_شمار::شمار(Ast_گره* nodep, منطقی assertNoDups, std::ostream* osp) {
    ثابت Instr_شمار_Visitor visitor{nodep, assertNoDups, osp};
    اگر (osp) ثابت Instr_شمار_Dump_Visitor dumper{nodep, osp};
    بازگشت visitor.instr_شمار();
}
