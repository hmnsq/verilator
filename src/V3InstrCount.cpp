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

صنف Instr_عدد_Visitor final : عام VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()        -> bool. Processed if assertNoDups
    //  AstNode::user2()        -> uint64_t.  Path cost + 1, 0 means don't dump
    ثابت VNUser2InUse m_inuser2;

    // MEMBERS
    uint32_t m_instr_عدد = 0;  // Running count of instructions
    ثابت Ast_عقدة* ثابت m_startNodep;  // Start node of count
    منطقي m_tracingCall = خطأ;  // Iterating into a CCall to a CFunc
    منطقي m_inCFunc = خطأ;  // Inside AstCFunc
    منطقي m_ignoreRemaining = خطأ;  // Ignore remaining statements in the block
    ثابت منطقي m_assertNoDups;  // Check for duplicates
    ثابت std::ostream* ثابت m_osp;  // Dump file
    std::unordered_map<ثابت AstCFunc*, uint32_t> m_funcCosts;  // Cost of each function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    صنف VisitBase final {
        // MEMBERS
        uint32_t m_saved_عدد;  // Count before visit applied
        Ast_عقدة* ثابت m_nodep;  // Node helper is visiting
        Instr_عدد_Visitor* ثابت m_visitor;  // Visitor whose start/endVisitBase are called

    عام:
        // CONSTRUCTORS
        VisitBase(Instr_عدد_Visitor* visitor, Ast_عقدة* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_عدد = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_عدد, m_nodep); }

    خاص:
        VL_UNCOPYABLE(VisitBase);
    };

عام:
    // CONSTRUCTORS
    Instr_عدد_Visitor(Ast_عقدة* nodep, منطقي assertNoDups, std::ostream* osp)
        : m_startNodep{nodep}
        , m_assertNoDups{assertNoDups}
        , m_osp{osp} {
        iterateConstNull(nodep);
    }
    ~Instr_عدد_Visitor() override = افتراضي;

    // METHODS
    uint32_t instr_عدد() ثابت { أرجع m_instr_عدد; }

