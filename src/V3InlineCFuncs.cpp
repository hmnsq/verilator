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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3InlineCFuncs.h"

#شامل "V3AstUserAllocator.h"
#شامل "V3ExecGraph.h"
#شامل "V3Graph.h"
#شامل "V3Stats.h"

#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Bipartite call graph containing function and call site vertices

رده InlineCFuncsFunctionVertex;
رده InlineCFuncsCallSiteVertex;

رده InlineCFuncsCallGraph final : عمومی V3Graph {
عمومی:
    InlineCFuncsCallGraph()
        : V3Graph{} {}
    ~InlineCFuncsCallGraph() override = پیشفرض;

    تهی addEdge(InlineCFuncsFunctionVertex& from, InlineCFuncsCallSiteVertex& بالاترین);
    تهی addEdge(InlineCFuncsCallSiteVertex& from, InlineCFuncsFunctionVertex& بالاترین);
};

رده EitherVertex VL_NOT_FINAL : عمومی V3GraphVertex {
    VL_RTTI_IMPL(EitherVertex, V3GraphVertex)
محافظتشده:
    explicit EitherVertex(InlineCFuncsCallGraph& graph)
        : V3GraphVertex{&graph} {}
};

رده InlineCFuncsFunctionVertex final : عمومی EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsFunctionVertex, EitherVertex)
    AstCFunc* ثابت m_cfuncp;  // The function
    ثابت نویسه* m_noInlineWyp = nullptr;  // First reason the function should not be inlined
    ثابت نویسه* m_keepWyp = nullptr;  // Why the function should not be removed
    size_t m_size = 0;  // The size of the function

عمومی:
    InlineCFuncsFunctionVertex(InlineCFuncsCallGraph& graph, AstCFunc* cfuncp)
        : EitherVertex{graph}
        , m_cfuncp{cfuncp} {}
    ~InlineCFuncsFunctionVertex() override = پیشفرض;

    // ACCESSORS
    AstCFunc* cfuncp() ثابت { بازگشت m_cfuncp; }
    size_t size() ثابت { بازگشت m_size; }
    تهی sizeInc(size_t مقدار = 1) { m_size += مقدار; }
    منطقی noInline() ثابت { بازگشت m_noInlineWyp; }
    تهی setNoInline(ثابت نویسه* whyp) {
        اگر (!m_noInlineWyp) m_noInlineWyp = whyp;
    }
    منطقی keep() ثابت { بازگشت m_keepWyp; }
    تهی setKeep(ثابت نویسه* whyp) {
        اگر (!m_keepWyp) m_keepWyp = whyp;
    }
    std::string dotColor() ثابت override {
        بازگشت m_noInlineWyp ? "red" : m_keepWyp ? "orange" : "black";
    }

    // debug
    File_سطر* fileline() ثابت override { بازگشت m_cfuncp->fileline(); }
    std::string dotShape() ثابت override { بازگشت "box"; }
    std::string name() ثابت override VL_MT_STABLE {
        std::string str = cvtToHex(m_cfuncp);
        str += "\n" + m_cfuncp->name();
        str += "\nsize: " + std::to_string(m_size);
        اگر (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        اگر (m_keepWyp) str += "\nKeep: "s + m_keepWyp;
        بازگشت str;
    }
};

رده InlineCFuncsCallSiteVertex final : عمومی EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsCallSiteVertex, EitherVertex)
    AstCCall* ثابت m_callp;  // The call site
    ثابت نویسه* m_noInlineWyp = nullptr;  // First reason the function should not be inlined

عمومی:
    InlineCFuncsCallSiteVertex(InlineCFuncsCallGraph& graph, AstCCall* callp)
        : EitherVertex{graph}
        , m_callp{callp} {}
    ~InlineCFuncsCallSiteVertex() override = پیشفرض;

    // ACCESSORS
    AstCCall* callp() ثابت { بازگشت m_callp; }
    منطقی noInline() ثابت { بازگشت m_noInlineWyp; }
    تهی setNoInline(ثابت نویسه* whyp) {
        اگر (!m_noInlineWyp) m_noInlineWyp = whyp;
    }

    // debug
    File_سطر* fileline() ثابت override { بازگشت m_callp->fileline(); }
    std::string dotColor() ثابت override { بازگشت m_noInlineWyp ? "red" : "black"; }
    std::string dotShape() ثابت override { بازگشت "ellipse"; }
    std::string name() ثابت override VL_MT_STABLE {
        std::string str = cvtToHex(m_callp);
        اگر (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        بازگشت str;
    }
};

