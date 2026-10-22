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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Cfg.h"
#ضمن "V3Const.h"
#ضمن "V3Dfg.h"
#ضمن "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

صنف Ast_منزل_Dfg_جوڑو_Ast_Refs final : عام VNVisitorConst {
    // STATE
    Dfg_گراف& m_dfg;  // The graph being processed
    // Function to get the DfgVertexVar for a AstVarScope
    ثابت std::function<Dfg_راس_Var*(AstVarScope*)> m_get_Var_راس;
    منطقي m_وچ_Sen_شے = خطأ;  // Inside an AstSenItem
    منطقي m_وچ_Loop = خطأ;  // Inside an AstLoop

    // VISITORS
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

    فراغ ویکھو(Ast_Sen_شے* nodep) override {
        VL_RESTORER(m_وچ_Sen_شے);
        m_وچ_Sen_شے = سچ;
        iterateChildrenConst(nodep);
    }

    فراغ ویکھو(AstLoop* nodep) override {
        VL_RESTORER(m_وچ_Loop);
        m_وچ_Loop = سچ;
        iterateChildrenConst(nodep);
    }

    فراغ ویکھو(AstVarRef* nodep) override {
        // Disguised hierarchical reference handled as external reference, ignore
        إذا (nodep->classOrPackagep()) أرجع;
        // If target is not supported, ignore
        AstVarScope* ثابت vscp = nodep->varScopep();
        إذا (!V3Dfg::isSupported(vscp)) أرجع;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        Dfg_راس_Var* ثابت varp = m_get_Var_راس(vscp);
        إذا (!varp) أرجع;
        // Create Ast reference vertices
        إذا (nodep->access().isReadOnly()) {
            DfgAstRd* ثابت astp = جديد DfgAstRd{m_dfg, nodep, m_وچ_Sen_شے, m_وچ_Loop};
            astp->srcp(varp);
            أرجع;
        }
        // Mark as written from non-DFG logic
        Dfg_راس_Var::سیٹ_Has_Mod_Wr_Refs(vscp);
    }

    Ast_منزل_Dfg_جوڑو_Ast_Refs(Dfg_گراف& dfg, Ast_گرہ* nodep,
                       std::function<Dfg_راس_Var*(AstVarScope*)> get_Var_راس)
        : m_dfg{dfg}
        , m_get_Var_راس{get_Var_راس} {
        iterateConst(nodep);
    }

عام:
    ساكن فراغ apply(Dfg_گراف& dfg, Ast_گرہ* nodep,
                      std::function<Dfg_راس_Var*(AstVarScope*)> get_Var_راس) {
        Ast_منزل_Dfg_جوڑو_Ast_Refs{dfg, nodep, get_Var_راس};
    }
};

فراغ V3DfgPasses::جوڑو_Ast_Refs(Dfg_گراف& dfg, Ast_گرہ* nodep,
                             std::function<Dfg_راس_Var*(AstVarScope*)> get_Var_راس) {
    Ast_منزل_Dfg_جوڑو_Ast_Refs::apply(dfg, nodep, get_Var_راس);
}

