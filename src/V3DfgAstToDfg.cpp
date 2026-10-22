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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Cfg.h"
#समावेश "V3Const.h"
#समावेश "V3Dfg.h"
#समावेश "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

श्रेणी AstToDfgAddAstRefs final : खुला VNVisitorConst {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    // Function to get the DfgVertexVar for a AstVarScope
    स्थिर std::function<DfgVertexVar*(AstVarScope*)> m_getVarVertex;
    bool m_inSenItem = false;  // Inside an AstSenItem
    bool m_inLoop = false;  // Inside an AstLoop

    // VISITORS
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

    व्योम visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = true;
        iterateChildrenConst(nodep);
    }

    व्योम visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = true;
        iterateChildrenConst(nodep);
    }

    व्योम visit(AstVarRef* nodep) override {
        // Disguised hierarchical reference handled as external reference, ignore
        अगर (nodep->classOrPackagep()) वापस;
        // If target is not supported, ignore
        AstVarScope* स्थिर vscp = nodep->varScopep();
        अगर (!V3Dfg::isSupported(vscp)) वापस;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        DfgVertexVar* स्थिर varp = m_getVarVertex(vscp);
        अगर (!varp) वापस;
        // Create Ast reference vertices
        अगर (nodep->access().isReadOnly()) {
            DfgAstRd* स्थिर astp = नया DfgAstRd{m_dfg, nodep, m_inSenItem, m_inLoop};
            astp->srcp(varp);
            वापस;
        }
        // Mark as written from non-DFG logic
        DfgVertexVar::setHasModWrRefs(vscp);
    }

    AstToDfgAddAstRefs(DfgGraph& dfg, Ast_गाँठ* nodep,
                       std::function<DfgVertexVar*(AstVarScope*)> getVarVertex)
        : m_dfg{dfg}
        , m_getVarVertex{getVarVertex} {
        iterateConst(nodep);
    }

खुला:
    अटल व्योम apply(DfgGraph& dfg, Ast_गाँठ* nodep,
                      std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
        AstToDfgAddAstRefs{dfg, nodep, getVarVertex};
    }
};

व्योम V3DfgPasses::addAstRefs(DfgGraph& dfg, Ast_गाँठ* nodep,
                             std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
    AstToDfgAddAstRefs::apply(dfg, nodep, getVarVertex);
}

