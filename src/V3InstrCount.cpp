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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3InstrCount.h"

#અન્તર્ભાવ <iomanip>
#અન્તર્ભાવ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the instruction cost for executing all logic within and below
/// a given AST node. Note this estimates the number of instructions we'll
/// execute, not the number we'll generate. That is, for conditionals,
/// we'll count instructions from either the 'if' or the 'else' branch,
/// whichever is larger. We know we won't run both.

વર્ગ Instr_ગણતરી_Visitor final : જાહેર VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()        -> bool. Processed if assertNoDups
    //  AstNode::user2()        -> uint64_t.  Path cost + 1, 0 means don't dump
    અચલ VNUser2InUse m_inuser2;

    // MEMBERS
    uint32_t m_instr_ગણતરી = 0;  // Running count of instructions
    અચલ Ast_ગાંઠ* અચલ m_startNodep;  // Start node of count
    તાર્કિક m_tracingCall = ખોટું;  // Iterating into a CCall to a CFunc
    તાર્કિક m_inCFunc = ખોટું;  // Inside AstCFunc
    તાર્કિક m_ignoreRemaining = ખોટું;  // Ignore remaining statements in the block
    અચલ તાર્કિક m_assertNoDups;  // Check for duplicates
    અચલ std::ostream* અચલ m_osp;  // Dump file
    std::unordered_map<અચલ AstCFunc*, uint32_t> m_funcCosts;  // Cost of each function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    વર્ગ VisitBase final {
        // MEMBERS
        uint32_t m_saved_ગણતરી;  // Count before visit applied
        Ast_ગાંઠ* અચલ m_nodep;  // Node helper is visiting
        Instr_ગણતરી_Visitor* અચલ m_visitor;  // Visitor whose start/endVisitBase are called

    જાહેર:
        // CONSTRUCTORS
        VisitBase(Instr_ગણતરી_Visitor* visitor, Ast_ગાંઠ* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_ગણતરી = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_ગણતરી, m_nodep); }

    ખાનગી:
        VL_UNCOPYABLE(VisitBase);
    };

જાહેર:
    // CONSTRUCTORS
    Instr_ગણતરી_Visitor(Ast_ગાંઠ* nodep, તાર્કિક assertNoDups, std::ostream* osp)
        : m_startNodep{nodep}
        , m_assertNoDups{assertNoDups}
        , m_osp{osp} {
        iterateConstNull(nodep);
    }
    ~Instr_ગણતરી_Visitor() override = મૂળભૂત;

    // METHODS
    uint32_t instr_ગણતરી() અચલ { પાછા m_instr_ગણતરી; }

