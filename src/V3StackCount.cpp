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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3StackCount.h"

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the stack byte size for executing all logic within and below a
/// given AST node. This is very rough, only for warning when stack is too
/// small.

ਵਰਗ ਢੇਰ_ਗਿਣਤੀ_Visitor final : ਜਨਤਕ VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;

    // MEMBERS
    uint64_t m_ਢੇਰ_ਮਾਪ = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    ਤਾਰਕਿਕ m_tracingCall = ਝੂਠ;  // Iterating into a CCall to a CFunc
    ਤਾਰਕਿਕ m_ignore_ਬਚਿਆ = ਝੂਠ;  // Ignore remaining statements in the block
    ਤਾਰਕਿਕ m_ਵਿੱਚ_CFunc = ਝੂਠ;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    ਵਰਗ ਦੇਖੋ_ਅਧਾਰ final {
        // MEMBERS
        uint64_t m_saved_ਗਿਣਤੀ;  // Count before visit applied
        Ast_ਗੰਢ* ਅਚਲ m_nodep;  // Node this RAII helper is visiting
        ਢੇਰ_ਗਿਣਤੀ_Visitor* ਅਚਲ m_visitor;  // Visitor whose start/endVisitBase are called

    ਜਨਤਕ:
        // CONSTRUCTORS
        ਦੇਖੋ_ਅਧਾਰ(ਢੇਰ_ਗਿਣਤੀ_Visitor* visitor, Ast_ਗੰਢ* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_ਗਿਣਤੀ = m_visitor->ਸ਼ੁਰੂ_ਦੇਖੋ_ਅਧਾਰ(nodep);
        }
        ~ਦੇਖੋ_ਅਧਾਰ() { m_visitor->ਸਿਰਾ_ਦੇਖੋ_ਅਧਾਰ(m_saved_ਗਿਣਤੀ, m_nodep); }

    ਨਿਜੀ:
        VL_UNCOPYABLE(ਦੇਖੋ_ਅਧਾਰ);
    };

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ ਢੇਰ_ਗਿਣਤੀ_Visitor(Ast_ਗੰਢ* nodep) { iterateConstNull(nodep); }
    ~ਢੇਰ_ਗਿਣਤੀ_Visitor() override = ਮੂਲ;

    // METHODS
    uint64_t ਢੇਰ_ਮਾਪ() ਅਚਲ { ਵਾਪਸ m_ਢੇਰ_ਮਾਪ + m_callMax; }

