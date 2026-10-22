// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Reorder statements within always blocks
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
// V3Reorder transformations:
//
//  reorderAll() reorders statements within individual blocks to avoid
//  shwdow variables use by non-blocking assignments when possible.
//  For exmaple, the left side needs a shadow variable for 'b', the
//  right side does not:
//      Bad:            Good:
//          b <= a;         c <= b;
//          c <= b;         b <= a;
//
// The scoreboard tracks data deps as follows:
//
//      ALWAYS
//              ASSIGN ({var} <= {cons})
//              Record as generating var_DLY (independent of use of var), consumers
//              ASSIGN ({var} = {cons}
//              Record generator and consumer
//      Any var that is only consumed can be ignored.
//      Then we split into separate ALWAYS blocks.
//
// The scoreboard includes innards of if/else nodes also.  Splitting is no
// longer limited to top-level statements, we can split within if-else
// blocks. We want to be able to split this:
//
// The optional reorder routine can optimize this:
//      NODEASSIGN/NODEIF/WHILE
//              S1: ASSIGN {v1} <= 0.   // Duplicate of below
//              S2: ASSIGN {v1} <= {v0}
//              S3: IF (...,
//                      X1: ASSIGN {v2} <= {v1}
//                      X2: ASSIGN {v3} <= {v2}
//      We'd like to swap S2 and S3, and X1 and X2.
//
//  Create a graph in split assignment order.
//      v3 -breakable-> v3Dly --> X2 --> v2 -brk-> v2Dly -> X1 -> v1
//      Likewise on each "upper" statement vertex
//              v3Dly & v2Dly -> S3 -> v1 & v2
//              v1 -brk-> v1Dly -> S2 -> v0
//                        v1Dly -> S1 -> {empty}
//
//*************************************************************************

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Reorder.h"

#ଅନ୍ତର୍ଭାଵ "V3EmitV.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <string>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ନାମାକାଶ {

//######################################################################
// Support classes

ଶ୍ରେଣୀ Reorder_ଗଣ୍ଠି_Vertex VL_NOT_FINAL : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(Reorder_ଗଣ୍ଠି_Vertex, V3GraphVertex)
    Ast_ଗଣ୍ଠି* ଅଚଳ m_nodep;

ସୁରକ୍ଷିତ:
    Reorder_ଗଣ୍ଠି_Vertex(V3Graph* graphp, Ast_ଗଣ୍ଠି* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~Reorder_ଗଣ୍ଠି_Vertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // ACCESSORS
    // Do not make accessor for nodep(),  It may change due to
    // reordering a lower block, but we don't repair it
    std::string name() ଅଚଳ override {
        std::string str = cvtToHex(m_nodep) + '\n';
        ଯଦି (AstVarScope* ଅଚଳ vscp = VN_CAST(m_nodep, VarScope)) {
            str += vscp->prettyName();
        } ଅନ୍ୟଥା {
            str += V3EmitV::debug_Verilog_For_ବୃକ୍ଷ(m_nodep);
            str = VString::quoteBackslash(str);
            str = VString::quoteAny(str, '"', '\\');
            str = VString::replaceSubstr(str, "\n", "\\l");
        }
        ଫେରନ୍ତୁ str;
    }
    File_ଧାଡ଼ି* fileline() ଅଚଳ override { ଫେରନ୍ତୁ nodep()->fileline(); }
    std::string dotShape() ଅଚଳ override { ଫେରନ୍ତୁ VN_IS(m_nodep, VarScope) ? "ellipse" : "box"; }

ସର୍ବସାଧାରଣ:
    ଆଭାସୀ Ast_ଗଣ୍ଠି* nodep() ଅଚଳ { ଫେରନ୍ତୁ m_nodep; }
};

ଶ୍ରେଣୀ ReorderImpureVertex final : ସର୍ବସାଧାରଣ Reorder_ଗଣ୍ଠି_Vertex {
    VL_RTTI_IMPL(ReorderImpureVertex, Reorder_ଗଣ୍ଠି_Vertex)
ସର୍ବସାଧାରଣ:
    explicit ReorderImpureVertex(V3Graph* graphp, Ast_ଗଣ୍ଠି* nodep)
        : Reorder_ଗଣ୍ଠି_Vertex{graphp, nodep} {}
    ~ReorderImpureVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ "*IMPURE*"; }
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "red"; }
};

