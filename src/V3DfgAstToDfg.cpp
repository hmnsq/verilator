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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Cfg.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dfg.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ਵਰਗ Ast_ਮੰਜ਼ਿਲ_Dfg_ਜੋੜੋ_Ast_Refs final : ਜਨਤਕ VNVisitorConst {
    // STATE
    Dfg_ਗ੍ਰਾਫ& m_dfg;  // The graph being processed
    // Function to get the DfgVertexVar for a AstVarScope
    ਅਚਲ std::function<Dfg_ਸ਼ੀਰਸ਼_Var*(AstVarScope*)> m_get_Var_ਸ਼ੀਰਸ਼;
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Sen_ਵਸਤੂ = ਝੂਠ;  // Inside an AstSenItem
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Loop = ਝੂਠ;  // Inside an AstLoop

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

    ਖਾਲੀ ਦੇਖੋ(Ast_Sen_ਵਸਤੂ* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Sen_ਵਸਤੂ);
        m_ਵਿੱਚ_Sen_ਵਸਤੂ = ਸੱਚ;
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstLoop* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Loop);
        m_ਵਿੱਚ_Loop = ਸੱਚ;
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        // Disguised hierarchical reference handled as external reference, ignore
        ਜੇ (nodep->classOrPackagep()) ਵਾਪਸ;
        // If target is not supported, ignore
        AstVarScope* ਅਚਲ vscp = nodep->varScopep();
        ਜੇ (!V3Dfg::isSupported(vscp)) ਵਾਪਸ;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = m_get_Var_ਸ਼ੀਰਸ਼(vscp);
        ਜੇ (!varp) ਵਾਪਸ;
        // Create Ast reference vertices
        ਜੇ (nodep->access().isReadOnly()) {
            DfgAstRd* ਅਚਲ astp = ਨਵਾਂ DfgAstRd{m_dfg, nodep, m_ਵਿੱਚ_Sen_ਵਸਤੂ, m_ਵਿੱਚ_Loop};
            astp->srcp(varp);
            ਵਾਪਸ;
        }
        // Mark as written from non-DFG logic
        Dfg_ਸ਼ੀਰਸ਼_Var::ਸੈੱਟ_Has_Mod_Wr_Refs(vscp);
    }

    Ast_ਮੰਜ਼ਿਲ_Dfg_ਜੋੜੋ_Ast_Refs(Dfg_ਗ੍ਰਾਫ& dfg, Ast_ਗੰਢ* nodep,
                       std::function<Dfg_ਸ਼ੀਰਸ਼_Var*(AstVarScope*)> get_Var_ਸ਼ੀਰਸ਼)
        : m_dfg{dfg}
        , m_get_Var_ਸ਼ੀਰਸ਼{get_Var_ਸ਼ੀਰਸ਼} {
        iterateConst(nodep);
    }

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(Dfg_ਗ੍ਰਾਫ& dfg, Ast_ਗੰਢ* nodep,
                      std::function<Dfg_ਸ਼ੀਰਸ਼_Var*(AstVarScope*)> get_Var_ਸ਼ੀਰਸ਼) {
        Ast_ਮੰਜ਼ਿਲ_Dfg_ਜੋੜੋ_Ast_Refs{dfg, nodep, get_Var_ਸ਼ੀਰਸ਼};
    }
};

ਖਾਲੀ V3DfgPasses::ਜੋੜੋ_Ast_Refs(Dfg_ਗ੍ਰਾਫ& dfg, Ast_ਗੰਢ* nodep,
                             std::function<Dfg_ਸ਼ੀਰਸ਼_Var*(AstVarScope*)> get_Var_ਸ਼ੀਰਸ਼) {
    Ast_ਮੰਜ਼ਿਲ_Dfg_ਜੋੜੋ_Ast_Refs::apply(dfg, nodep, get_Var_ਸ਼ੀਰਸ਼);
}

