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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3InlineCFuncs.h"

#ଅନ୍ତର୍ଭାଵ "V3AstUserAllocator.h"
#ଅନ୍ତର୍ଭାଵ "V3ExecGraph.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Bipartite call graph containing function and call site vertices

ଶ୍ରେଣୀ InlineCFuncsFunctionVertex;
ଶ୍ରେଣୀ InlineCFuncsCallSiteVertex;

ଶ୍ରେଣୀ InlineCFuncsCallGraph final : ସର୍ବସାଧାରଣ V3Graph {
ସର୍ବସାଧାରଣ:
    InlineCFuncsCallGraph()
        : V3Graph{} {}
    ~InlineCFuncsCallGraph() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    ଶୂନ୍ୟ addEdge(InlineCFuncsFunctionVertex& from, InlineCFuncsCallSiteVertex& ଶୀର୍ଷ);
    ଶୂନ୍ୟ addEdge(InlineCFuncsCallSiteVertex& from, InlineCFuncsFunctionVertex& ଶୀର୍ଷ);
};

ଶ୍ରେଣୀ EitherVertex VL_NOT_FINAL : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(EitherVertex, V3GraphVertex)
ସୁରକ୍ଷିତ:
    explicit EitherVertex(InlineCFuncsCallGraph& graph)
        : V3GraphVertex{&graph} {}
};

ଶ୍ରେଣୀ InlineCFuncsFunctionVertex final : ସର୍ବସାଧାରଣ EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsFunctionVertex, EitherVertex)
    AstCFunc* ଅଚଳ m_cfuncp;  // The function
    ଅଚଳ ଅକ୍ଷର* m_noInlineWyp = nullptr;  // First reason the function should not be inlined
    ଅଚଳ ଅକ୍ଷର* m_keepWyp = nullptr;  // Why the function should not be removed
    size_t m_size = 0;  // The size of the function

ସର୍ବସାଧାରଣ:
    InlineCFuncsFunctionVertex(InlineCFuncsCallGraph& graph, AstCFunc* cfuncp)
        : EitherVertex{graph}
        , m_cfuncp{cfuncp} {}
    ~InlineCFuncsFunctionVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // ACCESSORS
    AstCFunc* cfuncp() ଅଚଳ { ଫେରନ୍ତୁ m_cfuncp; }
    size_t size() ଅଚଳ { ଫେରନ୍ତୁ m_size; }
    ଶୂନ୍ୟ sizeInc(size_t ମୂଲ୍ୟ = 1) { m_size += ମୂଲ୍ୟ; }
    ତାର୍କିକ noInline() ଅଚଳ { ଫେରନ୍ତୁ m_noInlineWyp; }
    ଶୂନ୍ୟ setNoInline(ଅଚଳ ଅକ୍ଷର* whyp) {
        ଯଦି (!m_noInlineWyp) m_noInlineWyp = whyp;
    }
    ତାର୍କିକ keep() ଅଚଳ { ଫେରନ୍ତୁ m_keepWyp; }
    ଶୂନ୍ୟ setKeep(ଅଚଳ ଅକ୍ଷର* whyp) {
        ଯଦି (!m_keepWyp) m_keepWyp = whyp;
    }
    std::string dotColor() ଅଚଳ override {
        ଫେରନ୍ତୁ m_noInlineWyp ? "red" : m_keepWyp ? "orange" : "black";
    }

    // debug
    File_ଧାଡ଼ି* fileline() ଅଚଳ override { ଫେରନ୍ତୁ m_cfuncp->fileline(); }
    std::string dotShape() ଅଚଳ override { ଫେରନ୍ତୁ "box"; }
    std::string name() ଅଚଳ override VL_MT_STABLE {
        std::string str = cvtToHex(m_cfuncp);
        str += "\n" + m_cfuncp->name();
        str += "\nsize: " + std::to_string(m_size);
        ଯଦି (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        ଯଦି (m_keepWyp) str += "\nKeep: "s + m_keepWyp;
        ଫେରନ୍ତୁ str;
    }
};

