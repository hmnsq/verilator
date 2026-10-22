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

শ্রেণী AstToDfgAddAstRefs final : সর্বজনীন VNVisitorConst {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    // Function to get the DfgVertexVar for a AstVarScope
    ধ্রুবক std::function<DfgVertexVar*(AstVarScope*)> m_getVarVertex;
    যৌক্তিক m_inSenItem = মিথ্যা;  // Inside an AstSenItem
    যৌক্তিক m_inLoop = মিথ্যা;  // Inside an AstLoop

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
        যদি (nodep->classOrPackagep()) ফেরত;
        // If target is not supported, ignore
        AstVarScope* ধ্রুবক vscp = nodep->varScopep();
        যদি (!V3Dfg::isSupported(vscp)) ফেরত;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        DfgVertexVar* ধ্রুবক varp = m_getVarVertex(vscp);
        যদি (!varp) ফেরত;
        // Create Ast reference vertices
        যদি (nodep->access().isReadOnly()) {
            DfgAstRd* ধ্রুবক astp = নতুন DfgAstRd{m_dfg, nodep, m_inSenItem, m_inLoop};
            astp->srcp(varp);
            ফেরত;
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

সর্বজনীন:
    স্থির শূন্য apply(DfgGraph& dfg, Ast_গ্রন্থি* nodep,
                      std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
        AstToDfgAddAstRefs{dfg, nodep, getVarVertex};
    }
};

শূন্য V3DfgPasses::addAstRefs(DfgGraph& dfg, Ast_গ্রন্থি* nodep,
                             std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
    AstToDfgAddAstRefs::apply(dfg, nodep, getVarVertex);
}

শ্রেণী AstToDfgVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE
    // AstVarScope::user2() -> DfgVertexVar* : the corresponding variable vertex
    // AstVarScope::user3() -> bool : Already gathered - used fine grained below
    ধ্রুবক VNUser2InUse m_user2InUse;

    // STATE
    DfgGraph& m_dfg;  // The graph being built
    V3DfgAstToDfgContext& m_ctx;  // The context for stats
    AstScope* m_scopep = nullptr;  // The current scope, iff T_Scoped

    // METHODS

    // Mark variables referenced under node
    শূন্য markReferenced(Ast_গ্রন্থি* nodep) {
        V3DfgPasses::addAstRefs(m_dfg, nodep, [এই](AstVarScope* vscp) {  //
            ফেরত getVarVertex(vscp);
        });
    }

    DfgVertexVar* getVarVertex(AstVarScope* varp) {
        যদি (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep = varp->dtypep()->skipRefp();
            DfgVertexVar* ধ্রুবক vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<DfgVertexVar*>(নতুন DfgVarArray{m_dfg, varp})
                      : static_cast<DfgVertexVar*>(নতুন DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        ফেরত varp->user2u().template to<DfgVertexVar*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherWritten(ধ্রুবক Ast_গ্রন্থি* nodep) {
        ধ্রুবক VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{নতুন std::vector<DfgVertexVar*>{}};
        ধ্রুবক যৌক্তিক abort = nodep->exists([&](ধ্রুবক AstVarRef* refp) -> যৌক্তিক {
            যদি (refp->access().isReadOnly()) ফেরত মিথ্যা;
            AstVarScope* ধ্রুবক vscp = refp->varScopep();
            যদি (!V3Dfg::isSupported(vscp)) ফেরত সত্য;
            যদি (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            ফেরত মিথ্যা;
        });
        যদি (abort) {
            ++m_ctx.m_nonRepVar;
            ফেরত nullptr;
        }
        ফেরত resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherRead(ধ্রুবক Ast_গ্রন্থি* nodep) {
        ধ্রুবক VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{নতুন std::vector<DfgVertexVar*>{}};
        ধ্রুবক যৌক্তিক abort = nodep->exists([&](ধ্রুবক AstVarRef* refp) -> যৌক্তিক {
            যদি (refp->access().isWriteOnly()) ফেরত মিথ্যা;
            AstVarScope* ধ্রুবক vscp = refp->varScopep();
            যদি (!V3Dfg::isSupported(vscp)) ফেরত সত্য;
            যদি (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            ফেরত মিথ্যা;
        });
        যদি (abort) {
            ++m_ctx.m_nonRepVar;
            ফেরত nullptr;
        }
        ফেরত resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherLive(ধ্রুবক CfgGraph& cfg) {
        // Run analysis
        std::unique_ptr<std::vector<AstVarScope*>> varps = V3Cfg::liveVarScopes(cfg);
        যদি (!varps) {
            ++m_ctx.m_nonRepLive;
            ফেরত nullptr;
        }

        // Convert to vertics
        ধ্রুবক VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{নতুন std::vector<DfgVertexVar*>{}};
        resp->reserve(varps->আকার_2());
        জন্য (AstVarScope* ধ্রুবক varp : *varps) {
            যদি (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                ফেরত nullptr;
            }
            UASSERT_OBJ(!varp->user3SetOnce(), varp, "Live variables should be unique");
            resp->emplace_back(getVarVertex(varp));
        }
        ফেরত resp;
    }

    // Connect inputs and outputs of a DfgLogic
    শূন্য connect(DfgLogic& vtx, ধ্রুবক std::vector<DfgVertexVar*>& iVarps,
                 ধ্রুবক std::vector<DfgVertexVar*>& oVarps) {
        // Connect inputs
        জন্য (DfgVertexVar* ধ্রুবক iVarp : iVarps) vtx.add_প্রবেশ(iVarp);
        // Connect outputs
        জন্য (DfgVertexVar* ধ্রুবক oVarp : oVarps) {
            যদি (!oVarp->srcp()) oVarp->srcp(নতুন DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->addDriver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    যৌক্তিক convert(AstAlways* nodep) {
        ধ্রুবক VAlwaysKwd kwd = nodep->keyword();
        যদি (kwd == VAlwaysKwd::CONT_ASSIGN) {
            // TODO: simplify once CFG analysis can handle arrays
            যদি (ধ্রুবক AstAssignW* ধ্রুবক ap = VN_CAST(nodep->stmtsp(), AssignW)) {
                যদি (ap->nextp()) ফেরত মিথ্যা;
                // Cannot handle assignment with timing control
                যদি (ap->timingControlp()) ফেরত মিথ্যা;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                ধ্রুবক std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(ap);
                যদি (!oVarpsp) ফেরত মিথ্যা;
                // Gather read variables, give up if any are not supported
                ধ্রুবক std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherRead(ap);
                যদি (!iVarpsp) ফেরত মিথ্যা;
                // Create the DfgLogic
                DfgLogic* ধ্রুবক logicp = নতুন DfgLogic{m_dfg, nodep, m_scopep, nullptr};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                // Done
                ++m_ctx.m_representable;
                ফেরত সত্য;
            }
        }

        // Can only handle combinational logic
        যদি (nodep->sentreep()) ফেরত মিথ্যা;
        যদি (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_ASSIGN) {
            ফেরত মিথ্যা;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<CfgGraph> cfgp = CfgGraph::build(nodep->stmtsp());
        যদি (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            ফেরত মিথ্যা;
        }
        // Gather written variables, give up if any are not supported
        ধ্রুবক std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(nodep);
        যদি (!oVarpsp) ফেরত মিথ্যা;
        // Gather read variables, give up if any are not supported
        ধ্রুবক std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherLive(*cfgp);
        যদি (!iVarpsp) ফেরত মিথ্যা;
        // Create the DfgLogic
        DfgLogic* ধ্রুবক logicp = নতুন DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        // Done
        ++m_ctx.m_representable;
        ফেরত সত্য;
    }

    // VISITORS
    // Unhandled node
    শূন্য visit(Ast_গ্রন্থি* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    শূন্য visit(AstNetlist* nodep) override { iterate_And_পরবর্তী_Null(nodep->modulesp()); }
    শূন্য visit(AstModule* nodep) override { iterate_And_পরবর্তী_Null(nodep->stmtsp()); }
    শূন্য visit(AstIface* nodep) override {
        যদি (!nodep->hasVirtualRef()) {
            iterate_And_পরবর্তী_Null(nodep->stmtsp());
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

সর্বজনীন:
    স্থির শূন্য apply(DfgGraph& dfg, AstNetlist& মূল, V3DfgAstToDfgContext& ctx) {
        // Convert all logic under 'root'
        AstToDfgVisitor{dfg, মূল, ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        জন্য (DfgVertexVar* ধ্রুবক varp : dfg.varVertices().unlinkable()) {
            যদি (varp->srcp()) চালু;
            ধ্রুবক যৌক্তিক keep = varp->foreachSink([&](DfgVertex& d) { ফেরত !d.is<DfgAstRd>(); });
            যদি (!keep) {
                যখন (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<DfgGraph> V3DfgPasses::astToDfg(AstNetlist& netlist, V3DfgContext& ctx) {
    DfgGraph* ধ্রুবক dfgp = নতুন DfgGraph{ctx, "netlist"};
    AstToDfgVisitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    ফেরত std::unique_ptr<DfgGraph>{dfgp};
}
