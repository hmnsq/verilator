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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3InlineCFuncs.h"

#অন্তর্ভাব "V3AstUserAllocator.h"
#অন্তর্ভাব "V3ExecGraph.h"
#অন্তর্ভাব "V3Graph.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Bipartite call graph containing function and call site vertices

বর্গ InlineCFuncsFunctionVertex;
বর্গ InlineCFuncsCallSiteVertex;

বর্গ InlineCFuncsCallGraph final : সার্বজনিক V3Graph {
সার্বজনিক:
    InlineCFuncsCallGraph()
        : V3Graph{} {}
    ~InlineCFuncsCallGraph() override = স্বভাব;

    শূন্য addEdge(InlineCFuncsFunctionVertex& from, InlineCFuncsCallSiteVertex& শীর্ষ);
    শূন্য addEdge(InlineCFuncsCallSiteVertex& from, InlineCFuncsFunctionVertex& শীর্ষ);
};

বর্গ EitherVertex VL_NOT_FINAL : সার্বজনিক V3GraphVertex {
    VL_RTTI_IMPL(EitherVertex, V3GraphVertex)
রক্ষিত:
    explicit EitherVertex(InlineCFuncsCallGraph& graph)
        : V3GraphVertex{&graph} {}
};

বর্গ InlineCFuncsFunctionVertex final : সার্বজনিক EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsFunctionVertex, EitherVertex)
    AstCFunc* ধ্রুব m_cfuncp;  // The function
    ধ্রুব বর্ণ* m_noInlineWyp = nullptr;  // First reason the function should not be inlined
    ধ্রুব বর্ণ* m_keepWyp = nullptr;  // Why the function should not be removed
    size_t m_size = 0;  // The size of the function

সার্বজনিক:
    InlineCFuncsFunctionVertex(InlineCFuncsCallGraph& graph, AstCFunc* cfuncp)
        : EitherVertex{graph}
        , m_cfuncp{cfuncp} {}
    ~InlineCFuncsFunctionVertex() override = স্বভাব;

    // ACCESSORS
    AstCFunc* cfuncp() ধ্রুব { প্রত্যাগম m_cfuncp; }
    size_t size() ধ্রুব { প্রত্যাগম m_size; }
    শূন্য sizeInc(size_t মান = 1) { m_size += মান; }
    তার্কিক noInline() ধ্রুব { প্রত্যাগম m_noInlineWyp; }
    শূন্য setNoInline(ধ্রুব বর্ণ* whyp) {
        যদি (!m_noInlineWyp) m_noInlineWyp = whyp;
    }
    তার্কিক keep() ধ্রুব { প্রত্যাগম m_keepWyp; }
    শূন্য setKeep(ধ্রুব বর্ণ* whyp) {
        যদি (!m_keepWyp) m_keepWyp = whyp;
    }
    std::string dotColor() ধ্রুব override {
        প্রত্যাগম m_noInlineWyp ? "red" : m_keepWyp ? "orange" : "black";
    }

    // debug
    File_সারি* fileline() ধ্রুব override { প্রত্যাগম m_cfuncp->fileline(); }
    std::string dotShape() ধ্রুব override { প্রত্যাগম "box"; }
    std::string name() ধ্রুব override VL_MT_STABLE {
        std::string str = cvtToHex(m_cfuncp);
        str += "\n" + m_cfuncp->name();
        str += "\nsize: " + std::to_string(m_size);
        যদি (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        যদি (m_keepWyp) str += "\nKeep: "s + m_keepWyp;
        প্রত্যাগম str;
    }
};

বর্গ InlineCFuncsCallSiteVertex final : সার্বজনিক EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsCallSiteVertex, EitherVertex)
    AstCCall* ধ্রুব m_callp;  // The call site
    ধ্রুব বর্ণ* m_noInlineWyp = nullptr;  // First reason the function should not be inlined

সার্বজনিক:
    InlineCFuncsCallSiteVertex(InlineCFuncsCallGraph& graph, AstCCall* callp)
        : EitherVertex{graph}
        , m_callp{callp} {}
    ~InlineCFuncsCallSiteVertex() override = স্বভাব;

    // ACCESSORS
    AstCCall* callp() ধ্রুব { প্রত্যাগম m_callp; }
    তার্কিক noInline() ধ্রুব { প্রত্যাগম m_noInlineWyp; }
    শূন্য setNoInline(ধ্রুব বর্ণ* whyp) {
        যদি (!m_noInlineWyp) m_noInlineWyp = whyp;
    }

    // debug
    File_সারি* fileline() ধ্রুব override { প্রত্যাগম m_callp->fileline(); }
    std::string dotColor() ধ্রুব override { প্রত্যাগম m_noInlineWyp ? "red" : "black"; }
    std::string dotShape() ধ্রুব override { প্রত্যাগম "ellipse"; }
    std::string name() ধ্রুব override VL_MT_STABLE {
        std::string str = cvtToHex(m_callp);
        যদি (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        প্রত্যাগম str;
    }
};

