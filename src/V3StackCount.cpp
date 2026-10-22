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

صنف StackCountVisitor final : عام VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    ثابت VNUser2InUse m_inuser2;

    // MEMBERS
    uint64_t m_stackSize = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    منطقي m_tracingCall = خطأ;  // Iterating into a CCall to a CFunc
    منطقي m_ignoreRemaining = خطأ;  // Ignore remaining statements in the block
    منطقي m_inCFunc = خطأ;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    صنف VisitBase final {
        // MEMBERS
        uint64_t m_savedCount;  // Count before visit applied
        AstNode* ثابت m_nodep;  // Node this RAII helper is visiting
        StackCountVisitor* ثابت m_visitor;  // Visitor whose start/endVisitBase are called

    عام:
        // CONSTRUCTORS
        VisitBase(StackCountVisitor* visitor, AstNode* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_savedCount = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_savedCount, m_nodep); }

    خاص:
        VL_UNCOPYABLE(VisitBase);
    };

عام:
    // CONSTRUCTORS
    explicit StackCountVisitor(AstNode* nodep) { iterateConstNull(nodep); }
    ~StackCountVisitor() override = افتراضي;

    // METHODS
    uint64_t stackSize() ثابت { أرجع m_stackSize + m_callMax; }

خاص:
    فراغ reset() {
        m_stackSize = 0;
        m_ignoreRemaining = خطأ;
    }
    uint64_t startVisitBase(AstNode* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ثابت uint64_t savedCount = m_stackSize;
        m_stackSize = 0;
        أرجع savedCount;
    }
    فراغ endVisitBase(uint64_t savedCount, ثابت AstNode* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::left << m_stackSize << "  " << nodep);
        𐡄𐡍 (!m_ignoreRemaining) m_stackSize += savedCount;
    }

    // VISITORS
    فراغ visit(AstNodeIf* nodep) override {
        𐡄𐡍 (m_ignoreRemaining) أرجع;
        ثابت VisitBase vb{هذا, nodep};
        iterateAndNextConstNull(nodep->condp());
        ثابت uint64_t savedCount = m_stackSize;

        UINFO(8, "thensp:");
        reset();
        iterateAndNextConstNull(nodep->thensp());
        uint64_t ifCount = m_stackSize;
        𐡄𐡍 (nodep->branchPred().unlikely()) ifCount = 0;

        UINFO(8, "elsesp:");
        reset();
        iterateAndNextConstNull(nodep->elsesp());
        uint64_t elseCount = m_stackSize;
        𐡄𐡍 (nodep->branchPred().likely()) elseCount = 0;

        reset();
        𐡄𐡍 (ifCount >= elseCount) {
            m_stackSize = savedCount + ifCount;
            𐡄𐡍 (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } وإلا {
            m_stackSize = savedCount + elseCount;
            𐡄𐡍 (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    فراغ visit(AstCond* nodep) override {
        𐡄𐡍 (m_ignoreRemaining) أرجع;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ثابت VisitBase vb{هذا, nodep};
        iterateAndNextConstNull(nodep->condp());
        ثابت uint64_t savedCount = m_stackSize;

        UINFO(8, "?");
        reset();
        iterateAndNextConstNull(nodep->thenp());
        ثابت uint64_t ifCount = m_stackSize;

        UINFO(8, ":");
        reset();
        iterateAndNextConstNull(nodep->elsep());
        ثابت uint64_t elseCount = m_stackSize;

        reset();
        𐡄𐡍 (ifCount >= elseCount) {
            m_stackSize = savedCount + ifCount;
            𐡄𐡍 (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } وإلا {
            m_stackSize = savedCount + elseCount;
            𐡄𐡍 (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    فراغ visit(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    فراغ visit(AstNodeCCall* nodep) override {
        𐡄𐡍 (m_ignoreRemaining) أرجع;
        ثابت VisitBase vb{هذا, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = 𐡒𐡔𐡈;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    فراغ visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        𐡄𐡍 (!m_tracingCall && !nodep->entryPoint()) أرجع;
        m_tracingCall = خطأ;
        𐡄𐡍 (nodep->recursive()) أرجع;
        𐡄𐡍 (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignoreRemaining);
            VL_RESTORER(m_stackSize);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_inCFunc);
            m_tracingCall = خطأ;
            m_stackSize = 0;
            m_callMax = 0;
            m_inCFunc = 𐡒𐡔𐡈;
            ثابت VisitBase vb{هذا, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_stackSize + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = خطأ;
    }
    فراغ visit(AstVar* nodep) override {
        𐡄𐡍 (!m_inCFunc) أرجع;
        m_stackSize += nodep->isRef() ? حجم(فراغ*) : nodep->dtypep()->widthTotalBytes();
        iterateChildrenConst(nodep);
    }

    فراغ visit(AstNodeExpr* nodep) override {}  // Short-circuit
    فراغ visit(AstNode* nodep) override {
        𐡄𐡍 (m_ignoreRemaining) أرجع;
        ثابت VisitBase vb{هذا, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(StackCountVisitor);
};

uint64_t V3StackCount::count(AstNode* nodep) {
    ثابت StackCountVisitor visitor{nodep};
    أرجع visitor.stackSize();
}
