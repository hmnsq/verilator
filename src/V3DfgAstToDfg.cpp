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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Cfg.h"
#অন্তর্ভাৱ "V3Const.h"
#অন্তর্ভাৱ "V3Dfg.h"
#অন্তর্ভাৱ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

শ্ৰেণী AstToDfgAddAstRefs final : সার্ৱজনিক VNVisitorConst {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    // Function to get the DfgVertexVar for a AstVarScope
    ধ্রুৱ std::function<DfgVertexVar*(AstVarScope*)> m_getVarVertex;
    তার্কিক m_inSenItem = মিছা;  // Inside an AstSenItem
    তার্কিক m_inLoop = মিছা;  // Inside an AstLoop

    // VISITORS
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildrenConst(nodep); }

    শূন্য visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = সঁচা;
        iterateChildrenConst(nodep);
    }

    শূন্য visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = সঁচা;
        iterateChildrenConst(nodep);
    }

    শূন্য visit(AstVarRef* nodep) override {
        // Disguised hierarchical reference handled as external reference, ignore
        যদি (nodep->classOrPackagep()) ঘূৰাই;
        // If target is not supported, ignore
        AstVarScope* ধ্রুৱ vscp = nodep->varScopep();
        যদি (!V3Dfg::isSupported(vscp)) ঘূৰাই;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        DfgVertexVar* ধ্রুৱ varp = m_getVarVertex(vscp);
        যদি (!varp) ঘূৰাই;
        // Create Ast reference vertices
        যদি (nodep->access().isReadOnly()) {
            DfgAstRd* ধ্রুৱ astp = নূতন DfgAstRd{m_dfg, nodep, m_inSenItem, m_inLoop};
            astp->srcp(varp);
            ঘূৰাই;
        }
        // Mark as written from non-DFG logic
        DfgVertexVar::setHasModWrRefs(vscp);
    }

    AstToDfgAddAstRefs(DfgGraph& dfg, Ast_গ্ৰন্থি* nodep,
                       std::function<DfgVertexVar*(AstVarScope*)> getVarVertex)
        : m_dfg{dfg}
        , m_getVarVertex{getVarVertex} {
        iterateConst(nodep);
    }

সার্ৱজনিক:
    স্থির শূন্য apply(DfgGraph& dfg, Ast_গ্ৰন্থি* nodep,
                      std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
        AstToDfgAddAstRefs{dfg, nodep, getVarVertex};
    }
};

শূন্য V3DfgPasses::addAstRefs(DfgGraph& dfg, Ast_গ্ৰন্থি* nodep,
                             std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
    AstToDfgAddAstRefs::apply(dfg, nodep, getVarVertex);
}

