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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3InstrCount.h"

#ಅನ್ತರ್ಭಾವ <iomanip>
#ಅನ್ತರ್ಭಾವ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the instruction cost for executing all logic within and below
/// a given AST node. Note this estimates the number of instructions we'll
/// execute, not the number we'll generate. That is, for conditionals,
/// we'll count instructions from either the 'if' or the 'else' branch,
/// whichever is larger. We know we won't run both.

ವರ್ಗ Instr_ಎಣಿಕೆ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()        -> bool. Processed if assertNoDups
    //  AstNode::user2()        -> uint64_t.  Path cost + 1, 0 means don't dump
    ಧ್ರುವ VNUser2InUse m_inuser2;

    // MEMBERS
    uint32_t m_instr_ಎಣಿಕೆ = 0;  // Running count of instructions
    ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ m_startNodep;  // Start node of count
    ತಾರ್ಕಿಕ m_tracingCall = ಅಸತ್ಯ;  // Iterating into a CCall to a CFunc
    ತಾರ್ಕಿಕ m_inCFunc = ಅಸತ್ಯ;  // Inside AstCFunc
    ತಾರ್ಕಿಕ m_ignoreRemaining = ಅಸತ್ಯ;  // Ignore remaining statements in the block
    ಧ್ರುವ ತಾರ್ಕಿಕ m_assertNoDups;  // Check for duplicates
    ಧ್ರುವ std::ostream* ಧ್ರುವ m_osp;  // Dump file
    std::unordered_map<ಧ್ರುವ AstCFunc*, uint32_t> m_funcCosts;  // Cost of each function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    ವರ್ಗ VisitBase final {
        // MEMBERS
        uint32_t m_saved_ಎಣಿಕೆ;  // Count before visit applied
        Ast_ಗಂಟು* ಧ್ರುವ m_nodep;  // Node helper is visiting
        Instr_ಎಣಿಕೆ_Visitor* ಧ್ರುವ m_visitor;  // Visitor whose start/endVisitBase are called

    ಸಾರ್ವಜನಿಕ:
        // CONSTRUCTORS
        VisitBase(Instr_ಎಣಿಕೆ_Visitor* visitor, Ast_ಗಂಟು* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_ಎಣಿಕೆ = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_ಎಣಿಕೆ, m_nodep); }

    ಗುಪ್ತ:
        VL_UNCOPYABLE(VisitBase);
    };

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    Instr_ಎಣಿಕೆ_Visitor(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ assertNoDups, std::ostream* osp)
        : m_startNodep{nodep}
        , m_assertNoDups{assertNoDups}
        , m_osp{osp} {
        iterateConstNull(nodep);
    }
    ~Instr_ಎಣಿಕೆ_Visitor() override = ಸ್ವಭಾವ;

    // METHODS
    uint32_t instr_ಎಣಿಕೆ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_instr_ಎಣಿಕೆ; }

