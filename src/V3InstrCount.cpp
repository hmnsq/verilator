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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3InstrCount.h"

#അന്തര്ഭാവ <iomanip>
#അന്തര്ഭാവ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the instruction cost for executing all logic within and below
/// a given AST node. Note this estimates the number of instructions we'll
/// execute, not the number we'll generate. That is, for conditionals,
/// we'll count instructions from either the 'if' or the 'else' branch,
/// whichever is larger. We know we won't run both.

വർഗ്ഗം Instr_എണ്ണം_Visitor final : പൊതു VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()        -> bool. Processed if assertNoDups
    //  AstNode::user2()        -> uint64_t.  Path cost + 1, 0 means don't dump
    സ്ഥിരം VNUser2InUse m_inuser2;

    // MEMBERS
    uint32_t m_instr_എണ്ണം = 0;  // Running count of instructions
    സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം m_startNodep;  // Start node of count
    തർക്കം m_tracingCall = അസത്യം;  // Iterating into a CCall to a CFunc
    തർക്കം m_inCFunc = അസത്യം;  // Inside AstCFunc
    തർക്കം m_ignoreRemaining = അസത്യം;  // Ignore remaining statements in the block
    സ്ഥിരം തർക്കം m_assertNoDups;  // Check for duplicates
    സ്ഥിരം std::ostream* സ്ഥിരം m_osp;  // Dump file
    std::unordered_map<സ്ഥിരം AstCFunc*, uint32_t> m_funcCosts;  // Cost of each function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    വർഗ്ഗം VisitBase final {
        // MEMBERS
        uint32_t m_saved_എണ്ണം;  // Count before visit applied
        Ast_കണ്ണി* സ്ഥിരം m_nodep;  // Node helper is visiting
        Instr_എണ്ണം_Visitor* സ്ഥിരം m_visitor;  // Visitor whose start/endVisitBase are called

    പൊതു:
        // CONSTRUCTORS
        VisitBase(Instr_എണ്ണം_Visitor* visitor, Ast_കണ്ണി* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_എണ്ണം = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_എണ്ണം, m_nodep); }

    സ്വകാര്യ:
        VL_UNCOPYABLE(VisitBase);
    };

പൊതു:
    // CONSTRUCTORS
    Instr_എണ്ണം_Visitor(Ast_കണ്ണി* nodep, തർക്കം assertNoDups, std::ostream* osp)
        : m_startNodep{nodep}
        , m_assertNoDups{assertNoDups}
        , m_osp{osp} {
        iterateConstNull(nodep);
    }
    ~Instr_എണ്ണം_Visitor() override = സ്ഥിരസ്ഥിതി;

    // METHODS
    uint32_t instr_എണ്ണം() സ്ഥിരം { തിരികെ m_instr_എണ്ണം; }

