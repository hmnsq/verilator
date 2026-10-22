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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3StackCount.h"

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the stack byte size for executing all logic within and below a
/// given AST node. This is very rough, only for warning when stack is too
/// small.

श्रेणी ढेर_गिनती_Visitor final : खुला VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    स्थिर VNUser2InUse m_inuser2;

    // MEMBERS
    uint64_t m_ढेर_आकार = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    bool m_tracingCall = false;  // Iterating into a CCall to a CFunc
    bool m_ignoreRemaining = false;  // Ignore remaining statements in the block
    bool m_inCFunc = false;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    श्रेणी VisitBase final {
        // MEMBERS
        uint64_t m_saved_गिनती;  // Count before visit applied
        Ast_गाँठ* स्थिर m_nodep;  // Node this RAII helper is visiting
        ढेर_गिनती_Visitor* स्थिर m_visitor;  // Visitor whose start/endVisitBase are called

    खुला:
        // CONSTRUCTORS
        VisitBase(ढेर_गिनती_Visitor* visitor, Ast_गाँठ* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_गिनती = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_गिनती, m_nodep); }

    निजी:
        VL_UNCOPYABLE(VisitBase);
    };

खुला:
    // CONSTRUCTORS
    explicit ढेर_गिनती_Visitor(Ast_गाँठ* nodep) { iterateConstNull(nodep); }
    ~ढेर_गिनती_Visitor() override = शेष;

    // METHODS
    uint64_t ढेर_आकार() स्थिर { वापस m_ढेर_आकार + m_callMax; }

निजी:
    व्योम reset() {
        m_ढेर_आकार = 0;
        m_ignoreRemaining = false;
    }
    uint64_t startVisitBase(Ast_गाँठ* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        स्थिर uint64_t saved_गिनती = m_ढेर_आकार;
        m_ढेर_आकार = 0;
        वापस saved_गिनती;
    }
    व्योम endVisitBase(uint64_t saved_गिनती, स्थिर Ast_गाँठ* nodep) {
        UINFO(8, "cost " << std::रखो_चौड़ाई(6) << std::बाँए << m_ढेर_आकार << "  " << nodep);
        अगर (!m_ignoreRemaining) m_ढेर_आकार += saved_गिनती;
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_If* nodep) override {
        अगर (m_ignoreRemaining) वापस;
        स्थिर VisitBase vb{यह, nodep};
        iterate_And_अग्र_Const_Null(nodep->condp());
        स्थिर uint64_t saved_गिनती = m_ढेर_आकार;

        UINFO(8, "thensp:");
        reset();
        iterate_And_अग्र_Const_Null(nodep->thensp());
        uint64_t if_गिनती = m_ढेर_आकार;
        अगर (nodep->branchPred().unlikely()) if_गिनती = 0;

        UINFO(8, "elsesp:");
        reset();
        iterate_And_अग्र_Const_Null(nodep->elsesp());
        uint64_t else_गिनती = m_ढेर_आकार;
        अगर (nodep->branchPred().likely()) else_गिनती = 0;

        reset();
        अगर (if_गिनती >= else_गिनती) {
            m_ढेर_आकार = saved_गिनती + if_गिनती;
            अगर (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } अन्यथा {
            m_ढेर_आकार = saved_गिनती + else_गिनती;
            अगर (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    व्योम visit(AstCond* nodep) override {
        अगर (m_ignoreRemaining) वापस;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        स्थिर VisitBase vb{यह, nodep};
        iterate_And_अग्र_Const_Null(nodep->condp());
        स्थिर uint64_t saved_गिनती = m_ढेर_आकार;

        UINFO(8, "?");
        reset();
        iterate_And_अग्र_Const_Null(nodep->thenp());
        स्थिर uint64_t if_गिनती = m_ढेर_आकार;

        UINFO(8, ":");
        reset();
        iterate_And_अग्र_Const_Null(nodep->elsep());
        स्थिर uint64_t else_गिनती = m_ढेर_आकार;

        reset();
        अगर (if_गिनती >= else_गिनती) {
            m_ढेर_आकार = saved_गिनती + if_गिनती;
            अगर (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } अन्यथा {
            m_ढेर_आकार = saved_गिनती + else_गिनती;
            अगर (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    व्योम visit(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    व्योम visit(Ast_गाँठ_CCall* nodep) override {
        अगर (m_ignoreRemaining) वापस;
        स्थिर VisitBase vb{यह, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = true;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    व्योम visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        अगर (!m_tracingCall && !nodep->entryPoint()) वापस;
        m_tracingCall = false;
        अगर (nodep->recursive()) वापस;
        अगर (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignoreRemaining);
            VL_RESTORER(m_ढेर_आकार);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_inCFunc);
            m_tracingCall = false;
            m_ढेर_आकार = 0;
            m_callMax = 0;
            m_inCFunc = true;
            स्थिर VisitBase vb{यह, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_ढेर_आकार + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = false;
    }
    व्योम visit(AstVar* nodep) override {
        अगर (!m_inCFunc) वापस;
        m_ढेर_आकार += nodep->isRef() ? माप(व्योम*) : nodep->dtypep()->width_योग_Bytes();
        iterateChildrenConst(nodep);
    }

    व्योम visit(Ast_गाँठ_व्यंजक* nodep) override {}  // Short-circuit
    व्योम visit(Ast_गाँठ* nodep) override {
        अगर (m_ignoreRemaining) वापस;
        स्थिर VisitBase vb{यह, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(ढेर_गिनती_Visitor);
};

uint64_t V3_ढेर_गिनती::गिनती(Ast_गाँठ* nodep) {
    स्थिर ढेर_गिनती_Visitor visitor{nodep};
    वापस visitor.ढेर_आकार();
}
