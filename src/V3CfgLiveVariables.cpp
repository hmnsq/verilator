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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Cfg.h"

#অন্তর্ভাব <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

বর্গ CfgLiveVariables final : VNVisitorConst {
    // State associted with each basic block
    সংরচনা BlockState final {
        // Variables used in block, before a complete assignment in the same block
        std::unordered_set<AstVarScope*> m_gen;
        // Variables that are assigned a complete value in the basic block
        std::unordered_set<AstVarScope*> m_kill;
        std::unordered_set<AstVarScope*> m_liveIn;  // Variables live on entry to the block
        std::unordered_set<AstVarScope*> m_liveOut;  // Variables live on exit from the block
        তার্কিক m_isOnWorkList = অসত্য;  // Block is on work list
        তার্কিক m_wasProcessed = অসত্য;  // Already processed at least once
    };

    // STATE
    ধ্রুব CfgGraph& m_cfg;  // The CFG beign analysed
    // State for each block
    CfgBlockMap<BlockState> m_blockState{m_cfg.makeBlockMap<BlockState>()};
    BlockState* m_currp = nullptr;  // State of current block being analysed
    তার্কিক m_abort = অসত্য;  // Abort analysis - unhandled construct

    // METHODS

    স্থির তার্কিক isPacked(ধ্রুব AstVarScope* vscp) {
        Ast_গ্রন্থি_DType* ধ্রুব dtypep = vscp->dtypep()->skipRefp();
        যদি (ধ্রুব AstBasicDType* ধ্রুব typep = VN_CAST(dtypep, BasicDType)) {
            প্রত্যাগম typep->keyword().isIntNumeric();
        }
        যদি (VN_IS(dtypep, PackArrayDType)) প্রত্যাগম সত্য;
        যদি (ধ্রুব Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুব typep = VN_CAST(dtypep, গ্রন্থি_UOr_Struct_DType)) {
            প্রত্যাগম typep->packed();
        }
        প্রত্যাগম অসত্য;
    }

    শূন্য updateGen(Ast_গ্রন্থি* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ধ্রুব AstVarRef* refp) {
            // Only care about reads
            যদি (refp->access().isWriteOnly()) প্রত্যাগম অসত্য;
            // Grab referenced variable
            AstVarScope* ধ্রুব vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            যদি (m_currp->m_kill.গুনতি(vscp)) প্রত্যাগম অসত্য;
            m_currp->m_gen.emplace(vscp);
            প্রত্যাগম অসত্য;
        });
    }

    শূন্য updateKill(Ast_গ্রন্থি* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ধ্রুব AstVarRef* refp) {
            // Only care about writes
            যদি (refp->access().isReadOnly()) প্রত্যাগম অসত্য;
            // Grab referenced variable
            AstVarScope* ধ্রুব vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            যদি (vscp->varp()->ignoreSchedWrite()) প্রত্যাগম সত্য;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            যদি (!isPacked(vscp)) প্রত্যাগম অসত্য;
            // If packed, and updated partially, don't add to kill set
            যদি (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) প্রত্যাগম অসত্য;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            প্রত্যাগম অসত্য;
        });
    }

    শূন্য single(Ast_গ্রন্থি* nodep) {
        // Assume all reads hapen before any writes
        যদি (m_abort) প্রত্যাগম;
        updateGen(nodep);
        যদি (m_abort) প্রত্যাগম;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    তার্কিক transfer(ধ্রুব CfgBlock& bb) {
        BlockState& state = m_blockState[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_set<AstVarScope*> liveIn = state.m_gen;
        প্রতি (AstVarScope* ধ্রুব varp : state.m_liveOut) {
            যদি (state.m_kill.গুনতি(varp)) অনুবর্ত;
            liveIn.ঢোকাও(varp);
        }
        যদি (liveIn == state.m_liveIn) প্রত্যাগম অসত্য;
        std::swap(liveIn, state.m_liveIn);
        প্রত্যাগম সত্য;
    }

    // VISIT
    শূন্য visit(Ast_গ্রন্থি* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = সত্য;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    শূন্য visit(AstAssign* nodep) override { single(nodep); }
    শূন্য visit(AstAssignW* nodep) override { single(nodep); }
    শূন্য visit(AstDisplay* nodep) override { single(nodep); }
    শূন্য visit(AstFinish* nodep) override { single(nodep); }
    শূন্য visit(AstFinishFork* nodep) override { single(nodep); }
    শূন্য visit(Ast_Stmt_রাশিমালা* nodep) override { single(nodep); }
    শূন্য visit(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    শূন্য visit(AstIf* nodep) override { single(nodep->condp()); }
    শূন্য visit(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    explicit CfgLiveVariables(ধ্রুব CfgGraph& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        প্রতি (ধ্রুব V3GraphVertex& vtx : cfg.vertices()) {
            ধ্রুব CfgBlock& bb = static_cast<ধ্রুব CfgBlock&>(vtx);
            যদি (m_abort) প্রত্যাগম;
            VL_RESTORER(m_currp);
            m_currp = &m_blockState[bb];
            প্রতি (Ast_গ্রন্থি* ধ্রুব stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        যদি (m_abort) প্রত্যাগম;

        // Perform the flow analysis
        std::deque<ধ্রুব CfgBlock*> workList;
        ধ্রুব auto enqueue = [&](ধ্রুব CfgBlock& bb) {
            BlockState& state = m_blockState[bb];
            যদি (state.m_isOnWorkList) প্রত্যাগম;
            state.m_isOnWorkList = সত্য;
            workList.emplace_back(&bb);
        };

        enqueue(cfg.exit());

        যাবত্ (!workList.empty()) {
            ধ্রুব CfgBlock* ধ্রুব currp = workList.front();
            workList.বের_করো_front();
            BlockState& state = m_blockState[*currp];
            state.m_isOnWorkList = অসত্য;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](ধ্রুব CfgBlock& bb) {
                auto& liveIn = m_blockState[bb].m_liveIn;
                state.m_liveOut.ঢোকাও(liveIn.begin(), liveIn.end());
            });

            // Apply transfer function of block
            ধ্রুব তার্কিক changed = transfer(*currp);
            // Enqueue predecessors
            যদি (changed || !state.m_wasProcessed) currp->forEachPredecessor(enqueue);
            // Mark as done with first visit
            state.m_wasProcessed = সত্য;
        }
    }

সার্বজনিক:
    স্থির std::unique_ptr<std::vector<AstVarScope*>> apply(ধ্রুব CfgGraph& cfg) {
        CfgLiveVariables analysis{cfg};
        // If failed, return nullptr
        যদি (analysis.m_abort) প্রত্যাগম nullptr;
        // Gather variables live in to the entry block
        ধ্রুব std::unordered_set<AstVarScope*>& lin = analysis.m_blockState[cfg.enter()].m_liveIn;
        std::vector<AstVarScope*>* ধ্রুব resultp
            = নূতন std::vector<AstVarScope*>{lin.begin(), lin.end()};
        // Sort for stability
        std::stable_sort(resultp->begin(), resultp->end(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             প্রত্যাগম ap->name() < bp->name();
                         });
        প্রত্যাগম std::unique_ptr<std::vector<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::vector<AstVarScope*>> V3Cfg::liveVarScopes(ধ্রুব CfgGraph& cfg) {
    প্রত্যাগম CfgLiveVariables::apply(cfg);
}