സ്വകാര്യ:
    ശൂന്യം reset() {
        m_instr_എണ്ണം = 0;
        m_ignoreRemaining = അസത്യം;
    }
    uint32_t startVisitBase(Ast_കണ്ണി* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");
        എങ്കിൽ (m_assertNoDups && !m_inCFunc) {
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
                            << static_cast<Ast_കണ്ണി*>(nodep->user1p()));
            nodep->user1p(const_cast<ശൂന്യം*>(reinterpret_cast<സ്ഥിരം ശൂന്യം*>(m_startNodep)));
        }

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        സ്ഥിരം uint32_t saved_എണ്ണം = m_instr_എണ്ണം;
        m_instr_എണ്ണം = nodep->instr_എണ്ണം();
        തിരികെ saved_എണ്ണം;
    }
    ശൂന്യം endVisitBase(uint32_t saved_എണ്ണം, Ast_കണ്ണി* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::ഇടത് << m_instr_എണ്ണം << "  " << nodep);
        markCost(nodep);
        എങ്കിൽ (!m_ignoreRemaining) m_instr_എണ്ണം += saved_എണ്ണം;
    }
    ശൂന്യം markCost(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (m_osp) nodep->user2(m_instr_എണ്ണം + 1);  // Else don't mark to avoid writeback
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Sel* nodep) override {
        എങ്കിൽ (m_ignoreRemaining) തിരികെ;
        // This covers both AstArraySel and AstWordSel
        //
        // If some vector is a bazillion dwords long, and we're selecting 1
        // dword to read or write from it, our cost should be small.
        //
        // Hence, exclude the child of the AstWordSel from the computation,
        // whose cost scales with the size of the entire (maybe large) vector.
        സ്ഥിരം VisitBase vb{ഇത്, nodep};
        iterateAndNextConstNull(nodep->bitp());
    }
    ശൂന്യം visit(AstSel* nodep) override {
        എങ്കിൽ (m_ignoreRemaining) തിരികെ;
        // Similar to AstNodeSel above, a small select into a large vector
        // is not expensive. Count the cost of the AstSel itself (scales with
        // its width) and the cost of the lsbp() and widthp() nodes, but not
        // the fromp() node which could be disproportionately large.
        സ്ഥിരം VisitBase vb{ഇത്, nodep};
        iterateAndNextConstNull(nodep->lsbp());
    }
    ശൂന്യം visit(AstConcat* nodep) override {
        എങ്കിൽ (m_ignoreRemaining) തിരികെ;
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
    ശൂന്യം visit(Ast_കണ്ണി_If* nodep) override {
        എങ്കിൽ (m_ignoreRemaining) തിരികെ;
        സ്ഥിരം VisitBase vb{ഇത്, nodep};
        iterateAndNextConstNull(nodep->condp());
        സ്ഥിരം uint32_t saved_എണ്ണം = m_instr_എണ്ണം;

        UINFO(8, "thensp:");
        reset();
        iterateAndNextConstNull(nodep->thensp());
        uint32_t if_എണ്ണം = m_instr_എണ്ണം;
        എങ്കിൽ (nodep->branchPred().unlikely()) if_എണ്ണം = 0;

        UINFO(8, "elsesp:");
        reset();
        iterateAndNextConstNull(nodep->elsesp());
        uint32_t else_എണ്ണം = m_instr_എണ്ണം;
        എങ്കിൽ (nodep->branchPred().likely()) else_എണ്ണം = 0;

        reset();
        എങ്കിൽ (if_എണ്ണം >= else_എണ്ണം) {
            m_instr_എണ്ണം = saved_എണ്ണം + if_എണ്ണം;
            എങ്കിൽ (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } അല്ലെങ്കിൽ {
            m_instr_എണ്ണം = saved_എണ്ണം + else_എണ്ണം;
            എങ്കിൽ (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    ശൂന്യം visit(AstCond* nodep) override {
        എങ്കിൽ (m_ignoreRemaining) തിരികെ;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        സ്ഥിരം VisitBase vb{ഇത്, nodep};
        iterateAndNextConstNull(nodep->condp());
        സ്ഥിരം uint32_t saved_എണ്ണം = m_instr_എണ്ണം;

        UINFO(8, "?");
        reset();
        iterateAndNextConstNull(nodep->thenp());
        സ്ഥിരം uint32_t if_എണ്ണം = m_instr_എണ്ണം;

        UINFO(8, ":");
        reset();
        iterateAndNextConstNull(nodep->elsep());
        സ്ഥിരം uint32_t else_എണ്ണം = m_instr_എണ്ണം;

        reset();
        എങ്കിൽ (if_എണ്ണം >= else_എണ്ണം) {
            m_instr_എണ്ണം = saved_എണ്ണം + if_എണ്ണം;
            എങ്കിൽ (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } അല്ലെങ്കിൽ {
            m_instr_എണ്ണം = saved_എണ്ണം + else_എണ്ണം;
            എങ്കിൽ (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    ശൂന്യം visit(AstCAwait* nodep) override {
        എങ്കിൽ (m_ignoreRemaining) തിരികെ;
        iterateChildrenConst(nodep);
        // Anything past a co_await is irrelevant
        m_ignoreRemaining = സത്യം;
    }
    ശൂന്യം visit(AstFork* nodep) override {
        എങ്കിൽ (m_ignoreRemaining) തിരികെ;
        സ്ഥിരം VisitBase vb{ഇത്, nodep};
        iterateAndNextConstNull(nodep->stmtsp());
        uint32_t total_എണ്ണം = m_instr_എണ്ണം;
        // Sum counts in each statement until the first await
        വേണ്ടി (Ast_കണ്ണി* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            reset();
            iterateConst(stmtp);
            total_എണ്ണം += m_instr_എണ്ണം;
        }
        m_instr_എണ്ണം = total_എണ്ണം;
        m_ignoreRemaining = അസത്യം;
    }
    ശൂന്യം visit(AstActive* nodep) override {
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
    ശൂന്യം visit(Ast_കണ്ണി_CCall* nodep) override {
        എങ്കിൽ (m_ignoreRemaining) തിരികെ;
        സ്ഥിരം VisitBase vb{ഇത്, nodep};
        iterateChildrenConst(nodep);
        സ്ഥിരം auto it = m_funcCosts.find(nodep->funcp());
        എങ്കിൽ (it == m_funcCosts.end()) {
            m_tracingCall = സത്യം;
            iterateConst(nodep->funcp());
            UASSERT_OBJ(!m_tracingCall, nodep,
                        "visit(AstCFunc) should have cleared m_tracingCall.");
        } അല്ലെങ്കിൽ {
            m_instr_എണ്ണം += it->second;  // This guards and adds zero cost on recursive calls
        }
    }
    ശൂന്യം visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        UASSERT_OBJ(m_tracingCall || nodep == m_startNodep, nodep,
                    "AstCFunc not under AstCCall, or not start node");
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not be ignoring at the start of a CFunc");
        സ്ഥിരം auto emplacePair = m_funcCosts.emplace(nodep, 0);
        UASSERT_OBJ(emplacePair.second, nodep, "Should visit every CFunc at most once");
        uint32_t& funcCostr = emplacePair.first->second;
        m_tracingCall = അസത്യം;

        സ്ഥിരം uint32_t saved_എണ്ണം = m_instr_എണ്ണം;
        reset();
        {
            VL_RESTORER(m_inCFunc);
            m_inCFunc = സത്യം;
            സ്ഥിരം VisitBase vb{ഇത്, nodep};
            iterateChildrenConst(nodep);
        }
        funcCostr = m_instr_എണ്ണം;
        m_instr_എണ്ണം += saved_എണ്ണം;

        m_ignoreRemaining = അസത്യം;
    }
    ശൂന്യം visit(Ast_കണ്ണി_Cons* nodep) override {
        എങ്കിൽ (m_ignoreRemaining) തിരികെ;
        സ്ഥിരം VisitBase vb{ഇത്, nodep};
        // A constant value is not built at run time, so its parts cost nothing
        എങ്കിൽ (!nodep->is_Constant_വൃക്ഷം()) iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        എങ്കിൽ (m_ignoreRemaining) തിരികെ;
        സ്ഥിരം VisitBase vb{ഇത്, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(Instr_എണ്ണം_Visitor);
};

// Iterate the graph printing the critical path marked by previous visitation
വർഗ്ഗം Instr_എണ്ണം_Dump_Visitor final : പൊതു VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> int.  Path cost, 0 means don't dump

    // MEMBERS
    std::ostream* സ്ഥിരം m_osp;  // Dump file
    unsigned m_depth = 0;  // Current tree depth for printing indent

പൊതു:
    // CONSTRUCTORS
    Instr_എണ്ണം_Dump_Visitor(Ast_കണ്ണി* nodep, std::ostream* osp)
        : m_osp{osp} {
        // No check for nullptr output, so...
        UASSERT_OBJ(osp, nodep, "Don't call if not dumping");
        എങ്കിൽ (nodep) iterateConst(nodep);
    }
    ~Instr_എണ്ണം_Dump_Visitor() override = സ്ഥിരസ്ഥിതി;

സ്വകാര്യ:
    // METHODS
    string indent() സ്ഥിരം { തിരികെ string(m_depth, ':') + " "; }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        ++m_depth;
        എങ്കിൽ (സ്ഥിരം unsigned costPlus1 = nodep->user2()) {
            *m_osp << "  " << indent() << "cost " << std::setw(6) << std::ഇടത് << (costPlus1 - 1)
                   << "  " << nodep << '\n';
            iterateChildrenConst(nodep);
        }
        --m_depth;
    }

    VL_UNCOPYABLE(Instr_എണ്ണം_Dump_Visitor);
};

uint32_t V3_Instr_എണ്ണം::എണ്ണം(Ast_കണ്ണി* nodep, തർക്കം assertNoDups, std::ostream* osp) {
    സ്ഥിരം Instr_എണ്ണം_Visitor visitor{nodep, assertNoDups, osp};
    എങ്കിൽ (osp) സ്ഥിരം Instr_എണ്ണം_Dump_Visitor dumper{nodep, osp};
    തിരികെ visitor.instr_എണ്ണം();
}
