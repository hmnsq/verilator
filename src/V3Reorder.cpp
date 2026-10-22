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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Reorder.h"

#समावेश "V3EmitV.h"
#समावेश "V3Graph.h"
#समावेश "V3Stats.h"

#समावेश <string>
#समावेश <unordered_map>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

namespace {

//######################################################################
// Support classes

श्रेणी Reorder_गाँठ_Vertex VL_NOT_FINAL : खुला V3GraphVertex {
    VL_RTTI_IMPL(Reorder_गाँठ_Vertex, V3GraphVertex)
    Ast_गाँठ* स्थिर m_nodep;

रक्षित:
    Reorder_गाँठ_Vertex(V3Graph* graphp, Ast_गाँठ* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~Reorder_गाँठ_Vertex() override = शेष;
    // ACCESSORS
    // Do not make accessor for nodep(),  It may change due to
    // reordering a lower block, but we don't repair it
    std::string name() स्थिर override {
        std::string str = cvtToHex(m_nodep) + '\n';
        अगर (AstVarScope* स्थिर vscp = VN_CAST(m_nodep, VarScope)) {
            str += vscp->prettyName();
        } अन्यथा {
            str += V3EmitV::debug_Verilog_For_वृक्ष(m_nodep);
            str = VString::quoteBackslash(str);
            str = VString::quoteAny(str, '"', '\\');
            str = VString::replaceSubstr(str, "\n", "\\l");
        }
        वापस str;
    }
    File_पंक्ति* fileline() स्थिर override { वापस nodep()->fileline(); }
    std::string dotShape() स्थिर override { वापस VN_IS(m_nodep, VarScope) ? "ellipse" : "box"; }

खुला:
    भव Ast_गाँठ* nodep() स्थिर { वापस m_nodep; }
};

श्रेणी ReorderImpureVertex final : खुला Reorder_गाँठ_Vertex {
    VL_RTTI_IMPL(ReorderImpureVertex, Reorder_गाँठ_Vertex)
खुला:
    explicit ReorderImpureVertex(V3Graph* graphp, Ast_गाँठ* nodep)
        : Reorder_गाँठ_Vertex{graphp, nodep} {}
    ~ReorderImpureVertex() override = शेष;
    string name() स्थिर override VL_MT_STABLE { वापस "*IMPURE*"; }
    string dotColor() स्थिर override { वापस "red"; }
};

श्रेणी ReorderLogicVertex final : खुला Reorder_गाँठ_Vertex {
    VL_RTTI_IMPL(ReorderLogicVertex, Reorder_गाँठ_Vertex)
खुला:
    ReorderLogicVertex(V3Graph* graphp, Ast_गाँठ* nodep)
        : Reorder_गाँठ_Vertex{graphp, nodep} {}
    ~ReorderLogicVertex() override = शेष;
    string dotColor() स्थिर override { वापस "black"; }
};

श्रेणी ReorderVarStdVertex final : खुला Reorder_गाँठ_Vertex {
    VL_RTTI_IMPL(ReorderVarStdVertex, Reorder_गाँठ_Vertex)
खुला:
    ReorderVarStdVertex(V3Graph* graphp, AstVarScope* nodep)
        : Reorder_गाँठ_Vertex{graphp, nodep} {}
    ~ReorderVarStdVertex() override = शेष;
    string dotColor() स्थिर override { वापस "blue"; }
};

श्रेणी ReorderVarPostVertex final : खुला Reorder_गाँठ_Vertex {
    VL_RTTI_IMPL(ReorderVarPostVertex, Reorder_गाँठ_Vertex)
खुला:
    ReorderVarPostVertex(V3Graph* graphp, AstVarScope* nodep)
        : Reorder_गाँठ_Vertex{graphp, nodep} {}
    ~ReorderVarPostVertex() override = शेष;
    string name() स्थिर override { वापस "POST "s + Reorder_गाँठ_Vertex::name(); }
    string dotColor() स्थिर override { वापस "green"; }
};

//######################################################################
// Edge types

श्रेणी ReorderEdge VL_NOT_FINAL : खुला V3GraphEdge {
    VL_RTTI_IMPL(ReorderEdge, V3GraphEdge)
    uint32_t m_ignoreInStep = 0;  // Step number that if set to, causes this edge to be ignored
    अटल uint32_t s_step_अंक;  // Global step number
रक्षित:
    अटल constexpr पूर्णांक WEIGHT_NORMAL = 10;
    ReorderEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष, bool cutable)
        : V3GraphEdge{graphp, fromp, शीर्ष, WEIGHT_NORMAL, cutable} {}
    ~ReorderEdge() override = शेष;

    भव bool followScoreboard() स्थिर = 0;

    std::string dotStyle() स्थिर override {
        वापस ignoreThisStep() ? "dotted" : V3GraphEdge::dotStyle();
    }

खुला:
    // Iterator for graph functions
    अटल व्योम incrementStep() { ++s_step_अंक; }
    bool ignoreThisStep() स्थिर { वापस m_ignoreInStep == s_step_अंक; }
    व्योम setIgnoreThisStep() { m_ignoreInStep = s_step_अंक; }

    अटल bool followScoreboard(स्थिर V3GraphEdge* edgep) {
        स्थिर ReorderEdge& edge = *edgep->as<ReorderEdge>();
        वापस !edge.ignoreThisStep() && edge.followScoreboard();
    }
    अटल bool followCyclic(स्थिर V3GraphEdge* edgep) {
        स्थिर ReorderEdge& edge = *edgep->as<ReorderEdge>();
        वापस !edge.ignoreThisStep();
    }
};
uint32_t ReorderEdge::s_step_अंक = 0;

श्रेणी ReorderPostEdge final : खुला ReorderEdge {
    VL_RTTI_IMPL(ReorderPostEdge, ReorderEdge)
खुला:
    ReorderPostEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष)
        : ReorderEdge{graphp, fromp, शीर्ष, CUTABLE} {}
    ~ReorderPostEdge() override = शेष;
    bool followScoreboard() स्थिर override { वापस false; }
    string dotColor() स्थिर override { वापस "khaki"; }
};

