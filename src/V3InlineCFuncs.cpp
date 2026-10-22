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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3InlineCFuncs.h"

#அந்தர்பாவ "V3AstUserAllocator.h"
#அந்தர்பாவ "V3ExecGraph.h"
#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Bipartite call graph containing function and call site vertices

வகுப்பு InlineCFuncsFunctionVertex;
வகுப்பு InlineCFuncsCallSiteVertex;

வகுப்பு InlineCFuncsCallGraph final : பொது V3Graph {
பொது:
    InlineCFuncsCallGraph()
        : V3Graph{} {}
    ~InlineCFuncsCallGraph() override = இயல்பு;

    வெறுமை addEdge(InlineCFuncsFunctionVertex& from, InlineCFuncsCallSiteVertex& உச்சி);
    வெறுமை addEdge(InlineCFuncsCallSiteVertex& from, InlineCFuncsFunctionVertex& உச்சி);
};

வகுப்பு EitherVertex VL_NOT_FINAL : பொது V3GraphVertex {
    VL_RTTI_IMPL(EitherVertex, V3GraphVertex)
பாதுகாக்கப்பட்ட:
    explicit EitherVertex(InlineCFuncsCallGraph& graph)
        : V3GraphVertex{&graph} {}
};

வகுப்பு InlineCFuncsFunctionVertex final : பொது EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsFunctionVertex, EitherVertex)
    AstCFunc* மாறிலி m_cfuncp;  // The function
    மாறிலி எழுத்து* m_noInlineWyp = nullptr;  // First reason the function should not be inlined
    மாறிலி எழுத்து* m_keepWyp = nullptr;  // Why the function should not be removed
    size_t m_size = 0;  // The size of the function

பொது:
    InlineCFuncsFunctionVertex(InlineCFuncsCallGraph& graph, AstCFunc* cfuncp)
        : EitherVertex{graph}
        , m_cfuncp{cfuncp} {}
    ~InlineCFuncsFunctionVertex() override = இயல்பு;

    // ACCESSORS
    AstCFunc* cfuncp() மாறிலி { திரும்பு m_cfuncp; }
    size_t size() மாறிலி { திரும்பு m_size; }
    வெறுமை sizeInc(size_t மதிப்பு = 1) { m_size += மதிப்பு; }
    தருக்க noInline() மாறிலி { திரும்பு m_noInlineWyp; }
    வெறுமை setNoInline(மாறிலி எழுத்து* whyp) {
        எனில் (!m_noInlineWyp) m_noInlineWyp = whyp;
    }
    தருக்க keep() மாறிலி { திரும்பு m_keepWyp; }
    வெறுமை setKeep(மாறிலி எழுத்து* whyp) {
        எனில் (!m_keepWyp) m_keepWyp = whyp;
    }
    std::string dotColor() மாறிலி override {
        திரும்பு m_noInlineWyp ? "red" : m_keepWyp ? "orange" : "black";
    }

    // debug
    File_வரி* fileline() மாறிலி override { திரும்பு m_cfuncp->fileline(); }
    std::string dotShape() மாறிலி override { திரும்பு "box"; }
    std::string name() மாறிலி override VL_MT_STABLE {
        std::string str = cvtToHex(m_cfuncp);
        str += "\n" + m_cfuncp->name();
        str += "\nsize: " + std::to_string(m_size);
        எனில் (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        எனில் (m_keepWyp) str += "\nKeep: "s + m_keepWyp;
        திரும்பு str;
    }
};

வகுப்பு InlineCFuncsCallSiteVertex final : பொது EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsCallSiteVertex, EitherVertex)
    AstCCall* மாறிலி m_callp;  // The call site
    மாறிலி எழுத்து* m_noInlineWyp = nullptr;  // First reason the function should not be inlined

பொது:
    InlineCFuncsCallSiteVertex(InlineCFuncsCallGraph& graph, AstCCall* callp)
        : EitherVertex{graph}
        , m_callp{callp} {}
    ~InlineCFuncsCallSiteVertex() override = இயல்பு;

    // ACCESSORS
    AstCCall* callp() மாறிலி { திரும்பு m_callp; }
    தருக்க noInline() மாறிலி { திரும்பு m_noInlineWyp; }
    வெறுமை setNoInline(மாறிலி எழுத்து* whyp) {
        எனில் (!m_noInlineWyp) m_noInlineWyp = whyp;
    }

    // debug
    File_வரி* fileline() மாறிலி override { திரும்பு m_callp->fileline(); }
    std::string dotColor() மாறிலி override { திரும்பு m_noInlineWyp ? "red" : "black"; }
    std::string dotShape() மாறிலி override { திரும்பு "ellipse"; }
    std::string name() மாறிலி override VL_MT_STABLE {
        std::string str = cvtToHex(m_callp);
        எனில் (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        திரும்பு str;
    }
};

