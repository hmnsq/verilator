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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Cfg.h"
#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3Dfg.h"
#அந்தர்பாவ "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

வகுப்பு AstToDfgAddAstRefs final : பொது VNVisitorConst {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    // Function to get the DfgVertexVar for a AstVarScope
    மாறிலி std::function<DfgVertexVar*(AstVarScope*)> m_getVarVertex;
    தருக்க m_inSenItem = பொய்;  // Inside an AstSenItem
    தருக்க m_inLoop = பொய்;  // Inside an AstLoop

    // VISITORS
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }

    வெறுமை visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = உண்மை;
        iterateChildrenConst(nodep);
    }

    வெறுமை visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = உண்மை;
        iterateChildrenConst(nodep);
    }

    வெறுமை visit(AstVarRef* nodep) override {
        // Disguised hierarchical reference handled as external reference, ignore
        எனில் (nodep->classOrPackagep()) திரும்பு;
        // If target is not supported, ignore
        AstVarScope* மாறிலி vscp = nodep->varScopep();
        எனில் (!V3Dfg::isSupported(vscp)) திரும்பு;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        DfgVertexVar* மாறிலி varp = m_getVarVertex(vscp);
        எனில் (!varp) திரும்பு;
        // Create Ast reference vertices
        எனில் (nodep->access().isReadOnly()) {
            DfgAstRd* மாறிலி astp = புதிய DfgAstRd{m_dfg, nodep, m_inSenItem, m_inLoop};
            astp->srcp(varp);
            திரும்பு;
        }
        // Mark as written from non-DFG logic
        DfgVertexVar::setHasModWrRefs(vscp);
    }

    AstToDfgAddAstRefs(DfgGraph& dfg, Ast_முனை* nodep,
                       std::function<DfgVertexVar*(AstVarScope*)> getVarVertex)
        : m_dfg{dfg}
        , m_getVarVertex{getVarVertex} {
        iterateConst(nodep);
    }

பொது:
    நிலையான வெறுமை apply(DfgGraph& dfg, Ast_முனை* nodep,
                      std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
        AstToDfgAddAstRefs{dfg, nodep, getVarVertex};
    }
};

வெறுமை V3DfgPasses::addAstRefs(DfgGraph& dfg, Ast_முனை* nodep,
                             std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
    AstToDfgAddAstRefs::apply(dfg, nodep, getVarVertex);
}

