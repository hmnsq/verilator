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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Cfg.h"

#അന്തര്ഭാവ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

വർഗ്ഗം CfgLiveVariables final : VNVisitorConst {
    // State associted with each basic block
    ഘടന BlockState final {
        // Variables used in block, before a complete assignment in the same block
        std::unordered_set<AstVarScope*> m_gen;
        // Variables that are assigned a complete value in the basic block
        std::unordered_set<AstVarScope*> m_kill;
        std::unordered_set<AstVarScope*> m_liveIn;  // Variables live on entry to the block
        std::unordered_set<AstVarScope*> m_liveOut;  // Variables live on exit from the block
        തർക്കം m_isOnWorkList = അസത്യം;  // Block is on work list
        തർക്കം m_wasProcessed = അസത്യം;  // Already processed at least once
    };

    // STATE
    സ്ഥിരം CfgGraph& m_cfg;  // The CFG beign analysed
    // State for each block
    CfgBlockMap<BlockState> m_blockState{m_cfg.makeBlockMap<BlockState>()};
    BlockState* m_currp = nullptr;  // State of current block being analysed
    തർക്കം m_abort = അസത്യം;  // Abort analysis - unhandled construct

    // METHODS

    സ്ഥിതം തർക്കം isPacked(സ്ഥിരം AstVarScope* vscp) {
        Ast_കണ്ണി_DType* സ്ഥിരം dtypep = vscp->dtypep()->skipRefp();
        എങ്കിൽ (സ്ഥിരം AstBasicDType* സ്ഥിരം typep = VN_CAST(dtypep, BasicDType)) {
            തിരികെ typep->keyword().isIntNumeric();
        }
        എങ്കിൽ (VN_IS(dtypep, PackArrayDType)) തിരികെ സത്യം;
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം typep = VN_CAST(dtypep, കണ്ണി_UOr_Struct_DType)) {
            തിരികെ typep->packed();
        }
        തിരികെ അസത്യം;
    }

    ശൂന്യം updateGen(Ast_കണ്ണി* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](സ്ഥിരം AstVarRef* refp) {
            // Only care about reads
            എങ്കിൽ (refp->access().isWriteOnly()) തിരികെ അസത്യം;
            // Grab referenced variable
            AstVarScope* സ്ഥിരം vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            എങ്കിൽ (m_currp->m_kill.എണ്ണം(vscp)) തിരികെ അസത്യം;
            m_currp->m_gen.emplace(vscp);
            തിരികെ അസത്യം;
        });
    }

    ശൂന്യം updateKill(Ast_കണ്ണി* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](സ്ഥിരം AstVarRef* refp) {
            // Only care about writes
            എങ്കിൽ (refp->access().isReadOnly()) തിരികെ അസത്യം;
            // Grab referenced variable
            AstVarScope* സ്ഥിരം vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            എങ്കിൽ (vscp->varp()->ignoreSchedWrite()) തിരികെ സത്യം;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            എങ്കിൽ (!isPacked(vscp)) തിരികെ അസത്യം;
            // If packed, and updated partially, don't add to kill set
            എങ്കിൽ (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) തിരികെ അസത്യം;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            തിരികെ അസത്യം;
        });
    }

    ശൂന്യം single(Ast_കണ്ണി* nodep) {
        // Assume all reads hapen before any writes
        എങ്കിൽ (m_abort) തിരികെ;
        updateGen(nodep);
        എങ്കിൽ (m_abort) തിരികെ;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    തർക്കം transfer(സ്ഥിരം CfgBlock& bb) {
        BlockState& state = m_blockState[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_set<AstVarScope*> liveIn = state.m_gen;
        വേണ്ടി (AstVarScope* സ്ഥിരം varp : state.m_liveOut) {
            എങ്കിൽ (state.m_kill.എണ്ണം(varp)) തുടരുക;
            liveIn.തിരുകുക(varp);
        }
        എങ്കിൽ (liveIn == state.m_liveIn) തിരികെ അസത്യം;
        std::swap(liveIn, state.m_liveIn);
        തിരികെ സത്യം;
    }

    // VISIT
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = സത്യം;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    ശൂന്യം visit(AstAssign* nodep) override { single(nodep); }
    ശൂന്യം visit(AstAssignW* nodep) override { single(nodep); }
    ശൂന്യം visit(AstDisplay* nodep) override { single(nodep); }
    ശൂന്യം visit(AstFinish* nodep) override { single(nodep); }
    ശൂന്യം visit(AstFinishFork* nodep) override { single(nodep); }
    ശൂന്യം visit(Ast_Stmt_വ്യഞ്ജകം* nodep) override { single(nodep); }
    ശൂന്യം visit(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    ശൂന്യം visit(AstIf* nodep) override { single(nodep->condp()); }
    ശൂന്യം visit(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    explicit CfgLiveVariables(സ്ഥിരം CfgGraph& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : cfg.vertices()) {
            സ്ഥിരം CfgBlock& bb = static_cast<സ്ഥിരം CfgBlock&>(vtx);
            എങ്കിൽ (m_abort) തിരികെ;
            VL_RESTORER(m_currp);
            m_currp = &m_blockState[bb];
            വേണ്ടി (Ast_കണ്ണി* സ്ഥിരം stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        എങ്കിൽ (m_abort) തിരികെ;

        // Perform the flow analysis
        std::deque<സ്ഥിരം CfgBlock*> workList;
        സ്ഥിരം auto enqueue = [&](സ്ഥിരം CfgBlock& bb) {
            BlockState& state = m_blockState[bb];
            എങ്കിൽ (state.m_isOnWorkList) തിരികെ;
            state.m_isOnWorkList = സത്യം;
            workList.emplace_back(&bb);
        };

        enqueue(cfg.exit());

        ആകുമ്പോൾ (!workList.empty()) {
            സ്ഥിരം CfgBlock* സ്ഥിരം currp = workList.front();
            workList.എടുക്കുക_front();
            BlockState& state = m_blockState[*currp];
            state.m_isOnWorkList = അസത്യം;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](സ്ഥിരം CfgBlock& bb) {
                auto& liveIn = m_blockState[bb].m_liveIn;
                state.m_liveOut.തിരുകുക(liveIn.begin(), liveIn.end());
            });

            // Apply transfer function of block
            സ്ഥിരം തർക്കം changed = transfer(*currp);
            // Enqueue predecessors
            എങ്കിൽ (changed || !state.m_wasProcessed) currp->forEachPredecessor(enqueue);
            // Mark as done with first visit
            state.m_wasProcessed = സത്യം;
        }
    }

പൊതു:
    സ്ഥിതം std::unique_ptr<std::vector<AstVarScope*>> apply(സ്ഥിരം CfgGraph& cfg) {
        CfgLiveVariables analysis{cfg};
        // If failed, return nullptr
        എങ്കിൽ (analysis.m_abort) തിരികെ nullptr;
        // Gather variables live in to the entry block
        സ്ഥിരം std::unordered_set<AstVarScope*>& lin = analysis.m_blockState[cfg.enter()].m_liveIn;
        std::vector<AstVarScope*>* സ്ഥിരം resultp
            = പുതിയ std::vector<AstVarScope*>{lin.begin(), lin.end()};
        // Sort for stability
        std::stable_sort(resultp->begin(), resultp->end(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             തിരികെ ap->name() < bp->name();
                         });
        തിരികെ std::unique_ptr<std::vector<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::vector<AstVarScope*>> V3Cfg::liveVarScopes(സ്ഥിരം CfgGraph& cfg) {
    തിരികെ CfgLiveVariables::apply(cfg);
}
