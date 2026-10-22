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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Cfg.h"
#شامل "V3Const.h"
#شامل "V3Dfg.h"
#شامل "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

رده AstToDfgAddAstRefs final : عمومی VNVisitorConst {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    // Function to get the DfgVertexVar for a AstVarScope
    ثابت std::function<DfgVertexVar*(AstVarScope*)> m_getVarVertex;
    منطقی m_inSenItem = نادرست;  // Inside an AstSenItem
    منطقی m_inLoop = نادرست;  // Inside an AstLoop

    // VISITORS
    تهی visit(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

    تهی visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = درست;
        iterateChildrenConst(nodep);
    }

    تهی visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = درست;
        iterateChildrenConst(nodep);
    }

    تهی visit(AstVarRef* nodep) override {
        // Disguised hierarchical reference handled as external reference, ignore
        اگر (nodep->classOrPackagep()) بازگشت;
        // If target is not supported, ignore
        AstVarScope* ثابت vscp = nodep->varScopep();
        اگر (!V3Dfg::isSupported(vscp)) بازگشت;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        DfgVertexVar* ثابت varp = m_getVarVertex(vscp);
        اگر (!varp) بازگشت;
        // Create Ast reference vertices
        اگر (nodep->access().isReadOnly()) {
            DfgAstRd* ثابت astp = جدید DfgAstRd{m_dfg, nodep, m_inSenItem, m_inLoop};
            astp->srcp(varp);
            بازگشت;
        }
        // Mark as written from non-DFG logic
        DfgVertexVar::setHasModWrRefs(vscp);
    }

    AstToDfgAddAstRefs(DfgGraph& dfg, Ast_گره* nodep,
                       std::function<DfgVertexVar*(AstVarScope*)> getVarVertex)
        : m_dfg{dfg}
        , m_getVarVertex{getVarVertex} {
        iterateConst(nodep);
    }

عمومی:
    ایستا تهی apply(DfgGraph& dfg, Ast_گره* nodep,
                      std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
        AstToDfgAddAstRefs{dfg, nodep, getVarVertex};
    }
};

تهی V3DfgPasses::addAstRefs(DfgGraph& dfg, Ast_گره* nodep,
                             std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
    AstToDfgAddAstRefs::apply(dfg, nodep, getVarVertex);
}

