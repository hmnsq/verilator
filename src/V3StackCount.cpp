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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3StackCount.h"

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the stack byte size for executing all logic within and below a
/// given AST node. This is very rough, only for warning when stack is too
/// small.

صنف پشته_شمار_Visitor final : عام VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    ثابت VNUser2_درون_Use m_inuser2;

    // MEMBERS
    uint64_t m_پشته_اندازه = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    منطقي m_tracingCall = خطأ;  // Iterating into a CCall to a CFunc
    منطقي m_ignore_مانده = خطأ;  // Ignore remaining statements in the block
    منطقي m_درون_CFunc = خطأ;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    صنف بازدید_پایه final {
        // MEMBERS
        uint64_t m_saved_شمار;  // Count before visit applied
        Ast_گره* ثابت m_nodep;  // Node this RAII helper is visiting
        پشته_شمار_Visitor* ثابت m_visitor;  // Visitor whose start/endVisitBase are called

    عام:
        // CONSTRUCTORS
        بازدید_پایه(پشته_شمار_Visitor* visitor, Ast_گره* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_شمار = m_visitor->شروع_بازدید_پایه(nodep);
        }
        ~بازدید_پایه() { m_visitor->انتها_بازدید_پایه(m_saved_شمار, m_nodep); }

    خاص:
        VL_UNCOPYABLE(بازدید_پایه);
    };

عام:
    // CONSTRUCTORS
    صریح پشته_شمار_Visitor(Ast_گره* nodep) { iterateConstNull(nodep); }
    ~پشته_شمار_Visitor() override = افتراضي;

    // METHODS
    uint64_t پشته_اندازه() ثابت { أرجع m_پشته_اندازه + m_callMax; }

خاص:
    فراغ reset() {
        m_پشته_اندازه = 0;
        m_ignore_مانده = خطأ;
    }
    uint64_t شروع_بازدید_پایه(Ast_گره* nodep) {
        UASSERT_OBJ(!m_ignore_مانده, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ثابت uint64_t saved_شمار = m_پشته_اندازه;
        m_پشته_اندازه = 0;
        أرجع saved_شمار;
    }
    فراغ انتها_بازدید_پایه(uint64_t saved_شمار, ثابت Ast_گره* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::چپ << m_پشته_اندازه << "  " << nodep);
        إذا (!m_ignore_مانده) m_پشته_اندازه += saved_شمار;
    }

    // VISITORS
    فراغ بازدید(Ast_گره_If* nodep) override {
        إذا (m_ignore_مانده) أرجع;
        ثابت بازدید_پایه vb{هذا, nodep};
        iterate_And_بعدی_Const_Null(nodep->condp());
        ثابت uint64_t saved_شمار = m_پشته_اندازه;

        UINFO(8, "thensp:");
        reset();
        iterate_And_بعدی_Const_Null(nodep->thensp());
        uint64_t if_شمار = m_پشته_اندازه;
        إذا (nodep->branchPred().unlikely()) if_شمار = 0;

        UINFO(8, "elsesp:");
        reset();
        iterate_And_بعدی_Const_Null(nodep->elsesp());
        uint64_t else_شمار = m_پشته_اندازه;
        إذا (nodep->branchPred().likely()) else_شمار = 0;

        reset();
        إذا (if_شمار >= else_شمار) {
            m_پشته_اندازه = saved_شمار + if_شمار;
            إذا (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } وإلا {
            m_پشته_اندازه = saved_شمار + else_شمار;
            إذا (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    فراغ بازدید(AstCond* nodep) override {
        إذا (m_ignore_مانده) أرجع;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ثابت بازدید_پایه vb{هذا, nodep};
        iterate_And_بعدی_Const_Null(nodep->condp());
        ثابت uint64_t saved_شمار = m_پشته_اندازه;

        UINFO(8, "?");
        reset();
        iterate_And_بعدی_Const_Null(nodep->thenp());
        ثابت uint64_t if_شمار = m_پشته_اندازه;

        UINFO(8, ":");
        reset();
        iterate_And_بعدی_Const_Null(nodep->elsep());
        ثابت uint64_t else_شمار = m_پشته_اندازه;

        reset();
        إذا (if_شمار >= else_شمار) {
            m_پشته_اندازه = saved_شمار + if_شمار;
            إذا (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } وإلا {
            m_پشته_اندازه = saved_شمار + else_شمار;
            إذا (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    فراغ بازدید(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    فراغ بازدید(Ast_گره_CCall* nodep) override {
        إذا (m_ignore_مانده) أرجع;
        ثابت بازدید_پایه vb{هذا, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = درست;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    فراغ بازدید(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        إذا (!m_tracingCall && !nodep->مدخل_نقطه()) أرجع;
        m_tracingCall = خطأ;
        إذا (nodep->recursive()) أرجع;
        إذا (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignore_مانده);
            VL_RESTORER(m_پشته_اندازه);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_درون_CFunc);
            m_tracingCall = خطأ;
            m_پشته_اندازه = 0;
            m_callMax = 0;
            m_درون_CFunc = درست;
            ثابت بازدید_پایه vb{هذا, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_پشته_اندازه + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = خطأ;
    }
    فراغ بازدید(AstVar* nodep) override {
        إذا (!m_درون_CFunc) أرجع;
        m_پشته_اندازه += nodep->isRef() ? حجم(فراغ*) : nodep->dtypep()->عرض_مجموع_بایتها();
        iterateChildrenConst(nodep);
    }

    فراغ بازدید(Ast_گره_عبارت* nodep) override {}  // Short-circuit
    فراغ بازدید(Ast_گره* nodep) override {
        إذا (m_ignore_مانده) أرجع;
        ثابت بازدید_پایه vb{هذا, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(پشته_شمار_Visitor);
};

uint64_t V3_پشته_شمار::شمار(Ast_گره* nodep) {
    ثابت پشته_شمار_Visitor visitor{nodep};
    أرجع visitor.پشته_اندازه();
}
