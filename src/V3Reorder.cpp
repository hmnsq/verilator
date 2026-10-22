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

वर्ग Reorder_ग्रन्थिः_Vertex VL_NOT_FINAL : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(Reorder_ग्रन्थिः_Vertex, V3GraphVertex)
    Ast_ग्रन्थिः* ध्रुव m_nodep;

रक्षित:
    Reorder_ग्रन्थिः_Vertex(V3Graph* graphp, Ast_ग्रन्थिः* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~Reorder_ग्रन्थिः_Vertex() override = स्वभाव;
    // ACCESSORS
    // Do not make accessor for nodep(),  It may change due to
    // reordering a lower block, but we don't repair it
    std::string name() ध्रुव override {
        std::string str = cvt_गन्तव्यम्_Hex(m_nodep) + '\n';
        यदि (AstVarScope* ध्रुव vscp = VN_CAST(m_nodep, VarScope)) {
            str += vscp->prettyName();
        } अन्यथा {
            str += V3EmitV::debug_Verilog_For_वृक्षः(m_nodep);
            str = VString::quoteBackslash(str);
            str = VString::quoteAny(str, '"', '\\');
            str = VString::replaceSubstr(str, "\n", "\\l");
        }
        प्रत्यागम str;
    }
    File_पङ्क्तिः* fileline() ध्रुव override { प्रत्यागम nodep()->fileline(); }
    std::string dotShape() ध्रुव override { प्रत्यागम VN_IS(m_nodep, VarScope) ? "ellipse" : "box"; }

सार्वजनिक:
    आभास Ast_ग्रन्थिः* nodep() ध्रुव { प्रत्यागम m_nodep; }
};

वर्ग ReorderImpureVertex final : सार्वजनिक Reorder_ग्रन्थिः_Vertex {
    VL_RTTI_IMPL(ReorderImpureVertex, Reorder_ग्रन्थिः_Vertex)
सार्वजनिक:
    स्पष्ट ReorderImpureVertex(V3Graph* graphp, Ast_ग्रन्थिः* nodep)
        : Reorder_ग्रन्थिः_Vertex{graphp, nodep} {}
    ~ReorderImpureVertex() override = स्वभाव;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम "*IMPURE*"; }
    string dotColor() ध्रुव override { प्रत्यागम "red"; }
};

वर्ग ReorderLogicVertex final : सार्वजनिक Reorder_ग्रन्थिः_Vertex {
    VL_RTTI_IMPL(ReorderLogicVertex, Reorder_ग्रन्थिः_Vertex)
सार्वजनिक:
    ReorderLogicVertex(V3Graph* graphp, Ast_ग्रन्थिः* nodep)
        : Reorder_ग्रन्थिः_Vertex{graphp, nodep} {}
    ~ReorderLogicVertex() override = स्वभाव;
    string dotColor() ध्रुव override { प्रत्यागम "black"; }
};

वर्ग ReorderVarStdVertex final : सार्वजनिक Reorder_ग्रन्थिः_Vertex {
    VL_RTTI_IMPL(ReorderVarStdVertex, Reorder_ग्रन्थिः_Vertex)
सार्वजनिक:
    ReorderVarStdVertex(V3Graph* graphp, AstVarScope* nodep)
        : Reorder_ग्रन्थिः_Vertex{graphp, nodep} {}
    ~ReorderVarStdVertex() override = स्वभाव;
    string dotColor() ध्रुव override { प्रत्यागम "blue"; }
};

वर्ग ReorderVarPostVertex final : सार्वजनिक Reorder_ग्रन्थिः_Vertex {
    VL_RTTI_IMPL(ReorderVarPostVertex, Reorder_ग्रन्थिः_Vertex)
सार्वजनिक:
    ReorderVarPostVertex(V3Graph* graphp, AstVarScope* nodep)
        : Reorder_ग्रन्थिः_Vertex{graphp, nodep} {}
    ~ReorderVarPostVertex() override = स्वभाव;
    string name() ध्रुव override { प्रत्यागम "POST "s + Reorder_ग्रन्थिः_Vertex::name(); }
    string dotColor() ध्रुव override { प्रत्यागम "green"; }
};

//######################################################################
// Edge types

