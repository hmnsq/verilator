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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3StackCount.h"

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the stack byte size for executing all logic within and below a
/// given AST node. This is very rough, only for warning when stack is too
/// small.

শ্ৰেণী স্তূপ_গুনতি_Visitor final : সার্ৱজনিক VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    ধ্রুৱ VNUser2InUse m_inuser2;

    // MEMBERS
    uint64_t m_স্তূপ_Size = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    তার্কিক m_tracingCall = মিছা;  // Iterating into a CCall to a CFunc
    তার্কিক m_ignoreRemaining = মিছা;  // Ignore remaining statements in the block
    তার্কিক m_inCFunc = মিছা;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    শ্ৰেণী VisitBase final {
        // MEMBERS
        uint64_t m_saved_গুনতি;  // Count before visit applied
        Ast_গ্ৰন্থি* ধ্রুৱ m_nodep;  // Node this RAII helper is visiting
        স্তূপ_গুনতি_Visitor* ধ্রুৱ m_visitor;  // Visitor whose start/endVisitBase are called

    সার্ৱজনিক:
        // CONSTRUCTORS
        VisitBase(স্তূপ_গুনতি_Visitor* visitor, Ast_গ্ৰন্থি* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_গুনতি = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_গুনতি, m_nodep); }

    গুপ্ত:
        VL_UNCOPYABLE(VisitBase);
    };

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit স্তূপ_গুনতি_Visitor(Ast_গ্ৰন্থি* nodep) { iterateConstNull(nodep); }
    ~স্তূপ_গুনতি_Visitor() override = স্ৱভাৱ;

    // METHODS
    uint64_t স্তূপ_Size() ধ্রুৱ { ঘূৰাই m_স্তূপ_Size + m_callMax; }

গুপ্ত:
    শূন্য reset() {
        m_স্তূপ_Size = 0;
        m_ignoreRemaining = মিছা;
    }
    uint64_t startVisitBase(Ast_গ্ৰন্থি* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ধ্রুৱ uint64_t saved_গুনতি = m_স্তূপ_Size;
        m_স্তূপ_Size = 0;
        ঘূৰাই saved_গুনতি;
    }
    শূন্য endVisitBase(uint64_t saved_গুনতি, ধ্রুৱ Ast_গ্ৰন্থি* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::বাম << m_স্তূপ_Size << "  " << nodep);
        যদি (!m_ignoreRemaining) m_স্তূপ_Size += saved_গুনতি;
    }

    // VISITORS
    শূন্য visit(Ast_গ্ৰন্থি_If* nodep) override {
        যদি (m_ignoreRemaining) ঘূৰাই;
        ধ্রুৱ VisitBase vb{এতত্, nodep};
        iterateAndNextConstNull(nodep->condp());
        ধ্রুৱ uint64_t saved_গুনতি = m_স্তূপ_Size;

        UINFO(8, "thensp:");
        reset();
        iterateAndNextConstNull(nodep->thensp());
        uint64_t if_গুনতি = m_স্তূপ_Size;
        যদি (nodep->branchPred().unlikely()) if_গুনতি = 0;

        UINFO(8, "elsesp:");
        reset();
        iterateAndNextConstNull(nodep->elsesp());
        uint64_t else_গুনতি = m_স্তূপ_Size;
        যদি (nodep->branchPred().likely()) else_গুনতি = 0;

        reset();
        যদি (if_গুনতি >= else_গুনতি) {
            m_স্তূপ_Size = saved_গুনতি + if_গুনতি;
            যদি (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } অন্যথা {
            m_স্তূপ_Size = saved_গুনতি + else_গুনতি;
            যদি (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    শূন্য visit(AstCond* nodep) override {
        যদি (m_ignoreRemaining) ঘূৰাই;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ধ্রুৱ VisitBase vb{এতত্, nodep};
        iterateAndNextConstNull(nodep->condp());
        ধ্রুৱ uint64_t saved_গুনতি = m_স্তূপ_Size;

        UINFO(8, "?");
        reset();
        iterateAndNextConstNull(nodep->thenp());
        ধ্রুৱ uint64_t if_গুনতি = m_স্তূপ_Size;

        UINFO(8, ":");
        reset();
        iterateAndNextConstNull(nodep->elsep());
        ধ্রুৱ uint64_t else_গুনতি = m_স্তূপ_Size;

        reset();
        যদি (if_গুনতি >= else_গুনতি) {
            m_স্তূপ_Size = saved_গুনতি + if_গুনতি;
            যদি (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } অন্যথা {
            m_স্তূপ_Size = saved_গুনতি + else_গুনতি;
            যদি (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    শূন্য visit(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    শূন্য visit(Ast_গ্ৰন্থি_CCall* nodep) override {
        যদি (m_ignoreRemaining) ঘূৰাই;
        ধ্রুৱ VisitBase vb{এতত্, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = সঁচা;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    শূন্য visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        যদি (!m_tracingCall && !nodep->entryPoint()) ঘূৰাই;
        m_tracingCall = মিছা;
        যদি (nodep->recursive()) ঘূৰাই;
        যদি (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignoreRemaining);
            VL_RESTORER(m_স্তূপ_Size);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_inCFunc);
            m_tracingCall = মিছা;
            m_স্তূপ_Size = 0;
            m_callMax = 0;
            m_inCFunc = সঁচা;
            ধ্রুৱ VisitBase vb{এতত্, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_স্তূপ_Size + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = মিছা;
    }
    শূন্য visit(AstVar* nodep) override {
        যদি (!m_inCFunc) ঘূৰাই;
        m_স্তূপ_Size += nodep->isRef() ? পরিমাণ(শূন্য*) : nodep->dtypep()->widthTotalBytes();
        iterateChildrenConst(nodep);
    }

    শূন্য visit(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) override {}  // Short-circuit
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override {
        যদি (m_ignoreRemaining) ঘূৰাই;
        ধ্রুৱ VisitBase vb{এতত্, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(স্তূপ_গুনতি_Visitor);
};

uint64_t V3_স্তূপ_গুনতি::গুনতি(Ast_গ্ৰন্থি* nodep) {
    ধ্রুৱ স্তূপ_গুনতি_Visitor visitor{nodep};
    ঘূৰাই visitor.স্তূপ_Size();
}
