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

वर्ग Instr_मोजणी_Visitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()        -> bool. Processed if assertNoDups
    //  AstNode::user2()        -> uint64_t.  Path cost + 1, 0 means don't dump
    स्थिरांक VNUser2InUse m_inuser2;

    // MEMBERS
    uint32_t m_instr_मोजणी = 0;  // Running count of instructions
    स्थिरांक Ast_गाठ* स्थिरांक m_startNodep;  // Start node of count
    तार्किक m_tracingCall = असत्य;  // Iterating into a CCall to a CFunc
    तार्किक m_inCFunc = असत्य;  // Inside AstCFunc
    तार्किक m_ignoreRemaining = असत्य;  // Ignore remaining statements in the block
    स्थिरांक तार्किक m_assertNoDups;  // Check for duplicates
    स्थिरांक std::ostream* स्थिरांक m_osp;  // Dump file
    std::unordered_map<स्थिरांक AstCFunc*, uint32_t> m_funcCosts;  // Cost of each function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    वर्ग VisitBase final {
        // MEMBERS
        uint32_t m_saved_मोजणी;  // Count before visit applied
        Ast_गाठ* स्थिरांक m_nodep;  // Node helper is visiting
        Instr_मोजणी_Visitor* स्थिरांक m_visitor;  // Visitor whose start/endVisitBase are called

    सार्वजनिक:
        // CONSTRUCTORS
        VisitBase(Instr_मोजणी_Visitor* visitor, Ast_गाठ* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_मोजणी = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_मोजणी, m_nodep); }

    खाजगी:
        VL_UNCOPYABLE(VisitBase);
    };

सार्वजनिक:
    // CONSTRUCTORS
    Instr_मोजणी_Visitor(Ast_गाठ* nodep, तार्किक assertNoDups, std::ostream* osp)
        : m_startNodep{nodep}
        , m_assertNoDups{assertNoDups}
        , m_osp{osp} {
        iterateConstNull(nodep);
    }
    ~Instr_मोजणी_Visitor() override = पूर्वनिर्धारित;

    // METHODS
    uint32_t instr_मोजणी() स्थिरांक { परत m_instr_मोजणी; }

