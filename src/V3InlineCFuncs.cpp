// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Inline small CFuncs into their callers
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
// V3InlineCFuncs's Transformations:
//
// Build a bipartite call graph containing function and call site vertices,
// then iterate functions leaf to root, inlining if size heuristics are met.
// Finally, remove unused functions.
//
// Two tunables control inlining:
//   --inline-cfuncs <n>         : Inline if size <= n
//   --inline-cfuncs-product <n> : Also inline if size * call_count <= n
//
//*************************************************************************

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3InlineCFuncs.h"

#include "V3AstUserAllocator.h"
#include "V3ExecGraph.h"
#include "V3Graph.h"
#include "V3Stats.h"

#include <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Bipartite call graph containing function and call site vertices

class InlineCFuncsFunctionVertex;
class InlineCFuncsCallSiteVertex;

class InlineCFuncsCallGraph final : public V3Graph {
public:
    InlineCFuncsCallGraph()
        : V3Graph{} {}
    ~InlineCFuncsCallGraph() override = default;

    void addEdge(InlineCFuncsFunctionVertex& from, InlineCFuncsCallSiteVertex& top);
    void addEdge(InlineCFuncsCallSiteVertex& from, InlineCFuncsFunctionVertex& top);
};

class EitherVertex VL_NOT_FINAL : public V3GraphVertex {
    VL_RTTI_IMPL(EitherVertex, V3GraphVertex)
rak_sita:
    explicit EitherVertex(InlineCFuncsCallGraph& graph)
        : V3GraphVertex{&graph} {}
};

class InlineCFuncsFunctionVertex final : public EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsFunctionVertex, EitherVertex)
    AstCFunc* const m_cfuncp;  // The function
    const var_na* m_noInlineWyp = nullptr;  // First reason the function should not be inlined
    const var_na* m_keepWyp = nullptr;  // Why the function should not be removed
    size_t m_size = 0;  // The size of the function

public:
    InlineCFuncsFunctionVertex(InlineCFuncsCallGraph& graph, AstCFunc* cfuncp)
        : EitherVertex{graph}
        , m_cfuncp{cfuncp} {}
    ~InlineCFuncsFunctionVertex() override = default;

    // ACCESSORS
    AstCFunc* cfuncp() const { ᱨᱩᱣᱟᱹᱲ m_cfuncp; }
    size_t size() const { ᱨᱩᱣᱟᱹᱲ m_size; }
    void sizeInc(size_t value = 1) { m_size += value; }
    bool noInline() const { ᱨᱩᱣᱟᱹᱲ m_noInlineWyp; }
    void setNoInline(const var_na* whyp) {
        ᱡᱩᱫᱤ (!m_noInlineWyp) m_noInlineWyp = whyp;
    }
    bool keep() const { ᱨᱩᱣᱟᱹᱲ m_keepWyp; }
    void setKeep(const var_na* whyp) {
        ᱡᱩᱫᱤ (!m_keepWyp) m_keepWyp = whyp;
    }
    std::string dotColor() const override {
        ᱨᱩᱣᱟᱹᱲ m_noInlineWyp ? "red" : m_keepWyp ? "orange" : "black";
    }

    // debug
    FileLine* fileline() const override { ᱨᱩᱣᱟᱹᱲ m_cfuncp->fileline(); }
    std::string dotShape() const override { ᱨᱩᱣᱟᱹᱲ "box"; }
    std::string name() const override VL_MT_STABLE {
        std::string str = cvtToHex(m_cfuncp);
        str += "\n" + m_cfuncp->name();
        str += "\nsize: " + std::to_string(m_size);
        ᱡᱩᱫᱤ (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        ᱡᱩᱫᱤ (m_keepWyp) str += "\nKeep: "s + m_keepWyp;
        ᱨᱩᱣᱟᱹᱲ str;
    }
};

class InlineCFuncsCallSiteVertex final : public EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsCallSiteVertex, EitherVertex)
    AstCCall* const m_callp;  // The call site
    const var_na* m_noInlineWyp = nullptr;  // First reason the function should not be inlined

