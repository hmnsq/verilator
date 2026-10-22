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
    ~InlineCFuncsCallGraph() override = पूर्वनिर्धारित;

    शून्य addEdge(InlineCFuncsFunctionVertex& from, InlineCFuncsCallSiteVertex& शीर्ष);
    शून्य addEdge(InlineCFuncsCallSiteVertex& from, InlineCFuncsFunctionVertex& शीर्ष);
};

वर्ग EitherVertex VL_NOT_FINAL : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(EitherVertex, V3GraphVertex)
संरक्षित:
    explicit EitherVertex(InlineCFuncsCallGraph& graph)
        : V3GraphVertex{&graph} {}
};

वर्ग InlineCFuncsFunctionVertex final : सार्वजनिक EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsFunctionVertex, EitherVertex)
    AstCFunc* स्थिरांक m_cfuncp;  // The function
    स्थिरांक अक्षर* m_noInlineWyp = nullptr;  // First reason the function should not be inlined
    स्थिरांक अक्षर* m_keepWyp = nullptr;  // Why the function should not be removed
    माप_प्रकार m_size = 0;  // The size of the function

सार्वजनिक:
    InlineCFuncsFunctionVertex(InlineCFuncsCallGraph& graph, AstCFunc* cfuncp)
        : EitherVertex{graph}
        , m_cfuncp{cfuncp} {}
    ~InlineCFuncsFunctionVertex() override = पूर्वनिर्धारित;

    // ACCESSORS
    AstCFunc* cfuncp() स्थिरांक { परत m_cfuncp; }
    माप_प्रकार size() स्थिरांक { परत m_size; }
    शून्य sizeInc(माप_प्रकार मूल्य = 1) { m_size += मूल्य; }
    तार्किक noInline() स्थिरांक { परत m_noInlineWyp; }
    शून्य setNoInline(स्थिरांक अक्षर* whyp) {
        जर (!m_noInlineWyp) m_noInlineWyp = whyp;
    }
    तार्किक keep() स्थिरांक { परत m_keepWyp; }
    शून्य setKeep(स्थिरांक अक्षर* whyp) {
        जर (!m_keepWyp) m_keepWyp = whyp;
    }
    std::string dotColor() स्थिरांक override {
        परत m_noInlineWyp ? "red" : m_keepWyp ? "orange" : "black";
    }

    // debug
    File_ओळ* fileline() स्थिरांक override { परत m_cfuncp->fileline(); }
    std::string dotShape() स्थिरांक override { परत "box"; }
    std::string name() स्थिरांक override VL_MT_STABLE {
        std::string str = cvtToHex(m_cfuncp);
        str += "\n" + m_cfuncp->name();
        str += "\nsize: " + std::to_string(m_size);
        जर (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        जर (m_keepWyp) str += "\nKeep: "s + m_keepWyp;
        परत str;
    }
};

वर्ग InlineCFuncsCallSiteVertex final : सार्वजनिक EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsCallSiteVertex, EitherVertex)
    AstCCall* स्थिरांक m_callp;  // The call site
    स्थिरांक अक्षर* m_noInlineWyp = nullptr;  // First reason the function should not be inlined

सार्वजनिक:
    InlineCFuncsCallSiteVertex(InlineCFuncsCallGraph& graph, AstCCall* callp)
        : EitherVertex{graph}
        , m_callp{callp} {}
    ~InlineCFuncsCallSiteVertex() override = पूर्वनिर्धारित;

    // ACCESSORS
    AstCCall* callp() स्थिरांक { परत m_callp; }
    तार्किक noInline() स्थिरांक { परत m_noInlineWyp; }
    शून्य setNoInline(स्थिरांक अक्षर* whyp) {
        जर (!m_noInlineWyp) m_noInlineWyp = whyp;
    }

    // debug
    File_ओळ* fileline() स्थिरांक override { परत m_callp->fileline(); }
    std::string dotColor() स्थिरांक override { परत m_noInlineWyp ? "red" : "black"; }
    std::string dotShape() स्थिरांक override { परत "ellipse"; }
    std::string name() स्थिरांक override VL_MT_STABLE {
        std::string str = cvtToHex(m_callp);
        जर (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        परत str;
    }
};