வகுப்பு AstToDfgVisitor final : பொது VNVisitor {
    // NODE STATE
    // AstVarScope::user2() -> DfgVertexVar* : the corresponding variable vertex
    // AstVarScope::user3() -> bool : Already gathered - used fine grained below
    மாறிலி VNUser2InUse m_user2InUse;

    // STATE
    DfgGraph& m_dfg;  // The graph being built
    V3DfgAstToDfgContext& m_ctx;  // The context for stats
    AstScope* m_scopep = nullptr;  // The current scope, iff T_Scoped

    // METHODS

    // Mark variables referenced under node
    வெறுமை markReferenced(Ast_முனை* nodep) {
        V3DfgPasses::addAstRefs(m_dfg, nodep, [இது](AstVarScope* vscp) {  //
            திரும்பு getVarVertex(vscp);
        });
    }

    DfgVertexVar* getVarVertex(AstVarScope* varp) {
        எனில் (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            மாறிலி Ast_முனை_DType* மாறிலி dtypep = varp->dtypep()->skipRefp();
            DfgVertexVar* மாறிலி vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<DfgVertexVar*>(புதிய DfgVarArray{m_dfg, varp})
                      : static_cast<DfgVertexVar*>(புதிய DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        திரும்பு varp->user2u().template to<DfgVertexVar*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherWritten(மாறிலி Ast_முனை* nodep) {
        மாறிலி VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{புதிய std::vector<DfgVertexVar*>{}};
        மாறிலி தருக்க abort = nodep->exists([&](மாறிலி AstVarRef* refp) -> தருக்க {
            எனில் (refp->access().isReadOnly()) திரும்பு பொய்;
            AstVarScope* மாறிலி vscp = refp->varScopep();
            எனில் (!V3Dfg::isSupported(vscp)) திரும்பு உண்மை;
            எனில் (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            திரும்பு பொய்;
        });
        எனில் (abort) {
            ++m_ctx.m_nonRepVar;
            திரும்பு nullptr;
        }
        திரும்பு resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherRead(மாறிலி Ast_முனை* nodep) {
        மாறிலி VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{புதிய std::vector<DfgVertexVar*>{}};
        மாறிலி தருக்க abort = nodep->exists([&](மாறிலி AstVarRef* refp) -> தருக்க {
            எனில் (refp->access().isWriteOnly()) திரும்பு பொய்;
            AstVarScope* மாறிலி vscp = refp->varScopep();
            எனில் (!V3Dfg::isSupported(vscp)) திரும்பு உண்மை;
            எனில் (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            திரும்பு பொய்;
        });
        எனில் (abort) {
            ++m_ctx.m_nonRepVar;
            திரும்பு nullptr;
        }
        திரும்பு resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherLive(மாறிலி CfgGraph& cfg) {
        // Run analysis
        std::unique_ptr<std::vector<AstVarScope*>> varps = V3Cfg::liveVarScopes(cfg);
        எனில் (!varps) {
            ++m_ctx.m_nonRepLive;
            திரும்பு nullptr;
        }

        // Convert to vertics
        மாறிலி VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{புதிய std::vector<DfgVertexVar*>{}};
        resp->reserve(varps->size());
        க்கு (AstVarScope* மாறிலி varp : *varps) {
            எனில் (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                திரும்பு nullptr;
            }
            UASSERT_OBJ(!varp->user3SetOnce(), varp, "Live variables should be unique");
            resp->emplace_back(getVarVertex(varp));
        }
        திரும்பு resp;
    }

    // Connect inputs and outputs of a DfgLogic
    வெறுமை connect(DfgLogic& vtx, மாறிலி std::vector<DfgVertexVar*>& iVarps,
                 மாறிலி std::vector<DfgVertexVar*>& oVarps) {
        // Connect inputs
        க்கு (DfgVertexVar* மாறிலி iVarp : iVarps) vtx.add_உள்ளீடு(iVarp);
        // Connect outputs
        க்கு (DfgVertexVar* மாறிலி oVarp : oVarps) {
            எனில் (!oVarp->srcp()) oVarp->srcp(புதிய DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->addDriver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    தருக்க convert(AstAlways* nodep) {
        மாறிலி VAlwaysKwd kwd = nodep->keyword();
        எனில் (kwd == VAlwaysKwd::CONT_ASSIGN) {
            // TODO: simplify once CFG analysis can handle arrays
            எனில் (மாறிலி AstAssignW* மாறிலி ap = VN_CAST(nodep->stmtsp(), AssignW)) {
                எனில் (ap->nextp()) திரும்பு பொய்;
                // Cannot handle assignment with timing control
                எனில் (ap->timingControlp()) திரும்பு பொய்;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                மாறிலி std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(ap);
                எனில் (!oVarpsp) திரும்பு பொய்;
                // Gather read variables, give up if any are not supported
                மாறிலி std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherRead(ap);
                எனில் (!iVarpsp) திரும்பு பொய்;
                // Create the DfgLogic
                DfgLogic* மாறிலி logicp = புதிய DfgLogic{m_dfg, nodep, m_scopep, nullptr};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                // Done
                ++m_ctx.m_representable;
                திரும்பு உண்மை;
            }
        }

        // Can only handle combinational logic
        எனில் (nodep->sentreep()) திரும்பு பொய்;
        எனில் (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_ASSIGN) {
            திரும்பு பொய்;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<CfgGraph> cfgp = CfgGraph::build(nodep->stmtsp());
        எனில் (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            திரும்பு பொய்;
        }
        // Gather written variables, give up if any are not supported
        மாறிலி std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(nodep);
        எனில் (!oVarpsp) திரும்பு பொய்;
        // Gather read variables, give up if any are not supported
        மாறிலி std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherLive(*cfgp);
        எனில் (!iVarpsp) திரும்பு பொய்;
        // Create the DfgLogic
        DfgLogic* மாறிலி logicp = புதிய DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        // Done
        ++m_ctx.m_representable;
        திரும்பு உண்மை;
    }

    // VISITORS
    // Unhandled node
    வெறுமை visit(Ast_முனை* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    வெறுமை visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    வெறுமை visit(AstModule* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    வெறுமை visit(AstIface* nodep) override {
        எனில் (!nodep->hasVirtualRef()) {
            iterateAndNextNull(nodep->stmtsp());
        } இல்லையெனில் {
            markReferenced(nodep);
        }
    }
    வெறுமை visit(Ast_உச்சி_Scope* nodep) override { iterate(nodep->scopep()); }
    வெறுமை visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstActive* nodep) override {
        எனில் (nodep->hasCombo()) {
            iterateChildren(nodep);
        } இல்லையெனில் {
            markReferenced(nodep);
        }
    }

    // Non-representable constructs
    வெறுமை visit(AstCell* nodep) override { markReferenced(nodep); }
    வெறுமை visit(Ast_முனை_Procedure* nodep) override { markReferenced(nodep); }

    // Potentially representable constructs
    வெறுமை visit(AstAlways* nodep) override {
        எனில் (!convert(nodep)) markReferenced(nodep);
    }

    // CONSTRUCTOR
    AstToDfgVisitor(DfgGraph& dfg, AstNetlist& வேர், V3DfgAstToDfgContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {
        iterate(&வேர்);
    }
    VL_UNCOPYABLE(AstToDfgVisitor);
    VL_UNMOVABLE(AstToDfgVisitor);

பொது:
    நிலையான வெறுமை apply(DfgGraph& dfg, AstNetlist& வேர், V3DfgAstToDfgContext& ctx) {
        // Convert all logic under 'root'
        AstToDfgVisitor{dfg, வேர், ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        க்கு (DfgVertexVar* மாறிலி varp : dfg.varVertices().unlinkable()) {
            எனில் (varp->srcp()) தொடர்;
            மாறிலி தருக்க keep = varp->foreachSink([&](DfgVertex& d) { திரும்பு !d.is<DfgAstRd>(); });
            எனில் (!keep) {
                போது (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<DfgGraph> V3DfgPasses::astToDfg(AstNetlist& netlist, V3DfgContext& ctx) {
    DfgGraph* மாறிலி dfgp = புதிய DfgGraph{ctx, "netlist"};
    AstToDfgVisitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    திரும்பு std::unique_ptr<DfgGraph>{dfgp};
}
