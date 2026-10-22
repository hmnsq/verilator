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
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;

    // MEMBERS
    uint32_t m_instr_ಎಣಿಕೆ = 0;  // Running count of instructions
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ m_ಆರಂಭ_Nodep;  // Start node of count
    ತಾರ್ಕಿಕ m_tracingCall = ಸುಳ್ಳು;  // Iterating into a CCall to a CFunc
    ತಾರ್ಕಿಕ m_ಒಳ_CFunc = ಸುಳ್ಳು;  // Inside AstCFunc
    ತಾರ್ಕಿಕ m_ignore_ಉಳಿದ = ಸುಳ್ಳು;  // Ignore remaining statements in the block
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_assertNoDups;  // Check for duplicates
    ಸ್ಥಿರಾಂಕ std::ostream* ಸ್ಥಿರಾಂಕ m_osp;  // Dump file
    std::unordered_map<ಸ್ಥಿರಾಂಕ AstCFunc*, uint32_t> m_funcCosts;  // Cost of each function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    ವರ್ಗ ಭೇಟಿ_ತಳ final {
        // MEMBERS
        uint32_t m_saved_ಎಣಿಕೆ;  // Count before visit applied
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ m_nodep;  // Node helper is visiting
        Instr_ಎಣಿಕೆ_Visitor* ಸ್ಥಿರಾಂಕ m_visitor;  // Visitor whose start/endVisitBase are called

    ಸಾರ್ವಜನಿಕ:
        // CONSTRUCTORS
        ಭೇಟಿ_ತಳ(Instr_ಎಣಿಕೆ_Visitor* visitor, Ast_ಗಂಟು* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_ಎಣಿಕೆ = m_visitor->ಆರಂಭ_ಭೇಟಿ_ತಳ(nodep);
        }
        ~ಭೇಟಿ_ತಳ() { m_visitor->ಕೊನೆ_ಭೇಟಿ_ತಳ(m_saved_ಎಣಿಕೆ, m_nodep); }

    ಖಾಸಗಿ:
        VL_UNCOPYABLE(ಭೇಟಿ_ತಳ);
    };

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    Instr_ಎಣಿಕೆ_Visitor(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ assertNoDups, std::ostream* osp)
        : m_ಆರಂಭ_Nodep{nodep}
        , m_assertNoDups{assertNoDups}
        , m_osp{osp} {
        iterateConstNull(nodep);
    }
    ~Instr_ಎಣಿಕೆ_Visitor() override = ಪೂರ್ವನಿಯೋಜಿತ;

    // METHODS
    uint32_t instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_instr_ಎಣಿಕೆ; }

