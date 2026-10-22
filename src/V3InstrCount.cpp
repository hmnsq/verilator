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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3InstrCount.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <iomanip>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the instruction cost for executing all logic within and below
/// a given AST node. Note this estimates the number of instructions we'll
/// execute, not the number we'll generate. That is, for conditionals,
/// we'll count instructions from either the 'if' or the 'else' branch,
/// whichever is larger. We know we won't run both.

ਵਰਗ Instr_ਗਿਣਤੀ_Visitor final : ਜਨਤਕ VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()        -> bool. Processed if assertNoDups
    //  AstNode::user2()        -> uint64_t.  Path cost + 1, 0 means don't dump
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;

    // MEMBERS
    uint32_t m_instr_ਗਿਣਤੀ = 0;  // Running count of instructions
    ਅਚਲ Ast_ਗੰਢ* ਅਚਲ m_ਸ਼ੁਰੂ_Nodep;  // Start node of count
    ਤਾਰਕਿਕ m_tracingCall = ਝੂਠ;  // Iterating into a CCall to a CFunc
    ਤਾਰਕਿਕ m_ਵਿੱਚ_CFunc = ਝੂਠ;  // Inside AstCFunc
    ਤਾਰਕਿਕ m_ignore_ਬਚਿਆ = ਝੂਠ;  // Ignore remaining statements in the block
    ਅਚਲ ਤਾਰਕਿਕ m_assertNoDups;  // Check for duplicates
    ਅਚਲ std::ostream* ਅਚਲ m_osp;  // Dump file
    std::unordered_map<ਅਚਲ AstCFunc*, uint32_t> m_funcCosts;  // Cost of each function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    ਵਰਗ ਦੇਖੋ_ਅਧਾਰ final {
        // MEMBERS
        uint32_t m_saved_ਗਿਣਤੀ;  // Count before visit applied
        Ast_ਗੰਢ* ਅਚਲ m_nodep;  // Node helper is visiting
        Instr_ਗਿਣਤੀ_Visitor* ਅਚਲ m_visitor;  // Visitor whose start/endVisitBase are called

    ਜਨਤਕ:
        // CONSTRUCTORS
        ਦੇਖੋ_ਅਧਾਰ(Instr_ਗਿਣਤੀ_Visitor* visitor, Ast_ਗੰਢ* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_ਗਿਣਤੀ = m_visitor->ਸ਼ੁਰੂ_ਦੇਖੋ_ਅਧਾਰ(nodep);
        }
        ~ਦੇਖੋ_ਅਧਾਰ() { m_visitor->ਸਿਰਾ_ਦੇਖੋ_ਅਧਾਰ(m_saved_ਗਿਣਤੀ, m_nodep); }

    ਨਿਜੀ:
        VL_UNCOPYABLE(ਦੇਖੋ_ਅਧਾਰ);
    };

ਜਨਤਕ:
    // CONSTRUCTORS
    Instr_ਗਿਣਤੀ_Visitor(Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ assertNoDups, std::ostream* osp)
        : m_ਸ਼ੁਰੂ_Nodep{nodep}
        , m_assertNoDups{assertNoDups}
        , m_osp{osp} {
        iterateConstNull(nodep);
    }
    ~Instr_ਗਿਣਤੀ_Visitor() override = ਮੂਲ;

    // METHODS
    uint32_t instr_ਗਿਣਤੀ() ਅਚਲ { ਵਾਪਸ m_instr_ਗਿਣਤੀ; }

