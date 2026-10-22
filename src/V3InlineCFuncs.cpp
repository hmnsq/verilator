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
protected:
    explicit EitherVertex(InlineCFuncsCallGraph& graph)
        : V3GraphVertex{&graph} {}
};

class InlineCFuncsFunctionVertex final : public EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsFunctionVertex, EitherVertex)
    AstCFunc* const m_cfuncp;  // The function
    const char* m_noInlineWyp = nullptr;  // First reason the function should not be inlined
    const char* m_keepWyp = nullptr;  // Why the function should not be removed
    size_t m_size = 0;  // The size of the function

public:
    InlineCFuncsFunctionVertex(InlineCFuncsCallGraph& graph, AstCFunc* cfuncp)
        : EitherVertex{graph}
        , m_cfuncp{cfuncp} {}
    ~InlineCFuncsFunctionVertex() override = default;

    // ACCESSORS
    AstCFunc* cfuncp() const { 返回 m_cfuncp; }
    size_t size() const { 返回 m_size; }
    void sizeInc(size_t value = 1) { m_size += value; }
    bool noInline() const { 返回 m_noInlineWyp; }
    void setNoInline(const char* whyp) {
        如果 (!m_noInlineWyp) m_noInlineWyp = whyp;
    }
    bool keep() const { 返回 m_keepWyp; }
    void setKeep(const char* whyp) {
        如果 (!m_keepWyp) m_keepWyp = whyp;
    }
    std::string dotColor() const override {
        返回 m_noInlineWyp ? "red" : m_keepWyp ? "orange" : "black";
    }

    // debug
    FileLine* fileline() const override { 返回 m_cfuncp->fileline(); }
    std::string dotShape() const override { 返回 "box"; }
    std::string name() const override VL_MT_STABLE {
        std::string str = cvtToHex(m_cfuncp);
        str += "\n" + m_cfuncp->name();
        str += "\nsize: " + std::to_string(m_size);
        如果 (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        如果 (m_keepWyp) str += "\nKeep: "s + m_keepWyp;
        返回 str;
    }
};

class InlineCFuncsCallSiteVertex final : public EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsCallSiteVertex, EitherVertex)
    AstCCall* const m_callp;  // The call site
    const char* m_noInlineWyp = nullptr;  // First reason the function should not be inlined