ಖಾಸಗಿ:
    ಶೂನ್ಯ reset() {
        m_instr_ಎಣಿಕೆ = 0;
        m_ignore_ಉಳಿದ = ಸುಳ್ಳು;
    }
    uint32_t ಆರಂಭ_ಭೇಟಿ_ತಳ(Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(!m_ignore_ಉಳಿದ, nodep, "Should not reach here if ignoring");
        ಆದರೆ (m_assertNoDups && !m_ಒಳ_CFunc) {
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
            nodep->user1p(const_cast<ಶೂನ್ಯ*>(reinterpret_cast<ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ*>(m_ಆರಂಭ_Nodep)));
        }

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ಸ್ಥಿರಾಂಕ uint32_t saved_ಎಣಿಕೆ = m_instr_ಎಣಿಕೆ;
        m_instr_ಎಣಿಕೆ = nodep->instr_ಎಣಿಕೆ();
        ಹಿಂತಿರುಗಿ saved_ಎಣಿಕೆ;
    }
    ಶೂನ್ಯ ಕೊನೆ_ಭೇಟಿ_ತಳ(uint32_t saved_ಎಣಿಕೆ, Ast_ಗಂಟು* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::ಎಡ << m_instr_ಎಣಿಕೆ << "  " << nodep);
        mark_ವೆಚ್ಚ(nodep);
        ಆದರೆ (!m_ignore_ಉಳಿದ) m_instr_ಎಣಿಕೆ += saved_ಎಣಿಕೆ;
    }
    ಶೂನ್ಯ mark_ವೆಚ್ಚ(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (m_osp) nodep->user2(m_instr_ಎಣಿಕೆ + 1);  // Else don't mark to avoid writeback
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Sel* nodep) override {
        ಆದರೆ (m_ignore_ಉಳಿದ) ಹಿಂತಿರುಗಿ;
        // This covers both AstArraySel and AstWordSel
        //
        // If some vector is a bazillion dwords long, and we're selecting 1
        // dword to read or write from it, our cost should be small.
        //
        // Hence, exclude the child of the AstWordSel from the computation,
        // whose cost scales with the size of the entire (maybe large) vector.
        ಸ್ಥಿರಾಂಕ ಭೇಟಿ_ತಳ vb{ಇದು, nodep};
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->bitp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSel* nodep) override {
        ಆದರೆ (m_ignore_ಉಳಿದ) ಹಿಂತಿರುಗಿ;
        // Similar to AstNodeSel above, a small select into a large vector
        // is not expensive. Count the cost of the AstSel itself (scales with
        // its width) and the cost of the lsbp() and widthp() nodes, but not
        // the fromp() node which could be disproportionately large.
        ಸ್ಥಿರಾಂಕ ಭೇಟಿ_ತಳ vb{ಇದು, nodep};
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->lsbp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConcat* nodep) override {
        ಆದರೆ (m_ignore_ಉಳಿದ) ಹಿಂತಿರುಗಿ;
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
        mark_ವೆಚ್ಚ(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_If* nodep) override {
        ಆದರೆ (m_ignore_ಉಳಿದ) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ ಭೇಟಿ_ತಳ vb{ಇದು, nodep};
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->condp());
        ಸ್ಥಿರಾಂಕ uint32_t saved_ಎಣಿಕೆ = m_instr_ಎಣಿಕೆ;

        UINFO(8, "thensp:");
        reset();
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->thensp());
        uint32_t if_ಎಣಿಕೆ = m_instr_ಎಣಿಕೆ;
        ಆದರೆ (nodep->branchPred().unlikely()) if_ಎಣಿಕೆ = 0;

        UINFO(8, "elsesp:");
        reset();
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->elsesp());
        uint32_t else_ಎಣಿಕೆ = m_instr_ಎಣಿಕೆ;
        ಆದರೆ (nodep->branchPred().likely()) else_ಎಣಿಕೆ = 0;

        reset();
        ಆದರೆ (if_ಎಣಿಕೆ >= else_ಎಣಿಕೆ) {
            m_instr_ಎಣಿಕೆ = saved_ಎಣಿಕೆ + if_ಎಣಿಕೆ;
            ಆದರೆ (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_instr_ಎಣಿಕೆ = saved_ಎಣಿಕೆ + else_ಎಣಿಕೆ;
            ಆದರೆ (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCond* nodep) override {
        ಆದರೆ (m_ignore_ಉಳಿದ) ಹಿಂತಿರುಗಿ;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ಸ್ಥಿರಾಂಕ ಭೇಟಿ_ತಳ vb{ಇದು, nodep};
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->condp());
        ಸ್ಥಿರಾಂಕ uint32_t saved_ಎಣಿಕೆ = m_instr_ಎಣಿಕೆ;

        UINFO(8, "?");
        reset();
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->thenp());
        ಸ್ಥಿರಾಂಕ uint32_t if_ಎಣಿಕೆ = m_instr_ಎಣಿಕೆ;

        UINFO(8, ":");
        reset();
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->elsep());
        ಸ್ಥಿರಾಂಕ uint32_t else_ಎಣಿಕೆ = m_instr_ಎಣಿಕೆ;

        reset();
        ಆದರೆ (if_ಎಣಿಕೆ >= else_ಎಣಿಕೆ) {
            m_instr_ಎಣಿಕೆ = saved_ಎಣಿಕೆ + if_ಎಣಿಕೆ;
            ಆದರೆ (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_instr_ಎಣಿಕೆ = saved_ಎಣಿಕೆ + else_ಎಣಿಕೆ;
            ಆದರೆ (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCAwait* nodep) override {
        ಆದರೆ (m_ignore_ಉಳಿದ) ಹಿಂತಿರುಗಿ;
        iterateChildrenConst(nodep);
        // Anything past a co_await is irrelevant
        m_ignore_ಉಳಿದ = ನಿಜ;
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFork* nodep) override {
        ಆದರೆ (m_ignore_ಉಳಿದ) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ ಭೇಟಿ_ತಳ vb{ಇದು, nodep};
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
        uint32_t ಒಟ್ಟು_ಎಣಿಕೆ = m_instr_ಎಣಿಕೆ;
        // Sum counts in each statement until the first await
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            reset();
            iterateConst(stmtp);
            ಒಟ್ಟು_ಎಣಿಕೆ += m_instr_ಎಣಿಕೆ;
        }
        m_instr_ಎಣಿಕೆ = ಒಟ್ಟು_ಎಣಿಕೆ;
        m_ignore_ಉಳಿದ = ಸುಳ್ಳು;
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಕ್ರಿಯ* nodep) override {
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
        mark_ವೆಚ್ಚ(nodep);
        UASSERT_OBJ(nodep == m_ಆರಂಭ_Nodep, nodep, "Multiple actives, or not start node");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_CCall* nodep) override {
        ಆದರೆ (m_ignore_ಉಳಿದ) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ ಭೇಟಿ_ತಳ vb{ಇದು, nodep};
        iterateChildrenConst(nodep);
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_funcCosts.ಕಂಡುಹಿಡಿ(nodep->funcp());
        ಆದರೆ (it == m_funcCosts.ಕೊನೆ()) {
            m_tracingCall = ನಿಜ;
            iterateConst(nodep->funcp());
            UASSERT_OBJ(!m_tracingCall, nodep,
                        "visit(AstCFunc) should have cleared m_tracingCall.");
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_instr_ಎಣಿಕೆ += it->ಎರಡನೇ;  // This guards and adds zero cost on recursive calls
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        UASSERT_OBJ(m_tracingCall || nodep == m_ಆರಂಭ_Nodep, nodep,
                    "AstCFunc not under AstCCall, or not start node");
        UASSERT_OBJ(!m_ignore_ಉಳಿದ, nodep, "Should not be ignoring at the start of a CFunc");
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ emplacePair = m_funcCosts.emplace(nodep, 0);
        UASSERT_OBJ(emplacePair.ಎರಡನೇ, nodep, "Should visit every CFunc at most once");
        uint32_t& funcCostr = emplacePair.ಮೊದಲ->ಎರಡನೇ;
        m_tracingCall = ಸುಳ್ಳು;

        ಸ್ಥಿರಾಂಕ uint32_t saved_ಎಣಿಕೆ = m_instr_ಎಣಿಕೆ;
        reset();
        {
            VL_RESTORER(m_ಒಳ_CFunc);
            m_ಒಳ_CFunc = ನಿಜ;
            ಸ್ಥಿರಾಂಕ ಭೇಟಿ_ತಳ vb{ಇದು, nodep};
            iterateChildrenConst(nodep);
        }
        funcCostr = m_instr_ಎಣಿಕೆ;
        m_instr_ಎಣಿಕೆ += saved_ಎಣಿಕೆ;

        m_ignore_ಉಳಿದ = ಸುಳ್ಳು;
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Cons* nodep) override {
        ಆದರೆ (m_ignore_ಉಳಿದ) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ ಭೇಟಿ_ತಳ vb{ಇದು, nodep};
        // A constant value is not built at run time, so its parts cost nothing
        ಆದರೆ (!nodep->is_Constant_ವೃಕ್ಷ()) iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        ಆದರೆ (m_ignore_ಉಳಿದ) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ ಭೇಟಿ_ತಳ vb{ಇದು, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(Instr_ಎಣಿಕೆ_Visitor);
};

// Iterate the graph printing the critical path marked by previous visitation
ವರ್ಗ Instr_ಎಣಿಕೆ_Dump_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> int.  Path cost, 0 means don't dump

    // MEMBERS
    std::ostream* ಸ್ಥಿರಾಂಕ m_osp;  // Dump file
    ಚಿಹ್ನೆರಹಿತ m_ಆಳ = 0;  // Current tree depth for printing indent

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    Instr_ಎಣಿಕೆ_Dump_Visitor(Ast_ಗಂಟು* nodep, std::ostream* osp)
        : m_osp{osp} {
        // No check for nullptr output, so...
        UASSERT_OBJ(osp, nodep, "Don't call if not dumping");
        ಆದರೆ (nodep) iterateConst(nodep);
    }
    ~Instr_ಎಣಿಕೆ_Dump_Visitor() override = ಪೂರ್ವನಿಯೋಜಿತ;

ಖಾಸಗಿ:
    // METHODS
    string indent() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ string(m_ಆಳ, ':') + " "; }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        ++m_ಆಳ;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ ವೆಚ್ಚ_Plus1 = nodep->user2()) {
            *m_osp << "  " << indent() << "cost " << std::setw(6) << std::ಎಡ << (ವೆಚ್ಚ_Plus1 - 1)
                   << "  " << nodep << '\n';
            iterateChildrenConst(nodep);
        }
        --m_ಆಳ;
    }

    VL_UNCOPYABLE(Instr_ಎಣಿಕೆ_Dump_Visitor);
};

uint32_t V3_Instr_ಎಣಿಕೆ::ಎಣಿಕೆ_2(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ assertNoDups, std::ostream* osp) {
    ಸ್ಥಿರಾಂಕ Instr_ಎಣಿಕೆ_Visitor visitor{nodep, assertNoDups, osp};
    ಆದರೆ (osp) ಸ್ಥಿರಾಂಕ Instr_ಎಣಿಕೆ_Dump_Visitor dumper{nodep, osp};
    ಹಿಂತಿರುಗಿ visitor.instr_ಎಣಿಕೆ();
}