ખાનગી:
    શૂન્ય reset() {
        m_instr_ગણતરી = 0;
        m_ignoreRemaining = ખોટું;
    }
    uint32_t startVisitBase(Ast_ગાંઠ* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");
        જો (m_assertNoDups && !m_inCFunc) {
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
                            << static_cast<Ast_ગાંઠ*>(nodep->user1p()));
            nodep->user1p(const_cast<શૂન્ય*>(reinterpret_cast<અચલ શૂન્ય*>(m_startNodep)));
        }

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        અચલ uint32_t saved_ગણતરી = m_instr_ગણતરી;
        m_instr_ગણતરી = nodep->instr_ગણતરી();
        પાછા saved_ગણતરી;
    }
    શૂન્ય endVisitBase(uint32_t saved_ગણતરી, Ast_ગાંઠ* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::ડાબું << m_instr_ગણતરી << "  " << nodep);
        markCost(nodep);
        જો (!m_ignoreRemaining) m_instr_ગણતરી += saved_ગણતરી;
    }
    શૂન્ય markCost(Ast_ગાંઠ* nodep) {
        જો (m_osp) nodep->user2(m_instr_ગણતરી + 1);  // Else don't mark to avoid writeback
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Sel* nodep) override {
        જો (m_ignoreRemaining) પાછા;
        // This covers both AstArraySel and AstWordSel
        //
        // If some vector is a bazillion dwords long, and we're selecting 1
        // dword to read or write from it, our cost should be small.
        //
        // Hence, exclude the child of the AstWordSel from the computation,
        // whose cost scales with the size of the entire (maybe large) vector.
        અચલ VisitBase vb{આ, nodep};
        iterateAndNextConstNull(nodep->bitp());
    }
    શૂન્ય visit(AstSel* nodep) override {
        જો (m_ignoreRemaining) પાછા;
        // Similar to AstNodeSel above, a small select into a large vector
        // is not expensive. Count the cost of the AstSel itself (scales with
        // its width) and the cost of the lsbp() and widthp() nodes, but not
        // the fromp() node which could be disproportionately large.
        અચલ VisitBase vb{આ, nodep};
        iterateAndNextConstNull(nodep->lsbp());
    }
    શૂન્ય visit(AstConcat* nodep) override {
        જો (m_ignoreRemaining) પાછા;
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
    શૂન્ય visit(Ast_ગાંઠ_If* nodep) override {
        જો (m_ignoreRemaining) પાછા;
        અચલ VisitBase vb{આ, nodep};
        iterateAndNextConstNull(nodep->condp());
        અચલ uint32_t saved_ગણતરી = m_instr_ગણતરી;

        UINFO(8, "thensp:");
        reset();
        iterateAndNextConstNull(nodep->thensp());
        uint32_t if_ગણતરી = m_instr_ગણતરી;
        જો (nodep->branchPred().unlikely()) if_ગણતરી = 0;

        UINFO(8, "elsesp:");
        reset();
        iterateAndNextConstNull(nodep->elsesp());
        uint32_t else_ગણતરી = m_instr_ગણતરી;
        જો (nodep->branchPred().likely()) else_ગણતરી = 0;

        reset();
        જો (if_ગણતરી >= else_ગણતરી) {
            m_instr_ગણતરી = saved_ગણતરી + if_ગણતરી;
            જો (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } નહીંતર {
            m_instr_ગણતરી = saved_ગણતરી + else_ગણતરી;
            જો (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    શૂન્ય visit(AstCond* nodep) override {
        જો (m_ignoreRemaining) પાછા;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        અચલ VisitBase vb{આ, nodep};
        iterateAndNextConstNull(nodep->condp());
        અચલ uint32_t saved_ગણતરી = m_instr_ગણતરી;

        UINFO(8, "?");
        reset();
        iterateAndNextConstNull(nodep->thenp());
        અચલ uint32_t if_ગણતરી = m_instr_ગણતરી;

        UINFO(8, ":");
        reset();
        iterateAndNextConstNull(nodep->elsep());
        અચલ uint32_t else_ગણતરી = m_instr_ગણતરી;

        reset();
        જો (if_ગણતરી >= else_ગણતરી) {
            m_instr_ગણતરી = saved_ગણતરી + if_ગણતરી;
            જો (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } નહીંતર {
            m_instr_ગણતરી = saved_ગણતરી + else_ગણતરી;
            જો (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    શૂન્ય visit(AstCAwait* nodep) override {
        જો (m_ignoreRemaining) પાછા;
        iterateChildrenConst(nodep);
        // Anything past a co_await is irrelevant
        m_ignoreRemaining = સાચું;
    }
    શૂન્ય visit(AstFork* nodep) override {
        જો (m_ignoreRemaining) પાછા;
        અચલ VisitBase vb{આ, nodep};
        iterateAndNextConstNull(nodep->stmtsp());
        uint32_t total_ગણતરી = m_instr_ગણતરી;
        // Sum counts in each statement until the first await
        માટે (Ast_ગાંઠ* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            reset();
            iterateConst(stmtp);
            total_ગણતરી += m_instr_ગણતરી;
        }
        m_instr_ગણતરી = total_ગણતરી;
        m_ignoreRemaining = ખોટું;
    }
    શૂન્ય visit(AstActive* nodep) override {
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
    શૂન્ય visit(Ast_ગાંઠ_CCall* nodep) override {
        જો (m_ignoreRemaining) પાછા;
        અચલ VisitBase vb{આ, nodep};
        iterateChildrenConst(nodep);
        અચલ auto it = m_funcCosts.find(nodep->funcp());
        જો (it == m_funcCosts.end()) {
            m_tracingCall = સાચું;
            iterateConst(nodep->funcp());
            UASSERT_OBJ(!m_tracingCall, nodep,
                        "visit(AstCFunc) should have cleared m_tracingCall.");
        } નહીંતર {
            m_instr_ગણતરી += it->second;  // This guards and adds zero cost on recursive calls
        }
    }
    શૂન્ય visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        UASSERT_OBJ(m_tracingCall || nodep == m_startNodep, nodep,
                    "AstCFunc not under AstCCall, or not start node");
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not be ignoring at the start of a CFunc");
        અચલ auto emplacePair = m_funcCosts.emplace(nodep, 0);
        UASSERT_OBJ(emplacePair.second, nodep, "Should visit every CFunc at most once");
        uint32_t& funcCostr = emplacePair.first->second;
        m_tracingCall = ખોટું;

        અચલ uint32_t saved_ગણતરી = m_instr_ગણતરી;
        reset();
        {
            VL_RESTORER(m_inCFunc);
            m_inCFunc = સાચું;
            અચલ VisitBase vb{આ, nodep};
            iterateChildrenConst(nodep);
        }
        funcCostr = m_instr_ગણતરી;
        m_instr_ગણતરી += saved_ગણતરી;

        m_ignoreRemaining = ખોટું;
    }
    શૂન્ય visit(Ast_ગાંઠ_Cons* nodep) override {
        જો (m_ignoreRemaining) પાછા;
        અચલ VisitBase vb{આ, nodep};
        // A constant value is not built at run time, so its parts cost nothing
        જો (!nodep->is_Constant_વૃક્ષ()) iterateChildrenConst(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        જો (m_ignoreRemaining) પાછા;
        અચલ VisitBase vb{આ, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(Instr_ગણતરી_Visitor);
};

// Iterate the graph printing the critical path marked by previous visitation
વર્ગ Instr_ગણતરી_Dump_Visitor final : જાહેર VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> int.  Path cost, 0 means don't dump

    // MEMBERS
    std::ostream* અચલ m_osp;  // Dump file
    unsigned m_depth = 0;  // Current tree depth for printing indent

જાહેર:
    // CONSTRUCTORS
    Instr_ગણતરી_Dump_Visitor(Ast_ગાંઠ* nodep, std::ostream* osp)
        : m_osp{osp} {
        // No check for nullptr output, so...
        UASSERT_OBJ(osp, nodep, "Don't call if not dumping");
        જો (nodep) iterateConst(nodep);
    }
    ~Instr_ગણતરી_Dump_Visitor() override = મૂળભૂત;

ખાનગી:
    // METHODS
    string indent() અચલ { પાછા string(m_depth, ':') + " "; }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        ++m_depth;
        જો (અચલ unsigned costPlus1 = nodep->user2()) {
            *m_osp << "  " << indent() << "cost " << std::setw(6) << std::ડાબું << (costPlus1 - 1)
                   << "  " << nodep << '\n';
            iterateChildrenConst(nodep);
        }
        --m_depth;
    }

    VL_UNCOPYABLE(Instr_ગણતરી_Dump_Visitor);
};

uint32_t V3_Instr_ગણતરી::ગણતરી(Ast_ગાંઠ* nodep, તાર્કિક assertNoDups, std::ostream* osp) {
    અચલ Instr_ગણતરી_Visitor visitor{nodep, assertNoDups, osp};
    જો (osp) અચલ Instr_ગણતરી_Dump_Visitor dumper{nodep, osp};
    પાછા visitor.instr_ગણતરી();
}
