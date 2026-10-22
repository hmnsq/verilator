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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Cfg.h"

#شامل <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

جماعت Cfg_زندہ_Variables final : VNVisitorConst {
    // State associted with each basic block
    ساخت بلاک_State final {
        // Variables used in block, before a complete assignment in the same block
        std::unordered_سیٹ<AstVarScope*> m_gen;
        // Variables that are assigned a complete value in the basic block
        std::unordered_سیٹ<AstVarScope*> m_kill;
        std::unordered_سیٹ<AstVarScope*> m_زندہ_اندر;  // Variables live on entry to the block
        std::unordered_سیٹ<AstVarScope*> m_زندہ_برآمد;  // Variables live on exit from the block
        منطقی m_isOnWorkList = جھوٹ;  // Block is on work list
        منطقی m_wasProcessed = جھوٹ;  // Already processed at least once
    };

    // STATE
    ثابت Cfg_گراف& m_cfg;  // The CFG beign analysed
    // State for each block
    Cfg_بلاک_Map<بلاک_State> m_بلاک_State{m_cfg.make_بلاک_Map<بلاک_State>()};
    بلاک_State* m_currp = خالی_اشاریہ;  // State of current block being analysed
    منطقی m_abort = جھوٹ;  // Abort analysis - unhandled construct

    // METHODS

    ساکن منطقی isPacked(ثابت AstVarScope* vscp) {
        Ast_گرہ_DType* ثابت dtypep = vscp->dtypep()->skipRefp();
        اگر (ثابت AstBasicDType* ثابت typep = VN_CAST(dtypep, BasicDType)) {
            واپس typep->keyword().isIntNumeric();
        }
        اگر (VN_IS(dtypep, PackArrayDType)) واپس سچ;
        اگر (ثابت Ast_گرہ_UOr_Struct_DType* ثابت typep = VN_CAST(dtypep, گرہ_UOr_Struct_DType)) {
            واپس typep->packed();
        }
        واپس جھوٹ;
    }

    خالی updateGen(Ast_گرہ* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ثابت AstVarRef* refp) {
            // Only care about reads
            اگر (refp->access().isWriteOnly()) واپس جھوٹ;
            // Grab referenced variable
            AstVarScope* ثابت vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            اگر (m_currp->m_kill.گنتی(vscp)) واپس جھوٹ;
            m_currp->m_gen.emplace(vscp);
            واپس جھوٹ;
        });
    }

    خالی updateKill(Ast_گرہ* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ثابت AstVarRef* refp) {
            // Only care about writes
            اگر (refp->access().isReadOnly()) واپس جھوٹ;
            // Grab referenced variable
            AstVarScope* ثابت vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            اگر (vscp->varp()->ignoreSchedWrite()) واپس سچ;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            اگر (!isPacked(vscp)) واپس جھوٹ;
            // If packed, and updated partially, don't add to kill set
            اگر (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) واپس جھوٹ;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            واپس جھوٹ;
        });
    }

    خالی single(Ast_گرہ* nodep) {
        // Assume all reads hapen before any writes
        اگر (m_abort) واپس;
        updateGen(nodep);
        اگر (m_abort) واپس;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    منطقی transfer(ثابت Cfg_بلاک& bb) {
        بلاک_State& state = m_بلاک_State[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_سیٹ<AstVarScope*> زندہ_اندر = state.m_gen;
        برائے (AstVarScope* ثابت varp : state.m_زندہ_برآمد) {
            اگر (state.m_kill.گنتی(varp)) جاری;
            زندہ_اندر.داخل_کرو(varp);
        }
        اگر (زندہ_اندر == state.m_زندہ_اندر) واپس جھوٹ;
        std::تبادلہ(زندہ_اندر, state.m_زندہ_اندر);
        واپس سچ;
    }

    //دورہ
    خالی دورہ(Ast_گرہ* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = سچ;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    خالی دورہ(Ast_تفویض* nodep) override { single(nodep); }
    خالی دورہ(Ast_تفویض_W* nodep) override { single(nodep); }
    خالی دورہ(AstDisplay* nodep) override { single(nodep); }
    خالی دورہ(Ast_اختتام* nodep) override { single(nodep); }
    خالی دورہ(Ast_اختتام_Fork* nodep) override { single(nodep); }
    خالی دورہ(Ast_Stmt_اظہاریہ* nodep) override { single(nodep); }
    خالی دورہ(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    خالی دورہ(AstIf* nodep) override { single(nodep->condp()); }
    خالی دورہ(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    صریح Cfg_زندہ_Variables(ثابت Cfg_گراف& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        برائے (ثابت V3_گراف_راس& vtx : cfg.vertices()) {
            ثابت Cfg_بلاک& bb = static_cast<ثابت Cfg_بلاک&>(vtx);
            اگر (m_abort) واپس;
            VL_RESTORER(m_currp);
            m_currp = &m_بلاک_State[bb];
            برائے (Ast_گرہ* ثابت stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        اگر (m_abort) واپس;

        // Perform the flow analysis
        std::deque<ثابت Cfg_بلاک*> workList;
        ثابت خودکار قطار_میں_ڈالو = [&](ثابت Cfg_بلاک& bb) {
            بلاک_State& state = m_بلاک_State[bb];
            اگر (state.m_isOnWorkList) واپس;
            state.m_isOnWorkList = سچ;
            workList.emplace_عقب(&bb);
        };

        قطار_میں_ڈالو(cfg.باہر_نکلو());

        جب (!workList.خالی_خانے()) {
            ثابت Cfg_بلاک* ثابت currp = workList.front();
            workList.نکالو_front();
            بلاک_State& state = m_بلاک_State[*currp];
            state.m_isOnWorkList = جھوٹ;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](ثابت Cfg_بلاک& bb) {
                خودکار& زندہ_اندر = m_بلاک_State[bb].m_زندہ_اندر;
                state.m_زندہ_برآمد.داخل_کرو(زندہ_اندر.begin(), زندہ_اندر.انتہا());
            });

            // Apply transfer function of block
            ثابت منطقی بدلا = transfer(*currp);
            // Enqueue predecessors
            اگر (بدلا || !state.m_wasProcessed) currp->forEachPredecessor(قطار_میں_ڈالو);
            // Mark as done with first visit
            state.m_wasProcessed = سچ;
        }
    }

عوامی:
    ساکن std::unique_ptr<std::سمتیہ<AstVarScope*>> apply(ثابت Cfg_گراف& cfg) {
        Cfg_زندہ_Variables analysis{cfg};
        // If failed, return nullptr
        اگر (analysis.m_abort) واپس خالی_اشاریہ;
        // Gather variables live in to the entry block
        ثابت std::unordered_سیٹ<AstVarScope*>& lin = analysis.m_بلاک_State[cfg.enter()].m_زندہ_اندر;
        std::سمتیہ<AstVarScope*>* ثابت resultp
            = نیا std::سمتیہ<AstVarScope*>{lin.begin(), lin.انتہا()};
        // Sort for stability
        std::stable_ترتیب_دو(resultp->begin(), resultp->انتہا(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             واپس ap->name() < bp->name();
                         });
        واپس std::unique_ptr<std::سمتیہ<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::سمتیہ<AstVarScope*>> V3Cfg::زندہ_Var_Scopes(ثابت Cfg_گراف& cfg) {
    واپس Cfg_زندہ_Variables::apply(cfg);
}
