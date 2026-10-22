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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Reorder.h"

#अन्तर्भाव "V3EmitV.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <string>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

नामाकाश {

//######################################################################
// Support classes

वर्ग Reorder_गाठ_Vertex VL_NOT_FINAL : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(Reorder_गाठ_Vertex, V3GraphVertex)
    Ast_गाठ* ध्रुव m_nodep;

रक्षित:
    Reorder_गाठ_Vertex(V3Graph* graphp, Ast_गाठ* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~Reorder_गाठ_Vertex() override = स्वभाव;
    // ACCESSORS
    // Do not make accessor for nodep(),  It may change due to
    // reordering a lower block, but we don't repair it
    std::string name() ध्रुव override {
        std::string str = cvtToHex(m_nodep) + '\n';
        यदि (AstVarScope* ध्रुव vscp = VN_CAST(m_nodep, VarScope)) {
            str += vscp->prettyName();
        } अन्यथा {
            str += V3EmitV::debug_Verilog_For_वृक्ष(m_nodep);
            str = VString::quoteBackslash(str);
            str = VString::quoteAny(str, '"', '\\');
            str = VString::replaceSubstr(str, "\n", "\\l");
        }
        प्रत्यागम str;
    }
    File_ओळ* fileline() ध्रुव override { प्रत्यागम nodep()->fileline(); }
    std::string dotShape() ध्रुव override { प्रत्यागम VN_IS(m_nodep, VarScope) ? "ellipse" : "box"; }

सार्वजनिक:
    आभास Ast_गाठ* nodep() ध्रुव { प्रत्यागम m_nodep; }
};

वर्ग ReorderImpureVertex final : सार्वजनिक Reorder_गाठ_Vertex {
    VL_RTTI_IMPL(ReorderImpureVertex, Reorder_गाठ_Vertex)
सार्वजनिक:
    explicit ReorderImpureVertex(V3Graph* graphp, Ast_गाठ* nodep)
        : Reorder_गाठ_Vertex{graphp, nodep} {}
    ~ReorderImpureVertex() override = स्वभाव;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम "*IMPURE*"; }
    string dotColor() ध्रुव override { प्रत्यागम "red"; }
};

वर्ग ReorderLogicVertex final : सार्वजनिक Reorder_गाठ_Vertex {
    VL_RTTI_IMPL(ReorderLogicVertex, Reorder_गाठ_Vertex)
सार्वजनिक:
    ReorderLogicVertex(V3Graph* graphp, Ast_गाठ* nodep)
        : Reorder_गाठ_Vertex{graphp, nodep} {}
    ~ReorderLogicVertex() override = स्वभाव;
    string dotColor() ध्रुव override { प्रत्यागम "black"; }
};

वर्ग ReorderVarStdVertex final : सार्वजनिक Reorder_गाठ_Vertex {
    VL_RTTI_IMPL(ReorderVarStdVertex, Reorder_गाठ_Vertex)
सार्वजनिक:
    ReorderVarStdVertex(V3Graph* graphp, AstVarScope* nodep)
        : Reorder_गाठ_Vertex{graphp, nodep} {}
    ~ReorderVarStdVertex() override = स्वभाव;
    string dotColor() ध्रुव override { प्रत्यागम "blue"; }
};

वर्ग ReorderVarPostVertex final : सार्वजनिक Reorder_गाठ_Vertex {
    VL_RTTI_IMPL(ReorderVarPostVertex, Reorder_गाठ_Vertex)
सार्वजनिक:
    ReorderVarPostVertex(V3Graph* graphp, AstVarScope* nodep)
        : Reorder_गाठ_Vertex{graphp, nodep} {}
    ~ReorderVarPostVertex() override = स्वभाव;
    string name() ध्रुव override { प्रत्यागम "POST "s + Reorder_गाठ_Vertex::name(); }
    string dotColor() ध्रुव override { प्रत्यागम "green"; }
};

//######################################################################
// Edge types

