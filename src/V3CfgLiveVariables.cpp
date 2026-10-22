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

#include "config_build.h"
#include "verilatedos.h"

#include "V3Ast.h"
#include "V3Cfg.h"

#include <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

class CfgLiveVariables final : VNVisitorConst {
    // State associted with each basic block
    sa_nracanaa BlockState final {
        // Variables used in block, before a complete assignment in the same block
        std::unordered_set<AstVarScope*> m_gen;
        // Variables that are assigned a complete value in the basic block
        std::unordered_set<AstVarScope*> m_kill;
        std::unordered_set<AstVarScope*> m_liveIn;  // Variables live on entry to the block
        std::unordered_set<AstVarScope*> m_liveOut;  // Variables live on exit from the block
        bool m_isOnWorkList = false;  // Block is on work list
        bool m_wasProcessed = false;  // Already processed at least once
    };

    // STATE
    const CfgGraph& m_cfg;  // The CFG beign analysed
    // State for each block
    CfgBlockMap<BlockState> m_blockState{m_cfg.makeBlockMap<BlockState>()};
    BlockState* m_currp = nullptr;  // State of current block being analysed
    bool m_abort = false;  // Abort analysis - unhandled construct

    // METHODS

    static bool isPacked(const AstVarScope* vscp) {
        AstNodeDType* const dtypep = vscp->dtypep()->skipRefp();
        නම් (const AstBasicDType* const typep = VN_CAST(dtypep, BasicDType)) {
            ආපසු typep->keyword().isIntNumeric();
        }
        නම් (VN_IS(dtypep, PackArrayDType)) ආපසු true;
        නම් (const AstNodeUOrStructDType* const typep = VN_CAST(dtypep, NodeUOrStructDType)) {
            ආපසු typep->packed();
        }
        ආපසු false;
    }

    void updateGen(AstNode* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](const AstVarRef* refp) {
            // Only care about reads
            නම් (refp->access().isWriteOnly()) ආපසු false;
            // Grab referenced variable
            AstVarScope* const vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            නම් (m_currp->m_kill.count(vscp)) ආපසු false;
            m_currp->m_gen.emplace(vscp);
            ආපසු false;
        });
    }

    void updateKill(AstNode* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](const AstVarRef* refp) {
            // Only care about writes
            නම් (refp->access().isReadOnly()) ආපසු false;
            // Grab referenced variable
            AstVarScope* const vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            නම් (vscp->varp()->ignoreSchedWrite()) ආපසු true;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            නම් (!isPacked(vscp)) ආපසු false;
            // If packed, and updated partially, don't add to kill set
            නම් (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) ආපසු false;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            ආපසු false;
        });
    }

    void single(AstNode* nodep) {
        // Assume all reads hapen before any writes
        නම් (m_abort) ආපසු;
        updateGen(nodep);
        නම් (m_abort) ආපසු;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    bool transfer(const CfgBlock& bb) {
        BlockState& state = m_blockState[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_set<AstVarScope*> liveIn = state.m_gen;
        සඳහා (AstVarScope* const varp : state.m_liveOut) {
            නම් (state.m_kill.count(varp)) continue;
            liveIn.insert(varp);
        }
        නම් (liveIn == state.m_liveIn) ආපසු false;
        std::swap(liveIn, state.m_liveIn);
        ආපසු true;
    }

    // VISIT
    void visit(AstNode* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = true;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    void visit(AstAssign* nodep) override { single(nodep); }
    void visit(AstAssignW* nodep) override { single(nodep); }
    void visit(AstDisplay* nodep) override { single(nodep); }
    void visit(AstFinish* nodep) override { single(nodep); }
    void visit(AstFinishFork* nodep) override { single(nodep); }
    void visit(AstStmtExpr* nodep) override { single(nodep); }
    void visit(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    void visit(AstIf* nodep) override { single(nodep->condp()); }
    void visit(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    explicit CfgLiveVariables(const CfgGraph& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        සඳහා (const V3GraphVertex& vtx : cfg.vertices()) {
            const CfgBlock& bb = static_cast<const CfgBlock&>(vtx);
            නම් (m_abort) ආපසු;
            VL_RESTORER(m_currp);
            m_currp = &m_blockState[bb];
            සඳහා (AstNode* const stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        නම් (m_abort) ආපසු;

        // Perform the flow analysis
        std::deque<const CfgBlock*> workList;
        const auto enqueue = [&](const CfgBlock& bb) {
            BlockState& state = m_blockState[bb];
            නම් (state.m_isOnWorkList) ආපසු;
            state.m_isOnWorkList = true;
            workList.emplace_back(&bb);
        };

        enqueue(cfg.exit());

        අතරතුර (!workList.empty()) {
            const CfgBlock* const currp = workList.front();
            workList.pop_front();
            BlockState& state = m_blockState[*currp];
            state.m_isOnWorkList = false;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](const CfgBlock& bb) {
                auto& liveIn = m_blockState[bb].m_liveIn;
                state.m_liveOut.insert(liveIn.begin(), liveIn.end());
            });

            // Apply transfer function of block
            const bool changed = transfer(*currp);
            // Enqueue predecessors
            නම් (changed || !state.m_wasProcessed) currp->forEachPredecessor(enqueue);
            // Mark as done with first visit
            state.m_wasProcessed = true;
        }
    }

public:
    static std::unique_ptr<std::vector<AstVarScope*>> apply(const CfgGraph& cfg) {
        CfgLiveVariables analysis{cfg};
        // If failed, return nullptr
        නම් (analysis.m_abort) ආපසු nullptr;
        // Gather variables live in to the entry block
        const std::unordered_set<AstVarScope*>& lin = analysis.m_blockState[cfg.enter()].m_liveIn;
        std::vector<AstVarScope*>* const resultp
            = new std::vector<AstVarScope*>{lin.begin(), lin.end()};
        // Sort for stability
        std::stable_sort(resultp->begin(), resultp->end(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             ආපසු ap->name() < bp->name();
                         });
        ආපසු std::unique_ptr<std::vector<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::vector<AstVarScope*>> V3Cfg::liveVarScopes(const CfgGraph& cfg) {
    ආපසු CfgLiveVariables::apply(cfg);
}
