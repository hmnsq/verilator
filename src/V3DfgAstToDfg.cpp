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
    अचर std::function<DfgVertexVar*(AstVarScope*)> m_getVarVertex;
    तार्किक m_inSenItem = असत्य;  // Inside an AstSenItem
    तार्किक m_inLoop = असत्य;  // Inside an AstLoop

    // VISITORS
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildrenConst(nodep); }

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
        यदि (nodep->classOrPackagep()) फर्काऊ;
        // If target is not supported, ignore
        AstVarScope* अचर vscp = nodep->varScopep();
        यदि (!V3Dfg::isSupported(vscp)) फर्काऊ;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        DfgVertexVar* अचर varp = m_getVarVertex(vscp);
        यदि (!varp) फर्काऊ;
        // Create Ast reference vertices
        यदि (nodep->access().isReadOnly()) {
            DfgAstRd* अचर astp = नूतन DfgAstRd{m_dfg, nodep, m_inSenItem, m_inLoop};
            astp->srcp(varp);
            फर्काऊ;
        }
        // Mark as written from non-DFG logic
        DfgVertexVar::setHasModWrRefs(vscp);
    }

    AstToDfgAddAstRefs(DfgGraph& dfg, Ast_गाँठो* nodep,
                       std::function<DfgVertexVar*(AstVarScope*)> getVarVertex)
        : m_dfg{dfg}
        , m_getVarVertex{getVarVertex} {
        iterateConst(nodep);
    }

सार्वजनिक:
    स्थिर शून्य apply(DfgGraph& dfg, Ast_गाँठो* nodep,
                      std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
        AstToDfgAddAstRefs{dfg, nodep, getVarVertex};
    }
};

शून्य V3DfgPasses::addAstRefs(DfgGraph& dfg, Ast_गाँठो* nodep,
                             std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
    AstToDfgAddAstRefs::apply(dfg, nodep, getVarVertex);
}