वर्ग ReorderEdge VL_NOT_FINAL : सार्वजनिक V3GraphEdge {
    VL_RTTI_IMPL(ReorderEdge, V3GraphEdge)
    uint32_t m_ignore_अन्तर्_चरणः = 0;  // Step number that if set to, causes this edge to be ignored
    स्थिर uint32_t s_चरणः_अङ्कः;  // Global step number
रक्षित:
    स्थिर constexpr पूर्णाङ्क WEIGHT_प्रसामान्यम् = 10;
    ReorderEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शिखरम्, तार्किक cutable)
        : V3GraphEdge{graphp, fromp, शिखरम्, WEIGHT_प्रसामान्यम्, cutable} {}
    ~ReorderEdge() override = स्वभाव;

    आभास तार्किक followScoreboard() ध्रुव = 0;

    std::string dotStyle() ध्रुव override {
        प्रत्यागम ignore_This_चरणः() ? "dotted" : V3GraphEdge::dotStyle();
    }

सार्वजनिक:
    // Iterator for graph functions
    स्थिर शून्य increment_चरणः() { ++s_चरणः_अङ्कः; }
    तार्किक ignore_This_चरणः() ध्रुव { प्रत्यागम m_ignore_अन्तर्_चरणः == s_चरणः_अङ्कः; }
    शून्य समुच्चयः_Ignore_This_चरणः() { m_ignore_अन्तर्_चरणः = s_चरणः_अङ्कः; }

    स्थिर तार्किक followScoreboard(ध्रुव V3GraphEdge* edgep) {
        ध्रुव ReorderEdge& edge = *edgep->as<ReorderEdge>();
        प्रत्यागम !edge.ignore_This_चरणः() && edge.followScoreboard();
    }
    स्थिर तार्किक followCyclic(ध्रुव V3GraphEdge* edgep) {
        ध्रुव ReorderEdge& edge = *edgep->as<ReorderEdge>();
        प्रत्यागम !edge.ignore_This_चरणः();
    }
};
uint32_t ReorderEdge::s_चरणः_अङ्कः = 0;

वर्ग ReorderPostEdge final : सार्वजनिक ReorderEdge {
    VL_RTTI_IMPL(ReorderPostEdge, ReorderEdge)
सार्वजनिक:
    ReorderPostEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शिखरम्)
        : ReorderEdge{graphp, fromp, शिखरम्, CUTABLE} {}
    ~ReorderPostEdge() override = स्वभाव;
    तार्किक followScoreboard() ध्रुव override { प्रत्यागम असत्य; }
    string dotColor() ध्रुव override { प्रत्यागम "khaki"; }
};

वर्ग ReorderLVEdge final : सार्वजनिक ReorderEdge {
    VL_RTTI_IMPL(ReorderLVEdge, ReorderEdge)
सार्वजनिक:
    ReorderLVEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शिखरम्)
        : ReorderEdge{graphp, fromp, शिखरम्, CUTABLE} {}
    ~ReorderLVEdge() override = स्वभाव;
    तार्किक followScoreboard() ध्रुव override { प्रत्यागम सत्य; }
    string dotColor() ध्रुव override { प्रत्यागम "yellowGreen"; }
};

वर्ग ReorderRVEdge final : सार्वजनिक ReorderEdge {
    VL_RTTI_IMPL(ReorderRVEdge, ReorderEdge)
सार्वजनिक:
    ReorderRVEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शिखरम्)
        : ReorderEdge{graphp, fromp, शिखरम्, CUTABLE} {}
    ~ReorderRVEdge() override = स्वभाव;
    तार्किक followScoreboard() ध्रुव override { प्रत्यागम सत्य; }
    string dotColor() ध्रुव override { प्रत्यागम "green"; }
};

वर्ग ReorderScorebdEdge final : सार्वजनिक ReorderEdge {
    VL_RTTI_IMPL(ReorderScorebdEdge, ReorderEdge)
सार्वजनिक:
    ReorderScorebdEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शिखरम्)
        : ReorderEdge{graphp, fromp, शिखरम्, CUTABLE} {}
    ~ReorderScorebdEdge() override = स्वभाव;
    तार्किक followScoreboard() ध्रुव override { प्रत्यागम सत्य; }
    string dotColor() ध्रुव override { प्रत्यागम "blue"; }
};

