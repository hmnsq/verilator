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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3StackCount.h"

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the stack byte size for executing all logic within and below a
/// given AST node. This is very rough, only for warning when stack is too
/// small.

తరగతి దొంతర_లెక్క_Visitor final : బహిరంగ VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> uint64_t.  Path cost + 1,
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;

    // MEMBERS
    uint64_t m_దొంతర_కొలత = 0;  // Running count of local bytes in the current function
    uint64_t m_callMax = 0;  // Largest callee cost called from the current function
    తార్కిక m_tracingCall = అబద్ధం;  // Iterating into a CCall to a CFunc
    తార్కిక m_ignore_మిగిలిన = అబద్ధం;  // Ignore remaining statements in the block
    తార్కిక m_లోపల_CFunc = అబద్ధం;  // Inside function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    తరగతి సందర్శించు_పునాది final {
        // MEMBERS
        uint64_t m_saved_లెక్క;  // Count before visit applied
        Ast_కణుపు* స్థిరరాశి m_nodep;  // Node this RAII helper is visiting
        దొంతర_లెక్క_Visitor* స్థిరరాశి m_visitor;  // Visitor whose start/endVisitBase are called

    బహిరంగ:
        // CONSTRUCTORS
        సందర్శించు_పునాది(దొంతర_లెక్క_Visitor* visitor, Ast_కణుపు* nodep)
            : m_nodep{nodep}
            , m_visitor{visitor} {
            m_saved_లెక్క = m_visitor->ప్రారంభం_సందర్శించు_పునాది(nodep);
        }
        ~సందర్శించు_పునాది() { m_visitor->చివర_సందర్శించు_పునాది(m_saved_లెక్క, m_nodep); }

    అంతరంగ:
        VL_UNCOPYABLE(సందర్శించు_పునాది);
    };

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన దొంతర_లెక్క_Visitor(Ast_కణుపు* nodep) { iterateConstNull(nodep); }
    ~దొంతర_లెక్క_Visitor() override = అప్రమేయ;

    // METHODS
    uint64_t దొంతర_కొలత() స్థిరరాశి { తిరిగి m_దొంతర_కొలత + m_callMax; }