ਵਰਗ Ast_ਮੰਜ਼ਿਲ_Dfg_Visitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // AstVarScope::user2() -> DfgVertexVar* : the corresponding variable vertex
    // AstVarScope::user3() -> bool : Already gathered - used fine grained below
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_user2_ਵਿੱਚ_Use;

    // STATE
    Dfg_ਗ੍ਰਾਫ& m_dfg;  // The graph being built
    V3_Dfg_Ast_ਮੰਜ਼ਿਲ_Dfg_Context& m_ctx;  // The context for stats
    AstScope* m_scopep = ਖਾਲੀ_ਸੂਚਕ;  // The current scope, iff T_Scoped

    // METHODS

    // Mark variables referenced under node
    ਖਾਲੀ markReferenced(Ast_ਗੰਢ* nodep) {
        V3DfgPasses::ਜੋੜੋ_Ast_Refs(m_dfg, nodep, [ਇਹ](AstVarScope* vscp) {  //
            ਵਾਪਸ get_Var_ਸ਼ੀਰਸ਼(vscp);
        });
    }

    Dfg_ਸ਼ੀਰਸ਼_Var* get_Var_ਸ਼ੀਰਸ਼(AstVarScope* varp) {
        ਜੇ (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = varp->dtypep()->skipRefp();
            Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<Dfg_ਸ਼ੀਰਸ਼_Var*>(ਨਵਾਂ DfgVarArray{m_dfg, varp})
                      : static_cast<Dfg_ਸ਼ੀਰਸ਼_Var*>(ਨਵਾਂ DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        ਵਾਪਸ varp->user2u().ਸਾਂਚਾ ਮੰਜ਼ਿਲ<Dfg_ਸ਼ੀਰਸ਼_Var*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼_Var*>> gatherWritten(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਅਚਲ VNUser3_ਵਿੱਚ_Use user3_ਵਿੱਚ_Use;
        std::unique_ptr<std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼_Var*>> resp{ਨਵਾਂ std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼_Var*>{}};
        ਅਚਲ ਤਾਰਕਿਕ abort = nodep->exists([&](ਅਚਲ AstVarRef* refp) -> ਤਾਰਕਿਕ {
            ਜੇ (refp->access().isReadOnly()) ਵਾਪਸ ਝੂਠ;
            AstVarScope* ਅਚਲ vscp = refp->varScopep();
            ਜੇ (!V3Dfg::isSupported(vscp)) ਵਾਪਸ ਸੱਚ;
            ਜੇ (!vscp->user3_ਸੈੱਟ_Once()) resp->emplace_ਪਿੱਠ(get_Var_ਸ਼ੀਰਸ਼(vscp));
            ਵਾਪਸ ਝੂਠ;
        });
        ਜੇ (abort) {
            ++m_ctx.m_nonRepVar;
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        ਵਾਪਸ resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼_Var*>> gatherRead(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਅਚਲ VNUser3_ਵਿੱਚ_Use user3_ਵਿੱਚ_Use;
        std::unique_ptr<std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼_Var*>> resp{ਨਵਾਂ std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼_Var*>{}};
        ਅਚਲ ਤਾਰਕਿਕ abort = nodep->exists([&](ਅਚਲ AstVarRef* refp) -> ਤਾਰਕਿਕ {
            ਜੇ (refp->access().isWriteOnly()) ਵਾਪਸ ਝੂਠ;
            AstVarScope* ਅਚਲ vscp = refp->varScopep();
            ਜੇ (!V3Dfg::isSupported(vscp)) ਵਾਪਸ ਸੱਚ;
            ਜੇ (!vscp->user3_ਸੈੱਟ_Once()) resp->emplace_ਪਿੱਠ(get_Var_ਸ਼ੀਰਸ਼(vscp));
            ਵਾਪਸ ਝੂਠ;
        });
        ਜੇ (abort) {
            ++m_ctx.m_nonRepVar;
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        ਵਾਪਸ resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼_Var*>> gather_ਜੀਵਤ(ਅਚਲ Cfg_ਗ੍ਰਾਫ& cfg) {
        // Run analysis
        std::unique_ptr<std::ਸਦਿਸ਼<AstVarScope*>> varps = V3Cfg::ਜੀਵਤ_Var_Scopes(cfg);
        ਜੇ (!varps) {
            ++m_ctx.m_non_Rep_ਜੀਵਤ;
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }

        // Convert to vertics
        ਅਚਲ VNUser3_ਵਿੱਚ_Use user3_ਵਿੱਚ_Use;
        std::unique_ptr<std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼_Var*>> resp{ਨਵਾਂ std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼_Var*>{}};
        resp->reserve(varps->ਮਾਪ());
        ਲਈ (AstVarScope* ਅਚਲ varp : *varps) {
            ਜੇ (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            }
            UASSERT_OBJ(!varp->user3_ਸੈੱਟ_Once(), varp, "Live variables should be unique");
            resp->emplace_ਪਿੱਠ(get_Var_ਸ਼ੀਰਸ਼(varp));
        }
        ਵਾਪਸ resp;
    }

    // Connect inputs and outputs of a DfgLogic
    ਖਾਲੀ connect(DfgLogic& vtx, ਅਚਲ std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼_Var*>& iVarps,
                 ਅਚਲ std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼_Var*>& oVarps) {
        // Connect inputs
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ iVarp : iVarps) vtx.ਜੋੜੋ_ਨਿਵੇਸ਼(iVarp);
        // Connect outputs
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ oVarp : oVarps) {
            ਜੇ (!oVarp->srcp()) oVarp->srcp(ਨਵਾਂ DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->ਜੋੜੋ_Driver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    ਤਾਰਕਿਕ convert(AstAlways* nodep) {
        ਅਚਲ VAlwaysKwd kwd = nodep->keyword();
        ਜੇ (kwd == VAlwaysKwd::CONT_ਸੌਂਪੋ) {
            // TODO: simplify once CFG analysis can handle arrays
            ਜੇ (ਅਚਲ Ast_ਸੌਂਪੋ_W* ਅਚਲ ap = VN_CAST(nodep->stmtsp(), ਸੌਂਪੋ_W)) {
                ਜੇ (ap->nextp()) ਵਾਪਸ ਝੂਠ;
                // Cannot handle assignment with timing control
                ਜੇ (ap->timingControlp()) ਵਾਪਸ ਝੂਠ;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                ਅਚਲ std::unique_ptr<std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼_Var*>> oVarpsp = gatherWritten(ap);
                ਜੇ (!oVarpsp) ਵਾਪਸ ਝੂਠ;
                // Gather read variables, give up if any are not supported
                ਅਚਲ std::unique_ptr<std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼_Var*>> iVarpsp = gatherRead(ap);
                ਜੇ (!iVarpsp) ਵਾਪਸ ਝੂਠ;
                // Create the DfgLogic
                DfgLogic* ਅਚਲ logicp = ਨਵਾਂ DfgLogic{m_dfg, nodep, m_scopep, ਖਾਲੀ_ਸੂਚਕ};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                //ਪੂਰਾ
                ++m_ctx.m_representable;
                ਵਾਪਸ ਸੱਚ;
            }
        }

        // Can only handle combinational logic
        ਜੇ (nodep->sentreep()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_ਸੌਂਪੋ) {
            ਵਾਪਸ ਝੂਠ;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<Cfg_ਗ੍ਰਾਫ> cfgp = Cfg_ਗ੍ਰਾਫ::build(nodep->stmtsp());
        ਜੇ (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            ਵਾਪਸ ਝੂਠ;
        }
        // Gather written variables, give up if any are not supported
        ਅਚਲ std::unique_ptr<std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼_Var*>> oVarpsp = gatherWritten(nodep);
        ਜੇ (!oVarpsp) ਵਾਪਸ ਝੂਠ;
        // Gather read variables, give up if any are not supported
        ਅਚਲ std::unique_ptr<std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼_Var*>> iVarpsp = gather_ਜੀਵਤ(*cfgp);
        ਜੇ (!iVarpsp) ਵਾਪਸ ਝੂਠ;
        // Create the DfgLogic
        DfgLogic* ਅਚਲ logicp = ਨਵਾਂ DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        //ਪੂਰਾ
        ++m_ctx.m_representable;
        ਵਾਪਸ ਸੱਚ;
    }

    // VISITORS
    // Unhandled node
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    ਖਾਲੀ ਦੇਖੋ(AstNetlist* nodep) override { iterate_And_ਅਗਲਾ_Null(nodep->modulesp()); }
    ਖਾਲੀ ਦੇਖੋ(AstModule* nodep) override { iterate_And_ਅਗਲਾ_Null(nodep->stmtsp()); }
    ਖਾਲੀ ਦੇਖੋ(AstIface* nodep) override {
        ਜੇ (!nodep->hasVirtualRef()) {
            iterate_And_ਅਗਲਾ_Null(nodep->stmtsp());
        } ਨਹੀਂਤਾਂ {
            markReferenced(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਿਖਰ_Scope* nodep) override { iterate(nodep->scopep()); }
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਰਗਰਮ* nodep) override {
        ਜੇ (nodep->hasCombo()) {
            iterateChildren(nodep);
        } ਨਹੀਂਤਾਂ {
            markReferenced(nodep);
        }
    }

    // Non-representable constructs
    ਖਾਲੀ ਦੇਖੋ(AstCell* nodep) override { markReferenced(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Procedure* nodep) override { markReferenced(nodep); }

    // Potentially representable constructs
    ਖਾਲੀ ਦੇਖੋ(AstAlways* nodep) override {
        ਜੇ (!convert(nodep)) markReferenced(nodep);
    }

    // CONSTRUCTOR
    Ast_ਮੰਜ਼ਿਲ_Dfg_Visitor(Dfg_ਗ੍ਰਾਫ& dfg, AstNetlist& ਜੜ੍ਹ, V3_Dfg_Ast_ਮੰਜ਼ਿਲ_Dfg_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {
        iterate(&ਜੜ੍ਹ);
    }
    VL_UNCOPYABLE(Ast_ਮੰਜ਼ਿਲ_Dfg_Visitor);
    VL_UNMOVABLE(Ast_ਮੰਜ਼ਿਲ_Dfg_Visitor);

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(Dfg_ਗ੍ਰਾਫ& dfg, AstNetlist& ਜੜ੍ਹ, V3_Dfg_Ast_ਮੰਜ਼ਿਲ_Dfg_Context& ctx) {
        // Convert all logic under 'root'
        Ast_ਮੰਜ਼ਿਲ_Dfg_Visitor{dfg, ਜੜ੍ਹ, ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp : dfg.varVertices().unlinkable()) {
            ਜੇ (varp->srcp()) ਜਾਰੀ;
            ਅਚਲ ਤਾਰਕਿਕ keep = varp->foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& d) { ਵਾਪਸ !d.is<DfgAstRd>(); });
            ਜੇ (!keep) {
                ਜਦੋਂ (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<Dfg_ਗ੍ਰਾਫ> V3DfgPasses::ast_ਮੰਜ਼ਿਲ_Dfg(AstNetlist& netlist, V3DfgContext& ctx) {
    Dfg_ਗ੍ਰਾਫ* ਅਚਲ dfgp = ਨਵਾਂ Dfg_ਗ੍ਰਾਫ{ctx, "netlist"};
    Ast_ਮੰਜ਼ਿਲ_Dfg_Visitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    ਵਾਪਸ std::unique_ptr<Dfg_ਗ੍ਰਾਫ>{dfgp};
}
