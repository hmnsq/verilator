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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3InstrCount.h"

#அந்தர்பாவ <iomanip>
#அந்தர்பாவ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the instruction cost for executing all logic within and below
/// a given AST node. Note this estimates the number of instructions we'll
/// execute, not the number we'll generate. That is, for conditionals,
/// we'll count instructions from either the 'if' or the 'else' branch,
/// whichever is larger. We know we won't run both.

வகுப்பு Instr_மொத்தம்_Visitor final : பொது VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()        -> bool. Processed if assertNoDups
    //  AstNode::user2()        -> uint64_t.  Path cost + 1, 0 means don't dump
    மாறிலி VNUser2InUse m_inuser2;

    // MEMBERS
    uint32_t m_instr_மொத்தம் = 0;  // Running count of instructions
    மாறிலி Ast_முனை* மாறிலி m_startNodep;  // Start node of count
    தருக்க m_tracingCall = பொய்;  // Iterating into a CCall to a CFunc
    தருக்க m_inCFunc = பொய்;  // Inside AstCFunc
    தருக்க m_ignoreRemaining = பொய்;  // Ignore remaining statements in the block
    மாறிலி தருக்க m_assertNoDups;  // Check for duplicates
    மாறிலி std::ostream* மாறிலி m_osp;  // Dump file
    std::unordered_map<மாறிலி AstCFunc*, uint32_t> m_funcCosts;  // Cost of each function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    வகுப்பு VisitBase final {
        // MEMBERS
        uint32_t m_saved_மொத்தம்;  // Count before visit applied
        Ast_முனை* மாறிலி m_nodep;  // Node helper is visiting
        Instr_மொத்தம்_Visitor* மாறிலி m_visitor;  // Visitor whose start/endVisitBase are called

    பொது:
        // CONSTRUCTORS
        VisitBase(Instr_மொத்தம்_Visitor* visitor, Ast_முனை* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_மொத்தம் = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_மொத்தம், m_nodep); }

    தனிப்பட்ட:
        VL_UNCOPYABLE(VisitBase);
    };

பொது:
    // CONSTRUCTORS
    Instr_மொத்தம்_Visitor(Ast_முனை* nodep, தருக்க assertNoDups, std::ostream* osp)
        : m_startNodep{nodep}
        , m_assertNoDups{assertNoDups}
        , m_osp{osp} {
        iterateConstNull(nodep);
    }
    ~Instr_மொத்தம்_Visitor() override = இயல்பு;

    // METHODS
    uint32_t instr_மொத்தம்() மாறிலி { திரும்பு m_instr_மொத்தம்; }

