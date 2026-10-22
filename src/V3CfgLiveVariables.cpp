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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Cfg.h"

#ಅನ್ತರ್ಭಾವ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

ವರ್ಗ Cfg_ಜೀವಂತ_Variables final : VNVisitorConst {
    // State associted with each basic block
    ರಚನೆ ಖಂಡ_State final {
        // Variables used in block, before a complete assignment in the same block
        std::unordered_ಗಣ<AstVarScope*> m_gen;
        // Variables that are assigned a complete value in the basic block
        std::unordered_ಗಣ<AstVarScope*> m_kill;
        std::unordered_ಗಣ<AstVarScope*> m_ಜೀವಂತ_ಒಳ;  // Variables live on entry to the block
        std::unordered_ಗಣ<AstVarScope*> m_ಜೀವಂತ_ಹೊರಹೋಗು;  // Variables live on exit from the block
        ತಾರ್ಕಿಕ m_isOnWorkList = ಸುಳ್ಳು;  // Block is on work list
        ತಾರ್ಕಿಕ m_wasProcessed = ಸುಳ್ಳು;  // Already processed at least once
    };

    // STATE
    ಸ್ಥಿರಾಂಕ Cfg_ನಕ್ಷೆ& m_cfg;  // The CFG beign analysed
    // State for each block
    Cfg_ಖಂಡ_Map<ಖಂಡ_State> m_ಖಂಡ_State{m_cfg.make_ಖಂಡ_Map<ಖಂಡ_State>()};
    ಖಂಡ_State* m_currp = ಶೂನ್ಯಸೂಚಕ;  // State of current block being analysed
    ತಾರ್ಕಿಕ m_abort = ಸುಳ್ಳು;  // Abort analysis - unhandled construct

    // METHODS

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isPacked(ಸ್ಥಿರಾಂಕ AstVarScope* vscp) {
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = vscp->dtypep()->skipRefp();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ typep = VN_CAST(dtypep, BasicDType)) {
            ಹಿಂತಿರುಗಿ typep->keyword().isIntNumeric();
        }
        ಆದರೆ (VN_IS(dtypep, PackArrayDType)) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ typep = VN_CAST(dtypep, ಗಂಟು_UOr_Struct_DType)) {
            ಹಿಂತಿರುಗಿ typep->packed();
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಶೂನ್ಯ updateGen(Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ಸ್ಥಿರಾಂಕ AstVarRef* refp) {
            // Only care about reads
            ಆದರೆ (refp->access().isWriteOnly()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Grab referenced variable
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            ಆದರೆ (m_currp->m_kill.ಎಣಿಕೆ_2(vscp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            m_currp->m_gen.emplace(vscp);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
    }

    ಶೂನ್ಯ updateKill(Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ಸ್ಥಿರಾಂಕ AstVarRef* refp) {
            // Only care about writes
            ಆದರೆ (refp->access().isReadOnly()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Grab referenced variable
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            ಆದರೆ (vscp->varp()->ignoreSchedWrite()) ಹಿಂತಿರುಗಿ ನಿಜ;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            ಆದರೆ (!isPacked(vscp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // If packed, and updated partially, don't add to kill set
            ಆದರೆ (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
    }

    ಶೂನ್ಯ single(Ast_ಗಂಟು* nodep) {
        // Assume all reads hapen before any writes
        ಆದರೆ (m_abort) ಹಿಂತಿರುಗಿ;
        updateGen(nodep);
        ಆದರೆ (m_abort) ಹಿಂತಿರುಗಿ;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    ತಾರ್ಕಿಕ transfer(ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ& bb) {
        ಖಂಡ_State& state = m_ಖಂಡ_State[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_ಗಣ<AstVarScope*> ಜೀವಂತ_ಒಳ = state.m_gen;
        ಗಾಗಿ (AstVarScope* ಸ್ಥಿರಾಂಕ varp : state.m_ಜೀವಂತ_ಹೊರಹೋಗು) {
            ಆದರೆ (state.m_kill.ಎಣಿಕೆ_2(varp)) ಮುಂದುವರಿ;
            ಜೀವಂತ_ಒಳ.ಸೇರಿಸು(varp);
        }
        ಆದರೆ (ಜೀವಂತ_ಒಳ == state.m_ಜೀವಂತ_ಒಳ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        std::ಅದಲುಬದಲು(ಜೀವಂತ_ಒಳ, state.m_ಜೀವಂತ_ಒಳ);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    //ಭೇಟಿ
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = ನಿಜ;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು* nodep) override { single(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_W* nodep) override { single(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstDisplay* nodep) override { single(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಮುಕ್ತಾಯ* nodep) override { single(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಮುಕ್ತಾಯ_Fork* nodep) override { single(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override { single(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    ಶೂನ್ಯ ಭೇಟಿ(AstIf* nodep) override { single(nodep->condp()); }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    ಸ್ಪಷ್ಟ Cfg_ಜೀವಂತ_Variables(ಸ್ಥಿರಾಂಕ Cfg_ನಕ್ಷೆ& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : cfg.vertices()) {
            ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ& bb = static_cast<ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ&>(vtx);
            ಆದರೆ (m_abort) ಹಿಂತಿರುಗಿ;
            VL_RESTORER(m_currp);
            m_currp = &m_ಖಂಡ_State[bb];
            ಗಾಗಿ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        ಆದರೆ (m_abort) ಹಿಂತಿರುಗಿ;

        // Perform the flow analysis
        std::deque<ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ*> workList;
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಸರತಿಗೆ_ಸೇರಿಸು = [&](ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ& bb) {
            ಖಂಡ_State& state = m_ಖಂಡ_State[bb];
            ಆದರೆ (state.m_isOnWorkList) ಹಿಂತಿರುಗಿ;
            state.m_isOnWorkList = ನಿಜ;
            workList.emplace_ಹಿಂಭಾಗ(&bb);
        };

        ಸರತಿಗೆ_ಸೇರಿಸು(cfg.ನಿರ್ಗಮಿಸು());

        ಆಗ (!workList.ಖಾಲಿಗಳು()) {
            ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* ಸ್ಥಿರಾಂಕ currp = workList.front();
            workList.ತೆಗೆ_front();
            ಖಂಡ_State& state = m_ಖಂಡ_State[*currp];
            state.m_isOnWorkList = ಸುಳ್ಳು;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ& bb) {
                ಸ್ವಯಂಚಾಲಿತ& ಜೀವಂತ_ಒಳ = m_ಖಂಡ_State[bb].m_ಜೀವಂತ_ಒಳ;
                state.m_ಜೀವಂತ_ಹೊರಹೋಗು.ಸೇರಿಸು(ಜೀವಂತ_ಒಳ.begin(), ಜೀವಂತ_ಒಳ.ಕೊನೆ());
            });

            // Apply transfer function of block
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಬದಲಾಯಿತು = transfer(*currp);
            // Enqueue predecessors
            ಆದರೆ (ಬದಲಾಯಿತು || !state.m_wasProcessed) currp->forEachPredecessor(ಸರತಿಗೆ_ಸೇರಿಸು);
            // Mark as done with first visit
            state.m_wasProcessed = ನಿಜ;
        }
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::unique_ptr<std::ಸದಿಶ<AstVarScope*>> apply(ಸ್ಥಿರಾಂಕ Cfg_ನಕ್ಷೆ& cfg) {
        Cfg_ಜೀವಂತ_Variables analysis{cfg};
        // If failed, return nullptr
        ಆದರೆ (analysis.m_abort) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        // Gather variables live in to the entry block
        ಸ್ಥಿರಾಂಕ std::unordered_ಗಣ<AstVarScope*>& lin = analysis.m_ಖಂಡ_State[cfg.enter()].m_ಜೀವಂತ_ಒಳ;
        std::ಸದಿಶ<AstVarScope*>* ಸ್ಥಿರಾಂಕ resultp
            = ಹೊಸ std::ಸದಿಶ<AstVarScope*>{lin.begin(), lin.ಕೊನೆ()};
        // Sort for stability
        std::stable_ವಿಂಗಡಿಸು(resultp->begin(), resultp->ಕೊನೆ(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             ಹಿಂತಿರುಗಿ ap->name() < bp->name();
                         });
        ಹಿಂತಿರುಗಿ std::unique_ptr<std::ಸದಿಶ<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::ಸದಿಶ<AstVarScope*>> V3Cfg::ಜೀವಂತ_Var_Scopes(ಸ್ಥಿರಾಂಕ Cfg_ನಕ್ಷೆ& cfg) {
    ಹಿಂತಿರುಗಿ Cfg_ಜೀವಂತ_Variables::apply(cfg);
}
