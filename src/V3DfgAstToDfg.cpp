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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Cfg.h"
#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3Dfg.h"
#ଅନ୍ତର୍ଭାଵ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ଶ୍ରେଣୀ AstToDfgAddAstRefs final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    // Function to get the DfgVertexVar for a AstVarScope
    ଅଚଳ std::function<DfgVertexVar*(AstVarScope*)> m_getVarVertex;
    ତାର୍କିକ m_inSenItem = ମିଥ୍ୟା;  // Inside an AstSenItem
    ତାର୍କିକ m_inLoop = ମିଥ୍ୟା;  // Inside an AstLoop

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

    ଶୂନ୍ୟ visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = ସତ୍ୟ;
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = ସତ୍ୟ;
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        // Disguised hierarchical reference handled as external reference, ignore
        ଯଦି (nodep->classOrPackagep()) ଫେରନ୍ତୁ;
        // If target is not supported, ignore
        AstVarScope* ଅଚଳ vscp = nodep->varScopep();
        ଯଦି (!V3Dfg::isSupported(vscp)) ଫେରନ୍ତୁ;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        DfgVertexVar* ଅଚଳ varp = m_getVarVertex(vscp);
        ଯଦି (!varp) ଫେରନ୍ତୁ;
        // Create Ast reference vertices
        ଯଦି (nodep->access().isReadOnly()) {
            DfgAstRd* ଅଚଳ astp = ନୂତନ DfgAstRd{m_dfg, nodep, m_inSenItem, m_inLoop};
            astp->srcp(varp);
            ଫେରନ୍ତୁ;
        }
        // Mark as written from non-DFG logic
        DfgVertexVar::setHasModWrRefs(vscp);
    }

    AstToDfgAddAstRefs(DfgGraph& dfg, Ast_ଗଣ୍ଠି* nodep,
                       std::function<DfgVertexVar*(AstVarScope*)> getVarVertex)
        : m_dfg{dfg}
        , m_getVarVertex{getVarVertex} {
        iterateConst(nodep);
    }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(DfgGraph& dfg, Ast_ଗଣ୍ଠି* nodep,
                      std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
        AstToDfgAddAstRefs{dfg, nodep, getVarVertex};
    }
};

ଶୂନ୍ୟ V3DfgPasses::addAstRefs(DfgGraph& dfg, Ast_ଗଣ୍ଠି* nodep,
                             std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
    AstToDfgAddAstRefs::apply(dfg, nodep, getVarVertex);
}