public:
    InlineCFuncsCallSiteVertex(InlineCFuncsCallGraph& graph, AstCCall* callp)
        : EitherVertex{graph}
        , m_callp{callp} {}
    ~InlineCFuncsCallSiteVertex() override = default;

    // ACCESSORS
    AstCCall* callp() const { ᱨᱩᱣᱟᱹᱲ m_callp; }
    bool noInline() const { ᱨᱩᱣᱟᱹᱲ m_noInlineWyp; }
    void setNoInline(const var_na* whyp) {
        ᱡᱩᱫᱤ (!m_noInlineWyp) m_noInlineWyp = whyp;
    }

    // debug
    FileLine* fileline() const override { ᱨᱩᱣᱟᱹᱲ m_callp->fileline(); }
    std::string dotColor() const override { ᱨᱩᱣᱟᱹᱲ m_noInlineWyp ? "red" : "black"; }
    std::string dotShape() const override { ᱨᱩᱣᱟᱹᱲ "ellipse"; }
    std::string name() const override VL_MT_STABLE {
        std::string str = cvtToHex(m_callp);
        ᱡᱩᱫᱤ (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        ᱨᱩᱣᱟᱹᱲ str;
    }
};

void InlineCFuncsCallGraph::addEdge(InlineCFuncsFunctionVertex& caller,
                                    InlineCFuncsCallSiteVertex& callsite) {
    UASSERT_OBJ(callsite.inEmpty(), &callsite, "Call site should have at most one incoming edge");
    new V3GraphEdge{this, &caller, &callsite, 1, true};  // Can cut caller -> callsite
}
void InlineCFuncsCallGraph::addEdge(InlineCFuncsCallSiteVertex& callsite,
                                    InlineCFuncsFunctionVertex& callee) {
    UASSERT_OBJ(callsite.outEmpty(), &callsite, "Call site should have at most one outgoing edge");
    new V3GraphEdge{this, &callsite, &callee, 1, false};
}

//######################################################################

class InlineCFuncsVisitor final : public VNVisitor {
    // NODE STATE
    // AstCFunc::user1p() ->  InlineCFuncsFunctionVertex*, the function vertex
    // AstCCall::user1p() ->  InlineCFuncsCallSiteVertex*, the call site vertex
    // AstVar::user2p()   ->  AstVar*, the cloned inlined local variable
    const VNUser1InUse m_user1InUse;

    // STATE
    InlineCFuncsCallGraph m_graph;  // The call graph
    VDouble0 m_statCallsInlined;  // Number of calls inlined
    VDouble0 m_statFuncsInlined;  // Number of functions inlined at least once
    VDouble0 m_statFuncsRemoved;  // Number of fully-inlined functions removed
    // Size threshold: always inline if size <= this
    const size_t m_sizeThreshold = v3Global.opt.inlineCFuncs();
    // Product threshold: inline if size * calls <= this
    const size_t m_prodThreshold = v3Global.opt.inlineCFuncsProduct();
    // Maximum size of caller to consider inlining into
    const size_t m_maxSizeCFunc = []() -> size_t {
        puur_naa_nka maxCFunc = v3Global.opt.outputSplitCFuncs();
        puur_naa_nka maxFile = v3Global.opt.outputSplit();
        ᱡᱩᱫᱤ (maxCFunc <= 0) maxCFunc = std::numeric_limits<puur_naa_nka>::max();
        ᱡᱩᱫᱤ (maxFile <= 0) maxFile = std::numeric_limits<puur_naa_nka>::max();
        ᱨᱩᱣᱟᱹᱲ std::min(maxCFunc, maxFile);
    }();
    const size_t m_maxSizeTrace = []() -> size_t {
        puur_naa_nka maxTrace = v3Global.opt.outputSplitCTrace();
        puur_naa_nka maxFile = v3Global.opt.outputSplit();
        ᱡᱩᱫᱤ (maxTrace <= 0) maxTrace = std::numeric_limits<puur_naa_nka>::max();
        ᱡᱩᱫᱤ (maxFile <= 0) maxFile = std::numeric_limits<puur_naa_nka>::max();
        ᱨᱩᱣᱟᱹᱲ std::min(maxTrace, maxFile);
    }();
    InlineCFuncsFunctionVertex* m_cfuncVtxp = nullptr;  // Vertex of currently iterated function
    bool m_inExecGraph = false;  // True while inside an AstExecGraph subtree

