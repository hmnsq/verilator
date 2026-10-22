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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Reorder.h"

#অন্তর্ভাৱ "V3EmitV.h"
#অন্তর্ভাৱ "V3Graph.h"
#অন্তর্ভাৱ "V3Stats.h"

#অন্তর্ভাৱ <string>
#অন্তর্ভাৱ <unordered_map>
#অন্তর্ভাৱ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ {

//######################################################################
// Support classes

শ্ৰেণী Reorder_গ্ৰন্থি_Vertex VL_NOT_FINAL : সার্ৱজনিক V3GraphVertex {
    VL_RTTI_IMPL(Reorder_গ্ৰন্থি_Vertex, V3GraphVertex)
    Ast_গ্ৰন্থি* ধ্রুৱ m_nodep;

রক্ষিত:
    Reorder_গ্ৰন্থি_Vertex(V3Graph* graphp, Ast_গ্ৰন্থি* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~Reorder_গ্ৰন্থি_Vertex() override = স্ৱভাৱ;
    // ACCESSORS
    // Do not make accessor for nodep(),  It may change due to
    // reordering a lower block, but we don't repair it
    std::string name() ধ্রুৱ override {
        std::string str = cvtToHex(m_nodep) + '\n';
        যদি (AstVarScope* ধ্রুৱ vscp = VN_CAST(m_nodep, VarScope)) {
            str += vscp->prettyName();
        } অন্যথা {
            str += V3EmitV::debug_Verilog_For_বৃক্ষ(m_nodep);
            str = VString::quoteBackslash(str);
            str = VString::quoteAny(str, '"', '\\');
            str = VString::replaceSubstr(str, "\n", "\\l");
        }
        ঘূৰাই str;
    }
    File_সাৰি* fileline() ধ্রুৱ override { ঘূৰাই nodep()->fileline(); }
    std::string dotShape() ধ্রুৱ override { ঘূৰাই VN_IS(m_nodep, VarScope) ? "ellipse" : "box"; }

সার্ৱজনিক:
    আভাস Ast_গ্ৰন্থি* nodep() ধ্রুৱ { ঘূৰাই m_nodep; }
};

শ্ৰেণী ReorderImpureVertex final : সার্ৱজনিক Reorder_গ্ৰন্থি_Vertex {
    VL_RTTI_IMPL(ReorderImpureVertex, Reorder_গ্ৰন্থি_Vertex)
সার্ৱজনিক:
    explicit ReorderImpureVertex(V3Graph* graphp, Ast_গ্ৰন্থি* nodep)
        : Reorder_গ্ৰন্থি_Vertex{graphp, nodep} {}
    ~ReorderImpureVertex() override = স্ৱভাৱ;
    string name() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই "*IMPURE*"; }
    string dotColor() ধ্রুৱ override { ঘূৰাই "red"; }
};

শ্ৰেণী ReorderLogicVertex final : সার্ৱজনিক Reorder_গ্ৰন্থি_Vertex {
    VL_RTTI_IMPL(ReorderLogicVertex, Reorder_গ্ৰন্থি_Vertex)
সার্ৱজনিক:
    ReorderLogicVertex(V3Graph* graphp, Ast_গ্ৰন্থি* nodep)
        : Reorder_গ্ৰন্থি_Vertex{graphp, nodep} {}
    ~ReorderLogicVertex() override = স্ৱভাৱ;
    string dotColor() ধ্রুৱ override { ঘূৰাই "black"; }
};

শ্ৰেণী ReorderVarStdVertex final : সার্ৱজনিক Reorder_গ্ৰন্থি_Vertex {
    VL_RTTI_IMPL(ReorderVarStdVertex, Reorder_গ্ৰন্থি_Vertex)
সার্ৱজনিক:
    ReorderVarStdVertex(V3Graph* graphp, AstVarScope* nodep)
        : Reorder_গ্ৰন্থি_Vertex{graphp, nodep} {}
    ~ReorderVarStdVertex() override = স্ৱভাৱ;
    string dotColor() ধ্রুৱ override { ঘূৰাই "blue"; }
};

