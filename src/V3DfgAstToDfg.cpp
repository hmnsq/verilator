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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Cfg.h"
#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3Dfg.h"
#അന്തര്ഭാവ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

വർഗ്ഗം AstToDfgAddAstRefs final : പൊതു VNVisitorConst {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    // Function to get the DfgVertexVar for a AstVarScope
    സ്ഥിരം std::function<DfgVertexVar*(AstVarScope*)> m_getVarVertex;
    തർക്കം m_inSenItem = അസത്യം;  // Inside an AstSenItem
    തർക്കം m_inLoop = അസത്യം;  // Inside an AstLoop

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

    ശൂന്യം visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = സത്യം;
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = സത്യം;
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(AstVarRef* nodep) override {
        // Disguised hierarchical reference handled as external reference, ignore
        എങ്കിൽ (nodep->classOrPackagep()) തിരികെ;
        // If target is not supported, ignore
        AstVarScope* സ്ഥിരം vscp = nodep->varScopep();
        എങ്കിൽ (!V3Dfg::isSupported(vscp)) തിരികെ;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        DfgVertexVar* സ്ഥിരം varp = m_getVarVertex(vscp);
        എങ്കിൽ (!varp) തിരികെ;
        // Create Ast reference vertices
        എങ്കിൽ (nodep->access().isReadOnly()) {
            DfgAstRd* സ്ഥിരം astp = പുതിയ DfgAstRd{m_dfg, nodep, m_inSenItem, m_inLoop};
            astp->srcp(varp);
            തിരികെ;
        }
        // Mark as written from non-DFG logic
        DfgVertexVar::setHasModWrRefs(vscp);
    }

    AstToDfgAddAstRefs(DfgGraph& dfg, Ast_കണ്ണി* nodep,
                       std::function<DfgVertexVar*(AstVarScope*)> getVarVertex)
        : m_dfg{dfg}
        , m_getVarVertex{getVarVertex} {
        iterateConst(nodep);
    }

പൊതു:
    സ്ഥിതം ശൂന്യം apply(DfgGraph& dfg, Ast_കണ്ണി* nodep,
                      std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
        AstToDfgAddAstRefs{dfg, nodep, getVarVertex};
    }
};

ശൂന്യം V3DfgPasses::addAstRefs(DfgGraph& dfg, Ast_കണ്ണി* nodep,
                             std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
    AstToDfgAddAstRefs::apply(dfg, nodep, getVarVertex);
}