تهی InlineCFuncsCallGraph::addEdge(InlineCFuncsFunctionVertex& caller,
                                    InlineCFuncsCallSiteVertex& callsite) {
    UASSERT_OBJ(callsite.inEmpty(), &callsite, "Call site should have at most one incoming edge");
    جدید V3GraphEdge{این, &caller, &callsite, 1, درست};  // Can cut caller -> callsite
}
تهی InlineCFuncsCallGraph::addEdge(InlineCFuncsCallSiteVertex& callsite,
                                    InlineCFuncsFunctionVertex& callee) {
    UASSERT_OBJ(callsite.outEmpty(), &callsite, "Call site should have at most one outgoing edge");
    جدید V3GraphEdge{این, &callsite, &callee, 1, نادرست};
}

//######################################################################

رده InlineCFuncsVisitor final : عمومی VNVisitor {
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
        صحیح maxCFunc = v3Global.opt.outputSplitCFuncs();
        صحیح maxFile = v3Global.opt.outputSplit();
        اگر (maxCFunc <= 0) maxCFunc = std::numeric_limits<صحیح>::max();
        اگر (maxFile <= 0) maxFile = std::numeric_limits<صحیح>::max();
        بازگشت std::min(maxCFunc, maxFile);
    }();
    ثابت size_t m_maxSizeTrace = []() -> size_t {
        صحیح maxTrace = v3Global.opt.outputSplitCTrace();
        صحیح maxFile = v3Global.opt.outputSplit();
        اگر (maxTrace <= 0) maxTrace = std::numeric_limits<صحیح>::max();
        اگر (maxFile <= 0) maxFile = std::numeric_limits<صحیح>::max();
        بازگشت std::min(maxTrace, maxFile);
    }();
    InlineCFuncsFunctionVertex* m_cfuncVtxp = nullptr;  // Vertex of currently iterated function
    منطقی m_inExecGraph = نادرست;  // True while inside an AstExecGraph subtree

    // METHODS
    InlineCFuncsFunctionVertex* getInlineCFuncsFunctionVertexp(AstCFunc* cfuncp) {
        اگر (!cfuncp->user1p()) cfuncp->user1p(جدید InlineCFuncsFunctionVertex{m_graph, cfuncp});
        بازگشت cfuncp->user1u().to<InlineCFuncsFunctionVertex*>();
    }

    InlineCFuncsCallSiteVertex* getInlineCFuncsCallSiteVertexp(AstCCall* callp) {
        اگر (!callp->user1p()) callp->user1p(جدید InlineCFuncsCallSiteVertex{m_graph, callp});
        بازگشت callp->user1u().to<InlineCFuncsCallSiteVertex*>();
    }

    AstCLocalScope* inlineCall(AstCFunc* ثابت callerp,  //
                               AstCCall* ثابت callp,  //
                               AstCFunc* ثابت calleep,  //
                               ثابت size_t seq_رقم) {
        UINFO(6, "Inlining CFunc " << calleep->name() << " into " << callerp->name()
                                   << " at call site " << callp);

        Ast_گره_Stmt* ثابت callSitep = VN_AS(callp->backp(), Stmt_عبارت);
        ++m_statCallsInlined;

        // Callee might be empty, just delete the call
        اگر (!calleep->stmtsp()) {
            VL_DO_DANGLING(افزودن_Deletep(callSitep->unlinkFrBack()), callSitep);
            بازگشت nullptr;
        }

        // Replace call site with a local scope
        File_سطر* ثابت flp = callSitep->fileline();
        AstCLocalScope* ثابت lscopep = جدید AstCLocalScope{flp, nullptr};
        callSitep->replaceWith(lscopep);
        VL_DO_DANGLING(افزودن_Deletep(callSitep), callSitep);
        lscopep->addStmtsp(جدید AstComment{flp, "Inlined CFunc: " + calleep->name()});

        // Although it's in a local scope, we still make names of cloned locals unique
        ثابت std::string varPrefix
            = "__Vinline_" + std::to_string(seq_رقم) + "_" + calleep->name() + "_";

        // AstVar::user2p()  ->  AstVar*, the cloned inlined local variable
        ثابت VNUser2InUse user2InUse;

        // Clone local variables, add them to the local scope
        برای (AstVar* varp = calleep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            AstVar* ثابت newVarp = varp->clone_درخت(نادرست);
            newVarp->name(varPrefix + varp->name());
            lscopep->addStmtsp(newVarp);
            varp->user2p(newVarp);
        }

        // Clone the function body
        Ast_گره* ثابت bodyp = calleep->stmtsp()->clone_درخت(درست);
        lscopep->addStmtsp(bodyp);

        // Retarget local variable references to the cloned locals
        // Rename locals defined in the body, TODO: there should be none after #6280
        // Reset vertex pointers on calls
        bodyp->foreachAndNext([&](Ast_گره* nodep) {
            اگر (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
                اگر (AstVar* ثابت varp = VN_AS(refp->varp()->user2p(), Var)) {
                    // Keep dtype; V3Expand may have widened the reference beyond the variable
                    Ast_گره_DType* ثابت dtypep = refp->dtypep();
                    refp->varp(varp);
                    refp->dtypep(dtypep);
                }
            } وگرنه اگر (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                varp->name(varPrefix + varp->name());
            } وگرنه اگر (AstCCall* ثابت callp = VN_CAST(nodep, CCall)) {
                callp->user1p(nullptr);
            }
        });

        // Return the local scope
        بازگشت lscopep;
    }

    تهی doInlining() {
        // Need to gather vertices as we are changing the graph
        std::vector<InlineCFuncsFunctionVertex*> m_fVtxps;
        برای (V3GraphVertex& vtx : m_graph.vertices()) {
            اگر (InlineCFuncsFunctionVertex* ثابت fVtxp = vtx.cast<InlineCFuncsFunctionVertex>()) {
                m_fVtxps.emplace_back(fVtxp);
            }
        }

        // Iterate functions leaf to root
        برای (InlineCFuncsFunctionVertex* ثابت calleeVtxp : vlstd::reverse_view(m_fVtxps)) {
            // Should we inline this function?
            اگر (calleeVtxp->noInline()) ادامه;  // Told not to

            // Check size heuristics
            ثابت منطقی doIt = [&]() {
                // Inline if small enough
                اگر (calleeVtxp->size() <= m_sizeThreshold) بازگشت درست;
                // Inline if not too much bloat
                ثابت size_t nCalls = calleeVtxp->inEdges().size();
                اگر (nCalls * calleeVtxp->size() <= m_prodThreshold) بازگشت درست;
                // Otherwise don't inline
                بازگشت نادرست;
            }();
            اگر (!doIt) ادامه;

            // Ok, attempt to inline call sites
            size_t nInlined = 0;
            برای (ثابت V3GraphEdge* ثابت edgep : calleeVtxp->inEdges().unlinkable()) {
                InlineCFuncsCallSiteVertex* ثابت callVtxp
                    = edgep->fromp()->as<InlineCFuncsCallSiteVertex>();

                AstCFunc* ثابت calleep = calleeVtxp->cfuncp();
                AstCCall* ثابت callp = callVtxp->callp();
                UINFO(6, "Consider inlining " << calleep->name() << " at call site " << callp);
                // Should we inline this call site?
                اگر (callVtxp->noInline()) ادامه;  // Told not to
                اگر (callVtxp->inEmpty()) ادامه;  // Don't know where it's called from

                // Pick up the caller
                UASSERT_OBJ(callVtxp->inSize1(), callVtxp->callp(),
                            "Expected exactly one input edge for call site");
                InlineCFuncsFunctionVertex* ثابت callerVtxp
                    = callVtxp->inEdges().frontp()->fromp()->as<InlineCFuncsFunctionVertex>();
                AstCFunc* ثابت callerp = callerVtxp->cfuncp();

                // Don't make a function bigger than the limit
                ثابت size_t limit = callerp->isTrace() ? m_maxSizeTrace : m_maxSizeCFunc;
                اگر (callerVtxp->size() + calleeVtxp->size() > limit) ادامه;

                // Can't do it if it's in a different scope, self pointers differ
                اگر (callerp->scopep() != calleep->scopep()) ادامه;

                // Inline it
                اگر (!nInlined) ++m_statFuncsInlined;
                Ast_گره* ثابت inlinedp = inlineCall(callerp, callp, calleep, nInlined++);

                // Need to adjust the graph:
                // 1. Delete inlined call site
                VL_DO_DANGLING(callVtxp->unlinkDelete(&m_graph), callVtxp);
                // 2. Add new inlined call sites - also increments size of caller
                VL_RESTORER(m_cfuncVtxp);
                m_cfuncVtxp = callerVtxp;
                اگر (inlinedp) iterateChildrenConst(inlinedp);
            }
        }
    }

    تهی removeUnusedFuncs() {
        // Iterate root to leaves
        برای (V3GraphVertex* ثابت vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsFunctionVertex* ثابت fVtxp = vtxp->cast<InlineCFuncsFunctionVertex>();
            اگر (!fVtxp) ادامه;
            // Keep if still called
            اگر (!fVtxp->inEmpty()) ادامه;
            // Keep for other reasons
            اگر (fVtxp->keep()) ادامه;

            AstCFunc* ثابت funcp = fVtxp->cfuncp();
            UINFO(6, "Removing unused CFunc " << funcp);
            ++m_statFuncsRemoved;

            // Unlink all call sites
            برای (ثابت V3GraphEdge* ثابت edgep : vtxp->outEdges().unlinkable()) {
                edgep->بالاترین()->unlinkEdges(&m_graph);
            }
            // Delete function vertex
            vtxp->unlinkDelete(&m_graph);
            // Delete the function
            VL_DO_DANGLING(افزودن_Deletep(funcp->unlinkFrBack()), funcp);
        }

        // Delete inlined/deleted call site vertices (for debugging only)
        برای (V3GraphVertex* ثابت vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsCallSiteVertex* ثابت cVtxp = vtxp->cast<InlineCFuncsCallSiteVertex>();
            اگر (!cVtxp) ادامه;
            اگر (!cVtxp->inEmpty()) ادامه;
            اگر (!cVtxp->outEmpty()) ادامه;
            vtxp->unlinkDelete(&m_graph);
        }
    }

    // VISITORS
    تهی visit(AstCFunc* nodep) override {
        // Create the function vertex
        InlineCFuncsFunctionVertex* ثابت vtxp = getInlineCFuncsFunctionVertexp(nodep);

        // Check if the function itself is not inlineable
        اگر (nodep->rtnTypeVoid() != "void") vtxp->setNoInline("Not void");
        اگر (nodep->dpiImportPrototype()) vtxp->setNoInline("DPI import prototype");
        اگر (nodep->recursive()) vtxp->setNoInline("Recursive");
        اگر (nodep->argsp()) vtxp->setNoInline("Has arguments");
        اگر (nodep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Check if the function should not be removed
        اگر (nodep->entryPoint()) vtxp->setKeep("Entry point");
        اگر (nodep->dpiImportPrototype()) vtxp->setKeep("DPI import prototype");
        اگر (nodep->dpiExportDispatcher()) vtxp->setKeep("DPI export implementation");
        اگر (nodep->isVirtual()) vtxp->setKeep("Virtual method");

        // Iterate children
        VL_RESTORER(m_cfuncVtxp);
        m_cfuncVtxp = vtxp;
        iterateChildrenConst(nodep);
    }

    // Inlineable calls
    تهی visit(AstCCall* nodep) override {
        اگر (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        AstCFunc* ثابت calleep = nodep->funcp();

        // Create the call site vertex
        InlineCFuncsCallSiteVertex* ثابت vtxp = getInlineCFuncsCallSiteVertexp(nodep);

        // Check if the call site is not inlineable
        اگر (!VN_IS(nodep->backp(), Stmt_عبارت)) vtxp->setNoInline("Not in statement position");
        اگر (m_inExecGraph) vtxp->setNoInline("In ExecGraph");
        اگر (calleep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Add caller/callee edges
        اگر (m_cfuncVtxp) m_graph.addEdge(*m_cfuncVtxp, *vtxp);
        m_graph.addEdge(*vtxp, *getInlineCFuncsFunctionVertexp(calleep));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    تهی visit(Ast_گره_CCall* nodep) override {
        اگر (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Called elsewhere");
        iterateChildrenConst(nodep);
    }

    تهی visit(AstAddrOfCFunc* nodep) override {
        اگر (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Referenced by AddressOfCFunc");
        iterateChildrenConst(nodep);
    }

    // Nodes preventing inlining
    تهی visit(AstTraceDecl* nodep) override {
        // Referenced by TraceInc
        اگر (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains TraceDecl");

        اگر (AstCCall* ثابت callp = nodep->dtypeCallp()) {
            getInlineCFuncsCallSiteVertexp(callp)->setNoInline("Referenced by TraceDecl");
        }
        iterateChildrenConst(nodep);
    }
    تهی visit(AstExecGraph* nodep) override {
        // AstExecGraph is not cloneable, so can't inline the containing function
        اگر (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains ExecGraph");
        // Also mark functions referenced in the dependency graph
        برای (ثابت V3GraphVertex& vtx : nodep->depGraphp()->vertices()) {
            getInlineCFuncsFunctionVertexp(vtx.as<ExecMTask>()->funcp())
                ->setKeep("MTask function");
        }
        VL_RESTORER(m_inExecGraph);
        m_inExecGraph = درست;
        iterateChildrenConst(nodep);
    }
    تهی visit(AstCStmt* nodep) override {
        // Can reference anything in text
        اگر (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmt");
        iterateChildrenConst(nodep);
    }
    تهی visit(AstCExpr* nodep) override {
        // Can reference anything in text
        اگر (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExpr");
        iterateChildrenConst(nodep);
    }
    تهی visit(AstCStmtUser* nodep) override {
        // Can reference anything in text
        اگر (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmtUser");
        iterateChildrenConst(nodep);
    }
    تهی visit(AstCExprUser* nodep) override {
        // Can reference anything in text
        اگر (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExprUser");
        iterateChildrenConst(nodep);
    }
    تهی visit(AstCReturn* nodep) override {
        اگر (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CReturn");
        iterateChildrenConst(nodep);
    }

    // Base node
    تهی visit(Ast_گره* nodep) override {
        اگر (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        iterateChildrenConst(nodep);
    }

عمومی:
    // CONSTRUCTORS
    explicit InlineCFuncsVisitor(AstNetlist* nodep) {
        // Phase 1: Build call graph
        iterateConst(nodep);
        // Make acyclic in case there is recursion
        m_graph.acyclic(V3GraphEdge::followAlwaysTrue);
        // Order vertices (any topological order is fine)
        m_graph.order();
        اگر (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-graph");
        // Phase 2: Inline calls
        doInlining();
        اگر (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-inlined");
        // Phase 3: Remove unused functions
        removeUnusedFuncs();
        اگر (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-kept");
    }
    ~InlineCFuncsVisitor() override {
        V3Stats::addStat("Optimizations, Inline CFuncs, calls inlined", m_statCallsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions inlined", m_statFuncsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions removed", m_statFuncsRemoved);
    }
};

//######################################################################
// InlineCFuncs class functions

تهی V3InlineCFuncs::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    // Don't inline when profiling per-function (it would lose granularity)
    اگر (v3Global.opt.profCFuncs()) بازگشت;
    { InlineCFuncsVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("inlinecfuncs", 0, dump_درخت_Either_Level() >= 6);
}