ଶ୍ରେଣୀ ReorderLogicVertex final : ସର୍ବସାଧାରଣ Reorder_ଗଣ୍ଠି_Vertex {
    VL_RTTI_IMPL(ReorderLogicVertex, Reorder_ଗଣ୍ଠି_Vertex)
ସର୍ବସାଧାରଣ:
    ReorderLogicVertex(V3Graph* graphp, Ast_ଗଣ୍ଠି* nodep)
        : Reorder_ଗଣ୍ଠି_Vertex{graphp, nodep} {}
    ~ReorderLogicVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "black"; }
};

ଶ୍ରେଣୀ ReorderVarStdVertex final : ସର୍ବସାଧାରଣ Reorder_ଗଣ୍ଠି_Vertex {
    VL_RTTI_IMPL(ReorderVarStdVertex, Reorder_ଗଣ୍ଠି_Vertex)
ସର୍ବସାଧାରଣ:
    ReorderVarStdVertex(V3Graph* graphp, AstVarScope* nodep)
        : Reorder_ଗଣ୍ଠି_Vertex{graphp, nodep} {}
    ~ReorderVarStdVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "blue"; }
};

ଶ୍ରେଣୀ ReorderVarPostVertex final : ସର୍ବସାଧାରଣ Reorder_ଗଣ୍ଠି_Vertex {
    VL_RTTI_IMPL(ReorderVarPostVertex, Reorder_ଗଣ୍ଠି_Vertex)
ସର୍ବସାଧାରଣ:
    ReorderVarPostVertex(V3Graph* graphp, AstVarScope* nodep)
        : Reorder_ଗଣ୍ଠି_Vertex{graphp, nodep} {}
    ~ReorderVarPostVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    string name() ଅଚଳ override { ଫେରନ୍ତୁ "POST "s + Reorder_ଗଣ୍ଠି_Vertex::name(); }
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "green"; }
};

//######################################################################
// Edge types

ଶ୍ରେଣୀ ReorderEdge VL_NOT_FINAL : ସର୍ବସାଧାରଣ V3GraphEdge {
    VL_RTTI_IMPL(ReorderEdge, V3GraphEdge)
    uint32_t m_ignoreInStep = 0;  // Step number that if set to, causes this edge to be ignored
    ସ୍ଥିର uint32_t s_step_ଅଙ୍କ;  // Global step number
ସୁରକ୍ଷିତ:
    ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା WEIGHT_NORMAL = 10;
    ReorderEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ଶୀର୍ଷ, ତାର୍କିକ cutable)
        : V3GraphEdge{graphp, fromp, ଶୀର୍ଷ, WEIGHT_NORMAL, cutable} {}
    ~ReorderEdge() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    ଆଭାସୀ ତାର୍କିକ followScoreboard() ଅଚଳ = 0;

    std::string dotStyle() ଅଚଳ override {
        ଫେରନ୍ତୁ ignoreThisStep() ? "dotted" : V3GraphEdge::dotStyle();
    }

ସର୍ବସାଧାରଣ:
    // Iterator for graph functions
    ସ୍ଥିର ଶୂନ୍ୟ incrementStep() { ++s_step_ଅଙ୍କ; }
    ତାର୍କିକ ignoreThisStep() ଅଚଳ { ଫେରନ୍ତୁ m_ignoreInStep == s_step_ଅଙ୍କ; }
    ଶୂନ୍ୟ setIgnoreThisStep() { m_ignoreInStep = s_step_ଅଙ୍କ; }

    ସ୍ଥିର ତାର୍କିକ followScoreboard(ଅଚଳ V3GraphEdge* edgep) {
        ଅଚଳ ReorderEdge& edge = *edgep->as<ReorderEdge>();
        ଫେରନ୍ତୁ !edge.ignoreThisStep() && edge.followScoreboard();
    }
    ସ୍ଥିର ତାର୍କିକ followCyclic(ଅଚଳ V3GraphEdge* edgep) {
        ଅଚଳ ReorderEdge& edge = *edgep->as<ReorderEdge>();
        ଫେରନ୍ତୁ !edge.ignoreThisStep();
    }
};
uint32_t ReorderEdge::s_step_ଅଙ୍କ = 0;