শ্ৰেণী ReorderVarPostVertex final : সার্ৱজনিক Reorder_গ্ৰন্থি_Vertex {
    VL_RTTI_IMPL(ReorderVarPostVertex, Reorder_গ্ৰন্থি_Vertex)
সার্ৱজনিক:
    ReorderVarPostVertex(V3Graph* graphp, AstVarScope* nodep)
        : Reorder_গ্ৰন্থি_Vertex{graphp, nodep} {}
    ~ReorderVarPostVertex() override = স্ৱভাৱ;
    string name() ধ্রুৱ override { ঘূৰাই "POST "s + Reorder_গ্ৰন্থি_Vertex::name(); }
    string dotColor() ধ্রুৱ override { ঘূৰাই "green"; }
};

//######################################################################
// Edge types

শ্ৰেণী ReorderEdge VL_NOT_FINAL : সার্ৱজনিক V3GraphEdge {
    VL_RTTI_IMPL(ReorderEdge, V3GraphEdge)
    uint32_t m_ignoreInStep = 0;  // Step number that if set to, causes this edge to be ignored
    স্থির uint32_t s_step_অঙ্ক;  // Global step number
রক্ষিত:
    স্থির constexpr পূর্ণাঙ্ক WEIGHT_NORMAL = 10;
    ReorderEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীৰ্ষ, তার্কিক cutable)
        : V3GraphEdge{graphp, fromp, শীৰ্ষ, WEIGHT_NORMAL, cutable} {}
    ~ReorderEdge() override = স্ৱভাৱ;

    আভাস তার্কিক followScoreboard() ধ্রুৱ = 0;

    std::string dotStyle() ধ্রুৱ override {
        ঘূৰাই ignoreThisStep() ? "dotted" : V3GraphEdge::dotStyle();
    }

সার্ৱজনিক:
    // Iterator for graph functions
    স্থির শূন্য incrementStep() { ++s_step_অঙ্ক; }
    তার্কিক ignoreThisStep() ধ্রুৱ { ঘূৰাই m_ignoreInStep == s_step_অঙ্ক; }
    শূন্য setIgnoreThisStep() { m_ignoreInStep = s_step_অঙ্ক; }

    স্থির তার্কিক followScoreboard(ধ্রুৱ V3GraphEdge* edgep) {
        ধ্রুৱ ReorderEdge& edge = *edgep->as<ReorderEdge>();
        ঘূৰাই !edge.ignoreThisStep() && edge.followScoreboard();
    }
    স্থির তার্কিক followCyclic(ধ্রুৱ V3GraphEdge* edgep) {
        ধ্রুৱ ReorderEdge& edge = *edgep->as<ReorderEdge>();
        ঘূৰাই !edge.ignoreThisStep();
    }
};
uint32_t ReorderEdge::s_step_অঙ্ক = 0;

শ্ৰেণী ReorderPostEdge final : সার্ৱজনিক ReorderEdge {
    VL_RTTI_IMPL(ReorderPostEdge, ReorderEdge)
সার্ৱজনিক:
    ReorderPostEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীৰ্ষ)
        : ReorderEdge{graphp, fromp, শীৰ্ষ, CUTABLE} {}
    ~ReorderPostEdge() override = স্ৱভাৱ;
    তার্কিক followScoreboard() ধ্রুৱ override { ঘূৰাই মিছা; }
    string dotColor() ধ্রুৱ override { ঘূৰাই "khaki"; }
};

শ্ৰেণী ReorderLVEdge final : সার্ৱজনিক ReorderEdge {
    VL_RTTI_IMPL(ReorderLVEdge, ReorderEdge)
সার্ৱজনিক:
    ReorderLVEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীৰ্ষ)
        : ReorderEdge{graphp, fromp, শীৰ্ষ, CUTABLE} {}
    ~ReorderLVEdge() override = স্ৱভাৱ;
    তার্কিক followScoreboard() ধ্রুৱ override { ঘূৰাই সঁচা; }
    string dotColor() ধ্রুৱ override { ঘূৰাই "yellowGreen"; }
};

