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

वग्ग AstToDfgAddAstRefs final : सार्वजनिक VNVisitorConst {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    // Function to get the DfgVertexVar for a AstVarScope
    ध्रुव std::function<DfgVertexVar*(AstVarScope*)> m_getVarVertex;
    तार्किक m_inSenItem = मिच्छा;  // Inside an AstSenItem
    तार्किक m_inLoop = मिच्छा;  // Inside an AstLoop

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

    शून्य visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = सच्च;
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = सच्च;
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstVarRef* nodep) override {
        // Disguised hierarchical reference handled as external reference, ignore
        सचे (nodep->classOrPackagep()) पटिनिवत्त;
        // If target is not supported, ignore
        AstVarScope* ध्रुव vscp = nodep->varScopep();
        सचे (!V3Dfg::isSupported(vscp)) पटिनिवत्त;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        DfgVertexVar* ध्रुव varp = m_getVarVertex(vscp);
        सचे (!varp) पटिनिवत्त;
        // Create Ast reference vertices
        सचे (nodep->access().isReadOnly()) {
            DfgAstRd* ध्रुव astp = नूतन DfgAstRd{m_dfg, nodep, m_inSenItem, m_inLoop};
            astp->srcp(varp);
            पटिनिवत्त;
        }
        // Mark as written from non-DFG logic
        DfgVertexVar::setHasModWrRefs(vscp);
    }

    AstToDfgAddAstRefs(DfgGraph& dfg, Ast_ग्रन्थिः* nodep,
                       std::function<DfgVertexVar*(AstVarScope*)> getVarVertex)
        : m_dfg{dfg}
        , m_getVarVertex{getVarVertex} {
        iterateConst(nodep);
    }

सार्वजनिक:
    स्थिर शून्य apply(DfgGraph& dfg, Ast_ग्रन्थिः* nodep,
                      std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
        AstToDfgAddAstRefs{dfg, nodep, getVarVertex};
    }
};

शून्य V3DfgPasses::addAstRefs(DfgGraph& dfg, Ast_ग्रन्थिः* nodep,
                             std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
    AstToDfgAddAstRefs::apply(dfg, nodep, getVarVertex);
}

