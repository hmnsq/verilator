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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3StackCount.h"

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the stack byte size for executing all logic within and below a
/// given AST node. This is very rough, only for warning when stack is too
/// small.

صنف دۆۋە_سانى_Visitor final : عام VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    ثابت VNUser2InUse m_inuser2;

    // MEMBERS
    uint64_t m_دۆۋە_Size = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    منطقي m_tracingCall = خطأ;  // Iterating into a CCall to a CFunc
    منطقي m_ignoreRemaining = خطأ;  // Ignore remaining statements in the block
    منطقي m_inCFunc = خطأ;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    صنف VisitBase final {
        // MEMBERS
        uint64_t m_saved_سانى;  // Count before visit applied
        Ast_تۈگۈن* ثابت m_nodep;  // Node this RAII helper is visiting
        دۆۋە_سانى_Visitor* ثابت m_visitor;  // Visitor whose start/endVisitBase are called

    عام:
        // CONSTRUCTORS
        VisitBase(دۆۋە_سانى_Visitor* visitor, Ast_تۈگۈن* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_سانى = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_سانى, m_nodep); }

    خاص:
        VL_UNCOPYABLE(VisitBase);
    };

عام:
    // CONSTRUCTORS
    explicit دۆۋە_سانى_Visitor(Ast_تۈگۈن* nodep) { iterateConstNull(nodep); }
    ~دۆۋە_سانى_Visitor() override = افتراضي;

    // METHODS
    uint64_t دۆۋە_Size() ثابت { أرجع m_دۆۋە_Size + m_callMax; }

خاص:
    فراغ reset() {
        m_دۆۋە_Size = 0;
        m_ignoreRemaining = خطأ;
    }
    uint64_t startVisitBase(Ast_تۈگۈن* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ثابت uint64_t saved_سانى = m_دۆۋە_Size;
        m_دۆۋە_Size = 0;
        أرجع saved_سانى;
    }
    فراغ endVisitBase(uint64_t saved_سانى, ثابت Ast_تۈگۈن* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::سول << m_دۆۋە_Size << "  " << nodep);
        إذا (!m_ignoreRemaining) m_دۆۋە_Size += saved_سانى;
    }

    // VISITORS
    فراغ visit(Ast_تۈگۈن_If* nodep) override {
        إذا (m_ignoreRemaining) أرجع;
        ثابت VisitBase vb{هذا, nodep};
        iterateAndNextConstNull(nodep->condp());
        ثابت uint64_t saved_سانى = m_دۆۋە_Size;

        UINFO(8, "thensp:");
        reset();
        iterateAndNextConstNull(nodep->thensp());
        uint64_t if_سانى = m_دۆۋە_Size;
        إذا (nodep->branchPred().unlikely()) if_سانى = 0;

        UINFO(8, "elsesp:");
        reset();
        iterateAndNextConstNull(nodep->elsesp());
        uint64_t else_سانى = m_دۆۋە_Size;
        إذا (nodep->branchPred().likely()) else_سانى = 0;

        reset();
        إذا (if_سانى >= else_سانى) {
            m_دۆۋە_Size = saved_سانى + if_سانى;
            إذا (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } وإلا {
            m_دۆۋە_Size = saved_سانى + else_سانى;
            إذا (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    فراغ visit(AstCond* nodep) override {
        إذا (m_ignoreRemaining) أرجع;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ثابت VisitBase vb{هذا, nodep};
        iterateAndNextConstNull(nodep->condp());
        ثابت uint64_t saved_سانى = m_دۆۋە_Size;

        UINFO(8, "?");
        reset();
        iterateAndNextConstNull(nodep->thenp());
        ثابت uint64_t if_سانى = m_دۆۋە_Size;

        UINFO(8, ":");
        reset();
        iterateAndNextConstNull(nodep->elsep());
        ثابت uint64_t else_سانى = m_دۆۋە_Size;

        reset();
        إذا (if_سانى >= else_سانى) {
            m_دۆۋە_Size = saved_سانى + if_سانى;
            إذا (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } وإلا {
            m_دۆۋە_Size = saved_سانى + else_سانى;
            إذا (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    فراغ visit(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    فراغ visit(Ast_تۈگۈن_CCall* nodep) override {
        إذا (m_ignoreRemaining) أرجع;
        ثابت VisitBase vb{هذا, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = true;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    فراغ visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        إذا (!m_tracingCall && !nodep->entryPoint()) أرجع;
        m_tracingCall = خطأ;
        إذا (nodep->recursive()) أرجع;
        إذا (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignoreRemaining);
            VL_RESTORER(m_دۆۋە_Size);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_inCFunc);
            m_tracingCall = خطأ;
            m_دۆۋە_Size = 0;
            m_callMax = 0;
            m_inCFunc = true;
            ثابت VisitBase vb{هذا, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_دۆۋە_Size + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = خطأ;
    }
    فراغ visit(AstVar* nodep) override {
        إذا (!m_inCFunc) أرجع;
        m_دۆۋە_Size += nodep->isRef() ? حجم(فراغ*) : nodep->dtypep()->widthTotalBytes();
        iterateChildrenConst(nodep);
    }

    فراغ visit(Ast_تۈگۈن_ئىپادە* nodep) override {}  // Short-circuit
    فراغ visit(Ast_تۈگۈن* nodep) override {
        إذا (m_ignoreRemaining) أرجع;
        ثابت VisitBase vb{هذا, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(دۆۋە_سانى_Visitor);
};

uint64_t V3_دۆۋە_سانى::سانى(Ast_تۈگۈن* nodep) {
    ثابت دۆۋە_سانى_Visitor visitor{nodep};
    أرجع visitor.دۆۋە_Size();
}
