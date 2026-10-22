// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Convert AstModule to DfgGraph
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
// Convert and AstModule (before V3Scope), or the entire AstNetlist
// (after V3Scope) to an initial DfgGraph composed onlyof DfgLogic,
// DfgUnresolved and DfgVertexVar vertices. This will later be synthesized
// into primitive operations by V3DfgPasses::synthesize.
//
//*************************************************************************

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Cfg.h"
#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3Dfg.h"
#అన్తర్భావ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

తరగతి Ast_గమ్యం_Dfg_కూడు_Ast_Refs final : బహిరంగ VNVisitorConst {
    // STATE
    Dfg_లేఖాచిత్రం& m_dfg;  // The graph being processed
    // Function to get the DfgVertexVar for a AstVarScope
    స్థిరరాశి std::function<Dfg_శీర్షం_Var*(AstVarScope*)> m_get_Var_శీర్షం;
    తార్కిక m_లోపల_Sen_అంశం = అబద్ధం;  // Inside an AstSenItem
    తార్కిక m_లోపల_Loop = అబద్ధం;  // Inside an AstLoop

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

    శూన్య సందర్శించు(Ast_Sen_అంశం* nodep) override {
        VL_RESTORER(m_లోపల_Sen_అంశం);
        m_లోపల_Sen_అంశం = నిజం;
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(AstLoop* nodep) override {
        VL_RESTORER(m_లోపల_Loop);
        m_లోపల_Loop = నిజం;
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(AstVarRef* nodep) override {
        // Disguised hierarchical reference handled as external reference, ignore
        ఒకవేళ (nodep->classOrPackagep()) తిరిగి;
        // If target is not supported, ignore
        AstVarScope* స్థిరరాశి vscp = nodep->varScopep();
        ఒకవేళ (!V3Dfg::isSupported(vscp)) తిరిగి;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        Dfg_శీర్షం_Var* స్థిరరాశి varp = m_get_Var_శీర్షం(vscp);
        ఒకవేళ (!varp) తిరిగి;
        // Create Ast reference vertices
        ఒకవేళ (nodep->access().isReadOnly()) {
            DfgAstRd* స్థిరరాశి astp = కొత్త DfgAstRd{m_dfg, nodep, m_లోపల_Sen_అంశం, m_లోపల_Loop};
            astp->srcp(varp);
            తిరిగి;
        }
        // Mark as written from non-DFG logic
        Dfg_శీర్షం_Var::సమితి_Has_Mod_Wr_Refs(vscp);
    }

    Ast_గమ్యం_Dfg_కూడు_Ast_Refs(Dfg_లేఖాచిత్రం& dfg, Ast_కణుపు* nodep,
                       std::function<Dfg_శీర్షం_Var*(AstVarScope*)> get_Var_శీర్షం)
        : m_dfg{dfg}
        , m_get_Var_శీర్షం{get_Var_శీర్షం} {
        iterateConst(nodep);
    }

బహిరంగ:
    స్థిర శూన్య apply(Dfg_లేఖాచిత్రం& dfg, Ast_కణుపు* nodep,
                      std::function<Dfg_శీర్షం_Var*(AstVarScope*)> get_Var_శీర్షం) {
        Ast_గమ్యం_Dfg_కూడు_Ast_Refs{dfg, nodep, get_Var_శీర్షం};
    }
};

శూన్య V3DfgPasses::కూడు_Ast_Refs(Dfg_లేఖాచిత్రం& dfg, Ast_కణుపు* nodep,
                             std::function<Dfg_శీర్షం_Var*(AstVarScope*)> get_Var_శీర్షం) {
    Ast_గమ్యం_Dfg_కూడు_Ast_Refs::apply(dfg, nodep, get_Var_శీర్షం);
}

తరగతి Ast_గమ్యం_Dfg_Visitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // AstVarScope::user2() -> DfgVertexVar* : the corresponding variable vertex
    // AstVarScope::user3() -> bool : Already gathered - used fine grained below
    స్థిరరాశి VNUser2_లోపల_Use m_user2_లోపల_Use;

    // STATE
    Dfg_లేఖాచిత్రం& m_dfg;  // The graph being built
    V3_Dfg_Ast_గమ్యం_Dfg_Context& m_ctx;  // The context for stats
    AstScope* m_scopep = శూన్యసూచిక;  // The current scope, iff T_Scoped

    // METHODS

    // Mark variables referenced under node
    శూన్య markReferenced(Ast_కణుపు* nodep) {
        V3DfgPasses::కూడు_Ast_Refs(m_dfg, nodep, [ఇది](AstVarScope* vscp) {  //
            తిరిగి get_Var_శీర్షం(vscp);
        });
    }

    Dfg_శీర్షం_Var* get_Var_శీర్షం(AstVarScope* varp) {
        ఒకవేళ (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = varp->dtypep()->skipRefp();
            Dfg_శీర్షం_Var* స్థిరరాశి vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<Dfg_శీర్షం_Var*>(కొత్త DfgVarArray{m_dfg, varp})
                      : static_cast<Dfg_శీర్షం_Var*>(కొత్త DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        తిరిగి varp->user2u().మూస గమ్యం<Dfg_శీర్షం_Var*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::సదిశ<Dfg_శీర్షం_Var*>> gatherWritten(స్థిరరాశి Ast_కణుపు* nodep) {
        స్థిరరాశి VNUser3_లోపల_Use user3_లోపల_Use;
        std::unique_ptr<std::సదిశ<Dfg_శీర్షం_Var*>> resp{కొత్త std::సదిశ<Dfg_శీర్షం_Var*>{}};
        స్థిరరాశి తార్కిక abort = nodep->exists([&](స్థిరరాశి AstVarRef* refp) -> తార్కిక {
            ఒకవేళ (refp->access().isReadOnly()) తిరిగి అబద్ధం;
            AstVarScope* స్థిరరాశి vscp = refp->varScopep();
            ఒకవేళ (!V3Dfg::isSupported(vscp)) తిరిగి నిజం;
            ఒకవేళ (!vscp->user3_సమితి_Once()) resp->emplace_వెనుక(get_Var_శీర్షం(vscp));
            తిరిగి అబద్ధం;
        });
        ఒకవేళ (abort) {
            ++m_ctx.m_nonRepVar;
            తిరిగి శూన్యసూచిక;
        }
        తిరిగి resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::సదిశ<Dfg_శీర్షం_Var*>> gatherRead(స్థిరరాశి Ast_కణుపు* nodep) {
        స్థిరరాశి VNUser3_లోపల_Use user3_లోపల_Use;
        std::unique_ptr<std::సదిశ<Dfg_శీర్షం_Var*>> resp{కొత్త std::సదిశ<Dfg_శీర్షం_Var*>{}};
        స్థిరరాశి తార్కిక abort = nodep->exists([&](స్థిరరాశి AstVarRef* refp) -> తార్కిక {
            ఒకవేళ (refp->access().isWriteOnly()) తిరిగి అబద్ధం;
            AstVarScope* స్థిరరాశి vscp = refp->varScopep();
            ఒకవేళ (!V3Dfg::isSupported(vscp)) తిరిగి నిజం;
            ఒకవేళ (!vscp->user3_సమితి_Once()) resp->emplace_వెనుక(get_Var_శీర్షం(vscp));
            తిరిగి అబద్ధం;
        });
        ఒకవేళ (abort) {
            ++m_ctx.m_nonRepVar;
            తిరిగి శూన్యసూచిక;
        }
        తిరిగి resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::సదిశ<Dfg_శీర్షం_Var*>> gather_సజీవ(స్థిరరాశి Cfg_లేఖాచిత్రం& cfg) {
        // Run analysis
        std::unique_ptr<std::సదిశ<AstVarScope*>> varps = V3Cfg::సజీవ_Var_Scopes(cfg);
        ఒకవేళ (!varps) {
            ++m_ctx.m_non_Rep_సజీవ;
            తిరిగి శూన్యసూచిక;
        }

        // Convert to vertics
        స్థిరరాశి VNUser3_లోపల_Use user3_లోపల_Use;
        std::unique_ptr<std::సదిశ<Dfg_శీర్షం_Var*>> resp{కొత్త std::సదిశ<Dfg_శీర్షం_Var*>{}};
        resp->reserve(varps->కొలత());
        కొరకు (AstVarScope* స్థిరరాశి varp : *varps) {
            ఒకవేళ (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                తిరిగి శూన్యసూచిక;
            }
            UASSERT_OBJ(!varp->user3_సమితి_Once(), varp, "Live variables should be unique");
            resp->emplace_వెనుక(get_Var_శీర్షం(varp));
        }
        తిరిగి resp;
    }

    // Connect inputs and outputs of a DfgLogic
    శూన్య connect(DfgLogic& vtx, స్థిరరాశి std::సదిశ<Dfg_శీర్షం_Var*>& iVarps,
                 స్థిరరాశి std::సదిశ<Dfg_శీర్షం_Var*>& oVarps) {
        // Connect inputs
        కొరకు (Dfg_శీర్షం_Var* స్థిరరాశి iVarp : iVarps) vtx.కూడు_ఆదానం(iVarp);
        // Connect outputs
        కొరకు (Dfg_శీర్షం_Var* స్థిరరాశి oVarp : oVarps) {
            ఒకవేళ (!oVarp->srcp()) oVarp->srcp(కొత్త DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->కూడు_Driver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    తార్కిక convert(AstAlways* nodep) {
        స్థిరరాశి VAlwaysKwd kwd = nodep->keyword();
        ఒకవేళ (kwd == VAlwaysKwd::CONT_కేటాయించు) {
            // TODO: simplify once CFG analysis can handle arrays
            ఒకవేళ (స్థిరరాశి Ast_కేటాయించు_W* స్థిరరాశి ap = VN_CAST(nodep->stmtsp(), కేటాయించు_W)) {
                ఒకవేళ (ap->nextp()) తిరిగి అబద్ధం;
                // Cannot handle assignment with timing control
                ఒకవేళ (ap->timingControlp()) తిరిగి అబద్ధం;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                స్థిరరాశి std::unique_ptr<std::సదిశ<Dfg_శీర్షం_Var*>> oVarpsp = gatherWritten(ap);
                ఒకవేళ (!oVarpsp) తిరిగి అబద్ధం;
                // Gather read variables, give up if any are not supported
                స్థిరరాశి std::unique_ptr<std::సదిశ<Dfg_శీర్షం_Var*>> iVarpsp = gatherRead(ap);
                ఒకవేళ (!iVarpsp) తిరిగి అబద్ధం;
                // Create the DfgLogic
                DfgLogic* స్థిరరాశి logicp = కొత్త DfgLogic{m_dfg, nodep, m_scopep, శూన్యసూచిక};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                //పూర్తి
                ++m_ctx.m_representable;
                తిరిగి నిజం;
            }
        }

        // Can only handle combinational logic
        ఒకవేళ (nodep->sentreep()) తిరిగి అబద్ధం;
        ఒకవేళ (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_కేటాయించు) {
            తిరిగి అబద్ధం;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<Cfg_లేఖాచిత్రం> cfgp = Cfg_లేఖాచిత్రం::build(nodep->stmtsp());
        ఒకవేళ (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            తిరిగి అబద్ధం;
        }
        // Gather written variables, give up if any are not supported
        స్థిరరాశి std::unique_ptr<std::సదిశ<Dfg_శీర్షం_Var*>> oVarpsp = gatherWritten(nodep);
        ఒకవేళ (!oVarpsp) తిరిగి అబద్ధం;
        // Gather read variables, give up if any are not supported
        స్థిరరాశి std::unique_ptr<std::సదిశ<Dfg_శీర్షం_Var*>> iVarpsp = gather_సజీవ(*cfgp);
        ఒకవేళ (!iVarpsp) తిరిగి అబద్ధం;
        // Create the DfgLogic
        DfgLogic* స్థిరరాశి logicp = కొత్త DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        //పూర్తి
        ++m_ctx.m_representable;
        తిరిగి నిజం;
    }

    // VISITORS
    // Unhandled node
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    శూన్య సందర్శించు(AstNetlist* nodep) override { iterate_And_తదుపరి_Null(nodep->modulesp()); }
    శూన్య సందర్శించు(AstModule* nodep) override { iterate_And_తదుపరి_Null(nodep->stmtsp()); }
    శూన్య సందర్శించు(AstIface* nodep) override {
        ఒకవేళ (!nodep->hasVirtualRef()) {
            iterate_And_తదుపరి_Null(nodep->stmtsp());
        } లేకపోతే {
            markReferenced(nodep);
        }
    }
    శూన్య సందర్శించు(Ast_శిఖరం_Scope* nodep) override { iterate(nodep->scopep()); }
    శూన్య సందర్శించు(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_చురుకైన* nodep) override {
        ఒకవేళ (nodep->hasCombo()) {
            iterateChildren(nodep);
        } లేకపోతే {
            markReferenced(nodep);
        }
    }

    // Non-representable constructs
    శూన్య సందర్శించు(AstCell* nodep) override { markReferenced(nodep); }
    శూన్య సందర్శించు(Ast_కణుపు_Procedure* nodep) override { markReferenced(nodep); }

    // Potentially representable constructs
    శూన్య సందర్శించు(AstAlways* nodep) override {
        ఒకవేళ (!convert(nodep)) markReferenced(nodep);
    }

    // CONSTRUCTOR
    Ast_గమ్యం_Dfg_Visitor(Dfg_లేఖాచిత్రం& dfg, AstNetlist& మూలం, V3_Dfg_Ast_గమ్యం_Dfg_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {
        iterate(&మూలం);
    }
    VL_UNCOPYABLE(Ast_గమ్యం_Dfg_Visitor);
    VL_UNMOVABLE(Ast_గమ్యం_Dfg_Visitor);

బహిరంగ:
    స్థిర శూన్య apply(Dfg_లేఖాచిత్రం& dfg, AstNetlist& మూలం, V3_Dfg_Ast_గమ్యం_Dfg_Context& ctx) {
        // Convert all logic under 'root'
        Ast_గమ్యం_Dfg_Visitor{dfg, మూలం, ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        కొరకు (Dfg_శీర్షం_Var* స్థిరరాశి varp : dfg.varVertices().unlinkable()) {
            ఒకవేళ (varp->srcp()) కొనసాగు;
            స్థిరరాశి తార్కిక keep = varp->foreachSink([&](Dfg_శీర్షం& d) { తిరిగి !d.is<DfgAstRd>(); });
            ఒకవేళ (!keep) {
                అయితే (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<Dfg_లేఖాచిత్రం> V3DfgPasses::ast_గమ్యం_Dfg(AstNetlist& netlist, V3DfgContext& ctx) {
    Dfg_లేఖాచిత్రం* స్థిరరాశి dfgp = కొత్త Dfg_లేఖాచిత్రం{ctx, "netlist"};
    Ast_గమ్యం_Dfg_Visitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    తిరిగి std::unique_ptr<Dfg_లేఖాచిత్రం>{dfgp};
}
