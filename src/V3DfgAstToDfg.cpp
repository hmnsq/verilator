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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Cfg.h"
#ضمن "V3Const.h"
#ضمن "V3Dfg.h"
#ضمن "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

صنف AstToDfgAddAstRefs final : عام VNVisitorConst {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    // Function to get the DfgVertexVar for a AstVarScope
    ثابت std::function<DfgVertexVar*(AstVarScope*)> m_getVarVertex;
    منطقي m_inSenItem = خطأ;  // Inside an AstSenItem
    منطقي m_inLoop = خطأ;  // Inside an AstLoop

    // VISITORS
    فراغ visit(AstNode* nodep) override { iterateChildrenConst(nodep); }

    فراغ visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = true;
        iterateChildrenConst(nodep);
    }

    فراغ visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = true;
        iterateChildrenConst(nodep);
    }

    فراغ visit(AstVarRef* nodep) override {
        // Disguised hierarchical reference handled as external reference, ignore
        𐤀𐤌 (nodep->classOrPackagep()) أرجع;
        // If target is not supported, ignore
        AstVarScope* ثابت vscp = nodep->varScopep();
        𐤀𐤌 (!V3Dfg::isSupported(vscp)) أرجع;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        DfgVertexVar* ثابت varp = m_getVarVertex(vscp);
        𐤀𐤌 (!varp) أرجع;
        // Create Ast reference vertices
        𐤀𐤌 (nodep->access().isReadOnly()) {
            DfgAstRd* ثابت astp = جديد DfgAstRd{m_dfg, nodep, m_inSenItem, m_inLoop};
            astp->srcp(varp);
            أرجع;
        }
        // Mark as written from non-DFG logic
        DfgVertexVar::setHasModWrRefs(vscp);
    }

    AstToDfgAddAstRefs(DfgGraph& dfg, AstNode* nodep,
                       std::function<DfgVertexVar*(AstVarScope*)> getVarVertex)
        : m_dfg{dfg}
        , m_getVarVertex{getVarVertex} {
        iterateConst(nodep);
    }

عام:
    ساكن فراغ apply(DfgGraph& dfg, AstNode* nodep,
                      std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
        AstToDfgAddAstRefs{dfg, nodep, getVarVertex};
    }
};

فراغ V3DfgPasses::addAstRefs(DfgGraph& dfg, AstNode* nodep,
                             std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
    AstToDfgAddAstRefs::apply(dfg, nodep, getVarVertex);
}

