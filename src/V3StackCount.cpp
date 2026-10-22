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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3StackCount.h"

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the stack byte size for executing all logic within and below a
/// given AST node. This is very rough, only for warning when stack is too
/// small.

വർഗ്ഗം അട്ടി_എണ്ണം_Visitor final : പൊതു VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    സ്ഥിരം VNUser2InUse m_inuser2;

    // MEMBERS
    uint64_t m_അട്ടി_Size = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    തർക്കം m_tracingCall = അസത്യം;  // Iterating into a CCall to a CFunc
    തർക്കം m_ignoreRemaining = അസത്യം;  // Ignore remaining statements in the block
    തർക്കം m_inCFunc = അസത്യം;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    വർഗ്ഗം VisitBase final {
        // MEMBERS
        uint64_t m_saved_എണ്ണം;  // Count before visit applied
        Ast_കണ്ണി* സ്ഥിരം m_nodep;  // Node this RAII helper is visiting
        അട്ടി_എണ്ണം_Visitor* സ്ഥിരം m_visitor;  // Visitor whose start/endVisitBase are called

    പൊതു:
        // CONSTRUCTORS
        VisitBase(അട്ടി_എണ്ണം_Visitor* visitor, Ast_കണ്ണി* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_എണ്ണം = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_എണ്ണം, m_nodep); }

    സ്വകാര്യ:
        VL_UNCOPYABLE(VisitBase);
    };

പൊതു:
    // CONSTRUCTORS
    explicit അട്ടി_എണ്ണം_Visitor(Ast_കണ്ണി* nodep) { iterateConstNull(nodep); }
    ~അട്ടി_എണ്ണം_Visitor() override = സ്ഥിരസ്ഥിതി;

    // METHODS
    uint64_t അട്ടി_Size() സ്ഥിരം { തിരികെ m_അട്ടി_Size + m_callMax; }

സ്വകാര്യ:
    ശൂന്യം reset() {
        m_അട്ടി_Size = 0;
        m_ignoreRemaining = അസത്യം;
    }
    uint64_t startVisitBase(Ast_കണ്ണി* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        സ്ഥിരം uint64_t saved_എണ്ണം = m_അട്ടി_Size;
        m_അട്ടി_Size = 0;
        തിരികെ saved_എണ്ണം;
    }
    ശൂന്യം endVisitBase(uint64_t saved_എണ്ണം, സ്ഥിരം Ast_കണ്ണി* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::ഇടത് << m_അട്ടി_Size << "  " << nodep);
        എങ്കിൽ (!m_ignoreRemaining) m_അട്ടി_Size += saved_എണ്ണം;
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_If* nodep) override {
        എങ്കിൽ (m_ignoreRemaining) തിരികെ;
        സ്ഥിരം VisitBase vb{ഇത്, nodep};
        iterateAndNextConstNull(nodep->condp());
        സ്ഥിരം uint64_t saved_എണ്ണം = m_അട്ടി_Size;

        UINFO(8, "thensp:");
        reset();
        iterateAndNextConstNull(nodep->thensp());
        uint64_t if_എണ്ണം = m_അട്ടി_Size;
        എങ്കിൽ (nodep->branchPred().unlikely()) if_എണ്ണം = 0;

        UINFO(8, "elsesp:");
        reset();
        iterateAndNextConstNull(nodep->elsesp());
        uint64_t else_എണ്ണം = m_അട്ടി_Size;
        എങ്കിൽ (nodep->branchPred().likely()) else_എണ്ണം = 0;

        reset();
        എങ്കിൽ (if_എണ്ണം >= else_എണ്ണം) {
            m_അട്ടി_Size = saved_എണ്ണം + if_എണ്ണം;
            എങ്കിൽ (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } അല്ലെങ്കിൽ {
            m_അട്ടി_Size = saved_എണ്ണം + else_എണ്ണം;
            എങ്കിൽ (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    ശൂന്യം visit(AstCond* nodep) override {
        എങ്കിൽ (m_ignoreRemaining) തിരികെ;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        സ്ഥിരം VisitBase vb{ഇത്, nodep};
        iterateAndNextConstNull(nodep->condp());
        സ്ഥിരം uint64_t saved_എണ്ണം = m_അട്ടി_Size;

        UINFO(8, "?");
        reset();
        iterateAndNextConstNull(nodep->thenp());
        സ്ഥിരം uint64_t if_എണ്ണം = m_അട്ടി_Size;

        UINFO(8, ":");
        reset();
        iterateAndNextConstNull(nodep->elsep());
        സ്ഥിരം uint64_t else_എണ്ണം = m_അട്ടി_Size;

        reset();
        എങ്കിൽ (if_എണ്ണം >= else_എണ്ണം) {
            m_അട്ടി_Size = saved_എണ്ണം + if_എണ്ണം;
            എങ്കിൽ (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } അല്ലെങ്കിൽ {
            m_അട്ടി_Size = saved_എണ്ണം + else_എണ്ണം;
            എങ്കിൽ (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    ശൂന്യം visit(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    ശൂന്യം visit(Ast_കണ്ണി_CCall* nodep) override {
        എങ്കിൽ (m_ignoreRemaining) തിരികെ;
        സ്ഥിരം VisitBase vb{ഇത്, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = സത്യം;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    ശൂന്യം visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        എങ്കിൽ (!m_tracingCall && !nodep->entryPoint()) തിരികെ;
        m_tracingCall = അസത്യം;
        എങ്കിൽ (nodep->recursive()) തിരികെ;
        എങ്കിൽ (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignoreRemaining);
            VL_RESTORER(m_അട്ടി_Size);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_inCFunc);
            m_tracingCall = അസത്യം;
            m_അട്ടി_Size = 0;
            m_callMax = 0;
            m_inCFunc = സത്യം;
            സ്ഥിരം VisitBase vb{ഇത്, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_അട്ടി_Size + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = അസത്യം;
    }
    ശൂന്യം visit(AstVar* nodep) override {
        എങ്കിൽ (!m_inCFunc) തിരികെ;
        m_അട്ടി_Size += nodep->isRef() ? വലുപ്പം(ശൂന്യം*) : nodep->dtypep()->widthTotalBytes();
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) override {}  // Short-circuit
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        എങ്കിൽ (m_ignoreRemaining) തിരികെ;
        സ്ഥിരം VisitBase vb{ഇത്, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(അട്ടി_എണ്ണം_Visitor);
};

uint64_t V3_അട്ടി_എണ്ണം::എണ്ണം(Ast_കണ്ണി* nodep) {
    സ്ഥിരം അട്ടി_എണ്ണം_Visitor visitor{nodep};
    തിരികെ visitor.അട്ടി_Size();
}
