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

वर्ग InlineCFuncsFunctionVertex;
वर्ग InlineCFuncsCallSiteVertex;

वर्ग InlineCFuncsCallGraph final : सार्वजनिक V3Graph {
सार्वजनिक:
    InlineCFuncsCallGraph()
        : V3Graph{} {}
    ~InlineCFuncsCallGraph() override = स्वभाव;

    शून्य addEdge(InlineCFuncsFunctionVertex& from, InlineCFuncsCallSiteVertex& शीर्ष);
    शून्य addEdge(InlineCFuncsCallSiteVertex& from, InlineCFuncsFunctionVertex& शीर्ष);
};

वर्ग EitherVertex VL_NOT_FINAL : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(EitherVertex, V3GraphVertex)
रक्षित:
    explicit EitherVertex(InlineCFuncsCallGraph& graph)
        : V3GraphVertex{&graph} {}
};

वर्ग InlineCFuncsFunctionVertex final : सार्वजनिक EitherVertex {
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
    AstCFunc* cfuncp() ध्रुव { प्रत्यागम m_cfuncp; }
    size_t size() ध्रुव { प्रत्यागम m_size; }
    शून्य sizeInc(size_t मूल्य = 1) { m_size += मूल्य; }
    तार्किक noInline() ध्रुव { प्रत्यागम m_noInlineWyp; }
    शून्य setNoInline(ध्रुव वर्ण* whyp) {
        यदि (!m_noInlineWyp) m_noInlineWyp = whyp;
    }
    तार्किक keep() ध्रुव { प्रत्यागम m_keepWyp; }
    शून्य setKeep(ध्रुव वर्ण* whyp) {
        यदि (!m_keepWyp) m_keepWyp = whyp;
    }
    std::string dotColor() ध्रुव override {
        प्रत्यागम m_noInlineWyp ? "red" : m_keepWyp ? "orange" : "black";
    }

    // debug
    File_पंक्ति* fileline() ध्रुव override { प्रत्यागम m_cfuncp->fileline(); }
    std::string dotShape() ध्रुव override { प्रत्यागम "box"; }
    std::string name() ध्रुव override VL_MT_STABLE {
        std::string str = cvtToHex(m_cfuncp);
        str += "\n" + m_cfuncp->name();
        str += "\nsize: " + std::to_string(m_size);
        यदि (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        यदि (m_keepWyp) str += "\nKeep: "s + m_keepWyp;
        प्रत्यागम str;
    }
};

वर्ग InlineCFuncsCallSiteVertex final : सार्वजनिक EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsCallSiteVertex, EitherVertex)
    AstCCall* ध्रुव m_callp;  // The call site
    ध्रुव वर्ण* m_noInlineWyp = nullptr;  // First reason the function should not be inlined

सार्वजनिक:
    InlineCFuncsCallSiteVertex(InlineCFuncsCallGraph& graph, AstCCall* callp)
        : EitherVertex{graph}
        , m_callp{callp} {}
    ~InlineCFuncsCallSiteVertex() override = स्वभाव;

    // ACCESSORS
    AstCCall* callp() ध्रुव { प्रत्यागम m_callp; }
    तार्किक noInline() ध्रुव { प्रत्यागम m_noInlineWyp; }
    शून्य setNoInline(ध्रुव वर्ण* whyp) {
        यदि (!m_noInlineWyp) m_noInlineWyp = whyp;
    }

    // debug
    File_पंक्ति* fileline() ध्रुव override { प्रत्यागम m_callp->fileline(); }
    std::string dotColor() ध्रुव override { प्रत्यागम m_noInlineWyp ? "red" : "black"; }
    std::string dotShape() ध्रुव override { प्रत्यागम "ellipse"; }
    std::string name() ध्रुव override VL_MT_STABLE {
        std::string str = cvtToHex(m_callp);
        यदि (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        प्रत्यागम str;
    }
};

शून्य InlineCFuncsCallGraph::addEdge(InlineCFuncsFunctionVertex& caller,
                                    InlineCFuncsCallSiteVertex& callsite) {
    UASSERT_OBJ(callsite.inEmpty(), &callsite, "Call site should have at most one incoming edge");
    नूतन V3GraphEdge{एतत्, &caller, &callsite, 1, सत्य};  // Can cut caller -> callsite
}
शून्य InlineCFuncsCallGraph::addEdge(InlineCFuncsCallSiteVertex& callsite,
                                    InlineCFuncsFunctionVertex& callee) {
    UASSERT_OBJ(callsite.outEmpty(), &callsite, "Call site should have at most one outgoing edge");
    नूतन V3GraphEdge{एतत्, &callsite, &callee, 1, असत्य};
}

//######################################################################

