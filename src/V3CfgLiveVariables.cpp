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

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Cfg.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

ਵਰਗ Cfg_ਜੀਵਤ_Variables final : VNVisitorConst {
    // State associted with each basic block
    ਬਣਤਰ ਖੰਡ_State final {
        // Variables used in block, before a complete assignment in the same block
        std::unordered_ਸੈੱਟ<AstVarScope*> m_gen;
        // Variables that are assigned a complete value in the basic block
        std::unordered_ਸੈੱਟ<AstVarScope*> m_kill;
        std::unordered_ਸੈੱਟ<AstVarScope*> m_ਜੀਵਤ_ਵਿੱਚ;  // Variables live on entry to the block
        std::unordered_ਸੈੱਟ<AstVarScope*> m_ਜੀਵਤ_ਬਾਹਰਲਾ;  // Variables live on exit from the block
        ਤਾਰਕਿਕ m_isOnWorkList = ਝੂਠ;  // Block is on work list
        ਤਾਰਕਿਕ m_wasProcessed = ਝੂਠ;  // Already processed at least once
    };

    // STATE
    ਅਚਲ Cfg_ਗ੍ਰਾਫ& m_cfg;  // The CFG beign analysed
    // State for each block
    Cfg_ਖੰਡ_Map<ਖੰਡ_State> m_ਖੰਡ_State{m_cfg.make_ਖੰਡ_Map<ਖੰਡ_State>()};
    ਖੰਡ_State* m_currp = ਖਾਲੀ_ਸੂਚਕ;  // State of current block being analysed
    ਤਾਰਕਿਕ m_abort = ਝੂਠ;  // Abort analysis - unhandled construct

    // METHODS

    ਸਥਿਰ ਤਾਰਕਿਕ isPacked(ਅਚਲ AstVarScope* vscp) {
        Ast_ਗੰਢ_DType* ਅਚਲ dtypep = vscp->dtypep()->skipRefp();
        ਜੇ (ਅਚਲ AstBasicDType* ਅਚਲ typep = VN_CAST(dtypep, BasicDType)) {
            ਵਾਪਸ typep->keyword().isIntNumeric();
        }
        ਜੇ (VN_IS(dtypep, PackArrayDType)) ਵਾਪਸ ਸੱਚ;
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ typep = VN_CAST(dtypep, ਗੰਢ_UOr_Struct_DType)) {
            ਵਾਪਸ typep->packed();
        }
        ਵਾਪਸ ਝੂਠ;
    }

    ਖਾਲੀ updateGen(Ast_ਗੰਢ* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ਅਚਲ AstVarRef* refp) {
            // Only care about reads
            ਜੇ (refp->access().isWriteOnly()) ਵਾਪਸ ਝੂਠ;
            // Grab referenced variable
            AstVarScope* ਅਚਲ vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            ਜੇ (m_currp->m_kill.ਗਿਣਤੀ_2(vscp)) ਵਾਪਸ ਝੂਠ;
            m_currp->m_gen.emplace(vscp);
            ਵਾਪਸ ਝੂਠ;
        });
    }

    ਖਾਲੀ updateKill(Ast_ਗੰਢ* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ਅਚਲ AstVarRef* refp) {
            // Only care about writes
            ਜੇ (refp->access().isReadOnly()) ਵਾਪਸ ਝੂਠ;
            // Grab referenced variable
            AstVarScope* ਅਚਲ vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            ਜੇ (vscp->varp()->ignoreSchedWrite()) ਵਾਪਸ ਸੱਚ;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            ਜੇ (!isPacked(vscp)) ਵਾਪਸ ਝੂਠ;
            // If packed, and updated partially, don't add to kill set
            ਜੇ (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) ਵਾਪਸ ਝੂਠ;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            ਵਾਪਸ ਝੂਠ;
        });
    }

    ਖਾਲੀ single(Ast_ਗੰਢ* nodep) {
        // Assume all reads hapen before any writes
        ਜੇ (m_abort) ਵਾਪਸ;
        updateGen(nodep);
        ਜੇ (m_abort) ਵਾਪਸ;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    ਤਾਰਕਿਕ transfer(ਅਚਲ Cfg_ਖੰਡ& bb) {
        ਖੰਡ_State& state = m_ਖੰਡ_State[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_ਸੈੱਟ<AstVarScope*> ਜੀਵਤ_ਵਿੱਚ = state.m_gen;
        ਲਈ (AstVarScope* ਅਚਲ varp : state.m_ਜੀਵਤ_ਬਾਹਰਲਾ) {
            ਜੇ (state.m_kill.ਗਿਣਤੀ_2(varp)) ਜਾਰੀ;
            ਜੀਵਤ_ਵਿੱਚ.ਪਾਓ(varp);
        }
        ਜੇ (ਜੀਵਤ_ਵਿੱਚ == state.m_ਜੀਵਤ_ਵਿੱਚ) ਵਾਪਸ ਝੂਠ;
        std::ਅਦਲਾ_ਬਦਲੀ(ਜੀਵਤ_ਵਿੱਚ, state.m_ਜੀਵਤ_ਵਿੱਚ);
        ਵਾਪਸ ਸੱਚ;
    }

    //ਦੇਖੋ
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = ਸੱਚ;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ* nodep) override { single(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_W* nodep) override { single(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstDisplay* nodep) override { single(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਅੰਤ* nodep) override { single(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਅੰਤ_Fork* nodep) override { single(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_Stmt_ਵਿਅੰਜਕ* nodep) override { single(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    ਖਾਲੀ ਦੇਖੋ(AstIf* nodep) override { single(nodep->condp()); }
    ਖਾਲੀ ਦੇਖੋ(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    ਸਪਸ਼ਟ Cfg_ਜੀਵਤ_Variables(ਅਚਲ Cfg_ਗ੍ਰਾਫ& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : cfg.vertices()) {
            ਅਚਲ Cfg_ਖੰਡ& bb = static_cast<ਅਚਲ Cfg_ਖੰਡ&>(vtx);
            ਜੇ (m_abort) ਵਾਪਸ;
            VL_RESTORER(m_currp);
            m_currp = &m_ਖੰਡ_State[bb];
            ਲਈ (Ast_ਗੰਢ* ਅਚਲ stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        ਜੇ (m_abort) ਵਾਪਸ;

        // Perform the flow analysis
        std::deque<ਅਚਲ Cfg_ਖੰਡ*> workList;
        ਅਚਲ ਸਵੈਚਾਲਿਤ ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ = [&](ਅਚਲ Cfg_ਖੰਡ& bb) {
            ਖੰਡ_State& state = m_ਖੰਡ_State[bb];
            ਜੇ (state.m_isOnWorkList) ਵਾਪਸ;
            state.m_isOnWorkList = ਸੱਚ;
            workList.emplace_ਪਿੱਠ(&bb);
        };

        ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(cfg.ਬਾਹਰ_ਨਿਕਲੋ());

        ਜਦੋਂ (!workList.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਅਚਲ Cfg_ਖੰਡ* ਅਚਲ currp = workList.front();
            workList.ਕੱਢੋ_front();
            ਖੰਡ_State& state = m_ਖੰਡ_State[*currp];
            state.m_isOnWorkList = ਝੂਠ;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](ਅਚਲ Cfg_ਖੰਡ& bb) {
                ਸਵੈਚਾਲਿਤ& ਜੀਵਤ_ਵਿੱਚ = m_ਖੰਡ_State[bb].m_ਜੀਵਤ_ਵਿੱਚ;
                state.m_ਜੀਵਤ_ਬਾਹਰਲਾ.ਪਾਓ(ਜੀਵਤ_ਵਿੱਚ.begin(), ਜੀਵਤ_ਵਿੱਚ.ਸਿਰਾ());
            });

            // Apply transfer function of block
            ਅਚਲ ਤਾਰਕਿਕ ਬਦਲਿਆ = transfer(*currp);
            // Enqueue predecessors
            ਜੇ (ਬਦਲਿਆ || !state.m_wasProcessed) currp->forEachPredecessor(ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ);
            // Mark as done with first visit
            state.m_wasProcessed = ਸੱਚ;
        }
    }

ਜਨਤਕ:
    ਸਥਿਰ std::unique_ptr<std::ਸਦਿਸ਼<AstVarScope*>> apply(ਅਚਲ Cfg_ਗ੍ਰਾਫ& cfg) {
        Cfg_ਜੀਵਤ_Variables analysis{cfg};
        // If failed, return nullptr
        ਜੇ (analysis.m_abort) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        // Gather variables live in to the entry block
        ਅਚਲ std::unordered_ਸੈੱਟ<AstVarScope*>& lin = analysis.m_ਖੰਡ_State[cfg.enter()].m_ਜੀਵਤ_ਵਿੱਚ;
        std::ਸਦਿਸ਼<AstVarScope*>* ਅਚਲ resultp
            = ਨਵਾਂ std::ਸਦਿਸ਼<AstVarScope*>{lin.begin(), lin.ਸਿਰਾ()};
        // Sort for stability
        std::stable_ਛਾਂਟੋ(resultp->begin(), resultp->ਸਿਰਾ(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             ਵਾਪਸ ap->name() < bp->name();
                         });
        ਵਾਪਸ std::unique_ptr<std::ਸਦਿਸ਼<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::ਸਦਿਸ਼<AstVarScope*>> V3Cfg::ਜੀਵਤ_Var_Scopes(ਅਚਲ Cfg_ਗ੍ਰਾਫ& cfg) {
    ਵਾਪਸ Cfg_ਜੀਵਤ_Variables::apply(cfg);
}