श्रेणी ReorderLVEdge final : खुला ReorderEdge {
    VL_RTTI_IMPL(ReorderLVEdge, ReorderEdge)
खुला:
    ReorderLVEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष)
        : ReorderEdge{graphp, fromp, शीर्ष, CUTABLE} {}
    ~ReorderLVEdge() override = शेष;
    bool followScoreboard() स्थिर override { वापस true; }
    string dotColor() स्थिर override { वापस "yellowGreen"; }
};

श्रेणी ReorderRVEdge final : खुला ReorderEdge {
    VL_RTTI_IMPL(ReorderRVEdge, ReorderEdge)
खुला:
    ReorderRVEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष)
        : ReorderEdge{graphp, fromp, शीर्ष, CUTABLE} {}
    ~ReorderRVEdge() override = शेष;
    bool followScoreboard() स्थिर override { वापस true; }
    string dotColor() स्थिर override { वापस "green"; }
};

श्रेणी ReorderScorebdEdge final : खुला ReorderEdge {
    VL_RTTI_IMPL(ReorderScorebdEdge, ReorderEdge)
खुला:
    ReorderScorebdEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष)
        : ReorderEdge{graphp, fromp, शीर्ष, CUTABLE} {}
    ~ReorderScorebdEdge() override = शेष;
    bool followScoreboard() स्थिर override { वापस true; }
    string dotColor() स्थिर override { वापस "blue"; }
};

श्रेणी ReorderStrictEdge final : खुला ReorderEdge {
    VL_RTTI_IMPL(ReorderStrictEdge, ReorderEdge)
    // A strict order, based on the original statement order in the graph
    // The only non-cutable edge type
खुला:
    ReorderStrictEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष)
        : ReorderEdge{graphp, fromp, शीर्ष, NOT_CUTABLE} {}
    ~ReorderStrictEdge() override = शेष;
    bool followScoreboard() स्थिर override { वापस true; }
    string dotColor() स्थिर override { वापस "blue"; }
};

//######################################################################
// Reorder class functions