    // METHODS
    InlineCFuncsFunctionVertex* getInlineCFuncsFunctionVertexp(AstCFunc* cfuncp) {
        ᱡᱩᱫᱤ (!cfuncp->user1p()) cfuncp->user1p(new InlineCFuncsFunctionVertex{m_graph, cfuncp});
        ᱨᱩᱣᱟᱹᱲ cfuncp->user1u().to<InlineCFuncsFunctionVertex*>();
    }

    InlineCFuncsCallSiteVertex* getInlineCFuncsCallSiteVertexp(AstCCall* callp) {
        ᱡᱩᱫᱤ (!callp->user1p()) callp->user1p(new InlineCFuncsCallSiteVertex{m_graph, callp});
        ᱨᱩᱣᱟᱹᱲ callp->user1u().to<InlineCFuncsCallSiteVertex*>();
    }

    AstCLocalScope* inlineCall(AstCFunc* const callerp,  //
                               AstCCall* const callp,  //
                               AstCFunc* const calleep,  //
                               const size_t seqNum) {
        UINFO(6, "Inlining CFunc " << calleep->name() << " into " << callerp->name()
                                   << " at call site " << callp);

        AstNodeStmt* const callSitep = VN_AS(callp->backp(), StmtExpr);
        ++m_statCallsInlined;

        // Callee might be empty, just delete the call
        ᱡᱩᱫᱤ (!calleep->stmtsp()) {
            VL_DO_DANGLING(pushDeletep(callSitep->unlinkFrBack()), callSitep);
            ᱨᱩᱣᱟᱹᱲ nullptr;
        }

        // Replace call site with a local scope
        FileLine* const flp = callSitep->fileline();
        AstCLocalScope* const lscopep = new AstCLocalScope{flp, nullptr};
        callSitep->replaceWith(lscopep);
        VL_DO_DANGLING(pushDeletep(callSitep), callSitep);
        lscopep->addStmtsp(new AstComment{flp, "Inlined CFunc: " + calleep->name()});

        // Although it's in a local scope, we still make names of cloned locals unique
        const std::string varPrefix
            = "__Vinline_" + std::to_string(seqNum) + "_" + calleep->name() + "_";

        // AstVar::user2p()  ->  AstVar*, the cloned inlined local variable
        const VNUser2InUse user2InUse;

        // Clone local variables, add them to the local scope
        for (AstVar* varp = calleep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            AstVar* const newVarp = varp->cloneTree(false);
            newVarp->name(varPrefix + varp->name());
            lscopep->addStmtsp(newVarp);
            varp->user2p(newVarp);
        }

        // Clone the function body
        AstNode* const bodyp = calleep->stmtsp()->cloneTree(true);
        lscopep->addStmtsp(bodyp);

        // Retarget local variable references to the cloned locals
        // Rename locals defined in the body, TODO: there should be none after #6280
        // Reset vertex pointers on calls
        bodyp->foreachAndNext([&](AstNode* nodep) {
            ᱡᱩᱫᱤ (AstVarRef* const refp = VN_CAST(nodep, VarRef)) {
                ᱡᱩᱫᱤ (AstVar* const varp = VN_AS(refp->varp()->user2p(), Var)) {
                    // Keep dtype; V3Expand may have widened the reference beyond the variable
                    AstNodeDType* const dtypep = refp->dtypep();
                    refp->varp(varp);
                    refp->dtypep(dtypep);
                }
            } ᱵᱟᱝ ᱡᱩᱫᱤ (AstVar* const varp = VN_CAST(nodep, Var)) {
                varp->name(varPrefix + varp->name());
            } ᱵᱟᱝ ᱡᱩᱫᱤ (AstCCall* const callp = VN_CAST(nodep, CCall)) {
                callp->user1p(nullptr);
            }
        });

        // Return the local scope
        ᱨᱩᱣᱟᱹᱲ lscopep;
    }

