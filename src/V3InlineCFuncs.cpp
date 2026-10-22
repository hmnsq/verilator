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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3InlineCFuncs.h"

#ضمن "V3AstUserAllocator.h"
#ضمن "V3ExecGraph.h"
#ضمن "V3Graph.h"
#ضمن "V3Stats.h"

#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Bipartite call graph containing function and call site vertices

صنف InlineCFuncsFunctionVertex;
صنف InlineCFuncsCallSiteVertex;

صنف InlineCFuncsCallGraph final : عام V3Graph {
عام:
    InlineCFuncsCallGraph()
        : V3Graph{} {}
    ~InlineCFuncsCallGraph() override = افتراضي;

    فراغ addEdge(InlineCFuncsFunctionVertex& from, InlineCFuncsCallSiteVertex& top);
    فراغ addEdge(InlineCFuncsCallSiteVertex& from, InlineCFuncsFunctionVertex& top);
};

صنف EitherVertex VL_NOT_FINAL : عام V3GraphVertex {
    VL_RTTI_IMPL(EitherVertex, V3GraphVertex)
محمي:
    explicit EitherVertex(InlineCFuncsCallGraph& graph)
        : V3GraphVertex{&graph} {}
};

صنف InlineCFuncsFunctionVertex final : عام EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsFunctionVertex, EitherVertex)
    AstCFunc* ثابت m_cfuncp;  // The function
    ثابت حرف* m_noInlineWyp = nullptr;  // First reason the function should not be inlined
    ثابت حرف* m_keepWyp = nullptr;  // Why the function should not be removed
    size_t m_size = 0;  // The size of the function

عام:
    InlineCFuncsFunctionVertex(InlineCFuncsCallGraph& graph, AstCFunc* cfuncp)
        : EitherVertex{graph}
        , m_cfuncp{cfuncp} {}
    ~InlineCFuncsFunctionVertex() override = افتراضي;

    // ACCESSORS
    AstCFunc* cfuncp() ثابت { أرجع m_cfuncp; }
    size_t size() ثابت { أرجع m_size; }
    فراغ sizeInc(size_t value = 1) { m_size += value; }
    منطقي noInline() ثابت { أرجع m_noInlineWyp; }
    فراغ setNoInline(ثابت حرف* whyp) {
        𐡄𐡍 (!m_noInlineWyp) m_noInlineWyp = whyp;
    }
    منطقي keep() ثابت { أرجع m_keepWyp; }
    فراغ setKeep(ثابت حرف* whyp) {
        𐡄𐡍 (!m_keepWyp) m_keepWyp = whyp;
    }
    std::string dotColor() ثابت override {
        أرجع m_noInlineWyp ? "red" : m_keepWyp ? "orange" : "black";
    }

    // debug
    FileLine* fileline() ثابت override { أرجع m_cfuncp->fileline(); }
    std::string dotShape() ثابت override { أرجع "box"; }
    std::string name() ثابت override VL_MT_STABLE {
        std::string str = cvtToHex(m_cfuncp);
        str += "\n" + m_cfuncp->name();
        str += "\nsize: " + std::to_string(m_size);
        𐡄𐡍 (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        𐡄𐡍 (m_keepWyp) str += "\nKeep: "s + m_keepWyp;
        أرجع str;
    }
};

صنف InlineCFuncsCallSiteVertex final : عام EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsCallSiteVertex, EitherVertex)
    AstCCall* ثابت m_callp;  // The call site
    ثابت حرف* m_noInlineWyp = nullptr;  // First reason the function should not be inlined

عام:
    InlineCFuncsCallSiteVertex(InlineCFuncsCallGraph& graph, AstCCall* callp)
        : EitherVertex{graph}
        , m_callp{callp} {}
    ~InlineCFuncsCallSiteVertex() override = افتراضي;

    // ACCESSORS
    AstCCall* callp() ثابت { أرجع m_callp; }
    منطقي noInline() ثابت { أرجع m_noInlineWyp; }
    فراغ setNoInline(ثابت حرف* whyp) {
        𐡄𐡍 (!m_noInlineWyp) m_noInlineWyp = whyp;
    }

    // debug
    FileLine* fileline() ثابت override { أرجع m_callp->fileline(); }
    std::string dotColor() ثابت override { أرجع m_noInlineWyp ? "red" : "black"; }
    std::string dotShape() ثابت override { أرجع "ellipse"; }
    std::string name() ثابت override VL_MT_STABLE {
        std::string str = cvtToHex(m_callp);
        𐡄𐡍 (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        أرجع str;
    }
};

