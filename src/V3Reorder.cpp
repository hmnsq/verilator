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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Reorder.h"

#அந்தர்பாவ "V3EmitV.h"
#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <string>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

நாமாகாஶ {

//######################################################################
// Support classes

வகுப்பு Reorder_முனை_Vertex VL_NOT_FINAL : பொது V3GraphVertex {
    VL_RTTI_IMPL(Reorder_முனை_Vertex, V3GraphVertex)
    Ast_முனை* மாறிலி m_nodep;

பாதுகாக்கப்பட்ட:
    Reorder_முனை_Vertex(V3Graph* graphp, Ast_முனை* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~Reorder_முனை_Vertex() override = இயல்பு;
    // ACCESSORS
    // Do not make accessor for nodep(),  It may change due to
    // reordering a lower block, but we don't repair it
    std::string name() மாறிலி override {
        std::string str = cvtToHex(m_nodep) + '\n';
        எனில் (AstVarScope* மாறிலி vscp = VN_CAST(m_nodep, VarScope)) {
            str += vscp->prettyName();
        } இல்லையெனில் {
            str += V3EmitV::debug_Verilog_For_மரம்(m_nodep);
            str = VString::quoteBackslash(str);
            str = VString::quoteAny(str, '"', '\\');
            str = VString::replaceSubstr(str, "\n", "\\l");
        }
        திரும்பு str;
    }
    File_வரி* fileline() மாறிலி override { திரும்பு nodep()->fileline(); }
    std::string dotShape() மாறிலி override { திரும்பு VN_IS(m_nodep, VarScope) ? "ellipse" : "box"; }

பொது:
    மெய்நிகர் Ast_முனை* nodep() மாறிலி { திரும்பு m_nodep; }
};

வகுப்பு ReorderImpureVertex final : பொது Reorder_முனை_Vertex {
    VL_RTTI_IMPL(ReorderImpureVertex, Reorder_முனை_Vertex)
பொது:
    explicit ReorderImpureVertex(V3Graph* graphp, Ast_முனை* nodep)
        : Reorder_முனை_Vertex{graphp, nodep} {}
    ~ReorderImpureVertex() override = இயல்பு;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு "*IMPURE*"; }
    string dotColor() மாறிலி override { திரும்பு "red"; }
};

வகுப்பு ReorderLogicVertex final : பொது Reorder_முனை_Vertex {
    VL_RTTI_IMPL(ReorderLogicVertex, Reorder_முனை_Vertex)
பொது:
    ReorderLogicVertex(V3Graph* graphp, Ast_முனை* nodep)
        : Reorder_முனை_Vertex{graphp, nodep} {}
    ~ReorderLogicVertex() override = இயல்பு;
    string dotColor() மாறிலி override { திரும்பு "black"; }
};

வகுப்பு ReorderVarStdVertex final : பொது Reorder_முனை_Vertex {
    VL_RTTI_IMPL(ReorderVarStdVertex, Reorder_முனை_Vertex)
பொது:
    ReorderVarStdVertex(V3Graph* graphp, AstVarScope* nodep)
        : Reorder_முனை_Vertex{graphp, nodep} {}
    ~ReorderVarStdVertex() override = இயல்பு;
    string dotColor() மாறிலி override { திரும்பு "blue"; }
};

வகுப்பு ReorderVarPostVertex final : பொது Reorder_முனை_Vertex {
    VL_RTTI_IMPL(ReorderVarPostVertex, Reorder_முனை_Vertex)
பொது:
    ReorderVarPostVertex(V3Graph* graphp, AstVarScope* nodep)
        : Reorder_முனை_Vertex{graphp, nodep} {}
    ~ReorderVarPostVertex() override = இயல்பு;
    string name() மாறிலி override { திரும்பு "POST "s + Reorder_முனை_Vertex::name(); }
    string dotColor() மாறிலி override { திரும்பு "green"; }
};

//######################################################################
// Edge types

வகுப்பு ReorderEdge VL_NOT_FINAL : பொது V3GraphEdge {
    VL_RTTI_IMPL(ReorderEdge, V3GraphEdge)
    uint32_t m_ignoreInStep = 0;  // Step number that if set to, causes this edge to be ignored
    நிலையான uint32_t s_step_இலக்கம்;  // Global step number
பாதுகாக்கப்பட்ட:
    நிலையான constexpr முழு WEIGHT_NORMAL = 10;
    ReorderEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* உச்சி, தருக்க cutable)
        : V3GraphEdge{graphp, fromp, உச்சி, WEIGHT_NORMAL, cutable} {}
    ~ReorderEdge() override = இயல்பு;

    மெய்நிகர் தருக்க followScoreboard() மாறிலி = 0;

    std::string dotStyle() மாறிலி override {
        திரும்பு ignoreThisStep() ? "dotted" : V3GraphEdge::dotStyle();
    }

பொது:
    // Iterator for graph functions
    நிலையான வெறுமை incrementStep() { ++s_step_இலக்கம்; }
    தருக்க ignoreThisStep() மாறிலி { திரும்பு m_ignoreInStep == s_step_இலக்கம்; }
    வெறுமை setIgnoreThisStep() { m_ignoreInStep = s_step_இலக்கம்; }

    நிலையான தருக்க followScoreboard(மாறிலி V3GraphEdge* edgep) {
        மாறிலி ReorderEdge& edge = *edgep->as<ReorderEdge>();
        திரும்பு !edge.ignoreThisStep() && edge.followScoreboard();
    }
    நிலையான தருக்க followCyclic(மாறிலி V3GraphEdge* edgep) {
        மாறிலி ReorderEdge& edge = *edgep->as<ReorderEdge>();
        திரும்பு !edge.ignoreThisStep();
    }
};
uint32_t ReorderEdge::s_step_இலக்கம் = 0;