అంతరంగ:
    శూన్య reset() {
        m_దొంతర_కొలత = 0;
        m_ignore_మిగిలిన = అబద్ధం;
    }
    uint64_t ప్రారంభం_సందర్శించు_పునాది(Ast_కణుపు* nodep) {
        UASSERT_OBJ(!m_ignore_మిగిలిన, nodep, "Should not reach here if ignoring");

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        స్థిరరాశి uint64_t saved_లెక్క = m_దొంతర_కొలత;
        m_దొంతర_కొలత = 0;
        తిరిగి saved_లెక్క;
    }
    శూన్య చివర_సందర్శించు_పునాది(uint64_t saved_లెక్క, స్థిరరాశి Ast_కణుపు* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::ఎడమ << m_దొంతర_కొలత << "  " << nodep);
        ఒకవేళ (!m_ignore_మిగిలిన) m_దొంతర_కొలత += saved_లెక్క;
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_If* nodep) override {
        ఒకవేళ (m_ignore_మిగిలిన) తిరిగి;
        స్థిరరాశి సందర్శించు_పునాది vb{ఇది, nodep};
        iterate_And_తదుపరి_Const_Null(nodep->condp());
        స్థిరరాశి uint64_t saved_లెక్క = m_దొంతర_కొలత;

        UINFO(8, "thensp:");
        reset();
        iterate_And_తదుపరి_Const_Null(nodep->thensp());
        uint64_t if_లెక్క = m_దొంతర_కొలత;
        ఒకవేళ (nodep->branchPred().unlikely()) if_లెక్క = 0;

        UINFO(8, "elsesp:");
        reset();
        iterate_And_తదుపరి_Const_Null(nodep->elsesp());
        uint64_t else_లెక్క = m_దొంతర_కొలత;
        ఒకవేళ (nodep->branchPred().likely()) else_లెక్క = 0;

        reset();
        ఒకవేళ (if_లెక్క >= else_లెక్క) {
            m_దొంతర_కొలత = saved_లెక్క + if_లెక్క;
            ఒకవేళ (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } లేకపోతే {
            m_దొంతర_కొలత = saved_లెక్క + else_లెక్క;
            ఒకవేళ (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    శూన్య సందర్శించు(AstCond* nodep) override {
        ఒకవేళ (m_ignore_మిగిలిన) తిరిగి;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        స్థిరరాశి సందర్శించు_పునాది vb{ఇది, nodep};
        iterate_And_తదుపరి_Const_Null(nodep->condp());
        స్థిరరాశి uint64_t saved_లెక్క = m_దొంతర_కొలత;

        UINFO(8, "?");
        reset();
        iterate_And_తదుపరి_Const_Null(nodep->thenp());
        స్థిరరాశి uint64_t if_లెక్క = m_దొంతర_కొలత;

        UINFO(8, ":");
        reset();
        iterate_And_తదుపరి_Const_Null(nodep->elsep());
        స్థిరరాశి uint64_t else_లెక్క = m_దొంతర_కొలత;

        reset();
        ఒకవేళ (if_లెక్క >= else_లెక్క) {
            m_దొంతర_కొలత = saved_లెక్క + if_లెక్క;
            ఒకవేళ (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } లేకపోతే {
            m_దొంతర_కొలత = saved_లెక్క + else_లెక్క;
            ఒకవేళ (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    శూన్య సందర్శించు(AstFork* nodep) override { nodep->v3fatalSrc("Fork removed earlier"); }
    శూన్య సందర్శించు(Ast_కణుపు_CCall* nodep) override {
        ఒకవేళ (m_ignore_మిగిలిన) తిరిగి;
        స్థిరరాశి సందర్శించు_పునాది vb{ఇది, nodep};
        iterateChildrenConst(nodep);
        m_tracingCall = నిజం;
        iterateConst(nodep->funcp());
        UASSERT_OBJ(!m_tracingCall, nodep, "visit(AstCFunc) should have cleared m_tracingCall.");
    }
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        ఒకవేళ (!m_tracingCall && !nodep->నమోదు_బిందువు()) తిరిగి;
        m_tracingCall = అబద్ధం;
        ఒకవేళ (nodep->recursive()) తిరిగి;
        ఒకవేళ (!nodep->user2()) {  // Short circuit
            VL_RESTORER(m_ignore_మిగిలిన);
            VL_RESTORER(m_దొంతర_కొలత);
            VL_RESTORER(m_callMax);
            VL_RESTORER(m_లోపల_CFunc);
            m_tracingCall = అబద్ధం;
            m_దొంతర_కొలత = 0;
            m_callMax = 0;
            m_లోపల_CFunc = నిజం;
            స్థిరరాశి సందర్శించు_పునాది vb{ఇది, nodep};
            iterateChildrenConst(nodep);
            nodep->user2(m_దొంతర_కొలత + m_callMax + 1);
        }
        // Sequential calls reuse the same stack, so only the deepest counts
        m_callMax = std::max(m_callMax, nodep->user2() - 1);
        m_tracingCall = అబద్ధం;
    }
    శూన్య సందర్శించు(AstVar* nodep) override {
        ఒకవేళ (!m_లోపల_CFunc) తిరిగి;
        m_దొంతర_కొలత += nodep->isRef() ? పరిమాణం(శూన్య*) : nodep->dtypep()->వెడల్పు_మొత్తం_బైట్లు();
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు_సమాసం* nodep) override {}  // Short-circuit
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        ఒకవేళ (m_ignore_మిగిలిన) తిరిగి;
        స్థిరరాశి సందర్శించు_పునాది vb{ఇది, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(దొంతర_లెక్క_Visitor);
};

uint64_t V3_దొంతర_లెక్క::లెక్క(Ast_కణుపు* nodep) {
    స్థిరరాశి దొంతర_లెక్క_Visitor visitor{nodep};
    తిరిగి visitor.దొంతర_కొలత();
}
