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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Cfg.h"
#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3Dfg.h"
#અન્તર્ભાવ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

વર્ગ AstToDfgAddAstRefs final : જાહેર VNVisitorConst {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    // Function to get the DfgVertexVar for a AstVarScope
    અચલ std::function<DfgVertexVar*(AstVarScope*)> m_getVarVertex;
    તાર્કિક m_inSenItem = ખોટું;  // Inside an AstSenItem
    તાર્કિક m_inLoop = ખોટું;  // Inside an AstLoop

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

    શૂન્ય visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = સાચું;
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = સાચું;
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(AstVarRef* nodep) override {
        // Disguised hierarchical reference handled as external reference, ignore
        જો (nodep->classOrPackagep()) પાછા;
        // If target is not supported, ignore
        AstVarScope* અચલ vscp = nodep->varScopep();
        જો (!V3Dfg::isSupported(vscp)) પાછા;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        DfgVertexVar* અચલ varp = m_getVarVertex(vscp);
        જો (!varp) પાછા;
        // Create Ast reference vertices
        જો (nodep->access().isReadOnly()) {
            DfgAstRd* અચલ astp = નવું DfgAstRd{m_dfg, nodep, m_inSenItem, m_inLoop};
            astp->srcp(varp);
            પાછા;
        }
        // Mark as written from non-DFG logic
        DfgVertexVar::setHasModWrRefs(vscp);
    }

    AstToDfgAddAstRefs(DfgGraph& dfg, Ast_ગાંઠ* nodep,
                       std::function<DfgVertexVar*(AstVarScope*)> getVarVertex)
        : m_dfg{dfg}
        , m_getVarVertex{getVarVertex} {
        iterateConst(nodep);
    }

જાહેર:
    સ્થિર શૂન્ય apply(DfgGraph& dfg, Ast_ગાંઠ* nodep,
                      std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
        AstToDfgAddAstRefs{dfg, nodep, getVarVertex};
    }
};

શૂન્ય V3DfgPasses::addAstRefs(DfgGraph& dfg, Ast_ગાંઠ* nodep,
                             std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
    AstToDfgAddAstRefs::apply(dfg, nodep, getVarVertex);
}

