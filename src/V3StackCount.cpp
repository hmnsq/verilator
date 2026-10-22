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

वर्ग राशिः_गणनम्_Visitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    ध्रुव VNUser2_अन्तर्_Use m_inuser2;

    // MEMBERS
    uint64_t m_राशिः_आकार = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    तार्किक m_tracingCall = असत्य;  // Iterating into a CCall to a CFunc
    तार्किक m_ignoreRemaining = असत्य;  // Ignore remaining statements in the block
    तार्किक m_अन्तर्_CFunc = असत्य;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    वर्ग दर्शय_आधारः final {
        // MEMBERS
        uint64_t m_saved_गणनम्;  // Count before visit applied
        Ast_ग्रन्थिः* ध्रुव m_nodep;  // Node this RAII helper is visiting
        राशिः_गणनम्_Visitor* ध्रुव m_visitor;  // Visitor whose start/endVisitBase are called

    सार्वजनिक:
        // CONSTRUCTORS
        दर्शय_आधारः(राशिः_गणनम्_Visitor* visitor, Ast_ग्रन्थिः* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_गणनम् = m_visitor->start_दर्शय_आधारः(nodep);
        }
        ~दर्शय_आधारः() { m_visitor->अन्तः_दर्शय_आधारः(m_saved_गणनम्, m_nodep); }

    गुप्त:
        VL_UNCOPYABLE(दर्शय_आधारः);
    };

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट राशिः_गणनम्_Visitor(Ast_ग्रन्थिः* nodep) { iterateConstNull(nodep); }
    ~राशिः_गणनम्_Visitor() override = स्वभाव;

    // METHODS
    uint64_t राशिः_आकार() ध्रुव { प्रत्यागम m_राशिः_आकार + m_callMax; }

गुप्त:
    शून्य reset() {
        m_राशिः_आकार = 0;
        m_ignoreRemaining = असत्य;
    }
    uint64_t start_दर्शय_आधारः(Ast_ग्रन्थिः* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ध्रुव uint64_t saved_गणनम् = m_राशिः_आकार;
        m_राशिः_आकार = 0;
        प्रत्यागम saved_गणनम्;
    }
    शून्य अन्तः_दर्शय_आधारः(uint64_t saved_गणनम्, ध्रुव Ast_ग्रन्थिः* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::वामतः << m_राशिः_आकार << "  " << nodep);
        यदि (!m_ignoreRemaining) m_राशिः_आकार += saved_गणनम्;
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_If* nodep) override {
        यदि (m_ignoreRemaining) प्रत्यागम;
        ध्रुव दर्शय_आधारः vb{एतत्, nodep};
        iterate_And_अग्र_Const_Null(nodep->condp());
        ध्रुव uint64_t saved_गणनम् = m_राशिः_आकार;

        UINFO(8, "thensp:");
        reset();
        iterate_And_अग्र_Const_Null(nodep->thensp());
        uint64_t if_गणनम् = m_राशिः_आकार;
        यदि (nodep->branchPred().unlikely()) if_गणनम् = 0;

        UINFO(8, "elsesp:");
        reset();
        iterate_And_अग्र_Const_Null(nodep->elsesp());
        uint64_t else_गणनम् = m_राशिः_आकार;
        यदि (nodep->branchPred().likely()) else_गणनम् = 0;

        reset();
        यदि (if_गणनम् >= else_गणनम्) {
            m_राशिः_आकार = saved_गणनम् + if_गणनम्;
            यदि (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } अन्यथा {
            m_राशिः_आकार = saved_गणनम् + else_गणनम्;
            यदि (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    शून्य दर्शय(AstCond* nodep) override {
        यदि (m_ignoreRemaining) प्रत्यागम;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ध्रुव दर्शय_आधारः vb{एतत्, nodep};
        iterate_And_अग्र_Const_Null(nodep->condp());
        ध्रुव uint64_t saved_गणनम् = m_राशिः_आकार;

        UINFO(8, "?");
        reset();
        iterate_And_अग्र_Const_Null(nodep->thenp());
        ध्रुव uint64_t if_गणनम् = m_राशिः_आकार;

        UINFO(8, ":");
        reset();
        iterate_And_अग्र_Const_Null(nodep->elsep());
        ध्रुव uint64_t else_गणनम् = m_राशिः_आकार;

        reset();
        यदि (if_गणनम् >= else_गणनम्) {
            m_राशिः_आकार = saved_गणनम् + if_गणनम्;
            यदि (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } अन्यथा {
            m_राशिः_आकार = saved_गणनम् + else_गणनम्;
            यदि (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    शून्य दर्शय(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    शून्य दर्शय(Ast_ग्रन्थिः_CCall* nodep) override {
        यदि (m_ignoreRemaining) प्रत्यागम;
        ध्रुव दर्शय_आधारः vb{एतत्, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = सत्य;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    शून्य दर्शय(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        यदि (!m_tracingCall && !nodep->प्रविष्टिः_Point()) प्रत्यागम;
        m_tracingCall = असत्य;
        यदि (nodep->recursive()) प्रत्यागम;
        यदि (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignoreRemaining);
            VL_RESTORER(m_राशिः_आकार);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_अन्तर्_CFunc);
            m_tracingCall = असत्य;
            m_राशिः_आकार = 0;
            m_callMax = 0;
            m_अन्तर्_CFunc = सत्य;
            ध्रुव दर्शय_आधारः vb{एतत्, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_राशिः_आकार + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = असत्य;
    }
    शून्य दर्शय(AstVar* nodep) override {
        यदि (!m_अन्तर्_CFunc) प्रत्यागम;
        m_राशिः_आकार += nodep->isRef() ? परिमाण(शून्य*) : nodep->dtypep()->विस्तारः_योग_अष्टकानि();
        iterateChildrenConst(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) override {}  // Short-circuit
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        यदि (m_ignoreRemaining) प्रत्यागम;
        ध्रुव दर्शय_आधारः vb{एतत्, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(राशिः_गणनम्_Visitor);
};

uint64_t V3_राशिः_गणनम्::गणनम्(Ast_ग्रन्थिः* nodep) {
    ध्रुव राशिः_गणनम्_Visitor visitor{nodep};
    प्रत्यागम visitor.राशिः_आकार();
}