শ্ৰেণী ReorderRVEdge final : সার্ৱজনিক ReorderEdge {
    VL_RTTI_IMPL(ReorderRVEdge, ReorderEdge)
সার্ৱজনিক:
    ReorderRVEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীৰ্ষ)
        : ReorderEdge{graphp, fromp, শীৰ্ষ, CUTABLE} {}
    ~ReorderRVEdge() override = স্ৱভাৱ;
    তার্কিক followScoreboard() ধ্রুৱ override { ঘূৰাই সঁচা; }
    string dotColor() ধ্রুৱ override { ঘূৰাই "green"; }
};

শ্ৰেণী ReorderScorebdEdge final : সার্ৱজনিক ReorderEdge {
    VL_RTTI_IMPL(ReorderScorebdEdge, ReorderEdge)
সার্ৱজনিক:
    ReorderScorebdEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীৰ্ষ)
        : ReorderEdge{graphp, fromp, শীৰ্ষ, CUTABLE} {}
    ~ReorderScorebdEdge() override = স্ৱভাৱ;
    তার্কিক followScoreboard() ধ্রুৱ override { ঘূৰাই সঁচা; }
    string dotColor() ধ্রুৱ override { ঘূৰাই "blue"; }
};

শ্ৰেণী ReorderStrictEdge final : সার্ৱজনিক ReorderEdge {
    VL_RTTI_IMPL(ReorderStrictEdge, ReorderEdge)
    // A strict order, based on the original statement order in the graph
    // The only non-cutable edge type
সার্ৱজনিক:
    ReorderStrictEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীৰ্ষ)
        : ReorderEdge{graphp, fromp, শীৰ্ষ, NOT_CUTABLE} {}
    ~ReorderStrictEdge() override = স্ৱভাৱ;
    তার্কিক followScoreboard() ধ্রুৱ override { ঘূৰাই সঁচা; }
    string dotColor() ধ্রুৱ override { ঘূৰাই "blue"; }
};

//######################################################################
// Reorder class functions

