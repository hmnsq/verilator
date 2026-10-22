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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3InstrCount.h"

#अन्तर्भाव <iomanip>
#अन्तर्भाव <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the instruction cost for executing all logic within and below
/// a given AST node. Note this estimates the number of instructions we'll
/// execute, not the number we'll generate. That is, for conditionals,
/// we'll count instructions from either the 'if' or the 'else' branch,
/// whichever is larger. We know we won't run both.

वर्ग Instr_गणनम्_Visitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()        -> bool. Processed if assertNoDups
    //  AstNode::user2()        -> uint64_t.  Path cost + 1, 0 means don't dump
    ध्रुव VNUser2_अन्तर्_Use m_inuser2;

    // MEMBERS
    uint32_t m_instr_गणनम् = 0;  // Running count of instructions
    ध्रुव Ast_ग्रन्थिः* ध्रुव m_startNodep;  // Start node of count
    तार्किक m_tracingCall = असत्य;  // Iterating into a CCall to a CFunc
    तार्किक m_अन्तर्_CFunc = असत्य;  // Inside AstCFunc
    तार्किक m_ignoreRemaining = असत्य;  // Ignore remaining statements in the block
    ध्रुव तार्किक m_assertNoDups;  // Check for duplicates
    ध्रुव std::ostream* ध्रुव m_osp;  // Dump file
    std::unordered_map<ध्रुव AstCFunc*, uint32_t> m_funcCosts;  // Cost of each function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    वर्ग दर्शय_आधारः final {
        // MEMBERS
        uint32_t m_saved_गणनम्;  // Count before visit applied
        Ast_ग्रन्थिः* ध्रुव m_nodep;  // Node helper is visiting
        Instr_गणनम्_Visitor* ध्रुव m_visitor;  // Visitor whose start/endVisitBase are called

    सार्वजनिक:
        // CONSTRUCTORS
        दर्शय_आधारः(Instr_गणनम्_Visitor* visitor, Ast_ग्रन्थिः* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_गणनम् = m_visitor->start_दर्शय_आधारः(nodep);
        }
        ~दर्शय_आधारः() { m_visitor->अन्तः_दर्शय_आधारः(m_saved_गणनम्, m_nodep); }

    गुप्त:
        VL_UNCOPYABLE(दर्शय_आधारः);
    };

सार्वजनिक:
    // CONSTRUCTORS
    Instr_गणनम्_Visitor(Ast_ग्रन्थिः* nodep, तार्किक assertNoDups, std::ostream* osp)
        : m_startNodep{nodep}
        , m_assertNoDups{assertNoDups}
        , m_osp{osp} {
        iterateConstNull(nodep);
    }
    ~Instr_गणनम्_Visitor() override = स्वभाव;

    // METHODS
    uint32_t instr_गणनम्() ध्रुव { प्रत्यागम m_instr_गणनम्; }

