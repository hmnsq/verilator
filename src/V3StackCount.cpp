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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3StackCount.h"

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the stack byte size for executing all logic within and below a
/// given AST node. This is very rough, only for warning when stack is too
/// small.

ଶ୍ରେଣୀ ଗଦା_ଗଣନା_Visitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    ଅଚଳ VNUser2InUse m_inuser2;

    // MEMBERS
    uint64_t m_ଗଦା_Size = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    ତାର୍କିକ m_tracingCall = ମିଥ୍ୟା;  // Iterating into a CCall to a CFunc
    ତାର୍କିକ m_ignoreRemaining = ମିଥ୍ୟା;  // Ignore remaining statements in the block
    ତାର୍କିକ m_inCFunc = ମିଥ୍ୟା;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    ଶ୍ରେଣୀ VisitBase final {
        // MEMBERS
        uint64_t m_saved_ଗଣନା;  // Count before visit applied
        Ast_ଗଣ୍ଠି* ଅଚଳ m_nodep;  // Node this RAII helper is visiting
        ଗଦା_ଗଣନା_Visitor* ଅଚଳ m_visitor;  // Visitor whose start/endVisitBase are called

    ସର୍ବସାଧାରଣ:
        // CONSTRUCTORS
        VisitBase(ଗଦା_ଗଣନା_Visitor* visitor, Ast_ଗଣ୍ଠି* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_ଗଣନା = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_ଗଣନା, m_nodep); }

    ବ୍ୟକ୍ତିଗତ:
        VL_UNCOPYABLE(VisitBase);
    };

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit ଗଦା_ଗଣନା_Visitor(Ast_ଗଣ୍ଠି* nodep) { iterateConstNull(nodep); }
    ~ଗଦା_ଗଣନା_Visitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // METHODS
    uint64_t ଗଦା_Size() ଅଚଳ { ଫେରନ୍ତୁ m_ଗଦା_Size + m_callMax; }

ବ୍ୟକ୍ତିଗତ:
    ଶୂନ୍ୟ reset() {
        m_ଗଦା_Size = 0;
        m_ignoreRemaining = ମିଥ୍ୟା;
    }
    uint64_t startVisitBase(Ast_ଗଣ୍ଠି* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        ଅଚଳ uint64_t saved_ଗଣନା = m_ଗଦା_Size;
        m_ଗଦା_Size = 0;
        ଫେରନ୍ତୁ saved_ଗଣନା;
    }
    ଶୂନ୍ୟ endVisitBase(uint64_t saved_ଗଣନା, ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::ବାମ << m_ଗଦା_Size << "  " << nodep);
        ଯଦି (!m_ignoreRemaining) m_ଗଦା_Size += saved_ଗଣନା;
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_If* nodep) override {
        ଯଦି (m_ignoreRemaining) ଫେରନ୍ତୁ;
        ଅଚଳ VisitBase vb{ଏହି, nodep};
        iterateAndNextConstNull(nodep->condp());
        ଅଚଳ uint64_t saved_ଗଣନା = m_ଗଦା_Size;

        UINFO(8, "thensp:");
        reset();
        iterateAndNextConstNull(nodep->thensp());
        uint64_t if_ଗଣନା = m_ଗଦା_Size;
        ଯଦି (nodep->branchPred().unlikely()) if_ଗଣନା = 0;

        UINFO(8, "elsesp:");
        reset();
        iterateAndNextConstNull(nodep->elsesp());
        uint64_t else_ଗଣନା = m_ଗଦା_Size;
        ଯଦି (nodep->branchPred().likely()) else_ଗଣନା = 0;

        reset();
        ଯଦି (if_ଗଣନା >= else_ଗଣନା) {
            m_ଗଦା_Size = saved_ଗଣନା + if_ଗଣନା;
            ଯଦି (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } ଅନ୍ୟଥା {
            m_ଗଦା_Size = saved_ଗଣନା + else_ଗଣନା;
            ଯଦି (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    ଶୂନ୍ୟ visit(AstCond* nodep) override {
        ଯଦି (m_ignoreRemaining) ଫେରନ୍ତୁ;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        ଅଚଳ VisitBase vb{ଏହି, nodep};
        iterateAndNextConstNull(nodep->condp());
        ଅଚଳ uint64_t saved_ଗଣନା = m_ଗଦା_Size;

        UINFO(8, "?");
        reset();
        iterateAndNextConstNull(nodep->thenp());
        ଅଚଳ uint64_t if_ଗଣନା = m_ଗଦା_Size;

        UINFO(8, ":");
        reset();
        iterateAndNextConstNull(nodep->elsep());
        ଅଚଳ uint64_t else_ଗଣନା = m_ଗଦା_Size;

        reset();
        ଯଦି (if_ଗଣନା >= else_ଗଣନା) {
            m_ଗଦା_Size = saved_ଗଣନା + if_ଗଣନା;
            ଯଦି (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } ଅନ୍ୟଥା {
            m_ଗଦା_Size = saved_ଗଣନା + else_ଗଣନା;
            ଯଦି (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    ଶୂନ୍ୟ visit(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_CCall* nodep) override {
        ଯଦି (m_ignoreRemaining) ଫେରନ୍ତୁ;
        ଅଚଳ VisitBase vb{ଏହି, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = ସତ୍ୟ;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        ଯଦି (!m_tracingCall && !nodep->entryPoint()) ଫେରନ୍ତୁ;
        m_tracingCall = ମିଥ୍ୟା;
        ଯଦି (nodep->recursive()) ଫେରନ୍ତୁ;
        ଯଦି (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignoreRemaining);
            VL_RESTORER(m_ଗଦା_Size);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_inCFunc);
            m_tracingCall = ମିଥ୍ୟା;
            m_ଗଦା_Size = 0;
            m_callMax = 0;
            m_inCFunc = ସତ୍ୟ;
            ଅଚଳ VisitBase vb{ଏହି, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_ଗଦା_Size + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = ମିଥ୍ୟା;
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        ଯଦି (!m_inCFunc) ଫେରନ୍ତୁ;
        m_ଗଦା_Size += nodep->isRef() ? ଆକାର(ଶୂନ୍ୟ*) : nodep->dtypep()->widthTotalBytes();
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) override {}  // Short-circuit
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        ଯଦି (m_ignoreRemaining) ଫେରନ୍ତୁ;
        ଅଚଳ VisitBase vb{ଏହି, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(ଗଦା_ଗଣନା_Visitor);
};

uint64_t V3_ଗଦା_ଗଣନା::ଗଣନା_2(Ast_ଗଣ୍ଠି* nodep) {
    ଅଚଳ ଗଦା_ଗଣନା_Visitor visitor{nodep};
    ଫେରନ୍ତୁ visitor.ଗଦା_Size();
}
