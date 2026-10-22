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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3InstrCount.h"

#समावेश <iomanip>
#समावेश <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the instruction cost for executing all logic within and below
/// a given AST node. Note this estimates the number of instructions we'll
/// execute, not the number we'll generate. That is, for conditionals,
/// we'll count instructions from either the 'if' or the 'else' branch,
/// whichever is larger. We know we won't run both.

श्रेणी Instr_गिनती_Visitor final : खुला VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()        -> bool. Processed if assertNoDups
    //  AstNode::user2()        -> uint64_t.  Path cost + 1, 0 means don't dump
    स्थिर VNUser2InUse m_inuser2;

    // MEMBERS
    uint32_t m_instr_गिनती = 0;  // Running count of instructions
    स्थिर Ast_गाँठ* स्थिर m_startNodep;  // Start node of count
    bool m_tracingCall = false;  // Iterating into a CCall to a CFunc
    bool m_inCFunc = false;  // Inside AstCFunc
    bool m_ignoreRemaining = false;  // Ignore remaining statements in the block
    स्थिर bool m_assertNoDups;  // Check for duplicates
    स्थिर std::ब_प्रवाह* स्थिर m_osp;  // Dump file
    std::unordered_map<स्थिर AstCFunc*, uint32_t> m_funcCosts;  // Cost of each function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    श्रेणी VisitBase final {
        // MEMBERS
        uint32_t m_saved_गिनती;  // Count before visit applied
        Ast_गाँठ* स्थिर m_nodep;  // Node helper is visiting
        Instr_गिनती_Visitor* स्थिर m_visitor;  // Visitor whose start/endVisitBase are called

    खुला:
        // CONSTRUCTORS
        VisitBase(Instr_गिनती_Visitor* visitor, Ast_गाँठ* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_गिनती = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_गिनती, m_nodep); }

    निजी:
        VL_UNCOPYABLE(VisitBase);
    };

खुला:
    // CONSTRUCTORS
    Instr_गिनती_Visitor(Ast_गाँठ* nodep, bool assertNoDups, std::ब_प्रवाह* osp)
        : m_startNodep{nodep}
        , m_assertNoDups{assertNoDups}
        , m_osp{osp} {
        iterateConstNull(nodep);
    }
    ~Instr_गिनती_Visitor() override = शेष;

    // METHODS
    uint32_t instr_गिनती() स्थिर { वापस m_instr_गिनती; }