فراغ InlineCFuncsCallGraph::addEdge(InlineCFuncsFunctionVertex& caller,
                                    InlineCFuncsCallSiteVertex& callsite) {
    UASSERT_OBJ(callsite.inEmpty(), &callsite, "Call site should have at most one incoming edge");
    جديد V3GraphEdge{هذا, &caller, &callsite, 1, 𐡒𐡔𐡈};  // Can cut caller -> callsite
}
فراغ InlineCFuncsCallGraph::addEdge(InlineCFuncsCallSiteVertex& callsite,
                                    InlineCFuncsFunctionVertex& callee) {
    UASSERT_OBJ(callsite.outEmpty(), &callsite, "Call site should have at most one outgoing edge");
    جديد V3GraphEdge{هذا, &callsite, &callee, 1, خطأ};
}

//######################################################################

صنف InlineCFuncsVisitor final : عام VNVisitor {
    // NODE STATE
    // AstCFunc::user1p() ->  InlineCFuncsFunctionVertex*, the function vertex
    // AstCCall::user1p() ->  InlineCFuncsCallSiteVertex*, the call site vertex
    // AstVar::user2p()   ->  AstVar*, the cloned inlined local variable
    ثابت VNUser1InUse m_user1InUse;

    // STATE
    InlineCFuncsCallGraph m_graph;  // The call graph
    VDouble0 m_statCallsInlined;  // Number of calls inlined
    VDouble0 m_statFuncsInlined;  // Number of functions inlined at least once
    VDouble0 m_statFuncsRemoved;  // Number of fully-inlined functions removed
    // Size threshold: always inline if size <= this
    ثابت size_t m_sizeThreshold = v3Global.opt.inlineCFuncs();
    // Product threshold: inline if size * calls <= this
    ثابت size_t m_prodThreshold = v3Global.opt.inlineCFuncsProduct();
    // Maximum size of caller to consider inlining into
    ثابت size_t m_maxSizeCFunc = []() -> size_t {
        صحيح maxCFunc = v3Global.opt.outputSplitCFuncs();
        صحيح maxFile = v3Global.opt.outputSplit();
        𐡄𐡍 (maxCFunc <= 0) maxCFunc = std::numeric_limits<صحيح>::max();
        𐡄𐡍 (maxFile <= 0) maxFile = std::numeric_limits<صحيح>::max();
        أرجع std::min(maxCFunc, maxFile);
    }();
    ثابت size_t m_maxSizeTrace = []() -> size_t {
        صحيح maxTrace = v3Global.opt.outputSplitCTrace();
        صحيح maxFile = v3Global.opt.outputSplit();
        𐡄𐡍 (maxTrace <= 0) maxTrace = std::numeric_limits<صحيح>::max();
        𐡄𐡍 (maxFile <= 0) maxFile = std::numeric_limits<صحيح>::max();
        أرجع std::min(maxTrace, maxFile);
    }();
    InlineCFuncsFunctionVertex* m_cfuncVtxp = nullptr;  // Vertex of currently iterated function
    منطقي m_inExecGraph = خطأ;  // True while inside an AstExecGraph subtree

    // METHODS
    InlineCFuncsFunctionVertex* getInlineCFuncsFunctionVertexp(AstCFunc* cfuncp) {
        𐡄𐡍 (!cfuncp->user1p()) cfuncp->user1p(جديد InlineCFuncsFunctionVertex{m_graph, cfuncp});
        أرجع cfuncp->user1u().to<InlineCFuncsFunctionVertex*>();
    }

    InlineCFuncsCallSiteVertex* getInlineCFuncsCallSiteVertexp(AstCCall* callp) {
        𐡄𐡍 (!callp->user1p()) callp->user1p(جديد InlineCFuncsCallSiteVertex{m_graph, callp});
        أرجع callp->user1u().to<InlineCFuncsCallSiteVertex*>();
    }

    AstCLocalScope* inlineCall(AstCFunc* ثابت callerp,  //
                               AstCCall* ثابت callp,  //
                               AstCFunc* ثابت calleep,  //
                               ثابت size_t seqNum) {
        UINFO(6, "Inlining CFunc " << calleep->name() << " into " << callerp->name()
                                   << " at call site " << callp);

        AstNodeStmt* ثابت callSitep = VN_AS(callp->backp(), StmtExpr);
        ++m_statCallsInlined;

        // Callee might be empty, just delete the call
        𐡄𐡍 (!calleep->stmtsp()) {
            VL_DO_DANGLING(pushDeletep(callSitep->unlinkFrBack()), callSitep);
            أرجع nullptr;
        }

        // Replace call site with a local scope
        FileLine* ثابت flp = callSitep->fileline();
        AstCLocalScope* ثابت lscopep = جديد AstCLocalScope{flp, nullptr};
        callSitep->replaceWith(lscopep);
        VL_DO_DANGLING(pushDeletep(callSitep), callSitep);
        lscopep->addStmtsp(جديد AstComment{flp, "Inlined CFunc: " + calleep->name()});

        // Although it's in a local scope, we still make names of cloned locals unique
        ثابت std::string varPrefix
            = "__Vinline_" + std::to_string(seqNum) + "_" + calleep->name() + "_";

        // AstVar::user2p()  ->  AstVar*, the cloned inlined local variable
        ثابت VNUser2InUse user2InUse;

        // Clone local variables, add them to the local scope
        لكل (AstVar* varp = calleep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            AstVar* ثابت newVarp = varp->cloneTree(خطأ);
            newVarp->name(varPrefix + varp->name());
            lscopep->addStmtsp(newVarp);
            varp->user2p(newVarp);
        }

        // Clone the function body
        AstNode* ثابت bodyp = calleep->stmtsp()->cloneTree(𐡒𐡔𐡈);
        lscopep->addStmtsp(bodyp);

        // Retarget local variable references to the cloned locals
        // Rename locals defined in the body, TODO: there should be none after #6280
        // Reset vertex pointers on calls
        bodyp->foreachAndNext([&](AstNode* nodep) {
            𐡄𐡍 (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
                𐡄𐡍 (AstVar* ثابت varp = VN_AS(refp->varp()->user2p(), Var)) {
                    // Keep dtype; V3Expand may have widened the reference beyond the variable
                    AstNodeDType* ثابت dtypep = refp->dtypep();
                    refp->varp(varp);
                    refp->dtypep(dtypep);
                }
            } وإلا 𐡄𐡍 (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                varp->name(varPrefix + varp->name());
            } وإلا 𐡄𐡍 (AstCCall* ثابت callp = VN_CAST(nodep, CCall)) {
                callp->user1p(nullptr);
            }
        });

        // Return the local scope
        أرجع lscopep;
    }

    فراغ doInlining() {
        // Need to gather vertices as we are changing the graph
        std::vector<InlineCFuncsFunctionVertex*> m_fVtxps;
        لكل (V3GraphVertex& vtx : m_graph.vertices()) {
            𐡄𐡍 (InlineCFuncsFunctionVertex* ثابت fVtxp = vtx.cast<InlineCFuncsFunctionVertex>()) {
                m_fVtxps.emplace_back(fVtxp);
            }
        }

        // Iterate functions leaf to root
        لكل (InlineCFuncsFunctionVertex* ثابت calleeVtxp : vlstd::reverse_view(m_fVtxps)) {
            // Should we inline this function?
            𐡄𐡍 (calleeVtxp->noInline()) استمر;  // Told not to

            // Check size heuristics
            ثابت منطقي doIt = [&]() {
                // Inline if small enough
                𐡄𐡍 (calleeVtxp->size() <= m_sizeThreshold) أرجع 𐡒𐡔𐡈;
                // Inline if not too much bloat
                ثابت size_t nCalls = calleeVtxp->inEdges().size();
                𐡄𐡍 (nCalls * calleeVtxp->size() <= m_prodThreshold) أرجع 𐡒𐡔𐡈;
                // Otherwise don't inline
                أرجع خطأ;
            }();
            𐡄𐡍 (!doIt) استمر;

            // Ok, attempt to inline call sites
            size_t nInlined = 0;
            لكل (ثابت V3GraphEdge* ثابت edgep : calleeVtxp->inEdges().unlinkable()) {
                InlineCFuncsCallSiteVertex* ثابت callVtxp
                    = edgep->fromp()->as<InlineCFuncsCallSiteVertex>();

                AstCFunc* ثابت calleep = calleeVtxp->cfuncp();
                AstCCall* ثابت callp = callVtxp->callp();
                UINFO(6, "Consider inlining " << calleep->name() << " at call site " << callp);
                // Should we inline this call site?
                𐡄𐡍 (callVtxp->noInline()) استمر;  // Told not to
                𐡄𐡍 (callVtxp->inEmpty()) استمر;  // Don't know where it's called from

                // Pick up the caller
                UASSERT_OBJ(callVtxp->inSize1(), callVtxp->callp(),
                            "Expected exactly one input edge for call site");
                InlineCFuncsFunctionVertex* ثابت callerVtxp
                    = callVtxp->inEdges().frontp()->fromp()->as<InlineCFuncsFunctionVertex>();
                AstCFunc* ثابت callerp = callerVtxp->cfuncp();

                // Don't make a function bigger than the limit
                ثابت size_t limit = callerp->isTrace() ? m_maxSizeTrace : m_maxSizeCFunc;
                𐡄𐡍 (callerVtxp->size() + calleeVtxp->size() > limit) استمر;

                // Can't do it if it's in a different scope, self pointers differ
                𐡄𐡍 (callerp->scopep() != calleep->scopep()) استمر;

                // Inline it
                𐡄𐡍 (!nInlined) ++m_statFuncsInlined;
                AstNode* ثابت inlinedp = inlineCall(callerp, callp, calleep, nInlined++);

                // Need to adjust the graph:
                // 1. Delete inlined call site
                VL_DO_DANGLING(callVtxp->unlinkDelete(&m_graph), callVtxp);
                // 2. Add new inlined call sites - also increments size of caller
                VL_RESTORER(m_cfuncVtxp);
                m_cfuncVtxp = callerVtxp;
                𐡄𐡍 (inlinedp) iterateChildrenConst(inlinedp);
            }
        }
    }

    فراغ removeUnusedFuncs() {
        // Iterate root to leaves
        لكل (V3GraphVertex* ثابت vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsFunctionVertex* ثابت fVtxp = vtxp->cast<InlineCFuncsFunctionVertex>();
            𐡄𐡍 (!fVtxp) استمر;
            // Keep if still called
            𐡄𐡍 (!fVtxp->inEmpty()) استمر;
            // Keep for other reasons
            𐡄𐡍 (fVtxp->keep()) استمر;

            AstCFunc* ثابت funcp = fVtxp->cfuncp();
            UINFO(6, "Removing unused CFunc " << funcp);
            ++m_statFuncsRemoved;

            // Unlink all call sites
            لكل (ثابت V3GraphEdge* ثابت edgep : vtxp->outEdges().unlinkable()) {
                edgep->top()->unlinkEdges(&m_graph);
            }
            // Delete function vertex
            vtxp->unlinkDelete(&m_graph);
            // Delete the function
            VL_DO_DANGLING(pushDeletep(funcp->unlinkFrBack()), funcp);
        }

        // Delete inlined/deleted call site vertices (for debugging only)
        لكل (V3GraphVertex* ثابت vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsCallSiteVertex* ثابت cVtxp = vtxp->cast<InlineCFuncsCallSiteVertex>();
            𐡄𐡍 (!cVtxp) استمر;
            𐡄𐡍 (!cVtxp->inEmpty()) استمر;
            𐡄𐡍 (!cVtxp->outEmpty()) استمر;
            vtxp->unlinkDelete(&m_graph);
        }
    }

    // VISITORS
    فراغ visit(AstCFunc* nodep) override {
        // Create the function vertex
        InlineCFuncsFunctionVertex* ثابت vtxp = getInlineCFuncsFunctionVertexp(nodep);

        // Check if the function itself is not inlineable
        𐡄𐡍 (nodep->rtnTypeVoid() != "void") vtxp->setNoInline("Not void");
        𐡄𐡍 (nodep->dpiImportPrototype()) vtxp->setNoInline("DPI import prototype");
        𐡄𐡍 (nodep->recursive()) vtxp->setNoInline("Recursive");
        𐡄𐡍 (nodep->argsp()) vtxp->setNoInline("Has arguments");
        𐡄𐡍 (nodep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Check if the function should not be removed
        𐡄𐡍 (nodep->entryPoint()) vtxp->setKeep("Entry point");
        𐡄𐡍 (nodep->dpiImportPrototype()) vtxp->setKeep("DPI import prototype");
        𐡄𐡍 (nodep->dpiExportDispatcher()) vtxp->setKeep("DPI export implementation");
        𐡄𐡍 (nodep->isVirtual()) vtxp->setKeep("Virtual method");

        // Iterate children
        VL_RESTORER(m_cfuncVtxp);
        m_cfuncVtxp = vtxp;
        iterateChildrenConst(nodep);
    }

    // Inlineable calls
    فراغ visit(AstCCall* nodep) override {
        𐡄𐡍 (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        AstCFunc* ثابت calleep = nodep->funcp();

        // Create the call site vertex
        InlineCFuncsCallSiteVertex* ثابت vtxp = getInlineCFuncsCallSiteVertexp(nodep);

        // Check if the call site is not inlineable
        𐡄𐡍 (!VN_IS(nodep->backp(), StmtExpr)) vtxp->setNoInline("Not in statement position");
        𐡄𐡍 (m_inExecGraph) vtxp->setNoInline("In ExecGraph");
        𐡄𐡍 (calleep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Add caller/callee edges
        𐡄𐡍 (m_cfuncVtxp) m_graph.addEdge(*m_cfuncVtxp, *vtxp);
        m_graph.addEdge(*vtxp, *getInlineCFuncsFunctionVertexp(calleep));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    فراغ visit(AstNodeCCall* nodep) override {
        𐡄𐡍 (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Called elsewhere");
        iterateChildrenConst(nodep);
    }

    فراغ visit(AstAddrOfCFunc* nodep) override {
        𐡄𐡍 (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Referenced by AddressOfCFunc");
        iterateChildrenConst(nodep);
    }

    // Nodes preventing inlining
    فراغ visit(AstTraceDecl* nodep) override {
        // Referenced by TraceInc
        𐡄𐡍 (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains TraceDecl");

        𐡄𐡍 (AstCCall* ثابت callp = nodep->dtypeCallp()) {
            getInlineCFuncsCallSiteVertexp(callp)->setNoInline("Referenced by TraceDecl");
        }
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstExecGraph* nodep) override {
        // AstExecGraph is not cloneable, so can't inline the containing function
        𐡄𐡍 (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains ExecGraph");
        // Also mark functions referenced in the dependency graph
        لكل (ثابت V3GraphVertex& vtx : nodep->depGraphp()->vertices()) {
            getInlineCFuncsFunctionVertexp(vtx.as<ExecMTask>()->funcp())
                ->setKeep("MTask function");
        }
        VL_RESTORER(m_inExecGraph);
        m_inExecGraph = 𐡒𐡔𐡈;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstCStmt* nodep) override {
        // Can reference anything in text
        𐡄𐡍 (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmt");
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstCExpr* nodep) override {
        // Can reference anything in text
        𐡄𐡍 (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExpr");
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstCStmtUser* nodep) override {
        // Can reference anything in text
        𐡄𐡍 (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmtUser");
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstCExprUser* nodep) override {
        // Can reference anything in text
        𐡄𐡍 (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExprUser");
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstCReturn* nodep) override {
        𐡄𐡍 (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CReturn");
        iterateChildrenConst(nodep);
    }

    // Base node
    فراغ visit(AstNode* nodep) override {
        𐡄𐡍 (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        iterateChildrenConst(nodep);
    }

عام:
    // CONSTRUCTORS
    explicit InlineCFuncsVisitor(AstNetlist* nodep) {
        // Phase 1: Build call graph
        iterateConst(nodep);
        // Make acyclic in case there is recursion
        m_graph.acyclic(V3GraphEdge::followAlwaysTrue);
        // Order vertices (any topological order is fine)
        m_graph.order();
        𐡄𐡍 (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-graph");
        // Phase 2: Inline calls
        doInlining();
        𐡄𐡍 (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-inlined");
        // Phase 3: Remove unused functions
        removeUnusedFuncs();
        𐡄𐡍 (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-kept");
    }
    ~InlineCFuncsVisitor() override {
        V3Stats::addStat("Optimizations, Inline CFuncs, calls inlined", m_statCallsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions inlined", m_statFuncsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions removed", m_statFuncsRemoved);
    }
};

//######################################################################
// InlineCFuncs class functions

فراغ V3InlineCFuncs::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    // Don't inline when profiling per-function (it would lose granularity)
    𐡄𐡍 (v3Global.opt.profCFuncs()) أرجع;
    { InlineCFuncsVisitor{nodep}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("inlinecfuncs", 0, dumpTreeEitherLevel() >= 6);
}