ಗುಪ್ತ:
    ಶೂನ್ಯ reset() {
        m_instr_ಎಣಿಕೆ = 0;
        m_ignoreRemaining = ಅಸತ್ಯ;
    }
    uint32_t startVisitBase(Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");
        ಯದಿ (m_assertNoDups && !m_inCFunc) {
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
                            << static_cast<Ast_ಗಂಟು*>(nodep->user1p()));
            nodep->user1p(const_cast<ಶೂನ್ಯ*>(reinterpret_cast<ಧ್ರುವ ಶೂನ್ಯ*>(m_startNodep)));
        }

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ಧ್ರುವ uint32_t saved_ಎಣಿಕೆ = m_instr_ಎಣಿಕೆ;
        m_instr_ಎಣಿಕೆ = nodep->instr_ಎಣಿಕೆ();
        ಪ್ರತ್ಯಾಗಮ saved_ಎಣಿಕೆ;
    }
    ಶೂನ್ಯ endVisitBase(uint32_t saved_ಎಣಿಕೆ, Ast_ಗಂಟು* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::ಎಡ << m_instr_ಎಣಿಕೆ << "  " << nodep);
        markCost(nodep);
        ಯದಿ (!m_ignoreRemaining) m_instr_ಎಣಿಕೆ += saved_ಎಣಿಕೆ;
    }
    ಶೂನ್ಯ markCost(Ast_ಗಂಟು* nodep) {
        ಯದಿ (m_osp) nodep->user2(m_instr_ಎಣಿಕೆ + 1);  // Else don't mark to avoid writeback
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Sel* nodep) override {
        ಯದಿ (m_ignoreRemaining) ಪ್ರತ್ಯಾಗಮ;
        // This covers both AstArraySel and AstWordSel
        //
        // If some vector is a bazillion dwords long, and we're selecting 1
        // dword to read or write from it, our cost should be small.
        //
        // Hence, exclude the child of the AstWordSel from the computation,
        // whose cost scales with the size of the entire (maybe large) vector.
        ಧ್ರುವ VisitBase vb{ಏತತ್, nodep};
        iterateAndNextConstNull(nodep->bitp());
    }
    ಶೂನ್ಯ visit(AstSel* nodep) override {
        ಯದಿ (m_ignoreRemaining) ಪ್ರತ್ಯಾಗಮ;
        // Similar to AstNodeSel above, a small select into a large vector
        // is not expensive. Count the cost of the AstSel itself (scales with
        // its width) and the cost of the lsbp() and widthp() nodes, but not
        // the fromp() node which could be disproportionately large.
        ಧ್ರುವ VisitBase vb{ಏತತ್, nodep};
        iterateAndNextConstNull(nodep->lsbp());
    }
    ಶೂನ್ಯ visit(AstConcat* nodep) override {
        ಯದಿ (m_ignoreRemaining) ಪ್ರತ್ಯಾಗಮ;
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
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_If* nodep) override {
        ಯದಿ (m_ignoreRemaining) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ VisitBase vb{ಏತತ್, nodep};
        iterateAndNextConstNull(nodep->condp());
        ಧ್ರುವ uint32_t saved_ಎಣಿಕೆ = m_instr_ಎಣಿಕೆ;

        UINFO(8, "thensp:");
        reset();
        iterateAndNextConstNull(nodep->thensp());
        uint32_t if_ಎಣಿಕೆ = m_instr_ಎಣಿಕೆ;
        ಯದಿ (nodep->branchPred().unlikely()) if_ಎಣಿಕೆ = 0;

        UINFO(8, "elsesp:");
        reset();
        iterateAndNextConstNull(nodep->elsesp());
        uint32_t else_ಎಣಿಕೆ = m_instr_ಎಣಿಕೆ;
        ಯದಿ (nodep->branchPred().likely()) else_ಎಣಿಕೆ = 0;

        reset();
        ಯದಿ (if_ಎಣಿಕೆ >= else_ಎಣಿಕೆ) {
            m_instr_ಎಣಿಕೆ = saved_ಎಣಿಕೆ + if_ಎಣಿಕೆ;
            ಯದಿ (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } ಅನ್ಯಥಾ {
            m_instr_ಎಣಿಕೆ = saved_ಎಣಿಕೆ + else_ಎಣಿಕೆ;
            ಯದಿ (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    ಶೂನ್ಯ visit(AstCond* nodep) override {
        ಯದಿ (m_ignoreRemaining) ಪ್ರತ್ಯಾಗಮ;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ಧ್ರುವ VisitBase vb{ಏತತ್, nodep};
        iterateAndNextConstNull(nodep->condp());
        ಧ್ರುವ uint32_t saved_ಎಣಿಕೆ = m_instr_ಎಣಿಕೆ;

        UINFO(8, "?");
        reset();
        iterateAndNextConstNull(nodep->thenp());
        ಧ್ರುವ uint32_t if_ಎಣಿಕೆ = m_instr_ಎಣಿಕೆ;

        UINFO(8, ":");
        reset();
        iterateAndNextConstNull(nodep->elsep());
        ಧ್ರುವ uint32_t else_ಎಣಿಕೆ = m_instr_ಎಣಿಕೆ;

        reset();
        ಯದಿ (if_ಎಣಿಕೆ >= else_ಎಣಿಕೆ) {
            m_instr_ಎಣಿಕೆ = saved_ಎಣಿಕೆ + if_ಎಣಿಕೆ;
            ಯದಿ (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } ಅನ್ಯಥಾ {
            m_instr_ಎಣಿಕೆ = saved_ಎಣಿಕೆ + else_ಎಣಿಕೆ;
            ಯದಿ (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    ಶೂನ್ಯ visit(AstCAwait* nodep) override {
        ಯದಿ (m_ignoreRemaining) ಪ್ರತ್ಯಾಗಮ;
        iterateChildrenConst(nodep);
        // Anything past a co_await is irrelevant
        m_ignoreRemaining = ಸತ್ಯ;
    }
    ಶೂನ್ಯ visit(AstFork* nodep) override {
        ಯದಿ (m_ignoreRemaining) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ VisitBase vb{ಏತತ್, nodep};
        iterateAndNextConstNull(nodep->stmtsp());
        uint32_t total_ಎಣಿಕೆ = m_instr_ಎಣಿಕೆ;
        // Sum counts in each statement until the first await
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            reset();
            iterateConst(stmtp);
            total_ಎಣಿಕೆ += m_instr_ಎಣಿಕೆ;
        }
        m_instr_ಎಣಿಕೆ = total_ಎಣಿಕೆ;
        m_ignoreRemaining = ಅಸತ್ಯ;
    }
    ಶೂನ್ಯ visit(AstActive* nodep) override {
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
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_CCall* nodep) override {
        ಯದಿ (m_ignoreRemaining) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ VisitBase vb{ಏತತ್, nodep};
        iterateChildrenConst(nodep);
        ಧ್ರುವ auto it = m_funcCosts.find(nodep->funcp());
        ಯದಿ (it == m_funcCosts.end()) {
            m_tracingCall = ಸತ್ಯ;
            iterateConst(nodep->funcp());
            UASSERT_OBJ(!m_tracingCall, nodep,
                        "visit(AstCFunc) should have cleared m_tracingCall.");
        } ಅನ್ಯಥಾ {
            m_instr_ಎಣಿಕೆ += it->second;  // This guards and adds zero cost on recursive calls
        }
    }
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        UASSERT_OBJ(m_tracingCall || nodep == m_startNodep, nodep,
                    "AstCFunc not under AstCCall, or not start node");
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not be ignoring at the start of a CFunc");
        ಧ್ರುವ auto emplacePair = m_funcCosts.emplace(nodep, 0);
        UASSERT_OBJ(emplacePair.second, nodep, "Should visit every CFunc at most once");
        uint32_t& funcCostr = emplacePair.first->second;
        m_tracingCall = ಅಸತ್ಯ;

        ಧ್ರುವ uint32_t saved_ಎಣಿಕೆ = m_instr_ಎಣಿಕೆ;
        reset();
        {
            VL_RESTORER(m_inCFunc);
            m_inCFunc = ಸತ್ಯ;
            ಧ್ರುವ VisitBase vb{ಏತತ್, nodep};
            iterateChildrenConst(nodep);
        }
        funcCostr = m_instr_ಎಣಿಕೆ;
        m_instr_ಎಣಿಕೆ += saved_ಎಣಿಕೆ;

        m_ignoreRemaining = ಅಸತ್ಯ;
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Cons* nodep) override {
        ಯದಿ (m_ignoreRemaining) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ VisitBase vb{ಏತತ್, nodep};
        // A constant value is not built at run time, so its parts cost nothing
        ಯದಿ (!nodep->is_Constant_ವೃಕ್ಷ()) iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        ಯದಿ (m_ignoreRemaining) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ VisitBase vb{ಏತತ್, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(Instr_ಎಣಿಕೆ_Visitor);
};

// Iterate the graph printing the critical path marked by previous visitation
ವರ್ಗ Instr_ಎಣಿಕೆ_Dump_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> int.  Path cost, 0 means don't dump

    // MEMBERS
    std::ostream* ಧ್ರುವ m_osp;  // Dump file
    unsigned m_depth = 0;  // Current tree depth for printing indent

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    Instr_ಎಣಿಕೆ_Dump_Visitor(Ast_ಗಂಟು* nodep, std::ostream* osp)
        : m_osp{osp} {
        // No check for nullptr output, so...
        UASSERT_OBJ(osp, nodep, "Don't call if not dumping");
        ಯದಿ (nodep) iterateConst(nodep);
    }
    ~Instr_ಎಣಿಕೆ_Dump_Visitor() override = ಸ್ವಭಾವ;

ಗುಪ್ತ:
    // METHODS
    string indent() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ string(m_depth, ':') + " "; }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        ++m_depth;
        ಯದಿ (ಧ್ರುವ unsigned costPlus1 = nodep->user2()) {
            *m_osp << "  " << indent() << "cost " << std::setw(6) << std::ಎಡ << (costPlus1 - 1)
                   << "  " << nodep << '\n';
            iterateChildrenConst(nodep);
        }
        --m_depth;
    }

    VL_UNCOPYABLE(Instr_ಎಣಿಕೆ_Dump_Visitor);
};

uint32_t V3_Instr_ಎಣಿಕೆ::ಎಣಿಕೆ(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ assertNoDups, std::ostream* osp) {
    ಧ್ರುವ Instr_ಎಣಿಕೆ_Visitor visitor{nodep, assertNoDups, osp};
    ಯದಿ (osp) ಧ್ರುವ Instr_ಎಣಿಕೆ_Dump_Visitor dumper{nodep, osp};
    ಪ್ರತ್ಯಾಗಮ visitor.instr_ಎಣಿಕೆ();
}