श्रेणी ReorderVisitor final : खुला VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> ReorderVarStdVertex*.  Usage var, 0=not set yet
    // AstVarScope::user2p  -> ReorderVarPostVertex*.  Delayed assignment var, 0=not set yet
    // Ast*::user3p         -> ReorderLogicVertex*.  Statement (temporary only)
    // Ast*::user4          -> int.  Current ordering number (reorderBlock usage)

    // STATE
    V3Graph* m_graphp = nullptr;  // Scoreboard of var usages/dependencies
    ReorderImpureVertex* m_impureVtxp = nullptr;  // Element specifying PLI ordering
    bool m_inDly = false;  // Inside ASSIGNDLY
    स्थिर अक्षर* m_noReorderWhy = nullptr;  // Reason we can't reorder
    std::vector<ReorderLogicVertex*> m_stmtStackps;  // Current statements being tracked

    // METHODS
    व्योम cleanupBlockGraph(Ast_गाँठ* nodep) {
        // Transform the graph into what we need
        UINFO(5, "ReorderBlock " << nodep);

        // Simplify graph by removing redundant edges
        m_graphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        अगर (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_nodup", false);

        // Mark all the logic for this step by setting Vertex::user() to true
        m_graphp->userClearVertices();
        क्रम (Ast_गाँठ* nextp = nodep; nextp; nextp = nextp->nextp()) {
            nextp->user3u().to<ReorderLogicVertex*>()->user(true);
        }

        // New step
        ReorderEdge::incrementStep();

        // If a var vertex has only inputs, it's a input-only node,
        // and can be ignored for coloring **this block only**
        क्रम (V3GraphVertex& vtx : m_graphp->vertices()) {
            अगर (!vtx.outEmpty()) जारी;
            अगर (!vtx.is<ReorderVarStdVertex>()) जारी;
            क्रम (V3GraphEdge& edge : vtx.inEdges()) edge.as<ReorderEdge>()->setIgnoreThisStep();
        }

        // For reordering this single block only, mark all logic
        // vertexes not involved with this step as unimportant
        क्रम (V3GraphVertex& vertex : m_graphp->vertices()) {
            अगर (vertex.user()) जारी;
            अगर (!vertex.is<ReorderLogicVertex>()) जारी;
            क्रम (V3GraphEdge& edge : vertex.inEdges()) edge.as<ReorderEdge>()->setIgnoreThisStep();
            क्रम (V3GraphEdge& edge : vertex.outEdges())
                edge.as<ReorderEdge>()->setIgnoreThisStep();
        }

        // Weak coloring to determine what needs to remain in order
        // This follows all step-relevant edges excluding PostEdges, which are done later
        m_graphp->weaklyConnected(&ReorderEdge::followScoreboard);

        // Add hard orderings between all nodes of same color, in the order they appeared
        std::unordered_map<uint32_t, ReorderLogicVertex*> lastOfColor;
        क्रम (Ast_गाँठ* currp = nodep; currp; currp = currp->nextp()) {
            ReorderLogicVertex* स्थिर vtxp = currp->user3u().to<ReorderLogicVertex*>();
            स्थिर uint32_t color = vtxp->color();
            अगर (lastOfColor[color]) नया ReorderStrictEdge{m_graphp, lastOfColor[color], vtxp};
            lastOfColor[color] = vtxp;
        }

        // And a real ordering to get the statements into something reasonable
        // We don't care if there's cutable violations here...
        // Non-cutable violations should be impossible; as those edges are program-order
        अगर (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_pre", false);
        m_graphp->acyclic(&ReorderEdge::followCyclic);
        m_graphp->rank(&ReorderEdge::followCyclic);  // Or order(), but that's more expensive
        अगर (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_opt", false);
    }

    व्योम reorderBlock(Ast_गाँठ* nodep) {
        // Reorder statements in the completed graph

        // Map the rank numbers into nodes they associate with
        std::multimap<uint32_t, Ast_गाँठ*> rankMap;
        पूर्णांक currOrder = 0;  // Existing sequence number of assignment
        क्रम (Ast_गाँठ* currp = nodep; currp; currp = currp->nextp()) {
            स्थिर ReorderLogicVertex* स्थिर vtxp = currp->user3u().to<ReorderLogicVertex*>();
            rankMap.emplace(vtxp->rank(), currp);
            currp->user4(++currOrder);  // Record current ordering
        }

        // Is the current ordering OK?
        bool leaveAlone = true;
        uint64_t newOrder = 0;  // New sequence number of assignment
        क्रम (स्थिर स्वतः& item : rankMap) {
            स्थिर Ast_गाँठ* स्थिर nextp = item.second;
            अगर (++newOrder != nextp->user4()) leaveAlone = false;
        }
        अगर (leaveAlone) {
            UINFO(6, "   No changes");
            वापस;
        }

        VNRelinker replaceHandle;  // Where to add the list
        Ast_गाँठ* newListp = nullptr;
        क्रम (स्थिर स्वतः& item : rankMap) {
            Ast_गाँठ* स्थिर nextp = item.second;
            UINFO(6, "   New order: " << nextp);
            nextp->unlinkFrBack(nextp == nodep ? &replaceHandle : nullptr);
            newListp = Ast_गाँठ::add_अग्र(newListp, nextp);
        }
        replaceHandle.relink(newListp);
    }

    व्योम processBlock(Ast_गाँठ* nodep) {
        अगर (m_noReorderWhy) वापस;

        // Empty lists are ignorable
        अगर (!nodep) वापस;
        UASSERT_OBJ(nodep->firstAbovep(), nodep, "Node passed is in not head of list");
        UASSERT_OBJ(!nodep->user3p(), nodep, "Should not have a logic vertex");

        // It nothing to reorder with, just iterate
        अगर (!nodep->nextp()) {
            iterate(nodep);
            वापस;
        }

        // Process it
        UINFO(9, "  processBlock " << nodep);

        // Iterate across current block, making the scoreboard
        क्रम (Ast_गाँठ* currp = nodep; currp; currp = currp->nextp()) {
            // Create the logic vertex for this statement
            UASSERT_OBJ(!currp->user3p(), currp, "user3p should not be set");
            ReorderLogicVertex* स्थिर vtxp = नया ReorderLogicVertex{m_graphp, currp};
            currp->user3p(vtxp);

            // Visit the statement - this can recursively reorder sub statements
            m_stmtStackps.धकेलो_back(vtxp);
            iterate(currp);
            m_stmtStackps.निकालो_back();
        }

        अगर (m_noReorderWhy) {  // Jump or something nasty
            UINFO(9, "  NoReorderBlock because " << m_noReorderWhy);
            वापस;
        }

        // Reorder statements in this block
        cleanupBlockGraph(nodep);
        reorderBlock(nodep);

        // 'nodep' might no longer be the head of the list, rewind
        जबतक (nodep->backp()->nextp() == nodep) nodep = nodep->backp();

        // Delete vertexes and edges only applying to this block
        क्रम (Ast_गाँठ* currp = nodep; currp; currp = currp->nextp()) {
            currp->user3u().to<ReorderLogicVertex*>()->unlinkDelete(m_graphp);
            currp->user3p(nullptr);
        }
    }

    // VISITORS
    व्योम visit(AstAlways* nodep) override {
        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_inDly);
        VL_RESTORER(m_noReorderWhy);

        V3Graph graph;
        m_graphp = &graph;
        m_impureVtxp = nullptr;
        m_inDly = false;
        m_noReorderWhy = nullptr;

        स्थिर VNUser1InUse user1InUse;
        स्थिर VNUser2InUse user2InUse;
        स्थिर VNUser3InUse user3InUse;
        स्थिर VNUser4InUse user4InUse;

        UINFOTREE(9, nodep, "", "alwIn:");
        processBlock(nodep->stmtsp());
        UINFOTREE(9, nodep, "", "alwOut");

        m_stmtStackps.सफा();
    }

    व्योम visit(Ast_गाँठ_If* nodep) override {
        अगर (!m_graphp || m_noReorderWhy) वापस;
        iterate_And_अग्र_Null(nodep->condp());
        processBlock(nodep->thensp());
        processBlock(nodep->elsesp());
    }

    व्योम visit(AstJumpGo* nodep) override {
        अगर (!m_graphp || m_noReorderWhy) वापस;
        m_noReorderWhy = "JumpGo";
        iterateChildren(nodep);
    }

    व्योम visit(Ast_गाँठ_CCall* nodep) override {
        अगर (!m_graphp || m_noReorderWhy) वापस;
        // The callee is not inlined, so the variables it reads and writes carry no VarRef at
        // the call site, and the scoreboard would let assignments to them move across the
        // call. Treat any such call as a barrier.
        m_noReorderWhy = "CCall";
        iterateChildren(nodep);
    }

    व्योम visit(Ast_व्यंजक_Stmt* nodep) override {
        अगर (!m_graphp || m_noReorderWhy) वापस;
        VL_RESTORER(m_inDly);
        m_inDly = false;
        iterateChildren(nodep);
    }

    व्योम visit(AstAssignDly* nodep) override {
        अगर (!m_graphp || m_noReorderWhy) वापस;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        अगर (Ast_गाँठ* स्थिर controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_inDly);
        m_inDly = true;
        iterate(nodep->lhsp());
    }

    व्योम visit(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        अगर (m_graphp) m_noReorderWhy = "Handle";
        visit(static_cast<Ast_गाँठ*>(nodep));
    }

    व्योम visit(AstVarRef* nodep) override {
        अगर (!m_graphp || m_noReorderWhy) वापस;
        अगर (m_stmtStackps.empty()) वापस;

        // Reads of constants can be ignored - TODO: This should be "constexpr", not run-time const
        अगर (nodep->varp()->isConst()) वापस;

        // SPEEDUP: We add duplicate edges, that should be fixed

        AstVarScope* स्थिर vscp = nodep->varScopep();

        // Create vertexes for variable
        अगर (!vscp->user1p()) vscp->user1p(नया ReorderVarStdVertex{m_graphp, vscp});
        ReorderVarStdVertex* स्थिर vstdp = vscp->user1u().to<ReorderVarStdVertex*>();

        // Variable is read
        अगर (nodep->access().isReadOnly()) {
            क्रम (ReorderLogicVertex* स्थिर vtxp : m_stmtStackps) {
                नया ReorderRVEdge{m_graphp, vtxp, vstdp};
            }
            वापस;
        }

        // Variable is written, not NBA
        अगर (!m_inDly) {
            क्रम (ReorderLogicVertex* स्थिर vtxp : m_stmtStackps) {
                नया ReorderLVEdge{m_graphp, vstdp, vtxp};
            }
            वापस;
        }

        // Variable is written by NBA
        अगर (!vscp->user2p()) {
            ReorderVarPostVertex* स्थिर vpostp = नया ReorderVarPostVertex{m_graphp, vscp};
            vscp->user2p(vpostp);
            नया ReorderPostEdge{m_graphp, vstdp, vpostp};
        }
        ReorderVarPostVertex* स्थिर vpostp = vscp->user2u().to<ReorderVarPostVertex*>();
        क्रम (ReorderLogicVertex* स्थिर vtxp : m_stmtStackps) {
            नया ReorderLVEdge{m_graphp, vpostp, vtxp};
        }
    }

    व्योम visit(Ast_गाँठ* nodep) override {
        // Outside AstAlways, just descend
        अगर (!m_graphp) {
            iterateChildren(nodep);
            वापस;
        }

        // Early exit if decided not to reorder
        अगर (m_noReorderWhy) वापस;

        // Timing control prevents reordering
        अगर (nodep->isTimingControl()) {
            m_noReorderWhy = "TimingControl";
            वापस;
        }

        // Order all impure statements with other impure statements
        अगर (!nodep->isPure()) {
            अगर (!m_impureVtxp) m_impureVtxp = नया ReorderImpureVertex{m_graphp, nodep};
            // This edge is only used to find weakly connected components, so one edge is enough
            क्रम (ReorderLogicVertex* स्थिर vtxp : m_stmtStackps) {
                नया ReorderScorebdEdge{m_graphp, m_impureVtxp, vtxp};
            }
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
खुला:
    explicit ReorderVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReorderVisitor() override = शेष;
};

}  // namespace

//######################################################################
// V3Reorder class functions

व्योम V3Reorder::reorderAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReorderVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("reorder", 0, dump_वृक्ष_Either_Level() >= 3);
}
