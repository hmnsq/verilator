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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Cfg.h"

#அந்தர்பாவ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

வகுப்பு CfgLiveVariables final : VNVisitorConst {
    // State associted with each basic block
    கட்டமைப்பு BlockState final {
        // Variables used in block, before a complete assignment in the same block
        std::unordered_set<AstVarScope*> m_gen;
        // Variables that are assigned a complete value in the basic block
        std::unordered_set<AstVarScope*> m_kill;
        std::unordered_set<AstVarScope*> m_liveIn;  // Variables live on entry to the block
        std::unordered_set<AstVarScope*> m_liveOut;  // Variables live on exit from the block
        தருக்க m_isOnWorkList = பொய்;  // Block is on work list
        தருக்க m_wasProcessed = பொய்;  // Already processed at least once
    };

    // STATE
    மாறிலி CfgGraph& m_cfg;  // The CFG beign analysed
    // State for each block
    CfgBlockMap<BlockState> m_blockState{m_cfg.makeBlockMap<BlockState>()};
    BlockState* m_currp = nullptr;  // State of current block being analysed
    தருக்க m_abort = பொய்;  // Abort analysis - unhandled construct

    // METHODS

    நிலையான தருக்க isPacked(மாறிலி AstVarScope* vscp) {
        Ast_முனை_DType* மாறிலி dtypep = vscp->dtypep()->skipRefp();
        எனில் (மாறிலி AstBasicDType* மாறிலி typep = VN_CAST(dtypep, BasicDType)) {
            திரும்பு typep->keyword().isIntNumeric();
        }
        எனில் (VN_IS(dtypep, PackArrayDType)) திரும்பு உண்மை;
        எனில் (மாறிலி Ast_முனை_UOr_Struct_DType* மாறிலி typep = VN_CAST(dtypep, முனை_UOr_Struct_DType)) {
            திரும்பு typep->packed();
        }
        திரும்பு பொய்;
    }

    வெறுமை updateGen(Ast_முனை* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](மாறிலி AstVarRef* refp) {
            // Only care about reads
            எனில் (refp->access().isWriteOnly()) திரும்பு பொய்;
            // Grab referenced variable
            AstVarScope* மாறிலி vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            எனில் (m_currp->m_kill.மொத்தம்(vscp)) திரும்பு பொய்;
            m_currp->m_gen.emplace(vscp);
            திரும்பு பொய்;
        });
    }

    வெறுமை updateKill(Ast_முனை* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](மாறிலி AstVarRef* refp) {
            // Only care about writes
            எனில் (refp->access().isReadOnly()) திரும்பு பொய்;
            // Grab referenced variable
            AstVarScope* மாறிலி vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            எனில் (vscp->varp()->ignoreSchedWrite()) திரும்பு உண்மை;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            எனில் (!isPacked(vscp)) திரும்பு பொய்;
            // If packed, and updated partially, don't add to kill set
            எனில் (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) திரும்பு பொய்;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            திரும்பு பொய்;
        });
    }

    வெறுமை single(Ast_முனை* nodep) {
        // Assume all reads hapen before any writes
        எனில் (m_abort) திரும்பு;
        updateGen(nodep);
        எனில் (m_abort) திரும்பு;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    தருக்க transfer(மாறிலி CfgBlock& bb) {
        BlockState& state = m_blockState[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_set<AstVarScope*> liveIn = state.m_gen;
        க்கு (AstVarScope* மாறிலி varp : state.m_liveOut) {
            எனில் (state.m_kill.மொத்தம்(varp)) தொடர்;
            liveIn.செருகு(varp);
        }
        எனில் (liveIn == state.m_liveIn) திரும்பு பொய்;
        std::swap(liveIn, state.m_liveIn);
        திரும்பு உண்மை;
    }

    // VISIT
    வெறுமை visit(Ast_முனை* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = உண்மை;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    வெறுமை visit(AstAssign* nodep) override { single(nodep); }
    வெறுமை visit(AstAssignW* nodep) override { single(nodep); }
    வெறுமை visit(AstDisplay* nodep) override { single(nodep); }
    வெறுமை visit(AstFinish* nodep) override { single(nodep); }
    வெறுமை visit(AstFinishFork* nodep) override { single(nodep); }
    வெறுமை visit(Ast_Stmt_கோவை* nodep) override { single(nodep); }
    வெறுமை visit(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    வெறுமை visit(AstIf* nodep) override { single(nodep->condp()); }
    வெறுமை visit(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    explicit CfgLiveVariables(மாறிலி CfgGraph& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        க்கு (மாறிலி V3GraphVertex& vtx : cfg.vertices()) {
            மாறிலி CfgBlock& bb = static_cast<மாறிலி CfgBlock&>(vtx);
            எனில் (m_abort) திரும்பு;
            VL_RESTORER(m_currp);
            m_currp = &m_blockState[bb];
            க்கு (Ast_முனை* மாறிலி stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        எனில் (m_abort) திரும்பு;

        // Perform the flow analysis
        std::deque<மாறிலி CfgBlock*> workList;
        மாறிலி auto enqueue = [&](மாறிலி CfgBlock& bb) {
            BlockState& state = m_blockState[bb];
            எனில் (state.m_isOnWorkList) திரும்பு;
            state.m_isOnWorkList = உண்மை;
            workList.emplace_back(&bb);
        };

        enqueue(cfg.exit());

        போது (!workList.empty()) {
            மாறிலி CfgBlock* மாறிலி currp = workList.front();
            workList.எடு_front();
            BlockState& state = m_blockState[*currp];
            state.m_isOnWorkList = பொய்;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](மாறிலி CfgBlock& bb) {
                auto& liveIn = m_blockState[bb].m_liveIn;
                state.m_liveOut.செருகு(liveIn.begin(), liveIn.end());
            });

            // Apply transfer function of block
            மாறிலி தருக்க changed = transfer(*currp);
            // Enqueue predecessors
            எனில் (changed || !state.m_wasProcessed) currp->forEachPredecessor(enqueue);
            // Mark as done with first visit
            state.m_wasProcessed = உண்மை;
        }
    }

பொது:
    நிலையான std::unique_ptr<std::vector<AstVarScope*>> apply(மாறிலி CfgGraph& cfg) {
        CfgLiveVariables analysis{cfg};
        // If failed, return nullptr
        எனில் (analysis.m_abort) திரும்பு nullptr;
        // Gather variables live in to the entry block
        மாறிலி std::unordered_set<AstVarScope*>& lin = analysis.m_blockState[cfg.enter()].m_liveIn;
        std::vector<AstVarScope*>* மாறிலி resultp
            = புதிய std::vector<AstVarScope*>{lin.begin(), lin.end()};
        // Sort for stability
        std::stable_sort(resultp->begin(), resultp->end(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             திரும்பு ap->name() < bp->name();
                         });
        திரும்பு std::unique_ptr<std::vector<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::vector<AstVarScope*>> V3Cfg::liveVarScopes(மாறிலி CfgGraph& cfg) {
    திரும்பு CfgLiveVariables::apply(cfg);
}
