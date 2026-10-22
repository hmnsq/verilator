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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3InstrCount.h"

#ଅନ୍ତର୍ଭାଵ <iomanip>
#ଅନ୍ତର୍ଭାଵ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the instruction cost for executing all logic within and below
/// a given AST node. Note this estimates the number of instructions we'll
/// execute, not the number we'll generate. That is, for conditionals,
/// we'll count instructions from either the 'if' or the 'else' branch,
/// whichever is larger. We know we won't run both.

ଶ୍ରେଣୀ Instr_ଗଣନା_Visitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()        -> bool. Processed if assertNoDups
    //  AstNode::user2()        -> uint64_t.  Path cost + 1, 0 means don't dump
    ଅଚଳ VNUser2InUse m_inuser2;

    // MEMBERS
    uint32_t m_instr_ଗଣନା = 0;  // Running count of instructions
    ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ m_startNodep;  // Start node of count
    ତାର୍କିକ m_tracingCall = ମିଥ୍ୟା;  // Iterating into a CCall to a CFunc
    ତାର୍କିକ m_inCFunc = ମିଥ୍ୟା;  // Inside AstCFunc
    ତାର୍କିକ m_ignoreRemaining = ମିଥ୍ୟା;  // Ignore remaining statements in the block
    ଅଚଳ ତାର୍କିକ m_assertNoDups;  // Check for duplicates
    ଅଚଳ std::ostream* ଅଚଳ m_osp;  // Dump file
    std::unordered_map<ଅଚଳ AstCFunc*, uint32_t> m_funcCosts;  // Cost of each function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    ଶ୍ରେଣୀ VisitBase final {
        // MEMBERS
        uint32_t m_saved_ଗଣନା;  // Count before visit applied
        Ast_ଗଣ୍ଠି* ଅଚଳ m_nodep;  // Node helper is visiting
        Instr_ଗଣନା_Visitor* ଅଚଳ m_visitor;  // Visitor whose start/endVisitBase are called

    ସର୍ବସାଧାରଣ:
        // CONSTRUCTORS
        VisitBase(Instr_ଗଣନା_Visitor* visitor, Ast_ଗଣ୍ଠି* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_ଗଣନା = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_ଗଣନା, m_nodep); }

    ବ୍ୟକ୍ତିଗତ:
        VL_UNCOPYABLE(VisitBase);
    };

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    Instr_ଗଣନା_Visitor(Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ assertNoDups, std::ostream* osp)
        : m_startNodep{nodep}
        , m_assertNoDups{assertNoDups}
        , m_osp{osp} {
        iterateConstNull(nodep);
    }
    ~Instr_ଗଣନା_Visitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // METHODS
    uint32_t instr_ଗଣନା() ଅଚଳ { ଫେରନ୍ତୁ m_instr_ଗଣନା; }

