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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Cfg.h"

#अन्तर्भाव <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

वर्ग CfgLiveVariables final : VNVisitorConst {
    // State associted with each basic block
    संरचना BlockState final {
        // Variables used in block, before a complete assignment in the same block
        std::unordered_set<AstVarScope*> m_gen;
        // Variables that are assigned a complete value in the basic block
        std::unordered_set<AstVarScope*> m_kill;
        std::unordered_set<AstVarScope*> m_liveIn;  // Variables live on entry to the block
        std::unordered_set<AstVarScope*> m_liveOut;  // Variables live on exit from the block
        तार्किक m_isOnWorkList = मिच्छ;  // Block is on work list
        तार्किक m_wasProcessed = मिच्छ;  // Already processed at least once
    };

    // STATE
    ध्रुव CfgGraph& m_cfg;  // The CFG beign analysed
    // State for each block
    CfgBlockMap<BlockState> m_blockState{m_cfg.makeBlockMap<BlockState>()};
    BlockState* m_currp = nullptr;  // State of current block being analysed
    तार्किक m_abort = मिच्छ;  // Abort analysis - unhandled construct

    // METHODS

    स्थिर तार्किक isPacked(ध्रुव AstVarScope* vscp) {
        Ast_ग्रन्थिः_DType* ध्रुव dtypep = vscp->dtypep()->skipRefp();
        जइ (ध्रुव AstBasicDType* ध्रुव typep = VN_CAST(dtypep, BasicDType)) {
            प्रत्यागम typep->keyword().isIntNumeric();
        }
        जइ (VN_IS(dtypep, PackArrayDType)) प्रत्यागम सच्च;
        जइ (ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव typep = VN_CAST(dtypep, ग्रन्थिः_UOr_Struct_DType)) {
            प्रत्यागम typep->packed();
        }
        प्रत्यागम मिच्छ;
    }

    शून्य updateGen(Ast_ग्रन्थिः* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ध्रुव AstVarRef* refp) {
            // Only care about reads
            जइ (refp->access().isWriteOnly()) प्रत्यागम मिच्छ;
            // Grab referenced variable
            AstVarScope* ध्रुव vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            जइ (m_currp->m_kill.गणनम्(vscp)) प्रत्यागम मिच्छ;
            m_currp->m_gen.emplace(vscp);
            प्रत्यागम मिच्छ;
        });
    }

    शून्य updateKill(Ast_ग्रन्थिः* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ध्रुव AstVarRef* refp) {
            // Only care about writes
            जइ (refp->access().isReadOnly()) प्रत्यागम मिच्छ;
            // Grab referenced variable
            AstVarScope* ध्रुव vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            जइ (vscp->varp()->ignoreSchedWrite()) प्रत्यागम सच्च;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            जइ (!isPacked(vscp)) प्रत्यागम मिच्छ;
            // If packed, and updated partially, don't add to kill set
            जइ (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) प्रत्यागम मिच्छ;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            प्रत्यागम मिच्छ;
        });
    }

    शून्य single(Ast_ग्रन्थिः* nodep) {
        // Assume all reads hapen before any writes
        जइ (m_abort) प्रत्यागम;
        updateGen(nodep);
        जइ (m_abort) प्रत्यागम;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    तार्किक transfer(ध्रुव CfgBlock& bb) {
        BlockState& state = m_blockState[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_set<AstVarScope*> liveIn = state.m_gen;
        प्रति (AstVarScope* ध्रुव varp : state.m_liveOut) {
            जइ (state.m_kill.गणनम्(varp)) अनुवर्त;
            liveIn.निवेशय(varp);
        }
        जइ (liveIn == state.m_liveIn) प्रत्यागम मिच्छ;
        std::swap(liveIn, state.m_liveIn);
        प्रत्यागम सच्च;
    }

    // VISIT
    शून्य visit(Ast_ग्रन्थिः* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = सच्च;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    शून्य visit(AstAssign* nodep) override { single(nodep); }
    शून्य visit(AstAssignW* nodep) override { single(nodep); }
    शून्य visit(AstDisplay* nodep) override { single(nodep); }
    शून्य visit(AstFinish* nodep) override { single(nodep); }
    शून्य visit(AstFinishFork* nodep) override { single(nodep); }
    शून्य visit(Ast_Stmt_व्यञ्जकम्* nodep) override { single(nodep); }
    शून्य visit(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    शून्य visit(AstIf* nodep) override { single(nodep->condp()); }
    शून्य visit(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    explicit CfgLiveVariables(ध्रुव CfgGraph& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        प्रति (ध्रुव V3GraphVertex& vtx : cfg.vertices()) {
            ध्रुव CfgBlock& bb = static_cast<ध्रुव CfgBlock&>(vtx);
            जइ (m_abort) प्रत्यागम;
            VL_RESTORER(m_currp);
            m_currp = &m_blockState[bb];
            प्रति (Ast_ग्रन्थिः* ध्रुव stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        जइ (m_abort) प्रत्यागम;

        // Perform the flow analysis
        std::deque<ध्रुव CfgBlock*> workList;
        ध्रुव auto enqueue = [&](ध्रुव CfgBlock& bb) {
            BlockState& state = m_blockState[bb];
            जइ (state.m_isOnWorkList) प्रत्यागम;
            state.m_isOnWorkList = सच्च;
            workList.emplace_back(&bb);
        };

        enqueue(cfg.exit());

        जाव (!workList.empty()) {
            ध्रुव CfgBlock* ध्रुव currp = workList.front();
            workList.निष्कासय_front();
            BlockState& state = m_blockState[*currp];
            state.m_isOnWorkList = मिच्छ;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](ध्रुव CfgBlock& bb) {
                auto& liveIn = m_blockState[bb].m_liveIn;
                state.m_liveOut.निवेशय(liveIn.begin(), liveIn.end());
            });

            // Apply transfer function of block
            ध्रुव तार्किक changed = transfer(*currp);
            // Enqueue predecessors
            जइ (changed || !state.m_wasProcessed) currp->forEachPredecessor(enqueue);
            // Mark as done with first visit
            state.m_wasProcessed = सच्च;
        }
    }

सार्वजनिक:
    स्थिर std::unique_ptr<std::vector<AstVarScope*>> apply(ध्रुव CfgGraph& cfg) {
        CfgLiveVariables analysis{cfg};
        // If failed, return nullptr
        जइ (analysis.m_abort) प्रत्यागम nullptr;
        // Gather variables live in to the entry block
        ध्रुव std::unordered_set<AstVarScope*>& lin = analysis.m_blockState[cfg.enter()].m_liveIn;
        std::vector<AstVarScope*>* ध्रुव resultp
            = नूतन std::vector<AstVarScope*>{lin.begin(), lin.end()};
        // Sort for stability
        std::stable_sort(resultp->begin(), resultp->end(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             प्रत्यागम ap->name() < bp->name();
                         });
        प्रत्यागम std::unique_ptr<std::vector<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::vector<AstVarScope*>> V3Cfg::liveVarScopes(ध्रुव CfgGraph& cfg) {
    प्रत्यागम CfgLiveVariables::apply(cfg);
}