वग्ग AstToDfgVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstVarScope::user2() -> DfgVertexVar* : the corresponding variable vertex
    // AstVarScope::user3() -> bool : Already gathered - used fine grained below
    ध्रुव VNUser2InUse m_user2InUse;

    // STATE
    DfgGraph& m_dfg;  // The graph being built
    V3DfgAstToDfgContext& m_ctx;  // The context for stats
    AstScope* m_scopep = nullptr;  // The current scope, iff T_Scoped

    // METHODS

    // Mark variables referenced under node
    शून्य markReferenced(Ast_ग्रन्थिः* nodep) {
        V3DfgPasses::addAstRefs(m_dfg, nodep, [एतत्](AstVarScope* vscp) {  //
            पटिनिवत्त getVarVertex(vscp);
        });
    }

    DfgVertexVar* getVarVertex(AstVarScope* varp) {
        सचे (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = varp->dtypep()->skipRefp();
            DfgVertexVar* ध्रुव vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<DfgVertexVar*>(नूतन DfgVarArray{m_dfg, varp})
                      : static_cast<DfgVertexVar*>(नूतन DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        पटिनिवत्त varp->user2u().template to<DfgVertexVar*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherWritten(ध्रुव Ast_ग्रन्थिः* nodep) {
        ध्रुव VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{नूतन std::vector<DfgVertexVar*>{}};
        ध्रुव तार्किक abort = nodep->exists([&](ध्रुव AstVarRef* refp) -> तार्किक {
            सचे (refp->access().isReadOnly()) पटिनिवत्त मिच्छा;
            AstVarScope* ध्रुव vscp = refp->varScopep();
            सचे (!V3Dfg::isSupported(vscp)) पटिनिवत्त सच्च;
            सचे (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            पटिनिवत्त मिच्छा;
        });
        सचे (abort) {
            ++m_ctx.m_nonRepVar;
            पटिनिवत्त nullptr;
        }
        पटिनिवत्त resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherRead(ध्रुव Ast_ग्रन्थिः* nodep) {
        ध्रुव VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{नूतन std::vector<DfgVertexVar*>{}};
        ध्रुव तार्किक abort = nodep->exists([&](ध्रुव AstVarRef* refp) -> तार्किक {
            सचे (refp->access().isWriteOnly()) पटिनिवत्त मिच्छा;
            AstVarScope* ध्रुव vscp = refp->varScopep();
            सचे (!V3Dfg::isSupported(vscp)) पटिनिवत्त सच्च;
            सचे (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            पटिनिवत्त मिच्छा;
        });
        सचे (abort) {
            ++m_ctx.m_nonRepVar;
            पटिनिवत्त nullptr;
        }
        पटिनिवत्त resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherLive(ध्रुव CfgGraph& cfg) {
        // Run analysis
        std::unique_ptr<std::vector<AstVarScope*>> varps = V3Cfg::liveVarScopes(cfg);
        सचे (!varps) {
            ++m_ctx.m_nonRepLive;
            पटिनिवत्त nullptr;
        }

        // Convert to vertics
        ध्रुव VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{नूतन std::vector<DfgVertexVar*>{}};
        resp->reserve(varps->size());
        अत्थाय (AstVarScope* ध्रुव varp : *varps) {
            सचे (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                पटिनिवत्त nullptr;
            }
            UASSERT_OBJ(!varp->user3SetOnce(), varp, "Live variables should be unique");
            resp->emplace_back(getVarVertex(varp));
        }
        पटिनिवत्त resp;
    }

    // Connect inputs and outputs of a DfgLogic
    शून्य connect(DfgLogic& vtx, ध्रुव std::vector<DfgVertexVar*>& iVarps,
                 ध्रुव std::vector<DfgVertexVar*>& oVarps) {
        // Connect inputs
        अत्थाय (DfgVertexVar* ध्रुव iVarp : iVarps) vtx.add_निवेशः(iVarp);
        // Connect outputs
        अत्थाय (DfgVertexVar* ध्रुव oVarp : oVarps) {
            सचे (!oVarp->srcp()) oVarp->srcp(नूतन DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->addDriver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    तार्किक convert(AstAlways* nodep) {
        ध्रुव VAlwaysKwd kwd = nodep->keyword();
        सचे (kwd == VAlwaysKwd::CONT_ASSIGN) {
            // TODO: simplify once CFG analysis can handle arrays
            सचे (ध्रुव AstAssignW* ध्रुव ap = VN_CAST(nodep->stmtsp(), AssignW)) {
                सचे (ap->nextp()) पटिनिवत्त मिच्छा;
                // Cannot handle assignment with timing control
                सचे (ap->timingControlp()) पटिनिवत्त मिच्छा;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                ध्रुव std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(ap);
                सचे (!oVarpsp) पटिनिवत्त मिच्छा;
                // Gather read variables, give up if any are not supported
                ध्रुव std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherRead(ap);
                सचे (!iVarpsp) पटिनिवत्त मिच्छा;
                // Create the DfgLogic
                DfgLogic* ध्रुव logicp = नूतन DfgLogic{m_dfg, nodep, m_scopep, nullptr};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                // Done
                ++m_ctx.m_representable;
                पटिनिवत्त सच्च;
            }
        }

        // Can only handle combinational logic
        सचे (nodep->sentreep()) पटिनिवत्त मिच्छा;
        सचे (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_ASSIGN) {
            पटिनिवत्त मिच्छा;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<CfgGraph> cfgp = CfgGraph::build(nodep->stmtsp());
        सचे (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            पटिनिवत्त मिच्छा;
        }
        // Gather written variables, give up if any are not supported
        ध्रुव std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(nodep);
        सचे (!oVarpsp) पटिनिवत्त मिच्छा;
        // Gather read variables, give up if any are not supported
        ध्रुव std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherLive(*cfgp);
        सचे (!iVarpsp) पटिनिवत्त मिच्छा;
        // Create the DfgLogic
        DfgLogic* ध्रुव logicp = नूतन DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        // Done
        ++m_ctx.m_representable;
        पटिनिवत्त सच्च;
    }

    // VISITORS
    // Unhandled node
    शून्य visit(Ast_ग्रन्थिः* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    शून्य visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    शून्य visit(AstModule* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    शून्य visit(AstIface* nodep) override {
        सचे (!nodep->hasVirtualRef()) {
            iterateAndNextNull(nodep->stmtsp());
        } अञ्ञथा {
            markReferenced(nodep);
        }
    }
    शून्य visit(Ast_शिखरम्_Scope* nodep) override { iterate(nodep->scopep()); }
    शून्य visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstActive* nodep) override {
        सचे (nodep->hasCombo()) {
            iterateChildren(nodep);
        } अञ्ञथा {
            markReferenced(nodep);
        }
    }

    // Non-representable constructs
    शून्य visit(AstCell* nodep) override { markReferenced(nodep); }
    शून्य visit(Ast_ग्रन्थिः_Procedure* nodep) override { markReferenced(nodep); }

    // Potentially representable constructs
    शून्य visit(AstAlways* nodep) override {
        सचे (!convert(nodep)) markReferenced(nodep);
    }

    // CONSTRUCTOR
    AstToDfgVisitor(DfgGraph& dfg, AstNetlist& मूलम्, V3DfgAstToDfgContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {
        iterate(&मूलम्);
    }
    VL_UNCOPYABLE(AstToDfgVisitor);
    VL_UNMOVABLE(AstToDfgVisitor);

सार्वजनिक:
    स्थिर शून्य apply(DfgGraph& dfg, AstNetlist& मूलम्, V3DfgAstToDfgContext& ctx) {
        // Convert all logic under 'root'
        AstToDfgVisitor{dfg, मूलम्, ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        अत्थाय (DfgVertexVar* ध्रुव varp : dfg.varVertices().unlinkable()) {
            सचे (varp->srcp()) अनुवर्त;
            ध्रुव तार्किक keep = varp->foreachSink([&](DfgVertex& d) { पटिनिवत्त !d.is<DfgAstRd>(); });
            सचे (!keep) {
                याव (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<DfgGraph> V3DfgPasses::astToDfg(AstNetlist& netlist, V3DfgContext& ctx) {
    DfgGraph* ध्रुव dfgp = नूतन DfgGraph{ctx, "netlist"};
    AstToDfgVisitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    पटिनिवत्त std::unique_ptr<DfgGraph>{dfgp};
}