தனிப்பட்ட:
    வெறுமை reset() {
        m_instr_மொத்தம் = 0;
        m_ignoreRemaining = பொய்;
    }
    uint32_t startVisitBase(Ast_முனை* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");
        எனில் (m_assertNoDups && !m_inCFunc) {
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
                            << static_cast<Ast_முனை*>(nodep->user1p()));
            nodep->user1p(const_cast<வெறுமை*>(reinterpret_cast<மாறிலி வெறுமை*>(m_startNodep)));
        }

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        மாறிலி uint32_t saved_மொத்தம் = m_instr_மொத்தம்;
        m_instr_மொத்தம் = nodep->instr_மொத்தம்();
        திரும்பு saved_மொத்தம்;
    }
    வெறுமை endVisitBase(uint32_t saved_மொத்தம், Ast_முனை* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::இடது << m_instr_மொத்தம் << "  " << nodep);
        markCost(nodep);
        எனில் (!m_ignoreRemaining) m_instr_மொத்தம் += saved_மொத்தம்;
    }
    வெறுமை markCost(Ast_முனை* nodep) {
        எனில் (m_osp) nodep->user2(m_instr_மொத்தம் + 1);  // Else don't mark to avoid writeback
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Sel* nodep) override {
        எனில் (m_ignoreRemaining) திரும்பு;
        // This covers both AstArraySel and AstWordSel
        //
        // If some vector is a bazillion dwords long, and we're selecting 1
        // dword to read or write from it, our cost should be small.
        //
        // Hence, exclude the child of the AstWordSel from the computation,
        // whose cost scales with the size of the entire (maybe large) vector.
        மாறிலி VisitBase vb{இது, nodep};
        iterateAndNextConstNull(nodep->bitp());
    }
    வெறுமை visit(AstSel* nodep) override {
        எனில் (m_ignoreRemaining) திரும்பு;
        // Similar to AstNodeSel above, a small select into a large vector
        // is not expensive. Count the cost of the AstSel itself (scales with
        // its width) and the cost of the lsbp() and widthp() nodes, but not
        // the fromp() node which could be disproportionately large.
        மாறிலி VisitBase vb{இது, nodep};
        iterateAndNextConstNull(nodep->lsbp());
    }
    வெறுமை visit(AstConcat* nodep) override {
        எனில் (m_ignoreRemaining) திரும்பு;
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
    வெறுமை visit(Ast_முனை_If* nodep) override {
        எனில் (m_ignoreRemaining) திரும்பு;
        மாறிலி VisitBase vb{இது, nodep};
        iterateAndNextConstNull(nodep->condp());
        மாறிலி uint32_t saved_மொத்தம் = m_instr_மொத்தம்;

        UINFO(8, "thensp:");
        reset();
        iterateAndNextConstNull(nodep->thensp());
        uint32_t if_மொத்தம் = m_instr_மொத்தம்;
        எனில் (nodep->branchPred().unlikely()) if_மொத்தம் = 0;

        UINFO(8, "elsesp:");
        reset();
        iterateAndNextConstNull(nodep->elsesp());
        uint32_t else_மொத்தம் = m_instr_மொத்தம்;
        எனில் (nodep->branchPred().likely()) else_மொத்தம் = 0;

        reset();
        எனில் (if_மொத்தம் >= else_மொத்தம்) {
            m_instr_மொத்தம் = saved_மொத்தம் + if_மொத்தம்;
            எனில் (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } இல்லையெனில் {
            m_instr_மொத்தம் = saved_மொத்தம் + else_மொத்தம்;
            எனில் (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    வெறுமை visit(AstCond* nodep) override {
        எனில் (m_ignoreRemaining) திரும்பு;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        மாறிலி VisitBase vb{இது, nodep};
        iterateAndNextConstNull(nodep->condp());
        மாறிலி uint32_t saved_மொத்தம் = m_instr_மொத்தம்;

        UINFO(8, "?");
        reset();
        iterateAndNextConstNull(nodep->thenp());
        மாறிலி uint32_t if_மொத்தம் = m_instr_மொத்தம்;

        UINFO(8, ":");
        reset();
        iterateAndNextConstNull(nodep->elsep());
        மாறிலி uint32_t else_மொத்தம் = m_instr_மொத்தம்;

        reset();
        எனில் (if_மொத்தம் >= else_மொத்தம்) {
            m_instr_மொத்தம் = saved_மொத்தம் + if_மொத்தம்;
            எனில் (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } இல்லையெனில் {
            m_instr_மொத்தம் = saved_மொத்தம் + else_மொத்தம்;
            எனில் (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    வெறுமை visit(AstCAwait* nodep) override {
        எனில் (m_ignoreRemaining) திரும்பு;
        iterateChildrenConst(nodep);
        // Anything past a co_await is irrelevant
        m_ignoreRemaining = உண்மை;
    }
    வெறுமை visit(AstFork* nodep) override {
        எனில் (m_ignoreRemaining) திரும்பு;
        மாறிலி VisitBase vb{இது, nodep};
        iterateAndNextConstNull(nodep->stmtsp());
        uint32_t total_மொத்தம் = m_instr_மொத்தம்;
        // Sum counts in each statement until the first await
        க்கு (Ast_முனை* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            reset();
            iterateConst(stmtp);
            total_மொத்தம் += m_instr_மொத்தம்;
        }
        m_instr_மொத்தம் = total_மொத்தம்;
        m_ignoreRemaining = பொய்;
    }
    வெறுமை visit(AstActive* nodep) override {
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
    வெறுமை visit(Ast_முனை_CCall* nodep) override {
        எனில் (m_ignoreRemaining) திரும்பு;
        மாறிலி VisitBase vb{இது, nodep};
        iterateChildrenConst(nodep);
        மாறிலி auto it = m_funcCosts.find(nodep->funcp());
        எனில் (it == m_funcCosts.end()) {
            m_tracingCall = உண்மை;
            iterateConst(nodep->funcp());
            UASSERT_OBJ(!m_tracingCall, nodep,
                        "visit(AstCFunc) should have cleared m_tracingCall.");
        } இல்லையெனில் {
            m_instr_மொத்தம் += it->second;  // This guards and adds zero cost on recursive calls
        }
    }
    வெறுமை visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        UASSERT_OBJ(m_tracingCall || nodep == m_startNodep, nodep,
                    "AstCFunc not under AstCCall, or not start node");
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not be ignoring at the start of a CFunc");
        மாறிலி auto emplacePair = m_funcCosts.emplace(nodep, 0);
        UASSERT_OBJ(emplacePair.second, nodep, "Should visit every CFunc at most once");
        uint32_t& funcCostr = emplacePair.first->second;
        m_tracingCall = பொய்;

        மாறிலி uint32_t saved_மொத்தம் = m_instr_மொத்தம்;
        reset();
        {
            VL_RESTORER(m_inCFunc);
            m_inCFunc = உண்மை;
            மாறிலி VisitBase vb{இது, nodep};
            iterateChildrenConst(nodep);
        }
        funcCostr = m_instr_மொத்தம்;
        m_instr_மொத்தம் += saved_மொத்தம்;

        m_ignoreRemaining = பொய்;
    }
    வெறுமை visit(Ast_முனை_Cons* nodep) override {
        எனில் (m_ignoreRemaining) திரும்பு;
        மாறிலி VisitBase vb{இது, nodep};
        // A constant value is not built at run time, so its parts cost nothing
        எனில் (!nodep->is_Constant_மரம்()) iterateChildrenConst(nodep);
    }
    வெறுமை visit(Ast_முனை* nodep) override {
        எனில் (m_ignoreRemaining) திரும்பு;
        மாறிலி VisitBase vb{இது, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(Instr_மொத்தம்_Visitor);
};

// Iterate the graph printing the critical path marked by previous visitation
வகுப்பு Instr_மொத்தம்_Dump_Visitor final : பொது VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> int.  Path cost, 0 means don't dump

    // MEMBERS
    std::ostream* மாறிலி m_osp;  // Dump file
    unsigned m_depth = 0;  // Current tree depth for printing indent

பொது:
    // CONSTRUCTORS
    Instr_மொத்தம்_Dump_Visitor(Ast_முனை* nodep, std::ostream* osp)
        : m_osp{osp} {
        // No check for nullptr output, so...
        UASSERT_OBJ(osp, nodep, "Don't call if not dumping");
        எனில் (nodep) iterateConst(nodep);
    }
    ~Instr_மொத்தம்_Dump_Visitor() override = இயல்பு;

தனிப்பட்ட:
    // METHODS
    string indent() மாறிலி { திரும்பு string(m_depth, ':') + " "; }
    வெறுமை visit(Ast_முனை* nodep) override {
        ++m_depth;
        எனில் (மாறிலி unsigned costPlus1 = nodep->user2()) {
            *m_osp << "  " << indent() << "cost " << std::setw(6) << std::இடது << (costPlus1 - 1)
                   << "  " << nodep << '\n';
            iterateChildrenConst(nodep);
        }
        --m_depth;
    }

    VL_UNCOPYABLE(Instr_மொத்தம்_Dump_Visitor);
};

uint32_t V3_Instr_மொத்தம்::மொத்தம்(Ast_முனை* nodep, தருக்க assertNoDups, std::ostream* osp) {
    மாறிலி Instr_மொத்தம்_Visitor visitor{nodep, assertNoDups, osp};
    எனில் (osp) மாறிலி Instr_மொத்தம்_Dump_Visitor dumper{nodep, osp};
    திரும்பு visitor.instr_மொத்தம்();
}
