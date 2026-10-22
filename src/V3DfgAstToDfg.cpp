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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Cfg.h"
#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Dfg.h"
#अन्तर्भाव "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

वर्ग AstToDfgAddAstRefs final : सार्वजनिक VNVisitorConst {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    // Function to get the DfgVertexVar for a AstVarScope
    स्थिरांक std::function<DfgVertexVar*(AstVarScope*)> m_getVarVertex;
    तार्किक m_inSenItem = असत्य;  // Inside an AstSenItem
    तार्किक m_inLoop = असत्य;  // Inside an AstLoop

    // VISITORS
    शून्य visit(Ast_गाठ* nodep) override { iterateChildrenConst(nodep); }

    शून्य visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = सत्य;
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = सत्य;
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstVarRef* nodep) override {
        // Disguised hierarchical reference handled as external reference, ignore
        जर (nodep->classOrPackagep()) परत;
        // If target is not supported, ignore
        AstVarScope* स्थिरांक vscp = nodep->varScopep();
        जर (!V3Dfg::isSupported(vscp)) परत;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        DfgVertexVar* स्थिरांक varp = m_getVarVertex(vscp);
        जर (!varp) परत;
        // Create Ast reference vertices
        जर (nodep->access().isReadOnly()) {
            DfgAstRd* स्थिरांक astp = नवीन DfgAstRd{m_dfg, nodep, m_inSenItem, m_inLoop};
            astp->srcp(varp);
            परत;
        }
        // Mark as written from non-DFG logic
        DfgVertexVar::setHasModWrRefs(vscp);
    }

    AstToDfgAddAstRefs(DfgGraph& dfg, Ast_गाठ* nodep,
                       std::function<DfgVertexVar*(AstVarScope*)> getVarVertex)
        : m_dfg{dfg}
        , m_getVarVertex{getVarVertex} {
        iterateConst(nodep);
    }

सार्वजनिक:
    स्थिर शून्य apply(DfgGraph& dfg, Ast_गाठ* nodep,
                      std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
        AstToDfgAddAstRefs{dfg, nodep, getVarVertex};
    }
};

शून्य V3DfgPasses::addAstRefs(DfgGraph& dfg, Ast_गाठ* nodep,
                             std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
    AstToDfgAddAstRefs::apply(dfg, nodep, getVarVertex);
}

