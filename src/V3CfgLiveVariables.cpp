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

वर्ग Cfg_जीवितम्_Variables final : VNVisitorConst {
    // State associted with each basic block
    संरचना खण्डम्_State final {
        // Variables used in block, before a complete assignment in the same block
        std::unordered_समुच्चयः<AstVarScope*> m_gen;
        // Variables that are assigned a complete value in the basic block
        std::unordered_समुच्चयः<AstVarScope*> m_kill;
        std::unordered_समुच्चयः<AstVarScope*> m_जीवितम्_अन्तर्;  // Variables live on entry to the block
        std::unordered_समुच्चयः<AstVarScope*> m_जीवितम्_बहिर्गमनम्;  // Variables live on exit from the block
        तार्किक m_isOnWorkList = असत्य;  // Block is on work list
        तार्किक m_wasProcessed = असत्य;  // Already processed at least once
    };

    // STATE
    ध्रुव CfgGraph& m_cfg;  // The CFG beign analysed
    // State for each block
    Cfg_खण्डम्_Map<खण्डम्_State> m_खण्डम्_State{m_cfg.make_खण्डम्_Map<खण्डम्_State>()};
    खण्डम्_State* m_currp = शून्यसूचकः;  // State of current block being analysed
    तार्किक m_abort = असत्य;  // Abort analysis - unhandled construct

    // METHODS

    स्थिर तार्किक isPacked(ध्रुव AstVarScope* vscp) {
        Ast_ग्रन्थिः_DType* ध्रुव dtypep = vscp->dtypep()->skipRefp();
        यदि (ध्रुव AstBasicDType* ध्रुव typep = VN_CAST(dtypep, BasicDType)) {
            प्रत्यागम typep->keyword().isIntNumeric();
        }
        यदि (VN_IS(dtypep, PackArrayDType)) प्रत्यागम सत्य;
        यदि (ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव typep = VN_CAST(dtypep, ग्रन्थिः_UOr_Struct_DType)) {
            प्रत्यागम typep->packed();
        }
        प्रत्यागम असत्य;
    }

    शून्य updateGen(Ast_ग्रन्थिः* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ध्रुव AstVarRef* refp) {
            // Only care about reads
            यदि (refp->access().isWriteOnly()) प्रत्यागम असत्य;
            // Grab referenced variable
            AstVarScope* ध्रुव vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            यदि (m_currp->m_kill.गणनम्(vscp)) प्रत्यागम असत्य;
            m_currp->m_gen.emplace(vscp);
            प्रत्यागम असत्य;
        });
    }

    शून्य updateKill(Ast_ग्रन्थिः* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](ध्रुव AstVarRef* refp) {
            // Only care about writes
            यदि (refp->access().isReadOnly()) प्रत्यागम असत्य;
            // Grab referenced variable
            AstVarScope* ध्रुव vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            यदि (vscp->varp()->ignoreSchedWrite()) प्रत्यागम सत्य;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            यदि (!isPacked(vscp)) प्रत्यागम असत्य;
            // If packed, and updated partially, don't add to kill set
            यदि (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) प्रत्यागम असत्य;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            प्रत्यागम असत्य;
        });
    }

    शून्य single(Ast_ग्रन्थिः* nodep) {
        // Assume all reads hapen before any writes
        यदि (m_abort) प्रत्यागम;
        updateGen(nodep);
        यदि (m_abort) प्रत्यागम;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    तार्किक transfer(ध्रुव Cfg_खण्डम्& bb) {
        खण्डम्_State& state = m_खण्डम्_State[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_समुच्चयः<AstVarScope*> जीवितम्_अन्तर् = state.m_gen;
        प्रति (AstVarScope* ध्रुव varp : state.m_जीवितम्_बहिर्गमनम्) {
            यदि (state.m_kill.गणनम्(varp)) अनुवर्त;
            जीवितम्_अन्तर्.निवेशय(varp);
        }
        यदि (जीवितम्_अन्तर् == state.m_जीवितम्_अन्तर्) प्रत्यागम असत्य;
        std::swap(जीवितम्_अन्तर्, state.m_जीवितम्_अन्तर्);
        प्रत्यागम सत्य;
    }

    //दर्शय
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = सत्य;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    शून्य दर्शय(Ast_नियोजय* nodep) override { single(nodep); }
    शून्य दर्शय(Ast_नियोजय_W* nodep) override { single(nodep); }
    शून्य दर्शय(AstDisplay* nodep) override { single(nodep); }
    शून्य दर्शय(AstFinish* nodep) override { single(nodep); }
    शून्य दर्शय(AstFinishFork* nodep) override { single(nodep); }
    शून्य दर्शय(Ast_Stmt_व्यञ्जकम्* nodep) override { single(nodep); }
    शून्य दर्शय(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    शून्य दर्शय(AstIf* nodep) override { single(nodep->condp()); }
    शून्य दर्शय(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    स्पष्ट Cfg_जीवितम्_Variables(ध्रुव CfgGraph& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        प्रति (ध्रुव V3GraphVertex& vtx : cfg.vertices()) {
            ध्रुव Cfg_खण्डम्& bb = static_cast<ध्रुव Cfg_खण्डम्&>(vtx);
            यदि (m_abort) प्रत्यागम;
            VL_RESTORER(m_currp);
            m_currp = &m_खण्डम्_State[bb];
            प्रति (Ast_ग्रन्थिः* ध्रुव stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        यदि (m_abort) प्रत्यागम;

        // Perform the flow analysis
        std::deque<ध्रुव Cfg_खण्डम्*> workList;
        ध्रुव स्वचालित पङ्क्तौ_निक्षिप = [&](ध्रुव Cfg_खण्डम्& bb) {
            खण्डम्_State& state = m_खण्डम्_State[bb];
            यदि (state.m_isOnWorkList) प्रत्यागम;
            state.m_isOnWorkList = सत्य;
            workList.emplace_पृष्ठभागः(&bb);
        };

        पङ्क्तौ_निक्षिप(cfg.निर्गच्छ());

        यावत् (!workList.रिक्तानि()) {
            ध्रुव Cfg_खण्डम्* ध्रुव currp = workList.front();
            workList.निष्कासय_front();
            खण्डम्_State& state = m_खण्डम्_State[*currp];
            state.m_isOnWorkList = असत्य;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](ध्रुव Cfg_खण्डम्& bb) {
                स्वचालित& जीवितम्_अन्तर् = m_खण्डम्_State[bb].m_जीवितम्_अन्तर्;
                state.m_जीवितम्_बहिर्गमनम्.निवेशय(जीवितम्_अन्तर्.begin(), जीवितम्_अन्तर्.अन्तः_2());
            });

            // Apply transfer function of block
            ध्रुव तार्किक परिवर्तितम् = transfer(*currp);
            // Enqueue predecessors
            यदि (परिवर्तितम् || !state.m_wasProcessed) currp->forEachPredecessor(पङ्क्तौ_निक्षिप);
            // Mark as done with first visit
            state.m_wasProcessed = सत्य;
        }
    }

सार्वजनिक:
    स्थिर std::unique_ptr<std::सदिशम्<AstVarScope*>> apply(ध्रुव CfgGraph& cfg) {
        Cfg_जीवितम्_Variables analysis{cfg};
        // If failed, return nullptr
        यदि (analysis.m_abort) प्रत्यागम शून्यसूचकः;
        // Gather variables live in to the entry block
        ध्रुव std::unordered_समुच्चयः<AstVarScope*>& lin = analysis.m_खण्डम्_State[cfg.enter()].m_जीवितम्_अन्तर्;
        std::सदिशम्<AstVarScope*>* ध्रुव resultp
            = नूतन std::सदिशम्<AstVarScope*>{lin.begin(), lin.अन्तः_2()};
        // Sort for stability
        std::stable_क्रमय(resultp->begin(), resultp->अन्तः_2(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             प्रत्यागम ap->name() < bp->name();
                         });
        प्रत्यागम std::unique_ptr<std::सदिशम्<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::सदिशम्<AstVarScope*>> V3Cfg::जीवितम्_Var_Scopes(ध्रुव CfgGraph& cfg) {
    प्रत्यागम Cfg_जीवितम्_Variables::apply(cfg);
}
