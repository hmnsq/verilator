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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Cfg.h"
#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3Dfg.h"
#অন্তর্ভাব "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

বর্গ AstToDfgAddAstRefs final : সার্বজনিক VNVisitorConst {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    // Function to get the DfgVertexVar for a AstVarScope
    ধ্রুব std::function<DfgVertexVar*(AstVarScope*)> m_getVarVertex;
    তার্কিক m_inSenItem = অসত্য;  // Inside an AstSenItem
    তার্কিক m_inLoop = অসত্য;  // Inside an AstLoop

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

    শূন্য visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = সত্য;
        iterateChildrenConst(nodep);
    }

    শূন্য visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = সত্য;
        iterateChildrenConst(nodep);
    }

    শূন্য visit(AstVarRef* nodep) override {
        // Disguised hierarchical reference handled as external reference, ignore
        যদি (nodep->classOrPackagep()) প্রত্যাগম;
        // If target is not supported, ignore
        AstVarScope* ধ্রুব vscp = nodep->varScopep();
        যদি (!V3Dfg::isSupported(vscp)) প্রত্যাগম;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        DfgVertexVar* ধ্রুব varp = m_getVarVertex(vscp);
        যদি (!varp) প্রত্যাগম;
        // Create Ast reference vertices
        যদি (nodep->access().isReadOnly()) {
            DfgAstRd* ধ্রুব astp = নূতন DfgAstRd{m_dfg, nodep, m_inSenItem, m_inLoop};
            astp->srcp(varp);
            প্রত্যাগম;
        }
        // Mark as written from non-DFG logic
        DfgVertexVar::setHasModWrRefs(vscp);
    }

    AstToDfgAddAstRefs(DfgGraph& dfg, Ast_গ্রন্থি* nodep,
                       std::function<DfgVertexVar*(AstVarScope*)> getVarVertex)
        : m_dfg{dfg}
        , m_getVarVertex{getVarVertex} {
        iterateConst(nodep);
    }

সার্বজনিক:
    স্থির শূন্য apply(DfgGraph& dfg, Ast_গ্রন্থি* nodep,
                      std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
        AstToDfgAddAstRefs{dfg, nodep, getVarVertex};
    }
};

শূন্য V3DfgPasses::addAstRefs(DfgGraph& dfg, Ast_গ্রন্থি* nodep,
                             std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
    AstToDfgAddAstRefs::apply(dfg, nodep, getVarVertex);
}

