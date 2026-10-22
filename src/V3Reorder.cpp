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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Reorder.h"

#અન્તર્ભાવ "V3EmitV.h"
#અન્તર્ભાવ "V3Graph.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <string>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

નામાકાશ {

//######################################################################
// Support classes

વર્ગ Reorder_ગાંઠ_Vertex VL_NOT_FINAL : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(Reorder_ગાંઠ_Vertex, V3GraphVertex)
    Ast_ગાંઠ* અચલ m_nodep;

સુરક્ષિત:
    Reorder_ગાંઠ_Vertex(V3Graph* graphp, Ast_ગાંઠ* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~Reorder_ગાંઠ_Vertex() override = મૂળભૂત;
    // ACCESSORS
    // Do not make accessor for nodep(),  It may change due to
    // reordering a lower block, but we don't repair it
    std::string name() અચલ override {
        std::string str = cvtToHex(m_nodep) + '\n';
        જો (AstVarScope* અચલ vscp = VN_CAST(m_nodep, VarScope)) {
            str += vscp->prettyName();
        } નહીંતર {
            str += V3EmitV::debug_Verilog_For_વૃક્ષ(m_nodep);
            str = VString::quoteBackslash(str);
            str = VString::quoteAny(str, '"', '\\');
            str = VString::replaceSubstr(str, "\n", "\\l");
        }
        પાછા str;
    }
    File_લીટી* fileline() અચલ override { પાછા nodep()->fileline(); }
    std::string dotShape() અચલ override { પાછા VN_IS(m_nodep, VarScope) ? "ellipse" : "box"; }

જાહેર:
    આભાસી Ast_ગાંઠ* nodep() અચલ { પાછા m_nodep; }
};

વર્ગ ReorderImpureVertex final : જાહેર Reorder_ગાંઠ_Vertex {
    VL_RTTI_IMPL(ReorderImpureVertex, Reorder_ગાંઠ_Vertex)
જાહેર:
    explicit ReorderImpureVertex(V3Graph* graphp, Ast_ગાંઠ* nodep)
        : Reorder_ગાંઠ_Vertex{graphp, nodep} {}
    ~ReorderImpureVertex() override = મૂળભૂત;
    string name() અચલ override VL_MT_STABLE { પાછા "*IMPURE*"; }
    string dotColor() અચલ override { પાછા "red"; }
};

વર્ગ ReorderLogicVertex final : જાહેર Reorder_ગાંઠ_Vertex {
    VL_RTTI_IMPL(ReorderLogicVertex, Reorder_ગાંઠ_Vertex)
જાહેર:
    ReorderLogicVertex(V3Graph* graphp, Ast_ગાંઠ* nodep)
        : Reorder_ગાંઠ_Vertex{graphp, nodep} {}
    ~ReorderLogicVertex() override = મૂળભૂત;
    string dotColor() અચલ override { પાછા "black"; }
};

વર્ગ ReorderVarStdVertex final : જાહેર Reorder_ગાંઠ_Vertex {
    VL_RTTI_IMPL(ReorderVarStdVertex, Reorder_ગાંઠ_Vertex)
જાહેર:
    ReorderVarStdVertex(V3Graph* graphp, AstVarScope* nodep)
        : Reorder_ગાંઠ_Vertex{graphp, nodep} {}
    ~ReorderVarStdVertex() override = મૂળભૂત;
    string dotColor() અચલ override { પાછા "blue"; }
};

વર્ગ ReorderVarPostVertex final : જાહેર Reorder_ગાંઠ_Vertex {
    VL_RTTI_IMPL(ReorderVarPostVertex, Reorder_ગાંઠ_Vertex)
જાહેર:
    ReorderVarPostVertex(V3Graph* graphp, AstVarScope* nodep)
        : Reorder_ગાંઠ_Vertex{graphp, nodep} {}
    ~ReorderVarPostVertex() override = મૂળભૂત;
    string name() અચલ override { પાછા "POST "s + Reorder_ગાંઠ_Vertex::name(); }
    string dotColor() અચલ override { પાછા "green"; }
};

//######################################################################
// Edge types

વર્ગ ReorderEdge VL_NOT_FINAL : જાહેર V3GraphEdge {
    VL_RTTI_IMPL(ReorderEdge, V3GraphEdge)
    uint32_t m_ignoreInStep = 0;  // Step number that if set to, causes this edge to be ignored
    સ્થિર uint32_t s_step_અંક;  // Global step number
સુરક્ષિત:
    સ્થિર constexpr પૂર્ણાંક WEIGHT_NORMAL = 10;
    ReorderEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ટોચ, તાર્કિક cutable)
        : V3GraphEdge{graphp, fromp, ટોચ, WEIGHT_NORMAL, cutable} {}
    ~ReorderEdge() override = મૂળભૂત;

    આભાસી તાર્કિક followScoreboard() અચલ = 0;

    std::string dotStyle() અચલ override {
        પાછા ignoreThisStep() ? "dotted" : V3GraphEdge::dotStyle();
    }

જાહેર:
    // Iterator for graph functions
    સ્થિર શૂન્ય incrementStep() { ++s_step_અંક; }
    તાર્કિક ignoreThisStep() અચલ { પાછા m_ignoreInStep == s_step_અંક; }
    શૂન્ય setIgnoreThisStep() { m_ignoreInStep = s_step_અંક; }

    સ્થિર તાર્કિક followScoreboard(અચલ V3GraphEdge* edgep) {
        અચલ ReorderEdge& edge = *edgep->as<ReorderEdge>();
        પાછા !edge.ignoreThisStep() && edge.followScoreboard();
    }
    સ્થિર તાર્કિક followCyclic(અચલ V3GraphEdge* edgep) {
        અચલ ReorderEdge& edge = *edgep->as<ReorderEdge>();
        પાછા !edge.ignoreThisStep();
    }
};
uint32_t ReorderEdge::s_step_અંક = 0;