வெறுமை InlineCFuncsCallGraph::addEdge(InlineCFuncsFunctionVertex& caller,
                                    InlineCFuncsCallSiteVertex& callsite) {
    UASSERT_OBJ(callsite.inEmpty(), &callsite, "Call site should have at most one incoming edge");
    புதிய V3GraphEdge{இது, &caller, &callsite, 1, உண்மை};  // Can cut caller -> callsite
}
வெறுமை InlineCFuncsCallGraph::addEdge(InlineCFuncsCallSiteVertex& callsite,
                                    InlineCFuncsFunctionVertex& callee) {
    UASSERT_OBJ(callsite.outEmpty(), &callsite, "Call site should have at most one outgoing edge");
    புதிய V3GraphEdge{இது, &callsite, &callee, 1, பொய்};
}

//######################################################################

வகுப்பு InlineCFuncsVisitor final : பொது VNVisitor {
    // NODE STATE
    // AstCFunc::user1p() ->  InlineCFuncsFunctionVertex*, the function vertex
    // AstCCall::user1p() ->  InlineCFuncsCallSiteVertex*, the call site vertex
    // AstVar::user2p()   ->  AstVar*, the cloned inlined local variable
    மாறிலி VNUser1InUse m_user1InUse;

    // STATE
    InlineCFuncsCallGraph m_graph;  // The call graph
    VDouble0 m_statCallsInlined;  // Number of calls inlined
    VDouble0 m_statFuncsInlined;  // Number of functions inlined at least once
    VDouble0 m_statFuncsRemoved;  // Number of fully-inlined functions removed
    // Size threshold: always inline if size <= this
    மாறிலி size_t m_sizeThreshold = v3Global.opt.inlineCFuncs();
    // Product threshold: inline if size * calls <= this
    மாறிலி size_t m_prodThreshold = v3Global.opt.inlineCFuncsProduct();
    // Maximum size of caller to consider inlining into
    மாறிலி size_t m_maxSizeCFunc = []() -> size_t {
        முழு maxCFunc = v3Global.opt.outputSplitCFuncs();
        முழு maxFile = v3Global.opt.outputSplit();
        எனில் (maxCFunc <= 0) maxCFunc = std::numeric_limits<முழு>::max();
        எனில் (maxFile <= 0) maxFile = std::numeric_limits<முழு>::max();
        திரும்பு std::min(maxCFunc, maxFile);
    }();
    மாறிலி size_t m_maxSizeTrace = []() -> size_t {
        முழு maxTrace = v3Global.opt.outputSplitCTrace();
        முழு maxFile = v3Global.opt.outputSplit();
        எனில் (maxTrace <= 0) maxTrace = std::numeric_limits<முழு>::max();
        எனில் (maxFile <= 0) maxFile = std::numeric_limits<முழு>::max();
        திரும்பு std::min(maxTrace, maxFile);
    }();
    InlineCFuncsFunctionVertex* m_cfuncVtxp = nullptr;  // Vertex of currently iterated function
    தருக்க m_inExecGraph = பொய்;  // True while inside an AstExecGraph subtree

    // METHODS
    InlineCFuncsFunctionVertex* getInlineCFuncsFunctionVertexp(AstCFunc* cfuncp) {
        எனில் (!cfuncp->user1p()) cfuncp->user1p(புதிய InlineCFuncsFunctionVertex{m_graph, cfuncp});
        திரும்பு cfuncp->user1u().to<InlineCFuncsFunctionVertex*>();
    }

    InlineCFuncsCallSiteVertex* getInlineCFuncsCallSiteVertexp(AstCCall* callp) {
        எனில் (!callp->user1p()) callp->user1p(புதிய InlineCFuncsCallSiteVertex{m_graph, callp});
        திரும்பு callp->user1u().to<InlineCFuncsCallSiteVertex*>();
    }

    AstCLocalScope* inlineCall(AstCFunc* மாறிலி callerp,  //
                               AstCCall* மாறிலி callp,  //
                               AstCFunc* மாறிலி calleep,  //
                               மாறிலி size_t seq_இலக்கம்) {
        UINFO(6, "Inlining CFunc " << calleep->name() << " into " << callerp->name()
                                   << " at call site " << callp);

        Ast_முனை_Stmt* மாறிலி callSitep = VN_AS(callp->backp(), Stmt_கோவை);
        ++m_statCallsInlined;

        // Callee might be empty, just delete the call
        எனில் (!calleep->stmtsp()) {
            VL_DO_DANGLING(தள்ளு_Deletep(callSitep->unlinkFrBack()), callSitep);
            திரும்பு nullptr;
        }

        // Replace call site with a local scope
        File_வரி* மாறிலி flp = callSitep->fileline();
        AstCLocalScope* மாறிலி lscopep = புதிய AstCLocalScope{flp, nullptr};
        callSitep->replaceWith(lscopep);
        VL_DO_DANGLING(தள்ளு_Deletep(callSitep), callSitep);
        lscopep->addStmtsp(புதிய AstComment{flp, "Inlined CFunc: " + calleep->name()});

        // Although it's in a local scope, we still make names of cloned locals unique
        மாறிலி std::string varPrefix
            = "__Vinline_" + std::to_string(seq_இலக்கம்) + "_" + calleep->name() + "_";

        // AstVar::user2p()  ->  AstVar*, the cloned inlined local variable
        மாறிலி VNUser2InUse user2InUse;

        // Clone local variables, add them to the local scope
        க்கு (AstVar* varp = calleep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            AstVar* மாறிலி newVarp = varp->clone_மரம்(பொய்);
            newVarp->name(varPrefix + varp->name());
            lscopep->addStmtsp(newVarp);
            varp->user2p(newVarp);
        }

        // Clone the function body
        Ast_முனை* மாறிலி bodyp = calleep->stmtsp()->clone_மரம்(உண்மை);
        lscopep->addStmtsp(bodyp);

        // Retarget local variable references to the cloned locals
        // Rename locals defined in the body, TODO: there should be none after #6280
        // Reset vertex pointers on calls
        bodyp->foreachAndNext([&](Ast_முனை* nodep) {
            எனில் (AstVarRef* மாறிலி refp = VN_CAST(nodep, VarRef)) {
                எனில் (AstVar* மாறிலி varp = VN_AS(refp->varp()->user2p(), Var)) {
                    // Keep dtype; V3Expand may have widened the reference beyond the variable
                    Ast_முனை_DType* மாறிலி dtypep = refp->dtypep();
                    refp->varp(varp);
                    refp->dtypep(dtypep);
                }
            } இல்லையெனில் எனில் (AstVar* மாறிலி varp = VN_CAST(nodep, Var)) {
                varp->name(varPrefix + varp->name());
            } இல்லையெனில் எனில் (AstCCall* மாறிலி callp = VN_CAST(nodep, CCall)) {
                callp->user1p(nullptr);
            }
        });

        // Return the local scope
        திரும்பு lscopep;
    }

    வெறுமை doInlining() {
        // Need to gather vertices as we are changing the graph
        std::vector<InlineCFuncsFunctionVertex*> m_fVtxps;
        க்கு (V3GraphVertex& vtx : m_graph.vertices()) {
            எனில் (InlineCFuncsFunctionVertex* மாறிலி fVtxp = vtx.cast<InlineCFuncsFunctionVertex>()) {
                m_fVtxps.emplace_back(fVtxp);
            }
        }

        // Iterate functions leaf to root
        க்கு (InlineCFuncsFunctionVertex* மாறிலி calleeVtxp : vlstd::reverse_view(m_fVtxps)) {
            // Should we inline this function?
            எனில் (calleeVtxp->noInline()) தொடர்;  // Told not to

            // Check size heuristics
            மாறிலி தருக்க doIt = [&]() {
                // Inline if small enough
                எனில் (calleeVtxp->size() <= m_sizeThreshold) திரும்பு உண்மை;
                // Inline if not too much bloat
                மாறிலி size_t nCalls = calleeVtxp->inEdges().size();
                எனில் (nCalls * calleeVtxp->size() <= m_prodThreshold) திரும்பு உண்மை;
                // Otherwise don't inline
                திரும்பு பொய்;
            }();
            எனில் (!doIt) தொடர்;

            // Ok, attempt to inline call sites
            size_t nInlined = 0;
            க்கு (மாறிலி V3GraphEdge* மாறிலி edgep : calleeVtxp->inEdges().unlinkable()) {
                InlineCFuncsCallSiteVertex* மாறிலி callVtxp
                    = edgep->fromp()->as<InlineCFuncsCallSiteVertex>();

                AstCFunc* மாறிலி calleep = calleeVtxp->cfuncp();
                AstCCall* மாறிலி callp = callVtxp->callp();
                UINFO(6, "Consider inlining " << calleep->name() << " at call site " << callp);
                // Should we inline this call site?
                எனில் (callVtxp->noInline()) தொடர்;  // Told not to
                எனில் (callVtxp->inEmpty()) தொடர்;  // Don't know where it's called from

                // Pick up the caller
                UASSERT_OBJ(callVtxp->inSize1(), callVtxp->callp(),
                            "Expected exactly one input edge for call site");
                InlineCFuncsFunctionVertex* மாறிலி callerVtxp
                    = callVtxp->inEdges().frontp()->fromp()->as<InlineCFuncsFunctionVertex>();
                AstCFunc* மாறிலி callerp = callerVtxp->cfuncp();

                // Don't make a function bigger than the limit
                மாறிலி size_t limit = callerp->isTrace() ? m_maxSizeTrace : m_maxSizeCFunc;
                எனில் (callerVtxp->size() + calleeVtxp->size() > limit) தொடர்;

                // Can't do it if it's in a different scope, self pointers differ
                எனில் (callerp->scopep() != calleep->scopep()) தொடர்;

                // Inline it
                எனில் (!nInlined) ++m_statFuncsInlined;
                Ast_முனை* மாறிலி inlinedp = inlineCall(callerp, callp, calleep, nInlined++);

                // Need to adjust the graph:
                // 1. Delete inlined call site
                VL_DO_DANGLING(callVtxp->unlinkDelete(&m_graph), callVtxp);
                // 2. Add new inlined call sites - also increments size of caller
                VL_RESTORER(m_cfuncVtxp);
                m_cfuncVtxp = callerVtxp;
                எனில் (inlinedp) iterateChildrenConst(inlinedp);
            }
        }
    }

    வெறுமை removeUnusedFuncs() {
        // Iterate root to leaves
        க்கு (V3GraphVertex* மாறிலி vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsFunctionVertex* மாறிலி fVtxp = vtxp->cast<InlineCFuncsFunctionVertex>();
            எனில் (!fVtxp) தொடர்;
            // Keep if still called
            எனில் (!fVtxp->inEmpty()) தொடர்;
            // Keep for other reasons
            எனில் (fVtxp->keep()) தொடர்;

            AstCFunc* மாறிலி funcp = fVtxp->cfuncp();
            UINFO(6, "Removing unused CFunc " << funcp);
            ++m_statFuncsRemoved;

            // Unlink all call sites
            க்கு (மாறிலி V3GraphEdge* மாறிலி edgep : vtxp->outEdges().unlinkable()) {
                edgep->உச்சி()->unlinkEdges(&m_graph);
            }
            // Delete function vertex
            vtxp->unlinkDelete(&m_graph);
            // Delete the function
            VL_DO_DANGLING(தள்ளு_Deletep(funcp->unlinkFrBack()), funcp);
        }

        // Delete inlined/deleted call site vertices (for debugging only)
        க்கு (V3GraphVertex* மாறிலி vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsCallSiteVertex* மாறிலி cVtxp = vtxp->cast<InlineCFuncsCallSiteVertex>();
            எனில் (!cVtxp) தொடர்;
            எனில் (!cVtxp->inEmpty()) தொடர்;
            எனில் (!cVtxp->outEmpty()) தொடர்;
            vtxp->unlinkDelete(&m_graph);
        }
    }

    // VISITORS
    வெறுமை visit(AstCFunc* nodep) override {
        // Create the function vertex
        InlineCFuncsFunctionVertex* மாறிலி vtxp = getInlineCFuncsFunctionVertexp(nodep);

        // Check if the function itself is not inlineable
        எனில் (nodep->rtnTypeVoid() != "void") vtxp->setNoInline("Not void");
        எனில் (nodep->dpiImportPrototype()) vtxp->setNoInline("DPI import prototype");
        எனில் (nodep->recursive()) vtxp->setNoInline("Recursive");
        எனில் (nodep->argsp()) vtxp->setNoInline("Has arguments");
        எனில் (nodep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Check if the function should not be removed
        எனில் (nodep->entryPoint()) vtxp->setKeep("Entry point");
        எனில் (nodep->dpiImportPrototype()) vtxp->setKeep("DPI import prototype");
        எனில் (nodep->dpiExportDispatcher()) vtxp->setKeep("DPI export implementation");
        எனில் (nodep->isVirtual()) vtxp->setKeep("Virtual method");

        // Iterate children
        VL_RESTORER(m_cfuncVtxp);
        m_cfuncVtxp = vtxp;
        iterateChildrenConst(nodep);
    }

    // Inlineable calls
    வெறுமை visit(AstCCall* nodep) override {
        எனில் (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        AstCFunc* மாறிலி calleep = nodep->funcp();

        // Create the call site vertex
        InlineCFuncsCallSiteVertex* மாறிலி vtxp = getInlineCFuncsCallSiteVertexp(nodep);

        // Check if the call site is not inlineable
        எனில் (!VN_IS(nodep->backp(), Stmt_கோவை)) vtxp->setNoInline("Not in statement position");
        எனில் (m_inExecGraph) vtxp->setNoInline("In ExecGraph");
        எனில் (calleep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Add caller/callee edges
        எனில் (m_cfuncVtxp) m_graph.addEdge(*m_cfuncVtxp, *vtxp);
        m_graph.addEdge(*vtxp, *getInlineCFuncsFunctionVertexp(calleep));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    வெறுமை visit(Ast_முனை_CCall* nodep) override {
        எனில் (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Called elsewhere");
        iterateChildrenConst(nodep);
    }

    வெறுமை visit(AstAddrOfCFunc* nodep) override {
        எனில் (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Referenced by AddressOfCFunc");
        iterateChildrenConst(nodep);
    }

    // Nodes preventing inlining
    வெறுமை visit(AstTraceDecl* nodep) override {
        // Referenced by TraceInc
        எனில் (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains TraceDecl");

        எனில் (AstCCall* மாறிலி callp = nodep->dtypeCallp()) {
            getInlineCFuncsCallSiteVertexp(callp)->setNoInline("Referenced by TraceDecl");
        }
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstExecGraph* nodep) override {
        // AstExecGraph is not cloneable, so can't inline the containing function
        எனில் (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains ExecGraph");
        // Also mark functions referenced in the dependency graph
        க்கு (மாறிலி V3GraphVertex& vtx : nodep->depGraphp()->vertices()) {
            getInlineCFuncsFunctionVertexp(vtx.as<ExecMTask>()->funcp())
                ->setKeep("MTask function");
        }
        VL_RESTORER(m_inExecGraph);
        m_inExecGraph = உண்மை;
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstCStmt* nodep) override {
        // Can reference anything in text
        எனில் (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmt");
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstCExpr* nodep) override {
        // Can reference anything in text
        எனில் (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExpr");
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstCStmtUser* nodep) override {
        // Can reference anything in text
        எனில் (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmtUser");
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstCExprUser* nodep) override {
        // Can reference anything in text
        எனில் (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExprUser");
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstCReturn* nodep) override {
        எனில் (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CReturn");
        iterateChildrenConst(nodep);
    }

    // Base node
    வெறுமை visit(Ast_முனை* nodep) override {
        எனில் (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        iterateChildrenConst(nodep);
    }

பொது:
    // CONSTRUCTORS
    explicit InlineCFuncsVisitor(AstNetlist* nodep) {
        // Phase 1: Build call graph
        iterateConst(nodep);
        // Make acyclic in case there is recursion
        m_graph.acyclic(V3GraphEdge::followAlwaysTrue);
        // Order vertices (any topological order is fine)
        m_graph.order();
        எனில் (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-graph");
        // Phase 2: Inline calls
        doInlining();
        எனில் (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-inlined");
        // Phase 3: Remove unused functions
        removeUnusedFuncs();
        எனில் (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-kept");
    }
    ~InlineCFuncsVisitor() override {
        V3Stats::addStat("Optimizations, Inline CFuncs, calls inlined", m_statCallsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions inlined", m_statFuncsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions removed", m_statFuncsRemoved);
    }
};

//######################################################################
// InlineCFuncs class functions

வெறுமை V3InlineCFuncs::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    // Don't inline when profiling per-function (it would lose granularity)
    எனில் (v3Global.opt.profCFuncs()) திரும்பு;
    { InlineCFuncsVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("inlinecfuncs", 0, dump_மரம்_Either_Level() >= 6);
}