خاص:
    فراغ reset() {
        m_instr_عدد = 0;
        m_ignoreRemaining = خطأ;
    }
    uint32_t startVisitBase(Ast_عقدة* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");
        إذا (m_assertNoDups && !m_inCFunc) {
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
                            << static_cast<Ast_عقدة*>(nodep->user1p()));
            nodep->user1p(const_cast<فراغ*>(reinterpret_cast<ثابت فراغ*>(m_startNodep)));
        }

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ثابت uint32_t saved_عدد = m_instr_عدد;
        m_instr_عدد = nodep->instr_عدد();
        أرجع saved_عدد;
    }
    فراغ endVisitBase(uint32_t saved_عدد, Ast_عقدة* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::يسار << m_instr_عدد << "  " << nodep);
        markCost(nodep);
        إذا (!m_ignoreRemaining) m_instr_عدد += saved_عدد;
    }
    فراغ markCost(Ast_عقدة* nodep) {
        إذا (m_osp) nodep->user2(m_instr_عدد + 1);  // Else don't mark to avoid writeback
    }

    // VISITORS
    فراغ visit(Ast_عقدة_Sel* nodep) override {
        إذا (m_ignoreRemaining) أرجع;
        // This covers both AstArraySel and AstWordSel
        //
        // If some vector is a bazillion dwords long, and we're selecting 1
        // dword to read or write from it, our cost should be small.
        //
        // Hence, exclude the child of the AstWordSel from the computation,
        // whose cost scales with the size of the entire (maybe large) vector.
        ثابت VisitBase vb{هذا, nodep};
        iterateAndNextConstNull(nodep->bitp());
    }
    فراغ visit(AstSel* nodep) override {
        إذا (m_ignoreRemaining) أرجع;
        // Similar to AstNodeSel above, a small select into a large vector
        // is not expensive. Count the cost of the AstSel itself (scales with
        // its width) and the cost of the lsbp() and widthp() nodes, but not
        // the fromp() node which could be disproportionately large.
        ثابت VisitBase vb{هذا, nodep};
        iterateAndNextConstNull(nodep->lsbp());
    }
    فراغ visit(AstConcat* nodep) override {
        إذا (m_ignoreRemaining) أرجع;
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
    فراغ visit(Ast_عقدة_If* nodep) override {
        إذا (m_ignoreRemaining) أرجع;
        ثابت VisitBase vb{هذا, nodep};
        iterateAndNextConstNull(nodep->condp());
        ثابت uint32_t saved_عدد = m_instr_عدد;

        UINFO(8, "thensp:");
        reset();
        iterateAndNextConstNull(nodep->thensp());
        uint32_t if_عدد = m_instr_عدد;
        إذا (nodep->branchPred().unlikely()) if_عدد = 0;

        UINFO(8, "elsesp:");
        reset();
        iterateAndNextConstNull(nodep->elsesp());
        uint32_t else_عدد = m_instr_عدد;
        إذا (nodep->branchPred().likely()) else_عدد = 0;

        reset();
        إذا (if_عدد >= else_عدد) {
            m_instr_عدد = saved_عدد + if_عدد;
            إذا (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } وإلا {
            m_instr_عدد = saved_عدد + else_عدد;
            إذا (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    فراغ visit(AstCond* nodep) override {
        إذا (m_ignoreRemaining) أرجع;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ثابت VisitBase vb{هذا, nodep};
        iterateAndNextConstNull(nodep->condp());
        ثابت uint32_t saved_عدد = m_instr_عدد;

        UINFO(8, "?");
        reset();
        iterateAndNextConstNull(nodep->thenp());
        ثابت uint32_t if_عدد = m_instr_عدد;

        UINFO(8, ":");
        reset();
        iterateAndNextConstNull(nodep->elsep());
        ثابت uint32_t else_عدد = m_instr_عدد;

        reset();
        إذا (if_عدد >= else_عدد) {
            m_instr_عدد = saved_عدد + if_عدد;
            إذا (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } وإلا {
            m_instr_عدد = saved_عدد + else_عدد;
            إذا (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    فراغ visit(AstCAwait* nodep) override {
        إذا (m_ignoreRemaining) أرجع;
        iterateChildrenConst(nodep);
        // Anything past a co_await is irrelevant
        m_ignoreRemaining = true;
    }
    فراغ visit(AstFork* nodep) override {
        إذا (m_ignoreRemaining) أرجع;
        ثابت VisitBase vb{هذا, nodep};
        iterateAndNextConstNull(nodep->stmtsp());
        uint32_t total_عدد = m_instr_عدد;
        // Sum counts in each statement until the first await
        لكل (Ast_عقدة* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            reset();
            iterateConst(stmtp);
            total_عدد += m_instr_عدد;
        }
        m_instr_عدد = total_عدد;
        m_ignoreRemaining = خطأ;
    }
    فراغ visit(AstActive* nodep) override {
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
    فراغ visit(Ast_عقدة_CCall* nodep) override {
        إذا (m_ignoreRemaining) أرجع;
        ثابت VisitBase vb{هذا, nodep};
        iterateChildrenConst(nodep);
        ثابت auto it = m_funcCosts.find(nodep->funcp());
        إذا (it == m_funcCosts.end()) {
            m_tracingCall = true;
            iterateConst(nodep->funcp());
            UASSERT_OBJ(!m_tracingCall, nodep,
                        "visit(AstCFunc) should have cleared m_tracingCall.");
        } وإلا {
            m_instr_عدد += it->second;  // This guards and adds zero cost on recursive calls
        }
    }
    فراغ visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        UASSERT_OBJ(m_tracingCall || nodep == m_startNodep, nodep,
                    "AstCFunc not under AstCCall, or not start node");
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not be ignoring at the start of a CFunc");
        ثابت auto emplacePair = m_funcCosts.emplace(nodep, 0);
        UASSERT_OBJ(emplacePair.second, nodep, "Should visit every CFunc at most once");
        uint32_t& funcCostr = emplacePair.first->second;
        m_tracingCall = خطأ;

        ثابت uint32_t saved_عدد = m_instr_عدد;
        reset();
        {
            VL_RESTORER(m_inCFunc);
            m_inCFunc = true;
            ثابت VisitBase vb{هذا, nodep};
            iterateChildrenConst(nodep);
        }
        funcCostr = m_instr_عدد;
        m_instr_عدد += saved_عدد;

        m_ignoreRemaining = خطأ;
    }
    فراغ visit(Ast_عقدة_Cons* nodep) override {
        إذا (m_ignoreRemaining) أرجع;
        ثابت VisitBase vb{هذا, nodep};
        // A constant value is not built at run time, so its parts cost nothing
        إذا (!nodep->is_Constant_شجرة()) iterateChildrenConst(nodep);
    }
    فراغ visit(Ast_عقدة* nodep) override {
        إذا (m_ignoreRemaining) أرجع;
        ثابت VisitBase vb{هذا, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(Instr_عدد_Visitor);
};

// Iterate the graph printing the critical path marked by previous visitation
صنف Instr_عدد_Dump_Visitor final : عام VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> int.  Path cost, 0 means don't dump

    // MEMBERS
    std::ostream* ثابت m_osp;  // Dump file
    unsigned m_depth = 0;  // Current tree depth for printing indent

عام:
    // CONSTRUCTORS
    Instr_عدد_Dump_Visitor(Ast_عقدة* nodep, std::ostream* osp)
        : m_osp{osp} {
        // No check for nullptr output, so...
        UASSERT_OBJ(osp, nodep, "Don't call if not dumping");
        إذا (nodep) iterateConst(nodep);
    }
    ~Instr_عدد_Dump_Visitor() override = افتراضي;

خاص:
    // METHODS
    string indent() ثابت { أرجع string(m_depth, ':') + " "; }
    فراغ visit(Ast_عقدة* nodep) override {
        ++m_depth;
        إذا (ثابت unsigned costPlus1 = nodep->user2()) {
            *m_osp << "  " << indent() << "cost " << std::setw(6) << std::يسار << (costPlus1 - 1)
                   << "  " << nodep << '\n';
            iterateChildrenConst(nodep);
        }
        --m_depth;
    }

    VL_UNCOPYABLE(Instr_عدد_Dump_Visitor);
};

uint32_t V3_Instr_عدد::عدد(Ast_عقدة* nodep, منطقي assertNoDups, std::ostream* osp) {
    ثابت Instr_عدد_Visitor visitor{nodep, assertNoDups, osp};
    إذا (osp) ثابت Instr_عدد_Dump_Visitor dumper{nodep, osp};
    أرجع visitor.instr_عدد();
}
