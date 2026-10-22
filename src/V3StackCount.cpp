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
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;

    // MEMBERS
    uint64_t m_ರಾಶಿ_ಗಾತ್ರ = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    ತಾರ್ಕಿಕ m_tracingCall = ಸುಳ್ಳು;  // Iterating into a CCall to a CFunc
    ತಾರ್ಕಿಕ m_ignore_ಉಳಿದ = ಸುಳ್ಳು;  // Ignore remaining statements in the block
    ತಾರ್ಕಿಕ m_ಒಳ_CFunc = ಸುಳ್ಳು;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    ವರ್ಗ ಭೇಟಿ_ತಳ final {
        // MEMBERS
        uint64_t m_saved_ಎಣಿಕೆ;  // Count before visit applied
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ m_nodep;  // Node this RAII helper is visiting
        ರಾಶಿ_ಎಣಿಕೆ_Visitor* ಸ್ಥಿರಾಂಕ m_visitor;  // Visitor whose start/endVisitBase are called

    ಸಾರ್ವಜನಿಕ:
        // CONSTRUCTORS
        ಭೇಟಿ_ತಳ(ರಾಶಿ_ಎಣಿಕೆ_Visitor* visitor, Ast_ಗಂಟು* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_ಎಣಿಕೆ = m_visitor->ಆರಂಭ_ಭೇಟಿ_ತಳ(nodep);
        }
        ~ಭೇಟಿ_ತಳ() { m_visitor->ಕೊನೆ_ಭೇಟಿ_ತಳ(m_saved_ಎಣಿಕೆ, m_nodep); }

    ಖಾಸಗಿ:
        VL_UNCOPYABLE(ಭೇಟಿ_ತಳ);
    };

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ರಾಶಿ_ಎಣಿಕೆ_Visitor(Ast_ಗಂಟು* nodep) { iterateConstNull(nodep); }
    ~ರಾಶಿ_ಎಣಿಕೆ_Visitor() override = ಪೂರ್ವನಿಯೋಜಿತ;

    // METHODS
    uint64_t ರಾಶಿ_ಗಾತ್ರ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ರಾಶಿ_ಗಾತ್ರ + m_callMax; }

