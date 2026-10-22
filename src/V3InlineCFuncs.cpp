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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3InlineCFuncs.h"

#अन्तर्भाव "V3AstUserAllocator.h"
#अन्तर्भाव "V3ExecGraph.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Bipartite call graph containing function and call site vertices

वग्ग InlineCFuncsFunctionVertex;
वग्ग InlineCFuncsCallSiteVertex;

वग्ग InlineCFuncsCallGraph final : सार्वजनिक V3Graph {
सार्वजनिक:
    InlineCFuncsCallGraph()
        : V3Graph{} {}
    ~InlineCFuncsCallGraph() override = स्वभाव;

    शून्य addEdge(InlineCFuncsFunctionVertex& from, InlineCFuncsCallSiteVertex& शिखरम्);
    शून्य addEdge(InlineCFuncsCallSiteVertex& from, InlineCFuncsFunctionVertex& शिखरम्);
};

वग्ग EitherVertex VL_NOT_FINAL : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(EitherVertex, V3GraphVertex)
रक्षित:
    explicit EitherVertex(InlineCFuncsCallGraph& graph)
        : V3GraphVertex{&graph} {}
};

वग्ग InlineCFuncsFunctionVertex final : सार्वजनिक EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsFunctionVertex, EitherVertex)
    AstCFunc* ध्रुव m_cfuncp;  // The function
    ध्रुव वर्ण* m_noInlineWyp = nullptr;  // First reason the function should not be inlined
    ध्रुव वर्ण* m_keepWyp = nullptr;  // Why the function should not be removed
    size_t m_size = 0;  // The size of the function

सार्वजनिक:
    InlineCFuncsFunctionVertex(InlineCFuncsCallGraph& graph, AstCFunc* cfuncp)
        : EitherVertex{graph}
        , m_cfuncp{cfuncp} {}
    ~InlineCFuncsFunctionVertex() override = स्वभाव;

    // ACCESSORS
    AstCFunc* cfuncp() ध्रुव { पटिनिवत्त m_cfuncp; }
    size_t size() ध्रुव { पटिनिवत्त m_size; }
    शून्य sizeInc(size_t मानम् = 1) { m_size += मानम्; }
    तार्किक noInline() ध्रुव { पटिनिवत्त m_noInlineWyp; }
    शून्य setNoInline(ध्रुव वर्ण* whyp) {
        सचे (!m_noInlineWyp) m_noInlineWyp = whyp;
    }
    तार्किक keep() ध्रुव { पटिनिवत्त m_keepWyp; }
    शून्य setKeep(ध्रुव वर्ण* whyp) {
        सचे (!m_keepWyp) m_keepWyp = whyp;
    }
    std::string dotColor() ध्रुव override {
        पटिनिवत्त m_noInlineWyp ? "red" : m_keepWyp ? "orange" : "black";
    }

    // debug
    File_पङ्क्तिः* fileline() ध्रुव override { पटिनिवत्त m_cfuncp->fileline(); }
    std::string dotShape() ध्रुव override { पटिनिवत्त "box"; }
    std::string name() ध्रुव override VL_MT_STABLE {
        std::string str = cvtToHex(m_cfuncp);
        str += "\n" + m_cfuncp->name();
        str += "\nsize: " + std::to_string(m_size);
        सचे (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        सचे (m_keepWyp) str += "\nKeep: "s + m_keepWyp;
        पटिनिवत्त str;
    }
};

वग्ग InlineCFuncsCallSiteVertex final : सार्वजनिक EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsCallSiteVertex, EitherVertex)
    AstCCall* ध्रुव m_callp;  // The call site
    ध्रुव वर्ण* m_noInlineWyp = nullptr;  // First reason the function should not be inlined

सार्वजनिक:
    InlineCFuncsCallSiteVertex(InlineCFuncsCallGraph& graph, AstCCall* callp)
        : EitherVertex{graph}
        , m_callp{callp} {}
    ~InlineCFuncsCallSiteVertex() override = स्वभाव;

    // ACCESSORS
    AstCCall* callp() ध्रुव { पटिनिवत्त m_callp; }
    तार्किक noInline() ध्रुव { पटिनिवत्त m_noInlineWyp; }
    शून्य setNoInline(ध्रुव वर्ण* whyp) {
        सचे (!m_noInlineWyp) m_noInlineWyp = whyp;
    }

    // debug
    File_पङ्क्तिः* fileline() ध्रुव override { पटिनिवत्त m_callp->fileline(); }
    std::string dotColor() ध्रुव override { पटिनिवत्त m_noInlineWyp ? "red" : "black"; }
    std::string dotShape() ध्रुव override { पटिनिवत्त "ellipse"; }
    std::string name() ध्रुव override VL_MT_STABLE {
        std::string str = cvtToHex(m_callp);
        सचे (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        पटिनिवत्त str;
    }
};