صنف Ast_منزل_Dfg_Visitor final : عام VNVisitor {
    // NODE STATE
    // AstVarScope::user2() -> DfgVertexVar* : the corresponding variable vertex
    // AstVarScope::user3() -> bool : Already gathered - used fine grained below
    ثابت VNUser2_وچ_Use m_user2_وچ_Use;

    // STATE
    Dfg_گراف& m_dfg;  // The graph being built
    V3_Dfg_Ast_منزل_Dfg_Context& m_ctx;  // The context for stats
    AstScope* m_scopep = خالی_سوچک;  // The current scope, iff T_Scoped

    // METHODS

    // Mark variables referenced under node
    فراغ markReferenced(Ast_گرہ* nodep) {
        V3DfgPasses::جوڑو_Ast_Refs(m_dfg, nodep, [هذا](AstVarScope* vscp) {  //
            أرجع get_Var_راس(vscp);
        });
    }

    Dfg_راس_Var* get_Var_راس(AstVarScope* varp) {
        إذا (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            ثابت Ast_گرہ_DType* ثابت dtypep = varp->dtypep()->skipRefp();
            Dfg_راس_Var* ثابت vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<Dfg_راس_Var*>(جديد DfgVarArray{m_dfg, varp})
                      : static_cast<Dfg_راس_Var*>(جديد DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        أرجع varp->user2u().سانچا منزل<Dfg_راس_Var*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::سمتیہ<Dfg_راس_Var*>> gatherWritten(ثابت Ast_گرہ* nodep) {
        ثابت VNUser3_وچ_Use user3_وچ_Use;
        std::unique_ptr<std::سمتیہ<Dfg_راس_Var*>> resp{جديد std::سمتیہ<Dfg_راس_Var*>{}};
        ثابت منطقي abort = nodep->exists([&](ثابت AstVarRef* refp) -> منطقي {
            إذا (refp->access().isReadOnly()) أرجع خطأ;
            AstVarScope* ثابت vscp = refp->varScopep();
            إذا (!V3Dfg::isSupported(vscp)) أرجع سچ;
            إذا (!vscp->user3_سیٹ_Once()) resp->emplace_پٹھ(get_Var_راس(vscp));
            أرجع خطأ;
        });
        إذا (abort) {
            ++m_ctx.m_nonRepVar;
            أرجع خالی_سوچک;
        }
        أرجع resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::سمتیہ<Dfg_راس_Var*>> gatherRead(ثابت Ast_گرہ* nodep) {
        ثابت VNUser3_وچ_Use user3_وچ_Use;
        std::unique_ptr<std::سمتیہ<Dfg_راس_Var*>> resp{جديد std::سمتیہ<Dfg_راس_Var*>{}};
        ثابت منطقي abort = nodep->exists([&](ثابت AstVarRef* refp) -> منطقي {
            إذا (refp->access().isWriteOnly()) أرجع خطأ;
            AstVarScope* ثابت vscp = refp->varScopep();
            إذا (!V3Dfg::isSupported(vscp)) أرجع سچ;
            إذا (!vscp->user3_سیٹ_Once()) resp->emplace_پٹھ(get_Var_راس(vscp));
            أرجع خطأ;
        });
        إذا (abort) {
            ++m_ctx.m_nonRepVar;
            أرجع خالی_سوچک;
        }
        أرجع resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::سمتیہ<Dfg_راس_Var*>> gather_جیوندا(ثابت Cfg_گراف& cfg) {
        // Run analysis
        std::unique_ptr<std::سمتیہ<AstVarScope*>> varps = V3Cfg::جیوندا_Var_Scopes(cfg);
        إذا (!varps) {
            ++m_ctx.m_non_Rep_جیوندا;
            أرجع خالی_سوچک;
        }

        // Convert to vertics
        ثابت VNUser3_وچ_Use user3_وچ_Use;
        std::unique_ptr<std::سمتیہ<Dfg_راس_Var*>> resp{جديد std::سمتیہ<Dfg_راس_Var*>{}};
        resp->reserve(varps->ناپ());
        لكل (AstVarScope* ثابت varp : *varps) {
            إذا (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                أرجع خالی_سوچک;
            }
            UASSERT_OBJ(!varp->user3_سیٹ_Once(), varp, "Live variables should be unique");
            resp->emplace_پٹھ(get_Var_راس(varp));
        }
        أرجع resp;
    }

    // Connect inputs and outputs of a DfgLogic
    فراغ connect(DfgLogic& vtx, ثابت std::سمتیہ<Dfg_راس_Var*>& iVarps,
                 ثابت std::سمتیہ<Dfg_راس_Var*>& oVarps) {
        // Connect inputs
        لكل (Dfg_راس_Var* ثابت iVarp : iVarps) vtx.جوڑو_داخلہ(iVarp);
        // Connect outputs
        لكل (Dfg_راس_Var* ثابت oVarp : oVarps) {
            إذا (!oVarp->srcp()) oVarp->srcp(جديد DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->جوڑو_Driver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    منطقي convert(AstAlways* nodep) {
        ثابت VAlwaysKwd kwd = nodep->keyword();
        إذا (kwd == VAlwaysKwd::CONT_سونپو) {
            // TODO: simplify once CFG analysis can handle arrays
            إذا (ثابت Ast_سونپو_W* ثابت ap = VN_CAST(nodep->stmtsp(), سونپو_W)) {
                إذا (ap->nextp()) أرجع خطأ;
                // Cannot handle assignment with timing control
                إذا (ap->timingControlp()) أرجع خطأ;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                ثابت std::unique_ptr<std::سمتیہ<Dfg_راس_Var*>> oVarpsp = gatherWritten(ap);
                إذا (!oVarpsp) أرجع خطأ;
                // Gather read variables, give up if any are not supported
                ثابت std::unique_ptr<std::سمتیہ<Dfg_راس_Var*>> iVarpsp = gatherRead(ap);
                إذا (!iVarpsp) أرجع خطأ;
                // Create the DfgLogic
                DfgLogic* ثابت logicp = جديد DfgLogic{m_dfg, nodep, m_scopep, خالی_سوچک};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                //پورا
                ++m_ctx.m_representable;
                أرجع سچ;
            }
        }

        // Can only handle combinational logic
        إذا (nodep->sentreep()) أرجع خطأ;
        إذا (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_سونپو) {
            أرجع خطأ;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<Cfg_گراف> cfgp = Cfg_گراف::build(nodep->stmtsp());
        إذا (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            أرجع خطأ;
        }
        // Gather written variables, give up if any are not supported
        ثابت std::unique_ptr<std::سمتیہ<Dfg_راس_Var*>> oVarpsp = gatherWritten(nodep);
        إذا (!oVarpsp) أرجع خطأ;
        // Gather read variables, give up if any are not supported
        ثابت std::unique_ptr<std::سمتیہ<Dfg_راس_Var*>> iVarpsp = gather_جیوندا(*cfgp);
        إذا (!iVarpsp) أرجع خطأ;
        // Create the DfgLogic
        DfgLogic* ثابت logicp = جديد DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        //پورا
        ++m_ctx.m_representable;
        أرجع سچ;
    }

    // VISITORS
    // Unhandled node
    فراغ ویکھو(Ast_گرہ* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    فراغ ویکھو(AstNetlist* nodep) override { iterate_And_اگلا_Null(nodep->modulesp()); }
    فراغ ویکھو(AstModule* nodep) override { iterate_And_اگلا_Null(nodep->stmtsp()); }
    فراغ ویکھو(AstIface* nodep) override {
        إذا (!nodep->hasVirtualRef()) {
            iterate_And_اگلا_Null(nodep->stmtsp());
        } وإلا {
            markReferenced(nodep);
        }
    }
    فراغ ویکھو(Ast_چوٹی_Scope* nodep) override { iterate(nodep->scopep()); }
    فراغ ویکھو(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_سرگرم* nodep) override {
        إذا (nodep->hasCombo()) {
            iterateChildren(nodep);
        } وإلا {
            markReferenced(nodep);
        }
    }

    // Non-representable constructs
    فراغ ویکھو(AstCell* nodep) override { markReferenced(nodep); }
    فراغ ویکھو(Ast_گرہ_Procedure* nodep) override { markReferenced(nodep); }

    // Potentially representable constructs
    فراغ ویکھو(AstAlways* nodep) override {
        إذا (!convert(nodep)) markReferenced(nodep);
    }

    // CONSTRUCTOR
    Ast_منزل_Dfg_Visitor(Dfg_گراف& dfg, AstNetlist& جڑ, V3_Dfg_Ast_منزل_Dfg_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {
        iterate(&جڑ);
    }
    VL_UNCOPYABLE(Ast_منزل_Dfg_Visitor);
    VL_UNMOVABLE(Ast_منزل_Dfg_Visitor);

عام:
    ساكن فراغ apply(Dfg_گراف& dfg, AstNetlist& جڑ, V3_Dfg_Ast_منزل_Dfg_Context& ctx) {
        // Convert all logic under 'root'
        Ast_منزل_Dfg_Visitor{dfg, جڑ, ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        لكل (Dfg_راس_Var* ثابت varp : dfg.varVertices().unlinkable()) {
            إذا (varp->srcp()) استمر;
            ثابت منطقي keep = varp->foreachSink([&](Dfg_راس& d) { أرجع !d.is<DfgAstRd>(); });
            إذا (!keep) {
                طالما (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<Dfg_گراف> V3DfgPasses::ast_منزل_Dfg(AstNetlist& netlist, V3DfgContext& ctx) {
    Dfg_گراف* ثابت dfgp = جديد Dfg_گراف{ctx, "netlist"};
    Ast_منزل_Dfg_Visitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    أرجع std::unique_ptr<Dfg_گراف>{dfgp};
}