વર્ગ AstToDfgVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // AstVarScope::user2() -> DfgVertexVar* : the corresponding variable vertex
    // AstVarScope::user3() -> bool : Already gathered - used fine grained below
    અચલ VNUser2InUse m_user2InUse;

    // STATE
    DfgGraph& m_dfg;  // The graph being built
    V3DfgAstToDfgContext& m_ctx;  // The context for stats
    AstScope* m_scopep = nullptr;  // The current scope, iff T_Scoped

    // METHODS

    // Mark variables referenced under node
    શૂન્ય markReferenced(Ast_ગાંઠ* nodep) {
        V3DfgPasses::addAstRefs(m_dfg, nodep, [આ](AstVarScope* vscp) {  //
            પાછા getVarVertex(vscp);
        });
    }

    DfgVertexVar* getVarVertex(AstVarScope* varp) {
        જો (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            અચલ Ast_ગાંઠ_DType* અચલ dtypep = varp->dtypep()->skipRefp();
            DfgVertexVar* અચલ vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<DfgVertexVar*>(નવું DfgVarArray{m_dfg, varp})
                      : static_cast<DfgVertexVar*>(નવું DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        પાછા varp->user2u().template to<DfgVertexVar*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherWritten(અચલ Ast_ગાંઠ* nodep) {
        અચલ VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{નવું std::vector<DfgVertexVar*>{}};
        અચલ તાર્કિક abort = nodep->exists([&](અચલ AstVarRef* refp) -> તાર્કિક {
            જો (refp->access().isReadOnly()) પાછા ખોટું;
            AstVarScope* અચલ vscp = refp->varScopep();
            જો (!V3Dfg::isSupported(vscp)) પાછા સાચું;
            જો (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            પાછા ખોટું;
        });
        જો (abort) {
            ++m_ctx.m_nonRepVar;
            પાછા nullptr;
        }
        પાછા resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherRead(અચલ Ast_ગાંઠ* nodep) {
        અચલ VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{નવું std::vector<DfgVertexVar*>{}};
        અચલ તાર્કિક abort = nodep->exists([&](અચલ AstVarRef* refp) -> તાર્કિક {
            જો (refp->access().isWriteOnly()) પાછા ખોટું;
            AstVarScope* અચલ vscp = refp->varScopep();
            જો (!V3Dfg::isSupported(vscp)) પાછા સાચું;
            જો (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            પાછા ખોટું;
        });
        જો (abort) {
            ++m_ctx.m_nonRepVar;
            પાછા nullptr;
        }
        પાછા resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherLive(અચલ CfgGraph& cfg) {
        // Run analysis
        std::unique_ptr<std::vector<AstVarScope*>> varps = V3Cfg::liveVarScopes(cfg);
        જો (!varps) {
            ++m_ctx.m_nonRepLive;
            પાછા nullptr;
        }

        // Convert to vertics
        અચલ VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{નવું std::vector<DfgVertexVar*>{}};
        resp->reserve(varps->size());
        માટે (AstVarScope* અચલ varp : *varps) {
            જો (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                પાછા nullptr;
            }
            UASSERT_OBJ(!varp->user3SetOnce(), varp, "Live variables should be unique");
            resp->emplace_back(getVarVertex(varp));
        }
        પાછા resp;
    }

    // Connect inputs and outputs of a DfgLogic
    શૂન્ય connect(DfgLogic& vtx, અચલ std::vector<DfgVertexVar*>& iVarps,
                 અચલ std::vector<DfgVertexVar*>& oVarps) {
        // Connect inputs
        માટે (DfgVertexVar* અચલ iVarp : iVarps) vtx.add_આગત(iVarp);
        // Connect outputs
        માટે (DfgVertexVar* અચલ oVarp : oVarps) {
            જો (!oVarp->srcp()) oVarp->srcp(નવું DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->addDriver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    તાર્કિક convert(AstAlways* nodep) {
        અચલ VAlwaysKwd kwd = nodep->keyword();
        જો (kwd == VAlwaysKwd::CONT_ASSIGN) {
            // TODO: simplify once CFG analysis can handle arrays
            જો (અચલ AstAssignW* અચલ ap = VN_CAST(nodep->stmtsp(), AssignW)) {
                જો (ap->nextp()) પાછા ખોટું;
                // Cannot handle assignment with timing control
                જો (ap->timingControlp()) પાછા ખોટું;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                અચલ std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(ap);
                જો (!oVarpsp) પાછા ખોટું;
                // Gather read variables, give up if any are not supported
                અચલ std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherRead(ap);
                જો (!iVarpsp) પાછા ખોટું;
                // Create the DfgLogic
                DfgLogic* અચલ logicp = નવું DfgLogic{m_dfg, nodep, m_scopep, nullptr};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                // Done
                ++m_ctx.m_representable;
                પાછા સાચું;
            }
        }

        // Can only handle combinational logic
        જો (nodep->sentreep()) પાછા ખોટું;
        જો (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_ASSIGN) {
            પાછા ખોટું;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<CfgGraph> cfgp = CfgGraph::build(nodep->stmtsp());
        જો (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            પાછા ખોટું;
        }
        // Gather written variables, give up if any are not supported
        અચલ std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(nodep);
        જો (!oVarpsp) પાછા ખોટું;
        // Gather read variables, give up if any are not supported
        અચલ std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherLive(*cfgp);
        જો (!iVarpsp) પાછા ખોટું;
        // Create the DfgLogic
        DfgLogic* અચલ logicp = નવું DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        // Done
        ++m_ctx.m_representable;
        પાછા સાચું;
    }

    // VISITORS
    // Unhandled node
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    શૂન્ય visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    શૂન્ય visit(AstModule* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    શૂન્ય visit(AstIface* nodep) override {
        જો (!nodep->hasVirtualRef()) {
            iterateAndNextNull(nodep->stmtsp());
        } નહીંતર {
            markReferenced(nodep);
        }
    }
    શૂન્ય visit(Ast_ટોચ_Scope* nodep) override { iterate(nodep->scopep()); }
    શૂન્ય visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstActive* nodep) override {
        જો (nodep->hasCombo()) {
            iterateChildren(nodep);
        } નહીંતર {
            markReferenced(nodep);
        }
    }

    // Non-representable constructs
    શૂન્ય visit(AstCell* nodep) override { markReferenced(nodep); }
    શૂન્ય visit(Ast_ગાંઠ_Procedure* nodep) override { markReferenced(nodep); }

    // Potentially representable constructs
    શૂન્ય visit(AstAlways* nodep) override {
        જો (!convert(nodep)) markReferenced(nodep);
    }

    // CONSTRUCTOR
    AstToDfgVisitor(DfgGraph& dfg, AstNetlist& મૂળ, V3DfgAstToDfgContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {
        iterate(&મૂળ);
    }
    VL_UNCOPYABLE(AstToDfgVisitor);
    VL_UNMOVABLE(AstToDfgVisitor);

જાહેર:
    સ્થિર શૂન્ય apply(DfgGraph& dfg, AstNetlist& મૂળ, V3DfgAstToDfgContext& ctx) {
        // Convert all logic under 'root'
        AstToDfgVisitor{dfg, મૂળ, ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        માટે (DfgVertexVar* અચલ varp : dfg.varVertices().unlinkable()) {
            જો (varp->srcp()) ચાલુ;
            અચલ તાર્કિક keep = varp->foreachSink([&](DfgVertex& d) { પાછા !d.is<DfgAstRd>(); });
            જો (!keep) {
                જ્યારે (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<DfgGraph> V3DfgPasses::astToDfg(AstNetlist& netlist, V3DfgContext& ctx) {
    DfgGraph* અચલ dfgp = નવું DfgGraph{ctx, "netlist"};
    AstToDfgVisitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    પાછા std::unique_ptr<DfgGraph>{dfgp};
}