শূন্য InlineCFuncsCallGraph::addEdge(InlineCFuncsFunctionVertex& caller,
                                    InlineCFuncsCallSiteVertex& callsite) {
    UASSERT_OBJ(callsite.inEmpty(), &callsite, "Call site should have at most one incoming edge");
    নূতন V3GraphEdge{এতত্, &caller, &callsite, 1, সত্য};  // Can cut caller -> callsite
}
শূন্য InlineCFuncsCallGraph::addEdge(InlineCFuncsCallSiteVertex& callsite,
                                    InlineCFuncsFunctionVertex& callee) {
    UASSERT_OBJ(callsite.outEmpty(), &callsite, "Call site should have at most one outgoing edge");
    নূতন V3GraphEdge{এতত্, &callsite, &callee, 1, অসত্য};
}

//######################################################################

বর্গ InlineCFuncsVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    // AstCFunc::user1p() ->  InlineCFuncsFunctionVertex*, the function vertex
    // AstCCall::user1p() ->  InlineCFuncsCallSiteVertex*, the call site vertex
    // AstVar::user2p()   ->  AstVar*, the cloned inlined local variable
    ধ্রুব VNUser1InUse m_user1InUse;

    // STATE
    InlineCFuncsCallGraph m_graph;  // The call graph
    VDouble0 m_statCallsInlined;  // Number of calls inlined
    VDouble0 m_statFuncsInlined;  // Number of functions inlined at least once
    VDouble0 m_statFuncsRemoved;  // Number of fully-inlined functions removed
    // Size threshold: always inline if size <= this
    ধ্রুব size_t m_sizeThreshold = v3Global.opt.inlineCFuncs();
    // Product threshold: inline if size * calls <= this
    ধ্রুব size_t m_prodThreshold = v3Global.opt.inlineCFuncsProduct();
    // Maximum size of caller to consider inlining into
    ধ্রুব size_t m_maxSizeCFunc = []() -> size_t {
        পূর্ণাঙ্ক maxCFunc = v3Global.opt.outputSplitCFuncs();
        পূর্ণাঙ্ক maxFile = v3Global.opt.outputSplit();
        যদি (maxCFunc <= 0) maxCFunc = std::numeric_limits<পূর্ণাঙ্ক>::max();
        যদি (maxFile <= 0) maxFile = std::numeric_limits<পূর্ণাঙ্ক>::max();
        প্রত্যাগম std::min(maxCFunc, maxFile);
    }();
    ধ্রুব size_t m_maxSizeTrace = []() -> size_t {
        পূর্ণাঙ্ক maxTrace = v3Global.opt.outputSplitCTrace();
        পূর্ণাঙ্ক maxFile = v3Global.opt.outputSplit();
        যদি (maxTrace <= 0) maxTrace = std::numeric_limits<পূর্ণাঙ্ক>::max();
        যদি (maxFile <= 0) maxFile = std::numeric_limits<পূর্ণাঙ্ক>::max();
        প্রত্যাগম std::min(maxTrace, maxFile);
    }();
    InlineCFuncsFunctionVertex* m_cfuncVtxp = nullptr;  // Vertex of currently iterated function
    তার্কিক m_inExecGraph = অসত্য;  // True while inside an AstExecGraph subtree

    // METHODS
    InlineCFuncsFunctionVertex* getInlineCFuncsFunctionVertexp(AstCFunc* cfuncp) {
        যদি (!cfuncp->user1p()) cfuncp->user1p(নূতন InlineCFuncsFunctionVertex{m_graph, cfuncp});
        প্রত্যাগম cfuncp->user1u().to<InlineCFuncsFunctionVertex*>();
    }

    InlineCFuncsCallSiteVertex* getInlineCFuncsCallSiteVertexp(AstCCall* callp) {
        যদি (!callp->user1p()) callp->user1p(নূতন InlineCFuncsCallSiteVertex{m_graph, callp});
        প্রত্যাগম callp->user1u().to<InlineCFuncsCallSiteVertex*>();
    }

    AstCLocalScope* inlineCall(AstCFunc* ধ্রুব callerp,  //
                               AstCCall* ধ্রুব callp,  //
                               AstCFunc* ধ্রুব calleep,  //
                               ধ্রুব size_t seq_অঙ্ক) {
        UINFO(6, "Inlining CFunc " << calleep->name() << " into " << callerp->name()
                                   << " at call site " << callp);

        Ast_গ্রন্থি_Stmt* ধ্রুব callSitep = VN_AS(callp->backp(), Stmt_রাশিমালা);
        ++m_statCallsInlined;

        // Callee might be empty, just delete the call
        যদি (!calleep->stmtsp()) {
            VL_DO_DANGLING(ঠেলো_Deletep(callSitep->unlinkFrBack()), callSitep);
            প্রত্যাগম nullptr;
        }

        // Replace call site with a local scope
        File_সারি* ধ্রুব flp = callSitep->fileline();
        AstCLocalScope* ধ্রুব lscopep = নূতন AstCLocalScope{flp, nullptr};
        callSitep->replaceWith(lscopep);
        VL_DO_DANGLING(ঠেলো_Deletep(callSitep), callSitep);
        lscopep->addStmtsp(নূতন AstComment{flp, "Inlined CFunc: " + calleep->name()});

        // Although it's in a local scope, we still make names of cloned locals unique
        ধ্রুব std::string varPrefix
            = "__Vinline_" + std::to_string(seq_অঙ্ক) + "_" + calleep->name() + "_";

        // AstVar::user2p()  ->  AstVar*, the cloned inlined local variable
        ধ্রুব VNUser2InUse user2InUse;

        // Clone local variables, add them to the local scope
        প্রতি (AstVar* varp = calleep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            AstVar* ধ্রুব newVarp = varp->clone_বৃক্ষ(অসত্য);
            newVarp->name(varPrefix + varp->name());
            lscopep->addStmtsp(newVarp);
            varp->user2p(newVarp);
        }

        // Clone the function body
        Ast_গ্রন্থি* ধ্রুব bodyp = calleep->stmtsp()->clone_বৃক্ষ(সত্য);
        lscopep->addStmtsp(bodyp);

        // Retarget local variable references to the cloned locals
        // Rename locals defined in the body, TODO: there should be none after #6280
        // Reset vertex pointers on calls
        bodyp->foreachAndNext([&](Ast_গ্রন্থি* nodep) {
            যদি (AstVarRef* ধ্রুব refp = VN_CAST(nodep, VarRef)) {
                যদি (AstVar* ধ্রুব varp = VN_AS(refp->varp()->user2p(), Var)) {
                    // Keep dtype; V3Expand may have widened the reference beyond the variable
                    Ast_গ্রন্থি_DType* ধ্রুব dtypep = refp->dtypep();
                    refp->varp(varp);
                    refp->dtypep(dtypep);
                }
            } অন্যথা যদি (AstVar* ধ্রুব varp = VN_CAST(nodep, Var)) {
                varp->name(varPrefix + varp->name());
            } অন্যথা যদি (AstCCall* ধ্রুব callp = VN_CAST(nodep, CCall)) {
                callp->user1p(nullptr);
            }
        });

        // Return the local scope
        প্রত্যাগম lscopep;
    }

    শূন্য doInlining() {
        // Need to gather vertices as we are changing the graph
        std::vector<InlineCFuncsFunctionVertex*> m_fVtxps;
        প্রতি (V3GraphVertex& vtx : m_graph.vertices()) {
            যদি (InlineCFuncsFunctionVertex* ধ্রুব fVtxp = vtx.cast<InlineCFuncsFunctionVertex>()) {
                m_fVtxps.emplace_back(fVtxp);
            }
        }

        // Iterate functions leaf to root
        প্রতি (InlineCFuncsFunctionVertex* ধ্রুব calleeVtxp : vlstd::reverse_view(m_fVtxps)) {
            // Should we inline this function?
            যদি (calleeVtxp->noInline()) অনুবর্ত;  // Told not to

            // Check size heuristics
            ধ্রুব তার্কিক doIt = [&]() {
                // Inline if small enough
                যদি (calleeVtxp->size() <= m_sizeThreshold) প্রত্যাগম সত্য;
                // Inline if not too much bloat
                ধ্রুব size_t nCalls = calleeVtxp->inEdges().size();
                যদি (nCalls * calleeVtxp->size() <= m_prodThreshold) প্রত্যাগম সত্য;
                // Otherwise don't inline
                প্রত্যাগম অসত্য;
            }();
            যদি (!doIt) অনুবর্ত;

            // Ok, attempt to inline call sites
            size_t nInlined = 0;
            প্রতি (ধ্রুব V3GraphEdge* ধ্রুব edgep : calleeVtxp->inEdges().unlinkable()) {
                InlineCFuncsCallSiteVertex* ধ্রুব callVtxp
                    = edgep->fromp()->as<InlineCFuncsCallSiteVertex>();

                AstCFunc* ধ্রুব calleep = calleeVtxp->cfuncp();
                AstCCall* ধ্রুব callp = callVtxp->callp();
                UINFO(6, "Consider inlining " << calleep->name() << " at call site " << callp);
                // Should we inline this call site?
                যদি (callVtxp->noInline()) অনুবর্ত;  // Told not to
                যদি (callVtxp->inEmpty()) অনুবর্ত;  // Don't know where it's called from

                // Pick up the caller
                UASSERT_OBJ(callVtxp->inSize1(), callVtxp->callp(),
                            "Expected exactly one input edge for call site");
                InlineCFuncsFunctionVertex* ধ্রুব callerVtxp
                    = callVtxp->inEdges().frontp()->fromp()->as<InlineCFuncsFunctionVertex>();
                AstCFunc* ধ্রুব callerp = callerVtxp->cfuncp();

                // Don't make a function bigger than the limit
                ধ্রুব size_t limit = callerp->isTrace() ? m_maxSizeTrace : m_maxSizeCFunc;
                যদি (callerVtxp->size() + calleeVtxp->size() > limit) অনুবর্ত;

                // Can't do it if it's in a different scope, self pointers differ
                যদি (callerp->scopep() != calleep->scopep()) অনুবর্ত;

                // Inline it
                যদি (!nInlined) ++m_statFuncsInlined;
                Ast_গ্রন্থি* ধ্রুব inlinedp = inlineCall(callerp, callp, calleep, nInlined++);

                // Need to adjust the graph:
                // 1. Delete inlined call site
                VL_DO_DANGLING(callVtxp->unlinkDelete(&m_graph), callVtxp);
                // 2. Add new inlined call sites - also increments size of caller
                VL_RESTORER(m_cfuncVtxp);
                m_cfuncVtxp = callerVtxp;
                যদি (inlinedp) iterateChildrenConst(inlinedp);
            }
        }
    }

    শূন্য removeUnusedFuncs() {
        // Iterate root to leaves
        প্রতি (V3GraphVertex* ধ্রুব vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsFunctionVertex* ধ্রুব fVtxp = vtxp->cast<InlineCFuncsFunctionVertex>();
            যদি (!fVtxp) অনুবর্ত;
            // Keep if still called
            যদি (!fVtxp->inEmpty()) অনুবর্ত;
            // Keep for other reasons
            যদি (fVtxp->keep()) অনুবর্ত;

            AstCFunc* ধ্রুব funcp = fVtxp->cfuncp();
            UINFO(6, "Removing unused CFunc " << funcp);
            ++m_statFuncsRemoved;

            // Unlink all call sites
            প্রতি (ধ্রুব V3GraphEdge* ধ্রুব edgep : vtxp->outEdges().unlinkable()) {
                edgep->শীর্ষ()->unlinkEdges(&m_graph);
            }
            // Delete function vertex
            vtxp->unlinkDelete(&m_graph);
            // Delete the function
            VL_DO_DANGLING(ঠেলো_Deletep(funcp->unlinkFrBack()), funcp);
        }

        // Delete inlined/deleted call site vertices (for debugging only)
        প্রতি (V3GraphVertex* ধ্রুব vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsCallSiteVertex* ধ্রুব cVtxp = vtxp->cast<InlineCFuncsCallSiteVertex>();
            যদি (!cVtxp) অনুবর্ত;
            যদি (!cVtxp->inEmpty()) অনুবর্ত;
            যদি (!cVtxp->outEmpty()) অনুবর্ত;
            vtxp->unlinkDelete(&m_graph);
        }
    }

    // VISITORS
    শূন্য visit(AstCFunc* nodep) override {
        // Create the function vertex
        InlineCFuncsFunctionVertex* ধ্রুব vtxp = getInlineCFuncsFunctionVertexp(nodep);

        // Check if the function itself is not inlineable
        যদি (nodep->rtnTypeVoid() != "void") vtxp->setNoInline("Not void");
        যদি (nodep->dpiImportPrototype()) vtxp->setNoInline("DPI import prototype");
        যদি (nodep->recursive()) vtxp->setNoInline("Recursive");
        যদি (nodep->argsp()) vtxp->setNoInline("Has arguments");
        যদি (nodep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Check if the function should not be removed
        যদি (nodep->entryPoint()) vtxp->setKeep("Entry point");
        যদি (nodep->dpiImportPrototype()) vtxp->setKeep("DPI import prototype");
        যদি (nodep->dpiExportDispatcher()) vtxp->setKeep("DPI export implementation");
        যদি (nodep->isVirtual()) vtxp->setKeep("Virtual method");

        // Iterate children
        VL_RESTORER(m_cfuncVtxp);
        m_cfuncVtxp = vtxp;
        iterateChildrenConst(nodep);
    }

    // Inlineable calls
    শূন্য visit(AstCCall* nodep) override {
        যদি (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        AstCFunc* ধ্রুব calleep = nodep->funcp();

        // Create the call site vertex
        InlineCFuncsCallSiteVertex* ধ্রুব vtxp = getInlineCFuncsCallSiteVertexp(nodep);

        // Check if the call site is not inlineable
        যদি (!VN_IS(nodep->backp(), Stmt_রাশিমালা)) vtxp->setNoInline("Not in statement position");
        যদি (m_inExecGraph) vtxp->setNoInline("In ExecGraph");
        যদি (calleep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Add caller/callee edges
        যদি (m_cfuncVtxp) m_graph.addEdge(*m_cfuncVtxp, *vtxp);
        m_graph.addEdge(*vtxp, *getInlineCFuncsFunctionVertexp(calleep));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি_CCall* nodep) override {
        যদি (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Called elsewhere");
        iterateChildrenConst(nodep);
    }

    শূন্য visit(AstAddrOfCFunc* nodep) override {
        যদি (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Referenced by AddressOfCFunc");
        iterateChildrenConst(nodep);
    }

    // Nodes preventing inlining
    শূন্য visit(AstTraceDecl* nodep) override {
        // Referenced by TraceInc
        যদি (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains TraceDecl");

        যদি (AstCCall* ধ্রুব callp = nodep->dtypeCallp()) {
            getInlineCFuncsCallSiteVertexp(callp)->setNoInline("Referenced by TraceDecl");
        }
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstExecGraph* nodep) override {
        // AstExecGraph is not cloneable, so can't inline the containing function
        যদি (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains ExecGraph");
        // Also mark functions referenced in the dependency graph
        প্রতি (ধ্রুব V3GraphVertex& vtx : nodep->depGraphp()->vertices()) {
            getInlineCFuncsFunctionVertexp(vtx.as<ExecMTask>()->funcp())
                ->setKeep("MTask function");
        }
        VL_RESTORER(m_inExecGraph);
        m_inExecGraph = সত্য;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstCStmt* nodep) override {
        // Can reference anything in text
        যদি (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmt");
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstCExpr* nodep) override {
        // Can reference anything in text
        যদি (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExpr");
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstCStmtUser* nodep) override {
        // Can reference anything in text
        যদি (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmtUser");
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstCExprUser* nodep) override {
        // Can reference anything in text
        যদি (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExprUser");
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstCReturn* nodep) override {
        যদি (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CReturn");
        iterateChildrenConst(nodep);
    }

    // Base node
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যদি (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        iterateChildrenConst(nodep);
    }

সার্বজনিক:
    // CONSTRUCTORS
    explicit InlineCFuncsVisitor(AstNetlist* nodep) {
        // Phase 1: Build call graph
        iterateConst(nodep);
        // Make acyclic in case there is recursion
        m_graph.acyclic(V3GraphEdge::followAlwaysTrue);
        // Order vertices (any topological order is fine)
        m_graph.order();
        যদি (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-graph");
        // Phase 2: Inline calls
        doInlining();
        যদি (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-inlined");
        // Phase 3: Remove unused functions
        removeUnusedFuncs();
        যদি (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-kept");
    }
    ~InlineCFuncsVisitor() override {
        V3Stats::addStat("Optimizations, Inline CFuncs, calls inlined", m_statCallsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions inlined", m_statFuncsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions removed", m_statFuncsRemoved);
    }
};

//######################################################################
// InlineCFuncs class functions

শূন্য V3InlineCFuncs::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    // Don't inline when profiling per-function (it would lose granularity)
    যদি (v3Global.opt.profCFuncs()) প্রত্যাগম;
    { InlineCFuncsVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("inlinecfuncs", 0, dump_বৃক্ষ_Either_Level() >= 6);
}
