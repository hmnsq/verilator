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

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Cfg.h"

#అన్తర్భావ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

తరగతి Cfg_సజీవ_Variables final : VNVisitorConst {
    // State associted with each basic block
    నిర్మాణం ఖండం_State final {
        // Variables used in block, before a complete assignment in the same block
        std::unordered_సమితి<AstVarScope*> m_gen;
        // Variables that are assigned a complete value in the basic block
        std::unordered_సమితి<AstVarScope*> m_kill;
        std::unordered_సమితి<AstVarScope*> m_సజీవ_లోపల;  // Variables live on entry to the block
        std::unordered_సమితి<AstVarScope*> m_సజీవ_బహిర్గతం;  // Variables live on exit from the block
        తార్కిక m_isOnWorkList = అబద్ధం;  // Block is on work list
        తార్కిక m_wasProcessed = అబద్ధం;  // Already processed at least once
    };

    // STATE
    స్థిరరాశి Cfg_లేఖాచిత్రం& m_cfg;  // The CFG beign analysed
    // State for each block
    Cfg_ఖండం_Map<ఖండం_State> m_ఖండం_State{m_cfg.make_ఖండం_Map<ఖండం_State>()};
    ఖండం_State* m_currp = శూన్యసూచిక;  // State of current block being analysed
    తార్కిక m_abort = అబద్ధం;  // Abort analysis - unhandled construct

    // METHODS

    స్థిర తార్కిక isPacked(స్థిరరాశి AstVarScope* vscp) {
        Ast_కణుపు_DType* స్థిరరాశి dtypep = vscp->dtypep()->skipRefp();
        ఒకవేళ (స్థిరరాశి AstBasicDType* స్థిరరాశి typep = VN_CAST(dtypep, BasicDType)) {
            తిరిగి typep->keyword().isIntNumeric();
        }
        ఒకవేళ (VN_IS(dtypep, PackArrayDType)) తిరిగి నిజం;
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి typep = VN_CAST(dtypep, కణుపు_UOr_Struct_DType)) {
            తిరిగి typep->packed();
        }
        తిరిగి అబద్ధం;
    }

    శూన్య updateGen(Ast_కణుపు* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](స్థిరరాశి AstVarRef* refp) {
            // Only care about reads
            ఒకవేళ (refp->access().isWriteOnly()) తిరిగి అబద్ధం;
            // Grab referenced variable
            AstVarScope* స్థిరరాశి vscp = refp->varScopep();
            // Add to gen set, if not killed - assume whole variable read
            ఒకవేళ (m_currp->m_kill.లెక్క(vscp)) తిరిగి అబద్ధం;
            m_currp->m_gen.emplace(vscp);
            తిరిగి అబద్ధం;
        });
    }

    శూన్య updateKill(Ast_కణుపు* nodep) {
        UASSERT_OBJ(!m_abort, nodep, "Doing useless work");
        m_abort = nodep->exists([&](స్థిరరాశి AstVarRef* refp) {
            // Only care about writes
            ఒకవేళ (refp->access().isReadOnly()) తిరిగి అబద్ధం;
            // Grab referenced variable
            AstVarScope* స్థిరరాశి vscp = refp->varScopep();
            // Safety case: bail on variables with unusual semantics
            ఒకవేళ (vscp->varp()->ignoreSchedWrite()) తిరిగి నిజం;
            // If not a packed variable, assume it's not a whole update, don't add to kill set
            ఒకవేళ (!isPacked(vscp)) తిరిగి అబద్ధం;
            // If packed, and updated partially, don't add to kill set
            ఒకవేళ (refp->backp()->nextp() != refp && VN_IS(refp->abovep(), Sel)) తిరిగి అబద్ధం;
            // Whole variable is writen, add to kill set
            m_currp->m_kill.emplace(vscp);
            తిరిగి అబద్ధం;
        });
    }

    శూన్య single(Ast_కణుపు* nodep) {
        // Assume all reads hapen before any writes
        ఒకవేళ (m_abort) తిరిగి;
        updateGen(nodep);
        ఒకవేళ (m_abort) తిరిగి;
        updateKill(nodep);
    }

    // Apply transfer function of block, return true if changed
    తార్కిక transfer(స్థిరరాశి Cfg_ఖండం& bb) {
        ఖండం_State& state = m_ఖండం_State[bb];

        // liveIn = gen union (liveOut - kill)
        std::unordered_సమితి<AstVarScope*> సజీవ_లోపల = state.m_gen;
        కొరకు (AstVarScope* స్థిరరాశి varp : state.m_సజీవ_బహిర్గతం) {
            ఒకవేళ (state.m_kill.లెక్క(varp)) కొనసాగు;
            సజీవ_లోపల.చొప్పించు(varp);
        }
        ఒకవేళ (సజీవ_లోపల == state.m_సజీవ_లోపల) తిరిగి అబద్ధం;
        std::మార్పిడి(సజీవ_లోపల, state.m_సజీవ_లోపల);
        తిరిగి నిజం;
    }

    //సందర్శించు
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {  //
        UASSERT_OBJ(!m_abort, nodep, "Repeat traversal after abort");
        m_abort = నిజం;
        UINFO(9, "Unhandled AstNode type " << nodep->typeName());
    }

    శూన్య సందర్శించు(Ast_కేటాయించు* nodep) override { single(nodep); }
    శూన్య సందర్శించు(Ast_కేటాయించు_W* nodep) override { single(nodep); }
    శూన్య సందర్శించు(AstDisplay* nodep) override { single(nodep); }
    శూన్య సందర్శించు(Ast_ముగింపు* nodep) override { single(nodep); }
    శూన్య సందర్శించు(Ast_ముగింపు_Fork* nodep) override { single(nodep); }
    శూన్య సందర్శించు(Ast_Stmt_సమాసం* nodep) override { single(nodep); }
    శూన్య సందర్శించు(AstStop* nodep) override { single(nodep); }

    // Only the condition check belongs to the terminated basic block
    శూన్య సందర్శించు(AstIf* nodep) override { single(nodep->condp()); }
    శూన్య సందర్శించు(AstLoopTest* nodep) override { single(nodep->condp()); }

    // CONSTRUCTOR
    స్పష్టమైన Cfg_సజీవ_Variables(స్థిరరాశి Cfg_లేఖాచిత్రం& cfg)
        : m_cfg{cfg} {
        // For each basic block, compute the gen and kill set via visit
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : cfg.vertices()) {
            స్థిరరాశి Cfg_ఖండం& bb = static_cast<స్థిరరాశి Cfg_ఖండం&>(vtx);
            ఒకవేళ (m_abort) తిరిగి;
            VL_RESTORER(m_currp);
            m_currp = &m_ఖండం_State[bb];
            కొరకు (Ast_కణుపు* స్థిరరాశి stmtp : bb.stmtps()) iterateConst(stmtp);
        }
        ఒకవేళ (m_abort) తిరిగి;

        // Perform the flow analysis
        std::deque<స్థిరరాశి Cfg_ఖండం*> workList;
        స్థిరరాశి స్వయంచాలక వరుసలో_చేర్చు = [&](స్థిరరాశి Cfg_ఖండం& bb) {
            ఖండం_State& state = m_ఖండం_State[bb];
            ఒకవేళ (state.m_isOnWorkList) తిరిగి;
            state.m_isOnWorkList = నిజం;
            workList.emplace_వెనుక(&bb);
        };

        వరుసలో_చేర్చు(cfg.నిష్క్రమించు());

        అయితే (!workList.ఖాళీలు()) {
            స్థిరరాశి Cfg_ఖండం* స్థిరరాశి currp = workList.front();
            workList.తీయి_front();
            ఖండం_State& state = m_ఖండం_State[*currp];
            state.m_isOnWorkList = అబద్ధం;

            // Compute meet (liveOut = union liveIn of successors)
            currp->forEachSuccessor([&](స్థిరరాశి Cfg_ఖండం& bb) {
                స్వయంచాలక& సజీవ_లోపల = m_ఖండం_State[bb].m_సజీవ_లోపల;
                state.m_సజీవ_బహిర్గతం.చొప్పించు(సజీవ_లోపల.begin(), సజీవ_లోపల.చివర());
            });

            // Apply transfer function of block
            స్థిరరాశి తార్కిక మారింది = transfer(*currp);
            // Enqueue predecessors
            ఒకవేళ (మారింది || !state.m_wasProcessed) currp->forEachPredecessor(వరుసలో_చేర్చు);
            // Mark as done with first visit
            state.m_wasProcessed = నిజం;
        }
    }

