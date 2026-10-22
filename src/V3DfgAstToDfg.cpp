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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Cfg.h"
#شامل "V3Const.h"
#شامل "V3Dfg.h"
#شامل "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

جماعت Ast_مقصد_Dfg_جوڑو_Ast_Refs final : عوامی VNVisitorConst {
    // STATE
    Dfg_گراف& m_dfg;  // The graph being processed
    // Function to get the DfgVertexVar for a AstVarScope
    ثابت std::function<Dfg_راس_Var*(AstVarScope*)> m_get_Var_راس;
    منطقی m_اندر_Sen_شے = جھوٹ;  // Inside an AstSenItem
    منطقی m_اندر_Loop = جھوٹ;  // Inside an AstLoop

    // VISITORS
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

    خالی دورہ(Ast_Sen_شے* nodep) override {
        VL_RESTORER(m_اندر_Sen_شے);
        m_اندر_Sen_شے = سچ;
        iterateChildrenConst(nodep);
    }

    خالی دورہ(AstLoop* nodep) override {
        VL_RESTORER(m_اندر_Loop);
        m_اندر_Loop = سچ;
        iterateChildrenConst(nodep);
    }

    خالی دورہ(AstVarRef* nodep) override {
        // Disguised hierarchical reference handled as external reference, ignore
        اگر (nodep->classOrPackagep()) واپس;
        // If target is not supported, ignore
        AstVarScope* ثابت vscp = nodep->varScopep();
        اگر (!V3Dfg::isSupported(vscp)) واپس;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        Dfg_راس_Var* ثابت varp = m_get_Var_راس(vscp);
        اگر (!varp) واپس;
        // Create Ast reference vertices
        اگر (nodep->access().isReadOnly()) {
            DfgAstRd* ثابت astp = نیا DfgAstRd{m_dfg, nodep, m_اندر_Sen_شے, m_اندر_Loop};
            astp->srcp(varp);
            واپس;
        }
        // Mark as written from non-DFG logic
        Dfg_راس_Var::سیٹ_Has_Mod_Wr_Refs(vscp);
    }

    Ast_مقصد_Dfg_جوڑو_Ast_Refs(Dfg_گراف& dfg, Ast_گرہ* nodep,
                       std::function<Dfg_راس_Var*(AstVarScope*)> get_Var_راس)
        : m_dfg{dfg}
        , m_get_Var_راس{get_Var_راس} {
        iterateConst(nodep);
    }

عوامی:
    ساکن خالی apply(Dfg_گراف& dfg, Ast_گرہ* nodep,
                      std::function<Dfg_راس_Var*(AstVarScope*)> get_Var_راس) {
        Ast_مقصد_Dfg_جوڑو_Ast_Refs{dfg, nodep, get_Var_راس};
    }
};

خالی V3DfgPasses::جوڑو_Ast_Refs(Dfg_گراف& dfg, Ast_گرہ* nodep,
                             std::function<Dfg_راس_Var*(AstVarScope*)> get_Var_راس) {
    Ast_مقصد_Dfg_جوڑو_Ast_Refs::apply(dfg, nodep, get_Var_راس);
}

