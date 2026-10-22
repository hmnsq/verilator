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

صنف ډېری_شمېر_Visitor final : عام VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    ثابت VNUser2_دننه_Use m_inuser2;

    // MEMBERS
    uint64_t m_ډېری_اندازه = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    منطقي m_tracingCall = خطأ;  // Iterating into a CCall to a CFunc
    منطقي m_ignore_پاتې_شوی = خطأ;  // Ignore remaining statements in the block
    منطقي m_دننه_CFunc = خطأ;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    صنف لیدنه_بنسټ final {
        // MEMBERS
        uint64_t m_saved_شمېر;  // Count before visit applied
        Ast_غوټه* ثابت m_nodep;  // Node this RAII helper is visiting
        ډېری_شمېر_Visitor* ثابت m_visitor;  // Visitor whose start/endVisitBase are called

    عام:
        // CONSTRUCTORS
        لیدنه_بنسټ(ډېری_شمېر_Visitor* visitor, Ast_غوټه* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_شمېر = m_visitor->پیل_لیدنه_بنسټ(nodep);
        }
        ~لیدنه_بنسټ() { m_visitor->پای_ټکی_لیدنه_بنسټ(m_saved_شمېر, m_nodep); }

    خاص:
        VL_UNCOPYABLE(لیدنه_بنسټ);
    };

عام:
    // CONSTRUCTORS
    څرګند ډېری_شمېر_Visitor(Ast_غوټه* nodep) { iterateConstNull(nodep); }
    ~ډېری_شمېر_Visitor() override = افتراضي;

    // METHODS
    uint64_t ډېری_اندازه() ثابت { أرجع m_ډېری_اندازه + m_callMax; }

خاص:
    فراغ reset() {
        m_ډېری_اندازه = 0;
        m_ignore_پاتې_شوی = خطأ;
    }
    uint64_t پیل_لیدنه_بنسټ(Ast_غوټه* nodep) {
        UASSERT_OBJ(!m_ignore_پاتې_شوی, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ثابت uint64_t saved_شمېر = m_ډېری_اندازه;
        m_ډېری_اندازه = 0;
        أرجع saved_شمېر;
    }
    فراغ پای_ټکی_لیدنه_بنسټ(uint64_t saved_شمېر, ثابت Ast_غوټه* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::کیڼ << m_ډېری_اندازه << "  " << nodep);
        إذا (!m_ignore_پاتې_شوی) m_ډېری_اندازه += saved_شمېر;
    }

    // VISITORS
    فراغ لیدنه(Ast_غوټه_If* nodep) override {
        إذا (m_ignore_پاتې_شوی) أرجع;
        ثابت لیدنه_بنسټ vb{هذا, nodep};
        iterate_And_بل_Const_Null(nodep->condp());
        ثابت uint64_t saved_شمېر = m_ډېری_اندازه;

        UINFO(8, "thensp:");
        reset();
        iterate_And_بل_Const_Null(nodep->thensp());
        uint64_t if_شمېر = m_ډېری_اندازه;
        إذا (nodep->branchPred().unlikely()) if_شمېر = 0;

        UINFO(8, "elsesp:");
        reset();
        iterate_And_بل_Const_Null(nodep->elsesp());
        uint64_t else_شمېر = m_ډېری_اندازه;
        إذا (nodep->branchPred().likely()) else_شمېر = 0;

        reset();
        إذا (if_شمېر >= else_شمېر) {
            m_ډېری_اندازه = saved_شمېر + if_شمېر;
            إذا (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } وإلا {
            m_ډېری_اندازه = saved_شمېر + else_شمېر;
            إذا (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    فراغ لیدنه(AstCond* nodep) override {
        إذا (m_ignore_پاتې_شوی) أرجع;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ثابت لیدنه_بنسټ vb{هذا, nodep};
        iterate_And_بل_Const_Null(nodep->condp());
        ثابت uint64_t saved_شمېر = m_ډېری_اندازه;

        UINFO(8, "?");
        reset();
        iterate_And_بل_Const_Null(nodep->thenp());
        ثابت uint64_t if_شمېر = m_ډېری_اندازه;

        UINFO(8, ":");
        reset();
        iterate_And_بل_Const_Null(nodep->elsep());
        ثابت uint64_t else_شمېر = m_ډېری_اندازه;

        reset();
        إذا (if_شمېر >= else_شمېر) {
            m_ډېری_اندازه = saved_شمېر + if_شمېر;
            إذا (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } وإلا {
            m_ډېری_اندازه = saved_شمېر + else_شمېر;
            إذا (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    فراغ لیدنه(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    فراغ لیدنه(Ast_غوټه_CCall* nodep) override {
        إذا (m_ignore_پاتې_شوی) أرجع;
        ثابت لیدنه_بنسټ vb{هذا, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = ریښتیا;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    فراغ لیدنه(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        إذا (!m_tracingCall && !nodep->ثبت_ټکی()) أرجع;
        m_tracingCall = خطأ;
        إذا (nodep->recursive()) أرجع;
        إذا (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignore_پاتې_شوی);
            VL_RESTORER(m_ډېری_اندازه);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_دننه_CFunc);
            m_tracingCall = خطأ;
            m_ډېری_اندازه = 0;
            m_callMax = 0;
            m_دننه_CFunc = ریښتیا;
            ثابت لیدنه_بنسټ vb{هذا, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_ډېری_اندازه + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = خطأ;
    }
    فراغ لیدنه(AstVar* nodep) override {
        إذا (!m_دننه_CFunc) أرجع;
        m_ډېری_اندازه += nodep->isRef() ? حجم(فراغ*) : nodep->dtypep()->پلنوالی_ټول_بایټونه();
        iterateChildrenConst(nodep);
    }

    فراغ لیدنه(Ast_غوټه_بیان* nodep) override {}  // Short-circuit
    فراغ لیدنه(Ast_غوټه* nodep) override {
        إذا (m_ignore_پاتې_شوی) أرجع;
        ثابت لیدنه_بنسټ vb{هذا, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(ډېری_شمېر_Visitor);
};

uint64_t V3_ډېری_شمېر::شمېر(Ast_غوټه* nodep) {
    ثابت ډېری_شمېر_Visitor visitor{nodep};
    أرجع visitor.ډېری_اندازه();
}