బహిరంగ:
    స్థిర std::unique_ptr<std::సదిశ<AstVarScope*>> apply(స్థిరరాశి Cfg_లేఖాచిత్రం& cfg) {
        Cfg_సజీవ_Variables analysis{cfg};
        // If failed, return nullptr
        ఒకవేళ (analysis.m_abort) తిరిగి శూన్యసూచిక;
        // Gather variables live in to the entry block
        స్థిరరాశి std::unordered_సమితి<AstVarScope*>& lin = analysis.m_ఖండం_State[cfg.enter()].m_సజీవ_లోపల;
        std::సదిశ<AstVarScope*>* స్థిరరాశి resultp
            = కొత్త std::సదిశ<AstVarScope*>{lin.begin(), lin.చివర()};
        // Sort for stability
        std::stable_క్రమీకరించు(resultp->begin(), resultp->చివర(),
                         [](AstVarScope* ap, AstVarScope* bp) {  //
                             తిరిగి ap->name() < bp->name();
                         });
        తిరిగి std::unique_ptr<std::సదిశ<AstVarScope*>>{resultp};
    }
};

std::unique_ptr<std::సదిశ<AstVarScope*>> V3Cfg::సజీవ_Var_Scopes(స్థిరరాశి Cfg_లేఖాచిత్రం& cfg) {
    తిరిగి Cfg_సజీవ_Variables::apply(cfg);
}