வகுப்பு ReorderPostEdge final : பொது ReorderEdge {
    VL_RTTI_IMPL(ReorderPostEdge, ReorderEdge)
பொது:
    ReorderPostEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* உச்சி)
        : ReorderEdge{graphp, fromp, உச்சி, CUTABLE} {}
    ~ReorderPostEdge() override = இயல்பு;
    தருக்க followScoreboard() மாறிலி override { திரும்பு பொய்; }
    string dotColor() மாறிலி override { திரும்பு "khaki"; }
};

வகுப்பு ReorderLVEdge final : பொது ReorderEdge {
    VL_RTTI_IMPL(ReorderLVEdge, ReorderEdge)
பொது:
    ReorderLVEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* உச்சி)
        : ReorderEdge{graphp, fromp, உச்சி, CUTABLE} {}
    ~ReorderLVEdge() override = இயல்பு;
    தருக்க followScoreboard() மாறிலி override { திரும்பு உண்மை; }
    string dotColor() மாறிலி override { திரும்பு "yellowGreen"; }
};

வகுப்பு ReorderRVEdge final : பொது ReorderEdge {
    VL_RTTI_IMPL(ReorderRVEdge, ReorderEdge)
பொது:
    ReorderRVEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* உச்சி)
        : ReorderEdge{graphp, fromp, உச்சி, CUTABLE} {}
    ~ReorderRVEdge() override = இயல்பு;
    தருக்க followScoreboard() மாறிலி override { திரும்பு உண்மை; }
    string dotColor() மாறிலி override { திரும்பு "green"; }
};

வகுப்பு ReorderScorebdEdge final : பொது ReorderEdge {
    VL_RTTI_IMPL(ReorderScorebdEdge, ReorderEdge)
பொது:
    ReorderScorebdEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* உச்சி)
        : ReorderEdge{graphp, fromp, உச்சி, CUTABLE} {}
    ~ReorderScorebdEdge() override = இயல்பு;
    தருக்க followScoreboard() மாறிலி override { திரும்பு உண்மை; }
    string dotColor() மாறிலி override { திரும்பு "blue"; }
};

வகுப்பு ReorderStrictEdge final : பொது ReorderEdge {
    VL_RTTI_IMPL(ReorderStrictEdge, ReorderEdge)
    // A strict order, based on the original statement order in the graph
    // The only non-cutable edge type
பொது:
    ReorderStrictEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* உச்சி)
        : ReorderEdge{graphp, fromp, உச்சி, NOT_CUTABLE} {}
    ~ReorderStrictEdge() override = இயல்பு;
    தருக்க followScoreboard() மாறிலி override { திரும்பு உண்மை; }
    string dotColor() மாறிலி override { திரும்பு "blue"; }
};

//######################################################################
// Reorder class functions

