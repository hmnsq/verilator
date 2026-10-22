// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: CFG liveness analysis
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
//
// Classical data flow analysis computing live variables input to a CFG
// https://en.wikipedia.org/wiki/Live-variable_analysis
//
//*************************************************************************

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3Cfg.h"

#অন্তর্ভাৱ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

শ্ৰেণী CfgLiveVariables final : VNVisitorConst {
    // State associted with each basic block
    সংরচনা BlockState final {
        // Variables used in block, before a complete assignment in the same block
        std::unordered_set<AstVarScope*> m_gen;
        // Variables that are assigned a complete value in the basic block
        std::unordered_set<AstVarScope*> m_kill;
        std::unordered_set<AstVarScope*> m_liveIn;  // Variables live on entry to the block
        std::unordered_set<AstVarScope*> m_liveOut;  // Variables live on exit from the block
        তার্কিক m_isOnWorkList = মিছা;  // Block is on work list
        তার্কিক m_wasProcessed = মিছা;  // Already processed at least once
    };

    // STATE
    ধ্রুৱ CfgGraph& m_cfg;  // The CFG beign analysed
    // State for each block
    CfgBlockMap<BlockState> m_blockState{m_cfg.makeBlockMap<BlockState>()};
    BlockState* m_currp = nullptr;  // State of current block being analysed
    তার্কিক m_abort = মিছা;  // Abort analysis - unhandled construct

    // METHODS

    স্থির তার্কিক isPacked(ধ্রুৱ AstVarScope* vscp) {
        Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = vscp->dtypep()->skipRefp();
        যদি (ধ্রুৱ AstBasicDType* ধ্রুৱ typep = VN_CAST(dtypep, BasicDType)) {
            ঘূৰাই typep->keyword().isIntNumeric();
        }
        যদি (VN_IS(dtypep, PackArrayDType)) ঘূৰাই সঁচা;
        যদি (ধ্রুৱ Ast_গ্ৰন্থি_UOr_Struct_DType* ধ্রুৱ typep = VN_CAST(dtypep, গ্ৰন্থি_UOr_Struct_DType)) {
            ঘূৰাই typep->packed();
        }
        ঘূৰাই মিছা;
    }

    শূন্য updateGen(Ast_গ্ৰন্থি* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ধ্রুৱ AstVarRef* refp) {
            // Only care about reads
            যদি (refp->access().isWriteOnly()) ঘূৰাই মিছা;
            // Grab referenced variable
            AstVarScope* ধ্রুৱ vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            যদি (m_currp->m_kill.গুনতি(vscp)) ঘূৰাই মিছা;
            m_currp->m_gen.emplace(vscp);
            ঘূৰাই মিছা;
        });
    }

    শূন্য updateKill(Ast_গ্ৰন্থি* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ধ্রুৱ AstVarRef* refp) {
            // Only care about writes
            যদি (refp->access().isReadOnly()) ঘূৰাই মিছা;
            // Grab referenced variable
            AstVarScope* ধ্রুৱ vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            যদি (vscp->varp()->ignoreSchedWrite()) ঘূৰাই সঁচা;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            যদি (!isPacked(vscp)) ঘূৰাই মিছা;
            // If packed, and updated partially, don't add to kill set
            যদি (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) ঘূৰাই মিছা;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            ঘূৰাই মিছা;
        });
    }

    শূন্য single(Ast_গ্ৰন্থি* nodep) {
        // Assume all reads hapen before any writes
        যদি (m_abort) ঘূৰাই;
        updateGen(nodep);
        যদি (m_abort) ঘূৰাই;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    তার্কিক transfer(ধ্রুৱ CfgBlock& bb) {
        BlockState& state = m_blockState[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_set<AstVarScope*> liveIn = state.m_gen;
        বাবে (AstVarScope* ধ্রুৱ varp : state.m_liveOut) {
            যদি (state.m_kill.গুনতি(varp)) অনুৱর্ত;
            liveIn.ঢোকাও(varp);
        }
        যদি (liveIn == state.m_liveIn) ঘূৰাই মিছা;
        std::swap(liveIn, state.m_liveIn);
        ঘূৰাই সঁচা;
    }

    // VISIT
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = সঁচা;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    শূন্য visit(AstAssign* nodep) override { single(nodep); }
    শূন্য visit(AstAssignW* nodep) override { single(nodep); }
    শূন্য visit(AstDisplay* nodep) override { single(nodep); }
    শূন্য visit(AstFinish* nodep) override { single(nodep); }
    শূন্য visit(AstFinishFork* nodep) override { single(nodep); }
    শূন্য visit(Ast_Stmt_ৰাশিমালা* nodep) override { single(nodep); }
    শূন্য visit(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    শূন্য visit(AstIf* nodep) override { single(nodep->condp()); }
    শূন্য visit(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    explicit CfgLiveVariables(ধ্রুৱ CfgGraph& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        বাবে (ধ্রুৱ V3GraphVertex& vtx : cfg.vertices()) {
            ধ্রুৱ CfgBlock& bb = static_cast<ধ্রুৱ CfgBlock&>(vtx);
            যদি (m_abort) ঘূৰাই;
            VL_RESTORER(m_currp);
            m_currp = &m_blockState[bb];
            বাবে (Ast_গ্ৰন্থি* ধ্রুৱ stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        যদি (m_abort) ঘূৰাই;

        // Perform the flow analysis
        std::deque<ধ্রুৱ CfgBlock*> workList;
        ধ্রুৱ auto enqueue = [&](ধ্রুৱ CfgBlock& bb) {
            BlockState& state = m_blockState[bb];
            যদি (state.m_isOnWorkList) ঘূৰাই;
            state.m_isOnWorkList = সঁচা;
            workList.emplace_back(&bb);
        };

        enqueue(cfg.exit());

        যেতিয়ালৈ (!workList.empty()) {
            ধ্রুৱ CfgBlock* ধ্রুৱ currp = workList.front();
            workList.বেৰ_কৰো_front();
            BlockState& state = m_blockState[*currp];
            state.m_isOnWorkList = মিছা;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](ধ্রুৱ CfgBlock& bb) {
                auto& liveIn = m_blockState[bb].m_liveIn;
                state.m_liveOut.ঢোকাও(liveIn.begin(), liveIn.end());
            });

            // Apply transfer function of block
            ধ্রুৱ তার্কিক changed = transfer(*currp);
            // Enqueue predecessors
            যদি (changed || !state.m_wasProcessed) currp->forEachPredecessor(enqueue);
            // Mark as done with first visit
            state.m_wasProcessed = সঁচা;
        }
    }

সার্ৱজনিক:
    স্থির std::unique_ptr<std::vector<AstVarScope*>> apply(ধ্রুৱ CfgGraph& cfg) {
        CfgLiveVariables analysis{cfg};
        // If failed, return nullptr
        যদি (analysis.m_abort) ঘূৰাই nullptr;
        // Gather variables live in to the entry block
        ধ্রুৱ std::unordered_set<AstVarScope*>& lin = analysis.m_blockState[cfg.enter()].m_liveIn;
        std::vector<AstVarScope*>* ধ্রুৱ resultp
            = নূতন std::vector<AstVarScope*>{lin.begin(), lin.end()};
        // Sort for stability
        std::stable_sort(resultp->begin(), resultp->end(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             ঘূৰাই ap->name() < bp->name();
                         });
        ঘূৰাই std::unique_ptr<std::vector<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::vector<AstVarScope*>> V3Cfg::liveVarScopes(ধ্রুৱ CfgGraph& cfg) {
    ঘূৰাই CfgLiveVariables::apply(cfg);
}