ಖಾಸಗಿ:
    ಶೂನ್ಯ reset() {
        m_ರಾಶಿ_ಗಾತ್ರ = 0;
        m_ignore_ಉಳಿದ = ಸುಳ್ಳು;
    }
    uint64_t ಆರಂಭ_ಭೇಟಿ_ತಳ(Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(!m_ignore_ಉಳಿದ, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ಸ್ಥಿರಾಂಕ uint64_t saved_ಎಣಿಕೆ = m_ರಾಶಿ_ಗಾತ್ರ;
        m_ರಾಶಿ_ಗಾತ್ರ = 0;
        ಹಿಂತಿರುಗಿ saved_ಎಣಿಕೆ;
    }
    ಶೂನ್ಯ ಕೊನೆ_ಭೇಟಿ_ತಳ(uint64_t saved_ಎಣಿಕೆ, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::ಎಡ << m_ರಾಶಿ_ಗಾತ್ರ << "  " << nodep);
        ಆದರೆ (!m_ignore_ಉಳಿದ) m_ರಾಶಿ_ಗಾತ್ರ += saved_ಎಣಿಕೆ;
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_If* nodep) override {
        ಆದರೆ (m_ignore_ಉಳಿದ) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ ಭೇಟಿ_ತಳ vb{ಇದು, nodep};
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->condp());
        ಸ್ಥಿರಾಂಕ uint64_t saved_ಎಣಿಕೆ = m_ರಾಶಿ_ಗಾತ್ರ;

        UINFO(8, "thensp:");
        reset();
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->thensp());
        uint64_t if_ಎಣಿಕೆ = m_ರಾಶಿ_ಗಾತ್ರ;
        ಆದರೆ (nodep->branchPred().unlikely()) if_ಎಣಿಕೆ = 0;

        UINFO(8, "elsesp:");
        reset();
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->elsesp());
        uint64_t else_ಎಣಿಕೆ = m_ರಾಶಿ_ಗಾತ್ರ;
        ಆದರೆ (nodep->branchPred().likely()) else_ಎಣಿಕೆ = 0;

        reset();
        ಆದರೆ (if_ಎಣಿಕೆ >= else_ಎಣಿಕೆ) {
            m_ರಾಶಿ_ಗಾತ್ರ = saved_ಎಣಿಕೆ + if_ಎಣಿಕೆ;
            ಆದರೆ (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_ರಾಶಿ_ಗಾತ್ರ = saved_ಎಣಿಕೆ + else_ಎಣಿಕೆ;
            ಆದರೆ (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCond* nodep) override {
        ಆದರೆ (m_ignore_ಉಳಿದ) ಹಿಂತಿರುಗಿ;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ಸ್ಥಿರಾಂಕ ಭೇಟಿ_ತಳ vb{ಇದು, nodep};
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->condp());
        ಸ್ಥಿರಾಂಕ uint64_t saved_ಎಣಿಕೆ = m_ರಾಶಿ_ಗಾತ್ರ;

        UINFO(8, "?");
        reset();
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->thenp());
        ಸ್ಥಿರಾಂಕ uint64_t if_ಎಣಿಕೆ = m_ರಾಶಿ_ಗಾತ್ರ;

        UINFO(8, ":");
        reset();
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->elsep());
        ಸ್ಥಿರಾಂಕ uint64_t else_ಎಣಿಕೆ = m_ರಾಶಿ_ಗಾತ್ರ;

        reset();
        ಆದರೆ (if_ಎಣಿಕೆ >= else_ಎಣಿಕೆ) {
            m_ರಾಶಿ_ಗಾತ್ರ = saved_ಎಣಿಕೆ + if_ಎಣಿಕೆ;
            ಆದರೆ (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_ರಾಶಿ_ಗಾತ್ರ = saved_ಎಣಿಕೆ + else_ಎಣಿಕೆ;
            ಆದರೆ (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_CCall* nodep) override {
        ಆದರೆ (m_ignore_ಉಳಿದ) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ ಭೇಟಿ_ತಳ vb{ಇದು, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = ನಿಜ;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        ಆದರೆ (!m_tracingCall && !nodep->ನಮೂದು_ಬಿಂದು()) ಹಿಂತಿರುಗಿ;
        m_tracingCall = ಸುಳ್ಳು;
        ಆದರೆ (nodep->recursive()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignore_ಉಳಿದ);
            VL_RESTORER(m_ರಾಶಿ_ಗಾತ್ರ);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_ಒಳ_CFunc);
            m_tracingCall = ಸುಳ್ಳು;
            m_ರಾಶಿ_ಗಾತ್ರ = 0;
            m_callMax = 0;
            m_ಒಳ_CFunc = ನಿಜ;
            ಸ್ಥಿರಾಂಕ ಭೇಟಿ_ತಳ vb{ಇದು, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_ರಾಶಿ_ಗಾತ್ರ + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = ಸುಳ್ಳು;
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        ಆದರೆ (!m_ಒಳ_CFunc) ಹಿಂತಿರುಗಿ;
        m_ರಾಶಿ_ಗಾತ್ರ += nodep->isRef() ? ಗಾತ್ರ(ಶೂನ್ಯ*) : nodep->dtypep()->ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು();
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {}  // Short-circuit
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        ಆದರೆ (m_ignore_ಉಳಿದ) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ ಭೇಟಿ_ತಳ vb{ಇದು, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(ರಾಶಿ_ಎಣಿಕೆ_Visitor);
};

uint64_t V3_ರಾಶಿ_ಎಣಿಕೆ::ಎಣಿಕೆ_2(Ast_ಗಂಟು* nodep) {
    ಸ್ಥಿರಾಂಕ ರಾಶಿ_ಎಣಿಕೆ_Visitor visitor{nodep};
    ಹಿಂತಿರುಗಿ visitor.ರಾಶಿ_ಗಾತ್ರ();
}
