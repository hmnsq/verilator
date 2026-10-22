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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Cfg.h"

#ضمن <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

صنف Cfg_زنده_Variables final : VNVisitorConst {
    // State associted with each basic block
    بنية بلوک_State final {
        // Variables used in block, before a complete assignment in the same block
        std::unordered_مجموعه<AstVarScope*> m_gen;
        // Variables that are assigned a complete value in the basic block
        std::unordered_مجموعه<AstVarScope*> m_kill;
        std::unordered_مجموعه<AstVarScope*> m_زنده_درون;  // Variables live on entry to the block
        std::unordered_مجموعه<AstVarScope*> m_زنده_بیرون;  // Variables live on exit from the block
        منطقي m_isOnWorkList = خطأ;  // Block is on work list
        منطقي m_wasProcessed = خطأ;  // Already processed at least once
    };

    // STATE
    ثابت Cfg_گراف& m_cfg;  // The CFG beign analysed
    // State for each block
    Cfg_بلوک_Map<بلوک_State> m_بلوک_State{m_cfg.make_بلوک_Map<بلوک_State>()};
    بلوک_State* m_currp = اشاره_تهی;  // State of current block being analysed
    منطقي m_abort = خطأ;  // Abort analysis - unhandled construct

    // METHODS

    ساكن منطقي isPacked(ثابت AstVarScope* vscp) {
        Ast_گره_DType* ثابت dtypep = vscp->dtypep()->skipRefp();
        إذا (ثابت AstBasicDType* ثابت typep = VN_CAST(dtypep, BasicDType)) {
            أرجع typep->keyword().isIntNumeric();
        }
        إذا (VN_IS(dtypep, PackArrayDType)) أرجع درست;
        إذا (ثابت Ast_گره_UOr_Struct_DType* ثابت typep = VN_CAST(dtypep, گره_UOr_Struct_DType)) {
            أرجع typep->packed();
        }
        أرجع خطأ;
    }

    فراغ updateGen(Ast_گره* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ثابت AstVarRef* refp) {
            // Only care about reads
            إذا (refp->access().isWriteOnly()) أرجع خطأ;
            // Grab referenced variable
            AstVarScope* ثابت vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            إذا (m_currp->m_kill.شمار(vscp)) أرجع خطأ;
            m_currp->m_gen.emplace(vscp);
            أرجع خطأ;
        });
    }

    فراغ updateKill(Ast_گره* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ثابت AstVarRef* refp) {
            // Only care about writes
            إذا (refp->access().isReadOnly()) أرجع خطأ;
            // Grab referenced variable
            AstVarScope* ثابت vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            إذا (vscp->varp()->ignoreSchedWrite()) أرجع درست;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            إذا (!isPacked(vscp)) أرجع خطأ;
            // If packed, and updated partially, don't add to kill set
            إذا (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) أرجع خطأ;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            أرجع خطأ;
        });
    }

    فراغ single(Ast_گره* nodep) {
        // Assume all reads hapen before any writes
        إذا (m_abort) أرجع;
        updateGen(nodep);
        إذا (m_abort) أرجع;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    منطقي transfer(ثابت Cfg_بلوک& bb) {
        بلوک_State& state = m_بلوک_State[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_مجموعه<AstVarScope*> زنده_درون = state.m_gen;
        لكل (AstVarScope* ثابت varp : state.m_زنده_بیرون) {
            إذا (state.m_kill.شمار(varp)) استمر;
            زنده_درون.درج(varp);
        }
        إذا (زنده_درون == state.m_زنده_درون) أرجع خطأ;
        std::جابجا(زنده_درون, state.m_زنده_درون);
        أرجع درست;
    }

    //بازدید
    فراغ بازدید(Ast_گره* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = درست;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    فراغ بازدید(Ast_تخصیص_بده* nodep) override { single(nodep); }
    فراغ بازدید(Ast_تخصیص_بده_W* nodep) override { single(nodep); }
    فراغ بازدید(AstDisplay* nodep) override { single(nodep); }
    فراغ بازدید(Ast_پایان* nodep) override { single(nodep); }
    فراغ بازدید(Ast_پایان_Fork* nodep) override { single(nodep); }
    فراغ بازدید(Ast_Stmt_عبارت* nodep) override { single(nodep); }
    فراغ بازدید(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    فراغ بازدید(AstIf* nodep) override { single(nodep->condp()); }
    فراغ بازدید(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    صریح Cfg_زنده_Variables(ثابت Cfg_گراف& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        لكل (ثابت V3_گراف_رأس& vtx : cfg.vertices()) {
            ثابت Cfg_بلوک& bb = static_cast<ثابت Cfg_بلوک&>(vtx);
            إذا (m_abort) أرجع;
            VL_RESTORER(m_currp);
            m_currp = &m_بلوک_State[bb];
            لكل (Ast_گره* ثابت stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        إذا (m_abort) أرجع;

        // Perform the flow analysis
        std::deque<ثابت Cfg_بلوک*> workList;
        ثابت خودکار افزودن_به_صف = [&](ثابت Cfg_بلوک& bb) {
            بلوک_State& state = m_بلوک_State[bb];
            إذا (state.m_isOnWorkList) أرجع;
            state.m_isOnWorkList = درست;
            workList.emplace_پشت(&bb);
        };

        افزودن_به_صف(cfg.خروج());

        طالما (!workList.خالی()) {
            ثابت Cfg_بلوک* ثابت currp = workList.front();
            workList.برداشت_front();
            بلوک_State& state = m_بلوک_State[*currp];
            state.m_isOnWorkList = خطأ;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](ثابت Cfg_بلوک& bb) {
                خودکار& زنده_درون = m_بلوک_State[bb].m_زنده_درون;
                state.m_زنده_بیرون.درج(زنده_درون.begin(), زنده_درون.انتها());
            });

            // Apply transfer function of block
            ثابت منطقي تغییریافته = transfer(*currp);
            // Enqueue predecessors
            إذا (تغییریافته || !state.m_wasProcessed) currp->forEachPredecessor(افزودن_به_صف);
            // Mark as done with first visit
            state.m_wasProcessed = درست;
        }
    }

عام:
    ساكن std::unique_ptr<std::وکتور<AstVarScope*>> apply(ثابت Cfg_گراف& cfg) {
        Cfg_زنده_Variables analysis{cfg};
        // If failed, return nullptr
        إذا (analysis.m_abort) أرجع اشاره_تهی;
        // Gather variables live in to the entry block
        ثابت std::unordered_مجموعه<AstVarScope*>& lin = analysis.m_بلوک_State[cfg.enter()].m_زنده_درون;
        std::وکتور<AstVarScope*>* ثابت resultp
            = جديد std::وکتور<AstVarScope*>{lin.begin(), lin.انتها()};
        // Sort for stability
        std::stable_مرتب_سازی(resultp->begin(), resultp->انتها(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             أرجع ap->name() < bp->name();
                         });
        أرجع std::unique_ptr<std::وکتور<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::وکتور<AstVarScope*>> V3Cfg::زنده_Var_Scopes(ثابت Cfg_گراف& cfg) {
    أرجع Cfg_زنده_Variables::apply(cfg);
}
