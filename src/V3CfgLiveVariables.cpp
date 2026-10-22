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

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3Cfg.h"

#समावेश <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

श्रेणी CfgLiveVariables final : VNVisitorConst {
    // State associted with each basic block
    काष्ठा BlockState final {
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
    स्थिर CfgGraph& m_cfg;  // The CFG beign analysed
    // State for each block
    CfgBlockMap<BlockState> m_blockState{m_cfg.makeBlockMap<BlockState>()};
    BlockState* m_currp = nullptr;  // State of current block being analysed
    bool m_abort = false;  // Abort analysis - unhandled construct

    // METHODS

    अटल bool isPacked(स्थिर AstVarScope* vscp) {
        Ast_गाँठ_DType* स्थिर dtypep = vscp->dtypep()->skipRefp();
        अगर (स्थिर AstBasicDType* स्थिर typep = VN_CAST(dtypep, BasicDType)) {
            वापस typep->keyword().isIntNumeric();
        }
        अगर (VN_IS(dtypep, PackArrayDType)) वापस true;
        अगर (स्थिर Ast_गाँठ_UOr_Struct_DType* स्थिर typep = VN_CAST(dtypep, गाँठ_UOr_Struct_DType)) {
            वापस typep->packed();
        }
        वापस false;
    }

    व्योम updateGen(Ast_गाँठ* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](स्थिर AstVarRef* refp) {
            // Only care about reads
            अगर (refp->access().isWriteOnly()) वापस false;
            // Grab referenced variable
            AstVarScope* स्थिर vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            अगर (m_currp->m_kill.गिनती(vscp)) वापस false;
            m_currp->m_gen.emplace(vscp);
            वापस false;
        });
    }

    व्योम updateKill(Ast_गाँठ* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](स्थिर AstVarRef* refp) {
            // Only care about writes
            अगर (refp->access().isReadOnly()) वापस false;
            // Grab referenced variable
            AstVarScope* स्थिर vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            अगर (vscp->varp()->ignoreSchedWrite()) वापस true;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            अगर (!isPacked(vscp)) वापस false;
            // If packed, and updated partially, don't add to kill set
            अगर (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) वापस false;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            वापस false;
        });
    }

    व्योम single(Ast_गाँठ* nodep) {
        // Assume all reads hapen before any writes
        अगर (m_abort) वापस;
        updateGen(nodep);
        अगर (m_abort) वापस;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    bool transfer(स्थिर CfgBlock& bb) {
        BlockState& state = m_blockState[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_set<AstVarScope*> liveIn = state.m_gen;
        क्रम (AstVarScope* स्थिर varp : state.m_liveOut) {
            अगर (state.m_kill.गिनती(varp)) जारी;
            liveIn.जोड़ो(varp);
        }
        अगर (liveIn == state.m_liveIn) वापस false;
        std::swap(liveIn, state.m_liveIn);
        वापस true;
    }

    // VISIT
    व्योम visit(Ast_गाँठ* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = true;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    व्योम visit(AstAssign* nodep) override { single(nodep); }
    व्योम visit(AstAssignW* nodep) override { single(nodep); }
    व्योम visit(AstDisplay* nodep) override { single(nodep); }
    व्योम visit(AstFinish* nodep) override { single(nodep); }
    व्योम visit(AstFinishFork* nodep) override { single(nodep); }
    व्योम visit(Ast_Stmt_व्यंजक* nodep) override { single(nodep); }
    व्योम visit(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    व्योम visit(AstIf* nodep) override { single(nodep->condp()); }
    व्योम visit(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    explicit CfgLiveVariables(स्थिर CfgGraph& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        क्रम (स्थिर V3GraphVertex& vtx : cfg.vertices()) {
            स्थिर CfgBlock& bb = static_cast<स्थिर CfgBlock&>(vtx);
            अगर (m_abort) वापस;
            VL_RESTORER(m_currp);
            m_currp = &m_blockState[bb];
            क्रम (Ast_गाँठ* स्थिर stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        अगर (m_abort) वापस;

        // Perform the flow analysis
        std::deque<स्थिर CfgBlock*> workList;
        स्थिर स्वतः enqueue = [&](स्थिर CfgBlock& bb) {
            BlockState& state = m_blockState[bb];
            अगर (state.m_isOnWorkList) वापस;
            state.m_isOnWorkList = true;
            workList.emplace_back(&bb);
        };

        enqueue(cfg.निकास());

        जबतक (!workList.empty()) {
            स्थिर CfgBlock* स्थिर currp = workList.front();
            workList.निकालो_front();
            BlockState& state = m_blockState[*currp];
            state.m_isOnWorkList = false;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](स्थिर CfgBlock& bb) {
                स्वतः& liveIn = m_blockState[bb].m_liveIn;
                state.m_liveOut.जोड़ो(liveIn.begin(), liveIn.end());
            });

            // Apply transfer function of block
            स्थिर bool changed = transfer(*currp);
            // Enqueue predecessors
            अगर (changed || !state.m_wasProcessed) currp->forEachPredecessor(enqueue);
            // Mark as done with first visit
            state.m_wasProcessed = true;
        }
    }

खुला:
    अटल std::unique_ptr<std::vector<AstVarScope*>> apply(स्थिर CfgGraph& cfg) {
        CfgLiveVariables analysis{cfg};
        // If failed, return nullptr
        अगर (analysis.m_abort) वापस nullptr;
        // Gather variables live in to the entry block
        स्थिर std::unordered_set<AstVarScope*>& lin = analysis.m_blockState[cfg.enter()].m_liveIn;
        std::vector<AstVarScope*>* स्थिर resultp
            = नया std::vector<AstVarScope*>{lin.begin(), lin.end()};
        // Sort for stability
        std::stable_sort(resultp->begin(), resultp->end(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             वापस ap->name() < bp->name();
                         });
        वापस std::unique_ptr<std::vector<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::vector<AstVarScope*>> V3Cfg::liveVarScopes(स्थिर CfgGraph& cfg) {
    वापस CfgLiveVariables::apply(cfg);
}
