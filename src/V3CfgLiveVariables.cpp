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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Cfg.h"

#ضمن <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

صنف CfgLiveVariables final : VNVisitorConst {
    // State associted with each basic block
    بنية BlockState final {
        // Variables used in block, before a complete assignment in the same block
        std::unordered_set<AstVarScope*> m_gen;
        // Variables that are assigned a complete value in the basic block
        std::unordered_set<AstVarScope*> m_kill;
        std::unordered_set<AstVarScope*> m_liveIn;  // Variables live on entry to the block
        std::unordered_set<AstVarScope*> m_liveOut;  // Variables live on exit from the block
        منطقي m_isOnWorkList = ܕܓܠܐ;  // Block is on work list
        منطقي m_wasProcessed = ܕܓܠܐ;  // Already processed at least once
    };

    // STATE
    ثابت CfgGraph& m_cfg;  // The CFG beign analysed
    // State for each block
    CfgBlockMap<BlockState> m_blockState{m_cfg.makeBlockMap<BlockState>()};
    BlockState* m_currp = nullptr;  // State of current block being analysed
    منطقي m_abort = ܕܓܠܐ;  // Abort analysis - unhandled construct

    // METHODS

    ساكن منطقي isPacked(ثابت AstVarScope* vscp) {
        AstNodeDType* ثابت dtypep = vscp->dtypep()->skipRefp();
        ܐܢ (ثابت AstBasicDType* ثابت typep = VN_CAST(dtypep, BasicDType)) {
            ܦܢܐ typep->keyword().isIntNumeric();
        }
        ܐܢ (VN_IS(dtypep, PackArrayDType)) ܦܢܐ ܫܪܪܐ;
        ܐܢ (ثابت AstNodeUOrStructDType* ثابت typep = VN_CAST(dtypep, NodeUOrStructDType)) {
            ܦܢܐ typep->packed();
        }
        ܦܢܐ ܕܓܠܐ;
    }

    فراغ updateGen(AstNode* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ثابت AstVarRef* refp) {
            // Only care about reads
            ܐܢ (refp->access().isWriteOnly()) ܦܢܐ ܕܓܠܐ;
            // Grab referenced variable
            AstVarScope* ثابت vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            ܐܢ (m_currp->m_kill.count(vscp)) ܦܢܐ ܕܓܠܐ;
            m_currp->m_gen.emplace(vscp);
            ܦܢܐ ܕܓܠܐ;
        });
    }

    فراغ updateKill(AstNode* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ثابت AstVarRef* refp) {
            // Only care about writes
            ܐܢ (refp->access().isReadOnly()) ܦܢܐ ܕܓܠܐ;
            // Grab referenced variable
            AstVarScope* ثابت vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            ܐܢ (vscp->varp()->ignoreSchedWrite()) ܦܢܐ ܫܪܪܐ;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            ܐܢ (!isPacked(vscp)) ܦܢܐ ܕܓܠܐ;
            // If packed, and updated partially, don't add to kill set
            ܐܢ (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) ܦܢܐ ܕܓܠܐ;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            ܦܢܐ ܕܓܠܐ;
        });
    }

    فراغ single(AstNode* nodep) {
        // Assume all reads hapen before any writes
        ܐܢ (m_abort) ܦܢܐ;
        updateGen(nodep);
        ܐܢ (m_abort) ܦܢܐ;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    منطقي transfer(ثابت CfgBlock& bb) {
        BlockState& state = m_blockState[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_set<AstVarScope*> liveIn = state.m_gen;
        لكل (AstVarScope* ثابت varp : state.m_liveOut) {
            ܐܢ (state.m_kill.count(varp)) استمر;
            liveIn.insert(varp);
        }
        ܐܢ (liveIn == state.m_liveIn) ܦܢܐ ܕܓܠܐ;
        std::swap(liveIn, state.m_liveIn);
        ܦܢܐ ܫܪܪܐ;
    }

    // VISIT
    فراغ visit(AstNode* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = ܫܪܪܐ;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    فراغ visit(AstAssign* nodep) override { single(nodep); }
    فراغ visit(AstAssignW* nodep) override { single(nodep); }
    فراغ visit(AstDisplay* nodep) override { single(nodep); }
    فراغ visit(AstFinish* nodep) override { single(nodep); }
    فراغ visit(AstFinishFork* nodep) override { single(nodep); }
    فراغ visit(AstStmtExpr* nodep) override { single(nodep); }
    فراغ visit(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    فراغ visit(AstIf* nodep) override { single(nodep->condp()); }
    فراغ visit(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    explicit CfgLiveVariables(ثابت CfgGraph& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        لكل (ثابت V3GraphVertex& vtx : cfg.vertices()) {
            ثابت CfgBlock& bb = static_cast<ثابت CfgBlock&>(vtx);
            ܐܢ (m_abort) ܦܢܐ;
            VL_RESTORER(m_currp);
            m_currp = &m_blockState[bb];
            لكل (AstNode* ثابت stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        ܐܢ (m_abort) ܦܢܐ;

        // Perform the flow analysis
        std::deque<ثابت CfgBlock*> workList;
        ثابت auto enqueue = [&](ثابت CfgBlock& bb) {
            BlockState& state = m_blockState[bb];
            ܐܢ (state.m_isOnWorkList) ܦܢܐ;
            state.m_isOnWorkList = ܫܪܪܐ;
            workList.emplace_back(&bb);
        };

        enqueue(cfg.exit());

        ܟܡܐ (!workList.empty()) {
            ثابت CfgBlock* ثابت currp = workList.front();
            workList.pop_front();
            BlockState& state = m_blockState[*currp];
            state.m_isOnWorkList = ܕܓܠܐ;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](ثابت CfgBlock& bb) {
                auto& liveIn = m_blockState[bb].m_liveIn;
                state.m_liveOut.insert(liveIn.begin(), liveIn.end());
            });

            // Apply transfer function of block
            ثابت منطقي changed = transfer(*currp);
            // Enqueue predecessors
            ܐܢ (changed || !state.m_wasProcessed) currp->forEachPredecessor(enqueue);
            // Mark as done with first visit
            state.m_wasProcessed = ܫܪܪܐ;
        }
    }

عام:
    ساكن std::unique_ptr<std::vector<AstVarScope*>> apply(ثابت CfgGraph& cfg) {
        CfgLiveVariables analysis{cfg};
        // If failed, return nullptr
        ܐܢ (analysis.m_abort) ܦܢܐ nullptr;
        // Gather variables live in to the entry block
        ثابت std::unordered_set<AstVarScope*>& lin = analysis.m_blockState[cfg.enter()].m_liveIn;
        std::vector<AstVarScope*>* ثابت resultp
            = جديد std::vector<AstVarScope*>{lin.begin(), lin.end()};
        // Sort for stability
        std::stable_sort(resultp->begin(), resultp->end(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             ܦܢܐ ap->name() < bp->name();
                         });
        ܦܢܐ std::unique_ptr<std::vector<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::vector<AstVarScope*>> V3Cfg::liveVarScopes(ثابت CfgGraph& cfg) {
    ܦܢܐ CfgLiveVariables::apply(cfg);
}
