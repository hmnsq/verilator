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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3StackCount.h"

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the stack byte size for executing all logic within and below a
/// given AST node. This is very rough, only for warning when stack is too
/// small.

वर्ग थाक_गन्ती_Visitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    अचर VNUser2InUse m_inuser2;

    // MEMBERS
    uint64_t m_थाक_Size = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    तार्किक m_tracingCall = असत्य;  // Iterating into a CCall to a CFunc
    तार्किक m_ignoreRemaining = असत्य;  // Ignore remaining statements in the block
    तार्किक m_inCFunc = असत्य;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    वर्ग VisitBase final {
        // MEMBERS
        uint64_t m_saved_गन्ती;  // Count before visit applied
        Ast_गाँठो* अचर m_nodep;  // Node this RAII helper is visiting
        थाक_गन्ती_Visitor* अचर m_visitor;  // Visitor whose start/endVisitBase are called

    सार्वजनिक:
        // CONSTRUCTORS
        VisitBase(थाक_गन्ती_Visitor* visitor, Ast_गाँठो* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_गन्ती = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_गन्ती, m_nodep); }

    गुप्त:
        VL_UNCOPYABLE(VisitBase);
    };

सार्वजनिक:
    // CONSTRUCTORS
    explicit थाक_गन्ती_Visitor(Ast_गाँठो* nodep) { iterateConstNull(nodep); }
    ~थाक_गन्ती_Visitor() override = स्वभाव;

    // METHODS
    uint64_t थाक_Size() अचर { फर्काऊ m_थाक_Size + m_callMax; }

गुप्त:
    शून्य reset() {
        m_थाक_Size = 0;
        m_ignoreRemaining = असत्य;
    }
    uint64_t startVisitBase(Ast_गाँठो* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        अचर uint64_t saved_गन्ती = m_थाक_Size;
        m_थाक_Size = 0;
        फर्काऊ saved_गन्ती;
    }
    शून्य endVisitBase(uint64_t saved_गन्ती, अचर Ast_गाँठो* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::बायाँ << m_थाक_Size << "  " << nodep);
        यदि (!m_ignoreRemaining) m_थाक_Size += saved_गन्ती;
    }

    // VISITORS
    शून्य visit(Ast_गाँठो_If* nodep) override {
        यदि (m_ignoreRemaining) फर्काऊ;
        अचर VisitBase vb{एतत्, nodep};
        iterateAndNextConstNull(nodep->condp());
        अचर uint64_t saved_गन्ती = m_थाक_Size;

        UINFO(8, "thensp:");
        reset();
        iterateAndNextConstNull(nodep->thensp());
        uint64_t if_गन्ती = m_थाक_Size;
        यदि (nodep->branchPred().unlikely()) if_गन्ती = 0;

        UINFO(8, "elsesp:");
        reset();
        iterateAndNextConstNull(nodep->elsesp());
        uint64_t else_गन्ती = m_थाक_Size;
        यदि (nodep->branchPred().likely()) else_गन्ती = 0;

        reset();
        यदि (if_गन्ती >= else_गन्ती) {
            m_थाक_Size = saved_गन्ती + if_गन्ती;
            यदि (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } अन्यथा {
            m_थाक_Size = saved_गन्ती + else_गन्ती;
            यदि (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    शून्य visit(AstCond* nodep) override {
        यदि (m_ignoreRemaining) फर्काऊ;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        अचर VisitBase vb{एतत्, nodep};
        iterateAndNextConstNull(nodep->condp());
        अचर uint64_t saved_गन्ती = m_थाक_Size;

        UINFO(8, "?");
        reset();
        iterateAndNextConstNull(nodep->thenp());
        अचर uint64_t if_गन्ती = m_थाक_Size;

        UINFO(8, ":");
        reset();
        iterateAndNextConstNull(nodep->elsep());
        अचर uint64_t else_गन्ती = m_थाक_Size;

        reset();
        यदि (if_गन्ती >= else_गन्ती) {
            m_थाक_Size = saved_गन्ती + if_गन्ती;
            यदि (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } अन्यथा {
            m_थाक_Size = saved_गन्ती + else_गन्ती;
            यदि (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    शून्य visit(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    शून्य visit(Ast_गाँठो_CCall* nodep) override {
        यदि (m_ignoreRemaining) फर्काऊ;
        अचर VisitBase vb{एतत्, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = सत्य;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    शून्य visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        यदि (!m_tracingCall && !nodep->entryPoint()) फर्काऊ;
        m_tracingCall = असत्य;
        यदि (nodep->recursive()) फर्काऊ;
        यदि (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignoreRemaining);
            VL_RESTORER(m_थाक_Size);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_inCFunc);
            m_tracingCall = असत्य;
            m_थाक_Size = 0;
            m_callMax = 0;
            m_inCFunc = सत्य;
            अचर VisitBase vb{एतत्, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_थाक_Size + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = असत्य;
    }
    शून्य visit(AstVar* nodep) override {
        यदि (!m_inCFunc) फर्काऊ;
        m_थाक_Size += nodep->isRef() ? परिमाण(शून्य*) : nodep->dtypep()->widthTotalBytes();
        iterateChildrenConst(nodep);
    }

    शून्य visit(Ast_गाँठो_अभिव्यक्ति* nodep) override {}  // Short-circuit
    शून्य visit(Ast_गाँठो* nodep) override {
        यदि (m_ignoreRemaining) फर्काऊ;
        अचर VisitBase vb{एतत्, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(थाक_गन्ती_Visitor);
};

uint64_t V3_थाक_गन्ती::गन्ती(Ast_गाँठो* nodep) {
    अचर थाक_गन्ती_Visitor visitor{nodep};
    फर्काऊ visitor.थाक_Size();
}
