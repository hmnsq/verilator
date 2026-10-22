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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Cfg.h"
#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3Dfg.h"
#ಅನ್ತರ್ಭಾವ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ವರ್ಗ Ast_ಗಮ್ಯ_Dfg_ಕೂಡಿಸು_Ast_Refs final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // STATE
    Dfg_ನಕ್ಷೆ& m_dfg;  // The graph being processed
    // Function to get the DfgVertexVar for a AstVarScope
    ಸ್ಥಿರಾಂಕ std::function<Dfg_ಶೃಂಗ_Var*(AstVarScope*)> m_get_Var_ಶೃಂಗ;
    ತಾರ್ಕಿಕ m_ಒಳ_Sen_ಅಂಶ = ಸುಳ್ಳು;  // Inside an AstSenItem
    ತಾರ್ಕಿಕ m_ಒಳ_Loop = ಸುಳ್ಳು;  // Inside an AstLoop

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sen_ಅಂಶ* nodep) override {
        VL_RESTORER(m_ಒಳ_Sen_ಅಂಶ);
        m_ಒಳ_Sen_ಅಂಶ = ನಿಜ;
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstLoop* nodep) override {
        VL_RESTORER(m_ಒಳ_Loop);
        m_ಒಳ_Loop = ನಿಜ;
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        // Disguised hierarchical reference handled as external reference, ignore
        ಆದರೆ (nodep->classOrPackagep()) ಹಿಂತಿರುಗಿ;
        // If target is not supported, ignore
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = nodep->varScopep();
        ಆದರೆ (!V3Dfg::isSupported(vscp)) ಹಿಂತಿರುಗಿ;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = m_get_Var_ಶೃಂಗ(vscp);
        ಆದರೆ (!varp) ಹಿಂತಿರುಗಿ;
        // Create Ast reference vertices
        ಆದರೆ (nodep->access().isReadOnly()) {
            DfgAstRd* ಸ್ಥಿರಾಂಕ astp = ಹೊಸ DfgAstRd{m_dfg, nodep, m_ಒಳ_Sen_ಅಂಶ, m_ಒಳ_Loop};
            astp->srcp(varp);
            ಹಿಂತಿರುಗಿ;
        }
        // Mark as written from non-DFG logic
        Dfg_ಶೃಂಗ_Var::ಗಣ_Has_Mod_Wr_Refs(vscp);
    }

    Ast_ಗಮ್ಯ_Dfg_ಕೂಡಿಸು_Ast_Refs(Dfg_ನಕ್ಷೆ& dfg, Ast_ಗಂಟು* nodep,
                       std::function<Dfg_ಶೃಂಗ_Var*(AstVarScope*)> get_Var_ಶೃಂಗ)
        : m_dfg{dfg}
        , m_get_Var_ಶೃಂಗ{get_Var_ಶೃಂಗ} {
        iterateConst(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(Dfg_ನಕ್ಷೆ& dfg, Ast_ಗಂಟು* nodep,
                      std::function<Dfg_ಶೃಂಗ_Var*(AstVarScope*)> get_Var_ಶೃಂಗ) {
        Ast_ಗಮ್ಯ_Dfg_ಕೂಡಿಸು_Ast_Refs{dfg, nodep, get_Var_ಶೃಂಗ};
    }
};

ಶೂನ್ಯ V3DfgPasses::ಕೂಡಿಸು_Ast_Refs(Dfg_ನಕ್ಷೆ& dfg, Ast_ಗಂಟು* nodep,
                             std::function<Dfg_ಶೃಂಗ_Var*(AstVarScope*)> get_Var_ಶೃಂಗ) {
    Ast_ಗಮ್ಯ_Dfg_ಕೂಡಿಸು_Ast_Refs::apply(dfg, nodep, get_Var_ಶೃಂಗ);
}

ವರ್ಗ Ast_ಗಮ್ಯ_Dfg_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // AstVarScope::user2() -> DfgVertexVar* : the corresponding variable vertex
    // AstVarScope::user3() -> bool : Already gathered - used fine grained below
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_user2_ಒಳ_Use;

    // STATE
    Dfg_ನಕ್ಷೆ& m_dfg;  // The graph being built
    V3_Dfg_Ast_ಗಮ್ಯ_Dfg_Context& m_ctx;  // The context for stats
    AstScope* m_scopep = ಶೂನ್ಯಸೂಚಕ;  // The current scope, iff T_Scoped

    // METHODS

    // Mark variables referenced under node
    ಶೂನ್ಯ markReferenced(Ast_ಗಂಟು* nodep) {
        V3DfgPasses::ಕೂಡಿಸು_Ast_Refs(m_dfg, nodep, [ಇದು](AstVarScope* vscp) {  //
            ಹಿಂತಿರುಗಿ get_Var_ಶೃಂಗ(vscp);
        });
    }

    Dfg_ಶೃಂಗ_Var* get_Var_ಶೃಂಗ(AstVarScope* varp) {
        ಆದರೆ (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = varp->dtypep()->skipRefp();
            Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<Dfg_ಶೃಂಗ_Var*>(ಹೊಸ DfgVarArray{m_dfg, varp})
                      : static_cast<Dfg_ಶೃಂಗ_Var*>(ಹೊಸ DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        ಹಿಂತಿರುಗಿ varp->user2u().ಮಾದರಿ ಗಮ್ಯ<Dfg_ಶೃಂಗ_Var*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::ಸದಿಶ<Dfg_ಶೃಂಗ_Var*>> gatherWritten(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use user3_ಒಳ_Use;
        std::unique_ptr<std::ಸದಿಶ<Dfg_ಶೃಂಗ_Var*>> resp{ಹೊಸ std::ಸದಿಶ<Dfg_ಶೃಂಗ_Var*>{}};
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ abort = nodep->exists([&](ಸ್ಥಿರಾಂಕ AstVarRef* refp) -> ತಾರ್ಕಿಕ {
            ಆದರೆ (refp->access().isReadOnly()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = refp->varScopep();
            ಆದರೆ (!V3Dfg::isSupported(vscp)) ಹಿಂತಿರುಗಿ ನಿಜ;
            ಆದರೆ (!vscp->user3_ಗಣ_Once()) resp->emplace_ಹಿಂಭಾಗ(get_Var_ಶೃಂಗ(vscp));
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
        ಆದರೆ (abort) {
            ++m_ctx.m_nonRepVar;
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        ಹಿಂತಿರುಗಿ resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::ಸದಿಶ<Dfg_ಶೃಂಗ_Var*>> gatherRead(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use user3_ಒಳ_Use;
        std::unique_ptr<std::ಸದಿಶ<Dfg_ಶೃಂಗ_Var*>> resp{ಹೊಸ std::ಸದಿಶ<Dfg_ಶೃಂಗ_Var*>{}};
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ abort = nodep->exists([&](ಸ್ಥಿರಾಂಕ AstVarRef* refp) -> ತಾರ್ಕಿಕ {
            ಆದರೆ (refp->access().isWriteOnly()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = refp->varScopep();
            ಆದರೆ (!V3Dfg::isSupported(vscp)) ಹಿಂತಿರುಗಿ ನಿಜ;
            ಆದರೆ (!vscp->user3_ಗಣ_Once()) resp->emplace_ಹಿಂಭಾಗ(get_Var_ಶೃಂಗ(vscp));
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
        ಆದರೆ (abort) {
            ++m_ctx.m_nonRepVar;
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        ಹಿಂತಿರುಗಿ resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::ಸದಿಶ<Dfg_ಶೃಂಗ_Var*>> gather_ಜೀವಂತ(ಸ್ಥಿರಾಂಕ Cfg_ನಕ್ಷೆ& cfg) {
        // Run analysis
        std::unique_ptr<std::ಸದಿಶ<AstVarScope*>> varps = V3Cfg::ಜೀವಂತ_Var_Scopes(cfg);
        ಆದರೆ (!varps) {
            ++m_ctx.m_non_Rep_ಜೀವಂತ;
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }

        // Convert to vertics
        ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use user3_ಒಳ_Use;
        std::unique_ptr<std::ಸದಿಶ<Dfg_ಶೃಂಗ_Var*>> resp{ಹೊಸ std::ಸದಿಶ<Dfg_ಶೃಂಗ_Var*>{}};
        resp->reserve(varps->ಗಾತ್ರ_2());
        ಗಾಗಿ (AstVarScope* ಸ್ಥಿರಾಂಕ varp : *varps) {
            ಆದರೆ (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            }
            UASSERT_OBJ(!varp->user3_ಗಣ_Once(), varp, "Live variables should be unique");
            resp->emplace_ಹಿಂಭಾಗ(get_Var_ಶೃಂಗ(varp));
        }
        ಹಿಂತಿರುಗಿ resp;
    }

    // Connect inputs and outputs of a DfgLogic
    ಶೂನ್ಯ connect(DfgLogic& vtx, ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Dfg_ಶೃಂಗ_Var*>& iVarps,
                 ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Dfg_ಶೃಂಗ_Var*>& oVarps) {
        // Connect inputs
        ಗಾಗಿ (Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ iVarp : iVarps) vtx.ಕೂಡಿಸು_ಒಳಹರಿವು(iVarp);
        // Connect outputs
        ಗಾಗಿ (Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ oVarp : oVarps) {
            ಆದರೆ (!oVarp->srcp()) oVarp->srcp(ಹೊಸ DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->ಕೂಡಿಸು_Driver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    ತಾರ್ಕಿಕ convert(AstAlways* nodep) {
        ಸ್ಥಿರಾಂಕ VAlwaysKwd kwd = nodep->keyword();
        ಆದರೆ (kwd == VAlwaysKwd::CONT_ನಿಯೋಜಿಸು) {
            // TODO: simplify once CFG analysis can handle arrays
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ ap = VN_CAST(nodep->stmtsp(), ನಿಯೋಜಿಸು_W)) {
                ಆದರೆ (ap->nextp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                // Cannot handle assignment with timing control
                ಆದರೆ (ap->timingControlp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ಸದಿಶ<Dfg_ಶೃಂಗ_Var*>> oVarpsp = gatherWritten(ap);
                ಆದರೆ (!oVarpsp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                // Gather read variables, give up if any are not supported
                ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ಸದಿಶ<Dfg_ಶೃಂಗ_Var*>> iVarpsp = gatherRead(ap);
                ಆದರೆ (!iVarpsp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                // Create the DfgLogic
                DfgLogic* ಸ್ಥಿರಾಂಕ logicp = ಹೊಸ DfgLogic{m_dfg, nodep, m_scopep, ಶೂನ್ಯಸೂಚಕ};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                //ಮುಗಿದ
                ++m_ctx.m_representable;
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        }

        // Can only handle combinational logic
        ಆದರೆ (nodep->sentreep()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_ನಿಯೋಜಿಸು) {
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<Cfg_ನಕ್ಷೆ> cfgp = Cfg_ನಕ್ಷೆ::build(nodep->stmtsp());
        ಆದರೆ (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        // Gather written variables, give up if any are not supported
        ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ಸದಿಶ<Dfg_ಶೃಂಗ_Var*>> oVarpsp = gatherWritten(nodep);
        ಆದರೆ (!oVarpsp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Gather read variables, give up if any are not supported
        ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ಸದಿಶ<Dfg_ಶೃಂಗ_Var*>> iVarpsp = gather_ಜೀವಂತ(*cfgp);
        ಆದರೆ (!iVarpsp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Create the DfgLogic
        DfgLogic* ಸ್ಥಿರಾಂಕ logicp = ಹೊಸ DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        //ಮುಗಿದ
        ++m_ctx.m_representable;
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // VISITORS
    // Unhandled node
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    ಶೂನ್ಯ ಭೇಟಿ(AstNetlist* nodep) override { iterate_And_ಮುಂದಿನ_Null(nodep->modulesp()); }
    ಶೂನ್ಯ ಭೇಟಿ(AstModule* nodep) override { iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp()); }
    ಶೂನ್ಯ ಭೇಟಿ(AstIface* nodep) override {
        ಆದರೆ (!nodep->hasVirtualRef()) {
            iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            markReferenced(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ತುದಿ_Scope* nodep) override { iterate(nodep->scopep()); }
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಕ್ರಿಯ* nodep) override {
        ಆದರೆ (nodep->hasCombo()) {
            iterateChildren(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            markReferenced(nodep);
        }
    }

    // Non-representable constructs
    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override { markReferenced(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Procedure* nodep) override { markReferenced(nodep); }

    // Potentially representable constructs
    ಶೂನ್ಯ ಭೇಟಿ(AstAlways* nodep) override {
        ಆದರೆ (!convert(nodep)) markReferenced(nodep);
    }

    // CONSTRUCTOR
    Ast_ಗಮ್ಯ_Dfg_Visitor(Dfg_ನಕ್ಷೆ& dfg, AstNetlist& ಬೇರು, V3_Dfg_Ast_ಗಮ್ಯ_Dfg_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {
        iterate(&ಬೇರು);
    }
    VL_UNCOPYABLE(Ast_ಗಮ್ಯ_Dfg_Visitor);
    VL_UNMOVABLE(Ast_ಗಮ್ಯ_Dfg_Visitor);

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(Dfg_ನಕ್ಷೆ& dfg, AstNetlist& ಬೇರು, V3_Dfg_Ast_ಗಮ್ಯ_Dfg_Context& ctx) {
        // Convert all logic under 'root'
        Ast_ಗಮ್ಯ_Dfg_Visitor{dfg, ಬೇರು, ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        ಗಾಗಿ (Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp : dfg.varVertices().unlinkable()) {
            ಆದರೆ (varp->srcp()) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ keep = varp->foreachSink([&](Dfg_ಶೃಂಗ& d) { ಹಿಂತಿರುಗಿ !d.is<DfgAstRd>(); });
            ಆದರೆ (!keep) {
                ಆಗ (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<Dfg_ನಕ್ಷೆ> V3DfgPasses::ast_ಗಮ್ಯ_Dfg(AstNetlist& netlist, V3DfgContext& ctx) {
    Dfg_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ dfgp = ಹೊಸ Dfg_ನಕ್ಷೆ{ctx, "netlist"};
    Ast_ಗಮ್ಯ_Dfg_Visitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    ಹಿಂತಿರುಗಿ std::unique_ptr<Dfg_ನಕ್ಷೆ>{dfgp};
}
