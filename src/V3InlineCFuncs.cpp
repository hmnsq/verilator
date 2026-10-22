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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3InlineCFuncs.h"

#ಅನ್ತರ್ಭಾವ "V3AstUserAllocator.h"
#ಅನ್ತರ್ಭಾವ "V3ExecGraph.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Bipartite call graph containing function and call site vertices

ವರ್ಗ InlineCFuncsFunctionVertex;
ವರ್ಗ InlineCFuncsCallSiteVertex;

ವರ್ಗ InlineCFuncsCallGraph final : ಸಾರ್ವಜನಿಕ V3Graph {
ಸಾರ್ವಜನಿಕ:
    InlineCFuncsCallGraph()
        : V3Graph{} {}
    ~InlineCFuncsCallGraph() override = ಸ್ವಭಾವ;

    ಶೂನ್ಯ addEdge(InlineCFuncsFunctionVertex& from, InlineCFuncsCallSiteVertex& ತುದಿ);
    ಶೂನ್ಯ addEdge(InlineCFuncsCallSiteVertex& from, InlineCFuncsFunctionVertex& ತುದಿ);
};

ವರ್ಗ EitherVertex VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(EitherVertex, V3GraphVertex)
ರಕ್ಷಿತ:
    explicit EitherVertex(InlineCFuncsCallGraph& graph)
        : V3GraphVertex{&graph} {}
};

ವರ್ಗ InlineCFuncsFunctionVertex final : ಸಾರ್ವಜನಿಕ EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsFunctionVertex, EitherVertex)
    AstCFunc* ಧ್ರುವ m_cfuncp;  // The function
    ಧ್ರುವ ವರ್ಣ* m_noInlineWyp = nullptr;  // First reason the function should not be inlined
    ಧ್ರುವ ವರ್ಣ* m_keepWyp = nullptr;  // Why the function should not be removed
    size_t m_size = 0;  // The size of the function

ಸಾರ್ವಜನಿಕ:
    InlineCFuncsFunctionVertex(InlineCFuncsCallGraph& graph, AstCFunc* cfuncp)
        : EitherVertex{graph}
        , m_cfuncp{cfuncp} {}
    ~InlineCFuncsFunctionVertex() override = ಸ್ವಭಾವ;

    // ACCESSORS
    AstCFunc* cfuncp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_cfuncp; }
    size_t size() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_size; }
    ಶೂನ್ಯ sizeInc(size_t ಮೌಲ್ಯ = 1) { m_size += ಮೌಲ್ಯ; }
    ತಾರ್ಕಿಕ noInline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_noInlineWyp; }
    ಶೂನ್ಯ setNoInline(ಧ್ರುವ ವರ್ಣ* whyp) {
        ಯದಿ (!m_noInlineWyp) m_noInlineWyp = whyp;
    }
    ತಾರ್ಕಿಕ keep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_keepWyp; }
    ಶೂನ್ಯ setKeep(ಧ್ರುವ ವರ್ಣ* whyp) {
        ಯದಿ (!m_keepWyp) m_keepWyp = whyp;
    }
    std::string dotColor() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ m_noInlineWyp ? "red" : m_keepWyp ? "orange" : "black";
    }

    // debug
    File_ಸಾಲು* fileline() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_cfuncp->fileline(); }
    std::string dotShape() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "box"; }
    std::string name() ಧ್ರುವ override VL_MT_STABLE {
        std::string str = cvtToHex(m_cfuncp);
        str += "\n" + m_cfuncp->name();
        str += "\nsize: " + std::to_string(m_size);
        ಯದಿ (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        ಯದಿ (m_keepWyp) str += "\nKeep: "s + m_keepWyp;
        ಪ್ರತ್ಯಾಗಮ str;
    }
};

ವರ್ಗ InlineCFuncsCallSiteVertex final : ಸಾರ್ವಜನಿಕ EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsCallSiteVertex, EitherVertex)
    AstCCall* ಧ್ರುವ m_callp;  // The call site
    ಧ್ರುವ ವರ್ಣ* m_noInlineWyp = nullptr;  // First reason the function should not be inlined

