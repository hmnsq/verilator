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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3StackCount.h"

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the stack byte size for executing all logic within and below a
/// given AST node. This is very rough, only for warning when stack is too
/// small.

વર્ગ ઢગલો_ગણતરી_Visitor final : જાહેર VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    અચલ VNUser2InUse m_inuser2;

    // MEMBERS
    uint64_t m_ઢગલો_Size = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    તાર્કિક m_tracingCall = ખોટું;  // Iterating into a CCall to a CFunc
    તાર્કિક m_ignoreRemaining = ખોટું;  // Ignore remaining statements in the block
    તાર્કિક m_inCFunc = ખોટું;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    વર્ગ VisitBase final {
        // MEMBERS
        uint64_t m_saved_ગણતરી;  // Count before visit applied
        Ast_ગાંઠ* અચલ m_nodep;  // Node this RAII helper is visiting
        ઢગલો_ગણતરી_Visitor* અચલ m_visitor;  // Visitor whose start/endVisitBase are called

    જાહેર:
        // CONSTRUCTORS
        VisitBase(ઢગલો_ગણતરી_Visitor* visitor, Ast_ગાંઠ* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_ગણતરી = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_ગણતરી, m_nodep); }

    ખાનગી:
        VL_UNCOPYABLE(VisitBase);
    };

જાહેર:
    // CONSTRUCTORS
    explicit ઢગલો_ગણતરી_Visitor(Ast_ગાંઠ* nodep) { iterateConstNull(nodep); }
    ~ઢગલો_ગણતરી_Visitor() override = મૂળભૂત;

    // METHODS
    uint64_t ઢગલો_Size() અચલ { પાછા m_ઢગલો_Size + m_callMax; }

ખાનગી:
    શૂન્ય reset() {
        m_ઢગલો_Size = 0;
        m_ignoreRemaining = ખોટું;
    }
    uint64_t startVisitBase(Ast_ગાંઠ* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        અચલ uint64_t saved_ગણતરી = m_ઢગલો_Size;
        m_ઢગલો_Size = 0;
        પાછા saved_ગણતરી;
    }
    શૂન્ય endVisitBase(uint64_t saved_ગણતરી, અચલ Ast_ગાંઠ* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::ડાબું << m_ઢગલો_Size << "  " << nodep);
        જો (!m_ignoreRemaining) m_ઢગલો_Size += saved_ગણતરી;
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_If* nodep) override {
        જો (m_ignoreRemaining) પાછા;
        અચલ VisitBase vb{આ, nodep};
        iterateAndNextConstNull(nodep->condp());
        અચલ uint64_t saved_ગણતરી = m_ઢગલો_Size;

        UINFO(8, "thensp:");
        reset();
        iterateAndNextConstNull(nodep->thensp());
        uint64_t if_ગણતરી = m_ઢગલો_Size;
        જો (nodep->branchPred().unlikely()) if_ગણતરી = 0;

        UINFO(8, "elsesp:");
        reset();
        iterateAndNextConstNull(nodep->elsesp());
        uint64_t else_ગણતરી = m_ઢગલો_Size;
        જો (nodep->branchPred().likely()) else_ગણતરી = 0;

        reset();
        જો (if_ગણતરી >= else_ગણતરી) {
            m_ઢગલો_Size = saved_ગણતરી + if_ગણતરી;
            જો (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } નહીંતર {
            m_ઢગલો_Size = saved_ગણતરી + else_ગણતરી;
            જો (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    શૂન્ય visit(AstCond* nodep) override {
        જો (m_ignoreRemaining) પાછા;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        અચલ VisitBase vb{આ, nodep};
        iterateAndNextConstNull(nodep->condp());
        અચલ uint64_t saved_ગણતરી = m_ઢગલો_Size;

        UINFO(8, "?");
        reset();
        iterateAndNextConstNull(nodep->thenp());
        અચલ uint64_t if_ગણતરી = m_ઢગલો_Size;

        UINFO(8, ":");
        reset();
        iterateAndNextConstNull(nodep->elsep());
        અચલ uint64_t else_ગણતરી = m_ઢગલો_Size;

        reset();
        જો (if_ગણતરી >= else_ગણતરી) {
            m_ઢગલો_Size = saved_ગણતરી + if_ગણતરી;
            જો (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } નહીંતર {
            m_ઢગલો_Size = saved_ગણતરી + else_ગણતરી;
            જો (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    શૂન્ય visit(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    શૂન્ય visit(Ast_ગાંઠ_CCall* nodep) override {
        જો (m_ignoreRemaining) પાછા;
        અચલ VisitBase vb{આ, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = સાચું;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    શૂન્ય visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        જો (!m_tracingCall && !nodep->entryPoint()) પાછા;
        m_tracingCall = ખોટું;
        જો (nodep->recursive()) પાછા;
        જો (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignoreRemaining);
            VL_RESTORER(m_ઢગલો_Size);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_inCFunc);
            m_tracingCall = ખોટું;
            m_ઢગલો_Size = 0;
            m_callMax = 0;
            m_inCFunc = સાચું;
            અચલ VisitBase vb{આ, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_ઢગલો_Size + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = ખોટું;
    }
    શૂન્ય visit(AstVar* nodep) override {
        જો (!m_inCFunc) પાછા;
        m_ઢગલો_Size += nodep->isRef() ? કદ(શૂન્ય*) : nodep->dtypep()->widthTotalBytes();
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ_પદાવલિ* nodep) override {}  // Short-circuit
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        જો (m_ignoreRemaining) પાછા;
        અચલ VisitBase vb{આ, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(ઢગલો_ગણતરી_Visitor);
};

uint64_t V3_ઢગલો_ગણતરી::ગણતરી(Ast_ગાંઠ* nodep) {
    અચલ ઢગલો_ગણતરી_Visitor visitor{nodep};
    પાછા visitor.ઢગલો_Size();
}