खाजगी:
    शून्य reset() {
        m_instr_मोजणी = 0;
        m_ignoreRemaining = असत्य;
    }
    uint32_t startVisitBase(Ast_गाठ* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");
        जर (m_assertNoDups && !m_inCFunc) {
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
                            << static_cast<Ast_गाठ*>(nodep->user1p()));
            nodep->user1p(const_cast<शून्य*>(reinterpret_cast<स्थिरांक शून्य*>(m_startNodep)));
        }

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        स्थिरांक uint32_t saved_मोजणी = m_instr_मोजणी;
        m_instr_मोजणी = nodep->instr_मोजणी();
        परत saved_मोजणी;
    }
    शून्य endVisitBase(uint32_t saved_मोजणी, Ast_गाठ* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::डावा << m_instr_मोजणी << "  " << nodep);
        markCost(nodep);
        जर (!m_ignoreRemaining) m_instr_मोजणी += saved_मोजणी;
    }
    शून्य markCost(Ast_गाठ* nodep) {
        जर (m_osp) nodep->user2(m_instr_मोजणी + 1);  // Else don't mark to avoid writeback
    }

    // VISITORS
    शून्य visit(Ast_गाठ_Sel* nodep) override {
        जर (m_ignoreRemaining) परत;
        // This covers both AstArraySel and AstWordSel
        //
        // If some vector is a bazillion dwords long, and we're selecting 1
        // dword to read or write from it, our cost should be small.
        //
        // Hence, exclude the child of the AstWordSel from the computation,
        // whose cost scales with the size of the entire (maybe large) vector.
        स्थिरांक VisitBase vb{हे, nodep};
        iterateAndNextConstNull(nodep->bitp());
    }
    शून्य visit(AstSel* nodep) override {
        जर (m_ignoreRemaining) परत;
        // Similar to AstNodeSel above, a small select into a large vector
        // is not expensive. Count the cost of the AstSel itself (scales with
        // its width) and the cost of the lsbp() and widthp() nodes, but not
        // the fromp() node which could be disproportionately large.
        स्थिरांक VisitBase vb{हे, nodep};
        iterateAndNextConstNull(nodep->lsbp());
    }
    शून्य visit(AstConcat* nodep) override {
        जर (m_ignoreRemaining) परत;
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
    शून्य visit(Ast_गाठ_If* nodep) override {
        जर (m_ignoreRemaining) परत;
        स्थिरांक VisitBase vb{हे, nodep};
        iterateAndNextConstNull(nodep->condp());
        स्थिरांक uint32_t saved_मोजणी = m_instr_मोजणी;

        UINFO(8, "thensp:");
        reset();
        iterateAndNextConstNull(nodep->thensp());
        uint32_t if_मोजणी = m_instr_मोजणी;
        जर (nodep->branchPred().unlikely()) if_मोजणी = 0;

        UINFO(8, "elsesp:");
        reset();
        iterateAndNextConstNull(nodep->elsesp());
        uint32_t else_मोजणी = m_instr_मोजणी;
        जर (nodep->branchPred().likely()) else_मोजणी = 0;

        reset();
        जर (if_मोजणी >= else_मोजणी) {
            m_instr_मोजणी = saved_मोजणी + if_मोजणी;
            जर (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } अन्यथा {
            m_instr_मोजणी = saved_मोजणी + else_मोजणी;
            जर (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    शून्य visit(AstCond* nodep) override {
        जर (m_ignoreRemaining) परत;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        स्थिरांक VisitBase vb{हे, nodep};
        iterateAndNextConstNull(nodep->condp());
        स्थिरांक uint32_t saved_मोजणी = m_instr_मोजणी;

        UINFO(8, "?");
        reset();
        iterateAndNextConstNull(nodep->thenp());
        स्थिरांक uint32_t if_मोजणी = m_instr_मोजणी;

        UINFO(8, ":");
        reset();
        iterateAndNextConstNull(nodep->elsep());
        स्थिरांक uint32_t else_मोजणी = m_instr_मोजणी;

        reset();
        जर (if_मोजणी >= else_मोजणी) {
            m_instr_मोजणी = saved_मोजणी + if_मोजणी;
            जर (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } अन्यथा {
            m_instr_मोजणी = saved_मोजणी + else_मोजणी;
            जर (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    शून्य visit(AstCAwait* nodep) override {
        जर (m_ignoreRemaining) परत;
        iterateChildrenConst(nodep);
        // Anything past a co_await is irrelevant
        m_ignoreRemaining = सत्य;
    }
    शून्य visit(AstFork* nodep) override {
        जर (m_ignoreRemaining) परत;
        स्थिरांक VisitBase vb{हे, nodep};
        iterateAndNextConstNull(nodep->stmtsp());
        uint32_t total_मोजणी = m_instr_मोजणी;
        // Sum counts in each statement until the first await
        साठी (Ast_गाठ* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            reset();
            iterateConst(stmtp);
            total_मोजणी += m_instr_मोजणी;
        }
        m_instr_मोजणी = total_मोजणी;
        m_ignoreRemaining = असत्य;
    }
    शून्य visit(AstActive* nodep) override {
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
    शून्य visit(Ast_गाठ_CCall* nodep) override {
        जर (m_ignoreRemaining) परत;
        स्थिरांक VisitBase vb{हे, nodep};
        iterateChildrenConst(nodep);
        स्थिरांक स्वतः it = m_funcCosts.find(nodep->funcp());
        जर (it == m_funcCosts.end()) {
            m_tracingCall = सत्य;
            iterateConst(nodep->funcp());
            UASSERT_OBJ(!m_tracingCall, nodep,
                        "visit(AstCFunc) should have cleared m_tracingCall.");
        } अन्यथा {
            m_instr_मोजणी += it->second;  // This guards and adds zero cost on recursive calls
        }
    }
    शून्य visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        UASSERT_OBJ(m_tracingCall || nodep == m_startNodep, nodep,
                    "AstCFunc not under AstCCall, or not start node");
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not be ignoring at the start of a CFunc");
        स्थिरांक स्वतः emplacePair = m_funcCosts.emplace(nodep, 0);
        UASSERT_OBJ(emplacePair.second, nodep, "Should visit every CFunc at most once");
        uint32_t& funcCostr = emplacePair.first->second;
        m_tracingCall = असत्य;

        स्थिरांक uint32_t saved_मोजणी = m_instr_मोजणी;
        reset();
        {
            VL_RESTORER(m_inCFunc);
            m_inCFunc = सत्य;
            स्थिरांक VisitBase vb{हे, nodep};
            iterateChildrenConst(nodep);
        }
        funcCostr = m_instr_मोजणी;
        m_instr_मोजणी += saved_मोजणी;

        m_ignoreRemaining = असत्य;
    }
    शून्य visit(Ast_गाठ_Cons* nodep) override {
        जर (m_ignoreRemaining) परत;
        स्थिरांक VisitBase vb{हे, nodep};
        // A constant value is not built at run time, so its parts cost nothing
        जर (!nodep->is_Constant_वृक्ष()) iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_गाठ* nodep) override {
        जर (m_ignoreRemaining) परत;
        स्थिरांक VisitBase vb{हे, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(Instr_मोजणी_Visitor);
};

// Iterate the graph printing the critical path marked by previous visitation
वर्ग Instr_मोजणी_Dump_Visitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> int.  Path cost, 0 means don't dump

    // MEMBERS
    std::ostream* स्थिरांक m_osp;  // Dump file
    अचिन्हित m_depth = 0;  // Current tree depth for printing indent

सार्वजनिक:
    // CONSTRUCTORS
    Instr_मोजणी_Dump_Visitor(Ast_गाठ* nodep, std::ostream* osp)
        : m_osp{osp} {
        // No check for nullptr output, so...
        UASSERT_OBJ(osp, nodep, "Don't call if not dumping");
        जर (nodep) iterateConst(nodep);
    }
    ~Instr_मोजणी_Dump_Visitor() override = पूर्वनिर्धारित;

खाजगी:
    // METHODS
    string indent() स्थिरांक { परत string(m_depth, ':') + " "; }
    शून्य visit(Ast_गाठ* nodep) override {
        ++m_depth;
        जर (स्थिरांक अचिन्हित costPlus1 = nodep->user2()) {
            *m_osp << "  " << indent() << "cost " << std::setw(6) << std::डावा << (costPlus1 - 1)
                   << "  " << nodep << '\n';
            iterateChildrenConst(nodep);
        }
        --m_depth;
    }

    VL_UNCOPYABLE(Instr_मोजणी_Dump_Visitor);
};

uint32_t V3_Instr_मोजणी::मोजणी(Ast_गाठ* nodep, तार्किक assertNoDups, std::ostream* osp) {
    स्थिरांक Instr_मोजणी_Visitor visitor{nodep, assertNoDups, osp};
    जर (osp) स्थिरांक Instr_मोजणी_Dump_Visitor dumper{nodep, osp};
    परत visitor.instr_मोजणी();
}
