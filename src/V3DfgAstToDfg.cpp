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

صنف Ast_مقصد_Dfg_جمع_کن_Ast_Refs final : عام VNVisitorConst {
    // STATE
    Dfg_گراف& m_dfg;  // The graph being processed
    // Function to get the DfgVertexVar for a AstVarScope
    ثابت std::function<Dfg_رأس_Var*(AstVarScope*)> m_get_Var_رأس;
    منطقي m_درون_Sen_قلم = خطأ;  // Inside an AstSenItem
    منطقي m_درون_Loop = خطأ;  // Inside an AstLoop

    // VISITORS
    فراغ بازدید(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

    فراغ بازدید(Ast_Sen_قلم* nodep) override {
        VL_RESTORER(m_درون_Sen_قلم);
        m_درون_Sen_قلم = درست;
        iterateChildrenConst(nodep);
    }

    فراغ بازدید(AstLoop* nodep) override {
        VL_RESTORER(m_درون_Loop);
        m_درون_Loop = درست;
        iterateChildrenConst(nodep);
    }

    فراغ بازدید(AstVarRef* nodep) override {
        // Disguised hierarchical reference handled as external reference, ignore
        إذا (nodep->classOrPackagep()) أرجع;
        // If target is not supported, ignore
        AstVarScope* ثابت vscp = nodep->varScopep();
        إذا (!V3Dfg::isSupported(vscp)) أرجع;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        Dfg_رأس_Var* ثابت varp = m_get_Var_رأس(vscp);
        إذا (!varp) أرجع;
        // Create Ast reference vertices
        إذا (nodep->access().isReadOnly()) {
            DfgAstRd* ثابت astp = جديد DfgAstRd{m_dfg, nodep, m_درون_Sen_قلم, m_درون_Loop};
            astp->srcp(varp);
            أرجع;
        }
        // Mark as written from non-DFG logic
        Dfg_رأس_Var::مجموعه_Has_Mod_Wr_Refs(vscp);
    }

    Ast_مقصد_Dfg_جمع_کن_Ast_Refs(Dfg_گراف& dfg, Ast_گره* nodep,
                       std::function<Dfg_رأس_Var*(AstVarScope*)> get_Var_رأس)
        : m_dfg{dfg}
        , m_get_Var_رأس{get_Var_رأس} {
        iterateConst(nodep);
    }

عام:
    ساكن فراغ apply(Dfg_گراف& dfg, Ast_گره* nodep,
                      std::function<Dfg_رأس_Var*(AstVarScope*)> get_Var_رأس) {
        Ast_مقصد_Dfg_جمع_کن_Ast_Refs{dfg, nodep, get_Var_رأس};
    }
};

فراغ V3DfgPasses::جمع_کن_Ast_Refs(Dfg_گراف& dfg, Ast_گره* nodep,
                             std::function<Dfg_رأس_Var*(AstVarScope*)> get_Var_رأس) {
    Ast_مقصد_Dfg_جمع_کن_Ast_Refs::apply(dfg, nodep, get_Var_رأس);
}

صنف Ast_مقصد_Dfg_Visitor final : عام VNVisitor {
    // NODE STATE
    // AstVarScope::user2() -> DfgVertexVar* : the corresponding variable vertex
    // AstVarScope::user3() -> bool : Already gathered - used fine grained below
    ثابت VNUser2_درون_Use m_user2_درون_Use;

    // STATE
    Dfg_گراف& m_dfg;  // The graph being built
    V3_Dfg_Ast_مقصد_Dfg_Context& m_ctx;  // The context for stats
    AstScope* m_scopep = اشاره_تهی;  // The current scope, iff T_Scoped

    // METHODS

    // Mark variables referenced under node
    فراغ markReferenced(Ast_گره* nodep) {
        V3DfgPasses::جمع_کن_Ast_Refs(m_dfg, nodep, [هذا](AstVarScope* vscp) {  //
            أرجع get_Var_رأس(vscp);
        });
    }

    Dfg_رأس_Var* get_Var_رأس(AstVarScope* varp) {
        إذا (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            ثابت Ast_گره_DType* ثابت dtypep = varp->dtypep()->skipRefp();
            Dfg_رأس_Var* ثابت vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<Dfg_رأس_Var*>(جديد DfgVarArray{m_dfg, varp})
                      : static_cast<Dfg_رأس_Var*>(جديد DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        أرجع varp->user2u().قالب مقصد<Dfg_رأس_Var*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::وکتور<Dfg_رأس_Var*>> gatherWritten(ثابت Ast_گره* nodep) {
        ثابت VNUser3_درون_Use user3_درون_Use;
        std::unique_ptr<std::وکتور<Dfg_رأس_Var*>> resp{جديد std::وکتور<Dfg_رأس_Var*>{}};
        ثابت منطقي abort = nodep->exists([&](ثابت AstVarRef* refp) -> منطقي {
            إذا (refp->access().isReadOnly()) أرجع خطأ;
            AstVarScope* ثابت vscp = refp->varScopep();
            إذا (!V3Dfg::isSupported(vscp)) أرجع درست;
            إذا (!vscp->user3_مجموعه_Once()) resp->emplace_پشت(get_Var_رأس(vscp));
            أرجع خطأ;
        });
        إذا (abort) {
            ++m_ctx.m_nonRepVar;
            أرجع اشاره_تهی;
        }
        أرجع resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::وکتور<Dfg_رأس_Var*>> gatherRead(ثابت Ast_گره* nodep) {
        ثابت VNUser3_درون_Use user3_درون_Use;
        std::unique_ptr<std::وکتور<Dfg_رأس_Var*>> resp{جديد std::وکتور<Dfg_رأس_Var*>{}};
        ثابت منطقي abort = nodep->exists([&](ثابت AstVarRef* refp) -> منطقي {
            إذا (refp->access().isWriteOnly()) أرجع خطأ;
            AstVarScope* ثابت vscp = refp->varScopep();
            إذا (!V3Dfg::isSupported(vscp)) أرجع درست;
            إذا (!vscp->user3_مجموعه_Once()) resp->emplace_پشت(get_Var_رأس(vscp));
            أرجع خطأ;
        });
        إذا (abort) {
            ++m_ctx.m_nonRepVar;
            أرجع اشاره_تهی;
        }
        أرجع resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::وکتور<Dfg_رأس_Var*>> gather_زنده(ثابت Cfg_گراف& cfg) {
        // Run analysis
        std::unique_ptr<std::وکتور<AstVarScope*>> varps = V3Cfg::زنده_Var_Scopes(cfg);
        إذا (!varps) {
            ++m_ctx.m_non_Rep_زنده;
            أرجع اشاره_تهی;
        }

        // Convert to vertics
        ثابت VNUser3_درون_Use user3_درون_Use;
        std::unique_ptr<std::وکتور<Dfg_رأس_Var*>> resp{جديد std::وکتور<Dfg_رأس_Var*>{}};
        resp->reserve(varps->اندازه());
        لكل (AstVarScope* ثابت varp : *varps) {
            إذا (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                أرجع اشاره_تهی;
            }
            UASSERT_OBJ(!varp->user3_مجموعه_Once(), varp, "Live variables should be unique");
            resp->emplace_پشت(get_Var_رأس(varp));
        }
        أرجع resp;
    }

    // Connect inputs and outputs of a DfgLogic
    فراغ connect(DfgLogic& vtx, ثابت std::وکتور<Dfg_رأس_Var*>& iVarps,
                 ثابت std::وکتور<Dfg_رأس_Var*>& oVarps) {
        // Connect inputs
        لكل (Dfg_رأس_Var* ثابت iVarp : iVarps) vtx.جمع_کن_درون_داد(iVarp);
        // Connect outputs
        لكل (Dfg_رأس_Var* ثابت oVarp : oVarps) {
            إذا (!oVarp->srcp()) oVarp->srcp(جديد DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->جمع_کن_Driver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    منطقي convert(AstAlways* nodep) {
        ثابت VAlwaysKwd kwd = nodep->keyword();
        إذا (kwd == VAlwaysKwd::CONT_تخصیص_بده) {
            // TODO: simplify once CFG analysis can handle arrays
            إذا (ثابت Ast_تخصیص_بده_W* ثابت ap = VN_CAST(nodep->stmtsp(), تخصیص_بده_W)) {
                إذا (ap->nextp()) أرجع خطأ;
                // Cannot handle assignment with timing control
                إذا (ap->timingControlp()) أرجع خطأ;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                ثابت std::unique_ptr<std::وکتور<Dfg_رأس_Var*>> oVarpsp = gatherWritten(ap);
                إذا (!oVarpsp) أرجع خطأ;
                // Gather read variables, give up if any are not supported
                ثابت std::unique_ptr<std::وکتور<Dfg_رأس_Var*>> iVarpsp = gatherRead(ap);
                إذا (!iVarpsp) أرجع خطأ;
                // Create the DfgLogic
                DfgLogic* ثابت logicp = جديد DfgLogic{m_dfg, nodep, m_scopep, اشاره_تهی};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                //انجامشده
                ++m_ctx.m_representable;
                أرجع درست;
            }
        }

        // Can only handle combinational logic
        إذا (nodep->sentreep()) أرجع خطأ;
        إذا (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_تخصیص_بده) {
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
        ثابت std::unique_ptr<std::وکتور<Dfg_رأس_Var*>> oVarpsp = gatherWritten(nodep);
        إذا (!oVarpsp) أرجع خطأ;
        // Gather read variables, give up if any are not supported
        ثابت std::unique_ptr<std::وکتور<Dfg_رأس_Var*>> iVarpsp = gather_زنده(*cfgp);
        إذا (!iVarpsp) أرجع خطأ;
        // Create the DfgLogic
        DfgLogic* ثابت logicp = جديد DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        //انجامشده
        ++m_ctx.m_representable;
        أرجع درست;
    }

    // VISITORS
    // Unhandled node
    فراغ بازدید(Ast_گره* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    فراغ بازدید(AstNetlist* nodep) override { iterate_And_بعدی_Null(nodep->modulesp()); }
    فراغ بازدید(AstModule* nodep) override { iterate_And_بعدی_Null(nodep->stmtsp()); }
    فراغ بازدید(AstIface* nodep) override {
        إذا (!nodep->hasVirtualRef()) {
            iterate_And_بعدی_Null(nodep->stmtsp());
        } وإلا {
            markReferenced(nodep);
        }
    }
    فراغ بازدید(Ast_بالاترین_Scope* nodep) override { iterate(nodep->scopep()); }
    فراغ بازدید(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_فعال* nodep) override {
        إذا (nodep->hasCombo()) {
            iterateChildren(nodep);
        } وإلا {
            markReferenced(nodep);
        }
    }

    // Non-representable constructs
    فراغ بازدید(AstCell* nodep) override { markReferenced(nodep); }
    فراغ بازدید(Ast_گره_Procedure* nodep) override { markReferenced(nodep); }

    // Potentially representable constructs
    فراغ بازدید(AstAlways* nodep) override {
        إذا (!convert(nodep)) markReferenced(nodep);
    }

    // CONSTRUCTOR
    Ast_مقصد_Dfg_Visitor(Dfg_گراف& dfg, AstNetlist& ریشه, V3_Dfg_Ast_مقصد_Dfg_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {
        iterate(&ریشه);
    }
    VL_UNCOPYABLE(Ast_مقصد_Dfg_Visitor);
    VL_UNMOVABLE(Ast_مقصد_Dfg_Visitor);

عام:
    ساكن فراغ apply(Dfg_گراف& dfg, AstNetlist& ریشه, V3_Dfg_Ast_مقصد_Dfg_Context& ctx) {
        // Convert all logic under 'root'
        Ast_مقصد_Dfg_Visitor{dfg, ریشه, ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        لكل (Dfg_رأس_Var* ثابت varp : dfg.varVertices().unlinkable()) {
            إذا (varp->srcp()) استمر;
            ثابت منطقي keep = varp->foreachSink([&](Dfg_رأس& d) { أرجع !d.is<DfgAstRd>(); });
            إذا (!keep) {
                طالما (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<Dfg_گراف> V3DfgPasses::ast_مقصد_Dfg(AstNetlist& netlist, V3DfgContext& ctx) {
    Dfg_گراف* ثابت dfgp = جديد Dfg_گراف{ctx, "netlist"};
    Ast_مقصد_Dfg_Visitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    أرجع std::unique_ptr<Dfg_گراف>{dfgp};
}