ਨਿਜੀ:
    ਖਾਲੀ reset() {
        m_instr_ਗਿਣਤੀ = 0;
        m_ignore_ਬਚਿਆ = ਝੂਠ;
    }
    uint32_t ਸ਼ੁਰੂ_ਦੇਖੋ_ਅਧਾਰ(Ast_ਗੰਢ* nodep) {
        UASSERT_OBJ(!m_ignore_ਬਚਿਆ, nodep, "Should not reach here if ignoring");
        ਜੇ (m_assertNoDups && !m_ਵਿੱਚ_CFunc) {
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
                            << static_cast<Ast_ਗੰਢ*>(nodep->user1p()));
            nodep->user1p(const_cast<ਖਾਲੀ*>(reinterpret_cast<ਅਚਲ ਖਾਲੀ*>(m_ਸ਼ੁਰੂ_Nodep)));
        }

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ਅਚਲ uint32_t saved_ਗਿਣਤੀ = m_instr_ਗਿਣਤੀ;
        m_instr_ਗਿਣਤੀ = nodep->instr_ਗਿਣਤੀ();
        ਵਾਪਸ saved_ਗਿਣਤੀ;
    }
    ਖਾਲੀ ਸਿਰਾ_ਦੇਖੋ_ਅਧਾਰ(uint32_t saved_ਗਿਣਤੀ, Ast_ਗੰਢ* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::ਖੱਬੇ << m_instr_ਗਿਣਤੀ << "  " << nodep);
        mark_ਲਾਗਤ(nodep);
        ਜੇ (!m_ignore_ਬਚਿਆ) m_instr_ਗਿਣਤੀ += saved_ਗਿਣਤੀ;
    }
    ਖਾਲੀ mark_ਲਾਗਤ(Ast_ਗੰਢ* nodep) {
        ਜੇ (m_osp) nodep->user2(m_instr_ਗਿਣਤੀ + 1);  // Else don't mark to avoid writeback
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Sel* nodep) override {
        ਜੇ (m_ignore_ਬਚਿਆ) ਵਾਪਸ;
        // This covers both AstArraySel and AstWordSel
        //
        // If some vector is a bazillion dwords long, and we're selecting 1
        // dword to read or write from it, our cost should be small.
        //
        // Hence, exclude the child of the AstWordSel from the computation,
        // whose cost scales with the size of the entire (maybe large) vector.
        ਅਚਲ ਦੇਖੋ_ਅਧਾਰ vb{ਇਹ, nodep};
        iterate_And_ਅਗਲਾ_Const_Null(nodep->bitp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstSel* nodep) override {
        ਜੇ (m_ignore_ਬਚਿਆ) ਵਾਪਸ;
        // Similar to AstNodeSel above, a small select into a large vector
        // is not expensive. Count the cost of the AstSel itself (scales with
        // its width) and the cost of the lsbp() and widthp() nodes, but not
        // the fromp() node which could be disproportionately large.
        ਅਚਲ ਦੇਖੋ_ਅਧਾਰ vb{ਇਹ, nodep};
        iterate_And_ਅਗਲਾ_Const_Null(nodep->lsbp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstConcat* nodep) override {
        ਜੇ (m_ignore_ਬਚਿਆ) ਵਾਪਸ;
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
        mark_ਲਾਗਤ(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_If* nodep) override {
        ਜੇ (m_ignore_ਬਚਿਆ) ਵਾਪਸ;
        ਅਚਲ ਦੇਖੋ_ਅਧਾਰ vb{ਇਹ, nodep};
        iterate_And_ਅਗਲਾ_Const_Null(nodep->condp());
        ਅਚਲ uint32_t saved_ਗਿਣਤੀ = m_instr_ਗਿਣਤੀ;

        UINFO(8, "thensp:");
        reset();
        iterate_And_ਅਗਲਾ_Const_Null(nodep->thensp());
        uint32_t if_ਗਿਣਤੀ = m_instr_ਗਿਣਤੀ;
        ਜੇ (nodep->branchPred().unlikely()) if_ਗਿਣਤੀ = 0;

        UINFO(8, "elsesp:");
        reset();
        iterate_And_ਅਗਲਾ_Const_Null(nodep->elsesp());
        uint32_t else_ਗਿਣਤੀ = m_instr_ਗਿਣਤੀ;
        ਜੇ (nodep->branchPred().likely()) else_ਗਿਣਤੀ = 0;

        reset();
        ਜੇ (if_ਗਿਣਤੀ >= else_ਗਿਣਤੀ) {
            m_instr_ਗਿਣਤੀ = saved_ਗਿਣਤੀ + if_ਗਿਣਤੀ;
            ਜੇ (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } ਨਹੀਂਤਾਂ {
            m_instr_ਗਿਣਤੀ = saved_ਗਿਣਤੀ + else_ਗਿਣਤੀ;
            ਜੇ (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCond* nodep) override {
        ਜੇ (m_ignore_ਬਚਿਆ) ਵਾਪਸ;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ਅਚਲ ਦੇਖੋ_ਅਧਾਰ vb{ਇਹ, nodep};
        iterate_And_ਅਗਲਾ_Const_Null(nodep->condp());
        ਅਚਲ uint32_t saved_ਗਿਣਤੀ = m_instr_ਗਿਣਤੀ;

        UINFO(8, "?");
        reset();
        iterate_And_ਅਗਲਾ_Const_Null(nodep->thenp());
        ਅਚਲ uint32_t if_ਗਿਣਤੀ = m_instr_ਗਿਣਤੀ;

        UINFO(8, ":");
        reset();
        iterate_And_ਅਗਲਾ_Const_Null(nodep->elsep());
        ਅਚਲ uint32_t else_ਗਿਣਤੀ = m_instr_ਗਿਣਤੀ;

        reset();
        ਜੇ (if_ਗਿਣਤੀ >= else_ਗਿਣਤੀ) {
            m_instr_ਗਿਣਤੀ = saved_ਗਿਣਤੀ + if_ਗਿਣਤੀ;
            ਜੇ (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } ਨਹੀਂਤਾਂ {
            m_instr_ਗਿਣਤੀ = saved_ਗਿਣਤੀ + else_ਗਿਣਤੀ;
            ਜੇ (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCAwait* nodep) override {
        ਜੇ (m_ignore_ਬਚਿਆ) ਵਾਪਸ;
        iterateChildrenConst(nodep);
        // Anything past a co_await is irrelevant
        m_ignore_ਬਚਿਆ = ਸੱਚ;
    }
    ਖਾਲੀ ਦੇਖੋ(AstFork* nodep) override {
        ਜੇ (m_ignore_ਬਚਿਆ) ਵਾਪਸ;
        ਅਚਲ ਦੇਖੋ_ਅਧਾਰ vb{ਇਹ, nodep};
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
        uint32_t ਕੁੱਲ_ਗਿਣਤੀ = m_instr_ਗਿਣਤੀ;
        // Sum counts in each statement until the first await
        ਲਈ (Ast_ਗੰਢ* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            reset();
            iterateConst(stmtp);
            ਕੁੱਲ_ਗਿਣਤੀ += m_instr_ਗਿਣਤੀ;
        }
        m_instr_ਗਿਣਤੀ = ਕੁੱਲ_ਗਿਣਤੀ;
        m_ignore_ਬਚਿਆ = ਝੂਠ;
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਰਗਰਮ* nodep) override {
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
        mark_ਲਾਗਤ(nodep);
        UASSERT_OBJ(nodep == m_ਸ਼ੁਰੂ_Nodep, nodep, "Multiple actives, or not start node");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_CCall* nodep) override {
        ਜੇ (m_ignore_ਬਚਿਆ) ਵਾਪਸ;
        ਅਚਲ ਦੇਖੋ_ਅਧਾਰ vb{ਇਹ, nodep};
        iterateChildrenConst(nodep);
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_funcCosts.ਲੱਭੋ(nodep->funcp());
        ਜੇ (it == m_funcCosts.ਸਿਰਾ()) {
            m_tracingCall = ਸੱਚ;
            iterateConst(nodep->funcp());
            UASSERT_OBJ(!m_tracingCall, nodep,
                        "visit(AstCFunc) should have cleared m_tracingCall.");
        } ਨਹੀਂਤਾਂ {
            m_instr_ਗਿਣਤੀ += it->ਦੂਜਾ;  // This guards and adds zero cost on recursive calls
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        UASSERT_OBJ(m_tracingCall || nodep == m_ਸ਼ੁਰੂ_Nodep, nodep,
                    "AstCFunc not under AstCCall, or not start node");
        UASSERT_OBJ(!m_ignore_ਬਚਿਆ, nodep, "Should not be ignoring at the start of a CFunc");
        ਅਚਲ ਸਵੈਚਾਲਿਤ emplacePair = m_funcCosts.emplace(nodep, 0);
        UASSERT_OBJ(emplacePair.ਦੂਜਾ, nodep, "Should visit every CFunc at most once");
        uint32_t& funcCostr = emplacePair.ਪਹਿਲਾ->ਦੂਜਾ;
        m_tracingCall = ਝੂਠ;

        ਅਚਲ uint32_t saved_ਗਿਣਤੀ = m_instr_ਗਿਣਤੀ;
        reset();
        {
            VL_RESTORER(m_ਵਿੱਚ_CFunc);
            m_ਵਿੱਚ_CFunc = ਸੱਚ;
            ਅਚਲ ਦੇਖੋ_ਅਧਾਰ vb{ਇਹ, nodep};
            iterateChildrenConst(nodep);
        }
        funcCostr = m_instr_ਗਿਣਤੀ;
        m_instr_ਗਿਣਤੀ += saved_ਗਿਣਤੀ;

        m_ignore_ਬਚਿਆ = ਝੂਠ;
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Cons* nodep) override {
        ਜੇ (m_ignore_ਬਚਿਆ) ਵਾਪਸ;
        ਅਚਲ ਦੇਖੋ_ਅਧਾਰ vb{ਇਹ, nodep};
        // A constant value is not built at run time, so its parts cost nothing
        ਜੇ (!nodep->is_Constant_ਰੁੱਖ()) iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        ਜੇ (m_ignore_ਬਚਿਆ) ਵਾਪਸ;
        ਅਚਲ ਦੇਖੋ_ਅਧਾਰ vb{ਇਹ, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(Instr_ਗਿਣਤੀ_Visitor);
};

// Iterate the graph printing the critical path marked by previous visitation
ਵਰਗ Instr_ਗਿਣਤੀ_Dump_Visitor final : ਜਨਤਕ VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> int.  Path cost, 0 means don't dump

    // MEMBERS
    std::ostream* ਅਚਲ m_osp;  // Dump file
    ਬਿਨਾਨਿਸ਼ਾਨੀ m_ਡੂੰਘਾਈ = 0;  // Current tree depth for printing indent

ਜਨਤਕ:
    // CONSTRUCTORS
    Instr_ਗਿਣਤੀ_Dump_Visitor(Ast_ਗੰਢ* nodep, std::ostream* osp)
        : m_osp{osp} {
        // No check for nullptr output, so...
        UASSERT_OBJ(osp, nodep, "Don't call if not dumping");
        ਜੇ (nodep) iterateConst(nodep);
    }
    ~Instr_ਗਿਣਤੀ_Dump_Visitor() override = ਮੂਲ;

ਨਿਜੀ:
    // METHODS
    string indent() ਅਚਲ { ਵਾਪਸ string(m_ਡੂੰਘਾਈ, ':') + " "; }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        ++m_ਡੂੰਘਾਈ;
        ਜੇ (ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ ਲਾਗਤ_Plus1 = nodep->user2()) {
            *m_osp << "  " << indent() << "cost " << std::setw(6) << std::ਖੱਬੇ << (ਲਾਗਤ_Plus1 - 1)
                   << "  " << nodep << '\n';
            iterateChildrenConst(nodep);
        }
        --m_ਡੂੰਘਾਈ;
    }

    VL_UNCOPYABLE(Instr_ਗਿਣਤੀ_Dump_Visitor);
};

uint32_t V3_Instr_ਗਿਣਤੀ::ਗਿਣਤੀ_2(Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ assertNoDups, std::ostream* osp) {
    ਅਚਲ Instr_ਗਿਣਤੀ_Visitor visitor{nodep, assertNoDups, osp};
    ਜੇ (osp) ਅਚਲ Instr_ਗਿਣਤੀ_Dump_Visitor dumper{nodep, osp};
    ਵਾਪਸ visitor.instr_ਗਿਣਤੀ();
}
