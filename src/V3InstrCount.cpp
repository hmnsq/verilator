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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3InstrCount.h"

#شامل <iomanip>
#شامل <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the instruction cost for executing all logic within and below
/// a given AST node. Note this estimates the number of instructions we'll
/// execute, not the number we'll generate. That is, for conditionals,
/// we'll count instructions from either the 'if' or the 'else' branch,
/// whichever is larger. We know we won't run both.

جماعت Instr_گنتی_Visitor final : عوامی VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()        -> bool. Processed if assertNoDups
    //  AstNode::user2()        -> uint64_t.  Path cost + 1, 0 means don't dump
    ثابت VNUser2_اندر_Use m_inuser2;

    // MEMBERS
    uint32_t m_instr_گنتی = 0;  // Running count of instructions
    ثابت Ast_گرہ* ثابت m_آغاز_Nodep;  // Start node of count
    منطقی m_tracingCall = جھوٹ;  // Iterating into a CCall to a CFunc
    منطقی m_اندر_CFunc = جھوٹ;  // Inside AstCFunc
    منطقی m_ignore_بقایا = جھوٹ;  // Ignore remaining statements in the block
    ثابت منطقی m_assertNoDups;  // Check for duplicates
    ثابت std::ostream* ثابت m_osp;  // Dump file
    std::unordered_map<ثابت AstCFunc*, uint32_t> m_funcCosts;  // Cost of each function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    جماعت دورہ_بنیاد final {
        // MEMBERS
        uint32_t m_saved_گنتی;  // Count before visit applied
        Ast_گرہ* ثابت m_nodep;  // Node helper is visiting
        Instr_گنتی_Visitor* ثابت m_visitor;  // Visitor whose start/endVisitBase are called

    عوامی:
        // CONSTRUCTORS
        دورہ_بنیاد(Instr_گنتی_Visitor* visitor, Ast_گرہ* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_گنتی = m_visitor->آغاز_دورہ_بنیاد(nodep);
        }
        ~دورہ_بنیاد() { m_visitor->انتہا_دورہ_بنیاد(m_saved_گنتی, m_nodep); }

    نجی:
        VL_UNCOPYABLE(دورہ_بنیاد);
    };

عوامی:
    // CONSTRUCTORS
    Instr_گنتی_Visitor(Ast_گرہ* nodep, منطقی assertNoDups, std::ostream* osp)
        : m_آغاز_Nodep{nodep}
        , m_assertNoDups{assertNoDups}
        , m_osp{osp} {
        iterateConstNull(nodep);
    }
    ~Instr_گنتی_Visitor() override = طےشدہ;

    // METHODS
    uint32_t instr_گنتی() ثابت { واپس m_instr_گنتی; }