વર્ગ ReorderPostEdge final : જાહેર ReorderEdge {
    VL_RTTI_IMPL(ReorderPostEdge, ReorderEdge)
જાહેર:
    ReorderPostEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ટોચ)
        : ReorderEdge{graphp, fromp, ટોચ, CUTABLE} {}
    ~ReorderPostEdge() override = મૂળભૂત;
    તાર્કિક followScoreboard() અચલ override { પાછા ખોટું; }
    string dotColor() અચલ override { પાછા "khaki"; }
};

વર્ગ ReorderLVEdge final : જાહેર ReorderEdge {
    VL_RTTI_IMPL(ReorderLVEdge, ReorderEdge)
જાહેર:
    ReorderLVEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ટોચ)
        : ReorderEdge{graphp, fromp, ટોચ, CUTABLE} {}
    ~ReorderLVEdge() override = મૂળભૂત;
    તાર્કિક followScoreboard() અચલ override { પાછા સાચું; }
    string dotColor() અચલ override { પાછા "yellowGreen"; }
};

વર્ગ ReorderRVEdge final : જાહેર ReorderEdge {
    VL_RTTI_IMPL(ReorderRVEdge, ReorderEdge)
જાહેર:
    ReorderRVEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ટોચ)
        : ReorderEdge{graphp, fromp, ટોચ, CUTABLE} {}
    ~ReorderRVEdge() override = મૂળભૂત;
    તાર્કિક followScoreboard() અચલ override { પાછા સાચું; }
    string dotColor() અચલ override { પાછા "green"; }
};

વર્ગ ReorderScorebdEdge final : જાહેર ReorderEdge {
    VL_RTTI_IMPL(ReorderScorebdEdge, ReorderEdge)
જાહેર:
    ReorderScorebdEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ટોચ)
        : ReorderEdge{graphp, fromp, ટોચ, CUTABLE} {}
    ~ReorderScorebdEdge() override = મૂળભૂત;
    તાર્કિક followScoreboard() અચલ override { પાછા સાચું; }
    string dotColor() અચલ override { પાછા "blue"; }
};

વર્ગ ReorderStrictEdge final : જાહેર ReorderEdge {
    VL_RTTI_IMPL(ReorderStrictEdge, ReorderEdge)
    // A strict order, based on the original statement order in the graph
    // The only non-cutable edge type
જાહેર:
    ReorderStrictEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ટોચ)
        : ReorderEdge{graphp, fromp, ટોચ, NOT_CUTABLE} {}
    ~ReorderStrictEdge() override = મૂળભૂત;
    તાર્કિક followScoreboard() અચલ override { પાછા સાચું; }
    string dotColor() અચલ override { પાછા "blue"; }
};

