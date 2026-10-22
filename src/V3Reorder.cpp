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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Reorder.h"

#অন্তর্ভাব "V3EmitV.h"
#অন্তর্ভাব "V3Graph.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <string>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ {

//######################################################################
// Support classes

শ্রেণী Reorder_গ্রন্থি_Vertex VL_NOT_FINAL : সর্বজনীন V3GraphVertex {
    VL_RTTI_IMPL(Reorder_গ্রন্থি_Vertex, V3GraphVertex)
    Ast_গ্রন্থি* ধ্রুবক m_nodep;

সুরক্ষিত:
    Reorder_গ্রন্থি_Vertex(V3Graph* graphp, Ast_গ্রন্থি* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~Reorder_গ্রন্থি_Vertex() override = পূর্বনির্ধারিত;
    // ACCESSORS
    // Do not make accessor for nodep(),  It may change due to
    // reordering a lower block, but we don't repair it
    std::string name() ধ্রুবক override {
        std::string str = cvtToHex(m_nodep) + '\n';
        যদি (AstVarScope* ধ্রুবক vscp = VN_CAST(m_nodep, VarScope)) {
            str += vscp->prettyName();
        } অন্যথা {
            str += V3EmitV::debug_Verilog_For_বৃক্ষ(m_nodep);
            str = VString::quoteBackslash(str);
            str = VString::quoteAny(str, '"', '\\');
            str = VString::replaceSubstr(str, "\n", "\\l");
        }
        ফেরত str;
    }
    File_সারি* fileline() ধ্রুবক override { ফেরত nodep()->fileline(); }
    std::string dotShape() ধ্রুবক override { ফেরত VN_IS(m_nodep, VarScope) ? "ellipse" : "box"; }

সর্বজনীন:
    আভাসী Ast_গ্রন্থি* nodep() ধ্রুবক { ফেরত m_nodep; }
};

শ্রেণী ReorderImpureVertex final : সর্বজনীন Reorder_গ্রন্থি_Vertex {
    VL_RTTI_IMPL(ReorderImpureVertex, Reorder_গ্রন্থি_Vertex)
সর্বজনীন:
    explicit ReorderImpureVertex(V3Graph* graphp, Ast_গ্রন্থি* nodep)
        : Reorder_গ্রন্থি_Vertex{graphp, nodep} {}
    ~ReorderImpureVertex() override = পূর্বনির্ধারিত;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত "*IMPURE*"; }
    string dotColor() ধ্রুবক override { ফেরত "red"; }
};

শ্রেণী ReorderLogicVertex final : সর্বজনীন Reorder_গ্রন্থি_Vertex {
    VL_RTTI_IMPL(ReorderLogicVertex, Reorder_গ্রন্থি_Vertex)
সর্বজনীন:
    ReorderLogicVertex(V3Graph* graphp, Ast_গ্রন্থি* nodep)
        : Reorder_গ্রন্থি_Vertex{graphp, nodep} {}
    ~ReorderLogicVertex() override = পূর্বনির্ধারিত;
    string dotColor() ধ্রুবক override { ফেরত "black"; }
};

শ্রেণী ReorderVarStdVertex final : সর্বজনীন Reorder_গ্রন্থি_Vertex {
    VL_RTTI_IMPL(ReorderVarStdVertex, Reorder_গ্রন্থি_Vertex)
সর্বজনীন:
    ReorderVarStdVertex(V3Graph* graphp, AstVarScope* nodep)
        : Reorder_গ্রন্থি_Vertex{graphp, nodep} {}
    ~ReorderVarStdVertex() override = পূর্বনির্ধারিত;
    string dotColor() ধ্রুবক override { ফেরত "blue"; }
};

শ্রেণী ReorderVarPostVertex final : সর্বজনীন Reorder_গ্রন্থি_Vertex {
    VL_RTTI_IMPL(ReorderVarPostVertex, Reorder_গ্রন্থি_Vertex)
সর্বজনীন:
    ReorderVarPostVertex(V3Graph* graphp, AstVarScope* nodep)
        : Reorder_গ্রন্থি_Vertex{graphp, nodep} {}
    ~ReorderVarPostVertex() override = পূর্বনির্ধারিত;
    string name() ধ্রুবক override { ফেরত "POST "s + Reorder_গ্রন্থি_Vertex::name(); }
    string dotColor() ধ্রুবক override { ফেরত "green"; }
};

//######################################################################
// Edge types

