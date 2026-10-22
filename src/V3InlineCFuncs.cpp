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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3InlineCFuncs.h"

#അന്തര്ഭാവ "V3AstUserAllocator.h"
#അന്തര്ഭാവ "V3ExecGraph.h"
#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Bipartite call graph containing function and call site vertices

വർഗ്ഗം InlineCFuncsFunctionVertex;
വർഗ്ഗം InlineCFuncsCallSiteVertex;

വർഗ്ഗം InlineCFuncsCallGraph final : പൊതു V3Graph {
പൊതു:
    InlineCFuncsCallGraph()
        : V3Graph{} {}
    ~InlineCFuncsCallGraph() override = സ്ഥിരസ്ഥിതി;

    ശൂന്യം addEdge(InlineCFuncsFunctionVertex& from, InlineCFuncsCallSiteVertex& മുകൾ);
    ശൂന്യം addEdge(InlineCFuncsCallSiteVertex& from, InlineCFuncsFunctionVertex& മുകൾ);
};

വർഗ്ഗം EitherVertex VL_NOT_FINAL : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(EitherVertex, V3GraphVertex)
സംരക്ഷിത:
    explicit EitherVertex(InlineCFuncsCallGraph& graph)
        : V3GraphVertex{&graph} {}
};

വർഗ്ഗം InlineCFuncsFunctionVertex final : പൊതു EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsFunctionVertex, EitherVertex)
    AstCFunc* സ്ഥിരം m_cfuncp;  // The function
    സ്ഥിരം അക്ഷരം* m_noInlineWyp = nullptr;  // First reason the function should not be inlined
    സ്ഥിരം അക്ഷരം* m_keepWyp = nullptr;  // Why the function should not be removed
    size_t m_size = 0;  // The size of the function

പൊതു:
    InlineCFuncsFunctionVertex(InlineCFuncsCallGraph& graph, AstCFunc* cfuncp)
        : EitherVertex{graph}
        , m_cfuncp{cfuncp} {}
    ~InlineCFuncsFunctionVertex() override = സ്ഥിരസ്ഥിതി;

    // ACCESSORS
    AstCFunc* cfuncp() സ്ഥിരം { തിരികെ m_cfuncp; }
    size_t size() സ്ഥിരം { തിരികെ m_size; }
    ശൂന്യം sizeInc(size_t മൂല്യം = 1) { m_size += മൂല്യം; }
    തർക്കം noInline() സ്ഥിരം { തിരികെ m_noInlineWyp; }
    ശൂന്യം setNoInline(സ്ഥിരം അക്ഷരം* whyp) {
        എങ്കിൽ (!m_noInlineWyp) m_noInlineWyp = whyp;
    }
    തർക്കം keep() സ്ഥിരം { തിരികെ m_keepWyp; }
    ശൂന്യം setKeep(സ്ഥിരം അക്ഷരം* whyp) {
        എങ്കിൽ (!m_keepWyp) m_keepWyp = whyp;
    }
    std::string dotColor() സ്ഥിരം override {
        തിരികെ m_noInlineWyp ? "red" : m_keepWyp ? "orange" : "black";
    }

    // debug
    File_വരി* fileline() സ്ഥിരം override { തിരികെ m_cfuncp->fileline(); }
    std::string dotShape() സ്ഥിരം override { തിരികെ "box"; }
    std::string name() സ്ഥിരം override VL_MT_STABLE {
        std::string str = cvtToHex(m_cfuncp);
        str += "\n" + m_cfuncp->name();
        str += "\nsize: " + std::to_string(m_size);
        എങ്കിൽ (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        എങ്കിൽ (m_keepWyp) str += "\nKeep: "s + m_keepWyp;
        തിരികെ str;
    }
};