رده AstToDfgVisitor final : عمومی VNVisitor {
    // NODE STATE
    // AstVarScope::user2() -> DfgVertexVar* : the corresponding variable vertex
    // AstVarScope::user3() -> bool : Already gathered - used fine grained below
    ثابت VNUser2InUse m_user2InUse;

    // STATE
    DfgGraph& m_dfg;  // The graph being built
    V3DfgAstToDfgContext& m_ctx;  // The context for stats
    AstScope* m_scopep = nullptr;  // The current scope, iff T_Scoped

    // METHODS

    // Mark variables referenced under node
    تهی markReferenced(Ast_گره* nodep) {
        V3DfgPasses::addAstRefs(m_dfg, nodep, [این](AstVarScope* vscp) {  //
            بازگشت getVarVertex(vscp);
        });
    }

    DfgVertexVar* getVarVertex(AstVarScope* varp) {
        اگر (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            ثابت Ast_گره_DType* ثابت dtypep = varp->dtypep()->skipRefp();
            DfgVertexVar* ثابت vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<DfgVertexVar*>(جدید DfgVarArray{m_dfg, varp})
                      : static_cast<DfgVertexVar*>(جدید DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        بازگشت varp->user2u().template to<DfgVertexVar*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherWritten(ثابت Ast_گره* nodep) {
        ثابت VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{جدید std::vector<DfgVertexVar*>{}};
        ثابت منطقی abort = nodep->exists([&](ثابت AstVarRef* refp) -> منطقی {
            اگر (refp->access().isReadOnly()) بازگشت نادرست;
            AstVarScope* ثابت vscp = refp->varScopep();
            اگر (!V3Dfg::isSupported(vscp)) بازگشت درست;
            اگر (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            بازگشت نادرست;
        });
        اگر (abort) {
            ++m_ctx.m_nonRepVar;
            بازگشت nullptr;
        }
        بازگشت resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherRead(ثابت Ast_گره* nodep) {
        ثابت VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{جدید std::vector<DfgVertexVar*>{}};
        ثابت منطقی abort = nodep->exists([&](ثابت AstVarRef* refp) -> منطقی {
            اگر (refp->access().isWriteOnly()) بازگشت نادرست;
            AstVarScope* ثابت vscp = refp->varScopep();
            اگر (!V3Dfg::isSupported(vscp)) بازگشت درست;
            اگر (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            بازگشت نادرست;
        });
        اگر (abort) {
            ++m_ctx.m_nonRepVar;
            بازگشت nullptr;
        }
        بازگشت resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherLive(ثابت CfgGraph& cfg) {
        // Run analysis
        std::unique_ptr<std::vector<AstVarScope*>> varps = V3Cfg::liveVarScopes(cfg);
        اگر (!varps) {
            ++m_ctx.m_nonRepLive;
            بازگشت nullptr;
        }

        // Convert to vertics
        ثابت VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{جدید std::vector<DfgVertexVar*>{}};
        resp->reserve(varps->size());
        برای (AstVarScope* ثابت varp : *varps) {
            اگر (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                بازگشت nullptr;
            }
            UASSERT_OBJ(!varp->user3SetOnce(), varp, "Live variables should be unique");
            resp->emplace_back(getVarVertex(varp));
        }
        بازگشت resp;
    }

    // Connect inputs and outputs of a DfgLogic
    تهی connect(DfgLogic& vtx, ثابت std::vector<DfgVertexVar*>& iVarps,
                 ثابت std::vector<DfgVertexVar*>& oVarps) {
        // Connect inputs
        برای (DfgVertexVar* ثابت iVarp : iVarps) vtx.add_درون_داد(iVarp);
        // Connect outputs
        برای (DfgVertexVar* ثابت oVarp : oVarps) {
            اگر (!oVarp->srcp()) oVarp->srcp(جدید DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->addDriver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    منطقی convert(AstAlways* nodep) {
        ثابت VAlwaysKwd kwd = nodep->keyword();
        اگر (kwd == VAlwaysKwd::CONT_ASSIGN) {
            // TODO: simplify once CFG analysis can handle arrays
            اگر (ثابت AstAssignW* ثابت ap = VN_CAST(nodep->stmtsp(), AssignW)) {
                اگر (ap->nextp()) بازگشت نادرست;
                // Cannot handle assignment with timing control
                اگر (ap->timingControlp()) بازگشت نادرست;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                ثابت std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(ap);
                اگر (!oVarpsp) بازگشت نادرست;
                // Gather read variables, give up if any are not supported
                ثابت std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherRead(ap);
                اگر (!iVarpsp) بازگشت نادرست;
                // Create the DfgLogic
                DfgLogic* ثابت logicp = جدید DfgLogic{m_dfg, nodep, m_scopep, nullptr};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                // Done
                ++m_ctx.m_representable;
                بازگشت درست;
            }
        }

        // Can only handle combinational logic
        اگر (nodep->sentreep()) بازگشت نادرست;
        اگر (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_ASSIGN) {
            بازگشت نادرست;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<CfgGraph> cfgp = CfgGraph::build(nodep->stmtsp());
        اگر (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            بازگشت نادرست;
        }
        // Gather written variables, give up if any are not supported
        ثابت std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(nodep);
        اگر (!oVarpsp) بازگشت نادرست;
        // Gather read variables, give up if any are not supported
        ثابت std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherLive(*cfgp);
        اگر (!iVarpsp) بازگشت نادرست;
        // Create the DfgLogic
        DfgLogic* ثابت logicp = جدید DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        // Done
        ++m_ctx.m_representable;
        بازگشت درست;
    }

    // VISITORS
    // Unhandled node
    تهی visit(Ast_گره* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    تهی visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    تهی visit(AstModule* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    تهی visit(AstIface* nodep) override {
        اگر (!nodep->hasVirtualRef()) {
            iterateAndNextNull(nodep->stmtsp());
        } وگرنه {
            markReferenced(nodep);
        }
    }
    تهی visit(Ast_بالاترین_Scope* nodep) override { iterate(nodep->scopep()); }
    تهی visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstActive* nodep) override {
        اگر (nodep->hasCombo()) {
            iterateChildren(nodep);
        } وگرنه {
            markReferenced(nodep);
        }
    }

    // Non-representable constructs
    تهی visit(AstCell* nodep) override { markReferenced(nodep); }
    تهی visit(Ast_گره_Procedure* nodep) override { markReferenced(nodep); }

    // Potentially representable constructs
    تهی visit(AstAlways* nodep) override {
        اگر (!convert(nodep)) markReferenced(nodep);
    }

    // CONSTRUCTOR
    AstToDfgVisitor(DfgGraph& dfg, AstNetlist& ریشه, V3DfgAstToDfgContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {
        iterate(&ریشه);
    }
    VL_UNCOPYABLE(AstToDfgVisitor);
    VL_UNMOVABLE(AstToDfgVisitor);

عمومی:
    ایستا تهی apply(DfgGraph& dfg, AstNetlist& ریشه, V3DfgAstToDfgContext& ctx) {
        // Convert all logic under 'root'
        AstToDfgVisitor{dfg, ریشه, ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        برای (DfgVertexVar* ثابت varp : dfg.varVertices().unlinkable()) {
            اگر (varp->srcp()) ادامه;
            ثابت منطقی keep = varp->foreachSink([&](DfgVertex& d) { بازگشت !d.is<DfgAstRd>(); });
            اگر (!keep) {
                وقتی (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<DfgGraph> V3DfgPasses::astToDfg(AstNetlist& netlist, V3DfgContext& ctx) {
    DfgGraph* ثابت dfgp = جدید DfgGraph{ctx, "netlist"};
    AstToDfgVisitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    بازگشت std::unique_ptr<DfgGraph>{dfgp};
}
