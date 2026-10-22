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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3StackCount.h"

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the stack byte size for executing all logic within and below a
/// given AST node. This is very rough, only for warning when stack is too
/// small.

رده پشته_شمار_Visitor final : عمومی VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    ثابت VNUser2InUse m_inuser2;

    // MEMBERS
    uint64_t m_پشته_Size = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    منطقی m_tracingCall = نادرست;  // Iterating into a CCall to a CFunc
    منطقی m_ignoreRemaining = نادرست;  // Ignore remaining statements in the block
    منطقی m_inCFunc = نادرست;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    رده VisitBase final {
        // MEMBERS
        uint64_t m_saved_شمار;  // Count before visit applied
        Ast_گره* ثابت m_nodep;  // Node this RAII helper is visiting
        پشته_شمار_Visitor* ثابت m_visitor;  // Visitor whose start/endVisitBase are called

    عمومی:
        // CONSTRUCTORS
        VisitBase(پشته_شمار_Visitor* visitor, Ast_گره* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_شمار = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_شمار, m_nodep); }

    خصوصی:
        VL_UNCOPYABLE(VisitBase);
    };

عمومی:
    // CONSTRUCTORS
    explicit پشته_شمار_Visitor(Ast_گره* nodep) { iterateConstNull(nodep); }
    ~پشته_شمار_Visitor() override = پیشفرض;

    // METHODS
    uint64_t پشته_Size() ثابت { بازگشت m_پشته_Size + m_callMax; }

خصوصی:
    تهی reset() {
        m_پشته_Size = 0;
        m_ignoreRemaining = نادرست;
    }
    uint64_t startVisitBase(Ast_گره* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ثابت uint64_t saved_شمار = m_پشته_Size;
        m_پشته_Size = 0;
        بازگشت saved_شمار;
    }
    تهی endVisitBase(uint64_t saved_شمار, ثابت Ast_گره* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::چپ << m_پشته_Size << "  " << nodep);
        اگر (!m_ignoreRemaining) m_پشته_Size += saved_شمار;
    }

    // VISITORS
    تهی visit(Ast_گره_If* nodep) override {
        اگر (m_ignoreRemaining) بازگشت;
        ثابت VisitBase vb{این, nodep};
        iterateAndNextConstNull(nodep->condp());
        ثابت uint64_t saved_شمار = m_پشته_Size;

        UINFO(8, "thensp:");
        reset();
        iterateAndNextConstNull(nodep->thensp());
        uint64_t if_شمار = m_پشته_Size;
        اگر (nodep->branchPred().unlikely()) if_شمار = 0;

        UINFO(8, "elsesp:");
        reset();
        iterateAndNextConstNull(nodep->elsesp());
        uint64_t else_شمار = m_پشته_Size;
        اگر (nodep->branchPred().likely()) else_شمار = 0;

        reset();
        اگر (if_شمار >= else_شمار) {
            m_پشته_Size = saved_شمار + if_شمار;
            اگر (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } وگرنه {
            m_پشته_Size = saved_شمار + else_شمار;
            اگر (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    تهی visit(AstCond* nodep) override {
        اگر (m_ignoreRemaining) بازگشت;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ثابت VisitBase vb{این, nodep};
        iterateAndNextConstNull(nodep->condp());
        ثابت uint64_t saved_شمار = m_پشته_Size;

        UINFO(8, "?");
        reset();
        iterateAndNextConstNull(nodep->thenp());
        ثابت uint64_t if_شمار = m_پشته_Size;

        UINFO(8, ":");
        reset();
        iterateAndNextConstNull(nodep->elsep());
        ثابت uint64_t else_شمار = m_پشته_Size;

        reset();
        اگر (if_شمار >= else_شمار) {
            m_پشته_Size = saved_شمار + if_شمار;
            اگر (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } وگرنه {
            m_پشته_Size = saved_شمار + else_شمار;
            اگر (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    تهی visit(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    تهی visit(Ast_گره_CCall* nodep) override {
        اگر (m_ignoreRemaining) بازگشت;
        ثابت VisitBase vb{این, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = درست;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    تهی visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        اگر (!m_tracingCall && !nodep->entryPoint()) بازگشت;
        m_tracingCall = نادرست;
        اگر (nodep->recursive()) بازگشت;
        اگر (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignoreRemaining);
            VL_RESTORER(m_پشته_Size);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_inCFunc);
            m_tracingCall = نادرست;
            m_پشته_Size = 0;
            m_callMax = 0;
            m_inCFunc = درست;
            ثابت VisitBase vb{این, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_پشته_Size + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = نادرست;
    }
    تهی visit(AstVar* nodep) override {
        اگر (!m_inCFunc) بازگشت;
        m_پشته_Size += nodep->isRef() ? اندازه(تهی*) : nodep->dtypep()->widthTotalBytes();
        iterateChildrenConst(nodep);
    }

    تهی visit(Ast_گره_عبارت* nodep) override {}  // Short-circuit
    تهی visit(Ast_گره* nodep) override {
        اگر (m_ignoreRemaining) بازگشت;
        ثابت VisitBase vb{این, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(پشته_شمار_Visitor);
};

uint64_t V3_پشته_شمار::شمار(Ast_گره* nodep) {
    ثابت پشته_شمار_Visitor visitor{nodep};
    بازگشت visitor.پشته_Size();
}