श्रेणी AstToDfgVisitor final : खुला VNVisitor {
    // NODE STATE
    // AstVarScope::user2() -> DfgVertexVar* : the corresponding variable vertex
    // AstVarScope::user3() -> bool : Already gathered - used fine grained below
    स्थिर VNUser2InUse m_user2InUse;

    // STATE
    DfgGraph& m_dfg;  // The graph being built
    V3DfgAstToDfgContext& m_ctx;  // The context for stats
    AstScope* m_scopep = nullptr;  // The current scope, iff T_Scoped

    // METHODS

    // Mark variables referenced under node
    व्योम markReferenced(Ast_गाँठ* nodep) {
        V3DfgPasses::addAstRefs(m_dfg, nodep, [यह](AstVarScope* vscp) {  //
            वापस getVarVertex(vscp);
        });
    }

    DfgVertexVar* getVarVertex(AstVarScope* varp) {
        अगर (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            स्थिर Ast_गाँठ_DType* स्थिर dtypep = varp->dtypep()->skipRefp();
            DfgVertexVar* स्थिर vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<DfgVertexVar*>(नया DfgVarArray{m_dfg, varp})
                      : static_cast<DfgVertexVar*>(नया DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        वापस varp->user2u().ढाँचा to<DfgVertexVar*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherWritten(स्थिर Ast_गाँठ* nodep) {
        स्थिर VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{नया std::vector<DfgVertexVar*>{}};
        स्थिर bool पात = nodep->exists([&](स्थिर AstVarRef* refp) -> bool {
            अगर (refp->access().isReadOnly()) वापस false;
            AstVarScope* स्थिर vscp = refp->varScopep();
            अगर (!V3Dfg::isSupported(vscp)) वापस true;
            अगर (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            वापस false;
        });
        अगर (पात) {
            ++m_ctx.m_nonRepVar;
            वापस nullptr;
        }
        वापस resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherRead(स्थिर Ast_गाँठ* nodep) {
        स्थिर VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{नया std::vector<DfgVertexVar*>{}};
        स्थिर bool पात = nodep->exists([&](स्थिर AstVarRef* refp) -> bool {
            अगर (refp->access().isWriteOnly()) वापस false;
            AstVarScope* स्थिर vscp = refp->varScopep();
            अगर (!V3Dfg::isSupported(vscp)) वापस true;
            अगर (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            वापस false;
        });
        अगर (पात) {
            ++m_ctx.m_nonRepVar;
            वापस nullptr;
        }
        वापस resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherLive(स्थिर CfgGraph& cfg) {
        // Run analysis
        std::unique_ptr<std::vector<AstVarScope*>> varps = V3Cfg::liveVarScopes(cfg);
        अगर (!varps) {
            ++m_ctx.m_nonRepLive;
            वापस nullptr;
        }

        // Convert to vertics
        स्थिर VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{नया std::vector<DfgVertexVar*>{}};
        resp->reserve(varps->आकार());
        क्रम (AstVarScope* स्थिर varp : *varps) {
            अगर (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                वापस nullptr;
            }
            UASSERT_OBJ(!varp->user3SetOnce(), varp, "Live variables should be unique");
            resp->emplace_back(getVarVertex(varp));
        }
        वापस resp;
    }

    // Connect inputs and outputs of a DfgLogic
    व्योम connect(DfgLogic& vtx, स्थिर std::vector<DfgVertexVar*>& iVarps,
                 स्थिर std::vector<DfgVertexVar*>& oVarps) {
        // Connect inputs
        क्रम (DfgVertexVar* स्थिर iVarp : iVarps) vtx.add_निवेश(iVarp);
        // Connect outputs
        क्रम (DfgVertexVar* स्थिर oVarp : oVarps) {
            अगर (!oVarp->srcp()) oVarp->srcp(नया DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->addDriver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    bool convert(AstAlways* nodep) {
        स्थिर VAlwaysKwd kwd = nodep->keyword();
        अगर (kwd == VAlwaysKwd::CONT_ASSIGN) {
            // TODO: simplify once CFG analysis can handle arrays
            अगर (स्थिर AstAssignW* स्थिर ap = VN_CAST(nodep->stmtsp(), AssignW)) {
                अगर (ap->nextp()) वापस false;
                // Cannot handle assignment with timing control
                अगर (ap->timingControlp()) वापस false;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                स्थिर std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(ap);
                अगर (!oVarpsp) वापस false;
                // Gather read variables, give up if any are not supported
                स्थिर std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherRead(ap);
                अगर (!iVarpsp) वापस false;
                // Create the DfgLogic
                DfgLogic* स्थिर logicp = नया DfgLogic{m_dfg, nodep, m_scopep, nullptr};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                // Done
                ++m_ctx.m_representable;
                वापस true;
            }
        }

        // Can only handle combinational logic
        अगर (nodep->sentreep()) वापस false;
        अगर (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_ASSIGN) {
            वापस false;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<CfgGraph> cfgp = CfgGraph::build(nodep->stmtsp());
        अगर (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            वापस false;
        }
        // Gather written variables, give up if any are not supported
        स्थिर std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(nodep);
        अगर (!oVarpsp) वापस false;
        // Gather read variables, give up if any are not supported
        स्थिर std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherLive(*cfgp);
        अगर (!iVarpsp) वापस false;
        // Create the DfgLogic
        DfgLogic* स्थिर logicp = नया DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        // Done
        ++m_ctx.m_representable;
        वापस true;
    }

    // VISITORS
    // Unhandled node
    व्योम visit(Ast_गाँठ* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    व्योम visit(AstNetlist* nodep) override { iterate_And_अग्र_Null(nodep->modulesp()); }
    व्योम visit(AstModule* nodep) override { iterate_And_अग्र_Null(nodep->stmtsp()); }
    व्योम visit(AstIface* nodep) override {
        अगर (!nodep->hasVirtualRef()) {
            iterate_And_अग्र_Null(nodep->stmtsp());
        } अन्यथा {
            markReferenced(nodep);
        }
    }
    व्योम visit(Ast_शीर्ष_Scope* nodep) override { iterate(nodep->scopep()); }
    व्योम visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(AstActive* nodep) override {
        अगर (nodep->hasCombo()) {
            iterateChildren(nodep);
        } अन्यथा {
            markReferenced(nodep);
        }
    }

    // Non-representable constructs
    व्योम visit(AstCell* nodep) override { markReferenced(nodep); }
    व्योम visit(Ast_गाँठ_Procedure* nodep) override { markReferenced(nodep); }

    // Potentially representable constructs
    व्योम visit(AstAlways* nodep) override {
        अगर (!convert(nodep)) markReferenced(nodep);
    }

    // CONSTRUCTOR
    AstToDfgVisitor(DfgGraph& dfg, AstNetlist& मूल, V3DfgAstToDfgContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {
        iterate(&मूल);
    }
    VL_UNCOPYABLE(AstToDfgVisitor);
    VL_UNMOVABLE(AstToDfgVisitor);

खुला:
    अटल व्योम apply(DfgGraph& dfg, AstNetlist& मूल, V3DfgAstToDfgContext& ctx) {
        // Convert all logic under 'root'
        AstToDfgVisitor{dfg, मूल, ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        क्रम (DfgVertexVar* स्थिर varp : dfg.varVertices().unlinkable()) {
            अगर (varp->srcp()) जारी;
            स्थिर bool keep = varp->foreachSink([&](DfgVertex& d) { वापस !d.is<DfgAstRd>(); });
            अगर (!keep) {
                जबतक (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<DfgGraph> V3DfgPasses::astToDfg(AstNetlist& netlist, V3DfgContext& ctx) {
    DfgGraph* स्थिर dfgp = नया DfgGraph{ctx, "netlist"};
    AstToDfgVisitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    वापस std::unique_ptr<DfgGraph>{dfgp};
}
