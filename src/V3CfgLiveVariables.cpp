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

صنف Cfg_جیوندا_Variables final : VNVisitorConst {
    // State associted with each basic block
    بنية بلاک_State final {
        // Variables used in block, before a complete assignment in the same block
        std::unordered_سیٹ<AstVarScope*> m_gen;
        // Variables that are assigned a complete value in the basic block
        std::unordered_سیٹ<AstVarScope*> m_kill;
        std::unordered_سیٹ<AstVarScope*> m_جیوندا_وچ;  // Variables live on entry to the block
        std::unordered_سیٹ<AstVarScope*> m_جیوندا_باہرلا;  // Variables live on exit from the block
        منطقي m_isOnWorkList = خطأ;  // Block is on work list
        منطقي m_wasProcessed = خطأ;  // Already processed at least once
    };

    // STATE
    ثابت Cfg_گراف& m_cfg;  // The CFG beign analysed
    // State for each block
    Cfg_بلاک_Map<بلاک_State> m_بلاک_State{m_cfg.make_بلاک_Map<بلاک_State>()};
    بلاک_State* m_currp = خالی_سوچک;  // State of current block being analysed
    منطقي m_abort = خطأ;  // Abort analysis - unhandled construct

    // METHODS

    ساكن منطقي isPacked(ثابت AstVarScope* vscp) {
        Ast_گرہ_DType* ثابت dtypep = vscp->dtypep()->skipRefp();
        إذا (ثابت AstBasicDType* ثابت typep = VN_CAST(dtypep, BasicDType)) {
            أرجع typep->keyword().isIntNumeric();
        }
        إذا (VN_IS(dtypep, PackArrayDType)) أرجع سچ;
        إذا (ثابت Ast_گرہ_UOr_Struct_DType* ثابت typep = VN_CAST(dtypep, گرہ_UOr_Struct_DType)) {
            أرجع typep->packed();
        }
        أرجع خطأ;
    }

    فراغ updateGen(Ast_گرہ* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ثابت AstVarRef* refp) {
            // Only care about reads
            إذا (refp->access().isWriteOnly()) أرجع خطأ;
            // Grab referenced variable
            AstVarScope* ثابت vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            إذا (m_currp->m_kill.گنتی(vscp)) أرجع خطأ;
            m_currp->m_gen.emplace(vscp);
            أرجع خطأ;
        });
    }

    فراغ updateKill(Ast_گرہ* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ثابت AstVarRef* refp) {
            // Only care about writes
            إذا (refp->access().isReadOnly()) أرجع خطأ;
            // Grab referenced variable
            AstVarScope* ثابت vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            إذا (vscp->varp()->ignoreSchedWrite()) أرجع سچ;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            إذا (!isPacked(vscp)) أرجع خطأ;
            // If packed, and updated partially, don't add to kill set
            إذا (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) أرجع خطأ;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            أرجع خطأ;
        });
    }

    فراغ single(Ast_گرہ* nodep) {
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
        std::unordered_سیٹ<AstVarScope*> جیوندا_وچ = state.m_gen;
        لكل (AstVarScope* ثابت varp : state.m_جیوندا_باہرلا) {
            إذا (state.m_kill.گنتی(varp)) استمر;
            جیوندا_وچ.داخل_کرو(varp);
        }
        إذا (جیوندا_وچ == state.m_جیوندا_وچ) أرجع خطأ;
        std::وٹاندرا(جیوندا_وچ, state.m_جیوندا_وچ);
        أرجع سچ;
    }

    //ویکھو
    فراغ ویکھو(Ast_گرہ* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = سچ;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    فراغ ویکھو(Ast_سونپو* nodep) override { single(nodep); }
    فراغ ویکھو(Ast_سونپو_W* nodep) override { single(nodep); }
    فراغ ویکھو(AstDisplay* nodep) override { single(nodep); }
    فراغ ویکھو(Ast_انت* nodep) override { single(nodep); }
    فراغ ویکھو(Ast_انت_Fork* nodep) override { single(nodep); }
    فراغ ویکھو(Ast_Stmt_اظہاریہ* nodep) override { single(nodep); }
    فراغ ویکھو(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    فراغ ویکھو(AstIf* nodep) override { single(nodep->condp()); }
    فراغ ویکھو(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    واضح Cfg_جیوندا_Variables(ثابت Cfg_گراف& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        لكل (ثابت V3_گراف_راس& vtx : cfg.vertices()) {
            ثابت Cfg_بلاک& bb = static_cast<ثابت Cfg_بلاک&>(vtx);
            إذا (m_abort) أرجع;
            VL_RESTORER(m_currp);
            m_currp = &m_بلاک_State[bb];
            لكل (Ast_گرہ* ثابت stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        إذا (m_abort) أرجع;

        // Perform the flow analysis
        std::deque<ثابت Cfg_بلاک*> workList;
        ثابت خودکار قطار_وچ_پاؤ = [&](ثابت Cfg_بلاک& bb) {
            بلاک_State& state = m_بلاک_State[bb];
            إذا (state.m_isOnWorkList) أرجع;
            state.m_isOnWorkList = سچ;
            workList.emplace_پٹھ(&bb);
        };

        قطار_وچ_پاؤ(cfg.باہر_نکلو());

        طالما (!workList.خالی_تھاواں()) {
            ثابت Cfg_بلاک* ثابت currp = workList.front();
            workList.نکالو_front();
            بلاک_State& state = m_بلاک_State[*currp];
            state.m_isOnWorkList = خطأ;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](ثابت Cfg_بلاک& bb) {
                خودکار& جیوندا_وچ = m_بلاک_State[bb].m_جیوندا_وچ;
                state.m_جیوندا_باہرلا.داخل_کرو(جیوندا_وچ.begin(), جیوندا_وچ.سرا());
            });

            // Apply transfer function of block
            ثابت منطقي بدلیا = transfer(*currp);
            // Enqueue predecessors
            إذا (بدلیا || !state.m_wasProcessed) currp->forEachPredecessor(قطار_وچ_پاؤ);
            // Mark as done with first visit
            state.m_wasProcessed = سچ;
        }
    }

عام:
    ساكن std::unique_ptr<std::سمتیہ<AstVarScope*>> apply(ثابت Cfg_گراف& cfg) {
        Cfg_جیوندا_Variables analysis{cfg};
        // If failed, return nullptr
        إذا (analysis.m_abort) أرجع خالی_سوچک;
        // Gather variables live in to the entry block
        ثابت std::unordered_سیٹ<AstVarScope*>& lin = analysis.m_بلاک_State[cfg.enter()].m_جیوندا_وچ;
        std::سمتیہ<AstVarScope*>* ثابت resultp
            = جديد std::سمتیہ<AstVarScope*>{lin.begin(), lin.سرا()};
        // Sort for stability
        std::stable_ترتیب_دیو(resultp->begin(), resultp->سرا(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             أرجع ap->name() < bp->name();
                         });
        أرجع std::unique_ptr<std::سمتیہ<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::سمتیہ<AstVarScope*>> V3Cfg::جیوندا_Var_Scopes(ثابت Cfg_گراف& cfg) {
    أرجع Cfg_جیوندا_Variables::apply(cfg);
}