শ্ৰেণী ReorderVisitor final : সার্ৱজনিক VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> ReorderVarStdVertex*.  Usage var, 0=not set yet
    // AstVarScope::user2p  -> ReorderVarPostVertex*.  Delayed assignment var, 0=not set yet
    // Ast*::user3p         -> ReorderLogicVertex*.  Statement (temporary only)
    // Ast*::user4          -> int.  Current ordering number (reorderBlock usage)

    // STATE
    V3Graph* m_graphp = nullptr;  // Scoreboard of var usages/dependencies
    ReorderImpureVertex* m_impureVtxp = nullptr;  // Element specifying PLI ordering
    তার্কিক m_inDly = মিছা;  // Inside ASSIGNDLY
    ধ্রুৱ ৱর্ণ* m_noReorderWhy = nullptr;  // Reason we can't reorder
    std::vector<ReorderLogicVertex*> m_stmtStackps;  // Current statements being tracked

    // METHODS
    শূন্য cleanupBlockGraph(Ast_গ্ৰন্থি* nodep) {
        // Transform the graph into what we need
        UINFO(5, "ReorderBlock " << nodep);

        // Simplify graph by removing redundant edges
        m_graphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        যদি (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_nodup", মিছা);

        // Mark all the logic for this step by setting Vertex::user() to true
        m_graphp->userClearVertices();
        বাবে (Ast_গ্ৰন্থি* nextp = nodep; nextp; nextp = nextp->nextp()) {
            nextp->user3u().to<ReorderLogicVertex*>()->user(সঁচা);
        }

        // New step
        ReorderEdge::incrementStep();

        // If a var vertex has only inputs, it's a input-only node,
        // and can be ignored for coloring **this block only**
        বাবে (V3GraphVertex& vtx : m_graphp->vertices()) {
            যদি (!vtx.outEmpty()) অনুৱর্ত;
            যদি (!vtx.is<ReorderVarStdVertex>()) অনুৱর্ত;
            বাবে (V3GraphEdge& edge : vtx.inEdges()) edge.as<ReorderEdge>()->setIgnoreThisStep();
        }

        // For reordering this single block only, mark all logic
        // vertexes not involved with this step as unimportant
        বাবে (V3GraphVertex& vertex : m_graphp->vertices()) {
            যদি (vertex.user()) অনুৱর্ত;
            যদি (!vertex.is<ReorderLogicVertex>()) অনুৱর্ত;
            বাবে (V3GraphEdge& edge : vertex.inEdges()) edge.as<ReorderEdge>()->setIgnoreThisStep();
            বাবে (V3GraphEdge& edge : vertex.outEdges())
                edge.as<ReorderEdge>()->setIgnoreThisStep();
        }

        // Weak coloring to determine what needs to remain in order
        // This follows all step-relevant edges excluding PostEdges, which are done later
        m_graphp->weaklyConnected(&ReorderEdge::followScoreboard);

        // Add hard orderings between all nodes of same color, in the order they appeared
        std::unordered_map<uint32_t, ReorderLogicVertex*> lastOfColor;
        বাবে (Ast_গ্ৰন্থি* currp = nodep; currp; currp = currp->nextp()) {
            ReorderLogicVertex* ধ্রুৱ vtxp = currp->user3u().to<ReorderLogicVertex*>();
            ধ্রুৱ uint32_t color = vtxp->color();
            যদি (lastOfColor[color]) নূতন ReorderStrictEdge{m_graphp, lastOfColor[color], vtxp};
            lastOfColor[color] = vtxp;
        }

        // And a real ordering to get the statements into something reasonable
        // We don't care if there's cutable violations here...
        // Non-cutable violations should be impossible; as those edges are program-order
        যদি (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_pre", মিছা);
        m_graphp->acyclic(&ReorderEdge::followCyclic);
        m_graphp->rank(&ReorderEdge::followCyclic);  // Or order(), but that's more expensive
        যদি (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_opt", মিছা);
    }

    শূন্য reorderBlock(Ast_গ্ৰন্থি* nodep) {
        // Reorder statements in the completed graph

        // Map the rank numbers into nodes they associate with
        std::multimap<uint32_t, Ast_গ্ৰন্থি*> rankMap;
        পূর্ণাঙ্ক currOrder = 0;  // Existing sequence number of assignment
        বাবে (Ast_গ্ৰন্থি* currp = nodep; currp; currp = currp->nextp()) {
            ধ্রুৱ ReorderLogicVertex* ধ্রুৱ vtxp = currp->user3u().to<ReorderLogicVertex*>();
            rankMap.emplace(vtxp->rank(), currp);
            currp->user4(++currOrder);  // Record current ordering
        }

        // Is the current ordering OK?
        তার্কিক leaveAlone = সঁচা;
        uint64_t newOrder = 0;  // New sequence number of assignment
        বাবে (ধ্রুৱ auto& item : rankMap) {
            ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ nextp = item.second;
            যদি (++newOrder != nextp->user4()) leaveAlone = মিছা;
        }
        যদি (leaveAlone) {
            UINFO(6, "   No changes");
            ঘূৰাই;
        }

        VNRelinker replaceHandle;  // Where to add the list
        Ast_গ্ৰন্থি* newListp = nullptr;
        বাবে (ধ্রুৱ auto& item : rankMap) {
            Ast_গ্ৰন্থি* ধ্রুৱ nextp = item.second;
            UINFO(6, "   New order: " << nextp);
            nextp->unlinkFrBack(nextp == nodep ? &replaceHandle : nullptr);
            newListp = Ast_গ্ৰন্থি::addNext(newListp, nextp);
        }
        replaceHandle.relink(newListp);
    }

    শূন্য processBlock(Ast_গ্ৰন্থি* nodep) {
        যদি (m_noReorderWhy) ঘূৰাই;

        // Empty lists are ignorable
        যদি (!nodep) ঘূৰাই;
        UASSERT_OBJ(nodep->firstAbovep(), nodep, "Node passed is in not head of list");
        UASSERT_OBJ(!nodep->user3p(), nodep, "Should not have a logic vertex");

        // It nothing to reorder with, just iterate
        যদি (!nodep->nextp()) {
            iterate(nodep);
            ঘূৰাই;
        }

        // Process it
        UINFO(9, "  processBlock " << nodep);

        // Iterate across current block, making the scoreboard
        বাবে (Ast_গ্ৰন্থি* currp = nodep; currp; currp = currp->nextp()) {
            // Create the logic vertex for this statement
            UASSERT_OBJ(!currp->user3p(), currp, "user3p should not be set");
            ReorderLogicVertex* ধ্রুৱ vtxp = নূতন ReorderLogicVertex{m_graphp, currp};
            currp->user3p(vtxp);

            // Visit the statement - this can recursively reorder sub statements
            m_stmtStackps.ঠেলো_back(vtxp);
            iterate(currp);
            m_stmtStackps.বেৰ_কৰো_back();
        }

        যদি (m_noReorderWhy) {  // Jump or something nasty
            UINFO(9, "  NoReorderBlock because " << m_noReorderWhy);
            ঘূৰাই;
        }

        // Reorder statements in this block
        cleanupBlockGraph(nodep);
        reorderBlock(nodep);

        // 'nodep' might no longer be the head of the list, rewind
        যেতিয়ালৈ (nodep->backp()->nextp() == nodep) nodep = nodep->backp();

        // Delete vertexes and edges only applying to this block
        বাবে (Ast_গ্ৰন্থি* currp = nodep; currp; currp = currp->nextp()) {
            currp->user3u().to<ReorderLogicVertex*>()->unlinkDelete(m_graphp);
            currp->user3p(nullptr);
        }
    }

    // VISITORS
    শূন্য visit(AstAlways* nodep) override {
        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_inDly);
        VL_RESTORER(m_noReorderWhy);

        V3Graph graph;
        m_graphp = &graph;
        m_impureVtxp = nullptr;
        m_inDly = মিছা;
        m_noReorderWhy = nullptr;

        ধ্রুৱ VNUser1InUse user1InUse;
        ধ্রুৱ VNUser2InUse user2InUse;
        ধ্রুৱ VNUser3InUse user3InUse;
        ধ্রুৱ VNUser4InUse user4InUse;

        UINFOTREE(9, nodep, "", "alwIn:");
        processBlock(nodep->stmtsp());
        UINFOTREE(9, nodep, "", "alwOut");

        m_stmtStackps.clear();
    }

    শূন্য visit(Ast_গ্ৰন্থি_If* nodep) override {
        যদি (!m_graphp || m_noReorderWhy) ঘূৰাই;
        iterateAndNextNull(nodep->condp());
        processBlock(nodep->thensp());
        processBlock(nodep->elsesp());
    }

    শূন্য visit(AstJumpGo* nodep) override {
        যদি (!m_graphp || m_noReorderWhy) ঘূৰাই;
        m_noReorderWhy = "JumpGo";
        iterateChildren(nodep);
    }

    শূন্য visit(Ast_গ্ৰন্থি_CCall* nodep) override {
        যদি (!m_graphp || m_noReorderWhy) ঘূৰাই;
        // The callee is not inlined, so the variables it reads and writes carry no VarRef at
        // the call site, and the scoreboard would let assignments to them move across the
        // call. Treat any such call as a barrier.
        m_noReorderWhy = "CCall";
        iterateChildren(nodep);
    }

    শূন্য visit(Ast_ৰাশিমালা_Stmt* nodep) override {
        যদি (!m_graphp || m_noReorderWhy) ঘূৰাই;
        VL_RESTORER(m_inDly);
        m_inDly = মিছা;
        iterateChildren(nodep);
    }

    শূন্য visit(AstAssignDly* nodep) override {
        যদি (!m_graphp || m_noReorderWhy) ঘূৰাই;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_inDly);
        m_inDly = সঁচা;
        iterate(nodep->lhsp());
    }

    শূন্য visit(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        যদি (m_graphp) m_noReorderWhy = "Handle";
        visit(static_cast<Ast_গ্ৰন্থি*>(nodep));
    }

    শূন্য visit(AstVarRef* nodep) override {
        যদি (!m_graphp || m_noReorderWhy) ঘূৰাই;
        যদি (m_stmtStackps.empty()) ঘূৰাই;

        // Reads of constants can be ignored - TODO: This should be "constexpr", not run-time const
        যদি (nodep->varp()->isConst()) ঘূৰাই;

        // SPEEDUP: We add duplicate edges, that should be fixed

        AstVarScope* ধ্রুৱ vscp = nodep->varScopep();

        // Create vertexes for variable
        যদি (!vscp->user1p()) vscp->user1p(নূতন ReorderVarStdVertex{m_graphp, vscp});
        ReorderVarStdVertex* ধ্রুৱ vstdp = vscp->user1u().to<ReorderVarStdVertex*>();

        // Variable is read
        যদি (nodep->access().isReadOnly()) {
            বাবে (ReorderLogicVertex* ধ্রুৱ vtxp : m_stmtStackps) {
                নূতন ReorderRVEdge{m_graphp, vtxp, vstdp};
            }
            ঘূৰাই;
        }

        // Variable is written, not NBA
        যদি (!m_inDly) {
            বাবে (ReorderLogicVertex* ধ্রুৱ vtxp : m_stmtStackps) {
                নূতন ReorderLVEdge{m_graphp, vstdp, vtxp};
            }
            ঘূৰাই;
        }

        // Variable is written by NBA
        যদি (!vscp->user2p()) {
            ReorderVarPostVertex* ধ্রুৱ vpostp = নূতন ReorderVarPostVertex{m_graphp, vscp};
            vscp->user2p(vpostp);
            নূতন ReorderPostEdge{m_graphp, vstdp, vpostp};
        }
        ReorderVarPostVertex* ধ্রুৱ vpostp = vscp->user2u().to<ReorderVarPostVertex*>();
        বাবে (ReorderLogicVertex* ধ্রুৱ vtxp : m_stmtStackps) {
            নূতন ReorderLVEdge{m_graphp, vpostp, vtxp};
        }
    }

    শূন্য visit(Ast_গ্ৰন্থি* nodep) override {
        // Outside AstAlways, just descend
        যদি (!m_graphp) {
            iterateChildren(nodep);
            ঘূৰাই;
        }

        // Early exit if decided not to reorder
        যদি (m_noReorderWhy) ঘূৰাই;

        // Timing control prevents reordering
        যদি (nodep->isTimingControl()) {
            m_noReorderWhy = "TimingControl";
            ঘূৰাই;
        }

        // Order all impure statements with other impure statements
        যদি (!nodep->isPure()) {
            যদি (!m_impureVtxp) m_impureVtxp = নূতন ReorderImpureVertex{m_graphp, nodep};
            // This edge is only used to find weakly connected components, so one edge is enough
            বাবে (ReorderLogicVertex* ধ্রুৱ vtxp : m_stmtStackps) {
                নূতন ReorderScorebdEdge{m_graphp, m_impureVtxp, vtxp};
            }
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
সার্ৱজনিক:
    explicit ReorderVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReorderVisitor() override = স্ৱভাৱ;
};

}  // namespace

//######################################################################
// V3Reorder class functions

শূন্য V3Reorder::reorderAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReorderVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("reorder", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
