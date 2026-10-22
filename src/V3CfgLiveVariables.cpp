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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Cfg.h"

#ಅನ್ತರ್ಭಾವ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

ವರ್ಗ CfgLiveVariables final : VNVisitorConst {
    // State associted with each basic block
    ಸಂರಚನಾ BlockState final {
        // Variables used in block, before a complete assignment in the same block
        std::unordered_set<AstVarScope*> m_gen;
        // Variables that are assigned a complete value in the basic block
        std::unordered_set<AstVarScope*> m_kill;
        std::unordered_set<AstVarScope*> m_liveIn;  // Variables live on entry to the block
        std::unordered_set<AstVarScope*> m_liveOut;  // Variables live on exit from the block
        ತಾರ್ಕಿಕ m_isOnWorkList = ಅಸತ್ಯ;  // Block is on work list
        ತಾರ್ಕಿಕ m_wasProcessed = ಅಸತ್ಯ;  // Already processed at least once
    };

    // STATE
    ಧ್ರುವ CfgGraph& m_cfg;  // The CFG beign analysed
    // State for each block
    CfgBlockMap<BlockState> m_blockState{m_cfg.makeBlockMap<BlockState>()};
    BlockState* m_currp = nullptr;  // State of current block being analysed
    ತಾರ್ಕಿಕ m_abort = ಅಸತ್ಯ;  // Abort analysis - unhandled construct

    // METHODS

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isPacked(ಧ್ರುವ AstVarScope* vscp) {
        Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = vscp->dtypep()->skipRefp();
        ಯದಿ (ಧ್ರುವ AstBasicDType* ಧ್ರುವ typep = VN_CAST(dtypep, BasicDType)) {
            ಪ್ರತ್ಯಾಗಮ typep->keyword().isIntNumeric();
        }
        ಯದಿ (VN_IS(dtypep, PackArrayDType)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ typep = VN_CAST(dtypep, ಗಂಟು_UOr_Struct_DType)) {
            ಪ್ರತ್ಯಾಗಮ typep->packed();
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ಶೂನ್ಯ updateGen(Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ಧ್ರುವ AstVarRef* refp) {
            // Only care about reads
            ಯದಿ (refp->access().isWriteOnly()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Grab referenced variable
            AstVarScope* ಧ್ರುವ vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            ಯದಿ (m_currp->m_kill.ಎಣಿಕೆ(vscp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            m_currp->m_gen.emplace(vscp);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
    }

    ಶೂನ್ಯ updateKill(Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ಧ್ರುವ AstVarRef* refp) {
            // Only care about writes
            ಯದಿ (refp->access().isReadOnly()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Grab referenced variable
            AstVarScope* ಧ್ರುವ vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            ಯದಿ (vscp->varp()->ignoreSchedWrite()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            ಯದಿ (!isPacked(vscp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // If packed, and updated partially, don't add to kill set
            ಯದಿ (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
    }

    ಶೂನ್ಯ single(Ast_ಗಂಟು* nodep) {
        // Assume all reads hapen before any writes
        ಯದಿ (m_abort) ಪ್ರತ್ಯಾಗಮ;
        updateGen(nodep);
        ಯದಿ (m_abort) ಪ್ರತ್ಯಾಗಮ;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    ತಾರ್ಕಿಕ transfer(ಧ್ರುವ CfgBlock& bb) {
        BlockState& state = m_blockState[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_set<AstVarScope*> liveIn = state.m_gen;
        ಪ್ರತಿ (AstVarScope* ಧ್ರುವ varp : state.m_liveOut) {
            ಯದಿ (state.m_kill.ಎಣಿಕೆ(varp)) ಅನುವರ್ತ;
            liveIn.ಸೇರಿಸು(varp);
        }
        ಯದಿ (liveIn == state.m_liveIn) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        std::swap(liveIn, state.m_liveIn);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // VISIT
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = ಸತ್ಯ;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    ಶೂನ್ಯ visit(AstAssign* nodep) override { single(nodep); }
    ಶೂನ್ಯ visit(AstAssignW* nodep) override { single(nodep); }
    ಶೂನ್ಯ visit(AstDisplay* nodep) override { single(nodep); }
    ಶೂನ್ಯ visit(AstFinish* nodep) override { single(nodep); }
    ಶೂನ್ಯ visit(AstFinishFork* nodep) override { single(nodep); }
    ಶೂನ್ಯ visit(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override { single(nodep); }
    ಶೂನ್ಯ visit(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    ಶೂನ್ಯ visit(AstIf* nodep) override { single(nodep->condp()); }
    ಶೂನ್ಯ visit(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    explicit CfgLiveVariables(ಧ್ರುವ CfgGraph& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : cfg.vertices()) {
            ಧ್ರುವ CfgBlock& bb = static_cast<ಧ್ರುವ CfgBlock&>(vtx);
            ಯದಿ (m_abort) ಪ್ರತ್ಯಾಗಮ;
            VL_RESTORER(m_currp);
            m_currp = &m_blockState[bb];
            ಪ್ರತಿ (Ast_ಗಂಟು* ಧ್ರುವ stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        ಯದಿ (m_abort) ಪ್ರತ್ಯಾಗಮ;

        // Perform the flow analysis
        std::deque<ಧ್ರುವ CfgBlock*> workList;
        ಧ್ರುವ auto enqueue = [&](ಧ್ರುವ CfgBlock& bb) {
            BlockState& state = m_blockState[bb];
            ಯದಿ (state.m_isOnWorkList) ಪ್ರತ್ಯಾಗಮ;
            state.m_isOnWorkList = ಸತ್ಯ;
            workList.emplace_back(&bb);
        };

        enqueue(cfg.exit());

        ಯಾವತ್ (!workList.empty()) {
            ಧ್ರುವ CfgBlock* ಧ್ರುವ currp = workList.front();
            workList.ತೆಗೆ_front();
            BlockState& state = m_blockState[*currp];
            state.m_isOnWorkList = ಅಸತ್ಯ;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](ಧ್ರುವ CfgBlock& bb) {
                auto& liveIn = m_blockState[bb].m_liveIn;
                state.m_liveOut.ಸೇರಿಸು(liveIn.begin(), liveIn.end());
            });

            // Apply transfer function of block
            ಧ್ರುವ ತಾರ್ಕಿಕ changed = transfer(*currp);
            // Enqueue predecessors
            ಯದಿ (changed || !state.m_wasProcessed) currp->forEachPredecessor(enqueue);
            // Mark as done with first visit
            state.m_wasProcessed = ಸತ್ಯ;
        }
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::unique_ptr<std::vector<AstVarScope*>> apply(ಧ್ರುವ CfgGraph& cfg) {
        CfgLiveVariables analysis{cfg};
        // If failed, return nullptr
        ಯದಿ (analysis.m_abort) ಪ್ರತ್ಯಾಗಮ nullptr;
        // Gather variables live in to the entry block
        ಧ್ರುವ std::unordered_set<AstVarScope*>& lin = analysis.m_blockState[cfg.enter()].m_liveIn;
        std::vector<AstVarScope*>* ಧ್ರುವ resultp
            = ನೂತನ std::vector<AstVarScope*>{lin.begin(), lin.end()};
        // Sort for stability
        std::stable_sort(resultp->begin(), resultp->end(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             ಪ್ರತ್ಯಾಗಮ ap->name() < bp->name();
                         });
        ಪ್ರತ್ಯಾಗಮ std::unique_ptr<std::vector<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::vector<AstVarScope*>> V3Cfg::liveVarScopes(ಧ್ರುವ CfgGraph& cfg) {
    ಪ್ರತ್ಯಾಗಮ CfgLiveVariables::apply(cfg);
}