वर्ग AstToDfgVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstVarScope::user2() -> DfgVertexVar* : the corresponding variable vertex
    // AstVarScope::user3() -> bool : Already gathered - used fine grained below
    स्थिरांक VNUser2InUse m_user2InUse;

    // STATE
    DfgGraph& m_dfg;  // The graph being built
    V3DfgAstToDfgContext& m_ctx;  // The context for stats
    AstScope* m_scopep = nullptr;  // The current scope, iff T_Scoped

    // METHODS

    // Mark variables referenced under node
    शून्य markReferenced(Ast_गाठ* nodep) {
        V3DfgPasses::addAstRefs(m_dfg, nodep, [हे](AstVarScope* vscp) {  //
            परत getVarVertex(vscp);
        });
    }

    DfgVertexVar* getVarVertex(AstVarScope* varp) {
        जर (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            स्थिरांक Ast_गाठ_DType* स्थिरांक dtypep = varp->dtypep()->skipRefp();
            DfgVertexVar* स्थिरांक vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<DfgVertexVar*>(नवीन DfgVarArray{m_dfg, varp})
                      : static_cast<DfgVertexVar*>(नवीन DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        परत varp->user2u().ढाँचा to<DfgVertexVar*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherWritten(स्थिरांक Ast_गाठ* nodep) {
        स्थिरांक VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{नवीन std::vector<DfgVertexVar*>{}};
        स्थिरांक तार्किक पात = nodep->exists([&](स्थिरांक AstVarRef* refp) -> तार्किक {
            जर (refp->access().isReadOnly()) परत असत्य;
            AstVarScope* स्थिरांक vscp = refp->varScopep();
            जर (!V3Dfg::isSupported(vscp)) परत सत्य;
            जर (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            परत असत्य;
        });
        जर (पात) {
            ++m_ctx.m_nonRepVar;
            परत nullptr;
        }
        परत resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherRead(स्थिरांक Ast_गाठ* nodep) {
        स्थिरांक VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{नवीन std::vector<DfgVertexVar*>{}};
        स्थिरांक तार्किक पात = nodep->exists([&](स्थिरांक AstVarRef* refp) -> तार्किक {
            जर (refp->access().isWriteOnly()) परत असत्य;
            AstVarScope* स्थिरांक vscp = refp->varScopep();
            जर (!V3Dfg::isSupported(vscp)) परत सत्य;
            जर (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            परत असत्य;
        });
        जर (पात) {
            ++m_ctx.m_nonRepVar;
            परत nullptr;
        }
        परत resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherLive(स्थिरांक CfgGraph& cfg) {
        // Run analysis
        std::unique_ptr<std::vector<AstVarScope*>> varps = V3Cfg::liveVarScopes(cfg);
        जर (!varps) {
            ++m_ctx.m_nonRepLive;
            परत nullptr;
        }

        // Convert to vertics
        स्थिरांक VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{नवीन std::vector<DfgVertexVar*>{}};
        resp->reserve(varps->size());
        साठी (AstVarScope* स्थिरांक varp : *varps) {
            जर (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                परत nullptr;
            }
            UASSERT_OBJ(!varp->user3SetOnce(), varp, "Live variables should be unique");
            resp->emplace_back(getVarVertex(varp));
        }
        परत resp;
    }

    // Connect inputs and outputs of a DfgLogic
    शून्य connect(DfgLogic& vtx, स्थिरांक std::vector<DfgVertexVar*>& iVarps,
                 स्थिरांक std::vector<DfgVertexVar*>& oVarps) {
        // Connect inputs
        साठी (DfgVertexVar* स्थिरांक iVarp : iVarps) vtx.add_आदान(iVarp);
        // Connect outputs
        साठी (DfgVertexVar* स्थिरांक oVarp : oVarps) {
            जर (!oVarp->srcp()) oVarp->srcp(नवीन DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->addDriver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    तार्किक convert(AstAlways* nodep) {
        स्थिरांक VAlwaysKwd kwd = nodep->keyword();
        जर (kwd == VAlwaysKwd::CONT_ASSIGN) {
            // TODO: simplify once CFG analysis can handle arrays
            जर (स्थिरांक AstAssignW* स्थिरांक ap = VN_CAST(nodep->stmtsp(), AssignW)) {
                जर (ap->nextp()) परत असत्य;
                // Cannot handle assignment with timing control
                जर (ap->timingControlp()) परत असत्य;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                स्थिरांक std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(ap);
                जर (!oVarpsp) परत असत्य;
                // Gather read variables, give up if any are not supported
                स्थिरांक std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherRead(ap);
                जर (!iVarpsp) परत असत्य;
                // Create the DfgLogic
                DfgLogic* स्थिरांक logicp = नवीन DfgLogic{m_dfg, nodep, m_scopep, nullptr};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                // Done
                ++m_ctx.m_representable;
                परत सत्य;
            }
        }

        // Can only handle combinational logic
        जर (nodep->sentreep()) परत असत्य;
        जर (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_ASSIGN) {
            परत असत्य;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<CfgGraph> cfgp = CfgGraph::build(nodep->stmtsp());
        जर (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            परत असत्य;
        }
        // Gather written variables, give up if any are not supported
        स्थिरांक std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(nodep);
        जर (!oVarpsp) परत असत्य;
        // Gather read variables, give up if any are not supported
        स्थिरांक std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherLive(*cfgp);
        जर (!iVarpsp) परत असत्य;
        // Create the DfgLogic
        DfgLogic* स्थिरांक logicp = नवीन DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        // Done
        ++m_ctx.m_representable;
        परत सत्य;
    }

    // VISITORS
    // Unhandled node
    शून्य visit(Ast_गाठ* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    शून्य visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    शून्य visit(AstModule* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    शून्य visit(AstIface* nodep) override {
        जर (!nodep->hasVirtualRef()) {
            iterateAndNextNull(nodep->stmtsp());
        } अन्यथा {
            markReferenced(nodep);
        }
    }
    शून्य visit(Ast_शीर्ष_Scope* nodep) override { iterate(nodep->scopep()); }
    शून्य visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstActive* nodep) override {
        जर (nodep->hasCombo()) {
            iterateChildren(nodep);
        } अन्यथा {
            markReferenced(nodep);
        }
    }

    // Non-representable constructs
    शून्य visit(AstCell* nodep) override { markReferenced(nodep); }
    शून्य visit(Ast_गाठ_Procedure* nodep) override { markReferenced(nodep); }

    // Potentially representable constructs
    शून्य visit(AstAlways* nodep) override {
        जर (!convert(nodep)) markReferenced(nodep);
    }

    // CONSTRUCTOR
    AstToDfgVisitor(DfgGraph& dfg, AstNetlist& मूळ, V3DfgAstToDfgContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {
        iterate(&मूळ);
    }
    VL_UNCOPYABLE(AstToDfgVisitor);
    VL_UNMOVABLE(AstToDfgVisitor);

सार्वजनिक:
    स्थिर शून्य apply(DfgGraph& dfg, AstNetlist& मूळ, V3DfgAstToDfgContext& ctx) {
        // Convert all logic under 'root'
        AstToDfgVisitor{dfg, मूळ, ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        साठी (DfgVertexVar* स्थिरांक varp : dfg.varVertices().unlinkable()) {
            जर (varp->srcp()) सुरूठेवा;
            स्थिरांक तार्किक keep = varp->foreachSink([&](DfgVertex& d) { परत !d.is<DfgAstRd>(); });
            जर (!keep) {
                पर्यंत (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<DfgGraph> V3DfgPasses::astToDfg(AstNetlist& netlist, V3DfgContext& ctx) {
    DfgGraph* स्थिरांक dfgp = नवीन DfgGraph{ctx, "netlist"};
    AstToDfgVisitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    परत std::unique_ptr<DfgGraph>{dfgp};
}