शून्य InlineCFuncsCallGraph::addEdge(InlineCFuncsFunctionVertex& caller,
                                    InlineCFuncsCallSiteVertex& callsite) {
    UASSERT_OBJ(callsite.inEmpty(), &callsite, "Call site should have at most one incoming edge");
    नवीन V3GraphEdge{हे, &caller, &callsite, 1, सत्य};  // Can cut caller -> callsite
}
शून्य InlineCFuncsCallGraph::addEdge(InlineCFuncsCallSiteVertex& callsite,
                                    InlineCFuncsFunctionVertex& callee) {
    UASSERT_OBJ(callsite.outEmpty(), &callsite, "Call site should have at most one outgoing edge");
    नवीन V3GraphEdge{हे, &callsite, &callee, 1, असत्य};
}

//######################################################################

वर्ग InlineCFuncsVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstCFunc::user1p() ->  InlineCFuncsFunctionVertex*, the function vertex
    // AstCCall::user1p() ->  InlineCFuncsCallSiteVertex*, the call site vertex
    // AstVar::user2p()   ->  AstVar*, the cloned inlined local variable
    स्थिरांक VNUser1InUse m_user1InUse;

    // STATE
    InlineCFuncsCallGraph m_graph;  // The call graph
    VDouble0 m_statCallsInlined;  // Number of calls inlined
    VDouble0 m_statFuncsInlined;  // Number of functions inlined at least once
    VDouble0 m_statFuncsRemoved;  // Number of fully-inlined functions removed
    // Size threshold: always inline if size <= this
    स्थिरांक माप_प्रकार m_sizeThreshold = v3Global.opt.inlineCFuncs();
    // Product threshold: inline if size * calls <= this
    स्थिरांक माप_प्रकार m_prodThreshold = v3Global.opt.inlineCFuncsProduct();
    // Maximum size of caller to consider inlining into
    स्थिरांक माप_प्रकार m_maxSizeCFunc = []() -> माप_प्रकार {
        पूर्णांक maxCFunc = v3Global.opt.outputSplitCFuncs();
        पूर्णांक maxFile = v3Global.opt.outputSplit();
        जर (maxCFunc <= 0) maxCFunc = std::numeric_limits<पूर्णांक>::max();
        जर (maxFile <= 0) maxFile = std::numeric_limits<पूर्णांक>::max();
        परत std::min(maxCFunc, maxFile);
    }();
    स्थिरांक माप_प्रकार m_maxSizeTrace = []() -> माप_प्रकार {
        पूर्णांक maxTrace = v3Global.opt.outputSplitCTrace();
        पूर्णांक maxFile = v3Global.opt.outputSplit();
        जर (maxTrace <= 0) maxTrace = std::numeric_limits<पूर्णांक>::max();
        जर (maxFile <= 0) maxFile = std::numeric_limits<पूर्णांक>::max();
        परत std::min(maxTrace, maxFile);
    }();
    InlineCFuncsFunctionVertex* m_cfuncVtxp = nullptr;  // Vertex of currently iterated function
    तार्किक m_inExecGraph = असत्य;  // True while inside an AstExecGraph subtree

    // METHODS
    InlineCFuncsFunctionVertex* getInlineCFuncsFunctionVertexp(AstCFunc* cfuncp) {
        जर (!cfuncp->user1p()) cfuncp->user1p(नवीन InlineCFuncsFunctionVertex{m_graph, cfuncp});
        परत cfuncp->user1u().to<InlineCFuncsFunctionVertex*>();
    }

    InlineCFuncsCallSiteVertex* getInlineCFuncsCallSiteVertexp(AstCCall* callp) {
        जर (!callp->user1p()) callp->user1p(नवीन InlineCFuncsCallSiteVertex{m_graph, callp});
        परत callp->user1u().to<InlineCFuncsCallSiteVertex*>();
    }

    AstCLocalScope* inlineCall(AstCFunc* स्थिरांक callerp,  //
                               AstCCall* स्थिरांक callp,  //
                               AstCFunc* स्थिरांक calleep,  //
                               स्थिरांक माप_प्रकार seq_अंक) {
        UINFO(6, "Inlining CFunc " << calleep->name() << " into " << callerp->name()
                                   << " at call site " << callp);

        Ast_गाठ_Stmt* स्थिरांक callSitep = VN_AS(callp->backp(), Stmt_अभिव्यक्ती);
        ++m_statCallsInlined;

        // Callee might be empty, just delete the call
        जर (!calleep->stmtsp()) {
            VL_DO_DANGLING(ढकला_Deletep(callSitep->unlinkFrBack()), callSitep);
            परत nullptr;
        }

        // Replace call site with a local scope
        File_ओळ* स्थिरांक flp = callSitep->fileline();
        AstCLocalScope* स्थिरांक lscopep = नवीन AstCLocalScope{flp, nullptr};
        callSitep->replaceWith(lscopep);
        VL_DO_DANGLING(ढकला_Deletep(callSitep), callSitep);
        lscopep->addStmtsp(नवीन AstComment{flp, "Inlined CFunc: " + calleep->name()});

        // Although it's in a local scope, we still make names of cloned locals unique
        स्थिरांक std::string varPrefix
            = "__Vinline_" + std::to_string(seq_अंक) + "_" + calleep->name() + "_";

        // AstVar::user2p()  ->  AstVar*, the cloned inlined local variable
        स्थिरांक VNUser2InUse user2InUse;

        // Clone local variables, add them to the local scope
        साठी (AstVar* varp = calleep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            AstVar* स्थिरांक newVarp = varp->clone_वृक्ष(असत्य);
            newVarp->name(varPrefix + varp->name());
            lscopep->addStmtsp(newVarp);
            varp->user2p(newVarp);
        }

        // Clone the function body
        Ast_गाठ* स्थिरांक bodyp = calleep->stmtsp()->clone_वृक्ष(सत्य);
        lscopep->addStmtsp(bodyp);

        // Retarget local variable references to the cloned locals
        // Rename locals defined in the body, TODO: there should be none after #6280
        // Reset vertex pointers on calls
        bodyp->foreachAndNext([&](Ast_गाठ* nodep) {
            जर (AstVarRef* स्थिरांक refp = VN_CAST(nodep, VarRef)) {
                जर (AstVar* स्थिरांक varp = VN_AS(refp->varp()->user2p(), Var)) {
                    // Keep dtype; V3Expand may have widened the reference beyond the variable
                    Ast_गाठ_DType* स्थिरांक dtypep = refp->dtypep();
                    refp->varp(varp);
                    refp->dtypep(dtypep);
                }
            } अन्यथा जर (AstVar* स्थिरांक varp = VN_CAST(nodep, Var)) {
                varp->name(varPrefix + varp->name());
            } अन्यथा जर (AstCCall* स्थिरांक callp = VN_CAST(nodep, CCall)) {
                callp->user1p(nullptr);
            }
        });

        // Return the local scope
        परत lscopep;
    }

    शून्य doInlining() {
        // Need to gather vertices as we are changing the graph
        std::vector<InlineCFuncsFunctionVertex*> m_fVtxps;
        साठी (V3GraphVertex& vtx : m_graph.vertices()) {
            जर (InlineCFuncsFunctionVertex* स्थिरांक fVtxp = vtx.cast<InlineCFuncsFunctionVertex>()) {
                m_fVtxps.emplace_back(fVtxp);
            }
        }

        // Iterate functions leaf to root
        साठी (InlineCFuncsFunctionVertex* स्थिरांक calleeVtxp : vlstd::reverse_view(m_fVtxps)) {
            // Should we inline this function?
            जर (calleeVtxp->noInline()) सुरूठेवा;  // Told not to

            // Check size heuristics
            स्थिरांक तार्किक doIt = [&]() {
                // Inline if small enough
                जर (calleeVtxp->size() <= m_sizeThreshold) परत सत्य;
                // Inline if not too much bloat
                स्थिरांक माप_प्रकार nCalls = calleeVtxp->inEdges().size();
                जर (nCalls * calleeVtxp->size() <= m_prodThreshold) परत सत्य;
                // Otherwise don't inline
                परत असत्य;
            }();
            जर (!doIt) सुरूठेवा;

            // Ok, attempt to inline call sites
            माप_प्रकार nInlined = 0;
            साठी (स्थिरांक V3GraphEdge* स्थिरांक edgep : calleeVtxp->inEdges().unlinkable()) {
                InlineCFuncsCallSiteVertex* स्थिरांक callVtxp
                    = edgep->fromp()->as<InlineCFuncsCallSiteVertex>();

                AstCFunc* स्थिरांक calleep = calleeVtxp->cfuncp();
                AstCCall* स्थिरांक callp = callVtxp->callp();
                UINFO(6, "Consider inlining " << calleep->name() << " at call site " << callp);
                // Should we inline this call site?
                जर (callVtxp->noInline()) सुरूठेवा;  // Told not to
                जर (callVtxp->inEmpty()) सुरूठेवा;  // Don't know where it's called from

                // Pick up the caller
                UASSERT_OBJ(callVtxp->inSize1(), callVtxp->callp(),
                            "Expected exactly one input edge for call site");
                InlineCFuncsFunctionVertex* स्थिरांक callerVtxp
                    = callVtxp->inEdges().frontp()->fromp()->as<InlineCFuncsFunctionVertex>();
                AstCFunc* स्थिरांक callerp = callerVtxp->cfuncp();

                // Don't make a function bigger than the limit
                स्थिरांक माप_प्रकार limit = callerp->isTrace() ? m_maxSizeTrace : m_maxSizeCFunc;
                जर (callerVtxp->size() + calleeVtxp->size() > limit) सुरूठेवा;

                // Can't do it if it's in a different scope, self pointers differ
                जर (callerp->scopep() != calleep->scopep()) सुरूठेवा;

                // Inline it
                जर (!nInlined) ++m_statFuncsInlined;
                Ast_गाठ* स्थिरांक inlinedp = inlineCall(callerp, callp, calleep, nInlined++);

                // Need to adjust the graph:
                // 1. Delete inlined call site
                VL_DO_DANGLING(callVtxp->unlinkDelete(&m_graph), callVtxp);
                // 2. Add new inlined call sites - also increments size of caller
                VL_RESTORER(m_cfuncVtxp);
                m_cfuncVtxp = callerVtxp;
                जर (inlinedp) iterateChildrenConst(inlinedp);
            }
        }
    }

    शून्य removeUnusedFuncs() {
        // Iterate root to leaves
        साठी (V3GraphVertex* स्थिरांक vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsFunctionVertex* स्थिरांक fVtxp = vtxp->cast<InlineCFuncsFunctionVertex>();
            जर (!fVtxp) सुरूठेवा;
            // Keep if still called
            जर (!fVtxp->inEmpty()) सुरूठेवा;
            // Keep for other reasons
            जर (fVtxp->keep()) सुरूठेवा;

            AstCFunc* स्थिरांक funcp = fVtxp->cfuncp();
            UINFO(6, "Removing unused CFunc " << funcp);
            ++m_statFuncsRemoved;

            // Unlink all call sites
            साठी (स्थिरांक V3GraphEdge* स्थिरांक edgep : vtxp->outEdges().unlinkable()) {
                edgep->शीर्ष()->unlinkEdges(&m_graph);
            }
            // Delete function vertex
            vtxp->unlinkDelete(&m_graph);
            // Delete the function
            VL_DO_DANGLING(ढकला_Deletep(funcp->unlinkFrBack()), funcp);
        }

        // Delete inlined/deleted call site vertices (for debugging only)
        साठी (V3GraphVertex* स्थिरांक vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsCallSiteVertex* स्थिरांक cVtxp = vtxp->cast<InlineCFuncsCallSiteVertex>();
            जर (!cVtxp) सुरूठेवा;
            जर (!cVtxp->inEmpty()) सुरूठेवा;
            जर (!cVtxp->outEmpty()) सुरूठेवा;
            vtxp->unlinkDelete(&m_graph);
        }
    }

    // VISITORS
    शून्य visit(AstCFunc* nodep) override {
        // Create the function vertex
        InlineCFuncsFunctionVertex* स्थिरांक vtxp = getInlineCFuncsFunctionVertexp(nodep);

        // Check if the function itself is not inlineable
        जर (nodep->rtnTypeVoid() != "void") vtxp->setNoInline("Not void");
        जर (nodep->dpiImportPrototype()) vtxp->setNoInline("DPI import prototype");
        जर (nodep->recursive()) vtxp->setNoInline("Recursive");
        जर (nodep->argsp()) vtxp->setNoInline("Has arguments");
        जर (nodep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Check if the function should not be removed
        जर (nodep->entryPoint()) vtxp->setKeep("Entry point");
        जर (nodep->dpiImportPrototype()) vtxp->setKeep("DPI import prototype");
        जर (nodep->dpiExportDispatcher()) vtxp->setKeep("DPI export implementation");
        जर (nodep->isVirtual()) vtxp->setKeep("Virtual method");

        // Iterate children
        VL_RESTORER(m_cfuncVtxp);
        m_cfuncVtxp = vtxp;
        iterateChildrenConst(nodep);
    }

    // Inlineable calls
    शून्य visit(AstCCall* nodep) override {
        जर (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        AstCFunc* स्थिरांक calleep = nodep->funcp();

        // Create the call site vertex
        InlineCFuncsCallSiteVertex* स्थिरांक vtxp = getInlineCFuncsCallSiteVertexp(nodep);

        // Check if the call site is not inlineable
        जर (!VN_IS(nodep->backp(), Stmt_अभिव्यक्ती)) vtxp->setNoInline("Not in statement position");
        जर (m_inExecGraph) vtxp->setNoInline("In ExecGraph");
        जर (calleep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Add caller/callee edges
        जर (m_cfuncVtxp) m_graph.addEdge(*m_cfuncVtxp, *vtxp);
        m_graph.addEdge(*vtxp, *getInlineCFuncsFunctionVertexp(calleep));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    शून्य visit(Ast_गाठ_CCall* nodep) override {
        जर (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Called elsewhere");
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstAddrOfCFunc* nodep) override {
        जर (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Referenced by AddressOfCFunc");
        iterateChildrenConst(nodep);
    }

    // Nodes preventing inlining
    शून्य visit(AstTraceDecl* nodep) override {
        // Referenced by TraceInc
        जर (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains TraceDecl");

        जर (AstCCall* स्थिरांक callp = nodep->dtypeCallp()) {
            getInlineCFuncsCallSiteVertexp(callp)->setNoInline("Referenced by TraceDecl");
        }
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstExecGraph* nodep) override {
        // AstExecGraph is not cloneable, so can't inline the containing function
        जर (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains ExecGraph");
        // Also mark functions referenced in the dependency graph
        साठी (स्थिरांक V3GraphVertex& vtx : nodep->depGraphp()->vertices()) {
            getInlineCFuncsFunctionVertexp(vtx.as<ExecMTask>()->funcp())
                ->setKeep("MTask function");
        }
        VL_RESTORER(m_inExecGraph);
        m_inExecGraph = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCStmt* nodep) override {
        // Can reference anything in text
        जर (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmt");
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCExpr* nodep) override {
        // Can reference anything in text
        जर (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExpr");
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCStmtUser* nodep) override {
        // Can reference anything in text
        जर (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmtUser");
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCExprUser* nodep) override {
        // Can reference anything in text
        जर (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExprUser");
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCReturn* nodep) override {
        जर (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CReturn");
        iterateChildrenConst(nodep);
    }

    // Base node
    शून्य visit(Ast_गाठ* nodep) override {
        जर (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
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
        जर (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-graph");
        // Phase 2: Inline calls
        doInlining();
        जर (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-inlined");
        // Phase 3: Remove unused functions
        removeUnusedFuncs();
        जर (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-kept");
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
    जर (v3Global.opt.profCFuncs()) परत;
    { InlineCFuncsVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("inlinecfuncs", 0, dump_वृक्ष_Either_Level() >= 6);
}
