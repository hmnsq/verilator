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

جماعت ڈھیر_گنتی_Visitor final : عوامی VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    ثابت VNUser2_اندر_Use m_inuser2;

    // MEMBERS
    uint64_t m_ڈھیر_سائز = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    منطقی m_tracingCall = جھوٹ;  // Iterating into a CCall to a CFunc
    منطقی m_ignore_بقایا = جھوٹ;  // Ignore remaining statements in the block
    منطقی m_اندر_CFunc = جھوٹ;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    جماعت دورہ_بنیاد final {
        // MEMBERS
        uint64_t m_saved_گنتی;  // Count before visit applied
        Ast_گرہ* ثابت m_nodep;  // Node this RAII helper is visiting
        ڈھیر_گنتی_Visitor* ثابت m_visitor;  // Visitor whose start/endVisitBase are called

    عوامی:
        // CONSTRUCTORS
        دورہ_بنیاد(ڈھیر_گنتی_Visitor* visitor, Ast_گرہ* nodep)
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
    صریح ڈھیر_گنتی_Visitor(Ast_گرہ* nodep) { iterateConstNull(nodep); }
    ~ڈھیر_گنتی_Visitor() override = طےشدہ;

    // METHODS
    uint64_t ڈھیر_سائز() ثابت { واپس m_ڈھیر_سائز + m_callMax; }

نجی:
    خالی reset() {
        m_ڈھیر_سائز = 0;
        m_ignore_بقایا = جھوٹ;
    }
    uint64_t آغاز_دورہ_بنیاد(Ast_گرہ* nodep) {
        UASSERT_OBJ(!m_ignore_بقایا, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ثابت uint64_t saved_گنتی = m_ڈھیر_سائز;
        m_ڈھیر_سائز = 0;
        واپس saved_گنتی;
    }
    خالی انتہا_دورہ_بنیاد(uint64_t saved_گنتی, ثابت Ast_گرہ* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::بائیں << m_ڈھیر_سائز << "  " << nodep);
        اگر (!m_ignore_بقایا) m_ڈھیر_سائز += saved_گنتی;
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_If* nodep) override {
        اگر (m_ignore_بقایا) واپس;
        ثابت دورہ_بنیاد vb{یہ, nodep};
        iterate_And_اگلا_Const_Null(nodep->condp());
        ثابت uint64_t saved_گنتی = m_ڈھیر_سائز;

        UINFO(8, "thensp:");
        reset();
        iterate_And_اگلا_Const_Null(nodep->thensp());
        uint64_t if_گنتی = m_ڈھیر_سائز;
        اگر (nodep->branchPred().unlikely()) if_گنتی = 0;

        UINFO(8, "elsesp:");
        reset();
        iterate_And_اگلا_Const_Null(nodep->elsesp());
        uint64_t else_گنتی = m_ڈھیر_سائز;
        اگر (nodep->branchPred().likely()) else_گنتی = 0;

        reset();
        اگر (if_گنتی >= else_گنتی) {
            m_ڈھیر_سائز = saved_گنتی + if_گنتی;
            اگر (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } ورنہ {
            m_ڈھیر_سائز = saved_گنتی + else_گنتی;
            اگر (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    خالی دورہ(AstCond* nodep) override {
        اگر (m_ignore_بقایا) واپس;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ثابت دورہ_بنیاد vb{یہ, nodep};
        iterate_And_اگلا_Const_Null(nodep->condp());
        ثابت uint64_t saved_گنتی = m_ڈھیر_سائز;

        UINFO(8, "?");
        reset();
        iterate_And_اگلا_Const_Null(nodep->thenp());
        ثابت uint64_t if_گنتی = m_ڈھیر_سائز;

        UINFO(8, ":");
        reset();
        iterate_And_اگلا_Const_Null(nodep->elsep());
        ثابت uint64_t else_گنتی = m_ڈھیر_سائز;

        reset();
        اگر (if_گنتی >= else_گنتی) {
            m_ڈھیر_سائز = saved_گنتی + if_گنتی;
            اگر (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } ورنہ {
            m_ڈھیر_سائز = saved_گنتی + else_گنتی;
            اگر (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    خالی دورہ(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    خالی دورہ(Ast_گرہ_CCall* nodep) override {
        اگر (m_ignore_بقایا) واپس;
        ثابت دورہ_بنیاد vb{یہ, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = سچ;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    خالی دورہ(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        اگر (!m_tracingCall && !nodep->اندراج_نقطہ()) واپس;
        m_tracingCall = جھوٹ;
        اگر (nodep->recursive()) واپس;
        اگر (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignore_بقایا);
            VL_RESTORER(m_ڈھیر_سائز);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_اندر_CFunc);
            m_tracingCall = جھوٹ;
            m_ڈھیر_سائز = 0;
            m_callMax = 0;
            m_اندر_CFunc = سچ;
            ثابت دورہ_بنیاد vb{یہ, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_ڈھیر_سائز + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = جھوٹ;
    }
    خالی دورہ(AstVar* nodep) override {
        اگر (!m_اندر_CFunc) واپس;
        m_ڈھیر_سائز += nodep->isRef() ? حجم(خالی*) : nodep->dtypep()->چوڑائی_کل_بائٹیں();
        iterateChildrenConst(nodep);
    }

    خالی دورہ(Ast_گرہ_اظہاریہ* nodep) override {}  // Short-circuit
    خالی دورہ(Ast_گرہ* nodep) override {
        اگر (m_ignore_بقایا) واپس;
        ثابت دورہ_بنیاد vb{یہ, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(ڈھیر_گنتی_Visitor);
};

uint64_t V3_ڈھیر_گنتی::گنتی(Ast_گرہ* nodep) {
    ثابت ڈھیر_گنتی_Visitor visitor{nodep};
    واپس visitor.ڈھیر_سائز();
}
