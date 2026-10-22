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

वग्ग राशिः_गणनम्_Visitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    ध्रुव VNUser2InUse m_inuser2;

    // MEMBERS
    uint64_t m_राशिः_Size = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    तार्किक m_tracingCall = मिच्छा;  // Iterating into a CCall to a CFunc
    तार्किक m_ignoreRemaining = मिच्छा;  // Ignore remaining statements in the block
    तार्किक m_inCFunc = मिच्छा;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    वग्ग VisitBase final {
        // MEMBERS
        uint64_t m_saved_गणनम्;  // Count before visit applied
        Ast_ग्रन्थिः* ध्रुव m_nodep;  // Node this RAII helper is visiting
        राशिः_गणनम्_Visitor* ध्रुव m_visitor;  // Visitor whose start/endVisitBase are called

    सार्वजनिक:
        // CONSTRUCTORS
        VisitBase(राशिः_गणनम्_Visitor* visitor, Ast_ग्रन्थिः* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_गणनम् = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_गणनम्, m_nodep); }

    गुप्त:
        VL_UNCOPYABLE(VisitBase);
    };

सार्वजनिक:
    // CONSTRUCTORS
    explicit राशिः_गणनम्_Visitor(Ast_ग्रन्थिः* nodep) { iterateConstNull(nodep); }
    ~राशिः_गणनम्_Visitor() override = स्वभाव;

    // METHODS
    uint64_t राशिः_Size() ध्रुव { पटिनिवत्त m_राशिः_Size + m_callMax; }

गुप्त:
    शून्य reset() {
        m_राशिः_Size = 0;
        m_ignoreRemaining = मिच्छा;
    }
    uint64_t startVisitBase(Ast_ग्रन्थिः* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ध्रुव uint64_t saved_गणनम् = m_राशिः_Size;
        m_राशिः_Size = 0;
        पटिनिवत्त saved_गणनम्;
    }
    शून्य endVisitBase(uint64_t saved_गणनम्, ध्रुव Ast_ग्रन्थिः* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::वामः << m_राशिः_Size << "  " << nodep);
        सचे (!m_ignoreRemaining) m_राशिः_Size += saved_गणनम्;
    }

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_If* nodep) override {
        सचे (m_ignoreRemaining) पटिनिवत्त;
        ध्रुव VisitBase vb{एतत्, nodep};
        iterateAndNextConstNull(nodep->condp());
        ध्रुव uint64_t saved_गणनम् = m_राशिः_Size;

        UINFO(8, "thensp:");
        reset();
        iterateAndNextConstNull(nodep->thensp());
        uint64_t if_गणनम् = m_राशिः_Size;
        सचे (nodep->branchPred().unlikely()) if_गणनम् = 0;

        UINFO(8, "elsesp:");
        reset();
        iterateAndNextConstNull(nodep->elsesp());
        uint64_t else_गणनम् = m_राशिः_Size;
        सचे (nodep->branchPred().likely()) else_गणनम् = 0;

        reset();
        सचे (if_गणनम् >= else_गणनम्) {
            m_राशिः_Size = saved_गणनम् + if_गणनम्;
            सचे (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } अञ्ञथा {
            m_राशिः_Size = saved_गणनम् + else_गणनम्;
            सचे (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    शून्य visit(AstCond* nodep) override {
        सचे (m_ignoreRemaining) पटिनिवत्त;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ध्रुव VisitBase vb{एतत्, nodep};
        iterateAndNextConstNull(nodep->condp());
        ध्रुव uint64_t saved_गणनम् = m_राशिः_Size;

        UINFO(8, "?");
        reset();
        iterateAndNextConstNull(nodep->thenp());
        ध्रुव uint64_t if_गणनम् = m_राशिः_Size;

        UINFO(8, ":");
        reset();
        iterateAndNextConstNull(nodep->elsep());
        ध्रुव uint64_t else_गणनम् = m_राशिः_Size;

        reset();
        सचे (if_गणनम् >= else_गणनम्) {
            m_राशिः_Size = saved_गणनम् + if_गणनम्;
            सचे (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } अञ्ञथा {
            m_राशिः_Size = saved_गणनम् + else_गणनम्;
            सचे (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    शून्य visit(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    शून्य visit(Ast_ग्रन्थिः_CCall* nodep) override {
        सचे (m_ignoreRemaining) पटिनिवत्त;
        ध्रुव VisitBase vb{एतत्, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = सच्च;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    शून्य visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        सचे (!m_tracingCall && !nodep->entryPoint()) पटिनिवत्त;
        m_tracingCall = मिच्छा;
        सचे (nodep->recursive()) पटिनिवत्त;
        सचे (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignoreRemaining);
            VL_RESTORER(m_राशिः_Size);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_inCFunc);
            m_tracingCall = मिच्छा;
            m_राशिः_Size = 0;
            m_callMax = 0;
            m_inCFunc = सच्च;
            ध्रुव VisitBase vb{एतत्, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_राशिः_Size + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = मिच्छा;
    }
    शून्य visit(AstVar* nodep) override {
        सचे (!m_inCFunc) पटिनिवत्त;
        m_राशिः_Size += nodep->isRef() ? परिमाण(शून्य*) : nodep->dtypep()->widthTotalBytes();
        iterateChildrenConst(nodep);
    }

    शून्य visit(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) override {}  // Short-circuit
    शून्य visit(Ast_ग्रन्थिः* nodep) override {
        सचे (m_ignoreRemaining) पटिनिवत्त;
        ध्रुव VisitBase vb{एतत्, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(राशिः_गणनम्_Visitor);
};

uint64_t V3_राशिः_गणनम्::गणनम्(Ast_ग्रन्थिः* nodep) {
    ध्रुव राशिः_गणनम्_Visitor visitor{nodep};
    पटिनिवत्त visitor.राशिः_Size();
}
