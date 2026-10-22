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

    فراغ addEdge(InlineCFuncsFunctionVertex& from, InlineCFuncsCallSiteVertex& اتس);
    فراغ addEdge(InlineCFuncsCallSiteVertex& from, InlineCFuncsFunctionVertex& اتس);
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
    فراغ sizeInc(size_t نيلاي = 1) { m_size += نيلاي; }
    منطقي noInline() ثابت { أرجع m_noInlineWyp; }
    فراغ setNoInline(ثابت حرف* whyp) {
        إذا (!m_noInlineWyp) m_noInlineWyp = whyp;
    }
    منطقي keep() ثابت { أرجع m_keepWyp; }
    فراغ setKeep(ثابت حرف* whyp) {
        إذا (!m_keepWyp) m_keepWyp = whyp;
    }
    std::string dotColor() ثابت override {
        أرجع m_noInlineWyp ? "red" : m_keepWyp ? "orange" : "black";
    }

    // debug
    File_باريس* fileline() ثابت override { أرجع m_cfuncp->fileline(); }
    std::string dotShape() ثابت override { أرجع "box"; }
    std::string name() ثابت override VL_MT_STABLE {
        std::string str = cvtToHex(m_cfuncp);
        str += "\n" + m_cfuncp->name();
        str += "\nsize: " + std::to_string(m_size);
        إذا (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        إذا (m_keepWyp) str += "\nKeep: "s + m_keepWyp;
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
        إذا (!m_noInlineWyp) m_noInlineWyp = whyp;
    }

    // debug
    File_باريس* fileline() ثابت override { أرجع m_callp->fileline(); }
    std::string dotColor() ثابت override { أرجع m_noInlineWyp ? "red" : "black"; }
    std::string dotShape() ثابت override { أرجع "ellipse"; }
    std::string name() ثابت override VL_MT_STABLE {
        std::string str = cvtToHex(m_callp);
        إذا (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        أرجع str;
    }
};

فراغ InlineCFuncsCallGraph::addEdge(InlineCFuncsFunctionVertex& caller,
                                    InlineCFuncsCallSiteVertex& callsite) {
    UASSERT_OBJ(callsite.inEmpty(), &callsite, "Call site should have at most one incoming edge");
    جديد V3GraphEdge{هذا, &caller, &callsite, 1, true};  // Can cut caller -> callsite
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
        إذا (maxCFunc <= 0) maxCFunc = std::numeric_limits<صحيح>::max();
        إذا (maxFile <= 0) maxFile = std::numeric_limits<صحيح>::max();
        أرجع std::min(maxCFunc, maxFile);
    }();
    ثابت size_t m_maxSizeTrace = []() -> size_t {
        صحيح maxTrace = v3Global.opt.outputSplitCTrace();
        صحيح maxFile = v3Global.opt.outputSplit();
        إذا (maxTrace <= 0) maxTrace = std::numeric_limits<صحيح>::max();
        إذا (maxFile <= 0) maxFile = std::numeric_limits<صحيح>::max();
        أرجع std::min(maxTrace, maxFile);
    }();
    InlineCFuncsFunctionVertex* m_cfuncVtxp = nullptr;  // Vertex of currently iterated function
    منطقي m_inExecGraph = خطأ;  // True while inside an AstExecGraph subtree

    // METHODS
    InlineCFuncsFunctionVertex* getInlineCFuncsFunctionVertexp(AstCFunc* cfuncp) {
        إذا (!cfuncp->user1p()) cfuncp->user1p(جديد InlineCFuncsFunctionVertex{m_graph, cfuncp});
        أرجع cfuncp->user1u().to<InlineCFuncsFunctionVertex*>();
    }

    InlineCFuncsCallSiteVertex* getInlineCFuncsCallSiteVertexp(AstCCall* callp) {
        إذا (!callp->user1p()) callp->user1p(جديد InlineCFuncsCallSiteVertex{m_graph, callp});
        أرجع callp->user1u().to<InlineCFuncsCallSiteVertex*>();
    }

    AstCLocalScope* inlineCall(AstCFunc* ثابت callerp,  //
                               AstCCall* ثابت callp,  //
                               AstCFunc* ثابت calleep,  //
                               ثابت size_t seq_اڠک) {
        UINFO(6, "Inlining CFunc " << calleep->name() << " into " << callerp->name()
                                   << " at call site " << callp);

        Ast_نود_Stmt* ثابت callSitep = VN_AS(callp->backp(), Stmt_اوڠکاڤن);
        ++m_statCallsInlined;

        // Callee might be empty, just delete the call
        إذا (!calleep->stmtsp()) {
            VL_DO_DANGLING(تولق_Deletep(callSitep->unlinkFrBack()), callSitep);
            أرجع nullptr;
        }

        // Replace call site with a local scope
        File_باريس* ثابت flp = callSitep->fileline();
        AstCLocalScope* ثابت lscopep = جديد AstCLocalScope{flp, nullptr};
        callSitep->replaceWith(lscopep);
        VL_DO_DANGLING(تولق_Deletep(callSitep), callSitep);
        lscopep->addStmtsp(جديد AstComment{flp, "Inlined CFunc: " + calleep->name()});

        // Although it's in a local scope, we still make names of cloned locals unique
        ثابت std::string varPrefix
            = "__Vinline_" + std::to_string(seq_اڠک) + "_" + calleep->name() + "_";

        // AstVar::user2p()  ->  AstVar*, the cloned inlined local variable
        ثابت VNUser2InUse user2InUse;

        // Clone local variables, add them to the local scope
        لكل (AstVar* varp = calleep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            AstVar* ثابت newVarp = varp->clone_ڤوکوق(خطأ);
            newVarp->name(varPrefix + varp->name());
            lscopep->addStmtsp(newVarp);
            varp->user2p(newVarp);
        }

        // Clone the function body
        Ast_نود* ثابت bodyp = calleep->stmtsp()->clone_ڤوکوق(true);
        lscopep->addStmtsp(bodyp);

        // Retarget local variable references to the cloned locals
        // Rename locals defined in the body, TODO: there should be none after #6280
        // Reset vertex pointers on calls
        bodyp->foreachAndNext([&](Ast_نود* nodep) {
            إذا (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
                إذا (AstVar* ثابت varp = VN_AS(refp->varp()->user2p(), Var)) {
                    // Keep dtype; V3Expand may have widened the reference beyond the variable
                    Ast_نود_DType* ثابت dtypep = refp->dtypep();
                    refp->varp(varp);
                    refp->dtypep(dtypep);
                }
            } وإلا إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                varp->name(varPrefix + varp->name());
            } وإلا إذا (AstCCall* ثابت callp = VN_CAST(nodep, CCall)) {
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
            إذا (InlineCFuncsFunctionVertex* ثابت fVtxp = vtx.cast<InlineCFuncsFunctionVertex>()) {
                m_fVtxps.emplace_back(fVtxp);
            }
        }

        // Iterate functions leaf to root
        لكل (InlineCFuncsFunctionVertex* ثابت calleeVtxp : vlstd::reverse_view(m_fVtxps)) {
            // Should we inline this function?
            إذا (calleeVtxp->noInline()) استمر;  // Told not to

            // Check size heuristics
            ثابت منطقي doIt = [&]() {
                // Inline if small enough
                إذا (calleeVtxp->size() <= m_sizeThreshold) أرجع true;
                // Inline if not too much bloat
                ثابت size_t nCalls = calleeVtxp->inEdges().size();
                إذا (nCalls * calleeVtxp->size() <= m_prodThreshold) أرجع true;
                // Otherwise don't inline
                أرجع خطأ;
            }();
            إذا (!doIt) استمر;

            // Ok, attempt to inline call sites
            size_t nInlined = 0;
            لكل (ثابت V3GraphEdge* ثابت edgep : calleeVtxp->inEdges().unlinkable()) {
                InlineCFuncsCallSiteVertex* ثابت callVtxp
                    = edgep->fromp()->as<InlineCFuncsCallSiteVertex>();

                AstCFunc* ثابت calleep = calleeVtxp->cfuncp();
                AstCCall* ثابت callp = callVtxp->callp();
                UINFO(6, "Consider inlining " << calleep->name() << " at call site " << callp);
                // Should we inline this call site?
                إذا (callVtxp->noInline()) استمر;  // Told not to
                إذا (callVtxp->inEmpty()) استمر;  // Don't know where it's called from

                // Pick up the caller
                UASSERT_OBJ(callVtxp->inSize1(), callVtxp->callp(),
                            "Expected exactly one input edge for call site");
                InlineCFuncsFunctionVertex* ثابت callerVtxp
                    = callVtxp->inEdges().frontp()->fromp()->as<InlineCFuncsFunctionVertex>();
                AstCFunc* ثابت callerp = callerVtxp->cfuncp();

                // Don't make a function bigger than the limit
                ثابت size_t limit = callerp->isTrace() ? m_maxSizeTrace : m_maxSizeCFunc;
                إذا (callerVtxp->size() + calleeVtxp->size() > limit) استمر;

                // Can't do it if it's in a different scope, self pointers differ
                إذا (callerp->scopep() != calleep->scopep()) استمر;

                // Inline it
                إذا (!nInlined) ++m_statFuncsInlined;
                Ast_نود* ثابت inlinedp = inlineCall(callerp, callp, calleep, nInlined++);

                // Need to adjust the graph:
                // 1. Delete inlined call site
                VL_DO_DANGLING(callVtxp->unlinkDelete(&m_graph), callVtxp);
                // 2. Add new inlined call sites - also increments size of caller
                VL_RESTORER(m_cfuncVtxp);
                m_cfuncVtxp = callerVtxp;
                إذا (inlinedp) iterateChildrenConst(inlinedp);
            }
        }
    }

    فراغ removeUnusedFuncs() {
        // Iterate root to leaves
        لكل (V3GraphVertex* ثابت vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsFunctionVertex* ثابت fVtxp = vtxp->cast<InlineCFuncsFunctionVertex>();
            إذا (!fVtxp) استمر;
            // Keep if still called
            إذا (!fVtxp->inEmpty()) استمر;
            // Keep for other reasons
            إذا (fVtxp->keep()) استمر;

            AstCFunc* ثابت funcp = fVtxp->cfuncp();
            UINFO(6, "Removing unused CFunc " << funcp);
            ++m_statFuncsRemoved;

            // Unlink all call sites
            لكل (ثابت V3GraphEdge* ثابت edgep : vtxp->outEdges().unlinkable()) {
                edgep->اتس()->unlinkEdges(&m_graph);
            }
            // Delete function vertex
            vtxp->unlinkDelete(&m_graph);
            // Delete the function
            VL_DO_DANGLING(تولق_Deletep(funcp->unlinkFrBack()), funcp);
        }

        // Delete inlined/deleted call site vertices (for debugging only)
        لكل (V3GraphVertex* ثابت vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsCallSiteVertex* ثابت cVtxp = vtxp->cast<InlineCFuncsCallSiteVertex>();
            إذا (!cVtxp) استمر;
            إذا (!cVtxp->inEmpty()) استمر;
            إذا (!cVtxp->outEmpty()) استمر;
            vtxp->unlinkDelete(&m_graph);
        }
    }

    // VISITORS
    فراغ visit(AstCFunc* nodep) override {
        // Create the function vertex
        InlineCFuncsFunctionVertex* ثابت vtxp = getInlineCFuncsFunctionVertexp(nodep);

        // Check if the function itself is not inlineable
        إذا (nodep->rtnTypeVoid() != "void") vtxp->setNoInline("Not void");
        إذا (nodep->dpiImportPrototype()) vtxp->setNoInline("DPI import prototype");
        إذا (nodep->recursive()) vtxp->setNoInline("Recursive");
        إذا (nodep->argsp()) vtxp->setNoInline("Has arguments");
        إذا (nodep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Check if the function should not be removed
        إذا (nodep->entryPoint()) vtxp->setKeep("Entry point");
        إذا (nodep->dpiImportPrototype()) vtxp->setKeep("DPI import prototype");
        إذا (nodep->dpiExportDispatcher()) vtxp->setKeep("DPI export implementation");
        إذا (nodep->isVirtual()) vtxp->setKeep("Virtual method");

        // Iterate children
        VL_RESTORER(m_cfuncVtxp);
        m_cfuncVtxp = vtxp;
        iterateChildrenConst(nodep);
    }

    // Inlineable calls
    فراغ visit(AstCCall* nodep) override {
        إذا (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        AstCFunc* ثابت calleep = nodep->funcp();

        // Create the call site vertex
        InlineCFuncsCallSiteVertex* ثابت vtxp = getInlineCFuncsCallSiteVertexp(nodep);

        // Check if the call site is not inlineable
        إذا (!VN_IS(nodep->backp(), Stmt_اوڠکاڤن)) vtxp->setNoInline("Not in statement position");
        إذا (m_inExecGraph) vtxp->setNoInline("In ExecGraph");
        إذا (calleep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Add caller/callee edges
        إذا (m_cfuncVtxp) m_graph.addEdge(*m_cfuncVtxp, *vtxp);
        m_graph.addEdge(*vtxp, *getInlineCFuncsFunctionVertexp(calleep));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    فراغ visit(Ast_نود_CCall* nodep) override {
        إذا (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Called elsewhere");
        iterateChildrenConst(nodep);
    }

    فراغ visit(AstAddrOfCFunc* nodep) override {
        إذا (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Referenced by AddressOfCFunc");
        iterateChildrenConst(nodep);
    }

    // Nodes preventing inlining
    فراغ visit(AstTraceDecl* nodep) override {
        // Referenced by TraceInc
        إذا (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains TraceDecl");

        إذا (AstCCall* ثابت callp = nodep->dtypeCallp()) {
            getInlineCFuncsCallSiteVertexp(callp)->setNoInline("Referenced by TraceDecl");
        }
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstExecGraph* nodep) override {
        // AstExecGraph is not cloneable, so can't inline the containing function
        إذا (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains ExecGraph");
        // Also mark functions referenced in the dependency graph
        لكل (ثابت V3GraphVertex& vtx : nodep->depGraphp()->vertices()) {
            getInlineCFuncsFunctionVertexp(vtx.as<ExecMTask>()->funcp())
                ->setKeep("MTask function");
        }
        VL_RESTORER(m_inExecGraph);
        m_inExecGraph = true;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstCStmt* nodep) override {
        // Can reference anything in text
        إذا (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmt");
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstCExpr* nodep) override {
        // Can reference anything in text
        إذا (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExpr");
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstCStmtUser* nodep) override {
        // Can reference anything in text
        إذا (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmtUser");
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstCExprUser* nodep) override {
        // Can reference anything in text
        إذا (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExprUser");
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstCReturn* nodep) override {
        إذا (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CReturn");
        iterateChildrenConst(nodep);
    }

    // Base node
    فراغ visit(Ast_نود* nodep) override {
        إذا (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
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
        إذا (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-graph");
        // Phase 2: Inline calls
        doInlining();
        إذا (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-inlined");
        // Phase 3: Remove unused functions
        removeUnusedFuncs();
        إذا (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-kept");
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
    إذا (v3Global.opt.profCFuncs()) أرجع;
    { InlineCFuncsVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ڤوکوق("inlinecfuncs", 0, dump_ڤوکوق_Either_Level() >= 6);
}