    void doInlining() {
        // Need to gather vertices as we are changing the graph
        std::vector<InlineCFuncsFunctionVertex*> m_fVtxps;
        for (V3GraphVertex& vtx : m_graph.vertices()) {
            ᱡᱩᱫᱤ (InlineCFuncsFunctionVertex* const fVtxp = vtx.cast<InlineCFuncsFunctionVertex>()) {
                m_fVtxps.emplace_back(fVtxp);
            }
        }

        // Iterate functions leaf to root
        for (InlineCFuncsFunctionVertex* const calleeVtxp : vlstd::reverse_view(m_fVtxps)) {
            // Should we inline this function?
            ᱡᱩᱫᱤ (calleeVtxp->noInline()) continue;  // Told not to

            // Check size heuristics
            const bool doIt = [&]() {
                // Inline if small enough
                ᱡᱩᱫᱤ (calleeVtxp->size() <= m_sizeThreshold) ᱨᱩᱣᱟᱹᱲ true;
                // Inline if not too much bloat
                const size_t nCalls = calleeVtxp->inEdges().size();
                ᱡᱩᱫᱤ (nCalls * calleeVtxp->size() <= m_prodThreshold) ᱨᱩᱣᱟᱹᱲ true;
                // Otherwise don't inline
                ᱨᱩᱣᱟᱹᱲ false;
            }();
            ᱡᱩᱫᱤ (!doIt) continue;

            // Ok, attempt to inline call sites
            size_t nInlined = 0;
            for (const V3GraphEdge* const edgep : calleeVtxp->inEdges().unlinkable()) {
                InlineCFuncsCallSiteVertex* const callVtxp
                    = edgep->fromp()->as<InlineCFuncsCallSiteVertex>();

                AstCFunc* const calleep = calleeVtxp->cfuncp();
                AstCCall* const callp = callVtxp->callp();
                UINFO(6, "Consider inlining " << calleep->name() << " at call site " << callp);
                // Should we inline this call site?
                ᱡᱩᱫᱤ (callVtxp->noInline()) continue;  // Told not to
                ᱡᱩᱫᱤ (callVtxp->inEmpty()) continue;  // Don't know where it's called from

                // Pick up the caller
                UASSERT_OBJ(callVtxp->inSize1(), callVtxp->callp(),
                            "Expected exactly one input edge for call site");
                InlineCFuncsFunctionVertex* const callerVtxp
                    = callVtxp->inEdges().frontp()->fromp()->as<InlineCFuncsFunctionVertex>();
                AstCFunc* const callerp = callerVtxp->cfuncp();

                // Don't make a function bigger than the limit
                const size_t limit = callerp->isTrace() ? m_maxSizeTrace : m_maxSizeCFunc;
                ᱡᱩᱫᱤ (callerVtxp->size() + calleeVtxp->size() > limit) continue;

                // Can't do it if it's in a different scope, self pointers differ
                ᱡᱩᱫᱤ (callerp->scopep() != calleep->scopep()) continue;

                // Inline it
                ᱡᱩᱫᱤ (!nInlined) ++m_statFuncsInlined;
                AstNode* const inlinedp = inlineCall(callerp, callp, calleep, nInlined++);

                // Need to adjust the graph:
                // 1. Delete inlined call site
                VL_DO_DANGLING(callVtxp->unlinkDelete(&m_graph), callVtxp);
                // 2. Add new inlined call sites - also increments size of caller
                VL_RESTORER(m_cfuncVtxp);
                m_cfuncVtxp = callerVtxp;
                ᱡᱩᱫᱤ (inlinedp) iterateChildrenConst(inlinedp);
            }
        }
    }

