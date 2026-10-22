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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Cfg.h"

#ଅନ୍ତର୍ଭାଵ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

ଶ୍ରେଣୀ CfgLiveVariables final : VNVisitorConst {
    // State associted with each basic block
    ସଂରଚନା BlockState final {
        // Variables used in block, before a complete assignment in the same block
        std::unordered_set<AstVarScope*> m_gen;
        // Variables that are assigned a complete value in the basic block
        std::unordered_set<AstVarScope*> m_kill;
        std::unordered_set<AstVarScope*> m_liveIn;  // Variables live on entry to the block
        std::unordered_set<AstVarScope*> m_liveOut;  // Variables live on exit from the block
        ତାର୍କିକ m_isOnWorkList = ମିଥ୍ୟା;  // Block is on work list
        ତାର୍କିକ m_wasProcessed = ମିଥ୍ୟା;  // Already processed at least once
    };

    // STATE
    ଅଚଳ CfgGraph& m_cfg;  // The CFG beign analysed
    // State for each block
    CfgBlockMap<BlockState> m_blockState{m_cfg.makeBlockMap<BlockState>()};
    BlockState* m_currp = nullptr;  // State of current block being analysed
    ତାର୍କିକ m_abort = ମିଥ୍ୟା;  // Abort analysis - unhandled construct

    // METHODS

    ସ୍ଥିର ତାର୍କିକ isPacked(ଅଚଳ AstVarScope* vscp) {
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = vscp->dtypep()->skipRefp();
        ଯଦି (ଅଚଳ AstBasicDType* ଅଚଳ typep = VN_CAST(dtypep, BasicDType)) {
            ଫେରନ୍ତୁ typep->keyword().isIntNumeric();
        }
        ଯଦି (VN_IS(dtypep, PackArrayDType)) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ typep = VN_CAST(dtypep, ଗଣ୍ଠି_UOr_Struct_DType)) {
            ଫେରନ୍ତୁ typep->packed();
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ଶୂନ୍ୟ updateGen(Ast_ଗଣ୍ଠି* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ଅଚଳ AstVarRef* refp) {
            // Only care about reads
            ଯଦି (refp->access().isWriteOnly()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Grab referenced variable
            AstVarScope* ଅଚଳ vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            ଯଦି (m_currp->m_kill.ଗଣନା_2(vscp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            m_currp->m_gen.emplace(vscp);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });
    }

    ଶୂନ୍ୟ updateKill(Ast_ଗଣ୍ଠି* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ଅଚଳ AstVarRef* refp) {
            // Only care about writes
            ଯଦି (refp->access().isReadOnly()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Grab referenced variable
            AstVarScope* ଅଚଳ vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            ଯଦି (vscp->varp()->ignoreSchedWrite()) ଫେରନ୍ତୁ ସତ୍ୟ;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            ଯଦି (!isPacked(vscp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // If packed, and updated partially, don't add to kill set
            ଯଦି (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });
    }

    ଶୂନ୍ୟ single(Ast_ଗଣ୍ଠି* nodep) {
        // Assume all reads hapen before any writes
        ଯଦି (m_abort) ଫେରନ୍ତୁ;
        updateGen(nodep);
        ଯଦି (m_abort) ଫେରନ୍ତୁ;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    ତାର୍କିକ transfer(ଅଚଳ CfgBlock& bb) {
        BlockState& state = m_blockState[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_set<AstVarScope*> liveIn = state.m_gen;
        ପାଇଁ (AstVarScope* ଅଚଳ varp : state.m_liveOut) {
            ଯଦି (state.m_kill.ଗଣନା_2(varp)) ଜାରି;
            liveIn.ଭର୍ତ୍ତି(varp);
        }
        ଯଦି (liveIn == state.m_liveIn) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        std::swap(liveIn, state.m_liveIn);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // VISIT
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = ସତ୍ୟ;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    ଶୂନ୍ୟ visit(AstAssign* nodep) override { single(nodep); }
    ଶୂନ୍ୟ visit(AstAssignW* nodep) override { single(nodep); }
    ଶୂନ୍ୟ visit(AstDisplay* nodep) override { single(nodep); }
    ଶୂନ୍ୟ visit(AstFinish* nodep) override { single(nodep); }
    ଶୂନ୍ୟ visit(AstFinishFork* nodep) override { single(nodep); }
    ଶୂନ୍ୟ visit(Ast_Stmt_ପଦସମଷ୍ଟି* nodep) override { single(nodep); }
    ଶୂନ୍ୟ visit(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    ଶୂନ୍ୟ visit(AstIf* nodep) override { single(nodep->condp()); }
    ଶୂନ୍ୟ visit(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    explicit CfgLiveVariables(ଅଚଳ CfgGraph& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : cfg.vertices()) {
            ଅଚଳ CfgBlock& bb = static_cast<ଅଚଳ CfgBlock&>(vtx);
            ଯଦି (m_abort) ଫେରନ୍ତୁ;
            VL_RESTORER(m_currp);
            m_currp = &m_blockState[bb];
            ପାଇଁ (Ast_ଗଣ୍ଠି* ଅଚଳ stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        ଯଦି (m_abort) ଫେରନ୍ତୁ;

        // Perform the flow analysis
        std::deque<ଅଚଳ CfgBlock*> workList;
        ଅଚଳ auto enqueue = [&](ଅଚଳ CfgBlock& bb) {
            BlockState& state = m_blockState[bb];
            ଯଦି (state.m_isOnWorkList) ଫେରନ୍ତୁ;
            state.m_isOnWorkList = ସତ୍ୟ;
            workList.emplace_back(&bb);
        };

        enqueue(cfg.exit());

        ଯେତେବେଳେ (!workList.empty()) {
            ଅଚଳ CfgBlock* ଅଚଳ currp = workList.front();
            workList.କାଢ଼_front();
            BlockState& state = m_blockState[*currp];
            state.m_isOnWorkList = ମିଥ୍ୟା;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](ଅଚଳ CfgBlock& bb) {
                auto& liveIn = m_blockState[bb].m_liveIn;
                state.m_liveOut.ଭର୍ତ୍ତି(liveIn.begin(), liveIn.end());
            });

            // Apply transfer function of block
            ଅଚଳ ତାର୍କିକ changed = transfer(*currp);
            // Enqueue predecessors
            ଯଦି (changed || !state.m_wasProcessed) currp->forEachPredecessor(enqueue);
            // Mark as done with first visit
            state.m_wasProcessed = ସତ୍ୟ;
        }
    }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର std::unique_ptr<std::vector<AstVarScope*>> apply(ଅଚଳ CfgGraph& cfg) {
        CfgLiveVariables analysis{cfg};
        // If failed, return nullptr
        ଯଦି (analysis.m_abort) ଫେରନ୍ତୁ nullptr;
        // Gather variables live in to the entry block
        ଅଚଳ std::unordered_set<AstVarScope*>& lin = analysis.m_blockState[cfg.enter()].m_liveIn;
        std::vector<AstVarScope*>* ଅଚଳ resultp
            = ନୂତନ std::vector<AstVarScope*>{lin.begin(), lin.end()};
        // Sort for stability
        std::stable_sort(resultp->begin(), resultp->end(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             ଫେରନ୍ତୁ ap->name() < bp->name();
                         });
        ଫେରନ୍ତୁ std::unique_ptr<std::vector<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::vector<AstVarScope*>> V3Cfg::liveVarScopes(ଅଚଳ CfgGraph& cfg) {
    ଫେରନ୍ତୁ CfgLiveVariables::apply(cfg);
}
