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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Cfg.h"

#અન્તર્ભાવ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

વર્ગ CfgLiveVariables final : VNVisitorConst {
    // State associted with each basic block
    સંરચના BlockState final {
        // Variables used in block, before a complete assignment in the same block
        std::unordered_set<AstVarScope*> m_gen;
        // Variables that are assigned a complete value in the basic block
        std::unordered_set<AstVarScope*> m_kill;
        std::unordered_set<AstVarScope*> m_liveIn;  // Variables live on entry to the block
        std::unordered_set<AstVarScope*> m_liveOut;  // Variables live on exit from the block
        તાર્કિક m_isOnWorkList = ખોટું;  // Block is on work list
        તાર્કિક m_wasProcessed = ખોટું;  // Already processed at least once
    };

    // STATE
    અચલ CfgGraph& m_cfg;  // The CFG beign analysed
    // State for each block
    CfgBlockMap<BlockState> m_blockState{m_cfg.makeBlockMap<BlockState>()};
    BlockState* m_currp = nullptr;  // State of current block being analysed
    તાર્કિક m_abort = ખોટું;  // Abort analysis - unhandled construct

    // METHODS

    સ્થિર તાર્કિક isPacked(અચલ AstVarScope* vscp) {
        Ast_ગાંઠ_DType* અચલ dtypep = vscp->dtypep()->skipRefp();
        જો (અચલ AstBasicDType* અચલ typep = VN_CAST(dtypep, BasicDType)) {
            પાછા typep->keyword().isIntNumeric();
        }
        જો (VN_IS(dtypep, PackArrayDType)) પાછા સાચું;
        જો (અચલ Ast_ગાંઠ_UOr_Struct_DType* અચલ typep = VN_CAST(dtypep, ગાંઠ_UOr_Struct_DType)) {
            પાછા typep->packed();
        }
        પાછા ખોટું;
    }

    શૂન્ય updateGen(Ast_ગાંઠ* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](અચલ AstVarRef* refp) {
            // Only care about reads
            જો (refp->access().isWriteOnly()) પાછા ખોટું;
            // Grab referenced variable
            AstVarScope* અચલ vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            જો (m_currp->m_kill.ગણતરી(vscp)) પાછા ખોટું;
            m_currp->m_gen.emplace(vscp);
            પાછા ખોટું;
        });
    }

    શૂન્ય updateKill(Ast_ગાંઠ* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](અચલ AstVarRef* refp) {
            // Only care about writes
            જો (refp->access().isReadOnly()) પાછા ખોટું;
            // Grab referenced variable
            AstVarScope* અચલ vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            જો (vscp->varp()->ignoreSchedWrite()) પાછા સાચું;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            જો (!isPacked(vscp)) પાછા ખોટું;
            // If packed, and updated partially, don't add to kill set
            જો (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) પાછા ખોટું;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            પાછા ખોટું;
        });
    }

    શૂન્ય single(Ast_ગાંઠ* nodep) {
        // Assume all reads hapen before any writes
        જો (m_abort) પાછા;
        updateGen(nodep);
        જો (m_abort) પાછા;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    તાર્કિક transfer(અચલ CfgBlock& bb) {
        BlockState& state = m_blockState[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_set<AstVarScope*> liveIn = state.m_gen;
        માટે (AstVarScope* અચલ varp : state.m_liveOut) {
            જો (state.m_kill.ગણતરી(varp)) ચાલુ;
            liveIn.ઉમેરો(varp);
        }
        જો (liveIn == state.m_liveIn) પાછા ખોટું;
        std::swap(liveIn, state.m_liveIn);
        પાછા સાચું;
    }

    // VISIT
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = સાચું;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    શૂન્ય visit(AstAssign* nodep) override { single(nodep); }
    શૂન્ય visit(AstAssignW* nodep) override { single(nodep); }
    શૂન્ય visit(AstDisplay* nodep) override { single(nodep); }
    શૂન્ય visit(AstFinish* nodep) override { single(nodep); }
    શૂન્ય visit(AstFinishFork* nodep) override { single(nodep); }
    શૂન્ય visit(Ast_Stmt_પદાવલિ* nodep) override { single(nodep); }
    શૂન્ય visit(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    શૂન્ય visit(AstIf* nodep) override { single(nodep->condp()); }
    શૂન્ય visit(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    explicit CfgLiveVariables(અચલ CfgGraph& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        માટે (અચલ V3GraphVertex& vtx : cfg.vertices()) {
            અચલ CfgBlock& bb = static_cast<અચલ CfgBlock&>(vtx);
            જો (m_abort) પાછા;
            VL_RESTORER(m_currp);
            m_currp = &m_blockState[bb];
            માટે (Ast_ગાંઠ* અચલ stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        જો (m_abort) પાછા;

        // Perform the flow analysis
        std::deque<અચલ CfgBlock*> workList;
        અચલ auto enqueue = [&](અચલ CfgBlock& bb) {
            BlockState& state = m_blockState[bb];
            જો (state.m_isOnWorkList) પાછા;
            state.m_isOnWorkList = સાચું;
            workList.emplace_back(&bb);
        };

        enqueue(cfg.exit());

        જ્યારે (!workList.empty()) {
            અચલ CfgBlock* અચલ currp = workList.front();
            workList.કાઢો_front();
            BlockState& state = m_blockState[*currp];
            state.m_isOnWorkList = ખોટું;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](અચલ CfgBlock& bb) {
                auto& liveIn = m_blockState[bb].m_liveIn;
                state.m_liveOut.ઉમેરો(liveIn.begin(), liveIn.end());
            });

            // Apply transfer function of block
            અચલ તાર્કિક changed = transfer(*currp);
            // Enqueue predecessors
            જો (changed || !state.m_wasProcessed) currp->forEachPredecessor(enqueue);
            // Mark as done with first visit
            state.m_wasProcessed = સાચું;
        }
    }

જાહેર:
    સ્થિર std::unique_ptr<std::vector<AstVarScope*>> apply(અચલ CfgGraph& cfg) {
        CfgLiveVariables analysis{cfg};
        // If failed, return nullptr
        જો (analysis.m_abort) પાછા nullptr;
        // Gather variables live in to the entry block
        અચલ std::unordered_set<AstVarScope*>& lin = analysis.m_blockState[cfg.enter()].m_liveIn;
        std::vector<AstVarScope*>* અચલ resultp
            = નવું std::vector<AstVarScope*>{lin.begin(), lin.end()};
        // Sort for stability
        std::stable_sort(resultp->begin(), resultp->end(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             પાછા ap->name() < bp->name();
                         });
        પાછા std::unique_ptr<std::vector<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::vector<AstVarScope*>> V3Cfg::liveVarScopes(અચલ CfgGraph& cfg) {
    પાછા CfgLiveVariables::apply(cfg);
}