ବ୍ୟକ୍ତିଗତ:
    ଶୂନ୍ୟ reset() {
        m_instr_ଗଣନା = 0;
        m_ignoreRemaining = ମିଥ୍ୟା;
    }
    uint32_t startVisitBase(Ast_ଗଣ୍ଠି* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");
        ଯଦି (m_assertNoDups && !m_inCFunc) {
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
                            << static_cast<Ast_ଗଣ୍ଠି*>(nodep->user1p()));
            nodep->user1p(const_cast<ଶୂନ୍ୟ*>(reinterpret_cast<ଅଚଳ ଶୂନ୍ୟ*>(m_startNodep)));
        }

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ଅଚଳ uint32_t saved_ଗଣନା = m_instr_ଗଣନା;
        m_instr_ଗଣନା = nodep->instr_ଗଣନା();
        ଫେରନ୍ତୁ saved_ଗଣନା;
    }
    ଶୂନ୍ୟ endVisitBase(uint32_t saved_ଗଣନା, Ast_ଗଣ୍ଠି* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::ବାମ << m_instr_ଗଣନା << "  " << nodep);
        markCost(nodep);
        ଯଦି (!m_ignoreRemaining) m_instr_ଗଣନା += saved_ଗଣନା;
    }
    ଶୂନ୍ୟ markCost(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (m_osp) nodep->user2(m_instr_ଗଣନା + 1);  // Else don't mark to avoid writeback
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Sel* nodep) override {
        ଯଦି (m_ignoreRemaining) ଫେରନ୍ତୁ;
        // This covers both AstArraySel and AstWordSel
        //
        // If some vector is a bazillion dwords long, and we're selecting 1
        // dword to read or write from it, our cost should be small.
        //
        // Hence, exclude the child of the AstWordSel from the computation,
        // whose cost scales with the size of the entire (maybe large) vector.
        ଅଚଳ VisitBase vb{ଏହି, nodep};
        iterateAndNextConstNull(nodep->bitp());
    }
    ଶୂନ୍ୟ visit(AstSel* nodep) override {
        ଯଦି (m_ignoreRemaining) ଫେରନ୍ତୁ;
        // Similar to AstNodeSel above, a small select into a large vector
        // is not expensive. Count the cost of the AstSel itself (scales with
        // its width) and the cost of the lsbp() and widthp() nodes, but not
        // the fromp() node which could be disproportionately large.
        ଅଚଳ VisitBase vb{ଏହି, nodep};
        iterateAndNextConstNull(nodep->lsbp());
    }
    ଶୂନ୍ୟ visit(AstConcat* nodep) override {
        ଯଦି (m_ignoreRemaining) ଫେରନ୍ତୁ;
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
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_If* nodep) override {
        ଯଦି (m_ignoreRemaining) ଫେରନ୍ତୁ;
        ଅଚଳ VisitBase vb{ଏହି, nodep};
        iterateAndNextConstNull(nodep->condp());
        ଅଚଳ uint32_t saved_ଗଣନା = m_instr_ଗଣନା;

        UINFO(8, "thensp:");
        reset();
        iterateAndNextConstNull(nodep->thensp());
        uint32_t if_ଗଣନା = m_instr_ଗଣନା;
        ଯଦି (nodep->branchPred().unlikely()) if_ଗଣନା = 0;

        UINFO(8, "elsesp:");
        reset();
        iterateAndNextConstNull(nodep->elsesp());
        uint32_t else_ଗଣନା = m_instr_ଗଣନା;
        ଯଦି (nodep->branchPred().likely()) else_ଗଣନା = 0;

        reset();
        ଯଦି (if_ଗଣନା >= else_ଗଣନା) {
            m_instr_ଗଣନା = saved_ଗଣନା + if_ଗଣନା;
            ଯଦି (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } ଅନ୍ୟଥା {
            m_instr_ଗଣନା = saved_ଗଣନା + else_ଗଣନା;
            ଯଦି (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    ଶୂନ୍ୟ visit(AstCond* nodep) override {
        ଯଦି (m_ignoreRemaining) ଫେରନ୍ତୁ;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ଅଚଳ VisitBase vb{ଏହି, nodep};
        iterateAndNextConstNull(nodep->condp());
        ଅଚଳ uint32_t saved_ଗଣନା = m_instr_ଗଣନା;

        UINFO(8, "?");
        reset();
        iterateAndNextConstNull(nodep->thenp());
        ଅଚଳ uint32_t if_ଗଣନା = m_instr_ଗଣନା;

        UINFO(8, ":");
        reset();
        iterateAndNextConstNull(nodep->elsep());
        ଅଚଳ uint32_t else_ଗଣନା = m_instr_ଗଣନା;

        reset();
        ଯଦି (if_ଗଣନା >= else_ଗଣନା) {
            m_instr_ଗଣନା = saved_ଗଣନା + if_ଗଣନା;
            ଯଦି (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } ଅନ୍ୟଥା {
            m_instr_ଗଣନା = saved_ଗଣନା + else_ଗଣନା;
            ଯଦି (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    ଶୂନ୍ୟ visit(AstCAwait* nodep) override {
        ଯଦି (m_ignoreRemaining) ଫେରନ୍ତୁ;
        iterateChildrenConst(nodep);
        // Anything past a co_await is irrelevant
        m_ignoreRemaining = ସତ୍ୟ;
    }
    ଶୂନ୍ୟ visit(AstFork* nodep) override {
        ଯଦି (m_ignoreRemaining) ଫେରନ୍ତୁ;
        ଅଚଳ VisitBase vb{ଏହି, nodep};
        iterateAndNextConstNull(nodep->stmtsp());
        uint32_t total_ଗଣନା = m_instr_ଗଣନା;
        // Sum counts in each statement until the first await
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            reset();
            iterateConst(stmtp);
            total_ଗଣନା += m_instr_ଗଣନା;
        }
        m_instr_ଗଣନା = total_ଗଣନା;
        m_ignoreRemaining = ମିଥ୍ୟା;
    }
    ଶୂନ୍ୟ visit(AstActive* nodep) override {
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
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_CCall* nodep) override {
        ଯଦି (m_ignoreRemaining) ଫେରନ୍ତୁ;
        ଅଚଳ VisitBase vb{ଏହି, nodep};
        iterateChildrenConst(nodep);
        ଅଚଳ auto it = m_funcCosts.find(nodep->funcp());
        ଯଦି (it == m_funcCosts.end()) {
            m_tracingCall = ସତ୍ୟ;
            iterateConst(nodep->funcp());
            UASSERT_OBJ(!m_tracingCall, nodep,
                        "visit(AstCFunc) should have cleared m_tracingCall.");
        } ଅନ୍ୟଥା {
            m_instr_ଗଣନା += it->second;  // This guards and adds zero cost on recursive calls
        }
    }
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        UASSERT_OBJ(m_tracingCall || nodep == m_startNodep, nodep,
                    "AstCFunc not under AstCCall, or not start node");
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not be ignoring at the start of a CFunc");
        ଅଚଳ auto emplacePair = m_funcCosts.emplace(nodep, 0);
        UASSERT_OBJ(emplacePair.second, nodep, "Should visit every CFunc at most once");
        uint32_t& funcCostr = emplacePair.first->second;
        m_tracingCall = ମିଥ୍ୟା;

        ଅଚଳ uint32_t saved_ଗଣନା = m_instr_ଗଣନା;
        reset();
        {
            VL_RESTORER(m_inCFunc);
            m_inCFunc = ସତ୍ୟ;
            ଅଚଳ VisitBase vb{ଏହି, nodep};
            iterateChildrenConst(nodep);
        }
        funcCostr = m_instr_ଗଣନା;
        m_instr_ଗଣନା += saved_ଗଣନା;

        m_ignoreRemaining = ମିଥ୍ୟା;
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Cons* nodep) override {
        ଯଦି (m_ignoreRemaining) ଫେରନ୍ତୁ;
        ଅଚଳ VisitBase vb{ଏହି, nodep};
        // A constant value is not built at run time, so its parts cost nothing
        ଯଦି (!nodep->is_Constant_ବୃକ୍ଷ()) iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        ଯଦି (m_ignoreRemaining) ଫେରନ୍ତୁ;
        ଅଚଳ VisitBase vb{ଏହି, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(Instr_ଗଣନା_Visitor);
};

// Iterate the graph printing the critical path marked by previous visitation
ଶ୍ରେଣୀ Instr_ଗଣନା_Dump_Visitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> int.  Path cost, 0 means don't dump

    // MEMBERS
    std::ostream* ଅଚଳ m_osp;  // Dump file
    unsigned m_depth = 0;  // Current tree depth for printing indent

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    Instr_ଗଣନା_Dump_Visitor(Ast_ଗଣ୍ଠି* nodep, std::ostream* osp)
        : m_osp{osp} {
        // No check for nullptr output, so...
        UASSERT_OBJ(osp, nodep, "Don't call if not dumping");
        ଯଦି (nodep) iterateConst(nodep);
    }
    ~Instr_ଗଣନା_Dump_Visitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

ବ୍ୟକ୍ତିଗତ:
    // METHODS
    string indent() ଅଚଳ { ଫେରନ୍ତୁ string(m_depth, ':') + " "; }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        ++m_depth;
        ଯଦି (ଅଚଳ unsigned costPlus1 = nodep->user2()) {
            *m_osp << "  " << indent() << "cost " << std::setw(6) << std::ବାମ << (costPlus1 - 1)
                   << "  " << nodep << '\n';
            iterateChildrenConst(nodep);
        }
        --m_depth;
    }

    VL_UNCOPYABLE(Instr_ଗଣନା_Dump_Visitor);
};

uint32_t V3_Instr_ଗଣନା::ଗଣନା_2(Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ assertNoDups, std::ostream* osp) {
    ଅଚଳ Instr_ଗଣନା_Visitor visitor{nodep, assertNoDups, osp};
    ଯଦି (osp) ଅଚଳ Instr_ଗଣନା_Dump_Visitor dumper{nodep, osp};
    ଫେରନ୍ତୁ visitor.instr_ଗଣନା();
}