वर्ग ReorderEdge VL_NOT_FINAL : सार्वजनिक V3GraphEdge {
    VL_RTTI_IMPL(ReorderEdge, V3GraphEdge)
    uint32_t m_ignoreInStep = 0;  // Step number that if set to, causes this edge to be ignored
    स्थिर uint32_t s_step_अंक;  // Global step number
रक्षित:
    स्थिर constexpr पूर्णाङ्क WEIGHT_NORMAL = 10;
    ReorderEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष, तार्किक cutable)
        : V3GraphEdge{graphp, fromp, शीर्ष, WEIGHT_NORMAL, cutable} {}
    ~ReorderEdge() override = स्वभाव;

    आभास तार्किक followScoreboard() ध्रुव = 0;

    std::string dotStyle() ध्रुव override {
        प्रत्यागम ignoreThisStep() ? "dotted" : V3GraphEdge::dotStyle();
    }

सार्वजनिक:
    // Iterator for graph functions
    स्थिर शून्य incrementStep() { ++s_step_अंक; }
    तार्किक ignoreThisStep() ध्रुव { प्रत्यागम m_ignoreInStep == s_step_अंक; }
    शून्य setIgnoreThisStep() { m_ignoreInStep = s_step_अंक; }

    स्थिर तार्किक followScoreboard(ध्रुव V3GraphEdge* edgep) {
        ध्रुव ReorderEdge& edge = *edgep->as<ReorderEdge>();
        प्रत्यागम !edge.ignoreThisStep() && edge.followScoreboard();
    }
    स्थिर तार्किक followCyclic(ध्रुव V3GraphEdge* edgep) {
        ध्रुव ReorderEdge& edge = *edgep->as<ReorderEdge>();
        प्रत्यागम !edge.ignoreThisStep();
    }
};
uint32_t ReorderEdge::s_step_अंक = 0;

वर्ग ReorderPostEdge final : सार्वजनिक ReorderEdge {
    VL_RTTI_IMPL(ReorderPostEdge, ReorderEdge)
सार्वजनिक:
    ReorderPostEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष)
        : ReorderEdge{graphp, fromp, शीर्ष, CUTABLE} {}
    ~ReorderPostEdge() override = स्वभाव;
    तार्किक followScoreboard() ध्रुव override { प्रत्यागम असत्य; }
    string dotColor() ध्रुव override { प्रत्यागम "khaki"; }
};

वर्ग ReorderLVEdge final : सार्वजनिक ReorderEdge {
    VL_RTTI_IMPL(ReorderLVEdge, ReorderEdge)
सार्वजनिक:
    ReorderLVEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष)
        : ReorderEdge{graphp, fromp, शीर्ष, CUTABLE} {}
    ~ReorderLVEdge() override = स्वभाव;
    तार्किक followScoreboard() ध्रुव override { प्रत्यागम सत्य; }
    string dotColor() ध्रुव override { प्रत्यागम "yellowGreen"; }
};

वर्ग ReorderRVEdge final : सार्वजनिक ReorderEdge {
    VL_RTTI_IMPL(ReorderRVEdge, ReorderEdge)
सार्वजनिक:
    ReorderRVEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष)
        : ReorderEdge{graphp, fromp, शीर्ष, CUTABLE} {}
    ~ReorderRVEdge() override = स्वभाव;
    तार्किक followScoreboard() ध्रुव override { प्रत्यागम सत्य; }
    string dotColor() ध्रुव override { प्रत्यागम "green"; }
};

वर्ग ReorderScorebdEdge final : सार्वजनिक ReorderEdge {
    VL_RTTI_IMPL(ReorderScorebdEdge, ReorderEdge)
सार्वजनिक:
    ReorderScorebdEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष)
        : ReorderEdge{graphp, fromp, शीर्ष, CUTABLE} {}
    ~ReorderScorebdEdge() override = स्वभाव;
    तार्किक followScoreboard() ध्रुव override { प्रत्यागम सत्य; }
    string dotColor() ध्रुव override { प्रत्यागम "blue"; }
};

वर्ग ReorderStrictEdge final : सार्वजनिक ReorderEdge {
    VL_RTTI_IMPL(ReorderStrictEdge, ReorderEdge)
    // A strict order, based on the original statement order in the graph
    // The only non-cutable edge type
सार्वजनिक:
    ReorderStrictEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष)
        : ReorderEdge{graphp, fromp, शीर्ष, NOT_CUTABLE} {}
    ~ReorderStrictEdge() override = स्वभाव;
    तार्किक followScoreboard() ध्रुव override { प्रत्यागम सत्य; }
    string dotColor() ध्रुव override { प्रत्यागम "blue"; }
};

//######################################################################
// Reorder class functions

