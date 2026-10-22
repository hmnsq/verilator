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

ವರ್ಗ AstToDfgAddAstRefs final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    // Function to get the DfgVertexVar for a AstVarScope
    ಧ್ರುವ std::function<DfgVertexVar*(AstVarScope*)> m_getVarVertex;
    ತಾರ್ಕಿಕ m_inSenItem = ಅಸತ್ಯ;  // Inside an AstSenItem
    ತಾರ್ಕಿಕ m_inLoop = ಅಸತ್ಯ;  // Inside an AstLoop

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

    ಶೂನ್ಯ visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = ಸತ್ಯ;
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = ಸತ್ಯ;
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        // Disguised hierarchical reference handled as external reference, ignore
        ಯದಿ (nodep->classOrPackagep()) ಪ್ರತ್ಯಾಗಮ;
        // If target is not supported, ignore
        AstVarScope* ಧ್ರುವ vscp = nodep->varScopep();
        ಯದಿ (!V3Dfg::isSupported(vscp)) ಪ್ರತ್ಯಾಗಮ;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        DfgVertexVar* ಧ್ರುವ varp = m_getVarVertex(vscp);
        ಯದಿ (!varp) ಪ್ರತ್ಯಾಗಮ;
        // Create Ast reference vertices
        ಯದಿ (nodep->access().isReadOnly()) {
            DfgAstRd* ಧ್ರುವ astp = ನೂತನ DfgAstRd{m_dfg, nodep, m_inSenItem, m_inLoop};
            astp->srcp(varp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Mark as written from non-DFG logic
        DfgVertexVar::setHasModWrRefs(vscp);
    }

    AstToDfgAddAstRefs(DfgGraph& dfg, Ast_ಗಂಟು* nodep,
                       std::function<DfgVertexVar*(AstVarScope*)> getVarVertex)
        : m_dfg{dfg}
        , m_getVarVertex{getVarVertex} {
        iterateConst(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(DfgGraph& dfg, Ast_ಗಂಟು* nodep,
                      std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
        AstToDfgAddAstRefs{dfg, nodep, getVarVertex};
    }
};

ಶೂನ್ಯ V3DfgPasses::addAstRefs(DfgGraph& dfg, Ast_ಗಂಟು* nodep,
                             std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
    AstToDfgAddAstRefs::apply(dfg, nodep, getVarVertex);
}

ವರ್ಗ AstToDfgVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // AstVarScope::user2() -> DfgVertexVar* : the corresponding variable vertex
    // AstVarScope::user3() -> bool : Already gathered - used fine grained below
    ಧ್ರುವ VNUser2InUse m_user2InUse;

    // STATE
    DfgGraph& m_dfg;  // The graph being built
    V3DfgAstToDfgContext& m_ctx;  // The context for stats
    AstScope* m_scopep = nullptr;  // The current scope, iff T_Scoped

    // METHODS

    // Mark variables referenced under node
    ಶೂನ್ಯ markReferenced(Ast_ಗಂಟು* nodep) {
        V3DfgPasses::addAstRefs(m_dfg, nodep, [ಏತತ್](AstVarScope* vscp) {  //
            ಪ್ರತ್ಯಾಗಮ getVarVertex(vscp);
        });
    }

    DfgVertexVar* getVarVertex(AstVarScope* varp) {
        ಯದಿ (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = varp->dtypep()->skipRefp();
            DfgVertexVar* ಧ್ರುವ vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<DfgVertexVar*>(ನೂತನ DfgVarArray{m_dfg, varp})
                      : static_cast<DfgVertexVar*>(ನೂತನ DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        ಪ್ರತ್ಯಾಗಮ varp->user2u().template to<DfgVertexVar*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherWritten(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಧ್ರುವ VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{ನೂತನ std::vector<DfgVertexVar*>{}};
        ಧ್ರುವ ತಾರ್ಕಿಕ abort = nodep->exists([&](ಧ್ರುವ AstVarRef* refp) -> ತಾರ್ಕಿಕ {
            ಯದಿ (refp->access().isReadOnly()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            AstVarScope* ಧ್ರುವ vscp = refp->varScopep();
            ಯದಿ (!V3Dfg::isSupported(vscp)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            ಯದಿ (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
        ಯದಿ (abort) {
            ++m_ctx.m_nonRepVar;
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        ಪ್ರತ್ಯಾಗಮ resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherRead(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಧ್ರುವ VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{ನೂತನ std::vector<DfgVertexVar*>{}};
        ಧ್ರುವ ತಾರ್ಕಿಕ abort = nodep->exists([&](ಧ್ರುವ AstVarRef* refp) -> ತಾರ್ಕಿಕ {
            ಯದಿ (refp->access().isWriteOnly()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            AstVarScope* ಧ್ರುವ vscp = refp->varScopep();
            ಯದಿ (!V3Dfg::isSupported(vscp)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            ಯದಿ (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
        ಯದಿ (abort) {
            ++m_ctx.m_nonRepVar;
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        ಪ್ರತ್ಯಾಗಮ resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherLive(ಧ್ರುವ CfgGraph& cfg) {
        // Run analysis
        std::unique_ptr<std::vector<AstVarScope*>> varps = V3Cfg::liveVarScopes(cfg);
        ಯದಿ (!varps) {
            ++m_ctx.m_nonRepLive;
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }

        // Convert to vertics
        ಧ್ರುವ VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{ನೂತನ std::vector<DfgVertexVar*>{}};
        resp->reserve(varps->size());
        ಪ್ರತಿ (AstVarScope* ಧ್ರುವ varp : *varps) {
            ಯದಿ (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                ಪ್ರತ್ಯಾಗಮ nullptr;
            }
            UASSERT_OBJ(!varp->user3SetOnce(), varp, "Live variables should be unique");
            resp->emplace_back(getVarVertex(varp));
        }
        ಪ್ರತ್ಯಾಗಮ resp;
    }

    // Connect inputs and outputs of a DfgLogic
    ಶೂನ್ಯ connect(DfgLogic& vtx, ಧ್ರುವ std::vector<DfgVertexVar*>& iVarps,
                 ಧ್ರುವ std::vector<DfgVertexVar*>& oVarps) {
        // Connect inputs
        ಪ್ರತಿ (DfgVertexVar* ಧ್ರುವ iVarp : iVarps) vtx.add_ಒಳಹರಿವು(iVarp);
        // Connect outputs
        ಪ್ರತಿ (DfgVertexVar* ಧ್ರುವ oVarp : oVarps) {
            ಯದಿ (!oVarp->srcp()) oVarp->srcp(ನೂತನ DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->addDriver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    ತಾರ್ಕಿಕ convert(AstAlways* nodep) {
        ಧ್ರುವ VAlwaysKwd kwd = nodep->keyword();
        ಯದಿ (kwd == VAlwaysKwd::CONT_ASSIGN) {
            // TODO: simplify once CFG analysis can handle arrays
            ಯದಿ (ಧ್ರುವ AstAssignW* ಧ್ರುವ ap = VN_CAST(nodep->stmtsp(), AssignW)) {
                ಯದಿ (ap->nextp()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                // Cannot handle assignment with timing control
                ಯದಿ (ap->timingControlp()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                ಧ್ರುವ std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(ap);
                ಯದಿ (!oVarpsp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                // Gather read variables, give up if any are not supported
                ಧ್ರುವ std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherRead(ap);
                ಯದಿ (!iVarpsp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                // Create the DfgLogic
                DfgLogic* ಧ್ರುವ logicp = ನೂತನ DfgLogic{m_dfg, nodep, m_scopep, nullptr};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                // Done
                ++m_ctx.m_representable;
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }

        // Can only handle combinational logic
        ಯದಿ (nodep->sentreep()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_ASSIGN) {
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<CfgGraph> cfgp = CfgGraph::build(nodep->stmtsp());
        ಯದಿ (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        // Gather written variables, give up if any are not supported
        ಧ್ರುವ std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(nodep);
        ಯದಿ (!oVarpsp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Gather read variables, give up if any are not supported
        ಧ್ರುವ std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherLive(*cfgp);
        ಯದಿ (!iVarpsp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Create the DfgLogic
        DfgLogic* ಧ್ರುವ logicp = ನೂತನ DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        // Done
        ++m_ctx.m_representable;
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // VISITORS
    // Unhandled node
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    ಶೂನ್ಯ visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    ಶೂನ್ಯ visit(AstModule* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    ಶೂನ್ಯ visit(AstIface* nodep) override {
        ಯದಿ (!nodep->hasVirtualRef()) {
            iterateAndNextNull(nodep->stmtsp());
        } ಅನ್ಯಥಾ {
            markReferenced(nodep);
        }
    }
    ಶೂನ್ಯ visit(Ast_ತುದಿ_Scope* nodep) override { iterate(nodep->scopep()); }
    ಶೂನ್ಯ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstActive* nodep) override {
        ಯದಿ (nodep->hasCombo()) {
            iterateChildren(nodep);
        } ಅನ್ಯಥಾ {
            markReferenced(nodep);
        }
    }

    // Non-representable constructs
    ಶೂನ್ಯ visit(AstCell* nodep) override { markReferenced(nodep); }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Procedure* nodep) override { markReferenced(nodep); }

    // Potentially representable constructs
    ಶೂನ್ಯ visit(AstAlways* nodep) override {
        ಯದಿ (!convert(nodep)) markReferenced(nodep);
    }

    // CONSTRUCTOR
    AstToDfgVisitor(DfgGraph& dfg, AstNetlist& ಬೇರು, V3DfgAstToDfgContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {
        iterate(&ಬೇರು);
    }
    VL_UNCOPYABLE(AstToDfgVisitor);
    VL_UNMOVABLE(AstToDfgVisitor);

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(DfgGraph& dfg, AstNetlist& ಬೇರು, V3DfgAstToDfgContext& ctx) {
        // Convert all logic under 'root'
        AstToDfgVisitor{dfg, ಬೇರು, ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        ಪ್ರತಿ (DfgVertexVar* ಧ್ರುವ varp : dfg.varVertices().unlinkable()) {
            ಯದಿ (varp->srcp()) ಅನುವರ್ತ;
            ಧ್ರುವ ತಾರ್ಕಿಕ keep = varp->foreachSink([&](DfgVertex& d) { ಪ್ರತ್ಯಾಗಮ !d.is<DfgAstRd>(); });
            ಯದಿ (!keep) {
                ಯಾವತ್ (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<DfgGraph> V3DfgPasses::astToDfg(AstNetlist& netlist, V3DfgContext& ctx) {
    DfgGraph* ಧ್ರುವ dfgp = ನೂತನ DfgGraph{ctx, "netlist"};
    AstToDfgVisitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    ಪ್ರತ್ಯಾಗಮ std::unique_ptr<DfgGraph>{dfgp};
}