वर्ग AstToDfgVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstVarScope::user2() -> DfgVertexVar* : the corresponding variable vertex
    // AstVarScope::user3() -> bool : Already gathered - used fine grained below
    अचर VNUser2InUse m_user2InUse;

    // STATE
    DfgGraph& m_dfg;  // The graph being built
    V3DfgAstToDfgContext& m_ctx;  // The context for stats
    AstScope* m_scopep = nullptr;  // The current scope, iff T_Scoped

    // METHODS

    // Mark variables referenced under node
    शून्य markReferenced(Ast_गाँठो* nodep) {
        V3DfgPasses::addAstRefs(m_dfg, nodep, [एतत्](AstVarScope* vscp) {  //
            फर्काऊ getVarVertex(vscp);
        });
    }

    DfgVertexVar* getVarVertex(AstVarScope* varp) {
        यदि (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            अचर Ast_गाँठो_DType* अचर dtypep = varp->dtypep()->skipRefp();
            DfgVertexVar* अचर vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<DfgVertexVar*>(नूतन DfgVarArray{m_dfg, varp})
                      : static_cast<DfgVertexVar*>(नूतन DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        फर्काऊ varp->user2u().ढाँचा to<DfgVertexVar*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherWritten(अचर Ast_गाँठो* nodep) {
        अचर VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{नूतन std::vector<DfgVertexVar*>{}};
        अचर तार्किक पात = nodep->exists([&](अचर AstVarRef* refp) -> तार्किक {
            यदि (refp->access().isReadOnly()) फर्काऊ असत्य;
            AstVarScope* अचर vscp = refp->varScopep();
            यदि (!V3Dfg::isSupported(vscp)) फर्काऊ सत्य;
            यदि (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            फर्काऊ असत्य;
        });
        यदि (पात) {
            ++m_ctx.m_nonRepVar;
            फर्काऊ nullptr;
        }
        फर्काऊ resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherRead(अचर Ast_गाँठो* nodep) {
        अचर VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{नूतन std::vector<DfgVertexVar*>{}};
        अचर तार्किक पात = nodep->exists([&](अचर AstVarRef* refp) -> तार्किक {
            यदि (refp->access().isWriteOnly()) फर्काऊ असत्य;
            AstVarScope* अचर vscp = refp->varScopep();
            यदि (!V3Dfg::isSupported(vscp)) फर्काऊ सत्य;
            यदि (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            फर्काऊ असत्य;
        });
        यदि (पात) {
            ++m_ctx.m_nonRepVar;
            फर्काऊ nullptr;
        }
        फर्काऊ resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherLive(अचर CfgGraph& cfg) {
        // Run analysis
        std::unique_ptr<std::vector<AstVarScope*>> varps = V3Cfg::liveVarScopes(cfg);
        यदि (!varps) {
            ++m_ctx.m_nonRepLive;
            फर्काऊ nullptr;
        }

        // Convert to vertics
        अचर VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{नूतन std::vector<DfgVertexVar*>{}};
        resp->reserve(varps->size());
        लागि (AstVarScope* अचर varp : *varps) {
            यदि (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                फर्काऊ nullptr;
            }
            UASSERT_OBJ(!varp->user3SetOnce(), varp, "Live variables should be unique");
            resp->emplace_back(getVarVertex(varp));
        }
        फर्काऊ resp;
    }

    // Connect inputs and outputs of a DfgLogic
    शून्य connect(DfgLogic& vtx, अचर std::vector<DfgVertexVar*>& iVarps,
                 अचर std::vector<DfgVertexVar*>& oVarps) {
        // Connect inputs
        लागि (DfgVertexVar* अचर iVarp : iVarps) vtx.add_आगत(iVarp);
        // Connect outputs
        लागि (DfgVertexVar* अचर oVarp : oVarps) {
            यदि (!oVarp->srcp()) oVarp->srcp(नूतन DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->addDriver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    तार्किक convert(AstAlways* nodep) {
        अचर VAlwaysKwd kwd = nodep->keyword();
        यदि (kwd == VAlwaysKwd::CONT_ASSIGN) {
            // TODO: simplify once CFG analysis can handle arrays
            यदि (अचर AstAssignW* अचर ap = VN_CAST(nodep->stmtsp(), AssignW)) {
                यदि (ap->nextp()) फर्काऊ असत्य;
                // Cannot handle assignment with timing control
                यदि (ap->timingControlp()) फर्काऊ असत्य;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                अचर std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(ap);
                यदि (!oVarpsp) फर्काऊ असत्य;
                // Gather read variables, give up if any are not supported
                अचर std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherRead(ap);
                यदि (!iVarpsp) फर्काऊ असत्य;
                // Create the DfgLogic
                DfgLogic* अचर logicp = नूतन DfgLogic{m_dfg, nodep, m_scopep, nullptr};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                // Done
                ++m_ctx.m_representable;
                फर्काऊ सत्य;
            }
        }

        // Can only handle combinational logic
        यदि (nodep->sentreep()) फर्काऊ असत्य;
        यदि (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_ASSIGN) {
            फर्काऊ असत्य;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<CfgGraph> cfgp = CfgGraph::build(nodep->stmtsp());
        यदि (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            फर्काऊ असत्य;
        }
        // Gather written variables, give up if any are not supported
        अचर std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(nodep);
        यदि (!oVarpsp) फर्काऊ असत्य;
        // Gather read variables, give up if any are not supported
        अचर std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherLive(*cfgp);
        यदि (!iVarpsp) फर्काऊ असत्य;
        // Create the DfgLogic
        DfgLogic* अचर logicp = नूतन DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        // Done
        ++m_ctx.m_representable;
        फर्काऊ सत्य;
    }

    // VISITORS
    // Unhandled node
    शून्य visit(Ast_गाँठो* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    शून्य visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    शून्य visit(AstModule* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    शून्य visit(AstIface* nodep) override {
        यदि (!nodep->hasVirtualRef()) {
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
        यदि (nodep->hasCombo()) {
            iterateChildren(nodep);
        } अन्यथा {
            markReferenced(nodep);
        }
    }

    // Non-representable constructs
    शून्य visit(AstCell* nodep) override { markReferenced(nodep); }
    शून्य visit(Ast_गाँठो_Procedure* nodep) override { markReferenced(nodep); }

    // Potentially representable constructs
    शून्य visit(AstAlways* nodep) override {
        यदि (!convert(nodep)) markReferenced(nodep);
    }

    // CONSTRUCTOR
    AstToDfgVisitor(DfgGraph& dfg, AstNetlist& जरा, V3DfgAstToDfgContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {
        iterate(&जरा);
    }
    VL_UNCOPYABLE(AstToDfgVisitor);
    VL_UNMOVABLE(AstToDfgVisitor);

सार्वजनिक:
    स्थिर शून्य apply(DfgGraph& dfg, AstNetlist& जरा, V3DfgAstToDfgContext& ctx) {
        // Convert all logic under 'root'
        AstToDfgVisitor{dfg, जरा, ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        लागि (DfgVertexVar* अचर varp : dfg.varVertices().unlinkable()) {
            यदि (varp->srcp()) जारी;
            अचर तार्किक keep = varp->foreachSink([&](DfgVertex& d) { फर्काऊ !d.is<DfgAstRd>(); });
            यदि (!keep) {
                जबसम्म (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<DfgGraph> V3DfgPasses::astToDfg(AstNetlist& netlist, V3DfgContext& ctx) {
    DfgGraph* अचर dfgp = नूतन DfgGraph{ctx, "netlist"};
    AstToDfgVisitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    फर्काऊ std::unique_ptr<DfgGraph>{dfgp};
}
