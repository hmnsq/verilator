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

वर्ग ढेर_गिनती_Visitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    ध्रुव VNUser2InUse m_inuser2;

    // MEMBERS
    uint64_t m_ढेर_Size = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    तार्किक m_tracingCall = असत्य;  // Iterating into a CCall to a CFunc
    तार्किक m_ignoreRemaining = असत्य;  // Ignore remaining statements in the block
    तार्किक m_inCFunc = असत्य;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    वर्ग VisitBase final {
        // MEMBERS
        uint64_t m_saved_गिनती;  // Count before visit applied
        Ast_गाँठ* ध्रुव m_nodep;  // Node this RAII helper is visiting
        ढेर_गिनती_Visitor* ध्रुव m_visitor;  // Visitor whose start/endVisitBase are called

    सार्वजनिक:
        // CONSTRUCTORS
        VisitBase(ढेर_गिनती_Visitor* visitor, Ast_गाँठ* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_गिनती = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_गिनती, m_nodep); }

    गुप्त:
        VL_UNCOPYABLE(VisitBase);
    };

सार्वजनिक:
    // CONSTRUCTORS
    explicit ढेर_गिनती_Visitor(Ast_गाँठ* nodep) { iterateConstNull(nodep); }
    ~ढेर_गिनती_Visitor() override = स्वभाव;

    // METHODS
    uint64_t ढेर_Size() ध्रुव { प्रत्यागम m_ढेर_Size + m_callMax; }

गुप्त:
    शून्य reset() {
        m_ढेर_Size = 0;
        m_ignoreRemaining = असत्य;
    }
    uint64_t startVisitBase(Ast_गाँठ* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ध्रुव uint64_t saved_गिनती = m_ढेर_Size;
        m_ढेर_Size = 0;
        प्रत्यागम saved_गिनती;
    }
    शून्य endVisitBase(uint64_t saved_गिनती, ध्रुव Ast_गाँठ* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::बायाँ << m_ढेर_Size << "  " << nodep);
        यदि (!m_ignoreRemaining) m_ढेर_Size += saved_गिनती;
    }

    // VISITORS
    शून्य visit(Ast_गाँठ_If* nodep) override {
        यदि (m_ignoreRemaining) प्रत्यागम;
        ध्रुव VisitBase vb{एतत्, nodep};
        iterateAndNextConstNull(nodep->condp());
        ध्रुव uint64_t saved_गिनती = m_ढेर_Size;

        UINFO(8, "thensp:");
        reset();
        iterateAndNextConstNull(nodep->thensp());
        uint64_t if_गिनती = m_ढेर_Size;
        यदि (nodep->branchPred().unlikely()) if_गिनती = 0;

        UINFO(8, "elsesp:");
        reset();
        iterateAndNextConstNull(nodep->elsesp());
        uint64_t else_गिनती = m_ढेर_Size;
        यदि (nodep->branchPred().likely()) else_गिनती = 0;

        reset();
        यदि (if_गिनती >= else_गिनती) {
            m_ढेर_Size = saved_गिनती + if_गिनती;
            यदि (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } अन्यथा {
            m_ढेर_Size = saved_गिनती + else_गिनती;
            यदि (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    शून्य visit(AstCond* nodep) override {
        यदि (m_ignoreRemaining) प्रत्यागम;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ध्रुव VisitBase vb{एतत्, nodep};
        iterateAndNextConstNull(nodep->condp());
        ध्रुव uint64_t saved_गिनती = m_ढेर_Size;

        UINFO(8, "?");
        reset();
        iterateAndNextConstNull(nodep->thenp());
        ध्रुव uint64_t if_गिनती = m_ढेर_Size;

        UINFO(8, ":");
        reset();
        iterateAndNextConstNull(nodep->elsep());
        ध्रुव uint64_t else_गिनती = m_ढेर_Size;

        reset();
        यदि (if_गिनती >= else_गिनती) {
            m_ढेर_Size = saved_गिनती + if_गिनती;
            यदि (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } अन्यथा {
            m_ढेर_Size = saved_गिनती + else_गिनती;
            यदि (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    शून्य visit(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    शून्य visit(Ast_गाँठ_CCall* nodep) override {
        यदि (m_ignoreRemaining) प्रत्यागम;
        ध्रुव VisitBase vb{एतत्, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = सत्य;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    शून्य visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        यदि (!m_tracingCall && !nodep->entryPoint()) प्रत्यागम;
        m_tracingCall = असत्य;
        यदि (nodep->recursive()) प्रत्यागम;
        यदि (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignoreRemaining);
            VL_RESTORER(m_ढेर_Size);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_inCFunc);
            m_tracingCall = असत्य;
            m_ढेर_Size = 0;
            m_callMax = 0;
            m_inCFunc = सत्य;
            ध्रुव VisitBase vb{एतत्, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_ढेर_Size + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = असत्य;
    }
    शून्य visit(AstVar* nodep) override {
        यदि (!m_inCFunc) प्रत्यागम;
        m_ढेर_Size += nodep->isRef() ? परिमाण(शून्य*) : nodep->dtypep()->widthTotalBytes();
        iterateChildrenConst(nodep);
    }

    शून्य visit(Ast_गाँठ_व्यंजक* nodep) override {}  // Short-circuit
    शून्य visit(Ast_गाँठ* nodep) override {
        यदि (m_ignoreRemaining) प्रत्यागम;
        ध्रुव VisitBase vb{एतत्, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(ढेर_गिनती_Visitor);
};

uint64_t V3_ढेर_गिनती::गिनती(Ast_गाँठ* nodep) {
    ध्रुव ढेर_गिनती_Visitor visitor{nodep};
    प्रत्यागम visitor.ढेर_Size();
}