गुप्त:
    शून्य reset() {
        m_instr_गणनम् = 0;
        m_ignoreRemaining = असत्य;
    }
    uint32_t start_दर्शय_आधारः(Ast_ग्रन्थिः* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");
        यदि (m_assertNoDups && !m_अन्तर्_CFunc) {
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
                            << static_cast<Ast_ग्रन्थिः*>(nodep->user1p()));
            nodep->user1p(const_cast<शून्य*>(reinterpret_cast<ध्रुव शून्य*>(m_startNodep)));
        }

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ध्रुव uint32_t saved_गणनम् = m_instr_गणनम्;
        m_instr_गणनम् = nodep->instr_गणनम्();
        प्रत्यागम saved_गणनम्;
    }
    शून्य अन्तः_दर्शय_आधारः(uint32_t saved_गणनम्, Ast_ग्रन्थिः* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::वामतः << m_instr_गणनम् << "  " << nodep);
        markCost(nodep);
        यदि (!m_ignoreRemaining) m_instr_गणनम् += saved_गणनम्;
    }
    शून्य markCost(Ast_ग्रन्थिः* nodep) {
        यदि (m_osp) nodep->user2(m_instr_गणनम् + 1);  // Else don't mark to avoid writeback
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_Sel* nodep) override {
        यदि (m_ignoreRemaining) प्रत्यागम;
        // This covers both AstArraySel and AstWordSel
        //
        // If some vector is a bazillion dwords long, and we're selecting 1
        // dword to read or write from it, our cost should be small.
        //
        // Hence, exclude the child of the AstWordSel from the computation,
        // whose cost scales with the size of the entire (maybe large) vector.
        ध्रुव दर्शय_आधारः vb{एतत्, nodep};
        iterate_And_अग्र_Const_Null(nodep->bitp());
    }
    शून्य दर्शय(AstSel* nodep) override {
        यदि (m_ignoreRemaining) प्रत्यागम;
        // Similar to AstNodeSel above, a small select into a large vector
        // is not expensive. Count the cost of the AstSel itself (scales with
        // its width) and the cost of the lsbp() and widthp() nodes, but not
        // the fromp() node which could be disproportionately large.
        ध्रुव दर्शय_आधारः vb{एतत्, nodep};
        iterate_And_अग्र_Const_Null(nodep->lsbp());
    }
    शून्य दर्शय(AstConcat* nodep) override {
        यदि (m_ignoreRemaining) प्रत्यागम;
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
    शून्य दर्शय(Ast_ग्रन्थिः_If* nodep) override {
        यदि (m_ignoreRemaining) प्रत्यागम;
        ध्रुव दर्शय_आधारः vb{एतत्, nodep};
        iterate_And_अग्र_Const_Null(nodep->condp());
        ध्रुव uint32_t saved_गणनम् = m_instr_गणनम्;

        UINFO(8, "thensp:");
        reset();
        iterate_And_अग्र_Const_Null(nodep->thensp());
        uint32_t if_गणनम् = m_instr_गणनम्;
        यदि (nodep->branchPred().unlikely()) if_गणनम् = 0;

        UINFO(8, "elsesp:");
        reset();
        iterate_And_अग्र_Const_Null(nodep->elsesp());
        uint32_t else_गणनम् = m_instr_गणनम्;
        यदि (nodep->branchPred().likely()) else_गणनम् = 0;

        reset();
        यदि (if_गणनम् >= else_गणनम्) {
            m_instr_गणनम् = saved_गणनम् + if_गणनम्;
            यदि (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } अन्यथा {
            m_instr_गणनम् = saved_गणनम् + else_गणनम्;
            यदि (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    शून्य दर्शय(AstCond* nodep) override {
        यदि (m_ignoreRemaining) प्रत्यागम;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ध्रुव दर्शय_आधारः vb{एतत्, nodep};
        iterate_And_अग्र_Const_Null(nodep->condp());
        ध्रुव uint32_t saved_गणनम् = m_instr_गणनम्;

        UINFO(8, "?");
        reset();
        iterate_And_अग्र_Const_Null(nodep->thenp());
        ध्रुव uint32_t if_गणनम् = m_instr_गणनम्;

        UINFO(8, ":");
        reset();
        iterate_And_अग्र_Const_Null(nodep->elsep());
        ध्रुव uint32_t else_गणनम् = m_instr_गणनम्;

        reset();
        यदि (if_गणनम् >= else_गणनम्) {
            m_instr_गणनम् = saved_गणनम् + if_गणनम्;
            यदि (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } अन्यथा {
            m_instr_गणनम् = saved_गणनम् + else_गणनम्;
            यदि (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    शून्य दर्शय(AstCAwait* nodep) override {
        यदि (m_ignoreRemaining) प्रत्यागम;
        iterateChildrenConst(nodep);
        // Anything past a co_await is irrelevant
        m_ignoreRemaining = सत्य;
    }
    शून्य दर्शय(AstFork* nodep) override {
        यदि (m_ignoreRemaining) प्रत्यागम;
        ध्रुव दर्शय_आधारः vb{एतत्, nodep};
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        uint32_t योग_गणनम् = m_instr_गणनम्;
        // Sum counts in each statement until the first await
        प्रति (Ast_ग्रन्थिः* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            reset();
            iterateConst(stmtp);
            योग_गणनम् += m_instr_गणनम्;
        }
        m_instr_गणनम् = योग_गणनम्;
        m_ignoreRemaining = असत्य;
    }
    शून्य दर्शय(Ast_सक्रिय* nodep) override {
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
    शून्य दर्शय(Ast_ग्रन्थिः_CCall* nodep) override {
        यदि (m_ignoreRemaining) प्रत्यागम;
        ध्रुव दर्शय_आधारः vb{एतत्, nodep};
        iterateChildrenConst(nodep);
        ध्रुव स्वचालित it = m_funcCosts.find(nodep->funcp());
        यदि (it == m_funcCosts.अन्तः_2()) {
            m_tracingCall = सत्य;
            iterateConst(nodep->funcp());
            UASSERT_OBJ(!m_tracingCall, nodep,
                        "visit(AstCFunc) should have cleared m_tracingCall.");
        } अन्यथा {
            m_instr_गणनम् += it->second;  // This guards and adds zero cost on recursive calls
        }
    }
    शून्य दर्शय(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        UASSERT_OBJ(m_tracingCall || nodep == m_startNodep, nodep,
                    "AstCFunc not under AstCCall, or not start node");
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not be ignoring at the start of a CFunc");
        ध्रुव स्वचालित emplacePair = m_funcCosts.emplace(nodep, 0);
        UASSERT_OBJ(emplacePair.second, nodep, "Should visit every CFunc at most once");
        uint32_t& funcCostr = emplacePair.first->second;
        m_tracingCall = असत्य;

        ध्रुव uint32_t saved_गणनम् = m_instr_गणनम्;
        reset();
        {
            VL_RESTORER(m_अन्तर्_CFunc);
            m_अन्तर्_CFunc = सत्य;
            ध्रुव दर्शय_आधारः vb{एतत्, nodep};
            iterateChildrenConst(nodep);
        }
        funcCostr = m_instr_गणनम्;
        m_instr_गणनम् += saved_गणनम्;

        m_ignoreRemaining = असत्य;
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Cons* nodep) override {
        यदि (m_ignoreRemaining) प्रत्यागम;
        ध्रुव दर्शय_आधारः vb{एतत्, nodep};
        // A constant value is not built at run time, so its parts cost nothing
        यदि (!nodep->is_Constant_वृक्षः()) iterateChildrenConst(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        यदि (m_ignoreRemaining) प्रत्यागम;
        ध्रुव दर्शय_आधारः vb{एतत्, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(Instr_गणनम्_Visitor);
};

// Iterate the graph printing the critical path marked by previous visitation
वर्ग Instr_गणनम्_Dump_Visitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> int.  Path cost, 0 means don't dump

    // MEMBERS
    std::ostream* ध्रुव m_osp;  // Dump file
    अचिह्नित m_गभीरता = 0;  // Current tree depth for printing indent

सार्वजनिक:
    // CONSTRUCTORS
    Instr_गणनम्_Dump_Visitor(Ast_ग्रन्थिः* nodep, std::ostream* osp)
        : m_osp{osp} {
        // No check for nullptr output, so...
        UASSERT_OBJ(osp, nodep, "Don't call if not dumping");
        यदि (nodep) iterateConst(nodep);
    }
    ~Instr_गणनम्_Dump_Visitor() override = स्वभाव;

गुप्त:
    // METHODS
    string indent() ध्रुव { प्रत्यागम string(m_गभीरता, ':') + " "; }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        ++m_गभीरता;
        यदि (ध्रुव अचिह्नित costPlus1 = nodep->user2()) {
            *m_osp << "  " << indent() << "cost " << std::setw(6) << std::वामतः << (costPlus1 - 1)
                   << "  " << nodep << '\n';
            iterateChildrenConst(nodep);
        }
        --m_गभीरता;
    }

    VL_UNCOPYABLE(Instr_गणनम्_Dump_Visitor);
};

uint32_t V3_Instr_गणनम्::गणनम्(Ast_ग्रन्थिः* nodep, तार्किक assertNoDups, std::ostream* osp) {
    ध्रुव Instr_गणनम्_Visitor visitor{nodep, assertNoDups, osp};
    यदि (osp) ध्रुव Instr_गणनम्_Dump_Visitor dumper{nodep, osp};
    प्रत्यागम visitor.instr_गणनम्();
}