വർഗ്ഗം InlineCFuncsCallSiteVertex final : പൊതു EitherVertex {
    VL_RTTI_IMPL(InlineCFuncsCallSiteVertex, EitherVertex)
    AstCCall* സ്ഥിരം m_callp;  // The call site
    സ്ഥിരം അക്ഷരം* m_noInlineWyp = nullptr;  // First reason the function should not be inlined

പൊതു:
    InlineCFuncsCallSiteVertex(InlineCFuncsCallGraph& graph, AstCCall* callp)
        : EitherVertex{graph}
        , m_callp{callp} {}
    ~InlineCFuncsCallSiteVertex() override = സ്ഥിരസ്ഥിതി;

    // ACCESSORS
    AstCCall* callp() സ്ഥിരം { തിരികെ m_callp; }
    തർക്കം noInline() സ്ഥിരം { തിരികെ m_noInlineWyp; }
    ശൂന്യം setNoInline(സ്ഥിരം അക്ഷരം* whyp) {
        എങ്കിൽ (!m_noInlineWyp) m_noInlineWyp = whyp;
    }

    // debug
    File_വരി* fileline() സ്ഥിരം override { തിരികെ m_callp->fileline(); }
    std::string dotColor() സ്ഥിരം override { തിരികെ m_noInlineWyp ? "red" : "black"; }
    std::string dotShape() സ്ഥിരം override { തിരികെ "ellipse"; }
    std::string name() സ്ഥിരം override VL_MT_STABLE {
        std::string str = cvtToHex(m_callp);
        എങ്കിൽ (m_noInlineWyp) str += "\nNoInline: "s + m_noInlineWyp;
        തിരികെ str;
    }
};

ശൂന്യം InlineCFuncsCallGraph::addEdge(InlineCFuncsFunctionVertex& caller,
                                    InlineCFuncsCallSiteVertex& callsite) {
    UASSERT_OBJ(callsite.inEmpty(), &callsite, "Call site should have at most one incoming edge");
    പുതിയ V3GraphEdge{ഇത്, &caller, &callsite, 1, സത്യം};  // Can cut caller -> callsite
}
ശൂന്യം InlineCFuncsCallGraph::addEdge(InlineCFuncsCallSiteVertex& callsite,
                                    InlineCFuncsFunctionVertex& callee) {
    UASSERT_OBJ(callsite.outEmpty(), &callsite, "Call site should have at most one outgoing edge");
    പുതിയ V3GraphEdge{ഇത്, &callsite, &callee, 1, അസത്യം};
}

//######################################################################