ಸಾರ್ವಜನಿಕ:
    InlineCFuncsCallSiteVertex(InlineCFuncsCallGraph& graph, AstCCall* callp)
        : EitherVertex{graph}
        , m_callp{callp} {}
    ~InlineCFuncsCallSiteVertex() override = ಸ್ವಭಾವ;

    // ACCESSORS
    AstCCall* callp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_callp; }
    ತಾರ್ಕಿಕ noInline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_noInlineWyp; }
    ಶೂನ್ಯ setNoInline(ಧ್ರುವ ವರ್ಣ* whyp) {
        ಯದಿ (!m_noInlineWyp) m_noInlineWyp = whyp;
    }

    // debug
    File_ಸಾಲು* fileline() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_callp->fileline(); }
    std::string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_noInlineWyp ? "red" : "black"; }
    std::string dotShape() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "ellipse"; }
    std::string name() ಧ್ರುವ override VL_MT_STABLE {
        std::string str = cvtToHex(m_callp);
        ಯದಿ (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        ಪ್ರತ್ಯಾಗಮ str;
    }
};

ಶೂನ್ಯ InlineCFuncsCallGraph::addEdge(InlineCFuncsFunctionVertex& caller,
                                    InlineCFuncsCallSiteVertex& callsite) {
    UASSERT_OBJ(callsite.inEmpty(), &callsite, "Call site should have at most one incoming edge");
    ನೂತನ V3GraphEdge{ಏತತ್, &caller, &callsite, 1, ಸತ್ಯ};  // Can cut caller -> callsite
}
ಶೂನ್ಯ InlineCFuncsCallGraph::addEdge(InlineCFuncsCallSiteVertex& callsite,
                                    InlineCFuncsFunctionVertex& callee) {
    UASSERT_OBJ(callsite.outEmpty(), &callsite, "Call site should have at most one outgoing edge");
    ನೂತನ V3GraphEdge{ಏತತ್, &callsite, &callee, 1, ಅಸತ್ಯ};
}

//######################################################################