निजी:
    व्योम reset() {
        m_instr_गिनती = 0;
        m_ignoreRemaining = false;
    }
    uint32_t startVisitBase(Ast_गाँठ* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");
        अगर (m_assertNoDups && !m_inCFunc) {
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
                            << static_cast<Ast_गाँठ*>(nodep->user1p()));
            nodep->user1p(const_cast<व्योम*>(reinterpret_cast<स्थिर व्योम*>(m_startNodep)));
        }

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        स्थिर uint32_t saved_गिनती = m_instr_गिनती;
        m_instr_गिनती = nodep->instr_गिनती();
        वापस saved_गिनती;
    }
    व्योम endVisitBase(uint32_t saved_गिनती, Ast_गाँठ* nodep) {
        UINFO(8, "cost " << std::रखो_चौड़ाई(6) << std::बाँए << m_instr_गिनती << "  " << nodep);
        markCost(nodep);
        अगर (!m_ignoreRemaining) m_instr_गिनती += saved_गिनती;
    }
    व्योम markCost(Ast_गाँठ* nodep) {
        अगर (m_osp) nodep->user2(m_instr_गिनती + 1);  // Else don't mark to avoid writeback
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Sel* nodep) override {
        अगर (m_ignoreRemaining) वापस;
        // This covers both AstArraySel and AstWordSel
        //
        // If some vector is a bazillion dwords long, and we're selecting 1
        // dword to read or write from it, our cost should be small.
        //
        // Hence, exclude the child of the AstWordSel from the computation,
        // whose cost scales with the size of the entire (maybe large) vector.
        स्थिर VisitBase vb{यह, nodep};
        iterate_And_अग्र_Const_Null(nodep->bitp());
    }
    व्योम visit(AstSel* nodep) override {
        अगर (m_ignoreRemaining) वापस;
        // Similar to AstNodeSel above, a small select into a large vector
        // is not expensive. Count the cost of the AstSel itself (scales with
        // its width) and the cost of the lsbp() and widthp() nodes, but not
        // the fromp() node which could be disproportionately large.
        स्थिर VisitBase vb{यह, nodep};
        iterate_And_अग्र_Const_Null(nodep->lsbp());
    }
    व्योम visit(AstConcat* nodep) override {
        अगर (m_ignoreRemaining) वापस;
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
    व्योम visit(Ast_गाँठ_If* nodep) override {
        अगर (m_ignoreRemaining) वापस;
        स्थिर VisitBase vb{यह, nodep};
        iterate_And_अग्र_Const_Null(nodep->condp());
        स्थिर uint32_t saved_गिनती = m_instr_गिनती;

        UINFO(8, "thensp:");
        reset();
        iterate_And_अग्र_Const_Null(nodep->thensp());
        uint32_t if_गिनती = m_instr_गिनती;
        अगर (nodep->branchPred().unlikely()) if_गिनती = 0;

        UINFO(8, "elsesp:");
        reset();
        iterate_And_अग्र_Const_Null(nodep->elsesp());
        uint32_t else_गिनती = m_instr_गिनती;
        अगर (nodep->branchPred().likely()) else_गिनती = 0;

        reset();
        अगर (if_गिनती >= else_गिनती) {
            m_instr_गिनती = saved_गिनती + if_गिनती;
            अगर (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } अन्यथा {
            m_instr_गिनती = saved_गिनती + else_गिनती;
            अगर (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    व्योम visit(AstCond* nodep) override {
        अगर (m_ignoreRemaining) वापस;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        स्थिर VisitBase vb{यह, nodep};
        iterate_And_अग्र_Const_Null(nodep->condp());
        स्थिर uint32_t saved_गिनती = m_instr_गिनती;

        UINFO(8, "?");
        reset();
        iterate_And_अग्र_Const_Null(nodep->thenp());
        स्थिर uint32_t if_गिनती = m_instr_गिनती;

        UINFO(8, ":");
        reset();
        iterate_And_अग्र_Const_Null(nodep->elsep());
        स्थिर uint32_t else_गिनती = m_instr_गिनती;

        reset();
        अगर (if_गिनती >= else_गिनती) {
            m_instr_गिनती = saved_गिनती + if_गिनती;
            अगर (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } अन्यथा {
            m_instr_गिनती = saved_गिनती + else_गिनती;
            अगर (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    व्योम visit(AstCAwait* nodep) override {
        अगर (m_ignoreRemaining) वापस;
        iterateChildrenConst(nodep);
        // Anything past a co_await is irrelevant
        m_ignoreRemaining = true;
    }
    व्योम visit(AstFork* nodep) override {
        अगर (m_ignoreRemaining) वापस;
        स्थिर VisitBase vb{यह, nodep};
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        uint32_t योग_गिनती = m_instr_गिनती;
        // Sum counts in each statement until the first await
        क्रम (Ast_गाँठ* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            reset();
            iterateConst(stmtp);
            योग_गिनती += m_instr_गिनती;
        }
        m_instr_गिनती = योग_गिनती;
        m_ignoreRemaining = false;
    }
    व्योम visit(AstActive* nodep) override {
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
    व्योम visit(Ast_गाँठ_CCall* nodep) override {
        अगर (m_ignoreRemaining) वापस;
        स्थिर VisitBase vb{यह, nodep};
        iterateChildrenConst(nodep);
        स्थिर स्वतः it = m_funcCosts.find(nodep->funcp());
        अगर (it == m_funcCosts.end()) {
            m_tracingCall = true;
            iterateConst(nodep->funcp());
            UASSERT_OBJ(!m_tracingCall, nodep,
                        "visit(AstCFunc) should have cleared m_tracingCall.");
        } अन्यथा {
            m_instr_गिनती += it->second;  // This guards and adds zero cost on recursive calls
        }
    }
    व्योम visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        UASSERT_OBJ(m_tracingCall || nodep == m_startNodep, nodep,
                    "AstCFunc not under AstCCall, or not start node");
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not be ignoring at the start of a CFunc");
        स्थिर स्वतः emplacePair = m_funcCosts.emplace(nodep, 0);
        UASSERT_OBJ(emplacePair.second, nodep, "Should visit every CFunc at most once");
        uint32_t& funcCostr = emplacePair.first->second;
        m_tracingCall = false;

        स्थिर uint32_t saved_गिनती = m_instr_गिनती;
        reset();
        {
            VL_RESTORER(m_inCFunc);
            m_inCFunc = true;
            स्थिर VisitBase vb{यह, nodep};
            iterateChildrenConst(nodep);
        }
        funcCostr = m_instr_गिनती;
        m_instr_गिनती += saved_गिनती;

        m_ignoreRemaining = false;
    }
    व्योम visit(Ast_गाँठ_Cons* nodep) override {
        अगर (m_ignoreRemaining) वापस;
        स्थिर VisitBase vb{यह, nodep};
        // A constant value is not built at run time, so its parts cost nothing
        अगर (!nodep->is_Constant_वृक्ष()) iterateChildrenConst(nodep);
    }
    व्योम visit(Ast_गाँठ* nodep) override {
        अगर (m_ignoreRemaining) वापस;
        स्थिर VisitBase vb{यह, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(Instr_गिनती_Visitor);
};

// Iterate the graph printing the critical path marked by previous visitation
श्रेणी Instr_गिनती_Dump_Visitor final : खुला VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> int.  Path cost, 0 means don't dump

    // MEMBERS
    std::ब_प्रवाह* स्थिर m_osp;  // Dump file
    अचिन्हित m_depth = 0;  // Current tree depth for printing indent

खुला:
    // CONSTRUCTORS
    Instr_गिनती_Dump_Visitor(Ast_गाँठ* nodep, std::ब_प्रवाह* osp)
        : m_osp{osp} {
        // No check for nullptr output, so...
        UASSERT_OBJ(osp, nodep, "Don't call if not dumping");
        अगर (nodep) iterateConst(nodep);
    }
    ~Instr_गिनती_Dump_Visitor() override = शेष;

निजी:
    // METHODS
    string indent() स्थिर { वापस string(m_depth, ':') + " "; }
    व्योम visit(Ast_गाँठ* nodep) override {
        ++m_depth;
        अगर (स्थिर अचिन्हित costPlus1 = nodep->user2()) {
            *m_osp << "  " << indent() << "cost " << std::रखो_चौड़ाई(6) << std::बाँए << (costPlus1 - 1)
                   << "  " << nodep << '\n';
            iterateChildrenConst(nodep);
        }
        --m_depth;
    }

    VL_UNCOPYABLE(Instr_गिनती_Dump_Visitor);
};

uint32_t V3_Instr_गिनती::गिनती(Ast_गाँठ* nodep, bool assertNoDups, std::ब_प्रवाह* osp) {
    स्थिर Instr_गिनती_Visitor visitor{nodep, assertNoDups, osp};
    अगर (osp) स्थिर Instr_गिनती_Dump_Visitor dumper{nodep, osp};
    वापस visitor.instr_गिनती();
}
