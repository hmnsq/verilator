// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Estimate stack size to run the AST subtree.
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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3StackCount.h"

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the stack byte size for executing all logic within and below a
/// given AST node. This is very rough, only for warning when stack is too
/// small.

শ্রেণী স্তূপ_গুনতি_Visitor final : সর্বজনীন VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    ধ্রুবক VNUser2InUse m_inuser2;

    // MEMBERS
    uint64_t m_স্তূপ_আকার = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    যৌক্তিক m_tracingCall = মিথ্যা;  // Iterating into a CCall to a CFunc
    যৌক্তিক m_ignoreRemaining = মিথ্যা;  // Ignore remaining statements in the block
    যৌক্তিক m_inCFunc = মিথ্যা;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    শ্রেণী VisitBase final {
        // MEMBERS
        uint64_t m_saved_গুনতি;  // Count before visit applied
        Ast_গ্রন্থি* ধ্রুবক m_nodep;  // Node this RAII helper is visiting
        স্তূপ_গুনতি_Visitor* ধ্রুবক m_visitor;  // Visitor whose start/endVisitBase are called

    সর্বজনীন:
        // CONSTRUCTORS
        VisitBase(স্তূপ_গুনতি_Visitor* visitor, Ast_গ্রন্থি* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_গুনতি = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_গুনতি, m_nodep); }

    ব্যক্তিগত:
        VL_UNCOPYABLE(VisitBase);
    };

সর্বজনীন:
    // CONSTRUCTORS
    explicit স্তূপ_গুনতি_Visitor(Ast_গ্রন্থি* nodep) { iterateConstNull(nodep); }
    ~স্তূপ_গুনতি_Visitor() override = পূর্বনির্ধারিত;

    // METHODS
    uint64_t স্তূপ_আকার() ধ্রুবক { ফেরত m_স্তূপ_আকার + m_callMax; }

ব্যক্তিগত:
    শূন্য reset() {
        m_স্তূপ_আকার = 0;
        m_ignoreRemaining = মিথ্যা;
    }
    uint64_t startVisitBase(Ast_গ্রন্থি* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ধ্রুবক uint64_t saved_গুনতি = m_স্তূপ_আকার;
        m_স্তূপ_আকার = 0;
        ফেরত saved_গুনতি;
    }
    শূন্য endVisitBase(uint64_t saved_গুনতি, ধ্রুবক Ast_গ্রন্থি* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::বাম << m_স্তূপ_আকার << "  " << nodep);
        যদি (!m_ignoreRemaining) m_স্তূপ_আকার += saved_গুনতি;
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_If* nodep) override {
        যদি (m_ignoreRemaining) ফেরত;
        ধ্রুবক VisitBase vb{এই, nodep};
        iterate_And_পরবর্তী_Const_Null(nodep->condp());
        ধ্রুবক uint64_t saved_গুনতি = m_স্তূপ_আকার;

        UINFO(8, "thensp:");
        reset();
        iterate_And_পরবর্তী_Const_Null(nodep->thensp());
        uint64_t if_গুনতি = m_স্তূপ_আকার;
        যদি (nodep->branchPred().unlikely()) if_গুনতি = 0;

        UINFO(8, "elsesp:");
        reset();
        iterate_And_পরবর্তী_Const_Null(nodep->elsesp());
        uint64_t else_গুনতি = m_স্তূপ_আকার;
        যদি (nodep->branchPred().likely()) else_গুনতি = 0;

        reset();
        যদি (if_গুনতি >= else_গুনতি) {
            m_স্তূপ_আকার = saved_গুনতি + if_গুনতি;
            যদি (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } অন্যথা {
            m_স্তূপ_আকার = saved_গুনতি + else_গুনতি;
            যদি (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    শূন্য visit(AstCond* nodep) override {
        যদি (m_ignoreRemaining) ফেরত;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ধ্রুবক VisitBase vb{এই, nodep};
        iterate_And_পরবর্তী_Const_Null(nodep->condp());
        ধ্রুবক uint64_t saved_গুনতি = m_স্তূপ_আকার;

        UINFO(8, "?");
        reset();
        iterate_And_পরবর্তী_Const_Null(nodep->thenp());
        ধ্রুবক uint64_t if_গুনতি = m_স্তূপ_আকার;

        UINFO(8, ":");
        reset();
        iterate_And_পরবর্তী_Const_Null(nodep->elsep());
        ধ্রুবক uint64_t else_গুনতি = m_স্তূপ_আকার;

        reset();
        যদি (if_গুনতি >= else_গুনতি) {
            m_স্তূপ_আকার = saved_গুনতি + if_গুনতি;
            যদি (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } অন্যথা {
            m_স্তূপ_আকার = saved_গুনতি + else_গুনতি;
            যদি (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    শূন্য visit(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    শূন্য visit(Ast_গ্রন্থি_CCall* nodep) override {
        যদি (m_ignoreRemaining) ফেরত;
        ধ্রুবক VisitBase vb{এই, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = সত্য;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    শূন্য visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        যদি (!m_tracingCall && !nodep->entryPoint()) ফেরত;
        m_tracingCall = মিথ্যা;
        যদি (nodep->recursive()) ফেরত;
        যদি (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignoreRemaining);
            VL_RESTORER(m_স্তূপ_আকার);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_inCFunc);
            m_tracingCall = মিথ্যা;
            m_স্তূপ_আকার = 0;
            m_callMax = 0;
            m_inCFunc = সত্য;
            ধ্রুবক VisitBase vb{এই, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_স্তূপ_আকার + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = মিথ্যা;
    }
    শূন্য visit(AstVar* nodep) override {
        যদি (!m_inCFunc) ফেরত;
        m_স্তূপ_আকার += nodep->isRef() ? আকার(শূন্য*) : nodep->dtypep()->width_যোগ_Bytes();
        iterateChildrenConst(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি_রাশিমালা* nodep) override {}  // Short-circuit
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যদি (m_ignoreRemaining) ফেরত;
        ধ্রুবক VisitBase vb{এই, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(স্তূপ_গুনতি_Visitor);
};

uint64_t V3_স্তূপ_গুনতি::গুনতি(Ast_গ্রন্থি* nodep) {
    ধ্রুবক স্তূপ_গুনতি_Visitor visitor{nodep};
    ফেরত visitor.স্তূপ_আকার();
}
