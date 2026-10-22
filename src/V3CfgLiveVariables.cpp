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
        तार्किक m_isOnWorkList = असत्य;  // Block is on work list
        तार्किक m_wasProcessed = असत्य;  // Already processed at least once
    };

    // STATE
    स्थिरांक CfgGraph& m_cfg;  // The CFG beign analysed
    // State for each block
    CfgBlockMap<BlockState> m_blockState{m_cfg.makeBlockMap<BlockState>()};
    BlockState* m_currp = nullptr;  // State of current block being analysed
    तार्किक m_abort = असत्य;  // Abort analysis - unhandled construct

    // METHODS

    स्थिर तार्किक isPacked(स्थिरांक AstVarScope* vscp) {
        Ast_गाठ_DType* स्थिरांक dtypep = vscp->dtypep()->skipRefp();
        जर (स्थिरांक AstBasicDType* स्थिरांक typep = VN_CAST(dtypep, BasicDType)) {
            परत typep->keyword().isIntNumeric();
        }
        जर (VN_IS(dtypep, PackArrayDType)) परत सत्य;
        जर (स्थिरांक Ast_गाठ_UOr_Struct_DType* स्थिरांक typep = VN_CAST(dtypep, गाठ_UOr_Struct_DType)) {
            परत typep->packed();
        }
        परत असत्य;
    }

    शून्य updateGen(Ast_गाठ* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](स्थिरांक AstVarRef* refp) {
            // Only care about reads
            जर (refp->access().isWriteOnly()) परत असत्य;
            // Grab referenced variable
            AstVarScope* स्थिरांक vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            जर (m_currp->m_kill.मोजणी(vscp)) परत असत्य;
            m_currp->m_gen.emplace(vscp);
            परत असत्य;
        });
    }

    शून्य updateKill(Ast_गाठ* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](स्थिरांक AstVarRef* refp) {
            // Only care about writes
            जर (refp->access().isReadOnly()) परत असत्य;
            // Grab referenced variable
            AstVarScope* स्थिरांक vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            जर (vscp->varp()->ignoreSchedWrite()) परत सत्य;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            जर (!isPacked(vscp)) परत असत्य;
            // If packed, and updated partially, don't add to kill set
            जर (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) परत असत्य;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            परत असत्य;
        });
    }

    शून्य single(Ast_गाठ* nodep) {
        // Assume all reads hapen before any writes
        जर (m_abort) परत;
        updateGen(nodep);
        जर (m_abort) परत;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    तार्किक transfer(स्थिरांक CfgBlock& bb) {
        BlockState& state = m_blockState[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_set<AstVarScope*> liveIn = state.m_gen;
        साठी (AstVarScope* स्थिरांक varp : state.m_liveOut) {
            जर (state.m_kill.मोजणी(varp)) सुरूठेवा;
            liveIn.घाला(varp);
        }
        जर (liveIn == state.m_liveIn) परत असत्य;
        std::swap(liveIn, state.m_liveIn);
        परत सत्य;
    }

    // VISIT
    शून्य visit(Ast_गाठ* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = सत्य;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    शून्य visit(AstAssign* nodep) override { single(nodep); }
    शून्य visit(AstAssignW* nodep) override { single(nodep); }
    शून्य visit(AstDisplay* nodep) override { single(nodep); }
    शून्य visit(AstFinish* nodep) override { single(nodep); }
    शून्य visit(AstFinishFork* nodep) override { single(nodep); }
    शून्य visit(Ast_Stmt_अभिव्यक्ती* nodep) override { single(nodep); }
    शून्य visit(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    शून्य visit(AstIf* nodep) override { single(nodep->condp()); }
    शून्य visit(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    explicit CfgLiveVariables(स्थिरांक CfgGraph& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        साठी (स्थिरांक V3GraphVertex& vtx : cfg.vertices()) {
            स्थिरांक CfgBlock& bb = static_cast<स्थिरांक CfgBlock&>(vtx);
            जर (m_abort) परत;
            VL_RESTORER(m_currp);
            m_currp = &m_blockState[bb];
            साठी (Ast_गाठ* स्थिरांक stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        जर (m_abort) परत;

        // Perform the flow analysis
        std::deque<स्थिरांक CfgBlock*> workList;
        स्थिरांक स्वतः enqueue = [&](स्थिरांक CfgBlock& bb) {
            BlockState& state = m_blockState[bb];
            जर (state.m_isOnWorkList) परत;
            state.m_isOnWorkList = सत्य;
            workList.emplace_back(&bb);
        };

        enqueue(cfg.निकास());

        पर्यंत (!workList.empty()) {
            स्थिरांक CfgBlock* स्थिरांक currp = workList.front();
            workList.काढा_front();
            BlockState& state = m_blockState[*currp];
            state.m_isOnWorkList = असत्य;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](स्थिरांक CfgBlock& bb) {
                स्वतः& liveIn = m_blockState[bb].m_liveIn;
                state.m_liveOut.घाला(liveIn.begin(), liveIn.end());
            });

            // Apply transfer function of block
            स्थिरांक तार्किक changed = transfer(*currp);
            // Enqueue predecessors
            जर (changed || !state.m_wasProcessed) currp->forEachPredecessor(enqueue);
            // Mark as done with first visit
            state.m_wasProcessed = सत्य;
        }
    }

सार्वजनिक:
    स्थिर std::unique_ptr<std::vector<AstVarScope*>> apply(स्थिरांक CfgGraph& cfg) {
        CfgLiveVariables analysis{cfg};
        // If failed, return nullptr
        जर (analysis.m_abort) परत nullptr;
        // Gather variables live in to the entry block
        स्थिरांक std::unordered_set<AstVarScope*>& lin = analysis.m_blockState[cfg.enter()].m_liveIn;
        std::vector<AstVarScope*>* स्थिरांक resultp
            = नवीन std::vector<AstVarScope*>{lin.begin(), lin.end()};
        // Sort for stability
        std::stable_sort(resultp->begin(), resultp->end(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             परत ap->name() < bp->name();
                         });
        परत std::unique_ptr<std::vector<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::vector<AstVarScope*>> V3Cfg::liveVarScopes(स्थिरांक CfgGraph& cfg) {
    परत CfgLiveVariables::apply(cfg);
}