শ্রেণী ReorderEdge VL_NOT_FINAL : সর্বজনীন V3GraphEdge {
    VL_RTTI_IMPL(ReorderEdge, V3GraphEdge)
    uint32_t m_ignoreInStep = 0;  // Step number that if set to, causes this edge to be ignored
    স্থির uint32_t s_step_অঙ্ক;  // Global step number
সুরক্ষিত:
    স্থির constexpr পূর্ণসংখ্যা WEIGHT_NORMAL = 10;
    ReorderEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীর্ষ, যৌক্তিক cutable)
        : V3GraphEdge{graphp, fromp, শীর্ষ, WEIGHT_NORMAL, cutable} {}
    ~ReorderEdge() override = পূর্বনির্ধারিত;

    আভাসী যৌক্তিক followScoreboard() ধ্রুবক = 0;

    std::string dotStyle() ধ্রুবক override {
        ফেরত ignoreThisStep() ? "dotted" : V3GraphEdge::dotStyle();
    }

সর্বজনীন:
    // Iterator for graph functions
    স্থির শূন্য incrementStep() { ++s_step_অঙ্ক; }
    যৌক্তিক ignoreThisStep() ধ্রুবক { ফেরত m_ignoreInStep == s_step_অঙ্ক; }
    শূন্য setIgnoreThisStep() { m_ignoreInStep = s_step_অঙ্ক; }

    স্থির যৌক্তিক followScoreboard(ধ্রুবক V3GraphEdge* edgep) {
        ধ্রুবক ReorderEdge& edge = *edgep->as<ReorderEdge>();
        ফেরত !edge.ignoreThisStep() && edge.followScoreboard();
    }
    স্থির যৌক্তিক followCyclic(ধ্রুবক V3GraphEdge* edgep) {
        ধ্রুবক ReorderEdge& edge = *edgep->as<ReorderEdge>();
        ফেরত !edge.ignoreThisStep();
    }
};
uint32_t ReorderEdge::s_step_অঙ্ক = 0;

শ্রেণী ReorderPostEdge final : সর্বজনীন ReorderEdge {
    VL_RTTI_IMPL(ReorderPostEdge, ReorderEdge)
সর্বজনীন:
    ReorderPostEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীর্ষ)
        : ReorderEdge{graphp, fromp, শীর্ষ, CUTABLE} {}
    ~ReorderPostEdge() override = পূর্বনির্ধারিত;
    যৌক্তিক followScoreboard() ধ্রুবক override { ফেরত মিথ্যা; }
    string dotColor() ধ্রুবক override { ফেরত "khaki"; }
};

শ্রেণী ReorderLVEdge final : সর্বজনীন ReorderEdge {
    VL_RTTI_IMPL(ReorderLVEdge, ReorderEdge)
সর্বজনীন:
    ReorderLVEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীর্ষ)
        : ReorderEdge{graphp, fromp, শীর্ষ, CUTABLE} {}
    ~ReorderLVEdge() override = পূর্বনির্ধারিত;
    যৌক্তিক followScoreboard() ধ্রুবক override { ফেরত সত্য; }
    string dotColor() ধ্রুবক override { ফেরত "yellowGreen"; }
};

শ্রেণী ReorderRVEdge final : সর্বজনীন ReorderEdge {
    VL_RTTI_IMPL(ReorderRVEdge, ReorderEdge)
সর্বজনীন:
    ReorderRVEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীর্ষ)
        : ReorderEdge{graphp, fromp, শীর্ষ, CUTABLE} {}
    ~ReorderRVEdge() override = পূর্বনির্ধারিত;
    যৌক্তিক followScoreboard() ধ্রুবক override { ফেরত সত্য; }
    string dotColor() ধ্রুবক override { ফেরত "green"; }
};

শ্রেণী ReorderScorebdEdge final : সর্বজনীন ReorderEdge {
    VL_RTTI_IMPL(ReorderScorebdEdge, ReorderEdge)
সর্বজনীন:
    ReorderScorebdEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীর্ষ)
        : ReorderEdge{graphp, fromp, শীর্ষ, CUTABLE} {}
    ~ReorderScorebdEdge() override = পূর্বনির্ধারিত;
    যৌক্তিক followScoreboard() ধ্রুবক override { ফেরত সত্য; }
    string dotColor() ধ্রুবক override { ফেরত "blue"; }
};

