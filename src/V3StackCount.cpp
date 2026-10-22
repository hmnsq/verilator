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

वर्ग चळत_मोजणी_Visitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    स्थिरांक VNUser2InUse m_inuser2;

    // MEMBERS
    uint64_t m_चळत_Size = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    तार्किक m_tracingCall = असत्य;  // Iterating into a CCall to a CFunc
    तार्किक m_ignoreRemaining = असत्य;  // Ignore remaining statements in the block
    तार्किक m_inCFunc = असत्य;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    वर्ग VisitBase final {
        // MEMBERS
        uint64_t m_saved_मोजणी;  // Count before visit applied
        Ast_गाठ* स्थिरांक m_nodep;  // Node this RAII helper is visiting
        चळत_मोजणी_Visitor* स्थिरांक m_visitor;  // Visitor whose start/endVisitBase are called

    सार्वजनिक:
        // CONSTRUCTORS
        VisitBase(चळत_मोजणी_Visitor* visitor, Ast_गाठ* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_मोजणी = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_मोजणी, m_nodep); }

    खाजगी:
        VL_UNCOPYABLE(VisitBase);
    };

सार्वजनिक:
    // CONSTRUCTORS
    explicit चळत_मोजणी_Visitor(Ast_गाठ* nodep) { iterateConstNull(nodep); }
    ~चळत_मोजणी_Visitor() override = पूर्वनिर्धारित;

    // METHODS
    uint64_t चळत_Size() स्थिरांक { परत m_चळत_Size + m_callMax; }

खाजगी:
    शून्य reset() {
        m_चळत_Size = 0;
        m_ignoreRemaining = असत्य;
    }
    uint64_t startVisitBase(Ast_गाठ* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        स्थिरांक uint64_t saved_मोजणी = m_चळत_Size;
        m_चळत_Size = 0;
        परत saved_मोजणी;
    }
    शून्य endVisitBase(uint64_t saved_मोजणी, स्थिरांक Ast_गाठ* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::डावा << m_चळत_Size << "  " << nodep);
        जर (!m_ignoreRemaining) m_चळत_Size += saved_मोजणी;
    }

    // VISITORS
    शून्य visit(Ast_गाठ_If* nodep) override {
        जर (m_ignoreRemaining) परत;
        स्थिरांक VisitBase vb{हे, nodep};
        iterateAndNextConstNull(nodep->condp());
        स्थिरांक uint64_t saved_मोजणी = m_चळत_Size;

        UINFO(8, "thensp:");
        reset();
        iterateAndNextConstNull(nodep->thensp());
        uint64_t if_मोजणी = m_चळत_Size;
        जर (nodep->branchPred().unlikely()) if_मोजणी = 0;

        UINFO(8, "elsesp:");
        reset();
        iterateAndNextConstNull(nodep->elsesp());
        uint64_t else_मोजणी = m_चळत_Size;
        जर (nodep->branchPred().likely()) else_मोजणी = 0;

        reset();
        जर (if_मोजणी >= else_मोजणी) {
            m_चळत_Size = saved_मोजणी + if_मोजणी;
            जर (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } अन्यथा {
            m_चळत_Size = saved_मोजणी + else_मोजणी;
            जर (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    शून्य visit(AstCond* nodep) override {
        जर (m_ignoreRemaining) परत;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        स्थिरांक VisitBase vb{हे, nodep};
        iterateAndNextConstNull(nodep->condp());
        स्थिरांक uint64_t saved_मोजणी = m_चळत_Size;

        UINFO(8, "?");
        reset();
        iterateAndNextConstNull(nodep->thenp());
        स्थिरांक uint64_t if_मोजणी = m_चळत_Size;

        UINFO(8, ":");
        reset();
        iterateAndNextConstNull(nodep->elsep());
        स्थिरांक uint64_t else_मोजणी = m_चळत_Size;

        reset();
        जर (if_मोजणी >= else_मोजणी) {
            m_चळत_Size = saved_मोजणी + if_मोजणी;
            जर (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } अन्यथा {
            m_चळत_Size = saved_मोजणी + else_मोजणी;
            जर (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    शून्य visit(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    शून्य visit(Ast_गाठ_CCall* nodep) override {
        जर (m_ignoreRemaining) परत;
        स्थिरांक VisitBase vb{हे, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = सत्य;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    शून्य visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        जर (!m_tracingCall && !nodep->entryPoint()) परत;
        m_tracingCall = असत्य;
        जर (nodep->recursive()) परत;
        जर (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignoreRemaining);
            VL_RESTORER(m_चळत_Size);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_inCFunc);
            m_tracingCall = असत्य;
            m_चळत_Size = 0;
            m_callMax = 0;
            m_inCFunc = सत्य;
            स्थिरांक VisitBase vb{हे, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_चळत_Size + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = असत्य;
    }
    शून्य visit(AstVar* nodep) override {
        जर (!m_inCFunc) परत;
        m_चळत_Size += nodep->isRef() ? आकार(शून्य*) : nodep->dtypep()->widthTotalBytes();
        iterateChildrenConst(nodep);
    }

    शून्य visit(Ast_गाठ_अभिव्यक्ती* nodep) override {}  // Short-circuit
    शून्य visit(Ast_गाठ* nodep) override {
        जर (m_ignoreRemaining) परत;
        स्थिरांक VisitBase vb{हे, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(चळत_मोजणी_Visitor);
};

uint64_t V3_चळत_मोजणी::मोजणी(Ast_गाठ* nodep) {
    स्थिरांक चळत_मोजणी_Visitor visitor{nodep};
    परत visitor.चळत_Size();
}