শ্ৰেণী AstToDfgVisitor final : সার্ৱজনিক VNVisitor {
    // NODE STATE
    // AstVarScope::user2() -> DfgVertexVar* : the corresponding variable vertex
    // AstVarScope::user3() -> bool : Already gathered - used fine grained below
    ধ্রুৱ VNUser2InUse m_user2InUse;

    // STATE
    DfgGraph& m_dfg;  // The graph being built
    V3DfgAstToDfgContext& m_ctx;  // The context for stats
    AstScope* m_scopep = nullptr;  // The current scope, iff T_Scoped

    // METHODS

    // Mark variables referenced under node
    শূন্য markReferenced(Ast_গ্ৰন্থি* nodep) {
        V3DfgPasses::addAstRefs(m_dfg, nodep, [এতত্](AstVarScope* vscp) {  //
            ঘূৰাই getVarVertex(vscp);
        });
    }

    DfgVertexVar* getVarVertex(AstVarScope* varp) {
        যদি (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = varp->dtypep()->skipRefp();
            DfgVertexVar* ধ্রুৱ vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<DfgVertexVar*>(নূতন DfgVarArray{m_dfg, varp})
                      : static_cast<DfgVertexVar*>(নূতন DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        ঘূৰাই varp->user2u().template to<DfgVertexVar*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherWritten(ধ্রুৱ Ast_গ্ৰন্থি* nodep) {
        ধ্রুৱ VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{নূতন std::vector<DfgVertexVar*>{}};
        ধ্রুৱ তার্কিক abort = nodep->exists([&](ধ্রুৱ AstVarRef* refp) -> তার্কিক {
            যদি (refp->access().isReadOnly()) ঘূৰাই মিছা;
            AstVarScope* ধ্রুৱ vscp = refp->varScopep();
            যদি (!V3Dfg::isSupported(vscp)) ঘূৰাই সঁচা;
            যদি (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            ঘূৰাই মিছা;
        });
        যদি (abort) {
            ++m_ctx.m_nonRepVar;
            ঘূৰাই nullptr;
        }
        ঘূৰাই resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherRead(ধ্রুৱ Ast_গ্ৰন্থি* nodep) {
        ধ্রুৱ VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{নূতন std::vector<DfgVertexVar*>{}};
        ধ্রুৱ তার্কিক abort = nodep->exists([&](ধ্রুৱ AstVarRef* refp) -> তার্কিক {
            যদি (refp->access().isWriteOnly()) ঘূৰাই মিছা;
            AstVarScope* ধ্রুৱ vscp = refp->varScopep();
            যদি (!V3Dfg::isSupported(vscp)) ঘূৰাই সঁচা;
            যদি (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            ঘূৰাই মিছা;
        });
        যদি (abort) {
            ++m_ctx.m_nonRepVar;
            ঘূৰাই nullptr;
        }
        ঘূৰাই resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherLive(ধ্রুৱ CfgGraph& cfg) {
        // Run analysis
        std::unique_ptr<std::vector<AstVarScope*>> varps = V3Cfg::liveVarScopes(cfg);
        যদি (!varps) {
            ++m_ctx.m_nonRepLive;
            ঘূৰাই nullptr;
        }

        // Convert to vertics
        ধ্রুৱ VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{নূতন std::vector<DfgVertexVar*>{}};
        resp->reserve(varps->size());
        বাবে (AstVarScope* ধ্রুৱ varp : *varps) {
            যদি (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                ঘূৰাই nullptr;
            }
            UASSERT_OBJ(!varp->user3SetOnce(), varp, "Live variables should be unique");
            resp->emplace_back(getVarVertex(varp));
        }
        ঘূৰাই resp;
    }

    // Connect inputs and outputs of a DfgLogic
    শূন্য connect(DfgLogic& vtx, ধ্রুৱ std::vector<DfgVertexVar*>& iVarps,
                 ধ্রুৱ std::vector<DfgVertexVar*>& oVarps) {
        // Connect inputs
        বাবে (DfgVertexVar* ধ্রুৱ iVarp : iVarps) vtx.add_প্ৰবেশ(iVarp);
        // Connect outputs
        বাবে (DfgVertexVar* ধ্রুৱ oVarp : oVarps) {
            যদি (!oVarp->srcp()) oVarp->srcp(নূতন DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->addDriver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    তার্কিক convert(AstAlways* nodep) {
        ধ্রুৱ VAlwaysKwd kwd = nodep->keyword();
        যদি (kwd == VAlwaysKwd::CONT_ASSIGN) {
            // TODO: simplify once CFG analysis can handle arrays
            যদি (ধ্রুৱ AstAssignW* ধ্রুৱ ap = VN_CAST(nodep->stmtsp(), AssignW)) {
                যদি (ap->nextp()) ঘূৰাই মিছা;
                // Cannot handle assignment with timing control
                যদি (ap->timingControlp()) ঘূৰাই মিছা;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                ধ্রুৱ std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(ap);
                যদি (!oVarpsp) ঘূৰাই মিছা;
                // Gather read variables, give up if any are not supported
                ধ্রুৱ std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherRead(ap);
                যদি (!iVarpsp) ঘূৰাই মিছা;
                // Create the DfgLogic
                DfgLogic* ধ্রুৱ logicp = নূতন DfgLogic{m_dfg, nodep, m_scopep, nullptr};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                // Done
                ++m_ctx.m_representable;
                ঘূৰাই সঁচা;
            }
        }

        // Can only handle combinational logic
        যদি (nodep->sentreep()) ঘূৰাই মিছা;
        যদি (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_ASSIGN) {
            ঘূৰাই মিছা;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<CfgGraph> cfgp = CfgGraph::build(nodep->stmtsp());
        যদি (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            ঘূৰাই মিছা;
        }
        // Gather written variables, give up if any are not supported
        ধ্রুৱ std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(nodep);
        যদি (!oVarpsp) ঘূৰাই মিছা;
        // Gather read variables, give up if any are not supported
        ধ্রুৱ std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherLive(*cfgp);
        যদি (!iVarpsp) ঘূৰাই মিছা;
        // Create the DfgLogic
        DfgLogic* ধ্রুৱ logicp = নূতন DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        // Done
        ++m_ctx.m_representable;
        ঘূৰাই সঁচা;
    }

    // VISITORS
    // Unhandled node
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { markReferenced(nodep); }

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
    শূন্য visit(Ast_শীৰ্ষ_Scope* nodep) override { iterate(nodep->scopep()); }
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
    শূন্য visit(Ast_গ্ৰন্থি_Procedure* nodep) override { markReferenced(nodep); }

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

সার্ৱজনিক:
    স্থির শূন্য apply(DfgGraph& dfg, AstNetlist& মূল, V3DfgAstToDfgContext& ctx) {
        // Convert all logic under 'root'
        AstToDfgVisitor{dfg, মূল, ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        বাবে (DfgVertexVar* ধ্রুৱ varp : dfg.varVertices().unlinkable()) {
            যদি (varp->srcp()) অনুৱর্ত;
            ধ্রুৱ তার্কিক keep = varp->foreachSink([&](DfgVertex& d) { ঘূৰাই !d.is<DfgAstRd>(); });
            যদি (!keep) {
                যেতিয়ালৈ (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<DfgGraph> V3DfgPasses::astToDfg(AstNetlist& netlist, V3DfgContext& ctx) {
    DfgGraph* ধ্রুৱ dfgp = নূতন DfgGraph{ctx, "netlist"};
    AstToDfgVisitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    ঘূৰাই std::unique_ptr<DfgGraph>{dfgp};
}