ਨਿਜੀ:
    ਖਾਲੀ reset() {
        m_ਢੇਰ_ਮਾਪ = 0;
        m_ignore_ਬਚਿਆ = ਝੂਠ;
    }
    uint64_t ਸ਼ੁਰੂ_ਦੇਖੋ_ਅਧਾਰ(Ast_ਗੰਢ* nodep) {
        UASSERT_OBJ(!m_ignore_ਬਚਿਆ, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ਅਚਲ uint64_t saved_ਗਿਣਤੀ = m_ਢੇਰ_ਮਾਪ;
        m_ਢੇਰ_ਮਾਪ = 0;
        ਵਾਪਸ saved_ਗਿਣਤੀ;
    }
    ਖਾਲੀ ਸਿਰਾ_ਦੇਖੋ_ਅਧਾਰ(uint64_t saved_ਗਿਣਤੀ, ਅਚਲ Ast_ਗੰਢ* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::ਖੱਬੇ << m_ਢੇਰ_ਮਾਪ << "  " << nodep);
        ਜੇ (!m_ignore_ਬਚਿਆ) m_ਢੇਰ_ਮਾਪ += saved_ਗਿਣਤੀ;
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_If* nodep) override {
        ਜੇ (m_ignore_ਬਚਿਆ) ਵਾਪਸ;
        ਅਚਲ ਦੇਖੋ_ਅਧਾਰ vb{ਇਹ, nodep};
        iterate_And_ਅਗਲਾ_Const_Null(nodep->condp());
        ਅਚਲ uint64_t saved_ਗਿਣਤੀ = m_ਢੇਰ_ਮਾਪ;

        UINFO(8, "thensp:");
        reset();
        iterate_And_ਅਗਲਾ_Const_Null(nodep->thensp());
        uint64_t if_ਗਿਣਤੀ = m_ਢੇਰ_ਮਾਪ;
        ਜੇ (nodep->branchPred().unlikely()) if_ਗਿਣਤੀ = 0;

        UINFO(8, "elsesp:");
        reset();
        iterate_And_ਅਗਲਾ_Const_Null(nodep->elsesp());
        uint64_t else_ਗਿਣਤੀ = m_ਢੇਰ_ਮਾਪ;
        ਜੇ (nodep->branchPred().likely()) else_ਗਿਣਤੀ = 0;

        reset();
        ਜੇ (if_ਗਿਣਤੀ >= else_ਗਿਣਤੀ) {
            m_ਢੇਰ_ਮਾਪ = saved_ਗਿਣਤੀ + if_ਗਿਣਤੀ;
            ਜੇ (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } ਨਹੀਂਤਾਂ {
            m_ਢੇਰ_ਮਾਪ = saved_ਗਿਣਤੀ + else_ਗਿਣਤੀ;
            ਜੇ (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCond* nodep) override {
        ਜੇ (m_ignore_ਬਚਿਆ) ਵਾਪਸ;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ਅਚਲ ਦੇਖੋ_ਅਧਾਰ vb{ਇਹ, nodep};
        iterate_And_ਅਗਲਾ_Const_Null(nodep->condp());
        ਅਚਲ uint64_t saved_ਗਿਣਤੀ = m_ਢੇਰ_ਮਾਪ;

        UINFO(8, "?");
        reset();
        iterate_And_ਅਗਲਾ_Const_Null(nodep->thenp());
        ਅਚਲ uint64_t if_ਗਿਣਤੀ = m_ਢੇਰ_ਮਾਪ;

        UINFO(8, ":");
        reset();
        iterate_And_ਅਗਲਾ_Const_Null(nodep->elsep());
        ਅਚਲ uint64_t else_ਗਿਣਤੀ = m_ਢੇਰ_ਮਾਪ;

        reset();
        ਜੇ (if_ਗਿਣਤੀ >= else_ਗਿਣਤੀ) {
            m_ਢੇਰ_ਮਾਪ = saved_ਗਿਣਤੀ + if_ਗਿਣਤੀ;
            ਜੇ (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } ਨਹੀਂਤਾਂ {
            m_ਢੇਰ_ਮਾਪ = saved_ਗਿਣਤੀ + else_ਗਿਣਤੀ;
            ਜੇ (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_CCall* nodep) override {
        ਜੇ (m_ignore_ਬਚਿਆ) ਵਾਪਸ;
        ਅਚਲ ਦੇਖੋ_ਅਧਾਰ vb{ਇਹ, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = ਸੱਚ;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        ਜੇ (!m_tracingCall && !nodep->ਇੰਦਰਾਜ_ਬਿੰਦੂ()) ਵਾਪਸ;
        m_tracingCall = ਝੂਠ;
        ਜੇ (nodep->recursive()) ਵਾਪਸ;
        ਜੇ (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignore_ਬਚਿਆ);
            VL_RESTORER(m_ਢੇਰ_ਮਾਪ);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_ਵਿੱਚ_CFunc);
            m_tracingCall = ਝੂਠ;
            m_ਢੇਰ_ਮਾਪ = 0;
            m_callMax = 0;
            m_ਵਿੱਚ_CFunc = ਸੱਚ;
            ਅਚਲ ਦੇਖੋ_ਅਧਾਰ vb{ਇਹ, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_ਢੇਰ_ਮਾਪ + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = ਝੂਠ;
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        ਜੇ (!m_ਵਿੱਚ_CFunc) ਵਾਪਸ;
        m_ਢੇਰ_ਮਾਪ += nodep->isRef() ? ਆਕਾਰ(ਖਾਲੀ*) : nodep->dtypep()->ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ();
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) override {}  // Short-circuit
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        ਜੇ (m_ignore_ਬਚਿਆ) ਵਾਪਸ;
        ਅਚਲ ਦੇਖੋ_ਅਧਾਰ vb{ਇਹ, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(ਢੇਰ_ਗਿਣਤੀ_Visitor);
};

uint64_t V3_ਢੇਰ_ਗਿਣਤੀ::ਗਿਣਤੀ_2(Ast_ਗੰਢ* nodep) {
    ਅਚਲ ਢੇਰ_ਗਿਣਤੀ_Visitor visitor{nodep};
    ਵਾਪਸ visitor.ਢੇਰ_ਮਾਪ();
}
