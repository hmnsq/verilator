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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3InlineCFuncs.h"

#समावेश "V3AstUserAllocator.h"
#समावेश "V3ExecGraph.h"
#समावेश "V3Graph.h"
#समावेश "V3Stats.h"

#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Bipartite call graph containing function and call site vertices

श्रेणी InlineCFuncsFunctionVertex;
श्रेणी InlineCFuncsCallSiteVertex;

श्रेणी InlineCFuncsCallGraph final : खुला V3Graph {
खुला:
    InlineCFuncsCallGraph()
        : V3Graph{} {}
    ~InlineCFuncsCallGraph() override = शेष;

    व्योम addEdge(InlineCFuncsFunctionVertex& from, InlineCFuncsCallSiteVertex& शीर्ष);
    व्योम addEdge(InlineCFuncsCallSiteVertex& from, InlineCFuncsFunctionVertex& शीर्ष);
};

श्रेणी EitherVertex VL_NOT_FINAL : खुला V3GraphVertex {
    VL_RTTI_IMPL(EitherVertex, V3GraphVertex)
रक्षित:
    explicit EitherVertex(InlineCFuncsCallGraph& graph)
        : V3GraphVertex{&graph} {}
};

श्रेणी InlineCFuncsFunctionVertex final : खुला EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsFunctionVertex, EitherVertex)
    AstCFunc* स्थिर m_cfuncp;  // The function
    स्थिर अक्षर* m_noInlineWyp = nullptr;  // First reason the function should not be inlined
    स्थिर अक्षर* m_keepWyp = nullptr;  // Why the function should not be removed
    माप_प्रकार m_आकार = 0;  // The size of the function

खुला:
    InlineCFuncsFunctionVertex(InlineCFuncsCallGraph& graph, AstCFunc* cfuncp)
        : EitherVertex{graph}
        , m_cfuncp{cfuncp} {}
    ~InlineCFuncsFunctionVertex() override = शेष;

    // ACCESSORS
    AstCFunc* cfuncp() स्थिर { वापस m_cfuncp; }
    माप_प्रकार आकार() स्थिर { वापस m_आकार; }
    व्योम आकार_Inc(माप_प्रकार मूल्य = 1) { m_आकार += मूल्य; }
    bool noInline() स्थिर { वापस m_noInlineWyp; }
    व्योम setNoInline(स्थिर अक्षर* whyp) {
        अगर (!m_noInlineWyp) m_noInlineWyp = whyp;
    }
    bool keep() स्थिर { वापस m_keepWyp; }
    व्योम setKeep(स्थिर अक्षर* whyp) {
        अगर (!m_keepWyp) m_keepWyp = whyp;
    }
    std::string dotColor() स्थिर override {
        वापस m_noInlineWyp ? "red" : m_keepWyp ? "orange" : "black";
    }

    // debug
    File_पंक्ति* fileline() स्थिर override { वापस m_cfuncp->fileline(); }
    std::string dotShape() स्थिर override { वापस "box"; }
    std::string name() स्थिर override VL_MT_STABLE {
        std::string str = cvtToHex(m_cfuncp);
        str += "\n" + m_cfuncp->name();
        str += "\nsize: " + std::to_string(m_आकार);
        अगर (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        अगर (m_keepWyp) str += "\nKeep: "s + m_keepWyp;
        वापस str;
    }
};

श्रेणी InlineCFuncsCallSiteVertex final : खुला EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsCallSiteVertex, EitherVertex)
    AstCCall* स्थिर m_callp;  // The call site
    स्थिर अक्षर* m_noInlineWyp = nullptr;  // First reason the function should not be inlined

खुला:
    InlineCFuncsCallSiteVertex(InlineCFuncsCallGraph& graph, AstCCall* callp)
        : EitherVertex{graph}
        , m_callp{callp} {}
    ~InlineCFuncsCallSiteVertex() override = शेष;

    // ACCESSORS
    AstCCall* callp() स्थिर { वापस m_callp; }
    bool noInline() स्थिर { वापस m_noInlineWyp; }
    व्योम setNoInline(स्थिर अक्षर* whyp) {
        अगर (!m_noInlineWyp) m_noInlineWyp = whyp;
    }

    // debug
    File_पंक्ति* fileline() स्थिर override { वापस m_callp->fileline(); }
    std::string dotColor() स्थिर override { वापस m_noInlineWyp ? "red" : "black"; }
    std::string dotShape() स्थिर override { वापस "ellipse"; }
    std::string name() स्थिर override VL_MT_STABLE {
        std::string str = cvtToHex(m_callp);
        अगर (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        वापस str;
    }
};

