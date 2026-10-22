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

صنف Cfg_جيئرو_Variables final : VNVisitorConst {
    // State associted with each basic block
    بنية بلاڪ_State final {
        // Variables used in block, before a complete assignment in the same block
        std::unordered_سيٽ<AstVarScope*> m_gen;
        // Variables that are assigned a complete value in the basic block
        std::unordered_سيٽ<AstVarScope*> m_kill;
        std::unordered_سيٽ<AstVarScope*> m_جيئرو_اندريون;  // Variables live on entry to the block
        std::unordered_سيٽ<AstVarScope*> m_جيئرو_ٻاهريون;  // Variables live on exit from the block
        منطقي m_isOnWorkList = خطأ;  // Block is on work list
        منطقي m_wasProcessed = خطأ;  // Already processed at least once
    };

    // STATE
    ثابت Cfg_گراف& m_cfg;  // The CFG beign analysed
    // State for each block
    Cfg_بلاڪ_Map<بلاڪ_State> m_بلاڪ_State{m_cfg.make_بلاڪ_Map<بلاڪ_State>()};
    بلاڪ_State* m_currp = خالي_اشارو;  // State of current block being analysed
    منطقي m_abort = خطأ;  // Abort analysis - unhandled construct

    // METHODS

    ساكن منطقي isPacked(ثابت AstVarScope* vscp) {
        Ast_ڳنڍ_DType* ثابت dtypep = vscp->dtypep()->skipRefp();
        إذا (ثابت AstBasicDType* ثابت typep = VN_CAST(dtypep, BasicDType)) {
            أرجع typep->keyword().isIntNumeric();
        }
        إذا (VN_IS(dtypep, PackArrayDType)) أرجع سچ;
        إذا (ثابت Ast_ڳنڍ_UOr_Struct_DType* ثابت typep = VN_CAST(dtypep, ڳنڍ_UOr_Struct_DType)) {
            أرجع typep->packed();
        }
        أرجع خطأ;
    }

    فراغ updateGen(Ast_ڳنڍ* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ثابت AstVarRef* refp) {
            // Only care about reads
            إذا (refp->access().isWriteOnly()) أرجع خطأ;
            // Grab referenced variable
            AstVarScope* ثابت vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            إذا (m_currp->m_kill.ڳڻپ(vscp)) أرجع خطأ;
            m_currp->m_gen.emplace(vscp);
            أرجع خطأ;
        });
    }

    فراغ updateKill(Ast_ڳنڍ* nodep) {
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

    فراغ single(Ast_ڳنڍ* nodep) {
        // Assume all reads hapen before any writes
        إذا (m_abort) أرجع;
        updateGen(nodep);
        إذا (m_abort) أرجع;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    منطقي transfer(ثابت Cfg_بلاڪ& bb) {
        بلاڪ_State& state = m_بلاڪ_State[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_سيٽ<AstVarScope*> جيئرو_اندريون = state.m_gen;
        لكل (AstVarScope* ثابت varp : state.m_جيئرو_ٻاهريون) {
            إذا (state.m_kill.ڳڻپ(varp)) استمر;
            جيئرو_اندريون.وجهو(varp);
        }
        إذا (جيئرو_اندريون == state.m_جيئرو_اندريون) أرجع خطأ;
        std::مٽا_سٽا(جيئرو_اندريون, state.m_جيئرو_اندريون);
        أرجع سچ;
    }

    //گهمو
    فراغ گهمو(Ast_ڳنڍ* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = سچ;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    فراغ گهمو(Ast_سونپيو* nodep) override { single(nodep); }
    فراغ گهمو(Ast_سونپيو_W* nodep) override { single(nodep); }
    فراغ گهمو(AstDisplay* nodep) override { single(nodep); }
    فراغ گهمو(Ast_پڄاڻي* nodep) override { single(nodep); }
    فراغ گهمو(Ast_پڄاڻي_Fork* nodep) override { single(nodep); }
    فراغ گهمو(Ast_Stmt_اظهار* nodep) override { single(nodep); }
    فراغ گهمو(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    فراغ گهمو(AstIf* nodep) override { single(nodep->condp()); }
    فراغ گهمو(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    واضح Cfg_جيئرو_Variables(ثابت Cfg_گراف& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        لكل (ثابت V3_گراف_راس& vtx : cfg.vertices()) {
            ثابت Cfg_بلاڪ& bb = static_cast<ثابت Cfg_بلاڪ&>(vtx);
            إذا (m_abort) أرجع;
            VL_RESTORER(m_currp);
            m_currp = &m_بلاڪ_State[bb];
            لكل (Ast_ڳنڍ* ثابت stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        إذا (m_abort) أرجع;

        // Perform the flow analysis
        std::deque<ثابت Cfg_بلاڪ*> workList;
        ثابت خودڪار قطار_اندر_وجهو = [&](ثابت Cfg_بلاڪ& bb) {
            بلاڪ_State& state = m_بلاڪ_State[bb];
            إذا (state.m_isOnWorkList) أرجع;
            state.m_isOnWorkList = سچ;
            workList.emplace_پٺ(&bb);
        };

        قطار_اندر_وجهو(cfg.ٻاهر_نڪرو());

        طالما (!workList.خالي_جايون()) {
            ثابت Cfg_بلاڪ* ثابت currp = workList.front();
            workList.ڪڍو_front();
            بلاڪ_State& state = m_بلاڪ_State[*currp];
            state.m_isOnWorkList = خطأ;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](ثابت Cfg_بلاڪ& bb) {
                خودڪار& جيئرو_اندريون = m_بلاڪ_State[bb].m_جيئرو_اندريون;
                state.m_جيئرو_ٻاهريون.وجهو(جيئرو_اندريون.begin(), جيئرو_اندريون.پڇاڙي());
            });

            // Apply transfer function of block
            ثابت منطقي بدليو = transfer(*currp);
            // Enqueue predecessors
            إذا (بدليو || !state.m_wasProcessed) currp->forEachPredecessor(قطار_اندر_وجهو);
            // Mark as done with first visit
            state.m_wasProcessed = سچ;
        }
    }

عام:
    ساكن std::unique_ptr<std::ويڪٽر<AstVarScope*>> apply(ثابت Cfg_گراف& cfg) {
        Cfg_جيئرو_Variables analysis{cfg};
        // If failed, return nullptr
        إذا (analysis.m_abort) أرجع خالي_اشارو;
        // Gather variables live in to the entry block
        ثابت std::unordered_سيٽ<AstVarScope*>& lin = analysis.m_بلاڪ_State[cfg.enter()].m_جيئرو_اندريون;
        std::ويڪٽر<AstVarScope*>* ثابت resultp
            = جديد std::ويڪٽر<AstVarScope*>{lin.begin(), lin.پڇاڙي()};
        // Sort for stability
        std::stable_ترتيب_ڏيو(resultp->begin(), resultp->پڇاڙي(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             أرجع ap->name() < bp->name();
                         });
        أرجع std::unique_ptr<std::ويڪٽر<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::ويڪٽر<AstVarScope*>> V3Cfg::جيئرو_Var_Scopes(ثابت Cfg_گراف& cfg) {
    أرجع Cfg_جيئرو_Variables::apply(cfg);
}