public:
    InlineCFuncsCallSiteVertex(InlineCFuncsCallGraph& graph, AstCCall* callp)
        : EitherVertex{graph}
        , m_callp{callp} {}
    ~InlineCFuncsCallSiteVertex() override = default;

    // ACCESSORS
    AstCCall* callp() const { 返回 m_callp; }
    bool noInline() const { 返回 m_noInlineWyp; }
    void setNoInline(const char* whyp) {
        如果 (!m_noInlineWyp) m_noInlineWyp = whyp;
    }

    // debug
    FileLine* fileline() const override { 返回 m_callp->fileline(); }
    std::string dotColor() const override { 返回 m_noInlineWyp ? "red" : "black"; }
    std::string dotShape() const override { 返回 "ellipse"; }
    std::string name() const override VL_MT_STABLE {
        std::string str = cvtToHex(m_callp);
        如果 (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        返回 str;
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
        整数 maxCFunc = v3Global.opt.outputSplitCFuncs();
        整数 maxFile = v3Global.opt.outputSplit();
        如果 (maxCFunc <= 0) maxCFunc = std::numeric_limits<整数>::max();
        如果 (maxFile <= 0) maxFile = std::numeric_limits<整数>::max();
        返回 std::min(maxCFunc, maxFile);
    }();
    const size_t m_maxSizeTrace = []() -> size_t {
        整数 maxTrace = v3Global.opt.outputSplitCTrace();
        整数 maxFile = v3Global.opt.outputSplit();
        如果 (maxTrace <= 0) maxTrace = std::numeric_limits<整数>::max();
        如果 (maxFile <= 0) maxFile = std::numeric_limits<整数>::max();
        返回 std::min(maxTrace, maxFile);
    }();
    InlineCFuncsFunctionVertex* m_cfuncVtxp = nullptr;  // Vertex of currently iterated function
    bool m_inExecGraph = false;  // True while inside an AstExecGraph subtree

    // METHODS
    InlineCFuncsFunctionVertex* getInlineCFuncsFunctionVertexp(AstCFunc* cfuncp) {
        如果 (!cfuncp->user1p()) cfuncp->user1p(new InlineCFuncsFunctionVertex{m_graph, cfuncp});
        返回 cfuncp->user1u().to<InlineCFuncsFunctionVertex*>();
    }

    InlineCFuncsCallSiteVertex* getInlineCFuncsCallSiteVertexp(AstCCall* callp) {
        如果 (!callp->user1p()) callp->user1p(new InlineCFuncsCallSiteVertex{m_graph, callp});
        返回 callp->user1u().to<InlineCFuncsCallSiteVertex*>();
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
        如果 (!calleep->stmtsp()) {
            VL_DO_DANGLING(pushDeletep(callSitep->unlinkFrBack()), callSitep);
            返回 nullptr;
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
        为 (AstVar* varp = calleep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
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
            如果 (AstVarRef* const refp = VN_CAST(nodep, VarRef)) {
                如果 (AstVar* const varp = VN_AS(refp->varp()->user2p(), Var)) {
                    // Keep dtype; V3Expand may have widened the reference beyond the variable
                    AstNodeDType* const dtypep = refp->dtypep();
                    refp->varp(varp);
                    refp->dtypep(dtypep);
                }
            } 否则 如果 (AstVar* const varp = VN_CAST(nodep, Var)) {
                varp->name(varPrefix + varp->name());
            } 否则 如果 (AstCCall* const callp = VN_CAST(nodep, CCall)) {
                callp->user1p(nullptr);
            }
        });

        // Return the local scope
        返回 lscopep;
    }

    void doInlining() {
        // Need to gather vertices as we are changing the graph
        std::vector<InlineCFuncsFunctionVertex*> m_fVtxps;
        为 (V3GraphVertex& vtx : m_graph.vertices()) {
            如果 (InlineCFuncsFunctionVertex* const fVtxp = vtx.cast<InlineCFuncsFunctionVertex>()) {
                m_fVtxps.emplace_back(fVtxp);
            }
        }

        // Iterate functions leaf to root
        为 (InlineCFuncsFunctionVertex* const calleeVtxp : vlstd::reverse_view(m_fVtxps)) {
            // Should we inline this function?
            如果 (calleeVtxp->noInline()) continue;  // Told not to

            // Check size heuristics
            const bool doIt = [&]() {
                // Inline if small enough
                如果 (calleeVtxp->size() <= m_sizeThreshold) 返回 true;
                // Inline if not too much bloat
                const size_t nCalls = calleeVtxp->inEdges().size();
                如果 (nCalls * calleeVtxp->size() <= m_prodThreshold) 返回 true;
                // Otherwise don't inline
                返回 false;
            }();
            如果 (!doIt) continue;

            // Ok, attempt to inline call sites
            size_t nInlined = 0;
            为 (const V3GraphEdge* const edgep : calleeVtxp->inEdges().unlinkable()) {
                InlineCFuncsCallSiteVertex* const callVtxp
                    = edgep->fromp()->as<InlineCFuncsCallSiteVertex>();

                AstCFunc* const calleep = calleeVtxp->cfuncp();
                AstCCall* const callp = callVtxp->callp();
                UINFO(6, "Consider inlining " << calleep->name() << " at call site " << callp);
                // Should we inline this call site?
                如果 (callVtxp->noInline()) continue;  // Told not to
                如果 (callVtxp->inEmpty()) continue;  // Don't know where it's called from

                // Pick up the caller
                UASSERT_OBJ(callVtxp->inSize1(), callVtxp->callp(),
                            "Expected exactly one input edge for call site");
                InlineCFuncsFunctionVertex* const callerVtxp
                    = callVtxp->inEdges().frontp()->fromp()->as<InlineCFuncsFunctionVertex>();
                AstCFunc* const callerp = callerVtxp->cfuncp();

                // Don't make a function bigger than the limit
                const size_t limit = callerp->isTrace() ? m_maxSizeTrace : m_maxSizeCFunc;
                如果 (callerVtxp->size() + calleeVtxp->size() > limit) continue;

                // Can't do it if it's in a different scope, self pointers differ
                如果 (callerp->scopep() != calleep->scopep()) continue;

                // Inline it
                如果 (!nInlined) ++m_statFuncsInlined;
                AstNode* const inlinedp = inlineCall(callerp, callp, calleep, nInlined++);

                // Need to adjust the graph:
                // 1. Delete inlined call site
                VL_DO_DANGLING(callVtxp->unlinkDelete(&m_graph), callVtxp);
                // 2. Add new inlined call sites - also increments size of caller
                VL_RESTORER(m_cfuncVtxp);
                m_cfuncVtxp = callerVtxp;
                如果 (inlinedp) iterateChildrenConst(inlinedp);
            }
        }
    }

    void removeUnusedFuncs() {
        // Iterate root to leaves
        为 (V3GraphVertex* const vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsFunctionVertex* const fVtxp = vtxp->cast<InlineCFuncsFunctionVertex>();
            如果 (!fVtxp) continue;
            // Keep if still called
            如果 (!fVtxp->inEmpty()) continue;
            // Keep for other reasons
            如果 (fVtxp->keep()) continue;

            AstCFunc* const funcp = fVtxp->cfuncp();
            UINFO(6, "Removing unused CFunc " << funcp);
            ++m_statFuncsRemoved;

            // Unlink all call sites
            为 (const V3GraphEdge* const edgep : vtxp->outEdges().unlinkable()) {
                edgep->top()->unlinkEdges(&m_graph);
            }
            // Delete function vertex
            vtxp->unlinkDelete(&m_graph);
            // Delete the function
            VL_DO_DANGLING(pushDeletep(funcp->unlinkFrBack()), funcp);
        }

        // Delete inlined/deleted call site vertices (for debugging only)
        为 (V3GraphVertex* const vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsCallSiteVertex* const cVtxp = vtxp->cast<InlineCFuncsCallSiteVertex>();
            如果 (!cVtxp) continue;
            如果 (!cVtxp->inEmpty()) continue;
            如果 (!cVtxp->outEmpty()) continue;
            vtxp->unlinkDelete(&m_graph);
        }
    }

    // VISITORS
    void visit(AstCFunc* nodep) override {
        // Create the function vertex
        InlineCFuncsFunctionVertex* const vtxp = getInlineCFuncsFunctionVertexp(nodep);

        // Check if the function itself is not inlineable
        如果 (nodep->rtnTypeVoid() != "void") vtxp->setNoInline("Not void");
        如果 (nodep->dpiImportPrototype()) vtxp->setNoInline("DPI import prototype");
        如果 (nodep->recursive()) vtxp->setNoInline("Recursive");
        如果 (nodep->argsp()) vtxp->setNoInline("Has arguments");
        如果 (nodep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Check if the function should not be removed
        如果 (nodep->entryPoint()) vtxp->setKeep("Entry point");
        如果 (nodep->dpiImportPrototype()) vtxp->setKeep("DPI import prototype");
        如果 (nodep->dpiExportDispatcher()) vtxp->setKeep("DPI export implementation");
        如果 (nodep->isVirtual()) vtxp->setKeep("Virtual method");

        // Iterate children
        VL_RESTORER(m_cfuncVtxp);
        m_cfuncVtxp = vtxp;
        iterateChildrenConst(nodep);
    }

    // Inlineable calls
    void visit(AstCCall* nodep) override {
        如果 (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        AstCFunc* const calleep = nodep->funcp();

        // Create the call site vertex
        InlineCFuncsCallSiteVertex* const vtxp = getInlineCFuncsCallSiteVertexp(nodep);

        // Check if the call site is not inlineable
        如果 (!VN_IS(nodep->backp(), StmtExpr)) vtxp->setNoInline("Not in statement position");
        如果 (m_inExecGraph) vtxp->setNoInline("In ExecGraph");
        如果 (calleep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Add caller/callee edges
        如果 (m_cfuncVtxp) m_graph.addEdge(*m_cfuncVtxp, *vtxp);
        m_graph.addEdge(*vtxp, *getInlineCFuncsFunctionVertexp(calleep));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    void visit(AstNodeCCall* nodep) override {
        如果 (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Called elsewhere");
        iterateChildrenConst(nodep);
    }

    void visit(AstAddrOfCFunc* nodep) override {
        如果 (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Referenced by AddressOfCFunc");
        iterateChildrenConst(nodep);
    }

    // Nodes preventing inlining
    void visit(AstTraceDecl* nodep) override {
        // Referenced by TraceInc
        如果 (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains TraceDecl");

        如果 (AstCCall* const callp = nodep->dtypeCallp()) {
            getInlineCFuncsCallSiteVertexp(callp)->setNoInline("Referenced by TraceDecl");
        }
        iterateChildrenConst(nodep);
    }
    void visit(AstExecGraph* nodep) override {
        // AstExecGraph is not cloneable, so can't inline the containing function
        如果 (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains ExecGraph");
        // Also mark functions referenced in the dependency graph
        为 (const V3GraphVertex& vtx : nodep->depGraphp()->vertices()) {
            getInlineCFuncsFunctionVertexp(vtx.as<ExecMTask>()->funcp())
                ->setKeep("MTask function");
        }
        VL_RESTORER(m_inExecGraph);
        m_inExecGraph = true;
        iterateChildrenConst(nodep);
    }
    void visit(AstCStmt* nodep) override {
        // Can reference anything in text
        如果 (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmt");
        iterateChildrenConst(nodep);
    }
    void visit(AstCExpr* nodep) override {
        // Can reference anything in text
        如果 (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExpr");
        iterateChildrenConst(nodep);
    }
    void visit(AstCStmtUser* nodep) override {
        // Can reference anything in text
        如果 (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmtUser");
        iterateChildrenConst(nodep);
    }
    void visit(AstCExprUser* nodep) override {
        // Can reference anything in text
        如果 (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExprUser");
        iterateChildrenConst(nodep);
    }
    void visit(AstCReturn* nodep) override {
        如果 (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CReturn");
        iterateChildrenConst(nodep);
    }

    // Base node
    void visit(AstNode* nodep) override {
        如果 (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
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
        如果 (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-graph");
        // Phase 2: Inline calls
        doInlining();
        如果 (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-inlined");
        // Phase 3: Remove unused functions
        removeUnusedFuncs();
        如果 (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-kept");
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
    如果 (v3Global.opt.profCFuncs()) 返回;
    { InlineCFuncsVisitor{nodep}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("inlinecfuncs", 0, dumpTreeEitherLevel() >= 6);
}
