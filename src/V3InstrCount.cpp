// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Estimate instruction count to run the logic
//                         we would generate for any given AST subtree.
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

#అన్తర్భావ "V3InstrCount.h"

#అన్తర్భావ <iomanip>
#అన్తర్భావ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

/// Estimate the instruction cost for executing all logic within and below
/// a given AST node. Note this estimates the number of instructions we'll
/// execute, not the number we'll generate. That is, for conditionals,
/// we'll count instructions from either the 'if' or the 'else' branch,
/// whichever is larger. We know we won't run both.

తరగతి Instr_లెక్క_Visitor final : బహిరంగ VNVisitorConst {
    // NODE STATE
    //  AstNode::user1()        -> bool. Processed if assertNoDups
    //  AstNode::user2()        -> uint64_t.  Path cost + 1, 0 means don't dump
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;

    // MEMBERS
    uint32_t m_instr_లెక్క = 0;  // Running count of instructions
    స్థిరరాశి Ast_కణుపు* స్థిరరాశి m_ప్రారంభం_Nodep;  // Start node of count
    తార్కిక m_tracingCall = అబద్ధం;  // Iterating into a CCall to a CFunc
    తార్కిక m_లోపల_CFunc = అబద్ధం;  // Inside AstCFunc
    తార్కిక m_ignore_మిగిలిన = అబద్ధం;  // Ignore remaining statements in the block
    స్థిరరాశి తార్కిక m_assertNoDups;  // Check for duplicates
    స్థిరరాశి std::ostream* స్థిరరాశి m_osp;  // Dump file
    std::unordered_map<స్థిరరాశి AstCFunc*, uint32_t> m_funcCosts;  // Cost of each function

    // TYPES
    // Little class to cleanly call startVisitBase/endVisitBase
    తరగతి సందర్శించు_పునాది final {
        // MEMBERS
        uint32_t m_saved_లెక్క;  // Count before visit applied
        Ast_కణుపు* స్థిరరాశి m_nodep;  // Node helper is visiting
        Instr_లెక్క_Visitor* స్థిరరాశి m_visitor;  // Visitor whose start/endVisitBase are called

    బహిరంగ:
        // CONSTRUCTORS
        సందర్శించు_పునాది(Instr_లెక్క_Visitor* visitor, Ast_కణుపు* nodep)
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
    Instr_లెక్క_Visitor(Ast_కణుపు* nodep, తార్కిక assertNoDups, std::ostream* osp)
        : m_ప్రారంభం_Nodep{nodep}
        , m_assertNoDups{assertNoDups}
        , m_osp{osp} {
        iterateConstNull(nodep);
    }
    ~Instr_లెక్క_Visitor() override = అప్రమేయ;

    // METHODS
    uint32_t instr_లెక్క() స్థిరరాశి { తిరిగి m_instr_లెక్క; }

అంతరంగ:
    శూన్య reset() {
        m_instr_లెక్క = 0;
        m_ignore_మిగిలిన = అబద్ధం;
    }
    uint32_t ప్రారంభం_సందర్శించు_పునాది(Ast_కణుపు* nodep) {
        UASSERT_OBJ(!m_ignore_మిగిలిన, nodep, "Should not reach here if ignoring");
        ఒకవేళ (m_assertNoDups && !m_లోపల_CFunc) {
            // Ensure we don't count the same node twice
            //
            // We only enable this assert for the initial LogicMTask counts
            // in V3Order. We can't enable it for the 2nd pass in V3EmitC,
            // as we expect mtasks to contain common logic after V3Combine,
            // so this would fail.
            //
            // Also, we expect some collisions within calls to CFuncs
            // (which at the V3Order stage represent verilog tasks, not to
            // the CFuncs that V3Order will generate.) So don't check for
            // collisions in CFuncs.
            UASSERT_OBJ(!nodep->user1p(), nodep,
                        "Node originally inserted below logic vertex "
                            << static_cast<Ast_కణుపు*>(nodep->user1p()));
            nodep->user1p(const_cast<శూన్య*>(reinterpret_cast<స్థిరరాశి శూన్య*>(m_ప్రారంభం_Nodep)));
        }

        // Save the count, and add it back in during ~VisitBase This allows
        // debug prints to show local cost of each subtree, so we can see a
        // hierarchical view of the cost when in debug mode.
        స్థిరరాశి uint32_t saved_లెక్క = m_instr_లెక్క;
        m_instr_లెక్క = nodep->instr_లెక్క();
        తిరిగి saved_లెక్క;
    }
    శూన్య చివర_సందర్శించు_పునాది(uint32_t saved_లెక్క, Ast_కణుపు* nodep) {
        UINFO(8, "cost " << std::setw(6) << std::ఎడమ << m_instr_లెక్క << "  " << nodep);
        mark_ఖర్చు(nodep);
        ఒకవేళ (!m_ignore_మిగిలిన) m_instr_లెక్క += saved_లెక్క;
    }
    శూన్య mark_ఖర్చు(Ast_కణుపు* nodep) {
        ఒకవేళ (m_osp) nodep->user2(m_instr_లెక్క + 1);  // Else don't mark to avoid writeback
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Sel* nodep) override {
        ఒకవేళ (m_ignore_మిగిలిన) తిరిగి;
        // This covers both AstArraySel and AstWordSel
        //
        // If some vector is a bazillion dwords long, and we're selecting 1
        // dword to read or write from it, our cost should be small.
        //
        // Hence, exclude the child of the AstWordSel from the computation,
        // whose cost scales with the size of the entire (maybe large) vector.
        స్థిరరాశి సందర్శించు_పునాది vb{ఇది, nodep};
        iterate_And_తదుపరి_Const_Null(nodep->bitp());
    }
    శూన్య సందర్శించు(AstSel* nodep) override {
        ఒకవేళ (m_ignore_మిగిలిన) తిరిగి;
        // Similar to AstNodeSel above, a small select into a large vector
        // is not expensive. Count the cost of the AstSel itself (scales with
        // its width) and the cost of the lsbp() and widthp() nodes, but not
        // the fromp() node which could be disproportionately large.
        స్థిరరాశి సందర్శించు_పునాది vb{ఇది, nodep};
        iterate_And_తదుపరి_Const_Null(nodep->lsbp());
    }
    శూన్య సందర్శించు(AstConcat* nodep) override {
        ఒకవేళ (m_ignore_మిగిలిన) తిరిగి;
        // Ignore the cost of the concat node itself. The problem with
        // counting concat is that when we have many things concatted
        // together, it's not a single operation, but this:
        //
        //  concat(a, concat(b, concat(c, concat(d, ... ))))
        //
        // Then if we account a cost to each 'concat' that scales with its
        // width, this whole operation ends up with a cost accounting that
        // scales with N^2. Of course, the real operation isn't that
        // expensive: we won't copy each element over and over, we'll just
        // copy it once from its origin into its destination, so the actual
        // cost is linear with the size of the data. We don't need to count
        // the concat at all to reflect a linear cost; it's already there
        // in the width of the destination (which we count) and the sum of
        // the cost of the operands.
        iterateChildrenConst(nodep);
        mark_ఖర్చు(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_If* nodep) override {
        ఒకవేళ (m_ignore_మిగిలిన) తిరిగి;
        స్థిరరాశి సందర్శించు_పునాది vb{ఇది, nodep};
        iterate_And_తదుపరి_Const_Null(nodep->condp());
        స్థిరరాశి uint32_t saved_లెక్క = m_instr_లెక్క;

        UINFO(8, "thensp:");
        reset();
        iterate_And_తదుపరి_Const_Null(nodep->thensp());
        uint32_t if_లెక్క = m_instr_లెక్క;
        ఒకవేళ (nodep->branchPred().unlikely()) if_లెక్క = 0;

        UINFO(8, "elsesp:");
        reset();
        iterate_And_తదుపరి_Const_Null(nodep->elsesp());
        uint32_t else_లెక్క = m_instr_లెక్క;
        ఒకవేళ (nodep->branchPred().likely()) else_లెక్క = 0;

        reset();
        ఒకవేళ (if_లెక్క >= else_లెక్క) {
            m_instr_లెక్క = saved_లెక్క + if_లెక్క;
            ఒకవేళ (nodep->elsesp()) nodep->elsesp()->user2(0);  // Don't dump it
        } లేకపోతే {
            m_instr_లెక్క = saved_లెక్క + else_లెక్క;
            ఒకవేళ (nodep->thensp()) nodep->thensp()->user2(0);  // Don't dump it
        }
    }
    శూన్య సందర్శించు(AstCond* nodep) override {
        ఒకవేళ (m_ignore_మిగిలిన) తిరిగి;
        // Just like if/else above, the ternary operator only evaluates
        // one of the two expressions, so only count the max.
        స్థిరరాశి సందర్శించు_పునాది vb{ఇది, nodep};
        iterate_And_తదుపరి_Const_Null(nodep->condp());
        స్థిరరాశి uint32_t saved_లెక్క = m_instr_లెక్క;

        UINFO(8, "?");
        reset();
        iterate_And_తదుపరి_Const_Null(nodep->thenp());
        స్థిరరాశి uint32_t if_లెక్క = m_instr_లెక్క;

        UINFO(8, ":");
        reset();
        iterate_And_తదుపరి_Const_Null(nodep->elsep());
        స్థిరరాశి uint32_t else_లెక్క = m_instr_లెక్క;

        reset();
        ఒకవేళ (if_లెక్క >= else_లెక్క) {
            m_instr_లెక్క = saved_లెక్క + if_లెక్క;
            ఒకవేళ (nodep->elsep()) nodep->elsep()->user2(0);  // Don't dump it
        } లేకపోతే {
            m_instr_లెక్క = saved_లెక్క + else_లెక్క;
            ఒకవేళ (nodep->thenp()) nodep->thenp()->user2(0);  // Don't dump it
        }
    }
    శూన్య సందర్శించు(AstCAwait* nodep) override {
        ఒకవేళ (m_ignore_మిగిలిన) తిరిగి;
        iterateChildrenConst(nodep);
        // Anything past a co_await is irrelevant
        m_ignore_మిగిలిన = నిజం;
    }
    శూన్య సందర్శించు(AstFork* nodep) override {
        ఒకవేళ (m_ignore_మిగిలిన) తిరిగి;
        స్థిరరాశి సందర్శించు_పునాది vb{ఇది, nodep};
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
        uint32_t మొత్తం_లెక్క = m_instr_లెక్క;
        // Sum counts in each statement until the first await
        కొరకు (Ast_కణుపు* stmtp = nodep->forksp(); stmtp; stmtp = stmtp->nextp()) {
            reset();
            iterateConst(stmtp);
            మొత్తం_లెక్క += m_instr_లెక్క;
        }
        m_instr_లెక్క = మొత్తం_లెక్క;
        m_ignore_మిగిలిన = అబద్ధం;
    }
    శూన్య సందర్శించు(Ast_చురుకైన* nodep) override {
        // You'd think that the OrderLogicVertex's would be disjoint trees
        // of stuff in the AST, but it isn't so: V3Order makes an
        // OrderLogicVertex for each ACTIVE, and then also makes an
        // OrderLogicVertex for each statement within the ACTIVE.
        //
        // To avoid double-counting costs, stop recursing and short-circuit
        // the computation for each ACTIVE.
        //
        // Our intent is that this only stops at the root node of the
        // search; there should be no actives beneath the root, as there
        // are no actives-under-actives.  In any case, check that we're at
        // root:
        mark_ఖర్చు(nodep);
        UASSERT_OBJ(nodep == m_ప్రారంభం_Nodep, nodep, "Multiple actives, or not start node");
    }
    శూన్య సందర్శించు(Ast_కణుపు_CCall* nodep) override {
        ఒకవేళ (m_ignore_మిగిలిన) తిరిగి;
        స్థిరరాశి సందర్శించు_పునాది vb{ఇది, nodep};
        iterateChildrenConst(nodep);
        స్థిరరాశి స్వయంచాలక it = m_funcCosts.కనుగొను(nodep->funcp());
        ఒకవేళ (it == m_funcCosts.చివర()) {
            m_tracingCall = నిజం;
            iterateConst(nodep->funcp());
            UASSERT_OBJ(!m_tracingCall, nodep,
                        "visit(AstCFunc) should have cleared m_tracingCall.");
        } లేకపోతే {
            m_instr_లెక్క += it->రెండవ;  // This guards and adds zero cost on recursive calls
        }
    }
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        // Don't count a CFunc other than by tracing a call or counting it
        // from the root
        UASSERT_OBJ(m_tracingCall || nodep == m_ప్రారంభం_Nodep, nodep,
                    "AstCFunc not under AstCCall, or not start node");
        UASSERT_OBJ(!m_ignore_మిగిలిన, nodep, "Should not be ignoring at the start of a CFunc");
        స్థిరరాశి స్వయంచాలక emplacePair = m_funcCosts.emplace(nodep, 0);
        UASSERT_OBJ(emplacePair.రెండవ, nodep, "Should visit every CFunc at most once");
        uint32_t& funcCostr = emplacePair.మొదటి->రెండవ;
        m_tracingCall = అబద్ధం;

        స్థిరరాశి uint32_t saved_లెక్క = m_instr_లెక్క;
        reset();
        {
            VL_RESTORER(m_లోపల_CFunc);
            m_లోపల_CFunc = నిజం;
            స్థిరరాశి సందర్శించు_పునాది vb{ఇది, nodep};
            iterateChildrenConst(nodep);
        }
        funcCostr = m_instr_లెక్క;
        m_instr_లెక్క += saved_లెక్క;

        m_ignore_మిగిలిన = అబద్ధం;
    }
    శూన్య సందర్శించు(Ast_కణుపు_Cons* nodep) override {
        ఒకవేళ (m_ignore_మిగిలిన) తిరిగి;
        స్థిరరాశి సందర్శించు_పునాది vb{ఇది, nodep};
        // A constant value is not built at run time, so its parts cost nothing
        ఒకవేళ (!nodep->is_Constant_వృక్షం()) iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        ఒకవేళ (m_ignore_మిగిలిన) తిరిగి;
        స్థిరరాశి సందర్శించు_పునాది vb{ఇది, nodep};
        iterateChildrenConst(nodep);
    }

    VL_UNCOPYABLE(Instr_లెక్క_Visitor);
};

// Iterate the graph printing the critical path marked by previous visitation
తరగతి Instr_లెక్క_Dump_Visitor final : బహిరంగ VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> int.  Path cost, 0 means don't dump

    // MEMBERS
    std::ostream* స్థిరరాశి m_osp;  // Dump file
    గుర్తులేని m_లోతు = 0;  // Current tree depth for printing indent

బహిరంగ:
    // CONSTRUCTORS
    Instr_లెక్క_Dump_Visitor(Ast_కణుపు* nodep, std::ostream* osp)
        : m_osp{osp} {
        // No check for nullptr output, so...
        UASSERT_OBJ(osp, nodep, "Don't call if not dumping");
        ఒకవేళ (nodep) iterateConst(nodep);
    }
    ~Instr_లెక్క_Dump_Visitor() override = అప్రమేయ;

అంతరంగ:
    // METHODS
    string indent() స్థిరరాశి { తిరిగి string(m_లోతు, ':') + " "; }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        ++m_లోతు;
        ఒకవేళ (స్థిరరాశి గుర్తులేని ఖర్చు_Plus1 = nodep->user2()) {
            *m_osp << "  " << indent() << "cost " << std::setw(6) << std::ఎడమ << (ఖర్చు_Plus1 - 1)
                   << "  " << nodep << '\n';
            iterateChildrenConst(nodep);
        }
        --m_లోతు;
    }

    VL_UNCOPYABLE(Instr_లెక్క_Dump_Visitor);
};

uint32_t V3_Instr_లెక్క::లెక్క(Ast_కణుపు* nodep, తార్కిక assertNoDups, std::ostream* osp) {
    స్థిరరాశి Instr_లెక్క_Visitor visitor{nodep, assertNoDups, osp};
    ఒకవేళ (osp) స్థిరరాశి Instr_లెక్క_Dump_Visitor dumper{nodep, osp};
    తిరిగి visitor.instr_లెక్క();
}