ଶ୍ରେଣୀ AstToDfgVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // AstVarScope::user2() -> DfgVertexVar* : the corresponding variable vertex
    // AstVarScope::user3() -> bool : Already gathered - used fine grained below
    ଅଚଳ VNUser2InUse m_user2InUse;

    // STATE
    DfgGraph& m_dfg;  // The graph being built
    V3DfgAstToDfgContext& m_ctx;  // The context for stats
    AstScope* m_scopep = nullptr;  // The current scope, iff T_Scoped

    // METHODS

    // Mark variables referenced under node
    ଶୂନ୍ୟ markReferenced(Ast_ଗଣ୍ଠି* nodep) {
        V3DfgPasses::addAstRefs(m_dfg, nodep, [ଏହି](AstVarScope* vscp) {  //
            ଫେରନ୍ତୁ getVarVertex(vscp);
        });
    }

    DfgVertexVar* getVarVertex(AstVarScope* varp) {
        ଯଦି (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = varp->dtypep()->skipRefp();
            DfgVertexVar* ଅଚଳ vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<DfgVertexVar*>(ନୂତନ DfgVarArray{m_dfg, varp})
                      : static_cast<DfgVertexVar*>(ନୂତନ DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        ଫେରନ୍ତୁ varp->user2u().template to<DfgVertexVar*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherWritten(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଅଚଳ VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{ନୂତନ std::vector<DfgVertexVar*>{}};
        ଅଚଳ ତାର୍କିକ abort = nodep->exists([&](ଅଚଳ AstVarRef* refp) -> ତାର୍କିକ {
            ଯଦି (refp->access().isReadOnly()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            AstVarScope* ଅଚଳ vscp = refp->varScopep();
            ଯଦି (!V3Dfg::isSupported(vscp)) ଫେରନ୍ତୁ ସତ୍ୟ;
            ଯଦି (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });
        ଯଦି (abort) {
            ++m_ctx.m_nonRepVar;
            ଫେରନ୍ତୁ nullptr;
        }
        ଫେରନ୍ତୁ resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherRead(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଅଚଳ VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{ନୂତନ std::vector<DfgVertexVar*>{}};
        ଅଚଳ ତାର୍କିକ abort = nodep->exists([&](ଅଚଳ AstVarRef* refp) -> ତାର୍କିକ {
            ଯଦି (refp->access().isWriteOnly()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            AstVarScope* ଅଚଳ vscp = refp->varScopep();
            ଯଦି (!V3Dfg::isSupported(vscp)) ଫେରନ୍ତୁ ସତ୍ୟ;
            ଯଦି (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });
        ଯଦି (abort) {
            ++m_ctx.m_nonRepVar;
            ଫେରନ୍ତୁ nullptr;
        }
        ଫେରନ୍ତୁ resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherLive(ଅଚଳ CfgGraph& cfg) {
        // Run analysis
        std::unique_ptr<std::vector<AstVarScope*>> varps = V3Cfg::liveVarScopes(cfg);
        ଯଦି (!varps) {
            ++m_ctx.m_nonRepLive;
            ଫେରନ୍ତୁ nullptr;
        }

        // Convert to vertics
        ଅଚଳ VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{ନୂତନ std::vector<DfgVertexVar*>{}};
        resp->reserve(varps->size());
        ପାଇଁ (AstVarScope* ଅଚଳ varp : *varps) {
            ଯଦି (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                ଫେରନ୍ତୁ nullptr;
            }
            UASSERT_OBJ(!varp->user3SetOnce(), varp, "Live variables should be unique");
            resp->emplace_back(getVarVertex(varp));
        }
        ଫେରନ୍ତୁ resp;
    }

    // Connect inputs and outputs of a DfgLogic
    ଶୂନ୍ୟ connect(DfgLogic& vtx, ଅଚଳ std::vector<DfgVertexVar*>& iVarps,
                 ଅଚଳ std::vector<DfgVertexVar*>& oVarps) {
        // Connect inputs
        ପାଇଁ (DfgVertexVar* ଅଚଳ iVarp : iVarps) vtx.add_ନିବେଶ(iVarp);
        // Connect outputs
        ପାଇଁ (DfgVertexVar* ଅଚଳ oVarp : oVarps) {
            ଯଦି (!oVarp->srcp()) oVarp->srcp(ନୂତନ DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->addDriver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    ତାର୍କିକ convert(AstAlways* nodep) {
        ଅଚଳ VAlwaysKwd kwd = nodep->keyword();
        ଯଦି (kwd == VAlwaysKwd::CONT_ASSIGN) {
            // TODO: simplify once CFG analysis can handle arrays
            ଯଦି (ଅଚଳ AstAssignW* ଅଚଳ ap = VN_CAST(nodep->stmtsp(), AssignW)) {
                ଯଦି (ap->nextp()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                // Cannot handle assignment with timing control
                ଯଦି (ap->timingControlp()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                ଅଚଳ std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(ap);
                ଯଦି (!oVarpsp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                // Gather read variables, give up if any are not supported
                ଅଚଳ std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherRead(ap);
                ଯଦି (!iVarpsp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                // Create the DfgLogic
                DfgLogic* ଅଚଳ logicp = ନୂତନ DfgLogic{m_dfg, nodep, m_scopep, nullptr};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                // Done
                ++m_ctx.m_representable;
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }

        // Can only handle combinational logic
        ଯଦି (nodep->sentreep()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_ASSIGN) {
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<CfgGraph> cfgp = CfgGraph::build(nodep->stmtsp());
        ଯଦି (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        // Gather written variables, give up if any are not supported
        ଅଚଳ std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(nodep);
        ଯଦି (!oVarpsp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Gather read variables, give up if any are not supported
        ଅଚଳ std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherLive(*cfgp);
        ଯଦି (!iVarpsp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Create the DfgLogic
        DfgLogic* ଅଚଳ logicp = ନୂତନ DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        // Done
        ++m_ctx.m_representable;
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // VISITORS
    // Unhandled node
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    ଶୂନ୍ୟ visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    ଶୂନ୍ୟ visit(AstModule* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    ଶୂନ୍ୟ visit(AstIface* nodep) override {
        ଯଦି (!nodep->hasVirtualRef()) {
            iterateAndNextNull(nodep->stmtsp());
        } ଅନ୍ୟଥା {
            markReferenced(nodep);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଶୀର୍ଷ_Scope* nodep) override { iterate(nodep->scopep()); }
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstActive* nodep) override {
        ଯଦି (nodep->hasCombo()) {
            iterateChildren(nodep);
        } ଅନ୍ୟଥା {
            markReferenced(nodep);
        }
    }

    // Non-representable constructs
    ଶୂନ୍ୟ visit(AstCell* nodep) override { markReferenced(nodep); }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Procedure* nodep) override { markReferenced(nodep); }

    // Potentially representable constructs
    ଶୂନ୍ୟ visit(AstAlways* nodep) override {
        ଯଦି (!convert(nodep)) markReferenced(nodep);
    }

    // CONSTRUCTOR
    AstToDfgVisitor(DfgGraph& dfg, AstNetlist& ମୂଳ, V3DfgAstToDfgContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {
        iterate(&ମୂଳ);
    }
    VL_UNCOPYABLE(AstToDfgVisitor);
    VL_UNMOVABLE(AstToDfgVisitor);

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(DfgGraph& dfg, AstNetlist& ମୂଳ, V3DfgAstToDfgContext& ctx) {
        // Convert all logic under 'root'
        AstToDfgVisitor{dfg, ମୂଳ, ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        ପାଇଁ (DfgVertexVar* ଅଚଳ varp : dfg.varVertices().unlinkable()) {
            ଯଦି (varp->srcp()) ଜାରି;
            ଅଚଳ ତାର୍କିକ keep = varp->foreachSink([&](DfgVertex& d) { ଫେରନ୍ତୁ !d.is<DfgAstRd>(); });
            ଯଦି (!keep) {
                ଯେତେବେଳେ (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<DfgGraph> V3DfgPasses::astToDfg(AstNetlist& netlist, V3DfgContext& ctx) {
    DfgGraph* ଅଚଳ dfgp = ନୂତନ DfgGraph{ctx, "netlist"};
    AstToDfgVisitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    ଫେରନ୍ତୁ std::unique_ptr<DfgGraph>{dfgp};
}