جماعت Ast_مقصد_Dfg_Visitor final : عوامی VNVisitor {
    // NODE STATE
    // AstVarScope::user2() -> DfgVertexVar* : the corresponding variable vertex
    // AstVarScope::user3() -> bool : Already gathered - used fine grained below
    ثابت VNUser2_اندر_Use m_user2_اندر_Use;

    // STATE
    Dfg_گراف& m_dfg;  // The graph being built
    V3_Dfg_Ast_مقصد_Dfg_Context& m_ctx;  // The context for stats
    AstScope* m_scopep = خالی_اشاریہ;  // The current scope, iff T_Scoped

    // METHODS

    // Mark variables referenced under node
    خالی markReferenced(Ast_گرہ* nodep) {
        V3DfgPasses::جوڑو_Ast_Refs(m_dfg, nodep, [یہ](AstVarScope* vscp) {  //
            واپس get_Var_راس(vscp);
        });
    }

    Dfg_راس_Var* get_Var_راس(AstVarScope* varp) {
        اگر (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            ثابت Ast_گرہ_DType* ثابت dtypep = varp->dtypep()->skipRefp();
            Dfg_راس_Var* ثابت vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<Dfg_راس_Var*>(نیا DfgVarArray{m_dfg, varp})
                      : static_cast<Dfg_راس_Var*>(نیا DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        واپس varp->user2u().سانچہ مقصد<Dfg_راس_Var*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::سمتیہ<Dfg_راس_Var*>> gatherWritten(ثابت Ast_گرہ* nodep) {
        ثابت VNUser3_اندر_Use user3_اندر_Use;
        std::unique_ptr<std::سمتیہ<Dfg_راس_Var*>> resp{نیا std::سمتیہ<Dfg_راس_Var*>{}};
        ثابت منطقی abort = nodep->exists([&](ثابت AstVarRef* refp) -> منطقی {
            اگر (refp->access().isReadOnly()) واپس جھوٹ;
            AstVarScope* ثابت vscp = refp->varScopep();
            اگر (!V3Dfg::isSupported(vscp)) واپس سچ;
            اگر (!vscp->user3_سیٹ_Once()) resp->emplace_عقب(get_Var_راس(vscp));
            واپس جھوٹ;
        });
        اگر (abort) {
            ++m_ctx.m_nonRepVar;
            واپس خالی_اشاریہ;
        }
        واپس resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::سمتیہ<Dfg_راس_Var*>> gatherRead(ثابت Ast_گرہ* nodep) {
        ثابت VNUser3_اندر_Use user3_اندر_Use;
        std::unique_ptr<std::سمتیہ<Dfg_راس_Var*>> resp{نیا std::سمتیہ<Dfg_راس_Var*>{}};
        ثابت منطقی abort = nodep->exists([&](ثابت AstVarRef* refp) -> منطقی {
            اگر (refp->access().isWriteOnly()) واپس جھوٹ;
            AstVarScope* ثابت vscp = refp->varScopep();
            اگر (!V3Dfg::isSupported(vscp)) واپس سچ;
            اگر (!vscp->user3_سیٹ_Once()) resp->emplace_عقب(get_Var_راس(vscp));
            واپس جھوٹ;
        });
        اگر (abort) {
            ++m_ctx.m_nonRepVar;
            واپس خالی_اشاریہ;
        }
        واپس resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::سمتیہ<Dfg_راس_Var*>> gather_زندہ(ثابت Cfg_گراف& cfg) {
        // Run analysis
        std::unique_ptr<std::سمتیہ<AstVarScope*>> varps = V3Cfg::زندہ_Var_Scopes(cfg);
        اگر (!varps) {
            ++m_ctx.m_non_Rep_زندہ;
            واپس خالی_اشاریہ;
        }

        // Convert to vertics
        ثابت VNUser3_اندر_Use user3_اندر_Use;
        std::unique_ptr<std::سمتیہ<Dfg_راس_Var*>> resp{نیا std::سمتیہ<Dfg_راس_Var*>{}};
        resp->reserve(varps->سائز());
        برائے (AstVarScope* ثابت varp : *varps) {
            اگر (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                واپس خالی_اشاریہ;
            }
            UASSERT_OBJ(!varp->user3_سیٹ_Once(), varp, "Live variables should be unique");
            resp->emplace_عقب(get_Var_راس(varp));
        }
        واپس resp;
    }

    // Connect inputs and outputs of a DfgLogic
    خالی connect(DfgLogic& vtx, ثابت std::سمتیہ<Dfg_راس_Var*>& iVarps,
                 ثابت std::سمتیہ<Dfg_راس_Var*>& oVarps) {
        // Connect inputs
        برائے (Dfg_راس_Var* ثابت iVarp : iVarps) vtx.جوڑو_داخلہ(iVarp);
        // Connect outputs
        برائے (Dfg_راس_Var* ثابت oVarp : oVarps) {
            اگر (!oVarp->srcp()) oVarp->srcp(نیا DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->جوڑو_Driver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    منطقی convert(AstAlways* nodep) {
        ثابت VAlwaysKwd kwd = nodep->keyword();
        اگر (kwd == VAlwaysKwd::CONT_تفویض) {
            // TODO: simplify once CFG analysis can handle arrays
            اگر (ثابت Ast_تفویض_W* ثابت ap = VN_CAST(nodep->stmtsp(), تفویض_W)) {
                اگر (ap->nextp()) واپس جھوٹ;
                // Cannot handle assignment with timing control
                اگر (ap->timingControlp()) واپس جھوٹ;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                ثابت std::unique_ptr<std::سمتیہ<Dfg_راس_Var*>> oVarpsp = gatherWritten(ap);
                اگر (!oVarpsp) واپس جھوٹ;
                // Gather read variables, give up if any are not supported
                ثابت std::unique_ptr<std::سمتیہ<Dfg_راس_Var*>> iVarpsp = gatherRead(ap);
                اگر (!iVarpsp) واپس جھوٹ;
                // Create the DfgLogic
                DfgLogic* ثابت logicp = نیا DfgLogic{m_dfg, nodep, m_scopep, خالی_اشاریہ};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                //مکمل
                ++m_ctx.m_representable;
                واپس سچ;
            }
        }

        // Can only handle combinational logic
        اگر (nodep->sentreep()) واپس جھوٹ;
        اگر (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_تفویض) {
            واپس جھوٹ;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<Cfg_گراف> cfgp = Cfg_گراف::build(nodep->stmtsp());
        اگر (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            واپس جھوٹ;
        }
        // Gather written variables, give up if any are not supported
        ثابت std::unique_ptr<std::سمتیہ<Dfg_راس_Var*>> oVarpsp = gatherWritten(nodep);
        اگر (!oVarpsp) واپس جھوٹ;
        // Gather read variables, give up if any are not supported
        ثابت std::unique_ptr<std::سمتیہ<Dfg_راس_Var*>> iVarpsp = gather_زندہ(*cfgp);
        اگر (!iVarpsp) واپس جھوٹ;
        // Create the DfgLogic
        DfgLogic* ثابت logicp = نیا DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        //مکمل
        ++m_ctx.m_representable;
        واپس سچ;
    }

    // VISITORS
    // Unhandled node
    خالی دورہ(Ast_گرہ* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    خالی دورہ(AstNetlist* nodep) override { iterate_And_اگلا_Null(nodep->modulesp()); }
    خالی دورہ(AstModule* nodep) override { iterate_And_اگلا_Null(nodep->stmtsp()); }
    خالی دورہ(AstIface* nodep) override {
        اگر (!nodep->hasVirtualRef()) {
            iterate_And_اگلا_Null(nodep->stmtsp());
        } ورنہ {
            markReferenced(nodep);
        }
    }
    خالی دورہ(Ast_چوٹی_Scope* nodep) override { iterate(nodep->scopep()); }
    خالی دورہ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_فعال* nodep) override {
        اگر (nodep->hasCombo()) {
            iterateChildren(nodep);
        } ورنہ {
            markReferenced(nodep);
        }
    }

    // Non-representable constructs
    خالی دورہ(AstCell* nodep) override { markReferenced(nodep); }
    خالی دورہ(Ast_گرہ_Procedure* nodep) override { markReferenced(nodep); }

    // Potentially representable constructs
    خالی دورہ(AstAlways* nodep) override {
        اگر (!convert(nodep)) markReferenced(nodep);
    }

    // CONSTRUCTOR
    Ast_مقصد_Dfg_Visitor(Dfg_گراف& dfg, AstNetlist& جڑ, V3_Dfg_Ast_مقصد_Dfg_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {
        iterate(&جڑ);
    }
    VL_UNCOPYABLE(Ast_مقصد_Dfg_Visitor);
    VL_UNMOVABLE(Ast_مقصد_Dfg_Visitor);

عوامی:
    ساکن خالی apply(Dfg_گراف& dfg, AstNetlist& جڑ, V3_Dfg_Ast_مقصد_Dfg_Context& ctx) {
        // Convert all logic under 'root'
        Ast_مقصد_Dfg_Visitor{dfg, جڑ, ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        برائے (Dfg_راس_Var* ثابت varp : dfg.varVertices().unlinkable()) {
            اگر (varp->srcp()) جاری;
            ثابت منطقی keep = varp->foreachSink([&](Dfg_راس& d) { واپس !d.is<DfgAstRd>(); });
            اگر (!keep) {
                جب (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<Dfg_گراف> V3DfgPasses::ast_مقصد_Dfg(AstNetlist& netlist, V3DfgContext& ctx) {
    Dfg_گراف* ثابت dfgp = نیا Dfg_گراف{ctx, "netlist"};
    Ast_مقصد_Dfg_Visitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    واپس std::unique_ptr<Dfg_گراف>{dfgp};
}