نجی:
    خالی reset() {
        m_instr_گنتی = 0;
        m_ignore_بقایا = جھوٹ;
    }
    uint32_t آغاز_دورہ_بنیاد(Ast_گرہ* nodep) {
        UASSERT_OBJ(!m_ignore_بقایا, nodep, "Should not reach here if ignoring");
        اگر (m_assertNoDups && !m_اندر_CFunc) {
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
                            << static_cast<Ast_گرہ*>(nodep->user1p()));
            nodep->user1p(const_cast<خالی*>(reinterpret_cast<ثابت خالی*>(m_آغاز_Nodep)));
        }

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ثابت uint32_t saved_گنتی = m_instr_گنتی;
        m_instr_گنتی = nodep->instr_گنتی();
        واپس saved_گنتی;
    }
    خالی انتہا_دورہ_بنیاد(uint32_t saved_گنتی, Ast_گرہ* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::بائیں << m_instr_گنتی << "  " << nodep);
        mark_لاگت(nodep);
        اگر (!m_ignore_بقایا) m_instr_گنتی += saved_گنتی;
    }
    خالی mark_لاگت(Ast_گرہ* nodep) {
        اگر (m_osp) nodep->user2(m_instr_گنتی + 1);  // Else don't mark to avoid writeback
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_Sel* nodep) override {
        اگر (m_ignore_بقایا) واپس;
        // This covers both AstArraySel and AstWordSel
        //
        // If some vector is a bazillion dwords long, and we're selecting 1
        // dword to read or write from it, our cost should be small.
        //
        // Hence, exclude the child of the AstWordSel from the computation,
        // whose cost scales with the size of the entire (maybe large) vector.
        ثابت دورہ_بنیاد vb{یہ, nodep};
        iterate_And_اگلا_Const_Null(nodep->bitp());
    }
    خالی دورہ(AstSel* nodep) override {
        اگر (m_ignore_بقایا) واپس;
        // Similar to AstNodeSel above, a small select into a large vector
        // is not expensive. Count the cost of the AstSel itself (scales with
        // its width) and the cost of the lsbp() and widthp() nodes, but not
        // the fromp() node which could be disproportionately large.
        ثابت دورہ_بنیاد vb{یہ, nodep};
        iterate_And_اگلا_Const_Null(nodep->lsbp());
    }
    خالی دورہ(AstConcat* nodep) override {
        اگر (m_ignore_بقایا) واپس;
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
        mark_لاگت(nodep);
    }
    خالی دورہ(Ast_گرہ_If* nodep) override {
        اگر (m_ignore_بقایا) واپس;
        ثابت دورہ_بنیاد vb{یہ, nodep};
        iterate_And_اگلا_Const_Null(nodep->condp());
        ثابت uint32_t saved_گنتی = m_instr_گنتی;

        UINFO(8, "thensp:");
        reset();
        iterate_And_اگلا_Const_Null(nodep->thensp());
        uint32_t if_گنتی = m_instr_گنتی;
        اگر (nodep->branchPred().unlikely()) if_گنتی = 0;

        UINFO(8, "elsesp:");
        reset();
        iterate_And_اگلا_Const_Null(nodep->elsesp());
        uint32_t else_گنتی = m_instr_گنتی;
        اگر (nodep->branchPred().likely()) else_گنتی = 0;

        reset();
        اگر (if_گنتی >= else_گنتی) {
            m_instr_گنتی = saved_گنتی + if_گنتی;
            اگر (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } ورنہ {
            m_instr_گنتی = saved_گنتی + else_گنتی;
            اگر (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    خالی دورہ(AstCond* nodep) override {
        اگر (m_ignore_بقایا) واپس;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ثابت دورہ_بنیاد vb{یہ, nodep};
        iterate_And_اگلا_Const_Null(nodep->condp());
        ثابت uint32_t saved_گنتی = m_instr_گنتی;

        UINFO(8, "?");
        reset();
        iterate_And_اگلا_Const_Null(nodep->thenp());
        ثابت uint32_t if_گنتی = m_instr_گنتی;

        UINFO(8, ":");
        reset();
        iterate_And_اگلا_Const_Null(nodep->elsep());
        ثابت uint32_t else_گنتی = m_instr_گنتی;

        reset();
        اگر (if_گنتی >= else_گنتی) {
            m_instr_گنتی = saved_گنتی + if_گنتی;
            اگر (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } ورنہ {
            m_instr_گنتی = saved_گنتی + else_گنتی;
            اگر (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    خالی دورہ(AstCAwait* nodep) override {
        اگر (m_ignore_بقایا) واپس;
        iterateChildrenConst(nodep);
        // Anything past a co_await is irrelevant
        m_ignore_بقایا = سچ;
    }
    خالی دورہ(AstFork* nodep) override {
        اگر (m_ignore_بقایا) واپس;
        ثابت دورہ_بنیاد vb{یہ, nodep};
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
        uint32_t کل_گنتی = m_instr_گنتی;
        // Sum counts in each statement until the first await
        برائے (Ast_گرہ* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            reset();
            iterateConst(stmtp);
            کل_گنتی += m_instr_گنتی;
        }
        m_instr_گنتی = کل_گنتی;
        m_ignore_بقایا = جھوٹ;
    }
    خالی دورہ(Ast_فعال* nodep) override {
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
        mark_لاگت(nodep);
        UASSERT_OBJ(nodep == m_آغاز_Nodep, nodep, "Multiple actives, or not start node");
    }
    خالی دورہ(Ast_گرہ_CCall* nodep) override {
        اگر (m_ignore_بقایا) واپس;
        ثابت دورہ_بنیاد vb{یہ, nodep};
        iterateChildrenConst(nodep);
        ثابت خودکار it = m_funcCosts.ڈھونڈو(nodep->funcp());
        اگر (it == m_funcCosts.انتہا()) {
            m_tracingCall = سچ;
            iterateConst(nodep->funcp());
            UASSERT_OBJ(!m_tracingCall, nodep,
                        "visit(AstCFunc) should have cleared m_tracingCall.");
        } ورنہ {
            m_instr_گنتی += it->دوسرا;  // This guards and adds zero cost on recursive calls
        }
    }
    خالی دورہ(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        UASSERT_OBJ(m_tracingCall || nodep == m_آغاز_Nodep, nodep,
                    "AstCFunc not under AstCCall, or not start node");
        UASSERT_OBJ(!m_ignore_بقایا, nodep, "Should not be ignoring at the start of a CFunc");
        ثابت خودکار emplacePair = m_funcCosts.emplace(nodep, 0);
        UASSERT_OBJ(emplacePair.دوسرا, nodep, "Should visit every CFunc at most once");
        uint32_t& funcCostr = emplacePair.پہلا->دوسرا;
        m_tracingCall = جھوٹ;

        ثابت uint32_t saved_گنتی = m_instr_گنتی;
        reset();
        {
            VL_RESTORER(m_اندر_CFunc);
            m_اندر_CFunc = سچ;
            ثابت دورہ_بنیاد vb{یہ, nodep};
            iterateChildrenConst(nodep);
        }
        funcCostr = m_instr_گنتی;
        m_instr_گنتی += saved_گنتی;

        m_ignore_بقایا = جھوٹ;
    }
    خالی دورہ(Ast_گرہ_Cons* nodep) override {
        اگر (m_ignore_بقایا) واپس;
        ثابت دورہ_بنیاد vb{یہ, nodep};
        // A constant value is not built at run time, so its parts cost nothing
        اگر (!nodep->is_Constant_درخت()) iterateChildrenConst(nodep);
    }
    خالی دورہ(Ast_گرہ* nodep) override {
        اگر (m_ignore_بقایا) واپس;
        ثابت دورہ_بنیاد vb{یہ, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(Instr_گنتی_Visitor);
};

// Iterate the graph printing the critical path marked by previous visitation
جماعت Instr_گنتی_Dump_Visitor final : عوامی VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> int.  Path cost, 0 means don't dump

    // MEMBERS
    std::ostream* ثابت m_osp;  // Dump file
    غیرعلامتی m_گہرائی = 0;  // Current tree depth for printing indent

عوامی:
    // CONSTRUCTORS
    Instr_گنتی_Dump_Visitor(Ast_گرہ* nodep, std::ostream* osp)
        : m_osp{osp} {
        // No check for nullptr output, so...
        UASSERT_OBJ(osp, nodep, "Don't call if not dumping");
        اگر (nodep) iterateConst(nodep);
    }
    ~Instr_گنتی_Dump_Visitor() override = طےشدہ;

نجی:
    // METHODS
    string indent() ثابت { واپس string(m_گہرائی, ':') + " "; }
    خالی دورہ(Ast_گرہ* nodep) override {
        ++m_گہرائی;
        اگر (ثابت غیرعلامتی لاگت_Plus1 = nodep->user2()) {
            *m_osp << "  " << indent() << "cost " << std::setw(6) << std::بائیں << (لاگت_Plus1 - 1)
                   << "  " << nodep << '\n';
            iterateChildrenConst(nodep);
        }
        --m_گہرائی;
    }

    VL_UNCOPYABLE(Instr_گنتی_Dump_Visitor);
};

uint32_t V3_Instr_گنتی::گنتی(Ast_گرہ* nodep, منطقی assertNoDups, std::ostream* osp) {
    ثابت Instr_گنتی_Visitor visitor{nodep, assertNoDups, osp};
    اگر (osp) ثابت Instr_گنتی_Dump_Visitor dumper{nodep, osp};
    واپس visitor.instr_گنتی();
}
