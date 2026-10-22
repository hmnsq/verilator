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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3StackCount.h"

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the stack byte size for executing all logic within and below a
/// given AST node. This is very rough, only for warning when stack is too
/// small.

வகுப்பு அடுக்கு_மொத்தம்_Visitor final : பொது VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    மாறிலி VNUser2InUse m_inuser2;

    // MEMBERS
    uint64_t m_அடுக்கு_Size = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    தருக்க m_tracingCall = பொய்;  // Iterating into a CCall to a CFunc
    தருக்க m_ignoreRemaining = பொய்;  // Ignore remaining statements in the block
    தருக்க m_inCFunc = பொய்;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    வகுப்பு VisitBase final {
        // MEMBERS
        uint64_t m_saved_மொத்தம்;  // Count before visit applied
        Ast_முனை* மாறிலி m_nodep;  // Node this RAII helper is visiting
        அடுக்கு_மொத்தம்_Visitor* மாறிலி m_visitor;  // Visitor whose start/endVisitBase are called

    பொது:
        // CONSTRUCTORS
        VisitBase(அடுக்கு_மொத்தம்_Visitor* visitor, Ast_முனை* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_மொத்தம் = m_visitor->startVisitBase(nodep);
        }
        ~VisitBase() { m_visitor->endVisitBase(m_saved_மொத்தம், m_nodep); }

    தனிப்பட்ட:
        VL_UNCOPYABLE(VisitBase);
    };

பொது:
    // CONSTRUCTORS
    explicit அடுக்கு_மொத்தம்_Visitor(Ast_முனை* nodep) { iterateConstNull(nodep); }
    ~அடுக்கு_மொத்தம்_Visitor() override = இயல்பு;

    // METHODS
    uint64_t அடுக்கு_Size() மாறிலி { திரும்பு m_அடுக்கு_Size + m_callMax; }

தனிப்பட்ட:
    வெறுமை reset() {
        m_அடுக்கு_Size = 0;
        m_ignoreRemaining = பொய்;
    }
    uint64_t startVisitBase(Ast_முனை* nodep) {
        UASSERT_OBJ(!m_ignoreRemaining, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        மாறிலி uint64_t saved_மொத்தம் = m_அடுக்கு_Size;
        m_அடுக்கு_Size = 0;
        திரும்பு saved_மொத்தம்;
    }
    வெறுமை endVisitBase(uint64_t saved_மொத்தம், மாறிலி Ast_முனை* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::இடது << m_அடுக்கு_Size << "  " << nodep);
        எனில் (!m_ignoreRemaining) m_அடுக்கு_Size += saved_மொத்தம்;
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_If* nodep) override {
        எனில் (m_ignoreRemaining) திரும்பு;
        மாறிலி VisitBase vb{இது, nodep};
        iterateAndNextConstNull(nodep->condp());
        மாறிலி uint64_t saved_மொத்தம் = m_அடுக்கு_Size;

        UINFO(8, "thensp:");
        reset();
        iterateAndNextConstNull(nodep->thensp());
        uint64_t if_மொத்தம் = m_அடுக்கு_Size;
        எனில் (nodep->branchPred().unlikely()) if_மொத்தம் = 0;

        UINFO(8, "elsesp:");
        reset();
        iterateAndNextConstNull(nodep->elsesp());
        uint64_t else_மொத்தம் = m_அடுக்கு_Size;
        எனில் (nodep->branchPred().likely()) else_மொத்தம் = 0;

        reset();
        எனில் (if_மொத்தம் >= else_மொத்தம்) {
            m_அடுக்கு_Size = saved_மொத்தம் + if_மொத்தம்;
            எனில் (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } இல்லையெனில் {
            m_அடுக்கு_Size = saved_மொத்தம் + else_மொத்தம்;
            எனில் (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    வெறுமை visit(AstCond* nodep) override {
        எனில் (m_ignoreRemaining) திரும்பு;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        மாறிலி VisitBase vb{இது, nodep};
        iterateAndNextConstNull(nodep->condp());
        மாறிலி uint64_t saved_மொத்தம் = m_அடுக்கு_Size;

        UINFO(8, "?");
        reset();
        iterateAndNextConstNull(nodep->thenp());
        மாறிலி uint64_t if_மொத்தம் = m_அடுக்கு_Size;

        UINFO(8, ":");
        reset();
        iterateAndNextConstNull(nodep->elsep());
        மாறிலி uint64_t else_மொத்தம் = m_அடுக்கு_Size;

        reset();
        எனில் (if_மொத்தம் >= else_மொத்தம்) {
            m_அடுக்கு_Size = saved_மொத்தம் + if_மொத்தம்;
            எனில் (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } இல்லையெனில் {
            m_அடுக்கு_Size = saved_மொத்தம் + else_மொத்தம்;
            எனில் (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    வெறுமை visit(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    வெறுமை visit(Ast_முனை_CCall* nodep) override {
        எனில் (m_ignoreRemaining) திரும்பு;
        மாறிலி VisitBase vb{இது, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = உண்மை;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    வெறுமை visit(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        எனில் (!m_tracingCall && !nodep->entryPoint()) திரும்பு;
        m_tracingCall = பொய்;
        எனில் (nodep->recursive()) திரும்பு;
        எனில் (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignoreRemaining);
            VL_RESTORER(m_அடுக்கு_Size);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_inCFunc);
            m_tracingCall = பொய்;
            m_அடுக்கு_Size = 0;
            m_callMax = 0;
            m_inCFunc = உண்மை;
            மாறிலி VisitBase vb{இது, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_அடுக்கு_Size + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = பொய்;
    }
    வெறுமை visit(AstVar* nodep) override {
        எனில் (!m_inCFunc) திரும்பு;
        m_அடுக்கு_Size += nodep->isRef() ? அளவு(வெறுமை*) : nodep->dtypep()->widthTotalBytes();
        iterateChildrenConst(nodep);
    }

    வெறுமை visit(Ast_முனை_கோவை* nodep) override {}  // Short-circuit
    வெறுமை visit(Ast_முனை* nodep) override {
        எனில் (m_ignoreRemaining) திரும்பு;
        மாறிலி VisitBase vb{இது, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(அடுக்கு_மொத்தம்_Visitor);
};

uint64_t V3_அடுக்கு_மொத்தம்::மொத்தம்(Ast_முனை* nodep) {
    மாறிலி அடுக்கு_மொத்தம்_Visitor visitor{nodep};
    திரும்பு visitor.அடுக்கு_Size();
}