വർഗ്ഗം InlineCFuncsVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // AstCFunc::user1p() ->  InlineCFuncsFunctionVertex*, the function vertex
    // AstCCall::user1p() ->  InlineCFuncsCallSiteVertex*, the call site vertex
    // AstVar::user2p()   ->  AstVar*, the cloned inlined local variable
    സ്ഥിരം VNUser1InUse m_user1InUse;

    // STATE
    InlineCFuncsCallGraph m_graph;  // The call graph
    VDouble0 m_statCallsInlined;  // Number of calls inlined
    VDouble0 m_statFuncsInlined;  // Number of functions inlined at least once
    VDouble0 m_statFuncsRemoved;  // Number of fully-inlined functions removed
    // Size threshold: always inline if size <= this
    സ്ഥിരം size_t m_sizeThreshold = v3Global.opt.inlineCFuncs();
    // Product threshold: inline if size * calls <= this
    സ്ഥിരം size_t m_prodThreshold = v3Global.opt.inlineCFuncsProduct();
    // Maximum size of caller to consider inlining into
    സ്ഥിരം size_t m_maxSizeCFunc = []() -> size_t {
        പൂർണ്ണസംഖ്യ maxCFunc = v3Global.opt.outputSplitCFuncs();
        പൂർണ്ണസംഖ്യ maxFile = v3Global.opt.outputSplit();
        എങ്കിൽ (maxCFunc <= 0) maxCFunc = std::numeric_limits<പൂർണ്ണസംഖ്യ>::max();
        എങ്കിൽ (maxFile <= 0) maxFile = std::numeric_limits<പൂർണ്ണസംഖ്യ>::max();
        തിരികെ std::min(maxCFunc, maxFile);
    }();
    സ്ഥിരം size_t m_maxSizeTrace = []() -> size_t {
        പൂർണ്ണസംഖ്യ maxTrace = v3Global.opt.outputSplitCTrace();
        പൂർണ്ണസംഖ്യ maxFile = v3Global.opt.outputSplit();
        എങ്കിൽ (maxTrace <= 0) maxTrace = std::numeric_limits<പൂർണ്ണസംഖ്യ>::max();
        എങ്കിൽ (maxFile <= 0) maxFile = std::numeric_limits<പൂർണ്ണസംഖ്യ>::max();
        തിരികെ std::min(maxTrace, maxFile);
    }();
    InlineCFuncsFunctionVertex* m_cfuncVtxp = nullptr;  // Vertex of currently iterated function
    തർക്കം m_inExecGraph = അസത്യം;  // True while inside an AstExecGraph subtree

    // METHODS
    InlineCFuncsFunctionVertex* getInlineCFuncsFunctionVertexp(AstCFunc* cfuncp) {
        എങ്കിൽ (!cfuncp->user1p()) cfuncp->user1p(പുതിയ InlineCFuncsFunctionVertex{m_graph, cfuncp});
        തിരികെ cfuncp->user1u().to<InlineCFuncsFunctionVertex*>();
    }

    InlineCFuncsCallSiteVertex* getInlineCFuncsCallSiteVertexp(AstCCall* callp) {
        എങ്കിൽ (!callp->user1p()) callp->user1p(പുതിയ InlineCFuncsCallSiteVertex{m_graph, callp});
        തിരികെ callp->user1u().to<InlineCFuncsCallSiteVertex*>();
    }

    AstCLocalScope* inlineCall(AstCFunc* സ്ഥിരം callerp,  //
                               AstCCall* സ്ഥിരം callp,  //
                               AstCFunc* സ്ഥിരം calleep,  //
                               സ്ഥിരം size_t seq_അക്കം) {
        UINFO(6, "Inlining CFunc " << calleep->name() << " into " << callerp->name()
                                   << " at call site " << callp);

        Ast_കണ്ണി_Stmt* സ്ഥിരം callSitep = VN_AS(callp->backp(), Stmt_വ്യഞ്ജകം);
        ++m_statCallsInlined;

        // Callee might be empty, just delete the call
        എങ്കിൽ (!calleep->stmtsp()) {
            VL_DO_DANGLING(തള്ളുക_Deletep(callSitep->unlinkFrBack()), callSitep);
            തിരികെ nullptr;
        }

        // Replace call site with a local scope
        File_വരി* സ്ഥിരം flp = callSitep->fileline();
        AstCLocalScope* സ്ഥിരം lscopep = പുതിയ AstCLocalScope{flp, nullptr};
        callSitep->replaceWith(lscopep);
        VL_DO_DANGLING(തള്ളുക_Deletep(callSitep), callSitep);
        lscopep->addStmtsp(പുതിയ AstComment{flp, "Inlined CFunc: " + calleep->name()});

        // Although it's in a local scope, we still make names of cloned locals unique
        സ്ഥിരം std::string varPrefix
            = "__Vinline_" + std::to_string(seq_അക്കം) + "_" + calleep->name() + "_";

        // AstVar::user2p()  ->  AstVar*, the cloned inlined local variable
        സ്ഥിരം VNUser2InUse user2InUse;

        // Clone local variables, add them to the local scope
        വേണ്ടി (AstVar* varp = calleep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            AstVar* സ്ഥിരം newVarp = varp->clone_വൃക്ഷം(അസത്യം);
            newVarp->name(varPrefix + varp->name());
            lscopep->addStmtsp(newVarp);
            varp->user2p(newVarp);
        }

        // Clone the function body
        Ast_കണ്ണി* സ്ഥിരം bodyp = calleep->stmtsp()->clone_വൃക്ഷം(സത്യം);
        lscopep->addStmtsp(bodyp);

        // Retarget local variable references to the cloned locals
        // Rename locals defined in the body, TODO: there should be none after #6280
        // Reset vertex pointers on calls
        bodyp->foreachAndNext([&](Ast_കണ്ണി* nodep) {
            എങ്കിൽ (AstVarRef* സ്ഥിരം refp = VN_CAST(nodep, VarRef)) {
                എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_AS(refp->varp()->user2p(), Var)) {
                    // Keep dtype; V3Expand may have widened the reference beyond the variable
                    Ast_കണ്ണി_DType* സ്ഥിരം dtypep = refp->dtypep();
                    refp->varp(varp);
                    refp->dtypep(dtypep);
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var)) {
                varp->name(varPrefix + varp->name());
            } അല്ലെങ്കിൽ എങ്കിൽ (AstCCall* സ്ഥിരം callp = VN_CAST(nodep, CCall)) {
                callp->user1p(nullptr);
            }
        });

        // Return the local scope
        തിരികെ lscopep;
    }

    ശൂന്യം doInlining() {
        // Need to gather vertices as we are changing the graph
        std::vector<InlineCFuncsFunctionVertex*> m_fVtxps;
        വേണ്ടി (V3GraphVertex& vtx : m_graph.vertices()) {
            എങ്കിൽ (InlineCFuncsFunctionVertex* സ്ഥിരം fVtxp = vtx.cast<InlineCFuncsFunctionVertex>()) {
                m_fVtxps.emplace_back(fVtxp);
            }
        }

        // Iterate functions leaf to root
        വേണ്ടി (InlineCFuncsFunctionVertex* സ്ഥിരം calleeVtxp : vlstd::reverse_view(m_fVtxps)) {
            // Should we inline this function?
            എങ്കിൽ (calleeVtxp->noInline()) തുടരുക;  // Told not to

            // Check size heuristics
            സ്ഥിരം തർക്കം doIt = [&]() {
                // Inline if small enough
                എങ്കിൽ (calleeVtxp->size() <= m_sizeThreshold) തിരികെ സത്യം;
                // Inline if not too much bloat
                സ്ഥിരം size_t nCalls = calleeVtxp->inEdges().size();
                എങ്കിൽ (nCalls * calleeVtxp->size() <= m_prodThreshold) തിരികെ സത്യം;
                // Otherwise don't inline
                തിരികെ അസത്യം;
            }();
            എങ്കിൽ (!doIt) തുടരുക;

            // Ok, attempt to inline call sites
            size_t nInlined = 0;
            വേണ്ടി (സ്ഥിരം V3GraphEdge* സ്ഥിരം edgep : calleeVtxp->inEdges().unlinkable()) {
                InlineCFuncsCallSiteVertex* സ്ഥിരം callVtxp
                    = edgep->fromp()->as<InlineCFuncsCallSiteVertex>();

                AstCFunc* സ്ഥിരം calleep = calleeVtxp->cfuncp();
                AstCCall* സ്ഥിരം callp = callVtxp->callp();
                UINFO(6, "Consider inlining " << calleep->name() << " at call site " << callp);
                // Should we inline this call site?
                എങ്കിൽ (callVtxp->noInline()) തുടരുക;  // Told not to
                എങ്കിൽ (callVtxp->inEmpty()) തുടരുക;  // Don't know where it's called from

                // Pick up the caller
                UASSERT_OBJ(callVtxp->inSize1(), callVtxp->callp(),
                            "Expected exactly one input edge for call site");
                InlineCFuncsFunctionVertex* സ്ഥിരം callerVtxp
                    = callVtxp->inEdges().frontp()->fromp()->as<InlineCFuncsFunctionVertex>();
                AstCFunc* സ്ഥിരം callerp = callerVtxp->cfuncp();

                // Don't make a function bigger than the limit
                സ്ഥിരം size_t limit = callerp->isTrace() ? m_maxSizeTrace : m_maxSizeCFunc;
                എങ്കിൽ (callerVtxp->size() + calleeVtxp->size() > limit) തുടരുക;

                // Can't do it if it's in a different scope, self pointers differ
                എങ്കിൽ (callerp->scopep() != calleep->scopep()) തുടരുക;

                // Inline it
                എങ്കിൽ (!nInlined) ++m_statFuncsInlined;
                Ast_കണ്ണി* സ്ഥിരം inlinedp = inlineCall(callerp, callp, calleep, nInlined++);

                // Need to adjust the graph:
                // 1. Delete inlined call site
                VL_DO_DANGLING(callVtxp->unlinkDelete(&m_graph), callVtxp);
                // 2. Add new inlined call sites - also increments size of caller
                VL_RESTORER(m_cfuncVtxp);
                m_cfuncVtxp = callerVtxp;
                എങ്കിൽ (inlinedp) iterateChildrenConst(inlinedp);
            }
        }
    }

    ശൂന്യം removeUnusedFuncs() {
        // Iterate root to leaves
        വേണ്ടി (V3GraphVertex* സ്ഥിരം vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsFunctionVertex* സ്ഥിരം fVtxp = vtxp->cast<InlineCFuncsFunctionVertex>();
            എങ്കിൽ (!fVtxp) തുടരുക;
            // Keep if still called
            എങ്കിൽ (!fVtxp->inEmpty()) തുടരുക;
            // Keep for other reasons
            എങ്കിൽ (fVtxp->keep()) തുടരുക;

            AstCFunc* സ്ഥിരം funcp = fVtxp->cfuncp();
            UINFO(6, "Removing unused CFunc " << funcp);
            ++m_statFuncsRemoved;

            // Unlink all call sites
            വേണ്ടി (സ്ഥിരം V3GraphEdge* സ്ഥിരം edgep : vtxp->outEdges().unlinkable()) {
                edgep->മുകൾ()->unlinkEdges(&m_graph);
            }
            // Delete function vertex
            vtxp->unlinkDelete(&m_graph);
            // Delete the function
            VL_DO_DANGLING(തള്ളുക_Deletep(funcp->unlinkFrBack()), funcp);
        }

        // Delete inlined/deleted call site vertices (for debugging only)
        വേണ്ടി (V3GraphVertex* സ്ഥിരം vtxp : m_graph.vertices().unlinkable()) {
            InlineCFuncsCallSiteVertex* സ്ഥിരം cVtxp = vtxp->cast<InlineCFuncsCallSiteVertex>();
            എങ്കിൽ (!cVtxp) തുടരുക;
            എങ്കിൽ (!cVtxp->inEmpty()) തുടരുക;
            എങ്കിൽ (!cVtxp->outEmpty()) തുടരുക;
            vtxp->unlinkDelete(&m_graph);
        }
    }

    // VISITORS
    ശൂന്യം visit(AstCFunc* nodep) override {
        // Create the function vertex
        InlineCFuncsFunctionVertex* സ്ഥിരം vtxp = getInlineCFuncsFunctionVertexp(nodep);

        // Check if the function itself is not inlineable
        എങ്കിൽ (nodep->rtnTypeVoid() != "void") vtxp->setNoInline("Not void");
        എങ്കിൽ (nodep->dpiImportPrototype()) vtxp->setNoInline("DPI import prototype");
        എങ്കിൽ (nodep->recursive()) vtxp->setNoInline("Recursive");
        എങ്കിൽ (nodep->argsp()) vtxp->setNoInline("Has arguments");
        എങ്കിൽ (nodep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Check if the function should not be removed
        എങ്കിൽ (nodep->entryPoint()) vtxp->setKeep("Entry point");
        എങ്കിൽ (nodep->dpiImportPrototype()) vtxp->setKeep("DPI import prototype");
        എങ്കിൽ (nodep->dpiExportDispatcher()) vtxp->setKeep("DPI export implementation");
        എങ്കിൽ (nodep->isVirtual()) vtxp->setKeep("Virtual method");

        // Iterate children
        VL_RESTORER(m_cfuncVtxp);
        m_cfuncVtxp = vtxp;
        iterateChildrenConst(nodep);
    }

    // Inlineable calls
    ശൂന്യം visit(AstCCall* nodep) override {
        എങ്കിൽ (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        AstCFunc* സ്ഥിരം calleep = nodep->funcp();

        // Create the call site vertex
        InlineCFuncsCallSiteVertex* സ്ഥിരം vtxp = getInlineCFuncsCallSiteVertexp(nodep);

        // Check if the call site is not inlineable
        എങ്കിൽ (!VN_IS(nodep->backp(), Stmt_വ്യഞ്ജകം)) vtxp->setNoInline("Not in statement position");
        എങ്കിൽ (m_inExecGraph) vtxp->setNoInline("In ExecGraph");
        എങ്കിൽ (calleep->isVirtual()) vtxp->setNoInline("Virtual method");

        // Add caller/callee edges
        എങ്കിൽ (m_cfuncVtxp) m_graph.addEdge(*m_cfuncVtxp, *vtxp);
        m_graph.addEdge(*vtxp, *getInlineCFuncsFunctionVertexp(calleep));

        // Iterate children
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി_CCall* nodep) override {
        എങ്കിൽ (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Called elsewhere");
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(AstAddrOfCFunc* nodep) override {
        എങ്കിൽ (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        getInlineCFuncsFunctionVertexp(nodep->funcp())->setKeep("Referenced by AddressOfCFunc");
        iterateChildrenConst(nodep);
    }

    // Nodes preventing inlining
    ശൂന്യം visit(AstTraceDecl* nodep) override {
        // Referenced by TraceInc
        എങ്കിൽ (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains TraceDecl");

        എങ്കിൽ (AstCCall* സ്ഥിരം callp = nodep->dtypeCallp()) {
            getInlineCFuncsCallSiteVertexp(callp)->setNoInline("Referenced by TraceDecl");
        }
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstExecGraph* nodep) override {
        // AstExecGraph is not cloneable, so can't inline the containing function
        എങ്കിൽ (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains ExecGraph");
        // Also mark functions referenced in the dependency graph
        വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : nodep->depGraphp()->vertices()) {
            getInlineCFuncsFunctionVertexp(vtx.as<ExecMTask>()->funcp())
                ->setKeep("MTask function");
        }
        VL_RESTORER(m_inExecGraph);
        m_inExecGraph = സത്യം;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstCStmt* nodep) override {
        // Can reference anything in text
        എങ്കിൽ (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmt");
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstCExpr* nodep) override {
        // Can reference anything in text
        എങ്കിൽ (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExpr");
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstCStmtUser* nodep) override {
        // Can reference anything in text
        എങ്കിൽ (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CStmtUser");
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstCExprUser* nodep) override {
        // Can reference anything in text
        എങ്കിൽ (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CExprUser");
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstCReturn* nodep) override {
        എങ്കിൽ (m_cfuncVtxp) m_cfuncVtxp->setNoInline("Contains CReturn");
        iterateChildrenConst(nodep);
    }

    // Base node
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        എങ്കിൽ (m_cfuncVtxp) m_cfuncVtxp->sizeInc();
        iterateChildrenConst(nodep);
    }

പൊതു:
    // CONSTRUCTORS
    explicit InlineCFuncsVisitor(AstNetlist* nodep) {
        // Phase 1: Build call graph
        iterateConst(nodep);
        // Make acyclic in case there is recursion
        m_graph.acyclic(V3GraphEdge::followAlwaysTrue);
        // Order vertices (any topological order is fine)
        m_graph.order();
        എങ്കിൽ (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-graph");
        // Phase 2: Inline calls
        doInlining();
        എങ്കിൽ (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-inlined");
        // Phase 3: Remove unused functions
        removeUnusedFuncs();
        എങ്കിൽ (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("inlinecfuncs-kept");
    }
    ~InlineCFuncsVisitor() override {
        V3Stats::addStat("Optimizations, Inline CFuncs, calls inlined", m_statCallsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions inlined", m_statFuncsInlined);
        V3Stats::addStat("Optimizations, Inline CFuncs, functions removed", m_statFuncsRemoved);
    }
};

//######################################################################
// InlineCFuncs class functions

ശൂന്യം V3InlineCFuncs::inlineAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    // Don't inline when profiling per-function (it would lose granularity)
    എങ്കിൽ (v3Global.opt.profCFuncs()) തിരികെ;
    { InlineCFuncsVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("inlinecfuncs", 0, dump_വൃക്ഷം_Either_Level() >= 6);
}