ଶ୍ରେଣୀ ReorderPostEdge final : ସର୍ବସାଧାରଣ ReorderEdge {
    VL_RTTI_IMPL(ReorderPostEdge, ReorderEdge)
ସର୍ବସାଧାରଣ:
    ReorderPostEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ଶୀର୍ଷ)
        : ReorderEdge{graphp, fromp, ଶୀର୍ଷ, CUTABLE} {}
    ~ReorderPostEdge() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ତାର୍କିକ followScoreboard() ଅଚଳ override { ଫେରନ୍ତୁ ମିଥ୍ୟା; }
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "khaki"; }
};

ଶ୍ରେଣୀ ReorderLVEdge final : ସର୍ବସାଧାରଣ ReorderEdge {
    VL_RTTI_IMPL(ReorderLVEdge, ReorderEdge)
ସର୍ବସାଧାରଣ:
    ReorderLVEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ଶୀର୍ଷ)
        : ReorderEdge{graphp, fromp, ଶୀର୍ଷ, CUTABLE} {}
    ~ReorderLVEdge() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ତାର୍କିକ followScoreboard() ଅଚଳ override { ଫେରନ୍ତୁ ସତ୍ୟ; }
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "yellowGreen"; }
};

ଶ୍ରେଣୀ ReorderRVEdge final : ସର୍ବସାଧାରଣ ReorderEdge {
    VL_RTTI_IMPL(ReorderRVEdge, ReorderEdge)
ସର୍ବସାଧାରଣ:
    ReorderRVEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ଶୀର୍ଷ)
        : ReorderEdge{graphp, fromp, ଶୀର୍ଷ, CUTABLE} {}
    ~ReorderRVEdge() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ତାର୍କିକ followScoreboard() ଅଚଳ override { ଫେରନ୍ତୁ ସତ୍ୟ; }
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "green"; }
};

ଶ୍ରେଣୀ ReorderScorebdEdge final : ସର୍ବସାଧାରଣ ReorderEdge {
    VL_RTTI_IMPL(ReorderScorebdEdge, ReorderEdge)
ସର୍ବସାଧାରଣ:
    ReorderScorebdEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ଶୀର୍ଷ)
        : ReorderEdge{graphp, fromp, ଶୀର୍ଷ, CUTABLE} {}
    ~ReorderScorebdEdge() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ତାର୍କିକ followScoreboard() ଅଚଳ override { ଫେରନ୍ତୁ ସତ୍ୟ; }
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "blue"; }
};

ଶ୍ରେଣୀ ReorderStrictEdge final : ସର୍ବସାଧାରଣ ReorderEdge {
    VL_RTTI_IMPL(ReorderStrictEdge, ReorderEdge)
    // A strict order, based on the original statement order in the graph
    // The only non-cutable edge type
ସର୍ବସାଧାରଣ:
    ReorderStrictEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ଶୀର୍ଷ)
        : ReorderEdge{graphp, fromp, ଶୀର୍ଷ, NOT_CUTABLE} {}
    ~ReorderStrictEdge() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ତାର୍କିକ followScoreboard() ଅଚଳ override { ଫେରନ୍ତୁ ସତ୍ୟ; }
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "blue"; }
};

//######################################################################
// Reorder class functions