वर्ग ReorderStrictEdge final : सार्वजनिक ReorderEdge {
    VL_RTTI_IMPL(ReorderStrictEdge, ReorderEdge)
    // A strict order, based on the original statement order in the graph
    // The only non-cutable edge type
सार्वजनिक:
    ReorderStrictEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शिखरम्)
        : ReorderEdge{graphp, fromp, शिखरम्, NOT_CUTABLE} {}
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
    V3Graph* m_graphp = शून्यसूचकः;  // Scoreboard of var usages/dependencies
    ReorderImpureVertex* m_impureVtxp = शून्यसूचकः;  // Element specifying PLI ordering
    तार्किक m_अन्तर्_Dly = असत्य;  // Inside ASSIGNDLY
    ध्रुव वर्ण* m_noReorderWhy = शून्यसूचकः;  // Reason we can't reorder
    std::सदिशम्<ReorderLogicVertex*> m_stmtStackps;  // Current statements being tracked

    // METHODS
    शून्य cleanup_खण्डम्_Graph(Ast_ग्रन्थिः* nodep) {
        // Transform the graph into what we need
        UINFO(5, "ReorderBlock " << nodep);

        // Simplify graph by removing redundant edges
        m_graphp->remove_Redundant_धाराः_Max(&V3GraphEdge::followAlwaysTrue);
        यदि (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_nodup", असत्य);

        // Mark all the logic for this step by setting Vertex::user() to true
        m_graphp->userClearVertices();
        प्रति (Ast_ग्रन्थिः* nextp = nodep; nextp; nextp = nextp->nextp()) {
            nextp->user3u().गन्तव्यम्<ReorderLogicVertex*>()->user(सत्य);
        }

        // New step
        ReorderEdge::increment_चरणः();

        // If a var vertex has only inputs, it's a input-only node,
        // and can be ignored for coloring **this block only**
        प्रति (V3GraphVertex& vtx : m_graphp->vertices()) {
            यदि (!vtx.बहिर्गमनम्_रिक्तानि()) अनुवर्त;
            यदि (!vtx.is<ReorderVarStdVertex>()) अनुवर्त;
            प्रति (V3GraphEdge& edge : vtx.अन्तर्_धाराः()) edge.as<ReorderEdge>()->समुच्चयः_Ignore_This_चरणः();
        }

        // For reordering this single block only, mark all logic
        // vertexes not involved with this step as unimportant
        प्रति (V3GraphVertex& vertex : m_graphp->vertices()) {
            यदि (vertex.user()) अनुवर्त;
            यदि (!vertex.is<ReorderLogicVertex>()) अनुवर्त;
            प्रति (V3GraphEdge& edge : vertex.अन्तर्_धाराः()) edge.as<ReorderEdge>()->समुच्चयः_Ignore_This_चरणः();
            प्रति (V3GraphEdge& edge : vertex.बहिर्गमनम्_धाराः())
                edge.as<ReorderEdge>()->समुच्चयः_Ignore_This_चरणः();
        }

        // Weak coloring to determine what needs to remain in order
        // This follows all step-relevant edges excluding PostEdges, which are done later
        m_graphp->weaklyConnected(&ReorderEdge::followScoreboard);

        // Add hard orderings between all nodes of same color, in the order they appeared
        std::unordered_map<uint32_t, ReorderLogicVertex*> चरमः_सम्बन्धि_Color;
        प्रति (Ast_ग्रन्थिः* currp = nodep; currp; currp = currp->nextp()) {
            ReorderLogicVertex* ध्रुव vtxp = currp->user3u().गन्तव्यम्<ReorderLogicVertex*>();
            ध्रुव uint32_t color = vtxp->color();
            यदि (चरमः_सम्बन्धि_Color[color]) नूतन ReorderStrictEdge{m_graphp, चरमः_सम्बन्धि_Color[color], vtxp};
            चरमः_सम्बन्धि_Color[color] = vtxp;
        }

        // And a real ordering to get the statements into something reasonable
        // We don't care if there's cutable violations here...
        // Non-cutable violations should be impossible; as those edges are program-order
        यदि (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_pre", असत्य);
        m_graphp->acyclic(&ReorderEdge::followCyclic);
        m_graphp->rank(&ReorderEdge::followCyclic);  // Or order(), but that's more expensive
        यदि (dumpGraphLevel() >= 9) m_graphp->dumpDotFilePrefixed("reorderg_opt", असत्य);
    }

    शून्य reorder_खण्डम्(Ast_ग्रन्थिः* nodep) {
        // Reorder statements in the completed graph

        // Map the rank numbers into nodes they associate with
        std::multimap<uint32_t, Ast_ग्रन्थिः*> rankMap;
        पूर्णाङ्क curr_क्रमः = 0;  // Existing sequence number of assignment
        प्रति (Ast_ग्रन्थिः* currp = nodep; currp; currp = currp->nextp()) {
            ध्रुव ReorderLogicVertex* ध्रुव vtxp = currp->user3u().गन्तव्यम्<ReorderLogicVertex*>();
            rankMap.emplace(vtxp->rank(), currp);
            currp->user4(++curr_क्रमः);  // Record current ordering
        }

        // Is the current ordering OK?
        तार्किक leaveAlone = सत्य;
        uint64_t new_क्रमः = 0;  // New sequence number of assignment
        प्रति (ध्रुव स्वचालित& item : rankMap) {
            ध्रुव Ast_ग्रन्थिः* ध्रुव nextp = item.second;
            यदि (++new_क्रमः != nextp->user4()) leaveAlone = असत्य;
        }
        यदि (leaveAlone) {
            UINFO(6, "   No changes");
            प्रत्यागम;
        }

        VNRelinker replaceHandle;  // Where to add the list
        Ast_ग्रन्थिः* newListp = शून्यसूचकः;
        प्रति (ध्रुव स्वचालित& item : rankMap) {
            Ast_ग्रन्थिः* ध्रुव nextp = item.second;
            UINFO(6, "   New order: " << nextp);
            nextp->unlink_Fr_पृष्ठभागः(nextp == nodep ? &replaceHandle : शून्यसूचकः);
            newListp = Ast_ग्रन्थिः::सङ्कलय_अग्र(newListp, nextp);
        }
        replaceHandle.relink(newListp);
    }

    शून्य process_खण्डम्(Ast_ग्रन्थिः* nodep) {
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
        प्रति (Ast_ग्रन्थिः* currp = nodep; currp; currp = currp->nextp()) {
            // Create the logic vertex for this statement
            UASSERT_OBJ(!currp->user3p(), currp, "user3p should not be set");
            ReorderLogicVertex* ध्रुव vtxp = नूतन ReorderLogicVertex{m_graphp, currp};
            currp->user3p(vtxp);

            // Visit the statement - this can recursively reorder sub statements
            m_stmtStackps.प्रक्षिप_पृष्ठभागः(vtxp);
            iterate(currp);
            m_stmtStackps.निष्कासय_पृष्ठभागः();
        }

        यदि (m_noReorderWhy) {  // Jump or something nasty
            UINFO(9, "  NoReorderBlock because " << m_noReorderWhy);
            प्रत्यागम;
        }

        // Reorder statements in this block
        cleanup_खण्डम्_Graph(nodep);
        reorder_खण्डम्(nodep);

        // 'nodep' might no longer be the head of the list, rewind
        यावत् (nodep->backp()->nextp() == nodep) nodep = nodep->backp();

        // Delete vertexes and edges only applying to this block
        प्रति (Ast_ग्रन्थिः* currp = nodep; currp; currp = currp->nextp()) {
            currp->user3u().गन्तव्यम्<ReorderLogicVertex*>()->unlinkDelete(m_graphp);
            currp->user3p(शून्यसूचकः);
        }
    }

    // VISITORS
    शून्य दर्शय(AstAlways* nodep) override {
        UASSERT_OBJ(!m_graphp, nodep, "AstAlways should not nest");
        VL_RESTORER(m_graphp);
        VL_RESTORER(m_impureVtxp);
        VL_RESTORER(m_अन्तर्_Dly);
        VL_RESTORER(m_noReorderWhy);

        V3Graph graph;
        m_graphp = &graph;
        m_impureVtxp = शून्यसूचकः;
        m_अन्तर्_Dly = असत्य;
        m_noReorderWhy = शून्यसूचकः;

        ध्रुव VNUser1_अन्तर्_Use user1_अन्तर्_Use;
        ध्रुव VNUser2_अन्तर्_Use user2_अन्तर्_Use;
        ध्रुव VNUser3_अन्तर्_Use user3_अन्तर्_Use;
        ध्रुव VNUser4_अन्तर्_Use user4_अन्तर्_Use;

        UINFOTREE(9, nodep, "", "alwIn:");
        process_खण्डम्(nodep->stmtsp());
        UINFOTREE(9, nodep, "", "alwOut");

        m_stmtStackps.clear();
    }

    शून्य दर्शय(Ast_ग्रन्थिः_If* nodep) override {
        यदि (!m_graphp || m_noReorderWhy) प्रत्यागम;
        iterate_And_अग्र_Null(nodep->condp());
        process_खण्डम्(nodep->thensp());
        process_खण्डम्(nodep->elsesp());
    }

    शून्य दर्शय(AstJumpGo* nodep) override {
        यदि (!m_graphp || m_noReorderWhy) प्रत्यागम;
        m_noReorderWhy = "JumpGo";
        iterateChildren(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः_CCall* nodep) override {
        यदि (!m_graphp || m_noReorderWhy) प्रत्यागम;
        // The callee is not inlined, so the variables it reads and writes carry no VarRef at
        // the call site, and the scoreboard would let assignments to them move across the
        // call. Treat any such call as a barrier.
        m_noReorderWhy = "CCall";
        iterateChildren(nodep);
    }

    शून्य दर्शय(Ast_व्यञ्जकम्_Stmt* nodep) override {
        यदि (!m_graphp || m_noReorderWhy) प्रत्यागम;
        VL_RESTORER(m_अन्तर्_Dly);
        m_अन्तर्_Dly = असत्य;
        iterateChildren(nodep);
    }

    शून्य दर्शय(Ast_नियोजय_Dly* nodep) override {
        यदि (!m_graphp || m_noReorderWhy) प्रत्यागम;
        // An intra-assignment timing control does not block, but it reads variables when the NBA
        // executes, like the value
        यदि (Ast_ग्रन्थिः* ध्रुव controlp = nodep->timingControlp()) iterateChildren(controlp);
        iterate(nodep->rhsp());
        VL_RESTORER(m_अन्तर्_Dly);
        m_अन्तर्_Dly = सत्य;
        iterate(nodep->lhsp());
    }

    शून्य दर्शय(AstMemberSel* nodep) override {
        // A variable selected through a handle can be the same as one accessed otherwise, which
        // is not known here, so the accesses must be kept in order
        यदि (m_graphp) m_noReorderWhy = "Handle";
        दर्शय(static_cast<Ast_ग्रन्थिः*>(nodep));
    }

    शून्य दर्शय(AstVarRef* nodep) override {
        यदि (!m_graphp || m_noReorderWhy) प्रत्यागम;
        यदि (m_stmtStackps.रिक्तानि()) प्रत्यागम;

        // Reads of constants can be ignored - TODO: This should be "constexpr", not run-time const
        यदि (nodep->varp()->isConst()) प्रत्यागम;

        // SPEEDUP: We add duplicate edges, that should be fixed

        AstVarScope* ध्रुव vscp = nodep->varScopep();

        // Create vertexes for variable
        यदि (!vscp->user1p()) vscp->user1p(नूतन ReorderVarStdVertex{m_graphp, vscp});
        ReorderVarStdVertex* ध्रुव vstdp = vscp->user1u().गन्तव्यम्<ReorderVarStdVertex*>();

        // Variable is read
        यदि (nodep->access().isReadOnly()) {
            प्रति (ReorderLogicVertex* ध्रुव vtxp : m_stmtStackps) {
                नूतन ReorderRVEdge{m_graphp, vtxp, vstdp};
            }
            प्रत्यागम;
        }

        // Variable is written, not NBA
        यदि (!m_अन्तर्_Dly) {
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
        ReorderVarPostVertex* ध्रुव vpostp = vscp->user2u().गन्तव्यम्<ReorderVarPostVertex*>();
        प्रति (ReorderLogicVertex* ध्रुव vtxp : m_stmtStackps) {
            नूतन ReorderLVEdge{m_graphp, vpostp, vtxp};
        }
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
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
        यदि (!nodep->is_विशुद्धम्()) {
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
    स्पष्ट ReorderVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ReorderVisitor() override = स्वभाव;
};

}  // namespace

//######################################################################
// V3Reorder class functions

शून्य V3Reorder::reorderAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ReorderVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("reorder", 0, dump_वृक्षः_Either_Level() >= 3);
}