वर्ग ReorderVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE - Only under AstAlways
    // AstVarScope::user1p  -> ReorderVarStdVertex*.  Usage var, 0=not set yet
    // AstVarScope::user2p  -> ReorderVarPostVertex*.  Delayed assignment var, 0=not set yet
    // Ast*::user3p         -> ReorderLogicVertex*.  Statement (temporary only)
    // Ast*::user4          -> int.  Current ordering number (reorderBlock usage)

    // STATE
    V3Graph* m_graphp = nullptr;  // Scoreboard of var usages/dependencies
    ReorderImpureVertex* m_impureVtxp = nullptr;  // Element specifying PLI ordering
    तार्किक m_inDly = असत्य;  // Inside ASSIGNDLY
    ध्रुव वर्ण* m_noReorderWhy = nullptr;  // Reason we can't reorder
    std::vector<ReorderLogicVertex*> m_stmtStackps;  // Current statements being tracked

    // METHODS
    शून्य cleanupBlockGraph(Ast_गाठ* nodep) {
        // Transform the graph into what we need
        UINFO(5, "ReorderBlock " << nodep);

        // Simplify graph by removing redundant edges
        m_graphp->removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
        यदि (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_nodup", असत्य);

        // Mark all the logic for this step by setting Vertex::user() to true
        m_graphp->userClearVertices();
        प्रति (Ast_गाठ* nextp = nodep; nextp; nextp = nextp->nextp()) {
            nextp->user3u().to<ReorderLogicVertex*>()->user(सत्य);
        }

        // New step
        ReorderEdge::incrementStep();

        // If a var vertex has only inputs, it's a input-only node,
        // and can be ignored for coloring **this block only**
        प्रति (V3GraphVertex& vtx : m_graphp->vertices()) {
            यदि (!vtx.outEmpty()) अनुवर्त;
            यदि (!vtx.is<ReorderVarStdVertex>()) अनुवर्त;
            प्रति (V3GraphEdge& edge : vtx.inEdges()) edge.as<ReorderEdge>()->setIgnoreThisStep();
        }

        // For reordering this single block only, mark all logic
        // vertexes not involved with this step as unimportant
        प्रति (V3GraphVertex& vertex : m_graphp->vertices()) {
            यदि (vertex.user()) अनुवर्त;
            यदि (!vertex.is<ReorderLogicVertex>()) अनुवर्त;
            प्रति (V3GraphEdge& edge : vertex.inEdges()) edge.as<ReorderEdge>()->setIgnoreThisStep();
            प्रति (V3GraphEdge& edge : vertex.outEdges())
                edge.as<ReorderEdge>()->setIgnoreThisStep();
        }

        // Weak coloring to determine what needs to remain in order
        // This follows all step-relevant edges excluding PostEdges, which are done later
        m_graphp->weaklyConnected(&ReorderEdge::followScoreboard);

        // Add hard orderings between all nodes of same color, in the order they appeared
        std::unordered_map<uint32_t, ReorderLogicVertex*> lastOfColor;
        प्रति (Ast_गाठ* currp = nodep; currp; currp = currp->nextp()) {
            ReorderLogicVertex* ध्रुव vtxp = currp->user3u().to<ReorderLogicVertex*>();
            ध्रुव uint32_t color = vtxp->color();
            यदि (lastOfColor[color]) नूतन ReorderStrictEdge{m_graphp, lastOfColor[color], vtxp};
            lastOfColor[color] = vtxp;
        }

        // And a real ordering to get the statements into something reasonable
        // We don't care if there's cutable violations here...
        // Non-cutable violations should be impossible; as those edges are program-order
        यदि (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_pre", असत्य);
        m_graphp->acyclic(&ReorderEdge::followCyclic);
        m_graphp->rank(&ReorderEdge::followCyclic);  // Or order(), but that's more expensive
        यदि (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_opt", असत्य);
    }

    शून्य reorderBlock(Ast_गाठ* nodep) {
        // Reorder statements in the completed graph

        // Map the rank numbers into nodes they associate with
        std::multimap<uint32_t, Ast_गाठ*> rankMap;
        पूर्णाङ्क currOrder = 0;  // Existing sequence number of assignment
        प्रति (Ast_गाठ* currp = nodep; currp; currp = currp->nextp()) {
            ध्रुव ReorderLogicVertex* ध्रुव vtxp = currp->user3u().to<ReorderLogicVertex*>();
            rankMap.emplace(vtxp->rank(), currp);
            currp->user4(++currOrder);  // Record current ordering
        }

        // Is the current ordering OK?
        तार्किक leaveAlone = सत्य;
        uint64_t newOrder = 0;  // New sequence number of assignment
        प्रति (ध्रुव auto& item : rankMap) {
            ध्रुव Ast_गाठ* ध्रुव nextp = item.second;
            यदि (++newOrder != nextp->user4()) leaveAlone = असत्य;
        }
        यदि (leaveAlone) {
            UINFO(6, "   No changes");
            प्रत्यागम;
        }

        VNRelinker replaceHandle;  // Where to add the list
        Ast_गाठ* newListp = nullptr;
        प्रति (ध्रुव auto& item : rankMap) {
            Ast_गाठ* ध्रुव nextp = item.second;
            UINFO(6, "   New order: " << nextp);
            nextp->unlinkFrBack(nextp == nodep ? &replaceHandle : nullptr);
            newListp = Ast_गाठ::addNext(newListp, nextp);
        }
        replaceHandle.relink(newListp);
    }

    शून्य processBlock(Ast_गाठ* nodep) {
        यदि (m_noReorderWhy) प्रत्यागम;

        // Empty lists are ignorable
        यदि (!nodep) प्रत्यागम;
        UASSERT_OBJ(nodep->firstAbovep(), nodep, "Node passed is in not head of list");
        UASSERT_OBJ(!nodep->user3p(), nodep, "Should not have a logic vertex");

        // It nothing to reorder with, just iterate
        यदि (!nodep->nextp()) {
            iterate(nodep);
            प्रत्यागम;
        }

        // Process it
        UINFO(9, "  processBlock " << nodep);

        // Iterate across current block, making the scoreboard
        प्रति (Ast_गाठ* currp = nodep; currp; currp = currp->nextp()) {
            // Create the logic vertex for this statement
            UASSERT_OBJ(!currp->user3p(), currp, "user3p should not be set");
            ReorderLogicVertex* ध्रुव vtxp = नूतन ReorderLogicVertex{m_graphp, currp};
            currp->user3p(vtxp);

            // Visit the statement - this can recursively reorder sub statements
            m_stmtStackps.ढकला_back(vtxp);
            iterate(currp);
            m_stmtStackps.काढा_back();
        }

        यदि (m_noReorderWhy) {  // Jump or something nasty
            UINFO(9, "  NoReorderBlock because " << m_noReorderWhy);
            प्रत्यागम;
        }

        // Reorder statements in this block
        cleanupBlockGraph(nodep);
        reorderBlock(nodep);

        // 'nodep' might no longer be the head of the list, rewind
        यावत् (nodep->backp()->nextp() == nodep) nodep = nodep->backp();

        // Delete vertexes and edges only applying to this block
        प्रति (Ast_गाठ* currp = nodep; currp; currp = currp->nextp()) {
            currp->user3u().to<ReorderLogicVertex*>()->unlinkDelete(m_graphp);
            currp->user3p(nullptr);
        }
    }

    // VISITORS
    शून्य visit(AstAlways* nodep) override {
        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_inDly);
        VL_RESTORER(m_noReorderWhy);

        V3Graph graph;
        m_graphp = &graph;
        m_impureVtxp = nullptr;
        m_inDly = असत्य;
        m_noReorderWhy = nullptr;

        ध्रुव VNUser1InUse user1InUse;
        ध्रुव VNUser2InUse user2InUse;
        ध्रुव VNUser3InUse user3InUse;
        ध्रुव VNUser4InUse user4InUse;

        UINFOTREE(9, nodep, "", "alwIn:");
        processBlock(nodep->stmtsp());
        UINFOTREE(9, nodep, "", "alwOut");

        m_stmtStackps.clear();
    }

    शून्य visit(Ast_गाठ_If* nodep) override {
        यदि (!m_graphp || m_noReorderWhy) प्रत्यागम;
        iterateAndNextNull(nodep->condp());
        processBlock(nodep->thensp());
        processBlock(nodep->elsesp());
    }

    शून्य visit(AstJumpGo* nodep) override {
        यदि (!m_graphp || m_noReorderWhy) प्रत्यागम;
        m_noReorderWhy = "JumpGo";
        iterateChildren(nodep);
    }

    शून्य visit(Ast_गाठ_CCall* nodep) override {
        यदि (!m_graphp || m_noReorderWhy) प्रत्यागम;
        // The callee is not inlined, so the variables it reads and writes carry no VarRef at
        // the call site, and the scoreboard would let assignments to them move across the
        // call. Treat any such call as a barrier.
        m_noReorderWhy = "CCall";
        iterateChildren(nodep);
    }

    शून्य visit(Ast_अभिव्यक्ती_Stmt* nodep) override {
        यदि (!m_graphp || m_noReorderWhy) प्रत्यागम;
        VL_RESTORER(m_inDly);
        m_inDly = असत्य;
        iterateChildren(nodep);
    }

    शून्य visit(AstAssignDly* nodep) override {
        यदि (!m_graphp || m_noReorderWhy) प्रत्यागम;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        यदि (Ast_गाठ* ध्रुव controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_inDly);
        m_inDly = सत्य;
        iterate(nodep->lhsp());
    }

    शून्य visit(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        यदि (m_graphp) m_noReorderWhy = "Handle";
        visit(static_cast<Ast_गाठ*>(nodep));
    }

    शून्य visit(AstVarRef* nodep) override {
        यदि (!m_graphp || m_noReorderWhy) प्रत्यागम;
        यदि (m_stmtStackps.empty()) प्रत्यागम;

        // Reads of constants can be ignored - TODO: This should be "constexpr", not run-time const
        यदि (nodep->varp()->isConst()) प्रत्यागम;

        // SPEEDUP: We add duplicate edges, that should be fixed

        AstVarScope* ध्रुव vscp = nodep->varScopep();

        // Create vertexes for variable
        यदि (!vscp->user1p()) vscp->user1p(नूतन ReorderVarStdVertex{m_graphp, vscp});
        ReorderVarStdVertex* ध्रुव vstdp = vscp->user1u().to<ReorderVarStdVertex*>();

        // Variable is read
        यदि (nodep->access().isReadOnly()) {
            प्रति (ReorderLogicVertex* ध्रुव vtxp : m_stmtStackps) {
                नूतन ReorderRVEdge{m_graphp, vtxp, vstdp};
            }
            प्रत्यागम;
        }

        // Variable is written, not NBA
        यदि (!m_inDly) {
            प्रति (ReorderLogicVertex* ध्रुव vtxp : m_stmtStackps) {
                नूतन ReorderLVEdge{m_graphp, vstdp, vtxp};
            }
            प्रत्यागम;
        }

        // Variable is written by NBA
        यदि (!vscp->user2p()) {
            ReorderVarPostVertex* ध्रुव vpostp = नूतन ReorderVarPostVertex{m_graphp, vscp};
            vscp->user2p(vpostp);
            नूतन ReorderPostEdge{m_graphp, vstdp, vpostp};
        }
        ReorderVarPostVertex* ध्रुव vpostp = vscp->user2u().to<ReorderVarPostVertex*>();
        प्रति (ReorderLogicVertex* ध्रुव vtxp : m_stmtStackps) {
            नूतन ReorderLVEdge{m_graphp, vpostp, vtxp};
        }
    }

    शून्य visit(Ast_गाठ* nodep) override {
        // Outside AstAlways, just descend
        यदि (!m_graphp) {
            iterateChildren(nodep);
            प्रत्यागम;
        }

        // Early exit if decided not to reorder
        यदि (m_noReorderWhy) प्रत्यागम;

        // Timing control prevents reordering
        यदि (nodep->isTimingControl()) {
            m_noReorderWhy = "TimingControl";
            प्रत्यागम;
        }

        // Order all impure statements with other impure statements
        यदि (!nodep->isPure()) {
            यदि (!m_impureVtxp) m_impureVtxp = नूतन ReorderImpureVertex{m_graphp, nodep};
            // This edge is only used to find weakly connected components, so one edge is enough
            प्रति (ReorderLogicVertex* ध्रुव vtxp : m_stmtStackps) {
                नूतन ReorderScorebdEdge{m_graphp, m_impureVtxp, vtxp};
            }
        }

        iterateChildren(nodep);
    }

    // CONSTRUCTORS
सार्वजनिक:
    explicit ReorderVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReorderVisitor() override = स्वभाव;
};

}  // namespace

//######################################################################
// V3Reorder class functions

शून्य V3Reorder::reorderAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReorderVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("reorder", 0, dump_वृक्ष_Either_Level() >= 3);
}
