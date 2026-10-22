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

صنف Cfg_ژوندی_Variables final : VNVisitorConst {
    // State associted with each basic block
    بنية بلاک_State final {
        // Variables used in block, before a complete assignment in the same block
        std::unordered_سېټ<AstVarScope*> m_gen;
        // Variables that are assigned a complete value in the basic block
        std::unordered_سېټ<AstVarScope*> m_kill;
        std::unordered_سېټ<AstVarScope*> m_ژوندی_دننه;  // Variables live on entry to the block
        std::unordered_سېټ<AstVarScope*> m_ژوندی_بهر;  // Variables live on exit from the block
        منطقي m_isOnWorkList = خطأ;  // Block is on work list
        منطقي m_wasProcessed = خطأ;  // Already processed at least once
    };

    // STATE
    ثابت Cfg_ګراف& m_cfg;  // The CFG beign analysed
    // State for each block
    Cfg_بلاک_Map<بلاک_State> m_بلاک_State{m_cfg.make_بلاک_Map<بلاک_State>()};
    بلاک_State* m_currp = تش_اشاره;  // State of current block being analysed
    منطقي m_abort = خطأ;  // Abort analysis - unhandled construct

    // METHODS

    ساكن منطقي isPacked(ثابت AstVarScope* vscp) {
        Ast_غوټه_DType* ثابت dtypep = vscp->dtypep()->skipRefp();
        إذا (ثابت AstBasicDType* ثابت typep = VN_CAST(dtypep, BasicDType)) {
            أرجع typep->keyword().isIntNumeric();
        }
        إذا (VN_IS(dtypep, PackArrayDType)) أرجع ریښتیا;
        إذا (ثابت Ast_غوټه_UOr_Struct_DType* ثابت typep = VN_CAST(dtypep, غوټه_UOr_Struct_DType)) {
            أرجع typep->packed();
        }
        أرجع خطأ;
    }

    فراغ updateGen(Ast_غوټه* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ثابت AstVarRef* refp) {
            // Only care about reads
            إذا (refp->access().isWriteOnly()) أرجع خطأ;
            // Grab referenced variable
            AstVarScope* ثابت vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            إذا (m_currp->m_kill.شمېر(vscp)) أرجع خطأ;
            m_currp->m_gen.emplace(vscp);
            أرجع خطأ;
        });
    }

    فراغ updateKill(Ast_غوټه* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ثابت AstVarRef* refp) {
            // Only care about writes
            إذا (refp->access().isReadOnly()) أرجع خطأ;
            // Grab referenced variable
            AstVarScope* ثابت vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            إذا (vscp->varp()->ignoreSchedWrite()) أرجع ریښتیا;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            إذا (!isPacked(vscp)) أرجع خطأ;
            // If packed, and updated partially, don't add to kill set
            إذا (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) أرجع خطأ;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            أرجع خطأ;
        });
    }

    فراغ single(Ast_غوټه* nodep) {
        // Assume all reads hapen before any writes
        إذا (m_abort) أرجع;
        updateGen(nodep);
        إذا (m_abort) أرجع;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    منطقي transfer(ثابت Cfg_بلاک& bb) {
        بلاک_State& state = m_بلاک_State[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_سېټ<AstVarScope*> ژوندی_دننه = state.m_gen;
        لكل (AstVarScope* ثابت varp : state.m_ژوندی_بهر) {
            إذا (state.m_kill.شمېر(varp)) استمر;
            ژوندی_دننه.ورننباسه(varp);
        }
        إذا (ژوندی_دننه == state.m_ژوندی_دننه) أرجع خطأ;
        std::بدلون(ژوندی_دننه, state.m_ژوندی_دننه);
        أرجع ریښتیا;
    }

    //لیدنه
    فراغ لیدنه(Ast_غوټه* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = ریښتیا;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    فراغ لیدنه(Ast_ټاکل* nodep) override { single(nodep); }
    فراغ لیدنه(Ast_ټاکل_W* nodep) override { single(nodep); }
    فراغ لیدنه(AstDisplay* nodep) override { single(nodep); }
    فراغ لیدنه(Ast_پای* nodep) override { single(nodep); }
    فراغ لیدنه(Ast_پای_Fork* nodep) override { single(nodep); }
    فراغ لیدنه(Ast_Stmt_بیان* nodep) override { single(nodep); }
    فراغ لیدنه(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    فراغ لیدنه(AstIf* nodep) override { single(nodep->condp()); }
    فراغ لیدنه(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    څرګند Cfg_ژوندی_Variables(ثابت Cfg_ګراف& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        لكل (ثابت V3_ګراف_راس& vtx : cfg.vertices()) {
            ثابت Cfg_بلاک& bb = static_cast<ثابت Cfg_بلاک&>(vtx);
            إذا (m_abort) أرجع;
            VL_RESTORER(m_currp);
            m_currp = &m_بلاک_State[bb];
            لكل (Ast_غوټه* ثابت stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        إذا (m_abort) أرجع;

        // Perform the flow analysis
        std::deque<ثابت Cfg_بلاک*> workList;
        ثابت خپلکاری کتار_ته_ورزیاتول = [&](ثابت Cfg_بلاک& bb) {
            بلاک_State& state = m_بلاک_State[bb];
            إذا (state.m_isOnWorkList) أرجع;
            state.m_isOnWorkList = ریښتیا;
            workList.emplace_شا(&bb);
        };

        کتار_ته_ورزیاتول(cfg.وتل());

        طالما (!workList.تشه()) {
            ثابت Cfg_بلاک* ثابت currp = workList.front();
            workList.راوباسه_front();
            بلاک_State& state = m_بلاک_State[*currp];
            state.m_isOnWorkList = خطأ;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](ثابت Cfg_بلاک& bb) {
                خپلکاری& ژوندی_دننه = m_بلاک_State[bb].m_ژوندی_دننه;
                state.m_ژوندی_بهر.ورننباسه(ژوندی_دننه.begin(), ژوندی_دننه.پای_ټکی());
            });

            // Apply transfer function of block
            ثابت منطقي بدل_شوی = transfer(*currp);
            // Enqueue predecessors
            إذا (بدل_شوی || !state.m_wasProcessed) currp->forEachPredecessor(کتار_ته_ورزیاتول);
            // Mark as done with first visit
            state.m_wasProcessed = ریښتیا;
        }
    }

عام:
    ساكن std::unique_ptr<std::ویکټور<AstVarScope*>> apply(ثابت Cfg_ګراف& cfg) {
        Cfg_ژوندی_Variables analysis{cfg};
        // If failed, return nullptr
        إذا (analysis.m_abort) أرجع تش_اشاره;
        // Gather variables live in to the entry block
        ثابت std::unordered_سېټ<AstVarScope*>& lin = analysis.m_بلاک_State[cfg.enter()].m_ژوندی_دننه;
        std::ویکټور<AstVarScope*>* ثابت resultp
            = جديد std::ویکټور<AstVarScope*>{lin.begin(), lin.پای_ټکی()};
        // Sort for stability
        std::stable_ترتیبول(resultp->begin(), resultp->پای_ټکی(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             أرجع ap->name() < bp->name();
                         });
        أرجع std::unique_ptr<std::ویکټور<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::ویکټور<AstVarScope*>> V3Cfg::ژوندی_Var_Scopes(ثابت Cfg_ګراف& cfg) {
    أرجع Cfg_ژوندی_Variables::apply(cfg);
}
