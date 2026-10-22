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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3StackCount.h"

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the stack byte size for executing all logic within and below a
/// given AST node. This is very rough, only for warning when stack is too
/// small.

ವರ್ಗ ರಾಶಿ_ಎಣಿಕೆ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    ಧ್ರುವ VNUser2InUse m_inuser2;

    // MEMBERS
    uint64_t m_ರಾಶಿ_Size = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    ತಾರ್ಕಿಕ m_tracingCall = ಅಸತ್ಯ;  // Iterating into a CCall to a CFunc
    ತಾರ್ಕಿಕ m_ignoreRemaining = ಅಸತ್ಯ;  // Ignore remaining statements in the block
    ತಾರ್ಕಿಕ m_inCFunc = ಅಸತ್ಯ;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    ವರ್ಗ VisitBase final {
        // MEMBERS
        uint64_t m_saved_ಎಣಿಕೆ;  // Count before visit applied
        Ast_ಗಂಟು* ಧ್ರುವ m_nodep;  // Node this RAII helper is visiting
        ರಾಶಿ_ಎಣಿಕೆ_Visitor* ಧ್ರುವ m_visitor;  // Visitor whose start/endVisitBase are called

    ಸಾರ್ವಜನಿಕ:
        // CONSTRUCTORS
        VisitBase(ರಾಶಿ_ಎಣಿಕೆ_Visitor* visitor, Ast_ಗಂಟು* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_ಎಣಿಕೆ = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_ಎಣಿಕೆ, m_nodep); }

    ಗುಪ್ತ:
        VL_UNCOPYABLE(VisitBase);
    };

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit ರಾಶಿ_ಎಣಿಕೆ_Visitor(Ast_ಗಂಟು* nodep) { iterateConstNull(nodep); }
    ~ರಾಶಿ_ಎಣಿಕೆ_Visitor() override = ಸ್ವಭಾವ;

    // METHODS
    uint64_t ರಾಶಿ_Size() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ರಾಶಿ_Size + m_callMax; }

ಗುಪ್ತ:
    ಶೂನ್ಯ reset() {
        m_ರಾಶಿ_Size = 0;
        m_ignoreRemaining = ಅಸತ್ಯ;
    }
    uint64_t startVisitBase(Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ಧ್ರುವ uint64_t saved_ಎಣಿಕೆ = m_ರಾಶಿ_Size;
        m_ರಾಶಿ_Size = 0;
        ಪ್ರತ್ಯಾಗಮ saved_ಎಣಿಕೆ;
    }
    ಶೂನ್ಯ endVisitBase(uint64_t saved_ಎಣಿಕೆ, ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::ಎಡ << m_ರಾಶಿ_Size << "  " << nodep);
        ಯದಿ (!m_ignoreRemaining) m_ರಾಶಿ_Size += saved_ಎಣಿಕೆ;
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_If* nodep) override {
        ಯದಿ (m_ignoreRemaining) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ VisitBase vb{ಏತತ್, nodep};
        iterateAndNextConstNull(nodep->condp());
        ಧ್ರುವ uint64_t saved_ಎಣಿಕೆ = m_ರಾಶಿ_Size;

        UINFO(8, "thensp:");
        reset();
        iterateAndNextConstNull(nodep->thensp());
        uint64_t if_ಎಣಿಕೆ = m_ರಾಶಿ_Size;
        ಯದಿ (nodep->branchPred().unlikely()) if_ಎಣಿಕೆ = 0;

        UINFO(8, "elsesp:");
        reset();
        iterateAndNextConstNull(nodep->elsesp());
        uint64_t else_ಎಣಿಕೆ = m_ರಾಶಿ_Size;
        ಯದಿ (nodep->branchPred().likely()) else_ಎಣಿಕೆ = 0;

        reset();
        ಯದಿ (if_ಎಣಿಕೆ >= else_ಎಣಿಕೆ) {
            m_ರಾಶಿ_Size = saved_ಎಣಿಕೆ + if_ಎಣಿಕೆ;
            ಯದಿ (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } ಅನ್ಯಥಾ {
            m_ರಾಶಿ_Size = saved_ಎಣಿಕೆ + else_ಎಣಿಕೆ;
            ಯದಿ (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    ಶೂನ್ಯ visit(AstCond* nodep) override {
        ಯದಿ (m_ignoreRemaining) ಪ್ರತ್ಯಾಗಮ;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ಧ್ರುವ VisitBase vb{ಏತತ್, nodep};
        iterateAndNextConstNull(nodep->condp());
        ಧ್ರುವ uint64_t saved_ಎಣಿಕೆ = m_ರಾಶಿ_Size;

        UINFO(8, "?");
        reset();
        iterateAndNextConstNull(nodep->thenp());
        ಧ್ರುವ uint64_t if_ಎಣಿಕೆ = m_ರಾಶಿ_Size;

        UINFO(8, ":");
        reset();
        iterateAndNextConstNull(nodep->elsep());
        ಧ್ರುವ uint64_t else_ಎಣಿಕೆ = m_ರಾಶಿ_Size;

        reset();
        ಯದಿ (if_ಎಣಿಕೆ >= else_ಎಣಿಕೆ) {
            m_ರಾಶಿ_Size = saved_ಎಣಿಕೆ + if_ಎಣಿಕೆ;
            ಯದಿ (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } ಅನ್ಯಥಾ {
            m_ರಾಶಿ_Size = saved_ಎಣಿಕೆ + else_ಎಣಿಕೆ;
            ಯದಿ (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    ಶೂನ್ಯ visit(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_CCall* nodep) override {
        ಯದಿ (m_ignoreRemaining) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ VisitBase vb{ಏತತ್, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = ಸತ್ಯ;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        ಯದಿ (!m_tracingCall && !nodep->entryPoint()) ಪ್ರತ್ಯಾಗಮ;
        m_tracingCall = ಅಸತ್ಯ;
        ಯದಿ (nodep->recursive()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignoreRemaining);
            VL_RESTORER(m_ರಾಶಿ_Size);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_inCFunc);
            m_tracingCall = ಅಸತ್ಯ;
            m_ರಾಶಿ_Size = 0;
            m_callMax = 0;
            m_inCFunc = ಸತ್ಯ;
            ಧ್ರುವ VisitBase vb{ಏತತ್, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_ರಾಶಿ_Size + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = ಅಸತ್ಯ;
    }
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        ಯದಿ (!m_inCFunc) ಪ್ರತ್ಯಾಗಮ;
        m_ರಾಶಿ_Size += nodep->isRef() ? ಪರಿಮಾಣ(ಶೂನ್ಯ*) : nodep->dtypep()->widthTotalBytes();
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {}  // Short-circuit
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        ಯದಿ (m_ignoreRemaining) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ VisitBase vb{ಏತತ್, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(ರಾಶಿ_ಎಣಿಕೆ_Visitor);
};

uint64_t V3_ರಾಶಿ_ಎಣಿಕೆ::ಎಣಿಕೆ(Ast_ಗಂಟು* nodep) {
    ಧ್ರುವ ರಾಶಿ_ಎಣಿಕೆ_Visitor visitor{nodep};
    ಪ್ರತ್ಯಾಗಮ visitor.ರಾಶಿ_Size();
}