வகுப்பு ReorderVisitor final : பொது VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> ReorderVarStdVertex*.  Usage var, 0=not set yet
    // AstVarScope::user2p  -> ReorderVarPostVertex*.  Delayed assignment var, 0=not set yet
    // Ast*::user3p         -> ReorderLogicVertex*.  Statement (temporary only)
    // Ast*::user4          -> int.  Current ordering number (reorderBlock usage)

    // STATE
    V3Graph* m_graphp = nullptr;  // Scoreboard of var usages/dependencies
    ReorderImpureVertex* m_impureVtxp = nullptr;  // Element specifying PLI ordering
    தருக்க m_inDly = பொய்;  // Inside ASSIGNDLY
    மாறிலி எழுத்து* m_noReorderWhy = nullptr;  // Reason we can't reorder
    std::vector<ReorderLogicVertex*> m_stmtStackps;  // Current statements being tracked

    // METHODS
    வெறுமை cleanupBlockGraph(Ast_முனை* nodep) {
        // Transform the graph into what we need
        UINFO(5, "ReorderBlock " << nodep);

        // Simplify graph by removing redundant edges
        m_graphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        எனில் (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_nodup", பொய்);

        // Mark all the logic for this step by setting Vertex::user() to true
        m_graphp->userClearVertices();
        க்கு (Ast_முனை* nextp = nodep; nextp; nextp = nextp->nextp()) {
            nextp->user3u().to<ReorderLogicVertex*>()->user(உண்மை);
        }

        // New step
        ReorderEdge::incrementStep();

        // If a var vertex has only inputs, it's a input-only node,
        // and can be ignored for coloring **this block only**
        க்கு (V3GraphVertex& vtx : m_graphp->vertices()) {
            எனில் (!vtx.outEmpty()) தொடர்;
            எனில் (!vtx.is<ReorderVarStdVertex>()) தொடர்;
            க்கு (V3GraphEdge& edge : vtx.inEdges()) edge.as<ReorderEdge>()->setIgnoreThisStep();
        }

        // For reordering this single block only, mark all logic
        // vertexes not involved with this step as unimportant
        க்கு (V3GraphVertex& vertex : m_graphp->vertices()) {
            எனில் (vertex.user()) தொடர்;
            எனில் (!vertex.is<ReorderLogicVertex>()) தொடர்;
            க்கு (V3GraphEdge& edge : vertex.inEdges()) edge.as<ReorderEdge>()->setIgnoreThisStep();
            க்கு (V3GraphEdge& edge : vertex.outEdges())
                edge.as<ReorderEdge>()->setIgnoreThisStep();
        }

        // Weak coloring to determine what needs to remain in order
        // This follows all step-relevant edges excluding PostEdges, which are done later
        m_graphp->weaklyConnected(&ReorderEdge::followScoreboard);

        // Add hard orderings between all nodes of same color, in the order they appeared
        std::unordered_map<uint32_t, ReorderLogicVertex*> lastOfColor;
        க்கு (Ast_முனை* currp = nodep; currp; currp = currp->nextp()) {
            ReorderLogicVertex* மாறிலி vtxp = currp->user3u().to<ReorderLogicVertex*>();
            மாறிலி uint32_t color = vtxp->color();
            எனில் (lastOfColor[color]) புதிய ReorderStrictEdge{m_graphp, lastOfColor[color], vtxp};
            lastOfColor[color] = vtxp;
        }

        // And a real ordering to get the statements into something reasonable
        // We don't care if there's cutable violations here...
        // Non-cutable violations should be impossible; as those edges are program-order
        எனில் (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_pre", பொய்);
        m_graphp->acyclic(&ReorderEdge::followCyclic);
        m_graphp->rank(&ReorderEdge::followCyclic);  // Or order(), but that's more expensive
        எனில் (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_opt", பொய்);
    }

    வெறுமை reorderBlock(Ast_முனை* nodep) {
        // Reorder statements in the completed graph

        // Map the rank numbers into nodes they associate with
        std::multimap<uint32_t, Ast_முனை*> rankMap;
        முழு currOrder = 0;  // Existing sequence number of assignment
        க்கு (Ast_முனை* currp = nodep; currp; currp = currp->nextp()) {
            மாறிலி ReorderLogicVertex* மாறிலி vtxp = currp->user3u().to<ReorderLogicVertex*>();
            rankMap.emplace(vtxp->rank(), currp);
            currp->user4(++currOrder);  // Record current ordering
        }

        // Is the current ordering OK?
        தருக்க leaveAlone = உண்மை;
        uint64_t newOrder = 0;  // New sequence number of assignment
        க்கு (மாறிலி auto& item : rankMap) {
            மாறிலி Ast_முனை* மாறிலி nextp = item.second;
            எனில் (++newOrder != nextp->user4()) leaveAlone = பொய்;
        }
        எனில் (leaveAlone) {
            UINFO(6, "   No changes");
            திரும்பு;
        }

        VNRelinker replaceHandle;  // Where to add the list
        Ast_முனை* newListp = nullptr;
        க்கு (மாறிலி auto& item : rankMap) {
            Ast_முனை* மாறிலி nextp = item.second;
            UINFO(6, "   New order: " << nextp);
            nextp->unlinkFrBack(nextp == nodep ? &replaceHandle : nullptr);
            newListp = Ast_முனை::addNext(newListp, nextp);
        }
        replaceHandle.relink(newListp);
    }

    வெறுமை processBlock(Ast_முனை* nodep) {
        எனில் (m_noReorderWhy) திரும்பு;

        // Empty lists are ignorable
        எனில் (!nodep) திரும்பு;
        UASSERT_OBJ(nodep->firstAbovep(), nodep, "Node passed is in not head of list");
        UASSERT_OBJ(!nodep->user3p(), nodep, "Should not have a logic vertex");

        // It nothing to reorder with, just iterate
        எனில் (!nodep->nextp()) {
            iterate(nodep);
            திரும்பு;
        }

        // Process it
        UINFO(9, "  processBlock " << nodep);

        // Iterate across current block, making the scoreboard
        க்கு (Ast_முனை* currp = nodep; currp; currp = currp->nextp()) {
            // Create the logic vertex for this statement
            UASSERT_OBJ(!currp->user3p(), currp, "user3p should not be set");
            ReorderLogicVertex* மாறிலி vtxp = புதிய ReorderLogicVertex{m_graphp, currp};
            currp->user3p(vtxp);

            // Visit the statement - this can recursively reorder sub statements
            m_stmtStackps.தள்ளு_back(vtxp);
            iterate(currp);
            m_stmtStackps.எடு_back();
        }

        எனில் (m_noReorderWhy) {  // Jump or something nasty
            UINFO(9, "  NoReorderBlock because " << m_noReorderWhy);
            திரும்பு;
        }

        // Reorder statements in this block
        cleanupBlockGraph(nodep);
        reorderBlock(nodep);

        // 'nodep' might no longer be the head of the list, rewind
        போது (nodep->backp()->nextp() == nodep) nodep = nodep->backp();

        // Delete vertexes and edges only applying to this block
        க்கு (Ast_முனை* currp = nodep; currp; currp = currp->nextp()) {
            currp->user3u().to<ReorderLogicVertex*>()->unlinkDelete(m_graphp);
            currp->user3p(nullptr);
        }
    }

    // VISITORS
    வெறுமை visit(AstAlways* nodep) override {
        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_inDly);
        VL_RESTORER(m_noReorderWhy);

        V3Graph graph;
        m_graphp = &graph;
        m_impureVtxp = nullptr;
        m_inDly = பொய்;
        m_noReorderWhy = nullptr;

        மாறிலி VNUser1InUse user1InUse;
        மாறிலி VNUser2InUse user2InUse;
        மாறிலி VNUser3InUse user3InUse;
        மாறிலி VNUser4InUse user4InUse;

        UINFOTREE(9, nodep, "", "alwIn:");
        processBlock(nodep->stmtsp());
        UINFOTREE(9, nodep, "", "alwOut");

        m_stmtStackps.clear();
    }

    வெறுமை visit(Ast_முனை_If* nodep) override {
        எனில் (!m_graphp || m_noReorderWhy) திரும்பு;
        iterateAndNextNull(nodep->condp());
        processBlock(nodep->thensp());
        processBlock(nodep->elsesp());
    }

    வெறுமை visit(AstJumpGo* nodep) override {
        எனில் (!m_graphp || m_noReorderWhy) திரும்பு;
        m_noReorderWhy = "JumpGo";
        iterateChildren(nodep);
    }

    வெறுமை visit(Ast_முனை_CCall* nodep) override {
        எனில் (!m_graphp || m_noReorderWhy) திரும்பு;
        // The callee is not inlined, so the variables it reads and writes carry no VarRef at
        // the call site, and the scoreboard would let assignments to them move across the
        // call. Treat any such call as a barrier.
        m_noReorderWhy = "CCall";
        iterateChildren(nodep);
    }

    வெறுமை visit(Ast_கோவை_Stmt* nodep) override {
        எனில் (!m_graphp || m_noReorderWhy) திரும்பு;
        VL_RESTORER(m_inDly);
        m_inDly = பொய்;
        iterateChildren(nodep);
    }

    வெறுமை visit(AstAssignDly* nodep) override {
        எனில் (!m_graphp || m_noReorderWhy) திரும்பு;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        எனில் (Ast_முனை* மாறிலி controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_inDly);
        m_inDly = உண்மை;
        iterate(nodep->lhsp());
    }

    வெறுமை visit(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        எனில் (m_graphp) m_noReorderWhy = "Handle";
        visit(static_cast<Ast_முனை*>(nodep));
    }

    வெறுமை visit(AstVarRef* nodep) override {
        எனில் (!m_graphp || m_noReorderWhy) திரும்பு;
        எனில் (m_stmtStackps.empty()) திரும்பு;

        // Reads of constants can be ignored - TODO: This should be "constexpr", not run-time const
        எனில் (nodep->varp()->isConst()) திரும்பு;

        // SPEEDUP: We add duplicate edges, that should be fixed

        AstVarScope* மாறிலி vscp = nodep->varScopep();

        // Create vertexes for variable
        எனில் (!vscp->user1p()) vscp->user1p(புதிய ReorderVarStdVertex{m_graphp, vscp});
        ReorderVarStdVertex* மாறிலி vstdp = vscp->user1u().to<ReorderVarStdVertex*>();

        // Variable is read
        எனில் (nodep->access().isReadOnly()) {
            க்கு (ReorderLogicVertex* மாறிலி vtxp : m_stmtStackps) {
                புதிய ReorderRVEdge{m_graphp, vtxp, vstdp};
            }
            திரும்பு;
        }

        // Variable is written, not NBA
        எனில் (!m_inDly) {
            க்கு (ReorderLogicVertex* மாறிலி vtxp : m_stmtStackps) {
                புதிய ReorderLVEdge{m_graphp, vstdp, vtxp};
            }
            திரும்பு;
        }

        // Variable is written by NBA
        எனில் (!vscp->user2p()) {
            ReorderVarPostVertex* மாறிலி vpostp = புதிய ReorderVarPostVertex{m_graphp, vscp};
            vscp->user2p(vpostp);
            புதிய ReorderPostEdge{m_graphp, vstdp, vpostp};
        }
        ReorderVarPostVertex* மாறிலி vpostp = vscp->user2u().to<ReorderVarPostVertex*>();
        க்கு (ReorderLogicVertex* மாறிலி vtxp : m_stmtStackps) {
            புதிய ReorderLVEdge{m_graphp, vpostp, vtxp};
        }
    }

    வெறுமை visit(Ast_முனை* nodep) override {
        // Outside AstAlways, just descend
        எனில் (!m_graphp) {
            iterateChildren(nodep);
            திரும்பு;
        }

        // Early exit if decided not to reorder
        எனில் (m_noReorderWhy) திரும்பு;

        // Timing control prevents reordering
        எனில் (nodep->isTimingControl()) {
            m_noReorderWhy = "TimingControl";
            திரும்பு;
        }

        // Order all impure statements with other impure statements
        எனில் (!nodep->isPure()) {
            எனில் (!m_impureVtxp) m_impureVtxp = புதிய ReorderImpureVertex{m_graphp, nodep};
            // This edge is only used to find weakly connected components, so one edge is enough
            க்கு (ReorderLogicVertex* மாறிலி vtxp : m_stmtStackps) {
                புதிய ReorderScorebdEdge{m_graphp, m_impureVtxp, vtxp};
            }
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
பொது:
    explicit ReorderVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReorderVisitor() override = இயல்பு;
};

}  // namespace

//######################################################################
// V3Reorder class functions

வெறுமை V3Reorder::reorderAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReorderVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("reorder", 0, dump_மரம்_Either_Level() >= 3);
}