व्योम InlineCFuncsCallGraph::addEdge(InlineCFuncsFunctionVertex& caller,
                                    InlineCFuncsCallSiteVertex& callsite) {
    UASSERT_OBJ(callsite.inEmpty(), &callsite, "Call site should have at most one incoming edge");
    नया V3GraphEdge{यह, &caller, &callsite, 1, true};  // Can cut caller -> callsite
}
व्योम InlineCFuncsCallGraph::addEdge(InlineCFuncsCallSiteVertex& callsite,
                                    InlineCFuncsFunctionVertex& callee) {
    UASSERT_OBJ(callsite.outEmpty(), &callsite, "Call site should have at most one outgoing edge");
    नया V3GraphEdge{यह, &callsite, &callee, 1, false};
}

//######################################################################

श्रेणी InlineCFuncsVisitor final : खुला VNVisitor {
    // NODE STATE
    // AstCFunc::user1p() ->  InlineCFuncsFunctionVertex*, the function vertex
    // AstCCall::user1p() ->  InlineCFuncsCallSiteVertex*, the call site vertex
    // AstVar::user2p()   ->  AstVar*, the cloned inlined local variable
    स्थिर VNUser1InUse m_user1InUse;

    // STATE
    InlineCFuncsCallGraph m_graph;  // The call graph
    VDouble0 m_statCallsInlined;  // Number of calls inlined
    VDouble0 m_statFuncsInlined;  // Number of functions inlined at least once
    VDouble0 m_statFuncsRemoved;  // Number of fully-inlined functions removed
    // Size threshold: always inline if size <= this
    स्थिर माप_प्रकार m_आकार_Threshold = v3Global.opt.inlineCFuncs();
    // Product threshold: inline if size * calls <= this
    स्थिर माप_प्रकार m_prodThreshold = v3Global.opt.inlineCFuncsProduct();
    // Maximum size of caller to consider inlining into
    स्थिर माप_प्रकार m_max_आकार_CFunc = []() -> माप_प्रकार {
        पूर्णांक maxCFunc = v3Global.opt.outputSplitCFuncs();
        पूर्णांक maxFile = v3Global.opt.outputSplit();
        अगर (maxCFunc <= 0) maxCFunc = std::numeric_limits<पूर्णांक>::max();
        अगर (maxFile <= 0) maxFile = std::numeric_limits<पूर्णांक>::max();
        वापस std::min(maxCFunc, maxFile);
    }();
    स्थिर माप_प्रकार m_max_आकार_Trace = []() -> माप_प्रकार {
        पूर्णांक maxTrace = v3Global.opt.outputSplitCTrace();
        पूर्णांक maxFile = v3Global.opt.outputSplit();
        अगर (maxTrace <= 0) maxTrace = std::numeric_limits<पूर्णांक>::max();
        अगर (maxFile <= 0) maxFile = std::numeric_limits<पूर्णांक>::max();
        वापस std::min(maxTrace, maxFile);
    }();
    InlineCFuncsFunctionVertex* m_cfuncVtxp = nullptr;  // Vertex of currently iterated function
    bool m_inExecGraph = false;  // True while inside an AstExecGraph subtree

    // METHODS
    InlineCFuncsFunctionVertex* getInlineCFuncsFunctionVertexp(AstCFunc* cfuncp) {
        अगर (!cfuncp->user1p()) cfuncp->user1p(नया InlineCFuncsFunctionVertex{m_graph, cfuncp});
        वापस cfuncp->user1u().to<InlineCFuncsFunctionVertex*>();
    }

    InlineCFuncsCallSiteVertex* getInlineCFuncsCallSiteVertexp(AstCCall* callp) {
        अगर (!callp->user1p()) callp->user1p(नया InlineCFuncsCallSiteVertex{m_graph, callp});
        वापस callp->user1u().to<InlineCFuncsCallSiteVertex*>();
    }

    AstCLocalScope* inlineCall(AstCFunc* स्थिर callerp,  //
                               AstCCall* स्थिर callp,  //
                               AstCFunc* स्थिर calleep,  //
                               स्थिर माप_प्रकार seq_अंक) {
        UINFO(6, "Inlining CFunc " << calleep->name() << " into " << callerp->name()
                                   << " at call site " << callp);

        Ast_गाँठ_Stmt* स्थिर callSitep = VN_AS(callp->backp(), Stmt_व्यंजक);
        ++m_statCallsInlined;

        // Callee might be empty, just delete the call
        अगर (!calleep->stmtsp()) {
            VL_DO_DANGLING(धकेलो_Deletep(callSitep->unlinkFrBack()), callSitep);
            वापस nullptr;
        }

        // Replace call site with a local scope
        File_पंक्ति* स्थिर flp = callSitep->fileline();
        AstCLocalScope* स्थिर lscopep = नया AstCLocalScope{flp, nullptr};
        callSitep->replaceWith(lscopep);
        VL_DO_DANGLING(धकेलो_Deletep(callSitep), callSitep);
        lscopep->addStmtsp(नया AstComment{flp, "Inlined CFunc: " + calleep->name()});

        // Although it's in a local scope, we still make names of cloned locals unique
        स्थिर std::string varPrefix
            = "__Vinline_" + std::to_string(seq_अंक) + "_" + calleep->name() + "_";

        // AstVar::user2p()  ->  AstVar*, the cloned inlined local variable
        स्थिर VNUser2InUse user2InUse;

        // Clone local variables, add them to the local scope
        क्रम (AstVar* varp = calleep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            AstVar* स्थिर newVarp = varp->clone_वृक्ष(false);
            newVarp->name(varPrefix + varp->name());
            lscopep->addStmtsp(newVarp);
            varp->user2p(newVarp);
        }

        // Clone the function body
        Ast_गाँठ* स्थिर bodyp = calleep->stmtsp()->clone_वृक्ष(true);
        lscopep->addStmtsp(bodyp);

        // Retarget local variable references to the cloned locals
        // Rename locals defined in the body, TODO: there should be none after #6280
        // Reset vertex pointers on calls
        bodyp->foreach_And_अग्र([&](Ast_गाँठ* nodep) {
            अगर (AstVarRef* स्थिर refp = VN_CAST(nodep, VarRef)) {
                अगर (AstVar* स्थिर varp = VN_AS(refp->varp()->user2p(), Var)) {
                    // Keep dtype; V3Expand may have widened the reference beyond the variable
                    Ast_गाँठ_DType* स्थिर dtypep = refp->dtypep();
                    refp->varp(varp);
                    refp->dtypep(dtypep);
                }
            } अन्यथा अगर (AstVar* स्थिर varp = VN_CAST(nodep, Var)) {
                varp->name(varPrefix + varp->name());
            } अन्यथा अगर (AstCCall* स्थिर callp = VN_CAST(nodep, CCall)) {
                callp->user1p(nullptr);
            }
        });

        // Return the local scope
        वापस lscopep;
    }

    व्योम doInlining() {
        // Need to gather vertices as we are changing the graph
        std::vector<InlineCFuncsFunctionVertex*> m_fVtxps;
        क्रम (V3GraphVertex& vtx : m_graph.vertices()) {
            अगर (InlineCFuncsFunctionVertex* स्थिर fVtxp = vtx.cast<InlineCFuncsFunctionVertex>()) {
                m_fVtxps.emplace_back(fVtxp);
            }
        }

        // Iterate functions leaf to root
        क्रम (InlineCFuncsFunctionVertex* स्थिर calleeVtxp : vlstd::reverse_view(m_fVtxps)) {
            // Should we inline this function?
            अगर (calleeVtxp->noInline()) जारी;  // Told not to

            // Check size heuristics
            स्थिर bool doIt = [&]() {
                // Inline if small enough
                अगर (calleeVtxp->आकार() <= m_आकार_Threshold) वापस true;
                // Inline if not too much bloat
                स्थिर माप_प्रकार nCalls = calleeVtxp->inEdges().आकार();
                अगर (nCalls * calleeVtxp->आकार() <= m_prodThreshold) वापस true;
                // Otherwise don't inline
                वापस false;
            }();
            अगर (!doIt) जारी;

            // Ok, attempt to inline call sites
            माप_प्रकार nInlined = 0;
            क्रम (स्थिर V3GraphEdge* स्थिर edgep : calleeVtxp->inEdges().unlinkable()) {
                InlineCFuncsCallSiteVertex* स्थिर callVtxp
                    = edgep->fromp()->as<InlineCFuncsCallSiteVertex>();

                AstCFunc* स्थिर calleep = calleeVtxp->cfuncp();
                AstCCall* स्थिर callp = callVtxp->callp();
                UINFO(6, "Consider inlining " << calleep->name() << " at call site " << callp);
                // Should we inline this call site?
                अगर (callVtxp->noInline()) जारी;  // Told not to
                अगर (callVtxp->inEmpty()) जारी;  // Don't know where it's called from

                // Pick up the caller
                UASSERT_OBJ(callVtxp->inSize1(), callVtxp->callp(),
                            "Expected exactly one input edge for call site");
                InlineCFuncsFunctionVertex* स्थिर callerVtxp
                    = callVtxp->inEdges().frontp()->fromp()->as<InlineCFuncsFunctionVertex>();
                AstCFunc* स्थिर callerp = callerVtxp->cfuncp();

                // Don't make a function bigger than the limit
                स्थिर माप_प्रकार limit = callerp->isTrace() ? m_max_आकार_Trace : m_max_आकार_CFunc;
                अगर (callerVtxp->आकार() + calleeVtxp->आकार() > limit) जारी;

                // Can't do it if it's in a different scope, self pointers differ
                अगर (callerp->scopep() != calleep->scopep()) जारी;

                // Inline it
                अगर (!nInlined) ++m_statFuncsInlined;
                Ast_गाँठ* स्थिर inlinedp = inlineCall(callerp, callp, calleep, nInlined++);

                // Need to adjust the graph:
                // 1. Delete inlined call site
                VL_DO_DANGLING(callVtxp->unlinkDelete(&m_graph), callVtxp);
                // 2. Add new inlined call sites - also increments size of caller
                VL_RESTORER(m_cfuncVtxp);
                m_cfuncVtxp = callerVtxp;
                अगर (inlinedp) iterateChildrenConst(inlinedp);
            }
        }
    }

    व्योम removeUnusedFuncs() {
        // Iterate root to leaves
        क्रम (V3GraphVertex* स्थिर vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsFunctionVertex* स्थिर fVtxp = vtxp->cast<InlineCFuncsFunctionVertex>();
            अगर (!fVtxp) जारी;
            // Keep if still called
            अगर (!fVtxp->inEmpty()) जारी;
            // Keep for other reasons
            अगर (fVtxp->keep()) जारी;

            AstCFunc* स्थिर funcp = fVtxp->cfuncp();
            UINFO(6, "Removing unused CFunc " << funcp);
            ++m_statFuncsRemoved;

            // Unlink all call sites
            क्रम (स्थिर V3GraphEdge* स्थिर edgep : vtxp->outEdges().unlinkable()) {
                edgep->शीर्ष()->unlinkEdges(&m_graph);
            }
            // Delete function vertex
            vtxp->unlinkDelete(&m_graph);
            // Delete the function
            VL_DO_DANGLING(धकेलो_Deletep(funcp->unlinkFrBack()), funcp);
        }

        // Delete inlined/deleted call site vertices (for debugging only)
        क्रम (V3GraphVertex* स्थिर vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsCallSiteVertex* स्थिर cVtxp = vtxp->cast<InlineCFuncsCallSiteVertex>();
            अगर (!cVtxp) जारी;
            अगर (!cVtxp->inEmpty()) जारी;
            अगर (!cVtxp->outEmpty()) जारी;
            vtxp->unlinkDelete(&m_graph);
        }
    }

    // VISITORS
    व्योम visit(AstCFunc* nodep) override {
        // Create the function vertex
        InlineCFuncsFunctionVertex* स्थिर vtxp = getInlineCFuncsFunctionVertexp(nodep);

        // Check if the function itself is not inlineable
        अगर (nodep->rtnTypeVoid() != "void") vtxp->setNoInline("Not void");
        अगर (nodep->dpiImportPrototype()) vtxp->setNoInline("DPI import prototype");
        अगर (nodep->recursive()) vtxp->setNoInline("Recursive");
        अगर (nodep->argsp()) vtxp->setNoInline("Has arguments");
        अगर (nodep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Check if the function should not be removed
        अगर (nodep->entryPoint()) vtxp->setKeep("Entry point");
        अगर (nodep->dpiImportPrototype()) vtxp->setKeep("DPI import prototype");
        अगर (nodep->dpiExportDispatcher()) vtxp->setKeep("DPI export implementation");
        अगर (nodep->isVirtual()) vtxp->setKeep("Virtual method");

        // Iterate children
        VL_RESTORER(m_cfuncVtxp);
        m_cfuncVtxp = vtxp;
        iterateChildrenConst(nodep);
    }

    // Inlineable calls
    व्योम visit(AstCCall* nodep) override {
        अगर (m_cfuncVtxp) m_cfuncVtxp->आकार_Inc();
        AstCFunc* स्थिर calleep = nodep->funcp();

        // Create the call site vertex
        InlineCFuncsCallSiteVertex* स्थिर vtxp = getInlineCFuncsCallSiteVertexp(nodep);

        // Check if the call site is not inlineable
        अगर (!VN_IS(nodep->backp(), Stmt_व्यंजक)) vtxp->setNoInline("Not in statement position");
        अगर (m_inExecGraph) vtxp->setNoInline("In ExecGraph");
        अगर (calleep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Add caller/callee edges
        अगर (m_cfuncVtxp) m_graph.addEdge(*m_cfuncVtxp, *vtxp);
        m_graph.addEdge(*vtxp, *getInlineCFuncsFunctionVertexp(calleep));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    व्योम visit(Ast_गाँठ_CCall* nodep) override {
        अगर (m_cfuncVtxp) m_cfuncVtxp->आकार_Inc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Called elsewhere");
        iterateChildrenConst(nodep);
    }

    व्योम visit(AstAddrOfCFunc* nodep) override {
        अगर (m_cfuncVtxp) m_cfuncVtxp->आकार_Inc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Referenced by AddressOfCFunc");
        iterateChildrenConst(nodep);
    }

    // Nodes preventing inlining
    व्योम visit(AstTraceDecl* nodep) override {
        // Referenced by TraceInc
        अगर (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains TraceDecl");

        अगर (AstCCall* स्थिर callp = nodep->dtypeCallp()) {
            getInlineCFuncsCallSiteVertexp(callp)->setNoInline("Referenced by TraceDecl");
        }
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstExecGraph* nodep) override {
        // AstExecGraph is not cloneable, so can't inline the containing function
        अगर (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains ExecGraph");
        // Also mark functions referenced in the dependency graph
        क्रम (स्थिर V3GraphVertex& vtx : nodep->depGraphp()->vertices()) {
            getInlineCFuncsFunctionVertexp(vtx.as<ExecMTask>()->funcp())
                ->setKeep("MTask function");
        }
        VL_RESTORER(m_inExecGraph);
        m_inExecGraph = true;
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstCStmt* nodep) override {
        // Can reference anything in text
        अगर (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmt");
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstCExpr* nodep) override {
        // Can reference anything in text
        अगर (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExpr");
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstCStmtUser* nodep) override {
        // Can reference anything in text
        अगर (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmtUser");
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstCExprUser* nodep) override {
        // Can reference anything in text
        अगर (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExprUser");
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstCReturn* nodep) override {
        अगर (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CReturn");
        iterateChildrenConst(nodep);
    }

    // Base node
    व्योम visit(Ast_गाँठ* nodep) override {
        अगर (m_cfuncVtxp) m_cfuncVtxp->आकार_Inc();
        iterateChildrenConst(nodep);
    }

खुला:
    // CONSTRUCTORS
    explicit InlineCFuncsVisitor(AstNetlist* nodep) {
        // Phase 1: Build call graph
        iterateConst(nodep);
        // Make acyclic in case there is recursion
        m_graph.acyclic(V3GraphEdge::followAlwaysTrue);
        // Order vertices (any topological order is fine)
        m_graph.order();
        अगर (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-graph");
        // Phase 2: Inline calls
        doInlining();
        अगर (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-inlined");
        // Phase 3: Remove unused functions
        removeUnusedFuncs();
        अगर (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-kept");
    }
    ~InlineCFuncsVisitor() override {
        V3Stats::addStat("Optimizations, Inline CFuncs, calls inlined", m_statCallsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions inlined", m_statFuncsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions removed", m_statFuncsRemoved);
    }
};

//######################################################################
// InlineCFuncs class functions

व्योम V3InlineCFuncs::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    // Don't inline when profiling per-function (it would lose granularity)
    अगर (v3Global.opt.profCFuncs()) वापस;
    { InlineCFuncsVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("inlinecfuncs", 0, dump_वृक्ष_Either_Level() >= 6);
}