صنف AstToDfgVisitor final : عام VNVisitor {
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
    فراغ markReferenced(AstNode* nodep) {
        V3DfgPasses::addAstRefs(m_dfg, nodep, [هذا](AstVarScope* vscp) {  //
            أرجع getVarVertex(vscp);
        });
    }

    DfgVertexVar* getVarVertex(AstVarScope* varp) {
        𐤀𐤌 (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            ثابت AstNodeDType* ثابت dtypep = varp->dtypep()->skipRefp();
            DfgVertexVar* ثابت vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<DfgVertexVar*>(جديد DfgVarArray{m_dfg, varp})
                      : static_cast<DfgVertexVar*>(جديد DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        أرجع varp->user2u().template to<DfgVertexVar*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherWritten(ثابت AstNode* nodep) {
        ثابت VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{جديد std::vector<DfgVertexVar*>{}};
        ثابت منطقي abort = nodep->exists([&](ثابت AstVarRef* refp) -> منطقي {
            𐤀𐤌 (refp->access().isReadOnly()) أرجع خطأ;
            AstVarScope* ثابت vscp = refp->varScopep();
            𐤀𐤌 (!V3Dfg::isSupported(vscp)) أرجع true;
            𐤀𐤌 (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            أرجع خطأ;
        });
        𐤀𐤌 (abort) {
            ++m_ctx.m_nonRepVar;
            أرجع nullptr;
        }
        أرجع resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherRead(ثابت AstNode* nodep) {
        ثابت VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{جديد std::vector<DfgVertexVar*>{}};
        ثابت منطقي abort = nodep->exists([&](ثابت AstVarRef* refp) -> منطقي {
            𐤀𐤌 (refp->access().isWriteOnly()) أرجع خطأ;
            AstVarScope* ثابت vscp = refp->varScopep();
            𐤀𐤌 (!V3Dfg::isSupported(vscp)) أرجع true;
            𐤀𐤌 (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            أرجع خطأ;
        });
        𐤀𐤌 (abort) {
            ++m_ctx.m_nonRepVar;
            أرجع nullptr;
        }
        أرجع resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherLive(ثابت CfgGraph& cfg) {
        // Run analysis
        std::unique_ptr<std::vector<AstVarScope*>> varps = V3Cfg::liveVarScopes(cfg);
        𐤀𐤌 (!varps) {
            ++m_ctx.m_nonRepLive;
            أرجع nullptr;
        }

        // Convert to vertics
        ثابت VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{جديد std::vector<DfgVertexVar*>{}};
        resp->reserve(varps->size());
        لكل (AstVarScope* ثابت varp : *varps) {
            𐤀𐤌 (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                أرجع nullptr;
            }
            UASSERT_OBJ(!varp->user3SetOnce(), varp, "Live variables should be unique");
            resp->emplace_back(getVarVertex(varp));
        }
        أرجع resp;
    }

    // Connect inputs and outputs of a DfgLogic
    فراغ connect(DfgLogic& vtx, ثابت std::vector<DfgVertexVar*>& iVarps,
                 ثابت std::vector<DfgVertexVar*>& oVarps) {
        // Connect inputs
        لكل (DfgVertexVar* ثابت iVarp : iVarps) vtx.addInput(iVarp);
        // Connect outputs
        لكل (DfgVertexVar* ثابت oVarp : oVarps) {
            𐤀𐤌 (!oVarp->srcp()) oVarp->srcp(جديد DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->addDriver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    منطقي convert(AstAlways* nodep) {
        ثابت VAlwaysKwd kwd = nodep->keyword();
        𐤀𐤌 (kwd == VAlwaysKwd::CONT_ASSIGN) {
            // TODO: simplify once CFG analysis can handle arrays
            𐤀𐤌 (ثابت AstAssignW* ثابت ap = VN_CAST(nodep->stmtsp(), AssignW)) {
                𐤀𐤌 (ap->nextp()) أرجع خطأ;
                // Cannot handle assignment with timing control
                𐤀𐤌 (ap->timingControlp()) أرجع خطأ;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                ثابت std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(ap);
                𐤀𐤌 (!oVarpsp) أرجع خطأ;
                // Gather read variables, give up if any are not supported
                ثابت std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherRead(ap);
                𐤀𐤌 (!iVarpsp) أرجع خطأ;
                // Create the DfgLogic
                DfgLogic* ثابت logicp = جديد DfgLogic{m_dfg, nodep, m_scopep, nullptr};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                // Done
                ++m_ctx.m_representable;
                أرجع true;
            }
        }

        // Can only handle combinational logic
        𐤀𐤌 (nodep->sentreep()) أرجع خطأ;
        𐤀𐤌 (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_ASSIGN) {
            أرجع خطأ;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<CfgGraph> cfgp = CfgGraph::build(nodep->stmtsp());
        𐤀𐤌 (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            أرجع خطأ;
        }
        // Gather written variables, give up if any are not supported
        ثابت std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(nodep);
        𐤀𐤌 (!oVarpsp) أرجع خطأ;
        // Gather read variables, give up if any are not supported
        ثابت std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherLive(*cfgp);
        𐤀𐤌 (!iVarpsp) أرجع خطأ;
        // Create the DfgLogic
        DfgLogic* ثابت logicp = جديد DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        // Done
        ++m_ctx.m_representable;
        أرجع true;
    }

    // VISITORS
    // Unhandled node
    فراغ visit(AstNode* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    فراغ visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    فراغ visit(AstModule* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    فراغ visit(AstIface* nodep) override {
        𐤀𐤌 (!nodep->hasVirtualRef()) {
            iterateAndNextNull(nodep->stmtsp());
        } وإلا {
            markReferenced(nodep);
        }
    }
    فراغ visit(AstTopScope* nodep) override { iterate(nodep->scopep()); }
    فراغ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstActive* nodep) override {
        𐤀𐤌 (nodep->hasCombo()) {
            iterateChildren(nodep);
        } وإلا {
            markReferenced(nodep);
        }
    }

    // Non-representable constructs
    فراغ visit(AstCell* nodep) override { markReferenced(nodep); }
    فراغ visit(AstNodeProcedure* nodep) override { markReferenced(nodep); }

    // Potentially representable constructs
    فراغ visit(AstAlways* nodep) override {
        𐤀𐤌 (!convert(nodep)) markReferenced(nodep);
    }

    // CONSTRUCTOR
    AstToDfgVisitor(DfgGraph& dfg, AstNetlist& root, V3DfgAstToDfgContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {
        iterate(&root);
    }
    VL_UNCOPYABLE(AstToDfgVisitor);
    VL_UNMOVABLE(AstToDfgVisitor);

عام:
    ساكن فراغ apply(DfgGraph& dfg, AstNetlist& root, V3DfgAstToDfgContext& ctx) {
        // Convert all logic under 'root'
        AstToDfgVisitor{dfg, root, ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        لكل (DfgVertexVar* ثابت varp : dfg.varVertices().unlinkable()) {
            𐤀𐤌 (varp->srcp()) استمر;
            ثابت منطقي keep = varp->foreachSink([&](DfgVertex& d) { أرجع !d.is<DfgAstRd>(); });
            𐤀𐤌 (!keep) {
                طالما (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<DfgGraph> V3DfgPasses::astToDfg(AstNetlist& netlist, V3DfgContext& ctx) {
    DfgGraph* ثابت dfgp = جديد DfgGraph{ctx, "netlist"};
    AstToDfgVisitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    أرجع std::unique_ptr<DfgGraph>{dfgp};
}