ଶ୍ରେଣୀ ReorderVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> ReorderVarStdVertex*.  Usage var, 0=not set yet
    // AstVarScope::user2p  -> ReorderVarPostVertex*.  Delayed assignment var, 0=not set yet
    // Ast*::user3p         -> ReorderLogicVertex*.  Statement (temporary only)
    // Ast*::user4          -> int.  Current ordering number (reorderBlock usage)

    // STATE
    V3Graph* m_graphp = nullptr;  // Scoreboard of var usages/dependencies
    ReorderImpureVertex* m_impureVtxp = nullptr;  // Element specifying PLI ordering
    ତାର୍କିକ m_inDly = ମିଥ୍ୟା;  // Inside ASSIGNDLY
    ଅଚଳ ଅକ୍ଷର* m_noReorderWhy = nullptr;  // Reason we can't reorder
    std::vector<ReorderLogicVertex*> m_stmtStackps;  // Current statements being tracked

    // METHODS
    ଶୂନ୍ୟ cleanupBlockGraph(Ast_ଗଣ୍ଠି* nodep) {
        // Transform the graph into what we need
        UINFO(5, "ReorderBlock " << nodep);

        // Simplify graph by removing redundant edges
        m_graphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        ଯଦି (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_nodup", ମିଥ୍ୟା);

        // Mark all the logic for this step by setting Vertex::user() to true
        m_graphp->userClearVertices();
        ପାଇଁ (Ast_ଗଣ୍ଠି* nextp = nodep; nextp; nextp = nextp->nextp()) {
            nextp->user3u().to<ReorderLogicVertex*>()->user(ସତ୍ୟ);
        }

        // New step
        ReorderEdge::incrementStep();

        // If a var vertex has only inputs, it's a input-only node,
        // and can be ignored for coloring **this block only**
        ପାଇଁ (V3GraphVertex& vtx : m_graphp->vertices()) {
            ଯଦି (!vtx.outEmpty()) ଜାରି;
            ଯଦି (!vtx.is<ReorderVarStdVertex>()) ଜାରି;
            ପାଇଁ (V3GraphEdge& edge : vtx.inEdges()) edge.as<ReorderEdge>()->setIgnoreThisStep();
        }

        // For reordering this single block only, mark all logic
        // vertexes not involved with this step as unimportant
        ପାଇଁ (V3GraphVertex& vertex : m_graphp->vertices()) {
            ଯଦି (vertex.user()) ଜାରି;
            ଯଦି (!vertex.is<ReorderLogicVertex>()) ଜାରି;
            ପାଇଁ (V3GraphEdge& edge : vertex.inEdges()) edge.as<ReorderEdge>()->setIgnoreThisStep();
            ପାଇଁ (V3GraphEdge& edge : vertex.outEdges())
                edge.as<ReorderEdge>()->setIgnoreThisStep();
        }

        // Weak coloring to determine what needs to remain in order
        // This follows all step-relevant edges excluding PostEdges, which are done later
        m_graphp->weaklyConnected(&ReorderEdge::followScoreboard);

        // Add hard orderings between all nodes of same color, in the order they appeared
        std::unordered_map<uint32_t, ReorderLogicVertex*> lastOfColor;
        ପାଇଁ (Ast_ଗଣ୍ଠି* currp = nodep; currp; currp = currp->nextp()) {
            ReorderLogicVertex* ଅଚଳ vtxp = currp->user3u().to<ReorderLogicVertex*>();
            ଅଚଳ uint32_t color = vtxp->color();
            ଯଦି (lastOfColor[color]) ନୂତନ ReorderStrictEdge{m_graphp, lastOfColor[color], vtxp};
            lastOfColor[color] = vtxp;
        }

        // And a real ordering to get the statements into something reasonable
        // We don't care if there's cutable violations here...
        // Non-cutable violations should be impossible; as those edges are program-order
        ଯଦି (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_pre", ମିଥ୍ୟା);
        m_graphp->acyclic(&ReorderEdge::followCyclic);
        m_graphp->rank(&ReorderEdge::followCyclic);  // Or order(), but that's more expensive
        ଯଦି (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_opt", ମିଥ୍ୟା);
    }

    ଶୂନ୍ୟ reorderBlock(Ast_ଗଣ୍ଠି* nodep) {
        // Reorder statements in the completed graph

        // Map the rank numbers into nodes they associate with
        std::multimap<uint32_t, Ast_ଗଣ୍ଠି*> rankMap;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା currOrder = 0;  // Existing sequence number of assignment
        ପାଇଁ (Ast_ଗଣ୍ଠି* currp = nodep; currp; currp = currp->nextp()) {
            ଅଚଳ ReorderLogicVertex* ଅଚଳ vtxp = currp->user3u().to<ReorderLogicVertex*>();
            rankMap.emplace(vtxp->rank(), currp);
            currp->user4(++currOrder);  // Record current ordering
        }

        // Is the current ordering OK?
        ତାର୍କିକ leaveAlone = ସତ୍ୟ;
        uint64_t newOrder = 0;  // New sequence number of assignment
        ପାଇଁ (ଅଚଳ auto& item : rankMap) {
            ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ nextp = item.second;
            ଯଦି (++newOrder != nextp->user4()) leaveAlone = ମିଥ୍ୟା;
        }
        ଯଦି (leaveAlone) {
            UINFO(6, "   No changes");
            ଫେରନ୍ତୁ;
        }

        VNRelinker replaceHandle;  // Where to add the list
        Ast_ଗଣ୍ଠି* newListp = nullptr;
        ପାଇଁ (ଅଚଳ auto& item : rankMap) {
            Ast_ଗଣ୍ଠି* ଅଚଳ nextp = item.second;
            UINFO(6, "   New order: " << nextp);
            nextp->unlinkFrBack(nextp == nodep ? &replaceHandle : nullptr);
            newListp = Ast_ଗଣ୍ଠି::addNext(newListp, nextp);
        }
        replaceHandle.relink(newListp);
    }

    ଶୂନ୍ୟ processBlock(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (m_noReorderWhy) ଫେରନ୍ତୁ;

        // Empty lists are ignorable
        ଯଦି (!nodep) ଫେରନ୍ତୁ;
        UASSERT_OBJ(nodep->firstAbovep(), nodep, "Node passed is in not head of list");
        UASSERT_OBJ(!nodep->user3p(), nodep, "Should not have a logic vertex");

        // It nothing to reorder with, just iterate
        ଯଦି (!nodep->nextp()) {
            iterate(nodep);
            ଫେରନ୍ତୁ;
        }

        // Process it
        UINFO(9, "  processBlock " << nodep);

        // Iterate across current block, making the scoreboard
        ପାଇଁ (Ast_ଗଣ୍ଠି* currp = nodep; currp; currp = currp->nextp()) {
            // Create the logic vertex for this statement
            UASSERT_OBJ(!currp->user3p(), currp, "user3p should not be set");
            ReorderLogicVertex* ଅଚଳ vtxp = ନୂତନ ReorderLogicVertex{m_graphp, currp};
            currp->user3p(vtxp);

            // Visit the statement - this can recursively reorder sub statements
            m_stmtStackps.ଠେଲ_back(vtxp);
            iterate(currp);
            m_stmtStackps.କାଢ଼_back();
        }

        ଯଦି (m_noReorderWhy) {  // Jump or something nasty
            UINFO(9, "  NoReorderBlock because " << m_noReorderWhy);
            ଫେରନ୍ତୁ;
        }

        // Reorder statements in this block
        cleanupBlockGraph(nodep);
        reorderBlock(nodep);

        // 'nodep' might no longer be the head of the list, rewind
        ଯେତେବେଳେ (nodep->backp()->nextp() == nodep) nodep = nodep->backp();

        // Delete vertexes and edges only applying to this block
        ପାଇଁ (Ast_ଗଣ୍ଠି* currp = nodep; currp; currp = currp->nextp()) {
            currp->user3u().to<ReorderLogicVertex*>()->unlinkDelete(m_graphp);
            currp->user3p(nullptr);
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstAlways* nodep) override {
        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_inDly);
        VL_RESTORER(m_noReorderWhy);

        V3Graph graph;
        m_graphp = &graph;
        m_impureVtxp = nullptr;
        m_inDly = ମିଥ୍ୟା;
        m_noReorderWhy = nullptr;

        ଅଚଳ VNUser1InUse user1InUse;
        ଅଚଳ VNUser2InUse user2InUse;
        ଅଚଳ VNUser3InUse user3InUse;
        ଅଚଳ VNUser4InUse user4InUse;

        UINFOTREE(9, nodep, "", "alwIn:");
        processBlock(nodep->stmtsp());
        UINFOTREE(9, nodep, "", "alwOut");

        m_stmtStackps.clear();
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_If* nodep) override {
        ଯଦି (!m_graphp || m_noReorderWhy) ଫେରନ୍ତୁ;
        iterateAndNextNull(nodep->condp());
        processBlock(nodep->thensp());
        processBlock(nodep->elsesp());
    }

    ଶୂନ୍ୟ visit(AstJumpGo* nodep) override {
        ଯଦି (!m_graphp || m_noReorderWhy) ଫେରନ୍ତୁ;
        m_noReorderWhy = "JumpGo";
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_CCall* nodep) override {
        ଯଦି (!m_graphp || m_noReorderWhy) ଫେରନ୍ତୁ;
        // The callee is not inlined, so the variables it reads and writes carry no VarRef at
        // the call site, and the scoreboard would let assignments to them move across the
        // call. Treat any such call as a barrier.
        m_noReorderWhy = "CCall";
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ପଦସମଷ୍ଟି_Stmt* nodep) override {
        ଯଦି (!m_graphp || m_noReorderWhy) ଫେରନ୍ତୁ;
        VL_RESTORER(m_inDly);
        m_inDly = ମିଥ୍ୟା;
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstAssignDly* nodep) override {
        ଯଦି (!m_graphp || m_noReorderWhy) ଫେରନ୍ତୁ;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_inDly);
        m_inDly = ସତ୍ୟ;
        iterate(nodep->lhsp());
    }

    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        ଯଦି (m_graphp) m_noReorderWhy = "Handle";
        visit(static_cast<Ast_ଗଣ୍ଠି*>(nodep));
    }

    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        ଯଦି (!m_graphp || m_noReorderWhy) ଫେରନ୍ତୁ;
        ଯଦି (m_stmtStackps.empty()) ଫେରନ୍ତୁ;

        // Reads of constants can be ignored - TODO: This should be "constexpr", not run-time const
        ଯଦି (nodep->varp()->isConst()) ଫେରନ୍ତୁ;

        // SPEEDUP: We add duplicate edges, that should be fixed

        AstVarScope* ଅଚଳ vscp = nodep->varScopep();

        // Create vertexes for variable
        ଯଦି (!vscp->user1p()) vscp->user1p(ନୂତନ ReorderVarStdVertex{m_graphp, vscp});
        ReorderVarStdVertex* ଅଚଳ vstdp = vscp->user1u().to<ReorderVarStdVertex*>();

        // Variable is read
        ଯଦି (nodep->access().isReadOnly()) {
            ପାଇଁ (ReorderLogicVertex* ଅଚଳ vtxp : m_stmtStackps) {
                ନୂତନ ReorderRVEdge{m_graphp, vtxp, vstdp};
            }
            ଫେରନ୍ତୁ;
        }

        // Variable is written, not NBA
        ଯଦି (!m_inDly) {
            ପାଇଁ (ReorderLogicVertex* ଅଚଳ vtxp : m_stmtStackps) {
                ନୂତନ ReorderLVEdge{m_graphp, vstdp, vtxp};
            }
            ଫେରନ୍ତୁ;
        }

        // Variable is written by NBA
        ଯଦି (!vscp->user2p()) {
            ReorderVarPostVertex* ଅଚଳ vpostp = ନୂତନ ReorderVarPostVertex{m_graphp, vscp};
            vscp->user2p(vpostp);
            ନୂତନ ReorderPostEdge{m_graphp, vstdp, vpostp};
        }
        ReorderVarPostVertex* ଅଚଳ vpostp = vscp->user2u().to<ReorderVarPostVertex*>();
        ପାଇଁ (ReorderLogicVertex* ଅଚଳ vtxp : m_stmtStackps) {
            ନୂତନ ReorderLVEdge{m_graphp, vpostp, vtxp};
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        // Outside AstAlways, just descend
        ଯଦି (!m_graphp) {
            iterateChildren(nodep);
            ଫେରନ୍ତୁ;
        }

        // Early exit if decided not to reorder
        ଯଦି (m_noReorderWhy) ଫେରନ୍ତୁ;

        // Timing control prevents reordering
        ଯଦି (nodep->isTimingControl()) {
            m_noReorderWhy = "TimingControl";
            ଫେରନ୍ତୁ;
        }

        // Order all impure statements with other impure statements
        ଯଦି (!nodep->isPure()) {
            ଯଦି (!m_impureVtxp) m_impureVtxp = ନୂତନ ReorderImpureVertex{m_graphp, nodep};
            // This edge is only used to find weakly connected components, so one edge is enough
            ପାଇଁ (ReorderLogicVertex* ଅଚଳ vtxp : m_stmtStackps) {
                ନୂତନ ReorderScorebdEdge{m_graphp, m_impureVtxp, vtxp};
            }
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
ସର୍ବସାଧାରଣ:
    explicit ReorderVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReorderVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

}  // namespace

//######################################################################
// V3Reorder class functions

ଶୂନ୍ୟ V3Reorder::reorderAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReorderVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("reorder", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
