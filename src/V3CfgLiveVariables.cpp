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
        منطقي m_isOnWorkList = خطأ;  // Block is on work list
        منطقي m_wasProcessed = خطأ;  // Already processed at least once
    };

    // STATE
    ثابت CfgGraph& m_cfg;  // The CFG beign analysed
    // State for each block
    CfgBlockMap<BlockState> m_blockState{m_cfg.makeBlockMap<BlockState>()};
    BlockState* m_currp = nullptr;  // State of current block being analysed
    منطقي m_abort = خطأ;  // Abort analysis - unhandled construct

    // METHODS

    ساكن منطقي isPacked(ثابت AstVarScope* vscp) {
        Ast_نود_DType* ثابت dtypep = vscp->dtypep()->skipRefp();
        إذا (ثابت AstBasicDType* ثابت typep = VN_CAST(dtypep, BasicDType)) {
            أرجع typep->keyword().isIntNumeric();
        }
        إذا (VN_IS(dtypep, PackArrayDType)) أرجع true;
        إذا (ثابت Ast_نود_UOr_Struct_DType* ثابت typep = VN_CAST(dtypep, نود_UOr_Struct_DType)) {
            أرجع typep->packed();
        }
        أرجع خطأ;
    }

    فراغ updateGen(Ast_نود* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ثابت AstVarRef* refp) {
            // Only care about reads
            إذا (refp->access().isWriteOnly()) أرجع خطأ;
            // Grab referenced variable
            AstVarScope* ثابت vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            إذا (m_currp->m_kill.کيراءن(vscp)) أرجع خطأ;
            m_currp->m_gen.emplace(vscp);
            أرجع خطأ;
        });
    }

    فراغ updateKill(Ast_نود* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ثابت AstVarRef* refp) {
            // Only care about writes
            إذا (refp->access().isReadOnly()) أرجع خطأ;
            // Grab referenced variable
            AstVarScope* ثابت vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            إذا (vscp->varp()->ignoreSchedWrite()) أرجع true;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            إذا (!isPacked(vscp)) أرجع خطأ;
            // If packed, and updated partially, don't add to kill set
            إذا (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) أرجع خطأ;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            أرجع خطأ;
        });
    }

    فراغ single(Ast_نود* nodep) {
        // Assume all reads hapen before any writes
        إذا (m_abort) أرجع;
        updateGen(nodep);
        إذا (m_abort) أرجع;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    منطقي transfer(ثابت CfgBlock& bb) {
        BlockState& state = m_blockState[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_set<AstVarScope*> liveIn = state.m_gen;
        لكل (AstVarScope* ثابت varp : state.m_liveOut) {
            إذا (state.m_kill.کيراءن(varp)) استمر;
            liveIn.سيسيڤ(varp);
        }
        إذا (liveIn == state.m_liveIn) أرجع خطأ;
        std::swap(liveIn, state.m_liveIn);
        أرجع true;
    }

    // VISIT
    فراغ visit(Ast_نود* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = true;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    فراغ visit(AstAssign* nodep) override { single(nodep); }
    فراغ visit(AstAssignW* nodep) override { single(nodep); }
    فراغ visit(AstDisplay* nodep) override { single(nodep); }
    فراغ visit(AstFinish* nodep) override { single(nodep); }
    فراغ visit(AstFinishFork* nodep) override { single(nodep); }
    فراغ visit(Ast_Stmt_اوڠکاڤن* nodep) override { single(nodep); }
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
            إذا (m_abort) أرجع;
            VL_RESTORER(m_currp);
            m_currp = &m_blockState[bb];
            لكل (Ast_نود* ثابت stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        إذا (m_abort) أرجع;

        // Perform the flow analysis
        std::deque<ثابت CfgBlock*> workList;
        ثابت auto enqueue = [&](ثابت CfgBlock& bb) {
            BlockState& state = m_blockState[bb];
            إذا (state.m_isOnWorkList) أرجع;
            state.m_isOnWorkList = true;
            workList.emplace_back(&bb);
        };

        enqueue(cfg.exit());

        طالما (!workList.empty()) {
            ثابت CfgBlock* ثابت currp = workList.front();
            workList.کلوار_front();
            BlockState& state = m_blockState[*currp];
            state.m_isOnWorkList = خطأ;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](ثابت CfgBlock& bb) {
                auto& liveIn = m_blockState[bb].m_liveIn;
                state.m_liveOut.سيسيڤ(liveIn.begin(), liveIn.end());
            });

            // Apply transfer function of block
            ثابت منطقي changed = transfer(*currp);
            // Enqueue predecessors
            إذا (changed || !state.m_wasProcessed) currp->forEachPredecessor(enqueue);
            // Mark as done with first visit
            state.m_wasProcessed = true;
        }
    }

عام:
    ساكن std::unique_ptr<std::vector<AstVarScope*>> apply(ثابت CfgGraph& cfg) {
        CfgLiveVariables analysis{cfg};
        // If failed, return nullptr
        إذا (analysis.m_abort) أرجع nullptr;
        // Gather variables live in to the entry block
        ثابت std::unordered_set<AstVarScope*>& lin = analysis.m_blockState[cfg.enter()].m_liveIn;
        std::vector<AstVarScope*>* ثابت resultp
            = جديد std::vector<AstVarScope*>{lin.begin(), lin.end()};
        // Sort for stability
        std::stable_sort(resultp->begin(), resultp->end(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             أرجع ap->name() < bp->name();
                         });
        أرجع std::unique_ptr<std::vector<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::vector<AstVarScope*>> V3Cfg::liveVarScopes(ثابت CfgGraph& cfg) {
    أرجع CfgLiveVariables::apply(cfg);
}