//######################################################################
// Reorder class functions

વર્ગ ReorderVisitor final : જાહેર VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> ReorderVarStdVertex*.  Usage var, 0=not set yet
    // AstVarScope::user2p  -> ReorderVarPostVertex*.  Delayed assignment var, 0=not set yet
    // Ast*::user3p         -> ReorderLogicVertex*.  Statement (temporary only)
    // Ast*::user4          -> int.  Current ordering number (reorderBlock usage)

    // STATE
    V3Graph* m_graphp = nullptr;  // Scoreboard of var usages/dependencies
    ReorderImpureVertex* m_impureVtxp = nullptr;  // Element specifying PLI ordering
    તાર્કિક m_inDly = ખોટું;  // Inside ASSIGNDLY
    અચલ અક્ષર* m_noReorderWhy = nullptr;  // Reason we can't reorder
    std::vector<ReorderLogicVertex*> m_stmtStackps;  // Current statements being tracked

    // METHODS
    શૂન્ય cleanupBlockGraph(Ast_ગાંઠ* nodep) {
        // Transform the graph into what we need
        UINFO(5, "ReorderBlock " << nodep);

        // Simplify graph by removing redundant edges
        m_graphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        જો (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_nodup", ખોટું);

        // Mark all the logic for this step by setting Vertex::user() to true
        m_graphp->userClearVertices();
        માટે (Ast_ગાંઠ* nextp = nodep; nextp; nextp = nextp->nextp()) {
            nextp->user3u().to<ReorderLogicVertex*>()->user(સાચું);
        }

        // New step
        ReorderEdge::incrementStep();

        // If a var vertex has only inputs, it's a input-only node,
        // and can be ignored for coloring **this block only**
        માટે (V3GraphVertex& vtx : m_graphp->vertices()) {
            જો (!vtx.outEmpty()) ચાલુ;
            જો (!vtx.is<ReorderVarStdVertex>()) ચાલુ;
            માટે (V3GraphEdge& edge : vtx.inEdges()) edge.as<ReorderEdge>()->setIgnoreThisStep();
        }

        // For reordering this single block only, mark all logic
        // vertexes not involved with this step as unimportant
        માટે (V3GraphVertex& vertex : m_graphp->vertices()) {
            જો (vertex.user()) ચાલુ;
            જો (!vertex.is<ReorderLogicVertex>()) ચાલુ;
            માટે (V3GraphEdge& edge : vertex.inEdges()) edge.as<ReorderEdge>()->setIgnoreThisStep();
            માટે (V3GraphEdge& edge : vertex.outEdges())
                edge.as<ReorderEdge>()->setIgnoreThisStep();
        }

        // Weak coloring to determine what needs to remain in order
        // This follows all step-relevant edges excluding PostEdges, which are done later
        m_graphp->weaklyConnected(&ReorderEdge::followScoreboard);

        // Add hard orderings between all nodes of same color, in the order they appeared
        std::unordered_map<uint32_t, ReorderLogicVertex*> lastOfColor;
        માટે (Ast_ગાંઠ* currp = nodep; currp; currp = currp->nextp()) {
            ReorderLogicVertex* અચલ vtxp = currp->user3u().to<ReorderLogicVertex*>();
            અચલ uint32_t color = vtxp->color();
            જો (lastOfColor[color]) નવું ReorderStrictEdge{m_graphp, lastOfColor[color], vtxp};
            lastOfColor[color] = vtxp;
        }

        // And a real ordering to get the statements into something reasonable
        // We don't care if there's cutable violations here...
        // Non-cutable violations should be impossible; as those edges are program-order
        જો (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_pre", ખોટું);
        m_graphp->acyclic(&ReorderEdge::followCyclic);
        m_graphp->rank(&ReorderEdge::followCyclic);  // Or order(), but that's more expensive
        જો (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_opt", ખોટું);
    }

    શૂન્ય reorderBlock(Ast_ગાંઠ* nodep) {
        // Reorder statements in the completed graph

        // Map the rank numbers into nodes they associate with
        std::multimap<uint32_t, Ast_ગાંઠ*> rankMap;
        પૂર્ણાંક currOrder = 0;  // Existing sequence number of assignment
        માટે (Ast_ગાંઠ* currp = nodep; currp; currp = currp->nextp()) {
            અચલ ReorderLogicVertex* અચલ vtxp = currp->user3u().to<ReorderLogicVertex*>();
            rankMap.emplace(vtxp->rank(), currp);
            currp->user4(++currOrder);  // Record current ordering
        }

        // Is the current ordering OK?
        તાર્કિક leaveAlone = સાચું;
        uint64_t newOrder = 0;  // New sequence number of assignment
        માટે (અચલ auto& item : rankMap) {
            અચલ Ast_ગાંઠ* અચલ nextp = item.second;
            જો (++newOrder != nextp->user4()) leaveAlone = ખોટું;
        }
        જો (leaveAlone) {
            UINFO(6, "   No changes");
            પાછા;
        }

        VNRelinker replaceHandle;  // Where to add the list
        Ast_ગાંઠ* newListp = nullptr;
        માટે (અચલ auto& item : rankMap) {
            Ast_ગાંઠ* અચલ nextp = item.second;
            UINFO(6, "   New order: " << nextp);
            nextp->unlinkFrBack(nextp == nodep ? &replaceHandle : nullptr);
            newListp = Ast_ગાંઠ::addNext(newListp, nextp);
        }
        replaceHandle.relink(newListp);
    }

    શૂન્ય processBlock(Ast_ગાંઠ* nodep) {
        જો (m_noReorderWhy) પાછા;

        // Empty lists are ignorable
        જો (!nodep) પાછા;
        UASSERT_OBJ(nodep->firstAbovep(), nodep, "Node passed is in not head of list");
        UASSERT_OBJ(!nodep->user3p(), nodep, "Should not have a logic vertex");

        // It nothing to reorder with, just iterate
        જો (!nodep->nextp()) {
            iterate(nodep);
            પાછા;
        }

        // Process it
        UINFO(9, "  processBlock " << nodep);

        // Iterate across current block, making the scoreboard
        માટે (Ast_ગાંઠ* currp = nodep; currp; currp = currp->nextp()) {
            // Create the logic vertex for this statement
            UASSERT_OBJ(!currp->user3p(), currp, "user3p should not be set");
            ReorderLogicVertex* અચલ vtxp = નવું ReorderLogicVertex{m_graphp, currp};
            currp->user3p(vtxp);

            // Visit the statement - this can recursively reorder sub statements
            m_stmtStackps.ધકેલો_back(vtxp);
            iterate(currp);
            m_stmtStackps.કાઢો_back();
        }

        જો (m_noReorderWhy) {  // Jump or something nasty
            UINFO(9, "  NoReorderBlock because " << m_noReorderWhy);
            પાછા;
        }

        // Reorder statements in this block
        cleanupBlockGraph(nodep);
        reorderBlock(nodep);

        // 'nodep' might no longer be the head of the list, rewind
        જ્યારે (nodep->backp()->nextp() == nodep) nodep = nodep->backp();

        // Delete vertexes and edges only applying to this block
        માટે (Ast_ગાંઠ* currp = nodep; currp; currp = currp->nextp()) {
            currp->user3u().to<ReorderLogicVertex*>()->unlinkDelete(m_graphp);
            currp->user3p(nullptr);
        }
    }

    // VISITORS
    શૂન્ય visit(AstAlways* nodep) override {
        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_inDly);
        VL_RESTORER(m_noReorderWhy);

        V3Graph graph;
        m_graphp = &graph;
        m_impureVtxp = nullptr;
        m_inDly = ખોટું;
        m_noReorderWhy = nullptr;

        અચલ VNUser1InUse user1InUse;
        અચલ VNUser2InUse user2InUse;
        અચલ VNUser3InUse user3InUse;
        અચલ VNUser4InUse user4InUse;

        UINFOTREE(9, nodep, "", "alwIn:");
        processBlock(nodep->stmtsp());
        UINFOTREE(9, nodep, "", "alwOut");

        m_stmtStackps.clear();
    }

    શૂન્ય visit(Ast_ગાંઠ_If* nodep) override {
        જો (!m_graphp || m_noReorderWhy) પાછા;
        iterateAndNextNull(nodep->condp());
        processBlock(nodep->thensp());
        processBlock(nodep->elsesp());
    }

    શૂન્ય visit(AstJumpGo* nodep) override {
        જો (!m_graphp || m_noReorderWhy) પાછા;
        m_noReorderWhy = "JumpGo";
        iterateChildren(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ_CCall* nodep) override {
        જો (!m_graphp || m_noReorderWhy) પાછા;
        // The callee is not inlined, so the variables it reads and writes carry no VarRef at
        // the call site, and the scoreboard would let assignments to them move across the
        // call. Treat any such call as a barrier.
        m_noReorderWhy = "CCall";
        iterateChildren(nodep);
    }

    શૂન્ય visit(Ast_પદાવલિ_Stmt* nodep) override {
        જો (!m_graphp || m_noReorderWhy) પાછા;
        VL_RESTORER(m_inDly);
        m_inDly = ખોટું;
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstAssignDly* nodep) override {
        જો (!m_graphp || m_noReorderWhy) પાછા;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        જો (Ast_ગાંઠ* અચલ controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_inDly);
        m_inDly = સાચું;
        iterate(nodep->lhsp());
    }

    શૂન્ય visit(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        જો (m_graphp) m_noReorderWhy = "Handle";
        visit(static_cast<Ast_ગાંઠ*>(nodep));
    }

    શૂન્ય visit(AstVarRef* nodep) override {
        જો (!m_graphp || m_noReorderWhy) પાછા;
        જો (m_stmtStackps.empty()) પાછા;

        // Reads of constants can be ignored - TODO: This should be "constexpr", not run-time const
        જો (nodep->varp()->isConst()) પાછા;

        // SPEEDUP: We add duplicate edges, that should be fixed

        AstVarScope* અચલ vscp = nodep->varScopep();

        // Create vertexes for variable
        જો (!vscp->user1p()) vscp->user1p(નવું ReorderVarStdVertex{m_graphp, vscp});
        ReorderVarStdVertex* અચલ vstdp = vscp->user1u().to<ReorderVarStdVertex*>();

        // Variable is read
        જો (nodep->access().isReadOnly()) {
            માટે (ReorderLogicVertex* અચલ vtxp : m_stmtStackps) {
                નવું ReorderRVEdge{m_graphp, vtxp, vstdp};
            }
            પાછા;
        }

        // Variable is written, not NBA
        જો (!m_inDly) {
            માટે (ReorderLogicVertex* અચલ vtxp : m_stmtStackps) {
                નવું ReorderLVEdge{m_graphp, vstdp, vtxp};
            }
            પાછા;
        }

        // Variable is written by NBA
        જો (!vscp->user2p()) {
            ReorderVarPostVertex* અચલ vpostp = નવું ReorderVarPostVertex{m_graphp, vscp};
            vscp->user2p(vpostp);
            નવું ReorderPostEdge{m_graphp, vstdp, vpostp};
        }
        ReorderVarPostVertex* અચલ vpostp = vscp->user2u().to<ReorderVarPostVertex*>();
        માટે (ReorderLogicVertex* અચલ vtxp : m_stmtStackps) {
            નવું ReorderLVEdge{m_graphp, vpostp, vtxp};
        }
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        // Outside AstAlways, just descend
        જો (!m_graphp) {
            iterateChildren(nodep);
            પાછા;
        }

        // Early exit if decided not to reorder
        જો (m_noReorderWhy) પાછા;

        // Timing control prevents reordering
        જો (nodep->isTimingControl()) {
            m_noReorderWhy = "TimingControl";
            પાછા;
        }

        // Order all impure statements with other impure statements
        જો (!nodep->isPure()) {
            જો (!m_impureVtxp) m_impureVtxp = નવું ReorderImpureVertex{m_graphp, nodep};
            // This edge is only used to find weakly connected components, so one edge is enough
            માટે (ReorderLogicVertex* અચલ vtxp : m_stmtStackps) {
                નવું ReorderScorebdEdge{m_graphp, m_impureVtxp, vtxp};
            }
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
જાહેર:
    explicit ReorderVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReorderVisitor() override = મૂળભૂત;
};

}  // namespace

//######################################################################
// V3Reorder class functions

શૂન્ય V3Reorder::reorderAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReorderVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("reorder", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