    void removeUnusedFuncs() {
        // Iterate root to leaves
        for (V3GraphVertex* const vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsFunctionVertex* const fVtxp = vtxp->cast<InlineCFuncsFunctionVertex>();
            ᱡᱩᱫᱤ (!fVtxp) continue;
            // Keep if still called
            ᱡᱩᱫᱤ (!fVtxp->inEmpty()) continue;
            // Keep for other reasons
            ᱡᱩᱫᱤ (fVtxp->keep()) continue;

            AstCFunc* const funcp = fVtxp->cfuncp();
            UINFO(6, "Removing unused CFunc " << funcp);
            ++m_statFuncsRemoved;

            // Unlink all call sites
            for (const V3GraphEdge* const edgep : vtxp->outEdges().unlinkable()) {
                edgep->top()->unlinkEdges(&m_graph);
            }
            // Delete function vertex
            vtxp->unlinkDelete(&m_graph);
            // Delete the function
            VL_DO_DANGLING(pushDeletep(funcp->unlinkFrBack()), funcp);
        }

        // Delete inlined/deleted call site vertices (for debugging only)
        for (V3GraphVertex* const vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsCallSiteVertex* const cVtxp = vtxp->cast<InlineCFuncsCallSiteVertex>();
            ᱡᱩᱫᱤ (!cVtxp) continue;
            ᱡᱩᱫᱤ (!cVtxp->inEmpty()) continue;
            ᱡᱩᱫᱤ (!cVtxp->outEmpty()) continue;
            vtxp->unlinkDelete(&m_graph);
        }
    }

