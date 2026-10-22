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
    ध्रुव std::function<DfgVertexVar*(AstVarScope*)> m_getVarVertex;
    तार्किक m_inSenItem = मिच्छ;  // Inside an AstSenItem
    तार्किक m_inLoop = मिच्छ;  // Inside an AstLoop

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
        जइ (nodep->classOrPackagep()) प्रत्यागम;
        // If target is not supported, ignore
        AstVarScope* ध्रुव vscp = nodep->varScopep();
        जइ (!V3Dfg::isSupported(vscp)) प्रत्यागम;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        DfgVertexVar* ध्रुव varp = m_getVarVertex(vscp);
        जइ (!varp) प्रत्यागम;
        // Create Ast reference vertices
        जइ (nodep->access().isReadOnly()) {
            DfgAstRd* ध्रुव astp = नूतन DfgAstRd{m_dfg, nodep, m_inSenItem, m_inLoop};
            astp->srcp(varp);
            प्रत्यागम;
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

वर्ग AstToDfgVisitor final : सार्वजनिक VNVisitor {
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
            प्रत्यागम getVarVertex(vscp);
        });
    }

    DfgVertexVar* getVarVertex(AstVarScope* varp) {
        जइ (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = varp->dtypep()->skipRefp();
            DfgVertexVar* ध्रुव vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<DfgVertexVar*>(नूतन DfgVarArray{m_dfg, varp})
                      : static_cast<DfgVertexVar*>(नूतन DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        प्रत्यागम varp->user2u().template to<DfgVertexVar*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherWritten(ध्रुव Ast_ग्रन्थिः* nodep) {
        ध्रुव VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{नूतन std::vector<DfgVertexVar*>{}};
        ध्रुव तार्किक abort = nodep->exists([&](ध्रुव AstVarRef* refp) -> तार्किक {
            जइ (refp->access().isReadOnly()) प्रत्यागम मिच्छ;
            AstVarScope* ध्रुव vscp = refp->varScopep();
            जइ (!V3Dfg::isSupported(vscp)) प्रत्यागम सच्च;
            जइ (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            प्रत्यागम मिच्छ;
        });
        जइ (abort) {
            ++m_ctx.m_nonRepVar;
            प्रत्यागम nullptr;
        }
        प्रत्यागम resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherRead(ध्रुव Ast_ग्रन्थिः* nodep) {
        ध्रुव VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{नूतन std::vector<DfgVertexVar*>{}};
        ध्रुव तार्किक abort = nodep->exists([&](ध्रुव AstVarRef* refp) -> तार्किक {
            जइ (refp->access().isWriteOnly()) प्रत्यागम मिच्छ;
            AstVarScope* ध्रुव vscp = refp->varScopep();
            जइ (!V3Dfg::isSupported(vscp)) प्रत्यागम सच्च;
            जइ (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            प्रत्यागम मिच्छ;
        });
        जइ (abort) {
            ++m_ctx.m_nonRepVar;
            प्रत्यागम nullptr;
        }
        प्रत्यागम resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherLive(ध्रुव CfgGraph& cfg) {
        // Run analysis
        std::unique_ptr<std::vector<AstVarScope*>> varps = V3Cfg::liveVarScopes(cfg);
        जइ (!varps) {
            ++m_ctx.m_nonRepLive;
            प्रत्यागम nullptr;
        }

        // Convert to vertics
        ध्रुव VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{नूतन std::vector<DfgVertexVar*>{}};
        resp->reserve(varps->size());
        प्रति (AstVarScope* ध्रुव varp : *varps) {
            जइ (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                प्रत्यागम nullptr;
            }
            UASSERT_OBJ(!varp->user3SetOnce(), varp, "Live variables should be unique");
            resp->emplace_back(getVarVertex(varp));
        }
        प्रत्यागम resp;
    }

    // Connect inputs and outputs of a DfgLogic
    शून्य connect(DfgLogic& vtx, ध्रुव std::vector<DfgVertexVar*>& iVarps,
                 ध्रुव std::vector<DfgVertexVar*>& oVarps) {
        // Connect inputs
        प्रति (DfgVertexVar* ध्रुव iVarp : iVarps) vtx.add_निवेशः(iVarp);
        // Connect outputs
        प्रति (DfgVertexVar* ध्रुव oVarp : oVarps) {
            जइ (!oVarp->srcp()) oVarp->srcp(नूतन DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->addDriver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    तार्किक convert(AstAlways* nodep) {
        ध्रुव VAlwaysKwd kwd = nodep->keyword();
        जइ (kwd == VAlwaysKwd::CONT_ASSIGN) {
            // TODO: simplify once CFG analysis can handle arrays
            जइ (ध्रुव AstAssignW* ध्रुव ap = VN_CAST(nodep->stmtsp(), AssignW)) {
                जइ (ap->nextp()) प्रत्यागम मिच्छ;
                // Cannot handle assignment with timing control
                जइ (ap->timingControlp()) प्रत्यागम मिच्छ;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                ध्रुव std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(ap);
                जइ (!oVarpsp) प्रत्यागम मिच्छ;
                // Gather read variables, give up if any are not supported
                ध्रुव std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherRead(ap);
                जइ (!iVarpsp) प्रत्यागम मिच्छ;
                // Create the DfgLogic
                DfgLogic* ध्रुव logicp = नूतन DfgLogic{m_dfg, nodep, m_scopep, nullptr};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                // Done
                ++m_ctx.m_representable;
                प्रत्यागम सच्च;
            }
        }

        // Can only handle combinational logic
        जइ (nodep->sentreep()) प्रत्यागम मिच्छ;
        जइ (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_ASSIGN) {
            प्रत्यागम मिच्छ;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<CfgGraph> cfgp = CfgGraph::build(nodep->stmtsp());
        जइ (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            प्रत्यागम मिच्छ;
        }
        // Gather written variables, give up if any are not supported
        ध्रुव std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(nodep);
        जइ (!oVarpsp) प्रत्यागम मिच्छ;
        // Gather read variables, give up if any are not supported
        ध्रुव std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherLive(*cfgp);
        जइ (!iVarpsp) प्रत्यागम मिच्छ;
        // Create the DfgLogic
        DfgLogic* ध्रुव logicp = नूतन DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        // Done
        ++m_ctx.m_representable;
        प्रत्यागम सच्च;
    }

    // VISITORS
    // Unhandled node
    शून्य visit(Ast_ग्रन्थिः* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    शून्य visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    शून्य visit(AstModule* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    शून्य visit(AstIface* nodep) override {
        जइ (!nodep->hasVirtualRef()) {
            iterateAndNextNull(nodep->stmtsp());
        } अण्णहा {
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
        जइ (nodep->hasCombo()) {
            iterateChildren(nodep);
        } अण्णहा {
            markReferenced(nodep);
        }
    }

    // Non-representable constructs
    शून्य visit(AstCell* nodep) override { markReferenced(nodep); }
    शून्य visit(Ast_ग्रन्थिः_Procedure* nodep) override { markReferenced(nodep); }

    // Potentially representable constructs
    शून्य visit(AstAlways* nodep) override {
        जइ (!convert(nodep)) markReferenced(nodep);
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
        प्रति (DfgVertexVar* ध्रुव varp : dfg.varVertices().unlinkable()) {
            जइ (varp->srcp()) अनुवर्त;
            ध्रुव तार्किक keep = varp->foreachSink([&](DfgVertex& d) { प्रत्यागम !d.is<DfgAstRd>(); });
            जइ (!keep) {
                जाव (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<DfgGraph> V3DfgPasses::astToDfg(AstNetlist& netlist, V3DfgContext& ctx) {
    DfgGraph* ध्रुव dfgp = नूतन DfgGraph{ctx, "netlist"};
    AstToDfgVisitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    प्रत्यागम std::unique_ptr<DfgGraph>{dfgp};
}
