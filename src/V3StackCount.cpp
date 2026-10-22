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

صنف ڍير_ڳڻپ_Visitor final : عام VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    ثابت VNUser2_اندريون_Use m_inuser2;

    // MEMBERS
    uint64_t m_ڍير_سائيز = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    منطقي m_tracingCall = خطأ;  // Iterating into a CCall to a CFunc
    منطقي m_ignore_بچيل = خطأ;  // Ignore remaining statements in the block
    منطقي m_اندريون_CFunc = خطأ;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    صنف گهمو_بنياد final {
        // MEMBERS
        uint64_t m_saved_ڳڻپ;  // Count before visit applied
        Ast_ڳنڍ* ثابت m_nodep;  // Node this RAII helper is visiting
        ڍير_ڳڻپ_Visitor* ثابت m_visitor;  // Visitor whose start/endVisitBase are called

    عام:
        // CONSTRUCTORS
        گهمو_بنياد(ڍير_ڳڻپ_Visitor* visitor, Ast_ڳنڍ* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_ڳڻپ = m_visitor->شروعات_گهمو_بنياد(nodep);
        }
        ~گهمو_بنياد() { m_visitor->پڇاڙي_گهمو_بنياد(m_saved_ڳڻپ, m_nodep); }

    خاص:
        VL_UNCOPYABLE(گهمو_بنياد);
    };

عام:
    // CONSTRUCTORS
    واضح ڍير_ڳڻپ_Visitor(Ast_ڳنڍ* nodep) { iterateConstNull(nodep); }
    ~ڍير_ڳڻپ_Visitor() override = افتراضي;

    // METHODS
    uint64_t ڍير_سائيز() ثابت { أرجع m_ڍير_سائيز + m_callMax; }

خاص:
    فراغ reset() {
        m_ڍير_سائيز = 0;
        m_ignore_بچيل = خطأ;
    }
    uint64_t شروعات_گهمو_بنياد(Ast_ڳنڍ* nodep) {
        UASSERT_OBJ(!m_ignore_بچيل, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ثابت uint64_t saved_ڳڻپ = m_ڍير_سائيز;
        m_ڍير_سائيز = 0;
        أرجع saved_ڳڻپ;
    }
    فراغ پڇاڙي_گهمو_بنياد(uint64_t saved_ڳڻپ, ثابت Ast_ڳنڍ* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::کاٻي << m_ڍير_سائيز << "  " << nodep);
        إذا (!m_ignore_بچيل) m_ڍير_سائيز += saved_ڳڻپ;
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_If* nodep) override {
        إذا (m_ignore_بچيل) أرجع;
        ثابت گهمو_بنياد vb{هذا, nodep};
        iterate_And_ايندڙ_Const_Null(nodep->condp());
        ثابت uint64_t saved_ڳڻپ = m_ڍير_سائيز;

        UINFO(8, "thensp:");
        reset();
        iterate_And_ايندڙ_Const_Null(nodep->thensp());
        uint64_t if_ڳڻپ = m_ڍير_سائيز;
        إذا (nodep->branchPred().unlikely()) if_ڳڻپ = 0;

        UINFO(8, "elsesp:");
        reset();
        iterate_And_ايندڙ_Const_Null(nodep->elsesp());
        uint64_t else_ڳڻپ = m_ڍير_سائيز;
        إذا (nodep->branchPred().likely()) else_ڳڻپ = 0;

        reset();
        إذا (if_ڳڻپ >= else_ڳڻپ) {
            m_ڍير_سائيز = saved_ڳڻپ + if_ڳڻپ;
            إذا (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } وإلا {
            m_ڍير_سائيز = saved_ڳڻپ + else_ڳڻپ;
            إذا (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    فراغ گهمو(AstCond* nodep) override {
        إذا (m_ignore_بچيل) أرجع;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ثابت گهمو_بنياد vb{هذا, nodep};
        iterate_And_ايندڙ_Const_Null(nodep->condp());
        ثابت uint64_t saved_ڳڻپ = m_ڍير_سائيز;

        UINFO(8, "?");
        reset();
        iterate_And_ايندڙ_Const_Null(nodep->thenp());
        ثابت uint64_t if_ڳڻپ = m_ڍير_سائيز;

        UINFO(8, ":");
        reset();
        iterate_And_ايندڙ_Const_Null(nodep->elsep());
        ثابت uint64_t else_ڳڻپ = m_ڍير_سائيز;

        reset();
        إذا (if_ڳڻپ >= else_ڳڻپ) {
            m_ڍير_سائيز = saved_ڳڻپ + if_ڳڻپ;
            إذا (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } وإلا {
            m_ڍير_سائيز = saved_ڳڻپ + else_ڳڻپ;
            إذا (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    فراغ گهمو(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    فراغ گهمو(Ast_ڳنڍ_CCall* nodep) override {
        إذا (m_ignore_بچيل) أرجع;
        ثابت گهمو_بنياد vb{هذا, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = سچ;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    فراغ گهمو(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        إذا (!m_tracingCall && !nodep->داخلا_نقطو()) أرجع;
        m_tracingCall = خطأ;
        إذا (nodep->recursive()) أرجع;
        إذا (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignore_بچيل);
            VL_RESTORER(m_ڍير_سائيز);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_اندريون_CFunc);
            m_tracingCall = خطأ;
            m_ڍير_سائيز = 0;
            m_callMax = 0;
            m_اندريون_CFunc = سچ;
            ثابت گهمو_بنياد vb{هذا, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_ڍير_سائيز + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = خطأ;
    }
    فراغ گهمو(AstVar* nodep) override {
        إذا (!m_اندريون_CFunc) أرجع;
        m_ڍير_سائيز += nodep->isRef() ? حجم(فراغ*) : nodep->dtypep()->ويڪر_ڪل_بائيٽون();
        iterateChildrenConst(nodep);
    }

    فراغ گهمو(Ast_ڳنڍ_اظهار* nodep) override {}  // Short-circuit
    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        إذا (m_ignore_بچيل) أرجع;
        ثابت گهمو_بنياد vb{هذا, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(ڍير_ڳڻپ_Visitor);
};

uint64_t V3_ڍير_ڳڻپ::ڳڻپ(Ast_ڳنڍ* nodep) {
    ثابت ڍير_ڳڻپ_Visitor visitor{nodep};
    أرجع visitor.ڍير_سائيز();
}