শ্রেণী ReorderStrictEdge final : সর্বজনীন ReorderEdge {
    VL_RTTI_IMPL(ReorderStrictEdge, ReorderEdge)
    // A strict order, based on the original statement order in the graph
    // The only non-cutable edge type
সর্বজনীন:
    ReorderStrictEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীর্ষ)
        : ReorderEdge{graphp, fromp, শীর্ষ, NOT_CUTABLE} {}
    ~ReorderStrictEdge() override = পূর্বনির্ধারিত;
    যৌক্তিক followScoreboard() ধ্রুবক override { ফেরত সত্য; }
    string dotColor() ধ্রুবক override { ফেরত "blue"; }
};

//######################################################################
// Reorder class functions

শ্রেণী ReorderVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> ReorderVarStdVertex*.  Usage var, 0=not set yet
    // AstVarScope::user2p  -> ReorderVarPostVertex*.  Delayed assignment var, 0=not set yet
    // Ast*::user3p         -> ReorderLogicVertex*.  Statement (temporary only)
    // Ast*::user4          -> int.  Current ordering number (reorderBlock usage)

    // STATE
    V3Graph* m_graphp = nullptr;  // Scoreboard of var usages/dependencies
    ReorderImpureVertex* m_impureVtxp = nullptr;  // Element specifying PLI ordering
    যৌক্তিক m_inDly = মিথ্যা;  // Inside ASSIGNDLY
    ধ্রুবক অক্ষর* m_noReorderWhy = nullptr;  // Reason we can't reorder
    std::vector<ReorderLogicVertex*> m_stmtStackps;  // Current statements being tracked

    // METHODS
    শূন্য cleanupBlockGraph(Ast_গ্রন্থি* nodep) {
        // Transform the graph into what we need
        UINFO(5, "ReorderBlock " << nodep);

        // Simplify graph by removing redundant edges
        m_graphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        যদি (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_nodup", মিথ্যা);

        // Mark all the logic for this step by setting Vertex::user() to true
        m_graphp->userClearVertices();
        জন্য (Ast_গ্রন্থি* nextp = nodep; nextp; nextp = nextp->nextp()) {
            nextp->user3u().to<ReorderLogicVertex*>()->user(সত্য);
        }

        // New step
        ReorderEdge::incrementStep();

        // If a var vertex has only inputs, it's a input-only node,
        // and can be ignored for coloring **this block only**
        জন্য (V3GraphVertex& vtx : m_graphp->vertices()) {
            যদি (!vtx.outEmpty()) চালু;
            যদি (!vtx.is<ReorderVarStdVertex>()) চালু;
            জন্য (V3GraphEdge& edge : vtx.inEdges()) edge.as<ReorderEdge>()->setIgnoreThisStep();
        }

        // For reordering this single block only, mark all logic
        // vertexes not involved with this step as unimportant
        জন্য (V3GraphVertex& vertex : m_graphp->vertices()) {
            যদি (vertex.user()) চালু;
            যদি (!vertex.is<ReorderLogicVertex>()) চালু;
            জন্য (V3GraphEdge& edge : vertex.inEdges()) edge.as<ReorderEdge>()->setIgnoreThisStep();
            জন্য (V3GraphEdge& edge : vertex.outEdges())
                edge.as<ReorderEdge>()->setIgnoreThisStep();
        }

        // Weak coloring to determine what needs to remain in order
        // This follows all step-relevant edges excluding PostEdges, which are done later
        m_graphp->weaklyConnected(&ReorderEdge::followScoreboard);

        // Add hard orderings between all nodes of same color, in the order they appeared
        std::unordered_map<uint32_t, ReorderLogicVertex*> lastOfColor;
        জন্য (Ast_গ্রন্থি* currp = nodep; currp; currp = currp->nextp()) {
            ReorderLogicVertex* ধ্রুবক vtxp = currp->user3u().to<ReorderLogicVertex*>();
            ধ্রুবক uint32_t color = vtxp->color();
            যদি (lastOfColor[color]) নতুন ReorderStrictEdge{m_graphp, lastOfColor[color], vtxp};
            lastOfColor[color] = vtxp;
        }

        // And a real ordering to get the statements into something reasonable
        // We don't care if there's cutable violations here...
        // Non-cutable violations should be impossible; as those edges are program-order
        যদি (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_pre", মিথ্যা);
        m_graphp->acyclic(&ReorderEdge::followCyclic);
        m_graphp->rank(&ReorderEdge::followCyclic);  // Or order(), but that's more expensive
        যদি (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_opt", মিথ্যা);
    }

    শূন্য reorderBlock(Ast_গ্রন্থি* nodep) {
        // Reorder statements in the completed graph

        // Map the rank numbers into nodes they associate with
        std::multimap<uint32_t, Ast_গ্রন্থি*> rankMap;
        পূর্ণসংখ্যা currOrder = 0;  // Existing sequence number of assignment
        জন্য (Ast_গ্রন্থি* currp = nodep; currp; currp = currp->nextp()) {
            ধ্রুবক ReorderLogicVertex* ধ্রুবক vtxp = currp->user3u().to<ReorderLogicVertex*>();
            rankMap.emplace(vtxp->rank(), currp);
            currp->user4(++currOrder);  // Record current ordering
        }

        // Is the current ordering OK?
        যৌক্তিক leaveAlone = সত্য;
        uint64_t newOrder = 0;  // New sequence number of assignment
        জন্য (ধ্রুবক auto& item : rankMap) {
            ধ্রুবক Ast_গ্রন্থি* ধ্রুবক nextp = item.second;
            যদি (++newOrder != nextp->user4()) leaveAlone = মিথ্যা;
        }
        যদি (leaveAlone) {
            UINFO(6, "   No changes");
            ফেরত;
        }

        VNRelinker replaceHandle;  // Where to add the list
        Ast_গ্রন্থি* newListp = nullptr;
        জন্য (ধ্রুবক auto& item : rankMap) {
            Ast_গ্রন্থি* ধ্রুবক nextp = item.second;
            UINFO(6, "   New order: " << nextp);
            nextp->unlinkFrBack(nextp == nodep ? &replaceHandle : nullptr);
            newListp = Ast_গ্রন্থি::add_পরবর্তী(newListp, nextp);
        }
        replaceHandle.relink(newListp);
    }

    শূন্য processBlock(Ast_গ্রন্থি* nodep) {
        যদি (m_noReorderWhy) ফেরত;

        // Empty lists are ignorable
        যদি (!nodep) ফেরত;
        UASSERT_OBJ(nodep->firstAbovep(), nodep, "Node passed is in not head of list");
        UASSERT_OBJ(!nodep->user3p(), nodep, "Should not have a logic vertex");

        // It nothing to reorder with, just iterate
        যদি (!nodep->nextp()) {
            iterate(nodep);
            ফেরত;
        }

        // Process it
        UINFO(9, "  processBlock " << nodep);

        // Iterate across current block, making the scoreboard
        জন্য (Ast_গ্রন্থি* currp = nodep; currp; currp = currp->nextp()) {
            // Create the logic vertex for this statement
            UASSERT_OBJ(!currp->user3p(), currp, "user3p should not be set");
            ReorderLogicVertex* ধ্রুবক vtxp = নতুন ReorderLogicVertex{m_graphp, currp};
            currp->user3p(vtxp);

            // Visit the statement - this can recursively reorder sub statements
            m_stmtStackps.ঠেলো_back(vtxp);
            iterate(currp);
            m_stmtStackps.বের_করো_back();
        }

        যদি (m_noReorderWhy) {  // Jump or something nasty
            UINFO(9, "  NoReorderBlock because " << m_noReorderWhy);
            ফেরত;
        }

        // Reorder statements in this block
        cleanupBlockGraph(nodep);
        reorderBlock(nodep);

        // 'nodep' might no longer be the head of the list, rewind
        যখন (nodep->backp()->nextp() == nodep) nodep = nodep->backp();

        // Delete vertexes and edges only applying to this block
        জন্য (Ast_গ্রন্থি* currp = nodep; currp; currp = currp->nextp()) {
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
        m_inDly = মিথ্যা;
        m_noReorderWhy = nullptr;

        ধ্রুবক VNUser1InUse user1InUse;
        ধ্রুবক VNUser2InUse user2InUse;
        ধ্রুবক VNUser3InUse user3InUse;
        ধ্রুবক VNUser4InUse user4InUse;

        UINFOTREE(9, nodep, "", "alwIn:");
        processBlock(nodep->stmtsp());
        UINFOTREE(9, nodep, "", "alwOut");

        m_stmtStackps.clear();
    }

    শূন্য visit(Ast_গ্রন্থি_If* nodep) override {
        যদি (!m_graphp || m_noReorderWhy) ফেরত;
        iterate_And_পরবর্তী_Null(nodep->condp());
        processBlock(nodep->thensp());
        processBlock(nodep->elsesp());
    }

    শূন্য visit(AstJumpGo* nodep) override {
        যদি (!m_graphp || m_noReorderWhy) ফেরত;
        m_noReorderWhy = "JumpGo";
        iterateChildren(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি_CCall* nodep) override {
        যদি (!m_graphp || m_noReorderWhy) ফেরত;
        // The callee is not inlined, so the variables it reads and writes carry no VarRef at
        // the call site, and the scoreboard would let assignments to them move across the
        // call. Treat any such call as a barrier.
        m_noReorderWhy = "CCall";
        iterateChildren(nodep);
    }

    শূন্য visit(Ast_রাশিমালা_Stmt* nodep) override {
        যদি (!m_graphp || m_noReorderWhy) ফেরত;
        VL_RESTORER(m_inDly);
        m_inDly = মিথ্যা;
        iterateChildren(nodep);
    }

    শূন্য visit(AstAssignDly* nodep) override {
        যদি (!m_graphp || m_noReorderWhy) ফেরত;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        যদি (Ast_গ্রন্থি* ধ্রুবক controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_inDly);
        m_inDly = সত্য;
        iterate(nodep->lhsp());
    }

    শূন্য visit(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        যদি (m_graphp) m_noReorderWhy = "Handle";
        visit(static_cast<Ast_গ্রন্থি*>(nodep));
    }

    শূন্য visit(AstVarRef* nodep) override {
        যদি (!m_graphp || m_noReorderWhy) ফেরত;
        যদি (m_stmtStackps.empty()) ফেরত;

        // Reads of constants can be ignored - TODO: This should be "constexpr", not run-time const
        যদি (nodep->varp()->isConst()) ফেরত;

        // SPEEDUP: We add duplicate edges, that should be fixed

        AstVarScope* ধ্রুবক vscp = nodep->varScopep();

        // Create vertexes for variable
        যদি (!vscp->user1p()) vscp->user1p(নতুন ReorderVarStdVertex{m_graphp, vscp});
        ReorderVarStdVertex* ধ্রুবক vstdp = vscp->user1u().to<ReorderVarStdVertex*>();

        // Variable is read
        যদি (nodep->access().isReadOnly()) {
            জন্য (ReorderLogicVertex* ধ্রুবক vtxp : m_stmtStackps) {
                নতুন ReorderRVEdge{m_graphp, vtxp, vstdp};
            }
            ফেরত;
        }

        // Variable is written, not NBA
        যদি (!m_inDly) {
            জন্য (ReorderLogicVertex* ধ্রুবক vtxp : m_stmtStackps) {
                নতুন ReorderLVEdge{m_graphp, vstdp, vtxp};
            }
            ফেরত;
        }

        // Variable is written by NBA
        যদি (!vscp->user2p()) {
            ReorderVarPostVertex* ধ্রুবক vpostp = নতুন ReorderVarPostVertex{m_graphp, vscp};
            vscp->user2p(vpostp);
            নতুন ReorderPostEdge{m_graphp, vstdp, vpostp};
        }
        ReorderVarPostVertex* ধ্রুবক vpostp = vscp->user2u().to<ReorderVarPostVertex*>();
        জন্য (ReorderLogicVertex* ধ্রুবক vtxp : m_stmtStackps) {
            নতুন ReorderLVEdge{m_graphp, vpostp, vtxp};
        }
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        // Outside AstAlways, just descend
        যদি (!m_graphp) {
            iterateChildren(nodep);
            ফেরত;
        }

        // Early exit if decided not to reorder
        যদি (m_noReorderWhy) ফেরত;

        // Timing control prevents reordering
        যদি (nodep->isTimingControl()) {
            m_noReorderWhy = "TimingControl";
            ফেরত;
        }

        // Order all impure statements with other impure statements
        যদি (!nodep->isPure()) {
            যদি (!m_impureVtxp) m_impureVtxp = নতুন ReorderImpureVertex{m_graphp, nodep};
            // This edge is only used to find weakly connected components, so one edge is enough
            জন্য (ReorderLogicVertex* ধ্রুবক vtxp : m_stmtStackps) {
                নতুন ReorderScorebdEdge{m_graphp, m_impureVtxp, vtxp};
            }
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
সর্বজনীন:
    explicit ReorderVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReorderVisitor() override = পূর্বনির্ধারিত;
};

}  // namespace

//######################################################################
// V3Reorder class functions

শূন্য V3Reorder::reorderAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReorderVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("reorder", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