शून्य InlineCFuncsCallGraph::addEdge(InlineCFuncsFunctionVertex& caller,
                                    InlineCFuncsCallSiteVertex& callsite) {
    UASSERT_OBJ(callsite.inEmpty(), &callsite, "Call site should have at most one incoming edge");
    नूतन V3GraphEdge{एतत्, &caller, &callsite, 1, सच्च};  // Can cut caller -> callsite
}
शून्य InlineCFuncsCallGraph::addEdge(InlineCFuncsCallSiteVertex& callsite,
                                    InlineCFuncsFunctionVertex& callee) {
    UASSERT_OBJ(callsite.outEmpty(), &callsite, "Call site should have at most one outgoing edge");
    नूतन V3GraphEdge{एतत्, &callsite, &callee, 1, मिच्छा};
}

//######################################################################

वग्ग InlineCFuncsVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstCFunc::user1p() ->  InlineCFuncsFunctionVertex*, the function vertex
    // AstCCall::user1p() ->  InlineCFuncsCallSiteVertex*, the call site vertex
    // AstVar::user2p()   ->  AstVar*, the cloned inlined local variable
    ध्रुव VNUser1InUse m_user1InUse;

    // STATE
    InlineCFuncsCallGraph m_graph;  // The call graph
    VDouble0 m_statCallsInlined;  // Number of calls inlined
    VDouble0 m_statFuncsInlined;  // Number of functions inlined at least once
    VDouble0 m_statFuncsRemoved;  // Number of fully-inlined functions removed
    // Size threshold: always inline if size <= this
    ध्रुव size_t m_sizeThreshold = v3Global.opt.inlineCFuncs();
    // Product threshold: inline if size * calls <= this
    ध्रुव size_t m_prodThreshold = v3Global.opt.inlineCFuncsProduct();
    // Maximum size of caller to consider inlining into
    ध्रुव size_t m_maxSizeCFunc = []() -> size_t {
        पूर्णाङ्क maxCFunc = v3Global.opt.outputSplitCFuncs();
        पूर्णाङ्क maxFile = v3Global.opt.outputSplit();
        सचे (maxCFunc <= 0) maxCFunc = std::numeric_limits<पूर्णाङ्क>::max();
        सचे (maxFile <= 0) maxFile = std::numeric_limits<पूर्णाङ्क>::max();
        पटिनिवत्त std::min(maxCFunc, maxFile);
    }();
    ध्रुव size_t m_maxSizeTrace = []() -> size_t {
        पूर्णाङ्क maxTrace = v3Global.opt.outputSplitCTrace();
        पूर्णाङ्क maxFile = v3Global.opt.outputSplit();
        सचे (maxTrace <= 0) maxTrace = std::numeric_limits<पूर्णाङ्क>::max();
        सचे (maxFile <= 0) maxFile = std::numeric_limits<पूर्णाङ्क>::max();
        पटिनिवत्त std::min(maxTrace, maxFile);
    }();
    InlineCFuncsFunctionVertex* m_cfuncVtxp = nullptr;  // Vertex of currently iterated function
    तार्किक m_inExecGraph = मिच्छा;  // True while inside an AstExecGraph subtree

    // METHODS
    InlineCFuncsFunctionVertex* getInlineCFuncsFunctionVertexp(AstCFunc* cfuncp) {
        सचे (!cfuncp->user1p()) cfuncp->user1p(नूतन InlineCFuncsFunctionVertex{m_graph, cfuncp});
        पटिनिवत्त cfuncp->user1u().to<InlineCFuncsFunctionVertex*>();
    }

    InlineCFuncsCallSiteVertex* getInlineCFuncsCallSiteVertexp(AstCCall* callp) {
        सचे (!callp->user1p()) callp->user1p(नूतन InlineCFuncsCallSiteVertex{m_graph, callp});
        पटिनिवत्त callp->user1u().to<InlineCFuncsCallSiteVertex*>();
    }

    AstCLocalScope* inlineCall(AstCFunc* ध्रुव callerp,  //
                               AstCCall* ध्रुव callp,  //
                               AstCFunc* ध्रुव calleep,  //
                               ध्रुव size_t seq_अङ्कः) {
        UINFO(6, "Inlining CFunc " << calleep->name() << " into " << callerp->name()
                                   << " at call site " << callp);

        Ast_ग्रन्थिः_Stmt* ध्रुव callSitep = VN_AS(callp->backp(), Stmt_व्यञ्जकम्);
        ++m_statCallsInlined;

        // Callee might be empty, just delete the call
        सचे (!calleep->stmtsp()) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(callSitep->unlinkFrBack()), callSitep);
            पटिनिवत्त nullptr;
        }

        // Replace call site with a local scope
        File_पङ्क्तिः* ध्रुव flp = callSitep->fileline();
        AstCLocalScope* ध्रुव lscopep = नूतन AstCLocalScope{flp, nullptr};
        callSitep->replaceWith(lscopep);
        VL_DO_DANGLING(प्रक्षिप_Deletep(callSitep), callSitep);
        lscopep->addStmtsp(नूतन AstComment{flp, "Inlined CFunc: " + calleep->name()});

        // Although it's in a local scope, we still make names of cloned locals unique
        ध्रुव std::string varPrefix
            = "__Vinline_" + std::to_string(seq_अङ्कः) + "_" + calleep->name() + "_";

        // AstVar::user2p()  ->  AstVar*, the cloned inlined local variable
        ध्रुव VNUser2InUse user2InUse;

        // Clone local variables, add them to the local scope
        अत्थाय (AstVar* varp = calleep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            AstVar* ध्रुव newVarp = varp->clone_वृक्षः(मिच्छा);
            newVarp->name(varPrefix + varp->name());
            lscopep->addStmtsp(newVarp);
            varp->user2p(newVarp);
        }

        // Clone the function body
        Ast_ग्रन्थिः* ध्रुव bodyp = calleep->stmtsp()->clone_वृक्षः(सच्च);
        lscopep->addStmtsp(bodyp);

        // Retarget local variable references to the cloned locals
        // Rename locals defined in the body, TODO: there should be none after #6280
        // Reset vertex pointers on calls
        bodyp->foreachAndNext([&](Ast_ग्रन्थिः* nodep) {
            सचे (AstVarRef* ध्रुव refp = VN_CAST(nodep, VarRef)) {
                सचे (AstVar* ध्रुव varp = VN_AS(refp->varp()->user2p(), Var)) {
                    // Keep dtype; V3Expand may have widened the reference beyond the variable
                    Ast_ग्रन्थिः_DType* ध्रुव dtypep = refp->dtypep();
                    refp->varp(varp);
                    refp->dtypep(dtypep);
                }
            } अञ्ञथा सचे (AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                varp->name(varPrefix + varp->name());
            } अञ्ञथा सचे (AstCCall* ध्रुव callp = VN_CAST(nodep, CCall)) {
                callp->user1p(nullptr);
            }
        });

        // Return the local scope
        पटिनिवत्त lscopep;
    }

    शून्य doInlining() {
        // Need to gather vertices as we are changing the graph
        std::vector<InlineCFuncsFunctionVertex*> m_fVtxps;
        अत्थाय (V3GraphVertex& vtx : m_graph.vertices()) {
            सचे (InlineCFuncsFunctionVertex* ध्रुव fVtxp = vtx.cast<InlineCFuncsFunctionVertex>()) {
                m_fVtxps.emplace_back(fVtxp);
            }
        }

        // Iterate functions leaf to root
        अत्थाय (InlineCFuncsFunctionVertex* ध्रुव calleeVtxp : vlstd::reverse_view(m_fVtxps)) {
            // Should we inline this function?
            सचे (calleeVtxp->noInline()) अनुवर्त;  // Told not to

            // Check size heuristics
            ध्रुव तार्किक doIt = [&]() {
                // Inline if small enough
                सचे (calleeVtxp->size() <= m_sizeThreshold) पटिनिवत्त सच्च;
                // Inline if not too much bloat
                ध्रुव size_t nCalls = calleeVtxp->inEdges().size();
                सचे (nCalls * calleeVtxp->size() <= m_prodThreshold) पटिनिवत्त सच्च;
                // Otherwise don't inline
                पटिनिवत्त मिच्छा;
            }();
            सचे (!doIt) अनुवर्त;

            // Ok, attempt to inline call sites
            size_t nInlined = 0;
            अत्थाय (ध्रुव V3GraphEdge* ध्रुव edgep : calleeVtxp->inEdges().unlinkable()) {
                InlineCFuncsCallSiteVertex* ध्रुव callVtxp
                    = edgep->fromp()->as<InlineCFuncsCallSiteVertex>();

                AstCFunc* ध्रुव calleep = calleeVtxp->cfuncp();
                AstCCall* ध्रुव callp = callVtxp->callp();
                UINFO(6, "Consider inlining " << calleep->name() << " at call site " << callp);
                // Should we inline this call site?
                सचे (callVtxp->noInline()) अनुवर्त;  // Told not to
                सचे (callVtxp->inEmpty()) अनुवर्त;  // Don't know where it's called from

                // Pick up the caller
                UASSERT_OBJ(callVtxp->inSize1(), callVtxp->callp(),
                            "Expected exactly one input edge for call site");
                InlineCFuncsFunctionVertex* ध्रुव callerVtxp
                    = callVtxp->inEdges().frontp()->fromp()->as<InlineCFuncsFunctionVertex>();
                AstCFunc* ध्रुव callerp = callerVtxp->cfuncp();

                // Don't make a function bigger than the limit
                ध्रुव size_t limit = callerp->isTrace() ? m_maxSizeTrace : m_maxSizeCFunc;
                सचे (callerVtxp->size() + calleeVtxp->size() > limit) अनुवर्त;

                // Can't do it if it's in a different scope, self pointers differ
                सचे (callerp->scopep() != calleep->scopep()) अनुवर्त;

                // Inline it
                सचे (!nInlined) ++m_statFuncsInlined;
                Ast_ग्रन्थिः* ध्रुव inlinedp = inlineCall(callerp, callp, calleep, nInlined++);

                // Need to adjust the graph:
                // 1. Delete inlined call site
                VL_DO_DANGLING(callVtxp->unlinkDelete(&m_graph), callVtxp);
                // 2. Add new inlined call sites - also increments size of caller
                VL_RESTORER(m_cfuncVtxp);
                m_cfuncVtxp = callerVtxp;
                सचे (inlinedp) iterateChildrenConst(inlinedp);
            }
        }
    }

    शून्य removeUnusedFuncs() {
        // Iterate root to leaves
        अत्थाय (V3GraphVertex* ध्रुव vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsFunctionVertex* ध्रुव fVtxp = vtxp->cast<InlineCFuncsFunctionVertex>();
            सचे (!fVtxp) अनुवर्त;
            // Keep if still called
            सचे (!fVtxp->inEmpty()) अनुवर्त;
            // Keep for other reasons
            सचे (fVtxp->keep()) अनुवर्त;

            AstCFunc* ध्रुव funcp = fVtxp->cfuncp();
            UINFO(6, "Removing unused CFunc " << funcp);
            ++m_statFuncsRemoved;

            // Unlink all call sites
            अत्थाय (ध्रुव V3GraphEdge* ध्रुव edgep : vtxp->outEdges().unlinkable()) {
                edgep->शिखरम्()->unlinkEdges(&m_graph);
            }
            // Delete function vertex
            vtxp->unlinkDelete(&m_graph);
            // Delete the function
            VL_DO_DANGLING(प्रक्षिप_Deletep(funcp->unlinkFrBack()), funcp);
        }

        // Delete inlined/deleted call site vertices (for debugging only)
        अत्थाय (V3GraphVertex* ध्रुव vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsCallSiteVertex* ध्रुव cVtxp = vtxp->cast<InlineCFuncsCallSiteVertex>();
            सचे (!cVtxp) अनुवर्त;
            सचे (!cVtxp->inEmpty()) अनुवर्त;
            सचे (!cVtxp->outEmpty()) अनुवर्त;
            vtxp->unlinkDelete(&m_graph);
        }
    }

    // VISITORS
    शून्य visit(AstCFunc* nodep) override {
        // Create the function vertex
        InlineCFuncsFunctionVertex* ध्रुव vtxp = getInlineCFuncsFunctionVertexp(nodep);

        // Check if the function itself is not inlineable
        सचे (nodep->rtnTypeVoid() != "void") vtxp->setNoInline("Not void");
        सचे (nodep->dpiImportPrototype()) vtxp->setNoInline("DPI import prototype");
        सचे (nodep->recursive()) vtxp->setNoInline("Recursive");
        सचे (nodep->argsp()) vtxp->setNoInline("Has arguments");
        सचे (nodep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Check if the function should not be removed
        सचे (nodep->entryPoint()) vtxp->setKeep("Entry point");
        सचे (nodep->dpiImportPrototype()) vtxp->setKeep("DPI import prototype");
        सचे (nodep->dpiExportDispatcher()) vtxp->setKeep("DPI export implementation");
        सचे (nodep->isVirtual()) vtxp->setKeep("Virtual method");

        // Iterate children
        VL_RESTORER(m_cfuncVtxp);
        m_cfuncVtxp = vtxp;
        iterateChildrenConst(nodep);
    }

    // Inlineable calls
    शून्य visit(AstCCall* nodep) override {
        सचे (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        AstCFunc* ध्रुव calleep = nodep->funcp();

        // Create the call site vertex
        InlineCFuncsCallSiteVertex* ध्रुव vtxp = getInlineCFuncsCallSiteVertexp(nodep);

        // Check if the call site is not inlineable
        सचे (!VN_IS(nodep->backp(), Stmt_व्यञ्जकम्)) vtxp->setNoInline("Not in statement position");
        सचे (m_inExecGraph) vtxp->setNoInline("In ExecGraph");
        सचे (calleep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Add caller/callee edges
        सचे (m_cfuncVtxp) m_graph.addEdge(*m_cfuncVtxp, *vtxp);
        m_graph.addEdge(*vtxp, *getInlineCFuncsFunctionVertexp(calleep));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    शून्य visit(Ast_ग्रन्थिः_CCall* nodep) override {
        सचे (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Called elsewhere");
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstAddrOfCFunc* nodep) override {
        सचे (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Referenced by AddressOfCFunc");
        iterateChildrenConst(nodep);
    }

    // Nodes preventing inlining
    शून्य visit(AstTraceDecl* nodep) override {
        // Referenced by TraceInc
        सचे (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains TraceDecl");

        सचे (AstCCall* ध्रुव callp = nodep->dtypeCallp()) {
            getInlineCFuncsCallSiteVertexp(callp)->setNoInline("Referenced by TraceDecl");
        }
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstExecGraph* nodep) override {
        // AstExecGraph is not cloneable, so can't inline the containing function
        सचे (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains ExecGraph");
        // Also mark functions referenced in the dependency graph
        अत्थाय (ध्रुव V3GraphVertex& vtx : nodep->depGraphp()->vertices()) {
            getInlineCFuncsFunctionVertexp(vtx.as<ExecMTask>()->funcp())
                ->setKeep("MTask function");
        }
        VL_RESTORER(m_inExecGraph);
        m_inExecGraph = सच्च;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCStmt* nodep) override {
        // Can reference anything in text
        सचे (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmt");
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCExpr* nodep) override {
        // Can reference anything in text
        सचे (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExpr");
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCStmtUser* nodep) override {
        // Can reference anything in text
        सचे (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmtUser");
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCExprUser* nodep) override {
        // Can reference anything in text
        सचे (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExprUser");
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCReturn* nodep) override {
        सचे (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CReturn");
        iterateChildrenConst(nodep);
    }

    // Base node
    शून्य visit(Ast_ग्रन्थिः* nodep) override {
        सचे (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        iterateChildrenConst(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit InlineCFuncsVisitor(AstNetlist* nodep) {
        // Phase 1: Build call graph
        iterateConst(nodep);
        // Make acyclic in case there is recursion
        m_graph.acyclic(V3GraphEdge::followAlwaysTrue);
        // Order vertices (any topological order is fine)
        m_graph.order();
        सचे (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-graph");
        // Phase 2: Inline calls
        doInlining();
        सचे (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-inlined");
        // Phase 3: Remove unused functions
        removeUnusedFuncs();
        सचे (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-kept");
    }
    ~InlineCFuncsVisitor() override {
        V3Stats::addStat("Optimizations, Inline CFuncs, calls inlined", m_statCallsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions inlined", m_statFuncsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions removed", m_statFuncsRemoved);
    }
};

//######################################################################
// InlineCFuncs class functions

शून्य V3InlineCFuncs::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    // Don't inline when profiling per-function (it would lose granularity)
    सचे (v3Global.opt.profCFuncs()) पटिनिवत्त;
    { InlineCFuncsVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("inlinecfuncs", 0, dump_वृक्षः_Either_Level() >= 6);
}
