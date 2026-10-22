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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Cfg.h"

#شامل <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

رده CfgLiveVariables final : VNVisitorConst {
    // State associted with each basic block
    ساختار BlockState final {
        // Variables used in block, before a complete assignment in the same block
        std::unordered_set<AstVarScope*> m_gen;
        // Variables that are assigned a complete value in the basic block
        std::unordered_set<AstVarScope*> m_kill;
        std::unordered_set<AstVarScope*> m_liveIn;  // Variables live on entry to the block
        std::unordered_set<AstVarScope*> m_liveOut;  // Variables live on exit from the block
        منطقی m_isOnWorkList = نادرست;  // Block is on work list
        منطقی m_wasProcessed = نادرست;  // Already processed at least once
    };

    // STATE
    ثابت CfgGraph& m_cfg;  // The CFG beign analysed
    // State for each block
    CfgBlockMap<BlockState> m_blockState{m_cfg.makeBlockMap<BlockState>()};
    BlockState* m_currp = nullptr;  // State of current block being analysed
    منطقی m_abort = نادرست;  // Abort analysis - unhandled construct

    // METHODS

    ایستا منطقی isPacked(ثابت AstVarScope* vscp) {
        Ast_گره_DType* ثابت dtypep = vscp->dtypep()->skipRefp();
        اگر (ثابت AstBasicDType* ثابت typep = VN_CAST(dtypep, BasicDType)) {
            بازگشت typep->keyword().isIntNumeric();
        }
        اگر (VN_IS(dtypep, PackArrayDType)) بازگشت درست;
        اگر (ثابت Ast_گره_UOr_Struct_DType* ثابت typep = VN_CAST(dtypep, گره_UOr_Struct_DType)) {
            بازگشت typep->packed();
        }
        بازگشت نادرست;
    }

    تهی updateGen(Ast_گره* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ثابت AstVarRef* refp) {
            // Only care about reads
            اگر (refp->access().isWriteOnly()) بازگشت نادرست;
            // Grab referenced variable
            AstVarScope* ثابت vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            اگر (m_currp->m_kill.شمار(vscp)) بازگشت نادرست;
            m_currp->m_gen.emplace(vscp);
            بازگشت نادرست;
        });
    }

    تهی updateKill(Ast_گره* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ثابت AstVarRef* refp) {
            // Only care about writes
            اگر (refp->access().isReadOnly()) بازگشت نادرست;
            // Grab referenced variable
            AstVarScope* ثابت vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            اگر (vscp->varp()->ignoreSchedWrite()) بازگشت درست;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            اگر (!isPacked(vscp)) بازگشت نادرست;
            // If packed, and updated partially, don't add to kill set
            اگر (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) بازگشت نادرست;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            بازگشت نادرست;
        });
    }

    تهی single(Ast_گره* nodep) {
        // Assume all reads hapen before any writes
        اگر (m_abort) بازگشت;
        updateGen(nodep);
        اگر (m_abort) بازگشت;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    منطقی transfer(ثابت CfgBlock& bb) {
        BlockState& state = m_blockState[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_set<AstVarScope*> liveIn = state.m_gen;
        برای (AstVarScope* ثابت varp : state.m_liveOut) {
            اگر (state.m_kill.شمار(varp)) ادامه;
            liveIn.درج(varp);
        }
        اگر (liveIn == state.m_liveIn) بازگشت نادرست;
        std::swap(liveIn, state.m_liveIn);
        بازگشت درست;
    }

    // VISIT
    تهی visit(Ast_گره* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = درست;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    تهی visit(AstAssign* nodep) override { single(nodep); }
    تهی visit(AstAssignW* nodep) override { single(nodep); }
    تهی visit(AstDisplay* nodep) override { single(nodep); }
    تهی visit(AstFinish* nodep) override { single(nodep); }
    تهی visit(AstFinishFork* nodep) override { single(nodep); }
    تهی visit(Ast_Stmt_عبارت* nodep) override { single(nodep); }
    تهی visit(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    تهی visit(AstIf* nodep) override { single(nodep->condp()); }
    تهی visit(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    explicit CfgLiveVariables(ثابت CfgGraph& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        برای (ثابت V3GraphVertex& vtx : cfg.vertices()) {
            ثابت CfgBlock& bb = static_cast<ثابت CfgBlock&>(vtx);
            اگر (m_abort) بازگشت;
            VL_RESTORER(m_currp);
            m_currp = &m_blockState[bb];
            برای (Ast_گره* ثابت stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        اگر (m_abort) بازگشت;

        // Perform the flow analysis
        std::deque<ثابت CfgBlock*> workList;
        ثابت auto enqueue = [&](ثابت CfgBlock& bb) {
            BlockState& state = m_blockState[bb];
            اگر (state.m_isOnWorkList) بازگشت;
            state.m_isOnWorkList = درست;
            workList.emplace_back(&bb);
        };

        enqueue(cfg.exit());

        وقتی (!workList.empty()) {
            ثابت CfgBlock* ثابت currp = workList.front();
            workList.برداشت_front();
            BlockState& state = m_blockState[*currp];
            state.m_isOnWorkList = نادرست;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](ثابت CfgBlock& bb) {
                auto& liveIn = m_blockState[bb].m_liveIn;
                state.m_liveOut.درج(liveIn.begin(), liveIn.end());
            });

            // Apply transfer function of block
            ثابت منطقی changed = transfer(*currp);
            // Enqueue predecessors
            اگر (changed || !state.m_wasProcessed) currp->forEachPredecessor(enqueue);
            // Mark as done with first visit
            state.m_wasProcessed = درست;
        }
    }

عمومی:
    ایستا std::unique_ptr<std::vector<AstVarScope*>> apply(ثابت CfgGraph& cfg) {
        CfgLiveVariables analysis{cfg};
        // If failed, return nullptr
        اگر (analysis.m_abort) بازگشت nullptr;
        // Gather variables live in to the entry block
        ثابت std::unordered_set<AstVarScope*>& lin = analysis.m_blockState[cfg.enter()].m_liveIn;
        std::vector<AstVarScope*>* ثابت resultp
            = جدید std::vector<AstVarScope*>{lin.begin(), lin.end()};
        // Sort for stability
        std::stable_sort(resultp->begin(), resultp->end(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             بازگشت ap->name() < bp->name();
                         });
        بازگشت std::unique_ptr<std::vector<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::vector<AstVarScope*>> V3Cfg::liveVarScopes(ثابت CfgGraph& cfg) {
    بازگشت CfgLiveVariables::apply(cfg);
}