വർഗ്ഗം AstToDfgVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // AstVarScope::user2() -> DfgVertexVar* : the corresponding variable vertex
    // AstVarScope::user3() -> bool : Already gathered - used fine grained below
    സ്ഥിരം VNUser2InUse m_user2InUse;

    // STATE
    DfgGraph& m_dfg;  // The graph being built
    V3DfgAstToDfgContext& m_ctx;  // The context for stats
    AstScope* m_scopep = nullptr;  // The current scope, iff T_Scoped

    // METHODS

    // Mark variables referenced under node
    ശൂന്യം markReferenced(Ast_കണ്ണി* nodep) {
        V3DfgPasses::addAstRefs(m_dfg, nodep, [ഇത്](AstVarScope* vscp) {  //
            തിരികെ getVarVertex(vscp);
        });
    }

    DfgVertexVar* getVarVertex(AstVarScope* varp) {
        എങ്കിൽ (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = varp->dtypep()->skipRefp();
            DfgVertexVar* സ്ഥിരം vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<DfgVertexVar*>(പുതിയ DfgVarArray{m_dfg, varp})
                      : static_cast<DfgVertexVar*>(പുതിയ DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        തിരികെ varp->user2u().template to<DfgVertexVar*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherWritten(സ്ഥിരം Ast_കണ്ണി* nodep) {
        സ്ഥിരം VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{പുതിയ std::vector<DfgVertexVar*>{}};
        സ്ഥിരം തർക്കം abort = nodep->exists([&](സ്ഥിരം AstVarRef* refp) -> തർക്കം {
            എങ്കിൽ (refp->access().isReadOnly()) തിരികെ അസത്യം;
            AstVarScope* സ്ഥിരം vscp = refp->varScopep();
            എങ്കിൽ (!V3Dfg::isSupported(vscp)) തിരികെ സത്യം;
            എങ്കിൽ (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            തിരികെ അസത്യം;
        });
        എങ്കിൽ (abort) {
            ++m_ctx.m_nonRepVar;
            തിരികെ nullptr;
        }
        തിരികെ resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherRead(സ്ഥിരം Ast_കണ്ണി* nodep) {
        സ്ഥിരം VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{പുതിയ std::vector<DfgVertexVar*>{}};
        സ്ഥിരം തർക്കം abort = nodep->exists([&](സ്ഥിരം AstVarRef* refp) -> തർക്കം {
            എങ്കിൽ (refp->access().isWriteOnly()) തിരികെ അസത്യം;
            AstVarScope* സ്ഥിരം vscp = refp->varScopep();
            എങ്കിൽ (!V3Dfg::isSupported(vscp)) തിരികെ സത്യം;
            എങ്കിൽ (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            തിരികെ അസത്യം;
        });
        എങ്കിൽ (abort) {
            ++m_ctx.m_nonRepVar;
            തിരികെ nullptr;
        }
        തിരികെ resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherLive(സ്ഥിരം CfgGraph& cfg) {
        // Run analysis
        std::unique_ptr<std::vector<AstVarScope*>> varps = V3Cfg::liveVarScopes(cfg);
        എങ്കിൽ (!varps) {
            ++m_ctx.m_nonRepLive;
            തിരികെ nullptr;
        }

        // Convert to vertics
        സ്ഥിരം VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{പുതിയ std::vector<DfgVertexVar*>{}};
        resp->reserve(varps->size());
        വേണ്ടി (AstVarScope* സ്ഥിരം varp : *varps) {
            എങ്കിൽ (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                തിരികെ nullptr;
            }
            UASSERT_OBJ(!varp->user3SetOnce(), varp, "Live variables should be unique");
            resp->emplace_back(getVarVertex(varp));
        }
        തിരികെ resp;
    }

    // Connect inputs and outputs of a DfgLogic
    ശൂന്യം connect(DfgLogic& vtx, സ്ഥിരം std::vector<DfgVertexVar*>& iVarps,
                 സ്ഥിരം std::vector<DfgVertexVar*>& oVarps) {
        // Connect inputs
        വേണ്ടി (DfgVertexVar* സ്ഥിരം iVarp : iVarps) vtx.add_നിവേശം(iVarp);
        // Connect outputs
        വേണ്ടി (DfgVertexVar* സ്ഥിരം oVarp : oVarps) {
            എങ്കിൽ (!oVarp->srcp()) oVarp->srcp(പുതിയ DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->addDriver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    തർക്കം convert(AstAlways* nodep) {
        സ്ഥിരം VAlwaysKwd kwd = nodep->keyword();
        എങ്കിൽ (kwd == VAlwaysKwd::CONT_ASSIGN) {
            // TODO: simplify once CFG analysis can handle arrays
            എങ്കിൽ (സ്ഥിരം AstAssignW* സ്ഥിരം ap = VN_CAST(nodep->stmtsp(), AssignW)) {
                എങ്കിൽ (ap->nextp()) തിരികെ അസത്യം;
                // Cannot handle assignment with timing control
                എങ്കിൽ (ap->timingControlp()) തിരികെ അസത്യം;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                സ്ഥിരം std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(ap);
                എങ്കിൽ (!oVarpsp) തിരികെ അസത്യം;
                // Gather read variables, give up if any are not supported
                സ്ഥിരം std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherRead(ap);
                എങ്കിൽ (!iVarpsp) തിരികെ അസത്യം;
                // Create the DfgLogic
                DfgLogic* സ്ഥിരം logicp = പുതിയ DfgLogic{m_dfg, nodep, m_scopep, nullptr};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                // Done
                ++m_ctx.m_representable;
                തിരികെ സത്യം;
            }
        }

        // Can only handle combinational logic
        എങ്കിൽ (nodep->sentreep()) തിരികെ അസത്യം;
        എങ്കിൽ (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_ASSIGN) {
            തിരികെ അസത്യം;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<CfgGraph> cfgp = CfgGraph::build(nodep->stmtsp());
        എങ്കിൽ (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            തിരികെ അസത്യം;
        }
        // Gather written variables, give up if any are not supported
        സ്ഥിരം std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(nodep);
        എങ്കിൽ (!oVarpsp) തിരികെ അസത്യം;
        // Gather read variables, give up if any are not supported
        സ്ഥിരം std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherLive(*cfgp);
        എങ്കിൽ (!iVarpsp) തിരികെ അസത്യം;
        // Create the DfgLogic
        DfgLogic* സ്ഥിരം logicp = പുതിയ DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        // Done
        ++m_ctx.m_representable;
        തിരികെ സത്യം;
    }

    // VISITORS
    // Unhandled node
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    ശൂന്യം visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    ശൂന്യം visit(AstModule* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    ശൂന്യം visit(AstIface* nodep) override {
        എങ്കിൽ (!nodep->hasVirtualRef()) {
            iterateAndNextNull(nodep->stmtsp());
        } അല്ലെങ്കിൽ {
            markReferenced(nodep);
        }
    }
    ശൂന്യം visit(Ast_മുകൾ_Scope* nodep) override { iterate(nodep->scopep()); }
    ശൂന്യം visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstActive* nodep) override {
        എങ്കിൽ (nodep->hasCombo()) {
            iterateChildren(nodep);
        } അല്ലെങ്കിൽ {
            markReferenced(nodep);
        }
    }

    // Non-representable constructs
    ശൂന്യം visit(AstCell* nodep) override { markReferenced(nodep); }
    ശൂന്യം visit(Ast_കണ്ണി_Procedure* nodep) override { markReferenced(nodep); }

    // Potentially representable constructs
    ശൂന്യം visit(AstAlways* nodep) override {
        എങ്കിൽ (!convert(nodep)) markReferenced(nodep);
    }

    // CONSTRUCTOR
    AstToDfgVisitor(DfgGraph& dfg, AstNetlist& വേര്, V3DfgAstToDfgContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {
        iterate(&വേര്);
    }
    VL_UNCOPYABLE(AstToDfgVisitor);
    VL_UNMOVABLE(AstToDfgVisitor);

പൊതു:
    സ്ഥിതം ശൂന്യം apply(DfgGraph& dfg, AstNetlist& വേര്, V3DfgAstToDfgContext& ctx) {
        // Convert all logic under 'root'
        AstToDfgVisitor{dfg, വേര്, ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        വേണ്ടി (DfgVertexVar* സ്ഥിരം varp : dfg.varVertices().unlinkable()) {
            എങ്കിൽ (varp->srcp()) തുടരുക;
            സ്ഥിരം തർക്കം keep = varp->foreachSink([&](DfgVertex& d) { തിരികെ !d.is<DfgAstRd>(); });
            എങ്കിൽ (!keep) {
                ആകുമ്പോൾ (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<DfgGraph> V3DfgPasses::astToDfg(AstNetlist& netlist, V3DfgContext& ctx) {
    DfgGraph* സ്ഥിരം dfgp = പുതിയ DfgGraph{ctx, "netlist"};
    AstToDfgVisitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    തിരികെ std::unique_ptr<DfgGraph>{dfgp};
}