ವರ್ಗ InlineCFuncsVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // AstCFunc::user1p() ->  InlineCFuncsFunctionVertex*, the function vertex
    // AstCCall::user1p() ->  InlineCFuncsCallSiteVertex*, the call site vertex
    // AstVar::user2p()   ->  AstVar*, the cloned inlined local variable
    ಧ್ರುವ VNUser1InUse m_user1InUse;

    // STATE
    InlineCFuncsCallGraph m_graph;  // The call graph
    VDouble0 m_statCallsInlined;  // Number of calls inlined
    VDouble0 m_statFuncsInlined;  // Number of functions inlined at least once
    VDouble0 m_statFuncsRemoved;  // Number of fully-inlined functions removed
    // Size threshold: always inline if size <= this
    ಧ್ರುವ size_t m_sizeThreshold = v3Global.opt.inlineCFuncs();
    // Product threshold: inline if size * calls <= this
    ಧ್ರುವ size_t m_prodThreshold = v3Global.opt.inlineCFuncsProduct();
    // Maximum size of caller to consider inlining into
    ಧ್ರುವ size_t m_maxSizeCFunc = []() -> size_t {
        ಪೂರ್ಣಾಙ್ಕ maxCFunc = v3Global.opt.outputSplitCFuncs();
        ಪೂರ್ಣಾಙ್ಕ maxFile = v3Global.opt.outputSplit();
        ಯದಿ (maxCFunc <= 0) maxCFunc = std::numeric_limits<ಪೂರ್ಣಾಙ್ಕ>::max();
        ಯದಿ (maxFile <= 0) maxFile = std::numeric_limits<ಪೂರ್ಣಾಙ್ಕ>::max();
        ಪ್ರತ್ಯಾಗಮ std::min(maxCFunc, maxFile);
    }();
    ಧ್ರುವ size_t m_maxSizeTrace = []() -> size_t {
        ಪೂರ್ಣಾಙ್ಕ maxTrace = v3Global.opt.outputSplitCTrace();
        ಪೂರ್ಣಾಙ್ಕ maxFile = v3Global.opt.outputSplit();
        ಯದಿ (maxTrace <= 0) maxTrace = std::numeric_limits<ಪೂರ್ಣಾಙ್ಕ>::max();
        ಯದಿ (maxFile <= 0) maxFile = std::numeric_limits<ಪೂರ್ಣಾಙ್ಕ>::max();
        ಪ್ರತ್ಯಾಗಮ std::min(maxTrace, maxFile);
    }();
    InlineCFuncsFunctionVertex* m_cfuncVtxp = nullptr;  // Vertex of currently iterated function
    ತಾರ್ಕಿಕ m_inExecGraph = ಅಸತ್ಯ;  // True while inside an AstExecGraph subtree

    // METHODS
    InlineCFuncsFunctionVertex* getInlineCFuncsFunctionVertexp(AstCFunc* cfuncp) {
        ಯದಿ (!cfuncp->user1p()) cfuncp->user1p(ನೂತನ InlineCFuncsFunctionVertex{m_graph, cfuncp});
        ಪ್ರತ್ಯಾಗಮ cfuncp->user1u().to<InlineCFuncsFunctionVertex*>();
    }

    InlineCFuncsCallSiteVertex* getInlineCFuncsCallSiteVertexp(AstCCall* callp) {
        ಯದಿ (!callp->user1p()) callp->user1p(ನೂತನ InlineCFuncsCallSiteVertex{m_graph, callp});
        ಪ್ರತ್ಯಾಗಮ callp->user1u().to<InlineCFuncsCallSiteVertex*>();
    }

    AstCLocalScope* inlineCall(AstCFunc* ಧ್ರುವ callerp,  //
                               AstCCall* ಧ್ರುವ callp,  //
                               AstCFunc* ಧ್ರುವ calleep,  //
                               ಧ್ರುವ size_t seq_ಅಂಕೆ) {
        UINFO(6, "Inlining CFunc " << calleep->name() << " into " << callerp->name()
                                   << " at call site " << callp);

        Ast_ಗಂಟು_Stmt* ಧ್ರುವ callSitep = VN_AS(callp->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ);
        ++m_statCallsInlined;

        // Callee might be empty, just delete the call
        ಯದಿ (!calleep->stmtsp()) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(callSitep->unlinkFrBack()), callSitep);
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }

        // Replace call site with a local scope
        File_ಸಾಲು* ಧ್ರುವ flp = callSitep->fileline();
        AstCLocalScope* ಧ್ರುವ lscopep = ನೂತನ AstCLocalScope{flp, nullptr};
        callSitep->replaceWith(lscopep);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(callSitep), callSitep);
        lscopep->addStmtsp(ನೂತನ AstComment{flp, "Inlined CFunc: " + calleep->name()});

        // Although it's in a local scope, we still make names of cloned locals unique
        ಧ್ರುವ std::string varPrefix
            = "__Vinline_" + std::to_string(seq_ಅಂಕೆ) + "_" + calleep->name() + "_";

        // AstVar::user2p()  ->  AstVar*, the cloned inlined local variable
        ಧ್ರುವ VNUser2InUse user2InUse;

        // Clone local variables, add them to the local scope
        ಪ್ರತಿ (AstVar* varp = calleep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            AstVar* ಧ್ರುವ newVarp = varp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            newVarp->name(varPrefix + varp->name());
            lscopep->addStmtsp(newVarp);
            varp->user2p(newVarp);
        }

        // Clone the function body
        Ast_ಗಂಟು* ಧ್ರುವ bodyp = calleep->stmtsp()->clone_ವೃಕ್ಷ(ಸತ್ಯ);
        lscopep->addStmtsp(bodyp);

        // Retarget local variable references to the cloned locals
        // Rename locals defined in the body, TODO: there should be none after #6280
        // Reset vertex pointers on calls
        bodyp->foreachAndNext([&](Ast_ಗಂಟು* nodep) {
            ಯದಿ (AstVarRef* ಧ್ರುವ refp = VN_CAST(nodep, VarRef)) {
                ಯದಿ (AstVar* ಧ್ರುವ varp = VN_AS(refp->varp()->user2p(), Var)) {
                    // Keep dtype; V3Expand may have widened the reference beyond the variable
                    Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = refp->dtypep();
                    refp->varp(varp);
                    refp->dtypep(dtypep);
                }
            } ಅನ್ಯಥಾ ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var)) {
                varp->name(varPrefix + varp->name());
            } ಅನ್ಯಥಾ ಯದಿ (AstCCall* ಧ್ರುವ callp = VN_CAST(nodep, CCall)) {
                callp->user1p(nullptr);
            }
        });

        // Return the local scope
        ಪ್ರತ್ಯಾಗಮ lscopep;
    }

    ಶೂನ್ಯ doInlining() {
        // Need to gather vertices as we are changing the graph
        std::vector<InlineCFuncsFunctionVertex*> m_fVtxps;
        ಪ್ರತಿ (V3GraphVertex& vtx : m_graph.vertices()) {
            ಯದಿ (InlineCFuncsFunctionVertex* ಧ್ರುವ fVtxp = vtx.cast<InlineCFuncsFunctionVertex>()) {
                m_fVtxps.emplace_back(fVtxp);
            }
        }

        // Iterate functions leaf to root
        ಪ್ರತಿ (InlineCFuncsFunctionVertex* ಧ್ರುವ calleeVtxp : vlstd::reverse_view(m_fVtxps)) {
            // Should we inline this function?
            ಯದಿ (calleeVtxp->noInline()) ಅನುವರ್ತ;  // Told not to

            // Check size heuristics
            ಧ್ರುವ ತಾರ್ಕಿಕ doIt = [&]() {
                // Inline if small enough
                ಯದಿ (calleeVtxp->size() <= m_sizeThreshold) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                // Inline if not too much bloat
                ಧ್ರುವ size_t nCalls = calleeVtxp->inEdges().size();
                ಯದಿ (nCalls * calleeVtxp->size() <= m_prodThreshold) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                // Otherwise don't inline
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }();
            ಯದಿ (!doIt) ಅನುವರ್ತ;

            // Ok, attempt to inline call sites
            size_t nInlined = 0;
            ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge* ಧ್ರುವ edgep : calleeVtxp->inEdges().unlinkable()) {
                InlineCFuncsCallSiteVertex* ಧ್ರುವ callVtxp
                    = edgep->fromp()->as<InlineCFuncsCallSiteVertex>();

                AstCFunc* ಧ್ರುವ calleep = calleeVtxp->cfuncp();
                AstCCall* ಧ್ರುವ callp = callVtxp->callp();
                UINFO(6, "Consider inlining " << calleep->name() << " at call site " << callp);
                // Should we inline this call site?
                ಯದಿ (callVtxp->noInline()) ಅನುವರ್ತ;  // Told not to
                ಯದಿ (callVtxp->inEmpty()) ಅನುವರ್ತ;  // Don't know where it's called from

                // Pick up the caller
                UASSERT_OBJ(callVtxp->inSize1(), callVtxp->callp(),
                            "Expected exactly one input edge for call site");
                InlineCFuncsFunctionVertex* ಧ್ರುವ callerVtxp
                    = callVtxp->inEdges().frontp()->fromp()->as<InlineCFuncsFunctionVertex>();
                AstCFunc* ಧ್ರುವ callerp = callerVtxp->cfuncp();

                // Don't make a function bigger than the limit
                ಧ್ರುವ size_t limit = callerp->isTrace() ? m_maxSizeTrace : m_maxSizeCFunc;
                ಯದಿ (callerVtxp->size() + calleeVtxp->size() > limit) ಅನುವರ್ತ;

                // Can't do it if it's in a different scope, self pointers differ
                ಯದಿ (callerp->scopep() != calleep->scopep()) ಅನುವರ್ತ;

                // Inline it
                ಯದಿ (!nInlined) ++m_statFuncsInlined;
                Ast_ಗಂಟು* ಧ್ರುವ inlinedp = inlineCall(callerp, callp, calleep, nInlined++);

                // Need to adjust the graph:
                // 1. Delete inlined call site
                VL_DO_DANGLING(callVtxp->unlinkDelete(&m_graph), callVtxp);
                // 2. Add new inlined call sites - also increments size of caller
                VL_RESTORER(m_cfuncVtxp);
                m_cfuncVtxp = callerVtxp;
                ಯದಿ (inlinedp) iterateChildrenConst(inlinedp);
            }
        }
    }

    ಶೂನ್ಯ removeUnusedFuncs() {
        // Iterate root to leaves
        ಪ್ರತಿ (V3GraphVertex* ಧ್ರುವ vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsFunctionVertex* ಧ್ರುವ fVtxp = vtxp->cast<InlineCFuncsFunctionVertex>();
            ಯದಿ (!fVtxp) ಅನುವರ್ತ;
            // Keep if still called
            ಯದಿ (!fVtxp->inEmpty()) ಅನುವರ್ತ;
            // Keep for other reasons
            ಯದಿ (fVtxp->keep()) ಅನುವರ್ತ;

            AstCFunc* ಧ್ರುವ funcp = fVtxp->cfuncp();
            UINFO(6, "Removing unused CFunc " << funcp);
            ++m_statFuncsRemoved;

            // Unlink all call sites
            ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge* ಧ್ರುವ edgep : vtxp->outEdges().unlinkable()) {
                edgep->ತುದಿ()->unlinkEdges(&m_graph);
            }
            // Delete function vertex
            vtxp->unlinkDelete(&m_graph);
            // Delete the function
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(funcp->unlinkFrBack()), funcp);
        }

        // Delete inlined/deleted call site vertices (for debugging only)
        ಪ್ರತಿ (V3GraphVertex* ಧ್ರುವ vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsCallSiteVertex* ಧ್ರುವ cVtxp = vtxp->cast<InlineCFuncsCallSiteVertex>();
            ಯದಿ (!cVtxp) ಅನುವರ್ತ;
            ಯದಿ (!cVtxp->inEmpty()) ಅನುವರ್ತ;
            ಯದಿ (!cVtxp->outEmpty()) ಅನುವರ್ತ;
            vtxp->unlinkDelete(&m_graph);
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        // Create the function vertex
        InlineCFuncsFunctionVertex* ಧ್ರುವ vtxp = getInlineCFuncsFunctionVertexp(nodep);

        // Check if the function itself is not inlineable
        ಯದಿ (nodep->rtnTypeVoid() != "void") vtxp->setNoInline("Not void");
        ಯದಿ (nodep->dpiImportPrototype()) vtxp->setNoInline("DPI import prototype");
        ಯದಿ (nodep->recursive()) vtxp->setNoInline("Recursive");
        ಯದಿ (nodep->argsp()) vtxp->setNoInline("Has arguments");
        ಯದಿ (nodep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Check if the function should not be removed
        ಯದಿ (nodep->entryPoint()) vtxp->setKeep("Entry point");
        ಯದಿ (nodep->dpiImportPrototype()) vtxp->setKeep("DPI import prototype");
        ಯದಿ (nodep->dpiExportDispatcher()) vtxp->setKeep("DPI export implementation");
        ಯದಿ (nodep->isVirtual()) vtxp->setKeep("Virtual method");

        // Iterate children
        VL_RESTORER(m_cfuncVtxp);
        m_cfuncVtxp = vtxp;
        iterateChildrenConst(nodep);
    }

    // Inlineable calls
    ಶೂನ್ಯ visit(AstCCall* nodep) override {
        ಯದಿ (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        AstCFunc* ಧ್ರುವ calleep = nodep->funcp();

        // Create the call site vertex
        InlineCFuncsCallSiteVertex* ಧ್ರುವ vtxp = getInlineCFuncsCallSiteVertexp(nodep);

        // Check if the call site is not inlineable
        ಯದಿ (!VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)) vtxp->setNoInline("Not in statement position");
        ಯದಿ (m_inExecGraph) vtxp->setNoInline("In ExecGraph");
        ಯದಿ (calleep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Add caller/callee edges
        ಯದಿ (m_cfuncVtxp) m_graph.addEdge(*m_cfuncVtxp, *vtxp);
        m_graph.addEdge(*vtxp, *getInlineCFuncsFunctionVertexp(calleep));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_CCall* nodep) override {
        ಯದಿ (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Called elsewhere");
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ visit(AstAddrOfCFunc* nodep) override {
        ಯದಿ (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Referenced by AddressOfCFunc");
        iterateChildrenConst(nodep);
    }

    // Nodes preventing inlining
    ಶೂನ್ಯ visit(AstTraceDecl* nodep) override {
        // Referenced by TraceInc
        ಯದಿ (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains TraceDecl");

        ಯದಿ (AstCCall* ಧ್ರುವ callp = nodep->dtypeCallp()) {
            getInlineCFuncsCallSiteVertexp(callp)->setNoInline("Referenced by TraceDecl");
        }
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstExecGraph* nodep) override {
        // AstExecGraph is not cloneable, so can't inline the containing function
        ಯದಿ (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains ExecGraph");
        // Also mark functions referenced in the dependency graph
        ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : nodep->depGraphp()->vertices()) {
            getInlineCFuncsFunctionVertexp(vtx.as<ExecMTask>()->funcp())
                ->setKeep("MTask function");
        }
        VL_RESTORER(m_inExecGraph);
        m_inExecGraph = ಸತ್ಯ;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstCStmt* nodep) override {
        // Can reference anything in text
        ಯದಿ (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmt");
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstCExpr* nodep) override {
        // Can reference anything in text
        ಯದಿ (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExpr");
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstCStmtUser* nodep) override {
        // Can reference anything in text
        ಯದಿ (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmtUser");
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstCExprUser* nodep) override {
        // Can reference anything in text
        ಯದಿ (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExprUser");
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstCReturn* nodep) override {
        ಯದಿ (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CReturn");
        iterateChildrenConst(nodep);
    }

    // Base node
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        ಯದಿ (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        iterateChildrenConst(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit InlineCFuncsVisitor(AstNetlist* nodep) {
        // Phase 1: Build call graph
        iterateConst(nodep);
        // Make acyclic in case there is recursion
        m_graph.acyclic(V3GraphEdge::followAlwaysTrue);
        // Order vertices (any topological order is fine)
        m_graph.order();
        ಯದಿ (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-graph");
        // Phase 2: Inline calls
        doInlining();
        ಯದಿ (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-inlined");
        // Phase 3: Remove unused functions
        removeUnusedFuncs();
        ಯದಿ (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-kept");
    }
    ~InlineCFuncsVisitor() override {
        V3Stats::addStat("Optimizations, Inline CFuncs, calls inlined", m_statCallsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions inlined", m_statFuncsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions removed", m_statFuncsRemoved);
    }
};

//######################################################################
// InlineCFuncs class functions

ಶೂನ್ಯ V3InlineCFuncs::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    // Don't inline when profiling per-function (it would lose granularity)
    ಯದಿ (v3Global.opt.profCFuncs()) ಪ್ರತ್ಯಾಗಮ;
    { InlineCFuncsVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("inlinecfuncs", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}
