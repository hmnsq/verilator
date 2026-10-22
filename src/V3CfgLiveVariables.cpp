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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Cfg.h"

#অন্তর্ভাব <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

শ্রেণী CfgLiveVariables final : VNVisitorConst {
    // State associted with each basic block
    কাঠামো BlockState final {
        // Variables used in block, before a complete assignment in the same block
        std::unordered_set<AstVarScope*> m_gen;
        // Variables that are assigned a complete value in the basic block
        std::unordered_set<AstVarScope*> m_kill;
        std::unordered_set<AstVarScope*> m_liveIn;  // Variables live on entry to the block
        std::unordered_set<AstVarScope*> m_liveOut;  // Variables live on exit from the block
        যৌক্তিক m_isOnWorkList = মিথ্যা;  // Block is on work list
        যৌক্তিক m_wasProcessed = মিথ্যা;  // Already processed at least once
    };

    // STATE
    ধ্রুবক CfgGraph& m_cfg;  // The CFG beign analysed
    // State for each block
    CfgBlockMap<BlockState> m_blockState{m_cfg.makeBlockMap<BlockState>()};
    BlockState* m_currp = nullptr;  // State of current block being analysed
    যৌক্তিক m_abort = মিথ্যা;  // Abort analysis - unhandled construct

    // METHODS

    স্থির যৌক্তিক isPacked(ধ্রুবক AstVarScope* vscp) {
        Ast_গ্রন্থি_DType* ধ্রুবক dtypep = vscp->dtypep()->skipRefp();
        যদি (ধ্রুবক AstBasicDType* ধ্রুবক typep = VN_CAST(dtypep, BasicDType)) {
            ফেরত typep->keyword().isIntNumeric();
        }
        যদি (VN_IS(dtypep, PackArrayDType)) ফেরত সত্য;
        যদি (ধ্রুবক Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক typep = VN_CAST(dtypep, গ্রন্থি_UOr_Struct_DType)) {
            ফেরত typep->packed();
        }
        ফেরত মিথ্যা;
    }

    শূন্য updateGen(Ast_গ্রন্থি* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ধ্রুবক AstVarRef* refp) {
            // Only care about reads
            যদি (refp->access().isWriteOnly()) ফেরত মিথ্যা;
            // Grab referenced variable
            AstVarScope* ধ্রুবক vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            যদি (m_currp->m_kill.গুনতি(vscp)) ফেরত মিথ্যা;
            m_currp->m_gen.emplace(vscp);
            ফেরত মিথ্যা;
        });
    }

    শূন্য updateKill(Ast_গ্রন্থি* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ধ্রুবক AstVarRef* refp) {
            // Only care about writes
            যদি (refp->access().isReadOnly()) ফেরত মিথ্যা;
            // Grab referenced variable
            AstVarScope* ধ্রুবক vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            যদি (vscp->varp()->ignoreSchedWrite()) ফেরত সত্য;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            যদি (!isPacked(vscp)) ফেরত মিথ্যা;
            // If packed, and updated partially, don't add to kill set
            যদি (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) ফেরত মিথ্যা;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            ফেরত মিথ্যা;
        });
    }

    শূন্য single(Ast_গ্রন্থি* nodep) {
        // Assume all reads hapen before any writes
        যদি (m_abort) ফেরত;
        updateGen(nodep);
        যদি (m_abort) ফেরত;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    যৌক্তিক transfer(ধ্রুবক CfgBlock& bb) {
        BlockState& state = m_blockState[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_set<AstVarScope*> liveIn = state.m_gen;
        জন্য (AstVarScope* ধ্রুবক varp : state.m_liveOut) {
            যদি (state.m_kill.গুনতি(varp)) চালু;
            liveIn.ঢোকাও(varp);
        }
        যদি (liveIn == state.m_liveIn) ফেরত মিথ্যা;
        std::swap(liveIn, state.m_liveIn);
        ফেরত সত্য;
    }

    // VISIT
    শূন্য visit(Ast_গ্রন্থি* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = সত্য;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    শূন্য visit(AstAssign* nodep) override { single(nodep); }
    শূন্য visit(AstAssignW* nodep) override { single(nodep); }
    শূন্য visit(AstDisplay* nodep) override { single(nodep); }
    শূন্য visit(AstFinish* nodep) override { single(nodep); }
    শূন্য visit(AstFinishFork* nodep) override { single(nodep); }
    শূন্য visit(Ast_Stmt_রাশিমালা* nodep) override { single(nodep); }
    শূন্য visit(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    শূন্য visit(AstIf* nodep) override { single(nodep->condp()); }
    শূন্য visit(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    explicit CfgLiveVariables(ধ্রুবক CfgGraph& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        জন্য (ধ্রুবক V3GraphVertex& vtx : cfg.vertices()) {
            ধ্রুবক CfgBlock& bb = static_cast<ধ্রুবক CfgBlock&>(vtx);
            যদি (m_abort) ফেরত;
            VL_RESTORER(m_currp);
            m_currp = &m_blockState[bb];
            জন্য (Ast_গ্রন্থি* ধ্রুবক stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        যদি (m_abort) ফেরত;

        // Perform the flow analysis
        std::deque<ধ্রুবক CfgBlock*> workList;
        ধ্রুবক auto enqueue = [&](ধ্রুবক CfgBlock& bb) {
            BlockState& state = m_blockState[bb];
            যদি (state.m_isOnWorkList) ফেরত;
            state.m_isOnWorkList = সত্য;
            workList.emplace_back(&bb);
        };

        enqueue(cfg.exit());

        যখন (!workList.empty()) {
            ধ্রুবক CfgBlock* ধ্রুবক currp = workList.front();
            workList.বের_করো_front();
            BlockState& state = m_blockState[*currp];
            state.m_isOnWorkList = মিথ্যা;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](ধ্রুবক CfgBlock& bb) {
                auto& liveIn = m_blockState[bb].m_liveIn;
                state.m_liveOut.ঢোকাও(liveIn.begin(), liveIn.end());
            });

            // Apply transfer function of block
            ধ্রুবক যৌক্তিক changed = transfer(*currp);
            // Enqueue predecessors
            যদি (changed || !state.m_wasProcessed) currp->forEachPredecessor(enqueue);
            // Mark as done with first visit
            state.m_wasProcessed = সত্য;
        }
    }

সর্বজনীন:
    স্থির std::unique_ptr<std::vector<AstVarScope*>> apply(ধ্রুবক CfgGraph& cfg) {
        CfgLiveVariables analysis{cfg};
        // If failed, return nullptr
        যদি (analysis.m_abort) ফেরত nullptr;
        // Gather variables live in to the entry block
        ধ্রুবক std::unordered_set<AstVarScope*>& lin = analysis.m_blockState[cfg.enter()].m_liveIn;
        std::vector<AstVarScope*>* ধ্রুবক resultp
            = নতুন std::vector<AstVarScope*>{lin.begin(), lin.end()};
        // Sort for stability
        std::stable_sort(resultp->begin(), resultp->end(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             ফেরত ap->name() < bp->name();
                         });
        ফেরত std::unique_ptr<std::vector<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::vector<AstVarScope*>> V3Cfg::liveVarScopes(ধ্রুবক CfgGraph& cfg) {
    ফেরত CfgLiveVariables::apply(cfg);
}
