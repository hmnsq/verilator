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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3InlineCFuncs.h"

#અન્તર્ભાવ "V3AstUserAllocator.h"
#અન્તર્ભાવ "V3ExecGraph.h"
#અન્તર્ભાવ "V3Graph.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Bipartite call graph containing function and call site vertices

વર્ગ InlineCFuncsFunctionVertex;
વર્ગ InlineCFuncsCallSiteVertex;

વર્ગ InlineCFuncsCallGraph final : જાહેર V3Graph {
જાહેર:
    InlineCFuncsCallGraph()
        : V3Graph{} {}
    ~InlineCFuncsCallGraph() override = મૂળભૂત;

    શૂન્ય addEdge(InlineCFuncsFunctionVertex& from, InlineCFuncsCallSiteVertex& ટોચ);
    શૂન્ય addEdge(InlineCFuncsCallSiteVertex& from, InlineCFuncsFunctionVertex& ટોચ);
};

વર્ગ EitherVertex VL_NOT_FINAL : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(EitherVertex, V3GraphVertex)
સુરક્ષિત:
    explicit EitherVertex(InlineCFuncsCallGraph& graph)
        : V3GraphVertex{&graph} {}
};

વર્ગ InlineCFuncsFunctionVertex final : જાહેર EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsFunctionVertex, EitherVertex)
    AstCFunc* અચલ m_cfuncp;  // The function
    અચલ અક્ષર* m_noInlineWyp = nullptr;  // First reason the function should not be inlined
    અચલ અક્ષર* m_keepWyp = nullptr;  // Why the function should not be removed
    size_t m_size = 0;  // The size of the function

જાહેર:
    InlineCFuncsFunctionVertex(InlineCFuncsCallGraph& graph, AstCFunc* cfuncp)
        : EitherVertex{graph}
        , m_cfuncp{cfuncp} {}
    ~InlineCFuncsFunctionVertex() override = મૂળભૂત;

    // ACCESSORS
    AstCFunc* cfuncp() અચલ { પાછા m_cfuncp; }
    size_t size() અચલ { પાછા m_size; }
    શૂન્ય sizeInc(size_t કિંમત = 1) { m_size += કિંમત; }
    તાર્કિક noInline() અચલ { પાછા m_noInlineWyp; }
    શૂન્ય setNoInline(અચલ અક્ષર* whyp) {
        જો (!m_noInlineWyp) m_noInlineWyp = whyp;
    }
    તાર્કિક keep() અચલ { પાછા m_keepWyp; }
    શૂન્ય setKeep(અચલ અક્ષર* whyp) {
        જો (!m_keepWyp) m_keepWyp = whyp;
    }
    std::string dotColor() અચલ override {
        પાછા m_noInlineWyp ? "red" : m_keepWyp ? "orange" : "black";
    }

    // debug
    File_લીટી* fileline() અચલ override { પાછા m_cfuncp->fileline(); }
    std::string dotShape() અચલ override { પાછા "box"; }
    std::string name() અચલ override VL_MT_STABLE {
        std::string str = cvtToHex(m_cfuncp);
        str += "\n" + m_cfuncp->name();
        str += "\nsize: " + std::to_string(m_size);
        જો (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        જો (m_keepWyp) str += "\nKeep: "s + m_keepWyp;
        પાછા str;
    }
};

વર્ગ InlineCFuncsCallSiteVertex final : જાહેર EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsCallSiteVertex, EitherVertex)
    AstCCall* અચલ m_callp;  // The call site
    અચલ અક્ષર* m_noInlineWyp = nullptr;  // First reason the function should not be inlined

જાહેર:
    InlineCFuncsCallSiteVertex(InlineCFuncsCallGraph& graph, AstCCall* callp)
        : EitherVertex{graph}
        , m_callp{callp} {}
    ~InlineCFuncsCallSiteVertex() override = મૂળભૂત;

    // ACCESSORS
    AstCCall* callp() અચલ { પાછા m_callp; }
    તાર્કિક noInline() અચલ { પાછા m_noInlineWyp; }
    શૂન્ય setNoInline(અચલ અક્ષર* whyp) {
        જો (!m_noInlineWyp) m_noInlineWyp = whyp;
    }

    // debug
    File_લીટી* fileline() અચલ override { પાછા m_callp->fileline(); }
    std::string dotColor() અચલ override { પાછા m_noInlineWyp ? "red" : "black"; }
    std::string dotShape() અચલ override { પાછા "ellipse"; }
    std::string name() અચલ override VL_MT_STABLE {
        std::string str = cvtToHex(m_callp);
        જો (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        પાછા str;
    }
};