ଶ୍ରେଣୀ InlineCFuncsCallSiteVertex final : ସର୍ବସାଧାରଣ EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsCallSiteVertex, EitherVertex)
    AstCCall* ଅଚଳ m_callp;  // The call site
    ଅଚଳ ଅକ୍ଷର* m_noInlineWyp = nullptr;  // First reason the function should not be inlined

ସର୍ବସାଧାରଣ:
    InlineCFuncsCallSiteVertex(InlineCFuncsCallGraph& graph, AstCCall* callp)
        : EitherVertex{graph}
        , m_callp{callp} {}
    ~InlineCFuncsCallSiteVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // ACCESSORS
    AstCCall* callp() ଅଚଳ { ଫେରନ୍ତୁ m_callp; }
    ତାର୍କିକ noInline() ଅଚଳ { ଫେରନ୍ତୁ m_noInlineWyp; }
    ଶୂନ୍ୟ setNoInline(ଅଚଳ ଅକ୍ଷର* whyp) {
        ଯଦି (!m_noInlineWyp) m_noInlineWyp = whyp;
    }

    // debug
    File_ଧାଡ଼ି* fileline() ଅଚଳ override { ଫେରନ୍ତୁ m_callp->fileline(); }
    std::string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ m_noInlineWyp ? "red" : "black"; }
    std::string dotShape() ଅଚଳ override { ଫେରନ୍ତୁ "ellipse"; }
    std::string name() ଅଚଳ override VL_MT_STABLE {
        std::string str = cvtToHex(m_callp);
        ଯଦି (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        ଫେରନ୍ତୁ str;
    }
};

ଶୂନ୍ୟ InlineCFuncsCallGraph::addEdge(InlineCFuncsFunctionVertex& caller,
                                    InlineCFuncsCallSiteVertex& callsite) {
    UASSERT_OBJ(callsite.inEmpty(), &callsite, "Call site should have at most one incoming edge");
    ନୂତନ V3GraphEdge{ଏହି, &caller, &callsite, 1, ସତ୍ୟ};  // Can cut caller -> callsite
}
ଶୂନ୍ୟ InlineCFuncsCallGraph::addEdge(InlineCFuncsCallSiteVertex& callsite,
                                    InlineCFuncsFunctionVertex& callee) {
    UASSERT_OBJ(callsite.outEmpty(), &callsite, "Call site should have at most one outgoing edge");
    ନୂତନ V3GraphEdge{ଏହି, &callsite, &callee, 1, ମିଥ୍ୟା};
}

//######################################################################

