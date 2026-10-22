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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Cfg.h"
#include "V3Const.h"
#include "V3Dfg.h"
#include "V3DfgPasses.h"

VL_DEFINE_DEBUG_FUNCTIONS;

class AstToDfgAddAstRefs final : public VNVisitorConst {
    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    // Function to get the DfgVertexVar for a AstVarScope
    const std::function<DfgVertexVar*(AstVarScope*)> m_getVarVertex;
    bool m_inSenItem = false;  // Inside an AstSenItem
    bool m_inLoop = false;  // Inside an AstLoop

    // VISITORS
    void visit(AstNode* nodep) override { iterateChildrenConst(nodep); }

    void visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = true;
        iterateChildrenConst(nodep);
    }

    void visit(AstLoop* nodep) override {
        VL_RESTORER(m_inLoop);
        m_inLoop = true;
        iterateChildrenConst(nodep);
    }

    void visit(AstVarRef* nodep) override {
        // Disguised hierarchical reference handled as external reference, ignore
        නම් (nodep->classOrPackagep()) ආපසු;
        // If target is not supported, ignore
        AstVarScope* const vscp = nodep->varScopep();
        නම් (!V3Dfg::isSupported(vscp)) ආපසු;
        // V3Dfg::isSupported should reject vars with READWRITE references
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "AstVarScope with READWRITE ref not rejected");
        // Get target variable vergtex, ignore if not given one
        DfgVertexVar* const varp = m_getVarVertex(vscp);
        නම් (!varp) ආපසු;
        // Create Ast reference vertices
        නම් (nodep->access().isReadOnly()) {
            DfgAstRd* const astp = new DfgAstRd{m_dfg, nodep, m_inSenItem, m_inLoop};
            astp->srcp(varp);
            ආපසු;
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

public:
    static void apply(DfgGraph& dfg, AstNode* nodep,
                      std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
        AstToDfgAddAstRefs{dfg, nodep, getVarVertex};
    }
};

void V3DfgPasses::addAstRefs(DfgGraph& dfg, AstNode* nodep,
                             std::function<DfgVertexVar*(AstVarScope*)> getVarVertex) {
    AstToDfgAddAstRefs::apply(dfg, nodep, getVarVertex);
}

class AstToDfgVisitor final : public VNVisitor {
    // NODE STATE
    // AstVarScope::user2() -> DfgVertexVar* : the corresponding variable vertex
    // AstVarScope::user3() -> bool : Already gathered - used fine grained below
    const VNUser2InUse m_user2InUse;

    // STATE
    DfgGraph& m_dfg;  // The graph being built
    V3DfgAstToDfgContext& m_ctx;  // The context for stats
    AstScope* m_scopep = nullptr;  // The current scope, iff T_Scoped

    // METHODS

    // Mark variables referenced under node
    void markReferenced(AstNode* nodep) {
        V3DfgPasses::addAstRefs(m_dfg, nodep, [this](AstVarScope* vscp) {  //
            ආපසු getVarVertex(vscp);
        });
    }

    DfgVertexVar* getVarVertex(AstVarScope* varp) {
        නම් (!varp->user2p()) {
            // TODO: fix this up when removing the different flavours of DfgVar
            const AstNodeDType* const dtypep = varp->dtypep()->skipRefp();
            DfgVertexVar* const vtxp
                = VN_IS(dtypep, UnpackArrayDType)
                      ? static_cast<DfgVertexVar*>(new DfgVarArray{m_dfg, varp})
                      : static_cast<DfgVertexVar*>(new DfgVarPacked{m_dfg, varp});
            varp->user2p(vtxp);
        }
        ආපසු varp->user2u().template to<DfgVertexVar*>();
    }

    // Gather variables written by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherWritten(const AstNode* nodep) {
        const VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{new std::vector<DfgVertexVar*>{}};
        const bool abort = nodep->exists([&](const AstVarRef* refp) -> bool {
            නම් (refp->access().isReadOnly()) ආපසු false;
            AstVarScope* const vscp = refp->varScopep();
            නම් (!V3Dfg::isSupported(vscp)) ආපසු true;
            නම් (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            ආපසු false;
        });
        නම් (abort) {
            ++m_ctx.m_nonRepVar;
            ආපසු nullptr;
        }
        ආපසු resp;
    }