શૂન્ય InlineCFuncsCallGraph::addEdge(InlineCFuncsFunctionVertex& caller,
                                    InlineCFuncsCallSiteVertex& callsite) {
    UASSERT_OBJ(callsite.inEmpty(), &callsite, "Call site should have at most one incoming edge");
    નવું V3GraphEdge{આ, &caller, &callsite, 1, સાચું};  // Can cut caller -> callsite
}
શૂન્ય InlineCFuncsCallGraph::addEdge(InlineCFuncsCallSiteVertex& callsite,
                                    InlineCFuncsFunctionVertex& callee) {
    UASSERT_OBJ(callsite.outEmpty(), &callsite, "Call site should have at most one outgoing edge");
    નવું V3GraphEdge{આ, &callsite, &callee, 1, ખોટું};
}

//######################################################################

વર્ગ InlineCFuncsVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // AstCFunc::user1p() ->  InlineCFuncsFunctionVertex*, the function vertex
    // AstCCall::user1p() ->  InlineCFuncsCallSiteVertex*, the call site vertex
    // AstVar::user2p()   ->  AstVar*, the cloned inlined local variable
    અચલ VNUser1InUse m_user1InUse;

    // STATE
    InlineCFuncsCallGraph m_graph;  // The call graph
    VDouble0 m_statCallsInlined;  // Number of calls inlined
    VDouble0 m_statFuncsInlined;  // Number of functions inlined at least once
    VDouble0 m_statFuncsRemoved;  // Number of fully-inlined functions removed
    // Size threshold: always inline if size <= this
    અચલ size_t m_sizeThreshold = v3Global.opt.inlineCFuncs();
    // Product threshold: inline if size * calls <= this
    અચલ size_t m_prodThreshold = v3Global.opt.inlineCFuncsProduct();
    // Maximum size of caller to consider inlining into
    અચલ size_t m_maxSizeCFunc = []() -> size_t {
        પૂર્ણાંક maxCFunc = v3Global.opt.outputSplitCFuncs();
        પૂર્ણાંક maxFile = v3Global.opt.outputSplit();
        જો (maxCFunc <= 0) maxCFunc = std::numeric_limits<પૂર્ણાંક>::max();
        જો (maxFile <= 0) maxFile = std::numeric_limits<પૂર્ણાંક>::max();
        પાછા std::min(maxCFunc, maxFile);
    }();
    અચલ size_t m_maxSizeTrace = []() -> size_t {
        પૂર્ણાંક maxTrace = v3Global.opt.outputSplitCTrace();
        પૂર્ણાંક maxFile = v3Global.opt.outputSplit();
        જો (maxTrace <= 0) maxTrace = std::numeric_limits<પૂર્ણાંક>::max();
        જો (maxFile <= 0) maxFile = std::numeric_limits<પૂર્ણાંક>::max();
        પાછા std::min(maxTrace, maxFile);
    }();
    InlineCFuncsFunctionVertex* m_cfuncVtxp = nullptr;  // Vertex of currently iterated function
    તાર્કિક m_inExecGraph = ખોટું;  // True while inside an AstExecGraph subtree

    // METHODS
    InlineCFuncsFunctionVertex* getInlineCFuncsFunctionVertexp(AstCFunc* cfuncp) {
        જો (!cfuncp->user1p()) cfuncp->user1p(નવું InlineCFuncsFunctionVertex{m_graph, cfuncp});
        પાછા cfuncp->user1u().to<InlineCFuncsFunctionVertex*>();
    }

    InlineCFuncsCallSiteVertex* getInlineCFuncsCallSiteVertexp(AstCCall* callp) {
        જો (!callp->user1p()) callp->user1p(નવું InlineCFuncsCallSiteVertex{m_graph, callp});
        પાછા callp->user1u().to<InlineCFuncsCallSiteVertex*>();
    }

    AstCLocalScope* inlineCall(AstCFunc* અચલ callerp,  //
                               AstCCall* અચલ callp,  //
                               AstCFunc* અચલ calleep,  //
                               અચલ size_t seq_અંક) {
        UINFO(6, "Inlining CFunc " << calleep->name() << " into " << callerp->name()
                                   << " at call site " << callp);

        Ast_ગાંઠ_Stmt* અચલ callSitep = VN_AS(callp->backp(), Stmt_પદાવલિ);
        ++m_statCallsInlined;

        // Callee might be empty, just delete the call
        જો (!calleep->stmtsp()) {
            VL_DO_DANGLING(ધકેલો_Deletep(callSitep->unlinkFrBack()), callSitep);
            પાછા nullptr;
        }

        // Replace call site with a local scope
        File_લીટી* અચલ flp = callSitep->fileline();
        AstCLocalScope* અચલ lscopep = નવું AstCLocalScope{flp, nullptr};
        callSitep->replaceWith(lscopep);
        VL_DO_DANGLING(ધકેલો_Deletep(callSitep), callSitep);
        lscopep->addStmtsp(નવું AstComment{flp, "Inlined CFunc: " + calleep->name()});

        // Although it's in a local scope, we still make names of cloned locals unique
        અચલ std::string varPrefix
            = "__Vinline_" + std::to_string(seq_અંક) + "_" + calleep->name() + "_";

        // AstVar::user2p()  ->  AstVar*, the cloned inlined local variable
        અચલ VNUser2InUse user2InUse;

        // Clone local variables, add them to the local scope
        માટે (AstVar* varp = calleep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            AstVar* અચલ newVarp = varp->clone_વૃક્ષ(ખોટું);
            newVarp->name(varPrefix + varp->name());
            lscopep->addStmtsp(newVarp);
            varp->user2p(newVarp);
        }

        // Clone the function body
        Ast_ગાંઠ* અચલ bodyp = calleep->stmtsp()->clone_વૃક્ષ(સાચું);
        lscopep->addStmtsp(bodyp);

        // Retarget local variable references to the cloned locals
        // Rename locals defined in the body, TODO: there should be none after #6280
        // Reset vertex pointers on calls
        bodyp->foreachAndNext([&](Ast_ગાંઠ* nodep) {
            જો (AstVarRef* અચલ refp = VN_CAST(nodep, VarRef)) {
                જો (AstVar* અચલ varp = VN_AS(refp->varp()->user2p(), Var)) {
                    // Keep dtype; V3Expand may have widened the reference beyond the variable
                    Ast_ગાંઠ_DType* અચલ dtypep = refp->dtypep();
                    refp->varp(varp);
                    refp->dtypep(dtypep);
                }
            } નહીંતર જો (AstVar* અચલ varp = VN_CAST(nodep, Var)) {
                varp->name(varPrefix + varp->name());
            } નહીંતર જો (AstCCall* અચલ callp = VN_CAST(nodep, CCall)) {
                callp->user1p(nullptr);
            }
        });

        // Return the local scope
        પાછા lscopep;
    }

    શૂન્ય doInlining() {
        // Need to gather vertices as we are changing the graph
        std::vector<InlineCFuncsFunctionVertex*> m_fVtxps;
        માટે (V3GraphVertex& vtx : m_graph.vertices()) {
            જો (InlineCFuncsFunctionVertex* અચલ fVtxp = vtx.cast<InlineCFuncsFunctionVertex>()) {
                m_fVtxps.emplace_back(fVtxp);
            }
        }

        // Iterate functions leaf to root
        માટે (InlineCFuncsFunctionVertex* અચલ calleeVtxp : vlstd::reverse_view(m_fVtxps)) {
            // Should we inline this function?
            જો (calleeVtxp->noInline()) ચાલુ;  // Told not to

            // Check size heuristics
            અચલ તાર્કિક doIt = [&]() {
                // Inline if small enough
                જો (calleeVtxp->size() <= m_sizeThreshold) પાછા સાચું;
                // Inline if not too much bloat
                અચલ size_t nCalls = calleeVtxp->inEdges().size();
                જો (nCalls * calleeVtxp->size() <= m_prodThreshold) પાછા સાચું;
                // Otherwise don't inline
                પાછા ખોટું;
            }();
            જો (!doIt) ચાલુ;

            // Ok, attempt to inline call sites
            size_t nInlined = 0;
            માટે (અચલ V3GraphEdge* અચલ edgep : calleeVtxp->inEdges().unlinkable()) {
                InlineCFuncsCallSiteVertex* અચલ callVtxp
                    = edgep->fromp()->as<InlineCFuncsCallSiteVertex>();

                AstCFunc* અચલ calleep = calleeVtxp->cfuncp();
                AstCCall* અચલ callp = callVtxp->callp();
                UINFO(6, "Consider inlining " << calleep->name() << " at call site " << callp);
                // Should we inline this call site?
                જો (callVtxp->noInline()) ચાલુ;  // Told not to
                જો (callVtxp->inEmpty()) ચાલુ;  // Don't know where it's called from

                // Pick up the caller
                UASSERT_OBJ(callVtxp->inSize1(), callVtxp->callp(),
                            "Expected exactly one input edge for call site");
                InlineCFuncsFunctionVertex* અચલ callerVtxp
                    = callVtxp->inEdges().frontp()->fromp()->as<InlineCFuncsFunctionVertex>();
                AstCFunc* અચલ callerp = callerVtxp->cfuncp();

                // Don't make a function bigger than the limit
                અચલ size_t limit = callerp->isTrace() ? m_maxSizeTrace : m_maxSizeCFunc;
                જો (callerVtxp->size() + calleeVtxp->size() > limit) ચાલુ;

                // Can't do it if it's in a different scope, self pointers differ
                જો (callerp->scopep() != calleep->scopep()) ચાલુ;

                // Inline it
                જો (!nInlined) ++m_statFuncsInlined;
                Ast_ગાંઠ* અચલ inlinedp = inlineCall(callerp, callp, calleep, nInlined++);

                // Need to adjust the graph:
                // 1. Delete inlined call site
                VL_DO_DANGLING(callVtxp->unlinkDelete(&m_graph), callVtxp);
                // 2. Add new inlined call sites - also increments size of caller
                VL_RESTORER(m_cfuncVtxp);
                m_cfuncVtxp = callerVtxp;
                જો (inlinedp) iterateChildrenConst(inlinedp);
            }
        }
    }

    શૂન્ય removeUnusedFuncs() {
        // Iterate root to leaves
        માટે (V3GraphVertex* અચલ vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsFunctionVertex* અચલ fVtxp = vtxp->cast<InlineCFuncsFunctionVertex>();
            જો (!fVtxp) ચાલુ;
            // Keep if still called
            જો (!fVtxp->inEmpty()) ચાલુ;
            // Keep for other reasons
            જો (fVtxp->keep()) ચાલુ;

            AstCFunc* અચલ funcp = fVtxp->cfuncp();
            UINFO(6, "Removing unused CFunc " << funcp);
            ++m_statFuncsRemoved;

            // Unlink all call sites
            માટે (અચલ V3GraphEdge* અચલ edgep : vtxp->outEdges().unlinkable()) {
                edgep->ટોચ()->unlinkEdges(&m_graph);
            }
            // Delete function vertex
            vtxp->unlinkDelete(&m_graph);
            // Delete the function
            VL_DO_DANGLING(ધકેલો_Deletep(funcp->unlinkFrBack()), funcp);
        }

        // Delete inlined/deleted call site vertices (for debugging only)
        માટે (V3GraphVertex* અચલ vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsCallSiteVertex* અચલ cVtxp = vtxp->cast<InlineCFuncsCallSiteVertex>();
            જો (!cVtxp) ચાલુ;
            જો (!cVtxp->inEmpty()) ચાલુ;
            જો (!cVtxp->outEmpty()) ચાલુ;
            vtxp->unlinkDelete(&m_graph);
        }
    }

    // VISITORS
    શૂન્ય visit(AstCFunc* nodep) override {
        // Create the function vertex
        InlineCFuncsFunctionVertex* અચલ vtxp = getInlineCFuncsFunctionVertexp(nodep);

        // Check if the function itself is not inlineable
        જો (nodep->rtnTypeVoid() != "void") vtxp->setNoInline("Not void");
        જો (nodep->dpiImportPrototype()) vtxp->setNoInline("DPI import prototype");
        જો (nodep->recursive()) vtxp->setNoInline("Recursive");
        જો (nodep->argsp()) vtxp->setNoInline("Has arguments");
        જો (nodep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Check if the function should not be removed
        જો (nodep->entryPoint()) vtxp->setKeep("Entry point");
        જો (nodep->dpiImportPrototype()) vtxp->setKeep("DPI import prototype");
        જો (nodep->dpiExportDispatcher()) vtxp->setKeep("DPI export implementation");
        જો (nodep->isVirtual()) vtxp->setKeep("Virtual method");

        // Iterate children
        VL_RESTORER(m_cfuncVtxp);
        m_cfuncVtxp = vtxp;
        iterateChildrenConst(nodep);
    }

    // Inlineable calls
    શૂન્ય visit(AstCCall* nodep) override {
        જો (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        AstCFunc* અચલ calleep = nodep->funcp();

        // Create the call site vertex
        InlineCFuncsCallSiteVertex* અચલ vtxp = getInlineCFuncsCallSiteVertexp(nodep);

        // Check if the call site is not inlineable
        જો (!VN_IS(nodep->backp(), Stmt_પદાવલિ)) vtxp->setNoInline("Not in statement position");
        જો (m_inExecGraph) vtxp->setNoInline("In ExecGraph");
        જો (calleep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Add caller/callee edges
        જો (m_cfuncVtxp) m_graph.addEdge(*m_cfuncVtxp, *vtxp);
        m_graph.addEdge(*vtxp, *getInlineCFuncsFunctionVertexp(calleep));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ_CCall* nodep) override {
        જો (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Called elsewhere");
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(AstAddrOfCFunc* nodep) override {
        જો (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Referenced by AddressOfCFunc");
        iterateChildrenConst(nodep);
    }

    // Nodes preventing inlining
    શૂન્ય visit(AstTraceDecl* nodep) override {
        // Referenced by TraceInc
        જો (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains TraceDecl");

        જો (AstCCall* અચલ callp = nodep->dtypeCallp()) {
            getInlineCFuncsCallSiteVertexp(callp)->setNoInline("Referenced by TraceDecl");
        }
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstExecGraph* nodep) override {
        // AstExecGraph is not cloneable, so can't inline the containing function
        જો (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains ExecGraph");
        // Also mark functions referenced in the dependency graph
        માટે (અચલ V3GraphVertex& vtx : nodep->depGraphp()->vertices()) {
            getInlineCFuncsFunctionVertexp(vtx.as<ExecMTask>()->funcp())
                ->setKeep("MTask function");
        }
        VL_RESTORER(m_inExecGraph);
        m_inExecGraph = સાચું;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstCStmt* nodep) override {
        // Can reference anything in text
        જો (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmt");
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstCExpr* nodep) override {
        // Can reference anything in text
        જો (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExpr");
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstCStmtUser* nodep) override {
        // Can reference anything in text
        જો (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmtUser");
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstCExprUser* nodep) override {
        // Can reference anything in text
        જો (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExprUser");
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstCReturn* nodep) override {
        જો (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CReturn");
        iterateChildrenConst(nodep);
    }

    // Base node
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        જો (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        iterateChildrenConst(nodep);
    }

જાહેર:
    // CONSTRUCTORS
    explicit InlineCFuncsVisitor(AstNetlist* nodep) {
        // Phase 1: Build call graph
        iterateConst(nodep);
        // Make acyclic in case there is recursion
        m_graph.acyclic(V3GraphEdge::followAlwaysTrue);
        // Order vertices (any topological order is fine)
        m_graph.order();
        જો (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-graph");
        // Phase 2: Inline calls
        doInlining();
        જો (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-inlined");
        // Phase 3: Remove unused functions
        removeUnusedFuncs();
        જો (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-kept");
    }
    ~InlineCFuncsVisitor() override {
        V3Stats::addStat("Optimizations, Inline CFuncs, calls inlined", m_statCallsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions inlined", m_statFuncsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions removed", m_statFuncsRemoved);
    }
};

//######################################################################
// InlineCFuncs class functions

શૂન્ય V3InlineCFuncs::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    // Don't inline when profiling per-function (it would lose granularity)
    જો (v3Global.opt.profCFuncs()) પાછા;
    { InlineCFuncsVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("inlinecfuncs", 0, dump_વૃક્ષ_Either_Level() >= 6);
}