ଶ୍ରେଣୀ InlineCFuncsVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // AstCFunc::user1p() ->  InlineCFuncsFunctionVertex*, the function vertex
    // AstCCall::user1p() ->  InlineCFuncsCallSiteVertex*, the call site vertex
    // AstVar::user2p()   ->  AstVar*, the cloned inlined local variable
    ଅଚଳ VNUser1InUse m_user1InUse;

    // STATE
    InlineCFuncsCallGraph m_graph;  // The call graph
    VDouble0 m_statCallsInlined;  // Number of calls inlined
    VDouble0 m_statFuncsInlined;  // Number of functions inlined at least once
    VDouble0 m_statFuncsRemoved;  // Number of fully-inlined functions removed
    // Size threshold: always inline if size <= this
    ଅଚଳ size_t m_sizeThreshold = v3Global.opt.inlineCFuncs();
    // Product threshold: inline if size * calls <= this
    ଅଚଳ size_t m_prodThreshold = v3Global.opt.inlineCFuncsProduct();
    // Maximum size of caller to consider inlining into
    ଅଚଳ size_t m_maxSizeCFunc = []() -> size_t {
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxCFunc = v3Global.opt.outputSplitCFuncs();
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxFile = v3Global.opt.outputSplit();
        ଯଦି (maxCFunc <= 0) maxCFunc = std::numeric_limits<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>::max();
        ଯଦି (maxFile <= 0) maxFile = std::numeric_limits<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>::max();
        ଫେରନ୍ତୁ std::min(maxCFunc, maxFile);
    }();
    ଅଚଳ size_t m_maxSizeTrace = []() -> size_t {
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxTrace = v3Global.opt.outputSplitCTrace();
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxFile = v3Global.opt.outputSplit();
        ଯଦି (maxTrace <= 0) maxTrace = std::numeric_limits<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>::max();
        ଯଦି (maxFile <= 0) maxFile = std::numeric_limits<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>::max();
        ଫେରନ୍ତୁ std::min(maxTrace, maxFile);
    }();
    InlineCFuncsFunctionVertex* m_cfuncVtxp = nullptr;  // Vertex of currently iterated function
    ତାର୍କିକ m_inExecGraph = ମିଥ୍ୟା;  // True while inside an AstExecGraph subtree

    // METHODS
    InlineCFuncsFunctionVertex* getInlineCFuncsFunctionVertexp(AstCFunc* cfuncp) {
        ଯଦି (!cfuncp->user1p()) cfuncp->user1p(ନୂତନ InlineCFuncsFunctionVertex{m_graph, cfuncp});
        ଫେରନ୍ତୁ cfuncp->user1u().to<InlineCFuncsFunctionVertex*>();
    }

    InlineCFuncsCallSiteVertex* getInlineCFuncsCallSiteVertexp(AstCCall* callp) {
        ଯଦି (!callp->user1p()) callp->user1p(ନୂତନ InlineCFuncsCallSiteVertex{m_graph, callp});
        ଫେରନ୍ତୁ callp->user1u().to<InlineCFuncsCallSiteVertex*>();
    }

    AstCLocalScope* inlineCall(AstCFunc* ଅଚଳ callerp,  //
                               AstCCall* ଅଚଳ callp,  //
                               AstCFunc* ଅଚଳ calleep,  //
                               ଅଚଳ size_t seq_ଅଙ୍କ) {
        UINFO(6, "Inlining CFunc " << calleep->name() << " into " << callerp->name()
                                   << " at call site " << callp);

        Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ callSitep = VN_AS(callp->backp(), Stmt_ପଦସମଷ୍ଟି);
        ++m_statCallsInlined;

        // Callee might be empty, just delete the call
        ଯଦି (!calleep->stmtsp()) {
            VL_DO_DANGLING(ଠେଲ_Deletep(callSitep->unlinkFrBack()), callSitep);
            ଫେରନ୍ତୁ nullptr;
        }

        // Replace call site with a local scope
        File_ଧାଡ଼ି* ଅଚଳ flp = callSitep->fileline();
        AstCLocalScope* ଅଚଳ lscopep = ନୂତନ AstCLocalScope{flp, nullptr};
        callSitep->replaceWith(lscopep);
        VL_DO_DANGLING(ଠେଲ_Deletep(callSitep), callSitep);
        lscopep->addStmtsp(ନୂତନ AstComment{flp, "Inlined CFunc: " + calleep->name()});

        // Although it's in a local scope, we still make names of cloned locals unique
        ଅଚଳ std::string varPrefix
            = "__Vinline_" + std::to_string(seq_ଅଙ୍କ) + "_" + calleep->name() + "_";

        // AstVar::user2p()  ->  AstVar*, the cloned inlined local variable
        ଅଚଳ VNUser2InUse user2InUse;

        // Clone local variables, add them to the local scope
        ପାଇଁ (AstVar* varp = calleep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            AstVar* ଅଚଳ newVarp = varp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            newVarp->name(varPrefix + varp->name());
            lscopep->addStmtsp(newVarp);
            varp->user2p(newVarp);
        }

        // Clone the function body
        Ast_ଗଣ୍ଠି* ଅଚଳ bodyp = calleep->stmtsp()->clone_ବୃକ୍ଷ(ସତ୍ୟ);
        lscopep->addStmtsp(bodyp);

        // Retarget local variable references to the cloned locals
        // Rename locals defined in the body, TODO: there should be none after #6280
        // Reset vertex pointers on calls
        bodyp->foreachAndNext([&](Ast_ଗଣ୍ଠି* nodep) {
            ଯଦି (AstVarRef* ଅଚଳ refp = VN_CAST(nodep, VarRef)) {
                ଯଦି (AstVar* ଅଚଳ varp = VN_AS(refp->varp()->user2p(), Var)) {
                    // Keep dtype; V3Expand may have widened the reference beyond the variable
                    Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = refp->dtypep();
                    refp->varp(varp);
                    refp->dtypep(dtypep);
                }
            } ଅନ୍ୟଥା ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(nodep, Var)) {
                varp->name(varPrefix + varp->name());
            } ଅନ୍ୟଥା ଯଦି (AstCCall* ଅଚଳ callp = VN_CAST(nodep, CCall)) {
                callp->user1p(nullptr);
            }
        });

        // Return the local scope
        ଫେରନ୍ତୁ lscopep;
    }

    ଶୂନ୍ୟ doInlining() {
        // Need to gather vertices as we are changing the graph
        std::vector<InlineCFuncsFunctionVertex*> m_fVtxps;
        ପାଇଁ (V3GraphVertex& vtx : m_graph.vertices()) {
            ଯଦି (InlineCFuncsFunctionVertex* ଅଚଳ fVtxp = vtx.cast<InlineCFuncsFunctionVertex>()) {
                m_fVtxps.emplace_back(fVtxp);
            }
        }

        // Iterate functions leaf to root
        ପାଇଁ (InlineCFuncsFunctionVertex* ଅଚଳ calleeVtxp : vlstd::reverse_view(m_fVtxps)) {
            // Should we inline this function?
            ଯଦି (calleeVtxp->noInline()) ଜାରି;  // Told not to

            // Check size heuristics
            ଅଚଳ ତାର୍କିକ doIt = [&]() {
                // Inline if small enough
                ଯଦି (calleeVtxp->size() <= m_sizeThreshold) ଫେରନ୍ତୁ ସତ୍ୟ;
                // Inline if not too much bloat
                ଅଚଳ size_t nCalls = calleeVtxp->inEdges().size();
                ଯଦି (nCalls * calleeVtxp->size() <= m_prodThreshold) ଫେରନ୍ତୁ ସତ୍ୟ;
                // Otherwise don't inline
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }();
            ଯଦି (!doIt) ଜାରି;

            // Ok, attempt to inline call sites
            size_t nInlined = 0;
            ପାଇଁ (ଅଚଳ V3GraphEdge* ଅଚଳ edgep : calleeVtxp->inEdges().unlinkable()) {
                InlineCFuncsCallSiteVertex* ଅଚଳ callVtxp
                    = edgep->fromp()->as<InlineCFuncsCallSiteVertex>();

                AstCFunc* ଅଚଳ calleep = calleeVtxp->cfuncp();
                AstCCall* ଅଚଳ callp = callVtxp->callp();
                UINFO(6, "Consider inlining " << calleep->name() << " at call site " << callp);
                // Should we inline this call site?
                ଯଦି (callVtxp->noInline()) ଜାରି;  // Told not to
                ଯଦି (callVtxp->inEmpty()) ଜାରି;  // Don't know where it's called from

                // Pick up the caller
                UASSERT_OBJ(callVtxp->inSize1(), callVtxp->callp(),
                            "Expected exactly one input edge for call site");
                InlineCFuncsFunctionVertex* ଅଚଳ callerVtxp
                    = callVtxp->inEdges().frontp()->fromp()->as<InlineCFuncsFunctionVertex>();
                AstCFunc* ଅଚଳ callerp = callerVtxp->cfuncp();

                // Don't make a function bigger than the limit
                ଅଚଳ size_t limit = callerp->isTrace() ? m_maxSizeTrace : m_maxSizeCFunc;
                ଯଦି (callerVtxp->size() + calleeVtxp->size() > limit) ଜାରି;

                // Can't do it if it's in a different scope, self pointers differ
                ଯଦି (callerp->scopep() != calleep->scopep()) ଜାରି;

                // Inline it
                ଯଦି (!nInlined) ++m_statFuncsInlined;
                Ast_ଗଣ୍ଠି* ଅଚଳ inlinedp = inlineCall(callerp, callp, calleep, nInlined++);

                // Need to adjust the graph:
                // 1. Delete inlined call site
                VL_DO_DANGLING(callVtxp->unlinkDelete(&m_graph), callVtxp);
                // 2. Add new inlined call sites - also increments size of caller
                VL_RESTORER(m_cfuncVtxp);
                m_cfuncVtxp = callerVtxp;
                ଯଦି (inlinedp) iterateChildrenConst(inlinedp);
            }
        }
    }

    ଶୂନ୍ୟ removeUnusedFuncs() {
        // Iterate root to leaves
        ପାଇଁ (V3GraphVertex* ଅଚଳ vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsFunctionVertex* ଅଚଳ fVtxp = vtxp->cast<InlineCFuncsFunctionVertex>();
            ଯଦି (!fVtxp) ଜାରି;
            // Keep if still called
            ଯଦି (!fVtxp->inEmpty()) ଜାରି;
            // Keep for other reasons
            ଯଦି (fVtxp->keep()) ଜାରି;

            AstCFunc* ଅଚଳ funcp = fVtxp->cfuncp();
            UINFO(6, "Removing unused CFunc " << funcp);
            ++m_statFuncsRemoved;

            // Unlink all call sites
            ପାଇଁ (ଅଚଳ V3GraphEdge* ଅଚଳ edgep : vtxp->outEdges().unlinkable()) {
                edgep->ଶୀର୍ଷ()->unlinkEdges(&m_graph);
            }
            // Delete function vertex
            vtxp->unlinkDelete(&m_graph);
            // Delete the function
            VL_DO_DANGLING(ଠେଲ_Deletep(funcp->unlinkFrBack()), funcp);
        }

        // Delete inlined/deleted call site vertices (for debugging only)
        ପାଇଁ (V3GraphVertex* ଅଚଳ vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsCallSiteVertex* ଅଚଳ cVtxp = vtxp->cast<InlineCFuncsCallSiteVertex>();
            ଯଦି (!cVtxp) ଜାରି;
            ଯଦି (!cVtxp->inEmpty()) ଜାରି;
            ଯଦି (!cVtxp->outEmpty()) ଜାରି;
            vtxp->unlinkDelete(&m_graph);
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        // Create the function vertex
        InlineCFuncsFunctionVertex* ଅଚଳ vtxp = getInlineCFuncsFunctionVertexp(nodep);

        // Check if the function itself is not inlineable
        ଯଦି (nodep->rtnTypeVoid() != "void") vtxp->setNoInline("Not void");
        ଯଦି (nodep->dpiImportPrototype()) vtxp->setNoInline("DPI import prototype");
        ଯଦି (nodep->recursive()) vtxp->setNoInline("Recursive");
        ଯଦି (nodep->argsp()) vtxp->setNoInline("Has arguments");
        ଯଦି (nodep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Check if the function should not be removed
        ଯଦି (nodep->entryPoint()) vtxp->setKeep("Entry point");
        ଯଦି (nodep->dpiImportPrototype()) vtxp->setKeep("DPI import prototype");
        ଯଦି (nodep->dpiExportDispatcher()) vtxp->setKeep("DPI export implementation");
        ଯଦି (nodep->isVirtual()) vtxp->setKeep("Virtual method");

        // Iterate children
        VL_RESTORER(m_cfuncVtxp);
        m_cfuncVtxp = vtxp;
        iterateChildrenConst(nodep);
    }

    // Inlineable calls
    ଶୂନ୍ୟ visit(AstCCall* nodep) override {
        ଯଦି (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        AstCFunc* ଅଚଳ calleep = nodep->funcp();

        // Create the call site vertex
        InlineCFuncsCallSiteVertex* ଅଚଳ vtxp = getInlineCFuncsCallSiteVertexp(nodep);

        // Check if the call site is not inlineable
        ଯଦି (!VN_IS(nodep->backp(), Stmt_ପଦସମଷ୍ଟି)) vtxp->setNoInline("Not in statement position");
        ଯଦି (m_inExecGraph) vtxp->setNoInline("In ExecGraph");
        ଯଦି (calleep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Add caller/callee edges
        ଯଦି (m_cfuncVtxp) m_graph.addEdge(*m_cfuncVtxp, *vtxp);
        m_graph.addEdge(*vtxp, *getInlineCFuncsFunctionVertexp(calleep));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_CCall* nodep) override {
        ଯଦି (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Called elsewhere");
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(AstAddrOfCFunc* nodep) override {
        ଯଦି (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Referenced by AddressOfCFunc");
        iterateChildrenConst(nodep);
    }

    // Nodes preventing inlining
    ଶୂନ୍ୟ visit(AstTraceDecl* nodep) override {
        // Referenced by TraceInc
        ଯଦି (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains TraceDecl");

        ଯଦି (AstCCall* ଅଚଳ callp = nodep->dtypeCallp()) {
            getInlineCFuncsCallSiteVertexp(callp)->setNoInline("Referenced by TraceDecl");
        }
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstExecGraph* nodep) override {
        // AstExecGraph is not cloneable, so can't inline the containing function
        ଯଦି (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains ExecGraph");
        // Also mark functions referenced in the dependency graph
        ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : nodep->depGraphp()->vertices()) {
            getInlineCFuncsFunctionVertexp(vtx.as<ExecMTask>()->funcp())
                ->setKeep("MTask function");
        }
        VL_RESTORER(m_inExecGraph);
        m_inExecGraph = ସତ୍ୟ;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstCStmt* nodep) override {
        // Can reference anything in text
        ଯଦି (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmt");
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstCExpr* nodep) override {
        // Can reference anything in text
        ଯଦି (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExpr");
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstCStmtUser* nodep) override {
        // Can reference anything in text
        ଯଦି (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmtUser");
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstCExprUser* nodep) override {
        // Can reference anything in text
        ଯଦି (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExprUser");
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstCReturn* nodep) override {
        ଯଦି (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CReturn");
        iterateChildrenConst(nodep);
    }

    // Base node
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        ଯଦି (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        iterateChildrenConst(nodep);
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit InlineCFuncsVisitor(AstNetlist* nodep) {
        // Phase 1: Build call graph
        iterateConst(nodep);
        // Make acyclic in case there is recursion
        m_graph.acyclic(V3GraphEdge::followAlwaysTrue);
        // Order vertices (any topological order is fine)
        m_graph.order();
        ଯଦି (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-graph");
        // Phase 2: Inline calls
        doInlining();
        ଯଦି (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-inlined");
        // Phase 3: Remove unused functions
        removeUnusedFuncs();
        ଯଦି (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-kept");
    }
    ~InlineCFuncsVisitor() override {
        V3Stats::addStat("Optimizations, Inline CFuncs, calls inlined", m_statCallsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions inlined", m_statFuncsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions removed", m_statFuncsRemoved);
    }
};

//######################################################################
// InlineCFuncs class functions

ଶୂନ୍ୟ V3InlineCFuncs::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    // Don't inline when profiling per-function (it would lose granularity)
    ଯଦି (v3Global.opt.profCFuncs()) ଫେରନ୍ତୁ;
    { InlineCFuncsVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("inlinecfuncs", 0, dump_ବୃକ୍ଷ_Either_Level() >= 6);
}