    // Gather variables read by the given logic node.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherRead(const AstNode* nodep) {
        const VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{new std::vector<DfgVertexVar*>{}};
        const bool abort = nodep->exists([&](const AstVarRef* refp) -> bool {
            නම් (refp->access().isWriteOnly()) ආපසු false;
            AstVarScope* const vscp = refp->varScopep();
            නම් (!V3Dfg::isSupported(vscp)) ආපසු true;
            නම් (!vscp->user3SetOnce()) resp->emplace_back(getVarVertex(vscp));
            ආපසු false;
        });
        නම් (abort) {
            ++m_ctx.m_nonRepVar;
            ආපසු nullptr;
        }
        ආපසු resp;
    }

    // Gather variables live in to the given CFG.
    // Return nullptr if any are not supported.
    std::unique_ptr<std::vector<DfgVertexVar*>> gatherLive(const CfgGraph& cfg) {
        // Run analysis
        std::unique_ptr<std::vector<AstVarScope*>> varps = V3Cfg::liveVarScopes(cfg);
        නම් (!varps) {
            ++m_ctx.m_nonRepLive;
            ආපසු nullptr;
        }

        // Convert to vertics
        const VNUser3InUse user3InUse;
        std::unique_ptr<std::vector<DfgVertexVar*>> resp{new std::vector<DfgVertexVar*>{}};
        resp->reserve(varps->size());
        සඳහා (AstVarScope* const varp : *varps) {
            නම් (!V3Dfg::isSupported(varp)) {
                ++m_ctx.m_nonRepVar;
                ආපසු nullptr;
            }
            UASSERT_OBJ(!varp->user3SetOnce(), varp, "Live variables should be unique");
            resp->emplace_back(getVarVertex(varp));
        }
        ආපසු resp;
    }

    // Connect inputs and outputs of a DfgLogic
    void connect(DfgLogic& vtx, const std::vector<DfgVertexVar*>& iVarps,
                 const std::vector<DfgVertexVar*>& oVarps) {
        // Connect inputs
        සඳහා (DfgVertexVar* const iVarp : iVarps) vtx.addInput(iVarp);
        // Connect outputs
        සඳහා (DfgVertexVar* const oVarp : oVarps) {
            නම් (!oVarp->srcp()) oVarp->srcp(new DfgUnresolved{m_dfg, oVarp});
            oVarp->srcp()->as<DfgUnresolved>()->addDriver(&vtx);
        }
    }

    // Convert AstAlways to DfgLogic, return true if successful.
    bool convert(AstAlways* nodep) {
        const VAlwaysKwd kwd = nodep->keyword();
        නම් (kwd == VAlwaysKwd::CONT_ASSIGN) {
            // TODO: simplify once CFG analysis can handle arrays
            නම් (const AstAssignW* const ap = VN_CAST(nodep->stmtsp(), AssignW)) {
                නම් (ap->nextp()) ආපසු false;
                // Cannot handle assignment with timing control
                නම් (ap->timingControlp()) ආපසු false;
                // Potentially convertible block
                ++m_ctx.m_inputs;
                // Gather written variables, give up if any are not supported
                const std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(ap);
                නම් (!oVarpsp) ආපසු false;
                // Gather read variables, give up if any are not supported
                const std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherRead(ap);
                නම් (!iVarpsp) ආපසු false;
                // Create the DfgLogic
                DfgLogic* const logicp = new DfgLogic{m_dfg, nodep, m_scopep, nullptr};
                // Connect it up
                connect(*logicp, *iVarpsp, *oVarpsp);
                // Done
                ++m_ctx.m_representable;
                ආපසු true;
            }
        }

        // Can only handle combinational logic
        නම් (nodep->sentreep()) ආපසු false;
        නම් (kwd != VAlwaysKwd::ALWAYS  //
            && kwd != VAlwaysKwd::ALWAYS_COMB  //
            && kwd != VAlwaysKwd::CONT_ASSIGN) {
            ආපසු false;
        }

        // Potentially convertible block
        ++m_ctx.m_inputs;
        // Attempt to build CFG of AstAlways, give up if failed
        std::unique_ptr<CfgGraph> cfgp = CfgGraph::build(nodep->stmtsp());
        නම් (!cfgp) {
            ++m_ctx.m_nonRepCfg;
            ආපසු false;
        }
        // Gather written variables, give up if any are not supported
        const std::unique_ptr<std::vector<DfgVertexVar*>> oVarpsp = gatherWritten(nodep);
        නම් (!oVarpsp) ආපසු false;
        // Gather read variables, give up if any are not supported
        const std::unique_ptr<std::vector<DfgVertexVar*>> iVarpsp = gatherLive(*cfgp);
        නම් (!iVarpsp) ආපසු false;
        // Create the DfgLogic
        DfgLogic* const logicp = new DfgLogic{m_dfg, nodep, m_scopep, std::move(cfgp)};
        // Connect it up
        connect(*logicp, *iVarpsp, *oVarpsp);
        // Done
        ++m_ctx.m_representable;
        ආපසු true;
    }

    // VISITORS
    // Unhandled node
    void visit(AstNode* nodep) override { markReferenced(nodep); }

    // Containers to descend through to find logic constructs
    void visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }
    void visit(AstModule* nodep) override { iterateAndNextNull(nodep->stmtsp()); }
    void visit(AstIface* nodep) override {
        නම් (!nodep->hasVirtualRef()) {
            iterateAndNextNull(nodep->stmtsp());
        } නැතහොත් {
            markReferenced(nodep);
        }
    }
    void visit(AstTopScope* nodep) override { iterate(nodep->scopep()); }
    void visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    void visit(AstActive* nodep) override {
        නම් (nodep->hasCombo()) {
            iterateChildren(nodep);
        } නැතහොත් {
            markReferenced(nodep);
        }
    }

    // Non-representable constructs
    void visit(AstCell* nodep) override { markReferenced(nodep); }
    void visit(AstNodeProcedure* nodep) override { markReferenced(nodep); }

    // Potentially representable constructs
    void visit(AstAlways* nodep) override {
        නම් (!convert(nodep)) markReferenced(nodep);
    }

    // CONSTRUCTOR
    AstToDfgVisitor(DfgGraph& dfg, AstNetlist& root, V3DfgAstToDfgContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {
        iterate(&root);
    }
    VL_UNCOPYABLE(AstToDfgVisitor);
    VL_UNMOVABLE(AstToDfgVisitor);

public:
    static void apply(DfgGraph& dfg, AstNetlist& root, V3DfgAstToDfgContext& ctx) {
        // Convert all logic under 'root'
        AstToDfgVisitor{dfg, root, ctx};
        // Remove unread and undriven variables (created when something failed to convert)
        සඳහා (DfgVertexVar* const varp : dfg.varVertices().unlinkable()) {
            නම් (varp->srcp()) continue;
            const bool keep = varp->foreachSink([&](DfgVertex& d) { ආපසු !d.is<DfgAstRd>(); });
            නම් (!keep) {
                අතරතුර (varp->hasSinks()) varp->firtsSinkp()->unlinkDelete(dfg);
                VL_DO_DANGLING(varp->unlinkDelete(dfg), varp);
            }
        }
    }
};

std::unique_ptr<DfgGraph> V3DfgPasses::astToDfg(AstNetlist& netlist, V3DfgContext& ctx) {
    DfgGraph* const dfgp = new DfgGraph{ctx, "netlist"};
    AstToDfgVisitor::apply(*dfgp, netlist, ctx.m_ast2DfgContext);
    ආපසු std::unique_ptr<DfgGraph>{dfgp};
}