বর্গ AstToDfgVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    // AstVarScope::user2() -> DfgVertexVar* : the corresponding variable vertex
    // AstVarScope::user3() -> bool : Already gathered - used fine grained below
    ধ্রুব VNUser2InUse m_user2InUse;

    // STATE
    DfgGraph& m_dfg;  // The graph being built
    V3DfgAstToDfgContext& m_ctx;  // The context for stats
    AstScope* m_scopep = nullptr;  // The current scope, iff T_Scoped

    // METHODS

    // Mark variables referenced under node
    শূন্য markReferenced(Ast_গ্রন্থি* nodep) {
        V3DfgPasses::addAstRefs(m_dfg, nodep, [এতত্](AstVarScope* vscp) {  //
            প্রত্যাগম getVarVertex(vscp);
        });
    }

    DfgVertexVar* getVarVertex(AstVarScope* varp) {
        যদি (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব dtypep = varp->dtypep()->skipRefp();
            DfgVertexVar* ধ্রুব vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<DfgVertexVar*>(নূতন DfgVarArray{m_dfg, varp})
                      : static_cast<DfgVertexVar*>(নূতন DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        প্রত্যাগম varp->user2u().template to<DfgVertexVar*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherWritten(ধ্রুব Ast_গ্রন্থি* nodep) {
        ধ্রুব VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{নূতন std::vector<DfgVertexVar*>{}};
        ধ্রুব তার্কিক abort = nodep->exists([&](ধ্রুব AstVarRef* refp) -> তার্কিক {
            যদি (refp->access().isReadOnly()) প্রত্যাগম অসত্য;
            AstVarScope* ধ্রুব vscp = refp->varScopep();
            যদি (!V3Dfg::isSupported(vscp)) প্রত্যাগম সত্য;
            যদি (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            প্রত্যাগম অসত্য;
        });
        যদি (abort) {
            ++m_ctx.m_nonRepVar;
            প্রত্যাগম nullptr;
        }
        প্রত্যাগম resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherRead(ধ্রুব Ast_গ্রন্থি* nodep) {
        ধ্রুব VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{নূতন std::vector<DfgVertexVar*>{}};
        ধ্রুব তার্কিক abort = nodep->exists([&](ধ্রুব AstVarRef* refp) -> তার্কিক {
            যদি (refp->access().isWriteOnly()) প্রত্যাগম অসত্য;
            AstVarScope* ধ্রুব vscp = refp->varScopep();
            যদি (!V3Dfg::isSupported(vscp)) প্রত্যাগম সত্য;
            যদি (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            প্রত্যাগম অসত্য;
        });
        যদি (abort) {
            ++m_ctx.m_nonRepVar;
            প্রত্যাগম nullptr;
        }
        প্রত্যাগম resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherLive(ধ্রুব CfgGraph& cfg) {
        // Run analysis
        std::unique_ptr<std::vector<AstVarScope*>> varps = V3Cfg::liveVarScopes(cfg);
        যদি (!varps) {
            ++m_ctx.m_nonRepLive;
            প্রত্যাগম nullptr;
        }

        // Convert to vertics
        ধ্রুব VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{নূতন std::vector<DfgVertexVar*>{}};
        resp->reserve(varps->size());
        প্রতি (AstVarScope* ধ্রুব varp : *varps) {
            যদি (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                প্রত্যাগম nullptr;
            }
            UASSERT_OBJ(!varp->user3SetOnce(), varp, "Live variables should be unique");
            resp->emplace_back(getVarVertex(varp));
        }
        প্রত্যাগম resp;
    }

    // Connect inputs and outputs of a DfgLogic
    শূন্য connect(DfgLogic& vtx, ধ্রুব std::vector<DfgVertexVar*>& iVarps,
                 ধ্রুব std::vector<DfgVertexVar*>& oVarps) {
        // Connect inputs
        প্রতি (DfgVertexVar* ধ্রুব iVarp : iVarps) vtx.add_প্রবেশ(iVarp);
        // Connect outputs
        প্রতি (DfgVertexVar* ধ্রুব oVarp : oVarps) {
            যদি (!oVarp->srcp()) oVarp->srcp(নূতন DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->addDriver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    তার্কিক convert(AstAlways* nodep) {
        ধ্রুব VAlwaysKwd kwd = nodep->keyword();
        যদি (kwd == VAlwaysKwd::CONT_ASSIGN) {
            // TODO: simplify once CFG analysis can handle arrays
            যদি (ধ্রুব AstAssignW* ধ্রুব ap = VN_CAST(nodep->stmtsp(), AssignW)) {
                যদি (ap->nextp()) প্রত্যাগম অসত্য;
                // Cannot handle assignment with timing control
                যদি (ap->timingControlp()) প্রত্যাগম অসত্য;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                ধ্রুব std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(ap);
                যদি (!oVarpsp) প্রত্যাগম অসত্য;
                // Gather read variables, give up if any are not supported
                ধ্রুব std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherRead(ap);
                যদি (!iVarpsp) প্রত্যাগম অসত্য;
                // Create the DfgLogic
                DfgLogic* ধ্রুব logicp = নূতন DfgLogic{m_dfg, nodep, m_scopep, nullptr};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                // Done
                ++m_ctx.m_representable;
                প্রত্যাগম সত্য;
            }
        }

        // Can only handle combinational logic
        যদি (nodep->sentreep()) প্রত্যাগম অসত্য;
        যদি (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_ASSIGN) {
            প্রত্যাগম অসত্য;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<CfgGraph> cfgp = CfgGraph::build(nodep->stmtsp());
        যদি (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            প্রত্যাগম অসত্য;
        }
        // Gather written variables, give up if any are not supported
        ধ্রুব std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(nodep);
        যদি (!oVarpsp) প্রত্যাগম অসত্য;
        // Gather read variables, give up if any are not supported
        ধ্রুব std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherLive(*cfgp);
        যদি (!iVarpsp) প্রত্যাগম অসত্য;
        // Create the DfgLogic
        DfgLogic* ধ্রুব logicp = নূতন DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        // Done
        ++m_ctx.m_representable;
        প্রত্যাগম সত্য;
    }

    // VISITORS
    // Unhandled node
    শূন্য visit(Ast_গ্রন্থি* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    শূন্য visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    শূন্য visit(AstModule* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    শূন্য visit(AstIface* nodep) override {
        যদি (!nodep->hasVirtualRef()) {
            iterateAndNextNull(nodep->stmtsp());
        } অন্যথা {
            markReferenced(nodep);
        }
    }
    শূন্য visit(Ast_শীর্ষ_Scope* nodep) override { iterate(nodep->scopep()); }
    শূন্য visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstActive* nodep) override {
        যদি (nodep->hasCombo()) {
            iterateChildren(nodep);
        } অন্যথা {
            markReferenced(nodep);
        }
    }

    // Non-representable constructs
    শূন্য visit(AstCell* nodep) override { markReferenced(nodep); }
    শূন্য visit(Ast_গ্রন্থি_Procedure* nodep) override { markReferenced(nodep); }

    // Potentially representable constructs
    শূন্য visit(AstAlways* nodep) override {
        যদি (!convert(nodep)) markReferenced(nodep);
    }

    // CONSTRUCTOR
    AstToDfgVisitor(DfgGraph& dfg, AstNetlist& মূল, V3DfgAstToDfgContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {
        iterate(&মূল);
    }
    VL_UNCOPYABLE(AstToDfgVisitor);
    VL_UNMOVABLE(AstToDfgVisitor);

সার্বজনিক:
    স্থির শূন্য apply(DfgGraph& dfg, AstNetlist& মূল, V3DfgAstToDfgContext& ctx) {
        // Convert all logic under 'root'
        AstToDfgVisitor{dfg, মূল, ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        প্রতি (DfgVertexVar* ধ্রুব varp : dfg.varVertices().unlinkable()) {
            যদি (varp->srcp()) অনুবর্ত;
            ধ্রুব তার্কিক keep = varp->foreachSink([&](DfgVertex& d) { প্রত্যাগম !d.is<DfgAstRd>(); });
            যদি (!keep) {
                যাবত্ (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<DfgGraph> V3DfgPasses::astToDfg(AstNetlist& netlist, V3DfgContext& ctx) {
    DfgGraph* ধ্রুব dfgp = নূতন DfgGraph{ctx, "netlist"};
    AstToDfgVisitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    প্রত্যাগম std::unique_ptr<DfgGraph>{dfgp};
}