वर्ग InlineCFuncsVisitor final : सार्वजनिक VNVisitor {
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
        यदि (maxCFunc <= 0) maxCFunc = std::numeric_limits<पूर्णाङ्क>::max();
        यदि (maxFile <= 0) maxFile = std::numeric_limits<पूर्णाङ्क>::max();
        प्रत्यागम std::min(maxCFunc, maxFile);
    }();
    ध्रुव size_t m_maxSizeTrace = []() -> size_t {
        पूर्णाङ्क maxTrace = v3Global.opt.outputSplitCTrace();
        पूर्णाङ्क maxFile = v3Global.opt.outputSplit();
        यदि (maxTrace <= 0) maxTrace = std::numeric_limits<पूर्णाङ्क>::max();
        यदि (maxFile <= 0) maxFile = std::numeric_limits<पूर्णाङ्क>::max();
        प्रत्यागम std::min(maxTrace, maxFile);
    }();
    InlineCFuncsFunctionVertex* m_cfuncVtxp = nullptr;  // Vertex of currently iterated function
    तार्किक m_inExecGraph = असत्य;  // True while inside an AstExecGraph subtree

    // METHODS
    InlineCFuncsFunctionVertex* getInlineCFuncsFunctionVertexp(AstCFunc* cfuncp) {
        यदि (!cfuncp->user1p()) cfuncp->user1p(नूतन InlineCFuncsFunctionVertex{m_graph, cfuncp});
        प्रत्यागम cfuncp->user1u().to<InlineCFuncsFunctionVertex*>();
    }

    InlineCFuncsCallSiteVertex* getInlineCFuncsCallSiteVertexp(AstCCall* callp) {
        यदि (!callp->user1p()) callp->user1p(नूतन InlineCFuncsCallSiteVertex{m_graph, callp});
        प्रत्यागम callp->user1u().to<InlineCFuncsCallSiteVertex*>();
    }

    AstCLocalScope* inlineCall(AstCFunc* ध्रुव callerp,  //
                               AstCCall* ध्रुव callp,  //
                               AstCFunc* ध्रुव calleep,  //
                               ध्रुव size_t seq_अंक) {
        UINFO(6, "Inlining CFunc " << calleep->name() << " into " << callerp->name()
                                   << " at call site " << callp);

        Ast_गाँठ_Stmt* ध्रुव callSitep = VN_AS(callp->backp(), Stmt_व्यंजक);
        ++m_statCallsInlined;

        // Callee might be empty, just delete the call
        यदि (!calleep->stmtsp()) {
            VL_DO_DANGLING(धकेलो_Deletep(callSitep->unlinkFrBack()), callSitep);
            प्रत्यागम nullptr;
        }

        // Replace call site with a local scope
        File_पंक्ति* ध्रुव flp = callSitep->fileline();
        AstCLocalScope* ध्रुव lscopep = नूतन AstCLocalScope{flp, nullptr};
        callSitep->replaceWith(lscopep);
        VL_DO_DANGLING(धकेलो_Deletep(callSitep), callSitep);
        lscopep->addStmtsp(नूतन AstComment{flp, "Inlined CFunc: " + calleep->name()});

        // Although it's in a local scope, we still make names of cloned locals unique
        ध्रुव std::string varPrefix
            = "__Vinline_" + std::to_string(seq_अंक) + "_" + calleep->name() + "_";

        // AstVar::user2p()  ->  AstVar*, the cloned inlined local variable
        ध्रुव VNUser2InUse user2InUse;

        // Clone local variables, add them to the local scope
        प्रति (AstVar* varp = calleep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            AstVar* ध्रुव newVarp = varp->clone_वृक्ष(असत्य);
            newVarp->name(varPrefix + varp->name());
            lscopep->addStmtsp(newVarp);
            varp->user2p(newVarp);
        }

        // Clone the function body
        Ast_गाँठ* ध्रुव bodyp = calleep->stmtsp()->clone_वृक्ष(सत्य);
        lscopep->addStmtsp(bodyp);

        // Retarget local variable references to the cloned locals
        // Rename locals defined in the body, TODO: there should be none after #6280
        // Reset vertex pointers on calls
        bodyp->foreachAndNext([&](Ast_गाँठ* nodep) {
            यदि (AstVarRef* ध्रुव refp = VN_CAST(nodep, VarRef)) {
                यदि (AstVar* ध्रुव varp = VN_AS(refp->varp()->user2p(), Var)) {
                    // Keep dtype; V3Expand may have widened the reference beyond the variable
                    Ast_गाँठ_DType* ध्रुव dtypep = refp->dtypep();
                    refp->varp(varp);
                    refp->dtypep(dtypep);
                }
            } अन्यथा यदि (AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                varp->name(varPrefix + varp->name());
            } अन्यथा यदि (AstCCall* ध्रुव callp = VN_CAST(nodep, CCall)) {
                callp->user1p(nullptr);
            }
        });

        // Return the local scope
        प्रत्यागम lscopep;
    }

    शून्य doInlining() {
        // Need to gather vertices as we are changing the graph
        std::vector<InlineCFuncsFunctionVertex*> m_fVtxps;
        प्रति (V3GraphVertex& vtx : m_graph.vertices()) {
            यदि (InlineCFuncsFunctionVertex* ध्रुव fVtxp = vtx.cast<InlineCFuncsFunctionVertex>()) {
                m_fVtxps.emplace_back(fVtxp);
            }
        }

        // Iterate functions leaf to root
        प्रति (InlineCFuncsFunctionVertex* ध्रुव calleeVtxp : vlstd::reverse_view(m_fVtxps)) {
            // Should we inline this function?
            यदि (calleeVtxp->noInline()) अनुवर्त;  // Told not to

            // Check size heuristics
            ध्रुव तार्किक doIt = [&]() {
                // Inline if small enough
                यदि (calleeVtxp->size() <= m_sizeThreshold) प्रत्यागम सत्य;
                // Inline if not too much bloat
                ध्रुव size_t nCalls = calleeVtxp->inEdges().size();
                यदि (nCalls * calleeVtxp->size() <= m_prodThreshold) प्रत्यागम सत्य;
                // Otherwise don't inline
                प्रत्यागम असत्य;
            }();
            यदि (!doIt) अनुवर्त;

            // Ok, attempt to inline call sites
            size_t nInlined = 0;
            प्रति (ध्रुव V3GraphEdge* ध्रुव edgep : calleeVtxp->inEdges().unlinkable()) {
                InlineCFuncsCallSiteVertex* ध्रुव callVtxp
                    = edgep->fromp()->as<InlineCFuncsCallSiteVertex>();

                AstCFunc* ध्रुव calleep = calleeVtxp->cfuncp();
                AstCCall* ध्रुव callp = callVtxp->callp();
                UINFO(6, "Consider inlining " << calleep->name() << " at call site " << callp);
                // Should we inline this call site?
                यदि (callVtxp->noInline()) अनुवर्त;  // Told not to
                यदि (callVtxp->inEmpty()) अनुवर्त;  // Don't know where it's called from

                // Pick up the caller
                UASSERT_OBJ(callVtxp->inSize1(), callVtxp->callp(),
                            "Expected exactly one input edge for call site");
                InlineCFuncsFunctionVertex* ध्रुव callerVtxp
                    = callVtxp->inEdges().frontp()->fromp()->as<InlineCFuncsFunctionVertex>();
                AstCFunc* ध्रुव callerp = callerVtxp->cfuncp();

                // Don't make a function bigger than the limit
                ध्रुव size_t limit = callerp->isTrace() ? m_maxSizeTrace : m_maxSizeCFunc;
                यदि (callerVtxp->size() + calleeVtxp->size() > limit) अनुवर्त;

                // Can't do it if it's in a different scope, self pointers differ
                यदि (callerp->scopep() != calleep->scopep()) अनुवर्त;

                // Inline it
                यदि (!nInlined) ++m_statFuncsInlined;
                Ast_गाँठ* ध्रुव inlinedp = inlineCall(callerp, callp, calleep, nInlined++);

                // Need to adjust the graph:
                // 1. Delete inlined call site
                VL_DO_DANGLING(callVtxp->unlinkDelete(&m_graph), callVtxp);
                // 2. Add new inlined call sites - also increments size of caller
                VL_RESTORER(m_cfuncVtxp);
                m_cfuncVtxp = callerVtxp;
                यदि (inlinedp) iterateChildrenConst(inlinedp);
            }
        }
    }

    शून्य removeUnusedFuncs() {
        // Iterate root to leaves
        प्रति (V3GraphVertex* ध्रुव vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsFunctionVertex* ध्रुव fVtxp = vtxp->cast<InlineCFuncsFunctionVertex>();
            यदि (!fVtxp) अनुवर्त;
            // Keep if still called
            यदि (!fVtxp->inEmpty()) अनुवर्त;
            // Keep for other reasons
            यदि (fVtxp->keep()) अनुवर्त;

            AstCFunc* ध्रुव funcp = fVtxp->cfuncp();
            UINFO(6, "Removing unused CFunc " << funcp);
            ++m_statFuncsRemoved;

            // Unlink all call sites
            प्रति (ध्रुव V3GraphEdge* ध्रुव edgep : vtxp->outEdges().unlinkable()) {
                edgep->शीर्ष()->unlinkEdges(&m_graph);
            }
            // Delete function vertex
            vtxp->unlinkDelete(&m_graph);
            // Delete the function
            VL_DO_DANGLING(धकेलो_Deletep(funcp->unlinkFrBack()), funcp);
        }

        // Delete inlined/deleted call site vertices (for debugging only)
        प्रति (V3GraphVertex* ध्रुव vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsCallSiteVertex* ध्रुव cVtxp = vtxp->cast<InlineCFuncsCallSiteVertex>();
            यदि (!cVtxp) अनुवर्त;
            यदि (!cVtxp->inEmpty()) अनुवर्त;
            यदि (!cVtxp->outEmpty()) अनुवर्त;
            vtxp->unlinkDelete(&m_graph);
        }
    }

    // VISITORS
    शून्य visit(AstCFunc* nodep) override {
        // Create the function vertex
        InlineCFuncsFunctionVertex* ध्रुव vtxp = getInlineCFuncsFunctionVertexp(nodep);

        // Check if the function itself is not inlineable
        यदि (nodep->rtnTypeVoid() != "void") vtxp->setNoInline("Not void");
        यदि (nodep->dpiImportPrototype()) vtxp->setNoInline("DPI import prototype");
        यदि (nodep->recursive()) vtxp->setNoInline("Recursive");
        यदि (nodep->argsp()) vtxp->setNoInline("Has arguments");
        यदि (nodep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Check if the function should not be removed
        यदि (nodep->entryPoint()) vtxp->setKeep("Entry point");
        यदि (nodep->dpiImportPrototype()) vtxp->setKeep("DPI import prototype");
        यदि (nodep->dpiExportDispatcher()) vtxp->setKeep("DPI export implementation");
        यदि (nodep->isVirtual()) vtxp->setKeep("Virtual method");

        // Iterate children
        VL_RESTORER(m_cfuncVtxp);
        m_cfuncVtxp = vtxp;
        iterateChildrenConst(nodep);
    }

    // Inlineable calls
    शून्य visit(AstCCall* nodep) override {
        यदि (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        AstCFunc* ध्रुव calleep = nodep->funcp();

        // Create the call site vertex
        InlineCFuncsCallSiteVertex* ध्रुव vtxp = getInlineCFuncsCallSiteVertexp(nodep);

        // Check if the call site is not inlineable
        यदि (!VN_IS(nodep->backp(), Stmt_व्यंजक)) vtxp->setNoInline("Not in statement position");
        यदि (m_inExecGraph) vtxp->setNoInline("In ExecGraph");
        यदि (calleep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Add caller/callee edges
        यदि (m_cfuncVtxp) m_graph.addEdge(*m_cfuncVtxp, *vtxp);
        m_graph.addEdge(*vtxp, *getInlineCFuncsFunctionVertexp(calleep));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    शून्य visit(Ast_गाँठ_CCall* nodep) override {
        यदि (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Called elsewhere");
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstAddrOfCFunc* nodep) override {
        यदि (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Referenced by AddressOfCFunc");
        iterateChildrenConst(nodep);
    }

    // Nodes preventing inlining
    शून्य visit(AstTraceDecl* nodep) override {
        // Referenced by TraceInc
        यदि (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains TraceDecl");

        यदि (AstCCall* ध्रुव callp = nodep->dtypeCallp()) {
            getInlineCFuncsCallSiteVertexp(callp)->setNoInline("Referenced by TraceDecl");
        }
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstExecGraph* nodep) override {
        // AstExecGraph is not cloneable, so can't inline the containing function
        यदि (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains ExecGraph");
        // Also mark functions referenced in the dependency graph
        प्रति (ध्रुव V3GraphVertex& vtx : nodep->depGraphp()->vertices()) {
            getInlineCFuncsFunctionVertexp(vtx.as<ExecMTask>()->funcp())
                ->setKeep("MTask function");
        }
        VL_RESTORER(m_inExecGraph);
        m_inExecGraph = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCStmt* nodep) override {
        // Can reference anything in text
        यदि (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmt");
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCExpr* nodep) override {
        // Can reference anything in text
        यदि (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExpr");
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCStmtUser* nodep) override {
        // Can reference anything in text
        यदि (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmtUser");
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCExprUser* nodep) override {
        // Can reference anything in text
        यदि (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExprUser");
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCReturn* nodep) override {
        यदि (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CReturn");
        iterateChildrenConst(nodep);
    }

    // Base node
    शून्य visit(Ast_गाँठ* nodep) override {
        यदि (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
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
        यदि (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-graph");
        // Phase 2: Inline calls
        doInlining();
        यदि (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-inlined");
        // Phase 3: Remove unused functions
        removeUnusedFuncs();
        यदि (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-kept");
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
    यदि (v3Global.opt.profCFuncs()) प्रत्यागम;
    { InlineCFuncsVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("inlinecfuncs", 0, dump_वृक्ष_Either_Level() >= 6);
}