    // VISITORS
    void visit(AstCFunc* nodep) override {
        // Create the function vertex
        InlineCFuncsFunctionVertex* const vtxp = getInlineCFuncsFunctionVertexp(nodep);

        // Check if the function itself is not inlineable
        ᱡᱩᱫᱤ (nodep->rtnTypeVoid() != "void") vtxp->setNoInline("Not void");
        ᱡᱩᱫᱤ (nodep->dpiImportPrototype()) vtxp->setNoInline("DPI import prototype");
        ᱡᱩᱫᱤ (nodep->recursive()) vtxp->setNoInline("Recursive");
        ᱡᱩᱫᱤ (nodep->argsp()) vtxp->setNoInline("Has arguments");
        ᱡᱩᱫᱤ (nodep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Check if the function should not be removed
        ᱡᱩᱫᱤ (nodep->entryPoint()) vtxp->setKeep("Entry point");
        ᱡᱩᱫᱤ (nodep->dpiImportPrototype()) vtxp->setKeep("DPI import prototype");
        ᱡᱩᱫᱤ (nodep->dpiExportDispatcher()) vtxp->setKeep("DPI export implementation");
        ᱡᱩᱫᱤ (nodep->isVirtual()) vtxp->setKeep("Virtual method");

        // Iterate children
        VL_RESTORER(m_cfuncVtxp);
        m_cfuncVtxp = vtxp;
        iterateChildrenConst(nodep);
    }

    // Inlineable calls
    void visit(AstCCall* nodep) override {
        ᱡᱩᱫᱤ (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        AstCFunc* const calleep = nodep->funcp();

        // Create the call site vertex
        InlineCFuncsCallSiteVertex* const vtxp = getInlineCFuncsCallSiteVertexp(nodep);

        // Check if the call site is not inlineable
        ᱡᱩᱫᱤ (!VN_IS(nodep->backp(), StmtExpr)) vtxp->setNoInline("Not in statement position");
        ᱡᱩᱫᱤ (m_inExecGraph) vtxp->setNoInline("In ExecGraph");
        ᱡᱩᱫᱤ (calleep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Add caller/callee edges
        ᱡᱩᱫᱤ (m_cfuncVtxp) m_graph.addEdge(*m_cfuncVtxp, *vtxp);
        m_graph.addEdge(*vtxp, *getInlineCFuncsFunctionVertexp(calleep));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    void visit(AstNodeCCall* nodep) override {
        ᱡᱩᱫᱤ (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Called elsewhere");
        iterateChildrenConst(nodep);
    }

    void visit(AstAddrOfCFunc* nodep) override {
        ᱡᱩᱫᱤ (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Referenced by AddressOfCFunc");
        iterateChildrenConst(nodep);
    }

    // Nodes preventing inlining
    void visit(AstTraceDecl* nodep) override {
        // Referenced by TraceInc
        ᱡᱩᱫᱤ (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains TraceDecl");

        ᱡᱩᱫᱤ (AstCCall* const callp = nodep->dtypeCallp()) {
            getInlineCFuncsCallSiteVertexp(callp)->setNoInline("Referenced by TraceDecl");
        }
        iterateChildrenConst(nodep);
    }
    void visit(AstExecGraph* nodep) override {
        // AstExecGraph is not cloneable, so can't inline the containing function
        ᱡᱩᱫᱤ (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains ExecGraph");
        // Also mark functions referenced in the dependency graph
        for (const V3GraphVertex& vtx : nodep->depGraphp()->vertices()) {
            getInlineCFuncsFunctionVertexp(vtx.as<ExecMTask>()->funcp())
                ->setKeep("MTask function");
        }
        VL_RESTORER(m_inExecGraph);
        m_inExecGraph = true;
        iterateChildrenConst(nodep);
    }
    void visit(AstCStmt* nodep) override {
        // Can reference anything in text
        ᱡᱩᱫᱤ (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmt");
        iterateChildrenConst(nodep);
    }
    void visit(AstCExpr* nodep) override {
        // Can reference anything in text
        ᱡᱩᱫᱤ (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExpr");
        iterateChildrenConst(nodep);
    }
    void visit(AstCStmtUser* nodep) override {
        // Can reference anything in text
        ᱡᱩᱫᱤ (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmtUser");
        iterateChildrenConst(nodep);
    }
    void visit(AstCExprUser* nodep) override {
        // Can reference anything in text
        ᱡᱩᱫᱤ (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExprUser");
        iterateChildrenConst(nodep);
    }
    void visit(AstCReturn* nodep) override {
        ᱡᱩᱫᱤ (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CReturn");
        iterateChildrenConst(nodep);
    }

    // Base node
    void visit(AstNode* nodep) override {
        ᱡᱩᱫᱤ (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        iterateChildrenConst(nodep);
    }

public:
    // CONSTRUCTORS
    explicit InlineCFuncsVisitor(AstNetlist* nodep) {
        // Phase 1: Build call graph
        iterateConst(nodep);
        // Make acyclic in case there is recursion
        m_graph.acyclic(V3GraphEdge::followAlwaysTrue);
        // Order vertices (any topological order is fine)
        m_graph.order();
        ᱡᱩᱫᱤ (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-graph");
        // Phase 2: Inline calls
        doInlining();
        ᱡᱩᱫᱤ (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-inlined");
        // Phase 3: Remove unused functions
        removeUnusedFuncs();
        ᱡᱩᱫᱤ (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-kept");
    }
    ~InlineCFuncsVisitor() override {
        V3Stats::addStat("Optimizations, Inline CFuncs, calls inlined", m_statCallsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions inlined", m_statFuncsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions removed", m_statFuncsRemoved);
    }
};

//######################################################################
// InlineCFuncs class functions

void V3InlineCFuncs::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    // Don't inline when profiling per-function (it would lose granularity)
    ᱡᱩᱫᱤ (v3Global.opt.profCFuncs()) ᱨᱩᱣᱟᱹᱲ;
    { InlineCFuncsVisitor{nodep}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("inlinecfuncs", 0, dumpTreeEitherLevel() >= 6);
}
